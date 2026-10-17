"""Case impresso do LFH Fader 4 (4 faders de 75 mm + Arduino Pro Micro).

Feito a partir do template de faders da skill case-teclado-handwired.
Modelo em CadQuery que gera STEP (abre no Fusion 360) e STL (vai direto para a fatiadora).

    python lfh_fader4_case.py      -> gera os arquivos em export/ ao lado do script

Peças (imprimem sem suporte):
    plate   tampa com os rasgos das hastes; os faders são parafusados por baixo dela
    bottom  caixa com berço do Pro Micro, abertura USB e torres dos parafusos
    test_plate  faixa com um fader só, para conferir rasgo e furos antes da tampa inteira
"""
from pathlib import Path

import cadquery as cq

# --- Geral
PROJECT = "lfh_fader4"
CORNER_R = 4
E = 0.01

# --- Fader B103 de 75 mm (curso de 60 mm, canal duplo, 6 terminais)
# Medidas com (*) não estão no anúncio: conferir com a peça de teste.
FADER_COUNT = 4
FADER_PITCH = 30             # distância entre os faders (centro a centro)
FADER_BODY = (9.0, 75.0)     # corpo: largura x comprimento
FADER_BODY_H = 6.0
FADER_PINS_H = 6.0           # terminais abaixo do corpo (altura total 27 = 15 + 6 + 6)
FADER_TRAVEL = 60
LEVER = (1.2, 4.0)           # haste: espessura x largura (a largura vai no sentido do curso) (*)
LEVER_H = 15                 # haste acima do corpo
LEVER_DX = 0.0               # deslocamento da haste em relação ao centro do corpo (*)
SLOT_W = 2.4                 # rasgo: espessura da haste + folga
SLOT_LEN = FADER_TRAVEL + LEVER[1] + 0.6
FADER_HOLE_PITCH = 70.0      # distância entre os 2 furos M2 do corpo (*)
FADER_HOLE_D = 2.4           # passagem do parafuso M2
BODY_CLEAR = 0.3             # folga entre o corpo e as guias
RIB_T = 1.2                  # guias laterais sob a tampa (centralizam o corpo)
RIB_H = 3.0
RIB_END_GAP = 8              # guias param antes das pontas do corpo

# --- Tampa
PLATE_T = 2.5
MARGIN_X = 15                # da borda até o centro do primeiro/último fader
MARGIN_Y = 10                # da borda até a ponta do corpo do fader
LIP_H = 1.5                  # aba que encaixa por dentro das paredes da caixa
LIP_T = 1.2
LIP_CLEAR = 0.25

# --- Caixa
WALL = 2
FLOOR_T = 2
INNER_H = 20                 # corpo (6) + terminais (6) + solda e fios

# --- Parafusos M2 x 8 + insertos de calor M2 (3,2 x 4 mm) na tampa
# O parafuso entra por baixo da caixa, por dentro da torre, atravessa o topo da torre
# e rosqueia no inserto, que fica numa coluna embaixo da tampa.
SCREW_INSET = 6              # coluna da tampa precisa caber por dentro da parede
SCREW_LEN = 8
INSERT_HOLE_D = 3.0
INSERT_DEPTH = 4.2
INSERT_BOSS_D = 6.2
INSERT_BOSS_H = 5.0          # coluna embaixo da tampa (a torre da caixa termina embaixo dela)
SCREW_CLEAR_D = 2.4
SCREW_HEAD_D = 4.4
BOSS_TOP_T = 4.0             # topo da torre, onde a cabeça apoia
BOSS_D = 7

# --- Pro Micro (no fundo, entre o 2º e o 3º fader, USB na parede de trás)
PM_SIZE = (18.6, 33.6)       # placa + folga
PM_DX = 0                    # deslocamento lateral a partir do centro
CRADLE_T = 1.2
CRADLE_H = 2
USB_W = 12.5
USB_H = 8
USB_FLOOR_CUT = 1.5

BOT_H = FLOOR_T + INNER_H
TOWER_H = BOT_H - INSERT_BOSS_H


def size(count=FADER_COUNT):
    return (count - 1) * FADER_PITCH + 2 * MARGIN_X, FADER_BODY[1] + 2 * MARGIN_Y


def fader_centers(count=FADER_COUNT):
    w, d = size(count)
    return [(MARGIN_X + i * FADER_PITCH, d / 2) for i in range(count)]


def corners(w, d):
    i = SCREW_INSET
    return [(i, i), (w - i, i), (i, d - i), (w - i, d - i)]


def offset_pts(pts, dx, dy):
    return [(x + dx, y + dy) for x, y in pts]


