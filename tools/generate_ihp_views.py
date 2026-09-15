"""Generate CDL, SPICE and Xschem views from explicit total-W/ng, m=1 CDL."""

import argparse
from dataclasses import dataclass
from decimal import Decimal, InvalidOperation
from pathlib import Path
import re
import sys


IDENTIFIER = re.compile(r"[A-Za-z_][A-Za-z0-9_]*\Z")
NET_NAME = re.compile(r"[A-Za-z0-9_]+\Z")
NUMBER = r"(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?"
DIMENSION = re.compile(r"(" + NUMBER + r")([unp]?)\Z", re.I)
MODELS = {"sg13_lv_nmos", "sg13_lv_pmos"}


@dataclass(frozen=True)
class Device:
    name: str
    nets: tuple
    model: str
    w: str
    l: str
    ng: int


@dataclass(frozen=True)
class Circuit:
    name: str
    ports: tuple
    directions: tuple
    devices: tuple


def _lines(text):
    pending = None
    for number, raw in enumerate(text.splitlines(), 1):
        line = raw.strip()
        if not line:
            continue
        if line.startswith("+"):
            if pending is None or pending[1].startswith("*"):
                raise ValueError(f"line {number}: orphan continuation")
            pending = (pending[0], pending[1] + " " + line[1:].strip())
        else:
            if pending is not None:
                yield pending
            pending = (number, line)
    if pending is not None:
        yield pending


def _dimension(token, parameter):
    match = DIMENSION.fullmatch(token)
    if match is None or Decimal(match[1]) <= 0:
        raise ValueError(f"{parameter} must be a positive numeric SI value or use u/n/p")
    return token


def parse_cdl(text):
    """Parse one flat subcircuit with explicit numeric w, l, ng and m=1."""
    name, ports, pininfo = None, (), None
    devices, seen_devices, spellings = [], set(), {}
    ended = False

    def net(token):
        if not NET_NAME.fullmatch(token):
            raise ValueError(f"unsupported net name {token!r}: use letters, digits and underscores")
        previous = spellings.setdefault(token.lower(), token)
        if previous != token:
            raise ValueError(f"inconsistent net case: {previous!r} and {token!r}")
        return token

    for number, line in _lines(text):
        try:
            fields = line.split()
            head = fields[0].lower()
            if head == "*.pininfo":
                if name is None or ended or pininfo is not None:
                    raise ValueError("PININFO must occur once inside the subcircuit")
                pininfo = {}
                for entry in fields[1:]:
                    parts = entry.split(":")
                    if len(parts) != 2 or parts[1].upper() not in {"I", "O", "B"}:
                        raise ValueError(f"invalid PININFO entry {entry!r}")
                    pin, direction = parts
                    if pin in pininfo:
                        raise ValueError(f"duplicate PININFO port {pin!r}")
                    pininfo[net(pin)] = direction.upper()
            elif line.startswith("*"):
                if head.startswith(("*.pin", "*.sub", "*.global", "*.param")):
                    raise ValueError(f"unsupported CDL metadata {fields[0]!r}")
            elif head == ".subckt":
                if name is not None:
                    raise ValueError("expected exactly one flat subcircuit")
                if len(fields) < 3 or not IDENTIFIER.fullmatch(fields[1]):
                    raise ValueError("SUBCKT needs a simple name and at least one port")
                name = fields[1]
                if name.upper() in {"CON", "PRN", "AUX", "NUL", *(f"COM{i}" for i in range(1, 10)),
                                    *(f"LPT{i}" for i in range(1, 10))}:
                    raise ValueError("subcircuit name is a reserved filename")
                ports = tuple(net(p) for p in fields[2:])
                if len(set(ports)) != len(ports):
                    raise ValueError("duplicate subcircuit port")
            elif head == ".ends":
                if name is None or ended or len(fields) > 2:
                    raise ValueError("unexpected ENDS")
                if len(fields) == 2 and fields[1].lower() != name.lower():
                    raise ValueError("ENDS name does not match SUBCKT")
                ended = True
            else:
                if name is None or ended:
                    raise ValueError("expected SUBCKT or a comment outside the subcircuit")
                if head[0] != "m" or not IDENTIFIER.fullmatch(fields[0]) or len(fields) < 10:
                    raise ValueError("expected a flat MOS device with D G S B model w l ng m")
                if head in seen_devices:
                    raise ValueError(f"duplicate device {fields[0]!r}")
                seen_devices.add(head)
                if fields[5].lower() not in MODELS:
                    raise ValueError(f"unsupported model {fields[5]!r}")
                attrs = {}
                for field in fields[6:]:
                    pair = field.split("=")
                    if len(pair) != 2 or not pair[1]:
                        raise ValueError(f"expected parameter=value, got {field!r}")
                    key, value = pair[0].lower(), pair[1]
                    if key not in {"w", "l", "ng", "m"}:
                        raise ValueError(f"unsupported or ambiguous parameter {key!r}")
                    if key in attrs:
                        raise ValueError(f"duplicate parameter {key!r}")
                    attrs[key] = value
                if set(attrs) != {"w", "l", "ng", "m"}:
                    raise ValueError("explicit total w, l, ng and m=1 are required")
                if not re.fullmatch(NUMBER, attrs["m"]) or Decimal(attrs["m"]) != 1:
                    raise ValueError("m must be 1: provide explicit total W/ng, multiplier conversion is ambiguous")
                if not re.fullmatch(r"[0-9]+", attrs["ng"]) or int(attrs["ng"]) < 1:
                    raise ValueError("ng must be a positive integer finger count")
                devices.append(Device(fields[0], tuple(net(p) for p in fields[1:5]), fields[5],
                                      _dimension(attrs["w"], "w"), _dimension(attrs["l"], "l"),
                                      int(attrs["ng"])))
        except InvalidOperation as error:
            raise ValueError(f"line {number}: numeric literal outside the supported Decimal range") from error
        except ValueError as error:
            raise ValueError(f"line {number}: {error}") from error
    if name is None or not ended or not devices:
        raise ValueError("expected one complete SUBCKT/ENDS containing MOS devices")
    if pininfo is not None and set(pininfo) != set(ports):
        raise ValueError("PININFO must describe every declared port exactly once")
    directions = tuple(pininfo[p] if pininfo is not None else "B" for p in ports)
    return Circuit(name, ports, directions, tuple(devices))


