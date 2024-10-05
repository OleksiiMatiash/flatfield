QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets concurrent

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    FileUtils.cpp \
    ImageProcessor.cpp \
    ImageProcessorBayer.cpp \
    ImageProcessorMono.cpp \
    ImageProcessorRGB.cpp \
    MetadataReader.cpp \
    Processor.cpp \
    ReferenceFiles.cpp \
    ReferenceTableView.cpp \
    Settings.cpp \
    main.cpp \
    Flatfield.cpp

HEADERS += \
    DataStructs.h \
    FileUtils.h \
    Flatfield.h \
    ImageProcessor.h \
    ImageProcessorBayer.h \
    ImageProcessorMono.h \
    ImageProcessorRGB.h \
    LimitingDoubleValidator.h \
    MetadataReader.h \
    Processor.h \
    ReferenceFiles.h \
    ReferenceTableView.h \
    Settings.h

FORMS += \
    Flatfield.ui


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../OpenCV/build/x64/vc16/lib/ -lopencv_world4100
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../OpenCV/build/x64/vc16/lib/ -lopencv_world4100d
else:unix: LIBS += -L$$PWD/../OpenCV/build/x64/vc16/lib/ -lopencv_world4100

INCLUDEPATH += $$PWD/../OpenCV/build/include
DEPENDPATH += $$PWD/../OpenCV/build/include

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../exiv2/lib/release/ -lexiv2
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../exiv2/lib/debug/ -lexiv2
else:unix: LIBS += -L$$PWD/../exiv2/lib/ -lexiv2

INCLUDEPATH += $$PWD/../exiv2/include $$PWD/../exiv2/build
DEPENDPATH += $$PWD/../exiv2/include
