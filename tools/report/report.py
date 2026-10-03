#!/usr/bin/env python3
"""Deva's Awesome Adventures - the playtest report (0.15).

Reads the files the game writes next to its saves (RetroArch's save dir):
  deva_adventures_log.csv     one row per answer (profile 1; _2_, _3_ for the others)
  deva_adventures_diario.csv  one row per event: start, scenes, pauses, reminders, end (0.15)
  deva_adventures.sav         the profile (only its name is read)
and writes one HTML page for the grown-ups: per game, per session, the hardest questions.

  python3 tools/report/report.py <save dir or .csv files> [-o deva_report.html] [--text]

Only the standard library. The files may come from a console switched off in the middle of a
row (a last line cut short, zeros): those rows are skipped and counted. Rows of 0.14 and before
have no times. Times are measured by the game; dates come from the console's clock, which on
handhelds without a clock battery can be wrong.
"""
import argparse
import html
import os
import re
import statistics
import sys

GAMES = {
    "conta": "Conta", "parole": "Parole", "sequenze": "Sequenze", "balla": "Balla con me",
    "nome": "Il mio nome", "memory": "Memory", "ritmo": "Ritmo", "dove": "Sopra e sotto",
    "emozioni": "Emozioni", "storie": "Storie", "ginnastica": "Ginnastica", "trucco": "Trucca i mostri",
    "forme": "Forme e colori", "lettere": "Le lettere", "ombre": "Le ombre", "sentiero": "Il sentiero",
    "negozio": "Il negozio", "misure": "Le misure",
}
FINALS = {  # the duels of the tales logged apart: the last one (which feeling? under the villain's
    # name) and "what is missing?" (the duel of Memory: a question, not a board)
    "strega": "La strega (storia)", "stregone": "Lo stregone (storia)", "orco": "L'orco (storia)",
    "re": "Il re (storia)", "memory_sfida": "Memory (sfida)",
}
TITLES = dict(GAMES, **FINALS)
SCENES = {  # scene -> (what it is, name for the grown-ups)
    "title": ("menu", "menu iniziale"), "prova": ("menu", "prova dei tasti"), "menu": ("menu", "scelta del gioco"),
    "mappa": ("storia", "mappa"), "racconto": ("storia", "racconto"), "sfida": ("storia", "sfida al mostro"),
    "premio": ("premi", "premio"), "camerino": ("premi", "camerino"), "album": ("premi", "album"),
    "fine": ("fine", "buonanotte"), "salvataggi": ("grandi", "salvataggi"), "tastiera": ("grandi", "tastiera"),
    "opzioni": ("grandi", "opzioni"),
}
for _g, _t in GAMES.items():
    SCENES[_g] = ("gioco", _t)
KINDS = [("gioco", "giochi"), ("storia", "storia"), ("premi", "premi e camerino"), ("menu", "menu"),
         ("fine", "buonanotte"), ("grandi", "pagine dei grandi")]
LEVEL_MAX = 5


# ------------------------------------------------------------------ reading
def read_table(path, widths):
    """Rows as lists of fields; the header and damaged rows are left out. -> (rows, damaged)"""
    try:
        data = open(path, "rb").read()
    except OSError:
        return [], 0
    if data.startswith(b"\xef\xbb\xbf"):
        data = data[3:]
    lines = data.split(b"\n")
    cut = lines[-1] != b""  # every row ends with a newline: a last line without one was cut short
    rows, damaged = [], 0
    for i, raw in enumerate(lines):
        if raw == b"" and i == len(lines) - 1:
            continue
        if b"\x00" in raw or (cut and i == len(lines) - 1):
            damaged += 1
            continue
        line = raw.decode("utf-8", "replace").rstrip("\r")
        if not line or line.startswith("data;"):
            continue
        f = line.split(";")
        if len(f) not in widths:
            damaged += 1
            continue
        rows.append(f)
    return rows, damaged


def to_int(s, default=None):
    try:
        return int(s)
    except (TypeError, ValueError):
        return default


