"""Reconstruct per-round state from decoded replay events."""
import replay
from collections import defaultdict, Counter

def parse_map(text):
    info = {'dragons': [], 'tiles': {}}
    for l in text.split('\n'):
        p = l.split()
        if not p: continue
        if p[0] == 'MAP': info['w'], info['h'] = int(p[1]), int(p[2])
        elif p[0] == 'MAP_NAME': info['name'] = l[9:]
        elif p[0] == 'UNIT_LIMIT': info['unit_limit'] = int(p[1])
        elif p[0] == 'TILE':
            x, y, a, b = map(int, p[1:5])
            if a or b: info['tiles'][(x, y)] = (a, b)
        elif p[0] in ('DRAGON', 'SNAKE'):
            t = 'AB'[int(p[1])]; n = int(p[2]); c = list(map(int, p[3:]))
            info['dragons'].append((t, [(c[2*i], c[2*i+1]) for i in range(n)]))
    return info

class Game:
    def __init__(self, path):
        self.r = replay.load(path)
        self.info = parse_map(self.r['map'])
        self.team = {}
        self.body = {}
        for i, (t, b) in enumerate(self.info['dragons']):
            self.team[i] = t; self.body[i] = list(b)

    def run(self, cb=None):
        """Iterate events, maintaining bodies. cb(game, event, round) called after applying."""
        rnd = -1
        alive = dict(self.body)
        self.alive = alive
        for e in self.r['events']:
            k = e[0]
            if k == 'round':
                rnd = e[1]
            elif k == 'upd':
                _, i, f, head, tail = e
                b = alive.get(i)
                if b is None: continue
                if b[0] != head:   # the opening update repeats the spawn head; inserting it again made L one too long
                    b.insert(0, head)
                while len(b) > 1 and b[-1] != tail: b.pop()
            elif k == 'split':
                _, p, c, t, f, pb, cb_ = e
                alive[p] = list(pb); alive[c] = list(cb_); self.team[c] = t
            elif k == 'death':
                alive.pop(e[1], None)
            if cb: cb(self, e, rnd)
        return self
