#include "Header.h"

#include <cctype>
#include <cstdlib>

using namespace std;

static COORD makeCoord(SHORT x, SHORT y) {
    COORD c;
    c.X = x;
    c.Y = y;
    return c;
}

static WORD attrNormal() {
    return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
}

static WORD attrHighlight() {
    return BACKGROUND_RED | BACKGROUND_GREEN | FOREGROUND_INTENSITY;
}

void setConsoleTitleBar(const wchar_t* title) { SetConsoleTitleW(title); }

// =============================================================================
// Document — row spine (up/down on first cell) + horizontal char chains
// =============================================================================

Document::Document() : head_(nullptr), row_(0), col_(0) {}
Document::~Document() { destroy(); }

void Document::destroy() {
    Node* row = head_;
    while (row) {
        Node* nextRow = row->down;
        Node* cell = row;
        while (cell) {
            Node* next = cell->right;
            delete cell;
            cell = next;
        }
        row = nextRow;
    }
    head_ = nullptr;
    row_ = col_ = 0;
}

void Document::clear() { destroy(); }
bool Document::empty() const { return head_ == nullptr; }

Node* Document::rowHead(int r) const {
    if (!head_ || r < 0) return nullptr;
    Node* p = head_;
    for (int i = 0; i < r; ++i) {
        if (!p->down) return nullptr;
        p = p->down;
    }
    return p;
}

Node* Document::nodeAt(int r, int c) const {
    Node* p = rowHead(r);
    if (!p || c < 0) return nullptr;
    // Empty placeholder line
    if (p->data == '\0' && !p->right) return (c == 0) ? p : nullptr;
    for (int i = 0; i < c; ++i) {
        if (!p->right) return nullptr;
        p = p->right;
    }
    return p;
}

Node* Document::ensureRow(int r) {
    if (r < 0 || r >= maxRows()) return nullptr;
    if (!head_) {
        head_ = new Node('\0');
        if (r == 0) return head_;
    }
    Node* p = head_;
    for (int i = 0; i < r; ++i) {
        if (!p->down) {
            Node* nr = new Node('\0');
            nr->up = p;
            p->down = nr;
        }
        p = p->down;
    }
    return p;
}

int Document::lineCount() const {
    int n = 0;
    for (Node* p = head_; p; p = p->down) ++n;
    return n == 0 ? 1 : n;
}

int Document::lineLength(int r) const {
    Node* p = rowHead(r);
    if (!p) return 0;
    if (p->data == '\0' && !p->right) return 0;
    int len = 0;
    while (p) {
        ++len;
        p = p->right;
    }
    return len;
}

char Document::charAt(int r, int c) const {
    Node* n = nodeAt(r, c);
    if (!n || (n->data == '\0' && !n->right && c == 0 && lineLength(r) == 0)) return '\0';
    return n ? n->data : '\0';
}

void Document::setCursor(int r, int c) {
    if (!head_) ensureRow(0);
    if (r < 0) r = 0;
    int lines = lineCount();
    if (r >= lines) r = lines - 1;
    if (c < 0) c = 0;
    int len = lineLength(r);
    if (c > len) c = len;
    row_ = r;
    col_ = c;
}

void Document::moveLeft() {
    if (col_ > 0) --col_;
    else if (row_ > 0) {
        --row_;
        col_ = lineLength(row_);
    }
}

void Document::moveRight() {
    int len = lineLength(row_);
    if (col_ < len) ++col_;
    else if (row_ + 1 < lineCount()) {
        ++row_;
        col_ = 0;
    }
}

void Document::moveUp() {
    if (row_ > 0) {
        --row_;
        int len = lineLength(row_);
        if (col_ > len) col_ = len;
    }
}

void Document::moveDown() {
    if (row_ + 1 < lineCount()) {
        ++row_;
        int len = lineLength(row_);
        if (col_ > len) col_ = len;
    }
}

void Document::moveHome() { col_ = 0; }
void Document::moveEnd() { col_ = lineLength(row_); }
void Document::moveDocHome() {
    row_ = 0;
    col_ = 0;
}
void Document::moveDocEnd() {
    row_ = lineCount() - 1;
    if (row_ < 0) row_ = 0;
    col_ = lineLength(row_);
}

static void relinkRowHead(Node*& head, Node* oldHead, Node* newHead) {
    Node* above = oldHead->up;
    Node* below = oldHead->down;
    newHead->up = above;
    newHead->down = below;
    if (above) above->down = newHead;
    else head = newHead;
    if (below) below->up = newHead;
    oldHead->up = oldHead->down = nullptr;
}