def rects(pts, w, h, z0, t):
    return cq.Workplane("XY").workplane(offset=z0).pushPoints(pts).rect(w, h).extrude(t)


def circles(pts, d, z0, t):
    return cq.Workplane("XY").workplane(offset=z0).pushPoints(pts).circle(d / 2).extrude(t)


def rounded_block(w, d, h, r, x0=0, y0=0, z0=0):
    return (cq.Workplane("XY").workplane(offset=z0)
            .center(x0 + w / 2, y0 + d / 2).rect(w, d).extrude(h)
            .edges("|Z").fillet(r))


def fader_cutters(centers, z0, t):
    """Rasgo da haste + furos de fixação de cada fader, atravessando a tampa."""
    holes = [p for s in (-1, 1) for p in offset_pts(centers, 0, s * FADER_HOLE_PITCH / 2)]
    slots = (cq.Workplane("XY").workplane(offset=z0)
             .pushPoints(offset_pts(centers, LEVER_DX, 0))
             .slot2D(SLOT_LEN, SLOT_W, angle=90).extrude(t))
    return slots.union(circles(holes, FADER_HOLE_D, z0, t))


def fader_ribs(centers):
    """Guias laterais embaixo da tampa: o corpo do fader encaixa entre elas."""
    dx = FADER_BODY[0] / 2 + BODY_CLEAR + RIB_T / 2
    rib_len = FADER_BODY[1] - 2 * RIB_END_GAP
    return rects(offset_pts(centers, -dx, 0) + offset_pts(centers, dx, 0),
                 RIB_T, rib_len, -RIB_H, RIB_H + E)


def plate_piece(count=FADER_COUNT, screws=True, lip=True):
    """Tampa na posição montada: face de cima em z = PLATE_T, guias e colunas para baixo."""
    w, d = size(count)
    centers = fader_centers(count)
    body = rounded_block(w, d, PLATE_T, CORNER_R)
    body = body.union(fader_ribs(centers))

    if lip:
        o = WALL + LIP_CLEAR
        ir = max(CORNER_R - o, 0.5)
        ring = rounded_block(w - 2 * o, d - 2 * o, LIP_H + E, ir, o, o, -LIP_H)
        ring = ring.cut(rounded_block(w - 2 * (o + LIP_T), d - 2 * (o + LIP_T), LIP_H + 3 * E,
                                      max(ir - LIP_T, 0.5), o + LIP_T, o + LIP_T, -LIP_H - E))
        body = body.union(ring)

    if screws:
        body = body.union(circles(corners(w, d), INSERT_BOSS_D, -INSERT_BOSS_H, INSERT_BOSS_H + E))
        body = body.cut(circles(corners(w, d), INSERT_HOLE_D, -INSERT_BOSS_H - E, INSERT_DEPTH + E))

    return body.cut(fader_cutters(centers, -RIB_H - 1, PLATE_T + RIB_H + 2))


def bottom_piece(count=FADER_COUNT):
    w, d = size(count)
    ir = max(CORNER_R - WALL, 0.5)
    pm_x = w / 2 + PM_DX - PM_SIZE[0] / 2
    pm_y = d - WALL - PM_SIZE[1]

    body = rounded_block(w, d, BOT_H, CORNER_R)
    body = body.cut(rounded_block(w - 2 * WALL, d - 2 * WALL, BOT_H, ir, WALL, WALL, FLOOR_T))
    body = body.union(circles(corners(w, d), BOSS_D, 0, TOWER_H))

    # Berço do Pro Micro (aberto do lado do USB)
    cradle = (cq.Workplane("XY")
              .box(PM_SIZE[0] + 2 * CRADLE_T, PM_SIZE[1] + CRADLE_T, FLOOR_T + CRADLE_H, centered=False)
              .translate((pm_x - CRADLE_T, pm_y - CRADLE_T, 0)))
    cradle = cradle.cut(cq.Workplane("XY")
                        .box(PM_SIZE[0], PM_SIZE[1] + WALL + 1, CRADLE_H + E, centered=False)
                        .translate((pm_x, pm_y, FLOOR_T)))
    body = body.union(cradle)

    # Parafusos: rebaixo por baixo até perto do topo da torre + furo passante no topo
    body = body.cut(circles(corners(w, d), SCREW_CLEAR_D, -E, TOWER_H + 2 * E))
    body = body.cut(circles(corners(w, d), SCREW_HEAD_D, -E, TOWER_H - BOSS_TOP_T + E))

    # Abertura do USB
    body = body.cut(cq.Workplane("XY")
                    .box(USB_W, WALL + 2, USB_H, centered=False)
                    .translate((w / 2 + PM_DX - USB_W / 2, d - WALL - 1, FLOOR_T - USB_FLOOR_CUT)))
    return body


