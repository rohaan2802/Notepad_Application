#include "Header.h"

using namespace std;

static COORD makeCoord(SHORT x, SHORT y) {
    COORD c;
    c.X = x;
    c.Y = y;
    return c;
}

static WORD attrNormal() {
    /* Bright white on black — high contrast */
    return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
}
static WORD attrDim() {
    return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
}
static WORD attrTitle() {
    /* Bright cyan titles */
    return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
}
static WORD attrHighlight() {
    /* Black text on bright yellow */
    return BACKGROUND_RED | BACKGROUND_GREEN | FOREGROUND_INTENSITY;
}
static WORD attrSearch() {
    /* Bright green search pane */
    return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
}
static WORD attrSuggest() {
    /* Bright yellow suggestions */
    return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
}
static WORD attrStatus() {
    /* White on blue status bar */
    return BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
           FOREGROUND_INTENSITY;
}
static WORD attrWarn() {
    return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
}
static WORD attrError() {
    return FOREGROUND_RED | FOREGROUND_INTENSITY;
}
static WORD attrOk() {
    return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
}
static WORD attrPrompt() {
    return FOREGROUND_BLUE | FOREGROUND_INTENSITY;
}

void setConsoleTitleBar(const wchar_t* title) { SetConsoleTitleW(title); }

static bool isCancelChoice(const char* s) {
    if (!s || !s[0]) return false;
    if (s[0] == '0' && s[1] == '\0') return true;
    return false;
}

// =============================================================================
// Char / Word list helpers
// =============================================================================

void freeCharList(CharNode*& head) {
    while (head) {
        CharNode* n = head->next;
        delete head;
        head = n;
    }
    head = nullptr;
}

void freeWordList(WordNode*& head) {
    while (head) {
        WordNode* n = head->next;
        freeCharList(head->chars);
        delete head;
        head = n;
    }
    head = nullptr;
}

CharNode* dupCharList(const CharNode* src) {
    CharNode* head = nullptr;
    CharNode* tail = nullptr;
    for (const CharNode* p = src; p; p = p->next) {
        CharNode* n = new CharNode(p->ch);
        if (!head) head = tail = n;
        else {
            tail->next = n;
            tail = n;
        }
    }
    return head;
}

void charsFromCStr(CharNode*& head, const char* s) {
    freeCharList(head);
    if (!s) return;
    CharNode* tail = nullptr;
    for (int i = 0; s[i]; ++i) {
        CharNode* n = new CharNode(s[i]);
        if (!head) head = tail = n;
        else {
            tail->next = n;
            tail = n;
        }
    }
}

void charsToBuf(const CharNode* head, char* buf, int cap) {
    if (!buf || cap <= 0) return;
    int i = 0;
    for (const CharNode* p = head; p && i + 1 < cap; p = p->next) buf[i++] = p->ch;
    buf[i] = '\0';
}

int charsLen(const CharNode* head) {
    int n = 0;
    for (const CharNode* p = head; p; p = p->next) ++n;
    return n;
}

void appendChar(CharNode*& head, char c) {
    CharNode* n = new CharNode(c);
    if (!head) {
        head = n;
        return;
    }
    CharNode* t = head;
    while (t->next) t = t->next;
    t->next = n;
}

static bool charsEqualCI(const CharNode* a, const char* b) {
    if (!b) return a == nullptr;
    int i = 0;
    for (; a && b[i]; a = a->next, ++i) {
        if (tolower(static_cast<unsigned char>(a->ch)) !=
            tolower(static_cast<unsigned char>(b[i])))
            return false;
    }
    return a == nullptr && b[i] == '\0';
}

static bool prefixMatchCI(const char* word, const char* prefix) {
    if (!prefix || !prefix[0]) return false;
    for (int i = 0; prefix[i]; ++i) {
        if (!word[i]) return false;
        if (tolower(static_cast<unsigned char>(word[i])) !=
            tolower(static_cast<unsigned char>(prefix[i])))
            return false;
    }
    return true;
}

static bool wordInList(WordNode* head, const char* w) {
    for (WordNode* p = head; p; p = p->next) {
        char buf[MAX_WORD_BUF];
        charsToBuf(p->chars, buf, MAX_WORD_BUF);
        if (_stricmp(buf, w) == 0) return true;
    }
    return false;
}

// =============================================================================
// Document
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
    if (!n) return '\0';
    if (n->data == '\0' && !n->right && c == 0 && lineLength(r) == 0) return '\0';
    return n->data;
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

void Document::stripTrailingSpaces(int r) {
    Node* p = rowHead(r);
    if (!p) return;
    if (p->data == '\0' && !p->right) return;

    // Find last non-space
    Node* lastKeep = nullptr;
    for (Node* t = p; t; t = t->right) {
        if (t->data != ' ') lastKeep = t;
    }
    if (!lastKeep) {
        // entire line was spaces -> empty placeholder
        Node* cell = p->right;
        while (cell) {
            Node* nx = cell->right;
            delete cell;
            cell = nx;
        }
        p->right = nullptr;
        p->data = '\0';
        if (row_ == r && col_ > 0) col_ = 0;
        return;
    }
    // Delete nodes after lastKeep
    Node* doomed = lastKeep->right;
    lastKeep->right = nullptr;
    while (doomed) {
        Node* nx = doomed->right;
        delete doomed;
        doomed = nx;
    }
    // If row head itself was trailing space chain start handled above
    if (row_ == r) {
        int len = lineLength(r);
        if (col_ > len) col_ = len;
    }
}

int Document::wordStartCol() const {
    int c = col_;
    while (c > 0) {
        char ch = charAt(row_, c - 1);
        if (ch == ' ' || ch == '\0') break;
        --c;
    }
    return c;
}

bool Document::wrapCurrentWordToNextLine() {
    int start = wordStartCol();
    int len = lineLength(row_);
    if (start >= len) return false;
    if (lineCount() >= maxRows() && row_ + 1 >= maxRows()) return false;

    // Detach word (and anything after it — should only be the word at EOL)
    Node* row = rowHead(row_);
    if (!row) return false;

    Node* split = nullptr;
    if (start == 0) {
        split = row;
        // leave empty placeholder on this row
        Node* placeholder = new Node('\0');
        Node* above = row->up;
        placeholder->up = above;
        placeholder->down = row->down;
        if (above) above->down = placeholder;
        else head_ = placeholder;
        if (placeholder->down) placeholder->down->up = placeholder;
        split->up = split->down = nullptr;
        // Now insert split as new row below placeholder
        Node* below = placeholder->down;
        placeholder->down = split;
        split->up = placeholder;
        split->down = below;
        if (below) below->up = split;
        for (Node* t = split; t; t = t->right) t->up = t->down = nullptr;
        split->up = placeholder;
        split->down = below;
        ++row_;
        col_ = lineLength(row_);
        stripTrailingSpaces(row_ - 1);
        return true;
    }

    Node* pred = nodeAt(row_, start - 1);
    if (!pred) return false;
    split = pred->right;
    pred->right = nullptr;
    if (split) split->left = nullptr;

    // Clear up/down on horizontal chain
    for (Node* t = split; t; t = t->right) t->up = t->down = nullptr;

    Node* newRow = split;
    Node* below = row->down;
    row->down = newRow;
    newRow->up = row;
    newRow->down = below;
    if (below) below->up = newRow;

    stripTrailingSpaces(row_);
    ++row_;
    col_ = lineLength(row_);
    return true;
}