bool Document::insertChar(char ch) {
    if (!head_) ensureRow(0);
    if (row_ >= maxRows()) return false;
    if (lineLength(row_) >= maxCols()) return false;

    Node* row = ensureRow(row_);
    if (!row) return false;

    int len = lineLength(row_);
    if (col_ > len) col_ = len;

    // Empty line placeholder
    if (len == 0) {
        row->data = ch;
        col_ = 1;
        return true;
    }

    if (col_ == 0) {
        Node* neu = new Node(ch);
        neu->right = row;
        row->left = neu;
        relinkRowHead(head_, row, neu);
        col_ = 1;
        return true;
    }

    Node* pred = nodeAt(row_, col_ - 1);
    if (!pred) return false;
    Node* neu = new Node(ch);
    neu->left = pred;
    neu->right = pred->right;
    if (pred->right) pred->right->left = neu;
    pred->right = neu;
    ++col_;
    return true;
}

bool Document::insertNewline() {
    if (!head_) ensureRow(0);
    if (lineCount() >= maxRows()) return false;

    Node* row = ensureRow(row_);
    if (!row) return false;

    Node* split = nullptr;
    if (col_ == 0) {
        if (lineLength(row_) == 0) {
            split = nullptr;
        } else {
            // Push entire content to next row; leave empty placeholder here
            split = row;
            Node* placeholder = new Node('\0');
            Node* above = row->up;
            Node* below = row->down;
            placeholder->up = above;
            placeholder->down = row;
            row->up = placeholder;
            if (above) above->down = placeholder;
            else head_ = placeholder;
            // row keeps below
            (void)below;
            ++row_;
            col_ = 0;
            return true;
        }
    } else {
        Node* pred = nodeAt(row_, col_ - 1);
        if (!pred) return false;
        split = pred->right;
        pred->right = nullptr;
        if (split) split->left = nullptr;
    }

    Node* newRow = split ? split : new Node('\0');
    if (split) {
        for (Node* t = split; t; t = t->right) t->up = t->down = nullptr;
    }

    Node* below = row->down;
    row->down = newRow;
    newRow->up = row;
    newRow->down = below;
    if (below) below->up = newRow;

    ++row_;
    col_ = 0;
    return true;
}

bool Document::backspace(char& removed) {
    removed = '\0';
    if (!head_) return false;
    if (row_ == 0 && col_ == 0) return false;

    if (col_ > 0) {
        Node* target = nodeAt(row_, col_ - 1);
        if (!target) return false;
        removed = target->data;
        Node* L = target->left;
        Node* R = target->right;

        if (!L && !R) {
            // Only char on line -> placeholder
            target->data = '\0';
            --col_;
            return true;
        }
        if (!L) {
            // Removing row head
            if (R) {
                R->left = nullptr;
                relinkRowHead(head_, target, R);
                delete target;
            }
        } else {
            L->right = R;
            if (R) R->left = L;
            delete target;
        }
        --col_;
        return true;
    }

    // Join current row onto previous
    Node* prev = rowHead(row_ - 1);
    Node* cur = rowHead(row_);
    if (!prev || !cur) return false;

    int prevLen = lineLength(row_ - 1);
    Node* below = cur->down;
    bool curEmpty = (cur->data == '\0' && !cur->right);

    prev->down = below;
    if (below) below->up = prev;

    if (!curEmpty) {
        if (prev->data == '\0' && !prev->right) {
            // Replace empty prev with cur chain
            Node* above = prev->up;
            cur->up = above;
            cur->down = below;
            if (above) above->down = cur;
            else head_ = cur;
            if (below) below->up = cur;
            delete prev;
        } else {
            Node* end = prev;
            while (end->right) end = end->right;
            end->right = cur;
            cur->left = end;
            cur->up = cur->down = nullptr;
        }
    } else {
        delete cur;
    }

    --row_;
    col_ = prevLen;
    removed = '\n';
    return true;
}

bool Document::deleteForward(char& removed) {
    removed = '\0';
    if (!head_) ensureRow(0);

    int len = lineLength(row_);
    if (col_ < len) {
        Node* target = nodeAt(row_, col_);
        if (!target) return false;
        removed = target->data;
        Node* L = target->left;
        Node* R = target->right;

        if (!L && !R) {
            target->data = '\0';
            return true;
        }
        if (!L) {
            if (R) {
                R->left = nullptr;
                relinkRowHead(head_, target, R);
                delete target;
            }
        } else {
            L->right = R;
            if (R) R->left = L;
            delete target;
        }
        return true;
    }

    // Join next line
    Node* cur = rowHead(row_);
    Node* nxt = rowHead(row_ + 1);
    if (!cur || !nxt) return false;

    bool nxtEmpty = (nxt->data == '\0' && !nxt->right);
    Node* below = nxt->down;
    cur->down = below;
    if (below) below->up = cur;

    if (!nxtEmpty) {
        if (cur->data == '\0' && !cur->right) {
            Node* above = cur->up;
            nxt->up = above;
            nxt->down = below;
            if (above) above->down = nxt;
            else head_ = nxt;
            if (below) below->up = nxt;
            delete cur;
        } else {
            Node* end = cur;
            while (end->right) end = end->right;
            end->right = nxt;
            nxt->left = end;
            nxt->up = nxt->down = nullptr;
        }
    } else {
        delete nxt;
    }
    removed = '\n';
    return true;
}

