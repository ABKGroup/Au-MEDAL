import os
import csv
import gdstk
from multiprocessing import Pool
from itertools import product, combinations, chain
from shapely.geometry import Point, Polygon
from shapely.ops import unary_union
from parser import *
from net_extractor import NetExtractor


def check_overlap(metal1, metal2):
    if metal1 is None or metal2 is None:
        return False
    if gdstk.boolean(metal1, metal2, operation='and'):
        return True
    else:
        return False


def num_vias(grid_x, grid_y, m2_refs):
    vias = []
    for x in grid_x:
        for y in grid_y:
            vias.append(gdstk.rectangle(
                (x - 0.027, y - 0.027),
                (x + 0.027, y + 0.027),
                20
            ))

    num_v = len(vias)

    for v in vias:
        for m2 in m2_refs:
            if check_overlap(v, m2):
                num_v -= 1
                break

    return num_v


def run_eval(inputs):
    gdsdir, cell_name = inputs
    via_map = {18: [(16, 19), (17, 19)], 21: [(19, 20)]}
    min_width = 0.018

    if cell_name.startswith('.'):
        return []
    
    gds_path = f"{gdsdir}/{cell_name}/{cell_name}.gds"
    cell_data  = cell(gds_path, cell_name)

    ext = NetExtractor(cell_data, via_map)
    ext.extract()
    boundary = ext.layer_dict[100]['polys'][0]
    xs = [p[0] for p in boundary.points]
    x_max = max(xs)

    grid_x = []
    # grid_y = [i * 0.036 for i in range(1, 8)]
    grid_y = [0.036, 0.072, 0.135, 0.198, 0.234]
    
    x_idx = 0
    while x_idx <= x_max + 1e-6:
        grid_x.append(x_idx)
        x_idx += 0.027

    m2_ref = cell_data.layer_data[20]['polys'] if 20 in cell_data.layer_data.keys() else []
    num_via = num_vias(grid_x, grid_y, m2_ref)

    return num_via / (len(grid_x) * len(grid_y))


def main():
    gdsdir_asap_our = "../results/ours_ref"
    # gdsdir_csyn_our = ""
    # gdsdir_nctu_our = ""
    # gdsdir_asap_ref = ""
    # gdsdir_csyn_ref = ""
    # gdsdir_nctu_ref = ""

    set_asap_our = set(os.listdir(gdsdir_asap_our))
    # set_csyn_our = set(os.listdir(gdsdir_csyn_our))
    # set_nctu_our = set(os.listdir(gdsdir_nctu_our))

    # set_asap_ref = set(os.listdir(gdsdir_asap_ref))
    # set_csyn_ref = set(os.listdir(gdsdir_csyn_ref))
    # set_nctu_ref = set(os.listdir(gdsdir_nctu_ref))

    cell_list = set_asap_our
    # cell_list = set_csyn_our.intersection(set_nctu_our)
    # cell_list = cell_list.intersection(set_asap_our)
    # cell_list = cell_list.intersection(set_csyn_ref)
    # cell_list = cell_list.intersection(set_nctu_ref)
    # cell_list = cell_list.intersection(set_asap_ref)

    gdsdir = [gdsdir_asap_our]
    # gdsdir = [gdsdir_csyn_our, gdsdir_nctu_our, gdsdir_asap_our, gdsdir_csyn_ref, gdsdir_nctu_ref, gdsdir_asap_ref]
    result = []

    for cell_name in cell_list:
        if cell_name.startswith('.'):
            cell_list.remove(cell_name)

    for g in gdsdir:
        inputs = [(g, cell_name) for cell_name in cell_list]
        csvdir = f"{g.split('/')[-1]}.csv"

        with Pool(processes=32) as pool:
            results = (pool.map(run_eval, inputs))

        result = [[cell_name, res] for (g, cell_name), res in zip(inputs, results)]

        with open(csvdir, "w", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(["cell_name", "H2VVA"])
            # writer.writerows(sorted_result)        
            writer.writerows(result)        

    return


if __name__ == '__main__':
    main()