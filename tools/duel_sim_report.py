#!/usr/bin/env python3
"""Soma a saída de várias execuções do duel_sim (linhas #map e #pickup).

    ./duel_sim --teams ABAB --map 0 > a.txt; ./duel_sim --teams ABAB --map 1 > b.txt
    python3 tools/duel_sim_report.py a.txt b.txt
"""
import sys
from collections import defaultdict

MAPS = ["Arena", "Fortress", "River", "Maze", "Open Field"]


def pct(part, total):
    return "   -" if total == 0 else f"{round(100 * part / total):3d}%"


maps = defaultdict(lambda: [0.0] * 13)
pickups = defaultdict(lambda: [0] * 6)      # (tipo, cor) -> atrás/parelho/na frente
sides = defaultdict(lambda: [0] * 2)        # (tipo, cor, lado) -> coletas, vitórias
for path in sys.argv[1:]:
    for line in open(path):
        f = line.split()
        if not f:
            continue
        if f[0] == "#map":
            for i, v in enumerate(f[2:]):
                maps[int(f[1])][i] += float(v)
        elif f[0] == "#pickup":
            values = [int(v) for v in f[4:]]
            for i, v in enumerate(values):
                pickups[(f[1], f[2])][i] += v
            sides[(f[1], f[2], int(f[3]))][0] += values[0] + values[2] + values[4]
            sides[(f[1], f[2], int(f[3]))][1] += values[1] + values[3] + values[5]

total = [0.0] * 13
print(f"{'mapa':<11} {'rodadas':>7} {'A':>5} {'B':>5} {'base':>5} {'tempo':>6} | {'bots preso':>10} {'destrava/min':>12} | {'cpu preso':>9}")
for m in sorted(maps):
    r = maps[m]
    total = [a + b for a, b in zip(total, r)]
    rounds, wa, wb, draws, base, time, ab, sb, ac, sc, ub, uc, stalled = r
    print(f"{MAPS[m]:<11} {int(rounds):>7} {pct(wa, rounds):>5} {pct(wb, rounds):>5} {pct(base, rounds):>5} {time / max(rounds, 1) / 1000:5.0f}s |"
          f" {100 * sb / max(ab, 1):9.1f}% {ub / max(ab / 60000, 1e-9):12.2f} | {100 * sc / max(ac, 1):8.1f}%"
          + (f"  ({int(stalled)} partidas travadas)" if stalled else ""))
rounds, wa, wb = total[0], total[1], total[2]
print(f"{'total':<11} {int(rounds):>7} {pct(wa, rounds):>5} {pct(wb, rounds):>5} {pct(total[4], rounds):>5} {total[5] / max(rounds, 1) / 1000:5.0f}s |"
      f" {100 * total[7] / max(total[6], 1):9.1f}% {total[10] / max(total[6] / 60000, 1e-9):12.2f} | {100 * total[9] / max(total[8], 1):8.1f}%")

print("\nVitória na rodada de quem pegou o bônus, pela situação em vidas na coleta")
print(f"{'bônus':<8} {'cor':<7} | {'atrás':>13} | {'parelho':>13} | {'na frente':>13} | {'total':>5}")
order = ["grenade", "helmet", "clock", "shovel", "tank", "star", "gun", "boat"]
for name in order:
    for color in ("equipe", "cinza"):
        p = pickups.get((name, color))
        if not p:
            continue
        cells = [f"{pct(p[2 * b + 1], p[2 * b])} de {p[2 * b]:5d}" for b in range(3)]
        print(f"{name:<8} {color:<7} | " + " | ".join(cells) + f" | {pct(p[1] + p[3] + p[5], p[0] + p[2] + p[4])}")

print("\nPor lado do mapa: vitória de quem pegou (coletas)")
print(f"{'bônus':<8} | {'cinza, meio':>15} | {'cor, lado próprio':>17} | {'cor, lado inimigo':>17}")
for name in order:
    cells = []
    for key in ((name, "cinza", -1), (name, "equipe", 0), (name, "equipe", 1)):
        n, w = sides.get(key, [0, 0])
        cells.append(f"{pct(w, n)} ({n:5d})")
    print(f"{name:<8} | {cells[0]:>15} | {cells[1]:>17} | {cells[2]:>17}")