def to_float(s):
    try:
        return float(s.replace(",", "."))  # (a console with an Italian locale writes 5,2)
    except (TypeError, ValueError):
        return None


def read_answers(path):
    rows, damaged = read_table(path, (9, 12))
    out = []
    for f in rows:
        r = {"data": f[0], "sessione": to_int(f[1]), "gioco": f[2], "livello": to_int(f[3]), "domanda": f[4],
             "giusta": f[5], "scelta": f[6], "ok": f[7] == "giusta", "tentativo": to_int(f[8], 1),
             "secondi": None, "riascolti": 0, "at": None}
        if len(f) == 12:
            r["secondi"], r["riascolti"], r["at"] = to_float(f[9]), to_int(f[10], 0), to_int(f[11])
        if r["sessione"] is None or f[7] not in ("giusta", "sbagliata"):
            damaged += 1
            continue
        if r["gioco"] == "memory" and not r["domanda"].startswith("memory:"):
            r["gioco"] = "memory_sfida"
        # the guided try of a sequence is logged as "...-insieme" (":insieme" on the path): same question
        r["domanda"] = re.sub(r"[-:]insieme$", "", r["domanda"])
        out.append(r)
    return out, damaged


def read_diary(path):
    rows, damaged = read_table(path, (5,))
    out = []
    for f in rows:
        s, at = to_int(f[1]), to_int(f[2])
        if s is None or at is None:
            damaged += 1
            continue
        out.append({"data": f[0], "sessione": s, "at": at, "evento": f[3], "dettaglio": f[4]})
    return out, damaged


def read_name(path):
    try:
        for line in open(path, encoding="utf-8", errors="replace"):
            m = re.match(r"\s*nome\s*=\s*([A-Za-z]+)", line)
            if m:
                return m.group(1)
    except OSError:
        pass
    return None


def find_profiles(paths):
    """{profile: {"log": path, "diario": path, "sav": path}} from a folder or a list of files."""
    files = []
    for p in paths:
        if os.path.isdir(p):
            files += [os.path.join(p, n) for n in sorted(os.listdir(p))]
        else:
            files.append(p)
    prof = {}
    for path in files:
        m = re.match(r"deva_adventures(?:_(\d))?(_log\.csv|_diario\.csv|\.sav)$", os.path.basename(path))
        if not m:
            continue
        kind = {"_log.csv": "log", "_diario.csv": "diario", ".sav": "sav"}[m.group(2)]
        prof.setdefault(int(m.group(1) or 1), {})[kind] = path
    return {k: v for k, v in sorted(prof.items()) if "log" in v or "diario" in v}


# ------------------------------------------------------------------ the numbers
def is_board(r):
    """A row of a whole Memory board (the duel's "sparito" is a question like the others)."""
    return r["gioco"] == "memory" and r["domanda"].startswith("memory:")


def questions_of(answers):
    """A question = its tries: a row with tentativo 1 starts one (memory: a board per row)."""
    qs, cur = [], None
    for r in answers:
        key = (r["sessione"], r["gioco"], r["domanda"], r["giusta"])
        if cur is None or r["tentativo"] <= 1 or key != cur["key"] or is_board(r):
            cur = {"key": key, "sessione": r["sessione"], "data": r["data"], "gioco": r["gioco"], "domanda": r["domanda"],
                   "livello": r["livello"], "first_ok": r["ok"] and r["tentativo"] <= 1, "tries": 0,
                   "guided": False, "secondi": r["secondi"], "riascolti": 0, "at": r["at"], "ok": False,
                   "board": is_board(r)}
            qs.append(cur)
        cur["tries"] = max(cur["tries"], r["tentativo"])
        cur["guided"] = cur["guided"] or (r["tentativo"] >= 3 and not cur["board"])
        cur["riascolti"] += r["riascolti"]
        cur["ok"] = r["ok"]
    return qs


def median(xs):
    xs = [x for x in xs if x is not None]
    return statistics.median(xs) if xs else None


def pct(a, b):
    return round(100 * a / b) if b else None


