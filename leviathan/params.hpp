#pragma once
#define LEVIATHAN_PARAMS_VERSION 18   // must match main.cpp; always use params.hpp and main.cpp from the same release

// Trauma plan (used by several blocks below)
#ifndef P_TRAUMA_GROW
#define P_TRAUMA_GROW 400  // v13: Trauma plan 4 growRound / feedStart / feedPullStart
#endif
#ifndef P_TRAUMA_FEED
#define P_TRAUMA_FEED 400
#endif
#ifndef P_TRAUMA_PULL
#define P_TRAUMA_PULL 380
#endif
#ifndef P_TRAUMA_PLAN
#define P_TRAUMA_PLAN 3  // v10: 3 (v9: 0). Trauma: 0 = one king (v5/v6), 1 = swarm + pocket farming + meet at home, 2 = king + pocket farmers + couriers (v3 roles), 3 = king at home + one farmer on the private pocket behind the top portal, its tail reversed into the next farmer, the rest carried to the king (v7)
#endif
// Leviathan v11 - every tunable constant.
//
// Each value can be overridden at compile time without editing this file, e.g.
//     g++ ... -DP_SPLIT_MIN=6 -DP_FEED_START=420
// (tools/build.sh passes extra flags through). The per-map strategy table P_MAP_TABLE at the end
// applies to the 13 known maps only; every other map (the tournament's) plays with the defaults.
// See TUNING.md in the dev kit for how to measure a change.
// Every #define below must keep a numeric value (the build names the parameter if one is missing).

// ==============================================================================================
// MAP KNOWLEDGE AND PEARL MODEL
// ==============================================================================================
#ifndef P_MAPDB
#define P_MAPDB 1  // 1 = recognise known maps from maps.hpp (walls, portals, spawn-gap ranges, symmetry)
#endif
#ifndef P_DB_TENTATIVE
#define P_DB_TENTATIVE 1  // 1 = when several known maps fit the view, use one until the view rules it out
#endif
#ifndef P_UNSEEN_SURV
#define P_UNSEEN_SURV 0.1  // chance that a never-seen pearl is still uneaten (late game)
#endif
#ifndef P_UNSEEN_EARLY
#define P_UNSEEN_EARLY 60  // default for the per-map unseenEarly column (rounds of extra trust in unseen pearls)
#endif
#ifndef P_RESPAWN_SURV
#define P_RESPAWN_SURV 0.6  // weight of a possible respawn after a known spawn event
#endif
#ifndef P_DECAY
#define P_DECAY 40.0  // rounds: how fast a remembered pearl is assumed eaten by others
#endif
#ifndef P_FARM_MAXGAP
#define P_FARM_MAXGAP 5  // tiles whose max spawn gap is <= this are "farms" (respawn almost every round)
#endif
#ifndef P_FARM_ARRIVE
#define P_FARM_ARRIVE 0.7  // expected pearl on a farm tile when we get there (>= 2 moves away)
#endif
#ifndef P_HAZARD_UNSEEN
#define P_HAZARD_UNSEEN 0  // 1 = also count spawns during travel on tiles never observed
#endif
#ifndef P_GAP_PRIOR
#define P_GAP_PRIOR 200.0  // (unknown maps) assumed respawn gap before any is observed
#endif
#ifndef P_GAP_CD
#define P_GAP_CD 1.2  // (unknown maps) estimate a tile's mean respawn gap as this x the largest countdown seen on it (0 = use P_GAP_PRIOR)
#endif
#ifndef P_UNKNOWN_PRIOR
#define P_UNKNOWN_PRIOR 0.03  // (unknown maps) pearl probability of a never-seen tile
#endif
#ifndef P_CALIB
#define P_CALIB 0  // 1 = calibrate the pearl model online: compare its prediction for tiles coming into view with what is there
#endif
#ifndef P_CALIB_K
#define P_CALIB_K 8.0  // prior weight (in predicted pearls) of the built-in model in the calibration
#endif
#ifndef P_CALIB_FORGET
#define P_CALIB_FORGET 0.99  // per-turn forgetting of old calibration evidence
#endif
#ifndef P_CALIB_MAX
#define P_CALIB_MAX 3.0  // cap on a calibration factor
#endif
#ifndef P_FARM_VALUE
#define P_FARM_VALUE 0.8  // (unknown maps) value of a learned farm tile in the goal field
#endif
#ifndef P_FARM_GAMMA
#define P_FARM_GAMMA 0.88  // (unknown maps) distance discount for farm tiles
#endif
#ifndef P_PORTAL_TARGET
#define P_PORTAL_TARGET 0.3  // (unknown maps) value of an unexplored portal as a destination
#endif
#ifndef P_PORTAL_EXPLORE
#define P_PORTAL_EXPLORE 6.0  // (unknown maps) score of stepping into an unexplored portal
#endif
#ifndef P_BODY_MSG
#define P_BODY_MSG 1  // 1 = after splitting, send the child its body layout over sonar
#endif
#ifndef P_GOSSIP
#define P_GOSSIP 1  // 1 = share symmetry, portals, farms, sightings and the king over sonar
#endif

// ==============================================================================================
// ROUTE PLANNER (beam search over future paths)
// ==============================================================================================
#ifndef P_GOSSIP_CD
#define P_GOSSIP_CD 1  // 1 = tell allies the countdowns of rich tiles I have seen (sonar)
#endif
#ifndef P_RICH_GAP
#define P_RICH_GAP 100  // (P_GOSSIP_CD) a tile counts as rich if no countdown above this was seen on it
#endif
#ifndef P_RICH_AGE
#define P_RICH_AGE 30  // (P_GOSSIP_CD) only tiles seen within this many rounds
#endif
#ifndef P_RICH_MSGS
#define P_RICH_MSGS 12  // (P_GOSSIP_CD) at most this many rich tiles in the gossip pool
#endif
#ifndef P_SONAR_AIM
#define P_SONAR_AIM 1  // 1 = send the most important gossip along the rays that hit an ally
#endif
#ifndef P_W_PATH
#define P_W_PATH 1.0  // weight of the best planned path per first move
#endif
#ifndef P_W_DIFF
#define P_W_DIFF 0.3  // weight of the older "diffuse" goal field (sum over all tiles)
#endif
#ifndef P_BEAM_W
#define P_BEAM_W 120  // beam width
#endif
#ifndef P_BEAM_D
#define P_BEAM_D 24  // beam depth (moves)
#endif
#ifndef P_BEAM_Q
#define P_BEAM_Q 16  // minimum paths kept per first move
#endif
#ifndef P_BEAM_NODES
#define P_BEAM_NODES 30000  // hard cap on beam nodes per turn (keeps the CPU cost bounded)
#endif
#ifndef P_PATH_GAMMA
#define P_PATH_GAMMA 0.95  // per-move discount along a path
#endif
#ifndef P_TAIL_W
#define P_TAIL_W 0.8  // weight of the tail estimate at the end of a path
#endif
#ifndef P_PHI_KAPPA
#define P_PHI_KAPPA 0.92  // per-step decay of the tail estimate (distance to the best spot beyond the horizon)
#endif
#ifndef P_PHI_CLUSTER
#define P_PHI_CLUSTER 0.5  // how much neighbouring pearls add to a spot's tail value (rich regions attract)
#endif
#ifndef P_GAMMA
#define P_GAMMA 0.88  // discount of the diffuse goal field
#endif
#ifndef P_BFS_RADIUS
#define P_BFS_RADIUS 40  // radius (moves) of the goal field / planner area
#endif
#ifndef P_COMP_ENEMY
#define P_COMP_ENEMY 0.3  // value factor for pearls an enemy head reaches first
#endif
#ifndef P_COMP_ENEMY_MAPS
#define P_COMP_ENEMY_MAPS "Autarky", "Default", "Devil", "Prisoners Dilemma", "Trophy"  // v13: known maps where pearls an enemy head reaches first are valued at P_COMP_ENEMY_MAPV (the ladder's top team ate most of the pearls dropped in fights); on Queen Of Spades it lost 7 of 24
#endif
#ifndef P_COMP_ENEMY_MAPV
#define P_COMP_ENEMY_MAPV 0.6  // (P_COMP_ENEMY elsewhere: 0.3)
#endif
#ifndef P_COMP_ALLY
#define P_COMP_ALLY 0.25  // value factor for pearls an ally head reaches first
#endif
#ifndef P_COMP_RADIUS
#define P_COMP_RADIUS 12  // BFS radius used for those competition checks
#endif
#ifndef P_W_EAT
#define P_W_EAT 1.0  // bonus per pearl eaten by the move itself
#endif
#ifndef P_W_GOAL
#define P_W_GOAL 1.0  // weight of the combined goal value in the move score
#endif

