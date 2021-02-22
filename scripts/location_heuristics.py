# Copyright 2021 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

"""
Given a visit schedule, this script calculates the total visits and maximum
simultaneous visits for all locations in the dataset. 

Inputs:
    path_to_visit_file: Path to visit schedule CSV. Must have a column "lid" that
        describes the location id.
    output_file (optional): Where to output calculated csv. 
    
Outputs:
    CSV to file with three columns - lid, total_visits, max_simultaneous_visits.
    Locations without any visits will not have an entry.
"""

# Column name that describes the location id being visited.
LOCATION_ID_COLUMN_NAME = "lid"
# Any other column in the dataset that is fully populated.
OTHER_COLUMN = "start_time"

import sys
import pandas as pd
import heapq

if __name__ == "__main__":
    # Required argument is the path to the visit schedule and optional output file.
    if len(sys.argv) not in [2,3]:
        print("Usage location_heuristics.py <path_to_visit_file> <(opt) output_file>")
        exit(1)

    # Get command line arguments.
    path_to_visits = sys.argv[1]
    # Output file defaults to given path with .heuristic file extension.
    output_file = path_to_visits[ : path_to_visits.rfind(".")] + ".heuristic"
    # User may manually specify an output path as well.
    if len(sys.argv) == 3:
        output_file = sys.argv[2]

    # Load dataset.
    visits = pd.read_csv(path_to_visits)

    # Calculate total visits to a location.
    max_visits = (
        visits[[LOCATION_ID_COLUMN_NAME, OTHER_COLUMN]]
            .groupby(LOCATION_ID_COLUMN_NAME).count()
            .rename({OTHER_COLUMN: "total_visits"}, axis=1)
    )

    # Calculate the maximum simulatenous visits.
    max_visits['max_simultaneous_visits'] = 0
    max_in_visit = 0
    end_times = []
    for lid, row in max_visits.iterrows():
        for _, row in visits[visits[LOCATION_ID_COLUMN_NAME] == lid][['start_time','end_time']].iterrows():
            # Filter out end_times not in range.
            start_time = row['start_time']
            while len(end_times) and end_times[0] <= start_time:
                heapq.heappop(end_times)

            # Append in sorted order.
            heapq.heappush(end_times, row['end_time'])
            max_in_visit = max(max_in_visit, len(end_times))
        max_visits.at[lid, "max_simultaneous_visits"] = max_in_visit
        
        # Reset counts for next row/location
        max_in_visit = 0
        end_times.clear()

    # Output with index column which is the lids.
    max_visits.to_csv(output_file)
