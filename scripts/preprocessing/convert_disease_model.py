#!/usr/bin/env python3
# Copyright 2021 The Loimos Project Developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: MIT

"""
Converts disease models from UVA JSON format to the textproto format used by
our simulation.
"""

import argparse
import json
import os


# Helpers to convert between the meta format to final textproto.
def days_to_time_def(days):
    return {"time_in_state": {"days": days}}


def convert_transition(transition_edge):
    if "discrete" in transition_edge:
        if len(transition_edge["discrete"]) == 1:
            # Fixed distribution.
            return {"fixed": days_to_time_def(transition_edge["discrete"][0]["value"])}
        else:
            bins = []
            for edge in transition_edge["discrete"]:
                bins.append(
                    {"tval": {"days": edge["value"]}, "with_prob": edge["probability"]}
                )
            return {"discrete": {"bins": bins}}
    elif "normal" in transition_edge:
        return {
            "normal": {
                "tmean": {"days": transition_edge["normal"]["mean"]},
                "tvariance": {"days": transition_edge["normal"]["standardDeviation"]},
            }
        }
    elif "fixed" in transition_edge:
        return {"fixed": {"time_in_state": {"days": transition_edge["fixed"]}}}
    else:
        raise Exception(f"Unknown type {transition_edge}")


def create_transition_set(paths):
    transitions = []
    for end_state, prob, transition_edge in paths:
        base_fields = {"next_state": end_state, "with_prob": prob}
        base_fields.update(convert_transition(transition_edge))
        transitions.append(base_fields)
    return transitions


def to_textproto(out_file, dictv, offset=0):
    coffset = "  " * offset
    for key, value in dictv.items():
        if type(value) == dict:
            out_file.write(coffset + f"{key}: {{\n")
            to_textproto(out_file, value, offset + 1)
            out_file.write(coffset + "}\n")
        elif type(value) == list:
            for x in value:
                out_file.write(coffset + f"{key}: {{\n")
                to_textproto(out_file, x, offset + 1)
                out_file.write(coffset + "}\n")
        elif type(value) == bool:
            formatted_value = "true" if value else "false"
            out_file.write(coffset + f"{key}: {formatted_value}\n")
        else:
            formatted_value = value if type(value) != str else f'"{value}"'
            out_file.write(coffset + f"{key}: {formatted_value}\n")


# Main converter.
def convert_file(in_path, out_path):
    with open(in_path) as f:
        disease_dict = json.loads(f.read())

    # Filter out only disease path for main age group.
    states = {}
    state_names_to_index = {}
    actual_added = 0
    for index, state in enumerate(disease_dict["states"]):
        # if not "_a" in state['id']:
        #    continue
        # Paths will track transitions and transmissions.
        state["paths"] = []
        state["exp_paths"] = []
        states[state["id"]] = state
        state_names_to_index[state["id"]] = actual_added
        actual_added += 1

    # Add transitions to states. (transitions are explicit and timed based).
    for tns in disease_dict["transitions"]:
        state_id = tns["entryState"]
        if state_id in states:
            states[state_id]["paths"].append(
                (
                    state_names_to_index[tns["exitState"]],
                    tns["probability"],
                    tns["dwellTime"],
                )
            )

    # Add transmissions from states. (which are triggered externally).
    transmissions = {}
    for trms in disease_dict["transmissions"]:
        entryState = trms["entryState"]
        # contactState = trms['contactState']
        exitState = trms["exitState"]
        # Only use the first instance of a transmission.
        if entryState not in transmissions:
            transmissions[entryState] = exitState

    for entryPath, exitPath in transmissions.items():
        if entryPath in states and exitPath in states:
            states[entryPath]["exp_paths"].append(exitPath)

    # Convert from their json to intermediary dictionary format.
    converted_states = []
    for state in states.values():
        # if "_a" not in state["id"]:
        #    continue

        disease_state = {}
        disease_state["state_label"] = state["id"]
        disease_state["infectivity"] = state["infectivity"]
        disease_state["symptomatic"] = "(symptomatic)" in state["ann:label"]
        disease_state["susceptibility"] = state["susceptibility"]
        if len(state["paths"]):
            disease_state["timed_transition"] = {
                "transitions": create_transition_set(state["paths"])
            }
        elif len(state["exp_paths"]):
            disease_state["exposure_transition"] = {
                "transitions": {
                    "next_state": state_names_to_index[state["exp_paths"][0]]
                }
            }
        converted_states.append({"disease_states": disease_state})

    prefix, _ = os.path.splitext(in_path)
    label = os.path.basename(prefix)
    out_path = out_path.format(prefix=prefix)

    # Output as textproto
    with open(out_path, "w") as out_file:
        to_textproto(out_file, {"label": label})
        to_textproto(
            out_file, {"transmissibility": int(disease_dict["transmissibility"])}
        )
        to_textproto(
            out_file,
            {
                "starting_states": {
                    "starting_state": state_names_to_index[
                        disease_dict["initialState"]
                    ],
                }
            },
        )
        for state in converted_states:
            to_textproto(out_file, state)
        print(state_names_to_index)


if __name__ == "__main__":
    # Required argument is the path to the JSON disease mode
    parser = argparse.ArgumentParser(
        description="Converts a UVA JSON format to a new format."
    )
    parser.add_argument("model_to_convert")
    parser.add_argument(
        "--out-file",
        "-o",
        default="{prefix}.textproto",
        help="Path to the output file. By default saves to the input "
        + "file path with the extension changed to '.textproto'",
    )
    args = parser.parse_args()
    convert_file(args.model_to_convert, args.out_file)