def game_stats(qs, answers):
    out = []
    for g, title in TITLES.items():
        mine = [q for q in qs if q["gioco"] == g]
        if not mine:
            continue
        rows = [r for r in answers if r["gioco"] == g]
        levels = [r["livello"] for r in rows if r["livello"]]
        n = len(mine)
        st = {"gioco": g, "titolo": title, "n": n, "first": pct(sum(q["first_ok"] for q in mine), n),
              "guided": pct(sum(q["guided"] for q in mine), n), "tempo": median(q["secondi"] for q in mine),
              "riascolti": sum(q["riascolti"] for q in mine) / n,
              "liv_da": levels[0] if levels else None, "liv_a": levels[-1] if levels else None,
              "liv_max": max(levels) if levels else None, "sessioni": len({q["sessione"] for q in mine})}
        st["note"] = game_notes(st)
        out.append(st)
    return out


def game_notes(st):
    """Hints, not verdicts: the game already moves its level (three right in a row: up; a guided
    answer: down), so what matters is where it settles."""
    notes = []
    if st["n"] < 5:
        return ["pochi dati"]
    if st["gioco"] != "memory" and ((st["first"] or 0) < 50 or (st["guided"] or 0) >= 30):  # (boards: no tries)
        notes.append("difficile anche al livello 1" if st["liv_a"] == 1 else "difficile")
    if (st["first"] or 0) >= 85 and st["liv_a"] == LEVEL_MAX:
        notes.append("padroneggiato")
    if st["tempo"] is not None and st["tempo"] >= 12:
        notes.append("risponde piano (pensa o si distrae)")
    if st["riascolti"] >= 1:
        notes.append("chiede spesso di riascoltare")
    return notes


def session_stats(qs, diary):
    sessions = sorted({q["sessione"] for q in qs} | {e["sessione"] for e in diary})
    out = []
    for s in sessions:
        ev = [e for e in diary if e["sessione"] == s]
        mine = [q for q in qs if q["sessione"] == s]
        st = {"sessione": s, "n": len(mine), "first": pct(sum(q["first_ok"] for q in mine), len(mine)),
              "guided": sum(q["guided"] for q in mine), "tempo": median(q["secondi"] for q in mine),
              "pause": sum(1 for e in ev if e["evento"] == "pausa" and e["dettaglio"] == "aperta"),
              "promemoria": sum(1 for e in ev if e["evento"] == "promemoria"), "kinds": {}, "scenes": {},
              "games": [], "data": ev[0]["data"] if ev else (mine[0]["data"] if mine else ""), "durata": None,
              "fine": ""}
        played = {}
        for q in mine:
            played[q["gioco"]] = played.get(q["gioco"], 0) + 1
        st["games"] = sorted(played.items(), key=lambda kv: -kv[1])
        scenes = [e for e in ev if e["evento"] == "scena"]
        for a, b in zip(scenes, scenes[1:] + [None]):
            end = b["at"] if b else ev[-1]["at"]
            if end < a["at"]:  # a new start of the core with the same session number
                continue
            kind, name = SCENES.get(a["dettaglio"], ("menu", a["dettaglio"]))
            st["kinds"][kind] = st["kinds"].get(kind, 0) + end - a["at"]
            st["scenes"][name] = st["scenes"].get(name, 0) + end - a["at"]
        if ev:
            st["durata"] = sum(st["kinds"].values()) or (ev[-1]["at"] - ev[0]["at"])
            last = ev[-1]
            last_scene = next((e["dettaglio"] for e in reversed(ev) if e["evento"] == "scena"), "")
            if last["evento"] == "chiusa":  # Esci, or the frontend closed the core
                st["fine"] = "buonanotte, poi chiuso" if last_scene == "fine" else \
                    "chiuso dal menu iniziale (Esci)" if last_scene == "title" else \
                    "chiuso durante «%s» (da RetroArch?)" % SCENES.get(last_scene, ("", last_scene))[1]
            elif last_scene == "fine":
                st["fine"] = "buonanotte (tempo finito)"
            else:
                st["fine"] = "interrotto in «%s»: console spenta?" % SCENES.get(last_scene, ("", last_scene))[1]
        elif mine and mine[-1]["at"] is not None and mine[0]["at"] is not None:
            st["durata"] = mine[-1]["at"] - mine[0]["at"]
        out.append(st)
    return out


