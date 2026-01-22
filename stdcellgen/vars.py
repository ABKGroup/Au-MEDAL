N_TYPE = -1
P_TYPE = 1
VIRTUAL = 0


### For connection of MOSFET's terminal, please don't modify this numbers (Specially, DRAIN, GATE, SOURCE.)
DRAIN = 1
GATE = 2
SOURCE = 3
EXT_PIN = 4
ACCESS_POINT = 5

X = 0
Y = 1
Z = 2

## For Metal Direction ##
UPPER = 0
LOWER = 1
LEFT = 0 # X axis
RIGHT = 1 # X axis
TOP = 2 # Y axis
BOTTOM = 3 # Y axis


## For Corner
TOP_LEFT = 0
TOP_RIGHT = 1
BOTTOM_LEFT = 2
BOTTOM_RIGHT = 3

## For Metal Type
SIDE = 0
TIP = 1
CORNER = 2

### For layer direction
HORIZONTAL = 0
VERTICAL = 1
BIDIRECTION = 2
POINT = 3


POWER_NET = "VDD"
GND_NET = "VSS"

def INV_TYPE(TYPE):
    return -1*TYPE
    
DUMMY_NET = "-"
DUMMY_NAME = "DUMMY"