bool Document::insertAt(int r, int c, char ch) {
    setCursor(r, c);
    return insertChar(ch);
}

bool Document::eraseAt(int r, int c, char& removed) {
    setCursor(r, c + 1);
    return backspace(removed);
}

bool Document::insertNewlineAt(int r, int c) {
    setCursor(r, c);
    return insertNewline();
}

bool Document::joinLineWithPrevious(int r) {
    if (r <= 0) return false;
    setCursor(r, 0);
    char rm;
    return backspace(rm);
}

int Document::wordCount() const {
    int count = 0;
    bool inWord = false;
    if (!head_) return 0;
    for (Node* row = head_; row; row = row->down) {
        inWord = false;
        if (row->data == '\0' && !row->right) continue;
        for (Node* p = row; p; p = p->right) {
            if (isalnum(static_cast<unsigned char>(p->data))) {
                if (!inWord) {
                    ++count;
                    inWord = true;
                }
            } else {
                inWord = false;
            }
        }
    }
    return count;
}

int Document::charCount() const {
    int n = 0;
    if (!head_) return 0;
    for (Node* row = head_; row; row = row->down) {
        if (row->data == '\0' && !row->right) continue;
        for (Node* p = row; p; p = p->right) ++n;
    }
    return n;
}

string Document::lineText(int r) const {
    string s;
    Node* p = rowHead(r);
    if (!p) return s;
    if (p->data == '\0' && !p->right) return s;
    while (p) {
        s.push_back(p->data);
        p = p->right;
    }
    return s;
}

string Document::wordAtCursor() const {
    string line = lineText(row_);
    if (line.empty()) return string();
    int i = col_;
    if (i > static_cast<int>(line.size())) i = static_cast<int>(line.size());
    if (i > 0 && (i == static_cast<int>(line.size()) ||
                  !isalnum(static_cast<unsigned char>(line[static_cast<size_t>(i)]))))
        --i;
    if (i < 0 || i >= static_cast<int>(line.size()) ||
        !isalnum(static_cast<unsigned char>(line[static_cast<size_t>(i)])))
        return string();
    int a = i, b = i;
    while (a > 0 && isalnum(static_cast<unsigned char>(line[static_cast<size_t>(a - 1)]))) --a;
    while (b + 1 < static_cast<int>(line.size()) &&
           isalnum(static_cast<unsigned char>(line[static_cast<size_t>(b + 1)])))
        ++b;
    return line.substr(static_cast<size_t>(a), static_cast<size_t>(b - a + 1));
}

string Document::allText() const {
    string s;
    if (!head_) return s;
    for (Node* row = head_; row; row = row->down) {
        if (!(row->data == '\0' && !row->right)) {
            for (Node* p = row; p; p = p->right) s.push_back(p->data);
        }
        if (row->down) s.push_back('\n');
    }
    return s;
}

bool Document::findNext(const string& query, int& outRow, int& outCol, bool wrap) const {
    if (query.empty()) return false;
    const int lines = lineCount();

    auto scan = [&](int r0, size_t c0, int r1) -> bool {
        for (int r = r0; r < r1; ++r) {
            string line = lineText(r);
            size_t from = (r == r0) ? c0 : 0;
            if (from > line.size()) continue;
            size_t pos = line.find(query, from);
            if (pos != string::npos) {
                outRow = r;
                outCol = static_cast<int>(pos);
                return true;
            }
        }
        return false;
    };

    if (scan(row_, static_cast<size_t>(col_ + 1), lines)) return true;
    if (wrap && scan(0, 0, row_ + 1)) return true;
    return false;
}

bool Document::replaceAtCursor(const string& findStr, const string& replaceStr) {
    if (findStr.empty()) return false;
    string line = lineText(row_);
    if (col_ < 0 || col_ + static_cast<int>(findStr.size()) > static_cast<int>(line.size()))
        return false;
    if (line.compare(static_cast<size_t>(col_), findStr.size(), findStr) != 0) return false;
    for (size_t i = 0; i < findStr.size(); ++i) {
        char rm;
        if (!deleteForward(rm)) return false;
    }
    for (size_t i = 0; i < replaceStr.size(); ++i) {
        if (!insertChar(replaceStr[i])) return false;
    }
    return true;
}

bool Document::saveToFile(const string& path) const {
    ofstream out(path.c_str(), ios::out | ios::binary);
    if (!out) return false;
    out << allText();
    return static_cast<bool>(out);
}

