#!/usr/bin/env python3
# coding: utf-8

import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
import numpy as np
import os
import sys
import math
import argparse

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

    return parser.parse_args()

# Use a non-interactive backend so we don't have to worry about getting missing
# displays on remote systems (or setting up x-forwarding)
plt.switch_backend('Agg')

args = parse_args()
input_dir = args.input_dir
output_dir = args.output_dir
num_partitions = args.num_partitions
partition_on = args.partition_on

locations_path = os.path.join(input_dir, 'locations.csv')
locations = pd.read_csv(locations_path)

# Plot distribution
sns.jointplot(data=locations, x='lid', y=partition_on, kind='kde')

# Partition data
partition_width = int(math.ceil(locations.shape[0] / num_partitions))
locations['lid_partition'] = pd.cut(locations['lid'], num_partitions).cat.codes
partitions = pd.DataFrame(locations.groupby(by='lid_partition').sum())

# Show how heavy each partition is
fig, ax = plt.subplots(figsize=(10,6))
sns.barplot(data=locations, x='lid_partition', y=partition_on, ax=ax, ci=None)
plt.savefig(os.path.join(output_dir,'partition_weights.pdf'))

# Show the distribution in each partition (also shows outliers)
fig, ax = plt.subplots(figsize=(10,6))
sns.boxplot(data=locations, x='lid_partition', y=partition_on, ax=ax)
plt.savefig(os.path.join(output_dir,'partition_distribution.pdf'))

#plt.fill_between(partitions.index, partitions['max_simultaneous_visits'])
