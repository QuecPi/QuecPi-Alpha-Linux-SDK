/*
 * Copyright (c) 2013-2019 The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This file was originally distributed by Qualcomm Atheros, Inc.
 * under proprietary terms before Copyright ownership was assigned
 * to the Linux Foundation.
 */

#include <linux/firmware.h>
#include <linux/pm_qos.h>
#include "ol_if_athvar.h"
#include "ol_fw.h"
#include "targaddrs.h"
#include "bmi.h"
#include "ol_cfg.h"
#include "vos_api.h"
#include "wma_api.h"
#include "wma.h"
#if defined(HIF_PCI)
#include "if_pci.h"
#elif defined(HIF_USB)
#include "if_usb.h"
#else
#include "if_ath_sdio.h"
#include "regtable.h"
#endif

#define ATH_MODULE_NAME bmi
#include "a_debug.h"
#include "fw_one_bin.h"
#include "bin_sig.h"
#include "ar6320v2_dbg_regtable.h"
#include "epping_main.h"
#include "vos_cnss.h"

#ifndef REMOVE_PKT_LOG
#include "ol_txrx_types.h"
#include "pktlog_ac.h"
#endif

#include "qwlan_version.h"

#ifdef FW_RAM_DUMP_TO_PROC
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/proc_fs.h> /* Necessary because we use the proc fs */
#include <linux/uaccess.h> /* for copy_to_user */
#endif

#ifdef FEATURE_SECURE_FIRMWARE
static struct hash_fw fw_hash;
#endif

#if defined(HIF_PCI) || defined(HIF_SDIO)
static u_int32_t refclk_speed_to_hz[] = {
	48000000, /* SOC_REFCLK_48_MHZ */
	19200000, /* SOC_REFCLK_19_2_MHZ */
	24000000, /* SOC_REFCLK_24_MHZ */
	26000000, /* SOC_REFCLK_26_MHZ */
	37400000, /* SOC_REFCLK_37_4_MHZ */
	38400000, /* SOC_REFCLK_38_4_MHZ */
	40000000, /* SOC_REFCLK_40_MHZ */
	52000000, /* SOC_REFCLK_52_MHZ */
};
#endif

#ifdef HIF_PCI
#define MAX_SECTION_COUNT 5
#else
#define MAX_SECTION_COUNT 4
#endif

#ifdef HIF_SDIO

#ifdef MULTI_IF_NAME
#define PREFIX MULTI_IF_NAME "/"
#else
#define PREFIX ""
#endif

static struct ol_fw_files FW_FILES_QCA6174_FW_1_1 = {
	PREFIX "qwlan11.bin", "", PREFIX "bdwlan11.bin",
	PREFIX "otp11.bin", PREFIX "utf11.bin",
	PREFIX "utfbd11.bin", PREFIX "qsetup11.bin",
	PREFIX "epping11.bin"};
static struct ol_fw_files FW_FILES_QCA6174_FW_2_0 = {
	PREFIX "qwlan20.bin", "", PREFIX "bdwlan20.bin",
	PREFIX "otp20.bin", PREFIX "utf20.bin",
	PREFIX "utfbd20.bin", PREFIX "qsetup20.bin",
	PREFIX "epping20.bin"};
static struct ol_fw_files FW_FILES_QCA6174_FW_1_3 = {
	PREFIX "qwlan13.bin", "", PREFIX "bdwlan13.bin",
	PREFIX "otp13.bin", PREFIX "utf13.bin",
	PREFIX "utfbd13.bin", PREFIX "qsetup13.bin",
	PREFIX "epping13.bin"};
static struct ol_fw_files FW_FILES_QCA6174_FW_3_0 = {
	PREFIX "qwlan30.bin", PREFIX "qwlan30i.bin", PREFIX "bdwlan30.bin",
	PREFIX "otp30.bin", PREFIX "utf30.bin",
	PREFIX "utfbd30.bin", PREFIX "qsetup30.bin",
	PREFIX "epping30.bin"};
static struct ol_fw_files FW_FILES_DEFAULT = {
	PREFIX "qwlan.bin", "", PREFIX "bdwlan.bin",
	PREFIX "otp.bin", PREFIX "utf.bin",
	PREFIX "utfbd.bin", PREFIX "qsetup.bin",
	PREFIX "epping.bin"};

static A_STATUS ol_sdio_extra_initialization(struct ol_softc *scn);

static int ol_get_fw_files_for_target(struct ol_fw_files *pfw_files,
                                 u32 target_version)
{
    if (!pfw_files)
        return -ENODEV;

    switch (target_version) {
    case AR6320_REV1_VERSION:
    case AR6320_REV1_1_VERSION:
            memcpy(pfw_files, &FW_FILES_QCA6174_FW_1_1, sizeof(*pfw_files));
            break;
    case AR6320_REV1_3_VERSION:
            memcpy(pfw_files, &FW_FILES_QCA6174_FW_1_3, sizeof(*pfw_files));
            break;
    case AR6320_REV2_1_VERSION:
            memcpy(pfw_files, &FW_FILES_QCA6174_FW_2_0, sizeof(*pfw_files));
            break;
    case AR6320_REV3_VERSION:
    case AR6320_REV3_2_VERSION:
            memcpy(pfw_files, &FW_FILES_QCA6174_FW_3_0, sizeof(*pfw_files));
            break;
    case QCA9377_REV1_1_VERSION:
    case QCA9379_REV1_VERSION:
#ifdef CONFIG_TUFELLO_DUAL_FW_SUPPORT
            memcpy(pfw_files, &FW_FILES_DEFAULT, sizeof(*pfw_files));
#else
            memcpy(pfw_files, &FW_FILES_QCA6174_FW_3_0, sizeof(*pfw_files));
#endif
            break;
    default:
            memcpy(pfw_files, &FW_FILES_DEFAULT, sizeof(*pfw_files));
            pr_err("%s version mismatch 0x%X ",
                            __func__, target_version);
            break;
    }
    return 0;
}
#endif
#ifdef FW_RAM_DUMP_TO_FILE
#define GET_INODE_FROM_FILEP(filp) ((filp)->f_path.dentry->d_inode)

#if (defined(__ANDROID_COMMON_KERNEL__) && \
	(LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)))
int _readwrite_file(const char *filename, char *rbuf,
	const char *wbuf, size_t length, int mode)
{
	return -ENOTSUPP;
}
#else
int _readwrite_file(const char *filename, char *rbuf,
	const char *wbuf, size_t length, int mode)
{
	int ret = 0;
	struct file *filp = (struct file *)-ENOENT;
	mm_segment_t oldfs;
	oldfs = get_fs();
	set_fs(KERNEL_DS);

	do {
		filp = filp_open(filename, mode, S_IRUSR);

		if (IS_ERR(filp) || !filp->f_op) {
			ret = -ENOENT;
			break;
		}

		if (length == 0) {
			/* Read the length of the file only */
			struct inode    *inode;

			inode = GET_INODE_FROM_FILEP(filp);
			if (!inode) {
				printk(KERN_ERR
					"_readwrite_file: Error 2\n");
				ret = -ENOENT;
				break;
			}
			ret = i_size_read(inode->i_mapping->host);
			break;
		}

		if (wbuf) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0))
			ret = kernel_write(
#else
			ret = vfs_write(
#endif
				filp, wbuf, length, &filp->f_pos);
			if (ret < 0) {
				printk(KERN_ERR
					"_readwrite_file: Error 3\n");
				break;
			}
		} else {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0))
			ret = kernel_read(
#else
			ret = vfs_read(
#endif
				filp, rbuf, length, &filp->f_pos);
			if (ret < 0) {
				printk(KERN_ERR
					"_readwrite_file: Error 4\n");
				break;
			}
		}
	} while (0);

	if (!IS_ERR(filp))
		filp_close(filp, NULL);

	set_fs(oldfs);
	return ret;
}
#endif

#define CRASH_DUMP_PATH "/var/"
#define CRASH_DUMP_FILE "/var/cld_fwcrash.log"

static void crash_dump_file_init(const char* filename)
{
	int ret;

	ret = _readwrite_file(filename, NULL,
	                NULL, 0, (O_WRONLY | O_CREAT | O_TRUNC));
	if (ret < 0) {
		printk("%s:%d: fail to write\n", __func__, __LINE__);
	}

}

#if defined(HIF_USB)
void crash_dump_init(struct ol_softc *scn)
{
#define REGISTER_SIZE 47924
	int i, k;
	size_t fw_ram_seg_size[FW_RAM_SEG_CNT + 1] = {REGISTER_SIZE, DRAM_SIZE,
						      IRAM_SIZE, AXI_SIZE};

	for (i = 0; i < FW_RAM_SEG_CNT + 1; i++) {
		scn->ramdump[i] = (v_VOID_t *)kmalloc(sizeof(struct fw_ramdump) +
				   fw_ram_seg_size[i], GFP_KERNEL);
		if (!scn->ramdump[i]) {
			pr_err("Fail to allocate memory for scn ram dump %d", i);
			for (k = 0; k < i; k++) {
				kfree(scn->ramdump[k]);
				scn->ramdump[k] = NULL;
			}
			pr_err("crash dump initial failed");
			return;
		}
		(scn->ramdump[i])->mem = (A_UINT8 *) (scn->ramdump[i] + 1);
		(scn->ramdump[i])->length = 0;
	}
}
void crash_dump_exit(struct ol_softc *scn)
{
	int i = 0;

	for (i = 0; i < FW_RAM_SEG_CNT+1; i++) {
		if(scn->ramdump[i] != NULL)
			kfree(scn->ramdump[i]);
	}
}

static void crash_dump_write_buf(struct ol_softc *scn,char* buf, unsigned int len)
{
	A_UINT8 *ram_ptr = NULL;

	ram_ptr = (scn->ramdump[0])->mem + (scn->ramdump[0])->length;

	memcpy(ram_ptr, buf, len);
	(scn->ramdump[0])->length += len;
}
static void crash_dump_write(struct ol_softc *scn, char* fmt, ...)
{
	unsigned int len;
	A_UINT8 buf[128];
	va_list args;

	va_start(args, fmt);
	len = vsnprintf(buf, sizeof(buf) - 1, fmt, args);
	va_end(args);

	return crash_dump_write_buf(scn, buf, len);
}

static void writefile_work(struct work_struct *work)
{
	int status, i;
	char fw_dump_filename[40];
	struct ol_softc *scn = container_of(work, struct ol_softc, ramdump_usb_work);
	char *fw_ram_seg_name[FW_RAM_SEG_CNT] = {"DRAM", "IRAM", "AXI"};

	for(i = 0; i < FW_RAM_SEG_CNT+1; i++) {
		memset(fw_dump_filename, 0, sizeof(fw_dump_filename));
		if(i == 0)
			memcpy(fw_dump_filename, CRASH_DUMP_FILE,
			       sizeof(CRASH_DUMP_FILE));
		else
			scnprintf(fw_dump_filename, sizeof(fw_dump_filename),
				  "%scld_%s.bin", CRASH_DUMP_PATH, fw_ram_seg_name[i-1]);
		crash_dump_file_init(fw_dump_filename);

		if(scn->ramdump[i]) {
			status = _readwrite_file(fw_dump_filename, NULL,
						 (scn->ramdump[i])->mem,
						 (scn->ramdump[i])->length,
						 (O_RDWR|O_APPEND));
			scn->ramdump[i]->length = 0;
			scn->ramdump[i]->start_addr = 0;
			scn->ramdump[i]->mem = 0;
			if (status < 0) {
				printk(KERN_ERR "write failed with status code 0x%x\n", status);
				return;
			}
		}
	}
}
#else
static void crash_dump_write(const char* filename, char* buf, unsigned int len)
{
	int ret;
	ret = _readwrite_file(filename, NULL, buf, len,
	                      (O_WRONLY | O_APPEND));
	if (ret < 0) {
		printk("%s:%d: fail to write\n", __func__, __LINE__);
	}
}
#endif
#endif

#ifdef FW_RAM_DUMP_TO_PROC
#define PROCFS_CRASH_DUMP_DIR "crash"
#define PROCFS_CRASH_DUMP_NAME "ramdump"
#define PROCFS_CRASH_DUMP_PERM 0444

static struct proc_dir_entry *crash_file, *crash_dir;

/** crash_dump_get_file_data() - get data available in proc file
 *
 * @file - handle for the proc file.
 *
 * This function is used to retrieve the data passed while
 * creating proc file entry.
 *
 * Return: void pointer to ol_softc
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)) || defined(WITH_BACKPORTS)
static void *crash_dump_get_file_data(struct file *file)
{
	void *scn;

	scn = PDE_DATA(file_inode(file));
	return scn;
}
#else
static void *crash_dump_get_file_data(struct file *file)
{
	void *scn;

	scn = PDE(file->f_path.dentry->d_inode)->data;
	return scn;
}
#endif
/**
 * crash_dump_read() - perform read operation in ram dump proc file
 *
 * @file  - handle for the proc file.
 * @buf   - pointer to user space buffer.
 * @count - number of bytes to be read.
 * @pos   - offset in the from buffer.
 *
 * This function performs read operation for the ram dump proc file.
 *
 * Return: number of bytes read on success, error code otherwise.
 */
static ssize_t crash_dump_read(struct file *file, char __user *buf,
					size_t count, loff_t *pos)
{
	struct ol_softc *scn;
	size_t no_of_bytes_read = 0;
#ifdef HIF_USB
	int sec_len = 0, i = 0;
	size_t pos_data = *pos;
#endif

	scn = crash_dump_get_file_data(file);

#if defined(HIF_USB)
	for (i = 0; i < FW_RAM_SEG_CNT; i++) {
		if (pos_data >= (scn->ramdump[i])->length) {
			sec_len += (scn->ramdump[i])->length;
			pos_data -= (scn->ramdump[i])->length;
		}
		else {
			break;
		}
	}

	if (i < FW_RAM_SEG_CNT) {
		if(scn->ramdump[i]) {
			no_of_bytes_read = MIN((scn->ramdump[i])->length -
					       (*pos - sec_len), count);

			if (copy_to_user(buf, (scn->ramdump[i])->mem +
			    (*pos - sec_len), no_of_bytes_read)) {
				printk(KERN_ERR "copy to user space failed");
				return -EFAULT;
			}
			/* offset(pos) updated based on the copy done */
			*pos += no_of_bytes_read;
		}
	}
	else {
		printk(KERN_DEBUG "Fw crash ram dump completes");
	}
#else
	if (*pos < scn->ramdump_size) {
		if(scn->ramdump_base) {
			no_of_bytes_read = MIN(scn->ramdump_size -
					       *pos, count);
			if (copy_to_user(buf, scn->ramdump_base + *pos,
			    no_of_bytes_read)) {
				printk(KERN_ERR "copy to user space failed");
				return -EFAULT;
			}
			*pos += no_of_bytes_read;
		}
	}
	else {
		printk(KERN_DEBUG "Fw crash ram dump completes");
	}
#endif
	return no_of_bytes_read;
}
/**
 * struct crash_dump_fops - file operations for crash firmware ram dump feature
 * @read - read function for crash dump operation.
 *
 * This structure initialize the file operation handle for crash
 * dump feature
 */
static const struct file_operations crash_dump_fops = {
	read: crash_dump_read
};

/**
 * crash_dump_procfs_remove() - Remove file/dir under procfs for crash dump
 *
 * This function removes file/dir under proc file system that was
 * processing irmware crash dump
 *
 * Return:  None
 */
static void crash_dump_procfs_remove(void)
{
	if (crash_file) {
		remove_proc_entry(PROCFS_CRASH_DUMP_NAME, crash_dir);
		pr_debug("/proc/%s/%s removed\n", PROCFS_CRASH_DUMP_DIR,
				 PROCFS_CRASH_DUMP_NAME);
		crash_file = NULL;
	}
	if (crash_dir) {
		remove_proc_entry(PROCFS_CRASH_DUMP_DIR, NULL);
		pr_debug("/proc/%s removed\n", PROCFS_CRASH_DUMP_DIR);
		crash_dir = NULL;
	}
}

