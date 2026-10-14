#!/usr/bin/env python3
"""Runs the in-game reset patch code of a built Pico Loader in an ARM emulator (unicorn).

Checks the key check, the dispatch part for the stock SDK irq dispatcher and the reset part
exactly as assembled in picoLoader9: which keys trigger it, after how many vblanks, that the
handler is still entered with the right registers and return address, the slot 1 lock wait,
and the values the reset writes (EXMEMCNT, DMA, reset parameter 'PIGR').

What it can't check: whether the patch is installed into a real game (signature search, patch
space). That shows in /_pico/igr-log.txt on the console.

Usage: python3 loader/test/igr_patchcode_test.py [path to picoLoader9_<PLATFORM>.elf]
       (default: loader/work/pico-loader/build/picoLoader9_M3DS.elf, made by loader/build.sh)
Needs: pip install unicorn pyelftools
"""
import os
import struct
import sys

from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UcError
from unicorn.arm_const import (UC_CPU_ARM_926, UC_ARM_REG_CPSR, UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_R0,
                               UC_ARM_REG_R2, UC_ARM_REG_R4, UC_ARM_REG_SP)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ELF = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    ROOT, 'work', 'pico-loader', 'build', 'picoLoader9_M3DS.elf')

# layout of the emulated memory
MAIN_RAM = 0x02000000
KEYCHECK_BASE = 0x02001000
DISPATCH_BASE = 0x02001200
RESET_BASE = 0x02001400
IRQ_TABLE = 0x02002000
HANDLERS = 0x02003000      # one 'bx lr' per irq, ARM
IRQ_RETURN = 0x02004000    # where the handler returns to: end of one dispatch
RESET_SYSTEM = 0x02005000  # reboot entry: end of a reset
STACK = 0x02100000
SLOT1_LOCK = 0x027FFFE0
RESET_PARAM = 0x027FFC20
IO = 0x04000000
REG_MASTER_BRIGHT = 0x0400006C
REG_KEYINPUT = 0x04000130
REG_EXMEMCNT = 0x04000204
DMA_CNT = [0x040000B8, 0x040000C4, 0x040000D0, 0x040000DC]
PIGR = 0x52474950

KEY_A, KEY_B, KEY_DOWN, KEY_R, KEY_L = 1 << 0, 1 << 1, 1 << 7, 1 << 8, 1 << 9
COMBO = KEY_L | KEY_R | KEY_DOWN | KEY_B


def load_elf(path):
    with open(path, 'rb') as f:
        elf = ELFFile(f)
        sections = {}
        for name in ('patch_ingamereset_keycheck', 'patch_ingamereset_dispatch_sdk',
                     'patch_ingamereset_reset'):
            s = elf.get_section_by_name(name)
            if s is None:
                raise SystemExit(f'{name} not found in {path}')
            sections[name] = (s['sh_addr'], s.data())
        symbols = {}
        for sym in elf.get_section_by_name('.symtab').iter_symbols():
            if sym.name.startswith('patch_ingamereset_'):
                symbols[sym.name] = sym['st_value']
        return sections, symbols


