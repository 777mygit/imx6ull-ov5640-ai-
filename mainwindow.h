/******************************************************************
Copyright © Deng Zhimao Co., Ltd. 1990-2021. All rights reserved.
* @projectName   05_opencv_camera
* @brief         mainwindow.h
* @author        Deng Zhimao
* @email         1252699831@qq.com
* @net           www.openedv.com
* @date          2021-03-17
*******************************************************************/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QDebug>
#include <QAbstractSocket>

class Camera;
class QNetworkAccessManager;
class QNetworkReply;
class QSslError;
class QSslSocket;
class QDialog;
class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QStackedWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event);

private:
    /* 主容器，Widget也可以当作一种容器 */
    QWidget *mainWidget;
    QStackedWidget *stackedWidget;
    QWidget *cameraPage;
    QWidget *wifiPage;

    /* 滚动区域，方便开发高分辨率 */
    QScrollArea *scrollArea;

    /* 将采集到的图像使用Widget显示 */
    QLabel *displayLabel;
    QLabel *beautifyStatusLabel;

    /* 界面右侧区域布局 */
    QHBoxLayout *hboxLayout;

    /* 界面右侧区域布局 */
    QVBoxLayout *vboxLayout;

    /* 界面右侧区域容器 */
    QWidget *rightWidget;

    /* 界面右侧区域显示拍照的图片 */
    QLabel *photoLabel;

    /* 界面右侧区域摄像头设备下拉选择框 */
    QComboBox *comboBox;
    QComboBox *styleComboBox;

    /* 三个按钮：拍照、开关、云端美颜 */
    QPushButton *pushButton[3];
    QPushButton *albumButton;
    QPushButton *wifiButton;
    QPushButton *wifiPowerButton;
    QPushButton *wifiRefreshButton;
    QPushButton *wifiConnectButton;
    QPushButton *wifiBackButton;
    QLabel *wifiStatusLabel;
    QListWidget *wifiListWidget;
    QLineEdit *wifiSsidEdit;
    QLineEdit *wifiPasswordEdit;

    /* 拍照保存的照片 */
    QImage saveImage;
    QImage capturedPhotoImage;
    QImage beautyImage;
    QImage beautifyRequestImage;

    /* 摄像头设备 */
    Camera *camera;

    QNetworkAccessManager *networkManager;
    QNetworkReply *beautifyApiReply;
    QNetworkReply *beautifyDownloadReply;

    QSslSocket *apiSocket;
    QSslSocket *downloadSocket;
    QByteArray apiResponseBytes;
    QByteArray downloadResponseBytes;
    QByteArray apiRequestBytes;
    QByteArray downloadRequestBytes;
    QString pendingDownloadUrl;
    QString beautifyResultPrefix;
    QString beautifyPendingStyle;
    QLabel *beautifyResultLabel;
    QLabel *beautifyResultStatusLabel;
    QPushButton *beautifyResultButton;

    QDialog *albumDialog;
    QListWidget *albumListWidget;
    QDialog *previewDialog;
    QLabel *previewImageLabel;
    QLabel *previewStatusLabel;
    QComboBox *previewStyleComboBox;
    QPushButton *previewBeautifyButton;
    QString previewImagePath;

    /* 布局初始化 */
    void layoutInit();
    void initWifiPage();

    /* 扫描是否存在摄像头 */
    void scanCameraDevice();

    QByteArray readFirstLine(const QString &path) const;
    QString detectTfMountPoint() const;
    QString ensureAlbumDir(const QString &baseDir) const;
    QString saveImageToAlbum(const QImage &image, const QString &prefix) const;
    QString albumDirPath() const;
    QString wifiConfigPath() const;
    void loadWifiCredentials(QString *ssid, QString *psk) const;
    void saveWifiCredentials(const QString &ssid, const QString &psk) const;
    QString promptForStyle(const QString &style) const;
    QString prefixForStyle(const QString &style) const;
    bool runCommand(const QString &program, const QStringList &args,
                    int timeoutMs, QString *stdOut = nullptr,
                    QString *stdErr = nullptr) const;
    bool runCommandAndLog(const QString &program,
                          const QStringList &args,
                          int timeoutMs) const;
    bool hasWlan0Ipv4() const;
    bool waitForWlan0Ipv4(int timeoutMs, int stepMs) const;
    bool hasWpaCtrlSocket() const;
    bool ensureWifiControlReady(QString *errorMessage = nullptr) const;
    void ensureSystemTimeReasonable() const;
    void preferWlan0AsDefaultRoute() const;
    bool ensureWifiConnected(const QString &ssid, const QString &psk,
                             QString *errorMessage = nullptr) const;
    QStringList scanNearbyWifi(QString *errorMessage = nullptr) const;
    QString currentWifiSsid() const;
    QString currentWifiIpv4() const;
    bool isWifiPoweredOn() const;
    void updateWifiStatus(const QString &message = QString());
    QString promptTextWithSoftKeyboard(const QString &title,
                                       const QString &initialText,
                                       bool passwordMode,
                                       bool *accepted);
    void setPreviewImage(const QImage &image);
    void startApiRequestOverSsl(const QByteArray &apiKey, const QByteArray &payloadBytes);
    void startDownloadRequestOverSsl(const QString &url);
    static int extractContentLengthFromHttpHeaders(const QByteArray &response);
    static bool isHttpResponseComplete(const QByteArray &response);
    static QByteArray extractHttpBody(const QByteArray &response, int *statusCode, bool *chunked);
    static QByteArray decodeChunkedBody(const QByteArray &body);

private slots:
    /* 显示图像 */
    void showImage(const QImage&);

    /* 设置按钮文本 */
    void setButtonText(bool);

    /* 保存照片到本地 */
    void saveImageToLocal();
    void openWifiPage();
    void closeWifiPage();
    void toggleWifiPower();
    void refreshWifiList();
    void onWifiItemActivated(QListWidgetItem *item);
    void connectToWifi();
    void openAlbum();
    void refreshAlbum();
    void onAlbumItemActivated(QListWidgetItem *item);
    void onPreviewBeautifyClicked();

    void beautifyCurrentImage();
    void onBeautifyApiFinished();
    void onBeautifyDownloadFinished();
    void onBeautifyApiSslErrors(const QList<QSslError> &errors);
    void onBeautifyDownloadSslErrors(const QList<QSslError> &errors);

    void onApiSocketEncrypted();
    void onApiSocketReadyRead();
    void onApiSocketDisconnected();
    void onApiSocketError(QAbstractSocket::SocketError);
    void onApiSocketSslErrors(const QList<QSslError> &errors);

    void onDownloadSocketEncrypted();
    void onDownloadSocketReadyRead();
    void onDownloadSocketDisconnected();
    void onDownloadSocketError(QAbstractSocket::SocketError);
    void onDownloadSocketSslErrors(const QList<QSslError> &errors);
};
#endif // MAINWINDOW_H
