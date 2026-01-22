import os
import csv
import gdstk
from multiprocessing import Pool
from itertools import product, combinations, chain
from shapely.geometry import Point, Polygon
from shapely.ops import unary_union
from parser import *
from net_extractor import NetExtractor


def pin_union(polys, layer):
    targets = [p.geom for p in polys if p.layer == layer]
    return unary_union(targets) if targets else None


def has_m2(polys, m2_layer):
    return any(p.layer == m2_layer for p in polys)


def m2_cands(union_geom, grid_x, grid_y):
    pts = [(x, y) for x in grid_x for y in grid_y if union_geom.contains(Point(x, y))]
    return [(*p, "L") for p in pts] + [(*p, "R") for p in pts]


def build_variants(net_dict, m1_layer, m2_layer, via_layer,
                   grid_x, grid_y, min_w, base_cell_name):
    base = gdstk.Cell(base_cell_name)
    cand = {}
    for net, polys in net_dict.items():
        if has_m2(polys, m2_layer):
            continue
            cand[net].append(None)
        if net.upper() == 'VDD' or net.upper() == "VSS":
            continue
        union_geom = pin_union(polys, m1_layer)
        if union_geom:
            c = m2_cands(union_geom, grid_x, grid_y)
            if c:
                cand[net] = c
    if not cand:
        return []

    variants = []
    for combo in product(*cand.values()):
        cell = gdstk.Cell(f"{base_cell_name}_{len(variants)}")
        cell.add(gdstk.Reference(base))
        for (x, y, d) in combo:
            via = gdstk.rectangle((x - min_w/2, y - min_w/2),
                                  (x + min_w/2, y + min_w/2), via_layer)
            if d == "L":
                m2 = gdstk.rectangle((x - 0.028, y - min_w/2),
                                     (x + min_w/2, y + min_w/2), m2_layer)
            else:
                m2 = gdstk.rectangle((x - min_w/2, y - min_w/2),
                                     (x + 0.028, y + min_w/2), m2_layer)
            cell.add(via)
            cell.add(m2)
        variants.append(cell)
    return variants


def build_m2s(net_dict, m1_layer, m2_layer, grid_x, grid_y, min_w):
    cand = {}
    for net, polys in net_dict.items():
        # if has_m2(polys, m2_layer):
        #     continue
        if net.upper() == 'VDD' or net.upper() == "VSS":
            continue
        union_geom = pin_union(polys, m1_layer)
        if union_geom:
            c = m2_cands(union_geom, grid_x, grid_y)
            if c:
                cand[net] = c
    if not cand:
        return []

    variants = []
    for combo in product(*cand.values()):
        m2_cand = []
        for (x, y, d) in combo:
            if d == "L":
                m2 = gdstk.rectangle((x - 0.028, y - min_w/2),
                                     (x + min_w/2, y + min_w/2), m2_layer)
            else:
                m2 = gdstk.rectangle((x - min_w/2, y - min_w/2),
                                     (x + 0.028, y + min_w/2), m2_layer)

            m2_cand.append(m2)
        variants.append(m2_cand)
    return variants


def check_num_track(m2_ref):
    def check_overlap(metal1, metal2):
        if gdstk.boolean(metal1, metal2, operation='and'):
            return True
        else:
            return False

    y_tracks = [0.036 * i for i in range(1, 8)]

    num_track = 7

    if len(m2_ref) > 0:
        m2_track_metals = [gdstk.rectangle((0, y - 0.027), (1, y + 0.027), 20) for y in y_tracks]

        for track in m2_track_metals:
            for m2 in m2_ref:
                if check_overlap(track, m2):
                    num_track -= 1
                    break

    return num_track


def eval(cell_list, gdsdir):
    res = []

    for cell_name in cell_list:
        if cell_name.startswith('.'):
            continue
        gds_path = f"{gdsdir}/{cell_name}/{cell_name}.gds"
        cell_data  = cell(gds_path, cell_name)
        via_map = {18: [(16, 19), (17, 19)], 21: [(19, 20)]}
        ext = NetExtractor(cell_data, via_map)
        ext.extract()

        if 20 in ext.layer_dict.keys():
            num_track = check_num_track(ext.layer_dict[20]['polys'])
        else:
            num_track = 7

        res.append([cell_name, num_track])

    return res


def write_csv(res, csvdir):
    with open(csvdir, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["cell_name", "HPTA"])
        writer.writerows(res)      


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

    gdsdir = gdsdir_asap_our

    res_asap_our = eval(cell_list, gdsdir_asap_our)
    # res_csyn_our = eval(cell_list, gdsdir_csyn_our)
    # res_nctu_our = eval(cell_list, gdsdir_nctu_our)
    # res_asap_ref = eval(cell_list, gdsdir_asap_ref)
    # res_csyn_ref = eval(cell_list, gdsdir_csyn_ref)
    # res_nctu_ref = eval(cell_list, gdsdir_nctu_ref)

    csv_asap_our = "asap_our_track.csv"
    # csv_csyn_our = "csyn_our_track.csv"
    # csv_asap_ref = "asap_ref_track.csv"
    # csv_nctu_our = "nctu_our_track.csv"
    # csv_csyn_ref = "csyn_ref_track.csv"
    # csv_nctu_ref = "nctu_ref_track.csv"

    write_csv(res_asap_our, csv_asap_our)
    # write_csv(res_csyn_our, csv_csyn_our)
    # write_csv(res_nctu_our, csv_nctu_our)
    # write_csv(res_asap_ref, csv_asap_ref)
    # write_csv(res_csyn_ref, csv_csyn_ref)
    # write_csv(res_nctu_ref, csv_nctu_ref)


    return


if __name__ == '__main__':
    main()