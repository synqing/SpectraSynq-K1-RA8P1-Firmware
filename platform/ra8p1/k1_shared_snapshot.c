#include "k1_shared_snapshot.h"

#include <string.h>

static void barrier(void)
{
    __sync_synchronize();
}

void k1_shared_init(k1_shared_ctrl_t *ctrl)
{
    if (ctrl == 0) {
        return;
    }
    memset((void *)ctrl, 0, sizeof(*ctrl));
    ctrl->magic = K1_SNAP_MAGIC;
    ctrl->version = K1_SNAP_VERSION;
    ctrl->slot_count = K1_SNAP_SLOTS;
}

uint32_t k1_snapshot_crc(const k1_latest_snapshot_t *snap)
{
    if (snap == 0) {
        return 0u;
    }
    return k1_crc32_ieee((const uint8_t *)snap,
                         (uint32_t)((const uint8_t *)&snap->crc32 -
                                    (const uint8_t *)snap));
}

void k1_shared_publish(k1_shared_ctrl_t *ctrl, const k1_latest_snapshot_t *body)
{
    uint32_t seq;
    k1_shared_slot_t *slot;
    if (ctrl == 0 || body == 0 || ctrl->magic != K1_SNAP_MAGIC) {
        return;
    }
    seq = ctrl->write_seq;
    slot = &ctrl->slots[seq % K1_SNAP_SLOTS];
    ctrl->producer_state = 1u;
    slot->gen = slot->gen + 1u;
    barrier();
    slot->body = *body;
    slot->body.sequence = seq;
    slot->body.crc32 = k1_snapshot_crc(&slot->body);
    barrier();
    slot->gen = slot->gen + 1u;
    ctrl->last_heartbeat = body->heartbeat;
    barrier();
    ctrl->write_seq = seq + 1u;
    ctrl->producer_state = 0u;
}

int k1_shared_consume_prepare(k1_shared_ctrl_t *ctrl, k1_shared_ticket_t *ticket)
{
    uint32_t write;
    uint32_t read;
    uint32_t lag;
    if (ctrl == 0 || ticket == 0 || ctrl->magic != K1_SNAP_MAGIC) {
        return 0;
    }
    memset(ticket, 0, sizeof(*ticket));
    write = ctrl->write_seq;
    read = ctrl->read_seq;
    if (write == read) {
        return 0;
    }
    lag = write - read;
    if (lag > K1_SNAP_SLOTS) {
        uint32_t drop = lag - K1_SNAP_SLOTS;
        ctrl->overrun += drop;
        read = write - K1_SNAP_SLOTS;
        ctrl->read_seq = read;
    }
    ticket->write_observed = write;
    ticket->intended = read;
    ticket->prepared = 1u;
    return 1;
}

enum {
    K1_SNAP_COPY_OK = 1,
    K1_SNAP_COPY_GEN = -1,
    K1_SNAP_COPY_CRC = -2,
    K1_SNAP_COPY_SEQ = -3
};

static int copy_stable(const k1_shared_slot_t *slot, uint32_t intended,
                       k1_latest_snapshot_t *out)
{
    uint32_t g1;
    uint32_t g2;
    k1_latest_snapshot_t body;
    if (slot == 0 || out == 0) {
        return K1_SNAP_COPY_GEN;
    }
    g1 = slot->gen;
    barrier();
    body = slot->body;
    barrier();
    g2 = slot->gen;
    if ((g1 & 1u) != 0u || g1 != g2) {
        return K1_SNAP_COPY_GEN;
    }
    if (k1_snapshot_crc(&body) != body.crc32) {
        *out = body;
        return K1_SNAP_COPY_CRC;
    }
    if (body.sequence != intended) {
        *out = body;
        return K1_SNAP_COPY_SEQ;
    }
    *out = body;
    return K1_SNAP_COPY_OK;
}

int k1_shared_consume_finish(k1_shared_ctrl_t *ctrl, k1_ram_diag_t *diag,
                             k1_shared_ticket_t *ticket,
                             k1_latest_snapshot_t *out)
{
    uint32_t attempt;
    uint32_t intended;
    k1_latest_snapshot_t scratch;
    if (ctrl == 0 || ticket == 0 || out == 0 || ticket->prepared == 0u ||
        ctrl->magic != K1_SNAP_MAGIC) {
        return 0;
    }
    intended = ticket->intended;
    for (attempt = 0; attempt < K1_SNAP_RETRY_BOUND; ++attempt) {
        const k1_shared_slot_t *slot = &ctrl->slots[intended % K1_SNAP_SLOTS];
        int copied = copy_stable(slot, intended, &scratch);
        if (copied == K1_SNAP_COPY_OK) {
            *out = scratch;
            ctrl->read_seq = intended + 1u;
            ctrl->consumed_seq = out->sequence;
            ticket->prepared = 0u;
            return 1;
        }
        if (copied == K1_SNAP_COPY_CRC) {
            ctrl->crc_fail += 1u;
            k1_ram_diag_note_crc_fail(diag, scratch.timestamp_us);
        } else {
            ctrl->race_drop += 1u;
        }
        {
            uint32_t write = ctrl->write_seq;
            if (write == 0u) {
                break;
            }
            intended = write - 1u;
        }
    }
    ctrl->read_seq = ctrl->write_seq;
    ticket->prepared = 0u;
    return 0;
}

int k1_shared_consume(k1_shared_ctrl_t *ctrl, k1_ram_diag_t *diag,
                      k1_latest_snapshot_t *out)
{
    k1_shared_ticket_t ticket;
    if (!k1_shared_consume_prepare(ctrl, &ticket)) {
        return 0;
    }
    return k1_shared_consume_finish(ctrl, diag, &ticket, out);
}
