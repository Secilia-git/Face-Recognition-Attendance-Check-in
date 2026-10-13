#ifndef MANAGER_DIALOG_H
#define MANAGER_DIALOG_H

#include <QDialog>

namespace Ui {
class manager_Dialog;
}

class manager_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit manager_Dialog(QWidget *parent = nullptr);
    ~manager_Dialog();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_delete_clicked();

    void on_pushButton_late_clicked();
    void on_pushButton_return_clicked();

signals:
    void return_dia();

private:
    Ui::manager_Dialog *ui;
};

#endif // MANAGER_DIALOG_H
