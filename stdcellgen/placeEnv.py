import copy
import numpy as np

from .vars import *
from .structure import *


class PlaceEnv():
    def __init__(self, circuit):
        self.circuit = circuit
        self.save_dir = circuit.save_dir
        self.cell_name = circuit.get_cell_name()  
        self.fet_orders = copy.deepcopy(circuit.get_fets())
        self.fet_name_to_index = circuit.fet_name_to_index
        self.ext_pins = circuit.get_ext_pins()

        self.single_net_orders = None 
        self.double_net_orders = None

        self.num_fins = None
        self.double_num_fins = None

        self.nets = None
        self.grid_graph = None
        
        num_fet_gate = {N_TYPE: 0, P_TYPE: 0}
        fet_width = {N_TYPE: 0, P_TYPE: 0}
        
        for fet_type in [P_TYPE, N_TYPE]:
            for i in range(len(self.fet_orders[fet_type])):
                fet = self.fet_orders[fet_type][i]
                if fet != DUMMY_NAME:
                    num_fet_gate[fet_type] += fet.get_num_finger()
                    fet_width[fet_type] += 2*fet.get_num_finger()+2

        self.max_gatecut = min(num_fet_gate[N_TYPE], num_fet_gate[P_TYPE])
        self.max_width = fet_width[N_TYPE] + fet_width[P_TYPE]

    def __setstate__(self, state):
        self.__dict__.update(state)
        
        self.single_net_orders = None
        self.double_net_orders = None

        self.set_net_order()
        self.set_net_points()

    def print_order(self, name_list1, name_list2):
        for i in range(len(name_list1)):
            name1 = name_list1[i]
            name2 = name_list2[i]
            name = name1
            if len(name1) < len(name2):
                size = len(name2) - len(name1)
                name += " " * size
            print(name, end="  ")
        print()

    def print_single_net_order(self):
        single_pnet_order, single_nnet_order = self.single_net_orders[P_TYPE], self.single_net_orders[N_TYPE]

        print("single_pnet_order ", end="")
        self.print_order(single_pnet_order, single_nnet_order)
        
        print("single_nnet_order ", end="")
        self.print_order(single_nnet_order, single_pnet_order)

        print()

    def print_fet_order(self):
        pfet_order, nfet_order = self.get_fet_orders()
        for fet in pfet_order:
            print(fet, end=" ")
        print()
        for fet in nfet_order:
            print(fet, end=" ")
        print()

    def get_circuit(self):
        return self.circuit

    def get_fet_orders(self):
        fet_orders = self.fet_orders

        return fet_orders[P_TYPE], fet_orders[N_TYPE]
    
    def get_ext_pins(self):
        return self.ext_pins
    
    def get_double_net_orders(self):
        double_net_orders = self.double_net_orders

        return double_net_orders[P_TYPE], double_net_orders[N_TYPE]
    
    def get_single_net_orders(self):
        single_net_orders = self.single_net_orders

        return single_net_orders[P_TYPE], single_net_orders[N_TYPE]
    
    def get_double_num_fins(self):
        double_num_fins = self.double_num_fins
        
        return double_num_fins[P_TYPE], double_num_fins[N_TYPE]

    def get_num_poly(self):
        pnet_order, nnet_order = self.double_net_orders[P_TYPE][0], self.double_net_orders[N_TYPE][0]
        assert len(pnet_order) == len(nnet_order), \
            f"Error: The lengths of 'pnet_order' and 'nnet_order' do not match. \
                pnet_order has length {len(pnet_order)}, while nnet_order has length {len(nnet_order)}."
                
        return int((len(pnet_order)+1) / 2)


    def get_width(self):
        assert len(self.double_net_orders[N_TYPE][0]) == len(self.double_net_orders[P_TYPE][0]), \
            "net_orders[N_TYPE]'s length and net_orders[P_TYPE]'s length are not same."
            
        return len(self.double_net_orders[N_TYPE][0])

    def get_nets(self):
        return self.nets
    
    def set_fet_orders(self, pfet_order, nfet_order):
        self.fet_orders[P_TYPE] = pfet_order
        self.fet_orders[N_TYPE] = nfet_order

        self.single_net_orders = None
        self.double_net_orders = None

        self.set_net_order()
        self.set_net_points()


    def set_net_order(self):
        def flattening(fet_order):
            fet_order = copy.deepcopy(fet_order)
            dummy_cell = MOSFET(name=DUMMY_NAME, drain=DUMMY_NET, gate=DUMMY_NET, source=DUMMY_NET, body=DUMMY_NET, type=DUMMY_NET, options={"NFIN": 0})

            num_fet_diff = len(fet_order[N_TYPE]) - len(fet_order[P_TYPE])
            
            if num_fet_diff > 0:
                fet_order[P_TYPE] += [dummy_cell] * num_fet_diff
            else:
                fet_order[N_TYPE] += [dummy_cell] * abs(num_fet_diff)

            return fet_order
        
        def strip_dummy(order, num_fins = None):
            order = copy.deepcopy(order)

            first_index = {N_TYPE: 0, P_TYPE: 0}
            first_assign = {N_TYPE: False, P_TYPE: False}

            last_index = {N_TYPE: len(order[N_TYPE])-1, P_TYPE: len(order[P_TYPE])-1}

            for fet_type in [P_TYPE, N_TYPE]:
                for i, fet in enumerate(order[fet_type]):
                    if not (fet == DUMMY_NAME or fet == DUMMY_NET):
                        if first_index[fet_type] == 0 and first_assign[fet_type] == False:
                            first_assign[fet_type] = True
                            first_index[fet_type] = i
                        last_index[fet_type] = i

            first_fet_index_value = min(first_index[N_TYPE], first_index[P_TYPE])
            last_fet_index_value = max(last_index[N_TYPE], last_index[P_TYPE])
            
            for fet_type in [P_TYPE, N_TYPE]:
                order[fet_type] = order[fet_type][first_fet_index_value:last_fet_index_value + 1]
                if num_fins:
                    num_fins[fet_type] = num_fins[fet_type][first_fet_index_value:last_fet_index_value + 1]
            
            return order, num_fins

        def add_net_point(net_order, num_fins, fet, diffusion_sharing):
            num_fins_of_fet = fet.get_num_fins()

            if len(net_order) == 0:
                net_order.append(fet.get_drain())
                num_fins.append(num_fins_of_fet)
                
            else:
                if diffusion_sharing:
                    if fet.get_drain() != DUMMY_NET:
                        net_order[-1] = fet.get_drain()
                        num_fins[-1] = max(num_fins_of_fet, num_fins[-1])
                    
                else:
                    net_order.append(DUMMY_NET)
                    num_fins.append(0)
                        
                    net_order.append(fet.get_drain())
                    num_fins.append(num_fins_of_fet)
                
            net_order.append(fet.get_gate())
            num_fins.append(num_fins_of_fet)
            net_order.append(fet.get_source())
            num_fins.append(num_fins_of_fet)
        
        
        def add_diffusion_break(nnet_order, pnet_order, num_nfins, num_pfins):
            assert len(nnet_order) == len(pnet_order), \
                f"Length mismatch: nnet_order has {len(nnet_order)} elements, but pnet_order has {len(pnet_order)} elements."            
            
            i = 2
            while i < len(nnet_order) - 2:
                prev_nnet, curr_nnet, next_nnet = nnet_order[i - 1], nnet_order[i], nnet_order[i + 1]
                prev_pnet, curr_pnet, next_pnet = pnet_order[i - 1], pnet_order[i], pnet_order[i + 1]

                diff_net_add_dummy = (
                    prev_nnet != DUMMY_NET and curr_nnet == DUMMY_NET and next_nnet != DUMMY_NET and prev_nnet != next_nnet
                ) or (
                    prev_pnet != DUMMY_NET and curr_pnet == DUMMY_NET and next_pnet != DUMMY_NET and prev_pnet != next_pnet
                )

                same_net_add_dummy = (
                    prev_nnet != DUMMY_NET and curr_nnet == DUMMY_NET and next_nnet != DUMMY_NET and prev_nnet == next_nnet
                ) or (
                    prev_pnet != DUMMY_NET and curr_pnet == DUMMY_NET and next_pnet != DUMMY_NET and prev_pnet == next_pnet
                )

                if diff_net_add_dummy or same_net_add_dummy:
                    add_num = 2 * (int(design_rules["diffusion_break"]) - 1) if diff_net_add_dummy else 2 * (- 1)

                    for _ in range(add_num):
                        nnet_order.insert(i, DUMMY_NET)
                        pnet_order.insert(i, DUMMY_NET)
                        num_nfins.insert(i, 0)
                        num_pfins.insert(i, 0)
                
                i += 1
        
        fet_orders = self.fet_orders
        fet_orders, _ = strip_dummy(fet_orders)
        fet_orders = flattening(fet_orders)
    
        net_orders = {N_TYPE: list(), P_TYPE: list()}
        num_fins = {N_TYPE: list(), P_TYPE: list()}

        num_nfet = len(fet_orders[N_TYPE])
        num_pfet = len(fet_orders[P_TYPE])
  
        assert num_nfet == num_pfet, f"Mismatch in FET counts: num_nfet ({num_nfet}) != num_pfet ({num_pfet})"
        # nfet, pfet order length are always same.
        num_fet = num_nfet

        for i in range(num_fet):
            nfet = fet_orders[N_TYPE][i]
            pfet = fet_orders[P_TYPE][i]
            
            n_diffusion_sharing = False
            if len(net_orders[N_TYPE]) > 1:
                last_n_type_net = net_orders[N_TYPE][-1]
                last_num_nfin = num_fins[N_TYPE][-1]
                
                n_drain = nfet.get_drain()
                if last_n_type_net == n_drain or last_n_type_net == DUMMY_NET or n_drain == DUMMY_NET:
                    n_diffusion_sharing = True

            p_diffusion_sharing = False
            if len(net_orders[P_TYPE]) > 1:
                last_p_type_net = net_orders[P_TYPE][-1]
                last_num_pfin = num_fins[P_TYPE][-1]

                p_drain = pfet.get_drain()
                if last_p_type_net == p_drain or last_p_type_net == DUMMY_NET or p_drain == DUMMY_NET:
                    p_diffusion_sharing = True

            diffusion_sharing = (n_diffusion_sharing and p_diffusion_sharing)
            
            add_net_point(net_orders[N_TYPE], num_fins[N_TYPE], nfet, diffusion_sharing)
            add_net_point(net_orders[P_TYPE], num_fins[P_TYPE], pfet, diffusion_sharing)

        add_diffusion_break(net_orders[N_TYPE], net_orders[P_TYPE], num_fins[N_TYPE], num_fins[P_TYPE])
        
        net_orders, num_fins = strip_dummy(net_orders, num_fins)
        
        # Add DUMMY_NET at the start and end of the net orders
        for net_order in net_orders.values():
            net_order.insert(0, DUMMY_NET)
            net_order.append(DUMMY_NET)
            
        for num_fin_of_x in num_fins.values():
            num_fin_of_x.insert(0, 0)
            num_fin_of_x.append(0)
            
        assert len(net_orders[P_TYPE]) % 2 == 1 and len(net_orders[N_TYPE]) % 2 == 1, \
        f"Both net_orders lists must have an odd length, but got lengths: {net_orders[P_TYPE]} and {net_orders[N_TYPE]}"
    
        self.single_net_orders = net_orders
        self.num_fins = num_fins    
        
        self.set_double_net_orders()


    def set_double_net_orders(self):
        num_row = design_rules["num_row"]
        
        net_orders = {fet_type: self.single_net_orders[fet_type][:] for fet_type in [P_TYPE, N_TYPE]}
        num_fins = {fet_type: self.num_fins[fet_type][:] for fet_type in [P_TYPE, N_TYPE]}

        if num_row < 2:
            self.double_net_orders = {fet_type: [self.single_net_orders[fet_type]] for fet_type in [P_TYPE, N_TYPE]}
            self.double_num_fins = {fet_type: [self.num_fins[fet_type]] for fet_type in [P_TYPE, N_TYPE]}
            return

        assert len(net_orders[N_TYPE]) % num_row == 1 and len(net_orders[P_TYPE]) % num_row == 1, \
            "The width does not become even number."
    
        single_width = len(net_orders[N_TYPE])
        folding_point =  single_width // num_row

        if net_orders[N_TYPE][folding_point] == DUMMY_NET and net_orders[P_TYPE][folding_point] == DUMMY_NET:
            if (net_orders[N_TYPE][folding_point-1] == DUMMY_NET and net_orders[P_TYPE][folding_point-1] == DUMMY_NET and 
                net_orders[N_TYPE][folding_point+1] == DUMMY_NET and net_orders[P_TYPE][folding_point+1] == DUMMY_NET):
                
                for fet_type in [P_TYPE, N_TYPE]:
                    del net_orders[fet_type][folding_point-1:folding_point+2]  
                    del num_fins[fet_type][folding_point-1:folding_point+2]
                folding_point -= 1
                    
            else:
                for fet_type in [P_TYPE, N_TYPE]:
                    del net_orders[fet_type][folding_point]  
                    del num_fins[fet_type][folding_point]

        #if net_orders[N_TYPE][folding_point] != DUMMY_NET or net_orders[P_TYPE][folding_point] != DUMMY_NET:
        else:
            for fet_type in [P_TYPE, N_TYPE]:
                if net_orders[fet_type][folding_point+1] == DUMMY_NET:
                    net_orders[fet_type].insert(folding_point+1, DUMMY_NET)
                    num_fins[fet_type].insert(folding_point+1, 0)
                
                elif net_orders[fet_type][folding_point-1] == DUMMY_NET:
                    net_orders[fet_type].insert(folding_point-1, DUMMY_NET)
                    num_fins[fet_type].insert(folding_point-1, 0)
                
                else:
                    net_orders[fet_type].insert(folding_point+1, net_orders[fet_type][folding_point])
                    num_fins[fet_type].insert(folding_point+1, num_fins[fet_type][folding_point])
            
            folding_point += 1

        double_net_orders = {N_TYPE: list(), P_TYPE: list()}
        double_num_fins = {N_TYPE: list(), P_TYPE: list()}
        
        for row in range(num_row):
            from_index = row*folding_point
            to_index = (row+1)*folding_point
            
            single_rows = {fet_type: net_orders[fet_type][from_index:to_index] for fet_type in [P_TYPE, N_TYPE]}
            single_num_fins = {fet_type: num_fins[fet_type][from_index:to_index] for fet_type in [P_TYPE, N_TYPE]}

            for fet_type in [P_TYPE, N_TYPE]:
                if row % 2 == 1:
                    #### (n1 n2 n3)
                    if row != num_row-1:
                        single_rows[fet_type].append(DUMMY_NET)
                        single_num_fins[fet_type].append(0)
                    #### (n1 n2 n3 --)

                    single_rows[fet_type] = single_rows[fet_type][::-1]
                    single_num_fins[fet_type] = single_num_fins[fet_type][::-1]
                    #### (-- n3 n2 n1 --)

                elif row % 2 == 0:
                    #### (n1 n2 n3)
                    if row != 0:
                        single_rows[fet_type].insert(0, DUMMY_NET)
                        single_num_fins[fet_type].insert(0, 0)
                    #### (-- n1 n2 n3)
                
                single_rows[fet_type].append(DUMMY_NET)
                single_num_fins[fet_type].append(0)
                
                double_net_orders[fet_type].append(single_rows[fet_type])
                double_num_fins[fet_type].append(single_num_fins[fet_type])
        
        self.double_net_orders = double_net_orders
        self.double_num_fins = double_num_fins
        

    def set_net_points(self):
        def add_points(nets, net_name, num_fin_of_x, x, min_y, max_y, layer, is_flip): 
            def add_pin_to_net(nets, net_name, term, x, ys, layer, pn_flip, gnd_rail, power_rail):                
                def create_power_gnd_net_name(net_name, nets):
                    i = 0
                    while f"{net_name}_{i}" in nets:
                        i += 1
                    return f"{net_name}_{i}"

                def add_power_pin(net, term, x, y_rail):
                    for power_layer in design_rules["power_layer"]:
                        pin = Pin(net, term)
                        pin.add_point(x, y_rail, layers.index(power_layer))
                        net.add_pin(pin)

                is_power = (net_name == POWER_NET)
                is_gnd = (net_name == GND_NET)
                
                if is_power or is_gnd:
                    net_name = create_power_gnd_net_name(net_name, nets)
                    
                net = nets.setdefault(net_name, Net(net_name, is_ext_pin=(net_name in self.ext_pins)))
                pin = Pin(net, term)
                
                for y in ys:
                    pin.add_point(x, y, layer)
                
                net.add_pin(pin)
                
                if (is_power and not pn_flip) or (is_gnd and pn_flip):
                    add_power_pin(net, term, x, power_rail)
                    
                elif (is_gnd and not pn_flip) or (is_power and pn_flip):
                    add_power_pin(net, term, x, gnd_rail)

            x_unit = int(design_rules["x_unit"])
            
            term = GATE if x % 2 == 0 else SOURCE | DRAIN
            x = int(design_rules["x_offset"] + x * x_unit)

            ys = self.y_points[layer]

            if term == GATE:
                max_routing_y = int(max_y - design_rules["width"]["M1"] - design_rules["spacing"]["S2S"]["M1"]["M1"] - design_rules["y_offset"])
                min_routing_y = int(min_y + design_rules["width"]["M1"] + design_rules["spacing"]["S2S"]["M1"]["M1"] + design_rules["y_offset"])

                if net_name[N_TYPE] == net_name[P_TYPE] or (net_name[N_TYPE] == DUMMY_NET or net_name[P_TYPE] == DUMMY_NET):
                    name = net_name[N_TYPE] if net_name[N_TYPE] != DUMMY_NET else net_name[P_TYPE]
                    gate_ys = [y for y in ys if min_routing_y <= y <= max_routing_y]
                    add_pin_to_net(nets=nets, net_name=name, term=term, x=x, ys=gate_ys, layer=layer, pn_flip=is_flip, gnd_rail=min_y, power_rail=max_y)
                
                else:
                    if design_rules["contact_over_active_gate"] == False:
                        open(f"{self.save_dir}/{self.cell_name}/NO_GATE_MATCHING.txt", "w").close()
                        raise ValueError("If it is not a contact_over_active_gate, the gate cannot be placed over the active region.")
                    
                    nmos_ys = [y for y in ys if min_routing_y <= y < min_y + design_rules["cell_height"] / 2 - design_rules["width"]["GCut"] / 2]
                    add_pin_to_net(nets=nets, net_name=net_name[N_TYPE], term=term, x=x, ys=nmos_ys, layer=layer, pn_flip=is_flip, gnd_rail=min_y, power_rail=max_y)
                    
                    pmos_ys = [y for y in ys if min_y + design_rules["cell_height"] / 2 + design_rules["width"]["GCut"] / 2 < y <= max_routing_y]
                    add_pin_to_net(nets=nets, net_name=net_name[P_TYPE], term=term, x=x, ys=pmos_ys, layer=layer, pn_flip=is_flip, gnd_rail=min_y, power_rail=max_y)

            elif term == SOURCE | DRAIN:
                active_metal_width = design_rules["width"][design_rules["active_contact_layer"]]
                y_offset = design_rules["pitch"]["M1"] - design_rules["width"]["M1"]/2
                
                if net_name[N_TYPE] != DUMMY_NET:
                    n_active_length = design_rules["pitch"]["fin"] * num_fin_of_x[N_TYPE]
                    n_active_min = y_offset
                    n_active_max = n_active_min + n_active_length
                    
                    nmos_ys = list()
                    for y in ys:
                        metal_max = y + active_metal_width/2
                        metal_min = y - active_metal_width/2
                                                
                        if not ((metal_max < n_active_min) or (metal_min > n_active_max)):
                            nmos_ys.append(y)
                    
                    add_pin_to_net(nets=nets, net_name=net_name[N_TYPE], term=term, x=x, ys=nmos_ys, layer=layer, pn_flip=is_flip, gnd_rail=min_y, power_rail=max_y)
                
                if net_name[P_TYPE] != DUMMY_NET:     
                    p_active_length = design_rules["pitch"]["fin"] * num_fin_of_x[P_TYPE]
                    p_active_max = design_rules["cell_height"] - y_offset
                    p_active_min = p_active_max - p_active_length

                    pmos_ys = list()
                    for y in ys:
                        metal_max = y + active_metal_width/2
                        metal_min = y - active_metal_width/2
                                                
                        if not ((metal_max < p_active_min) or (metal_min > p_active_max)):
                            pmos_ys.append(y)
                    
                    add_pin_to_net(nets=nets, net_name=net_name[P_TYPE], term=term, x=x, ys=pmos_ys, layer=layer, pn_flip=is_flip, gnd_rail=min_y, power_rail=max_y)


        nets = {}
        layers = design_rules["routing_layers"]
        cell_height = design_rules["cell_height"]
        num_row = int(design_rules["num_row"])
        
        double_net_orders = {fet_type: self.double_net_orders[fet_type] for fet_type in [P_TYPE, N_TYPE]}
        double_num_fins = {fet_type: self.double_num_fins[fet_type] for fet_type in [P_TYPE, N_TYPE]}
        
        assert len(double_net_orders[N_TYPE]) == len(double_net_orders[P_TYPE]), \
            "Mismatch in length between N_TYPE and P_TYPE in double_net_orders."        

        self.x_points = self.get_x_points()
        self.y_points = self.get_y_points()

        for i in range(num_row):
            pn_flip = (i % 2 == 1)
            
            if not pn_flip:
                net_orders = {N_TYPE: double_net_orders[N_TYPE][i], P_TYPE: double_net_orders[P_TYPE][i]}
                num_fins = {N_TYPE: double_num_fins[N_TYPE][i], P_TYPE: double_num_fins[P_TYPE][i]}
            else:
                net_orders = {N_TYPE: double_net_orders[P_TYPE][i], P_TYPE: double_net_orders[N_TYPE][i]}
                num_fins = {N_TYPE: double_num_fins[P_TYPE][i], P_TYPE: double_num_fins[N_TYPE][i]}
                
            min_y = i*cell_height 
            max_y = (i+1)*cell_height 

            assert len(net_orders[N_TYPE]) == len(net_orders[P_TYPE]), \
                f"Length mismatch: net_orders[N_TYPE] ({len(net_orders[N_TYPE])}) and net_orders[P_TYPE] ({len(net_orders[P_TYPE])})"
                
            for x in range(len(net_orders[N_TYPE])):
                net_name = {N_TYPE: net_orders[N_TYPE][x], P_TYPE: net_orders[P_TYPE][x]}
                num_fin_of_x = {N_TYPE: num_fins[N_TYPE][x], P_TYPE: num_fins[P_TYPE][x]}
                layer =  layers.index(design_rules["active_contact_layer"]) if x % 2 else layers.index(design_rules["gate_contact_layer"])
                add_points(nets, net_name, num_fin_of_x, x, min_y, max_y, layer, pn_flip)
        

        self.nets = list(nets.values())

    def get_x_points(self):
        width = self.get_width()
        max_x = int(width * design_rules["x_unit"])
        num_layers = len(design_rules["routing_layers"])
        x_points = []

        for layer_num in range(num_layers):
            resolution = design_rules["x_routing_resolution"][layer_num]
            layer_range = list(range(int(width * resolution)))
            x_unit = design_rules["x_unit"] / resolution
            
            remove_x_index = set()
            for x in layer_range:
                if layer_num == design_rules["routing_layers"].index(design_rules["gate_contact_layer"]):
                    remove_x_index.add(0)
                    remove_x_index.add(len(layer_range)-1)
            
            layer_range = [v for i, v in enumerate(layer_range) if i not in remove_x_index]

            points = [int(design_rules["x_offset"] + x * x_unit) for x in layer_range]
            points = [x for x in points if x <= max_x]

            x_points.append(points)

        return x_points
        
    def get_y_points(self, allow_below_min_track=False):
        cell_height = design_rules["cell_height"]
        num_row = int(design_rules["num_row"])
        num_layers = len(design_rules["routing_layers"])
        
        y_points = [[] for _ in range(num_layers)]
        only_ver_metal_y_points = [[] for _ in range(num_layers)]
        
        y_offset = design_rules["y_offset"]
        
        for row in range(num_row):
            max_y = (row+1) * cell_height - design_rules["y_unit"]
            min_y = row * cell_height + design_rules["y_unit"]
            mid_y = min_y + (max_y-min_y)/2
            
            is_flip = (row % 2 == 1)
            gate_contact_y = int(mid_y + (design_rules["np_offset"] if not is_flip else -design_rules["np_offset"]))
            
            for layer_num in range(num_layers):
                y_unit = design_rules["y_unit"] / design_rules["y_routing_resolution"][layer_num]
                
                for y in np.arange(min_y, gate_contact_y, y_unit):
                    y = y + y_offset
                    if not allow_below_min_track:
                        if y_unit <= abs(gate_contact_y - y):
                            y_points[layer_num].append(int(y))
                    else:
                        y_points[layer_num].append(int(y))
                    
                for y in np.arange(max_y, gate_contact_y, -y_unit):
                    y = y - y_offset
                    if not allow_below_min_track:
                        if y_unit <= abs(gate_contact_y - y):
                            y_points[layer_num].append(int(y))
                    else:
                        y_points[layer_num].append(int(y))
            
                y_points[layer_num].append(gate_contact_y)
            
            ext_pin_layer = design_rules["ext_pin_layer"][-1]
            ext_pin_layers = design_rules["same_height_layers"][ext_pin_layer]

            ext_pin_layer_nums = [design_rules["routing_layers"].index(ext_pin_layer) for ext_pin_layer in ext_pin_layers]
            ext_pin_layer_width = design_rules["width"][ext_pin_layer]

            y_offset_for_ext_pin = round((design_rules["minimum_pin_length"] - ext_pin_layer_width) / 2)

            if design_rules["addition_y_grid_for_pin"]:
                upper_layers = design_rules["upper_layers"][ext_pin_layer]
                lower_layers = design_rules["lower_layers"][ext_pin_layer]

                upper_layer_nums = [design_rules["routing_layers"].index(layer) for layer in upper_layers if layer in design_rules["routing_layers"]]
                lower_layer_nums = [design_rules["routing_layers"].index(layer) for layer in lower_layers if layer in design_rules["routing_layers"]]

                gate_contact_layer = design_rules["routing_layers"].index(design_rules["gate_contact_layer"])

                candidate_layer_nums = ext_pin_layer_nums
                if 1 <= design_rules["complexity_level"]:
                    if gate_contact_layer in lower_layer_nums:
                        candidate_layer_nums += [gate_contact_layer]
                    candidate_layer_nums += lower_layer_nums

                if 2 <= design_rules["complexity_level"]:
                    candidate_layer_nums += upper_layer_nums
                
                for layer in candidate_layer_nums:
                    only_ver_metal_y_points[layer].append(gate_contact_y-y_offset_for_ext_pin)
                    only_ver_metal_y_points[layer].append(gate_contact_y+y_offset_for_ext_pin)
                    y_points[layer].extend(only_ver_metal_y_points[layer])
            
            for layer_num in range(num_layers):
                if layer_num == design_rules["routing_layers"].index(design_rules["active_contact_layer"]):
                    if 1 <= design_rules["complexity_level"]:
                        if design_rules["num_max_nmos_fins"] == 3:
                            additional_track = design_rules["power_width"]["M1"]/2 + design_rules["spacing"]["S2S"]["M1"]["M1"] + design_rules["pitch"]["fin"] * 3 - design_rules["width"]["LISD"]/2
                            y_points[layer_num].append(int(additional_track))
                        if design_rules["num_max_pmos_fins"] == 3:
                            additional_track = design_rules["cell_height"] - (design_rules["power_width"]["M1"]/2 + design_rules["spacing"]["S2S"]["M1"]["M1"] + design_rules["pitch"]["fin"] * 3 - design_rules["width"]["LISD"]/2)
                            y_points[layer_num].append(int(additional_track))

                y_points[layer_num] = sorted(list(set(y_points[layer_num])))
                only_ver_metal_y_points[layer_num] = sorted(list(set(only_ver_metal_y_points[layer_num])))

        self.only_ver_metal_y_points = only_ver_metal_y_points
        return y_points
    
