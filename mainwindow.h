#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVariant>

#include "avl.h"
#include "depttree.h"
#include "dbmanager.h"
#include <QTreeWidgetItem>
class QTreeWidget;
class QTableWidget;
class QLineEdit;
class QLabel;
class QPushButton;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // 部门
    void addDeptAsTop();
    void addDeptAsChild();
    void onDeptSelectionChanged();



    // 员工（右侧按钮）
    void addEmployee();
    void updateEmployee();
    void deleteSelectedRow();
    void clearAllInMemoryAndDb();
    void reloadFromDb(); // “刷新”按钮：重新从 DB 载入到 AVL

    void sortBySalary();
    void sortByNo();


private:
    // ---------- UI ----------
    QTreeWidget* treeDepts = nullptr;

    QTableWidget* tableEmps = nullptr;
    QLabel* statusLabel = nullptr;

    // 员工编辑框（你原先的）
    QLineEdit* editNo = nullptr;
    QLineEdit* editName = nullptr;
    QLineEdit* editDepno = nullptr;
    QLineEdit* editSalary = nullptr;

    QPushButton* btnAddEmp = nullptr;
    QPushButton* btnUpdateEmp = nullptr;
    QPushButton* btnDeleteEmp = nullptr;
    QPushButton* btnReload = nullptr;
    QPushButton* btnClearDb = nullptr;

    QPushButton* btnOrderBySalary = nullptr;
    QPushButton* btnOrderByNo = nullptr;
    // 新增部门区域
    QLineEdit* editDeptNo = nullptr;
    QLineEdit* editDeptName = nullptr;
    QPushButton* btnAddDeptTop = nullptr;
    QPushButton* btnAddDeptChild = nullptr;


    // ---------- DB ----------
    DbManager dbm;

    // ---------- In-Memory Main Data ----------
    // ★主数据：AVL 保存全部员工（按 no 作为 key）
    AvlTree empAvl;

    // 部门树（用于左侧展示 + 校验 depno 是否存在）
    DeptTree deptTree;

private:
    void buildUi();

    QString dbPath() const;
    void initDbAndLoad();

    enum SortMode { SortByNo, SortBySalary };
    SortMode sortMode = SortByNo;

    // 部门
    void seedDefaultDepartmentsIfEmpty();
    void loadDeptsToTree(int selectDeptId = 0);
    QVariant selectedDeptId() const;
    bool addDeptToDb(int depno, const QString& name, const QVariant& parentId, int* outNewId = nullptr);

    // 员工（内存为主）
    void loadEmployeesFromDbToAvl();     // DB -> AVL
    void saveEmployeesFromAvlToDb();     // AVL -> DB
    void refreshEmployeesByDeptSelection(); // AVL -> table
    int selectedDeptNoForFilter() const;

    void setStatus(const QString& s);

    void collectDeptNosFromItem(QTreeWidgetItem* item, QSet<int>& out) const;
    QSet<int> selectedDeptSubtreeNos() const;


};

#endif // MAINWINDOW_H
