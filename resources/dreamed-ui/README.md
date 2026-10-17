# RTTY notebook design reference

Generated with the built-in ImageGen tool as a design reference. Selected
direction: the left graphite/brass instrument panel. The implementation keeps
CW unclosable and reception metadata read-only; the generated X on CW and
editable metadata boxes are not product requirements. Settings and help open
from buttons as modals. Native fonts and the host's rendering stack are retained.

Reference: `rtty-instrument-directions.png`.

## Generation prompt

Use case: ui-mockup. Create a side-by-side comparison of TWO feasible native Dear
ImGui desktop radio decoder windows, each titled CW Notebook, for HAM operators
using SDR++. This is a design reference, not a website. Both show one compact
toolbar with Monitor RTTY checkbox, Close all tabs, Signals, CW Settings, How to
use buttons; a thin capture-range/status line; ONLY reception tabs CW,
14062.085 kHz RTTY, 14100.788 kHz RTTY; selected RTTY view with mark frequency,
45.45 baud, 170 Hz, Normal, Receiving; Pin signal, Ignore & close, Copy, Follow
controls; large dominant readable transcript CQ CQ DE TEST 123 K in a quiet
fixed-width-looking text area. No decorative charts, cards, gradients, glows,
large metrics or illustrations. Small consistent 4px corner radii, precise 8px
spacing, existing standard small app fonts, high text contrast. Direction A:
graphite instrument panel, restrained brass/amber selected-tab underline and pale
ivory text, neutral flat buttons. Direction B: warm dark neutral panel with
desaturated steel/sea green activity accent and off-white text, slightly more
spacious vertical rhythm. Dense useful polished professional receiver cockpit,
realistic 900x620 window proportions for each direction. Ensure modal
settings/help are not tabs. No personal identities, no actual received callsigns,
no screenshot of the operating system.
