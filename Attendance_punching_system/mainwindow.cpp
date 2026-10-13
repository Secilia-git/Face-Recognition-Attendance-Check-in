#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "databasecontrol.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , cameraTimer(nullptr)
    , isCameraOpen(false)
    , isPunchProcessing(false)
{
    ui->setupUi(this);
    setWindowTitle("人脸识别考勤打卡系统");

    // 摄像头刷新定时器
    cameraTimer = new QTimer(this);
    connect(cameraTimer, &QTimer::timeout, this, &MainWindow::updateCamera);

    // 加载人脸检测模型，支持从程序目录或上级目录读取
    QString cascadeName = "/haarcascade_frontalface_default.xml";
    QStringList cascadePaths;
    cascadePaths << QCoreApplication::applicationDirPath() + cascadeName
                 << QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../" + cascadeName)
                 << "D:/qt/build-Attendance_punching_system-Desktop_Qt_5_14_2_MinGW_64_bit-Debug"
                    + cascadeName;
    for (const QString &path : cascadePaths) {
        if (QFile::exists(path) && faceCascade.load(path.toStdString())) {
            qDebug() << "人脸检测模型加载成功:" << path;
            break;
        }
    }
    if (faceCascade.empty()) {
        QMessageBox::warning(this, "警告",
                             "未找到 haarcascade_frontalface_default.xml，"
                             "请把模型文件放到程序目录后重新运行");
    }

    // 读取所有注册用户，用于后续人脸比对
    faceLibrary = databasecontrol::instance()->queryAllUserinfo();
    refreshAttendanceTable();
}

MainWindow::~MainWindow()
{
    closeCamera();
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    closeCamera();
    QMainWindow::closeEvent(event);
}

