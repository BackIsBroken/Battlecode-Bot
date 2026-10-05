# Reconstruction refined from 109 supplied matches. See README.md.
import helper as unswbc
from helper import Direction
import mapdata
import winner_policy

# Schooltime tested better with the richness-field exploring; every other map explores like v7
POTX_MAPS = ('schooltime', 'queen_of_spades', 'devil', 'trophy')


def load_map(key):
    # only the maps that match our board size are compiled (keeps a newborn dragon's first turn cheap)
    e = mapdata.load(key).copy()
    e['variant'] = key
    e['key'] = 'schooltime' if key == 'schooltime_sparse' else key
    e['potx'] = e['key'] in POTX_MAPS
    return e

# ---------------------------------------------------------------- tuning
SPLIT_MIN_LEN = 4          # split once a dragon reaches this length
CELLS_PER_UNIT = 8
TRAP_SPLIT = True          # split to escape when boxed in
EVADE_SCALE = 1.0          # scrim variant: danger-penalty scale (Default: 0.6)
ATTACK_MARGIN = 0
SPACE_BFS_CAP = 55         # max cells visited in a space check
TARGET_BFS_CAP = 200        # max cells visited when searching for food
SPRINT_REACH = 5           # how far enemy/our sprint strikes can reach
DANGER_BASE = 60
DANGER_PER_LEN = 12
DANGER_FAR = 0.8
LATE_GAME = 470            # stop voluntary splitting after this round
SMALL_MAP_CELLS = 256      # maps this size or smaller get the crowded-map settings below
HEAD_ZONE_PEN = 25
FEED_FROM = 501        # late game: short dragons next to a long team mate die so it can eat them
CONS_FROM = 400        # with a big lead in numbers, every dragon pours its length into a longer team mate
CONS_MIN_UNITS = 30
CONS = False           # latched once we decide to consolidate
DOOM = frozenset()     # known map: (cell * 4 + travel dir) states that can only end in a dead end
DOOM_PEN = 700
USE_DOOM = False
PRIME_MAPS = ('trauma',)
POCKET_FARM = True
NO_POCKET_MAPS = ('autarky',)
STRAIGHT_MAPS = {'default':12,'queen_of_spades':12,'autarky':12}
NOKING_MAPS = ('trophy',)
# --- queen rules: after 500 rounds the longer queen (dragon id 0 or 1) wins; a dead queen has length 0 ---
QUEEN_MODE = True
Q_SONAR_FROM = 1         # announce the queen early, before collectors disperse
Q_DONATE_FROM = 380      # late queen feeding; queen ties still use the longest survivor
Q_RESERVE_FEED_FROM = 470  # release insurance only beside a verified live queen at the end
Q_PULL_FROM = 330        # ... and heads toward her announced cell from here
Q_OPEN = 14             # the queen avoids cells with fewer than this many free cells within 3 steps
Q_BLIND = 1500           # penalty for the queen stepping onto a cell she cannot see (blind portal exit)
Q_DANGER = 2500          # penalty for the queen stepping onto a cell an enemy head can reach
Q_STRIKE = True          # any other dragon that can reach the enemy queen's head takes it (head to head kills both)
QTEAM = {}               # dragon id 0/1 -> team letter, learned on sight
MY_Q = None              # our queen's id once known
QUEEN_FEEDABLE = True     # false only for a certified isolated queen room
POCKET_MAX = 8
FAST_SPLIT = False
FAST_SPLIT_ROOM = 3
RELAY_MAPS = ('big_empty',)
RELAY = False
RELAY_FROM = 400
LATE_DONATE_FROM = 480   # last rounds: donate to any longer team mate in view (was: within 3 steps)
LATE_DONATE_RADIUS = 6
QOS_FARM_UNTIL = 120   # Queen of Spades: only our first dragon (ids 0/1) is sent to the rich zone behind the portal
# Reuse validated offline distances; no per-dragon full-map search.
FARM_MAPS = ('trauma', 'queen_of_spades', 'stronghold', 'devil')
FARM_RANGE = 45
FARM_W = 60
FARMD = None
RUSHD = None           # distance to the map's richest spawn tiles (md_rush); early-game pull
RUSH_MAPS = ('schooltime', 'trophy')   # test: no Trophy cup race
RUSH_UNTIL = 200
LATE_KEEP_FROM = 440
LATE_KEEP_LEN = 10
RUSH_W = 25
# Trophy: the ladder opponent keeps ~8 dragons in the central cup from round ~20 (ladder replays M767730-M798394: it ate ~260 cup
# pearls by round 100, we ate 4; everything else was equal). One dragon in three (id % 3 == 0, id < 60) rushes the cup and
# holds it until round 200; the rest forage as before.
TROPHY_MOD = 3
TRAUMA_SUSTAINABLE = False   # md_sustainable pointed at the slow 's' beds; the map's own 'farm' field points at the fast refill lines
TROPHY_IDMAX = 60
TROPHY_UNTIL = 200
TROPHY_NEAR = 60
RUSH_NEAR = 10         # only dragons already this close race for the rich area
HOLD_W = 15            # dragons inside it are discouraged from wandering out
ZONEF = None           # md_zones fields for this map: {'mid', 'A', 'B'} -> distance bytes
ZONE_UNTIL = 200
ZONE_W = 25
ZONE_ROLES = 3         # id % 3: 0 -> centre, 1 -> our corner, 2 -> free
YIELD_FIELD = None
FINAL_FROM = 450        # from here donation needs no minimum team size, and ...
FINAL_PULL_RANGE = 30   # ... from 430 dragons drift toward the longest ally heard within this range
GROWER_MAPS = ('schooltime', 'default', 'devil')   # stronghold: lost 2/2 vs v36 with growers
KEEP_LONG = 12          # from KEEP_LONG_FROM, dragons this long only split to escape a trap
KEEP_LONG_FROM = 150
PIN = {}               # known map: portal exit cell -> the cells whose portal lands there
BLIND_ENTRY_PEN = 20   # standing on a portal exit whose far side we cannot see
Q_BLIND_ENTRY_PEN = 250  # queen: unseen inbound traffic can arrive after her turn
POCKET_PEN = 10        # a pocket whose only way on is a blind portal
LOOP_WINDOW = 13       # our last head cells ...
LOOP_PEN = 7           # ... cost this much per revisit
LOOP_HUNGRY = 8        # ... but only once we have gone this many moves without eating (circling a farm is fine)
LAST_EAT = 0           # move number of our last pearl
PORTAL_PENDING = None
OBSERVED_LINKS = {}
PORTAL_VISITS = {}
FIRST = True           # this dragon's first turn: interpreter start-up already used much of the CPU budget
ALLY_LENGTHS = {}      # recently observed allies, refreshed by checked sonar
ANNOUNCED = {}         # ally id -> (length, cell, round) heard over the 64-bit sonar
SONAR_FROM = 360       # start broadcasting length + position in all four directions
PULL_FROM = 999        # shorter dragons drift toward the longest ally they have heard of ...
PULL_RANGE = 24        # ... if it is within this many steps
PENDING_REPORTS = {}   # remote reports need two distinct recent observations
PACKET_BITS = 0
PACKET_MASK = 0
PACKET_TAG_MASK = 0
CELL_BITS = 0
CELL_MASK = 0
LAST_ROUND = 0
POST_LENGTH = 0
ACTION_KIND = 'move'
CONSOLIDATE_FROM = 400
DONATE_RADIUS = 3
ALLOW_EQUAL_DONORS = False
FEED_ROUND = 468       # round-500 check: short dragons beside our longest visible dragon turn into its pearls
FEED_MIN_UNITS = 10
SONAR_TAG = 5          # top of the word the strongest sonar team stamps on its messages
CLAIMS = {}
COORD_MAP = None
AIM_DIST = 0
GOAL, GOAL_SINCE = -1, -100
AIM = -1               # the cell we are heading for this turn (for the sonar decoy)
HUNT_FROM = 420        # late game: short dragons chase visible enemies longer than themselves
HUNT_W = 14
KING_MOD = 4           # one dragon in this many grows instead of splitting
KING_FROM = 60         # ... once the round number reaches this
GROWER_MIN_POP = 0     # no growers until the team has this many dragons (set per map)
TRADE_RISK_FACTOR = 1.0

DIRS = [Direction.NORTH, Direction.EAST, Direction.SOUTH, Direction.WEST]
ORDER = (0, 1, 2, 3)
VR = 3
VS = 7

GROW_BY_LENGTH = False
BALANCED_KINGS = False
W = H = N = 0
NB = None          # NB[d][i] -> neighbour cell index
EDGE = None        # EDGE[i] -> 4-char code tuple (0 empty, 1 kelp, 2 portal) or None
BED = None         # -1 no bed, None unknown, else round pearl may appear
PEARL = None
SEEN = None
GAP_EST = None      # observed pearl respawn gap; zero until a reset is actually seen
UNITS_CAP = 2
SMALL = False
CANDS = []         # bundled maps that still match everything we have seen
KNOWN = None       # the bundled map we are on, once identified
VALID = None       # cells already checked against the known map
RATE = None        # pearls per round a cell's bed produces (known map)
MAXRATE = 1.0
POTX = False       # explore by the richness field on this map
ZONED = False      # known map whose pearl beds differ a lot in speed
BEDC = None        # bed class per cell (known map)
POT = None         # richness of the neighbourhood, 0..35 (known map)
PORTS = {}         # portal id -> canonical edges seen (unknown maps)
LINKED = set()     # portal ids whose two ends are both known
STEPS = 0          # moves made by this dragon
TELE = -10 ** 6    # move number of our last trip through a portal
HIST = []          # cells our head has occupied, oldest first (tracks our body beyond vision)
UNSEEN_EXIT_PEN = 25   # stepping through a portal onto a cell we cannot see
QUEUE_PEN = 150        # ... right behind another dragon that went through (its tail may still be at the exit)
QUEUE_ROUNDS = 3
LASTOCC = None         # round each cell was last seen holding another dragon
EXP_T = 60             # unseen bed: assume it was last emptied this many rounds ago
EXP_UNSEEN = 60
EXP_POT = 1.5
CROWD_PEN = 12         # blind portal exit: extra risk per friendly head we can see (crowded zone)
SMALL_CHILD = 2        # child size when a long dragon splits
SMALL_CHILD_FROM = 6   # dragons at least this long split off a small child instead of half
END_GROW = 350         # after this round dragons stop splitting and grow for the round-500 length check
END_SAFE_LEN = 6       # ... and a dragon this long only trades heads with a much longer enemy
END_SAFE_MARGIN = 3
CHILD_ROOM = 6         # a split child must have this many free cells around our tail
MIN_NEED = 6           # big maps: short dragons avoid pockets smaller than this
SMALL_ATTACK_MARGIN = 1  # small maps: only start a head trade when the enemy is strictly longer
MASKT = [tuple((k >> d) & 1 for d in range(4)) for k in range(16)]
OPP = (2, 3, 0, 1)
CHECK_K = (17, 23, 24, 25, 31)   # window slots of our head and its four neighbours