def fader_dummy(count=FADER_COUNT):
    """Faders de mentira, só para conferir o conjunto no Fusion (não imprimir)."""
    parts = None
    for cx, cy in fader_centers(count):
        f = (cq.Workplane("XY").box(*FADER_BODY, FADER_BODY_H, centered=(True, True, False))
             .translate((cx, cy, -FADER_BODY_H)))
        f = f.union(cq.Workplane("XY").box(*LEVER, LEVER_H, centered=(True, True, False))
                    .translate((cx + LEVER_DX, cy, 0)))
        f = f.union(cq.Workplane("XY").box(FADER_BODY[0] - 2, 4, FADER_PINS_H, centered=(True, True, False))
                    .translate((cx, cy - FADER_BODY[1] / 2 + 4, -FADER_BODY_H - FADER_PINS_H)))
        f = f.union(cq.Workplane("XY").box(FADER_BODY[0] - 2, 4, FADER_PINS_H, centered=(True, True, False))
                    .translate((cx, cy + FADER_BODY[1] / 2 - 4, -FADER_BODY_H - FADER_PINS_H)))
        parts = f if parts is None else parts.union(f)
    return parts


def for_print(part):
    """Vira a peça de cabeça para baixo e apoia na mesa (z = 0)."""
    flipped = part.rotate((0, 0, 0), (1, 0, 0), 180)
    bb = flipped.val().BoundingBox()
    return flipped.translate((-bb.xmin, -bb.ymin, -bb.zmin))


def slice_views():
    """Cortes para o check_case.py: (peça, altura, título)."""
    plate = plate_piece()
    bottom = bottom_piece()
    return [
        (plate, PLATE_T / 2, "tampa"),
        (plate, -RIB_H / 2, "guias e aba"),
        (plate, -INSERT_BOSS_H + 1, "colunas dos insertos"),
        (bottom, FLOOR_T + 1, "caixa (paredes)"),
        (bottom, FLOOR_T / 2, "caixa (piso)"),
        (bottom, TOWER_H - 1, "topo das torres"),
    ]


def main():
    out = Path(__file__).parent / "export"
    out.mkdir(exist_ok=True)

    grip = SCREW_LEN - BOSS_TOP_T
    assert 2 <= grip <= INSERT_DEPTH, f"Parafuso de {SCREW_LEN} mm rosqueia {grip:.1f} mm no inserto"
    print(f"Parafuso M2 x {SCREW_LEN}: {grip:.1f} mm de rosca no inserto")

    # Parede entre o fim do rasgo e o furo do fader
    wall = FADER_HOLE_PITCH / 2 - SLOT_LEN / 2 - FADER_HOLE_D / 2
    assert wall >= 0.8, f"Só {wall:.1f} mm entre o rasgo e o furo do fader"
    print(f"Parede entre rasgo e furo do fader: {wall:.1f} mm")

    clear = BOT_H - FADER_BODY_H - FADER_PINS_H - FLOOR_T
    print(f"Espaço livre abaixo dos terminais dos faders: {clear:.1f} mm")

    w, d = size()
    print(f"Case: {w:.0f} x {d:.0f} x {BOT_H + PLATE_T:.1f} mm")

    plate = plate_piece()
    bottom = bottom_piece()
    test_plate = plate_piece(count=1, screws=False, lip=False)

    parts = [
        (f"{PROJECT}_plate", for_print(plate)),
        (f"{PROJECT}_bottom", bottom),
        (f"{PROJECT}_test_plate", for_print(test_plate)),
    ]
    for name, part in parts:
        cq.exporters.export(part, str(out / f"{name}.step"))
        cq.exporters.export(part, str(out / f"{name}.stl"), tolerance=0.01, angularTolerance=0.1)

    up = cq.Location((0, 0, BOT_H))
    asm = (cq.Assembly(name=PROJECT)
           .add(bottom, name="bottom", color=cq.Color(0.3, 0.3, 0.3))
           .add(plate, name="plate", loc=up, color=cq.Color(0.95, 0.95, 0.95))
           .add(fader_dummy(), name="faders (referencia)", loc=up, color=cq.Color(0.8, 0.4, 0.1)))
    asm.export(str(out / f"{PROJECT}_assembly.step"))
    print(f"Arquivos gerados em {out}")


if __name__ == "__main__":
    main()
