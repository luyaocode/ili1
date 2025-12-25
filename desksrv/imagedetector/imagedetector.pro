QT       += core gui widgets

TARGET = imagedetector
TEMPLATE = app
# 保留调试信息 + 禁用优化

include($$PWD/../mainconfig.pri)

HEADERS += \
    imagedetector.h

SOURCES += \
    imagedetector.cpp \
    main.cpp
LIBS += -lopencv_core -lopencv_dnn -lopencv_features2d -lopencv_flann -lopencv_imgcodecs \
        -lopencv_highgui -lopencv_imgproc -lopencv_ml -lopencv_objdetect -lopencv_photo -lopencv_shape \
        -lopencv_stitching -lopencv_superres -lopencv_videoio -lopencv_video -lopencv_videostab
