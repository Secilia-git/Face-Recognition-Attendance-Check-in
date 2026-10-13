#ifndef DATABASECONTROL_H
#define DATABASECONTROL_H


#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>
#include "userinfo.h"

// 考勤记录结构：保存一次打卡的关键信息
struct attendanceInfo
{
    QString user;          // 打卡用户名
    QString realName;      // 真实姓名
    QString department;    // 所属部门
    QString position;      // 职位
    QString punchDate;     // 打卡日期 yyyy-MM-dd
    QString punchTime;     // 打卡时间 HH:mm:ss
    QString status;        // 正常 / 迟到
    bool alreadyExists;    // 该用户当天是否已经打过卡

    attendanceInfo() : alreadyExists(false) {}
};

class databasecontrol : public QObject
{
    Q_OBJECT
public:
    static databasecontrol* instance();

    // 查询所有注册用户信息
    QList<userInfo> queryAllUserinfo();

    // 添加用户信息
    bool addUserinfo(const userInfo&info);

    // 修改用户信息（oldUser 为修改前的用户名，可同时修改用户名和资料）
    bool updateUserinfo(const QString &oldUser, const userInfo &newInfo);

    // 写入一条考勤记录；同一天同一用户不能重复打卡
    bool recordAttendance(const QString &user,
                          const QDateTime &punchTime,
                          attendanceInfo *result = nullptr);

    // 查询某天的全部考勤记录
    QList<attendanceInfo> queryAttendanceByDate(const QString &date);

    // 查询某天迟到人员的考勤记录
    QList<attendanceInfo> queryLateAttendance(const QString &date);

    bool isOpen() const;
    bool updateFaceFeature(const QString &user, const QByteArray &faceFeature);
    QByteArray queryFaceFeature(const QString &user);
    bool userExists(const QString &user);
    bool deleteUser(const QString &user);






private:
    explicit databasecontrol(QObject *parent = nullptr);//隐藏构造函数
    databasecontrol(const databasecontrol&) = delete; //删除拷贝函数

    databasecontrol& operator =(const userInfo&) =delete;
    void ensureTables();
    void ensureUserColumn(const QString &column, const QString &definition);
    attendanceInfo makeAttendanceInfo(const QSqlQuery &query) const;
    void fillAttendanceUserInfo(attendanceInfo &info) const;
    QSqlDatabase db;

};

#endif // DATABASECONTROL_H
