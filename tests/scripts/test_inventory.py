"""Unit checks for corpus inventory companion matching."""

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from inventory import find_export


class InventoryCompanionTests(unittest.TestCase):
    def test_only_mus_is_treated_as_a_source_suffix(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            companion_dir = root / "-finale27"
            companion_dir.mkdir()

            dotted_source = root / "Score.96:09:20"
            (companion_dir / "Score.musx").touch()
            self.assertEqual(find_export(dotted_source, "-finale27", ".musx"), (None, "missing"))

            dotted_companion = companion_dir / "Score.96:09:20.musx"
            dotted_companion.touch()
            self.assertEqual(
                find_export(dotted_source, "-finale27", ".musx"),
                (dotted_companion, "adjacent-exact"),
            )

            mus_source = root / "Score.MUS"
            self.assertEqual(
                find_export(mus_source, "-finale27", ".musx"),
                (companion_dir / "Score.musx", "adjacent-exact"),
            )


if __name__ == "__main__":
    unittest.main()
