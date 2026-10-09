---
read_when: Changing decoding, host integration, threading, or log persistence
---

# Architecture

The plugin creates a private IQFrontEnd channel at 8 kHz sample rate and 500 Hz bandwidth, following the selected Radio frequency. No extra waterfall cursor is created. A host Handler thread shifts this complex baseband to an 800 Hz mono representation and places packets in a bounded queue. One worker performs decoding, logging, and file commands. The GUI takes a snapshot; disk operations do not run in the GUI or stream callback.

An initial audio-stream implementation failed installed-host testing because Brown's CW AGC raised gaps and distorted the amplitude keying. The direct IQ path avoids that dependency and keeps speaker settings untouched.

The stream-drain thread is stopped before removing the private channel. Radio stream registration/deletion events reconnect or detach the decoder. Destruction unhooks UI and stream lifecycle events, stops the drain, and joins the worker. A playback/mode gate enables decoding with CW, USB, or LSB selected.

Packets carry sample rate, tuned RF frequency, and a discontinuity generation. Retuning, sample-rate changes, and overflow reset the detector. Unsaved segments preserve their own frequency and speed, including when autosave is disabled. File errors retain pending segments. A short queue lock contains no DSP, disk operations, or UI calls.

The decoder downsamples to 4 kHz after two low-pass stages. A 1024-point Hann FFT finds a stable audio tone using global and local spectral contrast, with three agreeing estimates before lock. Quadrature filtering creates a 1 ms envelope. A rolling noise estimate, fading-aware peak, hysteresis, and speed-dependent debounce produce key transitions. Two duration clusters estimate dot/dash timing; spaces follow learned timing. Unknown patterns are displayed literally as `[?]`.

The detector is intentionally deterministic. No language model or word dictionary fills in missing characters. The signal status and WPM are estimates, not probabilities. Initial acquisition takes several characters. A continuous carrier is not sufficient to produce valid text.

Markdown writes use a regular-file check, `O_NOFOLLOW`, append mode, and `fsync`. Rename uses an exclusive hard link followed by unlink, avoiding replacement races. Clear archives with an exclusive link before replacing the active file. The last log filename is stored separately and atomically replaced. Log operations are serialized by the worker.

There is no universal stable binary ABI across SDR++ forks. Rebuild and revalidate this plugin after replacing Brown. Do not attempt to repair ABI mismatches by re-signing or modifying the application bundle.

The transcript uses a read-only ImGui multiline text control with cached word wrapping. Selecting or scrolling holds a bounded snapshot while the decoder and journal continue. Cmd+C copies the selection; the context menu also offers the full current transcript. Follow text releases the held view. Only the UI thread accesses selection buffers. The host's internal multiline child scroll API is used for auto-follow; its ImGui headers were compared with the installed August 4 Brown revision. Recheck those headers when rebuilding for a different host.

The plugin claims waterfall input over its visible window, throughout gestures that start there, while its copy menu is open, and while a focused text control handles keys. A drag can leave the window without reaching the waterfall. The guard releases for a fresh outside click, preserving normal tuning.

Clear view and successful Clear log operations increment a transcript reset counter in the worker snapshot. The GUI uses that counter to discard held text and selection and resume following live text, even if its window was hidden during the command. Failed archival leaves both the transcript and reset counter unchanged. Clear log is a single-click operation with automatic archival. After archival succeeds it resets Morse timing, discards the worker's current audio packet and clears the queued audio so old characters cannot refill the cleared box.

## Boundaries

Retuning, sample-rate changes and dropped-audio resets mark a pending paragraph boundary without adding text. The next decoded character inserts at most one paragraph separator after existing text. Silence and repeated resets cannot accumulate blank lines or start an empty transcript with whitespace. Normal Morse word gaps and message paragraph breaks remain intact.

- Each live log write can be at most about one second behind reception; a sudden crash can lose that unsynced tail.
- An abrupt system failure during a disk error cannot guarantee recovery of text still only in memory.
- This is a one-channel decoder, not a band-wide Morse skimmer.
- The UI uses Brown's bundled ImGui version and shares the application's existing accessibility limits.
- No commercial services, update agent, account, license gate, or analytics are part of this local plugin.
