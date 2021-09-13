import pandas as pd
import numpy as np
'''
Introduces synthetic dataset that is setup such that a large 
portion of the population (risky subset) all visit a single location
for the entire day every day. The rest of the population is very safe
and stays in their room (only person at a location). We expect
all the risky people to get infected by the super infectious disease model.
And the healthy people to not get infected besides random initial infections.
'''

# Expect large outbreak will occur 
num_risky = 20000
num_safe = 5000
total_people = num_safe + num_risky 
total_days = 30
num_locations = num_safe + 1

# Single column csv for locations with no attributes.
pd.DataFrame(range(num_locations)).rename({0: "lid"}, axis=1).to_csv('locations.csv', index=False)

# Create people dataframe st risky people are 21 and safe are 65.
# Used to make sure chares are reading the proper data attributes.
people = pd.DataFrame(range(num_safe + num_risky)).rename({0: "pid"}, axis=1)
people['age'] = 21
people.loc[people.index[:num_safe], 'age'] = 65
people.to_csv('people.csv', index=False)

# Generate visit schedule s.t. risky people all visit the same location
# and risky people all visit the same location.
# visits = pd.DataFrame()
SECONDS_IN_DAY = 3600 * 24

# Init to row index.
def pidOfRow(x):
    return np.mod(x, total_people)

def pidToLocation(x):
    return np.where(x < num_safe, x+1, 0)

def determineStartTime(x):
    return np.floor(np.divide(x, total_people)) * SECONDS_IN_DAY + (SECONDS_IN_DAY / 4)

visits = pd.DataFrame(range(total_people * total_days)).rename({0: "pid"}, axis=1)
visits['start_time'] = determineStartTime(visits['pid'])
visits['pid'] = pidOfRow(visits['pid'])
visits['lid'] = pidToLocation(visits['pid'])
visits['duration'] = SECONDS_IN_DAY / 2
visits = visits.astype(int)
visits = visits[['pid', 'lid', 'start_time', 'duration']]
visits.to_csv('visits.csv', index=False)