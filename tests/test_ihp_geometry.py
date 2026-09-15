"""Compile and run the IHP geometry regressions without installing dependencies."""

import os
import json
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

from layout_fixture import contains, covered, fixture, grid_hit, read_gds, rectangles


ROOT = Path(__file__).resolve().parents[1]


class IhpGeometry(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.build = tempfile.TemporaryDirectory(prefix="aumedal-ihp-geometry-")
        cls.addClassCleanup(cls.build.cleanup)
        cls.binary = Path(cls.build.name) / "geometry-test"
        command = [
            os.environ.get("CXX", "g++"), "-std=c++17", "-O0",
            "-I", str(ROOT / "src/include"),
            str(ROOT / "tests/test_ihp_geometry.cpp"),
            str(ROOT / "src/config.cpp"), str(ROOT / "src/prelayout.cpp"),
            "-o", str(cls.binary),
        ]
        result = subprocess.run(command, text=True, capture_output=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)

    def test_geometry_and_writer(self):
        result = subprocess.run([str(self.binary)], cwd=ROOT, text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertRegex(result.stdout, r"TESTS [1-9][0-9]* FAILED 0")

    def test_feedback_on_all_retained_layouts(self):
        cases = []
        for folder in sorted((ROOT / "outputs/sg13g2_cells/cells").iterdir()):
            cdl = (folder / (folder.name + ".cdl")).read_text()
            ports = re.search(r"(?im)^\.subckt\s+\S+\s+(.+)", cdl)[1].split()
            cases.append(fixture(folder / (folder.name + ".gds"), ports))
        self.assertEqual(len(cases), 30)
        input_path = Path(self.build.name) / "layouts.json"
        output_path = Path(self.build.name) / "feedback.json"
        input_path.write_text(json.dumps(cases))
        result = subprocess.run([str(self.binary), str(input_path), str(output_path)],
                                cwd=ROOT, text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        actual = json.loads(output_path.read_text())
        self.assertEqual([c["name"] for c in actual], [c["name"] for c in cases])
        for before, after in zip(cases, actual):
            with self.subTest(cell=before["name"]):
                self.assertEqual(len(after["pins"]), len(before["ports"]))
                self.assertEqual(after["pins"][0], [0, -220, before["width_nm"], 220])
                self.assertEqual(after["pins"][1], [0, 3560, before["width_nm"], 4000])
                metal = after["rects"]["8"]
                for pin in after["pins"]:
                    self.assertTrue(covered(pin, metal), pin)
                labels = [l for l in after["labels"] if l["layer"] == 8]
                self.assertEqual(sorted(l["text"] for l in labels), sorted(before["ports"]))
                for label in labels:
                    pins = [p for p in after["pins"] if contains(p, label["x"], label["y"])]
                    self.assertTrue(pins, label)
                    if label["text"] not in ("VDD", "VSS"):
                        self.assertTrue(any(grid_hit(p) for p in pins), label)
                for cut in after["rects"]["6"]:
                    left, bottom, right, top = cut
                    self.assertTrue(covered([left-50, bottom-50, right+50, top+50], metal), cut)

    def test_fixture_checker_negative_controls(self):
        square = [(0, 0), (100, 0), (100, 100), (0, 100), (0, 0)]
        self.assertTrue(covered([0, 0, 100, 100], rectangles(square)))
        self.assertFalse(covered([-1, 0, 100, 100], rectangles(square)))
        self.assertTrue(grid_hit([400, 800, 560, 960]))
        self.assertFalse(grid_hit([101, 101, 261, 261]))
        path = next((ROOT / "outputs/sg13g2_cells/cells").glob("*/*.gds"))
        bad = Path(self.build.name) / "truncated.gds"
        bad.write_bytes(path.read_bytes()[:-1])
        with self.assertRaises(ValueError):
            read_gds(bad)


if __name__ == "__main__":
    unittest.main()
