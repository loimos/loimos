def remap(people, locations, visits, new_people_ids, new_location_ids):
    groups = [
        (people, 'pid', new_people_ids),
        (locations, 'lid', new_location_ids)
    ]

    # Remap location ids.
    
    new_data = []
    for to_remap, key, new_ids in groups:
        # Remaps the dataframes existing index to a new dense index.
        to_remap = to_remap.reset_index(drop=True).copy()
        to_remap['new_id'] = to_remap.index
        remapper = to_remap[[key, 'new_id']].copy()
        to_remap[key] = to_remap['new_id']
        to_remap.drop(["new_id"], axis=1, inplace=True)
        new_data.append(to_remap)

        # Replaced foreign key references.
        # for df in foreign_dfs:
        # Replace all references of the old keys with the new ones
        visits = visits.merge(remapper, left_on=key, right_on=key)
        visits[key]= visits['new_id']
        visits.drop(["new_id"], axis = 1, inplace=True)
            
    new_data.append(visits)
    return new_data
    return people, locations, visits