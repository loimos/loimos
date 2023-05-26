#!/usr/bin/env python3

import argparse
import os

import pandas as pd

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

    # Named/optional arguments:
    parser.add_argument(
        "-o", "--out-dir",
        default=None,
        help="The name of the directory where the sampled population "
             + "should be saved. If this argument is not specified, "
             + "the same path will be used for the input and output "
             + "directories"
    )
    parser.add_argument(
        "-pi",
        "--people-in-file",
        default=os.path.join("base_population", "{region}_person.csv"),
        help="The name of the file containing person data within the "
        + "population dir",
    )
    parser.add_argument(
        "-ri",
        "--residences-in-file",
        default=os.path.join("locations", "{region}_residence_locations.csv"),
        help="The name of the file containing home location data within the "
        + "population dir",
    )
    parser.add_argument(
        "-ai",
        "--activity-locs-in-file",
        default=os.path.join("locations", "{region}_activity_locations.csv"),
        help="The name of the file containing home location data within the "
        + "population dir",
    )
    parser.add_argument(
        "-rai",
        "--residences-assignments-in-file",
        default=os.path.join(
            "home_location_assignment", "{region}_household_residence_assignment.csv"
        ),
        help="The name of the file asigning households to home locations "
        + "within the population dir",
    )
    parser.add_argument(
        "-aai",
        "--activity-loc-adult-assignments-in-file",
        default=os.path.join(
            "location_assignment",
            "weekly",
            "{region}_adult_activity_location_assignment_week.csv",
        ),
        help="The name of the file containing adult visit data within the "
        + "population dir",
    )
    parser.add_argument(
        "-aci",
        "--activity-loc-child-assignments-in-file",
        default=os.path.join(
            "location_assignment",
            "weekly",
            "{region}_child_activity_location_assignment_week.csv",
        ),
        help="The name of the file containing adult visit data within the "
        + "population dir",
    )
    parser.add_argument(
        "-po",
        "--people-out-file",
        default="people.csv",
        help="The name of the file containing person data within the "
        + "population dir",
    )
    parser.add_argument(
        "-lo",
        "--locations-out-file",
        default="locations.csv",
        help="The name of the file containing location data within the "
        + "population dir",
    )
    parser.add_argument(
        "-vo",
        "--visits-out-file",
        default="visits.csv",
        help="The name of the file containing visit data within the "
        + "population dir",
    )
    parser.add_argument(
        "-f",
        "--flat",
        action="store_true",
        help="Pass this flag if the script should expect a flat input directory",
    )

    args = parser.parse_args()

    # Assume out and in dirs are the same by default for convience
    if args.out_dir is None:
        args.out_dir = args.in_dir

    return args

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


def make_contiguous(df, id_col="lid", offset=0, name="df"):
    # Check if its already contiguous
    if (1 == df[id_col].diff()).all():
        print(f"  {name} are already contiguous; subtracting offset")

        offset -= df[id_col].iloc[0]
        df[id_col] += offset

        return offset

    else:
        print(f"  {name} are not already contiguous; using index")

        index_col = f"new_{id_col}"
        old_col = f"old_{id_col}"
        update_record = pd.DataFrame({index_col: df.index, id_col: df[id_col]})
        df[old_col] = df[id_col]
        df[id_col] = df.index + offset

        return update_record


def update_ids(df, update, id_col="lid", name="df"):
    # If update is offset
    if isinstance(update, int):
        print(f"  Updating {name} using offset")
        df[id_col] += update
        return df

    # for arbitary updates (should be DF with two columns: new_col and id_col
    elif isinstance(update, pd.DataFrame):
        print(f"  Updating {name} using arbitary mapping")
        new_col = update.columns[0]
        new_df = pd.merge(df, update)
        new_df.drop(columns=id_col, inplace=True)
        new_df.rename(columns={new_col: id_col}, inplace=True)

        return new_df


def merge_locations(args):
    in_dir = args.in_dir

    activity_locs = read_csv(
        in_dir, args.activity_locs_in_file, args.region, should_flatten=args.flat
    )
    home_locs = read_csv(
        in_dir, args.residences_in_file, args.region, should_flatten=args.flat
    )

    activity_locs.rename(columns={"alid": "lid"}, inplace=True)
    home_locs.rename(columns={"rlid": "lid"}, inplace=True)

    activity_update = make_contiguous(activity_locs,
                                      name="activity locations")
    home_update = make_contiguous(
        home_locs, offset=activity_locs["lid"].iloc[-1] + 1,
        name="home locations"
    )

    # Make sure all columns are shared.
    for col, def_val in DEFAULT_VALUES.items():
        home_locs[col] = def_val
    home_locs["home"] = 1
    activity_locs["home"] = 0

    locations = pd.concat([activity_locs, home_locs], ignore_index=True)
    write_csv(args.out_dir, args.locations_out_file, locations)

    return activity_update, home_update


def fix_people(args):
    people = read_csv(
        args.in_dir, args.people_in_file, args.region, should_flatten=args.flat
    )
    people_update = make_contiguous(people, id_col="pid", name="people")
    write_csv(args.out_dir, args.people_out_file, people)

    return people_update


def merge_visits(args, activity_update, home_update, people_update):
    in_dir = args.in_dir

    adult_activity_visits = read_csv(
        in_dir,
        args.activity_loc_adult_assignments_in_file,
        args.region,
        should_flatten=args.flat,
    )
    child_activity_visits = read_csv(
        in_dir,
        args.activity_loc_child_assignments_in_file,
        args.region,
        should_flatten=args.flat,
    )

    visits = pd.concat(
        [adult_activity_visits, child_activity_visits], ignore_index=True
    )

    loc_update = activity_update
    if isinstance(activity_update, pd.DataFrame):
        loc_update = pd.concat([activity_update, home_update])

    visits = update_ids(visits, loc_update, name="visit lids")
    visits = update_ids(visits, people_update, id_col="pid", name="visit pids")
    visits.sort_values(["pid", "start_time"], inplace=True)

    write_csv(args.out_dir, args.visits_out_file, visits)


def main():
    args = parse_args()

    activity_update, home_update = merge_locations(args)
    people_update = fix_people(args)
    merge_visits(args, activity_update, home_update, people_update)


if __name__ == "__main__":
    main()
