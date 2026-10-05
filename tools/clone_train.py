"""Train the behavioural-clone models from tools/clone_extract.py outputs.

usage: python3 tools/clone_train.py OUTDIR NPZ...
Writes OUTDIR/kind.txt and OUTDIR/dir.txt (LightGBM text models) and prints held-out accuracy.
"""
import sys, os
import numpy as np
import lightgbm as lgb

out = sys.argv[1]
files = sorted(sys.argv[2:])
rng = np.random.RandomState(0)
test_files = set(rng.choice(files, max(1, len(files) // 6), replace=False))
DIRS = {'D': [], 'G': [], 'Y': []}
tr = {k: [] for k in 'DGY'}
te = {k: [] for k in 'DGY'}
for f in files:
    z = np.load(f)
    t = te if f in test_files else tr
    for k in 'DGY':
        t[k].append(z[k])
tr = {k: np.concatenate(v) for k, v in tr.items()}
te = {k: np.concatenate(v) for k, v in te.items()}
if os.environ.get('QUEEN'):
    # queen-only models: the colony model saw one queen decision in ~40
    for t in (tr, te):
        m = t['G'][:, 3] > 0
        for k in 'DGY':
            t[k] = t[k][m]
print('train', len(tr['Y']), 'test', len(te['Y']))


def kind_xy(t):
    X = np.concatenate([t['G'], t['D'].reshape(len(t['D']), -1)], axis=1)
    y = np.where(t['Y'] < 4, 0, t['Y'] - 3)   # 0 move, 1 split, 2 suicide
    return X, y


def dir_xy(t):
    m = t['Y'] < 4
    D, G, Y = t['D'][m], t['G'][m], t['Y'][m]
    n = len(Y)
    rows = np.concatenate([D.reshape(n * 4, -1), np.repeat(G, 4, axis=0),
                           np.tile(np.eye(4, dtype=np.float32), (n, 1))], axis=1)
    lab = np.zeros(n * 4, np.int32)
    lab[np.arange(n) * 4 + Y] = 1
    return rows, lab, n, Y


Xk, yk = kind_xy(tr)
Xkt, ykt = kind_xy(te)
mk = lgb.LGBMClassifier(n_estimators=int(os.environ.get('KT', 120)), num_leaves=int(os.environ.get('NL', 31)), learning_rate=float(os.environ.get('LR', 0.1)),
                        min_child_samples=50, verbose=-1)
mk.fit(Xk, yk)
pk = mk.predict(Xkt)
print('kind acc %.4f (baseline move %.4f)' % ((pk == ykt).mean(), (ykt == 0).mean()))
for c in (1, 2):
    sel = ykt == c
    print('  class %d recall %.3f precision %.3f' % (c, (pk[sel] == c).mean(), (ykt[pk == c] == c).mean() if (pk == c).any() else 0))

Xd, yd, nd, _ = dir_xy(tr)
Xdt, ydt, ndt, Yt = dir_xy(te)
md = lgb.LGBMRanker(n_estimators=int(os.environ.get('DT', 150)), num_leaves=int(os.environ.get('NL', 31)), learning_rate=float(os.environ.get('LR', 0.1)),
                    min_child_samples=50, verbose=-1)
md.fit(Xd, yd, group=[4] * nd)
sc = md.predict(Xdt).reshape(ndt, 4)
legal = te['D'][te['Y'] < 4][:, :, 0] > 0
sc = np.where(legal, sc, -1e9)
pred = sc.argmax(1)
print('dir acc %.4f' % (pred == Yt).mean())
# baseline: uniform among legal
print('  random-legal baseline %.4f' % np.mean(1.0 / np.maximum(1, legal.sum(1))))
os.makedirs(out, exist_ok=True)
mk.booster_.save_model(os.path.join(out, 'kind.txt'))
md.booster_.save_model(os.path.join(out, 'dir.txt'))
imp = md.booster_.feature_importance('gain')
names = ['d%d' % i for i in range(23)] + ['g%d' % i for i in range(14)] + ['abs%d' % i for i in range(4)]
print('top dir features', sorted(zip(imp, names), reverse=True)[:15])

if os.environ.get('BREAKDOWN'):
    Dm = te['D'][te['Y'] < 4]; Gm = te['G'][te['Y'] < 4]
    ok = pred == Yt
    vis = Gm[:, 12] > 0
    en = Gm[:, 4] > 0
    nl = legal.sum(1)
    print('pearl visible: n=%d acc %.3f | none: n=%d acc %.3f' % (vis.sum(), ok[vis].mean(), (~vis).sum(), ok[~vis].mean()))
    print('enemy visible acc %.3f | none %.3f' % (ok[en].mean(), ok[~en].mean()))
    for k in (1, 2, 3):
        s = nl == k
        print('legal=%d n=%d acc %.3f' % (k, s.sum(), ok[s].mean()))
    for r0 in (0, 100, 200, 300, 400):
        s = (Gm[:, 1] >= r0) & (Gm[:, 1] < r0 + 100)
        print('round %d acc %.3f' % (r0, ok[s].mean()))
