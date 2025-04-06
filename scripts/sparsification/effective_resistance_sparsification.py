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

import multiprocessing
import platform
import os
import cupy
from cupy import cuda
from mpi4py import MPI


# reference scripts:
# location_herustics.py
#

# CuPy + CUDA Kernels:
# CuPy is a flexible library that provides NumPy-like functionality on GPUs using CUDA. You can write custom CUDA kernels for optimal performance.

# cuBLAS/cuSPARSE/cuSOLVER from CUDA Toolkit:
# NVIDIA’s libraries (cuBLAS, cuSPARSE, cuSOLVER) offer high-performance linear algebra operations, including iterative solvers.

# scikit-cuda or PyCUDA:
# These wrappers around CUDA libraries let you handle allocations and computations manually.

# PETSc or MAGMA for Large-Scale CG:
# These specialized libraries offer high-performance solvers for scientific computing.

MPI.Init()

def worker_function(gpu_id, rank, hostname, *args):
    """Function executed by each worker process."""
    print(f"Rank {rank} on {hostname} using GPU {gpu_id}")
    # with cuda.Device(gpu_id):
    #     cupy.asarray(12345)  # Simple GPU operation
    process_subset(gpu_id, *args)

def run_on_this_node(args_generator):
    """Spawns GPU workers on each node."""
    hostname = platform.node()
    
    # Initialize MPI
    COMM = MPI.COMM_WORLD
    RANK = COMM.Get_rank()
    SIZE = COMM.Get_size()

    # Determine number of GPUs on each node
    num_gpus = cuda.runtime.getDeviceCount()
    local_gpu_id = 0
    #local_gpu_ids = list(range(num_gpus))

    worker_function(local_gpu_id, RANK, hostname, *args_generator(local_gpu_id))

    # Distribute GPUs across nodes
    # global_gpu_id_start = RANK * num_gpus
    # global_gpu_ids = [global_gpu_id_start + i for i in local_gpu_ids]

    # Spawn a worker for each local GPU
    # processes = []
    # for gpu_id in local_gpu_ids:
    #     p = multiprocessing.Process(target=worker_function, args=(gpu_id, RANK, hostname, *(args_generator(gpu_id))))
    #     p.start()
    #     processes.append(p)

    # Wait for all processes to finish
    # for p in processes:
    #     p.join()



def spawn_workers_per_node(args_generator):
    """Spawns GPU workers on each node."""
    hostname = platform.node()
    
    # Initialize MPI
    COMM = MPI.COMM_WORLD
    RANK = COMM.Get_rank()
    SIZE = COMM.Get_size()

    # Determine number of GPUs on each node
    num_gpus = cuda.runtime.getDeviceCount()
    local_gpu_ids = list(range(num_gpus))

    # Distribute GPUs across nodes
    global_gpu_id_start = RANK * num_gpus
    global_gpu_ids = [global_gpu_id_start + i for i in local_gpu_ids]

    # Spawn a worker for each local GPU
    processes = []
    for gpu_id in local_gpu_ids:
        p = multiprocessing.Process(target=worker_function, args=(gpu_id, RANK, hostname, *(args_generator(gpu_id))))
        p.start()
        processes.append(p)

    # Wait for all processes to finish
    for p in processes:
        p.join()

if __name__ == "__main__":
    multiprocessing.set_start_method("forkserver", force=True)
    # spawn_workers_per_node()


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

def process_subset(gpu_id, df_subset, q, epsilon=0.1, method='kts'):
    edge_list = df_subset[['pid', 'lid']].to_numpy()  # should be 2 x m shape
    weights = df_subset['duration'].to_numpy()  # weight edge by visit duration

    # Time the Network constructor
    start = perf_counter()
    network = Network(edge_list, weights)
    network_constructor_time = perf_counter() - start

    # Time the effective resistance calculation
    print("running effective resistance", flush=True)
    start = perf_counter()
    with cuda.Device(gpu_id):
        Effective_R = network.effR(epsilon, method)
    effR_time = perf_counter() - start
    print("effective resistance complete", flush=True)

    # Time the sparsification process
    print("running network.spl", flush=True)
    start = perf_counter()
    print(f"q: {q}, Effective_R: {Effective_R}, seed: 2020", flush=True)
    EffR_Sparse = network.spl(q, Effective_R, seed=2020)
    spl_time = perf_counter() - start
    print("network.spl complete", flush=True)

    print(f"marginal times: {network_constructor_time}, {effR_time}, {spl_time}", flush=True)
    filtered_df_subset = df_subset[df_subset[['pid', 'lid']].apply(tuple, axis=1).isin(map(tuple, EffR_Sparse.E_list))]

    return filtered_df_subset, network_constructor_time, effR_time, spl_time

def main():
    global args
    args = parse_args()

    script_start = perf_counter()

    preprocessing_time = 0
    network_constructor_time = 0
    effR_time = 0
    spl_time = 0

    if not os.path.exists(args.input_dir):
        print(f'input directory not found: {args.input_dir}', flush=True)
        raise FileNotFoundError(args.input_dir)

    input = os.path.join(args.input_dir, 'visits.csv')
    if args.test_mode:
        df = pd.read_csv(input, nrows=1000)
    else:
        df = pd.read_csv(input)


    if args.split is not None:
        num_subsets = cuda.runtime.getDeviceCount() if args.parallelize else args.split

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
        start = perf_counter()
        df['duration'] = df['duration'].mask(
                            subset_num(df['start_time']) != subset_num(df['end_time']),
                            subset_size * (subset_num(df['start_time']) + 1) - df['start_time'])
        df['end_time'] = df['end_time'].mask(
                            subset_num(df['start_time']) != subset_num(df['end_time']),
                            df['start_time'] + df['duration'])
        preprocessing_time = perf_counter() - start
        # TODO: spawn split-off days into other subset dataframes

        filtered_dfs = []
        results = [None] * len(subsets)

        start = perf_counter()
        subset_args = [[subset, int(args.resultant_sample_size * len(subset))] for _, subset in subsets]

        if args.parallelize:
            # spawn_workers_per_node(lambda gpu_id: (subset_args[gpu_id]))
            run_on_this_node(lambda gpu_id: (subset_args[gpu_id]))
        else:
            results = list(starmap(process_subset, subset_args))

        # Combine results and update global times
        filtered_dfs = []
        for result in results:
            filtered_dfs.append(result[0])
            network_constructor_time += result[1]
            effR_time += result[2]
            spl_time += result[3]

        print(f'Spent {perf_counter() - start}s sparsifying data')
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
    times["total"] = end - script_start

    times_path = args.time
    if os.path.exists(times_path):
        times_df = pd.read_csv(times_path)
    else:
        times_df = pd.DataFrame(columns=['network_constructor_time', 'effR_time', 'spl_time', 'total'])

    new_row = {
        'preprocessing_time': preprocessing_time,
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
