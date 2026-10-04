#!/usr/bin/env python3
"""Deva's Awesome Adventures - the release packages (1.2).

  python3 tools/release/mkdist.py [--out release]      (make dist builds the cores first)

Writes, for the version in src/common.h and the date of its entry in CHANGELOG.md:
  deva-adventures-<v>-src.tar.gz             the source tree (no build outputs, no VCS)
  deva-adventures-<v>-aarch64.zip            RetroArch drop-in for the handhelds (RK3326 and other arm64)
  deva-adventures-<v>-x86_64.zip             RetroArch drop-in for a Linux PC
  deva-adventures-<v>-linux-x86_64.tar.gz    the game on its own for a Linux PC (no RetroArch, SDL2)
  deva-adventures-<v>-windows-x64.zip        the game on its own for Windows 10 and 11 (SDL2.dll inside)
  deva-adventures-<v>-windows-x64-setup.exe  the same as an installer (needs makensis, else skipped)
  SHA256SUMS                                 the archives above
The macOS disk image is made on a Mac (tools/release/mkapp.sh) and joins them in the GitHub release.
Every archive is reproducible: sorted entries, the release date as their time, no owners.
Each package carries its own SHA256SUMS of the files inside, which its install.sh checks.
"""
import argparse
import calendar
import gzip
import hashlib
import io
import os
import re
import shutil
import stat
import subprocess
import sys
import tarfile
import time
import zipfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
NAME = "deva-adventures"
CORE = "deva_adventures_libretro.so"
INFO = "deva_adventures_libretro.info"
SRC_EXCLUDE_TOP = {"build", "release", ".git", "dist"}
SRC_EXCLUDE_ANY = {"__pycache__", ".pytest_cache"}
ARCHES = {  # arch -> (built core, glibc it needs)
    "aarch64": ("build/aarch64/" + CORE, "2.17"),
    "x86_64": ("build/x86_64/" + CORE, "2.17"),
}
DOCS = ["docs/manuale.pdf", "docs/playtest/scheda_osservazione.pdf"]
PC_BIN = "build/x86_64/deva-adventures"  # make linux-x86_64 (Zig, glibc >= 2.17)
WIN_EXE = "build/win64/deva-adventures.exe"  # make windows (Zig)
SDL2_VERSION = "2.32.10"  # packaging/sdl2/README.md
SDL2_WIN_ZIP = "build/sdl2/SDL2-%s-win32-x64.zip" % SDL2_VERSION  # make sdl2-windows
SDL2_WIN_SHA256 = "6cf9706eefd0a4a06dc764007934d428afaf029fabdd408a9e646048c91e18fb"


def version():
    m = re.search(r'#define GAME_VERSION "([^"]+)"', open(os.path.join(ROOT, "src/common.h")).read())
    if not m:
        sys.exit("no GAME_VERSION in src/common.h")
    return m.group(1)


def release_epoch(v):
    """The date of the version's entry in CHANGELOG.md, midnight UTC (SOURCE_DATE_EPOCH wins)."""
    if os.environ.get("SOURCE_DATE_EPOCH"):
        return int(os.environ["SOURCE_DATE_EPOCH"])
    m = re.search(r"^## \[%s\] - (\d{4}-\d{2}-\d{2})" % re.escape(v),
                  open(os.path.join(ROOT, "CHANGELOG.md")).read(), re.M)
    if not m:
        sys.exit("CHANGELOG.md has no '## [%s] - YYYY-MM-DD' entry" % v)
    return calendar.timegm(time.strptime(m.group(1), "%Y-%m-%d"))


def source_files():
    out = []
    for dirpath, dirnames, filenames in os.walk(ROOT):
        rel = os.path.relpath(dirpath, ROOT)
        top = rel.split(os.sep)[0]
        if rel != "." and top in SRC_EXCLUDE_TOP:
            dirnames[:] = []
            continue
        dirnames[:] = sorted(d for d in dirnames if d not in SRC_EXCLUDE_ANY and
                             not (rel == "." and d in SRC_EXCLUDE_TOP))
        for f in sorted(filenames):
            if f.endswith(".pyc"):
                continue
            out.append(os.path.normpath(os.path.join(rel, f)))
    return sorted(out)


def mode_of(path):
    return 0o755 if os.stat(path).st_mode & stat.S_IXUSR else 0o644


