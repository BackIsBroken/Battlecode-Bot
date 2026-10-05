# clone: behavioural clone of the rank-1 bot

Two LightGBM policies trained on ~1.2M decisions the rank-1 bot made in 104 replays
(tools/clone_extract.py, clone_train.py, clone_export.py), evaluated in pure Python:

* `clonemodel.py` KIND chooses move / split / suicide (99% agreement on held-out games), DIR ranks
  the four directions (72%; the rank-1 bot's moves without a visible pearl are close to random).
* `clonequeen.py` a direction model trained only on its queen's decisions (77%).
* `clonefeat.py` the shared feature code (the same code labelled the replays).

Hard rules on top: never an illegal step; the queen avoids pockets and enemy heads (space and
distance tiers), splits only with nobody within 3 cells, sheds 2 tail segments when trapped; other
dragons never step next to their queen; from round 200 the queen's position is relayed over sonar,
short dragons drift toward her and suicide 3 cells away (4 from round 420) so she eats the drops.

Local: 28.5-31 wins of 68 against v105 (17 maps x 2 seeds x both sides). Wins small maps by
elimination like the rank-1 bot (Devil, Autarky, Trophy, Dilemma); loses most big maps.
Metered sandbox (UNSW, Slithery Fight): p50 ~30M, p99 ~65M, max ~78M points per turn (limit 100M); a budget guard reads the sandbox clock. Models: 120-tree kind, 60-tree direction, trained on 368 rank-1 games.