/**
 * crash_dump_procfs_init() - Initialize procfs for memory dump
 *
 * @scn - struct ol_softc
 *
 * This function create file under proc file system to be used later for
 * processing firmware ram dump
 *
 * Return:   0 on success, error code otherwise.
 */
static int crash_dump_procfs_init(struct ol_softc *scn)
{
	crash_dir = proc_mkdir(PROCFS_CRASH_DUMP_DIR, NULL);
	if (crash_dir == NULL) {
		crash_dump_procfs_remove();
		pr_debug("Error: Could not initialize /proc/%s\n",
			 PROCFS_CRASH_DUMP_DIR);
		return -ENOMEM;
	}

	crash_file = proc_create_data(PROCFS_CRASH_DUMP_NAME,
				     PROCFS_CRASH_DUMP_PERM,
				     crash_dir,
				     &crash_dump_fops, scn);
	if (crash_file == NULL) {
		crash_dump_procfs_remove();
		pr_debug("Error: Could not initialize /proc/debug/%s\n",
			 PROCFS_CRASH_DUMP_NAME);
		return -ENOMEM;
	}

	pr_err("/proc/%s/%s created\n", PROCFS_CRASH_DUMP_DIR,
	       PROCFS_CRASH_DUMP_NAME);
	return 0;
}

#ifdef HIF_USB
void crash_dump_init(struct ol_softc *scn)
{
	int i, k;
	size_t fw_ram_seg_size[FW_RAM_SEG_CNT] = {DRAM_SIZE, IRAM_SIZE, AXI_SIZE};

	for (i = 0; i < FW_RAM_SEG_CNT; i++) {
		scn->ramdump[i] = (v_VOID_t *)kmalloc(sizeof(struct fw_ramdump) +
				   fw_ram_seg_size[i], GFP_KERNEL);
		if (!scn->ramdump[i]) {
			pr_err("Fail to allocate memory for scn ram dump %d", i);
			for (k = 0; k < i; k++) {
				kfree(scn->ramdump[k]);
				scn->ramdump[k] = NULL;
			}
			pr_err("crash dump initial failed");
			VOS_BUG(i);
			return;
		}
		(scn->ramdump[i])->mem = (A_UINT8 *) (scn->ramdump[i] + 1);
		(scn->ramdump[i])->length = 0;
	}
	crash_dump_procfs_init(scn);
}

void crash_dump_exit(struct ol_softc *scn)
{
	int k;

	crash_dump_procfs_remove();

	for (k = 0; k < FW_RAM_SEG_CNT; k++) {
		if(scn->ramdump[k] != NULL) {
			vos_mem_free(scn->ramdump[k]);
			scn->ramdump[k] = NULL;
		}
	}
}
#else
void crash_dump_exit(void)
{
	crash_dump_procfs_remove();
}
#endif
#endif

extern int qca_request_firmware(const struct firmware **firmware_p, const char *name,struct device *device);

#ifdef CONFIG_NON_QC_PLATFORM_PCI
static struct non_qc_platform_pci_fw_files FW_FILES_QCA6174_FW_1_1 = {
"qwlan11.bin", "bdwlan11.bin", "otp11.bin", "utf11.bin",
"utfbd11.bin", "epping11.bin", "evicted11.bin"};
static struct non_qc_platform_pci_fw_files FW_FILES_QCA6174_FW_2_0 = {
"qwlan20.bin", "bdwlan20.bin", "otp20.bin", "utf20.bin",
"utfbd20.bin", "epping20.bin", "evicted20.bin"};
static struct non_qc_platform_pci_fw_files FW_FILES_QCA6174_FW_1_3 = {
"qwlan13.bin", "bdwlan13.bin", "otp13.bin", "utf13.bin",
"utfbd13.bin", "epping13.bin", "evicted13.bin"};
static struct non_qc_platform_pci_fw_files FW_FILES_QCA6174_FW_3_0 = {
"qwlan30.bin", "bdwlan30.bin", "otp30.bin", "utf30.bin",
"utfbd30.bin", "epping30.bin", "evicted30.bin"};
static struct non_qc_platform_pci_fw_files FW_FILES_DEFAULT = {
"qwlan.bin", "bdwlan.bin", "otp.bin", "utf.bin",
"utfbd.bin", "epping.bin", "evicted.bin"};

static
int get_fw_files_for_non_qc_pci_target(struct non_qc_platform_pci_fw_files *pfw_files,
                           u32 target_type, u32 target_version)
{
	if (!pfw_files)
		return -ENODEV;

	switch (target_version) {
		case AR6320_REV1_VERSION:
		case AR6320_REV1_1_VERSION:
			memcpy(pfw_files, &FW_FILES_QCA6174_FW_1_1,
						sizeof(*pfw_files));
		break;
		case AR6320_REV1_3_VERSION:
			memcpy(pfw_files, &FW_FILES_QCA6174_FW_1_3,
						sizeof(*pfw_files));
			break;
		case AR6320_REV2_1_VERSION:
			memcpy(pfw_files, &FW_FILES_QCA6174_FW_2_0,
						sizeof(*pfw_files));
			break;
		case AR6320_REV3_VERSION:
		case AR6320_REV3_2_VERSION:
		case QCA9377_REV1_1_VERSION:
		case QCA9379_REV1_VERSION:
			memcpy(pfw_files, &FW_FILES_QCA6174_FW_3_0,
						sizeof(*pfw_files));
			break;
		default:
			memcpy(pfw_files, &FW_FILES_DEFAULT,
						sizeof(*pfw_files));
			printk("%s version mismatch 0x%X 0x%X",
				__func__, target_type, target_version);
			break;
	}
	return 0;
}
#endif
#ifdef HIF_USB
static A_STATUS ol_usb_extra_initialization(struct ol_softc *scn);
#endif

extern int
dbglog_parse_debug_logs(ol_scn_t scn, u_int8_t *datap, u_int32_t len);

static int ol_transfer_single_bin_file(struct ol_softc *scn,
				       u_int32_t address,
				       bool compressed)
{
	int status = EOK;
	const char *filename = AR61X4_SINGLE_FILE;
	const struct firmware *fw_entry;
	u_int32_t fw_entry_size;
	u_int8_t *temp_eeprom = NULL;
	FW_ONE_BIN_META_T *one_bin_meta_header = NULL;
	FW_BIN_HEADER_T *one_bin_header = NULL;
	SIGN_HEADER_T *sign_header = NULL;
	unsigned char *fw_entry_data = NULL;
	u_int32_t groupid = WLAN_GROUP_ID;
	u_int32_t binary_offset = 0;
	u_int32_t binary_len = 0;
	u_int32_t next_tag_offset = 0;
	u_int32_t param = 0;
	bool meta_header = FALSE;
	bool fw_sign = FALSE;
	bool is_group = FALSE;

#ifdef QCA_WIFI_FTM
	if (vos_get_conparam() == VOS_FTM_MODE)
		groupid = UTF_GROUP_ID;
#endif

	if (groupid == WLAN_GROUP_ID) {
		AR_DEBUG_PRINTF(ATH_DEBUG_TRC,
				("%s: Downloading mission mode firmware\n",
				 __func__));
	}
	else {
		AR_DEBUG_PRINTF(ATH_DEBUG_TRC,
				("%s: Downloading test mode firmware\n",
				__func__));
	}

	if (qca_request_firmware(&fw_entry, filename, scn->sc_osdev->device) != 0)
	{
		AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
				("%s: Failed to get %s\n",
				__func__, filename));
		return -ENOENT;
	}

	if (!fw_entry) {
		return A_ERROR;
	}
	fw_entry_size = fw_entry->size;
	fw_entry_data = (unsigned char *)fw_entry->data;
	binary_len = fw_entry_size;

	temp_eeprom = OS_MALLOC(scn->sc_osdev, fw_entry_size, GFP_ATOMIC);
	if (!temp_eeprom) {
		AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
				("%s: Memory allocation failed\n",
				__func__));
		release_firmware(fw_entry);
		return A_ERROR;
	}

	OS_MEMCPY(temp_eeprom, (u_int8_t *)fw_entry->data, fw_entry_size);

	is_group = FALSE;
	do {
		if (!meta_header) {
			if (fw_entry_size <= sizeof(FW_ONE_BIN_META_T)
			    + sizeof(FW_BIN_HEADER_T))
			{
				AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
						("%s: file size error!\n",
						__func__));
				status = A_ERROR;
				goto exit;
			}

			one_bin_meta_header = (FW_ONE_BIN_META_T*)fw_entry_data;
			if (one_bin_meta_header->magic_num != ONE_BIN_MAGIC_NUM)
			{
				AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
					("%s: one binary magic num err: %d\n",
					__func__,
					one_bin_meta_header->magic_num));
				status = A_ERROR;
				goto exit;
			}
			if (one_bin_meta_header->fst_tag_off
			    + sizeof(FW_BIN_HEADER_T) >= fw_entry_size)
			{
				AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
					("%s: one binary first tag offset error: %d\n",
					__func__, one_bin_meta_header->fst_tag_off));
				status = A_ERROR;
				goto exit;
			}

			one_bin_header = (FW_BIN_HEADER_T *)(
					 (u_int8_t *)fw_entry_data
					 + one_bin_meta_header->fst_tag_off);

                        while (one_bin_header->bin_group_id != groupid)
                        {
				if (one_bin_header->next_tag_off
				    + sizeof(FW_BIN_HEADER_T) > fw_entry_size)
				{
					AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
						("%s: tag offset is error: bin id: %d, bin len: %d, tag offset: %d \n",
						__func__, one_bin_header->binary_id,
						one_bin_header->binary_len,
						one_bin_header->next_tag_off));
					status = A_ERROR;
					goto exit;
				}

				one_bin_header = (FW_BIN_HEADER_T *)(
						(u_int8_t *)fw_entry_data
						+ one_bin_header->next_tag_off);
			}

			meta_header = TRUE;
		}

		binary_offset = one_bin_header->binary_off;
		binary_len = one_bin_header->binary_len;
		next_tag_offset = one_bin_header->next_tag_off;

		if (one_bin_header->action & ACTION_PARSE_SIG)
			fw_sign = TRUE;
		else
			fw_sign = FALSE;

		if (fw_sign)
		{
			if (binary_len < sizeof(SIGN_HEADER_T))
			{
				AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
					("%s: sign header size is error: bin id: %d, bin len: %d, sign header size: %zu \n",
					__func__, one_bin_header->binary_id,
					one_bin_header->binary_len,
					sizeof(SIGN_HEADER_T)));
				status = A_ERROR;
				goto exit;
			}
			sign_header = (SIGN_HEADER_T *)((u_int8_t *)fw_entry_data
					+ binary_offset);

			status = BMISignStreamStart(scn->hif_hdl, address,
						    (u_int8_t *)fw_entry_data
						    + binary_offset,
						    sizeof(SIGN_HEADER_T), scn);
			if (status != EOK)
			{
				AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
					("%s: unable to start sign stream\n",
					__func__));
				status = A_ERROR;
				goto exit;
			}

			binary_offset += sizeof(SIGN_HEADER_T);
			binary_len = sign_header->rampatch_len
				     - sizeof(SIGN_HEADER_T);
		}

		if (compressed)
			status = BMIFastDownload(scn->hif_hdl, address,
						 (u_int8_t *)fw_entry_data
						 + binary_offset,
						 binary_len, scn);
		else
			status = BMIWriteMemory(scn->hif_hdl, address,
						(u_int8_t *)fw_entry_data
						+ binary_offset,
						binary_len, scn);

		if (fw_sign)
		{
			binary_offset += binary_len;
			binary_len = sign_header->total_len
				     - sign_header->rampatch_len;

			if (binary_len > 0)
			{
				status = BMISignStreamStart(scn->hif_hdl, 0,
						(u_int8_t *)fw_entry_data
						+ binary_offset,
						binary_len, scn);
				if (status != EOK)
				{
					AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
						("%s:sign stream error\n",
						__func__));
				}
			}
		}

		if ((one_bin_header->action & ACTION_DOWNLOAD_EXEC)
						== ACTION_DOWNLOAD_EXEC)
		{
			param = 0;
			BMIExecute(scn->hif_hdl, address, &param, scn);
		}

		if ((next_tag_offset) > 0 &&
		    (one_bin_header->bin_group_id == groupid))
		{
			one_bin_header = (FW_BIN_HEADER_T *)(
					 (u_int8_t *)fw_entry_data
					 + one_bin_header->next_tag_off);
			if (one_bin_header->bin_group_id == groupid)
				is_group = TRUE;
			else
				is_group = FALSE;
		}
		else {
			is_group = FALSE;
		}

		if (!is_group)
			next_tag_offset = 0;

	} while (next_tag_offset > 0);

exit:
	if (temp_eeprom)
		OS_FREE(temp_eeprom);

	if (status != EOK) {
		AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
			("BMI operation failed: %d\n", __LINE__));
		release_firmware(fw_entry);
		return -1;
	}

	release_firmware(fw_entry);

	return status;
}

#ifdef FEATURE_SECURE_FIRMWARE
static int ol_check_fw_hash(const u8* data, u32 fw_size, ATH_BIN_FILE file)
{
	u8 *fw_mem = NULL;
	u8 *hash = NULL;
	u8 digest[SHA256_DIGEST_SIZE];
	u8 temp[SHA256_DIGEST_SIZE] = {};
	int ret = 0;

	switch(file) {
	case ATH_BOARD_DATA_FILE:
		hash = fw_hash.bdwlan;
		break;
	case ATH_OTP_FILE:
		hash = fw_hash.otp;
		break;
	case ATH_FIRMWARE_FILE:
#ifdef QCA_WIFI_FTM
		if (vos_get_conparam() == VOS_FTM_MODE) {
			hash = fw_hash.utf;
			break;
		}
#endif
		hash = fw_hash.qwlan;
	default:
		break;
	}

	if (!hash) {
		pr_err("No entry for file:%d Download FW in non-secure mode\n", file);
		goto end;
	}

	if (!OS_MEMCMP(hash, temp, SHA256_DIGEST_SIZE)) {
		pr_err("Download FW in non-secure mode:%d\n", file);
		goto end;
	}

	fw_mem = (u8 *)vos_get_fw_ptr();

	if (!fw_mem || (fw_size > MAX_FIRMWARE_SIZE)) {
		pr_err("No enough memory to copy FW data\n");
		ret = A_ERROR;
		goto end;
	}

	OS_MEMCPY(fw_mem, data, fw_size);

	ret = vos_get_sha_hash(fw_mem, fw_size, "sha256", digest);

	if (ret) {
		pr_err("Sha256 Hash computation failed err:%d\n", ret);
		goto end;
	}

	if (OS_MEMCMP(hash, digest, SHA256_DIGEST_SIZE) != 0) {
		pr_err("Hash Mismatch");
		vos_trace_hex_dump(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
						digest, SHA256_DIGEST_SIZE);
		vos_trace_hex_dump(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
						hash, SHA256_DIGEST_SIZE);
		ret = A_ERROR;
	}
end:
	return ret;
}
#endif

/**
 * ol_board_id_to_filename() - Auto BDF board_id to filename conversion
 * @scn:	ol_softc structure for board_id and chip_id info
 * @board_file:	o/p filename based on board_id and chip_id
 *
 * The API return board filename based on the board_id and chip_id.
 * eg: input = "bdwlan30.bin", board_id = 0x01, board_file = "bdwlan30.b01"
 * Return: The buffer with the formated board filename.
 */

#if (defined(CONFIG_CNSS) || defined(HIF_SDIO))
static char *ol_board_id_to_filename(struct ol_softc *scn, uint16_t board_id)
{
	int input_len;
	const char *input;
	char *dest = NULL;

	if (vos_get_conparam() == VOS_FTM_MODE)
		input = scn->fw_files.utf_board_data;
	else
		input = scn->fw_files.board_data;

	dest = kstrdup(input, GFP_KERNEL);

	if (!dest)
		goto out;

	input_len = adf_os_str_len(input);

	snprintf(&dest[input_len - 6], 5, "%.4x", board_id);
out:
	return dest;
}
#else
static char *ol_board_id_to_filename(struct ol_softc *scn, uint16_t board_id)
{
#ifdef CONFIG_NON_QC_PLATFORM_PCI
	return kstrdup(scn->fw_files.board_data, GFP_KERNEL);
#else

	return kstrdup(QCA_BOARD_DATA_FILE, GFP_KERNEL);
#endif
}
#endif

