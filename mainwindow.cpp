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
#include <QStandardPaths>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <functional>

static bool cmpSalaryAsc(const Emp& a, const Emp& b) {
    if (a.salary < b.salary) return true;
    if (a.salary > b.salary) return false;
    return a.no < b.no; // 工资一样时按工号稳定一下（保证确定性）
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{

    buildUi();
    qDebug() << "buid";
    initDbAndLoad();
}

MainWindow::~MainWindow() {
    dbm.close();
}

void MainWindow::setStatus(const QString& s) {
    if (statusLabel) statusLabel->setText(s);
}

QString MainWindow::dbPath() const {
    return QDir::homePath() + "/EmployeeManage.db";
}

void MainWindow::initDbAndLoad() {
    qDebug() << "1";
    //打开数据库
    if (!dbm.open(dbPath())) {
        QMessageBox::critical(this, "DB错误", "无法打开SQLite数据库:\n" + dbm.db().lastError().text());
        exit(1);
    }

    //建数据库
    QString err;
    if (!dbm.ensureTables(&err)) {
        QMessageBox::critical(this, "DB错误", "建表失败:\n" + err);
        exit(1);
    }
    qDebug() << "1";

    seedDefaultDepartmentsIfEmpty();

    deptRowsCache = dbm.fetchDepartments(&err);
    if (!err.isEmpty()) {
        QMessageBox::warning(this, "提示", "读取部门失败:\n" + err);
        return;
    }

    // 用缓存建树 + 绘制
    deptTree.buildFromRows(deptRowsCache);
    loadDeptsToTree(0);


    // ★启动时：DB -> AVL
    loadEmployeesFromDbToAvl();
    refreshEmployeesByDeptSelection();
}

void MainWindow::buildUi() {
    qDebug() << "1";
    setWindowTitle("EmployeeManage - 部门树 + 员工表 (Qt + SQLite)");
    qDebug() << "2";
    resize(1300, 780);
    qDebug() << "3";

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(10);

    // ========= 左侧：部门树 + 新增部门 =========
    auto* leftBox = new QGroupBox("部门树", central);
    auto* leftLay = new QVBoxLayout(leftBox);

    treeDepts = new QTreeWidget(leftBox);
    treeDepts->setHeaderLabels(QStringList() << "部门");
    leftLay->addWidget(treeDepts, 1);

    auto* addDeptBox = new QGroupBox("新增部门", leftBox);
    auto* addDeptLay = new QVBoxLayout(addDeptBox);

    auto* form = new QFormLayout();
    editDeptNo = new QLineEdit(addDeptBox);
    editDeptName = new QLineEdit(addDeptBox);
    editDeptNo->setPlaceholderText("整数，例如 6");
    editDeptName->setPlaceholderText("部门名称，例如 测试部");



    form->addRow("depno(部门编号):", editDeptNo);
    form->addRow("name(部门名):", editDeptName);
    addDeptLay->addLayout(form);

    auto* rowBtn = new QHBoxLayout();
    btnAddDeptTop = new QPushButton("添加为顶级部门", addDeptBox);
    btnAddDeptChild = new QPushButton("添加为选中部门子部门", addDeptBox);

    rowBtn->addWidget(btnAddDeptTop);
    rowBtn->addWidget(btnAddDeptChild);



    addDeptLay->addLayout(rowBtn);


    leftLay->addWidget(addDeptBox, 0);

    root->addWidget(leftBox, 0);

    // ========= 右侧：员工列表 + 增删改 =========
    auto* rightBox = new QGroupBox("员工列表", central);
    auto* rightLay = new QVBoxLayout(rightBox);

    tableEmps = new QTableWidget(rightBox);
    tableEmps->setColumnCount(4);
    tableEmps->setHorizontalHeaderLabels(QStringList() << "no" << "name" << "depno" << "salary");
    tableEmps->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableEmps->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableEmps->setSelectionMode(QAbstractItemView::SingleSelection);
    tableEmps->horizontalHeader()->setStretchLastSection(true);
    tableEmps->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    rightLay->addWidget(tableEmps, 1);

    auto* editBox = new QGroupBox("新增 / 修改", rightBox);
    auto* editLay = new QVBoxLayout(editBox);

    auto* form2 = new QFormLayout();
    editNo = new QLineEdit(editBox);
    editName = new QLineEdit(editBox);
    editDepno = new QLineEdit(editBox);
    editSalary = new QLineEdit(editBox);

    form2->addRow("no(工号):", editNo);
    form2->addRow("name(姓名):", editName);
    form2->addRow("depno(部门号):", editDepno);
    form2->addRow("salary(工资):", editSalary);
    editLay->addLayout(form2);

    auto* btnRow = new QHBoxLayout();
    btnAddEmp = new QPushButton("添加", editBox);
    btnUpdateEmp = new QPushButton("修改(按no)", editBox);
    btnDeleteEmp = new QPushButton("删除(选中行)", editBox);
    btnReload = new QPushButton("刷新", editBox);
    btnClearDb = new QPushButton("全清(DB)", editBox);

    btnOrderBySalary = new QPushButton("按工资排序", editBox);
    btnOrderByNo = new QPushButton("按工号排序", editBox);

    btnSaveAll = new QPushButton("保存",editBox);

    btnRow->addWidget(btnAddEmp);
    btnRow->addWidget(btnUpdateEmp);
    btnRow->addWidget(btnDeleteEmp);
    btnRow->addWidget(btnReload);
    btnRow->addWidget(btnClearDb);
    btnRow->addWidget(btnOrderBySalary);
    btnRow->addWidget(btnOrderByNo);
    btnRow->addWidget(btnSaveAll);
    editLay->addLayout(btnRow);

    rightLay->addWidget(editBox, 0);

    statusLabel = new QLabel("就绪", rightBox);
    rightLay->addWidget(statusLabel, 0);

    root->addWidget(rightBox, 1);

    // ========= signals =========
    connect(treeDepts, &QTreeWidget::itemSelectionChanged, this, &MainWindow::onDeptSelectionChanged);
    connect(btnAddDeptTop, &QPushButton::clicked, this, &MainWindow::addDeptAsTop);
    connect(btnAddDeptChild, &QPushButton::clicked, this, &MainWindow::addDeptAsChild);

    connect(btnAddEmp, &QPushButton::clicked, this, &MainWindow::addEmployee);
    connect(btnUpdateEmp, &QPushButton::clicked, this, &MainWindow::updateEmployee);
    connect(btnDeleteEmp, &QPushButton::clicked, this, &MainWindow::deleteSelectedRow);
    connect(btnReload, &QPushButton::clicked, this, &MainWindow::reloadFromDb);
    connect(btnClearDb, &QPushButton::clicked, this, &MainWindow::clearAllInMemoryAndDb);
    connect(btnOrderByNo, &QPushButton::clicked, this, &MainWindow::sortByNo);
    connect(btnOrderBySalary, &QPushButton::clicked, this, &MainWindow::sortBySalary);
    connect(btnSaveAll, &QPushButton::clicked, this, &MainWindow::saveAll);

}

void MainWindow::seedDefaultDepartmentsIfEmpty() {
    int cnt = 0;
    QString err;
    if (!dbm.countDepartments(&cnt, &err)) {
        QMessageBox::warning(this, "提示", "无法读取部门数量:\n" + err);
        return;
    }
    if (cnt > 0) return;

}

QVariant MainWindow::selectedDeptId() const {
    auto items = treeDepts->selectedItems();
    if (items.isEmpty()) return 0;
    return items.first()->data(0, Qt::UserRole);
}

int MainWindow::selectedDeptNoForFilter() const {
    QVariant idv = selectedDeptId();
    int id = idv.isValid() ? idv.toInt() : 0;
    return deptTree.depnoOf(id);
}

//加载部门到左侧的部门树
void MainWindow::loadDeptsToTree(int selectDeptId) {
    QString err;

    //绘制TreeWidget
    treeDepts->clear();

    auto makeItem = [&](int id, QTreeWidgetItem* parent) -> QTreeWidgetItem* {
        QString text = QString("%1 - %2").arg(deptTree.depnoOf(id)).arg(deptTree.nameOf(id));
        auto* it = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(treeDepts);
        it->setText(0, text);
        it->setData(0, Qt::UserRole, id);
        return it;
    };

    // 根
    auto* rootItem = makeItem(0, nullptr);

    // 递归构建
    std::function<void(int, QTreeWidgetItem*)> buildRec = [&](int pid, QTreeWidgetItem* parentItem) {
        auto children = deptTree.childrenOf(pid);
        for (int cid : children) {
            auto* it = makeItem(cid, parentItem);
            buildRec(cid, it);
        }
    };
    buildRec(0, rootItem);
    treeDepts->expandAll();

    // 选中
    std::function<QTreeWidgetItem*(QTreeWidgetItem*)> selectById;
    selectById = [&](QTreeWidgetItem* it) -> QTreeWidgetItem* {
        if (!it) return nullptr;
        if (it->data(0, Qt::UserRole).toInt() == selectDeptId) return it;
        for (int i = 0; i < it->childCount(); ++i) {
            if (auto* got = selectById(it->child(i))) return got;
        }
        return nullptr;
    };

    QTreeWidgetItem* toSel = selectById(rootItem);
    if (!toSel) toSel = rootItem;
    treeDepts->setCurrentItem(toSel);
}

bool MainWindow::addDeptToDb(int depno, const QString& name, const QVariant& parentId, int* outNewId) {
    QString err;
    if (!dbm.insertDepartment(depno, name, parentId, outNewId, &err)) {
        QMessageBox::warning(this, "添加部门失败", err);
        return false;
    }
    return true;
}

void MainWindow::addDeptAsTop() {
    bool ok=false;
    int depno = editDeptNo->text().trimmed().toInt(&ok);
    QString name = editDeptName->text().trimmed();

    if (!ok || depno <= 0) { QMessageBox::information(this,"提示","depno 必须是>0整数"); return; }
    if (name.isEmpty()) { QMessageBox::information(this,"提示","部门名不能为空"); return; }

    int newId=0;
    QVariant parentId;
    if (!addDeptToDb(depno, name, parentId, &newId)) return;

    appendDeptAndRefresh(newId, depno, name, parentId);
    setStatus(QString("已添加部门 %1-%2").arg(depno).arg(name));
}


void MainWindow::addDeptAsChild() {
    bool ok=false;
    int depno = editDeptNo->text().trimmed().toInt(&ok);
    QString name = editDeptName->text().trimmed();

    if (!ok || depno <= 0) { QMessageBox::information(this,"提示","depno 必须是>0整数"); return; }
    if (name.isEmpty()) { QMessageBox::information(this,"提示","部门名不能为空"); return; }

    QVariant pid = selectedDeptId();
    if (!pid.isValid() || pid.isNull()) pid = 0;
    int pidInt = pid.toInt();

    // 不允许挂在不存在的节点下
    if (!deptTree.containsId(pidInt)) pidInt = 0;

    QVariant parentId = (pidInt==0) ? QVariant() : QVariant(pidInt);

    int newId=0;
    if (!addDeptToDb(depno, name, parentId, &newId)) return;

    appendDeptAndRefresh(newId, depno, name, parentId);
    setStatus(QString("已添加子部门 %1-%2").arg(depno).arg(name));
}



void MainWindow::onDeptSelectionChanged() {
    //按当前部门展示员工
    refreshEmployeesByDeptSelection();


    //在编辑框中添加对应部门编号
    auto *it = treeDepts->currentItem();
    if (!it) return;

    int depno = deptTree.depnoOf(it->data(0, Qt::UserRole).toInt());


    if (depno == 0) {
        editDepno->clear();
        return;
    }

    editDepno->setText(QString::number(depno));
}

// ===================== 员工：DB<->AVL =====================

void MainWindow::loadEmployeesFromDbToAvl() {
    empAvl.clear();

    QString err;
    auto emps = dbm.fetchAllEmployees(&err);
    if (!err.isEmpty()) {
        QMessageBox::warning(this, "提示", "读取员工失败:\n" + err);
        return;
    }

    for (const auto& e : emps) {
        empAvl.insert(e);
    }
    setStatus(QString("已加载 %1 条员工记录（DB -> AVL）").arg(emps.size()));
}

void MainWindow::saveEmployeesFromAvlToDb() {
    QString err;
    QVector<Emp> emps = empAvl.inorder(); // 按 no 排序输出（AVL 遍历）
    if (!dbm.replaceAllEmployees(emps, &err)) {
        QMessageBox::warning(this, "保存失败", err);
        return;
    }
}

void MainWindow::refreshEmployeesByDeptSelection() {
    if (!tableEmps) return;

    // 1) 得到当前选中部门子树 depno 集合
    QSet<int> depSet = selectedDeptSubtreeNos();
    bool noFilter = depSet.isEmpty(); // 空集合代表“全部部门”

    // 2) 从 AVL 拿全量员工（按 no 升序）
    QVector<Emp> all = empAvl.inorder();
    QVector<Emp> show;
    show.reserve(all.size());
    for (int i = 0; i < all.size(); ++i) {
        const Emp& e = all[i];
        if (!noFilter && !depSet.contains(e.depno)) continue;
        show.push_back(e);
    }

    if (sortMode == SortBySalary) {
        std::sort(show.begin(), show.end(), cmpSalaryAsc);
    }

    // 3) 填表：depno 在集合里（或不需要过滤）就显示
    tableEmps->setRowCount(0);
    tableEmps->setRowCount(show.size());

    for (int r = 0; r < show.size(); ++r) {
        const Emp& e = show[r];
        tableEmps->setItem(r, 0, new QTableWidgetItem(QString::number(e.no)));
        tableEmps->setItem(r, 1, new QTableWidgetItem(e.name));
        tableEmps->setItem(r, 2, new QTableWidgetItem(QString::number(e.depno)));
        tableEmps->setItem(r, 3, new QTableWidgetItem(QString::number(e.salary)));
    }

    if (statusLabel) {
        QString modeText = (sortMode == SortBySalary) ? "工资升序" : "工号升序";
        statusLabel->setText(QString("当前显示 %1 条员工记录（AVL -> UI，%2）")
                                 .arg(show.size()).arg(modeText));
    }
}


void MainWindow::reloadFromDb() {
    loadDeptsToTree(selectedDeptId().toInt());
    loadEmployeesFromDbToAvl();
    refreshEmployeesByDeptSelection();
}

// ===================== 员工：所有操作走 AVL =====================

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

    // 部门必须存在
    if (!deptTree.containsDepno(depno)) {
        QMessageBox::information(this,"提示", QString("部门 %1 不存在，请先在左侧新增部门。").arg(depno));
        return;
    }

    if (empAvl.find(no)) {
        QMessageBox::information(this,"提示","该工号已存在（AVL中已存在 no）");
        return;
    }

    Emp e;
    e.no = no;
    e.name = name;
    e.depno = depno;
    e.salary = salary;

    empAvl.insert(e);

    //每一步保存到 DB（AVL -> DB）

    refreshEmployeesByDeptSelection();
}