// ==============================================================================================
// SAFETY (survival search, threats, space)
// ==============================================================================================
#ifndef P_SURV_DEPTH
#define P_SURV_DEPTH 24  // survival search depth (moves) for normal dragons
#endif
#ifndef P_TURN_DFS_BUDGET
#define P_TURN_DFS_BUDGET 80000  // total survival-search nodes per turn (CPU safety cap: ~600 points per node; x0.7 above 64x64, x0.45 above 128x128)
#endif
#ifndef P_DFS_LIMIT
#define P_DFS_LIMIT 6000  // node budget of that search
#endif
#ifndef P_W_PESS
#define P_W_PESS 30.0  // penalty when survival fails if adjacent enemy heads move into our exits
#endif
#ifndef P_W_PESS_ALLY
#define P_W_PESS_ALLY 8.0  // same, for adjacent ally heads
#endif
#ifndef P_W_ALLY_TRAP
#define P_W_ALLY_TRAP 60.0  // penalty for a move that leaves a nearby ally with no way out
#endif
#ifndef P_W_SPACE
#define P_W_SPACE 2.0  // penalty per missing tile of reachable space (< length + 3)
#endif
#ifndef P_SPACE_LEN_SCALE
#define P_SPACE_LEN_SCALE 10.0  // space penalty grows by length/this for long dragons
#endif
#ifndef P_STRIKE_FAV
#define P_STRIKE_FAV 0.05  // assumed chance an enemy that gains length by a head-on strikes us
#endif
#ifndef P_STRIKE_EQ
#define P_STRIKE_EQ 0.02  // same, when lengths are equal
#endif
#ifndef P_STRIKE_UNFAV
#define P_STRIKE_UNFAV 0.05  // same, when the enemy would lose length
#endif
#ifndef P_STRIKE_SPRINT
#define P_STRIKE_SPRINT 0.5  // factor for strikes that need a sprint
#endif
#ifndef P_BIGDIFF
#define P_BIGDIFF 6  // length gap from which P_STRIKE_BIGDIFF applies
#endif
#ifndef P_STRIKE_BIGDIFF
#define P_STRIKE_BIGDIFF 0.05  // assumed strike chance for a much longer target
#endif
#ifndef P_EQ_LOSS
#define P_EQ_LOSS 2.0  // extra cost of losing a unit in a trade
#endif
#ifndef P_UNSEEN_REACH
#define P_UNSEEN_REACH 4  // assumed sprint reach of an enemy whose length we cannot see
#endif
#ifndef P_UNIT_VALUE
#define P_UNIT_VALUE 3.0  // value of a unit on top of its length
#endif
#ifndef P_SPRINT_COST
#define P_SPRINT_COST 3.0  // score cost per extra sprint step (each costs one segment)
#endif
#ifndef P_START_EXTRAP
#define P_START_EXTRAP 0  // 1 = (unknown maps) a starting dragon whose body leaves the view assumes it continues straight
#endif
#ifndef P_PORTAL_PATHFIX
#define P_PORTAL_PATHFIX 1  // keep track of my body through a portal whose far end I learned by walking through it
#endif
#ifndef P_PORTAL_BLIND
#define P_PORTAL_BLIND 0.5  // penalty for stepping out of a portal exit we cannot see (v3: 2.0, which made units turn round at portals)
#endif
#ifndef P_PORTAL_CAUTIOUS_MAPS
#define P_PORTAL_CAUTIOUS_MAPS "Queen Of Spades"  // known maps that keep v3's portal caution (comma-separated names)
#endif
#ifndef P_PORTAL_BLIND_CAUTIOUS
#define P_PORTAL_BLIND_CAUTIOUS 2.0  // P_PORTAL_BLIND on those maps (v4's lower value lost 0/8 vs v3 on Queen)
#endif
#ifndef P_PORTAL_EXIT_STAY_CAUTIOUS
#define P_PORTAL_EXIT_STAY_CAUTIOUS 1.0  // P_PORTAL_EXIT_STAY on those maps
#endif
#ifndef P_PORTAL_EXIT_STAY
#define P_PORTAL_EXIT_STAY 0.3  // penalty for parking our head on a portal landing tile (v3: 1)
#endif
#ifndef P_ENDGAME_ROUND
#define P_ENDGAME_ROUND 470  // from this round threats are weighed P_ENDGAME_RISK times more
#endif
#ifndef P_ENDGAME_RISK
#define P_ENDGAME_RISK 3.0  // threat multiplier from P_ENDGAME_ROUND on
#endif
#ifndef P_RESCUE_X
#define P_RESCUE_X 1  // 1 = when doomed, split off the largest child even if its survival is uncertain
#endif
#ifndef P_RESCUE_BLIND_TURNS
#define P_RESCUE_BLIND_TURNS 1  // blind rescue split allowed during the first N turns (spawn in a dead end)
#endif
#ifndef P_RESCUE_PARENT
#define P_RESCUE_PARENT 1  // 1 = when doomed, prefer cutting a small tail so the head keeps the length
#endif
#ifndef P_RESCUE_PARENT_MINLEN
#define P_RESCUE_PARENT_MINLEN 8  // ...for dragons at least this long
#endif

// ==============================================================================================
// PRODUCTION AND FIGHTING
// ==============================================================================================
#ifndef P_SPLIT_MIN
#define P_SPLIT_MIN 4  // default split length (per-map column splitMin)
#endif
#ifndef P_MAX_UNITS
#define P_MAX_UNITS 9999  // default unit cap for splitting (per-map column maxUnits); the map's UNIT_LIMIT always applies
#endif
#ifndef P_GROW_ROUND
#define P_GROW_ROUND 400  // default round after which nobody splits (per-map column growRound)
#endif
#ifndef P_SPLIT_END
#define P_SPLIT_END 420  // absolute last round for normal splits
#endif
#ifndef P_CHILD_MAX
#define P_CHILD_MAX 0  // cap on child size (0 = half of the length)
#endif
#ifndef P_SPLIT_ENEMY_DIST
#define P_SPLIT_ENEMY_DIST 1  // no split with an enemy head this close (v3: 3)
#endif
#ifndef P_SPLIT_CROWD
#define P_SPLIT_CROWD 99  // no split with more than this many other segments around (v3: 2)
#endif
#ifndef P_SPLIT_SPACE_MULT
#define P_SPLIT_SPACE_MULT 2  // split only if reachable area >= this x length (v3: 4)
#endif
#ifndef P_SPLIT_DENSITY
#define P_SPLIT_DENSITY 99  // no split with more than this many visible allies
#endif
#ifndef P_AREA_PER_UNIT
#define P_AREA_PER_UNIT 0  // (off) reachable area required per unit
#endif
#ifndef P_ATTACK_MIN_GAIN
#define P_ATTACK_MIN_GAIN 0.0  // take a head-on trade when enemy length - my length >= this
#endif
#ifndef P_ATTACK_MAX_STEPS
#define P_ATTACK_MAX_STEPS 10  // longest sprint considered for an attack
#endif
#ifndef P_DEADEND
#define P_DEADEND 1  // 1 = may enter dead ends it can only leave by splitting (farm pockets)
#endif
#ifndef P_DEADEND_COST
#define P_DEADEND_COST 5.0  // default for the per-map deadEndCost column
#endif
#ifndef P_DEADEND_MIN_ROUND
#define P_DEADEND_MIN_ROUND 0  // no deliberate dead ends before this round
#endif
#ifndef P_ZONE_PULL
#define P_ZONE_PULL 0.0  // (experimental, off: hurt Trophy badly) early rush toward rich production zones of a known map
#endif
#ifndef P_ZONE_FRAC
#define P_ZONE_FRAC 1.0  // share of the units (chosen by id) that answer the zone pull
#endif
#ifndef P_ZONE_ONLINE
#define P_ZONE_ONLINE 1  // 1 = (unknown maps) find production zones from observed countdowns, for the zone pull
#endif
#ifndef P_ZONE_RICH_GAP
#define P_ZONE_RICH_GAP 60.0  // (P_ZONE_ONLINE) tiles with an estimated mean gap up to this count
#endif
#ifndef P_ZONE_ONLINE_UNTIL
#define P_ZONE_ONLINE_UNTIL 300  // (P_ZONE_ONLINE) the zone pull on unknown maps lasts until this round
#endif
#ifndef P_ZONE_UNTIL
#define P_ZONE_UNTIL 120  // ...during the first rounds only
#endif
#ifndef P_ZONE_RADIUS
#define P_ZONE_RADIUS 3  // zone = spawn rate summed within this walking distance
#endif
#ifndef P_ZONE_MIN_RATIO
#define P_ZONE_MIN_RATIO 2.5  // a zone must beat the map's average area by this factor
#endif
#ifndef P_ZONE_MIN_RATE
#define P_ZONE_MIN_RATE 0.1  // ...and yield at least this many pearls per round (v11: 0.1, so that Default's portal boxes count; v10: 0.15)
#endif
#ifndef P_ZONE_D0
#define P_ZONE_D0 15.0  // zone value = rate / (P_ZONE_D0 + distance)
#endif
#ifndef P_ZONE_MINDIST
#define P_ZONE_MINDIST 14  // ...and only from farther than this (closer ones are the route planner's job)
#endif
#ifndef P_ZONE_MARGIN
#define P_ZONE_MARGIN 1.5  // pull only when the zone beats where I am by this factor
#endif
#ifndef P_LATE_KING
#define P_LATE_KING 0  // >0: on maps whose kingSplitUntil is 9999 nobody is king until feedPullStart minus this
#endif
#ifndef P_KING_CAP
#define P_KING_CAP 90  // maps without splitting (Stronghold/Trauma): the king stops chasing pearls at this length (longer kings coil to death)
#endif
#ifndef P_KING_CAP_UNTIL
#define P_KING_CAP_UNTIL 470  // ...until this round
#endif
#ifndef P_POCKET
#define P_POCKET 1  // 1 = farm dead-end pockets on known maps (walk in, eat, split out, stub dies)
#endif
#ifndef P_POCKET_MAXLEN
#define P_POCKET_MAXLEN 0  // default for the per-map pocketMaxLen column (0 = no pocket farming)
#endif
#ifndef P_POCKET_ONLINE
#define P_POCKET_ONLINE 1  // 1 = (unknown maps) find farm pockets from walls and fast-respawning tiles seen so far
#endif
#ifndef P_POCKET_ONLINE_MAXLEN
#define P_POCKET_ONLINE_MAXLEN 12  // (P_POCKET_ONLINE) units up to this long farm them
#endif
#ifndef P_POCKET_ONLINE_MIN_EXP
#define P_POCKET_ONLINE_MIN_EXP 2.0  // (P_POCKET_ONLINE) fast tiles a pocket needs
#endif
#ifndef P_POCKET_MIN_EXP
#define P_POCKET_MIN_EXP 3.0  // a pocket must hold at least this many fast-respawning pearl tiles
#endif
#ifndef P_POCKET_MIN_ROUND
#define P_POCKET_MIN_ROUND 30  // no voluntary pocket visits before this round (the opening is a knife fight)
#endif
#ifndef P_POCKET_COST
#define P_POCKET_COST 1.0  // price of entering a pocket (the stub that dies at the end)
#endif
#ifndef P_ROLE_FEED_START
#define P_ROLE_FEED_START 60  // Stronghold/Trauma: dragons that are neither king nor farmer merge into the king from here
#endif
#ifndef P_COURIER_LEN
#if P_TRAUMA_PLAN == 3
#define P_COURIER_LEN 0
#else
#define P_COURIER_LEN 13
#endif  // Stronghold/Trauma: a farmer this long splits off its tail to feed the king (0 = never)
#endif
#ifndef P_COURIER_PULL
#define P_COURIER_PULL 3.0  // ...pull of a courier toward the king (map distance, any range)
#endif
#ifndef P_COURIER_KINGMAX
#define P_COURIER_KINGMAX 100  // ...no couriers while the king is at least this long (they would crowd it)
#endif
#ifndef P_COURIER_BLIND_UNTIL
#define P_COURIER_BLIND_UNTIL 150  // ...when the king's length is unknown, assume it is long from this round
#endif
#ifndef P_FARMERS
#define P_FARMERS 1  // ...into a second farmer while the team has fewer farmers than this, else into a courier
#endif
#ifndef P_FARMER_KEEP
#define P_FARMER_KEEP 5  // ...keeping about this many segments to go on farming
#endif
#ifndef P_POCKET_CONT
#define P_POCKET_CONT 1.0  // planner: value of what follows a pocket visit (the child walks on)
#endif
#ifndef P_POCKET_PULL
#define P_POCKET_PULL 3.0  // designated farmers (Stronghold, Trauma): pull toward the nearest free pocket
#endif
#ifndef P_DEADEND_MINPEARLS
#define P_DEADEND_MINPEARLS 2.0  // ...only if at least this many pearls are expected inside
#endif

