#pragma once
#ifndef NOTEPAD_HEADER_H
#define NOTEPAD_HEADER_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <conio.h>

#include <fstream>
#include <iostream>
#include <cctype>
#include <cstring>
#include <cstdlib>

// ---------------------------------------------------------------------------
// Zoomed console layout (~60% text / ~20% Search / ~20% Suggestions).
// Fewer rows/cols than a dense 120x40 grid so glyphs look screen-zoom large.
// ---------------------------------------------------------------------------
const int SCREEN_COLS = 90;
const int SCREEN_ROWS = 24;

const int TEXT_LEFT = 1;
const int TEXT_TOP = 2;
const int TEXT_COLS = 54;   // ~60% of width
const int TEXT_ROWS = 13;   // ~55% of height

const int SEARCH_LEFT = 57;
const int SEARCH_TOP = 2;
const int SEARCH_COLS = 31; // ~20%+ right pane
const int SEARCH_ROWS = 13;

const int SUGGEST_TOP = 16;
const int SUGGEST_ROWS = 5; // ~20% bottom
const int STATUS_ROW = 22;

const int MAX_SUGGESTIONS = 8;
const int WORD_STACK_CAP = 5;
const int MAX_PATH_BUF = 260;
const int MAX_QUERY_BUF = 64;
const int MAX_WORD_BUF = 96;

// ---------- Document: 2-D linked grid (left/right/up/down ONLY) ----------
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

    // Insert letter/space; whole-word wrap; strips trailing spaces on lines.
    bool insertChar(char ch);
    bool insertNewline();
    bool backspace(char& removed);
    bool deleteForward(char& removed);

    // Word-level helpers for undo/redo stacks
    bool eraseWordAt(int r, int c, int len);
    bool insertWordAt(int r, int c, const char* word);

    int lineCount() const;
    int lineLength(int r) const;
    int wordCount() const;
    int charCount() const;

    char charAt(int r, int c) const;
    void copyLine(int r, char* out, int outCap) const;
    void copyWordAtCursor(char* out, int outCap) const;
    int copyAllText(char* out, int outCap) const;

    bool findNext(const char* query, int& outRow, int& outCol, bool wrap) const;

    bool saveToFile(const char* path) const;
    bool loadFromFile(const char* path); // false if missing (caller may create)

    // Collect unique document words matching prefix into a linked list of WordNode
    // (caller frees). Uses CharNode-backed WordNode defined below — see WordNode.
    void collectPrefixWords(const char* prefix, struct WordNode*& outHead, int maxCount,
                            bool allowExtendedTokens) const;

    // Selection / clipboard helpers (inclusive start, exclusive end; may span lines)
    int copyRange(int r0, int c0, int r1, int c1, char* out, int outCap) const;
    bool deleteRange(int r0, int c0, int r1, int c1);
    bool insertTextAtCursor(const char* text, bool allowExtended);

    void render(int highlightRow, int highlightCol, int highlightLen,
                int selR0, int selC0, int selR1, int selC1, bool selOn) const;

private:
    Node* head_;
    int row_;
    int col_;

    Node* rowHead(int r) const;
    Node* nodeAt(int r, int c) const;
    Node* ensureRow(int r);
    void destroy();
    void stripTrailingSpaces(int r);
    int wordStartCol() const;
    bool wrapCurrentWordToNextLine();
    int maxRows() const { return TEXT_ROWS; }
    int maxCols() const { return TEXT_COLS; }
};

// ---------- Char linked list (avoid std::string for words) ----------
struct CharNode {
    char ch;
    CharNode* next;
    explicit CharNode(char c = '\0') : ch(c), next(nullptr) {}
};

struct WordNode {
    CharNode* chars;
    WordNode* next;
    WordNode() : chars(nullptr), next(nullptr) {}
};

void freeCharList(CharNode*& head);
void freeWordList(WordNode*& head);
CharNode* dupCharList(const CharNode* src);
void charsFromCStr(CharNode*& head, const char* s);
void charsToBuf(const CharNode* head, char* buf, int cap);
int charsLen(const CharNode* head);
void appendChar(CharNode*& head, char c);

