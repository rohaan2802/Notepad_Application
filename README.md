# Notepad_Application

Windows **console notepad** that stores typed characters in a **2D linked structure** (nodes with `left` / `right` / `up` / `down`). File menu supports new, load, and save. Sources: `Source.cpp`, `Header.h`.

## Overview

- Maximized console editor with a framed writing area (approx. columns `left_=1` … `right_=143`, rows `top_=0` … `bottom_=31`)
- Text stored in `linked_list` of `Node` cells for 2D cursor navigation
- Win32 console input via `ReadConsoleInput` (arrow keys, Enter, Backspace, F1 menu)
- Win32 `MessageBox` for save confirmations and “space full” warnings
- Sample text files in repo: `er.txt`, `rohaan.txt`

## Features

### Main menu (F1 or startup)
1. **New File** — `create_file()`  
2. **Load File** — `load_file()` into the editor buffer  
3. **Save File** — success dialog if content is marked safe (`safe_flag`)  
4. **Exit** — optional save confirmation  

### Editor
- Arrow keys move cursor within bounds (`gotoxy` + `SetConsoleCursorPosition`)
- A–Z / a–z insert into the linked list and echo to console
- Enter advances line; Backspace deletes via `delete_at_end` / redraw helpers
- Full-buffer warning when cursor hits bottom-right
- Header UI mentions “SEARCH” and “WORD SUGGESTIONS” regions (layout placeholders in `display_fun`)

## Tech stack

| Component | Technology |
|-----------|------------|
| Language | C++ |
| Platform | Windows (`Windows.h`, `conio.h`, console APIs) |
| Persistence | `<fstream>` |
| IDE | Visual Studio (`Project1.sln`, `i222327_Assignment02.vcxproj`) |

## Project structure

```
Notepad_Application/
├── Source.cpp              # main input loop
├── Header.h                # menu, Node, linked_list, file helpers, UI
├── Project1.sln
├── i222327_Assignment02.vcxproj
├── i222327_Assignment02.vcxproj.filters
├── er.txt
└── rohaan.txt
```

## How to build / run

**Windows only** (uses `HWND`, `MessageBox`, console input handles).

1. Open `Project1.sln` in Visual Studio.  
2. Build and run (prefer Console subsystem).  
3. Or: `cl Source.cpp` / MSVC project build with Windows SDK.

## Usage

1. On launch, console maximizes and the main menu appears.  
2. Create or load a file, then the notepad frame is drawn.  
3. Type letters; move with arrows; Backspace to delete; Enter for new line.  
4. Press **F1** to return to the menu for save/load/exit.  
5. Confirm save dialogs when exiting with unsaved work (as implemented).

## How to extend / modify

- Implement the SEARCH / word-suggestion areas wired in the header UI.  
- Persist the full linked-list contents on save (ensure `save_file` / `insert_character` stay consistent).  
- Support digits, punctuation, and Unicode if needed (currently letter-focused).  
- Extract `Header.h` implementations into `.cpp` files for cleaner builds.

## Author

**rohaan2802** (assignment id i222327) — [https://github.com/rohaan2802](https://github.com/rohaan2802)
