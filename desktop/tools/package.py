#!/usr/bin/env python3
"""Stage the Windows binaries and complete corresponding source together."""
import argparse, pathlib, zipfile, shutil, hashlib, subprocess, json
parser=argparse.ArgumentParser()
parser.add_argument('--build',required=True,type=pathlib.Path)
parser.add_argument('--juce',required=True,type=pathlib.Path)
parser.add_argument('--out',type=pathlib.Path,default=pathlib.Path('release'))
a=parser.parse_args(); root=pathlib.Path(__file__).resolve().parents[2]; desktop=root/'desktop'
a.out=a.out.resolve(); a.out.mkdir(parents=True,exist_ok=True)
stage=a.out/'stage'; stage.mkdir(exist_ok=True)
artefacts=a.build/'PulseFoundry_artefacts'/'Release'
exe=artefacts/'Standalone'/'Pulse Foundry.exe'; vst=artefacts/'VST3'/'Pulse Foundry.vst3'
if not exe.is_file() or not vst.is_dir(): raise SystemExit('Build both Windows targets before packaging.')
shutil.copy2(exe,stage/exe.name)
shutil.copytree(vst,stage/vst.name,dirs_exist_ok=True)
shutil.copy2(root/'LICENSE',stage/'LICENSE.txt')
shutil.copy2(root/'NOTICE.md',stage/'NOTICE.md')
shutil.copy2(desktop/'docs'/'QUICKSTART.md',stage/'README.txt')
shutil.copy2(desktop/'docs'/'THIRD_PARTY.md',stage/'THIRD_PARTY.md')
source_zip=stage/'PulseFoundry-source.zip'
with zipfile.ZipFile(source_zip,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
    # Explicit exclusions keep account metadata, build outputs and credentials out.
    for f in sorted(root.rglob('*')):
        if not f.is_file(): continue
        rel=f.relative_to(root)
        if any(p in {'.git','.deps','build','release','dist','__pycache__'} for p in rel.parts): continue
        if rel.parts[:2]==('desktop','generated'): continue
        z.write(f,pathlib.Path('PulseFoundry-source')/rel)
    for f in sorted(a.juce.rglob('*')):
        if not f.is_file() or any(p in {'.git','build','__pycache__'} for p in f.relative_to(a.juce).parts): continue
        z.write(f,pathlib.Path('PulseFoundry-source/desktop/third_party/JUCE')/f.relative_to(a.juce))
shutil.copy2(source_zip,a.out/source_zip.name)
manifest={
 'name':'Pulse Foundry','version':'0.1.0','firmware_commit':'a43cd4fcda23ec3d577dc425261c3eb28bd5abff',
 'juce_version':'7.0.12','juce_commit':'4f43011b96eb0636104cb3e433894cda98243626',
 'platform':'Windows x64','windows_runtime_validation':'pending on Windows / Ableton Live 12'}
(a.out/'BUILD-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(stage)
