#include "registration_dialog.h"
#include "ui_registration_dialog.h"
#include "databasecontrol.h"
#include "userinfo.h"

#include <QDebug>
#include <QApplication>
#include <QMessageBox>
#include <QFile>
#include <QCoreApplication>
#include <opencv2/opencv.hpp>

registration_Dialog::registration_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::registration_Dialog),
    isCameraOpen(false),
    isCapturing(false),
    hasFaceFeature(false)
{
    ui->setupUi(this);

    // 初始化摄像头定时器
    cameraTimer = new QTimer(this);
    connect(cameraTimer, &QTimer::timeout, this, &registration_Dialog::updateCamera);

    // 加载人脸检测模型
    QString cascadePath = QCoreApplication::applicationDirPath() + "/haarcascade_frontalface_default.xml";
    if (!QFile::exists(cascadePath)) {
        qDebug() << "人脸检测模型文件不存在:" << cascadePath;
        QMessageBox::warning(this, "警告", "人脸检测模型文件不存在，请将 haarcascade_frontalface_default.xml 放到程序目录");
    } else {
        if (!faceCascade.load(cascadePath.toStdString())) {
            qDebug() << "加载人脸检测模型失败";
        } else {
            qDebug() << "人脸检测模型加载成功";
        }
    }

    // 禁用注册按钮，直到人脸录入完成或用户取消
    ui->pushButton_ensure->setEnabled(false);

    // 如果用户取消勾选“启用并录入人脸”，允许直接完成注册
    connect(ui->checkBox_use_face, &QCheckBox::toggled, this,
            [this](bool checked) {
        if (!checked) {
            ui->pushButton_ensure->setEnabled(true);
        }
    });
}

// 析构函数 - 只定义一次
registration_Dialog::~registration_Dialog()
{
    closeCamera();
    delete ui;
}

void registration_Dialog::closeCamera()
{
    if (isCameraOpen) {
        cap.release();
        isCameraOpen = false;
        cameraTimer->stop();
    }
}

void registration_Dialog::on_pushButton_ensure_clicked()
{
    // 检查用户是否已存在
    QString user = ui->lineEdit_user->text().trimmed();
    QString password = ui->lineEdit_password->text().trimmed();

    if (user.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "提示", "用户名和密码不能为空");
        return;
    }

    if (databasecontrol::instance()->userExists(user)) {
        QMessageBox::warning(this, "提示", "用户名已存在，请重新输入");
        return;
    }

    // 如果启用了人脸识别但未采集人脸特征，提示
    if (ui->checkBox_use_face->isChecked() && !hasFaceFeature) {
        int ret = QMessageBox::question(this, "提示",
                                       "您启用了人脸识别但未录入人脸，是否继续注册？",
                                       QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::No) {
            return;
        }
    }

    // 收集扩展资料，注册时可一并保存到数据库
    QString realName = ui->lineEdit_realName->text().trimmed();
    QString department = ui->lineEdit_department->text().trimmed();
    QString position = ui->lineEdit_position->text().trimmed();
    QString phone = ui->lineEdit_phone->text().trimmed();

    // 创建用户
    QByteArray faceFeature;
    if (hasFaceFeature && !currentFeature.empty()) {
        faceFeature = userInfo::vectorToByteArray(currentFeature);
    }

    userInfo newUser(user, password.toInt(), faceFeature,
                     realName, department, position, phone);

    if (databasecontrol::instance()->addUserinfo(newUser)) {
        QMessageBox::information(this, "成功", "注册成功！");
        closeCamera();
        emit return_success();
        this->close();
    } else {
        QMessageBox::critical(this, "错误", "注册失败，请检查数据库连接");
    }
}

void registration_Dialog::on_pushButton_Return_clicked()
{
    closeCamera();
    emit return_success();
}

void registration_Dialog::on_pushButton_face_clicked()
{
    // 验证用户名和密码
    QString user = ui->lineEdit_user->text().trimmed();
    QString password = ui->lineEdit_password->text().trimmed();

    if (user.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先输入用户名和密码");
        return;
    }

    if (databasecontrol::instance()->userExists(user)) {
        QMessageBox::warning(this, "提示", "用户名已存在，请重新输入");
        return;
    }

    // 如果摄像头已打开，关闭并重置
    if (isCameraOpen) {
        closeCamera();
        ui->pushButton_face->setText("录入人脸");
        ui->label_camera->setText("摄像头画面");
        ui->label_status->setText("状态：已取消");
        ui->label_status->setStyleSheet("color: gray;");
        return;
    }

    // 打开摄像头
    cap.open(0);
    if (!cap.isOpened()) {
        QMessageBox::warning(this, "错误", "无法打开摄像头，请检查摄像头连接");
        return;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 320);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 240);
    isCameraOpen = true;
    isCapturing = false;
    hasFaceFeature = false;
    cameraTimer->start(30);

    ui->pushButton_face->setText("取消录入");
    ui->label_status->setText("状态：正在检测人脸...");
    ui->label_status->setStyleSheet("color: blue;");
    ui->pushButton_ensure->setEnabled(false);

    // 启动自动采集（延迟2秒后开始自动检测）
    QTimer::singleShot(2000, this, &registration_Dialog::autoCaptureAndSave);
}

