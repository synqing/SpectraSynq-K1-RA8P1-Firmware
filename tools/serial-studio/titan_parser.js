/**
 * Titan Mini host-snapshot parser. Schema 2.
 *
 * Missing measurements stay UNAVAILABLE. Validity flags never invent RMS,
 * rates, hop length or zero faults. JSON numbers only for numeric fields
 * (whitespace strings are not zero). Sequence is a string so uint64 IDs
 * survive JavaScript Number. Do not FFT these health fields.
 */
var SCHEMA = 2;
var UNAVAILABLE = "UNAVAILABLE";
var MAX_SAFE = 9007199254740991;
var STALE_MS = 2000;
var MODE_LABEL = [
  "DISCONNECTED",
  "IDENTIFYING",
  "READY",
  "LIVE",
  "TESTING",
  "COMPLETED",
  "FAULT",
  "REPLAY",
  "DEBUG HALTED",
  "STALE"
];
var RESULT_LABEL = ["NONE", "RUNNING", "PASSED", "FAILED", "NOT SUPPORTED"];
var lastValidMs = 0;
var lastSeq = "";
var sameSeq = 0;

function isBool01(v) {
  return v === true || v === false || v === 0 || v === 1;
}

function as01(v) {
  return v === true || v === 1 ? 1 : 0;
}

function isInt(v) {
  return typeof v === "number" && isFinite(v) && Math.floor(v) === v;
}

function isSafeInt(v) {
  return isInt(v) && v >= 0 && v <= MAX_SAFE;
}

function own(obj, key) {
  return !!(obj && Object.prototype.hasOwnProperty.call(obj, key));
}

function reqFlag(obj, key) {
  if (!own(obj, key) || !isBool01(obj[key]))
    return null;
  return as01(obj[key]);
}

function reqEnum(obj, key, max) {
  if (!own(obj, key) || !isInt(obj[key]) || obj[key] < 0 || obj[key] > max)
    return null;
  return obj[key];
}

function reqCount(obj, key) {
  if (!own(obj, key))
    return null;
  if (!isInt(obj[key]) || obj[key] < 0 || obj[key] > MAX_SAFE)
    return null;
  return obj[key];
}

function reqFinite(obj, key) {
  if (!own(obj, key))
    return null;
  var v = obj[key];
  if (typeof v !== "number" || !isFinite(v))
    return null;
  return v;
}

function reqString(obj, key) {
  if (!own(obj, key) || typeof obj[key] !== "string")
    return null;
  var s = obj[key].trim();
  return s === "" ? null : s;
}

function seqString(obj) {
  if (!own(obj, "sequence"))
    return null;
  var v = obj.sequence;
  if (typeof v === "string" && v.trim() !== "")
    return v.trim();
  if (isSafeInt(v))
    return String(v);
  return null;
}

function measured(obj, valid, key, kind) {
  if (!valid)
    return UNAVAILABLE;
  if (kind === "count")
    return reqCount(obj, key);
  if (kind === "finite")
    return reqFinite(obj, key);
  return null;
}

function frameText(frame) {
  if (typeof frame === "string")
    return frame;
  if (frame && typeof frame.length === "number") {
    var out = "";
    var n = frame.length;
    for (var i = 0; i < n; i++) {
      var b = frame[i];
      if (typeof b !== "number" || !isFinite(b))
        return "";
      out += String.fromCharCode(b & 255);
    }
    return out;
  }
  return "";
}

