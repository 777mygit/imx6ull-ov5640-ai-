QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

# 目标架构判断
TARGET_ARCH = $${QT_ARCH}
contains(TARGET_ARCH, arm){
    # ==========================================
    # ARM交叉编译配置（IMX6U开发板）
    # ==========================================
    # 指向你的opencv_IMX6U目录
    OPENCV_PATH = $$(HOME)/opencv_IMX6U
    
    # 头文件路径
    INCLUDEPATH += $$OPENCV_PATH/include \
                   $$OPENCV_PATH/include/opencv \
                   $$OPENCV_PATH/include/opencv2
    
    # 库文件路径和链接库
    LIBS += -L$$OPENCV_PATH/lib \
            -lopencv_core \
            -lopencv_highgui \
            -lopencv_imgproc \
            -lopencv_videoio \
            -lopencv_imgcodecs \
            -lopencv_features2d \
            -lopencv_objdetect
} else {
    # ==========================================
    # PC本地编译配置（Ubuntu桌面）
    # ==========================================
    LIBS += -L/usr/local/lib \
            -lopencv_core \
            -lopencv_highgui \
            -lopencv_imgproc \
            -lopencv_videoio \
            -lopencv_imgcodecs

    INCLUDEPATH += /usr/local/include
}

SOURCES += \
    camera.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    camera.h \
    mainwindow.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target