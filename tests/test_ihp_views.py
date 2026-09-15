"""Independent electrical round trips for the standalone IHP view generator."""

from collections import Counter
from decimal import Decimal
from fractions import Fraction
from pathlib import Path
import hashlib
import re
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
CLI = ROOT / "tools" / "generate_ihp_views.py"
CELLS = ROOT / "outputs" / "sg13g2_cells" / "cells"
NAND = """.SUBCKT nand Y A B VDD VSS
*.PININFO A:I B:I Y:O VDD:B VSS:B
MP1 Y B VDD VDD sg13_lv_pmos m=1 w=4.48u l=130.00n ng=4
MP0 Y A VDD VDD sg13_lv_pmos m=1 w=4.48u l=130.00n ng=4
MN1 net1 B VSS VSS sg13_lv_nmos m=1 w=2.96u l=130.00n ng=4
MN0 Y A net1 VSS sg13_lv_nmos m=1 w=2.96u l=130.00n ng=4
.ENDS
"""

# Pin-box excerpts from the supplied official IHP symbols, in D/G/S/B order.
PIN_BOXES = {
    "sg13_lv_nmos.sym": (
        (17.5, -32.5, 22.5, -27.5), (-22.5, -2.5, -17.5, 2.5),
        (17.5, 27.5, 22.5, 32.5), (19.921875, -0.078125, 20.078125, 0.078125),
    ),
    "sg13_lv_pmos.sym": (
        (17.5, 27.5, 22.5, 32.5), (-22.5, -2.5, -17.5, 2.5),
        (17.5, -32.5, 22.5, -27.5), (19.921875, -0.078125, 20.078125, 0.078125),
    ),
}
PIN_BOXES.update({"au_medal_" + name: boxes for name, boxes in list(PIN_BOXES.items())})


def scalar(token):
    match = re.fullmatch(r"([+\-\d.eE]+)([unp]?)", token, re.I)
    if not match:
        raise AssertionError("Invalid numeric token: " + token)
    return Fraction(Decimal(match[1])) * {
        "": Fraction(1), "u": Fraction(1, 10**6),
        "n": Fraction(1, 10**9), "p": Fraction(1, 10**12),
    }[match[2].lower()]


def properties(text):
    return dict((k.lower(), v.strip('"')) for k, v in
                re.findall(r'(\w+)=("[^"]*"|[^\s}]+)', text))


def device_record(name, nets, model, attrs):
    return (name, tuple(nets), model, scalar(attrs["w"]), scalar(attrs["l"]),
            scalar(attrs["ng"]), scalar(attrs.get("m", "1")))


def read_netlist(path):
    text = re.sub(r"\r?\n\s*\+", " ", path.read_text(encoding="utf-8"))
    header = re.search(r"(?im)^\s*\.subckt\s+(\S+)\s+(.+)$", text)
    if header is None:
        raise AssertionError("Missing subcircuit")
    records = []
    for line in text.splitlines():
        if re.match(r"\s*[MX]\S+\s", line, re.I):
            fields = line.split()
            name = fields[0][1:] if fields[0][0].upper() == "X" else fields[0]
            records.append(device_record(name, fields[1:5], fields[5], properties(line)))
    if not records:
        raise AssertionError("Empty device graph")
    return header[1], header[2].split(), Counter(records)


