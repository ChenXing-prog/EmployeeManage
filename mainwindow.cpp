#include "mainwindow.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QStatusBar>
#include <QStandardPaths>
#include <QDir>

#include <QSqlQuery>
#include <QSqlError>

static constexpr int COL_NO     = 0;
static constexpr int COL_NAME   = 1;
static constexpr int COL_DEPNO  = 2;
static constexpr int COL_SALARY = 3;

static constexpr int ROLE_DEPT_ID   = Qt::UserRole + 1; // departments.id
static constexpr int ROLE_DEPT_DEPNO= Qt::UserRole + 2; // departments.depno

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {

    buildUi();
    initDb();
    seedDefaultDepartmentsIfEmpty();
    loadDeptsToTree(0);
    refreshEmployeesByDeptSelection();
}

MainWindow::~MainWindow() {
    if (db.isOpen()) db.close();
}

// ---------------- UI ----------------
void MainWindow::buildUi() {
    setWindowTitle("EmployeeManage - 部门树 + 员工表 (Qt + SQLite)");
    resize(1200, 720);

    statusLabel = new QLabel(this);
    statusBar()->addWidget(statusLabel);

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *root = new QHBoxLayout(central);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(10);

    auto *splitter = new QSplitter(Qt::Horizontal, central);
    root->addWidget(splitter);

    // ============ Left: 部门树 + 新增部门 ============
    auto *left = new QWidget(splitter);
    auto *leftLay = new QVBoxLayout(left);
    leftLay->setContentsMargins(0,0,0,0);
    leftLay->setSpacing(10);

    auto *gbTree = new QGroupBox("部门树", left);
    auto *gbTreeLay = new QVBoxLayout(gbTree);

    treeDepts = new QTreeWidget(gbTree);
    treeDepts->setHeaderLabel("部门");
    treeDepts->setSelectionMode(QAbstractItemView::SingleSelection);
    gbTreeLay->addWidget(treeDepts);

    leftLay->addWidget(gbTree, 1);

    auto *gbAddDept = new QGroupBox("新增部门", left);
    auto *addDeptLay = new QVBoxLayout(gbAddDept);

    auto *deptForm = new QFormLayout();
    editDeptNo = new QLineEdit(gbAddDept);
    editDeptName = new QLineEdit(gbAddDept);
    editDeptNo->setPlaceholderText("整数，例如 6");
    editDeptName->setPlaceholderText("部门名称，例如 管理");

    deptForm->addRow("depno(部门编号):", editDeptNo);
    deptForm->addRow("name(部门名):", editDeptName);
    addDeptLay->addLayout(deptForm);

    auto *deptBtnRow = new QHBoxLayout();
    btnAddDeptTop = new QPushButton("添加为顶级部门", gbAddDept);
    btnAddDeptChild = new QPushButton("添加为选中部门子部门", gbAddDept);
    deptBtnRow->addWidget(btnAddDeptTop);
    deptBtnRow->addWidget(btnAddDeptChild);
    addDeptLay->addLayout(deptBtnRow);

    leftLay->addWidget(gbAddDept, 0);

    splitter->addWidget(left);

    // ============ Right: 员工列表 + 员工操作 ============
    auto *right = new QWidget(splitter);
    auto *rightLay = new QVBoxLayout(right);
    rightLay->setContentsMargins(0,0,0,0);
    rightLay->setSpacing(10);

    auto *gbEmps = new QGroupBox("员工列表", right);
    auto *gbEmpsLay = new QVBoxLayout(gbEmps);

    tableEmps = new QTableWidget(gbEmps);
    tableEmps->setColumnCount(4);
    tableEmps->setHorizontalHeaderLabels({"no", "name", "depno", "salary"});
    tableEmps->horizontalHeader()->setStretchLastSection(true);
    tableEmps->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableEmps->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableEmps->setSelectionMode(QAbstractItemView::SingleSelection);
    tableEmps->setEditTriggers(QAbstractItemView::NoEditTriggers);
    gbEmpsLay->addWidget(tableEmps);

    rightLay->addWidget(gbEmps, 1);

    auto *gbEdit = new QGroupBox("新增 / 修改", right);
    auto *gbEditLay = new QVBoxLayout(gbEdit);

    auto *empForm = new QFormLayout();
    editNo = new QLineEdit(gbEdit);
    editName = new QLineEdit(gbEdit);
    editDepno = new QLineEdit(gbEdit);
    editSalary = new QLineEdit(gbEdit);

    editNo->setPlaceholderText("整数，例如 1001");
    editName->setPlaceholderText("姓名，例如 张三");
    editDepno->setPlaceholderText("部门号整数，例如 2");
    editSalary->setPlaceholderText("工资，例如 6500.5");

    empForm->addRow("no(工号):", editNo);
    empForm->addRow("name(姓名):", editName);
    empForm->addRow("depno(部门号):", editDepno);
    empForm->addRow("salary(工资):", editSalary);
    gbEditLay->addLayout(empForm);

    auto *empBtnRow = new QHBoxLayout();
    btnAddEmp = new QPushButton("添加", gbEdit);
    btnUpdateEmp = new QPushButton("修改(按no)", gbEdit);
    btnDeleteEmp = new QPushButton("删除(选中行)", gbEdit);
    btnRefreshEmp = new QPushButton("刷新", gbEdit);
    btnClearEmp = new QPushButton("全清(DB)", gbEdit);

    empBtnRow->addWidget(btnAddEmp);
    empBtnRow->addWidget(btnUpdateEmp);
    empBtnRow->addWidget(btnDeleteEmp);
    empBtnRow->addWidget(btnRefreshEmp);
    empBtnRow->addWidget(btnClearEmp);
    gbEditLay->addLayout(empBtnRow);

    rightLay->addWidget(gbEdit, 0);

    splitter->addWidget(right);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);

    // -------- signals --------
    connect(btnAddDeptTop,   &QPushButton::clicked, this, &MainWindow::addDeptAsTop);
    connect(btnAddDeptChild, &QPushButton::clicked, this, &MainWindow::addDeptAsChild);
    connect(treeDepts, &QTreeWidget::itemSelectionChanged, this, &MainWindow::onDeptSelectionChanged);

    connect(btnAddEmp,    &QPushButton::clicked, this, &MainWindow::addEmployee);
    connect(btnUpdateEmp, &QPushButton::clicked, this, &MainWindow::updateEmployeeByNo);
    connect(btnDeleteEmp, &QPushButton::clicked, this, &MainWindow::deleteSelectedEmployee);
    connect(btnRefreshEmp,&QPushButton::clicked, this, &MainWindow::refreshEmployees);
    connect(btnClearEmp,  &QPushButton::clicked, this, &MainWindow::clearAllEmployees);

    connect(tableEmps, &QTableWidget::itemSelectionChanged, this, [this]{
        fillEmpInputsFromSelection();
    });
}

