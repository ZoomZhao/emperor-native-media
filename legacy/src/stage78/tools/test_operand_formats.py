import sys,unittest
sys.dont_write_bytecode=True
from operand_formats import decode,Unsupported
class FormatTests(unittest.TestCase):
 def test_absolute_vs_indexed(self):
  self.assertEqual(decode(bytes.fromhex('8b0520407000'))['fields'][0]['kind'],'absolute_displacement')
  self.assertEqual(decode(bytes.fromhex('8b842120407000'))['fields'][0]['kind'],'indexed_displacement')
 def test_segment(self):
  d=decode(bytes.fromhex('64a100000000'));self.assertEqual(d['operands'][0]['segment'],0x64);self.assertEqual(d['end'],6)
 def test_typed(self):
  for b,n in [('6683f808',4),('0fb74102',4),('f3a5',2),('f2ae',2),('f7d0',2),('6bc012',3),('a904000000',5),('3a442403',4)]:self.assertEqual(decode(bytes.fromhex(b))['end'],n)
 def test_signed_relative(self):self.assertEqual(decode(bytes.fromhex('ebfe'),0x100)['branch_target'],0x100)
 def test_unknown(self):
  for b in ['','0fff','f3c3','678b00','f08b00','c7c8']:
   with self.assertRaises(Unsupported):decode(bytes.fromhex(b))
 def test_truncation(self):
  for b in ['8b84','b800','6681f801','64a10000']:
   with self.assertRaises(Unsupported):decode(bytes.fromhex(b))
if __name__=='__main__':unittest.main()
