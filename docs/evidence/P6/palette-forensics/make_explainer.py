"""Palette-mapping explainer for the Captain (P6-PALETTE-FORENSICS).

Real data only:
  * palette stops: src/k1/core/visual/product_palette_data.cpp kStops31 / kStops38
  * renderer pixels: host replay of BOOM (room mic) through p6_v8_it5 / p6_v9_it6,
    frames byte-identical to the frozen P6 replay records (sha256 printed below)
  * gate / coordinate statistics: the replay CSVs.
"""
import csv, hashlib, math, collections, sys
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle

OUT = sys.argv[1]
TMP = "/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/palforensics/"
P6 = "/Users/spectrasynq/.claude/jobs/c8f0d616/tmp/p6/"

P32 = [[0, 0, 0, 0], [42, 42, 0, 0], [84, 255, 0, 0], [127, 255, 0, 45], [170, 255, 0, 255],
       [212, 255, 55, 45], [255, 255, 255, 0]]
P39 = [[0, 12, 0, 10], [32, 65, 0, 45], [64, 145, 0, 80], [96, 220, 0, 70], [128, 255, 36, 24],
       [160, 255, 92, 0], [192, 255, 145, 0], [218, 255, 190, 40], [240, 255, 70, 120], [255, 12, 0, 10]]


def sample(stops, u):
    u = u - math.floor(u)
    for a, b in zip(stops, stops[1:]):
        if u <= b[0] / 255.0:
            span = (b[0] - a[0]) / 255.0
            f = 0.0 if span <= 0 else max(0.0, min(1.0, (u - a[0] / 255.0) / span))
            return [(a[i] + (b[i] - a[i]) * f) / 255.0 for i in (1, 2, 3)]
    return [stops[-1][i] / 255.0 for i in (1, 2, 3)]


def frames(path):
    raw = open(path, "rb").read()
    print(path.split("/")[-1], hashlib.sha256(raw).hexdigest())
    a = np.frombuffer(raw, dtype="<u2").reshape(-1, 2, 160, 3).astype(np.float64) / 65535.0
    return a  # [frame, board, pixel, rgb]


def visible(rgb, ref):
    """Show each pixel's colour clearly: hue kept exactly, level lifted (sqrt) and capped."""
    m = rgb.max(axis=-1, keepdims=True)
    lift = np.clip(np.sqrt(m / ref), 0, 1)
    out = np.where(m > 1e-5, rgb / np.maximum(m, 1e-9) * lift, 0.0)
    return np.clip(out, 0, 1)


v8 = frames(TMP + "v8_boom.bin")
v9 = frames(TMP + "v9_boom.bin")
n = v8.shape[0]
t = np.arange(n) * 0.01

# Kymographs, board 0 (mode 32 Hybrid K1, palette 32). Rows = pixels 0..159, columns = time.
def kymo(a):
    k = a[:, 0, :, :]
    ref = np.percentile(k.max(axis=-1), 99.5)
    return visible(k, ref).transpose(1, 0, 2)

k8, k9 = kymo(v8), kymo(v9)

# CSV statistics (board 0, music frames)
def rows(path):
    return [r for r in csv.DictReader(open(path)) if r["board"] == "0"
            and r["silence"] in ("0", "false") and r["fresh"] in ("1", "true")]

def moving_seconds(R):
    w = collections.defaultdict(list)
    for r in R:
        w[int(float(r["t_s"]))].append(float(r["palette_coordinate"]))
    moving = 0
    for v in w.values():
        u = [v[0]]
        for x in v[1:]:
            u.append(u[-1] + ((x - (u[-1] % 1.0) + 0.5) % 1.0 - 0.5))
        moving += (max(u) - min(u)) > 1.0 / 32.0
    return moving / len(w)

mv8 = moving_seconds(rows(TMP + "v8_boom.csv"))
mv9 = moving_seconds(rows(TMP + "v9_boom.csv"))
gate = {}
for tag, f in (("-6 dB", "boom_room-6dB.csv"), ("as played", "boom_room.csv"), ("+6 dB", "boom_room+6dB.csv")):
    R = rows(P6 + "replay6/default/" + f)
    g = [float(r["tonality_gate"]) for r in R]
    gate[tag] = sum(1 for x in g if x <= 1e-4) / len(g)
print("moving seconds v8 %.3f v9 %.3f" % (mv8, mv9), "gate shut", gate)

