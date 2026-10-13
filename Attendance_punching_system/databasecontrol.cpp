#include "databasecontrol.h"
#include "userinfo.h"

#include <QCoreApplication>
#include <QSqlError>
#include <QDebug>
#include <QTime>
#include <stdexcept>

// 初始化数据库，并在首次运行时自动建表/补列
databasecontrol::databasecontrol(QObject *parent) : QObject(parent)
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    QString dbPath = QCoreApplication::applicationDirPath() + "/demo.db";
    db.setDatabaseName(dbPath);

    qDebug() << "数据库路径:" << dbPath;

    if (!db.open()) {
        qDebug() << "open db error:" << db.lastError().text();
        throw std::runtime_error(db.lastError().text().toStdString());
    }
    qDebug() << "open db success";

    // 创建/升级表结构，保证旧数据库也能正常使用新功能
    ensureTables();
}

databasecontrol *databasecontrol::instance()
{
    static databasecontrol dc;
    return &dc;
}

bool databasecontrol::isOpen() const
{
    return db.isOpen();
}

// 建表与升级：用户表增加资料字段，考勤表记录每天打卡
void databasecontrol::ensureTables()
{
    QSqlQuery query;
    QString createUserSQL =
            "CREATE TABLE IF NOT EXISTS t_user ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "user VARCHAR(50) UNIQUE NOT NULL, "
            "password INTEGER NOT NULL, "
            "face_feature BLOB, "
            "real_name VARCHAR(50) DEFAULT '', "
            "department VARCHAR(50) DEFAULT '', "
            "position VARCHAR(50) DEFAULT '', "
            "phone VARCHAR(30) DEFAULT '')";
    if (!query.exec(createUserSQL)) {
        qDebug() << "创建用户表失败:" << query.lastError().text();
    }

    // 老版本 t_user 可能没有下面这些列，逐列补齐
    ensureUserColumn("real_name", "VARCHAR(50) DEFAULT ''");
    ensureUserColumn("department", "VARCHAR(50) DEFAULT ''");
    ensureUserColumn("position", "VARCHAR(50) DEFAULT ''");
    ensureUserColumn("phone", "VARCHAR(30) DEFAULT ''");

    // 考勤表：同一天同一用户只能有一条记录
    QString createAttendanceSQL =
            "CREATE TABLE IF NOT EXISTS t_attendance ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "user VARCHAR(50) NOT NULL, "
            "punch_date VARCHAR(10) NOT NULL, "
            "punch_time VARCHAR(8) NOT NULL, "
            "status VARCHAR(10) NOT NULL, "
            "punch_datetime DATETIME NOT NULL, "
            "UNIQUE(user, punch_date))";
    if (!query.exec(createAttendanceSQL)) {
        qDebug() << "创建考勤表失败:" << query.lastError().text();
    }

    // 迟到查询常用日期和状态，建立索引加速
    query.exec("CREATE INDEX IF NOT EXISTS idx_attendance_date "
               "ON t_attendance(punch_date, status)");
}

// 检查旧表是否缺少某个列，缺少时自动 ALTER TABLE 补齐
void databasecontrol::ensureUserColumn(const QString &column,
                                       const QString &definition)
{
    QSqlQuery query;
    if (!query.exec("PRAGMA table_info(t_user)")) {
        qDebug() << "读取用户表结构失败:" << query.lastError().text();
        return;
    }

    QStringList columns;
    while (query.next()) {
        columns << query.value(1).toString();
    }
    if (columns.contains(column)) {
        return;
    }

    QSqlQuery alter;
    QString sql = "ALTER TABLE t_user ADD COLUMN " + column + " " + definition;
    if (!alter.exec(sql)) {
        qDebug() << "添加用户表字段失败:" << alter.lastError().text();
    }
}

// 查询所有注册用户信息
QList<userInfo> databasecontrol::queryAllUserinfo()
{
    QList<userInfo> infos;
    QString sql = "SELECT user, password, face_feature, "
                  "real_name, department, position, phone "
                  "FROM t_user ORDER BY id";
    QSqlQuery query;

    if (!query.exec(sql)) {
        qDebug() << "select error:" << query.lastError().text();
        return infos;
    }

    while (query.next()) {
        infos.append(userInfo(query.value("user").toString(),
                              query.value("password").toInt(),
                              query.value("face_feature").toByteArray(),
                              query.value("real_name").toString(),
                              query.value("department").toString(),
                              query.value("position").toString(),
                              query.value("phone").toString()));
    }
    return infos;
}