#if defined(CONFIG_HL_SUPPORT)
#define MAX_SUPPORTED_PEERS_REV1_1 9
#ifdef HIF_SDIO
#define MAX_SUPPORTED_PEERS 32
#else
#define MAX_SUPPORTED_PEERS 32
#endif
#else
#define MAX_SUPPORTED_PEERS_REV1_1 14
#define MAX_SUPPORTED_PEERS 32
#endif

#if defined(HIF_PCI)
const char *ol_get_fw_name(struct ol_softc *scn)
{
	return scn->fw_files.image_file;
}
#elif defined(HIF_SDIO)
const char *ol_get_fw_name(struct ol_softc *scn)
{
	const char *filename = NULL;

	if (vos_get_conparam() == VOS_IBSS_MODE) {
		filename = scn->fw_files.ibss_image_file;
		if (filename[0] != '\0') {
			scn->max_no_of_peers = MAX_SUPPORTED_PEERS;
		} else {
			filename = scn->fw_files.image_file;
		}
	} else {
		filename = scn->fw_files.image_file;
	}
	return filename;
}
#else
const char *ol_get_fw_name(struct ol_softc *scn)
{
	return QCA_FIRMWARE_FILE;
}
#endif

static int __ol_transfer_bin_file(struct ol_softc *scn, ATH_BIN_FILE file,
				u_int32_t address, bool compressed)
{
	int status = EOK;
	const char *filename = NULL;
	const struct firmware *fw_entry;
	u_int32_t fw_entry_size;
	u_int8_t *tempEeprom;
	u_int32_t board_data_size;
#ifdef QCA_SIGNED_SPLIT_BINARY_SUPPORT
	bool bin_sign = FALSE;
	int bin_off, bin_len;
	SIGN_HEADER_T *sign_header;
#endif
	int ret;
	char *bd_id_filename = NULL;

	if (scn->enablesinglebinary && file != ATH_BOARD_DATA_FILE) {
		/*
		 * Fallback to load split binaries if single binary is not found
		 */
		ret = ol_transfer_single_bin_file(scn,
						  address,
						  compressed);

		if (!ret)
			return ret;

		if (ret != -ENOENT)
			return -1;
	}

	switch (file) {
	default:
		printk("%s: Unknown file type\n", __func__);
		return -1;
	case ATH_OTP_FILE:
#if defined(CONFIG_CNSS) || defined(HIF_SDIO) || \
defined(CONFIG_NON_QC_PLATFORM_PCI)
		filename = scn->fw_files.otp_data;
#else
		filename = QCA_OTP_FILE;
#endif
#ifdef QCA_SIGNED_SPLIT_BINARY_SUPPORT
		bin_sign = TRUE;
#endif
		break;
	case ATH_FIRMWARE_FILE:
		if (WLAN_IS_EPPING_ENABLED(vos_get_conparam())) {
#if defined(CONFIG_CNSS) || defined(HIF_SDIO)
			filename = scn->fw_files.epping_file;
#else
			filename = QCA_FIRMWARE_EPPING_FILE;
#endif
			printk(KERN_INFO "%s: Loading epping firmware file %s\n",
				__func__, filename);
			break;
		}
#ifdef QCA_WIFI_FTM
		if (vos_get_conparam() == VOS_FTM_MODE) {
#if defined(CONFIG_CNSS) || defined(HIF_SDIO)
			filename = scn->fw_files.utf_file;
#else
			filename = QCA_UTF_FIRMWARE_FILE;
#endif
#ifdef QCA_SIGNED_SPLIT_BINARY_SUPPORT
			bin_sign = TRUE;
#endif
			printk(KERN_INFO "%s: Loading firmware file %s\n",
			       __func__, filename);
			break;
		}
#endif

		filename = ol_get_fw_name(scn);
#ifdef QCA_SIGNED_SPLIT_BINARY_SUPPORT
		bin_sign = TRUE;
#endif
		break;
	case ATH_PATCH_FILE:
		printk("%s: no Patch file defined\n", __func__);
		return EOK;
	case ATH_BOARD_DATA_FILE:
		bd_id_filename = ol_board_id_to_filename(scn, scn->board_id);
		if (bd_id_filename)
			filename = bd_id_filename;
		else {
			pr_err("%s: No memory to allocate board filename\n",
							__func__);
			return -1;
		}

#ifdef QCA_WIFI_FTM
		if (vos_get_conparam() == VOS_FTM_MODE) {
#ifdef QCA_SIGNED_SPLIT_BINARY_SUPPORT
			bin_sign = TRUE;
#endif
			printk(KERN_INFO "%s: Loading board data file %s\n",
				__func__, filename);
			break;
		}
#endif /* QCA_WIFI_FTM */

#ifdef QCA_SIGNED_SPLIT_BINARY_SUPPORT
		bin_sign = FALSE;
#endif
		break;
	case ATH_SETUP_FILE:
		if (vos_get_conparam() != VOS_FTM_MODE &&
		   !WLAN_IS_EPPING_ENABLED(vos_get_conparam())) {
#ifdef CONFIG_CNSS
			printk("%s: no Setup file defined\n", __func__);
			return -1;
#else
#ifdef HIF_SDIO
			filename = scn->fw_files.setup_file;
#else
			filename = QCA_SETUP_FILE;
#endif
#ifdef QCA_SIGNED_SPLIT_BINARY_SUPPORT
			bin_sign = TRUE;
#endif
			printk(KERN_INFO "%s: Loading setup file %s\n",
					__func__, filename);
#endif /* CONFIG_CNSS */
		} else {
			printk("%s: no Setup file needed\n", __func__);
			return -1;
		}
		break;
	case ATH_USB_WARM_RESET_FILE:
		filename = QCA_USB_WARM_RESET_FILE;
		break;
	}

       status = qca_request_firmware(&fw_entry, filename, scn->sc_osdev->device);
	if (status)
	{
		pr_err("%s: Failed to get %s:%d\n", __func__, filename, status);

		if (file == ATH_OTP_FILE)
			return -ENOENT;

#if (defined(CONFIG_CNSS) || defined(HIF_SDIO))

		if (file == ATH_BOARD_DATA_FILE) {
			if (strcmp(filename, scn->fw_files.board_data))
				filename = scn->fw_files.board_data;
			else {
				kfree(bd_id_filename);
				return -1;
			}

			pr_info("%s: Trying to load default %s\n",
							__func__, filename);

			status = qca_request_firmware(&fw_entry, filename,
					scn->sc_osdev->device);
			if (status) {
				pr_err("%s: Failed to get %s:%d\n",
						__func__, filename, status);
				kfree(bd_id_filename);
				return -1;
			}
		} else
			return -1;
#else
		kfree(bd_id_filename);
		return -1;
#endif
	}

	if (!fw_entry || !fw_entry->data) {
		pr_err("%s: Invalid fw_entries\n", __func__);
		status = A_NO_MEMORY;
		goto release_fw;
	}

	fw_entry_size = fw_entry->size;
	tempEeprom = NULL;

#ifdef FEATURE_SECURE_FIRMWARE
	if (scn->enable_fw_hash_check &&
	    ol_check_fw_hash(fw_entry->data, fw_entry_size, file)) {
		pr_err("Hash Check failed for file:%s\n", filename);
		status = A_ERROR;
		goto end;
	}
#endif

	if (file == ATH_BOARD_DATA_FILE)
	{
		u_int32_t board_ext_address;
		int32_t board_ext_data_size;

		tempEeprom = OS_MALLOC(scn->sc_osdev, fw_entry_size, GFP_ATOMIC);
		if (!tempEeprom) {
			pr_err("%s: Memory allocation failed\n", __func__);
			status = A_NO_MEMORY;
			goto release_fw;
		}

		OS_MEMCPY(tempEeprom, (u_int8_t *)fw_entry->data, fw_entry_size);

		status = vos_update_boarddata(tempEeprom, fw_entry_size);
		if (EOK != status) {
			AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
				("wlan: update boarddata failed, status=%d.\n",
				 status));
		}

		switch (scn->target_type) {
		case TARGET_TYPE_AR6004:
			board_data_size =  AR6004_BOARD_DATA_SZ;
			board_ext_data_size = AR6004_BOARD_EXT_DATA_SZ;
			break;
		case TARGET_TYPE_AR9888:
			board_data_size =  AR9888_BOARD_DATA_SZ;
			board_ext_data_size = AR9888_BOARD_EXT_DATA_SZ;
			break;
		default:
			board_data_size = 0;
			board_ext_data_size = 0;
			break;
		}

		/* Determine where in Target RAM to write Board Data */
		BMIReadMemory(scn->hif_hdl,
				HOST_INTEREST_ITEM_ADDRESS(scn->target_type, hi_board_ext_data),
				(u_int8_t *)&board_ext_address, 4, scn);
		printk("Board extended Data download address: 0x%x\n", board_ext_address);

		/*
		 * Check whether the target has allocated memory for extended board
		 * data and file contains extended board data
		 */
		if ((board_ext_address) && (fw_entry_size == (board_data_size + board_ext_data_size)))
		{
			u_int32_t param;

			status = BMIWriteMemory(scn->hif_hdl, board_ext_address,
					(u_int8_t *)(tempEeprom + board_data_size), board_ext_data_size, scn);

			if (status != EOK)
				goto end;

			/* Record the fact that extended board Data IS initialized */
			param = (board_ext_data_size << 16) | 1;
			BMIWriteMemory(scn->hif_hdl,
					HOST_INTEREST_ITEM_ADDRESS(scn->target_type, hi_board_ext_data_config),
					(u_int8_t *)&param, 4, scn);

			fw_entry_size = board_data_size;
		}
	}

#ifdef QCA_SIGNED_SPLIT_BINARY_SUPPORT
	if (bin_sign) {
		u_int32_t chip_id;

		if (fw_entry_size < sizeof(SIGN_HEADER_T)) {
			AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
				("%s: Invalid binary size %d\n", __func__,
				 fw_entry_size));
			status = A_ERROR;
			goto end;
		}

		sign_header = (SIGN_HEADER_T *)fw_entry->data;
		chip_id = cpu_to_le32(sign_header->product_id);
		if (sign_header->magic_num == SIGN_HEADER_MAGIC
		    && (chip_id == AR6320_REV1_1_VERSION
			|| chip_id == AR6320_REV1_3_VERSION
			|| chip_id == AR6320_REV2_1_VERSION)) {

			status = BMISignStreamStart(scn->hif_hdl, address,
						    (u_int8_t *)fw_entry->data,
						    sizeof(SIGN_HEADER_T), scn);
			if (status != EOK) {
				AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
					("%s: unable to start sign stream\n",
					__func__));
				status = A_ERROR;
				goto end;
			}

			bin_off = sizeof(SIGN_HEADER_T);
			bin_len = sign_header->rampatch_len
				  - sizeof(SIGN_HEADER_T);
		} else {
			bin_sign = FALSE;
			bin_off = 0;
			bin_len = fw_entry_size;
		}
	} else {
		bin_len = fw_entry_size;
		bin_off = 0;
	}

	if (compressed) {
		status = BMIFastDownload(scn->hif_hdl, address,
					 (u_int8_t *)fw_entry->data + bin_off,
					 bin_len, scn);
	} else {
		if (file == ATH_BOARD_DATA_FILE && fw_entry->data) {
			status = BMIWriteMemory(scn->hif_hdl, address,
						(u_int8_t *)tempEeprom,
						fw_entry_size, scn);
		} else {
			status = BMIWriteMemory(scn->hif_hdl, address,
						(u_int8_t *)fw_entry->data
						+ bin_off,
						bin_len, scn);
		}
	}

	if (bin_sign) {
		bin_off += bin_len;
		bin_len = sign_header->total_len
			  - sign_header->rampatch_len;

		if (bin_len > 0) {
			status = BMISignStreamStart(scn->hif_hdl, 0,
					(u_int8_t *)fw_entry->data + bin_off,
					bin_len, scn);
			if (status != EOK) {
				AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
					("%s:sign stream error\n",
					__func__));
			}
		}
	}
#else
	if (compressed) {
		status = BMIFastDownload(scn->hif_hdl, address,
					 (u_int8_t *)fw_entry->data,
					 fw_entry_size, scn);
	} else {
		if (file == ATH_BOARD_DATA_FILE && fw_entry->data) {
			status = BMIWriteMemory(scn->hif_hdl, address,
						(u_int8_t *)tempEeprom,
						fw_entry_size, scn);
		} else {
			status = BMIWriteMemory(scn->hif_hdl, address,
						(u_int8_t *)fw_entry->data,
						fw_entry_size, scn);
		}
	}
#endif	/* QCA_SIGNED_SPLIT_BINARY_SUPPORT */

end:
	if (tempEeprom) {
		OS_FREE(tempEeprom);
	}

	if (status != EOK) {
		pr_err("%s, BMI operation failed: %d\n", __func__, __LINE__);
		goto release_fw;
	}

	VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
		"%s: transferring file: %s size %d bytes done!", __func__,
		(filename!=NULL)?filename:"", fw_entry_size);

release_fw:
	if (fw_entry)
		release_firmware(fw_entry);

	if (bd_id_filename)
		kfree(bd_id_filename);

	return status;
}

static int ol_transfer_bin_file(struct ol_softc *scn, ATH_BIN_FILE file,
				u_int32_t address, bool compressed)
{
	int ret;

	/* Wait until suspend and resume are completed before loading FW */
	vos_lock_pm_sem();
	ret = __ol_transfer_bin_file(scn, file, address, compressed);
	vos_release_pm_sem();

	return ret;
}

u_int32_t host_interest_item_address(u_int32_t target_type, u_int32_t item_offset)
{
	switch (target_type) {
	default:
		ASSERT(0);
	case TARGET_TYPE_AR6002:
		return (AR6002_HOST_INTEREST_ADDRESS + item_offset);
	case TARGET_TYPE_AR6003:
		return (AR6003_HOST_INTEREST_ADDRESS + item_offset);
	case TARGET_TYPE_AR6004:
		return (AR6004_HOST_INTEREST_ADDRESS + item_offset);
	case TARGET_TYPE_AR6006:
		return (AR6006_HOST_INTEREST_ADDRESS + item_offset);
	case TARGET_TYPE_AR9888:
		return (AR9888_HOST_INTEREST_ADDRESS + item_offset);
	case TARGET_TYPE_AR6320:
	case TARGET_TYPE_AR6320V2:
		return (AR6320_HOST_INTEREST_ADDRESS + item_offset);
	}
}

#ifdef HIF_PCI
int dump_CE_register(struct ol_softc *scn)
{
#ifdef HIF_USB
	struct hif_usb_softc *sc = scn->hif_sc;
#else
	struct hif_pci_softc *sc = scn->hif_sc;
#endif
	A_UINT32 CE_reg_address = CE0_BASE_ADDRESS;
	A_UINT32 CE_reg_values[8][CE_USEFUL_SIZE>>2];
	A_UINT32 CE_reg_word_size = CE_USEFUL_SIZE>>2;
	A_UINT16 i, j;

	for(i = 0; i < 8; i++, CE_reg_address += CE_OFFSET) {
		if (HIFDiagReadMem(scn->hif_hdl, CE_reg_address,
			(A_UCHAR*)&CE_reg_values[i][0],
			CE_reg_word_size * sizeof(A_UINT32)) != A_OK)
		{
			printk(KERN_ERR "Dumping CE register failed!\n");
			return -EACCES;
		}
	}

	for (i = 0; i < 8; i++) {
		printk("CE%d Registers:\n", i);
		for (j = 0; j < CE_reg_word_size; j++) {
			printk("0x%08x ", CE_reg_values[i][j]);
			if (!((j+1)%5) || (CE_reg_word_size - 1) == j)
				printk("\n");
		}
	}

	return EOK;
}
#endif

