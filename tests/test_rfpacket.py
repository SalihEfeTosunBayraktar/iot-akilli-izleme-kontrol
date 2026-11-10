"""Checks the 32-bit sensor packet layout used by every node (libraries/RfPacket/src/RfPacket.h).

The C++ header is mirrored here because the PC has no host C++ compiler; the bit positions and
the checksum must stay identical to the header. Run: python -m unittest discover -s tests
"""
import itertools
import re
import unittest
from pathlib import Path

HEADER = Path(__file__).resolve().parents[1] / "libraries" / "RfPacket" / "src" / "RfPacket.h"
MAGIC = 0b101


def checksum(body):
    result = 0
    for i in range(7):
        result ^= (body >> (i * 4)) & 0x0F
    return result


def encode(node, motion, gas, temp, hum):
    body = (MAGIC << 25) | ((node & 3) << 23) | (int(motion) << 22) | (gas << 14) | ((temp & 0x7F) << 7) | (hum & 0x7F)
    return ((body << 4) | checksum(body)) & 0xFFFFFFFF


def decode(raw):
    body = raw >> 4
    if (raw & 0x0F) != checksum(body) or (body >> 25) != MAGIC:
        return None
    return (body >> 23) & 3, bool((body >> 22) & 1), (body >> 14) & 0xFF, (body >> 7) & 0x7F, body & 0x7F


class RfPacketTest(unittest.TestCase):
    def test_header_matches_mirrored_layout(self):
        text = HEADER.read_text(encoding="utf-8")
        for fragment in ("kMagic = 0b101", "<< 25", "<< 23", "<< 22", "<< 14", "<< 7", "(body << 4) | checksum(body)"):
            self.assertIn(fragment, text)
        self.assertIsNotNone(re.search(r"for \(uint8_t i = 0; i < 7; \+\+i\)", text))

    def test_round_trip_all_field_ranges(self):
        count = 0
        for node, motion, gas, temp, hum in itertools.product(range(4), (0, 1), range(0, 256, 17), range(0, 128, 9), range(0, 101, 7)):
            raw = encode(node, motion, gas, temp, hum)
            self.assertEqual(raw >> 31, 1)  # magic keeps the top bit set, so the frame is always 32 bits
            self.assertEqual(decode(raw), (node, bool(motion), gas, temp, hum))
            count += 1
        self.assertEqual(count, 28800)

    def test_every_single_bit_flip_is_rejected(self):
        raw = encode(0, True, 120, 24, 45)
        for bit in range(32):
            self.assertIsNone(decode(raw ^ (1 << bit)), f"bit {bit} flip accepted")


if __name__ == "__main__":
    unittest.main()
