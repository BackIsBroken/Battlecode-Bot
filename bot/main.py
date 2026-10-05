"""Swarm bot modelled on the rank-1 replays (see ANALYSIS.md).

Every dragon is its own process and only sees its 7x7 window, so all state
here is per dragon.  Plan per turn:
  1. read the window, identify the bundled map, remember pearls and timers;
  2. trapped -> split off the tail if that frees it, else suicide (no action);
  3. late game: short dragons next to a much longer ally suicide to feed it;
  4. length >= SPLIT_LEN -> split off a 2-segment child (queen stops later);
  5. otherwise move: time-aware BFS to the best pearl / bed, with danger and
     space checks, sprinting along the path when steps are free.
"""
import helper as unswbc
from helper import Direction
import mapdb

# ---------------------------------------------------------------- tuning
SPLIT_LEN = 4            # split as soon as a dragon reaches this length
QUEEN_SPLIT_UNTIL = 100  # the queen stops splitting from this round
FEED_FROM = 200          # short dragons start feeding longer allies
FEED_DIST = 3            # ... when their heads are this close
FEED_MAX_LEN = 3         # ... and they are at most this long
LATE_FEED_FROM = 440     # from here any dragon feeds a much longer ally
BFS_CAP = 260            # cells visited by the food search
FIRST_BFS_CAP = 120      # first turn of a process (interpreter start-up is expensive)
SPACE_CAP = 14           # flood-fill cap for the trap check

DIRS = (Direction.NORTH, Direction.EAST, Direction.SOUTH, Direction.WEST)
DX = (0, 1, 0, -1)
DY = (-1, 0, 1, 0)
OPP = (2, 3, 0, 1)
INF = 1 << 20
DEBUG = False

# ---------------------------------------------------------------- per-process state
W = H = N = 0
MAP = None               # bundled map dict once identified
CANDS = []               # bundled maps still consistent with what we saw
NB = None                # NB[c] -> (dest N, E, S, W), -1 = blocked
EDGE_SEEN = None         # unknown map: observed edge classes per cell
BED = None               # bed type index per cell (known map)
BEDRATE = None           # pearls per round for each bed type
PEARL = {}               # cell -> round a pearl was last seen there
DUE = {}                 # cell -> round its bed next tries to spawn
SEEN = None              # cell -> last round it was in view
HIST = []                # our head cells, oldest first
FIRST = True
MY_ID = -1
IS_QUEEN = False
MY_TEAM = 'A'
# shared over sonar: (cell, round, length)
OUR_Q = None             # where our queen was last reported
THEIR_Q = None           # where their queen was last seen
OUT = {}                 # this turn's broadcast state, filled by execute_turn
KEY = {'A': 0x5DEECE66D1F3A7B9, 'B': 0x2545F4914F6CDD1D}
MASK64 = (1 << 64) - 1
PAYLOAD_BITS = 34


def tag(payload):
    v = (payload ^ KEY[MY_TEAM] ^ (W << 40) ^ (H << 50)) & MASK64
    v = ((v ^ (v >> 31)) * 0x9E3779B97F4A7C15) & MASK64
    v = ((v ^ (v >> 29)) * 0xBF58476D1CE4E5B9) & MASK64
    return (v ^ (v >> 32)) & ((1 << (64 - PAYLOAD_BITS)) - 1)


def pack(kind, cell, rnd, length):
    payload = (kind & 3) | ((cell & 0x3FFF) << 2) | ((rnd & 511) << 16) | ((min(length, 511) & 511) << 25)
    return payload | (tag(payload) << PAYLOAD_BITS)


def unpack(msg):
    payload = msg & ((1 << PAYLOAD_BITS) - 1)
    if msg >> PAYLOAD_BITS != tag(payload):
        return None
    return payload & 3, (payload >> 2) & 0x3FFF, (payload >> 16) & 511, (payload >> 25) & 511


