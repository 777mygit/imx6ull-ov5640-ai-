/******************************************************************
Copyright © Deng Zhimao Co., Ltd. 1990-2021. All rights reserved.
* @projectName   05_opencv_camera
* @brief         mainwindow.cpp
* @author        Deng Zhimao
* @email         1252699831@qq.com
* @net           www.openedv.com
* @date          2021-03-17
*******************************************************************/
#include "mainwindow.h"
#include <QGuiApplication>
#include <QScreen>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QDialog>
#include <QEvent>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QListWidgetItem>
#include <QAbstractItemView>
#include <QPixmap>
#include <QBuffer>
#include <QTextStream>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QAbstractSocket>
#include <QSslSocket>
#include <QSslError>
#include <QSslConfiguration>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStackedWidget>
#include <QUrl>
#include <QThread>
#include "camera.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    networkManager = new QNetworkAccessManager(this);
    beautifyApiReply = nullptr;
    beautifyDownloadReply = nullptr;
    apiSocket = nullptr;
    downloadSocket = nullptr;
    stackedWidget = nullptr;
    cameraPage = nullptr;
    wifiPage = nullptr;
    styleComboBox = nullptr;
    albumButton = nullptr;
    wifiButton = nullptr;
    wifiPowerButton = nullptr;
    wifiRefreshButton = nullptr;
    wifiConnectButton = nullptr;
    wifiBackButton = nullptr;
    wifiStatusLabel = nullptr;
    wifiListWidget = nullptr;
    wifiSsidEdit = nullptr;
    wifiPasswordEdit = nullptr;
    beautifyResultLabel = nullptr;
    beautifyResultStatusLabel = nullptr;
    beautifyResultButton = nullptr;
    albumDialog = nullptr;
    albumListWidget = nullptr;
    previewDialog = nullptr;
    previewImageLabel = nullptr;
    previewStatusLabel = nullptr;
    previewStyleComboBox = nullptr;
    previewBeautifyButton = nullptr;

    qDebug()<<"SSL支持="<<QSslSocket::supportsSsl()
           <<"build="<<QSslSocket::sslLibraryBuildVersionString()
           <<"runtime="<<QSslSocket::sslLibraryVersionString()
           <<endl;

    /* 布局初始化 */
    layoutInit();

    /* 扫描摄像头 */
    scanCameraDevice();
}

MainWindow::~MainWindow()
{
}

void MainWindow::initWifiPage()
{
    QLabel *titleLabel = new QLabel("WiFi设置", wifiPage);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel{font-size:28px;font-weight:bold;}");

    wifiPowerButton = new QPushButton("打开WiFi", wifiPage);
    wifiRefreshButton = new QPushButton("刷新扫描附近WiFi", wifiPage);
    wifiConnectButton = new QPushButton("连接WiFi", wifiPage);
    wifiBackButton = new QPushButton("返回", wifiPage);
    wifiStatusLabel = new QLabel(wifiPage);
    wifiListWidget = new QListWidget(wifiPage);
    wifiSsidEdit = new QLineEdit(wifiPage);
    wifiPasswordEdit = new QLineEdit(wifiPage);

    wifiStatusLabel->setWordWrap(true);
    wifiStatusLabel->setMinimumHeight(60);
    wifiStatusLabel->setStyleSheet("QLabel{padding:8px;border:1px solid #bfbfbf;background:#f7f7f7;}");

    wifiListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    wifiListWidget->setAlternatingRowColors(true);

    wifiSsidEdit->setPlaceholderText("请选择或输入WiFi名称");
    wifiPasswordEdit->setPlaceholderText("请输入WiFi密码");
    wifiPasswordEdit->setEchoMode(QLineEdit::Password);
    wifiSsidEdit->setReadOnly(true);
    wifiPasswordEdit->setReadOnly(true);
    wifiSsidEdit->installEventFilter(this);
    wifiPasswordEdit->installEventFilter(this);

    QHBoxLayout *topButtonLayout = new QHBoxLayout();
    topButtonLayout->addWidget(wifiPowerButton);
    topButtonLayout->addWidget(wifiRefreshButton);

    QHBoxLayout *ssidLayout = new QHBoxLayout();
    ssidLayout->addWidget(new QLabel("WiFi名称:", wifiPage));
    ssidLayout->addWidget(wifiSsidEdit, 1);

    QHBoxLayout *passwordLayout = new QHBoxLayout();
    passwordLayout->addWidget(new QLabel("密码:", wifiPage));
    passwordLayout->addWidget(wifiPasswordEdit, 1);

    QHBoxLayout *bottomButtonLayout = new QHBoxLayout();
    bottomButtonLayout->addWidget(wifiConnectButton);
    bottomButtonLayout->addWidget(wifiBackButton);

    QVBoxLayout *layout = new QVBoxLayout(wifiPage);
    layout->addWidget(titleLabel);
    layout->addLayout(topButtonLayout);
    layout->addWidget(wifiStatusLabel);
    layout->addWidget(wifiListWidget, 1);
    layout->addLayout(ssidLayout);
    layout->addLayout(passwordLayout);
    layout->addLayout(bottomButtonLayout);
    wifiPage->setLayout(layout);

    connect(wifiPowerButton, SIGNAL(clicked()),
            this, SLOT(toggleWifiPower()));
    connect(wifiRefreshButton, SIGNAL(clicked()),
            this, SLOT(refreshWifiList()));
    connect(wifiConnectButton, SIGNAL(clicked()),
            this, SLOT(connectToWifi()));
    connect(wifiBackButton, SIGNAL(clicked()),
            this, SLOT(closeWifiPage()));
    connect(wifiListWidget, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SLOT(onWifiItemActivated(QListWidgetItem*)));
    connect(wifiListWidget, SIGNAL(itemActivated(QListWidgetItem*)),
            this, SLOT(onWifiItemActivated(QListWidgetItem*)));
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == wifiSsidEdit || watched == wifiPasswordEdit) &&
            (event->type() == QEvent::MouseButtonPress ||
             event->type() == QEvent::MouseButtonDblClick)) {
        QLineEdit *targetEdit = qobject_cast<QLineEdit *>(watched);
        if (!targetEdit)
            return QMainWindow::eventFilter(watched, event);

        const bool isPassword = (targetEdit == wifiPasswordEdit);
        bool accepted = false;
        const QString text = promptTextWithSoftKeyboard(
                    isPassword ? QString("输入WiFi密码") : QString("输入WiFi名称"),
                    targetEdit->text(),
                    isPassword,
                    &accepted);
        if (accepted)
            targetEdit->setText(text);
        return true;
    }

    return QMainWindow::eventFilter(watched, event);
}

QByteArray MainWindow::readFirstLine(const QString &path) const
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QByteArray();
    QByteArray line = f.readLine();
    return line.trimmed();
}

QString MainWindow::wifiConfigPath() const
{
    const QString mountPoint = detectTfMountPoint();
    if (!mountPoint.isEmpty())
        return mountPoint + "/wifi.conf";
    return QCoreApplication::applicationDirPath() + "/wifi.conf";
}

