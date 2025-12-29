#include "mainwindow.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QHeaderView>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>

#include <QSqlQuery>
#include <QSqlError>

#include<QstatusBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    buildUi();   // 先把界面搭出来
    initDb();    // 再连数据库
    loadDepts();
    loadEmployees(-1);
}

MainWindow::~MainWindow() {
    if (db.isOpen()) db.close();
}

void MainWindow::buildUi() {
    setWindowTitle("EmployeeManage - 部门树 + 员工表（Qt + SQLite）");
    resize(1100, 700);

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *root = new QHBoxLayout(central);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(10);

    // 左侧：部门树
    auto *leftBox = new QGroupBox("部门树", central);
    auto *leftLay = new QVBoxLayout(leftBox);

    treeDepts = new QTreeWidget(leftBox);
    treeDepts->setHeaderLabel("部门");
    treeDepts->setMinimumWidth(260);
    leftLay->addWidget(treeDepts);

    root->addWidget(leftBox, 1);

    // 右侧：上表格 + 下表单/按钮
    auto *rightBox = new QGroupBox("员工列表", central);
    auto *rightLay = new QVBoxLayout(rightBox);

    tableEmps = new QTableWidget(rightBox);
    tableEmps->setColumnCount(4);
    tableEmps->setHorizontalHeaderLabels({"no", "name", "depno", "salary"});
    tableEmps->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableEmps->setSelectionMode(QAbstractItemView::SingleSelection);
    tableEmps->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableEmps->horizontalHeader()->setStretchLastSection(true);
    tableEmps->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    rightLay->addWidget(tableEmps, 5);

    // 表单区
    auto *formBox = new QGroupBox("新增 / 修改", rightBox);
    auto *formLay = new QVBoxLayout(formBox);

    auto *form = new QFormLayout();
    editNo = new QLineEdit(formBox);
    editName = new QLineEdit(formBox);
    editDepno = new QLineEdit(formBox);
    editSalary = new QLineEdit(formBox);

    editNo->setPlaceholderText("整数，例如 1001");
    editName->setPlaceholderText("姓名，例如 张三");
    editDepno->setPlaceholderText("部门号整数，例如 2");
    editSalary->setPlaceholderText("工资，例如 6500.50");

    form->addRow("no(工号):", editNo);
    form->addRow("name(姓名):", editName);
    form->addRow("depno(部门号):", editDepno);
    form->addRow("salary(工资):", editSalary);
    formLay->addLayout(form);

    auto *btnRow = new QHBoxLayout();
    auto *btnAdd = new QPushButton("添加", formBox);
    auto *btnUpd = new QPushButton("修改(按no)", formBox);
    auto *btnDel = new QPushButton("删除(选中行)", formBox);
    auto *btnReload = new QPushButton("刷新", formBox);
    auto *btnClear = new QPushButton("全清(DB)", formBox);

    btnRow->addWidget(btnAdd);
    btnRow->addWidget(btnUpd);
    btnRow->addWidget(btnDel);
    btnRow->addWidget(btnReload);
    btnRow->addWidget(btnClear);
    formLay->addLayout(btnRow);

    rightLay->addWidget(formBox, 2);
    root->addWidget(rightBox, 3);

    // 状态栏
    statusLabel = new QLabel("就绪", this);
    statusBar()->addWidget(statusLabel);

    // 信号连接
    connect(treeDepts, &QTreeWidget::itemSelectionChanged,
            this, &MainWindow::onDeptSelectionChanged);

    connect(tableEmps, &QTableWidget::itemSelectionChanged, this, [this](){
        fillEditsFromCurrentRow();
    });

    connect(btnAdd, &QPushButton::clicked, this, &MainWindow::addEmployee);
    connect(btnUpd, &QPushButton::clicked, this, &MainWindow::updateEmployee);
    connect(btnDel, &QPushButton::clicked, this, &MainWindow::deleteEmployee);
    connect(btnReload, &QPushButton::clicked, this, &MainWindow::reloadAll);
    connect(btnClear, &QPushButton::clicked, this, &MainWindow::clearAll);
}

QString MainWindow::dbPath() const {
    // 放到用户目录下的 Documents/EmployeeManage/employee.db
    const QString base = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QDir dir(base);
    if (!dir.exists("EmployeeManage")) dir.mkdir("EmployeeManage");
    return dir.filePath("EmployeeManage/employee.db");
}

