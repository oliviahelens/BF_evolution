// Checkpoint/resume BFF soup simulator (restart-resilient).
// Faithful cubff semantics: single-IP over 128-byte pair, 8192 steps, dynamic
// bracket matching, per-byte 1/4096 mutation over the whole pair.
//
// Disk survives container restarts; processes don't. So we snapshot the soup
// every CKPT_EVERY epochs and resume from it. Re-running the same command after
// a restart continues the run instead of starting over.
//
// build: gcc -O3 -fopenmp bff2.c -o bff2 -lm
// run:   ./bff2 <tag> <N> <epochs> <mut/2^30> <rngseed> <seedfrac>
// files: <tag>.tsv (metrics, appended), <tag>.ckpt (state), <tag>.done (marker)
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
#define CKPT_EVERY 200
#define METRIC_EVERY 25

static inline uint64_t sm64(uint64_t *s){
  uint64_t z=(*s+=0x9E3779B97F4A7C15ULL);
  z=(z^(z>>30))*0xBF58476D1CE4E5B9ULL; z=(z^(z>>27))*0x94D049BB133111EBULL; return z^(z>>31);
}
static void evaluate(uint8_t *t){
  int ip=0,h0=0,h1=0,steps=0;
  while(steps<STEP_CAP){
    if(ip<0||ip>=PAIR) break;
    uint8_t op=t[ip]; int halted=0;
    switch(op){
      case 60:h0=(h0-1)&(PAIR-1);break; case 62:h0=(h0+1)&(PAIR-1);break;
      case 123:h1=(h1-1)&(PAIR-1);break; case 125:h1=(h1+1)&(PAIR-1);break;
      case 45:t[h0]--;break; case 43:t[h0]++;break;
      case 46:t[h1]=t[h0];break; case 44:t[h0]=t[h1];break;
      case 91: if(t[h0]==0){int d=1,p=ip+1;for(;p<PAIR&&d>0;p++){if(t[p]==93)d--;else if(t[p]==91)d++;}p--;if(d!=0){halted=1;break;}ip=p;} break;
      case 93: if(t[h0]!=0){int d=1,p=ip-1;for(;p>=0&&d>0;p--){if(t[p]==93)d++;else if(t[p]==91)d--;}p++;if(d!=0){halted=1;break;}ip=p;} break;
      default: break;
    }
    if(halted) break; ip++; steps++; if(ip<0||ip>=PAIR) break;
  }
}
static const int MF[6]={60,62,123,125,91,93}, MT[6]={62,60,125,123,93,91};
static uint8_t mswap(uint8_t b){for(int i=0;i<6;i++)if(b==MF[i])return MT[i];return b;}
static int self_copies(const uint8_t *P, uint64_t *rng){
  uint8_t cat[PAIR],R[TAPE],Pm[TAPE];
  for(int i=0;i<TAPE;i++)R[i]=sm64(rng)&0xFF;
  for(int i=0;i<TAPE;i++)Pm[i]=mswap(P[TAPE-1-i]);
  memcpy(cat,P,TAPE);memcpy(cat+TAPE,R,TAPE); evaluate(cat);
  for(int j=0;j+FUNC_L<=TAPE;j++){
    const uint8_t*w=cat+TAPE+j; int hadR=0;
    for(int k=0;k+FUNC_L<=TAPE;k++){if(!memcmp(w,R+k,FUNC_L)){hadR=1;break;}}
    if(hadR)continue;
    for(int s=0;s+FUNC_L<=TAPE;s++){if(!memcmp(w,P+s,FUNC_L))return 1;if(!memcmp(w,Pm+s,FUNC_L))return 1;}
  }
  return 0;
}

static uint8_t *soup; static size_t N; static uint32_t *idx;
static char f_tsv[512], f_ckpt[512], f_ckpt_tmp[512], f_done[512];

static int load_ckpt(int *epoch, uint64_t *gseed){
  FILE*f=fopen(f_ckpt,"rb"); if(!f) return 0;
  uint64_t ep,gs,n;
  if(fread(&ep,8,1,f)!=1||fread(&gs,8,1,f)!=1||fread(&n,8,1,f)!=1){fclose(f);return 0;}
  if(n!=N){fclose(f);return 0;}
  if(fread(soup,1,N*TAPE,f)!=N*TAPE){fclose(f);return 0;}
  fclose(f); *epoch=(int)ep; *gseed=gs; return 1;
}
// On resume, drop any metric rows written past the checkpoint epoch (they'll be
// recomputed as the run replays from the checkpoint), keeping the tsv clean.
static void truncate_tsv(int keep_epoch){
  FILE*f=fopen(f_tsv,"r"); if(!f) return;
  char tmp[520]; snprintf(tmp,520,"%s.trunc",f_tsv);
  FILE*o=fopen(tmp,"w"); if(!o){fclose(f);return;}
  char line[256];
  while(fgets(line,sizeof line,f)){
    if(line[0]=='#'){fputs(line,o);continue;}
    int ep; if(sscanf(line,"%d",&ep)==1 && ep<=keep_epoch) fputs(line,o);
  }
  fclose(f); fclose(o); rename(tmp,f_tsv);
}
static void save_ckpt(int epoch, uint64_t gseed){
  FILE*f=fopen(f_ckpt_tmp,"wb"); if(!f) return;
  uint64_t ep=epoch,gs=gseed,n=N;
  fwrite(&ep,8,1,f);fwrite(&gs,8,1,f);fwrite(&n,8,1,f);fwrite(soup,1,N*TAPE,f);
  fflush(f); fclose(f); rename(f_ckpt_tmp,f_ckpt);   // atomic replace
}
static void metrics(int epoch, uint64_t base){
  int SAMP=N<2000?(int)N:2000, hits=0;
  #pragma omp parallel reduction(+:hits)
  {
    uint64_t rng=0xABCDEFULL^(base*7)^(0x777ULL*(epoch+1))^(0x13ULL*(omp_get_thread_num()+1));
    #pragma omp for schedule(static)
    for(int s=0;s<SAMP;s++){size_t p=sm64(&rng)%N; if(self_copies(soup+p*TAPE,&rng))hits++;}
  }
  int ES=N<4096?(int)N:4096;
  static int*cnt=NULL; static size_t CB=1u<<24; if(!cnt)cnt=calloc(CB,sizeof(int));
  static uint32_t*touched=NULL; if(!touched)touched=malloc((size_t)4096*(TAPE-3)*sizeof(uint32_t));
  size_t M=0,nt=0; uint64_t rng2=0xFEEDULL^(base*3)^(0x99ULL*(epoch+1));
  for(int s=0;s<ES;s++){size_t p=sm64(&rng2)%N;const uint8_t*t=soup+p*TAPE;
    for(int j=0;j+4<=TAPE;j++){uint32_t key=((uint32_t)t[j]<<24)|((uint32_t)t[j+1]<<16)|((uint32_t)t[j+2]<<8)|t[j+3];
      size_t bk=key&(CB-1); if(cnt[bk]==0)touched[nt++]=bk; cnt[bk]++; M++;}}
  double H=0; for(size_t i=0;i<nt;i++){double pr=(double)cnt[touched[i]]/M;H-=pr*(log(pr)/log(2.0));cnt[touched[i]]=0;}
  double Hn=H/(log((double)M)/log(2.0));
  FILE*f=fopen(f_tsv,"a"); fprintf(f,"%d\t%.3f\t%.4f\n",epoch,100.0*hits/SAMP,Hn); fclose(f);
}

