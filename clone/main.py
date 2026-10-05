"""Behavioural clone of the rank-1 bot.

Two LightGBM models trained on ~990k of its decisions (tools/clone_*.py) choose each turn:
KIND picks move / split / suicide from window features, DIR ranks the four directions.
Features come from clonefeat.py, the same code that labelled the replays. Hard guards on top:
never an illegal step, the queen never suicides while she can move, splits must be legal.
"""
import helper as unswbc
from helper import Direction
import mapdb
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


QF, QT, QL, QR, QV, QR0 = (clonequeen.DIR_F, clonequeen.DIR_T, clonequeen.DIR_LC, clonequeen.DIR_RC,
                           clonequeen.DIR_V, clonequeen.DIR_R)


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


def pack(cell, rnd, length, team):
    payload = (1 | ((cell & 0x3FFF) << 2) | ((rnd & 511) << 16) | ((min(length, 511) & 511) << 25))
    return payload | (tag(payload, team) << 34)


def read_sonar(ct, rnd, team):
    global OUR_Q
    for m in ct.sonar_messages[-24:]:
        payload = m & ((1 << 34) - 1)
        if m >> 34 != tag(payload, team) or payload & 3 != 1:
            continue
        cell, r9, length = (payload >> 2) & 0x3FFF, (payload >> 16) & 511, (payload >> 25) & 511
        r = rnd - ((rnd - r9) & 511)
        if cell < N and (OUR_Q is None or r > OUR_Q[1]):
            OUR_Q = (cell, r, length)


def broadcast(ct, rnd, team, back):
    if OUR_Q is None or rnd - OUR_Q[1] > 4:
        return
    msg = pack(OUR_Q[0], OUR_Q[1], OUR_Q[2], team)
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

    global OUR_Q
    read_sonar(ct, rnd, my_team)
    for did in (0, 1):
        if did in heads and teams[did] == my_team:
            OUR_Q = (heads[did], rnd, len(parts[did]))
    if IS_QUEEN:
        OUR_Q = (head, rnd, L)
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
    kind = max(range(len(ks)), key=ks.__getitem__)

    # feeding (rank-1: short dragons 2-3 cells from a much longer ally suicide so it eats the drops)
    if not IS_QUEEN and L <= 3 and rnd >= 200 and OUR_Q is not None and rnd - OUR_Q[1] <= 1:
        dq = clonefeat.tdist(W, H, head, OUR_Q[0])
        if OUR_Q[2] >= L + 1 and dq <= (4 if rnd >= 420 else 3) and (rnd >= 420 or ct.unit_count >= 12):
            return
    if kind == 2 and (not IS_QUEEN or not legal):
        return   # suicide: the rank-1 bot dies here rather than crash, or to feed a long ally
    crowded = any(clonefeat.tdist(W, H, head, h) <= 3 for did, h in heads.items() if did != MY_ID)
    if kind == 1 and IS_QUEEN and legal and crowded:
        kind = 0   # her children spawn beside her: only split with room
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
    if IS_QUEEN:
        # The model rarely saw queen decisions; guard her like the rank-1 queen behaves:
        # never into a pocket (space), step away from adjacent enemy heads.
        need = min(20, max(12, 2 * L + 6))
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
    else:
        qh = next((h for did, h in heads.items() if did in (0, 1) and teams[did] == my_team), None)
        if qh is not None:
            away = [d for d in legal if clonefeat.tdist(W, H, CTX.nb(head)[d], qh) > 1]
            if away:
                cands = away
    if (not IS_QUEEN and L <= 3 and rnd >= 200 and OUR_Q is not None and rnd - OUR_Q[1] <= 6
            and fg[12] == 0 and OUR_Q[2] >= L + 2):
        here = clonefeat.tdist(W, H, head, OUR_Q[0])
        if here <= 25:
            closer = [d for d in cands if clonefeat.tdist(W, H, CTX.nb(head)[d], OUR_Q[0]) < here
                      and fd[d][10] >= 6]
            if closer:
                cands = closer
    best, bd = None, cands[0]
    for d in cands:
        x = fd[d] + fg + [1.0 if d == j else 0.0 for j in range(4)]
        s = dir_score(x)
        if best is None or s > best:
            best, bd = s, d
    n = CTX.nb(head)[bd]
    HIST.append(n)
    ct.make_move(DIRS[bd])
    SENT['back'] = (bd + 2) % 4
    SENT['ok'] = True


def main():
    global W, H, N, MY_ID, IS_QUEEN, CANDS
    ct, game = unswbc.init()
    W, H = game.width, game.height
    N = W * H
    for key, size in mapdb.SIZES.items():
        if size == (W, H):
            CANDS.append((key, mapdb.load(key)))
    MY_ID = ct.head.dragon_id
    IS_QUEEN = MY_ID in (0, 1)
    while unswbc.update(ct, game):
        SENT.clear()
        try:
            execute_turn(ct, game)
            if SENT.get('ok') and game.round_num >= 150:
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
