#!/usr/bin/env python3

import sys
import os
import math
import itertools
import numpy as np
import pandas as pd
import pprint

from numberpartitioning import karmarkar_karp
from utils.id_remapper import remap

def folding_partition(locations, num_partitions):
    partition_width = int(math.ceil(locations.shape[0] / num_partitions))
    
    # Note: the folding partition assumes that the elements are sorted in
    # descending order of weight, however, if we compose the sorting
    # permutation and the folding partition perumatation, we don't need
    # to pre-sort the data in order to compute this partition, since it
    # doesn't depend on the specific values of the items being partitioned
    
    partitions = []
    # We need to handle even and odd widths (values of k in the original
    # paper) seperately
    for i in range(num_partitions):
        cur_partition = []
        # Note that our indiexing is different from the original paper's
        # since we (1) have an explcit loop and (2) are starting our indices
        # at 0 rather than 1
        for j in range(0, partition_width-1, 2):
            cur_partition.append(num_partitions*j + i)
            cur_partition.append(num_partitions*(j + 2) - i - 1)
    
        if partition_width % 2 == 1:
            cur_partition.append((partition_width - 1)*num_partitions + i)

        partitions.append(cur_partition)

    #print('parition lengths:', [len(p) for p in partitions])

    permutation = np.fromiter(itertools.chain(*partitions), int)
    print(permutation)

    #We shouldn't have any duplicate ids
    #duplicate_ids = ids[id_counts > 1]
    #print('duplicate ids:\n', duplicate_ids)

    # The last partition is the only one which can be less than partition_width
    # so fill in the earlier out of bounds elements with elements from it
    out_of_bounds_mask = permutation >= locations.shape[0]
    num_out_of_bounds = np.sum(out_of_bounds_mask)
    permutation[out_of_bounds_mask] = permutation[-num_out_of_bounds:]
    permutation = permutation[:-num_out_of_bounds]

    #print('out of bounds ids:\n', permutation[out_of_bounds_mask])
    #print('out of bounds partitions:\n', np.where(out_of_bounds_mask)[0]
    #    // partition_width)
   
    # Make sure we took care of all the out of bounds elements
    out_of_bounds_mask = permutation >= locations.shape[0]
    assert(np.sum(out_of_bounds_mask) == 0)
    
    # Make sure the permutation is a bijection onto locations
    ids, id_counts = np.unique(permutation, return_counts=True)
    assert(not np.any(id_counts > 1))
    assert(permutation.shape[0] == locations.shape[0])
    
    #print(permutation.shape, locations.shape)
    #print('unique ids:\n', np.unique(permutation).shape[0],
    #    'total ids:', permutation.shape[0])

    print(permutation)
    return permutation

def main():
    path = sys.argv[1]
    people = pd.read_csv(os.path.join(path, 'people.csv'))
    locations = pd.read_csv(os.path.join(path, 'locations.csv'))
    visits = pd.read_csv(os.path.join(path, 'visits.csv'))
    num_partitions = int(sys.argv[2])

    # Partition data and combine new indices into a single list for easier
    # indexing
    #results = karmarkar_karp(list(locations['max_simultaneous_visits']),
    #        num_partitions=num_partitions, return_indices=True)
    #print('parition lengths:', [len(p) for p in results.partition])
    #permutation = np.fromiter(itertools.chain(*results.partition), int)

    locations.sort_values(by='max_simultaneous_visits', inplace=True,
            ascending=False)
    
    permutation = folding_partition(locations, num_partitions)
    print(permutation)

    # Remap expects an array of new indices in the same order as the orignal
    # dataframe, so we need to convert from the order the indices appear in
    # the output to the order they appear in the input
    num_locations = locations.shape[0]
    new_lids = np.fromiter(range(num_locations), int)
    print(locations.shape, permutation.shape, new_lids.shape)
    cauchy_perm = np.array([locations.iloc[permutation]['lid'],
        new_lids])
    print(cauchy_perm)
    cauchy_perm = cauchy_perm.transpose()
    print(cauchy_perm)
    cauchy_perm = cauchy_perm[cauchy_perm[:, 0].argsort()]
    print('(old lid, new lid) pairs:')
    print(cauchy_perm)

    #print('indices:', locations.index)
    #print('permutation:', permutation)
    #print('permuted indices:', locations.index[permutation])
    people, locations, visits = remap(people, locations, visits, people['pid'],
            cauchy_perm[:,1])
    # Loimos expects partitions to be in order, so we need to sort the data
    # to reflect the new indicies before writing everythign out
    locations.sort_values(by='lid', inplace=True)

    print(locations['lid'])

    #print(locations.columns)
    #print(locations)

    people.to_csv('people.csv', index=False)
    locations.to_csv('locations.csv', index=False)
    visits.to_csv('visits.csv', index=False)

if __name__ == '__main__':
    main()