def read_schematic(path, physical=False):
    wires, fets, labels, ports = [], [], [], []
    port_points = {}
    points = set()
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("N "):
            a = list(map(float, line.split()[1:5]))
            p, q = tuple(a[:2]), tuple(a[2:])
            if p[0] != q[0] and p[1] != q[1]:
                raise AssertionError("Non-orthogonal wire")
            wires.append((p, q))
            points.update((p, q))
        match = re.fullmatch(r"C \{([^}]+)\} (\S+) (\S+) (\d+) (\d+) \{(.*)\}", line)
        if match is None:
            continue
        sym, x, y, rot, flip, raw = match.groups()
        x, y = float(x), float(y)
        if rot != "0" or flip != "0":
            raise AssertionError("Unexpected symbol transform")
        attrs = properties(raw)
        if sym in PIN_BOXES:
            terminals = [(x + (a + c) / 2, y + (b + d) / 2)
                         for a, b, c, d in PIN_BOXES[sym]]
            points.update(terminals)
            fets.append((attrs, terminals))
        elif "lab" in attrs:
            points.add((x, y))
            labels.append(((x, y), attrs["lab"]))
            if sym in ("devices/ipin.sym", "devices/opin.sym", "devices/iopin.sym"):
                ports.append((attrs["lab"], {
                    "devices/ipin.sym": "I", "devices/opin.sym": "O",
                    "devices/iopin.sym": "B",
                }[sym]))
                port_points[attrs["lab"]] = (x, y)
        else:
            raise AssertionError("Unrecognised component: " + sym)
    parent = {p: p for p in points}

    def find(p):
        while parent[p] != p:
            parent[p] = parent[parent[p]]
            p = parent[p]
        return p

    def join(p, q):
        parent[find(p)] = find(q)

    for p, q in wires:
        for r in points:
            if (min(p[0], q[0]) <= r[0] <= max(p[0], q[0]) and
                    min(p[1], q[1]) <= r[1] <= max(p[1], q[1]) and
                    (p[0] == q[0] == r[0] or p[1] == q[1] == r[1])):
                join(p, r)
    if physical:
        return {"devices": {attrs["name"]: tuple(find(p) for p in terminals)
                            for attrs, terminals in fets},
                "ports": {name: find(p) for name, p in port_points.items()}}
    labelled = {}
    for p, name in labels:
        if name in labelled:
            join(p, labelled[name])
        labelled[name] = p
    names = {}
    for p, name in labels:
        root = find(p)
        if root in names and names[root] != name:
            raise AssertionError("Short between " + names[root] + " and " + name)
        names[root] = name
    records = []
    for attrs, terminals in fets:
        nets = [names.get(find(p), "FLOATING:" + str(find(p))) for p in terminals]
        records.append(device_record(attrs["name"], nets, attrs["model"], attrs))
    if not records:
        raise AssertionError("Empty schematic graph")
    return ports, Counter(records)


def symbol_ports(path):
    result = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("B 5 "):
            attrs = properties(line)
            result.append((attrs["name"], {"in": "I", "out": "O", "inout": "B"}[attrs["dir"]]))
    return result


class IHPViewsTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="ihp-views-test-")
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.source = self.base / "source" / "source.cdl"
        self.source.parent.mkdir()
        self.source.write_bytes(NAND.encode("ascii"))
        self.dest = self.base / "generated"

    def cli(self, source=None, output=None, *extra):
        return subprocess.run(
            [sys.executable, "-B", str(CLI), str(source or self.source),
             "--output-dir", str(output or self.dest), *extra],
            capture_output=True, text=True,
        )

    def successful(self, source=None, output=None):
        result = self.cli(source, output)
        self.assertEqual(result.returncode, 0, result.stderr)
        return result

    def compare_views(self, source, output):
        original = read_netlist(source)
        name, ports, graph = original
        self.assertEqual(read_netlist(output / (name + ".cdl")), original)
        self.assertEqual(read_netlist(output / (name + ".spi")), original)
        pininfo = dict(re.findall(r"(\w+):([IOB])", source.read_text(encoding="utf-8")))
        expected_ports = [(p, pininfo.get(p, "B")) for p in ports]
        sch_ports, sch_graph = read_schematic(output / (name + ".sch"))
        self.assertEqual(sch_ports, expected_ports)
        self.assertEqual(sch_graph, graph)
        self.assertEqual(symbol_ports(output / (name + ".sym")), expected_ports)
        self.assertEqual(len(graph), sum(graph.values()))

    def test_cli_emits_four_electrically_equal_views(self):
        self.successful()
        self.assertEqual({p.name for p in self.dest.iterdir()},
                         {"nand" + ext for ext in (".cdl", ".spi", ".sch", ".sym")})
        self.compare_views(self.source, self.dest)

    def test_multiplier_free_cdl_is_accepted_and_stays_multiplier_free(self):
        self.source.write_text(NAND.replace("m=1 ", ""), encoding="utf-8")
        self.successful()
        self.compare_views(self.source, self.dest)
        for path in self.dest.iterdir():
            self.assertNotRegex(path.read_text(), r"(?i)\bm\s*=|@m\b")

    def test_legacy_neutral_multiplier_is_consumed_but_never_authored(self):
        self.successful()
        self.compare_views(self.source, self.dest)
        for path in self.dest.iterdir():
            self.assertNotRegex(path.read_text(), r"(?i)\bm\s*=|@m\b")

    def test_schematic_embeds_one_no_multiplier_symbol_per_model(self):
        self.successful()
        text = (self.dest / "nand.sch").read_text()
        blocks = re.findall(r"(?ms)^C \{([^}]+)\}[^\n]*embed=true[^\n]*\n\[\n(.*?)^\]$", text)
        self.assertEqual(len(blocks), 2)
        self.assertEqual({name for name, _ in blocks},
                         {"au_medal_sg13_lv_nmos.sym", "au_medal_sg13_lv_pmos.sym"})
        for _, body in blocks:
            self.assertIn("IHP PDK Authors", body)
            self.assertIn("https://www.apache.org/licenses/LICENSE-2.0", body)
            self.assertIn("Modified", body)
            self.assertNotRegex(body, r"(?i)\bm\s*=|@m\b")
            self.assertIn("ng=@ng", body)
            self.assertIn("w=@w", body)
            self.assertIn("l=@l", body)

    def test_shipped_views_do_not_reintroduce_multiplier_parameters(self):
        views = [p for p in (ROOT / "outputs" / "sg13g2_cells").rglob("*")
                 if p.suffix in {".cdl", ".spi", ".sch"}]
        self.assertEqual(len(views), 91)
        for path in views:
            with self.subTest(view=str(path.relative_to(ROOT))):
                self.assertNotRegex(path.read_text(), r"(?i)\bm\s*=|@m\b")

    def test_cmos_drain_source_networks_are_wired_not_label_only(self):
        sources = sorted(CELLS.glob("*/*.cdl"))
        self.assertEqual(len(sources), 30)
        for source in sources:
            with self.subTest(cell=source.stem):
                target = self.base / source.stem
                self.successful(source, target)
                physical = read_schematic(target / (source.stem + ".sch"), physical=True)
                networks = {}
                for name, nets, *_ in read_netlist(source)[2]:
                    for terminal in (0, 2):
                        networks.setdefault(nets[terminal], set()).add(physical["devices"][name][terminal])
                for net, roots in networks.items():
                    self.assertEqual(len(roots), 1, (source.stem, net, roots))
                    if net in physical["ports"]:
                        self.assertEqual(roots, {physical["ports"][net]}, (source.stem, net))

    def test_buffer_stages_have_direct_gate_connections(self):
        source = CELLS / "sg13g2_buf_32" / "sg13g2_buf_32.cdl"
        self.successful(source)
        graph = read_schematic(self.dest / "sg13g2_buf_32.sch", physical=True)
        devs = graph["devices"]
        self.assertEqual(devs["MP0"][0], devs["MP1"][1])
        self.assertEqual(devs["MN0"][0], devs["MN1"][1])
        self.assertEqual(graph["ports"]["A"], devs["MP0"][1])
        self.assertEqual(graph["ports"]["A"], devs["MN0"][1])

    def test_wires_clear_official_attribute_anchor_band(self):
        # IHP symbol anchors: W(31.25,-26.25), L(31.25,-15),
        # ng(31.25,-2.5). This is a geometry guard,
        # not an assertion about native Xschem font bounding boxes.
        for source in sorted(CELLS.glob("*/*.cdl")):
            with self.subTest(cell=source.stem):
                target = self.base / source.stem
                self.successful(source, target)
                text = (target / (source.stem + ".sch")).read_text()
                wires = [tuple(map(float, line.split()[1:5]))
                         for line in text.splitlines() if line.startswith("N ")]
                for match in re.finditer(r"C \{au_medal_sg13_lv_[np]mos.sym\} (\S+) (\S+)", text):
                    x, y = map(float, match.groups())
                    for a, b, c, d in wires:
                        intersects = (max(a, c) >= x + 31.25 and min(a, c) <= x + 80 and
                                      max(b, d) >= y - 30 and min(b, d) <= y + 20)
                        self.assertFalse(intersects, (source.stem, x, y, (a, b, c, d)))

    def test_all_30_source_cdls_round_trip_and_outputs_stay_unchanged(self):
        sources = sorted(CELLS.glob("*/*.cdl"))
        self.assertEqual(len(sources), 30)
        existing = sorted((ROOT / "outputs").rglob("*"))
        before = {p: hashlib.sha256(p.read_bytes()).digest() for p in existing if p.is_file()}
        for source in sources:
            with self.subTest(cell=source.stem):
                output = self.base / source.stem
                self.successful(source, output)
                self.compare_views(source, output)
        after = {p: hashlib.sha256(p.read_bytes()).digest()
                 for p in (ROOT / "outputs").rglob("*") if p.is_file()}
        self.assertEqual(after, before)

    def test_independent_graph_parser_detects_wrong_gate_and_short(self):
        self.successful()
        sch = self.dest / "nand.sch"
        original = sch.read_text(encoding="utf-8")
        self.assertEqual(read_schematic(sch)[1], read_netlist(self.source)[2])
        bad = self.base / "wrong_gate.sch"
        bad.write_text(original.replace("lab=A}", "lab=BAD_GATE}"), encoding="utf-8")
        self.assertNotEqual(read_schematic(bad)[1], read_netlist(self.source)[2])
        match = re.search(r"C \{au_medal_sg13_lv_nmos.sym\} (\S+) (\S+)", original)
        x, y = map(float, match.groups())
        bad.write_text(original + f"N {x - 20:g} {y:g} {x + 20:g} {y:g} {{}}\n", encoding="utf-8")
        with self.assertRaisesRegex(AssertionError, "Short"):
            read_schematic(bad)

    def test_independent_graph_parser_detects_detached_device(self):
        self.successful()
        sch = self.dest / "nand.sch"
        bad = self.base / "detached.sch"
        text = sch.read_text(encoding="utf-8")
        text, count = re.subn(r"(C \{au_medal_sg13_lv_pmos.sym\} )\S+", r"\g<1>98760", text, count=1)
        self.assertEqual(count, 1)
        bad.write_text(text, encoding="utf-8")
        self.assertNotEqual(read_schematic(bad)[1], read_netlist(self.source)[2])

    def test_high_precision_width_and_length_are_exact(self):
        source = NAND.replace("4.48u", "4.48123456789012345678901234567890123456789e-6")
        source = source.replace("130.00n", "0.13000000000000000000000000000000000000001u")
        self.source.write_text(source, encoding="utf-8")
        self.successful()
        self.compare_views(self.source, self.dest)

    def test_mixed_units_and_continuations(self):
        source = NAND.replace("m=1 w=4.48u l=130.00n ng=4", "W=4480n L=0.13u\n+ NG=4 M=1.0")
        self.source.write_text(source, encoding="utf-8")
        self.successful()
        self.compare_views(self.source, self.dest)

    def test_missing_pininfo_uses_explicit_inout_ports(self):
        self.source.write_text("\n".join(l for l in NAND.splitlines() if not l.startswith("*.PININFO")) + "\n", encoding="utf-8")
        self.successful()
        self.compare_views(self.source, self.dest)

    def test_distinct_body_nets_and_drain_source_order(self):
        self.source.write_text(
            ".SUBCKT separate D0 G0 S0 B0 D1 G1 S1 B1\n"
            "MN D0 G0 S0 B0 sg13_lv_nmos w=1u l=130n ng=2 m=1\n"
            "MP D1 G1 S1 B1 sg13_lv_pmos w=2u l=130n ng=4 m=1\n"
            ".ENDS separate\n", encoding="utf-8")
        self.successful()
        self.compare_views(self.source, self.dest)

    def test_single_polarity_and_cycle_remain_electrically_exact(self):
        for model in ("sg13_lv_nmos", "sg13_lv_pmos"):
            with self.subTest(model=model):
                self.source.write_text(
                    ".SUBCKT cycle A B C BODY\n"
                    f"M1 A C B BODY {model} w=1u l=130n ng=1 m=1\n"
                    f"M2 B C A BODY {model} w=1u l=130n ng=1 m=1\n"
                    ".ENDS cycle\n", encoding="utf-8")
                output = self.base / model
                self.successful(output=output)
                self.compare_views(self.source, output)

    def test_crlf_source_is_unchanged_and_generated_views_use_crlf(self):
        original = NAND.replace("\n", "\r\n").encode("ascii")
        self.source.write_bytes(original)
        self.successful()
        self.assertEqual(self.source.read_bytes(), original)
        for path in self.dest.iterdir():
            data = path.read_bytes()
            self.assertIn(b"\r\n", data)
            self.assertNotIn(b"\n", data.replace(b"\r\n", b""))

    def test_invalid_or_ambiguous_inputs_fail_before_any_output(self):
        variants = {
            "multiplier": NAND.replace("m=1", "m=2", 1),
            "missing_ng": NAND.replace(" ng=4", "", 1),
            "per_finger": NAND.replace("ng=4", "nf=4", 1),
            "expression": NAND.replace("w=4.48u", "w={2.24u*2}", 1),
            "unknown_parameter": NAND.replace("ng=4", "ng=4 nfin=1000", 1),
            "duplicate_parameter": NAND.replace("ng=4", "ng=4 W=1u", 1),
            "zero_width": NAND.replace("w=4.48u", "w=0", 1),
            "negative_length": NAND.replace("l=130.00n", "l=-130n", 1),
            "nonfinite": NAND.replace("w=4.48u", "w=NaN", 1),
            "invalid_exponent": NAND.replace("w=4.48u", "w=1e99999999999999999999999", 1),
            "ambiguous_units": NAND.replace("w=4.48u", "w=4.48um", 1),
            "fractional_fingers": NAND.replace("ng=4", "ng=1.5", 1),
            "zero_fingers": NAND.replace("ng=4", "ng=0", 1),
            "unsupported_model": NAND.replace("sg13_lv_pmos", "other_pmos", 1),
            "extra_device": NAND.replace(".ENDS", "R1 A B 1k\n.ENDS"),
            "hierarchy": NAND.replace(".ENDS", "X1 Y A VDD VSS child\n.ENDS"),
            "directive": ".option scale=1u\n" + NAND,
            "missing_end": NAND.replace(".ENDS", ""),
            "wrong_end": NAND.replace(".ENDS", ".ENDS other"),
            "duplicate_subcircuit": NAND + NAND,
            "duplicate_device": NAND.replace("MP0", "MP1"),
            "duplicate_port": NAND.replace("Y A B VDD VSS", "Y A A VDD VSS", 1),
            "incomplete_pininfo": NAND.replace(" B:I", ""),
            "bad_pininfo": NAND.replace("B:I", "TYPO:I"),
            "pininfo_direction": NAND.replace("B:I", "B:Z"),
            "unsafe_name": NAND.replace(".SUBCKT nand", ".SUBCKT ../nand"),
            "unsafe_net": NAND.replace("net1", "net[1]"),
            "empty": ".SUBCKT empty A Y\n.ENDS\n",
            "orphan_continuation": "+ w=1u\n" + NAND,
            "trailing_text": NAND + "UNRECOGNISED\n",
        }
        for label, text in variants.items():
            with self.subTest(case=label):
                self.source.write_text(text, encoding="utf-8")
                result = self.cli()
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("error:", result.stderr.lower())
                self.assertNotIn("Traceback", result.stderr)
                self.assertFalse(self.dest.exists())

    def test_no_overwrite_and_no_partial_write_on_collision(self):
        self.dest.mkdir()
        sentinel = self.dest / "nand.sym"
        sentinel.write_bytes(b"keep this existing output\r\n")
        result = self.cli()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("exist", result.stderr.lower())
        self.assertEqual(list(self.dest.iterdir()), [sentinel])
        self.assertEqual(sentinel.read_bytes(), b"keep this existing output\r\n")

    def test_source_directory_is_rejected(self):
        result = self.cli(output=self.source.parent)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("separate", result.stderr.lower())
        self.assertEqual(list(self.source.parent.iterdir()), [self.source])

    def test_cli_rejects_missing_source_unknown_and_abbreviated_options(self):
        for extra in ("--typo", "--out", "--OUTPUT-DIR"):
            result = self.cli(None, None, extra, "x")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("unrecognized", result.stderr.lower())
        result = self.cli(source=self.base / "absent.cdl")
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(self.dest.exists())

    def test_reproducible_bytes(self):
        self.successful()
        other = self.base / "again"
        self.successful(output=other)
        self.assertEqual({p.name: p.read_bytes() for p in self.dest.iterdir()},
                         {p.name: p.read_bytes() for p in other.iterdir()})


if __name__ == "__main__":
    unittest.main()