bool Document::insertChar(char ch) {
    if (!head_) ensureRow(0);
    if (row_ >= maxRows()) return false;

    // Space: word separator only (not at BOL, not doubled). Trailing spaces are
    // stripped on Enter / wrap / save so stored lines end with a letter or '\n'.
    if (ch == ' ') {
        if (col_ == 0) return false;
        char prev = charAt(row_, col_ - 1);
        if (prev == ' ' || prev == '\0') return false;
    }

    // Whole-word wrap: if adding a letter would overflow, move current word down first
    if (ch != ' ') {
        int start = wordStartCol();
        int curWordLen = col_ - start; // letters already in word before insert
        if (start + curWordLen + 1 > maxCols()) {
            if (!wrapCurrentWordToNextLine()) return false;
        } else if (lineLength(row_) >= maxCols()) {
            // Line full but cursor mid-line with room in word sense — still block overflow
            if (col_ >= maxCols()) {
                if (!wrapCurrentWordToNextLine()) return false;
            } else if (col_ == lineLength(row_) && start + curWordLen + 1 > maxCols()) {
                if (!wrapCurrentWordToNextLine()) return false;
            } else if (lineLength(row_) >= maxCols() && col_ == lineLength(row_)) {
                if (!wrapCurrentWordToNextLine()) return false;
            }
        }
    }

    if (lineLength(row_) >= maxCols() && ch != ' ') {
        // After wrap attempt still full at insert point
        if (col_ >= maxCols()) return false;
        if (col_ == lineLength(row_) && lineLength(row_) >= maxCols()) {
            if (!wrapCurrentWordToNextLine()) return false;
        }
    }

    Node* row = ensureRow(row_);
    if (!row) return false;

    int len = lineLength(row_);
    if (col_ > len) col_ = len;
    if (len >= maxCols() && col_ == len) return false;

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
    // Mid-line insert: shift right via linked list (no overwrite)
    if (len >= maxCols()) return false;
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

    stripTrailingSpaces(row_);

    Node* split = nullptr;
    if (col_ == 0) {
        if (lineLength(row_) == 0) {
            split = nullptr;
        } else {
            split = row;
            Node* placeholder = new Node('\0');
            Node* above = row->up;
            placeholder->up = above;
            placeholder->down = row;
            row->up = placeholder;
            if (above) above->down = placeholder;
            else head_ = placeholder;
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
        stripTrailingSpaces(row_);
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
            target->data = '\0';
            --col_;
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
        --col_;
        return true;
    }

    // Join current row onto previous
    Node* prev = rowHead(row_ - 1);
    Node* cur = rowHead(row_);
    if (!prev || !cur) return false;

    stripTrailingSpaces(row_ - 1);
    int prevLen = lineLength(row_ - 1);
    // Joining must fit in maxCols
    int curLen = lineLength(row_);
    if (prevLen + curLen > maxCols()) return false;

    Node* below = cur->down;
    bool curEmpty = (cur->data == '\0' && !cur->right);

    prev->down = below;
    if (below) below->up = prev;

    if (!curEmpty) {
        if (prev->data == '\0' && !prev->right) {
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

    Node* cur = rowHead(row_);
    Node* nxt = rowHead(row_ + 1);
    if (!cur || !nxt) return false;

    int curLen = lineLength(row_);
    int nxtLen = lineLength(row_ + 1);
    if (curLen + nxtLen > maxCols()) return false;

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

bool Document::eraseWordAt(int r, int c, int len) {
    if (len <= 0) return false;
    setCursor(r, c + len);
    for (int i = 0; i < len; ++i) {
        char rm;
        if (!backspace(rm)) return false;
    }
    return true;
}

bool Document::insertWordAt(int r, int c, const char* word) {
    if (!word) return false;
    setCursor(r, c);
    for (int i = 0; word[i]; ++i) {
        if (!insertChar(word[i])) return false;
    }
    return true;
}

int Document::wordCount() const {
    int count = 0;
    bool inWord = false;
    if (!head_) return 0;
    for (Node* row = head_; row; row = row->down) {
        inWord = false;
        if (row->data == '\0' && !row->right) continue;
        for (Node* p = row; p; p = p->right) {
            if (isalpha(static_cast<unsigned char>(p->data))) {
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

void Document::copyLine(int r, char* out, int outCap) const {
    if (!out || outCap <= 0) return;
    out[0] = '\0';
    Node* p = rowHead(r);
    if (!p) return;
    if (p->data == '\0' && !p->right) return;
    int i = 0;
    while (p && i + 1 < outCap) {
        out[i++] = p->data;
        p = p->right;
    }
    out[i] = '\0';
}

void Document::copyWordAtCursor(char* out, int outCap) const {
    if (!out || outCap <= 0) return;
    out[0] = '\0';
    char line[512];
    copyLine(row_, line, 512);
    int len = static_cast<int>(strlen(line));
    if (len == 0) return;
    int i = col_;
    if (i > len) i = len;
    if (i > 0 && (i == len || !isalnum(static_cast<unsigned char>(line[i])))) --i;
    if (i < 0 || i >= len || !isalnum(static_cast<unsigned char>(line[i]))) return;
    int a = i, b = i;
    while (a > 0 && isalnum(static_cast<unsigned char>(line[a - 1]))) --a;
    while (b + 1 < len && isalnum(static_cast<unsigned char>(line[b + 1]))) ++b;
    int n = b - a + 1;
    if (n >= outCap) n = outCap - 1;
    for (int k = 0; k < n; ++k) out[k] = line[a + k];
    out[n] = '\0';
}

int Document::copyAllText(char* out, int outCap) const {
    if (!out || outCap <= 0) return 0;
    int i = 0;
    if (!head_) {
        out[0] = '\0';
        return 0;
    }
    for (Node* row = head_; row; row = row->down) {
        if (!(row->data == '\0' && !row->right)) {
            for (Node* p = row; p; p = p->right) {
                if (i + 1 >= outCap) {
                    out[i] = '\0';
                    return i;
                }
                out[i++] = p->data;
            }
        }
        if (row->down) {
            if (i + 1 >= outCap) {
                out[i] = '\0';
                return i;
            }
            out[i++] = '\n';
        }
    }
    out[i] = '\0';
    return i;
}

bool Document::findNext(const char* query, int& outRow, int& outCol, bool wrap) const {
    if (!query || !query[0]) return false;
    const int qlen = static_cast<int>(strlen(query));
    const int lines = lineCount();

    auto scan = [&](int r0, int c0, int r1) -> bool {
        for (int r = r0; r < r1; ++r) {
            char line[512];
            copyLine(r, line, 512);
            int len = static_cast<int>(strlen(line));
            int from = (r == r0) ? c0 : 0;
            for (int c = from; c + qlen <= len; ++c) {
                bool ok = true;
                for (int k = 0; k < qlen; ++k) {
                    if (line[c + k] != query[k]) {
                        ok = false;
                        break;
                    }
                }
                if (ok) {
                    outRow = r;
                    outCol = c;
                    return true;
                }
            }
        }
        return false;
    };

    if (scan(row_, col_ + 1, lines)) return true;
    if (wrap && scan(0, 0, row_ + 1)) return true;
    return false;
}

bool Document::saveToFile(const char* path) const {
    // Strip trailing spaces on every line before writing (rubric /5)
    Document* self = const_cast<Document*>(this);
    const int lines = lineCount();
    for (int r = 0; r < lines; ++r) self->stripTrailingSpaces(r);

    ofstream out(path, ios::out | ios::binary);
    if (!out) return false;
    char buf[65536];
    copyAllText(buf, 65536);
    out << buf;
    return static_cast<bool>(out);
}

bool Document::loadFromFile(const char* path) {
    ifstream in(path, ios::in | ios::binary);
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
            // Load filter: letters + spaces (digits/punct skipped unless already in file
            // as spaces/letters). Extended content in samples uses letters primarily.
            if (ch == ' ') {
                if (col_ > 0 && isalpha(static_cast<unsigned char>(charAt(row_, col_ - 1)))) {
                    insertChar(' ');
                }
            } else if (isalpha(static_cast<unsigned char>(ch))) {
                if (!insertChar(ch)) {
                    if (!insertNewline()) break;
                    if (!insertChar(ch)) break;
                }
            }
            // digits/punct ignored on load (alpha-only document)
        }
    }
    // Strip all trailing spaces
    for (int r = 0; r < lineCount(); ++r) stripTrailingSpaces(r);
    moveDocHome();
    return true;
}

void Document::collectPrefixWords(const char* prefix, WordNode*& outHead, int maxCount) const {
    outHead = nullptr;
    if (!prefix || !prefix[0] || maxCount <= 0) return;
    int count = 0;
    if (!head_) return;

    for (Node* row = head_; row && count < maxCount; row = row->down) {
        if (row->data == '\0' && !row->right) continue;
        char word[MAX_WORD_BUF];
        int wi = 0;
        for (Node* p = row; p; p = p->right) {
            if (isalpha(static_cast<unsigned char>(p->data))) {
                if (wi + 1 < MAX_WORD_BUF) word[wi++] = p->data;
            } else {
                if (wi > 0) {
                    word[wi] = '\0';
                    if (prefixMatchCI(word, prefix) && !wordInList(outHead, word)) {
                        WordNode* n = new WordNode();
                        charsFromCStr(n->chars, word);
                        n->next = outHead;
                        outHead = n;
                        ++count;
                        if (count >= maxCount) return;
                    }
                    wi = 0;
                }
            }
        }
        if (wi > 0) {
            word[wi] = '\0';
            if (prefixMatchCI(word, prefix) && !wordInList(outHead, word)) {
                WordNode* n = new WordNode();
                charsFromCStr(n->chars, word);
                n->next = outHead;
                outHead = n;
                ++count;
            }
        }
    }
}

void Document::render(int highlightRow, int highlightCol, int highlightLen) const {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    int r = 0;
    if (head_) {
        for (Node* row = head_; row; row = row->down, ++r) {
            SetConsoleCursorPosition(
                h, makeCoord(static_cast<SHORT>(TEXT_LEFT), static_cast<SHORT>(TEXT_TOP + r)));
            int c = 0;
            if (!(row->data == '\0' && !row->right)) {
                for (Node* p = row; p; p = p->right, ++c) {
                    bool hl = (highlightLen > 0 && r == highlightRow && c >= highlightCol &&
                               c < highlightCol + highlightLen);
                    SetConsoleTextAttribute(h, hl ? attrHighlight() : attrNormal());
                    cout << p->data;
                    if (hl) SetConsoleTextAttribute(h, attrNormal());
                }
            }
            for (int i = lineLength(r); i < maxCols(); ++i) cout << ' ';
        }
    }
    for (int rr = (head_ ? lineCount() : 0); rr < maxRows(); ++rr) {
        SetConsoleCursorPosition(
            h, makeCoord(static_cast<SHORT>(TEXT_LEFT), static_cast<SHORT>(TEXT_TOP + rr)));
        for (int i = 0; i < maxCols(); ++i) cout << ' ';
    }
    SetConsoleTextAttribute(h, attrNormal());
}

// =============================================================================
// WordStack / WordHistory
// =============================================================================

WordStack::WordStack() : top_(nullptr), depth_(0) {}
WordStack::~WordStack() { clear(); }

void WordStack::freeAction(WordAction* a) {
    if (!a) return;
    freeCharList(a->word);
    delete a;
}

void WordStack::clear() {
    while (top_) {
        WordAction* n = top_->next;
        freeAction(top_);
        top_ = n;
    }
    depth_ = 0;
}

void WordStack::dropOldest() {
    if (!top_) return;
    if (!top_->next) {
        freeAction(top_);
        top_ = nullptr;
        depth_ = 0;
        return;
    }
    WordAction* prev = nullptr;
    WordAction* cur = top_;
    while (cur->next) {
        prev = cur;
        cur = cur->next;
    }
    prev->next = nullptr;
    freeAction(cur);
    --depth_;
}

void WordStack::push(CharNode* wordOwned, int row, int col, bool inserted) {
    if (!wordOwned) return;
    while (depth_ >= WORD_STACK_CAP) dropOldest();
    WordAction* a = new WordAction();
    a->word = wordOwned;
    a->row = row;
    a->col = col;
    a->inserted = inserted;
    a->next = top_;
    top_ = a;
    ++depth_;
}

WordAction* WordStack::pop() {
    if (!top_) return nullptr;
    WordAction* a = top_;
    top_ = top_->next;
    a->next = nullptr;
    --depth_;
    return a;
}

WordHistory::WordHistory() {}
WordHistory::~WordHistory() { clear(); }

void WordHistory::clear() {
    undo_.clear();
    redo_.clear();
}

void WordHistory::recordInsert(CharNode* wordOwned, int row, int col) {
    redo_.clear();
    undo_.push(wordOwned, row, col, true);
}

void WordHistory::recordDelete(CharNode* wordOwned, int row, int col) {
    redo_.clear();
    undo_.push(wordOwned, row, col, false);
}

bool WordHistory::canUndo() const { return !undo_.empty(); }
bool WordHistory::canRedo() const { return !redo_.empty(); }
int WordHistory::undoDepth() const { return undo_.depth(); }
int WordHistory::redoDepth() const { return redo_.depth(); }

bool WordHistory::undo(Document& doc) {
    WordAction* a = undo_.pop();
    if (!a) return false;
    char buf[MAX_WORD_BUF];
    charsToBuf(a->word, buf, MAX_WORD_BUF);
    int len = charsLen(a->word);
    bool ok = false;
    if (a->inserted) {
        ok = doc.eraseWordAt(a->row, a->col, len);
        if (ok) {
            // Move to redo as "deleted" so redo re-inserts
            redo_.push(dupCharList(a->word), a->row, a->col, true);
        }
    } else {
        ok = doc.insertWordAt(a->row, a->col, buf);
        if (ok) redo_.push(dupCharList(a->word), a->row, a->col, false);
    }
    freeCharList(a->word);
    delete a;
    return ok;
}

bool WordHistory::redo(Document& doc) {
    WordAction* a = redo_.pop();
    if (!a) return false;
    char buf[MAX_WORD_BUF];
    charsToBuf(a->word, buf, MAX_WORD_BUF);
    int len = charsLen(a->word);
    bool ok = false;
    if (a->inserted) {
        ok = doc.insertWordAt(a->row, a->col, buf);
        if (ok) undo_.push(dupCharList(a->word), a->row, a->col, true);
    } else {
        ok = doc.eraseWordAt(a->row, a->col, len);
        if (ok) undo_.push(dupCharList(a->word), a->row, a->col, false);
    }
    freeCharList(a->word);
    delete a;
    return ok;
}

// =============================================================================
// NotepadApp
// =============================================================================

NotepadApp::NotepadApp()
    : dirty_(false),
      running_(true),
      showHelp_(false),
      extendedMode_(false),
      hlRow_(-1),
      hlCol_(-1),
      hlLen_(0),
      pendingWord_(nullptr),
      pendingRow_(0),
      pendingCol_(0),
      clipboard_(nullptr) {
    filePath_[0] = '\0';
    findQuery_[0] = '\0';
    replaceQuery_[0] = '\0';
}

NotepadApp::~NotepadApp() {
    freeCharList(pendingWord_);
    freeCharList(clipboard_);
    // doc_ and history_ destructors free their nodes
}

void NotepadApp::maximizeConsole() {
    HWND w = GetConsoleWindow();
    if (!w)
        return;
    ShowWindow(w, SW_RESTORE);
    ShowWindow(w, SW_SHOW);
    ShowWindow(w, SW_MAXIMIZE);
    SetForegroundWindow(w);
}

void NotepadApp::pinViewportTop() const {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi))
        return;
    SHORT lastCol = csbi.dwSize.X > 0 ? static_cast<SHORT>(csbi.dwSize.X - 1) : 0;
    SHORT lastRow = csbi.dwSize.Y > 0 ? static_cast<SHORT>(csbi.dwSize.Y - 1) : 0;
    SMALL_RECT vis = { 0, 0, lastCol, lastRow };
    SetConsoleWindowInfo(hOut, TRUE, &vis);
    SetConsoleCursorPosition(hOut, makeCoord(0, 0));
}

void NotepadApp::writePaddedRow(int y, WORD attr, const char* text) const {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    SHORT width = static_cast<SHORT>(SCREEN_COLS);
    if (GetConsoleScreenBufferInfo(hOut, &csbi) && csbi.dwSize.X > 0)
        width = csbi.dwSize.X;

    setColor(attr);
    gotoxy(0, y);
    int n = text ? static_cast<int>(strlen(text)) : 0;
    if (n > width)
        n = width;
    if (n > 0)
        cout.write(text, n);
    for (int i = n; i < width; ++i)
        cout.put(' ');
    cout.flush();
}

void NotepadApp::setupConsoleDisplay() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE || hOut == NULL)
        return;

    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
        mode |= ENABLE_PROCESSED_OUTPUT;
        mode &= ~ENABLE_WRAP_AT_EOL_OUTPUT;
        SetConsoleMode(hOut, mode);
    }

    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD inMode = 0;
    if (GetConsoleMode(hIn, &inMode)) {
        inMode |= ENABLE_EXTENDED_FLAGS;
        inMode &= ~ENABLE_QUICK_EDIT_MODE;
        SetConsoleMode(hIn, inMode);
    }

    RECT wa;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
    int pxW = wa.right - wa.left;
    int pxH = wa.bottom - wa.top - GetSystemMetrics(SM_CYCAPTION) - 16;
    if (pxW < 640) pxW = 640;
    if (pxH < 400) pxH = 400;

    SHORT fontY = static_cast<SHORT>(pxH / SCREEN_ROWS);
    // Prefer a larger readable font; still scale with window height.
    if (fontY > 36) fontY = 36;
    if (fontY < 22) fontY = 22;

    CONSOLE_FONT_INFOEX cfi;
    ZeroMemory(&cfi, sizeof(cfi));
    cfi.cbSize = sizeof(cfi);
    GetCurrentConsoleFontEx(hOut, FALSE, &cfi);
    cfi.dwFontSize.X = 0;
    cfi.dwFontSize.Y = fontY;
    wcscpy_s(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(hOut, FALSE, &cfi);

    SMALL_RECT tiny = { 0, 0, 1, 1 };
    SetConsoleWindowInfo(hOut, TRUE, &tiny);

    COORD starter = { static_cast<SHORT>(SCREEN_COLS), static_cast<SHORT>(SCREEN_ROWS) };
    SetConsoleScreenBufferSize(hOut, starter);
    SMALL_RECT starterWin = { 0, 0, static_cast<SHORT>(SCREEN_COLS - 1),
                              static_cast<SHORT>(SCREEN_ROWS - 1) };
    SetConsoleWindowInfo(hOut, TRUE, &starterWin);

    maximizeConsole();
    Sleep(40);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    SHORT cols = static_cast<SHORT>(SCREEN_COLS);
    SHORT rows = static_cast<SHORT>(SCREEN_ROWS);
    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        SHORT visCols = static_cast<SHORT>(csbi.srWindow.Right - csbi.srWindow.Left + 1);
        SHORT visRows = static_cast<SHORT>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
        if (visCols > cols) cols = visCols;
        if (visRows > rows) rows = visRows;
    }
    if (cols < SCREEN_COLS) cols = static_cast<SHORT>(SCREEN_COLS);
    if (rows < SCREEN_ROWS) rows = static_cast<SHORT>(SCREEN_ROWS);

    SetConsoleWindowInfo(hOut, TRUE, &tiny);
    COORD buf = { cols, rows };
    SetConsoleScreenBufferSize(hOut, buf);
    pinViewportTop();
    maximizeConsole();
    pinViewportTop();

    // Re-read visible window and force buffer == window (kills scroll ghosts).
    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        SHORT visCols = static_cast<SHORT>(csbi.srWindow.Right - csbi.srWindow.Left + 1);
        SHORT visRows = static_cast<SHORT>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
        if (visCols < 40) visCols = 40;
        if (visRows < 20) visRows = 20;
        SetConsoleWindowInfo(hOut, TRUE, &tiny);
        COORD exact = { visCols, visRows };
        SetConsoleScreenBufferSize(hOut, exact);
        SMALL_RECT exactWin = { 0, 0, static_cast<SHORT>(visCols - 1),
                                static_cast<SHORT>(visRows - 1) };
        SetConsoleWindowInfo(hOut, TRUE, &exactWin);
        pinViewportTop();
    }

    setColor(attrNormal());
}

