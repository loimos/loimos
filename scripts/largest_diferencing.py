#!/usr/bin/env python3

import sys
import os
import itertools
import pandas as pd

from numberpartitioning import karmarkar_karp
from utils.id_remapper import remap

def largest_difference(values, num_partitions):
    partitions = [[{v}] + num_partitions * set() for v in values]

def main():
    path = sys.argv[1]
    people = pd.read_csv(os.path.join(path, 'people.csv'))
    locations = pd.read_csv(os.path.join(path, 'locations.csv'))
    visits = pd.read_csv(os.path.join(path, 'visits.csv'))
    num_parts = int(sys.argv[2])

    results = karmarkar_karp(list(locations['max_simultaneous_visits']),
            num_parts=num_parts, return_indices=True)
    permutation = list(itertools.chain(*results.partition))

    remap(people, locations, visits, people['pid'],
            locations.iloc[permutation]['lid'])
    locations.sort_values(by='lid', inplace=True)

    print(locations.columns)
    print(locations)

    people.to_csv('people.csv', index=False)
    locations.to_csv('locations.csv', index=False)
    visits.to_csv('visits.csv', index=False)

if __name__ == '__main__':
    main()
