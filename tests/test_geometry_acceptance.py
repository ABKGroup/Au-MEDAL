"""Exercise the production post-search acceptance block, without an SMT run."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class GeometryAcceptance(unittest.TestCase):
    def test_actual_acceptance_block(self):
        source = (ROOT / "src/solve.cpp").read_text()
        start = source.index("        if (r.undecided) any_undecided = true;")
        stop_text = "        if (r.sat) return r;"
        stop = source.index(stop_text, start) + len(stop_text)
        block = source[start:stop]
        program = r'''
#include <iostream>
#include <limits>
#include <string>
struct RoutingResult {
    bool sat = false;
    bool undecided = false;
    std::string top_layer;
    bool geometry_unresolved = false;
};
RoutingResult accept(RoutingResult r, const RoutingResult& best_r,
                     size_t best_total, bool have_best, bool any_undecided,
                     bool any_geometry_unresolved = false) {
    const std::string top = "M1";
BLOCK
    return r;
}
int main() {
    const RoutingResult sat{true, false, ""};
    const RoutingResult unsat{false, false, ""};
    const RoutingResult unknown{false, true, ""};
    int errors = 0;
    auto check = [&](const char* name, RoutingResult r, bool want_sat, bool want_unknown) {
        if (r.sat != want_sat || r.undecided != want_unknown) {
            std::cerr << name << ": sat=" << r.sat << " undecided=" << r.undecided << "\n";
            ++errors;
        }
    };
    check("zero conflicts", accept(sat, sat, 0, true, false), true, false);
    check("iteration limit with conflicts", accept(sat, sat, 1, true, false), false, true);
    check("blocked search with conflicting best", accept(unsat, sat, 2, true, false), false, true);
    check("timeout after conflicting best", accept(unknown, sat, 3, true, false), false, true);
    check("proved infeasible without best", accept(unsat, unsat, 99, false, false), false, false);
    check("unknown without best", accept(unknown, unsat, 99, false, false), false, true);
    check("earlier unknown with clean best", accept(sat, sat, 0, true, true), true, true);
    if (!accept(sat, sat, 1, true, false).geometry_unresolved) ++errors;
    if (!accept(unsat, unsat, 99, false, true, true).geometry_unresolved) ++errors;
    if (accept(sat, sat, 0, true, true, true).geometry_unresolved) ++errors;
    return errors ? 1 : 0;
}
'''.replace("BLOCK", block)
        with tempfile.TemporaryDirectory(prefix="aumedal-acceptance-") as name:
            work = Path(name)
            cpp = work / "acceptance.cpp"
            exe = work / "acceptance"
            cpp.write_text(program)
            compiled = subprocess.run(
                [os.environ.get("CXX", "g++"), "-std=c++17", str(cpp), "-o", str(exe)],
                text=True, capture_output=True,
            )
            self.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
            result = subprocess.run([str(exe)], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
