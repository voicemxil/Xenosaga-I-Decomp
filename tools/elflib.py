#!/usr/bin/env python3
"""Minimal ELF32 little-endian reader for the MIPS objects in this repo.

Why: every verification tool here used to shell out to objdump once (or
twice) per function -- ~2400 process spawns for a full `verify.py` run,
which is where its runtime went. Everything those tools actually need
(instruction words, relocation types, function symbol offsets) is a few
struct.unpacks away, so read the files directly instead.

Only the subset the decomp tooling needs is implemented: section headers,
symbol tables, SHT_REL relocations, and raw section bytes.
"""
import struct

SHT_REL = 9
STT_FUNC = 2
STT_OBJECT = 1

# MIPS relocation type -> the spelling objdump prints after "R_MIPS_",
# which is what the existing masking code keys on.
MIPS_RELOC_NAMES = {
    1: "16", 2: "32", 3: "REL32", 4: "26", 5: "HI16", 6: "LO16",
    7: "GPREL16", 8: "LITERAL", 9: "GOT16", 10: "PC16", 11: "CALL16",
    12: "GPREL32",
}


class Elf32:
    """Lazily-parsed ELF32-LE image."""

    def __init__(self, path):
        self.path = path
        with open(path, "rb") as f:
            self.data = f.read()
        d = self.data
        if d[:4] != b"\x7fELF" or d[4] != 1 or d[5] != 1:
            raise ValueError(f"{path}: not an ELF32 little-endian file")
        e_shoff, = struct.unpack_from("<I", d, 0x20)
        e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHH", d, 0x2E)
        self.sections = []
        for i in range(e_shnum):
            off = e_shoff + i * e_shentsize
            (sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size,
             sh_link, sh_info, _align, sh_entsize) = struct.unpack_from(
                "<10I", d, off)
            self.sections.append({
                "name_off": sh_name, "type": sh_type, "flags": sh_flags,
                "addr": sh_addr, "offset": sh_offset, "size": sh_size,
                "link": sh_link, "info": sh_info, "entsize": sh_entsize,
                "index": i,
            })
        shstr = self.sections[e_shstrndx]
        for s in self.sections:
            s["name"] = self._cstr(shstr["offset"] + s["name_off"])
        self.by_name = {s["name"]: s for s in self.sections}
        self._symbols = None
        self._relocs = {}

    def _cstr(self, off):
        end = self.data.index(b"\x00", off)
        return self.data[off:end].decode("ascii", "replace")

    def section_bytes(self, name):
        s = self.by_name.get(name)
        if s is None or s["type"] == 8:  # SHT_NOBITS
            return b""
        return self.data[s["offset"]:s["offset"] + s["size"]]

    def symbols(self):
        """[(name, value, size, sym_type, section_name)] from .symtab."""
        if self._symbols is None:
            self._symbols = []
            st = self.by_name.get(".symtab")
            if st is not None:
                strtab = self.sections[st["link"]]
                for off in range(st["offset"], st["offset"] + st["size"], 16):
                    (st_name, st_value, st_size, st_info, _other,
                     st_shndx) = struct.unpack_from("<IIIBBH", self.data, off)
                    if st_shndx == 0 or st_shndx >= len(self.sections):
                        sec = ""
                    else:
                        sec = self.sections[st_shndx]["name"]
                    self._symbols.append((
                        self._cstr(strtab["offset"] + st_name),
                        st_value, st_size, st_info & 0xF, sec))
        return self._symbols

    def func_symbols(self, section=".text"):
        """name -> offset, for defined STT_FUNC symbols in `section`."""
        out = {}
        for name, value, _size, typ, sec in self.symbols():
            if typ == STT_FUNC and sec == section and name:
                out.setdefault(name, value)
        return out

    def func_symbols_sized(self, section=".text"):
        """name -> (offset, size) for defined STT_FUNC symbols in `section`."""
        out = {}
        for name, value, size, typ, sec in self.symbols():
            if typ == STT_FUNC and sec == section and name:
                out.setdefault(name, (value, size))
        return out

    def object_symbols(self):
        """name -> (section, offset, size) for defined data symbols."""
        out = {}
        for name, value, size, typ, sec in self.symbols():
            if typ == STT_OBJECT and sec and name:
                out.setdefault(name, (sec, value, size))
        return out

    def relocs(self, section=".text"):
        """byte offset within `section` -> relocation type name."""
        if section not in self._relocs:
            table = {}
            for s in self.sections:
                if s["type"] != SHT_REL or s["name"] != ".rel" + section:
                    continue
                for off in range(s["offset"], s["offset"] + s["size"], 8):
                    r_offset, r_info = struct.unpack_from("<II", self.data, off)
                    table[r_offset] = MIPS_RELOC_NAMES.get(
                        r_info & 0xFF, str(r_info & 0xFF))
            self._relocs[section] = table
        return self._relocs[section]

    def words_at_vaddr(self, addr, size):
        """Instruction words at a virtual address (for a linked ELF)."""
        for s in self.sections:
            if s["addr"] and s["addr"] <= addr < s["addr"] + s["size"]:
                if s["type"] == 8:
                    return []
                start = s["offset"] + (addr - s["addr"])
                avail = max(0, min(size, s["addr"] + s["size"] - addr))
                raw = self.data[start:start + avail]
                return list(struct.unpack_from(
                    f"<{len(raw) // 4}I", raw, 0)) if raw else []
        return []

    def words_at_offset(self, offset, size, section=".text"):
        """Instruction words at a section-relative offset (for a .o)."""
        s = self.by_name.get(section)
        if s is None or s["type"] == 8:
            return []
        avail = max(0, min(size, s["size"] - offset))
        raw = self.data[s["offset"] + offset:s["offset"] + offset + avail]
        return list(struct.unpack_from(f"<{len(raw) // 4}I", raw, 0)) if raw else []
