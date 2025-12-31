#include "depttree.h"

DeptTree::DeptTree() {
    clear();
}

void DeptTree::clear() {
    m_nodes.clear();
    m_firstChild.clear();
    m_nextSibling.clear();

    // 约定：id=0 是“全部部门”根
    DeptRow root;
    root.id = 0;
    root.depno = 0;
    root.name = QStringLiteral("全部部门");
    root.parentId = QVariant(); // null
    m_nodes.insert(0, root);

    // 根节点默认没有孩子、没有兄弟（不写也行）
    m_firstChild[0] = 0;
    m_nextSibling[0] = 0;
}

void DeptTree::buildFromRows(const QVector<DeptRow>& rows) {
    clear();

    // 1) 先放所有节点（节点表）
    for (const auto& r : rows) {
        if (r.id <= 0) continue;   // 保底：id 必须 >0（0 是虚拟根）
        m_nodes[r.id] = r;
        m_firstChild[r.id] = 0;
        m_nextSibling[r.id] = 0;
    }

    // 2) 用“孩子-兄弟”建立 parent-child 关系
    // 思路：对每个父节点，我们用 firstChild 指向第一个孩子，
    //      后续孩子用 nextSibling 串成链。
    //
    // 注意：你原实现 m_children.values(pid) 的“顺序”取决于插入顺序。
    // 这里我们同样按 rows 的顺序插入到兄弟链尾部，保证行为尽量一致。
    for (const auto& r : rows) {
        if (r.id <= 0) continue;

        // 计算父节点 pid
        int pid = 0;
        if (r.parentId.isValid() && !r.parentId.isNull()) {
            pid = r.parentId.toInt();
            if (!m_nodes.contains(pid)) pid = 0; // 父不存在则挂到根
        }

        // 把 r.id 挂到 pid 的孩子链上
        int childId = r.id;

        // 如果 pid 还没有 firstChild，就让它成为第一个孩子
        if (!m_firstChild.contains(pid) || m_firstChild.value(pid, 0) == 0) {
            m_firstChild[pid] = childId;
        } else {
            // 否则找到兄弟链尾部，挂上去
            int cur = m_firstChild.value(pid);
            while (cur != 0 && m_nextSibling.value(cur, 0) != 0) {
                cur = m_nextSibling.value(cur);
            }
            m_nextSibling[cur] = childId;
        }

        // 这个孩子本身的 nextSibling 默认是 0（clear/初始化里已处理）
    }

    // 3) 保险：确保根节点 firstChild 存在
    if (!m_firstChild.contains(0)) m_firstChild[0] = 0;
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
    QList<int> out;

    // 沿 firstChild + nextSibling 收集所有孩子
    QSet<int> vis;
    int cur = m_firstChild.value(id, 0);
    while (cur != 0 && !vis.contains(cur)) {
        vis.insert(cur);
        out.push_back(cur);
        cur = m_nextSibling.value(cur, 0);
    }


    return out;
}

QList<int> DeptTree::allIds() const {
    return m_nodes.keys();
}
