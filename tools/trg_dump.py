"""Disassemble a THPS1 level trigger file (.TRG): its nodes, their trigger command lists and the
object scripts of their script-only baddies and platforms.

    python tools/trg_dump.py <file.trg> [...]      listing on stdout
    python tools/trg_dump.py --check <file.trg> ... decode only: report unknown opcodes, desyncs and
                                                    per-opcode use counts

Opcode names come from game/include/game/script.h (one `NAME = 0x...,` per line); operand formats
are below, mirroring the handlers (CBaddy/CPlatform/CScriptOnlyBaddy::ExecuteCommand, SetVariable,
GetVariable, ExecuteCommandList). A clean --check over every level is the test that the formats
(and so the names' operand comments) are right: one wrong length desyncs the rest of the script.
"""
import collections
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
HEADER = os.path.join(HERE, '..', 'game', 'include', 'game', 'script.h')


def load_names():
    names = {}
    for m in re.finditer(r'\b((NODE|OBJ|C|V|TRG)_\w+) = (0x[0-9a-fA-F]+|\d+)',
                         open(HEADER).read()):
        names.setdefault(m.group(2), {})[int(m.group(3), 0)] = m.group(1)
    return names


NAMES = load_names()

# operand formats: w word, v word-or-variable (0x2000), V GetScriptValue (0x2000 and not 0x8000),
# b low byte of a word, i aligned int, s string, S string list, B box list, ? special-cased
SCMD_FMT = {
    'C_GOTO': 'w', 'C_GOTO_BREAK': 'w', 'C_LABEL': 'w', 'C_SET_SCRIPT_NODE': 'v',
    'C_ADD': '?', 'C_SUB': '?', 'C_IF_GT': 'vv', 'C_IF_LT': 'vv',
    'C_IF_EQ': 'vv', 'C_IF_FLAG_SET': 'w', 'C_IF_FLAG_CLEAR': 'w', 'C_WAIT': 'v',
    'C_INIT_ITEM': 's', 'C_CYCLE_ANIM': 'ww', 'C_RUN_ANIM': 'w',
    'C_MOVE_TO_ABSOLUTE': 'iii', 'C_MOVE_TO_NODE': 'v', 'C_MOVE_TO_RELATIVE': 'iii',
    'C_FOLLOW_COMPLEX_SURFACE': 'w', 'C_PLAY_SFX': 'w', 'C_PLAY_POSITIONAL_SFX': 'w', 'C_SPARK': 'w',
    'C_SET_FMV_CHECKSUM': 'i', 'C_MIDI_FADE_IN': 'w', 'C_MIDI_FADE_OUT': 'w',
    'C_SHAKE_CAMERA': 'v', 'C_SET_FMV_TRACK': 'w', 'C_EXPLODE_NODE': '?',
    'C_SMOKEJET_ON': 'wwwwww', 'C_FLASH_SCREEN': 'wwwww', 'C_MAKE_POWERUP': 'vwww',
    'C_FIREJET_ON': 'wwwwww', 'C_SET_WATER_DAMAGE': 'w', 'C_EXPLODE': '?',
    'C_TEXT_MESSAGE': 's', 'C_SEND_SIGNAL_TO_LINKS': 'v', 'C_SEND_PULSE_TO_LINKS': 'v', 'C_SEND_SIGNAL_TO_NODE': 'v',
    'C_SEND_PULSE_TO_NODE': 'v', 'C_GOALCOUNTER': 'w', 'C_BOUNDING_BOX_CHECK_ON': 'www',
    'C_DAMAGE_BRUCE': 'w', 'C_SCALE': 'ww', 'C_WATER_CLIP': 'w',
    'C_SCALE_X': 'ww', 'C_SCALE_Y': 'ww', 'C_SCALE_Z': 'ww', 'C_IS_BOUNCY': 'w',
    'C_MAKE_RAIN': 'V', 'C_MAKE_ROCK': 'wwwwwwwwww', 'C_PLAY_LOOPING_SFX': 'w',
    'C_PLAY_LOOPING_POSITIONAL_SFX': 'ww', 'C_BUBBLES': 'w',
}
# SetVariable operands (a variable as a statement)
SVAR_SET_FMT = {
    'V_HEALTH': 'w', 'V_COLLISION_RADIUS': 'w', 'V_PATTERN': 'w', 'V_SUBPATTERN': 'w',
    'V_REGISTER': 'bV', 'V_ANIM_SPEED': 'w', 'V_GROUND_TO_ORIGIN': 'w',
    'V_APPLY_GRAVITY': 'b', 'V_MODEL': 'w', 'V_ATTRIBUTE': 'ww',
    'V_ANGULAR_VELOCITY': 'VVV', 'V_ANGULAR_ACCELERATION': 'VVV', 'V_INPUT_SIGNAL': 'b', 'V_INPUT_HIT': 'b',
    'V_VERTICAL_COMPLIANCE': 'w', 'V_MODEL_CHECKSUM': 'i', 'V_ALLOW_WATER_CLIP': 'w',
    'V_VELOCITY': 'VVV', 'V_DEAD_CHECKSUM': 'i', 'V_ANGLES': 'VVV',
    'V_MY_X_POS': 'V', 'V_MY_Y_POS': 'V', 'V_MY_Z_POS': 'V',
}
# GetVariable operands (a variable read as an operand)
SVAR_GET_FMT = {'V_RANDOM': 'w', 'V_REGISTER': 'b', 'V_LINKED_NODE': 'v'}

