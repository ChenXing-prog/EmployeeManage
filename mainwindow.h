#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QVariant>

class QTreeWidget;
class QTreeWidgetItem;
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
    // -------- 部门 --------
    void addDeptAsTop();
    void addDeptAsChild();
    void onDeptSelectionChanged();

    // -------- 员工 --------
    void addEmployee();
    void updateEmployeeByNo();
    void deleteSelectedEmployee();
    void refreshEmployees();
    void clearAllEmployees();

private:
    // ---------- UI ----------
    QTreeWidget*  treeDepts = nullptr;

    QTableWidget* tableEmps = nullptr;
    QLabel*       statusLabel = nullptr;

    // dept inputs
    QLineEdit* editDeptNo = nullptr;
    QLineEdit* editDeptName = nullptr;
    QPushButton* btnAddDeptTop = nullptr;
    QPushButton* btnAddDeptChild = nullptr;

    // emp inputs
    QLineEdit* editNo = nullptr;
    QLineEdit* editName = nullptr;
    QLineEdit* editDepno = nullptr;
    QLineEdit* editSalary = nullptr;

    QPushButton* btnAddEmp = nullptr;
    QPushButton* btnUpdateEmp = nullptr;
    QPushButton* btnDeleteEmp = nullptr;
    QPushButton* btnRefreshEmp = nullptr;
    QPushButton* btnClearEmp = nullptr;

    // ---------- DB ----------
    QSqlDatabase db;

private:
    void buildUi();

    // db
    QString dbPath() const;
    void initDb();
    void seedDefaultDepartmentsIfEmpty();

    // departments
    void loadDeptsToTree(int selectDeptId = -1);
    QVariant selectedDeptId() const;     // departments.id
    int selectedDepno() const;           // departments.depno (0=全部)
    bool addDeptToDb(int depno, const QString& name, const QVariant& parentId, int* outNewId = nullptr);

    // employees
    void refreshEmployeesByDeptSelection();
    void fillEmpInputsFromSelection();
};

#endif // MAINWINDOW_H
