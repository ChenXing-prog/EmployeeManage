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

    void saveAll();

private:
    //UI
    QTreeWidget* treeDepts = nullptr;

    QTableWidget* tableEmps = nullptr;
    QLabel* statusLabel = nullptr;

    //员工编辑框
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

    QPushButton* btnSaveAll = nullptr;
    //新增部门区域
    QLineEdit* editDeptNo = nullptr;
    QLineEdit* editDeptName = nullptr;
    QPushButton* btnAddDeptTop = nullptr;
    QPushButton* btnAddDeptChild = nullptr;


    //DB
    DbManager dbm;

    //主数据：AVL 保存全部员工（按 no 作为 key）
    AvlTree empAvl;

    //部门树（用于左侧展示 + 校验 depno 是否存在）
    DeptTree deptTree;

    QVector<DeptRow> deptRowsCache;//部门主数据缓存（DB->内存，仅启动/刷新时加载一次）

private:
    void buildUi();

    QString dbPath() const;

    //初始化数据库
    void initDbAndLoad();

    enum SortMode { SortByNo, SortBySalary };
    SortMode sortMode = SortByNo;

    //部门
    //如果没有部门，就插入默认部门
    void seedDefaultDepartmentsIfEmpty();

    //从数据库中读取部门信息
    void loadDeptsToTree(int selectDeptId = 0);

    //退回选中部门的id
    QVariant selectedDeptId() const;

    //向数据库插入部门信息
    bool addDeptToDb(int depno, const QString& name, const QVariant& parentId, int* outNewId = nullptr);


    //从数据库中读取所有员工，构建AVL树
    void loadEmployeesFromDbToAvl();



    //将AVL中的员工数据写回数据库
    void saveEmployeesFromAvlToDb();

    //刷新table的显示信息
    void refreshEmployeesByDeptSelection();

    //把选中的部门id转换为depno
    int selectedDeptNoForFilter() const;

    //状态设置
    void setStatus(const QString& s);

    //找到子树
    void collectDeptNosFromItem(QTreeWidgetItem* item, QSet<int>& out) const;

    //过滤子树
    QSet<int> selectedDeptSubtreeNos() const;


    void appendDeptAndRefresh(int newId, int depno, const QString& name, const QVariant& parentId);

};

#endif // MAINWINDOW_H
