#!/usr/bin/env python3
"""Deva's Awesome Adventures - the release packages (1.0).

  python3 tools/release/mkdist.py [--out release]      (make dist builds the cores first)

Writes, for the version in src/common.h and the date of its entry in CHANGELOG.md:
  deva-adventures-<v>-src.tar.gz       the source tree (no build outputs, no VCS)
  deva-adventures-<v>-aarch64.zip      RetroArch drop-in for the handhelds (RK3326 and other arm64)
  deva-adventures-<v>-x86_64.zip       RetroArch drop-in for a Linux PC
  SHA256SUMS                           the three archives
Every archive is reproducible: sorted entries, the release date as their time, no owners.
Each zip carries its own SHA256SUMS of the files inside, which install.sh checks.
"""
import argparse
import calendar
import gzip
import hashlib
import io
import os
import re
import stat
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


def drop_in(v, arch, epoch):
    core_rel, glibc = ARCHES[arch]
    if not os.path.exists(os.path.join(ROOT, core_rel)):
        sys.exit("missing %s: run make dist (it builds the cores)" % core_rel)
    for d in DOCS:
        if not os.path.exists(os.path.join(ROOT, d)):
            sys.exit("missing %s" % d)
    sub = lambda s: s.replace("@VERSION@", v).replace("@ARCH@", arch).replace("@GLIBC@", glibc)
    files = [
        ("cores/" + CORE, read(core_rel), 0o755),
        ("info/" + INFO, read(INFO), 0o644),
        ("install.sh", sub(read("packaging/retroarch/install.sh").decode()).encode(), 0o755),
        ("uninstall.sh", read("packaging/retroarch/uninstall.sh"), 0o755),
        ("LEGGIMI.txt", sub(read("packaging/retroarch/LEGGIMI.txt").decode()).encode(), 0o644),
        ("LICENSE", read("LICENSE"), 0o644),
        ("THIRD_PARTY.md", read("THIRD_PARTY.md"), 0o644),
        ("CHANGELOG.md", read("CHANGELOG.md"), 0o644),
        ("tools/report/report.py", read("tools/report/report.py"), 0o755),
    ]
    files += [("docs/" + os.path.basename(d), read(d), 0o644) for d in DOCS]
    data_root = os.path.join(ROOT, "data", "deva_adventures")
    for dirpath, dirnames, filenames in os.walk(data_root):
        dirnames.sort()
        for f in sorted(filenames):
            full = os.path.join(dirpath, f)
            rel = os.path.relpath(full, data_root).replace(os.sep, "/")
            files.append(("system/deva_adventures/" + rel, read(os.path.relpath(full, ROOT)), 0o644))
    sums = "".join("%s  %s\n" % (sha256(d), n) for n, d, _ in sorted(files, key=lambda e: e[0]))
    files.append(("SHA256SUMS", sums.encode(), 0o644))
    return files


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

    with open(os.path.join(a.out, "SHA256SUMS"), "w") as f:
        for name in made:
            f.write("%s  %s\n" % (sha256(open(os.path.join(a.out, name), "rb").read()), name))
    for name in made + ["SHA256SUMS"]:
        print("  %10d  %s" % (os.path.getsize(os.path.join(a.out, name)), name))
    return 0


if __name__ == "__main__":
    sys.exit(main())
