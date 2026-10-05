"""Minimal decoder for UNSW Battlecode (.replay) files: packed Cap'n Proto."""
import struct, sys, pickle, os

def unpack(data: bytes) -> bytes:
    out = bytearray()
    i, n = 0, len(data)
    zero8 = bytes(8)
    while i < n:
        tag = data[i]; i += 1
        if tag == 0:
            cnt = data[i]; i += 1
            out += zero8 * (cnt + 1)
        elif tag == 0xff:
            out += data[i:i+8]; i += 8
            cnt = data[i]; i += 1
            out += data[i:i+cnt*8]; i += cnt*8
        else:
            w = bytearray(8)
            for b in range(8):
                if tag & (1 << b):
                    w[b] = data[i]; i += 1
            out += w
    return bytes(out)

class Msg:
    def __init__(self, raw):
        nseg = struct.unpack_from('<I', raw, 0)[0] + 1
        sizes = struct.unpack_from('<%dI' % nseg, raw, 4)
        off = 4 + 4 * nseg
        if off % 8: off += 4
        self.segs = []
        for s in sizes:
            self.segs.append(memoryview(raw)[off:off + s*8]); off += s*8

    def ptr(self, seg, word):
        """Resolve pointer at (seg, word). Returns (seg, kind, target_word, a, b)"""
        v = struct.unpack_from('<Q', self.segs[seg], word*8)[0]
        if v == 0: return None
        kind = v & 3
        if kind == 2:
            landing = (v >> 2) & 1
            off = (v >> 3) & ((1 << 29) - 1)
            sid = v >> 32
            assert not landing, 'double-far unsupported'
            # landing pad is a normal pointer whose offset is relative to pad
            v2 = struct.unpack_from('<Q', self.segs[sid], off*8)[0]
            return self._decode(sid, off, v2)
        return self._decode(seg, word, v)

    def _decode(self, seg, word, v):
        kind = v & 3
        lo = v & 0xffffffff
        o = lo >> 2
        if o & (1 << 29): o -= (1 << 30)
        tgt = word + 1 + o
        hi = v >> 32
        if kind == 0:
            return (seg, 0, tgt, hi & 0xffff, hi >> 16)  # data words, ptr count
        elif kind == 1:
            return (seg, 1, tgt, hi & 7, hi >> 3)  # elem size, count
        raise ValueError('bad ptr %d' % kind)

