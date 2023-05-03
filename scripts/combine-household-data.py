#!/usr/bin/env python3

import argparse
import os
import pandas as pd
import sys

from utils.ids import partitioned_merge, init_multiprocessing

# ----------------------------------------------------------------------


def CreateParser():
    cli_parser = argparse.ArgumentParser(
        description="Dorm remapping tool - makes Home become Other"
    )

    cli_parser.add_argument(
        "-s",
        "--region_prefix",
        dest="region_prefix",
        type=str,
        required=True,
        help="Region prefix",
    )

    cli_parser.add_argument(
        "-p",
        "--person_filename",
        dest="person_filename",
        type=str,
        required=False,
        help="Person filename",
    )

    cli_parser.add_argument(
        "-l",
        "--location_assignment_filename",
        dest="location_assignment_filename",
        type=str,
        required=False,
        help="Location assignment filename",
    )

    cli_parser.add_argument(
        "-i",
        "--input_dir",
        dest="input_dir",
        type=str,
        default="",
        required=False,
        help="input directory",
    )

    cli_parser.add_argument(
        "-o",
        "--output_dir",
        dest="output_dir",
        type=str,
        default="",
        required=False,
        help="output directory",
    )
    cli_parser.add_argument(
        "-n",
        "--num_tasks",
        default=1,
        type=int,
        help="Specifies the number of processes " + "to use (default is serial)",
    )
    cli_parser.add_argument(
        "-np",
        "--num_partitions",
        default=16,
        type=int,
        help="Specifies the number of partitions to "
        + "seperate data into before merging",
    )

    return cli_parser


def FileReport(filename, message):
    if not os.path.exists(filename):
        print(f"Missing file <{filename}>")
        # sys.exit(-1)
    else:
        print(f"Using {message}:  <{filename}>")


