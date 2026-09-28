#!/usr/bin/env python3
"""Known pulses and adversarial incomplete/misbound captures, no hardware."""
import unittest
from score_p601_loopback import score_bits, score_capture, synthesise

class ScorerTests(unittest.TestCase):
    def capture(self):
        return dict(bits=[0]*8+([1]*2+[0]*6)*8+[0]*40,
                    expected_cells=8, payload_hex="00", generation=3,
                    requested_generation=3, sample_period_counts=10,
                    period_counts=80, t0h_counts=20, t1h_counts=60,
                    reset_counts=300, build="fixture", uid="fixture",
                    truncated=False, uniform_sampling_qualified=True)
    def test_full_trace(self):
        self.assertEqual(score_capture(self.capture())["verdict"],"PASS")
    def test_lone_high_and_truncated(self):
        for b in ([1],[],[0,1],[1,0],[0]):
            self.assertTrue(score_bits(b,1)["mismatch"])
    def test_extra_edges(self):
        for a,b in ((True,False),(False,True)):
            self.assertTrue(score_bits(synthesise(8,a,b),8)["mismatch"])
    def test_missing_identity_and_metadata(self):
        for key in self.capture():
            c=self.capture();del c[key]
            self.assertNotEqual(score_capture(c)["verdict"],"PASS",key)
    def test_wrong_generation_reset_payload_and_period(self):
        for key,value,verdict in (("generation",4,"INVALID"),("reset_counts",10000,"INVALID"),
                                  ("payload_hex","ff","FAIL"),("period_counts",120,"FAIL"),
                                  ("uniform_sampling_qualified",False,"INVALID"),("truncated",True,"INVALID")):
            c=self.capture();c[key]=value
            self.assertEqual(score_capture(c)["verdict"],verdict,key)
    def test_ambiguous_width(self):
        c=self.capture();c["t1h_counts"]=25
        self.assertEqual(score_capture(c)["verdict"],"INVALID")

if __name__ == "__main__":unittest.main()
