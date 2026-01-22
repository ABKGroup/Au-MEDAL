import gdstk
from collections import defaultdict
from pathlib import Path
import numpy as np
from shapely.geometry import box as sbox


class cell:
    def __init__(self, gds_path, cell_name):
        self.cell_name  = cell_name
        self.gds_path   = gds_path
        self.layer_data = None
        self.rect_data  = None

        self.read_layer_data()
        self.layer_rects()


    def read_layer_data(self):
        gds_path = self.gds_path
        lib = gdstk.read_gds(str(Path(gds_path)))
        out = defaultdict(lambda: {"polys": [], "labels": []})
        for cell in lib.top_level():
            polys = cell.polygons
            for p in polys:
                out[p.layer]["polys"].append(p)
            labels = cell.labels
            for lbl in labels:
                out[lbl.layer]["labels"].append((lbl.text, lbl.origin))

        self.layer_data = dict(out)
        
        return dict(out)


    def _poly_to_rects(self, poly):
        pts = np.asarray(poly.points)
        ys = np.unique(pts[:, 1])
        ys.sort()
        rects = []
        for y0, y1 in zip(ys[:-1], ys[1:]):
            midy = (y0 + y1) / 2.0
            xs = []
            n = len(pts)
            for i in range(n):
                x1, y1_ = pts[i]
                x2, y2_ = pts[(i + 1) % n]
                if (y1_ <= midy < y2_) or (y2_ <= midy < y1_):
                    if y1_ == y2_:
                        continue
                    t = (midy - y1_) / (y2_ - y1_)
                    xs.append(x1 + t * (x2 - x1))
            xs.sort()
            for x0, x1 in zip(xs[::2], xs[1::2]):
                rects.append(sbox(x0, y0, x1, y1))

        return rects


    def layer_rects(self):
        layer_data  = self.layer_data
        out         = defaultdict(list)

        for ly, d in layer_data.items():
            for poly in d["polys"]:
                out[ly].extend(self._poly_to_rects(poly))

        self.rect_data = dict(out)

        return dict(out)
        