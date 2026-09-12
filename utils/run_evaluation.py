"""Unified CLI for cell layout evaluation (Calibre LVS/DRC/PEX, area, placement, GDS merge),
LEF/GDS metric extraction, and M1 pin extension via the run, metric and pin_extension subcommands."""

import os
import sys
import argparse
import csv
import datetime as dt
from multiprocessing import Pool
from collections import defaultdict
from copy import deepcopy
from itertools import combinations
import numpy as np
import gdspy
from gdspy import Polygon as GdspyPolygon
from shapely.geometry import Polygon
from shapely.geometry import box

EXCLUDED_DRC_RULES = {"ACTIVE.LUP.1"}

def get_placement(input_net, output_placement):
    input_file = open(input_net, 'r')
    output_file = open(output_placement, 'w+')
    lines = input_file.readlines()

    fets = []
    nmos = []
    pmos = []

    precision = 4

    for line in lines:
        if line[0] == '*':
            continue
        elif line[0] == ".":
            continue
        elif line[0] == "\n":
            continue
        name, s, g, d, b, mos, l, w, nfin, x, y, _d = line.split(" ")
        _, l = l.split("=")
        _, w = w.split("=")
        _, nfin = nfin.split("=")
        _, x = x.split("=")
        _, y = y.split("=")
        _, _d = _d.split("=")
        _d, _ = _d.split("\n")

        nfin = int(nfin)
        x = int(x)
        y = int(y)
        _d = int(_d)

        if x % 4 != 0:
            precision = 1

        fets.append([name, s, g, d, b, mos, l, w, nfin, x, y, _d])

    for fet in fets:
        fet[9] = fet[9] / precision
        fet[10] = fet[10] / precision
        if fet[5] == 'nmos_rvt':
            nmos.append(fet)
        elif fet[5] == 'pmos_rvt':
            pmos.append(fet)
        else:
            RuntimeError("Not PMOS nor NMOS")

    for i in range(len(pmos)):
        for j in range(i + 1, len(pmos)):
            if pmos[i][1] == pmos[j][1] and pmos[i][2] == pmos[j][2] and pmos[i][3] == pmos[j][3]:
                pmos[i][0] = pmos[j][0]
            elif pmos[i][1] == pmos[j][3] and pmos[i][2] == pmos[j][2] and pmos[i][3] == pmos[j][1]:
                pmos[i][0] = pmos[j][0]
    for i in range(len(nmos)):
        for j in range(i + 1, len(nmos)):
            if nmos[i][1] == nmos[j][1] and nmos[i][2] == nmos[j][2] and nmos[i][3] == nmos[j][3]:
                nmos[i][0] = nmos[j][0]
            elif nmos[i][1] == nmos[j][3] and nmos[i][2] == nmos[j][2] and nmos[i][3] == nmos[j][1]:
                nmos[i][0] = nmos[j][0]

    pmos = sorted(pmos, key=lambda x: x[9])
    nmos = sorted(nmos, key=lambda x: x[9])

    nmos_x = 71
    pmos_x = 71

    pmos_with_dummy = []
    nmos_with_dummy = []

    for p in pmos:
        while pmos_x < p[9]:
            pmos_with_dummy.append(None)
            pmos_x += 54
        pmos_with_dummy.append(p)
        pmos_x += 54

    for n in nmos:
        while nmos_x < n[9]:
            nmos_with_dummy.append(None)
            nmos_x += 54
        nmos_with_dummy.append(n)
        nmos_x += 54

    len_cell = max(len(pmos_with_dummy), len(nmos_with_dummy))

    while len(pmos_with_dummy) < len_cell:
        pmos_with_dummy.append(None)
    while len(nmos_with_dummy) < len_cell:
        nmos_with_dummy.append(None)

    assert(len(pmos_with_dummy) == len(nmos_with_dummy))

    output_file.write("-------- Solution 1 --------\n")

    for i in range(len(pmos_with_dummy)):
        n = nmos_with_dummy[i]
        p = pmos_with_dummy[i]

        line = ""
        if n:
            line += "NMOS : {}({}) [{} {} {}]".format(n[0], n[8], n[3], n[2], n[1])
        else:
            line += "NMOS : dummy(0) [dummy dummy dummy]"

        line += ", "

        if p:
            line += "PMOS : {}({}) [{} {} {}]".format(p[0], p[8], p[3], p[2], p[1])
        else:
            line += "PMOS : dummy(0) [dummy dummy dummy]"

        print("[Column {}]".format(i+1))
        print(line)
        line += "\n"

        output_file.write("[Column {}]\n".format(i+1))
        output_file.write(line)

    input_file.close()
    output_file.close()

    return


