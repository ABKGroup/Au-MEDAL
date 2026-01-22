import copy
from tqdm import *

from .structure import *
from scipy.stats import norm



class Placer():
    def __init__(self, placeEnv):
        self.placeEnv = placeEnv

    def read_placement(self, placement_file):
        self.placeEnv.circuit.read_placement(placement_file)
        n, p = self.placeEnv.circuit.get_each_fets()
        self.placeEnv.set_fet_orders(p, n)

        return copy.deepcopy(self.placeEnv)


    ####### AutoCellGen calling #######
    def dynamic_programming(self, solution_num):
        self.placeEnv.circuit.run_dynamic_programming(solution_num=solution_num)
        n, p = self.placeEnv.circuit.get_each_fets()
        self.placeEnv.set_fet_orders(p, n)

        return copy.deepcopy(self.placeEnv)