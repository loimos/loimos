#!/usr/bin/env python3
# Copyright 2021 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

"""
Given a visit schedule, this script calculates the total visits and maximum
simultaneous visits for all locations in the dataset. 

Inputs:
    path_to_visit_file: Path to visit schedule CSV. Must have a column 'lid'
        that describes the location id.
    output_file (optional): Where to output calculated csv. 
    
Outputs:
    CSV to file with three columns - lid, total_visits, max_simultaneous_visits.
    Locations without any visits will not have an entry.
"""

# Column name that describes the location id being visited.
LOCATION_ID_COLUMN_NAME = 'lid'
# Any other column in the dataset that is fully populated.
OTHER_COLUMN = 'start_time'

import pandas as pd
import heapq
import argparse
import os

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Calculates summary statistics for a given visit file.')
    
    # Positional/required arguments:
    parser.add_argument('population_dir', metavar='P',
        help='The path to directory containing data files for a population')

    # Named/optional arguments:
    parser.add_argument('-v', '--visits-file', default='visits.csv',
        help='The name of the visits file within population_dir. Should be ' +\
             'a csv file with a column called \'lid\'')
    parser.add_argument('-l', '--locations-file', default='locations.csv',
        help='The name of the locations file within population_dir. Should ' +\
             'a csv file with a column called \'lid\'')
    parser.add_argument('-o', '--output-file', default='{name}.heuristic.csv',
        help='Format of output files (\'{name}\' will be replaced by the ' +\
             'name of the corresponding input file, ignoring the extension).'+\
             'This output will be a csv containing the data from the ' +\
             'locations file, plus some extra information about visits '+\
             'computed by this script.')
    parser.add_argument('-O', '--override', action='store_true',
        help='Pass this argument to overwrite the locations file with the ' +\
             'output, rather than saving it to the specified output file')
    args = parser.parse_args()

    # Get command line arguments, and place input files in the population_dir
    path_to_visits = os.path.join(args.population_dir, args.visits_file)
    path_to_locations = os.path.join(args.population_dir, args.locations_file)

    # Output file defaults to given path with .heuristic.csv file extension.
    output_file = args.output_file.format(
        name=path_to_visits[ : path_to_visits.rfind('.')]
    )
    # If the override flag is set, save output to locations_file instead
    if args.override:
        output_file = path_to_locations

    # Load dataset.
    visits = pd.read_csv(path_to_visits)

    # Calculate total visits to a location.
    max_visits = (
        visits[[LOCATION_ID_COLUMN_NAME, OTHER_COLUMN]]
            .groupby(LOCATION_ID_COLUMN_NAME).count()
            .rename({OTHER_COLUMN: 'total_visits'}, axis=1)
    )

    # Calculate total visits for each day per location.
    daily_summaries = visits[['lid','daynum','start_time']].groupby(['lid','daynum']).count().unstack().fillna(0)
    # Rename columns.
    renaming = []
    for column in daily_summaries.columns:
        renaming.append(f'total_on_day_{column[1]}')
    daily_summaries.columns = renaming
    # Calculate some additional statistics.
    daily_summaries['average_daily_total'] = daily_summaries.mean(axis=1)
    daily_summaries['median_daily_total'] = daily_summaries.median(axis=1)
    daily_summaries['max_daily_total'] = daily_summaries.max(axis=1)
    # Merge in.
    max_visits = max_visits.merge(daily_summaries, left_index=True, right_index=True)

    # Calculate the maximum simulatenous visits.
    max_visits['max_simultaneous_visits'] = 0
    max_in_visit = 0
    end_times = []
    for lid, row in max_visits.iterrows():
        for _, row in visits[visits[LOCATION_ID_COLUMN_NAME] == lid][['start_time','end_time']].iterrows():
            # Filter out end_times not in range.
            start_time = row['start_time']
            while len(end_times) and end_times[0] <= start_time:
                heapq.heappop(end_times)

            # Append in sorted order.
            heapq.heappush(end_times, row['end_time'])
            max_in_visit = max(max_in_visit, len(end_times))
        max_visits.at[lid, 'max_simultaneous_visits'] = max_in_visit
        
        # Reset counts for next row/location
        max_in_visit = 0
        end_times.clear()

    # We need the max visit data to be a location attribute, so combine it
    # with the location data
    locations = pd.read_csv(path_to_locations)
    output_df = locations.join(max_visits, on='lid')

    # Output with index column which is the lids.
    output_df.to_csv(output_file)