void NotepadApp::gotoxy(int x, int y) const {
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE),
                             makeCoord(static_cast<SHORT>(x), static_cast<SHORT>(y)));
}

void NotepadApp::clearScreen() const {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    pinViewportTop();
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) {
        system("cls");
        pinViewportTop();
        return;
    }

    // Shrink buffer to the visible window so old rows cannot scroll back as ghosts.
    SHORT visCols = static_cast<SHORT>(csbi.srWindow.Right - csbi.srWindow.Left + 1);
    SHORT visRows = static_cast<SHORT>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
    if (visCols > 0 && visRows > 0) {
        if (csbi.dwSize.X != visCols || csbi.dwSize.Y != visRows) {
            SMALL_RECT tiny = { 0, 0, 1, 1 };
            SetConsoleWindowInfo(hOut, TRUE, &tiny);
            COORD exact = { visCols, visRows };
            SetConsoleScreenBufferSize(hOut, exact);
            SMALL_RECT exactWin = { 0, 0, static_cast<SHORT>(visCols - 1),
                                    static_cast<SHORT>(visRows - 1) };
            SetConsoleWindowInfo(hOut, TRUE, &exactWin);
            if (!GetConsoleScreenBufferInfo(hOut, &csbi)) {
                system("cls");
                pinViewportTop();
                return;
            }
        }
    }

    DWORD cells = static_cast<DWORD>(csbi.dwSize.X) * static_cast<DWORD>(csbi.dwSize.Y);
    DWORD written = 0;
    COORD home = { 0, 0 };
    WORD fillAttr = attrNormal();
    FillConsoleOutputCharacterA(hOut, ' ', cells, home, &written);
    FillConsoleOutputAttribute(hOut, fillAttr, cells, home, &written);
    SetConsoleCursorPosition(hOut, home);
    pinViewportTop();
}