void MainWindow::loadWifiCredentials(QString *ssid, QString *psk) const
{
    if (ssid)
        ssid->clear();
    if (psk)
        psk->clear();

    QStringList candidatePaths;
    const QString mountPoint = detectTfMountPoint();
    if (!mountPoint.isEmpty())
        candidatePaths << (mountPoint + "/wifi.conf");
    candidatePaths << (QCoreApplication::applicationDirPath() + "/wifi.conf");

    QString parsedSsid;
    QString parsedPsk;
    for (const QString &path : candidatePaths) {
        QFile conf(path);
        if (!conf.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;

        const QString content = QString::fromLocal8Bit(conf.readAll());
        const QStringList parts = content.split(QRegExp("[\r\n,]"),
                                               QString::SkipEmptyParts);
        for (const QString &rawPart : parts) {
            const QString part = rawPart.trimmed();
            if (part.startsWith("ssid="))
                parsedSsid = part.mid(5).trimmed();
            else if (part.startsWith("psk="))
                parsedPsk = part.mid(4).trimmed();
        }

        if (!parsedSsid.isEmpty() || !parsedPsk.isEmpty())
            break;
    }

    if (ssid)
        *ssid = parsedSsid;
    if (psk)
        *psk = parsedPsk;
}

void MainWindow::saveWifiCredentials(const QString &ssid, const QString &psk) const
{
    QFile conf(wifiConfigPath());
    if (!conf.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream ts(&conf);
    ts << "ssid=" << ssid << "\n";
    ts << "psk=" << psk << "\n";
    ts.flush();
    conf.close();
}

bool MainWindow::runCommand(const QString &program, const QStringList &args,
                            int timeoutMs, QString *stdOut, QString *stdErr) const
{
    QProcess p;
    p.start(program, args);
    if (!p.waitForStarted(timeoutMs))
        return false;
    if (!p.waitForFinished(timeoutMs)) {
        p.kill();
        p.waitForFinished(1000);
        return false;
    }

    if (stdOut)
        *stdOut = QString::fromLocal8Bit(p.readAllStandardOutput());
    if (stdErr)
        *stdErr = QString::fromLocal8Bit(p.readAllStandardError());
    return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

bool MainWindow::runCommandAndLog(const QString &program,
                                  const QStringList &args,
                                  int timeoutMs) const
{
    QString out;
    QString err;
    const bool ok = runCommand(program, args, timeoutMs, &out, &err);
    if (!ok || !out.trimmed().isEmpty() || !err.trimmed().isEmpty()) {
        qDebug()<<"cmd"<<program<<args
               <<"ok="<<ok
               <<"out="<<out.trimmed()
               <<"err="<<err.trimmed()
               <<endl;
    }
    return ok;
}

bool MainWindow::hasWlan0Ipv4() const
{
    const QNetworkInterface iface = QNetworkInterface::interfaceFromName("wlan0");
    if (!iface.isValid())
        return false;

    const QList<QNetworkAddressEntry> entries = iface.addressEntries();
    for (const QNetworkAddressEntry &entry : entries) {
        const QHostAddress ip = entry.ip();
        if (ip.protocol() == QAbstractSocket::IPv4Protocol &&
                ip != QHostAddress::AnyIPv4 &&
                !ip.isNull())
            return true;
    }
    return false;
}

bool MainWindow::waitForWlan0Ipv4(int timeoutMs, int stepMs) const
{
    int waited = 0;
    while (waited < timeoutMs) {
        if (hasWlan0Ipv4())
            return true;
        QThread::msleep(static_cast<unsigned long>(stepMs));
        waited += stepMs;
    }
    return hasWlan0Ipv4();
}

bool MainWindow::hasWpaCtrlSocket() const
{
    return QFile::exists("/var/run/wpa_supplicant/wlan0");
}

bool MainWindow::ensureWifiControlReady(QString *errorMessage) const
{
#if win32
    if (errorMessage)
        *errorMessage = "Windows下未实现开发板WiFi控制";
    return false;
#else
    const bool hasInterface = QFile::exists("/sys/class/net/wlan0")
            || QNetworkInterface::interfaceFromName("wlan0").isValid();
    if (!hasInterface) {
        if (errorMessage)
            *errorMessage = "未检测到wlan0，请确认USB WiFi网卡已插入";
        return false;
    }

    runCommandAndLog("ifconfig", QStringList() << "wlan0" << "up", 3000);
    if (hasWpaCtrlSocket())
        return true;

    const QString confPath = "/var/volatile/tmp/opencv_camera_scan.conf";
    QFile conf(confPath);
    if (conf.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ts(&conf);
        ts << "ctrl_interface=/var/run/wpa_supplicant\n";
        ts << "update_config=1\n";
        ts.flush();
        conf.close();
    }

    QString out;
    QString err;
    runCommand("wpa_supplicant",
               QStringList() << "-B" << "-i" << "wlan0"
                             << "-c" << confPath
                             << "-D" << "nl80211,wext",
               5000, &out, &err);
    if (!hasWpaCtrlSocket()) {
        out.clear();
        err.clear();
        runCommand("wpa_supplicant",
                   QStringList() << "-B" << "-i" << "wlan0"
                                 << "-c" << confPath
                                 << "-D" << "wext",
                   5000, &out, &err);
    }

    if (!hasWpaCtrlSocket()) {
        if (errorMessage)
            *errorMessage = QString("wpa_supplicant启动失败: %1 %2")
                    .arg(out.trimmed(), err.trimmed()).trimmed();
        return false;
    }
    return true;
#endif
}

void MainWindow::ensureSystemTimeReasonable() const
{
    const int year = QDateTime::currentDateTimeUtc().date().year();
    if (year >= 2020)
        return;

    runCommandAndLog("ntpd", QStringList() << "-n" << "-q" << "-p" << "pool.ntp.org", 20000);
    runCommandAndLog("busybox", QStringList() << "ntpd" << "-n" << "-q" << "-p" << "pool.ntp.org", 20000);
    runCommandAndLog("ntpdate", QStringList() << "-u" << "pool.ntp.org", 20000);
    runCommandAndLog("busybox", QStringList() << "ntpdate" << "-u" << "pool.ntp.org", 20000);
    runCommandAndLog("date", QStringList(), 3000);
}

void MainWindow::preferWlan0AsDefaultRoute() const
{
#if win32
    return;
#else
    QString out;
    QString err;
    if (!runCommand("route", QStringList() << "-n", 3000, &out, &err))
        return;

    QString wlanGw;
    QStringList eth1DefaultGws;
    QStringList wlan0DefaultGws;
    const QStringList lines = out.split('\n', QString::SkipEmptyParts);
    for (const QString &line : lines) {
        const QString l = line.trimmed();
        if (l.isEmpty())
            continue;
        if (l.startsWith("Destination") || l.startsWith("Kernel"))
            continue;

        const QStringList parts = l.split(QRegExp("\\s+"), QString::SkipEmptyParts);
        if (parts.size() < 8)
            continue;

        const QString dest = parts.at(0);
        const QString gw = parts.at(1);
        const QString flags = parts.at(3);
        const QString iface = parts.at(7);
        if (dest == "0.0.0.0" && iface == "wlan0" &&
                flags.contains("G") && gw != "0.0.0.0")
            wlanGw = gw;
        if (dest == "0.0.0.0" && iface == "wlan0")
            wlan0DefaultGws.append(gw);
        if (dest == "0.0.0.0" && iface == "eth1")
            eth1DefaultGws.append(gw);
    }

    if (wlanGw.isEmpty())
        return;

    const auto deleteDefaultRoute = [this](const QString &iface, const QString &gw) {
        if (!gw.isEmpty() && gw != "0.0.0.0") {
            runCommandAndLog("route",
                             QStringList() << "del" << "default"
                                           << "gw" << gw
                                           << "dev" << iface,
                             3000);
            runCommandAndLog("route",
                             QStringList() << "del" << "default"
                                           << "gw" << gw
                                           << iface,
                             3000);
            runCommandAndLog("busybox",
                             QStringList() << "route" << "del" << "default"
                                           << "gw" << gw
                                           << "dev" << iface,
                             3000);
        }

        runCommandAndLog("route",
                         QStringList() << "del" << "default"
                                       << "dev" << iface,
                         3000);
        runCommandAndLog("route",
                         QStringList() << "del" << "default"
                                       << iface,
                         3000);
        runCommandAndLog("route",
                         QStringList() << "del" << "-net" << "0.0.0.0"
                                       << "netmask" << "0.0.0.0"
                                       << "dev" << iface,
                         3000);
        runCommandAndLog("busybox",
                         QStringList() << "route" << "del" << "default"
                                       << "dev" << iface,
                         3000);
        runCommandAndLog("busybox",
                         QStringList() << "route" << "del" << "-net" << "0.0.0.0"
                                       << "netmask" << "0.0.0.0"
                                       << "dev" << iface,
                         3000);
        runCommandAndLog("ip",
                         QStringList() << "route" << "del" << "default"
                                       << "dev" << iface,
                         3000);
    };

    for (const QString &gw : eth1DefaultGws) {
        deleteDefaultRoute("eth1", gw);
    }

    for (const QString &gw : wlan0DefaultGws)
        deleteDefaultRoute("wlan0", gw);

    bool addOk = runCommandAndLog("route",
                                  QStringList() << "add" << "default"
                                                << "gw" << wlanGw
                                                << "dev" << "wlan0",
                                  3000);
    if (!addOk) {
        addOk = runCommandAndLog("busybox",
                                 QStringList() << "route" << "add" << "default"
                                               << "gw" << wlanGw
                                               << "dev" << "wlan0",
                                 3000);
    }
    if (!addOk) {
        addOk = runCommandAndLog("ip",
                                 QStringList() << "route" << "replace" << "default"
                                               << "via" << wlanGw
                                               << "dev" << "wlan0",
                                 3000);
    }
    if (!addOk) {
        runCommandAndLog("route",
                         QStringList() << "add" << "default"
                                       << "gw" << wlanGw
                                       << "wlan0",
                         3000);
    }

    QString afterOut;
    QString afterErr;
    if (runCommand("route", QStringList() << "-n", 3000, &afterOut, &afterErr))
        qDebug()<<"当前路由表\n"<<afterOut<<endl;
#endif
}

bool MainWindow::ensureWifiConnected(const QString &ssid, const QString &psk,
                                     QString *errorMessage) const
{
#if win32
    Q_UNUSED(ssid);
    Q_UNUSED(psk);
    if (errorMessage)
        *errorMessage = "Windows下未实现开发板WiFi连接";
    return false;
#else
    const QString trimmedSsid = ssid.trimmed();
    if (trimmedSsid.isEmpty()) {
        if (errorMessage)
            *errorMessage = "请选择要连接的WiFi名称";
        return false;
    }

    QString backendError;
    if (!ensureWifiControlReady(&backendError)) {
        if (errorMessage)
            *errorMessage = backendError;
        return false;
    }

    const QString connectedSsid = currentWifiSsid().trimmed();
    if (hasWlan0Ipv4() &&
            (connectedSsid.isEmpty() || connectedSsid == trimmedSsid)) {
        preferWlan0AsDefaultRoute();
        return true;
    }

    const auto escapeWpaString = [](QString value) -> QString {
        value.replace("\\", "\\\\");
        value.replace("\"", "\\\"");
        return value;
    };

    const auto wpaConnected = [this]() -> bool {
        QString out;
        QString err;
        if (!runCommand("wpa_cli", QStringList() << "-i" << "wlan0" << "status", 3000, &out, &err))
            return false;
        return out.contains("wpa_state=COMPLETED");
    };

    const auto tryWpaCliConnect = [this, trimmedSsid, psk, &escapeWpaString, &wpaConnected]() -> bool {
        QString out;
        QString err;
        runCommandAndLog("wpa_cli", QStringList() << "-i" << "wlan0" << "disconnect", 3000);
        runCommandAndLog("wpa_cli", QStringList() << "-i" << "wlan0" << "remove_network" << "all", 3000);
        if (!runCommand("wpa_cli", QStringList() << "-i" << "wlan0" << "add_network", 3000, &out, &err))
            return false;

        bool okId = false;
        const int netId = out.trimmed().toInt(&okId);
        if (!okId)
            return false;

        const QString quotedSsid = QString("\"%1\"").arg(escapeWpaString(trimmedSsid));
        if (!runCommandAndLog("wpa_cli",
                              QStringList() << "-i" << "wlan0"
                                            << "set_network" << QString::number(netId)
                                            << "ssid" << quotedSsid,
                              3000)) {
            return false;
        }

        if (psk.trimmed().isEmpty()) {
            if (!runCommandAndLog("wpa_cli",
                                  QStringList() << "-i" << "wlan0"
                                                << "set_network" << QString::number(netId)
                                                << "key_mgmt" << "NONE",
                                  3000)) {
                return false;
            }
        } else {
            const QString quotedPsk = QString("\"%1\"").arg(escapeWpaString(psk));
            if (!runCommandAndLog("wpa_cli",
                                  QStringList() << "-i" << "wlan0"
                                                << "set_network" << QString::number(netId)
                                                << "psk" << quotedPsk,
                                  3000)) {
                return false;
            }
        }

        runCommandAndLog("wpa_cli", QStringList() << "-i" << "wlan0" << "enable_network" << QString::number(netId), 3000);
        runCommandAndLog("wpa_cli", QStringList() << "-i" << "wlan0" << "select_network" << QString::number(netId), 3000);
        runCommandAndLog("wpa_cli", QStringList() << "-i" << "wlan0" << "reconnect", 3000);
        runCommandAndLog("wpa_cli", QStringList() << "-i" << "wlan0" << "save_config", 3000);

        runCommandAndLog("udhcpc", QStringList() << "-i" << "wlan0" << "-q" << "-n", 12000);

        return waitForWlan0Ipv4(12000, 500);
    };

    runCommandAndLog("ifconfig", QStringList() << "wlan0" << "up", 3000);
    if (tryWpaCliConnect()) {
        saveWifiCredentials(trimmedSsid, psk);
        preferWlan0AsDefaultRoute();
        return true;
    }

    const QString confPath = "/var/volatile/tmp/opencv_camera_wpa.conf";
    QFile conf(confPath);
    if (conf.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ts(&conf);
        ts << "ctrl_interface=/var/run/wpa_supplicant\n";
        ts << "update_config=1\n";
        ts << "network={\n";
        ts << "    ssid=\"" << escapeWpaString(trimmedSsid) << "\"\n";
        if (psk.trimmed().isEmpty())
            ts << "    key_mgmt=NONE\n";
        else {
            ts << "    psk=\"" << escapeWpaString(psk) << "\"\n";
            ts << "    key_mgmt=WPA-PSK\n";
        }
        ts << "}\n";
        ts.flush();
        conf.close();
    }

    runCommandAndLog("killall", QStringList() << "-q" << "wpa_supplicant", 3000);
    QString out;
    QString err;
    runCommand("wpa_supplicant",
               QStringList() << "-B" << "-i" << "wlan0"
                             << "-c" << confPath
                             << "-D" << "nl80211,wext",
               5000, &out, &err);
    if (!hasWpaCtrlSocket()) {
        out.clear();
        err.clear();
        runCommand("wpa_supplicant",
                   QStringList() << "-B" << "-i" << "wlan0"
                                 << "-c" << confPath
                                 << "-D" << "wext",
                   5000, &out, &err);
    }

    runCommandAndLog("udhcpc", QStringList() << "-i" << "wlan0" << "-q" << "-n", 12000);
    if (waitForWlan0Ipv4(12000, 500)) {
        saveWifiCredentials(trimmedSsid, psk);
        preferWlan0AsDefaultRoute();
        return true;
    }

    if (errorMessage)
        *errorMessage = QString("连接失败: %1 %2").arg(out.trimmed(), err.trimmed()).trimmed();
    return false;
#endif
}

QStringList MainWindow::scanNearbyWifi(QString *errorMessage) const
{
#if win32
    if (errorMessage)
        *errorMessage = "Windows下未实现开发板WiFi扫描";
    return QStringList();
#else
    QString backendError;
    if (!ensureWifiControlReady(&backendError)) {
        if (errorMessage)
            *errorMessage = backendError;
        return QStringList();
    }

    QStringList ssids;
    const auto appendUnique = [&ssids](const QString &ssid) {
        const QString trimmed = ssid.trimmed();
        if (!trimmed.isEmpty() && !ssids.contains(trimmed))
            ssids.append(trimmed);
    };

    QString out;
    QString err;
    if (runCommand("wpa_cli", QStringList() << "-i" << "wlan0" << "scan", 5000, &out, &err)) {
        QThread::msleep(2500);
        out.clear();
        err.clear();
        if (runCommand("wpa_cli", QStringList() << "-i" << "wlan0" << "scan_results", 5000, &out, &err)) {
            const QStringList lines = out.split('\n', QString::SkipEmptyParts);
            for (int i = 1; i < lines.size(); ++i) {
                const QString line = lines.at(i).trimmed();
                if (line.isEmpty())
                    continue;
                const QStringList parts = line.split('\t');
                if (parts.size() >= 5)
                    appendUnique(parts.mid(4).join("\t"));
            }
        }
    }

    if (ssids.isEmpty()) {
        out.clear();
        err.clear();
        if (runCommand("iwlist", QStringList() << "wlan0" << "scanning", 12000, &out, &err)) {
            const QStringList lines = out.split('\n', QString::SkipEmptyParts);
            for (const QString &line : lines) {
                const QString trimmed = line.trimmed();
                if (!trimmed.startsWith("ESSID:"))
                    continue;
                const int firstQuote = trimmed.indexOf('"');
                const int lastQuote = trimmed.lastIndexOf('"');
                if (firstQuote >= 0 && lastQuote > firstQuote)
                    appendUnique(trimmed.mid(firstQuote + 1, lastQuote - firstQuote - 1));
            }
        }
    }

    if (ssids.isEmpty() && errorMessage)
        *errorMessage = "未扫描到附近WiFi，请确认天线和驱动正常";
    return ssids;
#endif
}

QString MainWindow::currentWifiSsid() const
{
    QString out;
    QString err;
    if (!runCommand("wpa_cli", QStringList() << "-i" << "wlan0" << "status", 3000, &out, &err))
        return QString();

    const QStringList lines = out.split('\n', QString::SkipEmptyParts);
    for (const QString &line : lines) {
        if (line.startsWith("ssid="))
            return line.mid(5).trimmed();
    }
    return QString();
}

QString MainWindow::currentWifiIpv4() const
{
    const QNetworkInterface iface = QNetworkInterface::interfaceFromName("wlan0");
    if (!iface.isValid())
        return QString();

    const QList<QNetworkAddressEntry> entries = iface.addressEntries();
    for (const QNetworkAddressEntry &entry : entries) {
        const QHostAddress ip = entry.ip();
        if (ip.protocol() == QAbstractSocket::IPv4Protocol &&
                ip != QHostAddress::AnyIPv4 &&
                !ip.isNull()) {
            return ip.toString();
        }
    }
    return QString();
}

bool MainWindow::isWifiPoweredOn() const
{
    const QNetworkInterface iface = QNetworkInterface::interfaceFromName("wlan0");
    if (!iface.isValid())
        return false;
    return iface.flags().testFlag(QNetworkInterface::IsUp);
}

void MainWindow::updateWifiStatus(const QString &message)
{
    if (!wifiStatusLabel)
        return;

    QStringList lines;
    if (!message.trimmed().isEmpty())
        lines << message.trimmed();

    const bool hasInterface = QFile::exists("/sys/class/net/wlan0")
            || QNetworkInterface::interfaceFromName("wlan0").isValid();
    if (!hasInterface) {
        lines << "未检测到wlan0，请确认USB WiFi网卡已插入";
    } else if (isWifiPoweredOn()) {
        const QString ssid = currentWifiSsid();
        const QString ip = currentWifiIpv4();
        if (!ssid.isEmpty())
            lines << QString("当前连接: %1").arg(ssid);
        else
            lines << "WiFi已打开，尚未连接热点";
        if (!ip.isEmpty())
            lines << QString("IP地址: %1").arg(ip);
    } else {
        lines << "WiFi已关闭";
    }

    wifiStatusLabel->setText(lines.join("\n"));
    if (wifiPowerButton)
        wifiPowerButton->setText(isWifiPoweredOn() ? "关闭WiFi" : "打开WiFi");
}

void MainWindow::setPreviewImage(const QImage &image)
{
    if (!previewImageLabel)
        return;

    if (image.isNull()) {
        previewImageLabel->clear();
        return;
    }

    QSize targetSize;
    if (previewDialog) {
        if (previewDialog->layout())
            previewDialog->layout()->activate();
        targetSize = QSize(qMax(200, previewDialog->width() - 40),
                           qMax(160, previewDialog->height() - 150));
    }
    if (!targetSize.isValid() || targetSize.width() < 10 || targetSize.height() < 10) {
        targetSize = QSize(760, 340);
    }

    const QPixmap pix = QPixmap::fromImage(image).scaled(
                targetSize,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation);
    previewImageLabel->setPixmap(pix);
    previewStatusLabel->setGeometry(previewImageLabel->rect());
    previewStatusLabel->raise();
}

QString MainWindow::promptTextWithSoftKeyboard(const QString &title,
                                               const QString &initialText,
                                               bool passwordMode,
                                               bool *accepted)
{
    if (accepted)
        *accepted = false;

    QDialog dialog(this);
    dialog.setWindowTitle(title);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *tipLabel = new QLabel(passwordMode ? "点击下方按键输入WiFi密码"
                                               : "点击下方按键输入WiFi名称", &dialog);
    QLineEdit *edit = new QLineEdit(&dialog);
    edit->setText(initialText);
    if (passwordMode)
        edit->setEchoMode(QLineEdit::Password);

    layout->addWidget(tipLabel);
    layout->addWidget(edit);

    QList<QPushButton *> alphaButtons;
    bool upperCase = false;
    const QStringList symbolKeys = QStringList()
            << "-" << "_" << "." << "@" << "!" << "#" << "$" << "/";

    const auto updateAlphaKeys = [&]() {
        for (QPushButton *button : alphaButtons) {
            const QString lower = button->property("lowerText").toString();
            button->setText(upperCase ? lower.toUpper() : lower);
        }
    };

    const auto appendRow = [&](const QStringList &keys, bool alphabetic) {
        QHBoxLayout *rowLayout = new QHBoxLayout();
        for (const QString &key : keys) {
            QPushButton *button = new QPushButton(alphabetic && upperCase ? key.toUpper() : key, &dialog);
            button->setMinimumHeight(42);
            button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            if (alphabetic) {
                button->setProperty("lowerText", key.toLower());
                alphaButtons.append(button);
            }
            QObject::connect(button, &QPushButton::clicked, &dialog, [edit, button]() {
                edit->insert(button->text());
            });
            rowLayout->addWidget(button);
        }
        layout->addLayout(rowLayout);
    };

    appendRow(QStringList() << "1" << "2" << "3" << "4" << "5"
                            << "6" << "7" << "8" << "9" << "0", false);
    appendRow(QStringList() << "q" << "w" << "e" << "r" << "t"
                            << "y" << "u" << "i" << "o" << "p", true);
    appendRow(QStringList() << "a" << "s" << "d" << "f" << "g"
                            << "h" << "j" << "k" << "l", true);
    appendRow(QStringList() << "z" << "x" << "c" << "v" << "b"
                            << "n" << "m", true);
    appendRow(symbolKeys, false);

    QPushButton *shiftButton = new QPushButton("大小写", &dialog);
    QPushButton *spaceButton = new QPushButton("空格", &dialog);
    QPushButton *backspaceButton = new QPushButton("退格", &dialog);
    QPushButton *clearButton = new QPushButton("清空", &dialog);
    QPushButton *okButton = new QPushButton("确定", &dialog);
    QPushButton *cancelButton = new QPushButton("取消", &dialog);

    shiftButton->setMinimumHeight(46);
    spaceButton->setMinimumHeight(46);
    backspaceButton->setMinimumHeight(46);
    clearButton->setMinimumHeight(46);
    okButton->setMinimumHeight(46);
    cancelButton->setMinimumHeight(46);

    QHBoxLayout *actionLayout = new QHBoxLayout();
    actionLayout->addWidget(shiftButton);
    actionLayout->addWidget(spaceButton);
    actionLayout->addWidget(backspaceButton);
    actionLayout->addWidget(clearButton);
    actionLayout->addWidget(okButton);
    actionLayout->addWidget(cancelButton);
    layout->addLayout(actionLayout);

    QObject::connect(shiftButton, &QPushButton::clicked, &dialog, [&]() {
        upperCase = !upperCase;
        updateAlphaKeys();
    });
    QObject::connect(spaceButton, &QPushButton::clicked, &dialog, [edit]() {
        edit->insert(" ");
    });
    QObject::connect(backspaceButton, &QPushButton::clicked, &dialog, [edit]() {
        edit->backspace();
    });
    QObject::connect(clearButton, &QPushButton::clicked, edit, &QLineEdit::clear);
    QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    updateAlphaKeys();

#if __arm__
    dialog.resize(this->size());
    const int ret = dialog.exec();
#else
    dialog.resize(800, 480);
    const int ret = dialog.exec();
#endif

    if (ret == QDialog::Accepted) {
        if (accepted)
            *accepted = true;
        return edit->text();
    }

    return initialText;
}

void MainWindow::layoutInit()
{
    /* 获取屏幕的分辨率，Qt官方建议使用这
     * 种方法获取屏幕分辨率，防上多屏设备导致对应不上
     * 注意，这是获取整个桌面系统的分辨率
     */
    QList <QScreen *> list_screen =  QGuiApplication::screens();

    /* 如果是ARM平台，直接设置大小为屏幕的大小 */
#if __arm__
    /* 重设大小 */
    this->resize(list_screen.at(0)->geometry().width(),
                 list_screen.at(0)->geometry().height());
#else
    /* 否则则设置主窗体大小为800x480 */
    this->resize(800, 480);
#endif

    /* 实例化与布局，常规操作 */
    mainWidget = new QWidget();
    stackedWidget = new QStackedWidget(mainWidget);
    cameraPage = new QWidget();
    wifiPage = new QWidget();
    photoLabel = new QLabel();
    rightWidget = new QWidget();
    comboBox = new QComboBox();
    styleComboBox = new QComboBox();
    pushButton[0] = new QPushButton();
    pushButton[1] = new QPushButton();
    pushButton[2] = new QPushButton();
    albumButton = new QPushButton();
    wifiButton = new QPushButton();
    scrollArea = new QScrollArea();
    displayLabel = new QLabel(scrollArea);
    beautifyStatusLabel = new QLabel(displayLabel);
    vboxLayout = new QVBoxLayout();
    hboxLayout = new QHBoxLayout();
    QVBoxLayout *rootLayout = new QVBoxLayout();

    vboxLayout->addWidget(photoLabel);
    vboxLayout->addWidget(comboBox);
    vboxLayout->addWidget(pushButton[0]);
    vboxLayout->addWidget(styleComboBox);
    vboxLayout->addWidget(pushButton[2]);
    vboxLayout->addWidget(wifiButton);
    vboxLayout->addWidget(albumButton);
    vboxLayout->addWidget(pushButton[1]);

    rightWidget->setLayout(vboxLayout);

    hboxLayout->addWidget(scrollArea);
    hboxLayout->addWidget(rightWidget);
    cameraPage->setLayout(hboxLayout);
    stackedWidget->addWidget(cameraPage);
    stackedWidget->addWidget(wifiPage);
    rootLayout->addWidget(stackedWidget);
    mainWidget->setLayout(rootLayout);

    this->setCentralWidget(mainWidget);

    pushButton[0]->setMaximumHeight(40);
    pushButton[0]->setMaximumWidth(200);

    pushButton[1]->setMaximumHeight(40);
    pushButton[1]->setMaximumWidth(200);

    pushButton[2]->setMaximumHeight(40);
    pushButton[2]->setMaximumWidth(200);

    albumButton->setMaximumHeight(40);
    albumButton->setMaximumWidth(200);
    wifiButton->setMaximumHeight(40);
    wifiButton->setMaximumWidth(200);

    comboBox->setMaximumHeight(40);
    comboBox->setMaximumWidth(200);
    styleComboBox->setMaximumHeight(40);
    styleComboBox->setMaximumWidth(200);
    photoLabel->setMaximumSize(100, 75);
    scrollArea->setMinimumWidth(this->width()
                                - comboBox->width());

    /* 显示图像最大画面为xx */
    displayLabel->setMinimumWidth(scrollArea->width() * 0.75);
    displayLabel->setMinimumHeight(scrollArea->height() * 0.75);
    scrollArea->setWidget(displayLabel);

    /* 居中显示 */
    scrollArea->setAlignment(Qt::AlignCenter);

    /* 自动拉伸 */
    photoLabel->setScaledContents(true);
    displayLabel->setScaledContents(true);

    beautifyStatusLabel->setAlignment(Qt::AlignCenter);
    beautifyStatusLabel->setText(QString());
    beautifyStatusLabel->setStyleSheet("QLabel{color:white;background-color:rgba(0,0,0,160);font-size:28px;}");
    beautifyStatusLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    beautifyStatusLabel->hide();

    /* 设置一些属性 */
    pushButton[0]->setText("拍照");
    pushButton[0]->setEnabled(false);
    pushButton[2]->setText("美颜");
    pushButton[2]->setEnabled(false);
    wifiButton->setText("WiFi");
    albumButton->setText("相册");
    pushButton[1]->setText("开始");
    pushButton[1]->setCheckable(true);

    styleComboBox->addItem("动漫风格");
    styleComboBox->addItem("机械风格");
    styleComboBox->addItem("写实风格");
    styleComboBox->addItem("赛博风格");

    /* 摄像头 */
    camera = new Camera(this);
    initWifiPage();
    updateWifiStatus();

    /* 信号连接槽 */
    connect(camera, SIGNAL(readyImage(QImage)),
            this, SLOT(showImage(QImage)));
    connect(pushButton[1], SIGNAL(clicked(bool)),
            camera, SLOT(cameraProcess(bool)));
    connect(pushButton[1], SIGNAL(clicked(bool)),
            this, SLOT(setButtonText(bool)));
    connect(pushButton[0], SIGNAL(clicked()),
            this, SLOT(saveImageToLocal()));
    connect(pushButton[2], SIGNAL(clicked()),
            this, SLOT(beautifyCurrentImage()));
    connect(wifiButton, SIGNAL(clicked()),
            this, SLOT(openWifiPage()));
    connect(albumButton, SIGNAL(clicked()),
            this, SLOT(openAlbum()));

}

void MainWindow::scanCameraDevice()
{
    /* 如果是Windows系统，一般是摄像头0 */
#if win32
    comboBox->addItem("windows摄像头0");
    connect(comboBox,
            SIGNAL(currentIndexChanged(int)),
            camera, SLOT(selectCameraDevice(int)));
#else
    /* QFile文件指向/dev/video0 */
    QFile file("/dev/video0");

    /* 如果文件存在 */
    if (file.exists())
        comboBox->addItem("video0");
    else {
        displayLabel->setText("无摄像头设备");
        return;
    }

    file.setFileName("/dev/video1");

    if (file.exists()) {
        comboBox->addItem("video1");
        /* 开发板ov5640等设备是1 */
        comboBox->setCurrentIndex(1);
    }

    file.setFileName("/dev/video2");

    if (file.exists())
        /* 开发板USB摄像头设备是2 */
        comboBox->addItem("video2");

#if !__arm__
    /* ubuntu的USB摄像头一般是0 */
    comboBox->setCurrentIndex(0);
#endif

    connect(comboBox,
            SIGNAL(currentIndexChanged(int)),
            camera, SLOT(selectCameraDevice(int)));
#endif
}

void MainWindow::openWifiPage()
{
    if (stackedWidget && wifiPage)
        stackedWidget->setCurrentWidget(wifiPage);

    QString savedSsid;
    QString savedPsk;
    loadWifiCredentials(&savedSsid, &savedPsk);
    if (wifiSsidEdit && wifiSsidEdit->text().trimmed().isEmpty())
        wifiSsidEdit->setText(savedSsid);
    if (wifiPasswordEdit && wifiPasswordEdit->text().isEmpty())
        wifiPasswordEdit->setText(savedPsk);

    updateWifiStatus();
}

void MainWindow::closeWifiPage()
{
    if (stackedWidget && cameraPage)
        stackedWidget->setCurrentWidget(cameraPage);
}

void MainWindow::toggleWifiPower()
{
#if win32
    updateWifiStatus("Windows下未实现开发板WiFi控制");
    return;
#else
    if (!isWifiPoweredOn()) {
        QString errorMessage;
        if (ensureWifiControlReady(&errorMessage))
            updateWifiStatus("WiFi已打开");
        else
            updateWifiStatus(errorMessage);
        return;
    }

    runCommandAndLog("killall", QStringList() << "-q" << "wpa_supplicant", 3000);
    runCommandAndLog("ifconfig", QStringList() << "wlan0" << "down", 3000);
    updateWifiStatus("WiFi已关闭");
#endif
}

void MainWindow::refreshWifiList()
{
    if (!wifiListWidget)
        return;

    wifiListWidget->clear();
    updateWifiStatus("正在扫描附近WiFi，请稍候...");

    QString errorMessage;
    const QStringList ssids = scanNearbyWifi(&errorMessage);
    for (const QString &ssid : ssids)
        wifiListWidget->addItem(ssid);

    if (!ssids.isEmpty()) {
        wifiListWidget->setCurrentRow(0);
        if (wifiSsidEdit && wifiSsidEdit->text().trimmed().isEmpty())
            wifiSsidEdit->setText(ssids.first());
        updateWifiStatus(QString("扫描完成，发现%1个WiFi").arg(ssids.size()));
    } else {
        updateWifiStatus(errorMessage.isEmpty() ? QString("未扫描到附近WiFi") : errorMessage);
    }
}

void MainWindow::onWifiItemActivated(QListWidgetItem *item)
{
    if (!item || !wifiSsidEdit)
        return;
    wifiSsidEdit->setText(item->text());
}

void MainWindow::connectToWifi()
{
    if (!wifiSsidEdit || !wifiPasswordEdit)
        return;

    const QString ssid = wifiSsidEdit->text().trimmed();
    const QString psk = wifiPasswordEdit->text();
    if (ssid.isEmpty()) {
        updateWifiStatus("请先选择或输入WiFi名称");
        return;
    }

    updateWifiStatus(QString("正在连接 %1，请稍候...").arg(ssid));
    QString errorMessage;
    if (ensureWifiConnected(ssid, psk, &errorMessage))
        updateWifiStatus(QString("已连接到 %1").arg(ssid));
    else
        updateWifiStatus(errorMessage.isEmpty() ? QString("连接失败") : errorMessage);
}

void MainWindow::showImage(const QImage &image)
{
    /* 显示图像 */
    displayLabel->setPixmap(QPixmap::fromImage(image));
    saveImage = image;
    if (beautifyStatusLabel) {
        beautifyStatusLabel->setGeometry(displayLabel->rect());
        beautifyStatusLabel->raise();
    }

    /* 判断图像是否为空，空则设置拍照按钮不可用 */
    if (!saveImage.isNull()) {
        pushButton[0]->setEnabled(true);
        if (!beautifyApiReply && !beautifyDownloadReply && !apiSocket && !downloadSocket)
            pushButton[2]->setEnabled(!capturedPhotoImage.isNull());
    } else {
        pushButton[0]->setEnabled(false);
        pushButton[2]->setEnabled(!capturedPhotoImage.isNull());
    }
}

void MainWindow::setButtonText(bool bl)
{
    if (bl) {
        /* 设置摄像头设备 */
        camera->selectCameraDevice(comboBox->currentIndex());
        pushButton[1]->setText("关闭");
    } else {
        /* 若关闭了摄像头则禁用拍照按钮 */
        pushButton[0]->setEnabled(false);
        pushButton[1]->setText("开始");
    }
}

QString MainWindow::detectTfMountPoint() const
{
    const QStringList candidateMountPoints = {
        "/run/media/mmcblk0p1",
        "/media/mmcblk0p1"
    };

    for (const QString &candidate : candidateMountPoints) {
        if (QDir(candidate).exists())
            return candidate;
    }

    QFile mountsFile("/proc/mounts");
    if (!mountsFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();

    QTextStream in(&mountsFile);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (!line.startsWith("/dev/mmcblk0p1 "))
            continue;
        const QStringList parts = line.split(' ', QString::SkipEmptyParts);
        if (parts.size() >= 2)
            return parts.at(1);
    }

    return QString();
}

QString MainWindow::ensureAlbumDir(const QString &baseDir) const
{
    const QString saveDirPath = baseDir + "/opencv_camera_photos";
    QDir().mkpath(saveDirPath);
    return saveDirPath;
}

QString MainWindow::saveImageToAlbum(const QImage &image, const QString &prefix) const
{
    QString baseDir = detectTfMountPoint();
    if (baseDir.isEmpty())
        baseDir = QCoreApplication::applicationDirPath();

    const QString saveDirPath = ensureAlbumDir(baseDir);
    const QString fileName = saveDirPath + "/"
            + prefix + "_"
            + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz")
            + ".png";

    if (!image.save(fileName, "PNG", -1))
        return QString();

    return fileName;
}

QString MainWindow::albumDirPath() const
{
    QString baseDir = detectTfMountPoint();
    if (baseDir.isEmpty())
        baseDir = QCoreApplication::applicationDirPath();
    return ensureAlbumDir(baseDir);
}

QString MainWindow::promptForStyle(const QString &style) const
{
    if (style == "机械风格")
        return "图片变为机械风格，金属质感，工业设计细节丰富，高清精致，保留主体结构与细节";
    if (style == "写实风格")
        return "图片变为写实风格，真实光影与材质，高分辨率细节，色彩自然，保留主体结构与细节";
    if (style == "赛博风格")
        return "图片变为赛博朋克风格，霓虹灯光，未来都市氛围，高对比，高清精致，保留主体结构与细节";
    return "图片变为动漫风格，高清精致，保留五官细节";
}

QString MainWindow::prefixForStyle(const QString &style) const
{
    if (style == "机械风格")
        return "mecha";
    if (style == "写实风格")
        return "real";
    if (style == "赛博风格")
        return "cyber";
    return "anime";
}

void MainWindow::saveImageToLocal()
{
    /* 判断图像是否为空 */
    if (!saveImage.isNull()) {
        capturedPhotoImage = saveImage;
        const QString fileName = saveImageToAlbum(saveImage, "photo");
        if (fileName.isEmpty()) {
            qDebug()<<"保存失败！"<<endl;
            return;
        }
        qDebug()<<"正在保存"<<fileName<<"图片,请稍候..."<<endl;

        /* 设置拍照的图像为显示在photoLabel上 */
        photoLabel->setPixmap(QPixmap::fromImage(QImage(fileName)));
        pushButton[2]->setEnabled(true);

        qDebug()<<"保存完成！"<<endl;
    }
}

void MainWindow::openAlbum()
{
    if (!albumDialog) {
        albumDialog = new QDialog(this);
        albumDialog->setWindowTitle("相册");

        albumListWidget = new QListWidget(albumDialog);
        albumListWidget->setViewMode(QListView::IconMode);
        albumListWidget->setResizeMode(QListView::Adjust);
        albumListWidget->setMovement(QListView::Static);
        albumListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        albumListWidget->setIconSize(QSize(120, 90));
        albumListWidget->setSpacing(8);
        QPushButton *albumBackButton = new QPushButton("返回", albumDialog);

        QVBoxLayout *layout = new QVBoxLayout(albumDialog);
        layout->addWidget(albumListWidget, 1);
        layout->addWidget(albumBackButton);
        albumDialog->setLayout(layout);

        connect(albumListWidget, SIGNAL(itemClicked(QListWidgetItem*)),
                this, SLOT(onAlbumItemActivated(QListWidgetItem*)));
        connect(albumListWidget, SIGNAL(itemActivated(QListWidgetItem*)),
                this, SLOT(onAlbumItemActivated(QListWidgetItem*)));
        connect(albumBackButton, SIGNAL(clicked()),
                albumDialog, SLOT(close()));
    }

    refreshAlbum();
#if __arm__
    albumDialog->showFullScreen();
#else
    albumDialog->resize(800, 480);
    albumDialog->show();
#endif
    albumDialog->raise();
    albumDialog->activateWindow();
}

void MainWindow::refreshAlbum()
{
    if (!albumListWidget)
        return;

    albumListWidget->clear();
    const QString dirPath = albumDirPath();
    QDir dir(dirPath);
    if (!dir.exists())
        return;

    const QStringList nameFilters = QStringList() << "*.png" << "*.jpg" << "*.jpeg";
    const QFileInfoList files = dir.entryInfoList(nameFilters, QDir::Files | QDir::NoDotAndDotDot, QDir::Time | QDir::Reversed);
    for (const QFileInfo &fi : files) {
        QPixmap pix(fi.absoluteFilePath());
        if (!pix.isNull())
            pix = pix.scaled(albumListWidget->iconSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QListWidgetItem *item = new QListWidgetItem(QIcon(pix), fi.fileName());
        item->setData(Qt::UserRole, fi.absoluteFilePath());
        item->setTextAlignment(Qt::AlignHCenter);
        albumListWidget->addItem(item);
    }
}

void MainWindow::onAlbumItemActivated(QListWidgetItem *item)
{
    if (!item)
        return;
    const QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty())
        return;

    if (!previewDialog) {
        previewDialog = new QDialog(this);
        previewDialog->setWindowTitle("预览");

        previewImageLabel = new QLabel(previewDialog);
        previewImageLabel->setAlignment(Qt::AlignCenter);
        previewImageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        previewImageLabel->setMinimumHeight(240);
        previewImageLabel->setStyleSheet("QLabel{background:#111111;border:1px solid #444444;}");

        previewStatusLabel = new QLabel(previewImageLabel);
        previewStatusLabel->setAlignment(Qt::AlignCenter);
        previewStatusLabel->setText(QString());
        previewStatusLabel->setStyleSheet("QLabel{color:white;background-color:rgba(0,0,0,160);font-size:28px;}");
        previewStatusLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        previewStatusLabel->hide();

        previewStyleComboBox = new QComboBox(previewDialog);
        previewStyleComboBox->addItem("动漫风格");
        previewStyleComboBox->addItem("机械风格");
        previewStyleComboBox->addItem("写实风格");
        previewStyleComboBox->addItem("赛博风格");

        previewBeautifyButton = new QPushButton("美颜", previewDialog);
        QPushButton *previewBackButton = new QPushButton("返回", previewDialog);
        connect(previewBeautifyButton, SIGNAL(clicked()),
                this, SLOT(onPreviewBeautifyClicked()));
        connect(previewBackButton, SIGNAL(clicked()),
                previewDialog, SLOT(close()));

        QVBoxLayout *layout = new QVBoxLayout(previewDialog);
        layout->addWidget(previewImageLabel, 1);
        layout->addWidget(previewStyleComboBox);
        layout->addWidget(previewBeautifyButton);
        layout->addWidget(previewBackButton);
        previewDialog->setLayout(layout);
    }

#if __arm__
    previewDialog->showFullScreen();
#else
    previewDialog->resize(800, 480);
    previewDialog->show();
#endif
    previewImagePath = path;
    const QImage img(path);
    setPreviewImage(img);
    previewDialog->raise();
    previewDialog->activateWindow();
}

void MainWindow::onPreviewBeautifyClicked()
{
    if (previewImagePath.isEmpty())
        return;
    if (beautifyApiReply || beautifyDownloadReply)
        return;

    const QImage img(previewImagePath);
    if (img.isNull())
        return;

    const QString style = previewStyleComboBox ? previewStyleComboBox->currentText() : QString("动漫风格");
    beautifyRequestImage = img;
    beautifyPendingStyle = style;
    beautifyResultLabel = previewImageLabel;
    beautifyResultStatusLabel = previewStatusLabel;
    beautifyResultButton = previewBeautifyButton;
    beautifyResultPrefix = "beauty_" + prefixForStyle(style) + "_";

    beautifyCurrentImage();
}

void MainWindow::beautifyCurrentImage()
{
    if (capturedPhotoImage.isNull() && beautifyRequestImage.isNull())
        return;
    if (beautifyApiReply || beautifyDownloadReply)
        return;

    QString wifiSsid = QString::fromLocal8Bit(qgetenv("WIFI_SSID")).trimmed();
    QString wifiPsk = QString::fromLocal8Bit(qgetenv("WIFI_PSK"));
    if (wifiSsid.isEmpty())
        loadWifiCredentials(&wifiSsid, &wifiPsk);

    ensureSystemTimeReasonable();
    if (!wifiSsid.isEmpty()) {
        QString wifiError;
        ensureWifiConnected(wifiSsid, wifiPsk, &wifiError);
        if (!wifiError.isEmpty())
            qDebug()<<"WiFi连接提示"<<wifiError<<endl;
    }
    preferWlan0AsDefaultRoute();
    if (!hasWlan0Ipv4()) {
        qDebug()<<"WiFi未连接或未获取到IP，尝试继续请求云端"<<endl;
    }

    QByteArray apiKey = qgetenv("DOUBAO_API_KEY");
    if (apiKey.isEmpty()) {
        const QString mountPoint = detectTfMountPoint();
        if (!mountPoint.isEmpty())
            apiKey = readFirstLine(mountPoint + "/doubao_api_key.txt");
    }
    if (apiKey.isEmpty())
        apiKey = readFirstLine("/etc/doubao_api_key.txt");
    if (apiKey.isEmpty())
        apiKey = readFirstLine("/etc/doubao_api_key");
    if (apiKey.isEmpty()) {
        qDebug()<<"未配置豆包API Key"<<endl;
        return;
    }

    const QString style = !beautifyPendingStyle.isEmpty()
            ? beautifyPendingStyle
            : (styleComboBox ? styleComboBox->currentText() : QString("动漫风格"));
    beautifyPendingStyle.clear();

    if (!beautifyResultLabel)
        beautifyResultLabel = photoLabel;
    if (!beautifyResultStatusLabel)
        beautifyResultStatusLabel = beautifyStatusLabel;
    if (!beautifyResultButton)
        beautifyResultButton = pushButton[2];
    if (beautifyResultPrefix.isEmpty())
        beautifyResultPrefix = "beauty_" + prefixForStyle(style) + "_";

    if (beautifyResultStatusLabel) {
        QWidget *parent = beautifyResultStatusLabel->parentWidget();
        if (parent)
            beautifyResultStatusLabel->setGeometry(parent->rect());
        beautifyResultStatusLabel->setText("正在美颜中，请稍候...");
        beautifyResultStatusLabel->show();
        beautifyResultStatusLabel->raise();
    }
    qDebug()<<"正在美颜中，请稍候..."<<endl;

    const QImage sourceImage = beautifyRequestImage.isNull() ? capturedPhotoImage : beautifyRequestImage;
    beautifyRequestImage = QImage();

    QByteArray jpegBytes;
    QBuffer buffer(&jpegBytes);
    if (!buffer.open(QIODevice::WriteOnly)) {
        qDebug()<<"图片编码失败"<<endl;
        if (beautifyResultStatusLabel)
            beautifyResultStatusLabel->hide();
        return;
    }
    if (!sourceImage.save(&buffer, "JPG", 95)) {
        qDebug()<<"图片编码失败"<<endl;
        if (beautifyResultStatusLabel)
            beautifyResultStatusLabel->hide();
        return;
    }

    const QString imageDataUrl = "data:image/jpeg;base64," + QString::fromLatin1(jpegBytes.toBase64());

    QJsonObject payload;
    payload.insert("model", "doubao-seedream-5-0-260128");
    payload.insert("prompt", promptForStyle(style));
    payload.insert("image", imageDataUrl);
    payload.insert("size", "2K");
    payload.insert("output_format", "png");
    payload.insert("watermark", false);

    if (beautifyResultButton) {
        beautifyResultButton->setEnabled(false);
        beautifyResultButton->setText("美颜中...");
    }

    apiRequestBytes = QJsonDocument(payload).toJson(QJsonDocument::Compact);

    startApiRequestOverSsl(apiKey, apiRequestBytes);
}

void MainWindow::onBeautifyApiFinished()
{
    if (!beautifyApiReply)
        return;

    const QNetworkReply::NetworkError err = beautifyApiReply->error();
    const QString errStr = beautifyApiReply->errorString();
    const int httpStatus = beautifyApiReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = beautifyApiReply->readAll();
    beautifyApiReply->deleteLater();
    beautifyApiReply = nullptr;

    if (err != QNetworkReply::NoError) {
        qDebug()<<"美颜请求失败"
               <<"err="<<err
               <<"http="<<httpStatus
               <<"msg="<<errStr
               <<"body="<<QString::fromUtf8(body.left(512))
               <<endl;
        if (beautifyResultButton) {
            beautifyResultButton->setText("美颜");
            beautifyResultButton->setEnabled(beautifyResultButton == pushButton[2] ? !capturedPhotoImage.isNull() : true);
        }
        if (beautifyResultStatusLabel)
            beautifyResultStatusLabel->hide();
        beautifyResultPrefix.clear();
        beautifyResultLabel = nullptr;
        beautifyResultStatusLabel = nullptr;
        beautifyResultButton = nullptr;
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qDebug()<<"美颜返回解析失败"<<endl;
        if (beautifyResultButton) {
            beautifyResultButton->setText("美颜");
            beautifyResultButton->setEnabled(beautifyResultButton == pushButton[2] ? !capturedPhotoImage.isNull() : true);
        }
        if (beautifyResultStatusLabel)
            beautifyResultStatusLabel->hide();
        beautifyResultPrefix.clear();
        beautifyResultLabel = nullptr;
        beautifyResultStatusLabel = nullptr;
        beautifyResultButton = nullptr;
        return;
    }

    const QJsonObject obj = doc.object();
    const QJsonArray data = obj.value("data").toArray();
    if (data.isEmpty() || !data.at(0).isObject()) {
        qDebug()<<"美颜返回缺少图片URL"<<endl;
        if (beautifyResultButton) {
            beautifyResultButton->setText("美颜");
            beautifyResultButton->setEnabled(beautifyResultButton == pushButton[2] ? !capturedPhotoImage.isNull() : true);
        }
        if (beautifyResultStatusLabel)
            beautifyResultStatusLabel->hide();
        beautifyResultPrefix.clear();
        beautifyResultLabel = nullptr;
        beautifyResultStatusLabel = nullptr;
        beautifyResultButton = nullptr;
        return;
    }

    const QString url = data.at(0).toObject().value("url").toString();
    if (url.isEmpty()) {
        qDebug()<<"美颜返回缺少图片URL"<<endl;
        if (beautifyResultButton) {
            beautifyResultButton->setText("美颜");
            beautifyResultButton->setEnabled(beautifyResultButton == pushButton[2] ? !capturedPhotoImage.isNull() : true);
        }
        if (beautifyResultStatusLabel)
            beautifyResultStatusLabel->hide();
        beautifyResultPrefix.clear();
        beautifyResultLabel = nullptr;
        beautifyResultStatusLabel = nullptr;
        beautifyResultButton = nullptr;
        return;
    }

    QNetworkRequest request = QNetworkRequest(QUrl(url));
    beautifyDownloadReply = networkManager->get(request);
    connect(beautifyDownloadReply, SIGNAL(finished()), this, SLOT(onBeautifyDownloadFinished()));
    connect(beautifyDownloadReply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onBeautifyDownloadSslErrors(QList<QSslError>)));
}

void MainWindow::onBeautifyDownloadFinished()
{
    if (!beautifyDownloadReply)
        return;

    const QNetworkReply::NetworkError err = beautifyDownloadReply->error();
    const QString errStr = beautifyDownloadReply->errorString();
    const int httpStatus = beautifyDownloadReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray bytes = beautifyDownloadReply->readAll();
    beautifyDownloadReply->deleteLater();
    beautifyDownloadReply = nullptr;

    if (err != QNetworkReply::NoError) {
        qDebug()<<"下载美颜图片失败"
               <<"err="<<err
               <<"http="<<httpStatus
               <<"msg="<<errStr
               <<endl;
        if (beautifyResultButton) {
            beautifyResultButton->setText("美颜");
            beautifyResultButton->setEnabled(beautifyResultButton == pushButton[2] ? !capturedPhotoImage.isNull() : true);
        }
        if (beautifyResultStatusLabel)
            beautifyResultStatusLabel->hide();
        beautifyResultPrefix.clear();
        beautifyResultLabel = nullptr;
        beautifyResultStatusLabel = nullptr;
        beautifyResultButton = nullptr;
        return;
    }

    const QImage image = QImage::fromData(bytes);
    if (image.isNull()) {
        qDebug()<<"美颜图片解码失败"<<endl;
        if (beautifyResultButton) {
            beautifyResultButton->setText("美颜");
            beautifyResultButton->setEnabled(beautifyResultButton == pushButton[2] ? !capturedPhotoImage.isNull() : true);
        }
        if (beautifyResultStatusLabel)
            beautifyResultStatusLabel->hide();
        beautifyResultPrefix.clear();
        beautifyResultLabel = nullptr;
        beautifyResultStatusLabel = nullptr;
        beautifyResultButton = nullptr;
        return;
    }

    beautyImage = image;
    if (beautifyResultLabel == previewImageLabel)
        setPreviewImage(beautyImage);
    else if (beautifyResultLabel)
        beautifyResultLabel->setPixmap(QPixmap::fromImage(beautyImage));
    else
        photoLabel->setPixmap(QPixmap::fromImage(beautyImage));

    const QString saveDirPath = albumDirPath();
    const QString prefix = beautifyResultPrefix.isEmpty() ? QString("beauty") : beautifyResultPrefix;
    const QString fileName = saveDirPath + "/"
            + prefix
            + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz")
            + ".png";
    qDebug()<<"正在保存"<<fileName<<"图片,请稍候..."<<endl;
    if (!beautyImage.save(fileName, "PNG", -1))
        qDebug()<<"保存美颜图片失败！"<<endl;
    else
        qDebug()<<"保存完成！"<<endl;

    if (beautifyResultLabel == previewImageLabel)
        previewImagePath = fileName;
    if (albumListWidget)
        refreshAlbum();

    if (beautifyResultButton) {
        beautifyResultButton->setText("美颜");
        beautifyResultButton->setEnabled(beautifyResultButton == pushButton[2] ? !capturedPhotoImage.isNull() : true);
    }
    if (beautifyResultStatusLabel)
        beautifyResultStatusLabel->hide();
    beautifyResultPrefix.clear();
    beautifyResultLabel = nullptr;
    beautifyResultStatusLabel = nullptr;
    beautifyResultButton = nullptr;
}

void MainWindow::onBeautifyApiSslErrors(const QList<QSslError> &errors)
{
    qDebug()<<"美颜API SSL错误"<<errors.size()<<endl;
    for (const QSslError &e : errors)
        qDebug()<<"ssl="<<e.errorString()<<endl;

    if (beautifyApiReply && qgetenv("ALLOW_INSECURE_SSL") == "1")
        beautifyApiReply->ignoreSslErrors();
}

void MainWindow::onBeautifyDownloadSslErrors(const QList<QSslError> &errors)
{
    qDebug()<<"美颜下载 SSL错误"<<errors.size()<<endl;
    for (const QSslError &e : errors)
        qDebug()<<"ssl="<<e.errorString()<<endl;

    if (beautifyDownloadReply && qgetenv("ALLOW_INSECURE_SSL") == "1")
        beautifyDownloadReply->ignoreSslErrors();
}

void MainWindow::startApiRequestOverSsl(const QByteArray &apiKey, const QByteArray &payloadBytes)
{
    if (apiSocket) {
        apiSocket->disconnect(this);
        apiSocket->deleteLater();
        apiSocket = nullptr;
    }

    apiResponseBytes.clear();

    const QString host = "ark.cn-beijing.volces.com";
    const QString path = "/api/v3/images/generations";
    const QByteArray body = payloadBytes;

    QByteArray request;
    request += "POST " + path.toLatin1() + " HTTP/1.1\r\n";
    request += "Host: " + host.toLatin1() + "\r\n";
    request += "Authorization: Bearer " + apiKey + "\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";
    request += body;

    apiRequestBytes = request;

    apiSocket = new QSslSocket(this);
    connect(apiSocket, SIGNAL(encrypted()), this, SLOT(onApiSocketEncrypted()));
    connect(apiSocket, SIGNAL(readyRead()), this, SLOT(onApiSocketReadyRead()));
    connect(apiSocket, SIGNAL(disconnected()), this, SLOT(onApiSocketDisconnected()));
    connect(apiSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onApiSocketError(QAbstractSocket::SocketError)));
    connect(apiSocket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onApiSocketSslErrors(QList<QSslError>)));

    QSslConfiguration conf = apiSocket->sslConfiguration();
    conf.setProtocol(QSsl::TlsV1_2OrLater);
    conf.setPeerVerifyMode(qgetenv("ALLOW_INSECURE_SSL") == "1" ? QSslSocket::VerifyNone : QSslSocket::VerifyPeer);
    conf.setCaCertificates(QSslConfiguration::systemCaCertificates());
    apiSocket->setSslConfiguration(conf);
    apiSocket->setPeerVerifyName(host);
    if (qgetenv("ALLOW_INSECURE_SSL") == "1")
        apiSocket->ignoreSslErrors();

    const QNetworkInterface wlan0 = QNetworkInterface::interfaceFromName("wlan0");
    const QList<QNetworkAddressEntry> entries = wlan0.addressEntries();
    for (const QNetworkAddressEntry &entry : entries) {
        const QHostAddress ip = entry.ip();
        if (ip.protocol() == QAbstractSocket::IPv4Protocol && ip != QHostAddress::AnyIPv4 && !ip.isNull()) {
            if (!apiSocket->bind(ip))
                qDebug()<<"绑定wlan0地址失败"<<ip.toString()<<apiSocket->errorString()<<endl;
            else
                qDebug()<<"绑定wlan0地址"<<ip.toString()<<endl;
            break;
        }
    }

    apiSocket->connectToHostEncrypted(host, 443, QIODevice::ReadWrite, QAbstractSocket::IPv4Protocol);
}

