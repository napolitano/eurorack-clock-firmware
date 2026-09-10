<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# HIL Capture Exchange Format

CLOCK uses one intentionally simple long-form CSV format so timing evidence can be exported from different oscilloscopes and logic analyzers without coupling the repository to one vendor.

The header is exact:

```csv
time_us,signal,level
```

Each following row records one digital transition. `time_us` is a non-negative timestamp in microseconds, `signal` is a stable logical name, and `level` is `0` or `1`. Rows must be globally monotonic in time. Recommended signal names are `OUT1` … `OUT8`, `SYNC`, `RST`, `SCK`, `SDA`, `CS`, and `DISPLAY_MARKER` where a particular bench setup provides those probes.

Example:

```csv
time_us,signal,level
0.000,OUT1,1
2.100,OUT2,1
10000.050,OUT1,0
10002.150,OUT2,0
500000.000,OUT1,1
500002.000,OUT2,1
```

Vendor CSV exports may be converted to this format outside the project. Conversion must preserve the original capture timebase; do not round timestamps to scheduler ticks before analysis.

The standard analyzer is `scripts/analyze_hil_capture.py`. It supports descriptive summaries, pulse-width checks, expected-period checks, inter-channel skew and idle-vs-display-stress comparisons. Thresholds are supplied explicitly so analyzer output records the criterion used.

Examples:

```bash
python scripts/analyze_hil_capture.py summary capture.csv
python scripts/analyze_hil_capture.py pulse-width capture.csv --signal OUT1 --expected-us 10000 --tolerance-us 50
python scripts/analyze_hil_capture.py skew capture.csv --signals OUT1,OUT2,OUT3,OUT4,OUT5,OUT6,OUT7,OUT8 --max-skew-us 50
python scripts/analyze_hil_capture.py compare idle.csv stress.csv --signal OUT1 --max-added-deviation-us 50 --require-same-event-count
```

A failed analyzer command exits with status `2`; malformed or insufficient evidence exits with status `1`.

<h6 align="center">From Munich with &#9829;</h6>
