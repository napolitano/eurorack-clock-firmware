#!/usr/bin/env python3
"""Render narrated CLOCK Storybook tutorials locally from authored source.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0

The local publication contract is narration-first:
1. render/cache ElevenLabs narration segments,
2. measure real audio durations,
3. resolve Storybook holds from those durations,
4. render the sparse/change-driven picture timeline,
5. mux the narration onto the publication timeline.

Secrets are read from the process environment or repository-root .env. Environment
variables win over .env values. The API key is never written to output metadata.
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import math
import os
import re
import shutil
import subprocess
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable

ROOT = Path(__file__).resolve().parents[1]
EXAMPLES_DIR = ROOT / "docs" / "tutorials" / "examples"
VOICEOVER_DIR = ROOT / "docs" / "tutorials" / "voiceover"
SEGMENTS_FILE = VOICEOVER_DIR / "segments.json"
DEFAULT_OUTPUT_ROOT = ROOT / "tutorial-output"
DEFAULT_API_BASE = "https://api.elevenlabs.io"
DEFAULT_MODEL_SELECTOR = "auto-v4"
DEFAULT_OUTPUT_FORMAT = "mp3_44100_128"
DEFAULT_BODY_HEADROOM_MS = 1200
DEFAULT_OPENING_MIN_MS = 18000
DEFAULT_CLOSING_MIN_MS = 20000
DEFAULT_ROUNDING_MS = 100


class RenderError(RuntimeError):
    """Raised for an actionable local publication failure."""


@dataclass(frozen=True)
class LocalConfig:
    api_key: str
    voice_id: str
    model_selector: str
    output_format: str
    api_base: str
    body_headroom_ms: int
    opening_min_ms: int
    closing_min_ms: int
    rounding_ms: int


@dataclass(frozen=True)
class SegmentSpec:
    segment_id: str
    paragraphs: tuple[int, ...]
    text: str


@dataclass(frozen=True)
class SegmentAudio:
    segment_id: str
    audio_path: Path
    metadata_path: Path
    duration_ms: int
    cache_hit: bool


def log(message: str) -> None:
    print(message, flush=True)


def load_dotenv(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    if not path.is_file():
        return values
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("export "):
            line = line[7:].lstrip()
        if "=" not in line:
            raise RenderError(f"invalid .env line without '=': {raw!r}")
        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip()
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", key):
            raise RenderError(f"invalid .env variable name: {key!r}")
        if len(value) >= 2 and value[0] == value[-1] and value[0] in {'\"', "'"}:
            quote = value[0]
            value = value[1:-1]
            if quote == '\"':
                value = bytes(value, "utf-8").decode("unicode_escape")
        values[key] = value
    return values


def merged_environment(dotenv: Path) -> dict[str, str]:
    values = load_dotenv(dotenv)
    values.update(os.environ)
    return values


def positive_int(env: dict[str, str], key: str, default: int) -> int:
    raw = env.get(key, str(default)).strip()
    try:
        value = int(raw)
    except ValueError as exc:
        raise RenderError(f"{key} must be an integer") from exc
    if value <= 0:
        raise RenderError(f"{key} must be greater than zero")
    return value


def load_config(dotenv: Path, require_api: bool) -> LocalConfig:
    env = merged_environment(dotenv)
    api_key = env.get("ELEVENLABS_API_KEY", "").strip()
    voice_id = env.get("ELEVENLABS_VOICE_ID", "").strip()
    if require_api and not api_key:
        raise RenderError("ELEVENLABS_API_KEY is missing; copy .env.example to .env and set the local secret")
    if require_api and not voice_id:
        raise RenderError("ELEVENLABS_VOICE_ID is missing; set the cloned voice ID in local .env")
    output_format = env.get("ELEVENLABS_OUTPUT_FORMAT", DEFAULT_OUTPUT_FORMAT).strip() or DEFAULT_OUTPUT_FORMAT
    if not output_format.startswith("mp3_"):
        raise RenderError("the local tutorial runner currently requires an ElevenLabs mp3_* output format")
    return LocalConfig(
        api_key=api_key,
        voice_id=voice_id,
        model_selector=env.get("ELEVENLABS_MODEL_ID", DEFAULT_MODEL_SELECTOR).strip() or DEFAULT_MODEL_SELECTOR,
        output_format=output_format,
        api_base=env.get("ELEVENLABS_API_BASE", DEFAULT_API_BASE).strip().rstrip("/"),
        body_headroom_ms=positive_int(env, "CLOCK_TUTORIAL_BODY_HEADROOM_MS", DEFAULT_BODY_HEADROOM_MS),
        opening_min_ms=positive_int(env, "CLOCK_TUTORIAL_OPENING_MIN_MS", DEFAULT_OPENING_MIN_MS),
        closing_min_ms=positive_int(env, "CLOCK_TUTORIAL_CLOSING_MIN_MS", DEFAULT_CLOSING_MIN_MS),
        rounding_ms=positive_int(env, "CLOCK_TUTORIAL_ROUNDING_MS", DEFAULT_ROUNDING_MS),
    )


def request_json(
    method: str,
    url: str,
    api_key: str,
    payload: dict[str, Any] | None = None,
    attempts: int = 6,
) -> dict[str, Any] | list[Any]:
    body = None if payload is None else json.dumps(payload).encode("utf-8")
    headers = {"Accept": "application/json", "xi-api-key": api_key}
    if body is not None:
        headers["Content-Type"] = "application/json"
    for attempt in range(attempts):
        request = urllib.request.Request(url, data=body, headers=headers, method=method)
        try:
            with urllib.request.urlopen(request, timeout=120) as response:
                return json.loads(response.read().decode("utf-8"))
        except urllib.error.HTTPError as exc:
            message = exc.read().decode("utf-8", errors="replace")[:1200]
            retriable = exc.code == 429 or 500 <= exc.code < 600
            if not retriable or attempt + 1 >= attempts:
                raise RenderError(f"ElevenLabs HTTP {exc.code}: {message}") from exc
            retry_after = exc.headers.get("Retry-After")
            try:
                delay = float(retry_after) if retry_after else min(2 ** attempt, 20)
            except ValueError:
                delay = min(2 ** attempt, 20)
            log(f"ElevenLabs returned HTTP {exc.code}; retrying after {delay:.1f}s")
            time.sleep(max(0.5, delay))
        except urllib.error.URLError as exc:
            if attempt + 1 >= attempts:
                raise RenderError(f"ElevenLabs request failed: {exc.reason}") from exc
            delay = min(2 ** attempt, 20)
            log(f"ElevenLabs connection error; retrying after {delay:.1f}s")
            time.sleep(delay)
    raise RenderError("ElevenLabs request failed after retries")


def get_models(config: LocalConfig) -> list[dict[str, Any]]:
    raw = request_json("GET", f"{config.api_base}/v1/models", config.api_key)
    if isinstance(raw, dict):
        models = raw.get("models", [])
    else:
        models = raw
    if not isinstance(models, list):
        raise RenderError("unexpected /v1/models response")
    return [m for m in models if isinstance(m, dict)]


def model_label(model: dict[str, Any]) -> str:
    return f"{model.get('model_id', '?')} ({model.get('name', 'unnamed')})"


def resolve_model(config: LocalConfig) -> str:
    models = get_models(config)
    tts = [m for m in models if bool(m.get("can_do_text_to_speech"))]
    selector = config.model_selector
    if selector != DEFAULT_MODEL_SELECTOR:
        match = next((m for m in tts if m.get("model_id") == selector), None)
        if match is None:
            visible = ", ".join(model_label(m) for m in tts)
            raise RenderError(f"ELEVENLABS_MODEL_ID={selector!r} is not an accessible TTS model. Available: {visible}")
        return selector

    candidates = [
        m for m in tts
        if "v4" in str(m.get("model_id", "")).lower() or "v4" in str(m.get("name", "")).lower()
    ]
    if len(candidates) == 1:
        selected = str(candidates[0]["model_id"])
        log(f"Resolved auto-v4 to {model_label(candidates[0])}")
        return selected
    visible = ", ".join(model_label(m) for m in tts)
    if not candidates:
        raise RenderError(
            "ELEVENLABS_MODEL_ID=auto-v4 found no accessible TTS model containing 'v4'. "
            f"Set the exact model ID from your account in .env. Accessible TTS models: {visible}"
        )
    raise RenderError(
        "ELEVENLABS_MODEL_ID=auto-v4 is ambiguous. Set the exact v4 model ID in .env. "
        "Matches: " + ", ".join(model_label(m) for m in candidates)
    )


def voiceover_paragraphs(path: Path) -> list[str]:
    paragraphs = [chunk.strip() for chunk in re.split(r"\n\s*\n", path.read_text(encoding="utf-8")) if chunk.strip()]
    if not paragraphs:
        raise RenderError(f"voice-over script contains no paragraphs: {path}")
    return paragraphs


def load_segment_catalog() -> dict[str, dict[str, Any]]:
    data = json.loads(SEGMENTS_FILE.read_text(encoding="utf-8"))
    if data.get("schema") != 1 or not isinstance(data.get("stories"), dict):
        raise RenderError(f"unsupported narration segment catalog: {SEGMENTS_FILE}")
    return data["stories"]


def segment_specs(stem: str, entry: dict[str, Any]) -> list[SegmentSpec]:
    script_path = VOICEOVER_DIR / str(entry["voiceover"])
    paragraphs = voiceover_paragraphs(script_path)
    result: list[SegmentSpec] = []
    seen: set[str] = set()
    for raw in entry.get("segments", []):
        segment_id = str(raw.get("id", ""))
        indices = tuple(int(value) for value in raw.get("paragraphs", []))
        if not re.fullmatch(r"s\d{2,}", segment_id) or segment_id in seen:
            raise RenderError(f"invalid/duplicate segment ID in {stem}: {segment_id!r}")
        if not indices or min(indices) < 1 or max(indices) > len(paragraphs):
            raise RenderError(f"paragraph index outside voice-over script in {stem}/{segment_id}")
        seen.add(segment_id)
        text = "\n\n".join(paragraphs[index - 1] for index in indices)
        result.append(SegmentSpec(segment_id, indices, text))
    if not result:
        raise RenderError(f"no narration segments defined for {stem}")
    return result


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def run_checked(args: list[str], cwd: Path = ROOT, capture: bool = False) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(args, cwd=cwd, text=True, capture_output=capture, check=False)
    if result.returncode != 0:
        details = ""
        if capture:
            details = "\n" + (result.stdout or "") + (result.stderr or "")
        raise RenderError(f"command failed ({result.returncode}): {' '.join(args)}{details}")
    return result


def executable(name: str, configured: str | None = None) -> str:
    if configured:
        candidate = Path(configured).expanduser()
        if candidate.is_file():
            return str(candidate.resolve())
        found = shutil.which(configured)
        if found:
            return found
        raise RenderError(f"cannot locate executable: {configured}")
    found = shutil.which(name)
    if not found:
        raise RenderError(f"{name} is not available on PATH")
    return found


def probe_duration_ms(ffprobe: str, media: Path) -> int:
    result = run_checked([
        ffprobe, "-v", "error", "-show_entries", "format=duration",
        "-of", "default=noprint_wrappers=1:nokey=1", str(media),
    ], capture=True)
    raw = result.stdout.strip()
    try:
        seconds = float(raw)
    except ValueError as exc:
        raise RenderError(f"ffprobe returned invalid duration for {media}: {raw!r}") from exc
    if not math.isfinite(seconds) or seconds <= 0:
        raise RenderError(f"invalid media duration for {media}: {seconds}")
    return max(1, int(math.ceil(seconds * 1000.0)))


def alignment_duration_ms(response: dict[str, Any]) -> int | None:
    for key in ("normalized_alignment", "alignment"):
        alignment = response.get(key)
        if not isinstance(alignment, dict):
            continue
        ends = alignment.get("character_end_times_seconds")
        if isinstance(ends, list) and ends:
            try:
                seconds = float(ends[-1])
            except (TypeError, ValueError):
                continue
            if math.isfinite(seconds) and seconds > 0:
                return int(math.ceil(seconds * 1000.0))
    return None


def synthesize_segment(
    config: LocalConfig,
    model_id: str,
    stem: str,
    spec: SegmentSpec,
    audio_dir: Path,
    ffprobe: str,
    force: bool,
) -> SegmentAudio:
    audio_dir.mkdir(parents=True, exist_ok=True)
    audio_path = audio_dir / f"{spec.segment_id}.mp3"
    metadata_path = audio_dir / f"{spec.segment_id}.json"
    cache_payload = {
        "text_sha256": sha256_text(spec.text),
        "voice_id": config.voice_id,
        "model_id": model_id,
        "output_format": config.output_format,
    }
    cache_key = hashlib.sha256(json.dumps(cache_payload, sort_keys=True).encode("utf-8")).hexdigest()
    if not force and audio_path.is_file() and metadata_path.is_file():
        try:
            existing = json.loads(metadata_path.read_text(encoding="utf-8"))
            if existing.get("cache_key") == cache_key:
                duration_ms = probe_duration_ms(ffprobe, audio_path)
                return SegmentAudio(spec.segment_id, audio_path, metadata_path, duration_ms, True)
        except (json.JSONDecodeError, OSError, RenderError):
            pass

    encoded_voice = urllib.parse.quote(config.voice_id, safe="")
    query = urllib.parse.urlencode({"output_format": config.output_format})
    url = f"{config.api_base}/v1/text-to-speech/{encoded_voice}/with-timestamps?{query}"
    response = request_json("POST", url, config.api_key, {"text": spec.text, "model_id": model_id})
    if not isinstance(response, dict) or not isinstance(response.get("audio_base64"), str):
        raise RenderError(f"ElevenLabs returned no audio for {stem}/{spec.segment_id}")
    try:
        audio = base64.b64decode(response["audio_base64"], validate=True)
    except Exception as exc:
        raise RenderError(f"ElevenLabs returned invalid base64 audio for {stem}/{spec.segment_id}") from exc
    if not audio:
        raise RenderError(f"ElevenLabs returned empty audio for {stem}/{spec.segment_id}")
    temp_path = audio_path.with_suffix(".mp3.tmp")
    temp_path.write_bytes(audio)
    temp_path.replace(audio_path)
    try:
        duration_ms = probe_duration_ms(ffprobe, audio_path)
    except RenderError:
        duration_ms = alignment_duration_ms(response) or 0
        if duration_ms <= 0:
            raise
    metadata = {
        "schema": 1,
        "story": stem,
        "segment_id": spec.segment_id,
        "paragraphs": list(spec.paragraphs),
        "cache_key": cache_key,
        **cache_payload,
        "duration_ms": duration_ms,
    }
    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    return SegmentAudio(spec.segment_id, audio_path, metadata_path, duration_ms, False)


def cached_segment_audio(
    config: LocalConfig,
    model_id: str,
    stem: str,
    spec: SegmentSpec,
    audio_dir: Path,
    ffprobe: str,
) -> SegmentAudio:
    audio_path = audio_dir / f"{spec.segment_id}.mp3"
    metadata_path = audio_dir / f"{spec.segment_id}.json"
    if not audio_path.is_file() or not metadata_path.is_file():
        raise RenderError(f"cached narration missing for {stem}/{spec.segment_id}; run without --picture-only first")
    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    expected = {
        "text_sha256": sha256_text(spec.text),
        "voice_id": config.voice_id,
        "model_id": model_id,
        "output_format": config.output_format,
    }
    for key, value in expected.items():
        if metadata.get(key) != value:
            raise RenderError(f"cached narration is stale for {stem}/{spec.segment_id} ({key} changed)")
    return SegmentAudio(spec.segment_id, audio_path, metadata_path, probe_duration_ms(ffprobe, audio_path), True)


def round_up_ms(value: int, quantum: int) -> int:
    return ((value + quantum - 1) // quantum) * quantum


def segment_budgets(
    config: LocalConfig,
    audio: list[SegmentAudio],
    body_headroom_ms: int | None = None,
) -> dict[str, int]:
    budgets: dict[str, int] = {}
    headroom_ms = config.body_headroom_ms if body_headroom_ms is None else body_headroom_ms
    if headroom_ms <= 0:
        raise RenderError("narration body headroom must be greater than zero")
    for index, item in enumerate(audio):
        budget = item.duration_ms + headroom_ms
        if index == 0:
            budget = max(budget, config.opening_min_ms)
        if index + 1 == len(audio):
            budget = max(budget, config.closing_min_ms)
        budgets[item.segment_id] = round_up_ms(budget, config.rounding_ms)
    return budgets


def resolve_story(stem: str, budgets: dict[str, int], output_path: Path) -> dict[str, int]:
    source = EXAMPLES_DIR / f"{stem}.yaml"
    lines = source.read_text(encoding="utf-8").splitlines()
    seen: set[str] = set()
    resolved_budgets: dict[str, int] = {}
    narration_pattern = re.compile(r'^\s*(?:-\s*)?narration:\s*"?([a-z0-9-]+)"?\s*$')
    duration_pattern = re.compile(r'^(\s*)(duration_ms|wait_ms):\s*\d+\s*$')
    for index, line in enumerate(lines):
        match = narration_pattern.match(line)
        if not match:
            continue
        segment_id = match.group(1)
        if segment_id not in budgets:
            raise RenderError(f"story {stem} contains narration ID not present in segments.json: {segment_id}")
        if segment_id in seen:
            raise RenderError(f"story {stem} contains duplicate narration ID: {segment_id}")
        seen.add(segment_id)
        base_indent = len(line) - len(line.lstrip(" "))
        replaced = False
        for probe in range(index + 1, min(len(lines), index + 18)):
            if narration_pattern.match(lines[probe]):
                break
            stripped = lines[probe].strip()
            if not stripped or stripped.startswith("#"):
                continue
            indent = len(lines[probe]) - len(lines[probe].lstrip(" "))
            if indent < base_indent:
                break
            dm = duration_pattern.match(lines[probe])
            if dm:
                authored_budget = int(lines[probe].split(":", 1)[1].strip())
                resolved_budget = max(authored_budget, budgets[segment_id])
                lines[probe] = f"{dm.group(1)}{dm.group(2)}: {resolved_budget}"
                resolved_budgets[segment_id] = resolved_budget
                replaced = True
                break
        if not replaced:
            raise RenderError(f"story {stem}/{segment_id} has no bounded duration_ms or wait_ms")
    missing = set(budgets) - seen
    if missing:
        raise RenderError(f"segments.json contains narration IDs not anchored in {stem}: {sorted(missing)}")

    asset_pattern = re.compile(r'^(\s*)(intro_video|outro_video):\s*(.*?)\s*$')
    for index, line in enumerate(lines):
        match = asset_pattern.match(line)
        if not match:
            continue
        raw_reference = match.group(3).strip()
        if len(raw_reference) >= 2 and raw_reference[0] == raw_reference[-1] and raw_reference[0] in {"\"", "'"}:
            raw_reference = raw_reference[1:-1]
        asset = Path(raw_reference)
        if not asset.is_absolute():
            asset = source.parent / asset
        asset = asset.resolve()
        if not asset.is_file():
            raise RenderError(f"story {stem} references missing publication asset: {asset}")
        normalized = asset.as_posix().replace('"', '\\"')
        lines[index] = f'{match.group(1)}{match.group(2)}: "{normalized}"'

    resolved = "\n".join(lines) + "\n"
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(resolved, encoding="utf-8")
    return resolved_budgets


def write_timing_plan(
    stem: str,
    model_id: str,
    audio: list[SegmentAudio],
    budgets: dict[str, int],
    output_path: Path,
) -> None:
    payload = {
        "schema": 1,
        "story": stem,
        "model_id": model_id,
        "segments": [
            {
                "id": item.segment_id,
                "audio_duration_ms": item.duration_ms,
                "story_budget_ms": budgets[item.segment_id],
                "cache_hit": item.cache_hit,
                "audio": item.audio_path.relative_to(DEFAULT_OUTPUT_ROOT if DEFAULT_OUTPUT_ROOT in item.audio_path.parents else item.audio_path.parent).as_posix()
                if item.audio_path.is_absolute() else item.audio_path.as_posix(),
            }
            for item in audio
        ],
    }
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def build_storybook(skip_build: bool) -> Path:
    build_dir = ROOT / "build" / "simulator-headless-release"
    binary = build_dir / ("clock-storybook.exe" if os.name == "nt" else "clock-storybook")
    if skip_build:
        if not binary.is_file():
            raise RenderError(f"--skip-build requested but {binary} does not exist")
        return binary
    run_checked(["cmake", "--preset", "simulator-headless-release"])
    run_checked(["cmake", "--build", "--preset", "simulator-headless-release", "--target", "clock-storybook"])
    if not binary.is_file():
        raise RenderError(f"Storybook build completed but executable is missing: {binary}")
    return binary


def source_revision() -> str:
    git = shutil.which("git")
    if not git or not (ROOT / ".git").exists():
        return "working-tree"
    result = subprocess.run([git, "rev-parse", "HEAD"], cwd=ROOT, text=True, capture_output=True, check=False)
    return result.stdout.strip() if result.returncode == 0 and result.stdout.strip() else "working-tree"


def probe_has_audio(ffprobe: str, media: Path) -> bool:
    result = run_checked([
        ffprobe, "-v", "error", "-select_streams", "a:0", "-show_entries", "stream=codec_type",
        "-of", "default=noprint_wrappers=1:nokey=1", str(media),
    ], capture=True)
    return bool(result.stdout.strip())


def mix_narration(
    ffmpeg: str,
    ffprobe: str,
    publication_video: Path,
    narration_manifest: Path,
    audio: list[SegmentAudio],
    output_path: Path,
) -> None:
    manifest = json.loads(narration_manifest.read_text(encoding="utf-8"))
    cues = manifest.get("cues", [])
    by_id = {item.segment_id: item for item in audio}
    if {str(cue.get("id")) for cue in cues} != set(by_id):
        raise RenderError("publication narration cue IDs do not match rendered narration segments")
    total_ms = probe_duration_ms(ffprobe, publication_video)
    has_base_audio = probe_has_audio(ffprobe, publication_video)

    args = [ffmpeg, "-y", "-v", "error", "-i", str(publication_video)]
    base_index: int
    next_index = 1
    if has_base_audio:
        base_index = 0
    else:
        args += ["-f", "lavfi", "-t", f"{total_ms / 1000.0:.3f}", "-i",
                 "anullsrc=channel_layout=stereo:sample_rate=48000"]
        base_index = 1
        next_index = 2

    ordered: list[tuple[dict[str, Any], SegmentAudio, int]] = []
    for cue in cues:
        item = by_id[str(cue["id"])]
        input_index = next_index
        next_index += 1
        args += ["-i", str(item.audio_path)]
        ordered.append((cue, item, input_index))

    filters = [f"[{base_index}:a:0]aresample=48000,aformat=channel_layouts=stereo[basea]"]
    labels = ["[basea]"]
    for order, (cue, _item, input_index) in enumerate(ordered):
        delay_ms = int(round(int(cue["start_us"]) / 1000.0))
        label = f"n{order}"
        filters.append(
            f"[{input_index}:a:0]aresample=48000,aformat=channel_layouts=stereo,"
            f"adelay={delay_ms}:all=1[{label}]"
        )
        labels.append(f"[{label}]")
    filters.append("".join(labels) + f"amix=inputs={len(labels)}:normalize=0:dropout_transition=0[aout]")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    temp_path = output_path.with_suffix(".tmp.mp4")
    args += [
        "-filter_complex", ";".join(filters), "-map", "0:v:0", "-map", "[aout]",
        "-c:v", "copy", "-c:a", "aac", "-b:a", "192k", "-ar", "48000", "-ac", "2",
        "-t", f"{total_ms / 1000.0:.3f}", "-movflags", "+faststart", str(temp_path),
    ]
    run_checked(args)
    temp_path.replace(output_path)


def story_number(stem: str) -> str:
    return stem.split("-", 1)[0]


def story_title(stem: str) -> str:
    story = EXAMPLES_DIR / f"{stem}.yaml"
    for line in story.read_text(encoding="utf-8").splitlines():
        if line.startswith("title:"):
            return line.split(":", 1)[1].strip().strip('"\'')
    raise RenderError(f"tutorial has no top-level title: {story}")


def select_stories(target: str | None, render_all: bool, catalog: dict[str, dict[str, Any]]) -> list[str]:
    stems = sorted(catalog)
    if render_all:
        return stems
    if not target:
        raise RenderError("specify a tutorial name or --all")
    normalized = target.strip().lower()
    matches: list[str] = []
    for stem in stems:
        number = story_number(stem)
        source_name = stem.split("-", 1)[1] if "-" in stem else stem
        aliases = {
            stem.lower(),
            source_name.lower(),
            story_title(stem).lower(),
            number,
            number.lstrip("0"),
        }
        if normalized in aliases:
            matches.append(stem)
    if len(matches) != 1:
        raise RenderError(
            f"cannot resolve tutorial {target!r}; use its exact tutorial name, filename stem, or internal source number"
        )
    return matches


def render_one(
    stem: str,
    entry: dict[str, Any],
    config: LocalConfig,
    model_id: str,
    ffmpeg: str,
    ffprobe: str,
    storybook: Path | None,
    output_root: Path,
    force_tts: bool,
    audio_only: bool,
    picture_only: bool,
) -> dict[str, Any]:
    log(f"\n=== {stem} ===")
    specs = segment_specs(stem, entry)
    audio_dir = output_root / "audio" / stem
    audio: list[SegmentAudio] = []
    for index, spec in enumerate(specs, 1):
        log(f"Narration {index}/{len(specs)}: {spec.segment_id}")
        if picture_only:
            item = cached_segment_audio(config, model_id, stem, spec, audio_dir, ffprobe)
        else:
            item = synthesize_segment(config, model_id, stem, spec, audio_dir, ffprobe, force_tts)
        audio.append(item)
        log(f"  {item.duration_ms / 1000.0:.2f}s {'(cache)' if item.cache_hit else '(rendered)'}")

    story_headroom = entry.get("headroom_ms")
    if story_headroom is not None and not isinstance(story_headroom, int):
        raise RenderError(f"segments.json headroom_ms for {stem} must be an integer")
    budgets = segment_budgets(config, audio, story_headroom)
    resolved_story = output_root / "resolved-stories" / f"{stem}.yaml"
    resolved_budgets = resolve_story(stem, budgets, resolved_story)
    timing_plan = output_root / "plans" / f"{stem}.json"
    write_timing_plan(stem, model_id, audio, resolved_budgets, timing_plan)
    log(f"Resolved story: {resolved_story}")
    if audio_only:
        return {"story": stem, "audio_only": True, "segments": len(audio), "plan": str(timing_plan)}
    if storybook is None:
        raise RenderError("internal error: Storybook executable not available")

    run_checked([str(storybook), "validate", str(resolved_story)])
    publication_dir = output_root / "video" / stem
    run_checked([
        str(storybook), "video", str(resolved_story), "--output", str(publication_dir),
        "--source-revision", source_revision(), "--format", "mp4",
        "--ffmpeg", ffmpeg, "--ffprobe", ffprobe,
    ])
    manifest = json.loads((publication_dir / "publication-manifest.json").read_text(encoding="utf-8"))
    story_id = str(manifest["story_id"])
    publication_video = publication_dir / f"{story_id}.mp4"
    narration_manifest = publication_dir / f"{story_id}.narration.json"
    if not publication_video.is_file() or not narration_manifest.is_file():
        raise RenderError(f"Storybook publication did not produce expected media/voice timing for {stem}")
    final_path = output_root / "final" / f"{stem}.mp4"
    mix_narration(ffmpeg, ffprobe, publication_video, narration_manifest, audio, final_path)
    duration_ms = probe_duration_ms(ffprobe, final_path)
    log(f"Final master: {final_path} ({duration_ms / 1000.0:.2f}s)")
    return {
        "story": stem,
        "segments": len(audio),
        "final": str(final_path),
        "duration_ms": duration_ms,
        "plan": str(timing_plan),
    }


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Render narrated CLOCK Storybook publication masters locally")
    parser.add_argument("tutorial", nargs="?", help="tutorial name, filename stem, or internal source number")
    parser.add_argument("--all", action="store_true", help="render all editable tutorials")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--audio-only", action="store_true", help="render/cache narration and resolve timing, but do not render picture")
    mode.add_argument("--picture-only", action="store_true", help="reuse matching cached narration; render picture and final mux only")
    parser.add_argument("--force-tts", action="store_true", help="regenerate narration even when the segment cache matches")
    parser.add_argument("--skip-build", action="store_true", help="reuse an existing Release clock-storybook executable")
    parser.add_argument("--env-file", type=Path, default=ROOT / ".env", help="local environment file (default: repository-root .env)")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT_ROOT, help="ignored local output root")
    parser.add_argument("--ffmpeg", help="ffmpeg executable/path override")
    parser.add_argument("--ffprobe", help="ffprobe executable/path override")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        catalog = load_segment_catalog()
        stories = select_stories(args.tutorial, args.all, catalog)
        ffmpeg = executable("ffmpeg", args.ffmpeg) if not args.audio_only else (args.ffmpeg or "ffmpeg")
        ffprobe = executable("ffprobe", args.ffprobe)
        config = load_config(args.env_file.resolve(), require_api=not args.picture_only)
        # Picture-only still validates cache identity. It therefore needs voice/model configuration,
        # but can run without an API key because no network request is performed.
        if args.picture_only and not config.voice_id:
            raise RenderError("ELEVENLABS_VOICE_ID is required to validate the cached narration identity")
        if args.picture_only:
            if config.model_selector == DEFAULT_MODEL_SELECTOR:
                # Cache metadata contains the resolved model; auto-v4 cannot be resolved offline without an API key.
                first_meta = args.output.resolve() / "audio" / stories[0] / "s01.json"
                if not first_meta.is_file():
                    raise RenderError("--picture-only with auto-v4 needs an existing narration cache or an exact model ID")
                model_id = str(json.loads(first_meta.read_text(encoding="utf-8")).get("model_id", ""))
                if not model_id:
                    raise RenderError("cached narration metadata has no model_id")
            else:
                model_id = config.model_selector
        else:
            model_id = resolve_model(config)
        log(f"ElevenLabs model: {model_id}")
        output_root = args.output.resolve()
        output_root.mkdir(parents=True, exist_ok=True)
        storybook = None if args.audio_only else build_storybook(args.skip_build)
        results = []
        for stem in stories:
            results.append(render_one(
                stem, catalog[stem], config, model_id, ffmpeg, ffprobe, storybook, output_root,
                args.force_tts, args.audio_only, args.picture_only,
            ))
        report = {
            "schema": 1,
            "model_id": model_id,
            "voice_id": config.voice_id,
            "output_format": config.output_format,
            "results": results,
        }
        report_path = output_root / "render-report.json"
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        log(f"\nRender report: {report_path}")
        return 0
    except RenderError as exc:
        print(f"render_tutorials.py: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
