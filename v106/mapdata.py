# Official 1.2.7 maps; modules are loaded only for matching board sizes.
SIZES = {'arena': (11, 11), 'australia': (64, 64), 'autarky': (54, 18), 'big_empty': (64, 64), 'Colosseum': (16, 16), 'default': (32, 32), 'default_small': (16, 16), 'devil': (32, 16), 'dilemma': (32, 16), 'islands': (56, 40), 'maze': (48, 24), 'portals': (32, 16), 'queen_of_spades': (25, 35), 'schooltime': (60, 40), 'schooltime_sparse': (60, 40), 'slithery_fight': (63, 27), 'stripes': (24, 12), 'stronghold': (48, 24), 'tower_defense': (32, 16), 'trauma': (48, 24), 'trophy': (25, 25), 'unsw': (64, 64), 'weakhold': (40, 15)}

def load(key):
    return __import__('md_' + key).M
