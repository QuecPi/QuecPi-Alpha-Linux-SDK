require qt5.inc
require qt5-git.inc

LICENSE = "GPL-3.0 | The-Qt-Company-Commercial"
LIC_FILES_CHKSUM = " \
    file://LICENSE.GPL3;md5=d32239bcb673463ab874e80d47fae504 \
"

DEPENDS += "qtbase qtdeclarative qtmultimedia qtxmlpatterns"
SRC_URI = "git://gitee.com/windowhero/qtdatavis3d.git;name=qtdatavis3d;branch=5.15.2;protocol=https"
SRCREV = "1168c788a117e4556e6cd0ba1e267a86ef62b0c4"
