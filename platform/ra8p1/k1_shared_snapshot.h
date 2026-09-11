#pragma once

#include "k1_crc32.h"
#include "titan_ram_diag.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Latest-state renderer snapshot. Silent drop is allowed here; it is
   forbidden for the 12.8 kHz / 96-sample audio path.

   Publication order (producer, single writer):
     1) claim slot gen odd (in-flight)
     2) write body fields except crc
     3) crc-last
     4) gen even (stable) then write_seq++
   Target cache: CleanDCache the slot after step 4, or map the ring
   non-cacheable. A DSB/memory barrier alone is not M85 D-cache coherence.

   Consumer: InvalidateDCache the slot, copy, require gen even and unchanged,
   crc match, and body.sequence == intended. On mismatch, bounded retry of
   the newest stable slot (write_seq-1), then drop. Never return a payload
   whose sequence is not the one committed. */

#define K1_SNAP_MAGIC 0x4B315348u /* K1SH */
#define K1_SNAP_VERSION 2u
#define K1_SNAP_SLOTS 8u
#define K1_SNAP_RETRY_BOUND 2u

typedef struct {
    uint32_t sequence;
    uint32_t timestamp_us;
    uint32_t heartbeat;
    uint32_t valid_mask;
    uint32_t error_flags;
    uint32_t vp_frame_id;
    uint32_t output_hash;
    uint32_t dma_complete_cycles;
    uint32_t session_seq;
    uint32_t reserved[2];
    uint32_t crc32;
} k1_latest_snapshot_t;

#if defined(__cplusplus)
static_assert(sizeof(k1_latest_snapshot_t) == 48u, "snapshot ABI");
#else
_Static_assert(sizeof(k1_latest_snapshot_t) == 48u, "snapshot ABI");
#endif

typedef struct {
    volatile uint32_t gen;
    k1_latest_snapshot_t body;
} k1_shared_slot_t;

typedef struct {
    uint32_t write_observed;
    uint32_t intended;
    uint32_t prepared;
} k1_shared_ticket_t;

typedef struct {
    volatile uint32_t magic;
    volatile uint32_t version;
    volatile uint32_t slot_count;
    volatile uint32_t write_seq;
    volatile uint32_t read_seq;
    volatile uint32_t crc_fail;
    volatile uint32_t overrun;
    volatile uint32_t race_drop;
    volatile uint32_t consumed_seq;
    volatile uint32_t last_heartbeat;
    volatile uint32_t producer_state;
    k1_shared_slot_t slots[K1_SNAP_SLOTS];
} k1_shared_ctrl_t;

void k1_shared_init(k1_shared_ctrl_t *ctrl);
uint32_t k1_snapshot_crc(const k1_latest_snapshot_t *snap);
void k1_shared_publish(k1_shared_ctrl_t *ctrl, const k1_latest_snapshot_t *body);
int k1_shared_consume_prepare(k1_shared_ctrl_t *ctrl, k1_shared_ticket_t *ticket);
int k1_shared_consume_finish(k1_shared_ctrl_t *ctrl, k1_ram_diag_t *diag,
                             k1_shared_ticket_t *ticket,
                             k1_latest_snapshot_t *out);
int k1_shared_consume(k1_shared_ctrl_t *ctrl, k1_ram_diag_t *diag,
                      k1_latest_snapshot_t *out);

#ifdef __cplusplus
}
#endif
