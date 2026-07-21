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

## Result: the unseeded soup is inert

Self-copy stayed at **0%** in every configuration tried:

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

## Interpretation

Two explanations remain, not yet disambiguated:

1. **Scale threshold.** The paper uses soups of 2¹³–2¹⁶ programs (8k–64k) and long
   runs; spontaneous emergence may be a sharp threshold phenomenon below which
   nucleation effectively never fires. Tested up to 6× the tool's size without a
   hint of nucleation, but that is still ~7× below the paper's largest soup.
2. **Fidelity gap vs. reference cubff.** Some BFF interaction detail (mutation model
   applied per-byte across the whole soup, program length, head initialization, or a
   semantic subtlety) may differ from the reference and be what enables nucleation.

## Concrete next steps

- Compare parameters and interaction semantics against the reference `cubff`
  implementation — especially the **mutation model**, which is the usual source of
  the raw variation nucleation feeds on.
- Run a genuinely paper-scale soup (2¹⁴–2¹⁶) for ≥10k epochs offline (out of reach
  of a browser tab; heavy but feasible as a batch job) to test the scale-threshold
  hypothesis directly.
- If emergence is reproduced offline, decide whether it is worth surfacing in the
  tool (likely as an offline-computed replay rather than live, given the compute).

Until one of these lands, the tool is honestly a **seeded-replicator
demonstrator**: it shows that a replicator, once present, takes over — not that one
arises from nothing.