def main():
    print("starting run")

    # ----------------------------------------------------------------------
    # Parser construction
    cli_parser = CreateParser()
    args = cli_parser.parse_args()

    input_dir = args.input_dir
    if input_dir == "":
        input_dir = os.getcwd()
    output_dir = args.output_dir
    if output_dir == "":
        output_dir = os.getcwd()

    num_partitions = args.num_partitions
    num_tasks = args.num_tasks

    if num_tasks > 1:
        init_multiprocessing()

    region_prefix = args.region_prefix

    print("parsed args")

    p_filename = os.path.join(
        input_dir, "base_population", region_prefix + "_person.csv"
    )
    h_filename = os.path.join(
        input_dir, "base_population", region_prefix + "_household.csv"
    )
    # rl_filename = os.path.join(input_dir, region_prefix
    #                            + '_residence_locations_final.csv')
    # al_filename = os.path.join(input_dir, region_prefix '
    #                            + '_activity_locations_final.csv')
    hra_filename = os.path.join(
        input_dir,
        "home_location_assignment",
        region_prefix + "_household_residence_assignment.csv",
    )
    # ala_filename = os.path.join(input_dir, region_prefix
    #                             + '_activity_location_assignment'
    #                             + '_week_final.csv')

    print("extracted file names")

    # print(f'#> Residences: {rl_filename}')
    # print(f'#> Activities: {al_filename}')
    print(f"#> HRA: {hra_filename}")
    # print(f'#> ALA: {ala_filename}')

    FileReport(p_filename, "person file")
    FileReport(h_filename, "household file")
    # FileReport(rl_filename, 'residence location file')
    # FileReport(al_filename, 'activity location file')
    FileReport(hra_filename, "household-to-residence file")
    # FileReport(ala_filename, 'activity location assignment file')

    print("starting to read in files")

    # Target header:
    # pid,hid,age,gender,employment_status,race,hispanic,designation,
    # hh_size,hh_income,workers_in_family,lid, admin1,admin2,admin3,admin4,
    # residence_longitude,residence_latitude

    # Person file header:
    # hid,pid,serialno,person_number,record_type,age,relationship,sex,
    # school_enrollment,grade_level_attending,employment_status,occupation_socp,
    # race,hispanic,designation
    person_df = pd.read_csv(
        p_filename,
        usecols=[
            "pid",
            "hid",
            "age",
            "sex",
            "employment_status",
            "designation",
            "race",
            "hispanic",
        ],
    )
    print("person_df:")
    print(person_df)
    # print('person df memory usage:', person_df.memory_usage(deep=True).sum())
    # print(memory_usage())

    # admin1,admin2,admin3,admin4,hid,serialno,puma,record_type,hh_unit_wt,
    # hh_size,vehicles,hh_income,units_in_structure,business,heating_fuel,
    # household_language,family_type_and_employment_status,workers_in_family
    # p_h_df=None
    household_df = pd.read_csv(
        h_filename, usecols=["hid", "hh_size", "hh_income", "workers_in_family"]
    )
    print("household_df:")
    print(household_df)
    # print('household df memory usage:',
    #       household_df.memory_usage(deep=True).sum())

    p_h_df = person_df.merge(household_df, how="left", left_on="hid", right_on="hid")
    print("p_h_df:")
    print(p_h_df)
    # print('p-h combined df memory usage:',
    #       p_h_df.memory_usage(deep=True).sum())
    # print(memory_usage())

    # hid,lid,longitude,latitude,altitude,admin1,admin2,admin3,admin4,
    # area_sqm,associate_link_func_class,pub_pk,pub_kg,pub_01,pub_02,
    # pub_03,pub_04,pub_05,pub_06,pub_07,pub_08,pub_09,pub_10,pub_11,pub_12
    # gidi_person_df=None
    hra_df = pd.read_csv(
        hra_filename,
        usecols=[
            "hid",
            "rlid",
            "longitude",
            "latitude",
            "admin1",
            "admin2",
            "admin3",
            "admin4",
        ],
    )
    print("hra_df:")
    print(hra_df)
    hra_df.rename(columns={"rlid": "lid"}, inplace=True)
    print("hra_df after rename:")
    print(hra_df)
    # print('hra df memory usage:', hra_df.memory_usage(deep=True).sum())
    # print(memory_usage(all_vars=locals()))

    # print('min:', p_h_df['hid'].min(), 'max:', p_h_df['hid'].max())
    # print('min:', hra_df['hid'].min(), 'max:', hra_df['hid'].max())

    # We get memory errors from tryign to merge the full dataset, so try
    # merging along partitions
    gidi_person_df = partitioned_merge(
        p_h_df,
        hra_df,
        "hid",
        num_partitions=num_partitions,
        num_tasks=num_tasks,
        args={"how": "left"},
    )
    # p_h_partitioned=partition_df(p_h_df, num_partitions=num_partitions)
    # hra_partitioned=partition_df(hra_df, num_partitions=num_partitions)

    # gidi_person_partitioned=[p_h_partitioned[i].merge(
    #     hra_partitioned[i], how='left', left_on='hid', right_on='hid',
    # ) for i in range(num_partitions-1)]
    # gidi_person_df=pd.concat(gidi_person_partitioned)

    # min: 0 max: 12848291
    # gidi_person_df=p_h_df.merge(hra_df, how='left', left_on='hid',
    #        right_on='hid', copy=False)
    # print('gidi person df memory usage:',
    #       gidi_person_df.memory_usage(deep=True))

    # Zero out any missing columns
    gidi_person_df.fillna(0, inplace=True)

    # Convert categorical variables to ints (ones with missing data were
    # forced to become floats in order to suppoet having nas)
    gidi_person_df = gidi_person_df.astype(
        {"employment_status": int, "workers_in_family": int}, copy=False
    )
    # print(memory_usage())

    gidi_person_df.to_csv(
        os.path.join(output_dir, region_prefix + "_gidi_person.csv.gz"), index=False
    )

    sys.exit(0)


main()
