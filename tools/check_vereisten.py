#!/usr/bin/env python3
"""Controleert of elke vereiste uit de opdracht in de code of de README staat.

De lector zoekt met Ctrl+F op "Vereiste <n>:". Dit script zoekt hetzelfde en meldt wat ontbreekt.
Gebruik:  python3 tools/check_vereisten.py
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent

# Rood in de opdracht: hoeven niet in de code aangeduid te worden.
NOT_MARKED = {10, 11, 12, 14, 15, 16, 17, 18, 19}
# Groen in de opdracht: moeten in de README staan.
IN_README = {1, 20, 54, 55}
# Nog niet gedaan (gepland): worden gemeld maar laten de check niet falen.
PLANNED = {54, 55}

SOURCES = [p for p in ROOT.rglob("*") if p.suffix in {".h", ".cpp", ".qml", ".txt", ".md"}
           and "third_party" not in p.parts and "build" not in p.parts[0:2] and p.is_file()]

found = {}
for path in SOURCES:
    for match in re.finditer(r"Vereiste (\d+): (\d+)\.", path.read_text(encoding="utf-8", errors="ignore")):
        number, repeated = int(match.group(1)), int(match.group(2))
        if number != repeated:
            print(f"FOUT  {path.relative_to(ROOT)}: 'Vereiste {number}: {repeated}.' nummers verschillen")
        found.setdefault(number, set()).add(path.relative_to(ROOT).as_posix())

missing = []
for number in range(1, 61):
    if number in NOT_MARKED:
        continue
    files = found.get(number, set())
    if number in IN_README and "README.md" not in files and number not in PLANNED:
        missing.append(f"{number} (moet in README.md)")
    elif not files and number not in PLANNED:
        missing.append(str(number))

for number in sorted(found):
    print(f"Vereiste {number:2d}: {', '.join(sorted(found[number]))}")
print()
print("Gepland, nog niet gedaan:", ", ".join(str(n) for n in sorted(PLANNED - found.keys())) or "-")
if missing:
    print("ONTBREEKT:", ", ".join(missing))
    sys.exit(1)
print("Alle vereisten zijn aangeduid.")