def read_sonar(ct, rnd):
    global OUR_Q, THEIR_Q
    msgs = ct.sonar_messages
    if len(msgs) > 24:
        msgs = msgs[-24:]
    for m in msgs:
        u = unpack(m)
        if u is None:
            continue
        kind, cell, r9, length = u
        r = rnd - ((rnd - r9) & 511)     # sent at most 511 rounds ago
        if cell >= N or r > rnd:
            continue
        if kind == 1 and (OUR_Q is None or r > OUR_Q[1]):
            OUR_Q = (cell, r, length)
        elif kind == 2 and (THEIR_Q is None or r > THEIR_Q[1]):
            THEIR_Q = (cell, r, length)


def broadcast(ct, rnd):
    if not OUT.get('ok'):
        return
    msgs = []
    if OUR_Q is not None and rnd - OUR_Q[1] <= 4:
        msgs.append(pack(1, OUR_Q[0], OUR_Q[1], OUR_Q[2]))
    if THEIR_Q is not None and rnd - THEIR_Q[1] <= 6:
        msgs.append(pack(2, THEIR_Q[0], THEIR_Q[1], THEIR_Q[2]))
    if not msgs:
        return
    back = OUT.get('back', -1)
    k = rnd
    for d in range(4):
        if d == back:
            continue
        ct.send_sonar(DIRS[d], msgs[k % len(msgs)])
        k += 1


def setup(w, h):
    global W, H, N, NB, SEEN, CANDS, EDGE_SEEN
    W, H, N = w, h, w * h
    NB = [None] * N
    SEEN = [-1000] * N
    EDGE_SEEN = [None] * N
    CANDS = []
    for key, size in mapdb.SIZES.items():
        if size == (w, h):
            try:
                CANDS.append((key, mapdb.load(key)))
            except Exception:
                pass


def activate(m):
    global MAP, BED, BEDRATE, NB
    MAP = m
    BED = m['beds']
    BEDRATE = [0.0] + [2.0 / (a + b) if a + b > 0 else 0.0 for a, b in m['bedtypes'][1:]]
    NB = [None] * N


def nb(c):
    t = NB[c]
    if t is None:
        x, y = c % W, c // W
        geo = (((y - 1) % H) * W + x, y * W + (x + 1) % W, ((y + 1) % H) * W + x, y * W + (x - 1) % W)
        if MAP is not None:
            wl = MAP['walls'][c]
            t = [geo[d] if not (wl >> d) & 1 else -1 for d in range(4)]
            if wl & 0xF0:
                for pc, pd, dest in MAP['portals']:
                    if pc == c:
                        t[pd] = dest
        else:
            e = EDGE_SEEN[c]
            t = list(geo)
            if e is not None:
                for d in range(4):
                    if e[d] != 0:
                        t[d] = -1      # kelp, or a portal we cannot follow on an unknown map
        t = tuple(t)
        NB[c] = t
    return t


def tok_class(tok):
    return 0 if tok == '.' else (1 if tok == 'w' else 2)


def identify(cells, edges, timers):
    """Drop bundled candidates that contradict the window."""
    global CANDS
    keep = []
    for key, m in CANDS:
        walls, beds = m['walls'], m['beds']
        ok = True
        for k in range(49):
            c = cells[k]
            wl = walls[c]
            e = edges[k]
            for d in range(4):
                cls = 1 if (wl >> d) & 1 else (2 if (wl >> (d + 4)) & 1 else 0)
                if cls != e[d]:
                    ok = False
                    break
            if not ok or ((timers[k] >= 0) != (beds[c] != 0)):
                ok = False
                break
        if ok:
            keep.append((key, m))
    CANDS = keep
    if len(keep) == 1:
        activate(keep[0][1])