void NotepadApp::setColor(WORD attr) const {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), attr);
}

void NotepadApp::messageBoxInfo(const wchar_t* text, const wchar_t* caption) const {
    MessageBoxW(nullptr, text, caption, MB_OK | MB_ICONINFORMATION);
}

int NotepadApp::messageBoxYesNo(const wchar_t* text, const wchar_t* caption) const {
    return MessageBoxW(nullptr, text, caption, MB_YESNO | MB_ICONQUESTION);
}

bool NotepadApp::isAllowedChar(char ch) const {
    if (ch == ' ') return true;
    if (isalpha(static_cast<unsigned char>(ch))) return true;
    if (extendedMode_) {
        if (ch >= 32 && ch <= 126) return true;
    }
    return false;
}

void NotepadApp::drawChrome() const {
    setColor(attrTitle());
    gotoxy(0, 0);
    cout << "+-- NOTEPAD APPLICATION -- Interactive Editor "
            "-------------------------------------------+\n";
    setColor(attrDim());
    cout << "| Esc=Menu | 0/Esc Back | F1 Help | Ctrl+N/O/S | Ctrl+Z/Y Undo/Redo | "
            "Ctrl+F Search | F3 Next |\n";

    // Text pane left border + Search right pane borders
    for (int y = TEXT_TOP; y < TEXT_TOP + TEXT_ROWS; ++y) {
        setColor(attrNormal());
        gotoxy(0, y);
        cout << '|';
        gotoxy(TEXT_LEFT + TEXT_COLS, y);
        cout << '|';
        gotoxy(SEARCH_LEFT + SEARCH_COLS, y);
        cout << '|';
    }
    // Bottom of text/search
    gotoxy(0, TEXT_TOP + TEXT_ROWS);
    cout << '+';
    for (int i = 0; i < TEXT_COLS; ++i) cout << '-';
    cout << '+';
    for (int i = 0; i < SEARCH_COLS; ++i) cout << '-';
    cout << '+';

    // Suggestions frame
    setColor(attrSuggest());
    gotoxy(0, SUGGEST_TOP - 1);
    cout << "| Word suggestions ";
    for (int i = 0; i < 70; ++i) cout << ' ';
    cout << '|';
    for (int y = SUGGEST_TOP; y < SUGGEST_TOP + SUGGEST_ROWS - 1; ++y) {
        gotoxy(0, y);
        cout << '|';
        gotoxy(SCREEN_COLS - 1, y);
        cout << '|';
    }
    gotoxy(0, SUGGEST_TOP + SUGGEST_ROWS - 1);
    cout << '+';
    for (int i = 0; i < SCREEN_COLS - 2; ++i) cout << '-';
    cout << '+';
    setColor(attrNormal());
}