bool Document::loadFromFile(const string& path) {
    ifstream in(path.c_str(), ios::in | ios::binary);
    if (!in) return false;
    clear();
    ensureRow(0);
    row_ = 0;
    col_ = 0;
    char ch;
    while (in.get(ch)) {
        if (ch == '\r') continue;
        if (ch == '\n') {
            if (!insertNewline()) break;
        } else {
            if (ch == '\t') ch = ' ';
            if (ch < 32 || ch > 126) continue;
            if (!insertChar(ch)) {
                if (!insertNewline()) break;
                if (!insertChar(ch)) break;
            }
        }
    }
    moveDocHome();
    return true;
}

void Document::render(int highlightRow, int highlightCol, int highlightLen) const {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    int r = 0;
    if (head_) {
        for (Node* row = head_; row; row = row->down, ++r) {
            SetConsoleCursorPosition(
                h, makeCoord(static_cast<SHORT>(PANE_LEFT), static_cast<SHORT>(PANE_TOP + r)));
            int c = 0;
            if (!(row->data == '\0' && !row->right)) {
                for (Node* p = row; p; p = p->right, ++c) {
                    bool hl = (highlightLen > 0 && r == highlightRow && c >= highlightCol &&
                               c < highlightCol + highlightLen);
                    if (hl) SetConsoleTextAttribute(h, attrHighlight());
                    cout << p->data;
                    if (hl) SetConsoleTextAttribute(h, attrNormal());
                }
            }
            for (int i = lineLength(r); i < maxCols(); ++i) cout << ' ';
        }
    }
    for (int rr = (head_ ? lineCount() : 0); rr < maxRows(); ++rr) {
        SetConsoleCursorPosition(
            h, makeCoord(static_cast<SHORT>(PANE_LEFT), static_cast<SHORT>(PANE_TOP + rr)));
        for (int i = 0; i < maxCols(); ++i) cout << ' ';
    }
}

// =============================================================================
// CommandHistory
// =============================================================================

CommandHistory::CommandHistory() : head_(nullptr), current_(nullptr) {}
CommandHistory::~CommandHistory() { clear(); }

void CommandHistory::destroyList(EditCommand* n) {
    while (n) {
        EditCommand* nx = n->next;
        delete n;
        n = nx;
    }
}

void CommandHistory::clear() {
    destroyList(head_);
    head_ = current_ = nullptr;
}

void CommandHistory::discardRedoBranch() {
    if (!current_) {
        destroyList(head_);
        head_ = nullptr;
        return;
    }
    EditCommand* doomed = current_->next;
    current_->next = nullptr;
    destroyList(doomed);
}

void CommandHistory::record(CmdKind kind, char ch, int row, int col) {
    discardRedoBranch();
    EditCommand* cmd = new EditCommand(kind, ch, row, col);
    if (!current_) {
        destroyList(head_);
        head_ = current_ = cmd;
        return;
    }
    current_->next = cmd;
    cmd->prev = current_;
    current_ = cmd;
}

bool CommandHistory::canUndo() const { return current_ != nullptr; }
bool CommandHistory::canRedo() const {
    return current_ ? current_->next != nullptr : head_ != nullptr;
}

bool CommandHistory::undo(Document& doc) {
    if (!current_) return false;
    EditCommand* cmd = current_;
    switch (cmd->kind) {
    case CmdKind::InsertChar: {
        char rm = '\0';
        doc.setCursor(cmd->row, cmd->col + 1);
        if (!doc.backspace(rm)) return false;
        break;
    }
    case CmdKind::DeleteChar: {
        doc.setCursor(cmd->row, cmd->col);
        if (cmd->ch == '\n') doc.insertNewline();
        else {
            doc.insertChar(cmd->ch);
            doc.setCursor(cmd->row, cmd->col);
        }
        break;
    }
    case CmdKind::InsertLine: {
        doc.setCursor(cmd->row + 1, 0);
        char rm;
        if (!doc.backspace(rm)) return false;
        break;
    }
    case CmdKind::JoinLine: {
        doc.setCursor(cmd->row, cmd->col);
        if (!doc.insertNewline()) return false;
        break;
    }
    }
    current_ = cmd->prev;
    return true;
}

bool CommandHistory::redo(Document& doc) {
    EditCommand* cmd = current_ ? current_->next : head_;
    if (!cmd) return false;
    switch (cmd->kind) {
    case CmdKind::InsertChar:
        doc.setCursor(cmd->row, cmd->col);
        if (!doc.insertChar(cmd->ch)) return false;
        break;
    case CmdKind::DeleteChar:
        if (cmd->ch == '\n') {
            doc.setCursor(cmd->row + 1, 0);
            char rm;
            if (!doc.backspace(rm)) return false;
        } else {
            doc.setCursor(cmd->row, cmd->col + 1);
            char rm;
            if (!doc.backspace(rm)) return false;
        }
        break;
    case CmdKind::InsertLine:
        doc.setCursor(cmd->row, cmd->col);
        if (!doc.insertNewline()) return false;
        break;
    case CmdKind::JoinLine: {
        doc.setCursor(cmd->row + 1, 0);
        char rm;
        if (!doc.backspace(rm)) return false;
        break;
    }
    }
    current_ = cmd;
    return true;
}

