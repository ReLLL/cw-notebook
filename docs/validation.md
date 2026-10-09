---
read_when: Changing thresholds, supported ranges, compatibility, or reliability claims
---

# Validation

## Public, reproducible checks

Run `sh scripts/check.sh` for the decoder/worker tests and Python installer tests. These checks need no SDR hardware, Brown installation or private recording.

Coverage includes ideal timing and noisy/fading synthetic audio at 3, 5, 12, 20, 40, 80, 120 and 200 WPM; sample-rate and tone variations; noise-only input; log rename/collision/path/symlink protections; clear/archive/restart behavior; manual-speed persistence; two minutes of silence; silent retuning/rate resets; and recovery of normal text afterward.

Installer tests use temporary profiles and dummy files. They check initial install, update backups, duplicate prevention, preservation of unrelated settings, invalid-config refusal and rollback after a failed config replacement. They do not replace the real plugin.

The standalone suite is also run with AddressSanitizer and UndefinedBehaviorSanitizer. The live host itself is not sanitizer-instrumented.

## Optional local recordings

Private reception fixtures are not distributed. Maintainers can configure `CW_RECORDING_FIXTURE` with a local 8 kHz float32 recording matching the optional checks in `tests/tests.cpp`. The companion `4xz-fading-8k.f32` must be beside it. These checks are skipped explicitly when not configured; synthetic coverage still runs.

Manual-speed operation was checked on a faded live CW recording; automatic speed was less stable. No perfect live transcript or universal real-world speed range is claimed.

## Host checks

Development validation used an Apple Silicon Mac, macOS 26, RTL-SDR Blog V4, and SDR++Brown 1.2.1 built August 4, 2026.

Verified during development:

- Live decoding, receiver stop/start, plugin disable/enable and app restart.
- File rename, continued append, clear with recovery archive, and restoring the last log.
- Text selection, Cmd+C and context-menu copying.
- No waterfall retuning from physical mouse gestures started inside the plugin.
- Clipboard summary containing date/time, weekday, frequency, speed, carrier offset and decoded text.
- Single-click Clear log empties the transcript and archives the previous file.
- Silent stream changes no longer fill the box with blank pages.

Multiple-display automation coordinates were occasionally unreliable; physical mouse checks were used for gesture isolation. These are bounded checks, not an all-day stability guarantee.

For each release, record the actual host smoke test and artifact checksum in its release notes. A deployment target is not evidence that every older OS version has been tested.