#if (defined(CONFIG_CNSS) && !defined(HIF_USB)) || defined(HIF_SDIO) || \
defined(CONFIG_NON_QC_PLATFORM_PCI)
static struct ol_softc *ramdump_scn;
#ifdef TARGET_DUMP_FOR_NON_QC_PLATFORM
void *ol_fw_dram_addr=NULL;
void *ol_fw_iram_addr=NULL;
void *ol_fw_axi_addr=NULL;
u_int32_t ol_fw_dram_size;
u_int32_t ol_fw_iram_size;
u_int32_t ol_fw_axi_size;
#endif

#if defined(HIF_SDIO)
int ol_copy_ramdump(struct ol_softc *scn)
{
	int ret;

	if (!vos_is_ssr_fw_dump_required())
		return 0;

	if (!scn->ramdump_base || !scn->ramdump_size) {
		pr_info("%s: No RAM dump will be collected since ramdump_base "
			"is NULL or ramdump_size is 0!\n", __func__);
		ret = -EACCES;
		goto out;
	}

	vos_request_pm_qos_type(PM_QOS_CPU_DMA_LATENCY,
				DISABLE_KRAIT_IDLE_PS_VAL);
	ret = ol_target_coredump(scn, scn->ramdump_base, scn->ramdump_size);
	vos_remove_pm_qos();

out:
	return ret;
}
#else
int ol_copy_ramdump(struct ol_softc *scn)
{
	int ret;

	if (!vos_is_ssr_fw_dump_required())
		return 0;

	if (!scn->ramdump_base || !scn->ramdump_size) {
		pr_info("%s: No RAM dump will be collected since ramdump_base "
			"is NULL or ramdump_size is 0!\n", __func__);
		ret = -EACCES;
		goto out;
	}

	ret = ol_target_coredump(scn, scn->ramdump_base, scn->ramdump_size);

out:
	return ret;
}
#endif

static void ramdump_work_handler(struct work_struct *ramdump)
{
	struct device *dev = NULL;

#if !defined(HIF_SDIO)
#ifdef WLAN_DEBUG
	int ret;
#endif
#endif
	u_int32_t host_interest_address;
	u_int32_t dram_dump_values[4];
#ifdef TARGET_DUMP_FOR_NON_QC_PLATFORM
#ifndef CONFIG_NON_QC_PLATFORM_PCI
	u_int8_t *byte_ptr;
#endif
#endif
	if (!ramdump_scn) {
		printk("No RAM dump will be collected since ramdump_scn is NULL!\n");
		goto out_fail;
	}

	if (ramdump_scn->adf_dev && ramdump_scn->adf_dev->dev)
		dev = ramdump_scn->adf_dev->dev;

#if !defined(HIF_SDIO)
#ifdef WLAN_DEBUG
	ret = hif_pci_check_soc_status(ramdump_scn->hif_sc);
	if (ret)
		goto out_fail;

	ret = dump_CE_register(ramdump_scn);
	if (ret)
		goto out_fail;

	dump_CE_debug_register(ramdump_scn->hif_sc);
#endif
#endif

	if (HIFDiagReadMem(ramdump_scn->hif_hdl,
		host_interest_item_address(ramdump_scn->target_type,
		offsetof(struct host_interest_s, hi_failure_state)),
		(A_UCHAR *)&host_interest_address, sizeof(u_int32_t)) != A_OK) {
		printk(KERN_ERR "HifDiagReadiMem FW Dump Area Pointer failed!\n");
#if !defined(HIF_SDIO)
		ol_copy_ramdump(ramdump_scn);
		vos_device_crashed(dev);
		return;
#endif
		goto out_fail;
	}
	printk("Host interest item address: 0x%08x\n", host_interest_address);

	if (HIFDiagReadMem(ramdump_scn->hif_hdl, host_interest_address,
		(A_UCHAR *)&dram_dump_values[0], 4 * sizeof(u_int32_t)) != A_OK)
	{
		printk("HifDiagReadiMem FW Dump Area failed!\n");
		goto out_fail;
	}
	printk("FW Assertion at PC: 0x%08x BadVA: 0x%08x TargetID: 0x%08x\n",
		dram_dump_values[2], dram_dump_values[3], dram_dump_values[0]);

#ifdef TARGET_DUMP_FOR_NON_QC_PLATFORM
	/* Allocate memory to save ramdump */
	if (ramdump_scn->enableFwSelfRecovery) {
		vos_set_logp_in_progress(VOS_MODULE_ID_VOSS, FALSE);
#if defined(HIF_SDIO) && defined(WLAN_OPEN_SOURCE)
		if (dev)
			kobject_uevent(&dev->kobj, KOBJ_OFFLINE);
#endif
		goto out_fail;
	}

	/* Buffer for ramdump should be pre-allocated when probing SDIO */
	if (!ramdump_scn->ramdump_base) {
		pr_err("%s: fail to alloc mem for FW RAM dump\n",
				__func__);
		goto out_fail;
	}

#ifndef CONFIG_NON_QC_PLATFORM_PCI
	ol_fw_dram_size = DRAM_SIZE;
	ol_fw_iram_size = IRAM_SIZE;
	ol_fw_axi_size = AXI_SIZE;
	ol_fw_dram_addr = ramdump_scn->ramdump_base;
	byte_ptr = (u_int8_t *)ol_fw_dram_addr;
	ol_fw_axi_addr = (void *)(byte_ptr + DRAM_SIZE);
	ol_fw_iram_addr = (void *)(byte_ptr + DRAM_SIZE + AXI_SIZE);

	pr_err("%s: DRAM => mem = %pK, len = %d\n", __func__,
				ol_fw_dram_addr, DRAM_SIZE);
	pr_err("%s: AXI  => mem = %pK, len = %d\n", __func__,
				ol_fw_axi_addr, AXI_SIZE);
	pr_err("%s: IRAM => mem = %pK, len = %d\n", __func__,
				ol_fw_iram_addr, IRAM_SIZE);
#endif
#endif

	if (ol_copy_ramdump(ramdump_scn))
		goto out_fail;

	printk("%s: RAM dump collecting completed!\n", __func__);

#if (defined(HIF_SDIO) || defined(CONFIG_NON_QC_PLATFORM_PCI)) && !defined(CONFIG_CNSS)
#ifndef FW_RAM_DUMP_TO_PROC
	panic("CNSS Ram dump collected\n");
#endif
#else
	/* Notify SSR framework the target has crashed. */
	vos_device_crashed(dev);
#endif
	return;

out_fail:
	/* Silent SSR on dump failure */
#if defined(CNSS_SELF_RECOVERY) || defined(TARGET_DUMP_FOR_NON_QC_PLATFORM)
#if !defined(HIF_SDIO)
	vos_device_self_recovery(dev);
#endif
#else

#if defined(HIF_SDIO) && !defined(CONFIG_CNSS)
	panic("CNSS Ram dump collection failed \n");
#else
	vos_device_crashed(dev);
#endif
#endif

	vos_set_logp_in_progress(VOS_MODULE_ID_VOSS, FALSE);
	return;
}

static DECLARE_WORK(ramdump_work, ramdump_work_handler);

void ol_schedule_ramdump_work(struct ol_softc *scn)
{
	ramdump_scn = scn;
	schedule_work(&ramdump_work);
}

static void fw_indication_work_handler(struct work_struct *fw_indication)
{
	struct device *dev = NULL;

	if (ramdump_scn && ramdump_scn->adf_dev
			&& ramdump_scn->adf_dev->dev)
		dev = ramdump_scn->adf_dev->dev;

	if (!dev) {
		pr_err("%s Device is Invalid\n", __func__);
		return;
	}

	vos_device_self_recovery(dev);
}

static DECLARE_WORK(fw_indication_work, fw_indication_work_handler);

void ol_schedule_fw_indication_work(struct ol_softc *scn)
{
	ramdump_scn = scn;
	schedule_work(&fw_indication_work);
}
#elif defined(HIF_USB)
void ol_schedule_fw_indication_work(struct ol_softc *scn)
{
}
void ol_schedule_ramdump_work(struct ol_softc *scn)
{
	VOS_BUG(0);
}
#endif

#ifdef HIF_USB
/* Save memory addresses where we save FW ram dump, and then we could obtain
 * them by symbol table. */
A_UINT32 fw_stack_addr;
void *fw_ram_seg_addr[FW_RAM_SEG_CNT];

/* ol_ramdump_handler is to receive information of firmware crash dump, and
 * save it in host memory. It consists of 5 parts: registers, call stack,
 * DRAM dump, IRAM dump, and AXI dump, and they are reported to host in order.
 *
 * registers: wrapped in a USB packet by starting as FW_ASSERT_PATTERN and
 *            60 registers.
 * call stack: wrapped in multiple USB packets, and each of them starts as
 *             FW_REG_PATTERN and contains multiple double-words. The tail
 *             of the last packet is FW_REG_END_PATTERN.
 * DRAM dump: wrapped in multiple USB pakcets, and each of them start as
 *            FW_RAMDUMP_PATTERN and contains multiple double-wors. The tail
 *            of the last packet is FW_RAMDUMP_END_PATTERN;
 * IRAM dump and AXI dump are with the same format as DRAM dump.
 */
