"""Authored CFGs and finite-domain properties; no original instruction execution."""
import itertools,json,unittest
from pathlib import Path
from monotone_flags import Solver,TOP,INCOMING,TOP_STATE,join,leq,substitute
def node(pc,successors=(),**kwargs):return dict(pc=pc,successors=list(successors),**kwargs)
def uniform(values):return tuple(frozenset(values)for _ in range(6))
class FlagsTests(unittest.TestCase):
 def solve(self,rows,entries):return Solver({r['pc']:r for r in rows},entries).solve()
 def test_loop_inc_preserves_carry(self):
  s=self.solve([node(10,[11],defines=63),node(11,[12,13]),node(12,[11],defines=62),node(13,normal_return=True)],[10])
  self.assertEqual(s.summaries[10][0],frozenset([10]))
  for b in range(1,6):self.assertEqual(s.summaries[10][b],frozenset([10,12]))
 def test_self_recursion_with_base(self):
  s=self.solve([node(10,[11,13]),node(11,[12],local_callee=10),node(12,normal_return=True),node(13,[14],defines=63),node(14,normal_return=True)],[10])
  self.assertEqual(s.summaries[10],uniform([13]));self.assertFalse(s.no_return_promotions)
 def test_mutual_recursion_with_base(self):
  rows=[node(10,[11],local_callee=20),node(11,normal_return=True),node(20,[21,23]),node(21,[22],local_callee=10),node(22,normal_return=True),node(23,[24],defines=63),node(24,normal_return=True)]
  for order in ([10,20],[20,10]):
   s=self.solve(rows if order[0]==10 else list(reversed(rows)),order)
   self.assertEqual(s.summaries[10],uniform([23]));self.assertEqual(s.summaries[20],uniform([23]))
 def test_no_base_recursion_explicit_top_not_dead(self):
  s=self.solve([node(10,[11],local_callee=20),node(11,normal_return=True),node(20,[21],local_callee=10),node(21,normal_return=True)],[10,20])
  self.assertEqual(s.summaries[10],TOP_STATE);self.assertEqual(s.summaries[20],TOP_STATE);self.assertEqual(sorted(s.no_return_promotions),[10,20]);self.assertIn(11,s.states[10])
 def test_unknown_call_kills_provenance(self):
  s=self.solve([node(1,[2],defines=63),node(2,[3],call_unknown=True),node(3,normal_return=True)],[1]);self.assertEqual(s.summaries[1],TOP_STATE)
 def test_unknown_jump_cannot_be_silently_dead(self):
  s=self.solve([node(1,open_exit=True)],[1]);self.assertEqual(s.summaries[1],TOP_STATE);self.assertEqual(s.diagnostics,[(1,1)])
 def test_missing_cfg_successor_is_top(self):
  s=self.solve([node(1,[999])],[1]);self.assertEqual(s.summaries[1],TOP_STATE);self.assertEqual(s.diagnostics,[(1,999)])
 def test_undefined_flags_are_top(self):
  s=self.solve([node(1,[2],defines=59,undefined=4),node(2,normal_return=True)],[1]);self.assertEqual(s.summaries[1][2],frozenset([TOP]));self.assertEqual(s.summaries[1][3],frozenset([1]))
 def test_variable_shift_includes_zero_and_undefined(self):
  s=self.solve([node(1,[2],defines=63),node(2,[3],defines=27,undefined=36,conditional_effect=True),node(3,normal_return=True)],[1])
  self.assertEqual(s.summaries[1][0],frozenset([1,2]));self.assertEqual(s.summaries[1][5],frozenset([TOP]));self.assertEqual(s.summaries[1][2],frozenset([TOP]))
 def test_rep_zero_iterations_preserve_prior_flags(self):
  s=self.solve([node(1,[2],defines=63),node(2,[3],defines=63,conditional_effect=True),node(3,normal_return=True)],[1]);self.assertEqual(s.summaries[1],uniform([1,2]))
 def test_cross_block_carry_producer(self):
  s=self.solve([node(1,[2],defines=63),node(2,[3,4]),node(3,[4]),node(4,[5],defines=63),node(5,normal_return=True)],[1]);self.assertEqual(s.states[1][4][0],frozenset([1]))
 def test_unrecognized_flag_semantics_is_top(self):
  s=self.solve([node(1,[2],semantic_unknown=True),node(2,normal_return=True)],[1]);self.assertEqual(s.summaries[1],TOP_STATE)
 def test_join_and_call_substitution_monotone_on_finite_domain(self):
  origins=[INCOMING,1,2];values=[uniform([origins[i]for i in range(3)if m&(1<<i)])for m in range(8)]+[TOP_STATE]
  comparable=[(a,b)for a in values for b in values if leq(a,b)]
  for a,b in comparable:
   for c,d in comparable:self.assertTrue(leq(substitute(a,c),substitute(b,d)))
   for c in values:self.assertTrue(leq(join(a,c),join(b,c)))
  self.assertTrue(leq(None,TOP_STATE));self.assertIsNone(substitute(None,uniform([1])))
 def test_instruction_transfer_monotone(self):
  values=[uniform(v)for v in ([],[INCOMING],[1],[INCOMING,1],[TOP])];s=Solver({1:node(1,normal_return=True)},[1])
  transfers=[node(1),node(1,defines=63),node(1,undefined=63),node(1,defines=27,undefined=36,conditional_effect=True),node(1,semantic_unknown=True)]
  for a,b in itertools.product(values,repeat=2):
   if leq(a,b):
    for r in transfers:self.assertTrue(leq(s.effect(r,a),s.effect(r,b)))
if __name__=='__main__':
 suite=unittest.defaultTestLoader.loadTestsFromTestCase(FlagsTests);result=unittest.TextTestRunner(verbosity=2).run(suite)
 path=Path(__file__).resolve().parents[1]/'evidence/flags-unit-results.json';assert not path.exists();path.write_text(json.dumps(dict(tests=result.testsRun,failures=len(result.failures),errors=len(result.errors),passed=result.wasSuccessful(),original_or_generated_game_executions=0),indent=2)+'\n')
 raise SystemExit(0 if result.wasSuccessful()else 1)
