# Leviathan v17: UNSW Battlecode "Deep-Sea Dragons"

C++20: `main.cpp` (the bot), `params.hpp` (every tunable constant and the per-map table), `maps.hpp`
(embedded copies of the known maps, unchanged). Submit the `leviathan` folder as before; `bot.toml` is
unchanged. Use the three files together: `main.cpp` refuses a `params.hpp` from another release
(`LEVIATHAN_PARAMS_VERSION` is now 17).

**On the ten ladder maps v17 plays exactly as v16** (the same games, move for move). Everything new is for
maps the bot does not know, which is what the tournament will use.

## 1. What your 110 games say (v39 = v16)

v16 won 40 of 110.

| Opponent | Won |
|---|---|
| cheji bt (3 battles) | 11 of 30 |
| Stockfish | 14 of 30 |
| forgot to mention | 10 of 30 |
| Sponge (Albert and Bob) (M711546) | 4 of 10 |
| free trip to sydney pls (M711581) | 1 of 10 |

| Map | Won | | Map | Won |
|---|---|---|---|---|
| Portals | 7 of 11 | | Devil | 4 of 11 |
| Prisoners Dilemma | 7 of 11 | | Slithery Fight | 3 of 11 |
| Queen Of Spades | 6 of 11 | | Autarky | 1 of 11 |
| Default | 5 of 11 | | Schooltime | 1 of 11 |
| Trophy | 5 of 11 | | Trauma | 0 of 11 |

**49 of the 70 losses were eliminations.** Why:

* **They eat more; we do not lose more.** Both teams lose about the same length to deaths (the other team
  loses more on Autarky and Trauma). But from round 50 they eat 2–4 times the spawned pearls we eat:

  | Map | Rounds 50–100 | Rounds 100–150 | Rounds 150–200 |
  |---|---|---|---|
  | Autarky | 21 vs 41 | 13 vs 39 | 14 vs 45 |
  | Trauma | 22 vs 40 | 24 vs 63 | 24 vs 92 |
  | Schooltime | 65 vs 48 | 50 vs 71 | 47 vs 96 |
  | Default | 21 vs 26 | 19 vs 36 | 21 vs 30 |

  (spawned pearls eaten per 50 rounds, us vs them, average of the 11 games. On Autarky, Devil and Slithery
  they also eat about twice the pearls that dead dragons drop.)
* **More units, spread wider.** Over rounds 20–200 on Autarky they had 24 units to our 11 and held 13 of
  24 map cells to our 6. On Trauma 30 to 5.
* **Portals.** On Default the other teams crossed portals 14–70 times in the first 100 rounds (median 40:
  they farm the rooms in the middle); we crossed 4–33 times (median 11).
* **Our endgame merge wastes length.** Slithery against forgot to mention (M711332): both longest dragons
  84, we lost on total length, 107 to 201. Our merge took the total from 205 to 132 between rounds 400 and
  425 while the king grew from 51 to 62. In a local game the donations dropped 76 pearls; our own dragons
  ate 63 of them, mostly not the king, and carried them again (half is lost each time).

## 2. The tournament uses unseen maps: how good is the bot there?

On a map it does not know the bot plays "blind": no map knowledge, no per-map plan. To measure that on
the real ladder, I uploaded a build with map recognition switched off and played unranked battles.

| Blind build, ladder maps | Won |
|---|---|
| v16 blind vs cheji bt, Stockfish, Cache me outside, forgot to mention, calc | 7 of 50 |
| v16 blind vs bread first search, Sponge, Peanut Butter | 6 of 30 |
| v16 **with** its map knowledge (your replays) vs cheji bt, Stockfish, forgot to mention | 35 of 90 |

(SSS lost 9 of 10 to the blind build, eliminated in 8 of them; its bot seems broken at the moment, so it is
not counted.)

**So the bot's ladder rating rests on its knowledge of the ten maps.** Blind, it beat Peanut Butter 2 times
in 10 (v16 with the map: 12 of 15). Either the other teams know these maps too (the tournament then takes
that away from them as well), or their generic play is much better than ours. I could not tell which: the
replay files are on a storage host the session is not allowed to reach, so I could not watch the blind
games.

## 3. What changed in v17 (unknown maps only)

| Change | Parameters | Why |
|---|---|---|
| **Stay at rich spots.** A small dragon (up to 12 long, 60% of them by id) whose head has fast-respawning tiles around it (those it or an ally has seen give 0.15+ pearls a round within 3 steps) pays 4 for a move to a spot with less than half that | `P_ZONE_ONLINE` 1, `P_OHOLD_STAY` 4, `P_OHOLD_RATE` 0.15, `P_OHOLD_FRAC` 0.6, `P_OHOLD_MAXLEN` 12 | the unknown-map version of the Devil hold; the other teams' heads stay where the pearls are, ours wandered off |
| **Small maps play like the known elimination maps.** Under 2,000 tiles: no king until the end, the fight rules the known elimination maps use (an even trade only where our heads outnumber theirs; an enemy that outnumbers us near a spot is assumed to strike), and the lone-king hunt from round 300 | `P_ADAPT_CFG` 1, `P_LOCAL_UNKNOWN` 1, `P_LONE_HUNT` 2 | on the known maps those rules and that plan were tuned against the ladder; blind, the bot used none of them |
| **A king when the other team builds one.** On those small maps, once an enemy 12+ long has been seen, our longest stops splitting (from round 150) | `P_DYN_KING` 1, `P_DYN_KING_LEN` 12, `P_DYN_KING_FROM` 150 | without it the blind build lost round-500 games on the longest dragon with more total length than the other team: 165 to 95, 193 to 145, 63 to 29 |
| **An unexplored portal is a way out.** The survival search counts a portal whose other end nobody has seen as an exit | `P_SURV_UNKPORTAL` 1 | rooms behind portals counted as dead ends |

