# 人脸识别考勤打卡系统

## 项目简介

这是一个基于 Qt Widgets、OpenCV 和 SQLite 的桌面考勤打卡系统。系统通过摄像头采集人脸，完成注册、识别和打卡，并将用户信息与考勤记录保存到本地数据库。

项目包含以下三个主要界面：

- 登录窗口
- 员工注册窗口
- 人脸打卡主窗口
- 管理人员窗口

## 功能特性

- 支持员工账号注册，记录用户名、数字密码、姓名、部门、职位和手机号。
- 支持开启“人脸识别打卡”功能，注册时录入人脸特征。
- 登录后进入主窗口，通过摄像头实时检测人脸并自动识别用户。
- 同一用户同一天只能打卡一次，重复打卡会被拦截。
- 打卡时间超过 08:50:00 记为“迟到”，否则记为“正常”。
- 主窗口展示当日全部打卡记录。
- 管理窗口可查询用户、查询当日迟到人员、删除选中用户。

## 技术栈

- C++11
- Qt 5.14.2 Widgets
- Qt SQL 模块（SQLite）
- OpenCV 4.5.5
- Windows / MinGW 64 位

## 目录结构

```text
Attendance_punching_system/
|-- Attendance_punching_system.pro
|-- main.cpp
|-- dialog.h
|-- dialog.cpp
|-- dialog.ui
|-- registration_dialog.h
|-- registration_dialog.cpp
|-- registration_dialog.ui
|-- mainwindow.h
|-- mainwindow.cpp
|-- mainwindow.ui
|-- manager_dialog.h
|-- manager_dialog.cpp
|-- manager_dialog.ui
|-- databasecontrol.h
|-- databasecontrol.cpp
|-- userinfo.h
|-- userinfo.cpp
`-- README.md
```

## 模块说明

| 文件 | 作用 |
| --- | --- |
| `main.cpp` | 程序入口，创建各窗口并连接窗口之间的跳转信号 |
| `dialog.*` | 登录窗口，校验用户名和数字密码 |
| `registration_dialog.*` | 员工注册窗口，支持录入用户资料和人脸特征 |
| `mainwindow.*` | 人脸打卡主窗口，打开摄像头、识别人脸、写入考勤记录并展示当日打卡 |
| `manager_dialog.*` | 管理窗口，查询用户、查询迟到人员、删除用户 |
| `databasecontrol.*` | 数据库单例控制类，负责建表、查询、写入和升级表结构 |
| `userinfo.*` | 用户信息模型，包含人脸特征的序列化和反序列化 |

## 编译运行

### 环境要求

- Qt 5.14.2，使用 MinGW 64 位编译器。
- OpenCV 4.5.5，编译环境需与 Qt 的 MinGW 工具链匹配。
- 摄像头设备。
- OpenCV 自带的人脸检测模型 `haarcascade_frontalface_default.xml`。

当前 `.pro` 文件中的 OpenCV 路径为：

```text
D:/opencv4.5.5-MinGw7.3.0/include
D:/opencv4.5.5-MinGw7.3.0/x64/mingw/bin
```

如果本机 OpenCV 路径不同，需要同步修改 `Attendance_punching_system.pro` 中的 `INCLUDEPATH` 和 `LIBS`。

### Qt Creator 编译

1. 用 Qt Creator 打开 `Attendance_punching_system.pro`。
2. 选择 Qt 5.14.2 MinGW 64 位构建套件。
3. 执行构建。
4. 将 `haarcascade_frontalface_default.xml` 放到生成的可执行文件同目录。
5. 将 `opencv_world455.dll` 放到可执行文件同目录，或确保该目录在 `PATH` 中。

### 命令行编译

在 Qt 自带的 MinGW 命令行环境中执行：

```text
mkdir build
cd build
qmake ../Attendance_punching_system.pro
mingw32-make
```

### 数据文件

程序首次运行时会自动在可执行文件所在目录创建 `demo.db`，用于保存用户和考勤数据。如果需要恢复干净环境，可以在确认备份后删除该文件，重新运行程序会自动建表。

## 使用流程

### 登录

- 输入用户名和数字密码。
- 用户名或密码为空时，程序会提示。
- 密码必须为纯数字。
- 登录成功后进入人脸打卡主窗口。

### 注册

1. 在登录窗口点击“注册”。
2. 输入用户名、数字密码、真实姓名、部门、职位和手机号。
3. 默认勾选“启用并录入人脸”。
4. 点击“录入人脸”，摄像头会检测人脸并自动采集特征。
5. 人脸录入成功后，点击“完成注册”。

如果勾选了人脸识别但未录入人脸，程序会询问是否继续注册。未录入人脸的账号只能登录，无法用于人脸打卡。

### 人脸打卡

1. 登录后进入主窗口。
2. 点击“打卡”打开摄像头。
3. 正视摄像头，系统会检测人脸并尝试识别。
4. 识别成功后自动写入考勤记录并关闭摄像头。
5. 打卡结果会显示在状态栏和当日打卡表格中。

### 管理

- 点击“查询所有信息”显示已注册用户和密码。
- 点击“查询迟到人员”显示当日的迟到人员。
- 选中用户后点击“删除选中信息”可删除该账号。

## 数据库设计

数据库使用 SQLite，文件名为 `demo.db`，包含两张表。

### 用户表 `t_user`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `id` | INTEGER PRIMARY KEY AUTOINCREMENT | 自增主键 |
| `user` | VARCHAR(50) UNIQUE NOT NULL | 登录用户名 |
| `password` | INTEGER NOT NULL | 数字密码 |
| `face_feature` | BLOB | 人脸特征数据 |
| `real_name` | VARCHAR(50) DEFAULT '' | 真实姓名 |
| `department` | VARCHAR(50) DEFAULT '' | 部门 |
| `position` | VARCHAR(50) DEFAULT '' | 职位 |
| `phone` | VARCHAR(30) DEFAULT '' | 手机号 |

### 考勤表 `t_attendance`

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `id` | INTEGER PRIMARY KEY AUTOINCREMENT | 自增主键 |
| `user` | VARCHAR(50) NOT NULL | 打卡用户名 |
| `punch_date` | VARCHAR(10) NOT NULL | 打卡日期，格式 `yyyy-MM-dd` |
| `punch_time` | VARCHAR(8) NOT NULL | 打卡时间，格式 `HH:mm:ss` |
| `status` | VARCHAR(10) NOT NULL | 考勤状态，`正常` 或 `迟到` |
| `punch_datetime` | DATETIME NOT NULL | 完整打卡时间 |

`t_attendance` 对 `(user, punch_date)` 建立了唯一约束，用于防止同一用户同一天重复打卡。

## 核心逻辑说明

### 数据库控制

`databasecontrol` 是单例类，启动时打开 `demo.db` 并自动建表。旧数据库缺少新字段时，程序会通过 `ALTER TABLE` 自动补充字段，方便升级现有数据。

主要接口包括：

- `queryAllUserinfo`：查询所有注册用户。
- `addUserinfo`：新增用户。
- `updateUserinfo`：修改用户信息，用户名变更时同步更新考勤历史。
- `recordAttendance`：写入考勤记录，判断迟到并防止重复打卡。
- `queryAttendanceByDate`：查询某日全部考勤记录。
- `queryLateAttendance`：查询某日迟到人员。
- `updateFaceFeature` / `queryFaceFeature`：更新和查询人脸特征。
- `deleteUser`：删除用户及其考勤记录。

### 人脸识别

- 使用 OpenCV Haar Cascade 检测人脸。
- 人脸区域转换为 100×100 灰度图并归一化到 0~1，作为特征向量。
- 人脸特征以 `QByteArray` 形式存入数据库。
- 识别时计算当前特征与库中特征的欧氏距离和归一化相关系数。
- 当前阈值条件为欧氏距离不超过 22，相关系数不低于 0.55。

该方案属于轻量级简单特征比对，适合演示和基础场景。

## 已知限制

- 管理员窗口没有身份验证，任何进入登录窗口的人都可以点击“管理员”进入管理界面。
- 密码以整数明文形式保存在本地 SQLite 中，未做哈希加密。
- 人脸识别采用简单像素特征，受光线、角度、遮挡和摄像头质量影响较大。
- 同一用户同一天只允许一次打卡，不支持上下班两次打卡、请假、加班等复杂考勤规则。
- 迟到判断时间 08:50:00 直接写死在代码中，没有界面配置项。
- 管理窗口的“查询迟到人员”只查询当天记录。
- 管理窗口删除用户时只删除 `t_user` 中的账号，当前界面不会级联删除该用户的考勤记录。
- 系统依赖本地摄像头和 `haarcascade_frontalface_default.xml`，缺少摄像头或模型文件时无法进行人脸打卡。
