"""Behavioural clone of the rank-1 bot.

Two LightGBM models trained on ~990k of its decisions (tools/clone_*.py) choose each turn:
KIND picks move / split / suicide from window features, DIR ranks the four directions.
Features come from clonefeat.py, the same code that labelled the replays. Hard guards on top:
never an illegal step, the queen never suicides while she can move, splits must be legal.
"""
import gc
import random
import time
import helper as unswbc
from helper import Direction
import mapdb
import opening
import clonefeat
import clonemodel
import clonequeen

DIRS = (Direction.NORTH, Direction.EAST, Direction.SOUTH, Direction.WEST)
W = H = N = 0
CTX = None
CANDS = []
HIST = []
MY_ID = -1
IS_QUEEN = False
SHIELD = False   # queen prefers cells with allies around and fewer enemy heads (rank-1 queen sits in her swarm)
ESCORT = True    # idle dragons drift toward the queen
ESCORT_FROM = 150  # rank-1 vs v106: 4-6 allies within 7 of the queen already in rounds 50-149
SAFE = True      # queen measures room as cells she reaches before any enemy head
QHOME = True     # from round 100 the queen drifts back toward her spawn (rank-1: 11 cells out at r50-99, 7 by r150)
QROOM = 2        # other dragons keep this far from the queen's head (real games: allies boxed her in)
SPLIT_BIAS = 0.0  # our smaller colonies push the model to split twice as often as rank-1 mid-game
CHAMPS = False    # feed a champion once the queen is gone
LOCAL_ODDS = False  # scale head-trade rates by local head count
NO_EARLY_SUICIDE = 0  # round before which legal dragons never choose suicide (0 = off)
FORCE_SPLIT = 0  # round until which non-queen dragons always split at length >= 4 (0 = off)
GREEDY = False   # always take the shortest path to the nearest visible pearl
GREEDY_QUEEN = False
OPEN_RUSH = True # opening: head for the rank-1 bot's learned opening positions on this map
OPEN_UNTIL = 60
OPEN_FORCE = True  # keep heading for the opening spots even with pearls in view (rank-1 holds the fields)
OPEN_MAX_AREA = 1000  # small maps only: on big maps the learned targets just scatter the colony
RUSH = False     # opening: head straight for the nearest fountain while no pearl is in view
RUSH_UNTIL = 40
FARM = True      # walk to pearl fountains (beds refilling every ~1-3 rounds) and circle them
FARM_UNTIL = 400
FARM_RADIUS = 6
FOUNT = None
ESC = True       # queen keeps two exits other heads cannot reach next turn
QD_FEED = 0.0    # suicide-score boost next to a long ally once the queen is gone
BLIND = False    # queen avoids portal exits outside her window
REACH = False     # queen keeps her head beyond visible enemies' sprint reach
ATTACK_RATES = {1: (0.68, 0.37, 0.0), 2: (0.68, 0.37, 0.0), 3: (0.79, 0.63, 0.31), 4: (0.31, 0.10, 0.03),
                5: (0.07, 0.0, 0.03)}
SPRINT_ATTACK = {2: (0.02, 0.0, 0.0), 3: (0.43, 0.24, 0.22), 4: (0.25, 0.16, 0.09)}
HOME = None
HOMED = None
PEARLS = set()


KF, KT, KL, KR, KV, KR0 = (clonemodel.KIND_F, clonemodel.KIND_T, clonemodel.KIND_LC, clonemodel.KIND_RC,
                           clonemodel.KIND_V, clonemodel.KIND_R)
DF, DT, DL, DR, DV, DR0 = (clonemodel.DIR_F, clonemodel.DIR_T, clonemodel.DIR_LC, clonemodel.DIR_RC,
                           clonemodel.DIR_V, clonemodel.DIR_R)


def kind_scores(x):
    k = clonemodel.KIND_CLASSES
    s = [0.0] * k
    for j, i in enumerate(KR0):
        while i >= 0:
            i = KL[i] if x[KF[i]] <= KT[i] else KR[i]
        s[j % k] += KV[~i]
    return s


HAS_STEPS = hasattr(clonemodel, 'STEPS_R')


