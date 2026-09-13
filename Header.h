#pragma once
#ifndef NOTEPAD_HEADER_H
#define NOTEPAD_HEADER_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <conio.h>

#include <fstream>
#include <iostream>
#include <string>

// Console geometry (writing pane)
const int PANE_LEFT = 2;
const int PANE_TOP = 2;
const int PANE_RIGHT = 118;
const int PANE_BOTTOM = 28;
const int STATUS_ROW = 30;
const int SUGGEST_ROW = 32;
const int MAX_SUGGESTIONS = 6;

// ---------- Document: 2-D linked grid (no char[][] buffer) ----------
struct Node {
    char data;
    Node* left;
    Node* right;
    Node* up;
    Node* down;

    explicit Node(char value = '\0')
        : data(value), left(nullptr), right(nullptr), up(nullptr), down(nullptr) {}
};

class Document {
public:
    Document();
    ~Document();

    void clear();
    bool empty() const;

    int cursorRow() const { return row_; }
    int cursorCol() const { return col_; }
    void setCursor(int r, int c);

    void moveLeft();
    void moveRight();
    void moveUp();
    void moveDown();
    void moveHome();
    void moveEnd();
    void moveDocHome();
    void moveDocEnd();

    // Insert printable / space at cursor; advances cursor. Returns false if full.
    bool insertChar(char ch);
    // Insert newline (split / new row).
    bool insertNewline();
    // Backspace: erase char left of cursor.
    bool backspace(char& removed);
    // Delete: erase char under cursor.
    bool deleteForward(char& removed);

    // Undo helpers that do not move semantics beyond explicit positions.
    bool insertAt(int r, int c, char ch);
    bool eraseAt(int r, int c, char& removed);
    bool insertNewlineAt(int r, int c);
    bool joinLineWithPrevious(int r);

    int lineCount() const;
    int lineLength(int r) const;
    int wordCount() const;
    int charCount() const;

    char charAt(int r, int c) const;
    std::string lineText(int r) const;
    std::string wordAtCursor() const;
    std::string allText() const;

    bool findNext(const std::string& query, int& outRow, int& outCol, bool wrap) const;
    bool replaceAtCursor(const std::string& findStr, const std::string& replaceStr);

    bool saveToFile(const std::string& path) const;
    bool loadFromFile(const std::string& path);

    void render(int highlightRow, int highlightCol, int highlightLen) const;

private:
    Node* head_;
    int row_;
    int col_;

    Node* rowHead(int r) const;
    Node* nodeAt(int r, int c) const;
    Node* ensureRow(int r);
    void destroy();
    int maxRows() const { return PANE_BOTTOM - PANE_TOP + 1; }
    int maxCols() const { return PANE_RIGHT - PANE_LEFT; }
};

// ---------- Undo / redo: doubly-linked command list ----------
enum class CmdKind {
    InsertChar,
    DeleteChar,   // backspace or delete recorded with position of removed char
    InsertLine,
    JoinLine      // undo of InsertLine
};

struct EditCommand {
    CmdKind kind;
    char ch;
    int row;
    int col;
    EditCommand* prev;
    EditCommand* next;

    EditCommand(CmdKind k, char c, int r, int co)
        : kind(k), ch(c), row(r), col(co), prev(nullptr), next(nullptr) {}
};

class CommandHistory {
public:
    CommandHistory();
    ~CommandHistory();

    void clear();
    void record(CmdKind kind, char ch, int row, int col);
    bool canUndo() const;
    bool canRedo() const;
    bool undo(Document& doc);
    bool redo(Document& doc);

private:
    EditCommand* head_;
    EditCommand* current_; // last executed command; redo uses current_->next

    void discardRedoBranch();
    void destroyList(EditCommand* n);
};

// ---------- Dictionary suggestions (linked list of words) ----------
struct WordNode {
    std::string word;
    WordNode* next;
    explicit WordNode(const std::string& w) : word(w), next(nullptr) {}
};

class Dictionary {
public:
    Dictionary();
    ~Dictionary();
    void suggest(const std::string& prefix, WordNode*& outHead, int maxCount) const;

private:
    WordNode* words_;
    void add(const char* w);
};

// ---------- Editor / UI ----------
class NotepadApp {
public:
    NotepadApp();
    int run();

private:
    Document doc_;
    CommandHistory history_;
    Dictionary dictionary_;

    std::string filePath_;
    bool dirty_;
    bool running_;
    bool showHelp_;

    std::string findQuery_;
    std::string replaceQuery_;
    int hlRow_;
    int hlCol_;
    int hlLen_;

    std::string clipboard_;

    void maximizeConsole();
    void gotoxy(int x, int y) const;
    void clearScreen() const;
    void drawChrome() const;
    void drawStatus() const;
    void drawSuggestions() const;
    void refresh();

    void showMainMenu();
    bool promptFileName(const char* title, std::string& out);
    bool confirmDiscard();
    void messageBoxInfo(const wchar_t* text, const wchar_t* caption) const;
    int messageBoxYesNo(const wchar_t* text, const wchar_t* caption) const;

    void actionNew();
    void actionOpen();
    void actionSave();
    void actionSaveAs();
    void actionFind();
    void actionFindNext();
    void actionReplace();
    void actionCopy();
    void actionCut();
    void actionPaste();
    void actionHelp();
    void actionUndo();
    void actionRedo();

    void handleKey(const KEY_EVENT_RECORD& key);
    void typeChar(char ch);
    void doBackspace();
    void doDelete();
    void doEnter();

    std::string readPromptLine(const char* label);
};

void setConsoleTitleBar(const wchar_t* title);

#endif