void NotepadApp::drawSearchPane() const {
    setColor(attrSearch());
    gotoxy(SEARCH_LEFT + 1, SEARCH_TOP);
    cout << " SEARCH PANE";
    for (int i = 12; i < SEARCH_COLS - 1; ++i) cout << ' ';

    gotoxy(SEARCH_LEFT + 1, SEARCH_TOP + 2);
    cout << "Ctrl+F query:";
    for (int i = 13; i < SEARCH_COLS - 1; ++i) cout << ' ';

    gotoxy(SEARCH_LEFT + 1, SEARCH_TOP + 3);
    if (findQuery_[0]) {
        cout << "\"";
        int n = static_cast<int>(strlen(findQuery_));
        if (n > SEARCH_COLS - 4) n = SEARCH_COLS - 4;
        for (int i = 0; i < n; ++i) cout << findQuery_[i];
        cout << "\"";
        for (int i = n + 2; i < SEARCH_COLS - 1; ++i) cout << ' ';
    } else {
        cout << "(none)";
        for (int i = 6; i < SEARCH_COLS - 1; ++i) cout << ' ';
    }

    gotoxy(SEARCH_LEFT + 1, SEARCH_TOP + 5);
    cout << "Find results:";
    for (int i = 16; i < SEARCH_COLS - 1; ++i) cout << ' ';

    gotoxy(SEARCH_LEFT + 1, SEARCH_TOP + 6);
    if (findQuery_[0] && hlLen_ > 0) {
        cout << "hit @ Ln " << (hlRow_ + 1) << " Col " << (hlCol_ + 1);
        for (int i = 20; i < SEARCH_COLS - 1; ++i) cout << ' ';
    } else if (findQuery_[0]) {
        cout << "No match highlighted";
        for (int i = 20; i < SEARCH_COLS - 1; ++i) cout << ' ';
    } else {
        cout << "No search yet";
        for (int i = 22; i < SEARCH_COLS - 1; ++i) cout << ' ';
    }

    gotoxy(SEARCH_LEFT + 1, SEARCH_TOP + 8);
    cout << "F3 = Find Next";
    for (int i = 14; i < SEARCH_COLS - 1; ++i) cout << ' ';

    gotoxy(SEARCH_LEFT + 1, SEARCH_TOP + 10);
    cout << "Mode: " << (extendedMode_ ? "Letters+symbols" : "Letters only");
    for (int i = 20; i < SEARCH_COLS - 1; ++i) cout << ' ';
    setColor(attrNormal());
}

void NotepadApp::drawStatus() const {
    setColor(attrStatus());
    gotoxy(0, STATUS_ROW);
    const char* name = filePath_[0] ? filePath_ : "(untitled)";
    cout << " Status | " << name << (dirty_ ? " *" : "  ") << " | Ln " << (doc_.cursorRow() + 1)
         << ", Col " << (doc_.cursorCol() + 1) << " | Words " << doc_.wordCount() << " | Chars "
         << doc_.charCount() << " | Undo:" << history_.undoDepth() << "/" << WORD_STACK_CAP
         << " Redo:" << history_.redoDepth() << "/" << WORD_STACK_CAP
         << (extendedMode_ ? " | Letters+symbols" : " | Letters only") << " | F1 Help";
    setColor(attrNormal());
}

