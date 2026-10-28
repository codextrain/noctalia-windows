import unittest
from vitoria_voice import parse,grammar
class Commands(unittest.TestCase):
 def test_pc_commands(self):
  for phrase in ['PC, desligar em 1 hora','pê cê desligar em uma hora','p c desligar em uma hora']:
   self.assertEqual(parse(phrase),{'action':'schedule','minutes':60})
  self.assertEqual(parse('PC desligar em trinta e cinco minutos'),{'action':'schedule','minutes':35})
  for phrase in ['PC cancelar','pê cê cancelar','p c cancelar']:
   self.assertEqual(parse(phrase),{'action':'cancel'})
  self.assertEqual(parse('PC desativar comandos de voz'),{'action':'disable'})
 def test_short_minutes_and_grammar_gender(self):
  from vitoria_voice import spoken
  for phrase in ['PC desligar em dois minutos','PC desligar em 2 minutos','pê cê desligar em dois minutos']:
   self.assertEqual(parse(phrase),{'action':'schedule','minutes':2})
  phrases=grammar()
  for wake in ['pê cê','p c']:
   for n in range(1,61):
    self.assertIn(f'{wake} desligar em {spoken(n)} '+('minuto' if n==1 else 'minutos'),phrases)
   self.assertIn(wake+' desligar em um minuto',phrases)
   self.assertIn(wake+' desligar em dois minutos',phrases)
   self.assertIn(wake+' desligar em vinte e dois minutos',phrases)
   self.assertIn(wake+' desligar em uma hora',phrases)
   self.assertIn(wake+' desligar em duas horas',phrases)
   self.assertIn(wake+' desligar em vinte e uma horas',phrases)
   self.assertNotIn(wake+' desligar em duas minutos',phrases)
   self.assertNotIn(wake+' desligar em uma minuto',phrases)
 def test_reject_unrelated_and_out_of_range(self):
  for phrase in ['computador desligar em uma hora','desligar em uma hora','PC desligar agora','PC desligar em zero minutos','PC desligar em 25 horas','PC formatar disco','o pc desligar em uma hora']:
   self.assertIsNone(parse(phrase))
 def test_grammar_commands_are_parseable(self):
  for phrase in grammar()[:-1]:self.assertIsNotNone(parse(phrase),phrase)

class Feedback(unittest.TestCase):
 def test_only_fresh_successful_voice_acknowledgements_play(self):
  import tempfile,json
  from pathlib import Path
  from vitoria_voice import feedback
  with tempfile.TemporaryDirectory() as directory:
   p=Path(directory)/'config.json';sounds=[]
   def write(v):p.write_text(json.dumps({'windows':{'shutdown_voice_feedback_json':json.dumps(v)}}))
   p.write_text('{}');self.assertEqual(feedback(p,'',lambda:sounds.append(1),10000),'');self.assertEqual(sounds,[])
   write({'id':'one','action':'schedule','accepted':True,'createdAt':9000})
   seen=feedback(p,'',lambda:sounds.append(1),10000);self.assertEqual(len(sounds),1)
   feedback(p,seen,lambda:sounds.append(1),10000);self.assertEqual(len(sounds),1)
   write({'id':'old','action':'cancel','accepted':True,'createdAt':1000});feedback(p,seen,lambda:sounds.append(1),10000);self.assertEqual(len(sounds),1)
   write({'id':'failed','action':'schedule','accepted':False,'createdAt':9000});feedback(p,seen,lambda:sounds.append(1),10000);self.assertEqual(len(sounds),1)

if __name__=='__main__':unittest.main()
