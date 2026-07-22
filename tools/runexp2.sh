#!/bin/bash
# Ensemble to estimate emergence PROBABILITY vs size. Resumable (bff2 checkpoints).
cd "$(dirname "$0")"
MUT=262144
CONFIGS=(
  # more 2^17 seeds -> P(emerge) at the size we know emerges
  "e2p17_s4 131072 8000 4 0.0"
  "e2p17_s5 131072 8000 5 0.0"
  "e2p17_s6 131072 8000 6 0.0"
  "e2p17_s7 131072 8000 7 0.0"
  # does 98304 ever fire, with more seeds / longer?
  "e98304_s2 98304 20000 2 0.0"
  "e98304_s3 98304 20000 3 0.0"
  # one very long 65536 run
  "e65536_s3 65536 40000 3 0.0"
)
echo "ENSEMBLE START $(date -u +%H:%M:%S)"
for cfg in "${CONFIGS[@]}"; do
  set -- $cfg; tag=$1; Nn=$2; ep=$3; rs=$4; sf=$5
  if [ -f "$tag.done" ]; then echo "skip $tag (done)"; continue; fi
  echo ">> $tag N=$Nn ep=$ep $(date -u +%H:%M:%S)"
  ./bff2 "$tag" "$Nn" "$ep" "$MUT" "$rs" "$sf"
  echo "   finished $tag rc=$? peak=$(awk 'NR>2{if($2>m)m=$2}END{print m}' $tag.tsv)% $(date -u +%H:%M:%S)"
done
echo "ENSEMBLE ALL DONE $(date -u +%H:%M:%S)"
