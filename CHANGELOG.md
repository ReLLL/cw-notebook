# Changelog

## 1.0.25 - 2026-10-09

- Make persistence tests verify original text across autosave record boundaries. Explicitly save mid-message to exercise the case that failed on macOS CI. Decoder behavior is unchanged from 1.0.24.

## 1.0.24 - 2026-10-09

- Begin checking speed agreement earlier, keeping the eight-mark minimum and dot/dash/gap validation. A clean short CQ no longer waits for a later transmission before showing text.
- Reopening or re-enabling CW Notebook centers its window in the visible host area and brings it to the front. Add a sidebar Bring transcript to front button without interrupting reception.
- Test short CQ acquisition at 5-40 WPM and window recovery after shrinking the host viewport, alongside existing slow-CW, noise and transcript-selection regressions.

## 1.0.23 - 2026-10-09

- Restart timing acquisition from zero even on a 1 Hz retune. Show learning rather than an assumed speed until calibrated.
- Buffer startup key durations and require supported dot/dash timing before emitting letters. Remove the early single-group fallback that could lock onto interference at a falsely high speed.
- Skip implausible leading startup marks when replaying the buffered first characters.
- Add 5, 8, 12 and 18 WPM startup tests with initial false pulses, slow audio checks, and a 20-to-8 WPM retune regression.

## 1.0.22 - 2026-10-09

- Fix incoming text freezing after clicking or scrolling in the transcript. Follow text now controls automatic scrolling only; both views keep updating without toggling the recovery checkbox.
- Plain clicks retain automatic scrolling. Selection and scroll position remain usable while new text arrives.
- Remove ham vocabulary suggestions, their controls, decoder stage and future log output. Preserve existing logs.
- Add a regression test using the host's real ImGui multiline widget, including active selection and updates while Follow text is off.

## 1.0.21 - 2026-10-09

- Preserve original decoding alongside a separately selectable timing-recovery transcript.
- Optional, default-on key-glitch filtering, adaptive character spacing, quiet-period relearning, and same-element ham vocabulary hints. Suggestions never replace the original text.
- Retuning preserves text and continues on one new line when the next character arrives. Release old station speed/tone locks by default, with a checkbox to retain them.
- Discard stale queued samples after retuning. Retry stalled private channels with bounded backoff while respecting Pause and receiver stop.
- Save both interpretations and clearly labeled hints to Markdown. Clear/archive clears both views and hints.
- Add a manual reconnect control and private 20-second diagnostic capture. Capture write failures do not stop decoding.
- Regression coverage for damaged timing, clean random Morse, retained originals, retuning, preferences, Pause, and watchdog behavior.

## 1.0.19  -  2026-10-09

First public release.

- Live CW decoding with automatic timing and optional manual speed/carrier controls.
- Selectable transcript, copy context menu and formatted clipboard reception summary.
- Recoverable Markdown logs, filename controls and one-click archive/clear.
- Silent retuning no longer accumulates blank lines.
- Full plugin mouse gestures stay isolated from waterfall tuning.
- Public installation/update instructions, architecture notes, contributor guide and upstream credits.
- Installer supports release binaries and custom profile folders, validates config before replacement, and restores the previous plugin if config replacement fails.
- Portable core tests and installer regression checks; private recordings excluded.

Earlier versions were local development builds.