def get_gds_area(gds_file, cell_name):
    try:
        lib = gdspy.GdsLibrary()
        lib.read_gds(gds_file)
        cell = lib.cells.get(cell_name)

        if cell is None:
            print(f"Cell '{cell_name}' not found in GDSII file '{gds_file}'")
            return None, None

        polygons = cell.get_polygons(by_spec=True)[(100, 0)]

        if not polygons:
            print(f"No polygons found on layer 100 in cell {cell.name}")
            return None, None

        product_points = [round(p[0] * p[1], 8) for p in polygons[0]]

        return max(product_points)

    except FileNotFoundError:
        print(f"Error: GDSII file not found: {gds_file}")
        return None, None
    except Exception as e:
        print(f"An error occurred: {e}")
        return None, None


def parse_drc_list(drc_rule_path):
    drc_rule_file = open(drc_rule_path)
    lines = drc_rule_file.readlines()
    start = False
    drc_list = []

    for line in lines:
        if "Rule Checks" in line:
            start = True

        if start:
            if '{' in line:
                drc_rule, _ = line.split('{')

                if drc_rule == '':
                    drc_rule = line_pre.replace('\n', '')

                drc_list.append(drc_rule)

        line_pre = line

    return drc_list


def parse_drc(cell_name, database, drc_list):
    drc_file = open(database + "/" + cell_name + "/" + "drc.results")
    lines = drc_file.readlines()
    current_rule = None

    parsed_file = open(database + "/" + cell_name + "/" + "parsed_drc.results", "w+")

    parsed = {i: [] for i in drc_list}
    drc_error = {}

    for line in lines:
        line = line.replace('\n', '')

        if line in drc_list:
            current_rule = line
            continue

        if current_rule is None:
            continue
        else:
            parsed[current_rule].append(line.replace('\n', ''))

    for key, values in parsed.items():
        if len(values) == 0:
            continue
        if key in EXCLUDED_DRC_RULES:
            continue
        if values[0][0:3] == '0 0':
            continue
        else:
            drc_error[key] = values

    for key, values in drc_error.items():
        parsed_file.write(key)
        parsed_file.write('\n')
        for line in values:
            parsed_file.write(line)
            parsed_file.write('\n')
        parsed_file.write('\n')

    parsed_file.close()

    return parsed, drc_error


def run_lvs(database, cell_name):
    run_lvs = "calibre -spice {}/{}/{}.net -lvs -hier ".format(database, cell_name, cell_name)
    run_lvs_script = run_lvs + database + "/" + cell_name + "/run_lvs.rul"

    os.system("echo %s" % (run_lvs_script))
    os.system(run_lvs_script)


def run_drc(database, cell_name):
    run_drc = "calibre -drc -hier "
    run_drc_script = run_drc + database + "/" + cell_name + "/run_drc.rul"

    os.system("echo %s" % (run_drc_script))
    os.system(run_drc_script)


def run_pex(database, cell_name):
    run_lvs = "calibre -lvs -hier "
    run_xact = "calibre -xact -3d "
    run_lvs_script = run_lvs + database + "/" + cell_name + "/run_xact.rul"
    run_xact_script = run_xact + database + "/" + cell_name + "/run_xact.rul"

    os.system("echo %s" % (run_lvs_script))
    os.system(run_lvs_script)
    os.system("echo %s" % (run_xact_script))
    os.system(run_xact_script)


