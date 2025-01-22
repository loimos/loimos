#!/usr/bin/env python3
import os
import numpy as np
import pandas as pd
from EffectiveResistanceSampling.Network import Network
import networkx as nx
import pickle
import argparse
from time import perf_counter
from threading import Thread
from rich.progress import *

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
        help="approxomate percentage of number of edges to maintain in the sparsified network",
    )

    parser.add_argument(
        "-s", "--separate-days",
        action="store_true",
        help="Run sparsification on each day separately and reassemble into final csv",
    )

    parser.add_argument(
        "-v", "--visual",
        action="store_true",
        help="show visual progress indicators of thread execution",
    )


    return parser.parse_args()


def process_subset(df_subset, q, thread_results, thread_idx, progressbar, progresstask):
    progressbar.update(progresstask, advance=1)
    if thread_results is not None and thread_idx is not None:
        start_time = perf_counter()
    edge_list = df_subset[['pid', 'lid']].to_numpy()  # should be 2 x m shape
    weights = df_subset['duration'].to_numpy()  # weight edge by visit duration

    network = Network(edge_list, weights)
    epsilon = 0.1
    method = 'kts'
    progressbar.update(progresstask, advance=1)

    Effective_R = network.effR(epsilon, method)
    EffR_Sparse = network.spl(q, Effective_R, seed=2020)
    progressbar.update(progresstask, advance=1)

    filtered_df_subset = df_subset[df_subset[['pid', 'lid']].apply(tuple, axis=1).isin(map(tuple, EffR_Sparse.E_list))]

    if thread_results is not None and thread_idx is not None:
        end_time = perf_counter()
        thread_results[thread_idx] = filtered_df_subset
        print(f"worker thread {thread_idx} completed in {end_time - start_time :0.2f} seconds")
    progressbar.update(progresstask, advance=1)
    return filtered_df_subset

def main():
    args = parse_args()

    print(f'parsing input data')
    if not os.path.exists(args.input_dir):
        print(f'input directory not found: {args.input_dir}')
        raise FileNotFoundError(args.input_dir)

    input = os.path.join(args.input_dir, 'visits.csv')
    df = pd.read_csv(input)

    with Progress(TextColumn("[progress.description]{task.description}"),
                  BarColumn(), TaskProgressColumn(),
                  TimeElapsedColumn()) as progress_bar:
        if args.separate_days:
            print(f'Splitting data into subsets by day')
            subsets = df.groupby('daynum')

            if args.visual:
                preprocessing_task = progress_bar.add_task(f"[green]splitting data into [bold]7 subsets", total=1.0)
            day_keys = sorted(subsets.groups.keys())
            unit_scaling = 1 / (60 * 60) # convert seconds to hours
            for daynum in day_keys[:-1]: 
                # ignore the very last day 
                # as we don't have a dataframe 
                # which succeeds it to cull overlap 
                # days from

                day_df = subsets.get_group(daynum).copy()
                for idx, row in day_df.iterrows():
                    if args.visual:
                        progress_bar.update(preprocessing_task, advance=(1.0 / (len(day_keys) * len(day_df))))
                    end_time = row['start_time'] + row['duration']
                    # assume 24-hour boundary; adjust as needed
                    if end_time * unit_scaling > 24:
                        new_duration = 24 - row['start_time']
                        # truncate duration for this day
                        df.loc[idx, 'duration'] = new_duration
                        # remove from subsequent days
                        for future_day in day_keys[day_keys.index(daynum)+1:]:
                            if idx in subsets.get_group(future_day).index:
                                df.drop(idx, inplace=True)
            if args.visual:
                progress_bar.remove_task(preprocessing_task)
            filtered_dfs = []
            threads = []
            results = [None] * len(subsets)

            # python3 effective_resistance_sparsification.py -s ~/src/data/coc-unsparsified ~/src/data-spars-para/coc-sparsified-90 0.9

            start = perf_counter()
            for i, subset in subsets:
                if args.visual:
                    progress_task = progress_bar.add_task(f"[cyan]day {i}", total=4)
                else:
                    print(f'Initializing day {i+1} worker thread')
                q = int(float(args.resultant_sample_size) * float(len(subset)))
                t_args = (progress_bar, progress_task, subset, q, results, i) if args.visual else (subset, q, results, i)
                threads.append(Thread(target=process_subset, args=t_args))
                threads[i].start()

            for _, thread in enumerate(threads):
                thread.join()

            print(f'Initializing day {i+1} worker thread')
            for _, df_subset in enumerate(results):
                filtered_dfs.append(df_subset)

            # TODO: cull multi-day visits
            end = perf_counter()
            final_filtered_df = pd.concat(filtered_dfs)
        else:
            q = int(float(args.resultant_sample_size) * float(len(df)))
            final_filtered_df = process_subset(df, q, None, None)

        if not os.path.exists(args.output_dir):
            os.makedirs(args.output_dir)

        final_filtered_df.to_csv(os.path.join(args.output_dir, 'visits.csv'), index=False)
        print(f'complete: {os.path.join(args.output_dir, "visits.csv")}')

if __name__ == "__main__":
    main()
