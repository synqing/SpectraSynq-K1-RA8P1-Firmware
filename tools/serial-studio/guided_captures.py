#!/usr/bin/env python3
"""Guided labelled captures through the live dashboard API (no MIDI/CDC access).

Arms each label in order, watches the controller's recent MIDI for the complete
gesture on that label's exact channel/CC (absolute: <=16 and >=111; button: 127
and 0), then finishes. The controller validates the full raw capture itself; a
failed capture is re-armed (3 tries). Prints one line per event.
usage: guided_captures.py [LABEL ...]   default: Fader1 Fader4 R5 R8 Record
"""
import json, sys, time, urllib.request

B = "http://127.0.0.1:8776"
H = {"Host": "127.0.0.1:8776"}
RULES = {"Fader1": (0, 40, "absolute"), "Fader4": (0, 43, "absolute"),
         "R5": (1, 40, "button"), "R8": (1, 43, "button"), "Record": (0, 54, "button")}


def get(path):
    return json.load(urllib.request.urlopen(urllib.request.Request(B + path, headers=H), timeout=3))


def post(path, body, token):
    req = urllib.request.Request(B + path, data=json.dumps(body).encode(), method="POST",
                                 headers={**H, "Content-Type": "application/json", "X-SMC-Token": token})
    try:
        return json.load(urllib.request.urlopen(req, timeout=5))
    except urllib.error.HTTPError as exc:
        return json.load(exc)


def say(msg):
    print(time.strftime("%T"), msg, flush=True)


def run(label, token):
    ch, cc, kind = RULES[label]
    for attempt in range(1, 4):
        post("/api/qualification", {"action": "begin", "control": label}, token)
        say(f"ARMED {label} (try {attempt}) - move only {label} now")
        values, start = set(), time.time()
        seen_before = {json.dumps(m) for m in get("/api/status").get("recent") or []}
        while time.time() - start < 110:
            for m in get("/api/status").get("recent") or []:
                key = json.dumps(m)
                if key in seen_before:
                    continue
                seen_before.add(key)
                if (m["channel"], m["cc"]) == (ch, cc):
                    values.add(m["value"])
            done = (min(values) <= 16 and max(values) >= 111) if kind == "absolute" and values \
                else {0, 127} <= values
            if done:
                time.sleep(0.6)  # let the tail of the gesture land
                break
            time.sleep(0.15)
        result = post("/api/qualification", {"action": "finish"}, token)
        say(f"{label}: {'PASS' if result.get('ok') else 'FAIL'} - {result.get('reason')} "
            f"events={result.get('events')} observed={result.get('observed')}")
        if result.get("ok"):
            return True
    return False


def main():
    labels = sys.argv[1:] or ["Fader1", "Fader4", "R5", "R8", "Record"]
    token = get("/api/session")["token"]
    results = {label: run(label, token) for label in labels}
    status = get("/api/status")
    say(f"SUMMARY {results} qualified={status['qualification']['qualified']} faders_armed={status['faders_armed']}")


if __name__ == "__main__":
    main()
