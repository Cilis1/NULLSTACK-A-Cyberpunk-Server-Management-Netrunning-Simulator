# Changelog

## Unreleased
- Add a native Windows game window with rack status, action buttons, feedback, and new-run control. Linux retains its terminal interface.
- Add a Windows installer that downloads and verifies the release package and creates shortcuts.

## 0.1.0 — Early access: Back Room
- Add a small playable terminal scenario with hosting, intrusion, rack expansion, maintenance, heat, victory and loss.

## Foundation
- Establish design and architecture documents and milestone roadmap.
- Add a standalone C++20 deterministic core prototype, in-memory replay, event bus, headless runner and tests.
- Add a versioned replay log format and headless recording/playback.
- Add versioned transactional SQLite snapshots and headless save/resume.
- Add build presets and initial cross-platform CI configuration.

M0 is in progress: content infrastructure, dependency integration, release validation and several engineering requirements are still outstanding.
