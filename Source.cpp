#include "Header.h"

using namespace std;

static COORD makeCoord(SHORT x, SHORT y) {
    COORD c;
    c.X = x;
    c.Y = y;
    return c;
}

static WORD attrNormal() {
    /* Bright white on black - high contrast */
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

// Prevent Ctrl+C from killing the process - it must copy like Notepad.
static BOOL WINAPI ignoreCtrlCHandler(DWORD ctrlType) {
    if (ctrlType == CTRL_C_EVENT || ctrlType == CTRL_BREAK_EVENT)
        return TRUE; // handled: do not terminate
    return FALSE;
}

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

void Document::stripTrailingSpaces(int /*r*/) {
    // Keep spaces exactly as the user typed them (Windows Notepad behavior).
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

    // Detach word (and anything after it - should only be the word at EOL)
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

    // Spaces are freely allowed (any count, including at start of line) - like Notepad.

    // Whole-word wrap: if adding a letter would overflow, move current word down first
    if (ch != ' ') {
        int start = wordStartCol();
        int curWordLen = col_ - start; // letters already in word before insert
        if (start + curWordLen + 1 > maxCols()) {
            if (!wrapCurrentWordToNextLine()) return false;
        } else if (lineLength(row_) >= maxCols()) {
            // Line full but cursor mid-line with room in word sense - still block overflow
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

static bool isWordTokenChar(char ch) {
    unsigned char u = static_cast<unsigned char>(ch);
    if (isalnum(u)) return true;
    // symbols/punctuation count as token pieces for suggestions (not space)
    return (ch >= 33 && ch <= 126 && !isalnum(u));
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
    if (i > 0 && (i == len || !isWordTokenChar(line[i]))) --i;
    if (i < 0 || i >= len || !isWordTokenChar(line[i])) return;
    int a = i, b = i;
    while (a > 0 && isWordTokenChar(line[a - 1])) --a;
    while (b + 1 < len && isWordTokenChar(line[b + 1])) ++b;
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

static bool isTokenChar(char ch, bool allowExtended) {
    unsigned char u = static_cast<unsigned char>(ch);
    if (isalpha(u)) return true;
    if (allowExtended) {
        if (isdigit(u)) return true;
        if (ch != ' ' && ch >= 33 && ch <= 126) return true; // symbols + digits
    }
    return false;
}

void Document::collectPrefixWords(const char* prefix, WordNode*& outHead, int maxCount,
                                  bool allowExtendedTokens) const {
    outHead = nullptr;
    if (!prefix || !prefix[0] || maxCount <= 0) return;
    int count = 0;
    if (!head_) return;

    for (Node* row = head_; row && count < maxCount; row = row->down) {
        if (row->data == '\0' && !row->right) continue;
        char word[MAX_WORD_BUF];
        int wi = 0;
        for (Node* p = row; p; p = p->right) {
            if (isTokenChar(p->data, allowExtendedTokens)) {
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

static bool cellInSelection(int r, int c, int r0, int c0, int r1, int c1, bool selOn) {
    if (!selOn) return false;
    if (r < r0 || r > r1) return false;
    if (r0 == r1) return c >= c0 && c < c1;
    if (r == r0) return c >= c0;
    if (r == r1) return c < c1;
    return true;
}

void Document::render(int viewTopRow, int highlightRow, int highlightCol, int highlightLen,
                      int selR0, int selC0, int selR1, int selC1, bool selOn) const {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    const int vis = visibleRows();
    if (viewTopRow < 0) viewTopRow = 0;

    for (int screen = 0; screen < vis; ++screen) {
        int r = viewTopRow + screen;
        SetConsoleCursorPosition(
            h, makeCoord(static_cast<SHORT>(TEXT_LEFT), static_cast<SHORT>(TEXT_TOP + screen)));
        if (r >= lineCount()) {
            for (int i = 0; i < maxCols(); ++i) cout << ' ';
            continue;
        }
        int c = 0;
        Node* row = rowHead(r);
        if (row && !(row->data == '\0' && !row->right)) {
            for (Node* p = row; p; p = p->right, ++c) {
                bool hl = (highlightLen > 0 && r == highlightRow && c >= highlightCol &&
                           c < highlightCol + highlightLen);
                bool sel = cellInSelection(r, c, selR0, selC0, selR1, selC1, selOn);
                SetConsoleTextAttribute(h, (sel || hl) ? attrHighlight() : attrNormal());
                cout << p->data;
                if (sel || hl) SetConsoleTextAttribute(h, attrNormal());
            }
        }
        for (int i = c; i < maxCols(); ++i) cout << ' ';
    }
    SetConsoleTextAttribute(h, attrNormal());
}

int Document::copyRange(int r0, int c0, int r1, int c1, char* out, int outCap) const {
    if (!out || outCap <= 0) return 0;
    out[0] = '\0';
    if (r0 > r1 || (r0 == r1 && c0 >= c1)) return 0;
    int n = 0;
    for (int r = r0; r <= r1; ++r) {
        int start = (r == r0) ? c0 : 0;
        int end = (r == r1) ? c1 : lineLength(r);
        for (int c = start; c < end; ++c) {
            if (n + 1 >= outCap) { out[n] = '\0'; return n; }
            char ch = charAt(r, c);
            if (ch == '\0') continue;
            out[n++] = ch;
        }
        if (r < r1) {
            if (n + 1 >= outCap) { out[n] = '\0'; return n; }
            out[n++] = '\n';
        }
    }
    out[n] = '\0';
    return n;
}

bool Document::deleteRange(int r0, int c0, int r1, int c1) {
    if (r0 > r1 || (r0 == r1 && c0 >= c1)) return false;
    // Delete from the end so indices stay valid.
    setCursor(r1, c1);
    while (!(row_ == r0 && col_ == c0)) {
        char rm = '\0';
        if (col_ > 0 || (row_ == r0 && col_ > c0)) {
            if (!backspace(rm)) break;
        } else if (row_ > r0) {
            if (!backspace(rm)) break;
        } else {
            break;
        }
        if (row_ < r0 || (row_ == r0 && col_ < c0)) {
            setCursor(r0, c0);
            break;
        }
    }
    setCursor(r0, c0);
    return true;
}

bool Document::insertTextAtCursor(const char* text, bool allowExtended) {
    if (!text) return false;
    for (int i = 0; text[i]; ++i) {
        char ch = text[i];
        if (ch == '\n' || ch == '\r') {
            if (ch == '\r' && text[i + 1] == '\n') ++i;
            if (!insertNewline()) return false;
            continue;
        }
        unsigned char u = static_cast<unsigned char>(ch);
        bool ok = (ch == ' ') || isalpha(u) || (allowExtended && ch >= 32 && ch <= 126);
        if (!ok) continue;
        if (!insertChar(ch)) return false;
    }
    return true;
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
    char buf[8192];
    charsToBuf(a->word, buf, 8192);
    int len = charsLen(a->word);
    bool ok = false;
    auto endAfter = [](int r, int c, const char* s, int& er, int& ec) {
        er = r; ec = c;
        for (int i = 0; s[i]; ++i) {
            if (s[i] == '\n') { ++er; ec = 0; }
            else ++ec;
        }
    };
    // Special: newline insert/delete
    if (len == 1 && buf[0] == '\n') {
        if (a->inserted) {
            doc.setCursor(a->row + 1, 0);
            char rm = '\0';
            ok = doc.backspace(rm);
            if (ok) redo_.push(dupCharList(a->word), a->row, a->col, true);
        } else {
            doc.setCursor(a->row, a->col);
            ok = doc.insertNewline();
            if (ok) redo_.push(dupCharList(a->word), a->row, a->col, false);
        }
        freeCharList(a->word);
        delete a;
        return ok;
    }
    if (a->inserted) {
        if (strchr(buf, '\n')) {
            int er = 0, ec = 0;
            endAfter(a->row, a->col, buf, er, ec);
            ok = doc.deleteRange(a->row, a->col, er, ec);
        } else {
            ok = doc.eraseWordAt(a->row, a->col, len);
        }
        if (ok) redo_.push(dupCharList(a->word), a->row, a->col, true);
    } else {
        doc.setCursor(a->row, a->col);
        if (strchr(buf, '\n')) {
            ok = doc.insertTextAtCursor(buf, true);
        } else {
            ok = doc.insertWordAt(a->row, a->col, buf);
        }
        if (ok) redo_.push(dupCharList(a->word), a->row, a->col, false);
    }
    freeCharList(a->word);
    delete a;
    return ok;
}


bool WordHistory::redo(Document& doc) {
    WordAction* a = redo_.pop();
    if (!a) return false;
    char buf[8192];
    charsToBuf(a->word, buf, 8192);
    int len = charsLen(a->word);
    bool ok = false;
    auto endAfter = [](int r, int c, const char* s, int& er, int& ec) {
        er = r; ec = c;
        for (int i = 0; s[i]; ++i) {
            if (s[i] == '\n') { ++er; ec = 0; }
            else ++ec;
        }
    };
    if (len == 1 && buf[0] == '\n') {
        if (a->inserted) {
            doc.setCursor(a->row, a->col);
            ok = doc.insertNewline();
            if (ok) undo_.push(dupCharList(a->word), a->row, a->col, true);
        } else {
            doc.setCursor(a->row + 1, 0);
            char rm = '\0';
            ok = doc.backspace(rm);
            if (ok) undo_.push(dupCharList(a->word), a->row, a->col, false);
        }
        freeCharList(a->word);
        delete a;
        return ok;
    }
    if (a->inserted) {
        doc.setCursor(a->row, a->col);
        if (strchr(buf, '\n')) {
            ok = doc.insertTextAtCursor(buf, true);
        } else {
            ok = doc.insertWordAt(a->row, a->col, buf);
        }
        if (ok) undo_.push(dupCharList(a->word), a->row, a->col, true);
    } else {
        if (strchr(buf, '\n')) {
            int er = 0, ec = 0;
            endAfter(a->row, a->col, buf, er, ec);
            ok = doc.deleteRange(a->row, a->col, er, ec);
        } else {
            ok = doc.eraseWordAt(a->row, a->col, len);
        }
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
      clipboard_(nullptr),
      selOn_(false),
      selAnchorRow_(0),
      selAnchorCol_(0),
      suggestCount_(0),
      docViewTop_(0) {
    filePath_[0] = '\0';
    findQuery_[0] = '\0';
    replaceQuery_[0] = '\0';
    for (int i = 0; i < MAX_SUGGESTIONS; ++i) suggestCache_[i][0] = '\0';
}

NotepadApp::~NotepadApp() {
    freeCharList(pendingWord_);
    freeCharList(clipboard_);
    // doc_ and history_ destructors free their nodes
}

void NotepadApp::ensureScrollableBuffer() const {
    // Windows often shrinks the buffer to the window on maximize/resize, which
    // locks/hides the scrollbar. Always keep the buffer larger than the window.
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi))
        return;

    SHORT winCols = static_cast<SHORT>(csbi.srWindow.Right - csbi.srWindow.Left + 1);
    SHORT winRows = static_cast<SHORT>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
    if (winCols < 40) winCols = 40;
    if (winRows < 10) winRows = 10;

    // Extra space so vertical + horizontal scrollbars stay available and unlocked.
    SHORT needCols = static_cast<SHORT>(winCols + 80);
    SHORT needRows = static_cast<SHORT>(winRows + 600);
    if (needCols < SCREEN_COLS + 80) needCols = static_cast<SHORT>(SCREEN_COLS + 80);
    if (needRows < SCREEN_ROWS + 600) needRows = static_cast<SHORT>(SCREEN_ROWS + 600);

    if (csbi.dwSize.X < needCols || csbi.dwSize.Y < needRows) {
        COORD bigger = { needCols, needRows };
        SetConsoleScreenBufferSize(hOut, bigger);
    }

    // Re-assert the visible window size does NOT cover the whole buffer.
    if (!GetConsoleScreenBufferInfo(hOut, &csbi))
        return;
    SHORT maxWinCols = static_cast<SHORT>(csbi.dwSize.X - 1);
    SHORT maxWinRows = static_cast<SHORT>(csbi.dwSize.Y - 1);
    SHORT keepCols = winCols;
    SHORT keepRows = winRows;
    if (keepCols > maxWinCols) keepCols = maxWinCols;
    if (keepRows > maxWinRows) keepRows = maxWinRows;
    // Leave at least ~50 buffer rows below the window so scrollbar can move.
    if (csbi.dwSize.Y - keepRows < 50) {
        keepRows = static_cast<SHORT>(csbi.dwSize.Y - 50);
        if (keepRows < 10) keepRows = 10;
    }
    SHORT left = csbi.srWindow.Left;
    SHORT top = csbi.srWindow.Top;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (left + keepCols >= csbi.dwSize.X)
        left = static_cast<SHORT>(csbi.dwSize.X - keepCols);
    if (top + keepRows >= csbi.dwSize.Y)
        top = static_cast<SHORT>(csbi.dwSize.Y - keepRows);
    SMALL_RECT win = { left, top,
                       static_cast<SHORT>(left + keepCols - 1),
                       static_cast<SHORT>(top + keepRows - 1) };
    SetConsoleWindowInfo(hOut, TRUE, &win);
}

void NotepadApp::maximizeConsole() {
    HWND w = GetConsoleWindow();
    if (!w)
        return;
    ShowWindow(w, SW_RESTORE);
    ShowWindow(w, SW_SHOW);
    ShowWindow(w, SW_MAXIMIZE);
    SetForegroundWindow(w);
    Sleep(30);
    // Maximize often equals buffer to window - unlock scrollbar again.
    ensureScrollableBuffer();
}


void NotepadApp::scrollViewportBy(int rowDelta, int colDelta) const {
    if (rowDelta == 0 && colDelta == 0) return;
    ensureScrollableBuffer();
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi))
        return;

    SHORT winCols = static_cast<SHORT>(csbi.srWindow.Right - csbi.srWindow.Left + 1);
    SHORT winRows = static_cast<SHORT>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
    int left = csbi.srWindow.Left + colDelta;
    int top = csbi.srWindow.Top + rowDelta;

    int maxLeft = csbi.dwSize.X - winCols;
    int maxTop = csbi.dwSize.Y - winRows;
    if (maxLeft < 0) maxLeft = 0;
    if (maxTop < 0) maxTop = 0;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (left > maxLeft) left = maxLeft;
    if (top > maxTop) top = maxTop;

    SMALL_RECT win = {
        static_cast<SHORT>(left),
        static_cast<SHORT>(top),
        static_cast<SHORT>(left + winCols - 1),
        static_cast<SHORT>(top + winRows - 1)
    };
    SetConsoleWindowInfo(hOut, TRUE, &win);
}

bool NotepadApp::handleMouseWheel(const MOUSE_EVENT_RECORD& mouse) const {
    // Two-finger trackpad scroll arrives as MOUSE_WHEELED / MOUSE_HWHEELED.
    if (mouse.dwEventFlags & MOUSE_WHEELED) {
        const SHORT delta = static_cast<SHORT>((mouse.dwButtonState >> 16) & 0xFFFF);
        // Positive delta = scroll up (content down / view toward buffer top).
        int steps = static_cast<int>(delta) / WHEEL_DELTA;
        if (steps == 0) steps = (delta > 0) ? 1 : -1;
        // Match typical Windows apps: one notch moves a few lines.
        scrollViewportBy(-steps * 3, 0);
        return true;
    }
    if (mouse.dwEventFlags & MOUSE_HWHEELED) {
        const SHORT delta = static_cast<SHORT>((mouse.dwButtonState >> 16) & 0xFFFF);
        int steps = static_cast<int>(delta) / WHEEL_DELTA;
        if (steps == 0) steps = (delta > 0) ? 1 : -1;
        scrollViewportBy(0, -steps * 3);
        return true;
    }
    return false;
}

bool NotepadApp::readLineInteractive(char* out, int outCap) {
    if (!out || outCap <= 0) return false;
    out[0] = '\0';
    setCookedInput();
    ensureScrollableBuffer();
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));

    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    int len = 0;
    cout.flush();

    for (;;) {
        INPUT_RECORD rec;
        DWORD n = 0;
        if (!ReadConsoleInput(hIn, &rec, 1, &n) || n == 0) continue;

        if (rec.EventType == WINDOW_BUFFER_SIZE_EVENT) {
            ensureScrollableBuffer();
            continue;
        }
        if (rec.EventType == MOUSE_EVENT) {
            handleMouseWheel(rec.Event.MouseEvent);
            continue;
        }
        if (rec.EventType != KEY_EVENT || !rec.Event.KeyEvent.bKeyDown)
            continue;

        const KEY_EVENT_RECORD& key = rec.Event.KeyEvent;
        const WORD vk = key.wVirtualKeyCode;
        char ch = key.uChar.AsciiChar;

        if (vk == VK_RETURN) {
            cout << "\n";
            cout.flush();
            out[len] = '\0';
            return true;
        }
        if (vk == VK_ESCAPE) {
            out[0] = '0';
            out[1] = '\0';
            cout << "\n";
            cout.flush();
            return true;
        }
        if (vk == VK_BACK) {
            if (len > 0) {
                --len;
                out[len] = '\0';
                // erase last echoed character
                cout << "\b \b";
                cout.flush();
            }
            continue;
        }
        if (ch >= 32 && ch <= 126) {
            if (len + 1 < outCap) {
                out[len++] = ch;
                out[len] = '\0';
                cout << ch;
                cout.flush();
            }
        }
    }
}

void NotepadApp::pinViewportTop() const {
    // Soft scroll-to-top only. Never resize window to the full buffer.
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi))
        return;
    ensureScrollableBuffer();
    if (!GetConsoleScreenBufferInfo(hOut, &csbi))
        return;
    SHORT winW = static_cast<SHORT>(csbi.srWindow.Right - csbi.srWindow.Left);
    SHORT winH = static_cast<SHORT>(csbi.srWindow.Bottom - csbi.srWindow.Top);
    if (winW < 1) winW = 1;
    if (winH < 1) winH = 1;
    if (winW >= csbi.dwSize.X - 1) winW = static_cast<SHORT>(csbi.dwSize.X - 2);
    if (winH >= csbi.dwSize.Y - 1) winH = static_cast<SHORT>(csbi.dwSize.Y - 2);
    if (winW < 1) winW = 1;
    if (winH < 1) winH = 1;
    SMALL_RECT vis = { 0, 0, winW, winH };
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


void NotepadApp::setCookedInput() const {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD inMode = 0;
    if (GetConsoleMode(hIn, &inMode)) {
        inMode |= ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT;
        // No LINE_INPUT/ECHO - we echo ourselves in readLineInteractive so wheel
        // events can be handled (trackpad two-finger scroll) while typing.
        inMode |= ENABLE_PROCESSED_INPUT;
        inMode &= ~ENABLE_LINE_INPUT;
        inMode &= ~ENABLE_ECHO_INPUT;
        inMode &= ~ENABLE_QUICK_EDIT_MODE;
        SetConsoleMode(hIn, inMode);
    }
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD outMode = 0;
    if (GetConsoleMode(hOut, &outMode)) {
        outMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
        SetConsoleMode(hOut, outMode);
    }
    // Make sure the cursor is visible for typing a choice.
    CONSOLE_CURSOR_INFO ci;
    if (GetConsoleCursorInfo(hOut, &ci)) {
        ci.bVisible = TRUE;
        if (ci.dwSize < 1) ci.dwSize = 25;
        SetConsoleCursorInfo(hOut, &ci);
    }
}

void NotepadApp::setRawEditorInput() const {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD inMode = 0;
    if (GetConsoleMode(hIn, &inMode)) {
        inMode |= ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
        inMode &= ~ENABLE_QUICK_EDIT_MODE;
        inMode &= ~ENABLE_PROCESSED_INPUT; // Ctrl+C as a key for copy
        inMode &= ~ENABLE_LINE_INPUT;
        inMode &= ~ENABLE_ECHO_INPUT;
        SetConsoleMode(hIn, inMode);
    }
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD outMode = 0;
    if (GetConsoleMode(hOut, &outMode)) {
        outMode |= ENABLE_PROCESSED_OUTPUT;
        outMode &= ~ENABLE_WRAP_AT_EOL_OUTPUT; // fixed panes
        SetConsoleMode(hOut, outMode);
    }
    FlushConsoleInputBuffer(hIn);
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
        inMode |= ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
        inMode &= ~ENABLE_QUICK_EDIT_MODE;
        inMode &= ~ENABLE_PROCESSED_INPUT; // Ctrl+C arrives as a key, not a kill signal
        SetConsoleMode(hIn, inMode);
    }
    SetConsoleCtrlHandler(ignoreCtrlCHandler, TRUE);

    RECT wa;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
    int pxW = wa.right - wa.left;
    int pxH = wa.bottom - wa.top - GetSystemMetrics(SM_CYCAPTION) - 16;
    if (pxW < 640) pxW = 640;
    if (pxH < 400) pxH = 400;

    // Screen-zoom style: size glyphs so SCREEN_ROWS fills the work area
    // (like zooming the display - fewer cells, much larger text).
    const int layoutRows = SCREEN_ROWS;
    SHORT fontY = static_cast<SHORT>(pxH / layoutRows);
    if (fontY > 64) fontY = 64;
    if (fontY < 32) fontY = 32;

    // Back off only if a half-row would clip at the bottom.
    while (fontY > 28 && (fontY * layoutRows) > (pxH - 8)) {
        --fontY;
    }

    CONSOLE_FONT_INFOEX cfi;
    ZeroMemory(&cfi, sizeof(cfi));
    cfi.cbSize = sizeof(cfi);
    GetCurrentConsoleFontEx(hOut, FALSE, &cfi);
    // Wider cells to match the tall glyphs (zoom feel, not skinny tall letters).
    SHORT fontX = static_cast<SHORT>(fontY / 2);
    if (fontX < 16) fontX = 16;
    cfi.dwFontSize.X = fontX;
    cfi.dwFontSize.Y = fontY;
    wcscpy_s(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(hOut, FALSE, &cfi);

    // Start with a layout-sized window, then unlock a large scroll buffer.
    SMALL_RECT tiny = { 0, 0, 1, 1 };
    SetConsoleWindowInfo(hOut, TRUE, &tiny);
    COORD starter = { static_cast<SHORT>(SCREEN_COLS + 80),
                      static_cast<SHORT>(SCREEN_ROWS + 600) };
    SetConsoleScreenBufferSize(hOut, starter);
    SMALL_RECT starterWin = { 0, 0, static_cast<SHORT>(SCREEN_COLS - 1),
                              static_cast<SHORT>(SCREEN_ROWS - 1) };
    SetConsoleWindowInfo(hOut, TRUE, &starterWin);

    maximizeConsole(); // also calls ensureScrollableBuffer()
    Sleep(40);
    ensureScrollableBuffer();
    ensureScrollableBuffer(); // second pass after Windows finishes maximize layout

    setColor(attrNormal());
}

void NotepadApp::gotoxy(int x, int y) const {
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE),
                             makeCoord(static_cast<SHORT>(x), static_cast<SHORT>(y)));
}

void NotepadApp::clearScreen(bool resetScroll) const {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    ensureScrollableBuffer();
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) {
        system("cls");
        ensureScrollableBuffer();
        return;
    }

    // Wipe the ENTIRE buffer. Never shrink it to the window (that locks scroll).
    DWORD cells = static_cast<DWORD>(csbi.dwSize.X) * static_cast<DWORD>(csbi.dwSize.Y);
    DWORD written = 0;
    COORD home = { 0, 0 };
    WORD fillAttr = attrNormal();
    FillConsoleOutputCharacterA(hOut, ' ', cells, home, &written);
    FillConsoleOutputAttribute(hOut, fillAttr, cells, home, &written);
    if (resetScroll) {
        // Move view to top without locking - keep window smaller than buffer.
        SetConsoleCursorPosition(hOut, home);
        pinViewportTop();
    }
    ensureScrollableBuffer();
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

bool NotepadApp::isWordChar(char ch) const {
    unsigned char u = static_cast<unsigned char>(ch);
    if (isalnum(u)) return true;
    if (extendedMode_ && ch >= 33 && ch <= 126) return true;
    return isalpha(u);
}

void NotepadApp::drawChrome() const {
    writePaddedRow(0, attrTitle(), "+-- NOTEPAD - Editor --+");
    writePaddedRow(1, attrDim(), "| Esc Menu | F1 Help | Tab/Alt+1-8 Suggest | Ctrl+E AlphaNumeric | Ctrl+Z/Y |");

    // Text pane + search pane; right edge locked to SCREEN_COLS - 1 for clean alignment.
    const int rightEdge = SCREEN_COLS - 1;
    for (int y = TEXT_TOP; y < TEXT_TOP + TEXT_ROWS; ++y) {
        setColor(attrNormal());
        gotoxy(0, y);
        cout << '|';
        gotoxy(TEXT_LEFT + TEXT_COLS, y);
        cout << '|';
        gotoxy(rightEdge, y);
        cout << '|';
    }
    gotoxy(0, TEXT_TOP + TEXT_ROWS);
    cout << '+';
    for (int i = 0; i < TEXT_COLS; ++i) cout << '-';
    cout << '+';
    int mid = rightEdge - (TEXT_LEFT + TEXT_COLS) - 1;
    if (mid < 0) mid = 0;
    for (int i = 0; i < mid; ++i) cout << '-';
    cout << '+';

    // Suggestions frame
    setColor(attrSuggest());
    writePaddedRow(SUGGEST_TOP - 1, attrSuggest(), "| Word suggestions");
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
    const int inner = SEARCH_COLS - 1; // space inside right border
    auto putRow = [&](int y, const char* text) {
        gotoxy(SEARCH_LEFT + 1, y);
        int n = text ? static_cast<int>(strlen(text)) : 0;
        if (n > inner) n = inner;
        for (int i = 0; i < n; ++i) cout << text[i];
        for (int i = n; i < inner; ++i) cout << ' ';
    };

    putRow(SEARCH_TOP, " SEARCH PANE");
    putRow(SEARCH_TOP + 2, "Ctrl+F query:");

    if (findQuery_[0]) {
        char buf[128];
        int qn = static_cast<int>(strlen(findQuery_));
        if (qn > 40) qn = 40;
        buf[0] = '"';
        for (int i = 0; i < qn; ++i) buf[i + 1] = findQuery_[i];
        buf[qn + 1] = '"';
        buf[qn + 2] = '\0';
        putRow(SEARCH_TOP + 3, buf);
    } else {
        putRow(SEARCH_TOP + 3, "(none)");
    }

    putRow(SEARCH_TOP + 5, "Find results:");
    if (findQuery_[0] && hlLen_ > 0) {
        char buf[64];
        sprintf_s(buf, "hit @ Ln %d Col %d", hlRow_ + 1, hlCol_ + 1);
        putRow(SEARCH_TOP + 6, buf);
    } else if (findQuery_[0]) {
        putRow(SEARCH_TOP + 6, "No match highlighted");
    } else {
        putRow(SEARCH_TOP + 6, "No search yet");
    }

    putRow(SEARCH_TOP + 8, "F3 = Find Next");
    putRow(SEARCH_TOP + 9, "Ctrl+H = Replace");
    putRow(SEARCH_TOP + 10, extendedMode_ ? "Mode: AlphaNumeric" : "Mode: Letters only");
    setColor(attrNormal());
}

void NotepadApp::drawStatus() const {
    setColor(attrStatus());
    int statusY = STATUS_ROW;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int winW = SCREEN_COLS;
    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        int lastSafe = static_cast<int>(csbi.srWindow.Bottom) - 1; // leave bottom visible row blank
        if (lastSafe < 1) lastSafe = 1;
        if (statusY > lastSafe) statusY = lastSafe;
        winW = static_cast<int>(csbi.srWindow.Right - csbi.srWindow.Left + 1);
        if (winW < SCREEN_COLS) winW = SCREEN_COLS;
    }
    gotoxy(0, statusY);
    const char* name = filePath_[0] ? filePath_ : "(untitled)";
    char line[512];
    sprintf_s(line, " Status | %s%s | Ln %d, Col %d | Words %d | Undo %d/%d Redo %d/%d%s",
              name,
              dirty_ ? " *" : "  ",
              doc_.cursorRow() + 1,
              doc_.cursorCol() + 1,
              doc_.wordCount(),
              history_.undoDepth(), WORD_STACK_CAP,
              history_.redoDepth(), WORD_STACK_CAP,
              extendedMode_ ? " | AlphaNumeric" : " | Letters only");
    int n = static_cast<int>(strlen(line));
    if (n > winW) {
        if (winW > 3) {
            line[winW - 3] = '.';
            line[winW - 2] = '.';
            line[winW - 1] = '.';
            line[winW] = '\0';
            n = winW;
        } else {
            line[winW] = '\0';
            n = winW;
        }
    }
    cout << line;
    for (int i = n; i < winW; ++i) cout << ' ';
    setColor(attrNormal());
}


void NotepadApp::drawSuggestions() {
    setColor(attrSuggest());
    char prefix[MAX_WORD_BUF];
    prefix[0] = '\0';
    // Build prefix from the token at the cursor (letters always; digits/symbols too).
    {
        char line[512];
        doc_.copyLine(doc_.cursorRow(), line, 512);
        int len = static_cast<int>(strlen(line));
        int c = doc_.cursorCol();
        if (c > len) c = len;
        auto isTok = [](char ch) {
            unsigned char u = static_cast<unsigned char>(ch);
            return isalnum(u) || (ch >= 33 && ch <= 126 && ch != ' ');
        };
        int i = c;
        if (i > 0 && (i >= len || !isTok(line[i]))) --i;
        if (i >= 0 && i < len && isTok(line[i])) {
            int a = i, b = i;
            while (a > 0 && isTok(line[a - 1])) --a;
            while (b + 1 < len && isTok(line[b + 1])) ++b;
            // Prefix = token start through cursor (Notepad-style completion).
            int end = c;
            if (end < a) end = a;
            if (end > b + 1) end = b + 1;
            int n = end - a;
            if (n >= MAX_WORD_BUF) n = MAX_WORD_BUF - 1;
            if (n < 0) n = 0;
            for (int k = 0; k < n; ++k) prefix[k] = line[a + k];
            prefix[n] = '\0';
        }
    }
    WordNode* list = nullptr;
    // Suggestions work in Letters-only AND AlphaNumeric modes.
    doc_.collectPrefixWords(prefix, list, MAX_SUGGESTIONS, true);
    suggestCount_ = 0;
    for (int i = 0; i < MAX_SUGGESTIONS; ++i) suggestCache_[i][0] = '\0';

    char body[1024];
    body[0] = '\0';
    {
        char head[128];
        sprintf_s(head, "Suggest \"%s\" (1-8/Tab): ", prefix[0] ? prefix : "");
        strcpy_s(body, head);
        if (!list) {
            strcat_s(body, "(no matches yet)");
        } else {
            char tmp[MAX_SUGGESTIONS][MAX_WORD_BUF];
            int n = 0;
            for (WordNode* p = list; p && n < MAX_SUGGESTIONS; p = p->next) {
                charsToBuf(p->chars, tmp[n], MAX_WORD_BUF);
                ++n;
            }
            for (int i = 0; i < n; ++i) {
                strcpy_s(suggestCache_[i], tmp[n - 1 - i]);
                char piece[128];
                sprintf_s(piece, "[%d] %s  ", i + 1, suggestCache_[i]);
                if (strlen(body) + strlen(piece) + 1 < sizeof(body))
                    strcat_s(body, piece);
            }
            suggestCount_ = n;
        }
    }
    freeWordList(list);

    int contentWidth = static_cast<int>(strlen(body)) + 4; // | body |
    if (contentWidth < SCREEN_COLS) contentWidth = SCREEN_COLS;
    adjustWindowForSuggestions(contentWidth);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int winW = SCREEN_COLS;
    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        winW = static_cast<int>(csbi.srWindow.Right - csbi.srWindow.Left + 1);
        if (winW < SCREEN_COLS) winW = SCREEN_COLS;
    }

    auto drawBorderedRow = [&](int y, const char* text) {
        gotoxy(0, y);
        cout << '|';
        int inner = winW - 2;
        if (inner < 1) inner = 1;
        int n = text ? static_cast<int>(strlen(text)) : 0;
        if (n > inner) n = inner;
        for (int i = 0; i < n; ++i) cout << text[i];
        for (int i = n; i < inner; ++i) cout << ' ';
        if (winW >= 2) cout << '|';
    };

    drawBorderedRow(SUGGEST_TOP, body);
    drawBorderedRow(SUGGEST_TOP + 2,
                    "Suggest pick BOTH modes: Tab=#1 | Alt+1-8 | Letters-only also 1-8 | Ctrl+Z/Y");
    setColor(attrNormal());
}


