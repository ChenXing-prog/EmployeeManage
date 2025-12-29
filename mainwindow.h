#pragma once

#include <QMainWindow>
#include <QTreeWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QSqlDatabase>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onDeptSelectionChanged();
    void addEmployee();
    void updateEmployee();
    void deleteEmployee();
    void reloadAll();
    void clearAll();

private:
    // ---------- UI（纯代码创建） ----------
    QTreeWidget* treeDepts = nullptr;
    QTableWidget* tableEmps = nullptr;

    QLineEdit* editNo = nullptr;
    QLineEdit* editName = nullptr;
    QLineEdit* editDepno = nullptr;
    QLineEdit* editSalary = nullptr;

    QLabel* statusLabel = nullptr;

    // ---------- DB ----------
    QSqlDatabase db;

    // ---------- helpers ----------
    void buildUi();
    void initDb();
    QString dbPath() const;

    void createTablesIfNeeded();
    void loadDepts();
    void loadEmployees(int depnoFilter /*-1 = all*/);
    void fillEditsFromCurrentRow();
    int currentSelectedDepno() const; // -1 for all
};
