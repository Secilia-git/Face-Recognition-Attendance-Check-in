#include "dialog.h"
#include "ui_dialog.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include<QDebug>
#include <QSqlError>
#include<databasecontrol.h>
#include<userinfo.h>

#include<QMessageBox>
Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_pushButton_loading_clicked()
{
    //获取用户输入
    QString input_user = ui->lineEdit_user->text();
    QString input_password = ui->lineEdit_password->text();
    //判断是否为空
    if(input_user.isEmpty())
    {
        QMessageBox::warning(this,"提升","请输入用户名");
        ui->lineEdit_user->setFocus();
        return;
    }
    if (input_password.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请输入密码");
        ui->lineEdit_password->setFocus();
        return;
    }
    bool ok;
    int inputPassword = input_password.toInt(&ok);

    if (!ok)
    {
        QMessageBox::warning(this, "错误", "密码必须是纯数字");
        ui->lineEdit_password->clear();
        ui->lineEdit_password->setFocus();
        return;
    }
    QSqlQuery query;
    query.prepare("SELECT password FROM t_user where user = ? ");
    query.addBindValue(input_user);

    if (!query.exec())
    {
        qDebug() << "查询失败:" << query.lastError().text();
        QMessageBox::critical(this,"错误","没有用户信息，请注册");
        return ;
    }
    //比对密码
    if(query.next())
    {
        int dbpassword = query.value(0).toInt();

        if(dbpassword == inputPassword)
        {
            QMessageBox::information(this,"成功",QString("欢迎 %1 登录").arg(input_user));
            emit loginSuccess();

        }
        else
        {
            QMessageBox::warning(this, "错误", "密码错误");
            ui->lineEdit_user->clear();
            ui->lineEdit_password->clear();
        }

    }
    else
    {
        QMessageBox::warning(this, "错误", "用户不存在");
        ui->lineEdit_user->clear();
        ui->lineEdit_password->clear();
        ui->lineEdit_user->setFocus();

    }



    return ;  // 未找到
}


void Dialog::on_pushButton_registration_clicked()
{
    emit registrationSuccess();
}

void Dialog::on_pushButton_manager_clicked()
{
    emit managerSuccess();
}