void registration_Dialog::updateCamera()
{
    if (!isCameraOpen) return;

    cap.read(currentFrame);
    if (currentFrame.empty()) {
        return;
    }

    // 检测人脸并绘制框
    cv::Mat displayFrame = currentFrame.clone();
    cv::Rect faceRect;
    bool hasFace = detectFace(displayFrame, faceRect);

    if (hasFace) {
        cv::rectangle(displayFrame, faceRect, cv::Scalar(0, 255, 0), 2);
        cv::putText(displayFrame, "Face Detected",
                    cv::Point(faceRect.x, faceRect.y - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);

        if (!isCapturing && !hasFaceFeature) {
            ui->label_status->setText("状态：✅ 检测到人脸，准备采集...");
            ui->label_status->setStyleSheet("color: green;");
        }
    } else {
        if (!isCapturing && !hasFaceFeature) {
            ui->label_status->setText("状态：未检测到人脸，请调整位置");
            ui->label_status->setStyleSheet("color: red;");
        }
    }

    // 显示画面
    cv::Mat rgbFrame;
    cv::cvtColor(displayFrame, rgbFrame, cv::COLOR_BGR2RGB);
    QImage img(rgbFrame.data, rgbFrame.cols, rgbFrame.rows,
               rgbFrame.step, QImage::Format_RGB888);
    ui->label_camera->setPixmap(QPixmap::fromImage(img).scaled(280, 150,
                               Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void registration_Dialog::autoCaptureAndSave()
{
    if (!isCameraOpen || currentFrame.empty()) {
        QTimer::singleShot(500, this, &registration_Dialog::autoCaptureAndSave);
        return;
    }

    QString user = ui->lineEdit_user->text().trimmed();
    if (user.isEmpty()) {
        ui->label_status->setText("状态：❌ 请先输入用户名");
        ui->label_status->setStyleSheet("color: red;");
        QMessageBox::warning(this, "提示", "请先输入用户名");
        closeCamera();
        ui->pushButton_face->setText("录入人脸");
        return;
    }

    cv::Rect faceRect;
    if (!detectFace(currentFrame, faceRect)) {
        ui->label_status->setText("状态：未检测到人脸，请调整位置");
        ui->label_status->setStyleSheet("color: red;");
        QTimer::singleShot(1000, this, &registration_Dialog::autoCaptureAndSave);
        return;
    }

    isCapturing = true;
    ui->label_status->setText("状态：正在提取人脸特征...");
    ui->label_status->setStyleSheet("color: orange;");

    cv::Mat faceROI = currentFrame(faceRect);
    currentFeature = extractFaceFeature(faceROI);

    if (currentFeature.empty()) {
        ui->label_status->setText("状态：❌ 特征提取失败，请重试");
        ui->label_status->setStyleSheet("color: red;");
        QMessageBox::warning(this, "提示", "人脸特征提取失败，请重试");
        isCapturing = false;
        closeCamera();
        ui->pushButton_face->setText("录入人脸");
        return;
    }

    hasFaceFeature = true;
    isCapturing = false;

    // 显示采集到的人脸
    cv::Mat rgbFace;
    cv::cvtColor(faceROI, rgbFace, cv::COLOR_BGR2RGB);
    QImage faceImg(rgbFace.data, rgbFace.cols, rgbFace.rows,
                   rgbFace.step, QImage::Format_RGB888);
    ui->label_face_preview->setPixmap(QPixmap::fromImage(faceImg).scaled(100, 100,
                                      Qt::KeepAspectRatio, Qt::SmoothTransformation));

    ui->label_status->setText("状态：✅ 人脸采集成功，正在保存...");
    ui->label_status->setStyleSheet("color: green;");

    closeCamera();
    ui->pushButton_face->setText("录入人脸");
    ui->label_camera->setText("摄像头画面");

    saveFaceFeatureToDatabase();
}

void registration_Dialog::saveFaceFeatureToDatabase()
{
    if (!hasFaceFeature || currentFeature.empty()) {
        QMessageBox::warning(this, "提示", "无人脸特征可保存");
        return;
    }

    QString user = ui->lineEdit_user->text().trimmed();
    QString password = ui->lineEdit_password->text().trimmed();

    if (user.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "提示", "用户名或密码为空");
        return;
    }

    // 只暂存人脸特征，等用户点击“完成注册”时再写数据库，
    // 避免旧逻辑在注册前就创建用户导致“用户名已存在”。
    ui->label_status->setText("状态：✅ 人脸录入成功，请点击完成注册");
    ui->label_status->setStyleSheet("color: green;");
    ui->pushButton_ensure->setEnabled(true);

    QMessageBox::information(this, "成功",
                            QString("人脸录入成功！\n用户名：%1\n请点击“完成注册”提交资料").arg(user));
}

bool registration_Dialog::detectFace(cv::Mat &frame, cv::Rect &faceRect)
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

std::vector<float> registration_Dialog::extractFaceFeature(const cv::Mat &faceImage)
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
    for (int i = 0; i < resized.rows; i++) {
        for (int j = 0; j < resized.cols; j++) {
            feature.push_back(static_cast<float>(resized.at<uchar>(i, j)) / 255.0f);
        }
    }

    return feature;
}
