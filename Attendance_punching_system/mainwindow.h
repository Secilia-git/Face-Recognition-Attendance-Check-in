#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QImage>
#include <QList>
#include <vector>
#include <opencv2/opencv.hpp>
#include "userinfo.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// 主窗口：通过摄像头识别人脸并完成考勤打卡
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    // 窗口关闭时释放摄像头，避免程序退出后摄像头仍被占用
    void closeEvent(QCloseEvent *event) override;

private slots:
    void updateCamera();                         // 摄像头定时刷新画面
    void on_pushButton_start_clicked();          // 开始人脸打卡
    void on_pushButton_stop_clicked();           // 停止人脸打卡
    void on_pushButton_refresh_clicked();        // 刷新今日考勤记录

private:
    Ui::MainWindow *ui;

    cv::VideoCapture cap;                        // 摄像头对象
    QTimer *cameraTimer;                         // 画面刷新定时器
    bool isCameraOpen;                           // 摄像头是否打开
    bool isPunchProcessing;                      // 是否正在处理一次打卡

    cv::CascadeClassifier faceCascade;           // OpenCV 人脸检测模型
    QList<userInfo> faceLibrary;                 // 数据库中的注册用户及人脸特征

    bool detectFace(cv::Mat &frame, cv::Rect &faceRect);
    std::vector<float> extractFaceFeature(const cv::Mat &faceImage);
    bool recognizeFace(const std::vector<float> &feature,
                       QString &user,
                       double &score);
    void refreshAttendanceTable();
    void closeCamera();
};

#endif // MAINWINDOW_H