def steps_pred(x):
    # sprint length class (0 -> 1 step ... 3 -> 4 steps), trained on the rank-1 bot's moves at length >= 5
    k = clonemodel.STEPS_CLASSES
    s = [0.0] * k
    F, T, LC, RC, V = clonemodel.STEPS_F, clonemodel.STEPS_T, clonemodel.STEPS_LC, clonemodel.STEPS_RC, clonemodel.STEPS_V
    for j, i in enumerate(clonemodel.STEPS_R):
        while i >= 0:
            i = LC[i] if x[F[i]] <= T[i] else RC[i]
        s[j % k] += V[~i]
    return max(range(k), key=s.__getitem__) + 1


QF, QT, QL, QR, QV, QR0 = (clonequeen.DIR_F, clonequeen.DIR_T, clonequeen.DIR_LC, clonequeen.DIR_RC,
                           clonequeen.DIR_V, clonequeen.DIR_R)


def safe_space(st, start, cap=20):
    """Cells the queen reaches strictly before any visible enemy head (a Voronoi share), body
    cells counted free once her own tail passes. Plain space counts corridors other dragons are
    about to fill; real-game queens died boxed in that way."""
    nb = CTX.nb
    occ = st.occ
    own = {c: i for i, c in enumerate(st.body)}
    L = len(st.body)
    other = {}
    q = []
    for h, did, ally, n in st.heads:
        if not ally:
            other[h] = 0
            q.append(h)
    i = 0
    while i < len(q):
        c = q[i]
        i += 1
        t = other[c]
        if t >= 6:
            continue
        for n in nb(c):
            if n >= 0 and n not in other and n not in occ:
                other[n] = t + 1
                q.append(n)
    if other.get(start, 99) <= 1:
        return 0
    seen = {start}
    q = [(start, 1)]
    i = 0
    while i < len(q) and len(seen) < cap:
        c, t = q[i]
        i += 1
        for n in nb(c):
            if n < 0 or n in seen:
                continue
            j = own.get(n)
            if j is not None:
                if L + 1 - j > t + 1:
                    continue
            elif n in occ:
                continue
            if other.get(n, 99) <= t + 1:
                continue
            seen.add(n)
            q.append((n, t + 1))
    return len(seen)


HOMEQ = [None, 0]   # BFS queue from the queen's spawn, read position


def home_bfs(budget):
    global HOMED
    if HOMED is None:
        HOMED = {HOME: 0}
        HOMEQ[0] = [HOME]
        HOMEQ[1] = 0
    q, i = HOMEQ[0], HOMEQ[1]
    end = i + budget
    while i < len(q) and i < end:
        c = q[i]
        i += 1
        for n2 in CTX.nb(c):
            if n2 >= 0 and n2 not in HOMED:
                HOMED[n2] = HOMED[c] + 1
                q.append(n2)
    HOMEQ[1] = i
    if i >= len(q):
        HOMEQ[0] = None


T0 = [0]


def spent():
    # in the judge's sandbox the clock runs on CPU points (1 point = 1 ns): ~100M per turn
    return time.perf_counter_ns() - T0[0]


OPENF = [None]   # (distance bytes, transform or None) for our side, set once the map is known