# Pick three loud moments where both renderers have light on board 0.
lvl8 = v8[:, 0].max(axis=(1, 2)); lvl9 = v9[:, 0].max(axis=(1, 2))
both = np.minimum(lvl8 / lvl8.max(), lvl9 / lvl9.max())
moments = []
for lo, hi in ((15, 35), (45, 65), (75, 95)):
    i0, i1 = int(lo / 0.01), int(hi / 0.01)
    moments.append(i0 + int(np.argmax(both[i0:i1])))

# ---------------------------------------------------------------- figure
INK = "#1d1d1f"; MUTED = "#5f6368"; BAD = "#b3261e"; GOOD = "#1e6b3a"
plt.rcParams.update({"font.family": "Helvetica", "font.size": 13, "text.color": INK,
                     "axes.labelcolor": INK, "xtick.color": MUTED, "ytick.color": MUTED})
fig = plt.figure(figsize=(16, 25), dpi=100, facecolor="white")

def axes(x, y, w, h):
    return fig.add_axes([x, 1 - y - h, w, h])

def heading(y, text, sub=None, colour=INK):
    fig.text(0.04, 1 - y, text, fontsize=21, weight="bold", color=colour, va="top")
    if sub:
        fig.text(0.04, 1 - y - 0.024, sub, fontsize=14, color=MUTED, va="top")

fig.text(0.04, 0.985, "How the lights pick colours from a palette", fontsize=30, weight="bold", va="top")
fig.text(0.04, 0.957, "Titan v9 today versus the colour rule that fixed the K1 in July 2026.  "
         "Real palettes, real song (BOOM, room microphone), real renderer output from the host replay.",
         fontsize=14, color=MUTED, va="top")

# 1. Palettes
heading(0.062, "1.  The two palettes", "Left end = position 0, right end = position 1. Every colour shown below is taken from these strips.")
for j, (name, stops) in enumerate((("Palette 32  (Black-Red-Magenta-Yellow)", P32), ("Palette 39  (K1 Crimson Gold)", P39))):
    ax = axes(0.04 + j * 0.48, 0.106, 0.44, 0.028)
    u = np.linspace(0, 1, 512)
    ax.imshow(np.array([[sample(stops, x) for x in u]]), aspect="auto", extent=(0, 1, 0, 1))
    ax.set_yticks([]); ax.set_xticks([0, 0.25, 0.5, 0.75, 1]); ax.tick_params(labelsize=11)
    for s in ax.spines.values(): s.set_visible(False)
    ax.set_title(name, fontsize=14, loc="left", pad=6)

def kymo_panel(y, k, label):
    ax = axes(0.04, y, 0.92, 0.10)
    ax.imshow(k, aspect="auto", extent=(0, n * 0.01, 160, 0), interpolation="nearest")
    ax.set_facecolor("black")
    ax.set_yticks([0, 80, 160]); ax.set_yticklabels(["end", "centre", "end"], fontsize=11)
    ax.set_xlabel("seconds into the song", fontsize=12)
    ax.tick_params(labelsize=11)
    ax.text(0.005, 1.04, label, transform=ax.transAxes, fontsize=13, color=MUTED)
    for m in moments:
        ax.axvline(m * 0.01, color="white", lw=1.0, ls=(0, (2, 3)))
    return ax

def snapshots(y, a, ref_board=0, colour=INK):
    ref = np.percentile(a[:, 0].max(axis=-1), 99.5)
    for j, m in enumerate(moments):
        ax = axes(0.04 + j * 0.31, y, 0.29, 0.025)
        px = visible(a[m, 0][None, :, :], ref)
        ax.imshow(px, aspect="auto", interpolation="nearest")
        ax.set_xticks([]); ax.set_yticks([]); ax.set_facecolor("black")
        ax.set_title("the 160 LEDs at %.1f s" % (m * 0.01), fontsize=12, loc="left", pad=4, color=colour)


def coord_series(path):
    R = rows(path)
    return np.array([float(r["t_s"]) for r in R]), np.array([float(r["palette_coordinate"]) for r in R])

def fold32(c):
    lo, hi = 0.3294, 1.0
    return lo + (hi - lo) * (1.0 - abs(2.0 * c - 1.0))

