def remap(people, locations, visits, new_people_ids, new_location_ids):
    # Remap person ids.
    people = people.reset_index(drop=True).copy()
    people['temp_id'] = people.index
    person_remapper = people[['pid', 'temp_id']].copy()
    people['pid'] = people['temp_id']
    people.drop(["temp_id"], axis=1, inplace=True)
    visits = visits.merge(person_remapper, left_on='pid', right_on='pid')
    visits['pid']= visits['temp_id']
    visits.drop(["temp_id"], axis = 1, inplace=True)

    # Remap location ids.
    locations = locations.reset_index(drop=True).copy()
    locations['temp_id'] = locations.index
    loc_remapper = locations[['lid', 'temp_id']].copy()
    locations['lid'] = locations['temp_id']
    locations.drop(["temp_id"], axis=1, inplace=True)
    visits = visits.merge(loc_remapper, left_on='lid', right_on='lid')
    visits['lid']= visits['temp_id']
    visits.drop(["temp_id"], axis = 1, inplace=True)
    
    return people, locations, visits