#!/usr/bin/env python3

import argparse
import os
import shutil

import pandas as pd
import numpy as np
from preprocess import read_csv, write_csv, make_contiguous, update_ids, VISIT_SORT_COLS
from create_textproto import (
    create_textproto,
    create_offsets_textproto,
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
    parser.add_argument(
        "-nl",
        "--num_locations",
        default=None,
        type=int,
        help="The number of rows to read from the locations file. Defaults to "
        + "reading all rows if not passed. Can be helpful for debugging.",
    )
    parser.add_argument(
        "-nv",
        "--num_visits",
        default=None,
        type=int,
        help="The number of rows to read from the visits file. Defaults to "
        + "reading all rows if not passed. Can be helpful for debugging.",
    )
    parser.add_argument(
        "-nt",
        "--num_tasks",
        default=1,
        type=int,
        help="The number of tasks with which to run any merges",
    )
    parser.add_argument(
        "-ppt",
        "--num_partitions_per_task",
        default=None,
        type=int,
        help="The number of partitions for merge dataframes per task",
    )
    PARTITION_OPTIONS = ["locations", "people"]
    parser.add_argument(
        "--to-partition",
        "-t",
        default=PARTITION_OPTIONS,
        choices=PARTITION_OPTIONS,
        nargs="+",
        help="Specifies which files to partition (defaults to both people"
        + " and locations)",
    )

    # Flags
    parser.add_argument(
        "-val",
        "--validate",
        action="store_true",
        help="Pass this flag if the script should validate its results",
    )
    parser.add_argument(
        "-oo",
        "--offsets-only",
        action="store_true",
        help="Pass this flag if the script should only update the partition "
        + "offsets and not the population files",
    )
    parser.add_argument(
        "-d",
        "--set-default",
        action="store_true",
        help="Pass this flag if the script should set this partitioning as the "
        + "default for this dataset",
    )

    args = parser.parse_args()

    # Don't use multiple partitions (by default) unless we're also using
    # multiple tasks
    if args.num_partitions_per_task is None:
        if args.num_tasks == 1:
            args.num_partitions_per_task = 1
        else:
            args.num_partitions_per_task = 4

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


UNASSIGNED_PARTITION = -1


# Assumes df has already been sorted, and returns list of cuts to make to form
# partitions (i.e. the partition offsets)
def linear_cut_partition(
    df, load_col="total_visits", num_partitions=16, partition_col="partition"
):
    mean_load_per_partition = df[load_col].sum() / num_partitions
    print(
        f"Calculating {num_partitions} partitions with a mean load of "
        + f"{mean_load_per_partition}",
        flush=True,
    )

    # Locations larger than the expected load per partitions should get their
    # own partition to avoid having empty partitions
    next_partition = 0
    df[partition_col] = UNASSIGNED_PARTITION
    unassigned_mask = df[partition_col] == UNASSIGNED_PARTITION
    large_loc_mask = df[load_col] > mean_load_per_partition
    num_large_locs = large_loc_mask.sum()
    while num_large_locs > 0:
        print(
            f"Found {num_large_locs} locs with load greater than "
            + f"{mean_load_per_partition}"
        )
        partition_ids = np.arange(
            next_partition, next_partition + num_large_locs, 1
        ).astype(int)
        df.loc[large_loc_mask, partition_col] = partition_ids

        # Assiging large locations to their own partitions reduces
        # the mean load per partition, so we may need to do this again
        next_partition += num_large_locs
        unassigned_mask = df[partition_col] == UNASSIGNED_PARTITION
        mean_load_per_partition = df.loc[unassigned_mask, load_col].sum() / (
            num_partitions - next_partition
        )
        large_loc_mask = (df[load_col] > mean_load_per_partition) & unassigned_mask
        num_large_locs = large_loc_mask.sum()

    print(
        f"Calculating remaining {num_partitions - next_partition} "
        + f"partitions with a mean load of {mean_load_per_partition}",
        flush=True,
    )

    # We need partitions to be contiguous in order for them to be completely
    # described by their starting offsets
    assigned_mask = ~unassigned_mask
    single_location_loads = df.loc[assigned_mask, load_col]
    df.loc[assigned_mask, load_col] = mean_load_per_partition

    df[partition_col] = (
        np.ceil(df[load_col].cumsum() / mean_load_per_partition) - 1
    ).astype(int)
    df.loc[df[partition_col] < 0, partition_col] = 0

    df.at[df.shape[0] - 1, partition_col] = num_partitions - 1

    df.loc[assigned_mask, load_col] = single_location_loads

    partition_load = get_partition_load(
        df, load_col=load_col, partition_col=partition_col
    )
    print(partition_load, flush=True)

    offsets = get_offsets(df, partition_col=partition_col)
    df.drop(columns=partition_col, inplace=True)
    return offsets


LOCATION_SORT_BY = ["admin1", "admin2", "admin3", "admin4"]


def partition_locations(args):
    locations = read_csv(args.in_dir, args.locations_file, nrows=args.num_locations)
    print("locations loaded:", flush=True)
    print(locations, flush=True)

    # Reindexing doesn't depend on the partition
    if not args.offsets_only:
        locations.sort_values(LOCATION_SORT_BY, inplace=True)
        print("locations sorted:", flush=True)
        print(locations, flush=True)

    lid_update = make_contiguous(
        locations, name="locations", reset_index=True, validate=args.validate
    )
    print("lids reset:", flush=True)
    print(lid_update, flush=True)

    offsets = linear_cut_partition(
        locations, load_col=args.location_load_col, num_partitions=args.num_partitions
    )
    print(f"partition completed with {len(offsets)} offsets", flush=True)

    if not args.offsets_only:
        write_csv(args.out_dir, args.locations_file, locations)
    return offsets, lid_update


PEOPLE_SORT_BY = "home_lid"


def partition_people(args):
    people = read_csv(args.in_dir, args.people_file)
    print("people loaded:", flush=True)
    print(people, flush=True)

    if "home_lid" not in people.columns or "total_visits" not in people.columns:
        visits = read_csv(args.in_dir, args.visits_file)
        visits_by_person = visits[["lid", "pid"]].groupby("pid")
        people["home_lid"] = visits_by_person.head(1)["lid"]
        people["total_visits"] = visits_by_person.count()

        people["home_lid"].fillna(-1, inplace=True)
        people["total_visits"].fillna(0, inplace=True)

    # Reindexing doesn't depend on the partition
    if not args.offsets_only:
        people.sort_values(PEOPLE_SORT_BY, inplace=True)
        print("people sorted:", flush=True)
        print(people, flush=True)

    pid_update = make_contiguous(
        people, name="people", reset_index=True, validate=args.validate, id_col="pid"
    )
    print("pids reset:", flush=True)
    print(pid_update, flush=True)

    offsets = linear_cut_partition(
        people, load_col=args.location_load_col, num_partitions=args.num_partitions
    )
    print(f"partition completed with {len(offsets)} offsets", flush=True)

    if not args.offsets_only:
        write_csv(args.out_dir, args.people_file, people)
    return offsets, pid_update


def update_chunk(args, visits_chunk, id_update, id_col="lid"):
    if args.num_locations:
        n = visits_chunk.shape[0]
        visits_chunk = visits_chunk[visits_chunk[id_col].isin(id_update[id_col])]
        print(f"  {visits_chunk.shape[0]}/{n} visits kept", flush=True)

    visits_chunk = update_ids(
        visits_chunk,
        id_update,
        id_col=id_col,
        name="visits",
        num_tasks=args.num_tasks,
        num_partitions=args.num_partitions_per_task * args.num_tasks,
        validate=args.validate,
    )
    print("  Lids updated in visits", flush=True)
    return visits_chunk


def update_visits(args, id_update, id_col="lid", sort_values=True):
    if isinstance(id_update, int) and 0 == id_update:
        return
    if args.num_visits is not None:
        visit_chunks = read_csv(
            args.in_dir, args.visits_file, iterator=True, chunksize=args.num_visits
        )
        chunk_dfs = []
        for i, chunk in enumerate(visit_chunks):
            print(f"Updating {id_col}s in chunk {i}")
            chunk = update_chunk(args, chunk, id_update, id_col=id_col)
            if sort_values:
                chunk_dfs.append(chunk)
            elif i == 0:
                write_csv(args.out_dir, args.visits_file, chunk)
            else:
                write_csv(args.out_dir, args.visits_file, chunk, mode="a", header=False)
        if sort_values:
            visits = pd.concat(chunk_dfs)
            print("Sorting visits")
            visits.sort_values(VISIT_SORT_COLS, inplace=True)

            print("Saving visits", flush=True)
            write_csv(args.out_dir, args.visits_file, visits)

    else:
        visits = read_csv(args.in_dir, args.visits_file, args.num_visits)

        print(f"Updating visits {id_col}s", flush=True)
        visits = update_chunk(args, visits, id_update, id_col=id_col)
        if sort_values:
            print("Sorting visits")
            visits.sort_values(VISIT_SORT_COLS, inplace=True)

        print("Saving visits", flush=True)
        write_csv(args.out_dir, args.visits_file, visits)


def main(args):
    if not os.path.isdir(args.out_dir):
        os.makedirs(args.out_dir)

    if "locations" in args.to_partition:
        offsets, lid_update = partition_locations(args)
        if not args.offsets_only:
            update_visits(args, lid_update, sort_values=False)
            if "people" not in args.to_partition:
                create_textproto(
                    args.out_dir, args.visits_file, VISITS_TYPES, metadata_type="visits"
                )

        create_offsets_textproto(
            args.out_dir, f"location_offsets_{len(offsets)}.textproto", offsets
        )
        if not args.set_default:
            offsets = None
        if args.set_default and not args.offsets_only:
            create_textproto(
                args.out_dir,
                args.locations_file,
                LOCATIONS_TYPES,
                partition_offsets=offsets,
            )
    elif args.in_dir != args.out_dir:
        shutil.copy(os.path.join(args.in_dir, args.locations_file), args.out_dir)

    if "people" in args.to_partition:
        offsets, pid_update = partition_people(args)
        if not args.offsets_only:
            update_visits(args, pid_update, id_col="pid")
            create_textproto(
                args.out_dir, args.visits_file, VISITS_TYPES, metadata_type="visits"
            )
        create_offsets_textproto(
            args.out_dir, f"person_offsets_{len(offsets)}.textproto", offsets
        )
        if not args.set_default:
            offsets = None
        if args.set_default and not args.offsets_only:
            create_textproto(
                args.out_dir, args.people_file, PEOPLE_TYPES, partition_offsets=offsets
        )
    elif args.in_dir != args.out_dir:
        shutil.copy(os.path.join(args.in_dir, args.people_file), args.out_dir)


if __name__ == "__main__":
    main(parse_args())