// 添加用户信息（包含人脸特征和扩展资料）
bool databasecontrol::addUserinfo(const userInfo &info)
{
    QSqlQuery query;
    query.prepare("INSERT INTO t_user("
                  "user, password, face_feature, real_name, department, position, phone) "
                  "VALUES (:user, :password, :feature, :real_name, :department, :position, :phone)");
    query.bindValue(":user", info.user());
    query.bindValue(":password", info.password());
    query.bindValue(":feature", info.faceFeature());
    query.bindValue(":real_name", info.realName());
    query.bindValue(":department", info.department());
    query.bindValue(":position", info.position());
    query.bindValue(":phone", info.phone());

    if (!query.exec()) {
        qDebug() << "insert error:" << query.lastError().text();
        return false;
    }
    return true;
}

// 修改用户信息；用户名允许修改，但不能改成其他已存在的用户名
bool databasecontrol::updateUserinfo(const QString &oldUser, const userInfo &newInfo)
{
    if (oldUser.isEmpty() || newInfo.user().isEmpty()) {
        return false;
    }

    // 如果修改了用户名，先检查新用户名是否已被占用
    if (oldUser != newInfo.user()) {
        QSqlQuery check;
        check.prepare("SELECT COUNT(*) FROM t_user "
                      "WHERE user = :new_user AND user <> :old_user");
        check.bindValue(":new_user", newInfo.user());
        check.bindValue(":old_user", oldUser);
        if (check.exec() && check.next() && check.value(0).toInt() > 0) {
            return false;
        }
    }

    QSqlQuery query;
    query.prepare("UPDATE t_user SET "
                  "user = :new_user, password = :password, "
                  "real_name = :real_name, department = :department, "
                  "position = :position, phone = :phone "
                  "WHERE user = :old_user");
    query.bindValue(":new_user", newInfo.user());
    query.bindValue(":password", newInfo.password());
    query.bindValue(":real_name", newInfo.realName());
    query.bindValue(":department", newInfo.department());
    query.bindValue(":position", newInfo.position());
    query.bindValue(":phone", newInfo.phone());
    query.bindValue(":old_user", oldUser);

    if (!query.exec()) {
        qDebug() << "更新用户信息失败:" << query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        return false;
    }

    // 用户名变化时同步修改历史考勤记录，保证关联数据不丢失
    if (oldUser != newInfo.user()) {
        QSqlQuery attendanceQuery;
        attendanceQuery.prepare("UPDATE t_attendance SET user = :new_user WHERE user = :old_user");
        attendanceQuery.bindValue(":new_user", newInfo.user());
        attendanceQuery.bindValue(":old_user", oldUser);
        attendanceQuery.exec();
    }

    return true;
}

// 记录考勤：早上 8:50 之后打卡算迟到，同一天不能重复打卡
bool databasecontrol::recordAttendance(const QString &user,
                                       const QDateTime &punchTime,
                                       attendanceInfo *result)
{
    attendanceInfo info;
    info.user = user;
    info.punchDate = punchTime.toString("yyyy-MM-dd");
    info.punchTime = punchTime.toString("HH:mm:ss");
    info.status = punchTime.time() > QTime(8, 50, 0) ? "迟到" : "正常";

    // 检查当天是否已经打过卡
    QSqlQuery check;
    check.prepare("SELECT punch_time, status FROM t_attendance "
                  "WHERE user = :user AND punch_date = :date");
    check.bindValue(":user", user);
    check.bindValue(":date", info.punchDate);
    if (check.exec() && check.next()) {
        info.punchTime = check.value("punch_time").toString();
        info.status = check.value("status").toString();
        info.alreadyExists = true;
        fillAttendanceUserInfo(info);
        if (result) {
            *result = info;
        }
        return false;
    }

    QSqlQuery query;
    query.prepare("INSERT INTO t_attendance("
                  "user, punch_date, punch_time, status, punch_datetime) "
                  "VALUES (:user, :date, :time, :status, :datetime)");
    query.bindValue(":user", user);
    query.bindValue(":date", info.punchDate);
    query.bindValue(":time", info.punchTime);
    query.bindValue(":status", info.status);
    query.bindValue(":datetime", punchTime.toString("yyyy-MM-dd HH:mm:ss"));

    if (!query.exec()) {
        qDebug() << "写入考勤失败:" << query.lastError().text();
        return false;
    }

    fillAttendanceUserInfo(info);
    if (result) {
        *result = info;
    }
    return true;
}