void MainWindow::startDownloadRequestOverSsl(const QString &url)
{
    pendingDownloadUrl = url;

    if (downloadSocket) {
        downloadSocket->disconnect(this);
        downloadSocket->deleteLater();
        downloadSocket = nullptr;
    }

    downloadResponseBytes.clear();

    const QUrl qurl(url);
    const QString host = qurl.host();
    const int port = qurl.port(443);
    const QString path = qurl.path().isEmpty() ? "/" : qurl.path();
    const QString fullPath = qurl.hasQuery() ? (path + "?" + qurl.query()) : path;

    QByteArray request;
    request += "GET " + fullPath.toLatin1() + " HTTP/1.1\r\n";
    request += "Host: " + host.toLatin1() + "\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";

    downloadRequestBytes = request;

    downloadSocket = new QSslSocket(this);
    connect(downloadSocket, SIGNAL(encrypted()), this, SLOT(onDownloadSocketEncrypted()));
    connect(downloadSocket, SIGNAL(readyRead()), this, SLOT(onDownloadSocketReadyRead()));
    connect(downloadSocket, SIGNAL(disconnected()), this, SLOT(onDownloadSocketDisconnected()));
    connect(downloadSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onDownloadSocketError(QAbstractSocket::SocketError)));
    connect(downloadSocket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onDownloadSocketSslErrors(QList<QSslError>)));

    QSslConfiguration conf = downloadSocket->sslConfiguration();
    conf.setProtocol(QSsl::TlsV1_2OrLater);
    conf.setPeerVerifyMode(qgetenv("ALLOW_INSECURE_SSL") == "1" ? QSslSocket::VerifyNone : QSslSocket::VerifyPeer);
    conf.setCaCertificates(QSslConfiguration::systemCaCertificates());
    downloadSocket->setSslConfiguration(conf);
    downloadSocket->setPeerVerifyName(host);
    if (qgetenv("ALLOW_INSECURE_SSL") == "1")
        downloadSocket->ignoreSslErrors();

    const QNetworkInterface wlan0 = QNetworkInterface::interfaceFromName("wlan0");
    const QList<QNetworkAddressEntry> entries = wlan0.addressEntries();
    for (const QNetworkAddressEntry &entry : entries) {
        const QHostAddress ip = entry.ip();
        if (ip.protocol() == QAbstractSocket::IPv4Protocol && ip != QHostAddress::AnyIPv4 && !ip.isNull()) {
            if (!downloadSocket->bind(ip))
                qDebug()<<"绑定wlan0地址失败"<<ip.toString()<<downloadSocket->errorString()<<endl;
            else
                qDebug()<<"绑定wlan0地址"<<ip.toString()<<endl;
            break;
        }
    }

    downloadSocket->connectToHostEncrypted(host, port, QIODevice::ReadWrite, QAbstractSocket::IPv4Protocol);
}

