#!/bin/bash
# Probability study: P(emerge within 8000 epochs) vs soup size. Many seeds/size.
# 8000 epochs is ample (observed onsets were all < 3200). Resumable via bff2 ckpts.
cd "$(dirname "$0")"
MUT=262144; EP=8000
declare -a JOBS
add(){ local N=$1; shift; for s in "$@"; do JOBS+=("p${N}_s${s} $N $EP $s"); done; }
add 65536  4 5 6 7 8 9 10 11
add 98304  4 5 6 7 8 9 10 11
add 131072 8 9 10 11 12 13
add 163840 1 2 3 4 5 6

echo "PROBSTUDY START $(date -u +%H:%M:%S)  (${#JOBS[@]} runs)"
for j in "${JOBS[@]}"; do
  set -- $j; tag=$1; Nn=$2; ep=$3; rs=$4
  if [ -f "$tag.done" ]; then echo "skip $tag"; continue; fi
  echo ">> $tag N=$Nn $(date -u +%H:%M:%S)"
  ./bff2 "$tag" "$Nn" "$ep" "$MUT" "$rs" 0.0
  echo "   $tag rc=$? peak=$(awk 'NR>2{if($2>m)m=$2}END{print m}' $tag.tsv)% em=$(awk 'NR>2&&$2>50{print $1;exit}' $tag.tsv) $(date -u +%H:%M:%S)"
done
echo "PROBSTUDY DONE $(date -u +%H:%M:%S)"
