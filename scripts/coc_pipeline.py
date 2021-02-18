"""
Remaps city-of-charlotville dataset to use a 
"""
import os
import sys
import pandas as pd

_COC_VALUES = {
    "work": 0,
    "school": 0,
    "other": 0,
    "college": 0,
    "religion": 0,
    "designation": "none:home"
}

def combine_residences_and_activities(activity_locations, residence_locations):
    # Make sure all columns are shared.
    for col, def_val in _COC_VALUES.items():
        residence_locations[col] = def_val
    residence_locations["home"] = 1
    activity_locations["home"] = 0
    
    return activity_locations.append(residence_locations).reset_index().drop("index", axis=1)

def id_remapper(people, locations, visits):
    groups = [
        (people, 'pid'),
        (locations, 'lid')
    ]

    # Remap location ids.
    for to_remap, key in groups:
        # Remaps the dataframes existing index to a new dense index.
        to_remap['new_id'] = to_remap.index
        remapper = to_remap[[key, 'new_id']].copy()
        to_remap[key] = to_remap['new_id']
        to_remap.drop(["new_id"], axis=1, inplace=True)

        # Replaced foreign key references.
        # for df in foreign_dfs:
        # Replace all references of the old keys with the new ones
        visits = visits.merge(remapper, left_on=key, right_on=key)
        visits[key]= visits['new_id']
        visits.drop(["new_id"], axis = 1, inplace=True)
            
    return people, locations, visits

if __name__ == "__main__":
    # Required argument is the path to folder and optional override.
    if len(sys.argv) not in [2,3]:
        print("Usage coc_pineline.py <path_to_coc_folder>")
        exit(1)
    path_to_coc = sys.argv[1]
    if path_to_coc.endswith("/"):
        path_to_coc = path_to_coc[:-1]
    override = len(sys.argv) == 3 and sys.argv[2] == "--override"


    # Get user confirmation.
    if not override:
        print(f"This script will overwrite the existing files at {path_to_coc}")
        val = input("Enter 'Yes' to confirm:")
        if val != "Yes":
            print("Aborting...") 
            exit(0)

    # Read in all the datasetes.
    people = pd.read_csv(f"{path_to_coc}/coc_person.csv")
    locations = pd.read_csv(f"{path_to_coc}/coc_activity_locations.csv")
    residences = pd.read_csv(f"{path_to_coc}/coc_residence_locations.csv")
    visits = pd.read_csv(f"{path_to_coc}/coc_visits.csv")

    # Combines activity and residence locations.
    combined = combine_residences_and_activities(locations, residences)

    # Remap all ids
    people, combined, visits = id_remapper(people, combined, visits)

    # Cleanup
    os.remove(f"{path_to_coc}/coc_person.csv")
    os.remove(f"{path_to_coc}/coc_activity_locations.csv")
    os.remove(f"{path_to_coc}/coc_residence_locations.csv")
    os.remove(f"{path_to_coc}/coc_visits.csv")

    # Output
    people.to_csv(f"{path_to_coc}/people.csv", index=False)
    combined.to_csv(f"{path_to_coc}/locations.csv", index=False)
    visits.to_csv(f"{path_to_coc}/visits.csv", index=False)     