// ==============================================================================================
// HUNTING AND KILL MODE
// ==============================================================================================
#ifndef P_HUNT
#define P_HUNT 1.0  // default for the per-map hunt column (attraction toward big enemies)
#endif
#ifndef P_HUNT_AGE
#define P_HUNT_AGE 12  // sightings older than this (rounds) are ignored for hunting
#endif
#ifndef P_HUNT_MIN
#define P_HUNT_MIN 10  // only hunt enemies at least this long
#endif
#ifndef P_HUNT_MARGIN
#define P_HUNT_MARGIN 3  // ...and at least this much longer than the hunter
#endif
#ifndef P_HUNT_MINUNITS
#define P_HUNT_MINUNITS 4  // hunt only while we have at least this many units
#endif
#ifndef P_HUNT_BODY
#define P_HUNT_BODY 0  // (off) follow visible big enemy bodies toward their head
#endif
#ifndef P_HUNT_BODY_MIN
#define P_HUNT_BODY_MIN 8  // (off) minimum visible body length to follow
#endif
#ifndef P_KILL
#define P_KILL 1  // 1 = kill mode: few enemies seen lately and we outnumber them -> hunt all, any trade
#endif
#ifndef P_KILL_WINDOW
#define P_KILL_WINDOW 30  // rounds of sightings counted
#endif
#ifndef P_KILL_MIN_ROUND
#define P_KILL_MIN_ROUND 100  // kill mode never before this round
#endif
#ifndef P_KILL_MAX_ENEMY
#define P_KILL_MAX_ENEMY 3  // at most this many distinct enemies seen in the window
#endif
#ifndef P_KILL_RATIO
#define P_KILL_RATIO 3  // and we have at least ratio x that + 2 units
#endif
#ifndef P_HUNT_KILL
#define P_HUNT_KILL 3.0  // hunt attraction in kill mode
#endif

// ==============================================================================================
// KING (the longest dragon of the team)
// ==============================================================================================
#ifndef P_QUEEN_SAFE
#define P_QUEEN_SAFE 1  // v18: dragon 0/1 is the queen (round 500 tiebreak: queen length first): no pocket farming, no donation, no head-on trade
#endif
#ifndef P_QUEEN_SURV_DEPTH
#define P_QUEEN_SURV_DEPTH 20  // v18: the queen needs this many moves of room (not just her length + 2)
#endif
#ifndef P_QUEEN_STRIKE
#define P_QUEEN_STRIKE 0.8  // v18: assumed chance that any enemy in reach strikes the queen
#endif
#ifndef P_QUEEN_RISK
#define P_QUEEN_RISK 3.0  // v18: threat weight multiplier for the queen
#endif
#ifndef P_QUEEN_HOME_R
#define P_QUEEN_HOME_R 6  // v18: the queen stays within this many steps of her spawn (0 = off)
#endif
#ifndef P_QUEEN_HOME_PULL
#define P_QUEEN_HOME_PULL 4.0  // v18: score per step beyond P_QUEEN_HOME_R for a move away from home
#endif
#ifndef P_QUEEN_ROOM_R
#define P_QUEEN_ROOM_R 2  // v18: our other dragons keep this many steps from the queen's head
#endif
#ifndef P_QUEEN_ROOM_PEN
#define P_QUEEN_ROOM_PEN 3.0  // v18: score per step inside that radius
#endif
#ifndef P_QUEEN_NODIE
#define P_QUEEN_NODIE 1e9  // v18: score charged to a queen candidate that kills her on purpose
#endif
#ifndef P_KING
#define P_KING 1  // 1 = king logic on (0 = pure swarm)
#endif
#ifndef P_KING_SPLIT_UNTIL
#define P_KING_SPLIT_UNTIL 100  // default for the per-map kingSplitUntil column
#endif
#ifndef P_KING_CLAIM_LEN
#define P_KING_CLAIM_LEN 5  // minimum length to be king
#endif
#ifndef P_KING_FRESH
#define P_KING_FRESH 40  // rounds a king report stays valid (reports are relayed over sonar)
#endif
#ifndef P_KING_RELAY_AGE
#define P_KING_RELAY_AGE 40  // relay king reports up to this age
#endif
#ifndef P_KING_RISK
#define P_KING_RISK 2.0  // threat weight multiplier for the king once it stops splitting
#endif
#ifndef P_KING_STRIKE
#define P_KING_STRIKE 0.4  // assumed strike chance of any shorter enemy in reach of the king (v8 guard build: 0.9)
#endif
#ifndef P_KING_SURV_DEPTH
#define P_KING_SURV_DEPTH 130  // survival search depth for the king
#endif
#ifndef P_LONG_DFS_LIMIT
#define P_LONG_DFS_LIMIT 15000  // node budget of that search for other dragons longer than P_SURV_DEPTH
#endif
#ifndef P_LONG_TRAP
#define P_LONG_TRAP 3000.0  // penalty when a long dragon can neither follow its tail nor reach open space
#endif
#ifndef P_CUT_TRAP
#define P_CUT_TRAP 8000.0  // penalty when a long dragon's survival search ran out of nodes and the tail/space test fails
#endif
#ifndef P_LONG_ALLMAPS
#define P_LONG_ALLMAPS 0  // 0 = P_LONG_TRAP and the room check only on maps without splitting (Stronghold/Trauma)
#endif
#ifndef P_LONG_TRAP_LEN
#define P_LONG_TRAP_LEN 12  // ...applied to dragons at least this long
#endif
#ifndef P_TAIL_SLACK
#define P_TAIL_SLACK 2  // extra moves of margin when checking that a long dragon can follow its tail
#endif
#ifndef P_ROOM_FACTOR
#define P_ROOM_FACTOR 2.5  // a dragon stops valuing pearls as its length approaches room/this (room = connected area around it)
#endif
#ifndef P_ROOM_MINLEN
#define P_ROOM_MINLEN 20  // ...only checked for dragons at least this long
#endif
#ifndef P_KING_DFS_LIMIT
#define P_KING_DFS_LIMIT 40000  // node budget of the king's survival search
#endif
#ifndef P_KING_DANGER
#define P_KING_DANGER 0.6  // king avoids tiles near recent enemy sightings with this weight
#endif
#ifndef P_DANGER_AGE
#define P_DANGER_AGE 10  // sightings younger than this count as danger
#endif
#ifndef P_KING_EXPO
#define P_KING_EXPO 0.03  // long dragons: penalty per length unit for ending a move within sprint reach of an enemy seen recently but out of view now
#endif
#ifndef P_EXPO_MINLEN
#define P_EXPO_MINLEN 15  // (with P_KING_EXPO) dragons at least this long count the exposure
#endif
#ifndef P_EXPO_AGE
#define P_EXPO_AGE 12  // sightings up to this many rounds old count
#endif
#ifndef P_KING_AWAY
#define P_KING_AWAY 0.5  // extra king penalty on the enemy half of the map
#endif
#ifndef P_KING_DEFER
#define P_KING_DEFER 0.03  // default for the per-map kingDefer column
#endif
#ifndef P_DEFER_RADIUS
#define P_DEFER_RADIUS 8  // pearls within this distance of the king are left to it
#endif
#ifndef P_DEFER_SLACK
#define P_DEFER_SLACK 2  // king counts as "first" if it arrives at most this many moves later
#endif
#ifndef P_KING_SPACE
#define P_KING_SPACE 3  // penalty for other heads within 2 steps of the king's head
#endif
#ifndef P_KING_LEN
#define P_KING_LEN 9999  // (off) any dragon this long behaves like a king
#endif
#ifndef P_KING_NOSPLIT
#define P_KING_NOSPLIT 1  // dragons longer than P_KING_LEN never split
#endif

// ==============================================================================================
// FEEDING (endgame consolidation into the king)
// ==============================================================================================
#ifndef P_FEED_START
#define P_FEED_START 400  // default for the per-map feedStart column
#endif
#ifndef P_FEED_PULL_START
#define P_FEED_PULL_START 380  // default for the per-map feedPullStart column
#endif
#ifndef P_FEED_PULL
#define P_FEED_PULL 1  // 1 = pull units toward the king from feedPullStart
#endif
#ifndef P_FEED_MINUNITS
#define P_FEED_MINUNITS 6  // feed only while we have at least this many units (2 on no-split maps)
#endif
#ifndef P_FEED_KEEP_FRAC
#define P_FEED_KEEP_FRAC 0.0  // endgame feeding stops below this x max(enemies seen in the last 40 rounds, most units we had) (0.4 tested: lost on Big Empty)
#endif
#ifndef P_FKO_KNOWN
#define P_FKO_KNOWN 0  // v12: 1 = P_FEED_KING_ONLY on the known maps only
#endif
#ifndef P_FEED_KING_ONLY
#define P_FEED_KING_ONLY 0  // 1 = endgame feeding only into the king (not into whatever big ally is at hand when the king is far)
#endif
#ifndef P_FEED_MARGIN
#define P_FEED_MARGIN 2  // feed only dragons at least this much longer than the donor
#endif
#ifndef P_FEED_ATTRACT
#define P_FEED_ATTRACT 2.0  // attraction toward the ring around the king's head
#endif
#ifndef P_DONATE_DIST
#define P_DONATE_DIST 4  // donate (die) within this distance of the king's head
#endif
#ifndef P_DONATE_MAXPEARLS
#define P_DONATE_MAXPEARLS 99  // ...unless it already has this many pearls around it
#endif
#ifndef P_LOCAL_FEED_DIST
#define P_LOCAL_FEED_DIST 20  // if the king is farther than this, feed the biggest dragon at hand
#endif
#ifndef P_CAP_FEED
#define P_CAP_FEED 1  // 1 = at the unit cap, long units feed the king even before feedStart
#endif
#ifndef P_CAP_FEED_MARGIN
#define P_CAP_FEED_MARGIN 2  // "at the cap" = unit count >= limit - this
#endif
#ifndef P_CAP_FEED_MINLEN
#define P_CAP_FEED_MINLEN 3  // minimum length of a unit that feeds at the cap
#endif
#ifndef P_BIG_LEAD
#define P_BIG_LEAD 40  // start advertising the biggest ally this many rounds before feedStart
#endif
#ifndef P_BIG_MINLEN
#define P_BIG_MINLEN 8  // minimum length to advertise itself as big
#endif
#ifndef P_ISO_ROUNDS
#define P_ISO_ROUNDS 0  // (off) isolation mode: no enemy seen for N rounds -> stop splitting
#endif
#ifndef P_ISO_MIN_ROUND
#define P_ISO_MIN_ROUND 120  // (off) isolation mode not before this round
#endif
#ifndef P_ISO_FEED_START
#define P_ISO_FEED_START 350  // (off) feeding start in isolation mode
#endif
#ifndef P_SPACE_ALLY
#define P_SPACE_ALLY 0  // (off) count allies as competitors in the space check
#endif

