---
read_when: Changing decoding, host integration, threading, or log persistence
---

# Architecture

The plugin creates a private IQFrontEnd channel at 8 kHz sample rate, following the selected Radio frequency. In CW mode its width follows Radio's filter (50-500 Hz in the supported host); USB/LSB retain a centered 500 Hz channel. Metadata updates change taps through RxVFO's synchronized bandwidth API without resetting timing. This uses the host's bundled VOLK library. No extra waterfall cursor is created. A host Handler thread shifts complex baseband to an 800 Hz mono representation and enqueues it. One worker performs decoding, logging and file commands. The GUI takes a snapshot; disk operations never run in the GUI or stream callback.

An initial audio-stream implementation failed installed-host testing because Brown's CW AGC raised gaps and distorted the amplitude keying. The direct IQ path avoids that dependency and keeps speaker settings untouched.

The stream-drain thread is stopped before removing the private channel. Radio stream registration/deletion events reconnect or detach the decoder. Destruction unhooks UI and stream lifecycle events, stops the drain, and joins the worker. A playback/mode gate enables decoding with CW, USB, or LSB selected.

Packets carry sample rate, tuned RF frequency, and a discontinuity generation. Retuning, sample-rate changes, and overflow reset the detector. Unsaved segments preserve their own frequency and speed, including when autosave is disabled. File errors retain pending segments. A short queue lock contains no DSP, disk operations, or UI calls.

The decoder downsamples to 4 kHz after two low-pass stages. A 1024-point Hann FFT finds a stable audio tone using global and local spectral contrast, with three agreeing estimates before lock. Quadrature filtering creates a 1 ms envelope. A rolling noise estimate, fading-aware peak, hysteresis, and speed-dependent debounce produce key transitions. Two duration clusters estimate dot/dash timing; spaces follow learned timing. Unknown patterns are displayed literally as `[?]`.

The detector is deterministic. No language model or word dictionary fills in missing characters. Signal status and WPM are estimates, not probabilities. A continuous carrier cannot produce valid text.

Ordinary 5-45 WPM acquisition uses a 90 Hz tone-envelope low-pass cutoff and 6 ms edge debounce. This is an internal noise filter after tone selection, separate from Radio's selected channel width. Extended-speed acquisition retains 200 Hz and 2 ms. The ordinary amplitude gate requires three times the estimated noise envelope and a peak above four times noise; the extended path and manual speeds outside 5-45 retain the earlier five/six-times gates. Tone lock, hysteresis and the absolute peak floor remain required. The previous ordinary gate fragmented weak but audible dashes, leading to false fast timing or indefinite learning.

Automatic acquisition starts uncalibrated after frequency changes greater than 0.5 Hz. Default estimates must fall within 5-45 WPM; an explicit extended preference permits 3-200 WPM. Out-of-range estimates are rejected, never clamped. Speed fitting uses only the latest 16 marks and recent gaps, independently of the full replay buffer. A bounded-loss search fits the Morse 1:3 duration model with at least eight marks, three dots, three dashes, 75% mark support and compatible inner gaps. This avoids short interference pulses defining the dot cluster. A clean mixed three-mark character can still establish timing earlier through the stricter cluster path. Acquisition waits for a live gap exceeding 2.2 candidate units, so output does not require another key-down transition. Single-class marks remain ambiguous. Established timing rejects candidate jumps beyond 0.7-1.4 times its unit and retains its estimate when recent evidence lacks support.

Measured key/gap durations are replayed in order using learned timing. RunBuffer keeps 4096 intervals in memory, then spills to an anonymous temporary file on the worker. Old intervals are not silently truncated. Storage failures surface through the worker error state. Replay skips leading marks outside the plausible range as startup interference. Uncalibrated quiet periods keep the buffer. Manual WPM interprets held intervals without restarting tone detection. Retuning, tone changes, explicit resets and stream discontinuities clear uncommitted runs. Temporary buffering is not crash recovery or an RF recorder.

Until calibrated, WPM is zero and the UI reports learning plus a buffered interval count. Confirmed originals stay append-only; reinterpretation is confined to held intervals. Manual WPM overrides the automatic range; default retuning releases it. The extended preference is appended to the settings file, with false used for older files.

After two seconds of detected keying, Timing also produces a separate provisional string. A bounded 512-run tail is rescored at most four times per second against 5-45 WPM, with a modest 20 WPM prior. All measured runs remain available for confirmed replay, even when the displayed preview uses only a tail. Preview may include an unfinished character and is allowed to change. It never calls the confirmed-output callback. DecoderStats carries the preview to the GUI; transcriptText composes the confirmed text and a labeled provisional section for display/copy only. Acquisition or reset removes the draft. This offers early feedback without saving guesses as confirmed reception.

