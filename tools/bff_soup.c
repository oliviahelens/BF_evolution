// Faithful CPU port of cubff BFF soup (single-IP, 64-byte tapes, 8192 steps,
// per-byte 1/4096 mutation over the concatenated pair). Tests whether
// self-replicators emerge by chance as a function of soup size.
//
// build: gcc -O3 -march=native -fopenmp bff.c -o bff
// run:   ./bff <num_programs> <epochs> <mutation_prob_num over 2^30> [seed]
//   e.g. ./bff 131072 2000 262144   (262144 = 1<<18 = 1/4096, cubff default)
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <omp.h>

#define TAPE 64
#define PAIR 128
#define STEP_CAP 8192
#define FUNC_L 12

static inline uint64_t sm64(uint64_t *s){ // splitmix64
  uint64_t z = (*s += 0x9E3779B97F4A7C15ULL);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

// Run one 128-byte concatenated pair as a single BFF program: IP from 0,
// h0=h1=0, dynamic bracket matching over the live tape, halt off-tape / unmatched
// bracket / step cap.
static void evaluate(uint8_t *t){
  int ip=0, h0=0, h1=0, steps=0;
  while(steps<STEP_CAP){
    if(ip<0||ip>=PAIR) break;
    uint8_t op=t[ip]; int halted=0;
    switch(op){
      case 60: h0=(h0-1)&(PAIR-1); break;
      case 62: h0=(h0+1)&(PAIR-1); break;
      case 123:h1=(h1-1)&(PAIR-1); break;
      case 125:h1=(h1+1)&(PAIR-1); break;
      case 45: t[h0]--; break;
      case 43: t[h0]++; break;
      case 46: t[h1]=t[h0]; break;
      case 44: t[h0]=t[h1]; break;
      case 91: if(t[h0]==0){ int d=1,p=ip+1; for(;p<PAIR&&d>0;p++){ if(t[p]==93)d--; else if(t[p]==91)d++; } p--; if(d!=0){halted=1;break;} ip=p; } break;
      case 93: if(t[h0]!=0){ int d=1,p=ip-1; for(;p>=0&&d>0;p--){ if(t[p]==93)d++; else if(t[p]==91)d--; } p++; if(d!=0){halted=1;break;} ip=p; } break;
      default: break;
    }
    if(halted) break;
    ip++; steps++;
    if(ip<0||ip>=PAIR) break;
  }
}

// ---- metrics -----------------------------------------------------------
static const int MSWAP_FROM[6]={60,62,123,125,91,93};
static const int MSWAP_TO[6]  ={62,60,125,123,93,91};
static uint8_t mswap(uint8_t b){ for(int i=0;i<6;i++) if(b==MSWAP_FROM[i]) return MSWAP_TO[i]; return b; }

// does program P copy a >=12-byte run of itself (fwd or BF-mirror) into a fresh
// random partner? (same template-free test the web tool uses)
static int self_copies(const uint8_t *P, uint64_t *rng){
  uint8_t cat[PAIR], R[TAPE], Pm[TAPE];
  for(int i=0;i<TAPE;i++) R[i]=sm64(rng)&0xFF;
  for(int i=0;i<TAPE;i++) Pm[i]=mswap(P[TAPE-1-i]);
  memcpy(cat,P,TAPE); memcpy(cat+TAPE,R,TAPE);
  evaluate(cat);
  // check every 12-window in partner half against every 12-window of P and Pm
  for(int j=0;j+FUNC_L<=TAPE;j++){
    const uint8_t *w=cat+TAPE+j;
    // skip windows the random partner already had (≈never for 12 bytes) — check against original R
    int hadR=0;
    for(int k=0;k+FUNC_L<=TAPE;k++){ if(!memcmp(w,R+k,FUNC_L)){hadR=1;break;} }
    if(hadR) continue;
    for(int s=0;s+FUNC_L<=TAPE;s++){ if(!memcmp(w,P+s,FUNC_L)) return 1; if(!memcmp(w,Pm+s,FUNC_L)) return 1; }
  }
  return 0;
}

int main(int argc, char**argv){
  size_t N = argc>1? strtoull(argv[1],0,10) : 131072;
  int EPOCHS = argc>2? atoi(argv[2]) : 2000;
  uint32_t mut_prob = argc>3? (uint32_t)strtoul(argv[3],0,10) : (1u<<18); // /2^30
  uint64_t seed = argc>4? strtoull(argv[4],0,10) : 12345;
  double seed_frac = argc>5? atof(argv[5]) : 0.0;   // fraction to seed w/ palindrome replicator
  if(N&1) N--;

  uint8_t *soup = malloc(N*TAPE);
  uint32_t *idx = malloc(N*sizeof(uint32_t));
  uint64_t gseed=seed;
  for(size_t i=0;i<N*TAPE;i++) soup[i]=sm64(&gseed)&0xFF;
  for(size_t i=0;i<N;i++) idx[i]=i;
  // optional: seed a fraction with the paper's palindrome replicator core
  const char *REP = "[[{.>]-] ]-]>.{[[";
  int RL = (int)strlen(REP);
  if(seed_frac>0){
    size_t ns=(size_t)(N*seed_frac);
    for(size_t k=0;k<ns;k++){ size_t p=sm64(&gseed)%N; for(int i=0;i<RL;i++) soup[p*TAPE+i]=(uint8_t)REP[i]; }
  }

  printf("# BFF soup  N=%zu  epochs=%d  mut=%u/2^30 (1/%.0f)  steps=%d\n",
         N,EPOCHS,mut_prob, (double)(1u<<30)/mut_prob, STEP_CAP);
  printf("# epoch\tselfcopy%%\tentropy4\n");
  fflush(stdout);

  int nth = omp_get_max_threads();

  for(int e=0; e<=EPOCHS; e++){
    if(e){
      // Fisher-Yates shuffle (serial, cheap vs. reactions)
      for(size_t i=N-1;i>0;i--){ uint64_t r=sm64(&gseed); size_t j=r%(i+1); uint32_t t=idx[i];idx[i]=idx[j];idx[j]=t; }
      #pragma omp parallel
      {
        int tid=omp_get_thread_num();
        uint64_t rng = seed ^ (0x1234567ULL*(e+1)) ^ (0x9abcdefULL*(tid+1));
        #pragma omp for schedule(static)
        for(size_t k=0;k<N/2;k++){
          uint32_t a=idx[2*k], b=idx[2*k+1];
          uint8_t cat[PAIR];
          memcpy(cat, soup+(size_t)a*TAPE, TAPE);
          memcpy(cat+TAPE, soup+(size_t)b*TAPE, TAPE);
          evaluate(cat);
          // per-byte mutation over the whole pair (cubff model)
          for(int i=0;i<PAIR;i++){
            uint64_t rr=sm64(&rng);
            uint8_t repl=rr&0xFF;
            if( ((rr>>8)&((1ULL<<30)-1)) < mut_prob ) cat[i]=repl;
          }
          memcpy(soup+(size_t)a*TAPE, cat, TAPE);
          memcpy(soup+(size_t)b*TAPE, cat+TAPE, TAPE);
        }
      }
    }
    // metrics every 25 epochs (and at 0)
    if(e%25==0){
      // self-copy rate over a sample
      int SAMP = N<2000? (int)N : 2000;
      int hits=0;
      #pragma omp parallel reduction(+:hits)
      {
        uint64_t rng = 0xABCDEFULL ^ (seed*7) ^ (0x777ULL*(e+1)) ^ (0x13ULL*(omp_get_thread_num()+1));
        #pragma omp for schedule(static)
        for(int s=0;s<SAMP;s++){ size_t p=(sm64(&rng)%N); if(self_copies(soup+p*TAPE,&rng)) hits++; }
      }
      // 4-gram normalized entropy over a sample (hash table via sort-free approx: use 2^20 buckets)
      // sample up to 4096 programs
      int ES = N<4096?(int)N:4096;
      // count 4-grams in a hashmap approximated by a big array of 2^24 (16M) -> too big per-call; use open map
      // simpler: order-0 byte entropy over the sample as a coarse order signal, plus 4-gram via small hash
      static int *cnt=NULL; static size_t CB=1u<<24; if(!cnt) cnt=calloc(CB,sizeof(int));
      // clear lazily: we track touched keys
      // To keep it simple and correct, use a local dynamic approach:
      // (4-gram key is 32-bit; bucket by &(CB-1))
      size_t M=0;
      // gather touched buckets to reset
      static uint32_t *touched=NULL; if(!touched) touched=malloc((size_t)ES*(TAPE-3)*sizeof(uint32_t));
      size_t nt=0;
      uint64_t rng2=0xFEEDULL^(seed*3)^(0x99ULL*(e+1));
      for(int s=0;s<ES;s++){
        size_t p=(sm64(&rng2)%N); const uint8_t*t=soup+p*TAPE;
        for(int j=0;j+4<=TAPE;j++){
          uint32_t key=((uint32_t)t[j]<<24)|((uint32_t)t[j+1]<<16)|((uint32_t)t[j+2]<<8)|t[j+3];
          size_t bk=key&(CB-1);
          if(cnt[bk]==0) touched[nt++]=bk;
          cnt[bk]++; M++;
        }
      }
      double H=0; for(size_t i=0;i<nt;i++){ double pr=(double)cnt[touched[i]]/M; H-=pr*(log(pr)/log(2.0)); cnt[touched[i]]=0; }
      double Hn = H/(log((double)M)/log(2.0));
      printf("%d\t%.3f\t%.4f\n", e, 100.0*hits/SAMP, Hn);
      fflush(stdout);
    }
  }
  return 0;
}
