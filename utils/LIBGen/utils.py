import os
import shutil
import re
import subprocess as sp
import itertools

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
        # return f"Write {file_name} done."

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
            # pass

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
