import os
import utils
import re

def change_cell_area(REF_DIR, CHANGE_DIR, database):
    os.makedirs(CHANGE_DIR, exist_ok=True)
    lib_files = os.listdir(REF_DIR)
    existing_cells = os.listdir(database)

    for lib_file in lib_files:
        if not lib_file.endswith('.lib'):
            continue
        lines = utils.getFileLines(os.path.join(REF_DIR, lib_file))
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
                # num_pre, num_post = line.split(';')
                line = line_pre + ": " + cell_area + ";\n"

            w_lines.append(line)
            
        utils.writeFileLines(os.path.join(CHANGE_DIR, lib_file), w_lines)        


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
    utils.writeFileLines(lib_name + '/run_sm.tcl', w_lines)


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
    lines = utils.getFileLines('../ref_configure.tcl')

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
    utils.writeFileLines(lib_name + '/configure_sm.tcl', w_lines)