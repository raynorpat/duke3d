#!/usr/bin/env python3
"""Retype C `long` to fixed-width types for 64-bit builds.

`long` / `signed long` / `long int` become `int32_t`, and `unsigned long`
becomes `uint32_t`. `long long` and anything inside comments or string and
character literals are left alone. Every file that changes gets
`#include <stdint.h>` (before its first #include, after its include guard,
or at the top). The rewrite is idempotent.

Usage: tools/retype_long.py FILE...
"""
import re
import sys

TOKEN = re.compile(r'\b(unsigned\s+long\s+long|signed\s+long\s+long|long\s+long'
                   r'|unsigned\s+long\s+int|unsigned\s+long|signed\s+long'
                   r'|long\s+int|long)\b')
SKIP = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.S)


def retype_code(code):
    def rep(m):
        t = ' '.join(m.group(1).split())
        if 'long long' in t:
            return m.group(1)
        return 'uint32_t' if t.startswith('unsigned') else 'int32_t'
    return TOKEN.sub(rep, code)


def retype(text):
    out, last = [], 0
    for m in SKIP.finditer(text):
        out.append(retype_code(text[last:m.start()]))
        out.append(m.group(0))
        last = m.end()
    out.append(retype_code(text[last:]))
    return ''.join(out)


def add_stdint(text):
    if '<stdint.h>' in text:
        return text
    line = '#include <stdint.h>\n'
    m = re.search(r'^[ \t]*#[ \t]*include\b', text, re.M)
    if m:
        return text[:m.start()] + line + text[m.start():]
    m = re.search(r'^[ \t]*#[ \t]*ifndef[ \t]+(\w+)[ \t]*\n[ \t]*#[ \t]*define[ \t]+\1\b[^\n]*\n',
                  text, re.M)
    if m:
        return text[:m.end()] + line + text[m.end():]
    return line + text


def main(paths):
    changed = 0
    for path in paths:
        with open(path, encoding='latin-1', newline='') as f:
            old = f.read()
        new = retype(old)
        if new != old:
            new = add_stdint(new)
            with open(path, 'w', encoding='latin-1', newline='') as f:
                f.write(new)
            changed += 1
    print(f'retyped {changed} of {len(paths)} files')


if __name__ == '__main__':
    main(sys.argv[1:])
