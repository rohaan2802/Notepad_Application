# Notepad_Application

Windows **console notepad** that stores characters in a **2-D linked grid** (`Node` with `left` / `right` / `up` / `down`). File menu: new, load, save. Assignment id **i222327**.

**Sources:** `Source.cpp`, `Header.h` · VS: `Project1.sln`, `i222327_Assignment02.vcxproj`  
[rohaan2802](https://github.com/rohaan2802)

---

## Table of contents

1. [Data structure](#data-structure)
2. [Console geometry](#console-geometry)
3. [Input](#input)
4. [Menus](#menus)
5. [Samples](#samples)
6. [Build](#build)
7. [Gaps](#gaps)

---

## Data structure

`linked_list` of `Node` cells so the cursor can move on a **grid**, not a 1-D string:

- Insert letter → new node linked to neighbors  
- Arrows update the current pointer + `gotoxy` / `SetConsoleCursorPosition`  
- Backspace: `delete_at_end` (and redraw helpers)  
- Enter: next row in the 2-D structure  

This is the assignment’s point: notepad **without** a giant `char[][]` only (the links *are* the document).

---

## Console geometry

Framed writing area approximately:

- Columns `left_ = 1` … `right_ = 143`  
- Rows `top_ = 0` … `bottom_ = 31`  

Console is **maximized**. Hitting bottom-right raises a **space full** `MessageBox`. Header chrome mentions **SEARCH** and **WORD SUGGESTIONS** (`display_fun`) — layout placeholders unless you implement them.

---

## Input

Win32 `ReadConsoleInput`:

- Arrows — move inside bounds  
- A–Z / a–z — insert + echo  
- Enter — new line  
- Backspace — delete  
- **F1** — return to file menu  

`MessageBox` for save confirmations and full-buffer warnings.

---

## Menus

Startup / F1:

1. **New File** — `create_file()`  
2. **Load File** — `load_file()`  
3. **Save File** — success dialog if `safe_flag`  
4. **Exit** — optional save confirm  

Persistence: `<fstream>`. Keep save in sync with every insert (easy viva bug if save dumps an empty buffer).

---

## Samples

`er.txt`, `rohaan.txt` in the repo.

---

## Build

**Windows only** (`Windows.h`, `conio.h`, HWND, `MessageBox`).

Open `Project1.sln` → Console subsystem → run. Or `cl Source.cpp` with the Windows SDK.

---

## Gaps

- SEARCH / suggestions not wired.  
- Letters-focused (digits/punctuation/Unicode may be ignored).  
- Implementations live in `Header.h` — split to `.cpp` for cleaner builds.  
- Confirm `save_file` writes the full linked structure.

---

## Author

**rohaan2802** (i222327) · [https://github.com/rohaan2802](https://github.com/rohaan2802)
