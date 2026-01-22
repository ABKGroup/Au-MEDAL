import os
from shapely.geometry import Polygon
from shapely.geometry import box
from collections import defaultdict
from copy import deepcopy
import numpy as np
from itertools import combinations
import gdspy
from gdspy import Polygon as GdspyPolygon
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


def check_x_overlap(xs1, xs2):
    min1, max1 = min(xs1), max(xs1)
    min2, max2 = min(xs2), max(xs2)
    return max(min1, min2) <= min(max1, max2)  


def check_y_overlap(ys1, ys2):
    min1, max1 = min(ys1), max(ys1)
    min2, max2 = min(ys2), max(ys2)
    return max(min1, min2) <= min(max1, max2)  


def get_not_corners_from_two(poly1, poly2):
    p1_xs = [x for (x, y) in poly1.exterior.coords]
    p1_ys = [y for (x, y) in poly1.exterior.coords]
    p2_xs = [x for (x, y) in poly2.exterior.coords]
    p2_ys = [y for (x, y) in poly2.exterior.coords]  

    p1_lx = min(p1_xs)
    p1_ux = max(p1_xs)
    p1_ly = min(p1_ys)
    p1_uy = max(p1_ys)

    p2_lx = min(p2_xs)
    p2_ux = max(p2_xs)
    p2_ly = min(p2_ys)
    p2_uy = max(p2_ys)

    if p1_lx == p2_lx:
        if p1_ly == p2_ly:
            return [(p1_lx, min(p1_uy, p2_uy)), (min(p1_ux, p2_ux), p1_ly)]
        elif p1_uy == p2_uy:
            return [(p1_lx, max(p1_ly, p2_ly)), (min(p1_ux, p2_ux), p2_uy)]
        else:
            return [(p1_lx, min(p1_uy, p2_uy)), (p1_lx, max(p1_ly, p2_ly))]

    elif p1_ux == p2_ux:
        if p1_uy == p2_uy:
            return [(max(p1_lx, p2_lx), p1_uy), (p1_ux, max(p1_ly, p2_ly))]
        elif p1_ly == p2_ly:
            return [(max(p1_lx, p2_lx), p1_ly), (p1_ux, min(p1_uy, p2_uy))]
        else:
            return [(p1_ux, min(p1_uy, p2_uy)), (p1_ux, max(p1_ly, p2_ly))]

    elif p1_ly == p2_ly:
        return [(min(p1_ux, p2_ux), p1_ly), (max(p1_lx, p2_lx)), p1_ly]

    elif p1_uy == p2_uy:
        return [(min(p1_ux, p2_ux), p1_uy), (max(p1_lx, p2_lx)), p1_uy]

    else:
        return []

def get_not_corners(polygons):
    not_corners = []
    for poly1, poly2 in combinations(polygons, 2):
        if not poly1.intersects(poly2):
            continue

        not_corners_from_two = get_not_corners_from_two(poly1, poly2)
        for n in not_corners_from_two:
            not_corners.append(n)

    return not_corners


def get_target_pins_up(polygons):
    no_need = []
    needed = []
    verticals = []

    for p in polygons:
        p_xs = [x for (x, y) in p.exterior.coords]
        p_ys = [y for (x, y) in p.exterior.coords]

        p_w = max(p_xs) - min(p_xs)
        p_h = max(p_ys) - min(p_ys)

        if p_h > p_w:
            verticals.append(p)

    for p1, p2 in combinations(verticals, 2):
        p1_xs = [x for (x, y) in p1.exterior.coords]
        p1_ys = [y for (x, y) in p1.exterior.coords]
        p1_uy = max(p1_ys)
        p1_ly = min(p1_ys)
        p1_ux = max(p1_xs)
        p1_lx = min(p1_xs)

        p2_xs = [x for (x, y) in p2.exterior.coords]
        p2_ys = [y for (x, y) in p2.exterior.coords]
        p2_width = max(p2_xs) - min(p2_xs)
        p2_uy = max(p2_ys)
        p2_ly = min(p2_ys)
        p2_ux = max(p2_xs)
        p2_lx = min(p2_xs)

        if check_y_overlap(p1_ys, p2_ys):
            if min(abs(p2_ux - p1_lx), abs(p2_lx - p1_uy)) < 25:
                no_need.append(p1 if p1_uy < p2_uy else p2)

    for p in verticals:
        if p in no_need:
            continue
        needed.append(p)

    return needed


