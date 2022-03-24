#!/usr/bin/env python3

import sys
import os
import math
import itertools
import numpy as np
import pandas as pd
import argparse

def parse_args():
    parser = argparse.ArgumentParser()

    # Positional/required args
    parser.add_argument('input_dir', metavar='I',
            help='The path to the directory containing the input files')

    return parser.parse_args()

def main():
    # Parse args and read data
    args = parse_args()
    input_dir = args.input_dir
    
    people = pd.read_csv(os.path.join(input_dir, 'people.csv'))
    locations = pd.read_csv(os.path.join(input_dir, 'locations.csv'))
    visits = pd.read_csv(os.path.join(input_dir, 'visits.csv'))

    # Make sure ids are contiguous
    assert(np.all(people['pid'] == people.index))
    assert(np.all(locations['lid'] == locations.index))

    # Make sure all locations and people referenced in visits exist
    visitors = people.merge(visits, on='pid', how='inner',
            validate='one_to_many')
    assert(visitors.shape[0] == visits.shape[0])
    visited = locations.merge(visits, on='lid', how='inner',
            validate='one_to_many')
    assert(visited.shape[0] == visits.shape[0])

    print('All tests passed!')

if __name__ == '__main__':
    main()
