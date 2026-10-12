/* Appended by test_pdm_asrc_admission.py to the production ownership bodies. */
static void initialise_host(void) {
    unsigned lane;
    memset(capture_stream,0,sizeof capture_stream);
    memset(capture_owner,0,sizeof capture_owner);
    memset(capture_metrics,0,sizeof capture_metrics);
    memset(&capture_asrc,0,sizeof capture_asrc);
    capture_stream_epoch=0;capture_rate_segment=0;
    capture_paired_slots=0;capture_discard_next_pair=0;
    capture_stale_active=0;capture_restart_requested=0;
    capture_ap_hops=0;capture_last_hop_us=0;
    capture_pair_epoch_drops=0;capture_pair_skew_drops=0;
    capture_asrc_push_drop=0;
    memset(&capture_service,0,sizeof capture_service);
    capture_poll_start_cycles=capture_poll_entry_filled=capture_poll_pairs=0;
    host_cycle_offset=0;host_cycle_step=0;
    k1_pdm_target_reset_logical_stream(0);
    host_time_us=1000000u;
    for(lane=0;lane<2;++lane) {
        assert(k1_pdm_stream_context_configure(&capture_stream[lane],296,296)==0);
        assert(k1_pdm_stream_context_start(&capture_stream[lane],host_time_us)==0);
    }
    capture_initialised=1;capture_running=1;
}
static void raw_pair(unsigned value) {
    unsigned lane,i;uint32_t slot;
    host_time_us+=7400;
    for(lane=0;lane<2;++lane) {
        assert(k1_pdm_stream_context_filling_slot(&capture_stream[lane],&slot)==0);
        for(i=0;i<296;++i) capture_buffer[lane][slot][i]=(int32_t)value;
        assert(k1_pdm_stream_context_on_data(&capture_stream[lane],296,host_time_us)==K1_PDM_STREAM_SLOT_READY);
    }
}
static void check_hop_order(void) {
    k1_audio_hop_t hop;unsigned i;
    assert(k1_pdm_target_try_read_ap_hop(&hop)==K1_AUDIO_READ_OK);
    for(i=0;i<180;++i) {
        uint64_t phase=hop.source_first_frac_q16+(uint64_t)i*hop.phase_increment_q16;
        uint64_t index=hop.source_first_index+(phase>>16);
        uint32_t frac=(uint32_t)(phase&65535u);
        int a=(int)(index/296u+1u)*16;
        int b=(int)((index+1u)/296u+1u)*16;
        int expected=(a*(int)(65536u-frac)+b*(int)frac+32768)>>16;
        assert(hop.pcm[i]==expected);
    }
}
static void normal_input_order(void) {
    unsigned i;initialise_host();
    for(i=1;i<=3;++i){raw_pair(i);k1_pdm_target_poll();}
    check_hop_order();check_hop_order();
    raw_pair(4);k1_pdm_target_poll();check_hop_order();
    raw_pair(5);k1_pdm_target_poll();check_hop_order();
    assert(capture_asrc.push_rejected==0 && capture_asrc.discarded_samples==0);
    assert(capture_asrc.consumed_samples+capture_asrc.filled==5*296);
    puts("normal: exact chronological PCM and metadata across DMA slots");
}
static void pressure_releases_dma_and_declares_loss(void) {
    unsigned i;k1_audio_hop_t hop;
    initialise_host();
    for(i=1;i<=3;++i){raw_pair(i);k1_pdm_target_poll();}
    assert(capture_asrc.filled==888);
    raw_pair(4);k1_pdm_target_poll();
    assert(!capture_owner[0].valid && !capture_owner[1].valid);
    assert(capture_stream[0].overflow_events==0 && capture_stream[1].overflow_events==0);
    assert(capture_asrc.push_rejected==1 && capture_asrc.discarded_samples==4*296);
    assert(capture_asrc.filled==0 && capture_meta_count==0 && capture_source_write_index==0);
    assert(capture_stream_epoch==2);
    assert(k1_pdm_target_try_read_ap_hop(&hop)==K1_AUDIO_READ_DISCONTINUITY);
    assert(hop.discontinuity_reason==2);
    raw_pair(5);k1_pdm_target_poll();raw_pair(6);k1_pdm_target_poll();
    assert(k1_pdm_target_try_read_ap_hop(&hop)==K1_AUDIO_READ_OK);
    assert(hop.stream_epoch==2 && hop.hop_sequence==1);
    assert(hop.pcm[0]==5*16 && hop.pcm[179]==6*16);
    for(i=7;i<=24;++i) {
        raw_pair(i);k1_pdm_target_poll();
        while(k1_pdm_target_try_read_ap_hop(&hop)==K1_AUDIO_READ_OK) {}
    }
    assert(capture_asrc.push_rejected==1 && capture_asrc.discarded_samples==4*296);
    assert(capture_stream[0].overflow_events==0 && capture_stream[1].overflow_events==0);
    assert(capture_stream[0].recovery_count==0 && capture_stream[1].recovery_count==0);
    puts("pressure: DMA released, actual rejected+queued loss counted, new epoch before fresh PCM");
}
static void rate_ignores_downstream_admission(void) {
    k1_pdm_owner_t owner={0};
    initialise_host();owner.sequence=42;owner.capture_end_us=1000000u;
    k1_pdm_target_observe_rate(&owner);
    k1_pdm_target_reset_logical_stream(2u);
    assert(capture_rate_t0_us==1000000u && capture_rate_first_sequence==42);
    owner.sequence=76;owner.capture_end_us=1242857u;
    k1_pdm_target_observe_rate(&owner);
    assert(capture_asrc.source_hz==40000 && !capture_rate_provisional);
    owner.sequence=77;owner.capture_end_us=1250000u;
    k1_pdm_target_observe_rate(&owner);
    assert(capture_rate_provisional && !capture_rate_locked);
    assert(capture_measured_hz==41440 && capture_asrc.source_hz==41440);
    assert(capture_rate_segment==1);
    k1_pdm_target_reset_logical_stream(2u);
    assert(capture_rate_provisional && !capture_rate_locked && capture_measured_hz==41440);
    assert(capture_rate_t0_us==1000000u && capture_rate_first_sequence==42);
    owner.sequence=180;owner.capture_end_us=1990000u;
    k1_pdm_target_observe_rate(&owner);
    assert(!capture_rate_locked && capture_rate_segment==1);
    capture_asrc.push_rejected=99;capture_metrics[0].processed_slots=187;
    owner.sequence=322;owner.capture_end_us=3000000u;
    k1_pdm_target_observe_rate(&owner);
    assert(capture_rate_locked && capture_measured_hz==41440 && capture_asrc.source_hz==41440);
    assert(capture_rate_segment==2);
    k1_pdm_target_reset_logical_stream(2u);
    assert(capture_rate_locked && capture_measured_hz==41440);
    initialise_host();owner.sequence=UINT32_MAX-100u;owner.capture_end_us=1000000u;
    k1_pdm_target_observe_rate(&owner);
    owner.sequence=179u;owner.capture_end_us=3000000u;
    k1_pdm_target_observe_rate(&owner);
    assert(capture_rate_locked && capture_measured_hz==41440);
    puts("rate: 250ms provisional, 2s refinement, raw sequence/time, no first-block bias, uint32 wrap");
}
static void metadata_loss_requires_discontinuity(void) {
    k1_audio_hop_t hop;initialise_host();
    raw_pair(1);k1_pdm_target_poll();
    capture_meta_count=K1_CAPTURE_META_CAP;
    raw_pair(2);k1_pdm_target_poll();
    assert(capture_stream_epoch==2 && capture_asrc.filled==0);
    assert(capture_asrc.discarded_samples==2*296 && capture_asrc_push_drop==1);
    assert(capture_meta_count==0 && !capture_owner[0].valid && !capture_owner[1].valid);
    assert(k1_pdm_target_try_read_ap_hop(&hop)==K1_AUDIO_READ_DISCONTINUITY);
    assert(hop.discontinuity_reason==2);
    puts("metadata fault: explicit loss, no silent concatenation");
}
int main(void) {
    normal_input_order();
    pressure_releases_dma_and_declares_loss();
    rate_ignores_downstream_admission();
    metadata_loss_requires_discontinuity();
    puts("K1_PDM_ASRC_ADMISSION=PASS");return 0;
}
