#include "depttree.h"

DeptTree::DepNode* DeptTree::makeNode(int depno, const QString& name) {
    DepNode* n = new DepNode;
    n->depno = depno;
    n->name = name;
    m_all.push_back(n);
    return n;
}

void DeptTree::linkChild(DepNode* parent, DepNode* child) {
    child->parent = parent;
    // 插到孩子链表头（你也可以改成按 depno 有序插入）
    child->nextSibling = parent->firstChild;
    parent->firstChild = child;
}

void DeptTree::clear() {
    for (auto* n : m_all) delete n;
    m_all.clear();
    m_root = nullptr;
}

bool DeptTree::build(const QVector<DeptRow>& depts, QString* err) {
    clear();
    if (depts.isEmpty()) {
        if (err) *err = "departments 表为空";
        return false;
    }

    // 先创建所有节点
    // 注意：这里不用 map/unordered_map（后面要加索引我再按你“自写结构”来写）
    QVector<DepNode*> nodes;
    nodes.reserve(depts.size());
    for (const auto& d : depts) {
        nodes.push_back(makeNode(d.depno, d.depname));
    }

    auto findByDepno = [&](int depno) -> DepNode* {
        for (auto* n : nodes) if (n->depno == depno) return n;
        return nullptr;
    };

    // 再连边（parent-child）
    DepNode* rootCandidate = nullptr;
    for (const auto& d : depts) {
        DepNode* cur = findByDepno(d.depno);
        if (!cur) continue;

        if (d.parent < 0) {
            // parent = NULL
            if (!rootCandidate) rootCandidate = cur;
        } else {
            DepNode* p = findByDepno(d.parent);
            if (p) linkChild(p, cur);
        }
    }

    if (!rootCandidate) {
        // 若没有 NULL parent，就找 depno=0 当根
        rootCandidate = findByDepno(0);
    }

    if (!rootCandidate) {
        if (err) *err = "找不到根部门（parent NULL 或 depno=0）";
        return false;
    }

    m_root = rootCandidate;
    return true;
}

void DeptTree::loadEmployees(const QVector<EmpRow>& emps) {
    // 先清空每个部门 AVL
    for (auto* n : m_all) n->employees.clear();

    for (const auto& r : emps) {
        DepNode* d = findDep(r.depno);
        if (!d) continue;
        Emp e;
        e.no = r.no;
        e.name = r.name;
        e.depno = r.depno;
        e.salary = r.salary;
        d->employees.insert(e); // no 重复会被拒绝（DB 本身 no 是主键）
    }
}

DeptTree::DepNode* DeptTree::findRec(DepNode* cur, int depno) const {
    if (!cur) return nullptr;
    if (cur->depno == depno) return cur;
    for (DepNode* ch = cur->firstChild; ch; ch = ch->nextSibling) {
        if (auto* hit = findRec(ch, depno)) return hit;
    }
    return nullptr;
}

DeptTree::DepNode* DeptTree::findDep(int depno) const {
    return findRec(m_root, depno);
}

void DeptTree::collectRec(DepNode* cur, QSet<int>& s) const {
    if (!cur) return;
    s.insert(cur->depno);
    for (DepNode* ch = cur->firstChild; ch; ch = ch->nextSibling) {
        collectRec(ch, s);
    }
}

QSet<int> DeptTree::collectSubtreeDepnos(int depno) const {
    QSet<int> s;
    DepNode* d = findDep(depno);
    collectRec(d, s);
    return s;
}
