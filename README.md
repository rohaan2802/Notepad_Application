# Professional Notepad Application

Windows **console notepad** for a Data Structures assignment — upgraded to a **professional editor**.

The document is a **2-D linked grid** of `Node` cells (`left` / `right` / `up` / `down`), **not** a `char[][]` buffer.  
**Undo / redo** use a **doubly-linked list of `EditCommand` nodes** (command pattern), not an array stack.

**Author:** Mohammad Rohaan (22I-2327) · [rohaan2802](https://github.com/rohaan2802)  
**Solution:** `Project1.sln` · `i222327_Assignment02.vcxproj`  
**Sources:** `Header.h`, `Source.cpp`

> **Demo without building:** open [`docs/screenshots/`](docs/screenshots/) — 20 PNGs cover every major feature.

<p align="center">
  <img src="docs/screenshots/01-main-menu.png" alt="Main menu" width="720" />
</p>

---

## Table of contents

1. [Features](#features)
2. [Screenshot gallery](#screenshot-gallery)
3. [Architecture](#architecture)
4. [Keymap](#keymap)
5. [Data structure](#data-structure)
6. [Samples](#samples)
7. [Build and run](#build-and-run)
8. [Test cases](#test-cases)
9. [Assignment compliance](#assignment-compliance)
10. [Author](#author)

---

## Features

| Area | What you get |
|------|----------------|
| Document model | Single shared `Document` (fixes old dual `list` / `list1` bug) |
| Typing | Letters, digits, punctuation, space (printable ASCII 32–126) |
| Editing | Backspace, Delete, Enter (split / join lines) |
| Navigation | Arrows, Home / End, Ctrl+Home / Ctrl+End |
| Undo / Redo | `Ctrl+Z` / `Ctrl+Y` via linked `EditCommand` list |
| Files | New, Open, Save, Save As (menu + hotkeys); dirty `*` flag |
| Search | `Ctrl+F` + highlighted match; `F3` find next (wraps) |
| Replace | `Ctrl+H` find + replace with history recording |
| Suggestions | Live **WORD SUGGESTIONS** from a linked dictionary |
| Clipboard | `Ctrl+C` / `Ctrl+X` / `Ctrl+V` (word or line) |
| Status bar | Path, dirty, line/col, word/char counts, undo/redo flags |
| Help | `F1` keymap overlay; menu option 6 |
| Safety | Unsaved-change prompts on New / Open / Exit |

---

## Screenshot gallery

| # | Screenshot | Feature shown |
|---|------------|----------------|
| 1 | ![menu](docs/screenshots/01-main-menu.png) | Main menu |
| 2 | ![new](docs/screenshots/02-new-file.png) | New file |
| 3 | ![empty](docs/screenshots/03-editor-empty.png) | Empty editor chrome |
| 4 | ![type](docs/screenshots/04-typing-text.png) | Typing text + digits/punctuation |
| 5 | ![nav](docs/screenshots/05-navigation-arrows.png) | Cursor navigation |
| 6 | ![bksp](docs/screenshots/06-backspace-delete.png) | Backspace / Delete |
| 7 | ![undo](docs/screenshots/07-undo-command.png) | Undo (command list) |
| 8 | ![redo](docs/screenshots/08-redo-command.png) | Redo |
| 9 | ![find](docs/screenshots/09-search-prompt.png) | Find prompt |
| 10 | ![hl](docs/screenshots/10-search-highlight.png) | Search highlight |
| 11 | ![sug](docs/screenshots/11-word-suggestions.png) | Word suggestions |
| 12 | ![save](docs/screenshots/12-save-success.png) | Save success |
| 13 | ![open](docs/screenshots/13-open-load.png) | Open / load |
| 14 | ![status](docs/screenshots/14-status-bar.png) | Status bar |
| 15 | ![help](docs/screenshots/15-help-keymap.png) | Help / keymap overlay |
| 16 | ![repl](docs/screenshots/16-replace-dialog.png) | Replace |
| 17 | ![clip](docs/screenshots/17-clipboard-paste.png) | Clipboard paste |
| 18 | ![exit](docs/screenshots/18-exit-confirm.png) | Exit confirm |
| 19 | ![arch](docs/screenshots/19-architecture-overview.png) | Architecture overview |
| 20 | ![poem](docs/screenshots/20-sample-poem.png) | Loaded sample poem |

<p align="center">
  <img src="docs/screenshots/04-typing-text.png" width="480" />
  <img src="docs/screenshots/10-search-highlight.png" width="480" />
</p>
<p align="center">
  <img src="docs/screenshots/07-undo-command.png" width="480" />
  <img src="docs/screenshots/15-help-keymap.png" width="480" />
</p>

---

## Architecture

```text
main()
  └─ NotepadApp::run()
        ├─ showMainMenu()          New / Open / Save / Save As / Edit / Help / Exit
        └─ ReadConsoleInput loop
              ├─ Document          2D linked Node grid
              ├─ CommandHistory    doubly-linked EditCommand list
              └─ Dictionary        linked WordNode list (suggestions)
```

`Header.h` declares types and the app API.  
`Source.cpp` implements the document, history, dictionary, UI, and `main`.

---

## Keymap

| Key | Action |
|-----|--------|
| printable ASCII | Insert character into linked grid |
| Enter | New line (split row) |
| Backspace | Delete left / join with previous line |
| Delete | Delete under cursor / join with next line |
| Arrows | Move cursor |
| Home / End | Start / end of line |
| Ctrl+Home / Ctrl+End | Start / end of document |
| Ctrl+N | New file |
| Ctrl+O | Open file |
| Ctrl+S | Save |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |
| Ctrl+F | Find |
| F3 | Find next |
| Ctrl+H | Replace |
| Ctrl+C / X / V | Copy / Cut / Paste |
| F1 | Toggle help overlay |
| Esc | Return to main menu |

---

## Data structure

```text
Node
  char data
  Node* left, *right   // characters on a line
  Node* up, *down      // row spine (row-head links)

EditCommand
  CmdKind kind         // InsertChar | DeleteChar | InsertLine | JoinLine
  char ch
  int row, col
  EditCommand* prev, *next

CommandHistory
  head_ / current_     // current_ = last executed; redo uses current_->next
```

Empty lines keep a placeholder node (`data == '\0'`, no `right`) so the row spine stays valid.

---

## Samples

| File | Purpose |
|------|---------|
| `samples/welcome.txt` | Overview + search demo text |
| `samples/poem.txt` | Short poem |
| `samples/notes.txt` | Checklist-style notes |
| `samples/search_demo.txt` | Find / replace (`needle`) |
| `samples/charset.txt` | Digits + punctuation |

Open from the menu with names like `welcome` or `samples/welcome`.

---

## Build and run

**Windows only** (Win32 console APIs + `MessageBoxW`).

1. Open `Project1.sln` in Visual Studio 2022, or:
2. MSBuild:

```bat
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" Project1.sln /p:Configuration=Debug /p:Platform=x64
```

3. Run `x64\Debug\Project1.exe` from the project folder (so `samples/` resolves).

Verified locally: **Debug | x64** build succeeds.

---

## Test cases

| ID | Steps | Expected |
|----|-------|----------|
| T01 | Menu → 1 New → type `Hello` | Characters appear; status words/chars update; dirty `*` |
| T02 | Type letters + `123!?,.` | All printable ASCII inserts |
| T03 | Enter mid-line | Line splits; cursor at col 1 of new row |
| T04 | Backspace mid-word | Previous char removed; undo available |
| T05 | Delete at EOL with next line | Lines join |
| T06 | Ctrl+Z then Ctrl+Y | Text restores via command list |
| T07 | Type more after undo | Redo branch discarded |
| T08 | Ctrl+S Save As `demo` | `demo.txt` written from grid |
| T09 | Restart → Open `demo` | Content reloads into linked list |
| T10 | Open `samples/welcome` → Ctrl+F `linked` → F3 | Match highlighted; next wraps |
| T11 | Ctrl+H replace `needle`→`target` | First match replaced; dirty |
| T12 | Type `und` | Suggestions show `undo` |
| T13 | Ctrl+C word, move, Ctrl+V | Word pasted |
| T14 | F1 | Keymap overlay toggles |
| T15 | Edit then menu Exit without save | Confirm dialog appears |
| T16 | Fill a long line to pane width | “Space full” warning; no crash |

---

## Assignment compliance

From `rohaan.txt`:

- **No array document** — text lives only in the `Node` grid.
- **Linked lists for structure** — rows/cols + undo/redo commands + dictionary words.
- **Intuitive keys** — documented above and in the F1 overlay.
- **Insert / delete before undo/redo** — both work and are recorded as commands.
- **You test thoroughly** — see the table above; samples exercise search/replace.

---

## Author

**Mohammad Rohaan** — 22I-2327  
[https://github.com/rohaan2802](https://github.com/rohaan2802)  
Project: [Notepad_Application](https://github.com/rohaan2802/Notepad_Application)
