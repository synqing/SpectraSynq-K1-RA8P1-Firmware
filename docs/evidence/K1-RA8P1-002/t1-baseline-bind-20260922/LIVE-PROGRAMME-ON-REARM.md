# Live programme command — prepared, not armed

Do not run this until Captain says **Rearm**, the Titan application USB port is back, and the P603 wire move has been confirmed with power off.

The saved file `ws2816-pair-candidate-20260921-a/PROGRAMME-COMMAND.sh` is a dry run. It omits `--execute`. Do not run that saved file against `programme-ws2816-pair-20260921-01`. A dry run creates the output directory, and the programmer then refuses to use it.

The armed command, only after Rearm, is the source-level image. The older no-symbol candidate stays on disk and is not this command.

```sh
python3 scripts/programme_scalar.py \
  --build /Users/spectrasynq/Workspace_Management/Software/SpectraSynq-K1-RA8P1-Firmware/docs/evidence/K1-RA8P1-002/ws2816-pair-symbols-20260922 \
  --output /Users/spectrasynq/Workspace_Management/EdgeAI_Artifacts/Titan/k1-ra8p1-002/programme-ws2816-pair-symbols-20260922-01 \
  --execute
```

Start that command first. Reply `WAITING` only after `events.jsonl` shows `WAITING_FOR_IDENTIFIED_ROM`. Watching the events file does not arm the programmer.

Events, in order: `CDC_EXCLUSIVE`, `WAITING_FOR_IDENTIFIED_ROM`, `ROM_SEEN`, `ROM_IDENTIFIED_WRITING`, `WRITE_VERIFIED`, `APP_CDC_SEEN`, `APP_IDENTITY`. Failure event: `PROGRAMME_FAILED`. There is no `PROGRAMME_VERIFY_PASS` event.

After `WRITE_VERIFIED`, check the application independently. The programmer now checks both the board id `545433931bd25436593630352d068363` and the build. For this image the build must be `e5d51385318c6ee75bf0d40ab835ee2d94551e7ac744bdc0cb8de22309f4ea1f`. Also read back backend `ws2816_gpt_pair`, profile 3, mode 32, palettes 33 and 43, emit off, brightness 24.

Probe power stays off. No mass erase, unlock, or option-byte change.