void NotepadApp::refresh() {
    clearScreen(false);
    drawChrome();
    int r0 = 0, c0 = 0, r1 = 0, c1 = 0;
    if (selOn_) getSelectionBounds(r0, c0, r1, c1);
    ensureCursorVisible();
    doc_.render(docViewTop_, hlRow_, hlCol_, hlLen_, r0, c0, r1, c1, selOn_);
    drawSearchPane();
    drawSuggestions();
    drawStatus();
if (showHelp_) {
        setColor(attrTitle());
        gotoxy(2, 3);
        cout << "+============= HELP / SHORTCUTS (current build) =============+";
        gotoxy(2, 4);
        cout << "| TYPING: Letters-only default | Ctrl+E = AlphaNumeric mode  |";
        gotoxy(2, 5);
        cout << "| Spaces unlimited | Enter | Bksp/Del | Shift+Arrows | C/X/V |";
        gotoxy(2, 6);
        cout << "| SUGGESTIONS: BOTH modes - Tab=#1, Alt+1-8 (Letters also 1-8)|";
        gotoxy(2, 7);
        cout << "| FIND: Ctrl+F | F3 next | Ctrl+H replace (next or ALL)      |";
        gotoxy(2, 8);
        cout << "| UNDO/REDO: Ctrl+Z/Y for type/space/Enter/cut/paste/replace |";
        gotoxy(2, 9);
        cout << "|   suggestions and deletes too (stack up to 200 actions)    |";
        gotoxy(2, 10);
        cout << "| SAVE: both Save and Save As write .txt files               |";
        gotoxy(2, 11);
        cout << "|   Save = current file | Save As = new name (separate copy) |";
        gotoxy(2, 12);
        cout << "| TEST: type, Tab suggest, Ctrl+Z, menu 6 Save As, F1 again  |";
        gotoxy(2, 13);
        cout << "| Esc = menu | trackpad/wheel scrolls | caret stays in pane  |";
        gotoxy(2, 14);
        cout << "+===========================================================+";
        setColor(attrNormal());
    }


    // Erase any clipped leftover pixels on the bottom-most visible line / buffer row.
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        DWORD written = 0;
        SHORT lastVis = csbi.srWindow.Bottom;
        SHORT width = csbi.dwSize.X;
        COORD rowHome = { 0, lastVis };
        FillConsoleOutputCharacterA(hOut, ' ', static_cast<DWORD>(width), rowHome, &written);
        FillConsoleOutputAttribute(hOut, attrNormal(), static_cast<DWORD>(width), rowHome, &written);
        if (csbi.dwSize.Y - 1 > lastVis) {
            COORD bufBottom = { 0, static_cast<SHORT>(csbi.dwSize.Y - 1) };
            FillConsoleOutputCharacterA(hOut, ' ', static_cast<DWORD>(width), bufBottom, &written);
            FillConsoleOutputAttribute(hOut, attrNormal(), static_cast<DWORD>(width), bufBottom, &written);
        }
    }

    // Keep the blink cursor inside the notepad pane (real Notepad behavior).
    // If the doc cursor is past the bottom visible line, ensureCursorVisible()
    // already scrolled docViewTop_ - map to screen coords within TEXT_ROWS.
    int screenRow = doc_.cursorRow() - docViewTop_;
    if (screenRow < 0) screenRow = 0;
    if (screenRow >= TEXT_ROWS) screenRow = TEXT_ROWS - 1;
    int screenCol = doc_.cursorCol();
    if (screenCol < 0) screenCol = 0;
    if (screenCol >= TEXT_COLS) screenCol = TEXT_COLS - 1;
    gotoxy(TEXT_LEFT + screenCol, TEXT_TOP + screenRow);
}