void MainWindow::updateEmployee() {
    bool okNo=false, okDep=false, okSal=false;
    int no = editNo->text().trimmed().toInt(&okNo);
    int depno = editDepno->text().trimmed().toInt(&okDep);
    double salary = editSalary->text().trimmed().toDouble(&okSal);
    QString name = editName->text().trimmed();

    if (!okNo || no <= 0) { QMessageBox::information(this,"提示","工号 no 必须是 >0 的整数"); return; }
    if (name.isEmpty()) { QMessageBox::information(this,"提示","姓名不能为空"); return; }
    if (!okDep || depno <= 0) { QMessageBox::information(this,"提示","部门号 depno 必须是 >0 的整数"); return; }
    if (!okSal) { QMessageBox::information(this,"提示","工资 salary 必须是数字"); return; }

    if (!deptTree.containsDepno(depno)) {
        QMessageBox::information(this,"提示", QString("部门 %1 不存在，请先新增部门。").arg(depno));
        return;
    }

    Emp* p = empAvl.find(no);
    if (!p) {
        QMessageBox::information(this,"提示","找不到该工号（AVL中不存在）");
        return;
    }

    // AVL 以 no 为 key，修改 name/depno/salary 不影响平衡/排序
    p->name = name;
    p->depno = depno;
    p->salary = salary;

    refreshEmployeesByDeptSelection();
}