void NotepadApp::drawSuggestions() const {
    setColor(attrSuggest());
    gotoxy(2, SUGGEST_TOP);
    char prefix[MAX_WORD_BUF];
    doc_.copyWordAtCursor(prefix, MAX_WORD_BUF);
    WordNode* list = nullptr;
    doc_.collectPrefixWords(prefix, list, MAX_SUGGESTIONS);
    cout << "Prefix \"" << (prefix[0] ? prefix : "") << "\": ";
    if (!list) {
        cout << "(type letters — matches from document words appear here)          ";
    } else {
        int i = 0;
        for (WordNode* p = list; p; p = p->next, ++i) {
            char w[MAX_WORD_BUF];
            charsToBuf(p->chars, w, MAX_WORD_BUF);
            cout << "[" << (i + 1) << "] " << w << "  ";
        }
        cout << "                    ";
    }
    freeWordList(list);
    gotoxy(2, SUGGEST_TOP + 2);
    cout << "Keys: Arrows move | Enter new line | Backspace/Delete | Ctrl+H Replace | "
            "Ctrl+C/X/V copy/cut/paste | Ctrl+E typing mode";
    setColor(attrNormal());
}

void NotepadApp::refresh() {
    clearScreen();
    drawChrome();
    doc_.render(hlRow_, hlCol_, hlLen_);
    drawSearchPane();
    drawSuggestions();
    drawStatus();
    if (showHelp_) {
        setColor(attrTitle());
        gotoxy(8, 6);
        cout << "+==================== HELP — KEYBOARD SHORTCUTS ==================+";
        gotoxy(8, 7);
        cout << "| Letters A-Z only by default. Space separates words.            |";
        gotoxy(8, 8);
        cout << "| Enter starts a new line. Long words wrap to the next line.     |";
        gotoxy(8, 9);
        cout << "| Backspace / Delete remove characters. Arrow keys move around.  |";
        gotoxy(8, 10);
        cout << "| Ctrl+Z undo last word | Ctrl+Y redo last word (up to 5).       |";
        gotoxy(8, 11);
        cout << "| Ctrl+N New | Ctrl+O Open | Ctrl+S Save | Esc opens the menu    |";
        gotoxy(8, 12);
        cout << "| Ctrl+F Find | F3 Find next | Ctrl+H Replace                    |";
        gotoxy(8, 13);
        cout << "| Ctrl+E: allow numbers and symbols (off = letters only).        |";
        gotoxy(8, 14);
        cout << "| Layout: text on the left, find on the right, suggestions below |";
        gotoxy(8, 15);
        cout << "+================================================================+";
        setColor(attrNormal());
    }
    gotoxy(TEXT_LEFT + doc_.cursorCol(), TEXT_TOP + doc_.cursorRow());
}

bool NotepadApp::promptFileName(const char* title, char* out, int outCap) {
    clearScreen();
    setColor(attrTitle());
    cout << "\n\n  " << title << "\n";
    setColor(attrDim());
    cout << "  (Enter 0 to go back)\n\n";
    setColor(attrPrompt());
    cout << "  Enter file name: ";
    setColor(attrNormal());
    cout.flush();
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    char name[MAX_PATH_BUF];
    name[0] = '\0';
    cin.getline(name, MAX_PATH_BUF);
    if (!name[0]) cin.getline(name, MAX_PATH_BUF);
    if (!name[0] || isCancelChoice(name)) {
        setColor(attrWarn());
        cout << "\n  Going back...\n";
        setColor(attrNormal());
        return false;
    }
    size_t n = strlen(name);
    if (n < 4 || strcmp(name + n - 4, ".txt") != 0) {
        if (n + 4 < MAX_PATH_BUF) strcat_s(name, ".txt");
    }
    strncpy_s(out, outCap, name, _TRUNCATE);
    return true;
}

void NotepadApp::readPromptLine(const char* label, char* out, int outCap) {
    setColor(attrPrompt());
    cout << label;
    setColor(attrNormal());
    cout.flush();
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    out[0] = '\0';
    cin.getline(out, outCap);
    if (!out[0]) cin.getline(out, outCap);
}

bool NotepadApp::confirmDiscard() {
    if (!dirty_) return true;
    return messageBoxYesNo(L"Document has unsaved changes. Discard them?", L"Unsaved Changes") ==
           IDYES;
}

void NotepadApp::commitPendingWord() {
    if (!pendingWord_) return;
    history_.recordInsert(dupCharList(pendingWord_), pendingRow_, pendingCol_);
    freeCharList(pendingWord_);
    pendingWord_ = nullptr;
}

void NotepadApp::actionNew() {
    if (!confirmDiscard()) return;
    commitPendingWord();
    freeCharList(pendingWord_);
    doc_.clear();
    history_.clear();
    filePath_[0] = '\0';
    dirty_ = false;
    hlLen_ = 0;
    messageBoxInfo(L"New document ready.", L"New File");
}

void NotepadApp::actionLoad() {
    if (!confirmDiscard()) return;
    clearScreen();
    char path[MAX_PATH_BUF];
    if (!promptFileName("Load file", path, MAX_PATH_BUF)) return;

    commitPendingWord();
    freeCharList(pendingWord_);

    if (!doc_.loadFromFile(path)) {
        char alt[MAX_PATH_BUF];
        strcpy_s(alt, "samples\\");
        strcat_s(alt, path);
        if (doc_.loadFromFile(alt)) {
            strcpy_s(path, alt);
        } else {
            // Missing file -> auto-create empty
            ofstream create(path, ios::out | ios::binary);
            if (!create) {
                messageBoxInfo(L"Could not create that file. Check the name and try again.", L"Load");
                return;
            }
            create.close();
            doc_.clear();
            messageBoxInfo(L"That file was missing, so a new empty file was created.", L"Load");
        }
    }
    strcpy_s(filePath_, path);
    history_.clear();
    dirty_ = false;
    hlLen_ = 0;
    messageBoxInfo(L"File loaded successfully.", L"Load");
}

void NotepadApp::actionSave() {
    if (!filePath_[0]) {
        actionSaveAs();
        return;
    }
    commitPendingWord();
    if (!doc_.saveToFile(filePath_)) {
        // auto-create path if needed
        ofstream create(filePath_, ios::out | ios::binary);
        if (!create) {
            messageBoxInfo(L"Could not save the file. Check the name and try again.", L"Save");
            return;
        }
        create.close();
        if (!doc_.saveToFile(filePath_)) {
            messageBoxInfo(L"Could not save the file. Check the name and try again.", L"Save");
            return;
        }
    }
    dirty_ = false;
    messageBoxInfo(L"File saved successfully.", L"Save");
}

void NotepadApp::actionSaveAs() {
    clearScreen();
    char path[MAX_PATH_BUF];
    if (!promptFileName("Save As", path, MAX_PATH_BUF)) return;
    commitPendingWord();
    if (!doc_.saveToFile(path)) {
        messageBoxInfo(L"Could not save the file. Check the name and try again.", L"Save As");
        return;
    }
    strcpy_s(filePath_, path);
    dirty_ = false;
    messageBoxInfo(L"File saved successfully.", L"Save As");
}

