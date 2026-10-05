"""Train the sprint-length model: how many steps the rank-1 bot takes on a move (length >= 5 only).

usage: python3 tools/clone_train_steps.py OUTDIR NPZ...   -> OUTDIR/steps.txt
Features: global features + the chosen direction's features + free steps; class = min(steps, 4) - 1.
"""
import sys, os
import numpy as np
import lightgbm as lgb

out = sys.argv[1]
files = sorted(sys.argv[2:])
rng = np.random.RandomState(0)
test_files = set(rng.choice(files, max(1, len(files) // 6), replace=False))


def load(fs):
    X, y = [], []
    for f in fs:
        z = np.load(f)
        D, G, Y, M = z['D'], z['G'], z['Y'], z['M']
        m = (Y < 4) & (G[:, 0] >= 5)
        idx = np.nonzero(m)[0]
        if not len(idx):
            continue
        chosen = D[idx, Y[idx]]
        free = (G[idx, 0:1] + 3) // 4
        X.append(np.concatenate([G[idx], chosen, free], axis=1))
        y.append(np.minimum(np.minimum(M[idx, 2], free[:, 0]), 4) - 1)
    return np.concatenate(X), np.concatenate(y).astype(int)


Xtr, ytr = load([f for f in files if f not in test_files])
Xte, yte = load([f for f in files if f in test_files])
print('train', len(ytr), 'test', len(yte), 'class shares', np.bincount(yte) / len(yte))
m = lgb.LGBMClassifier(n_estimators=int(os.environ.get('ST', 60)), num_leaves=31, learning_rate=0.1,
                       min_child_samples=50, verbose=-1)
m.fit(Xtr, ytr)
p = m.predict(Xte)
print('steps acc %.3f (always-1 baseline %.3f); mean predicted %.2f vs true %.2f' % (
    (p == yte).mean(), (yte == 0).mean(), p.mean() + 1, yte.mean() + 1))
os.makedirs(out, exist_ok=True)
m.booster_.save_model(os.path.join(out, 'steps.txt'))
