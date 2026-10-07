#!/usr/bin/env python3
"""Arte do Tank 1990 para o menu do EmuELEC (ver EMUELEC.md).

Monta, só com os sprites (resources/png/texture.png) e a fonte do jogo:
  Tank1990-marquee.png  o logo "TANK 1990 / REMAKE" em tijolos, fundo transparente
  Tank1990-image.png    a arte principal, 16:9: campo de batalha, os 4 jogadores e o logo
  Tank1990-thumb.png    a capa, 3:4, para os temas que mostram caixa

Uso: python3 tools/emuelec/art/make_art.py (precisa do Pillow). Grava em tools/emuelec/art/.
Tudo é desenhado em pixels lógicos e ampliado sem suavizar, como o jogo faz na tela.
"""
import os
import random

from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
OUT = os.path.dirname(os.path.abspath(__file__))
TEXTURE = Image.open(os.path.join(ROOT, "resources/png/texture.png")).convert("RGBA")
FONT = os.path.join(ROOT, "resources/font/prstartk.ttf")

# Cores dos jogadores (Player::getPlayerColor) e o sprite de cada um (spriteconfig.cpp:
# P3 usa o sprite do P2 e P4 o do P1, tingidos com SDL_SetTextureColorMod)
PLAYERS = [
    ((640, 0), (255, 215, 0)),
    ((768, 0), (0, 255, 0)),
    ((768, 0), (0, 100, 255)),
    ((640, 0), (255, 50, 50)),
]
# Direções dentro do bloco de 128 px de cada tanque: cima, esquerda, baixo, direita
UP, LEFT, DOWN, RIGHT = 0, 1, 2, 3
GOLD = (255, 200, 40)


def sprite(x, y, w, h):
    return TEXTURE.crop((x, y, x + w, y + h))