function parse(frame) {
  var s = frameText(frame).replace(/\r/g, "").trim();
  if (!s)
    return [];
  var obj;
  try {
    obj = JSON.parse(s);
  } catch (e) {
    if (s.indexOf("SIMULATED DATA") === 0 || s.indexOf("LIVE TITAN") === 0)
      return s.split(",");
    return [];
  }
  if (!obj || typeof obj !== "object" || Array.isArray(obj))
    return [];

  var schema = reqEnum(obj, "schema", 100);
  var mode = reqEnum(obj, "mode", 9);
  var identityOk = reqFlag(obj, "identity_ok");
  var protocol = reqEnum(obj, "protocol", 1);
  var simulated = reqFlag(obj, "simulated");
  var healthValid = reqFlag(obj, "health_valid");
  var micAValid = reqFlag(obj, "mic_a_valid");
  var micBValid = reqFlag(obj, "mic_b_valid");
  if (
    schema !== SCHEMA || mode === null || identityOk === null || protocol !== 1 ||
    simulated === null || healthValid === null || micAValid === null || micBValid === null
  )
    return [];

  var uid = reqString(obj, "uid");
  var build = reqString(obj, "build");
  var source = reqString(obj, "source");
  var contract = reqString(obj, "contract");
  if (identityOk) {
    if (!uid || !build || !source || !contract)
      return [];
  } else {
    uid = uid || "UNIDENTIFIED";
    build = build || "UNIDENTIFIED";
    source = source || "UNIDENTIFIED";
    contract = contract || "UNIDENTIFIED";
  }

  var hopMax = measured(obj, healthValid, "hop_max_us", "finite");
  var late = measured(obj, healthValid, "late_starts", "count");
  var deadlines = measured(obj, healthValid, "deadlines", "count");
  var crc = measured(obj, healthValid, "crc_mismatches", "count");
  var dma = measured(obj, healthValid, "dma_irqs", "count");
  var latched = measured(obj, healthValid, "latched_frames", "count");
  var ledFaults = measured(obj, healthValid, "led_faults", "count");
  if (
    hopMax === null || late === null || deadlines === null || crc === null ||
    dma === null || latched === null || ledFaults === null
  )
    return [];
  if (typeof hopMax === "number" && hopMax < 0)
    return [];

  var rmsA = measured(obj, micAValid, "mic_a_rms_dbfs", "finite");
  var rmsB = measured(obj, micBValid, "mic_b_rms_dbfs", "finite");
  if (rmsA === null || rmsB === null)
    return [];

  var rate = measured(obj, healthValid, "capture_rate_hz", "finite");
  if (rate === null)
    return [];
  if (typeof rate === "number" && rate <= 0)
    return [];

  var hopSamples = measured(obj, healthValid, "hop_samples", "count");
  var admitted = measured(obj, healthValid, "admitted_rate_hz", "finite");
  if (hopSamples === null || admitted === null)
    return [];
  if (typeof hopSamples === "number" && hopSamples <= 0)
    return [];
  if (typeof admitted === "number" && admitted <= 0)
    return [];

  var gateResult = own(obj, "gate_result") ? reqEnum(obj, "gate_result", 4) : 0;
  if (gateResult === null)
    return [];

  var sequence = seqString(obj);
  if (!sequence)
    return [];

  if (sequence === lastSeq)
    sameSeq += 1;
  else {
    sameSeq = 0;
    lastSeq = sequence;
  }

  var now = Date.now();
  lastValidMs = now;
  try {
    if (typeof tableSet === "function") {
      tableSet("titan_watchdog", "last_valid_ms", now);
      tableSet("titan_watchdog", "host_stale", 0);
    }
  } catch (err) {}

  var frozen = sameSeq >= 8 ? 1 : 0;
  var ledHealth = healthValid
    ? (ledFaults > 0 ? "FAULT" : "OK")
    : UNAVAILABLE;
  var banner = simulated
    ? "SIMULATED DATA — TITAN NOT CONNECTED"
    : "LIVE TITAN";
  if (frozen)
    banner = banner + " / FROZEN SEQUENCE";

  return [
    banner,
    MODE_LABEL[mode],
    RESULT_LABEL[gateResult],
    uid,
    build,
    source,
    contract,
    identityOk,
    rmsA,
    rmsB,
    micAValid,
    micBValid,
    rate,
    hopMax,
    late,
    deadlines,
    crc,
    ledHealth,
    dma,
    latched,
    ledFaults,
    hopSamples,
    admitted,
    sequence,
    healthValid,
    simulated,
    "Lane A / Lane B — U13/U14 mapping unresolved"
  ];
}
