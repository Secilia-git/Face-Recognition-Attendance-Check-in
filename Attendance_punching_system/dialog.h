#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();

private slots:
    void on_pushButton_loading_clicked();

    void on_pushButton_registration_clicked();


    void on_pushButton_manager_clicked();

signals:
    void loginSuccess();
    void registrationSuccess();
    void managerSuccess();

private:
    Ui::Dialog *ui;
};

#endif // DIALOG_H