TCMD_FMT = {
    'TRG_SETCHEATRESTARTS': 'S', 'TRG_SENDVISIBLE': 'w', 'TRG_SETFOGGINGPARAMS': 'www',
    'TRG_PLAYSOUND': 'w', 'TRG_STOPSOUND': 'w', 'TRG_DEBUGTEXT': 's', 'TRG_SPOOLIN': 's',
    'TRG_SPOOLOUT': 's', 'TRG_SPOOLENV': 's', 'TRG_SETCAMANGLE': 'ww', 'TRG_BACKGROUNDON': 'w',
    'TRG_BACKGROUNDOFF': 'w', 'TRG_KILLEVERYTHINGINBOX': 'B', 'TRG_SETINITIALPULSES': 'w',
    'TRG_SETCAMDISTXZ': 'ww', 'TRG_SEEKXA': 'w', 'TRG_PLAYXA': 'ww',
    'TRG_SETRESTART': 's', 'TRG_SETVISIBILITYINBOX': 'wwB', 'TRG_SETOBJFILE': 's',
    'TRG_SETCAMDISTY': 'ww', 'TRG_SETCAMOFFSETX': 'ww', 'TRG_SETCAMOFFSETY': 'ww',
    'TRG_SETCAMOFFSETZ': 'ww', 'TRG_SETGAMELEVEL': 'w', 'TRG_IFPULSECOUNT': 'w',
    'TRG_MIDICHANNELMUTE': 'w', 'TRG_SETDUALBUFFERSIZE': 'w', 'TRG_SETCAMCOLIJSIDE': 'w',
    'TRG_SETCAMCOLIJBACK': 'w', 'TRG_MIDIFADEIN': 'w', 'TRG_MIDIFADEOUT': 'w',
    'TRG_SETREVERBTYPE': 'w', 'TRG_SPOOLMIDI': 's', 'TRG_SETCAMMODE': 'w',
    'TRG_SETCAMCOMPLEXMODE': 'w', 'TRG_LOADAI': 's', 'TRG_IGNOREBRUCEINPUT': 'w',
    'TRG_SETCAMCOLIJANGLESIDE': 'w', 'TRG_SETCAMCOLIJANGLEBACK': 'w', 'TRG_SETOTPUSHBACK': 'w',
    'TRG_SETCAMZOOM': 'ww', 'TRG_SETCAMPITCHDAMP': 'w', 'TRG_SETOTPUSHBACK2': 'w',
    'TRG_SETSUSPENDDISTANCE': 'w', 'TRG_BACKGROUNDCREATE': 'iwww', 'TRG_SETCAMYDAMP': 'w',
    'TRG_SETDROPDAMAGEON': 'ww', 'TRG_SETRESTARTNOMESSAGE': 's', 'TRG_IGNOREBRUCEINPUTFREEZE': 'w',
    'TRG_SETRESTART2': 's', 'TRG_SETFADECOLOR': 'ww', 'TRG_GAPPOLYHIT': 'iw', 'TRG_SETSKYCOLOR': 'ww',
}


class DecodeError(Exception):
    pass