// =============================================================================
// Dictionary
// =============================================================================

Dictionary::Dictionary() : words_(nullptr) {
    const char* seed[] = {
        "the",          "and",         "notepad",     "document",    "linked",
        "list",         "insert",      "delete",      "undo",        "redo",
        "search",       "replace",     "save",        "load",        "file",
        "cursor",       "keyboard",    "windows",     "console",     "professional",
        "assignment",   "structure",   "node",        "character",   "suggestion",
        "feature",      "application", "editor",      "command",     "history",
        "navigation",   "clipboard",   "paste",       "copy",        "cut",
        "help",         "status",      "word",        "line",        "count",
        "find",         "next",        "home",        "end",         "enter",
        "space",        "punctuation", "rohaan",      "student",     "project",
        "visual",       "studio",      "compile",     "build",       "open",
        nullptr};
    for (int i = 0; seed[i]; ++i) add(seed[i]);
}

Dictionary::~Dictionary() {
    while (words_) {
        WordNode* n = words_->next;
        delete words_;
        words_ = n;
    }
}

void Dictionary::add(const char* w) {
    WordNode* n = new WordNode(w);
    n->next = words_;
    words_ = n;
}

void Dictionary::suggest(const string& prefix, WordNode*& outHead, int maxCount) const {
    outHead = nullptr;
    if (prefix.empty()) return;
    WordNode* tail = nullptr;
    int count = 0;
    string pref = prefix;
    for (size_t i = 0; i < pref.size(); ++i) pref[i] = static_cast<char>(tolower(static_cast<unsigned char>(pref[i])));

    for (WordNode* p = words_; p && count < maxCount; p = p->next) {
        string w = p->word;
        if (w.size() < pref.size()) continue;
        bool ok = true;
        for (size_t i = 0; i < pref.size(); ++i) {
            if (tolower(static_cast<unsigned char>(w[i])) != pref[i]) {
                ok = false;
                break;
            }
        }
        if (!ok) continue;
        WordNode* n = new WordNode(p->word);
        if (!outHead) outHead = tail = n;
        else {
            tail->next = n;
            tail = n;
        }
        ++count;
    }
}

// =============================================================================
// NotepadApp
// =============================================================================

NotepadApp::NotepadApp()
    : dirty_(false), running_(true), showHelp_(false), hlRow_(-1), hlCol_(-1), hlLen_(0) {}

void NotepadApp::maximizeConsole() {
    HWND w = GetConsoleWindow();
    if (w) ShowWindow(w, SW_MAXIMIZE);
}

void NotepadApp::gotoxy(int x, int y) const {
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE),
                             makeCoord(static_cast<SHORT>(x), static_cast<SHORT>(y)));
}

void NotepadApp::clearScreen() const { system("cls"); }

void NotepadApp::messageBoxInfo(const wchar_t* text, const wchar_t* caption) const {
    MessageBoxW(nullptr, text, caption, MB_OK | MB_ICONINFORMATION);
}

int NotepadApp::messageBoxYesNo(const wchar_t* text, const wchar_t* caption) const {
    return MessageBoxW(nullptr, text, caption, MB_YESNO | MB_ICONQUESTION);
}

void NotepadApp::drawChrome() const {
    gotoxy(0, 0);
    cout << "+-- Professional Notepad (2D Linked List) -------------------------------------------+\n";
    cout << "| Ctrl+N New | Ctrl+O Open | Ctrl+S Save | Ctrl+Z Undo | Ctrl+Y Redo | Ctrl+F Find  |\n";
    for (int y = PANE_TOP; y <= PANE_BOTTOM; ++y) {
        gotoxy(0, y);
        cout << '|';
        gotoxy(PANE_RIGHT + 1, y);
        cout << '|';
    }
    gotoxy(0, PANE_BOTTOM + 1);
    cout << '+';
    for (int i = 0; i < PANE_RIGHT; ++i) cout << '-';
    cout << '+';
}

void NotepadApp::drawStatus() const {
    gotoxy(0, STATUS_ROW);
    string name = filePath_.empty() ? string("(untitled)") : filePath_;
    cout << " Status | " << name << (dirty_ ? " *" : "  ") << " | Ln " << (doc_.cursorRow() + 1)
         << ", Col " << (doc_.cursorCol() + 1) << " | Words " << doc_.wordCount() << " | Chars "
         << doc_.charCount() << " | Undo:" << (history_.canUndo() ? 'Y' : 'N')
         << " Redo:" << (history_.canRedo() ? 'Y' : 'N') << " | F1 Help | Esc Menu     ";
}