def get_target_pins_down(polygons):
    needed = []
    no_need = []
    verticals = []

    for p in polygons:
        p_xs = [x for (x, y) in p.exterior.coords]
        p_ys = [y for (x, y) in p.exterior.coords]

        p_w = max(p_xs) - min(p_xs)
        p_h = max(p_ys) - min(p_ys)

        if p_h > p_w:
            verticals.append(p)

    for p1, p2 in combinations(verticals, 2):
        p1_xs = [x for (x, y) in p1.exterior.coords]
        p1_ys = [y for (x, y) in p1.exterior.coords]
        p1_uy = max(p1_ys)
        p1_ly = min(p1_ys)
        p1_ux = max(p1_xs)
        p1_lx = min(p1_xs)

        p2_xs = [x for (x, y) in p2.exterior.coords]
        p2_ys = [y for (x, y) in p2.exterior.coords]
        p2_width = max(p2_xs) - min(p2_xs)
        p2_uy = max(p2_ys)
        p2_ly = min(p2_ys)
        p2_ux = max(p2_xs)
        p2_lx = min(p2_xs)

        if check_y_overlap(p1_ys, p2_ys):
            if min(abs(p2_ux - p1_lx), abs(p2_lx - p1_uy)) < 25:
                no_need.append(p2 if p1_uy < p2_uy else p1)    

    for p in verticals:
        if p in no_need:
            continue
        needed.append(p)

    return needed


def get_distance_up(target_poly, pin_and_obs, not_corner):
    xs_pin = [x for (x, y) in target_poly.exterior.coords]
    ys_pin = [y for (x, y) in target_poly.exterior.coords]
    lx_pin = min(xs_pin)
    ux_pin = max(xs_pin)
    ly_pin = min(ys_pin)
    uy_pin = max(ys_pin)

    target_obs = []
    distance_obs = [float('inf')]
    spacing_rule = 0.025

    for o_ps in pin_and_obs:
        if min(y for (x, y) in o_ps.exterior.coords) < ly_pin:
            continue
        else:
            target_obs.append(o_ps)

    for o_ps in target_obs:
        xs_obs = [x for (x, y) in o_ps.exterior.coords]
        ys_obs = [y for (x, y) in o_ps.exterior.coords]
        lx_obs = min(xs_obs)
        ux_obs = max(xs_obs)
        ly_obs = min(ys_obs)
        uy_obs = max(ys_obs)
        max_side = 0

        if check_x_overlap(xs_pin, xs_obs):
            if max(xs_obs) - min(xs_obs) < 0.037:
                spacing_rule = 0.031
            extra_length = ly_obs - uy_pin - spacing_rule

        else:
            if ly_obs < uy_pin:
                continue
            else:
                ori_height = ly_obs - uy_pin

                if (ux_obs, ly_obs) in not_corner and (lx_obs, ly_obs) in not_corner:
                    continue
                elif (ux_obs, ly_obs) in not_corner:
                    width_sq = (ux_pin - lx_obs) ** 2
                elif (lx_obs, ly_obs) in not_corner:
                    width_sq = (lx_pin - ux_obs) ** 2
                else:
                    width_sq = min((ux_pin - lx_obs) ** 2, (lx_pin - ux_obs) ** 2)
            if width_sq > 0.0004:
                max_side = float('inf')
                continue

            height = np.sqrt(max(0, 0.0004 - width_sq))               
            extra_length = ori_height - height - 0.001

        distance_obs.append(extra_length)

    return min(distance_obs) 

