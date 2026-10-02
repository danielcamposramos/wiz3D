#!/usr/bin/env python3
"""Write bin/temp_version.h as bin/generate_version.ps1 does, into the build
tree (ver/bin/), so the read-only source tree stays untouched.
usage: gen_version.py <repo> <outdir> <git-sha> <yyyy-mm-dd>"""
import os, re, sys
repo, out, sha, date = sys.argv[1:5]
raw = open(os.path.join(repo, 'VERSION.txt')).read().strip()
major, minor, patch = (int(x) for x in raw.split('-', 1)[0].split('.')[:3])
ps1 = open(os.path.join(repo, 'bin', 'generate_version.ps1'), encoding='utf-8').read()
body = re.search(r'@"\n(.*?)\n"@', ps1, re.S).group(1)
body = (body.replace('$major', str(major)).replace('$minor', str(minor))
        .replace('$patch', str(patch)).replace('$gitSha', sha)
        .replace('$buildDate', date).replace('$buildTime', '00:00:00'))
os.makedirs(os.path.join(out, 'bin'), exist_ok=True)
os.makedirs(os.path.join(out, 'inc'), exist_ok=True)
with open(os.path.join(out, 'bin', 'temp_version.h'), 'w') as f:
    f.write(body + '\n')
print('temp_version.h:', major, minor, patch, sha, date)