void NotepadApp::drawSuggestions() const {
    gotoxy(0, SUGGEST_ROW);
    cout << " WORD SUGGESTIONS: ";
    string prefix = doc_.wordAtCursor();
    WordNode* list = nullptr;
    dictionary_.suggest(prefix, list, MAX_SUGGESTIONS);
    if (!list) cout << "(type letters to see matches)                                   ";
    else {
        int i = 0;
        for (WordNode* p = list; p; p = p->next, ++i) cout << "[" << (i + 1) << "] " << p->word << "  ";
        cout << "                 ";
    }
    while (list) {
        WordNode* n = list->next;
        delete list;
        list = n;
    }
    gotoxy(0, SUGGEST_ROW + 1);
    cout << " Keys: Arrows | Enter | Bksp/Del | Ctrl+H Replace | Ctrl+C/X/V | F3 Find Next | Home/End";
}

void NotepadApp::refresh() {
    drawChrome();
    doc_.render(hlRow_, hlCol_, hlLen_);
    drawStatus();
    drawSuggestions();
    if (showHelp_) {
        gotoxy(24, 8);
        cout << "+-------------- KEYMAP ---------------+";
        gotoxy(24, 9);
        cout << "| Ctrl+N New    Ctrl+O Open           |";
        gotoxy(24, 10);
        cout << "| Ctrl+S Save   Ctrl+Z/Y Undo/Redo    |";
        gotoxy(24, 11);
        cout << "| Ctrl+F Find   F3 Find Next          |";
        gotoxy(24, 12);
        cout << "| Ctrl+H Replace                      |";
        gotoxy(24, 13);
        cout << "| Ctrl+C/X/V Copy Cut Paste           |";
        gotoxy(24, 14);
        cout << "| F1 toggle help   Esc main menu      |";
        gotoxy(24, 15);
        cout << "+-------------------------------------+";
    }
    gotoxy(PANE_LEFT + doc_.cursorCol(), PANE_TOP + doc_.cursorRow());
}

bool NotepadApp::promptFileName(const char* title, string& out) {
    cout << "\n\t" << title << "\n\tName (adds .txt if missing): ";
    cout.flush();
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    string name;
    getline(cin, name);
    if (name.empty()) getline(cin, name);
    if (name.empty()) return false;
    if (name.size() < 4 || name.substr(name.size() - 4) != ".txt") name += ".txt";
    out = name;
    return true;
}

string NotepadApp::readPromptLine(const char* label) {
    cout << label;
    cout.flush();
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    string line;
    getline(cin, line);
    if (line.empty()) getline(cin, line);
    return line;
}

bool NotepadApp::confirmDiscard() {
    if (!dirty_) return true;
    return messageBoxYesNo(L"Document has unsaved changes. Discard them?", L"Unsaved Changes") ==
           IDYES;
}

void NotepadApp::actionNew() {
    if (!confirmDiscard()) return;
    doc_.clear();
    history_.clear();
    filePath_.clear();
    dirty_ = false;
    hlLen_ = 0;
    messageBoxInfo(L"New linked-list document ready.", L"New File");
}

void NotepadApp::actionOpen() {
    if (!confirmDiscard()) return;
    clearScreen();
    string path;
    if (!promptFileName("Open file", path)) return;
    if (!doc_.loadFromFile(path)) {
        // try samples/
        string alt = string("samples/") + path;
        if (!doc_.loadFromFile(alt)) {
            messageBoxInfo(L"Could not open file.", L"Open");
            return;
        }
        path = alt;
    }
    filePath_ = path;
    history_.clear();
    dirty_ = false;
    hlLen_ = 0;
    messageBoxInfo(L"File loaded into the 2D linked-list document.", L"Open");
}

void NotepadApp::actionSave() {
    if (filePath_.empty()) {
        actionSaveAs();
        return;
    }
    if (!doc_.saveToFile(filePath_)) {
        messageBoxInfo(L"Save failed.", L"Save");
        return;
    }
    dirty_ = false;
    messageBoxInfo(L"File saved successfully.", L"Save");
}

void NotepadApp::actionSaveAs() {
    clearScreen();
    string path;
    if (!promptFileName("Save As", path)) return;
    if (!doc_.saveToFile(path)) {
        messageBoxInfo(L"Save failed.", L"Save As");
        return;
    }
    filePath_ = path;
    dirty_ = false;
    messageBoxInfo(L"File saved successfully.", L"Save As");
}

void NotepadApp::actionFind() {
    clearScreen();
    cout << "\n\n\tFind\n";
    findQuery_ = readPromptLine("\tText to find: ");
    if (findQuery_.empty()) return;
    actionFindNext();
}

