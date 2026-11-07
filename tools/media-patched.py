"""Exit 0 when a default.xbe is media-patched: the disc check's branch at 0x121EBC is already "jmp 0x121EC3"
(EB 05), so the port's ROTK_DiscCheckFailed hook there must be left out (patches/media-patched-xbe.patch).
usage: python3 media-patched.py default.xbe"""
import struct, sys

d = open(sys.argv[1], 'rb').read()
if d[:4] != b'XBEH':
    sys.exit('not an XBE: ' + sys.argv[1])
base = struct.unpack_from('<I', d, 0x104)[0]
count, headers = struct.unpack_from('<2I', d, 0x11C)
for i in range(count):
    _, va, vsize, raw, rsize = struct.unpack_from('<5I', d, headers - base + i * 56)
    if va <= 0x121EBC < va + min(vsize, rsize):
        sys.exit(0 if d[raw + 0x121EBC - va:raw + 0x121EBC - va + 2] == b'\xEB\x05' else 1)
sys.exit('0x121EBC is in no section')
