import os
import csv
import gdstk
from multiprocessing import Pool
from shapely.geometry import Point, Polygon
from shapely.ops import unary_union
from parser import *
from net_extractor import NetExtractor


def combos(lists, bad_at_c):
    def dfs(k, acc):
        if k == len(lists):
            yield tuple(acc); return
        for x in lists[k]:
            if k == 2 and bad_at_c(x):
                continue
            acc.append(x)
            yield from dfs(k + 1, acc)
            acc.pop()
    yield from dfs(0, [])


def pin_union(polys, layer):
    targets = [p.geom for p in polys if p.layer == layer]
    return unary_union(targets) if targets else None


def has_m2(polys, m2_layer):
    return any(p.layer == m2_layer for p in polys)


def m2_cands(union_geom, grid_x, grid_y):
    pts = []
    for x in grid_x:
        for y in grid_y:
            lx = x - 0.0089
            ux = x + 0.0089
            ly = y - 0.0089
            uy = y + 0.0089

            poly = Polygon([[lx, ly], [ux, ly], [lx, uy], [ux, uy]])

            if union_geom.contains(poly):
                pts.append([x, y])

    return [(*p, "L") for p in pts] + [(*p, "R") for p in pts]


def check_overlap(metal1, metal2):
    if metal1 is None or metal2 is None:
        return False
    if gdstk.boolean(metal1, metal2, operation='and'):
        # print(f"metal1:\n {metal1.points}")
        # print(f"metal2:\n {metal2.points}")
        # print("============================")
        return True
    else:
        return False


def check_spacing(metal1, metal2):
    if metal1 is None or metal2 is None:
        return False    
    p1 = metal1.points
    p2 = metal2.points
    xs_1 = [p[0] for p in p1]
    xs_2 = [p[0] for p in p2]
    ys_1 = [p[1] for p in p1]
    ys_2 = [p[1] for p in p2]
    
    lx_1 = min(xs_1)
    ux_1 = max(xs_1)
    lx_2 = min(xs_2)
    ux_2 = max(xs_2)
    ly_1 = min(ys_1)
    uy_1 = max(ys_1)
    ly_2 = min(ys_2)
    uy_2 = max(ys_2)

    # side to side
    if lx_1 == lx_2:
        if abs(ly_1 - uy_2) < 0.018:
            # print(f"Side to Side : {abs(ly_1 - uy_2)}")
            # print(f"points: {p1} {p2}")
            return True
        
        elif abs(uy_1 - ly_2) < 0.018:
            # print(f"Side to Side : {abs(uy_1 - ly_2)}")
            # print(f"points: {p1} {p2}")
            return True
        
    # tip to tip
    elif ly_1 == ly_2:
        # if abs(lx_1 - ux_2) < 31:
        if abs(lx_1 - ux_2) < 0.031:
            # print(f"Tip to Tip : {abs(lx_1 - ux_2)}")
            # print(f"points: {lx_1} {ux_2}")
            return True

        # if abs(ux_1 - lx_2) < 31:
        if abs(ux_1 - lx_2) < 0.031:
            # print(f"Tip to Tip : {abs(ux_1 - lx_2)}")
            # print(f"points: {ux_1} {lx_2}")
            return True
        
    # Corner to Corner
    else:
        min_c2c = float("inf")
        for x1, y1 in p1:
            for x2, y2 in p2:
                dist = (x1 - x2) ** 2 + (y1 - y2) ** 2
                if min_c2c > dist:
                    min_c2c = dist

        if min_c2c < 0.0004:
            # print(f"Corner to Corner : {dist}")
            # print(f"points: {p1} {p2}")
            return True
                    
    return False


def check_spacing_ref(metal, metal_ref):
    if metal is None:
        return False
    p_new = metal.points
    p_ref = metal_ref.points
    xs_new = [p[0] for p in p_new]
    xs_ref = [p[0] for p in p_ref]
    ys_new = [p[1] for p in p_new]
    ys_ref = [p[1] for p in p_ref]
    
    lx_new = min(xs_new)
    ux_new = max(xs_new)
    lx_ref = min(xs_ref)
    ux_ref = max(xs_ref)
    ly_new = min(ys_new)
    uy_new = max(ys_new)
    ly_ref = min(ys_ref)
    uy_ref = max(ys_ref)

    # tip to tip
    if ly_new == ly_ref:
        if abs(lx_new - ux_ref) < 0.031:
            # print(f"Tip to Tip : {abs(lx_new - ux_ref)}")
            # print(f"points: {lx_new} {ux_ref}")
            return True

        if abs(ux_new - lx_ref) < 0.031:
            # print(f"Tip to Tip : {abs(ux_new - lx_ref)}")
            # print(f"points: {ux_new} {lx_ref}")
            return True
        
    # side to side
    elif max(lx_new, lx_ref) < min(ux_new, ux_ref):
        if abs(ly_new - uy_ref) < 0.018:
            # print(f"Side to Side : {abs(ly_new - uy_ref)}")
            # print("points")
            # print(p_new)
            # print(p_ref)
            # print("====================")
            return True
        
        elif abs(uy_new - ly_ref) < 0.018:
            # print(f"Side to Side : {abs(uy_new - ly_ref)}")
            # print("points")
            # print(p_new)
            # print(p_ref)
            # print("====================")
            return True
    
    # corner to corner
    else:
        min_c2c = float("inf")
        for x1, y1 in p_new:
            for x2, y2 in p_ref:
                dist = (x1 - x2) ** 2 + (y1 - y2) ** 2
                if min_c2c > dist:
                    min_c2c = dist

        if min_c2c < 0.0004:
            # print(f"Corner to Corner : {dist}")
            # print("points")
            # print(p_new)
            # print(p_ref)
            # print("====================")
            return True
            
    return False