void MainWindow::deleteSelectedRow() {
    auto ranges = tableEmps->selectedRanges();
    if (ranges.isEmpty()) {
        QMessageBox::information(this,"提示","请先选中一行再删除");
        return;
    }
    int row = ranges.first().topRow();
    auto* itemNo = tableEmps->item(row, 0);
    if (!itemNo) return;

    bool ok=false;
    int no = itemNo->text().toInt(&ok);
    if (!ok) return;

    if (!empAvl.find(no)) {
        QMessageBox::information(this,"提示","AVL中找不到该工号，可能已被删除");
        return;
    }

    empAvl.remove(no);

    refreshEmployeesByDeptSelection();
}

void MainWindow::clearAllInMemoryAndDb() {
    if (QMessageBox::question(this, "确认", "确定要删除全部员工记录吗？") != QMessageBox::Yes)
        return;

    empAvl.clear();
    refreshEmployeesByDeptSelection();
}


//递归找到子树
void MainWindow::collectDeptNosFromItem(QTreeWidgetItem* item, QSet<int>& out) const {
    if (!item) return;

    bool ok = false;
    int depno = deptTree.depnoOf(item->data(0, Qt::UserRole).toInt(&ok));
    if (ok) out.insert(depno);

    for (int i = 0; i < item->childCount(); ++i) {
        collectDeptNosFromItem(item->child(i), out);
    }
}