void NotepadApp::actionFindNext() {
    if (findQuery_.empty()) {
        actionFind();
        return;
    }
    int r = 0, c = 0;
    if (doc_.findNext(findQuery_, r, c, true)) {
        doc_.setCursor(r, c);
        hlRow_ = r;
        hlCol_ = c;
        hlLen_ = static_cast<int>(findQuery_.size());
    } else {
        hlLen_ = 0;
        messageBoxInfo(L"No matches found.", L"Find");
    }
}

void NotepadApp::actionReplace() {
    clearScreen();
    cout << "\n\n\tReplace\n";
    findQuery_ = readPromptLine("\tFind: ");
    replaceQuery_ = readPromptLine("\tReplace with: ");
    if (findQuery_.empty()) return;
    int r = 0, c = 0;
    if (!doc_.findNext(findQuery_, r, c, true)) {
        messageBoxInfo(L"No matches found.", L"Replace");
        return;
    }
    doc_.setCursor(r, c);
    for (size_t i = 0; i < findQuery_.size(); ++i) {
        char rm;
        int rr = doc_.cursorRow();
        int cc = doc_.cursorCol();
        if (doc_.deleteForward(rm)) history_.record(CmdKind::DeleteChar, rm, rr, cc);
    }
    for (size_t i = 0; i < replaceQuery_.size(); ++i) {
        int rr = doc_.cursorRow();
        int cc = doc_.cursorCol();
        char ch = replaceQuery_[i];
        if (doc_.insertChar(ch)) history_.record(CmdKind::InsertChar, ch, rr, cc);
    }
    dirty_ = true;
    hlRow_ = r;
    hlCol_ = c;
    hlLen_ = static_cast<int>(replaceQuery_.size());
}

void NotepadApp::actionCopy() {
    clipboard_ = doc_.wordAtCursor();
    if (clipboard_.empty()) clipboard_ = doc_.lineText(doc_.cursorRow());
}

void NotepadApp::actionCut() {
    actionCopy();
    string target = doc_.wordAtCursor();
    if (!target.empty()) {
        string line = doc_.lineText(doc_.cursorRow());
        size_t pos = line.find(target);
        if (pos != string::npos) {
            doc_.setCursor(doc_.cursorRow(), static_cast<int>(pos + target.size()));
            for (size_t i = 0; i < target.size(); ++i) doBackspace();
        }
    } else {
        int len = static_cast<int>(doc_.lineText(doc_.cursorRow()).size());
        doc_.setCursor(doc_.cursorRow(), len);
        for (int i = 0; i < len; ++i) doBackspace();
    }
}

void NotepadApp::actionPaste() {
    for (size_t i = 0; i < clipboard_.size(); ++i) typeChar(clipboard_[i]);
}

void NotepadApp::actionHelp() { showHelp_ = !showHelp_; }

void NotepadApp::actionUndo() {
    if (history_.undo(doc_)) dirty_ = true;
}

void NotepadApp::actionRedo() {
    if (history_.redo(doc_)) dirty_ = true;
}

void NotepadApp::typeChar(char ch) {
    int r = doc_.cursorRow();
    int c = doc_.cursorCol();
    if (!doc_.insertChar(ch)) {
        messageBoxInfo(L"Writing space is full for this line/page.", L"Space Full");
        return;
    }
    history_.record(CmdKind::InsertChar, ch, r, c);
    dirty_ = true;
    hlLen_ = 0;
}

void NotepadApp::doBackspace() {
    int r = doc_.cursorRow();
    int c = doc_.cursorCol();
    char rm = '\0';
    int joinCol = (c == 0 && r > 0) ? static_cast<int>(doc_.lineText(r - 1).size()) : 0;
    if (!doc_.backspace(rm)) return;
    if (rm == '\n') history_.record(CmdKind::JoinLine, '\n', r - 1, joinCol);
    else history_.record(CmdKind::DeleteChar, rm, r, c - 1);
    dirty_ = true;
    hlLen_ = 0;
}

void NotepadApp::doDelete() {
    int r = doc_.cursorRow();
    int c = doc_.cursorCol();
    char rm = '\0';
    if (!doc_.deleteForward(rm)) return;
    history_.record(CmdKind::DeleteChar, rm, r, c);
    dirty_ = true;
    hlLen_ = 0;
}

void NotepadApp::doEnter() {
    int r = doc_.cursorRow();
    int c = doc_.cursorCol();
    if (!doc_.insertNewline()) {
        messageBoxInfo(L"Maximum rows reached.", L"Space Full");
        return;
    }
    history_.record(CmdKind::InsertLine, '\n', r, c);
    dirty_ = true;
    hlLen_ = 0;
}

