# CW Notebook for SDR++Brown

Decode one CW/Morse signal into a selectable transcript and recoverable Markdown log. Maintained by [ReLLL](https://github.com/ReLLL). Free and open source under GPL-3.0-or-later.

If the transcript is hidden or outside the visible area, use **Bring CW Notebook to front** in the sidebar. The window returns centered and in front without interrupting reception. Disabling and re-enabling also reopens it.

## Features

- Automatic timing defaults to 5-45 WPM, with an optional 3-200 WPM range and manual speed/carrier controls.
- Key and gap durations are held during learning and replayed when timing becomes usable. Long buffers spill to a temporary file instead of dropping their beginning.
- Speed learning uses the latest 16 marks and recent gaps. Poor evidence leaves timing unchanged; sustained, strongly supported new timing can correct a mistaken automatic speed lock. Manual speed stays fixed.
- Provisional letters appear after two seconds of detected keying and update as timing improves. The labeled draft is replaced by confirmed replay; only confirmed text is autosaved.
- CW reception follows Radio's filter width. Transcript, Decoder and How to use tabs keep the main view compact.
- Readable, resizable text; selection, Cmd+C, and right-click Copy selection / Copy all.
- **Copy to clipboard** exports the displayed transcript with local SQL-style date/time, weekday, UTC offset, current frequency, speed, and carrier offset.
- Markdown autosave, rename, new log, Finder reveal, and one-click clear with a dated backup.
- Select and scroll while incoming text continues; plugin gestures do not retune the waterfall.
- No extra blank pages during silence or silent tuning.
- Retuning continues on one new line, with automatic reacquisition and stalled-channel recovery.
- Original decode plus an optional timing-recovery view. No vocabulary correction or suggestions.
- Local processing. No accounts, microphone permission, virtual audio cable, telemetry, or network decoder.

## Compatibility

The supplied binary is for **Apple Silicon Macs, macOS 15 or newer**, and **SDR++Brown 1.2.1, August 4, 2026 build**. It is tested on macOS 26. Older macOS versions meeting the build target have not been tested.

SDR++ forks do not provide a stable plugin ABI. A different Brown build may require rebuilding against its matching headers. Intel, Windows, Linux host plugins, upstream SDR++, and other forks are not currently validated. The standalone decoder tests can run without Brown.

This is an independent community plugin, not an official SDR++Brown release. The binary is not Developer ID signed or notarized. If macOS or your host rejects it, build locally with matching headers; do not modify the host's signature or disable security protections.

## Install

1. Install the compatible [SDR++Brown](https://github.com/sannysanoff/SDRPlusPlusBrown), launch it once, then **quit it**.
2. Download the Apple Silicon ZIP and its `SHA256SUMS.txt` from [Releases](https://github.com/ReLLL/cw-notebook/releases/latest).
3. In Terminal, open the download folder and verify the ZIP:

   ```sh
   shasum -a 256 -c SHA256SUMS.txt
   ```

4. Extract the ZIP, open its folder in Terminal, and run:

   ```sh
   python3 scripts/install.py --binary cw_notebook.dylib
   ```

5. Restart Brown. **CW Notebook** appears in the sidebar. Start the receiver, choose **CW** in Radio, and tune one narrow keyed signal.

Python 3 is required; no Python packages are needed. The installer refuses to run while Brown is open and checks the binary architecture. It backs up the configuration and previous plugin, then installs outside the signed app bundle.

- Plugin: `~/Library/Application Support/sdrpp-brown/plugins/cw_notebook.dylib`
- Backups: `~/Library/Application Support/sdrpp-brown/cw-notebook-backups/`
- Logs: `~/Documents/SDR++Brown/CW Logs/`

For a custom Brown profile, add `--config-dir "/path/to/profile"`.

### Update or roll back

Download a newer compatible release, quit Brown, and run the same installer. Your logs and decoder preferences remain. There is no automatic updater. Use GitHub's **Watch → Custom → Releases** to follow updates.

To roll back, quit Brown and restore the previous `cw_notebook.dylib` from its dated backup. Restore the matching `config.json` only if needed; that also restores other settings to the backup time. Keep your current config copy first.

To uninstall, disable/remove the **CW Notebook** instance in Module Manager. With Brown closed, remove its absolute plugin path from the `modules` array in `config.json` and move only `cw_notebook.dylib` to Trash. Keep the logs.

## Receiving Morse

1. Choose **CW** and put one keyed carrier inside Radio's filter. CW Notebook follows its width; narrow it to exclude nearby stations. USB/LSB retain a centered 500 Hz CW channel. Speaker mute does not stop decoding.
2. Tune another station or band normally. Acquisition restarts automatically; the next decoded text starts on one new line. Earlier text remains. Manual speed/tone locks are released by default; disable **Release manual locks when tuning a new station** in Decoder to retain them. An intentional Pause remains paused.
3. Learning starts from zero after retuning. Key/gap durations stay buffered until mixed dot/dash evidence and a character gap establish timing. A clean D can unlock at its first character gap. Ambiguous sequences show a labeled provisional decode after about two seconds; confirmed output waits for stronger evidence. Silence retains the buffer. Manual WPM in **Decoder** can decode held intervals. **Relearn**, retuning, tone changes and stream resets discard uncommitted intervals; confirmed text remains.
4. **Follow** controls automatic scrolling only. Clicking never freezes new text. Selecting or scrolling back holds the scroll position while text keeps arriving; enable Follow to jump to the latest.
5. **Clear view** clears the screen only. **Clear log** archives the file and completely clears the displayed text, unfinished characters and queued audio. Newly received text can then appear.

**Copy to clipboard**, beside Clear log, copies the selected version without artificial display wrapping, including text received while scrolled back or on another tab. Its timestamp and metadata describe the moment of copying. Right-click **Copy all** copies text without metadata. **Log file...** contains Open, Show in Finder, Rename and New log.

Markdown logs use UTC section timestamps. Retuning starts a new frequency section. Rename never overwrites another log; the `.md` extension is automatic.

### Recovery controls

The original decode is the default. The selector above the transcript switches between **Original decode** and **Timing + fade recovery (unverified)**. Repair is enabled for the separate version by default in **Decoder**. During learning, a labeled provisional section appears after two seconds of detected keying and can be revised every quarter second. Its first-pass speed stays within 5-45 WPM with a modest 20 WPM preference for ambiguous evidence. It does not call the confirmed-output callback or enter the Markdown log. Confirmed replay replaces it; already confirmed text is not retroactively rewritten. Retuning clears the provisional section and its measured intervals.

Recovery follows sudden amplitude fades at the learned element speed, so a previous loud character is less likely to hide a quieter dot. It keeps the original noise-floor and carrier checks. It also filters very short key glitches, estimates uneven character spacing, and relearns timing after a long quiet interval. It can still misinterpret a signal, and cannot reconstruct elements lost below the detectable noise level. No dictionary, station template, repeated-message substitution or vocabulary suggestions are used. Changing messages require no special action.

Autosave retains labeled original chunks and differing timing-recovery chunks in the same Markdown file. Turning an option off affects future decoding, not historical text. **Clear log** clears both views after archiving successfully. A restart restores settings and the log path; session text is available in the saved file, not reloaded into the box. Old log entries are preserved when updating from a version that included vocabulary hints.

If receiver samples stop arriving, the plugin retries its private channel after three seconds, backing off to 30 seconds. It leaves a stopped receiver or deliberate Pause alone. **Reconnect decoder** retries manually. **Save 20s signal** stores a private 8 kHz mono float32 capture under the log folder's `Diagnostics` directory for offline comparisons; continuous raw audio is not recorded.

### Limits

- One signal at a time. This is not a band-wide skimmer.
- Automatic timing defaults to 5-45 WPM. Manual speed and the extended range support 3-200 WPM in synthetic tests; this is not a guarantee for every real signal or sending style.
- Learning buffers last for the current uninterrupted reception. Temporary storage does not survive a crash, restart or explicit reset. Disk failures are reported. Buffering cannot recover RF/audio samples missed before tone detection or during a stream interruption.
- Fading, interference, weak signals and nonstandard timing can cause errors in either view. `[?]` marks unknown patterns.
- Deep fading can destabilize automatic speed. A manual speed lock may help.
- The displayed transcript is bounded to roughly 80-100 KB; saved logs are not trimmed. Saving failures retain pending text up to a visible 2 MB safety limit.
- Silence cannot identify a station, and a continuous carrier is not Morse.

## Build from source

Install Xcode Command Line Tools and CMake. Homebrew headers needed by Brown's interfaces:

```sh
brew install cmake fftw volk
git clone https://github.com/ReLLL/cw-notebook.git
git clone https://github.com/sannysanoff/SDRPlusPlusBrown.git
git -C SDRPlusPlusBrown checkout 679f48deee5299a199d15d30e6e785fac6054896
cd cw-notebook
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DBROWN_SOURCE="../SDRPlusPlusBrown" \
  -DBROWN_APP="/Applications/SDR++Brown.app" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
python3 scripts/install.py
```

The pinned headers match the supported August 4 host. For a different host, use its exact source revision and verify the ImGui ABI. Do not replace its bundled ImGui with a newer version.

For core and installer tests without Brown:

```sh
sh scripts/check.sh
```

## Architecture

```text
Brown IQ stream
  → private channel at 8 kHz, following Radio's CW filter (500 Hz for USB/LSB)
  → shift to an 800 Hz mono tone
  → bounded audio queue
  → one decoder / log worker
       ├─ tone detection → key transitions → Morse timing → text
       ├─ recoverable Markdown journal
       └─ snapshot → ImGui transcript and clipboard
```

The GUI never performs DSP or log writes. One host stream-drain thread feeds one worker; there is no decoder pool. Retuning and audio gaps reset timing. Input guards keep plugin mouse gestures out of the waterfall.

Details: [architecture](docs/architecture.md), [validation](docs/validation.md), [release process](docs/RELEASING.md).

## Contribute and license

Bug reports, patches, decoder improvements and ports are welcome through [Issues](https://github.com/ReLLL/cw-notebook/issues) and pull requests. See [CONTRIBUTING.md](CONTRIBUTING.md).

Copyright © 2026 ReLLL and contributors. Licensed under **GNU GPL version 3 or, at your option, any later version**. You may use, study, modify and redistribute the project, including commercially, under that license. Distributed derivatives must preserve the applicable license and source obligations. No warranty is provided.

See [LICENSE](LICENSE), [CREDITS.md](CREDITS.md), and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
