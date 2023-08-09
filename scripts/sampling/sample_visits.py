#!/usr/bin/env python3

import argparse
import os
import sys

import pandas as pd

# Python modules need to either be in/below this dir or in the path
currentdir = os.path.dirname(os.path.realpath(__file__))
parentdir = os.path.dirname(currentdir)
sys.path.append(parentdir)
from utils import ids  # noqa


def parse_args():
    parser = argparse.ArgumentParser(
        description="Samples visits falling into a certain range"
    )

    # Positional/required arguments:
    parser.add_argument(
        "pop_dir",
        metavar="P",
        help="A path to a directory containing data on the population to sample from",
    )

    # Named/optional arguments:
    parser.add_argument(
        "-vi",
        "--visits-in-file",
        default="visits.csv",
        help="The name of the file containing visit data within the "
        + "population dir",
    )
    parser.add_argument(
        "-v",
        "--visits-out-file",
        default="visits_wed.csv",
        help="The name of the file to save sampled visit data to within the "
        + "population dir",
    )
    parser.add_argument(
        "-t",
        "--times",
        type=int,
        nargs=2,
        default=[2 * 86400, 3 * 86400],
        help="The range of start times to sample",
    )

    return parser.parse_args()


def main():
    args = parse_args()

    pop_dir = args.pop_dir

    visits_in_file = os.path.join(pop_dir, args.visits_in_file)
    visits_out_file = os.path.join(pop_dir, args.visits_out_file)

    print(f"Loading visits from {visits_in_file}")
    visits = pd.read_csv(visits_in_file)

    start, end = args.times
    mask = (start <= visits["start_time"]) & (visits["start_time"] < end)
    print(f"Saving sampled visits to {visits_out_file}")
    visits[mask].to_csv(visits_out_file)


if __name__ == "__main__":
    main()
