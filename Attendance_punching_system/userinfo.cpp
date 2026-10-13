#include "userinfo.h"

userInfo::userInfo(QObject *parent) : QObject(parent)
{
}

userInfo::userInfo(const QString &user,
                   int password,
                   const QByteArray &faceFeature,
                   const QString &realName,
                   const QString &department,
                   const QString &position,
                   const QString &phone)
    : QObject(nullptr)
    , _user(user)
    , _password(password)
    , _faceFeature(faceFeature)
    , _realName(realName)
    , _department(department)
    , _position(position)
    , _phone(phone)
{
}

userInfo::userInfo(const userInfo &rhs)
    : QObject(nullptr)
    , _user(rhs._user)
    , _password(rhs._password)
    , _faceFeature(rhs._faceFeature)
    , _realName(rhs._realName)
    , _department(rhs._department)
    , _position(rhs._position)
    , _phone(rhs._phone)
{
}

userInfo &userInfo::operator=(const userInfo &rhs)
{
    if (this == &rhs) {
        return *this;
    }

    _user = rhs._user;
    _password = rhs._password;
    _faceFeature = rhs._faceFeature;
    _realName = rhs._realName;
    _department = rhs._department;
    _position = rhs._position;
    _phone = rhs._phone;

    return *this;
}

QByteArray userInfo::vectorToByteArray(const std::vector<float> &vec)
{
    if (vec.empty()) {
        return QByteArray();
    }

    // 将 vector<float> 的内存按字节复制到 QByteArray
    const char *data = reinterpret_cast<const char *>(vec.data());
    size_t size = vec.size() * sizeof(float);
    return QByteArray(data, static_cast<int>(size));
}

std::vector<float> userInfo::byteArrayToVector(const QByteArray &byteArray)
{
    std::vector<float> vec;
    if (byteArray.isEmpty()) {
        return vec;
    }

    // 校验数据长度是否是 float 的整数倍，避免越界读取
    if (byteArray.size() % static_cast<int>(sizeof(float)) != 0) {
        qDebug() << "警告：QByteArray 大小不是 float 的整数倍";
        return vec;
    }

    const float *data = reinterpret_cast<const float *>(byteArray.constData());
    size_t count = static_cast<size_t>(byteArray.size()) / sizeof(float);
    vec.assign(data, data + count);

    return vec;
}

QString userInfo::user() const
{
    return _user;
}

int userInfo::password() const
{
    return _password;
}

QByteArray userInfo::faceFeature() const
{
    return _faceFeature;
}

QString userInfo::realName() const
{
    return _realName;
}

QString userInfo::department() const
{
    return _department;
}

QString userInfo::position() const
{
    return _position;
}

QString userInfo::phone() const
{
    return _phone;
}