class Reader:
    def __init__(self, data, pos, end):
        self.d, self.p, self.end = data, pos, end

    def word(self):
        if self.p + 2 > self.end:
            raise DecodeError('ran into the next node')
        v = struct.unpack_from('<H', self.d, self.p)[0]
        self.p += 2
        return v

    def peek(self):
        if self.p + 2 > self.end:
            raise DecodeError('ran into the next node')
        return struct.unpack_from('<H', self.d, self.p)[0]

    def align4(self):
        self.p = (self.p + 3) & ~3

    def int32(self):
        self.align4()
        if self.p + 4 > self.end:
            raise DecodeError('ran into the next node')
        v = struct.unpack_from('<i', self.d, self.p)[0]
        self.p += 4
        return v

    def string(self):
        end = self.d.index(b'\0', self.p)
        s = self.d[self.p:end].decode('latin-1')
        self.p = end + 1
        self.p += self.p & 1
        return s


def var_name(v):
    return NAMES['V'].get(v, 'V_%04X' % v)


def read_var_operands(r, v, stats):
    """A variable read as an operand: its name plus whatever GetVariable consumes for it."""
    name = var_name(v)
    stats['SVAR ' + name] += 1
    args = [read_operand(r, c, stats) for c in SVAR_GET_FMT.get(name, '')]
    return name + ('(%s)' % ', '.join(args) if args else '')


def read_operand(r, c, stats):
    if c == 'w':
        return str(r.word())
    if c == 'b':
        return str(r.word() & 0xff)
    if c in 'vV':
        v = r.word()
        if v & 0x2000 and (c == 'v' or not v & 0x8000):
            return read_var_operands(r, v, stats)
        return str(v if c == 'v' or not v & 0x8000 else v - 0x10000)
    if c == 'i':
        v = r.int32()
        return '0x%08x' % (v & 0xffffffff)
    if c == 's':
        return '"%s"' % r.string()
    if c == 'S':
        names = []
        while r.d[r.p] != 0:
            names.append('"%s"' % r.string())
        r.word()
        return '[%s]' % ', '.join(names)
    if c == 'B':
        boxes = []
        while r.peek() != 0xff:
            boxes.append('(%s)' % ', '.join(str(r.int32()) for _ in range(6)))
        r.word()
        return '[%s]' % ', '.join(boxes)
    raise ValueError(c)


def decode_object_script(r, stats):
    """Yields (offset, depth, text) per statement, until C_DONE (or the next node: a script that
    only loops need not have one)."""
    depth = 0
    while True:
        at = r.p
        if at == r.end:
            yield at, depth, '; (no C_DONE: the next node follows)'
            return
        op = r.word()
        if op == 0x4100:
            yield at, depth, 'C_DONE'
            if r.end - r.p < 4:
                return
            # a script disabled by an C_DONE put in front of it: the rest is never run
            yield r.p, 0, '; (unreachable: after C_DONE)'
            depth = 0
            continue
        if op & 0x4000:
            name = NAMES['C'].get(op)
            if name is None:
                raise DecodeError('unknown command 0x%04x at 0x%x' % (op, at))
            stats[name] += 1
            if name in ('C_ADD', 'C_SUB'):
                args = [read_var_operands(r, r.word(), stats), read_operand(r, 'v', stats)]
            elif name in ('C_EXPLODE_NODE', 'C_EXPLODE'):
                args = [read_operand(r, 'v', stats)] if name == 'C_EXPLODE_NODE' else []
                kind = r.word()
                args.append(str(kind))
                if kind == 2:
                    args += [read_operand(r, 'V', stats), str(r.word()), str(r.word())]
            else:
                args = [read_operand(r, c, stats) for c in SCMD_FMT.get(name, '')]
            if name == 'C_ENDIF':
                depth = max(depth - 1, 0)
            yield at, depth, name + (' ' + ', '.join(args) if args else '')
            if name.startswith('C_IF_'):
                depth += 1
        elif op & 0x2000:
            name = var_name(op)
            if name not in SVAR_SET_FMT:
                raise DecodeError('unknown/unsettable variable 0x%04x at 0x%x' % (op, at))
            stats['set ' + name] += 1
            args = [read_operand(r, c, stats) for c in SVAR_SET_FMT[name]]
            yield at, depth, 'set %s = %s' % (name, ', '.join(args))
        else:
            stats['(skipped word)'] += 1
            yield at, depth, '.word 0x%04x  ; skipped by ParseScript' % op


