#!/usr/bin/env python
# coding: utf-8

# In[1]:

import os
import pandas as pd
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
import argparse

# Use a non-interactive backend so we don't have to worry about getting missing
# displays on remote systems (or setting up x-forwarding)
plt.switch_backend('Agg')

def parse_args():
    parser = argparse.ArgumentParser()
    
    # Positional/required arguments:
    parser.add_argument('input_dir', metavar='I',
        help='The path to directory containing data files for a population')

    return parser.parse_args()

def main():
    args = parse_args()
    #input_dir = '../../data/populations/coc/'
    input_dir = args.input_dir
    output_dir = input_dir
    locations = pd.read_csv(os.path.join(input_dir, 'locations.csv'))
    people = pd.read_csv(os.path.join(input_dir, 'people.csv'))
    visits = pd.read_csv(os.path.join(input_dir, 'visits.csv'))

    cols = [f'total_on_day_{d}' for d in range(7)]
    missing_mask = np.any(np.isnan(locations[cols]),axis=1)
    locations.loc[missing_mask,cols] = 0

    visit_counts = np.array(locations[cols]).flatten()
    print(np.mean(visit_counts), np.std(visit_counts))
    print(np.quantile(visit_counts, [.25,.5,.75]))

    visited_locations_mask = visit_counts != 0
    visited_location_counts = visit_counts[visited_locations_mask]
    print(visited_location_counts.shape, visit_counts.shape)

    fig, ax = plt.subplots(figsize=(10,6))
    sns.histplot(visited_location_counts, bins=50, kde=False, log_scale=(True,True))
    plt.savefig(os.path.join(output_dir, 'visits_hist.pdf'))
    #sns.kdeplot(visited_location_counts, log_scale=(True,True))

    visits_by_location = visits.groupby(by='lid')
    visit_counts_by_location = visits_by_location[['lid','start_time']].count()
    
    visit_counts_by_location['num_visit_sources'] = \
            [len(np.unique(people.iloc[grouped['pid']]['lid'])) \
            for lid, grouped in visits_by_location]
    print(visit_counts_by_location)

    fig, ax = plt.subplots(figsize=(10,6))
    sns.histplot(visit_counts_by_location, bins=50, x='num_visit_sources', kde=False, log_scale=(True,True))
    plt.savefig(os.path.join(output_dir, 'visit_sources_hist.pdf'))

    print(visit_counts_by_location['num_visit_sources'].mean(), visit_counts_by_location['num_visit_sources'].std())

if __name__ == '__main__':
    main()
