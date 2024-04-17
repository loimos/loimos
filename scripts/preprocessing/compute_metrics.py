import pandas as pd
import sys
import os
import numpy as np
from scipy.sparse import csr_matrix, coo_matrix
import subprocess

# Usage - compute_metrics.py (path to population directory) (path to loimos home directory)

arguments = sys.argv

try:
    in_dir = arguments[1]
except IndexError:
    raise IndexError("Provide the path to a population directory as an argument please.")

try:
    loimos_home = arguments[2]
except:
    raise IndexError("Provide the path to your loimos home as your second argument please.")

try:
    df = pd.read_csv(os.path.join(in_dir, 'visits.csv'))
    print("Loaded CSV " + os.path.join(in_dir, 'visits.csv') + "\n", flush=True)
except:
    raise FileNotFoundError("Please provide a valid path to a population directory as a command line argument.\n") 


def matrix_metrics():
    aggregated_df = df.groupby(['pid', 'lid'])['duration'].sum().reset_index()

    print("Finished aggregating, converting to a COO matrix. \n", flush=True)
    coo = coo_matrix((aggregated_df['duration'], (aggregated_df['pid'], aggregated_df['lid'])))

    print("Converting COO to CSR now. \n", flush=True)
    adj_matrix = coo.tocsr()
    print("Finished converting to CSR.", flush=True)
    print("Size of matrix: " + str(adj_matrix.shape) + "\n", flush=True)

    nnz = adj_matrix.count_nonzero()
    annz = adj_matrix.getnnz(axis=1).mean()
    onenorm = np.max(adj_matrix.sum(axis=0))
    frobnorm = np.sqrt(np.sum(np.square(adj_matrix.data)))

    return nnz, annz, onenorm, frobnorm
    
def location_heuristics():
    command = ['python3', os.path.join(loimos_home, 'scripts/preprocessing/location_heuristics.py')] + [in_dir]

    try:
        subprocess.run(command, check=True, stdout=subprocess.PIPE)
    except:
        print("An error occurred while trying to run location heuristics script. Please ensure it is located at loimos_home/scripts/preprocessing/location_heuristics.py", flush=True)

def location_heuristic_metrics():
    try:
        heuristics = pd.read_csv(os.path.join(in_dir, 'locations.heuristic.csv'))
    except:
        raise FileNotFoundError("The location heuristics csv file doesn't exist in the population directory", flush=True)
    
    msvmed = heuristics["max_simultaneous_visits"].median()
    msvmax = heuristics["max_simultaneous_visits"].max()
    tvmed = heuristics["total_visits"].median()
    tvmax = heuristics["total_visits"].max()
    msvmean = heuristics["max_simultaneous_visits"].mean()
    msvmeansq = (heuristics["max_simultaneous_visits"] ** 2).mean()
    tvmean = heuristics["total_visits"].mean()
    tvmeansq = (heuristics["total_visits"] ** 2).mean()

    return msvmed, msvmax, tvmed, tvmax, msvmean, msvmeansq, tvmean, tvmeansq

print(f"Starting all metric calculations for {in_dir} \n \n", flush=True)

print("Calculating all adjacency matrix metrics \n", flush=True)
nnz, annz, onenorm, frobnorm = matrix_metrics()
print(f'Matrix metrics: \n NNZ: {nnz} \n ANNZ: {annz} \n ONENORM: {onenorm} \n FROBNORM: {frobnorm}', flush=True)

print("\nRunning location heuristics script \n", flush=True)
location_heuristics()
print("Done running the location heuristics script \n", flush=True)

print("\nComputing metrics from location heuristics \n", flush=True)
msvmed, msvmax, tvmed, tvmax, msvmean, msvmeansq, tvmean, tvmeansq = location_heuristic_metrics()
print(f'Location heuristics metrics: \n MSVMED: {msvmed} \n MSVMAX: {msvmax} \n TVMED: {tvmed} \n TVMAX: {tvmax} \n MSVMEAN: {msvmean} \n MSVMEANSQ: {msvmeansq} \n TVMEAN: {tvmean} \n TVMEANSQ: {tvmean}', flush=True)

df_data = np.array([nnz, annz, onenorm, msvmed, msvmax, msvmean, msvmeansq, tvmed, tvmax, tvmean, tvmeansq]).reshape(1, -1)
df = pd.DataFrame(data=df_data, columns=['nnz', 'annz', 'onenorm', 'msvmed', 'msvmax', 'msvmean', 'msvmeansq', 'tvmed', 'tvmax', 'tvmean', 'tvmeansq'])

df.to_csv(os.path.join(in_dir, 'metrics.csv'), index=False)