// ==============================================================================================
// PER-MAP STRATEGY
// Map name (MAP_NAME line of the .map file) followed by
//   splitMin       split when at least this long (9999 = never)
//   maxUnits       stop splitting at this many units
//   kingSplitUntil the king keeps splitting until this round, then only grows (9999 = no king phase)
//   growRound      nobody splits from this round on
//   feedStart      from this round units donate themselves next to the king
//   feedPullStart  from this round units are pulled toward the king
//   hunt           attraction toward the biggest enemy seen recently (0 = off)
//   kingDefer      other units value pearls the king reaches first at this factor (1 = no deference)
//   deadEndCost    penalty for entering a dead end that can only be left by splitting
//   unseenEarly    rounds during which never-seen pearls are trusted more; 0 = off
//   pocketMaxLen   units up to this long farm dead-end pockets (0 = off). With splitMin 9999 it also
//                  switches on the king / farmer / courier roles (Stronghold, Trauma).
// Maps not listed (Big Empty, Schooltime, unknown maps) use the P_ defaults above.
// ==============================================================================================
#ifndef P_ADAPT_CFG
#define P_ADAPT_CFG 1  // 1 = unknown maps smaller than P_BIG_MAP_TILES play the pure-swarm plan (no king until the end)
#endif
#ifndef P_BIG_MAP_TILES
#define P_BIG_MAP_TILES 2000  // (P_ADAPT_CFG) unknown maps with at least this many tiles keep the king plan
#endif
#ifndef P_SMALL_GROW_ROUND
#define P_SMALL_GROW_ROUND 450  // (P_ADAPT_CFG) small unknown maps: last split round
#endif
#ifndef P_SMALL_FEED_START
#define P_SMALL_FEED_START 450  // (P_ADAPT_CFG) small unknown maps: endgame feeding starts
#endif
#ifndef P_SMALL_FEED_PULL_START
#define P_SMALL_FEED_PULL_START 430  // (P_ADAPT_CFG) small unknown maps: units start walking to the biggest ally
#endif

// ---------------------------------------------------------------- v5 ENDGAME (E2)
// Ladder games that reach round 500 are decided by the longest dragon; the top teams merge
// everything into one king from about round 360 (their king then grows ~1 per round).
#ifndef P_E2_FEED
#define P_E2_FEED 0  // >0: endgame feeding starts no later than this round (maps with feedStart >= 300)
#endif
#ifndef P_E2_PULL
#define P_E2_PULL 0  // >0: pull toward the king starts no later than this round
#endif
#ifndef P_E2_GROW
#define P_E2_GROW 0  // >0: nobody splits after this round
#endif
#ifndef P_E2_KING_SAFE
#define P_E2_KING_SAFE 1  // 1 = in the endgame the king takes no blind portal and weighs threats P_KING_RISK times
#endif
#ifndef P_E2_KING_NOATTACK
#define P_E2_KING_NOATTACK 0  // (off) 1 = in the endgame the king never takes a head-on trade unless trapped
#endif
#ifndef P_E2_LEAD
#define P_E2_LEAD 20  // ...from this many rounds before feedPullStart
#endif
#ifndef P_E2_LONGSAFE
#define P_E2_LONGSAFE 1  // 1 = the king (any map) avoids moves after which it can neither follow its tail nor reach open space
#endif
#ifndef P_E2_HORIZON_MARGIN
#define P_E2_HORIZON_MARGIN 0  // v14 (off): survival checks and the long-dragon trap test look this many rounds past the last round (a 52-long king on Schooltime coiled in its last 40 rounds, ate, was trapped at round 472 and shredded itself by rescue splits); 15: 24 of 38 against 25
#endif
#ifndef P_E2_HORIZON
#define P_E2_HORIZON 1  // 1 = survival checks look no further than the last round (no rescue splits at round 498)
#endif
#ifndef P_E2_KING_FRESH
#define P_E2_KING_FRESH 15  // >0: from feedPullStart a king report is valid this many rounds (P_KING_FRESH before)
#endif
#ifndef P_E2_HUNT
#define P_E2_HUNT 0  // 1 = endgame hunters: some small dragons go after the enemy's longest dragon
#endif
#ifndef P_E2_HUNT_START
#define P_E2_HUNT_START 350  // ...from this round
#endif
#ifndef P_E2_HUNT_MINLEN
#define P_E2_HUNT_MINLEN 3  // ...hunters are at least this long (a dragon of length L strikes up to L-1 tiles away)
#endif
#ifndef P_E2_HUNT_MAXLEN
#define P_E2_HUNT_MAXLEN 10  // ...and at most this long
#endif
#ifndef P_E2_HUNT_FRAC
#define P_E2_HUNT_FRAC 0.25  // ...share of the dragons that hunt (chosen by id)
#endif
#ifndef P_E2_HUNT_AGE
#define P_E2_HUNT_AGE 20  // target: enemy head seen within this many rounds...
#endif
#ifndef P_E2_HUNT_TGT
#define P_E2_HUNT_TGT 12  // ...at least this long...
#endif
#ifndef P_E2_HUNT_MARGIN
#define P_E2_HUNT_MARGIN 5  // ...and at least our king's length minus this
#endif
#ifndef P_E2_HUNT_ATTRACT
#define P_E2_HUNT_ATTRACT 3.0  // attraction toward the target's head
#endif
#ifndef P_E2_HUNT_PULL
#define P_E2_HUNT_PULL 2.0  // long-range pull toward it (map distance)
#endif
#ifndef P_E2_DUMP_ROUND
#define P_E2_DUMP_ROUND 0  // >0: from this round every dragon near the king feeds it, whatever the unit count
#endif
#ifndef P_RV
#define P_RV 0  // (off) endgame meeting tile, everyone walks there and feeds the biggest dragon on it: 1 = chosen by the king and spread over sonar, 2 = our first spawn tile
#endif
#ifndef P_RV_LEAD
#define P_RV_LEAD 60  // the king chooses it this many rounds before feedPullStart (anyone without one chooses at feedPullStart)
#endif
#ifndef P_RV_SEARCH
#define P_RV_SEARCH 8  // ...the most open seen tile within this many steps of the chooser's head
#endif
#ifndef P_RV_ZONE
#define P_RV_ZONE 6  // feeding happens only within this distance of the meeting tile
#endif
#ifndef P_RV_PULL
#define P_RV_PULL 3.0  // pull toward the meeting tile (map distance, any range)
#endif
#ifndef P_RV_MINTARGET
#define P_RV_MINTARGET 6  // at the meeting tile, feed only dragons at least this long
#endif
#ifndef P_E2_PATHDIST
#define P_E2_PATHDIST 0  // 1 = donors measure the distance to the king through the map (walls, portals), not in a straight line
#endif
#ifndef P_LOCALMIN_MAPS
#define P_LOCALMIN_MAPS "Schooltime", "Slithery Fight"  // v12: known maps (comma-separated names) where endgame feeding goes into a dragon other than the king only if it is P_LOCALMIN_LEN+ long (v11 fed any longer ally near when the king was far: several mid-size "local kings" and a short king)
#endif
#ifndef P_LOCALMIN_LEN
#define P_LOCALMIN_LEN 12
#endif
#ifndef P_E2_LOCALMIN
#define P_E2_LOCALMIN 0  // feed a dragon other than the king only if it is at least this long
#endif

// ---- v6: per-map plans for Schooltime, Trauma, Portals and Slithery Fight (see README)
#ifndef P_SCH_ZONE_PULL
#define P_SCH_ZONE_PULL 2.0  // Schooltime: pull toward the rich block at the bottom middle (only reachable through portals)
#endif
#ifndef P_SCH_ZONE_FRAC
#define P_SCH_ZONE_FRAC 0.3  // ...share of the dragons (by id) that answer it
#endif
#ifndef P_SCH_ZONE_UNTIL
#define P_SCH_ZONE_UNTIL 380  // ...until this round
#endif
#ifndef P_DEVIL_UNSEEN
#define P_DEVIL_UNSEEN 60  // Devil: unseenEarly column
#endif
#ifndef P_DEFAULT_ZONE_PULL
#define P_DEFAULT_ZONE_PULL 0.0  // v13: Default: pull of a share of the units toward a zone (with P_ZONE_CENTER_MAPS: the portal rooms in the middle)
#endif
#ifndef P_DEFAULT_ZONE_FRAC
#define P_DEFAULT_ZONE_FRAC 0.3
#endif
#ifndef P_DEFAULT_ZONE_UNTIL
#define P_DEFAULT_ZONE_UNTIL 380
#endif
#ifndef P_ZONE_CENTER_MAPS
#define P_ZONE_CENTER_MAPS ""  // v13: known maps whose zone pull goes to the zone nearest the map centre, whatever is closer or richer (Default: the rooms the top team lives in)
#endif
#ifndef P_QUEEN_ZONE_PULL
#define P_QUEEN_ZONE_PULL 0.0  // v13: Queen Of Spades: pull of a share of the units toward the two fast boxes (each reached through a portal)
#endif
#ifndef P_QUEEN_ZONE_FRAC
#define P_QUEEN_ZONE_FRAC 0.5
#endif
#ifndef P_QUEEN_ZONE_UNTIL
#define P_QUEEN_ZONE_UNTIL 150
#endif
#ifndef P_QUEEN_UNSEEN
#define P_QUEEN_UNSEEN 60  // Queen Of Spades: unseenEarly column
#endif
#ifndef P_DEFAULT_UNSEEN
#define P_DEFAULT_UNSEEN 60  // Default: unseenEarly column
#endif
#ifndef P_DILEMMA_UNSEEN
#define P_DILEMMA_UNSEEN 200  // Prisoners Dilemma: unseenEarly column
#endif
#ifndef P_AUTARKY_UNSEEN
#define P_AUTARKY_UNSEEN 200  // Autarky: unseenEarly column
#endif
#ifndef P_DSMALL_UNSEEN
#define P_DSMALL_UNSEEN 60  // Default Small: unseenEarly column
#endif
#ifndef P_TROPHY_UNSEEN
#define P_TROPHY_UNSEEN 60  // Trophy: unseenEarly column (rounds of extra trust in pearls not seen yet)
#endif
#ifndef P_TROPHY_ZONE_PULL
#define P_TROPHY_ZONE_PULL 0.0  // Trophy: pull toward the centre cup
#endif
#ifndef P_TROPHY_ZONE_FRAC
#define P_TROPHY_ZONE_FRAC 0.3
#endif
#ifndef P_TROPHY_ZONE_UNTIL
#define P_TROPHY_ZONE_UNTIL 150
#endif
#ifndef P_PORTALS_DUMP
#define P_PORTALS_DUMP 360  // v14: Portals: from this round every dragon feeds a longer one, down to 2 units (0 = P_E7_DUMP, 445, as v13). With GROW/FEED/PULL 280/320/300: 23 of 24 against 17, our longest 49.5 against 38.8
#endif
#ifndef P_PORTALS_FKO
#define P_PORTALS_FKO 0  // v14 (off): Portals: 1 = endgame feeding only into the king (22 of 24, as good)
#endif
#ifndef P_PORTALS_FEED
#define P_PORTALS_FEED 320  // Portals: feedStart (v13: 360)
#endif
#ifndef P_PORTALS_GROW
#define P_PORTALS_GROW 280  // v14: Portals: nobody splits after this round (v13: = P_PORTALS_FEED, 360)
#endif
#ifndef P_PORTALS_PULL
#define P_PORTALS_PULL 300  // Portals: feedPullStart (v13: 340)
#endif
#ifndef P_PORTALS_RV
#define P_PORTALS_RV 0  // Portals: endgame meeting tile mode (0 = none, 2 = our first spawn tile)
#endif
#if P_TRAUMA_PLAN == 3
#define P_TRAUMA_ROW {"Trauma",  9999,   64,    0,   0, 440, 410, 0.0, 0.15, 5.0, 60, 999}
#elif P_TRAUMA_PLAN == 1
#define P_TRAUMA_ROW {"Trauma",     4, 9999,  100, 400, 400, 380, 1.0, 0.03, 5.0, 60, 12, 0.0, 0.0, 0, 2}
#elif P_TRAUMA_PLAN == 2
#define P_TRAUMA_ROW {"Trauma",  9999,   64,    0,   0, 440, 410, 0.0, 0.15, 5.0, 60, 999}
#elif P_TRAUMA_PLAN == 4
#define P_TRAUMA_ROW {"Trauma",     4, 9999,  100, P_TRAUMA_GROW, P_TRAUMA_FEED, P_TRAUMA_PULL, 1.0, 0.03, 5.0, 60,  0}, /* v13: plan 4 = the round-500 swarm (Slithery's row) */
#else
#define P_TRAUMA_ROW {"Trauma",  9999,   64,    0,   0,  60,  50, 0.0, 0.15, 5.0, 60,  0}
#endif


