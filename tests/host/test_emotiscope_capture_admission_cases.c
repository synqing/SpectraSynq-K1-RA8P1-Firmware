static void contract_and_timestamps(void) {
    k1_audio_hop_t hop;
    uint64_t last_epoch=0, last_sequence=0;
    uint32_t blocks=0;
    initialise_host();
    for (unsigned pair=1; pair <= 100; ++pair) {
        raw_pair(pair);
        k1_pdm_target_poll();
        while (k1_pdm_target_try_read_ap_hop(&hop)==K1_AUDIO_READ_OK) {
            blocks++;
            assert(hop.contract_version==2 && hop.samples==64);
            assert(hop.analysis_rate_hz==12800);
            assert(hop.hop_sequence==blocks && hop.stream_epoch==1);
            assert(hop.analysis_begin==(uint64_t)(blocks-1u)*64u);
            assert(hop.analysis_end_exclusive==(uint64_t)blocks*64u);
            assert(hop.analysis_end_exclusive*15u/4u==(uint64_t)blocks*240u);
            assert(hop.source_first_index==
                ((uint64_t)(blocks-1u)*64u*hop.phase_increment_q16>>16));
            assert(hop.source_last_index==
                (((uint64_t)(blocks-1u)*64u+63u)*hop.phase_increment_q16>>16));
            assert(hop.source_support_last_index==hop.source_last_index+1u);
            assert(hop.source_block_last_sequence>=hop.source_block_first_sequence);
            assert(hop.capture_estimate_valid && !hop.physical_timestamp_valid);
            uint64_t delta=(hop.supporting_block_source_end_exclusive-1u-
                hop.source_last_index+47u)*65536u-hop.source_last_frac_q16;
            uint64_t delay=delta*1000000u/(65536u*hop.source_rate_hz);
            assert(hop.newest_sample_capture_estimate_us==
                   hop.supporting_dma_receipt_us-delay);
            assert(k1_audio_hop_accept(&last_epoch,&last_sequence,&hop)==-1);
            for (unsigned i=64; i<180; ++i) assert(hop.pcm[i]==0);
        }
        assert(capture_asrc.consumed_samples+capture_asrc.filled==pair*296u);
        assert(capture_asrc.discarded_samples==0 && capture_asrc.push_rejected==0);
    }
    assert(blocks==148u);
    puts("contract:64/12800,v2,sequential ranges,source support,FIR timestamp,legacy rejects");
}
static void port_pressure_and_recovery(void) {
    k1_audio_hop_t hop;
    initialise_host();
    for(unsigned i=1;i<=4;++i){raw_pair(i);k1_pdm_target_poll();}
    assert(capture_asrc.push_rejected==1 && capture_asrc.discarded_samples==1184);
    assert(capture_stream_epoch==2 && capture_asrc.filled==0);
    assert(capture_asrc.filter_write_pos==0);
    for(unsigned i=0;i<K1_ASRC24_FILTER_TAPS;++i)
        assert(capture_asrc.filter_history[i]==0);
    assert(k1_pdm_target_try_read_ap_hop(&hop)==K1_AUDIO_READ_DISCONTINUITY);
    assert(hop.discontinuity_reason==2);
    raw_pair(5);k1_pdm_target_poll();
    assert(k1_pdm_target_try_read_ap_hop(&hop)==K1_AUDIO_READ_OK);
    assert(hop.stream_epoch==2 && hop.hop_sequence==1 && hop.samples==64);
    assert(capture_stream[0].recovery_count==0 && capture_stream[1].recovery_count==0);
    assert(!capture_owner[0].valid && !capture_owner[1].valid);
    puts("pressure:explicit rejected/queued loss,new epoch,cleared FIR,raw DMA ownership preserved");
}
int main(void) {
    contract_and_timestamps();
    port_pressure_and_recovery();
    rate_ignores_downstream_admission();
    metadata_loss_requires_discontinuity();
    puts("EMOTISCOPE_CAPTURE_ADMISSION=PASS");
    return 0;
}
