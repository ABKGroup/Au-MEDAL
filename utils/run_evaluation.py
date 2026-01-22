import os
from multiprocessing import Pool
import argparse
import datetime as dt
import gdspy

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

######## CALIBRE LVS, DRC, PEX Scripts ########
"""
def make_lvs_script(cell_name, database, spice):
    return

def make_drc_script(cell_name, database):
    return

def make_pex_script(cell_name, database, spice):
    return
"""


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


def main():
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

    param = parser.parse_args()

    database = param.save_dir
    _, database_name = database.split("/")

    cells = os.listdir(database)
    spice = param.spice

    """
    if param.gen_script:
        for cell in cells:
            if cell[0] == '.':
                continue
            print(cell)
            database_cell = database + "/" + cell

            if database_cell.endswith(".gds"):
                continue

            if cell + ".gds" in os.listdir(database_cell):
                lvs_path = database_cell + "/" + "run_lvs.rul"
                drc_path = database_cell + "/" + "run_drc.rul"
                pex_path = database_cell + "/" + "run_xact.rul"

                wf_lvs = open(lvs_path, "w")
                wf_drc = open(drc_path, "w")
                wf_pex = open(pex_path, "w")

                script_lvs = make_lvs_script(cell, database, spice)
                script_drc = make_drc_script(cell, database)
                script_pex = make_pex_script(cell, database, spice)

                wf_lvs.write(script_lvs)
                wf_drc.write(script_drc)
                wf_pex.write(script_pex)

                wf_lvs.close()
                wf_drc.close()
                wf_pex.close()
    """

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
            os.makedirs("../LIBGen/ref_" + database_name, exist_ok=True)
            os.makedirs("../LIBGen/ref_" + database_name + "/ADDER", exist_ok=True)
            os.makedirs("../LIBGen/ref_" + database_name + "/AO", exist_ok=True)
            os.makedirs("../LIBGen/ref_" + database_name + "/INVBUF", exist_ok=True)
            os.makedirs("../LIBGen/ref_" + database_name + "/OA", exist_ok=True)
            os.makedirs("../LIBGen/ref_" + database_name + "/PHY", exist_ok=True)
            os.makedirs("../LIBGen/ref_" + database_name + "/SEQ", exist_ok=True)
            os.makedirs("../LIBGen/ref_" + database_name + "/SIMPLE", exist_ok=True)
            os.makedirs("../LIBGen/ref_" + database_name + "/SIMPLE", exist_ok=True)

            for f in os.listdir(cell_dir):
                if f[-3:] == ".sp":
                    merge_spice(cell, cell_dir + "/" + f, "../LIBGen/ref_{}/{}/{}".format(database, cell_type, f))


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
        os.chdir("LIBGen")
        os.system(f"python run_libGen.py {database_name}")

    if param.merge_gds:
        merge_gds(param.save_dir)

    
if __name__ == '__main__':
    main()
