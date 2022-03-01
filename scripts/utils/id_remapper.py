# Copyright 2020 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT
#

# Sets the 'pid' column in people and the 'lid' column in locations
# to the given array/series of values passed as new_people_ids and
# new_location_ids, respectively, and adjusts the corresponding column(s)
# in visits accordingly
def remap(people, locations, visits,
        new_people_ids=None, new_location_ids=None):
    if new_people_ids is not None:
        # Remap person ids
        people = people.reindex(new_people_ids) # makes a copy
        people = people.reset_index(drop=True)
        people['temp_id'] = people.index
        person_remapper = people[['pid', 'temp_id']].copy()
        people['pid'] = people['temp_id']
        people.drop(["temp_id"], axis=1, inplace=True)
        visits = visits.merge(person_remapper, left_on='pid', right_on='pid')
        visits['pid']= visits['temp_id']
        visits.drop(["temp_id"], axis = 1, inplace=True)

    if new_location_ids is not None:
        # Remap location ids
        locations = locations.reindex(new_location_ids) # makes a copy
        locations = locations.reset_index(drop=True)
        locations['temp_id'] = locations.index
        loc_remapper = locations[['lid', 'temp_id']].copy()
        locations['lid'] = locations['temp_id']
        locations.drop(["temp_id"], axis=1, inplace=True)
        visits = visits.merge(loc_remapper, left_on='lid', right_on='lid')
        visits['lid']= visits['temp_id']
        visits.drop(["temp_id"], axis = 1, inplace=True)

    return people, locations, visits
