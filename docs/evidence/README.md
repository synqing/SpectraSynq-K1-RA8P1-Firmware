# Evidence

Each implementation phase gets its own folder. Evidence must distinguish:

- HOST PASS
- TARGET COMPILE PASS
- TARGET EXECUTION PASS
- PHYSICAL AUDIO/TIMING PASS

Never collapse these into one generic PASS. Preserve commands, toolchain versions, board identity where available, output digests, benchmark distributions and mutation receipts.
