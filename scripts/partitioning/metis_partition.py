#!/usr/bin/python3

import metis
import argparse
import os

import networkx as nx
import pandas as pd
import numpy as np

from sklearn import tree

def parse_args():

    parser = argparse.ArgumentParser(
        description="Calculates summary statistics for a given visit file."
    )

    # Positional/required arguments:
    parser.add_argument(
        "population_dir",
        metavar="P",
        help="The path to directory containing data files for a population",
    )

    # Named/optional arguments:
    parser.add_argument(
        "-v",
        "--visits-file",
        default="visits.csv",
        help="The name of the visits file within population_dir. Should be "
        + "a csv file with a column called 'lid' and a column called 'pid'",
    )
    parser.add_argument(
        "-l",
        "--locations-file",
        default="locations.csv",
        help="The name of the locations file within population_dir. Should "
        + "a csv file with a column called 'lid'",
    )
    parser.add_argument(
        "-p",
        "--people-file",
        default="people.csv",
        help="The name of the people file within population_dir. Should "
        + "a csv file with a column called 'pid'",
    )
    parser.add_argument(
        "-m",
        "--metrics-file",
        default="metrics.csv",
        help="The name of the metrics file within population_dir"
    )
    parser.add_argument(
        "-td",
        "--train-dir",
        default=None,
        help="The name of the metrics file within population_dir"
    )
    parser.add_argument(
        "-n",
        "--num-partitions",
        type=int,
        default=37,
        help="Number of partitions to create"
    )

    return parser.parse_args()


def set_load_est(args, people, locations, locations_mask, graph,
        load_col="loc_load", person_load_col = "person_load", num_people=0):
    train_metrics_path = os.path.join(args.train_dir, args.metrics_file)
    test_metrics_path = os.path.join(args.population_dir, args.metrics_file)
    train_metrics = pd.read_csv(train_metrics_path)

    test_metrics = None
    if os.path.isfile(test_metrics_path):
        test_metrics = pd.read_csv(test_metrics_path)

    time_model = train_load_predictor(train_metrics,
            tree.DecisionTreeRegressor(), test=test_metrics, use_log=True)

    locations.rename(columns={"total_visits": "num_visits"}, inplace=True)
    locations[load_col] = 0.0
    locations.loc[locations_mask, load_col] = np.exp(time_model.predict(
        np.log(locations[locations_mask][["num_visits"]])))

    # METIS requires integer weights
    unit_size = np.floor(np.log10(locations[locations_mask][load_col].min())) \
            - 1
    # print(unit_size)
    locations[load_col] *= 10**-unit_size
    locations[load_col] = locations[load_col].round()

    load_dict = {lid + num_people: int(locations.iloc[lid][load_col])
            for lid in locations["lid"]}
    nx.set_node_attributes(graph, load_dict, load_col)

    load_dict = {pid: int(people.iloc[pid][person_load_col])
            for pid in people["pid"]}
    nx.set_node_attributes(graph, load_dict, person_load_col)
    graph.graph["node_weight_attr"] = [load_col, person_load_col]

    return locations, graph


def get_vars(df, x_cols, y_cols, use_log=True):
    x = df[x_cols]
    y = df[y_cols]
    if use_log:
        y = np.log(y)
    return x, y


def train_load_predictor(train, predictor, test=None, x_cols=["num_visits"],
        y_cols=["time"], use_log=True):
    x_train, y_train = get_vars(train, x_cols, y_cols, use_log=use_log)

    predictor.fit(x_train, y_train)
    print(f"Train R^2: {predictor.score(x_train, y_train)}")
    if test is not None:
        x_test, y_test = get_vars(test, x_cols, y_cols, use_log=use_log)
        print(f"Test R^2: {predictor.score(x_test, y_test)}")

    return predictor


def main():
    args = parse_args()

    visits_path = os.path.join(args.population_dir, args.visits_file)
    locations_path = os.path.join(args.population_dir, args.locations_file)
    people_path = os.path.join(args.population_dir, args.people_file)

    visits = pd.read_csv(visits_path)
    people = pd.read_csv(people_path)
    locations = pd.read_csv(locations_path)

    num_people = people.shape[0]
    print(people.shape[0], locations.shape[0])
    print(len(visits["pid"].unique()), len(visits["lid"].unique()))
    visit_lids = visits["lid"].unique()
    locations_mask = locations["lid"].isin(visit_lids)

    people["person_load"] = \
            visits[["pid", "start_time"]].groupby(by="pid").count()
    #print(people["person_load"])

    visits["lid"] += num_people
    graph = nx.from_pandas_edgelist(visits, source="lid", target="pid")
    print(graph)

    tolerances = None
    if args.train_dir is not None:
        locations, graph = set_load_est(args, people, locations,
                locations_mask, graph, num_people=num_people)
        tolerances = [1.5, 1.5]

    _, partitioned = metis.part_graph(graph, nparts=args.num_partitions,
            ubvec=tolerances)
    people["partition"] = partitioned[0:num_people]
    locations["partition"] = -1
    locations.loc[locations_mask, "partition"] = partitioned[num_people:]

    pid_bins = people[["person_load", "partition"]].groupby("partition")
    lid_bins = locations[["loc_load", "partition"]].groupby("partition")
    print(pid_bins.count())
    print(lid_bins.count())
    print(pid_bins.count()["person_load"] + lid_bins.count()["loc_load"])

    print(pid_bins.sum())
    print(lid_bins.sum())

    people.to_csv("people.csv", index=False)
    locations.to_csv("locations.csv", index=False)

if __name__ == "__main__":
    main()

