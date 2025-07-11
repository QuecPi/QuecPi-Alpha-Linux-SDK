require qt5.inc
#require qt5-git.inc

SRC_URI = " \
        file://git.tar.gz \
"

CVE_PRODUCT = "qt"
S = "${WORKDIR}/git"
PV = "5.15.2+gitAUTOINC+cc52debd90"

HOMEPAGE = "http://www.qt.io"
LICENSE = "GFDL-1.3 & BSD & ( GPL-3.0 & The-Qt-Company-GPL-Exception-1.0 | The-Qt-Company-Commercial ) & ( GPL-2.0+ | LGPL-3.0 | The-Qt-Company-Commercial )"
LIC_FILES_CHKSUM = " \
    file://LICENSE.LGPL3;md5=e6a600fd5e1d9cbde2d983680233ad02 \
    file://LICENSE.GPL2;md5=b234ee4d69f5fce4486a80fdaf4a4263 \
    file://LICENSE.GPL3;md5=d32239bcb673463ab874e80d47fae504 \
    file://LICENSE.GPL3-EXCEPT;md5=763d8c535a234d9a3fb682c7ecb6c073 \
    file://LICENSE.FDL;md5=6d9f2a9af4c8b8c3c769f6cc1b6aaf7e \
"

DEPENDS += "qtbase qtdeclarative qtxmlpatterns"
# Patches from https://github.com/meta-qt5/qttools/commits/b5.15
# 5.15.meta-qt5.1
SRC_URI += " \
    file://0001-add-noqtwebkit-configuration.patch \
    file://0002-linguist-tools-cmake-allow-overriding-the-location-f.patch \
    file://0003-src.pro-Add-option-noqdoc-to-disable-qdoc-builds.patch \
"
SRC_URI:append:class-native = " ${@bb.utils.contains('PACKAGECONFIG', 'clang', 'file://0004-Force-native-build-of-qt-help-tools-as-qhelpgenerato.patch', '', d)}"

FILES:${PN}-tools += "${datadir}${QT_DIR_NAME}/phrasebooks"
FILES:${PN}-examples = "${datadir}${QT_DIR_NAME}/examples"

PACKAGECONFIG ??= ""
PACKAGECONFIG:append:toolchain-clang = " clang"

PACKAGECONFIG[qtwebkit] = ",,qtwebkit"
PACKAGECONFIG[clang] = ",,clang"

COMPATIBLE_HOST:toolchain-clang:riscv32 = "null"
COMPATIBLE_HOST:toolchain-clang:riscv64 = "null"

export YOCTO_ALTERNATE_EXE_PATH = "${STAGING_BINDIR}/llvm-config"

EXTRA_QMAKEVARS_PRE += " \
    ${@bb.utils.contains('PACKAGECONFIG', 'qtwebkit', '', 'CONFIG+=noqtwebkit', d)} \
    ${@bb.utils.contains('PACKAGECONFIG', 'clang', 'CONFIG+=disable_external_rpath CONFIG+=assistant', 'CONFIG+=noqdoc', d)} \
"
SRCREV = "cc52debd905e0ed061290d6fd00a5f1ab67478a5"

BBCLASSEXTEND = "native nativesdk"

