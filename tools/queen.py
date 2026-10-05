from sim import Game
import glob, sys
for f in sys.argv[1:]:
    g = Game(f)
    Q = {g.team[0]: 0, g.team[1]: 1}
    info = {'A': [], 'B': []}
    deaths = {}
    last = {}
    def cb(g, e, rnd):
        if e[0] == 'round' and rnd % 50 == 0:
            for t, q in Q.items():
                info[t].append(len(g.alive[q]) if q in g.alive else 0)
        if e[0] == 'action' and e[1] in (0, 1): last[e[1]] = e[2]
        if e[0] == 'death' and e[1] in (0, 1): deaths[g.team[e[1]]] = (rnd, e[2], last.get(e[1]))
    g.run(cb)
    res = g.r['result']
    print(f.split('/')[-1], g.info['name'][:14].ljust(14), 'W', res['winner'], 'A', info['A'], deaths.get('A'), '| B', info['B'], deaths.get('B'), '| q', res['A']['queen'], res['B']['queen'])