def setup(w, h):
    global W, H, N, NB, EDGE, BED, PEARL, SEEN, GAP_EST, UNITS_CAP, SMALL
    global DANGER_BASE, DANGER_PER_LEN, DANGER_FAR, HEAD_ZONE_PEN
    W, H, N = w, h, w * h
    global CELL_BITS, CELL_MASK, PACKET_BITS, PACKET_MASK, PACKET_TAG_MASK
    CELL_BITS = max(1, (N - 1).bit_length())
    CELL_MASK = (1 << CELL_BITS) - 1
    PACKET_BITS = CELL_BITS + 32
    PACKET_MASK = (1 << PACKET_BITS) - 1
    PACKET_TAG_MASK = (1 << max(0, 64 - PACKET_BITS)) - 1
    NB = [None] * N
    EDGE = [None] * N
    BED = [None] * N
    PEARL = [False] * N
    SEEN = [-1] * N
    GAP_EST = [0] * N
    global LASTOCC
    LASTOCC = [-99] * N
    UNITS_CAP = min(60, max(2, N // CELLS_PER_UNIT))
    global CANDS, KNOWN, VALID, RATE, POT, PORTS, LINKED, ZONED
    ZONED = False
    # Map knowledge is optional: a broken data module must not prevent
    # the first action. Keep learning from vision if a candidate cannot load.
    CANDS = []
    for key, size in mapdata.SIZES.items():
        if size != (w, h):
            continue
        try:
            candidate = load_map(key)
            if ((candidate['w'], candidate['h']) != (w, h) or
                    any(len(candidate[name]) != N for name in ('walls', 'beds', 'pot'))):
                continue
            CANDS.append(candidate)
        except Exception:
            continue
    configure_strategy(None)
    KNOWN = VALID = RATE = POT = None
    PORTS = {}
    LINKED = set()
    # small maps are crowded with enemy heads: caution there costs more pearls than it saves dragons
    SMALL = N <= SMALL_MAP_CELLS
    if SMALL:
        DANGER_BASE, DANGER_PER_LEN, DANGER_FAR, HEAD_ZONE_PEN = 20, 4, 0.3, 8


def nbs(i):
    t = NB[i]
    if t is None:
        x = i % W
        y = i // W
        t = NB[i] = (((y - 1) % H) * W + x, y * W + (x + 1) % W,
                     ((y + 1) % H) * W + x, y * W + (x - 1) % W)
    return t


def geo(i):
    x = i % W
    y = i // W
    return (((y - 1) % H) * W + x, y * W + (x + 1) % W, ((y + 1) % H) * W + x, y * W + (x - 1) % W)


def set_nb(c, d, dest):
    t = list(nbs(c))
    t[d] = dest
    NB[c] = tuple(t)


def tok_class(tok):
    if tok == ".":
        return 0
    if tok == "w":
        return 1
    return 2


def expected(e, i):
    # edge classes (0 open, 1 kelp, 2 portal) the bundled map has around cell i
    t = MASKT[e['walls'][i]]
    if i in e['pcells']:
        t = list(t)
        for c, d, dest in e['portals']:
            if c == i:
                t[d] = 2
        t = tuple(t)
    return t


def activate(e):
    # trust the bundled map: every wall and portal exit is known from now on
    global KNOWN, EDGE, NB, VALID, RATE, POT
    KNOWN = e
    # Portals: use the map data for routing, but keep the unknown-map growth/endgame policy that
    # converted length better in local tests (map-data policy: +30 length at round 100, lost on longest).
    configure_strategy(None if e.get("key") in ("portals", "slithery_fight", "autarky") else e.get("key"), e.get("key"))
    if e.get("key") == "slithery_fight":
        # test: consolidate earlier on Slithery Fight (ladder M740800: at round 400 we had 62 dragons / 237 length
        # against their 18 / 131 and still lost the length check 36 v 42)
        global CONSOLIDATE_FROM, END_GROW
        CONSOLIDATE_FROM = 340
        END_GROW = min(END_GROW, 320)
    global BEDC
    BEDC = e['beds']
    kc = e['kcells']
    if len(kc) * 3 < N:
        EDGE = [MASKT[0]] * N
        for c, m in zip(kc, e['kmasks']):
            EDGE[c] = MASKT[m]
    else:
        EDGE = list(map(MASKT.__getitem__, e['walls']))
    NB = [None] * N
    for c, d, dest in e['portals']:
        set_nb(c, d, dest)
    RATE = e['rates']      # indexed by BEDC[cell]
    global ZONED
    pos = [r for r in RATE if r > 0]
    ZONED = bool(pos) and max(pos) >= 2 * min(pos)
    global FARMD, YIELD_FIELD, RUSHD, ZONEF
    # Old auxiliary fields are valid only for their original geometry and beds.
    ZONEF = RUSHD = YIELD_FIELD = None
    if e.get('aux_fields', True):
        from md_zones import ZONES
        ZONEF = ZONES.get(e['key'])
        from md_rush import RUSH
        RUSHD = RUSH.get(e['key']) if e['key'] in RUSH_MAPS else None
        if e['key'] == 'schooltime':
            from yield37 import FIELDS
            YIELD_FIELD = FIELDS[e['key']]
    # distance to the nearest always-refilling bed, precomputed offline (computing it here cost a newborn
    # dragon its whole first-turn budget: live v41 lost dragons to 'no valid action' on Trauma/Stronghold/Devil)
    FARMD = e.get('farm') if e['key'] in FARM_MAPS else None
    if e['key'] == 'trauma' and TRAUMA_SUSTAINABLE:
        from md_sustainable import FARM_DISTANCE
        FARMD = FARM_DISTANCE
    global MAXRATE, POTX
    MAXRATE = max(pos) if pos else 1.0
    POTX = e.get('potx', False)
    POT = e['pot']
    global DOOM, PIN
    # The precomputed dead-end table assumed a portal keeps the travel direction; with it on, Trauma
    # expansion fell from 24 to 10 dragons by round 200. Disabled until the portal rule is verified.
    DOOM = frozenset(e.get('doom', ())) if USE_DOOM else frozenset()
    PIN = {}
    for c, d, dest in e['portals']:
        PIN.setdefault(dest, []).append(c)
    # Match seeds randomize pearl respawns. Use only live countdowns and sightings.


def configure_strategy(key, phase_key=None):
    """Map profiles change growth policy; all maps share the same legal moves.

    Unrecognised maps keep conservative defaults. This is deliberately explicit
    so future map changes can be retested instead of silently reusing tuning.
    """
    global END_GROW, KING_FROM, GROW_BY_LENGTH, BALANCED_KINGS, FEED_FROM
    global UNITS_CAP, SMALL, DANGER_BASE, DANGER_PER_LEN, DANGER_FAR, HEAD_ZONE_PEN
    global GROWER_MIN_POP, TRADE_RISK_FACTOR
    GROWER_MIN_POP, TRADE_RISK_FACTOR = 0, 1.0
    global LOOP_PEN, LOOP_HUNGRY, RELAY
    LOOP_PEN, LOOP_HUNGRY = 7, 8
    RELAY = key in RELAY_MAPS
    global FINAL_PULL_RANGE, FINAL_FROM, LATE_DONATE_FROM, PULL_FROM, PULL_RANGE
    # Big Empty only (every other map keeps v46's endgame exactly): merge earlier and from further away
    # v48 merged from 430/455 with a 60-step pull: live, 111-112 self-destructs per Big Empty game between rounds
    # 400 and 450 (total length 373 -> 51). v49 keeps v48's relay and long pull but merges on v46's schedule.
    # Big Empty: wide donation from 470 (was 480) - M190476 (lost 48-49) becomes 50-49 against the recorded opponent
    FINAL_PULL_RANGE, FINAL_FROM, LATE_DONATE_FROM = (60, 470, 470) if key == 'big_empty' else (30, 450, 480)
    PULL_FROM, PULL_RANGE = (380, 60) if key == 'big_empty' else (999, 24)
    global FAST_SPLIT
    # top-team replays: on Default Small winners split on 66% of turns at length 4+, we split on 41%
    FAST_SPLIT = key is None or key == 'default_small'
    global EVADE_SCALE
    # scrim variant: on Default we step away from nearby enemy heads 69% of the time, opponents 55%
    EVADE_SCALE = 0.6 if key == 'default' else 1.0
    # (Yueci Li 22) these layouts benefit from abandoning empty patrol loops sooner
    if key in ("default", "queen_of_spades", "stronghold"):
        LOOP_PEN, LOOP_HUNGRY = 16, 4
    # (Yueci Li 18) crowded feeding corridors need enough collectors before reserving growers
    if key in ('default_small', 'queen_of_spades'):
        GROWER_MIN_POP = min(12, max(4, max(2, N // CELLS_PER_UNIT) // 2))
    # (Yueci Li 18) in Colosseum a short dragon can contest a longer enemy at low trade cost
    if key == 'Colosseum':
        TRADE_RISK_FACTOR = 0.25
    # Reset all profile overrides if a bundled-map identification is rejected.
    UNITS_CAP = min(60, max(2, N // CELLS_PER_UNIT))
    SMALL = N <= SMALL_MAP_CELLS
    if SMALL:
        DANGER_BASE, DANGER_PER_LEN, DANGER_FAR, HEAD_ZONE_PEN = 20, 4, 0.3, 8
    else:
        DANGER_BASE, DANGER_PER_LEN, DANGER_FAR, HEAD_ZONE_PEN = 60, 12, 0.8, 25
        if key is None:
            # On unseen open maps, an equal head collision loses a collector.
            DANGER_BASE, DANGER_PER_LEN, DANGER_FAR, HEAD_ZONE_PEN = 85, 14, 0.9, 30
    END_GROW, KING_FROM = 350, 160
    if key is None:
        GROWER_MIN_POP = 6
    GROW_BY_LENGTH, BALANCED_KINGS, FEED_FROM = False, True, 501
    if key in ('default', 'schooltime', 'trophy'):
        END_GROW, KING_FROM = 350, 160
        GROW_BY_LENGTH, BALANCED_KINGS, FEED_FROM = True, True, 501
    elif key == 'queen_of_spades':
        END_GROW, KING_FROM = 350, 160
        SMALL = True
        DANGER_BASE, DANGER_PER_LEN, DANGER_FAR, HEAD_ZONE_PEN = 20, 4, 0.3, 8
        BALANCED_KINGS, FEED_FROM = True, 501
    elif SMALL:
        FEED_FROM = 501
    # The replay target keeps a broad colony until late, without reserving growers
    # by dragon ID. Leave four slots for emergency tail-first splits.
    global CONSOLIDATE_FROM, ALLOW_EQUAL_DONORS
    CONSOLIDATE_FROM = 455 if key == 'big_empty' else 400
    # Tested Default profile: break equal-length ties by ID to seed growers.
    ALLOW_EQUAL_DONORS = key == 'default'
    if key is not None:
        UNITS_CAP = min(60, max(8, N // 8))
        END_GROW, KING_FROM = 450, 501
        GROW_BY_LENGTH = BALANCED_KINGS = False
        if key in GROWER_MAPS:
            # Live (v36): on these maps our whole colony stayed at length 3-4 for 400 rounds while the
            # opponent kept dragons of 8-20 from round ~150 and ended with 29-59. Reserve growers
            # the way Trophy does (Trophy: best live map for this family).
            END_GROW, KING_FROM = 350, 160
            GROW_BY_LENGTH = BALANCED_KINGS = True
            CONSOLIDATE_FROM = 430
        elif key == 'trophy':
            END_GROW, KING_FROM = 350, 160
            GROW_BY_LENGTH = BALANCED_KINGS = True
            CONSOLIDATE_FROM = 430
        elif key == 'trauma':
            KING_FROM = 250
            GROW_BY_LENGTH = BALANCED_KINGS = True
            CONSOLIDATE_FROM = 450
        elif key == 'big_empty':
            # Replay 217909 had more total length but lost 52 to 66 on the
            # longest dragon. Grow a small set of anchors before final merges.
            KING_FROM, GROWER_MIN_POP = 130, 24
            BALANCED_KINGS = True
    if key in NOKING_MAPS:
        # A dead-queen tie still uses the longest survivor. Keep the Trophy
        # grower profile instead of disabling every reserve until round 501.
        END_GROW, KING_FROM = 350, 160
        GROW_BY_LENGTH = BALANCED_KINGS = True
    if key in ('stronghold', 'trauma'):
        # (Yueci Li 57) Merge 80 rounds earlier on the two farm-corridor maps. The longest
        # dragon decides round 500 and growth is capped at +1 per turn, so the late merge
        # left most team length unconverted. Local vs Li55: Stronghold 16-4, Trauma 12-8.
        CONSOLIDATE_FROM = max(320, CONSOLIDATE_FROM - 80)
        FINAL_FROM = max(380, FINAL_FROM - 40)
        LATE_DONATE_FROM = max(420, LATE_DONATE_FROM - 40)

    # Reconstructed colony/grower timing from the supplied winning trajectories.
    global KING_MOD, KEEP_LONG, KEEP_LONG_FROM, SMALL_CHILD_FROM, SONAR_FROM
    global Q_DONATE_FROM, Q_PULL_FROM, LATE_KEEP_FROM
    profile = winner_policy.profile(key if phase_key is None else phase_key, N)
    UNITS_CAP = profile['cap']
    END_GROW, KING_FROM = profile['stop_split'], profile['grow_from']
    GROWER_MIN_POP = profile['grow_population']
    KING_MOD, BALANCED_KINGS, GROW_BY_LENGTH = 8, True, True
    KEEP_LONG, KEEP_LONG_FROM = 8, 180
    SMALL_CHILD_FROM = 5
    CONSOLIDATE_FROM = profile['merge_from']
    FINAL_FROM, LATE_DONATE_FROM = profile['final_from'], profile['wide_feed_from']
    PULL_FROM, PULL_RANGE = profile['pull_from'], 36
    Q_DONATE_FROM, Q_PULL_FROM = profile['queen_feed_from'], profile['queen_pull_from']
    SONAR_FROM = profile['sonar_from']
    LATE_KEEP_FROM = 380
    # Short children need an immediate legal exit; spatial checks below remain.
    FAST_SPLIT = profile['fast_split']


def learn_portal(i, d, tok):
    # unknown map: once both ends of a portal have been seen, link them
    if d == 0 or d == 3:
        key = (i, d)
    else:
        key = (geo(i)[d], OPP[d])
    s = PORTS.get(tok)
    if s is None:
        s = PORTS[tok] = []
    if key in s:
        return
    s.append(key)
    if len(s) == 2 and tok not in LINKED:
        LINKED.add(tok)
        (a, sa), (b, sb) = s
        if sa != sb:
            return
        set_nb(a, sa, geo(b)[sa])
        set_nb(geo(a)[sa], OPP[sa], b)
        set_nb(b, sa, geo(a)[sa])
        set_nb(geo(b)[sa], OPP[sa], a)
        for c, dd in ((a, sa), (geo(a)[sa], OPP[sa]), (b, sa), (geo(b)[sa], OPP[sa])):
            e = EDGE[c]
            if e is not None:
                t = list(e)
                t[dd] = 0
                EDGE[c] = tuple(t)


def code(tok):
    if tok == ".":
        return 0
    if tok == "w":
        return 1
    return 2


def plain_neighbor(cell, direction):
    x, y = cell % W, cell // W
    return ((y + (direction == 2) - (direction == 0)) % H) * W + ((x + (direction == 1) - (direction == 3)) % W)


def open_dir(i, d):
    e = EDGE[i]
    return e is None or e[d] == 0      # unknown is optimistic; portals treated as walls


def recent_pearl(c, rnd):
    # A sighting is evidence only for a short time on a contested map.
    return PEARL[c] and 0 <= rnd - SEEN[c] <= 12


def tdist(a, b):
    ax, ay = a % W, a // W
    bx, by = b % W, b // W
    dx = abs(ax - bx)
    dy = abs(ay - by)
    return min(dx, W - dx) + min(dy, H - dy)


def identify(tl, hl, vl):
    global CANDS
    keep = []
    step = 1
    for e in CANDS:
        ok = True
        for k in range(0, VS * VS, step):
            line = tl[k]
            x, y, hp, pt = line.split()
            i = int(y) * W + int(x)
            r = k // VS
            if (int(pt) >= 0) != (e['rates'][e['beds'][i]] > 0):
                ok = False
                break
            if (tok_class(hl[k]), tok_class(vl[k + r + 1]), tok_class(hl[k + VS]), tok_class(vl[k + r])) != expected(e, i):
                ok = False
                break
        if ok:
            keep.append(e)
    CANDS = keep
    if len(keep) == 1:
        activate(keep[0])


def forget_map():
    # what we saw contradicts the bundled map: fall back to learning the map from vision
    global DOOM, PIN, COORD_MAP
    COORD_MAP = None
    DOOM = frozenset()
    PIN = {}
    global CANDS, KNOWN, EDGE, NB, RATE, POT, VALID, ZONED, PORTS, LINKED, RUSHD, ZONEF, FARMD, YIELD_FIELD
    RUSHD = ZONEF = FARMD = YIELD_FIELD = None
    PORTS, LINKED = {}, set()
    CANDS = []
    configure_strategy(None)
    ZONED = False
    KNOWN = RATE = POT = VALID = None
    EDGE = [None] * N
    NB = [None] * N
    for (cell, direction), destination in OBSERVED_LINKS.items():
        set_nb(cell, direction, destination)


def go(ct, head, dirs):
    # issue a move and remember the cells our head passes through
    global STEPS, TELE, LAST_EAT, POST_LENGTH, AIM
    if len(dirs) > 1:
        AIM = -1
    POST_LENGTH = ct.length - max(0, len(dirs) - (ct.length + 3) // 4)
    c = head
    for d in dirs:
        n = nbs(c)[d]
        if n != geo(c)[d]:
            TELE = STEPS
        c = n
        HIST.append(c)
        if recent_pearl(c, LAST_ROUND):
            LAST_EAT = STEPS + 1
            POST_LENGTH += 1
        PEARL[c] = False
        STEPS += 1
    if len(dirs) == 1:
        ct.make_move(DIRS[dirs[0]])
    else:
        ct.make_moves([DIRS[d] for d in dirs])


def packet_tag(payload, team):
    # Version-specific keyed checksum, also bound to board dimensions and team.
    # This rejects the old public-tag spoof protocol; it is not a guarantee
    # against an adversary with our source. Keep visibility/freshness checks too.
    mask = 0xffffffffffffffff
    value = (payload ^ 0xD895AF6471C302BE ^ (W << 48) ^ (H << 32)
             ^ (int(team == 'B') << 63)) & mask
    value = ((value ^ (value >> 30)) * 0xBF58476D1CE4E5B9) & mask
    value = ((value ^ (value >> 27)) * 0x94D049BB133111EB) & mask
    return (value ^ (value >> 31)) & PACKET_TAG_MASK

def make_packet(kind, owner, amount, cell, stamp, team):
    # round(9), cell(board-sized), amount(8), owner(14), kind(1), checksum.
    # At least 16 checksum bits on a 256x256 board; 20 on Big Empty.
    payload = ((stamp & 511) | ((cell & CELL_MASK) << 9)
               | ((amount & 255) << (9 + CELL_BITS))
               | ((owner & 16383) << (17 + CELL_BITS))
               | ((kind & 1) << (31 + CELL_BITS)))
    return payload | (packet_tag(payload, team) << PACKET_BITS)

def coordination_map(tile_lines):
    """Classify the sonar policy without prematurely changing navigation."""
    if KNOWN is not None:
        return KNOWN.get('key')
    if not CANDS or any(e['key'] not in ('trauma', 'stronghold') for e in CANDS):
        return None
    observed = []
    for line in tile_lines:
        x, y, pearl, timer = line.split()
        observed.append((int(y) * W + int(x), int(timer) >= 0))
    matches = [e['key'] for e in CANDS
               if all(present == (e['rates'][e['beds'][cell]] > 0) for cell, present in observed)]
    return matches[0] if len(matches) == 1 else None


def execute_turn(ct, game):
    global ORDER, LAST_ROUND, POST_LENGTH, ACTION_KIND, GOAL, GOAL_SINCE
    global QUEEN_FEEDABLE
    QUEEN_FEEDABLE = True
    global STEPS, TELE, LAST_EAT
    global HIST
    global KING_FROM, END_GROW, GROW_BY_LENGTH, BALANCED_KINGS, CONSOLIDATE_FROM
    rnd = game.round_num
    turns_left = max(1, 500 - rnd)
    LAST_ROUND, POST_LENGTH, ACTION_KIND = rnd, ct.length, 'move'
    global AIM, AIM_DIST, COORD_MAP
    AIM, AIM_DIST = -1, 0
    vis = ct.vision
    my_id = ct.head.dragon_id
    my_team = ct.head.team.value
    IS_Q = QUEEN_MODE and my_id in (0, 1)
    ORDER = (2, 3, 0, 1) if my_team == "B" and SMALL else (0, 1, 2, 3)
    profile = KNOWN.get("key") if KNOWN is not None else None
    FACE_IDX = 'NESW'.index(ct.head.dir.value)
    STRAIGHT_BONUS = STRAIGHT_MAPS.get(profile, 0)
    if my_team == "B" and profile == "schooltime":
        ORDER = (2, 3, 0, 1)
    route_guard = True
    avoid_loops = profile in ("arena", "trophy")
    L = ct.length
    free_me = (L + 3) // 4
    last = ct.unit_count <= 1     # trading our last dragon ends the game for us
    space_cap = 40 if FIRST else SPACE_BFS_CAP
    target_cap = 90 if FIRST else TARGET_BFS_CAP
    race_cap = 70 if FIRST else 160
    if IS_Q:
        # Existing v99 replays hit the 100M instruction ceiling. Keep queen
        # searches smaller than colony searches, including the first turn.
        space_cap = min(space_cap, 24)
        target_cap = min(target_cap, 60)
        race_cap = min(race_cap, 40)

    # ---------------- parse vision (raw lines, cheap)
    hl = "".join(vis._edge_lines[:VS + 1]).split()
    vl = "".join(vis._edge_lines[VS + 1:]).split()
    tl = vis._tile_lines
    global PORTAL_PENDING
    if PORTAL_PENDING is not None:
        source, direction, stamp, prior_length = PORTAL_PENDING
        x, y, _, _ = tl[24].split()
        landed = int(y) * W + int(x)
        if rnd == stamp + 1:
            HIST.append(landed)
            if ct.length > prior_length:
                LAST_EAT = STEPS
            OBSERVED_LINKS[source, direction] = landed
            OBSERVED_LINKS[landed, OPP[direction]] = source
            set_nb(source, direction, landed)
            set_nb(landed, OPP[direction], source)
        PORTAL_PENDING = None
    if KNOWN is None and CANDS:
        identify(tl, hl, vl)
    if COORD_MAP is None or KNOWN is not None:
        COORD_MAP = coordination_map(tl)
    # Equal-valued routes need different tie breaks on open water and farms.
    # Apply after identification so newborns use their map's policy immediately.
    route_map = KNOWN.get('key') if KNOWN is not None else None
    if route_map == 'big_empty':
        target_cap = min(target_cap, 160)
        forward = 'NESW'.index(ct.head.dir.value)
        ORDER = (forward, (forward + 1) % 4, (forward + 3) % 4, (forward + 2) % 4)
    elif (route_map in ('stronghold', 'trauma') or
          (route_map is None and CANDS and all(e['key'] in ('stronghold', 'trauma') for e in CANDS))):
        bias = ((my_id * 2654435761) >> 7) & 3
        ORDER = (bias, (bias + 1) % 4, (bias + 3) % 4, (bias + 2) % 4)
    if route_map == 'default_small' and my_team == 'A':
        ORDER = (2, 1, 3, 0)
    if route_map == 'stronghold' and my_team == 'A':
        KING_FROM, END_GROW, CONSOLIDATE_FROM = 250, 350, 370  # (Li 57) merge 80 rounds earlier
        GROW_BY_LENGTH = BALANCED_KINGS = True
    if route_map == 'stronghold':
        # Reserve the last 200 rounds for growth in the narrow farm corridors.
        END_GROW = 300
    if route_map == 'trauma':
        END_GROW = 270
    if route_map in ('arena', 'Colosseum'):
        ORDER = (3, 0, 1, 2) if my_team == 'B' else (1, 2, 3, 0)
    head = -1
    pearls_view = []
    view = set()
    for k, line in enumerate(tl):
        x, y, hp, pt = line.split()
        i = int(y) * W + int(x)
        r = k // VS
        if KNOWN is None:
            tn, te, ts, tw = hl[k], vl[k + r + 1], hl[k + VS], vl[k + r]
            ec = (code(tn), code(te), code(ts), code(tw))
            if 2 in ec:
                ec = list(ec)
                for d, tok in ((0, tn), (1, te), (2, ts), (3, tw)):
                    if ec[d] == 2:
                        learn_portal(i, d, tok)
                        if tok in LINKED or (i, d) in OBSERVED_LINKS:
                            ec[d] = 0
                ec = tuple(ec)
            EDGE[i] = ec
        else:
            if ((tok_class(hl[k]), tok_class(vl[k + r + 1]), tok_class(hl[k + VS]), tok_class(vl[k + r])) != expected(KNOWN, i)
                    or (int(pt) >= 0) != (KNOWN['rates'][KNOWN['beds'][i]] > 0)):
                forget_map()
                return execute_turn(ct, game)
        pt = int(pt)
        if (KNOWN is None and pt > 0 and SEEN[i] == rnd - 1 and
                BED[i] == rnd):
            # This tile was watched while its countdown reset. The new due
            # round minus the old one is an observed gap, not a replay seed.
            gap = pt
            old_gap = GAP_EST[i]
            GAP_EST[i] = gap if old_gap == 0 else (3 * old_gap + gap) // 4
        if hp == "1":
            PEARL[i] = True
            BED[i] = -1 if pt < 0 else rnd + pt
            pearls_view.append(i)
        else:
            PEARL[i] = False
            BED[i] = -1 if pt < 0 else rnd + pt
        SEEN[i] = rnd
        view.add(i)
        if k == 24:
            head = i

    occ = {}          # cell -> (team, id, dir_index, is_head)
    vis_len = {}
    for line in vis._body_lines:
        team, did, x, y, facing, ish = line.split()
        did = int(did)
        c = int(y) * W + int(x)
        occ[c] = (team, did, "NESW".index(facing), ish == "1")
        vis_len[did] = vis_len.get(did, 0) + 1
        if did != my_id:
            LASTOCC[c] = rnd

    visible_allies_pre = {o_[1] for c_, o_ in occ.items() if o_[0] == my_team and o_[3] and o_[1] != my_id}
    my_q = my_id if IS_Q else None
    if QUEEN_MODE:
        for c_, o_ in occ.items():
            if o_[1] in (0, 1):
                QTEAM[o_[1]] = o_[0]
        if my_q is None:
            for k_, t_ in QTEAM.items():
                if t_ == my_team:
                    my_q = k_
            if my_q is None:
                for k_, t_ in QTEAM.items():
                    if t_ != my_team:
                        my_q = 1 - k_
    QUEEN_ON = QUEEN_MODE and my_q is not None and (IS_Q or my_q in ANNOUNCED or my_q in visible_allies_pre)
    global MY_Q
    MY_Q = my_q
    enemy_q = (1 - my_q) if my_q is not None else None
    enemy_q_head = None
    if enemy_q is not None:
        for c_, o_ in occ.items():
            if o_[3] and o_[1] == enemy_q and o_[0] != my_team:
                enemy_q_head = c_
    # Pocket farming (Li63). Ladder replays M389785-M393193: on Autarky and Trauma the fast tiles sit in
    # short dead-end pockets at the end of one-wide tubes; the opponent walks in, eats, and splits all
    # but two segments out through its tail (Autarky pockets: 1,760 pearls to our 193). We split our
    # 14-long Autarky starter at round 0 instead. Local vs Li62 (C++ build of the same rule): Autarky 8-0.
    route_key = KNOWN.get('key') if KNOWN is not None else None
    # Keep a minority of established collectors as insurance for the longest-
    # dragon tiebreak. Missing a queen report does not prove she is dead.
    reserve_grower = (not IS_Q and winner_policy.reserve(
        route_key, rnd, L, ct.unit_count, my_id))
    # Protect existing length from routine halving, but retain only a minority
    # as queen insurance. Other established collectors may feed her safely.
    queen_reserve = reserve_grower and L >= 10 and (my_id // 2) % 6 == 1
    if (POCKET_FARM and not IS_Q and not reserve_grower and route_key not in NO_POCKET_MAPS and head >= 0 and L >= 4 and rnd < 496 and EDGE[head] is not None
            and route_key != 'dilemma' and not (W == 63 and H == 27)):   # off on Prisoners Dilemma (lost 0-4 locally; eliminated by round 58 vs a top-20 team with it on) and Slithery Fight (1-3)   # off on Prisoners Dilemma (lost 0-4) and Slithery Fight (1-3)
        facing = 'NESW'.index(ct.head.dir.value)
        back = (facing + 2) % 4

        def can_step(src, dst):
            return any(open_dir(src, dd) and nbs(src)[dd] == dst for dd in range(4))

        def exits_of(c, bk):
            # anything but kelp is a way on: a portal leads somewhere, so it is not a dead end
            e = EDGE[c]
            return [d for d in range(4) if d != bk and (e is None or e[d] != 1)]

        ex = exits_of(head, back)
        if not ex and ct.can_split(L - 2):
            ct.do_split(L - 2)
            POST_LENGTH, ACTION_KIND = 2, 'split'
            return
        if len(ex) == 1 and L >= 8 and EDGE[head][ex[0]] == 0 and ct.can_split(L - 2):
            # Inside a one-way passage whose way on is blocked (a team mate or a corpse in the pocket):
            # if the passage still ends in a dead end, leave through the tail now instead of letting
            # the normal rules halve the chain (ladder M437009: our 17-long Autarky chain split 8/9 at
            # round 29 and never recovered while the opponent's grew from 18 to 68).
            n1 = nbs(head)[ex[0]]
            # Li73: also when another head sits next to the way on (ladder Autarky 0/11: opponents raid our
            # pockets, the chain would not step next to them and the normal rules halved it, e.g. 17 -> 8/9)
            # Li77: only a head that can actually step onto n1 next turn counts (ladder M546613: enemy heads in
            # the next column, behind the tube's kelp wall, made a pocket chain split itself to pieces)
            if n1 in occ or any(o[3] and o[1] != my_id and can_step(oc, n1) for oc, o in occ.items()):
                c, d, dead = head, ex[0], False
                for _ in range(POCKET_MAX):
                    n = nbs(c)[d]
                    if EDGE[n] is None:
                        break
                    e2 = exits_of(n, (d + 2) % 4)
                    if not e2:
                        dead = True
                        break
                    if len(e2) != 1 or EDGE[n][e2[0]] != 0:
                        break
                    c, d = n, e2[0]
                if dead:
                    ct.do_split(L - 2)
                    POST_LENGTH, ACTION_KIND = 2, 'split'
                    return
        firsts = []
        if open_dir(head, facing):
            firsts.append(facing)
        for d0 in ORDER:
            if d0 != back and d0 != facing and open_dir(head, d0):
                firsts.append(d0)
        for first_d in firsts:
            if EDGE[head][first_d] != 0:
                continue
            c, d, depth, food = head, first_d, 0, 0
            dead, clear = False, True
            for _ in range(POCKET_MAX):
                n = nbs(c)[d]
                if n not in view or EDGE[n] is None or n in occ:
                    clear = False
                    break
                depth += 1
                if PEARL[n] or (BED[n] is not None and BED[n] >= 0 and BED[n] <= rnd + depth + 1):
                    food += 1
                e2 = exits_of(n, (d + 2) % 4)
                if not e2:
                    dead = True
                    break
                if len(e2) != 1 or EDGE[n][e2[0]] != 0:
                    break
                c, d = n, e2[0]
            need_food = 0 if len(ex) == 1 else 2
            if not (dead and clear and food >= need_food and L >= depth + 3):
                continue
            n0 = nbs(head)[first_d]
            if any(o[3] and o[1] != my_id and can_step(oc, n0) for oc, o in occ.items()):
                continue
            go(ct, head, [first_d])
            return

    enemy_heads = []
    other_heads = []
    for c, (team, did, di, ish) in occ.items():
        if ish and did != my_id:
            other_heads.append((c, did))
            if team != my_team:
                enemy_heads.append((c, did))
    n_friend_heads = len(other_heads) - len(enemy_heads)
    for c, did in other_heads:
        if occ[c][0] == my_team:
            old = ALLY_LENGTHS.get(did)
            measured = vis_len[did]
            if old is not None and rnd - old[1] <= 2:
                measured = max(measured, old[0] - 2)
            ALLY_LENGTHS[did] = (measured, rnd)
    for owner in list(CLAIMS):
        if rnd - CLAIMS[owner][2] > 2:
            del CLAIMS[owner]
    visible_heads = {did: c for c, did in other_heads}
    visible_allies = {did for c, did in other_heads if occ[c][0] == my_team}
    # Bound work under sonar flooding and sample across the received batch.
    messages = ct.sonar_messages if CELL_BITS <= 16 else ()
    stride = max(1, (len(messages) + 95) // 96)
    offset = (rnd + my_id) % stride
    checked = set()
    for value in messages[offset::stride]:
        if value in checked or value < 0 or value > 0xffffffffffffffff:
            continue
        checked.add(value)
        payload = value & PACKET_MASK
        stamp = payload & 511
        if not 0 <= rnd - stamp <= 3:
            continue
        if value >> PACKET_BITS != packet_tag(payload, my_team):
            continue
        kind = (payload >> (31 + CELL_BITS)) & 1
        did = (payload >> (17 + CELL_BITS)) & 16383
        amount = (payload >> (9 + CELL_BITS)) & 255
        cell = (payload >> 9) & CELL_MASK
        if did == my_id or cell >= N:
            continue
        if kind:
            if did in visible_allies and amount <= 127 and rnd - stamp <= 2:
                old = CLAIMS.get(did)
                if old is None or stamp >= old[2]:
                    CLAIMS[did] = (cell, amount, stamp)
            continue
        ln = amount
        if ln < 2 or ln > N:
            continue
        observed_head = visible_heads.get(did)
        if observed_head is not None:
            if did not in visible_allies:
                continue
            cell = observed_head
            ln = max(ln, vis_len[did])
        elif cell in view:
            occupant = occ.get(cell)
            if occupant is None or occupant[1] != did or occupant[0] != my_team or not occupant[3]:
                continue
        elif did not in ANNOUNCED:
            pending = PENDING_REPORTS.get(did)
            if pending is None or not 0 < stamp - pending[2] <= 3:
                if pending is None or stamp > pending[2]:
                    PENDING_REPORTS[did] = (ln, cell, stamp)
                continue
        # Relays retain the original timestamp and cannot revive a vanished
        # grower indefinitely. Reject packets that roll its state backwards.
        previous = ANNOUNCED.get(did)
        if previous is not None and stamp < previous[2]:
            continue
        ANNOUNCED[did] = (ln, cell, stamp)
        if did in (0, 1):
            QTEAM[did] = my_team
        PENDING_REPORTS.pop(did, None)
        if did in visible_allies and rnd - stamp <= 1:
            ALLY_LENGTHS[did] = (max(ln, vis_len.get(did, 0)), rnd)
    for did in list(ANNOUNCED):
        ln, cell, stamp = ANNOUNCED[did]
        # A newly observed empty cell invalidates a stationary old report.
        # A directly seen ally can be tracked to its actual head instead.
        actual = visible_heads.get(did)
        if rnd - stamp > 3:
            del ANNOUNCED[did]
        elif actual is not None and did in visible_allies:
            ANNOUNCED[did] = (ln, actual, stamp)
        elif cell in view:
            del ANNOUNCED[did]
    for did in list(PENDING_REPORTS):
        if rnd - PENDING_REPORTS[did][2] > 3:
            del PENDING_REPORTS[did]
    if len(ALLY_LENGTHS) > 100:
        for did in list(ALLY_LENGTHS):
            if rnd - ALLY_LENGTHS[did][1] > 12:
                del ALLY_LENGTHS[did]

    # A checked remote queen report can identify her team without direct sight.
    # Recompute after parsing this turn's messages; IDs do not imply A/B.
    if QUEEN_MODE and not IS_Q:
        for qid in (0, 1):
            if QTEAM.get(qid) == my_team:
                my_q = qid
                break
    MY_Q = my_q
    QUEEN_ON = QUEEN_MODE and my_q is not None and (IS_Q or my_q in ANNOUNCED or my_q in visible_allies_pre)
    # A sealed, bed-free queen room cannot receive outside donations. In a
    # queen-length tie, outside collectors must still consolidate their length.
    queen_cell = visible_heads.get(my_q)
    if queen_cell is None and my_q in ANNOUNCED:
        queen_cell = ANNOUNCED[my_q][1]
    if not IS_Q and QUEEN_ON and queen_cell is not None:
        room = {queen_cell}
        frontier = [queen_cell]
        sealed = True
        for cell in frontier:
            edges = EDGE[cell]
            no_bed = (BED[cell] == -1 if cell in view else
                      KNOWN is not None and RATE[BEDC[cell]] == 0)
            if edges is None or not no_bed or 2 in edges:
                sealed = False
                break
            for direction in ORDER:
                if edges[direction] == 1:
                    continue
                dest = nbs(cell)[direction]
                # A portal or unexplored boundary invalidates this certificate.
                if (dest != plain_neighbor(cell, direction) or EDGE[dest] is None):
                    sealed = False
                    break
                if dest not in room:
                    room.add(dest)
                    frontier.append(dest)
            if not sealed or len(room) > 8:
                sealed = False
                break
        if sealed and head not in room:
            QUEEN_FEEDABLE = False
            QUEEN_ON = False
    enemy_q = 1 - my_q if my_q is not None else None
    enemy_q_head = next((c for c, o in occ.items() if o[3] and o[1] == enemy_q and o[0] != my_team), None)

    # ---------------- own body order (head = rank 0)
    rank = {head: 0}
    cur = head
    k = 0
    while True:
        nxt = -1
        for d in ORDER:
            n = nbs(cur)[d]
            o = occ.get(n)
            if o is None or o[1] != my_id or o[3] or n in rank:
                continue
            if nbs(n)[o[2]] == cur:
                nxt = n
                break
        if nxt < 0:
            break
        k += 1
        rank[nxt] = k
        cur = nxt

    # our body beyond the 7x7 window (it can trail through a portal): recover it from our own path
    if not HIST or HIST[-1] != head:
        HIST = [c for c, r in sorted(rank.items(), key=lambda z: -z[1])]
    elif len(HIST) > L + 8:
        HIST = HIST[-(L + 8):]
    far_body = {}
    nh = len(HIST)
    for j in range(1, min(L, nh)):
        c = HIST[nh - 1 - j]
        if c not in rank and c not in occ:
            far_body[c] = j

    own_rank = rank | far_body

    sprint_old_body_cache = {}

    def sprint_body(cells, remaining):
        # Occupancy BEFORE the next substep, including the current tail.
        # Paid shrink and food from completed substeps are reflected in remaining.
        steps = len(cells) - 1
        body = set(cells[-remaining:])
        key = (steps, remaining)
        if key not in sprint_old_body_cache:
            sprint_old_body_cache[key] = frozenset(c for c, r in own_rank.items()
                                                    if r + steps < remaining)
        body.update(sprint_old_body_cache[key])
        return body

    npearl = 0
    for p in pearls_view:
        if tdist(p, head) <= 3:
            npearl += 1
    eat_margin = 1 + min(npearl, 4)

    # block[c] = first move number at which c may be entered (absent = free, 10**6 = never)
    block = {}
    for c, o in occ.items():
        if o[1] == my_id:
            r = rank.get(c)
            block[c] = 10 ** 6 if r is None else L - r + eat_margin + 1
        else:
            block[c] = 10 ** 6
    for c, r in far_body.items():
        block[c] = L - r + eat_margin + 1

    # Paths to pearls must respect walls and portals. Manhattan distance made
    # neighbours on the other side of a wall look like they had won the race.
    rival_dist = {}
    race_q = []
    for c, did in sorted(other_heads, key=lambda z: z[1]):
        rival_dist[c] = (0, did)
        race_q.append(c)
    race_i = 0
    while race_i < len(race_q) and race_i < race_cap:
        c = race_q[race_i]
        race_i += 1
        dc, did = rival_dist[c]
        if dc >= 6:
            continue
        for d in ORDER:
            if not open_dir(c, d):
                continue
            n = nbs(c)[d]
            if n in rival_dist or n in block:
                continue
            rival_dist[n] = (dc + 1, did)
            race_q.append(n)

    # A later enemy acts after the queen has moved. Old tail cells can already
    # be empty; do not use them as permanent barriers in its reach forecast.
    queen_steps = 3 if FIRST else 5
    queen_tail_from = max(0, L - queen_steps - max(0, queen_steps - free_me))

    def threat_can_continue(cell):
        return (cell not in block or (IS_Q and cell in own_rank
                                     and own_rank[cell] >= queen_tail_from))

    def visible_enemy_length(cell, did):
        # Certify only a complete short chain, including its tail's visible
        # neighbourhood. Partial bodies retain the conservative long estimate.
        length = vis_len.get(did, 0)
        if not 2 <= length <= 5:
            return None
        chain = {cell}
        while len(chain) <= 5:
            predecessors = []
            edges = EDGE[cell]
            if edges is None or 2 in edges:
                return None
            for direction in ORDER:
                if edges[direction] == 1:
                    continue
                dest = nbs(cell)[direction]
                if dest not in view or dest != plain_neighbor(cell, direction):
                    return None
                occupant = occ.get(dest)
                if (occupant is not None and occupant[1] == did
                        and not occupant[3] and nbs(dest)[occupant[2]] == cell
                        and dest not in chain):
                    predecessors.append(dest)
            if not predecessors:
                return length if len(chain) == length else None
            if len(predecessors) != 1:
                return None
            cell = predecessors[0]
            chain.add(cell)
        return None

    danger = {}
    coarse_enemies = []
    threat_horizon = 6 if IS_Q else SPRINT_REACH
    head_zone = set()
    friendly_head_zone = set()
    for c, did in other_heads:
        if turns_left == 1 and did < my_id:
            continue
        e = EDGE[c]
        for d in ORDER:
            if e is None or e[d] == 0:
                n = nbs(c)[d]
                head_zone.add(n)
                if occ[c][0] == my_team:
                    friendly_head_zone.add(n)
    # Bounded sprint threats include food collected before paying for a later
    # step. Lower IDs have already acted on the final round.
    for c, did in enemy_heads:
        if turns_left == 1 and did < my_id:
            continue
        vl_ = max(2, vis_len.get(did, 1))
        certified_length = visible_enemy_length(c, did) if IS_Q else None
        if IS_Q and certified_length is None:
            coarse_enemies.append((c, max(vl_, 6)))
            continue
        if certified_length is not None:
            vl_ = certified_length
        free_e = (vl_ + 3) // 4
        fr = [(c, vl_)]
        best_remaining = {c: vl_}
        for depth in range(1, threat_horizon + 1):
            nf = []
            for x, remaining in fr:
                if depth > free_e and remaining <= 2:
                    continue
                e = EDGE[x]
                for d in ORDER:
                    if e is not None and e[d] != 0:
                        continue
                    n = nbs(x)[d]
                    # Unseen food remains possible for a queen's threat forecast.
                    gain = int((IS_Q and n not in view) or recent_pearl(n, rnd))
                    nr = remaining + gain - int(depth > free_e)
                    if best_remaining.get(n, -1) >= nr:
                        continue
                    best_remaining[n] = nr
                    old = danger.get(n)
                    if old is None or old[0] > depth or (old[0] == depth and old[1] < vl_):
                        danger[n] = (depth, vl_)
                    if threat_can_continue(n):
                        nf.append((n, nr))
            fr = nf

    if coarse_enemies:
        # Share overlapping waves: nearest threat distance is unchanged, while
        # a portal region is explored once rather than once per enemy head.
        wave = list(dict.fromkeys(c for c, _ in coarse_enemies))
        visited_threat = set(wave)
        threat_length = max(v for _, v in coarse_enemies)
        for depth in range(1, threat_horizon + 1):
            following = []
            for cell in wave:
                for direction in ORDER:
                    if not open_dir(cell, direction):
                        continue
                    dest = nbs(cell)[direction]
                    if dest in visited_threat:
                        continue
                    visited_threat.add(dest)
                    old = danger.get(dest)
                    if old is None or old[0] > depth or (old[0] == depth and old[1] < threat_length):
                        danger[dest] = (depth, threat_length)
                    if threat_can_continue(dest):
                        following.append(dest)
            wave = following

    role_id = my_id - (my_team == "B") if BALANCED_KINGS else my_id
    grower_stride = 16 if route_map == 'big_empty' else KING_MOD
    king = ((role_id % grower_stride == 0) or (GROW_BY_LENGTH and L >= 8)) and rnd >= KING_FROM and rnd < LATE_GAME and ct.unit_count >= GROWER_MIN_POP
    if route_map == 'big_empty' and rnd >= 250 and L >= 12 and ct.unit_count >= 12:
        king = True
    prime = route_map in PRIME_MAPS and IS_Q
    if prime:
        # Trauma: our first dragon never splits and grows all game. Ladder: the opponent kept one
        # dragon from the start (13 at round 200, 34 at 500) and beat our 125-length colony whose
        # merges produced three dragons of ~20.
        king = True
    # All large survivors become growers; small neighbours donate their pearls.
    if rnd >= CONSOLIDATE_FROM:
        king = L >= 8
    if IS_Q or reserve_grower:
        king = True
    he = EDGE[head]
    cons_from = Q_DONATE_FROM if QUEEN_ON else CONSOLIDATE_FROM
    protected_queen_reserve = QUEEN_ON and queen_reserve
    reserve_qa = ANNOUNCED.get(my_q) if protected_queen_reserve else None
    late_reserve_feed = (protected_queen_reserve and rnd >= Q_RESERVE_FEED_FROM
                         and my_q in visible_allies and reserve_qa is not None
                         and 0 <= rnd - reserve_qa[2] <= 1
                         and max(vis_len.get(my_q, 0), reserve_qa[0]) >= 8)
    if (rnd >= cons_from and (ct.unit_count >= 4 or rnd >= FINAL_FROM)
            and not last and not IS_Q and (not protected_queen_reserve or late_reserve_feed)):
        recipient = None
        for c, did in other_heads:
            if occ[c][0] != my_team:
                continue
            if QUEEN_ON:
                # only the queen's length counts: feed her and nobody else
                if did == my_q and tdist(head, c) <= (LATE_DONATE_RADIUS if rnd >= LATE_DONATE_FROM else DONATE_RADIUS) \
                        and not any(tdist(ec, c) <= 4 or tdist(ec, head) <= 3 for ec, _ in enemy_heads):
                    recipient = ((10 ** 6, -did), c)
                continue
            fl = max(vis_len[did], ALLY_LENGTHS.get(did, (0, 0))[0])
            # An ID tie-break, when enabled, prevents mutual donations.
            better = (fl, -did) > (L, -my_id) if ALLOW_EQUAL_DONORS else fl > L
            if not better or tdist(head, c) > (LATE_DONATE_RADIUS if rnd >= LATE_DONATE_FROM else DONATE_RADIUS):
                continue
            if fl < L + (0 if L <= 4 else 2):
                continue
            if route_map == 'big_empty' and rnd < 480 and fl < max(12, L + 4):
                continue
            if any(tdist(ec, c) <= 4 or tdist(ec, head) <= 3 for ec, _ in enemy_heads):
                continue
            if recipient is None or (fl, -did) > recipient[0]:
                recipient = ((fl, -did), c)
        if recipient is not None:
            # Ensure a short passable path exists: geometric neighbours can be
            # on opposite sides of a wall or disconnected by portals.
            start = recipient[1]
            reach = {start}
            frontier = [start]
            travel_budget = min(LATE_DONATE_RADIUS if rnd >= LATE_DONATE_FROM else DONATE_RADIUS,
                                max(0, turns_left - 1))
            for _ in range(travel_budget):
                nxt = []
                for c in frontier:
                    for d in ORDER:
                        n = nbs(c)[d]
                        if late_reserve_feed and (n not in view or EDGE[c] is None
                                                  or EDGE[c][d] != 0 or n != plain_neighbor(c, d)):
                            continue  # insurance needs a visible path without portal uncertainty
                        if open_dir(c, d) and n not in reach and (n not in occ or occ[n][1] == my_id):
                            reach.add(n)
                            nxt.append(n)
                frontier = nxt
            if head in reach:
                for d in ORDER:
                    if (rank.get(nbs(head)[d]) == 1 and open_dir(head, d)
                            and (not late_reserve_feed or nbs(head)[d] == plain_neighbor(head, d))):
                        go(ct, head, [d])
                        ACTION_KIND = 'donate'
                        return
    global CONS
    if FEED_FROM <= 430 and rnd >= CONS_FROM and ct.unit_count >= CONS_MIN_UNITS:
        CONS = True
    aggressive = CONS and ct.unit_count >= 6 and not QUEEN_ON
    if ((aggressive or (rnd >= FEED_FROM and not king and L <= 6 and ct.unit_count >= 4))
            and not last and not IS_Q and not (reserve_grower and L >= 10)):
        # the round-500 check only counts our longest dragon: a short dragon beside a much longer
        # team mate is worth more as pearls on that dragon's doorstep than alive
        feed = None
        for c, did in other_heads:
            if occ[c][0] != my_team:
                continue
            fl = vis_len.get(did, 1)
            ok_ = (fl > L or (fl == L and did < my_id)) if aggressive else fl >= L + 5
            if ok_ and tdist(head, c) <= 3 and (feed is None or fl > feed[0]):
                feed = (fl, c)
        if feed is not None:
            safe = True
            for c, did in enemy_heads:
                if tdist(c, feed[1]) <= 5:
                    safe = False
            if safe:
                # step back into our own neck: only we die, and our body turns into pearls here
                back = -1
                for d in ORDER:
                    if nbs(head)[d] in rank and rank[nbs(head)[d]] == 1:
                        back = d
                if back >= 0:
                    go(ct, head, [back])
                    return
    kpull = None
    if aggressive and not last:
        for c, did in other_heads:
            if occ[c][0] == my_team:
                fl = vis_len.get(did, 1)
                if (fl > L or (fl == L and did < my_id)) and tdist(head, c) > 3 and (kpull is None or fl > kpull[0]):
                    kpull = (fl, c)
    if kpull is None and rnd >= FINAL_FROM - 20 and not last and ANNOUNCED:
        # Final rounds: live, the opponent ended with 46-52 from the same total length we spread over
        # 3-19 dragons of 10-35. Head toward our longest dragon we have heard of so donation can merge us.
        best = None
        for did, (ln, cell, _) in ANNOUNCED.items():
            if ln > L + 2 and 2 < tdist(head, cell) <= FINAL_PULL_RANGE and (best is None or ln > best[0]):
                best = (ln, cell)
        kpull = best
    if kpull is None and rnd >= PULL_FROM and not king and not last and ANNOUNCED:
        # converge on the longest dragon we have heard of so consolidation (round 400+) finds a recipient
        best = None
        for did, (ln, cell, _) in ANNOUNCED.items():
            if ln >= max(2 * L, L + 4) and 3 < tdist(head, cell) <= PULL_RANGE and (best is None or ln > best[0]):
                best = (ln, cell)
        kpull = best
    if QUEEN_ON:
        kpull = None
        if not IS_Q and not queen_reserve and rnd >= Q_PULL_FROM and not last:
            qa = ANNOUNCED.get(my_q)
            if qa is not None and 2 < tdist(head, qa[1]) <= 90:
                kpull = (qa[0], qa[1])
    hunt = None
    if rnd >= HUNT_FROM and not king and not last and ct.unit_count >= 6 and not QUEEN_ON:
        # trading a short dragon for a long enemy can decide the round-500 longest-dragon check
        for c, did in enemy_heads:
            el = vis_len.get(did, 1)
            if el >= L + 1 and (hunt is None or el > hunt[0]):
                hunt = (el, c)

    # ---------------- 1. head trades, including sprint strikes (targets cannot dodge)
    att_margin = SMALL_ATTACK_MARGIN if SMALL else ATTACK_MARGIN
    if route_map is None:
        # An equal head trade kills both dragons. The replay lost 24 enemy
        # collisions initiated by our own collectors, including length-3 ties.
        att_margin = max(att_margin, 3 if ct.unit_count <= 12 else 2)
    if route_map == 'queen_of_spades':
        att_margin = max(att_margin, 2)
    elif route_map == 'schooltime' and ct.unit_count <= 12:
        # A depleted outside colony needs collectors; keep established pressure.
        # Queen strikes retain their separate priority; ordinary trades need +2.
        att_margin = max(att_margin, 2)
    elif route_map == 'default' and my_team == 'B':
        att_margin = max(att_margin, 1)
    if ct.unit_count <= 3:
        att_margin = max(att_margin, 3)
    if route_map in ('devil', 'default_small') and ct.unit_count <= 12:
        att_margin = max(att_margin, 3)
    if route_map == 'default' and ct.unit_count <= 12:
        att_margin = max(att_margin, 2)
    if route_map == 'default' and L >= 4 and rnd < END_GROW and ct.unit_count < UNITS_CAP:
        att_margin = max(att_margin, 4)
    if rnd >= END_GROW and L >= END_SAFE_LEN:
        att_margin = max(att_margin, END_SAFE_MARGIN)
    best_attack = None
    # A pearl collected on an earlier sprint step can pay for a later step.
    # Conversely, payment is checked before landing: length 2 cannot sprint
    # onto a pearl on step two. Track the actual intermediate length.
    maxd = 4
    min_gain = max(att_margin, max(3, L // 2)) if king and L >= 5 else att_margin
    strike = Q_STRIKE and not IS_Q and enemy_q_head is not None
    worthwhile = strike or (any(vis_len.get(did, 1) >= L + min_gain for _, did in enemy_heads) and not IS_Q)
    if IS_Q:
        worthwhile = False
    fr = [(head, L, (), (head,))] if worthwhile and not last else []
    visited = {head: L}
    if strike:
        maxd = 6
    for depth in range(1, maxd + 1):
        nf = []
        for x, rem, route, cells in fr:
            if depth > free_me and rem <= 2:
                continue
            e = EDGE[x]
            live_body = sprint_body(cells, rem)
            for d in ORDER:
                if e is not None and e[d] != 0:
                    continue
                n = nbs(x)[d]
                if n not in view or n in live_body:
                    continue
                o = occ.get(n)
                if o is not None and (o[1] != my_id or n not in own_rank):
                    if o[3] and o[0] != my_team:
                        gain = vis_len.get(o[1], 1) - L
                        if strike and o[1] == enemy_q:
                            gain = 10 ** 6
                        if gain >= att_margin and (best_attack is None or (gain, -depth) > best_attack[0]):
                            best_attack = ((gain, -depth), route + (d,))
                    continue
                nr = rem + int(recent_pearl(n, rnd) and n not in cells) - (depth > free_me)
                if visited.get(n, -1) >= nr:
                    continue
                visited[n] = nr
                nf.append((n, nr, route + (d,), cells + (n,)))
        fr = nf
    if king and L >= 5 and best_attack is not None and best_attack[0][0] < max(3, L // 2):
        best_attack = None
    if best_attack is not None and not last:
        go(ct, head, best_attack[1])
        return

    # ---------------- 2. space check
    space_cache = {}

    def space_from(s, cap):
        cache_key = (s, cap)
        if cache_key in space_cache:
            return space_cache[cache_key]
        seen = set()
        seen.add(s)
        q = [s]
        qt = [1]
        qi = 0
        while qi < len(q) and qi < cap:
            c = q[qi]
            t = qt[qi] + 1
            qi += 1
            e = EDGE[c]
            for d in ORDER:
                if e is not None and e[d] != 0:
                    continue
                n = nbs(c)[d]
                if n in seen:
                    continue
                b = block.get(n)
                if b is not None and t < b:
                    continue
                seen.add(n)
                q.append(n)
                qt.append(t)
        space_cache[cache_key] = qi
        return qi

    need = min(L + 2, space_cap - 5)
    if not SMALL and need < MIN_NEED:
        need = MIN_NEED
    if IS_Q:
        need = max(need, 15)
    # Space beyond the final turn cannot justify sacrificing length now.
    need = min(need, turns_left)

    # Risk-aware routing helps collectors take safe detours toward food.
    # Keep the original travel-time routing in the two narrow farm layouts:
    # weighted detours there made collectors abandon their connected beds.
    if ((route_map is None and not enemy_heads) or route_map in ('stronghold', 'trauma', 'arena', 'Colosseum') or
            (route_map is None and CANDS and
             all(e['key'] in ('stronghold', 'trauma') for e in CANDS))):
        # ---------------- 3. food search from head
        dist = {head: 0}
        first = {head: -1}
        q = [head]
        qi = 0
        found = []
        while qi < len(q) and qi < target_cap:
            c = q[qi]
            qi += 1
            dc = dist[c]
            if c != head:
                if recent_pearl(c, rnd):
                    # An old sighting is less reliable than food we can see now.
                    age = max(0, rnd - SEEN[c])
                    found.append((dc + min(24, age // 4), c))
                else:
                    b = BED[c]
                    if b is not None and b >= rnd and b <= rnd + dc + (1 if enemy_heads else 8):
                        found.append((dc + 3 + max(0, b - rnd - dc), c))
            e = EDGE[c]
            fc = first[c]
            for d in ORDER:
                if e is not None and e[d] != 0:
                    continue
                n = nbs(c)[d]
                if n in dist:
                    continue
                b = block.get(n)
                if b is not None and dc + 1 < b:
                    continue
                dist[n] = dc + 1
                first[n] = d if dc == 0 else fc
                q.append(n)
    else:
        # ---------------- 3. food search from head
        dist = {head: 0}
        first = {head: -1}
        from heapq import heappop, heappush
        risk_unit = 4.0 if king or L >= 8 else 1.5
        route_risk = {cell: risk_unit / threat[0] for cell, threat in danger.items()}
        for cell in head_zone:
            route_risk.setdefault(cell, 0.5)
        costs = {head: 0.0}
        q = [(0.0, 0, head)]
        serial = 0
        qi = 0
        found = []
        while q and qi < target_cap:
            cost, _, c = heappop(q)
            if cost != costs[c]:
                continue
            qi += 1
            dc = dist[c]
            food_cost = cost
            if c != head:
                if recent_pearl(c, rnd):
                    # An old sighting is less reliable than food we can see now.
                    age = max(0, rnd - SEEN[c])
                    found.append((food_cost + min(24, age // 4), c))
                else:
                    b = BED[c]
                    if b is not None and b >= rnd and b <= rnd + dc + (1 if enemy_heads else 8):
                        found.append((food_cost + 3 + max(0, b - rnd - dc), c))
            e = EDGE[c]
            fc = first[c]
            for d in ORDER:
                if e is not None and e[d] != 0:
                    continue
                n = nbs(c)[d]
                b = block.get(n)
                if b is not None and dc + 1 < b:
                    continue
                risk = route_risk.get(n, 0.0)
                nc = cost + 1.0 + risk
                if nc >= costs.get(n, 1e9):
                    continue
                costs[n] = nc
                dist[n] = dc + 1
                first[n] = d if dc == 0 else fc
                serial += 1
                heappush(q, (nc, serial, n))
    if KNOWN is None or KNOWN.get('key') in ('default_small', 'default', 'schooltime'):
        # Keep nearest-food priority, but break equal-distance ties per dragon.
        # This avoids sending the colony toward the same pearl in lockstep.
        found.sort(key=lambda z: (z[0], ((z[1] * 2654435761) ^ (my_id * 2246822519)) & 65535))
    else:
        found.sort(key=lambda z: (z[0], N - 1 - z[1] if my_team == "B" and (SMALL or profile == "schooltime") else z[1]))
    dscore = [0.0, 0.0, 0.0, 0.0]
    picked = 0
    accepted_targets = {}
    best_contested = None
    for dc, c in found:
        fd = first[c]
        if fd < 0:
            continue
        margin = 0
        race = rival_dist.get(c)
        if race is not None and race[1] in CLAIMS:
            # An ally heading elsewhere is not automatically a food competitor.
            if CLAIMS[race[1]][0] != c:
                race = None
        claimed = any(target == c and (max(0, distance - (rnd - stamp)), owner) < (dist[c], my_id)
                      for owner, (target, distance, stamp) in CLAIMS.items())
        if claimed:
            continue
        if race is not None:
            dd, oid = race
            margin = dist[c] - dd - (0 if oid < my_id else -0.5)
        if (margin >= 2) if SMALL else (margin >= 1):
            if best_contested is None or dc < best_contested[0]:
                best_contested = (dc, fd)
            continue
        accepted_targets.setdefault(fd, c)
        dscore[fd] += (60.0 / (1 + dc)) * (1.0 if picked == 0 else 0.35)
        picked += 1
        if picked >= 4:
            break
    # nothing safe to eat: head for the nearest contested pearl so we keep growing
    if picked == 0 and best_contested is not None:
        dscore[best_contested[1]] += 25.0 / (1 + best_contested[0])
        picked = 1

    if KNOWN is None:
        # The replay's four fast beds were abandoned for distant unknown tiles.
        # Reward only gaps witnessed live, and cap the pull so imminent danger
        # and an actual visible pearl can still decide the next step.
        farm_pull = [0.0, 0.0, 0.0, 0.0]
        for c, dc in dist.items():
            if c == head or dc > 12 or SEEN[c] < rnd - 60:
                continue
            gap = GAP_EST[c]
            if 0 < gap <= 16:
                fd = first[c]
                if fd >= 0:
                    farm_pull[fd] += 72.0 * 4.0 / ((1 + dc) * (gap + 3.0))
        for d in ORDER:
            dscore[d] += min(60.0, farm_pull[d])

    explore_dir = -1
    farmer = route_map != 'trauma' and KNOWN is not None and (KNOWN.get('key') != 'queen_of_spades' or (IS_Q and rnd < QOS_FARM_UNTIL))
    if farmer and ZONEF is not None and rnd < ZONE_UNTIL and 'mid' in ZONEF and my_id % ZONE_ROLES == 0:
        farmer = False   # centre-role dragons ignore the edge-bed farm pull (Devil)
    if farmer and FARMD is not None and 0 < FARMD[head] <= FARM_RANGE and rnd < CONSOLIDATE_FROM:
        # Trauma / Stronghold: a line of beds that refill every round. Live (M114901) our colony never found
        # Trauma's and starved at 5 dragons for 350 rounds; locally, finding it took us from 6 to 33 dragons.
        he_ = EDGE[head]
        for d in ORDER:
            if he_ is not None and he_[d] != 0:
                continue
            n = nbs(head)[d]
            if FARMD[n] < FARMD[head]:
                dscore[d] += max(FARM_W / (1 + FARMD[head]), 12)
                break
    rush_on = RUSHD is not None and rnd < RUSH_UNTIL
    rush_near = RUSH_NEAR
    if route_map == 'trophy':
        rush_on = RUSHD is not None and rnd < TROPHY_UNTIL and TROPHY_MOD > 0 and my_id >= 2 and (my_id % TROPHY_MOD == 0 or my_id == 2) and my_id < TROPHY_IDMAX
        rush_near = TROPHY_NEAR
    if rush_on and RUSHD[head] <= rush_near:
        # Hold the richest spawn area (Default centre/corner squares, Trophy cup, Devil centre,
        # Queen of Spades spade, Schooltime pool): ladder opponents own these by round ~20.
        # Only nearby dragons race for it (pulling the whole colony crowded it: 2-8 vs Li57),
        # and dragons already inside stay and split there.
        he_ = EDGE[head]
        hd = RUSHD[head]
        for d in ORDER:
            if he_ is not None and he_[d] != 0:
                continue
            nd = RUSHD[nbs(head)[d]]
            if hd > 1 and nd < hd:
                dscore[d] += RUSH_W
            elif hd <= 1 and nd > 2:
                dscore[d] -= HOLD_W
    if ZONEF is not None and rnd < ZONE_UNTIL:
        # Devil: every third dragon (id % 3 == 0) heads for the centre's fast tiles and holds them instead
        # of following the edge-bed farm pull (ladder: we never contested the centre and were eliminated).
        # Tried on Default too (centre + own corner roles): lost 3-13 vs Li57, so Devil only.
        role = my_id % ZONE_ROLES
        zf = ZONEF.get('mid') if role == 0 else ZONEF.get(my_team) if role == 1 else None
        if zf is not None and zf[head] < 255:
            he_ = EDGE[head]
            hd = zf[head]
            for d in ORDER:
                if he_ is not None and he_[d] != 0:
                    continue
                nd = zf[nbs(head)[d]]
                if hd > 1 and nd < hd:
                    dscore[d] += ZONE_W
                elif hd <= 1 and nd > 2:
                    dscore[d] -= HOLD_W
    if picked == 0 and ZONED and POTX:
        bestv = None
        for c, dc in dist.items():
            if dc == 0:
                continue
            s = SEEN[c]
            if s < 0:
                rt = RATE[BEDC[c]] * (rnd if rnd > EXP_T else EXP_T)
                val = EXP_UNSEEN * (rt if rt < 0.9 else 0.9)
            else:
                val = 0.0
                b = BED[c]
                if b is not None and b >= rnd:
                    val += 30 - max(0, b - rnd - dc)
            val += EXP_POT * POT[c] - 2 * dc
            if bestv is None or val > bestv:
                bestv = val
                explore_dir = first[c]
    elif picked == 0:
        bestv = None
        for c, dc in dist.items():
            if dc == 0:
                continue
            s = SEEN[c]
            if s >= 0:
                if KNOWN is None:
                    # Once a fast reset has been witnessed, revisit that bed
                    # instead of always walking into unseen, often barren cells.
                    val = min(80, rnd - s) - 2 * dc
                    gap = GAP_EST[c]
                    if gap:
                        val += max(0, 200 - 6 * gap)
                else:
                    val = rnd - s - 2 * dc
            elif ZONED:
                val = 500 + 500 * RATE[BEDC[c]] / MAXRATE - 2 * dc
            else:
                val = (150 if KNOWN is None else 1000) - 2 * dc
            b = BED[c]
            if b is not None and b >= rnd:
                val += 30 - max(0, b - rnd - dc)
            if bestv is None or val > bestv:
                bestv = val
                explore_dir = first[c]

    # Follow future resource yield when immediate food does not settle the route.
    if KNOWN is not None and YIELD_FIELD is not None and rnd < CONSOLIDATE_FROM:
        for d in ORDER:
            n = nbs(head)[d]
            if open_dir(head, d):
                dscore[d] += 0.22 * (YIELD_FIELD[n] - YIELD_FIELD[head])

    # Brief target commitment avoids oscillation between equally useful pearls.
    if KNOWN is not None and KNOWN.get('key') == 'queen_of_spades' and GOAL in first and first[GOAL] >= 0 and rnd - GOAL_SINCE < 8:
        if recent_pearl(GOAL, rnd) and (GOAL in view or rnd - SEEN[GOAL] < 6):
            dscore[first[GOAL]] += 10 / (1 + dist[GOAL])
    # ---------------- 4. score moves
    # (Yueci Li 18) certify a short continuation using exact tail release and food growth
    def continuation(c, path, growth, step, visible_only=False):
        if step >= min(3, turns_left):
            return True
        e = EDGE[c]
        for d in ORDER:
            if e is not None and e[d] != 0:
                continue
            n = nbs(c)[d]
            if visible_only and n not in view:
                continue
            o = occ.get(n)
            r = own_rank.get(n)
            if o is not None and (o[1] != my_id or r is None):
                continue
            if r is not None and step + 1 <= L - r + growth:
                continue
            if any(p == n and step + 1 - j <= L + growth for j, p in enumerate(path)):
                continue
            if continuation(n, path + (n,), growth + int(recent_pearl(n, rnd) and n not in path), step + 1, visible_only):
                return True
        return False

    queen_pocket_cache = {}

    def queen_dead_end(start, previous):
        if turns_left <= 1:
            return False  # no next turn needs an exit
        key = (start, previous)
        if key in queen_pocket_cache:
            return queen_pocket_cache[key]
        seen = {previous}
        cell = start
        result = False
        for _ in range(min(12, turns_left - 1)):
            if cell in seen or EDGE[cell] is None:
                break  # loop/unknown frontier: not a proof of safety or death
            seen.add(cell)
            exits = set()
            neighbours = nbs(cell)
            for direction in ORDER:
                if open_dir(cell, direction) and neighbours[direction] != previous:
                    exits.add(neighbours[direction])
            if not exits:
                result = True
                break
            if len(exits) != 1:
                break
            previous, cell = cell, next(iter(exits))
        queen_pocket_cache[key] = result
        return result

    recent = {}
    if STEPS - LAST_EAT > LOOP_HUNGRY:
        for c in HIST[-LOOP_WINDOW:-1]:
            recent[c] = recent.get(c, 0) + 1
    cand = []
    queen_pockets = set()
    for d in ORDER:
        if he is not None and he[d] != 0:
            continue
        n = nbs(head)[d]
        b = block.get(n)
        if b is not None and 1 < b:
            continue
        sc = 0.0
        if route_guard and not continuation(n, (head, n), int(recent_pearl(n, rnd)), 1):
            sc -= 500 if L >= 6 or profile == 'queen_of_spades' else 90
        if n * 4 + d in DOOM:
            sc -= DOOM_PEN
        sp = space_from(n, space_cap)
        if sp < need:
            sc -= 400.0 * (need - sp) / need + (300 if sp < min(L, 6) else 0)
        dl = danger.get(n)
        if kpull is not None and hunt is None:
            sc += 10 * (tdist(head, kpull[1]) - tdist(n, kpull[1]))
        if hunt is not None:
            sc += HUNT_W * (tdist(head, hunt[1]) - tdist(n, hunt[1]))
            if dl is not None and dl[1] >= L + 1:
                dl = None
        if dl is not None:
            if last:
                sc -= 180
            dd, ev = dl
            excess = max(0, L - ev + 1)
            if king:
                # our growers keep well clear of enemy heads: losing one costs the round-500 check
                sc -= (120 + 24 * excess) * (1.0 if dd == 1 else 0.8)
            else:
                sc -= EVADE_SCALE * (DANGER_BASE + DANGER_PER_LEN * excess) * (1.0 if dd == 1 else DANGER_FAR) * (TRADE_RISK_FACTOR if not last and ev >= L + 1 else 1.0)
        if n in head_zone and dl is None:
            sc -= 50 if king else HEAD_ZONE_PEN
        if route_map == 'queen_of_spades' and n in friendly_head_zone and dl is None:
            sc -= 35
        if n not in view:
            if not (L <= 4 and STEPS - LAST_EAT > LOOP_HUNGRY):
                sc -= UNSEEN_EXIT_PEN + CROWD_PEN * n_friend_heads
            if LASTOCC[head] >= rnd - QUEUE_ROUNDS or LASTOCC[n] >= rnd - QUEUE_ROUNDS:
                sc -= QUEUE_PEN
        else:
            # a pocket whose only way on is a portal commits us to a blind exit next turn
            en = EDGE[n]
            seen_way = blind_way = False
            for d2 in ORDER:
                if d2 == OPP[d] or (en is not None and en[d2] != 0):
                    continue
                m = nbs(n)[d2]
                bm = block.get(m)
                if bm is not None and bm > 2:
                    continue
                if m in view:
                    seen_way = True
                    break
                blind_way = True
            if blind_way and not seen_way:
                sc -= POCKET_PEN
        if avoid_loops and n in view and n in HIST[-8:]:
            sc -= 20
        if n in recent:
            # circling the same few cells (a target we keep being pushed away from) wastes the opening
            sc -= LOOP_PEN * recent[n]
        src = PIN.get(n)
        if src is not None:
            for c in src:
                if c not in view:
                    if IS_Q:
                        sc -= Q_BLIND_ENTRY_PEN
                    else:
                        sc -= 3 if L <= 4 and STEPS - LAST_EAT > LOOP_HUNGRY else BLIND_ENTRY_PEN
                    break
        if recent_pearl(n, rnd):
            sc += 40
        if KNOWN is not None and route_map == 'trauma' and FARMD is not None and (L <= 6 or prime) and 0 < FARMD[head] < 65535:
            sc += 24 * (FARMD[head] - FARMD[n])
        sc += dscore[d]
        if explore_dir == d:
            sc += 15
        if picked == 0 and d == FACE_IDX and not enemy_heads and STRAIGHT_BONUS:
            sc += STRAIGHT_BONUS
        e = EDGE[n]
        opens = 0
        for d2 in ORDER:
            if e is not None and e[d2] != 0:
                continue
            m = nbs(n)[d2]
            bb = block.get(m)
            if bb is None or bb <= 2:
                opens += 1
        sc += 3 * opens
        if IS_Q:
            if queen_dead_end(n, head):
                queen_pockets.add(d)
                sc -= 4000
            # The danger search follows passable edges. Nearby heads behind
            # kelp must not scare the queen into a corridor she cannot exit.
            if dl is not None:
                sc -= Q_DANGER + 180 * max(0, SPRINT_REACH + 1 - dl[0])
            if n in friendly_head_zone:
                sc -= 100
            if n not in view:
                sc -= Q_BLIND
            seen_o = {n}
            fo_ = [n]
            for _ in range(3):
                if len(seen_o) >= Q_OPEN:
                    break
                nfo_ = []
                for x_ in fo_:
                    ex_ = EDGE[x_]
                    for dd_ in ORDER:
                        if ex_ is not None and ex_[dd_] != 0:
                            continue
                        y_ = nbs(x_)[dd_]
                        if y_ in seen_o or y_ in occ or y_ in far_body:
                            continue
                        seen_o.add(y_)
                        nfo_.append(y_)
                        if len(seen_o) >= Q_OPEN:
                            break
                    if len(seen_o) >= Q_OPEN:
                        break
                fo_ = nfo_
            if len(seen_o) < Q_OPEN:
                sc -= 40 * (Q_OPEN - len(seen_o))
        cand.append((sc, d, sp))

    if KNOWN is None and he is not None and (not cand or
            (not IS_Q and max(c[0] for c in cand) < -120)) and (not IS_Q or cand or not ct.can_split(2)):
        portals = [d for d in ORDER if he[d] == 2
                   and rnd - PORTAL_VISITS.get((head, d), -1000) > 12]
        if portals:
            d = portals[my_id % len(portals)]
            PORTAL_PENDING = (head, d, rnd, L)
            PORTAL_VISITS[head, d] = rnd
            STEPS += 1
            TELE = STEPS
            ct.make_move(DIRS[d])
            return

    if not cand:
        # The head keeps the queen ID. Preserve her length unless a permanent
        # neck pocket makes retaining a long tail the only remaining insurance.
        if IS_Q and L >= 4 and ct.can_split(2):
            # Cutting a tail segment can release a neighbouring tail cell,
            # but cutting cannot release rank 1 (the queen's retained neck).
            neighbouring_ranks = [own_rank[nbs(head)[d]] for d in ORDER
                                   if open_dir(head, d)
                                   and own_rank.get(nbs(head)[d], -1) >= 2]
            child = 2
            if neighbouring_ranks:
                child = min(max(2, L - r) for r in neighbouring_ranks)
            elif (L >= 6 and he is not None
                  and all(he[d] == 1 or (he[d] == 0 and
                          own_rank.get(nbs(head)[d]) == 1) for d in ORDER)):
                # A head boxed against its neck cannot be rescued by repeated
                # tiny cuts. Preserve the long tail for a dead-queen tiebreak
                # only when that reversed child's first exit is visibly clear.
                tail = next((c for c, r in own_rank.items() if r == L - 1), -1)
                if tail >= 0:
                    for d in ORDER:
                        n = nbs(tail)[d]
                        if (open_dir(tail, d) and n in view and n not in occ
                                and n not in far_body and n not in head_zone
                                and n not in danger):
                            child = L - 2
                            break
            if not ct.can_split(child):
                child = 2
            ct.do_split(child)
            POST_LENGTH, ACTION_KIND = L - child, 'split'
            return
        for d in ORDER:
            if he is None or he[d] == 0:
                o = occ.get(nbs(head)[d])
                if o is not None and o[3] and o[0] != my_team:
                    go(ct, head, [d])
                    return
        # stuck: splitting at least saves the tail half
        if rnd >= LATE_KEEP_FROM and L >= LATE_KEEP_LEN and ct.can_split(L - 2):
            # A long dragon coiled into itself late: hand everything but two segments to the tail
            # instead of halving (ladder/local replays: 35 -> 18 -> 9 cascades after round 450).
            # The tail may be out of sight; any open, unoccupied exit will do.
            tail = next((c for c, r in (rank | far_body).items() if r == L - 1), -1)
            if tail >= 0:
                for d in ORDER:
                    n = nbs(tail)[d]
                    if open_dir(tail, d) and n not in occ and n not in far_body and n * 4 + d not in DOOM:
                        child = 2 if IS_Q else L - 2
                        ct.do_split(child)
                        POST_LENGTH, ACTION_KIND = L - child, 'split'
                        return
        if KNOWN is not None:
            if ct.can_split(L // 2):
                # Preserve most of a stranded grower in the child at the tail.
                tail = next((c for c, r in (rank | far_body).items() if r == L - 1), -1)
                child = L // 2
                if tail >= 0 and L >= 5:
                    for d in ORDER:
                        n = nbs(tail)[d]
                        if (open_dir(tail, d) and n in view and n not in occ
                                and n not in far_body and n not in head_zone
                                and n * 4 + d not in DOOM):
                            child = L - 2
                            break
                child = 2 if IS_Q else child
                ct.do_split(child)
                POST_LENGTH, ACTION_KIND = L - child, 'split'
                return
        if ct.can_split(L // 2) and ct.unit_count < min(ct.unit_limit, UNITS_CAP):
            ct.do_split(2 if IS_Q else L // 2)
            POST_LENGTH, ACTION_KIND = L - (2 if IS_Q else L // 2), 'split'
            return
        # nothing survives: die alone rather than take a team mate with us
        worst = None
        for d in ORDER:
            o = occ.get(nbs(head)[d])
            if he is not None and he[d] != 0:
                pr = 2          # kelp: only we die
            elif o is None:
                pr = 4
            elif o[3] and o[0] != my_team:
                pr = 5          # enemy head: at least it is a trade
            elif o[3]:
                pr = 0          # friendly head: costs us two dragons
            else:
                pr = 3          # any body: only we die
            if worst is None or pr > worst[0]:
                worst = (pr, d)
        go(ct, head, [worst[1]])
        return

    # Keep Devil's immediately safe move when forecast food would lure us
    # into an enemy head's reachable cell.
    safe_choices = [entry for entry in cand if nbs(head)[entry[1]] not in danger]
    if not IS_Q and route_map == 'devil' and enemy_heads and safe_choices and hunt is None:
        cand = safe_choices
    if IS_Q:
        # A known cul-de-sac is certain death; a reachable enemy cell is a
        # forecast. Do not retreat into a proven pocket to avoid that forecast.
        outside_pockets = [entry for entry in cand if entry[1] not in queen_pockets]
        if outside_pockets:
            cand = outside_pockets
        # An unseen portal exit has no visible enemy forecast, which does not
        # make it safe. Prefer a visible short continuation unless that first
        # step is already in one-step head collision range.
        visible_routes = []
        for entry in cand:
            cell = nbs(head)[entry[1]]
            if (entry[1] not in queen_pockets and cell in view
                    and danger.get(cell, (7, 0))[0] > 1
                    and continuation(cell, (head, cell), int(recent_pearl(cell, rnd)), 1, True)):
                visible_routes.append(entry)
        if visible_routes:
            cand = visible_routes
        safe_queen = [entry for entry in cand if nbs(head)[entry[1]] not in danger]
        if safe_queen:
            cand = safe_queen
    immediate_scores = {d: score for score, d, _ in cand}

    # Look far enough ahead to notice a queen coiling into her own body.
    # A narrow, bounded beam replans from fresh vision every turn.
    queen_plans = {}
    queen_depths = {}
    if len(cand) >= 2 or IS_Q:
        horizon = min((6 if FIRST else min(14, max(6, L + 2))) if IS_Q
                      else (3 if FIRST else 5), turns_left)
        width = 2 if IS_Q else (3 if FIRST else 4)
        branch_limit = (24 if FIRST else 48) if IS_Q else (24 if FIRST else 48)
        # Fair per-direction tranches bound total queen work, not just each
        # candidate separately. Reaching a budget frontier is inconclusive.
        examined_limit = max(32, (160 if FIRST else 256) // len(cand)) if IS_Q else 10 ** 6
        revised = []
        for original, direction, space in cand:
            start = nbs(head)[direction]
            growth = int(recent_pearl(start, rnd))
            beam = [(0.0, start, growth, (head, start))]
            reached = 1
            best_future = 0.0
            branches = 0
            examined = 0
            best_path = (head, start)
            uncertain_frontier = False
            for step in range(2, horizon + 1):
                if branches >= branch_limit or examined >= examined_limit:
                    uncertain_frontier = True
                    break
                nxt = []
                for value, cell, grown, path in beam:
                    if branches >= branch_limit or examined >= examined_limit:
                        uncertain_frontier = True
                        break
                    live_path = frozenset(path[-(L + grown):])
                    for d in ORDER:
                        examined += 1
                        if examined > examined_limit:
                            uncertain_frontier = True
                            break
                        if not open_dir(cell, d):
                            continue
                        n = nbs(cell)[d]
                        o = occ.get(n)
                        r = own_rank.get(n)
                        if o is not None and (o[1] != my_id or r is None):
                            continue
                        if r is not None and step <= L - r + grown:
                            continue
                        # Newly visited cells become reusable only after the
                        # simulated tail has passed them, accounting for growth.
                        if n in live_path:
                            continue
                        # Unknown space is a frontier, not a proven dead end.
                        if n not in view and EDGE[n] is None:
                            uncertain_frontier = True
                            continue
                        branches += 1
                        if branches > branch_limit:
                            uncertain_frontier = True
                            break
                        food = int(recent_pearl(n, rnd) and n not in path)
                        danger_cost = 0
                        if step <= 3 and n in danger:
                            danger_cost = 12 if king else 6
                        food_gain = 24 if route_map == 'devil' else 10
                        score = value + (food_gain * food - danger_cost - (2 if n not in view else 0)) * (0.8 ** (step - 1))
                        nxt.append((score, n, grown + food, path + (n,)))
                if not nxt:
                    break
                nxt.sort(key=lambda state: -state[0])
                beam = nxt[:width]
                reached = step
                best_future = beam[0][0]
                best_path = beam[0][3]
            queen_plans[direction] = best_path
            if IS_Q:
                queen_depths[direction] = reached
            # Beam pruning is not a proof of impossibility. Budget/unknown
            # frontiers never receive a dead-end penalty.
            penalty = 0 if uncertain_frontier else ((220 if IS_Q else
                       (30 if KNOWN is not None else 100)) *
                       max(0, min(horizon, 6 if IS_Q else 4) - reached))
            # A speculative future pearl must not outweigh immediate escape
            # priorities when enemy heads are already visible.
            if enemy_heads:
                best_future = min(0.0, best_future)
            revised.append((original + best_future - penalty, direction, space))
        cand = revised

    if IS_Q:
        # Depth is a soft preference within the safe first-step set. Pruning,
        # changing enemy bodies and future pearl spawns limit this forecast.
        cand = [(score + 20 * min(6, queen_depths.get(d, 1)), d, space)
                for score, d, space in cand]
    cand.sort(key=lambda z: -z[0])
    best_sc, best_d, best_sp = cand[0]
    immediate_sc = immediate_scores[best_d] if route_map == 'devil' else best_sc
    chosen = next((cell for _, cell in found if first[cell] == best_d), -1)
    if chosen != GOAL or rnd - GOAL_SINCE >= 8:
        GOAL, GOAL_SINCE = chosen, rnd
    for _, target in found:
        if first[target] == best_d:
            AIM, AIM_DIST = target, max(0, dist[target] - 1)
            break

    def escape_has_exit(cells, remaining):
        if turns_left <= 1:
            return True
        final_body = sprint_body(cells, remaining)
        for direction in ORDER:
            if not open_dir(cells[-1], direction):
                continue
            dest = nbs(cells[-1])[direction]
            occupant = occ.get(dest)
            if dest not in view or dest in final_body:
                continue
            if occupant is not None and (occupant[1] != my_id or dest not in own_rank):
                continue
            return True
        return False

    # A sprint can escape the next enemy action when every single step is
    # threatened. Pay a segment instead of sacrificing a valuable grower.
    if (enemy_heads and L >= 2 and immediate_sc < -45
            and (not IS_Q or all(nbs(head)[d] in danger for _, d, _ in cand))
            and (route_map == 'queen_of_spades' or COORD_MAP == 'trauma'
                 or all(nbs(head)[d] in danger for _, d, _ in cand))):
        fr = [(head, L, (), (head,))]
        escape = None
        checks = 0
        escape_depth = (3 if FIRST else 5) if IS_Q else (2 if FIRST else 3)
        for depth in range(1, escape_depth + 1):
            if IS_Q:
                checks = 0  # two space checks per layer, at most eight total
            nf = []
            for c, rem, path, cells in fr:
                if depth > free_me and rem <= 2:
                    continue
                live_body = sprint_body(cells, rem)
                for d in ORDER:
                    if not open_dir(c, d):
                        continue
                    n = nbs(c)[d]
                    occupant = occ.get(n)
                    if (n not in view or n in live_body or n * 4 + d in DOOM
                            or (occupant is not None and (occupant[1] != my_id or n not in own_rank))):
                        continue
                    nr = rem + int(recent_pearl(n, rnd) and n not in cells) - int(depth > free_me)
                    route = path + (d,)
                    nf.append((n, nr, route, cells + (n,)))
                    if depth < 2 or n in danger or n in head_zone or checks >= (2 if IS_Q else 8):
                        continue
                    # A broad five-step forecast must not repeatedly consume
                    # a short queen to length two. Keep three unless the chosen
                    # single step is in immediate one-step collision range.
                    if (IS_Q and nr < 3 and nr < L
                            and danger.get(nbs(head)[best_d], (6, 0))[0] > 1):
                        continue
                    if not escape_has_exit(cells + (n,), nr):
                        continue
                    if IS_Q and queen_dead_end(n, c):
                        continue
                    checks += 1
                    sp = space_from(n, min(30, space_cap))
                    if sp < min(nr + 2, 12):
                        continue
                    score = (nr - L) * 8 + min(sp, 16)
                    if escape is None or score > escape[0]:
                        escape = score, route
            if IS_Q:
                nf.sort(key=lambda state: (state[0] in danger, -state[1]))
                nf = nf[:12]
            fr = nf
            if escape is not None:
                go(ct, head, escape[1])
                return

    # desperation trade when the best option is a trap
    if (best_sp < min(L, 4, turns_left) and not IS_Q
            and not continuation(nbs(head)[best_d], (head, nbs(head)[best_d]),
                                 int(recent_pearl(nbs(head)[best_d], rnd)), 1)):
        for d in ORDER:
            if he is None or he[d] == 0:
                o = occ.get(nbs(head)[d])
                if o is not None and o[3] and o[0] != my_team:
                    go(ct, head, [d])
                    return

    # ---------------- 5. split
    productive_split = route_map == 'default' and rnd < END_GROW and ct.unit_count < min(ct.unit_limit, UNITS_CAP)
    near_danger = head in danger and not SMALL
    if near_danger and (productive_split or route_map == 'devil') and danger[head][0] > 2:
        near_danger = False
    # A conservative area estimate is not a trap if the tail permits a route.
    best_next = nbs(head)[best_d]
    trapped = best_sp < need and not continuation(best_next, (head, best_next), int(recent_pearl(best_next, rnd)), 1)
    # Reverse the valuable tail out of danger, leaving only two head segments.
    # The child takes a turn immediately; inspect its exit before committing.
    if not IS_Q and L >= 5 and ct.can_split(L - 2) and (trapped or best_sc < -180):
        tail = next((c for c, r in (rank | far_body).items() if r == L - 1), -1)
        if tail >= 0:
            for d in ORDER:
                n = nbs(tail)[d]
                if (open_dir(tail, d) and n in view and n not in occ and n not in far_body
                        and n not in head_zone and n * 4 + d not in DOOM
                        and space_from(n, space_cap) >= min(L, 12)):
                    ct.do_split(L - 2)
                    POST_LENGTH, ACTION_KIND = 2, 'split'
                    return
    # A collapsed colony needs collectors as well as one protected grower.
    # Bud only two segments from an established survivor; never halve it.
    if (not IS_Q and L >= 8 and ct.unit_count <= 4 and rnd < 220
            and not trapped and head not in danger
            and ct.unit_count < min(ct.unit_limit, UNITS_CAP) and ct.can_split(2)):
        tail = next((c for c, r in own_rank.items() if r == L - 1), -1)
        if tail >= 0:
            for direction in ORDER:
                start = nbs(tail)[direction]
                if (not open_dir(tail, direction) or start not in view
                        or start in occ or start in far_body or start in head_zone
                        or start in danger or start * 4 + direction in DOOM):
                    continue
                room = {start}
                queue = [start]
                for cell in queue:
                    if len(room) >= 4:
                        break
                    for d in ORDER:
                        n = nbs(cell)[d]
                        if (open_dir(cell, d) and n in view and n not in room
                                and n not in occ and n not in far_body
                                and n not in head_zone and n not in danger):
                            room.add(n)
                            queue.append(n)
                            if len(room) >= 4:
                                break
                if len(room) >= 4:
                    ct.do_split(2)
                    POST_LENGTH, ACTION_KIND = L - 2, 'split'
                    return
    if TRAP_SPLIT:
        # Late in the game a long dragon does not halve itself on a trap estimate: ladder replays
        # showed 20+ such halvings per game after round 450 (22 -> 11 -> 5), each one a direct
        # loss on the round-500 longest-dragon check.
        late_long = rnd >= LATE_KEEP_FROM and L >= LATE_KEEP_LEN
        want_split = (L >= SPLIT_MIN_LEN and rnd < min(LATE_GAME, END_GROW) and not king) or (trapped and L >= 4 and not late_long)
    else:
        want_split = L >= SPLIT_MIN_LEN and rnd < LATE_GAME
    if IS_Q:
        # Winners bud collectors early, then preserve their queen's length.
        # Only bud two segments and require the existing safe-tail checks.
        want_split = (rnd < 200 and L >= 4 and not trapped and head not in danger
                      and ct.unit_count < min(48, ct.unit_limit, UNITS_CAP))
    if want_split and not trapped and L >= KEEP_LONG and rnd >= KEEP_LONG_FROM:
        # Live Schooltime: we had dragons of 31-36 at round 200-250, then halved them (36 -> 18 -> 9)
        # and lost the round-500 check 16 vs 29. A long dragon is worth more whole.
        want_split = False
    if (want_split and not near_danger
            and ct.unit_count < min(ct.unit_limit, UNITS_CAP) and ct.can_split(L // 2)):
        ok = True
        tail = -1
        for c, r in (rank | far_body).items():
            if r == L - 1:
                tail = c
        if tail >= 0:
            ok = False
            e = EDGE[tail]
            for d in ORDER:
                if e is not None and e[d] != 0:
                    continue
                # the newborn moves after us this round: it needs a first step onto a cell we can see
                # is free (a blind portal exit may hold a team mate's head: both die)
                n = nbs(tail)[d]
                # an unseen cell is fine when it is the plain neighbour: our tail usually sits on the edge
                # of the 7x7 view, so requiring sight blocked most splits (Devil: 3 of 4)
                seen_ok = n in view or (n == geo(tail)[d] and LASTOCC[n] < rnd - 2)
                child_exit_safe = n not in danger if productive_split else n not in head_zone or FAST_SPLIT
                if n not in occ and seen_ok and n not in far_body and child_exit_safe and n * 4 + d not in DOOM:
                    ok = True
                    break
            if ok and not trapped:
                # the child starts at our tail: do not give birth in a dead end
                seen_c = {tail}
                q = [tail]
                qi = 0
                while qi < len(q) and len(seen_c) < CHILD_ROOM + 1:
                    c = q[qi]
                    qi += 1
                    e = EDGE[c]
                    for d in ORDER:
                        if e is not None and e[d] != 0:
                            continue
                        n = nbs(c)[d]
                        if n in seen_c or n in occ or n in far_body:
                            continue
                        seen_c.add(n)
                        q.append(n)
                required_room = 2 if productive_split else (4 if route_map == 'devil' else (FAST_SPLIT_ROOM if FAST_SPLIT else CHILD_ROOM))
                ok = len(seen_c) > required_room
        if ok:
            # near the dragon cap, a long dragon only buds off a short child and keeps its length (round-500 check)
            cap = min(ct.unit_limit, UNITS_CAP)
            small = not trapped and L >= SMALL_CHILD_FROM and ct.unit_count >= cap - 4
            q_child = 2 if IS_Q or not trapped else L // 2
            ct.do_split(q_child)
            POST_LENGTH = L - q_child
            ACTION_KIND = 'split'
            return

    # Use the scored path for harvesting or a short transit toward its food target.
    # Do not expand beyond current sight, spend length, or invent a new route.
    plan = queen_plans.get(best_d, ())
    target = accepted_targets.get(best_d)
    if COORD_MAP == 'trauma' and L >= 8 and not enemy_heads and target in view and recent_pearl(target, rnd) and 2 <= dist.get(target, 0) <= 16:
        # Reuse the food search's scored target and distances. The transit
        # below stays within sight and keeps all existing collision checks.
        reverse_path = [target]
        for _ in range(dist[target]):
            cell = reverse_path[-1]
            previous = next((nbs(cell)[d] for d in ORDER
                             if open_dir(cell, d) and nbs(cell)[d] in dist
                             and dist[nbs(cell)[d]] == dist[cell] - 1
                             and (nbs(cell)[d] == head or first.get(nbs(cell)[d]) == best_d)), None)
            if previous is None:
                break
            reverse_path.append(previous)
            if previous == head:
                recovered = tuple(reversed(reverse_path))
                if recovered[1] == nbs(head)[best_d]:
                    plan = recovered
                break
    transit = L >= 8 and target in plan[2:] and recent_pearl(target, rnd)
    if free_me >= 2 and not enemy_heads and len(plan) >= 3:
        cells = (head,)
        route = []
        remaining = L
        last_food_route = None
        for dest in plan[1:min(len(plan), min(free_me, 5) + 1)]:
            cell = cells[-1]
            direction = next((d for d in ORDER if open_dir(cell, d)
                              and nbs(cell)[d] == dest), None)
            occupant = occ.get(dest)
            if (direction is None or dest not in view or dest in head_zone
                    or dest in danger or dest in sprint_body(cells, remaining)
                    or (occupant is not None and occupant[1] != my_id)):
                break
            remaining += int(recent_pearl(dest, rnd))
            route.append(direction)
            cells += (dest,)
            if (len(route) >= 2 and (recent_pearl(dest, rnd) or (transit and len(route) == 2))
                    and escape_has_exit(cells, remaining)
                    and not queen_dead_end(dest, cell)):
                last_food_route = route[:]
        if last_food_route is not None:
            go(ct, head, last_food_route)
            return
    go(ct, head, [best_d])


def send_coordination(ct):
    # A newly explored portal's destination is not known until next turn.
    if (PORTAL_PENDING is not None or CELL_BITS > 16
            or not hasattr(unswbc, 'UINT64_MAX')):
        return
    team = ct.head.team.value
    is_queen_ = QUEEN_MODE and ct.head.dragon_id in (0, 1)
    if not is_queen_ and COORD_MAP == 'trauma' and LAST_ROUND < SONAR_FROM and AIM >= 0 and ACTION_KIND == 'move':
        packet = make_packet(1, ct.head.dragon_id, min(127, AIM_DIST),
                             AIM, LAST_ROUND, team)
        for d in DIRS:
            ct.send_sonar(d, packet)
        return
    relay_q = None
    if (QUEEN_MODE and QUEEN_FEEDABLE and not is_queen_
            and MY_Q is not None and LAST_ROUND >= Q_SONAR_FROM):
        qa_ = ANNOUNCED.get(MY_Q)
        if qa_ is not None and 0 <= LAST_ROUND - qa_[2] <= 2:
            relay_q = qa_
    if ACTION_KIND == 'donate' or (relay_q is None and LAST_ROUND < (Q_SONAR_FROM if is_queen_ else SONAR_FROM)):
        return
    if relay_q is not None:
        packet = make_packet(0, MY_Q, min(255, max(0, relay_q[0])), relay_q[1], relay_q[2], team)
        for d in DIRS:
            ct.send_sonar(d, packet)
        return
    cell = HIST[-1] if HIST else (ct.head.position.y * W + ct.head.position.x)
    sid, sln = ct.head.dragon_id, POST_LENGTH
    packet_round = LAST_ROUND
    if RELAY and LAST_ROUND >= RELAY_FROM:
        # Lines stop at the first dragon, so relay the longest fresh ally.
        for did, (ln, c_, seen) in ANNOUNCED.items():
            if ln > sln and 0 <= LAST_ROUND - seen <= 2:
                sid, sln, cell, packet_round = did, ln, c_, seen
    packet = make_packet(0, sid, min(255, max(0, sln)), cell, packet_round, team)
    for d in DIRS:
        ct.send_sonar(d, packet)


def fallback(ct):
    try:
        here = ct.get_position()
        ht = ct.get_tile(here)
        for d in DIRS:
            if ht.get_edge(d).is_passable() and not ht.get_edge(d).is_portal():
                nt = ct.get_tile(here.add_dir(d))
                if nt is not None and nt.get_dragon() is None:
                    ct.make_move(d)
                    return
        # every neighbour holds a dragon: prefer kelp, which kills only us
        for d in DIRS:
            if not ht.get_edge(d).is_passable():
                ct.make_move(d)
                return
    except Exception:
        pass
    ct.make_move(Direction.NORTH)


def main() -> None:
    global FIRST
    import sys
    from io import StringIO
    from contextlib import redirect_stdout
    output = sys.stdout
    ct, game = unswbc.init()
    setup(game.width, game.height)
    while True:
        try:
            if not unswbc.update(ct, game):
                break
        except EOFError:
            break
        # (Yueci Li 18) the judge charges for every write: emit the whole turn once
        buffer = StringIO()
        with redirect_stdout(buffer):
            try:
                execute_turn(ct, game)
            except Exception:
                fallback(ct)
            FIRST = False
            try:
                send_coordination(ct)
            except Exception:
                pass
            unswbc.end_turn()
        output.write(buffer.getvalue())
        output.flush()


if __name__ == "__main__":
    main()