def trace_panel(y, path, colour_of, label, colour):
    tt, cc = coord_series(path)
    ax = axes(0.04, y, 0.92, 0.06)
    u = np.linspace(0, 1, 256)
    bg = np.array([[colour_of(x)] for x in u[::-1]])
    ax.imshow(bg, aspect="auto", extent=(0, n * 0.01, 0, 1), alpha=0.9)
    ax.plot(tt, cc, ".", ms=1.6, color="white")
    ax.set_xlim(0, n * 0.01); ax.set_ylim(0, 1)
    ax.set_yticks([0, 0.5, 1]); ax.tick_params(labelsize=10)
    ax.set_ylabel("palette\nposition", fontsize=11)
    ax.text(0.005, 1.06, label, transform=ax.transAxes, fontsize=13, color=colour)

# 2. v9 today
heading(0.153, "2.  Titan v9 today:  the colour parks and the whole strip wears it",
        "Each column is one moment; each row is one LED of the lower edge (palette 32).", BAD)
kymo_panel(0.195, k9, "v9 renderer output over the whole song")
trace_panel(0.33, TMP + "v9_boom.csv", lambda c: sample(P32, fold32(c)),
            "where v9 samples the palette, frame by frame (white dots; background = colour at that position)", BAD)
snapshots(0.418, v9, colour=BAD)
fig.text(0.04, 1 - 0.452,
         "The colour moves only when v9 is confident about the harmony, and then at most a quarter of the palette per second.\n"
         "Confidence is multiplied by a 'tonality gate' that shuts on dense, loud music, so the position sits still for most of the song.",
         fontsize=14, va="top")

# 3. K1 rule
heading(0.505, "3.  The K1 fix:  colour is re-read from the music every frame",
        "K1 commit ae5d90a (2 Jul 2026, in the shipping k1_hardware build). Titan v8 already follows the same rule.", GOOD)
kymo_panel(0.55, k8, "v8 renderer output over the same song (same LEDs, same palette)")
trace_panel(0.685, TMP + "v8_boom.csv", lambda c: sample(P32, c),
            "where the K1 rule samples the palette, frame by frame", GOOD)
snapshots(0.773, v8, colour=GOOD)
fig.text(0.04, 1 - 0.807,
         "Every frame the 12 note strengths (folded from the 80 frequency bins) give one live palette position; that colour is laid down\n"
         "and travels outward. On palette 32 most live positions fall in its long black-to-red stretch, so this song reads red with bright and dark steps.",
         fontsize=14, va="top")

# 4. Measurements
heading(0.86, "4.  What the replay measures (BOOM, room microphone)")
ax = axes(0.17, 0.893, 0.30, 0.05)
vals = [mv8 * 100, mv9 * 100]
ax.barh([1, 0], vals, color=[GOOD, BAD], height=0.55)
ax.set_yticks([1, 0]); ax.set_yticklabels(["K1 rule (Titan v8)", "Titan v9"], fontsize=13)
ax.set_xlim(0, 100); ax.set_xlabel("% of seconds in which the colour moves at all", fontsize=12)
for yv, v in zip([1, 0], vals):
    ax.text(v + 1.5, yv, "%.0f%%" % v, va="center", fontsize=14, weight="bold")
for s in ("top", "right"): ax.spines[s].set_visible(False)

ax = axes(0.60, 0.893, 0.36, 0.05)
tags = list(gate.keys()); gv = [gate[k] * 100 for k in tags]
ax.bar(range(3), gv, color=[BAD] * 3, width=0.55)
ax.set_xticks(range(3)); ax.set_xticklabels(["quieter (-6 dB)", "as played", "louder (+6 dB)"], fontsize=12)
ax.set_ylim(0, 100); ax.set_ylabel("% of music frames", fontsize=12)
for i, v in enumerate(gv):
    ax.text(i, v + 2.5, "%.0f%%" % v, ha="center", fontsize=14, weight="bold")
ax.set_title("v9: how often the gate holds the colour still", fontsize=13, loc="left")
for s in ("top", "right"): ax.spines[s].set_visible(False)

fig.text(0.04, 0.006,
         "Louder music shuts the gate more often. That is the same failure the K1 had before 2 Jul 2026 ('every palette shows a handful of colours').\n"
         "Colours in panels 2 and 3 are lifted for visibility; hue is unchanged. PROPOSAL - NOT APPROVED: see FORENSICS.md for the fix.",
         fontsize=12.5, color=MUTED, va="bottom")
fig.savefig(OUT, dpi=100, facecolor="white")
print("wrote", OUT, "moments", [round(m * 0.01, 2) for m in moments])
