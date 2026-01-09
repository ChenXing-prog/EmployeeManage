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
    QVariant parentId;
};

class DeptTree {
public:
    DeptTree();

    void clear();
    void buildFromRows(const QVector<DeptRow>& rows);

    //查询
    bool containsId(int id) const;
    int depnoOf(int id) const;
    QString nameOf(int id) const;

    //按depno 判断是否存在部门
    bool containsDepno(int depno) const;

    //树结构
    QList<int> childrenOf(int id) const;
    QList<int> allIds() const;

private:

    QMap<int, DeptRow> m_nodes;

    QMap<int, int> m_firstChild;

    QMap<int, int> m_nextSibling;
};

#endif
