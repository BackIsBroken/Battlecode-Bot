from sim import Game
from geom import Geo
import sys, glob
from collections import Counter, defaultdict
for f in sys.argv[1:]:
    g = Game(f); geo = Geo(g.r['map'])
    T = 'B'
    q = 0 if g.team[0] == T else 1
    def mind(a, b):
        dx = abs(a[0]-b[0]); dy = abs(a[1]-b[1])
        return min(dx, geo.W-dx) + min(dy, geo.H-dy)
    drops = {}   # pearl cell -> (round, source kind)
    pend = {}
    eaten = Counter()
    qsplits = []
    sui = defaultdict(Counter)
    lastlen = {}
    def cb(g, e, rnd):
        if e[0] == 'action' and g.team[e[1]] == T:
            if e[2] == ('suicide',):
                qd = mind(g.alive[e[1]][0], g.alive[q][0]) if q in g.alive else 99
                sui[rnd // 100 * 100][min(qd, 10)] += 1
                pend['src'] = ('sui', e[1])
            if e[1] == q and e[2] and e[2][0] == 'split': qsplits.append((rnd, e[2][1], len(g.alive[q])))
        if e[0] == 'death':
            pend['src'] = pend.get('src', ('death', e[1]))
            pend['dead'] = True
        if e[0] == 'tile' and e[2] and pend.get('dead'):
            drops[e[1]] = pend['src'][0]
        if e[0] in ('turn', 'round'): pend.clear()
        if e[0] == 'upd' and e[1] == q:
            if e[3] in drops: eaten[drops.pop(e[3])] += 1
    g.run(cb)
    print(f.split('/')[-1], g.info['name'][:12], 'queen splits', qsplits[:12], len(qsplits))
    print('   queen ate drops', dict(eaten))
    for r in sorted(sui): print('   r%d suicides by qdist' % r, sorted(sui[r].items()))
