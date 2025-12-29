#include "dbmanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>

DbManager::DbManager() {
    m_connName = "conn_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
}

DbManager::~DbManager() {
    close();
}

bool DbManager::open(const QString& path) {
    if (QSqlDatabase::contains(m_connName)) {
        m_db = QSqlDatabase::database(m_connName);
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", m_connName);
    }
    m_db.setDatabaseName(path);
    return m_db.open();
}

void DbManager::close() {
    if (m_db.isValid()) {
        m_db.close();
    }
    if (QSqlDatabase::contains(m_connName)) {
        QSqlDatabase::removeDatabase(m_connName);
    }
}

bool DbManager::isOpen() const {
    return m_db.isOpen();
}

QSqlDatabase DbManager::db() const {
    return m_db;
}

bool DbManager::ensureTables(QString* err) {
    QSqlQuery q(m_db);

    const char* ddlDept =
        "CREATE TABLE IF NOT EXISTS departments("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "depno INTEGER NOT NULL UNIQUE,"
        "name TEXT NOT NULL,"
        "parent_id INTEGER NULL"
        ");";

    if (!q.exec(ddlDept)) {
        if (err) *err = q.lastError().text();
        return false;
    }

    const char* ddlEmp =
        "CREATE TABLE IF NOT EXISTS employees("
        "no INTEGER PRIMARY KEY,"
        "name TEXT NOT NULL,"
        "depno INTEGER NOT NULL,"
        "salary REAL NOT NULL"
        ");";

    if (!q.exec(ddlEmp)) {
        if (err) *err = q.lastError().text();
        return false;
    }
    return true;
}

QVector<DeptRow> DbManager::fetchDepartments(QString* err) const {
    QVector<DeptRow> out;
    QSqlQuery q(m_db);
    if (!q.exec("SELECT id, depno, name, parent_id FROM departments ORDER BY id ASC;")) {
        if (err) *err = q.lastError().text();
        return out;
    }
    while (q.next()) {
        DeptRow r;
        r.id = q.value(0).toInt();
        r.depno = q.value(1).toInt();
        r.name = q.value(2).toString();
        r.parentId = q.value(3);
        out.push_back(r);
    }
    return out;
}

bool DbManager::countDepartments(int* outCount, QString* err) const {
    if (!outCount) return false;
    QSqlQuery q(m_db);
    if (!q.exec("SELECT COUNT(*) FROM departments;")) {
        if (err) *err = q.lastError().text();
        return false;
    }
    if (q.next()) {
        *outCount = q.value(0).toInt();
        return true;
    }
    return false;
}

bool DbManager::insertDepartment(int depno, const QString& name, const QVariant& parentId, int* outNewId, QString* err) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO departments(depno, name, parent_id) VALUES(?,?,?);");
    q.addBindValue(depno);
    q.addBindValue(name);
    if (parentId.isValid() && !parentId.isNull()) q.addBindValue(parentId);
    else q.addBindValue(QVariant(QVariant::Int)); // NULL
    if (!q.exec()) {
        if (err) *err = q.lastError().text();
        return false;
    }
    if (outNewId) *outNewId = q.lastInsertId().toInt();
    return true;
}

QVector<Emp> DbManager::fetchEmployeesByDept(int depno, QString* err) const {
    QVector<Emp> out;
    QSqlQuery q(m_db);
    q.prepare("SELECT no, name, depno, salary FROM employees WHERE depno=?;");
    q.addBindValue(depno);
    if (!q.exec()) {
        if (err) *err = q.lastError().text();
        return out;
    }
    while (q.next()) {
        Emp e;
        e.no = q.value(0).toInt();
        e.name = q.value(1).toString();
        e.depno = q.value(2).toInt();
        e.salary = q.value(3).toDouble();
        out.push_back(e);
    }
    return out;
}

QVector<Emp> DbManager::fetchAllEmployees(QString* err) const {
    QVector<Emp> out;
    QSqlQuery q(m_db);
    if (!q.exec("SELECT no, name, depno, salary FROM employees;")) {
        if (err) *err = q.lastError().text();
        return out;
    }
    while (q.next()) {
        Emp e;
        e.no = q.value(0).toInt();
        e.name = q.value(1).toString();
        e.depno = q.value(2).toInt();
        e.salary = q.value(3).toDouble();
        out.push_back(e);
    }
    return out;
}

bool DbManager::replaceAllEmployees(const QVector<Emp>& emps, QString* err) {
    if (!m_db.transaction()) {
        if (err) *err = m_db.lastError().text();
        return false;
    }

    QSqlQuery q(m_db);
    if (!q.exec("DELETE FROM employees;")) {
        m_db.rollback();
        if (err) *err = q.lastError().text();
        return false;
    }

    q.prepare("INSERT INTO employees(no,name,depno,salary) VALUES(?,?,?,?);");
    for (const auto& e : emps) {
        q.addBindValue(e.no);
        q.addBindValue(e.name);
        q.addBindValue(e.depno);
        q.addBindValue(e.salary);
        if (!q.exec()) {
            m_db.rollback();
            if (err) *err = q.lastError().text();
            return false;
        }
    }

    if (!m_db.commit()) {
        if (err) *err = m_db.lastError().text();
        return false;
    }
    return true;
}

bool DbManager::clearEmployees(QString* err) {
    QSqlQuery q(m_db);
    if (!q.exec("DELETE FROM employees;")) {
        if (err) *err = q.lastError().text();
        return false;
    }
    return true;
}
