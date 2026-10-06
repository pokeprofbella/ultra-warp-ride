#!/usr/bin/env python3
"""Packs a linked plugin ELF into a .3gx file (format "3GX$0002", as read by
Luma3DS and Azahar). Written for this project so no extra tool is needed.

usage: pack3gx.py input.elf output.3gx
"""
import struct
import sys

TITLE = "Ultra Warp Ride - Missao Living Dex"
AUTHOR = "OTBella"
SUMMARY = "Anos-luz exatos e chances no Ultra Warp Ride"
DESCRIPTION = ("Mostra os anos-luz exatos de cada buraco do Ultra Warp Ride, com cor, aneis "
               "e as chances de lendario, shiny e Ultra Beast.")
VERSION = (1, 0, 0)
TARGETS = [0x001B5000, 0x001B5100]  # Ultra Sun, Ultra Moon (low title IDs)

LOAD_ADDR = 0x07000100
# The loader runs this function before starting the plugin and compares its
# return value with exeLoadChecksum. Ours does nothing and returns 0.
EXE_LOAD_FUNC = [0xE3A00000, 0xE12FFF1E, 0xE320F000]  # mov r0,#0 ; bx lr ; nop (end marker)
FLAGS = (1 << 0) | (2 << 4) | (1 << 7)  # embedded load func | console+emulator | no swap needed


def segments(elf):
    if elf[:4] != b"\x7fELF" or elf[4] != 1 or elf[5] != 1:
        sys.exit("not a 32-bit little-endian ELF")
    phoff, = struct.unpack_from("<I", elf, 0x1C)
    phentsize, phnum = struct.unpack_from("<HH", elf, 0x2A)
    out = []
    for i in range(phnum):
        p_type, off, vaddr, _pa, filesz, memsz, flags, _al = struct.unpack_from(
            "<8I", elf, phoff + i * phentsize)
        if p_type == 1 and memsz:  # PT_LOAD
            out.append((vaddr, elf[off:off + filesz], memsz, flags))
    return sorted(out)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    elf = open(sys.argv[1], "rb").read()
    segs = segments(elf)
    code = [s for s in segs if s[3] & 1]          # executable
    data = [s for s in segs if s[3] & 2]          # writable
    rodata = [s for s in segs if s[3] & 3 == 0]   # read-only
    if len(code) != 1 or len(data) != 1 or len(rodata) > 1:
        sys.exit("unexpected segments: %s" % [(hex(v), m, f) for v, _, m, f in segs])
    code, data = code[0], data[0]
    if code[0] != LOAD_ADDR:
        sys.exit("code must start at 0x%08X (got 0x%08X)" % (LOAD_ADDR, code[0]))

    # The loader places code, rodata and data back to back, so keep the gaps.
    ro_start = rodata[0][0] if rodata else data[0]
    if len(code[1]) > ro_start - code[0] or (rodata and len(rodata[0][1]) > data[0] - ro_start):
        sys.exit("segments overlap")
    code_bytes = code[1].ljust(ro_start - code[0], b"\0")
    ro_bytes = rodata[0][1].ljust(data[0] - ro_start, b"\0") if rodata else b""
    data_bytes = data[1]
    bss = data[2] - len(data[1])

    header_size = 8 + 4 + 4 + 18 * 4 + 10 * 4 + 2 * 4 + 3 * 4
    body = bytearray()

    def put(b, align=4):
        while (header_size + len(body)) % align:
            body.append(0)
        pos = header_size + len(body)
        body.extend(b)
        return pos

    strings = {}
    for key, text in (("title", TITLE), ("author", AUTHOR), ("summary", SUMMARY), ("desc", DESCRIPTION)):
        raw = text.encode("ascii") + b"\0"
        strings[key] = (len(raw), put(raw, 1))
    targets_off = put(struct.pack("<%dI" % len(TARGETS), *TARGETS))
    func_off = put(struct.pack("<%dI" % len(EXE_LOAD_FUNC), *EXE_LOAD_FUNC))
    code_off = put(code_bytes, 8)
    ro_off = put(ro_bytes)
    data_off = put(data_bytes)

    version = (VERSION[0] << 24) | (VERSION[1] << 16) | (VERSION[2] << 8)
    infos = struct.pack(
        "<8I2I4I4I",
        strings["author"][0], strings["author"][1],
        strings["title"][0], strings["title"][1],
        strings["summary"][0], strings["summary"][1],
        strings["desc"][0], strings["desc"][1],
        FLAGS, 0,            # flags, exeLoadChecksum (our load func returns 0)
        0, 0, 0, 0,          # builtInLoadExeArgs
        0, 0, 0, 0)          # builtInSwapSaveLoadArgs
    exe = struct.pack("<10I", code_off, ro_off, data_off,
                      len(code_bytes), len(ro_bytes), len(data_bytes), bss,
                      func_off, 0, 0)
    header = (b"3GX$0002" + struct.pack("<II", version, 0) + infos + exe +
              struct.pack("<II", len(TARGETS), targets_off) + struct.pack("<3I", 0, 0, 0))
    assert len(header) == header_size
    open(sys.argv[2], "wb").write(header + body)
    print("criado %s (code %d, rodata %d, data %d, bss %d bytes)" % (
        sys.argv[2], len(code_bytes), len(ro_bytes), len(data_bytes), bss))


if __name__ == "__main__":
    main()
