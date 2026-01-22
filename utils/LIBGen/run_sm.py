
import os
import sys
import re
import subprocess as sp

import utils
import changeFiles as cf

def run_sm(database):
    libDir = f"LIBGEN_{database}"
    libName = f"LIB_{database}"

    os.makedirs(libName, exist_ok=True)
    except_lib = utils.getExceptLib(libDir, libName)

    if len(except_lib) == 8:
        print("Lib data already exists")
        return    

    
    cf.change_cell_area('../ref_LIB', f'./LIB', f'../../../{database}')

    run_lines = utils.getFileLines("../ref_run.tcl")
    modified_run_lines = utils.except_sm("cells", run_lines, except_lib)

    utils.copy_folder('../ref_hspice', "./hspice")
    
    SPICEModeldir = "./spice_models/"   
    ref_SPICEModeldir = "../ref_" + database + "/"

    utils.copy_folder(ref_SPICEModeldir, SPICEModeldir)
    print(SPICEModeldir)

    convert = re.compile(".+(ASAP7_75t_[.\w]+)[.]sp")
    for prefix in os.listdir(SPICEModeldir):
        for cdl in os.listdir(os.path.join(SPICEModeldir, prefix)):
            if convert.match(cdl):
                new_file = re.sub(convert.match(cdl).group(1), "ASAP7_75t_R", cdl)
                utils.move_files(SPICEModeldir+"/"+prefix+"/"+cdl, SPICEModeldir+"/"+prefix+"/"+new_file)
    
    cells = cf.read_sp(SPICEModeldir)
    libGen(modified_run_lines, cells, database)
    print("============== Lib generation done ================")

def libGen(run_lines, cells, database):
    libName = f"LIB_{database}"
    cf.change_run(run_lines, cells, libName)
    os.makedirs(libName+'/sis_logs', exist_ok=True)
    sp.call(f"siliconsmart {libName}/run_sm.tcl", shell=True)


if __name__ == "__main__":
    database = sys.argv[1]

    run_sm(database)
