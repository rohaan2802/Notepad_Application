# Notepad Application (CS218 Data Structures — Assignment 02)

Windows **console notepad** aligned with the official Fall 2024 brief:

- [`docs/DS Assignment02, Fall 2024.pdf`](docs/DS%20Assignment02%2C%20Fall%202024.pdf)
- [`docs/DS Assignment02 Self Evaluation Sheet.pdf`](docs/DS%20Assignment02%20Self%20Evaluation%20Sheet.pdf)

The document is a **2-D linked grid** of `Node` cells (`left` / `right` / `up` / `down` only) — **not** a `char[][]` buffer.  
**Undo / Redo** are **linked-list STACKS of WORDS** (capacity **5** each direction). Each `Ctrl+Z` / `Ctrl+Y` undoes or redoes **one word**.

**Author:** Mohammad Rohaan (22I-2327) · [rohaan2802](https://github.com/rohaan2802)  
**Solution:** `Project1.sln` · `i222327_Assignment02.vcxproj`  
**Sources:** `Header.h`, `Source.cpp`

> Browse the full visual walkthrough in [`docs/screenshots/`](docs/screenshots/) — **30** annotated panels covering the editor UI, core editing flows, and assignment rubric scenarios (including long-paragraph wrap and end-to-end test cases).

<p align="center">
  <img src="docs/screenshots/28-layout-60-20-20.png" alt="60/20/20 layout" width="720" />
</p>

---

## Table of contents

1. [Official requirements summary](#official-requirements-summary)
2. [Self-evaluation sheet (200 pts)](#self-evaluation-sheet-200-pts)
3. [Architecture](#architecture)
4. [Data structures](#data-structures)
5. [Keymap](#keymap)
6. [Extras beyond the rubric](#extras-beyond-the-rubric)
7. [Extended Mode (`Ctrl+E`)](#extended-mode-ctrle)
8. [Build and run](#build-and-run)
9. [Samples](#samples)
10. [Test cases (T01–T20)](#test-cases-t01t20)
11. [Screenshot gallery (01–30)](#screenshot-gallery-0130)
12. [Docs & reference sources](#docs--reference-sources)
13. [Limitations](#limitations)
14. [Author](#author)

---

## Official requirements summary

From **DS Assignment02, Fall 2024**:

| Topic | Requirement |
|-------|-------------|
| Model | 2-D linked list; each node = `char` + `left/right/up/down` |
| Input | Character-by-character (no `getline` for document typing) |
| Insert | Shift forward (no overwrite); start / middle / end / between lines |
| Alphabet | **Only A–Z / a–z** (non-letters ignored) |
| Wrap | If the current word cannot fit, **move the entire word** to the next line |
| Enter | Mid-line splits remainder; multiple Enter → empty lines |
| Delete | Backspace one-by-one; remaining text shifts left; join lines |
| Trailing spaces | **Penalty** if lines keep trailing spaces — lines should end with a letter or `\n` |
| Undo/Redo | **Stacks**; each action = **one word**; last **5 words** only |
| Window | Fixed (non-scrollable): **~60% text**, **~20% Suggestions (bottom)**, **~20% Search (right)** |
| Cursor | Free inside text; **not** into empty regions beyond text |
| Menu | New / Load / Save / Exit (save-before-quit) |
| Files | Missing save/load path → **auto-create** |
| Cleanup | Free all nodes on exit |
| Classes | Must use classes; avoid arrays / STL / `std::string` for the document |

Official self-eval sheet totals **/200**. Cursor-movement sample helper is included as [`docs/CursorMovment.cpp`](docs/CursorMovment.cpp).

---

## Self-evaluation sheet (200 pts)

Honest mapping of how **this repo** satisfies each row (default path = **ALPHA-ONLY**, Extended Mode off).

| Sr | Item | Pts | Status | How this project satisfies it |
|----|------|-----|--------|-------------------------------|
| 1 | Correct text insertion — no getline for typing; alphabets only; shift forward; insert anywhere | /15 | **PASS** | `ReadConsoleInput` + `typeChar`; non-letters ignored; `Document::insertChar` links a new `Node` (no overwrite) |
| 2 | Enter key — remainder to next line; empty Enter → empty lines | /10 | **PASS** | `Document::insertNewline` |
| 3 | Whole-word wrap; no trailing spaces; no mid-word split | /5 | **PASS** | `wrapCurrentWordToNextLine`; `stripTrailingSpaces` on Enter / wrap / save |
| 4 | Backspace deletes one-by-one | /15 | **PASS** | `Document::backspace` |
| 5 | Deletion complete — start/mid/end + delete between lines | /15 | **PASS** | Backspace + `deleteForward` join previous/next rows |
| 6 | No trailing spaces — line ends with alphabet or `\n` | /5 | **PASS** | Strip on Enter/wrap/save; spaces only as mid-line separators while typing |
| 7 | Undo via **stack** only | /20 | **PASS** | `WordStack` linked list (`WordAction* next`) |
| 8 | Undo **5 words**, one word per key | /10 | **PASS** | Cap `WORD_STACK_CAP = 5`; `Ctrl+Z` pops one word |
| 9 | Actions beyond 5 words untouched | /5 | **PASS** | Oldest stack node dropped when pushing 6th |
| 10 | Redo via **stack** only | /20 | **PASS** | Separate redo `WordStack` |
| 11 | Redo 5 words, one word per key | /10 | **PASS** | `Ctrl+Y` redoes one word; cap 5 |
| 12 | Beyond 5 words untouched by redo | /5 | **PASS** | Same capacity / drop-oldest policy |
| 13 | Menu New / Load / Save / Exit (+ save before quit) | /5 | **PASS** | Menu options 1–4; Exit confirms save if dirty. Save As / Help kept as **extras** |
| 14 | Cursor does not enter areas without text | /10 | **PASS** | `setCursor` / moves clamp to existing lines & line lengths |
| 15 | Fixed window; words do not overflow defined area | /10 | **PASS** | Fixed `TEXT_COLS`/`TEXT_ROWS`; wrap before overflow |
| 16 | Cursor movement up/right/down/left | /20 | **PASS** | Arrow keys + Home/End |
| 17 | File handling — create / load / save | /20 | **PASS** | Load missing → auto-create empty `.txt`; save writes linked-list text |
| 18 | Plagiarism | −200% | — | Original class-based redesign |
| — | **Total** | **/200** | **PASS** | See screenshots 21–30 + self-eval checklist PNG |

<p align="center">
  <img src="docs/screenshots/29-self-eval-checklist.png" alt="Self-eval checklist" width="720" />
</p>

---

## Architecture

```text
main()
  └─ NotepadApp::run()
        ├─ showMainMenu()     1 New | 2 Load | 3 Save | 4 Exit
        │                     (+ 5 Continue | 6 Save As | 7 Help extras)
        └─ ReadConsoleInput loop
              ├─ Document       2D Node grid (left/right/up/down)
              ├─ WordHistory    undo_ / redo_ WordStack (cap 5 words)
              └─ UI frames      text ~60% | Search right ~20% | Suggestions bottom ~20%
```

`Header.h` declares types and the app API.  
`Source.cpp` implements document, word stacks, UI, and `main`.

Layout constants (`TEXT_*`, `SEARCH_*`, `SUGGEST_*`) fix **non-scrollable** frames.

---

## Data structures

```text
Node
  char data
  Node* left, *right, *up, *down     // document ONLY uses these four links

CharNode                             // linked chars for a word (avoid std::string)
  char ch; CharNode* next

WordAction                           // one stack frame = one WORD
  CharNode* word
  int row, col
  bool inserted
  WordAction* next

WordStack                            // linked stack, capacity 5
  WordAction* top_; int depth_

WordHistory
  WordStack undo_, redo_
```

Empty lines keep a placeholder node (`data == '\0'`, no `right`) so the row spine stays valid.  
On exit, `Document` / stack destructors free all heap nodes.

**Note on `std::string` / STL:** the document path uses `char` nodes and small Win32/`char[]` buffers (`MAX_PATH_BUF`, query buffers). `fstream` is used for file I/O. Filename buffers are required by Win32 prompts. This is intentional minimization vs. storing the document in `std::string` or `char[][]`.

---

## Keymap

| Key | Action | Rubric? |
|-----|--------|---------|
| `A–Z` / `a–z` | Insert letter into linked grid | Required |
| `Space` | Word separator (mid-line); commits pending word to undo stack | Required |
| digits / punct | **Ignored** unless Extended Mode | Required default |
| `Enter` | Split remainder / empty line; strip trailing spaces | Required |
| `Backspace` | Delete left / join previous line | Required |
| `Delete` | Delete under cursor / join next line | Required+ |
| Arrows / Home / End | Move within text only | Required |
| `Ctrl+Z` / `Ctrl+Y` | Undo / redo **one word** (stack cap 5) | Required |
| Esc → Menu `1–4` | New / Load / Save / Exit | Required |
| `Ctrl+N` / `Ctrl+O` / `Ctrl+S` | New / Load / Save hotkeys | Extra |
| `Ctrl+F` / `F3` | Find (right Search pane) / Find next | Extra (pane required now) |
| `Ctrl+H` | Replace | Extra |
| `Ctrl+C` / `X` / `V` | Copy / Cut / Paste | Extra |
| `Ctrl+E` | Toggle **Extended Mode** (digits/punct) | Extra |
| `F1` | Help overlay + rubric mapping | Extra |
| Menu `6` Save As / `7` Help | Convenience | Extra |

Word boundaries for undo are recorded on **Space**, **Enter**, or after **backspacing an entire word**.

---

## Extras beyond the rubric

- Color UI (title / search / suggestions / status attributes)
- Status bar: path, dirty `*`, ln/col, words, chars, undo/redo depth, ALPHA|EXT
- Live **suggestions from document words** (prefix match) in the bottom 20% frame
- Right **Search pane** shows last query + hit location (Assn3 placeholder when idle)
- Clipboard word/line copy-cut-paste
- Sample files with **long paragraphs** for wrap demos
- Pillow screenshot generator: `docs/generate_screenshots_extra.py`

---

## Extended Mode (`Ctrl+E`)

| Mode | Behavior |
|------|----------|
| **OFF (default)** | Strict rubric: **alphabetic only**; digits/punctuation ignored (including filtered paste) |
| **ON** | Also allows printable ASCII digits/punctuation for demos |

Be honest in grading: **self-eval PASS claims refer to ALPHA-ONLY (Extended OFF).** Extended Mode is an opt-in teaching aid and must not be confused with the graded default.

---

## Build and run

**Windows only** (Win32 console APIs + `MessageBoxW`).

1. Open `Project1.sln` in Visual Studio 2022, **or**
2. MSBuild:

```bat
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" Project1.sln /p:Configuration=Debug /p:Platform=x64
```

3. Run `x64\Debug\Project1.exe` from the project folder (so `samples/` resolves).

Verified: **Debug \| x64** builds successfully after the rubric alignment rewrite.

---

## Samples

| File | Purpose |
|------|---------|
| `samples/long_paragraph.txt` | Long alpha paragraphs for whole-word wrap demos |
| `samples/testcase_battery.txt` | Narrative covering T01–T20 |
| `samples/welcome.txt` | Overview text |
| `samples/poem.txt` | Short poem |
| `samples/notes.txt` | Checklist-style notes |
| `samples/search_demo.txt` | Find / replace practice |
| `samples/charset.txt` | Historical charset sample (use with Extended Mode if digits present) |

Load from menu option **2** with names like `long_paragraph` or `samples/long_paragraph`.  
If the file is missing, the app **auto-creates** an empty `.txt`.

---

## Test cases (T01–T20)

| ID | Steps | Expected |
|----|-------|----------|
| T01 | Menu → 1 New → type `Hello` | Letters appear; status words/chars; dirty `*` |
| T02 | Type `123!?` with Extended OFF | Ignored; document unchanged |
| T03 | Enter mid-line | Remainder moves to next row; cursor col 0 |
| T04 | Backspace mid-word | Prior char removed; text shifts left |
| T05 | Delete at EOL with next line | Lines join if fit width |
| T06 | Type five words + spaces → `Ctrl+Z` | Last **word** removed |
| T07 | `Ctrl+Y` after undo | Word restored via redo stack |
| T08 | Type 7 words; undo 5 times | First 2 words remain (beyond cap untouched) |
| T09 | Save As `demo` | File written; no trailing spaces on lines |
| T10 | Restart → Load `demo` | Content reloads into Node grid |
| T11 | Load missing `brand_new_file` | Empty file auto-created |
| T12 | Open `long_paragraph` | Whole-word wrap visible across lines |
| T13 | `Ctrl+F` query → highlight; `F3` next | Right Search pane shows query + hit |
| T14 | Type prefix seen in doc | Bottom suggestions list matches |
| T15 | `Ctrl+H` replace (extra) | First match replaced |
| T16 | `F1` | Help / rubric keymap overlay |
| T17 | Arrows into blank below last line | Cursor stays on existing text |
| T18 | Insert mid-line letter | Right side shifts; no overwrite |
| T19 | Exit with dirty doc | Save-before-quit prompt |
| T20 | Visual layout check | Text / Search / Suggestions frames present (~60/20/20) |

---

## Screenshot gallery (01–30)

### Feature tour (01–20)

| # | Screenshot | Feature |
|---|------------|---------|
| 1 | ![menu](docs/screenshots/01-main-menu.png) | Main menu |
| 2 | ![new](docs/screenshots/02-new-file.png) | New file |
| 3 | ![empty](docs/screenshots/03-editor-empty.png) | Empty editor chrome |
| 4 | ![type](docs/screenshots/04-typing-text.png) | Typing |
| 5 | ![nav](docs/screenshots/05-navigation-arrows.png) | Navigation |
| 6 | ![bksp](docs/screenshots/06-backspace-delete.png) | Backspace / Delete |
| 7 | ![undo](docs/screenshots/07-undo-command.png) | Undo |
| 8 | ![redo](docs/screenshots/08-redo-command.png) | Redo |
| 9 | ![find](docs/screenshots/09-search-prompt.png) | Find prompt |
| 10 | ![hl](docs/screenshots/10-search-highlight.png) | Search highlight |
| 11 | ![sug](docs/screenshots/11-word-suggestions.png) | Suggestions |
| 12 | ![save](docs/screenshots/12-save-success.png) | Save |
| 13 | ![open](docs/screenshots/13-open-load.png) | Load |
| 14 | ![status](docs/screenshots/14-status-bar.png) | Status bar |
| 15 | ![help](docs/screenshots/15-help-keymap.png) | Help |
| 16 | ![repl](docs/screenshots/16-replace-dialog.png) | Replace |
| 17 | ![clip](docs/screenshots/17-clipboard-paste.png) | Clipboard |
| 18 | ![exit](docs/screenshots/18-exit-confirm.png) | Exit confirm |
| 19 | ![arch](docs/screenshots/19-architecture-overview.png) | Architecture |
| 20 | ![poem](docs/screenshots/20-sample-poem.png) | Sample poem |

### Rubric / long-paragraph battery (21–30)

| # | Screenshot | Feature |
|---|------------|---------|
| 21 | ![wrap](docs/screenshots/21-long-paragraph-wrap.png) | Long paragraph + whole-word wrap |
| 22 | ![alpha](docs/screenshots/22-alpha-only-filter.png) | Digits/punct ignored (alpha-only) |
| 23 | ![shift](docs/screenshots/23-insert-middle-shift.png) | Mid-line insert shifts right |
| 24 | ![enter](docs/screenshots/24-enter-split-line.png) | Enter splits line |
| 25 | ![join](docs/screenshots/25-backspace-join-lines.png) | Backspace at col 0 joins lines |
| 26 | ![undo5](docs/screenshots/26-undo-five-words-stack.png) | Undo last 5 words (stack) |
| 27 | ![redo5](docs/screenshots/27-redo-five-words-stack.png) | Redo stack |
| 28 | ![lay](docs/screenshots/28-layout-60-20-20.png) | Layout 60% / 20% / 20% |
| 29 | ![eval](docs/screenshots/29-self-eval-checklist.png) | Self-eval 200 pts checklist |
| 30 | ![bat](docs/screenshots/30-full-testcase-battery.png) | T01–T20 narrative battery |

Regenerate 21–30:

```bat
python docs\generate_screenshots_extra.py
```

<p align="center">
  <img src="docs/screenshots/21-long-paragraph-wrap.png" width="480" />
  <img src="docs/screenshots/26-undo-five-words-stack.png" width="480" />
</p>

---

## Docs & reference sources

| Path | Contents |
|------|----------|
| `docs/DS Assignment02, Fall 2024.pdf` | Official assignment |
| `docs/DS Assignment02 Self Evaluation Sheet.pdf` | Official /200 sheet |
| `docs/CursorMovment.cpp` | Provided cursor-movement sample |
| `docs/reference/` | Small copy of earlier `Project1` sources (`Header.h`, `Source.cpp`, `rohaan.txt`) |
| `docs/screenshots/` | PNG gallery 01–30 |
| `docs/generate_screenshots_extra.py` | Generator for 21–30 |

Build trees under `docs/Project1/.vs/`, `x64/`, `*.pdb`, etc. are **gitignored**.

---

## Limitations

- Console UI approximates 60/20/20 with fixed character-cell frames (not a pixel-perfect GUI).
- Word undo records completed words (Space/Enter/backspace-word). Mid-word undo of a single letter while still typing peels the pending buffer rather than a stack pop.
- Join-line backspace/delete refuses the join if the combined line would exceed `TEXT_COLS` (prevents overflow).
- Find/Replace UI still uses a short console prompt (results display in the **required** right Search pane). Full Assn3 search UX is left as the placeholder notes.
- `samples/charset.txt` may contain digits — open under Extended Mode or expect filtering on load.
- Official classroom zip format (`ROLL_SECTION_02.zip` single `.cpp`) differs from this GitHub multi-file VS project layout.

---

## Author

**Mohammad Rohaan** — 22I-2327  
[https://github.com/rohaan2802](https://github.com/rohaan2802)  
Project: [Notepad_Application](https://github.com/rohaan2802/Notepad_Application)