def tinted(img, color):
    """Multiplica o RGB pela cor, como o SDL_SetTextureColorMod."""
    r, g, b, a = img.split()
    r = r.point(lambda v: v * color[0] // 255)
    g = g.point(lambda v: v * color[1] // 255)
    b = b.point(lambda v: v * color[2] // 255)
    return Image.merge("RGBA", (r, g, b, a))


def tank(block, direction, color=None):
    img = sprite(block[0] + direction * 32, block[1], 32, 32)
    return tinted(img, color) if color else img


def scaled(img, k):
    return img.resize((img.width * k, img.height * k), Image.NEAREST)


def text_outlined(draw, xy, text, font, fill, outline=(0, 0, 0), width=2):
    x, y = xy
    for dx in range(-width, width + 1):
        for dy in range(-width, width + 1):
            if dx or dy:
                draw.text((x + dx, y + dy), text, font=font, fill=outline)
    draw.text((x, y), text, font=font, fill=fill)


# ---- Logo em tijolos ------------------------------------------------------------------

GLYPHS = {
    "T": ["#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."],
    "A": [".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"],
    "N": ["#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#", "#...#"],
    "K": ["#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"],
    "1": ["..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."],
    "9": [".###.", "#...#", "#...#", ".####", "....#", "....#", ".###."],
    "0": [".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###."],
    " ": ["...", "...", "...", "...", "...", "...", "..."],
}


def brick_fill(w, h, brick=(214, 32, 32), mortar=(255, 255, 255), dark=(140, 16, 16)):
    """Padrão de tijolos do logo do jogo: fiadas de 4 px (3 de tijolo e 1 de junta),
    tijolos de 8 px, juntas desencontradas de uma fiada para a outra."""
    img = Image.new("RGB", (w, h), mortar)
    d = ImageDraw.Draw(img)
    for row, y in enumerate(range(0, h, 4)):
        offset = 0 if row % 2 == 0 else 4
        for x in range(-8 + offset, w, 8):
            d.rectangle((x, y, x + 6, y + 2), fill=brick)
            d.line((x, y + 2, x + 6, y + 2), fill=dark)
    return img


def brick_word(word, cell, spacing=2):
    """Palavra em letras de 5x7 células, engrossadas (a letra fina de 1 célula some no
    padrão de tijolos) e preenchidas com tijolos, com sombra preta."""
    bold = cell // 4  # metade da junta entre células: engrossa sem fechar o miolo do 9 e do A
    widths = [len(GLYPHS[c][0]) for c in word]
    cols = sum(widths) + spacing * (len(word) - 1)
    w, h = cols * cell + 2 * bold, 7 * cell + 2 * bold
    mask = Image.new("L", (w, h), 0)
    md = ImageDraw.Draw(mask)
    x0 = 0
    for c, gw in zip(word, widths):
        for r, line in enumerate(GLYPHS[c]):
            for col, ch in enumerate(line):
                if ch == "#":
                    md.rectangle((bold + (x0 + col) * cell, bold + r * cell,
                                  bold + (x0 + col + 1) * cell - 1, bold + (r + 1) * cell - 1), fill=255)
        x0 += gw + spacing
    mask = mask.filter(ImageFilter.MaxFilter(2 * bold + 1))
    out = Image.new("RGBA", (w + 4, h + 4), (0, 0, 0, 0))
    out.paste(Image.new("RGBA", (w, h), (0, 0, 0, 255)), (4, 4), mask)
    out.paste(brick_fill(w, h), (0, 0), mask)
    return out


def logo(cell=8):
    """Logo: TANK 1990 em tijolos e, embaixo, a faixa dourada REMAKE entre duas estrelas."""
    word = brick_word("TANK 1990", cell)
    font = ImageFont.truetype(FONT, 3 * cell)
    label = "REMAKE"
    tw = int(font.getlength(label))
    band_w, band_h = tw + 4 * cell, 4 * cell + 4
    w = max(word.width, band_w + 2 * band_h + 2 * cell) + 8
    h = word.height + band_h + cell + 6
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    img.alpha_composite(word, ((w - word.width) // 2, 0))
    d = ImageDraw.Draw(img)
    bx, by = (w - band_w) // 2, word.height + cell
    d.rectangle((bx + 4, by + 4, bx + band_w + 3, by + band_h + 3), fill=(0, 0, 0, 255))
    d.rectangle((bx, by, bx + band_w - 1, by + band_h - 1), fill=(24, 24, 24, 255), outline=GOLD, width=2)
    star = sprite(896, 160, 32, 32).resize((band_h, band_h), Image.NEAREST)
    img.alpha_composite(star, (bx - band_h - cell, by))
    img.alpha_composite(star, (bx + band_w + cell, by))
    text_outlined(d, (bx + (band_w - tw) // 2, by + (band_h - 3 * cell) // 2 + 1), label, font, GOLD, width=1)
    return img


# ---- Campo de batalha ------------------------------------------------------------------

BRICK = sprite(928, 0, 16, 16)
STONE = sprite(928, 144, 16, 16)
WATER = sprite(928, 160, 16, 16)
BUSH = sprite(928, 192, 16, 16)
ICE = sprite(928, 208, 16, 16)
EAGLE = sprite(944, 0, 32, 32)
BULLET = sprite(944, 128, 8, 8)
BLAST = sprite(1040 + 2 * 64, 0, 64, 64)
SPARK = sprite(1108 + 32, 0, 32, 32)
SHIELD = sprite(976, 0, 32, 32)


def battlefield(w, h, seed):
    """Um mapa no estilo das fases: colunas de tijolo, aço, água, mato e gelo."""
    rng = random.Random(seed)
    img = Image.new("RGBA", (w, h), (0, 0, 0, 255))
    cols, rows = w // 16 + 1, h // 16 + 1
    for c in range(cols):
        for r in range(rows):
            x, y = c * 16, r * 16
            band = c % 6
            if r in (rows // 2, rows // 2 + 1) and c % 9 < 3:
                img.alpha_composite(WATER, (x, y))
            elif band in (1, 2) and r % 7 not in (3, 4):
                img.alpha_composite(BRICK, (x, y))
            elif band == 4 and r % 9 == 2:
                img.alpha_composite(STONE, (x, y))
            elif rng.random() < 0.05:
                img.alpha_composite(BUSH, (x, y))
            elif rng.random() < 0.02:
                img.alpha_composite(ICE, (x, y))
    return img


def clear(img, box):
    """Abre um espaço preto no mapa (onde ficam tanques, águia e logo)."""
    ImageDraw.Draw(img).rectangle(box, fill=(0, 0, 0, 255))


def fort(img, cx, bottom):
    """A águia e o muro de tijolos em volta, embaixo no centro."""
    clear(img, (cx - 32, bottom - 48, cx + 31, bottom))
    for x in range(cx - 32, cx + 32, 16):
        img.alpha_composite(BRICK, (x, bottom - 48))
    for y in (bottom - 32, bottom - 16):
        img.alpha_composite(BRICK, (cx - 32, y))
        img.alpha_composite(BRICK, (cx + 16, y))
    img.alpha_composite(EAGLE, (cx - 16, bottom - 32))


def dim_top(img, height, strength=210):
    """Escurece de cima para baixo, para o logo ler bem por cima do mapa."""
    shade = Image.new("RGBA", img.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(shade)
    for y in range(height):
        a = int(strength * (1 - y / height))
        d.line((0, y, img.width, y), fill=(0, 0, 0, a))
    img.alpha_composite(shade)


# Inimigos: canto do bloco de cada tipo na textura (as colunas de 128 px são as cores)
ENEMY_WHITE_A = (128, 0)
ENEMY_WHITE_B = (128, 64)
ENEMY_RED_BONUS = (0, 0)
ENEMY_TAN_C = (384, 128)


def scene(w, h, seed, cell, logo_y, tagline_y, players_y, enemies):
    img = battlefield(w, h, seed)
    cx = w // 2
    fort(img, cx, h)
    # Os quatro jogadores lado a lado, subindo, cada um com o tiro à frente
    gap = 52
    xs = [cx - 2 * gap + 10 + i * gap for i in range(4)]
    for x in xs:
        clear(img, (x - 6, players_y - 44, x + 37, players_y + 37))
    for i, ((block, color), x) in enumerate(zip(PLAYERS, xs)):
        img.alpha_composite(tank(block, UP, color if i >= 2 else None), (x, players_y))
        img.alpha_composite(BULLET, (x + 12, players_y - 18 - (i % 2) * 16))
    img.alpha_composite(SHIELD, (xs[0], players_y))
    # Inimigos descendo: um explodindo, os outros atirando
    for (x, y, block, boom) in enemies:
        clear(img, (x - 4, y - 4, x + 35, y + 60))
        img.alpha_composite(tank(block, DOWN), (x, y))
        if boom:
            img.alpha_composite(BLAST, (x - 16, y - 16))
        else:
            img.alpha_composite(BULLET, (x + 12, y + 40))
    # Faixa escura atrás do logo e do texto, que some aos poucos no mapa
    mark = logo(cell)
    band_bottom = tagline_y + 34
    clear(img, (0, 0, w, band_bottom - 24))
    shade = Image.new("RGBA", img.size, (0, 0, 0, 0))
    sd = ImageDraw.Draw(shade)
    for y in range(band_bottom - 24, band_bottom + 16):
        a = int(255 * (1 - (y - (band_bottom - 24)) / 40))
        sd.line((0, y, w, y), fill=(0, 0, 0, max(a, 0)))
    img.alpha_composite(shade)
    img.alpha_composite(mark, ((w - mark.width) // 2, logo_y))
    d = ImageDraw.Draw(img)
    font = ImageFont.truetype(FONT, 8)
    lines = ["1-4 PLAYERS - CAMPAIGN - DUEL - SURVIVAL"]
    if font.getlength(lines[0]) > w - 16:
        lines = ["1-4 PLAYERS", "CAMPAIGN - DUEL - SURVIVAL"]
    for i, part in enumerate(lines):
        tw = font.getlength(part)
        text_outlined(d, ((w - tw) // 2, tagline_y + i * 12), part, font, (255, 255, 255), width=1)
    return img


def main():
    mark = logo(8)
    padded = Image.new("RGBA", (mark.width + 16, mark.height + 16), (0, 0, 0, 0))
    padded.alpha_composite(mark, (8, 8))
    scaled(padded, 3).save(os.path.join(OUT, "Tank1990-marquee.png"))

    wide = scene(640, 360, 1990, cell=8, logo_y=12, tagline_y=134, players_y=262,
                 enemies=[(56, 186, ENEMY_WHITE_A, False), (540, 178, ENEMY_RED_BONUS, True),
                          (470, 214, ENEMY_TAN_C, False), (150, 206, ENEMY_WHITE_B, False)])
    scaled(wide, 2).convert("RGB").save(os.path.join(OUT, "Tank1990-image.png"))

    tall = scene(360, 480, 1985, cell=5, logo_y=16, tagline_y=118, players_y=370,
                 enemies=[(30, 200, ENEMY_WHITE_A, False), (290, 190, ENEMY_RED_BONUS, True),
                          (170, 250, ENEMY_TAN_C, False), (250, 290, ENEMY_WHITE_B, False)])
    scaled(tall, 2).convert("RGB").save(os.path.join(OUT, "Tank1990-thumb.png"))


if __name__ == "__main__":
    main()
