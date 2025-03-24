#!/usr/bin/env python3
from os import system
import subprocess
import os
import numpy as np
import pandas as pd
from EffectiveResistanceSampling.Network import Network
import networkx as nx
import argparse
from time import perf_counter
from multiprocessing import Pool, set_start_method
from itertools import starmap

# reference scripts:
# location_herustics.py
#

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

    parser.add_argument(
        "resultant_sample_size",
        metavar="Q",
        type=float,
        help="approximate percentage of number of edges to maintain in the sparsified network",
    )

    parser.add_argument(
        "-s", "--split",
        type=int,
        default=1,
        help="Run sparsification on S separate equally-sized subintervals",
    )

    parser.add_argument(
        "-p", "--parallelize",
        action="store_true",
        help="If -s or --split is set to an integer value, run the [-s/--split value] subinterval calculations in parallel rather than in serial sequence",
    )

    parser.add_argument(
        "--process_count",
        default=0,
        type=int,
        help="Number of processes to use for parallelization. Default is the number of subintervals specified by -s/--split. ",
    )

    parser.add_argument(
        "--time",
        default="times.csv",
        type=str,
        help="time and store the timing results in this csv file",
    )

    parser.add_argument(
        "-t", "--test-mode",
        action="store_true",
        help="run in experimental/debug mode in which only first 10000 lines of visits.csv are read",
    )

    args = parser.parse_args()

    if args.parallelize and args.process_count == 0:
        args.process_count = args.split


    return args


# Global variables to track timing
network_constructor_time = 0
effR_time = 0
spl_time = 0

def process_subset(df_subset, q, epsilon=0.1, method='kts'):
    global network_constructor_time, effR_time, spl_time

    edge_list = df_subset[['pid', 'lid']].to_numpy()  # should be 2 x m shape
    weights = df_subset['duration'].to_numpy()  # weight edge by visit duration

    # Time the Network constructor
    start = perf_counter()
    network = Network(edge_list, weights)
    network_constructor_time += perf_counter() - start

    # Time the effective resistance calculation
    print("running effective resistance", flush=True)
    start = perf_counter()
    Effective_R = network.effR(epsilon, method)
    effR_time += perf_counter() - start
    print("effective resistance complete", flush=True)

    # Time the sparsification process
    print("running network.spl", flush=True)
    start = perf_counter()
    EffR_Sparse = network.spl(q, Effective_R, seed=2020)
    spl_time += perf_counter() - start
    print("network.spl complete", flush=True)

    filtered_df_subset = df_subset[df_subset[['pid', 'lid']].apply(tuple, axis=1).isin(map(tuple, EffR_Sparse.E_list))]

    return filtered_df_subset

def main():
    global args
    args = parse_args()

    if not os.path.exists(args.input_dir):
        print(f'input directory not found: {args.input_dir}', flush=True)
        raise FileNotFoundError(args.input_dir)

    input = os.path.join(args.input_dir, 'visits.csv')
    if args.test_mode:
        df = pd.read_csv(input, nrows=1000)
    else:
        df = pd.read_csv(input)

    # TODO: profile preprocessing
    # if args.visual:
        # if subprocess.run(['git', 'checkout', 'debug/rich-progresss-bars'], cwd=fr'{os.path.dirname(os.path.realpath(__file__))}/EffectiveResistanceSampling').returncode != 0:
            # raise Exception("git checkout failed")
    # elif subprocess.run(['git', 'checkout', 'older-pythons'], cwd=fr'{os.path.dirname(os.path.realpath(__file__))}/EffectiveResistanceSampling').returncode != 0:
        # raise Exception("git checkout failed")
    # print("EffectiveResistanceSampling repo updated")


    if args.split is not None:
        num_subsets = int(args.split)

        times = {}

        days = df.groupby('daynum')
        num_days = len(days.groups.keys())
        seconds_in_day = (24 * 60.0 * 60.0)

        subset_size = (seconds_in_day * num_days) / num_subsets
        print("Sparsifying:", flush=True)
        print(f"{num_subsets} intervals of length {subset_size} seconds each", flush=True)
        def subset_num(col):
            return np.floor(col / subset_size)

        subsets = df.groupby(subset_num(df['start_time']))

        # temp
        df['duration'] = df['duration'].mask(
                            subset_num(df['start_time']) != subset_num(df['end_time']),
                            subset_size * (subset_num(df['start_time']) + 1) - df['start_time'])
        df['end_time'] = df['end_time'].mask(
                            subset_num(df['start_time']) != subset_num(df['end_time']),
                            df['start_time'] + df['duration'])
        # TODO: spawn split-off days into other subset dataframes

        filtered_dfs = []
        results = [None] * len(subsets)

        start = perf_counter()
        q = int(args.resultant_sample_size * len(subsets))
        subset_args = [[subset, q] for i, subset
                in subsets]

        if args.parallelize:
            with Pool(processes=args.process_count) as p:
                filtered_dfs = p.starmap(process_subset, subset_args)
        else:
            filtered_dfs = starmap(process_subset, subset_args)

        print(f'Spent {start - perf_counter()}s sparcifying data"')
        final_filtered_df = pd.concat(filtered_dfs)
    else:
        q = int(args.resultant_sample_size * float(len(df)))
        final_filtered_df = process_subset(df, q, None, None)

    if not os.path.exists(args.output_dir):
        os.makedirs(args.output_dir)

    final_filtered_df.to_csv(os.path.join(args.output_dir, 'visits.csv'), index=False)

    # Ensure times is a dictionary with proper keys and values
    network_constructor_time /= len(subsets)
    effR_time /= len(subsets)
    spl_time /= len(subsets)

    end = perf_counter()
    times["total"] = end - start

    times_path = args.time
    if os.path.exists(times_path):
        times_df = pd.read_csv(times_path)
    else:
        times_df = pd.DataFrame(columns=['network_constructor_time', 'effR_time', 'spl_time', 'total'])

    new_row = {
        'network_constructor_time': network_constructor_time,
        'effR_time': effR_time,
        'spl_time': spl_time,
        'total': times["total"]
    }

    times_df = pd.concat([times_df, pd.DataFrame([new_row])], ignore_index=True)
    times_df.to_csv(times_path, index=False)

    print(f'complete: {os.path.join(args.output_dir, "visits.csv")}', flush=True)

if __name__ == "__main__":
    main()
