#!/usr/bin/env python3
from os import system
import subprocess
import os
import numpy as np
import pandas as pd
from EffectiveResistanceSampling.Network import Network
import networkx as nx
import pickle
import argparse
from time import perf_counter
from multiprocessing import pool
from rich.progress import *

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
    # fix mispelling, should be approximate
    parser.add_argument(
        "resultant_sample_size",
        metavar="Q",
        help="approximate percentage of number of edges to maintain in the sparsified network",
    )

    parser.add_argument(
        "-s", "--split",
        help="Run sparsification on S separate equally-sized subintervals",
    )

    parser.add_argument(
        "-p", "--parallelize",
        action="store_true",
        help="If -s or --split is set to an integer value, run the [-s/--split value] subinterval calculations in parallel rather than in serial sequence",
    )

    parser.add_argument(
        "--process_count",
        help="Number of processes to use for parallelization. Default is the number of subintervals specified by -s/--split. ",
    )

    parser.add_argument(
        "-v", "--visual",
        action="store_true",
        help="show visual progress indicators of thread execution",
    )

    parser.add_argument(
        "-t", "--test-mode",
        action="store_true",
        help="run in experimental/debug mode in which only first 10000 lines of visits.csv are read",
    )

    return parser.parse_args()


def process_subset(df_subset, q, thread_results, thread_idx, progressbar, progresstask):
    if args.visual:
        progressbar.update(progresstask, advance=1)
    if thread_results is not None and thread_idx is not None:
        start_time = perf_counter()
    edge_list = df_subset[['pid', 'lid']].to_numpy()  # should be 2 x m shape
    weights = df_subset['duration'].to_numpy()  # weight edge by visit duration

    if args.visual:
        subtask = progressbar.add_task(f"[cyan]interval {thread_idx}[/cyan] -- network init", total=1.0)
        network = Network(edge_list, weights, progress_bar=progressbar, progress_task=subtask)
        progressbar.remove_task(subtask)
    else:
        network = Network(edge_list, weights)
        # NOTE: ask abt undirected vs directed for e-list to adj list


    epsilon = 0.1
    method = 'kts'
    if args.visual:
        progressbar.update(progresstask, advance=1)
        subtask = progressbar.add_task(f"[cyan]interval {thread_idx}[/cyan] -- effective resistance", total=1.0)
        Effective_R = network.effR(epsilon, method, progress_bar=progressbar, progress_task=subtask)
        progressbar.remove_task(subtask)
    else:
        print("running effective resistance", flush=True)
        Effective_R = network.effR(epsilon, method)
        print("effective resistance complete", flush=True)

    if args.visual:
        subtask = progressbar.add_task(f"[cyan]interval {thread_idx}[/cyan] -- network.spl", total=1.0)
        EffR_Sparse = network.spl(q, Effective_R, progressbar, subtask, seed=2020)
        progressbar.remove_task(subtask)
        progressbar.update(progresstask, advance=1)
    else:
        print("running network.spl", flush=True)
        EffR_Sparse = network.spl(q, Effective_R, seed=2020)
        print("network.spl complete", flush=True)

    filtered_df_subset = df_subset[df_subset[['pid', 'lid']].apply(tuple, axis=1).isin(map(tuple, EffR_Sparse.E_list))]

    if thread_results is not None and thread_idx is not None:
        end_time = perf_counter()
        thread_results[thread_idx] = filtered_df_subset
        print(f"worker thread {thread_idx} completed in {end_time - start_time :0.2f} seconds", flush=True)
    if args.visual:
        progressbar.update(progresstask, advance=1)
    return filtered_df_subset

def main():
    global args
    args = parse_args()

    if not os.path.exists(args.input_dir):
        print(f'input directory not found: {args.input_dir}', flush=True)
        raise FileNotFoundError(args.input_dir)

    input = os.path.join(args.input_dir, 'visits.csv')
    if args.test_mode:
        df = pd.read_csv(input, nrows=10000)
    else:
        df = pd.read_csv(input)

    # if args.visual:
        # if subprocess.run(['git', 'checkout', 'debug/rich-progresss-bars'], cwd=fr'{os.path.dirname(os.path.realpath(__file__))}/EffectiveResistanceSampling').returncode != 0:
            # raise Exception("git checkout failed")
    # elif subprocess.run(['git', 'checkout', 'older-pythons'], cwd=fr'{os.path.dirname(os.path.realpath(__file__))}/EffectiveResistanceSampling').returncode != 0:
        # raise Exception("git checkout failed")
    # print("EffectiveResistanceSampling repo updated")


    with Progress(TextColumn("[progress.description]{task.description}"),
                  BarColumn(), TaskProgressColumn(),
                  TimeElapsedColumn()) as progress_bar:
        if args.split is not None:
            num_subsets = int(args.split)

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
            df['duration'].mask(
                                subset_num(df['start_time']) != subset_num(df['end_time']), 
                                subset_size * (subset_num(df['start_time']) + 1) - df['start_time'], inplace=True)
            df['end_time'].mask(
                                subset_num(df['start_time']) != subset_num(df['end_time']), 
                                df['start_time'] + df['duration'], inplace=True)
            # TODO: spawn split-off days into other subset dataframes
            
            filtered_dfs = []
            threads = []
            results = [None] * len(subsets)

            start = perf_counter()
            with pool.Pool(processes=int(args.process_count)) as p:
                tasks = []
                for i, subset in subsets:
                    if args.visual:
                        progress_task = progress_bar.add_task(f"[cyan]interval {i}[/cyan] ([bold]{len(subset)}[/bold] rows)", total=4)
                    else:
                        print(f'Initializing interval {i} worker process', flush=True)
                    q = int(float(args.resultant_sample_size) * len(subset))
                    t_args = (subset, q, results, i, progress_bar, progress_task) if args.visual else (subset, q, results, i, None, None)
                    tasks.append(p.apply_async(process_subset, t_args))

                for task in tasks:
                    task.wait()
                    filtered_dfs.append(task.get())

                if not args.parallelize:
                    threads[i].join()

            if args.parallelize:
                for _, thread in enumerate(threads):
                    thread.join()

            print(f'Initializing interval {i+1} worker thread', flush=True)
            for _, df_subset in enumerate(results):
                filtered_dfs.append(df_subset)

            end = perf_counter()
            final_filtered_df = pd.concat(filtered_dfs)
        else:
            q = int(float(args.resultant_sample_size) * float(len(df)))
            final_filtered_df = process_subset(df, q, None, None)

        if not os.path.exists(args.output_dir):
            os.makedirs(args.output_dir)

        final_filtered_df.to_csv(os.path.join(args.output_dir, 'visits.csv'), index=False)
        print(f'complete: {os.path.join(args.output_dir, "visits.csv")}', flush=True)

if __name__ == "__main__":
    main()
