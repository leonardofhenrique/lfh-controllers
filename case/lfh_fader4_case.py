"""Case impresso do LFH Fader 4: placa perfurada 70 x 90 mm com 4 faders de 75 mm
soldados em cima e o Pro Micro soldado embaixo (USB-C saindo pela borda de cima).

Feito com o template de placa perfurada da skill case-teclado-handwired.
Modelo em CadQuery que gera STEP (abre no Fusion 360) e STL (vai direto para a fatiadora).

    python lfh_fader4_case.py      -> gera os arquivos em export/ ao lado do script

Peças (imprimem sem suporte):
    plate         tampa com os rasgos das hastes e colunas que descem até a placa
    bottom        caixa com colunas que seguram a placa e abertura do USB-C
    test_stencil  tampa fina (sem colunas): apoie sobre os faders para conferir os rasgos

Fixação: 4 parafusos M2 x 8 entram por baixo da caixa, atravessam a coluna da caixa e o
furo do canto da placa e rosqueiam no inserto de calor da coluna da tampa. Tampa, placa
e caixa ficam presas juntas e não aparece parafuso em cima.

Coordenadas da placa: vista de cima (lado dos faders), USB para cima, origem no canto de
baixo à esquerda. Medidas com (*) foram estimadas por foto: conferir com régua.
"""
from pathlib import Path

import cadquery as cq

# --- Geral
PROJECT = "lfh_fader4"
CORNER_R = 3
E = 0.01

# --- Placa perfurada
BOARD = (70.0, 90.0)
BOARD_T = 1.6
BOARD_CLEAR = 0.6            # folga entre a placa e as paredes
BOARD_HOLE_INSET = 2.0       # centro dos furos dos cantos até as bordas (*)
UNDER_H = 12.0               # do piso até a placa: Pro Micro, solda, fios e fita (*)

# --- Faders B103 de 75 mm (curso de 60 mm), soldados na placa
FADER_X = [6.2, 24.9, 46.0, 65.0]  # centro de cada fader a partir da borda esquerda (*)
FADER_Y = 45.0               # centro do fader a partir da borda de baixo (*)
FADER_BODY = (9.0, 75.0)
FADER_H = 6.0                # corpo acima da placa
FADER_TRAVEL = 60
LEVER = (1.2, 4.0)           # haste: espessura x largura (largura no sentido do curso) (*)
LEVER_H = 15
SLOT_W = 2.4                 # rasgo: espessura da haste + folga (haste atravessada: ~4,6)
SLOT_LEN = FADER_TRAVEL + LEVER[1] + 0.6
PLATE_GAP = 0.5              # folga entre o topo dos faders e a tampa

# --- Pro Micro (embaixo da placa) e USB-C
USB_X = 32.0                 # centro do conector a partir da borda esquerda (*)
USB_DROP = 4.5               # da face de baixo da placa até o centro do conector (*)
USB_W = 13.0                 # abertura para a capa do cabo
USB_H = 8.0

# --- Caixa e tampa
WALL = 2
FLOOR_T = 2
PLATE_T = 2.5
LIP_H = 1.5                  # aba da tampa que encaixa por dentro das paredes
LIP_T = 1.2
LIP_CLEAR = 0.25

# --- Parafusos M2 x 8 + insertos de calor M2 (3,2 x 4 mm) nas colunas da tampa
SCREW_LEN = 8
STANDOFF_D = 6.5             # coluna da caixa (embaixo da placa)
BOSS_TOP_T = 2.5             # topo da coluna da caixa, onde a cabeça apoia
SCREW_CLEAR_D = 2.4
SCREW_HEAD_D = 4.4
SPACER_D = 5.6               # coluna da tampa (em cima da placa), cortada rente à parede
INSERT_HOLE_D = 3.0
INSERT_DEPTH = 4.2

# Alturas (z = 0 embaixo da caixa)
Z_BOARD = FLOOR_T + UNDER_H                       # face de baixo da placa
Z_PLATE = Z_BOARD + BOARD_T + FADER_H + PLATE_GAP  # face de baixo da tampa
BOT_H = Z_PLATE
SPACER_H = Z_PLATE - Z_BOARD - BOARD_T