def open_dist():
    if OPENF[0] is None:
        ent = opening.OPEN.get(CANDS[0][0])
        if ent is None:
            OPENF[0] = False
            return None
        tf, dist = ent[0], ent[1]
        M = CANDS[0][1]
        t0 = [c for t, cs in M['starts'] if t == 0 for c in cs]
        t1 = [c for t, cs in M['starts'] if t == 1 for c in cs]
        h = SPAWN[0] if SPAWN[0] is not None else 0
        near0 = min(clonefeat.tdist(W, H, h, c) for c in t0)
        near1 = min(clonefeat.tdist(W, H, h, c) for c in t1)
        OPENF[0] = (dist, tf if near1 < near0 else None)
    if not OPENF[0]:
        return None
    dist, tf = OPENF[0]
    if tf is None:
        return lambda c: dist[c]
    if tf == 'mx':
        return lambda c: dist[(c // W) * W + (W - 1 - c % W)]
    return lambda c: dist[(H - 1 - c // W) * W + (W - 1 - c % W)]


SPAWN = [None]


def dir_score(x):
    # the queen has her own direction model, trained on the rank-1 queen's ~30k decisions
    if IS_QUEEN:
        F, T, LC, RC, V, R = QF, QT, QL, QR, QV, QR0
    else:
        F, T, LC, RC, V, R = DF, DT, DL, DR, DV, DR0
    s = 0.0
    for i in R:
        while i >= 0:
            i = LC[i] if x[F[i]] <= T[i] else RC[i]
        s += V[~i]
    return s


# ---- sonar: the queen announces herself; feeders relay it (keyed so enemy echoes are ignored)
OUR_Q = None             # (cell, round, length)
SENT = {}
KEY = {'A': 0x5DEECE66D1F3A7B9, 'B': 0x2545F4914F6CDD1D}
MASK64 = (1 << 64) - 1


def tag(payload, team):
    v = (payload ^ KEY[team] ^ (W << 40) ^ (H << 50)) & MASK64
    v = ((v ^ (v >> 31)) * 0x9E3779B97F4A7C15) & MASK64
    v = ((v ^ (v >> 29)) * 0xBF58476D1CE4E5B9) & MASK64
    return (v ^ (v >> 32)) & ((1 << 30) - 1)


CHAMP = None             # (cell, round, length): the longest non-queen ally announcing itself
TARGET = None            # who feeders feed: the queen while she lives, else the champion
I_CHAMP = False


def pack(cell, rnd, length, team, kind=1):
    payload = (kind | ((cell & 0x3FFF) << 2) | ((rnd & 511) << 16) | ((min(length, 511) & 511) << 25))
    return payload | (tag(payload, team) << 34)


def read_sonar(ct, rnd, team):
    global OUR_Q, CHAMP
    for m in ct.sonar_messages[-24:]:
        payload = m & ((1 << 34) - 1)
        kind = payload & 3
        if m >> 34 != tag(payload, team) or kind not in (1, 2):
            continue
        cell, r9, length = (payload >> 2) & 0x3FFF, (payload >> 16) & 511, (payload >> 25) & 511
        r = rnd - ((rnd - r9) & 511)
        if cell >= N:
            continue
        if kind == 1:
            if OUR_Q is None or r > OUR_Q[1]:
                OUR_Q = (cell, r, length)
        elif (CHAMP is None or rnd - CHAMP[1] > 4 or length > CHAMP[2]
              or (length == CHAMP[2] and r > CHAMP[1])):
            CHAMP = (cell, r, length)


def broadcast(ct, rnd, team, back):
    if OUR_Q is not None and rnd - OUR_Q[1] <= 4:
        msg = pack(OUR_Q[0], OUR_Q[1], OUR_Q[2], team)
    elif CHAMP is not None and rnd - CHAMP[1] <= 4:
        msg = pack(CHAMP[0], CHAMP[1], CHAMP[2], team, 2)
    else:
        return
    for d in range(4):
        if d != back:
            ct.send_sonar(DIRS[d], msg)


def tok(t):
    return 0 if t == '.' else (1 if t == 'w' else 2)


def identify(cells, edges, timers):
    global CANDS, CTX
    keep = []
    for key, m in CANDS:
        walls, beds = m['walls'], m['beds']
        ok = True
        for k in range(49):
            c = cells[k]
            wl = walls[c]
            for d in range(4):
                cls = 1 if (wl >> d) & 1 else (2 if (wl >> (d + 4)) & 1 else 0)
                if cls != edges[k][d]:
                    ok = False
                    break
            if not ok or ((timers[k] >= 0) != (beds[c] != 0)):
                ok = False
                break
        if ok:
            keep.append((key, m))
    if keep:
        CANDS = keep
    CTX = clonefeat.MapCtx(CANDS[0][1])
    global FOUNT
    FOUNT = CANDS[0][1].get('fount')


def chain(parts, head):
    prev = {}
    for p, f in parts.items():
        if p != head:
            n = CTX.nb(p)[f]
            if n >= 0:
                prev[n] = p
    order = [head]
    seen = {head}
    cur = head
    while cur in prev and prev[cur] not in seen:
        cur = prev[cur]
        seen.add(cur)
        order.append(cur)
    return order


def execute_turn(ct, game):
    global HIST
    rnd = game.round_num
    vis = ct.vision
    L = ct.length
    my_team = ct.head.team.value
    tl = vis._tile_lines
    hl = "".join(vis._edge_lines[:8]).split()
    vl = "".join(vis._edge_lines[8:]).split()
    cells = [0] * 49
    timers = [0] * 49
    edges = [None] * 49
    for k in range(49):
        x, y, hp, pt = tl[k].split()
        c = int(y) * W + int(x)
        cells[k] = c
        timers[k] = int(pt)
        r = k // 7
        edges[k] = (tok(hl[k]), tok(vl[k + r + 1]), tok(hl[k + 7]), tok(vl[k + r]))
        if hp == '1':
            PEARLS.add(c)
        else:
            PEARLS.discard(c)
    head = cells[24]
    if CTX is None or len(CANDS) > 1:
        identify(cells, edges, timers)

    parts = {}
    heads = {}
    teams = {}
    for line in vis._body_lines:
        team, did, x, y, facing, ish = line.split()
        did = int(did)
        c = int(y) * W + int(x)
        parts.setdefault(did, {})[c] = 'NESW'.index(facing)
        teams[did] = team
        if ish == '1':
            heads[did] = c

    global OUR_Q, HOME, HOMED
    if IS_QUEEN and HOME is None:
        HOME = head
    if SPAWN[0] is None:
        SPAWN[0] = head
    if QHOME and IS_QUEEN and rnd >= 3 and CTX is not None and len(CANDS) <= 1 and (HOMED is None or HOMEQ[0]):
        home_bfs(150 if spent() < 40_000_000 else 0)   # spread over the early turns: a whole-map BFS in one turn broke the CPU cap
    read_sonar(ct, rnd, my_team)
    for did in (0, 1):
        if did in heads and teams[did] == my_team:
            OUR_Q = (heads[did], rnd, len(parts[did]))
    if IS_QUEEN:
        OUR_Q = (head, rnd, L)
    # Rank-1 keeps growing a dragon after its queen dies (52 cells by the end, vs 23 for us):
    # once she has gone quiet the longest dragon (>= 8) announces itself and feeders go to it.
    global TARGET, I_CHAMP, CHAMP
    I_CHAMP = False
    if OUR_Q is not None and rnd - OUR_Q[1] <= 15:
        TARGET = OUR_Q
    elif CHAMPS and rnd >= 200:
        ch = CHAMP if CHAMP is not None and rnd - CHAMP[1] <= 4 else None
        if not IS_QUEEN and L >= 8 and (ch is None or ch[2] < L or ch[0] == head):
            CHAMP = TARGET = (head, rnd, L)
            I_CHAMP = True
        else:
            TARGET = ch
    else:
        TARGET = None
    SENT['back'] = -1

    if not HIST or HIST[-1] != head:
        HIST = list(reversed(chain(parts.get(MY_ID, {head: 0}), head)))
    body = HIST[-L:][::-1]
    if len(HIST) > 600:
        HIST = HIST[-300:]

    others = []
    for did, ps in parts.items():
        if did == MY_ID:
            continue
        h = heads.get(did)
        cl = chain(ps, h) if h is not None else [-1]
        rest = [c for c in ps if c not in cl]
        others.append((did, teams[did] == my_team, cl + rest))
    st = clonefeat.State(CTX, rnd, MY_ID, body, others, PEARLS, ct.unit_count, IS_QUEEN)
    fd, fg = clonefeat.features(st)
    legal = [d for d in range(4) if fd[d][0] > 0]

    xk = fg + [v for f in fd for v in f]
    ks = kind_scores(xk)
    if not IS_QUEEN and rnd >= 100:
        ks[1] -= SPLIT_BIAS
    if (QD_FEED and not IS_QUEEN and L <= 3 and rnd >= 200 and fg[9] >= L + 3
            and (OUR_Q is None or rnd - OUR_Q[1] > 15)):
        ks[2] += QD_FEED   # queen gone: rank-1 feeds nearby long allies more (11% vs our 7%)
    kind = max(range(len(ks)), key=ks.__getitem__)

    # feeding (rank-1: short dragons 2-3 cells from a much longer ally suicide so it eats the drops)
    if not IS_QUEEN and not I_CHAMP and L <= 3 and rnd >= 200 and TARGET is not None and rnd - TARGET[1] <= 1:
        dq = clonefeat.tdist(W, H, head, TARGET[0])
        if TARGET[2] >= L + 1 and dq <= (4 if rnd >= 420 else 3) and (rnd >= 420 or ct.unit_count >= 12):
            return
    attacks = [d for d in range(4) if fd[d][15] > 0] if not IS_QUEEN else []
    if attacks and ATTACK_RATES:
        # rank-1 head trades, measured over 121 games: attack rate by own length and the
        # target's length (longer / equal / shorter than us)
        def tlen(d):
            n = CTX.nb(head)[d]
            return max((len(parts[did]) for did, h in heads.items() if h == n and teams[did] != my_team), default=0)
        ad = max(attacks, key=tlen)
        tl = tlen(ad)
        rate = ATTACK_RATES[min(L, 5)][0 if tl > L else (1 if tl == L else 2)]
        if LOCAL_ODDS:
            # a trade drops pearls for whoever has more heads nearby: press when we outnumber them
            na = sum(1 for did, h in heads.items() if teams[did] == my_team and did != MY_ID and clonefeat.tdist(W, H, head, h) <= 3)
            ne = sum(1 for did, h in heads.items() if teams[did] != my_team and clonefeat.tdist(W, H, head, h) <= 3)
            rate = min(0.95, rate * 1.6) if na + 1 > ne else rate * 0.5
        if random.random() < rate:
            HIST.append(CTX.nb(head)[ad])
            ct.make_move(DIRS[ad])
            SENT['back'] = (ad + 2) % 4
            SENT['ok'] = True
            return
        attacks = []
    elif SPRINT_ATTACK and not IS_QUEEN and 2 <= L <= 4:
        # rank-1 length 3-4 dragons pay a segment to sprint onto an enemy head two cells away
        eh = {h: len(parts[did]) for did, h in heads.items() if teams[did] != my_team}
        tg = []
        for d1 in range(4):
            m = CTX.nb(head)[d1]
            if m < 0 or m in st.occ:
                continue
            for d2 in range(4):
                n2 = CTX.nb(m)[d2]
                if n2 in eh:
                    tg.append((eh[n2], d1, d2, m, n2))
        if tg:
            tl, d1, d2, m, n2 = max(tg)
            rate = SPRINT_ATTACK[L][0 if tl > L else (1 if tl == L else 2)]
            if random.random() < rate:
                HIST.append(m)
                HIST.append(n2)
                ct.make_moves([DIRS[d1], DIRS[d2]])
                SENT['back'] = (d2 + 2) % 4
                SENT['ok'] = True
                return
    if kind == 2 and legal and rnd < NO_EARLY_SUICIDE:
        kind = 0   # economy: early suicides only throw a dragon away (feeding starts later)
    if kind == 2 and (not IS_QUEEN or not legal):
        return   # suicide: the rank-1 bot dies here rather than crash, or to feed a long ally
    crowded = any(clonefeat.tdist(W, H, head, h) <= 3 for did, h in heads.items() if did != MY_ID)
    if kind == 1 and IS_QUEEN and legal and crowded and rnd >= 50:
        kind = 0   # her children spawn beside her: only split with room (the rank-1 queen splits freely early)
    if FORCE_SPLIT and not IS_QUEEN and rnd < FORCE_SPLIT and L >= 4 and legal and kind == 0 and fg[11] > 2:
        kind = 1   # economy: more dragons sooner covers more pearls (rank-1 splits ~85% at L4 early)
    if rnd < 50 and L >= 4 and IS_QUEEN and legal and kind == 0:
        kind = 1   # rank-1 queen: 87% of her length-4 turns before round 50 are splits (colony bootstrap)
    if kind == 1 or not legal:
        if L >= 4 and ct.unit_count < ct.unit_limit:
            # trapped: the queen sheds two tail segments at a time (her tail frees), others keep the tail
            child = 2 if legal or IS_QUEEN else L - 2
            ct.do_split(child)
            SENT['ok'] = True
            return
        if not legal:
            return
    cands = legal
    # The rank-1 bot steps onto adjacent enemy heads (a trade): those cells are occupied, so they are
    # not "legal" steps, but its direction model saw them as options (feature 15) and chose them.
    if attacks:
        cands = legal + attacks
    if IS_QUEEN:
        # The model rarely saw queen decisions; guard her like the rank-1 queen behaves:
        # never into a pocket (space), step away from adjacent enemy heads.
        need = min(20, max(12, 2 * L + 6))
        if SAFE and spent() < 50_000_000:
            for d in legal:
                fd[d][10] = min(fd[d][10], safe_space(st, CTX.nb(head)[d]))
        for ok in (lambda f: f[10] >= need and f[13] == 0 and f[11] >= 4,
                   lambda f: f[10] >= need and f[13] == 0 and f[11] >= 3,
                   lambda f: f[10] >= need and f[13] == 0,
                   lambda f: f[10] >= need,
                   lambda f: f[10] >= min(need, L + 4) and f[13] == 0,
                   lambda f: f[10] >= min(need, L + 4)):
            sel = [d for d in legal if ok(fd[d])]
            if sel:
                cands = sel
                break
        else:
            cands = [max(legal, key=lambda d: fd[d][10])]
        # Real games: almost every queen loss is a length 2-3 enemy 1-3 cells away stepping onto her.
        # With an enemy head within 4, put as much distance as possible between her and it.
        if fg[11] <= 4:
            roomy = [d for d in cands if fd[d][10] >= min(need, L + 4)] or cands
            far = max(fd[d][11] for d in roomy)
            cands = [d for d in roomy if fd[d][11] == far]
        if SHIELD and len(cands) > 1:
            def shield(d):
                n = CTX.nb(head)[d]
                v = 0
                for did, h in heads.items():
                    if did == MY_ID:
                        continue
                    k = clonefeat.tdist(W, H, n, h)
                    if teams[did] == my_team:
                        v += 1 if 2 <= k <= 5 else 0
                    elif k <= 5:
                        v -= 2
                return v
            top = max(shield(d) for d in cands)
            cands = [d for d in cands if shield(d) == top]
        if ESC and len(cands) > 1:
            # one-step lookahead: keep exits no other head can take next turn (boxed-in queens
            # were our most common loss)
            ohn = set()
            for did, h in heads.items():
                if did != MY_ID:
                    ohn.update(CTX.nb(h))
            tail = set(body[-2:]) if L >= 3 else set(body[-1:])
            def exits(d):
                n2 = CTX.nb(head)[d]
                return sum(1 for m in CTX.nb(n2) if m >= 0 and m != head and m not in ohn
                           and (m not in st.occ or m in tail))
            for need_e in (2, 1):
                sel = [d for d in cands if exits(d) >= need_e]
                if sel:
                    cands = sel
                    break
        # a portal can drop her outside her window onto a dragon she cannot see (real losses on
        # Portals): only take a blind exit when nothing in view is left
        seen = [d for d in cands if fd[d][21] > 0] if BLIND else []
        if seen:
            cands = seen
        if REACH and cands:
            # v105-style hunters sprint onto the queen from 2-6 cells: a length-L dragon covers
            # ceil(L/4) free steps plus up to L-1 paid ones. Keep her head out of that reach.
            hunters = []
            for did, h in heads.items():
                if teams[did] == my_team:
                    continue
                ln = len(parts[did])
                reach = min((ln + 3) // 4 + ln - 1, 8)
                dist = {h: 0}
                q = [h]
                i = 0
                while i < len(q):
                    c = q[i]
                    i += 1
                    if dist[c] >= reach:
                        continue
                    for n2 in CTX.nb(c):
                        if n2 >= 0 and n2 not in dist and n2 not in st.occ:
                            dist[n2] = dist[c] + 1
                            q.append(n2)
                hunters.append((dist, reach))
            if hunters:
                def margin(d):
                    n2 = CTX.nb(head)[d]
                    return min(dist.get(n2, 99) - reach for dist, reach in hunters)
                safe = [d for d in cands if margin(d) >= 1]
                if safe:
                    cands = safe
                else:
                    top = max(margin(d) for d in cands)
                    cands = [d for d in cands if margin(d) == top]
        if QHOME and rnd >= 100 and fg[11] > 4 and HOMED is not None and not HOMEQ[0]:
            here = HOMED.get(head, 0)
            if here > 6:
                closer = [d for d in cands if HOMED.get(CTX.nb(head)[d], 99) < here]
                if closer:
                    cands = closer
    else:
        qh = next((h for did, h in heads.items() if did in (0, 1) and teams[did] == my_team), None)
        if qh is not None:
            away = [d for d in legal if clonefeat.tdist(W, H, CTX.nb(head)[d], qh) > QROOM]
            if away:
                cands = away
    if (not IS_QUEEN and L <= 3 and rnd >= 200 and TARGET is not None and rnd - TARGET[1] <= 6
            and fg[12] == 0 and TARGET[2] >= L + 2):
        here = clonefeat.tdist(W, H, head, TARGET[0])
        if here <= 25:
            closer = [d for d in cands if clonefeat.tdist(W, H, CTX.nb(head)[d], TARGET[0]) < here
                      and fd[d][10] >= 6]
            if closer:
                cands = closer
    if (OPEN_RUSH and not IS_QUEEN and rnd < OPEN_UNTIL and W * H <= OPEN_MAX_AREA and len(CANDS) <= 1
            and (fg[12] == 0 or OPEN_FORCE)):
        # opening race: walk to where the rank-1 bot's dragons stand in rounds 10-40 on this map
        # (learned from its replays, mirrored to our side); it wins the pearl fields that way
        od = open_dist()
        if od is not None:
            here = od(head)
            if (2 if OPEN_FORCE else 0) < here < 255:
                closer = [d for d in cands if CTX.nb(head)[d] >= 0 and od(CTX.nb(head)[d]) < here
                          and fd[d][10] >= 4]
                if closer:
                    cands = closer
    if RUSH and FOUNT is not None and rnd < RUSH_UNTIL and fg[12] == 0 and len(CANDS) <= 1:
        # opening race: rank-1 reaches the fountains ~4 cells ahead of us by round 20 and then
        # holds them (Devil: 140 fountain pearls vs our 2); go straight for the nearest one
        fh = FOUNT[head]
        if 0 < fh < 255:
            closer = [d for d in cands if CTX.nb(head)[d] >= 0 and FOUNT[CTX.nb(head)[d]] < fh
                      and fd[d][10] >= 4]
            if closer:
                cands = closer
    if (FARM and FOUNT is not None and not IS_QUEEN and rnd < FARM_UNTIL and len(CANDS) <= 1
            and W * H <= OPEN_MAX_AREA):
        # rank-1's colony growth comes from fountains (Devil, r<150: 140 of its pearls vs our 2):
        # its short dragons walk to one and loop a 2x2 block over it, splitting at length 4
        fh = FOUNT[head]
        if fh <= FARM_RADIUS:
            def fd_(d):
                n2 = CTX.nb(head)[d]
                return FOUNT[n2] if n2 >= 0 else 255
            if fh <= 1:
                ring = [d for d in cands if fd_(d) <= 1 and fd[d][10] >= 3]
                if ring:
                    cands = ring
            elif fg[12] == 0 or fh <= 4:
                closer = [d for d in cands if fd_(d) < fh and fd[d][10] >= 4]
                if closer:
                    cands = closer
    if (ESCORT and not IS_QUEEN and rnd >= ESCORT_FROM and OUR_Q is not None and rnd - OUR_Q[1] <= 6
            and fg[12] == 0 and fg[11] > 3):
        here = clonefeat.tdist(W, H, head, OUR_Q[0])
        if 5 < here <= 20:
            closer = [d for d in cands if clonefeat.tdist(W, H, CTX.nb(head)[d], OUR_Q[0]) < here
                      and fd[d][10] >= 6]
            if closer:
                cands = closer
    if GREEDY and fg[12] > 0 and len(cands) > 1 and (not IS_QUEEN or GREEDY_QUEEN):
        # Rank-1 (and so the plain clone) moves toward a visible pearl only ~53% of the time; its
        # edge is volume, not skill. Take the shortest path to the nearest pearl (feature 8 is the
        # BFS pearl distance through that step, 0 when the step itself eats one); the model only
        # breaks ties.
        bestp = min(fd[d][8] for d in cands)
        if bestp < 10:
            cands = [d for d in cands if fd[d][8] == bestp]
    best, bd = None, cands[0]
    for d in (cands if len(cands) > 1 else ()):
        x = fd[d] + fg + [1.0 if d == j else 0.0 for j in range(4)]
        s = dir_score(x)
        if best is None or s > best:
            best, bd = s, d
    n = CTX.nb(head)[bd]
    path = [bd]
    free = (L + 3) // 4
    if HAS_STEPS and L >= 5 and free >= 2 and fd[bd][15] == 0:
        k = min(free, steps_pred(fg + fd[bd] + [float(free)]))
        cur, cbody = n, ([n] + body if n in PEARLS else [n] + body[:-1])
        win = st.win
        while len(path) < k and spent() < 45_000_000:
            if L > 24:
                # long bodies make each re-simulation costly (CPU cap): keep straight while clear
                nxt = CTX.nb(cur)[path[-1]]
                if nxt < 0 or nxt in st.occ or nxt not in win or any(nxt in CTX.nb(h) for h in heads.values()):
                    break
                path.append(path[-1])
                cur = nxt
                continue
            st2 = clonefeat.State(CTX, rnd, MY_ID, cbody, others, PEARLS, ct.unit_count, IS_QUEEN)
            fd2, fg2 = clonefeat.features(st2)
            ok2 = [d for d in range(4) if fd2[d][0] > 0 and CTX.nb(cur)[d] in win
                   and fd2[d][13] == 0 and fd2[d][10] >= (min(20, 2 * L + 6) if IS_QUEEN else min(L + 4, 14))]
            if not ok2:
                break
            b2, s2 = ok2[0], None
            for d in (ok2 if len(ok2) > 1 else ()):
                v = dir_score(fd2[d] + fg2 + [1.0 if d == j else 0.0 for j in range(4)])
                if s2 is None or v > s2:
                    b2, s2 = d, v
            nxt = CTX.nb(cur)[b2]
            path.append(b2)
            cbody = [nxt] + cbody if nxt in PEARLS else [nxt] + cbody[:-1]
            cur = nxt
    c = head
    for d in path:
        c = CTX.nb(c)[d]
        HIST.append(c)
    if len(path) == 1:
        ct.make_move(DIRS[bd])
    else:
        ct.make_moves([DIRS[d] for d in path])
    SENT['back'] = (path[-1] + 2) % 4
    SENT['ok'] = True


def main():
    global W, H, N, MY_ID, IS_QUEEN, CANDS
    ct, game = unswbc.init()
    W, H = game.width, game.height
    N = W * H
    for key, size in mapdb.SIZES.items():
        if size == (W, H):
            CANDS.append((key, mapdb.load(key)))
    gc.disable()
    gc.freeze()
    MY_ID = ct.head.dragon_id
    IS_QUEEN = MY_ID in (0, 1)
    while unswbc.update(ct, game):
        SENT.clear()
        try:
            # a new dragon's first turn also pays ~31M points of interpreter start-up and model
            # loading (the clock starts at 0 with the process): count it against the guard
            T0[0] = time.perf_counter_ns() if T0[0] else 0
            execute_turn(ct, game)
            T0[0] = T0[0] or 1
            # automatic cyclic GC fired at random turns and pushed them past the CPU cap:
            # collect the young generation by hand on cheap turns instead
            if spent() < 40_000_000:
                gc.collect(0 if game.round_num % 100 else 1)
            if SENT.get('ok') and game.round_num >= ESCORT_FROM - 10:
                broadcast(ct, game.round_num, ct.head.team.value, SENT.get('back', -1))
        except Exception as e:
            ct.output_log('ERR', repr(e)[:200])
            here = ct.get_position()
            t = ct.get_tile(here)
            for dd in DIRS:
                if t.get_edge(dd).is_passable():
                    nxt = ct.get_tile(here.add_dir(dd))
                    if nxt is not None and nxt.get_dragon() is None:
                        ct.make_move(dd)
                        break
        unswbc.end_turn()


if __name__ == '__main__':
    main()