void NotepadApp::handleKey(const KEY_EVENT_RECORD& key) {
    const bool ctrl = (key.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
    const WORD vk = key.wVirtualKeyCode;
    const char ch = key.uChar.AsciiChar;

    if (ctrl) {
        switch (vk) {
        case 'N':
            actionNew();
            clearScreen();
            refresh();
            return;
        case 'O':
            actionOpen();
            clearScreen();
            refresh();
            return;
        case 'S':
            actionSave();
            clearScreen();
            refresh();
            return;
        case 'Z':
            actionUndo();
            return;
        case 'Y':
            actionRedo();
            return;
        case 'F':
            actionFind();
            clearScreen();
            refresh();
            return;
        case 'H':
            actionReplace();
            clearScreen();
            refresh();
            return;
        case 'C':
            actionCopy();
            return;
        case 'X':
            actionCut();
            return;
        case 'V':
            actionPaste();
            return;
        default:
            break;
        }
    }

    switch (vk) {
    case VK_ESCAPE:
        clearScreen();
        showMainMenu();
        if (running_) {
            clearScreen();
            refresh();
        }
        return;
    case VK_F1:
        actionHelp();
        return;
    case VK_F3:
        actionFindNext();
        return;
    case VK_LEFT:
        doc_.moveLeft();
        return;
    case VK_RIGHT:
        doc_.moveRight();
        return;
    case VK_UP:
        doc_.moveUp();
        return;
    case VK_DOWN:
        doc_.moveDown();
        return;
    case VK_HOME:
        if (ctrl) doc_.moveDocHome();
        else doc_.moveHome();
        return;
    case VK_END:
        if (ctrl) doc_.moveDocEnd();
        else doc_.moveEnd();
        return;
    case VK_RETURN:
        doEnter();
        return;
    case VK_BACK:
        doBackspace();
        return;
    case VK_DELETE:
        doDelete();
        return;
    default:
        break;
    }

    if (ch >= 32 && ch <= 126) typeChar(ch);
}

void NotepadApp::showMainMenu() {
    while (running_) {
        clearScreen();
        cout << "\n\n\n";
        cout << "\t\t========================================================\n";
        cout << "\t\t   PROFESSIONAL NOTEPAD  --  MAIN MENU\n";
        cout << "\t\t   Document = 2D linked list | Undo/Redo = command list\n";
        cout << "\t\t========================================================\n\n";
        cout << "\t\t  1. New File\n";
        cout << "\t\t  2. Open File\n";
        cout << "\t\t  3. Save File\n";
        cout << "\t\t  4. Save As\n";
        cout << "\t\t  5. Continue Editing\n";
        cout << "\t\t  6. Help / Keymap\n";
        cout << "\t\t  7. Exit\n\n";
        cout << "\t\t  Choice: ";
        int choice = 0;
        if (!(cin >> choice)) {
            cin.clear();
            string junk;
            getline(cin, junk);
            continue;
        }
        string junk;
        getline(cin, junk);

        switch (choice) {
        case 1:
            actionNew();
            return;
        case 2:
            actionOpen();
            return;
        case 3:
            actionSave();
            return;
        case 4:
            actionSaveAs();
            return;
        case 5:
            return;
        case 6:
            messageBoxInfo(L"Ctrl+N New | Ctrl+O Open | Ctrl+S Save | Ctrl+Z Undo | Ctrl+Y Redo\n"
                           L"Ctrl+F Find | F3 Find Next | Ctrl+H Replace\n"
                           L"Ctrl+C/X/V Copy Cut Paste | F1 Help | Esc Menu",
                           L"Keymap");
            break;
        case 7:
            if (dirty_) {
                if (messageBoxYesNo(L"Save before exiting?", L"Confirm Exit") == IDYES) actionSave();
            }
            running_ = false;
            return;
        default:
            messageBoxInfo(L"Invalid choice.", L"Menu");
            break;
        }
    }
}

int NotepadApp::run() {
    setConsoleTitleBar(L"Professional Notepad -- 2D Linked List Editor");
    maximizeConsole();
    // Start with empty document spine so editing works immediately after menu
    doc_.clear();

    showMainMenu();
    if (!running_) return 0;

    clearScreen();
    refresh();

    HANDLE rhnd = GetStdHandle(STD_INPUT_HANDLE);
    DWORD events = 0;
    DWORD readCount = 0;

    while (running_) {
        GetNumberOfConsoleInputEvents(rhnd, &events);
        if (events == 0) {
            Sleep(10);
            continue;
        }
        INPUT_RECORD buffer[64];
        if (!ReadConsoleInput(rhnd, buffer, 64, &readCount)) continue;
        for (DWORD i = 0; i < readCount; ++i) {
            if (buffer[i].EventType == KEY_EVENT && buffer[i].Event.KeyEvent.bKeyDown) {
                handleKey(buffer[i].Event.KeyEvent);
                if (!running_) break;
                refresh();
            }
        }
    }
    return 0;
}

int main() {
    NotepadApp app;
    return app.run();
}