def write_tar_gz(path, prefix, files, epoch):
    """files: [(name in the archive, data bytes, mode)]"""
    raw = io.BytesIO()
    with tarfile.open(fileobj=raw, mode="w", format=tarfile.PAX_FORMAT) as tar:
        dirs = set()
        for name, _, _ in files:
            parts = name.split("/")[:-1]
            for i in range(1, len(parts) + 1):
                dirs.add("/".join(parts[:i]))
        entries = [(d, None, 0o755) for d in sorted(dirs)] + list(files)
        entries.sort(key=lambda e: e[0])
        root = tarfile.TarInfo(prefix)
        root.type, root.mode, root.mtime = tarfile.DIRTYPE, 0o755, epoch
        tar.addfile(root)
        for name, data, mode in entries:
            ti = tarfile.TarInfo(prefix + "/" + name)
            ti.mtime, ti.mode, ti.uid, ti.gid, ti.uname, ti.gname = epoch, mode, 0, 0, "", ""
            if data is None:
                ti.type = tarfile.DIRTYPE
                tar.addfile(ti)
            else:
                ti.size = len(data)
                tar.addfile(ti, io.BytesIO(data))
    with open(path, "wb") as f:
        with gzip.GzipFile(filename="", mode="wb", fileobj=f, mtime=epoch, compresslevel=9) as gz:
            gz.write(raw.getvalue())


def write_zip(path, prefix, files, epoch):
    stamp = time.gmtime(max(epoch, 315532800))[:6]  # (zip dates start in 1980)
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name, data, mode in sorted(files, key=lambda e: e[0]):
            zi = zipfile.ZipInfo(prefix + "/" + name, date_time=stamp)
            zi.compress_type = zipfile.ZIP_DEFLATED
            zi.create_system = 3  # unix: the permissions below count
            zi.external_attr = ((stat.S_IFREG | mode) & 0xFFFF) << 16
            z.writestr(zi, data)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def read(rel):
    with open(os.path.join(ROOT, rel), "rb") as f:
        return f.read()


def need(*rels):
    for rel in rels:
        if not os.path.exists(os.path.join(ROOT, rel)):
            sys.exit("missing %s: run make dist (it builds the cores and the PC program)" % rel)


def common():
    """What every package carries besides the game: licences, history, docs, the report tool."""
    files = [
        ("LICENSE", read("LICENSE"), 0o644),
        ("THIRD_PARTY.md", read("THIRD_PARTY.md"), 0o644),
        ("CHANGELOG.md", read("CHANGELOG.md"), 0o644),
        ("tools/report/report.py", read("tools/report/report.py"), 0o755),
    ]
    return files + [("docs/" + os.path.basename(d), read(d), 0o644) for d in DOCS]


def game_data(prefix):
    files = []
    data_root = os.path.join(ROOT, "data", "deva_adventures")
    for dirpath, dirnames, filenames in os.walk(data_root):
        dirnames.sort()
        for f in sorted(filenames):
            full = os.path.join(dirpath, f)
            rel = os.path.relpath(full, data_root).replace(os.sep, "/")
            files.append((prefix + "deva_adventures/" + rel, read(os.path.relpath(full, ROOT)), 0o644))
    return files


def with_sums(files):
    sums = "".join("%s  %s\n" % (sha256(d), n) for n, d, _ in sorted(files, key=lambda e: e[0]))
    return files + [("SHA256SUMS", sums.encode(), 0o644)]


def drop_in(v, arch, epoch):
    core_rel, glibc = ARCHES[arch]
    need(core_rel, *DOCS)

    def sub(text):
        return text.replace("@VERSION@", v).replace("@ARCH@", arch).replace("@GLIBC@", glibc)
    files = [
        ("cores/" + CORE, read(core_rel), 0o755),
        ("info/" + INFO, read(INFO), 0o644),
        ("install.sh", sub(read("packaging/retroarch/install.sh").decode()).encode(), 0o755),
        ("uninstall.sh", read("packaging/retroarch/uninstall.sh"), 0o755),
        ("LEGGIMI.txt", sub(read("packaging/retroarch/LEGGIMI.txt").decode()).encode(), 0o644),
    ]
    return with_sums(files + common() + game_data("system/"))


def pc_package(v, epoch):
    """The game on its own: the program with its data beside it (a portable folder), install.sh."""
    need(PC_BIN, *DOCS)

    def sub(text):
        return text.replace("@VERSION@", v)
    files = [
        ("deva-adventures", read(PC_BIN), 0o755),
        ("deva-adventures.desktop", read("packaging/linux/deva-adventures.desktop"), 0o644),
        ("deva-adventures.png", read("packaging/linux/deva-adventures.png"), 0o644),
        ("install.sh", sub(read("packaging/linux/install.sh").decode()).encode(), 0o755),
        ("LEGGIMI.txt", sub(read("packaging/linux/LEGGIMI.txt").decode()).encode(), 0o644),
    ]
    return with_sums(files + common() + game_data(""))


