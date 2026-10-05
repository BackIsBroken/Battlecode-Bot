"""Map geometry: kelp, portals, wraparound."""
DIRS = 'NESW'
DXY = {'N': (0, -1), 'E': (1, 0), 'S': (0, 1), 'W': (-1, 0)}

class Geo:
    def __init__(self, maptext):
        self.edges = {}  # canonical key -> (type, portal id)
        portals = {}
        self.beds = {}
        for l in maptext.split('\n'):
            p = l.split()
            if not p: continue
            if p[0] == 'MAP': self.W, self.H = int(p[1]), int(p[2])
            elif p[0] == 'TILE':
                x, y, a, b = map(int, p[1:5])
                if a or b: self.beds[(x, y)] = (a, b)
            elif p[0] == 'EDGE':
                e, t, pid = int(p[1]), int(p[2]), int(p[3])
                r = self.W + 1
                i, a = divmod(e, r)
                if i % 2 == 0: key = ('H', a, i // 2)
                else: key = ('V', a, (i - 1) // 2)
                key = self.canon(key)
                if t == 2:
                    portals.setdefault(pid, []).append(key)
                    self.edges[key] = (2, pid)
                elif t == 1:
                    self.edges[key] = (1, -1)
        self.partner = {}
        for pid, ks in portals.items():
            if len(ks) == 2:
                self.partner[ks[0]] = ks[1]; self.partner[ks[1]] = ks[0]
        self._dest = {}

    def canon(self, k):
        t, x, y = k
        if t == 'H' and y == self.H: return ('H', x, 0)
        if t == 'V' and x == self.W: return ('V', 0, y)
        return k

    def edge_key(self, x, y, d):
        if d == 'N': return self.canon(('H', x, y))
        if d == 'S': return self.canon(('H', x, (y + 1) % self.H))
        if d == 'W': return self.canon(('V', x, y))
        return self.canon(('V', (x + 1) % self.W, y))

    def dest(self, c, d):
        k = (c, d)
        if k in self._dest: return self._dest[k]
        x, y = c
        ek = self.edge_key(x, y, d)
        t = self.edges.get(ek, (0, -1))[0]
        if t == 1: r = None
        elif t == 2 and ek in self.partner:
            pk = self.partner[ek]
            pt, px, py = pk
            if pt == 'V' and d in 'EW':
                r = (px if d == 'E' else px - 1, py)
            elif pt == 'H' and d in 'NS':
                r = (px, py if d == 'S' else py - 1)
            else:
                r = (px, py)
            r = (r[0] % self.W, r[1] % self.H)
        else:
            dx, dy = DXY[d]
            r = ((x + dx) % self.W, (y + dy) % self.H)
        self._dest[k] = r
        return r
