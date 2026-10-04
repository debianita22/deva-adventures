#!/usr/bin/env python3
"""Deva's Awesome Adventures - the notes of a GitHub release (1.2).

    python3 tools/release/notes.py 1.2.0 > notes.md

Which package is for what (Italian first, then English: the repository speaks English, the game
Italian), how to check a download, then the section of CHANGELOG.md for that version as it is.
The CI (.github/workflows/ci.yml) publishes them with the packages when a tag vX.Y.Z is pushed.
"""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# (the end of the file name, what it is for in Italian, in English)
PACKAGES = [
    ("windows-x64-setup.exe", "Windows 10 e 11 (64 bit), con l'installazione", "Windows 10 and 11, installer"),
    ("windows-x64.zip", "Windows 10 e 11 (64 bit), senza installare", "Windows 10 and 11, portable"),
    ("macos.dmg", "macOS 10.13 o successivo, Apple Silicon e Intel", "macOS 10.13 or later, Apple Silicon and Intel"),
    ("linux-x86_64.tar.gz", "PC Linux (x86_64), senza RetroArch", "Linux PC (x86_64), no RetroArch needed"),
    ("aarch64.zip", "RetroArch sulle console arm64 (RK3326 e simili)", "RetroArch on arm64 handhelds"),
    ("x86_64.zip", "RetroArch su un PC Linux", "RetroArch on a Linux PC"),
    ("src.tar.gz", "i sorgenti", "source code"),
]


def section(text, v):
    m = re.search(r"^## \[%s\] - (\S+)\n(.*?)(?=^## \[|\Z)" % re.escape(v), text, re.M | re.S)
    if not m:
        sys.exit("notes.py: no section [%s] in CHANGELOG.md" % v)
    return m.group(1), m.group(2).strip()


def unwrap(md):
    """GitHub shows every newline of release notes as a line break: one line per paragraph or item."""
    out, fence = [], False
    for line in md.split("\n"):
        s = line.strip()
        if s.startswith("```"):
            fence = not fence
            out.append(line)
            continue
        block = not s or fence or s.startswith(("#", "|", "- ", "* ", ">")) or re.match(r"\d+\. ", s)
        prev = out[-1].strip() if out else ""
        if not block and prev and not prev.startswith(("#", "|", "```")):
            out[-1] = out[-1].rstrip() + " " + s
        else:
            out.append(line)
    return "\n".join(out)


def main():
    if len(sys.argv) != 2 or not re.fullmatch(r"\d+\.\d+\.\d+", sys.argv[1]):
        sys.exit("usage: notes.py X.Y.Z")
    v = sys.argv[1]
    with open(os.path.join(ROOT, "CHANGELOG.md"), encoding="utf-8") as f:
        day, body = section(f.read(), v)
    out = [
        "Gioco didattico in italiano per bambini di 5 anni: diciotto giochi con la voce, quattro "
        "storie, tre salvataggi. *An Italian learning game for 5-year-olds.* (%s)" % day,
        "",
        "| Pacchetto | Per | *For* |",
        "| --- | --- | --- |",
    ]
    for end, it, en in PACKAGES:
        out.append("| `deva-adventures-%s-%s` | %s | *%s* |" % (v, end, it, en))
    out += [
        "",
        "Il manuale per i genitori è in ogni pacchetto (`docs/manuale.pdf`), con un `LEGGIMI.txt` "
        "per il sistema. I programmi non sono firmati con un certificato a pagamento: la prima volta "
        "Windows dice \"Windows ha protetto il PC\" (Ulteriori informazioni, Esegui comunque) e macOS "
        "chiede di consentire l'app (Impostazioni di Sistema, Privacy e sicurezza, Apri comunque). "
        "Prima, se vuoi, controlla il file scaricato: la sua impronta (`sha256sum <file>` su Linux, "
        "`shasum -a 256 <file>` su macOS, `(Get-FileHash <file>).Hash` in PowerShell) deve essere "
        "quella della sua riga in `SHA256SUMS`.",
        "",
        "## Novità della %s" % v,
        "",
        unwrap(body),
        "",
    ]
    sys.stdout.write("\n".join(out))


if __name__ == "__main__":
    main()
