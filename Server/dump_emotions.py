# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import os
import json
from dateutil import tz
from collections import namedtuple

from azure.cosmosdb.table import TableService

script_dir = os.path.dirname(os.path.abspath(__file__))


def dump_emotions(config, partition):
    query = "FaceCount gt 0 and PartitionKey eq '" + partition + "'"
    all_sentiments = sorted(config.colors_for_emotions.keys())
    headers = ["Timestamp"]
    for sentiment in all_sentiments:
        headers += [sentiment]

    filename = partition + ".csv"
    with open(filename, "w") as log:
        log.write(",".join(headers) + "\n")

        storage_account = TableService(connection_string=config.connection_string)

        count = 0
        try:
            query_results = list(
                storage_account.query_entities(
                    "Psi", filter=query, num_results=1000, timeout=30
                )
            )
            for entity in query_results:
                for x in ["Face1", "Face2", "Face3", "Face4", "Face5"]:
                    d = entity["Timestamp"]
                    localtime = d.astimezone(tz.tzlocal())
                    row = [str(localtime)]
                    for sentiment in all_sentiments:
                        key = "{}_{}".format(x, sentiment)
                        if key in entity:
                            row += [entity[key]]
                    if len(row) > 1:
                        log.write(",".join(row) + "\n")
                        count += 1
        except Exception as e:
            print(e)

    print("wrote {} records to {}".format(count, filename))


def dump_emotions_from_all_cameras(config):
    for zone in config.camera_zones:
        for cam in zone:
            dump_emotions(config, cam)


if __name__ == "__main__":
    with open(os.path.join(script_dir, "config.json"), "r") as f:
        d = json.load(f)
        config = namedtuple("Config", d.keys())(*d.values())

    dump_emotions_from_all_cameras(config)
