#ifndef USERINFO_H
#define USERINFO_H

#include <QByteArray>
#include <QString>
#include <vector>
#include <QObject>
#include <QDebug>

class userInfo : public QObject
{
    Q_OBJECT
public:
    // 默认构造函数
    explicit userInfo(QObject *parent = nullptr);

    // 完整用户信息构造：用户名、密码、人脸特征，以及可选的真实姓名/部门/职位/手机号
    userInfo(const QString &user,
             int password,
             const QByteArray &faceFeature = QByteArray(),
             const QString &realName = QString(),
             const QString &department = QString(),
             const QString &position = QString(),
             const QString &phone = QString());

    // 拷贝构造与赋值，方便在 QList 中保存用户信息
    explicit userInfo(const userInfo &rhs);
    userInfo &operator=(const userInfo &rhs);

    QString user() const;
    int password() const;
    QByteArray faceFeature() const;
    QString realName() const;
    QString department() const;
    QString position() const;
    QString phone() const;

    // 人脸特征 vector<float> 与 QByteArray 互转工具
    static QByteArray vectorToByteArray(const std::vector<float>& vec);
    static std::vector<float> byteArrayToVector(const QByteArray& byteArray);

private:
    QString _user;
    int _password;
    QByteArray _faceFeature;
    QString _realName;
    QString _department;
    QString _position;
    QString _phone;
};

#endif // USERINFO_H