void ol_ramdump_handler(struct ol_softc *scn)
{
	A_UINT32 *reg, pattern, i, start_addr = 0;
	A_UINT32 MSPId = 0, mSPId = 0, SIId = 0, CRMId = 0, len;
	A_UINT8 *data;
	A_UINT8 str_buf[128];
	A_UINT8 *ram_ptr = NULL;
	A_UINT32 remaining;
	char *fw_ram_seg_name[FW_RAM_SEG_CNT] = {"DRAM", "IRAM", "AXI"};
	size_t fw_ram_seg_size[FW_RAM_SEG_CNT] = {DRAM_SIZE, IRAM_SIZE, AXI_SIZE};

	data = scn->hif_sc->fw_data;
	len = scn->hif_sc->fw_data_len;
	pattern = *((A_UINT32 *) data);

	if (pattern == FW_ASSERT_PATTERN) {
		MSPId = (scn->target_fw_version & 0xf0000000) >> 28;
		mSPId = (scn->target_fw_version & 0xf000000) >> 24;
		SIId = (scn->target_fw_version & 0xf00000) >> 20;
		CRMId = scn->target_fw_version & 0x7fff;
		pr_err("Firmware crash detected...\n");
		pr_err("Host SW version: %s\n", QWLAN_VERSIONSTR);
		pr_err("FW version: %d.%d.%d.%d", MSPId, mSPId, SIId, CRMId);
#ifdef FW_RAM_DUMP_TO_FILE
		crash_dump_write(scn, "Firmware crash detected...\n");
		crash_dump_write(scn, "Host SW version: %s\n", QWLAN_VERSIONSTR);
		crash_dump_write(scn, "FW version: %d.%d.%d.%d\n", MSPId, mSPId, SIId, CRMId);
#endif

		if (vos_is_load_unload_in_progress(VOS_MODULE_ID_VOSS, NULL)) {
			printk("%s: Loading/Unloading is in progress, ignore!\n",
				__func__);
			return;
		}

		if (scn->enableFwSelfRecovery || scn->enableRamdumpCollection)
			vos_set_logp_in_progress(VOS_MODULE_ID_VOSS, TRUE);

		reg = (A_UINT32 *) (data + 4);
#ifdef FW_RAM_DUMP_TO_FILE
		for (i = 0; i < min_t(A_UINT32, len - 4, FW_REG_DUMP_CNT); i += 4) {
			memset(str_buf, 0, sizeof(str_buf));
			hex_dump_to_buffer(reg+i, 16, 16, 4, str_buf, sizeof(str_buf), false);
			crash_dump_write(scn, "%#08x: %s\n", i*4, str_buf);
		}
#endif
		print_hex_dump(KERN_DEBUG, " ", DUMP_PREFIX_OFFSET, 16, 4, reg,
				min_t(A_UINT32, len - 4, FW_REG_DUMP_CNT * 4),
				false);
		scn->fw_ram_dumping = 0;

	}
	else if (pattern == FW_REG_PATTERN) {
		reg = (A_UINT32 *) (data + 4);
		start_addr = *reg++;
		if (scn->fw_ram_dumping == 0) {
			pr_err("Firmware stack dump:");
#ifdef FW_RAM_DUMP_TO_FILE
			crash_dump_write(scn, "Firmware stack dump:\n");
#endif
			scn->fw_ram_dumping = 1;
			fw_stack_addr = start_addr;
		}
		remaining = len - 8;
		/* len is in byte, but it's printed in double-word. */
		for (i = 0; i < (len - 8); i += 16) {
			if ((*reg == FW_REG_END_PATTERN) && (i == len - 12)) {
				scn->fw_ram_dumping = 0;
				pr_err("Stack start address = %#08x\n",
					fw_stack_addr);
#ifdef FW_RAM_DUMP_TO_FILE
				crash_dump_write(scn, "Stack start address = %#08x\n",
						 fw_stack_addr);
#endif
				break;
			}
			hex_dump_to_buffer(reg, remaining, 16, 4, str_buf,
						sizeof(str_buf), false);
#ifndef FW_RAM_DUMP_TO_FILE
			pr_err("%#08x: %s\n", start_addr + i, str_buf);
#else
			crash_dump_write(scn,"%#08x: %s\n", start_addr + i, str_buf);
#endif
			remaining -= 16;
			reg += 4;
		}
	}
	else if ((!scn->enableFwSelfRecovery)&&
			((pattern & FW_RAMDUMP_PATTERN_MASK) ==
						FW_RAMDUMP_PATTERN)) {
		VOS_ASSERT(scn->ramdump_index < FW_RAM_SEG_CNT);
#ifndef FW_RAM_DUMP_TO_FILE
		i = scn->ramdump_index;
#else
		i = scn->ramdump_index + 1;
#endif
		reg = (A_UINT32 *) (data + 4);
		if (scn->fw_ram_dumping == 0) {
			scn->fw_ram_dumping = 1;
#ifndef FW_RAM_DUMP_TO_FILE
			pr_err("Firmware %s dump:\n", fw_ram_seg_name[i]);
#else
			pr_err("Firmware %s dump:\n", fw_ram_seg_name[i - 1]);
#endif

#if !defined(FW_RAM_DUMP_TO_PROC) && !defined(FW_RAM_DUMP_TO_FILE)
			scn->ramdump[i] = kmalloc(sizeof(struct fw_ramdump) +
							fw_ram_seg_size[i],
							GFP_KERNEL);
			if (!scn->ramdump[i]) {
				pr_err("Fail to allocate memory for ram dump");
				VOS_BUG(0);
			}
			(scn->ramdump[i])->mem =
				(A_UINT8 *) (scn->ramdump[i] + 1);
			fw_ram_seg_addr[i] = (scn->ramdump[i])->mem;
			pr_err("FW %s start addr = %#08x\n",
				fw_ram_seg_name[i], *reg);
			pr_err("Memory addr for %s = %pK\n",
				fw_ram_seg_name[i],
				(scn->ramdump[i])->mem);
			(scn->ramdump[i])->length = 0;
#endif
			(scn->ramdump[i])->start_addr = *reg;
		}
		reg++;
		ram_ptr = (scn->ramdump[i])->mem + (scn->ramdump[i])->length;
		(scn->ramdump[i])->length += (len - 8);
#ifndef FW_RAM_DUMP_TO_FILE
		if ((scn->ramdump[i])->length <= fw_ram_seg_size[i]) {
#else
		if ((scn->ramdump[i])->length <= fw_ram_seg_size[i - 1]) {
#endif
			memcpy(ram_ptr, (A_UINT8 *) reg, len - 8);
		}
		else {
			pr_err("memory copy overlap \n");
			VOS_BUG(0);
		}

		if (pattern == FW_RAMDUMP_END_PATTERN) {
#ifdef FW_RAM_DUMP_TO_FILE
			int j;
			for (j = 0; j < scn->ramdump[i]->length; j += 16) {
			        memset(str_buf, 0, sizeof(str_buf));
			        hex_dump_to_buffer(scn->ramdump[i]->mem + j, 16,
					           16, 4, str_buf, sizeof(str_buf), false);
			}
			pr_err("%s memory size = %d\n", fw_ram_seg_name[i -1],
			       (scn->ramdump[i])->length);
			if (i == FW_RAM_SEG_CNT) {
#else
			pr_err("%s memory size = %d\n", fw_ram_seg_name[i],
			       (scn->ramdump[i])->length);
			if (i == FW_RAM_SEG_CNT - 1) {
#endif
#if defined(FW_RAM_DUMP_TO_PROC)
				pr_err("F/W crash log dump completed\n");
#elif defined(FW_RAM_DUMP_TO_FILE)
				INIT_WORK(&scn->ramdump_usb_work, writefile_work);
				schedule_work(&scn->ramdump_usb_work);
				pr_err("F/W crash log dump completed\n");
#else
				VOS_BUG(0);
#endif
			}

			scn->ramdump_index++;
			scn->fw_ram_dumping = 0;
		}
	}
}
#endif

#define REGISTER_DUMP_LEN_MAX   60
#define REG_DUMP_COUNT		60

#if defined(CONFIG_CNSS) && defined(HIF_PCI)
static int __ol_target_failure(struct ol_softc *scn, void *wma_hdl)
{
	return 0;
}
#else
static int __ol_target_failure(struct ol_softc *scn, void *wma_hdl)
{
	unsigned int reg_dump_area = 0;
	unsigned int  reg_dump_cnt = 0;
	unsigned int reg_dump_values[REGISTER_DUMP_LEN_MAX];
	unsigned int i, dbglog_hdr_address;
	struct dbglog_hdr_host dbglog_hdr;
	struct dbglog_buf_host dbglog_buf;
	unsigned char *dbglog_data;
	tp_wma_handle wma = (tp_wma_handle) wma_hdl;
	unsigned int addr = host_interest_item_address(scn->target_type,
					offsetof(struct host_interest_s,
							hi_failure_state));

	if (HIFDiagReadMem(scn->hif_hdl, addr, (A_UCHAR *)&reg_dump_area,
						sizeof(A_UINT32)) != A_OK) {
		pr_err("%s FW Dump Area Pointer failed\n", __func__);
		return -ENOENT;
	}

	pr_info("%s Target Register Dump Location 0x%08X\n", __func__,
							reg_dump_area);

	reg_dump_cnt = REG_DUMP_COUNT;

	if (HIFDiagReadMem(scn->hif_hdl, reg_dump_area,
				(unsigned char *)&reg_dump_values[0],
				reg_dump_cnt * sizeof(A_UINT32)) != A_OK) {
		pr_err("%s FW Dump Area failed\n", __func__);
		return -ENOENT;
	}

	pr_info("%s Target Register Dump\n", __func__);
	for (i = 0; i < reg_dump_cnt; i++)
		pr_info("[%02d]   :  0x%08X\n", i, reg_dump_values[i]);

	if (!scn->enablefwlog) {
		pr_info("%s: FWLog is disabled in ini\n", __func__);
		return 0;
	}

	addr = host_interest_item_address(scn->target_type,
					  offsetof(struct host_interest_s,
							hi_dbglog_hdr));
	if (HIFDiagReadMem(scn->hif_hdl, addr,
				(unsigned char *)&dbglog_hdr_address,
				sizeof(dbglog_hdr_address)) != A_OK) {
		pr_err("%s FW dbglog_hdr_address failed\n", __func__);
		return -ENOENT;
	}

	if (HIFDiagReadMem(scn->hif_hdl, dbglog_hdr_address,
				(unsigned char *)&dbglog_hdr,
				sizeof(dbglog_hdr)) != A_OK) {
		pr_err("%s FW dbglog_hdr failed\n", __func__);
		return -ENOENT;
	}

	if (HIFDiagReadMem(scn->hif_hdl, (unsigned int)dbglog_hdr.dbuf,
						(unsigned char *)&dbglog_buf,
						sizeof(dbglog_buf)) != A_OK) {
		pr_err("%s FW dbglog_buf failed\n", __func__);
		return -ENOENT;
	}

	dbglog_data = adf_os_mem_alloc(scn->adf_dev,  dbglog_buf.length + 4);

	if (dbglog_data) {
		if (HIFDiagReadMem(scn->hif_hdl,
					(unsigned int)dbglog_buf.buffer,
					dbglog_data + 4,
					dbglog_buf.length) != A_OK)
			pr_err("%s FW dbglog_data failed\n", __func__);
		else {
			pr_info("%s dbglog_hdr.dbuf=%u, dbglog_data=%pK,"
				"dbglog_buf.buffer=%u, dbglog_buf.length=%u\n",
				__func__, dbglog_hdr.dbuf, dbglog_data,
				dbglog_buf.buffer, dbglog_buf.length);

			OS_MEMCPY(dbglog_data, &dbglog_hdr.dropped, 4);

			if (wma) {
				wma->is_fw_assert = 1;
				(void)dbglog_parse_debug_logs(wma, dbglog_data,
							dbglog_buf.length + 4);
			}
		}
		adf_os_mem_free(dbglog_data);
	}
	return 0;
}
#endif

void ol_target_failure(void *instance, A_STATUS status)
{
	struct ol_softc *scn = (struct ol_softc *)instance;
	void *vos_context = vos_get_global_context(VOS_MODULE_ID_WDA, NULL);
	tp_wma_handle wma = vos_get_context(VOS_MODULE_ID_WDA, vos_context);
#ifndef HIF_USB
	int ret;
#endif

#ifdef HIF_USB
	/* Currently, only firmware crash triggers ol_target_failure.
	   In case, we need to dump RAM data. */
	if (status == A_USB_ERROR) {
		ol_ramdump_handler(scn);
		return;
	}
#endif

	vos_event_set(&wma->recovery_event);

	if (OL_TRGET_STATUS_RESET == scn->target_status) {
		printk("Target is already asserted, ignore!\n");
		return;
	}

	scn->target_status = OL_TRGET_STATUS_RESET;

	if (vos_is_logp_in_progress(VOS_MODULE_ID_VOSS, NULL)) {
		pr_info("%s: LOGP is in progress, ignore!\n", __func__);
		return;
	}

#if defined(HIF_PCI) && defined(WLAN_DEBUG)
	if (vos_is_load_in_progress(VOS_MODULE_ID_VOSS, NULL)) {
		pr_err("XXX TARGET ASSERTED during driver loading XXX\n");

		if (hif_pci_check_soc_status(scn->hif_sc)
		    || dump_CE_register(scn)) {
			return;
		}

		dump_CE_debug_register(scn->hif_sc);
		ol_copy_ramdump(scn);
#ifndef CONFIG_NON_QC_PLATFORM_PCI
		VOS_BUG(0);
#endif
	}
#endif

	vos_set_logp_in_progress(VOS_MODULE_ID_VOSS, TRUE);
	if (vos_is_load_unload_in_progress(VOS_MODULE_ID_VOSS, NULL)) {
		printk("%s: Loading/Unloading is in progress, ignore!\n",
			__func__);
		vos_set_logp_in_progress(VOS_MODULE_ID_VOSS, FALSE);
#ifndef CONFIG_NON_QC_PLATFORM_PCI
		return;
#endif
	}

#ifdef HIF_PCI
	ret = hif_pci_check_fw_reg(scn->hif_sc);
	if (0 == ret) {
		if (scn->enable_self_recovery) {
			ol_schedule_fw_indication_work(scn);
			return;
		}
	} else if (-1 == ret) {
		return;
	}
#endif

#ifdef HIF_SDIO
	ret = hif_sdio_check_fw_reg(scn);
	if (0 == ret) {
		if (scn->enable_self_recovery) {
			ol_schedule_fw_indication_work(scn);
			return;
		}
	}
#endif

	printk("XXX TARGET ASSERTED XXX\n");

	if ((!scn->fastfwdump_host) || (!scn->fastfwdump_fw)) {
		if (__ol_target_failure(scn, wma))
			return;
	}

#if  defined(CONFIG_CNSS) || defined(HIF_SDIO) || \
defined(CONFIG_NON_QC_PLATFORM_PCI)
	vos_svc_fw_shutdown_ind(scn->adf_dev->dev);
	/* Collect the RAM dump through a workqueue */
	if (scn->enableRamdumpCollection)
		ol_schedule_ramdump_work(scn);
	else
		printk("%s: athdiag read for target reg\n", __func__);
#endif

	return;
}

int
ol_configure_target(struct ol_softc *scn)
{
	u_int32_t param;
#if defined(CONFIG_CNSS) && defined(HIF_PCI)
	struct cnss_platform_cap cap;
#endif

	/* Tell target which HTC version it is used*/
	param = HTC_PROTOCOL_VERSION;
	if (BMIWriteMemory(scn->hif_hdl,
				host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_app_host_interest)),
				(u_int8_t *)&param,
				4, scn)!= A_OK)
	{
		printk("BMIWriteMemory for htc version failed \n");
		return -1;
	}

	/* set the firmware mode to STA/IBSS/AP */
	{
		if (BMIReadMemory(scn->hif_hdl,
					host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_option_flag)),
					(A_UCHAR *)&param,
					4, scn)!= A_OK)
		{
			printk("BMIReadMemory for setting fwmode failed \n");
			return A_ERROR;
		}

		/* TODO following parameters need to be re-visited. */
		param |= (1 << HI_OPTION_NUM_DEV_SHIFT); //num_device
		param |= (HI_OPTION_FW_MODE_AP << HI_OPTION_FW_MODE_SHIFT); //Firmware mode ??
		param |= (1 << HI_OPTION_MAC_ADDR_METHOD_SHIFT); //mac_addr_method
		param |= (0 << HI_OPTION_FW_BRIDGE_SHIFT);  //firmware_bridge
		param |= (0 << HI_OPTION_FW_SUBMODE_SHIFT); //fwsubmode

		if (BMIWriteMemory(scn->hif_hdl,
					host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_option_flag)),
					(A_UCHAR *)&param,
					4, scn) != A_OK)
		{
			printk("BMIWriteMemory for setting fwmode failed \n");
			return A_ERROR;
		}
	}

#if defined(HIF_PCI)
#if (CONFIG_DISABLE_CDC_MAX_PERF_WAR)
	{
		/* set the firmware to disable CDC max perf WAR */
		if (BMIReadMemory(scn->hif_hdl,
					host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_option_flag2)),
					(A_UCHAR *)&param,
					4, scn)!= A_OK)
		{
			printk("BMIReadMemory for setting cdc max perf failed \n");
			return A_ERROR;
		}

		param |= HI_OPTION_DISABLE_CDC_MAX_PERF_WAR;
		if (BMIWriteMemory(scn->hif_hdl,
					host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_option_flag2)),
					(A_UCHAR *)&param,
					4, scn) != A_OK)
		{
			printk("BMIWriteMemory for setting cdc max perf failed \n");
			return A_ERROR;
		}
	}
#endif /* CONFIG_CDC_MAX_PERF_WAR */

#endif /*HIF_PCI*/

#if defined(CONFIG_CNSS) && defined(HIF_PCI)
	{
		int ret;

		ret = vos_get_platform_cap(&cap);
		if (ret)
			pr_err("platform capability info from CNSS not available\n");

		if (!ret && cap.cap_flag & CNSS_HAS_EXTERNAL_SWREG) {
			if (BMIReadMemory(scn->hif_hdl,
				host_interest_item_address(scn->target_type,
					offsetof(struct host_interest_s, hi_option_flag2)),
						(A_UCHAR *)&param, 4, scn)!= A_OK) {
				printk("BMIReadMemory for setting external SWREG failed\n");
				return A_ERROR;
			}

			param |= HI_OPTION_USE_EXT_LDO;
			if (BMIWriteMemory(scn->hif_hdl,
				host_interest_item_address(scn->target_type,
					offsetof(struct host_interest_s, hi_option_flag2)),
						(A_UCHAR *)&param, 4, scn) != A_OK) {
				printk("BMIWriteMemory for setting external SWREG failed\n");
				return A_ERROR;
			}
		}
	}
#endif

#ifdef WLAN_FEATURE_LPSS
	if (scn->enablelpasssupport) {
		if (BMIReadMemory(scn->hif_hdl,
			  host_interest_item_address(scn->target_type,
			     offsetof(struct host_interest_s, hi_option_flag2)),
				  (A_UCHAR *)&param, 4, scn)!= A_OK) {
			printk("BMIReadMemory for setting LPASS Support failed\n");
			return A_ERROR;
		}

		param |= HI_OPTION_DBUART_SUPPORT;
		if (BMIWriteMemory(scn->hif_hdl,
			   host_interest_item_address(scn->target_type,
			      offsetof(struct host_interest_s, hi_option_flag2)),
				   (A_UCHAR *)&param, 4, scn) != A_OK) {
			printk("BMIWriteMemory for setting LPASS Support failed\n");
			return A_ERROR;
		}
	}
#endif

	/* If host is running on a BE CPU, set the host interest area */
	{
#ifdef BIG_ENDIAN_HOST
		param = 1;
#else
		param = 0;
#endif
		if (BMIWriteMemory(scn->hif_hdl,
					host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_be)),
					(A_UCHAR *)&param,
					4, scn) != A_OK)
		{
			printk("BMIWriteMemory for setting host CPU BE mode failed \n");
			return A_ERROR;
		}
	}

	/* FW descriptor/Data swap flags */
	{
		param = 0;
		if (BMIWriteMemory(scn->hif_hdl,
					host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_fw_swap)),
					(A_UCHAR *)&param,
					4, scn) != A_OK)
		{
			printk("BMIWriteMemory for setting FW data/desc swap flags failed \n");
			return A_ERROR;
		}
	}

#ifdef HIF_SDIO
	if(scn->fastfwdump_host) {
		if (BMIReadMemory(scn->hif_hdl,
			  host_interest_item_address(scn->target_type,
			     offsetof(struct host_interest_s, hi_option_flag2)),
				  (A_UCHAR *)&param, 4, scn)!= A_OK) {
			printk("BMIReadMemory for setting LPASS Support failed\n");
			return A_ERROR;
		}

		param |= HI_OPTION_SDIO_CRASH_DUMP_ENHANCEMENT_HOST;
		if (BMIWriteMemory(scn->hif_hdl,
			   host_interest_item_address(scn->target_type,
			      offsetof(struct host_interest_s, hi_option_flag2)),
				   (A_UCHAR *)&param, 4, scn) != A_OK) {
			printk("BMIWriteMemory for setting LPASS Support failed\n");
			return A_ERROR;
		}
	}
#endif

	return A_OK;
}

static int
ol_check_dataset_patch(struct ol_softc *scn, u_int32_t *address)
{
	/* Check if patch file needed for this target type/version. */
	return 0;
}

#if defined(HIF_PCI) || defined(HIF_SDIO)

A_STATUS ol_fw_populate_clk_settings(A_refclk_speed_t refclk,
				struct cmnos_clock_s *clock_s)
{
	if (!clock_s)
		return A_ERROR;

	switch (refclk) {
	case SOC_REFCLK_48_MHZ:
		clock_s->wlan_pll.div = 0xE;
		clock_s->wlan_pll.rnfrac = 0x2AAA8;
		clock_s->pll_settling_time = 2400;
		break;
	case SOC_REFCLK_19_2_MHZ:
		clock_s->wlan_pll.div = 0x24;
		clock_s->wlan_pll.rnfrac = 0x2AAA8;
		clock_s->pll_settling_time = 960;
		break;
	case SOC_REFCLK_24_MHZ:
		clock_s->wlan_pll.div = 0x1D;
		clock_s->wlan_pll.rnfrac = 0x15551;
		clock_s->pll_settling_time = 1200;
		break;
	case SOC_REFCLK_26_MHZ:
		clock_s->wlan_pll.div = 0x1B;
		clock_s->wlan_pll.rnfrac = 0x4EC4;
		clock_s->pll_settling_time = 1300;
		break;
	case SOC_REFCLK_37_4_MHZ:
		clock_s->wlan_pll.div = 0x12;
		clock_s->wlan_pll.rnfrac = 0x34B49;
		clock_s->pll_settling_time = 1870;
		break;
	case SOC_REFCLK_38_4_MHZ:
		clock_s->wlan_pll.div = 0x12;
		clock_s->wlan_pll.rnfrac = 0x15551;
		clock_s->pll_settling_time = 1920;
		break;
	case SOC_REFCLK_40_MHZ:
		clock_s->wlan_pll.div = 0x11;
		clock_s->wlan_pll.rnfrac = 0x26665;
		clock_s->pll_settling_time = 2000;
		break;
	case SOC_REFCLK_52_MHZ:
		clock_s->wlan_pll.div = 0x1B;
		clock_s->wlan_pll.rnfrac = 0x4EC4;
		clock_s->pll_settling_time = 2600;
		break;
	case SOC_REFCLK_UNKNOWN:
		clock_s->wlan_pll.refdiv = 0;
		clock_s->wlan_pll.div = 0;
		clock_s->wlan_pll.rnfrac = 0;
		clock_s->wlan_pll.outdiv = 0;
		clock_s->pll_settling_time = 1024;
		clock_s->refclk_hz = 0;
	default:
		return A_ERROR;
	}