def merge_spice(cell, indir, outdir):
    rf = open(indir, "r")
    wf = open(outdir, "w+")

    lines = rf.readlines()

    for line in lines:
        if ".include" in line:
            sp = line.split(" ")
            file_name = sp[1].replace("\"", "")
            file_name = file_name.replace("\n", "")

            includes = open(file_name, "r")

            for l in includes.readlines():
                if cell.upper() in l:
                    l = l.replace(cell.upper(), cell)
                wf.write(l)

            wf.write("\n")
            includes.close()

        elif ".SUBCKT" in line and "_ASAP7" in line:
            sp = line.split(" ")
            cell_name = cell
            sp[1] = cell_name
            j = (" ").join(sp)
            wf.write(j)

        else:
            wf.write(line)

    rf.close()
    wf.close()

def sort_spice(cell_name):
    if "HAX" in cell_name.upper():
        return "ADDER"
    elif "FAX" in cell_name.upper():
        return "ADDER"
    elif "AO" in cell_name.upper():
        return "AO"
    elif "A2O1" in cell_name.upper():
        return "AO"
    elif "BUF" in cell_name.upper():
        return "INVBUF"
    elif "INV" in cell_name.upper():
        return "INVBUF"
    elif "HB" in cell_name.upper():
        return "INVBUF"
    elif "OA" in cell_name.upper():
        return "OA"
    elif "O2A1" in cell_name.upper():
        return "OA"
    elif "DECAP" in cell_name.upper():
        return "PHY"
    elif "FILLER" in cell_name.upper():
        return "PHY"
    elif "TAPCELL" in cell_name.upper():
        return "PHY"
    elif "TIE" in cell_name.upper():
        return "PHY"
    elif "DFF" in cell_name.upper():
        return "SEQ"
    elif "DHL" in cell_name.upper():
        return "SEQ"
    elif "DLL" in cell_name.upper():
        return "SEQ"
    elif "SDF" in cell_name.upper():
        return "SEQ"
    elif "ICG" in cell_name.upper():
        return "SEQ"
    else:
        return "SIMPLE"


def merge_gds(database):
    database_name = database.replace("../", "")
    output_gds_file = f"../{database_name}.gds"

    main_lib = gdspy.GdsLibrary()

    for cell_name in os.listdir(database):
        if cell_name[0] == '.':
            continue
        read_lib = gdspy.GdsLibrary().read_gds(f"{database}/{cell_name}/{cell_name}.gds")

        if hasattr(read_lib, 'unit') and read_lib.unit is not None:
            main_lib.unit = read_lib.unit
            main_lib.precision = read_lib.precision
            main_lib_set_props = True
        elif not main_lib_set_props:
            main_lib.unit = 1.0e-6
            main_lib.precision = 2.5e-10
            main_lib_set_props = True

        for cell in read_lib.cells.values():
            main_lib.add(cell)

    main_lib.write_gds(output_gds_file)