def backtrack(net_dict, m1_layer, m2_layer, grid_x, grid_y, min_w, m2_refs):
    cand = {}
    for net, polys in net_dict.items():
        if net.upper() == 'VDD' or net.upper() == "VSS":
            continue
        union_geom = pin_union(polys, m1_layer)
        if union_geom:
            c = m2_cands(union_geom, grid_x, grid_y)

            if c:
                cand[net] = c

        if has_m2(polys, m2_layer):
            if net not in cand.keys():
                cand[net] = []
            cand[net].append(None)
    
    if not cand:
        return
    
    variants = []

    pin_order = list(cand.keys())

    def dfs(idx, m2_cand):
        if idx == len(pin_order):
            assert(len(m2_cand) == len(pin_order))
            variants.append(m2_cand.copy())
            return

        for choice in cand[pin_order[idx]]:
            if choice is None:
                m2_cand.append(None)

            else:
                x, y, d = choice
                if d == "L":
                    m2 = gdstk.rectangle(
                        (x - 0.028,  y - min_w / 2),
                        (x + min_w / 2 + 0.005, y + min_w / 2),
                        m2_layer
                    )
                else:
                    m2 = gdstk.rectangle(
                        (x - min_w / 2 - 0.005, y - min_w / 2),
                        (x + 0.028,     y + min_w / 2),
                        m2_layer
                    )

                violated = False
                for m2_ref in m2_refs:
                    if check_overlap(m2, m2_ref) or check_spacing_ref(m2, m2_ref):
                        violated = True
                        break

                for cand_rect in m2_cand:
                    # print("violated")
                    if check_overlap(m2, cand_rect) or check_spacing(m2, cand_rect):
                        violated = True
                        break

                if violated:
                    continue

                m2_cand.append(m2)

            dfs(idx + 1, m2_cand)
            m2_cand.pop()

    dfs(0, [])

    return len(variants)


def write_gds(src_gds_path, variants, cell_name, out_gds_path):
    os.makedirs(out_gds_path, exist_ok=True)
    for idx, v in enumerate(variants):
        lib = gdstk.read_gds(src_gds_path)
        lib.add(v)
        lib.write_gds(f"{out_gds_path}/{cell_name}_{idx}.gds")


def run_eval(inputs):
    gdsdir, cell_name = inputs
    gds_path = f"{gdsdir}/{cell_name}/{cell_name}.gds"
    via_map = {18: [(16, 19), (17, 19)], 21: [(19, 20)]}
    grid_x = [i * 0.027 for i in range(80)]
    # grid_x = [i * 0.009 for i in range(240)]

    # Down-Up Grid
    grid_y = [i * 0.036 for i in range(1, 8)]

    # Up-Down Grid
    # grid_y = [0.27 - 0.036 * i for i in range(1, 8)]

    # Grid Including Middle
    # grid_y = [0.036, 0.072, 0.108, 0.135, 0.162, 0.198, 0.234, 0.27]
    min_width = 0.018

    if cell_name.startswith('.'):
        return []
    
    cell_data  = cell(gds_path, cell_name)

    ext = NetExtractor(cell_data, via_map)
    ext.extract()

    dict_pins = {}

    for key, vals in ext.net_dict.items():
        if key.startswith("NET_"):
            continue
        else:
            dict_pins[key] = vals

    print(cell_name)
    m2_ref = cell_data.layer_data[20]['polys'] if 20 in cell_data.layer_data.keys() else []
    val = backtrack(net_dict=dict_pins, grid_x=grid_x, grid_y=grid_y, min_w=min_width, m1_layer=19, m2_layer=20, m2_refs=m2_ref)

    return val


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
            writer.writerow(["cell_name", "PAF"])
            writer.writerows(result)        

    return


if __name__ == '__main__':
    main()