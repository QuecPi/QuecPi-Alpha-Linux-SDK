require qt5.inc
#require qt5-git.inc
require qt5-ptest.inc

SRC_URI = " \
        file://git.tar.gz \
"

CVE_PRODUCT = "qt"

S = "${WORKDIR}/git"
PV = "5.15.2+gitAUTOINC+104eae5b17"

HOMEPAGE = "http://www.qt.io"
LICENSE = "GFDL-1.3 & BSD & ( GPL-3.0 & The-Qt-Company-GPL-Exception-1.0 | The-Qt-Company-Commercial ) & ( GPL-2.0+ | LGPL-3.0 | The-Qt-Company-Commercial )"
LIC_FILES_CHKSUM = " \
    file://LICENSE.LGPL3;md5=e6a600fd5e1d9cbde2d983680233ad02 \
    file://LICENSE.GPL2;md5=b234ee4d69f5fce4486a80fdaf4a4263 \
    file://LICENSE.GPL3;md5=d32239bcb673463ab874e80d47fae504 \
    file://LICENSE.GPL3-EXCEPT;md5=763d8c535a234d9a3fb682c7ecb6c073 \
    file://LICENSE.FDL;md5=6d9f2a9af4c8b8c3c769f6cc1b6aaf7e \
"

# Patches from https://github.com/meta-qt5/qtdeclarative/commits/b5.15
# 5.15.meta-qt5.1
SRC_URI += " \
    file://0001-Use-OE_QMAKE_PATH_EXTERNAL_HOST_BINS-to-locate-qmlca.patch \
    file://0001-yarr-Include-limits-for-numeric_limits.patch \
    file://0001-qmldebug-Include-limits-header.patch \
"

LDFLAGS:append:riscv64 = " -pthread"

DEPENDS += "qtbase qtdeclarative-native"

PACKAGECONFIG ??= "qml-debug qml-network ${@bb.utils.contains('DISTRO_FEATURES', 'qt5-static', 'static', '', d)}"
PACKAGECONFIG[qml-debug] = "-qml-debug,-no-qml-debug"
PACKAGECONFIG[qml-network] = "-qml-network, -no-qml-network"
PACKAGECONFIG[static] = ",,qtdeclarative-native"

EXTRA_QMAKEVARS_CONFIGURE += "${PACKAGECONFIG_CONFARGS}"

do_install_ptest() {
    mkdir -p ${D}${PTEST_PATH}
    for var in `find ${B}/tests/auto/ -name tst_*`; do
        case=$(basename ${var})
        if [ -z `echo ${case} | grep '\.'` ]; then
            dname=$(dirname ${var})
            pdir=$(basename ${dname})
            echo ${pdir}/${case} >> ${D}${PTEST_PATH}/tst_list

            mkdir ${D}${PTEST_PATH}/${pdir}
            install -m 0744 ${var} ${D}${PTEST_PATH}/${pdir}
            data_dir=${S}/${dname##${B}}/data
            if [ -d ${data_dir} ]; then
                cp -r ${data_dir} ${D}${PTEST_PATH}/${pdir}
            fi
        fi
    done
}

do_install:append:class-nativesdk() {
    # qml files not needed in nativesdk
    rm -rf ${D}${OE_QMAKE_PATH_QML}
}

SRCREV = "104eae5b17b0ec700391e9539ee3a4f638588194"

BBCLASSEXTEND =+ "native nativesdk"
