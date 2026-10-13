#include "mainwindow.h"
#include"dialog.h"
#include"manager_dialog.h"
#include"registration_dialog.h"
#include <QApplication>
#include"databasecontrol.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Dialog dialog;
    MainWindow mainWindow;
    registration_Dialog Registration_Dialog;
    manager_Dialog Manager_Dialog;
    databasecontrol* db = databasecontrol::instance();
    if(!db->isOpen())
    {
        return -1;
    }

    //通过登录跳转到主界面
    QObject::connect(&dialog,&Dialog::loginSuccess,[&](){
        mainWindow.show();
        dialog.hide();
    });

    //注册界面
    QObject::connect(&dialog,&Dialog::registrationSuccess,[&](){
        Registration_Dialog.show();
        dialog.hide();
    });
    //返回登录界面
    QObject::connect(&Registration_Dialog,&registration_Dialog::return_success,[&](){

        dialog.show();
        Registration_Dialog.hide();

    });
    //管理员界面
    QObject::connect(&dialog,&Dialog::managerSuccess,[&](){
        Manager_Dialog.show();
        dialog.hide();
    });
    QObject::connect(&Manager_Dialog,&manager_Dialog::return_dia,[&](){
        dialog.show();
        Manager_Dialog.hide();
    });




    dialog.show();
    return a.exec();
}
