# Contributing

Everyone is welcome to report bugs, propose improvements, submit pull requests or maintain a fork. Contributions are offered under GPL-3.0-or-later, the same license as the project. No contributor license agreement is required.

## Report a problem

Include the plugin version, Brown build/source revision, OS and CPU architecture, receiver, mode, approximate signal speed, relevant settings, and steps to reproduce. Explain expected and actual behavior. Remove personal paths, station logs and sensitive content before posting. Do not upload raw recordings unless you have the right to share them.

## Make a change

1. Fork the repository and create a focused branch.
2. Read `AGENTS.md` and `docs/architecture.md`.
3. Keep DSP and file operations out of the GUI thread; keep queues bounded and logs recoverable.
4. Add a regression test for behavioral fixes. Preserve normal Morse spacing and report uncertainty without fabricating text.
5. Run `sh scripts/check.sh`. For memory checks:

   ```sh
   cmake -S . -B build-sanitize -DBUILD_PLUGIN=OFF -DSANITIZE=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
   cmake --build build-sanitize --parallel 4
   ctest --test-dir build-sanitize --output-on-failure
   ```

6. Build and test host changes against the documented Brown revision. Verify tuning, selection, clipboard, log recovery and restart behavior.
7. Open a pull request explaining the problem, change, validation and any compatibility limits.

Use your preferred public identity in commits. Do not add personal recordings or local configuration to the repository. Credit upstream work and retain its license notices.
