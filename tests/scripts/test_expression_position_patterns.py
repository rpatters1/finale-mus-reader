"""Unit checks for the exploratory selector census (no corpus required)."""
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from expression_position_patterns import combinations, records


class PositionPatternsTests(unittest.TestCase):
    def test_fixed_rows(self):
        dump = "epoch=2 byteOrder=big\n" + "\n".join(
            f"DT cmper=7 cmper2=0 inci={i} words=[{' '.join(map(str, range(i * 6, i * 6 + 6)))}] bytes=00"
            for i in range(3))
        self.assertEqual(records(dump), {7: list(range(18))})
        with self.assertRaises(ValueError):
            records(dump.replace("inci=1", "inci=4"))

    def test_variable_byte_orders(self):
        words = list(range(17)) + [-72]
        for name, prefix in (("big", ">"), ("little", "<")):
            payload = struct.pack(prefix + "18h", *words).hex()
            dump = f"epoch=3 byteOrder={name}\nDT cmper=7 cmper2=0 inci=0 words=[] bytes={payload}"
            self.assertEqual(records(dump), {7: words})
            with self.assertRaises(ValueError):
                records(dump[:-4])

    def test_missing_combinations_and_conflicts(self):
        observations = [{"corpus_id": "mus-example", "cmper": i,
                         "words": [0] * 18, "x": value}
                        for i, value in enumerate(("manual", "stem"))]
        rows = combinations(observations, "x")
        self.assertEqual(len(rows), 56)
        self.assertTrue(rows[0]["conflict"])
        self.assertEqual(rows[0]["distinct_sources"], 1)
        self.assertEqual(rows[0]["definitions"], 2)
        self.assertEqual(sum(r["definitions"] == 0 for r in rows), 55)


if __name__ == "__main__":
    unittest.main()
