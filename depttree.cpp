#include "depttree.h"

DeptTree::DeptTree() {
    clear();
}

void DeptTree::clear() {
    m_nodes.clear();
    m_children.clear();

    // 约定：id=0 是“全部部门”根
    DeptRow root;
    root.id = 0;
    root.depno = 0;
    root.name = QStringLiteral("全部部门");
    root.parentId = QVariant(); // null
    m_nodes.insert(0, root);
}

void DeptTree::buildFromRows(const QVector<DeptRow>& rows) {
    clear();

    // 先放所有节点
    for (const auto& r : rows) {
        m_nodes[r.id] = r;
    }

    // 再建 parent-child 关系
    for (const auto& r : rows) {
        int pid = 0;
        if (r.parentId.isValid() && !r.parentId.isNull()) {
            pid = r.parentId.toInt();
            if (!m_nodes.contains(pid)) pid = 0;
        }
        m_children.insert(pid, r.id);
    }
}

bool DeptTree::containsId(int id) const {
    return m_nodes.contains(id);
}

int DeptTree::depnoOf(int id) const {
    return m_nodes.contains(id) ? m_nodes[id].depno : 0;
}

QString DeptTree::nameOf(int id) const {
    return m_nodes.contains(id) ? m_nodes[id].name : QString();
}

bool DeptTree::containsDepno(int depno) const {
    if (depno <= 0) return false;
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        if (it.key() == 0) continue; // 跳过“全部部门”
        if (it.value().depno == depno) return true;
    }
    return false;
}

QList<int> DeptTree::childrenOf(int id) const {
    return m_children.values(id);
}

QList<int> DeptTree::allIds() const {
    return m_nodes.keys();
}