bool NotepadApp::promptFileName(const char* title, char* out, int outCap) {
    setCookedInput();
    clearScreen(true);
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
    readLineInteractive(name, MAX_PATH_BUF);
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
    readLineInteractive(out, outCap);
}

bool NotepadApp::confirmDiscard() {
    if (!dirty_) return true;
    return messageBoxYesNo(L"Document has unsaved changes. Discard them?", L"Unsaved Changes") ==
           IDYES;
}

void NotepadApp::commitPendingWord() {
    // Per-keystroke undo already recorded each character. Just clear the buffer.
    if (!pendingWord_) return;
    freeCharList(pendingWord_);
    pendingWord_ = nullptr;
}


void NotepadApp::clearSelection() { selOn_ = false; }

void NotepadApp::ensureSelectionAnchor() {
    if (!selOn_) {
        selOn_ = true;
        selAnchorRow_ = doc_.cursorRow();
        selAnchorCol_ = doc_.cursorCol();
    }
}

void NotepadApp::getSelectionBounds(int& r0, int& c0, int& r1, int& c1) const {
    int ar = selAnchorRow_, ac = selAnchorCol_;
    int cr = doc_.cursorRow(), cc = doc_.cursorCol();
    if (ar < cr || (ar == cr && ac <= cc)) {
        r0 = ar; c0 = ac; r1 = cr; c1 = cc;
    } else {
        r0 = cr; c0 = cc; r1 = ar; c1 = ac;
    }
}

