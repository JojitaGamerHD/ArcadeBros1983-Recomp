"""Extract the verified LLVM subset without relying on Windows tar's zstd limit."""
from __future__ import annotations

import argparse
from compression import zstd
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import stat
import sys
import tarfile


SHA256 = "1a3f2f998e230b5791cc4e2bfe11a1f7c07fe42c96ff44f227c9db0bedfaf091"
ARCHIVE_SIZE = 276626313
OPTIONS = {zstd.DecompressionParameter.window_log_max: 30}
CHUNK = 1024 * 1024


def archive_parts(name: str, base: tuple[str, ...] = (), *, link: bool = False) -> tuple[str, ...]:
    if not name or name.startswith("/") or "\\" in name or ":" in name or "\0" in name:
        raise ValueError(f"Unsafe archive path: {name!r}")
    parts = list(base)
    for part in name.split("/"):
        if part in ("", "."):
            continue
        if part == "..":
            if not link or not parts:
                raise ValueError(f"Unsafe archive path: {name!r}")
            parts.pop()
        else:
            parts.append(part)
    return tuple(parts)


def selected(relative: tuple[str, ...]) -> bool:
    name = "/".join(relative)
    return bool(
        re.fullmatch(r"bin/(?:clang(?:-\d+)?|lld|lld-link|llvm-readobj)\.exe", name, re.I)
        or re.fullmatch(r"bin/[^/]+\.dll", name, re.I)
        or re.fullmatch(r"lib/clang/[^/]+/include(?:/.*)?", name)
        or re.search(r"(?:^|/)(?:LICENSE|NOTICE|COPYING)[^/]*$", name, re.I)
    )


def find_prefix(archive: Path) -> tuple[str, ...]:
    # This short initial scan avoids assumptions about the top-level folder.
    # The complete extraction pass below consumes the stream to its checksum.
    with zstd.open(archive, "rb", options=OPTIONS) as stream:
        with tarfile.open(fileobj=stream, mode="r|") as tar:
            for member in tar:
                parts = archive_parts(member.name)
                if len(parts) >= 2 and parts[-2:] == ("bin", "clang.exe"):
                    return parts[:-2]
    raise ValueError("The verified archive does not contain bin/clang.exe")


def is_reparse(path: Path) -> bool:
    info = path.lstat()
    return stat.S_ISLNK(info.st_mode) or bool(
        getattr(info, "st_file_attributes", 0) & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400)
    )


def output_path(destination: Path, relative: tuple[str, ...]) -> Path:
    if not relative:
        raise ValueError("An output file has no name")
    for part in relative:
        stem = part.split(".", 1)[0].upper()
        if (
            part.endswith((".", " "))
            or re.search(r'[<>:"/\\|?*\x00-\x1f]', part)
            or stem in {"CON", "PRN", "AUX", "NUL", "CONIN$", "CONOUT$"}
            or re.fullmatch(r"(?:COM|LPT)[1-9]", stem)
        ):
            raise ValueError(f"Unsafe Windows output component: {part!r}")
    parent = destination
    for part in relative[:-1]:
        parent = parent / part
        if parent.exists():
            if is_reparse(parent) or not parent.is_dir():
                raise ValueError(f"Output parent is not a regular directory: {parent}")
        else:
            parent.mkdir()
    target = destination.joinpath(*relative)
    if not target.resolve(strict=False).is_relative_to(destination):
        raise ValueError(f"Output would escape the destination: {target}")
    return target


def extract(archive: Path, destination: Path) -> dict:
    if sys.maxsize <= 2**32:
        raise ValueError("A 64-bit Python build is required for this archive")
    if archive.stat().st_size != ARCHIVE_SIZE:
        raise ValueError("The LLVM archive has an unexpected size")
    with archive.open("rb") as source:
        if hashlib.file_digest(source, "sha256").hexdigest() != SHA256:
            raise ValueError("The LLVM archive SHA256 does not match the pinned official asset")

    if destination.exists():
        if is_reparse(destination) or not destination.is_dir() or any(destination.iterdir()):
            raise ValueError("The destination must be a new directory or an empty regular directory")
    else:
        destination.mkdir(parents=True)
    destination = destination.resolve(strict=True)
    prefix = find_prefix(archive)
    regular: dict[tuple[str, ...], Path] = {}
    links: dict[tuple[str, ...], tuple[str, ...]] = {}
    outputs: dict[tuple[str, ...], tuple[str, ...]] = {}
    names: set[str] = set()
    total_bytes = 0

    with zstd.open(archive, "rb", options=OPTIONS) as stream:
        with tarfile.open(fileobj=stream, mode="r|") as tar:
            for member in tar:
                parts = archive_parts(member.name)
                if parts[:len(prefix)] != prefix:
                    continue
                relative = parts[len(prefix):]
                if not selected(relative) or member.isdir():
                    continue
                if not (member.isreg() or member.islnk() or member.issym()):
                    raise ValueError(f"Unsupported selected archive entry: {member.name}")
                key = "/".join(relative).casefold()
                if key in names:
                    raise ValueError(f"Duplicate output filename: {member.name}")
                names.add(key)
                outputs[parts] = relative
                target = output_path(destination, relative)
                if member.isreg():
                    source = tar.extractfile(member)
                    if source is None:
                        raise ValueError(f"Could not read archive file: {member.name}")
                    with source, target.open("xb") as output:
                        shutil.copyfileobj(source, output, CHUNK)
                    if target.stat().st_size != member.size:
                        raise ValueError(f"Truncated archive file: {member.name}")
                    regular[parts] = target
                    total_bytes += member.size
                else:
                    base = parts[:-1] if member.issym() else ()
                    linked = archive_parts(member.linkname, base, link=True)
                    if linked[:len(prefix)] != prefix or not selected(linked[len(prefix):]):
                        raise ValueError(f"Archive link target is outside the selected subset: {member.name}")
                    links[parts] = linked
        # Tar stops at its end markers. Read the rest so zstd validates its frame
        # checksum and rejects truncation instead of silently accepting a prefix.
        while stream.read(CHUNK):
            pass

    def resolve_regular(name: tuple[str, ...], visiting: set[tuple[str, ...]]) -> Path:
        if name in regular:
            return regular[name]
        if name in visiting or name not in links:
            raise ValueError(f"Archive link has no selected regular target: {'/'.join(name)}")
        return resolve_regular(links[name], visiting | {name})

    for name in links:
        source = resolve_regular(name, set())
        target = output_path(destination, outputs[name])
        with source.open("rb") as input_file, target.open("xb") as output_file:
            shutil.copyfileobj(input_file, output_file, CHUNK)
        total_bytes += target.stat().st_size

    for tool in ("clang.exe", "lld-link.exe", "llvm-readobj.exe"):
        if not (destination / "bin" / tool).is_file():
            raise ValueError(f"Required tool is absent after extraction: {tool}")
    return {"prefix": "/".join(prefix), "count": len(outputs), "bytes": total_bytes}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("dest", type=Path)
    args = parser.parse_args()
    try:
        result = extract(args.archive.resolve(strict=True), args.dest.absolute())
    except (OSError, ValueError, tarfile.TarError, zstd.ZstdError) as error:
        parser.exit(1, f"LLVM extraction failed: {error}\n")
    print(json.dumps(result, ensure_ascii=True))


if __name__ == "__main__":
    main()
