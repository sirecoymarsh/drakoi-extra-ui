/* Audio-content anchors; no PC elapsed-time clock. Shared with the headless test. */
#ifndef VOCAL_SYNC_H
#define VOCAL_SYNC_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define VS_TRACK_MAX 32
#define VS_MATCH_MAX 4
typedef struct VS_ROW {uint32_t hash,frame,track;int16_t values[8];} VS_ROW;
typedef struct VS_INDEX {uint32_t tracks,count,frames[VS_TRACK_MAX];VS_ROW *rows;uint32_t buckets[65537];} VS_INDEX;
typedef struct VS_MATCH {uint32_t track,frame,hits,wrapHits;} VS_MATCH;
typedef struct VS_SPAN {uint32_t offset,bytes,count;VS_MATCH matches[VS_MATCH_MAX];} VS_SPAN;
typedef struct VS_RING {uint32_t bytes;VS_SPAN spans[8];} VS_RING;
static const unsigned vs_offsets[8]={0,34,126,258,510,714,906,1022};
static int vs_load(VS_INDEX *ix,FILE *f) {
    char magic[8];uint32_t i;VS_ROW *rows;
    memset(ix,0,sizeof(*ix));
    if(fread(magic,1,8,f)!=8||memcmp(magic,"DKVF0001",8))return 0;
    if(fread(&ix->tracks,4,1,f)!=1||fread(&ix->count,4,1,f)!=1||!ix->tracks||ix->tracks>VS_TRACK_MAX||!ix->count||ix->count>1000000)return 0;
    if(fread(ix->frames,4,ix->tracks,f)!=ix->tracks)return 0;
    for(i=0;i<ix->tracks;i++)if(!ix->frames[i]||ix->frames[i]>44100u*3600)return 0;
    rows=(VS_ROW*)malloc(sizeof(VS_ROW)*ix->count);if(!rows)return 0;
    if(fread(rows,sizeof(VS_ROW),ix->count,f)!=ix->count||fgetc(f)!=EOF){free(rows);return 0;}
    for(i=0;i<ix->count;i++)if(rows[i].track>=ix->tracks||rows[i].frame>=ix->frames[rows[i].track]||(i&&rows[i].hash<rows[i-1].hash)){free(rows);return 0;}
    ix->rows=rows;{uint32_t bucket,at=0;for(bucket=0;bucket<=65536;bucket++){while(at<ix->count&&(rows[at].hash>>16)<bucket)at++;ix->buckets[bucket]=at;}}return 1;
}
static uint32_t vs_hash(const int16_t *p) {
    uint32_t h=2166136261u;unsigned k;
    for(k=0;k<8;k++)h=(h^((uint16_t)p[vs_offsets[k]]>>5))*16777619u;
    return h;
}
static unsigned vs_match(const VS_INDEX *ix,const int16_t *pcm,uint32_t frames,VS_MATCH out[VS_MATCH_MAX]) {
    VS_MATCH candidates[32];unsigned n=0,k,m,j,result=0,best=0;uint32_t pos,limit;
    if(!ix->rows||frames<2560)return 0;
    limit=frames-512;
    memset(candidates,0,sizeof(candidates));
    for(pos=0;pos<limit;pos++){
        const int16_t *p=pcm+pos*2;uint32_t hash=vs_hash(p),lo=ix->buckets[hash>>16],hi=ix->buckets[(hash>>16)+1];
        while(lo<hi){uint32_t mid=lo+(hi-lo)/2;if(ix->rows[mid].hash<hash)lo=mid+1;else hi=mid;}
        for(;lo<ix->count&&ix->rows[lo].hash==hash;lo++){
            const VS_ROW*r=ix->rows+lo;uint32_t start;int sum=0,delta;
            for(k=0;k<8;k++){delta=(int)p[vs_offsets[k]]-r->values[k];if(delta<0)delta=-delta;if(delta>4)break;sum+=delta;}
            if(k!=8||sum>16)continue;
            start=(r->frame+ix->frames[r->track]-(pos%ix->frames[r->track]))%ix->frames[r->track];
            for(m=0;m<n;m++)if(candidates[m].track==r->track&&candidates[m].frame==start)break;
            if(m==n){if(n==32)return 0;candidates[n].track=r->track;candidates[n].frame=start;candidates[n].hits=0;n++;}
            candidates[m].hits++;if(start+pos>=ix->frames[r->track])candidates[m].wrapHits++;
        }
    }
    for(j=0;j<n;j++)if(candidates[j].hits>best)best=candidates[j].hits;
    for(j=0;j<n;j++)if(candidates[j].hits>=2&&candidates[j].hits==best){if(result==VS_MATCH_MAX)return 0;out[result++]=candidates[j];}
    return result;
}
/* Invalidate every overlapping committed span before Lock exposes writable memory. */
static void vs_invalidate(VS_RING *ring,uint32_t offset,uint32_t bytes) {
    VS_SPAN keep[8];unsigned i,k,n=0;uint32_t finish;
    if(!bytes)return;finish=offset+bytes;
    memset(keep,0,sizeof(keep));
    for(i=0;i<8;i++){
        VS_SPAN*s=ring->spans+i;uint32_t end=s->offset+s->bytes;
        if(!s->bytes)continue;
        if(offset>=end||s->offset>=finish){if(n<8)keep[n++]=*s;continue;}
        if(s->offset<offset&&n<8){keep[n]=*s;keep[n].bytes=offset-s->offset;n++;}
        if(end>finish&&n<8){
            keep[n]=*s;keep[n].offset=finish;keep[n].bytes=end-finish;
            for(k=0;k<s->count;k++)keep[n].matches[k].frame+=(finish-s->offset)/4;
            n++;
        }
    }
    memcpy(ring->spans,keep,sizeof(keep));
}
static void vs_commit(VS_RING *ring,uint32_t offset,uint32_t bytes,const VS_MATCH*m,unsigned count) {
    unsigned i;
    vs_invalidate(ring,offset,bytes);
    if(!count||count>VS_MATCH_MAX||offset>ring->bytes||bytes>ring->bytes-offset)return;
    for(i=0;i<8;i++)if(!ring->spans[i].bytes){VS_SPAN*s=ring->spans+i;s->offset=offset;s->bytes=bytes;s->count=count;memcpy(s->matches,m,count*sizeof(*m));return;}
}
static unsigned vs_at(const VS_INDEX*ix,const VS_RING*ring,uint32_t cursor,VS_MATCH*out) {
    unsigned i,k;if(cursor>=ring->bytes)return 0;
    for(i=0;i<8;i++){const VS_SPAN*s=ring->spans+i;
        if(s->bytes&&cursor>=s->offset&&cursor-s->offset<s->bytes){
            for(k=0;k<s->count;k++){out[k]=s->matches[k];out[k].frame+=(cursor-s->offset)/4;if(out[k].frame>=ix->frames[out[k].track]){if(out[k].wrapHits<2)return 0;out[k].frame%=ix->frames[out[k].track];}}
            return s->count;
        }
    }return 0;
}
#endif