void NotepadApp::deleteSelectionIfAny() {
    if (!selOn_) return;
    int r0, c0, r1, c1;
    getSelectionBounds(r0, c0, r1, c1);
    if (r0 == r1 && c0 == c1) { clearSelection(); return; }
    commitPendingWord();
    char buf[8192];
    buf[0] = '\0';
    doc_.copyRange(r0, c0, r1, c1, buf, 8192);
    if (buf[0]) {
        CharNode* oldw = nullptr;
        charsFromCStr(oldw, buf);
        history_.recordDelete(oldw, r0, c0);
    }
    doc_.deleteRange(r0, c0, r1, c1);
    clearSelection();
    dirty_ = true;
    hlLen_ = 0;
}



void NotepadApp::recordTypedChar(char ch, int row, int col) {
    CharNode* n = nullptr;
    appendChar(n, ch);
    history_.recordInsert(n, row, col);
}

void NotepadApp::recordRemovedChar(char ch, int row, int col) {
    CharNode* n = nullptr;
    appendChar(n, ch);
    history_.recordDelete(n, row, col);
}

void NotepadApp::adjustWindowForSuggestions(int contentWidth) {
    // Widen the visible console window when suggestions need more horizontal room;
    // shrink back toward the layout width when the list is short/empty.
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
    ensureScrollableBuffer();
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;

    SHORT needCols = static_cast<SHORT>(contentWidth + 4);
    if (needCols < SCREEN_COLS) needCols = static_cast<SHORT>(SCREEN_COLS);
    if (needCols > csbi.dwSize.X - 1) needCols = static_cast<SHORT>(csbi.dwSize.X - 1);

    SHORT winRows = static_cast<SHORT>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
    if (winRows < SCREEN_ROWS) winRows = static_cast<SHORT>(SCREEN_ROWS);
    if (winRows > csbi.dwSize.Y - 2) winRows = static_cast<SHORT>(csbi.dwSize.Y - 2);

    SHORT left = csbi.srWindow.Left;
    SHORT top = csbi.srWindow.Top;
    if (left + needCols >= csbi.dwSize.X)
        left = static_cast<SHORT>(csbi.dwSize.X - needCols);
    if (left < 0) left = 0;
    SMALL_RECT win = { left, top,
                       static_cast<SHORT>(left + needCols - 1),
                       static_cast<SHORT>(top + winRows - 1) };
    SetConsoleWindowInfo(hOut, TRUE, &win);
}

