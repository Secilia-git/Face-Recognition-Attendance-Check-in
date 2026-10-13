#include "manager_dialog.h"
#include "ui_manager_dialog.h"
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include<QDebug>
#include <QSqlError>
#include<userinfo.h>
#include<QMessageBox>

#include "databasecontrol.h"
#include <QCoreApplication>
#include <QTime>
#include <stdexcept>
manager_Dialog::manager_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::manager_Dialog)
{
    ui->setupUi(this);
}

manager_Dialog::~manager_Dialog()
{
    delete ui;
}

void manager_Dialog::on_pushButton_clicked()
{
    QString sql = "select user , password from t_user;";
    QSqlQuery query;
    bool ok =query.exec(sql);
    if(!ok)
    {
        qDebug()<<"select error:"<<query.lastError().text();
        return;
    }

    QList<userInfo> infos;


    while(query.next())
    {
        QString user = query.value("user").toString();
        int password = query.value("password").toInt();
        infos.append(userInfo(user, password,QByteArray()));
    }
    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setRowCount(infos.size());
    ui->tableWidget->setHorizontalHeaderLabels({"用户名","密码"});

    for (int i=0;i<infos.size();i++)
    {
        QString user = infos.at(i).user();
        int password = infos.at(i).password();
        ui->tableWidget->setItem(i,0,new QTableWidgetItem(user));
        ui->tableWidget->setItem(i,1,new QTableWidgetItem(QString::number(password)));
    }
}

void manager_Dialog::on_pushButton_delete_clicked()
{
    int currentRow = ui->tableWidget->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "提示", "请先选择要删除的行！");
        return;
    }

    QString user = ui->tableWidget->item(currentRow, 0)->text();

    // 更简洁的确认对话框写法
    if (QMessageBox::question(this, "确认删除",
                              QString("确定要删除用户 '%1' 吗？").arg(user),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
        return;
    }

    QSqlQuery query;
    query.prepare("DELETE FROM t_user WHERE user = ?");
    query.addBindValue(user);

    if (!query.exec()) {
        QMessageBox::critical(this, "错误", "删除失败: " + query.lastError().text());
        return;
    }

    ui->tableWidget->removeRow(currentRow);

}

void manager_Dialog::on_pushButton_late_clicked()
{
    QList<userInfo> infos;
    // 1. 清空表格和对应的数据容器，防止数据残留叠加
    ui->tableWidget_late->clearContents();
    ui->tableWidget_late->setRowCount(0);
    infos.clear();

    // 2. SQL 中需要获取 username, real_name, status 用于展示，不需要 password
    QString sql = "SELECT a.user, u.real_name, a.punch_date, a.punch_time, a.status "
                  "FROM t_attendance a "
                  "LEFT JOIN t_user u ON a.user = u.user "
                  "WHERE a.punch_date = :date AND a.status = '迟到' "
                  "ORDER BY a.punch_time";

    QSqlQuery query;
    query.prepare(sql);
    QString date = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    query.bindValue(":date", date);

    if (!query.exec()) {
        qDebug() << "查询迟到列表失败:" << query.lastError().text();
        return;
    }

    // 3. 循环读取数据库结果
    while(query.next())
    {
        // 注意：你的 userInfo 构造函数需要 7 个参数。迟到查询不需要人脸特征、密码等。
        // 这里传入空字符串和 0 补齐参数。
        QString user = query.value("user").toString();
        QString realName = query.value("real_name").toString();
        QString status = query.value("status").toString();

        // 使用查询出来的真实姓名填补到 userInfo 中
        infos.append(userInfo(user, 0, QByteArray(), realName, "", "", ""));
    }

    // 4. 设置表格结构：显示“用户名”和“真实姓名/状态”
    ui->tableWidget_late->setColumnCount(2);
    ui->tableWidget_late->setRowCount(infos.size());
    ui->tableWidget_late->setHorizontalHeaderLabels({"用户名", "真实姓名(迟到)"});

    // 5. 填充表格
    for (int i = 0; i < infos.size(); i++)
    {
        const userInfo &info = infos.at(i);

        // 显示用户名
        ui->tableWidget_late->setItem(i, 0, new QTableWidgetItem(info.user()));
        // 显示真实姓名（前面SQL里传入的）
        ui->tableWidget_late->setItem(i, 1, new QTableWidgetItem(info.realName()));
    }

    // 设置列宽自适应
    ui->tableWidget_late->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void manager_Dialog::on_pushButton_return_clicked()
{
    emit return_dia();
}