void NotepadApp::actionFind() {
    clearScreen();
    setColor(attrTitle());
    cout << "\n\n  Find text\n";
    setColor(attrDim());
    cout << "  (Enter 0 to go back)\n\n";
    readPromptLine("  Enter text to find: ", findQuery_, MAX_QUERY_BUF);
    if (!findQuery_[0] || isCancelChoice(findQuery_)) {
        findQuery_[0] = '\0';
        return;
    }
    actionFindNext();
}

void NotepadApp::actionFindNext() {
    if (!findQuery_[0]) {
        actionFind();
        return;
    }
    int r = 0, c = 0;
    if (doc_.findNext(findQuery_, r, c, true)) {
        doc_.setCursor(r, c);
        hlRow_ = r;
        hlCol_ = c;
        hlLen_ = static_cast<int>(strlen(findQuery_));
    } else {
        hlLen_ = 0;
        messageBoxInfo(L"No matches found. Try a different search.", L"Find");
    }
}

void NotepadApp::actionReplace() {
    clearScreen();
    setColor(attrTitle());
    cout << "\n\n  Replace text\n";
    setColor(attrDim());
    cout << "  (Enter 0 to go back)\n\n";
    readPromptLine("  Enter text to find: ", findQuery_, MAX_QUERY_BUF);
    if (!findQuery_[0] || isCancelChoice(findQuery_)) {
        findQuery_[0] = '\0';
        return;
    }
    readPromptLine("  Enter replacement text: ", replaceQuery_, MAX_QUERY_BUF);
    if (isCancelChoice(replaceQuery_)) {
        replaceQuery_[0] = '\0';
        return;
    }
    int r = 0, c = 0;
    if (!doc_.findNext(findQuery_, r, c, true)) {
        messageBoxInfo(L"No matches found. Try a different search.", L"Replace");
        return;
    }
    doc_.setCursor(r, c);
    commitPendingWord();
    int flen = static_cast<int>(strlen(findQuery_));
    for (int i = 0; i < flen; ++i) {
        char rm;
        doc_.deleteForward(rm);
    }
    for (int i = 0; replaceQuery_[i]; ++i) {
        if (isAllowedChar(replaceQuery_[i])) doc_.insertChar(replaceQuery_[i]);
    }
    dirty_ = true;
    hlRow_ = r;
    hlCol_ = c;
    hlLen_ = static_cast<int>(strlen(replaceQuery_));
}

void NotepadApp::actionCopy() {
    freeCharList(clipboard_);
    char buf[MAX_WORD_BUF];
    doc_.copyWordAtCursor(buf, MAX_WORD_BUF);
    if (!buf[0]) {
        char line[512];
        doc_.copyLine(doc_.cursorRow(), line, 512);
        charsFromCStr(clipboard_, line);
    } else {
        charsFromCStr(clipboard_, buf);
    }
}

void NotepadApp::actionCut() {
    actionCopy();
    char target[MAX_WORD_BUF];
    doc_.copyWordAtCursor(target, MAX_WORD_BUF);
    if (target[0]) {
        char line[512];
        doc_.copyLine(doc_.cursorRow(), line, 512);
        const char* pos = strstr(line, target);
        if (pos) {
            int start = static_cast<int>(pos - line);
            int len = static_cast<int>(strlen(target));
            commitPendingWord();
            CharNode* w = nullptr;
            charsFromCStr(w, target);
            history_.recordDelete(w, doc_.cursorRow(), start);
            doc_.eraseWordAt(doc_.cursorRow(), start, len);
            dirty_ = true;
        }
    }
}

void NotepadApp::actionPaste() {
    if (!clipboard_) return;
    for (CharNode* p = clipboard_; p; p = p->next) typeChar(p->ch);
}

void NotepadApp::actionHelp() { showHelp_ = !showHelp_; }

void NotepadApp::actionToggleExtended() {
    extendedMode_ = !extendedMode_;
    messageBoxInfo(extendedMode_
                       ? L"Extended typing is ON.\r\n"
                         L"You can type letters, numbers, and symbols.\r\n"
                         L"Press Ctrl+E again to go back to letters only."
                       : L"Letters-only typing is ON.\r\n"
                         L"Only letters and spaces are allowed.\r\n"
                         L"Press Ctrl+E to allow numbers and symbols.",
                   L"Typing Mode");
}

void NotepadApp::actionUndo() {
    commitPendingWord();
    if (history_.undo(doc_)) dirty_ = true;
}

void NotepadApp::actionRedo() {
    commitPendingWord();
    if (history_.redo(doc_)) dirty_ = true;
}

void NotepadApp::typeChar(char ch) {
    if (!isAllowedChar(ch)) return;

    if (ch == ' ') {
        commitPendingWord();
        if (!doc_.insertChar(' ')) return;
        dirty_ = true;
        hlLen_ = 0;
        return;
    }

    // Letter (or extended printable)
    if (!pendingWord_) {
        pendingRow_ = doc_.cursorRow();
        pendingCol_ = doc_.cursorCol();
    }
    if (!doc_.insertChar(ch)) {
        messageBoxInfo(L"This line or page is full. No more room to type here.", L"Space Full");
        return;
    }
    appendChar(pendingWord_, ch);
    // If wrap moved us, pending start may need update when word started wrap —
    // approximate: if cursor row changed from pendingRow, update pending coords
    if (doc_.cursorRow() != pendingRow_ && charsLen(pendingWord_) > 0) {
        // Word was wrapped: start is beginning of current line
        pendingRow_ = doc_.cursorRow();
        pendingCol_ = doc_.cursorCol() - charsLen(pendingWord_);
        if (pendingCol_ < 0) pendingCol_ = 0;
    }
    dirty_ = true;
    hlLen_ = 0;
}

void NotepadApp::doBackspace() {
    char rm = '\0';
    int r = doc_.cursorRow();
    int c = doc_.cursorCol();

    if (c > 0) {
        char left = doc_.charAt(r, c - 1);
        if (pendingWord_ && charsLen(pendingWord_) > 0 && isalpha(static_cast<unsigned char>(left))) {
            // Peel last pending letter
            CharNode* p = pendingWord_;
            CharNode* prev = nullptr;
            while (p->next) {
                prev = p;
                p = p->next;
            }
            if (!doc_.backspace(rm)) return;
            if (!prev) {
                delete pendingWord_;
                pendingWord_ = nullptr;
            } else {
                prev->next = nullptr;
                delete p;
            }
            dirty_ = true;
            hlLen_ = 0;
            return;
        }
        // Backspacing into a completed word: collect word being deleted letter by letter
        // When crossing into previous word boundary, record whole word once finished
        if (!doc_.backspace(rm)) return;
        if (rm != ' ' && rm != '\n' && isalpha(static_cast<unsigned char>(rm))) {
            // Build reverse then we commit when hitting space/boundary on next ops —
            // simpler: if left of cursor is space or BOL, we just deleted end of a word
            bool wordEnd = (doc_.cursorCol() == 0) ||
                           (doc_.charAt(doc_.cursorRow(), doc_.cursorCol() - 1) == ' ');
            // Accumulate into a temporary via pending delete buffer using clipboard style
            // For rubric: record word when fully removed. Use a static-ish approach:
            // push single-letter deletes only when word boundary hit — store in pendingWord_ reversed
            // Reuse pendingWord_ as deleted-char accumulator when not typing
            if (!pendingWord_) {
                // start delete-word capture at this char
                appendChar(pendingWord_, rm);
                pendingRow_ = doc_.cursorRow();
                pendingCol_ = doc_.cursorCol();
            } else {
                // prepend
                CharNode* n = new CharNode(rm);
                n->next = pendingWord_;
                pendingWord_ = n;
                pendingCol_ = doc_.cursorCol();
                pendingRow_ = doc_.cursorRow();
            }
            if (wordEnd) {
                // completed deleting a word
                history_.recordDelete(dupCharList(pendingWord_), pendingRow_, pendingCol_);
                freeCharList(pendingWord_);
                pendingWord_ = nullptr;
            }
        } else {
            // Hit space or other: flush any delete accumulator
            if (pendingWord_) {
                history_.recordDelete(dupCharList(pendingWord_), pendingRow_, pendingCol_);
                freeCharList(pendingWord_);
                pendingWord_ = nullptr;
            }
        }
        dirty_ = true;
        hlLen_ = 0;
        return;
    }

    // Join lines at col 0
    commitPendingWord();
    freeCharList(pendingWord_);
    if (!doc_.backspace(rm)) return;
    dirty_ = true;
    hlLen_ = 0;
}