void NotepadApp::actionSelectAll() {
    commitPendingWord();
    if (doc_.empty()) { clearSelection(); return; }
    selOn_ = true;
    selAnchorRow_ = 0;
    selAnchorCol_ = 0;
    doc_.moveDocEnd();
}


void NotepadApp::ensureCursorVisible() {
    // When the cursor hits the bottom (or top) edge of the notepad pane,
    // scroll the document view - never place the caret outside the border.
    int r = doc_.cursorRow();
    if (r < docViewTop_) docViewTop_ = r;
    while (r >= docViewTop_ + TEXT_ROWS) {
        ++docViewTop_;
    }
    if (docViewTop_ < 0) docViewTop_ = 0;
    int maxTop = doc_.lineCount() - TEXT_ROWS;
    if (maxTop < 0) maxTop = 0;
    // Allow one blank line beyond last content so Enter can scroll.
    if (r + 1 - TEXT_ROWS > maxTop) maxTop = r + 1 - TEXT_ROWS;
    if (maxTop < 0) maxTop = 0;
    if (docViewTop_ > maxTop) docViewTop_ = maxTop;
}

void NotepadApp::getTokenBoundsAtCursor(int& startCol, int& endCol) const {
    char line[512];
    doc_.copyLine(doc_.cursorRow(), line, 512);
    int len = static_cast<int>(strlen(line));
    int c = doc_.cursorCol();
    if (c > len) c = len;
    startCol = c;
    endCol = c;
    // If cursor is mid-token or just after a token char, expand to full token.
    int i = c;
    if (i > 0 && (i >= len || !isWordChar(line[i]))) --i;
    if (i < 0 || i >= len || !isWordChar(line[i])) {
        startCol = c;
        endCol = c;
        return;
    }
    startCol = i;
    while (startCol > 0 && isWordChar(line[startCol - 1])) --startCol;
    endCol = i;
    while (endCol + 1 < len && isWordChar(line[endCol + 1])) ++endCol;
    ++endCol; // exclusive
}