OUTER = (BOARD[0] + 2 * (BOARD_CLEAR + WALL), BOARD[1] + 2 * (BOARD_CLEAR + WALL))
BX0 = BY0 = WALL + BOARD_CLEAR                    # canto da placa dentro da caixa


def board_pt(x, y):
    """Ponto da placa (coordenadas da placa) -> coordenadas da caixa."""
    return (BX0 + x, BY0 + y)


def board_holes():
    i = BOARD_HOLE_INSET
    return [board_pt(x, y) for x in (i, BOARD[0] - i) for y in (i, BOARD[1] - i)]


def fader_centers():
    return [board_pt(x, FADER_Y) for x in FADER_X]


def circles(pts, d, z0, t):
    return cq.Workplane("XY").workplane(offset=z0).pushPoints(pts).circle(d / 2).extrude(t)


def rounded_block(w, d, h, r, x0=0, y0=0, z0=0):
    return (cq.Workplane("XY").workplane(offset=z0)
            .center(x0 + w / 2, y0 + d / 2).rect(w, d).extrude(h)
            .edges("|Z").fillet(r))


def box(x0, y0, w, d, z0, h):
    return cq.Workplane("XY").box(w, d, h, centered=False).translate((x0, y0, z0))


def slots(z0, t):
    return (cq.Workplane("XY").workplane(offset=z0)
            .pushPoints(fader_centers()).slot2D(SLOT_LEN, SLOT_W, angle=90).extrude(t))


def plate_piece(spacers=True, lip=True, t=PLATE_T):
    """Tampa na posição montada: face de baixo em z = 0, face de cima em z = t."""
    w, d = OUTER
    body = rounded_block(w, d, t, CORNER_R)

    inner_o = WALL + LIP_CLEAR
    if lip:
        ir = max(CORNER_R - inner_o, 0.5)
        ring = rounded_block(w - 2 * inner_o, d - 2 * inner_o, LIP_H + E, ir, inner_o, inner_o, -LIP_H)
        ring = ring.cut(rounded_block(w - 2 * (inner_o + LIP_T), d - 2 * (inner_o + LIP_T),
                                      LIP_H + 3 * E, 0.5, inner_o + LIP_T, inner_o + LIP_T, -LIP_H - E))
        body = body.union(ring)

    if spacers:
        cols = circles(board_holes(), SPACER_D, -SPACER_H, SPACER_H + E)
        cols = cols.intersect(box(inner_o, inner_o, w - 2 * inner_o, d - 2 * inner_o, -SPACER_H - 1, SPACER_H + 2))
        body = body.union(cols)
        body = body.cut(circles(board_holes(), INSERT_HOLE_D, -SPACER_H - E, INSERT_DEPTH + E))

    return body.cut(slots(-SPACER_H - 1, SPACER_H + t + 2))


def bottom_piece():
    w, d = OUTER
    ir = max(CORNER_R - WALL, 0.5)
    holes = board_holes()

    body = rounded_block(w, d, BOT_H, CORNER_R)
    body = body.cut(rounded_block(w - 2 * WALL, d - 2 * WALL, BOT_H, ir, WALL, WALL, FLOOR_T))
    body = body.union(circles(holes, STANDOFF_D, 0, Z_BOARD))

    # Parafusos: rebaixo por baixo até perto do topo da coluna + furo passante no topo
    body = body.cut(circles(holes, SCREW_CLEAR_D, -E, Z_BOARD + 2 * E))
    body = body.cut(circles(holes, SCREW_HEAD_D, -E, Z_BOARD - BOSS_TOP_T + E))

    # USB-C: abertura na parede de cima, na altura do conector
    ux, _ = board_pt(USB_X, 0)
    uz = Z_BOARD - USB_DROP
    body = body.cut(box(ux - USB_W / 2, d - WALL - 1, USB_W, WALL + 2, uz - USB_H / 2, USB_H))
    return body


