// Diagnostic probe: is the interpreter running correctly, and do programs loop?
// Measures, for many reactions: steps-until-halt, halt reason, and whether the
// 8192 step cap is hit (= a program stuck in a loop until forced to stop).
// Compares FRESH RANDOM programs vs. an EMERGED soup loaded from a checkpoint.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define TAPE 64
#define PAIR 128
#define STEP_CAP 8192

static uint64_t rs;
static inline uint64_t sm64(){uint64_t z=(rs+=0x9E3779B97F4A7C15ULL);z=(z^(z>>30))*0xBF58476D1CE4E5B9ULL;z=(z^(z>>27))*0x94D049BB133111EBULL;return z^(z>>31);}

// evaluate returning (steps, reason): 0=ran off tape, 1=unmatched bracket, 2=hit step cap
static int evaluate(uint8_t *t, int *out_steps){
  int ip=0,h0=0,h1=0,steps=0,reason=0;
  while(steps<STEP_CAP){
    if(ip<0||ip>=PAIR){reason=0;break;}
    uint8_t op=t[ip]; int halted=0;
    switch(op){
      case 60:h0=(h0-1)&(PAIR-1);break; case 62:h0=(h0+1)&(PAIR-1);break;
      case 123:h1=(h1-1)&(PAIR-1);break; case 125:h1=(h1+1)&(PAIR-1);break;
      case 45:t[h0]--;break; case 43:t[h0]++;break;
      case 46:t[h1]=t[h0];break; case 44:t[h0]=t[h1];break;
      case 91: if(t[h0]==0){int d=1,p=ip+1;for(;p<PAIR&&d>0;p++){if(t[p]==93)d--;else if(t[p]==91)d++;}p--;if(d!=0){halted=1;reason=1;break;}ip=p;} break;
      case 93: if(t[h0]!=0){int d=1,p=ip-1;for(;p>=0&&d>0;p--){if(t[p]==93)d++;else if(t[p]==91)d--;}p++;if(d!=0){halted=1;reason=1;break;}ip=p;} break;
      default: break;
    }
    if(halted) break; ip++; steps++; if(ip<0||ip>=PAIR){reason=0;break;}
  }
  if(steps>=STEP_CAP) reason=2;
  *out_steps=steps; return reason;
}

static void report(const char*label, uint8_t*soup, size_t N){
  const int M=20000;
  long tot=0; int capHit=0, offTape=0, unmatched=0, buckets[6]={0};
  long maxs=0;
  for(int i=0;i<M;i++){
    uint8_t cat[PAIR];
    if(soup){ size_t a=sm64()%N,b=sm64()%N; memcpy(cat,soup+a*TAPE,TAPE); memcpy(cat+TAPE,soup+b*TAPE,TAPE);}
    else { for(int j=0;j<PAIR;j++) cat[j]=sm64()&0xFF; }
    int steps, r=evaluate(cat,&steps);
    tot+=steps; if(steps>maxs)maxs=steps;
    if(r==2)capHit++; else if(r==0)offTape++; else unmatched++;
    int b = steps==0?0: steps<10?1: steps<100?2: steps<1000?3: steps<STEP_CAP?4:5;
    buckets[b]++;
  }
  printf("%-22s avg_steps=%.1f max=%ld | halt: offTape=%.1f%% unmatched=%.1f%% CAP(loop)=%.1f%%\n",
    label, (double)tot/M, maxs, 100.0*offTape/M, 100.0*unmatched/M, 100.0*capHit/M);
  printf("%-22s steps dist: [0]=%.1f%% [1-9]=%.1f%% [10-99]=%.1f%% [100-999]=%.1f%% [1k-8k)=%.1f%% [=cap]=%.1f%%\n",
    "", 100.0*buckets[0]/M,100.0*buckets[1]/M,100.0*buckets[2]/M,100.0*buckets[3]/M,100.0*buckets[4]/M,100.0*buckets[5]/M);
}

int main(int argc,char**argv){
  rs=99;
  printf("=== FRESH RANDOM programs (pre-life baseline) ===\n");
  report("random", NULL, 0);

  if(argc>1){
    FILE*f=fopen(argv[1],"rb"); if(!f){perror("ckpt");return 1;}
    uint64_t ep,gs,n; fread(&ep,8,1,f);fread(&gs,8,1,f);fread(&n,8,1,f);
    uint8_t*soup=malloc(n*TAPE); fread(soup,1,n*TAPE,f); fclose(f);
    printf("\n=== EMERGED soup %s (epoch %llu, N=%llu) ===\n", argv[1],(unsigned long long)ep,(unsigned long long)n);
    report("emerged", soup, n);
  }
  return 0;
}
