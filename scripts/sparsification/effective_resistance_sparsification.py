import numpy as np
import pandas as pd
from EffectiveResistanceSampling.Network import Network
import networkx as nx
import pickle
import argparse

def parse_args():
    parser = argparse.ArgumentParser()

    # Positional/required arguments:
    parser.add_argument(
        "input_dir",
        metavar="I",
        help="The path to directory containing data files for a population",
    )
    parser.add_argument(
        "output_dir",
        metavar="O",
        help="The directory in which the output files should be saved",
    )

    return parser.parse_args()

# name result file visits.csv
def main():
    args = parse_args()

    input = 'visits.csv'
    df = pd.read_csv(input, usecols=['pid', 'lid'])

    edge_list = df[['pid', 'lid']].to_numpy()  # Transpose to get 2 x m shape
    print('read visits.csv')

    # TODO: weights as visit durations
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