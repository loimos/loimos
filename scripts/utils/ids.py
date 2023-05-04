# Copyright 2020 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT
#

import numpy as np
import pandas as pd
import functools
from multiprocessing import Pool, set_start_method


def init_multiprocessing(start_method="forkserver"):
    set_start_method(start_method)


# Returns the boundaries for a given number of equally sized partitions on
# the specified columns. Pass in both left and right cols to ensure both
# datasets share partition bounds.
def get_bounds(left_col, right_col=None, num_partitions=10):
    col_min = left_col.min()
    col_max = left_col.max()

    if right_col is not None:
        col_min = min(col_min, right_col.min())
        col_max = max(col_max, right_col.max())

    # Add one so that we don't loose the rows with the last id
    bounds = np.linspace(col_min, col_max + 1, num=num_partitions + 1, dtype=int)

    return bounds


# Assumes the values in column 'on' are of a numerical type
def partition_df(df, on="hid", num_partitions=10, bounds=None):
    # Choose bounds from number of partitions, if no bounds are explcitly given
    if bounds is None:
        bounds = get_bounds(df[on], num_partitions=num_partitions)
    else:
        num_partitions = len(bounds) - 1

    bounded_dfs = []
    for i in range(num_partitions):
        bounds_mask = (bounds[i] <= df[on]) & (df[on] < bounds[i + 1])
        bounded_dfs.append(df[bounds_mask])
    return bounded_dfs


def partitioned_merge(left, right, on, num_tasks=1, num_partitions=128, args={}):
    # Both dataframes should share the same bounds, so that correpsonding
    # ids are in the same partitions
    bounds = get_bounds(left[on], right[on], num_partitions=num_partitions)

    left_partitions = partition_df(left, on=on, bounds=bounds)
    right_partitions = partition_df(right, on=on, bounds=bounds)

    # for i in range(num_partitions):
    #    print(f'{i}-th left partition:')
    #    print(left_partitions[i])
    #    print(f'{i}-th right partition:')
    #    print(right_partitions[i])

    merged_partitions = []
    if 1 == num_tasks:
        merged_partitions = [
            left_partitions[i].merge(right_partitions[i], on=on, **args)
            for i in range(num_partitions)
        ]
    else:
        with Pool(num_tasks) as pool:
            merged_partitions = pool.starmap(
                functools.partial(pd.DataFrame.merge, on=on, **args),
                zip(left_partitions, right_partitions),
            )

    # for i in range(num_partitions):
    #    print(f'{i}-th merged partition:')
    #    print(merged_partitions[i])

    merged_partitions = filter(lambda p: p.shape[0] != 0, merged_partitions)

    # i = 0
    # for p in merged_partitions:
    #    print(f'{i}-th merged partition:')
    #    print(p)
    #    print(p.shape)
    #    i += 1

    merged_df = pd.concat(merged_partitions)

    return merged_df


# Sets the 'pid' column in people and the 'lid' column in locations
# to the given array/series of values passed as new_people_ids and
# new_location_ids, respectively, and adjusts the corresponding column(s)
# in visits accordingly
def remap(
    people,
    locations,
    visits,
    new_people_ids=None,
    new_location_ids=None,
    num_partitions=1,
    num_tasks=1,
):
    if new_people_ids is not None:
        # Remap person ids
        people = people.reindex(new_people_ids)  # makes a copy
        people = people.reset_index(drop=True)
        people["temp_id"] = people.index
        person_remapper = people[["pid", "temp_id"]].copy()

        people["pid"] = people["temp_id"]
        people.drop(["temp_id"], axis=1, inplace=True)

        if 1 == num_partitions:
            visits = visits.merge(person_remapper, on="pid")
        else:
            visits = partitioned_merge(
                visits,
                person_remapper,
                "pid",
                num_tasks=num_tasks,
                args={"how": "left"},
            )
        visits["pid"] = visits["temp_id"]
        visits.drop(["temp_id"], axis=1, inplace=True)

    if new_location_ids is not None:
        # Remap location ids
        locations = locations.reindex(new_location_ids)  # makes a copy
        locations = locations.reset_index(drop=True)
        locations["temp_id"] = locations.index
        loc_remapper = locations[["lid", "temp_id"]].copy()

        locations["lid"] = locations["temp_id"]
        locations.drop(["temp_id"], axis=1, inplace=True)

        if 1 == num_partitions:
            visits = visits.merge(loc_remapper, on="lid")
        else:
            visits = partitioned_merge(
                visits, loc_remapper, "lid", num_tasks=num_tasks, args={"how": "left"}
            )
        visits["lid"] = visits["temp_id"]
        visits.drop(["temp_id"], axis=1, inplace=True)

        # if 1 == num_partitions:
        #    people = people.merge(loc_remapper, on='lid')
        # else:
        #    people = partitioned_merge(people, loc_remapper, 'lid',
        #            num_tasks=num_tasks, args={'how': 'left'})
        # people['hid'] = people['temp_id']
        # people.drop(["temp_id"], axis = 1, inplace=True)

    # Partitioned merges can sometimes mess up the order
    # people.sort_values(by='pid', inplace=True)
    visits.sort_values(by=["pid", "start_time"], inplace=True)

    return people, locations, visits
