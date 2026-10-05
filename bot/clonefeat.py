"""Window features for the behavioural clone of the rank-1 bot.

Shared by tools/clone_extract.py (replay side) and the bot, so both compute identical
numbers. Inputs are only what one dragon can know: its 7x7 window, its own body, the
team's unit count and the static bundled map.
"""

INF = 99
NDIR = 23
NGLOB = 14


class MapCtx:
    def __init__(self, m):
        self.W, self.H = m['w'], m['h']
        self.N = self.W * self.H
        self.walls = m['walls']
        self.beds = m['beds']
        self.rate = [0.0] + [2.0 / (a + b) if a + b else 0.0 for a, b in m['bedtypes'][1:]]
        self.fast = m.get('fast')
        self.portal = {}
        for c, d, dest in m['portals']:
            self.portal[c * 4 + d] = dest
        self._nb = [None] * self.N

    def nb(self, c):
        t = self._nb[c]
        if t is None:
            W, H = self.W, self.H
            x, y = c % W, c // W
            geo = (((y - 1) % H) * W + x, y * W + (x + 1) % W, ((y + 1) % H) * W + x, y * W + (x - 1) % W)
            wl = self.walls[c]
            t = tuple(-1 if (wl >> d) & 1 else self.portal.get(c * 4 + d, geo[d]) for d in range(4))
            self._nb[c] = t
        return t


class State:
    """others: list of (id, is_ally, [cells head first]) for every other dragon (clipped to the window here)."""

    def __init__(self, ctx, rnd, me, body, others, pearls, n_team, is_queen):
        self.ctx = ctx
        self.rnd = rnd
        self.me = me
        self.body = body
        self.n_team = n_team
        self.is_queen = is_queen
        W, H = ctx.W, ctx.H
        head = body[0]
        hx, hy = head % W, head // W
        win = set()
        for dy in range(-3, 4):
            yy = ((hy + dy) % H) * W
            for dx in range(-3, 4):
                win.add(yy + (hx + dx) % W)
        self.win = win
        self.pearls = [c for c in pearls if c in win]
        occ = {}
        heads = []
        for did, ally, cells in others:
            vis = [c for c in cells if c in win]
            if not vis:
                continue
            for c in vis:
                occ[c] = did
            if cells[0] in win:
                heads.append((cells[0], did, ally, len(vis)))
        for c in body:
            occ[c] = me
        self.occ = occ
        self.heads = heads


def tdist(W, H, a, b):
    dx = abs(a % W - b % W)
    dy = abs(a // W - b // W)
    return min(dx, W - dx) + min(dy, H - dy)


def features(st):
    ctx = st.ctx
    W, H = ctx.W, ctx.H
    nb = ctx.nb
    body = st.body
    head = body[0]
    L = len(body)
    occ = st.occ
    win = st.win
    pearls = set(st.pearls)
    own = {c: i for i, c in enumerate(body)}
    hn = nb(head)
    back = -1
    if L > 1:
        for d in range(4):
            if hn[d] == body[1]:
                back = d
    facing = (back + 2) % 4 if back >= 0 else 0
    enemies = [(h, did, n) for h, did, ally, n in st.heads if not ally]
    allies = [(h, did, n) for h, did, ally, n in st.heads if ally]
    queen_h = None
    enemy_q = None
    for h, did, ally, n in st.heads:
        if did in (0, 1):
            if ally:
                queen_h = h
            else:
                enemy_q = h
    enemy_heads = set(h for h, _, _ in enemies)

    def free_at(c):
        i = own.get(c)
        if i is not None:
            return L + 1 - i
        return INF if c in occ else 0

    def space(start, cap=20):
        seen = {start}
        q = [(start, 1)]
        i = 0
        while i < len(q) and len(seen) < cap:
            c, t = q[i]
            i += 1
            for n in nb(c):
                if n >= 0 and n not in seen and free_at(n) <= t + 1:
                    seen.add(n)
                    q.append((n, t + 1))
        return len(seen)

    def pearl_dist(start):
        if start in pearls:
            return 0, 1
        seen = {start: 0}
        q = [start]
        i = 0
        best = 10
        near = 0
        while i < len(q):
            c = q[i]
            i += 1
            dc = seen[c]
            if dc >= 9:
                break
            for n in nb(c):
                if n >= 0 and n not in seen and n in win and (n not in occ):
                    seen[n] = dc + 1
                    q.append(n)
                    if n in pearls:
                        if dc + 1 < best:
                            best = dc + 1
                        if dc + 1 <= 3:
                            near += 1
        return best, near

    fast = ctx.fast
    out = []
    for d in range(4):
        n = hn[d]
        f = [0.0] * NDIR
        if n < 0:
            f[1] = 1.0
            f[10] = 0.0
            f[11] = f[12] = 8.0
            f[8] = 10.0
            out.append(f)
            continue
        legal = n not in occ
        f[0] = 1.0 if legal else 0.0
        f[2] = 1.0 if d == facing else 0.0
        f[3] = 1.0 if d == (facing + 3) % 4 else 0.0
        f[4] = 1.0 if d == (facing + 1) % 4 else 0.0
        f[5] = 1.0 if d == back else 0.0
        x, y = head % W, head // W
        plain = (((y - 1) % H) * W + x, y * W + (x + 1) % W, ((y + 1) % H) * W + x, y * W + (x - 1) % W)[d]
        f[6] = 1.0 if n != plain else 0.0
        f[7] = 1.0 if n in pearls else 0.0
        if legal:
            pd, near = pearl_dist(n)
            f[8] = pd
            f[9] = near
            f[10] = space(n)
        else:
            f[8] = 10.0
        f[11] = min([tdist(W, H, n, h) for h, _, _ in enemies] + [8])
        f[12] = min([tdist(W, H, n, h) for h, _, _ in allies] + [8])
        adj_e = [ln for h, _, ln in enemies if n in nb(h)]
        f[13] = 1.0 if adj_e else 0.0
        f[14] = max(adj_e) if adj_e else 0.0
        f[15] = 1.0 if n in enemy_heads else 0.0
        f[16] = 1.0 if n == enemy_q else 0.0
        f[17] = ctx.rate[ctx.beds[n]]
        if fast is not None:
            f[18] = min(fast[n], 60) - min(fast[head], 60)
        if queen_h is not None and not st.is_queen:
            f[19] = tdist(W, H, n, queen_h) - tdist(W, H, head, queen_h)
        f[20] = sum(1 for m in nb(n) if m >= 0 and m not in occ)
        f[21] = 1.0 if n in win else 0.0
        f[22] = 1.0 if any(n in nb(h) for h, _, _ in allies) else 0.0
        out.append(f)

    g = [0.0] * NGLOB
    g[0] = L
    g[1] = st.rnd
    g[2] = st.n_team
    g[3] = 1.0 if st.is_queen else 0.0
    g[4] = len(enemies)
    g[5] = len(allies)
    g[6] = sum(1 for d in range(4) if out[d][0])
    g[7] = tdist(W, H, head, queen_h) if queen_h is not None and not st.is_queen else 20
    g[8] = 1.0 if enemy_q is not None else 0.0
    big = 0
    bigd = 20
    for h, did, ln in allies:
        dd = tdist(W, H, head, h)
        if dd <= 4 and ln > big:
            big, bigd = ln, dd
    g[9] = big
    g[10] = bigd
    g[11] = min([tdist(W, H, head, h) for h, _, _ in enemies] + [8])
    g[12] = len(st.pearls)
    g[13] = (L + 3) // 4
    return out, g
