import stdcellgen
import pickle
import os

import argparse 

def load(file_name, create_func):
    if os.path.exists(file_name):
        with open(file_name, "rb") as file:
            return pickle.load(file)
    else:
        data = create_func()
        with open(file_name, "wb") as file:
            pickle.dump(data, file)
        return data

def place(placeEnv, placement_file, solution_num):
    placer = stdcellgen.Placer(placeEnv=placeEnv)
    if placement_file == '':
        placement = placer.dynamic_programming(solution_num=solution_num)
    else:
        placement = placer.read_placement(placement_file=placement_file)
    return placement

def route(placement, pre_layout):
    router = stdcellgen.Router(placement=placement, pre_layout=pre_layout)
    routing = router.route()
    return routing

def print_placement_result(placement):    
    [net.print_info() for net in placement.get_nets()]
    placement.print_single_net_order()
    placement.print_fet_order()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--save_dir', type=str, default="./database", help="Set design result directory")
    parser.add_argument('--cell_name', type=str, required=True, help='cell name')
    parser.add_argument('--schematic', type=str, required=True, help='schematic')
    parser.add_argument('--config', type=str, required=True, help='configure')
    parser.add_argument('--placement_file', type=str, default='', help='placement_file')
    
    param = parser.parse_args()
    
    save_dir = param.save_dir
    cell_name = param.cell_name
    schematic = param.schematic
    config = param.config
    placement_file = param.placement_file
    
    stdcellgen.set_design_rules(config)
    
    circuit = stdcellgen.Circuit(cell_name=cell_name, file_name=schematic, save_dir=save_dir)
    database_dir =f"{save_dir}/{cell_name}"

    placeEnv = stdcellgen.PlaceEnv(circuit=circuit)
    layout = stdcellgen.Layout(cell_name=cell_name)

    placement_pkl = f"{database_dir}/placement.pkl"
    routing_pkl = f"{database_dir}/routing.pkl"

    done = False
    solution_num = 0
    while not done:
        solution_num += 1
        placement = load(placement_pkl, lambda: place(placeEnv=placeEnv, placement_file=placement_file, solution_num=solution_num))
        layout.set_placement(placement=placement)
        #print_placement_result(placement)
        routing = load(routing_pkl, lambda: route(placement=placement, pre_layout=layout))

        if routing[0] != False:
            done = True
        else:
            print("UNSAT")
            open(f"{save_dir}/ROUTING_UNSAT_{solution_num}.txt", "w").close()
            os.remove(placement_pkl)
            os.remove(routing_pkl)
        
        if solution_num == 1:
            break        

    #print("routing path: ", routing[0])
    if routing[0] != False:
        layout.set_routing(routing)
        layout.write_gds(database_dir)
