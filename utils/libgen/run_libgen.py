# Portions Copyright 2022 Synopsys, Inc. All rights reserved. Portions of
# these TCL scripts are proprietary to and owned by Synopsys, Inc. and may only
# be used for internal use by educational institutions (including United States
# government labs, research institutes and federally funded research and
# development centers) on Synopsys tools for non-profit research, development,
# instruction, and other non-commercial uses or as otherwise specifically set forth
# by written agreement with Synopsys. All other use, reproduction, modification, or
# distribution of these TCL scripts is strictly prohibited.
"""Single-file libgen flow that stages SiliconSmart inputs and generates
the Liberty library for a database under LIBGEN_<database>."""
import os
import sys
import re
import time
import shutil
import itertools
import subprocess as sp

def getConfig():
    phigs = [4.372]
    eots = [1.0e-9]
    ls = [2.1e-008]
    vdds = [0.70]
    temp = [25.0]

    return itertools.product(phigs, eots, ls, vdds, temp)

def getFileLines(file_name):
    with open(file_name, "r") as f:
        return f.readlines()
    
def writeFileLines(file_name, lines):
    with open(file_name, "w") as f:
        f.writelines(lines)

def move_files(source_folder, destination_folder):
    try:
        shutil.move(source_folder, destination_folder)
    except Exception as e:
        pass

def copy_files(source_files, destination_files):
    try:
        shutil.copy(source_files, destination_files)
    except Exception as e:
        pass
    
def copy_folder(source_folder, destination_folder):
    try:
        shutil.copytree(source_folder, destination_folder)
        print(source_folder)
        print(destination_folder)
    except Exception as e:
        pass

def delete_files(folder):
    try:
        shutil.rmtree(folder)
    except Exception as e:
        pass

def getExceptLib(libDir, lib_name):
    except_lib = list()
    type = ['AO', 'OA', 'SIMPLE', 'INVBUF', 'SEQ', 'ADDER', 'CKINVDC', 'PHY']
    for t in type:
        if os.path.isfile(libDir+"/"+lib_name+"/"+t+"_"+lib_name+".lib"):
            except_lib.append(t)

    return except_lib


def except_sm(prefix, run_lines, except_lib):
    w_lines = list()
    for line in run_lines:
        for name in except_lib:
            if re.search(prefix+name, line):
                line = "# "+line
        w_lines.append(line)

    writeFileLines('./except_sm.tcl', w_lines)    
    return w_lines

def change_cell_area(REF_DIR, CHANGE_DIR, database):
    os.makedirs(CHANGE_DIR, exist_ok=True)
    lib_files = os.listdir(REF_DIR)
    existing_cells = os.listdir(database)

    for lib_file in lib_files:
        if not lib_file.endswith('.lib'):
            continue
        lines = getFileLines(os.path.join(REF_DIR, lib_file))
        w_lines = list()
        for line in lines:
            if "cell (" in line or "cell(" in line:
                _, cell_name = line.split('(')
                cell_name, _ = cell_name.split(')')
                cell_name = cell_name.replace("CUSTOM_4_372_0_70_75T", "R")
                cell_dir = f"{database}/{cell_name}/area.txt"

                if cell_name in existing_cells:
                    try:
                        file_area = open(cell_dir, 'r')
                        file_line = file_area.readlines()
                        if file_line is not None:
                            if file_line[0][0] != '(':
                                cell_area = file_line[0]
                                
                    except FileNotFoundError:
                        print("no existing area files for cell {}".format(cell_name))
                else:
                    area = 0
            
            if "CUSTOM_4_372_0_70_75T" in line:
                line = line.replace("CUSTOM_4_372_0_70_75T", "R")
            
            if "area" in line and cell_name in existing_cells:
                line_pre, line_post = line.split(":")
                line = line_pre + ": " + cell_area + ";\n"

            w_lines.append(line)
            
        writeFileLines(os.path.join(CHANGE_DIR, lib_file), w_lines)        


def change_run(run_lines, cells, lib_name):
    w_lines = list()
    read_conf_tcl(lib_name)
    for line in run_lines:
        words = line.split(' ')
        if words[0] == 'set':
            if words[1] == 'cellsAO':
                cell_list = str()
                for idx, cell in enumerate(cells['AO']):
                    cell_list += cell + ' '
                words[2] = '{ ' + cell_list + '}' + '\n'
            elif words[1] == 'cellsOA':
                cell_list = str()
                for idx, cell in enumerate(cells['OA']):
                    cell_list += cell + ' '
                words[2] = '{ ' + cell_list + '}' + '\n'
            elif words[1] == 'cellsSIMPLE':
                cell_list = str()
                for idx, cell in enumerate(cells['SIMPLE']):
                    cell_list += cell + ' '
                words[2] = '{ ' + cell_list + '}' + '\n'
            elif words[1] == 'cellsINVBUF':
                cell_list = str()
                for idx, cell in enumerate(cells['INVBUF']):
                    cell_list += cell + ' '
                words[2] = '{ ' + cell_list + '}' + '\n'
            elif words[1] == 'cellsSEQ':
                cell_list = str()
                for idx, cell in enumerate(cells['SEQ']):
                    cell_list += cell + ' '
                words[2] = '{ ' + cell_list + '}' + '\n'
            elif words[1] == 'cellsADDER':
                cell_list = str()
                for idx, cell in enumerate(cells['ADDER']):
                    cell_list += cell + ' '
                words[2] = '{ ' + cell_list + '}' + '\n'
            elif words[1] == 'cellsCKINVDC':
                cell_list = str()
                for idx, cell in enumerate(cells['CKINVDC']):
                    cell_list += cell + ' '
                words[2] = '{ ' + cell_list + '}' + '\n'
            elif words[1] == 'cellsPHY':
                cell_list = str()
                for idx, cell in enumerate(cells['PHY']):
                    cell_list += cell + ' '
                words[2] = '{ ' + cell_list + '}' + '\n'
            elif words[1] == 'charpoint':
                words[2] = lib_name + '\n'
        line = ' '.join(words)
        w_lines.append(line)
    writeFileLines(lib_name + '/run_sm.tcl', w_lines)


