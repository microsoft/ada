#!/usr/bin/env python3
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import _thread
import argparse
import json
import os
import time
from collections import namedtuple

import numpy as np
import pandas as pd
from azure.cosmosdb.table import TableService

import priority_queue

script_dir = os.path.dirname(os.path.abspath(__file__))


class Sensei:
    """
    A class that provides emotion data from Sensei cosmos database or from
    pre-recorded *.csv files on disk
    """

    def __init__(self, camera_zones, emotions, connection_string, debug=False):
        """
        Initialize Sensei with a list of camera tuples, each tuple represents a 'zone' containing
        one or more cameras, the values will be maximized across those cameras.  Also provide
        the list of emotions we expect to be getting from the cosmos datbase at the Azure
        connetion string.
        """
        self.camera_zones = camera_zones
        self.debug = debug
        self.emotions = emotions
        self.new_emotions = priority_queue.PriorityQueue()
        self.playback_row = 0
        self.playback_delay = 0
        self.start_time = 0
        self.previous_emotions = None
        self.recorded_data = None
        self.play_recording = False
        self.stopping = False
        self.running = False
        self.storage_account = None
        self.connection_string = connection_string

    def start(self):
        if not self.running:
            if not self.connection_string:
                print("Missing Sensei connection string")
                return

            try:
                self.storage_account = TableService(
                    connection_string=self.connection_string
                )
            except:
                print("Cannot connect to Sensei storage.")
                return

            self.running = True
            self.stopping = False
            self.start_time = time.time()
            _thread.start_new_thread(
                self._fetch_data_and_compute_emotions_perodically, ()
            )

    def stop(self):
        self.stopping = True

    def get_next_emotions(self):
        """read next set of emotions from the queue or return None if nothing has arrived yet.
        Also drain the queue so stuff doesn't built up if the server stops calling this for a while
        """
        data = None
        while self.new_emotions.size() > 0:
            priority, data = self.new_emotions.dequeue()
        return data

    def load(self, history_dir, playback_delay=10, playback_weights=None):
        """
        Load the historical data from the given directory, and play back each row
        at the given playback_delay in seconds.
        """
        self.recorded_data = list()
        self.playback_delay = playback_delay
        self.playback_weights = playback_weights
        self.play_recording = True
        self.max_rows = 0
        for i in range(len(self.camera_zones)):
            zone = self.camera_zones[i]
            for name in zone:
                filename = os.path.join(
                    history_dir, "Grouped_by_Hour_{}.csv".format(name)
                )
                print("Loading: {}".format(filename))
                temp = pd.read_csv(filename)
                rows = temp.shape[0]
                if rows > self.max_rows:
                    self.max_rows = rows
                self.recorded_data.append(temp)

    def _max_of_camera_dicts(self, camera_dict1, camera_dict2):
        """
        Return the maximum values for each key in the given pair of dictionaries.
        :param camera_dict1: the first camera dictionary
        :param camera_dict2: the second camera dictionary
        :return: a dictionary containing the maximum of the two values for each key
        """
        result = {}
        for key in camera_dict1:
            v1 = camera_dict1[key]
            result[key] = v1
            if key in camera_dict2:
                v2 = camera_dict2[key]
                if v2 > v1:
                    result[key] = v2
        for key in camera_dict2:
            if key not in result:
                result[key] = camera_dict2[key]
        return result

    def _read_face_sentiment_data(self, partition):
        """Reads sentiment data table from Azure for specified partition.
        Returns a dictionary containing sentiment data.

        Inputs:
        partition = within Azure sentiment data, the specific partition
                    being analyzed. i.e. all cameras from the first floor of
                    the building.

        Note: We want to be able to associate a particular partition of data
            with a specific series of LEDs (a "zone") in the installation.
            i.e. Data from the first floor illuminates LEDs in the lower left
            quadrant of the installation."""

        query = "FaceCount gt 0 and PartitionKey eq '" + partition + "'"
        all_sentiments = sorted(self.emotions)
        face_sentiment = {x: 0 for x in all_sentiments}

        try:
            start = time.time()
            query_results = list(
                self.storage_account.query_entities(
                    "Psi", filter=query, num_results=20, timeout=5
                )
            )
            stop = time.time()
            if stop - start > 5:
                print(
                    "### Cosmos is slow, {} took {} seconds".format(
                        partition, stop - start
                    )
                )

            for entity in query_results:
                for x in ["Face1", "Face2", "Face3", "Face4", "Face5"]:
                    for sentiment in all_sentiments:
                        key = "{}_{}".format(x, sentiment)
                        # not all samples have emotion data
                        if key in entity:
                            sentiment_score = float(entity["%s_%s" % (x, sentiment)])
                            if face_sentiment[sentiment] == 0:
                                face_sentiment[sentiment] = sentiment_score
                            else:
                                # average
                                # face_sentiment[sentiment] = (face_sentiment[sentiment] + sentiment_score) / 2
                                # take max
                                face_sentiment[sentiment] = max(
                                    face_sentiment[sentiment], sentiment_score
                                )

        except Exception as e:
            print("### error in _read_face_sentiment_data", e)

        # face_sentiment is an average sentiment array over all samples (and faces)
        # e.g. {'Anger': 0.002378021,
        #       'Contempt': 0.0001994435,
        #       'Disgust': 7.647883e-05,
        #       'Fear': 0.0001017679,
        #       'Happiness': 0.8633057,
        #       'Neutral': 0.9993325,
        #       'Sadness': 0.0004095407,
        #       'Surprise': 0.007230375}
        return face_sentiment

    def _read_face_sentiment_data_file(self, partition):
        """Reads sentiment data from recorded_data.
        Returns a dictionary containing sentiment data.

        Inputs:
        partition = within Azure sentiment data, the specific partition
                    being analyzed. i.e. all cameras from the first floor of
                    the building.

        Note: We want to be able to associate a particular partition of data
            with a specific series of LEDs (a "zone") in the installation.
            i.e. Data from the first floor illuminates LEDs in the lower left
            quadrant of the installation."""

        all_sentiments = sorted(self.emotions)
        face_sentiment = {x: 0 for x in all_sentiments}

        cam_list = [item for t in self.camera_zones for item in t]
        camera_no = cam_list.index(partition)

        try:
            for sentiment in all_sentiments:
                # not all samples have emotion data
                if self.playback_row >= self.recorded_data[camera_no].shape[0]:
                    face_sentiment[sentiment] = self.recorded_data[camera_no].iloc[0][
                        sentiment
                    ]
                else:
                    face_sentiment[sentiment] = self.recorded_data[camera_no].iloc[
                        self.playback_row
                    ][sentiment]
        except Exception as e:
            print("### error in _read_face_sentiment_data_file:", e)

        # scale by the weights
        if self.playback_weights is not None:
            for key in self.playback_weights:
                scale = self.playback_weights[key]
                if key in face_sentiment:
                    face_sentiment[key] *= scale

        # face_sentiment is an average sentiment array over all samples (and faces)
        return face_sentiment

    def _compute_most_probable_dicts(self):
        zone_dict_list = []
        camera_results = {}
        for camera_pair in self.camera_zones:
            max_dict = {}
            for camera in camera_pair:
                if camera in camera_results:
                    # don't query the same camera more than once per iteration
                    camera_dict = camera_results[camera]
                else:
                    if self.play_recording:
                        camera_dict = self._read_face_sentiment_data_file(camera)
                    else:
                        camera_dict = self._read_face_sentiment_data(camera)

                    camera_results[camera] = camera_dict
                max_dict = self._max_of_camera_dicts(max_dict, camera_dict)

            zone_dict_list.append(max_dict)

        if self.play_recording and self.start_time + self.playback_delay < time.time():
            self.playback_row += 1
            self.start_time = time.time()
            if self.playback_row >= self.max_rows:
                self.playback_row = 0

        # return an emotion dict for each zone.
        return zone_dict_list

    def _get_highest_emotions_and_values(self, zone_dict_list, except_list=[]):
        # return the maximum emotion for each zone.
        result = []
        for row in zone_dict_list:
            keys = list(row.keys())
            values = list(row.values())
            for e in except_list:
                if e in keys:
                    i = keys.index(e)
                    del keys[i]
                    del values[i]
            i = np.argmax(values)
            result += [(keys[i], values[i])]
        return result

    def _compute_emotion_by_zone(self):
        zone_dict_list = self._compute_most_probable_dicts()
        # emotion with the highest score for each camera zone, not including the overly dominant 'Neutral' emotion
        highest_other_emotion_and_values_per_zone = (
            self._get_highest_emotions_and_values(zone_dict_list, ["neutral"])
        )
        # now include the neutral emotion.
        highest_emotion_and_values_per_zone = self._get_highest_emotions_and_values(
            zone_dict_list
        )

        i1 = np.argmax([x[1] for x in highest_other_emotion_and_values_per_zone])
        i2 = np.argmax([x[1] for x in highest_emotion_and_values_per_zone])
        s1 = highest_other_emotion_and_values_per_zone[i1][1]
        s2 = highest_emotion_and_values_per_zone[i2][1]

        if s1 > s2 / 4:
            # then favor the other emotion over the more boring 'Neutral' emotions
            highest_emotion_and_values_per_zone = (
                highest_other_emotion_and_values_per_zone
            )

        zones = []
        for highest_ev in highest_emotion_and_values_per_zone:
            zones += [highest_ev[0]]

        return zones

    def _fetch_data_and_compute_emotions_perodically(self):
        while not self.stopping:
            new_emotions = self._compute_emotion_by_zone()
            if self.previous_emotions is None or new_emotions != self.previous_emotions:
                self.previous_emotions = new_emotions
                self.new_emotions.enqueue(0, new_emotions)
            time.sleep(5)  # 5 second so we don't hit cosmos too hard
            # todo: replace this with an SignalR WebSocket so server can
            # push new emotions in realtime.
        self.running = False


if __name__ == "__main__":
    parser = argparse.ArgumentParser("Sensei returns emotions from Sensei database")
    parser.add_argument(
        "--loop",
        help="Loop values from a file or not (default 'false' (Not))",
        action="store_true",
    )

    with open(os.path.join(script_dir, "config.json"), "r") as f:
        d = json.load(f)
        config = namedtuple("Config", d.keys())(*d.values())

    emotions = [key for key in config.colors_for_emotions]

    sensei = Sensei(
        config.camera_zones, emotions, config.connection_string, config.debug
    )
    args = parser.parse_args()
    if args.loop:
        history_files = os.path.join(os.path.join(script_dir, config.history_dir))
        sensei.load(history_files, config.playback_delay, config.playback_weights)

    sensei.start()
    while True:
        result = sensei.get_next_emotions(10)
        if result is not None:
            print(result)
