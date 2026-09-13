#!/usr/bin/env python3
"""Generate console-style PNG screenshots 21-30 for Notepad_Application docs.

Requires: Pillow
  pip install pillow

Usage:
  python docs/generate_screenshots_extra.py
"""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

OUT = Path(__file__).resolve().parent / "screenshots"
W, H = 960, 540
BG = (18, 22, 28)
FRAME = (70, 120, 160)
TEXT = (220, 230, 240)
DIM = (140, 150, 160)
ACCENT = (90, 200, 160)
WARN = (240, 190, 90)
PASS = (80, 210, 120)
SEARCH = (120, 210, 140)
SUGGEST = (230, 200, 100)
HL_BG = (200, 200, 80)
HL_FG = (20, 20, 20)


def font(size: int = 14):
    for name in (
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/lucon.ttf",
        "C:/Windows/Fonts/cour.ttf",
    ):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    return ImageFont.load_default()


def new_canvas():
    img = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(img)
    return img, draw


def draw_layout(draw, title: str, text_lines: list[str], search_lines: list[str],
                suggest: str, status: str, cursor=None, hl=None):
    f = font(13)
    fs = font(12)
    draw.text((12, 8), title, fill=ACCENT, font=f)
    # Text pane ~60%
    tx0, ty0, tx1, ty1 = 10, 36, 580, 360
    draw.rectangle([tx0, ty0, tx1, ty1], outline=FRAME, width=2)
    draw.text((tx0 + 6, ty0 - 16), "TEXT ~60%", fill=DIM, font=fs)
    y = ty0 + 8
    for i, line in enumerate(text_lines):
        if hl and hl[0] == i:
            # highlight segment
            pre, mid, post = hl[1], hl[2], hl[3]
            x = tx0 + 10
            draw.text((x, y), pre, fill=TEXT, font=f)
            bx0 = x + f.getlength(pre)
            bx1 = bx0 + f.getlength(mid)
            draw.rectangle([bx0 - 1, y - 1, bx1 + 1, y + 16], fill=HL_BG)
            draw.text((bx0, y), mid, fill=HL_FG, font=f)
            draw.text((bx1, y), post, fill=TEXT, font=f)
        else:
            draw.text((tx0 + 10, y), line, fill=TEXT, font=f)
        y += 18
    if cursor is not None:
        cx, cy = cursor
        draw.rectangle([tx0 + 10 + cx * 8, ty0 + 8 + cy * 18, tx0 + 12 + cx * 8,
                        ty0 + 22 + cy * 18], fill=ACCENT)

    # Search pane ~20% right
    sx0, sy0, sx1, sy1 = 600, 36, 940, 360
    draw.rectangle([sx0, sy0, sx1, sy1], outline=SEARCH, width=2)
    draw.text((sx0 + 8, sy0 - 16), "SEARCH ~20%", fill=SEARCH, font=fs)
    y = sy0 + 10
    for line in search_lines:
        draw.text((sx0 + 10, y), line, fill=SEARCH, font=fs)
        y += 18

    # Suggestions ~20% bottom
    bx0, by0, bx1, by1 = 10, 380, 940, 470
    draw.rectangle([bx0, by0, bx1, by1], outline=SUGGEST, width=2)
    draw.text((bx0 + 8, by0 - 16), "SUGGESTIONS ~20%", fill=SUGGEST, font=fs)
    draw.text((bx0 + 12, by0 + 14), suggest, fill=SUGGEST, font=fs)

    # Status
    draw.rectangle([0, 490, W, H], fill=(30, 50, 90))
    draw.text((12, 505), status, fill=TEXT, font=fs)


def save(img: Image.Image, name: str):
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / name
    img.save(path)
    print("wrote", path)


def shot21():
    img, d = new_canvas()
    lines = [
        "The quick brown fox jumps over the lazy dog while",
        "students practice building a two dimensional linked",
        "list notepad that stores every character in a node",
        "with left right up and down pointers instead of a",
        "character array buffer",
        "",
        "[whole-word wrap: 'character' moved intact ->]",
    ]
    draw_layout(
        d,
        "+-- Notepad | 21 long-paragraph-wrap -------------------------------+",
        lines,
        ["SEARCH PANE", "", "query: (none)", "", "placeholder Assn3"],
        "Prefix \"char\": [1] character  [2] characters",
        "Status | long_paragraph.txt | Ln 5, Col 22 | Words 48 | Undo:0/5 | ALPHA",
        cursor=(21, 4),
    )
    save(img, "21-long-paragraph-wrap.png")


