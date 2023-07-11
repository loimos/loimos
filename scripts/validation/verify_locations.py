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
def compare_column(metrics, baseline, col, merge_on="old_lid", epsilon=1e-6):
    df = pd.merge(metrics, baseline,
            on=merge_on, suffixes=["", BASELINE_SUFFIX])

    mask = df[col] == df[col + BASELINE_SUFFIX]
    err = None
    std_err = None
    if not mask.all():
        err = (df[col] - df[col + BASELINE_SUFFIX]).abs()
        mask = err < epsilon

    matches = mask.sum()
    total = df.shape[0]
    if mask.all():
        print(f"all {col} values match")
    else:
        print(f"{matches}/{total} ({matches/total:.2%}) of {col} values match")

        out = "  "
        std_err = (err ** 2).mean()
        if isinstance(std_err, float):
            out += f"std err: {std_err:0.3}, "
        else:
            out += f"std err: {std_err}, "

        total_err = err.sum()
        if isinstance(total_err, float):
            out += f"total err: {total_err:0.3}, "
        else:
            out += f"total err: {total_err}, "

        rel_err = total_err / df[col + BASELINE_SUFFIX].sum()
        if isinstance(rel_err, float):
            out += f"rel err: {rel_err:0.3}"
        else:
            out += f"rel err: {rel_err}"
        print(out)

        if col == "max_possible_edges":
            df[~mask].to_csv("diff_locs.csv", index=False)


def main():
    args = parse_args()

    pop_dir = args.pop_dir
    print(f"Verifying location stats in {pop_dir}")

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
