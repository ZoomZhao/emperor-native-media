#!/usr/bin/env python3
"""Copy a verified self-authored App to a new version directory; no game data."""
from pathlib import Path
import argparse,hashlib,json,os,plistlib,shutil,subprocess

SCHEMA='EMPEROR_SELF_AUTHORED_APP_BUILD_V1'
FILES={
 'Contents/Info.plist','Contents/PkgInfo','Contents/MacOS/EmperorApp',
 'Contents/Resources/LICENSE','Contents/Resources/THIRD_PARTY_NOTICES.md',
 'Contents/Resources/README.md','Contents/Resources/source-manifest.json',
 'Contents/_CodeSignature/CodeResources',
}
PLAY='''原生城市 · 版本化交付

Start.command 默认 CPU；Start-Metal.command 是同一包的可选 Metal 路径。
请保持本目录完整。应用和启动器使用相对路径，可一起移动；不会安装登录项。
不包含原版素材、存档或资源目录配置。首次会询问包含 DATA、Model 等目录
的原始数据根；也可传 --data-root /absolute/owned/data，或使用当前进程的
EMPEROR_GAMEDATA_ROOT。显式失效路径不会静默改用别处；退出后重新指定。

首页 → 新建原生城市… → 选择“数据通过”的空城来源。
不勾开发选项仍为 v6。“城市经营与预防巡检（开发版）”明确选择新版 v8。
创建后城市暂停、人口为零。先点“入城口”，把道路接到标记所在原有路网，
再建邻路住宅，恢复等待实际行人迁入。“邻路”只指局部道路；“入城可达”
是当前运行时路径条件（可含步行地面），“接原路网”另查实际道路分区。
这些信息不代表已派遣或必然到达。接路有利于布置连续的送水和巡检路线。
人口形成劳力后建水井和巡检塔，查看实际派工、水缓存和巡检记录；工人
或已派出的行人不代表所有住宅已被服务。暂停可建；1×/2×/4×只改墙钟速度。

存档… 使用新的 .encity 文件名；继续… 核对原始来源/模型并按保存策略暂停
载入。不会自动迁移旧档，也不会运行离线补算。退出使用菜单或 Cmd+Q，
有未保存内容时先另存或取消退出。保留旧包和与其匹配的旧档用于回退；
旧 App 不能保证识别新策略。开发验证检查点不是普通玩家开局入口。

已实现为限定原生玩法，不是完整原版：市场持续供给、完整日历/健康/消费、
更高住宅演化、实际灭火/灾后恢复和非空原版城市续玩仍有缺口。
原始数据始终外置只读。release.json 和包内 source-manifest.json 绑定本版本。
'''
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def verify_signature(app):
 x=subprocess.run(['/usr/bin/codesign','--verify','--deep','--strict',str(app)],capture_output=True,text=True)
 if x.returncode:raise ValueError('strict App signature verification failed: '+x.stderr.strip())
def validate_bundle(app):
 app=Path(app)
 if app.is_symlink()or not app.is_dir()or app.suffix!='.app':raise ValueError('regular .app directory required')
 files={}
 for path in app.rglob('*'):
  if path.is_symlink():raise ValueError('symlink in App bundle')
  if path.is_file():
   name=path.relative_to(app).as_posix()
   if name not in FILES:raise ValueError('unexpected bundle file; game data is never packaged: '+name)
   files[name]=sha(path)
 if set(files)!=FILES:raise ValueError('incomplete App metadata/executable/signature')
 info=plistlib.loads((app/'Contents/Info.plist').read_bytes())
 if info.get('CFBundleExecutable')!='EmperorApp':raise ValueError('unexpected executable')
 manifest=json.loads((app/'Contents/Resources/source-manifest.json').read_text())
 if manifest.get('schema')!=SCHEMA or not manifest.get('sources_sha256')or not manifest.get('compiled_sources'):raise ValueError('missing production provenance')
 verify_signature(app)
 return files,manifest
def launcher(metal):
 return '''#!/bin/sh
set -u
native_release_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
native_app="$native_release_dir/EmperorApp.app/Contents/MacOS/EmperorApp"
if [ ! -f "$native_release_dir/release.json" ] || [ -f "$native_release_dir/DELIVERY-INCOMPLETE.txt" ] || [ ! -x "$native_app" ]; then
  printf '%s\n' 'This version is incomplete. Keep the App, launchers and release.json together.' >&2
  exit 2
fi
EMPEROR_NATIVE_METAL='''+('1'if metal else'0')+''' "$native_app" "$@"
native_rc=$?
if [ "$native_rc" -ne 0 ]; then
  printf '%s\n' 'App did not start normally. See PLAY.txt; use --data-root with the correct original directory.' >&2
  if [ -t 0 ]; then printf '%s' 'Press Return to close: '; IFS= read -r native_reply || :; fi
fi
exit "$native_rc"
'''
def package(app,output):
 app=Path(app).absolute();provided=Path(output).absolute();root=Path(__file__).resolve().parents[1]
 if provided.is_symlink():raise FileExistsError('symlink delivery destination refused')
 output=provided.resolve()
 if output==root or root in output.parents:raise ValueError('release must be outside repository')
 if output==app.resolve()or app.resolve()in output.parents or output in app.resolve().parents:raise ValueError('delivery and source App must be separate')
 if output.exists()or output.is_symlink():raise FileExistsError('new unique version directory required')
 if not output.parent.is_dir():raise ValueError('existing delivery parent required')
 files,manifest=validate_bundle(app)
 # Exclusive reservation prevents replacement even if another process races.
 output.mkdir();target=output/'EmperorApp.app'
 try:
  shutil.copytree(app,target,copy_function=shutil.copy2)
  copied,_=validate_bundle(target)
  if copied!=files:raise ValueError('copied App bytes differ')
  for name,metal in [('Start.command',False),('Start-Metal.command',True)]:
   p=output/name;p.write_text(launcher(metal));p.chmod(0o755)
  (output/'PLAY.txt').write_text(PLAY)
  release={'schema':'EMPEROR_NATIVE_VERSIONED_DELIVERY_V1','App':'EmperorApp.app','binary_sha256':files['Contents/MacOS/EmperorApp'],'production_files':len(manifest['sources_sha256']),'compiled_units':len(manifest['compiled_sources']),'bundle_files_sha256':files,'launchers_sha256':{n:sha(output/n)for n in ['Start.command','Start-Metal.command','PLAY.txt']},'original_data_bundled':False,'resource_root_saved':False,'default_rules':6,'old_packages_overwritten':False,'packager_sha256':sha(__file__)}
  if validate_bundle(app)[0]!=files:raise ValueError('source App changed during delivery')
  # Launchers refuse until this complete final receipt is atomically published.
  receipt=output/'.release-receipt.tmp'
  with receipt.open('x')as f:f.write(json.dumps(release,indent=2)+'\n');f.flush();os.fsync(f.fileno())
  os.link(receipt,output/'release.json');receipt.unlink()
  return release
 except Exception as e:
  (output/'DELIVERY-INCOMPLETE.txt').write_text('Not a verified release: '+str(e)+'\n')
  raise
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--app',type=Path,required=True);p.add_argument('--output-dir',type=Path,required=True);a=p.parse_args();r=package(a.app,a.output_dir);print(json.dumps({'output':str(a.output_dir),'binary_sha256':r['binary_sha256'],'production_files':r['production_files'],'original_data_bundled':False}))
if __name__=='__main__':main()