QSet<int> MainWindow::selectedDeptSubtreeNos() const {
    QSet<int> s;
    if (!treeDepts) return s;

    QTreeWidgetItem* cur = treeDepts->currentItem();
    if (!cur) return s;

    // 约定：0 - 全部部门（根），选它就代表不过滤/显示全部
    bool ok = false;
    int depno = deptTree.depnoOf(cur->data(0, Qt::UserRole).toInt(&ok));
    qDebug() << depno;
    if (ok && depno == 0) {
        return s; // 返回空集合，表示“不过滤”
    }

    collectDeptNosFromItem(cur, s);
    return s;
}
void MainWindow::sortByNo(){
    sortMode = SortMode::SortByNo;
    refreshEmployeesByDeptSelection();
}

void MainWindow::sortBySalary(){
    sortMode = SortMode::SortBySalary;
    refreshEmployeesByDeptSelection();
}
void MainWindow::appendDeptAndRefresh(int newId, int depno, const QString& name, const QVariant& parentId) {
    DeptRow r;
    r.id = newId;
    r.depno = depno;
    r.name = name;
    r.parentId = parentId;  // 顶级就 QVariant()

    deptRowsCache.push_back(r);                 //  更新内存主数据
    deptTree.buildFromRows(deptRowsCache);      //  用缓存重建 DeptTree（不读 DB）
    loadDeptsToTree(newId);                     //  刷新 UI，并选中新节点
}

void MainWindow::saveAll(){
    saveEmployeesFromAvlToDb();
}
