import time
from collections import defaultdict
import sys
import networkx as nx
import copy
import os

from .structure import *
from .logic import *


class Router():
    def __init__(self, placement, pre_layout):
        z3.set_param('model', True)
    
        self.pre_layout = pre_layout
        self.placement = placement
        self.cell_name = placement.cell_name
        self.save_dir = placement.save_dir

        self.nets = placement.get_nets()

        self.x_points = placement.x_points
        self.y_points = placement.y_points
        self.only_ver_metal_y_points = placement.only_ver_metal_y_points
        
        self.placement_net_boundary = dict()
        self.global_processes_vars = set()

        self.var_info = dict()

        self.metal_point_vars = None
        self.metal_edge_vars = None
        
        self.net_point_vars = None
        
        self.net_hor_point_vars = None
        self.net_ver_point_vars = None
        
        self.net_edge_vars = None

        self.solver = None

        self.net_com_point_vars = None
        self.net_com_edge_vars = None
        
        self.side_point_vars = None
        self.tip_point_vars = None
        self.corner_point_vars = None
        
        self.metals = None
        
        self.hor_lower_via_enc = False
        self.hor_upper_via_enc = False

        self.ver_lower_via_enc = False
        self.ver_upper_via_enc = False
        
        self.external_pin_point = False


    def get_routing_graph(self):
        layers = design_rules["routing_layers"]        
        x_points, y_points, only_ver_metal_y_points = self.x_points, self.y_points, self.only_ver_metal_y_points
        
        directions = design_rules["routing_directions"]
        graph = nx.Graph()

        for curr_z, layer in enumerate(layers):
            direction = directions[curr_z]
            for x_index in range(len(x_points[curr_z])):
                curr_x = x_points[curr_z][x_index]
                for y_index in range(len(y_points[curr_z])):
                    curr_y = y_points[curr_z][y_index]

                    is_for_ext_pin = (curr_y in only_ver_metal_y_points[curr_z])

                    x_ex = design_rules["extension"][layer] if direction == HORIZONTAL else design_rules["width"][layer]
                    y_ex = design_rules["extension"][layer] if direction == VERTICAL else design_rules["width"][layer]              
                    
                    curr_over_active = self.pre_layout.is_overlap(curr_x, curr_y, x_ex, y_ex, "Active")
                    is_power_rail = (layer in design_rules["power_layer"]) and (curr_y % int(design_rules["cell_height"]) == 0)

                    if layer == design_rules["gate_contact_layer"] and \
                       design_rules["contact_over_active_gate"] == False and \
                       curr_over_active:
                        continue

                    if x_index > 0:
                        if (direction in [HORIZONTAL, BIDIRECTION] or is_power_rail):
                            prev_x = x_points[curr_z][x_index - 1]
                            prev_over_active = self.pre_layout.is_overlap(prev_x, curr_y, x_ex, y_ex, "Active")

                            x_add_edge = True
                            if layer == design_rules["gate_contact_layer"] and \
                                design_rules["contact_over_active_gate"] == False and \
                                prev_over_active:
                                    x_add_edge = False
                            
                            if layer == design_rules["active_contact_layer"]:
                                prev_over_active = self.pre_layout.is_overlap(prev_x, curr_y, x_ex, y_ex, "Active")
                                if curr_over_active and prev_over_active:
                                    x_add_edge = False

                            if design_rules["complexity_level"] < 1:
                                if is_for_ext_pin:
                                    x_add_edge = False

                            if x_add_edge:                                
                                graph.add_edge((prev_x, curr_y, curr_z), (curr_x, curr_y, curr_z))


                    if y_index > 0:
                        if (direction in [VERTICAL, BIDIRECTION]):
                            prev_y = y_points[curr_z][y_index-1]
                            prev_over_active = self.pre_layout.is_overlap(curr_x, prev_y, x_ex, y_ex, "Active")
                            
                            y_add_edge = True
                            if layer == design_rules["gate_contact_layer"] and \
                                design_rules["contact_over_active_gate"] == False and \
                                prev_over_active:
                                    y_add_edge = False
                            
                            if y_add_edge:
                                graph.add_edge((curr_x, prev_y, curr_z), (curr_x, curr_y, curr_z))
                    

                    if curr_z > 0:
                        curr_layer = design_rules["routing_layers"][curr_z]
                        lower_layers = design_rules["lower_layers"][curr_layer]

                        if lower_layers != None:
                            for lower_layer in lower_layers:
                                prev_z = design_rules["routing_layers"].index(lower_layer)
                                z_add_edge = True

                                if curr_x not in x_points[prev_z] or curr_y not in y_points[prev_z]:
                                    z_add_edge = False

                                if (lower_layer == design_rules["gate_contact_layer"] and \
                                   design_rules["contact_over_active_gate"] == False and \
                                   curr_over_active):
                                    z_add_edge = False

                                if design_rules["complexity_level"] < 1:
                                    if is_for_ext_pin:
                                        z_add_edge = False
                        
                                if z_add_edge == True:
                                    graph.add_edge((curr_x, curr_y, prev_z), (curr_x, curr_y, curr_z))

        return graph



    def save_layer_images(self, output_dir):
        import os
        import matplotlib.pyplot as plt
        import networkx as nx
        from itertools import combinations

        if not os.path.exists(output_dir):
            os.makedirs(output_dir)

        layers = {}
        for node in self.routing_graph.nodes():
            layer = node[2]
            layers.setdefault(layer, []).append(node)

        scale = 100
        font_size = 5
        node_size = 100

        for layer in sorted(layers.keys()):
            fig, ax = plt.subplots(figsize=(8, 6))

            sub_nodes = layers[layer]
            sub_edges = [
                edge for edge in self.routing_graph.edges() 
                if edge[0][2] == layer and edge[1][2] == layer
            ]

            subgraph = nx.Graph()
            subgraph.add_nodes_from(sub_nodes)
            subgraph.add_edges_from(sub_edges)

            pos = {node: (node[0] * scale, node[1] * scale) for node in subgraph.nodes()}

            nx.draw(
                subgraph,
                pos,
                ax=ax,
                with_labels=False,
                node_color='skyblue',
                edge_color='gray',
                node_size=node_size
            )

            labels = {node: f"({node[0]}, {node[1]})" for node in subgraph.nodes()}
            pos_labels = {node: (pos[node][0], pos[node][1] - 1000) for node in subgraph.nodes()}
            nx.draw_networkx_labels(subgraph, pos_labels, labels, font_size=font_size, ax=ax)

            layer_name = design_rules["routing_layers"][int(layer)]
            ax.set_title(f'{layer_name}\'s possible routing tracks')
            ax.set_aspect('equal')

            x_coords = [coord[0] for coord in pos.values()]
            y_coords = [coord[1] for coord in pos.values()]
            margin = scale * 20 

            ax.set_xlim(min(x_coords) - margin, max(x_coords) + margin)
            ax.set_ylim(min(y_coords) - margin, max(y_coords) + margin)

            plt.tight_layout()
            output_file = os.path.join(output_dir, f"{layer_name}.png")
            plt.savefig(output_file, dpi=800, bbox_inches='tight', pad_inches=0.1)
            plt.close(fig)

        global_pos = {node: (node[0] * scale, node[1] * scale) for node in self.routing_graph.nodes()}
        layer_keys = sorted(layers.keys())

        for layer_a, layer_b in combinations(layer_keys, 2):
            nodes_a = set(layers[layer_a])
            nodes_b = set(layers[layer_b])
            inter_edges = [
                edge for edge in self.routing_graph.edges()
                if (edge[0] in nodes_a and edge[1] in nodes_b) or (edge[0] in nodes_b and edge[1] in nodes_a)
            ]
            nodes_in_inter_edges = set()
            for edge in inter_edges:
                nodes_in_inter_edges.add(edge[0])
                nodes_in_inter_edges.add(edge[1])

            if not nodes_in_inter_edges or not inter_edges:
                continue

            fig, ax = plt.subplots(figsize=(8, 6))
            nx.draw_networkx_nodes(
                self.routing_graph, global_pos, nodelist=list(nodes_in_inter_edges),
                ax=ax, node_color='white', node_size=node_size, edgecolors='black'
            )
            nx.draw_networkx_edges(
                self.routing_graph, global_pos,
                edgelist=inter_edges, ax=ax, edge_color='red', width=1
            )
            labels = {node: f"({node[0]}, {node[1]})" for node in nodes_in_inter_edges}
            pos_labels = {node: (global_pos[node][0], global_pos[node][1] - 1000) for node in nodes_in_inter_edges}
            nx.draw_networkx_labels(self.routing_graph, pos_labels, labels, font_size=font_size, ax=ax)

            layer_name_a = design_rules["routing_layers"][int(layer_a)]
            layer_name_b = design_rules["routing_layers"][int(layer_b)]
            ax.set_title(f'Possible via positions between {layer_name_a} and {layer_name_b}')
            ax.set_aspect('equal')

            x_coords = [global_pos[node][0] for node in nodes_in_inter_edges]
            y_coords = [global_pos[node][1] for node in nodes_in_inter_edges]
            margin = scale * 20
            ax.set_xlim(min(x_coords) - margin, max(x_coords) + margin)
            ax.set_ylim(min(y_coords) - margin, max(y_coords) + margin)

            plt.tight_layout()
            output_file = os.path.join(output_dir, f"{layer_name_a}-{layer_name_b}.png")
            plt.savefig(output_file, dpi=800, bbox_inches='tight', pad_inches=0.1)
            plt.close(fig)

    
    def get_metals(self):
        return self.metals

    def get_via_enc(self):
        return (self.hor_lower_via_enc, self.hor_upper_via_enc, self.ver_lower_via_enc, self.ver_upper_via_enc)
    
    def get_external_pin_point(self):
        return self.external_pin_point
    
    def to_smt2_file(self, file, solver):
        with open(file, "w") as f:
            f.write(solver.sexpr())
    
    def is_redundant_var(self, var):
        return var in self.global_processes_vars

    def extract_vars(self, expr):
        if z3.is_const(expr) and not (z3.is_true(expr) or z3.is_false(expr)):
            return {expr}
        variables = set()
        for child in expr.children():
            variables |= self.extract_vars(child) 
        return variables
        
    def add_constraint(self, constraint):
        if not constraint is z3.Bool(True) or constraint is not True:
            self.solver.add(constraint)

    def get_edge(self, var):
        info = self.var_info[var]
        u = info["u"]
        v = info["v"]
        
        return u, v
    
    def get_point_var(self, point, prefix=None, net=None):
        x, y, z = point
        
        condition_name = ""
        if prefix != None:
            condition_name += prefix+"_"
            
        condition_name += "G_"
        if net != None:
            condition_name += str(net)+"_"
            
        condition_name += "x"+str(x)
        condition_name += "y"+str(y)
        condition_name += "z"+str(z)

        condition_name = z3.Bool(condition_name)

        self.var_info[condition_name] = {'u': point, 'v': point, 'prefix': prefix, 'net': net}

        return condition_name


    def get_edge_var(self, u, v, prefix=None, net=None, commodity=None):
        x1, y1, curr_z = u
        x2, y2, next_z = v
        
        condition_name = ""
        if prefix != None:
            condition_name += prefix+"_"
        
        condition_name += "E_"
        if net != None:
            condition_name += str(net)+"_"

        if commodity != None:
            condition_name += "C"+str(commodity)+"_"
         
        condition_name += "x"+str(x1)
        condition_name += "y"+str(y1)
        condition_name += "z"+str(curr_z)

        condition_name += "_"

        condition_name += "x"+str(x2)
        condition_name += "y"+str(y2)
        condition_name += "z"+str(next_z)

        condition_name = z3.Bool(condition_name)

        
        self.var_info[condition_name] = {'u': u, 'v': v, 'prefix': prefix, 'net': net}
        
        return condition_name


    def make_vars(self, prefix=None, bounding_box=None, net=None, commodity=None):
        routing_graph = self.routing_graph
        
        cell_height = design_rules["cell_height"]
        layers = design_rules["routing_layers"]

        if net and net.is_power and commodity == None:
            net = copy.deepcopy(net)
            if POWER_NET in net.get_name():
                net.name = POWER_NET
                
            elif GND_NET in net.get_name():
                net.name = GND_NET

        min_x, min_y, min_z, max_x, max_y, max_z = (0, 0, 0, sys.maxsize, sys.maxsize, len(layers) - 1)            

        if bounding_box:
            min_x, min_y, _, max_x, max_y, _ = bounding_box 
            if net.is_power:
                if commodity == None:
                    min_x = 0
                    max_x = sys.maxsize

            else:
                x_right_tolerance, x_left_tolerance, y_upper_tolerance, y_lower_tolerance = net.get_tolerance()

                min_x -= x_left_tolerance * design_rules["x_unit"]
                max_x += x_right_tolerance * design_rules["x_unit"]
            
                min_y -= y_lower_tolerance * design_rules["y_unit"]
                max_y += y_upper_tolerance * design_rules["y_unit"]

        point_vars = dict()
        for point in routing_graph.nodes():
            x, y, z = point
            if (x < min_x or max_x < x) or (y < min_y or max_y < y) or (z < min_z or max_z < z):
                continue
            
            if net == DUMMY_NET and point not in net.get_pin_points():
                continue
            
            point_var = self.get_point_var(point=point, prefix=prefix, net=net)
            point_vars[point] = point_var

        edge_vars = defaultdict(dict)
        for u, v in routing_graph.edges():
            x1, y1, curr_z = u
            x2, y2, next_z = v

            if (x1 < min_x or max_x < x1) or (y1 < min_y or max_y < y1) or (curr_z < min_z or max_z < curr_z) or \
               (x2 < min_x or max_x < x2) or (y2 < min_y or max_y < y2) or (next_z < min_z or max_z < next_z):
                continue

            if net == DUMMY_NET and point not in net.get_pin_points():
                continue

            edge_var = self.get_edge_var(u=u, v=v, prefix=prefix, net=net, commodity=commodity)
            edge_vars[u][v] = edge_var
            edge_vars[v][u] = edge_var
            
            if (y1 == y2 and y1 % cell_height == 0):
                if (curr_z == next_z) and (layers[curr_z] in design_rules["power_layer"]):
                    if net and net.is_power and commodity == None:
                        constraint = (edge_var == True)
                        self.add_constraint(constraint)

                if (design_rules["num_row"] == 1) and \
                   (layers[curr_z] not in design_rules["power_layer"] and layers[next_z] not in design_rules["power_layer"]):
                    if net and not net.is_power:
                        constraint = (edge_var == False)
                        self.add_constraint(constraint)
        
        return point_vars, edge_vars


    def set_vars(self):
        prev_time = time.time()
        metal_point_vars, metal_edge_vars = self.make_vars()
        metal_ver_point_vars, _ = self.make_vars("METAL_VER")
        metal_hor_point_vars, _ = self.make_vars("METAL_HOR")


        for metal_num in range(len(design_rules["routing_layers"])):
            metal_vars = [value for (x, y, z), value in metal_point_vars.items() if z == metal_num]
            self.add_constraint(z3.Bool(f"Metal{metal_num}_usage") == Or(*metal_vars))

        nets = self.nets
        
        side_point_vars = [None] * 4
        tip_point_vars = [None] * 4
        corner_point_vars = [None] * 4
        via_enc_ver_point_vars = [None] * 2
        via_enc_hor_point_vars = [None] * 2
    
        
        for direction in [UPPER, LOWER]:
            if direction == UPPER:    surfix = "U"
            elif direction == LOWER:    surfix = "L"
                
            via_enc_ver_point_vars[direction], _ = self.make_vars(prefix="VIA_ENC_VER_"+surfix)
            via_enc_hor_point_vars[direction], _ = self.make_vars(prefix="VIA_ENC_HOR_"+surfix)
        
        for direction in [RIGHT, LEFT, TOP, BOTTOM]:
            if direction == RIGHT:    surfix = "R"
            elif direction == LEFT:    surfix = "L"
            elif direction == TOP:    surfix = "T"
            elif direction == BOTTOM:    surfix = "B"
            
            side_point_vars[direction], _ = self.make_vars(prefix="SIDE_"+surfix)
            tip_point_vars[direction], _ = self.make_vars(prefix="TIP_"+surfix)
            
        for direction in [TOP_RIGHT, TOP_LEFT, BOTTOM_RIGHT, BOTTOM_LEFT]:
            if direction == TOP_RIGHT:    surfix = "TR"
            elif direction == TOP_LEFT:    surfix = "TL"
            elif direction == BOTTOM_RIGHT:    surfix = "BR"
            elif direction == BOTTOM_LEFT:    surfix = "BL"
            
            corner_point_vars[direction], _ = self.make_vars(prefix="CORNER_"+surfix)
        
        net_point_vars = dict()
        net_edge_vars = dict()
        net_com_point_vars = dict()
        net_com_edge_vars = dict()
        
        for net in nets:
            bounding_box = net.get_bounding_box()
            
            net_point_vars[net], net_edge_vars[net] = self.make_vars(bounding_box=bounding_box, net=net)
            num_pins = net.get_num_pins()
            
            num_commodity = num_pins-1
            
            net_com_edge_vars_list = list()
            for commodity in range(num_commodity):
                _, net_com_edge_var = self.make_vars(bounding_box=bounding_box, net=net, commodity=commodity)
                net_com_edge_vars_list.append(net_com_edge_var)
            
            net_com_edge_vars[net] = net_com_edge_vars_list
        
        self.metal_point_vars = metal_point_vars
        self.metal_ver_point_vars = metal_ver_point_vars
        self.metal_hor_point_vars = metal_hor_point_vars
        
        self.metal_edge_vars = metal_edge_vars
        
        self.net_point_vars = net_point_vars
        self.net_edge_vars = net_edge_vars        

        self.net_com_point_vars = net_com_point_vars
        self.net_com_edge_vars = net_com_edge_vars        
        
        self.side_point_vars = side_point_vars
        self.tip_point_vars = tip_point_vars
        self.corner_point_vars = corner_point_vars
        self.via_enc_ver_point_vars = via_enc_ver_point_vars
        self.via_enc_hor_point_vars = via_enc_hor_point_vars
    
        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"The stage of creating variables for the SMT solver is complete. [{after_time-prev_time:.2f} s]")        


    def get_layer_enclosure(self, direction, via_of_layer, layer):
        try:
            layer_enc = design_rules["enclosure"][via_of_layer][layer]
            layer_enc = int(layer_enc + design_rules["width"][via_of_layer] / 2)
        except KeyError:
            layer_enc = 0
            logger.log(level=logging.INFO, msg=f"There is no {direction} via defined in layer {layer}. "
                    "Therefore, the enclosure metal extension for the via is set to 0.")
        return layer_enc


    def set_via_variables(self):
        prev_time = time.time()
        constraints_list = list()
        constraints_list.append(z3.Bool("set_via_variables"))
        
        metal_edge_vars = self.metal_edge_vars
        x_points, y_points, only_ver_metal_y_points = self.x_points, self.y_points, self.only_ver_metal_y_points
        via_point_vars = dict()

        via_upper_metal_edges = dict()
        for z, layer in enumerate(design_rules["routing_layers"]):
            upper_layers = design_rules["upper_layers"][layer]
            
            upper_zs = []
            for upper_layer in upper_layers:
                if upper_layer in design_rules["routing_layers"]:
                    layer_index = design_rules["routing_layers"].index(upper_layer)
                    upper_zs.append(layer_index)
            
            
            same_height_layers = design_rules["same_height_layers"][layer]
            same_height_zs = [design_rules["routing_layers"].index(layer) for layer in same_height_layers]
            max_same_height_layer_index = max(same_height_zs)
        
            
            for x_index in range(len(x_points[z])):
                x = x_points[z][x_index]                                
                for y_index in range(len(y_points[z])):
                    y = y_points[z][y_index]
                    
                    point = (x, y, z)
                    if point not in metal_edge_vars:
                        continue
                    
                    upper_points = [(x, y, upper_z) for upper_z in upper_zs]

                    upper_via_of_layer = design_rules["upper_via"][layer]
                    upper_via_index = design_rules["vias"].index(upper_via_of_layer) if upper_via_of_layer in design_rules["vias"] else None
                    via_point = (x, y, upper_via_index)
                    
                    if upper_via_index != None:
                        for upper_point in upper_points:
                            upper_edge_var = metal_edge_vars[point].get(upper_point)
                            if upper_edge_var != None:
                                if via_point not in via_upper_metal_edges:
                                    via_upper_metal_edges[via_point] = []
                                via_upper_metal_edges[via_point].append(upper_edge_var)

                        if z == max_same_height_layer_index:         
                            via_point_var = self.get_point_var(point=via_point, prefix="VIA")
                            via_point_vars[via_point] = via_point_var

                            if via_upper_metal_edges.get(via_point) != None:
                                constraint = Equal((Or(*via_upper_metal_edges.get(via_point))), via_point_var)
                                constraints_list.append(constraint)

                                
        self.via_point_vars = via_point_vars
        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added conditions to distinguish metal vias. [{after_time-prev_time:.2f} s]")

        return constraints_list


    def set_corner_variables(self):
        prev_time = time.time()
        constraints_list = list()
        constraints_list.append(z3.Bool("set_corner_variables"))

        corner_point_vars = self.corner_point_vars        
        metal_edge_vars = self.metal_edge_vars
        x_points, y_points, only_ver_metal_y_points = self.x_points, self.y_points, self.only_ver_metal_y_points

        for z, layer in enumerate(design_rules["routing_layers"]):
            lower_layers = design_rules["lower_layers"][layer]
            upper_layers = design_rules["upper_layers"][layer]
            
            lower_zs = []
            for lower_layer in lower_layers:
                if lower_layer in design_rules["routing_layers"]:
                    layer_index = design_rules["routing_layers"].index(lower_layer)
                    lower_zs.append(layer_index)

            upper_zs = []
            for upper_layer in upper_layers:
                if upper_layer in design_rules["routing_layers"]:
                    layer_index = design_rules["routing_layers"].index(upper_layer)
                    upper_zs.append(layer_index)

            same_height_layers = design_rules["same_height_layers"][layer]
            same_height_zs = [design_rules["routing_layers"].index(layer) for layer in same_height_layers]
        
            
            for x_index in range(len(x_points[z])):
                x = x_points[z][x_index]
                for y_index in range(len(y_points[z])):
                    y = y_points[z][y_index]
                    
                    point = (x, y, z)
                    if point not in metal_edge_vars:
                        continue
                    
                    this_point_metal_edges = metal_edge_vars[point]

                    left_x = get_position(x_points[z], x_index-1)
                    right_x = get_position(x_points[z], x_index+1)
                    
                    bottom_y = get_position(y_points[z], y_index-1)
                    top_y = get_position(y_points[z], y_index+1)
                    
                    right_point = get_point(right_x, y, z)
                    left_point = get_point(left_x, y, z)
                    top_point = get_point(x, top_y, z)
                    bottom_point = get_point(x, bottom_y, z)

                    same_height_points = [(x, y, same_height_z) for same_height_z in same_height_zs]
                    lower_points = [(x, y, lower_z) for lower_z in lower_zs]
                    upper_points = [(x, y, upper_z) for upper_z in upper_zs]

                    right_metal_edge_var = this_point_metal_edges.get(right_point)
                    left_metal_edge_var = this_point_metal_edges.get(left_point)
                    top_metal_edge_var = this_point_metal_edges.get(top_point)
                    bottom_metal_edge_var = this_point_metal_edges.get(bottom_point)              
                    
                    same_height_metal_edge_vars = [
                        same_height_metal_edge for same_height_metal_edge in (this_point_metal_edges.get(same_height_point) for same_height_point in same_height_points)
                        if same_height_metal_edge is not None
                    ]

                    lower_metal_edge_vars = [
                        lower_metal_edge for lower_metal_edge in (this_point_metal_edges.get(lower_point) for lower_point in lower_points)
                        if lower_metal_edge is not None
                    ]

                    upper_metal_edge_vars = [
                        upper_metal_edge for upper_metal_edge in (this_point_metal_edges.get(upper_point) for upper_point in upper_points)
                        if upper_metal_edge is not None
                    ]
                    
                    top_left_corner_point_var = corner_point_vars[TOP_LEFT].get(point)
                    top_right_corner_point_var = corner_point_vars[TOP_RIGHT].get(point)
                    bottom_left_corner_point_var = corner_point_vars[BOTTOM_LEFT].get(point)
                    bottom_right_corner_point_var = corner_point_vars[BOTTOM_RIGHT].get(point)                      
                    
                    if top_left_corner_point_var != None:
                        constraint = (top_left_corner_point_var == Or(
                            And(
                                Or(right_metal_edge_var, bottom_metal_edge_var, *upper_metal_edge_vars, *lower_metal_edge_vars, *same_height_metal_edge_vars),
                                Not(Or(left_metal_edge_var, top_metal_edge_var))
                            ),
                            And(left_metal_edge_var, top_metal_edge_var, must_match_num=2)
                        ))
                        constraints_list.append(constraint)
                    
                    if top_right_corner_point_var != None:
                        constraint = (top_right_corner_point_var == Or(
                            And(
                                Or(left_metal_edge_var, bottom_metal_edge_var, *upper_metal_edge_vars, *lower_metal_edge_vars, *same_height_metal_edge_vars),
                                Not(Or(right_metal_edge_var, top_metal_edge_var))
                            ),
                            And(right_metal_edge_var, top_metal_edge_var, must_match_num=2)
                        ))
                        constraints_list.append(constraint)
                    
                    if bottom_left_corner_point_var != None:
                        constraint = (bottom_left_corner_point_var == Or(
                            And(
                                Or(right_metal_edge_var, top_metal_edge_var, *upper_metal_edge_vars, *lower_metal_edge_vars, *same_height_metal_edge_vars),
                                Not(Or(left_metal_edge_var, bottom_metal_edge_var))
                            ),
                            And(left_metal_edge_var, bottom_metal_edge_var, must_match_num=2)
                        ))
                        constraints_list.append(constraint)
                        
                    if bottom_right_corner_point_var != None:
                        constraint = (bottom_right_corner_point_var == Or(
                            And(
                                Or(left_metal_edge_var, top_metal_edge_var, *upper_metal_edge_vars, *lower_metal_edge_vars, *same_height_metal_edge_vars),
                                Not(Or(right_metal_edge_var, bottom_metal_edge_var))
                            ),
                            And(right_metal_edge_var, bottom_metal_edge_var, must_match_num=2)
                        ))
                        constraints_list.append(constraint)

        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added conditions to distinguish metal corners. [{after_time-prev_time:.2f} s]")
        return constraints_list


    def set_tip_variables(self):
        prev_time = time.time()
        constraints_list = list()
        constraints_list.append(z3.Bool("set_tip_variables"))
        
        tip_point_vars = self.tip_point_vars        
        metal_edge_vars = self.metal_edge_vars
        x_points, y_points, only_ver_metal_y_points = self.x_points, self.y_points, self.only_ver_metal_y_points

        for z, layer in enumerate(design_rules["routing_layers"]):
            lower_layers = design_rules["lower_layers"][layer]
            upper_layers = design_rules["upper_layers"][layer]
            
            lower_zs = []
            for lower_layer in lower_layers:
                if lower_layer in design_rules["routing_layers"]:
                    layer_index = design_rules["routing_layers"].index(lower_layer)
                    lower_zs.append(layer_index)

            upper_zs = []
            for upper_layer in upper_layers:
                if upper_layer in design_rules["routing_layers"]:
                    layer_index = design_rules["routing_layers"].index(upper_layer)
                    upper_zs.append(layer_index)
                    
            same_height_layers = design_rules["same_height_layers"][layer]
            same_height_zs = [design_rules["routing_layers"].index(layer) for layer in same_height_layers]
            
            for x_index in range(len(x_points[z])):
                x = x_points[z][x_index]
                for y_index in range(len(y_points[z])):
                    y = y_points[z][y_index]
                    
                    point = (x, y, z)
                    if point not in metal_edge_vars:
                        continue
                                        
                    this_point_metal_edges = metal_edge_vars[point]

                    bottom_y = get_position(y_points[z], y_index-1)
                    top_y = get_position(y_points[z], y_index+1)
                    
                    right_x = get_position(x_points[z], x_index+1)
                    left_x = get_position(x_points[z], x_index-1)
                    
                    top_conditions = list()
                    bottom_conditions = list()
                    right_conditions = list()
                    left_conditions = list()
                    
                    right_point = get_point(right_x, y, z)
                    left_point = get_point(left_x, y, z)
                    top_point = get_point(x, top_y, z)
                    bottom_point = get_point(x, bottom_y, z)

                    same_height_points = [(x, y, same_height_z) for same_height_z in same_height_zs]
                    lower_points = [(x, y, lower_z) for lower_z in lower_zs]
                    upper_points = [(x, y, upper_z) for upper_z in upper_zs]

                    right_metal_edge_var = this_point_metal_edges.get(right_point)
                    left_metal_edge_var = this_point_metal_edges.get(left_point)
                    top_metal_edge_var = this_point_metal_edges.get(top_point)
                    bottom_metal_edge_var = this_point_metal_edges.get(bottom_point)    
                               
                    same_height_metal_edge_vars = [
                        same_height_metal_edge for same_height_metal_edge in (this_point_metal_edges.get(same_height_point) for same_height_point in same_height_points)
                        if same_height_metal_edge is not None
                    ]

                    lower_metal_edge_vars = [
                        lower_metal_edge for lower_metal_edge in (this_point_metal_edges.get(lower_point) for lower_point in lower_points)
                        if lower_metal_edge is not None
                    ]

                    upper_metal_edge_vars = [
                        upper_metal_edge for upper_metal_edge in (this_point_metal_edges.get(upper_point) for upper_point in upper_points)
                        if upper_metal_edge is not None
                    ]  
                    
                    # Basic Tip variables
                    if design_rules["width"][layer] <= design_rules["min_side_len"][layer]:
                        left_conditions.append(And(
                            Or(right_metal_edge_var, *upper_metal_edge_vars, *lower_metal_edge_vars, *same_height_metal_edge_vars), 
                            Not(Or(left_metal_edge_var, top_metal_edge_var, bottom_metal_edge_var))
                        ))
                        
                        right_conditions.append(And(
                            Or(left_metal_edge_var, *upper_metal_edge_vars, *lower_metal_edge_vars, *same_height_metal_edge_vars), 
                            Not(Or(right_metal_edge_var, top_metal_edge_var, bottom_metal_edge_var))
                        ))

                        top_conditions.append(And(
                            Or(bottom_metal_edge_var, *upper_metal_edge_vars, *lower_metal_edge_vars, *same_height_metal_edge_vars), 
                            Not(Or(top_metal_edge_var, left_metal_edge_var, right_metal_edge_var))
                        ))

                        bottom_conditions.append(And(
                            Or(top_metal_edge_var, *upper_metal_edge_vars, *lower_metal_edge_vars, *same_height_metal_edge_vars), 
                            Not(Or(bottom_metal_edge_var, left_metal_edge_var, right_metal_edge_var))
                        ))          
                    
                    for direction in [HORIZONTAL, VERTICAL]:
                        if direction == HORIZONTAL:
                            main_axis_index = x_index
                            main_axis_points = x_points
                            
                            main_axis_position = x
                            sub_axis_position = y
                            
                            sub_axis_next_position = top_y
                            sub_axis_prev_position = bottom_y

                            sub_axis_next_metal_edge_var = metal_edge_vars[(x, y, z)].get((x, top_y, z))
                            sub_axis_prev_metal_edge_var = metal_edge_vars[(x, y, z)].get((x, bottom_y, z))

                            sub_axis_next_conditions = top_conditions
                            sub_axis_prev_conditions = bottom_conditions

                            
                        elif direction == VERTICAL:
                            main_axis_index = y_index
                            main_axis_points = y_points
                            
                            main_axis_position = y
                            sub_axis_position = x
                            
                            sub_axis_next_position = right_x
                            sub_axis_prev_position = left_x

                            sub_axis_next_metal_edge_var = metal_edge_vars[(x, y, z)].get((right_x, y, z))
                            sub_axis_prev_metal_edge_var = metal_edge_vars[(x, y, z)].get((left_x, y, z))

                            sub_axis_next_conditions = right_conditions
                            sub_axis_prev_conditions = left_conditions


                        for smallest_index in range(main_axis_index, -1, -1):
                            smallest_position = main_axis_points[z][smallest_index]
                            smallest_point = get_point(smallest_position, sub_axis_position, z) if direction == HORIZONTAL else get_point(sub_axis_position, smallest_position, z)
                            
                            length = main_axis_position - smallest_position + design_rules["width"][layer]

                            if design_rules["min_side_len"][layer] < length - design_rules["width"][layer]:
                                break

                            smallest_sub_next_point = get_point(smallest_position, sub_axis_next_position, z) if direction == HORIZONTAL else get_point(sub_axis_next_position, smallest_position, z)
                            smallest_sub_prev_point = get_point(smallest_position, sub_axis_prev_position, z) if direction == HORIZONTAL else get_point(sub_axis_prev_position, smallest_position, z)
                            
                            smallest_sub_next_metal_edge = metal_edge_vars[smallest_point].get(smallest_sub_next_point)
                            smallest_sub_prev_metal_edge = metal_edge_vars[smallest_point].get(smallest_sub_prev_point)

                            sub_next_metal_edges = {smallest_sub_next_metal_edge}
                            sub_prev_metal_edges = {smallest_sub_prev_metal_edge}

                            target_metal_edges = set()

                            for largest_index in range(smallest_index+1, len(main_axis_points[z]), 1):
                                largest_position = main_axis_points[z][largest_index]
                                
                                largest_point = get_point(largest_position, sub_axis_position, z) if direction == HORIZONTAL else get_point(sub_axis_position, largest_position, z)

                                largest_sub_next_point = get_point(largest_position, sub_axis_next_position, z) if direction == HORIZONTAL else get_point(sub_axis_next_position, largest_position, z)
                                largest_sub_prev_point = get_point(largest_position, sub_axis_prev_position, z) if direction == HORIZONTAL else get_point(sub_axis_prev_position, largest_position, z)

                                largest_sub_next_metal_edge = metal_edge_vars[largest_point].get(largest_sub_next_point)
                                largest_sub_prev_metal_edge = metal_edge_vars[largest_point].get(largest_sub_prev_point)   
                                
                                sub_next_metal_edges.add(largest_sub_next_metal_edge)
                                sub_prev_metal_edges.add(largest_sub_prev_metal_edge)
                                  
                                prev_smallest_position = get_position(main_axis_points[z], smallest_index-1)
                                prev_smallest_point = get_point(prev_smallest_position, sub_axis_position, z) if direction == HORIZONTAL else get_point(sub_axis_position, prev_smallest_position, z)

                                next_largest_position = get_position(main_axis_points[z], largest_index+1)
                                next_largest_point = get_point(next_largest_position, sub_axis_position, z) if direction == HORIZONTAL else get_point(sub_axis_position, next_largest_position, z)

                                prev_smallest_metal_edge = metal_edge_vars[prev_smallest_point].get(smallest_point)
                                next_largest_metal_edge = metal_edge_vars[largest_point].get(next_largest_point)
                                
                                prev_largest_position = get_position(main_axis_points[z], largest_index-1)
                                prev_largest_point = get_point(prev_largest_position, sub_axis_position, z) if direction == HORIZONTAL else get_point(sub_axis_position, prev_largest_position, z)
                                
                                if metal_edge_vars[prev_largest_point].get(largest_point) == None:
                                    break
                                target_metal_edges.add(metal_edge_vars[prev_largest_point].get(largest_point))
                                
                                if largest_position < main_axis_position:
                                    continue                   
                                
                                length = (largest_position - smallest_position) + design_rules["width"][layer]

                                if length <= design_rules["min_side_len"][layer]:
                                    sub_axis_next_condition = And(And(*list(target_metal_edges)), Not(Or(*list(sub_next_metal_edges), prev_smallest_metal_edge, next_largest_metal_edge)))
                                    sub_axis_next_conditions.append(sub_axis_next_condition)

                                    sub_axis_prev_condition = And(And(*list(target_metal_edges)), Not(Or(*list(sub_prev_metal_edges), prev_smallest_metal_edge, next_largest_metal_edge)))
                                    sub_axis_prev_conditions.append(sub_axis_prev_condition)
                                    
                                
                                if design_rules["routing_directions"][z] == BIDIRECTION:
                                    largest_sub_next_metal_edge = metal_edge_vars[largest_point].get(largest_sub_next_point)
                                    largest_sub_prev_metal_edge = metal_edge_vars[largest_point].get(largest_sub_prev_point)                                                
                                    
                                    
                                    #### L Shape  
                                    if length - design_rules["width"][layer] <= design_rules["min_side_len"][layer] and 0 < length - design_rules["width"][layer]:
                                        if (main_axis_position != smallest_position):
                                            if smallest_sub_next_metal_edge != None:                                                    
                                                sub_axis_next_condition = And(And(*list(target_metal_edges), smallest_sub_next_metal_edge), Not(Or(sub_axis_next_metal_edge_var, next_largest_metal_edge)))
                                                sub_axis_next_conditions.append(sub_axis_next_condition)

                                            if smallest_sub_prev_metal_edge != None:
                                                sub_axis_prev_condition = And(And(*list(target_metal_edges), smallest_sub_prev_metal_edge), Not(Or(sub_axis_prev_metal_edge_var, next_largest_metal_edge)))
                                                sub_axis_prev_conditions.append(sub_axis_prev_condition)

                                        if (main_axis_position != largest_position):
                                            if largest_sub_next_metal_edge != None:                                                    
                                                sub_axis_next_condition = And(And(*list(target_metal_edges), largest_sub_next_metal_edge), Not(Or(sub_axis_next_metal_edge_var, prev_smallest_metal_edge)))
                                                sub_axis_next_conditions.append(sub_axis_next_condition)

                                            if largest_sub_prev_metal_edge != None:
                                                sub_axis_prev_condition = And(And(*list(target_metal_edges), largest_sub_prev_metal_edge), Not(Or(sub_axis_prev_metal_edge_var, prev_smallest_metal_edge)))
                                                sub_axis_prev_conditions.append(sub_axis_prev_condition)
                                                                              
                                    
                                    #### U Shape
                                    elif length - 2*design_rules["width"][layer] <= design_rules["min_side_len"][layer] and 0 < length - design_rules["width"][layer]:
                                        if (main_axis_position != smallest_position) and \
                                           (main_axis_position != largest_position) and \
                                            smallest_sub_next_metal_edge != None and \
                                            largest_sub_next_metal_edge != None and \
                                            smallest_sub_prev_metal_edge != None and \
                                            largest_sub_prev_metal_edge != None:
                                            
                                            sub_axis_next_condition = And(And(*list(target_metal_edges), smallest_sub_next_metal_edge, largest_sub_next_metal_edge), Not(Or(sub_axis_next_metal_edge_var)))
                                            sub_axis_next_conditions.append(sub_axis_next_condition) 

                                            sub_axis_prev_condition = And(And(*list(target_metal_edges), smallest_sub_prev_metal_edge, largest_sub_prev_metal_edge), Not(Or(sub_axis_prev_metal_edge_var)))
                                            sub_axis_prev_conditions.append(sub_axis_prev_condition)
                                            
                                    else:
                                        break


                    if tip_point_vars[TOP].get(point) != None:
                        constraint = (tip_point_vars[TOP].get(point) == Or(*top_conditions))
                        constraints_list.append(constraint)
                    
                    if tip_point_vars[BOTTOM].get(point) != None:
                        constraint = (tip_point_vars[BOTTOM].get(point) == Or(*bottom_conditions))
                        constraints_list.append(constraint)
                    
                    if tip_point_vars[RIGHT].get(point) != None:    
                        constraint = (tip_point_vars[RIGHT].get(point) == Or(*right_conditions))
                        constraints_list.append(constraint)
                        
                    if tip_point_vars[LEFT].get(point) != None:
                        constraint = (tip_point_vars[LEFT].get(point) == Or(*left_conditions))
                        constraints_list.append(constraint)
        
        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added conditions to distinguish metal tips. [{after_time-prev_time:.2f} s]")

        return constraints_list


    def set_side_variables(self):
        prev_time = time.time()
        constraints_list = [z3.Bool("set_side_variables")]
        
        side_point_vars = self.side_point_vars
        metal_edge_vars = self.metal_edge_vars
        x_points, y_points, only_ver_metal_y_points = self.x_points, self.y_points, self.only_ver_metal_y_points
        
        routing_layers = design_rules["routing_layers"]
        routing_directions = design_rules["routing_directions"]
        widths = design_rules["width"]
        extensions = design_rules["extension"]
        min_side_lens = design_rules["min_side_len"]

        for z, layer in enumerate(routing_layers):
            routing_dir = routing_directions[z]
            width_or_ext = widths[layer] if routing_dir == BIDIRECTION else extensions[layer]

            for x_index in range(len(x_points[z])):
                x = x_points[z][x_index]

                for y_index in range(len(y_points[z])):
                    y = y_points[z][y_index]

                    point = (x, y, z)
                    
                    if point not in metal_edge_vars:
                        continue
                    
                    bottom_y = get_position(y_points[z], y_index-1)
                    top_y = get_position(y_points[z], y_index+1)
                    
                    right_x = get_position(x_points[z], x_index+1)
                    left_x = get_position(x_points[z], x_index-1)

                    top_conditions = []
                    bottom_conditions = []
                    right_conditions = []
                    left_conditions = []
                    
                    for direction in (HORIZONTAL, VERTICAL):
                        if direction == HORIZONTAL:
                            index = x_index
                            points = x_points
                            pos = x
                            sub_axis_pos = y
                            sub_axis_prev = bottom_y
                            sub_axis_next = top_y
                            sub_axis_prev_conditions = bottom_conditions
                            sub_axis_next_conditions = top_conditions
                            
                        elif direction == VERTICAL:
                            index = y_index
                            points = y_points
                            pos = y
                            sub_axis_pos = x
                            sub_axis_prev = left_x
                            sub_axis_next = right_x
                            sub_axis_prev_conditions = left_conditions
                            sub_axis_next_conditions = right_conditions
                            
                        else:
                            logger.log(level=logging.INFO, msg=f"In the set_side_variables function, please input the correct routing direction.")


                        for smallest_index in range(index, -1, -1):
                            smallest_pos = points[z][smallest_index]

                            smallest_point = (get_point(smallest_pos, sub_axis_pos, z) if direction == HORIZONTAL else \
                                              get_point(sub_axis_pos, smallest_pos, z))
                            
                            base_metal_length = (pos - smallest_pos) + width_or_ext
                            done = base_metal_length > min_side_lens[layer]

                            smallest_sub_next_point = get_point(smallest_pos, sub_axis_next, z) if direction == HORIZONTAL else get_point(sub_axis_next, smallest_pos, z)
                            smallest_sub_prev_point = get_point(smallest_pos, sub_axis_prev, z) if direction == HORIZONTAL else get_point(sub_axis_prev, smallest_pos, z)
                                
                            smallest_sub_next_metal_edge = metal_edge_vars[smallest_point].get(smallest_sub_next_point)
                            smallest_sub_prev_metal_edge = metal_edge_vars[smallest_point].get(smallest_sub_prev_point)

                            sub_next_metal_edges = {smallest_sub_next_metal_edge}
                            sub_prev_metal_edges = {smallest_sub_prev_metal_edge}
                            
                            target_metal_edges = set()

                            for largest_index in range(smallest_index + 1, len(points[z])):
                                largest_pos = points[z][largest_index]
                                
                                largest_point = (get_point(largest_pos, sub_axis_pos, z)
                                                if direction == HORIZONTAL 
                                                else get_point(sub_axis_pos, largest_pos, z))
                                
                                largest_sub_next_point = get_point(largest_pos, sub_axis_next, z) if direction == HORIZONTAL else get_point(sub_axis_next, largest_pos, z)
                                largest_sub_prev_point = get_point(largest_pos, sub_axis_prev, z) if direction == HORIZONTAL else get_point(sub_axis_prev, largest_pos, z)
                                
                                largest_sub_next_metal_edge = metal_edge_vars[largest_point].get(largest_sub_next_point)
                                largest_sub_prev_metal_edge = metal_edge_vars[largest_point].get(largest_sub_prev_point)
                                
                                sub_next_metal_edges.add(largest_sub_next_metal_edge)
                                sub_prev_metal_edges.add(largest_sub_prev_metal_edge)
                                
                                prev_pos = get_position(points[z], largest_index - 1)
                                if prev_pos is None:
                                    continue
                                
                                prev_point = (get_point(prev_pos, sub_axis_pos, z) if direction == HORIZONTAL else \
                                              get_point(sub_axis_pos, prev_pos, z))
                                
                                if metal_edge_vars[prev_point].get(largest_point) is None:
                                    break
                                
                                target_metal_edges.add(metal_edge_vars[prev_point].get(largest_point))
                                
                                
                                if largest_pos < pos:
                                    continue
                                        
                                metal_length = (largest_pos - smallest_pos) + width_or_ext
                                
                                if routing_dir == BIDIRECTION:
                                    if metal_length - 2 * widths[layer] > min_side_lens[layer]:
                                        if pos != smallest_pos and pos != largest_pos:
                                            temp_next = sub_next_metal_edges - {smallest_sub_next_metal_edge, largest_sub_next_metal_edge}
                                            cond_next = And(And(*list(target_metal_edges)), Not(Or(*list(temp_next))))
                                            sub_axis_next_conditions.append(cond_next)

                                            temp_prev = sub_prev_metal_edges - {smallest_sub_prev_metal_edge, largest_sub_prev_metal_edge}
                                            cond_prev = And(And(*list(target_metal_edges)), Not(Or(*list(temp_prev))))
                                            sub_axis_prev_conditions.append(cond_prev)
                                        else:
                                            if pos != smallest_pos:
                                                temp_next = sub_next_metal_edges - {smallest_sub_next_metal_edge}
                                                cond_next = And(And(*list(target_metal_edges)), Not(Or(*list(temp_next))))
                                                sub_axis_next_conditions.append(cond_next)

                                                temp_prev = sub_prev_metal_edges - {smallest_sub_prev_metal_edge}
                                                cond_prev = And(And(*list(target_metal_edges)), Not(Or(*list(temp_prev))))
                                                sub_axis_prev_conditions.append(cond_prev)
                                            if pos != largest_pos:
                                                temp_next = sub_next_metal_edges - {largest_sub_next_metal_edge}
                                                cond_next = And(And(*list(target_metal_edges)), Not(Or(*list(temp_next))))
                                                sub_axis_next_conditions.append(cond_next)

                                                temp_prev = sub_prev_metal_edges - {largest_sub_prev_metal_edge}
                                                cond_prev = And(And(*list(target_metal_edges)), Not(Or(*list(temp_prev))))
                                                sub_axis_prev_conditions.append(cond_prev)
                                        break
                                    elif metal_length - widths[layer] > min_side_lens[layer]:
                                        if pos != smallest_pos:
                                            temp_next = sub_next_metal_edges - {smallest_sub_next_metal_edge}
                                            cond_next = And(And(*list(target_metal_edges)), Not(Or(*list(temp_next))))
                                            sub_axis_next_conditions.append(cond_next)

                                            temp_prev = sub_prev_metal_edges - {smallest_sub_prev_metal_edge}
                                            cond_prev = And(And(*list(target_metal_edges)), Not(Or(*list(temp_prev))))
                                            sub_axis_prev_conditions.append(cond_prev)
                                        if pos != largest_pos:
                                            temp_next = sub_next_metal_edges - {largest_sub_next_metal_edge}
                                            cond_next = And(And(*list(target_metal_edges)), Not(Or(*list(temp_next))))
                                            sub_axis_next_conditions.append(cond_next)

                                            temp_prev = sub_prev_metal_edges - {largest_sub_prev_metal_edge}
                                            cond_prev = And(And(*list(target_metal_edges)), Not(Or(*list(temp_prev))))
                                            sub_axis_prev_conditions.append(cond_prev)
                                    elif metal_length > min_side_lens[layer]:
                                        cond_next = And(And(*list(target_metal_edges)), Not(Or(*list(sub_next_metal_edges))))
                                        sub_axis_next_conditions.append(cond_next)
                                        cond_prev = And(And(*list(target_metal_edges)), Not(Or(*list(sub_prev_metal_edges))))
                                        sub_axis_prev_conditions.append(cond_prev)
                                
                                else:
                                    if metal_length > min_side_lens[layer]:
                                        cond_next = And(And(*list(target_metal_edges)), Not(Or(*list(sub_next_metal_edges))))
                                        sub_axis_next_conditions.append(cond_next)
                                        cond_prev = And(And(*list(target_metal_edges)), Not(Or(*list(sub_prev_metal_edges))))
                                        sub_axis_prev_conditions.append(cond_prev)
                                        break
                            if done:
                                break

                    if side_point_vars[TOP].get(point) is not None:
                        constraints_list.append(side_point_vars[TOP].get(point) == Or(*top_conditions))
                    if side_point_vars[BOTTOM].get(point) is not None:
                        constraints_list.append(side_point_vars[BOTTOM].get(point) == Or(*bottom_conditions))
                    if side_point_vars[RIGHT].get(point) is not None:
                        constraints_list.append(side_point_vars[RIGHT].get(point) == Or(*right_conditions))
                    if side_point_vars[LEFT].get(point) is not None:
                        constraints_list.append(side_point_vars[LEFT].get(point) == Or(*left_conditions))
                        
        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added conditions to distinguish metal sides. [{after_time - prev_time:.2f} s]")
        return constraints_list


    def exclusive_metal_edge_type(self):
        prev_time = time.time()
        constraints_list = [z3.Bool("exclusive_metal_edge")]
        
        side_point_vars = self.side_point_vars
        tip_point_vars = self.tip_point_vars

        metal_point_vars = self.metal_point_vars
        x_points, y_points, only_ver_metal_y_points = self.x_points, self.y_points, self.only_ver_metal_y_points
        
        routing_layers = design_rules["routing_layers"]
        for z, layer in enumerate(routing_layers):            
            for x_index in range(len(x_points[z])):
                x = x_points[z][x_index]
                for y_index in range(len(y_points[z])):
                    y = y_points[z][y_index]

                    point = (x, y, z)
                    if point not in metal_point_vars:
                        continue
                    
                    if side_point_vars[TOP].get(point) is not None and tip_point_vars[TOP].get(point) is not None:
                        constraint = Not(And(side_point_vars[TOP].get(point), tip_point_vars[TOP].get(point)))
                        constraints_list.append(constraint)
                    
                    if side_point_vars[BOTTOM].get(point) is not None and tip_point_vars[BOTTOM].get(point) is not None:
                        constraint = Not(And(side_point_vars[BOTTOM].get(point), tip_point_vars[BOTTOM].get(point)))
                        constraints_list.append(constraint)
                    
                    if side_point_vars[RIGHT].get(point) is not None and tip_point_vars[RIGHT].get(point) is not None:
                        constraint = Not(And(side_point_vars[RIGHT].get(point), tip_point_vars[RIGHT].get(point)))
                        constraints_list.append(constraint)
                    
                    if side_point_vars[LEFT].get(point) is not None and tip_point_vars[LEFT].get(point) is not None:
                        constraint = Not(And(side_point_vars[LEFT].get(point), tip_point_vars[LEFT].get(point)))
                        constraints_list.append(constraint)

        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added constraints for distingushing metal tips. [{after_time-prev_time:.2f} s]")

        return constraints_list


    def metal_direction_definition(self):
        constraints_list = [z3.Bool("metal_direction_definition")]
        
        metal_edge_vars = self.metal_edge_vars
        
        metal_ver_point_vars = self.metal_ver_point_vars
        metal_hor_point_vars = self.metal_hor_point_vars
        
        x_points, y_points = self.x_points, self.y_points
        
        for z, curr_layer in enumerate(design_rules["routing_layers"]):            
            for x_index in range(len(x_points[z])):
                x = x_points[z][x_index]
                
                left_x = get_position(x_points[z], x_index-1)
                right_x = get_position(x_points[z], x_index+1)
                
                for y_index in range(len(y_points[z])):   
                    y = y_points[z][y_index]            
                    
                    bottom_y = get_position(y_points[z], y_index-1)
                    top_y = get_position(y_points[z], y_index+1)
                    
                    point = (x, y, z)

                    right_point = get_point(right_x, y, z)
                    left_point = get_point(left_x, y, z)
                    top_point = get_point(x, top_y, z)
                    bottom_point = get_point(x, bottom_y, z)  
                    
                    right_metal_edge = metal_edge_vars[point].get(right_point)
                    left_metal_edge = metal_edge_vars[point].get(left_point)
                    top_metal_edge = metal_edge_vars[point].get(top_point)
                    bottom_metal_edge = metal_edge_vars[point].get(bottom_point)   
                    
                    metal_ver_point_var = metal_ver_point_vars.get(point)
                    metal_hor_point_var = metal_hor_point_vars.get(point)
                    
                    if metal_ver_point_var != None:
                        ver_metal_type = (metal_ver_point_var == Or(top_metal_edge, bottom_metal_edge))
                        constraints_list.append(ver_metal_type)
                    
                    if metal_hor_point_var != None:
                        hor_metal_type = (metal_hor_point_var == Or(right_metal_edge, left_metal_edge))
                        constraints_list.append(hor_metal_type)
                    
        return constraints_list



    def set_geometric_variables(self):
        prev_time = time.time()
        constraints_list = list()

        variables = self.metal_direction_definition()
        constraints_list.extend(variables)

        variables = self.set_tip_variables()
        constraints_list.extend(variables)

        variables = self.set_via_variables()
        constraints_list.extend(variables)

        variables = self.set_corner_variables()
        constraints_list.extend(variables)
    
        variables = self.set_side_variables()
        constraints_list.extend(variables)

        variables = self.exclusive_metal_edge_type()
        constraints_list.extend(variables)
        
        self.solver.add(*constraints_list)
            

        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"The stage of adding geometric variables like side, tip, and corner for the SMT solver is complete. [{after_time-prev_time:.2f} s]")


    def pre_layout_design_rule(self):
        prev_time = time.time()
        constraints_list = list()
        constraints_list.append(z3.Bool("pre_layout_design_rule"))

        via_point_vars = self.via_point_vars   
        side_point_vars = self.side_point_vars
        tip_point_vars = self.tip_point_vars
        corner_point_vars = self.corner_point_vars        
        
        metal_edge_vars = self.metal_edge_vars

        x_points, y_points, only_ver_metal_y_points = self.x_points, self.y_points, self.only_ver_metal_y_points

        for z, curr_layer in enumerate(design_rules["routing_layers"]+design_rules["vias"]):            
            is_layer = (curr_layer in design_rules["routing_layers"])
            if is_layer:
                routing_dir = design_rules["routing_directions"][z]
                
            else:
                routing_dir = BIDIRECTION
                z -= len(design_rules["routing_layers"]) # change to via index
            
            for next_layer in list(design_rules["layer_map"].keys()):
                S2S_distance = design_rules["spacing"]["S2S"].get(curr_layer, {}).get(next_layer, 0)
                S2T_distance = design_rules["spacing"]["S2T"].get(curr_layer, {}).get(next_layer, 0)
                T2T_distance = design_rules["spacing"]["T2T"].get(curr_layer, {}).get(next_layer, 0)
                C2C_distance = design_rules["spacing"]["C2C"].get(curr_layer, {}).get(next_layer, 0)

                if S2S_distance < 0 and S2T_distance < 0 and T2T_distance < 0 and C2C_distance < 0:
                    continue

                for x_index in range(len(x_points[z])):
                    x = x_points[z][x_index]
                    for y_index in range(len(y_points[z])):
                        y = y_points[z][y_index]
                            
                        x_ex = design_rules["extension"][curr_layer] if routing_dir == HORIZONTAL else design_rules["width"][curr_layer]
                        y_ex = design_rules["extension"][curr_layer] if routing_dir == VERTICAL else design_rules["width"][curr_layer]
                        
                        if 0 <= S2S_distance:
                            ### Side to Side
                            adj_metals, not_edge = self.pre_layout.query_polygon(x_points[z], y_points[z], x_index, y_index, x_ex, y_ex, next_layer, z, SIDE, S2S_distance)
                            for dir, adj_metal in adj_metals.items():
                                if not adj_metal:
                                    continue
                                point_vars = side_point_vars[dir] if is_layer else via_point_vars
                                point_val = point_vars.get((x, y, z))
                                if point_val is not None:
                                    if not_edge == None:
                                        constraints_list.append(point_val == False)
                                    else:
                                        try:
                                            not_edge_var = metal_edge_vars[not_edge[0]][not_edge[1]]
                                            constraints_list.append(z3.Or(not_edge_var, z3.Not(point_val)))
                                        except KeyError:
                                            constraints_list.append(point_val == False)


                        if 0 <= S2T_distance:
                            ### Side to Tip                                
                            adj_metals, not_edge = self.pre_layout.query_polygon(x_points[z], y_points[z], x_index, y_index, x_ex, y_ex, next_layer, z, TIP, S2T_distance)
                            for dir, adj_metal in adj_metals.items():
                                if not adj_metal:
                                    continue
                                point_vars = side_point_vars[dir] if is_layer else via_point_vars
                                point_val = point_vars.get((x, y, z))
                                if point_val is not None:
                                    if not_edge == None:
                                        constraints_list.append(point_val == False)
                                    else:
                                        try:
                                            not_edge_var = metal_edge_vars[not_edge[0]][not_edge[1]]
                                            constraints_list.append(z3.Or(not_edge_var, z3.Not(point_val)))
                                        except KeyError:
                                            constraints_list.append(point_val == False)
                                
                                
                            adj_metals, not_edge = self.pre_layout.query_polygon(x_points[z], y_points[z], x_index, y_index, x_ex, y_ex, next_layer, z, SIDE, S2T_distance)
                            for dir, adj_metal in adj_metals.items():
                                if not adj_metal:
                                    continue
                                point_vars = tip_point_vars[dir] if is_layer else via_point_vars
                                point_val = point_vars.get((x, y, z))
                                if point_val is not None:
                                    if not_edge == None:
                                        constraints_list.append(point_val == False)
                                    else:
                                        try:
                                            not_edge_var = metal_edge_vars[not_edge[0]][not_edge[1]]
                                            constraints_list.append(z3.Or(not_edge_var, z3.Not(point_val)))
                                        except KeyError:
                                            constraints_list.append(point_val == False)

                        if 0 <= T2T_distance:
                            ### Tip to Tip
                            adj_metals, not_edge = self.pre_layout.query_polygon(x_points[z], y_points[z], x_index, y_index, x_ex, y_ex, next_layer, z, TIP, T2T_distance)
                            for dir, adj_metal in adj_metals.items():
                                if not adj_metal:
                                    continue
                                point_vars = tip_point_vars[dir] if is_layer else via_point_vars
                                point_val = point_vars.get((x, y, z))
                                if point_val is not None:
                                    if not_edge == None:
                                        constraints_list.append(point_val == False)
                                    else:
                                        try:
                                            not_edge_var = metal_edge_vars[not_edge[0]][not_edge[1]]
                                            constraints_list.append(z3.Or(not_edge_var, z3.Not(point_val)))
                                        except KeyError:
                                            constraints_list.append(point_val == False)

                        if 0 <= C2C_distance:
                            ### Corner to Corner
                            adj_metals, not_edge = self.pre_layout.query_polygon(x_points[z], y_points[z], x_index, y_index, x_ex, y_ex, next_layer, z, CORNER, C2C_distance)
                            for dir, adj_metal in adj_metals.items():
                                if not adj_metal:
                                    continue
                                point_vars = corner_point_vars[dir] if is_layer else via_point_vars
                                point_val = point_vars.get((x, y, z))
                                if point_val is not None:
                                    constraints_list.append(point_val == False)


        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added design constraints to account for previously fixed objects. [{after_time-prev_time:.2f} s]")

        return constraints_list


    ### Commodity Flow Conservation (CFC)
    def commodity_flow_conservation(self):
        prev_time = time.time()
        constraints_list = list()
        constraints_list.append(z3.Bool("commodity_flow_conservation"))

        def get_pin_edge_vars(net_com_edge_vars, net, commodity, points):
            boundary_net_com_edge_vars = list()
                
            for point in points: 
                adj_points = net_com_edge_vars[net][commodity][point].keys()
                for adj_point in adj_points:
                    if adj_point not in points:
                        net_com_edge_var = net_com_edge_vars[net][commodity][point][adj_point]
                        boundary_net_com_edge_vars.append(net_com_edge_var)
            return boundary_net_com_edge_vars  
        
        nets = self.nets
        net_com_edge_vars = self.net_com_edge_vars
        net_edge_vars = self.net_edge_vars
        
        for net in nets:
            if net == DUMMY_NET or net.is_power:
                continue
            net_points = net_edge_vars[net].keys()
            
            pins = net.get_pins()
            source_points = pins[0].get_points()
            
            num_pins = net.get_num_pins()
            if num_pins <= 1:
                continue
            
            for point in net_points:                
                for commodity in range(num_pins-1):                   
                    sink_pin = pins[commodity+1]
                    sink_points = sink_pin.get_points()
                    
                    if point in source_points+sink_points:
                        continue
                    
                    boundary_net_com_edge_vars = list()
                    
                    adj_points = net_com_edge_vars[net][commodity][point].keys()
                    for adj_point in adj_points:
                        net_com_edge_var = net_com_edge_vars[net][commodity][point][adj_point]
                        boundary_net_com_edge_vars.append(net_com_edge_var)
                    
                    if len(boundary_net_com_edge_vars) == 1:
                        constraint = (boundary_net_com_edge_vars[0] == False)
                        constraints_list.append(constraint)
                        
                    elif len(boundary_net_com_edge_vars) == 2:
                        constraint = (z3.Not(z3.Xor(*boundary_net_com_edge_vars)))
                        constraints_list.append(constraint)
                        
                    elif len(boundary_net_com_edge_vars) > 2:
                        constraint = (Not_Exactly_one(boundary_net_com_edge_vars))
                        constraints_list.append(constraint)
                        
            
            for pin_num in range(num_pins):
                pin = pins[pin_num]                
                points = pin.get_points()
                
                if pin_num == 0: # source pin
                    for commodity in range(num_pins-1):
                        boundary_net_com_edge_vars = get_pin_edge_vars(net_com_edge_vars, net, commodity, points)

                        if len(boundary_net_com_edge_vars) == 0:
                            raise RuntimeError(f"Error: There are no edges that can leave the source pin. net: {net}")
                        
                        elif len(boundary_net_com_edge_vars) == 1:
                            constraint = (boundary_net_com_edge_vars[0] == True)
                            constraints_list.append(constraint)
                                                    
                        elif len(boundary_net_com_edge_vars) > 1:
                            constraint = (Exactly(boundary_net_com_edge_vars, 1))
                            constraints_list.append(constraint)


                elif 0 < pin_num < num_pins: # sink pin
                    commodity = pin_num-1
                    boundary_net_com_edge_vars = get_pin_edge_vars(net_com_edge_vars, net, commodity, points)
                    
                    if len(boundary_net_com_edge_vars) == 0:
                        raise RuntimeError(f"Error: There are no edges that can leave the sink pin. net: {net}, {points}")
                    
                    elif len(boundary_net_com_edge_vars) == 1:
                        constraint = (boundary_net_com_edge_vars[0] == True)
                        constraints_list.append(constraint)

                    elif len(boundary_net_com_edge_vars) > 1:
                        constraint = (Exactly(boundary_net_com_edge_vars, 1))
                        constraints_list.append(constraint)

        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added multi-commodity flow conservation constraints. [{after_time-prev_time:.2f} s]")
        return constraints_list


    def exclusiveness_vertex(self):
        prev_time = time.time()
        constraints_list = list()
        constraints_list.append(z3.Bool("exclusiveness_vertex"))

        nets = self.nets
        metal_point_vars = self.metal_point_vars
        net_point_vars = self.net_point_vars
        net_edge_vars = self.net_edge_vars
            
        for net in nets:            
            for point in list(net_point_vars[net].keys()):
                net_point_var = net_point_vars[net][point]
                
                net_edge_var_list = list()
                net_edge_var_list_wo_via = list()
                
                adj_points = list(net_edge_vars[net][point].keys())
                for adj_point in list(adj_points):                
                    net_edge_var = net_edge_vars[net][point][adj_point]
                    net_edge_var_list.append(net_edge_var)
                    
                    if point[Z] == adj_point[Z]:
                        net_edge_var_list_wo_via.append(net_edge_var)

                if len(net_edge_var_list) > 0:
                    net_edge_var_list = list(set(net_edge_var_list))
                    constraint = Equal(net_point_var, (Or(*net_edge_var_list)))
                else:
                    constraint = (net_point_var == False)
                constraints_list.append(constraint)
                    
                #if len(net_edge_var_list_wo_via) > 0:
                #    net_edge_var_list_wo_via = list(set(net_edge_var_list_wo_via))
                #    self.add_constraint(z3.AtMost(*net_edge_var_list_wo_via, 4))
                    
                    
        for point in list(metal_point_vars.keys()):
            net_point_var_list = list()
            for net in nets:                
                if point in net_point_vars[net].keys():
                    net_point_var = net_point_vars[net][point]
                    net_point_var_list.append(net_point_var)
            
            if len(net_point_var_list) > 0:
                net_point_var_list = list(set(net_point_var_list))
                constraint = (z3.AtMost(*net_point_var_list, 1))
                constraints_list.append(constraint)

        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added constraints for exclusiveness vertex. [{after_time-prev_time:.2f} s]")
        return constraints_list
                    

    def edge_assignment(self):
        prev_time = time.time()
        constraints_list = list()
        constraints_list.append(z3.Bool("edge_assignment"))

        nets = self.nets
        net_edge_vars = self.net_edge_vars
        net_com_edge_vars = self.net_com_edge_vars
        
        for net in nets:
            if net == DUMMY_NET or net.is_power:
                continue
            
            com_edge_vars = net_com_edge_vars[net]
            edge_vars = net_edge_vars[net]
            
            num_commodity = net.get_num_pins()-1
            
            points = edge_vars.keys()
            for point in points:                  
                adj_points = edge_vars[point].keys()
                for adj_point in adj_points:
                    if point < adj_point:
                        continue

                    net_edge_var = edge_vars[point][adj_point]
                    com_edges = list()
                    for commodity in range(num_commodity):
                        net_com_edge_var = com_edge_vars[commodity][point][adj_point]
                        com_edges.append(net_com_edge_var)
                        constraint = Or(Not(net_com_edge_var), net_edge_var)
                        constraints_list.append(constraint)


        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added constraints for edge assignment. [{after_time-prev_time:.2f} s]")
        return constraints_list



    def metal_segment(self):
        prev_time = time.time()

        constraints_list = list()
        constraints_list.append(z3.Bool("metal_segment"))

        nets = self.nets
        metal_edge_vars = self.metal_edge_vars
        metal_point_vars = self.metal_point_vars
        net_edge_vars = self.net_edge_vars
        
        for point in metal_point_vars.keys():
            metal_point_var = metal_point_vars[point]
            metal_edge_var_list = list()
            
            for adj_point in metal_edge_vars[point].keys():
                metal_edge_var = metal_edge_vars[point][adj_point]
                metal_edge_var_list.append(metal_edge_var)
                
                net_edge_var_list = list()
                for net in nets:
                    if net == DUMMY_NET or net.is_power:
                        continue
                    try:
                        net_edge_var = net_edge_vars[net][point][adj_point]
                        net_edge_var_list.append(net_edge_var)
                    except KeyError:
                        continue
                
                if len(net_edge_var_list) > 0:
                    net_edge_var_list = list(set(net_edge_var_list))
                    
                    constraint = (z3.AtMost(*net_edge_var_list, 1))
                    constraints_list.append(constraint)

                    constraint = Equal(metal_edge_var, (Or(*net_edge_var_list)))
                    constraints_list.append(constraint)
                else:
                    constraint = (metal_edge_var == False)
                    constraints_list.append(constraint)
                        
            constraint = Equal(metal_point_var, (Or(*metal_edge_var_list)))
            constraints_list.append(constraint)


        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added constraints for metal segments. [{after_time-prev_time:.2f} s]")
        return constraints_list



    def reserve_pin_points(self):
        prev_time = time.time()
        constraints_list = list()
        constraints_list.append(z3.Bool("reserve_pin_points"))

        nets = self.nets
        net_point_vars = self.net_point_vars
        metal_point_vars = self.metal_point_vars
        
        for net in nets:
            pins = net.get_pins()     
            for pin in pins:
                pin_term = pin.get_term_name()
                points = pin.get_points()
                
                if pin_term in [DRAIN, SOURCE, GATE]:
                    for point in points:
                        if point in net_point_vars[net].keys():
                            constraint = (net_point_vars[net][point] == metal_point_vars[point])
                            constraints_list.append(constraint)


        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added constraints for blocking other pin points. [{after_time-prev_time:.2f} s]")
        return constraints_list
                                            
    
    def exclusiveness_layer(self):
        prev_time = time.time()
        constraints_list = list()
        constraints_list.append(z3.Bool("exclusiveness_layer"))

        metal_edge_vars = self.metal_edge_vars
        net_point_vars = self.net_point_vars
        nets = self.nets
    
        x_points, y_points, only_ver_metal_y_points = self.x_points, self.y_points, self.only_ver_metal_y_points
        for curr_z, curr_layer in enumerate(design_rules["routing_layers"]):            
            for next_layer in design_rules["same_height_layers"][curr_layer]:
                next_z = design_rules["routing_layers"].index(next_layer)
                if curr_z == next_z:
                    continue

                for x_index in range(len(x_points[curr_z])):
                    x = x_points[curr_z][x_index]
                    for y_index in range(len(y_points[curr_z])):   
                        y = y_points[curr_z][y_index]            
                        
                        right_x = get_position(x_points[curr_z], x_index+1)
                        top_y = get_position(y_points[curr_z], y_index+1)

                        u = (x, y, curr_z)
                        v = (x, y, next_z)

                        if u not in metal_edge_vars or \
                           v not in metal_edge_vars:
                            continue

                        for net1 in nets:
                            if net1 == DUMMY_NET or net1.is_power:
                                continue
                            if u in net_point_vars[net1]:
                                u_var = net_point_vars[net1][u]
                                for net2 in nets:
                                    if net2 == DUMMY_NET or net2.is_power:
                                        continue    
                                    if net1 == net2 or \
                                       (POWER_NET in net1.get_name() and POWER_NET in net2.get_name()) or \
                                       (GND_NET in net1.get_name() and GND_NET in net2.get_name()):
                                           
                                        if (curr_layer not in design_rules["no_overlap"]) and \
                                           (next_layer not in design_rules["no_overlap"]):
                                            continue
                                            
                                    if v in net_point_vars[net2]:
                                        v_var = net_point_vars[net2][v]
                                        constraint = (z3.Not(z3.And(u_var, v_var)))
                                        constraints_list.append(constraint)

                        right_u = (right_x, y, curr_z)
                        top_u = (x, top_y, curr_z)

                        right_v = (right_x, y, next_z)
                        top_v = (x, top_y, next_z)

                        right_edge_var1 = metal_edge_vars[u].get(right_u)
                        right_edge_var2 = metal_edge_vars[v].get(right_v)

                        top_edge_var1 = metal_edge_vars[u].get(top_u)
                        top_edge_var2 = metal_edge_vars[v].get(top_v)

                        if right_edge_var1 != None and right_edge_var2 != None:
                            constraint = (z3.Not(z3.And(right_edge_var1, right_edge_var2)))
                            constraints_list.append(constraint)

                        if top_edge_var1 != None and top_edge_var2 != None:
                            constraint = (z3.Not(z3.And(top_edge_var1, top_edge_var2)))
                            constraints_list.append(constraint)
        
        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added constraints for exclusiveness of layers. [{after_time-prev_time:.2f} s]")
        return constraints_list


    def get_spacing_constraint(self, curr_point_var, next_point_var, conditions=None):
        if curr_point_var is None or next_point_var is None:
            return None

        base_constraint = Not(And(curr_point_var, next_point_var))
        if conditions is not None:
            return Or(Not(conditions), base_constraint)
        return base_constraint



    def get_spacing_constraints(self, vars1, vars2, x_points, x_index, y_points, y_index, curr_z, next_z, spacing, is_horizontal, is_layer, need_curr_via_enc, need_next_via_enc):
        if spacing <= 0:    
            return list()
        
        constraints_list = list()

        via_enc_point_vars = self.via_enc_hor_point_vars if is_horizontal else self.via_enc_ver_point_vars

        curr_x = x_points[curr_z][x_index]
        curr_y = y_points[curr_z][y_index]

        curr_point = (curr_x, curr_y, curr_z)
        curr_point_var = vars1.get(curr_point)
        
        if curr_point_var == None:
            return constraints_list

        if is_layer == True:
            curr_layer = design_rules["routing_layers"][curr_z]
            next_layer = design_rules["routing_layers"][next_z]
            
            curr_routing_dir = design_rules["routing_directions"][curr_z]
            next_routing_dir = design_rules["routing_directions"][next_z]
            
            lower_via_of_curr_layer = design_rules["lower_via"][curr_layer]
            upper_via_of_curr_layer = design_rules["upper_via"][curr_layer]

            curr_lower_layer_enc = self.get_layer_enclosure("lower", lower_via_of_curr_layer, curr_layer)
            curr_upper_layer_enc = self.get_layer_enclosure("upper", upper_via_of_curr_layer, curr_layer)
            
            curr_lower_via_enc_var = via_enc_point_vars[LOWER].get(curr_point)
            curr_upper_via_enc_var = via_enc_point_vars[UPPER].get(curr_point)
            
            
        else:
            curr_layer = design_rules["vias"][curr_z]
            next_layer = design_rules["vias"][next_z]
            
            curr_routing_dir = BIDIRECTION
            next_routing_dir = BIDIRECTION


        if ((curr_y % design_rules["cell_height"] == 0) and \
            (curr_layer in design_rules["power_layer"]) and \
            (not is_horizontal)):
            curr_layer_ex = design_rules["power_width"][curr_layer]
        elif ((curr_routing_dir == HORIZONTAL and is_horizontal) or \
            (curr_routing_dir == VERTICAL and not is_horizontal)):
            curr_layer_ex = design_rules["extension"][curr_layer]
        else:
            curr_layer_ex = design_rules["width"][curr_layer]


        for x_i in range(len(x_points[next_z])):
            next_x = x_points[next_z][x_i]
            if (next_x <= curr_x and is_horizontal):
                continue
            
            for y_i in range(len(y_points[next_z])):
                next_y = y_points[next_z][y_i]
                if (next_y <= curr_y and not is_horizontal):
                    continue
                
                next_point = (next_x, next_y, next_z)
                next_point_var = vars2.get(next_point)
                
                if next_point_var == None:
                    #if is_horizontal and curr_y == next_y:
                    #    upper_next_y_point = None
                    #    lower_next_y_point = None
