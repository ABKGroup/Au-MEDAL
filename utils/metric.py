import os
from shapely.geometry import box
from collections import defaultdict
from copy import deepcopy
import numpy as np
from itertools import combinations
import csv
import gdspy
import argparse

def parse_lef(database):
    cell_name = None
    layer = None
    type_metal = None
    metal_name = None
    rectangles = False
    current_rectangles = []

    lef_file = open(database, "r")
    lines = lef_file.readlines()

    lef_data = {}

    for line in lines:
        line = line.replace('\n', '')
        line = line.lstrip()
        split = line.split(" ")

        if split[0] == "MACRO":
            cell_name = split[1]
            lef_data[cell_name] = {
                "PIN" : {"M1" : defaultdict(list), "M2" : defaultdict(list), "V1" : defaultdict(list)},
                "OBS" : {"M1" : [], "M2" : [], "V1" : []}
            }

        elif cell_name == None:
            continue

        elif split[0] == "CLASS":
            continue

        elif split[0] == "ORIGIN":
            continue

        elif split[0] == "FOREIGN":
            continue

        elif split[0] == "SIZE":
            continue

        elif split[0] == "SYMMETRY":
            continue

        elif split[0] == "SITE":
            continue

        if split[0] == "PIN":
            type_metal = "PIN"
            metal_name = split[1]

        elif split[0] == "DIRECTION":
            continue

        elif split[0] == "USE":
            continue

        elif split[0] == "PORT":
            continue

        elif split[0] == "LAYER":
            layer = split[1]
        
        elif split[0] == "OBS":
            type_metal = "OBS"
            metal_name = "OBS"

        elif split[0] == "LAYER":
            layer = split[1]

        elif split[0] == "RECT":
            lx, ly, ux, uy = float(split[1]), float(split[2]), float(split[3]), float(split[4])
            if type_metal == "PIN":
                lef_data[cell_name]["PIN"][layer][metal_name].append(box(lx, ly, ux, uy))
            elif type_metal == "OBS":
                lef_data[cell_name]["OBS"][layer].append(box(lx, ly, ux, uy))

    return lef_data


def calculate_metric(m1_pin, m1_obs, m2_pin, m2_obs, save_dir='output.csv'):
    metric_1 = 0 # Pin Length
    metric_2 = 0 # OBS Length
    metric_3 = 0 # M2 Length

    m1_pin_inter = 0
    m1_blk_inter = 0
    pin_ext_inter = 0
    num_via_inter = 0

    m2_and_spacing = []
    m2_pin_spacing = []

    for pin1, pin2 in combinations(m1_pin, 2):
        m1_pin_inter += pin1.intersection(pin2).area

    for blk1, blk2 in combinations(m1_obs, 2):
        m1_blk_inter += blk1.intersection(blk2).area

    for m2 in m2_pin:
        metric_3 += m2.area

    for m2 in m2_obs:
        metric_3 += m2.area
        
    for pin in m1_pin:
        metric_1 += pin.area

    metric_1 -= m1_pin_inter
    metric_1 = metric_1 / 0.018

    for blk in m1_obs:
        metric_2 += blk.area

    metric_2 -= m1_blk_inter
    metric_2 = metric_2 / 0.018
    
    metric_3 = metric_3 / 0.018

    return round(metric_1, 6), round(metric_2, 6), round(metric_3, 6)


def write_metrics_to_csv(metrics: dict, filename: str = "cell_metrics.csv"):
    with open(filename, mode="w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["Cell Name", "Pin Length", "Blockage Length", "M2 Length", "Num Vias"])

        for cell_name, values in metrics.items():
            if len(values) != 4:
                raise ValueError(f"Cell {cell_name} metric not 4")
            writer.writerow([cell_name] + values)


def count_shapes_per_cell(gds_path: str, target_layer=18):
    lib = gdspy.GdsLibrary()
    lib.read_gds(gds_path)

    count = {}
    via1s = {}

    for cell in lib.cells.values():
        boxes = []
        count[cell.name] = 0
        for (layer, datatype), polygons in cell.get_polygons(by_spec=True).items():
            if layer != target_layer:
                continue
            for polygon in polygons:
                y_coords = polygon[:, 1]
                if all(0.02 < y < 0.25 for y in y_coords):
                    count[cell.name] += 1
                    minx, miny = polygon.min(axis=0)
                    maxx, maxy = polygon.max(axis=0)
                    boxes.append(box(minx, miny, maxx, maxy))
        via1s[cell.name] = boxes

    return count, via1s


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--gds_path', type=str, default="../results.gds", help="Set GDS directory")
    parser.add_argument('--lef_path', type=str, default="../results.lef", help="Set LEF file path")

    param = parser.parse_args()
    gds_path = param.gds_path
    lef_path = param.lef_path

    lef_data = parse_lef(lef_path)
    whichfile, _ = lef_path.split('.l')
    num_v0, _ = count_shapes_per_cell(gds_path)
    num_v1, via1s = count_shapes_per_cell(gds_path, 21)
    results = {}

    for lef_key, lef_val in lef_data.items():
        print(lef_key)
        m1_pin_dict = lef_val["PIN"]["M1"]
        m2_pin_dict = lef_val["PIN"]["M2"]

        m1_pin = []
        m2_pin = []

        for m1_key, m1_list in m1_pin_dict.items():
            if m1_key[0] == "V":
                continue
            for m1 in m1_list:
                m1_pin.append(m1)

        for m2_list in m2_pin_dict.values():
            for m2 in m2_list:
                m2_pin.append(m2)

        m1_obs = lef_val["OBS"]["M1"]
        m2_obs = lef_val["OBS"]["M2"]
        metric_1, metric_2, metric_3 = calculate_metric(m1_pin, m1_obs, m2_pin, m2_obs)
        metric_4 = num_v0[lef_key] + num_v1[lef_key]

        results[lef_key] = [metric_1, metric_2, metric_3, metric_4]

    write_metrics_to_csv(results, whichfile + ".csv")