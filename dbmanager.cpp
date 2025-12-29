#include "dbmanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

bool DbManager::open(const QString& path, QString* err) {
    if (QSqlDatabase::contains("main_conn")) {
        m_db = QSqlDatabase::database("main_conn");
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", "main_conn");
    }
    m_db.setDatabaseName(path);

    if (!m_db.open()) {
        if (err) *err = m_db.lastError().text();
        return false;
    }
    return true;
}

void DbManager::close() {
    if (m_db.isValid() && m_db.isOpen()) m_db.close();
}

bool DbManager::initSchema(QString* err) {
    QSqlQuery q(m_db);

    const char* ddlDept =
        "CREATE TABLE IF NOT EXISTS departments("
        " depno INTEGER PRIMARY KEY,"
        " depname TEXT NOT NULL,"
        " parent INTEGER NULL"
        ");";

    const char* ddlEmp =
        "CREATE TABLE IF NOT EXISTS employees("
        " no INTEGER PRIMARY KEY,"
        " name TEXT NOT NULL,"
        " depno INTEGER NOT NULL,"
        " salary REAL NOT NULL"
        ");";

    if (!q.exec(ddlDept)) { if (err) *err = q.lastError().text(); return false; }
    if (!q.exec(ddlEmp))  { if (err) *err = q.lastError().text(); return false; }
    return true;
}

bool DbManager::seedDepartmentsIfEmpty(QString* err) {
    QSqlQuery q(m_db);
    if (!q.exec("SELECT COUNT(*) FROM departments;")) {
        if (err) *err = q.lastError().text();
        return false;
    }
    int cnt = 0;
    if (q.next()) cnt = q.value(0).toInt();
    if (cnt > 0) return true;

    // 给你一份最小可视化层级：公司0 -> 研发10/行政20/市场30 -> 子部门
    if (!m_db.transaction()) { /* ignore */ }

    auto execOne = [&](const QString& sql, const QVariantList& binds) -> bool {
        QSqlQuery ins(m_db);
        ins.prepare(sql);
        for (auto& v : binds) ins.addBindValue(v);
        if (!ins.exec()) { if (err) *err = ins.lastError().text(); return false; }
        return true;
    };

    const QString ins = "INSERT INTO departments(depno, depname, parent) VALUES(?,?,?);";

    // 根
    if (!execOne(ins, {0, "公司", QVariant()})) { m_db.rollback(); return false; }
    // 一级
    if (!execOne(ins, {10, "研发中心", 0})) { m_db.rollback(); return false; }
    if (!execOne(ins, {20, "行政中心", 0})) { m_db.rollback(); return false; }
    if (!execOne(ins, {30, "市场中心", 0})) { m_db.rollback(); return false; }
    // 二级
    if (!execOne(ins, {11, "软件部", 10})) { m_db.rollback(); return false; }
    if (!execOne(ins, {12, "测试部", 10})) { m_db.rollback(); return false; }
    if (!execOne(ins, {21, "人事部", 20})) { m_db.rollback(); return false; }
    if (!execOne(ins, {22, "财务部", 20})) { m_db.rollback(); return false; }

    if (!m_db.commit()) { if (err) *err = m_db.lastError().text(); return false; }
    return true;
}

QVector<DeptRow> DbManager::fetchDepartments(QString* err) const {
    QVector<DeptRow> out;
    QSqlQuery q(m_db);
    if (!q.exec("SELECT depno, depname, parent FROM departments;")) {
        if (err) *err = q.lastError().text();
        return out;
    }
    while (q.next()) {
        DeptRow r;
        r.depno = q.value(0).toInt();
        r.depname = q.value(1).toString();
        r.parent = q.value(2).isNull() ? -1 : q.value(2).toInt();
        out.push_back(r);
    }
    return out;
}

QVector<EmpRow> DbManager::fetchEmployees(QString* err) const {
    QVector<EmpRow> out;
    QSqlQuery q(m_db);
    if (!q.exec("SELECT no, name, depno, salary FROM employees;")) {
        if (err) *err = q.lastError().text();
        return out;
    }
    while (q.next()) {
        EmpRow r;
        r.no = q.value(0).toInt();
        r.name = q.value(1).toString();
        r.depno = q.value(2).toInt();
        r.salary = q.value(3).toDouble();
        out.push_back(r);
    }
    return out;
}

bool DbManager::replaceAllEmployees(const QVector<EmpRow>& rows, QString* err) {
    if (!m_db.transaction()) {
        if (err) *err = m_db.lastError().text();
        return false;
    }

    QSqlQuery del(m_db);
    if (!del.exec("DELETE FROM employees;")) {
        if (err) *err = del.lastError().text();
        m_db.rollback();
        return false;
    }

    QSqlQuery ins(m_db);
    ins.prepare("INSERT INTO employees(no,name,depno,salary) VALUES(?,?,?,?);");

    for (const auto& r : rows) {
        ins.addBindValue(r.no);
        ins.addBindValue(r.name);
        ins.addBindValue(r.depno);
        ins.addBindValue(r.salary);
        if (!ins.exec()) {
            if (err) *err = ins.lastError().text();
            m_db.rollback();
            return false;
        }
        ins.finish();
    }

    if (!m_db.commit()) {
        if (err) *err = m_db.lastError().text();
        return false;
    }
    return true;
}