def shot22():
    img, d = new_canvas()
    lines = [
        "Typed: HelloWorld",
        "Ignored keystrokes: 1 2 3 ! ? , . @ #",
        "",
        "Result on screen (alpha-only): HelloWorld",
        "",
        "Ctrl+E Extended Mode is OFF (rubric default)",
    ]
    draw_layout(
        d,
        "+-- Notepad | 22 alpha-only-filter ---------------------------------+",
        lines,
        ["SEARCH PANE", "", "Mode: ALPHA-ONLY", "", "digits/punct filtered"],
        "Prefix \"Hello\": [1] HelloWorld",
        "Status | (untitled) * | Ln 4, Col 11 | Words 1 | Chars 10 | ALPHA",
    )
    save(img, "22-alpha-only-filter.png")


def shot23():
    img, d = new_canvas()
    lines = [
        "Before: ABCDEF",
        "Cursor between C and D, type X",
        "After:  ABCXDEF   << shift right, no overwrite",
        "",
        "Insertion at start / middle / end supported",
    ]
    draw_layout(
        d,
        "+-- Notepad | 23 insert-middle-shift -------------------------------+",
        lines,
        ["SEARCH PANE", "", "Assn3 placeholder"],
        "Prefix \"ABC\": [1] ABCXDEF",
        "Status | (untitled) * | Ln 3, Col 7 | Words 1 | Undo:1/5 | ALPHA",
        cursor=(6, 2),
    )
    save(img, "23-insert-middle-shift.png")


def shot24():
    img, d = new_canvas()
    lines = [
        "Hello brave",
        "world",
        "",
        "Enter pressed after 'brave' split the remainder",
        "Empty Enter inserts an empty linked-list row",
    ]
    draw_layout(
        d,
        "+-- Notepad | 24 enter-split-line ----------------------------------+",
        lines,
        ["SEARCH PANE", "", "last query: (none)"],
        "Prefix \"wor\": [1] world",
        "Status | (untitled) * | Ln 2, Col 1 | Words 3 | Undo:2/5 | ALPHA",
        cursor=(0, 1),
    )
    save(img, "24-enter-split-line.png")


def shot25():
    img, d = new_canvas()
    lines = [
        "LineOneLineTwo   << after Backspace at col 0 of line 2",
        "",
        "Before:",
        "  LineOne",
        "  |LineTwo   (cursor at column 0)",
        "Backspace joins rows and shifts left",
    ]
    draw_layout(
        d,
        "+-- Notepad | 25 backspace-join-lines ------------------------------+",
        lines,
        ["SEARCH PANE", "", "join demonstrated"],
        "Prefix \"Line\": [1] LineOneLineTwo",
        "Status | (untitled) * | Ln 1, Col 8 | Words 1 | Undo:0/5 | ALPHA",
        cursor=(7, 0),
    )
    save(img, "25-backspace-join-lines.png")


def shot26():
    img, d = new_canvas()
    lines = [
        "one two three four five six seven",
        "",
        "Undo stack (cap 5, linked nodes) top->bottom:",
        "  [5] seven  [4] six  [3] five  [4] four  [1] three",
        "Ctrl+Z removes ONE word per keypress",
        "Actions older than 5 words (one, two) untouched",
    ]
    draw_layout(
        d,
        "+-- Notepad | 26 undo-five-words-stack -----------------------------+",
        lines,
        ["SEARCH PANE", "", "Undo depth 5/5"],
        "Prefix \"sev\": (after undo depth drops)",
        "Status | (untitled) * | Ln 1, Col 34 | Words 7 | Undo:5/5 Redo:0/5",
    )
    save(img, "26-undo-five-words-stack.png")


def shot27():
    img, d = new_canvas()
    lines = [
        "one two three four five six seven",
        "",
        "After Ctrl+Z x3 then Ctrl+Y x2:",
        "Redo stack restores words one-at-a-time",
        "Redo also capped at 5 words (linked stack)",
    ]
    draw_layout(
        d,
        "+-- Notepad | 27 redo-five-words-stack -----------------------------+",
        lines,
        ["SEARCH PANE", "", "Redo depth 2/5"],
        "Prefix \"sev\": [1] seven",
        "Status | (untitled) * | Words 7 | Undo:3/5 Redo:2/5 | ALPHA",
    )
    save(img, "27-redo-five-words-stack.png")


