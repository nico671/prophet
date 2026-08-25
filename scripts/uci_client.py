"""Small, strict UCI process helper shared by local validation tools."""

from __future__ import annotations

import queue
import subprocess
import threading
import time
from pathlib import Path
from typing import Callable


class UciError(RuntimeError):
    pass


class UciEngine:
    def __init__(self, engine: str | Path, timeout: float = 30.0) -> None:
        self.timeout = timeout
        self.process = subprocess.Popen(
            [str(engine)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, text=True, bufsize=1,
        )
        self.lines: queue.Queue[str | None] = queue.Queue()
        assert self.process.stdout is not None
        self.reader = threading.Thread(target=self._read, daemon=True)
        self.reader.start()

    def _read(self) -> None:
        assert self.process.stdout is not None
        for line in self.process.stdout:
            self.lines.put(line.rstrip("\r\n"))
        self.lines.put(None)

    def send(self, command: str) -> None:
        if self.process.poll() is not None:
            raise UciError(f"engine exited before `{command}` (status {self.process.returncode})")
        assert self.process.stdin is not None
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()

    def expect(self, predicate: Callable[[str], bool], description: str,
               timeout: float | None = None) -> tuple[str, list[str]]:
        deadline = time.monotonic() + (self.timeout if timeout is None else timeout)
        seen: list[str] = []
        while time.monotonic() < deadline:
            try:
                line = self.lines.get(timeout=min(0.1, deadline - time.monotonic()))
            except queue.Empty:
                continue
            if line is None:
                raise UciError(f"engine exited while waiting for {description}; output: {seen!r}")
            seen.append(line)
            if predicate(line):
                return line, seen
        raise UciError(f"timed out waiting for {description}; output: {seen!r}")

    def initialize(self) -> None:
        self.send("uci")
        self.expect(lambda line: line == "uciok", "uciok")
        self.send("isready")
        self.expect(lambda line: line == "readyok", "readyok")

    def quit(self) -> None:
        if self.process.poll() is None:
            try:
                self.send("quit")
                self.process.wait(timeout=self.timeout)
            except (UciError, subprocess.TimeoutExpired) as error:
                self.process.kill()
                self.process.wait()
                raise UciError(f"engine did not quit cleanly: {error}") from error
        if self.process.returncode != 0:
            raise UciError(f"engine exited abnormally with status {self.process.returncode}")

    def terminate(self) -> None:
        if self.process.poll() is None:
            self.process.kill()
            self.process.wait()
