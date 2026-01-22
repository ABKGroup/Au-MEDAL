
import z3
from itertools import combinations

def Equal(var1, var2):
    t1 = Or(Not(var1), var2)
    t2 = Or(Not(var2), var1)

    return And(t1, t2)

def Exactly(vars, num):        
    return And(z3.AtMost(*vars, num), z3.AtLeast(*vars, num))

def get_point(x, y, z):
    return None if None in (x, y, z) else (x, y, z)

def get_position(lst, idx):
    return lst[idx] if 0 <= idx < len(lst) else None

def AtMost(vars, k):
    clauses = []
    for comb in combinations(vars, k + 1):
        clause = Not(And(*comb))
        clauses.append(clause)
    return And(*clauses)

def Not_Exactly_one(vars):
    var1 = z3.AtMost(*vars, 2)
    
    or_conditions = []
    for i in range(len(vars)):
        condition = Or(*[Not(vars[i]) if j == i else vars[j] for j in range(len(vars))])
        or_conditions.append(condition)
    
    var2 = And(*or_conditions)
    
    return And(var1, var2)    

def And(*conditions, must_match_num=None, default=False):
    valid = []       
    count_valid = 0  

    for cond in conditions:
        if cond is None:
            continue
        count_valid += 1
        if cond is False:
            return False
        if cond is not True:
            valid.append(cond)

    if count_valid == 0:
        return default

    if must_match_num is not None and must_match_num != count_valid:
        return False

    if not valid:
        return True

    if len(valid) == 1:
        return valid[0]

    return z3.And(*valid)


def Or(*conditions, default=False):
    conds = []
    for cond in conditions:
        if cond is None:
            continue
        if cond is True:
            return True
        if cond is not False:
            conds.append(cond)
    if not conds:
        return default
    if len(conds) == 1:
        return conds[0]
    return z3.Or(*conds)


def Not(condition, default=True):
    if condition is True:
        return False
    if condition is False:
        return True
    if condition is None:
        return default
    return z3.Not(condition)