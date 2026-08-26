#!/usr/bin/env python3
"""Coordinate local validation evidence and the deliberately narrow release action."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import platform
import re
import subprocess
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
CONFIG_PATH = ROOT / "config/validation.json"
RUNS = ROOT / "validation-runs"
WORKTREES = ROOT / "validation-worktrees"


def command(args: list[str], cwd: Path = ROOT, check: bool = True) -> str:
    result = subprocess.run(args, cwd=cwd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if check and result.returncode:
        raise RuntimeError(f"command failed ({result.returncode}): {' '.join(args)}\n{result.stdout}")
    return result.stdout


def git(*args: str, check: bool = True) -> str:
    return command(["git", *args], check=check).strip()


def config() -> dict[str, Any]:
    return json.loads(CONFIG_PATH.read_text())


def ref_sha(ref: str) -> str:
    return git("rev-parse", f"{ref}^{{commit}}")


def ensure_cutover(settings: dict[str, Any]) -> str:
    tag = settings["cutover_tag"]
    if not git("rev-parse", "-q", "--verify", f"refs/tags/{tag}", check=False):
        raise RuntimeError(f"cutover tag `{tag}` is absent; merge this workflow, then run `just bootstrap-validation` on current main")
    return tag


def select_baseline(settings: dict[str, Any]) -> tuple[str, str]:
    cutover = ensure_cutover(settings)
    tags = git("for-each-ref", "--format=%(refname:short)", "--sort=-version:refname", "refs/tags/v*").splitlines()
    for tag in tags:
        if not re.fullmatch(r"v[0-9]+(?:\.[0-9]+){1,2}(?:[-+][0-9A-Za-z.-]+)?", tag):
            continue
        if git("cat-file", "-t", f"refs/tags/{tag}") != "tag":
            continue
        if subprocess.run(["git", "merge-base", "--is-ancestor", f"{cutover}^{{commit}}", f"{tag}^{{commit}}"], cwd=ROOT).returncode == 0:
            return tag, ref_sha(tag)
    return cutover, ref_sha(cutover)


def require_current_main() -> None:
    if git("branch", "--show-current") != "main":
        raise RuntimeError("must run on local main")
    if git("status", "--porcelain"):
        raise RuntimeError("working tree must be clean")
    remote = git("rev-parse", "-q", "--verify", "refs/remotes/origin/main", check=False)
    if not remote or git("rev-parse", "HEAD") != remote:
        raise RuntimeError("HEAD must equal refs/remotes/origin/main; fetch or update main first")


def new_run(label: str) -> Path:
    stamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    path = RUNS / f"{stamp}-{label}"
    path.mkdir(parents=True, exist_ok=False)
    return path


def stream_command(args: list[str], cwd: Path = ROOT) -> None:
    result = subprocess.run(args, cwd=cwd)
    if result.returncode:
        raise RuntimeError(f"command failed ({result.returncode}): {' '.join(args)}")


def add_worktree(path: Path, ref: str) -> None:
    command(["git", "worktree", "add", "--detach", str(path), ref])


def build(worktree: Path, mode: str, output: Path) -> None:
    print(f"[validation] building {mode} binary: {output}", flush=True)
    stream_command(["just", "build", mode, str(output), "1"], cwd=worktree)


def result_entry(args: list[str], cwd: Path, log: Path, label: str) -> dict[str, Any]:
    print(f"[validation] starting {label}", flush=True)
    log.parent.mkdir(parents=True, exist_ok=True)
    with log.open("w") as output:
        process = subprocess.Popen(args, cwd=cwd, text=True, stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, bufsize=1)
        assert process.stdout is not None
        for line in process.stdout:
            sys.stdout.write(line)
            sys.stdout.flush()
            output.write(line)
            output.flush()
        process.wait()
    passed = process.returncode == 0
    print(f"[validation] {label} {'passed' if passed else 'failed'}; log: {log}", flush=True)
    return {"passed": passed, "exit_code": process.returncode, "log": str(log)}


def identical_sprt(output_dir: Path, log: Path, baseline_sha: str) -> dict[str, Any]:
    """Record the vacuous strength result when candidate and baseline are identical."""
    output_dir.mkdir(parents=True, exist_ok=True)
    message = ("SPRT skipped: candidate and baseline resolve to the same commit "
               f"({baseline_sha}); there is no strength delta to test.\n")
    log.write_text(message)
    evidence = {"verdict": "identical", "passed": True, "skipped": True,
                "reason": message.strip(), "baseline_sha": baseline_sha,
                "candidate_sha": baseline_sha, "log": str(log)}
    (output_dir / "sprt.json").write_text(json.dumps(evidence, indent=2) + "\n")
    print(message.strip(), flush=True)
    return {"passed": True, "exit_code": 0, "log": str(log)}


def write_manifest(path: Path, data: dict[str, Any]) -> Path:
    target = path / "manifest.json"
    target.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n")
    return target


def read_json(path: Path) -> dict[str, Any]:
    try:
        return json.loads(path.read_text())
    except (OSError, json.JSONDecodeError):
        return {"available": False}


def machine_data() -> dict[str, str]:
    return {"macos_version": platform.mac_ver()[0], "architecture": platform.machine(),
            "compiler": command(["cc", "--version"]).splitlines()[0]}


def local_match(kind: str, baseline_arg: str | None) -> int:
    settings = config()
    baseline_ref, baseline_sha = (baseline_arg, ref_sha(baseline_arg)) if baseline_arg else select_baseline(settings)
    run = new_run(kind)
    print(f"[validation] {kind}: baseline {baseline_ref} ({baseline_sha})", flush=True)
    baseline_tree = WORKTREES / run.name / "baseline"
    add_worktree(baseline_tree, baseline_ref)
    baseline_bin, candidate_bin = run / "baseline", run / "candidate"
    build(baseline_tree, "release", baseline_bin)
    build(ROOT, "release", candidate_bin)
    if kind == "benchmark":
        args = [sys.executable, "-u", str(ROOT / "scripts/bench.py"), "--baseline", str(baseline_bin), "--candidate", str(candidate_bin),
                "--config", str(CONFIG_PATH), "--output", str(run / "benchmark.json")]
        entry = result_entry(args, ROOT, run / "benchmark.log", "comparative benchmark")
    else:
        candidate_sha = git("rev-parse", "HEAD")
        if candidate_sha == baseline_sha:
            entry = identical_sprt(run / "sprt", run / "sprt.log", baseline_sha)
        else:
            args = [sys.executable, "-u", str(ROOT / "scripts/sprt.py"), "--baseline", str(baseline_bin), "--candidate", str(candidate_bin),
                    "--config", str(CONFIG_PATH), "--output-dir", str(run / "sprt")]
            entry = result_entry(args, ROOT, run / "sprt.log", "SPRT")
    print(f"baseline: {baseline_ref} ({baseline_sha})", flush=True)
    print(f"{kind} {'passed' if entry['passed'] else 'failed'}; evidence: {run}", flush=True)
    return 0 if entry["passed"] else 1


def validate(version: str, speed_override: bool) -> int:
    settings = config()
    baseline_ref, baseline_sha = select_baseline(settings)
    candidate_sha = git("rev-parse", "HEAD")
    run = new_run(version)
    print(f"[validation] validating {version} at {candidate_sha}", flush=True)
    print(f"[validation] selected baseline {baseline_ref} ({baseline_sha})", flush=True)
    baseline_tree, candidate_tree = WORKTREES / run.name / "baseline", WORKTREES / run.name / "candidate"
    add_worktree(baseline_tree, baseline_ref)
    add_worktree(candidate_tree, candidate_sha)
    baseline_bin, candidate_bin = run / "bin" / "baseline", run / "bin" / "candidate"
    build(baseline_tree, "release", baseline_bin)
    build(candidate_tree, "release", candidate_bin)
    checks = result_entry(["just", "check"], candidate_tree, run / "check.log", "strict checks")
    benchmark = result_entry([sys.executable, "-u", str(ROOT / "scripts/bench.py"), "--baseline", str(baseline_bin), "--candidate", str(candidate_bin),
                              "--config", str(CONFIG_PATH), "--output", str(run / "benchmark.json")], ROOT, run / "benchmark.log", "comparative benchmark")
    if candidate_sha == baseline_sha:
        sprt = identical_sprt(run / "sprt", run / "sprt.log", baseline_sha)
    else:
        sprt = result_entry([sys.executable, "-u", str(ROOT / "scripts/sprt.py"), "--baseline", str(baseline_bin), "--candidate", str(candidate_bin),
                             "--config", str(CONFIG_PATH), "--output-dir", str(run / "sprt")], ROOT, run / "sprt.log", "SPRT")
    benchmark["result"] = read_json(run / "benchmark.json")
    sprt["result"] = read_json(run / "sprt" / "sprt.json")
    manifest = {
        "version": version, "candidate": {"ref": "HEAD", "sha": candidate_sha},
        "baseline": {"ref": baseline_ref, "sha": baseline_sha}, "machine": machine_data(),
        "release": settings["release"], "configuration": settings,
        "checks": checks, "benchmark": benchmark, "sprt": sprt,
        "artifacts": {"run": str(run), "baseline_binary": str(baseline_bin), "candidate_binary": str(candidate_bin),
                      "benchmark": str(run / "benchmark.json"), "sprt": str(run / "sprt" / "sprt.json")},
        "speed_override_requested": speed_override,
    }
    manifest_path = write_manifest(run, manifest)
    speed_ok = benchmark["passed"] or speed_override
    passed = checks["passed"] and speed_ok and sprt["passed"]
    print(f"baseline: {baseline_ref} ({baseline_sha})", flush=True)
    print(f"validation {'passed' if passed else 'failed'}; manifest: {manifest_path}", flush=True)
    return 0 if passed else 1


def bootstrap() -> int:
    settings = config()
    require_current_main()
    tag = settings["cutover_tag"]
    if git("rev-parse", "-q", "--verify", f"refs/tags/{tag}", check=False):
        raise RuntimeError(f"refusing to replace existing cutover tag `{tag}`")
    command(["git", "tag", "-a", tag, "-m", "Validation cutover 2026-08-24", "HEAD"])
    command(["git", "push", "origin", f"refs/tags/{tag}"])
    print(f"created and pushed {tag}")
    return 0


def release(version: str, manifest_arg: str | None, speed_override: bool) -> int:
    if not re.fullmatch(r"[0-9]+(?:\.[0-9]+){1,2}(?:[-+][0-9A-Za-z.-]+)?", version):
        raise RuntimeError("version must omit the leading v and use a semantic version")
    settings = config()
    require_current_main()
    ensure_cutover(settings)
    baseline_ref, baseline_sha = select_baseline(settings)
    tag = f"v{version}"
    if git("rev-parse", "-q", "--verify", f"refs/tags/{tag}", check=False):
        raise RuntimeError(f"tag already exists: {tag}")
    candidates = [Path(manifest_arg)] if manifest_arg else sorted(RUNS.glob("*/manifest.json"), reverse=True)
    manifest_path = None
    manifest: dict[str, Any] | None = None
    for item in candidates:
        if not item.is_file():
            continue
        try:
            loaded = json.loads(item.read_text())
        except json.JSONDecodeError:
            continue
        if loaded.get("version") == version and loaded.get("candidate", {}).get("sha") == git("rev-parse", "HEAD") and loaded.get("baseline", {}).get("sha") == baseline_sha:
            manifest_path, manifest = item, loaded
            break
    if not manifest_path:
        raise RuntimeError("no validation manifest found; pass `manifest=<path>`")
    assert manifest is not None
    if not manifest.get("checks", {}).get("passed"):
        raise RuntimeError("strict checks did not pass")
    bench_ok = manifest.get("benchmark", {}).get("passed")
    if not bench_ok and not (speed_override and manifest.get("speed_override_requested")):
        raise RuntimeError("benchmark failed; rerun validate with a recorded speed override and pass speed_override=1")
    if not manifest.get("sprt", {}).get("passed"):
        raise RuntimeError("SPRT is not accepted")
    command(["git", "tag", "-a", tag, "-F", str(manifest_path), "HEAD"])
    command(["git", "push", "origin", f"refs/tags/{tag}"])
    print(f"created and pushed {tag}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    actions = parser.add_subparsers(dest="action", required=True)
    actions.add_parser("baseline")
    actions.add_parser("bootstrap")
    for name in ("benchmark", "sprt"):
        item = actions.add_parser(name)
        item.add_argument("--baseline")
    item = actions.add_parser("validate")
    item.add_argument("version")
    item.add_argument("--speed-override", action="store_true")
    item = actions.add_parser("release")
    item.add_argument("version")
    item.add_argument("--manifest")
    item.add_argument("--speed-override", action="store_true")
    args = parser.parse_args()
    try:
        if args.action == "baseline":
            ref, sha = select_baseline(config()); print(f"{ref} {sha}", flush=True); return 0
        if args.action == "bootstrap": return bootstrap()
        if args.action in ("benchmark", "sprt"): return local_match(args.action, args.baseline)
        if args.action == "validate": return validate(args.version, args.speed_override)
        return release(args.version, args.manifest, args.speed_override)
    except RuntimeError as error:
        print(f"validation error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