class Machine:
    def __init__(self, sections, symbols):
        self.uc = Uc(UC_ARCH_ARM, UC_MODE_ARM)
        self.uc.ctl_set_cpu_model(UC_CPU_ARM_926)  # ARMv5TE like the ARM946E-S (has swp)
        self.uc.mem_map(MAIN_RAM, 0x800000)
        self.uc.mem_map(IO, 0x2000)
        self.sym = symbols
        self.bases = {}
        for name, base in (('patch_ingamereset_keycheck', KEYCHECK_BASE),
                           ('patch_ingamereset_dispatch_sdk', DISPATCH_BASE),
                           ('patch_ingamereset_reset', RESET_BASE)):
            addr, data = sections[name]
            self.bases[name] = (addr, base, len(data))
            self.uc.mem_write(base, data)
        # the literals Pico Loader fills in when it places the code (InGameResetPatchCode.h)
        self.set_literal('patch_ingamereset_resetAddress', self.target('patch_ingamereset_resetEntry'))
        self.set_literal('patch_ingamereset_sdkKeyCheck', self.target('patch_ingamereset_keyCheck'))
        self.set_literal('patch_ingamereset_sdkIrqTable', IRQ_TABLE)
        self.set_literal('patch_ingamereset_sdkIrqReturn', IRQ_RETURN)
        self.set_literal('patch_ingamereset_slot1LockAddress', SLOT1_LOCK)
        self.set_literal('patch_ingamereset_resetParamAddress', RESET_PARAM)
        self.set_literal('patch_ingamereset_resetParam', PIGR)
        self.set_literal('patch_ingamereset_resetSystemEntry', RESET_SYSTEM)
        for i in range(32):
            self.w32(IRQ_TABLE + 4 * i, HANDLERS + 0x10 * i)
            # mov r4, #i ; bx lr  (r4 tells which handler ran)
            self.w32(HANDLERS + 0x10 * i, 0xE3A04000 | i)
            self.w32(HANDLERS + 0x10 * i + 4, 0xE12FFF1E)
        self.w16(REG_KEYINPUT, 0x3FF)
        self.w16(REG_EXMEMCNT, 0xE880)
        for cnt in DMA_CNT:
            self.w32(cnt, 0x80000000)

    def target(self, name):
        """Address of a symbol after placing its section, thumb bit kept."""
        value = self.sym[name]
        for addr, base, size in self.bases.values():
            if addr <= (value & ~1) < addr + size:
                return value - addr + base
        raise KeyError(name)

    def set_literal(self, name, value):
        self.w32(self.target(name) & ~1, value)

    def w16(self, addr, value):
        self.uc.mem_write(addr, struct.pack('<H', value))

    def w32(self, addr, value):
        self.uc.mem_write(addr, struct.pack('<I', value))

    def r16(self, addr):
        return struct.unpack('<H', self.uc.mem_read(addr, 2))[0]

    def r32(self, addr):
        return struct.unpack('<I', self.uc.mem_read(addr, 4))[0]

    def press(self, keys):
        self.w16(REG_KEYINPUT, 0x3FF & ~keys)

    def dispatch(self, irq):
        """One pass through the patched dispatcher for interrupt irq (0 = vblank).
        Returns 'handler' with the handler number, or 'reset'."""
        self.uc.reg_write(UC_ARM_REG_SP, STACK)
        self.uc.reg_write(UC_ARM_REG_R0, irq)
        self.uc.reg_write(UC_ARM_REG_R4, 0xFFFFFFFF)
        self.uc.reg_write(UC_ARM_REG_LR, 0xDEADBEEF)
        self.uc.reg_write(UC_ARM_REG_CPSR, 0xD2)  # irq mode, irqs off, arm
        stops = {IRQ_RETURN, RESET_SYSTEM}
        state = {}

        def hook(uc, address, size, user_data):
            if address in stops:
                state['stop'] = address
                uc.emu_stop()

        h = self.uc.hook_add(1 << 2, hook)  # UC_HOOK_CODE
        try:
            self.uc.emu_start(self.target('patch_ingamereset_sdkDispatch'), 0, count=500)
        except UcError as e:
            raise AssertionError(f'emulation error {e} at 0x{self.uc.reg_read(UC_ARM_REG_PC):08X}')
        finally:
            self.uc.hook_del(h)
        stop = state.get('stop')
        if stop == IRQ_RETURN:
            return 'handler', self.uc.reg_read(UC_ARM_REG_R4)
        if stop == RESET_SYSTEM:
            return 'reset', None
        raise AssertionError(f'did not finish, pc 0x{self.uc.reg_read(UC_ARM_REG_PC):08X}')


def find_local(path, name, section_name):
    with open(path, 'rb') as f:
        elf = ELFFile(f)
        for sym in elf.get_section_by_name('.symtab').iter_symbols():
            if sym.name == name:
                sec = elf.get_section(sym['st_shndx'])
                if sec.name == section_name:
                    return sym['st_value'] - sec['sh_addr']
    raise KeyError(name)


FAILURES = []


def check(condition, text):
    print(('ok    ' if condition else 'FAIL  ') + text)
    if not condition:
        FAILURES.append(text)