def main_evaluation():
    parser = argparse.ArgumentParser()
    parser.add_argument('--num_process', type=int, default=4, help="Number of Calibre Processes")
    parser.add_argument('--save_dir', type=str, default="../results", help="Set design result directory")
    parser.add_argument('--spice', type=str, default="asap7sc7p5t.sp", help="SPICE directory")

    parser.add_argument('--gen_script', type=bool, default=False, help="Generates Script File")
    parser.add_argument('--lvs', type=bool, default=False, help="Run LVS")
    parser.add_argument('--drc', type=bool, default=False, help="Run DRC")
    parser.add_argument('--pex', type=bool, default=False, help="Run PEX")

    parser.add_argument('--get_gds_area', type=bool, default=False, help="Generates area info file")
    parser.add_argument('--get_placement', type=bool, default=False, help="Generates Placement Solutions")
    parser.add_argument('--lib', type=bool, default=False, help="Run LIBGEN")
    parser.add_argument('--merge_gds', type=bool, default=False, help="Merges GDS File")
    parser.add_argument('--force', type=bool, default=False, help="Runs every cell without exception")

    param = parser.parse_args(sys.argv[2:])

    database = param.save_dir
    _, database_name = database.split("/")

    cells = os.listdir(database)
    spice = param.spice

    if param.get_gds_area:
        todo_area = []

        for cell in cells:
            if cell[0] == '.':
                continue
            if "area.txt" in os.listdir(database + "/" + cell) and not param.force:
                continue
            else:
                todo_area.append(cell)

        for todo in todo_area:
            area_path = database + "/" + todo + "/" + "area.txt"
            gds_path = database + "/" + todo + "/" + todo + ".gds"
            wf_area = open(area_path, 'w+')
            cell_area = get_gds_area(gds_path, todo)
            print(cell_area)
            wf_area.write(str(cell_area))
            wf_area.close()

    if param.lvs:
        todo_lvs = []
        lvs_summary = open(param.save_dir + "_lvs_summary.txt", "w+")

        for cell in cells:
            if cell[0] == '.':
                continue
            if cell.endswith(".gds"):
                continue
            if f"{cell}.net" in os.listdir(database + "/" + cell) and not param.force:
                continue
            else:
                todo_lvs.append((database, cell))

        with Pool(processes=param.num_process) as pool:
            pool.starmap(run_lvs, todo_lvs)

        for cell in cells:
            if cell[0] == '.':
                continue
            try:
                lvs_file = open(f"{param.save_dir}/{cell}/lvs.report", 'r')
                is_correct = "CORRECT"

                for line in lvs_file.readlines():
                    if "INCORRECT" in line:
                        is_correct = "INCORRECT"
                        break

                lvs_summary.write(f"{cell} {is_correct}\n")

            except FileNotFoundError:
                print("LVS report not found")
            except NotADirectoryError:
                print("Not a Directory")

        lvs_summary.close()


    if param.drc:
        drc_list = parse_drc_list("ruledir/drcRules_calibre_asap7.rul")
        cells = os.listdir(database)
        todo_drc = []

        drc_results = open(param.save_dir + "_drc_results.txt", "w+")
        drc_summary = open(param.save_dir + "_drc_summary.txt", "w+")

        type_drc = {}

        for cell in cells:
            if cell[0] == '.':
                continue
            if "drc.results" in os.listdir(database + "/" + cell) and not param.force:
                continue
            else:
                todo_drc.append((database, cell))

        with Pool(processes=param.num_process) as pool:
            pool.starmap(run_drc, todo_drc)

        for cell in cells:
            if cell[0] == '.':
                continue
            try:
                parsed, drc_error = parse_drc(cell, database, drc_list)

                drc_results.write(str(cell))
                drc_results.write('\n\n')

                for key, val in drc_error.items():
                    if key not in type_drc.keys():
                        type_drc[key] = []
                    type_drc[key].append(str(cell))
                    drc_results.write(key)
                    drc_results.write('\n')
                    for v in val:
                        drc_results.write(v)
                        drc_results.write('\n')
                    drc_results.write('\n')

                if len(drc_error) == 0:
                    drc_results.write("------------------------------DRC CLEAN------------------------------\n")
                drc_results.write("----------------------------------------------------------------------\n\n")
            except:
                print(cell)
                print("DRC File Not Found")

        for key, val in type_drc.items():
            drc_summary.write(key)
            drc_summary.write('\n')

            str_val = str(val)
            str_val = str_val.replace("[", "")
            str_val = str_val.replace("]", "")
            str_val = str_val.replace("'", "")

            drc_summary.write(str_val)
            drc_summary.write('\n\n')

        drc_results.close()
        drc_summary.close()

    if param.pex:
        todo_pex = []

        for cell in cells:
            if cell[0] == '.':
                continue
            if cell + ".sp.pex" in os.listdir(database + "/" + cell) and not param.force:
                continue
            else:
                todo_pex.append((database, cell))

        with Pool(processes=param.num_process) as pool:
            pool.starmap(run_pex, todo_pex)

        for cell in cells:
            if cell[0] == '.':
                continue
            print(cell)
            cell_dir = database + "/" + cell
            cell_type = sort_spice(cell)
            print(cell_type)
            os.makedirs("../libgen/ref_" + database_name, exist_ok=True)
            os.makedirs("../libgen/ref_" + database_name + "/ADDER", exist_ok=True)
            os.makedirs("../libgen/ref_" + database_name + "/AO", exist_ok=True)
            os.makedirs("../libgen/ref_" + database_name + "/INVBUF", exist_ok=True)
            os.makedirs("../libgen/ref_" + database_name + "/OA", exist_ok=True)
            os.makedirs("../libgen/ref_" + database_name + "/PHY", exist_ok=True)
            os.makedirs("../libgen/ref_" + database_name + "/SEQ", exist_ok=True)
            os.makedirs("../libgen/ref_" + database_name + "/SIMPLE", exist_ok=True)
            os.makedirs("../libgen/ref_" + database_name + "/SIMPLE", exist_ok=True)

            for f in os.listdir(cell_dir):
                if f[-3:] == ".sp":
                    merge_spice(cell, cell_dir + "/" + f, "../libgen/ref_{}/{}/{}".format(database, cell_type, f))


    if param.get_placement:
        todo_placement = []

        for cell in cells:
            if cell[0] == '.':
                continue
            if cell.endswith(".gds"):
                continue
            if cell + ".txt" in os.listdir(database + "/" + cell) and not param.force:
                continue
            else:
                todo_placement.append(cell)

        for cell in todo_placement:
            input_net = f"{database}/{cell}/{cell}.net"
            output_placement = f"{database}/{cell}/{cell}.txt"

            get_placement(input_net, output_placement)


    if param.lib:
        os.chdir("libgen")
        os.system(f"python run_libgen.py {database_name}")

    if param.merge_gds:
        merge_gds(param.save_dir)


