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
        # Finish once the gesture has started and the mixer then goes quiet for
        # 1.5 s. The controller validates identity and sweep on the full raw capture.
        start, last_events, last_change = time.time(), 0, None
        minimum = 3 if kind == "absolute" else 2
        while time.time() - start < 110:
            events = (get("/api/status").get("qualification") or {}).get("events") or 0
            if events != last_events:
                last_events, last_change = events, time.time()
            if last_change and events >= minimum and time.time() - last_change >= 1.5:
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