void MainWindow::initDb() {
    // QSQLITE 驱动
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath());

    if (!db.open()) {
        QMessageBox::critical(this, "DB错误", "无法打开SQLite数据库:\n" + db.lastError().text());
        return;
    }
    createTablesIfNeeded();
    statusLabel->setText("DB: " + db.databaseName());
}

void MainWindow::createTablesIfNeeded() {
    QSqlQuery q(db);

    // employees
    if (!q.exec(
            "CREATE TABLE IF NOT EXISTS employees("
            "no INTEGER PRIMARY KEY,"
            "name TEXT NOT NULL,"
            "depno INTEGER NOT NULL,"
            "salary REAL NOT NULL"
            ");"
            )) {
        QMessageBox::critical(this, "建表失败", q.lastError().text());
        return;
    }

    // depts：简单部门表（你后面要“部门树”可以扩展 parent_id）
    if (!q.exec(
            "CREATE TABLE IF NOT EXISTS depts("
            "depno INTEGER PRIMARY KEY,"
            "depname TEXT NOT NULL,"
            "parent_depno INTEGER DEFAULT NULL"
            ");"
            )) {
        QMessageBox::critical(this, "建表失败", q.lastError().text());
        return;
    }

    // 如果没有部门，插入几个默认部门
    if (!q.exec("SELECT COUNT(*) FROM depts;")) return;
    if (q.next() && q.value(0).toInt() == 0) {
        q.exec("INSERT INTO depts(depno, depname, parent_depno) VALUES (1,'行政',NULL);");
        q.exec("INSERT INTO depts(depno, depname, parent_depno) VALUES (2,'研发',NULL);");
        q.exec("INSERT INTO depts(depno, depname, parent_depno) VALUES (3,'财务',NULL);");
        q.exec("INSERT INTO depts(depno, depname, parent_depno) VALUES (4,'研发-后端',2);");
        q.exec("INSERT INTO depts(depno, depname, parent_depno) VALUES (5,'研发-前端',2);");
    }
}

void MainWindow::loadDepts() {
    treeDepts->clear();

    // 先做一个“全部”
    auto *all = new QTreeWidgetItem(treeDepts);
    all->setText(0, "全部部门");
    all->setData(0, Qt::UserRole, -1);

    // 读取部门表，用 parent_depno 组树
    QSqlQuery q(db);
    if (!q.exec("SELECT depno, depname, parent_depno FROM depts ORDER BY depno;")) {
        QMessageBox::warning(this, "查询失败", q.lastError().text());
        return;
    }

    // 简单做法：先存起来，再二次挂载
    struct DeptRow { int depno; QString name; QVariant parent; };
    QList<DeptRow> rows;
    while (q.next()) {
        rows.push_back({ q.value(0).toInt(), q.value(1).toString(), q.value(2) });
    }

    QMap<int, QTreeWidgetItem*> itemMap;

    // 先创建所有 item（先不挂）
    for (auto &r : rows) {
        auto *it = new QTreeWidgetItem();
        it->setText(0, QString("%1 - %2").arg(r.depno).arg(r.name));
        it->setData(0, Qt::UserRole, r.depno);
        itemMap[r.depno] = it;
    }

    // 再挂到父节点（没有父就挂到 all）
    for (auto &r : rows) {
        QTreeWidgetItem *it = itemMap[r.depno];
        if (r.parent.isNull()) {
            all->addChild(it);
        } else {
            int p = r.parent.toInt();
            if (itemMap.contains(p)) itemMap[p]->addChild(it);
            else all->addChild(it);
        }
    }

    treeDepts->expandAll();
    treeDepts->setCurrentItem(all);
}

int MainWindow::currentSelectedDepno() const {
    auto items = treeDepts->selectedItems();
    if (items.isEmpty()) return -1;
    return items.first()->data(0, Qt::UserRole).toInt();
}