The same worker runs two detectors on each packet. The original disables timing recovery; recovery uses symmetric key debounce and a bounded character-gap estimate. It resets calibrated timing once after 12 seconds without keying, preserving uncalibrated buffers. Both transcripts are bounded and retained independently. The default UI shows the original. Changing views never feeds text back into either detector.

Once timing is calibrated, the recovery detector uses a peak-memory time constant of 1.5 dot units, bounded to 30-300 ms, instead of the original fixed roughly 333 ms. This prevents a loud character's peak from masking a following attenuated dot. Acquisition, noise-floor multipliers, tone-lock requirements and debounce limits are unchanged. The original detector keeps its earlier threshold behavior; the recovery checkbox controls the alternative. Tests use abrupt attenuation and varied letters/digits, without station-specific logic.

The window has Transcript, Decoder and How to use tabs, a shared live status and one log toolbar. Report copying reads the worker snapshot even when the transcript tab is hidden. The log menu participates in the same waterfall input guard as transcript popups.

There is no vocabulary suggestion or substitution stage. Markdown saves original chunks and differing recovery chunks. The retired vocabulary preference slot is read and ignored, and written as zero for compatibility with older settings files.

Retuning clears stale queued audio and resets both detectors. By default a station change releases manual speed/tone locks; that behavior is independently configurable. A callback-count watchdog reconnects only the private channel after three seconds without data, with retry delays capped at 30 seconds. It never treats silence or missing decoded characters as a stalled stream, and respects Pause and receiver stop. Failed diagnostic recording disables capture while decoding continues.

Markdown writes use a regular-file check, `O_NOFOLLOW`, append mode, and `fsync`. Rename uses an exclusive hard link followed by unlink, avoiding replacement races. Clear archives with an exclusive link before replacing the active file. The last log filename is stored separately and atomically replaced. Log operations are serialized by the worker.

There is no universal stable binary ABI across SDR++ forks. Rebuild and revalidate this plugin after replacing Brown. Do not attempt to repair ABI mismatches by re-signing or modifying the application bundle.

The transcript uses a read-only ImGui multiline text control with cached word wrapping. Its buffer always receives incoming text, including while selected or scrolled back. Follow text controls scrolling only; a plain click leaves it enabled, while selection or wheel scrolling disables automatic scrolling. Appending preserves selection indices; replacement, trimming, resize or view changes clear stale selection. Cmd+C copies the selection; the context menu also offers the full current transcript. Only the UI thread accesses selection buffers. The host's internal multiline child scroll API is used for auto-follow; recheck matching ImGui headers when rebuilding for a different host.

The plugin claims waterfall input over its visible window, throughout gestures that start there, while its copy menu is open, and while a focused text control handles keys. A drag can leave the window without reaching the waterfall. The guard releases for a fresh outside click, preserving normal tuning.

Enabling the plugin or reopening its transcript requests one centered, focused presentation within the host viewport. The sidebar also offers Bring transcript to front without restarting the stream. Size constraints shrink with the viewport; ordinary frames do not steal focus or recenter the window.

Clear view and successful Clear log operations increment a transcript reset counter in the worker snapshot. The GUI uses that counter to discard held text and selection and resume following live text, even if its window was hidden during the command. Failed archival leaves both the transcript and reset counter unchanged. Clear log is a single-click operation with automatic archival. After archival succeeds it resets Morse timing, discards the worker's current audio packet and clears the queued audio so old characters cannot refill the cleared box.

## Correcting an established speed

Every duration-fit candidate is rejected if two or more recent marks exceed 4.3 proposed dot units. Repeated long marks contradict the fast interpretation even when its bounded-loss score fits the shorter majority. This asymmetric check still tolerates one long transient and avoids forcing a specific WPM or station message.

Ordinary tracking stays within 0.7-1.4 times the current unit. A separate candidate search can escape that range only with 16 recent marks, at least three dots and three dashes, at least six compatible inner gaps, 90% mark support and mean bounded mark loss no greater than 0.12. Four agreeing updates from distinct completed marks must agree within 12%. The change occurs after a gap longer than 2.2 times both timing units, following completion of the current character. This corrects a false startup lock or a sustained operator-speed change without replacing earlier text. Manual speed changes clear any pending automatic correction; manual locks never run the candidate search.

## Boundaries

Retuning, sample-rate changes and dropped-audio resets mark a pending line boundary without adding text. The next decoded character follows exactly one newline after trimming trailing whitespace from the preceding text. Silence and repeated resets cannot accumulate blank lines or start an empty transcript with whitespace. Normal Morse word gaps and message paragraph breaks remain intact within a reception run.

- Each live log write can be at most about one second behind reception; a sudden crash can lose that unsynced tail.
- An abrupt system failure during a disk error cannot guarantee recovery of text still only in memory.
- This is a one-channel decoder, not a band-wide Morse skimmer.
- The UI uses Brown's bundled ImGui version and shares the application's existing accessibility limits.
- No commercial services, update agent, account, license gate, or analytics are part of this local plugin.
