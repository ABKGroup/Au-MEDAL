import os
import csv
from liberty.parser import parse_liberty

def parse_lib(liberty_str):
    lib = parse_liberty(liberty_str)
    results = {}

    for cell in lib.get_groups("cell"):
        cell_name = cell.args[0]
        results[cell_name] = {}
        for pin in cell.get_groups("pin"):
            pin_name = pin.args[0]
            results[cell_name][pin_name] = {"RP": [], "FP": [], "RD": [], "FD": [], "RT": [], "FT": []}
            for timing in pin.get_groups("timing"):
                for group in timing.get_groups("cell_rise"):
                    values = group.get("values", [])
                    if not values:
                        continue
                    all_vals = []
                    for v in values:
                        numbers = list(map(float, v.value.replace("\\", "").replace("n", "").strip('"').split(",")))
                        all_vals.extend(numbers)
                    if all_vals:
                        results[cell_name][pin_name]["RD"].append(all_vals)
                for group in timing.get_groups("cell_fall"):
                    values = group.get("values", [])
                    if not values:
                        continue
                    all_vals = []
                    for v in values:
                        numbers = list(map(float, v.value.replace("\\", "").replace("n", "").strip('"').split(",")))
                        all_vals.extend(numbers)
                    if all_vals:
                        results[cell_name][pin_name]["FD"].append(all_vals)
                for group in timing.get_groups("rise_transition"):
                    values = group.get("values", [])
                    if not values:
                        continue
                    all_vals = []
                    for v in values:
                        numbers = list(map(float, v.value.replace("\\", "").replace("n", "").strip('"').split(",")))
                        all_vals.extend(numbers)
                    if all_vals:
                        results[cell_name][pin_name]["RT"].append(all_vals)
                for group in timing.get_groups("fall_transition"):
                    values = group.get("values", [])
                    if not values:
                        continue
                    all_vals = []
                    for v in values:
                        numbers = list(map(float, v.value.replace("\\", "").replace("n", "").strip('"').split(",")))
                        all_vals.extend(numbers)
                    if all_vals:
                        results[cell_name][pin_name]["FT"].append(all_vals)

            for power in pin.get_groups("internal_power"):
                for group in power.get_groups("rise_power"):
                    values = group.get("values", [])
                    if not values:
                        continue
                    all_vals = []
                    for v in values:
                        numbers = list(map(float, v.value.replace("\\", "").replace("n", "").strip('"').split(",")))
                        all_vals.extend(numbers)
                    if all_vals:
                        results[cell_name][pin_name]["RP"].append(all_vals)
                for group in power.get_groups("fall_power"):
                    values = group.get("values", [])
                    if not values:
                        continue
                    all_vals = []
                    for v in values:
                        numbers = list(map(float, v.value.replace("\\", "").replace("n", "").strip('"').split(",")))
                        all_vals.extend(numbers)
                    if all_vals:
                        results[cell_name][pin_name]["FP"].append(all_vals)
    return results


def save_results(lib_path):
    dict_results = {}

    for fp in lib_path:
        file = open(fp).read()
        file = file.replace("comment : \"\" ; ", "")
        file = file.replace("date : \"$Date: Fri Nov 27 12:11:00 2020 $\" ; ", "")
        res = parse_lib(file)
        for cell_name, cell_val in res.items():
            dict_results[cell_name] = cell_val

    return dict_results


def write_csv(res, csvdir):
    with open(csvdir, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["cell_name", "RP", "FP", "RD", "FD", "RT", "FT"])
        writer.writerows(res)   


