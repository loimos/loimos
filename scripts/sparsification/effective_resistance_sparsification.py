import numpy as np
import pandas as pd
from Network import Network
import networkx as nx
import pickle

# name result file visits.csv
df = pd.read_csv('visits.csv', usecols=['pid', 'lid'])
print('read')

edge_list = df[['pid', 'lid']].to_numpy()  # Transpose to get 2 x m shape

# TODO: weights as visit durations
print(edge_list.shape[0])
weights = np.ones(edge_list.shape[0])  # Default weight of 1 for each edge

network = Network(edge_list, weights)
epsilon=0.1
method='kts'

Effective_R = network.effR(epsilon, method)
q = 10000

EffR_Sparse = network.spl(q, Effective_R, seed=2020)

with open('network_original.pkl', 'wb') as outp:
    pickle.dump(network, outp, pickle.HIGHEST_PROTOCOL)

with open('network_sparsified.pkl', 'wb') as outp:
    pickle.dump(EffR_Sparse, outp, pickle.HIGHEST_PROTOCOL)

print('complete')