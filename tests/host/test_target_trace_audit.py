import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('audit', Path(__file__).parents[2]/'scripts/audit_target_trace.py')
audit = importlib.util.module_from_spec(spec); spec.loader.exec_module(audit)


class TargetTraceAuditTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.a = Path(self.temp.name)/'reference'
        self.b = Path(self.temp.name)/'candidate'
        self.lines = [f'field[{i}]=0\n' for i in range(206)]
        self.lines += [f'pixel[{i}]=0\n' for i in range(320)] + ['END\n']
        self.a.write_text(''.join(self.lines)); self.b.write_text(''.join(self.lines))

    def test_exact_and_prefix_never_accepts(self):
        self.assertTrue(audit.audit(self.a,self.b)['exact_pass'])
        self.assertFalse(audit.audit(self.a,self.b,True)['exact_pass'])

    def test_large_integer_difference_is_not_rounded_away(self):
        self.a.write_text(''.join(self.lines).replace('field[0]=0', 'field[0]=18446744073709551614'))
        self.b.write_text(''.join(self.lines).replace('field[0]=0', 'field[0]=18446744073709551615'))
        result=audit.audit(self.a,self.b)
        self.assertFalse(result['exact_pass'])
        self.assertEqual(result['fields_by_name']['field[0]']['max_absolute'],1)

    def test_float_step_and_last_pixel_rejected(self):
        self.a.write_text(''.join(self.lines).replace('field[0]=0','field[0]=0.5'))
        self.b.write_text(''.join(self.lines).replace('field[0]=0','field[0]=0.5000000596046448').replace('pixel[319]=0','pixel[319]=1'))
        result=audit.audit(self.a,self.b)
        self.assertEqual(result['differing_fields'],2)
        self.assertEqual(result['fields_by_name']['field[0]']['max_f32_ulp'],1)

    def test_poison_schema_and_truncation_rejected(self):
        for text in [''.join(self.lines[:-1]), ''.join(self.lines).replace('pixel[319]','pixel[318]'),
                     ''.join(self.lines).replace('field[0]=0','field[0]=nan')]:
            self.a.write_text(text); self.b.write_text(text)
            with self.assertRaises(ValueError): audit.audit(self.a,self.b)


if __name__=='__main__': unittest.main()
