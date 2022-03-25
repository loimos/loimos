#!/usr/bin/env python3

import sys
import os
import math
import itertools
import numpy as np
import pandas as pd
import functools
import argparse

from utils.ids import partition_df
from multiprocessing import Pool, set_start_method

def parse_args():
    parser = argparse.ArgumentParser()

    # Positional/required args
    parser.add_argument('input_dir', metavar='I',
            help='The path to the directory containing the input files')
    
    # Named/optional arguments:
    parser.add_argument('-n', '--n-tasks', default=1, type=int,
        help='Specifies the number of processes to use (default is serial)')

    return parser.parse_args()

def partitioned_merge(left, right, on, n_tasks=1, num_partitions=128, args={}):
    left_partitions = partition_df(left, on=on, num_partitions=num_partitions)
    right_partitions = partition_df(right, on=on, num_partitions=num_partitions)

    merged_partitions = []
    if 1 == n_tasks:
        merged_partitions = [left_partitions[i].merge(
            right_partitions[i], on=on, **args
        ) for i in range(num_partitions-1)]
    else:
        with Pool(n_tasks) as pool:
            merged_partitions = pool.starmap(
                functools.partial(pd.DataFrame.merge, on=on, **args),
                zip(left_partitions, right_partitions))

    merged_df = pd.concat(merged_partitions)

    return merged_df

def main():
    # Parse args and read data
    args = parse_args()
    input_dir = args.input_dir
    n_tasks = args.n_tasks
    
    people = pd.read_csv(os.path.join(input_dir, 'people.csv'))
    locations = pd.read_csv(os.path.join(input_dir, 'locations.csv'))
    visits = pd.read_csv(os.path.join(input_dir, 'visits.csv'))

    print('Make sure ids are contiguous')
    assert(np.all(people['pid'] == people.index))
    assert(np.all(locations['lid'] == locations.index))

    print('Make sure all locations and people referenced in visits exist')
    set_start_method('spawn')
    visitors = partitioned_merge(people, visits, 'pid', n_tasks=n_tasks,
            args={'how': 'inner', 'validate': 'one_to_many'})
    visited = partitioned_merge(locations, visits, 'lid', n_tasks=n_tasks,
            args={'how': 'inner', 'validate': 'one_to_many'})
    #people.merge(visits, on='pid', how='inner',
    #        validate='one_to_many')
    #visited = locations.merge(visits, on='lid', how='inner',
    #        validate='one_to_many')
    print(f'  total visits: {visits.shape[0]}')
    print(f'  visits with valid pid: {visitors.shape[0]}')
    print(f'  visits with valid lid: {visited.shape[0]}')
    assert(visitors.shape[0] == visits.shape[0])
    assert(visited.shape[0] == visits.shape[0])

    print('All tests passed!')

if __name__ == '__main__':
    main()
