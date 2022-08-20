TEMPLATE = app
CONFIG += console c++11
CONFIG -= qt

win32 {
LIBS += -liconv
}

QMAKE_LFLAGS += -static -static-libgcc -static-libstdc++
QMAKE_CXXFLAGS += -Wno-narrowing
CONFIG(release, debug|release) {
    QMAKE_CXXFLAGS += -DNDEBUG
}

SOURCES += main.cpp \
    functionsection.cpp \
    section.cpp \
    imagesection.cpp \
    unicodestring.cpp \
    csxparser.cpp \
    positioncomputer.cpp \
    header.cpp \
    talk.cpp \
    choice.cpp \
    txtparser.cpp \
    special.cpp

HEADERS += \
    functionsection.h \
    section.h \
    imagesection.h \
    unicodestring.h \
    csxparser.h \
    positioncomputer.h \
    header.h \
    talk.h \
    choice.h \
    txtparser.h \
    special.h

OTHER_FILES +=

