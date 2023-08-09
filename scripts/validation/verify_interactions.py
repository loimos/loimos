#!/usr/bin/env python3

import argparse
import os
import sys
import glob
import functools

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
        "--interactions-files",
        default=["interactions_chare_*.csv"],
        nargs="+",
        help="The names of the files containing Loimos output (with "
        "interactions) to verify in the population dir",
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


INTERACTION_COLUMNS = [
    "lid",
    "dep_pid",
    "dep_start",
    "dep_end",
    "arr_pid",
    "arr_start",
    "arr_end",
]


def read_interactions(pop_dir, in_files):
    interactions_in_files = functools.reduce(
        lambda file_list, in_file: file_list
        + glob.glob(os.path.join(pop_dir, in_file)),
        in_files,
        [],
    )

    interactions = None
    total_interactions = 0
    for f in interactions_in_files:
        if os.stat(f).st_size > 0:
            df = pd.read_csv(f, names=INTERACTION_COLUMNS)
            total_interactions += df.shape[0]
            if interactions is None:
                interactions = df
            else:
                interactions = pd.concat([interactions, df])

    return interactions


def check_mask(interactions, mask, items="interactions", checked_for=""):
    if not mask.all():
        mask = ~mask
        print(
            f"{mask.sum()}/{interactions.shape[0]} {items} " + f"weren't {checked_for}:"
        )
        print(interactions[mask])
    else:
        print(f"All {items} {checked_for} " + f"({mask.sum()}/{interactions.shape[0]})")


def check_for_duplicates(interactions):
    duplicate_mask = ~interactions.duplicated()
    check_mask(interactions, duplicate_mask, checked_for="unique (not flipped)")

    swapped = interactions.copy()
    swapped[
        ["arr_pid", "arr_start", "arr_end", "dep_pid", "dep_start", "dep_end"]
    ] = swapped[["dep_pid", "dep_start", "dep_end", "arr_pid", "arr_start", "arr_end"]]
    flipped_duplicates = pd.merge(interactions, swapped)

    if flipped_duplicates.shape[0] > 0:
        print(
            f"{flipped_duplicates.shape[0]}/{interactions.shape[0]} "
            + "weren't unique (flipped):"
        )
        print(flipped_duplicates)
    else:
        print(
            "All interactions unique (flipped) "
            + f"({interactions.shape[0]}/{interactions.shape[0]})"
        )

    return duplicate_mask


def check_overlaps(interactions):
    overlap_mask = (
        (interactions["dep_start"] <= interactions["arr_start"])
        & (interactions["dep_end"] > interactions["arr_start"])
    ) | (
        (interactions["arr_start"] <= interactions["dep_start"])
        & (interactions["arr_end"] > interactions["dep_start"])
    )

    check_mask(interactions, overlap_mask, checked_for="overlapped")

    return overlap_mask


def check_against_visits(interactions, visits):
    arrivals = pd.merge(
        interactions,
        visits,
        how="left",
        left_on=["lid", "arr_pid", "arr_start", "arr_end"],
        right_on=["lid", "pid", "start_time", "end_time"],
    )
    departures = pd.merge(
        interactions,
        visits,
        how="left",
        left_on=["lid", "dep_pid", "dep_start", "dep_end"],
        right_on=["lid", "pid", "start_time", "end_time"],
    )

    arrival_matched_mask = ~arrivals["duration"].isna()
    departure_matched_mask = ~departures["duration"].isna()

    check_mask(
        interactions,
        arrival_matched_mask,
        items="arrivals",
        checked_for="found in visits file",
    )
    check_mask(
        interactions,
        departure_matched_mask,
        items="departures",
        checked_for="found in visits file",
    )

    return arrival_matched_mask, departure_matched_mask


def main():
    args = parse_args()

    pop_dir = args.pop_dir

    visits_in_file = os.path.join(pop_dir, args.visits_file)
    visits = pd.read_csv(visits_in_file)
    # End time is not a required variable for visits
    visits["end_time"] = visits["start_time"] + visits["duration"]

    interactions = read_interactions(pop_dir, args.interactions_files)

    check_for_duplicates(interactions)
    check_overlaps(interactions)
    check_against_visits(interactions, visits)


if __name__ == "__main__":
    main()
