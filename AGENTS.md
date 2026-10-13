# CW Notebook

Receive-only SDR++Brown plugin maintained by ReLLL. Public source: GPL-3.0-or-later.

- C++17; use the host's ImGui and public stream API. Never replace or re-sign Brown.
- Decoder and log worker are independent of ImGui. CW uses an 8 kHz private IQ channel mapped to 800 Hz; USB/LSB/DSB/AM use a 16/32/64 kHz channel aligned with Radio's full selected passband. AM also supports envelope-demodulated Morse audio tones. Test ReceiveChannel/ChannelMixer with both decoding paths when changing mode integration.
- One decoding worker; one host stream-drain thread plus Brown's channel DSP. No worker pool or network services.
- Keep UI responsive, logs recoverable, and all errors visible. Preserve the original decode. Timing recovery is a separate, unverified interpretation. No vocabulary correction or suggestions.
- During timing acquisition, show a labeled provisional draft after about two seconds; never autosave it as confirmed text. Retuning clears measured intervals and drafts, preserves confirmed text, and starts fresh timing acquisition.
- Never overwrite a user log when renaming. Clearing archives the previous file.
- Tests: `cmake --build build -j 4 && ctest --test-dir build --output-on-failure`.
- Memory checks: configure `build-sanitize` with `-DBUILD_PLUGIN=OFF -DSANITIZE=ON`.
- Read `docs/architecture.md` before changing stream lifecycle, timing, or file persistence.
- Read `docs/validation.md` before changing decoder thresholds or claiming reception quality.
- Transcript content must always refresh; Follow text controls scrolling only. Preserve selection while appending, Cmd+C, right-click copy options, and the full-gesture guard against waterfall retuning. Run the optional real-ImGui transcript regression target when changing this behavior.
- Install via `python3 scripts/install.py` while Brown is closed. Config backups are automatic.
- Read `docs/RELEASING.md` before publishing. Run `scripts/check.sh` and host smoke checks.
- Keep personal names, home paths, recordings, logs and credentials out of public source and artifacts. Public identity: ReLLL.
- No accounts, telemetry, automatic downloads or remote update execution. Browser analysis is user-triggered only: preserve transcript text, include current reception context and uncertainty, copy the full prompt, and disclose clipboard fallback instead of claiming all sites auto-submit.