QByteArray MainWindow::decodeChunkedBody(const QByteArray &body)
{
    QByteArray out;
    int pos = 0;
    while (pos < body.size()) {
        const int lineEnd = body.indexOf("\r\n", pos);
        if (lineEnd < 0)
            break;
        const QByteArray sizeLine = body.mid(pos, lineEnd - pos).trimmed();
        bool ok = false;
        const int chunkSize = sizeLine.toInt(&ok, 16);
        if (!ok)
            break;
        pos = lineEnd + 2;
        if (chunkSize == 0)
            break;
        if (pos + chunkSize > body.size())
            break;
        out += body.mid(pos, chunkSize);
        pos += chunkSize;
        if (body.mid(pos, 2) == "\r\n")
            pos += 2;
    }
    return out;
}

int MainWindow::extractContentLengthFromHttpHeaders(const QByteArray &response)
{
    int headerEnd = response.indexOf("\r\n\r\n");
    if (headerEnd < 0)
        headerEnd = response.indexOf("\n\n");
    if (headerEnd < 0)
        return -1;

    const QByteArray header = response.left(headerEnd);
    const QList<QByteArray> lines = header.split('\n');
    for (const QByteArray &rawLine : lines) {
        const QByteArray line = rawLine.trimmed();
        if (!line.toLower().startsWith("content-length:"))
            continue;
        bool ok = false;
        const int value = line.mid(QByteArray("content-length:").size()).trimmed().toInt(&ok);
        if (ok)
            return value;
    }
    return -1;
}