void NotepadApp::doDelete() {
    commitPendingWord();
    freeCharList(pendingWord_);
    char rm = '\0';
    if (!doc_.deleteForward(rm)) return;
    dirty_ = true;
    hlLen_ = 0;
}

void NotepadApp::doEnter() {
    commitPendingWord();
    if (!doc_.insertNewline()) {
        messageBoxInfo(L"No more room for new lines.", L"Space Full");
        return;
    }
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
            actionLoad();
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
        case 'E':
            actionToggleExtended();
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
        commitPendingWord();
        doc_.moveLeft();
        return;
    case VK_RIGHT:
        commitPendingWord();
        doc_.moveRight();
        return;
    case VK_UP:
        commitPendingWord();
        doc_.moveUp();
        return;
    case VK_DOWN:
        commitPendingWord();
        doc_.moveDown();
        return;
    case VK_HOME:
        commitPendingWord();
        if (ctrl) doc_.moveDocHome();
        else doc_.moveHome();
        return;
    case VK_END:
        commitPendingWord();
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

bool NotepadApp::showWelcomeScreen() {
    for (;;) {
        clearScreen();
        setColor(attrOk());
        cout << "\n\n";
        cout << "  =====================================================================\n";
        cout << "  ||                                                                 ||\n";
        cout << "  ||                    NOTEPAD APPLICATION                          ||\n";
        cout << "  ||                                                                 ||\n";
        cout << "  ||           Interactive Console Editor With Search                ||\n";
        cout << "  ||           Author: Mohammad Rohaan - 22I-2327                    ||\n";
        cout << "  ||                                                                 ||\n";
        cout << "  =====================================================================\n\n";
        setColor(attrTitle());
        cout << "   WELCOME\n\n";
        setColor(attrNormal());
        cout << "     1  = Start notepad\n";
        cout << "     0  = Exit\n\n";
        setColor(attrPrompt());
        cout << "  Enter your choice: ";
        setColor(attrNormal());

        char tok[64];
        tok[0] = '\0';
        if (!(cin >> tok)) {
            if (cin.eof())
                return false;
            cin.clear();
            cin.ignore(10000, '\n');
            continue;
        }
        cin.ignore(10000, '\n');

        if (isCancelChoice(tok)) {
            clearScreen();
            setColor(attrOk());
            cout << "\n  Goodbye. Thank you for using Notepad.\n\n";
            setColor(attrNormal());
            running_ = false;
            return false;
        }
        if (tok[0] == '1' && tok[1] == '\0')
            return true;

        setColor(attrError());
        cout << "\n  That option is not on the list. Type 1 to start or 0 to exit.\n";
        setColor(attrNormal());
        Sleep(900);
        clearScreen();  // wipe error before redrawing welcome only
    }
}

void NotepadApp::showMainMenu() {
    while (running_) {
        clearScreen();
        setColor(attrTitle());
        cout << "\n\n";
        cout << "  =====================================================================\n";
        cout << "  ||                     NOTEPAD — MAIN MENU                         ||\n";
        cout << "  =====================================================================\n\n";
        setColor(attrNormal());
        cout << "     1.  New File\n";
        cout << "     2.  Load File\n";
        cout << "     3.  Save File\n";
        cout << "     4.  Exit program\n";
        cout << "  ---------------------------------------------------------------------\n";
        cout << "     5.  Continue Editing\n";
        cout << "     6.  Save As\n";
        cout << "     7.  Help\n";
        cout << "     0.  Back to Welcome\n\n";
        setColor(attrPrompt());
        cout << "  Enter your choice: ";
        setColor(attrNormal());

        char tok[64];
        tok[0] = '\0';
        if (!(cin >> tok)) {
            if (cin.eof()) {
                running_ = false;
                return;
            }
            cin.clear();
            cin.ignore(10000, '\n');
            setColor(attrError());
            cout << "\n  Please choose a number from the list of options.\n";
            setColor(attrNormal());
            Sleep(800);
            continue;
        }
        cin.ignore(10000, '\n');

        if (isCancelChoice(tok)) {
            if (!showWelcomeScreen())
                return;
            continue;
        }

        int choice = atoi(tok);
        switch (choice) {
        case 1:
            actionNew();
            return;
        case 2:
            actionLoad();
            return;
        case 3:
            actionSave();
            return;
        case 4:
            if (dirty_) {
                if (messageBoxYesNo(L"Save before exiting?", L"Confirm Exit") == IDYES) actionSave();
            }
            running_ = false;
            return;
        case 5:
            return;
        case 6:
            actionSaveAs();
            break;
        case 7:
            messageBoxInfo(
                L"Typing: letters only by default. Space separates words.\r\n"
                L"Enter starts a new line. Backspace and Delete remove text.\r\n"
                L"Ctrl+Z undoes the last word; Ctrl+Y redoes it (up to 5).\r\n"
                L"Menu: New / Open / Save / Exit. Esc opens the menu. 0 goes back.\r\n"
                L"F1 Help, Ctrl+F Find, F3 Find next, Ctrl+H Replace.\r\n"
                L"Ctrl+E allows numbers and symbols. Ctrl+C / X / V for copy/cut/paste.",
                L"Help");
            break;
        default:
            setColor(attrError());
            cout << "\n  That option is not on the list. Choose 1-7, or 0 to go back.\n";
            setColor(attrNormal());
            Sleep(900);
            clearScreen();  // wipe error; loop redraws main menu only
            break;
        }
    }
}

int NotepadApp::run() {
    setConsoleTitleBar(L"Notepad Application — Interactive Editor");
    setupConsoleDisplay();
    doc_.clear();

    if (!showWelcomeScreen())
        return 0;

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
    // Destructors free all document / stack nodes
    return 0;
}

int main() {
    NotepadApp app;
    return app.run();
}
