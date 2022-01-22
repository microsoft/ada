import os
import csv
import pandas as pd
import numpy as np

for name in os.listdir():
    if name.endswith(".csv"):
        print("{}==========================================".format(name))
        temp = pd.read_csv(name)
        for i in range(temp.shape[0]):
            row = temp.iloc[i]

            scores = []
            emotions = ["neutral", "happiness", "sadness", "anger", "fear", "surprise", "disgust", "contempt"]
            for key in emotions:
                scores += [float(row[key])]

            idx = np.argsort(scores)

            #print(["{}({:.2f})".format(se[i], scores[i]) for i in reversed(range(len(idx)))])
            for i in reversed(range(len(idx))):
                if scores[idx[i]] > 0.01:
                    print("{} ({:.2f})".format(emotions[idx[i]], scores[idx[i]]), end=", ")
            print()
