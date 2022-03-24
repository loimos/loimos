#!/usr/bin/env python3

import sys
import os
import math
import itertools
import numpy as np
import pandas as pd
import argparse

from utils.ids import partition_df

def parse_args():
    parser = argparse.ArgumentParser()

    # Positional/required args
    parser.add_argument('input_dir', metavar='I',
            help='The path to the directory containing the input files')

    return parser.parse_args()

def partitioned_merge(left, right, on, num_partitions=128, args={}):
    left_partitioned = partition_df(left, on=on, num_partitions=num_partitions)
    right_partitioned = partition_df(right, on=on, num_partitions=num_partitions)

    merged_partitioned = [left_partitioned[i].merge(
        right_partitioned[i], on=on, **args
    ) for i in range(num_partitions-1)]
    merged_df = pd.concat(merged_partitioned)

    return merged_df

def main():
    # Parse args and read data
    args = parse_args()
    input_dir = args.input_dir
    
    people = pd.read_csv(os.path.join(input_dir, 'people.csv'))
    locations = pd.read_csv(os.path.join(input_dir, 'locations.csv'))
    visits = pd.read_csv(os.path.join(input_dir, 'visits.csv'))

    print('Make sure ids are contiguous')
    assert(np.all(people['pid'] == people.index))
    assert(np.all(locations['lid'] == locations.index))

    print('Make sure all locations and people referenced in visits exist')
    visitors = partitioned_merge(people, visits, 'pid',
            args={'how': 'inner', 'validate': 'one_to_many'})
    visited = partitioned_merge(locations, visits, 'lid',
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
