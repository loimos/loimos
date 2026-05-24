import pandas as pd
import sys
import os
import numpy as np
from scipy.sparse import csr_matrix, coo_matrix
import subprocess

# Usage - compute_metrics_exclude_locations.py (path to population directory)

arguments = sys.argv

try:
    in_dir = arguments[1]
except IndexError:
    raise IndexError("Provide the path to a population directory as an argument please.")

try:
    print('Reading in CSV ' + os.path.join(in_dir, 'locations.csv') + "\n", flush=True)
    df = pd.read_csv(os.path.join(in_dir, 'locations.csv'))
    print("Loaded CSV " + os.path.join(in_dir, 'locations.csv') + "\n", flush=True)
except:
    raise FileNotFoundError("Please provide a valid path to a population directory as a command line argument.\n") 

print(f'Original amount of locations: {df.shape[0]}', flush=True)
df = df[df['total_visits'] > 0]
print (f'Number of rows after excluding locations with 0 visits: {df.shape[0]}', flush=True)
msv_med = df['max_simultaneous_visits'].median()
msv_max = df['max_simultaneous_visits'].max()
tv_med = df['total_visits'].median()
tv_max = df['total_visits'].max()
msv_mean = df['max_simultaneous_visits'].mean()
msv_meansq = (df['max_simultaneous_visits'] ** 2).mean()
tv_mean = df['total_visits'].mean()
tv_meansq = (df['total_visits'] ** 2).mean()

print(f"MSVMED: {msv_med}\nMSVMAX: {msv_max}\nTVMED: {tv_med}\nTVMAX: {tv_max}\nMSVMEAN: {msv_mean}\nMSVMEANSQ: {msv_meansq}\nTVMEAN: {tv_mean}\nTVMEANSQ: {tv_meansq}", flush=True)
