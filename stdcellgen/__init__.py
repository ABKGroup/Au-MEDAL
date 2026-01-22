import os
import json
from .vars import *

class DotDict(dict):
    def __getitem__(self, item):
        if item not in self:
            self[item] = DotDict()
        return super().__getitem__(item)

    def get(self, key, default=None):
        return super().get(key, default)
    
    def __bool__(self):
        return bool(self.keys()) 
    
design_rules = DotDict()

def set_design_rules(json_file):
    print("Info: Reading the design rules file.")
    global design_rules, Circuit, PlaceEnv, Router, Placer, Layout
    if not os.path.exists(json_file):
        raise FileNotFoundError(f"Cannot find JSON file: {json_file}")
    
    with open(json_file, 'r', encoding='utf-8') as f:
        data = json.load(f)
    
    design_rules = DotDict(data)
    
    ### Modify the routing direction variable name for readability.
    direction_mapping = {"B": BIDIRECTION, "H": HORIZONTAL, "V": VERTICAL}
    design_rules["routing_directions"] = [direction_mapping.get(item) for item in design_rules["routing_directions"]]

    ### Calculate the layer pitch
    layers = set(design_rules["width"].keys()) & set(design_rules["spacing"]["S2S"].keys())
    for layer in layers:
        try:
            design_rules["pitch"][layer] = design_rules["width"][layer] + design_rules["spacing"]["S2S"][layer][layer]
        except KeyError:
            print(f"Warning: Ignore {layer} pitch calculation: Pitch is calculated only when both the layer's width and its side-to-side minimum spacing are declared.")

    ### Add same height layers
    layers_related_to_routing = set(design_rules["routing_layers"]) | set(design_rules["vias"])
    
    for routing_layer in layers_related_to_routing:
        design_rules["same_height_layers"].setdefault(routing_layer, []).append(routing_layer)
        
    design_rules["np_offset"] = (design_rules["num_max_nmos_fins"] - design_rules["num_max_pmos_fins"])/2 * design_rules["pitch"]["fin"]
    design_rules["num_track"] = design_rules["cell_height"] / design_rules["pitch"]["M1"]

    design_rules["x_offset"] = (design_rules["pitch"]["Gate"]/2) #* design_rules["diffusion_break"]
    design_rules["y_offset"] = (design_rules["power_width"]["M1"] - design_rules["width"]["M1"])/2

    design_rules["x_unit"] = design_rules["pitch"]["Gate"]/2
    design_rules["y_unit"] = design_rules["pitch"]["M1"]
    
    from .circuit import Circuit
    from .placeEnv import PlaceEnv
    from .router import Router
    from .placer import Placer
    from .layout import Layout
    

def check_design_rules():
    metal_width = design_rules["width"]["M1"]
    max_active_fins = design_rules["num_max_pmos_fins"] + design_rules["num_max_nmos_fins"]
    min_vert_space = design_rules["spacing"]["S2S"]["Active"]["Active"]
    cell_height = design_rules["cell_height"]
    
    assert cell_height - metal_width - (max_active_fins) * design_rules["pitch"]["fin"] - min_vert_space >= 0, "You have entered an invalid maximum number of fins for pfet and nfet."
    assert cell_height % design_rules["pitch"]["fin"] == 0, 'The cell height must be a multiple of the fin pitch. {design_rules["pitch"]["fin"]}'
    assert design_rules["extension"]["LIG"] > design_rules["width"]["Gate"], "[Error] A violation of the 'LIG.GATE.AUX.1' rule will occur. Therefore, please ensure that the widths of the LIG and Gate layers are set to different values. (This will be addressed in a future update, if possible.)"
    assert design_rules["width"]["M1"] == design_rules["width"]["V0"], "[Error] A violation of the 'V0.M1.AUX.3' rule will occur. Therefore, please ensure that the widths of the V0 and M1 layers are set to same values."
    assert design_rules["width"]["M2"] == design_rules["width"]["V1"], "[Error] A violation of the 'V1.M2.AUX.2' rule will occur. Therefore, please ensure that the widths of the V0 and M1 layers are set to same values."
    assert design_rules["spacing"]["S2S"]["Gate"]["Active"] + design_rules["offset"]["Gate"]["Active"] <= design_rules["spacing"]["S2S"]["Gate"]["Gate"], (
        "The sum of the spacing and offset between 'Gate' and 'Active' exceeds the allowed gate-to-gate spacing."
    )
    for layer_num, layer in enumerate(design_rules["routing_layers"]):
        if design_rules["routing_directions"][layer_num] == BIDIRECTION:
            assert bool(design_rules["width"][layer]) and bool(design_rules["offset"][layer]), ""
            assert design_rules["width"][layer] == design_rules["extension"][layer], "Width and extension must match for bidirectional routing."