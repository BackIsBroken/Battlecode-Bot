from sim import Game
from geom import Geo
import sys
def render(g, geo, center, R=5, mark=None):
    occ = {}
    for j, b in g.alive.items():
        for k, c in enumerate(b):
            ch = ('A' if g.team[j] == 'A' else 'B') if k == 0 else ('a' if g.team[j] == 'A' else 'b')
            occ[c] = ch
    cx, cy = center
    lines = []
    for dy in range(-R, R + 1):
        row = ''
        for dx in range(-R, R + 1):
            x, y = (cx + dx) % geo.W, (cy + dy) % geo.H
            ch = occ.get((x, y), '.')
            if (x, y) == mark: ch = '@' if ch == '.' else ch.upper() if ch.islower() else '*'
            wall_e = geo.dest((x, y), 'E') is None
            row += ch + ('|' if wall_e else ' ')
        lines.append(row)
        # south walls
        srow = ''
        for dx in range(-R, R + 1):
            x, y = (cx + dx) % geo.W, (cy + dy) % geo.H
            srow += ('-' if geo.dest((x, y), 'S') is None else ' ') + ' '
        lines.append(srow)
    return '\n'.join(lines)
if __name__ == '__main__':
    f, team, n = sys.argv[1], sys.argv[2], int(sys.argv[3])
    g = Game(f); geo = Geo(g.r['map'])
    cnt = [0]
    def cb(g, e, rnd):
        if e[0] == 'action' and g.team[e[1]] == team and e[2] == ('suicide',) and cnt[0] < n and rnd > 60:
            cnt[0] += 1
            b = g.alive[e[1]]
            print('round', rnd, 'id', e[1], 'len', len(b), 'head', b[0])
            print(render(g, geo, b[0], 4, b[0]))
    g.run(cb)
