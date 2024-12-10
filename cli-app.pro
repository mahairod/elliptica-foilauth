PREFIX = elliptica
NAME = foilauth

openrepos {
    DEFINES += OPENREPOS
    CONFIG += app_settings
}

TARGET = $${PREFIX}-$${NAME}-cli
CONFIG += link_pkgconfig
PKGCONFIG += glib-2.0 gobject-2.0
QT += sql dbus concurrent

QMAKE_CXXFLAGS += -Wno-unused-parameter
QMAKE_CFLAGS += -Wno-unused-parameter

linux:equals(QT_ARCH, x86_64) {
    CONFIG += desktop
	INST_DIR=/opt
} else {
	INST_DIR=/usr/share
}

app_settings {
    # This path is hardcoded in jolla-settings
    TRANSLATIONS_PATH = /usr/share/translations
} else {
    TRANSLATIONS_PATH = $${INST_DIR}/$${TARGET}/translations
}

CONFIG(debug, debug|release) {
    DEFINES += DEBUG HARBOUR_DEBUG
}

DEFINES += PROGRAM_PREFIX=$${PREFIX}

equals(PREFIX, elliptica) {
    DEFINES += _CONSOLE
}

equals(QT_ARCH, arm64){
    message(Linking with OpenSSL)
    PKGCONFIG += libcrypto
}

# Fix libfoil compilation warnings:
DEFINES += GLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_32
DEFINES += GLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_MAX_ALLOWED

# Directories

HARBOUR_LIB_DIR = harbour-lib
HARBOUR_LIB_INCLUDE = $${HARBOUR_LIB_DIR}/include
HARBOUR_LIB_SRC = $${HARBOUR_LIB_DIR}/src

LIBGLIBUTIL_DIR = libglibutil
LIBGLIBUTIL_INCLUDE = $${LIBGLIBUTIL_DIR}/include

FOIL_DIR = foil
LIBFOIL_DIR = $${FOIL_DIR}/libfoil
LIBFOIL_INCLUDE = $${LIBFOIL_DIR}/include
LIBFOIL_SRC = $${LIBFOIL_DIR}/src

LIBFOILMSG_DIR = $${FOIL_DIR}/libfoilmsg
LIBFOILMSG_INCLUDE = $${LIBFOILMSG_DIR}/include
LIBFOILMSG_SRC = $${LIBFOILMSG_DIR}/src

# Libraries
LIBS +=  -ldl

OTHER_FILES += \
    translations/*.ts

INCLUDEPATH += \
    src \
    $${LIBFOIL_SRC} \
    $${LIBFOIL_INCLUDE} \
    $${LIBFOILMSG_INCLUDE} \
    $${LIBGLIBUTIL_INCLUDE}

HEADERS += \
    src/FoilAuth.h \
    src/FoilAuthDefs.h \
    src/FoilAuthModel.h \
    src/FoilAuthToken.h \
    src/FoilAuthTypes.h \

SOURCES += \
    src/FoilAuth.cpp \
    src/FoilAuthModel.cpp \
    src/FoilAuthToken.cpp \
    src/cli.cpp

SOURCES += \
    $$files($${LIBFOIL_SRC}/*.c) \
    $$files($${LIBFOIL_SRC}/openssl/*.c) \
    $$files($${LIBFOILMSG_SRC}/*.c) \
    $$files($${LIBGLIBUTIL_DIR}/src/*.c)

HEADERS += \
    $$files($${LIBFOIL_INCLUDE}/*.h) \
    $$files($${LIBFOILMSG_INCLUDE}/*.h) \
    $$files($${LIBGLIBUTIL_INCLUDE}/*.h)

# harbour-lib

INCLUDEPATH += \
    $${HARBOUR_LIB_INCLUDE}

HEADERS += \
    $${HARBOUR_LIB_INCLUDE}/HarbourBase32.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourDebug.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourProtoBuf.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourTask.h \

SOURCES += \
    $${HARBOUR_LIB_SRC}/HarbourBase32.cpp \
    $${HARBOUR_LIB_SRC}/HarbourProtoBuf.cpp \
    $${HARBOUR_LIB_SRC}/HarbourTask.cpp

# openssl

!equals(QT_ARCH, arm64){
SOURCES += \
    $${HARBOUR_LIB_SRC}/libcrypto.c
}

# Translations

TRANSLATION_IDBASED=-idbased

defineTest(addTrFile) {
    rel = translations/$${1}
    OTHER_FILES += $${rel}.ts
    export(OTHER_FILES)

    in = $${_PRO_FILE_PWD_}/$$rel
    out = $${OUT_PWD}/$$rel

    s = $$replace(1,-,_)
    lupdate_target = lupdate_$$s
    qm_target = qm_$$s

    $${lupdate_target}.commands = lupdate -noobsolete -locations none $${TRANSLATION_SOURCES} -ts \"$${in}.ts\" && \
        mkdir -p \"$${OUT_PWD}/translations\" &&  [ \"$${in}.ts\" != \"$${out}.ts\" ] && \
        cp -af \"$${in}.ts\" \"$${out}.ts\" || :

    $${qm_target}.path = $$TRANSLATIONS_PATH
    $${qm_target}.depends = $${lupdate_target}
    $${qm_target}.commands = lrelease $$TRANSLATION_IDBASED \"$${out}.ts\" && \
        $(INSTALL_FILE) \"$${out}.qm\" $(INSTALL_ROOT)$${TRANSLATIONS_PATH}/

    QMAKE_EXTRA_TARGETS += $${lupdate_target} $${qm_target}
    INSTALLS += $${qm_target}

    export($${lupdate_target}.commands)
    export($${qm_target}.path)
    export($${qm_target}.depends)
    export($${qm_target}.commands)
    export(QMAKE_EXTRA_TARGETS)
    export(INSTALLS)
}

LANGUAGES = de fr hu pl ru sv zh_CN

addTrFile($${TARGET})
for(l, LANGUAGES) {
    addTrFile($${TARGET}-$$l)
}

qm.path = $$TRANSLATIONS_PATH
qm.CONFIG += no_check_exist
INSTALLS += qm
