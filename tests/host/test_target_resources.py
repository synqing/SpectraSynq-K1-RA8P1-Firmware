import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'scripts'))
from target_resources import validate_resources


class TargetResourceAcceptanceTests(unittest.TestCase):
    def setUp(self):
        self.acceptance = {
            'stack_untouched_min_bytes': 4096,
            'heap_free_at_maximum_min_bytes': 32768,
            'heap_used_growth_max_bytes': 0,
            'heap_maximum_growth_max_bytes': 0,
        }
        self.before = {
            'stack_untouched_bytes': 8192,
            'heap_total': 100000,
            'heap_used': 10000,
            'heap_maximum': 12000,
        }

    def validate(self, **changes):
        after = dict(self.before)
        after.update(changes)
        validate_resources(self.before, after, self.acceptance, 'test')

    def test_unchanged_resources_pass(self):
        self.validate()

    def test_heap_pool_change_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'heap pool changed'):
            self.validate(heap_total=99999)

    def test_stack_reserve_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'resource reserve'):
            self.validate(stack_untouched_bytes=4095)

    def test_heap_reserve_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'resource reserve'):
            self.validate(heap_maximum=70000)

    def test_live_heap_growth_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'heap growth'):
            self.validate(heap_used=10001)

    def test_heap_highwater_growth_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'heap growth'):
            self.validate(heap_maximum=12001)


if __name__ == '__main__':
    unittest.main()