	clock_s->refclk_hz = refclk_speed_to_hz[refclk];
	clock_s->wlan_pll.refdiv = 0;
	clock_s->wlan_pll.outdiv = 1;

	return A_OK;
}

A_STATUS ol_patch_pll_switch(struct ol_softc * scn)
{
	HIF_DEVICE *hif_device = scn->hif_hdl;
	A_STATUS status;
	u_int32_t addr = 0;
	u_int32_t reg_val = 0;
	u_int32_t mem_val = 0;
	struct cmnos_clock_s clock_s;
	u_int32_t cmnos_core_clk_div_addr = 0;
	u_int32_t cmnos_cpu_pll_init_done_addr = 0;
	u_int32_t cmnos_cpu_speed_addr = 0;
#ifdef HIF_USB/* fail for USB case */
	struct hif_usb_softc *sc = scn->hif_sc;
#elif defined HIF_PCI
	struct hif_pci_softc *sc = scn->hif_sc;
#else
    struct ath_hif_sdio_softc *sc = scn->hif_sc;
#endif

	switch (scn->target_version) {
	case AR6320_REV1_1_VERSION:
		cmnos_core_clk_div_addr = AR6320_CORE_CLK_DIV_ADDR;
		cmnos_cpu_pll_init_done_addr = AR6320_CPU_PLL_INIT_DONE_ADDR;
		cmnos_cpu_speed_addr = AR6320_CPU_SPEED_ADDR;
		break;
	case AR6320_REV1_3_VERSION:
	case AR6320_REV2_1_VERSION:
		cmnos_core_clk_div_addr = AR6320V2_CORE_CLK_DIV_ADDR;
		cmnos_cpu_pll_init_done_addr = AR6320V2_CPU_PLL_INIT_DONE_ADDR;
		cmnos_cpu_speed_addr = AR6320V2_CPU_SPEED_ADDR;
		break;
	case AR6320_REV3_VERSION:
	case AR6320_REV3_2_VERSION:
	case QCA9377_REV1_1_VERSION:
	case QCA9379_REV1_VERSION:
		cmnos_core_clk_div_addr = AR6320V3_CORE_CLK_DIV_ADDR;
		cmnos_cpu_pll_init_done_addr = AR6320V3_CPU_PLL_INIT_DONE_ADDR;
		cmnos_cpu_speed_addr = AR6320V3_CPU_SPEED_ADDR;
		break;
	default:
		pr_err("%s: Unsupported target version %x\n", __func__,
		       scn->target_version);
		return A_ERROR;
	}

	addr = (RTC_SOC_BASE_ADDRESS | EFUSE_OFFSET);
	status = BMIReadSOCRegister(hif_device, addr, &reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to read EFUSE Addr\n");
		return status;
	}

	status = ol_fw_populate_clk_settings(EFUSE_XTAL_SEL_GET(reg_val),
					&clock_s);
	if (status != A_OK) {
		pr_err("Failed to set clock settings\n");
		return status;
	}
	pr_debug("crystal_freq: %dHz\n", clock_s.refclk_hz);

	/* ------Step 1----*/
	reg_val = 0;
	addr = (RTC_SOC_BASE_ADDRESS | BB_PLL_CONFIG_OFFSET);
	status = BMIReadSOCRegister(hif_device, addr, &reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to read PLL_CONFIG Addr\n");
		return status;
	}
	pr_debug("Step 1a: %8X\n", reg_val);

	reg_val &= ~(BB_PLL_CONFIG_FRAC_MASK | BB_PLL_CONFIG_OUTDIV_MASK);
	reg_val |= (BB_PLL_CONFIG_FRAC_SET(clock_s.wlan_pll.rnfrac) |
			BB_PLL_CONFIG_OUTDIV_SET(clock_s.wlan_pll.outdiv));
	status = BMIWriteSOCRegister(hif_device, addr, reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to write PLL_CONFIG Addr\n");
		return status;
	}

	reg_val = 0;
	status = BMIReadSOCRegister(hif_device, addr, &reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to read back PLL_CONFIG Addr\n");
		return status;
	}
	pr_debug("Step 1b: %8X\n", reg_val);

	/* ------Step 2----*/
	reg_val = 0;
	addr = (RTC_WMAC_BASE_ADDRESS | WLAN_PLL_SETTLE_OFFSET);
	status = BMIReadSOCRegister(hif_device, addr, &reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to read PLL_SETTLE Addr\n");
		return status;
	}
	pr_debug("Step 2a: %8X\n", reg_val);

	reg_val &= ~WLAN_PLL_SETTLE_TIME_MASK;
	reg_val |= WLAN_PLL_SETTLE_TIME_SET(clock_s.pll_settling_time);
	status = BMIWriteSOCRegister(hif_device, addr, reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to write PLL_SETTLE Addr\n");
		return status;
	}

	reg_val = 0;
	status = BMIReadSOCRegister(hif_device, addr, &reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to read back PLL_SETTLE Addr\n");
		return status;
	}
	pr_debug("Step 2b: %8X\n", reg_val);

	/* ------Step 3----*/
	reg_val = 0;
	addr = (RTC_SOC_BASE_ADDRESS | SOC_CORE_CLK_CTRL_OFFSET);
	status = BMIReadSOCRegister(hif_device, addr, &reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to read CLK_CTRL Addr\n");
		return status;
	}
	pr_debug("Step 3a: %8X\n", reg_val);

	reg_val &= ~SOC_CORE_CLK_CTRL_DIV_MASK;
	reg_val |= SOC_CORE_CLK_CTRL_DIV_SET(1);
	status = BMIWriteSOCRegister(hif_device, addr, reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to write CLK_CTRL Addr\n");
		return status;
	}

	reg_val = 0;
	status = BMIReadSOCRegister(hif_device, addr, &reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to read back CLK_CTRL Addr\n");
		return status;
	}
	pr_debug("Step 3b: %8X\n", reg_val);

	/* ------Step 4-----*/
	mem_val = 1;
	status = BMIWriteMemory(hif_device, cmnos_core_clk_div_addr,
			(A_UCHAR *)&mem_val, 4, scn);
	if (status != A_OK) {
		pr_err("Failed to write CLK_DIV Addr\n");
		return status;
	}

	/* ------Step 5-----*/
	reg_val = 0;
	addr = (RTC_WMAC_BASE_ADDRESS | WLAN_PLL_CONTROL_OFFSET);
	status = BMIReadSOCRegister(hif_device, addr, &reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to read PLL_CTRL Addr\n");
		return status;
	}
	pr_debug("Step 5a: %8X\n", reg_val);

	reg_val &= ~(WLAN_PLL_CONTROL_REFDIV_MASK | WLAN_PLL_CONTROL_DIV_MASK |
			WLAN_PLL_CONTROL_NOPWD_MASK);
	reg_val |=  (WLAN_PLL_CONTROL_REFDIV_SET(clock_s.wlan_pll.refdiv) |
			WLAN_PLL_CONTROL_DIV_SET(clock_s.wlan_pll.div) |
			WLAN_PLL_CONTROL_NOPWD_SET(1));
	status = BMIWriteSOCRegister(hif_device, addr, reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to write PLL_CTRL Addr\n");
		return status;
	}

	reg_val = 0;
	status = BMIReadSOCRegister(hif_device, addr, &reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to read back PLL_CTRL Addr\n");
		return status;
	}
	OS_DELAY(100);
	pr_debug("Step 5b: %8X\n", reg_val);

	/* ------Step 6-------*/
	do {
		reg_val = 0;
		status = BMIReadSOCRegister(hif_device, (RTC_WMAC_BASE_ADDRESS |
				RTC_SYNC_STATUS_OFFSET), &reg_val, scn);
		if (status != A_OK) {
			pr_err("Failed to read RTC_SYNC_STATUS Addr\n");
			return status;
		}
	} while(RTC_SYNC_STATUS_PLL_CHANGING_GET(reg_val));

	/* ------Step 7-------*/
	reg_val = 0;
	addr = (RTC_WMAC_BASE_ADDRESS | WLAN_PLL_CONTROL_OFFSET);
	status = BMIReadSOCRegister(hif_device, addr, &reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to read PLL_CTRL Addr for CTRL_BYPASS\n");
		return status;
	}
	pr_debug("Step 7a: %8X\n", reg_val);

	reg_val &= ~WLAN_PLL_CONTROL_BYPASS_MASK;
	reg_val |= WLAN_PLL_CONTROL_BYPASS_SET(0);
	status = BMIWriteSOCRegister(hif_device, addr, reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to write PLL_CTRL Addr for CTRL_BYPASS\n");
		return status;
	}

	reg_val = 0;
	status = BMIReadSOCRegister(hif_device, addr, &reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to read back PLL_CTRL Addr for CTRL_BYPASS\n");
		return status;
	}
	pr_debug("Step 7b: %8X\n", reg_val);

	/* ------Step 8--------*/
	do {
		reg_val = 0;
		status = BMIReadSOCRegister(hif_device,
			(RTC_WMAC_BASE_ADDRESS | RTC_SYNC_STATUS_OFFSET),
			&reg_val, scn);
		if (status != A_OK) {
			pr_err("Failed to read SYNC_STATUS Addr\n");
			return status;
		}
	} while(RTC_SYNC_STATUS_PLL_CHANGING_GET(reg_val));

	/* ------Step 9--------*/
	reg_val = 0;
	addr = (RTC_SOC_BASE_ADDRESS | SOC_CPU_CLOCK_OFFSET);
	status = BMIReadSOCRegister(hif_device, addr, &reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to read CPU_CLK Addr\n");
		return status;
	}
	pr_debug("Step 9a: %8X\n", reg_val);

	reg_val &= ~SOC_CPU_CLOCK_STANDARD_MASK;
	reg_val |= SOC_CPU_CLOCK_STANDARD_SET(1);;
	status = BMIWriteSOCRegister(hif_device, addr, reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to write CPU_CLK Addr\n");
		return status;
	}

	reg_val = 0;
	status = BMIReadSOCRegister(hif_device, addr, &reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to read back CPU_CLK Addr\n");
		return status;
	}
	pr_debug("Step 9b: %8X\n", reg_val);

	/* ------Step 10-------*/
	reg_val = 0;
	addr = (RTC_WMAC_BASE_ADDRESS | WLAN_PLL_CONTROL_OFFSET);
	status = BMIReadSOCRegister(hif_device, addr, &reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to read PLL_CTRL Addr for NOPWD\n");
		return status;
	}
	pr_debug("Step 10a: %8X\n", reg_val);

	reg_val &= ~WLAN_PLL_CONTROL_NOPWD_MASK;
	status = BMIWriteSOCRegister(hif_device, addr, reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to write PLL_CTRL Addr for NOPWD\n");
		return status;
	}

	reg_val = 0;
	status = BMIReadSOCRegister(hif_device, addr, &reg_val, scn);
	if (status != A_OK) {
		pr_err("Failed to read back PLL_CTRL Addr for NOPWD\n");
		return status;
	}
	pr_debug("Step 10b: %8X\n", reg_val);

	/* ------Step 11-------*/
	mem_val = 1;
	status = BMIWriteMemory(hif_device, cmnos_cpu_pll_init_done_addr,
			(A_UCHAR *)&mem_val, 4, scn);
	if (status != A_OK) {
		pr_err("Failed to write PLL_INIT Addr\n");
		return status;
	}

	mem_val = TARGET_CPU_FREQ;
	status = BMIWriteMemory(hif_device, cmnos_cpu_speed_addr,
			(A_UCHAR *)&mem_val, 4, scn);
	if (status != A_OK) {
		pr_err("Failed to write CPU_SPEED Addr\n");
		return status;
	}

	return status;
}
#endif

#ifdef HIF_PCI
#ifndef CONFIG_NON_QC_PLATFORM_PCI
/* AXI Start Address */
#define TARGET_ADDR (0xa0000)

void ol_transfer_codeswap_struct(struct ol_softc *scn) {
	struct hif_pci_softc *sc = scn->hif_sc;
	struct codeswap_codeseg_info wlan_codeswap;
	A_STATUS rv;

	if (!sc || !sc->hif_device) {
		pr_err("%s: hif_pci_softc is null\n", __func__);
		return;
	}
	if (cnss_get_codeswap_struct(&wlan_codeswap)) {
		pr_err("%s: failed to get codeswap structure\n", __func__);
		return;
	}
	rv = BMIWriteMemory(scn->hif_hdl, TARGET_ADDR,
		(u_int8_t *)&wlan_codeswap, sizeof(wlan_codeswap), scn);

	if (rv != A_OK) {
		pr_err("Failed to Write 0xa0000 for Target Memory Expansion\n");
		return;
	}
	pr_info("%s:codeswap structure is successfully downloaded\n", __func__);
}
#endif
#endif

#ifdef FEATURE_USB_WARM_RESET
static inline int ol_download_usb_warm_reset_firmeware(struct ol_softc *scn)
{
	uint32_t warm_reset_load_status = 0;
	uint32_t address = BMI_SEGMENTED_WRITE_ADDR;

	if (scn->enable_usb_warm_reset) {
		BMIReadMemory(scn->hif_hdl,
			      host_interest_item_address(scn->target_type,
				offsetof(struct host_interest_s, hi_minidump)),
			      (uint8_t *)&warm_reset_load_status,
			      sizeof(warm_reset_load_status), scn);

		if (!warm_reset_load_status) {
			/* first load, download warm_reset.bin */
			if (ol_transfer_bin_file(scn, ATH_USB_WARM_RESET_FILE,
						 address, TRUE) != EOK)
				return -1;
			/*
			 * FW will update the hi_minidump
			 * after recv HIFDiagWriteWARMRESET
			 */
			/* fall-thru */
		}
	}

	return EOK;
}
#else
static inline int ol_download_usb_warm_reset_firmeware(struct ol_softc *scn)
{
	return EOK;
}
#endif

int ol_download_firmware(struct ol_softc *scn)
{
	uint32_t param, address = 0;
	uint8_t bdf_ret = 0;
	int status = !EOK;
#if defined(HIF_PCI) || defined(HIF_SDIO)
	A_STATUS ret;
#endif

#if defined(CONFIG_NON_QC_PLATFORM_PCI)
		if (0 != get_fw_files_for_non_qc_pci_target(&scn->fw_files,
						scn->target_type,
						scn->target_version)) {
			printk("%s: No FW files from CNSS driver\n", __func__);
			return -1;
		}
#elif defined(HIF_PCI)
		if (0 != cnss_get_fw_files_for_target(&scn->fw_files,
						scn->target_type,
						scn->target_version)) {
			printk("%s: No FW files from CNSS driver\n", __func__);
			return -1;
		}
#elif defined(HIF_SDIO)
       if (0 != ol_get_fw_files_for_target(&scn->fw_files,
                                              scn->target_version)) {
                printk("%s: No FW files from driver\n", __func__);
                return -1;
       }
#endif
	/* Transfer Board Data from Target EEPROM to Target RAM */
	/* Determine where in Target RAM to write Board Data */
	BMIReadMemory(scn->hif_hdl,
			host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_board_data)),
			(u_int8_t *)&address, 4, scn);

	if (!address) {
		address = AR6004_REV5_BOARD_DATA_ADDRESS;
		printk("%s: Target address not known! Using 0x%x\n", __func__, address);
	}

