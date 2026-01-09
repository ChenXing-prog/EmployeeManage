#pragma once
#include <QString>
#include <QVector>
#include <algorithm>

struct Emp {
    int no;
    QString name;
    int depno;
    double salary;
};

class AvlTree {
public:
    AvlTree() = default;
    ~AvlTree() { clear(); }

    AvlTree(const AvlTree&) = delete;
    AvlTree& operator=(const AvlTree&) = delete;

    bool insert(const Emp& e);
    bool remove(int no);
    Emp* find(int no);

    QVector<Emp> inorder() const;
    void clear();

    int size() const { return m_size; }

private:
    struct Node {
        Emp e;
        Node* l = nullptr;
        Node* r = nullptr;
        int h = 1;
        Node(const Emp& x): e(x) {}
    };

    Node* root = nullptr;
    int m_size = 0;

    static int height(Node* n) { return n ? n->h : 0; }
    static int balance(Node* n) { return n ? height(n->l) - height(n->r) : 0; }
    static void upd(Node* n) {
        if (!n) return;
        n->h = std::max(height(n->l), height(n->r)) + 1;
    }

    static Node* rotateRight(Node* y);
    static Node* rotateLeft(Node* x);
    static Node* rebalance(Node* n);

    static Node* insertRec(Node* n, const Emp& e, bool& ok);
    static Node* minNode(Node* n);
    static Node* removeRec(Node* n, int no, bool& ok);

    static Emp* findRec(Node* n, int no);
    static void inorderRec(Node* n, QVector<Emp>& out);
    static void freeRec(Node* n);
};