// ---------------------------------------------------------------- v7
#ifndef P_INVADE_FRAC
#define P_INVADE_FRAC 0.0  // v15 (sparring opponents only): share of the dragons (by id) pulled into the other team's home from P_INVADE_START
#endif
#ifndef P_INVADE_START
#define P_INVADE_START 150
#endif
#ifndef P_INVADE_PULL
#define P_INVADE_PULL 3.0
#endif
#ifndef P_INVADE_ARRIVE
#define P_INVADE_ARRIVE 2  // ...until this close to one of its starting tiles
#endif
#ifndef P_ELIM_SPLIT
#define P_ELIM_SPLIT 4  // v15: splitMin of the elimination maps' rows (Autarky, Default, Devil, Dilemma, Queen, Trophy): a dragon splits in two from this length
#endif
#ifndef P_GRAD_FEED
#define P_GRAD_FEED 0  // v15 (off): on the P_LOCALMIN_MAPS (Schooltime, Slithery Fight), from this round dragons of P_GRAD_FEED_MINLEN+ feed the king while shorter ones farm on
#endif
#ifndef P_GRAD_FEED_MINLEN
#define P_GRAD_FEED_MINLEN 6
#endif
#ifndef P_KING_DROP_PEN
#define P_KING_DROP_PEN 0.0  // v15: from the endgame pull, a non-king dragon pays this per pearl it would eat within P_KING_DROP_R of the king's head (the pearls feeders drop for the king: on the ladder's Portals game other dragons of ours ate 47 of the 96, the king 38)
#endif
#ifndef P_KING_DROP_R
#define P_KING_DROP_R 4
#endif
#ifndef P_TRAUMA_GATE
#define P_TRAUMA_GATE 0  // v15: Trauma: a 2-3-long courier circles the 2x2 block at the landing tile of the home entrance our couriers do not use
#endif
#ifndef P_GATE_SWARM
#define P_GATE_SWARM 0  // v15: the gate and the home guard only once this many different enemy dragons were seen in the last 60 rounds (0 = always)
#endif
#ifndef P_GATE_START
#define P_GATE_START 100
#endif
#ifndef P_GATE_R
#define P_GATE_R 12  // ...eligible couriers within this many steps walk to it
#endif
#ifndef P_GATE_PULL
#define P_GATE_PULL 4.0
#endif
#ifndef P_HEIR
#define P_HEIR 0  // v16: Trauma (plan 3): a share P_HEIR_FRAC of the couriers (by id), once P_HEIR_MIN long and from round P_HEIR_START, stops carrying its length to the king (an heir if the king dies)
#endif
#ifndef P_HEIR_MIN
#define P_HEIR_MIN 6
#endif
#ifndef P_HEIR_START
#define P_HEIR_START 150
#endif
#ifndef P_HEIR_FRAC
#define P_HEIR_FRAC 0.25
#endif
#ifndef P_SECTOR
#define P_SECTOR 0  // v16: 1 = sector spread on the maps in P_SECTOR_MAPS: every dragon (not the king, not a zone holder) is pulled toward its home sector (P_SECTOR_SIZE x P_SECTOR_SIZE tiles, chosen by id and weighted by pearl production) while elsewhere
#endif
#ifndef P_SECTOR_MAPS
#define P_SECTOR_MAPS "*"
#endif
#ifndef P_SECTOR_SIZE
#define P_SECTOR_SIZE 10
#endif
#ifndef P_SECTOR_PULL
#define P_SECTOR_PULL 1.0
#endif
#ifndef P_SECTOR_START
#define P_SECTOR_START 0
#endif
#ifndef P_SECTOR_UNTIL
#define P_SECTOR_UNTIL 9999
#endif
#ifndef P_SECTOR_MAXLEN
#define P_SECTOR_MAXLEN 12
#endif
#ifndef P_GROW_UNITS
#define P_GROW_UNITS 64  // v16: no growth split once we have this many units (the slots above stay free for escape splits of trapped dragons; the engine refuses any split at 64)
#endif
#ifndef P_HOLD
#define P_HOLD 1  // v16: zone hold on the maps listed in P_HOLD_TABLE: a share P_HOLD_FRAC of our dragons up to P_HOLD_MAXLEN long walks to the zone (pull P_HOLD_PULL) and, inside, pays P_HOLD_STAY for a move that leaves it
#endif
#ifndef P_HOLD_TABLE
#define P_HOLD_TABLE {"Devil", 14, 3, 17, 12, 0}
#endif
#ifndef P_HOLD_FRAC
#define P_HOLD_FRAC 0.4
#endif
#ifndef P_HOLD_PULL
#define P_HOLD_PULL 4.0
#endif
#ifndef P_HOLD_STAY
#define P_HOLD_STAY 6.0
#endif
#ifndef P_HOLD_START
#define P_HOLD_START 0
#endif
#ifndef P_HOLD_UNTIL
#define P_HOLD_UNTIL 9999
#endif
#ifndef P_HOLD_MAXLEN
#define P_HOLD_MAXLEN 12
#endif
#ifndef P_RUSH_NOTRADE
#define P_RUSH_NOTRADE 0  // v16: 1 = the opening rusher takes no trade that does not gain length before it arrives
#endif
#ifndef P_KING_GATE
#define P_KING_GATE 0  // v15: Trauma (plan 3): 1 = once an enemy dragon has been seen inside our home field the king circles a loop through the couriers' landing tile (an enemy coming through lands on its body); 2 = from P_KG_START whatever it saw; 0 = off
#endif
#ifndef P_KG_START
#define P_KG_START 150
#endif
#ifndef P_KG_UNTIL
#define P_KG_UNTIL 9999
#endif
#ifndef P_KG_PULL
#define P_KG_PULL 6.0  // pull of the king toward the loop
#endif
#ifndef P_KG_BONUS
#define P_KG_BONUS 20.0  // bonus of the move that keeps it going round
#endif
#ifndef P_SURV_MAYBE
#define P_SURV_MAYBE 0.3  // v15: >0 = dragons of P_SURV_MAYBE_LEN+ count a pearl on an unseen tile of at least this probability as eaten when they check their room (their tail stays)
#endif
#ifndef P_SURV_MAYBE_LEN
#define P_SURV_MAYBE_LEN 12
#endif
#ifndef P_SURV_MAYBE_WHERE
#define P_SURV_MAYBE_WHERE 1  // 0 = every map, 1 = Portals only
#endif
#ifndef P_FENCE_SWARM
#define P_FENCE_SWARM 0  // v15: Trauma: >0 = the king keeps to its home field once it has seen this many enemy dragons within 60 rounds (0 = never unless P_KING_FENCE)
#endif
#ifndef P_FENCE_START
#define P_FENCE_START 0  // ...not before this round
#endif
#ifndef P_FENCE_UNTIL
#define P_FENCE_UNTIL cfg.feedPullStart  // ...and until this round (v10-v12's fence: the endgame pull, round 410)
#endif
#ifndef P_GATE_DOOR
#define P_GATE_DOOR 0  // v15: 0 = the gate holds the landing farthest from the couriers' one, 1 = the couriers' landing itself
#endif
#ifndef P_INVADE_DOOR
#define P_INVADE_DOOR 0  // v15 (sparring opponent): 1 = invaders go in through the door the other team's couriers use
#endif
#ifndef P_GATE_MINDIST
#define P_GATE_MINDIST 3  // ...only if that landing is at least this far from the couriers' one
#endif
#ifndef P_HOME_GUARD
#define P_HOME_GUARD 0  // v15: Trauma: with P_KING_GUARD, our short dragons strike any enemy head inside our home field
#endif
#ifndef P_PORTALS_KRISK
#define P_PORTALS_KRISK 0.0  // v15: P_PORTAL_RISK_KING on the Portals map (-1 = the general value, 0.5)
#endif
#ifndef P_PORTALS_KRISK_END
#define P_PORTALS_KRISK_END 450  // ...until this round (then the general value again: a king that dies in the last rounds cannot be rebuilt)
#endif
#ifndef P_PSPRINT
#define P_PSPRINT 1  // v15: sprints through pearls (each pearl eaten after the first step gives back the segment the step costs): 1 = our strikes use them and our threat model expects them, 2 = strikes only, 3 = threat model only, 0 = off (v14)
#endif
#ifndef P_PSPRINT_WHERE
#define P_PSPRINT_WHERE 1  // v15: 0 = everywhere, 1 = not on the known round-500 maps (Schooltime, Slithery Fight, Portals, Trauma)
#endif
#ifndef P_PSPRINT_MAX
#define P_PSPRINT_MAX 8  // ...longest such sprint considered (steps)
#endif
#ifndef P_KING_SPACE_MARGIN
#define P_KING_SPACE_MARGIN 2  // v14: the king (and dragons of P_KING_SPACE_MARGIN_LEN+) count as their room only tiles they reach this many moves before any enemy head (0 = v13). On the ladder (Trauma) a 2-long enemy closed the only way out of our 34-long king's coil
#endif
#ifndef P_KING_SPACE_MARGIN_LEN
#define P_KING_SPACE_MARGIN_LEN 12
#endif
#ifndef P_SPREAD_W
#define P_SPREAD_W 3.0  // v14: penalty per unit of closeness to our other heads (R + 1 - distance, Chebyshev), before the endgame pull; 0 = off (v13). On the ladder 210 (Schooltime) and 297 (Slithery) of our dragons died boxed in by our own bodies
#endif
#ifndef P_SPREAD_R
#define P_SPREAD_R 2
#endif
#ifndef P_SIM_INHERIT
#define P_SIM_INHERIT 0  // (simulator-only experiment, never on the judge) newborns start with their parent's map knowledge
#endif
#ifndef P_EVADE_SPRINT
#define P_EVADE_SPRINT 0  // v17: 1 = long dragons a shorter enemy can strike also weigh 2-step sprints (2 = and 3-step), 0 = off
#endif
#ifndef P_EVADE_KNOWN
#define P_EVADE_KNOWN 0  // ...1 = also on the known maps (0 = unknown maps only)
#endif
#ifndef P_EVADE_MINLEN
#define P_EVADE_MINLEN 10  // ...dragons at least this long (the king whatever its length)
#endif
#ifndef P_EARLY_SPREAD
#define P_EARLY_SPREAD 0  // v17: unknown maps: the spread penalty applies before this round (the opening: find the map's riches)
#endif
#ifndef P_OHOLD_STAY
#define P_OHOLD_STAY 4.0  // v17: unknown maps (needs P_ZONE_ONLINE): cost of leaving a rich spot (0 = off)
#endif
#ifndef P_OHOLD_MAXN
#define P_OHOLD_MAXN 2000  // ...only on maps with fewer tiles than this (0 = every size)
#endif
#ifndef P_OHOLD_RATE
#define P_OHOLD_RATE 0.15  // ...a spot is rich when the known spawn tiles within P_ZONE_RADIUS give this many pearls a round
#endif
#ifndef P_OHOLD_FRAC
#define P_OHOLD_FRAC 0.6  // ...share of the dragons (by id) that hold
#endif
#ifndef P_OHOLD_MAXLEN
#define P_OHOLD_MAXLEN 12  // ...dragons up to this long
#endif
#ifndef P_DYN_KING
#define P_DYN_KING 1  // v17: 1 = unknown maps: a king (our longest stops splitting) once an enemy of P_DYN_KING_LEN+ has been seen
#endif
#ifndef P_DYN_KING_LEN
#define P_DYN_KING_LEN 12
#endif
#ifndef P_DYN_KING_FROM
#define P_DYN_KING_FROM 150  // ...not before this round
#endif
#ifndef P_UNK_FEED
#define P_UNK_FEED 0  // v17: unknown maps: endgame feeding from this round at the latest (0 = the config's)
#endif
#ifndef P_UNK_PULL
#define P_UNK_PULL 0  // ...pull toward the king from this round at the latest
#endif
#ifndef P_UNK_GROW
#define P_UNK_GROW 0  // ...nobody splits after this round
#endif
#ifndef P_UNK_KING_UNTIL
#define P_UNK_KING_UNTIL 100  // v17: unknown maps: a king from this round at the latest (9999 = the config's)
#endif
#ifndef P_SMALL_KING_UNTIL
#define P_SMALL_KING_UNTIL 9999  // v17: (P_ADAPT_CFG) small unknown maps: a king from this round (9999 = none until the end, as v16)
#endif
#ifndef P_SURV_UNKPORTAL
#define P_SURV_UNKPORTAL 1  // v17: 1 = the survival search counts an unexplored portal as a way out (unknown maps: rooms behind portals were dead ends)
#endif
#ifndef P_SURV_UNKPORTAL_MIN
#define P_SURV_UNKPORTAL_MIN 1  // ...reached at this step or later
#endif
#ifndef P_CENTER_RUSH
#define P_CENTER_RUSH 0.0  // v17: unknown maps: pull toward the map centre in the opening (0 = off)
#endif
#ifndef P_CENTER_FRAC
#define P_CENTER_FRAC 0.5  // ...share of the dragons (by id)
#endif
#ifndef P_CENTER_UNTIL
#define P_CENTER_UNTIL 80  // ...until this round
#endif
#ifndef P_CENTER_ARRIVE
#define P_CENTER_ARRIVE 3  // ...no pull within this path distance of the centre
#endif
#ifndef P_CENTER_MAXLEN
#define P_CENTER_MAXLEN 12  // ...dragons up to this long
#endif
#ifndef P_LOCAL_UNKNOWN
#define P_LOCAL_UNKNOWN 1  // v17: 1 = the local-numbers rules (P_ATTACK_LOCAL, P_OUTNUM_P) also on unknown maps that play the small-map plan (P_ADAPT_CFG)
#endif
#ifndef P_SPREAD_UNKNOWN
#define P_SPREAD_UNKNOWN 0  // v17: 1 = the spread penalty (P_SPREAD_W) also applies on unknown maps, 2 = on unknown maps of P_BIG_MAP_TILES+
#endif
#ifndef P_SPREAD_MAPS
#define P_SPREAD_MAPS "Schooltime", "Slithery Fight"  // v14: known maps where P_SPREAD_W applies ("*" = every known map: the elimination maps fell to 29 of 72 from 53; Portals unchanged)
#endif
#ifndef P_DB_EXTRA_MAX
#define P_DB_EXTRA_MAX 48  // v14: a known map stays recognised with up to this many extra pearl tiles (mirrors counted); 0 = v13: any extra tile makes it a variant
#endif
#ifndef P_DB_EXTRA_A
#define P_DB_EXTRA_A 1  // v14: gaps assumed for an extra pearl tile: P_DB_EXTRA_A..max(P_DB_EXTRA_B, largest countdown seen)
#endif
#ifndef P_DB_EXTRA_B
#define P_DB_EXTRA_B 100
#endif
#ifndef P_DB_VARIANT
#define P_DB_VARIANT 1  // 1 = a known map with another pearl layout (ladder variants) keeps its walls, portals and strategy; pearls are learned
#endif
#ifndef P_POCKET_REVERSE
#define P_POCKET_REVERSE (P_TRAUMA_PLAN == 3)  // farmers leaving a pocket split their tail back in as the next farmer (Trauma plan 3)
#endif
#ifndef P_KING_FENCE
#define P_KING_FENCE 0  // v13: 0 (v10-v12: 1 with plan 3). 1 = the king of Stronghold/Trauma stays in its home field, near where couriers arrive; on the ladder it coiled there and was trapped or killed (2 of 2 v12 games), unfenced it grows in the maze (kings 43 against 35 locally)
#endif
#ifndef P_KING_ANCHOR_R
#define P_KING_ANCHOR_R 6  // ...within this distance of that tile
#endif
#ifndef P_KING_ANCHOR_PULL
#define P_KING_ANCHOR_PULL 2.0  // ...pulled back when farther
#endif
#ifndef P_ROLE_WAIT_DB
#define P_ROLE_WAIT_DB 1  // v7 fix: a child decides its role only once the map is recognised for sure (a tentative Stronghold made pocket children couriers)
#endif
#ifndef P_UNIT_FENCE
#define P_UNIT_FENCE 0  // (off) Trauma plan 3: farmers and couriers also stay in the home field and the pocket's region
#endif
#ifndef P_MAP_GOSSIP
#define P_MAP_GOSSIP 1  // 1 = tell allies (a newborn child) which known map this is; they adopt it if their view fits
#endif
#ifndef P_KING_REPLACE
#define P_KING_REPLACE 1  // Stronghold/Trauma roles: when no king has been heard of for P_KING_FRESH rounds, the longest courier becomes king
#endif
#ifndef P_PORTAL_TOP_BLOCK
#define P_PORTAL_TOP_BLOCK 1  // a body seen going into a portal blocks the portal's exit tile (its head part is there)
#endif
#ifndef P_ATTACK_LONG_MIN
#define P_ATTACK_LONG_MIN 0  // v8 guard build: 10 = dragons this long only trade head-on with an enemy at least as long, even when doomed (0 = off, as v7)
#endif
#ifndef P_PORTAL_RISK
#define P_PORTAL_RISK 0.0  // cost of a move through a portal whose exit tile is out of sight, x (length + 2)
#endif
#ifndef P_PORTAL_RISK_LEN
#define P_PORTAL_RISK_LEN 10  // v8: dragons at least this long pay the king's portal risk too (0 = only the king)
#endif
#ifndef P_PORTAL_RISK_KING
#define P_PORTAL_RISK_KING 0.5  // ...the same for the king (it has most to lose: a Trauma king died crossing into the maze)
#endif
#ifndef P_DB_SPAWNS
#define P_DB_SPAWNS 1  // 1 = trust the stored pearl layout of a known map while it fits what is seen; 0 = keep only walls, portals and plan, learn pearls
#endif
#ifndef P_ATTEMPT_MEMORY
#define P_ATTEMPT_MEMORY 1  // remember that a pearl spawn attempt happened on an unwatched tile even after its countdown restarts
#endif
#ifndef P_HOME_WALLS
#define P_HOME_WALLS 1  // our side of the map (path distance to both teams' starts) also on variants of known maps
#endif
#ifndef P_ATTEMPT_SIDE
#define P_ATTEMPT_SIDE 3  // ...an attempt learnt from a mirror or an ally counts only this many steps inside our side of the map
#endif
#ifndef P_ATTEMPT_OWN_CLASS
#define P_ATTEMPT_OWN_CLASS 1  // remembered attempts are calibrated on their own (they must not drag down the regular attempt class)
#endif
#ifndef P_ATTEMPT_MAXGAP
#define P_ATTEMPT_MAXGAP 40  // ...and only on tiles whose spawn gap is at most this
#endif
#ifndef P_LONG_STRIKE_LEN
#define P_LONG_STRIKE_LEN 0  // v8 guard build: 10 = dragons at least this long assume a shorter enemy in reach strikes with P_LONG_STRIKE (0 = off)
#endif
#ifndef P_LONG_STRIKE
#define P_LONG_STRIKE 0.5
#endif
#ifndef P_BIG_SPRINT_FULL
#define P_BIG_SPRINT_FULL 0  // v8 guard build: 1 = for the king and dragons of P_LONG_STRIKE_LEN+ a sprint strike is as likely as a one-step strike
#endif
#ifndef P_PORTAL_PROBE
#define P_PORTAL_PROBE 0  // v8: probe a blind portal with a lone sonar ray before crossing it (see main.cpp)
#endif
#ifndef P_PROBE_MAXLEN
#define P_PROBE_MAXLEN 3  // ...only when the ray's line behind the portal is at most this long (a small room)
#endif
#ifndef P_PROBE_PEN
#define P_PROBE_PEN 1.0  // ...cost of crossing into a room the probe found occupied, x (length + 2)
#endif
#ifndef P_BOX_ONEWAY
#define P_BOX_ONEWAY 0  // v8: known maps: 2x2 portal boxes are entered from one side only (exits land on the other)
#endif
#ifndef P_LANDING_PEN
#define P_LANDING_PEN 0.0  // v8: cost of ending a move on such an exit tile, x (length + 2)
#endif
#ifndef P_ATTEMPT_ENCLOSED
#define P_ATTEMPT_ENCLOSED 8  // v8: >0 = attempt memory only on tiles in rooms of at most this many tiles that only portals lead into (0 = everywhere, as v7.1: cost 4 games in 40 on random pearl layouts)
#endif
#ifndef P_DOOM_STRICT_LEN
#define P_DOOM_STRICT_LEN 8  // v9: dragons this long trade themselves for a shorter enemy only when truly doomed (0 = off, as v8)
#endif
#ifndef P_DOOM_SURV
#define P_DOOM_SURV 3  // ...truly doomed = every move leaves fewer than this many moves of room
#endif
#ifndef P_DEADEND_FLOOR
#define P_DEADEND_FLOOR 1000  // v9: least penalty for a move into a dead end (-1e9 = none, as v8: kings with 40+ moves of room scored a bonus)
#endif
#ifndef P_RESCUE_STRICT_LEN
#define P_RESCUE_STRICT_LEN 8  // v9: dragons this long rescue-split only when truly doomed (0 = off, as v8)
#endif
#ifndef P_RESCUE_SURV
#define P_RESCUE_SURV 6  // ...truly doomed = the best move leaves fewer than this many moves of room
#endif
#ifndef P_BIG_NOSPLIT_LEN
#define P_BIG_NOSPLIT_LEN 0  // v9: from kingSplitUntil on, dragons this long do not split for growth (0 = off, as v8)
#endif
#ifndef P_SMALL_STRIKER_LEN
#define P_SMALL_STRIKER_LEN 0  // v9 option: 6 = enemies up to this long within 2 steps are assumed to strike the king / dragons of P_SMALL_TARGET_LEN+ (0 = off, as v8; locally neutral, so left off)
#endif
#ifndef P_SMALL_TARGET_LEN
#define P_SMALL_TARGET_LEN 10  // ...long dragons = at least this long
#endif
#ifndef P_SMALL_STRIKE
#define P_SMALL_STRIKE 0.8  // ...with this chance
#endif
#ifndef P_SURV_FLOOR
#define P_SURV_FLOOR 50  // v9: nodes a survival search still gets after the turn budget is spent (v8: 300)
#endif
#ifndef P_BUDGET_64_BIG
#define P_BUDGET_64_BIG 1  // v9: 1 = a 64x64 map gets the 0.7 search budget of bigger maps (v8: 0, only above 64x64)
#endif
#ifndef P_SIDE_DETECT
#define P_SIDE_DETECT 1  // v10: take our side of a known map from where we start, not from the team letter (0 = v9)
#endif
#ifndef P_TAIL_GROW_KNOWN
#define P_TAIL_GROW_KNOWN 1  // v11: the same on the known maps (1: 124 of 160 against 116 with 3)
#endif
#ifndef P_TAIL_GROW
#define P_TAIL_GROW 0  // v11: 0 as in v10 on unknown maps (1: 77 of 144 on the unseen maps against 73, but 0 of 18 on the three lane maps; 3 with the lone hunt: 67). A dragon with a pearl next to its head may eat and keep its tail one turn longer: 1 = assume so everywhere, 2 = allies only, 3 = only in the survival search (1 lost every game on long lane maps: units stopped following each other)
#endif
#ifndef P_KING_PESS2
#define P_KING_PESS2 0.0  // v10: >0 = penalty for a king's (or long dragon's) move after which every way on runs through a shorter enemy's strike reach at the next move
#endif
#ifndef P_KING_PESS2_LEN
#define P_KING_PESS2_LEN 12  // ...dragons at least this long get the same check
#endif
#ifndef P_KING_PESS2_DEPTH
#define P_KING_PESS2_DEPTH 16  // ...survival depth of that check
#endif
#ifndef P_KING_SPACE2
#define P_KING_SPACE2 0.0  // v10: >0 = before the endgame pull, other dragons pay this per step inside P_KING_SPACE2_R of the king's head
#endif
#ifndef P_KING_SPACE2_R
#define P_KING_SPACE2_R 4
#endif
#ifndef P_KING_SPACE_ALLY
#define P_KING_SPACE_ALLY 0  // v10: 1 = before the endgame pull the king's room count leaves out tiles an ally reaches first (keeps it out of our own crowds)
#endif
#ifndef P_TAIL_SPLIT
#define P_TAIL_SPLIT 0  // v10: >0 = an ally at least this long keeps its tail one turn longer on swarm maps (it may split)
#endif
#ifndef P_MIRROR_FIX
#define P_MIRROR_FIX 1  // 1 = a countdown copied from the mirrored tile does not hide a pearl that is probably there (known maps, our side of the map); 2 = everywhere
#endif
#ifndef P_LONE_HUNT
#define P_LONE_HUNT 2  // v11: 1 = on known maps, 2 = everywhere: when at most P_LONE_MAX distinct enemy dragons were seen in the last P_LONE_WINDOW rounds and one is long, every short non-king dragon hunts it
#endif
#ifndef P_LONE_START
#define P_LONE_START 300  // ...from this round
#endif
#ifndef P_LONE_MAXLEN
#define P_LONE_MAXLEN 12  // ...hunters are at most this long
#endif
#ifndef P_LONE_MINUNITS
#define P_LONE_MINUNITS 3  // ...and we have at least this many units
#endif
#ifndef P_LONE_WINDOW
#define P_LONE_WINDOW 40  // ...rounds of sightings (own and gossip) counted
#endif
#ifndef P_LONE_MAX
#define P_LONE_MAX 2  // ...at most this many distinct enemies seen in the window
#endif
#ifndef P_LONE_MINLEN
#define P_LONE_MINLEN 10  // ...the target is at least this long (and 3 longer than the hunter)
#endif
#ifndef P_LONE_PULL
#define P_LONE_PULL 3.0  // ...long-range pull toward its last known head
#endif
#ifndef P_ZONE_POCKETS
#define P_ZONE_POCKETS 1  // v11: zones may lie in dead-end pockets on maps that do not farm pockets (pocketMaxLen 0)
#endif
#ifndef P_OPEN_RUSH
#define P_OPEN_RUSH 2.0  // v11: known maps: pull of the opening rush (the original nearest to the best zone heads there from round 0)
#endif
#ifndef P_OPEN_RUSH_MAPS
#define P_OPEN_RUSH_MAPS "Trophy"  // known maps with the opening rush (v11: Trophy; v13 also Queen Of Spades, into a fast box; v14: Trophy only, on the ladder's Queen layout the rush won 58 of 124 games against 68 without)
#endif
#ifndef P_OPEN_RUSH_N
#define P_OPEN_RUSH_N 1  // ...how many of our original dragons rush
#endif
#ifndef P_OPEN_RUSH_SPREAD
#define P_OPEN_RUSH_SPREAD 0  // v13: 1 = the next original heads for the best other zone (on the rush maps)
#endif
#ifndef P_OPEN_RUSH_UNTIL
#define P_OPEN_RUSH_UNTIL 60  // ...until this round
#endif
#ifndef P_OPEN_RUSH_ARRIVE
#define P_OPEN_RUSH_ARRIVE 2  // ...stop pulling within this distance of the zone centre
#endif
#ifndef P_PREV_FIX
#define P_PREV_FIX 1  // v11: the mirror fix counts only an attempt that fell between my last look (or round 0) and now, and allows for S being the tile's first attempt (v10: 0)
#endif
#ifndef P_MIRROR_UNSEEN_P
#define P_MIRROR_UNSEEN_P 0.0  // (unknown maps, off) pearl probability of a spawn tile known only from its mirror
#endif