def decode_command_list(r, stats):
    depth = 0
    while True:
        at = r.p
        op = r.word()
        if op == 0xffff:
            yield at, depth, 'TRG_ENDCOMMANDLIST'
            return
        name = NAMES['TRG'].get(op)
        if name is None:
            raise DecodeError('unknown trigger command 0x%04x at 0x%x' % (op, at))
        stats[name] += 1
        args = [read_operand(r, c, stats) for c in TCMD_FMT.get(name, '')]
        if name == 'TRG_ENDIF':
            depth = max(depth - 1, 0)
        yield at, depth, name + (' ' + ', '.join(args) if args else '')
        if name == 'TRG_IFPULSECOUNT':
            depth += 1


def links(r):
    n = r.word()
    return [r.word() for _ in range(n)]


def parse_trg(data):
    if data[:4] != b'_TRG':
        raise DecodeError('not a trigger file')
    n = struct.unpack_from('<H', data, 8)[0]
    return [struct.unpack_from('<I', data, 12 + 4 * i)[0] for i in range(n)]


def dump(path, out, stats, errors):
    data = open(path, 'rb').read()
    offsets = parse_trg(data)
    if out:
        out.write('; %s: %d nodes\n' % (path, len(offsets)))

    def emit(lines):
        for at, depth, text in lines:
            if out:
                out.write('    %06x  %s%s\n' % (at, '    ' * depth, text))

    ends = sorted(set(offsets + [len(data)]))
    for i, off in enumerate(offsets):
        end = ends[ends.index(off) + 1] if off != ends[-1] else len(data)
        r = Reader(data, off, end)
        t = r.word()
        tname = NAMES['NODE'].get(t, 'NODE_%d' % t)
        stats['node ' + tname] += 1
        head, body = '', None
        try:
            if t in (1, 7):
                otype, seed = r.word(), r.word()
                lk = links(r)
                flags = []
                while data[r.p] != 0xff:
                    flags.append(data[r.p])
                    r.p += 1
                r.p += 1
                pos = [r.int32() for _ in range(3)]
                ang = [struct.unpack('<h', struct.pack('<H', r.word()))[0] for _ in range(3)]
                oname = NAMES['OBJ'].get(otype, 'type %d' % otype)
                head = '%s seed %d links %s flags %s pos %s angles %s' % (oname, seed, lk, flags,
                                                                           pos, ang)
                if otype in (203, 402):
                    body = decode_object_script(r, stats)
            elif t in (4, 15):
                body = decode_command_list(r, stats)
            elif t in (2, 6, 9):
                lk = links(r)
                checksum = r.int32()
                head = 'links %s checksum 0x%08x' % (lk, checksum & 0xffffffff)
                if t == 6:
                    body = decode_command_list(r, stats)
            elif t == 8:
                lk = links(r)
                pos = [r.int32() for _ in range(3)]
                r.p += 6
                name = r.string()
                head = '"%s" links %s pos %s' % (name, lk, pos)
                body = decode_command_list(r, stats)
            elif t == 5:
                ptype = r.word()
                head = 'powerup %d links %s' % (ptype, links(r))
            elif t in (3, 10, 11, 12, 13, 14, 1000, 1001):
                head = 'links %s' % links(r)
            if out:
                out.write('\nnode %d @%06x %s %s\n' % (i, off, tname, head))
            if body:
                emit(body)
                # each node's data should end at the next one (bar alignment padding)
                if end - r.p >= 4 and t != 255:
                    raise DecodeError('%d bytes left before the next node' % (end - r.p))
        except DecodeError as e:
            errors.append('%s node %d (%s @%06x): %s' % (path, i, tname, off, e))
            if out:
                out.write('    !! %s\n' % e)


def main(argv):
    check = '--check' in argv
    paths = [a for a in argv if a != '--check']
    if not paths:
        print(__doc__)
        return 2
    stats, errors = collections.Counter(), []
    for p in paths:
        dump(p, None if check else sys.stdout, stats, errors)
    if check:
        for k, v in sorted(stats.items()):
            print('%6d  %s' % (v, k))
    for e in errors:
        print('ERROR', e, file=sys.stderr)
    if check:
        print('%d file(s), %d error(s)' % (len(paths), len(errors)), file=sys.stderr)
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
