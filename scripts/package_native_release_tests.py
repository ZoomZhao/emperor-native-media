#!/usr/bin/env python3
"""Authored bundle/path tests; never grant an actual signature exception."""
from pathlib import Path
import importlib.util,json,os,plistlib,subprocess,tempfile,unittest
from unittest.mock import patch
spec=importlib.util.spec_from_file_location('package_native_release',Path(__file__).with_name('package_native_release.py'));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class ReleaseTests(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory(prefix='native-release-authored-');self.root=Path(self.temp.name);self.app=self.root/'Authored.app'
  for n in m.FILES:
   p=self.app/n;p.parent.mkdir(parents=True,exist_ok=True);p.write_text('self-authored test\n')
  (self.app/'Contents/Info.plist').write_bytes(plistlib.dumps({'CFBundleExecutable':'EmperorApp'}))
  (self.app/'Contents/Resources/source-manifest.json').write_text(json.dumps({'schema':m.SCHEMA,'sources_sha256':{'authored.cpp':'a'},'compiled_sources':['authored.cpp']}))
  e=self.app/'Contents/MacOS/EmperorApp';e.write_text('#!/bin/sh\nprintf "%s\\n" "$EMPEROR_NATIVE_METAL" "$@"\n');e.chmod(0o755)
  self.sig=patch.object(m,'verify_signature',return_value=None);self.sig.start()
 def tearDown(self):self.sig.stop();self.temp.cleanup()
 def test_new_copy_and_no_private_root(self):
  out=self.root/'版本 with spaces';r=m.package(self.app,out);self.assertFalse(r['original_data_bundled']);self.assertFalse(r['resource_root_saved']);self.assertEqual(m.validate_bundle(self.app)[0],m.validate_bundle(out/'EmperorApp.app')[0]);self.assertNotIn(str(self.root),(out/'Start.command').read_text());self.assertTrue((out/'PLAY.txt').exists())
 def test_collision_keeps_existing(self):
  out=self.root/'existing';out.mkdir();p=out/'keep';p.write_text('keep')
  with self.assertRaises(FileExistsError):m.package(self.app,out)
  self.assertEqual(p.read_text(),'keep')
 def test_original_extension_refused_before_output(self):
  p=self.app/'Contents/Resources/private.sav';p.write_bytes(b'authored forbidden-name fixture');out=self.root/'refused'
  with self.assertRaises(ValueError):m.package(self.app,out)
  self.assertFalse(out.exists())
 def test_symlink_refused(self):
  p=self.app/'Contents/Resources/shortcut';p.symlink_to(self.app/'Contents/Info.plist')
  with self.assertRaises(ValueError):m.package(self.app,self.root/'refused')
 def test_missing_metadata_refused(self):
  (self.app/'Contents/PkgInfo').unlink()
  with self.assertRaises(ValueError):m.package(self.app,self.root/'refused')
 def test_relocation_and_exact_arguments(self):
  out=self.root/'old';m.package(self.app,out);moved=self.root/'moved 中文';out.rename(moved);env=dict(os.environ);env['EMPEROR_NATIVE_METAL']='unexpected';arg='/not a real asset folder/中文'
  for name,mode in [('Start.command','0'),('Start-Metal.command','1')]:
   x=subprocess.run([str(moved/name),'--data-root',arg],env=env,capture_output=True,text=True);self.assertEqual(x.returncode,0);self.assertEqual(x.stdout.splitlines(),[mode,'--data-root',arg])
  self.assertEqual(env['EMPEROR_NATIVE_METAL'],'unexpected')
 def test_incomplete_version_will_not_launch(self):
  out=self.root/'out';m.package(self.app,out);(out/'release.json').unlink();x=subprocess.run([str(out/'Start.command')],capture_output=True,text=True);self.assertEqual(x.returncode,2)
 def test_dangling_output_alias_refused(self):
  link=self.root/'alias';target=self.root/'not-created';link.symlink_to(target,target_is_directory=True)
  with self.assertRaises(FileExistsError):m.package(self.app,link)
  self.assertTrue(link.is_symlink());self.assertFalse(target.exists())
 def test_no_output_inside_source(self):
  with self.assertRaises(ValueError):m.package(self.app,self.app/'nested')
  self.assertFalse((self.app/'nested').exists())
 def test_signature_failure_publishes_nothing(self):
  out=self.root/'bad-signature'
  with patch.object(m,'verify_signature',side_effect=ValueError('authored signature failure')):
   with self.assertRaises(ValueError):m.package(self.app,out)
  self.assertFalse(out.exists())
if __name__=='__main__':unittest.main()
