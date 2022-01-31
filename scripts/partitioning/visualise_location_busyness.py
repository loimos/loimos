#!/usr/bin/env python3
# coding: utf-8

import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
import numpy as np
import os
import sys
import math

# Use a non-interactive backend so we don't have to worry about getting missing
# displays on remote systems (or setting up x-forwarding)
plt.switch_backend('Agg')

#input_dir = os.path.join('..', '..', 'data', 'populations', 'coc')
#n_partitions = 40
input_dir = sys.argv[1]
n_partitions = int(sys.argv[2])

locations_path = os.path.join(input_dir, 'locations.csv')
locations = pd.read_csv(locations_path)

# Plot distribution
sns.jointplot(data=locations, x='lid', y='max_simultaneous_visits', kind='kde')

# Partition data
partition_width = int(math.ceil(locations.shape[0] / n_partitions))
locations['lid_partition'] = pd.cut(locations['lid'], n_partitions).cat.codes
partitions = pd.DataFrame(locations.groupby(by='lid_partition').sum())

# Show how heavy each partition is
fig, ax = plt.subplots(figsize=(10,6))
sns.barplot(data=locations, x='lid_partition', y='max_simultaneous_visits',
        ax=ax, estimator=np.sum)
plt.savefig('partition_weights.pdf')

# Show the distribution in each partition (also shows outliers)
fig, ax = plt.subplots(figsize=(10,6))
sns.boxplot(data=locations, x='lid_partition', y='max_simultaneous_visits',
        ax=ax)
plt.savefig('partition_distribution.pdf')

#plt.fill_between(partitions.index, partitions['max_simultaneous_visits'])