void NotepadApp::actionApplySuggestion(int index) {
    if (index < 0 || index >= suggestCount_) return;
    const char* word = suggestCache_[index];
    if (!word[0]) return;

    // Drop pending typing without committing - we replace the token at the cursor.
    freeCharList(pendingWord_);
    pendingWord_ = nullptr;
    deleteSelectionIfAny();

    int row = doc_.cursorRow();
    int start = 0, end = 0;
    getTokenBoundsAtCursor(start, end);

    char oldTok[MAX_WORD_BUF];
    oldTok[0] = '\0';
    if (end > start) {
        char line[512];
        doc_.copyLine(row, line, 512);
        int n = end - start;
        if (n >= MAX_WORD_BUF) n = MAX_WORD_BUF - 1;
        for (int i = 0; i < n; ++i) oldTok[i] = line[start + i];
        oldTok[n] = '\0';
        doc_.eraseWordAt(row, start, end - start);
    }
    doc_.setCursor(row, start);
    int insertAt = start;
    doc_.insertTextAtCursor(word, extendedMode_);

    // Undo stack is LIFO: record old-delete first, then new-insert so the
    // first Ctrl+Z removes the suggestion and the next restores the old token.
    if (oldTok[0]) {
        CharNode* oldw = nullptr;
        charsFromCStr(oldw, oldTok);
        history_.recordDelete(oldw, row, insertAt);
    }
    CharNode* neu = nullptr;
    charsFromCStr(neu, word);
    history_.recordInsert(neu, row, insertAt);

    dirty_ = true;
    hlLen_ = 0;
    ensureCursorVisible();
}

