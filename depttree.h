#pragma once
#include <QString>
#include <QVector>
#include <QSet>
#include "avl.h"
#include "dbmanager.h"

class DeptTree {
public:
    struct DepNode {
        int depno;
        QString name;

        DepNode* parent = nullptr;
        DepNode* firstChild = nullptr;
        DepNode* nextSibling = nullptr;

        AvlTree employees;   // 本部门员工 AVL（按 no）
    };

    DeptTree() = default;
    ~DeptTree() { clear(); }

    void clear();

    // 从 DB 构建部门树（departments 表）
    bool build(const QVector<DeptRow>& depts, QString* err = nullptr);

    // 把 employees 装入对应部门的 AVL
    void loadEmployees(const QVector<EmpRow>& emps);

    DepNode* root() const { return m_root; }
    DepNode* findDep(int depno) const;  // 简单 DFS（够用，后面可加索引）

    // 获取某部门子树的 depno 集合（用于“包含子部门”筛选）
    QSet<int> collectSubtreeDepnos(int depno) const;

private:
    DepNode* m_root = nullptr;
    QVector<DepNode*> m_all; // 便于释放

    DepNode* makeNode(int depno, const QString& name);
    void linkChild(DepNode* parent, DepNode* child);

    DepNode* findRec(DepNode* cur, int depno) const;
    void collectRec(DepNode* cur, QSet<int>& s) const;
};