def get_distance_down(target_poly, pin_and_obs, not_corner):
    xs_pin = [x for (x, y) in target_poly.exterior.coords]
    ys_pin = [y for (x, y) in target_poly.exterior.coords]
    lx_pin = min(xs_pin)
    ux_pin = max(xs_pin)
    ly_pin = min(ys_pin)
    uy_pin = max(ys_pin)
    spacing_rule = 0.025

    target_obs = []
    distance_obs = [float('inf')]

    for o_ps in pin_and_obs:
        if max(y for (x, y) in o_ps.exterior.coords) > ly_pin:
            continue
        else:
            target_obs.append(o_ps)

    for o_ps in target_obs:
        xs_obs = [x for (x, y) in o_ps.exterior.coords]
        ys_obs = [y for (x, y) in o_ps.exterior.coords]
        lx_obs = min(xs_obs)
        ux_obs = max(xs_obs)
        ly_obs = min(ys_obs)
        uy_obs = max(ys_obs)
        max_side = 0

        if check_x_overlap(xs_pin, xs_obs):
            if max(xs_obs) - min(xs_obs) < 0.037:
                spacing_rule = 0.031            
            extra_length = ly_pin - uy_obs - spacing_rule

        else:
            if uy_obs > ly_pin:
                continue
            else:
                ori_height = ly_pin - uy_obs

                if (ux_obs, uy_obs) in not_corner and (lx_obs, uy_obs) in not_corner:
                    continue
                elif (ux_obs, uy_obs) in not_corner:
                    width_sq = (ux_pin - lx_obs) ** 2
                elif (lx_obs, uy_obs) in not_corner:
                    width_sq = (lx_pin - ux_obs) ** 2
                else:
                    width_sq = min((ux_pin - lx_obs) ** 2, (lx_pin - ux_obs) ** 2)

            if width_sq > 0.0004:
                max_side = float('inf')
                continue

            height = np.sqrt(max(0, 0.0004 - width_sq))               
            extra_length = ori_height - height - 0.001

        distance_obs.append(extra_length)

    return min(distance_obs) 


def get_distance(polygons, layer, not_corner):
    list_pin = polygons["PIN"][layer]
    list_obs = polygons["OBS"][layer]
    all_obs = deepcopy(list_obs)
    for vs in list_pin.values():
        for v in vs:
            all_obs.append(v)
    not_corner = get_not_corners(all_obs)

    extended = {}
    extended_poly = []
    original_poly = []

    pin_down = deepcopy(list_pin)

    for lists_pin_name, lists_pin in list_pin.items():
        if lists_pin_name[0] == "V":
            for l_pin in lists_pin:
                extended_poly.append(l_pin)
            continue
        for l_pin in lists_pin:
            extended[l_pin] = {-1: (0, 0), 1: (0, 0)}

    for pin_name, pin_vals in list_pin.items():
        if pin_name[0] == "V":
            continue
        pin_and_obs = deepcopy(list_obs)

        for other_pins in list_pin.values():
            if pin_vals == other_pins:
                continue
            else:
                for o_ps in other_pins:
                    pin_and_obs.append(o_ps)

        target_pins_down = get_target_pins_down(pin_vals)
        for target_pin_down in target_pins_down:
            xs  = [x for (x, y) in target_pin_down.exterior.coords]
            ys  = [y for (x, y) in target_pin_down.exterior.coords]
            ux = max(xs)
            lx = min(xs)
            uy = max(ys)
            ly = min(ys)

            dist_down = get_distance_down(target_pin_down, pin_and_obs, not_corner)
            down = round(dist_down, 6)

            if down > 0:
                new_pin_down = box(lx, ly - down, ux, uy)
                len_0 = len(pin_down[pin_name])
                pin_down[pin_name].append(new_pin_down)
                len_1 = len(pin_down[pin_name])
                pin_down[pin_name].remove(target_pin_down)
                len_2 = len(pin_down[pin_name])
    
    pin_updown = deepcopy(pin_down)

    for pin_name, pin_vals in pin_down.items():
        if pin_name[0] == "V":
            continue
        pin_and_obs = deepcopy(list_obs)

        for other_pins in pin_down.values():
            if pin_vals == other_pins:
                continue
            else:
                for o_ps in other_pins:
                    pin_and_obs.append(o_ps)

        target_pins_up = get_target_pins_up(pin_vals)
        for target_pin_up in target_pins_up:
            xs = [x for (x, y) in target_pin_up.exterior.coords]
            ys = [y for (x, y) in target_pin_up.exterior.coords]
            ux = max(xs)
            lx = min(xs)
            uy = max(ys)
            ly = min(ys)

            dist_up = get_distance_up(target_pin_up, pin_and_obs, not_corner)
            up = round(dist_up, 6)

            if up > 0:
                new_pin_up = box(lx, ly, ux, uy + up)
                len_0 = len(pin_updown[pin_name])
                pin_updown[pin_name].append(new_pin_up)
                len_1 = len(pin_updown[pin_name])
                pin_updown[pin_name].remove(target_pin_up)
                len_2 = len(pin_updown[pin_name])

    for pin in pin_updown.values():
        for p in pin:
            extended_poly.append(p)
    for obs in list_obs:
        extended_poly.append(obs)

    return extended_poly

