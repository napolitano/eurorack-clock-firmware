<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# Storybook pre-produced video assets

This directory is the canonical repository location for the two approved South Signal Lab Storybook bumper clips. They are **authored publication inputs**, not Storybook-rendered output, and therefore are the only MP4 files intentionally tracked in the repository.

Tracked source assets:

- `south-signal-lab-intro.mp4` — 1920x1080, 30 fps, H.264 + 48 kHz stereo AAC, 4.5 s.
- `south-signal-lab-outro.mp4` — 1920x1080, 30 fps, H.264 + 48 kHz stereo AAC, 5.5 s.

All editable examples reference both files through `publication.intro_video` and `publication.outro_video`. Paths resolve relative to the Story YAML. SB-7 still probes and normalizes the clips before final composition, so future replacement assets may use a different supported source encoding or duration.

Generated tutorial videos, WebM files, retained frame trees, voice-over renders, subtitle sidecars and publication manifests are workstation artifacts. `.gitignore` rejects them globally and explicitly re-allows only these two approved bumper MP4s.

Do not add rendered tutorial output to this directory merely to bypass the ignore rules.

<h6 align="center">From Munich with &#9829;</h6>