// ---- v12: fights against a team that hunts (0-10 against the top team with v11)
#ifndef P_SKIRMISH
#define P_SKIRMISH 0.0  // v12: pull of a short dragon toward the tiles next to a visible enemy head that is longer than it (a head-on there gains length); 0 = off
#endif
#ifndef P_SKIRMISH_MAXLEN
#define P_SKIRMISH_MAXLEN 6  // ...only dragons up to this long
#endif
#ifndef P_SKIRMISH_EQ
#define P_SKIRMISH_EQ 0  // ...1 = also an enemy of equal length when our heads near it outnumber theirs by P_SKIRMISH_EQ_MARGIN
#endif
#ifndef P_SKIRMISH_EQ_MARGIN
#define P_SKIRMISH_EQ_MARGIN 1
#endif
#ifndef P_ATTACK_LOCAL
#define P_ATTACK_LOCAL 1  // v12: an even trade (equal lengths, not doomed) only when, without the two, our heads within P_ATTACK_LOCAL_R of the spot are at least theirs + P_ATTACK_LOCAL_MARGIN (the survivors eat the drops); 2 = also a trade that gains length unless the enemy is 3x my length; 0 = off (v11)
#endif
#ifndef P_ATTACK_LOCAL_MARGIN
#define P_ATTACK_LOCAL_MARGIN 0
#endif
#ifndef P_ATTACK_LOCAL_R
#define P_ATTACK_LOCAL_R 4  // ...radius (manhattan from the enemy head)
#endif
#ifndef P_ATTACK_LOCAL_WHERE
#define P_ATTACK_LOCAL_WHERE 3  // where P_ATTACK_LOCAL, P_OUTNUM_P and P_REINFORCE apply: 1 = known maps, 2 = every map, 3 = the known elimination maps (Autarky, Default, Devil, Dilemma, Queen, Trophy; on Slithery Fight they cost 5 of 16)
#endif
#ifndef P_OUTNUM_P
#define P_OUTNUM_P 0.3  // v12: assumed strike chance of an enemy that would not lose length by the trade and whose heads within P_OUTNUM_R outnumber ours there (me included); halved for sprints; 0 = off (v11)
#endif
#ifndef P_OUTNUM_R
#define P_OUTNUM_R 4  // ...radius for those counts
#endif
#ifndef P_PORTALS_BLIND
#define P_PORTALS_BLIND -1.0  // v12: P_PORTAL_BLIND on the Portals map (-1 = the general value); 71 pairs of our dragons met head-on through portals there in one local game
#endif
#ifndef P_PORTALS_EXIT_STAY
#define P_PORTALS_EXIT_STAY -1.0  // v12: P_PORTAL_EXIT_STAY on the Portals map (-1 = the general value)
#endif
#ifndef P_REINFORCE
#define P_REINFORCE 0.0  // v12: pull of a short dragon toward a visible ally head that is within 2 of an enemy head (a trade there drops pearls; the side with more heads near eats them); 0 = off
#endif
#ifndef P_KMEET
#define P_KMEET 0  // v12: from this round (0 = off), on the known maps that merge at the end, every king and every dragon of P_KMEET_MINLEN+ walks to our richest start tile; kings far apart never hear of each other (3 "kings" of 23-25 in a local Slithery game at round 440), there they see each other and the shorter feeds the longer
#endif
#ifndef P_KMEET_MINLEN
#define P_KMEET_MINLEN 12
#endif
#ifndef P_KMEET_PULL
#define P_KMEET_PULL 3.0  // ...pull strength (as P_RV_PULL)
#endif
#ifndef P_KMEET_ZONE
#define P_KMEET_ZONE 4  // ...stop pulling within this distance of the tile
#endif
#ifndef P_ATTACK_UNFAV
#define P_ATTACK_UNFAV 0  // v14 (off): on the P_ATTACK_LOCAL maps, strike an enemy up to this much shorter when our other heads near it outnumber theirs by P_ATTACK_UNFAV_MARGIN. 1 / 3: 52 / 51 of 72 against 53
#endif
#ifndef P_ATTACK_UNFAV_MARGIN
#define P_ATTACK_UNFAV_MARGIN 1
#endif
#ifndef P_KING_GUARD
#define P_KING_GUARD 0  // v14 (off; never triggered locally): 1 = known maps, 2 = all maps: a short dragon (up to P_KING_GUARD_MAXLEN) strikes an enemy head within P_KING_GUARD_R of our king, shorter than it, even at a loss, and is pulled toward it
#endif
#ifndef P_KING_GUARD_R
#define P_KING_GUARD_R 6
#endif
#ifndef P_KING_GUARD_MAXLEN
#define P_KING_GUARD_MAXLEN 8
#endif
#ifndef P_KING_GUARD_PULL
#define P_KING_GUARD_PULL 2.0
#endif
#ifndef P_KING_GUARD_MINKING
#define P_KING_GUARD_MINKING 12  // ...only for a king at least this long
#endif
#ifndef P_SKIRMISH_WHERE
#define P_SKIRMISH_WHERE 1  // 1 = known maps only, 2 = every map
#endif