bool MainWindow::isHttpResponseComplete(const QByteArray &response)
{
    int headerEnd = response.indexOf("\r\n\r\n");
    int sepLen = 4;
    if (headerEnd < 0) {
        headerEnd = response.indexOf("\n\n");
        sepLen = 2;
    }
    if (headerEnd < 0)
        return false;

    const QByteArray header = response.left(headerEnd).toLower();
    const QByteArray body = response.mid(headerEnd + sepLen);
    if (header.contains("transfer-encoding: chunked"))
        return body.contains("\r\n0\r\n\r\n") || body.endsWith("\r\n0\r\n\r\n");

    const int contentLength = extractContentLengthFromHttpHeaders(response);
    if (contentLength >= 0)
        return body.size() >= contentLength;

    return false;
}

QByteArray MainWindow::extractHttpBody(const QByteArray &response, int *statusCode, bool *chunked)
{
    if (statusCode)
        *statusCode = 0;
    if (chunked)
        *chunked = false;

    int headerEnd = response.indexOf("\r\n\r\n");
    int sepLen = 4;
    if (headerEnd < 0) {
        headerEnd = response.indexOf("\n\n");
        sepLen = 2;
    }
    if (headerEnd < 0)
        return QByteArray();

    const QByteArray header = response.left(headerEnd);
    const QByteArray body = response.mid(headerEnd + sepLen);

    const int firstLineEnd = header.indexOf("\r\n") >= 0 ? header.indexOf("\r\n") : header.indexOf("\n");
    const QByteArray statusLine = firstLineEnd >= 0 ? header.left(firstLineEnd) : header;
    const QList<QByteArray> statusParts = statusLine.split(' ');
    if (statusCode && statusParts.size() >= 2)
        *statusCode = statusParts.at(1).toInt();

    if (chunked) {
        const QByteArray lowerHeader = header.toLower();
        *chunked = lowerHeader.contains("transfer-encoding: chunked");
    }

    return body;
}

