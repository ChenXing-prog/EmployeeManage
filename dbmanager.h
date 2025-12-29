#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QSqlDatabase>
#include <QVector>
#include <QString>
#include <QVariant>

#include "avl.h"       // Emp
#include "depttree.h"  // DeptRow

class DbManager {
public:
    DbManager();
    ~DbManager();

    bool open(const QString& path);
    void close();
    bool isOpen() const;
    QSqlDatabase db() const;

    // 建表
    bool ensureTables(QString* err = nullptr);

    // 部门
    QVector<DeptRow> fetchDepartments(QString* err = nullptr) const;
    bool insertDepartment(int depno, const QString& name, const QVariant& parentId, int* outNewId, QString* err = nullptr);
    bool countDepartments(int* outCount, QString* err = nullptr) const;

    // 员工（旧接口保留不用也行）
    QVector<Emp> fetchEmployeesByDept(int depno, QString* err = nullptr) const;

    // ★新增：一次性加载全部员工
    QVector<Emp> fetchAllEmployees(QString* err = nullptr) const;

    // ★新增：用内存主数据全量写回 DB（持久化）
    bool replaceAllEmployees(const QVector<Emp>& emps, QString* err = nullptr);

    bool clearEmployees(QString* err = nullptr);

private:
    QSqlDatabase m_db;
    QString m_connName;
};

#endif // DBMANAGER_H
