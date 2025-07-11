inherit cmake

SUMMARY = "Tensorflow Lite"
DESCRIPTION = "TensorFlow Lite C++ Library"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/${LICENSE};md5=89aea4e17d99a7cacdbeed46a0096b10"

DEPENDS = "\
    protobuf \
    protobuf-native \
    jpeg \
    "
SRCREV = "a94aab6bc28c7ac957e17ac808c87fc817b3d2f5"
BRANCH = "iot-ml.lnx.${@'.'.join(d.getVar('PV').split('.')[0:2])}"

SRC_URI = "\
         git://git.codelinaro.org/clo/le/external/github.com/tensorflow/tensorflow.git;protocol=https;branch=${BRANCH};destsuffix=src \
         file://tensorflow-lite.pc.in \
         file://ooura_fft-v1.0.tar.gz \
         file://neon2sse-a15b489e.tar.gz \
         "

S = "${WORKDIR}/src"

OECMAKE_SOURCEPATH = "${S}/tensorflow/lite/c"

do_configure[network] = "1"

do_configure:prepend() {

    #anton add 替代 CMake 外部下载
    mkdir -p ${WORKDIR}/build/_deps/fft2d-subbuild/fft2d-populate-prefix/src/
    cp ${FILE_DIRNAME}/tensorflow-lite-2.16.1.qcom/ooura_fft-v1.0.tar.gz \
        ${WORKDIR}/build/_deps/fft2d-subbuild/fft2d-populate-prefix/src/v1.0.tar.gz
    mkdir -p  ${WORKDIR}/build/_deps/neon2sse-subbuild/neon2sse-populate-prefix/src/
    cp ${FILE_DIRNAME}/tensorflow-lite-2.16.1.qcom/neon2sse-a15b489e.tar.gz \
       ${WORKDIR}/build/_deps/neon2sse-subbuild/neon2sse-populate-prefix/src/a15b489e1222b2087007546b4912e21293ea86ff.tar.gz

    mkdir -p ${WORKDIR}/build
    cd ${WORKDIR}/build
    cmake ../src/tensorflow/lite/c
    
#    bbnote ">>> Custom note: ooura_fft copied to expected location"
#    bbnote ">>> Current WORKDIR is: ${WORKDIR}"
#    bbnote ">>> Current BUILD dir is: ${B}"
#    bbfatal ">>> Stopping build on purpose after file copy — test complete" 

    find ${WORKDIR}/build -name Makefile -exec rm -r {} \;
    find ${WORKDIR}/build -name cmake_install.cmake -exec rm -r {} \;
    find ${WORKDIR}/build -name CMakeCache.txt -exec rm -r {} \;
    find ${WORKDIR}/build -name CMakeFiles -exec rm -rf {} +
}




OECMAKE_TARGET_COMPILE += "\
    benchmark_model \
    label_image \
    multimodel_label_image \
    inf_diff_run_eval \
    image_classify_run_eval \
    object_detect_run_eval \
    "

EXTRA_OECMAKE += "\
    -DCMAKE_SYSTEM_NAME=Linux \
    -DSYSROOT_INCDIR=${STAGING_INCDIR} \
    -DSYSROOT_LIBDIR=${STAGING_LIBDIR} \
    -DSYSROOT_BINDIR_NATIVE=${STAGING_BINDIR_NATIVE} \
    -DTFLITE_INSTALL_INCDIR=${includedir} \
    -DTFLITE_INSTALL_BINDIR=${bindir} \
    -DTFLITE_INSTALL_LIBDIR=${libdir} \
    -DTFLITE_ENABLE_XNNPACK=ON \
    -DTFLITE_ENABLE_EVALUATION_TOOLS=ON \
    -DTFLITE_ENABLE_NNAPI=OFF \
    -DTFLITE_ENABLE_RUY=ON \
    -DTFLITE_ENABLE_HEXAGON=OFF \
    -DTFLITE_ENABLE_LABEL_IMAGE=ON \
    "

PACKAGECONFIG ?= "gpu"

PACKAGECONFIG[gpu] = " -DTFLITE_ENABLE_GPU=ON ,  -DTFLITE_ENABLE_GPU=OFF, qcom-adreno vulkan-headers, qcom-adreno"

FILES_${PN} = "${libdir}/lib*.so ${bindir}/*"
FILES_${PN}-dev += "${includedir}"

SOLIBS = ".so*"
FILES_SOLIBSDEV = ""

TFLITE_HEADERS="tensorflow/lite tensorflow/core/public tensorflow/core/platform tensorflow/core/lib tensorflow/lite/examples/label_image"

do_install:append() {

    for HPATH in ${TFLITE_HEADERS};
    do
        install -d ${D}${includedir}/${HPATH}
        cd ${S}/${HPATH}
        cp --parents $(find . \( ! -name "*hexagon*" -name "*.h*" \)) ${D}${includedir}/${HPATH}
    done

    install -d ${D}${libdir}
    install ${B}/libtensorflow*.so ${D}${libdir}/

    install -d ${D}${includedir}/gemmlowp

    cd ${B}/gemmlowp
    cp --parents $(find . -name "*.h*") ${D}${includedir}/gemmlowp/

    install -d ${D}${includedir}/ruy

    cd ${B}/ruy/ruy
    cp --parents $(find . -name "*.h*") ${D}${includedir}/ruy/

    install -d ${D}${includedir}/flatbuffers

    cd ${B}/flatbuffers/include
    cp  --parents $(find . -name "*.h*") ${D}${includedir}/

    install -d ${D}${libdir}/pkgconfig
    install -m 0644 ${WORKDIR}/tensorflow-lite.pc.in ${D}${libdir}/pkgconfig/tensorflow-lite.pc
    sed -i 's:@version@:${PV}:g
        s:@libdir@:${libdir}:g
        s:@includedir@:${includedir}:g' ${D}${libdir}/pkgconfig/tensorflow-lite.pc

}