#ifndef P_E7_DEFER
#define P_E7_DEFER 1  // 1 = from the endgame pull on, all maps leave pearls near the king to the king
#endif
#ifndef P_E7_DEFER_F
#define P_E7_DEFER_F 0.03  // ...valued at this factor by the other dragons
#endif
#ifndef P_E7_DUMP
#define P_E7_DUMP 445  // >0: from this round feeding goes on down to 2 units (v5/v6 stopped below 6)
#endif
#ifndef P_E7_DUMP_MARGIN
#define P_E7_DUMP_MARGIN 0  // ...and then feeds any dragon at least this much longer (0: equal lengths too, the lower id is king)
#endif
#ifndef P_E7_BIG_LEN
#define P_E7_BIG_LEN 12  // >0: once an enemy dragon this long has been seen, merge earlier:
#endif
#ifndef P_E7_BIG_FEED
#define P_E7_BIG_FEED 400  // ...feeding from this round at the latest
#endif
#ifndef P_E7_BIG_PULL
#define P_E7_BIG_PULL 380  // ...pull toward the king from this round at the latest
#endif
#ifndef P_ELIM_FEED
#define P_ELIM_FEED 400  // feedStart of the elimination-prone maps (Default, Devil, Queen, Trophy, Dilemma, Autarky)
#endif
#ifndef P_ELIM_PULL
#define P_ELIM_PULL 380  // ...and their feedPullStart
#endif