#if defined(HIF_PCI) || defined(HIF_SDIO)
	ret = ol_patch_pll_switch(scn);
	if (ret) {
		pr_err("pll switch failed. status %d\n", ret);
		return -1;
	}
#endif

	if (scn->cal_in_flash) {
		/* Write EEPROM or Flash data to Target RAM */
		status = ol_transfer_bin_file(scn, ATH_FLASH_FILE, address, FALSE);
	}

	if (status == EOK) {
		/* Record the fact that Board Data is initialized */
		param = 1;
		BMIWriteMemory(scn->hif_hdl,
				host_interest_item_address(scn->target_type,
					offsetof(struct host_interest_s, hi_board_data_initialized)),
				(u_int8_t *)&param, 4, scn);
	} else {
		/* Transfer One Time Programmable data */
		address = BMI_SEGMENTED_WRITE_ADDR;

		if ( scn->enablesinglebinary == FALSE ) {
#ifdef HIF_PCI
#ifndef CONFIG_NON_QC_PLATFORM_PCI
			ol_transfer_codeswap_struct(scn);
#endif
#endif

			status = ol_transfer_bin_file(scn, ATH_OTP_FILE,
						      address, TRUE);
			if (status == EOK) {
				/* Execute the OTP code only if entry found and downloaded */
				param = 0x10;
				BMIExecute(scn->hif_hdl, address, &param, scn);
				bdf_ret = param & 0xff;
				if (!bdf_ret)
					scn->board_id = (param >> 8) & 0xffff;
				pr_err("%s: chip_id:0x%0x board_id:0x%0x\n",
						__func__, scn->target_version,
							scn->board_id);
			} else if (status < 0) {
				return status;
			}
		}

		BMIReadMemory(scn->hif_hdl,
			host_interest_item_address(scn->target_type,
				offsetof(struct host_interest_s,
						hi_board_data)),
				(u_int8_t *)&address, 4, scn);

		if (!address) {
			address = AR6004_REV5_BOARD_DATA_ADDRESS;
			pr_err("%s: Target address not known! Using 0x%x\n",
							__func__, address);
		}

		/* Flash is either not available or invalid */
		if (ol_transfer_bin_file(scn, ATH_BOARD_DATA_FILE,
					address, FALSE) != EOK) {
			pr_err("%s: Board Data Download Failed\n", __func__);
			return -1;
		}

		/* Record the fact that Board Data is initialized */
		param = 1;
		BMIWriteMemory(scn->hif_hdl,
				host_interest_item_address(scn->target_type,
					offsetof(struct host_interest_s,
						hi_board_data_initialized)),
				(u_int8_t *)&param, 4, scn);

		address = BMI_SEGMENTED_WRITE_ADDR;
		param = 0x0;
		BMIExecute(scn->hif_hdl, address, &param, scn);
	}

	if (scn->target_version == AR6320_REV1_1_VERSION){
		/* To disable PCIe use 96 AXI memory as internal buffering,
		 *  highest bit of PCIE_TXBUF_ADDRESS need be set as 1
		 */
		u_int32_t addr = 0x3A058; /* PCIE_TXBUF_ADDRESS */
		u_int32_t value = 0;
		/* Disable PCIe AXI memory */
		BMIReadMemory(scn->hif_hdl, addr, (A_UCHAR*)&value, 4, scn);
		value |= 0x80000000; /* PCIE_TXBUF_BYPASS_SET(1) */
		BMIWriteMemory(scn->hif_hdl, addr, (A_UCHAR*)&value, 4, scn);
		value = 0;
		BMIReadMemory(scn->hif_hdl, addr, (A_UCHAR*)&value, 4, scn);
		printk("Disable PCIe use AXI memory:0x%08X-0x%08X\n", addr, value);
	}

	address = BMI_SEGMENTED_WRITE_ADDR;
	if (scn->enablesinglebinary == FALSE) {
		if (ol_transfer_bin_file(scn, ATH_SETUP_FILE,
					BMI_SEGMENTED_WRITE_ADDR, TRUE) == EOK) {
			/* Execute the SETUP code only if entry found and downloaded */
			param = 0;
			BMIExecute(scn->hif_hdl, address, &param, scn);
		}
	}

	if (ol_download_usb_warm_reset_firmeware(scn) != EOK)
		return -1;

	/* Download Target firmware - TODO point to target specific files in runtime */
	if (ol_transfer_bin_file(scn, ATH_FIRMWARE_FILE, address, TRUE) != EOK) {
		return -1;
	}

	/* Apply the patches */
	if (ol_check_dataset_patch(scn, &address))
	{
		if ((ol_transfer_bin_file(scn, ATH_PATCH_FILE, address, FALSE)) != EOK) {
			return -1;
		}
		BMIWriteMemory(scn->hif_hdl,
				host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_dset_list_head)),
				(u_int8_t *)&address, 4, scn);
	}

	if (scn->enableuartprint ||
		(WLAN_IS_EPPING_ENABLED(vos_get_conparam()) &&
		WLAN_IS_EPPING_FW_UART(vos_get_conparam()))) {
		switch (scn->target_version){
			case AR6004_VERSION_REV1_3:
				param = 11;
				break;
			case AR6320_REV1_VERSION:
			case AR6320_REV2_VERSION:
			case AR6320_REV3_VERSION:
			case AR6320_REV3_2_VERSION:
			case QCA9377_REV1_1_VERSION:
			case AR6320_REV4_VERSION:
			case AR6320_DEV_VERSION:
			/* for SDIO, debug uart output gpio is 29, otherwise it is 6. */
#ifdef HIF_SDIO
				param = 19;
#else
				param = 6;
#endif
				break;
			case QCA9379_REV1_VERSION:
#if defined(HIF_SDIO) || defined(HIF_USB)
				param = 19;
#else
				param = 6;
#endif
				break;
			default:
			/* Configure GPIO AR9888 UART */
				param = 7;
			}

		BMIWriteMemory(scn->hif_hdl,
				host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_dbg_uart_txpin)),
				(u_int8_t *)&param, 4, scn);
		param = 1;
		BMIWriteMemory(scn->hif_hdl,
				host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_serial_enable)),
				(u_int8_t *)&param, 4, scn);
	} else {
		/*
		 * Explicitly setting UART prints to zero as target turns it on
		 * based on scratch registers.
		 */
		param = 0;
		BMIWriteMemory(scn->hif_hdl,
				host_interest_item_address(scn->target_type, offsetof(struct host_interest_s,hi_serial_enable)),
				(u_int8_t *)&param, 4, scn);
	}

#ifdef HIF_SDIO
	/* HACK override dbg TX pin to avoid side effects of default GPIO_6 */
	param = 19;
	BMIWriteMemory(scn->hif_hdl,
		host_interest_item_address(scn->target_type,
		offsetof(struct host_interest_s,
		hi_dbg_uart_txpin)),
		(u_int8_t *)&param, 4, scn);
#endif


	if (scn->enablefwlog) {
		BMIReadMemory(scn->hif_hdl,
				host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_option_flag)),
				(u_int8_t *)&param, 4, scn);

		param &= ~(HI_OPTION_DISABLE_DBGLOG);
		BMIWriteMemory(scn->hif_hdl,
				host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_option_flag)),
				(u_int8_t *)&param, 4, scn);
	} else {
		/*
		 * Explicitly setting fwlog prints to zero as target turns it on
		 * based on scratch registers.
		 */
		BMIReadMemory(scn->hif_hdl,
				host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_option_flag)),
				(u_int8_t *)&param, 4, scn);

		param |= HI_OPTION_DISABLE_DBGLOG;
		BMIWriteMemory(scn->hif_hdl,
				host_interest_item_address(scn->target_type, offsetof(struct host_interest_s, hi_option_flag)),
				(u_int8_t *)&param, 4, scn);
	}

#ifdef HIF_SDIO
	status = ol_sdio_extra_initialization(scn);
#elif defined(HIF_USB)
	status = ol_usb_extra_initialization(scn);
#endif

	return status;
}

#if defined(HIF_PCI) || defined(HIF_SDIO)
int ol_diag_read(struct ol_softc *scn, u_int8_t *buffer,
	u_int32_t pos, size_t count)
{
	int result = 0;

	if ((4 == count) && ((pos & 3) == 0)) {
		result = HIFDiagReadAccess(scn->hif_hdl, pos,
			(u_int32_t*)buffer);
	} else {
#ifdef HIF_PCI
		size_t amountRead = 0;
		size_t readSize = PCIE_READ_LIMIT;
		size_t remainder = 0;
		if (count > PCIE_READ_LIMIT) {
			while ((amountRead < count) && (0 == result)) {
				result = HIFDiagReadMem(scn->hif_hdl, pos,
					buffer, readSize);
				if (0 == result) {
					buffer += readSize;
					pos += readSize;
					amountRead += readSize;
					remainder = count - amountRead;
					if (remainder < PCIE_READ_LIMIT)
						readSize = remainder;
				}
			}
		} else {
#endif
			result = HIFDiagReadMem(scn->hif_hdl, pos,
					buffer, count);
#ifdef HIF_PCI
		}
#endif
	}

	if (!result) {
		return count;
	} else {
		return -EIO;
	}
}

#ifdef HIF_PCI
static int ol_ath_get_reg_table(A_UINT32 target_version,
				tgt_reg_table *reg_table)
{
	int section_len = 0;

	if (!reg_table) {
		ASSERT(0);
		return section_len;
	}

	switch (target_version) {
	case AR6320_REV2_1_VERSION:
		reg_table->section = (tgt_reg_section *)&ar6320v2_reg_table[0];
		reg_table->section_size = sizeof(ar6320v2_reg_table)
					 /sizeof(ar6320v2_reg_table[0]);
		section_len = AR6320_REV2_1_REG_SIZE;
		break;
	case AR6320_REV3_VERSION:
	case AR6320_REV3_2_VERSION:
	case QCA9377_REV1_1_VERSION:
	case QCA9379_REV1_VERSION:
		reg_table->section = (tgt_reg_section *)&ar6320v3_reg_table[0];
		reg_table->section_size = sizeof(ar6320v3_reg_table)
					/sizeof(ar6320v3_reg_table[0]);
		section_len = AR6320_REV3_REG_SIZE;
		break;
	default:
		reg_table->section = (void *)NULL;
		reg_table->section_size = 0;
		section_len = 0;
	}

	return section_len;
}
#elif defined(HIF_SDIO)
static int ol_ath_get_reg_table(uint32_t target_version,
				tgt_reg_table *reg_table)
{
	int len = 0;

	if (!reg_table) {
		ASSERT(0);
		return len;
	}

	switch (target_version) {
	case AR6320_REV3_VERSION:
	case AR6320_REV3_2_VERSION:
	case QCA9377_REV1_1_VERSION:
		reg_table->section = (tgt_reg_section *)&ar6320v3_reg_table[0];
		reg_table->section_size = sizeof(ar6320v3_reg_table)/
			sizeof(ar6320v3_reg_table[0]);
		len = AR6320_REV3_REG_SIZE;
		break;
	default:
		reg_table->section = (void *)NULL;
		reg_table->section_size = 0;
		len = 0;
		break;
	}

	return len;
}
#endif

static int ol_diag_read_reg_loc(struct ol_softc *scn, u_int8_t *buffer,
		u_int32_t buffer_len)
{
	int i, len, section_len, fill_len;
	int dump_len, result = 0;
	tgt_reg_table reg_table;
	tgt_reg_section *curr_sec, *next_sec;

	section_len = ol_ath_get_reg_table(scn->target_version, &reg_table);

	if (!reg_table.section || !reg_table.section_size || !section_len) {
		printk(KERN_ERR "%s: failed to get reg table\n", __func__);
		result = -EIO;
		goto out;
	}

	curr_sec = reg_table.section;
	for (i = 0; i < reg_table.section_size; i++) {

		dump_len = curr_sec->end_addr - curr_sec->start_addr;

		if ((buffer_len - result) < dump_len) {
			printk("Not enough memory to dump the registers:"
					" %d: 0x%08x-0x%08x\n", i,
					curr_sec->start_addr,
					curr_sec->end_addr);
			goto out;
		}

		len = ol_diag_read(scn, buffer, curr_sec->start_addr, dump_len);

		if (len != -EIO) {
			buffer += len;
			result += len;
		} else {
			printk(KERN_ERR "%s: can't read reg 0x%08x len = %d\n",
			       __func__, curr_sec->start_addr, dump_len);
			result = -EIO;
			goto out;
		}

		if (result < section_len) {
			next_sec = (tgt_reg_section *)((u_int8_t *)curr_sec
							+ sizeof(*curr_sec));
			fill_len = next_sec->start_addr - curr_sec->end_addr;
			if ((buffer_len - result) < fill_len) {
				printk("Not enough memory to fill registers:"
						" %d: 0x%08x-0x%08x\n", i,
						curr_sec->end_addr,
						next_sec->start_addr);
				goto out;
			}

			if (fill_len) {
				buffer += fill_len;
				result += fill_len;
			}
		}
		curr_sec++;
	}

out:
	return result;
}

#ifdef HIF_PCI
static void ol_dump_target_memory(HIF_DEVICE *hif_device, void *memoryBlock)
{
	char *bufferLoc = memoryBlock;
	u_int32_t sectionCount = 0;
	u_int32_t address = 0;
	u_int32_t size = 0;

	for ( ; sectionCount < 2; sectionCount++) {
		switch (sectionCount) {
		case 0:
			address = DRAM_LOCAL_BASE_ADDRESS;
			size = DRAM_SIZE;
			break;
		case 1:
			address = AXI_LOCATION;
			size = AXI_SIZE;
		default:
			break;
		}

		HIFDumpTargetMemory(hif_device, bufferLoc, address, size);
		bufferLoc += size;
	}
}

static int ol_get_iram1_len_and_pos(struct ol_softc *scn, uint32_t *pos,
				     uint32_t *len)
{
	int status = scn->target_status;
	int ret = hif_pci_set_ram_config_reg(scn->hif_sc, IRAM1_LOCATION >> 20);

	if ((status != OL_TRGET_STATUS_RESET) || ret) {
		pr_debug("%s: Skip IRAM1 Section; Target Status:%d; ret:%d\n",
			 __func__, status, ret);
		return -EBUSY;
	}

	*pos = IRAM1_LOCATION;
	*len = IRAM1_SIZE;

	return 0;
}

static int ol_get_iram2_len_and_pos(struct ol_softc *scn, uint32_t *pos,
				    uint32_t *len)
{
	int ret = hif_pci_set_ram_config_reg(scn->hif_sc, IRAM2_LOCATION >> 20);

	if (ret) {
		pr_debug("Skipping IRAM2 Section; ret:%d\n", ret);
		return -EBUSY;
	}

	*pos = IRAM2_LOCATION;
	*len = IRAM2_SIZE;

	return 0;
}

static int ol_get_iram_len_and_pos(struct ol_softc *scn, uint32_t *pos, uint32_t
				   *len, uint32_t section)
{
	switch (section) {
	case 3:
		pr_info("%s: Dumping IRAM1 section\n", __func__);
		return ol_get_iram1_len_and_pos(scn, pos, len);
	case 4:
		pr_info("%s: Dumping IRAM2 section\n", __func__);
		return ol_get_iram2_len_and_pos(scn, pos, len);
	default:
		pr_err("%s: Invalid Arguments\n", __func__);
		return -EINVAL;
	}

	return 0;
}
#else /* HIF_PCI */
static int ol_get_iram_len_and_pos(struct ol_softc *scn, uint32_t *pos, uint32_t
				   *len, uint32_t section)
{
	*pos = IRAM_LOCATION;
	*len = IRAM_SIZE;

	pr_info("%s: Dumping IRAM Section\n", __func__);
	return 0;
}
#endif

/**
 * ol_get_reg_len() - Get dump length of FW register
 * @scn: Pointer of struct ol_softc
 * @buffer_len: buffer length limitation of register
 *
 * Return: dump length of FW register.
 */
