/******************************************************************
Copyright © Deng Zhimao Co., Ltd. 1990-2021. All rights reserved.
* @projectName   05_opencv_camera
* @brief         camera.cpp
* @author        Deng Zhimao
* @email         1252699831@qq.com
* @net           www.openedv.com
* @date          2021-03-17
*******************************************************************/
#include "camera.h"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <QImage>
#include <QDebug>

Camera::Camera(QObject *parent) :
    QObject(parent)
{
    /* 实例化 */
    capture = new cv::VideoCapture();
    timer = new QTimer(this);

    /* 信号槽连接 */
    connect(timer, SIGNAL(timeout()), this, SLOT(timerTimeOut()));
}

Camera::~Camera()
{
    delete capture;
    capture = NULL;
}

void Camera::selectCameraDevice(int index)
{
    /* 如果有其他摄像头打开了，先释放 */
    if (capture->isOpened()) {
        capture->release();
    }

    /* 打开摄像头设备 */
    capture->open(index);
}

bool Camera::cameraProcess(bool bl)
{
    if (bl) {
        /* 为什么是33？1000/33约等于30帧，也就是一秒最多显示30帧  */
        timer->start(33);
    } else {
        timer->stop();
    }
    /* 返回摄像头的状态 */
    return capture->isOpened();
}

void Camera::timerTimeOut()
{
    /* 如果摄像头没有打开，停止定时器，返回 */
    if (!capture->isOpened()) {
        timer->stop();
        return;
    }

    static cv::Mat frame;
    static cv::Mat rotatedFrame;
    *capture >> frame;
    if (!frame.empty()) {
        cv::flip(frame, rotatedFrame, 0);
        /* 发送图片信号 */
        emit readyImage(matToQImage(rotatedFrame));
    }
}

QImage Camera::matToQImage(const cv::Mat &img)
{
    /* USB摄像头和OV5640等都是RGB三通道，不考虑单/四通道摄像头 */
    if (img.type() == CV_8UC3) {
        cv::Mat rgb;
#if defined(CV_VERSION_MAJOR) && (CV_VERSION_MAJOR >= 3)
        cv::cvtColor(img, rgb, cv::COLOR_BGR2RGB);
#else
        cv::cvtColor(img, rgb, CV_BGR2RGB);
#endif
        QImage qImage((const uchar*)rgb.data, rgb.cols, rgb.rows, rgb.step,
                      QImage::Format_RGB888);
        return qImage.copy();
    }

    if (img.type() == CV_8UC2) {
        cv::Mat rgb;
#if defined(CV_VERSION_MAJOR) && (CV_VERSION_MAJOR >= 3)
        cv::cvtColor(img, rgb, cv::COLOR_YUV2RGB_YUYV);
#else
        cv::cvtColor(img, rgb, CV_YUV2RGB_YUYV);
#endif
        QImage qImage((const uchar*)rgb.data, rgb.cols, rgb.rows, rgb.step,
                      QImage::Format_RGB888);
        return qImage.copy();
    }

    if (img.type() == CV_8UC1) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
        QImage qImage((const uchar*)img.data, img.cols, img.rows, img.step,
                      QImage::Format_Grayscale8);
        return qImage.copy();
#else
        QImage qImage((const uchar*)img.data, img.cols, img.rows, img.step,
                      QImage::Format_Indexed8);
        QVector<QRgb> colorTable;
        colorTable.reserve(256);
        for (int i = 0; i < 256; i++)
            colorTable.push_back(qRgb(i, i, i));
        qImage.setColorTable(colorTable);
        return qImage.copy();
#endif
    }

    /* 返回QImage */
    return QImage();
}
