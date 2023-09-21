#!/usr/bin/env python3

import argparse
import os
import shutil

import pandas as pd
from preprocess import read_csv, write_csv, make_contiguous, update_ids
from create_textproto import (
    create_textproto,
    PEOPLE_TYPES,
    LOCATIONS_TYPES,
    VISITS_TYPES,
)


def parse_args():
    parser = argparse.ArgumentParser(
        description="Partitions the specified data using the specified scheme"
    )

    # Positional/required arguments:
    parser.add_argument(
        "in_dir",
        metavar="I",
        help="A path to a directory containing data on the population to partition",
    )
    parser.add_argument(
        "num_partitions",
        metavar="P",
        help="The number of partitions to split the data into",
        type=int,
    )

    # Named/optional arguments:
    parser.add_argument(
        "-o",
        "--out-dir",
        default=None,
        help="The name of the directory where the partitioned population data",
    )
    parser.add_argument(
        "-p",
        "--people-file",
        default="people.csv",
        help="The name of the file containing person data within the "
        + "population dir",
    )
    parser.add_argument(
        "-l",
        "--locations-file",
        default="locations.csv",
        help="The name of the file containing location data within the "
        + "population dir",
    )
    parser.add_argument(
        "-v",
        "--visits-file",
        default="visits.csv",
        help="The name of the file containing visit data within the "
        + "population dir",
    )
    parser.add_argument(
        "-ll",
        "--location-load-col",
        default="total_visits",
        help="The column to use to represent location load",
    )

    args = parser.parse_args()

    # Assume out and in dirs are the same by default for convience
    if args.out_dir is None:
        args.out_dir = args.in_dir

    return args


def get_partition_load(df, load_col="total_visits", partition_col="partition"):
    partition_summary = df[[partition_col, load_col]].groupby(partition_col)
    cols = [
        partition_summary.sum().rename(columns={load_col: "total_load"}),
        partition_summary.count().rename(columns={load_col: "size"}),
        partition_summary.min().rename(columns={load_col: "min_load"}),
        partition_summary.max().rename(columns={load_col: "max_load"}),
    ]
    return pd.concat(cols, axis="columns")


def refine_partition(df, load_col="total_visits"):
    partition_loads = get_partition_load(df, load_col=load_col)
    partition_loads


def get_offsets(df, partition_col="partition"):
    boundary_mask = df[partition_col].diff() != 0
    return list(df[boundary_mask].index)


# Assumes df has already been sorted, and returns list of cuts to make to form
# partitions (i.e. the partition offsets)
def linear_cut_partition(
    df, load_col="total_visits", num_partitions=16, partition_col="partition"
):
    mean_load = df[load_col].mean()
    mean_load_per_partition = mean_load * df.shape[0] / num_partitions

    # Add df to each partition until it exceeds the mean load
    df["partition"] = -1
    partition_id = 0
    partition_load = 0
    for index, row in df.iterrows():
        partition_load += row[load_col]
        df.loc[index, "partition"] = partition_id
        if partition_load > mean_load_per_partition:
            partition_id += 1
            partition_load = 0

    print(mean_load_per_partition)
    partition_load = get_partition_load(
        df, load_col=load_col, partition_col=partition_col
    )
    print(partition_load)

    return get_offsets(df, partition_col=partition_col)


LOCATION_SORT_BY = ["admin1", "admin2", "admin3", "admin4"]


def main():
    args = parse_args()

    if not os.path.isdir(args.out_dir):
        os.makedirs(args.out_dir)

    locations = read_csv(args.in_dir, args.locations_file)
    visits = read_csv(args.in_dir, args.visits_file)

    # Reindexing doesn't depend on the partition
    locations.sort_values(LOCATION_SORT_BY, inplace=True)
    lid_update = make_contiguous(locations, name="locations", reset_index=True)
    visits = update_ids(visits, lid_update, name="visits")

    offsets = linear_cut_partition(
        locations, load_col=args.location_load_col, num_partitions=args.num_partitions
    )

    shutil.copy(os.path.join(args.in_dir, args.people_file), args.out_dir)
    write_csv(args.out_dir, args.locations_file, locations)
    write_csv(args.out_dir, args.visits_file, visits)

    create_textproto(args.out_dir, args.people_file, PEOPLE_TYPES)
    create_textproto(
        args.out_dir, args.locations_file, LOCATIONS_TYPES, partition_offsets=offsets
    )
    create_textproto(args.out_dir, args.visits_file, VISITS_TYPES)


if __name__ == "__main__":
    main()
