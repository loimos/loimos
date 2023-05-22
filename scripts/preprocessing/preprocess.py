#!/usr/bin/env python3

import argparse
import os
import sys
import glob
import shutil

import pandas as pd

# Python modules need to either be in/below this dir or in the path
currentdir = os.path.dirname(os.path.realpath(__file__))
parentdir = os.path.dirname(currentdir)
sys.path.append(parentdir)
from utils import ids  # noqa

DEFAULT_VALUES = {
    "work": 0,
    "school": 0,
    "other": 0,
    "college": 0,
    "religion": 0,
    "designation": "none:home",
}


def parse_args():
    parser = argparse.ArgumentParser(
        description="Samples a given set of locations (along with their "
        + "associated visits and people) from the given population"
    )

    # Positional/required arguments:
    parser.add_argument(
        "region",
        metavar="R",
        help="The name of the region descriped by the input population",
    )
    parser.add_argument(
        "in_dir",
        metavar="I",
        help="A path to a directory containing data on the population to sample from",
    )
    parser.add_argument(
        "out_dir",
        metavar="O",
        help="The name of the directory where the sampled popualtion should be saved",
    )

    # Named/optional arguments:
    parser.add_argument(
        "-pi",
        "--people-in-file",
        nargs="+",
        default=os.path.join("base_population", "{region}_person.csv"),
        help="The name of the file containing person data within the "
        + "population dir",
    )
    parser.add_argument(
        "-ri",
        "--residences-in-file",
        nargs="+",
        default=os.path.join("locations", "{region}_residence_locations.csv"),
        help="The name of the file containing home location data within the "
        + "population dir",
    )
    parser.add_argument(
        "-ai",
        "--activity-locs-in-file",
        nargs="+",
        default=os.path.join("locations", "{region}_activity_locations.csv"),
        help="The name of the file containing home location data within the "
        + "population dir",
    )
    parser.add_argument(
        "-rai",
        "--residences-assignments-in-file",
        nargs="+",
        default=os.path.join("home_location_assignment",
                             "{region}_household_residence_assignment.csv"),
        help="The name of the file asigning households to home locations "
        + "within the population dir",
    )
    parser.add_argument(
        "-aai",
        "--activity-loc-adult-assignments-in-file",
        nargs="+",
        default=os.path.join(
            "location_assignment", "weekly",
            "{region}_adult_activity_location_assignment_week.csv"),
        help="The name of the file containing adult visit data within the "
        + "population dir",
    )
    parser.add_argument(
        "-aci",
        "--activity-loc-child-assignments-in-file",
        nargs="+",
        default=os.path.join(
            "location_assignment", "weekly",
            "{region}_child_activity_location_assignment_week.csv"),
        help="The name of the file containing adult visit data within the "
        + "population dir",
    )
    parser.add_argument(
        "-po",
        "--people-out-file",
        nargs="+",
        default="people.csv",
        help="The name of the file containing person data within the "
        + "population dir",
    )
    parser.add_argument(
        "-lo",
        "--locations-out-file",
        nargs="+",
        default="locations.csv",
        help="The name of the file containing location data within the "
        + "population dir",
    )
    parser.add_argument(
        "-vo",
        "--visits-out-file",
        nargs="+",
        default="visits.csv",
        help="The name of the file containing visit data within the "
        + "population dir",
    )
    parser.add_argument(
        "-f",
        "--flat",
        action="store_true",
        help="Pass this flag if the script should expect a flat input directory"
    )

    return parser.parse_args()

def read_csv(in_dir, filename, region, should_flatten=False):
    if should_flatten:
        filename = os.path.basename(filename)

    path = os.path.join(in_dir, filename.format(region=region))
    print(f"Reading {path}")
    return pd.read_csv(path)

def write_csv(out_dir, filename, df):
    path = os.path.join(out_dir, filename)
    print(f"Saving results to {path}")
    df.to_csv(path, index=False)

def merge_locations(args):
    in_dir = args.in_dir

    activity_locs = read_csv(in_dir, args.activity_locs_in_file, args.region,
                             should_flatten=args.flat)
    home_locs = read_csv(in_dir, args.residences_in_file, args.region,
                         should_flatten=args.flat)

    activity_locs.rename(columns={"alid": "lid"}, inplace=True)
    home_locs.rename(columns={"rlid": "lid"}, inplace=True)

    activity_offset = activity_locs["lid"].min()
    activity_locs["lid"] -= activity_offset

    home_offset = home_locs["lid"].min() - activity_locs["lid"].max() + 1
    home_locs["lid"] -= activity_offset

    # Make sure all columns are shared.
    for col, def_val in DEFAULT_VALUES.items():
        home_locs[col] = def_val
    home_locs["home"] = 1
    activity_locs["home"] = 0

    locations = pd.concat([activity_locs, home_locs], ignore_index=True)
    write_csv(args.out_dir, args.locations_out_file, locations)

    return activity_offset, home_offset

def fix_people(args):
    people = read_csv(args.in_dir, args.people_in_file, args.region,
                      should_flatten=args.flat)
    people_offset = people["pid"].min()
    people["pid"] -= people_offset
    write_csv(args.out_dir, args.people_out_file, people)

    return people_offset

def merge_visits(args, activity_offset, home_offset, people_offset):
    in_dir = args.in_dir

    adult_activity_visits = read_csv(
            in_dir,
            args.activity_loc_adult_assignments_in_file,
            args.region,
            should_flatten=args.flat)
    child_activity_visits = read_csv(
            in_dir,
            args.activity_loc_child_assignments_in_file,
            args.region,
            should_flatten=args.flat)
    home_visits = read_csv(in_dir, args.residences_assignments_in_file,
                           args.region,
                           should_flatten=args.flat)

    adult_activity_visits["lid"] -= activity_offset
    child_activity_visits["lid"] -= activity_offset

    visits = pd.concat(
            [adult_activity_visits, child_activity_visits],
            ignore_index=True)
    visits["pid"] -= people_offset

    write_csv(args.out_dir, args.visits_out_file, visits)

def main():
    args = parse_args()

    activity_offset, home_offset = merge_locations(args)
    people_offset = fix_people(args)
    merge_visits(args, activity_offset, home_offset, people_offset)

if __name__ == "__main__":
    main()
