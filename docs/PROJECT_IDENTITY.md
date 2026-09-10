<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# South Signal Lab CLOCK — Project Identity and Citation

CLOCK is intentionally a short product-facing name. Because the word “clock” is generic, repository metadata, citations, archival records, and search-oriented prose use **South Signal Lab CLOCK** as the canonical unambiguous project identity.

## Identity contract

| Field | Canonical value |
| --- | --- |
| Product name | `CLOCK` |
| Canonical searchable/citation name | `South Signal Lab CLOCK` |
| Compact firmware identity | `SSL CLOCK` |
| Author | Axel Napolitano |
| Publisher / creator brand | South Signal Lab |
| Software license | `PolyForm-Noncommercial-1.0.0` |
| Embedded target | STM32F401CCU6 / ARM Cortex-M4 |
| Primary domain | Eurorack / modular-synthesizer timing and rhythm |

The product name shown on the front panel and boot identity remains CLOCK. `South Signal Lab CLOCK` is used where ambiguity matters: search engines, software catalogs, citations, repository descriptions, release archives, and persistent identifiers.

## Product scope boundary

The canonical product identity includes a deliberate functional boundary: **South Signal Lab CLOCK is a digital timing, gate, and trigger instrument.** Hardware Rev 1 is not positioned as a general CV/modulation generator. It has dedicated SYNC/RST inputs and eight digital outputs, but no general parameter-CV inputs or analog CV/modulation output subsystem.

This distinction should remain explicit in README, manual, release notes, and comparisons. Missing analog modulation is not a prerelease defect; it is a DIY cost/buildability and signal-quality decision. See [`ROADMAP.md`](ROADMAP.md) for the product rationale and the separate post-1.0 feature path.

## Machine-readable metadata

Two root-level files are authoritative companions to `src/version.h`:

- [`../CITATION.cff`](../CITATION.cff) supplies Citation File Format 1.2 metadata. GitHub recognizes it and exposes **Cite this repository** on the repository page. Zenodo can also consume supported CFF metadata when archiving GitHub releases.
- [`../codemeta.json`](../codemeta.json) supplies CodeMeta 3.0 JSON-LD as `SoftwareSourceCode`, including author, publisher/brand, development status, target platform, programming languages, license, and discovery keywords.

CI documentation validation requires both files to carry the same current firmware version and canonical identity as the repository. This prevents release metadata from silently lagging behind firmware versioning.

## Persistent identifiers

CLOCK does **not** invent or reserve identifier-looking strings. Persistent identifiers are added only after the corresponding external archive has actually minted them.

Planned identifier roles are deliberately distinct:

| Identifier | Role |
| --- | --- |
| Git tag | Human/developer release identity |
| Commit hash | Exact Git revision |
| Release SHA-256 | Integrity of a packaged artifact |
| DOI | Citable archived software release / concept record |
| SWHID | Content-addressed Software Heritage identity |

### DOI / Zenodo policy

A DOI should be introduced for a publication-quality beta or stable release rather than for every rapidly moving alpha snapshot. Once Zenodo is connected to the real public GitHub repository and an archive is minted:

1. add the real DOI to `CITATION.cff`;
2. add the persistent identifier to `codemeta.json`;
3. add a DOI badge/link to the root README;
4. preserve the Zenodo concept DOI separately from version-specific DOIs when both exist;
5. never add `.zenodo.json` unless Zenodo-specific metadata is actually required, so `CITATION.cff` remains the single citation metadata source.

### Repository URL policy

The current firmware update QR points to the verified GitHub account URL `https://github.com/napolitano`. A repository-specific `repository-code` / `codeRepository` URL is intentionally omitted from CFF/CodeMeta until the final public repository URL is known and verifiable. Once established, the same exact canonical repository URL should be used in both metadata files and in the firmware update target.

## Discovery vocabulary

The primary machine-readable keywords are intentionally descriptive rather than marketing-oriented:

`eurorack`, `modular synthesizer`, `clock generator`, `trigger sequencer`, `gate sequencer`, `euclidean rhythm`, `rhythm generator`, `STM32F401`, `embedded firmware`, `PlatformIO`, `music technology`.

These should also guide the eventual GitHub repository description and Topics so search-engine, GitHub, DataCite/Zenodo, and software-catalog terminology remain aligned.

<h6 align="center">From Munich with &#9829;</h6>
