"""Replay-derived phases; heuristics reconstruct behaviour, not private source.

Evidence: 109 supplied matches, including M1027691-M1029527. These are
phase heuristics, not exact recovered decisions. Some large-map colonies
reach 62; selected growers and late concentration occur in many winners.
Offline replay data is never loaded by this bot.
"""

# grow round, required population, merge round. Known map keys are the ones
# already identified from observed tiles by the supplied bot's map matcher.
PHASES = {
    'islands': (200, 24, 350),
    'australia': (200, 24, 350),
    'unsw': (190, 24, 350),
    'schooltime': (200, 24, 350),
    'portals': (230, 12, 350),
    'maze': (230, 12, 350),
    'slithery_fight': (230, 16, 350),
    'trauma': (210, 16, 350),
    'weakhold': (220, 8, 350),
    'tower_defense': (220, 12, 350),
}

def profile(key, area):
    grow, population, merge = PHASES.get(key, (200, 16, 370))
    queen_support = key in ('maze', 'trauma', 'portals', 'slithery_fight', 'default')
    return dict(cap=min(62, max(8, area // 8)), grow_from=grow,
                grow_population=population, stop_split=350,
                merge_from=merge, final_from=max(410, merge+40),
                wide_feed_from=440, pull_from=max(280, merge-40),
                queen_feed_from=220 if queen_support else 350,
                queen_pull_from=210 if queen_support else max(280, merge-40),
                sonar_from=180,
                fast_split=key is not None)

def reserve(key, rnd, length, population, dragon_id):
    grow, minimum, _ = PHASES.get(key, (200, 16, 370))
    if rnd >= 180 and length >= 8:
        return True
    return (rnd >= grow and population >= minimum
            and (dragon_id // 2) % 8 == 1)