#ifndef P_DEVIL_ZONE_PULL
#define P_DEVIL_ZONE_PULL 0.0  // Devil: early rush of a share of the units to the centre farms (the top teams take it in the first 20 rounds)
#endif
#ifndef P_DEVIL_ZONE_FRAC
#define P_DEVIL_ZONE_FRAC 0.6
#endif
#ifndef P_DEVIL_ZONE_UNTIL
#define P_DEVIL_ZONE_UNTIL 100
#endif

#ifndef P_SCH_DUMP
#define P_SCH_DUMP 0  // v14 (off): Schooltime / Slithery Fight / Default: from this round every dragon feeds a longer one (0 = P_E7_DUMP)
#endif
#ifndef P_SLI_DUMP
#define P_SLI_DUMP 0
#endif
#ifndef P_DEFAULT_DUMP
#define P_DEFAULT_DUMP 0
#endif
#ifndef P_DEFAULT_GROW
#define P_DEFAULT_GROW 450  // v14: Default growRound / feedStart / feedPullStart (as v13: 450 / P_ELIM_FEED / P_ELIM_PULL); 330/360/340 + P_DEFAULT_DUMP 400: 22 of 32 against 26
#endif
#ifndef P_DEFAULT_FEED
#define P_DEFAULT_FEED P_ELIM_FEED
#endif
#ifndef P_DEFAULT_PULL
#define P_DEFAULT_PULL P_ELIM_PULL
#endif
#ifndef P_AUTARKY_POCKET
#define P_AUTARKY_POCKET 12  // v14: Autarky / Dilemma: farm dead-end pockets with units up to this long (as v13: 12; 0 = never: 25 of 48 against 37)
#endif
#ifndef P_DILEMMA_POCKET
#define P_DILEMMA_POCKET 12
#endif
#ifndef P_SCH_GROW
#define P_SCH_GROW 400  // v12: Schooltime growRound / feedStart / feedPullStart (v11: 400 / 400 / 380)
#endif
#ifndef P_SCH_FEED
#define P_SCH_FEED 400
#endif
#ifndef P_SCH_PULL
#define P_SCH_PULL 380
#endif
#ifndef P_SLI_GROW
#define P_SLI_GROW 400  // v12: Slithery Fight growRound / feedStart / feedPullStart (v11: 400 / 400 / 380)
#endif
#ifndef P_SLI_FEED
#define P_SLI_FEED 400
#endif
#ifndef P_SLI_PULL
#define P_SLI_PULL 380
#endif

#ifndef P_MAP_TABLE
// columns: name, splitMin, maxUnits, kingSplitUntil, growRound, feedStart, feedPullStart, hunt, kingDefer,
//          deadEndCost, unseenEarly, pocketMaxLen [, zonePull, zoneFrac, zoneUntil, rv]  (missing = 0)
#define P_MAP_TABLE \
    /* elimination-prone maps: pure swarm, consolidate only at the very end */ \
    {"Default",           P_ELIM_SPLIT, 64, 9999, P_DEFAULT_GROW, P_DEFAULT_FEED, P_DEFAULT_PULL, 1.0, 1.00, 5.0, P_DEFAULT_UNSEEN,  0, P_DEFAULT_ZONE_PULL, P_DEFAULT_ZONE_FRAC, P_DEFAULT_ZONE_UNTIL, 0}, \
    {"Default Small",     4, 64, 9999, 450, P_ELIM_FEED, P_ELIM_PULL, 1.0, 1.00, 5.0, P_DSMALL_UNSEEN,  0}, \
    {"Devil",             P_ELIM_SPLIT, 64, 9999, 450, P_ELIM_FEED, P_ELIM_PULL, 1.0, 1.00, 5.0, P_DEVIL_UNSEEN,  0, P_DEVIL_ZONE_PULL, P_DEVIL_ZONE_FRAC, P_DEVIL_ZONE_UNTIL, 0}, \
    {"Queen Of Spades",   P_ELIM_SPLIT, 64, 9999, 450, P_ELIM_FEED, P_ELIM_PULL, 1.0, 1.00, 5.0, P_QUEEN_UNSEEN,  0, P_QUEEN_ZONE_PULL, P_QUEEN_ZONE_FRAC, P_QUEEN_ZONE_UNTIL, 0}, \
    {"Trophy",            P_ELIM_SPLIT, 64, 9999, 450, P_ELIM_FEED, P_ELIM_PULL, 1.0, 1.00, 5.0, P_TROPHY_UNSEEN, 0, P_TROPHY_ZONE_PULL, P_TROPHY_ZONE_FRAC, P_TROPHY_ZONE_UNTIL, 0}, \
    {"Prisoners Dilemma", P_ELIM_SPLIT, 64, 9999, 450, P_ELIM_FEED, P_ELIM_PULL, 1.0, 1.00, 5.0, P_DILEMMA_UNSEEN, P_DILEMMA_POCKET}, \
    {"Autarky",           P_ELIM_SPLIT, 64, 9999, 450, P_ELIM_FEED, P_ELIM_PULL, 1.0, 1.00, 5.0, P_AUTARKY_UNSEEN, P_AUTARKY_POCKET}, \
    /* round-500 maps: swarm + king from round 100 (the defaults), with map-specific extras */ \
    {"Schooltime",        4, 9999, 100, P_SCH_GROW, P_SCH_FEED, P_SCH_PULL, 1.0, 0.03, 5.0, 60,  0, P_SCH_ZONE_PULL, P_SCH_ZONE_FRAC, P_SCH_ZONE_UNTIL, 0}, \
    {"Portals",           4, 9999, 100, P_PORTALS_GROW, P_PORTALS_FEED, P_PORTALS_PULL, 1.0, 0.03, 5.0, 60, 0, 0.0, 0.0, 0, P_PORTALS_RV}, \
    {"Slithery Fight",    4, 9999, 100, P_SLI_GROW, P_SLI_FEED, P_SLI_PULL, 1.0, 0.03, 5.0, 60,  0}, \
    /* separated fields: no splitting, merge into one king early */ \
    {"Stronghold",     9999, 64,    0,   0,  60,  50, 0.0, 0.15, 5.0, 60,  0}, \
    P_TRAUMA_ROW
    /* Big Empty and unknown maps use the defaults: swarm + king from round 100 */
#endif