def netlist(circuit, spice=False):
    lines = [f".SUBCKT {circuit.name} {' '.join(circuit.ports)}",
             "*.PININFO " + " ".join(f"{p}:{d}" for p, d in zip(circuit.ports, circuit.directions))]
    for device in circuit.devices:
        name = "X" + device.name if spice else device.name
        lines.append(f"{name} {' '.join(device.nets)} {device.model} "
                     f"w={device.w} l={device.l} ng={device.ng} m=1")
    lines.append(f".ENDS {circuit.name}")
    return "\n".join(lines) + "\n"


def _stacks(devices, pmos):
    # Only identical terminal nets form a stack, with no source/drain swapping.
    remaining = sorted(devices, key=lambda device: device.name.lower())
    up, down = (2, 0) if pmos else (0, 2)
    result = []
    while remaining:
        lower_nets = {device.nets[down] for device in remaining}
        roots = [device for device in remaining if device.nets[up] not in lower_nets]
        current = roots[0] if roots else remaining[0]
        stack = []
        while current is not None:
            stack.append(current)
            remaining.remove(current)
            current = next((other for other in remaining
                            if other.nets[up] == current.nets[down]), None)
        result.append(stack)
    return result


def _labelled_schematic(circuit):
    """Draw vertical series stacks with PMOS above NMOS and explicit net labels."""
    groups = [_stacks([d for d in circuit.devices if d.model.lower() == model], pmos)
              for model, pmos in (("sg13_lv_pmos", True), ("sg13_lv_nmos", False))]
    p_depth, n_depth = [max((len(s) for s in group), default=1) for group in groups]
    middle, bottom = p_depth * 160 + 80, (p_depth + n_depth) * 160 + 200
    columns = max(len(group) for group in groups)
    pitch = max(300, max(len(d.w) + len(d.l) for d in circuit.devices) * 7 + 160)
    pitch = ((pitch + 19) // 20) * 20
    right = (columns - 1) * pitch + 260
    lines = ["v {xschem version=3.4.7 file_version=1.2}", "G {}", "K {}",
             "V {}", "S {}", "E {}", f"T {{{circuit.name}}} -240 -80 0 0 0.4 0.4 {{}}"]
    components, labels, rail_points, endpoints = [], {}, {"VDD": [], "VSS": []}, [{}, {}]

    def wire(x, y, u, v):
        if (x, y) != (u, v):
            lines.append(f"N {x} {y} {u} {v} {{}}")

    def label(x, y, name):
        if (x, y) in labels:
            if labels[x, y] != name:
                raise ValueError("schematic label collision")
            return
        labels[x, y] = name
        components.append(f"C {{lab_wire.sym}} {x} {y} 0 0 "
                          f"{{name=l{len(labels)} sig_type=std_logic lab={name}}}")

    for row, group in enumerate(groups):
        pmos = row == 0
        up, down = (2, 0) if pmos else (0, 2)
        for column, stack in enumerate(group):
            x = column * pitch
            first_y = middle - 100 - (len(stack) - 1) * 160 if pmos else middle + 100
            for index, device in enumerate(stack):
                y = first_y + index * 160
                upper_y = y - (80 if index else 60)
                lower_y = y + (80 if index < len(stack) - 1 else 60)
                if pmos and index == 0 and device.nets[up] == "VDD":
                    upper_y = 0
                    rail_points["VDD"].append(x + 20)
                if not pmos and index == len(stack) - 1 and device.nets[down] == "VSS":
                    lower_y = bottom
                    rail_points["VSS"].append(x + 20)
                wire(x + 20, y - 30, x + 20, upper_y)
                wire(x + 20, y + 30, x + 20, lower_y)
                label(x + 20, upper_y, device.nets[up])
                label(x + 20, lower_y, device.nets[down])
                wire(x - 100, y, x - 20, y)
                label(x - 100, y, device.nets[1])
                wire(x + 20, y, x + 30, y)
                wire(x + 30, y, x + 30, y - 40)
                wire(x + 30, y - 40, x + 100, y - 40)
                label(x + 100, y - 40, device.nets[3])
                components.append(f"C {{{device.model.lower()}.sym}} {x} {y} 0 0 "
                                  f"{{name={device.name} w={device.w} l={device.l} ng={device.ng} "
                                  f"m=1 model={device.model} spiceprefix=X}}")
                if (pmos and index == len(stack) - 1) or (not pmos and index == 0):
                    endpoints[row][column] = (x + 20, lower_y if pmos else upper_y, device.nets[0])
    for column, (x, y, name) in endpoints[0].items():
        other = endpoints[1].get(column)
        if other is not None and other[2] == name:
            wire(x, y, other[0], other[1])
    for name, y in (("VDD", 0), ("VSS", bottom)):
        if rail_points[name]:
            wire(-160, y, max(rail_points[name]), y)
            label(-160, y, name)
    for index, (port, direction) in enumerate(zip(circuit.ports, circuit.directions), 1):
        x = right if direction == "O" else -240
        y = middle + (index - 1) * 40
        symbol = {"I": "ipin", "O": "opin", "B": "iopin"}[direction]
        wire(x, y, x - 40 if direction == "O" else x + 40, y)
        components.append(f"C {{devices/{symbol}.sym}} {x} {y} 0 0 {{name=p{index} lab={port}}}")
    return "\n".join(lines + components) + "\n"


# Stage/SP topology layout adapted from the retained track_sch3/scripts/gen_sch.py.
# Its old width/multiplier conversion and device/net renaming are NOT used here.
class _LayoutNotApplicable(Exception):
    pass


def _comps(edges, block):
    par = list(range(len(edges)))

    def f(i):
        while par[i] != i:
            par[i] = par[par[i]]
            i = par[i]
        return i
    seen = {}
    for i, (_, a, b) in enumerate(edges):
        for n in (a, b):
            if n in block:
                continue
            if n in seen:
                par[f(i)] = f(seen[n])
            else:
                seen[n] = i
    groups = {}
    for i in range(len(edges)):
        groups.setdefault(f(i), []).append(edges[i])
    return list(groups.values())


def _reach(edges, src, block):
    got = {src}
    grow = True
    while grow:
        grow = False
        for _, a, b in edges:
            for u, v in ((a, b), (b, a)):
                if u in got and v not in got and v != block:
                    got.add(v)
                    grow = True
    return got


def _sp(edges, top, bot):
    if len(edges) == 1:
        i, a, b = edges[0]
        if {a, b} != {top, bot}:
            raise _LayoutNotApplicable('network is not series-parallel near %s %s' % (top, bot))
        return ('leaf', i, top, bot)
    groups = _comps(edges, {top, bot})
    if len(groups) > 1:
        kids = []
        for g in groups:
            k = _sp(g, top, bot)
            kids += k[1] if k[0] == 'par' else [k]
        return ('par', kids)
    order = []
    frontier = [top]
    seen = {top}
    while frontier:
        nxt = []
        for u in frontier:
            for _, a, b in edges:
                for x, y in ((a, b), (b, a)):
                    if x == u and y not in seen:
                        seen.add(y)
                        order.append(y)
                        nxt.append(y)
        frontier = nxt
    for v in order:
        if v in (top, bot):
            continue
        comp = _reach(edges, top, v)
        if bot in comp:
            continue
        upper = [e for e in edges if e[1] in comp or e[2] in comp]
        lower = [e for e in edges if not (e[1] in comp or e[2] in comp)]
        kids = []
        for k in (_sp(upper, top, v), _sp(lower, v, bot)):
            kids += k[1] if k[0] == 'ser' else [k]
        return ('ser', kids)
    raise _LayoutNotApplicable('network is not series-parallel between %s and %s' % (top, bot))


def _size(t):
    if t[0] == 'leaf':
        return (1, 1)
    ss = [_size(k) for k in t[1]]
    if t[0] == 'ser':
        return (max(s[0] for s in ss), sum(s[1] for s in ss))
    return (sum(s[0] for s in ss), max(s[1] for s in ss))


def _leaves(t):
    if t[0] == 'leaf':
        return [t[1]]
    out = []
    for k in t[1]:
        out += _leaves(k)
    return out


def _sort_par(t, key):
    if t[0] == 'leaf':
        return t
    kids = [_sort_par(k, key) for k in t[1]]
    if t[0] == 'par':
        kids.sort(key=lambda k: min(key(i) for i in _leaves(k)))
    return (t[0], kids)


def _stages_cmos(devs, ports):
    par = list(range(len(devs)))

    def f(i):
        while par[i] != i:
            par[i] = par[par[i]]
            i = par[i]
        return i
    first = {}
    for i, d in enumerate(devs):
        for n in (d['d'], d['s']):
            if n in ('VDD', 'VSS'):
                continue
            if n in first:
                par[f(i)] = f(first[n])
            else:
                first[n] = i
    groups = {}
    for i in range(len(devs)):
        groups.setdefault(f(i), []).append(i)
    gates = set(d['g'] for d in devs)
    out = []
    for members in groups.values():
        P = [i for i in members if devs[i]['typ'] == 'p']
        N = [i for i in members if devs[i]['typ'] == 'n']
        pn = set()
        nn = set()
        for i in P:
            pn |= {devs[i]['d'], devs[i]['s']}
        for i in N:
            nn |= {devs[i]['d'], devs[i]['s']}
        o = (pn & nn) - set(('VDD', 'VSS'))
        if len(o) != 1 or not P or not N:
            raise _LayoutNotApplicable('stage with devices %s has outputs %s' % ([devs[i]['name'] for i in members], sorted(o)))
        o = o.pop()
        if 'VSS' in pn or 'VDD' in nn:
            raise _LayoutNotApplicable('pull-up touches VSS or pull-down touches VDD')
        for i in P:
            if devs[i]['b'] != 'VDD':
                raise _LayoutNotApplicable('pmos body not VDD: ' + devs[i]['name'])
        for i in N:
            if devs[i]['b'] != 'VSS':
                raise _LayoutNotApplicable('nmos body not VSS: ' + devs[i]['name'])
        for n in (pn | nn) - set(('VDD', 'VSS')) - {o}:
            if n in ports or n in gates:
                raise _LayoutNotApplicable('internal node %s is used outside its stage' % n)
        ptree = _sp([(i, devs[i]['d'], devs[i]['s']) for i in P], 'VDD', o)
        ntree = _sp([(i, devs[i]['d'], devs[i]['s']) for i in N], o, 'VSS')
        out.append(dict(out=o, P=P, N=N, ptree=ptree, ntree=ntree,
                        inputs=set(devs[i]['g'] for i in members)))
    # topological order, ties by first input port
    order = []
    left = list(range(len(out)))
    pidx = {p: k for k, p in enumerate(ports)}
    while left:
        ready = [k for k in left if not any(out[j]['out'] in out[k]['inputs'] for j in left if j != k)]
        if not ready:
            raise _LayoutNotApplicable('stage loop')
        ready.sort(key=lambda k: min(pidx.get(n, 999) for n in out[k]['inputs']))
        order.append(ready[0])
        left.remove(ready[0])
    return [out[k] for k in order]


def _cmos_schematic(circuit):
    """Reuse the historical stage/SP layout, never its parameter conversion."""
    rails = {'VDD', 'VSS'}
    if not rails <= set(circuit.ports):
        raise _LayoutNotApplicable('no explicit VDD/VSS rails')
    devs = [dict(name=d.name, d=d.nets[0], g=d.nets[1], s=d.nets[2], b=d.nets[3],
                 typ='p' if d.model.lower() == 'sg13_lv_pmos' else 'n')
            for d in circuit.devices]
    sts = _stages_cmos(devs, circuit.ports)
    inputs = [p for p in circuit.ports if p not in rails | {s['out'] for s in sts}]
    if any(s['out'] in circuit.ports for s in sts[:-1]):
        raise _LayoutNotApplicable('multiple external stage outputs')
    if any(p not in {d['g'] for d in devs} for p in inputs):
        raise _LayoutNotApplicable('non-gate input port')
    pidx = {p: k for k, p in enumerate(circuit.ports)}
    for s in sts:
        key = lambda i: (pidx.get(devs[i]['g'], 900), devs[i]['g'])
        s['ptree'] = _sort_par(s['ptree'], key)
        s['ntree'] = _sort_par(s['ntree'], key)
    hp, hn = (max(_size(s[t])[1] for s in sts) for t in ('ptree', 'ntree'))
    yp, mid, yn, bottom = hp * 120, hp * 120 + 60, hp * 120 + 120, (hp + hn) * 120 + 120
    pitch = max(260, max(len(d.w) + len(d.l) for d in circuit.devices) * 7 + 160)
    pitch = ((pitch + 19) // 20) * 20
    col = 0
    for s in sts:
        s['c0'], s['w'] = col, max(_size(s['ptree'])[0], _size(s['ntree'])[0])
        col += s['w']
    right = (col - 1) * pitch + 260
    wires, components, pos, anchors = set(), [], {}, {}

    def wire(x, y, u, v):
        if (x, y) != (u, v):
            assert x == u or y == v
            wires.add(tuple(sorted(((x, y), (u, v)))))

    def join(y, xs):
        xs = sorted(set(xs))
        for a, b in zip(xs, xs[1:]):
            wire(a, y, b, y)

    def label(x, y, name):
        components.append(f'C {{lab_wire.sym}} {x} {y} 0 0 '
                          f'{{name=l{len(components)} sig_type=std_logic lab={name}}}')

    def place(t, c0, r0, ybase):
        if t[0] == 'leaf':
            _, i, top, bot = t
            x, y = c0 * pitch, ybase + r0 * 120 + 60
            d = circuit.devices[i]
            up, down = (2, 0) if devs[i]['typ'] == 'p' else (0, 2)
            if d.nets[up] != top or d.nets[down] != bot:
                raise _LayoutNotApplicable('source/drain orientation needs a labelled view')
            pos[i] = (x, y)
            wire(x + 20, y - 60, x + 20, y - 30)
            wire(x + 20, y + 30, x + 20, y + 60)
            anchors.setdefault(top, []).append((x + 20, y - 60))
            anchors.setdefault(bot, []).append((x + 20, y + 60))
            return [x + 20], [x + 20]
        if t[0] == 'ser':
            row, tops, previous = r0, None, None
            for child in t[1]:
                ct, cb = place(child, c0, row, ybase)
                if previous is None:
                    tops = ct
                else:
                    join(ybase + row * 120, previous + ct)
                previous = cb
                row += _size(child)[1]
            return tops, previous
        height = _size(t)[1]
        tops, bots = [], []
        for child in t[1]:
            width, depth = _size(child)
            ct, cb = place(child, c0, r0, ybase)
            if depth < height:
                join(ybase + (r0 + depth) * 120, cb)
                x = min(cb)
                wire(x, ybase + (r0 + depth) * 120, x, ybase + (r0 + height) * 120)
                cb = [x]
            tops += ct
            bots += cb
            c0 += width
        return tops, bots

    vdd, vss, direct = [right], [right], set()
    for k, s in enumerate(sts):
        ph, nh = _size(s['ptree'])[1], _size(s['ntree'])[1]
        pt, pb = place(s['ptree'], s['c0'], hp - ph, 0)
        nt, nb = place(s['ntree'], s['c0'], 0, yn)
        for x in pt:
            wire(x, (hp - ph) * 120, x, 0)
        for x in nb:
            wire(x, yn + nh * 120, x, bottom)
        for x in pb:
            wire(x, yp, x, mid)
        for x in nt:
            wire(x, yn, x, mid)
        vdd += pt
        vss += nb
        s['bus'] = pb + nt
        inv = len(s['P']) == len(s['N']) == 1 and devs[s['P'][0]]['g'] == devs[s['N'][0]]['g']
        g = devs[s['P'][0]]['g']
        s['direct'] = inv and ((k and sts[k - 1]['out'] == g) or (k == 0 and g in inputs))
        if s['direct']:
            direct.update(s['P'] + s['N'])
    port_xy = {'VDD': (right, 0), 'VSS': (right, bottom)}
    for k, s in enumerate(sts):
        if not s['direct']:
            continue
        xp, p_y = pos[s['P'][0]]
        xn, n_y = pos[s['N'][0]]
        xg = xp - 60
        wire(xp - 20, p_y, xg, p_y)
        wire(xn - 20, n_y, xg, n_y)
        wire(xg, p_y, xg, mid)
        wire(xg, mid, xg, n_y)
        s['xg'] = xg
        if k == 0:
            port_xy[devs[s['P'][0]]['g']] = (-240, mid)
            wire(-240, mid, xg, mid)
    for k, s in enumerate(sts):
        net, xs = s['out'], list(s['bus'])
        if net in circuit.ports:
            port_xy[net] = (right, mid)
            xs.append(right)
        else:
            x = (s['c0'] + s['w'] - 1) * pitch + 100
            xs.append(x)
            label(x, mid, net)
        if k + 1 < len(sts) and sts[k + 1]['direct']:
            xs.append(sts[k + 1]['xg'])
        join(mid, xs)
    # Name internal series nodes on horizontal stubs, not on vertical wires.
    for net, points in anchors.items():
        if net not in rails | {s['out'] for s in sts}:
            x, y = max(points)
            wire(x, y, x + 80, y)
            label(x + 80, y, net)
    for i, d in enumerate(circuit.devices):
        x, y = pos[i]
        # Official attributes start at x+31.25; exit left of that band
        # and run above the W anchor (y-26.25), on the existing 10 grid.
        wire(x + 20, y, x + 30, y)
        wire(x + 30, y, x + 30, y - 40)
        wire(x + 30, y - 40, x + 100, y - 40)
        label(x + 100, y - 40, d.nets[3])
        if i not in direct:
            wire(x - 100, y, x - 20, y)
            label(x - 100, y, d.nets[1])
        components.append(f'C {{{d.model.lower()}.sym}} {x} {y} 0 0 '
                          f'{{name={d.name} w={d.w} l={d.l} ng={d.ng} m=1 model={d.model} spiceprefix=X}}')
    anchor = next((inputs.index(p) for p in inputs if p in port_xy), (len(inputs) - 1) / 2)
    for j, p in enumerate(inputs):
        if p not in port_xy:
            y = int(mid + (j - anchor) * 40)
            direction = circuit.directions[circuit.ports.index(p)]
            x = -240 if direction == 'I' else -200
            wire(-240, y, -200, y)
            port_xy[p] = (x, y)
    join(0, vdd)
    join(bottom, vss)
    # Component declaration order, including supply ports, matches the CDL.
    for i, (p, direction) in enumerate(zip(circuit.ports, circuit.directions), 1):
        x, y = port_xy[p]
        sym = {'I': 'ipin', 'O': 'opin', 'B': 'iopin'}[direction]
        components.append(f'C {{devices/{sym}.sym}} {x} {y} 0 0 {{name=p{i} lab={p}}}')
    lines = ['v {xschem version=3.4.7 file_version=1.2}', 'G {}', 'K {}', 'V {}', 'S {}', 'E {}',
             f'T {{{circuit.name}}} -240 -80 0 0 0.4 0.4 {{}}']
    lines += [f'N {x} {y} {u} {v} {{}}' for (x, y), (u, v) in sorted(wires)]
    return '\n'.join(lines + components) + '\n'


def schematic(circuit):
    """Wire CMOS stages; retain exact labelled terminals for other topologies."""
    try:
        return _cmos_schematic(circuit)
    except _LayoutNotApplicable:
        # Non-CMOS, cyclic or oppositely oriented sources must not be transformed.
        return _labelled_schematic(circuit)


def symbol(circuit):
    # B records retain SUBCKT order even though outputs are drawn on the right.
    half_width = max(100, len(circuit.name) * 4, max(map(len, circuit.ports)) * 8)
    half_width = ((half_width + 19) // 20) * 20
    bottom = len(circuit.ports) * 40 + 20
    lines = ["v {xschem version=3.4.7 file_version=1.2}", "G {}",
             f'K {{type=subcircuit format="@name @pinlist {circuit.name}" '
             f'template="name=x1" schematic="{circuit.name}.sch"}}',
             "V {}", "S {}", "E {}",
             f"B 4 {-half_width} 0 {half_width} {bottom} {{}}",
             "T {@name} 0 -40 0 0 0.2 0.2 {}",
             f"T {{{circuit.name}}} {-half_width + 10} -20 0 0 0.2 0.2 {{}}"]
    for index, (port, direction) in enumerate(zip(circuit.ports, circuit.directions), 1):
        side = 1 if direction == "O" else -1
        x, y = side * (half_width + 20), index * 40
        kind = {"I": "in", "O": "out", "B": "inout"}[direction]
        lines.extend([
            f"L 4 {x} {y} {side * half_width} {y} {{}}",
            f"B 5 {x - 2.5} {y - 2.5} {x + 2.5} {y + 2.5} {{name={port} dir={kind}}}",
            f"T {{{port}}} {side * (half_width - 10)} {y - 10} 0 {1 if side > 0 else 0} 0.2 0.2 {{}}",
        ])
    return "\n".join(lines) + "\n"


def generate(source, output_dir):
    """Write four fresh files after parsing and collision preflight."""
    source, output_dir = Path(source).resolve(strict=True), Path(output_dir).resolve()
    if output_dir == source.parent:
        raise ValueError("output directory must be separate from the source directory")
    raw = source.read_bytes()
    lf_only = raw.replace(b"\r\n", b"")
    if b"\r" in lf_only or (b"\r\n" in raw and b"\n" in lf_only):
        raise ValueError("source must use consistent LF or CRLF line endings")
    newline = "\r\n" if b"\r\n" in raw else "\n"
    circuit = parse_cdl(raw.decode("utf-8-sig"))
    payloads = {".cdl": netlist(circuit), ".spi": netlist(circuit, spice=True),
                ".sch": schematic(circuit), ".sym": symbol(circuit)}
    targets = {output_dir / (circuit.name + extension): body
               for extension, body in payloads.items()}
    for path in targets:
        if path.exists() or path.is_symlink():
            raise ValueError(f"output already exists: {path}")
    output_dir.mkdir(parents=True, exist_ok=True)
    for path, body in targets.items():
        with path.open("xb") as stream:
            stream.write(body.replace("\n", newline).encode("utf-8"))
    return tuple(targets)


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Generate CDL/SPI/SCH/SYM from one flat IHP total-W/ng, m=1 CDL.",
        epilog="Requires explicit numeric w, l, ng and m=1. Bare W/L values use SI metres. "
               "PININFO I/O/B supplies port directions, otherwise ports are inout. "
               "Xschem resolves sg13_lv_nmos.sym and sg13_lv_pmos.sym through its IHP library path.",
        allow_abbrev=False,
    )
    parser.add_argument("source", type=Path, help="source CDL, read only")
    parser.add_argument("--output-dir", type=Path, required=True,
                        help="separate destination directory, existing files are never replaced")
    args = parser.parse_args(argv)
    try:
        paths = generate(args.source, args.output_dir)
    except (OSError, UnicodeError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    for path in paths:
        print(path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