// ---------------- DB ----------------
QString MainWindow::dbPath() const {
    // macOS: ~/Library/Application Support/<AppName>/employee_manage.sqlite
    return QDir::homePath() + "/EmployeeManage.db";
}

void MainWindow::initDb() {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath());
    if (!db.open()) {
        QMessageBox::critical(this, "DB错误", "无法打开SQLite数据库:\n" + db.lastError().text());
        std::exit(1);
    }

    QSqlQuery q(db);
    q.exec("PRAGMA foreign_keys = ON;");
    // departments: id(内部主键) + depno(展示/业务编号) + parent_id
    if (!q.exec(
            "CREATE TABLE IF NOT EXISTS departments("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "depno INTEGER NOT NULL UNIQUE,"
            "name TEXT NOT NULL,"
            "parent_id INTEGER,"
            "FOREIGN KEY(parent_id) REFERENCES departments(id)"
            ");")) {
        QMessageBox::critical(this, "建表失败", q.lastError().text());
        std::exit(1);
    }

    if (!q.exec(
            "CREATE TABLE IF NOT EXISTS employees("
            "no INTEGER PRIMARY KEY,"
            "name TEXT NOT NULL,"
            "depno INTEGER NOT NULL,"
            "salary REAL NOT NULL,"
            "FOREIGN KEY(depno) REFERENCES departments(depno)"
            ");")) {
        QMessageBox::critical(this, "建表失败", q.lastError().text());
        std::exit(1);
    }

    statusLabel->setText("DB: " + dbPath());
}

