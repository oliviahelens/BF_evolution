# Brainfuck Evolution Lab + BFF Playground

Two self-contained, single-file interactive web tools. No build step, no
dependencies, no storage — open either HTML file directly in a browser.

## `evolution_lab.html` — Brainfuck Evolution Lab
A genetic-programming tool that evolves a standard Brainfuck program from random
noise until its output matches a target string.

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
- A *single* seeded program dies out stochastically, so **Seed replicator** seeds
  ~20% of the soup at once. This makes the takeover reliably demonstrable (the
  brief's stated goal) rather than chance-dependent; replicator coverage then
  climbs to ~70–90% within a couple hundred epochs.
