# v106

v105 plus three behaviours taken from the rank-1 bot's replays (see ../ANALYSIS.md):

* collectors keep at least 2 cells away from their own queen unless donating (`main.py`, "(v106) the rank-1 bot's queen survives")
* Devil: two dragons in three hold the centre instead of one in three (`MID_ROLES`)
* length 2-3 dragons step onto an adjacent enemy head when it is longer or is the enemy queen (`ATTACK_SHORT`)

Local result against v105: 34-34 over 68 games (17 maps x 2 seeds x both sides). Not yet tested against the rank-1 bot.
Submit the same way as v105; bot.toml and helper.py are unchanged.
