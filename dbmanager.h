#pragma once
#include <QString>
#include <QSqlDatabase>
#include <QVector>

struct DeptRow {
    int depno;
    QString depname;
    int parent;          // -1 表示 NULL
};

struct EmpRow {
    int no;
    QString name;
    int depno;
    double salary;
};

class DbManager {
public:
    bool open(const QString& path, QString* err = nullptr);
    void close();

    bool initSchema(QString* err = nullptr);
    bool seedDepartmentsIfEmpty(QString* err = nullptr);

    QVector<DeptRow> fetchDepartments(QString* err = nullptr) const;
    QVector<EmpRow>  fetchEmployees(QString* err = nullptr) const;

    // 先给你“写回”的接口，后面增删改/Undo/Redo/TopK 会用到
    bool replaceAllEmployees(const QVector<EmpRow>& rows, QString* err = nullptr);

    QSqlDatabase db() const { return m_db; }

private:
    QSqlDatabase m_db;
};
