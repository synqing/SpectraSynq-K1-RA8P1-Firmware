"""Decode opcode17/HWST v1/v2. Register values are observations, not LED proof."""
import struct
FIELDS = ("version bytes cycle_begin cpu_hz gpt_hz mcuver fsblctrl0 sramwtsc vscr "
          "sckdivcr sckdivcr2 sckscr pllccr pll2ccr gptckcr ccr mpu_ctrl dmctl "
          "sram0_arbitration mstpcrc p601_pfs p604_pfs").split()
for lane in ("a", "b"):
    FIELDS += [f"gpt_{lane}_{name}" for name in
               "gtcr gtcnt gtpr gtber gtior gtpsr ccra ccrb ccrc ccre ccrd ccrf".split()]
for channel in range(4):
    FIELDS += [f"dma{channel}_{name}" for name in "src dest count mode enable buffer_write activation".split()]
FIELDS += [f"elc_{n}" for n in range(8)]
V1_FIELDS = FIELDS + ["cycle_end"]
FIELDS += ["pllccr2", "pll2ccr2", "gptckdivcr", "cycle_end"]

def decode(raw):
    if len(raw) < 8:
        raise ValueError("register snapshot length mismatch")
    version, size = struct.unpack_from("<2I", raw)
    fields = {1: V1_FIELDS, 2: FIELDS}.get(version)
    if fields is None or len(raw) != len(fields)*4:
        raise ValueError("register snapshot length mismatch")
    words=struct.unpack("<"+"I"*len(fields),raw)
    if size != len(raw):
        raise ValueError("register snapshot version/size mismatch")
    return dict(zip(fields,words))
