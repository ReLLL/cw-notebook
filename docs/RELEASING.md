# Release process

Maintainer: [ReLLL](https://github.com/ReLLL). Repository: [cw-notebook](https://github.com/ReLLL/cw-notebook).

## Prepare

1. Increment the three-part version in `CMakeLists.txt` and update `CHANGELOG.md`.
2. Run `sh scripts/check.sh` and the sanitizer commands in `CONTRIBUTING.md`.
3. Build Release against the supported Brown headers and app. Set the deployment target explicitly; current public Apple Silicon target is macOS 15.0.
4. Install the candidate with Brown closed. Check live reception, pause/resume, text selection, metadata copying, clear/archive, input isolation and app restart.
5. Review every staged path for credentials, private paths, recordings and personal information. Keep Git author/committer identity at the intended public GitHub username and noreply address.
6. Sanitize prose without rewriting upstream license texts or source code.

## Package

```sh
python3 scripts/package.py --binary build-release/cw_notebook.dylib
```

This creates a versioned ZIP and `SHA256SUMS.txt` in ignored `dist/`, using an explicit allowlist. The archive includes the installer, README, changelog, license and third-party notices. It does not include logs, fixtures, config or private build directories.

Inspect `otool -L`, `vtool -show-build`, and `strings` for dependency paths, minimum OS and personal build paths. Do not re-sign or modify Brown. This community plugin is not a notarized standalone application; document that accurately. Publish source at the same tag as any binary.

## Publish and verify

1. Commit reviewed source, push the default branch and create a version tag.
2. Create a GitHub release for that exact tag with the ZIP and checksum file.
3. Download both release assets into a fresh temporary directory and verify the checksum and archive contents.
4. Verify the public repository, license detection, tag/commit, release notes and contributor links.
5. Record tested OS, CPU architecture and Brown revision. Do not claim untested platform support.
6. For fixes, publish a new patch version. Do not silently replace existing release assets.

Issues and pull requests are the update/contribution queue. Users update manually with the backed-up installer and may subscribe to GitHub release notifications. There is no automatic updater or network executable delivery.
