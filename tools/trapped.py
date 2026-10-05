from sim import Game
from geom import Geo
import glob, sys
from collections import Counter
res = Counter()
for f in sys.argv[1:]:
    g = Game(f); geo = Geo(g.r['map'])
    pearls = set()
    def cb(g, e, rnd):
        if e[0] == 'tile':
            (pearls.add if e[2] else pearls.discard)(e[1])
        if e[0] == 'action':
            i = e[1]; b = g.alive.get(i)
            if not b: return
            occ = {}
            for j, bb in g.alive.items():
                for c in bb: occ[c] = j
            free = 0
            for d in 'NESW':
                t = geo.dest(b[0], d)
                if t is None: continue
                if t in occ:
                    if t == b[-1] and t not in pearls and len(b) > 2 and occ[t] == i: free += 1
                    continue
                free += 1
            a = e[2]
            kind = a[0] if a else None
            res[(g.team[i], kind, free)] += 1
    g.run(cb)
for t in 'AB':
    for k in ('move', 'split', 'suicide'):
        print(t, k, [(f, res[(t, k, f)]) for f in range(5)])
