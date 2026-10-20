# Mission and working rules

Make users’ legally owned ORIGINAL Emperor data load and render natively on macOS, then port logic. Current phase: **DATA FIRST**.

- Every change must raise a measured STATUS.md metric or remove its named blocker. Start with a baseline and report X/Y with an explicit denominator. Unknown is unmeasured; synthetic passes are not original-data progress.
- Freeze CitySimulation, EMP1 and existing authored gameplay. Keep them; do not extend or use them as substitutes for original behavior.
- Clean room (relaxed 2026-10-17 by explicit user authorization): analysing the user's legally owned original executable and data files to extract format facts, data tables and algorithm behaviour for interoperability is permitted, including disassembly and running the original game as an oracle. Repo hygiene still applies: never commit or redistribute original code, disassembly/decompilation listings, original assets or derived binaries. Prefer published format documents and properly licensed modern parsers where they exist. Record every extracted fact with its exe offset/hash so it stays reproducible.
- Users supply assets with --data-root. Real-data tests read only EMPEROR_GAMEDATA_ROOT and exit successfully with SKIP when unset. A verified local directory may be injected for one test process; never alter global environment.
- Never commit original assets or derivatives: sprites, atlases, pixel/hash dumps, screenshots, binaries, captures or game-code archives. Aggregate metrics and authored test source are allowed. Keep all user data read-only.
- Keep README.md, one-screen STATUS.md and one document per format. Preserve licenses and repository contribution metadata. Delete stale round reports via ordinary recoverable Git commits; never rewrite history or remove untracked user files/local task evidence.
- Use explicit path manifests for commits, normal main pushes and direct remote SHA verification. No force pushes. Do not turn a permission failure for one action into a ban on all data work.
