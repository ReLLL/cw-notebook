# Changelog

## 1.0.32 - 2026-10-10

- Reject a speed candidate when at least two recent marks exceed 4.3 of its proposed dot units. The fit can no longer discard repeated slow dashes as outliers to manufacture a faster lock.
- Add a regression for distorted slow marks that previously calibrated at 37.5 WPM, then verify recovery at 20 WPM. Preserve the recent-window buffer and manual-speed behavior.

## 1.0.31 - 2026-10-10

- Reduce false fast-speed estimates caused by noisy, fragmented marks. Ordinary-speed acquisition uses a 90 Hz tone-envelope filter and 6 ms edge debounce; the extended-speed path retains its previous acquisition settings.
- Lower the ordinary-speed instantaneous noise gate from five to three times the measured noise envelope, retaining tone lock, hysteresis and peak checks. Weak dashes are less likely to break into short pulses.
- Add reproducible band-limited noise tests with varied Morse, three random seeds, both decoding paths and noise-only rejection. No expected station text or vocabulary is used.

## 1.0.30 - 2026-10-10

- Recover from an incorrect automatic speed lock when sustained new timing contradicts it. Require 16 recent marks, strong dot/dash and gap support, and four agreeing updates from distinct marks before changing speed at a character boundary.
- Preserve manual speed locks, existing transcript text and the ordinary speed range. Short interference cannot trigger this correction; no vocabulary or expected-message matching is used.
- Add regressions for slower/faster transitions, brief interference and manual locks. Compare 24 starting points from three private reception samples against the preceding build.

## 1.0.29

- Add speed-aware fade tracking to the optional recovery version: an earlier loud character no longer holds its peak threshold for a fixed 333 ms.
- Preserve the original detector and noise-rejection gates. Label the alternative Timing + fade recovery in the transcript, clipboard and log.
- Add audio regressions for abrupt attenuation at 8-40 WPM, varied letters/digits, clean-signal preservation and noise-only input.

## 1.0.28

- Fix persistent provisional output after interference: learn from the latest 16 marks and recent gaps while retaining the complete buffer for replay.
- Fit distorted dot/dash durations with bounded outlier influence; retain supported timing when new evidence is poor.
- Add a regression for startup interference followed by distorted Morse, alongside real reception replay checks.

## 1.0.27 - 2026-10-09

- Show labeled provisional letters after two seconds of detected keying, enabled by default. Reinterpret the draft as timing evidence changes; replace it with confirmed replay without duplicates.
- Keep tentative text out of autosaved logs. Clipboard copies include its provisional label. Retuning clears both the draft and held intervals.

## 1.0.26 - 2026-10-09

- Learn from a clean mixed character and flush at its closing gap. Retain measured intervals during learning, including long pauses; spill long buffers to temporary storage instead of truncating them.
- Manual WPM decodes held data. Default automatic range is 5-45 WPM; the optional extended range and manual settings support 3-200 WPM. Reject sudden speed-class jumps.
- Follow Radio's CW filter bandwidth. Keep direct IQ decoding independent of audio AGC.
- Consolidate controls into Transcript, Decoder and How to use tabs with one log toolbar. Copy includes text received while other tabs are open.

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