class S:
    __slots__ = ('m', 'seg', 'w', 'dw', 'pc')
    def __init__(self, m, seg, w, dw, pc):
        self.m, self.seg, self.w, self.dw, self.pc = m, seg, w, dw, pc
    def _raw(self, fmt, off):
        sz = struct.calcsize(fmt)
        if off + sz > self.dw * 8: return 0
        return struct.unpack_from(fmt, self.m.segs[self.seg], self.w*8 + off)[0]
    def i32(self, o): return self._raw('<i', o*4)
    def u32(self, o): return self._raw('<I', o*4)
    def u16(self, o): return self._raw('<H', o*2)
    def u8(self, o): return self._raw('<B', o)
    def u64(self, o): return self._raw('<Q', o*8)
    def bit(self, b):
        by = self._raw('<B', b // 8)
        return bool((by >> (b % 8)) & 1)
    def p(self, i):
        if i >= self.pc: return None
        return self.m.ptr(self.seg, self.w + self.dw + i)
    def struct(self, i):
        r = self.p(i)
        if r is None: return None
        seg, k, tgt, dw, pc = r
        return S(self.m, seg, tgt, dw, pc)
    def text(self, i):
        r = self.p(i)
        if r is None: return ''
        seg, k, tgt, es, cnt = r
        return bytes(self.m.segs[seg][tgt*8: tgt*8 + cnt - 1]).decode('utf-8', 'replace')
    def slist(self, i):
        r = self.p(i)
        if r is None: return []
        seg, k, tgt, es, cnt = r
        if es == 7:
            tag = struct.unpack_from('<Q', self.m.segs[seg], tgt*8)[0]
            n = (tag & 0xffffffff) >> 2
            dw = (tag >> 32) & 0xffff; pc = tag >> 48
            step = dw + pc
            return [S(self.m, seg, tgt + 1 + j*step, dw, pc) for j in range(n)]
        raise ValueError('non composite list es=%d' % es)
    def u16list(self, i):
        r = self.p(i)
        if r is None: return []
        seg, k, tgt, es, cnt = r
        assert es == 3, es
        return list(struct.unpack_from('<%dH' % cnt, self.m.segs[seg], tgt*8))

DIRS = 'NESW'
REASONS = ['wall', 'self', 'body', 'head', 'noaction']

def pt(s):
    return (s.i32(0), s.i32(1)) if s is not None else None

def decode_event(e):
    k = e.u16(0)
    x = e.struct(0)
    if k == 0: return ('round', x.i32(0))
    if k == 1: return ('turn', x.i32(0))
    if k == 2: return ('countdown', pt(x.struct(0)), x.i32(0))
    if k == 3: return ('tile', pt(x.struct(0)), x.bit(0))
    if k == 4:
        a = x.struct(0)
        ins = x.struct(1)
        act = None
        if a is not None:
            w = a.u16(0)
            if w == 0: act = ('move', ''.join(DIRS[d] for d in a.u16list(0)))
            elif w == 1: act = ('split', a.i32(1))
            else: act = ('suicide',)
        return ('action', x.i32(0), act, ins.u64(0) if ins else None, x.bit(32))
    if k == 5: return ('elog', x.i32(0), x.text(0))
    if k == 6: return ('log', x.i32(0), x.text(0))
    if k == 7: return ('ind', x.i32(0), x.text(0))
    if k == 8: return ('draw', x.i32(0))
    if k == 9: return ('upd', x.i32(0), DIRS[x.u16(2)], pt(x.struct(0)), pt(x.struct(1)))
    if k == 10:
        return ('split', x.i32(0), x.i32(1), 'AB'[x.u16(4)], DIRS[x.u16(5)],
                [pt(s) for s in x.slist(0)], [pt(s) for s in x.slist(1)])
    if k == 11: return ('death', x.i32(0), REASONS[x.u16(2)])
    if k == 12: return ('sonar', x.i32(0), DIRS[x.u16(2)], x.u64(2), pt(x.struct(0)), pt(x.struct(1)),
                        x.i32(3) if x.u16(3) == 1 else None)
    return ('unk', k)

def load(path, keep=('round','turn','tile','action','upd','split','death','sonar','log','ind')):
    cache = path + '.pkl'
    if os.path.exists(cache) and os.path.getmtime(cache) > os.path.getmtime(path):
        with open(cache, 'rb') as f: return pickle.load(f)
    raw = unpack(open(path, 'rb').read())
    m = Msg(raw)
    seg, k, tgt, dw, pc = m.ptr(0, 0)
    root = S(m, seg, tgt, dw, pc)
    r = {'map': root.text(0), 'botA': root.text(1), 'botB': root.text(2),
         'version': root.u32(0)}
    res = root.struct(4)
    def standing(s):
        return dict(dragons=s.i32(0), longest=s.i32(1), total=s.i32(2), queen=s.i32(3)) if s else None
    r['result'] = dict(terminated=res.bit(0), reason=res.u16(1),
                       winner=('AB'[res.u16(3)] if res.u16(2) == 1 else None),
                       A=standing(res.struct(0)), B=standing(res.struct(1)))
    evs = []
    for e in root.slist(3):
        d = decode_event(e)
        if d[0] in keep: evs.append(d)
    r['events'] = evs
    with open(cache, 'wb') as f: pickle.dump(r, f)
    return r

if __name__ == '__main__':
    for p in sys.argv[1:]:
        r = load(p)
        hdr = r['map'].split('\n')
        name = [l for l in hdr if l.startswith('MAP_NAME')]
        print(os.path.basename(p), name, r['botA'], r['botB'], r['result'], len(r['events']))