void MainWindow::seedDefaultDepartmentsIfEmpty() {
    QSqlQuery q(db);
    if (!q.exec("SELECT COUNT(*) FROM departments;")) return;
    if (!q.next()) return;
    if (q.value(0).toInt() > 0) return;

    // 默认部门树：1行政, 2研发(子:4后端 5前端), 3财务
    db.transaction();

    auto ins = [&](int depno, const QString& name, QVariant parentId){
        QSqlQuery t(db);
        t.prepare("INSERT INTO departments(depno,name,parent_id) VALUES(?,?,?);");
        t.addBindValue(depno);
        t.addBindValue(name);
        if (parentId.isNull()) t.addBindValue(QVariant(QVariant::Int));
        else t.addBindValue(parentId);
        if (!t.exec()) {
            db.rollback();
            QMessageBox::critical(this, "初始化失败", t.lastError().text());
            std::exit(1);
        }
        return t.lastInsertId();
    };

    QVariant id1 = ins(1, "行政", QVariant());
    QVariant id2 = ins(2, "研发", QVariant());
    QVariant id3 = ins(3, "财务", QVariant());
    ins(4, "研发-后端", id2);
    ins(5, "研发-前端", id2);
    Q_UNUSED(id1);
    Q_UNUSED(id3);

    db.commit();
}

// ---------------- departments -> tree ----------------
QVariant MainWindow::selectedDeptId() const {
    auto items = treeDepts->selectedItems();
    if (items.isEmpty()) return QVariant();
    return items.first()->data(0, ROLE_DEPT_ID);
}

int MainWindow::selectedDepno() const {
    auto items = treeDepts->selectedItems();
    if (items.isEmpty()) return 0;
    return items.first()->data(0, ROLE_DEPT_DEPNO).toInt();
}

void MainWindow::loadDeptsToTree(int selectDeptId) {
    treeDepts->clear();

    // 顶部虚拟节点：0-全部部门
    auto *all = new QTreeWidgetItem(treeDepts);
    all->setText(0, "0 - 全部部门");
    all->setData(0, ROLE_DEPT_ID, 0);
    all->setData(0, ROLE_DEPT_DEPNO, 0);

    // 先把所有部门读出来
    struct DeptRow { int id; int depno; QString name; QVariant parentId; };
    QVector<DeptRow> rows;

    QSqlQuery q(db);
    if (!q.exec("SELECT id, depno, name, parent_id FROM departments ORDER BY depno ASC;")) {
        QMessageBox::warning(this, "读取部门失败", q.lastError().text());
        treeDepts->setCurrentItem(all);
        return;
    }
    while (q.next()) {
        DeptRow r;
        r.id = q.value(0).toInt();
        r.depno = q.value(1).toInt();
        r.name = q.value(2).toString();
        r.parentId = q.value(3);
        rows.push_back(r);
    }

    // 建 id->item 映射
    QHash<int, QTreeWidgetItem*> id2item;
    id2item.insert(0, all);

    // 先创建所有 item（挂到 all 上，稍后再调整父子）
    for (const auto& r : rows) {
        auto *it = new QTreeWidgetItem();
        it->setText(0, QString("%1 - %2").arg(r.depno).arg(r.name));
        it->setData(0, ROLE_DEPT_ID, r.id);
        it->setData(0, ROLE_DEPT_DEPNO, r.depno);
        all->addChild(it);
        id2item.insert(r.id, it);
    }

    // 再根据 parent_id 重挂
    for (const auto& r : rows) {
        if (r.parentId.isNull()) continue;
        int pid = r.parentId.toInt();
        if (!id2item.contains(pid)) continue;

        auto *child = id2item.value(r.id);
        auto *parent = id2item.value(pid);
        if (child && parent && child->parent() != parent) {
            // 从当前父节点移除再加到正确父节点
            QTreeWidgetItem* oldParent = child->parent();
            if (oldParent) {
                int idx = oldParent->indexOfChild(child);
                if (idx >= 0) oldParent->takeChild(idx);
            }
            parent->addChild(child);
        }
    }

    all->setExpanded(true);

    // 默认选中
    QTreeWidgetItem* toSelect = all;
    if (selectDeptId > 0 && id2item.contains(selectDeptId))
        toSelect = id2item.value(selectDeptId);

    treeDepts->setCurrentItem(toSelect);
}

