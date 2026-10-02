#!/usr/bin/env python3
"""stdin -> stdout: UTF-8 text with non-ASCII characters written as rc octal
escapes. llvm-rc rejects non-ASCII bytes in narrow strings (VERSIONINFO
copyright signs); Microsoft rc takes them from the code page."""
import sys
for ch in sys.stdin.read():
    if ord(ch) < 128:
        sys.stdout.write(ch)
    else:
        try:
            sys.stdout.write('\\%03o' % ch.encode('cp1252')[0])
        except UnicodeEncodeError:
            sys.stdout.write('?')
