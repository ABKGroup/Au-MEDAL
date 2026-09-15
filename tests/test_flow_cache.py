"""Run the production flow.cpp CLI with real config/placement parsing and test boundaries.

The stub tests cache identity and failure handling, not GDS geometry or SMT correctness.
AUMEDAL_FLOW optionally adds the original bogus-GDS probe against a real compiled flow.
All inputs, outputs and executables are confined to a temporary directory.
"""

import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class FlowCache(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.build = tempfile.TemporaryDirectory(prefix="aumedal-flow-cache-build-")
        cls.addClassCleanup(cls.build.cleanup)
        cls.binary = Path(cls.build.name) / "flow-cache-test"
        command = [os.environ.get("CXX", "g++"), "-std=c++17", "-O0",
                   "-I", str(ROOT / "src/include"), str(ROOT / "tests/flow_cache_stub.cpp"),
                   str(ROOT / "src/config.cpp"), str(ROOT / "src/circuit.cpp"),
                   "-o", str(cls.binary)]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="aumedal-flow-cache-")
        self.addCleanup(self.temp.cleanup)
        self.work = Path(self.temp.name)
        self.cfg = self.work / "config.json"
        self.placement = self.work / "placement.json"
        self.config = {
            "design_rules": {"width": {"fin": 1, "Gate": 10, "M1": 10},
                             "spacing": {"S2S": {"fin": {"fin": 0}, "Gate": {"Gate": 10},
                                                 "M1": {"M1": 10}}}},
            "design_specs": {"cell_height": 1000, "gate_contact_layer": "M1",
                             "active_contact_layer": "M1", "ext_pin_layer": ["M1"],
                             "power_layer": ["M1"], "power_width": {"M1": 10},
                             "max_fins": {"nmos": 100, "pmos": 100}, "diffusion_break": 0,
                             "routing_layers": [{"layer_name": "M1", "direction": "H"}],
                             "vias": []},
            "design_options": {"active_y_offset": 10},
        }
        self.design = {"columns": [{"nmos": {"name": "n", "fin": 10,
                                              "nets": ["Y", "A", "VSS"]},
                                    "pmos": {"name": "p", "fin": 10,
                                              "nets": ["Y", "A", "VDD"]}}]}
        self.write_inputs()
        self.target = self.work / "out" / "cell"
        self.gds = self.target / "cell.gds"
        self.cache = self.target / "cell.routing.json"

    def write_inputs(self):
        self.cfg.write_text(json.dumps(self.config))
        self.placement.write_text(json.dumps(self.design))

    def run_flow(self, *flags, binary=None, fail_writer=False, status=None):
        env = os.environ.copy()
        env.pop("AUMEDAL_TEST_FAIL_WRITER", None)
        env.pop("AUMEDAL_TEST_STATUS", None)
        if status:
            env["AUMEDAL_TEST_STATUS"] = status
        if fail_writer:
            env["AUMEDAL_TEST_FAIL_WRITER"] = str(fail_writer)
        return subprocess.run(
            [str(binary or self.binary), "--save_dir", str(self.work / "out"),
             "--cell_name", "cell", "--config", str(self.cfg),
             "--placement_file", str(self.placement), "--ports", "A,Y,VDD,VSS", *flags],
            capture_output=True, text=True, env=env,
        )

    def assert_solved(self, result):
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("[TEST] solve_router", result.stdout)
        self.assertNotIn("SAT (cached)", result.stdout)

    def seed(self):
        self.assert_solved(self.run_flow())
        self.assertTrue(self.gds.is_file())
        self.assertTrue(self.cache.is_file())

    def test_happy_cache_hit_and_no_cache(self):
        self.seed()
        before = self.gds.read_bytes()
        result = self.run_flow()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("SAT (cached)", result.stdout)
        self.assertNotIn("[TEST] solve_router", result.stdout)
        self.assertNotIn("[TEST] write_routing_gds", result.stdout)
        self.assertEqual(self.gds.read_bytes(), before)
        self.assert_solved(self.run_flow("--no_cache"))

    def test_bogus_gds_missing_inputs(self):
        self.target.mkdir(parents=True)
        self.gds.write_bytes(b"not a GDS file")
        self.cfg.unlink()
        self.placement.unlink()
        for flags in [(), ("--no_cache",)]:
            with self.subTest(flags=flags):
                result = self.run_flow(*flags)
                self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                self.assertNotIn("SAT (cached)", result.stdout)

    def test_bogus_gds_with_valid_inputs_is_not_a_hit(self):
        self.target.mkdir(parents=True)
        self.gds.write_bytes(b"bogus GDS")
        self.assert_solved(self.run_flow())

    def test_modified_config_invalidates_hit(self):
        self.seed()
        self.config["design_rules"]["width"]["M1"] = 11
        self.write_inputs()
        self.assert_solved(self.run_flow())

    def test_modified_placement_invalidates_hit(self):
        self.seed()
        stamp = self.placement.stat()
        self.design["columns"][0]["nmos"]["fin"] = 11
        self.write_inputs()
        os.utime(self.placement, ns=(stamp.st_atime_ns, stamp.st_mtime_ns))
        self.assert_solved(self.run_flow())

    def test_modified_ports_invalidates_hit(self):
        self.seed()
        self.assert_solved(self.run_flow("--ports", "A,Y"))

    def test_invalid_port_is_not_hidden_by_cache(self):
        self.seed()
        result = self.run_flow("--ports", "absent")
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)

    def test_modified_gds_invalidates_hit(self):
        self.seed()
        data = bytearray(self.gds.read_bytes())
        data[-2] ^= 1
        self.gds.write_bytes(data)
        self.assert_solved(self.run_flow())

    def test_modified_routing_invalidates_hit(self):
        self.seed()
        cached = json.loads(self.cache.read_text())
        cached["tolerance"] = 99
        self.cache.write_text(json.dumps(cached))
        self.assert_solved(self.run_flow())

    def test_corrupt_cache_invalidates_hit(self):
        self.seed()
        self.cache.write_text("{broken")
        self.assert_solved(self.run_flow())

    def test_producer_identity_invalidates_hit(self):
        self.seed()
        other = self.work / "other-flow"
        shutil.copy2(self.binary, other)
        with other.open("ab") as out:
            out.write(b"different producer identity")
        self.assert_solved(self.run_flow(binary=other))

    def test_explicit_regeneration_and_missing_gds(self):
        self.seed()
        expected = self.gds.read_bytes()
        self.gds.unlink()
        result = self.run_flow("--regen_gds_only")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("[TEST] solve_router", result.stdout)
        self.assertIn("[TEST] write_routing_gds", result.stdout)
        self.assertEqual(self.gds.read_bytes(), expected)

    def test_regeneration_allows_new_emitter_but_does_not_recertify_solver(self):
        self.seed()
        other = self.work / "rebuilt-flow"
        shutil.copy2(self.binary, other)
        with other.open("ab") as out:
            out.write(b"rebuilt emitter")
        result = self.run_flow("--regen_gds_only", binary=other)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("[TEST] solve_router", result.stdout)
        self.assert_solved(self.run_flow(binary=other))

    def test_regeneration_rejects_changed_config(self):
        self.seed()
        self.config["design_rules"]["width"]["M1"] = 11
        self.write_inputs()
        self.assert_regen_rejected()

    def test_regeneration_rejects_changed_placement(self):
        self.seed()
        self.design["columns"][0]["nmos"]["nets"][0] = "A"
        self.write_inputs()
        self.assert_regen_rejected()

    def test_regeneration_rejects_changed_ports(self):
        self.seed()
        self.assert_regen_rejected("--ports", "A,Y")

    def assert_regen_rejected(self, *flags):
        before = self.gds.read_bytes()
        result = self.run_flow("--regen_gds_only", *flags)
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
        self.assertNotIn("[TEST] write_routing_gds", result.stdout)
        self.assertNotIn("[TEST] solve_router", result.stdout)
        self.assertEqual(self.gds.read_bytes(), before)

    def test_regeneration_rejects_legacy_unvalidated_routing(self):
        self.target.mkdir(parents=True)
        self.gds.write_bytes(b"bogus GDS")
        self.cache.write_text('{"sat":true,"top_layer":"M1","tolerance":0}')
        self.assert_regen_rejected()

    def test_regeneration_rejects_tampered_routing(self):
        self.seed()
        cached = json.loads(self.cache.read_text())
        cached["tolerance"] = 99
        self.cache.write_text(json.dumps(cached))
        self.assert_regen_rejected()

    def certify_unresolved_geometry(self):
        cached = json.loads(self.cache.read_text())
        metadata = cached.pop("_cache")
        cached["geometry_unresolved"] = True
        payload = json.dumps(cached, sort_keys=True, separators=(",", ":")).encode()
        fingerprint = 14695981039346656037
        for byte in payload:
            fingerprint = ((fingerprint ^ byte) * 1099511628211) & ((1 << 64) - 1)
        metadata["routing"] = f"fnv1a64:{fingerprint:x}:{len(payload)}"
        cached["_cache"] = metadata
        self.cache.write_text(json.dumps(cached))

    def test_geometry_unresolved_cannot_be_cached_sat(self):
        self.seed()
        self.certify_unresolved_geometry()
        self.assert_solved(self.run_flow())

    def test_geometry_unresolved_cannot_be_regenerated_as_sat(self):
        self.seed()
        self.certify_unresolved_geometry()
        self.assert_regen_rejected()

    def test_failed_writer_cannot_certify_routing_or_gds(self):
        result = self.run_flow(fail_writer=True)
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
        result = self.run_flow("--regen_gds_only")
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
        self.assert_solved(self.run_flow())

    def test_failed_rewrite_invalidates_old_cache(self):
        self.seed()
        result = self.run_flow("--no_cache", fail_writer=True)
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
        self.assert_solved(self.run_flow())

    def test_failed_regeneration_preserves_route_for_retry(self):
        self.seed()
        route_before = json.loads(self.cache.read_text())
        gds_before = self.gds.read_bytes()
        result = self.run_flow("--regen_gds_only", fail_writer="before_open")
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
        self.assertEqual(self.gds.read_bytes(), gds_before)
        self.assertTrue(self.cache.is_file(), "completed routing must survive writer failure")
        route_after = json.loads(self.cache.read_text())
        for key in ["inputs", "solver", "routing"]:
            self.assertEqual(route_after["_cache"][key], route_before["_cache"][key])
        self.assertEqual({k: v for k, v in route_after.items() if k != "_cache"},
                         {k: v for k, v in route_before.items() if k != "_cache"})
        result = self.run_flow(fail_writer=True)
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
        self.assertIn("[TEST] solve_router", result.stdout)
        self.assertNotIn("SAT (cached)", result.stdout)
        result = self.run_flow("--regen_gds_only")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("[TEST] solve_router", result.stdout)
        self.assertEqual(self.gds.read_bytes(), gds_before)

    def test_failed_new_inputs_preserve_prior_routing_identity(self):
        self.seed()
        self.config["design_rules"]["width"]["M1"] = 11
        self.write_inputs()
        result = self.run_flow(fail_writer=True)
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
        self.assertTrue(self.cache.is_file())
        self.assert_regen_rejected()
        self.config["design_rules"]["width"]["M1"] = 10
        self.write_inputs()
        result = self.run_flow("--regen_gds_only")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("[TEST] solve_router", result.stdout)

    def test_unsat_marker_blocks_hit(self):
        self.seed()
        (self.target / "ROUTING_UNSAT.txt").touch()
        self.assert_solved(self.run_flow())

    def test_undecided_and_timeout_are_distinct(self):
        for status, label in [("geometry", "UNDECIDED"), ("timeout", "TIMEOUT")]:
            with self.subTest(status=status):
                result = self.run_flow(status=status)
                self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
                self.assertIn("- status: " + label, result.stdout)
                self.assertFalse((self.target / "ROUTING_UNSAT.txt").exists())
                self.assertFalse(self.cache.exists())
                self.assertFalse(self.gds.exists())
                if status == "geometry":
                    self.assertNotIn("TIMEOUT", result.stdout)
                    self.assertIn("geometry", result.stdout.lower())

    def test_unsat_is_still_unsat(self):
        result = self.run_flow(status="unsat")
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("- status: UNSAT", result.stdout)
        self.assertTrue((self.target / "ROUTING_UNSAT.txt").exists())
        self.assertFalse(self.cache.exists())

    def test_old_unsat_marker_removed_for_undecided(self):
        self.target.mkdir(parents=True)
        marker = self.target / "ROUTING_UNSAT.txt"
        for status in ["geometry", "timeout"]:
            with self.subTest(status=status):
                marker.touch()
                result = self.run_flow(status=status)
                self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
                self.assertFalse(marker.exists())

    def test_equivalent_formatting_and_port_order_keep_hit(self):
        self.seed()
        self.cfg.write_text(json.dumps(self.config, indent=4))
        self.placement.write_text(json.dumps(self.design, indent=4))
        result = self.run_flow("--ports", " VSS, Y, A,VDD")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("SAT (cached)", result.stdout)

    def test_cache_publish_failure_is_not_sat(self):
        self.target.mkdir(parents=True)
        self.cache.with_suffix(".json.tmp").mkdir()
        result = self.run_flow()
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
        self.assertNotIn("- status: SAT", result.stdout)
        self.assertFalse(self.cache.exists())

    @unittest.skipUnless(Path("/dev/full").exists(), "requires /dev/full")
    def test_writer_stream_error_is_not_sat(self):
        self.target.mkdir(parents=True)
        self.gds.symlink_to("/dev/full")
        result = self.run_flow("--no_cache")
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
        self.assertNotIn("- status: SAT", result.stdout)
        self.assertFalse(self.cache.exists())

    @unittest.skipUnless(os.environ.get("AUMEDAL_FLOW"), "AUMEDAL_FLOW not set")
    def test_real_flow_bogus_gds_missing_inputs(self):
        self.target.mkdir(parents=True)
        self.gds.write_bytes(b"not a GDS file")
        self.cfg.unlink()
        self.placement.unlink()
        result = self.run_flow(binary=os.environ["AUMEDAL_FLOW"])
        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
        self.assertNotIn("SAT (cached)", result.stdout)


if __name__ == "__main__":
    unittest.main()