def main():
    sections, symbols = load_elf(ELF)
    held_offset = find_local(ELF, 'heldFrames', 'patch_ingamereset_keycheck')
    print(f'{ELF}\nkeycheck 0x{len(sections["patch_ingamereset_keycheck"][1]):X} bytes, '
          f'dispatch_sdk 0x{len(sections["patch_ingamereset_dispatch_sdk"][1]):X}, '
          f'reset 0x{len(sections["patch_ingamereset_reset"][1]):X}')

    def held(m):
        return m.r16(KEYCHECK_BASE + held_offset)

    # 1. no keys: every interrupt reaches its handler, nothing counts
    m = Machine(sections, symbols)
    check(m.dispatch(0) == ('handler', 0), 'vblank without keys enters handler 0')
    check(m.dispatch(5) == ('handler', 5), 'irq 5 enters handler 5')
    check(held(m) == 0, 'no keys: hold count stays 0')
    check(m.r16(REG_MASTER_BRIGHT) == 0, 'no keys: master brightness untouched')

    # 2. the combo, held: 120 vblanks are counted, the 121st resets
    m = Machine(sections, symbols)
    m.press(COMBO)
    results = [m.dispatch(0) for _ in range(120)]
    check(all(r == ('handler', 0) for r in results), '120 vblanks with L+R+Down+B: game keeps running')
    check(held(m) == 120, f'hold count is 120 (is {held(m)})')
    check(m.dispatch(3) == ('handler', 3), 'other irqs do not count or reset')
    check(m.dispatch(0) == ('reset', None), '121st vblank (2 s at 60 fps) enters the reboot')
    check(m.r32(RESET_PARAM) == PIGR, "reset parameter is 'PIGR'")
    check(m.r16(REG_EXMEMCNT) & 0x0880 == 0, 'both slots given to the arm9 (EXMEMCNT)')
    check(all(m.r32(c) == 0 for c in DMA_CNT), 'all DMA channels stopped')
    check(m.r16(SLOT1_LOCK + 4) == 0x7D, 'slot 1 lock taken (owner 0x7D)')
    check(m.r16(REG_MASTER_BRIGHT) == 0x4008, 'main screen brightened (hotkey recognized)')

    # 3. released early: count restarts
    m = Machine(sections, symbols)
    m.press(COMBO)
    for _ in range(100):
        m.dispatch(0)
    m.press(0)
    m.dispatch(0)
    check(held(m) == 0, 'releasing the keys resets the count')

    # 4. incomplete combos never reset
    for name, keys in (('L+R+Down', KEY_L | KEY_R | KEY_DOWN), ('L+R+B', KEY_L | KEY_R | KEY_B),
                       ('R+Down+B', KEY_R | KEY_DOWN | KEY_B)):
        m = Machine(sections, symbols)
        m.press(keys)
        r = [m.dispatch(0) for _ in range(200)]
        check(all(x == ('handler', 0) for x in r) and m.r32(RESET_PARAM) == 0, f'{name} never resets')

    # 5. extra keys (A as well) still count
    m = Machine(sections, symbols)
    m.press(COMBO | KEY_A)
    r = [m.dispatch(0) for _ in range(121)]
    check(r[-1] == ('reset', None), 'L+R+Down+B+A also resets')

    # 6. slot 1 lock held by the game: wait, keep running, try again
    m = Machine(sections, symbols)
    m.w16(SLOT1_LOCK + 4, 0x42)
    m.w32(SLOT1_LOCK, 0x42)
    m.press(COMBO)
    r = [m.dispatch(0) for _ in range(125)]
    check(all(x == ('handler', 0) for x in r), 'lock held: game keeps running, handler still entered')
    check(m.r32(RESET_PARAM) == 0, 'lock held: no reset parameter written')
    check(m.r16(REG_MASTER_BRIGHT) == 0x4008, 'lock held: screen brightened while waiting')
    m.w16(SLOT1_LOCK + 4, 0)
    m.w32(SLOT1_LOCK, 0)
    check(m.dispatch(0) == ('reset', None), 'lock released: reset on the next vblank')

    print()
    if FAILURES:
        print(f'{len(FAILURES)} check(s) failed')
        sys.exit(1)
    print('all checks passed')


if __name__ == '__main__':
    main()
