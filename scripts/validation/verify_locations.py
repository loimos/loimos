#!/usr/bin/env python3

import argparse
import os
import sys
import glob
import functools

import pandas as pd

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
    parser.add_argument(
        "baseline_file",
        metavar="B",
        help="A path to a file containing baseline location metrics to compare against",
    )

    # Named/optional arguments:
    parser.add_argument(
        "-lm",
        "--loimos-metrics",
        default="metrics.csv",
        help="The names of the files containing Loimos locationm metrics "
        + "to verify in the population dir",
    )
    parser.add_argument(
        "-l",
        "--locations-file",
        default="locations.csv",
        help="The name of the file containing location data within the "
        + "population dir",
    )

    return parser.parse_args()


COMPARISION_COLUMNS = ["max_occupancy", "conn_prob", "max_possible_edges",
    "num_expected_edges"]
BASELINE_SUFFIX = "_baseline"
def compare_column(metrics, baseline, col, merge_on="old_lid"):
    tmp = pd.merge(metrics[[merge_on, col]], baseline[[merge_on, col]],
            on=merge_on, suffixes=["", BASELINE_SUFFIX])
    mask = tmp[col] == tmp[col + BASELINE_SUFFIX]

    if mask.all():
        print(f"all {col} values match")
    else:
        matches = mask.sum()
        total = tmp.shape[0]
        print(f"{matches}/{total} ({matches/total:.2%}) "
                + f"{col} values match")


def main():
    args = parse_args()

    pop_dir = args.pop_dir

    locations = pd.read_csv(os.path.join(pop_dir, args.locations_file))
    loimos_metrics = pd.read_csv(os.path.join(pop_dir, args.loimos_metrics),
            names=["lid"] + COMPARISION_COLUMNS)
    baseline_metrics = pd.read_csv(args.baseline_file)

    tmp = locations[["lid", "old_lid"]]
    loimos_metrics = loimos_metrics.merge(tmp, on="lid")

    baseline_metrics.rename(columns={"lid": "old_lid"}, inplace=True)

    for col in COMPARISION_COLUMNS:
        compare_column(loimos_metrics, baseline_metrics, col)


if __name__ == "__main__":
    main()