def read_sp(SPICEModeldir):
    cell_types = {
        'AO': [],
        'OA': [],
        'SIMPLE': [],
        'INVBUF': [],
        'SEQ': [],
        'ADDER': [],
        'CKINVDC': [],
        'PHY': [] 
    }
    
    for LIBS in os.listdir(SPICEModeldir):
        for cdl in os.listdir(os.path.join(SPICEModeldir, LIBS)):
            print(cdl)
            if cdl.endswith('.sp'):
                cell_name = cdl[:-3]
                cell_types[LIBS].append(cell_name)            

    return cell_types


def read_conf_tcl(lib_name):
    lines = getFileLines('../ref_configure.tcl')

    w_lines = list()
    for line in lines:
        words = line.split(' ')
        if words[0] == "set":
            if words[1] == "active_pvts":
                words[2] = lib_name + "\n"
        elif words[0] == "create_operating_condition":
            words[1] = lib_name + "\n"
        elif words[0] == "add_opc_supplies":
            words[1] = lib_name
            words[3] = "0.70" + "\n"
        elif words[0] == "add_opc_grounds":
            words[1] = lib_name
        elif words[0] == "set_opc_temperature":
            words[1] = lib_name
            words[2] = "25.0" + "\n"
        elif words[0] == "set_opc_process":
            words[1] = lib_name
        elif words[0] == "set_opc_default_voltage":
            words[1] = lib_name
            words[2] = "0.7" + "\n"
        line = " ".join(words)
        w_lines.append(line)
    writeFileLines(lib_name + '/configure_sm.tcl', w_lines)


def run_sm(database):
    libDir = f"LIBGEN_{database}"
    libName = f"LIB_{database}"

    os.makedirs(libName, exist_ok=True)
    except_lib = getExceptLib(libDir, libName)

    if len(except_lib) == 8:
        print("Lib data already exists")
        return    

    
    change_cell_area('../ref_lib', f'./LIB', f'../../../{database}')

    run_lines = getFileLines("../ref_run.tcl")
    modified_run_lines = except_sm("cells", run_lines, except_lib)

    copy_folder('../ref_hspice', "./hspice")
    
    SPICEModeldir = "./spice_models/"   
    ref_SPICEModeldir = "../ref_" + database + "/"

    copy_folder(ref_SPICEModeldir, SPICEModeldir)
    print(SPICEModeldir)

    convert = re.compile(".+(ASAP7_75t_[.\w]+)[.]sp")
    for prefix in os.listdir(SPICEModeldir):
        for cdl in os.listdir(os.path.join(SPICEModeldir, prefix)):
            if convert.match(cdl):
                new_file = re.sub(convert.match(cdl).group(1), "ASAP7_75t_R", cdl)
                move_files(SPICEModeldir+"/"+prefix+"/"+cdl, SPICEModeldir+"/"+prefix+"/"+new_file)
    
    cells = read_sp(SPICEModeldir)
    libGen(modified_run_lines, cells, database)
    print("============== Lib generation done ================")

def libGen(run_lines, cells, database):
    libName = f"LIB_{database}"
    change_run(run_lines, cells, libName)
    os.makedirs(libName+'/sis_logs', exist_ok=True)
    sp.call(f"siliconsmart {libName}/run_sm.tcl", shell=True)

def run(database):
	start = time.time()
	dir_name = f"LIBGEN_{database}"
	os.makedirs(dir_name, exist_ok=True)
	os.chdir(dir_name)

	sp.call(f"python ../run_libgen.py sm {database}", shell=True)
	os.chdir("../")

if __name__ == "__main__":
	if sys.argv[1] == "sm":
		database = sys.argv[2]

		run_sm(database)
	else:
		database = sys.argv[1]

		libDir = f"LIBGEN_{database}"
		os.makedirs(libDir, exist_ok=True)
		lib_name = f"LIB_{database}"
		except_lib = getExceptLib(libDir, lib_name)		

		if len(except_lib) == 8:
			print("data already exists")

		run(database)

		time.sleep(5)
