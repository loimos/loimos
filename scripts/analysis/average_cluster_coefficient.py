# Usage - average_cluster_coefficient.py (visits.csv file path)

import igraph as ig
import pandas as pd
import sys
import random

arguments = sys.argv

try:
    file_name = arguments[1]
except IndexError:
    raise IndexError("Provide all the path to a visits.csv file as an argument please")

try:
    df = pd.read_csv(file_name)
    print("Loaded CSV " + file_name + "\n", flush=True)
except:
    raise FileNotFoundError("Please provide a valid path to the visits CSV file as a command line argument.\n") 


df['pid'] = df['pid'].apply(lambda x: 'p' + str(x))
df['lid'] = df['lid'].apply(lambda x: 'l' + str(x))
print("Updated PID and LID names to be unique \n", flush=True)

G = ig.Graph.TupleList(df.itertuples(index=False), directed=False)
print("Loaded graph into igraph", flush=True)
print("Graph has " + str(G.vcount()) + " vertices and " + str(G.ecount()) + " edges\n", flush=True)


num_vertices = 0
tot_coefficient = 0

shuffled_vertices = random.sample(list(G.vs), len(G.vs))

for i in shuffled_vertices:
    if i["name"][0] != "p":
        continue

    print(i["name"])

    num_combs = 0
    tot_local_coefficient = 0 
    num_vertices += 1

    neighbors = G.neighbors(i)
    if len(neighbors) < 2:
        continue

    for m in range(len(neighbors)):
        for n in range(m + 1, len(neighbors)):
            common_neighbors = set(G.neighbors(neighbors[m])) & set(G.neighbors(neighbors[n]))
            common_neighbors.discard(i.index)

            qimn = len(common_neighbors)
            km = len(G.neighbors(neighbors[m]))
            kn = len(G.neighbors(neighbors[n]))
            nimn = 1 + qimn

            tot_local_coefficient += (qimn) / ((km - nimn) + (kn - nimn) + qimn)
            num_combs += 1

    tot_coefficient += (tot_local_coefficient / num_combs)

    if (num_vertices % 10 == 0):
        print (str(num_vertices) + " vertices: " + str(tot_coefficient / num_vertices), flush=True)


print("\n \n The overall average cluster coefficient using squares is: " + str(tot_coefficient / num_vertices), flush=True)
