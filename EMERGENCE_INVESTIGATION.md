# Investigation: does self-replication emerge by chance in the soup?

**Question.** Agüera y Arcas et al., *Computational Life* (2024), report that
self-replicating programs arise *by chance* from an unseeded random BFF soup. The
Two-IP soup (Mode C) and README previously claimed to reproduce this ("novel
replicators emerge and persist, self-copy ~85–90% indefinitely"). Do they?

**Method.** The soup engine was ported verbatim to standalone Node scripts and run
far longer than a browser tab practically allows, starting from a random soup with
**no seeding**. Self-copy was measured with the same template-free, mirror-aware
test the tool uses (a ≥12-byte contiguous run of a program's own bytes landing in a
fresh random partner). Harness validity was confirmed: it flags the seeded
palindrome replicator at ~98% and random programs at 0%.

## Result at small scale (≤9k programs): the soup is inert

At the web tool's scale and a few times larger, self-copy stayed at **0%** in every
configuration tried:

| Reaction model | Soup size | Epochs | Mutation % | Peak self-copy |
|---|---|---|---|---|
| Mode C (two-IP ring, cap 1024) | 1,600 | 3,000 | 0.02 | 0.0% |
| Mode C, cap 1024 | 1,600 | 1,500 | 1.0 (50× default) | 0.0% |
| Mode B (single-IP, paper-faithful) | 1,600 | 1,500 | 0.02 | 0.0% |
| Mode B | 9,216 (6×) | 1,500 | 0.05 | 0.0% |

A finer diagnostic (Mode B, 2,304 programs, 4,000 epochs, mut 0.2%) tracked more
than the strict metric:

- **4-gram entropy stays pinned at ~0.99 (maximum disorder) the whole run** — it
  never begins to drop, so no motif is spreading. The soup does not start to order.
- **Lenient self-copy (6-byte runs) is also 0%** apart from single-sample noise —
  not even partial proto-replicators nucleate.
- Programs *do* execute and write (~2–20 byte-writes per interaction), but the
  writes are random and never organize into a self-copying structure.

So this is not "emergence is slow" — the nucleation event does not happen at all at
any reachable scale here.

## Resolution: it is scale. Emergence fires at cubff scale.

Two explanations were possible: a **scale threshold**, or a **fidelity gap** vs. the
reference. The fidelity gap was ruled out first by reading the `cubff` source
(`common.h`, `main.cc`, `common_language.h`):

- Reference mutation: after each interaction, **every one of the 128 bytes** of the
  concatenated pair is independently replaced with a random byte with probability
  **1/4096** (`mutation_prob = 2¹⁸`, denominator `2³⁰`). That is ≈0.0156 bytes per
  program per epoch — essentially identical to the web tool's ≈0.0128. **Mutation is
  not the difference.**
- Tape size (64), pair steps (8192), single-IP execution: all already match Mode B.
- The only ~2-orders-of-magnitude gap is **soup size: 131,072 (2¹⁷) vs. 1,600.**

A faithful native C port (`tools/bff_soup.c`, exact cubff semantics, OpenMP) was
then run at scale from a **fully random, unseeded** start. Engine validity was
confirmed separately: seeded at 20% it climbs 18%→61% and entropy falls 0.93→0.59.

**At 2¹⁷ programs, self-replication emerges by chance** — a sharp phase transition:

| epoch | self-copy | entropy |
|---|---|---|
| 3,025 | 0.0% | 0.957 |
| 3,050 | 2.8% | 0.937 |
| 3,075 | 16.2% | 0.711 |
| 3,150 | 83.7% | 0.457 |
| 3,300 | **97.25%** | 0.449 |

Flat at 0% for ~3,000 epochs, then 0%→97% in ~150 epochs as one lineage nucleates
and sweeps the soup; entropy crashes 0.96→0.45. After the peak it settles into a
live replicator ecology oscillating ~60–70% (ongoing mutation + competition). The
full trace is saved in `tools/emergence_2p17_run.tsv`.

### Soup-size sweep (single run each, 8,000 epochs, unseeded)

| Soup size | Peak self-copy | Emerged? | Final entropy |
|---|---|---|---|
| 1,600 (the web tool) | 0% | no | ~0.99 |
| 4,096 | 0.2% | no | 0.92 |
| 8,192 | 0.05% | no | 0.94 |
| 16,384 | 0.15% | no | 0.95 |
| 32,768 | 0.10% | no | 0.96 |
| 65,536 | 0.10% | no | 0.96 |
| **131,072 (2¹⁷)** | **97.25%** | **yes** | 0.45 |

**Caveats.** These are single runs per size. Emergence is stochastic and its
expected onset grows as the soup shrinks, so "no emergence in 8,000 epochs" at
≤65,536 does **not** prove those sizes never emerge — they may simply need far more
epochs. What is solid: emergence is real and reproduces the paper at 2¹⁷, and it is
utterly out of reach at the web tool's 1,600. Reproducibility across more 2¹⁷ seeds
and longer runs at 32k–65k are the remaining loose ends (compute was repeatedly
interrupted by container restarts).

## Follow-up: emergence is stochastic, and the "threshold" is a rate

A reproducibility + threshold sweep (restart-resilient `tools/bff2.c`, unseeded,
cubff mutation 1/4096; raw traces in `tools/results/`) shows the transition is
**not a sharp deterministic size threshold** — it is a stochastic nucleation event
whose rate rises steeply with soup size.

| Soup size | seeds | emerged | max epochs | detail |
|---|---|---|---|---|
| 131,072 (2¹⁷) | 3 | **2 of 3** | 20,000 | seed1 @3,150, seed3 @2,925; **seed2 never emerged in 20,000** |
| 98,304 | 1 | 0 of 1 | 12,000 | — |
| 65,536 (2¹⁶) | 2 | 0 of 2 | 20,000 | — |
| 32,768 (2¹⁵) | 1 | 0 of 1 | 30,000 | — |
| ≤9,216 | many | 0 | — | earlier JS + C runs |

Two things stand out:

1. **Even at 2¹⁷, emergence is not guaranteed.** Two seeds nucleated by ~epoch
   3,000; a third ran a full 20,000 epochs (>6× the others' onset) and never fired.
   So "2¹⁷ emerges around epoch 3,000" is really "2¹⁷ has a per-epoch nucleation
   probability high enough to usually fire within a few thousand epochs — but not
   always."
2. **Below ~10⁵ programs, no emergence was observed** in runs of 12k–30k epochs.
   With single/double seeds this does not *prove* those sizes can never emerge — the
   nucleation rate is simply low enough that none fired in the budget tested. The
   expected time-to-emergence grows rapidly as the soup shrinks.

This reframes the "critical size": there is no hard cutoff, but the emergence rate
becomes practically observable somewhere near **10⁵ programs**, which is why cubff's
default is 131,072 and why the web tool's 1,600 never shows it.

### Interpreter sanity check (why the soup never freezes)

Instrumenting the interpreter (`tools/probe.c`) over 20,000 reactions confirmed the
BFF engine runs correctly and explained the dynamics:

| soup | avg steps | ran off tape | unmatched bracket | hit 8,192-step cap (looping) |
|---|---|---|---|---|
| random (pre-life) | 617 | 61% | 33% | **6%** |
| emerged (92% self-copy) | 7,443 | 9% | 1% | **90%** |

Random code almost never forms a working loop (6% reach the step cap), so the
pre-life soup churns through short executions rather than settling. After emergence,
90% of programs are in productive copy-loops (~7,400 of 8,192 steps). The soup still
never freezes because the step cap force-terminates every loop, programs re-pair
each epoch, and mutation + partner-overwriting continuously perturb the population —
so self-copy oscillates in a live steady state rather than locking at 100%.
Replication itself was verified directly: the palindrome replicator stamps a copy of
itself into a fresh random partner in 3,000/3,000 trials; random programs, 0.

## Recommendation for the tool

Live in-browser emergence at 2¹⁷ is not feasible (JS single-threaded would take
hours per run; even optimized C is minutes). So the tool cannot show spontaneous
emergence *live*. Options, in order of value:

1. **Keep the honest framing already shipped** — the soups are seeded-replicator
   demonstrators. (Done.)
2. **Add an offline "emergence replay" mode**: precompute a 2¹⁷ run with
   `tools/bff_soup.c`, record a downsampled grid history, and let the tool play it
   back so users can *watch* the phase transition without running it live.
3. Optionally ship `tools/bff_soup.c` as the "run it yourself at scale" path for the
   curious, documented in the README.

## Reproducing

```
gcc -O3 -fopenmp tools/bff_soup.c -o bff_soup -lm      # portable build (no -march=native)
./bff_soup 131072 8000 262144 1 0.0                    # N, epochs, mut(/2^30), rng-seed, replicator-fraction
```
The 5th arg is the replicator seed fraction; **0.0 = fully random start, no copier
planted.** Emergence typically ignites between epoch ~2,000 and ~4,000.