void MainWindow::onApiSocketEncrypted()
{
    if (!apiSocket)
        return;
    apiSocket->write(apiRequestBytes);
    apiSocket->flush();
}

void MainWindow::onApiSocketReadyRead()
{
    if (!apiSocket)
        return;
    apiResponseBytes += apiSocket->readAll();
    if (isHttpResponseComplete(apiResponseBytes)) {
        qDebug()<<"API响应已接收完整，主动关闭连接"<<endl;
        apiSocket->disconnectFromHost();
    }
}

void MainWindow::onApiSocketDisconnected()
{
    if (!apiSocket)
        return;

    int status = 0;
    bool chunked = false;
    QByteArray body = extractHttpBody(apiResponseBytes, &status, &chunked);
    if (chunked)
        body = decodeChunkedBody(body);

    apiSocket->deleteLater();
    apiSocket = nullptr;

    if (status != 200) {
        qDebug()<<"美颜请求失败"
               <<"http="<<status
               <<"body="<<QString::fromUtf8(body.left(512))
               <<endl;
        if (beautifyResultButton) {
            beautifyResultButton->setText("美颜");
            beautifyResultButton->setEnabled(beautifyResultButton == pushButton[2] ? !capturedPhotoImage.isNull() : true);
        }
        if (beautifyResultStatusLabel)
            beautifyResultStatusLabel->hide();
        beautifyResultPrefix.clear();
        beautifyResultLabel = nullptr;
        beautifyResultStatusLabel = nullptr;
        beautifyResultButton = nullptr;
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qDebug()<<"美颜返回解析失败"<<endl;
        if (beautifyResultButton) {
            beautifyResultButton->setText("美颜");
            beautifyResultButton->setEnabled(beautifyResultButton == pushButton[2] ? !capturedPhotoImage.isNull() : true);
        }
        if (beautifyResultStatusLabel)
            beautifyResultStatusLabel->hide();
        beautifyResultPrefix.clear();
        beautifyResultLabel = nullptr;
        beautifyResultStatusLabel = nullptr;
        beautifyResultButton = nullptr;
        return;
    }

    const QJsonArray data = doc.object().value("data").toArray();
    if (data.isEmpty() || !data.at(0).isObject()) {
        qDebug()<<"美颜返回缺少图片URL"<<endl;
        if (beautifyResultButton) {
            beautifyResultButton->setText("美颜");
            beautifyResultButton->setEnabled(beautifyResultButton == pushButton[2] ? !capturedPhotoImage.isNull() : true);
        }
        if (beautifyResultStatusLabel)
            beautifyResultStatusLabel->hide();
        beautifyResultPrefix.clear();
        beautifyResultLabel = nullptr;
        beautifyResultStatusLabel = nullptr;
        beautifyResultButton = nullptr;
        return;
    }

    const QString url = data.at(0).toObject().value("url").toString();
    if (url.isEmpty()) {
        qDebug()<<"美颜返回缺少图片URL"<<endl;
        if (beautifyResultButton) {
            beautifyResultButton->setText("美颜");
            beautifyResultButton->setEnabled(beautifyResultButton == pushButton[2] ? !capturedPhotoImage.isNull() : true);
        }
        if (beautifyResultStatusLabel)
            beautifyResultStatusLabel->hide();
        beautifyResultPrefix.clear();
        beautifyResultLabel = nullptr;
        beautifyResultStatusLabel = nullptr;
        beautifyResultButton = nullptr;
        return;
    }

    startDownloadRequestOverSsl(url);
}