def tdist(a, b):
    dx = abs(a % W - b % W)
    dy = abs(a // W - b // W)
    return min(dx, W - dx) + min(dy, H - dy)


def in_window(c, head):
    dx = (c % W - head % W) % W
    dy = (c // W - head // W) % H
    return (dx <= 3 or dx >= W - 3) and (dy <= 3 or dy >= H - 3)


def chain(parts, head):
    """Order a dragon's visible parts from the head; parts: cell -> facing."""
    prev = {}
    for p, f in parts.items():
        if p != head:
            n = nb(p)[f] if NB is not None else -1
            if n >= 0:
                prev[n] = p
    order = [head]
    cur = head
    seen = {head}
    while cur in prev:
        cur = prev[cur]
        if cur in seen:
            break
        seen.add(cur)
        order.append(cur)
    return order


def execute_turn(ct, game):
    global FIRST, HIST
    rnd = game.round_num
    vis = ct.vision
    L = ct.length
    my_team = ct.head.team.value

    # ------------------------------------------------ read the window
    tl = vis._tile_lines
    hl = "".join(vis._edge_lines[:8]).split()
    vl = "".join(vis._edge_lines[8:]).split()
    cells = [0] * 49
    timers = [0] * 49
    edges = [None] * 49
    pearls_now = []
    for k in range(49):
        x, y, hp, pt = tl[k].split()
        c = int(y) * W + int(x)
        cells[k] = c
        t = int(pt)
        timers[k] = t
        r = k // 7
        edges[k] = (tok_class(hl[k]), tok_class(vl[k + r + 1]), tok_class(hl[k + 7]), tok_class(vl[k + r]))
        if hp == '1':
            PEARL[c] = rnd
            pearls_now.append(c)
        else:
            PEARL.pop(c, None)
        if t >= 0:
            DUE[c] = rnd + t
        SEEN[c] = rnd
    head = cells[24]
    if MAP is None:
        if CANDS:
            identify(cells, edges, timers)
        if MAP is None:
            for k in range(49):
                c = cells[k]
                if EDGE_SEEN[c] != edges[k]:
                    EDGE_SEEN[c] = edges[k]
                    NB[c] = None
                    for d in range(4):
                        x, y = c % W, c // W
                        g = (((y - 1) % H) * W + x, y * W + (x + 1) % W, ((y + 1) % H) * W + x, y * W + (x - 1) % W)[d]
                        NB[g] = None

    # ------------------------------------------------ dragons in view
    parts = {}           # id -> {cell: facing}
    heads = {}           # id -> head cell
    teams = {}
    occ = {}             # cell -> id
    for line in vis._body_lines:
        team, did, x, y, facing, ish = line.split()
        did = int(did)
        c = int(y) * W + int(x)
        f = 'NESW'.index(facing)
        parts.setdefault(did, {})[c] = f
        teams[did] = team
        occ[c] = did
        if ish == '1':
            heads[did] = c

    global OUR_Q, THEIR_Q
    OUT.clear()
    read_sonar(ct, rnd)
    for did in (0, 1):
        h = heads.get(did)
        if h is None:
            continue
        Lq = len(parts[did])
        if teams[did] == my_team:
            OUR_Q = (h, rnd, Lq)
        else:
            THEIR_Q = (h, rnd, Lq)
    if IS_QUEEN:
        OUR_Q = (head, rnd, L)

    # our own body: visible chain plus remembered head path beyond the window
    if FIRST or not HIST or HIST[-1] != head:
        mine = chain(parts.get(MY_ID, {head: 0}), head)
        HIST = list(reversed(mine))
    body = HIST[-L:][::-1] if len(HIST) >= L else HIST[::-1]
    if len(HIST) > 400:
        HIST = HIST[-200:]

    # free time: earliest step at which a body cell may be entered
    free_at = {}
    for i, c in enumerate(body):
        free_at[c] = L + 1 - i
    lengths = {MY_ID: L}
    for did, ps in parts.items():
        if did == MY_ID:
            continue
        h = heads.get(did)
        if h is None:
            for c in ps:
                free_at[c] = INF
            continue
        order = chain(ps, h)
        tail = order[-1]
        complete = len(order) == len(ps) and all(in_window(n, head) for n in (
            (tail - W) % N, (tail + W) % N, (tail // W) * W + (tail + 1) % W, (tail // W) * W + (tail - 1) % W))
        Lj = len(order)
        lengths[did] = Lj if complete else max(Lj, len(ps)) + 100
        for i, c in enumerate(order):
            free_at[c] = (Lj + 1 - i) if complete else INF
        for c in ps:
            if c not in free_at:
                free_at[c] = INF

    my_q = None
    enemy_q = None
    for did in (0, 1):
        if did in teams:
            if teams[did] == my_team:
                my_q = did
            else:
                enemy_q = did

    # danger: cells an enemy head can step onto before our next turn
    danger = {}
    for did, h in heads.items():
        if did == MY_ID or teams[did] == my_team:
            continue
        Le = lengths.get(did, 2)
        reach = max(1, (min(Le, 99) + 3) // 4)
        frontier = [h]
        seen_d = {h: 0}
        for step in range(1, reach + 1):
            nf = []
            for c in frontier:
                for n in nb(c):
                    if n >= 0 and n not in seen_d and (n not in occ or n == head):
                        seen_d[n] = step
                        nf.append(n)
            frontier = nf
        for c, s in seen_d.items():
            if s == 0:
                continue
            w = 1.0 if s == 1 else 0.5
            old = danger.get(c)
            if old is None or old[0] < w or (old[0] == w and old[1] < Le):
                danger[c] = (w, Le, did)

    # ------------------------------------------------ helpers
    my_nb = nb(head)
    facing = 'NESW'.index(ct.head.dir.value)

    def step_ok(c, t):
        return c >= 0 and free_at.get(c, 0) <= t

    # how soon any other head could reach each nearby cell (they advance into our space too)
    hdist = {}
    hq = []
    for did, h in heads.items():
        if did != MY_ID:
            hdist[h] = 0
            hq.append(h)
    i = 0
    while i < len(hq) and len(hq) < 400:
        c = hq[i]
        i += 1
        t = hdist[c] + 1
        if t > 8:
            continue
        for n in nb(c):
            if n >= 0 and n not in hdist and free_at.get(n, 0) <= t:
                hdist[n] = t
                hq.append(n)

    def space(start, t0, cap, contested=True):
        seen = {start}
        q = [(start, t0)]
        i = 0
        hd = hdist if contested else {}
        while i < len(q) and len(seen) < cap:
            c, t = q[i]
            i += 1
            t += 1
            for n in nb(c):
                if n >= 0 and n not in seen and free_at.get(n, 0) <= t and hd.get(n, INF) > t:
                    if contested and IS_QUEEN and tdist(c, n) != 1 and not in_window(n, head):
                        continue   # a portal into the unknown is no room for the queen
                    seen.add(n)
                    q.append((n, t))
        return len(seen)

    def space_score(start, t0):
        # penalty for a head position with too little room; the queen also checks its own coils
        need = min(SPACE_CAP, L + 4)
        sp = space(start, t0, need)
        pen = 0.0
        if sp < need:
            pen += 150.0 + (need - sp) * 20.0
        if IS_QUEEN and L >= 6:
            need2 = min(L + 12, 44)
            sp2 = space(start, t0, need2, False)
            if sp2 < need2:
                pen += 300.0 + (need2 - sp2) * 30.0
            sp = min(sp, sp2)
        return pen, sp

    # ------------------------------------------------ trapped?
    legal = [d for d in range(4) if step_ok(my_nb[d], 1)]
    can_split = ct.unit_count < ct.unit_limit

    def child_ok(size, need=6):
        # the child is our tail reversed: its head is our tail cell, and it moves later this round
        if len(body) < L:
            return False
        th = body[-1]
        blocked = set(body[-size:])
        seen = {th}
        q = [th]
        i = 0
        while i < len(q) and len(seen) < need + 1:
            c = q[i]
            i += 1
            for n in nb(c):
                if n >= 0 and n not in seen and n not in blocked and free_at.get(n, 0) <= 1 + len(seen) // 3:
                    seen.add(n)
                    q.append(n)
        return len(seen) - 1 >= need

    if not legal:
        if L >= 4 and can_split and L - 2 >= 2 and child_ok(L - 2):
            ct.do_split(L - 2)
            OUT['ok'] = True
            return
        if DEBUG:
            ct.output_log('trapped')
        return  # no valid action: the engine removes us and drops our pearls

    # ------------------------------------------------ feeding (late game)
    if not IS_QUEEN and rnd >= FEED_FROM:
        best = None
        for did, h in heads.items():
            if did == MY_ID or teams[did] != my_team:
                continue
            Lj = lengths.get(did, 0)
            if Lj > 99:
                Lj -= 100   # incomplete view: at least this long
            d = tdist(head, h)
            is_q = did == my_q
            if rnd >= LATE_FEED_FROM:
                ok = d <= FEED_DIST + 1 and Lj >= L + 3 and (L <= 6 or Lj >= 2 * L)
            else:
                ok = (d <= FEED_DIST and L <= FEED_MAX_LEN and
                      (Lj >= 2 * L + 2 or (is_q and Lj >= L + 1)))
            if ok:
                key = (is_q, Lj)
                if best is None or key > best:
                    best = key
        if best is not None:
            return  # suicide next to the longer ally: it eats our pearls

    # ------------------------------------------------ head attack: stepping onto an enemy head kills both
    if not IS_QUEEN:
        best_atk = None
        for d in range(4):
            n = my_nb[d]
            did = occ.get(n)
            if did is None or heads.get(did) != n or teams[did] == my_team:
                continue
            Le = lengths.get(did, 2)
            if Le > 99:
                Le -= 100
            if did == enemy_q or Le >= L or (L <= 3 and Le >= 2 and ct.unit_count >= 6):
                key = (did == enemy_q, Le)
                if best_atk is None or key > best_atk[0]:
                    best_atk = (key, d)
        if best_atk is not None:
            ct.make_move(DIRS[best_atk[1]])
            OUT['ok'] = True
            OUT['back'] = OPP[best_atk[1]]
            return

    # ------------------------------------------------ split
    want_split = L >= SPLIT_LEN and can_split
    if IS_QUEEN and rnd >= QUEEN_SPLIT_UNTIL:
        want_split = False
    if want_split and rnd >= 470:
        want_split = False
    if want_split:
        hd = danger.get(head)
        crowded = IS_QUEEN and any(tdist(h, head) <= 2 for did, h in heads.items() if did != MY_ID)
        if (hd is None or (hd[1] <= 2 and not IS_QUEEN)) and not crowded and child_ok(2):
            ct.do_split(2)
            OUT['ok'] = True
            OUT['back'] = next((dd for dd in range(4) if len(body) > 1 and my_nb[dd] == body[1]), -1)
            return

    # ------------------------------------------------ food search (time-aware BFS)
    cap = FIRST_BFS_CAP if FIRST else BFS_CAP
    first = {}
    dist = {head: 0}
    order = []
    q = []
    for d in legal:
        n = my_nb[d]
        if n not in dist:
            dist[n] = 1
            first[n] = d
            q.append(n)
    i = 0
    while i < len(q) and len(q) < cap:
        c = q[i]
        i += 1
        t = dist[c] + 1
        fd = first[c]
        for n in nb(c):
            if n >= 0 and n not in dist and free_at.get(n, 0) <= t:
                dist[n] = t
                first[n] = fd
                q.append(n)

    other_heads = [h for did, h in heads.items() if did != MY_ID]
    best_dir = [0.0] * 4
    best_cell = [-1] * 4
    for c in q:
        v = 0.0
        if c in PEARL:
            age = rnd - PEARL[c]
            v = 10.0 if age == 0 else max(0.0, 10.0 - age * 0.4)
        elif MAP is not None:
            bt = BED[c]
            if bt:
                due = DUE.get(c)
                if due is not None and SEEN[c] >= rnd - 40:
                    if due <= rnd + dist[c]:
                        v = 7.0
                else:
                    v = min(8.0, 10.0 * BEDRATE[bt] * (rnd - SEEN[c]) * 0.5)
        elif SEEN[c] < 0:
            v = 0.3
        if v <= 0.0:
            continue
        dc = dist[c]
        # someone else is closer: likely gone by the time we arrive
        for h in other_heads:
            if tdist(h, c) < dc:
                v *= 0.35
                break
        s = v / (dc + 1.0)
        d = first[c]
        if s > best_dir[d]:
            best_dir[d] = s
            best_cell[d] = c

    # ------------------------------------------------ long-range goals from sonar / sightings
    goal_bonus = [0.0] * 4

    def pull(g, w):
        if g in dist:
            goal_bonus[first[g]] += w
            return
        here = tdist(head, g)
        for dd in legal:
            if tdist(my_nb[dd], g) < here:
                goal_bonus[dd] += w * 0.6

    if not IS_QUEEN:
        if THEIR_Q is not None and rnd - THEIR_Q[1] <= 6:
            g = THEIR_Q[0]
            dq = tdist(head, g)
            hunter = L <= 6 and (dq <= 12 or (rnd >= 300 and dq <= 24) or (MY_ID % 4 == 0 and dq <= 30))
            if hunter:
                pull(g, 6.0 if dq > 3 else 12.0)
        if OUR_Q is not None and rnd - OUR_Q[1] <= 8 and OUR_Q[0] != head:
            g = OUR_Q[0]
            dq = tdist(head, g)
            feeder = rnd >= FEED_FROM and L <= FEED_MAX_LEN + (3 if rnd >= LATE_FEED_FROM else 0)
            if not feeder:
                if dq <= 3:
                    for dd in legal:
                        if tdist(my_nb[dd], g) > dq:
                            goal_bonus[dd] += 3.0
            elif L <= FEED_MAX_LEN + (3 if rnd >= LATE_FEED_FROM else 0) and OUR_Q[2] > L + 1:
                if rnd >= LATE_FEED_FROM and dq <= 30:
                    pull(g, 4.0)
                elif dq <= 14 and (MY_ID % 3 == 0 or dq <= 6):
                    pull(g, 2.0)

    # ------------------------------------------------ score first steps
    qh = heads.get(my_q) if my_q is not None and not IS_QUEEN else None
    feeding_ok = rnd >= FEED_FROM and L <= FEED_MAX_LEN + (3 if rnd >= LATE_FEED_FROM else 0)
    scores = {}
    dbg = []
    for d in legal:
        n = my_nb[d]
        s = best_dir[d] * 10.0 + goal_bonus[d]
        dg = danger.get(n)
        if dg is not None:
            w, Le, did = dg
            if IS_QUEEN:
                s -= 400.0 * w
            elif did == enemy_q:
                s += 30.0 * w         # trading for their queen wins the tiebreak
            elif Le >= L + 2 and L <= 3:
                s -= 2.0 * w          # an even trade in dragons, a good one in length
            else:
                s -= (25.0 + 4.0 * L) * w
        pen, sp = space_score(n, 1)
        s -= pen
        if IS_QUEEN:
            for did, h in heads.items():
                if did != MY_ID and tdist(n, h) <= 2:
                    s -= 12.0
        elif qh is not None and not feeding_ok and tdist(n, qh) <= 2:
            s -= 30.0   # leave our queen room to move
        if not in_window(n, head):
            # through a portal onto a cell we cannot see
            s -= 600.0 if IS_QUEEN else 80.0
            if OUR_Q is not None and rnd - OUR_Q[1] <= 3 and tdist(n, OUR_Q[0]) <= 2 + rnd - OUR_Q[1]:
                s -= 3000.0     # our queen may be standing right there
        if d == facing:
            s += 0.2
        scores[d] = s
        if DEBUG:
            dbg.append('%s:%.1f/sp%d' % ('NESW'[d], s, sp))
    d = max(scores, key=scores.get)
    if DEBUG:
        ct.output_log('mv', 'NESW'[d], ' '.join(dbg))
    if scores[d] <= -2500.0 and not IS_QUEEN:
        return   # every way on risks killing our queen: dying here is cheaper

    # ------------------------------------------------ sprint along the path when it is free
    free_steps = (L + 3) // 4
    path = [d]
    tgt = best_cell[d]
    if free_steps >= 2 and tgt >= 0 and dist.get(tgt, 0) >= 2:
        # rebuild a path toward the target, step by step, greedily by BFS distance from it
        cur = my_nb[d]
        used = {cur}
        own = {c: i for i, c in enumerate(body)}
        steps = 1
        while steps < free_steps and cur != tgt:
            nxt = None
            bd = INF
            for dd in range(4):
                n = nb(cur)[dd]
                if n < 0 or n in used or not in_window(n, head):
                    continue
                if n in occ and n not in own:
                    continue
                if n in own and L + 1 - own[n] > steps + 1:
                    continue
                dn = tdist(n, tgt)
                if dn < bd:
                    bd, nxt = dn, dd
            if nxt is None or bd >= tdist(cur, tgt):
                break
            n = nb(cur)[nxt]
            dg = danger.get(n)
            if dg is not None and (IS_QUEEN or dg[1] > L):
                break
            if IS_QUEEN and tdist(n, head) > 2:
                break
            path.append(nxt)
            used.add(n)
            cur = n
            steps += 1
        if len(path) > 1:
            # check the space around where the sprint ends, with our body where it will be then
            newbody = []
            c = head
            for dd in path:
                c = nb(c)[dd]
                newbody.append(c)
            newbody = newbody[::-1] + body[:max(0, L - len(path))]
            saved = {}
            for c in body:
                saved[c] = free_at.pop(c, None)
            for i2, c in enumerate(newbody):
                saved.setdefault(c, free_at.get(c))
                free_at[c] = L + 1 - i2
            pen, spf = space_score(cur, 0)
            if DEBUG:
                ct.output_log('sprint', ''.join('NESW'[x] for x in path), 'sp', spf, 'pen', pen)
            if pen > 0:
                path = [d]
            for c, v in saved.items():
                if v is None:
                    free_at.pop(c, None)
                else:
                    free_at[c] = v

    c = head
    for dd in path:
        c = nb(c)[dd]
        HIST.append(c)
    if len(path) == 1:
        ct.make_move(DIRS[d])
    else:
        ct.make_moves([DIRS[x] for x in path])
    OUT['ok'] = True
    OUT['back'] = OPP[path[-1]]


def main():
    global FIRST, MY_ID, IS_QUEEN
    ct, game = unswbc.init()
    setup(game.width, game.height)
    MY_ID = ct.head.dragon_id
    IS_QUEEN = MY_ID in (0, 1)
    global MY_TEAM
    MY_TEAM = ct.head.team.value
    while unswbc.update(ct, game):
        try:
            execute_turn(ct, game)
        except Exception as e:
            ct.output_log('ERR', repr(e)[:200])
            # fall back to any open step
            here = ct.get_position()
            t = ct.get_tile(here)
            for dd in DIRS:
                if t.get_edge(dd).is_passable():
                    nxt = ct.get_tile(here.add_dir(dd))
                    if nxt is not None and nxt.get_dragon() is None:
                        ct.make_move(dd)
                        break
        try:
            broadcast(ct, game.round_num)
        except Exception as e:
            ct.output_log('ERR sonar', repr(e)[:200])
        FIRST = False
        unswbc.end_turn()


if __name__ == '__main__':
    main()
