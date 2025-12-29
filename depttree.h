#ifndef DEPTTREE_H
#define DEPTTREE_H

#include <QMap>
#include <QVector>
#include <QString>
#include <QVariant>

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
    QMap<int, DeptRow> m_nodes;        // id -> row
    QMultiMap<int, int> m_children;    // parentId -> childId
};

#endif // DEPTTREE_H
