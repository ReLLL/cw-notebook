# CW Notebook

Receive-only SDR++Brown plugin maintained by ReLLL. Public source: GPL-3.0-or-later.

- C++17; use the host's ImGui and public stream API. Never replace or re-sign Brown.
- Decoder and log worker are independent of ImGui. IQ callback shifts an 8 kHz channel to an 800 Hz testable audio representation and enqueues it.
- One decoding worker; one host stream-drain thread plus Brown's channel DSP. No worker pool or network services.
- Keep UI responsive, logs recoverable, and all errors visible. Never invent corrected Morse text.
- Never overwrite a user log when renaming. Clearing archives the previous file.
- Tests: `cmake --build build -j 4 && ctest --test-dir build --output-on-failure`.
- Memory checks: configure `build-sanitize` with `-DBUILD_PLUGIN=OFF -DSANITIZE=ON`.
- Read `docs/architecture.md` before changing stream lifecycle, timing, or file persistence.
- Read `docs/validation.md` before changing decoder thresholds or claiming reception quality.
- Transcript selection uses a held view; reception and saving continue. Preserve Cmd+C, right-click copy options, and the full-gesture guard against waterfall retuning. Verify these with a physical mouse if multi-display automation coordinates are unreliable.
- Install via `python3 scripts/install.py` while Brown is closed. Config backups are automatic.
- Read `docs/RELEASING.md` before publishing. Run `scripts/check.sh` and host smoke checks.
- Keep personal names, home paths, recordings, logs and credentials out of public source and artifacts. Public identity: ReLLL.
- No accounts, telemetry, automatic downloads or remote update execution.
