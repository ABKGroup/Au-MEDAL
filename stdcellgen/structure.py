import sys
import logging
from .vars import *

from . import design_rules

logged_messages = set()

class UniqueFilter(logging.Filter):
    def filter(self, record):
        message = record.getMessage()
        if message in logged_messages:
            return False 
        logged_messages.add(message)
        return True 

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

handler = logging.StreamHandler()
handler.addFilter(UniqueFilter())
formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
handler.setFormatter(formatter)
logger.addHandler(handler)


class MOSFET():
    def __init__(self, name, drain, gate, source, body, type, options):
        self.name = name
        self.drain = drain
        self.gate = gate
        self.source = source
        self.body = body
        self.type = type
        self.options = options
        self.is_flipped = -1
        self.max_num_fins = design_rules["num_max_nmos_fins"] if "NMOS" in type.upper() else design_rules["num_max_pmos_fins"]

    def __str__(self):
        return self.name
    
    def __repr__(self):
        return self.__str__()
    
    def __eq__(self, other):
        if isinstance(other, MOSFET):
            return self.name == other.name
        elif isinstance(other, str):
            return self.name == other
        return False
    
    def __hash__(self):
        return hash(self.name)

    def flip(self):
        self.drain, self.source = self.source, self.drain
        self.is_flipped = -1 * self.is_flipped

    ### Calculate nfet/pfet width
    def get_num_finger(self):
        num_fins = int(self.options["NFIN"])
        return (num_fins-1) // self.max_num_fins + 1
    
    def get_max_num_fins(self):
        return self.max_num_fins 

    def get_type(self):
        return self.type

    def get_drain(self):
        return self.drain
    
    def get_gate(self):
        return self.gate

    def get_source(self):
        return self.source

    def get_body(self):
        return self.body

    def get_name(self):
        return self.name

    def get_flip_state(self):
        return self.is_flipped
    
    def get_options(self):
        return self.options

    def get_width(self):
        num_finger = self.get_num_finger()
        return 2*num_finger+1
    
    def get_num_fins(self):
        return self.options["NFIN"]


#### Pin is consisted of several points. ####
#### ex: [(2, 3, 1), (3, 3, 1)] ####

class Pin():
    def __init__(self, net, term_name: str):
        self.net = net 
        self.term_name = term_name
        self.points = list()

    def __str__(self):
        return str(self.points)
    
    def __repr__(self):
        return self.__str__()
    
    def set_net(self, net):
        self.net = net 

    def set_term_name(self, term_name):
        self.term_name = term_name

    def set_points(self, points):
        self.points = points

    def add_point(self, x, y, layer):
        self.points.append((x, y, layer))
        self.points = list(set(self.points))

    def get_points(self):
        return self.points

    def get_mid_point(self):
        points = self.points

        xs = list()
        ys = list()
        layers = list()

        for point in points:
            xs.append(point[0])
            ys.append(point[1])
            layers.append(point[2])

        x = int(sum(xs)/len(xs))
        y = int(sum(ys)/len(ys))
        layer = int(sum(layers)/len(layers))

        return (x, y, layer)

    def get_net(self):
        return self.net
    
    def get_term_name(self):
        return self.term_name


