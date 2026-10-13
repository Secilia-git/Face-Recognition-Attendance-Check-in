#ifndef REGISTRATION_DIALOG_H
#define REGISTRATION_DIALOG_H

#include <QDialog>
#include <QTimer>
#include <QImage>
#include <opencv2/opencv.hpp>
#include <vector>

namespace Ui {
class registration_Dialog;
}

class registration_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit registration_Dialog(QWidget *parent = nullptr);
    ~registration_Dialog();

private slots:
    void on_pushButton_ensure_clicked();
    void on_pushButton_Return_clicked();
    void on_pushButton_face_clicked();


    void updateCamera();           // 更新摄像头画面
    void autoCaptureAndSave();     // 自动采集并保存人脸特征

signals:
    void return_success();

private:
    Ui::registration_Dialog *ui;

    // 摄像头相关
    cv::VideoCapture cap;
    cv::Mat currentFrame;
    QTimer *cameraTimer;
    bool isCameraOpen;
    bool isCapturing;

    // 人脸特征
    std::vector<float> currentFeature;
    bool hasFaceFeature;

    // 人脸检测器
    cv::CascadeClassifier faceCascade;

    // 辅助方法
    bool detectFace(cv::Mat &frame, cv::Rect &faceRect);
    std::vector<float> extractFaceFeature(const cv::Mat &faceImage);
    void closeCamera();
    void saveFaceFeatureToDatabase();
};

#endif // REGISTRATION_DIALOG_H
