#!/usr/bin/env python3

# Copyright 2021 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

"""
Remaps city-of-charlottseville dataset to use a dense id format and combines
residence and activity locations into a single file.
"""
import argparse
import os
import sys
import pandas as pd

_DEFAULT_VALUES = {
    "work": 0,
    "school": 0,
    "other": 0,
    "college": 0,
    "religion": 0,
    "designation": "none:home"
}

def parse_args():
    parser = argparse.ArgumentParser(
        description='Prepares population dataset for use by Loimos')

    # Positional/required arguments:
    parser.add_argument('population_dir', metavar='P',
        help='The path to directory containing data files for a population; '+\
             'the directory name should be the same as the prefix used ' +\
             'in the names of the data files')

    # Named/optional arguments:
    parser.add_argument('-o', '--override', action='store_true',
        help='Pass this flag in order to override the original files with ' +\
             'the processed versions')
    parser.add_argument('-p', '--people-file', default='{prefix}_person.csv')
    parser.add_argument('-a', '--activity-locations-file',
        default='{prefix}_activity_locations.csv')
    parser.add_argument('-r', '--residence-locations-file',
        default='{prefix}_residence_locations.csv')
    parser.add_argument('-v', '--visits-file',
        default='{prefix}_visits.csv')

    return parser.parse_args()

def combine_residences_and_activities(activity_locations, residence_locations):
    # Make sure all columns are shared.
    for col, def_val in _DEFAULT_VALUES.items():
        residence_locations[col] = def_val
    residence_locations["home"] = 1
    activity_locations["home"] = 0
    
    return activity_locations.append(residence_locations).reset_index().drop("index", axis=1)

def id_remapper(people, locations, visits):
    groups = [
        (people, 'pid', {'pid': 'visits'}),
        (locations, 'lid', {'lid': 'visits', 'hid': 'people'})
    ]
    data = {'people': people, 'locations': locations, 'visits': visits}

    # Remap location ids.
    for to_remap, key, external_references in groups:
        # Remaps the dataframes existing index to a new dense index.
        to_remap['new_id'] = to_remap.index
        remapper = to_remap[[key, 'new_id']].copy()
        to_remap[key] = to_remap['new_id']
        to_remap.drop(["new_id"], axis=1, inplace=True)

        # Replace all references of the old keys with the new ones
        for ref_key, ref_name in external_references.items():
            ref = data[ref_name]
            ref = ref.merge(remapper, how='left', left_on=ref_key, right_on=key)
            ref[ref_key]= ref['new_id']
            ref.drop(["new_id"], axis = 1, inplace=True)
            data[ref_name] = ref
            
    return data['people'], data['locations'], data['visits']

if __name__ == "__main__":
    args = parse_args()

    override = args.override
    population_dir = args.population_dir
    
    # We need to normalise the given path so that relative paths don't break
    # the script and basename works correctly
    population_dir = os.path.abspath(population_dir)
    prefix = os.path.basename(population_dir)

    # Get user confirmation.
    #if not override:
    #    print(f"This script will overwrite the existing files at {population_dir}")
    #    val = input("Enter 'Yes' to confirm:")
    #    if val != "Yes":
    #        print("Aborting...") 
    #        exit(0)

    people_file = os.path.join(
            population_dir,
            args.people_file.format(prefix=prefix))
    activity_locations_file = os.path.join(
            population_dir,
            args.activity_locations_file.format(prefix=prefix))
    residence_locations_file = os.path.join(
            population_dir,
            args.residence_locations_file.format(prefix=prefix))
    visits_file = os.path.join(
            population_dir,
            args.visits_file.format(prefix=prefix))

    print(people_file)
    print(activity_locations_file)
    print(residence_locations_file)
    print(visits_file)

    # Read in all the datasetes.
    people = pd.read_csv(people_file)
    activity_locations = pd.read_csv(activity_locations_file)
    residences = pd.read_csv(residence_locations_file)
    visits = pd.read_csv(visits_file)

    print('Before reindexing:')
    print(people)

    # Combines activity and residence locations.
    combined = combine_residences_and_activities(activity_locations, residences)

    # Remap all ids
    people, combined, visits = id_remapper(people, combined, visits)
    print('After reindexing:')
    print(people)

    # Fix types
    combined.fillna(0, inplace=True)
    combined = combined.astype({'shopping': int})
    print(combined.dtypes)

    # Make sure all the visits are in the right order
    visits.sort_values(by=['pid', 'start_time'], inplace=True)

    if override:
        # Cleanup
        os.remove(people_file)
        os.remove(activity_locations_file)
        os.remove(residence_locations_file)
        os.remove(visits_file)

    people_file = os.path.join(population_dir, 'people.csv')
    locations_file = os.path.join(population_dir, 'locations.csv')
    visits_file = os.path.join(population_dir, 'visits.csv')

    # Output
    people.to_csv(people_file, index=False)
    combined.to_csv(locations_file, index=False)
    visits.to_csv(visits_file, index=False)     
