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
        if 2 == len(right_col.shape):
            right_col = right_col.iloc[:,0]
        col_min = min(col_min, right_col.min())
        col_max = max(col_max, right_col.max())

    # Add one so that we don't loose the rows with the last id
    bounds = np.linspace(col_min, col_max + 1, num=num_partitions + 1, dtype=int)

    return bounds


# Assumes the values in column 'on' are of a numerical type
def partition_df(df, on="hid", num_partitions=10, bounds=None):
    # Make sure that we are only using the first provided column,
    # as we only want 1D bounds
    first_col = df[on].iloc[:,0]

    # Choose bounds from number of partitions, if no bounds are explcitly given
    if bounds is None:
        bounds = get_bounds(df[on], num_partitions=num_partitions)
    else:
        num_partitions = len(bounds) - 1

    bounded_dfs = []
    for i in range(num_partitions):
        bounds_mask = (bounds[i] <= first_col) & (first_col < bounds[i + 1])
        bounded_dfs.append(df[bounds_mask])
    return bounded_dfs


def partitioned_merge(left, right, on, num_tasks=1, num_partitions=128,
                      init_mp=False, sort_by=None, **args):
    if init_mp:
        init_multiprocessing()

    # Both dataframes should share the same bounds, so that correpsonding
    # ids are in the same partitions
    bounds = get_bounds(left[on].iloc[:,0], right[on].iloc[:,0],
                        num_partitions=num_partitions)

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
    
    # Partitioning can mess up the order of the rows, so let's fix that
    if sort_by is not None:
        merged_df.sort_values(sort_by, inplace=True)
    merged_df.reset_index(inplace=True, drop=True)

    return merged_df


# Sets the 'pid' column in people and the 'lid' column in locations
# to match the row index in each dataframe, and adjusts the corresponding
# column(s) in visits accordingly
def remap(people, locations, visits, num_tasks=1, num_partitions=32):
    groups = [("people", "pid", ["visits"]), ("locations", "lid", ["visits", "people"])]
    data = {"people": people, "locations": locations, "visits": visits}

    # Remap location ids.
    for to_remap_name, key, external_references in groups:
        to_remap = data[to_remap_name]

        # print(f"{to_remap_name} before remap:")
        # print(to_remap)

        # Remaps the dataframes existing index to a new dense index.
        to_remap["new_id"] = to_remap.index
        remapper = to_remap[[key, "new_id"]].copy()
        to_remap[key] = to_remap["new_id"]
        to_remap.drop(["new_id"], axis=1, inplace=True)

        # print(f"{to_remap_name} after remap:")
        # print(to_remap)

        # Save changes t
        data[to_remap_name] = to_remap

        # Replaced foreign key references.
        # for df in foreign_dfs:
        # Replace all references of the old keys with the new ones
        for ref_name in external_references:
            ref = data[ref_name]
            # ref = ref.merge(remapper, how='left', left_on=key, right_on=key)
            ref = partitioned_merge(
                ref,
                remapper,
                key,
                num_partitions=num_partitions,
                num_tasks=num_tasks,
                args={"how": "left"},
            )
            ref[key] = ref["new_id"]
            ref.drop(["new_id"], axis=1, inplace=True)
            data[ref_name] = ref

    return data["people"], data["locations"], data["visits"]
