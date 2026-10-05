# Rank-1 bot: what the replays show

Source: 34 tournament replays (`battle-M1089202` and `battle-M1089444`, 17 maps each).
The rank-1 bot plays team B in every game and wins 31 of 34. In the M10894xx batch, team A is
the previous submission (`baseline/v105`, which sends four sonars every turn).

Replays are packed Cap'n Proto (`tools/replay.py` decodes them; `tools/sim.py` rebuilds
bodies per event; `tools/geom.py` models kelp, portals and wraparound and matches every recorded
move in all 34 games).

## Rules confirmed from replays and local experiments

* Round 500 tiebreak order: **queen length**, then longest dragon, then total length. Queens are
  dragons 0 and 1 (one per team); a dead queen counts as 0.
* Any death drops pearls on the dead dragon's segments 0, 2, 4, ... (head first), whatever the cause.
* Sending no action at all is "suicide" (death reason "no valid action"), and leaves the same pearls.
* Moving into your own tail is fatal: the tail does not move out of the way first. Every body cell
  is blocked at the moment of a move.
* Pearl beds in the tournament maps are the bundled ones (`TILE x y min max` = respawn gap range);
  the replay map text hides them, but every recorded spawn is on a bundled bed.
* Tournament maps are the bundled maps with sides/team order sometimes swapped.

## How the rank-1 bot plays

| | rank-1 (B) | v105 (A, M10894xx) |
|---|---|---|
| queen alive at round 500 | 11 / 17 | 1 / 17 |
| deaths by kelp / own body / other body | ~0 | hundreds per game |
| deaths by "no action" (suicide) | ~240 per game | 0 |

* **Split at length 4 into 2 + 2**, every time it can (1,751 of the first-100-round splits are (4,2)).
  The colony is a swarm of length 2-3 dragons that grows to the unit limit (62).
* **Never makes a fatal move.** When trapped (no free neighbour) it suicides (3,129 cases), or first
  splits off all but 2 segments so the tail part can reverse out (`(L, L-2)` splits).
* **Feeding.** From round ~200, a short dragon (length 2-3) whose head is 2-3 cells from a clearly
  longer ally (usually the queen, else a long "grower") suicides on purpose; the long dragon eats
  the drops. Of ~1,370 deliberate (not trapped) suicides, 466 feed the queen and ~900 feed another
  long dragon. On Maze the queen ate 153 suicide drops and grew from 7 to 118.
* **Queen.** Splits down to 2-3 in the first ~100 rounds like everyone else, stays near its start
  area, stays small and alive until ~round 250, then grows to 40-120.
* **Food.** Eats 3-10x more than v105 on Trauma/Devil: the swarm sits on the fast-respawn beds,
  and about half of everything eaten is pearls dropped by dead dragons.
* Sprints are used, mostly within the free ceil(L/4) steps; almost never pays segments.
* Sonar: four 64-bit messages per turn from ~round 100 (format not decoded; this bot uses its own).

## This bot (`bot/`)

A clean reimplementation of the above (`bot/main.py`); `tools/gen_maps.py` builds the map data
modules `bot/md_*.py` from the bundled maps. `baseline/v105` is the previous submission, kept for
local comparison: `python3 tools/match.py bot baseline/v105 --both`.

## Second batch (M1100428, M1100962, M1101160, M1101179: 53 games)

The rank-1 bot is again team B (the only side that suicides; it never crashes) and wins 45 of 53.
The M1100428 opponent (v105-like: four sonars per turn, hundreds of crashes) beats it on 5 maps:
twice on queen length after B's queen died (Around UNSW, Maze), twice on longest dragon with both
queens dead (Australia, Portals) and once by elimination (Stripes).

* **Queen:** stays at length 2-4 until round 200-250 in every game, then grows to 25-110. It is not
  very defensive: with an enemy head 2-5 cells away it moves closer about as often as away, but
  with an enemy head adjacent it steps away 181 times in 198. It still died in 23 of 53 games
  (14 struck by an enemy dragon, 8 trapped).
* **Queen strikes:** B killed the opponent's queen 24 times, almost always with a length 2-3
  dragon taking 1-2 steps onto the queen's head (a few paid sprints of 3-4 steps).
* Feeding and swarm statistics match the first batch.

## Local results for `bot/` against `baseline/v105`

17 tournament maps x seeds 1-2 x both sides (68 games): 23-24 wins. Biggest losses: our queen
dies in ~54 of 68 games (mostly v105's paid sprint strikes, which reach about length+2 cells),
and the longest-dragon tiebreak. Metered sandbox on Around UNSW: p99 54M, max 64M points per turn
(limit 100M).