void MainWindow::onApiSocketError(QAbstractSocket::SocketError)
{
    if (!apiSocket)
        return;
    if (apiSocket->error() == QAbstractSocket::RemoteHostClosedError) {
        qDebug()<<"美颜请求连接由服务器关闭，等待读取剩余数据"<<endl;
        return;
    }
    qDebug()<<"美颜请求失败"
           <<"err="<<apiSocket->error()
           <<"msg="<<apiSocket->errorString()
           <<endl;
    apiSocket->deleteLater();
    apiSocket = nullptr;
    if (beautifyResultButton) {
        beautifyResultButton->setText("美颜");
        beautifyResultButton->setEnabled(beautifyResultButton == pushButton[2] ? !capturedPhotoImage.isNull() : true);
    }
    if (beautifyResultStatusLabel)
        beautifyResultStatusLabel->hide();
    beautifyResultPrefix.clear();
    beautifyResultLabel = nullptr;
    beautifyResultStatusLabel = nullptr;
    beautifyResultButton = nullptr;
}

void MainWindow::onApiSocketSslErrors(const QList<QSslError> &errors)
{
    qDebug()<<"美颜API SSL错误"<<errors.size()<<endl;
    for (const QSslError &e : errors)
        qDebug()<<"ssl="<<e.errorString()<<endl;
    if (apiSocket && qgetenv("ALLOW_INSECURE_SSL") == "1")
        apiSocket->ignoreSslErrors();
}

