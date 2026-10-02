#!/usr/bin/env python3
"""Copy the built DLLs into the runtime layout: proxy and wrapper DLLs next
to each other, output plug-ins in OutputMethods/ (see S3DAPI LoadOutputDll)."""
import os, shutil, sys
build, out, sol, arch = sys.argv[1:5]
shutil.rmtree(out, ignore_errors=True)
os.makedirs(os.path.join(out, 'OutputMethods'))
for f in sorted(os.listdir(os.path.join(build, 'bin'))):
    dest = 'OutputMethods' if f.lower().startswith('sidebyside') else ''
    shutil.copy(os.path.join(build, 'bin', f), os.path.join(out, dest, f))
    print('staged', os.path.join(dest, f))

# runtime config template shipped with the project's own DX9 release layout
cfg = os.path.join(sol, 'releases', 'wiz3D', 'dx9', arch, 'wiz3D_Config.xml')
if os.path.isfile(cfg):
    shutil.copy(cfg, out)
    print('staged wiz3D_Config.xml (from releases/wiz3D/dx9/%s)' % arch)
prof = os.path.join(sol, 'S3DDriverSetup', 'Content', 'BaseProfile.xml')
if os.path.isfile(prof):
    shutil.copy(prof, out)
    print('staged BaseProfile.xml')
