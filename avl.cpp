#include "avl.h"

AvlTree::Node* AvlTree::rotateRight(Node* y) {
    Node* x = y->l;
    Node* T2 = x->r;
    x->r = y;
    y->l = T2;
    upd(y);
    upd(x);
    return x;
}

AvlTree::Node* AvlTree::rotateLeft(Node* x) {
    Node* y = x->r;
    Node* T2 = y->l;
    y->l = x;
    x->r = T2;
    upd(x);
    upd(y);
    return y;
}

AvlTree::Node* AvlTree::rebalance(Node* n) {
    upd(n);
    int b = balance(n);

    // LL
    if (b > 1 && balance(n->l) >= 0) return rotateRight(n);
    // LR
    if (b > 1 && balance(n->l) < 0) { n->l = rotateLeft(n->l); return rotateRight(n); }
    // RR
    if (b < -1 && balance(n->r) <= 0) return rotateLeft(n);
    // RL
    if (b < -1 && balance(n->r) > 0) { n->r = rotateRight(n->r); return rotateLeft(n); }

    return n;
}

AvlTree::Node* AvlTree::insertRec(Node* n, const Emp& e, bool& ok) {
    if (!n) { ok = true; return new Node(e); }
    if (e.no < n->e.no) n->l = insertRec(n->l, e, ok);
    else if (e.no > n->e.no) n->r = insertRec(n->r, e, ok);
    else { ok = false; return n; } // 重复 no

    return rebalance(n);
}

AvlTree::Node* AvlTree::minNode(Node* n) {
    Node* cur = n;
    while (cur && cur->l) cur = cur->l;
    return cur;
}

AvlTree::Node* AvlTree::removeRec(Node* n, int no, bool& ok) {
    if (!n) { ok = false; return nullptr; }

    if (no < n->e.no) n->l = removeRec(n->l, no, ok);
    else if (no > n->e.no) n->r = removeRec(n->r, no, ok);
    else {
        ok = true;
        // 0/1 child
        if (!n->l || !n->r) {
            Node* child = n->l ? n->l : n->r;
            delete n;
            return child;
        }
        // 2 children: replace with inorder successor
        Node* succ = minNode(n->r);
        n->e = succ->e;
        bool dummy = false;
        n->r = removeRec(n->r, succ->e.no, dummy);
    }

    if (!n) return nullptr;
    return rebalance(n);
}

Emp* AvlTree::findRec(Node* n, int no) {
    if (!n) return nullptr;
    if (no < n->e.no) return findRec(n->l, no);
    if (no > n->e.no) return findRec(n->r, no);
    return &n->e;
}

void AvlTree::inorderRec(Node* n, QVector<Emp>& out) {
    if (!n) return;
    inorderRec(n->l, out);
    out.push_back(n->e);
    inorderRec(n->r, out);
}

void AvlTree::freeRec(Node* n) {
    if (!n) return;
    freeRec(n->l);
    freeRec(n->r);
    delete n;
}

bool AvlTree::insert(const Emp& e) {
    bool ok = false;
    root = insertRec(root, e, ok);
    if (ok) m_size++;
    return ok;
}

bool AvlTree::remove(int no) {
    bool ok = false;
    root = removeRec(root, no, ok);
    if (ok) m_size--;
    return ok;
}

Emp* AvlTree::find(int no) {
    return findRec(root, no);
}

QVector<Emp> AvlTree::inorder() const {
    QVector<Emp> out;
    out.reserve(m_size);
    inorderRec(root, out);
    return out;
}

void AvlTree::clear() {
    freeRec(root);
    root = nullptr;
    m_size = 0;
}