LENGTH_NORMALIZATION = 0.018
ROW_WINDOW_MIN = 0.02
ROW_WINDOW_MAX = 0.25


def _init_cell_entry():
    return {
        "PIN": {"M1": defaultdict(list), "M2": defaultdict(list), "V1": defaultdict(list)},
        "OBS": {"M1": [], "M2": [], "V1": []},
    }


def parse_lef_metric(database):
    cell_name = None
    layer = None
    object_type = None
    pin_name = None
    lef_data = {}

    with open(database, "r", encoding="utf-8") as lef_file:
        for raw_line in lef_file:
            tokens = raw_line.strip().split()
            if not tokens:
                continue

            keyword = tokens[0]
            if keyword == "MACRO":
                cell_name = tokens[1]
                lef_data[cell_name] = _init_cell_entry()
                continue

            if cell_name is None:
                continue

            if keyword in {"CLASS", "ORIGIN", "FOREIGN", "SIZE", "SYMMETRY", "SITE", "DIRECTION", "USE", "PORT"}:
                continue

            if keyword == "PIN":
                object_type = "PIN"
                pin_name = tokens[1]
                continue

            if keyword == "OBS":
                object_type = "OBS"
                pin_name = "OBS"
                continue

            if keyword == "LAYER":
                layer = tokens[1]
                continue

            if keyword != "RECT":
                continue

            lx, ly, ux, uy = map(float, tokens[1:5])
            rectangle = box(lx, ly, ux, uy)
            if object_type == "PIN":
                lef_data[cell_name]["PIN"][layer][pin_name].append(rectangle)
            elif object_type == "OBS":
                lef_data[cell_name]["OBS"][layer].append(rectangle)

    return lef_data


def calculate_metric(m1_pin, m1_obs, m2_pin, m2_obs, save_dir="output.csv"):
    del save_dir

    pin_overlap = sum(pin1.intersection(pin2).area for pin1, pin2 in combinations(m1_pin, 2))
    obs_overlap = sum(obs1.intersection(obs2).area for obs1, obs2 in combinations(m1_obs, 2))

    m1_pin_area = sum(pin.area for pin in m1_pin) - pin_overlap
    m1_obs_area = sum(obs.area for obs in m1_obs) - obs_overlap
    m2_area = sum(m2.area for m2 in m2_pin) + sum(m2.area for m2 in m2_obs)

    metric_1 = round(m1_pin_area / LENGTH_NORMALIZATION, 6)
    metric_2 = round(m1_obs_area / LENGTH_NORMALIZATION, 6)
    metric_3 = round(m2_area / LENGTH_NORMALIZATION, 6)
    return metric_1, metric_2, metric_3


