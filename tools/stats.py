import sys, glob, os
from collections import Counter, defaultdict
from sim import Game

def stats(path):
    g = Game(path)
    snap = {}
    acts = defaultdict(Counter)
    deaths = defaultdict(Counter)
    sonar = Counter()
    splits = defaultdict(list)
    eat = Counter()
    cur = {'turn': None, 'rnd': 0}
    def cb(g, e, rnd):
        k = e[0]
        if k == 'round':
            if rnd in (50, 100, 150, 200, 250, 300, 350, 400, 450, 499):
                s = {}
                for t in 'AB':
                    ls = [len(b) for i, b in g.alive.items() if g.team[i] == t]
                    s[t] = (len(ls), sum(ls), max(ls) if ls else 0)
                snap[rnd] = s
        elif k == 'action':
            t = g.team[e[1]]
            a = e[2]
            if a is None: acts[t]['none'] += 1
            elif a[0] == 'move': acts[t]['move%d' % len(a[1])] += 1
            else: acts[t][a[0]] += 1
            if a and a[0] == 'split':
                splits[t].append((rnd, a[1], len(g.alive.get(e[1], []))))
        elif k == 'death':
            deaths[g.team[e[1]]][e[2]] += 1
        elif k == 'sonar':
            sonar[g.team[e[1]]] += 1
    g.run(cb)
    return g, snap, acts, deaths, sonar, splits

if __name__ == '__main__':
    for p in sys.argv[1:]:
        g, snap, acts, deaths, sonar, splits = stats(p)
        print('=' * 20, os.path.basename(p), g.info['name'], g.info['w'], g.info['h'], 'limit', g.info.get('unit_limit'), 'winner', g.r['result']['winner'])
        for rr in sorted(snap): print('  r%3d' % rr, ' A n=%2d tot=%3d max=%3d' % snap[rr]['A'], ' | B n=%2d tot=%3d max=%3d' % snap[rr]['B'])
        for t in 'AB':
            print('  ', t, dict(acts[t]), 'deaths', dict(deaths[t]), 'sonar', sonar[t])
        print('   final', g.r['result']['A'], g.r['result']['B'])