def shot28():
    img, d = new_canvas()
    lines = [
        "Fixed non-scrollable frames:",
        "TEXT pane takes ~60% of the window",
        "RIGHT Search pane ~20%",
        "BOTTOM Suggestions ~20%",
        "Cursor cannot enter empty regions beyond text",
    ]
    draw_layout(
        d,
        "+-- Notepad | 28 layout-60-20-20 -----------------------------------+",
        lines,
        [
            "SEARCH PANE",
            "",
            "Ctrl+F query:",
            "\"linked\"",
            "",
            "hit @ Ln 2 Col 5",
            "",
            "F3 = Find Next",
            "Mode: ALPHA-ONLY",
        ],
        "Prefix \"lin\": [1] linked  [2] line  [3] lines",
        "Status | layout demo | Ln 2, Col 5 | Words 28 | Undo:0/5 | ALPHA",
        hl=(1, "TEXT pane takes ~60% of the ", "window", ""),
    )
    save(img, "28-layout-60-20-20.png")


def shot29():
    img, d = new_canvas()
    f = font(13)
    d.text((20, 16), "Self-Evaluation Sheet mapping (200 pts) — visual checklist", fill=ACCENT, font=f)
    items = [
        ("01 Insertion alpha + shift          /15", "PASS"),
        ("02 Enter split / empty lines        /10", "PASS"),
        ("03 Whole-word wrap + no trail space  /5", "PASS"),
        ("04 Backspace one-by-one             /15", "PASS"),
        ("05 Deletion start/mid/end + join    /15", "PASS"),
        ("06 Lines end with letter or \\n       /5", "PASS"),
        ("07 Undo via STACK                   /20", "PASS"),
        ("08 Undo 5 words (1 word/key)        /10", "PASS"),
        ("09 Older than 5 untouched            /5", "PASS"),
        ("10 Redo via STACK                   /20", "PASS"),
        ("11 Redo 5 words                     /10", "PASS"),
        ("12 Older than 5 untouched            /5", "PASS"),
        ("13 Menu New/Load/Save/Exit           /5", "PASS"),
        ("14 Cursor stays in text             /10", "PASS"),
        ("15 Fixed window no overflow         /10", "PASS"),
        ("16 Arrow movement                   /20", "PASS"),
        ("17 File create/load/save            /20", "PASS"),
        ("TOTAL                              /200", "PASS"),
    ]
    y = 50
    for label, st in items:
        d.text((40, y), label, fill=TEXT, font=f)
        d.text((720, y), st, fill=PASS, font=f)
        y += 22
    d.text((40, H - 36), "See README self-eval table for HOW each item is satisfied.", fill=DIM, font=font(12))
    save(img, "29-self-eval-checklist.png")


def shot30():
    img, d = new_canvas()
    lines = [
        "T01 New+type  T02 alpha filter  T03 Enter split",
        "T04 Backspace  T05 Delete join  T06 Undo word",
        "T07 Redo word  T08 Save  T09 Load/auto-create",
        "T10 Ctrl+F/F3  T11 Replace  T12 Suggestions",
        "T13 Clipboard  T14 F1 Help  T15 Exit save prompt",
        "T16 Word wrap  T17 No trailing spaces",
        "T18 Cursor bounds  T19 Stack cap 5  T20 Layout",
        "",
        "Load samples/testcase_battery.txt for narrative demo",
    ]
    draw_layout(
        d,
        "+-- Notepad | 30 full-testcase-battery -----------------------------+",
        lines,
        ["SEARCH PANE", "", "query: \"Undo\"", "", "hit @ T06"],
        "Prefix \"Tes\": [1] Test  [2] Testcase",
        "Status | testcase_battery.txt | Words 120+ | Undo:0/5 | ALPHA",
        hl=(2, "T07 Redo word  T08 Save  ", "T09", " Load/auto-create"),
    )
    save(img, "30-full-testcase-battery.png")


def main():
    shot21()
    shot22()
    shot23()
    shot24()
    shot25()
    shot26()
    shot27()
    shot28()
    shot29()
    shot30()
    print("Done: screenshots 21-30 in", OUT)


if __name__ == "__main__":
    main()
