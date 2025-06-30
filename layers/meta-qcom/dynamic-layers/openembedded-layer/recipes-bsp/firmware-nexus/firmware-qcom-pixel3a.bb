DESCRIPTION = "QCOM Firmware for Google Pixel 3a / 3a XL"

FW_QCOM_NAME = "sargo"
EXTRA_DEVICE_NAME = "bonito"
AOSP_BUILD = "sp2a.220405.003"
CHECKSUM_vendor = "85defed7"

SRC_URI[vendor.sha256sum] = "427d9a6f1d0196c0301d37353230359237df43bbee8920f987477867c6163d56"

require firmware-qcom-pixel.inc