def crlf(data):
    """Text for Windows' Notepad of before 2018."""
    return data.replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")


def windows_package(v, epoch):
    """The game on its own for Windows: the program, SDL2.dll and the data beside it (a portable folder)."""
    need(WIN_EXE, SDL2_WIN_ZIP, *DOCS)
    sdl = read(SDL2_WIN_ZIP)
    if sha256(sdl) != SDL2_WIN_SHA256:
        sys.exit("%s is not the SDL2 build pinned in packaging/sdl2/README.md" % SDL2_WIN_ZIP)

    def sub(text):
        return text.replace("@VERSION@", v)
    files = [
        ("deva-adventures.exe", read(WIN_EXE), 0o755),
        ("SDL2.dll", zipfile.ZipFile(io.BytesIO(sdl)).read("SDL2.dll"), 0o644),
        ("LICENSE-SDL2.txt", crlf(read("packaging/sdl2/LICENSE.txt")), 0o644),
        ("LICENSE-mingw-w64.txt", crlf(read("packaging/windows/LICENSE-mingw-w64.txt")), 0o644),
        ("deva-adventures.png", read("packaging/linux/deva-adventures.png"), 0o644),
        ("deva-adventures.ico", read("packaging/windows/deva-adventures.ico"), 0o644),
        ("LEGGIMI.txt", crlf(sub(read("packaging/windows/LEGGIMI.txt").decode()).encode()), 0o644),
    ]
    return with_sums(files + common() + game_data(""))


def windows_installer(v, epoch, files, out_dir):
    """setup.exe from the same files (NSIS), or None without makensis."""
    makensis = shutil.which("makensis")
    if not makensis:
        print("(no makensis: the Windows installer is skipped; apt install nsis)")
        return None
    stage = os.path.join(ROOT, "build", "win64", "stage")
    shutil.rmtree(stage, ignore_errors=True)
    for name, data, _ in files:
        path = os.path.join(stage, *name.split("/"))
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "wb") as f:
            f.write(data)
    name = "%s-%s-windows-x64-setup.exe" % (NAME, v)
    out = os.path.abspath(os.path.join(out_dir, name))  # (makensis takes paths from the script's folder)
    env = dict(os.environ, SOURCE_DATE_EPOCH=str(epoch))
    subprocess.run([makensis, "-V2", "-DVERSION=" + v, "-DSRC=" + stage, "-DOUT=" + out,
                    os.path.join(ROOT, "packaging", "windows", "installer.nsi")], check=True, env=env)
    return name


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--out", default=os.path.join(ROOT, "release"))
    a = ap.parse_args()
    v = version()
    epoch = release_epoch(v)
    os.makedirs(a.out, exist_ok=True)
    made = []

    src = [(rel.replace(os.sep, "/"), read(rel), mode_of(os.path.join(ROOT, rel))) for rel in source_files()]
    name = "%s-%s-src.tar.gz" % (NAME, v)
    write_tar_gz(os.path.join(a.out, name), "%s-%s" % (NAME, v), src, epoch)
    made.append(name)
    print("%-40s %4d files" % (name, len(src)))

    for arch in ARCHES:
        files = drop_in(v, arch, epoch)
        name = "%s-%s-%s.zip" % (NAME, v, arch)
        write_zip(os.path.join(a.out, name), "%s-%s-%s" % (NAME, v, arch), files, epoch)
        made.append(name)
        print("%-40s %4d files" % (name, len(files)))

    files = pc_package(v, epoch)
    name = "%s-%s-linux-x86_64.tar.gz" % (NAME, v)
    write_tar_gz(os.path.join(a.out, name), "%s-%s-linux-x86_64" % (NAME, v), files, epoch)
    made.append(name)
    print("%-40s %4d files" % (name, len(files)))

    files = windows_package(v, epoch)
    name = "%s-%s-windows-x64.zip" % (NAME, v)
    write_zip(os.path.join(a.out, name), "%s-%s-windows-x64" % (NAME, v), files, epoch)
    made.append(name)
    print("%-40s %4d files" % (name, len(files)))
    name = windows_installer(v, epoch, files, a.out)
    if name:
        made.append(name)
        print("%-40s (installer)" % name)

    with open(os.path.join(a.out, "SHA256SUMS"), "w") as f:
        for name in made:
            f.write("%s  %s\n" % (sha256(open(os.path.join(a.out, name), "rb").read()), name))
    for name in made + ["SHA256SUMS"]:
        print("  %10d  %s" % (os.path.getsize(os.path.join(a.out, name)), name))
    return 0


if __name__ == "__main__":
    sys.exit(main())
