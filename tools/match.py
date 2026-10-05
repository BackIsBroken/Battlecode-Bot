"""Run many local matches in parallel and summarise the results.

usage: python3 tools/match.py BOT_A BOT_B [--maps m1,m2] [--seeds 1,2] [-j 4] [--both]
--both plays every pairing from both sides (A/B swapped).
"""
import argparse, os, re, subprocess, sys, time
from concurrent.futures import ThreadPoolExecutor

MAPDIR = '/usr/local/lib/python3.11/dist-packages/unswbc/templates/maps'
TOURNAMENT = ['unsw', 'australia', 'autarky', 'default', 'devil', 'islands', 'maze', 'portals',
              'dilemma', 'queen_of_spades', 'schooltime', 'slithery_fight', 'stripes',
              'tower_defense', 'trauma', 'trophy', 'weakhold']
RES = re.compile(r'team (\w) wins after (\d+) rounds \((.*)\)|draw after (\d+) rounds \((.*)\)')

def play(a, b, m, seed, replay_dir):
    cmd = ['unswbc', 'run', os.path.join(MAPDIR, m + '.map'), a, b, '--seed', str(seed)]
    if replay_dir:
        cmd += ['-o', os.path.join(replay_dir, '%s-%s-%s-%d.replay' % (os.path.basename(a), os.path.basename(b), m, seed))]
    else:
        cmd += ['--no-replay']
    t = time.time()
    p = subprocess.run(cmd, capture_output=True, text=True, timeout=3600)
    out = p.stdout + p.stderr
    mt = None
    for line in out.splitlines():
        r = RES.search(line)
        if r: mt = r
    errs = [l for l in out.splitlines() if 'ran out of time' in l or 'Traceback' in l or 'Error' in l or 'exceeded' in l]
    if mt is None:
        return (m, seed, a, b, None, 'error', time.time() - t, out[-500:])
    if mt.group(1):
        w = mt.group(1); reason = mt.group(3)
    else:
        w = None; reason = mt.group(5)
    return (m, seed, a, b, w, reason, time.time() - t, errs[:3])

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('a'); ap.add_argument('b')
    ap.add_argument('--maps', default=','.join(TOURNAMENT))
    ap.add_argument('--seeds', default='1')
    ap.add_argument('-j', type=int, default=4)
    ap.add_argument('--both', action='store_true')
    ap.add_argument('--replays', default='')
    args = ap.parse_args()
    if args.replays: os.makedirs(args.replays, exist_ok=True)
    jobs = []
    for s in args.seeds.split(','):
        for m in args.maps.split(','):
            jobs.append((args.a, args.b, m, int(s)))
            if args.both: jobs.append((args.b, args.a, m, int(s)))
    score = {args.a: 0.0, args.b: 0.0}
    rows = []
    with ThreadPoolExecutor(args.j) as ex:
        futs = [ex.submit(play, *j, args.replays) for j in jobs]
        for f in futs:
            m, seed, a, b, w, reason, dt, errs = f.result()
            if w == 'A': score[a] += 1; win = a
            elif w == 'B': score[b] += 1; win = b
            else:
                win = 'draw' if reason != 'error' else 'ERROR'
                if reason != 'error': score[a] += .5; score[b] += .5
            rows.append((m, seed, a, b, win, reason))
            print('%-16s s%-3d %s vs %s -> %s (%s) %.0fs %s' % (m, seed, os.path.basename(a), os.path.basename(b), os.path.basename(win), reason, dt, errs if errs else ''), flush=True)
    print('TOTAL', {os.path.basename(k): v for k, v in score.items()}, 'of', len(jobs))

if __name__ == '__main__':
    main()
