<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# GitHub Wiki Publication

The GitHub Wiki is a **generated publication surface**, not a second documentation source. Canonical content stays in the repository and is transformed by [`scripts/build_wiki.py`](../scripts/build_wiki.py). The [`Publish Wiki`](../.github/workflows/wiki.yml) workflow rebuilds the Wiki after every push that reaches the repository default branch and can also be run manually.

## Generated structure

The generator publishes a curated hierarchy instead of blindly copying every Markdown file:

- Home, User Guide index, User Manual, and Roadmap;
- a **24-page Using CLOCK section that mirrors the publication manual chapter-for-chapter**, from Start here through Technical status and specifications;
- Architecture, Timing, Configuration, and Simulator;
- Native Tests, Test Coverage, HIL Qualification, and V1 Forward Compatibility;
- Developer Setup, Development Workflow, Dependencies, and Documentation Index;
- Licensing, Project Identity, Contributing, and Security.

The chapter-oriented repository source lives in `docs/user-guide/`. `docs/USER_GUIDE.md` remains the consolidated operating-reference view, but the generated Wiki uses the split chapter set so its navigation follows the same mental model as the ODT/PDF manual instead of presenting one very long page.

`_Sidebar.md` is generated from the same page manifest so navigation cannot silently drift away from the page set. Repository-relative Markdown links are rewritten either to the corresponding Wiki page or to the exact source commit in the main repository. Direct edits in the GitHub Wiki are intentionally overwritten on the next successful publication.

Every generated content page ends with the canonical centered **From Munich with ♥** project footer.

## ODT manual download

The generated **User Manual** Wiki page contains a prominent download link to the version-frozen ODT manual for the exact source commit being published:

```text
docs/manual/clock-user-manual.<version>.odt  # final stable releases only
```

Wiki generation fails when that frozen ODT is missing. This prevents the Wiki from advertising a manual whose version does not match the firmware/documentation baseline.

## One-time GitHub setup

GitHub stores each Wiki in a separate `<owner>/<repository>.wiki.git` repository. GitHub does not create that backing repository until the Wiki has been initialized once.

1. Enable **Wiki** under **Settings -> General -> Features** if it is not already enabled.
2. Open the repository's **Wiki** tab and create one initial page manually. Its content is temporary; the first successful workflow run replaces it with the generated Wiki.
3. Keep GitHub Actions enabled. The Wiki workflow grants only the `contents: write` permission needed for its repository-scoped `GITHUB_TOKEN` to publish the Wiki.

After this one-time initialization no separate PAT is required for the Wiki of the same repository.

## Local preview

Generate the same Wiki tree locally without publishing it:

```bash
python scripts/build_wiki.py \
  --output build/wiki \
  --repository napolitano/eurorack-clock-firmware \
  --ref main
```

The generator validates the complete page inventory, the current-version ODT link, source-of-truth notices, and the canonical Munich footer before returning success.

<h6 align="center">From Munich with &#9829;</h6>