void MainWindow::loadEmployees(int depnoFilter) {
    tableEmps->setRowCount(0);

    QSqlQuery q(db);
    if (depnoFilter < 0) {
        if (!q.exec("SELECT no,name,depno,salary FROM employees ORDER BY no;")) {
            QMessageBox::warning(this, "查询失败", q.lastError().text());
            return;
        }
    } else {
        q.prepare("SELECT no,name,depno,salary FROM employees WHERE depno=? ORDER BY no;");
        q.addBindValue(depnoFilter);
        if (!q.exec()) {
            QMessageBox::warning(this, "查询失败", q.lastError().text());
            return;
        }
    }

    int row = 0;
    while (q.next()) {
        tableEmps->insertRow(row);
        tableEmps->setItem(row, 0, new QTableWidgetItem(q.value(0).toString()));
        tableEmps->setItem(row, 1, new QTableWidgetItem(q.value(1).toString()));
        tableEmps->setItem(row, 2, new QTableWidgetItem(q.value(2).toString()));
        tableEmps->setItem(row, 3, new QTableWidgetItem(q.value(3).toString()));
        row++;
    }

    statusLabel->setText(QString("已加载 %1 条员工记录").arg(row));
}

void MainWindow::onDeptSelectionChanged() {
    int dep = currentSelectedDepno();
    loadEmployees(dep);
}

void MainWindow::fillEditsFromCurrentRow() {
    int r = tableEmps->currentRow();
    if (r < 0) return;
    editNo->setText(tableEmps->item(r,0)->text());
    editName->setText(tableEmps->item(r,1)->text());
    editDepno->setText(tableEmps->item(r,2)->text());
    editSalary->setText(tableEmps->item(r,3)->text());
}

void MainWindow::addEmployee() {
    bool ok1=false, ok2=false;
    int no = editNo->text().trimmed().toInt(&ok1);
    int depno = editDepno->text().trimmed().toInt(&ok2);
    double salary = editSalary->text().trimmed().toDouble();

    QString name = editName->text().trimmed();
    if (!ok1 || !ok2 || name.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请检查 no/depno/name 是否正确。");
        return;
    }

    QSqlQuery q(db);
    q.prepare("INSERT INTO employees(no,name,depno,salary) VALUES(?,?,?,?);");
    q.addBindValue(no);
    q.addBindValue(name);
    q.addBindValue(depno);
    q.addBindValue(salary);

    if (!q.exec()) {
        QMessageBox::warning(this, "添加失败", q.lastError().text());
        return;
    }
    reloadAll();
}

void MainWindow::updateEmployee() {
    bool ok1=false, ok2=false;
    int no = editNo->text().trimmed().toInt(&ok1);
    int depno = editDepno->text().trimmed().toInt(&ok2);
    double salary = editSalary->text().trimmed().toDouble();
    QString name = editName->text().trimmed();

    if (!ok1 || !ok2 || name.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请检查 no/depno/name 是否正确。");
        return;
    }

    QSqlQuery q(db);
    q.prepare("UPDATE employees SET name=?, depno=?, salary=? WHERE no=?;");
    q.addBindValue(name);
    q.addBindValue(depno);
    q.addBindValue(salary);
    q.addBindValue(no);

    if (!q.exec()) {
        QMessageBox::warning(this, "修改失败", q.lastError().text());
        return;
    }
    if (q.numRowsAffected() == 0) {
        QMessageBox::information(this, "提示", "没有找到该 no 的记录。");
    }
    reloadAll();
}

void MainWindow::deleteEmployee() {
    int r = tableEmps->currentRow();
    if (r < 0) {
        QMessageBox::information(this, "提示", "请先选中一行。");
        return;
    }
    int no = tableEmps->item(r,0)->text().toInt();

    QSqlQuery q(db);
    q.prepare("DELETE FROM employees WHERE no=?;");
    q.addBindValue(no);
    if (!q.exec()) {
        QMessageBox::warning(this, "删除失败", q.lastError().text());
        return;
    }
    reloadAll();
}

void MainWindow::reloadAll() {
    loadDepts(); // 部门表可能变了（你后面会加部门管理）
    int dep = currentSelectedDepno();
    loadEmployees(dep);
}

void MainWindow::clearAll() {
    if (QMessageBox::question(this, "确认", "确定要清空全部 employees 记录吗？") != QMessageBox::Yes)
        return;

    QSqlQuery q(db);
    if (!q.exec("DELETE FROM employees;")) {
        QMessageBox::warning(this, "清空失败", q.lastError().text());
        return;
    }
    reloadAll();
}