def calc_results(dict_our, dict_ref):
    def avg(l):
        if len(l) > 10:
            sorted_list = sorted(l)
            n = len(l)
            k = int(n * 0.05)

            trimmed = sorted_list[k:n-k]
            return sum(trimmed) / len(trimmed)
        else:        
            return sum(l) / len(l) if len(l) > 0 else None
        
    cell_list = set(dict_our.keys()).intersection(dict_ref.keys())
    results = []

    for cell_name in cell_list:
        val_our = dict_our[cell_name]
        val_ref = dict_ref[cell_name]
        rp = []
        fp = []
        rd = []
        fd = []
        rt = []
        ft = []

        assert(len(val_our.keys()) == len(val_ref.keys()))

        for pin_name in val_our.keys():
            pin_val_our = dict_our[cell_name][pin_name]
            pin_val_ref = dict_ref[cell_name][pin_name]

            assert(len(pin_val_our["RP"]) == len(pin_val_ref["RP"]))
            assert(len(pin_val_our["FP"]) == len(pin_val_ref["FP"]))
            assert(len(pin_val_our["RD"]) == len(pin_val_ref["RD"]))
            assert(len(pin_val_our["FD"]) == len(pin_val_ref["FD"]))
            assert(len(pin_val_our["RT"]) == len(pin_val_ref["RT"]))
            assert(len(pin_val_our["FT"]) == len(pin_val_ref["FT"]))

            for i in range(len(pin_val_our["RP"])):
                rp.append(avg([x / (y + 1e-20) for x, y in zip(pin_val_our["RP"][i], pin_val_ref["RP"][i])]))

            for i in range(len(pin_val_our["FP"])):
                fp.append(avg([x / (y + 1e-20) for x, y in zip(pin_val_our["FP"][i], pin_val_ref["FP"][i])]))

            for i in range(len(pin_val_our["RD"])):
                rd.append(avg([x / (y + 1e-20) for x, y in zip(pin_val_our["RD"][i], pin_val_ref["RD"][i])]))

            for i in range(len(pin_val_our["FD"])):
                fd.append(avg([x / (y + 1e-20) for x, y in zip(pin_val_our["FD"][i], pin_val_ref["FD"][i])]))

            for i in range(len(pin_val_our["RT"])):
                rt.append(avg([x / (y + 1e-20) for x, y in zip(pin_val_our["RT"][i], pin_val_ref["RT"][i])]))

            for i in range(len(pin_val_our["FT"])):
                ft.append(avg([x / (y + 1e-20) for x, y in zip(pin_val_our["FT"][i], pin_val_ref["FT"][i])]))

        results.append([cell_name, avg(rp), avg(fp), avg(rd), avg(fd), avg(rt), avg(ft)])


    return results

csyn_our = []
nctu_our = []
asap_our = []
csyn_ref = []
nctu_ref = []
asap_ref = []

asap_our_path = "../results/ours_ref"
csyn_our_path = ""
nctu_our_path = ""
asap_ref_path = ""
csyn_ref_path = ""
nctu_ref_path = ""

for filename in os.listdir(asap_our_path):
    if filename.endswith('.lib'):
        asap_our.append(os.path.join(asap_our_path, filename))

# for filename in os.listdir(csyn_our_path):
#     if filename.endswith('.lib'):
#         csyn_our.append(os.path.join(csyn_our_path, filename))

# for filename in os.listdir(nctu_our_path):
#     if filename.endswith('.lib'):
#         nctu_our.append(os.path.join(nctu_our_path, filename))

# for filename in os.listdir(asap_ref_path):
#     if filename.endswith('.lib'):
#         asap_ref.append(os.path.join(asap_ref_path, filename))

# for filename in os.listdir(csyn_ref_path):
#     if filename.endswith('.lib'):
#         csyn_ref.append(os.path.join(csyn_ref_path, filename))

# for filename in os.listdir(nctu_ref_path):
#     if filename.endswith('.lib'):
#         nctu_ref.append(os.path.join(nctu_ref_path, filename))

# need reference .lib file (from siliconsmart)
asap_our_dict = save_results(asap_our)
# csyn_our_dict = save_results(csyn_our)
# nctu_our_dict = save_results(nctu_our)
# asap_dict = save_results(asap_ref)
# csyn_dict = save_results(csyn_ref)
# nctu_dict = save_results(nctu_ref)


res_asap = calc_results(dict_our=asap_our_dict, dict_ref=asap_dict)
csv_asap = './asap_trim.csv'
write_csv(csvdir=csv_asap, res=res_asap)

# res_csyn = calc_results(dict_our=csyn_our_dict, dict_ref=csyn_dict)
# csv_csyn = './csyn_trim.csv'
# write_csv(csvdir=csv_csyn, res=res_csyn)

# res_nctu = calc_results(dict_our=nctu_our_dict, dict_ref=nctu_dict)
# csv_nctu = './nctu_trim.csv'
# write_csv(csvdir=csv_nctu, res=res_nctu)
