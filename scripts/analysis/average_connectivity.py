# Usage - average_connectivity.py (visits.csv file path) (sample weight - "None", "Degree", or "Degree_Inverse") (sample size)

import igraph as ig
import pandas as pd
import sys
import numpy as np

# returns sample as 2D array of vertex pairs
def sample(G, weight, sample_size):
    if weight == "None":
        random_vertex_indices = np.random.choice(G.vcount(), size=(sample_size, 2), replace=False, p=None)
    
    else:
        degrees = G.degree()
        sum = 0

        if (weight == "Degree"):
            for i in degrees:
                sum += i
            probabilities = []

            for i in degrees:
                probabilities.append(i / sum)

            random_vertex_indices = np.random.choice(G.vcount(), size=(sample_size, 2), replace=False, p=probabilities)

        elif (weight == "Degree_Inverse"):
            for i in degrees:
                sum += (1/i)
            probabilities = []

            for i in degrees:
                probabilities.append((1/i) / sum)

            random_vertex_indices = np.random.choice(G.vcount(), size=(sample_size, 2), replace=False, p=probabilities)
        
        else:
            raise ValueError("Choose weight: None, Degree, or Degree_Inverse")


    return random_vertex_indices


arguments = sys.argv

try:
    file_name = arguments[1]
    weight = arguments[2]
    sample_size = int(arguments[3])
except IndexError:
    raise IndexError("Provide all three command-line arguments please")


try:
    df = pd.read_csv(file_name)
    print("Loaded CSV " + file_name + "\n")
except:
    raise FileNotFoundError("Please provide a valid path to the visits CSV file as a command line argument.\n") 


df['pid'] = df['pid'].apply(lambda x: 'p' + str(x))
df['lid'] = df['lid'].apply(lambda x: 'l' + str(x))
print("Updated PID and LID names to be unique \n")

G = ig.Graph.TupleList(df.itertuples(index=False), directed=False)
print("Loaded graph into igraph")
print("Graph has " + str(G.vcount()) + " vertices and " + str(G.ecount()) + " edges \n")

random_sample = sample(G, weight, int(sample_size))
print("Gathered sample \n")

sum = 0 
for i in random_sample:
    cuts = G.mincut_value(source = i[0], target = i[1])
    sum += cuts

print("The approximate average connectivity is: " + str(sum / sample_size))