void NotepadApp::actionNew() {
    if (!confirmDiscard()) return;
    commitPendingWord();
    freeCharList(pendingWord_);
    doc_.clear();
    history_.clear();
    docViewTop_ = 0;
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
    messageBoxInfo(
        L"Saved as a separate .txt copy under the new name.\r\n"
        L"The editor now uses that file path (Save will update it).",
        L"Save As");
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

void NotepadApp::actionReplaceAll() {
    if (!findQuery_[0]) return;
    commitPendingWord();
    clearSelection();
    int replaced = 0;
    int guard = 0;
    doc_.setCursor(0, 0);
    while (guard++ < 10000) {
        int r = 0, c = 0;
        if (!doc_.findNext(findQuery_, r, c, false)) {
            if (replaced == 0) {
                doc_.setCursor(0, 0);
                if (!doc_.findNext(findQuery_, r, c, true)) break;
            } else {
                break;
            }
        }
        int flen = static_cast<int>(strlen(findQuery_));
        int rlen = static_cast<int>(strlen(replaceQuery_));
        doc_.setCursor(r, c);
        CharNode* oldw = nullptr;
        charsFromCStr(oldw, findQuery_);
        history_.recordDelete(oldw, r, c);
        for (int i = 0; i < flen; ++i) {
            char rm;
            doc_.deleteForward(rm);
        }
        if (rlen > 0) {
            CharNode* neu = nullptr;
            charsFromCStr(neu, replaceQuery_);
            history_.recordInsert(neu, r, c);
        }
        doc_.insertTextAtCursor(replaceQuery_, extendedMode_);
        ++replaced;
        doc_.setCursor(r, c + rlen);
    }
    dirty_ = replaced > 0;
    wchar_t msg[128];
    swprintf_s(msg, L"Replaced %d place(s).", replaced);
    messageBoxInfo(msg, L"Replace All");
}

void NotepadApp::actionReplace() {
    clearScreen(true);
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

    char mode[64];
    for (;;) {
        mode[0] = '\0';
        setColor(attrPrompt());
        cout << "\n  1 = Replace next match\n  2 = Replace ALL matches\n  0 = Cancel\n";
        readPromptLine("  Choose: ", mode, 64);
        if (isCancelChoice(mode) || !mode[0]) return;
        if ((mode[0] == '1' || mode[0] == '2') && mode[1] == '\0') break;
        setColor(attrError());
        cout << "\n  That option is not on the list. Choose 1, 2, or 0.\n";
        setColor(attrNormal());
        Sleep(900);
        clearScreen(true);
        setColor(attrTitle());
        cout << "\n\n  Replace text\n";
        setColor(attrDim());
        cout << "  Find: \"" << findQuery_ << "\"\n";
        cout << "  Replace with: \"" << replaceQuery_ << "\"\n";
        setColor(attrNormal());
    }

    if (mode[0] == '2') {
        actionReplaceAll();
        return;
    }

    int r = 0, c = 0;
    if (!doc_.findNext(findQuery_, r, c, true)) {
        messageBoxInfo(L"No matches found. Try a different search.", L"Replace");
        return;
    }
    doc_.setCursor(r, c);
    commitPendingWord();
    clearSelection();
    int flen = static_cast<int>(strlen(findQuery_));
    CharNode* oldw = nullptr;
    charsFromCStr(oldw, findQuery_);
    history_.recordDelete(oldw, r, c);
    for (int i = 0; i < flen; ++i) {
        char rm;
        doc_.deleteForward(rm);
    }
    int rlen = static_cast<int>(strlen(replaceQuery_));
    if (rlen > 0) {
        CharNode* neu = nullptr;
        charsFromCStr(neu, replaceQuery_);
        history_.recordInsert(neu, r, c);
    }
    doc_.insertTextAtCursor(replaceQuery_, extendedMode_);
    dirty_ = true;
    hlRow_ = r;
    hlCol_ = c;
    hlLen_ = rlen;
    ensureCursorVisible();
}


void NotepadApp::actionCopy() {
    freeCharList(clipboard_);
    char buf[8192];
    buf[0] = '\0';
    if (selOn_) {
        int r0, c0, r1, c1;
        getSelectionBounds(r0, c0, r1, c1);
        doc_.copyRange(r0, c0, r1, c1, buf, 8192);
    } else {
        doc_.copyWordAtCursor(buf, MAX_WORD_BUF);
        if (!buf[0]) doc_.copyLine(doc_.cursorRow(), buf, 8192);
    }
    if (buf[0]) charsFromCStr(clipboard_, buf);
}

void NotepadApp::actionCut() {
    actionCopy();
    if (selOn_) {
        deleteSelectionIfAny();
        return;
    }
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
    commitPendingWord();
    deleteSelectionIfAny();
    char buf[8192];
    charsToBuf(clipboard_, buf, 8192);
    if (!buf[0]) return;
    int row = doc_.cursorRow();
    int col = doc_.cursorCol();
    CharNode* neu = nullptr;
    charsFromCStr(neu, buf);
    history_.recordInsert(neu, row, col);
    doc_.insertTextAtCursor(buf, extendedMode_);
    dirty_ = true;
    hlLen_ = 0;
    ensureCursorVisible();
}


void NotepadApp::actionHelp() { showHelp_ = !showHelp_; }

void NotepadApp::actionToggleExtended() {
    extendedMode_ = !extendedMode_;
    messageBoxInfo(extendedMode_
                       ? L"AlphaNumeric mode is ON.\r\n"
                         L"You can type letters, numbers, and symbols.\r\n"
                         L"Press Ctrl+E again for Letters only."
                       : L"Letters-only mode is ON.\r\n"
                         L"Only letters and spaces are allowed.\r\n"
                         L"Press Ctrl+E for AlphaNumeric mode.",
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
    if (selOn_) deleteSelectionIfAny();

    if (ch == ' ') {
        commitPendingWord();
        int row = doc_.cursorRow();
        int col = doc_.cursorCol();
        if (!doc_.insertChar(' ')) return;
        recordTypedChar(' ', row, col);
        dirty_ = true;
        hlLen_ = 0;
        return;
    }

    if (!pendingWord_) {
        pendingRow_ = doc_.cursorRow();
        pendingCol_ = doc_.cursorCol();
    }
    int row = doc_.cursorRow();
    int col = doc_.cursorCol();
    if (!doc_.insertChar(ch)) {
        messageBoxInfo(L"This line or page is full. No more room to type here.", L"Space Full");
        return;
    }
    recordTypedChar(ch, row, col);
    appendChar(pendingWord_, ch);
    if (doc_.cursorRow() != pendingRow_ && charsLen(pendingWord_) > 0) {
        pendingRow_ = doc_.cursorRow();
        pendingCol_ = doc_.cursorCol() - charsLen(pendingWord_);
        if (pendingCol_ < 0) pendingCol_ = 0;
    }
    dirty_ = true;
    hlLen_ = 0;
}

void NotepadApp::doBackspace() {
    if (selOn_) { deleteSelectionIfAny(); return; }
    char rm = '\0';
    int r = doc_.cursorRow();
    int c = doc_.cursorCol();

    // Peel pending typing buffer if we delete inside it (no extra history beyond per-char).
    if (c > 0 && pendingWord_ && charsLen(pendingWord_) > 0) {
        CharNode* p = pendingWord_;
        CharNode* prev = nullptr;
        while (p->next) { prev = p; p = p->next; }
        if (!doc_.backspace(rm)) return;
        recordRemovedChar(rm, r, c - 1);
        if (!prev) { delete pendingWord_; pendingWord_ = nullptr; }
        else { prev->next = nullptr; delete p; }
        dirty_ = true;
        hlLen_ = 0;
        return;
    }

    if (!doc_.backspace(rm)) return;
    // Join-line case: backspace at col 0 removes a newline.
    if (c == 0 && r > 0) {
        CharNode* nl = nullptr;
        appendChar(nl, '\n');
        // Newline was at end of previous line
        int prevLen = doc_.lineLength(doc_.cursorRow());
        history_.recordDelete(nl, doc_.cursorRow(), prevLen);
    } else {
        recordRemovedChar(rm, r, c - 1);
        if (pendingWord_) { freeCharList(pendingWord_); pendingWord_ = nullptr; }
    }
    dirty_ = true;
    hlLen_ = 0;
}

void NotepadApp::doDelete() {
    if (selOn_) { deleteSelectionIfAny(); return; }
    commitPendingWord();
    int r = doc_.cursorRow();
    int c = doc_.cursorCol();
    char rm = '\0';
    if (!doc_.deleteForward(rm)) return;
    if (rm == '\n' || (rm == '\0' && false)) {
        CharNode* nl = nullptr;
        appendChar(nl, '\n');
        history_.recordDelete(nl, r, c);
    } else if (rm != '\0') {
        recordRemovedChar(rm, r, c);
    }
    dirty_ = true;
    hlLen_ = 0;
}

void NotepadApp::doEnter() {
    if (selOn_) deleteSelectionIfAny();
    commitPendingWord();
    int row = doc_.cursorRow();
    int col = doc_.cursorCol();
    if (!doc_.insertNewline()) {
        messageBoxInfo(L"No more room for new lines.", L"Space Full");
        return;
    }
    CharNode* nl = nullptr;
    appendChar(nl, '\n');
    history_.recordInsert(nl, row, col);
    dirty_ = true;
    hlLen_ = 0;
    ensureCursorVisible();
}

void NotepadApp::handleKey(const KEY_EVENT_RECORD& key) {
    const bool ctrl = (key.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
    const bool shift = (key.dwControlKeyState & SHIFT_PRESSED) != 0;
    const WORD vk = key.wVirtualKeyCode;
    const char ch = key.uChar.AsciiChar;

    if (ctrl) {
        switch (vk) {
        case 'N':
            actionNew();
            clearScreen(true);
            setRawEditorInput();
            refresh();
            return;
        case 'O':
            actionLoad();
            clearScreen(true);
            setRawEditorInput();
            refresh();
            return;
        case 'S':
            actionSave();
            clearScreen(true);
            setRawEditorInput();
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
            clearScreen(true);
            setRawEditorInput();
            refresh();
            return;
        case 'H':
            actionReplace();
            clearScreen(true);
            setRawEditorInput();
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
        case 'A':
            actionSelectAll();
            return;
        case 'E':
            actionToggleExtended();
            return;
        default:
            break;
        }
    }

    // Pick suggestion: Tab = first; Alt+1..8 = pick that item.
    // In letters-only mode, plain 1-8 also picks (digits are not typed anyway).
    const bool alt = (key.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0;
    if (!ctrl) {
        if (vk == VK_TAB) {
            actionApplySuggestion(0);
            return;
        }
        if (ch >= '1' && ch <= '8' && suggestCount_ > 0) {
            int idx = ch - '1';
            // Both modes: Alt+1..8 picks. Letters-only: plain 1..8 also picks.
            // AlphaNumeric: plain digits type normally unless Alt is held.
            if (idx < suggestCount_ && (alt || !extendedMode_)) {
                actionApplySuggestion(idx);
                return;
            }
        }
    }

    switch (vk) {
    case VK_ESCAPE:
        clearScreen(true);
        showMainMenu();
        if (running_) {
            clearScreen(true);
            setRawEditorInput();
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
        if (shift) ensureSelectionAnchor();
        else clearSelection();
        doc_.moveLeft();
        return;
    case VK_RIGHT:
        commitPendingWord();
        if (shift) ensureSelectionAnchor();
        else clearSelection();
        doc_.moveRight();
        return;
    case VK_UP:
        commitPendingWord();
        if (shift) ensureSelectionAnchor();
        else clearSelection();
        doc_.moveUp();
        return;
    case VK_DOWN:
        commitPendingWord();
        if (shift) ensureSelectionAnchor();
        else clearSelection();
        doc_.moveDown();
        return;
    case VK_HOME:
        commitPendingWord();
        if (shift) ensureSelectionAnchor();
        else clearSelection();
        if (ctrl) doc_.moveDocHome();
        else doc_.moveHome();
        return;
    case VK_END:
        commitPendingWord();
        if (shift) ensureSelectionAnchor();
        else clearSelection();
        if (ctrl) doc_.moveDocEnd();
        else doc_.moveEnd();
        return;
    case VK_RETURN:
        if (selOn_) deleteSelectionIfAny();
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
        setCookedInput();
        ensureScrollableBuffer();
        clearScreen(true);
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
        cout.flush();

        char tok[64];
        tok[0] = '\0';
        if (!readLineInteractive(tok, 64))
            return false;
        char* p = tok;
        while (*p == ' ' || *p == '\t') ++p;
        if (p != tok) memmove(tok, p, strlen(p) + 1);

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
        setCookedInput();
        ensureScrollableBuffer();
        clearScreen(true);
        setColor(attrTitle());
        cout << "\n\n";
        cout << "  =====================================================================\n";
        cout << "  ||                     NOTEPAD - MAIN MENU                         ||\n";
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
        cout.flush();

        char tok[64];
        tok[0] = '\0';
        if (!readLineInteractive(tok, 64)) {
            running_ = false;
            return;
        }
        char* p = tok;
        while (*p == ' ' || *p == '\t') ++p;
        if (p != tok) memmove(tok, p, strlen(p) + 1);
        if (!tok[0]) {
            setColor(attrError());
            cout << "\n  Please choose a number from the list of options.\n";
            setColor(attrNormal());
            Sleep(800);
            continue;
        }

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
                L"NOTEPAD - HELP (current features)\r\n\r\n"
                L"TYPING\r\n"
                L"- Default: Letters-only. Spaces are unlimited.\r\n"
                L"- Ctrl+E: AlphaNumeric mode (letters + numbers + symbols).\r\n"
                L"- Enter = new line. Backspace / Delete remove text.\r\n"
                L"- At the bottom edge of the notepad pane, the view scrolls down\r\n"
                L"  (caret stays inside the border, like Windows Notepad).\r\n\r\n"
                L"SELECTION / CLIPBOARD\r\n"
                L"- Shift+Arrows select. Ctrl+A select all.\r\n"
                L"- Ctrl+C copy, Ctrl+X cut, Ctrl+V paste.\r\n"
                L"- Paste replaces the current selection.\r\n\r\n"
                L"SUGGESTIONS (bottom panel) - BOTH MODES\r\n"
                L"- Suggestion pick works in Letters-only AND in AlphaNumeric.\r\n"
                L"- Tab = insert suggestion #1 at the cursor (does not jump away).\r\n"
                L"- Alt+1 .. Alt+8 = insert that suggestion at the cursor (both modes).\r\n"
                L"- Letters-only: plain keys 1-8 can also pick when the list shows.\r\n"
                L"- AlphaNumeric: hold Alt with 1-8 so digits still type normally.\r\n\r\n"
                L"FIND / REPLACE\r\n"
                L"- Ctrl+F find, F3 find next.\r\n"
                L"- Ctrl+H replace - choose next match or Replace ALL.\r\n"
                L"- Wrong menu choice: error clears, then you are asked again.\r\n\r\n"
                L"UNDO / REDO (Ctrl+Z / Ctrl+Y)\r\n"
                L"- Covers typing, space, Enter, Backspace/Delete, cut, paste,\r\n"
                L"  replace, and suggestion pick. Stack holds up to 200 actions.\r\n\r\n"
                L"SAVE vs SAVE AS\r\n"
                L"- Both write .txt files.\r\n"
                L"- Save updates the current file (or asks a name if untitled).\r\n"
                L"- Save As asks a new name and writes a separate copy; the editor\r\n"
                L"  then points at that new file.\r\n\r\n"
                L"HOW TO TEST\r\n"
                L"- Type words, press Tab or Alt+1 to pick a suggestion, then Ctrl+Z.\r\n"
                L"- Use menu 6 Save As with a new .txt name, then open that file.\r\n"
                L"- Press F1 for the on-screen shortcut overlay.\r\n\r\n"
                L"OTHER\r\n"
                L"- Esc = main menu. F1 = on-screen shortcut overlay.\r\n"
                L"- Trackpad two-finger scroll / mouse wheel scrolls the view.\r\n"
                L"- Menu 7 = this Help box.",
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
    setConsoleTitleBar(L"Notepad Application - Interactive Editor");
    setupConsoleDisplay();
    doc_.clear();

    if (!showWelcomeScreen())
        return 0;

    showMainMenu();
    if (!running_) return 0;

    clearScreen(true);
    setRawEditorInput();
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
            if (buffer[i].EventType == WINDOW_BUFFER_SIZE_EVENT) {
                ensureScrollableBuffer();
                refresh();
                continue;
            }
            if (buffer[i].EventType == MOUSE_EVENT) {
                const MOUSE_EVENT_RECORD& mouse = buffer[i].Event.MouseEvent;
                if (mouse.dwEventFlags & MOUSE_WHEELED) {
                    const SHORT delta = static_cast<SHORT>((mouse.dwButtonState >> 16) & 0xFFFF);
                    int steps = static_cast<int>(delta) / WHEEL_DELTA;
                    if (steps == 0) steps = (delta > 0) ? 1 : -1;
                    // Prefer scrolling the document view (Notepad-like). Fall back
                    // to console viewport scroll if already at doc bounds.
                    int before = docViewTop_;
                    docViewTop_ -= steps * 3;
                    if (docViewTop_ < 0) docViewTop_ = 0;
                    int maxTop = doc_.lineCount() - TEXT_ROWS;
                    if (maxTop < 0) maxTop = 0;
                    if (docViewTop_ > maxTop) docViewTop_ = maxTop;
                    if (docViewTop_ != before) {
                        refresh();
                    } else {
                        handleMouseWheel(mouse);
                    }
                } else {
                    handleMouseWheel(mouse);
                }
                continue;
            }
            if (buffer[i].EventType == KEY_EVENT && buffer[i].Event.KeyEvent.bKeyDown) {
                handleKey(buffer[i].Event.KeyEvent);
                if (!running_) break;
                ensureScrollableBuffer();
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