def replace_layer(gds_path, output_path, extended, layer=19, datatype=0):
    lib = gdspy.GdsLibrary()
    lib.read_gds(gds_path)
    topcell = lib.top_level()[0]

    topcell.polygons = [
        p for p in topcell.polygons
        if not (p.layers[0] == layer and p.datatypes[0] == datatype)
    ]

    if isinstance(extended, Polygon):
        extended = [extended]

    valid_polygons = [p for p in extended if isinstance(p, Polygon) and not p.is_empty]

    for poly in valid_polygons:
        coords = list(poly.exterior.coords)
        gdspy_poly = GdspyPolygon(coords, layer=layer, datatype=datatype)
        topcell.add(gdspy_poly)

    output_os = output_path
    path_os = output_os.split("/")
    os.makedirs(os.path.join(path_os[0], path_os[1], path_os[2]), exist_ok=True)

    lib.unit = 1.0e-6
    lib.precision = 2.5e-10
    lib_set_props = True

    lib.write_gds(output_path)
    print(f"Extended GDS written in {output_path}")


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--gds_path', type=str, default="../results", help="Set GDS directory")
    parser.add_argument('--lef_path', type=str, default="../asap7_ref.lef", help="Set LEF file path")

    param = parser.parse_args()
    gds_path = param.gds_path
    lef_path = param.lef_path

    des = gds_path + "_extended"
    lef_data = parse_lef(lef_path)
    whichfile, _ = lef_path.split('.l')
    results = {}
    for lef_key, lef_val in lef_data.items():
        # if lef_key == "DFFASRHQNx1_ASAP7_75t_R":
        #     continue
        print(lef_key)
        pin_obs = []
        m2_pin = lef_val["PIN"]["M2"]
        m2_obs = lef_val["OBS"]["M2"]

        for pins in lef_val["PIN"]["M1"].values():
            for p in pins:
                pin_obs.append(p)
        for obs in lef_val["OBS"]["M1"]:
            pin_obs.append(obs)
        not_corners = get_not_corners(pin_obs)

        extended = get_distance(lef_val, 'M1', not_corners)

        try:
            replace_layer(f"{gds_path}/{lef_key}/{lef_key}.gds", f"{des}/{lef_key}/{lef_key}.gds", extended)
        except FileNotFoundError:
            print(f"{gds_path}/{lef_key}/{lef_key}.gds")
            print("Not Intersection")