// 查询某天全部考勤记录，用于主界面展示今日考勤
QList<attendanceInfo> databasecontrol::queryAttendanceByDate(const QString &date)
{
    QList<attendanceInfo> list;
    QString sql = "SELECT a.user, a.punch_date, a.punch_time, a.status, "
                  "u.real_name, u.department, u.position "
                  "FROM t_attendance a "
                  "LEFT JOIN t_user u ON a.user = u.user "
                  "WHERE a.punch_date = :date "
                  "ORDER BY a.punch_time";
    QSqlQuery query;
    query.prepare(sql);
    query.bindValue(":date", date);

    if (!query.exec()) {
        qDebug() << "查询考勤失败:" << query.lastError().text();
        return list;
    }

    while (query.next()) {
        list.append(makeAttendanceInfo(query));
    }
    return list;
}

// 查询某天迟到人员列表，经理界面单独展示
QList<attendanceInfo> databasecontrol::queryLateAttendance(const QString &date)
{
    QList<attendanceInfo> list;
    QString sql = "SELECT a.user, a.punch_date, a.punch_time, a.status, "
                  "u.real_name, u.department, u.position "
                  "FROM t_attendance a "
                  "LEFT JOIN t_user u ON a.user = u.user "
                  "WHERE a.punch_date = :date AND a.status = '迟到' "
                  "ORDER BY a.punch_time";
    QSqlQuery query;
    query.prepare(sql);
    query.bindValue(":date", date);

    if (!query.exec()) {
        qDebug() << "查询迟到列表失败:" << query.lastError().text();
        return list;
    }

    while (query.next()) {
        list.append(makeAttendanceInfo(query));
    }
    return list;
}

// 从查询结果构造一条考勤信息
attendanceInfo databasecontrol::makeAttendanceInfo(const QSqlQuery &query) const
{
    attendanceInfo info;
    info.user = query.value("user").toString();
    info.realName = query.value("real_name").toString();
    info.department = query.value("department").toString();
    info.position = query.value("position").toString();
    info.punchDate = query.value("punch_date").toString();
    info.punchTime = query.value("punch_time").toString();
    info.status = query.value("status").toString();
    return info;
}

// 给考勤记录补充用户资料，方便列表直接显示姓名和部门
void databasecontrol::fillAttendanceUserInfo(attendanceInfo &info) const
{
    QSqlQuery query;
    query.prepare("SELECT real_name, department, position FROM t_user WHERE user = :user");
    query.bindValue(":user", info.user);
    if (query.exec() && query.next()) {
        info.realName = query.value("real_name").toString();
        info.department = query.value("department").toString();
        info.position = query.value("position").toString();
    }
}

// 更新用户的人脸特征
bool databasecontrol::updateFaceFeature(const QString &user, const QByteArray &faceFeature)
{
    QSqlQuery query;
    query.prepare("UPDATE t_user SET face_feature = :feature WHERE user = :user");
    query.bindValue(":feature", faceFeature);
    query.bindValue(":user", user);

    if (!query.exec()) {
        qDebug() << "更新人脸特征失败:" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

// 查询特定用户的人脸特征
QByteArray databasecontrol::queryFaceFeature(const QString &user)
{
    QSqlQuery query;
    query.prepare("SELECT face_feature FROM t_user WHERE user = :user");
    query.bindValue(":user", user);

    if (query.exec() && query.next()) {
        return query.value("face_feature").toByteArray();
    }
    qDebug() << "查询人脸特征失败或用户不存在:" << user;
    return QByteArray();
}

// 检查用户是否存在
bool databasecontrol::userExists(const QString &user)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM t_user WHERE user = :user");
    query.bindValue(":user", user);

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

// 删除用户，同时删除该用户的历史考勤记录
bool databasecontrol::deleteUser(const QString &user)
{
    QSqlQuery attendanceQuery;
    attendanceQuery.prepare("DELETE FROM t_attendance WHERE user = :user");
    attendanceQuery.bindValue(":user", user);
    attendanceQuery.exec();

    QSqlQuery query;
    query.prepare("DELETE FROM t_user WHERE user = :user");
    query.bindValue(":user", user);

    if (!query.exec()) {
        qDebug() << "删除用户失败:" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}
