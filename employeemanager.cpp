#include "employeemanager.h"
#include <QStringList>

EmployeeManager::EmployeeManager() {}

EmployeeManager::~EmployeeManager() {
    clearAllNodes();
}

void EmployeeManager::clearAllNodes() {
    EmployeeNode* cur = head;
    while (cur) {
        EmployeeNode* nxt = cur->next;
        delete cur;
        cur = nxt;
    }
    head = nullptr;
    headNo = headDepno = headSalary = nullptr;
}

void EmployeeManager::loadFromRows(const QVector<EmployeeNode>& rows) {
    clearAllNodes();

    EmployeeNode* tail = nullptr;
    for (const auto& r : rows) {
        auto* node = new EmployeeNode;
        node->no = r.no;
        node->name = r.name;
        node->depno = r.depno;
        node->salary = r.salary;

        if (!head) { head = node; tail = node; }
        else { tail->next = node; tail = node; }
    }

    resetSortPointers();
}

void EmployeeManager::resetSortPointers() {
    headNo = headDepno = headSalary = nullptr;
    for (auto* cur = head; cur; cur = cur->next) {
        cur->pno = nullptr;
        cur->pdepno = nullptr;
        cur->psalary = nullptr;
    }
}

QString EmployeeManager::formatLine(const EmployeeNode* e) {
    return QString("no=%1 | name=%2 | depno=%3 | salary=%4")
    .arg(e->no)
        .arg(e->name)
        .arg(e->depno)
        .arg(QString::number(e->salary, 'f', 2));
}

QStringList EmployeeManager::dumpByNext() const {
    QStringList out;
    for (auto* cur = head; cur; cur = cur->next) out << formatLine(cur);
    return out;
}

void EmployeeManager::buildByNo() {
    resetSortPointers();
    EmployeeNode* sorted = nullptr;

    for (auto* cur = head; cur; cur = cur->next) {
        auto* node = cur;

        if (!sorted || node->no < sorted->no) {
            node->pno = sorted;
            sorted = node;
        } else {
            EmployeeNode* p = sorted;
            while (p->pno && p->pno->no <= node->no) p = p->pno;
            node->pno = p->pno;
            p->pno = node;
        }
    }
    headNo = sorted;
}

void EmployeeManager::buildByDepno() {
    resetSortPointers();
    EmployeeNode* sorted = nullptr;

    for (auto* cur = head; cur; cur = cur->next) {
        auto* node = cur;

        auto less = [](EmployeeNode* a, EmployeeNode* b){
            if (a->depno != b->depno) return a->depno < b->depno;
            return a->no < b->no;
        };

        if (!sorted || less(node, sorted)) {
            node->pdepno = sorted;
            sorted = node;
        } else {
            EmployeeNode* p = sorted;
            while (p->pdepno && !less(node, p->pdepno)) p = p->pdepno;
            node->pdepno = p->pdepno;
            p->pdepno = node;
        }
    }
    headDepno = sorted;
}

void EmployeeManager::buildBySalary() {
    resetSortPointers();
    EmployeeNode* sorted = nullptr;

    for (auto* cur = head; cur; cur = cur->next) {
        auto* node = cur;

        auto less = [](EmployeeNode* a, EmployeeNode* b){
            if (a->salary != b->salary) return a->salary < b->salary;
            return a->no < b->no;
        };

        if (!sorted || less(node, sorted)) {
            node->psalary = sorted;
            sorted = node;
        } else {
            EmployeeNode* p = sorted;
            while (p->psalary && !less(node, p->psalary)) p = p->psalary;
            node->psalary = p->psalary;
            p->psalary = node;
        }
    }
    headSalary = sorted;
}

QStringList EmployeeManager::dumpByPno() const {
    QStringList out;
    for (auto* cur = headNo; cur; cur = cur->pno) out << formatLine(cur);
    return out;
}

QStringList EmployeeManager::dumpByPdepno() const {
    QStringList out;
    for (auto* cur = headDepno; cur; cur = cur->pdepno) out << formatLine(cur);
    return out;
}

QStringList EmployeeManager::dumpByPsalary() const {
    QStringList out;
    for (auto* cur = headSalary; cur; cur = cur->psalary) out << formatLine(cur);
    return out;
}

QVector<EmployeeNode> EmployeeManager::toVectorByNext() const {
    QVector<EmployeeNode> v;
    for (auto* cur = head; cur; cur = cur->next) {
        EmployeeNode t;
        t.no = cur->no;
        t.name = cur->name;
        t.depno = cur->depno;
        t.salary = cur->salary;
        v.push_back(t);
    }
    return v;
}
