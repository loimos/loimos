#!/usr/bin/env python3

import sys
import os
import itertools
import numpy as np
import pandas as pd
import pprint

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

    # Partition data and combine new indices into a single list for easier
    # indexing
    results = karmarkar_karp(list(locations['max_simultaneous_visits']),
            num_parts=num_parts, return_indices=True)
    print('parition lengths:', [len(p) for p in results.partition])
    permutation = np.fromiter(itertools.chain(*results.partition), int)

    # Remap expects an array of new indices in the same order as the orignal
    # dataframe, so we need to convert from the order the indices appear in
    # the output to the order they appear in the input
    cauchy_perm = np.array([locations.iloc[permutation]['lid'], permutation])
    cauchy_perm = cauchy_perm.transpose()
    cauchy_perm = cauchy_perm[cauchy_perm[:, 0].argsort()]
    print('(lid, new lid) pairs:')
    print(cauchy_perm)

    #print('indices:', locations.index)
    #print('permutation:', permutation)
    #print('permuted indices:', locations.index[permutation])
    people, locations, visits = remap(people, locations, visits, people['pid'],
            cauchy_perm[:,1])
    # Loimos expects partitions to be in order, so we need to sort the data
    # to reflect the new indicies before writing everythign out
    locations.sort_values(by='lid', inplace=True)

    #print(locations.columns)
    #print(locations)

    people.to_csv('people.csv', index=False)
    locations.to_csv('locations.csv', index=False)
    visits.to_csv('visits.csv', index=False)

if __name__ == '__main__':
    main()