class Net():
    def __init__(self, name, is_ext_pin=False):
        self.name = name
        self.pins = list()
        self.partitioned_pins = None

        self.bounding_box = (sys.maxsize, sys.maxsize, sys.maxsize, 0, 0, 0) # (lx, ly, lz, ux, uy, uz)

        self.x_right_tolerance = 0
        self.x_left_tolerance = 0
        self.y_upper_tolerance = 0
        self.y_lower_tolerance = 0

        self.is_power = (POWER_NET in name.upper() or GND_NET in name.upper())
        self.is_ext_pin = is_ext_pin

    def __eq__(self, other):
        if isinstance(other, Net):
            return self.name == other.name
        elif isinstance(other, str):
            return self.name == other
        return False
    
    def __hash__(self):
        return hash(self.name)

    def __str__(self):
        return self.name
    
    def __repr__(self):
        return self.__str__()

    def set_tolerance(self, x_right_tolerance, x_left_tolerance, y_upper_tolerance, y_lower_tolerance):
        self.x_right_tolerance = x_right_tolerance
        self.x_left_tolerance = x_left_tolerance
        self.y_upper_tolerance = y_upper_tolerance
        self.y_lower_tolerance = y_lower_tolerance
    
    def get_tolerance(self):
        return (self.x_right_tolerance, self.x_left_tolerance, self.y_upper_tolerance, self.y_lower_tolerance)

    def set_ext_pin_points(self, x_points, y_points):
        if not self.is_ext_pin or self.is_power:
            return
        
        cell_height = design_rules["cell_height"]
        
        min_x, min_y, min_z = self.bounding_box[0], self.bounding_box[1], self.bounding_box[2]
        max_x, max_y, max_z = self.bounding_box[3], self.bounding_box[4], self.bounding_box[5]
                
        ext_pin = Pin(self, EXT_PIN)
                
        min_x = int(max(0, min_x - self.x_left_tolerance * int(design_rules["x_unit"])))
        max_x = int(max_x + self.x_right_tolerance * int(design_rules["x_unit"]))
        
        max_row = int(max_y-1) // cell_height
        min_row = int(min_y) // cell_height

        ys = list()
        
        for row in range(min_row, max_row+1):
            is_flip = (row % 2 == 1)

            row_max_y = (row+1)*cell_height
            row_min_y = row*cell_height

            contact_y = int(row_min_y + (row_max_y-row_min_y)/2 + (design_rules["np_offset"] if not is_flip else -design_rules["np_offset"]))
            ys.append(contact_y)

        for ext_pin_layer in design_rules["ext_pin_layer"]:
            ext_pin_layer_index = design_rules["routing_layers"].index(ext_pin_layer)
            x_unit = int(design_rules["x_unit"] / design_rules["x_routing_resolution"][ext_pin_layer_index])
            
            bound_x_points = [x_point for x_point in x_points[ext_pin_layer_index] if min_x <= x_point <= max_x]

            for x in bound_x_points:
                for y in ys:
                    ext_pin.add_point(x, y, ext_pin_layer_index)
                
        self.add_pin(ext_pin)

        min_y = int(max(0, min_y - self.y_lower_tolerance * int(design_rules["y_unit"])))
        max_y = int(max_y + self.y_upper_tolerance * int(design_rules["y_unit"]))
        if design_rules["ensure_access_points"]:
            acc_pin = Pin(self, ACCESS_POINT)
            for ext_pin_layer in design_rules["ext_pin_layer"]:
                upper_pin_layers = design_rules["upper_layers"][ext_pin_layer]

                for upper_pin_layer in upper_pin_layers:
                    upper_pin_layer_index = design_rules["routing_layers"].index(upper_pin_layer)


                    bound_y_points = [y_point for y_point in y_points[upper_pin_layer_index] if min_y <= y_point <= max_y]
                    
                    x_unit = int(design_rules["x_unit"] / design_rules["x_routing_resolution"][upper_pin_layer_index])
                    y_unit = int(design_rules["y_unit"] / design_rules["y_routing_resolution"][upper_pin_layer_index])
                    
                    for x in range(min_x, max_x+x_unit, x_unit):
                        for y in bound_y_points:
                            acc_pin.add_point(x, y, upper_pin_layer_index)
                            
            self.add_pin(acc_pin)


    def print_info(self, file_name=None):
        str = f"{self.name}: "

        pins = self.pins
        for pin in pins:
            str += f"{pin} "
        str += "\n"

        if file_name == None:
            print(str)
        else:
            f = open(file_name, "a")
            f.write(str)
            f.close()

    def get_pin_points(self):
        pin_points = list()
        pins = self.pins
        for pin in pins:
            points = pin.get_points()
            pin_points.extend(points)
            
        return pin_points

    def get_HPWL(self):
        min_x, min_y, _ = self.bounding_box[0], self.bounding_box[1], self.bounding_box[2]
        max_x, max_y, _ = self.bounding_box[3], self.bounding_box[4], self.bounding_box[5]

        dx = max_x-min_x
        dy = max_y-min_y
    
        HPWL = (dx + dy) / 2

        return HPWL
            
    def get_pins(self):
        return self.pins

    def get_num_pins(self):
        return len(self.pins)

    def get_name(self):
        return self.name

    def set_bounding_box(self):
        pins = self.pins

        min_x, min_y, min_z = self.bounding_box[0], self.bounding_box[1], self.bounding_box[2]
        max_x, max_y, max_z = self.bounding_box[3], self.bounding_box[4], self.bounding_box[5]

        for pin in pins:
            points = pin.get_points()
            for point in points:
                x, y, z = point

                if x < min_x:    min_x = x
                if y < min_y:    min_y = y
                if z < min_z:    min_z = z

                if max_x < x:    max_x = x
                if max_y < y:    max_y = y
                if max_z < z:    max_z = z

        self.bounding_box = (min_x, min_y, min_z, max_x, max_y, max_z)

    def get_bounding_box(self):
        return self.bounding_box

    def add_pin(self, pin):
        self.pins.append(pin)
        self.set_bounding_box()

    def remove_pin(self, pin):
        self.pins.remove(pin)

        