def write_metrics_to_csv(metrics, filename="cell_metrics.csv"):
    with open(filename, mode="w", newline="", encoding="utf-8") as output_file:
        writer = csv.writer(output_file)
        writer.writerow(["Cell Name", "dbPin Length", "Blockage Length", "M2 Length", "Num Vias"])

        for cell_name, values in metrics.items():
            if len(values) != 4:
                raise ValueError(f"Cell {cell_name} metric not 4")
            writer.writerow([cell_name] + values)


def count_shapes_per_cell(gds_path, target_layer=18):
    gds_library = gdspy.GdsLibrary()
    gds_library.read_gds(gds_path)

    counts = {}
    boxes_per_cell = {}

    for cell in gds_library.cells.values():
        boxes = []
        counts[cell.name] = 0
        for (layer, _datatype), polygons in cell.get_polygons(by_spec=True).items():
            if layer != target_layer:
                continue
            for polygon in polygons:
                y_coords = polygon[:, 1]
                if all(ROW_WINDOW_MIN < y < ROW_WINDOW_MAX for y in y_coords):
                    counts[cell.name] += 1
                    min_x, min_y = polygon.min(axis=0)
                    max_x, max_y = polygon.max(axis=0)
                    boxes.append(box(min_x, min_y, max_x, max_y))
        boxes_per_cell[cell.name] = boxes

    return counts, boxes_per_cell


def main_metric():
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument("--gds_path", type=str, default="../results.gds", help="Set GDS directory")
    arg_parser.add_argument("--lef_path", type=str, default="../results.lef", help="Set LEF file path")
    args = arg_parser.parse_args(sys.argv[2:])

    lef_data = parse_lef_metric(args.lef_path)
    output_prefix = args.lef_path.rsplit(".l", 1)[0]
    num_v0, _ = count_shapes_per_cell(args.gds_path)
    num_v1, _ = count_shapes_per_cell(args.gds_path, 21)

    results = {}
    for cell_name, cell_data in lef_data.items():
        print(cell_name)
        m1_pin_shapes = []
        for pin_name, pin_shapes in cell_data["PIN"]["M1"].items():
            if pin_name.startswith("V"):
                continue
            m1_pin_shapes.extend(pin_shapes)

        m2_pin_shapes = []
        for pin_shapes in cell_data["PIN"]["M2"].values():
            m2_pin_shapes.extend(pin_shapes)

        m1_obs_shapes = cell_data["OBS"]["M1"]
        m2_obs_shapes = cell_data["OBS"]["M2"]
        metric_1, metric_2, metric_3 = calculate_metric(
            m1_pin_shapes,
            m1_obs_shapes,
            m2_pin_shapes,
            m2_obs_shapes,
        )
        metric_4 = num_v0[cell_name] + num_v1[cell_name]
        results[cell_name] = [metric_1, metric_2, metric_3, metric_4]

    write_metrics_to_csv(results, output_prefix + ".csv")


def parse_lef_pinext(database):
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


def main_pin_extension():
    parser = argparse.ArgumentParser()
    parser.add_argument('--gds_path', type=str, default="../results", help="Set GDS directory")
    parser.add_argument('--lef_path', type=str, default="../asap7_ref.lef", help="Set LEF file path")

    param = parser.parse_args(sys.argv[2:])
    gds_path = param.gds_path
    lef_path = param.lef_path

    des = gds_path + "_extended"
    lef_data = parse_lef_pinext(lef_path)
    whichfile, _ = lef_path.split('.l')
    results = {}
    for lef_key, lef_val in lef_data.items():
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


if __name__ == '__main__':
    if len(sys.argv) < 2 or sys.argv[1] not in ('run', 'metric', 'pin_extension'):
        print("valid subcommands: run, metric, pin_extension", file=sys.stderr)
        sys.exit(1)
    if sys.argv[1] == 'run':
        main_evaluation()
    elif sys.argv[1] == 'metric':
        main_metric()
    elif sys.argv[1] == 'pin_extension':
        main_pin_extension()
