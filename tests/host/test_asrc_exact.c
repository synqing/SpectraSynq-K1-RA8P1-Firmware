#include "k1_asrc_24k.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void oracle_reset(k1_asrc24_t *, uint32_t, uint32_t);
int oracle_set_rate(k1_asrc24_t *, uint32_t);
int oracle_push(k1_asrc24_t *, const int16_t *, uint32_t);
int oracle_pull180(k1_asrc24_t *, int16_t *);
static uint32_t rng = 0x41535243u;
static int16_t random_pcm(void) {
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return (int16_t)((int32_t)(rng & 65535u) - 32768);
}
static void equal(const k1_asrc24_t *a, const k1_asrc24_t *b) {
    assert(memcmp(a, b, sizeof(*a)) == 0);
}
static void pull(k1_asrc24_t *a, k1_asrc24_t *b) {
    int16_t x[K1_ASRC24_OUT_FRAMES], y[K1_ASRC24_OUT_FRAMES];
    memset(x, 0x56, sizeof(x)); memset(y, 0x56, sizeof(y));
    assert(k1_asrc24_pull180(a,x) == oracle_pull180(b,y));
    assert(memcmp(x,y,sizeof(x)) == 0); equal(a,b);
}
int main(void) {
    const uint32_t rates[] = {36000,40000,41441,44000};
    unsigned ri;
#ifdef K1_EMOTISCOPE_PORT
    for (ri=0; ri<4; ++ri) {
        for (uint32_t cursor=0; cursor<95; ++cursor) {
            for (unsigned kind=0;kind<5;++kind) {
                k1_asrc24_t a,b;
                k1_asrc24_reset(&a,7,rates[ri]); oracle_reset(&b,7,rates[ri]); equal(&a,&b);
                a.filter_write_pos=cursor;
                for(unsigned j=0;j<95;++j)
                    a.filter_history[j]=kind==0 ? INT16_MIN : kind==1 ? INT16_MAX :
                        kind==2 ? (j==37 ? INT16_MAX : 0) : kind==3 ? (j%2 ? INT16_MIN : INT16_MAX) : random_pcm();
                b=a;
                int16_t sample=kind==0 ? INT16_MIN : kind==1 ? INT16_MAX : random_pcm();
                assert(k1_asrc24_push(&a,&sample,1)==oracle_push(&b,&sample,1)); equal(&a,&b);
            }
        }
    }
#endif
    for (ri=0;ri<4;++ri) {
        k1_asrc24_t a,b;
        k1_asrc24_reset(&a,17,rates[ri]); oracle_reset(&b,17,rates[ri]); equal(&a,&b);
        a.write_pos=a.read_pos=900; b=a;
        for(unsigned chunk=0;chunk<1200;++chunk) {
            int16_t samples[296];
            uint32_t n=1u+(rng%296u);
            for(uint32_t j=0;j<n;++j) samples[j]=chunk<10 ? INT16_MIN : chunk<20 ? INT16_MAX :
                chunk<30 ? (j==0 ? INT16_MAX : 0) : random_pcm();
            while(K1_ASRC24_RING-a.filled<n) pull(&a,&b);
            /* Batch versus one-sample pushes also proves fragmentation invariance. */
            assert(k1_asrc24_push(&a,samples,n)==0);
            for(uint32_t j=0;j<n;++j) assert(oracle_push(&b,samples+j,1)==0);
            equal(&a,&b);
            if(chunk%3==0) pull(&a,&b);
            if(chunk%137==0) {
                uint32_t rate=rates[(ri+chunk/137)%4];
                assert(k1_asrc24_set_rate(&a,rate)==oracle_set_rate(&b,rate)); equal(&a,&b);
            }
        }
        while(a.filled>=k1_asrc24_required_for_pull180(&a)) pull(&a,&b);
        /* Rejected pushes and invalid rate updates retain identical accounting. */
        assert(k1_asrc24_push(&a,0,1)==oracle_push(&b,0,1)); equal(&a,&b);
        assert(k1_asrc24_set_rate(&a,35000)==oracle_set_rate(&b,35000)); equal(&a,&b);
        int16_t too_many[K1_ASRC24_RING+1]={0};
        assert(k1_asrc24_push(&a,too_many,K1_ASRC24_RING+1)==oracle_push(&b,too_many,K1_ASRC24_RING+1)); equal(&a,&b);
    }
    puts("exact ASRC parity: all cursor positions, extrema/impulse/random, four rates, rate updates, fragmentation, ring wrap and rejected admission");
}
