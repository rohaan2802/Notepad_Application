# Notepad_Application

Windows **console notepad** for a Data Structures assignment. The document is a **2-D linked grid** (`Node` with `left` / `right` / `up` / `down`), not a `char[][]` buffer. File menu: new, load, save, exit. Visual Studio project id **i222327_Assignment02**.

**Sources:** `Header.h`, `Source.cpp`  
**Solution:** `Project1.sln` · `i222327_Assignment02.vcxproj`  
**Author:** Mohammad Rohaan (22I-2327) · [rohaan2802](https://github.com/rohaan2802)

`rohaan.txt` in this repo restates the brief: do **not** use arrays for the document; use linked lists; map keys for save / next line; insertion and deletion must work before undo/redo would be marked.

---

## Table of contents

1. [What this program does](#what-this-program-does)
2. [Architecture](#architecture)
3. [Data structure](#data-structure)
4. [File-by-file](#file-by-file)
5. [Console geometry and chrome](#console-geometry-and-chrome)
6. [Menus and persistence](#menus-and-persistence)
7. [Keyboard input (`Source.cpp`)](#keyboard-input-sourcecpp)
8. [Load-file editor (`load_file`)](#load-file-editor-load_file)
9. [Data files](#data-files)
10. [Build and run](#build-and-run)
11. [Limitations](#limitations)
12. [Author](#author)

---

## What this program does

1. Maximize the console (`maximizeConsole` / `ShowWindow(..., SW_MAXIMIZE)`).
2. Show a numbered file menu (`show_menu`).
3. Draw a framed writing area (`display_fun`).
4. Accept letters, arrows, Enter, Backspace, and **F1** (back to menu).
5. Store each typed character in a `linked_list` of `Node` cells and, on insert, append to a `.txt` file via `insert_character`.

This is a **custom text editor**, not Microsoft Notepad. Digits, punctuation, and Unicode are not part of the insert path in `main`.

---

## Architecture

```text
main()                    Header.h globals + menu
  maximizeConsole()
  show_menu()             1 New / 2 Load / 3 Save / 4 Exit
  display_fun()           frame + SEARCH / WORD SUGGESTIONS chrome
  ReadConsoleInput loop   arrows, letters, Enter, Backspace, F1
        │
        ├── list.insert_at_end / list.delete_at_end / list.fun1
        ├── insert_character  → ofstream append to file_
        └── F1 → show_menu()
```

`Header.h` is not a pure header: it **defines** `Node`, `linked_list`, `show_menu`, `create_file`, `load_file`, `insert_character`, `save_file`, and Win32 helpers. `Source.cpp` includes it and runs the live editor loop.

Two list objects exist:

| Object | Where | Used by |
|--------|--------|---------|
| `list1` | `Header.h` | `load_file` |
| `list` | `Source.cpp` | `main` key loop |

They do **not** share nodes. Loading a file fills `list1`; typing in the main loop fills `list`.

---

## Data structure

```text
class Node
  char data
  Node* left, *right, *up, *down
  Node(char value)  // all links nullptr

class linked_list
  Node* head
  insert_at_end(char value, int &x, int &y)
  delete_at_end(int x, int y)
  fun1(int x)              // reprint one row at gotoxy(1, x)
  delete_from_file()       // rewrite file_ from the grid
```

`insert_at_end` walks `down` `x` times (creating `\0` row sentinels if needed), then `right` `y` times (creating `\0` column sentinels), then splices a new character node and wires `up`/`down` to the previous row.

`delete_at_end` walks to `(x, y)`, copies the next node’s `data` leftward (or writes `'\0'`), then calls `delete_from_file()`.

The assignment point is that **links are the document**. Cursor motion uses integer `x`,`y` plus `gotoxy`, not a stored “current node” pointer.

---

## File-by-file

| File | Role |
|------|------|
| `Header.h` | Constants, globals, `Node` / `linked_list`, menu, file I/O, Win32 UI |
| `Source.cpp` | `main`: maximize, menu, `ReadConsoleInput` editor |
| `Project1.sln` | Visual Studio solution |
| `i222327_Assignment02.vcxproj` | MSBuild project (assignment id) |
| `i222327_Assignment02.vcxproj.filters` | Solution Explorer filters |
| `er.txt` | Sample / scratch text (may be empty) |
| `rohaan.txt` | Assignment notes plus leftover typed text |
| `.gitattributes` / `.gitignore` | Git metadata |

---

## Console geometry and chrome

From `Header.h`:

| Constant | Value | Meaning |
|----------|-------|---------|
| `left_` | 1 | Left of writing area |
| `top_` | 0 | Top row |
| `right_` | 143 | Right bound |
| `bottom_` | 31 | Bottom row |

`gotoxy` uses `SetConsoleCursorPosition`. `clear_fun` overwrites 30 rows with spaces. Hitting `y == bottom_ && x == right_ - 1` sets `isFull` and calls `warning_msg()` (`MessageBox`: “Writing space is full.”).

`display_fun` prints a pipe-bordered banner that includes the word **SEARCH** on the first line and **WORD SUGGESTIONS** under the frame. Those labels are **layout only**; there is no search or dictionary function.

---

## Menus and persistence

`show_menu` (invalid choice recurses; Save uses `goto label`):

| Choice | Action |
|--------|--------|
| 1 New File | `create_file()` |
| 2 Load File | `load_file(x, y, isFull)` |
| 3 Save File | If `safe_flag`, `saved_msg()`; else “CREATE FILE FIRST OR LOAD FILE” |
| 4 Exit | If `safe_flag`, `confrim_msg()`; `IDYES` → `saved_msg()` then `exit(0)` |

`create_file` / `load_file` read up to **12** name characters, then append `.txt` into `file_[20]`. `create_file` opens the file and sets `safe_flag = true`.

Declared helpers:

- `insert_character(char ch)` — `ios::app` one character to `file_`
- `save_file()` — `list1.delete_from_file()` (rewrites from **`list1`**, not `Source.cpp`’s `list`)
- `confrim_msg()` — “Do you want to save the file before exiting?” (`MB_YESNO`)
- `saved_msg()` — “File saved successfully!”

Menu cases 3 and 4 **never call** `save_file()`. Typing in `main` persists only through `insert_character` appends. Backspace in `main` rewrites via `list.delete_from_file()`.

Globals: `ofstream obj1`, `ifstream obj_2`, `flag`, `safe_flag`, `isFull`, `file_`.

---

## Keyboard input (`Source.cpp`)

`main` uses `GetNumberOfConsoleInputEvents` / `ReadConsoleInput` on `INPUT_RECORD` (up to 200 events). First key clears the frame via `clear_fun`.

| Key | Handler |
|-----|---------|
| `VK_UP` / `VK_DOWN` | Move `y` within `top_`…`bottom_` |
| `VK_LEFT` | Decrement `x` if `x > left_` |
| `VK_RIGHT` | If `x < right_ - 1`, `insert_character(' ')` and increment `x` |
| `VK_RETURN` | Next row, `x = left_`, `insert_character('\n')` |
| `VK_BACK` | `list.delete_at_end(y, x)` then `list.fun1(y)` to redraw the row |
| `VK_F1` | `cls` + `show_menu()` |
| Default / letters | After the `switch`, `AsciiChar` in `A–Z` / `a–z` → `list.insert_at_end` + echo + `insert_character` |

The `while (Running)` loop never sets `Running = false`; exit is `exit(0)` from menu option 4. Space is **not** inserted in this loop (only letters). `VK_RIGHT` appends spaces to the file without inserting a `Node`.

---

## Load-file editor (`load_file`)

If `file_` opens:

1. `display_fun()`, then read characters with `obj_2.get`.
2. Keep letters, space, and `'\n'` (skip other bytes).
3. Insert into **`list1`** with `insert_at_end(file_char, y, x)` (note argument order vs `main`, which passes `y, x` as well).
4. Wrap at `right_`; full-buffer → `warning_msg`.
5. If not full, a nested `_getch()` loop until **Esc (27)**: letters, Backspace, Enter (`\r`), space. Then `show_menu()`.

If the file is missing, it creates `file_` (`ios::out`), sets `safe_flag`, and returns without entering the nested editor.

---

## Data files

| File | Contents |
|------|----------|
| `er.txt` | Present in the tree; may be empty |
| `rohaan.txt` | Assignment constraints (no arrays for the document; key mappings; undo/redo not marked unless insert/delete work) plus leftover editor text |

Working directory should be the folder that contains these `.txt` files so `create_file` / `load_file` resolve names.

---

## Build and run

**Windows only:** `Windows.h`, `conio.h`, `HWND`, `MessageBoxW` (`L"..."` strings), `ReadConsoleInput`.

1. Open `Project1.sln` in Visual Studio (Console subsystem).
2. Build and run, or `cl Source.cpp` with the Windows SDK (the `.cpp` includes `Header.h`).
3. Choose 1 or 2, then type in the frame. **F1** returns to the menu. **Esc** leaves the load-file nested editor.

There is no CMake/`g++` portable path: `MessageBox` and console APIs are Win32.

---

## Limitations

- **SEARCH** and **WORD SUGGESTIONS** are printed labels only.
- No undo/redo (mentioned in `rohaan.txt`; not implemented).
- `save_file()` is unused by the menu; Save/Exit dialogs do not rewrite the grid.
- Dual lists (`list` vs `list1`) desynchronize load vs type-in-`main`.
- Insert in `main` is letters only; load path also allows space/newline.
- `file_[20]` and other C arrays exist for names/UI; the **document** is the linked grid.
- `show_menu` uses `goto` and recursion on invalid input.
- Implementations live in `Header.h` (slower incremental builds, ODR risk if included twice).

**Extend (only if you add it):** one shared list; call `save_file` from menu 3/4; wire search; digits/punctuation; real undo stack.

---

## Author

**Mohammad Rohaan** — 22I-2327  
[https://github.com/rohaan2802](https://github.com/rohaan2802)
