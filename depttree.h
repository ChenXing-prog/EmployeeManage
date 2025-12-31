#ifndef DEPTTREE_H
#define DEPTTREE_H

#include <QMap>
#include <QVector>
#include <QString>
#include <QVariant>
#include <QList>

struct DeptRow {
    int id = 0;
    int depno = 0;
    QString name;
    QVariant parentId; // null 表示顶级；0 通常是“全部部门”
};

class DeptTree {
public:
    DeptTree();

    void clear();
    void buildFromRows(const QVector<DeptRow>& rows);

    // 查询
    bool containsId(int id) const;
    int depnoOf(int id) const;
    QString nameOf(int id) const;

    // ★新增：按 depno 判断是否存在部门（用于添加员工校验）
    bool containsDepno(int depno) const;

    // 树结构（用于 UI 构建）
    QList<int> childrenOf(int id) const;
    QList<int> allIds() const;

private:
    // 仍然保留节点表：id -> row
    QMap<int, DeptRow> m_nodes;

    // ✅ 孩子-兄弟表示法（内部结构）
    // firstChild[id] = 该节点的第一个孩子 id（没有则为 0 / 不存在）
    QMap<int, int> m_firstChild;

    // nextSibling[id] = 该节点的下一个兄弟 id（没有则为 0 / 不存在）
    QMap<int, int> m_nextSibling;
};

#endif // DEPTTREE_H