// 打开摄像头并开始逐帧识别
void MainWindow::on_pushButton_start_clicked()
{
    if (isCameraOpen) {
        return;
    }
    if (faceCascade.empty()) {
        QMessageBox::warning(this, "提示", "人脸检测模型未加载，无法开始打卡");
        return;
    }

    cap.open(0);
    if (!cap.isOpened()) {
        QMessageBox::warning(this, "错误", "无法打开摄像头，请检查摄像头连接");
        return;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    isCameraOpen = true;
    isPunchProcessing = false;
    cameraTimer->start(33);

    ui->label_status->setText("状态：请正对摄像头");
    ui->label_status->setStyleSheet("color: blue;");
}

// 停止摄像头
void MainWindow::on_pushButton_stop_clicked()
{
    closeCamera();
    ui->label_status->setText("状态：已停止人脸打卡");
    ui->label_status->setStyleSheet("color: gray;");
}

// 刷新今日记录，同时重新加载注册用户（方便经理修改后同步）
void MainWindow::on_pushButton_refresh_clicked()
{
    faceLibrary = databasecontrol::instance()->queryAllUserinfo();
    refreshAttendanceTable();
    ui->label_status->setText("状态：记录已刷新");
    ui->label_status->setStyleSheet("color: gray;");
}

// 摄像头画面刷新：检测人脸、显示画面并尝试识别
void MainWindow::updateCamera()
{
    if (!isCameraOpen) {
        return;
    }

    cv::Mat frame;
    cap.read(frame);
    if (frame.empty()) {
        return;
    }

    cv::Rect faceRect;
    bool hasFace = detectFace(frame, faceRect);
    cv::Mat displayFrame = frame.clone();
    if (hasFace) {
        cv::rectangle(displayFrame, faceRect, cv::Scalar(0, 255, 0), 2);
    }

    // BGR -> RGB 后显示到界面上
    cv::Mat rgbFrame;
    cv::cvtColor(displayFrame, rgbFrame, cv::COLOR_BGR2RGB);
    QImage img(rgbFrame.data, rgbFrame.cols, rgbFrame.rows,
               rgbFrame.step, QImage::Format_RGB888);
    ui->label_camera->setPixmap(QPixmap::fromImage(img).scaled(
    ui->label_camera->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    // 检测到人脸且当前没有正在处理打卡时进行比对
    if (!hasFace || isPunchProcessing) {
        if (!hasFace) {
            ui->label_status->setText("状态：未检测到人脸，请调整位置");
            ui->label_status->setStyleSheet("color: red;");
        }
        return;
    }

    std::vector<float> feature = extractFaceFeature(frame(faceRect));
    QString recognizedUser;
    double score = 0.0;
    if (feature.empty() || !recognizeFace(feature, recognizedUser, score)) {
        ui->label_status->setText("状态：未识别出注册用户，请靠近摄像头");
        ui->label_status->setStyleSheet("color: red;");
        return;
    }

    // 识别成功，锁定本次处理，避免同一画面重复打卡
    isPunchProcessing = true;
    QDateTime now = QDateTime::currentDateTime();
    attendanceInfo result;
    bool inserted = databasecontrol::instance()->recordAttendance(
        recognizedUser, now, &result);



    if (inserted) {
        QString message = result.status == "迟到"
                ? "已打卡，注意：当前时间已超过 8:50，记为迟到"
                : "打卡成功，考勤状态：正常";
        ui->label_status->setText("状态：" + message);
        ui->label_status->setStyleSheet(result.status == "迟到" ? "color: red;" : "color: green;");
        QMessageBox::information(this, "考勤结果", message);
    } else if (result.alreadyExists) {
        ui->label_status->setText("状态：该用户今天已打过卡，不能重复打卡");
        ui->label_status->setStyleSheet("color: orange;");
        QMessageBox::information(this, "提示", "该用户今天已打过卡，不能重复打卡");
    } else {
        ui->label_status->setText("状态：考勤写入失败，请检查数据库");
        ui->label_status->setStyleSheet("color: red;");
    }

    refreshAttendanceTable();
    closeCamera();
}

// 使用最小欧氏距离 + 归一化相关系数进行人脸匹配
bool MainWindow::recognizeFace(const std::vector<float> &feature,
                               QString &user,
                               double &score)
{
    double bestDistance = 1e9;
    double bestCorr = -1.0;
    QString bestUser;

    for (const userInfo &info : faceLibrary) {
        QByteArray bytes = info.faceFeature();
        if (bytes.isEmpty()) {
            continue;
        }

        std::vector<float> stored = userInfo::byteArrayToVector(bytes);
        if (stored.size() != feature.size() || stored.empty()) {
            continue;
        }

        // 欧氏距离：越小说明两张脸越接近
        double sumSq = 0.0;
        // 归一化相关：先减去各自均值，再计算余弦相似度
        double meanA = 0.0;
        double meanB = 0.0;
        for (size_t i = 0; i < feature.size(); ++i) {
            double diff = feature[i] - stored[i];
            sumSq += diff * diff;
            meanA += feature[i];
            meanB += stored[i];
        }
        double distance = std::sqrt(sumSq);
        meanA /= static_cast<double>(feature.size());
        meanB /= static_cast<double>(feature.size());

        double dot = 0.0;
        double normA = 0.0;
        double normB = 0.0;
        for (size_t i = 0; i < feature.size(); ++i) {
            double a = feature[i] - meanA;
            double b = stored[i] - meanB;
            dot += a * b;
            normA += a * a;
            normB += b * b;
        }
        double corr = dot / (std::sqrt(normA) * std::sqrt(normB) + 1e-9);

        if (distance < bestDistance) {
            bestDistance = distance;
            bestCorr = corr;
            bestUser = info.user();
        }
    }

    // 阈值由实际摄像头画面调节；距离越小、相关度越高越可靠
    if (!bestUser.isEmpty() && bestDistance <= 22.0 && bestCorr >= 0.55) {
        user = bestUser;
        score = std::max(0.0, (1.0 - bestDistance / 30.0) * 100.0);
        return true;
    }

    score = 0.0;
    return false;
}

// 显示今日考勤记录
void MainWindow::refreshAttendanceTable()
{
    QString today = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QList<attendanceInfo> list =
            databasecontrol::instance()->queryAttendanceByDate(today);

    ui->tableWidget_attendance->clear();
    ui->tableWidget_attendance->setColumnCount(6);
    ui->tableWidget_attendance->setRowCount(list.size());
    ui->tableWidget_attendance->setHorizontalHeaderLabels(
                {"日期", "时间", "用户名", "姓名", "部门", "状态"});

    for (int i = 0; i < list.size(); ++i) {
        const attendanceInfo &info = list.at(i);
        ui->tableWidget_attendance->setItem(i, 0, new QTableWidgetItem(info.punchDate));
        ui->tableWidget_attendance->setItem(i, 1, new QTableWidgetItem(info.punchTime));
        ui->tableWidget_attendance->setItem(i, 2, new QTableWidgetItem(info.user));
        ui->tableWidget_attendance->setItem(i, 3, new QTableWidgetItem(info.realName));
        ui->tableWidget_attendance->setItem(i, 4, new QTableWidgetItem(info.department));
        ui->tableWidget_attendance->setItem(i, 5, new QTableWidgetItem(info.status));
    }

    ui->tableWidget_attendance->horizontalHeader()->setSectionResizeMode(
                QHeaderView::Stretch);
}

bool MainWindow::detectFace(cv::Mat &frame, cv::Rect &faceRect)
{
    if (faceCascade.empty()) {
        return false;
    }

    cv::Mat gray;
    if (frame.channels() == 3) {
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = frame;
    }
    cv::equalizeHist(gray, gray);

    std::vector<cv::Rect> faces;
    faceCascade.detectMultiScale(gray, faces, 1.1, 3, 0, cv::Size(80, 80));
    if (!faces.empty()) {
        faceRect = faces[0];
        return true;
    }
    return false;
}

// 与注册界面保持一致：灰度、缩放 100x100、归一化到 0~1
std::vector<float> MainWindow::extractFaceFeature(const cv::Mat &faceImage)
{
    std::vector<float> feature;
    cv::Mat gray;
    if (faceImage.channels() == 3) {
        cv::cvtColor(faceImage, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = faceImage;
    }

    cv::Mat resized;
    cv::resize(gray, resized, cv::Size(100, 100));
    feature.reserve(resized.total());
    for (int i = 0; i < resized.rows; ++i) {
        for (int j = 0; j < resized.cols; ++j) {
            feature.push_back(static_cast<float>(resized.at<uchar>(i, j)) / 255.0f);
        }
    }
    return feature;
}

// 释放摄像头资源
void MainWindow::closeCamera()
{
    if (isCameraOpen) {
        cap.release();
        isCameraOpen = false;
        if (cameraTimer) {
            cameraTimer->stop();
        }
    }
}
