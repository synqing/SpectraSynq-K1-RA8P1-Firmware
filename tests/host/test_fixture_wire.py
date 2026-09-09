import sys
from pathlib import Path
import struct
import unittest
sys.path.insert(0,str(Path(__file__).parents[2]/'scripts'))
from fixture_wire import decode, read_schema


class CompactWireTests(unittest.TestCase):
    def test_exact_types_and_uint64(self):
        body=b'f'+struct.pack('<f',.5)+b'8'+struct.pack('<Q',2**64-1)
        self.assertEqual(decode(body,[('float','f'),('epoch','i')]),b'float=0.5\nepoch=18446744073709551615\n')

    def test_all_integer_widths(self):
        for width in (1,2,3,4,8):
            value=2**(8*width)-1
            self.assertEqual(decode(str(width).encode()+value.to_bytes(width,'little'),[('x','i')]),f'x={value}\n'.encode())

    def test_mutants_rejected(self):
        for body in (b'',b'f\0',b'1\0',b'f'+struct.pack('<f',float('nan')),b'f'+struct.pack('<f',1)+b'1\0'):
            with self.assertRaises(ValueError): decode(body,[('x','f')])
        with self.assertRaises(ValueError): read_schema('pixel[0]=i\nEND\n')


if __name__=='__main__': unittest.main()