bool MainWindow::addDeptToDb(int depno, const QString& name, const QVariant& parentId, int* outNewId) {
    QSqlQuery q(db);
    q.prepare("INSERT INTO departments(depno,name,parent_id) VALUES(?,?,?);");
    q.addBindValue(depno);
    q.addBindValue(name.trimmed());
    if (parentId.isNull() || parentId.toInt() == 0) q.addBindValue(QVariant(QVariant::Int));
    else q.addBindValue(parentId);

    if (!q.exec()) {
        QMessageBox::warning(this, "添加部门失败", q.lastError().text());
        return false;
    }
    if (outNewId) *outNewId = q.lastInsertId().toInt();
    return true;
}

void MainWindow::addDeptAsTop() {
    bool ok1=false;
    int depno = editDeptNo->text().trimmed().toInt(&ok1);
    QString name = editDeptName->text().trimmed();

    if (!ok1 || depno <= 0) {
        QMessageBox::information(this, "提示", "depno 必须是 >0 的整数。");
        return;
    }
    if (name.isEmpty()) {
        QMessageBox::information(this, "提示", "部门名不能为空。");
        return;
    }

    int newId = -1;
    if (addDeptToDb(depno, name, QVariant(), &newId)) {
        loadDeptsToTree(newId);
        editDeptNo->clear();
        editDeptName->clear();
    }
}

void MainWindow::addDeptAsChild() {
    QVariant pid = selectedDeptId(); // departments.id
    if (!pid.isValid()) pid = 0;

    bool ok1=false;
    int depno = editDeptNo->text().trimmed().toInt(&ok1);
    QString name = editDeptName->text().trimmed();

    if (!ok1 || depno <= 0) {
        QMessageBox::information(this, "提示", "depno 必须是 >0 的整数。");
        return;
    }
    if (name.isEmpty()) {
        QMessageBox::information(this, "提示", "部门名不能为空。");
        return;
    }

    // 选中的是“全部部门”(id=0)时，等价于顶级部门
    int parentId = pid.toInt();
    QVariant parentVar = (parentId == 0 ? QVariant() : pid);

    int newId = -1;
    if (addDeptToDb(depno, name, parentVar, &newId)) {
        loadDeptsToTree(newId);
        editDeptNo->clear();
        editDeptName->clear();
    }
}

void MainWindow::onDeptSelectionChanged() {
    refreshEmployeesByDeptSelection();
}

// ---------------- employees ----------------
void MainWindow::refreshEmployees() {
    refreshEmployeesByDeptSelection();
}
void MainWindow::refreshEmployeesByDeptSelection() {
    // 选中的是“全部部门”
    QVariant deptIdVar = selectedDeptId();
    bool ok = false;
    int deptId = deptIdVar.toInt(&ok);

    QSqlQuery q(db);

    if (!ok || deptId <= 0) {
        // 全部部门：显示所有员工
        q.prepare("SELECT no,name,depno,salary FROM employees ORDER BY no ASC;");
        if (!q.exec()) {
            QMessageBox::warning(this, "查询失败", q.lastError().text());
            return;
        }
    } else {
        q.prepare(
            "WITH RECURSIVE sub(id, depno) AS ("
            "  SELECT id, depno FROM departments WHERE id = ?"
            "  UNION ALL "
            "  SELECT d.id, d.depno FROM departments d "
            "  JOIN sub s ON d.parent_id = s.id"
            ") "
            "SELECT e.no, e.name, e.depno, e.salary "
            "FROM employees e "
            "WHERE e.depno IN (SELECT depno FROM sub) "
            "ORDER BY e.no ASC;"
            );
        q.addBindValue(deptId);

        if (!q.exec()) {
            QMessageBox::warning(this, "查询失败", q.lastError().text());
            return;
        }
    }

    // 把结果填到你的 tableEmps（QTableWidget）里
    tableEmps->setRowCount(0);
    int row = 0;
    while (q.next()) {
        tableEmps->insertRow(row);
        tableEmps->setItem(row, 0, new QTableWidgetItem(q.value(0).toString()));
        tableEmps->setItem(row, 1, new QTableWidgetItem(q.value(1).toString()));
        tableEmps->setItem(row, 2, new QTableWidgetItem(q.value(2).toString()));
        tableEmps->setItem(row, 3, new QTableWidgetItem(q.value(3).toString()));
        row++;
    }

    statusLabel->setText(QString("当前显示 %1 条员工记录").arg(row));
}


