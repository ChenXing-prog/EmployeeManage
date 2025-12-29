#pragma once
#include <QString>
#include <QVector>

struct EmployeeNode {
    int no = 0;
    QString name;
    int depno = 0;
    double salary = 0.0;

    EmployeeNode* next = nullptr;     // 主链
    EmployeeNode* pno = nullptr;      // 按 no 链
    EmployeeNode* pdepno = nullptr;   // 按 depno 链
    EmployeeNode* psalary = nullptr;  // 按 salary 链
};

class EmployeeManager {
public:
    EmployeeManager();
    ~EmployeeManager();

    void clearAllNodes();                 // 仅清内存链表
    void loadFromRows(const QVector<EmployeeNode>& rows); // 从“数据行”建立主链

    // 主链输出
    QStringList dumpByNext() const;

    // 构建三条排序指针链（不改 next）
    void buildByNo();
    void buildByDepno();
    void buildBySalary();

    QStringList dumpByPno() const;
    QStringList dumpByPdepno() const;
    QStringList dumpByPsalary() const;

    // 取主链所有数据（用于全量写回DB）
    QVector<EmployeeNode> toVectorByNext() const;

private:
    EmployeeNode* head = nullptr;

    // 三个“排序链头”
    EmployeeNode* headNo = nullptr;
    EmployeeNode* headDepno = nullptr;
    EmployeeNode* headSalary = nullptr;

    void resetSortPointers();
    static QString formatLine(const EmployeeNode* e);
};
