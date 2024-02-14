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

print("Aggregating dataframe by grouping by pid and lid and adding durations over 7 days. \n", flush=True)
aggregated_df = df.groupby(['pid', 'lid'])['duration'].sum().reset_index()

print("Finished aggregating, converting to a COO matrix. \n", flush=True)
coo = coo_matrix((aggregated_df['duration'], (aggregated_df['pid'], aggregated_df['lid'])))

print("Converting COO to CSR now. \n", flush=True)
adj_matrix = coo.tocsr()
print("Finished converting to CSR. \n", flush=True)
print(vars(adj_matrix))
