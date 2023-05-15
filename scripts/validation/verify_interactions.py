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


def parse_args():
    parser = argparse.ArgumentParser(
        description="Checks Loimos interaction output against "
        + "an input population deck"
    )

    # Positional/required arguments:
    parser.add_argument(
        "pop_dir",
        metavar="P",
        help="A path to a directory containing data on the population to sample from",
    )

    # Named/optional arguments:
    parser.add_argument(
        "-ip",
        "--interactions-file",
        default="interactions.csv",
        help="The name of the file containing Loimos output (with interactions) "
        + "to verify in the population dir",
    )
    parser.add_argument(
        "-pp",
        "--people-file",
        default="people.csv",
        help="The name of the file containing person data within the "
        + "population dir",
    )
    parser.add_argument(
        "-lp",
        "--locations-file",
        default="locations.csv",
        help="The name of the file containing location data within the "
        + "population dir",
    )
    parser.add_argument(
        "-vp",
        "--visits-file",
        nargs="+",
        default="visits.csv",
        help="The name of the file containing visit data within the "
        + "population dir",
    )

    return parser.parse_args()

def check_mask(interactions, mask, items="interactions",
               checked_for=""):
    if not mask.all():
        mask = ~mask
        print(f"{mask.sum()}/{interactions.shape[0]} {items} "
              + f"weren't {checked_for}:")
        print(interactions[mask])
    else:
        print(f"All {items} {checked_for} "
              + f"({mask.sum()}/{interactions.shape[0]})")

def check_overlaps(interactions):
    overlap_mask = (
        (interactions['dep_start'] <= interactions['arr_start'])
        & (interactions['dep_end'] > interactions['arr_start'])
    ) | (
        (interactions['arr_start'] <= interactions['dep_start'])
        & (interactions['arr_end'] > interactions['dep_start'])
    )

    check_mask(interactions, overlap_mask, checked_for="overlapped")

    return overlap_mask

def check_against_visits(interactions, visits):
    arrivals = pd.merge(
        interactions, visits, how="left",
        left_on=["lid", "arr_pid", "arr_start", "arr_end"],
        right_on=["lid", "pid", "start_time", "end_time"])
    departures = pd.merge(
        interactions, visits, how="left",
        left_on=["lid", "dep_pid", "dep_start", "dep_end"],
        right_on=["lid", "pid", "start_time", "end_time"])
    
    arrival_matched_mask = ~arrivals["duration"].isna()
    departure_matched_mask = ~departures["duration"].isna()
    
    check_mask(interactions, arrival_matched_mask, items="arrivals",
               checked_for="found in visits file")
    check_mask(interactions, departure_matched_mask, items="departures",
               checked_for="found in visits file")

    return arrival_matched_mask, departure_matched_mask

def main():
    args = parse_args()

    pop_dir = args.pop_dir

    visits_in_file = os.path.join(pop_dir, args.visits_file)
    interactions_in_file = os.path.join(pop_dir, args.interactions_file)

    visits = pd.read_csv(visits_in_file)
    interactions = pd.read_csv(interactions_in_file)

    check_overlaps(interactions)
    check_against_visits(interactions, visits)

if __name__ == "__main__":
    main()