#
                    #    for upper_y_i in range(y_i+1, len(y_points[next_z])):
                    #        upper_next_y = y_points[next_z][upper_y_i]
                    #        upper_next_pont = (next_x, upper_next_y, next_z)
                    #        if upper_next_pont in self.metal_point_vars.keys():
                    #            upper_next_y_point = upper_next_pont
                    #            break
#
                    #    for lower_y_i in range(y_i-1, -1, -1):
                    #        lower_next_y = y_points[next_z][lower_y_i]
                    #        lower_next_point = (next_x, lower_next_y, next_z)
                    #        if lower_next_point in self.metal_point_vars.keys():
                    #            lower_next_y_point = lower_next_point
                    #            break
                    #    
                    #    if upper_next_y_point != None and lower_next_y_point != None:
                    #        next_point_var = self.metal_edge_vars[upper_next_y_point].get(lower_next_y_point)
                    #        print("Hi!: ", curr_point, next_point_var)
#
                    #    else:
                    #        continue
                    continue
                    
                if ((next_y % design_rules["cell_height"] == 0) and \
                   (next_layer in design_rules["power_layer"]) and \
                   (not is_horizontal)):
                    next_layer_ex = design_rules["power_width"][next_layer]
                elif ((next_routing_dir == HORIZONTAL and is_horizontal) or \
                    (next_routing_dir == VERTICAL and not is_horizontal)):
                    next_layer_ex = design_rules["extension"][next_layer]
                else:
                    next_layer_ex = design_rules["width"][next_layer]
    
                overlapped_metal = abs(int(next_y - curr_y) if is_horizontal else int(next_x - curr_x)) - design_rules["width"][curr_layer]/2 - design_rules["width"][next_layer]/2 # negative = overlap, positive = no overlap

                if overlapped_metal >= 0:
                    continue

                if is_layer:
                    lower_via_of_next_layer = design_rules["lower_via"][next_layer]
                    upper_via_of_next_layer = design_rules["upper_via"][next_layer]

                    next_lower_layer_enc = self.get_layer_enclosure("lower", lower_via_of_next_layer, next_layer)
                    next_upper_layer_enc = self.get_layer_enclosure("upper", upper_via_of_next_layer, next_layer)

                    next_lower_via_enc_var = via_enc_point_vars[LOWER].get(next_point)
                    next_upper_via_enc_var = via_enc_point_vars[UPPER].get(next_point)        


                unit_spacing = int(next_x - curr_x) if is_horizontal else int(next_y - curr_y)
                curr_ex = curr_layer_ex/2
                next_ex = next_layer_ex/2

                if (unit_spacing - curr_ex - next_ex) < spacing:
                    constraint = self.get_spacing_constraint(curr_point_var, next_point_var)
                    if constraint != None:
                        constraints_list.append(constraint)
                    
                if is_layer:
                    curr_lower_via_ex = max(curr_lower_layer_enc, curr_ex)
                    curr_upper_via_ex = max(curr_upper_layer_enc, curr_ex)        
                    
                    next_lower_via_ex = max(next_lower_layer_enc, next_ex)
                    next_upper_via_ex = max(next_upper_layer_enc, next_ex)

                    if need_next_via_enc:
                        if (unit_spacing - curr_ex - next_lower_via_ex) < spacing and next_lower_via_enc_var != None:
                            constraint = self.get_spacing_constraint(curr_point_var, next_point_var, conditions=next_lower_via_enc_var)
                            if constraint != None:
                                constraints_list.append(constraint)

                        if (unit_spacing - curr_ex - next_upper_via_ex) < spacing and next_upper_via_enc_var != None:
                            constraint = self.get_spacing_constraint(curr_point_var, next_point_var, conditions=next_upper_via_enc_var)
                            if constraint != None:
                                constraints_list.append(constraint)

                    if need_curr_via_enc:
                        if (unit_spacing - curr_lower_via_ex - next_ex) < spacing and curr_lower_via_enc_var != None:
                            constraint = self.get_spacing_constraint(curr_point_var, next_point_var, conditions=curr_lower_via_enc_var)
                            if constraint != None:
                                constraints_list.append(constraint)

                        if (unit_spacing - curr_upper_via_ex - next_ex) < spacing and curr_upper_via_enc_var != None:
                            constraint = self.get_spacing_constraint(curr_point_var, next_point_var, conditions=curr_upper_via_enc_var)
                            if constraint != None:
                                constraints_list.append(constraint)

                    if need_curr_via_enc and need_next_via_enc:
                        spacing1 = (unit_spacing - curr_lower_via_ex - next_lower_via_ex)
                        spacing2 = (unit_spacing - curr_lower_via_ex - next_upper_via_ex)
                        spacing3 = (unit_spacing - curr_upper_via_ex - next_lower_via_ex)
                        spacing4 = (unit_spacing - curr_upper_via_ex - next_upper_via_ex)
                        
                        if spacing1 < spacing and next_lower_via_enc_var != None and curr_lower_via_enc_var != None:
                            constraint = self.get_spacing_constraint(curr_point_var, next_point_var, conditions=z3.And(next_lower_via_enc_var, curr_lower_via_enc_var))
                            if constraint != None:
                                constraints_list.append(constraint)

                        if spacing2 < spacing and next_upper_via_enc_var != None and curr_lower_via_enc_var != None:
                            constraint = self.get_spacing_constraint(curr_point_var, next_point_var, conditions=z3.And(next_upper_via_enc_var, curr_lower_via_enc_var))
                            if constraint != None:
                                constraints_list.append(constraint)

                        if spacing3 < spacing and curr_upper_via_enc_var != None and next_lower_via_enc_var != None:
                            constraint = self.get_spacing_constraint(curr_point_var, next_point_var, conditions=z3.And(curr_upper_via_enc_var, next_lower_via_enc_var))
                            if constraint != None:
                                constraints_list.append(constraint)

                        if spacing4 < spacing and curr_upper_via_enc_var != None and next_upper_via_enc_var != None:
                            constraint = self.get_spacing_constraint(curr_point_var, next_point_var, conditions=z3.And(curr_upper_via_enc_var, next_upper_via_enc_var))
                            if constraint != None:
                                constraints_list.append(constraint)
                             
                        elif spacing <= min(spacing1, spacing2, spacing3, spacing4):
                            break

        return constraints_list



    def side_to_side_spacing_rule(self):
        prev_time = time.time()
        constraints_list = list()
        x_points, y_points, only_ver_metal_y_points = self.x_points, self.y_points, self.only_ver_metal_y_points
        side_point_vars = self.side_point_vars
                        
        for curr_z, curr_layer in enumerate(design_rules["routing_layers"]):
            for next_z, next_layer in enumerate(design_rules["routing_layers"]):
                spacing = design_rules["spacing"]["S2S"].get(curr_layer, {}).get(next_layer, -1)
                if spacing < 0:
                    continue    
                for x_index in range(len(x_points[curr_z])):
                    for y_index in range(len(y_points[curr_z])):
                        constraints = self.get_spacing_constraints(vars1=side_point_vars[RIGHT], vars2=side_point_vars[LEFT], \
                                                    x_points=x_points, x_index=x_index, y_points=y_points, y_index=y_index, \
                                                    curr_z=curr_z, next_z=next_z, spacing=spacing, is_horizontal=True, is_layer=True, \
                                                    need_curr_via_enc=False, need_next_via_enc=False)
                        
                        constraints_list.extend(constraints)

                        constraints = self.get_spacing_constraints(vars1=side_point_vars[TOP], vars2=side_point_vars[BOTTOM], \
                                                    x_points=x_points, x_index=x_index, y_points=y_points, y_index=y_index, \
                                                    curr_z=curr_z, next_z=next_z, spacing=spacing, is_horizontal=False, is_layer=True, \
                                                    need_curr_via_enc=False, need_next_via_enc=False)                        

                        constraints_list.extend(constraints)
        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added design rule constraints for minimum Side-to-Side spacing. [{after_time-prev_time:.2f} s]")
        return constraints_list



    def tip_to_tip_spacing_rule(self):
        prev_time = time.time()
        constraints_list = list()

        x_points, y_points, only_ver_metal_y_points = self.x_points, self.y_points, self.only_ver_metal_y_points
        tip_point_vars = self.tip_point_vars
        
        for curr_z, curr_layer in enumerate(design_rules["routing_layers"]):
            for next_z, next_layer in enumerate(design_rules["routing_layers"]):
                spacing = design_rules["spacing"]["T2T"].get(curr_layer, {}).get(next_layer, -1)
                if spacing < 0:
                    continue    
                for x_index in range(len(x_points[curr_z])):
                    for y_index in range(len(y_points[curr_z])):
                        constraints = self.get_spacing_constraints(vars1=tip_point_vars[RIGHT], vars2=tip_point_vars[LEFT], \
                                                        x_points=x_points, x_index=x_index, y_points=y_points, y_index=y_index, \
                                                        curr_z=curr_z, next_z=next_z, spacing=spacing, is_horizontal=True, is_layer=True, need_curr_via_enc=True, need_next_via_enc=True)

                        constraints_list.extend(constraints)

                        constraints = self.get_spacing_constraints(vars1=tip_point_vars[TOP], vars2=tip_point_vars[BOTTOM], \
                                                        x_points=x_points, x_index=x_index, y_points=y_points, y_index=y_index, \
                                                        curr_z=curr_z, next_z=next_z, spacing=spacing, is_horizontal=False, is_layer=True, need_curr_via_enc=True, need_next_via_enc=True)       

                        constraints_list.extend(constraints)
        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added design rule constraints for minimum Tip-to-Tip spacing. [{after_time-prev_time:.2f} s]")
        return constraints_list

              
    def side_to_tip_spacing_rule(self):
        prev_time = time.time()

        constraints_list = list()

        x_points, y_points, only_ver_metal_y_points = self.x_points, self.y_points, self.only_ver_metal_y_points
        side_point_vars = self.side_point_vars
        tip_point_vars = self.tip_point_vars
        
        for curr_z, curr_layer in enumerate(design_rules["routing_layers"]):
            for next_z, next_layer in enumerate(design_rules["routing_layers"]):
                spacing = design_rules["spacing"]["S2T"].get(curr_layer, {}).get(next_layer, -1)
                if spacing < 0:
                    continue    
                for x_index in range(len(x_points[curr_z])):
                    for y_index in range(len(y_points[curr_z])):
                        constraints = self.get_spacing_constraints(vars1=side_point_vars[RIGHT], vars2=tip_point_vars[LEFT], \
                                                            x_points=x_points, x_index=x_index, y_points=y_points, y_index=y_index, \
                                                            curr_z=curr_z, next_z=next_z, spacing=spacing, is_horizontal=True, is_layer=True, need_curr_via_enc=False, need_next_via_enc=True)
                        constraints_list.extend(constraints)

                        constraints = self.get_spacing_constraints(vars1=tip_point_vars[RIGHT], vars2=side_point_vars[LEFT], \
                                                            x_points=x_points, x_index=x_index, y_points=y_points, y_index=y_index, \
                                                            curr_z=curr_z, next_z=next_z, spacing=spacing, is_horizontal=True, is_layer=True, need_curr_via_enc=True, need_next_via_enc=False)
                        constraints_list.extend(constraints)

                        constraints = self.get_spacing_constraints(vars1=side_point_vars[TOP], vars2=tip_point_vars[BOTTOM], \
                                                            x_points=x_points, x_index=x_index, y_points=y_points, y_index=y_index, \
                                                            curr_z=curr_z, next_z=next_z, spacing=spacing, is_horizontal=False, is_layer=True, need_curr_via_enc=False, need_next_via_enc=True)      
                        constraints_list.extend(constraints)

                        constraints = self.get_spacing_constraints(vars1=tip_point_vars[TOP], vars2=side_point_vars[BOTTOM], \
                                                            x_points=x_points, x_index=x_index, y_points=y_points, y_index=y_index, \
                                                            curr_z=curr_z, next_z=next_z, spacing=spacing, is_horizontal=False, is_layer=True, need_curr_via_enc=True, need_next_via_enc=False)    
                        constraints_list.extend(constraints)
        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added design rule constraints for minimum Side-to-Tip spacing. [{after_time-prev_time:.2f} s]")
        return constraints_list


    def get_corner_spacing_constraints(self, vars1, vars2, x_points, x_index, y_points, y_index, curr_z, next_z, spacing, is_upward, is_layer):
        def dist(dx, dy):
            return (dx**2 + dy**2) ** 0.5

        if spacing < 0:
            return []

        constraints_list = []
        
        curr_x = x_points[curr_z][x_index]
        curr_y = y_points[curr_z][y_index]
        
        curr_point = (curr_x, curr_y, curr_z)
        curr_point_var = vars1.get(curr_point)

        if curr_point_var is None:
            return constraints_list
        
        if is_layer:
            curr_layer = design_rules["routing_layers"][curr_z]
            next_layer = design_rules["routing_layers"][next_z]
            lower_via_curr = design_rules["lower_via"][curr_layer]
            upper_via_curr = design_rules["upper_via"][curr_layer]
            lower_via_next = design_rules["lower_via"][next_layer]
            upper_via_next = design_rules["upper_via"][next_layer]
        else:
            curr_layer = design_rules["vias"][curr_z]
            next_layer = design_rules["vias"][next_z]

        curr_x_ex = max(design_rules["width"][curr_layer], design_rules["extension"][curr_layer])
        for order, curr_dir in enumerate([HORIZONTAL, VERTICAL]):
            if not is_layer and order == 1:
                break
            if is_layer and design_rules["routing_directions"][curr_z] not in (BIDIRECTION, curr_dir):
                continue

            if is_layer:
                curr_via_enc_vars = (self.via_enc_hor_point_vars if curr_dir == HORIZONTAL else self.via_enc_ver_point_vars)
                curr_lower_layer_enc = self.get_layer_enclosure("lower", lower_via_curr, curr_layer)
                curr_upper_layer_enc = self.get_layer_enclosure("upper", upper_via_curr, curr_layer)
                curr_lower_via_enc_var = curr_via_enc_vars[LOWER].get(curr_point)
                curr_upper_via_enc_var = curr_via_enc_vars[UPPER].get(curr_point)
            
            for order, next_dir in enumerate([HORIZONTAL, VERTICAL]):
                if not is_layer and order == 1:
                    break

                if is_layer and design_rules["routing_directions"][next_z] not in (BIDIRECTION, next_dir):
                    continue

                for x_i in range(len(x_points[next_z])):
                    next_x = x_points[next_z][x_i]
                    
                    if next_x <= curr_x:
                        continue

                    
                    next_x_ex = max(design_rules["width"][next_layer], design_rules["extension"][next_layer])
                    for y_i in range(len(y_points[next_z])):
                        next_y = get_position(y_points[next_z], y_i) if is_upward else \
                                 get_position(y_points[next_z], (len(y_points[next_z])-1)-y_i)
                        
                        if next_y == None \
                            or (is_upward and next_y <= curr_y) \
                            or (not is_upward and next_y >= curr_y):
                            continue
                        
                        curr_y_ex = design_rules["power_width"][curr_layer] if (curr_y % design_rules["cell_height"] == 0 and curr_layer in design_rules["power_layer"]) else curr_x_ex
                        next_y_ex = design_rules["power_width"][next_layer] if (next_y % design_rules["cell_height"] == 0 and next_layer in design_rules["power_layer"]) else next_x_ex

                        next_point = (next_x, next_y, next_z)
                        next_point_var = vars2.get(next_point)
                        
                        x_unit_spacing = next_x - curr_x
                        y_unit_spacing = (next_y - curr_y) if is_upward else (curr_y - next_y)
                        basic_spacing = dist(x_unit_spacing - curr_x_ex/2 - next_x_ex/2, y_unit_spacing - curr_y_ex/2 - next_y_ex/2)
                        if basic_spacing < spacing:
                            constraint = self.get_spacing_constraint(curr_point_var, next_point_var)
                            if constraint is not None:
                                constraints_list.append(constraint)
                        
                        if is_layer:
                            next_via_enc_vars = (self.via_enc_hor_point_vars if next_dir == HORIZONTAL else self.via_enc_ver_point_vars)
                            next_lower_layer_enc = self.get_layer_enclosure("lower", lower_via_next, next_layer)
                            next_upper_layer_enc = self.get_layer_enclosure("upper", upper_via_next, next_layer)
                            next_lower_via_enc_var = next_via_enc_vars[LOWER].get(next_point)
                            next_upper_via_enc_var = next_via_enc_vars[UPPER].get(next_point)
                            
                            for (layer_ex1, layer_ex2, curr_via_enc_var, next_via_enc_var) in [
                                (curr_lower_layer_enc, next_lower_layer_enc, curr_lower_via_enc_var, next_lower_via_enc_var),
                                (curr_upper_layer_enc, next_upper_layer_enc, curr_upper_via_enc_var, next_upper_via_enc_var)
                            ]:
                                for axis, dx_mask, dy_mask in [(HORIZONTAL, 1, 0), (VERTICAL, 0, 1)]:
                                    if curr_dir == axis:
                                        x_ex = (curr_x_ex/2 if not dx_mask else max(layer_ex1, curr_x_ex/2))
                                        y_ex = (curr_y_ex/2 if not dy_mask else max(layer_ex1, curr_y_ex/2))
                                        
                                        new_spacing = dist(x_unit_spacing - next_x_ex/2 - x_ex,
                                                            y_unit_spacing - next_y_ex/2 - y_ex)
                                        if new_spacing < spacing:
                                            constraint = self.get_spacing_constraint(curr_point_var, next_point_var, conditions=curr_via_enc_var)
                                            if constraint is not None:
                                                constraints_list.append(constraint)
                                    if next_dir == axis:
                                        x_ex = (next_x_ex/2 if not dx_mask 
                                                else max(layer_ex2, next_x_ex/2))
                                        y_ex = (next_y_ex/2 if not dy_mask 
                                                else max(layer_ex2, next_y_ex/2))
                                        new_spacing = dist(x_unit_spacing - curr_x_ex/2 - x_ex,
                                                            y_unit_spacing - curr_y_ex/2 - y_ex)
                                        if new_spacing < spacing:
                                            constraint = self.get_spacing_constraint(curr_point_var, next_point_var, conditions=next_via_enc_var)
                                            if constraint is not None:
                                                constraints_list.append(constraint)
                                    
                                    for axis2, dx_mask2, dy_mask2 in [(HORIZONTAL, 1, 0), (VERTICAL, 0, 1)]:
                                        if curr_dir == axis and next_dir == axis2:
                                            x_ex1 = (curr_x_ex/2 if not dx_mask else max(layer_ex1, curr_x_ex/2))
                                            y_ex1 = (curr_y_ex/2 if not dy_mask else max(layer_ex1, curr_y_ex/2))
                                            x_ex2 = (next_x_ex/2 if not dx_mask2 else max(layer_ex2, next_x_ex/2))
                                            y_ex2 = (next_y_ex/2 if not dy_mask2 else max(layer_ex2, next_y_ex/2))
                                            
                                            new_spacing = dist(x_unit_spacing - x_ex1 - x_ex2, y_unit_spacing - y_ex1 - y_ex2)
                                            if new_spacing < spacing and curr_via_enc_var is not None and next_via_enc_var is not None:
                                                constraint = self.get_spacing_constraint(curr_point_var, next_point_var,
                                                                                        conditions=z3.And(curr_via_enc_var, next_via_enc_var))
                                                if constraint is not None:
                                                    constraints_list.append(constraint)
                            
                            for (layer_ex1, layer_ex2, curr_via_enc_var, next_via_enc_var) in [
                                (curr_lower_layer_enc, next_upper_layer_enc, curr_lower_via_enc_var, next_upper_via_enc_var),
                                (curr_upper_layer_enc, next_lower_layer_enc, curr_upper_via_enc_var, next_lower_via_enc_var)
                            ]:
                                for axis1, dx_mask1, dy_mask1 in [(HORIZONTAL, 1, 0), (VERTICAL, 0, 1)]:
                                    for axis2, dx_mask2, dy_mask2 in [(HORIZONTAL, 1, 0), (VERTICAL, 0, 1)]:
                                        if curr_dir == axis1 and next_dir == axis2:
                                            x_ex1 = (curr_x_ex/2 if not dx_mask1 else max(layer_ex1, curr_x_ex/2))
                                            y_ex1 = (curr_y_ex/2 if not dy_mask1 else max(layer_ex1, curr_y_ex/2))
                                            x_ex2 = (next_x_ex/2 if not dx_mask2 else max(layer_ex2, next_x_ex/2))
                                            y_ex2 = (next_y_ex/2 if not dy_mask2 else max(layer_ex2, next_y_ex/2))
                                            new_spacing = dist(x_unit_spacing - x_ex1 - x_ex2, y_unit_spacing - y_ex1 - y_ex2)
                                            if new_spacing < spacing and curr_via_enc_var is not None and next_via_enc_var is not None:
                                                constraint = self.get_spacing_constraint(curr_point_var, next_point_var,
                                                                                        conditions=z3.And(curr_via_enc_var, next_via_enc_var))
                                                if constraint is not None:
                                                    constraints_list.append(constraint)
        
        return constraints_list


    def corner_to_corner_spacing_rule(self):
        prev_time = time.time()
        constraints_list = list()

        x_points, y_points, only_ver_metal_y_points = self.x_points, self.y_points, self.only_ver_metal_y_points
        corner_point_vars = self.corner_point_vars
        
        for curr_z, curr_layer in enumerate(design_rules["routing_layers"]):
            for next_z, next_layer in enumerate(design_rules["routing_layers"]):
                spacing = design_rules["spacing"]["C2C"].get(curr_layer, {}).get(next_layer, -1)
                if spacing < 0:
                    continue
                
                for x_index in range(len(x_points[curr_z])):
                    for y_index in range(len(y_points[curr_z])):      
                        constraints = self.get_corner_spacing_constraints(vars1=corner_point_vars[TOP_RIGHT], vars2=corner_point_vars[BOTTOM_LEFT], \
                            x_points=x_points, x_index=x_index, y_points=y_points, y_index=y_index, \
                                curr_z=curr_z, next_z=next_z, spacing=spacing, is_upward=True, is_layer=True)
                        
                        constraints_list.extend(constraints)

                        constraints = self.get_corner_spacing_constraints(vars1=corner_point_vars[BOTTOM_RIGHT], vars2=corner_point_vars[TOP_LEFT], \
                            x_points=x_points, x_index=x_index, y_points=y_points, y_index=y_index, \
                                curr_z=curr_z, next_z=next_z, spacing=spacing, is_upward=False, is_layer=True)
                        
                        constraints_list.extend(constraints)
        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added design rule constraints for minimum Corner-to-Corner spacing. [{after_time-prev_time:.2f} s]")
        return constraints_list


    def via_spacing_rule(self):
        prev_time = time.time()
        constraints_list = list()

        x_points, y_points, only_ver_metal_y_points = self.x_points, self.y_points, self.only_ver_metal_y_points
        via_x_points, via_y_points = [None] * len(design_rules["vias"]), [None] * len(design_rules["vias"])

        via_point_vars = self.via_point_vars
        
        for z, via in enumerate(design_rules["vias"]):
            via_x_points[z] = set()
            via_y_points[z] = set()

            if via in design_rules["lower_layers"]:
                lower_layers = design_rules["lower_layers"][via]
            else:
                logger.log(level=logging.INFO, msg=f"Lower layer of via {via} not found.")

            if via in design_rules["upper_layers"]:    
                upper_layers = design_rules["upper_layers"][via]
            else:
                logger.log(level=logging.INFO, msg=f"Upper layer of via {via} not found.")

            for lower_layer in lower_layers:
                for upper_layer in upper_layers:
                    if lower_layer in design_rules["routing_layers"]:
                        lower_layer_index = design_rules["routing_layers"].index(lower_layer)
                    else:
                        logger.log(level=logging.INFO, msg=f"Lower layer {lower_layer} of via {via} not found.")
                        continue
                    
                    if upper_layer in design_rules["routing_layers"]:
                        upper_layer_index = design_rules["routing_layers"].index(upper_layer)
                    else:
                        logger.log(level=logging.INFO, msg=f"Upper layer {upper_layer} of via {via} not found.")
                        continue

                    via_x_points[z] = via_x_points[z] | (set(x_points[lower_layer_index]) & set(x_points[upper_layer_index]))
                    via_y_points[z] = via_y_points[z] | (set(y_points[lower_layer_index]) & set(y_points[upper_layer_index]))
            via_x_points[z] = sorted(list(via_x_points[z]))
            via_y_points[z] = sorted(list(via_y_points[z]))

        for curr_z, curr_layer in enumerate(design_rules["vias"]):
            for next_z, next_layer in enumerate(design_rules["vias"]):
                T2T_spacing = design_rules["spacing"]["T2T"].get(curr_layer, {}).get(next_layer, -1)
                C2C_spacing = design_rules["spacing"]["C2C"].get(curr_layer, {}).get(next_layer, -1)
                
                if T2T_spacing < 0 and C2C_spacing < 0:
                    continue
                
                for x_index in range(len(via_x_points[curr_z])):
                    for y_index in range(len(via_y_points[curr_z])):
                        constraints = self.get_spacing_constraints(vars1=via_point_vars, vars2=via_point_vars, \
                            x_points=via_x_points, x_index=x_index, y_points=via_y_points, y_index=y_index, \
                            curr_z=curr_z, next_z=next_z, spacing=T2T_spacing, is_horizontal=True, is_layer=False, need_curr_via_enc=False, need_next_via_enc=False) 

                        constraints_list.extend(constraints)

                        constraints = self.get_spacing_constraints(vars1=via_point_vars, vars2=via_point_vars, \
                            x_points=via_x_points, x_index=x_index, y_points=via_y_points, y_index=y_index, \
                            curr_z=curr_z, next_z=next_z, spacing=T2T_spacing, is_horizontal=False, is_layer=False, need_curr_via_enc=False, need_next_via_enc=False)

                        constraints_list.extend(constraints)

                        constraints = self.get_corner_spacing_constraints(vars1=via_point_vars, vars2=via_point_vars, \
                            x_points=via_x_points, x_index=x_index, y_points=via_y_points, y_index=y_index, \
                            curr_z=curr_z, next_z=next_z, spacing=C2C_spacing, is_upward=True, is_layer=False)
                        
                        constraints_list.extend(constraints)

                        constraints = self.get_corner_spacing_constraints(vars1=via_point_vars, vars2=via_point_vars, \
                            x_points=via_x_points, x_index=x_index, y_points=via_y_points, y_index=y_index, \
                            curr_z=curr_z, next_z=next_z, spacing=C2C_spacing, is_upward=False, is_layer=False)

                        constraints_list.extend(constraints)
        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added design rule constraints for minimum Via-to-Via spacing. [{after_time-prev_time:.2f} s]")
        return constraints_list


    def get_area_constraint(self, vars, x_points, x_index, y_points, y_index, point, min_length):
        x, y, z = point
        layer = design_rules["routing_layers"][z]
        routing_dir = design_rules["routing_directions"][z]
        
        hor_vars = list()
        hor_curr_point_var = vars[LEFT].get((x, y, z))
        for i in range(x_index, len(x_points[z])):
            next_point = x_points[z][i]
            
            layer_extension = design_rules["extension"][layer] if routing_dir == HORIZONTAL else design_rules["width"][layer]
            hor_length_position = (next_point - x) + layer_extension     

            if hor_length_position < min_length:
                hor_next_point_var = vars[RIGHT].get((next_point, y, z)) 
                if hor_curr_point_var != None and hor_next_point_var != None:
                    hor_var = z3.Not(z3.And(hor_curr_point_var, hor_next_point_var))
                    hor_vars.append(hor_var)
            else:
                break
                    
        ver_vars = list()
        ver_curr_point_var = vars[BOTTOM].get((x, y, z))
        for i in range(y_index, len(y_points[z])):
            next_point = y_points[z][i]
            
            layer_extension = design_rules["extension"][layer] if routing_dir == VERTICAL else design_rules["width"][layer]
            ver_length_position = (next_point - y) + layer_extension

            if ver_length_position < min_length:
                ver_next_point_var = vars[TOP].get((x, next_point, z))
                if ver_curr_point_var != None and ver_next_point_var != None:
                    ver_var = z3.Not(z3.And(ver_curr_point_var, ver_next_point_var))
                    ver_vars.append(ver_var)
            else:
                break
        
        if hor_vars and ver_vars:
            return Or(*hor_vars, *ver_vars)
        
        elif hor_vars:
            return Or(*hor_vars)
        
        elif ver_vars:
            return Or(*ver_vars)
        

    def minimum_area_rule(self):
        prev_time = time.time()
        constraints_list = list()

        x_points, y_points, only_ver_metal_y_points = self.x_points, self.y_points, self.only_ver_metal_y_points
        tip_point_vars = self.tip_point_vars
        
        for z, layer in enumerate(design_rules["routing_layers"]):
            length = design_rules["min_area"][layer]/design_rules["width"][layer]
            for x_index in range(len(x_points[z])):
                x = x_points[z][x_index]
                for y_index in range(len(y_points[z])):
                    y = y_points[z][y_index]

                    constraint = self.get_area_constraint(tip_point_vars, x_points, x_index, y_points, y_index, (x, y, z), length)
                    if constraint != None:
                        constraints_list.append(constraint)
        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added design rule constraints for minimum area. [{after_time-prev_time:.2f} s]")
        return constraints_list


    def minimum_pin_length(self):
        prev_time = time.time()
        constraints_list = list()
        constraints_list.append(z3.Bool("minimum_pin_length"))

        x_points, y_points, only_ver_metal_y_points = self.x_points, self.y_points, self.only_ver_metal_y_points
        net_com_edge_vars = self.net_com_edge_vars
        net_edge_vars = self.net_edge_vars
        nets = self.nets
        
        for net in nets:
            if net.is_power or not net.is_ext_pin:
                continue
            
            pins = net.get_pins()

            outer_pin = None
            outer_pin_comm = None
            for i, pin in enumerate(pins):
                if pin.get_term_name() == EXT_PIN:
                    outer_pin = pin
                    outer_pin_comm = i - 1 
                    break
            
            if outer_pin == None:
                logger.log(level=logging.ERROR, msg=f"External pin not found for net {net.name}.")
                exit()

            outer_points = outer_pin.get_points()
            
            min_x = min(coord[0] for coord in outer_points)
            max_x = max(coord[0] for coord in outer_points)
            
            outer_pin_ys = set([y for (_, y, _) in outer_points])

            zs = [design_rules["routing_layers"].index(layer) for layer in design_rules["ext_pin_layer"]]
            for z in zs:
                routing_dir = design_rules["routing_directions"][z]
                layer = design_rules["routing_layers"][z]

                if design_rules["minimum_pin_length"] <= design_rules["width"][layer]:
                    break

                for x_index in range(len(x_points[z])):
                    x = x_points[z][x_index]
                    
                    if not (min_x <= x <= max_x):
                        continue
                    
                    for outer_pin_y in outer_pin_ys:
                        outer_pin_y_index = y_points[z].index(outer_pin_y)
                        y_pin_edge_vars = list()
                        for y_index in range(outer_pin_y_index, -1, -1):
                            curr_y = y_points[z][y_index]
                            
                            final_y = get_position(y_points[z], len(y_points[z])-1)
                            
                            layer_extension = design_rules["extension"][design_rules["ext_pin_layer"][0]] if routing_dir == VERTICAL else \
                                              design_rules["width"][design_rules["ext_pin_layer"][0]]
                            
                            length = (final_y - curr_y) + layer_extension
                            if length < design_rules["minimum_pin_length"]:
                                break
                            
                            pin_edges = list()
                            curr_point = (x, curr_y, z)
                            for i in range(y_index+1, len(y_points[z])):
                                next_y = y_points[z][i]
                                next_point = (x, next_y, z)
                                
                                net_edge = (curr_point, next_point)
                                net_edge_var = net_edge_vars[net].get(curr_point, {}).get(next_point)
                                
                                if net_edge_var == None:
                                    #pin_edges = list()
                                    continue

                                pin_edges.append(net_edge)
                                curr_point = next_point

                                length = (next_y - curr_y) + layer_extension
                                if design_rules["minimum_pin_length"] <= length:
                                    if outer_pin_y != None and \
                                        any(u[Y] == outer_pin_y or v[Y] == outer_pin_y for u, v in pin_edges):    
                                        break
                                    else:
                                        continue
                            
                            #if outer_pin_y != None and \
                            #    any(u[Y] == outer_pin_y or v[Y] == outer_pin_y for u, v in pin_edges):
                            #    pass
                            #else:
                            #    pin_edges = []
                            
                            pin_edge_vars = list()
                            for u, v in pin_edges:
                                pin_edge_vars.append(net_edge_vars[net].get(u, {}).get(v))
                                
                            if len(pin_edge_vars) > 0:
                                ext_pin_var = z3.Bool(f"EXT_PIN_{net}_{x}_{curr_y}_{z}")
                                constraint = (ext_pin_var == z3.And(*pin_edge_vars))
                                constraints_list.append(constraint)

                                pin_edge_vars = z3.And(*pin_edge_vars)
                                y_pin_edge_vars.append(pin_edge_vars)
                            
                            
                        outer_pin = Or(*y_pin_edge_vars)
                        
                        pin_point = (x, outer_pin_y, z)

                        adj_points = net_com_edge_vars[net][outer_pin_comm][pin_point].keys()
                        
                        boundary_edges = list()
                        for adj_point in adj_points:
                            if adj_point not in outer_points:
                                net_com_edge_var = net_com_edge_vars[net][outer_pin_comm][pin_point][adj_point]
                                boundary_edges.append(net_com_edge_var)
                        
                        for boundary_edge in boundary_edges:
                            constraint = Or(Not(boundary_edge), outer_pin)
                            constraints_list.append(constraint)
                            
        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added design rule constraints for minimum pin length. [{after_time-prev_time:.2f} s]")
        return constraints_list


    def add_via_enclosure_rule(self):
        prev_time = time.time()
        constraints_list = list()
        metal_edge_vars = self.metal_edge_vars
        
        via_enc_ver_point_vars = self.via_enc_ver_point_vars
        via_enc_hor_point_vars = self.via_enc_hor_point_vars
        
        x_points, y_points, only_ver_metal_y_points = self.x_points, self.y_points, self.only_ver_metal_y_points
        
        for z, layer in enumerate(design_rules["routing_layers"]):
            for x_index in range(len(x_points[z])):
                x = x_points[z][x_index]
                
                left_x = get_position(x_points[z], x_index-1)
                right_x = get_position(x_points[z], x_index+1)
                
                for y_index in range(len(y_points[z])): 
                    y = y_points[z][y_index]
                    
                    bottom_y = get_position(y_points[z], y_index-1)
                    top_y = get_position(y_points[z], y_index+1)

                    point = (x, y, z)

                    right_point = get_point(right_x, y, z)
                    left_point = get_point(left_x, y, z)
                    top_point = get_point(x, top_y, z)
                    bottom_point = get_point(x, bottom_y, z)

                    lower_zs = list()
                    lower_layers = design_rules["lower_layers"][layer]
                    if lower_layers != None:
                        for lower_layer in lower_layers:
                            if lower_layer in design_rules["routing_layers"]:
                                lower_zs.append(design_rules["routing_layers"].index(lower_layer))
                    
                    upper_zs = list()
                    upper_layers = design_rules["upper_layers"][layer]
                    if upper_layers != None:        
                        for upper_layer in upper_layers:     
                            if upper_layer in design_rules["routing_layers"]:      
                                upper_zs.append(design_rules["routing_layers"].index(upper_layer))
                    
                    lower_points = [(x, y, lower_z) for lower_z in lower_zs]
                    upper_points = [(x, y, upper_z) for upper_z in upper_zs]
          
                    right_metal_edge = metal_edge_vars[point].get(right_point)
                    left_metal_edge = metal_edge_vars[point].get(left_point)
                    top_metal_edge = metal_edge_vars[point].get(top_point)
                    bottom_metal_edge = metal_edge_vars[point].get(bottom_point)   

                    lower_metal_edges = [
                        metal_edge_vars[point].get(lower_point) 
                        for lower_point in lower_points 
                        if metal_edge_vars[point].get(lower_point) != None
                    ]

                    upper_metal_edges = [
                        metal_edge_vars[point].get(upper_point) 
                        for upper_point in upper_points 
                        if metal_edge_vars[point].get(upper_point) != None
                    ]     

                    routing_direction = design_rules["routing_directions"][z]
                    if routing_direction == VERTICAL:
                        constraint = Not(Or(via_enc_hor_point_vars[LOWER].get(point), via_enc_hor_point_vars[UPPER].get(point)))
                        constraints_list.append(constraint)

                    elif routing_direction == HORIZONTAL:
                        constraint = Not(Or(via_enc_ver_point_vars[LOWER].get(point), via_enc_ver_point_vars[UPPER].get(point)))
                        constraints_list.append(constraint)

                    
                    constraint = Equal((Or(*lower_metal_edges)), (Or(via_enc_hor_point_vars[LOWER].get(point), via_enc_ver_point_vars[LOWER].get(point))))
                    constraints_list.append(constraint)

                    
                    constraint = Equal((Or(*upper_metal_edges)), (Or(via_enc_hor_point_vars[UPPER].get(point), via_enc_ver_point_vars[UPPER].get(point))))
                    constraints_list.append(constraint)


                    constraint = Not(And(Or(via_enc_hor_point_vars[LOWER].get(point), via_enc_hor_point_vars[UPPER].get(point)), 
                                         Or(via_enc_ver_point_vars[LOWER].get(point), via_enc_ver_point_vars[UPPER].get(point))))
                    constraints_list.append(constraint)


                    constraint = Or(Not(Or(right_metal_edge, left_metal_edge)), Not(Or(via_enc_ver_point_vars[LOWER].get(point), via_enc_ver_point_vars[UPPER].get(point))))                                
                    constraints_list.append(constraint)

                
                    constraint = Or(Not(Or(top_metal_edge, bottom_metal_edge)), Not(Or(via_enc_hor_point_vars[LOWER].get(point), via_enc_hor_point_vars[UPPER].get(point))))
                    constraints_list.append(constraint)

        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added design rule constraints for via enclosure metals. [{after_time-prev_time:.2f} s]")
        return constraints_list

       
       
    def add_no_cross_via(self):
        prev_time = time.time()

        constraints_list = list()
        constraints_list.append(z3.Bool("no_via_cross"))
        metal_point_vars = self.metal_point_vars
        
        metal_ver_point_vars = self.metal_ver_point_vars
        metal_hor_point_vars = self.metal_hor_point_vars

        via_enc_ver_point_vars = self.via_enc_ver_point_vars
        via_enc_hor_point_vars = self.via_enc_hor_point_vars
        
        x_points, y_points = self.x_points, self.y_points
        for curr_z, layer in enumerate(design_rules["routing_layers"]):
            layer_width = design_rules["width"][layer]
            #for via_dir in ["upper_via", "lower_via"]:
            for via_dir in ["lower_via"]:
                via_layer = design_rules[via_dir][layer]
                if via_layer == None:
                    continue
                via_width = design_rules["width"][via_layer]
                
                via_dir_ = LOWER if via_dir == "lower_via" else UPPER
                
                for via_x_index in range(len(x_points[curr_z])):
                    center_via_x = x_points[curr_z][via_x_index]
                    
                    min_via_x = center_via_x - via_width/2
                    max_via_x = center_via_x + via_width/2
                                        
                    for via_y_index in range(len(y_points[curr_z])):
                        center_via_y = y_points[curr_z][via_y_index]
                        
                        min_via_y = center_via_y - via_width/2
                        max_via_y = center_via_y + via_width/2
                        
                        curr_point = (center_via_x, center_via_y, curr_z)

                        via_enc_ver_point_var = via_enc_ver_point_vars[via_dir_].get(curr_point)
                        via_enc_hor_point_var = via_enc_hor_point_vars[via_dir_].get(curr_point)
                        
                        
                        for metal_x_index in range(len(x_points[curr_z])):
                            center_metal_x = x_points[curr_z][metal_x_index]
                            
                            min_metal_x = center_metal_x - layer_width/2
                            max_metal_x = center_metal_x + layer_width/2
                            
                            # No x overlap
                            if max_via_x < min_metal_x or max_metal_x < min_via_x:
                                continue
                            
                            elif center_metal_x <= center_via_x:
                                x_mode = "MATCH"
                            
                            else:
                                x_mode = "OVERLAP"

                            for metal_y_index in range(len(y_points[curr_z])):
                                center_metal_y = y_points[curr_z][metal_y_index]
                                
                                min_metal_y = center_metal_y - layer_width/2
                                max_metal_y = center_metal_y + layer_width/2
                                
                                metal_point = (center_metal_x, center_metal_y, curr_z)
                                
                                if metal_point not in metal_point_vars.keys():
                                    continue

                                # No y overlap
                                if max_via_y < min_metal_y or max_metal_y < min_via_y:
                                    continue
                                
                                elif center_metal_y == center_via_y:
                                    y_mode = "MATCH"
                                
                                else:
                                    y_mode = "OVERLAP"
                                    
                                
                                if x_mode == "MATCH" and y_mode == "MATCH":
                                    ver_const = Or(Not(via_enc_ver_point_var), Not(metal_hor_point_vars[metal_point]))
                                    hor_const = Or(Not(via_enc_hor_point_var), Not(metal_ver_point_vars[metal_point]))
                                
                                elif x_mode == "MATCH" and y_mode == "OVERLAP":
                                    ver_const = Or(Not(via_enc_ver_point_var), Not(metal_hor_point_vars[metal_point]))
                                    hor_const = Or(Not(via_enc_hor_point_var), Not(metal_point_vars[metal_point]))

                                elif x_mode == "OVERLAP" and y_mode == "MATCH":
                                    ver_const = Or(Not(via_enc_ver_point_var), Not(metal_point_vars[metal_point]))
                                    hor_const = Or(Not(via_enc_hor_point_var), Not(metal_ver_point_vars[metal_point]))
                                    
                                elif x_mode == "OVERLAP" and y_mode == "OVERLAP":
                                    ver_const = Or(Not(via_enc_ver_point_var), Not(metal_point_vars[metal_point]))
                                    hor_const = Or(Not(via_enc_hor_point_var), Not(metal_point_vars[metal_point]))
                                    
                                constraints_list.append(ver_const)
                                constraints_list.append(hor_const) 
                            
        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added constraints to prevent the existence of two orthogonal routing layers at via locations. [{after_time-prev_time:.2f} s]")
        return constraints_list



    def set_optimize(self, solver):   
        prev_time = time.time()        
        metal_edge_vars = self.metal_edge_vars
        metal_point_vars = self.metal_point_vars
        layers = design_rules["routing_layers"]

        def minimize_metal_edge(layer_num, direction, i, n, solver):
            if direction == HORIZONTAL:
                moving_axis = X
            elif direction == VERTICAL:
                moving_axis = Y  
            elif direction == BIDIRECTION:
                moving_axis = Y # it can be X as well.

            metals = set()
            points = list(metal_point_vars.keys())

            layer = layers[layer_num]
            same_height_layers = design_rules["same_height_layers"][layer]
            for same_layer in same_height_layers:
                same_layer_num = layers.index(same_layer)
                for point in points:
                    if point[Z] != same_layer_num:
                        continue
                    
                    adj_points = list(metal_edge_vars[point].keys())
                    for adj_point in adj_points:
                        if adj_point[Z] != same_layer_num or adj_point < point or (direction != BIDIRECTION and point[moving_axis] == adj_point[moving_axis]):
                            continue
                        edge = (point, adj_point)
                        metals.add(edge)

            metals = list(metals)
            
            gate_contact_y = int(design_rules["cell_height"]/2 + design_rules["np_offset"])

            priorities = set()

            metal_vars = list()
            total_metal_length = 0
            for point, adj_point in metals:
                ext_pin_zs = [design_rules["routing_layers"].index(layer) for layer in design_rules["ext_pin_layer"]]
                is_ext_pin_layer = (point[Z] in ext_pin_zs and adj_point[Z] in ext_pin_zs)
                if is_ext_pin_layer and design_rules["pin_stretch_aware"]:
                    dist0 = abs(gate_contact_y - point[Y])
                    dist1 = abs(gate_contact_y - adj_point[Y])
                    
                    priority = min(dist0, dist1)
                else:
                    priority = 0
                
                priorities.add(priority)

                metal_length = abs(point[X]-adj_point[X]) + abs(point[Y]-adj_point[Y])
                metal_vars.append((metal_length, priority, metal_edge_vars[point][adj_point]))
                total_metal_length += metal_length

            if len(metal_vars) == 0:
                return

            priorities = sorted(list(priorities))

            #num_bits = total_metal_length.bit_length()
            #metal_vars_sum_expr = z3.Sum([z3.If(var, z3.BitVecVal(length, num_bits), z3.BitVecVal(0, num_bits)) for (length, var) in metal_vars])
            
            for target_priority in priorities:
                metal_vars_sum_expr = list()
                for (length, priority, var) in metal_vars:
                    #weight = 1 + priorities.index(priority) * 0.1
                    #weighted_length = int(length / weight)
                    if priority == target_priority:
                        metal_vars_sum_expr.append(z3.If(var, z3.IntVal(length), z3.IntVal(0)))

                metal_vars_sum_expr = z3.Sum(metal_vars_sum_expr)
                solver.minimize(metal_vars_sum_expr)


        def minimize_z_metal_edge(layer_num, solver):
            metals = set()
            points = list(metal_point_vars.keys())

            layer = layers[layer_num]
            same_height_layers = design_rules["same_height_layers"][layer]
            for layer_ in same_height_layers:
                same_layer_num = layers.index(layer_)
                for point in points:
                    if point[Z] != same_layer_num:
                        continue

                    adj_points = list(metal_edge_vars[point].keys())
                    for adj_point in adj_points:
                        if adj_point[Z] >= same_layer_num:
                            continue
                        edge = (point, adj_point)
                        metals.add(edge)

            metals = list(metals)
            metal_vars = [metal_edge_vars[point][adj_point] for point, adj_point in metals]

            if len(metal_vars) == 0:
                return

            #num_bits = len(metal_vars).bit_length()
            #metal_vars_sum_expr = z3.Sum([z3.If(var, z3.BitVecVal(1, num_bits), z3.BitVecVal(0, num_bits)) for var in metal_vars])            
            metal_vars_sum_expr = z3.Sum([z3.If(var, z3.IntVal(1), z3.IntVal(0)) for var in metal_vars])
            solver.minimize(metal_vars_sum_expr)
            #solver.maximize(metal_vars_sum_expr)


        done_layers = list()
        for layer_num in reversed(range(len(layers))):
            layer = layers[layer_num]
            same_height_layers = design_rules["same_height_layers"][layer]
            same_layer_nums = [layers.index(layer) for layer in same_height_layers]

            if layer_num not in done_layers:
                #### Via First
                minimize_z_metal_edge(layer_num, solver)
                if "BI" in design_rules["metal_direction_priority"]:
                    minimize_metal_edge(layer_num, BIDIRECTION, 1, 1, solver)
                elif "VER" in design_rules["metal_direction_priority"]:
                    minimize_metal_edge(layer_num, VERTICAL, 1, 1, solver)
                    minimize_metal_edge(layer_num, HORIZONTAL, 1, 1, solver)
                elif "HOR" in design_rules["metal_direction_priority"]:
                    minimize_metal_edge(layer_num, HORIZONTAL, 1, 1, solver)
                    minimize_metal_edge(layer_num, VERTICAL, 1, 1, solver)
                
                done_layers.extend(same_layer_nums)

        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"Added objective functions [{after_time-prev_time:.2f} s]")


    def check_sat(self, solver):
        prev_time = time.time()
        metal_edge_vars = self.metal_edge_vars
        net_com_edge_vars = self.net_com_edge_vars
        net_point_vars = self.net_point_vars
        net_edge_vars = self.net_edge_vars

        metal_point_vars = self.metal_point_vars
        via_enc_hor_point_vars = self.via_enc_hor_point_vars
        via_enc_ver_point_vars = self.via_enc_ver_point_vars
        
        masking_metals = set()

        is_sat = False
        
        if solver.check() == z3.sat:
            is_sat = True

            model = solver.model()
            metals = list()
            metals_without_net = set()
            
            external_pin_point = dict()
            
            hor_lower_via_enc = list()
            hor_upper_via_enc = list()
            ver_lower_via_enc = list()
            ver_upper_via_enc = list()
            
            with open(f'{self.save_dir}/{self.cell_name}/{self.cell_name}.z3', 'w') as f:
                for decl in model.decls():
                    f.write(f'{decl.name()} = {model[decl]}\n')
            
            for point in list(metal_point_vars.keys()):
                if model[via_enc_hor_point_vars[LOWER][point]] == True:
                    hor_lower_via_enc.append(point)

                if model[via_enc_hor_point_vars[UPPER][point]] == True:
                    hor_upper_via_enc.append(point)

                if model[via_enc_ver_point_vars[LOWER][point]] == True:
                    ver_lower_via_enc.append(point)

                if model[via_enc_ver_point_vars[UPPER][point]] == True:
                    ver_upper_via_enc.append(point)

                for adj_point in list(metal_edge_vars[point].keys()):
                    if adj_point >= point:
                        if model[metal_edge_vars[point][adj_point]] == True:
                            metals.append((point, adj_point))

            if design_rules["ensure_access_points"]:
                ext_pin_layers = design_rules["ext_pin_layer"]
                upper_layers = list()
                for ext_pin_layer in ext_pin_layers:
                    upper_layers = upper_layers + design_rules["upper_layers"][ext_pin_layer]
                upper_layers = list(set(upper_layers))
                upper_zs = [z for z, layer in enumerate(design_rules["routing_layers"]) if layer in upper_layers]

                for net in self.nets:
                    if net.is_power or net.name == DUMMY_NET or not net.is_ext_pin:
                        continue

                    for point in net_point_vars[net].keys():
                        for adj_point in net_edge_vars[net][point].keys():
                            if adj_point < point or \
                                (point[Z] not in upper_zs and adj_point[Z] not in upper_zs):
                                continue

                            num_pins = net.get_num_pins()
                            remove_via_enc = False
                            for i in range(num_pins-2, -1, -1):
                                if net_com_edge_vars[net][i].get(point, {}).get(adj_point) != None and \
                                    model[net_com_edge_vars[net][i].get(point, {}).get(adj_point)] == True:

                                    if i == num_pins-2:
                                        masking_metals.add((point, adj_point))

                                        if point[X] == adj_point[X] and \
                                            point[Y] == adj_point[Y] and \
                                            point[Z] != adj_point[Z]:
                                            
                                            remove_via_enc = True

                                    else:
                                        remove_via_enc = False
                                        masking_metals.discard((point, adj_point))
                                

                            if remove_via_enc:
                                x = point[X]
                                y = point[Y]

                                min_z = min(point[Z], adj_point[Z])
                                max_z = max(point[Z], adj_point[Z])
                                
                                lower_point = (x, y, min_z)
                                upper_point = (x, y, max_z)

                                if upper_point in hor_lower_via_enc:
                                    hor_lower_via_enc.remove(upper_point)
                                if upper_point in ver_lower_via_enc:
                                    ver_lower_via_enc.remove(upper_point)
                                if lower_point in hor_upper_via_enc:
                                    hor_upper_via_enc.remove(lower_point)
                                if lower_point in ver_upper_via_enc:
                                    ver_upper_via_enc.remove(lower_point)


                for net in self.nets:
                    if net.is_power or net.name == DUMMY_NET:
                        continue

                    for point in net_point_vars[net].keys():
                        for adj_point in net_edge_vars[net][point].keys():
                            if adj_point < point or \
                                point[Z] not in upper_zs or \
                                adj_point[Z] not in upper_zs:
                                continue

                            if model[metal_edge_vars[point][adj_point]] == True:
                                metals_without_net.add((point, adj_point))

                            num_pins = net.get_num_pins()

                            for i in range(num_pins-1):
                                if net_com_edge_vars[net][i].get(point, {}).get(adj_point) != None and \
                                    model[net_com_edge_vars[net][i].get(point, {}).get(adj_point)] == True:
                                    metals_without_net.discard((point, adj_point))


            repeat = True
            while repeat:
                masking_vertices = set()
                for edge in masking_metals:
                    masking_vertices.update(edge)

                repeat = False
                to_remove = set()
                for edge in metals_without_net:
                    if edge[0] in masking_vertices or edge[1] in masking_vertices:
                        masking_metals.add(edge)
                        to_remove.add(edge)
                        repeat = True
                metals_without_net -= to_remove


            metals = [metal for metal in metals if metal not in masking_metals]
                        
                        
            for d in model:
                if d.name().startswith("EXT_PIN_") and model[d] == z3.BoolVal(True):
                    _, _, net, x, y, z = d.name().split('_')
                    external_pin_point[net] = (int(x), int(y), int(z))     
                    
            self.metals = metals
            
            self.hor_lower_via_enc = hor_lower_via_enc
            self.hor_upper_via_enc = hor_upper_via_enc
            self.ver_lower_via_enc = ver_lower_via_enc
            self.ver_upper_via_enc = ver_upper_via_enc
            
            self.external_pin_point = external_pin_point
            
        else:   
            self.metals = False
            
            self.hor_lower_via_enc = False
            self.hor_upper_via_enc = False
            self.ver_lower_via_enc = False
            self.ver_upper_via_enc = False
            
            self.external_pin_point = False

        after_time = time.time()
        logger.log(level=logging.INFO, msg=f"SMT Sovler Runtime [{after_time-prev_time:.2f} s]")
        
        return is_sat
   
    def set_nets(self):
        if os.path.isfile(f"{self.save_dir}/{self.cell_name}/net_info.txt"):
            os.system(f"rm {self.save_dir}/{self.cell_name}/net_info.txt")
        for net in self.nets:
            net.print_info(file_name=f"{self.save_dir}/{self.cell_name}/net_info.txt")
            self.placement_net_boundary[net] = net.get_bounding_box()
            if not net.is_power and net != DUMMY_NET:
                net.set_tolerance(design_rules["x_tolerance"], design_rules["x_tolerance"], design_rules["y_tolerance"], design_rules["y_tolerance"])
            net.set_ext_pin_points(self.x_points, self.y_points)
    
    def low_resolution_routing(self):
        valid_x_points = set()
        for net in self.nets:
            if not (net.is_power or net.name == DUMMY_NAME or (not net.is_ext_pin and len(net.get_pins()) < 2)):
                points = net.get_pin_points()
                for point in points:
                    valid_x_points.add(point[X])
        
        for layer_index in range(len(design_rules["routing_layers"])):
            self.x_points[layer_index] = sorted(list(set(self.x_points[layer_index]) & valid_x_points))  


    def route(self):        
        layers = dict()

        for i in range(len(design_rules["routing_layers"])):
            top_layer = design_rules["routing_layers"][-1]            
            layers[top_layer] = copy.deepcopy(design_rules["routing_layers"])
            design_rules["routing_layers"] = design_rules["routing_layers"][:-1]

        original_nets = copy.deepcopy(self.nets)

        is_sat = False
        num_try = 0
        for top_layer in ['M1', 'M2']:
            if top_layer == 'M1' and design_rules['ext_pin_layer'][-1] == "M1" and design_rules['ensure_access_points']:
                continue
            design_rules["routing_layers"] = layers[top_layer]

            for t in range(0, design_rules["max_tolerance"]+1, 2):
                design_rules["x_tolerance"] = t
                design_rules["y_tolerance"] = t
            
                self.x_points = self.placement.get_x_points()
                self.y_points = self.placement.get_y_points(allow_below_min_track=design_rules["allow_below_min_track"])

                num_try += 1

                logger.log(level=logging.INFO, msg=f"{num_try}th Iteration")
                logger.log(level=logging.INFO, msg=f"Top layer: {top_layer}, Tolerance: {t}, allow_below_min_track: {design_rules['allow_below_min_track']}")

                prev_time = time.time()
                
                solver = z3.Optimize()

                self.solver = solver
                
                self.nets = copy.deepcopy(original_nets)
                
                if design_rules["low_resolution_routing"]:
                    self.low_resolution_routing()
                    design_rules["low_resolution_routing"] = False
                
                for layer_num, layer in enumerate(design_rules["routing_layers"]):
                    logger.log(level=logging.INFO, msg=f"{num_try}th iter's X routing track of {layer}: {self.x_points[layer_num]}")
                    logger.log(level=logging.INFO, msg=f"{num_try}th iter's Y routing track of {layer}: {self.y_points[layer_num]}")
                    logger.log(level=logging.INFO, msg=f"{num_try}th iter's Y routing track for external pin of {layer}: {self.only_ver_metal_y_points[layer_num]}")

                self.set_nets()
                
                self.routing_graph = self.get_routing_graph()
                self.save_layer_images(f"{self.save_dir}/{self.cell_name}/layer_track_info")
                
                self.set_vars()
                self.set_geometric_variables() 

                step_constraints = list()

                step_constraints.append(self.side_to_side_spacing_rule())        
                step_constraints.append(self.tip_to_tip_spacing_rule())        
                step_constraints.append(self.side_to_tip_spacing_rule())      
                step_constraints.append(self.corner_to_corner_spacing_rule())
                step_constraints.append(self.via_spacing_rule())  

                step_constraints.append(self.commodity_flow_conservation())
                step_constraints.append(self.edge_assignment())  
                step_constraints.append(self.exclusiveness_vertex())        
                step_constraints.append(self.metal_segment())
                step_constraints.append(self.exclusiveness_layer())

                step_constraints.append(self.pre_layout_design_rule())
                step_constraints.append(self.reserve_pin_points())
        
                step_constraints.append(self.minimum_area_rule())      
                step_constraints.append(self.minimum_pin_length())
                step_constraints.append(self.add_via_enclosure_rule())
                step_constraints.append(self.add_no_cross_via())

                for i, step_constraint in enumerate(step_constraints):
                    if step_constraint is not None:
                        filtered = [cond for cond in step_constraint if cond is not True]
                        if not filtered:
                            continue

                        solver.add(*filtered)
                        
                self.set_optimize(solver)
                self.to_smt2_file(f"{self.save_dir}/{self.cell_name}/{self.cell_name}.smt2", solver)

                after_time = time.time()
                logger.log(level=logging.INFO, msg=f"The stage of generating all SMT constraints is complete. [{after_time-prev_time:.2f} s]")
                
                is_sat = self.check_sat(solver)
                                
                if is_sat:
                    break
                else:
                    logger.log(level=logging.INFO, msg=f"No satisfying solution was found. Expanding the search space, we will try to solve the problem again.")
                    print()
                    print()
    
            if is_sat:
                break       

        return (self.metals, self.hor_lower_via_enc, self.hor_upper_via_enc, self.ver_lower_via_enc, self.ver_upper_via_enc, self.external_pin_point)
    
