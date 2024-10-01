import os
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

    input = os.path.join(args.input_dir, 'visits.csv')
    df = pd.read_csv(input, usecols=['pid', 'lid', 'duration'])

    edge_list = df[['pid', 'lid']].to_numpy()  # should be 2 x m shape

    # weight edge by visit duration
    weights = df['duration'].to_numpy()  

    print(f'sparsifying network of {len(df)} visit edges')
    network = Network(edge_list, weights)
    epsilon=0.1
    method='kts'

    print(f'calculating effective resistance with epsilon={epsilon} and method={method}')
    Effective_R = network.effR(epsilon, method)

    # sparsifies the network using effective resistance measure calculated above
    q = 10000
    print(f'sparsifying network with {q} samples')
    EffR_Sparse = network.spl(q, Effective_R, seed=2020)
    print(f'sparsified; resulting network has {len(EffR_Sparse.edge_list)} edges')

    filtered_df = df[df[['pid', 'lid']].apply(tuple, axis=1).isin(map(tuple, EffR_Sparse.edge_list))]
    if not os.path.exists(args.output_dir):
        os.makedirs(args.output_dir)

    filtered_df.to_csv(os.path.join(args.output_dir, 'visits.csv'), index=False)

    print(f'complete: {os.path.join(args.output_dir, "visits.csv")}')

main()