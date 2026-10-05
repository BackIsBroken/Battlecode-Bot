"""Extract (window features, action) samples of one team from replays, for behavioural cloning.

Only information a dragon could see is used: its 7x7 window (bodies, heads, pearls)
plus static map knowledge (kelp, portals, pearl beds of the bundled map).
The feature code is shared with the bot through bot/clonefeat.py so both compute the same numbers.

usage: python3 tools/clone_extract.py OUT.npz TEAM REPLAY...   (TEAM 'auto' = the side that suicides)
"""
import os, sys, re
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', 'bot'))
from sim import Game
import clonefeat
import mapdb

ALIAS = {'around_unsw': 'unsw', 'prisoners_dilemma': 'dilemma'}


def bundled(name):
    k = re.sub('[^a-z]', '_', name.lower())
    return ALIAS.get(k, k)


def auto_team(g):
    team = dict(g.team)
    for e in g.r['events']:
        if e[0] == 'split':
            team[e[2]] = e[3]
        elif e[0] == 'action' and e[2] == ('suicide',):
            return team.get(e[1], 'B')
    return 'B'


def extract(path, team, stride=1):
    g = Game(path)
    if team == 'auto':
        team = auto_team(g)
    key = bundled(g.info['name'])
    M = mapdb.load(key)
    W, H = M['w'], M['h']
    ctx = clonefeat.MapCtx(M)
    pearls = set()
    rows_dir, rows_glob, labels, meta = [], [], [], []
    count = [0]
    queens = {g.team[0]: 0, g.team[1]: 1}

    def cb(g, e, rnd):
        if e[0] == 'tile':
            c = e[1][1] * W + e[1][0]
            (pearls.add if e[2] else pearls.discard)(c)
            return
        if e[0] != 'action' or g.team.get(e[1]) != team or e[2] is None:
            return
        count[0] += 1
        if count[0] % stride:
            return
        me = e[1]
        body = [y * W + x for x, y in g.alive[me]]
        head = body[0]
        others = []
        for j, b in g.alive.items():
            if j == me:
                continue
            others.append((j, g.team[j] == team, [y * W + x for x, y in b]))
        n_team = sum(1 for j in g.alive if g.team[j] == team)
        st = clonefeat.State(ctx, rnd, me, body, others, pearls, n_team, me == queens.get(team))
        fd, fg = clonefeat.features(st)
        a = e[2]
        if a[0] == 'move':
            lab = 'NESW'.index(a[1][0])
            steps = len(a[1])
        elif a[0] == 'split':
            lab = 4; steps = a[1]
        else:
            lab = 5; steps = 0
        rows_dir.append(fd); rows_glob.append(fg); labels.append(lab); meta.append((rnd, len(body), steps))

    g.run(cb)
    return np.array(rows_dir, np.float32), np.array(rows_glob, np.float32), np.array(labels, np.int8), np.array(meta, np.int16)


if __name__ == '__main__':
    out, team = sys.argv[1], sys.argv[2]
    stride = int(os.environ.get('STRIDE', '1'))
    D, G, Y, Mt, Gid = [], [], [], [], []
    for i, p in enumerate(sys.argv[3:]):
        d, gl, y, m = extract(p, team, stride)
        if len(y):
            D.append(d); G.append(gl); Y.append(y); Mt.append(m); Gid.append(np.full(len(y), hash(os.path.basename(p)) % 100000, np.int32))
    np.savez_compressed(out, D=np.concatenate(D), G=np.concatenate(G), Y=np.concatenate(Y),
                        M=np.concatenate(Mt), gid=np.concatenate(Gid))