int main(int argc,char**argv){
  if(argc<7){fprintf(stderr,"usage: %s tag N epochs mut rngseed seedfrac\n",argv[0]);return 2;}
  const char*tag=argv[1];
  N=strtoull(argv[2],0,10); int EP=atoi(argv[3]);
  uint32_t mut=(uint32_t)strtoul(argv[4],0,10);
  uint64_t rngseed=strtoull(argv[5],0,10); double sf=atof(argv[6]);
  if(N&1)N--;
  snprintf(f_tsv,512,"%s.tsv",tag); snprintf(f_ckpt,512,"%s.ckpt",tag);
  snprintf(f_ckpt_tmp,512,"%s.ckpt.tmp",tag); snprintf(f_done,512,"%s.done",tag);

  FILE*d=fopen(f_done,"r"); if(d){fclose(d); fprintf(stderr,"[%s] already done\n",tag); return 0;}

  soup=malloc(N*TAPE); idx=malloc(N*sizeof(uint32_t));
  int epoch=0; uint64_t gseed=rngseed;
  if(load_ckpt(&epoch,&gseed)){
    truncate_tsv(epoch);
    fprintf(stderr,"[%s] resumed at epoch %d\n",tag,epoch);
  } else {
    uint64_t gs=rngseed; for(size_t i=0;i<N*TAPE;i++)soup[i]=sm64(&gs)&0xFF; gseed=gs;
    if(sf>0){const char*REP="[[{.>]-] ]-]>.{[[";int RL=strlen(REP);size_t ns=(size_t)(N*sf);
      for(size_t k=0;k<ns;k++){size_t p=sm64(&gseed)%N;for(int i=0;i<RL;i++)soup[p*TAPE+i]=(uint8_t)REP[i];}}
    FILE*f=fopen(f_tsv,"w"); fprintf(f,"# tag=%s N=%zu epochs=%d mut=%u rngseed=%llu seedfrac=%.3f\n# epoch\tselfcopy%%\tentropy4\n",tag,N,EP,mut,(unsigned long long)rngseed,sf); fclose(f);
    metrics(0,rngseed); epoch=0;
  }
  for(size_t i=0;i<N;i++)idx[i]=i;

  for(int e=epoch+1;e<=EP;e++){
    for(size_t i=N-1;i>0;i--){uint64_t r=sm64(&gseed);size_t j=r%(i+1);uint32_t t=idx[i];idx[i]=idx[j];idx[j]=t;}
    #pragma omp parallel
    {
      int tid=omp_get_thread_num();
      uint64_t rng=rngseed^(0x1234567ULL*(e+1))^(0x9abcdefULL*(tid+1));
      #pragma omp for schedule(static)
      for(size_t k=0;k<N/2;k++){
        uint32_t a=idx[2*k],b=idx[2*k+1]; uint8_t cat[PAIR];
        memcpy(cat,soup+(size_t)a*TAPE,TAPE); memcpy(cat+TAPE,soup+(size_t)b*TAPE,TAPE);
        evaluate(cat);
        for(int i=0;i<PAIR;i++){uint64_t rr=sm64(&rng);uint8_t rp=rr&0xFF;if(((rr>>8)&((1ULL<<30)-1))<mut)cat[i]=rp;}
        memcpy(soup+(size_t)a*TAPE,cat,TAPE); memcpy(soup+(size_t)b*TAPE,cat+TAPE,TAPE);
      }
    }
    if(e%METRIC_EVERY==0) metrics(e,rngseed);
    if(e%CKPT_EVERY==0) save_ckpt(e,gseed);
  }
  save_ckpt(EP,gseed);
  FILE*f=fopen(f_done,"w"); fprintf(f,"done %d\n",EP); fclose(f);
  fprintf(stderr,"[%s] DONE at epoch %d\n",tag,EP);
  return 0;
}
