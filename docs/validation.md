---
read_when: Changing thresholds, supported ranges, compatibility, or reliability claims
---

# Validation

## Public, reproducible checks

Run `sh scripts/check.sh` for the decoder/worker tests and Python installer tests. These checks need no SDR hardware, Brown installation or private recording.

Coverage includes ideal timing and noisy/fading synthetic audio at 3, 5, 12, 20, 40, 80, 120 and 200 WPM; sample-rate and tone variations; noise-only input; log rename/collision/path/symlink protections; clear/archive/restart behavior; manual-speed persistence; two minutes of silence; silent retuning/rate resets; and recovery of normal text afterward.

Installer tests use temporary profiles and dummy files. They check initial install, update backups, duplicate prevention, preservation of unrelated settings, invalid-config refusal and rollback after a failed config replacement. They do not replace the real plugin.

The standalone suite is also run with AddressSanitizer and UndefinedBehaviorSanitizer. The live host itself is not sanitizer-instrumented.

Recovery tests inject brief key dropouts, false pulses, compressed character gaps and stretched element gaps at the timing layer. Clean randomized Morse is compared against the original at 5-200 WPM. Worker tests check retained originals, separate views, persisted choices, retuning with exactly one newline, automatic lock release, optional lock retention, and intentional Pause. A fake clock exercises stream watchdog grace, retry, backoff and stop/Pause behavior. These are deterministic tests, not evidence of a measured field error rate.

To test the transcript against matching host ImGui sources:

```sh
cmake -S . -B build-ui -DBUILD_PLUGIN=OFF -DCW_IMGUI_SOURCE=/path/to/Brown/core/src/imgui
cmake --build build-ui --target cw_transcript_tests --parallel 4
ctest --test-dir build-ui -R cw_transcript_tests --output-on-failure
```

The headless test drives the actual TranscriptView and ImGui multiline widget. It reproduces the pre-1.0.22 freeze after a plain click, then verifies incoming text while selected, scrolled or Follow-disabled, active widget buffer updates, returning to automatic scrolling, and clear/replacement behavior. Only Brown's unrelated widget-registry hook is stubbed. This target is optional for portable builds without host sources, and required when modifying the transcript UI.

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

## Local 1.0.21 check, 2026-10-09

Core/recovery tests, three installer tests, and AddressSanitizer/UndefinedBehaviorSanitizer checks passed. The installed Apple Silicon binary matched the Release build byte-for-byte. Five live retunes between 7.021960 and 14.021960 MHz kept sample counts advancing, with zero dropped packets or reported errors; manual private-channel reconnect resumed reception. The original/recovery view switch and expanded controls were inspected in Brown. The receiver was left at 14.021960 MHz, CW, with center tuning enabled and Signal Snap Auto disabled.

No complete live CW transcript was obtained during this check. The new timing-recovery accuracy evidence is synthetic; the live checks establish stream continuity and UI operation, not a field error-rate improvement. Existing input-isolation code was retained; prior physical mouse checks remain the evidence for drag isolation.

## Local 1.0.22 check, 2026-10-09

The real-ImGui test first failed with `Clicking transcript froze incoming text`, then passed after separating content updates from automatic scrolling. Active selection, the widget's internal text length, Follow-disabled updates and resuming Follow were checked. Core/recovery, installer and ASan/UBSan checks also passed. Installed binary matched the Release build; Brown loaded 1.0.22 with original view and Follow enabled, receiver samples advancing and no errors or drops. Vocabulary controls and status fields were removed. The pre-restart receive frequency, 14.013980 MHz, was restored. Live reception did not provide a new transcript during the bounded smoke check; the deterministic widget test is the evidence for the refresh fix.

## Slow acquisition regression

Both original and recovery timing paths are tested at 5, 8, 12 and 18 WPM after six short startup pulses. The old algorithm failed by declaring a fast lock before the slow message. Tests require no output from those initial pulses, then the complete buffered slow prefix and subsequent message at the correct speed. Audio coverage includes 8 and 18 WPM alongside the existing 3-200 WPM cases. An engine test retunes from 20 WPM by just 1 Hz, requires uncalibrated zero WPM, and then decodes an 8 WPM station without carrying the previous estimate forward. These synthetic regressions do not establish accuracy for every noisy on-air signal.

## Local 1.0.24 check, 2026-10-09

Release preparation follow-up: macOS CI exposed a test assumption that a received phrase always fits in one autosave record. An explicit mid-message save reproduced the failure locally. Version 1.0.25 checks the original records joined in order, excluding recovery records, and keeps the same decoder behavior. Local core, recovery, installer and ASan/UBSan checks passed with the forced split.

A short clean CQ followed by silence reproduced missing output in 1.0.23. Starting candidate agreement earlier fixes this regression at 5, 8, 12, 18, 25 and 40 WPM in both timing paths while retaining the startup-pulse rejection tests. Core/recovery, three installer tests, ASan/UBSan and the real-ImGui test passed. The ImGui test also checks that reopening an off-screen window after resizing to 640 x 480 places it within view, focuses it and raises it above another panel.

The installed binary matched the Release build (SHA-256 `2ebb9fdea25144c320a2a27e3962901deb758278389800a23bc9c433a9a2b0d5`). Brown loaded version 1.0.24; disabling and re-enabling the plugin visibly restored its centered window. Reception resumed at the pre-update 14.031070 MHz with live text and no reported drops or errors. This verifies operation, not the accuracy of every received character. A separate 20-second fading recording produced no text in either 1.0.23 or 1.0.24; the startup improvement is demonstrated by the controlled short-CQ regression, not that recording.