void MainWindow::fillEmpInputsFromSelection() {
    int r = tableEmps->currentRow();
    if (r < 0) return;

    auto *iNo = tableEmps->item(r, COL_NO);
    auto *iName = tableEmps->item(r, COL_NAME);
    auto *iDep = tableEmps->item(r, COL_DEPNO);
    auto *iSal = tableEmps->item(r, COL_SALARY);
    if (!iNo || !iName || !iDep || !iSal) return;

    editNo->setText(iNo->text());
    editName->setText(iName->text());
    editDepno->setText(iDep->text());
    editSalary->setText(iSal->text());
}

void MainWindow::addEmployee() {
    bool okNo=false, okDep=false, okSal=false;
    int no = editNo->text().trimmed().toInt(&okNo);
    int depno = editDepno->text().trimmed().toInt(&okDep);
    double salary = editSalary->text().trimmed().toDouble(&okSal);
    QString name = editName->text().trimmed();

    if (!okNo || no <= 0) { QMessageBox::information(this,"提示","工号 no 必须是 >0 的整数"); return; }
    if (name.isEmpty()) { QMessageBox::information(this,"提示","姓名不能为空"); return; }
    if (!okDep || depno <= 0) { QMessageBox::information(this,"提示","部门号 depno 必须是 >0 的整数"); return; }
    if (!okSal) { QMessageBox::information(this,"提示","工资 salary 必须是数字"); return; }
    if(salary < 0){QMessageBox::information(this,"提示","工资 salary 必须是正数"); return; }

    QSqlQuery q(db);
    q.prepare("INSERT INTO employees(no,name,depno,salary) VALUES(?,?,?,?);");
    q.addBindValue(no);
    q.addBindValue(name);
    q.addBindValue(depno);
    q.addBindValue(salary);


    if (!q.exec()) {
        QSqlError err = q.lastError();
        QString msg = err.text();

        if(msg.contains("UNIQUE")){
            QMessageBox::warning(this, "添加失败", "工号已存在，不能重复添加");
        }
        else if(msg.contains("constraint")){
            QMessageBox::warning(this, "添加失败", "部门不存在，请检查部门号是否正确");
        }
        return;
    }
    refreshEmployeesByDeptSelection();
}

void MainWindow::updateEmployeeByNo() {
    bool okNo=false, okDep=false, okSal=false;
    int no = editNo->text().trimmed().toInt(&okNo);
    int depno = editDepno->text().trimmed().toInt(&okDep);
    double salary = editSalary->text().trimmed().toDouble(&okSal);
    QString name = editName->text().trimmed();

    if (!okNo || no <= 0) { QMessageBox::information(this,"提示","工号 no 必须是 >0 的整数"); return; }
    if (name.isEmpty()) { QMessageBox::information(this,"提示","姓名不能为空"); return; }
    if (!okDep || depno <= 0) { QMessageBox::information(this,"提示","部门号 depno 必须是 >0 的整数"); return; }
    if (!okSal) { QMessageBox::information(this,"提示","工资 salary 必须是数字"); return; }

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
    if (q.numRowsAffected() <= 0) {
        QMessageBox::information(this, "提示", "没有找到该 no 的员工记录。");
        return;
    }
    refreshEmployeesByDeptSelection();
}

void MainWindow::deleteSelectedEmployee() {
    int r = tableEmps->currentRow();
    if (r < 0) {
        QMessageBox::information(this, "提示", "请先选中要删除的员工行。");
        return;
    }
    bool ok=false;
    int no = tableEmps->item(r, COL_NO)->text().toInt(&ok);
    if (!ok) return;

    if (QMessageBox::question(this, "确认删除", QString("确定删除工号 %1 ?").arg(no))
        != QMessageBox::Yes) return;

    QSqlQuery q(db);
    q.prepare("DELETE FROM employees WHERE no=?;");
    q.addBindValue(no);
    if (!q.exec()) {
        QMessageBox::warning(this, "删除失败", q.lastError().text());
        return;
    }
    refreshEmployeesByDeptSelection();
}

void MainWindow::clearAllEmployees() {
    if (QMessageBox::question(this, "确认全清", "确定清空 employees 表中所有员工记录？")
        != QMessageBox::Yes) return;

    QSqlQuery q(db);
    if (!q.exec("DELETE FROM employees;")) {
        QMessageBox::warning(this, "全清失败", q.lastError().text());
        return;
    }
    refreshEmployeesByDeptSelection();
}
