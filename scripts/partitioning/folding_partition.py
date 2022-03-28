#!/usr/bin/env python3

import sys
import os
import math
import itertools
import numpy as np
import pandas as pd
import argparse

# Python modules need to either be in/below this dir or in the path
currentdir = os.path.dirname(os.path.realpath(__file__))
parentdir = os.path.dirname(currentdir)
sys.path.append(parentdir)
from utils.ids import remap, init_multiprocessing

def parse_args():
    parser = argparse.ArgumentParser()

    # Positional/required args
    parser.add_argument('input_dir', metavar='I',
            help='The path to the directory containing the input files')

    # Named/optional args
    parser.add_argument('-o', '--output-dir', default='.',
            help='The path to the directory containing the input files')
    parser.add_argument('-n', '--num-partitions', type=int, default=576,
            help='The number of partitions to optimise for (should equal '+\
                 'the location chare count you wish to run with)')
    parser.add_argument('-p', '--partition-on',
            default='max_simultaneous_visits',
            help='The column in the input data which should be balanced ' +\
                 'across partitions')
    parser.add_argument('-nt', '--num-tasks', default=1, type=int,
        help='Specifies the number of processes to use (default is serial)')
    parser.add_argument('-np', '--num-merge-partitions', default=32, type=int,
        help='Specifies the number of partitions to seperate data into' + \
             'before merging')

    return parser.parse_args()

# This implemntation of the folding partition alorithm is based on the
# description of the algortihm given in:
#   Babel, L., Kellerer, H., & Kotov, V. (1998). The k-partitioning problem.
#   Mathematical Methods of Operations Research, 47(1), 59–82.
#   https://doi.org/10.1007/BF01193837
# Pass in another value of i_0 to change the starting index all the partition
# indices are relative to
def folding_partition(num_elements, num_partitions, i_0=0):
    partition_width = int(math.ceil(num_elements / num_partitions))
    num_extra = partition_width - num_elements % partition_width

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
        # Note that our indexing is different from the original paper's
        # since we (1) have an explcit loop and (2) are starting our indices
        # at 0 rather than 1
        for j in range(0, partition_width-1, 2):
            cur_partition.append(i_0 + num_partitions*(j + 2) - i - 1)
            cur_partition.append(i_0 + num_partitions*j + i)
    
        if partition_width % 2 == 1:
            cur_partition.append(i_0 + (partition_width - 1)*num_partitions + i)
        #if num_partitions - i >= num_extra:
        #    print(cur_partition.pop())

        partitions.append(cur_partition)

    return partitions, num_extra

def partitions_to_permutation(partitions, num_elements):
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
    dup_mask = id_counts > 1
    #print(ids[dup_mask])
    #print(permutation.shape[0], num_elements)
    assert(not np.any(id_counts > 1))
    assert(permutation.shape[0] == num_elements)
    
    return permutation

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


def main():
    # Parse args and read data
    args = parse_args()
    num_partitions = args.num_partitions
    partition_on = args.partition_on
    num_tasks = args.num_tasks
    num_merge_partitions = args.num_merge_partitions

    if num_tasks > 1:
        init_multiprocessing()
    
    input_dir = args.input_dir
    output_dir = args.output_dir
    
    people = pd.read_csv(os.path.join(input_dir, 'people.csv'))
    locations = pd.read_csv(os.path.join(input_dir, 'locations.csv'))
    visits = pd.read_csv(os.path.join(input_dir, 'visits.csv'))
    
    num_elements = locations.shape[0]
    
    homes_mask = locations['designation'].str.contains('home')
    num_homes = np.sum(homes_mask)
    num_non_homes = num_elements - num_homes
    print(num_homes, ' homes, ', num_non_homes, ' other locs')

    # The folding partition algroithm expects the data to be pre-sorted
    # probably don't need to sort home and non-home locatiosn seperately,
    # as there shouldn't be much overlap, but might be safer to do so anyway
    locations.sort_values(by=partition_on, ascending=False)
    locations.reset_index(inplace=True, drop=True)
    
    # Build partitions seperately for home and non-home locations, as they
    # have drastically different numbers of visits
    home_partitions, num_extra = folding_partition(num_homes, num_partitions)
    
    # For the sake of convience, just fill out the home_partitions completely
    # using a few non-home locations rather than doing any complicated indexing
    non_home_partitions, _ = folding_partition(num_non_homes - num_extra,
            num_partitions,
            i_0 = num_homes + num_extra)
    for i in range(num_partitions):
        home_partitions[i].extend(non_home_partitions[i])
    partitions = home_partitions

    permutation = partitions_to_permutation(partitions, num_elements)
    inverted_permutation = invert_permutation(locations, permutation)
    people, locations, visits = remap(people, locations, visits,
            new_location_ids=inverted_permutation,
            num_partitions=num_merge_partitions, num_tasks=num_tasks)

    # We need to save the new version of visits.csv as remap updates the lids
    # of each visit to reflect each location's new lid and position in
    # locations.csv
    people.to_csv(os.path.join(output_dir, 'people.csv'), index=False)
    locations.to_csv(os.path.join(output_dir,'locations.csv'), index=False)
    visits.to_csv(os.path.join(output_dir, 'visits.csv'), index=False)

if __name__ == '__main__':
    main()