def dummy_parts():
    """Placa, faders e Pro Micro de mentira, só para conferir o conjunto (não imprimir)."""
    board = box(BX0, BY0, *BOARD, Z_BOARD, BOARD_T)
    board = board.cut(circles(board_holes(), 2.0, Z_BOARD - E, BOARD_T + 2 * E))
    faders = None
    for cx, cy in fader_centers():
        f = (cq.Workplane("XY").box(*FADER_BODY, FADER_H, centered=(True, True, False))
             .translate((cx, cy, Z_BOARD + BOARD_T)))
        f = f.union(cq.Workplane("XY").box(*LEVER, LEVER_H, centered=(True, True, False))
                    .translate((cx, cy, Z_BOARD + BOARD_T + FADER_H)))
        faders = f if faders is None else faders.union(f)
    ux, uy = board_pt(USB_X, BOARD[1])
    pm = box(ux - 9, uy - 33, 18, 33, Z_BOARD - 4.1, 1.6)
    pm = pm.union(box(ux - 4.5, uy - 6.5, 9, 8, Z_BOARD - USB_DROP - 1.6, 3.2))
    return board, faders, pm


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
        (plate, -LIP_H / 2, "aba e colunas"),
        (plate, -SPACER_H + 1, "pé das colunas (insertos)"),
        (bottom, FLOOR_T + 1, "caixa (colunas)"),
        (bottom, Z_BOARD - USB_DROP, "altura do USB-C"),
        (bottom, FLOOR_T / 2, "caixa (piso)"),
    ]


def main():
    out = Path(__file__).parent / "export"
    out.mkdir(exist_ok=True)
    for old in out.glob(f"{PROJECT}_*"):
        old.unlink()

    grip = SCREW_LEN - BOSS_TOP_T - BOARD_T
    assert 2 <= grip <= INSERT_DEPTH, f"Parafuso de {SCREW_LEN} mm rosqueia {grip:.1f} mm no inserto"
    print(f"Parafuso M2 x {SCREW_LEN}: {grip:.1f} mm de rosca no inserto")

    # Os rasgos não podem cortar as colunas da tampa
    for hx, hy in board_holes():
        for cx, cy in fader_centers():
            near_x = abs(hx - cx) < SPACER_D / 2 + SLOT_W / 2
            near_y = abs(hy - cy) < SLOT_LEN / 2 + SPACER_D / 2
            assert not (near_x and near_y), "Um rasgo passa por cima de uma coluna da tampa"

    w, d = OUTER
    print(f"Case: {w:.1f} x {d:.1f} x {BOT_H + PLATE_T:.1f} mm; haste {LEVER_H - PLATE_GAP - PLATE_T:.1f} mm acima da tampa")

    plate = plate_piece()
    bottom = bottom_piece()
    stencil = plate_piece(spacers=False, lip=False, t=1.2)

    parts = [
        (f"{PROJECT}_plate", for_print(plate)),
        (f"{PROJECT}_bottom", bottom),
        (f"{PROJECT}_test_stencil", stencil),
    ]
    for name, part in parts:
        cq.exporters.export(part, str(out / f"{name}.step"))
        cq.exporters.export(part, str(out / f"{name}.stl"), tolerance=0.01, angularTolerance=0.1)

    board, faders, pm = dummy_parts()
    asm = (cq.Assembly(name=PROJECT)
           .add(bottom, name="bottom", color=cq.Color(0.3, 0.3, 0.3))
           .add(plate, name="plate", loc=cq.Location((0, 0, Z_PLATE)), color=cq.Color(0.95, 0.95, 0.95))
           .add(board, name="placa (referencia)", color=cq.Color(0.1, 0.5, 0.2))
           .add(faders, name="faders (referencia)", color=cq.Color(0.15, 0.15, 0.15))
           .add(pm, name="pro micro (referencia)", color=cq.Color(0.5, 0.2, 0.6)))
    asm.export(str(out / f"{PROJECT}_assembly.step"))
    print(f"Arquivos gerados em {out}")


if __name__ == "__main__":
    main()