def hardest(qs, top=8):
    by = {}
    for q in qs:
        if q["board"]:
            continue
        by.setdefault((q["gioco"], q["domanda"]), []).append(q)
    rows = []
    for (g, d), lst in by.items():
        wrong = [q for q in lst if not q["first_ok"]]
        if len(lst) >= 2 and len(wrong) >= 2:
            missed = {}
            for q in wrong:  # the right answers she missed most ("nome:DEVA": which letters)
                missed[q["key"][3]] = missed.get(q["key"][3], 0) + 1
            if len(set(q["key"][3] for q in lst)) > 1:
                most = sorted(missed.items(), key=lambda kv: -kv[1])[:3]
                d += " (giusta: %s)" % ", ".join("%s ×%d" % kv if kv[1] > 1 else kv[0] for kv in most)
            rows.append((len(wrong) / len(lst), len(wrong), len(lst), g, d, sum(q["guided"] for q in lst)))
    rows.sort(key=lambda r: (-r[0], -r[1]))
    return rows[:top]


# ------------------------------------------------------------------ the page
def fmt_s(x):
    return "–" if x is None else ("%.1f s" % x if x < 60 else "%d min" % round(x / 60))


def fmt_min(sec):
    if sec is None:
        return "–"
    if sec >= 600:
        return "%d min" % round(sec / 60)
    return "%d min %02d s" % (sec // 60, sec % 60) if sec >= 60 else "%d s" % sec


def fmt_pct(x):
    return "–" if x is None else "%d%%" % x


def chart(points, title, unit, ymax=None):
    """A small line chart (inline SVG): points = [(label, value or None)]."""
    pts = [(l, v) for l, v in points if v is not None]
    if len(pts) < 2:
        return ""
    w, h, l, r, t, b = 560, 170, 40, 12, 22, 28
    top = ymax or max(v for _, v in pts) * 1.15 or 1
    xs = [l + i * (w - l - r) / (len(pts) - 1) for i in range(len(pts))]
    ys = [t + (h - t - b) * (1 - v / top) for _, v in pts]
    path = " ".join("%s%.1f,%.1f" % ("M" if i == 0 else "L", x, y) for i, (x, y) in enumerate(zip(xs, ys)))
    grid = ""
    for k in range(5):
        v = top * k / 4
        y = t + (h - t - b) * (1 - k / 4)
        grid += '<line x1="%d" x2="%d" y1="%.1f" y2="%.1f" class="grid"/>' % (l, w - r, y, y)
        grid += '<text x="%d" y="%.1f" class="ax" text-anchor="end">%d%s</text>' % (l - 4, y + 4, round(v), unit)
    labels = ""
    step = max(1, len(pts) // 12)
    for i, (lab, _) in enumerate(pts):
        if i % step == 0 or i == len(pts) - 1:
            labels += '<text x="%.1f" y="%d" class="ax" text-anchor="middle">%s</text>' % (xs[i], h - 8, lab)
    dots = "".join('<circle cx="%.1f" cy="%.1f" r="3"><title>%s: %s%s</title></circle>' % (x, y, lab, round(v, 1), unit)
                   for (lab, v), x, y in zip(pts, xs, ys))
    return ('<figure><figcaption>%s</figcaption><svg viewBox="0 0 %d %d" role="img" aria-label="%s">%s'
            '<path d="%s" class="line"/>%s%s</svg></figure>' % (html.escape(title), w, h, html.escape(title), grid,
                                                               path, dots, labels))


CSS = """
:root{--bg:#fffaf3;--card:#ffffff;--ink:#2b2333;--soft:#6f6577;--line:#eadfce;--accent:#c2417a;--good:#2f8f5b;
--warn:#b5651d;--chip:#f6eadb}
@media (prefers-color-scheme:dark){:root{--bg:#1d1922;--card:#28232f;--ink:#f1eaf5;--soft:#b5a9bd;--line:#3d3546;
--accent:#ef7fb0;--good:#72cf9b;--warn:#f0a65e;--chip:#352e3d}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);
font:15px/1.5 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
main{max-width:980px;margin:0 auto;padding:24px 16px 48px}h1{font-size:26px;margin:0 0 4px}
h2{font-size:19px;margin:32px 0 10px}h3{font-size:16px;margin:18px 0 6px}.sub{color:var(--soft);margin:0 0 18px}
.cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:10px}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:12px 14px}
.card b{display:block;font-size:22px}.card span{color:var(--soft);font-size:13px}
.wrap{overflow-x:auto;background:var(--card);border:1px solid var(--line);border-radius:12px}
table{border-collapse:collapse;width:100%;font-size:14px}th,td{padding:7px 10px;text-align:left;
border-bottom:1px solid var(--line);vertical-align:top}th{color:var(--soft);font-weight:600;white-space:nowrap}
tr:last-child td{border-bottom:0}th.n{text-align:right}td.n{text-align:right;white-space:nowrap;font-variant-numeric:tabular-nums}
.chip{display:inline-block;background:var(--chip);border-radius:999px;padding:0 8px;margin:2px 4px 0 0;font-size:12px;
white-space:nowrap}
@media (max-width:600px){table{font-size:13px}th,td{padding:6px 8px}}
.chip.hard{color:var(--warn)}.chip.good{color:var(--good)}
figure{margin:12px 0;background:var(--card);border:1px solid var(--line);border-radius:12px;padding:10px}
figcaption{font-weight:600;font-size:14px;margin-bottom:4px}svg{width:100%;height:auto;display:block}
.grid{stroke:var(--line)}.ax{fill:var(--soft);font-size:11px}.line{fill:none;stroke:var(--accent);stroke-width:2.5}
circle{fill:var(--accent)}.note{color:var(--soft);font-size:13px}ul{margin:6px 0 0;padding-left:20px}
.sess{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:12px 14px;margin:0 0 10px}
.sh{display:flex;flex-wrap:wrap;gap:4px 12px;align-items:baseline;justify-content:space-between}
.sess p{margin:4px 0}.games{color:var(--soft);font-size:13px}.end{font-size:13px;color:var(--soft)}
.bar{display:flex;height:12px;border-radius:6px;overflow:hidden;margin:8px 0 4px;background:var(--line)}
.bar i{display:block;height:100%}.legend{display:flex;flex-wrap:wrap;gap:2px 12px;font-size:12px;color:var(--soft)}
.legend i{display:inline-block;width:10px;height:10px;border-radius:3px;margin-right:4px;vertical-align:-1px}
.k-gioco{background:#d9578f}.k-storia{background:#7a64c9}.k-premi{background:#e0a33a}.k-menu{background:#4fa3a5}
.k-fine{background:#5b6e9e}.k-grandi{background:#9a8f86}
@media print{body{background:#fff}main{max-width:none}.card,.wrap,figure{break-inside:avoid}}
"""


def profile_html(num, name, answers, diary, damaged):
    qs = questions_of(answers)
    games = game_stats(qs, answers)
    sessions = session_stats(qs, diary)
    who = name or "profilo %d" % num
    out = ['<section><h2>%s</h2>' % html.escape(who)]
    timed = [q["secondi"] for q in qs if q["secondi"] is not None]
    play = sum(s["durata"] or 0 for s in sessions)
    cards = [("sessioni", len(sessions)), ("tempo di gioco", fmt_min(play) if play else "–"),
             ("domande", len(qs)), ("giuste al primo colpo", fmt_pct(pct(sum(q["first_ok"] for q in qs), len(qs)))),
             ("con l'aiuto guidato", fmt_pct(pct(sum(q["guided"] for q in qs), len(qs)))),
             ("tempo per rispondere (mediana)", fmt_s(median(timed)))]
    out.append('<div class="cards">%s</div>' % "".join(
        '<div class="card"><b>%s</b><span>%s</span></div>' % (html.escape(str(v)), k) for k, v in cards))

    out.append('<h3>Per gioco</h3><div class="wrap"><table><tr><th>Gioco</th><th class="n">Domande</th>'
               '<th class="n">Al primo colpo</th><th class="n">Aiuto guidato</th><th class="n">Tempo (mediana)</th>'
               '<th class="n">Livello</th><th class="n">Riascolti</th></tr>')
    for g in sorted(games, key=lambda g: -g["n"]):
        lv = "–" if g["liv_da"] is None else ("%d" % g["liv_a"] if g["liv_da"] == g["liv_a"]
                                              else "%d → %d" % (g["liv_da"], g["liv_a"]))
        chips = "".join('<span class="chip %s">%s</span>' % (
            "good" if n == "padroneggiato" else "hard" if n.startswith("difficile") else "", html.escape(n))
            for n in g["note"])
        tempo = fmt_s(g["tempo"]) + (" a partita" if g["gioco"] == "memory" and g["tempo"] is not None else "")
        out.append('<tr><td>%s%s</td><td class="n">%d</td><td class="n">%s</td><td class="n">%s</td>'
                   '<td class="n">%s</td><td class="n">%s</td><td class="n">%.1f</td></tr>' % (
                       html.escape(g["titolo"]), '<br>' + chips if chips else "", g["n"], fmt_pct(g["first"]),
                       "–" if g["gioco"] == "memory" else fmt_pct(g["guided"]), tempo, lv, g["riascolti"]))
    out.append("</table></div>")

    out.append(chart([(str(s["sessione"]), s["first"]) for s in sessions if s["n"] >= 3],
                     "Giuste al primo colpo, sessione per sessione", "%", 100))
    out.append(chart([(str(s["sessione"]), s["tempo"]) for s in sessions if s["n"] >= 3],
                     "Secondi per rispondere (mediana), sessione per sessione", ""))

    out.append('<h3>Sessione per sessione</h3>')
    for s in reversed(sessions):  # the newest first
        total = sum(s["kinds"].values())
        head = ["Sessione %d" % s["sessione"]]
        if s["data"]:
            head.append(html.escape(s["data"][:16]))
        if s["durata"]:
            head.append(fmt_min(s["durata"]))
        facts = "%d domande, %s al primo colpo, %d con l'aiuto guidato" % (s["n"], fmt_pct(s["first"]), s["guided"]) \
            if s["n"] else "nessuna domanda"
        if s["pause"] or s["promemoria"]:
            facts += "; %d pause, %d promemoria" % (s["pause"], s["promemoria"])
        played = ", ".join("%s (%d)" % (TITLES.get(g, g), n) for g, n in s["games"])
        bar = ""
        if total:
            bar = '<div class="bar">%s</div><div class="legend">%s</div>' % (
                "".join('<i class="k-%s" style="width:%.1f%%" title="%s: %s"></i>' % (
                    k, 100 * s["kinds"][k] / total, label, fmt_min(s["kinds"][k])) for k, label in KINDS
                    if s["kinds"].get(k)),
                "".join('<span><i class="k-%s"></i>%s %s</span>' % (k, label, fmt_min(s["kinds"][k]))
                        for k, label in KINDS if s["kinds"].get(k)))
        out.append('<div class="sess"><div class="sh"><b>%s</b>%s</div><p>%s</p>%s%s</div>' % (
            " · ".join(head), '<span class="end">%s</span>' % html.escape(s["fine"]) if s["fine"] else "",
            html.escape(facts), '<p class="games">%s</p>' % html.escape(played) if played else "", bar))

    hard = hardest(qs)
    if hard:
        out.append('<h3>Le domande più difficili</h3><div class="wrap"><table><tr><th>Gioco</th>'
                   '<th>Domanda</th><th class="n">Sbagliate al primo colpo</th>'
                   '<th class="n">Con l\'aiuto guidato</th></tr>')
        for _, wrong, n, g, d, guided in hard:
            out.append('<tr><td>%s</td><td><code>%s</code></td><td class="n">%d su %d</td><td class="n">%d</td></tr>'
                       % (html.escape(GAMES.get(g, g)), html.escape(d), wrong, n, guided))
        out.append("</table></div>")

    notes = []
    if damaged:
        notes.append("%d righe rovinate (console spenta mentre scriveva?) sono state saltate." % damaged)
    if any(r["secondi"] is None and r["at"] is None for r in answers):
        notes.append("Le righe scritte dalla 0.14 o prima non hanno i tempi.")
    if not diary:
        notes.append("Nessun diario delle sessioni (arriva con la 0.15): durata e scene non disponibili.")
    if notes:
        out.append('<p class="note">%s</p>' % " ".join(notes))
    out.append("</section>")
    return "".join(out), {"nome": who, "sessioni": len(sessions), "domande": len(qs), "giochi": games,
                          "damaged": damaged}


def build(paths):
    profiles = find_profiles(paths)
    if not profiles:
        return None, []
    parts, summary = [], []
    for num, files in profiles.items():
        answers, d1 = read_answers(files["log"]) if "log" in files else ([], 0)
        diary, d2 = read_diary(files["diario"]) if "diario" in files else ([], 0)
        name = read_name(files["sav"]) if "sav" in files else None
        body, info = profile_html(num, name, answers, diary, d1 + d2)
        parts.append(body)
        summary.append(info)
    page = ('<!doctype html><html lang="it"><head><meta charset="utf-8">'
            '<meta name="viewport" content="width=device-width,initial-scale=1">'
            '<title>Deva, come gioca</title><style>%s</style></head><body><main>'
            '<h1>Deva\'s Awesome Adventures: come gioca</h1>'
            '<p class="sub">Dai file del gioco (domande e diario delle sessioni). I numeri sono indizi, non voti: '
            'il gioco cambia livello da solo (tre giuste di fila: sale; un aiuto guidato: scende), conta dove '
            'si ferma. Le date vengono dall\'orologio della console.</p>%s'
            '<p class="note">«Al primo colpo»: domande giuste al primo tentativo. «Aiuto guidato»: domande '
            'arrivate al terzo tentativo, quando il gioco mostra la strada. «Tempo»: secondi da quando poteva '
            'rispondere (per Memory, la partita intera). «Riascolti»: richieste di risentire la domanda (B), per '
            'domanda. «Promemoria»: inviti del gioco dopo un po\' di silenzio.</p></main></body></html>'
            % (CSS, "".join(parts)))
    return page, summary


def main():
    ap = argparse.ArgumentParser(description="Il resoconto del playtest dai file di Deva's Awesome Adventures.")
    ap.add_argument("paths", nargs="+", help="la cartella dei salvataggi, o i file .csv / .sav")
    ap.add_argument("-o", "--out", default="deva_report.html", help="la pagina da scrivere")
    ap.add_argument("--text", action="store_true", help="anche un riassunto per giochi sul terminale")
    a = ap.parse_args()
    page, summary = build(a.paths)
    if page is None:
        print("Nessun file deva_adventures_*log.csv o *_diario.csv in: %s" % " ".join(a.paths), file=sys.stderr)
        return 1
    with open(a.out, "w", encoding="utf-8") as f:
        f.write(page)
    for p in summary:
        print("%s: %d sessioni, %d domande%s" % (p["nome"], p["sessioni"], p["domande"],
                                                 ", %d righe rovinate saltate" % p["damaged"] if p["damaged"] else ""))
        if a.text:
            for g in sorted(p["giochi"], key=lambda g: -g["n"]):
                print("  %-16s %3d domande, al primo colpo %4s, guidate %4s, tempo %7s  %s" % (
                    g["titolo"], g["n"], fmt_pct(g["first"]), fmt_pct(g["guided"]), fmt_s(g["tempo"]),
                    ", ".join(g["note"])))
    print("Scritto: %s" % a.out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
