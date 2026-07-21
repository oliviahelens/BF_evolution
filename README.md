# BF Evolution Lab + BFF Playground

Two self-contained, single-file interactive web tools. Open either HTML file
directly in a browser.

## `evolution_lab.html` — BF Evolution Lab
A genetic-programming tool that evolves a standard BF (Brainfuck) program from
random noise until its output matches a target string.

- Alphabet `+ - < > [ ] .` (no input). Tape 4000 cells, 6000-step execution cap.
- Population 400; fitness = per-byte distance to the target (+256 for missing
  bytes) plus a small bloat penalty; top 50% survive; 6 elites carried over;
  ~12% fresh random injected each generation; 60% crossover / 40% clone, then a
  burst-mutation operator (substitute / insert / delete).
- Type a target (default `hi`), press **Evolve**. `hi` solves in ~100–150
  generations. Longer targets (e.g. `brain`) take many thousands and may need a
  **Reset** — the diversity injection keeps runs off permanent plateaus.

## `bff_playground.html` — BFF Playground
An interactive for the self-modifying "BFF" variant from Agüera y Arcas et al.,
*Computational Life* (2024), where code and data share one tape and programs can
copy themselves. Three pointers run over one byte tape: an instruction pointer
and two heads (`head0`/read, `head1`/write).

**Mode A — Tape debugger.** Type a tape, set head start positions, then
**Step**/**Run**. The tape is drawn as cells (toggle value/glyph) with IP, R and
W marked in distinct colors; the log narrates each instruction and highlights the
cells it changed. Presets: the self-copier `[.>}]` and the paper's palindrome
replicator `[[{.>]-] ]-]>.{[[`.

**Mode B — Primordial soup.** A 40×40 grid of 64-byte programs. Each epoch pairs
random programs, concatenates them into a 128-byte tape, runs it under BFF rules,
and splits the result back — no fitness, no selection. **Seed replicator** drops
the palindrome replicator into the soup; watch the replicator's color overtake
the grid. Color shows similarity to the replicator (or a content hash).

**Mode C — Two-IP soup.** The same soup, but each concatenated pair is treated as
a 128-cell **ring** with **two** instruction pointers — one per program (starting
at positions 0 and 64), each with its own heads. Both run forward and wrap past the
end, alternating one step at a time, so either program can act on the other (the
setup is symmetric, so pairing order is irrelevant). With no "run off the end" to
stop them, reactions end at an adjustable **step cap** (default 1024). Press
**Seed replicator** to drop the palindrome into ~20% of the grid; its lineage then
spreads and the self-copy rate climbs, and a higher step cap gives it more room to
act.

**Spontaneous emergence — honest status.** The paper reports self-replicators
arising *by chance* from an unseeded random soup. That result does **not** reproduce
at this tool's scale: from random noise with no seed, the self-copy rate stays at
**0%** for thousands of epochs at 1,600 programs (and up to 65,536 in single runs).
It is a matter of **scale**, not a bug — a faithful native port at the paper's
131,072-program (2¹⁷) scale *does* emerge by chance: 0% → **97% self-copy in ~150
epochs** around epoch ~3,000, with entropy crashing 0.96 → 0.45. Reproducing the
per-program mutation rate confirmed it matches the reference `cubff` (≈0.015
bytes/program/epoch), so mutation is not the missing ingredient — soup size is.
Emergence at 2¹⁷ takes minutes in optimized C and is out of reach for a live browser
tab, so this tab remains a **seeded-replicator demonstrator**. See
[`EMERGENCE_INVESTIGATION.md`](EMERGENCE_INVESTIGATION.md) for the full write-up and
[`tools/bff_soup.c`](tools/bff_soup.c) to run the emergence experiment yourself.

Both soups report, beyond the seed-template match (`replicators` / `avg seed
similarity`): **entropy** (normalized Shannon entropy of 4-grams across the soup, a
template-free order signal that falls as any motif spreads), **self-copy rate** (a
template-free, mirror-aware functional test — the fraction of programs that copy a
≥12-byte run of themselves into a fresh random partner, which catches novel
replicators the seed template misses), and a sparkline of these over time.

### Semantics (faithful to cubff)
Command bytes: `<`60 `>`62 `{`123 `}`125 `-`45 `+`43 `.`46 `,`44 `[`91 `]`93;
every other byte is a no-op. Loops test `tape[head0]`. Brackets are matched
**dynamically over the live tape** at jump time (the tape rewrites itself).
Heads wrap mod tape length; the IP halts when it runs off the tape, hits an
unmatched bracket, or after 8192 steps.

### Notes on the soup
- The brief specifies heads start at position 0 of the concatenation. Under those
  semantics the palindrome replicator is genuinely self-replicating (it stamps a
  mirror copy of itself into its partner), which was verified directly.
- Nothing self-replicating emerges from the random soup on its own at this scale,
  so both soups rely on seeding to show anything. **Seed replicator** seeds ~20% of
  the soup at once (a *single* seed dies out stochastically); the seeded lineage
  then spreads to ~70–90% coverage within a couple hundred epochs. This demonstrates
  that a self-replicator, once present, takes over — it does **not** demonstrate that
  one arises by chance (see "Spontaneous emergence" above).
