#include "pdm_capture.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 k1_pdm_stream_t a,b;
 uint32_t slot,epoch,sequence;uint64_t start,end;
 assert(k1_pdm_stream_context_configure(&a,296,296)==0);
 assert(k1_pdm_stream_context_configure(&b,296,296)==0);
 assert(k1_pdm_stream_context_start(&a,0)==0);
 assert(k1_pdm_stream_context_start(&b,0)==0);
 /* Hardware witness: servicing can take 12.259 ms while blocks arrive every
    7.4 ms. Two completions can arrive before the consumer next runs. */
 assert(k1_pdm_stream_context_on_data(&a,296,100)==1);
 assert(k1_pdm_stream_context_on_data(&a,296,7500)==1);
 assert(k1_pdm_stream_context_on_data(&b,296,110)==1);
 /* A one-sided extra completion must not poison every later paired epoch. */
 k1_pdm_stream_context_stop(&a);k1_pdm_stream_context_stop(&b);
 assert(k1_pdm_stream_context_recover(&a,13000)==0);
 assert(k1_pdm_stream_context_recover(&b,13000)==0);
 assert(k1_pdm_stream_context_on_data(&a,296,20400)==1);
 assert(k1_pdm_stream_context_on_data(&b,296,20410)==1);
 assert(a.next_sequence==b.next_sequence);
 assert(a.epoch==b.epoch && a.epoch==2);
 assert(k1_pdm_stream_context_acquire(&a,&slot,&epoch,&sequence,&start,&end)==0);
 assert(k1_pdm_stream_context_release(&a,slot,epoch-1,sequence)==K1_PDM_STREAM_STALE_OWNER);
 assert(k1_pdm_stream_context_release(&a,slot,epoch,sequence)==0);
 puts("PDM bounded headroom and paired recovery passed");
}
