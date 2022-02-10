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

# This implemntation of the folding partition alorithm is based on the
# description of the algortihm given in:
#   Babel, L., Kellerer, H., & Kotov, V. (1998). The k-partitioning problem.
#   Mathematical Methods of Operations Research, 47(1), 59–82.
#   https://doi.org/10.1007/BF01193837
def folding_partition(num_elements, num_partitions):
    partition_width = int(math.ceil(num_elements / num_partitions))
    
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

    # Using a single array simiplifies things from here on out
    permutation = np.fromiter(itertools.chain(*partitions), int)

    # The last partition is the only one which can be less than partition_width
    # so fill in the earlier out of bounds elements with elements from it
    out_of_bounds_mask = permutation >= num_elements
    num_out_of_bounds = np.sum(out_of_bounds_mask)
    if num_out_of_bounds > 0:
        permutation[out_of_bounds_mask] = permutation[-num_out_of_bounds:]
        permutation = permutation[:-num_out_of_bounds]
   
    # Make sure we took care of all the out of bounds elements
    out_of_bounds_mask = permutation >= num_elements
    assert(np.sum(out_of_bounds_mask) == 0)
    
    # Make sure the permutation is a bijection onto elements
    ids, id_counts = np.unique(permutation, return_counts=True)
    assert(not np.any(id_counts > 1))
    assert(permutation.shape[0] == num_elements)
    
    return permutation

# Helper function which reports the aggregate values in each partition.
# Implmented here for debuging purposes
def get_partition_mean(df, num_partitions,
        partition_by='lid',
        partition_col='partition',
        agg_col='max_simultaneous_visits'):
    # Partition data
    partition_width = int(math.ceil(df.shape[0] / num_partitions))
    df[partition_col] = \
            pd.cut(df[partition_by], num_partitions).cat.codes
    partitions = pd.DataFrame(df.groupby(by=partition_col).mean())
    
    return df[[partition_col, partition_by, agg_col]]

# Remap expects an array of new indices in the same order as the orignal
# dataframe, whereas the folding partition algorithm gives us the order
# the idnices appear in the final partitioned version. This helper function
# lets us convert between these two orderings
def invert_permutation(df, permutation, id_col='lid'):
    num_elements = df.shape[0]
    old_ids = df.iloc[permutation][id_col]
    new_ids = np.fromiter(range(num_elements), int)

    # The argsort trick requires a column vector, but it's much easier
    # to create the array row-wise, hence the transpose
    cauchy_perm = np.array([old_ids, new_ids])
    cauchy_perm = cauchy_perm.transpose()
    cauchy_perm = cauchy_perm[cauchy_perm[:, 0].argsort()]

    return cauchy_perm[:,1]

def main():
    # Parse args and read data
    path = sys.argv[1]
    people = pd.read_csv(os.path.join(path, 'people.csv'))
    locations = pd.read_csv(os.path.join(path, 'locations.csv'))
    visits = pd.read_csv(os.path.join(path, 'visits.csv'))
    num_partitions = int(sys.argv[2])

    # The folding partition algroithm expects the data to be pre-sorted
    locations.sort_values(by='max_simultaneous_visits', inplace=True,
            ascending=False)
    locations.reset_index(inplace=True, drop=True)
    
    permutation = folding_partition(locations.shape[0], num_partitions)
    inverted_permutation = invert_permutation(locations, permutation)
    people, locations, visits = remap(people, locations, visits,
            new_location_ids=inverted_permutation)
    
    # We need to save the new version of visits.csv as remap updates the lids
    # of each visit to reflect each location's new lid and position in
    # locations.csv
    people.to_csv('people.csv', index=False)
    locations.to_csv('locations.csv', index=False)
    visits.to_csv('visits.csv', index=False)

if __name__ == '__main__':
    main()