static int ol_get_reg_len(struct ol_softc *scn, u_int32_t buffer_len)
{
	int i, section_len, fill_len;
	int dump_len, result = 0;
	tgt_reg_table reg_table;
	tgt_reg_section *curr_sec, *next_sec;

	section_len = ol_ath_get_reg_table(scn->target_version, &reg_table);

	if (!reg_table.section || !reg_table.section_size || !section_len) {
		printk(KERN_ERR "%s: failed to get reg table\n", __func__);
		result = -EIO;
		goto out;
	}

	curr_sec = reg_table.section;
	for (i = 0; i < reg_table.section_size; i++) {

		dump_len = curr_sec->end_addr - curr_sec->start_addr;

		result += dump_len;

		if (result < section_len) {
			next_sec = (tgt_reg_section *)((u_int8_t *)curr_sec
							+ sizeof(*curr_sec));
			fill_len = next_sec->start_addr - curr_sec->end_addr;
			if ((buffer_len - result) < fill_len) {
				printk("Not enough memory to fill registers:"
						" %d: 0x%08x-0x%08x\n", i,
						curr_sec->end_addr,
						next_sec->start_addr);
				goto out;
			}

			if (fill_len)
				result += fill_len;
		}
		curr_sec++;
	}

out:
	return result;
}

static int ol_read_reg_section(struct ol_softc *scn, char *ptr, uint32_t len)
{
	return ol_diag_read_reg_loc(scn, ptr, len);
}

#ifndef CONFIG_HL_SUPPORT
static int ol_dump_fail_debug_info(struct ol_softc *scn, void *ptr)
{
	dump_CE_register(scn);
	dump_CE_debug_register(scn->hif_sc);
	ol_dump_target_memory(scn->hif_hdl, ptr);

	return -EACCES;
}
#else
static int ol_dump_fail_debug_info(struct ol_softc *scn, void *ptr)
{
	return 0;
}
#endif

/**---------------------------------------------------------------------------
 *   \brief  ol_target_coredump
 *
 *   Function to perform core dump for the target
 *
 *   \param:   scn - ol_softc handler
 *             memoryBlock - non-NULL reserved memory location
 *             blockLength - size of the dump to collect
 *
 *   \return:  None
 * --------------------------------------------------------------------------*/
int ol_target_coredump(void *inst, void *memoryBlock, u_int32_t blockLength)
{
	struct ol_softc *scn = (struct ol_softc *)inst;
	uint8_t *bufferLoc = memoryBlock;
	int result[MAX_SECTION_COUNT];
	int ret = 0;
	uint32_t amountRead = 0;
	uint32_t sectionCount = 0;
	uint32_t pos = 0;
	uint32_t readLen = 0;
	char fw_dump_filename[40];

#ifdef CONFIG_NON_QC_PLATFORM_PCI
	char *fw_ram_seg_name[] = {"DRAM ", "AXI ", "REG ", "IRAM1 ", "IRAM2 "};
#else
	char *fw_ram_seg_name[] = {"DRAM", "AXI", "REG", "IRAM"};
#endif

	if (scn->fastfwdump_host && scn->fastfwdump_fw) {
		if(scn->pdev_txrx_handle) {
			ol_tx_queue_flush(scn->pdev_txrx_handle);
			ol_txrx_pdev_pause(scn->pdev_txrx_handle,
					   OL_TXQ_PAUSE_REASON_FW);
			/*
			 * For BMI ram dump, need to pause VDEV's MCAST_BCAST
			 * and Default mgmt txq in case any tx will cause FW
			 * abnormal except our BMI cmd
			 */
			ol_txrx_pdev_pause_vdev_txq(scn->pdev_txrx_handle,
						OL_TXQ_PAUSE_REASON_CRASH_DUMP);
		}
#ifdef HIF_SDIO
		HIFMaskInterrupt(scn->hif_hdl);
#endif
		BMIInit(scn);
	}

#ifdef FW_RAM_DUMP_TO_PROC
	crash_dump_procfs_init(scn);
#endif

	while ((sectionCount < MAX_SECTION_COUNT) &&
	       (amountRead < blockLength)) {
		switch (sectionCount) {
		case 0:
			pos = DRAM_LOCATION;
			readLen = DRAM_SIZE;
			pr_err("%s: Dumping DRAM section...\n", __func__);
			break;
		case 1:
			pos = AXI_LOCATION;
			readLen = AXI_SIZE;
			pr_err("%s: Dumping AXI section...\n", __func__);
			break;
		case 2:
			pos = REGISTER_LOCATION;
			readLen = ol_get_reg_len(scn, blockLength-amountRead);
			pr_err("%s: Dumping Register section...\n", __func__);
			break;
		case 3:
		case 4:
			ret = ol_get_iram_len_and_pos(scn, &pos, &readLen,
						      sectionCount);
			if (ret) {
				pr_err("%s: Fail to Dump IRAM Section ret:%d\n",
				       __func__, ret);
				goto end;
			}
			break;
		default:
			pr_err("%s: INVALID SECTION_:%d\n", __func__,
			       sectionCount);
			ret = 0;
			goto end;
		}
#ifdef FW_RAM_DUMP_TO_FILE
		memset(fw_dump_filename, 0, sizeof(fw_dump_filename));
		scnprintf(fw_dump_filename, sizeof(fw_dump_filename), "%scld_%s.bin",
			  CRASH_DUMP_PATH, fw_ram_seg_name[sectionCount]);
		if(sectionCount != 2)
			crash_dump_file_init(fw_dump_filename);
#endif
		if (blockLength - amountRead < readLen) {
			pr_err("%s: No memory to dump section:%d buffer!\n",
			       __func__, sectionCount);
			ret = -ENOMEM;
			goto end;
		}

		if (pos == REGISTER_LOCATION) {
			/*
			 * Reg dump can't use BMI related API, and it also
			 * destroyed BMI context. Reg dump should be dumped at
			 * last, otherwise BMIInit should be called again
			 * before IRAM dump.
			 */
			if (scn->fastfwdump_host && scn->fastfwdump_fw)
				result[sectionCount] = readLen;
			else
				result[sectionCount] = ol_read_reg_section(scn,
								bufferLoc,
								blockLength -
								amountRead);
		} else {
			if (scn->fastfwdump_host && scn->fastfwdump_fw) {
				ret = BMIReadMemory(scn->hif_hdl, pos,
						    bufferLoc, readLen, scn);
				if(ret) {
					pr_err("%s:%d BMI CRASH READ FAILURE ! \n", __func__, __LINE__);
					goto end;
				}
				result[sectionCount] = readLen;
			} else {
				result[sectionCount] = ol_diag_read(scn,
								bufferLoc,
								pos, readLen);
			}
		}

		if (result[sectionCount] == -EIO) {
			ret = ol_dump_fail_debug_info(scn, memoryBlock);
			goto end;
		}

		pr_info("%s: Section:%d Bytes Read:%0x\n", __func__,
			sectionCount, result[sectionCount]);
#ifdef CONFIG_NON_QC_PLATFORM_PCI
		printk("\nMemory addr for %s = 0x%p (size: %x)\n",fw_ram_seg_name[sectionCount], bufferLoc, result[sectionCount]);
#endif
#ifdef FW_RAM_DUMP_TO_FILE
		if(sectionCount != 2) {
			crash_dump_write(fw_dump_filename, bufferLoc, result[sectionCount]);
		}
#endif
		amountRead += result[sectionCount];
		bufferLoc += result[sectionCount];
		sectionCount++;
	}
end:
	if (scn->fastfwdump_host && scn->fastfwdump_fw) {
		BMICleanup(scn);
#ifdef HIF_SDIO
		HIFUnMaskInterrupt(scn->hif_hdl);
#endif
		if (scn->pdev_txrx_handle) {
			ol_txrx_pdev_unpause(scn->pdev_txrx_handle,
				     OL_TXQ_PAUSE_REASON_FW);
			ol_txrx_pdev_unpause_vdev_txq(scn->pdev_txrx_handle,
				     OL_TXQ_PAUSE_REASON_CRASH_DUMP);
		}
	}

	if ((!ret) && scn->fastfwdump_host && scn->fastfwdump_fw) {
		pr_err("%s: Register section is delayed here\n", __func__);
		ol_read_reg_section(scn,
				(char *)memoryBlock + result[0] + result[1],
				blockLength - result[0] - result[1]);
	}

	return ret;
}
#endif

u_int8_t ol_get_number_of_peers_supported(struct ol_softc *scn)
{
	u_int8_t max_no_of_peers = 0;

	switch (scn->target_version) {
		case AR6320_REV1_1_VERSION:
			if(scn->max_no_of_peers > MAX_SUPPORTED_PEERS_REV1_1)
				max_no_of_peers = MAX_SUPPORTED_PEERS_REV1_1;
			else
				max_no_of_peers = scn->max_no_of_peers;
			break;

		default:
			if(scn->max_no_of_peers > MAX_SUPPORTED_PEERS)
				max_no_of_peers = MAX_SUPPORTED_PEERS;
			else
				max_no_of_peers = scn->max_no_of_peers;
			break;

	}
	return max_no_of_peers;
}

#ifdef HIF_SDIO

/*Setting SDIO block size, mbox ISR yield limit for SDIO based HIF*/
static A_STATUS
ol_sdio_extra_initialization(struct ol_softc *scn)
{

	A_STATUS status;
	u_int32_t param;
#ifdef CONFIG_DISABLE_SLEEP_BMI_OPTION
	uint32 value;
#endif

	do{
		A_UINT32 blocksizes[HTC_MAILBOX_NUM_MAX];
		unsigned int MboxIsrYieldValue = 99;
		A_UINT32 TargetType = TARGET_TYPE_AR6320;
		/* get the block sizes */
		status = HIFConfigureDevice(scn->hif_hdl, HIF_DEVICE_GET_MBOX_BLOCK_SIZE,
									blocksizes, sizeof(blocksizes));

		if (A_FAILED(status)) {
			printk("Failed to get block size info from HIF layer...\n");
			break;
		}
			/* note: we actually get the block size for mailbox 1, for SDIO the block
						size on mailbox 0 is artificially set to 1 must be a power of 2 */
		A_ASSERT((blocksizes[1] & (blocksizes[1] - 1)) == 0);

		/* set the host interest area for the block size */
		status = BMIWriteMemory(scn->hif_hdl,
					HOST_INTEREST_ITEM_ADDRESS(TargetType, hi_mbox_io_block_sz),
					(A_UCHAR *)&blocksizes[1],
					4,
					scn);

		if (A_FAILED(status)) {
			printk("BMIWriteMemory for IO block size failed \n");
			break;
		}

		if (MboxIsrYieldValue != 0) {
				/* set the host interest area for the mbox ISR yield limit */
			status = BMIWriteMemory(scn->hif_hdl,
						HOST_INTEREST_ITEM_ADDRESS(TargetType,
						hi_mbox_isr_yield_limit),
						(A_UCHAR *)&MboxIsrYieldValue,
						4,
						scn);

			if (A_FAILED(status)) {
				printk("BMIWriteMemory for yield limit failed \n");
				break;
			}
		}

#ifdef CONFIG_DISABLE_SLEEP_BMI_OPTION

		printk("%s: prevent ROME from sleeping\n",__func__);
		BMIReadSOCRegister(scn->hif_hdl,
			MBOX_BASE_ADDRESS + LOCAL_SCRATCH_OFFSET,
			/* this address should be 0x80C0 for ROME*/
			&value,
			scn);

		value |= SOC_OPTION_SLEEP_DISABLE;

		BMIWriteSOCRegister(scn->hif_hdl,
			MBOX_BASE_ADDRESS + LOCAL_SCRATCH_OFFSET,
			value,
			scn);
#endif
		status = BMIReadMemory(scn->hif_hdl,
				HOST_INTEREST_ITEM_ADDRESS(scn->target_type,
				hi_acs_flags),
				(u_int8_t *)&param,
				4,
				scn);
		if (A_FAILED(status)) {
			printk("BMIReadMemory for hi_acs_flags failed \n");
			break;
		}

		param |= (HI_ACS_FLAGS_SDIO_SWAP_MAILBOX_SET |
			HI_ACS_FLAGS_ALT_DATA_CREDIT_SIZE);

		if (!vos_is_ptp_tx_opt_enabled() &&
		    !vos_is_ocb_tx_per_pkt_stats_enabled())
			param |= HI_ACS_FLAGS_SDIO_REDUCE_TX_COMPL_SET;

		/* enable TX completion to collect tx_desc for pktlog */
		if (vos_is_packet_log_enabled())
			param &= ~HI_ACS_FLAGS_SDIO_REDUCE_TX_COMPL_SET;

		BMIWriteMemory(scn->hif_hdl,
				host_interest_item_address(scn->target_type,
				offsetof(struct host_interest_s,
					hi_acs_flags)),
				(u_int8_t *)&param, 4, scn);

	}while(FALSE);

	return status;
}

void
ol_target_ready(struct ol_softc *scn, void *cfg_ctx)
{
	u_int32_t value = 0;
	A_STATUS status = EOK;

	status = HIFDiagReadMem(scn->hif_hdl,
		host_interest_item_address(scn->target_type,
		offsetof(struct host_interest_s, hi_acs_flags)),
		(A_UCHAR *)&value, sizeof(u_int32_t));

	if (status != EOK) {
		printk("%s: HIFDiagReadMem failed:%d\n", __func__, status);
		return;
	}

	if (value & HI_ACS_FLAGS_SDIO_SWAP_MAILBOX_FW_ACK) {
		HIFSetMailboxSwap(scn->hif_hdl);
	}

	if (value & HI_ACS_FLAGS_SDIO_REDUCE_TX_COMPL_FW_ACK) {
		ol_cfg_set_tx_free_at_download(cfg_ctx);

	}
#ifdef HIF_MBOX_SLEEP_WAR
	HIFSetMboxSleep(scn->hif_hdl, true, true, true);
#endif
}
#endif

#if (defined(HIF_USB)) && (defined(USB_RESET_RESUME_PERSISTENCE))
static A_STATUS ol_usb_reset_resume_enable(struct ol_softc *scn)
{
	A_STATUS status;
	u_int32_t value, addr;

	addr = host_interest_item_address(scn->target_type,
			offsetof(struct host_interest_s, hi_option_flag2));

	status = BMIReadMemory(scn->hif_hdl, addr, (A_UCHAR *)&value, 4, scn);
	if (status != A_OK)
		return status;

	value |= HI_OPTION_USB_RESET_RESUME;

	status = BMIWriteMemory(scn->hif_hdl, addr, (A_UCHAR *)&value, 4, scn);
	return status;
}
#else
static inline A_STATUS ol_usb_reset_resume_enable(struct ol_softc *scn)
{
	return A_OK;
}
#endif

#ifdef HIF_USB
static A_STATUS
ol_usb_extra_initialization(struct ol_softc *scn)
{
	A_STATUS status = !EOK;
	u_int32_t param = 0;

	status = ol_usb_reset_resume_enable(scn);
	if (status != A_OK)
		return status;

	param |= HI_ACS_FLAGS_ALT_DATA_CREDIT_SIZE;
	status = BMIWriteMemory(scn->hif_hdl,
				host_interest_item_address(scn->target_type,
				offsetof(struct host_interest_s,
					hi_acs_flags)),
				(u_int8_t *)&param, 4, scn);

	return status;
}
#endif

/**
 * ol_pktlog_init()- Pktlog Module initialization
 * @hif_sc:	ol_softc structure.
 *
 * The API is used to initialize pktlog module for
 * all bus types.
 *
 */

#ifndef REMOVE_PKT_LOG
void ol_pktlog_init(void *hif_sc)
{
	struct ol_softc *ol_sc = (struct ol_softc *)hif_sc;
	int ret;

	ol_pl_sethandle(&ol_sc->pdev_txrx_handle->pl_dev, ol_sc);

	ret = pktlogmod_init(ol_sc);

	if (ret)
		pr_err("%s: pktlogmod_init failed ret:%d\n", __func__, ret);
}
#endif