On the ten ladder maps none of this applies (the map is recognised on the first turn): v17 and v16 played
the same games move for move on Autarky, Devil, Portals, Schooltime and Trauma (seed 7, against v13).

New options, all off: `P_SPREAD_UNKNOWN` (spread penalty on unknown maps; 2 = big ones only),
`P_EARLY_SPREAD` (spread in the opening), `P_CENTER_RUSH` (+ `_FRAC`, `_UNTIL`, `_ARRIVE`, `_MAXLEN`: walk
to the map centre in the opening), `P_EVADE_SPRINT` (+ `_MINLEN`, `_KNOWN`: long dragons weigh 2–3 step
sprints away from a shorter enemy that can strike), `P_SMALL_KING_UNTIL`, `P_UNK_FEED`/`_PULL`/`_GROW`,
`P_UNK_KING_UNTIL` (earlier king and endgame on unknown maps), `P_SIM_INHERIT` (simulator-only
experiment: newborns start with their parent's knowledge).

## 4. Tests

**Blind against blind** (both sides of 24 unseen maps, against v16 blind):

| Build | Won |
|---|---|
| v17 | 36 of 48 |
| without the rich-spot hold | 28 of 48 |
| small-map plan and fight rules only | 28 of 48 |
| + spread in the opening (`P_EARLY_SPREAD` 40) | 26 of 48 |
| + tail growth assumption (`P_TAIL_GROW` 1) | 25 of 48 |
| hold threshold 0.1 instead of 0.15 | 29 of 48 |
| no small-map plan (king from round 100 everywhere) | 29 of 48 |

Part of this lead is v16's own weakness: several wins are eliminations of v16 blind after it merged
into one king at round 400.

**Blind against v16 with its map knowledge** (the ten ladder maps, both sides, seeds 2 and 3):
v16 blind won 7 of 20 on seed 2; v17 blind won 16 of 40 (9 of 20 on the five elimination maps, 7 of 20 on the
five round-500 maps; without the dynamic king 14, with 5 on the round-500 maps). Tried and dropped (seed 2, 20 games each, v16 blind 7): spread penalty on unknown maps 3,
unknown tiles worth 0.1 instead of 0.03 (more exploring) 3, centre rush 4, centre rush + pull to rich zones
3.5, pull to rich zones 2, portal attraction 5, mirror tiles 6.

**On the real ladder** (blind builds, unranked): the top five beat every blind version I tried: v16 blind 7
of 50, small-map plan and fight rules 9 of 50, + hold and portals 5 of 50. Against bread first search,
Sponge and Peanut Butter: v16 blind 6 of 30, an earlier v17 candidate (hold, portals, small-map plan,
merge from round 400) 5 of 30. These tests can
only show a large change; none of the versions was measurably better or worse there.

**Simulator-only check:** giving every newborn its parent's whole map knowledge (impossible on the real
engine, where each dragon is its own process) did not help (6 of 20 against 7), so knowledge sharing is
not what the blind bot lacks.

## 5. CPU

On the judge's own engine (sandbox, 100M points a turn allowed): Frontier, 64x64 (the biggest legal map),
blind v17 against v16: median 27.2M points a turn, at most 39.8M. Schooltime with the map (as v16):
median 14.5M, at most 36.4M.

## 6. Ladder rating

The ranked games the server schedules use the active version, so while the blind test builds were active
the team lost about 80 points (1,937 to 1,854; they lost 5–0 to teams rated 1,700–1,760 on these maps).
v39 was active again from 16:40 UTC, and v17 is active now; it plays the ten ladder maps exactly as v39.

## 7. Not solved

* **Blind play against the ladder teams on these maps.** Even mid-table teams beat the blind builds 4 of 5
  times. If they know these maps, the tournament will even that out; if not, our generic play is much
  weaker than theirs, and none of the ideas above closed the gap.
* The known-map issues from v16 (economy on the elimination maps, Schooltime, Trauma, the Trophy rusher)
  are unchanged: I spent this round on unseen-map play instead, since that is what the tournament uses.
* Our endgame merge: the king-drop penalty (`P_KING_DROP_PEN` 3: others do not eat next to the king) was
  neutral (7 of 12 against 7 of 12 against v15 on the round-500 maps).
