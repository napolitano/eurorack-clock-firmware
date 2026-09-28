<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# Storybook pre-produced video assets

This directory is the canonical repository location for optional pre-produced CLOCK Storybook publication clips such as a South Signal Lab intro or end card.

These files are **publication inputs**, not Storybook-rendered scenes. A story references them through `publication.intro_video` / `publication.outro_video`, normally with a path relative to the story file. Their original geometry, frame rate and audio layout may differ from the generated tutorial: SB-7 probes and normalizes them before final composition.

No placeholder video is committed. A missing asset referenced by a story is a hard publication failure.

<h6 align="center">From Munich with &#9829;</h6>
