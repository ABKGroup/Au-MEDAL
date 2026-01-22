from .vars import *

from .structure import *
from collections import defaultdict
import gdspy
import shapely.geometry as sg
from shapely.geometry import LineString, MultiPoint, Polygon
from shapely.ops import unary_union


class Layout():
    def __init__(self, cell_name):
        self.cell_name = cell_name
        self.lib = gdspy.GdsLibrary(unit=1e-6, precision=1e-6 / 4000)
        self.cell = self.lib.new_cell(self.cell_name)
        self.rectangles = dict()
        
        self.external_pin_point = None
        self.placement = None
    
    def set_placement(self, placement):
        self.placement = placement
        self.write_placement()

    def set_routing(self, results):
        (metals, hor_lower_via_enc, hor_upper_via_enc, ver_lower_via_enc, ver_upper_via_enc, external_pin_point) = results
        self.external_pin_point = external_pin_point
        
        self.write_metal(metals)
        self.write_via_enc_metal(hor_lower_via_enc, hor_upper_via_enc, ver_lower_via_enc, ver_upper_via_enc)
        #self.write_sdt()

    def write_sdt(self):
        rectangles = self.rectangles
        
        active_map_num = design_rules["layer_map"]["Active"]
        lisd_map_num = design_rules["layer_map"]["LISD"]
        sdt_map_num = design_rules["layer_map"]["SDT"]
        
        active_polygons = rectangles[active_map_num]
        lisd_polygons = rectangles[lisd_map_num]
        
        for poly_active in active_polygons:
            for poly_lisd in lisd_polygons:
                inter = poly_active.intersection(poly_lisd)
                if inter is not None:
                    if sdt_map_num not in self.rectangles.keys():
                        self.rectangles[sdt_map_num] = list()
                    self.rectangles[sdt_map_num].append(inter)


    def get_metal_direction(self, lx, ly, ux, uy, threshold):
        width = ux - lx   
        height = uy - ly  

        if width <= threshold and height <= threshold:
            return POINT

        elif width > threshold and height <= threshold:
            return HORIZONTAL
        
        elif height > threshold and width <= threshold:
            return VERTICAL

        elif width > threshold and height > threshold:
            return BIDIRECTION


    def check_layer_overlap(self, target_polygon, layer):
        rectangles = self.rectangles

        layer_map_num = design_rules["layer_map"][layer]
        if layer_map_num not in rectangles.keys():
            rectangles[layer_map_num] = list()
        layer_polygons = rectangles[layer_map_num]
        
        for polygon in layer_polygons:
            if self.check_overlap(target_polygon, polygon, True, X):
                if self.check_overlap(target_polygon, polygon, True, Y):
                    return polygon
        
        
        return False

    def check_overlap(self, polygon1, polygon2, border, direction):
        poly1_minx, poly1_miny, poly1_maxx, poly1_maxy = polygon1.bounds
        poly2_minx, poly2_miny, poly2_maxx, poly2_maxy = polygon2.bounds

        if direction == X:
            overlap = (poly1_minx <= poly2_maxx if border else poly1_minx < poly2_maxx) and \
                    (poly1_maxx >= poly2_minx if border else poly1_maxx > poly2_minx)
        elif direction == Y:
            overlap = (poly1_miny <= poly2_maxy if border else poly1_miny < poly2_maxy) and \
                    (poly1_maxy >= poly2_miny if border else poly1_maxy > poly2_miny)

        return overlap


    def write_via_enc_metal(self, hor_lower_via_enc, hor_upper_via_enc, ver_lower_via_enc, ver_upper_via_enc):
        def get_enclosure_metals(via_enc, direction, via_type):
            enclosure_metals = list()
            for via_enc_entry in via_enc:
                x, y, z = via_enc_entry
                layer = design_rules["routing_layers"][z]
                via = design_rules[via_type][layer]
                if via == None:
                    continue

                if y % design_rules["cell_height"] == 0 and layer in design_rules["power_layer"]:
                    metal_width = design_rules["power_width"][layer]
                else:
                    metal_width = design_rules["width"][layer]

                metal_ex = design_rules["extension"][layer]
                via_width = design_rules["width"][via]

                if direction == HORIZONTAL:
                    x_extension, y_extension = design_rules["enclosure"][via][layer], 0
                    x_metal_width, y_metal_width = metal_ex, metal_width

                elif direction == VERTICAL:
                    x_extension, y_extension = 0, design_rules["enclosure"][via][layer]
                    x_metal_width, y_metal_width = metal_width, metal_ex


                lx = (x - (via_width / 2 + x_extension)) if x_extension != 0 else (x - (x_metal_width / 2))
                ux = (x + (via_width / 2 + x_extension)) if x_extension != 0 else (x + (x_metal_width / 2))

                ly = (y - (via_width / 2 + y_extension)) if y_extension != 0 else (y - (y_metal_width / 2))
                uy = (y + (via_width / 2 + y_extension)) if y_extension != 0 else (y + (y_metal_width / 2))

                enclosure_metal = (lx, ly, ux, uy)

                self.add_rectangle_to_shapely(enclosure_metal, layer=design_rules["layer_map"][layer], cut=True)
            
            return enclosure_metals

        get_enclosure_metals(hor_lower_via_enc, HORIZONTAL, "lower_via")
        get_enclosure_metals(hor_upper_via_enc, HORIZONTAL, "upper_via")
        get_enclosure_metals(ver_lower_via_enc, VERTICAL, "lower_via")
        get_enclosure_metals(ver_upper_via_enc, VERTICAL, "upper_via")
                           

    def check_inside(self, polygon1, polygon2, border, direction):
        poly1_minx, poly1_miny, poly1_maxx, poly1_maxy = polygon1.bounds
        poly2_minx, poly2_miny, poly2_maxx, poly2_maxy = polygon2.bounds

        if direction == X:
            if border:
                inside_x1 = (poly2_minx <= poly1_minx) and (poly1_maxx <= poly2_maxx)
                inside_x2 = (poly1_minx <= poly2_minx) and (poly2_maxx <= poly1_maxx)
            else:
                inside_x1 = (poly2_minx < poly1_minx) and (poly1_maxx < poly2_maxx)
                inside_x2 = (poly1_minx < poly2_minx) and (poly2_maxx < poly1_maxx)
            
            return inside_x1 or inside_x2

        elif direction == Y:
            if border:
                inside_y1 = (poly2_miny <= poly1_miny) and (poly1_maxy <= poly2_maxy)
                inside_y2 = (poly1_miny <= poly2_miny) and (poly2_maxy <= poly1_maxy)
            else:
                inside_y1 = (poly2_miny < poly1_miny) and (poly1_maxy < poly2_maxy)
                inside_y2 = (poly1_miny < poly2_miny) and (poly2_maxy < poly1_maxy)
            
            return inside_y1 or inside_y2


    def is_overlap(self, x, y, x_ex, y_ex, layer):
        rectangles = self.rectangles

        lx = x - x_ex / 2
        ux = x + x_ex / 2
        ly = y - y_ex / 2
        uy = y + y_ex / 2

        input_square = sg.box(lx, ly, ux, uy)

        layer_map_num = design_rules["layer_map"][layer]
        if layer_map_num not in rectangles.keys():
            rectangles[layer_map_num] = list()
        layer_polygons = rectangles[layer_map_num]

        for polygon in layer_polygons:

            x_overlapped = self.check_overlap(input_square, polygon, False, X)
            y_overlapped = self.check_overlap(input_square, polygon, False, Y)
            
            if x_overlapped and y_overlapped:
                return True

        return False


    def query_polygon(self, x_points, y_points, x_index, y_index, x_ex, y_ex, layer, z, input_metal_type, target_dist):
        rectangles = self.rectangles

        x = x_points[x_index]
        y = y_points[y_index]

        right_x = x_points[x_index + 1] if x_index < len(x_points) - 1 else -1
        left_x = x_points[x_index - 1] if x_index > 0 else -1

        up_y = y_points[y_index + 1] if y_index < len(y_points) - 1 else -1
        down_y = y_points[y_index - 1] if y_index > 0 else -1


        half_x_ex = x_ex / 2
        half_y_ex = y_ex / 2
        lx, ly = x - half_x_ex, y - half_y_ex
        ux, uy = x + half_x_ex, y + half_y_ex
        input_square = sg.box(lx, ly, ux, uy)
        input_minx, input_miny, input_maxx, input_maxy = input_square.bounds

        try:
            min_side_len = design_rules["min_side_len"][layer]
        except KeyError:
            min_side_len = 36

        layer_map_num = design_rules["layer_map"][layer]
        if layer_map_num not in rectangles.keys():
            rectangles[layer_map_num] = list()
        layer_polygons = rectangles[layer_map_num]

        if input_metal_type == CORNER:
            target_polygons = {TOP_RIGHT: [], BOTTOM_RIGHT: [], TOP_LEFT: [], BOTTOM_LEFT: []}
        else:
            target_polygons = {RIGHT: [], LEFT: [], TOP: [], BOTTOM: []}

        edge = None

        for polygon in layer_polygons:
            poly_minx, poly_miny, poly_maxx, poly_maxy = polygon.bounds
            poly_dir = self.get_metal_direction(poly_minx, poly_miny, poly_maxx, poly_maxy, min_side_len)

            if polygon.distance(input_square) >= target_dist:
                continue

            overlap_x = self.check_overlap(input_square, polygon, False, X)
            overlap_y = self.check_overlap(input_square, polygon, False, Y)

            if input_metal_type == CORNER:
                if not overlap_x and not overlap_y:
                    if poly_minx > input_maxx and poly_miny > input_maxy:
                        target_polygons[TOP_RIGHT].append(polygon)
                    elif poly_minx > input_maxx and poly_maxy < input_miny:
                        target_polygons[BOTTOM_RIGHT].append(polygon)
                    elif poly_maxx < input_minx and poly_miny > input_maxy:
                        target_polygons[TOP_LEFT].append(polygon)
                    elif poly_maxx < input_minx and poly_maxy < input_miny:
                        target_polygons[BOTTOM_LEFT].append(polygon)
            else:
                if poly_minx > input_maxx and overlap_y:
                    if input_metal_type == SIDE and poly_dir in [VERTICAL, BIDIRECTION]:
                        target_polygons[RIGHT].append(polygon)
                    elif input_metal_type == TIP and poly_dir in [HORIZONTAL, POINT]:
                        target_polygons[RIGHT].append(polygon)


                    if right_x != None:
                        if input_miny < poly_maxy < input_maxy:
                            for i in range(y_index-1, -1, -1):
                                prev_y = y_points[i]
                                prev_y_maxy = prev_y + half_y_ex
                                if prev_y_maxy < input_miny:
                                    break
                                
                                if prev_y_maxy == poly_maxy:
                                    edge = ((x, prev_y, z), (right_x, prev_y, z))
                        
                        elif input_miny < poly_miny < input_maxy:
                            for i in range(y_index+1, len(y_points)):
                                next_y = y_points[i]
                                next_y_miny = next_y - half_y_ex
                                if input_maxy < next_y_miny:
                                    break
                                
                                if next_y_miny == poly_miny:
                                    edge = ((x, next_y, z), (right_x, next_y, z))

                elif poly_maxx < input_minx and overlap_y:
                    if input_metal_type == SIDE and poly_dir in [VERTICAL, BIDIRECTION]:
                        target_polygons[LEFT].append(polygon)

                    elif input_metal_type == TIP and poly_dir in [HORIZONTAL, POINT]:
                        target_polygons[LEFT].append(polygon)


                    if left_x != None:
                        if input_miny < poly_maxy < input_maxy:
                            for i in range(y_index-1, -1, -1):
                                prev_y = y_points[i]
                                prev_y_maxy = prev_y + half_y_ex
                                if prev_y_maxy < input_miny:
                                    break
                                
                                if prev_y_maxy == poly_maxy:
                                    edge = ((left_x, prev_y, z), (x, prev_y, z))
                        
                        elif input_miny < poly_miny < input_maxy:
                            for i in range(y_index+1, len(y_points)):
                                next_y = y_points[i]
                                next_y_miny = next_y - half_y_ex
                                if input_maxy < next_y_miny:
                                    break

                                if next_y_miny == poly_miny:
                                    edge = ((left_x, next_y, z), (x, next_y, z))

                elif poly_miny > input_maxy and overlap_x:
                    if input_metal_type == SIDE and poly_dir in [HORIZONTAL, BIDIRECTION]:
                        target_polygons[TOP].append(polygon)
                    elif input_metal_type == TIP and poly_dir in [VERTICAL, POINT]:
                        target_polygons[TOP].append(polygon)

                    if up_y is not None:
                        if input_minx < poly_maxx < input_maxx:
                            for i in range(x_index - 1, -1, -1):
                                prev_x = x_points[i]
                                prev_x_max = prev_x + half_x_ex
                                if prev_x_max < input_minx:
                                    break
                                if prev_x_max == poly_maxx:
                                    edge = ((prev_x, y, z), (prev_x, up_y, z))

                        elif input_minx < poly_minx < input_maxx:
                            for i in range(x_index + 1, len(x_points)):
                                next_x = x_points[i]
                                next_x_min = next_x - half_x_ex
                                if input_maxx < next_x_min:
                                    break

                                if next_x_min == poly_minx:
                                    edge = ((next_x, y, z), (next_x, up_y, z))

                elif poly_maxy < input_miny and overlap_x:
                    if input_metal_type == SIDE and poly_dir in [HORIZONTAL, BIDIRECTION]:
                        target_polygons[BOTTOM].append(polygon)
                    elif input_metal_type == TIP and poly_dir in [VERTICAL, POINT]:
                        target_polygons[BOTTOM].append(polygon)

                    if down_y is not None:
                        if input_minx < poly_maxx < input_maxx:
                            for i in range(x_index - 1, -1, -1):
                                prev_x = x_points[i]
                                prev_x_max = prev_x + half_x_ex
                                if prev_x_max < input_minx:
                                    break

                                if prev_x_max == poly_maxx:
                                    edge = ((prev_x, down_y, z), (prev_x, y, z))


                        elif input_minx < poly_minx < input_maxx:
                            for i in range(x_index + 1, len(x_points)):
                                next_x = x_points[i]
                                next_x_min = next_x - half_x_ex
                                if input_maxx < next_x_min:
                                    break

                                if next_x_min == poly_minx:
                                    edge = ((next_x, down_y, z), (next_x, y, z))


        return target_polygons, edge


    def merge_rectangles(self, rectangles, except_layer=None):
        merged_rectangles = {}

        for layer in design_rules["layer_map"].values():
            if layer not in rectangles:
                continue

            if layer == except_layer:
                merged_rectangles[layer] = rectangles[layer]
                continue

            layer_polygons = rectangles[layer]
            if not layer_polygons:
                continue

            merged_polygon = unary_union(layer_polygons)

            if merged_polygon.geom_type == 'Polygon':
                merged_rectangles.setdefault(layer, []).append(merged_polygon)
            elif merged_polygon.geom_type == 'MultiPolygon':
                for poly in merged_polygon.geoms:
                    merged_rectangles.setdefault(layer, []).append(poly)
            else:
                raise ValueError("Unexpected geometry type: {}".format(merged_polygon.geom_type))
        return merged_rectangles


    def add_rectangle_to_shapely(self, square, layer, cut=False):
        rectangles = self.rectangles
        
        max_y = design_rules["num_row"] * design_rules["cell_height"] + design_rules["power_width"][design_rules["power_layer"][-1]]/2
        min_y = -design_rules["power_width"][design_rules["power_layer"][-1]]/2
        
        lx, ly, ux, uy = square
                
        if cut:
            ly = max(ly, min_y)
            uy = min(uy, max_y)
        
        polygon = sg.Polygon([(lx, ly), (ux, ly), (ux, uy), (lx, uy)])
        if layer not in rectangles.keys():
            rectangles[layer] = list()
        rectangles[layer].append(polygon)


    def add_polygons(self):
        def edge_type(edge):
            p1, p2 = edge
            if p1[X] == p2[X]:
                return VERTICAL
            elif p1[Y] == p2[Y]:
                return HORIZONTAL


        def find_all_edges(polygon):
            exterior_coords = list(polygon.exterior.coords)

            candidates = []
            for i in range(len(exterior_coords) - 1):
                p1, p2 = exterior_coords[i], exterior_coords[i+1]
                dir = edge_type((p1, p2))
                if dir == HORIZONTAL: 
                    y_val = p1[Y]
                    candidates.append(((p1, p2), dir, y_val, i))
                elif dir == VERTICAL:  
                    x_val = p1[X]
                    candidates.append(((p1, p2), dir, x_val, i))

            edges = None
            packed_edges = list()

            for a in range(len(candidates)):
                edge_a, type_a, val_a, idx_a = candidates[a]
                seg_a = LineString(edge_a)

                for b in range(a+1, len(candidates)):
                    edge_b, type_b, val_b, idx_b = candidates[b]

                    if type_a != type_b:
                        continue

                    if (type_a == HORIZONTAL or type_a == VERTICAL) and (val_a == val_b):
                        continue

                    seg_b = LineString(edge_b)                    
                    dist = seg_a.distance(seg_b)

                    packed_edges.append((dist, (edge_a, edge_b)))

            return packed_edges


        def patch_gap(distance, edges, min_spacing):
            if (distance is None or distance >= min_spacing) or edges is None:
                return None
            edge1, edge2 = edges

            type1 = edge_type(edge1)
            type2 = edge_type(edge2)

            if type1 == VERTICAL and type2 == VERTICAL:
                x1 = edge1[0][X]
                x2 = edge2[0][X]

                y1_min, y1_max = min(edge1[0][Y], edge1[1][Y]), max(edge1[0][Y], edge1[1][Y])
                y2_min, y2_max = min(edge2[0][Y], edge2[1][Y]), max(edge2[0][Y], edge2[1][Y])

                y_low = max(y1_min, y2_min)
                y_high = min(y1_max, y2_max)

                if y_low >= y_high:
                    points = list(edge1) + list(edge2)
                    return None
                    #return MultiPoint(points).convex_hull


                x_low = min(x1, x2)
                x_high = max(x1, x2)
                patch_coords = [(x_low, y_low), (x_high, y_low), (x_high, y_high), (x_low, y_high), (x_low, y_low)]
                return Polygon(patch_coords)

            elif type1 == HORIZONTAL and type2 == HORIZONTAL:
                y1 = edge1[0][Y]
                y2 = edge2[0][Y]

                x1_min, x1_max = min(edge1[0][0], edge1[1][0]), max(edge1[0][0], edge1[1][0])
                x2_min, x2_max = min(edge2[0][0], edge2[1][0]), max(edge2[0][0], edge2[1][0])
                x_low = max(x1_min, x2_min)
                x_high = min(x1_max, x2_max)
                if x_low >= x_high:
                    points = list(edge1) + list(edge2)
                    return MultiPoint(points).convex_hull

                y_low = min(y1, y2)
                y_high = max(y1, y2)
                patch_coords = [(x_low, y_low), (x_high, y_low), (x_high, y_high), (x_low, y_high), (x_low, y_low)]
                return Polygon(patch_coords)
            
            else:
                points = list(edge1) + list(edge2)
                return MultiPoint(points).convex_hull


        merged_rectangles = self.merge_rectangles(self.rectangles)
        
        for layer_name, layer in design_rules["layer_map"].items():
            if layer not in merged_rectangles.keys():
                continue
            for i, polygon in enumerate(merged_rectangles[layer]):
                try:
                    min_spacing = design_rules["spacing"]["S2S"][layer_name][layer_name]
                except KeyError:
                    min_spacing = 0
                packed_edges = find_all_edges(polygon)

                for (distance, edges) in packed_edges:
                    patch = patch_gap(distance, edges, min_spacing)
                    #if patch is not None:
                    #    merged_rectangles[layer][i] = merged_rectangles[layer][i].union(patch)

        return merged_rectangles


    def write_gds(self, save_dir):
        merged_rectangles = self.add_polygons()

        for layer in design_rules["layer_map"].values():
            if layer not in merged_rectangles.keys():
                continue
            for polygon in merged_rectangles[layer]:
                scaled_coords = [(x / 1000, y / 1000) for x, y in polygon.exterior.coords]
                gds_polygon = gdspy.Polygon(scaled_coords, layer=layer, datatype=0)
                self.cell.add(gds_polygon)
            
        for net, point in self.external_pin_point.items():
            z = point[Z]
            layer_name = design_rules["routing_layers"][z]
            layer_num = design_rules["layer_map"][layer_name]
            
            gds_point = gdspy.Label(net, (point[X] / 1000, point[Y] / 1000), layer=layer_num, texttype=251, magnification=1/1000)
            self.cell.add(gds_point)
        
        self.lib.write_gds(f'{save_dir}/{self.cell_name}.gds')


    def write_placement(self):
        self.add_poly()
        self.power_routing()
        self.add_rail()
        self.add_active()
        self.add_boundary()
        self.add_well_and_pn_select()
        self.add_fin()
        self.add_gcut()
        
        self.add_sdt()
        self.add_lisd()

        self.rectangles = self.merge_rectangles(self.rectangles, except_layer=design_rules["layer_map"]["Active"])
    
    def write_metal(self, result_metals):
        def get_continuous_metals(metals):
            def build_graph(edges):
                graph = defaultdict(list)
                for start, end in edges:
                    graph[start].append(end)
                    graph[end].append(start)
                return graph

            def find_continuous_paths(graph):
                def dfs(node, visited):
                    stack, path = [node], []
                    while stack:
                        current = stack.pop()
                        if current not in visited:
                            visited.add(current)
                            path.append(current)
                            stack.extend(neighbor for neighbor in graph[current] if neighbor not in visited)
                    return path

                visited, continuous_paths = set(), []
                for node in graph:
                    if node not in visited:
                        path = dfs(node, visited)
                        if path:
                            continuous_paths.append(sorted(path))
                return continuous_paths

            def combine_continuous_metals(continuous_paths):
                return [(path[0], path[-1]) for path in continuous_paths]

            horizontal_metals = [(start, end) for start, end in metals if start[1] == end[1]]
            vertical_metals = [(start, end) for start, end in metals if start[0] == end[0]]

            horizontal_graph = build_graph(horizontal_metals)
            vertical_graph = build_graph(vertical_metals)

            horizontal_paths = find_continuous_paths(horizontal_graph)
            vertical_paths = find_continuous_paths(vertical_graph)

            combined_horizontal_metals = combine_continuous_metals(horizontal_paths)
            combined_vertical_metals = combine_continuous_metals(vertical_paths)

            return combined_horizontal_metals, combined_vertical_metals
        
        layers = design_rules["routing_layers"]
        via_layers = design_rules["vias"]
        
        metals = {layer: [] for layer in layers}
        x_metals = {layer: [] for layer in layers}
        y_metals = {layer: [] for layer in layers}
        
        vias = {layer: [] for layer in via_layers}
        lower_layer = dict()
        upper_layer = dict()
        
        for metal in result_metals:
            metal_xy = ((metal[0][X], metal[0][Y]), (metal[1][X], metal[1][Y]))
            
            for layer_num, layer in enumerate(layers):
                if metal[0][Z] == layer_num and metal[1][Z] == layer_num:
                    metals[layer].append(metal_xy)

            if metal[0][X] == metal[1][X] and metal[0][Y] == metal[1][Y] and metal[0][Z] != metal[1][Z]:
                lower_layer_index = metal[0][Z]
                upper_layer_index = metal[1][Z]
                
                lower_layer[metal_xy] = layers[lower_layer_index]
                upper_layer[metal_xy] = layers[upper_layer_index]

                via1 = design_rules["upper_via"][layers[lower_layer_index]]
                via2 = design_rules["lower_via"][layers[upper_layer_index]]

                if len(via1) > 0:
                    add_via = True if via1 == via2 else False 
                    vias[via1].append((metal_xy, add_via))
                    
        for layer in layers:
            x_metals[layer], y_metals[layer] = get_continuous_metals(metals[layer])
            
            for metal in x_metals[layer] + y_metals[layer]:
                square = self.get_square(metal, layer)
                self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"][layer], cut=True)
                    

        for via_layer in vias:
            for (via, via_flag) in vias[via_layer]:
                if via_flag:
                    square = self.get_square(via, via_layer)
                    self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"][via_layer], cut=True)

                
                upper_layer_ = upper_layer[via]
                square = self.get_square(via, upper_layer_)
                self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"][upper_layer_], cut=True)
                
                lower_layer_ = lower_layer[via]
                square = self.get_square(via, lower_layer_)
                self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"][lower_layer_], cut=True)
            
            
    def get_square(self, metal, layer):            
        def get_vertical_metal_square(metal, layer):     
            y_value = metal[0][Y]
            
            if layer in design_rules["routing_layers"]:
                z = design_rules["routing_layers"].index(layer)
                routing_dir = design_rules["routing_directions"][z]
            else:
                routing_dir = BIDIRECTION
            
            if y_value % design_rules["cell_height"] == 0 and layer in design_rules["power_layer"]:
                x_ex = design_rules["width"][layer]
                y_ex = design_rules["power_width"][layer]
            else:
                x_ex = design_rules["extension"][layer] if routing_dir == HORIZONTAL else design_rules["width"][layer]
                y_ex = design_rules["extension"][layer] if routing_dir == VERTICAL else design_rules["width"][layer]

            x_value = metal[0][X]
            x_min = x_value - int(x_ex / 2)
            x_max = x_value + int(x_ex / 2)

            y_min = min(metal[0][Y], metal[1][Y])
            y_max = max(metal[0][Y], metal[1][Y])

            y_min = y_min - int(y_ex / 2)
            y_max = y_max + int(y_ex / 2)

            return x_min, y_min, x_max, y_max


        def get_horizontal_metal_square(metal, layer):
            y_value = metal[0][Y]
            
            if layer in design_rules["routing_layers"]:
                z = design_rules["routing_layers"].index(layer)
                routing_dir = design_rules["routing_directions"][z]
            else:
                routing_dir = BIDIRECTION
            
            if y_value % design_rules["cell_height"] == 0 and layer in design_rules["power_layer"]:
                x_ex = design_rules["width"][layer]
                y_ex = design_rules["power_width"][layer]
            else:
                x_ex = design_rules["extension"][layer] if routing_dir == HORIZONTAL else design_rules["width"][layer]
                y_ex = design_rules["extension"][layer] if routing_dir == VERTICAL else design_rules["width"][layer]

            y_min = y_value - int(y_ex / 2)
            y_max = y_value + int(y_ex / 2)
            
            x_min = min(metal[0][X], metal[1][X]) - int(x_ex / 2)
            x_max = max(metal[0][X], metal[1][X]) + int(x_ex / 2)
            
            polygon = sg.box(x_min, y_min, x_max, y_max)
            is_over_active = self.check_layer_overlap(polygon, "Active") 
            
            power_layer = design_rules["power_layer"][-1]
            if layer == design_rules["active_contact_layer"]:
                offset = design_rules["pitch"]["fin"] - (design_rules["width"][layer]/2 + design_rules["width"]["M1"]/2)
                if y_value == design_rules["power_width"][power_layer] + design_rules["spacing"]["S2S"][power_layer][power_layer]:
                    if is_over_active != False:
                        y_max += offset
                    
                if y_value == design_rules["cell_height"] - design_rules["power_width"][power_layer] - design_rules["spacing"]["S2S"][power_layer][power_layer]:        
                    if is_over_active != False:
                        y_min -= offset


            return x_min, y_min, x_max, y_max

        def get_point_square(point, layer):
            center_x = point[0]
            center_y = metal[0][Y]
            
            if layer in design_rules["routing_layers"]:
                z = design_rules["routing_layers"].index(layer)
                routing_dir = design_rules["routing_directions"][z]
            else:
                routing_dir = BIDIRECTION
            
            if center_y % design_rules["cell_height"] == 0 and layer in design_rules["power_layer"]:
                x_ex = design_rules["width"][layer]
                y_ex = design_rules["power_width"][layer]
            else:
                x_ex = design_rules["extension"][layer] if routing_dir == HORIZONTAL else design_rules["width"][layer]
                y_ex = design_rules["extension"][layer] if routing_dir == VERTICAL else design_rules["width"][layer]
                
            
            return (center_x - int(x_ex / 2), center_y - int(y_ex / 2),
                    center_x + int(x_ex / 2), center_y + int(y_ex / 2))


        if metal[0][X] == metal[1][X] and metal[0][Y] == metal[1][Y]:
            min_x, min_y, max_x, max_y = get_point_square(metal[0], layer)
            
        elif metal[0][X] == metal[1][X]:  # Vertical metal
            min_x, min_y, max_x, max_y = get_vertical_metal_square(metal, layer)
        
        elif metal[0][Y] == metal[1][Y]:  # Horizontal metal
            min_x, min_y, max_x, max_y = get_horizontal_metal_square(metal, layer)
                
        return (min_x, min_y, max_x, max_y)
    
    def add_poly(self):                
        num_poly = self.placement.get_num_poly()
        
        min_y = 0
        max_y = design_rules["cell_height"] * design_rules["num_row"]
        
        x_offset = design_rules["x_offset"]
        for x in range(num_poly):
            center_x = x_offset + x * design_rules["pitch"]["Gate"]
            
            min_x = center_x - design_rules["width"]["Gate"]/2
            max_x = center_x + design_rules["width"]["Gate"]/2
            
            square = (min_x, min_y, max_x, max_y)
            self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"]["Gate"])
    
    def add_fin(self):
        min_x = 0
        max_x = self.placement.get_num_poly() * design_rules["pitch"]["Gate"]

        num_fin = int(design_rules["cell_height"]*design_rules["num_row"] / design_rules["pitch"]["fin"])
        
        y_offset = design_rules["pitch"]["fin"]/2
        
        for i in range(num_fin):
            center_y = y_offset + i * design_rules["pitch"]["fin"]
            
            min_y = center_y - design_rules["width"]["fin"]/2
            max_y = center_y + design_rules["width"]["fin"]/2
            
            square = (min_x, min_y, max_x, max_y)
            self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"]["fin"])            
    
    
    def power_routing(self):
        power_metals = list()
        nets = self.placement.nets
        power_nets = [net for net in nets if net.is_power]
        for power_net in power_nets:
            pins = power_net.get_pins()
            
            contact_points = pins[0]
            power_rail_pins = pins[1:]
            
            first_power_rail_pin = power_rail_pins[0]
            power_rail_point = first_power_rail_pin.get_points()[0]
                        
            rail_y_pos = power_rail_point[Y]
            closest_point = min(contact_points.get_points(), key=lambda p: abs(p[1] - rail_y_pos))
            rail_point = (closest_point[X], rail_y_pos, closest_point[Z])
            
            power_metals.append((closest_point, rail_point))            
            contact_layer = design_rules["active_contact_layer"]
            for power_layer in design_rules["power_layer"]:
                if power_layer == contact_layer:
                    continue
                
                contact_layer_z = design_rules["routing_layers"].index(contact_layer)
                power_layer_z = design_rules["routing_layers"].index(power_layer)
                
                vias = set()
                zs = list()
                
                z = contact_layer_z
                if contact_layer_z < power_layer_z:
                    lower_via = design_rules["lower_via"][power_layer]
                    while z < power_layer_z:
                        layer = design_rules["routing_layers"][z]
                        upper_via = design_rules["upper_via"][layer]
                        
                        if upper_via in vias:
                            z += 1
                            continue

                        vias.add(upper_via)
                        zs.append(z)
                        
                        z += 1
                        
                        if upper_via == lower_via:
                            zs.append(power_layer_z)
                            break
                        
                
                else:
                    upper_via = design_rules["upper_via"][power_layer]
                    while power_layer_z < z:
                        layer = design_rules["routing_layers"][z]
                        lower_via = design_rules["lower_via"][layer]
                        
                        if lower_via in vias:
                            z -= 1
                            continue

                        vias.add(lower_via)
                        zs.append(z)
                        
                        z -= 1
                        
                        if upper_via == lower_via:
                            zs.append(power_layer_z)
                            break

                for z1, z2 in zip(zs[:-1], zs[1:]):
                    point1 = (rail_point[X], rail_point[Y], z1)
                    point2 = (rail_point[X], rail_point[Y], z2)
                    power_metals.append((point1, point2))
        
        self.write_metal(power_metals)
        
        
    def add_rail(self):
        num_row = design_rules["num_row"]
        
        min_x = 0
        max_x = self.placement.get_num_poly() * design_rules["pitch"]["Gate"]
        
        for num_rail in range(num_row+1):
            rail = num_rail*design_rules["cell_height"]

            for power_rail in design_rules["power_layer"]:
                min_y = rail - design_rules["power_width"][power_rail]/2
                max_y = rail + design_rules["power_width"][power_rail]/2
                
                square = (min_x, min_y, max_x, max_y)
                self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"][power_rail])

                layer_num = design_rules["layer_map"][design_rules["power_layer"][-1]]
                power = POWER_NET if (num_rail % 2) == 1 else GND_NET
                gds_point = gdspy.Label(power, ((min_x + (max_x-min_x)/2) / 1000, (min_y + (max_y-min_y)/2) / 1000), layer=layer_num, texttype=251, magnification=1/1000)
                self.cell.add(gds_point)
        
    
    def add_boundary(self):
        min_x = 0
        max_x = self.placement.get_num_poly() * design_rules["pitch"]["Gate"]
        
        min_y = 0
        max_y = design_rules["cell_height"]*design_rules["num_row"]
        
        square = (min_x, min_y, max_x, max_y)
        self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"]["BOUNDARY"])


    def add_well_and_pn_select(self):
        num_row = design_rules["num_row"]
        
        min_x = 0
        max_x = self.placement.get_num_poly() * design_rules["pitch"]["Gate"]
        
        for row in range(num_row):
            is_flip = (row % 2 == 1)
            
            min_y = row*design_rules["cell_height"]
            max_y = (row+1)*design_rules["cell_height"]
            
            p_min_y = int(min_y + (max_y-min_y)/2 + (design_rules["np_offset"] if not is_flip else -design_rules["np_offset"]))        
            n_max_y = p_min_y
            
            square = (min_x, p_min_y, max_x, max_y)
            self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"]["well"])
            gds_point = gdspy.Label(POWER_NET, ((max_x-min_x)/2 / 1000, (p_min_y+(max_y-p_min_y)/2) / 1000), layer=design_rules["layer_map"]["well"], texttype=251, magnification=1/1000)
            self.cell.add(gds_point)
            gds_point = gdspy.Label(GND_NET, ((max_x + design_rules["pitch"]["M1"]) / 1000, (p_min_y+(max_y-p_min_y)/2) / 1000), layer=design_rules["layer_map"]["P_SUB"], texttype=251, magnification=1/1000)
            self.cell.add(gds_point)
            
            self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"]["Pselect"])
            
            square = (min_x, min_y, max_x, n_max_y)
            self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"]["Nselect"])
        
        
    def add_gcut(self):
        num_row = design_rules["num_row"]

        double_pnet_orders, double_nnet_orders = self.placement.get_double_net_orders()
        
        x_offset = design_rules["x_offset"]

        for row in range(num_row):
            is_flip = (row % 2 == 1)

            min_y = row*design_rules["cell_height"]
            max_y = (row+1)*design_rules["cell_height"]
            
            center_y = int(min_y + (max_y-min_y)/2 + (design_rules["np_offset"] if not is_flip else -design_rules["np_offset"]))     
            
            if not is_flip:
                pnet_orders = double_pnet_orders[row]
                nnet_orders = double_nnet_orders[row]
            
            else:
                nnet_orders = double_pnet_orders[row]
                pnet_orders = double_nnet_orders[row]
            
            for x in range(0, len(pnet_orders), 2):
                if pnet_orders[x] == DUMMY_NET and nnet_orders[x] == DUMMY_NET:
                    center_x = x_offset + x * design_rules["x_unit"]
                    
                    min_x = center_x - design_rules["width"]["Gate"]/2 - design_rules["offset"]["GCut"]["Gate"]
                    max_x = center_x + design_rules["width"]["Gate"]/2 + design_rules["offset"]["GCut"]["Gate"]
                
                    min_y = center_y - design_rules["width"]["GCut"]/2
                    max_y = center_y + design_rules["width"]["GCut"]/2
                    square = (min_x, min_y, max_x, max_y)
                    self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"]["GCut"])
                
        min_x = 0
        max_x = self.placement.get_num_poly() * design_rules["pitch"]["Gate"]
        
        for row in range(num_row+1):
            rail = row*design_rules["cell_height"]
            
            min_y = rail - design_rules["width"]["GCut"]/2
            max_y = rail + design_rules["width"]["GCut"]/2
            
            square = (min_x, min_y, max_x, max_y)
            self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"]["GCut"])
    
    
    
    def add_active(self):
        num_row = design_rules["num_row"]
        double_pnum_fins, double_nnum_fins = self.placement.get_double_num_fins()

        assert len(double_pnum_fins[0]) == len(double_nnum_fins[0]), \
            f"Error: The lengths of 'pnum_fins' and 'nnum_fins' do not match. \
                'pnum_fins' has length {len(pnum_fins)}, while 'nnum_fins' has length {len(nnum_fins)}."

        x_offset = design_rules["x_offset"]
        y_offset = design_rules["pitch"]["M1"]-design_rules["width"]["M1"]/2
        
        for row in range(num_row):
            is_flip = (row % 2 == 1)
            
            if not is_flip:
                pnum_fins = double_pnum_fins[row]
                nnum_fins = double_nnum_fins[row]
            
            else:
                nnum_fins = double_pnum_fins[row]
                pnum_fins = double_nnum_fins[row]
            
            for x in range(2, len(pnum_fins), 2):
                center_x = x_offset + x * design_rules["x_unit"]
                    
                min_x = center_x - design_rules["width"]["Gate"]/2 - design_rules["offset"]["Gate"]["Active"]
                max_x = center_x + design_rules["width"]["Gate"]/2 + design_rules["offset"]["Gate"]["Active"]
                        
                max_y = (row+1)*design_rules["cell_height"] - y_offset
                min_y = max_y - design_rules["pitch"]["fin"] * pnum_fins[x]
                
                if max_y - min_y:
                    square = (min_x, min_y, max_x, max_y)
                    self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"]["Active"])
                            
                min_y = row*design_rules["cell_height"] + y_offset
                max_y = min_y + design_rules["pitch"]["fin"] * nnum_fins[x]
                    
                if max_y - min_y:
                    square = (min_x, min_y, max_x, max_y)
                    self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"]["Active"])    
                        
                        
    def add_sdt(self):
        num_row = design_rules["num_row"]
        double_pnum_fins, double_nnum_fins = self.placement.get_double_num_fins()
        double_pnet_orders, double_nnet_orders = self.placement.get_double_net_orders()
        nets = self.placement.get_nets()
        
        assert len(double_pnum_fins[0]) == len(double_nnum_fins[0]), \
            f"Error: The lengths of 'pnum_fins' and 'nnum_fins' do not match. \
                'pnum_fins' has length {len(pnum_fins)}, while 'nnum_fins' has length {len(nnum_fins)}."

        x_offset = design_rules["x_offset"]
        y_offset = design_rules["pitch"]["M1"] - design_rules["width"]["M1"]/2

        for row in range(num_row):
            is_flip = (row % 2 == 1)
            
            if not is_flip:
                pnum_fins = double_pnum_fins[row]
                nnum_fins = double_nnum_fins[row]

                pnet_orders = double_pnet_orders[row]
                nnet_orders = double_nnet_orders[row]
            
            else:
                nnum_fins = double_pnum_fins[row]
                pnum_fins = double_nnum_fins[row]

                nnet_orders = double_pnet_orders[row]
                pnet_orders = double_nnet_orders[row]
                
            for x in range(1, len(pnum_fins), 2):
                center_x = x_offset + x * design_rules["x_unit"]
                    
                min_x = center_x - design_rules["width"]["SDT"]/2 
                max_x = center_x + design_rules["width"]["SDT"]/2
                
                pnet = next((net for net in nets if pnet_orders[x] == net.name), None)

                if pnet == None:
                    pnet = next((net for net in nets if pnet_orders[x] in net.name), None)     
                
                if pnet != None and pnet.get_num_pins() > 1:
                    max_y = (row+1)*design_rules["cell_height"] - y_offset
                    min_y = max_y - design_rules["pitch"]["fin"] * pnum_fins[x]
                    
                    if max_y - min_y:
                        square = (min_x, min_y, max_x, max_y)
                        self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"]["SDT"])
                
                nnet = next((net for net in nets if nnet_orders[x] == net.name), None)

                if nnet == None:
                    nnet = next((net for net in nets if nnet_orders[x] in net.name), None)     
                
                if nnet != None and nnet.get_num_pins() > 1:
                    min_y = row*design_rules["cell_height"] + y_offset
                    max_y = min_y + design_rules["pitch"]["fin"] * nnum_fins[x]
                        
                    if max_y - min_y:
                        square = (min_x, min_y, max_x, max_y)
                        self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"]["SDT"])              


    def add_lisd(self):
        num_row = design_rules["num_row"]
        double_pnum_fins, double_nnum_fins = self.placement.get_double_num_fins()
        double_pnet_orders, double_nnet_orders = self.placement.get_double_net_orders()
        nets = self.placement.get_nets()
        
        assert len(double_pnum_fins[0]) == len(double_nnum_fins[0]), \
            f"Error: The lengths of 'pnum_fins' and 'nnum_fins' do not match. \
                'pnum_fins' has length {len(pnum_fins)}, while 'nnum_fins' has length {len(nnum_fins)}."

        x_offset = design_rules["x_offset"]
        y_offset = design_rules["pitch"]["M1"] - design_rules["width"]["M1"]/2

        for row in range(num_row):
            is_flip = (row % 2 == 1)
            
            if not is_flip:
                pnum_fins = double_pnum_fins[row]
                nnum_fins = double_nnum_fins[row]

                pnet_orders = double_pnet_orders[row]
                nnet_orders = double_nnet_orders[row]
            
            else:
                nnum_fins = double_pnum_fins[row]
                pnum_fins = double_nnum_fins[row]

                nnet_orders = double_pnet_orders[row]
                pnet_orders = double_nnet_orders[row]
            
            for x in range(1, len(pnum_fins), 2):
                center_x = x_offset + x * design_rules["x_unit"]
                    
                min_x = center_x - design_rules["width"]["LISD"]/2 
                max_x = center_x + design_rules["width"]["LISD"]/2
                
                pnet = next((net for net in nets if pnet_orders[x] == net.name), None)

                if pnet == None:
                    pnet = next((net for net in nets if pnet_orders[x] in net.name), None)
                
                if pnet != None and pnet.get_num_pins() > 1:
                    max_y = (row+1)*design_rules["cell_height"] - y_offset
                    min_y = max_y - design_rules["pitch"]["fin"] * pnum_fins[x]
                    
                    if max_y - min_y:
                        square = (min_x, min_y, max_x, max_y)
                        self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"]["LISD"])
                
                nnet = next((net for net in nets if nnet_orders[x] == net.name), None)

                if nnet == None:
                    nnet = next((net for net in nets if nnet_orders[x] in net.name), None)      

                if nnet != None and nnet.get_num_pins() > 1:
                    min_y = row*design_rules["cell_height"] + y_offset
                    max_y = min_y + design_rules["pitch"]["fin"] * nnum_fins[x]
                        
                    if max_y - min_y:
                        square = (min_x, min_y, max_x, max_y)
                        self.add_rectangle_to_shapely(square, layer=design_rules["layer_map"]["LISD"]) 
    
    
