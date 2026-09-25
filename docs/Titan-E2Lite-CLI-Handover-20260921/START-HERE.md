# Start here — instruction to the implementing CLI agent

Read `Titan-E2Lite-Execution-Brief.md` completely, then execute it on the user's Mac.

Your objective is a working, repeatable E2 Lite → Titan Mini RA8P1 CPU0 debug connection and direct agent operation of the required workflow. The current error is 0x00030815, DAP not found. Remote terminal access works; a previous System Events query was denied assistive access. The running e² studio workspace had no saved `.launch` configuration. Do not ask the user to navigate menus you can inspect yourself.

First inspect applicable repository instructions/current bench leave-state, the installed debugger backend and the actual settings. Prefer the existing command path; add only the small local MCP/GUI capability that is actually missing. A tunnel does not grant macOS Accessibility permission. Resolve a real permission prompt with one precise user action only if necessary.

Preserve the resident firmware and device security. Verify target/probe identity, exclusive ownership, SWD, clock, target reference, and RA8 hot-plug authentication (AL=None). Do not repeat unchanged failed attempts. Treat saved cable diagrams as documentation, not proof of actual wiring. Record reset/halt effects truthfully.

Continue through connection, clean detach/reconnect, reusable configuration and verified leave-state. Return concise evidence-backed results, not another plan. Keep token use and scope bounded; no multi-agent campaign.
