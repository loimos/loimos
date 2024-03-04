import pandas as pd
import random
import sys
import os

# Usage - graph_perturbation.py (path to current population directory) (proportion of visits to perturb) (path to new population directory)

arguments = sys.argv

try:
    in_dir = arguments[1]
except IndexError:
    raise IndexError("Provide the path to a population directory as an argument please.")

try:
    p = float(arguments[2])
    print(f"Proportion of graph to perturbate: {p}", flush=True)
except IndexError:
    raise IndexError("Provide the proportion of visits to perturb please.")

try: 
    out_dir = arguments[3]
except IndexError:
    raise IndexError("Provide the output population directory path please.")

try:
    df = pd.read_csv(os.path.join(in_dir, 'visits.csv'))
    print("Loaded CSV " + os.path.join(in_dir, 'visits.csv') + "\n", flush=True)
except:
    raise FileNotFoundError("Please provide a valid path to a population directory as a command line argument.\n") 


num_visits = len(df)
n = int(p * num_visits)

random_rows = df.sample(n)
print(random_rows)
unique_lids = df['lid'].nunique()
random_values = random.choices(range(unique_lids), k=n)

df.loc[random_rows.index, 'lid'] = random_values

print(f"Successfully perturbed {p} proportion of graph.", flush=True)

if not os.path.exists(out_dir):
    os.makedirs(out_dir)

print(f"Exporting new visits.csv file to {os.path.join(out_dir, 'visits.csv')}.", flush=True)
df.to_csv(os.path.join(out_dir, 'visits.csv'), index=False)

print("CSV exported, now creating symbolic links for all other files in the population directory.", flush=True)
soft_link_files = ['locations.csv', 'locations.textproto', 'people.csv', 'people.textproto', 'visits.textproto']

for file in soft_link_files:
    destination_path = os.path.join(out_dir, file)
    source_path = os.path.realpath(os.path.join(in_dir, file))

    os.symlink(source_path, destination_path)

print(f"Successfully created new population in directory {out_dir}", flush=True)
