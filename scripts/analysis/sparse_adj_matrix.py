import pandas as pd
import numpy as np
from scipy.sparse import csr_matrix
import sys

# Usage - sparse_adj_matrix.py (visits.csv file path)

arguments = sys.argv

try:
    file_name = arguments[1]
except IndexError:
    raise IndexError("Provide the path to a visits.csv file as an argument please.")

try:
    df = pd.read_csv(file_name)
    print("Loaded CSV " + file_name + "\n", flush=True)
except:
    raise FileNotFoundError("Please provide a valid path to the visits CSV file as a command line argument.\n") 

print("Starting calculation of total visit durations over 7 days. \n", flush=True)
total_durations = dict()
for index, row in df.iterrows():
    if row['pid'] in total_durations:
        if row['lid'] in total_durations.get(row['pid']):
            total_durations[row['pid']][row['lid']] += row['duration']
        else:
            total_durations[row['pid']][row['lid']] = row['duration']
    else:
        total_durations[row['pid']] = dict() 
        total_durations[row['pid']][row['lid']] = row['duration']

print("Finished calculating durations. There are a total of " + str(len(total_durations)) + " unique people who visited at least one location. \n", flush=True)

print("Beginning conversion to CSR sparse adjacency matrix. \n", flush=True)

values = []
columns = []
rows = []

row_ptr = 0
for pid in total_durations:
    rows.append(row_ptr)
    sorted_lids = sorted(total_durations[pid])
    for lid in sorted_lids:
        weight = total_durations[pid][lid]
        values.append(weight)
        columns.append(lid)
        row_ptr += 1

adj_matrix = csr_matrix((values, columns, rows))

print("Finished converting to CSR. \n", flush=True)