void MainWindow::onDownloadSocketEncrypted()
{
    if (!downloadSocket)
        return;
    downloadSocket->write(downloadRequestBytes);
    downloadSocket->flush();
}

void MainWindow::onDownloadSocketReadyRead()
{
    if (!downloadSocket)
        return;
    downloadResponseBytes += downloadSocket->readAll();
    if (isHttpResponseComplete(downloadResponseBytes)) {
        qDebug()<<"下载响应已接收完整，主动关闭连接"<<endl;
        downloadSocket->disconnectFromHost();
    }
}

void MainWindow::onDownloadSocketDisconnected()
{
    if (!downloadSocket)
        return;

    int status = 0;
    bool chunked = false;
    QByteArray body = extractHttpBody(downloadResponseBytes, &status, &chunked);
    if (chunked)
        body = decodeChunkedBody(body);

    downloadSocket->deleteLater();
    downloadSocket = nullptr;

    if (status != 200) {
        qDebug()<<"下载美颜图片失败"
               <<"http="<<status
               <<endl;
        if (beautifyResultButton) {
            beautifyResultButton->setText("美颜");
            beautifyResultButton->setEnabled(beautifyResultButton == pushButton[2] ? !capturedPhotoImage.isNull() : true);
        }
        if (beautifyResultStatusLabel)
            beautifyResultStatusLabel->hide();
        beautifyResultPrefix.clear();
        beautifyResultLabel = nullptr;
        beautifyResultStatusLabel = nullptr;
        beautifyResultButton = nullptr;
        return;
    }

    const QImage image = QImage::fromData(body);
    if (image.isNull()) {
        qDebug()<<"美颜图片解码失败"<<endl;
        if (beautifyResultButton) {
            beautifyResultButton->setText("美颜");
            beautifyResultButton->setEnabled(beautifyResultButton == pushButton[2] ? !capturedPhotoImage.isNull() : true);
        }
        if (beautifyResultStatusLabel)
            beautifyResultStatusLabel->hide();
        beautifyResultPrefix.clear();
        beautifyResultLabel = nullptr;
        beautifyResultStatusLabel = nullptr;
        beautifyResultButton = nullptr;
        return;
    }

    beautyImage = image;
    if (beautifyResultLabel == previewImageLabel)
        setPreviewImage(beautyImage);
    else if (beautifyResultLabel)
        beautifyResultLabel->setPixmap(QPixmap::fromImage(beautyImage));
    else
        photoLabel->setPixmap(QPixmap::fromImage(beautyImage));

    const QString prefix = beautifyResultPrefix.isEmpty() ? QString("beauty") : beautifyResultPrefix;
    const QString fileName = saveImageToAlbum(beautyImage, prefix);
    if (fileName.isEmpty())
        qDebug()<<"保存美颜图片失败"<<endl;
    else
        qDebug()<<"保存完成！"<<endl;

    if (beautifyResultLabel == previewImageLabel)
        previewImagePath = fileName;
    if (albumListWidget)
        refreshAlbum();

    if (beautifyResultButton) {
        beautifyResultButton->setText("美颜");
        beautifyResultButton->setEnabled(beautifyResultButton == pushButton[2] ? !capturedPhotoImage.isNull() : true);
    }
    if (beautifyResultStatusLabel)
        beautifyResultStatusLabel->hide();
    beautifyResultPrefix.clear();
    beautifyResultLabel = nullptr;
    beautifyResultStatusLabel = nullptr;
    beautifyResultButton = nullptr;
}

void MainWindow::onDownloadSocketError(QAbstractSocket::SocketError)
{
    if (!downloadSocket)
        return;
    if (downloadSocket->error() == QAbstractSocket::RemoteHostClosedError) {
        qDebug()<<"下载连接由服务器关闭，等待读取剩余数据"<<endl;
        return;
    }
    qDebug()<<"下载美颜图片失败"
           <<"msg="<<downloadSocket->errorString()
           <<endl;
    downloadSocket->deleteLater();
    downloadSocket = nullptr;
    if (beautifyResultButton) {
        beautifyResultButton->setText("美颜");
        beautifyResultButton->setEnabled(beautifyResultButton == pushButton[2] ? !capturedPhotoImage.isNull() : true);
    }
    if (beautifyResultStatusLabel)
        beautifyResultStatusLabel->hide();
    beautifyResultPrefix.clear();
    beautifyResultLabel = nullptr;
    beautifyResultStatusLabel = nullptr;
    beautifyResultButton = nullptr;
}

void MainWindow::onDownloadSocketSslErrors(const QList<QSslError> &errors)
{
    qDebug()<<"美颜下载 SSL错误"<<errors.size()<<endl;
    for (const QSslError &e : errors)
        qDebug()<<"ssl="<<e.errorString()<<endl;
    if (downloadSocket && qgetenv("ALLOW_INSECURE_SSL") == "1")
        downloadSocket->ignoreSslErrors();
}
