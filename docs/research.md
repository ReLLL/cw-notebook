# Design and source notes

These references informed the interface and validation approach. The Morse decoder is an original implementation; no Fldigi decoder source was copied.

| Reference | Influence |
| --- | --- |
| [SDR++Brown](https://github.com/sannysanoff/SDRPlusPlusBrown) | Host C++/ImGui UI, private IQ channels, stream lifecycle and external plugin loading. |
| [SDR++](https://github.com/AlexandreRouma/SDRPlusPlus) | Original modular SDR architecture underlying Brown. |
| [Fldigi CW configuration](https://www.w1hkj.org/FldigiHelp/cw_configuration_page.html) | Automatic timing with manual controls and a relearn action. |
| [Fldigi CW mode](https://www.w1hkj.org/FldigiHelp/cw_page.html) | Dedicated CW operating controls and receive workflow. |
| [ITU-R M.1677-1](https://www.itu.int/rec/R-REC-M.1677-1-200910-I/) | Morse element and spacing reference. |
| [ARRL operating aids](https://www.arrl.org/operating-aids) | Background for a removed vocabulary experiment. The released decoder does not suggest or substitute words. |
| [Priyom CW monitoring reference](https://priyom.org/military-stations/israel/4xz) | Background for evaluating repeated identifiers in local reception tests; not proof of station identity. |

Failure modes considered: unclear audio routing, timing mismatch, initial acquisition, fading, noise-only false text, UI stalls, lost logs, filename collisions, accidental clearing and mouse input reaching the waterfall.

See [credits](../CREDITS.md) for dependencies, authorship and licenses.