// ---------- Undo / Redo: STACK of WORDS (linked, capacity 5) ----------
struct WordAction {
    CharNode* word; // letters of the word (no space)
    int row;
    int col;        // start column of the word in the document
    bool inserted;  // true = word was typed/inserted; undo removes it
    WordAction* next;

    WordAction()
        : word(nullptr), row(0), col(0), inserted(true), next(nullptr) {}
};

class WordStack {
public:
    WordStack();
    ~WordStack();

    void clear();
    void push(CharNode* wordOwned, int row, int col, bool inserted);
    WordAction* pop(); // caller owns returned node (or null)
    bool empty() const { return top_ == nullptr; }
    int depth() const { return depth_; }

private:
    WordAction* top_;
    int depth_;
    void freeAction(WordAction* a);
    void dropOldest();
};

class WordHistory {
public:
    WordHistory();
    ~WordHistory();

    void clear();
    void recordInsert(CharNode* wordOwned, int row, int col);
    void recordDelete(CharNode* wordOwned, int row, int col);
    bool canUndo() const;
    bool canRedo() const;
    int undoDepth() const;
    int redoDepth() const;
    bool undo(Document& doc);
    bool redo(Document& doc);

private:
    WordStack undo_;
    WordStack redo_;
};

// ---------- Editor / UI ----------
class NotepadApp {
public:
    NotepadApp();
    ~NotepadApp();
    int run();

private:
    Document doc_;
    WordHistory history_;

    char filePath_[MAX_PATH_BUF];
    bool dirty_;
    bool running_;
    bool showHelp_;
    bool extendedMode_; // Ctrl+E: allow digits/punct (OFF = rubric alpha-only)

    char findQuery_[MAX_QUERY_BUF];
    char replaceQuery_[MAX_QUERY_BUF];
    int hlRow_;
    int hlCol_;
    int hlLen_;

    CharNode* pendingWord_; // letters typed since last Space/Enter/word boundary
    int pendingRow_;
    int pendingCol_;

    CharNode* clipboard_;

    // Text selection (anchor + cursor). When selOn_, selected range is between
    // (selAnchorRow_/Col_) and the live cursor.
    bool selOn_;
    int selAnchorRow_;
    int selAnchorCol_;

    // Cached word suggestions for 1-8 / Tab pick
    char suggestCache_[MAX_SUGGESTIONS][MAX_WORD_BUF];
    int suggestCount_;

    void maximizeConsole();
    void setupConsoleDisplay();
    void pinViewportTop() const;
    void writePaddedRow(int y, WORD attr, const char* text) const;
    void gotoxy(int x, int y) const;
    void clearScreen(bool resetScroll = true) const;
    void setColor(WORD attr) const;
    void drawChrome() const;
    void drawSearchPane() const;
    void drawStatus() const;
    void drawSuggestions();
    void refresh();

    bool showWelcomeScreen();
    void showMainMenu();
    bool promptFileName(const char* title, char* out, int outCap);
    bool confirmDiscard();
    void messageBoxInfo(const wchar_t* text, const wchar_t* caption) const;
    int messageBoxYesNo(const wchar_t* text, const wchar_t* caption) const;

    void actionNew();
    void actionLoad();
    void actionSave();
    void actionSaveAs();
    void actionFind();
    void actionFindNext();
    void actionReplace();
    void actionReplaceAll();
    void actionCopy();
    void actionCut();
    void actionPaste();
    void actionSelectAll();
    void actionHelp();
    void actionUndo();
    void actionRedo();
    void actionToggleExtended();
    void actionApplySuggestion(int index); // 0-based

    void handleKey(const KEY_EVENT_RECORD& key);
    void typeChar(char ch);
    void commitPendingWord();
    void doBackspace();
    void doDelete();
    void doEnter();

    void clearSelection();
    void ensureSelectionAnchor();
    void getSelectionBounds(int& r0, int& c0, int& r1, int& c1) const;
    bool hasSelection() const { return selOn_; }
    void deleteSelectionIfAny();

    void readPromptLine(const char* label, char* out, int outCap);
    bool isAllowedChar(char ch) const;
    bool isWordChar(char ch) const;
};

void setConsoleTitleBar(const wchar_t* title);

#endif
