# Copyright 2021 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT
"""Translates normal graph (not bi-partite) to people/locations/visits.
 
1) Denotes each node in as a home-location with the existing edges between
nodes representing where people in that home location are "allowed" to travel to.
2) Create people for each home location.
3) For each day and for each person, randomly create a schedule that splits
time between home location and "allowed" other locations.
"""
import random
import pandas as pd
from typing import Any, List
import os
import shutil

RANDOM_SEED = 42
random.seed(RANDOM_SEED)

# CONSTANTS
NUM_DAYS = 7
PEOPLE_PER_LOCATION = lambda: random.randint(3, 5)
VISITS_PER_PERSON_PER_DAY = lambda: random.randint(5, 7)
OCCUPIED_HOURS_PER_DAY = lambda: random.randint(14, 18)
HOURS_OF_THE_DAY = list(range(24))
DAY_LENGTH = 60 * 60 * 24

class CSVWriter():
    """Lightweight wrapper to write continously to a CSV file."""
    def __init__(self, filename, headers, out_dir):
        if out_dir:
            self.file = open(os.path.join(out_dir, filename), "w")
        else:
            self.file = open(filename, "w")

        if not self.file:
            raise OSError("Error opening file.")
        self.write_row(headers)

    def __del__(self):
        self.file.close()

    def write_row(self, objects: List[Any]):
        self.file.write(','.join(map(str, objects)) + '\n')


def graph_to_disease_model(graph, out_dir, template_dir):
    """Translates a standard graph to a bi-partite population model for loimos."""
    # Create output folder if it doesn't exist
    if out_dir and not os.path.exists(out_dir):
        os.makedirs(out_dir)

        # ...and copy over textproto templates
        for template in ['people', 'visits', 'locations']:
            filename = f'{template}.textproto'
            shutil.copyfile(
                    os.path.join(template_dir, filename),
                    os.path.join(out_dir, filename))
    
    # Create dataframes holding visit information.
    location_writer = CSVWriter("locations.csv", ['lid'], out_dir)
    people_writer = CSVWriter("people.csv", ['pid'], out_dir)
    visit_writer = CSVWriter("visits.csv",
                             ['pid', 'lid', 'start_time', 'duration'],
                             out_dir)

    people_created = 0
    for home_location in graph.Nodes():
        location_id = home_location.GetId()
        location_writer.write_row([location_id])
        allowed_visit_locations = ([location_id] +
                                   list(home_location.GetOutEdges()))
        # Generate schedule for each person.
        for _ in range(PEOPLE_PER_LOCATION()):
            person_id = people_created
            people_created += 1
            people_writer.write_row([person_id])

            for day in range(NUM_DAYS):
                # Randomly split up time of day for each "activity"
                hours_occupied_for_day = OCCUPIED_HOURS_PER_DAY()
                visits_for_day = VISITS_PER_PERSON_PER_DAY()
                locations_to_visit = random.choices(allowed_visit_locations,
                                                    k=visits_for_day)
                sleeping_hours = (24 - hours_occupied_for_day) // 2
                activity_start_times = sorted(
                    random.sample(
                        HOURS_OF_THE_DAY[sleeping_hours:24 - sleeping_hours +
                                         1], visits_for_day + 1))

                # Add visits.
                for i in range(visits_for_day):
                    location_id = locations_to_visit[i]
                    start_time = 3600 * activity_start_times[i]
                    end_time = 3600 * activity_start_times[i + 1]
                    absolute_start_time = DAY_LENGTH * day + start_time
                    duration = end_time - start_time
                    visit_writer.write_row([
                        person_id, location_id, absolute_start_time, duration
                    ])
