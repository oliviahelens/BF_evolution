#!/bin/bash
# Restart-resilient experiment driver. Each ./bff2 resumes from its checkpoint,
# and completed configs (<tag>.done) are skipped. Re-run this after any restart.
cd "$(dirname "$0")"
MUT=262144   # 1/4096, cubff default

# config: tag  N  epochs  rngseed  seedfrac
CONFIGS=(
  "r2p17_s2 131072 5000 2 0.0"     # reproducibility at 2^17
  "r2p17_s3 131072 5000 3 0.0"
  "n65536_s1 65536 20000 1 0.0"    # threshold: give smaller soups more time
  "n65536_s2 65536 20000 2 0.0"
  "n98304_s1 98304 12000 1 0.0"
  "n32768_s1 32768 30000 1 0.0"
)

echo "DRIVER START $(date -u +%H:%M:%S)"
for cfg in "${CONFIGS[@]}"; do
  set -- $cfg; tag=$1; Nn=$2; ep=$3; rs=$4; sf=$5
  if [ -f "$tag.done" ]; then echo "skip $tag (done)"; continue; fi
  echo ">> $tag  N=$Nn ep=$ep  $(date -u +%H:%M:%S)"
  ./bff2 "$tag" "$Nn" "$ep" "$MUT" "$rs" "$sf"
  echo "   finished $tag rc=$? $(date -u +%H:%M:%S)"
done
echo "DRIVER ALL DONE $(date -u +%H:%M:%S)"
