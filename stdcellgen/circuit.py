import copy
import os
import fnmatch

from .structure import *

class Circuit():
    def __init__(self, cell_name, file_name, save_dir, graph_type=None):
        self.fets, self.ext_pins, self.fet_name_to_fet = self.get_infos(file_name, cell_name)        
        self.cell_name = cell_name
        self.file_name = file_name

        self.save_dir = save_dir
        self.fet_name_to_index = dict()
        self.index_to_fet_name = list()
        self.fet_name_to_feat = dict()

        self.fet_indices = {N_TYPE: [], P_TYPE: []}

        self.fet_names_in_net_name = dict()
        self.net_name_to_index = dict()
        self.index_to_net_name = list()

        self.term_type = dict()
        self.graph_type = graph_type
        
    def get_infos(self, file_name, cell_name):
        with open(file_name, "r") as f:
            lines = [line.strip() for line in f]

        fets = {N_TYPE: [], P_TYPE: []}
        fet_name_to_fet = {}
        start = False
        ext_pins = []

        for line in lines:
            upper_line = line.upper()
            words = line.split()

            if not start and f" {cell_name.upper()} " in upper_line:
                start = True
                ext_pins = words[2:]

            if start:
                if words and "M" in words[0].upper():  # Checking if it's a MOSFET
                    name, drain, gate, source, body, fet_type = words[:6]
                    options = {opt.split("=")[0].upper(): opt.split("=")[1] for opt in words[6:]}
                    nfin = int(options["NFIN"])

                    if "PMOS" in fet_type.upper():
                        max_fin = int(design_rules["num_max_pmos_fins"])
                    elif "NMOS" in fet_type.upper():
                        max_fin = int(design_rules["num_max_nmos_fins"])
                    else:
                        raise TypeError("Invalid fet_type!")

                    # modifying fingers
                    nfingers = nfin // max_fin
                    remainder = nfin % max_fin

                    fins_per_finger = [max_fin] * nfingers

                    if remainder > 0:
                        if remainder <= max_fin // 2:
                            fins_per_finger.append(remainder)
                        else:
                            if nfingers > 0:  #
                                fins_per_finger[-1] = max_fin - (max_fin - remainder) // 2
                                fins_per_finger.append((max_fin - remainder) // 2)
                            else:
                                fins_per_finger = [remainder]  # 

                    if len(fins_per_finger) > 1 and abs(fins_per_finger[-1] - fins_per_finger[-2]) > 1:
                        avg = sum(fins_per_finger) // len(fins_per_finger)
                        fins_per_finger = [avg + (1 if i < sum(fins_per_finger) % len(fins_per_finger) else 0) for i in range(len(fins_per_finger))]

                    for i, fins in enumerate(fins_per_finger):
                        new_options = copy.deepcopy(options)
                        new_options["NFIN"] = fins
                        new_options["W"] = str(fins * design_rules["pitch"]["fin"])+"n"

                        fet_name = f"{name}_{i}"                                                                
                        fet = MOSFET(fet_name, drain, gate, source, body, fet_type, new_options)
                        
                        fet_name_to_fet[fet_name] = fet
                        fets[N_TYPE if "NMOS" in fet_type.upper() else P_TYPE].append(fet)

                elif ".END" in upper_line:
                    break

        if not start:
            raise RuntimeError(f"Cell name '{cell_name}' not found.")

        return fets, ext_pins, fet_name_to_fet



    def get_fet_info(self):
        file_name = self.file_name
        cell_name = self.cell_name

        fets_info = {}
        ext_pins = []

        start = False

        with open(file_name, "r") as f:
            lines_file = [line.strip() for line in f]

        for line in lines_file:
            upper_line = line.upper()
            words = line.split()

            if not start and f" {cell_name.upper()} " in upper_line:
                start = True
                ext_pins = words[2:]

            if start:
                if words and "MM" in words[0].upper():  # Checking if it's a MOSFET
                    name, drain, gate, source, body, fet_type = words[:6]
                    options = {opt.split("=")[0].upper(): opt.split("=")[1] for opt in words[6:]}
                    nfin = int(options["NFIN"])
                    fets_info[name] = [drain, gate, source, body, fet_type, options]

                elif ".END" in upper_line:
                    break
        
        return fets_info, ext_pins


    def read_placement(self, placement_file="", solution_num=1):
        def is_int(value):
            try:
                int(value)
                return True
            except:
                return False

        file_fetorder = open(f"{placement_file}", "r")
        lines_fetorder = file_fetorder.readlines()
        file_fetorder.close()

        os.makedirs(name=f"{self.save_dir}/{self.cell_name}", exist_ok=True)

        nfets = []
        pfets = []
        fet_names = []
        start_fet = False

        ext_pins = set()

        for line in lines_fetorder:
            if f"Solution {solution_num}" in line:
                start_fet = True
                continue

            elif "MOS" in line:
                l = line.strip()
                l = l.replace("[", "")
                l = l.replace("]", "")                
                n_split, p_split = l.split(", ")
                list_n = n_split.split(" ")
                list_p = p_split.split(" ")

                n_name_fin = list_n[2]
                n_name_fin = n_name_fin.replace(")", "")
                name, fin = n_name_fin.split("(")
                d_n, g_n, s_n = list_n[3:6]

                if not is_int(d_n):
                    ext_pins.add(d_n)
                if not is_int(g_n):
                    ext_pins.add(g_n)
                if not is_int(s_n):
                    ext_pins.add(s_n)

                nfets.append([name, fin, d_n, g_n, s_n])

                p_name_fin = list_p[2]
                p_name_fin = p_name_fin.replace(")", "")
                name, fin = p_name_fin.split("(")
                d_p, g_p, s_p = list_p[3:6]

                if not is_int(d_p):
                    ext_pins.add(d_p)
                if not is_int(g_p):
                    ext_pins.add(g_p)
                if not is_int(s_p):
                    ext_pins.add(s_p)

                pfets.append([name, fin, d_p, g_p, s_p])

            elif f"Solution {solution_num+1}" in line:
                start_fet = False
                break

        #fets_info, ext_pins = self.get_fet_info()

        ext_pins = list(ext_pins)

        fets = {N_TYPE: [], P_TYPE: []}
        fet_name_to_fet = {}
        fet_names = []

        for _fet_type in [N_TYPE, P_TYPE]:
            _fets = nfets if _fet_type == N_TYPE else pfets
            
            for fet in _fets:
                fet_idx = 0
                name, fins, d, g, s = fet

                if name == "dummy":
                    fet = MOSFET(
                        name=DUMMY_NAME, 
                        drain=DUMMY_NET, 
                        gate=DUMMY_NET, 
                        source=DUMMY_NET, 
                        body=DUMMY_NET, 
                        type=DUMMY_NET, 
                        options={"NFIN": 0}
                    )

                else:
                    #_, _, _, b, fet_type, options = fets_info[name]
                    b = "VDD" if _fet_type == P_TYPE else "VSS"
                    fet_type = "PMOS" if _fet_type == P_TYPE else "NMOS"

                    options = dict()
                    options["NFIN"] = int(fins)
                    options["W"] = str(int(fins) * design_rules["pitch"]["fin"]) + "n"

                    while f"{name}_{fet_idx}" in fet_names:
                        fet_idx += 1
                        
                    fet_name = name + "_" + str(fet_idx)
                    fet_names.append(fet_name)
                    
                    fet = MOSFET(fet_name, d, g, s, b, fet_type, options)
                    fet_name_to_fet[fet_name] = fet

                fets[_fet_type].append(fet)
                
        self.fets = fets
        self.ext_pins = ext_pins
        self.fet_name_to_fet = fet_name_to_fet



    def run_dynamic_programming(self, solution_num):
        cell_name = self.cell_name
        file_name = self.file_name

        placement_file_name = f"{self.save_dir}/{cell_name}/{cell_name}.txt"

        if not os.path.exists(f"{placement_file_name}"):
            ### Generating .style file with INIT ###
            rf = open("./DP-placer/DATA/input/placement_file.style", "r")
            wf = open("./DP-placer/DATA/input/Au-MEDAL.style", "w+")
            
            diffusion_break = design_rules["diffusion_break"]
            num_max_nmos_fins = design_rules["num_max_nmos_fins"]
            num_max_pmos_fins = design_rules["num_max_pmos_fins"]
 
            lines = rf.readlines()
 
            for line in lines:
                if "NET_DIFF_GAP" in line:
                    wf.write(f"NET_DIFF_GAP {diffusion_break}:\n")
                elif "NMOS_MAX_FIN" in line:
                    wf.write(f"NMOS_MAX_FIN {num_max_nmos_fins}\n")
                elif "PMOS_MAX_FIN" in line:
                    wf.write(f"PMOS_MAX_FIN {num_max_pmos_fins}\n")
                #elif "RELAXATION" in line:
                #    wf.write("RELAXATION 5")
                elif "AVOID_GATECUT" in line:
                    wf.write("AVOID_GATECUT true")
                elif "XC_NUM" in line:
                    wf.write("XC_NUM 3")
                else:
                    wf.write(line)
 
            rf.close()
            wf.close()

            #os.system("echo " + f"./DP-placer/MAKE/PLACE/Au-MEDAL.run_csyn_fp {self.file_name} {cell_name}")
            #os.system(f"./DP-placer/MAKE/PLACE/Au-MEDAL.run_csyn_fp {file_name} {cell_name} > /dev/null 2>&1")
            os.system(f"./DP-placer/MAKE/PLACE/Au-MEDAL.run_csyn_fp {file_name} {cell_name}")
            os.system(f"rm output.txt")


            for filename in os.listdir("./DP-placer/MAKE/PLACE/output/placement"):
                if fnmatch.fnmatch(filename, f"{cell_name}_w*.txt"): # change widths
                    os.makedirs(name=f"{self.save_dir}/{cell_name}", exist_ok=True)
                    move_file = f"mv ./DP-placer/MAKE/PLACE/output/placement/{filename} {placement_file_name}"
                    os.system(move_file)

        self.read_placement(placement_file=placement_file_name, solution_num=solution_num)

    def get_cell_name(self):
        return self.cell_name

    def get_fets(self):
        return self.fets

    def get_each_fets(self):
        fets = self.fets

        nfets = fets[N_TYPE]
        pfets = fets[P_TYPE]

        return nfets, pfets

    def get_num_fets(self):
        fets = self.fets

        nfets = fets[N_TYPE]
        pfets = fets[P_TYPE]

        return len(nfets)+len(pfets)

    def get_ext_pins(self):
        return self.ext_pins
    

    def set_fet_order(self, place_result):
        pfet_order = []
        nfet_order = []
        list_mos = []
        read = False

        lines = place_result.readlines()

        for line in lines:
            if "mos" in line:
                read = True
                continue
            elif "end" in line:
                read = False
                break
            elif read == True:
                sp = line.split(" ")
                list_mos.append(sp)

        list_mos[0].pop()
        list_mos[1].pop()
        pmos = list_mos[0]
        nmos = list_mos[1]

        for p in pmos:
            if p == "NULL":
                pfet_order.append(MOSFET(name=DUMMY_NAME, drain=DUMMY_NET, gate=DUMMY_NET, source=DUMMY_NET, body=DUMMY_NET, type=DUMMY_NET, options={"NFIN": 0}))
                continue
            elif "f" in p:
                fingers = p.split("_f")
                fingers[1] = str(int(fingers[1]) - 1)
                name_p = "_".join(fingers)
            else:
                name_p = p + "_0"
            pfet_order.append(self.fet_name_to_fet[name_p])

        for n in nmos:
            if n == "NULL":
                nfet_order.append(MOSFET(name=DUMMY_NAME, drain=DUMMY_NET, gate=DUMMY_NET, source=DUMMY_NET, body=DUMMY_NET, type=DUMMY_NET, options={"NFIN": 0}))
                continue
            elif "f" in n:
                fingers = n.split("_f")
                fingers[1] = str(int(fingers[1]) - 1)
                name_n = "_".join(fingers)
            else:
                name_n = n + "_0"
            nfet_order.append(self.fet_name_to_fet[name_n])

        return pfet_order, nfet_order
