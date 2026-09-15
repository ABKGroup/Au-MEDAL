"""Read the committed flat, orthogonal GDS fixtures for geometry-only tests.

Unsupported geometry is rejected. These helpers do not implement PDK DRC or LVS.
"""

from collections import defaultdict
import math
import struct


def _real8(data):
    return (-1 if data[0] & 128 else 1) * int.from_bytes(data[1:], "big") / 2**56 * 16**((data[0] & 127) - 64)


def read_gds(path):
    data = path.read_bytes()
    offset, scale, element, name = 0, None, None, None
    cells = {}
    ended = False
    while offset < len(data):
        if ended or offset + 4 > len(data):
            raise ValueError("trailing or truncated GDS record")
        length, kind, dtype = struct.unpack_from(">HBB", data, offset)
        if length < 4 or length % 2 or offset + length > len(data):
            raise ValueError("invalid GDS record length")
        payload = data[offset + 4:offset + length]
        offset += length
        if kind == 3:
            if len(payload) != 16:
                raise ValueError("invalid GDS units")
            scale = _real8(payload[8:]) * 1e9
        elif kind == 6:
            name = payload.rstrip(b"\0").decode("ascii")
            if name in cells:
                raise ValueError("duplicate GDS cell")
            cells[name] = []
        elif kind in (8, 12):
            if element is not None or name is None:
                raise ValueError("nested or unowned GDS element")
            element = {"type": kind}
        elif kind in (9, 10, 11, 21, 45):
            raise ValueError("fixture reader requires flat boundaries and text")
        elif kind in (13, 14, 22):
            if element is None or len(payload) != 2:
                raise ValueError("invalid GDS layer or purpose")
            element["layer" if kind == 13 else "datatype"] = struct.unpack(">h", payload)[0]
        elif kind == 16:
            if element is None or scale is None or len(payload) % 8:
                raise ValueError("invalid GDS coordinates")
            raw = struct.unpack(">" + "i" * (len(payload) // 4), payload)
            element["xy"] = [(round(raw[i] * scale, 6), round(raw[i+1] * scale, 6))
                             for i in range(0, len(raw), 2)]
        elif kind == 25:
            if element is None:
                raise ValueError("unowned GDS text")
            element["text"] = payload.rstrip(b"\0").decode("ascii")
        elif kind == 17:
            if element is None or not all(k in element for k in ("layer", "datatype", "xy")):
                raise ValueError("incomplete GDS element")
            cells[name].append(element)
            element = None
        elif kind == 4:
            ended = True
    if not ended or element is not None or not scale or not cells:
        raise ValueError("incomplete GDS library")
    return cells


def rectangles(polygon):
    if len(polygon) < 5 or polygon[0] != polygon[-1]:
        raise ValueError("open or degenerate boundary")
    if any(a != c and b != d for (a, b), (c, d) in zip(polygon, polygon[1:])):
        raise ValueError("non-orthogonal fixture boundary")
    xs = sorted({x for x, y in polygon})
    result = []
    for left, right in zip(xs, xs[1:]):
        x = (left + right) / 2
        ys = sorted(b for (a, b), (c, d) in zip(polygon, polygon[1:])
                    if b == d and min(a, c) < x < max(a, c))
        if len(ys) % 2:
            raise ValueError("unpaired orthogonal crossings")
        result.extend([left, ys[i], right, ys[i+1]] for i in range(0, len(ys), 2))
    return result


def covered(box, shapes):
    left, bottom, right, top = box
    if left >= right or bottom >= top:
        return False
    xs = sorted({left, right} | {x for r in shapes for x in (r[0], r[2]) if left < x < right})
    for a, b in zip(xs, xs[1:]):
        mid = (a + b) / 2
        intervals = sorted((max(bottom, r[1]), min(top, r[3])) for r in shapes
                           if r[0] <= mid <= r[2] and r[1] < top and r[3] > bottom)
        high = bottom
        for lo, hi in intervals:
            if lo > high:
                return False
            high = max(high, hi)
        if high < top:
            return False
    return True


def contains(box, x, y):
    return box[0] <= x <= box[2] and box[1] <= y <= box[3]


def grid_hit(box):
    return (math.ceil(box[0] / 480) * 480 <= box[2]
            and math.ceil(box[1] / 420) * 420 <= box[3])


def fixture(path, ports):
    cells = read_gds(path)
    if set(cells) != {path.stem}:
        raise ValueError("unexpected fixture cell set")
    layers = defaultdict(list)
    labels = []
    boundary = []
    for element in cells[path.stem]:
        if element["type"] == 8:
            parts = rectangles(element["xy"])
            if element["datatype"] == 0:
                layers[str(element["layer"])].extend(parts)
            if (element["layer"], element["datatype"]) == (189, 4):
                boundary.extend(parts)
        else:
            labels.append({"text": element["text"], "x": element["xy"][0][0],
                           "y": element["xy"][0][1], "layer": element["layer"],
                           "texttype": element["datatype"]})
    if len(boundary) != 1 or boundary[0][0:2] != [0, 0] or boundary[0][3] != 3780:
        raise ValueError("unexpected fixture boundary")
    return {"name": path.stem, "width_nm": boundary[0][2], "rects": dict(layers),
            "labels": labels, "ports": ports}
