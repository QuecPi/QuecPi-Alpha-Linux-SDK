/*
 * Copyright (c) 2012-2019, 2021 The Linux Foundation. All rights reserved.
 * Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
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


/*========================================================================

  \file  wlan_hdd_main.c

  \brief WLAN Host Device Driver implementation

  ========================================================================*/

/**=========================================================================

                       EDIT HISTORY FOR FILE


  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.


  $Header:$   $DateTime: $ $Author: $


  when        who    what, where, why
  --------    ---    --------------------------------------------------------
  04/5/09     Shailender     Created module.
  02/24/10    Sudhir.S.Kohalli  Added to support param for SoftAP module
  06/03/10    js - Added support to hostapd driven deauth/disassoc/mic failure
  ==========================================================================*/

/*--------------------------------------------------------------------------
  Include Files
  ------------------------------------------------------------------------*/
#include<net/addrconf.h>
#include <wlan_hdd_includes.h>
#include <vos_api.h>
#include <vos_sched.h>
#ifdef WLAN_FEATURE_LPSS
#include <vos_utils.h>
#endif
#include <linux/etherdevice.h>
#include <linux/firmware.h>
#include <wcnss_api.h>
#include <wlan_hdd_tx_rx.h>
#include <wniApi.h>
#include <wlan_nlink_srv.h>
#include <wlan_hdd_cfg.h>
#include <wlan_ptt_sock_svc.h>
#include <dbglog_host.h>
#include <wlan_logging_sock_svc.h>
#include <wlan_hdd_wowl.h>
#include <wlan_hdd_misc.h>
#include <wlan_hdd_wext.h>
#include "wlan_hdd_request_manager.h"
#include "wlan_hdd_trace.h"
#include "vos_types.h"
#include "vos_trace.h"

#include <linux/wireless.h>
#include <net/cfg80211.h>
#include <linux/inetdevice.h>
#include <net/addrconf.h>
#include "wlan_hdd_cfg80211.h"
#include "wlan_hdd_p2p.h"
#include <linux/rtnetlink.h>
#include "sapApi.h"
#include <linux/semaphore.h>
#include <linux/ctype.h>
#include <linux/compat.h>
#include <linux/pm_qos.h>
#include <linux/ethtool.h>
#ifdef MSM_PLATFORM
#ifdef CONFIG_CNSS
#include <soc/qcom/subsystem_restart.h>
#endif
#endif
#include <wlan_hdd_hostapd.h>
#include <wlan_hdd_softap_tx_rx.h>
#include "cfgApi.h"
#include "wlan_hdd_dev_pwr.h"
#include "qwlan_version.h"
#include "wlan_qct_wda.h"
#include "wlan_hdd_tdls.h"
#ifdef FEATURE_WLAN_CH_AVOID
#include "vos_cnss.h"
#include "regdomain_common.h"

extern int hdd_hostapd_stop (struct net_device *dev);
#endif /* FEATURE_WLAN_CH_AVOID */

#ifdef WLAN_FEATURE_NAN
#include "wlan_hdd_nan.h"
#endif /* WLAN_FEATURE_NAN */

#include "wlan_hdd_debugfs.h"
#include "epping_main.h"
#include "wlan_hdd_memdump.h"

#include <wlan_hdd_ipa.h>
#if defined(HIF_PCI)
#include "if_pci.h"
#elif defined(HIF_USB)
#include "if_usb.h"
#elif defined(HIF_SDIO)
#include "if_ath_sdio.h"
#endif
#include "wma.h"
#include "ol_fw.h"
#include "wlan_hdd_ocb.h"
#include "wlan_hdd_tsf.h"
#include "tl_shim.h"
#include "wlan_hdd_oemdata.h"
#include "sirApi.h"

#ifdef CNSS_GENL
#include <net/cnss_nl.h>
#endif

#include <wlan_hdd_spectral.h>

#if defined(LINUX_QCMBR)
#define SIOCIOCTLTX99 (SIOCDEVPRIVATE+13)
#endif

#ifdef QCA_ARP_SPOOFING_WAR
#include "ol_if_athvar.h"
#define HDD_ARP_PACKET_TYPE_OFFSET 12
#endif

#ifdef MODULE
#define WLAN_MODULE_NAME  module_name(THIS_MODULE)
#else
#define WLAN_MODULE_NAME  "wlan"
#endif

#ifdef TIMER_MANAGER
#define TIMER_MANAGER_STR " +TIMER_MANAGER"
#else
#define TIMER_MANAGER_STR ""
#endif

#ifdef MEMORY_DEBUG
#define MEMORY_DEBUG_STR " +MEMORY_DEBUG"
#else
#define MEMORY_DEBUG_STR ""
#endif

#ifdef IPA_UC_OFFLOAD
/* If IPA UC data path is enabled, target should reserve extra tx descriptors
 * for IPA WDI data path.
 * Then host data path should allow less TX packet pumping in case
 * IPA WDI data path enabled */
#define WLAN_TFC_IPAUC_TX_DESC_RESERVE   100
#endif /* IPA_UC_OFFLOAD */

/* the Android framework expects this param even though we don't use it */
#define BUF_LEN 20
static char fwpath_buffer[BUF_LEN];
static struct kparam_string fwpath = {
   .string = fwpath_buffer,
   .maxlen = BUF_LEN,
};

static char *country_code;
static int   enable_11d = -1;
static int   enable_dfs_chan_scan = -1;
#ifdef FEATURE_LARGE_PREALLOC
static char *version_string = QWLAN_VERSIONSTR;
#endif

#ifndef MODULE
static int wlan_hdd_inited;
static char fwpath_mode_local[BUF_LEN];
#endif

/*
 * spinlock for synchronizing asynchronous request/response
 * (full description of use in wlan_hdd_main.h)
 */
//DEFINE_SPINLOCK(hdd_context_lock);
adf_os_spinlock_t hdd_context_lock;
/*
 * The rate at which the driver sends RESTART event to supplicant
 * once the function 'vos_wlanRestart()' is called
 *
 */
#define WLAN_HDD_RESTART_RETRY_DELAY_MS 5000  /* 5 second */
#define WLAN_HDD_RESTART_RETRY_MAX_CNT  5     /* 5 retries */

/*
 * Size of Driver command strings from upper layer
 */
#define SIZE_OF_SETROAMMODE             11    /* size of SETROAMMODE */
#define SIZE_OF_GETROAMMODE             11    /* size of GETROAMMODE */

/*
 * Ibss prop IE from command will be of size:
 * size  = sizeof(oui) + sizeof(oui_data) + 1(Element ID) + 1(EID Length)
 * OUI_DATA should be at least 3 bytes long
 */
#define WLAN_HDD_IBSS_MIN_OUI_DATA_LENGTH (3)


#if defined(FEATURE_WLAN_ESE) && defined(FEATURE_WLAN_ESE_UPLOAD)
#define TID_MIN_VALUE 0
#define TID_MAX_VALUE 15
#endif /* FEATURE_WLAN_ESE && FEATURE_WLAN_ESE_UPLOAD */
/*
 * Maximum buffer size used for returning the data back to user space
 */
#define WLAN_MAX_BUF_SIZE 1024
#define WLAN_PRIV_DATA_MAX_LEN    8192
/*
 * Driver miracast parameters 0-Disabled
 * 1-Source, 2-Sink
 */
#define WLAN_HDD_DRIVER_MIRACAST_CFG_MIN_VAL 0
#define WLAN_HDD_DRIVER_MIRACAST_CFG_MAX_VAL 2

/*
 * When ever we need to print IBSSPEERINFOALL for more than 16 STA
 * we will split the printing.
 */
#define NUM_OF_STA_DATA_TO_PRINT 16

#define WLAN_NLINK_CESIUM 30

/*Nss - 1, (Nss = 2 for 2x2)*/
#define NUM_OF_SOUNDING_DIMENSIONS 1

struct android_wifi_reassoc_params {
   unsigned char bssid[18];
   int channel;
};

#define ANDROID_WIFI_ACTION_FRAME_SIZE 1040
struct android_wifi_af_params {
   unsigned char bssid[18];
   int channel;
   int dwell_time;
   int len;
   unsigned char data[ANDROID_WIFI_ACTION_FRAME_SIZE];
} ;

#ifdef WLAN_FEATURE_EXTWOW_SUPPORT
#define WLAN_HDD_MAX_TCP_PORT            65535
#define WLAN_WAIT_TIME_READY_TO_EXTWOW   2000
#endif

#define AUTO_SUSPEND_DELAY_MS    1500

static vos_wake_lock_t wlan_wake_lock;
/* set when SSR is needed after unload */
static e_hdd_ssr_required isSsrRequired = HDD_SSR_NOT_REQUIRED;

#define WOW_MAX_FILTER_LISTS     1
#define WOW_MAX_FILTERS_PER_LIST 4
#define WOW_MIN_PATTERN_SIZE     6
#define WOW_MAX_PATTERN_SIZE     64

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,11,0)) || defined(WITH_BACKPORTS)
static const struct wiphy_wowlan_support wowlan_support_reg_init = {
    .flags = WIPHY_WOWLAN_ANY |
             WIPHY_WOWLAN_MAGIC_PKT |
             WIPHY_WOWLAN_DISCONNECT |
             WIPHY_WOWLAN_SUPPORTS_GTK_REKEY |
             WIPHY_WOWLAN_GTK_REKEY_FAILURE |
             WIPHY_WOWLAN_EAP_IDENTITY_REQ |
             WIPHY_WOWLAN_4WAY_HANDSHAKE |
             WIPHY_WOWLAN_RFKILL_RELEASE,
    .n_patterns = WOW_MAX_FILTER_LISTS * WOW_MAX_FILTERS_PER_LIST,
    .pattern_min_len = WOW_MIN_PATTERN_SIZE,
    .pattern_max_len = WOW_MAX_PATTERN_SIZE,
};

static const struct cfg80211_wowlan wowlan_config = {
	.any = true,
	.disconnect = true,
	.magic_pkt = true,
	.gtk_rekey_failure = true,
	.eap_identity_req = true,
	.four_way_handshake = true,
	.rfkill_release = true,
};
#endif

/* Internal function declarations */
static int hdd_driver_init(void);
static void hdd_driver_exit(void);
#ifdef FW_RAM_DUMP_TO_PROC
extern void crash_dump_exit(void);
#endif

/* Internal function declarations */

static void hdd_tx_fail_ind_callback(v_U8_t *MacAddr, v_U8_t seqNo);

static struct sock *cesium_nl_srv_sock;
static v_U16_t cesium_pid;

static int hdd_ParseIBSSTXFailEventParams(tANI_U8 *pValue,
                                          tANI_U8 *tx_fail_count,
                                          tANI_U16 *pid);

void wlan_hdd_restart_timer_cb(v_PVOID_t usrDataForCallback);

/**
 * struct init_comp - Driver loading status
 * @wlan_start_comp: Completion event
 * @status: Success/Failure
 */
struct init_comp {
	struct completion wlan_start_comp;
	int status;
};
static struct init_comp wlan_comp;

/* Keep track unload status */
static uint32_t g_current_unload_state;
#define TRACK_UNLOAD_STATUS(state) (g_current_unload_state =  state)

#ifdef QCA_WIFI_FTM
extern int hdd_ftm_stop(hdd_context_t *pHddCtx);
#endif
#ifdef FEATURE_WLAN_AUTO_SHUTDOWN
v_VOID_t wlan_hdd_auto_shutdown_cb(v_VOID_t);
#endif

/* Store WLAN driver version info in a global variable such that crash debugger
   can extract it from driver debug symbol and crashdump for post processing */
tANI_U8 g_wlan_driver_version[ ] = QWLAN_VERSIONSTR;

/**
 * hdd_device_mode_to_string() - return string conversion of device mode
 * @device_mode: device mode
 *
 * This utility function helps log string conversion of device mode.
 *
 * Return: string conversion of device mode, if match found;
 *	   "Unknown" otherwise.
 */
const char* hdd_device_mode_to_string(uint8_t device_mode)
{
	switch (device_mode) {
	CASE_RETURN_STRING(WLAN_HDD_INFRA_STATION);
	CASE_RETURN_STRING(WLAN_HDD_SOFTAP);
	CASE_RETURN_STRING(WLAN_HDD_P2P_CLIENT);
	CASE_RETURN_STRING(WLAN_HDD_P2P_GO);
	CASE_RETURN_STRING(WLAN_HDD_MONITOR);
	CASE_RETURN_STRING(WLAN_HDD_FTM);
	CASE_RETURN_STRING(WLAN_HDD_IBSS);
	CASE_RETURN_STRING(WLAN_HDD_P2P_DEVICE);
	CASE_RETURN_STRING(WLAN_HDD_OCB);
	CASE_RETURN_STRING(WLAN_HDD_NDI);
	default:
		return "Unknown";
	}
}

#ifdef WLAN_FEATURE_USB_RECOVERY

/* 1 means usb recovery onging, 0 means no usb recovery or recovery done(probe done) */
int usb_recovery_status = 0;

extern int hif_usb_wlan_en_check(void);

/*
 * Helper func to judge whether it is in recovery state
 *
 *   usb_recovery_status is triggered by our driver
 *   while WLAN_EN PIN could also by triggered by platform driver.
 */

int hdd_in_recovery_state(void)
{
	return ( usb_recovery_status || (!hif_usb_wlan_en_check()) );
}

#endif

#ifdef QCA_LL_TX_FLOW_CT

/**
 * wlan_hdd_clean_tx_flow_control_timer - Function cleans tx flow control timer
 * @hddctx: pointer to hddctx
 * @hdd_adapter_t: pointer to hdd_adapter_t
 *
 * Function deregister's, destroy tx flow control timer
 *
 * Return: None
 */
void wlan_hdd_clean_tx_flow_control_timer(hdd_context_t *hddctx,
					hdd_adapter_t *adapter)
{
	WLANTL_DeRegisterTXFlowControl(hddctx->pvosContext,
					adapter->sessionId);
	if (adapter->tx_flow_timer_initialized == VOS_TRUE) {
		vos_timer_destroy(&adapter->tx_flow_control_timer);
		adapter->tx_flow_timer_initialized = VOS_FALSE;
	}
}

#endif

/**
 * wlan_hdd_find_opclass() - Find operating class for a channel
 * @hal: handler to HAL
 * @channel: channel id
 * @bw_offset: bandwidth offset
 *
 * Function invokes sme api to find the operating class
 *
 * Return: operating class
 */
uint8_t wlan_hdd_find_opclass(tHalHandle hal, uint8_t channel,
	                      uint8_t bw_offset)
{
	uint8_t opclass = 0;

	sme_get_opclass(hal, channel, bw_offset, &opclass);
	return opclass;
}

#ifdef WLAN_FEATURE_SAP_TO_FOLLOW_STA_CHAN
void hdd_csa_notify_cb
(
   void *hdd_context,
   void *indi_param
);
#endif//#ifdef WLAN_FEATURE_SAP_TO_FOLLOW_STA_CHAN

#ifdef FEATURE_GREEN_AP

static void hdd_wlan_green_ap_timer_fn(void *phddctx)
{
    hdd_context_t *pHddCtx = (hdd_context_t *)phddctx;
    hdd_green_ap_ctx_t *green_ap;

    if (0 != wlan_hdd_validate_context(pHddCtx))
        return;

    green_ap = pHddCtx->green_ap_ctx;

    if (green_ap)
        hdd_wlan_green_ap_mc(pHddCtx, green_ap->ps_event);
}

static VOS_STATUS hdd_wlan_green_ap_attach(hdd_context_t *pHddCtx)
{
    hdd_green_ap_ctx_t *green_ap;
    VOS_STATUS status = VOS_STATUS_SUCCESS;

    ENTER();

    green_ap = vos_mem_malloc(sizeof(hdd_green_ap_ctx_t));

    if (!green_ap) {
        hddLog(LOGP, FL("Memory allocation for Green-AP failed!"));
        status = VOS_STATUS_E_NOMEM;
        goto error;
    }

    vos_mem_zero((void *)green_ap, sizeof(*green_ap));
    green_ap->pHddContext = pHddCtx;
    pHddCtx->green_ap_ctx = green_ap;

    green_ap->ps_state = GREEN_AP_PS_OFF_STATE;
    green_ap->ps_event = 0;
    green_ap->num_nodes = 0;
    green_ap->ps_on_time = GREEN_AP_PS_ON_TIME;
    green_ap->ps_delay_time = GREEN_AP_PS_DELAY_TIME;

    vos_timer_init(&green_ap->ps_timer,
            VOS_TIMER_TYPE_SW,
            hdd_wlan_green_ap_timer_fn,
            (void *)pHddCtx);

error:

    EXIT();
    return status;
}

static VOS_STATUS hdd_wlan_green_ap_deattach(hdd_context_t *pHddCtx)
{
    hdd_green_ap_ctx_t *green_ap = pHddCtx->green_ap_ctx;
    VOS_STATUS status = VOS_STATUS_SUCCESS;

    ENTER();

    if (green_ap == NULL) {
        hddLog(LOG1, FL("Green-AP is not enabled"));
        status = VOS_STATUS_E_NOSUPPORT;
        goto done;
    }

    status = vos_timer_deinit(&green_ap->ps_timer);
    if (!VOS_IS_STATUS_SUCCESS(status))
        hddLog(LOG1, FL("Cannot deallocate Green-AP's timer"));

    /* release memory */
    vos_mem_zero((void *)green_ap, sizeof(*green_ap));
    vos_mem_free(green_ap);
    pHddCtx->green_ap_ctx = NULL;

done:

    EXIT();
    return status;
}

static void hdd_wlan_green_ap_update(hdd_context_t *pHddCtx,
    hdd_green_ap_ps_state_t state,
    hdd_green_ap_event_t event)
{
    hdd_green_ap_ctx_t *green_ap = pHddCtx->green_ap_ctx;

    green_ap->ps_state = state;
    green_ap->ps_event = event;
}

static int hdd_wlan_green_ap_enable(hdd_adapter_t *pHostapdAdapter,
        v_U8_t enable)
{
    int ret = 0;

    hddLog(LOG1, "%s: Set Green-AP val: %d", __func__, enable);

    ret = process_wma_set_command(
            (int)pHostapdAdapter->sessionId,
            (int)WMI_PDEV_GREEN_AP_PS_ENABLE_CMDID,
            enable,
            DBG_CMD);

    return ret;
}

void hdd_wlan_green_ap_mc(hdd_context_t *pHddCtx,
        hdd_green_ap_event_t event)
{
    hdd_green_ap_ctx_t *green_ap = pHddCtx->green_ap_ctx;
    hdd_adapter_t *pAdapter = NULL;

    if (green_ap == NULL)
        return ;

    hddLog(LOG1, "%s: Green-AP event: %d, state: %d, num_nodes: %d",
        __func__, event, green_ap->ps_state, green_ap->num_nodes);

    /* handle the green ap ps event */
    switch(event) {
        case GREEN_AP_PS_START_EVENT:
            green_ap->ps_enable = 1;
            break;

        case GREEN_AP_PS_STOP_EVENT:
            green_ap->ps_enable = 0;
            break;

        case GREEN_AP_ADD_STA_EVENT:
            green_ap->num_nodes++;
            break;

        case GREEN_AP_DEL_STA_EVENT:
            if (green_ap->num_nodes)
                green_ap->num_nodes--;
            break;

        case GREEN_AP_PS_ON_EVENT:
        case GREEN_AP_PS_WAIT_EVENT:
            break;

        default:
            hddLog(LOGE, "%s: invalid event %d", __func__, event);
            break;
    }

    pAdapter = hdd_get_adapter (pHddCtx, WLAN_HDD_SOFTAP );

    if (pAdapter == NULL) {
        hddLog(LOGE, FL("Green-AP no SAP adapter"));
        goto done;
    }

    /* Confirm that power save is enabled  before doing state transitions */
    if (!green_ap->ps_enable) {
        hddLog(VOS_TRACE_LEVEL_INFO, FL("green ap is disabled"));
        hdd_wlan_green_ap_update(pHddCtx,
            GREEN_AP_PS_OFF_STATE, GREEN_AP_PS_WAIT_EVENT);
        if (hdd_wlan_green_ap_enable(pAdapter, 0))
            hddLog(LOGE, FL("failed to set green ap mode"));
        goto done;
    }

    /* handle the green ap ps state */
    switch(green_ap->ps_state) {
        case GREEN_AP_PS_IDLE_STATE:
            hdd_wlan_green_ap_update(pHddCtx,
                GREEN_AP_PS_OFF_STATE, GREEN_AP_PS_WAIT_EVENT);
            break;

        case GREEN_AP_PS_OFF_STATE:
            if (!green_ap->num_nodes) {
                hdd_wlan_green_ap_update(pHddCtx,
                    GREEN_AP_PS_WAIT_STATE, GREEN_AP_PS_WAIT_EVENT);
                vos_timer_start(&green_ap->ps_timer,
                        green_ap->ps_delay_time);
            }
            break;

        case GREEN_AP_PS_WAIT_STATE:
            if (!green_ap->num_nodes) {
                hdd_wlan_green_ap_update(pHddCtx,
                   GREEN_AP_PS_ON_STATE, GREEN_AP_PS_WAIT_EVENT);

                hdd_wlan_green_ap_enable(pAdapter, 1);

                if (green_ap->ps_on_time) {
                    hdd_wlan_green_ap_update(pHddCtx,
                        0, GREEN_AP_PS_WAIT_EVENT);
                    vos_timer_start(&green_ap->ps_timer,
                            green_ap->ps_on_time);
                }
            } else {
                hdd_wlan_green_ap_update(pHddCtx,
                    GREEN_AP_PS_OFF_STATE, GREEN_AP_PS_WAIT_EVENT);
            }
            break;

        case GREEN_AP_PS_ON_STATE:
            if (green_ap->num_nodes) {
                if (hdd_wlan_green_ap_enable(pAdapter, 0)) {
                    hddLog(LOGE, FL("FAILED TO SET GREEN-AP mode"));
                    goto done;
                }
                hdd_wlan_green_ap_update(pHddCtx,
                    GREEN_AP_PS_OFF_STATE, GREEN_AP_PS_WAIT_EVENT);
            } else if ((green_ap->ps_event == GREEN_AP_PS_WAIT_EVENT) &&
                    (green_ap->ps_on_time)) {

                /* ps_on_time timeout, switch to ps off */
                hdd_wlan_green_ap_update(pHddCtx,
                    GREEN_AP_PS_WAIT_STATE, GREEN_AP_PS_ON_EVENT);

                if (hdd_wlan_green_ap_enable(pAdapter, 0)) {
                    hddLog(LOGE, FL("FAILED TO SET GREEN-AP mode"));
                    goto done;
                }

                vos_timer_start(&green_ap->ps_timer,
                        green_ap->ps_delay_time);
            }
            break;

        default:
            hddLog(LOGE, "%s: invalid state %d", __func__, green_ap->ps_state);
            hdd_wlan_green_ap_update(pHddCtx,
                GREEN_AP_PS_OFF_STATE, GREEN_AP_PS_WAIT_EVENT);
            break;
    }

done:
    return;
}

/**
 * hdd_wlan_green_ap_init() - Initialize Green AP feature
 * @hdd_ctx: HDD global context
 *
 * Return: none
 */
void hdd_wlan_green_ap_init(struct hdd_context_s *hdd_ctx)
{
	if (!VOS_IS_STATUS_SUCCESS(hdd_wlan_green_ap_attach(hdd_ctx)))
		hddLog(LOGE, FL("Failed to allocate Green-AP resource"));
}

/**
 * hdd_wlan_green_ap_deinit() - De-initialize Green AP feature
 * @hdd_ctx: HDD global context
 *
 * Return: none
 */
void hdd_wlan_green_ap_deinit(struct hdd_context_s *hdd_ctx)
{
	if (!VOS_IS_STATUS_SUCCESS(hdd_wlan_green_ap_deattach(hdd_ctx)))
		hddLog(LOGE, FL("Cannot deallocate Green-AP resource"));
}

/**
 * wlan_hdd_set_egap_support() - helper function to set egap support flag
 * @hdd_ctx:   pointer to hdd context
 * @cfg:       pointer to hdd target configuration knob
 *
 * Return:     None
 */
void wlan_hdd_set_egap_support(hdd_context_t *hdd_ctx, struct hdd_tgt_cfg *cfg)
{
	if (hdd_ctx && cfg)
		hdd_ctx->green_ap_ctx->egap_support = cfg->egap_support;
}

/**
 * hdd_wlan_is_egap_enabled() - Get Enhance Green AP feature status
 * @fw_egap_support: flag whether firmware supports egap or not
 * @cfg: pointer to the struct hdd_config_t
 *
 * Return: true if firmware, feature_flag and ini are all enabled the egap
 */
static bool hdd_wlan_is_egap_enabled(bool fw_egap_support, hdd_config_t *cfg)
{
	/* check if the firmware and ini are both enabled the egap,
	 * and also the feature_flag enable.
	 */
	if (fw_egap_support && cfg->enable_egap &&
			cfg->egap_feature_flag)
		return true;

	return false;
}


/**
 * hdd_wlan_enable_egap() - Enable Enhance Green AP
 * @hdd_ctx: HDD global context
 *
 * Return: 0 on success, negative errno on failure
 */
int hdd_wlan_enable_egap(struct hdd_context_s *hdd_ctx)
{
	hdd_config_t *cfg;

	if (!hdd_ctx) {
		hddLog(LOGE, FL("hdd context is NULL"));
		return -EINVAL;
	}

	cfg = hdd_ctx->cfg_ini;

	if (!cfg) {
		hddLog(LOGE, FL("hdd cfg is NULL"));
		return -EINVAL;
	}

	if (!hdd_ctx->green_ap_ctx) {
		hddLog(LOGE, FL("green ap context is NULL"));
		return -EINVAL;
	}

	if (!hdd_wlan_is_egap_enabled(hdd_ctx->green_ap_ctx->egap_support,
			hdd_ctx->cfg_ini))
		return -ENOTSUPP;

	if (VOS_STATUS_SUCCESS != sme_send_egap_conf_params(cfg->enable_egap,
			cfg->egap_inact_time,
			cfg->egap_wait_time,
			cfg->egap_feature_flag))
		return -EINVAL;
	return 0;
}

/**
 * hdd_wlan_green_ap_start_bss() - Notify Green AP of Start BSS event
 * @hdd_ctx: HDD global context
 *
 * Return: none
 */
void hdd_wlan_green_ap_start_bss(struct hdd_context_s *hdd_ctx)
{
	hdd_config_t *cfg;

	if (!hdd_ctx) {
		hddLog(LOGE, FL("hdd context is NULL"));
		return;
	}

	cfg = hdd_ctx->cfg_ini;

	if (!cfg) {
		hddLog(LOGE, FL("hdd cfg is NULL"));
		return;
	}

	if (!hdd_ctx->green_ap_ctx) {
		hddLog(LOGE, FL("green ap context is NULL"));
		return;
	}

	if (hdd_wlan_is_egap_enabled(hdd_ctx->green_ap_ctx->egap_support,
			hdd_ctx->cfg_ini))
		return;

	if ((hdd_ctx->concurrency_mode & VOS_SAP) &&
			!(hdd_ctx->concurrency_mode & (~VOS_SAP)) &&
			cfg->enable2x2 && cfg->enableGreenAP) {
		hddLog(LOG1,
			FL("Green AP enabled - sta_con: %d, 2x2: %d, GAP: %d"),
			(VOS_STA & hdd_ctx->concurrency_mode),
			cfg->enable2x2, cfg->enableGreenAP);
		hdd_wlan_green_ap_mc(hdd_ctx, GREEN_AP_PS_START_EVENT);
	} else {
		hdd_wlan_green_ap_mc(hdd_ctx, GREEN_AP_PS_STOP_EVENT);
		hddLog(LOG1,
			FL("Green AP disabled- sta_con: %d, 2x2: %d, GAP: %d"),
			(VOS_STA & hdd_ctx->concurrency_mode),
			cfg->enable2x2, cfg->enableGreenAP);
	}
}

/**
 * hdd_wlan_green_ap_stop_bss() - Notify Green AP of Stop BSS event
 * @hdd_ctx: HDD global context
 *
 * Return: none
 */
void hdd_wlan_green_ap_stop_bss(struct hdd_context_s *hdd_ctx)
{
	if (!hdd_ctx) {
		hddLog(LOGE, FL("hdd context is NULL"));
		return;
	}

	if (!hdd_ctx->cfg_ini) {
		hddLog(LOGE, FL("hdd cfg is NULL"));
		return;
	}

	if (!hdd_ctx->green_ap_ctx) {
		hddLog(LOGE, FL("green ap context is NULL"));
		return;
	}

	if (hdd_wlan_is_egap_enabled(hdd_ctx->green_ap_ctx->egap_support,
			hdd_ctx->cfg_ini))
		return;

	/* For AP+AP mode, only trigger GREEN_AP_PS_STOP_EVENT, when the
	 * last AP stops.
	 */
	if (1 == (hdd_ctx->no_of_open_sessions[VOS_STA_SAP_MODE]))
		hdd_wlan_green_ap_mc(hdd_ctx, GREEN_AP_PS_STOP_EVENT);
}

/**
 * hdd_wlan_green_ap_add_sta() - Notify Green AP of Add Station  event
 * @hdd_ctx: HDD global context
 *
 * Return: none
 */
void hdd_wlan_green_ap_add_sta(struct hdd_context_s *hdd_ctx)
{
	if (!hdd_ctx) {
		hddLog(LOGE, FL("hdd context is NULL"));
		return;
	}

	if (!hdd_ctx->cfg_ini) {
		hddLog(LOGE, FL("hdd cfg is NULL"));
		return;
	}

	if (!hdd_ctx->green_ap_ctx) {
		hddLog(LOGE, FL("green ap context is NULL"));
		return;
	}

	if (hdd_wlan_is_egap_enabled(hdd_ctx->green_ap_ctx->egap_support,
			hdd_ctx->cfg_ini))
		return;

	hdd_wlan_green_ap_mc(hdd_ctx, GREEN_AP_ADD_STA_EVENT);
}

/**
 * hdd_wlan_green_ap_del_sta() - Notify Green AP of Delete Station event
 * @hdd_ctx: HDD global context
 *
 * Return: none
 */
void hdd_wlan_green_ap_del_sta(struct hdd_context_s *hdd_ctx)
{
	if (!hdd_ctx) {
		hddLog(LOGE, FL("hdd context is NULL"));
		return;
	}

	if (!hdd_ctx->cfg_ini) {
		hddLog(LOGE, FL("hdd cfg is NULL"));
		return;
	}

	if (!hdd_ctx->green_ap_ctx) {
		hddLog(LOGE, FL("green ap context is NULL"));
		return;
	}

	if (hdd_wlan_is_egap_enabled(hdd_ctx->green_ap_ctx->egap_support,
			hdd_ctx->cfg_ini))
		return;

	hdd_wlan_green_ap_mc(hdd_ctx, GREEN_AP_DEL_STA_EVENT);
}

#endif /* FEATURE_GREEN_AP */

/**
 * hdd_lost_link_info_cb() - callback function to get lost link information
 * @context: HDD context
 * @lost_link_info: lost link information
 *
 * Return: none
 */
static void hdd_lost_link_info_cb(void *context,
				  struct sir_lost_link_info *lost_link_info)
{
	hdd_context_t *hdd_ctx = (hdd_context_t *)context;
	int status;
	hdd_adapter_t *adapter;

	status = wlan_hdd_validate_context(hdd_ctx);
	if (0 != status)
		return;

	if (NULL == lost_link_info) {
		hddLog(LOGE, "%s: lost_link_info is NULL", __func__);
		return;
	}

	adapter = hdd_get_adapter_by_vdev(hdd_ctx, lost_link_info->vdev_id);
	if (NULL == adapter) {
		hddLog(LOGE, "%s: invalid adapter", __func__);
		return;
	}

	adapter->rssi_on_disconnect = lost_link_info->rssi;
	hddLog(LOG1, "%s: rssi on disconnect %d",
		     __func__, adapter->rssi_on_disconnect);
}

/**
 * __hdd_smps_force_mode_cb() - callback for smps force mode
 * event
 * @context: HDD context
 * @smps_mode_event: smps force mode event info
 *
 * Return: none
 */
static void __hdd_smps_force_mode_cb(void *context,
		struct sir_smps_force_mode_event *smps_mode_event)
{
	hdd_context_t *hdd_ctx = (hdd_context_t *)context;
	int status;
	hdd_adapter_t *adapter;

	ENTER();

	status = wlan_hdd_validate_context(hdd_ctx);
	if (0 != status)
		return;

	if (NULL == smps_mode_event) {
		hddLog(LOGE, FL("smps_mode_event is NULL"));
		return;
	}

	adapter = hdd_get_adapter_by_vdev(hdd_ctx, smps_mode_event->vdev_id);
	if (NULL == adapter) {
		hddLog(LOGE, FL("Invalid adapter"));
		return;
	}

	adapter->smps_force_mode_status = smps_mode_event->status;

	complete(&adapter->smps_force_mode_comp_var);

	hddLog(LOG1, FL("status %d vdev_id: %d"),
	       smps_mode_event->status, smps_mode_event->vdev_id);
}

/**
 * hdd_smps_force_mode_cb() - Wrapper to protect
 * __hdd_smps_force_mode_cb callback for smps force mode event
 * @context: HDD context
 * @smps_mode_event: smps force mode event info
 *
 * Return: none
 */
static void hdd_smps_force_mode_cb(void *context,
		struct sir_smps_force_mode_event *smps_mode_event)
{
	vos_ssr_protect(__func__);
	__hdd_smps_force_mode_cb(context, smps_mode_event);
	vos_ssr_unprotect(__func__);

}

#if defined (FEATURE_WLAN_MCC_TO_SCC_SWITCH) || defined (FEATURE_WLAN_STA_AP_MODE_DFS_DISABLE) || defined (FEATURE_WLAN_CH_AVOID)
/**
 * wlan_hdd_restart_sap() - to restart SAP in driver internally
 * @ap_adapter: - Pointer to SAP hdd_adapter_t structure
 *
 * wlan_hdd_restart_sap first delete SAP and do cleanup.
 * After that WLANSAP_StartBss start re-start process of SAP.
 *
 * Return: None
 */

void wlan_hdd_restart_sap(hdd_adapter_t *ap_adapter)
{
    hdd_ap_ctx_t *pHddApCtx;
    hdd_hostapd_state_t *pHostapdState;
    VOS_STATUS vos_status;
    hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(ap_adapter);
#ifdef CFG80211_DEL_STA_V2
    struct station_del_parameters delStaParams;
#endif
    tsap_Config_t *pConfig;
    void *p_sap_ctx;

    pHddApCtx = WLAN_HDD_GET_AP_CTX_PTR(ap_adapter);
    pConfig = &pHddApCtx->sapConfig;
    p_sap_ctx = pHddApCtx->sapContext;

    mutex_lock(&pHddCtx->sap_lock);
    if (test_bit(SOFTAP_BSS_STARTED, &ap_adapter->event_flags)) {
#ifdef CFG80211_DEL_STA_V2
        delStaParams.mac = NULL;
        delStaParams.subtype = SIR_MAC_MGMT_DEAUTH >> 4;
        delStaParams.reason_code = eCsrForcedDeauthSta;
        wlan_hdd_cfg80211_del_station(ap_adapter->wdev.wiphy, ap_adapter->dev,
                                      &delStaParams);
#else
        wlan_hdd_cfg80211_del_station(ap_adapter->wdev.wiphy, ap_adapter->dev,
                                      NULL);
#endif
        hdd_cleanup_actionframe(pHddCtx, ap_adapter);

        pHostapdState = WLAN_HDD_GET_HOSTAP_STATE_PTR(ap_adapter);
        vos_event_reset(&pHostapdState->stop_bss_event);

        if (VOS_STATUS_SUCCESS == WLANSAP_StopBss(p_sap_ctx)) {
            vos_status = vos_wait_single_event(&pHostapdState->stop_bss_event,
                                               10000);
            if (!VOS_IS_STATUS_SUCCESS(vos_status)) {
                hddLog(LOGE, FL("SAP Stop Failed"));
                goto end;
            }
        }
        clear_bit(SOFTAP_BSS_STARTED, &ap_adapter->event_flags);
        wlan_hdd_decr_active_session(pHddCtx, ap_adapter->device_mode);
        hddLog(LOGE, FL("SAP Stop Success"));
#ifdef WLAN_FEATURE_SAP_TO_FOLLOW_STA_CHAN
                /*this delay is needed to ensure proper resource cleanup of SAP*/
                vos_sleep(1000);
#endif  //#ifdef WLAN_FEATURE_SAP_TO_FOLLOW_STA_CHAN
        if (pHddCtx->cfg_ini->apOBSSProtEnabled)
            vos_runtime_pm_allow_suspend(pHddCtx->runtime_context.obss);
        if (0 != wlan_hdd_cfg80211_update_apies(ap_adapter)) {
            hddLog(LOGE, FL("SAP Not able to set AP IEs"));
            WLANSAP_ResetSapConfigAddIE(pConfig, eUPDATE_IE_ALL);
            goto end;
        }

        vos_event_reset(&pHostapdState->vosEvent);
        if (WLANSAP_StartBss(p_sap_ctx, hdd_hostapd_SAPEventCB, pConfig,
            (v_PVOID_t)ap_adapter->dev) != VOS_STATUS_SUCCESS) {
            hddLog(LOGE, FL("SAP Start Bss fail"));
            WLANSAP_ResetSapConfigAddIE(pConfig, eUPDATE_IE_ALL);
            goto end;
        }

        hddLog(LOG1, FL("Waiting for SAP to start"));
        vos_status = vos_wait_single_event(&pHostapdState->vosEvent, 10000);
        WLANSAP_ResetSapConfigAddIE(pConfig, eUPDATE_IE_ALL);
        if (!VOS_IS_STATUS_SUCCESS(vos_status)) {
            hddLog(LOGE, FL("SAP Start failed"));
            goto end;
        }
        hddLog(LOGE, FL("SAP Start Success"));
        set_bit(SOFTAP_BSS_STARTED, &ap_adapter->event_flags);
        wlan_hdd_incr_active_session(pHddCtx, ap_adapter->device_mode);
        pHostapdState->bCommit = TRUE;
        if (pHddCtx->cfg_ini->apOBSSProtEnabled)
            vos_runtime_pm_prevent_suspend(pHddCtx->runtime_context.obss);
    }
end:
    mutex_unlock(&pHddCtx->sap_lock);
    return;
}
#endif

static int __hdd_netdev_notifier_call(struct notifier_block * nb,
                                         unsigned long state,
                                         void *data)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,11,0))
   struct netdev_notifier_info *dev_notif_info = data;
   struct net_device *dev = dev_notif_info->dev;
#else
   struct net_device *dev = data;
#endif
   hdd_adapter_t *pAdapter = WLAN_HDD_GET_PRIV_PTR(dev);
   hdd_context_t *pHddCtx;

   //Make sure that this callback corresponds to our device.
   if ((strncmp(dev->name, "wlan", 4)) &&
      (strncmp(dev->name, "softAP", 6)) &&
      (strncmp(dev->name, "p2p", 3)))
      return NOTIFY_DONE;

   if ((pAdapter->magic != WLAN_HDD_ADAPTER_MAGIC) ||
      (pAdapter->dev != dev)) {
      hddLog(LOGE, FL("device adapter is not matching!!!"));
      return NOTIFY_DONE;
   }

   if (!dev->ieee80211_ptr) {
      hddLog(LOGE, FL("ieee80211_ptr is NULL!!!"));
      return NOTIFY_DONE;
   }

   pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
   if (NULL == pHddCtx)
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,"%s: HDD Context Null Pointer", __func__);
      VOS_ASSERT(0);
      return NOTIFY_DONE;
   }
   if (pHddCtx->isLogpInProgress)
      return NOTIFY_DONE;


   hddLog(VOS_TRACE_LEVEL_INFO, "%s: %s New Net Device State = %lu",
          __func__, dev->name, state);

   switch (state) {
   case NETDEV_REGISTER:
        break;

   case NETDEV_UNREGISTER:
        break;

   case NETDEV_UP:
#ifdef FEATURE_WLAN_CH_AVOID
        sme_ChAvoidUpdateReq(pHddCtx->hHal);
#endif
        break;

   case NETDEV_DOWN:
        break;

   case NETDEV_CHANGE:
        if(TRUE == pAdapter->isLinkUpSvcNeeded)
           complete(&pAdapter->linkup_event_var);
        break;

   case NETDEV_GOING_DOWN:
        if( pAdapter->scan_info.mScanPending != FALSE )
        {
           unsigned long rc;
#ifdef WLAN_FEATURE_USB_RECOVERY
           if(hdd_in_recovery_state()) {
              printk(KERN_ERR "%s in recovery state, ignore abort scan\n", __func__);
              break;
           }
#endif
           INIT_COMPLETION(pAdapter->scan_info.abortscan_event_var);
           hdd_abort_mac_scan(pAdapter->pHddCtx, pAdapter->sessionId,
                              eCSR_SCAN_ABORT_DEFAULT);
           rc = wait_for_completion_timeout(
                               &pAdapter->scan_info.abortscan_event_var,
                               msecs_to_jiffies(WLAN_WAIT_TIME_ABORTSCAN));
           if (!rc) {
              VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                        "%s: Timeout occurred while waiting for abortscan",
                        __func__);
           }
        }
        else
        {
           vos_flush_work(&pAdapter->scan_block_work);
           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
               "%s: Scan is not Pending from user" , __func__);
        }
        break;

   default:
        break;
   }

   return NOTIFY_DONE;
}

/**
 * hdd_netdev_notifier_call() - netdev notifier callback function
 * @nb: pointer to notifier block
 * @state: state
 * @ndev: ndev pointer
 *
 * Return: 0 on success, error number otherwise.
 */
static int hdd_netdev_notifier_call(struct notifier_block * nb,
					unsigned long state,
					void *ndev)
{
	int ret;

	vos_ssr_protect(__func__);
	ret = __hdd_netdev_notifier_call(nb, state, ndev);
	vos_ssr_unprotect(__func__);

	return ret;
}

struct notifier_block hdd_netdev_notifier = {
   .notifier_call = hdd_netdev_notifier_call,
};

/*---------------------------------------------------------------------------
 *   Function definitions
 *-------------------------------------------------------------------------*/
void hdd_unregister_mcast_bcast_filter(hdd_context_t *pHddCtx);
void hdd_register_mcast_bcast_filter(hdd_context_t *pHddCtx);
//variable to hold the insmod parameters
static int con_mode;
#ifndef MODULE
/* current con_mode - used only for statically linked driver
 * con_mode is changed by user space to indicate a mode change which will
 * result in calling the module exit and init functions. The module
 * exit function will clean up based on the value of con_mode prior to it
 * being changed by user space. So curr_con_mode records the current con_mode
 * for exit when con_mode becomes the next mode for init
 */
static int curr_con_mode;
#endif

/**---------------------------------------------------------------------------

  \brief hdd_vos_trace_enable() - Configure initial VOS Trace enable

  Called immediately after the cfg.ini is read in order to configure
  the desired trace levels.

  \param  - moduleId - module whose trace level is being configured
  \param  - bitmask - bitmask of log levels to be enabled

  \return - void

  --------------------------------------------------------------------------*/
static void hdd_vos_trace_enable(VOS_MODULE_ID moduleId, v_U32_t bitmask)
{
   VOS_TRACE_LEVEL level;

   /* if the bitmask is the default value, then a bitmask was not
      specified in cfg.ini, so leave the logging level alone (it
      will remain at the "compiled in" default value) */
   if (CFG_VOS_TRACE_ENABLE_DEFAULT == bitmask)
   {
      return;
   }

   /* a mask was specified.  start by disabling all logging */
   vos_trace_setValue(moduleId, VOS_TRACE_LEVEL_NONE, 0);

   /* now cycle through the bitmask until all "set" bits are serviced */
   level = VOS_TRACE_LEVEL_FATAL;
   while (0 != bitmask)
   {
      if (bitmask & 1)
      {
         vos_trace_setValue(moduleId, level, 1);
      }
      level++;
      bitmask >>= 1;
   }
}


/*
 * FUNCTION: wlan_hdd_validate_context
 * This function is used to check the HDD context
 */
int wlan_hdd_validate_context(hdd_context_t *pHddCtx)
{

    if (NULL == pHddCtx || NULL == pHddCtx->cfg_ini) {
        hddLog(LOG1, FL("%pS HDD context is Null"), (void *)_RET_IP_);
        return -ENODEV;
    }

    if (pHddCtx->isLogpInProgress) {
        hddLog(LOG1, FL("%pS LOGP in Progress. Ignore!!!"), (void *)_RET_IP_);
        return -EAGAIN;
    }

    if ((pHddCtx->isLoadInProgress) ||
        (pHddCtx->isUnloadInProgress)) {
        hddLog(LOG1,
             FL("%pS loading: %d unloading:%d in Progress. Ignore!!!"),
             (void *)_RET_IP_,
             pHddCtx->isLoadInProgress,
             pHddCtx->isUnloadInProgress);
        if (pHddCtx->isUnloadInProgress)
             hddLog(LOG1, "current unload state: %d", g_current_unload_state);
        return -EAGAIN;
    }
    return 0;
}


void hdd_checkandupdate_phymode( hdd_context_t *pHddCtx)
{
   hdd_adapter_t *pAdapter = NULL;
   hdd_station_ctx_t *pHddStaCtx = NULL;
   eCsrPhyMode phyMode;
   hdd_config_t *cfg_param = NULL;

   if (NULL == pHddCtx)
   {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_FATAL,
               "HDD Context is null !!");
       return ;
   }

   pAdapter = hdd_get_adapter(pHddCtx, WLAN_HDD_INFRA_STATION);
   if (NULL == pAdapter)
   {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_FATAL,
               "pAdapter is null !!");
       return ;
   }

   pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);

   cfg_param = pHddCtx->cfg_ini;
   if (NULL == cfg_param)
   {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_FATAL,
               "cfg_params not available !!");
       return ;
   }

   phyMode = sme_GetPhyMode(WLAN_HDD_GET_HAL_CTX(pAdapter));

   if (!pHddCtx->isVHT80Allowed)
   {
       if ((eCSR_DOT11_MODE_AUTO == phyMode) ||
           (eCSR_DOT11_MODE_11ac == phyMode) ||
           (eCSR_DOT11_MODE_11ac_ONLY == phyMode))
       {
            VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                    "Setting phymode to 11n!!");
           sme_SetPhyMode(WLAN_HDD_GET_HAL_CTX(pAdapter), eCSR_DOT11_MODE_11n);
       }
   }
   else
   {
       /*New country Supports 11ac as well resetting value back from .ini*/
       sme_SetPhyMode(WLAN_HDD_GET_HAL_CTX(pAdapter),
             hdd_cfg_xlate_to_csr_phy_mode(cfg_param->dot11Mode));
       return ;
   }

   if ((eConnectionState_Associated == pHddStaCtx->conn_info.connState) &&
       ((eCSR_CFG_DOT11_MODE_11AC_ONLY == pHddStaCtx->conn_info.dot11Mode) ||
        (eCSR_CFG_DOT11_MODE_11AC == pHddStaCtx->conn_info.dot11Mode)))
   {
       VOS_STATUS vosStatus;

       // need to issue a disconnect to CSR.
       INIT_COMPLETION(pAdapter->disconnect_comp_var);
       vosStatus = sme_RoamDisconnect(WLAN_HDD_GET_HAL_CTX(pAdapter),
                          pAdapter->sessionId,
                          eCSR_DISCONNECT_REASON_UNSPECIFIED );

       if (VOS_STATUS_SUCCESS == vosStatus)
       {
           unsigned long rc;

           rc = wait_for_completion_timeout(&pAdapter->disconnect_comp_var,
                      msecs_to_jiffies(WLAN_WAIT_TIME_DISCONNECT));
           if (!rc)
               hddLog(LOGE, FL("failure waiting for disconnect_comp_var"));
        }
   }
}

static void
hdd_checkandupdate_dfssetting(hdd_adapter_t *pAdapter, char *country_code)
{
    hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
    hdd_config_t *cfg_param;

    if (NULL == pHddCtx)
    {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_FATAL,
                "HDD Context is null !!");
        return ;
    }

    cfg_param = pHddCtx->cfg_ini;

    if (NULL == cfg_param)
    {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_FATAL,
                "cfg_params not available !!");
        return ;
    }

    if (NULL != strstr(cfg_param->listOfNonDfsCountryCode, country_code))
    {
       /*New country doesn't support DFS */
       sme_UpdateDfsSetting(WLAN_HDD_GET_HAL_CTX(pAdapter), 0);
    }
    else
    {
       /*New country Supports DFS as well resetting value back from .ini*/
       sme_UpdateDfsSetting(WLAN_HDD_GET_HAL_CTX(pAdapter), cfg_param->enableDFSChnlScan);
    }

}

/* Function header is left blank intentionally */
static int hdd_parse_setrmcenable_command(tANI_U8 *pValue, tANI_U8 *pRmcEnable)
{
    tANI_U8 *inPtr = pValue;
    int tempInt;
    int v = 0;
    char buf[32];
    *pRmcEnable = 0;

    inPtr = strnchr(pValue, strlen(pValue), SPACE_ASCII_VALUE);

    if (NULL == inPtr)
    {
        return 0;
    }

    else if (SPACE_ASCII_VALUE != *inPtr)
    {
        return 0;
    }

    while ((SPACE_ASCII_VALUE  == *inPtr) && ('\0' !=  *inPtr)) inPtr++;

    if ('\0' == *inPtr)
    {
        return 0;
    }

    sscanf(inPtr, "%31s ", buf);
    v = kstrtos32(buf, 10, &tempInt);
    if ( v < 0)
    {
       return -EINVAL;
    }

    *pRmcEnable = tempInt;

    VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
       "ucRmcEnable: %d", *pRmcEnable);

    return 0;
}

/* Function header is left blank intentionally */
static int hdd_parse_setrmcactionperiod_command(tANI_U8 *pValue,
           tANI_U32 *pActionPeriod)
{
    tANI_U8 *inPtr = pValue;
    int tempInt;
    int v = 0;
    char buf[32];
    *pActionPeriod = 0;

    inPtr = strnchr(pValue, strlen(pValue), SPACE_ASCII_VALUE);

    if (NULL == inPtr)
    {
        return -EINVAL;
    }

    else if (SPACE_ASCII_VALUE != *inPtr)
    {
        return -EINVAL;
    }

    while ((SPACE_ASCII_VALUE  == *inPtr) && ('\0' !=  *inPtr)) inPtr++;

    if ('\0' == *inPtr)
    {
        return 0;
    }

    sscanf(inPtr, "%31s ", buf);
    v = kstrtos32(buf, 10, &tempInt);
    if ( v < 0)
    {
       return -EINVAL;
    }

    if ( (tempInt < WNI_CFG_RMC_ACTION_PERIOD_FREQUENCY_STAMIN) ||
         (tempInt > WNI_CFG_RMC_ACTION_PERIOD_FREQUENCY_STAMAX) )
    {
       return -EINVAL;
    }

    *pActionPeriod = tempInt;

    VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
       "uActionPeriod: %d", *pActionPeriod);

    return 0;
}

/* Function header is left blank intentionally */
static int hdd_parse_setrmcrate_command(tANI_U8 *pValue,
           tANI_U32 *pRate, tTxrateinfoflags *pTxFlags)
{
    tANI_U8 *inPtr = pValue;
    int tempInt;
    int v = 0;
    char buf[32];
    *pRate = 0;
    *pTxFlags = 0;

    inPtr = strnchr(pValue, strlen(pValue), SPACE_ASCII_VALUE);

    if (NULL == inPtr)
    {
        return -EINVAL;
    }

    else if (SPACE_ASCII_VALUE != *inPtr)
    {
        return -EINVAL;
    }

    while ((SPACE_ASCII_VALUE  == *inPtr) && ('\0' !=  *inPtr)) inPtr++;

    if ('\0' == *inPtr)
    {
        return 0;
    }

    sscanf(inPtr, "%31s ", buf);
    v = kstrtos32(buf, 10, &tempInt);
    if ( v < 0)
    {
       return -EINVAL;
    }

    switch (tempInt)
    {
        default:
            VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_WARN,
            "Unsupported rate: %d", tempInt);
            return -EINVAL;
        case 0:
        case 6:
        case 9:
        case 12:
        case 18:
        case 24:
        case 36:
        case 48:
        case 54:
            *pTxFlags = eHAL_TX_RATE_LEGACY;
            *pRate = tempInt * 10;
            break;
        case 65:
            *pTxFlags = eHAL_TX_RATE_HT20;
            *pRate = tempInt * 10;
            break;
        case 72:
            *pTxFlags = eHAL_TX_RATE_HT20 | eHAL_TX_RATE_SGI;
            *pRate = 722;
            break;
    }

    VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
       "Rate: %d", *pRate);

    return 0;
}


/**
 * hdd_get_ibss_peer_info_cb() - IBSS peer Info request callback
 * @UserData: Adapter private data
 * @pPeerInfoRsp: Peer info response
 *
 * This is an asynchronous callback function from SME when the peer info
 * is received
 *
 * Return: 0 for success non-zero for failure
 */
void
hdd_get_ibss_peer_info_cb(v_VOID_t *pUserData,
                          tSirPeerInfoRspParams *pPeerInfo)
{
   hdd_adapter_t *pAdapter = (hdd_adapter_t *)pUserData;
   hdd_station_ctx_t *pStaCtx;
   v_U8_t   i;

   /*Sanity check*/
   if ((NULL == pAdapter) || (WLAN_HDD_ADAPTER_MAGIC != pAdapter->magic)) {
      VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_FATAL,
         "invalid adapter or adapter has invalid magic");
      return;
   }

   pStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);

   if (NULL != pPeerInfo && eHAL_STATUS_SUCCESS == pPeerInfo->status) {
      /* validate number of peers */
      if (pPeerInfo->numPeers > HDD_MAX_NUM_IBSS_STA) {
         hddLog(LOGW,
                FL("Limiting num_peers %u to %u"),
                pPeerInfo->numPeers, HDD_MAX_NUM_IBSS_STA);
         pPeerInfo->numPeers = HDD_MAX_NUM_IBSS_STA;
      }

      pStaCtx->ibss_peer_info.status = pPeerInfo->status;
      pStaCtx->ibss_peer_info.numPeers = pPeerInfo->numPeers;

      for (i = 0; i < pPeerInfo->numPeers; i++) {
          pStaCtx->ibss_peer_info.peerInfoParams[i] =
                                         pPeerInfo->peerInfoParams[i];
      }
   } else {
      hddLog(LOGE, FL("peerInfo %s: status %u, numPeers %u"),
                       pPeerInfo ? "valid" : "null",
                       pPeerInfo ? pPeerInfo->status : eHAL_STATUS_FAILURE,
                       pPeerInfo ? pPeerInfo->numPeers : 0);
      pStaCtx->ibss_peer_info.numPeers = 0;
      pStaCtx->ibss_peer_info.status = eHAL_STATUS_FAILURE;
   }

   complete(&pAdapter->ibss_peer_info_comp);
}

/**---------------------------------------------------------------------------

  \brief hdd_cfg80211_get_ibss_peer_info_all() -

  Request function to get IBSS peer info from lower layers

  \pAdapter -> Adapter context

  \return - 0 for success non-zero for failure
  --------------------------------------------------------------------------*/
static
VOS_STATUS hdd_cfg80211_get_ibss_peer_info_all(hdd_adapter_t *pAdapter)
{
   tHalHandle hHal = WLAN_HDD_GET_HAL_CTX(pAdapter);
   VOS_STATUS retStatus = VOS_STATUS_E_FAILURE;
   unsigned long rc;

   INIT_COMPLETION(pAdapter->ibss_peer_info_comp);

   retStatus = sme_RequestIBSSPeerInfo(hHal, pAdapter,
                                    hdd_get_ibss_peer_info_cb,
                                    VOS_TRUE, 0xFF);

   if (VOS_STATUS_SUCCESS == retStatus)
   {
      rc = wait_for_completion_timeout
               (&pAdapter->ibss_peer_info_comp,
                msecs_to_jiffies(IBSS_PEER_INFO_REQ_TIMOEUT));

      /* status will be 0 if timed out */
      if (!rc) {
          hddLog(VOS_TRACE_LEVEL_WARN, "%s: Warning: IBSS_PEER_INFO_TIMEOUT",
                 __func__);
          retStatus = VOS_STATUS_E_FAILURE;
          return retStatus;
      }
   }
   else
   {
      hddLog(VOS_TRACE_LEVEL_WARN,
             "%s: Warning: sme_RequestIBSSPeerInfo Request failed", __func__);
   }

   return retStatus;
}

/**---------------------------------------------------------------------------

  \brief hdd_cfg80211_get_ibss_peer_info() -

  Request function to get IBSS peer info from lower layers

  \pAdapter -> Adapter context
  \staIdx -> Sta index for which the peer info is requested

  \return - 0 for success non-zero for failure
  --------------------------------------------------------------------------*/
static VOS_STATUS
hdd_cfg80211_get_ibss_peer_info(hdd_adapter_t *pAdapter, v_U8_t staIdx)
{
    unsigned long rc;
    tHalHandle hHal = WLAN_HDD_GET_HAL_CTX(pAdapter);
    VOS_STATUS retStatus = VOS_STATUS_E_FAILURE;

    INIT_COMPLETION(pAdapter->ibss_peer_info_comp);

    retStatus = sme_RequestIBSSPeerInfo(hHal, pAdapter,
                                     hdd_get_ibss_peer_info_cb,
                                     VOS_FALSE, staIdx);

    if (VOS_STATUS_SUCCESS == retStatus)
    {
       rc = wait_for_completion_timeout
                (&pAdapter->ibss_peer_info_comp,
                msecs_to_jiffies(IBSS_PEER_INFO_REQ_TIMOEUT));

       /* status = 0 on timeout */
       if (!rc) {
          hddLog(VOS_TRACE_LEVEL_WARN, "%s: Warning: IBSS_PEER_INFO_TIMEOUT",
                 __func__);
          retStatus = VOS_STATUS_E_FAILURE;
          return retStatus;
       }
    }
    else
    {
       hddLog(VOS_TRACE_LEVEL_WARN,
              "%s: Warning: sme_RequestIBSSPeerInfo Request failed", __func__);
    }

    return retStatus;
}

/* Function header is left blank intentionally */
static VOS_STATUS
hdd_parse_get_ibss_peer_info(tANI_U8 *pValue, v_MACADDR_t *pPeerMacAddr)
{
    tANI_U8 *inPtr = pValue;
    size_t inPtrLen = strlen(pValue);

    inPtr = strnchr(pValue, inPtrLen, SPACE_ASCII_VALUE);

    if (NULL == inPtr)
    {
        return VOS_STATUS_E_FAILURE;;
    }

    else if (SPACE_ASCII_VALUE != *inPtr)
    {
        return VOS_STATUS_E_FAILURE;;
    }

    while ((SPACE_ASCII_VALUE  == *inPtr) && ('\0' !=  *inPtr) ) inPtr++;

    if ('\0' == *inPtr)
    {
        return VOS_STATUS_E_FAILURE;;
    }

    inPtrLen -= (inPtr - pValue);
    if (inPtrLen < 17)
    {
        return VOS_STATUS_E_FAILURE;
    }

    if (inPtr[2] != ':' || inPtr[5] != ':' || inPtr[8] != ':' ||
        inPtr[11] != ':' || inPtr[14] != ':')
    {
       return VOS_STATUS_E_FAILURE;;
    }
    sscanf(inPtr, "%2x:%2x:%2x:%2x:%2x:%2x",
                  (unsigned int *)&pPeerMacAddr->bytes[0],
                  (unsigned int *)&pPeerMacAddr->bytes[1],
                  (unsigned int *)&pPeerMacAddr->bytes[2],
                  (unsigned int *)&pPeerMacAddr->bytes[3],
                  (unsigned int *)&pPeerMacAddr->bytes[4],
                  (unsigned int *)&pPeerMacAddr->bytes[5]);

    return VOS_STATUS_SUCCESS;
}

#ifdef IPA_UC_STA_OFFLOAD
static void hdd_set_thermal_level_cb(void *pContext, u_int8_t level)
{
   hdd_context_t *pHddCtx = (hdd_context_t *)pContext;

   /* Change IPA to SW path when throttle level greater than 0 */
   if (level > THROTTLE_LEVEL_0)
      hdd_ipa_send_mcc_scc_msg(pHddCtx, TRUE);
   else
      /* restore original concurrency mode */
      hdd_ipa_send_mcc_scc_msg(pHddCtx, pHddCtx->mcc_mode);
}
#else
static void hdd_set_thermal_level_cb(void *pContext, u_int8_t level)
{
}
#endif

#ifdef FEATURE_WLAN_THERMAL_SHUTDOWN
static bool
hdd_system_suspend_state(hdd_context_t *hdd_ctx)
{
	bool s;

	adf_os_spin_lock_irqsave(&hdd_ctx->thermal_suspend_lock);
	s = hdd_ctx->system_suspended;
	adf_os_spin_unlock_irqrestore(&hdd_ctx->thermal_suspend_lock);
	return s;
}

/**
 * hdd_system_suspend_state_set() - Set the system suspended state
 * @hdd_ctx: Pointer to hdd_context_t
 * @state: The target state to be set
 *
 * Set the system suspended state
 *
 * Return: The state before set.
 */
bool
hdd_system_suspend_state_set(hdd_context_t *hdd_ctx, bool state)
{
	bool old;

	adf_os_spin_lock_irqsave(&hdd_ctx->thermal_suspend_lock);
	old = hdd_ctx->system_suspended;
	hdd_ctx->system_suspended = state;
	adf_os_spin_unlock_irqrestore(&hdd_ctx->thermal_suspend_lock);

	return old;
}

/**
 * hdd_thermal_suspend_state() - Get the thermal suspend state
 * @hdd_ctx: Pointer to hdd_context_t
 *
 * Get the thermal suspend state
 *
 * Return: The current thermal suspend state
 */
int
hdd_thermal_suspend_state(hdd_context_t *hdd_ctx)
{
	int s;

	adf_os_spin_lock_irqsave(&hdd_ctx->thermal_suspend_lock);
	s = hdd_ctx->thermal_suspend_state;
	adf_os_spin_unlock_irqrestore(&hdd_ctx->thermal_suspend_lock);

	return s;
}

static bool
hdd_thermal_suspend_transit(hdd_context_t *hdd_ctx, int target, int *old)
{
	int s;
	bool ret = false;

	adf_os_spin_lock_irqsave(&hdd_ctx->thermal_suspend_lock);

	s = hdd_ctx->thermal_suspend_state;
	if (old)
		*old = s;

	switch (target) {
	case HDD_WLAN_THERMAL_ACTIVE:
		if (s == HDD_WLAN_THERMAL_RESUMING ||
			s == HDD_WLAN_THERMAL_SUSPENDING)
			ret = true;
		break;
	case HDD_WLAN_THERMAL_SUSPENDING:
		if (s == HDD_WLAN_THERMAL_ACTIVE)
			ret = true;
		break;
	case HDD_WLAN_THERMAL_SUSPENDED:
		if (s == HDD_WLAN_THERMAL_SUSPENDING ||
			s == HDD_WLAN_THERMAL_RESUMING)
			ret = true;
		break;
	case HDD_WLAN_THERMAL_RESUMING:
		if (s == HDD_WLAN_THERMAL_SUSPENDED)
			ret = true;
		break;
	case HDD_WLAN_THERMAL_DEINIT:
		if (s != HDD_WLAN_THERMAL_DEINIT)
			ret = true;
		break;
	}

	if (ret)
		hdd_ctx->thermal_suspend_state = target;

	adf_os_spin_unlock_irqrestore(&hdd_ctx->thermal_suspend_lock);
	return ret;
}

static void
hdd_thermal_suspend_cleanup(hdd_context_t *hdd_ctx)
{
	int state;
	bool okay;

	if (!hdd_ctx->thermal_suspend_wq)
		return;

	cancel_delayed_work_sync(&hdd_ctx->thermal_suspend_work);
	okay = hdd_thermal_suspend_transit(hdd_ctx, HDD_WLAN_THERMAL_DEINIT,
					   &state);
	destroy_workqueue(hdd_ctx->thermal_suspend_wq);
	hdd_ctx->thermal_suspend_wq = NULL;

	if (!okay || state == HDD_WLAN_THERMAL_ACTIVE)
		return;

	if (state == HDD_WLAN_THERMAL_SUSPENDED) {
		hdd_prevent_suspend(WIFI_POWER_EVENT_WAKELOCK_THERMAL);
		__wlan_hdd_cfg80211_resume_wlan(hdd_ctx->wiphy, true);
		hdd_allow_suspend(WIFI_POWER_EVENT_WAKELOCK_THERMAL);
	}
}

static inline void
hdd_thermal_resume_complete_ind(struct wiphy *wiphy)
{
	struct sk_buff *vendor_event;

	hddLog(LOG1, FL("Thermal resume complete indication"));

	vendor_event = cfg80211_vendor_event_alloc(wiphy, NULL,
		sizeof(uint8_t) + NLMSG_HDRLEN,
		QCA_NL80211_VENDOR_SUBCMD_THERMAL_EVENT_INDEX,
		GFP_KERNEL);
	if (!vendor_event) {
		hddLog(LOGE, FL("cfg80211_vendor_event_alloc failed"));
		return;
	}

	nla_put_flag(vendor_event,
		QCA_WLAN_VENDOR_ATTR_THERMAL_EVENT_RESUME_COMPLETE);

	cfg80211_vendor_event(vendor_event, GFP_KERNEL);
}



static void
hdd_thermal_suspend_work(struct work_struct *work)
{
	hdd_context_t *hdd_ctx =
		container_of(work, hdd_context_t, thermal_suspend_work.work);
	struct wiphy *wiphy = hdd_ctx->wiphy;
	int ret;

	while (hdd_system_suspend_state(hdd_ctx)) {
		hddLog(LOG1, FL("Waiting for system resume complete"));
		schedule_timeout_interruptible(100 * HZ / 1000);
	}

	if (hdd_thermal_suspend_transit(hdd_ctx,
		HDD_WLAN_THERMAL_SUSPENDING, NULL)) {
		hdd_prevent_suspend(WIFI_POWER_EVENT_WAKELOCK_THERMAL);
		ret = __wlan_hdd_cfg80211_suspend_wlan(wiphy, NULL, true);
		hdd_allow_suspend(WIFI_POWER_EVENT_WAKELOCK_THERMAL);
		if (ret) {
			hdd_thermal_suspend_transit(hdd_ctx,
				HDD_WLAN_THERMAL_ACTIVE, NULL);
			hddLog(LOGE, FL("Thermal suspend failed: %d"), ret);
			return;
		}
		hdd_thermal_suspend_transit(hdd_ctx, HDD_WLAN_THERMAL_SUSPENDED,
						NULL);
	} else if (hdd_thermal_suspend_transit(hdd_ctx,
		   HDD_WLAN_THERMAL_RESUMING, NULL)) {
		hdd_prevent_suspend(WIFI_POWER_EVENT_WAKELOCK_THERMAL);
		ret = __wlan_hdd_cfg80211_resume_wlan(wiphy, true);
		hdd_allow_suspend(WIFI_POWER_EVENT_WAKELOCK_THERMAL);
		if (ret) {
			hdd_thermal_suspend_transit(hdd_ctx,
				HDD_WLAN_THERMAL_SUSPENDED, NULL);
			hddLog(LOGE, FL("Thermal resume failed: %d"), ret);
			return;
		}
		hdd_thermal_suspend_transit(hdd_ctx, HDD_WLAN_THERMAL_ACTIVE,
						NULL);
	} else {
		hddLog(LOGE, FL("Should not reach here"));
	}
}

/**
 * hdd_thermal_suspend_queue_work() - Queue a thermal suspend work
 * @hdd_ctx: Pointer to hdd_context_t
 * @ms: Delay time in milliseconds to execute the work
 *
 * Queue thermal suspend work on the workqueue after delay
 *
 * Return: false if work was already on a queue, true otherwise.
 */
bool
hdd_thermal_suspend_queue_work(hdd_context_t *hdd_ctx, unsigned long ms)
{
	hddLog(LOG1, FL("Queue a thermal suspend work, delay %ld ms"), ms);
	return queue_delayed_work(hdd_ctx->thermal_suspend_wq,
		&hdd_ctx->thermal_suspend_work, (ms * HZ) / 1000);
}

static void
hdd_thermal_temp_ind_event_cb(hdd_context_t *hdd_ctx, uint32_t degreeC)
{
	struct sk_buff *vendor_event;

	vendor_event = cfg80211_vendor_event_alloc(hdd_ctx->wiphy,
			NULL, sizeof(uint32_t) + NLMSG_HDRLEN,
			QCA_NL80211_VENDOR_SUBCMD_THERMAL_EVENT_INDEX,
			GFP_KERNEL);
	if (!vendor_event) {
		hddLog(LOGE, FL("cfg80211_vendor_event_alloc failed"));
		return;
	}

	if (nla_put_u32(vendor_event,
		QCA_WLAN_VENDOR_ATTR_THERMAL_EVENT_TEMPERATURE, degreeC)) {
		hddLog(LOGE, FL("nla put failed"));
		kfree_skb(vendor_event);
		return;
	}

	cfg80211_vendor_event(vendor_event, GFP_KERNEL);

	if (!hdd_ctx->cfg_ini->thermal_shutdown_auto_enabled) {
		return;
	}

	/*
	 * Here, we only do thermal suspend.
	 *
	 * We can only resume FW elsewhere in two ways:
	 * 1. triggered by host when it detects FW reported T is less than Tr
	 * 2. user space app launch thermal resume after suspend as app wants
	 *
	 */
	if ((hdd_thermal_suspend_state(hdd_ctx) == HDD_WLAN_THERMAL_ACTIVE &&
		degreeC >= hdd_ctx->cfg_ini->thermal_suspend_threshold) ||
		(hdd_thermal_suspend_state(hdd_ctx) == HDD_WLAN_THERMAL_SUSPENDED &&
		degreeC < hdd_ctx->cfg_ini->thermal_resume_threshold)) {
		hdd_thermal_suspend_queue_work(hdd_ctx, 0);
	}
}
#else
bool
hdd_system_suspend_state_set(hdd_context_t *hdd_ctx, bool state)
{
	return TRUE;
}

static inline void
hdd_thermal_suspend_cleanup(hdd_context_t *hdd_ctx)
{
	return;
}

static inline void
hdd_thermal_temp_ind_event_cb(hdd_context_t *hdd_ctx, uint32_t degreeC)
{
	return;
}

#endif

/**---------------------------------------------------------------------------

  \brief hdd_setIbssPowerSaveParams - update IBSS Power Save params to WMA.

  This function sets the IBSS power save config parameters to WMA
  which will send it to firmware if FW supports IBSS power save
  before vdev start.

  \param  - hdd_adapter_t Hdd adapter.

  \return - VOS_STATUS VOS_STATUS_SUCCESS on Success and VOS_STATUS_E_FAILURE
            on failure.

  --------------------------------------------------------------------------*/
VOS_STATUS hdd_setIbssPowerSaveParams(hdd_adapter_t *pAdapter)
{
    int ret;
    hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX( pAdapter );

    if (pHddCtx == NULL) {
        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                   "%s: HDD context is null", __func__);
        return VOS_STATUS_E_FAILURE;
    }

    ret = process_wma_set_command(pAdapter->sessionId,
                             WMA_VDEV_IBSS_SET_ATIM_WINDOW_SIZE,
                             pHddCtx->cfg_ini->ibssATIMWinSize,
                             VDEV_CMD);
    if (0 != ret) {
        hddLog(VOS_TRACE_LEVEL_ERROR,
               FL("WMA_VDEV_IBSS_SET_ATIM_WINDOW_SIZE failed %d"),
                   ret);
        return VOS_STATUS_E_FAILURE;
    }

    ret = process_wma_set_command(pAdapter->sessionId,
                             WMA_VDEV_IBSS_SET_POWER_SAVE_ALLOWED,
                             pHddCtx->cfg_ini->isIbssPowerSaveAllowed,
                             VDEV_CMD);
    if (0 != ret) {
        hddLog(VOS_TRACE_LEVEL_ERROR,
               FL("WMA_VDEV_IBSS_SET_POWER_SAVE_ALLOWED failed %d"),
                   ret);
        return VOS_STATUS_E_FAILURE;
    }

    ret = process_wma_set_command(pAdapter->sessionId,
                             WMA_VDEV_IBSS_SET_POWER_COLLAPSE_ALLOWED,
                             pHddCtx->cfg_ini->isIbssPowerCollapseAllowed,
                             VDEV_CMD);
    if (0 != ret) {
        hddLog(VOS_TRACE_LEVEL_ERROR,
               FL("WMA_VDEV_IBSS_SET_POWER_COLLAPSE_ALLOWED failed %d"),
                   ret);
        return VOS_STATUS_E_FAILURE;
    }

    ret = process_wma_set_command(pAdapter->sessionId,
                             WMA_VDEV_IBSS_SET_AWAKE_ON_TX_RX,
                             pHddCtx->cfg_ini->isIbssAwakeOnTxRx,
                             VDEV_CMD);
    if (0 != ret) {
        hddLog(VOS_TRACE_LEVEL_ERROR,
               FL("WMA_VDEV_IBSS_SET_AWAKE_ON_TX_RX failed %d"),
                   ret);
        return VOS_STATUS_E_FAILURE;
    }

    ret = process_wma_set_command(pAdapter->sessionId,
                             WMA_VDEV_IBSS_SET_INACTIVITY_TIME,
                             pHddCtx->cfg_ini->ibssInactivityCount,
                             VDEV_CMD);
    if (0 != ret) {
        hddLog(VOS_TRACE_LEVEL_ERROR,
               FL("WMA_VDEV_IBSS_SET_INACTIVITY_TIME failed %d"),
                   ret);
        return VOS_STATUS_E_FAILURE;
    }

    ret = process_wma_set_command(pAdapter->sessionId,
                             WMA_VDEV_IBSS_SET_TXSP_END_INACTIVITY_TIME,
                             pHddCtx->cfg_ini->ibssTxSpEndInactivityTime,
                             VDEV_CMD);
    if (0 != ret) {
        hddLog(VOS_TRACE_LEVEL_ERROR,
               FL("WMA_VDEV_IBSS_SET_TXSP_END_INACTIVITY_TIME failed %d"),
                   ret);
        return VOS_STATUS_E_FAILURE;
    }

    ret = process_wma_set_command(pAdapter->sessionId,
                             WMA_VDEV_IBSS_PS_SET_WARMUP_TIME_SECS,
                             pHddCtx->cfg_ini->ibssPsWarmupTime,
                             VDEV_CMD);
    if (0 != ret) {
        hddLog(VOS_TRACE_LEVEL_ERROR,
               FL("WMA_VDEV_IBSS_PS_SET_WARMUP_TIME_SECS failed %d"),
                   ret);
        return VOS_STATUS_E_FAILURE;
    }

    ret = process_wma_set_command(pAdapter->sessionId,
                            WMA_VDEV_IBSS_PS_SET_1RX_CHAIN_IN_ATIM_WINDOW,
                            pHddCtx->cfg_ini->ibssPs1RxChainInAtimEnable,
                            VDEV_CMD);
    if (0 != ret) {
        hddLog(VOS_TRACE_LEVEL_ERROR,
               FL("WMA_VDEV_IBSS_PS_SET_1RX_CHAIN_IN_ATIM_WINDOW failed %d"),
                   ret);
        return VOS_STATUS_E_FAILURE;
    }

    return VOS_STATUS_SUCCESS;
}

#if  defined (WLAN_FEATURE_VOWIFI_11R) || defined (FEATURE_WLAN_ESE) || defined(FEATURE_WLAN_LFR)
/**
 * hdd_parse_reassoc_command_data() - HDD Parse reassoc command data
 * @pValue:     Pointer to input data (its a NUL terminated string)
 * @pTargetApBssid: Pointer to target Ap bssid
 * @pChannel:     Pointer to the Target AP channel
 *
 * This function parses the reasoc command data passed in the format
 * REASSOC<space><bssid><space><channel>
 *
 * Return: 0 for success non-zero for failure
 */
static int hdd_parse_reassoc_command_v1_data(const tANI_U8 *pValue,
                                             tANI_U8 *pTargetApBssid,
                                             tANI_U8 *pChannel)
{
    const tANI_U8 *inPtr = pValue;
    int tempInt;
    int v = 0;
    tANI_U8 tempBuf[32];
    /* 12 hexa decimal digits, 5 ':' and '\0' */
    tANI_U8 macAddress[18];


    inPtr = strnchr(pValue, strlen(pValue), SPACE_ASCII_VALUE);
    /* no argument after the command */
    if (NULL == inPtr) {
        return -EINVAL;
    } else if (SPACE_ASCII_VALUE != *inPtr) {
        /*no space after the command*/
        return -EINVAL;
    }

    /* Removing empty spaces */
    while ((SPACE_ASCII_VALUE  == *inPtr) && ('\0' !=  *inPtr) ) inPtr++;

    /* no argument followed by spaces */
    if ('\0' == *inPtr) {
        return -EINVAL;
    }

    v = sscanf(inPtr, "%17s", macAddress);
    if (!((1 == v) && hdd_is_valid_mac_address(macAddress))) {
        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
             "Invalid MAC address or All hex inputs are not read (%d)", v);
        return -EINVAL;
    }

    pTargetApBssid[0] = hdd_parse_hex(macAddress[0]) << 4 |
                        hdd_parse_hex(macAddress[1]);
    pTargetApBssid[1] = hdd_parse_hex(macAddress[3]) << 4 |
                        hdd_parse_hex(macAddress[4]);
    pTargetApBssid[2] = hdd_parse_hex(macAddress[6]) << 4 |
                        hdd_parse_hex(macAddress[7]);
    pTargetApBssid[3] = hdd_parse_hex(macAddress[9]) << 4 |
                        hdd_parse_hex(macAddress[10]);
    pTargetApBssid[4] = hdd_parse_hex(macAddress[12]) << 4 |
                        hdd_parse_hex(macAddress[13]);
    pTargetApBssid[5] = hdd_parse_hex(macAddress[15]) << 4 |
                        hdd_parse_hex(macAddress[16]);

    /* point to the next argument */
    inPtr = strnchr(inPtr, strlen(inPtr), SPACE_ASCII_VALUE);
    /* no argument after the command */
    if (NULL == inPtr) return -EINVAL;

    /* Removing empty spaces */
    while ((SPACE_ASCII_VALUE  == *inPtr) && ('\0' !=  *inPtr) ) inPtr++;

    /*no argument followed by spaces*/
    if ('\0' == *inPtr) {
        return -EINVAL;
    }

    /*getting the next argument ie the channel number */
    v = sscanf(inPtr, "%31s ", tempBuf);
    if (1 != v) return -EINVAL;

    /*
     * Sigma DUT sends connected bssid and channel 0 to indicate
     * driver to issue reassoc to same AP.
     * Hence do not treat channel 0 as invalid.
     */
    v = kstrtos32(tempBuf, 10, &tempInt);
    if ((v < 0) ||
        (tempInt < 0) ||
        (tempInt > WNI_CFG_CURRENT_CHANNEL_STAMAX)) {
        return -EINVAL;
    }

    *pChannel = tempInt;
    return VOS_STATUS_SUCCESS;
}

/**
 * hdd_reassoc() - perform a user space directed reassoc
 * @pAdapter: Adapter upon which the command was received
 * @bssid:    BSSID with which to reassociate
 * @channel:  channel upon which to reassociate
 * @src:      The source for the trigger of this action
 *
 * Return:    0 for success non-zero for failure
 */
int hdd_reassoc(hdd_adapter_t *pAdapter, const tANI_U8 *bssid,
                const tANI_U8 channel, const handoff_src src)
{
   hdd_station_ctx_t *pHddStaCtx;
   int ret = 0;

   pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);

   /* if not associated, no need to proceed with reassoc */
   if (eConnectionState_Associated != pHddStaCtx->conn_info.connState) {
      hddLog(VOS_TRACE_LEVEL_INFO, "%s: Not associated", __func__);
      ret = -EINVAL;
      goto exit;
   }

   /* if the target bssid is same as currently associated AP,
      then no need to proceed with reassoc */
   if (!memcmp(bssid, pHddStaCtx->conn_info.bssId, sizeof(tSirMacAddr))) {
      hddLog(VOS_TRACE_LEVEL_INFO,
             "%s: Reassoc BSSID is same as currently associated AP bssid",
             __func__);
      ret = -EINVAL;
      goto exit;
   }

   /* Check channel number is a valid channel number */
   if (VOS_STATUS_SUCCESS !=
       wlan_hdd_validate_operation_channel(pAdapter, channel)) {
      hddLog(VOS_TRACE_LEVEL_ERROR, "%s: Invalid Channel %d",
             __func__, channel);
      ret = -EINVAL;
      goto exit;
   }

   /* Proceed with reassoc */
#ifdef WLAN_FEATURE_ROAM_SCAN_OFFLOAD
   {
      tCsrHandoffRequest handoffInfo;
      hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);

      handoffInfo.channel = channel;
      handoffInfo.src = src;
      memcpy(handoffInfo.bssid, bssid, sizeof(tSirMacAddr));
      sme_HandoffRequest(pHddCtx->hHal, pAdapter->sessionId, &handoffInfo);
   }
#endif
 exit:
   return ret;
}

/*
  \brief hdd_parse_reassoc_v1() - parse version 1 of the REASSOC command

  This function parses the v1 REASSOC command with the format

      REASSOC xx:xx:xx:xx:xx:xx CH

  Where "xx:xx:xx:xx:xx:xx" is the Hex-ASCII representation of the
  BSSID and CH is the ASCII representation of the channel.  For
  example

      REASSOC 00:0a:0b:11:22:33 48

  \param - pAdapter - Adapter upon which the command was received
  \param - command - ASCII text command that was received

  \return - 0 for success non-zero for failure

  --------------------------------------------------------------------------*/
static int
hdd_parse_reassoc_v1(hdd_adapter_t *pAdapter, const char *command)
{
   tANI_U8 channel = 0;
   tSirMacAddr bssid;
   int ret;

   ret = hdd_parse_reassoc_command_v1_data(command, bssid, &channel);
   if (ret)
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,
             "%s: Failed to parse reassoc command data", __func__);
   } else {
      ret = hdd_reassoc(pAdapter, bssid, channel, REASSOC);
   }
   return ret;
}

/*
  \brief hdd_parse_reassoc_v2() - parse version 2 of the REASSOC command

  This function parses the v2 REASSOC command with the format

      REASSOC <android_wifi_reassoc_params>

  \param - pAdapter - Adapter upon which the command was received
  \param - command - command that was received, ASCII command followed
                     by binary data
  \param - total_len - total length of the command received

  \return - 0 for success non-zero for failure

  --------------------------------------------------------------------------*/
static int
hdd_parse_reassoc_v2(hdd_adapter_t *pAdapter,
                     const char *command,
                     int total_len)
{
   struct android_wifi_reassoc_params params;
   tSirMacAddr bssid;
   int ret;

   if (total_len < sizeof(params) + 8) {
      hddLog(LOGE, FL("Invalid command length"));
      return -EINVAL;
   }

   /* The params are located after "REASSOC " */
   memcpy(&params, command + 8, sizeof(params));

   if (!mac_pton(params.bssid, (u8 *)&bssid)) {
      hddLog(LOGE, "%s: MAC address parsing failed", __func__);
      ret = -EINVAL;
   } else {
      ret = hdd_reassoc(pAdapter, bssid, params.channel, REASSOC);
   }
   return ret;
}

/*
  \brief hdd_parse_reassoc() - parse the REASSOC command

  There are two different versions of the REASSOC command.  Version 1
  of the command contains a parameter list that is ASCII characters
  whereas version 2 contains a combination of ASCII and binary
  payload.  Determine if a version 1 or a version 2 command is being
  parsed by examining the parameters, and then dispatch the parser
  that is appropriate for the command.

  \param - pAdapter - Adapter upon which the command was received
  \param - command - command that was received
  \param - total_len - total length of the command received

  \return - 0 for success non-zero for failure

  --------------------------------------------------------------------------*/
static int
hdd_parse_reassoc(hdd_adapter_t *pAdapter, const char *command, int total_len)
{
   int ret;

   /* both versions start with "REASSOC "
    * v1 has a bssid and channel # as an ASCII string
    *    REASSOC xx:xx:xx:xx:xx:xx CH
    * v2 has a C struct
    *    REASSOC <binary c struct>
    *
    * The first field in the v2 struct is also the bssid in ASCII.
    * But in the case of a v2 message the BSSID is NUL-terminated.
    * Hence we can peek at that offset to see if this is V1 or V2
    * REASSOC xx:xx:xx:xx:xx:xx*
    *           1111111111222222
    * 01234567890123456789012345
    */

   if (total_len < 26) {
      hddLog(LOGE, FL("Invalid command (total_len=%d)"), total_len);
      return -EINVAL;
   }

   if (command[25]) {
      ret = hdd_parse_reassoc_v1(pAdapter, command);
   } else {
      ret = hdd_parse_reassoc_v2(pAdapter, command, total_len);
   }

   return ret;
}

/**
 * hdd_parse_send_action_frame_v1_data() - HDD Parse send action frame data
 * @pValue:     Pointer to input data (its a NUL terminated string)
 * @pTargetApBssid: Pointer to target Ap bssid
 * @pChannel:       Pointer to the Target AP channel
 * @pDwellTime:     pDwellTime Pointer to the time to stay off-channel
 *                  after transmitting action frame
 * @pBuf:           Pointer to data
 * @pBufLen:        Pointer to data length
 *
 * This function parses the send action frame data passed in the format
 * SENDACTIONFRAME<space><bssid><space><channel><space><dwelltime><space><data>
 *
 * Return: 0 for success non-zero for failure
 */
static int
hdd_parse_send_action_frame_v1_data(const tANI_U8 *pValue,
                                    tANI_U8 *pTargetApBssid,
                                    tANI_U8 *pChannel, tANI_U8 *pDwellTime,
                                    tANI_U8 **pBuf, tANI_U8 *pBufLen)
{
    const tANI_U8 *inPtr = pValue;
    const tANI_U8 *dataEnd;
    int tempInt;
    int j = 0;
    int i = 0;
    int v = 0;
    tANI_U8 tempBuf[32];
    tANI_U8 tempByte = 0;
    /* 12 hexa decimal digits, 5 ':' and '\0' */
    tANI_U8 macAddress[18];

    inPtr = strnchr(pValue, strlen(pValue), SPACE_ASCII_VALUE);
    /* no argument after the command */
    if (NULL == inPtr) {
        return -EINVAL;
    } else if (SPACE_ASCII_VALUE != *inPtr) {
        /* no space after the command */
        return -EINVAL;
    }

    /* removing empty spaces */
    while ((SPACE_ASCII_VALUE  == *inPtr) && ('\0' !=  *inPtr) ) inPtr++;

    /*no argument followed by spaces*/
    if ('\0' == *inPtr) {
        return -EINVAL;
    }

    v = sscanf(inPtr, "%17s", macAddress);
    if (!((1 == v) && hdd_is_valid_mac_address(macAddress))) {
        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
             "Invalid MAC address or All hex inputs are not read (%d)", v);
        return -EINVAL;
    }

    pTargetApBssid[0] = hdd_parse_hex(macAddress[0]) << 4 |
                        hdd_parse_hex(macAddress[1]);
    pTargetApBssid[1] = hdd_parse_hex(macAddress[3]) << 4 |
                        hdd_parse_hex(macAddress[4]);
    pTargetApBssid[2] = hdd_parse_hex(macAddress[6]) << 4 |
                        hdd_parse_hex(macAddress[7]);
    pTargetApBssid[3] = hdd_parse_hex(macAddress[9]) << 4 |
                        hdd_parse_hex(macAddress[10]);
    pTargetApBssid[4] = hdd_parse_hex(macAddress[12]) << 4 |
                        hdd_parse_hex(macAddress[13]);
    pTargetApBssid[5] = hdd_parse_hex(macAddress[15]) << 4 |
                        hdd_parse_hex(macAddress[16]);

    /* point to the next argument */
    inPtr = strnchr(inPtr, strlen(inPtr), SPACE_ASCII_VALUE);
    /* no argument after the command */
    if (NULL == inPtr) return -EINVAL;

    /* removing empty spaces */
    while ((SPACE_ASCII_VALUE  == *inPtr) && ('\0' !=  *inPtr) ) inPtr++;

    /* no argument followed by spaces */
    if ('\0' == *inPtr) {
        return -EINVAL;
    }

    /* getting the next argument ie the channel number */
    v = sscanf(inPtr, "%31s ", tempBuf);
    if (1 != v) return -EINVAL;

    v = kstrtos32(tempBuf, 10, &tempInt);
    if (v < 0 || tempInt < 0 || tempInt > WNI_CFG_CURRENT_CHANNEL_STAMAX)
     return -EINVAL;

    *pChannel = tempInt;

    /* point to the next argument */
    inPtr = strnchr(inPtr, strlen(inPtr), SPACE_ASCII_VALUE);
    /* no argument after the command */
    if (NULL == inPtr) return -EINVAL;
    /* removing empty spaces */
    while ((SPACE_ASCII_VALUE  == *inPtr) && ('\0' !=  *inPtr) ) inPtr++;

    /* no argument followed by spaces */
    if ('\0' == *inPtr) {
        return -EINVAL;
    }

    /* Getting the next argument ie the dwell time */
    v = sscanf(inPtr, "%31s ", tempBuf);
    if (1 != v) return -EINVAL;

    v = kstrtos32(tempBuf, 10, &tempInt);
    if ( v < 0 || tempInt < 0) return -EINVAL;

    *pDwellTime = tempInt;

    /* point to the next argument */
    inPtr = strnchr(inPtr, strlen(inPtr), SPACE_ASCII_VALUE);
    /* no argument after the command */
    if (NULL == inPtr) return -EINVAL;
    /* removing empty spaces */
    while ((SPACE_ASCII_VALUE  == *inPtr) && ('\0' !=  *inPtr) ) inPtr++;

    /* no argument followed by spaces */
    if ('\0' == *inPtr) {
        return -EINVAL;
    }

    /* find the length of data */
    dataEnd = inPtr;
    while('\0' !=  *dataEnd) {
        dataEnd++;
    }
    *pBufLen = dataEnd - inPtr ;
    if (*pBufLen <= 0)  return -EINVAL;

    /* Allocate the number of bytes based on the number of input characters
       whether it is even or odd.
       if the number of input characters are even, then we need N/2 byte.
       if the number of input characters are odd, then we need do (N+1)/2 to
       compensate rounding off.
       For example, if N = 18, then (18 + 1)/2 = 9 bytes are enough.
       If N = 19, then we need 10 bytes, hence (19 + 1)/2 = 10 bytes */
    *pBuf = vos_mem_malloc((*pBufLen + 1)/2);
    if (NULL == *pBuf) {
        hddLog(VOS_TRACE_LEVEL_FATAL,
           "%s: vos_mem_alloc failed ", __func__);
        return -EINVAL;
    }

    /* the buffer received from the upper layer is character buffer,
       we need to prepare the buffer taking 2 characters in to a U8 hex decimal number
       for example 7f0000f0...form a buffer to contain 7f in 0th location, 00 in 1st
       and f0 in 3rd location */
    for (i = 0, j = 0; j < *pBufLen; j += 2) {
        if( j+1 == *pBufLen) {
             tempByte = hdd_parse_hex(inPtr[j]);
        } else {
              tempByte = (hdd_parse_hex(inPtr[j]) << 4) | (hdd_parse_hex(inPtr[j + 1]));
        }
        (*pBuf)[i++] = tempByte;
    }
    *pBufLen = i;
    return 0;
}

/*
  \brief hdd_sendactionframe() - send a user space supplied action frame

  \param - pAdapter - Adapter upon which the command was received
  \param - bssid - BSSID target of the action frame
  \param - channel - channel upon which to send the frame
  \param - dwell_time - amount of time to dwell when the frame is sent
  \param - payload_len - length of the payload
  \param - payload - payload of the frame

  \return - 0 for success non-zero for failure

  --------------------------------------------------------------------------*/
static int
hdd_sendactionframe(hdd_adapter_t *pAdapter, const tANI_U8 *bssid,
                    const tANI_U8 channel, const tANI_U8 dwell_time,
                    const int payload_len, const tANI_U8 *payload)
{
   struct ieee80211_channel chan;
   int frame_len, ret = 0;
   tANI_U8 *frame;
   struct ieee80211_hdr_3addr *hdr;
   u64 cookie;
   hdd_station_ctx_t *pHddStaCtx;
   hdd_context_t *pHddCtx;
   tpSirMacVendorSpecificFrameHdr pVendorSpecific =
                   (tpSirMacVendorSpecificFrameHdr) payload;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)) || defined(WITH_BACKPORTS)
   struct cfg80211_mgmt_tx_params params;
#endif
   pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
   pHddCtx = WLAN_HDD_GET_CTX(pAdapter);

   /* if not associated, no need to send action frame */
   if (eConnectionState_Associated != pHddStaCtx->conn_info.connState) {
      hddLog(VOS_TRACE_LEVEL_INFO, "%s: Not associated", __func__);
      ret = -EINVAL;
      goto exit;
   }

   /* if the target bssid is different from currently associated AP,
      then no need to send action frame */
   if (memcmp(bssid, pHddStaCtx->conn_info.bssId, VOS_MAC_ADDR_SIZE)) {
      hddLog(VOS_TRACE_LEVEL_INFO, "%s: STA is not associated to this AP",
             __func__);
      ret = -EINVAL;
      goto exit;
   }

   chan.center_freq = sme_ChnToFreq(channel);
#ifdef WLAN_FEATURE_ROAM_SCAN_OFFLOAD
   /* Check if it is specific action frame */
   if (pVendorSpecific->category == SIR_MAC_ACTION_VENDOR_SPECIFIC_CATEGORY) {
       static const tANI_U8 Oui[] = { 0x00, 0x00, 0xf0 };
       if (vos_mem_compare(pVendorSpecific->Oui, (void *) Oui, 3)) {
           /* if the channel number is different from operating channel then
              no need to send action frame */
           if (channel != 0) {
               if (channel != pHddStaCtx->conn_info.operationChannel) {
                   hddLog(VOS_TRACE_LEVEL_INFO,
                     "%s: channel(%d) is different from operating channel(%d)",
                     __func__, channel,
                     pHddStaCtx->conn_info.operationChannel);
                   ret = -EINVAL;
                   goto exit;
               }
               /* If channel number is specified and same as home channel,
                * ensure that action frame is sent immediately by cancelling
                * roaming scans. Otherwise large dwell times may cause long
                * delays in sending action frames.
                */
               sme_abortRoamScan(pHddCtx->hHal, pAdapter->sessionId);
           } else {
               /* 0 is accepted as current home channel, delayed
                * transmission of action frame is ok.
                */
               chan.center_freq =
                       sme_ChnToFreq(pHddStaCtx->conn_info.operationChannel);
           }
       }
   }
#endif //#if WLAN_FEATURE_ROAM_SCAN_OFFLOAD
   if (chan.center_freq == 0) {
      hddLog(VOS_TRACE_LEVEL_ERROR, "%s:invalid channel number %d",
              __func__, channel);
      ret = -EINVAL;
      goto exit;
   }

   frame_len = payload_len + 24;
   frame = vos_mem_malloc(frame_len);
   if (!frame) {
      hddLog(VOS_TRACE_LEVEL_ERROR, "%s:memory allocation failed", __func__);
      ret = -ENOMEM;
      goto exit;
   }
   vos_mem_zero(frame, frame_len);

   hdr = (struct ieee80211_hdr_3addr *)frame;
   hdr->frame_control =
      cpu_to_le16(IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_ACTION);
   vos_mem_copy(hdr->addr1, bssid, VOS_MAC_ADDR_SIZE);
   vos_mem_copy(hdr->addr2, pAdapter->macAddressCurrent.bytes,
                VOS_MAC_ADDR_SIZE);
   vos_mem_copy(hdr->addr3, bssid, VOS_MAC_ADDR_SIZE);
   vos_mem_copy(hdr + 1, payload, payload_len);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)) || defined(WITH_BACKPORTS)
   params.chan = &chan;
   params.offchan = 0;
   params.wait = dwell_time;
   params.buf = frame;
   params.len = frame_len;
   params.no_cck = 1;
   params.dont_wait_for_ack = 1;
   ret = wlan_hdd_mgmt_tx(NULL, &pAdapter->wdev, &params, &cookie);
#else
   ret = wlan_hdd_mgmt_tx(NULL,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)) || defined(WITH_BACKPORTS)
                         &(pAdapter->wdev),
#else
                         pAdapter->dev,
#endif
                         &chan, 0,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,8,0)) && !defined(WITH_BACKPORTS)
                         NL80211_CHAN_HT20, 1,
#endif
                         dwell_time, frame, frame_len, 1, 1, &cookie );
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)*/

   vos_mem_free(frame);
 exit:
   return ret;
}

/*
  \brief hdd_parse_sendactionframe_v1() - parse version 1 of the
         SENDACTIONFRAME command

  This function parses the v1 SENDACTIONFRAME command with the format

      SENDACTIONFRAME xx:xx:xx:xx:xx:xx CH DW xxxxxx

  Where "xx:xx:xx:xx:xx:xx" is the Hex-ASCII representation of the
  BSSID, CH is the ASCII representation of the channel, DW is the
  ASCII representation of the dwell time, and xxxxxx is the Hex-ASCII
  payload.  For example

      SENDACTIONFRAME 00:0a:0b:11:22:33 48 40 aabbccddee

  \param - pAdapter - Adapter upon which the command was received
  \param - command - ASCII text command that was received

  \return - 0 for success non-zero for failure

  --------------------------------------------------------------------------*/
static int
hdd_parse_sendactionframe_v1(hdd_adapter_t *pAdapter, const char *command)
{
   tANI_U8 channel = 0;
   tANI_U8 dwell_time = 0;
   tANI_U8 payload_len = 0;
   tANI_U8 *payload = NULL;
   tSirMacAddr bssid;
   int ret;

   ret = hdd_parse_send_action_frame_v1_data(command, bssid, &channel,
                                             &dwell_time, &payload,
                                             &payload_len);
   if (ret) {
      hddLog(VOS_TRACE_LEVEL_ERROR,
             "%s: Failed to parse send action frame data", __func__);
   } else {
      ret = hdd_sendactionframe(pAdapter, bssid, channel, dwell_time,
                                payload_len, payload);
      vos_mem_free(payload);
   }

   return ret;
}

/**
 * hdd_parse_sendactionframe_v2() - parse version 2 of the
 *                                  SENDACTIONFRAME command
 * @pAdapter: Adapter upon which the command was received
 * @command: command that was received, ASCII command followed
 *           by binary data
 * @total_len: total length of command
 *
 * This function parses the v2 SENDACTIONFRAME command with the format
 * SENDACTIONFRAME <android_wifi_af_params>
 *
 * Return: 0 for success non-zero for failure
 */
static int
hdd_parse_sendactionframe_v2(hdd_adapter_t *pAdapter,
                             const char *command, int total_len)
{
	struct android_wifi_af_params *params;
	tSirMacAddr bssid;
	int ret;
	int len_wo_payload = 0;

	/* The params are located after "SENDACTIONFRAME " */
	total_len -= 16;
	len_wo_payload = sizeof(*params) - ANDROID_WIFI_ACTION_FRAME_SIZE;
	if (total_len <= len_wo_payload) {
		hddLog(LOGE, FL("Invalid command len"));
		return -EINVAL;
	}

	params = (struct android_wifi_af_params *)(command + 16);

	if (params->len <= 0 || params->len > ANDROID_WIFI_ACTION_FRAME_SIZE ||
            (params->len > (total_len - len_wo_payload))) {
		hddLog(LOGE, FL("Invalid payload length: %d"), params->len);
		return -EINVAL;
	}

	if (!mac_pton(params->bssid, (u8 *)&bssid)) {
		hddLog(LOGE, FL("MAC address parsing failed"));
		return -EINVAL;
	}

	if (params->channel < 0 ||
	    params->channel > WNI_CFG_CURRENT_CHANNEL_STAMAX) {
		hddLog(LOGE, FL("Invalid channel: %d"), params->channel);
		return -EINVAL;
	}

	if (params->dwell_time < 0) {
		hddLog(LOGE, FL("Invalid dwell_time: %d"), params->dwell_time);
		return -EINVAL;
	}

	ret = hdd_sendactionframe(pAdapter, bssid, params->channel,
				params->dwell_time, params->len, params->data);

	return ret;
}

/*
  \brief hdd_parse_sendactionframe() - parse the SENDACTIONFRAME command

  There are two different versions of the SENDACTIONFRAME command.
  Version 1 of the command contains a parameter list that is ASCII
  characters whereas version 2 contains a combination of ASCII and
  binary payload.  Determine if a version 1 or a version 2 command is
  being parsed by examining the parameters, and then dispatch the
  parser that is appropriate for the version of the command.

  \param - pAdapter - Adapter upon which the command was received
  \param - command - command that was received

  \return - 0 for success non-zero for failure

  --------------------------------------------------------------------------*/
static int
hdd_parse_sendactionframe(hdd_adapter_t *pAdapter, const char *command,
                          int total_len)
{
   int ret;

   /* both versions start with "SENDACTIONFRAME "
    * v1 has a bssid and other parameters as an ASCII string
    *    SENDACTIONFRAME xx:xx:xx:xx:xx:xx CH DWELL LEN FRAME
    * v2 has a C struct
    *    SENDACTIONFRAME <binary c struct>
    *
    * The first field in the v2 struct is also the bssid in ASCII.
    * But in the case of a v2 message the BSSID is NUL-terminated.
    * Hence we can peek at that offset to see if this is V1 or V2
    * SENDACTIONFRAME xx:xx:xx:xx:xx:xx*
    *           111111111122222222223333
    * 0123456789012345678901234567890123
    *
    * For both the commands, a valid command must have atleast first 34 length
    * of data.
    */
   if (total_len < 34) {
       hddLog(LOGE, FL("Invalid command (total_len=%d)"), total_len);
       return -EINVAL;
   }

   if (command[33]) {
      ret = hdd_parse_sendactionframe_v1(pAdapter, command);
   } else {
      ret = hdd_parse_sendactionframe_v2(pAdapter, command, total_len);
   }

   return ret;
}

static void hdd_getBand_helper(hdd_context_t *pHddCtx, int *pBand)
{
    eCsrBand band = -1;
    sme_GetFreqBand((tHalHandle)(pHddCtx->hHal), &band);
    switch (band) {
    case eCSR_BAND_ALL:
        *pBand = WLAN_HDD_UI_BAND_AUTO;
        break;

    case eCSR_BAND_24:
        *pBand = WLAN_HDD_UI_BAND_2_4_GHZ;
        break;

    case eCSR_BAND_5G:
        *pBand = WLAN_HDD_UI_BAND_5_GHZ;
        break;

    default:
        hddLog(LOGW, FL("Invalid Band %d"), band);
        *pBand = -1;
        break;
    }
}


/**
 * hdd_parse_channellist() - HDD Parse channel list
 * @pValue:     Pointer to input data
 * @pChannelList: Pointer to input channel list
 * @pNumChannels: Pointer to number of roam scan channels
 *
 * This function parses the channel list passed in the format
 * SETROAMSCANCHANNELS<space><Number of channels><space>Channel 1<space>
 * Channel 2<space>Channel N
 * if the Number of channels (N) does not match with the actual number of
 * channels passed then take the minimum of N and count of (Ch1, Ch2, ...Ch M)
 * For example, if SETROAMSCANCHANNELS 3 36 40 44 48, only 36, 40 and 44 shall
 * be taken.
 * If SETROAMSCANCHANNELS 5 36 40 44 48, ignore 5 and take 36, 40, 44 and 48.
 * This function does not take care of removing duplicate channels from the list
 *
 * Return: 0 for success non-zero for failure
 */
static int
hdd_parse_channellist(const tANI_U8 *pValue, tANI_U8 *pChannelList,
                      tANI_U8 *pNumChannels)
{
    const tANI_U8 *inPtr = pValue;
    int tempInt;
    int j = 0;
    int v = 0;
    char buf[32];

    inPtr = strnchr(pValue, strlen(pValue), SPACE_ASCII_VALUE);
    /* no argument after the command */
    if (NULL == inPtr) {
        return -EINVAL;
    } else if (SPACE_ASCII_VALUE != *inPtr) {
        /* no space after the command */
        return -EINVAL;
    }

    /* removing empty spaces */
    while ((SPACE_ASCII_VALUE  == *inPtr) && ('\0' !=  *inPtr)) inPtr++;

    /* no argument followed by spaces */
    if ('\0' == *inPtr) {
        return -EINVAL;
    }

    /*getting the first argument ie the number of channels*/
    v = sscanf(inPtr, "%31s ", buf);
    if (1 != v) return -EINVAL;

    v = kstrtos32(buf, 10, &tempInt);
    if ((v < 0) ||
        (tempInt <= 0) ||
        (tempInt > WNI_CFG_VALID_CHANNEL_LIST_LEN)) {
       return -EINVAL;
    }

    *pNumChannels = tempInt;

    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
               "Number of channels are: %d", *pNumChannels);

    for (j = 0; j < (*pNumChannels); j++) {
        /* inPtr pointing to the beginning of first space after number of
           channels*/
        inPtr = strpbrk( inPtr, " " );
        /* no channel list after the number of channels argument */
        if (NULL == inPtr) {
            if (0 != j) {
                *pNumChannels = j;
                return 0;
            } else {
                return -EINVAL;
            }
        }

        /* removing empty space */
        while ((SPACE_ASCII_VALUE == *inPtr) && ('\0' != *inPtr)) inPtr++;

        /*no channel list after the number of channels argument and spaces*/
        if ('\0' == *inPtr) {
            if (0 != j) {
                *pNumChannels = j;
                return 0;
            } else {
                return -EINVAL;
            }
        }

        v = sscanf(inPtr, "%31s ", buf);
        if (1 != v) return -EINVAL;

        v = kstrtos32(buf, 10, &tempInt);
        if ((v < 0) ||
            (tempInt <= 0) ||
            (tempInt > WNI_CFG_CURRENT_CHANNEL_STAMAX)) {
           return -EINVAL;
        }
        pChannelList[j] = tempInt;

        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
                   "Channel %d added to preferred channel list",
                   pChannelList[j] );
    }

    return 0;
}

/*
  \brief hdd_parse_set_roam_scan_channels_v1() - parse version 1 of the
  SETROAMSCANCHANNELS command

  This function parses the v1 SETROAMSCANCHANNELS command with the format

      SETROAMSCANCHANNELS N C1 C2 ... Cn

  Where "N" is the ASCII representation of the number of channels and
  C1 thru Cn is the ASCII representation of the channels.  For example

      SETROAMSCANCHANNELS 4 36 40 44 48

  \param - pAdapter - Adapter upon which the command was received
  \param - command - ASCII text command that was received

  \return - 0 for success non-zero for failure

  --------------------------------------------------------------------------*/
static int
hdd_parse_set_roam_scan_channels_v1(hdd_adapter_t *pAdapter,
                                    const char *command)
{
   tANI_U8 channel_list[WNI_CFG_VALID_CHANNEL_LIST_LEN] = {0};
   tANI_U8 num_chan = 0;
   eHalStatus status;
   hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
   int ret;

   ret = hdd_parse_channellist(command, channel_list, &num_chan);
   if (ret) {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "%s: Failed to parse channel list information", __func__);
      goto exit;
   }

   MTRACE(vos_trace(VOS_MODULE_ID_HDD,
                    TRACE_CODE_HDD_SETROAMSCANCHANNELS_IOCTL,
                    pAdapter->sessionId, num_chan));

   if (num_chan > WNI_CFG_VALID_CHANNEL_LIST_LEN) {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: number of channels (%d) supported exceeded max (%d)",
                  __func__, num_chan, WNI_CFG_VALID_CHANNEL_LIST_LEN);
      ret = -EINVAL;
      goto exit;
   }

   status = sme_ChangeRoamScanChannelList(pHddCtx->hHal, pAdapter->sessionId,
                                          channel_list, num_chan);
   if (eHAL_STATUS_SUCCESS != status) {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "%s: Failed to update channel list information", __func__);
      ret = -EINVAL;
      goto exit;
   }
 exit:
   return ret;
}

/*
  \brief hdd_parse_set_roam_scan_channels_v2() - parse version 2 of the
  SETROAMSCANCHANNELS command

  This function parses the v2 SETROAMSCANCHANNELS command with the format

      SETROAMSCANCHANNELS [N][C1][C2][Cn]

  The command begins with SETROAMSCANCHANNELS followed by a space, but
  what follows the space is an array of u08 parameters.  For example

      SETROAMSCANCHANNELS [0x04 0x24 0x28 0x2c 0x30]

  \param - pAdapter - Adapter upon which the command was received
  \param - command - command that was received, ASCII command followed
                     by binary data

  \return - 0 for success non-zero for failure

  --------------------------------------------------------------------------*/
static int
hdd_parse_set_roam_scan_channels_v2(hdd_adapter_t *pAdapter,
                                    const char *command)
{
   const tANI_U8 *value;
   tANI_U8 channel_list[WNI_CFG_VALID_CHANNEL_LIST_LEN] = {0};
   tANI_U8 channel;
   tANI_U8 num_chan;
   int i;
   hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
   eHalStatus status;
   int ret = 0;

   /* array of values begins after "SETROAMSCANCHANNELS " */
   value = command + 20;

   num_chan = *value++;
   if (num_chan > WNI_CFG_VALID_CHANNEL_LIST_LEN) {
      VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "%s: number of channels (%d) supported exceeded max (%d)",
                __func__, num_chan, WNI_CFG_VALID_CHANNEL_LIST_LEN);
      ret = -EINVAL;
      goto exit;
   }

   MTRACE(vos_trace(VOS_MODULE_ID_HDD,
                    TRACE_CODE_HDD_SETROAMSCANCHANNELS_IOCTL,
                    pAdapter->sessionId, num_chan));

   for (i = 0; i < num_chan; i++) {
      channel = *value++;
      if (!channel) {
         VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                   "%s: Channels end at index %d, expected %d",
                   __func__, i, num_chan);
         ret = -EINVAL;
         goto exit;
      }

      if (channel > WNI_CFG_CURRENT_CHANNEL_STAMAX) {
         VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                   "%s: index %d invalid channel %d", __func__, i, channel);
         ret = -EINVAL;
         goto exit;
      }
      channel_list[i] = channel;
   }
   status = sme_ChangeRoamScanChannelList(pHddCtx->hHal, pAdapter->sessionId,
                                          channel_list, num_chan);
   if (eHAL_STATUS_SUCCESS != status) {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "%s: Failed to update channel list information", __func__);
      ret = -EINVAL;
      goto exit;
   }
 exit:
   return ret;
}

/*
  \brief hdd_parse_set_roam_scan_channels() - parse the
  SETROAMSCANCHANNELS command

  There are two different versions of the SETROAMSCANCHANNELS command.
  Version 1 of the command contains a parameter list that is ASCII
  characters whereas version 2 contains a binary payload.  Determine
  if a version 1 or a version 2 command is being parsed by examining
  the parameters, and then dispatch the parser that is appropriate for
  the command.

  \param - pAdapter - Adapter upon which the command was received
  \param - command - command that was received

  \return - 0 for success non-zero for failure

  --------------------------------------------------------------------------*/
static int
hdd_parse_set_roam_scan_channels(hdd_adapter_t *pAdapter,
                                 const char *command)
{
   const char *cursor;
   char ch;
   bool v1;
   int ret;

   /* start after "SETROAMSCANCHANNELS " */
   cursor = command + 20;

   /* assume we have a version 1 command until proven otherwise */
   v1 = true;

   /* v1 params will only contain ASCII digits and space */
   while ((ch = *cursor++) && v1) {
      if (!(isdigit(ch) || isspace(ch))) {
         v1 = false;
      }
   }
   if (v1) {
      ret = hdd_parse_set_roam_scan_channels_v1(pAdapter, command);
   } else {
      ret = hdd_parse_set_roam_scan_channels_v2(pAdapter, command);
   }

   return ret;
}

#endif /* WLAN_FEATURE_VOWIFI_11R || FEATURE_WLAN_ESE || FEATURE_WLAN_LFR */

#if defined(FEATURE_WLAN_ESE) && defined(FEATURE_WLAN_ESE_UPLOAD)
/**---------------------------------------------------------------------------

  \brief hdd_parse_plm_cmd() - HDD Parse Plm command

  This function parses the plm command passed in the format
  CCXPLMREQ<space><enable><space><dialog_token><space>
  <meas_token><space><num_of_bursts><space><burst_int><space>
  <measu duration><space><burst_len><space><desired_tx_pwr>
  <space><multcast_addr><space><number_of_channels>
  <space><channel_numbers>

  \param  - pValue Pointer to input data
  \param  - pPlmRequest Pointer to output struct tpSirPlmReq

  \return - 0 for success non-zero for failure

  --------------------------------------------------------------------------*/
static eHalStatus hdd_parse_plm_cmd(tANI_U8 *pValue, tSirPlmReq *pPlmRequest)
{
     tANI_U8 *cmdPtr = NULL;
     int count, content = 0, ret = 0;
     char buf[32];

     /* moving to argument list */
     cmdPtr = strnchr(pValue, strlen(pValue), SPACE_ASCII_VALUE);
     if (NULL == cmdPtr)
        return eHAL_STATUS_FAILURE;

     /*no space after the command*/
     if (SPACE_ASCII_VALUE != *cmdPtr)
        return eHAL_STATUS_FAILURE;

     /*removing empty spaces*/
     while ((SPACE_ASCII_VALUE == *cmdPtr) && ('\0' != *cmdPtr))
        cmdPtr++;

     /* START/STOP PLM req */
     ret = sscanf(cmdPtr, "%31s ", buf);
     if (1 != ret) return eHAL_STATUS_FAILURE;

     ret = kstrtos32(buf, 10, &content);
     if ( ret < 0) return eHAL_STATUS_FAILURE;

     pPlmRequest->enable = content;
     cmdPtr = strpbrk(cmdPtr, " ");

     if (NULL == cmdPtr)
        return eHAL_STATUS_FAILURE;

     /*removing empty spaces*/
     while ((SPACE_ASCII_VALUE == *cmdPtr) && ('\0' != *cmdPtr))
        cmdPtr++;

     /* Dialog token of radio meas req containing meas reqIE */
     ret = sscanf(cmdPtr, "%31s ", buf);
     if (1 != ret) return eHAL_STATUS_FAILURE;

     ret = kstrtos32(buf, 10, &content);
     if ( ret < 0) return eHAL_STATUS_FAILURE;

     pPlmRequest->diag_token = content;
     hddLog(VOS_TRACE_LEVEL_DEBUG, "diag token %d", pPlmRequest->diag_token);
     cmdPtr = strpbrk(cmdPtr, " ");

     if (NULL == cmdPtr)
        return eHAL_STATUS_FAILURE;

     /*removing empty spaces*/
     while ((SPACE_ASCII_VALUE == *cmdPtr) && ('\0' != *cmdPtr))
         cmdPtr++;

     /* measurement token of meas req IE */
     ret = sscanf(cmdPtr, "%31s ", buf);
     if (1 != ret) return eHAL_STATUS_FAILURE;

     ret = kstrtos32(buf, 10, &content);
     if ( ret < 0) return eHAL_STATUS_FAILURE;

     pPlmRequest->meas_token = content;
     hddLog(VOS_TRACE_LEVEL_DEBUG, "meas token %d", pPlmRequest->meas_token);

     hddLog(VOS_TRACE_LEVEL_ERROR,
            "PLM req %s", pPlmRequest->enable ? "START" : "STOP");
     if (pPlmRequest->enable) {

        cmdPtr = strpbrk(cmdPtr, " ");

        if (NULL == cmdPtr)
           return eHAL_STATUS_FAILURE;

        /*removing empty spaces*/
        while ((SPACE_ASCII_VALUE == *cmdPtr) && ('\0' != *cmdPtr))
             cmdPtr++;

        /* total number of bursts after which STA stops sending */
        ret = sscanf(cmdPtr, "%31s ", buf);
        if (1 != ret) return eHAL_STATUS_FAILURE;

        ret = kstrtos32(buf, 10, &content);
        if ( ret < 0) return eHAL_STATUS_FAILURE;

        if (content < 0)
           return eHAL_STATUS_FAILURE;

        pPlmRequest->numBursts= content;
        hddLog(VOS_TRACE_LEVEL_DEBUG, "num burst %d", pPlmRequest->numBursts);
        cmdPtr = strpbrk(cmdPtr, " ");

        if (NULL == cmdPtr)
           return eHAL_STATUS_FAILURE;

        /* removing empty spaces */
        while ((SPACE_ASCII_VALUE == *cmdPtr) && ('\0' != *cmdPtr))
            cmdPtr++;

        /* burst interval in seconds */
        ret = sscanf(cmdPtr, "%31s ", buf);
        if (1 != ret) return eHAL_STATUS_FAILURE;

        ret = kstrtos32(buf, 10, &content);
        if ( ret < 0) return eHAL_STATUS_FAILURE;

        if (content <= 0)
           return eHAL_STATUS_FAILURE;

        pPlmRequest->burstInt = content;
        hddLog(VOS_TRACE_LEVEL_DEBUG, "burst Int %d", pPlmRequest->burstInt);
        cmdPtr = strpbrk(cmdPtr, " ");

        if (NULL == cmdPtr)
           return eHAL_STATUS_FAILURE;

        /*removing empty spaces*/
        while ((SPACE_ASCII_VALUE == *cmdPtr) && ('\0' != *cmdPtr))
             cmdPtr++;

        /* Meas dur in TU's,STA goes off-ch and transmit PLM bursts */
        ret = sscanf(cmdPtr, "%31s ", buf);
        if (1 != ret) return eHAL_STATUS_FAILURE;

        ret = kstrtos32(buf, 10, &content);
        if ( ret < 0) return eHAL_STATUS_FAILURE;

        if (content <= 0)
           return eHAL_STATUS_FAILURE;

        pPlmRequest->measDuration = content;
        hddLog(VOS_TRACE_LEVEL_DEBUG, "measDur %d", pPlmRequest->measDuration);
        cmdPtr = strpbrk(cmdPtr, " ");

        if (NULL == cmdPtr)
           return eHAL_STATUS_FAILURE;

        /*removing empty spaces*/
        while ((SPACE_ASCII_VALUE == *cmdPtr) && ('\0' != *cmdPtr))
            cmdPtr++;

        /* burst length of PLM bursts */
        ret = sscanf(cmdPtr, "%31s ", buf);
        if (1 != ret) return eHAL_STATUS_FAILURE;

        ret = kstrtos32(buf, 10, &content);
        if ( ret < 0) return eHAL_STATUS_FAILURE;

        if (content <= 0)
           return eHAL_STATUS_FAILURE;

        pPlmRequest->burstLen = content;
        hddLog(VOS_TRACE_LEVEL_DEBUG, "burstLen %d", pPlmRequest->burstLen);
        cmdPtr = strpbrk(cmdPtr, " ");

        if (NULL == cmdPtr)
           return eHAL_STATUS_FAILURE;

        /*removing empty spaces*/
        while ((SPACE_ASCII_VALUE == *cmdPtr) && ('\0' != *cmdPtr))
             cmdPtr++;

        /* desired tx power for transmission of PLM bursts */
        ret = sscanf(cmdPtr, "%31s ", buf);
        if (1 != ret) return eHAL_STATUS_FAILURE;

        ret = kstrtos32(buf, 10, &content);
        if ( ret < 0) return eHAL_STATUS_FAILURE;

        if (content <= 0)
           return eHAL_STATUS_FAILURE;

        pPlmRequest->desiredTxPwr = content;
        hddLog(VOS_TRACE_LEVEL_DEBUG,
               "desiredTxPwr %d", pPlmRequest->desiredTxPwr);

        for (count = 0; count < VOS_MAC_ADDR_SIZE; count++)
        {
            cmdPtr = strpbrk(cmdPtr, " ");

            if (NULL == cmdPtr)
               return eHAL_STATUS_FAILURE;

            /*removing empty spaces*/
            while ((SPACE_ASCII_VALUE == *cmdPtr) && ('\0' != *cmdPtr))
                cmdPtr++;

            ret = sscanf(cmdPtr, "%31s ", buf);
            if (1 != ret) return eHAL_STATUS_FAILURE;

            ret = kstrtos32(buf, 16, &content);
            if ( ret < 0) return eHAL_STATUS_FAILURE;

            pPlmRequest->macAddr[count] = content;
        }

        hddLog(VOS_TRACE_LEVEL_DEBUG, "MC addr "MAC_ADDRESS_STR,
                          MAC_ADDR_ARRAY(pPlmRequest->macAddr));

        cmdPtr = strpbrk(cmdPtr, " ");

        if (NULL == cmdPtr)
           return eHAL_STATUS_FAILURE;

        /*removing empty spaces*/
        while ((SPACE_ASCII_VALUE == *cmdPtr) && ('\0' != *cmdPtr))
            cmdPtr++;

        /* number of channels */
        ret = sscanf(cmdPtr, "%31s ", buf);
        if (1 != ret) return eHAL_STATUS_FAILURE;

        ret = kstrtos32(buf, 10, &content);
        if ( ret < 0) return eHAL_STATUS_FAILURE;

        if (content < 0)
           return eHAL_STATUS_FAILURE;

        content = VOS_MIN(content, WNI_CFG_VALID_CHANNEL_LIST_LEN);
        pPlmRequest->plmNumCh = content;
        hddLog(LOG1, FL("Numch: %d"), pPlmRequest->plmNumCh);

        /* Channel numbers */
        for (count = 0; count < pPlmRequest->plmNumCh; count++)
        {
             cmdPtr = strpbrk(cmdPtr, " ");

             if (NULL == cmdPtr)
                return eHAL_STATUS_FAILURE;

             /*removing empty spaces*/
             while ((SPACE_ASCII_VALUE == *cmdPtr) && ('\0' != *cmdPtr))
                cmdPtr++;

             ret = sscanf(cmdPtr, "%31s ", buf);
             if (1 != ret) return eHAL_STATUS_FAILURE;

             ret = kstrtos32(buf, 10, &content);
             if (ret < 0 || content <= 0 ||
                 content > WNI_CFG_CURRENT_CHANNEL_STAMAX)
                 return eHAL_STATUS_FAILURE;

             pPlmRequest->plmChList[count]= content;
             hddLog(VOS_TRACE_LEVEL_DEBUG, " ch- %d",
                    pPlmRequest->plmChList[count]);
         }
     } /* If PLM START */

     return eHAL_STATUS_SUCCESS;
}
#endif
#ifdef WLAN_FEATURE_EXTWOW_SUPPORT
static void wlan_hdd_ready_to_extwow(void *callbackContext,
                                             boolean is_success)
{
    hdd_context_t *pHddCtx = (hdd_context_t *)callbackContext;
    int rc;

    rc = wlan_hdd_validate_context(pHddCtx);
    if (0 != rc)
       return;

    pHddCtx->ext_wow_should_suspend = is_success;
    complete(&pHddCtx->ready_to_extwow);
}

static int hdd_enable_ext_wow(hdd_adapter_t *pAdapter,
                               tpSirExtWoWParams arg_params)
{
    tSirExtWoWParams params;
    eHalStatus halStatus = eHAL_STATUS_FAILURE;
    hdd_context_t  *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
    tHalHandle hHal = WLAN_HDD_GET_HAL_CTX(pAdapter);
    int rc;

    vos_mem_copy(&params, arg_params, sizeof(params));

    INIT_COMPLETION(pHddCtx->ready_to_extwow);

    halStatus = sme_ConfigureExtWoW(hHal, &params,
                &wlan_hdd_ready_to_extwow, pHddCtx);
    if (eHAL_STATUS_SUCCESS != halStatus) {
        hddLog(VOS_TRACE_LEVEL_ERROR,
               FL("sme_ConfigureExtWoW returned failure %d"), halStatus);
        return -EPERM;
    }

    rc = wait_for_completion_timeout(&pHddCtx->ready_to_extwow,
                             msecs_to_jiffies(WLAN_WAIT_TIME_READY_TO_EXTWOW));
    if (!rc) {
        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                   "%s: Failed to get ready to extwow", __func__);
        return -EPERM;
    }

    if (pHddCtx->ext_wow_should_suspend) {
       if (pHddCtx->cfg_ini->extWowGotoSuspend) {
          VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
              "%s: Received ready to ExtWoW. Going to suspend", __func__);

          wlan_hdd_cfg80211_suspend_wlan(pHddCtx->wiphy, NULL);
          wlan_hif_pci_suspend();
       }
    } else {
        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
              "%s: Received ready to ExtWoW failure", __func__);
        return -EPERM;
    }

    return 0;
}

static int hdd_enable_ext_wow_parser(hdd_adapter_t *pAdapter, int vdev_id,
                                                                  int value)
{
   tSirExtWoWParams params;
   hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
   int rc;

   rc = wlan_hdd_validate_context(pHddCtx);
   if (0 != rc)
       return -EINVAL;

   if (value < EXT_WOW_TYPE_APP_TYPE1 || value > EXT_WOW_TYPE_APP_TYPE1_2 ) {
       hddLog(VOS_TRACE_LEVEL_ERROR, FL("Invalid type"));
       return -EINVAL;
   }

   if (value == EXT_WOW_TYPE_APP_TYPE1 &&
        pHddCtx->is_extwow_app_type1_param_set)
        params.type = value;
   else if (value == EXT_WOW_TYPE_APP_TYPE2 &&
        pHddCtx->is_extwow_app_type2_param_set)
        params.type = value;
   else if (value == EXT_WOW_TYPE_APP_TYPE1_2 &&
        pHddCtx->is_extwow_app_type1_param_set &&
        pHddCtx->is_extwow_app_type2_param_set)
        params.type = value;
   else {
        hddLog(VOS_TRACE_LEVEL_ERROR,
           FL("Set app params before enable it value %d"),value);
        return -EINVAL;
   }

   params.vdev_id = vdev_id;
   params.wakeup_pin_num = pHddCtx->cfg_ini->extWowApp1WakeupPinNumber |
                      (pHddCtx->cfg_ini->extWowApp2WakeupPinNumber << 8);

   return hdd_enable_ext_wow(pAdapter, &params);
}

static int hdd_set_app_type1_params(tHalHandle hHal,
                         tpSirAppType1Params arg_params)
{
    tSirAppType1Params params;
    eHalStatus halStatus = eHAL_STATUS_FAILURE;

    vos_mem_copy(&params, arg_params, sizeof(params));

    halStatus = sme_ConfigureAppType1Params(hHal, &params);
    if (eHAL_STATUS_SUCCESS != halStatus) {
        hddLog(VOS_TRACE_LEVEL_ERROR,
             FL("sme_ConfigureAppType1Params returned failure %d"), halStatus);
        return -EPERM;
    }

    return 0;
}

static int hdd_set_app_type1_parser(hdd_adapter_t *pAdapter,
                                             char *arg, int len)
{
    hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
    tHalHandle hHal = WLAN_HDD_GET_HAL_CTX(pAdapter);
    char id[20], password[20];
    tSirAppType1Params params;
    int rc, i;

    rc = wlan_hdd_validate_context(pHddCtx);
    if (0 != rc)
       return -EINVAL;

    if (2 != sscanf(arg, "%8s %16s", id, password)) {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 FL("Invalid Number of arguments"));
       return -EINVAL;
    }

    memset(&params, 0, sizeof(tSirAppType1Params));
    params.vdev_id = pAdapter->sessionId;
    for (i = 0; i < ETHER_ADDR_LEN; i++)
        params.wakee_mac_addr[i] = pAdapter->macAddressCurrent.bytes[i];

    params.id_length = strlen(id);
    vos_mem_copy(params.identification_id, id, params.id_length);
    params.pass_length = strlen(password);
    vos_mem_copy(params.password, password, params.pass_length);

    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
        "%s: %d %pM %.8s %u %.16s %u",
        __func__, params.vdev_id, params.wakee_mac_addr,
        params.identification_id, params.id_length,
        params.password, params.pass_length );

    return hdd_set_app_type1_params(hHal, &params);
}

static int hdd_set_app_type2_params(tHalHandle hHal,
                          tpSirAppType2Params arg_params)
{
    tSirAppType2Params params;
    eHalStatus halStatus = eHAL_STATUS_FAILURE;

    vos_mem_copy(&params, arg_params, sizeof(params));

    halStatus = sme_ConfigureAppType2Params(hHal, &params);
    if (eHAL_STATUS_SUCCESS != halStatus)
    {
        hddLog(VOS_TRACE_LEVEL_ERROR,
             FL("sme_ConfigureAppType2Params returned failure %d"), halStatus);
        return -EPERM;
    }

    return 0;
}

static int hdd_set_app_type2_parser(hdd_adapter_t *pAdapter,
                            char *arg, int len)
{
    hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
    tHalHandle hHal = WLAN_HDD_GET_HAL_CTX(pAdapter);
    char mac_addr[20], rc4_key[20];
    unsigned int gateway_mac[6], i;
    tSirAppType2Params params;
    int ret;

    ret = wlan_hdd_validate_context(pHddCtx);
    if (0 != ret)
        return -EINVAL;

    memset(&params, 0, sizeof(tSirAppType2Params));

    ret = sscanf(arg, "%17s %16s %x %x %x %u %u %hu %hu %u %u %u %u %u %u",
        mac_addr, rc4_key, (unsigned int *)&params.ip_id,
        (unsigned int*)&params.ip_device_ip,
        (unsigned int*)&params.ip_server_ip,
        (unsigned int*)&params.tcp_seq, (unsigned int*)&params.tcp_ack_seq,
        (uint16_t*)&params.tcp_src_port,
        (uint16_t*)&params.tcp_dst_port,
        (unsigned int*)&params.keepalive_init,
        (unsigned int*)&params.keepalive_min,
        (unsigned int*)&params.keepalive_max,
        (unsigned int*)&params.keepalive_inc,
        (unsigned int*)&params.tcp_tx_timeout_val,
        (unsigned int*)&params.tcp_rx_timeout_val);


    if (ret != 15 && ret != 7) {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "Invalid Number of arguments");
        return -EINVAL;
    }

    if (6 != sscanf(mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x", &gateway_mac[0],
             &gateway_mac[1], &gateway_mac[2], &gateway_mac[3],
             &gateway_mac[4], &gateway_mac[5])) {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
               "Invalid MacAddress Input %s", mac_addr);
        return -EINVAL;
    }

    if (params.tcp_src_port > WLAN_HDD_MAX_TCP_PORT ||
           params.tcp_dst_port > WLAN_HDD_MAX_TCP_PORT) {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
             "Invalid TCP Port Number");
        return -EINVAL;
    }

    for (i = 0; i < ETHER_ADDR_LEN; i++)
        params.gateway_mac[i] = (uint8_t) gateway_mac[i];

    params.rc4_key_len = strlen(rc4_key);
    vos_mem_copy(params.rc4_key, rc4_key, params.rc4_key_len);

    params.vdev_id = pAdapter->sessionId;
    params.tcp_src_port = (params.tcp_src_port != 0)?
        params.tcp_src_port : pHddCtx->cfg_ini->extWowApp2TcpSrcPort;
    params.tcp_dst_port = (params.tcp_dst_port != 0)?
        params.tcp_dst_port : pHddCtx->cfg_ini->extWowApp2TcpDstPort;
    params.keepalive_init = (params.keepalive_init != 0)?
        params.keepalive_init : pHddCtx->cfg_ini->extWowApp2KAInitPingInterval;
    params.keepalive_min = (params.keepalive_min != 0)?
        params.keepalive_min : pHddCtx->cfg_ini->extWowApp2KAMinPingInterval;
    params.keepalive_max = (params.keepalive_max != 0)?
        params.keepalive_max : pHddCtx->cfg_ini->extWowApp2KAMaxPingInterval;
    params.keepalive_inc = (params.keepalive_inc != 0)?
        params.keepalive_inc : pHddCtx->cfg_ini->extWowApp2KAIncPingInterval;
    params.tcp_tx_timeout_val = (params.tcp_tx_timeout_val != 0)?
        params.tcp_tx_timeout_val : pHddCtx->cfg_ini->extWowApp2TcpTxTimeout;
    params.tcp_rx_timeout_val = (params.tcp_rx_timeout_val != 0)?
        params.tcp_rx_timeout_val : pHddCtx->cfg_ini->extWowApp2TcpRxTimeout;

    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
        "%s: %pM %.16s %u %u %u %u %u %u %u %u %u %u %u %u %u",
        __func__, gateway_mac, rc4_key, params.ip_id, params.ip_device_ip,
        params.ip_server_ip, params.tcp_seq, params.tcp_ack_seq,
        params.tcp_src_port, params.tcp_dst_port, params.keepalive_init,
        params.keepalive_min, params.keepalive_max,
        params.keepalive_inc, params.tcp_tx_timeout_val,
        params.tcp_rx_timeout_val);

    return hdd_set_app_type2_params(hHal, &params);
}

#endif

int wlan_hdd_set_mc_rate(hdd_adapter_t *pAdapter, int targetRate)
{
   tSirRateUpdateInd rateUpdate = {0};
   eHalStatus status;
   hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
   hdd_config_t *pConfig = NULL;

   if (pHddCtx == NULL) {
      hddLog(LOGE, FL("HDD context is null"));
      return -EINVAL;
   }

   if ((WLAN_HDD_IBSS != pAdapter->device_mode) &&
       (WLAN_HDD_SOFTAP != pAdapter->device_mode) &&
       (WLAN_HDD_INFRA_STATION != pAdapter->device_mode)) {
      hddLog(LOGE, FL("Received SETMCRATE cmd in invalid device mode %s(%d)"),
             hdd_device_mode_to_string(pAdapter->device_mode),
             pAdapter->device_mode);
      hddLog(LOGE,
             FL("SETMCRATE cmd is allowed only in STA, IBSS or SOFTAP mode"));
      return -EINVAL;
   }

   pConfig = pHddCtx->cfg_ini;
   rateUpdate.nss = (pConfig->enable2x2 == 0) ? 0 : 1;
   rateUpdate.dev_mode = pAdapter->device_mode;
   rateUpdate.mcastDataRate24GHz = targetRate;
   rateUpdate.mcastDataRate24GHzTxFlag = 1;
   rateUpdate.mcastDataRate5GHz = targetRate;
   rateUpdate.bcastDataRate = -1;
   memcpy(rateUpdate.bssid, pAdapter->macAddressCurrent.bytes,
      sizeof(rateUpdate.bssid));
   hddLog(LOG1, FL("MC Target rate %d, mac = %pM, dev_mode %s(%d)"),
      rateUpdate.mcastDataRate24GHz, rateUpdate.bssid,
      hdd_device_mode_to_string(pAdapter->device_mode),
      pAdapter->device_mode);

   status = sme_SendRateUpdateInd(pHddCtx->hHal, &rateUpdate);
   if (eHAL_STATUS_SUCCESS != status) {
      hddLog(LOGE, FL("SETMCRATE failed"));
      return -EFAULT;
   }
   return 0;
}

/**---------------------------------------------------------------------------

  \brief hdd_parse_setmaxtxpower_command() - HDD Parse MAXTXPOWER command

  This function parses the MAXTXPOWER command passed in the format
  MAXTXPOWER<space>X(Tx power in dbm)
  For example input commands:
  1) MAXTXPOWER -8 -> This is translated into set max TX power to -8 dbm
  2) MAXTXPOWER -23 -> This is translated into set max TX power to -23 dbm

  \param  - pValue Pointer to MAXTXPOWER command
  \param  - pDbm Pointer to tx power

  \return - 0 for success non-zero for failure

  --------------------------------------------------------------------------*/
static int hdd_parse_setmaxtxpower_command(tANI_U8 *pValue, int *pTxPower)
{
    tANI_U8 *inPtr = pValue;
    int tempInt;
    int v = 0;
    *pTxPower = 0;

    inPtr = strnchr(pValue, strlen(pValue), SPACE_ASCII_VALUE);
    /*no argument after the command*/
    if (NULL == inPtr)
    {
        return -EINVAL;
    }

    /*no space after the command*/
    else if (SPACE_ASCII_VALUE != *inPtr)
    {
        return -EINVAL;
    }

    /*removing empty spaces*/
    while ((SPACE_ASCII_VALUE  == *inPtr) && ('\0' !=  *inPtr)) inPtr++;

    /*no argument followed by spaces*/
    if ('\0' == *inPtr)
    {
        return 0;
    }

    v = kstrtos32(inPtr, 10, &tempInt);

    /* Range checking for passed parameter */
    if ( (tempInt < HDD_MIN_TX_POWER) ||
         (tempInt > HDD_MAX_TX_POWER) )
    {
       return -EINVAL;
    }

    *pTxPower = tempInt;

    VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
       "SETMAXTXPOWER: %d", *pTxPower);

    return 0;
} /*End of hdd_parse_setmaxtxpower_command*/


static int hdd_get_dwell_time(hdd_config_t *pCfg, tANI_U8 *command, char *extra, tANI_U8 n, tANI_U8 *len)
{
    int ret = 0;

    if (!pCfg || !command || !extra || !len)
    {
        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
             "%s: argument passed for GETDWELLTIME is incorrect", __func__);
        ret = -EINVAL;
        return ret;
    }

    if (strncmp(command, "GETDWELLTIME ACTIVE MAX", 23) == 0)
    {
        *len = scnprintf(extra, n, "GETDWELLTIME ACTIVE MAX %u\n",
                (int)pCfg->nActiveMaxChnTime);
        return ret;
    }
    else if (strncmp(command, "GETDWELLTIME ACTIVE MIN", 23) == 0)
    {
        *len = scnprintf(extra, n, "GETDWELLTIME ACTIVE MIN %u\n",
                (int)pCfg->nActiveMinChnTime);
        return ret;
    }
    else if (strncmp(command, "GETDWELLTIME PASSIVE MAX", 24) == 0)
    {
        *len = scnprintf(extra, n, "GETDWELLTIME PASSIVE MAX %u\n",
                (int)pCfg->nPassiveMaxChnTime);
        return ret;
    }
    else if (strncmp(command, "GETDWELLTIME PASSIVE MIN", 24) == 0)
    {
        *len = scnprintf(extra, n, "GETDWELLTIME PASSIVE MIN %u\n",
                (int)pCfg->nPassiveMinChnTime);
        return ret;
    }
    else if (strncmp(command, "GETDWELLTIME", 12) == 0)
    {
        *len = scnprintf(extra, n, "GETDWELLTIME %u \n",
                (int)pCfg->nActiveMaxChnTime);
        return ret;
    }
    else
    {
        ret = -EINVAL;
    }

    return ret;
}

int hdd_drv_cmd_validate(tANI_U8 *command, int len)
{
	if (command[len] != ' ')
		return -EINVAL;

	return 0;
}

static int hdd_set_dwell_time(hdd_adapter_t *pAdapter, tANI_U8 *command)
{
    tHalHandle hHal;
    hdd_config_t *pCfg;
    tANI_U8 *value = command;
    tSmeConfigParams smeConfig;
    int val = 0, ret = 0, temp = 0;

    if (!pAdapter || !command || !(pCfg = (WLAN_HDD_GET_CTX(pAdapter))->cfg_ini)
        || !(hHal = (WLAN_HDD_GET_HAL_CTX(pAdapter))))
    {
        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
         "%s: argument passed for SETDWELLTIME is incorrect", __func__);
        ret = -EINVAL;
        return ret;
    }

    vos_mem_zero(&smeConfig, sizeof(smeConfig));
    sme_GetConfigParam(hHal, &smeConfig);

    if (strncmp(command, "SETDWELLTIME ACTIVE MAX", 23) == 0 )
    {
        if (hdd_drv_cmd_validate(command, 23))
            return -EINVAL;

        value = value + 24;
        temp = kstrtou32(value, 10, &val);
        if (temp != 0 || val < CFG_ACTIVE_MAX_CHANNEL_TIME_MIN ||
                         val > CFG_ACTIVE_MAX_CHANNEL_TIME_MAX )
        {
            VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
             "%s: argument passed for SETDWELLTIME ACTIVE MAX is incorrect", __func__);
            ret = -EFAULT;
            return ret;
        }
        pCfg->nActiveMaxChnTime = val;
        smeConfig.csrConfig.nActiveMaxChnTime = val;
        sme_UpdateConfig(hHal, &smeConfig);
    }
    else if (strncmp(command, "SETDWELLTIME ACTIVE MIN", 23) == 0)
    {
        if (hdd_drv_cmd_validate(command, 23))
            return -EINVAL;

        value = value + 24;
        temp = kstrtou32(value, 10, &val);
        if (temp !=0 || val < CFG_ACTIVE_MIN_CHANNEL_TIME_MIN  ||
                        val > CFG_ACTIVE_MIN_CHANNEL_TIME_MAX )
        {
            VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
             "%s: argument passed for SETDWELLTIME ACTIVE MIN is incorrect", __func__);
            ret = -EFAULT;
            return ret;
        }
        pCfg->nActiveMinChnTime = val;
        smeConfig.csrConfig.nActiveMinChnTime = val;
        sme_UpdateConfig(hHal, &smeConfig);
    }
    else if (strncmp(command, "SETDWELLTIME PASSIVE MAX", 24) == 0)
    {
        if (hdd_drv_cmd_validate(command, 24))
            return -EINVAL;

        value = value + 25;
        temp = kstrtou32(value, 10, &val);
        if (temp != 0 || val < CFG_PASSIVE_MAX_CHANNEL_TIME_MIN ||
                         val > CFG_PASSIVE_MAX_CHANNEL_TIME_MAX )
        {
            VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
             "%s: argument passed for SETDWELLTIME PASSIVE MAX is incorrect", __func__);
            ret = -EFAULT;
            return ret;
        }
        pCfg->nPassiveMaxChnTime = val;
        smeConfig.csrConfig.nPassiveMaxChnTime = val;
        sme_UpdateConfig(hHal, &smeConfig);
    }
    else if (strncmp(command, "SETDWELLTIME PASSIVE MIN", 24) == 0)
    {
        if (hdd_drv_cmd_validate(command, 24))
            return -EINVAL;

        value = value + 25;
        temp = kstrtou32(value, 10, &val);
        if (temp != 0 || val < CFG_PASSIVE_MIN_CHANNEL_TIME_MIN ||
                         val > CFG_PASSIVE_MIN_CHANNEL_TIME_MAX )
        {
            VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
             "%s: argument passed for SETDWELLTIME PASSIVE MIN is incorrect", __func__);
            ret = -EFAULT;
            return ret;
        }
        pCfg->nPassiveMinChnTime = val;
        smeConfig.csrConfig.nPassiveMinChnTime = val;
        sme_UpdateConfig(hHal, &smeConfig);
    }
    else if (strncmp(command, "SETDWELLTIME", 12) == 0)
    {
        if (hdd_drv_cmd_validate(command, 12))
            return -EINVAL;

        value = value + 13;
        temp = kstrtou32(value, 10, &val);
        if (temp != 0 || val < CFG_ACTIVE_MAX_CHANNEL_TIME_MIN ||
                         val > CFG_ACTIVE_MAX_CHANNEL_TIME_MAX )
        {
            VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
             "%s: argument passed for SETDWELLTIME is incorrect", __func__);
            ret = -EFAULT;
            return ret;
        }
        pCfg->nActiveMaxChnTime = val;
        smeConfig.csrConfig.nActiveMaxChnTime = val;
        sme_UpdateConfig(hHal, &smeConfig);
    }
    else
    {
        ret = -EINVAL;
    }

    return ret;
}
/**
 * hdd_indicate_mgmt_frame() - Wrapper to indicate management frame to
 * user space
 * @frame_ind: Management frame data to be informed.
 *
 * This function is used to indicate management frame to
 * user space
 *
 * Return: None
 *
 */
void hdd_indicate_mgmt_frame(tSirSmeMgmtFrameInd *frame_ind)
{
	hdd_context_t *hdd_ctx;
	hdd_adapter_t *adapter;
	v_CONTEXT_t vos_context;
	int i;
	struct ieee80211_mgmt *mgmt =
		(struct ieee80211_mgmt *)frame_ind->frameBuf;

	/* Get the global VOSS context.*/
	vos_context = vos_get_global_context(VOS_MODULE_ID_SYS, NULL);
	if (!vos_context) {
		hddLog(LOGE, FL("Global VOS context is Null"));
		return;
	}
	/* Get the HDD context.*/
	hdd_ctx =
	  (hdd_context_t *)vos_get_context(VOS_MODULE_ID_HDD, vos_context);

	if (0 != wlan_hdd_validate_context(hdd_ctx))
		return;

	if (frame_ind->frame_len < ieee80211_hdrlen(mgmt->frame_control)) {
		hddLog(LOGE, FL("Invalid frame length"));
		return;
	}

	if (HDD_SESSION_ID_ANY == frame_ind->sessionId) {
		for (i = 0; i < HDD_SESSION_MAX; i++) {
			adapter =
				hdd_get_adapter_by_sme_session_id(hdd_ctx, i);
			if (adapter)
				break;
		}
	} else {
		adapter = hdd_get_adapter_by_sme_session_id(hdd_ctx,
					frame_ind->sessionId);
	}

	if ((NULL != adapter) &&
		(WLAN_HDD_ADAPTER_MAGIC == adapter->magic))
		__hdd_indicate_mgmt_frame(adapter,
						frame_ind->frame_len,
						frame_ind->frameBuf,
						frame_ind->frameType,
						frame_ind->rxChan,
						frame_ind->rxRssi,
						frame_ind->rx_flags);
	return;
}

struct fw_state {
	bool fw_active;
};

/**
 * hdd_get_fw_state_cb() - validates the context and notifies the caller
 * @cookie: cookie from the request contest
 *
 * Return: none
 */
static void hdd_get_fw_state_cb(void *cookie)
{
	struct hdd_request *request;
	struct fw_state *priv;

	request = hdd_request_get(cookie);
	if (!request) {
		hddLog(LOGE, FL("Obsolete request"));
		return;
	}

	priv = hdd_request_priv(request);

	priv->fw_active = true;

	hdd_request_complete(request);
	hdd_request_put(request);
}

/**
 * wlan_hdd_get_fw_state() - get firmware state
 * @adapter:     pointer to the adapter
 *
 * This function sends a request to firmware and waits
 * on a timer to invoke the callback. if the callback is invoked then
 * true will be returned or otherwise fail status will be returned.
 *
 * Return: true, firmware is active.
 *         false, firmware is in bad state.
 */
bool wlan_hdd_get_fw_state(hdd_adapter_t *adapter)
{
	hdd_context_t *hdd_ctx = WLAN_HDD_GET_CTX(adapter);
	eHalStatus hstatus;
	int ret;
	void *cookie;
	struct fw_state *priv;
	static const struct hdd_request_params params = {
		.priv_size = sizeof(*priv),
		.timeout_ms = WLAN_WAIT_TIME_FW_STATE,
	};
	struct hdd_request *request;
	bool fw_active;

	if (wlan_hdd_validate_context(hdd_ctx) != 0)
		return false;

	request = hdd_request_alloc(&params);
	if (!request) {
		hddLog(LOGE, FL("Request allocation failure"));
		return false;
	}

	cookie = hdd_request_cookie(request);

	hstatus = sme_get_fw_state(WLAN_HDD_GET_HAL_CTX(adapter),
				   hdd_get_fw_state_cb,
				   cookie);
	if (eHAL_STATUS_SUCCESS != hstatus) {
		hddLog(LOGE, FL("Unable to retrieve firmware status"));
		fw_active = false;
	} else {
		/* request is sent -- wait for the response */
		ret = hdd_request_wait_for_response(request);
		if (ret) {
			hddLog(LOGE,
				FL("SME timed out while retrieving firmware status"));
			fw_active = false;
		} else {
			priv = hdd_request_priv(request);
			fw_active = priv->fw_active;
		}
	}

	hdd_request_put(request);

	return fw_active;
}

struct link_status_priv {
	uint8_t link_status;
};

static void hdd_get_link_status_cb(uint8_t status, void *context)
{
	struct hdd_request *request;
	struct link_status_priv *priv;

	request = hdd_request_get(context);
	if (!request) {
		hddLog(VOS_TRACE_LEVEL_ERROR, "%s: Obsolete request", __func__);
		return;
	}

	priv = hdd_request_priv(request);
	priv->link_status = status;
	hdd_request_complete(request);
	hdd_request_put(request);
}

/**
 * wlan_hdd_get_link_status() - get link status
 * @pAdapter:     pointer to the adapter
 *
 * This function sends a request to query the link status and waits
 * on a timer to invoke the callback. if the callback is invoked then
 * latest link status shall be returned or otherwise cached value
 * will be returned.
 *
 * Return: On success, link status shall be returned.
 *         On error or not associated, link status 0 will be returned.
 */
static int wlan_hdd_get_link_status(hdd_adapter_t *pAdapter)
{
	hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
	hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
	eHalStatus hstatus;
	int ret;
	void *cookie;
	struct hdd_request *request;
	struct link_status_priv *priv;
	static const struct hdd_request_params params = {
		.priv_size = sizeof(*priv),
		.timeout_ms = WLAN_WAIT_TIME_LINK_STATUS,
	};

	if (pHddCtx->isLogpInProgress) {
		hddLog(LOGW, FL("LOGP in Progress. Ignore!!!"));
		return 0;
	}

	if (eConnectionState_Associated != pHddStaCtx->conn_info.connState) {
	/* If not associated, then expected link status return value is 0 */
		hddLog(LOG1, FL("Not associated!"));
		return 0;
	}

	request = hdd_request_alloc(&params);
	if (!request) {
		hddLog(VOS_TRACE_LEVEL_ERROR, "%s: Request allocation failure",
				__func__);
		return 0;
	}
	cookie = hdd_request_cookie(request);

	hstatus = sme_getLinkStatus(WLAN_HDD_GET_HAL_CTX(pAdapter),
				hdd_get_link_status_cb,
				cookie,
				pAdapter->sessionId);
	if (eHAL_STATUS_SUCCESS != hstatus) {
		hddLog(VOS_TRACE_LEVEL_ERROR,
			"%s: Unable to retrieve link status", __func__);
		/* return a cached value */
	} else {
		/* request is sent -- wait for the response */
		ret = hdd_request_wait_for_response(request);
		if (ret) {
			hddLog(VOS_TRACE_LEVEL_ERROR,
			       "%s: SME timed out while retrieving link status",
			       __func__);
			/* return a cached value */
		} else {
			/* update the adapter with the fresh results */
			priv = hdd_request_priv(request);
			pAdapter->linkStatus = priv->link_status;
		}
	}
	/*
	 * either we never sent a request, we sent a request and
	 * received a response or we sent a request and timed out.
	 * regardless we are done with the request.
	 */
	hdd_request_put(request);

	/* either callback updated adapter stats or it has cached data */
	return pAdapter->linkStatus;
}

#ifdef WLAN_FEATURE_ROAM_OFFLOAD
static void hdd_wma_send_fastreassoc_cmd(int sessionId, tSirMacAddr bssid,
                                     int channel)
{
    t_wma_roam_invoke_cmd *fastreassoc;
    vos_msg_t msg = {0};

    fastreassoc = vos_mem_malloc(sizeof(*fastreassoc));
    if (NULL == fastreassoc) {
        hddLog(LOGE, FL("vos_mem_alloc failed for fastreassoc"));
        return;
    }
    fastreassoc->vdev_id = sessionId;
    fastreassoc->channel = channel;
    fastreassoc->bssid[0] = bssid[0];
    fastreassoc->bssid[1] = bssid[1];
    fastreassoc->bssid[2] = bssid[2];
    fastreassoc->bssid[3] = bssid[3];
    fastreassoc->bssid[4] = bssid[4];
    fastreassoc->bssid[5] = bssid[5];

    msg.type = SIR_HAL_ROAM_INVOKE;
    msg.reserved = 0;
    msg.bodyptr = fastreassoc;
    if (VOS_STATUS_SUCCESS != vos_mq_post_message(VOS_MODULE_ID_WDA,
                                                             &msg)) {
        vos_mem_free(fastreassoc);
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
               FL("Not able to post ROAM_INVOKE_CMD message to WMA"));
    }
}
#endif

/**
 * hdd_set_miracast_mode() - function used to set the miracast mode value
 * @pAdapter: pointer to the adapter of the interface.
 * @command: pointer to the command buffer "MIRACAST <value>".
 * Return: 0 on success -EINVAL on failure.
 */
int hdd_set_miracast_mode(hdd_adapter_t *pAdapter, tANI_U8 *command)
{
    tHalHandle hHal;
    tANI_U8 filterType = 0;
    hdd_context_t *pHddCtx = NULL;
    tANI_U8 *value;
    int ret;
    eHalStatus ret_status;

    pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
    if (0 != wlan_hdd_validate_context(pHddCtx))
        return -EINVAL;

    hHal = pHddCtx->hHal;

    value = command + 9;

    /* Convert the value from ascii to integer */
    ret = kstrtou8(value, 10, &filterType);
    if (ret < 0) {
        /* If the input value is greater than max value of datatype,
         * then also kstrtou8 fails
         */
        hddLog(VOS_TRACE_LEVEL_ERROR, "%s: kstrtou8 failed range", __func__);
        return -EINVAL;
    }

    /* Filtertype value should be either 0-Disabled, 1-Source, 2-sink */
    if ((filterType < WLAN_HDD_DRIVER_MIRACAST_CFG_MIN_VAL ) ||
            (filterType > WLAN_HDD_DRIVER_MIRACAST_CFG_MAX_VAL)) {
        hddLog(VOS_TRACE_LEVEL_ERROR, "%s: Accepted Values are 0 to 2."
                "0-Disabled, 1-Source, 2-Sink", __func__);
        return -EINVAL;
    }
    hddLog(VOS_TRACE_LEVEL_INFO, "%s: miracast mode %hu", __func__, filterType);
    pHddCtx->miracast_value = filterType;

    ret_status = sme_set_miracast(hHal, filterType);
    if (eHAL_STATUS_SUCCESS != ret_status) {
        hddLog(LOGE, "Failed to set miracast");
        return -EBUSY;
    }

    if (hdd_is_mcc_in_24G(pHddCtx)) {
        return hdd_set_mas(pAdapter, filterType);
    }

    return 0;
}

/* Function header is left blank intentionally */
static int hdd_parse_set_ibss_oui_data_command(uint8_t *command, uint8_t *ie,
                                             int32_t *oui_length, int32_t limit)
{
   uint8_t len;
   uint8_t data;

   while ((SPACE_ASCII_VALUE == *command) && ('\0' != *command)) {
      command++;
      limit--;
   }

   len = 2;

   while ((SPACE_ASCII_VALUE != *command) && ('\0' != *command) &&
          (limit > 1)) {
      sscanf(command, "%02x", (unsigned int *)&data);
      ie[len++] = data;
      command += 2;
      limit -= 2;
   }

   *oui_length = len - 2;

   while ((SPACE_ASCII_VALUE == *command) && ('\0' != *command)) {
      command++;
      limit--;
   }

   while ((SPACE_ASCII_VALUE != *command) && ('\0' != *command) &&
         (limit > 1)) {
      sscanf(command, "%02x", (unsigned int *)&data);
      ie[len++] = data;
      command += 2;
      limit -= 2;
   }

   ie[0] = IE_EID_VENDOR;
   ie[1] = len - 2;

   return len;
}


/**
 * hdd_set_mas() - Function to set MAS value to UMAC
 * @adapter:		Pointer to HDD adapter
 * @mas_value:		0-Disable, 1-Enable MAS
 *
 * This function passes down the value of MAS to UMAC
 *
 * Return: Configuration message posting status, SUCCESS or Fail
 *
 */
int32_t hdd_set_mas(hdd_adapter_t *adapter, tANI_U8 mas_value)
{
	hdd_context_t *hdd_ctx = NULL;
	eHalStatus ret_status;

	hdd_ctx = WLAN_HDD_GET_CTX(adapter);
	if (!hdd_ctx)
		return -EFAULT;

	if (mas_value) {
		/* Miracast is ON. Disable MAS and configure P2P quota */
		if (hdd_ctx->cfg_ini->enableMCCAdaptiveScheduler) {
			if (cfgSetInt(hdd_ctx->hHal,
				WNI_CFG_ENABLE_MCC_ADAPTIVE_SCHED, 0)
				!= eSIR_SUCCESS) {
				hddLog(LOGE,
					"Could not pass on WNI_CFG_ENABLE_MCC_ADAPTIVE_SCHED to CCM");
			}

			ret_status = sme_set_mas(false);
			if (eHAL_STATUS_SUCCESS != ret_status) {
				hddLog(LOGE, "Failed to disable MAS");
				return -EBUSY;
			}
		}

		/* Config p2p quota */
		if (adapter->device_mode == WLAN_HDD_INFRA_STATION)
			hdd_wlan_set_mcc_p2p_quota(adapter,
				100 - HDD_DEFAULT_MCC_P2P_QUOTA);
		else if (adapter->device_mode == WLAN_HDD_P2P_GO)
			hdd_wlan_go_set_mcc_p2p_quota(adapter,
				HDD_DEFAULT_MCC_P2P_QUOTA);
		else
			hdd_wlan_set_mcc_p2p_quota(adapter,
				HDD_DEFAULT_MCC_P2P_QUOTA);
	} else {
		/* Reset p2p quota */
		if (adapter->device_mode == WLAN_HDD_P2P_GO)
			hdd_wlan_go_set_mcc_p2p_quota(adapter,
						HDD_RESET_MCC_P2P_QUOTA);
		else
			hdd_wlan_set_mcc_p2p_quota(adapter,
						HDD_RESET_MCC_P2P_QUOTA);

		/* Miracast is OFF. Enable MAS and reset P2P quota */
		if (hdd_ctx->cfg_ini->enableMCCAdaptiveScheduler) {
			if (cfgSetInt(hdd_ctx->hHal,
				WNI_CFG_ENABLE_MCC_ADAPTIVE_SCHED, 1)
				!= eSIR_SUCCESS) {
				hddLog(LOGE, "Could not pass on WNI_CFG_ENABLE_MCC_ADAPTIVE_SCHED to CCM");
			}

			/* Enable MAS */
			ret_status = sme_set_mas(true);
			if (eHAL_STATUS_SUCCESS != ret_status) {
				hddLog(LOGE, "Unable to enable MAS");
				return -EBUSY;
			}
		}
	}

	return 0;
}

/**
 * hdd_is_mcc_in_24G() - Function to check for MCC in 2.4GHz
 * @hdd_ctx:	Pointer to HDD context
 *
 * This function is used to check for MCC operation in 2.4GHz band.
 * STA, P2P and SAP adapters are only considered.
 *
 * Return: Non zero value if MCC is detected in 2.4GHz band
 *
 */
uint8_t hdd_is_mcc_in_24G(hdd_context_t *hdd_ctx)
{
	VOS_STATUS status;
	hdd_adapter_t *hdd_adapter = NULL;
	hdd_adapter_list_node_t *adapter_node = NULL, *next = NULL;
	uint8_t ret = 0;
	hdd_station_ctx_t *sta_ctx;
	hdd_ap_ctx_t *ap_ctx;
	uint8_t ch1 = 0, ch2 = 0;
	uint8_t channel = 0;
	hdd_hostapd_state_t *hostapd_state;

	status =  hdd_get_front_adapter(hdd_ctx, &adapter_node);

	/* loop through all adapters and check MCC for STA,P2P,SAP adapters */
	while (NULL != adapter_node && VOS_STATUS_SUCCESS == status) {
		hdd_adapter = adapter_node->pAdapter;

		if (!((hdd_adapter->device_mode >= WLAN_HDD_INFRA_STATION)
					&& (hdd_adapter->device_mode
						<= WLAN_HDD_P2P_GO))) {
			/* skip for other adapters */
			status = hdd_get_next_adapter(hdd_ctx,
					adapter_node, &next);
			adapter_node = next;
			continue;
		} else {
			if (WLAN_HDD_INFRA_STATION ==
					hdd_adapter->device_mode ||
					WLAN_HDD_P2P_CLIENT ==
					hdd_adapter->device_mode) {
				sta_ctx =
					WLAN_HDD_GET_STATION_CTX_PTR(
								hdd_adapter);
				if (eConnectionState_Associated ==
						sta_ctx->conn_info.connState)
					channel =
						sta_ctx->conn_info.
							operationChannel;
			} else if (WLAN_HDD_P2P_GO ==
					hdd_adapter->device_mode ||
					WLAN_HDD_SOFTAP ==
					hdd_adapter->device_mode) {
				ap_ctx = WLAN_HDD_GET_AP_CTX_PTR(hdd_adapter);
				hostapd_state =
					WLAN_HDD_GET_HOSTAP_STATE_PTR(
								hdd_adapter);
				if (hostapd_state->bssState == BSS_START &&
						hostapd_state->vosStatus ==
						VOS_STATUS_SUCCESS)
					channel = ap_ctx->operatingChannel;
			}

			if ((ch1 == 0) ||
					((ch2 != 0) && (ch2 != channel))) {
				ch1 = channel;
			} else if ((ch2 == 0) ||
					((ch1 != 0) && (ch1 != channel))) {
				ch2 = channel;
			}

			if ((ch1 != 0 && ch2 != 0) && (ch1 != ch2) &&
					((ch1 <= SIR_11B_CHANNEL_END) &&
					 (ch2 <= SIR_11B_CHANNEL_END))) {
				hddLog(LOGE,
					"MCC in 2.4Ghz on channels %d and %d",
					ch1, ch2);
				return 1;
			}

			status = hdd_get_next_adapter(hdd_ctx,
					adapter_node, &next);
			adapter_node = next;
		}
	}
	return ret;
}

/**
 * wlan_hdd_get_link_speed() - get link speed
 * @pAdapter:     pointer to the adapter
 * @link_speed:   pointer to link speed
 *
 * This function fetches per bssid link speed.
 *
 * Return: if associated, link speed shall be returned.
 *         if not associated, link speed of 0 is returned.
 *         On error, error number will be returned.
 */
int wlan_hdd_get_link_speed(hdd_adapter_t *sta_adapter, uint32_t *link_speed)
{
    hdd_context_t *hddctx = WLAN_HDD_GET_CTX(sta_adapter);
    hdd_station_ctx_t *hdd_stactx = WLAN_HDD_GET_STATION_CTX_PTR(sta_adapter);
    int ret;

    ret = wlan_hdd_validate_context(hddctx);
    if (0 != ret)
        return ret;

    if (eConnectionState_Associated != hdd_stactx->conn_info.connState) {
       /* we are not connected so we don't have a classAstats */
       *link_speed = 0;
    } else {
        VOS_STATUS status;
        tSirMacAddr bssid;

        vos_mem_copy(bssid, hdd_stactx->conn_info.bssId, VOS_MAC_ADDR_SIZE);

        status = wlan_hdd_get_linkspeed_for_peermac(sta_adapter, bssid);
        if (!VOS_IS_STATUS_SUCCESS(status)) {
           hddLog(LOGE, FL("Unable to retrieve SME linkspeed"));
           return -EINVAL;
        }
        *link_speed = sta_adapter->ls_stats.estLinkSpeed;
        /* linkspeed in units of 500 kbps */
        *link_speed = (*link_speed) / 500;
    }
    return 0;
}

#if defined(FEATURE_WLAN_ESE) && defined(FEATURE_WLAN_ESE_UPLOAD)
/**
 * hdd_parse_ese_beacon_req() - Parse ese beacon request
 * @pValue:     Pointer to input data (its a NUL terminated string)
 * @pEseBcnReq: pEseBcnReq output pointer to store parsed ie information
 *
 * This function parses the ese beacon request passed in the format
 * CCXBEACONREQ<space><Number of fields><space><Measurement token>
 * <space>Channel 1<space>Scan Mode <space>Meas Duration<space>Channel N
 * <space>Scan Mode N<space>Meas Duration N
 * if the Number of bcn req fields (N) does not match with the actual number of
 * fields passed then take N.
 * <Meas Token><Channel><Scan Mode> and <Meas Duration> are treated as one pair
 * For example, CCXBEACONREQ 2 1 1 1 30 2 44 0 40.
 * This function does not take care of removing duplicate channels from the list
 *
 * Return: 0 for success non-zero for failure
 */
static VOS_STATUS hdd_parse_ese_beacon_req(tANI_U8 *pValue,
                                     tCsrEseBeaconReq *pEseBcnReq)
{
    tANI_U8 *inPtr = pValue;
    uint8_t input = 0;
    uint32_t tempInt = 0;
    int j = 0, i = 0, v = 0;
    char buf[32];

    inPtr = strnchr(pValue, strlen(pValue), SPACE_ASCII_VALUE);
    /* no argument after the command */
    if (NULL == inPtr) {
        return -EINVAL;
    } else if (SPACE_ASCII_VALUE != *inPtr) {
        /* no space after the command */
        return -EINVAL;
    }

    /* removing empty spaces */
    while ((SPACE_ASCII_VALUE  == *inPtr) && ('\0' !=  *inPtr)) inPtr++;

    /* no argument followed by spaces */
    if ('\0' == *inPtr) return -EINVAL;

    /* Getting the first argument ie Number of IE fields */
    v = sscanf(inPtr, "%31s ", buf);
    if (1 != v) return -EINVAL;

    v = kstrtou8(buf, 10, &input);
    if (v < 0) return -EINVAL;

    input = VOS_MIN(input, SIR_ESE_MAX_MEAS_IE_REQS);
    pEseBcnReq->numBcnReqIe = input;

    hddLog(LOG1, "Number of Bcn Req Ie fields: %d", pEseBcnReq->numBcnReqIe);

    for (j = 0; j < (pEseBcnReq->numBcnReqIe); j++) {
        for (i = 0; i < 4; i++) {
            /* inPtr pointing to the beginning of first space after number of
               ie fields */
            inPtr = strpbrk( inPtr, " " );
            /* no ie data after the number of ie fields argument */
            if (NULL == inPtr) return -EINVAL;

            /* removing empty space */
            while ((SPACE_ASCII_VALUE == *inPtr) && ('\0' != *inPtr)) inPtr++;

            /* no ie data after the number of ie fields argument and spaces */
            if ('\0' == *inPtr) return -EINVAL;

            v = sscanf(inPtr, "%31s ", buf);
            if (1 != v) return -EINVAL;

            v = kstrtou32(buf, 10, &tempInt);
            if (v < 0) return -EINVAL;

            switch (i) {
            case 0:  /* Measurement token */
                if (!tempInt) {
                   VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                             "Invalid Measurement Token: %d", tempInt);
                   return -EINVAL;
                }
                pEseBcnReq->bcnReq[j].measurementToken = tempInt;
                break;

            case 1:  /* Channel number */
                if ((!tempInt) ||
                    (tempInt > WNI_CFG_CURRENT_CHANNEL_STAMAX)) {
                   VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                             "Invalid Channel Number: %d", tempInt);
                   return -EINVAL;
                }
                pEseBcnReq->bcnReq[j].channel = tempInt;
                break;

            case 2:  /* Scan mode */
                if ((tempInt < eSIR_PASSIVE_SCAN) ||
                    (tempInt > eSIR_BEACON_TABLE)) {
                   VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                             "Invalid Scan Mode: %d Expected{0|1|2}", tempInt);
                   return -EINVAL;
                }
                pEseBcnReq->bcnReq[j].scanMode= tempInt;
                break;

            case 3:  /* Measurement duration */
                if (((!tempInt) &&
                    (pEseBcnReq->bcnReq[j].scanMode != eSIR_BEACON_TABLE)) ||
                    ((pEseBcnReq->bcnReq[j].scanMode == eSIR_BEACON_TABLE))) {
                   VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                             "Invalid Measurement Duration: %d", tempInt);
                   return -EINVAL;
                }
                pEseBcnReq->bcnReq[j].measurementDuration = tempInt;
                break;
            }
        }
    }

    for (j = 0; j < pEseBcnReq->numBcnReqIe; j++) {
        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                   "Index(%d) Measurement Token(%u) Channel(%u) Scan Mode(%u) Measurement Duration(%u)",
                   j,
                   pEseBcnReq->bcnReq[j].measurementToken,
                   pEseBcnReq->bcnReq[j].channel,
                   pEseBcnReq->bcnReq[j].scanMode,
                   pEseBcnReq->bcnReq[j].measurementDuration);
    }

    return VOS_STATUS_SUCCESS;
}

struct tsm_priv {
	tAniTrafStrmMetrics tsm_metrics;
};

static void hdd_GetTsmStatsCB(tAniTrafStrmMetrics tsmMetrics,
			      const tANI_U32 staId,
			      void *pContext)
{
	struct hdd_request *request;
	struct tsm_priv *priv;

	ENTER();
	request = hdd_request_get(pContext);
	if (!request) {
		hddLog(LOGE, FL("Obsolete request"));
		return;
	}
	priv = hdd_request_priv(request);
	priv->tsm_metrics = tsmMetrics;
	hdd_request_complete(request);
	hdd_request_put(request);
	EXIT();
}

static VOS_STATUS hdd_get_tsm_stats(hdd_adapter_t *pAdapter,
				    const tANI_U8 tid,
				    tAniTrafStrmMetrics* pTsmMetrics)
{
	hdd_station_ctx_t  *pHddStaCtx = NULL;
	eHalStatus          hstatus;
	VOS_STATUS          vstatus = VOS_STATUS_SUCCESS;
	int                 ret;
	hdd_context_t      *pHddCtx = NULL;
	void               *cookie;
	struct hdd_request *request;
	struct tsm_priv    *priv;
	static const struct hdd_request_params params = {
		.priv_size = sizeof(*priv),
		.timeout_ms = WLAN_WAIT_TIME_STATS,
	};

	if (!pAdapter) {
		hddLog(LOGE, FL("pAdapter is NULL"));
		return VOS_STATUS_E_FAULT;
	}

	pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
	pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);

	request = hdd_request_alloc(&params);
	if (!request) {
		hddLog(LOGE, FL("Request allocation failure"));
		return VOS_STATUS_E_NOMEM;
	}
	cookie = hdd_request_cookie(request);

	/* query tsm stats */
	hstatus = sme_GetTsmStats(pHddCtx->hHal, hdd_GetTsmStatsCB,
				  pHddStaCtx->conn_info.staId[ 0 ],
				  pHddStaCtx->conn_info.bssId,
				  cookie, pHddCtx->pvosContext, tid);
	if (eHAL_STATUS_SUCCESS != hstatus) {
		hddLog(VOS_TRACE_LEVEL_ERROR,
		       "%s: Unable to retrieve tsm statistics",
		       __func__);
		vstatus = VOS_STATUS_E_FAULT;
		goto cleanup;
	}

	ret = hdd_request_wait_for_response(request);
	if (ret) {
		hddLog(LOGE,
		       FL("SME timed out while retrieving tsm statistics"));
		vstatus = VOS_STATUS_E_TIMEOUT;
		goto cleanup;
	}

	priv = hdd_request_priv(request);
	*pTsmMetrics = priv->tsm_metrics;

cleanup:
	hdd_request_put(request);

	return vstatus;
}

/**
 * hdd_parse_get_cckm_ie() - HDD Parse and fetch the CCKM IE
 * @pValue:  Pointer to input data
 * @pCckmIe: Pointer to output cckm Ie
 * @pCckmIeLen: Pointer to output cckm ie length
 *
 * This function parses the SETCCKM IE command
 *
 * Return: 0 for success non-zero for failure
 */
static VOS_STATUS
hdd_parse_get_cckm_ie(tANI_U8 *pValue, tANI_U8 **pCckmIe, tANI_U8 *pCckmIeLen)
{
    tANI_U8 *inPtr = pValue;
    tANI_U8 *dataEnd;
    int      j = 0;
    int      i = 0;
    tANI_U8  tempByte = 0;
    inPtr = strnchr(pValue, strlen(pValue), SPACE_ASCII_VALUE);
    /* no argument after the command */
    if (NULL == inPtr) {
        return -EINVAL;
    } else if (SPACE_ASCII_VALUE != *inPtr) {
        /* no space after the command */
        return -EINVAL;
    }
    /* removing empty spaces */
    while ((SPACE_ASCII_VALUE  == *inPtr) && ('\0' !=  *inPtr) ) inPtr++;
    /* no argument followed by spaces */
    if ('\0' == *inPtr) {
        return -EINVAL;
    }
    /* find the length of data */
    dataEnd = inPtr;
    while('\0' !=  *dataEnd) {
        dataEnd++;
        ++(*pCckmIeLen);
    }
    if (*pCckmIeLen <= 0)  return -EINVAL;
    /*
     * Allocate the number of bytes based on the number of input characters
     * whether it is even or odd.
     * if the number of input characters are even, then we need N/2 byte.
     * if the number of input characters are odd, then we need do (N+1)/2 to
     * compensate rounding off.
     * For example, if N = 18, then (18 + 1)/2 = 9 bytes are enough.
     * If N = 19, then we need 10 bytes, hence (19 + 1)/2 = 10 bytes
     */
    *pCckmIe = vos_mem_malloc((*pCckmIeLen + 1)/2);
    if (NULL == *pCckmIe) {
        hddLog(LOGP, FL("vos_mem_alloc failed"));
        return -ENOMEM;
    }
    vos_mem_zero(*pCckmIe, (*pCckmIeLen + 1)/2);
    /*
     * the buffer received from the upper layer is character buffer,
     * we need to prepare the buffer taking 2 characters in to a U8 hex
     * decimal number for example 7f0000f0...form a buffer to contain
     * 7f in 0th location, 00 in 1st and f0 in 3rd location
     */
    for (i = 0, j = 0; j < *pCckmIeLen; j += 2) {
        tempByte = (hdd_parse_hex(inPtr[j]) << 4)
                   | (hdd_parse_hex(inPtr[j + 1]));
        (*pCckmIe)[i++] = tempByte;
    }
    *pCckmIeLen = i;
    return VOS_STATUS_SUCCESS;
}

#endif /*FEATURE_WLAN_ESE && FEATURE_WLAN_ESE_UPLOAD */

/**
 * drv_cmd_set_fcc_channel() - handle fcc constraint request
 * @hdd_ctx: HDD context
 * @cmd: command ptr
 * @cmd_len: command len
 *
 * Return: status
 */
static int drv_cmd_set_fcc_channel(hdd_adapter_t *adapter, uint8_t *cmd,
                                   uint8_t cmd_len)
{
	uint8_t *value;
	uint8_t fcc_constraint;
	eHalStatus status;
	int ret = 0;
        hdd_context_t *hdd_ctx = WLAN_HDD_GET_CTX(adapter);

	value =  cmd + cmd_len + 1;

	ret = kstrtou8(value, 10, &fcc_constraint);
	if ((ret < 0) || (fcc_constraint > 1)) {
		/*
		 *  If the input value is greater than max value of datatype,
		 *  then also it is a failure
		 */
		hddLog(LOGE, FL("value out of range"));
		return -EINVAL;
	}

	status = sme_handle_set_fcc_channel(hdd_ctx->hHal, !fcc_constraint,
			adapter->scan_info.mScanPending);
	if (status != eHAL_STATUS_SUCCESS)
		ret = -EPERM;

	return ret;
}

/**
 * hdd_set_rx_filter() - set RX filter
 * @adapter: Pointer to adapter
 * @action: Filter action
 * @pattern: Address pattern
 *
 * Address pattern is most significant byte of address for example
 * 0x01 for IPV4 multicast address
 * 0x33 for IPV6 multicast address
 * 0xFF for broadcast address
 *
 * Return: 0 for success, non-zero for failure
 */
static int hdd_set_rx_filter(hdd_adapter_t *adapter, bool action,
			uint8_t pattern)
{
	int ret;
	uint8_t i, j;
	tHalHandle handle;
	tSirRcvFltMcAddrList *filter;
	hdd_context_t* hdd_ctx = WLAN_HDD_GET_CTX(adapter);

	ret = wlan_hdd_validate_context(hdd_ctx);
	if (0 != ret)
		return ret;

	handle = hdd_ctx->hHal;

	if (NULL == handle) {
		hddLog(LOGE, FL("HAL Handle is NULL"));
		return -EINVAL;
	}

	if (!hdd_ctx->cfg_ini->fEnableMCAddrList) {
		hddLog(VOS_TRACE_LEVEL_ERROR, FL("mc addr ini is disabled"));
		return -EINVAL;
	}

	/*
	 * If action is false it means start dropping packets
	 * Set addr_filter_pattern which will be used when sending
	 * MC/BC address list to target
	 */
	if (!action)
		adapter->addr_filter_pattern = pattern;
	else
		adapter->addr_filter_pattern = 0;

	if (((adapter->device_mode == WLAN_HDD_INFRA_STATION) ||
		(adapter->device_mode == WLAN_HDD_P2P_CLIENT)) &&
		adapter->mc_addr_list.mc_cnt &&
		hdd_connIsConnected(WLAN_HDD_GET_STATION_CTX_PTR(adapter))) {


		filter = vos_mem_malloc(sizeof(*filter));
		if (NULL == filter) {
			hddLog(LOGE, FL("Could not allocate Memory"));
			return -ENOMEM;
		}
		vos_mem_zero(filter, sizeof(*filter));
		filter->action = action;
		for (i = 0, j = 0; i < adapter->mc_addr_list.mc_cnt; i++) {
			if (!memcmp(&adapter->mc_addr_list.addr[i * ETH_ALEN],
				&pattern, 1)) {
				memcpy(filter->multicastAddr[j],
				    &adapter->mc_addr_list.addr[i * ETH_ALEN],
				    ETH_ALEN);
				hddLog(LOG1, "%s RX filter : addr ="
				    MAC_ADDRESS_STR,
				    action ? "setting" : "clearing",
				    MAC_ADDR_ARRAY(filter->multicastAddr[j]));
				j++;
			}
			if (j == SIR_MAX_NUM_MULTICAST_ADDRESS)
				break;
		}
		filter->ulMulticastAddrCnt = j;
		/* Set rx filter */
		sme_8023MulticastList(handle, adapter->sessionId, filter);
		vos_mem_free(filter);
	} else {
		hddLog(LOGW, FL("mode %d mc_cnt %d"),
			adapter->device_mode, adapter->mc_addr_list.mc_cnt);
	}

	return 0;
}

/**
 * hdd_driver_rxfilter_comand_handler() - RXFILTER driver command handler
 * @command: Pointer to input string driver command
 * @adapter: Pointer to adapter
 * @action: Action to enable/disable filtering
 *
 * If action == false
 * Start filtering out data packets based on type
 * RXFILTER-REMOVE 0 -> Start filtering out unicast data packets
 * RXFILTER-REMOVE 1 -> Start filtering out broadcast data packets
 * RXFILTER-REMOVE 2 -> Start filtering out IPV4 mcast data packets
 * RXFILTER-REMOVE 3 -> Start filtering out IPV6 mcast data packets
 *
 * if action == true
 * Stop filtering data packets based on type
 * RXFILTER-ADD 0 -> Stop filtering unicast data packets
 * RXFILTER-ADD 1 -> Stop filtering broadcast data packets
 * RXFILTER-ADD 2 -> Stop filtering IPV4 mcast data packets
 * RXFILTER-ADD 3 -> Stop filtering IPV6 mcast data packets
 *
 * Current implementation only supports IPV4 address filtering by
 * selectively allowing IPV4 multicast data packest based on
 * address list received in .ndo_set_rx_mode
 *
 * Return: 0 for success, non-zero for failure
 */
static int hdd_driver_rxfilter_comand_handler(uint8_t *command,
						hdd_adapter_t *adapter,
						bool action)
{
	int ret = 0;
	uint8_t *value;
	uint8_t type;

	value = command;
	/* Skip space after RXFILTER-REMOVE OR RXFILTER-ADD based on action */
	if (!action)
		value = command + 16;
	else
		value = command + 13;
	ret = kstrtou8(value, 10, &type);
	if (ret < 0) {
		hddLog(LOGE,
			FL("kstrtou8 failed invalid input value"));
		return -EINVAL;
	}

	switch (type) {
	case 2:
		/* Set rx filter for IPV4 multicast data packets */
		ret = hdd_set_rx_filter(adapter, action, 0x01);
		break;
	default:
		hddLog(LOG1, FL("Unsupported RXFILTER type %d"), type);
		break;
	}

	return ret;
}

/**
 * hdd_parse_setantennamode_command() - HDD Parse SETANTENNAMODE
 * command
 * @value: Pointer to SETANTENNAMODE command
 * @mode: Pointer to antenna mode
 *
 * This function parses the SETANTENNAMODE command passed in the format
 * SETANTENNAMODE<space>mode
 *
 * Return: 0 for success non-zero for failure
 */
static int hdd_parse_setantennamode_command(const uint8_t *value,
					int *mode)
{
	const uint8_t *in_ptr = value;
	int tmp, v;
	char arg1[32];
	*mode = 0;

	in_ptr = strnchr(value, strlen(value), SPACE_ASCII_VALUE);

	/* no argument after the command */
	if (NULL == in_ptr) {
		hddLog(LOGE, FL("No argument after the command"));
		return -EINVAL;
	}

	/* no space after the command */
	if (SPACE_ASCII_VALUE != *in_ptr) {
		hddLog(LOGE, FL("No space after the command"));
		return -EINVAL;
	}

	/* remove empty spaces */
	while ((SPACE_ASCII_VALUE == *in_ptr) && ('\0' != *in_ptr))
		in_ptr++;

	/* no argument followed by spaces */
	if ('\0' == *in_ptr) {
		hddLog(LOGE, FL("No argument followed by spaces"));
		return -EINVAL;
	}

	/* get the argument i.e. antenna mode */
	v = sscanf(in_ptr, "%31s ", arg1);
	if (1 != v) {
		hddLog(LOGE, FL("argument retrieval from cmd string failed"));
		return -EINVAL;
	}

	v = kstrtos32(arg1, 10, &tmp);
	if (v < 0) {
		hddLog(LOGE, FL("argument string to integer conversion failed"));
		return -EINVAL;
	}
	*mode = tmp;

	return 0;
}

/**
 * hdd_is_supported_chain_mask_2x2() - Verify if supported chain
 * mask is 2x2 mode
 * @hdd_ctx: Pointer to hdd contex
 *
 * Return: true if supported chain mask 2x2 else false
 */
static bool hdd_is_supported_chain_mask_2x2(hdd_context_t *hdd_ctx)
{
	hdd_config_t *config = hdd_ctx->cfg_ini;

	if (hdd_ctx->per_band_chainmask_supp == 0x01) {
		return (((hdd_ctx->supp_2g_chain_mask & 0x03)
			 == 0x03) ||
			((hdd_ctx->supp_5g_chain_mask & 0x03)
			 == 0x03)) ? true : false;
	}

	return (config->enable2x2 == 0x01) ? true : false;
}

/**
 * hdd_is_supported_chain_mask_1x1() - Verify if the supported
 * chain mask is 1x1
 * @hdd_ctx: Pointer to hdd contex
 *
 * Return: true if supported chain mask 1x1 else false
 */
static bool hdd_is_supported_chain_mask_1x1(hdd_context_t *hdd_ctx)
{
	hdd_config_t *config = hdd_ctx->cfg_ini;

	if (hdd_ctx->per_band_chainmask_supp == 0x01) {
		return ((hdd_ctx->supp_2g_chain_mask <= 0x02) &&
			(hdd_ctx->supp_5g_chain_mask <= 0x02)) ?
			true : false;
	}

	return (!config->enable2x2) ? true : false;
}

/**
 * switch_antenna_mode_non_conn_state() - Dynamic switch to 1x1
 * antenna mode when there are no connections
 * @hdd_ctx: Pointer to hdd contex
 * @adapter: Pointer to hdd adapter
 * @chains: Number of TX/RX chains to set
 *
 * Return: 0 if success else non zero
 */
static int switch_antenna_mode_non_conn_state(hdd_context_t *hdd_ctx,
					      hdd_adapter_t *adapter,
					      uint8_t chains)
{
	int ret;
	eHalStatus hal_status;
	bool enable_smps;
	int smps_mode;

	ret = wlan_hdd_update_txrx_chain_mask(hdd_ctx,
					(chains == 2) ? 0x3 : 0x1);

	if (0 != ret) {
		hddLog(LOGE,
		       FL("Failed to update chain mask: %d"),
		       chains);
		return ret;
	}

	/* Update HT SMPS as static/disabled in the SME configuration
	 * If there is STA connection followed by dynamic switch
	 * to 1x1 protocol stack would include SM power save IE as
	 * static in the assoc mgmt frame and after association
	 * SMPS force mode command will be sent to FW to initiate
	 * SMPS action frames to AP. In this case, SMPS force mode
	 * command event can be expected from firmware with the
	 * TX status of SMPS action frames. Inclusion of SM power
	 * save IE and sending of SMPS action frames will not happen
	 * for switch to 2x2 mode. But SME config should still be
	 * updated to disabled.
	 */
	adapter->smps_force_mode_status = 0;

	enable_smps = (chains == 1) ? true : false;
	smps_mode = (chains == 1) ? HDD_SMPS_MODE_STATIC :
			HDD_SMPS_MODE_DISABLED;

	hal_status = sme_update_mimo_power_save(hdd_ctx->hHal,
			enable_smps, smps_mode);
	if (eHAL_STATUS_SUCCESS != hal_status) {
		hddLog(LOG1,
		       FL("Update MIMO power SME config failed: %d"),
		       hal_status);
		return -EFAULT;
	}

	hddLog(LOG1, FL("Updated SME config enable smps: %d mode: %d"),
	       enable_smps, smps_mode);

	return 0;
}

/**
 * switch_to_1x1_connected_sta_state() - Dynamic switch to 1x1
 * antenna mode in standalone station
 * @hdd_ctx: Pointer to hdd contex
 * @adapter: Pointer to hdd adapter
 *
 * Return: 0 if success else non zero
 */
static int switch_to_1x1_connected_sta_state(hdd_context_t *hdd_ctx,
					     hdd_adapter_t *adapter)
{
	int ret;
	eHalStatus hal_status;
	bool send_smps;

	/*  Check TDLS status and update antenna mode */
	ret = wlan_hdd_tdls_antenna_switch(hdd_ctx, adapter,
					   HDD_ANTENNA_MODE_1X1);
	if (0 != ret)
		return ret;

	/* If intersection of sta and AP NSS is 1x1 then
	 * skip SMPS indication to AP. Only update the chain mask
	 * and other configuration.
	 */
	send_smps = sme_is_sta_smps_allowed(hdd_ctx->hHal,
					adapter->sessionId);
	if (!send_smps) {
		hddLog(LOGE, FL("Need not indicate SMPS to AP"));
		goto chain_mask;
	}

	INIT_COMPLETION(adapter->smps_force_mode_comp_var);

	hddLog(LOG1, FL("Send SMPS force mode command"));
	ret = process_wma_set_command((int)adapter->sessionId,
				WMI_STA_SMPS_FORCE_MODE_CMDID,
				WMI_SMPS_FORCED_MODE_STATIC,
				VDEV_CMD);
	if (0 != ret) {
		hddLog(LOGE,
		       FL("Failed to send SMPS force mode to static"));
		return ret;
	}

	/* Block on SMPS force mode event only for mode static */
	ret = wait_for_completion_timeout(
		&adapter->smps_force_mode_comp_var,
		msecs_to_jiffies(WLAN_WAIT_SMPS_FORCE_MODE));
	if (!ret) {
		hddLog(LOGE,
			FL("SMPS force mode event timeout: %d"),
			ret);
		return -EFAULT;
	}
	ret = adapter->smps_force_mode_status;
	adapter->smps_force_mode_status = 0;
	if (0 != ret) {
		hddLog(LOGE, FL("SMPS force mode status: %d "),
		       ret);
		return ret;
	}

chain_mask:
	hddLog(LOG1, FL("Update chain mask to 1x1"));
	ret = wlan_hdd_update_txrx_chain_mask(hdd_ctx, 1);
	if (0 != ret) {
		hddLog(LOGE, FL("Failed to switch to 1x1 mode"));
		return ret;
	}

	/* Update SME SM power save config */
	hal_status = sme_update_mimo_power_save(hdd_ctx->hHal,
					true, HDD_SMPS_MODE_STATIC);
	if (eHAL_STATUS_SUCCESS != hal_status) {
		hddLog(LOG1,
		       FL("Failed to update SMPS config to static: %d"),
		       hal_status);
		return -EFAULT;
	}

	hddLog(LOG1, FL("Successfully switched to 1x1 mode"));
	return 0;
}

/**
 * switch_to_2x2_connected_sta_state() - Dynamic switch to 2x2
 * antenna mode in standalone station
 * @hdd_ctx: Pointer to hdd contex
 * @adapter: Pointer to hdd adapter
 *
 * Return: 0 if success else non zero
 */
static int switch_to_2x2_connected_sta_state(hdd_context_t *hdd_ctx,
					     hdd_adapter_t *adapter)
{
	int ret;
	eHalStatus hal_status;
	bool send_smps;

	/*  Check TDLS status and update antenna mode */
	ret = wlan_hdd_tdls_antenna_switch(hdd_ctx, adapter,
					   HDD_ANTENNA_MODE_2X2);
	if (0 != ret)
		return ret;

	hddLog(LOG1, FL("Update chain mask to 2x2"));
	ret = wlan_hdd_update_txrx_chain_mask(hdd_ctx, 3);
	if (0 != ret) {
		hddLog(LOGE, FL("Failed to switch to 2x2 mode"));
		return ret;
	}

	/* If intersection of sta and AP NSS is 1x1 then
	 * skip SMPS indication to AP.
	 */
	send_smps = sme_is_sta_smps_allowed(hdd_ctx->hHal,
					adapter->sessionId);
	if (!send_smps) {
		hddLog(LOGE, FL("Need not indicate SMPS to AP"));
		goto exit;
	}

	hddLog(LOG1, FL("Send SMPS force mode command "));

	/* No need to block on SMPS force mode event when
	 * the mode switch is 2x2 since the chain mask
	 * has already been updated to 2x2
	 */
	ret = process_wma_set_command((int)adapter->sessionId,
				WMI_STA_SMPS_FORCE_MODE_CMDID,
				WMI_SMPS_FORCED_MODE_DISABLED,
				VDEV_CMD);
	if (0 != ret) {
		hddLog(LOGE,
		       FL("Failed to send SMPS force mode to disabled"));
		return ret;
	}

exit:
	/* Update SME SM power save config */
	hal_status = sme_update_mimo_power_save(hdd_ctx->hHal,
				false, HDD_SMPS_MODE_DISABLED);
	if (eHAL_STATUS_SUCCESS != hal_status) {
		hddLog(LOG1,
		       FL("Failed to update SMPS config to disable: %d"),
		       hal_status);
		return -EFAULT;
	}

	hddLog(LOG1, FL("Successfully switched to 2x2 mode"));
	return 0;
}

/**
 * hdd_decide_dynamic_chain_mask() - Dynamically decide and set chain mask
 * for STA
 * @hdd_ctx: Pointer to hdd context
 * @forced_mode: set to valid mode to force the mode
 *
 * This function decides the chain mask to set considering number of
 * active sessions, type of active sessions and concurrency.
 * If forced_mode is valid, chan mask is set to the forced_mode.
 *
 * Return: void
 */
void hdd_decide_dynamic_chain_mask(hdd_context_t *hdd_ctx,
	enum antenna_mode forced_mode)
{
	int ret;
	hdd_adapter_t *sta_adapter;
	hdd_station_ctx_t *hdd_sta_ctx;
	enum antenna_mode mode;

	if (wlan_hdd_validate_context(hdd_ctx))
		return;

	if (!hdd_ctx->cfg_ini->enable_dynamic_sta_chainmask ||
	    !hdd_is_supported_chain_mask_2x2(hdd_ctx)) {
		hddLog(LOG1,
			FL("dynamic antenna mode in INI is %d or 2x2 not supported"),
			hdd_ctx->cfg_ini->enable_dynamic_sta_chainmask);
		return;
	}
	hddLog(LOG1, FL("Current antenna mode: %d"),
		hdd_ctx->current_antenna_mode);

	if (HDD_ANTENNA_MODE_INVALID != forced_mode) {
		mode = forced_mode;
	} else if (!wlan_hdd_get_active_session_count(hdd_ctx)) {
		/* If no active connection set 1x1 */
		mode = HDD_ANTENNA_MODE_1X1;
	} else if (1 == wlan_hdd_get_active_session_count(hdd_ctx) &&
		   hdd_ctx->no_of_active_sessions[WLAN_HDD_INFRA_STATION]) {
		sta_adapter = hdd_get_adapter(hdd_ctx, WLAN_HDD_INFRA_STATION);
		if (!sta_adapter) {
			hddLog(LOGE, FL("Sta adapter null!!"));
			return;
		}
		hdd_sta_ctx = WLAN_HDD_GET_STATION_CTX_PTR(sta_adapter);

		if (!hdd_connIsConnected(hdd_sta_ctx)) {
			hddLog(LOGE, FL("Sta not connected"));
			return;
		}
		/*
		 * In case only sta is connected set value depending
		 * on AP capability.
		 */
		mode = hdd_sta_ctx->conn_info.nss;
	} else {
		/* If non sta is active or concurrency set 2x2 */
		mode = HDD_ANTENNA_MODE_2X2;
	}

	if (mode != hdd_ctx->current_antenna_mode) {

		ret = wlan_hdd_update_txrx_chain_mask(hdd_ctx,
			(HDD_ANTENNA_MODE_2X2 == mode) ? 0x3 : 0x1);

		if (0 != ret) {
			hddLog(LOGE,
				FL("Failed to update chain mask: %d"),
				mode);
			return;
		}
		hdd_ctx->current_antenna_mode = mode;

		hddLog(LOG1, FL("Antenna mode updated to: %d"),
			hdd_ctx->current_antenna_mode);
	}
}

/**
 * drv_cmd_set_antenna_mode() - SET ANTENNA MODE driver command
 * handler
 * @hdd_ctx: Pointer to hdd context
 * @cmd: Pointer to input command
 * @command_len: Command length
 *
 * Return: 0 for success non-zero for failure
 */
static int drv_cmd_set_antenna_mode(hdd_adapter_t *adapter,
				    uint8_t *command,
				    uint8_t cmd_len)
{
	int ret;
	int mode;
	uint8_t *value = command;
	hdd_context_t *hdd_ctx = WLAN_HDD_GET_CTX(adapter);

	if ((hdd_ctx->concurrency_mode > 1) ||
	    (hdd_ctx->no_of_active_sessions[WLAN_HDD_INFRA_STATION] > 1)) {
		hddLog(LOGE, FL("Operation invalid in non sta or concurrent mode"));
		ret = -EPERM;
		goto exit;
	}

	ret = hdd_parse_setantennamode_command(value, &mode);
	if (0 != ret) {
		hddLog(LOGE, FL("Invalid SETANTENNA command"));
		goto exit;
	}

	hddLog(LOG1, FL("Request to switch antenna mode to: %d"), mode);

	if (hdd_ctx->current_antenna_mode == mode) {
		hddLog(LOGE, FL("System already in the requested mode"));
		ret = 0;
		goto exit;
	}

	if ((HDD_ANTENNA_MODE_2X2 == mode) &&
	    (!hdd_is_supported_chain_mask_2x2(hdd_ctx))) {
		hddLog(LOGE, FL("System does not support 2x2 mode"));
		ret = -EPERM;
		goto exit;
	}

	if ((HDD_ANTENNA_MODE_1X1 == mode) &&
	    hdd_is_supported_chain_mask_1x1(hdd_ctx)) {
		hddLog(LOGE, FL("System already in 1x1 mode"));
		ret = 0;
		goto exit;
	}

	/* Non connected state */
	if (0 == wlan_hdd_get_active_session_count(hdd_ctx)) {
		hddLog(LOG1,
		       FL("Switch to %d x %d in non connected state"),
		       mode, mode);

		ret = switch_antenna_mode_non_conn_state(
				hdd_ctx, adapter, mode);
		if (0 != ret) {
			hddLog(LOGE,
			       FL("Failed to switch to %d x %d mode"),
			       mode, mode);
			goto exit;
		}

		hdd_ctx->current_antenna_mode = (mode == 1) ?
			HDD_ANTENNA_MODE_1X1 : HDD_ANTENNA_MODE_2X2;

	} else if ((hdd_ctx->concurrency_mode <= 1) &&
		   (hdd_ctx->no_of_active_sessions[
		    WLAN_HDD_INFRA_STATION] <= 1)) {
		hddLog(LOG1,
		       FL("Switch to %d x %d in connected sta state"),
		       mode, mode);

		if (HDD_ANTENNA_MODE_1X1 == mode) {
			ret = switch_to_1x1_connected_sta_state(
				hdd_ctx, adapter);
			if (0 != ret) {
				hddLog(LOGE,
				       FL("Failed to switch to 1x1 mode"));
				goto exit;
			}
			hdd_ctx->current_antenna_mode =
				HDD_ANTENNA_MODE_1X1;

		} else if (HDD_ANTENNA_MODE_2X2 == mode) {
			ret = switch_to_2x2_connected_sta_state(
				hdd_ctx, adapter);
			if (0 != ret) {
				hddLog(LOGE,
				       FL("Failed to switch to 2x2 mode"));
				goto exit;
			}
			hdd_ctx->current_antenna_mode =
				HDD_ANTENNA_MODE_2X2;
		}
	}

	/* Update the user requested nss in the mac context.
	 * This will be used in tdls protocol engine to form tdls
	 * Management frames.
	 */
	sme_update_user_configured_nss(
		hdd_ctx->hHal,
		hdd_ctx->current_antenna_mode);

exit:
#ifdef FEATURE_WLAN_TDLS
	/* Reset tdls NSS flags */
	if (hdd_ctx->tdls_nss_switch_in_progress &&
	    hdd_ctx->tdls_nss_teardown_complete) {
		hdd_ctx->tdls_nss_switch_in_progress = false;
		hdd_ctx->tdls_nss_teardown_complete = false;
	}

	hddLog(LOG1,
	       FL("tdls_nss_switch_in_progress: %d tdls_nss_teardown_complete: %d"),
	       hdd_ctx->tdls_nss_switch_in_progress,
	       hdd_ctx->tdls_nss_teardown_complete);
#endif
	hddLog(LOG1, FL("Set antenna status: %d current mode: %d"),
	       ret, hdd_ctx->current_antenna_mode);
	return ret;
}

/**
 * drv_cmd_get_antenna_mode() - GET ANTENNA MODE driver command
 * handler
 * @adapter: Pointer to hdd adapter
 * @hdd_ctx: Pointer to hdd context
 * @command: Pointer to input command
 * @command_len: length of the command
 * @priv_data: private data coming with the driver command
 *
 * Return: 0 for success non-zero for failure
 */
static inline int drv_cmd_get_antenna_mode(hdd_adapter_t *adapter,
					   hdd_context_t *hdd_ctx,
					   uint8_t *command,
					   uint8_t command_len,
					   hdd_priv_data_t *priv_data)
{
	uint32_t antenna_mode = 0;
	char extra[32];
	uint8_t len = 0;

	antenna_mode = hdd_ctx->current_antenna_mode;
	len = scnprintf(extra, sizeof(extra), "%s %d", command,
			antenna_mode);
	len = VOS_MIN(priv_data->total_len, len + 1);
	if (copy_to_user(priv_data->buf, &extra, len)) {
		hddLog(LOGE, FL("Failed to copy data to user buffer"));
		return -EFAULT;
	}

	return 0;
}

static int hdd_driver_command(hdd_adapter_t *pAdapter,
                              hdd_priv_data_t *ppriv_data)
{
   hdd_priv_data_t priv_data;
   tANI_U8 *command = NULL;
   int ret = 0;

   ENTER();

   if (VOS_FTM_MODE == hdd_get_conparam()) {
        hddLog(LOGE, FL("Command not allowed in FTM mode"));
        return -EINVAL;
   }

   /*
    * Note that valid pointers are provided by caller
    */

   /* copy to local struct to avoid numerous changes to legacy code */
   priv_data = *ppriv_data;

   if (priv_data.total_len <= 0  ||
       priv_data.total_len > WLAN_PRIV_DATA_MAX_LEN)
   {
       hddLog(VOS_TRACE_LEVEL_WARN,
              "%s:invalid priv_data.total_len(%d)!!!", __func__,
              priv_data.total_len);
       ret = -EINVAL;
       goto exit;
   }

   /* Allocate +1 for '\0' */
   command = vos_mem_malloc(priv_data.total_len + 1);
   if (!command)
   {
       hddLog(VOS_TRACE_LEVEL_ERROR,
              "%s: failed to allocate memory", __func__);
       ret = -ENOMEM;
       goto exit;
   }

   if (copy_from_user(command, priv_data.buf, priv_data.total_len))
   {
       ret = -EFAULT;
       goto exit;
   }

   /* Make sure the command is NUL-terminated */
   command[priv_data.total_len] = '\0';

   /* at one time the following block of code was conditional. braces
    * have been retained to avoid re-indenting the legacy code
    */
   {
       hdd_context_t *pHddCtx = (hdd_context_t*)pAdapter->pHddCtx;

       hddLog(VOS_TRACE_LEVEL_INFO,
              "%s: Received %s cmd from Wi-Fi GUI***", __func__, command);

       if (strncmp(command, "P2P_DEV_ADDR", 12) == 0 )
       {
           MTRACE(vos_trace(VOS_MODULE_ID_HDD,
                            TRACE_CODE_HDD_P2P_DEV_ADDR_IOCTL,
                            pAdapter->sessionId, (unsigned)
                            (*(pHddCtx->p2pDeviceAddress.bytes+2)<<24 |
                             *(pHddCtx->p2pDeviceAddress.bytes+3)<<16 |
                             *(pHddCtx->p2pDeviceAddress.bytes+4)<<8  |
                             *(pHddCtx->p2pDeviceAddress.bytes+5))));
           if (copy_to_user(priv_data.buf, pHddCtx->p2pDeviceAddress.bytes,
                                                           sizeof(tSirMacAddr)))
           {
               hddLog(VOS_TRACE_LEVEL_ERROR,
                     "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
           }
       }
       else if (strncmp(command, "SETBAND", 7) == 0)
       {
           tANI_U8 *ptr = command ;

           if (hdd_drv_cmd_validate(command, 7))
               goto exit;

           /* Change band request received */

           /* First 8 bytes will have "SETBAND " and
            * 9 byte will have band setting value */
           hddLog(VOS_TRACE_LEVEL_INFO,
                  "%s: SetBandCommand Info  comm %s UL %d, TL %d",
                  __func__, command, priv_data.used_len, priv_data.total_len);
           /* Change band request received */
           ret = hdd_setBand_helper(pAdapter->dev, ptr);
       }
       else if (strncmp(command, "SETWMMPS", 8) == 0)
       {
           tANI_U8 *ptr = command;

           if (hdd_drv_cmd_validate(command, 8))
               goto exit;

           ret = hdd_wmmps_helper(pAdapter, ptr);
       }
       else if (strncasecmp(command, "COUNTRY", 7) == 0)
       {
           eHalStatus status;
           unsigned long rc;
           char *country_code;

           if (hdd_drv_cmd_validate(command, 7))
               goto exit;

           country_code = command + 8;

           INIT_COMPLETION(pAdapter->change_country_code);
           hdd_checkandupdate_dfssetting(pAdapter, country_code);

           status =
              sme_ChangeCountryCode(pHddCtx->hHal,
                                    wlan_hdd_change_country_code_callback,
                                    country_code, pAdapter,
                                    pHddCtx->pvosContext,
                                    eSIR_TRUE, eSIR_TRUE);
           if (status == eHAL_STATUS_SUCCESS)
           {
               rc = wait_for_completion_timeout(
                       &pAdapter->change_country_code,
                       msecs_to_jiffies(WLAN_WAIT_TIME_COUNTRY));
               if (!rc) {
                   hddLog(VOS_TRACE_LEVEL_ERROR,
                          "%s: SME while setting country code timed out",
                          __func__);
               }
           }
           else
           {
               hddLog(VOS_TRACE_LEVEL_ERROR,
                      "%s: SME Change Country code fail, status=%d",
                      __func__, status);
               ret = -EINVAL;
           }

       }
       else if (strncmp(command, "SETSUSPENDMODE", 14) == 0)
       {
       }
#ifdef WLAN_FEATURE_NEIGHBOR_ROAMING
       else if (strncmp(command, "SETROAMTRIGGER", 14) == 0)
       {
           tANI_U8 *value = command;
           tANI_S8 rssi = 0;
           tANI_U8 lookUpThreshold = CFG_NEIGHBOR_LOOKUP_RSSI_THRESHOLD_DEFAULT;
           eHalStatus status = eHAL_STATUS_SUCCESS;

           if (hdd_drv_cmd_validate(command, 14))
               goto exit;

           /* Move pointer to ahead of SETROAMTRIGGER<delimiter> */
           value = value + 15;

           /* Convert the value from ascii to integer */
           ret = kstrtos8(value, 10, &rssi);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype, then also
                  kstrtou8 fails */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed Input value may be out of range[%d - %d]",
                      __func__,
                      CFG_NEIGHBOR_LOOKUP_RSSI_THRESHOLD_MIN,
                      CFG_NEIGHBOR_LOOKUP_RSSI_THRESHOLD_MAX);
               ret = -EINVAL;
               goto exit;
           }

           lookUpThreshold = abs(rssi);

           if ((lookUpThreshold < CFG_NEIGHBOR_LOOKUP_RSSI_THRESHOLD_MIN) ||
               (lookUpThreshold > CFG_NEIGHBOR_LOOKUP_RSSI_THRESHOLD_MAX))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "Neighbor lookup threshold value %d is out of range"
                      " (Min: %d Max: %d)", lookUpThreshold,
                      CFG_NEIGHBOR_LOOKUP_RSSI_THRESHOLD_MIN,
                      CFG_NEIGHBOR_LOOKUP_RSSI_THRESHOLD_MAX);
               ret = -EINVAL;
               goto exit;
           }

           MTRACE(vos_trace(VOS_MODULE_ID_HDD,
                            TRACE_CODE_HDD_SETROAMTRIGGER_IOCTL,
                            pAdapter->sessionId, lookUpThreshold));
           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to Set Roam trigger"
                      " (Neighbor lookup threshold) = %d", __func__, lookUpThreshold);

           pHddCtx->cfg_ini->nNeighborLookupRssiThreshold = lookUpThreshold;
           status = sme_setNeighborLookupRssiThreshold(pHddCtx->hHal,
                                                       pAdapter->sessionId,
                                                       lookUpThreshold);
           if (eHAL_STATUS_SUCCESS != status)
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: Failed to set roam trigger, try again", __func__);
               ret = -EPERM;
               goto exit;
           }

           /* Set Reassoc threshold to (lookup rssi threshold + 5 dBm) */
           pHddCtx->cfg_ini->nNeighborReassocRssiThreshold =
                                                      lookUpThreshold + 5;
           sme_setNeighborReassocRssiThreshold(pHddCtx->hHal,
                                               pAdapter->sessionId,
                                               lookUpThreshold + 5);
       }
       else if (strncmp(command, "GETROAMTRIGGER", 14) == 0)
       {
           tANI_U8 lookUpThreshold =
                              sme_getNeighborLookupRssiThreshold(pHddCtx->hHal);
           int rssi = (-1) * lookUpThreshold;
           char extra[32];
           tANI_U8 len = 0;
           MTRACE(vos_trace(VOS_MODULE_ID_HDD,
                            TRACE_CODE_HDD_GETROAMTRIGGER_IOCTL,
                            pAdapter->sessionId, lookUpThreshold));
           len = scnprintf(extra, sizeof(extra), "%s %d", command, rssi);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
       else if (strncmp(command, "SETROAMSCANPERIOD", 17) == 0)
       {
           tANI_U8 *value = command;
           tANI_U8 roamScanPeriod = 0;
           tANI_U16 neighborEmptyScanRefreshPeriod = CFG_EMPTY_SCAN_REFRESH_PERIOD_DEFAULT;

           if (hdd_drv_cmd_validate(command, 17))
               goto exit;

           /* input refresh period is in terms of seconds */
           /* Move pointer to ahead of SETROAMSCANPERIOD<delimiter> */
           value = value + 18;
           /* Convert the value from ascii to integer */
           ret = kstrtou8(value, 10, &roamScanPeriod);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype, then also
                  kstrtou8 fails */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed Input value may be out of range[%d - %d]",
                      __func__,
                      (CFG_EMPTY_SCAN_REFRESH_PERIOD_MIN/1000),
                      (CFG_EMPTY_SCAN_REFRESH_PERIOD_MAX/1000));
               ret = -EINVAL;
               goto exit;
           }

           if ((roamScanPeriod < (CFG_EMPTY_SCAN_REFRESH_PERIOD_MIN/1000)) ||
               (roamScanPeriod > (CFG_EMPTY_SCAN_REFRESH_PERIOD_MAX/1000)))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "Roam scan period value %d is out of range"
                      " (Min: %d Max: %d)", roamScanPeriod,
                      (CFG_EMPTY_SCAN_REFRESH_PERIOD_MIN/1000),
                      (CFG_EMPTY_SCAN_REFRESH_PERIOD_MAX/1000));
               ret = -EINVAL;
               goto exit;
           }
           MTRACE(vos_trace(VOS_MODULE_ID_HDD,
                            TRACE_CODE_HDD_SETROAMSCANPERIOD_IOCTL,
                            pAdapter->sessionId, roamScanPeriod));
           neighborEmptyScanRefreshPeriod = roamScanPeriod * 1000;

           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to Set roam scan period"
                      " (Empty Scan refresh period) = %d", __func__, roamScanPeriod);

           pHddCtx->cfg_ini->nEmptyScanRefreshPeriod = neighborEmptyScanRefreshPeriod;
           sme_UpdateEmptyScanRefreshPeriod(pHddCtx->hHal,
                                            pAdapter->sessionId,
                                            neighborEmptyScanRefreshPeriod);
       }
       else if (strncmp(command, "GETROAMSCANPERIOD", 17) == 0)
       {
           tANI_U16 nEmptyScanRefreshPeriod =
                                   sme_getEmptyScanRefreshPeriod(pHddCtx->hHal);
           char extra[32];
           tANI_U8 len = 0;

           MTRACE(vos_trace(VOS_MODULE_ID_HDD,
                            TRACE_CODE_HDD_GETROAMSCANPERIOD_IOCTL,
                            pAdapter->sessionId, nEmptyScanRefreshPeriod));
           len = scnprintf(extra, sizeof(extra), "%s %d",
                   "GETROAMSCANPERIOD", (nEmptyScanRefreshPeriod/1000));
           /* Returned value is in units of seconds */
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
       else if (strncmp(command, "SETROAMSCANREFRESHPERIOD", 24) == 0)
       {
           tANI_U8 *value = command;
           tANI_U8 roamScanRefreshPeriod = 0;
           tANI_U16 neighborScanRefreshPeriod = CFG_NEIGHBOR_SCAN_RESULTS_REFRESH_PERIOD_DEFAULT;

           if (hdd_drv_cmd_validate(command, 24))
               goto exit;

           /* input refresh period is in terms of seconds */
           /* Move pointer to ahead of SETROAMSCANREFRESHPERIOD<delimiter> */
           value = value + 25;

           /* Convert the value from ascii to integer */
           ret = kstrtou8(value, 10, &roamScanRefreshPeriod);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype, then also
                  kstrtou8 fails */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed Input value may be out of range[%d - %d]",
                      __func__,
                      (CFG_NEIGHBOR_SCAN_RESULTS_REFRESH_PERIOD_MIN/1000),
                      (CFG_NEIGHBOR_SCAN_RESULTS_REFRESH_PERIOD_MAX/1000));
               ret = -EINVAL;
               goto exit;
           }

           if ((roamScanRefreshPeriod < (CFG_NEIGHBOR_SCAN_RESULTS_REFRESH_PERIOD_MIN/1000)) ||
               (roamScanRefreshPeriod > (CFG_NEIGHBOR_SCAN_RESULTS_REFRESH_PERIOD_MAX/1000)))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "Neighbor scan results refresh period value %d is out of range"
                      " (Min: %d Max: %d)", roamScanRefreshPeriod,
                      (CFG_NEIGHBOR_SCAN_RESULTS_REFRESH_PERIOD_MIN/1000),
                      (CFG_NEIGHBOR_SCAN_RESULTS_REFRESH_PERIOD_MAX/1000));
               ret = -EINVAL;
               goto exit;
           }
           neighborScanRefreshPeriod = roamScanRefreshPeriod * 1000;

           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to Set roam scan refresh period"
                      " (Scan refresh period) = %d", __func__, roamScanRefreshPeriod);

           pHddCtx->cfg_ini->nNeighborResultsRefreshPeriod = neighborScanRefreshPeriod;
           sme_setNeighborScanRefreshPeriod(pHddCtx->hHal,
                                            pAdapter->sessionId,
                                            neighborScanRefreshPeriod);
       }
       else if (strncmp(command, "GETROAMSCANREFRESHPERIOD", 24) == 0)
       {
           tANI_U16 value = sme_getNeighborScanRefreshPeriod(pHddCtx->hHal);
           char extra[32];
           tANI_U8 len = 0;

           len = scnprintf(extra, sizeof(extra), "%s %d",
                   "GETROAMSCANREFRESHPERIOD", (value/1000));
           /* Returned value is in units of seconds */
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
#ifdef FEATURE_WLAN_LFR
       /* SETROAMMODE */
       else if (strncmp(command, "SETROAMMODE", SIZE_OF_SETROAMMODE) == 0)
       {
           tANI_U8 *value = command;
	   tANI_BOOLEAN roamMode = CFG_LFR_FEATURE_ENABLED_DEFAULT;

           if (hdd_drv_cmd_validate(command, SIZE_OF_SETROAMMODE))
               goto exit;

	   /* Move pointer to ahead of SETROAMMODE<delimiter> */
	   value = value + SIZE_OF_SETROAMMODE + 1;

	   /* Convert the value from ascii to integer */
	   ret = kstrtou8(value, 10, &roamMode);
	   if (ret < 0)
	   {
	      /* If the input value is greater than max value of datatype, then also
		  kstrtou8 fails */
	      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
		   "%s: kstrtou8 failed range [%d - %d]", __func__,
		   CFG_LFR_FEATURE_ENABLED_MIN,
		   CFG_LFR_FEATURE_ENABLED_MAX);
              ret = -EINVAL;
	      goto exit;
	   }
           if ((roamMode < CFG_LFR_FEATURE_ENABLED_MIN) ||
	       (roamMode > CFG_LFR_FEATURE_ENABLED_MAX))
           {
              VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
			"Roam Mode value %d is out of range"
			" (Min: %d Max: %d)", roamMode,
			CFG_LFR_FEATURE_ENABLED_MIN,
			CFG_LFR_FEATURE_ENABLED_MAX);
	      ret = -EINVAL;
	      goto exit;
	   }

	   VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_DEBUG,
		   "%s: Received Command to Set Roam Mode = %d", __func__, roamMode);
           /*
	    * Note that
	    *     SETROAMMODE 0 is to enable LFR while
	    *     SETROAMMODE 1 is to disable LFR, but
	    *     NotifyIsFastRoamIniFeatureEnabled 0/1 is to enable/disable.
	    *     So, we have to invert the value to call sme_UpdateIsFastRoamIniFeatureEnabled.
	    */
	   if (CFG_LFR_FEATURE_ENABLED_MIN == roamMode)
	       roamMode = CFG_LFR_FEATURE_ENABLED_MAX;    /* Roam enable */
	   else
	       roamMode = CFG_LFR_FEATURE_ENABLED_MIN;    /* Roam disable */

	   pHddCtx->cfg_ini->isFastRoamIniFeatureEnabled = roamMode;
	   /* LFR2 is dependent on Fast Roam. So, enable/disable LFR2
	    * variable. if Fast Roam has been changed from disabled to enabled,
	    * then enable LFR2 and send the LFR START command to the firmware.
	    * Otherwise, send the LFR STOP command to the firmware and then
	    * disable LFR2.The sequence is different.
	    */
	   if (roamMode) {
		   pHddCtx->cfg_ini->isRoamOffloadScanEnabled = roamMode;
		   sme_UpdateRoamScanOffloadEnabled((tHalHandle)(pHddCtx->hHal),
				   pHddCtx->cfg_ini->isRoamOffloadScanEnabled);
		   sme_UpdateIsFastRoamIniFeatureEnabled(pHddCtx->hHal,
				   pAdapter->sessionId,
				   roamMode);
	   } else {
		   sme_UpdateIsFastRoamIniFeatureEnabled(pHddCtx->hHal,
				   pAdapter->sessionId,
				   roamMode);
		   pHddCtx->cfg_ini->isRoamOffloadScanEnabled = roamMode;
		   sme_UpdateRoamScanOffloadEnabled((tHalHandle)(pHddCtx->hHal),
				   pHddCtx->cfg_ini->isRoamOffloadScanEnabled);
	   }
       }
       /* GETROAMMODE */
       else if (strncmp(command, "GETROAMMODE", SIZE_OF_GETROAMMODE) == 0)
       {
	   tANI_BOOLEAN roamMode = sme_getIsLfrFeatureEnabled(pHddCtx->hHal);
	   char extra[32];
	   tANI_U8 len = 0;

           /*
            * roamMode value shall be inverted because the semantics is
            * different.
            */
           if (CFG_LFR_FEATURE_ENABLED_MIN == roamMode)
	       roamMode = CFG_LFR_FEATURE_ENABLED_MAX;
           else
	       roamMode = CFG_LFR_FEATURE_ENABLED_MIN;

	   len = scnprintf(extra, sizeof(extra), "%s %d", command, roamMode);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
	   }
       }
#endif
#endif
#if  defined (WLAN_FEATURE_VOWIFI_11R) || defined (FEATURE_WLAN_ESE) || defined(FEATURE_WLAN_LFR)
       else if (strncmp(command, "SETROAMDELTA", 12) == 0)
       {
           tANI_U8 *value = command;
           tANI_U8 roamRssiDiff = CFG_ROAM_RSSI_DIFF_DEFAULT;

           if (hdd_drv_cmd_validate(command, 12))
               goto exit;

           /* Move pointer to ahead of SETROAMDELTA<delimiter> */
           value = value + 13;
           /* Convert the value from ascii to integer */
           ret = kstrtou8(value, 10, &roamRssiDiff);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype, then also
                  kstrtou8 fails */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed range [%d - %d]", __func__,
                      CFG_ROAM_RSSI_DIFF_MIN,
                      CFG_ROAM_RSSI_DIFF_MAX);
               ret = -EINVAL;
               goto exit;
           }

           if ((roamRssiDiff < CFG_ROAM_RSSI_DIFF_MIN) ||
               (roamRssiDiff > CFG_ROAM_RSSI_DIFF_MAX))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "Roam rssi diff value %d is out of range"
                      " (Min: %d Max: %d)", roamRssiDiff,
                      CFG_ROAM_RSSI_DIFF_MIN,
                      CFG_ROAM_RSSI_DIFF_MAX);
               ret = -EINVAL;
               goto exit;
           }

           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to Set roam rssi diff = %d", __func__, roamRssiDiff);

           pHddCtx->cfg_ini->RoamRssiDiff = roamRssiDiff;
           sme_UpdateRoamRssiDiff(pHddCtx->hHal,
                                  pAdapter->sessionId, roamRssiDiff);
       }
       else if (strncmp(command, "GETROAMDELTA", 12) == 0)
       {
           tANI_U8 roamRssiDiff = sme_getRoamRssiDiff(pHddCtx->hHal);
           char extra[32];
           tANI_U8 len = 0;

           MTRACE(vos_trace(VOS_MODULE_ID_HDD,
                            TRACE_CODE_HDD_GETROAMDELTA_IOCTL,
                            pAdapter->sessionId, roamRssiDiff));
           len = scnprintf(extra, sizeof(extra), "%s %d",
                   command, roamRssiDiff);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
#endif
#if  defined (WLAN_FEATURE_VOWIFI_11R) || defined (FEATURE_WLAN_ESE) || defined(FEATURE_WLAN_LFR)
       else if (strncmp(command, "GETBAND", 7) == 0)
       {
           int band = -1;
           char extra[32];
           tANI_U8 len = 0;
           hdd_getBand_helper(pHddCtx, &band);

           MTRACE(vos_trace(VOS_MODULE_ID_HDD,
                            TRACE_CODE_HDD_GETBAND_IOCTL,
                            pAdapter->sessionId, band));
           len = scnprintf(extra, sizeof(extra), "%s %d", command, band);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
       else if (strncmp(command, "SETROAMSCANCHANNELS ", 20) == 0)
       {
           if (hdd_drv_cmd_validate(command, 20))
               goto exit;

           ret = hdd_parse_set_roam_scan_channels(pAdapter, command);
       }
       else if (strncmp(command, "GETROAMSCANCHANNELS", 19) == 0)
       {
           tANI_U8 ChannelList[WNI_CFG_VALID_CHANNEL_LIST_LEN] = {0};
           tANI_U8 numChannels = 0;
           tANI_U8 j = 0;
           char extra[128] = {0};
           int len;

           if (eHAL_STATUS_SUCCESS != sme_getRoamScanChannelList(
                                                           pHddCtx->hHal,
                                                           ChannelList,
                                                           &numChannels,
                                                           pAdapter->sessionId))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_FATAL,
                  "%s: failed to get roam scan channel list", __func__);
               ret = -EFAULT;
               goto exit;
           }
           MTRACE(vos_trace(VOS_MODULE_ID_HDD,
                            TRACE_CODE_HDD_GETROAMSCANCHANNELS_IOCTL,
                            pAdapter->sessionId, numChannels));
           /* output channel list is of the format
           [Number of roam scan channels][Channel1][Channel2]... */
           /* copy the number of channels in the 0th index */
           len = scnprintf(extra, sizeof(extra), "%s %d", command, numChannels);
           for (j = 0; (j < numChannels) && len <= sizeof(extra); j++)
           {
               len += scnprintf(extra + len, sizeof(extra) - len, " %d",
                       ChannelList[j]);
           }

           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
       else if (strncmp(command, "GETCCXMODE", 10) == 0)
       {
           tANI_BOOLEAN eseMode = sme_getIsEseFeatureEnabled(pHddCtx->hHal);
           char extra[32];
           tANI_U8 len = 0;

           /* Check if the features OKC/ESE/11R are supported simultaneously,
              then this operation is not permitted (return FAILURE) */
           if (eseMode &&
               hdd_is_okc_mode_enabled(pHddCtx) &&
               sme_getIsFtFeatureEnabled(pHddCtx->hHal))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_WARN,
                  "%s: OKC/ESE/11R are supported simultaneously"
                  " hence this operation is not permitted!", __func__);
               ret = -EPERM;
               goto exit;
           }

           len = scnprintf(extra, sizeof(extra), "%s %d",
                   "GETCCXMODE", eseMode);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
       else if (strncmp(command, "GETOKCMODE", 10) == 0)
       {
           tANI_BOOLEAN okcMode = hdd_is_okc_mode_enabled(pHddCtx);
           char extra[32];
           tANI_U8 len = 0;

           /* Check if the features OKC/ESE/11R are supported simultaneously,
              then this operation is not permitted (return FAILURE) */
           if (okcMode &&
               sme_getIsEseFeatureEnabled(pHddCtx->hHal) &&
               sme_getIsFtFeatureEnabled(pHddCtx->hHal))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_WARN,
                  "%s: OKC/ESE/11R are supported simultaneously"
                  " hence this operation is not permitted!", __func__);
               ret = -EPERM;
               goto exit;
           }

           len = scnprintf(extra, sizeof(extra), "%s %d",
                   "GETOKCMODE", okcMode);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
       else if (strncmp(command, "GETFASTROAM", 11) == 0)
       {
           tANI_BOOLEAN lfrMode = sme_getIsLfrFeatureEnabled(pHddCtx->hHal);
           char extra[32];
           tANI_U8 len = 0;

           len = scnprintf(extra, sizeof(extra), "%s %d",
                   "GETFASTROAM", lfrMode);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
       else if (strncmp(command, "GETFASTTRANSITION", 17) == 0)
       {
           tANI_BOOLEAN ft = sme_getIsFtFeatureEnabled(pHddCtx->hHal);
           char extra[32];
           tANI_U8 len = 0;

           len = scnprintf(extra, sizeof(extra), "%s %d",
                   "GETFASTTRANSITION", ft);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
       else if (strncmp(command, "SETROAMSCANCHANNELMINTIME", 25) == 0)
       {
           tANI_U8 *value = command;
           tANI_U8 minTime = CFG_NEIGHBOR_SCAN_MIN_CHAN_TIME_DEFAULT;

           if (hdd_drv_cmd_validate(command, 25))
               goto exit;

           /* Move pointer to ahead of SETROAMSCANCHANNELMINTIME<delimiter> */
           value = value + 26;
           /* Convert the value from ascii to integer */
           ret = kstrtou8(value, 10, &minTime);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype, then also
                  kstrtou8 fails */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed range [%d - %d]", __func__,
                      CFG_NEIGHBOR_SCAN_MIN_CHAN_TIME_MIN,
                      CFG_NEIGHBOR_SCAN_MIN_CHAN_TIME_MAX);
               ret = -EINVAL;
               goto exit;
           }
           if ((minTime < CFG_NEIGHBOR_SCAN_MIN_CHAN_TIME_MIN) ||
               (minTime > CFG_NEIGHBOR_SCAN_MIN_CHAN_TIME_MAX))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "scan min channel time value %d is out of range"
                      " (Min: %d Max: %d)", minTime,
                      CFG_NEIGHBOR_SCAN_MIN_CHAN_TIME_MIN,
                      CFG_NEIGHBOR_SCAN_MIN_CHAN_TIME_MAX);
               ret = -EINVAL;
               goto exit;
           }

           MTRACE(vos_trace(VOS_MODULE_ID_HDD,
                            TRACE_CODE_HDD_SETROAMSCANCHANNELMINTIME_IOCTL,
                            pAdapter->sessionId, minTime));
           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to change channel min time = %d", __func__, minTime);

           pHddCtx->cfg_ini->nNeighborScanMinChanTime = minTime;
           sme_setNeighborScanMinChanTime(pHddCtx->hHal,
                                          minTime, pAdapter->sessionId);
       }
       else if (strncmp(command, "SENDACTIONFRAME", 15) == 0)
       {
           if (hdd_drv_cmd_validate(command, 15))
               goto exit;

           ret = hdd_parse_sendactionframe(pAdapter, command,
                                           priv_data.total_len);
       }
       else if (strncmp(command, "GETROAMSCANCHANNELMINTIME", 25) == 0)
       {
           tANI_U16 val = sme_getNeighborScanMinChanTime(
                                         pHddCtx->hHal,
                                         pAdapter->sessionId);
           char extra[32];
           tANI_U8 len = 0;

           /* value is interms of msec */
           len = scnprintf(extra, sizeof(extra), "%s %d",
                   "GETROAMSCANCHANNELMINTIME", val);
           MTRACE(vos_trace(VOS_MODULE_ID_HDD,
                            TRACE_CODE_HDD_GETROAMSCANCHANNELMINTIME_IOCTL,
                            pAdapter->sessionId, val));
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
       else if (strncmp(command, "SETSCANCHANNELTIME", 18) == 0)
       {
           tANI_U8 *value = command;
           tANI_U16 maxTime = CFG_NEIGHBOR_SCAN_MAX_CHAN_TIME_DEFAULT;

           if (hdd_drv_cmd_validate(command, 18))
               goto exit;

           /* Move pointer to ahead of SETSCANCHANNELTIME<delimiter> */
           value = value + 19;
           /* Convert the value from ascii to integer */
           ret = kstrtou16(value, 10, &maxTime);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype, then also
                  kstrtou16 fails */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou16 failed range [%d - %d]", __func__,
                      CFG_NEIGHBOR_SCAN_MAX_CHAN_TIME_MIN,
                      CFG_NEIGHBOR_SCAN_MAX_CHAN_TIME_MAX);
               ret = -EINVAL;
               goto exit;
           }

           if ((maxTime < CFG_NEIGHBOR_SCAN_MAX_CHAN_TIME_MIN) ||
               (maxTime > CFG_NEIGHBOR_SCAN_MAX_CHAN_TIME_MAX))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "lfr mode value %d is out of range"
                      " (Min: %d Max: %d)", maxTime,
                      CFG_NEIGHBOR_SCAN_MAX_CHAN_TIME_MIN,
                      CFG_NEIGHBOR_SCAN_MAX_CHAN_TIME_MAX);
               ret = -EINVAL;
               goto exit;
           }

           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to change channel max time = %d", __func__, maxTime);

           pHddCtx->cfg_ini->nNeighborScanMaxChanTime = maxTime;
           sme_setNeighborScanMaxChanTime(pHddCtx->hHal,
                                          pAdapter->sessionId, maxTime);
       }
       else if (strncmp(command, "GETSCANCHANNELTIME", 18) == 0)
       {
           tANI_U16 val = sme_getNeighborScanMaxChanTime(pHddCtx->hHal,
                                                         pAdapter->sessionId);
           char extra[32];
           tANI_U8 len = 0;

           /* value is interms of msec */
           len = scnprintf(extra, sizeof(extra), "%s %d",
                   "GETSCANCHANNELTIME", val);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
       else if (strncmp(command, "SETSCANHOMETIME", 15) == 0)
       {
           tANI_U8 *value = command;
           tANI_U16 val = CFG_NEIGHBOR_SCAN_TIMER_PERIOD_DEFAULT;

           if (hdd_drv_cmd_validate(command, 15))
               goto exit;

           /* Move pointer to ahead of SETSCANHOMETIME<delimiter> */
           value = value + 16;
           /* Convert the value from ascii to integer */
           ret = kstrtou16(value, 10, &val);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype, then also
                  kstrtou16 fails */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou16 failed range [%d - %d]", __func__,
                      CFG_NEIGHBOR_SCAN_TIMER_PERIOD_MIN,
                      CFG_NEIGHBOR_SCAN_TIMER_PERIOD_MAX);
               ret = -EINVAL;
               goto exit;
           }

           if ((val < CFG_NEIGHBOR_SCAN_TIMER_PERIOD_MIN) ||
               (val > CFG_NEIGHBOR_SCAN_TIMER_PERIOD_MAX))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "scan home time value %d is out of range"
                      " (Min: %d Max: %d)", val,
                      CFG_NEIGHBOR_SCAN_TIMER_PERIOD_MIN,
                      CFG_NEIGHBOR_SCAN_TIMER_PERIOD_MAX);
               ret = -EINVAL;
               goto exit;
           }

           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to change scan home time = %d", __func__, val);

           pHddCtx->cfg_ini->nNeighborScanPeriod = val;
           sme_setNeighborScanPeriod(pHddCtx->hHal,
                                     pAdapter->sessionId, val);
       }
       else if (strncmp(command, "GETSCANHOMETIME", 15) == 0)
       {
           tANI_U16 val = sme_getNeighborScanPeriod(pHddCtx->hHal,
                                                    pAdapter->sessionId);
           char extra[32];
           tANI_U8 len = 0;

           /* value is interms of msec */
           len = scnprintf(extra, sizeof(extra), "%s %d",
                   "GETSCANHOMETIME", val);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
       else if (strncmp(command, "SETROAMINTRABAND", 16) == 0)
       {
           tANI_U8 *value = command;
           tANI_U8 val = CFG_ROAM_INTRA_BAND_DEFAULT;

           if (hdd_drv_cmd_validate(command, 16))
               goto exit;

           /* Move pointer to ahead of SETROAMINTRABAND<delimiter> */
           value = value + 17;
           /* Convert the value from ascii to integer */
           ret = kstrtou8(value, 10, &val);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype, then also
                  kstrtou8 fails */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed range [%d - %d]", __func__,
                      CFG_ROAM_INTRA_BAND_MIN,
                      CFG_ROAM_INTRA_BAND_MAX);
               ret = -EINVAL;
               goto exit;
           }

           if ((val < CFG_ROAM_INTRA_BAND_MIN) ||
               (val > CFG_ROAM_INTRA_BAND_MAX))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "intra band mode value %d is out of range"
                      " (Min: %d Max: %d)", val,
                      CFG_ROAM_INTRA_BAND_MIN,
                      CFG_ROAM_INTRA_BAND_MAX);
               ret = -EINVAL;
               goto exit;
           }
           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to change intra band = %d", __func__, val);

           pHddCtx->cfg_ini->nRoamIntraBand = val;
           sme_setRoamIntraBand(pHddCtx->hHal, val);
       }
       else if (strncmp(command, "GETROAMINTRABAND", 16) == 0)
       {
           tANI_U16 val = sme_getRoamIntraBand(pHddCtx->hHal);
           char extra[32];
           tANI_U8 len = 0;

           /* value is interms of msec */
           len = scnprintf(extra, sizeof(extra), "%s %d",
                   "GETROAMINTRABAND", val);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
       else if (strncmp(command, "SETSCANNPROBES", 14) == 0)
       {
           tANI_U8 *value = command;
           tANI_U8 nProbes = CFG_ROAM_SCAN_N_PROBES_DEFAULT;

           if (hdd_drv_cmd_validate(command, 14))
               goto exit;

           /* Move pointer to ahead of SETSCANNPROBES<delimiter> */
           value = value + 15;
           /* Convert the value from ascii to integer */
           ret = kstrtou8(value, 10, &nProbes);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype, then also
                  kstrtou8 fails */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed range [%d - %d]", __func__,
                      CFG_ROAM_SCAN_N_PROBES_MIN,
                      CFG_ROAM_SCAN_N_PROBES_MAX);
               ret = -EINVAL;
               goto exit;
           }

           if ((nProbes < CFG_ROAM_SCAN_N_PROBES_MIN) ||
               (nProbes > CFG_ROAM_SCAN_N_PROBES_MAX))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "NProbes value %d is out of range"
                      " (Min: %d Max: %d)", nProbes,
                      CFG_ROAM_SCAN_N_PROBES_MIN,
                      CFG_ROAM_SCAN_N_PROBES_MAX);
               ret = -EINVAL;
               goto exit;
           }

           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to Set nProbes = %d", __func__, nProbes);

           pHddCtx->cfg_ini->nProbes = nProbes;
           sme_UpdateRoamScanNProbes(pHddCtx->hHal, pAdapter->sessionId,
                                     nProbes);
       }
       else if (strncmp(command, "GETSCANNPROBES", 14) == 0)
       {
           tANI_U8 val = sme_getRoamScanNProbes(pHddCtx->hHal);
           char extra[32];
           tANI_U8 len = 0;

           len = scnprintf(extra, sizeof(extra), "%s %d", command, val);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
       else if (strncmp(command, "SETSCANHOMEAWAYTIME", 19) == 0)
       {
           tANI_U8 *value = command;
           tANI_U16 homeAwayTime = CFG_ROAM_SCAN_HOME_AWAY_TIME_DEFAULT;

           if (hdd_drv_cmd_validate(command, 19))
               goto exit;

           /* Move pointer to ahead of SETSCANHOMEAWAYTIME<delimiter> */
           /* input value is in units of msec */
           value = value + 20;
           /* Convert the value from ascii to integer */
           ret = kstrtou16(value, 10, &homeAwayTime);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype, then also
                  kstrtou8 fails */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed range [%d - %d]", __func__,
                      CFG_ROAM_SCAN_HOME_AWAY_TIME_MIN,
                      CFG_ROAM_SCAN_HOME_AWAY_TIME_MAX);
               ret = -EINVAL;
               goto exit;
           }

           if ((homeAwayTime < CFG_ROAM_SCAN_HOME_AWAY_TIME_MIN) ||
               (homeAwayTime > CFG_ROAM_SCAN_HOME_AWAY_TIME_MAX))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "homeAwayTime value %d is out of range"
                      " (Min: %d Max: %d)", homeAwayTime,
                      CFG_ROAM_SCAN_HOME_AWAY_TIME_MIN,
                      CFG_ROAM_SCAN_HOME_AWAY_TIME_MAX);
               ret = -EINVAL;
               goto exit;
           }

           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to Set scan away time = %d", __func__, homeAwayTime);
           if (pHddCtx->cfg_ini->nRoamScanHomeAwayTime != homeAwayTime)
           {
               pHddCtx->cfg_ini->nRoamScanHomeAwayTime = homeAwayTime;
               sme_UpdateRoamScanHomeAwayTime(pHddCtx->hHal,
                                              pAdapter->sessionId,
                                              homeAwayTime, eANI_BOOLEAN_TRUE);
           }
       }
       else if (strncmp(command, "GETSCANHOMEAWAYTIME", 19) == 0)
       {
           tANI_U16 val = sme_getRoamScanHomeAwayTime(pHddCtx->hHal);
           char extra[32];
           tANI_U8 len = 0;

           len = scnprintf(extra, sizeof(extra), "%s %d", command, val);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
       else if (strncmp(command, "REASSOC", 7) == 0)
       {
           if (hdd_drv_cmd_validate(command, 7))
               goto exit;

           ret = hdd_parse_reassoc(pAdapter, command, priv_data.total_len);
       }
       else if (strncmp(command, "SETWESMODE", 10) == 0)
       {
           tANI_U8 *value = command;
           tANI_BOOLEAN wesMode = CFG_ENABLE_WES_MODE_NAME_DEFAULT;

           if (hdd_drv_cmd_validate(command, 10))
               goto exit;

           /* Move pointer to ahead of SETWESMODE<delimiter> */
           value = value + 11;
           /* Convert the value from ascii to integer */
           ret = kstrtou8(value, 10, &wesMode);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype, then also
                  kstrtou8 fails */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed range [%d - %d]", __func__,
                      CFG_ENABLE_WES_MODE_NAME_MIN,
                      CFG_ENABLE_WES_MODE_NAME_MAX);
               ret = -EINVAL;
               goto exit;
           }

           if ((wesMode < CFG_ENABLE_WES_MODE_NAME_MIN) ||
               (wesMode > CFG_ENABLE_WES_MODE_NAME_MAX))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "WES Mode value %d is out of range"
                      " (Min: %d Max: %d)", wesMode,
                      CFG_ENABLE_WES_MODE_NAME_MIN,
                      CFG_ENABLE_WES_MODE_NAME_MAX);
               ret = -EINVAL;
               goto exit;
           }
           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to Set WES Mode rssi diff = %d", __func__, wesMode);

           pHddCtx->cfg_ini->isWESModeEnabled = wesMode;
           sme_UpdateWESMode(pHddCtx->hHal, wesMode, pAdapter->sessionId);
       }
       else if (strncmp(command, "GETWESMODE", 10) == 0)
       {
           tANI_BOOLEAN wesMode = sme_GetWESMode(pHddCtx->hHal);
           char extra[32];
           tANI_U8 len = 0;

           len = scnprintf(extra, sizeof(extra), "%s %d", command, wesMode);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
       else if (strncmp(command, "SETOPPORTUNISTICRSSIDIFF", 24) == 0)
       {
           tANI_U8 *value = command;
           tANI_U8 nOpportunisticThresholdDiff = CFG_OPPORTUNISTIC_SCAN_THRESHOLD_DIFF_DEFAULT;

           if (hdd_drv_cmd_validate(command, 24))
               goto exit;

           /* Move pointer to ahead of SETOPPORTUNISTICRSSIDIFF<delimiter> */
           value = value + 25;
           /* Convert the value from ascii to integer */
           ret = kstrtou8(value, 10, &nOpportunisticThresholdDiff);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype, then also
                  kstrtou8 fails */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed.", __func__);
               ret = -EINVAL;
               goto exit;
           }

           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to Set Opportunistic Threshold diff = %d",
                      __func__,
                nOpportunisticThresholdDiff);

           sme_SetRoamOpportunisticScanThresholdDiff(pHddCtx->hHal,
                                                   pAdapter->sessionId,
                                                   nOpportunisticThresholdDiff);
       }
       else if (strncmp(command, "GETOPPORTUNISTICRSSIDIFF", 24) == 0)
       {
           tANI_S8 val = sme_GetRoamOpportunisticScanThresholdDiff(
                                                                 pHddCtx->hHal);
           char extra[32];
           tANI_U8 len = 0;

           len = scnprintf(extra, sizeof(extra), "%s %d", command, val);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
       else if (strncmp(command, "SETROAMRESCANRSSIDIFF", 21) == 0)
       {
           tANI_U8 *value = command;
           tANI_U8 nRoamRescanRssiDiff = CFG_ROAM_RESCAN_RSSI_DIFF_DEFAULT;

           if (hdd_drv_cmd_validate(command, 21))
               goto exit;

           /* Move pointer to ahead of SETROAMRESCANRSSIDIFF<delimiter> */
           value = value + 22;
           /* Convert the value from ascii to integer */
           ret = kstrtou8(value, 10, &nRoamRescanRssiDiff);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype, then also
                  kstrtou8 fails */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed.", __func__);
               ret = -EINVAL;
               goto exit;
           }

           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to Set Roam Rescan RSSI Diff = %d",
                      __func__,
                      nRoamRescanRssiDiff);
           sme_SetRoamRescanRssiDiff(pHddCtx->hHal,
                                     pAdapter->sessionId,
                                     nRoamRescanRssiDiff);
       }
       else if (strncmp(command, "GETROAMRESCANRSSIDIFF", 21) == 0)
       {
           tANI_U8 val = sme_GetRoamRescanRssiDiff(pHddCtx->hHal);
           char extra[32];
           tANI_U8 len = 0;

           len = scnprintf(extra, sizeof(extra), "%s %d", command, val);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
#endif /* WLAN_FEATURE_VOWIFI_11R || FEATURE_WLAN_ESE || FEATURE_WLAN_LFR */
#ifdef FEATURE_WLAN_LFR
       else if (strncmp(command, "SETFASTROAM", 11) == 0)
       {
           tANI_U8 *value = command;
           tANI_U8 lfrMode = CFG_LFR_FEATURE_ENABLED_DEFAULT;

           if (hdd_drv_cmd_validate(command, 11))
               goto exit;

           /* Move pointer to ahead of SETFASTROAM<delimiter> */
           value = value + 12;
           /* Convert the value from ascii to integer */
           ret = kstrtou8(value, 10, &lfrMode);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype, then also
                  kstrtou8 fails */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed range [%d - %d]", __func__,
                      CFG_LFR_FEATURE_ENABLED_MIN,
                      CFG_LFR_FEATURE_ENABLED_MAX);
               ret = -EINVAL;
               goto exit;
           }

           if ((lfrMode < CFG_LFR_FEATURE_ENABLED_MIN) ||
               (lfrMode > CFG_LFR_FEATURE_ENABLED_MAX))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "lfr mode value %d is out of range"
                      " (Min: %d Max: %d)", lfrMode,
                      CFG_LFR_FEATURE_ENABLED_MIN,
                      CFG_LFR_FEATURE_ENABLED_MAX);
               ret = -EINVAL;
               goto exit;
           }

           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to change lfr mode = %d", __func__, lfrMode);

           pHddCtx->cfg_ini->isFastRoamIniFeatureEnabled = lfrMode;
           sme_UpdateIsFastRoamIniFeatureEnabled(pHddCtx->hHal,
                                                 pAdapter->sessionId,
                                                 lfrMode);
       }
#endif
#ifdef WLAN_FEATURE_VOWIFI_11R
       else if (strncmp(command, "SETFASTTRANSITION", 17) == 0)
       {
           tANI_U8 *value = command;
           tANI_U8 ft = CFG_FAST_TRANSITION_ENABLED_NAME_DEFAULT;

           if (hdd_drv_cmd_validate(command, 17))
               goto exit;

           /* Move pointer to ahead of SETFASTROAM<delimiter> */
           value = value + 18;
           /* Convert the value from ascii to integer */
           ret = kstrtou8(value, 10, &ft);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype, then also
                  kstrtou8 fails */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed range [%d - %d]", __func__,
                      CFG_FAST_TRANSITION_ENABLED_NAME_MIN,
                      CFG_FAST_TRANSITION_ENABLED_NAME_MAX);
               ret = -EINVAL;
               goto exit;
           }

           if ((ft < CFG_FAST_TRANSITION_ENABLED_NAME_MIN) ||
               (ft > CFG_FAST_TRANSITION_ENABLED_NAME_MAX))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "ft mode value %d is out of range"
                      " (Min: %d Max: %d)", ft,
                      CFG_FAST_TRANSITION_ENABLED_NAME_MIN,
                      CFG_FAST_TRANSITION_ENABLED_NAME_MAX);
               ret = -EINVAL;
               goto exit;
           }

           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to change ft mode = %d", __func__, ft);

           pHddCtx->cfg_ini->isFastTransitionEnabled = ft;
           sme_UpdateFastTransitionEnabled(pHddCtx->hHal, ft);
       }

       else if (strncmp(command, "FASTREASSOC", 11) == 0)
       {
           tANI_U8 *value = command;
           tANI_U8 channel = 0;
           tSirMacAddr targetApBssid;
           tHalHandle hHal;
           v_U32_t roamId = 0;
           tCsrRoamModifyProfileFields modProfileFields;
#ifdef WLAN_FEATURE_ROAM_SCAN_OFFLOAD
           tCsrHandoffRequest handoffInfo;
#endif
           hdd_station_ctx_t *pHddStaCtx = NULL;
           pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
           hHal = WLAN_HDD_GET_HAL_CTX(pAdapter);

           /* if not associated, no need to proceed with reassoc */
           if (eConnectionState_Associated != pHddStaCtx->conn_info.connState)
           {
               VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO, "%s:Not associated!",__func__);
               ret = -EINVAL;
               goto exit;
           }

           ret = hdd_parse_reassoc_command_v1_data(value, targetApBssid,
                                                   &channel);
           if (ret)
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                          "%s: Failed to parse reassoc command data", __func__);
               goto exit;
           }

           /* if the target bssid is same as currently associated AP,
              issue reassoc to same AP */
           if (VOS_TRUE == vos_mem_compare(targetApBssid,
                                           pHddStaCtx->conn_info.bssId, sizeof(tSirMacAddr)))
           {
               VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO, "%s:Reassoc BSSID is same as currently associated AP bssid",__func__);
               sme_GetModifyProfileFields(hHal, pAdapter->sessionId,
                                       &modProfileFields);
               sme_RoamReassoc(hHal, pAdapter->sessionId,
                            NULL, modProfileFields, &roamId, 1);
               ret = 0;
               goto exit;
           }

           /* Check channel number is a valid channel number */
           if(VOS_STATUS_SUCCESS !=
                         wlan_hdd_validate_operation_channel(pAdapter, channel))
           {
               hddLog(LOGE, FL("Invalid Channel [%d]"), channel);

               ret = -EINVAL;
               goto exit;
           }
#ifdef WLAN_FEATURE_ROAM_OFFLOAD
           if (pHddCtx->cfg_ini->isRoamOffloadEnabled) {
               hdd_wma_send_fastreassoc_cmd((int)pAdapter->sessionId, targetApBssid,
                                       (int) channel);
               goto exit;
           }
#endif
           /* Proceed with reassoc */
#ifdef WLAN_FEATURE_ROAM_SCAN_OFFLOAD
           handoffInfo.channel = channel;
           handoffInfo.src = FASTREASSOC;
           vos_mem_copy(handoffInfo.bssid, targetApBssid, sizeof(tSirMacAddr));
           sme_HandoffRequest(pHddCtx->hHal, pAdapter->sessionId, &handoffInfo);
#endif
       }
#endif
#if defined(FEATURE_WLAN_ESE) && defined(FEATURE_WLAN_ESE_UPLOAD)
       else if (strncmp(command, "CCXPLMREQ", 9) == 0)
       {
           tANI_U8 *value = command;
           eHalStatus status = eHAL_STATUS_SUCCESS;
           tpSirPlmReq pPlmRequest;

           pPlmRequest = vos_mem_malloc(sizeof(tSirPlmReq));
           if (NULL == pPlmRequest){
               ret = -ENOMEM;
               goto exit;
           }

           status = hdd_parse_plm_cmd(value, pPlmRequest);
           if (eHAL_STATUS_SUCCESS != status){
               vos_mem_free(pPlmRequest);
               pPlmRequest = NULL;
               ret = -EINVAL;
               goto exit;
           }
           pPlmRequest->sessionId = pAdapter->sessionId;

           status = sme_SetPlmRequest(pHddCtx->hHal, pPlmRequest);
           if (eHAL_STATUS_SUCCESS != status)
           {
               vos_mem_free(pPlmRequest);
               pPlmRequest = NULL;
               ret = -EINVAL;
               goto exit;
           }
       }
#endif
#ifdef FEATURE_WLAN_ESE
       else if (strncmp(command, "SETCCXMODE", 10) == 0)
       {
           tANI_U8 *value = command;
           tANI_U8 eseMode = CFG_ESE_FEATURE_ENABLED_DEFAULT;

           if (hdd_drv_cmd_validate(command, 10))
               goto exit;

           /* Check if the features OKC/ESE/11R are supported simultaneously,
              then this operation is not permitted (return FAILURE) */
           if (sme_getIsEseFeatureEnabled(pHddCtx->hHal) &&
               hdd_is_okc_mode_enabled(pHddCtx) &&
               sme_getIsFtFeatureEnabled(pHddCtx->hHal))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_WARN,
                  "%s: OKC/ESE/11R are supported simultaneously"
                  " hence this operation is not permitted!", __func__);
               ret = -EPERM;
               goto exit;
           }

           /* Move pointer to ahead of SETCCXMODE<delimiter> */
           value = value + 11;
           /* Convert the value from ascii to integer */
           ret = kstrtou8(value, 10, &eseMode);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype, then also
                  kstrtou8 fails */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed range [%d - %d]", __func__,
                      CFG_ESE_FEATURE_ENABLED_MIN,
                      CFG_ESE_FEATURE_ENABLED_MAX);
               ret = -EINVAL;
               goto exit;
           }
           if ((eseMode < CFG_ESE_FEATURE_ENABLED_MIN) ||
               (eseMode > CFG_ESE_FEATURE_ENABLED_MAX))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "Ese mode value %d is out of range"
                      " (Min: %d Max: %d)", eseMode,
                      CFG_ESE_FEATURE_ENABLED_MIN,
                      CFG_ESE_FEATURE_ENABLED_MAX);
               ret = -EINVAL;
               goto exit;
           }
           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to change ese mode = %d", __func__, eseMode);

           pHddCtx->cfg_ini->isEseIniFeatureEnabled = eseMode;
           sme_UpdateIsEseFeatureEnabled(pHddCtx->hHal, pAdapter->sessionId,
                                         eseMode);
       }
#endif
       else if (strncmp(command, "SETROAMSCANCONTROL", 18) == 0)
       {
           tANI_U8 *value = command;
           tANI_BOOLEAN roamScanControl = 0;

           if (hdd_drv_cmd_validate(command, 18))
               goto exit;

           /* Move pointer to ahead of SETROAMSCANCONTROL<delimiter> */
           value = value + 19;
           /* Convert the value from ascii to integer */
           ret = kstrtou8(value, 10, &roamScanControl);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype, then also
                  kstrtou8 fails */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed ", __func__);
               ret = -EINVAL;
               goto exit;
           }

           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to Set roam scan control = %d", __func__, roamScanControl);

           if (0 != roamScanControl)
           {
               ret = 0; /* return success but ignore param value "TRUE" */
               goto exit;
           }

           sme_SetRoamScanControl(pHddCtx->hHal,
                                  pAdapter->sessionId, roamScanControl);
       }
#ifdef FEATURE_WLAN_OKC
       else if (strncmp(command, "SETOKCMODE", 10) == 0)
       {
           tANI_U8 *value = command;
           tANI_U8 okcMode = CFG_OKC_FEATURE_ENABLED_DEFAULT;

           if (hdd_drv_cmd_validate(command, 10))
               goto exit;

           /* Check if the features OKC/ESE/11R are supported simultaneously,
              then this operation is not permitted (return FAILURE) */
           if (sme_getIsEseFeatureEnabled(pHddCtx->hHal) &&
               hdd_is_okc_mode_enabled(pHddCtx) &&
               sme_getIsFtFeatureEnabled(pHddCtx->hHal))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_WARN,
                  "%s: OKC/ESE/11R are supported simultaneously"
                  " hence this operation is not permitted!", __func__);
               ret = -EPERM;
               goto exit;
           }

           /* Move pointer to ahead of SETOKCMODE<delimiter> */
           value = value + 11;
           /* Convert the value from ascii to integer */
           ret = kstrtou8(value, 10, &okcMode);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype, then also
                  kstrtou8 fails */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed range [%d - %d]", __func__,
                      CFG_OKC_FEATURE_ENABLED_MIN,
                      CFG_OKC_FEATURE_ENABLED_MAX);
               ret = -EINVAL;
               goto exit;
           }

           if ((okcMode < CFG_OKC_FEATURE_ENABLED_MIN) ||
               (okcMode > CFG_OKC_FEATURE_ENABLED_MAX))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "Okc mode value %d is out of range"
                      " (Min: %d Max: %d)", okcMode,
                      CFG_OKC_FEATURE_ENABLED_MIN,
                      CFG_OKC_FEATURE_ENABLED_MAX);
               ret = -EINVAL;
               goto exit;
           }

           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to change okc mode = %d", __func__, okcMode);
           pHddCtx->cfg_ini->isOkcIniFeatureEnabled = okcMode;
       }
#endif  /* FEATURE_WLAN_OKC */
       else if (strncmp(command, "BTCOEXMODE", 10) == 0 )
       {
           char *bcMode;
           int ret;

           if (hdd_drv_cmd_validate(command, 10))
               goto exit;

           bcMode = command + 11;
           if ('1' == *bcMode)
           {
               VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_DEBUG,
                         FL("BTCOEXMODE %d"), *bcMode);
               pHddCtx->btCoexModeSet = TRUE;
               ret = wlan_hdd_scan_abort(pAdapter);
               if (ret < 0) {
                   hddLog(LOGE,
                          FL("Failed to abort existing scan status:%d"), ret);
               }
           }
           else if ('2' == *bcMode)
           {
               VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_DEBUG,
                         FL("BTCOEXMODE %d"), *bcMode);
               pHddCtx->btCoexModeSet = FALSE;
           }
       }
       else if (strncmp(command, "GETROAMSCANCONTROL", 18) == 0)
       {
           tANI_BOOLEAN roamScanControl = sme_GetRoamScanControl(pHddCtx->hHal);
           char extra[32];
           tANI_U8 len = 0;

           len = scnprintf(extra, sizeof(extra), "%s %d",
                   command, roamScanControl);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
#ifdef WLAN_FEATURE_PACKET_FILTERING
       else if (strncmp(command, "ENABLE_PKTFILTER_IPV6", 21) == 0)
       {
           tANI_U8 filterType = 0;
           tANI_U8 *value = command;

           if (hdd_drv_cmd_validate(command, 21))
               goto exit;

           /* Move pointer to ahead of ENABLE_PKTFILTER_IPV6<delimiter> */
           value = value + 22;

           /* Convert the value from ascii to integer */
           ret = kstrtou8(value, 10, &filterType);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype,
                * then also kstrtou8 fails
                */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed range ", __func__);
               ret = -EINVAL;
               goto exit;
           }

           if (filterType != 0 && filterType != 1)
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: Accepted Values are 0 and 1 ", __func__);
               ret = -EINVAL;
               goto exit;
           }
           wlan_hdd_setIPv6Filter(WLAN_HDD_GET_CTX(pAdapter), filterType,
                   pAdapter->sessionId);
       }
#endif
       else if (strncmp(command, "BTCOEXMODE", 10) == 0 )
       {
           char *bcMode;

           if (hdd_drv_cmd_validate(command, 10))
               goto exit;

           bcMode = command + 11;
           if ('1' == *bcMode)
           {
               VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_DEBUG,
                         FL("BTCOEXMODE %d"), *bcMode);
               pHddCtx->btCoexModeSet = TRUE;
           }
           else if ('2' == *bcMode)
           {
               VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_DEBUG,
                         FL("BTCOEXMODE %d"), *bcMode);
               pHddCtx->btCoexModeSet = FALSE;
           }
       }
       else if (strncmp(command, "SCAN-ACTIVE", 11) == 0)
       {
          hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
          hddLog(LOG1, FL("making default scan to ACTIVE"));
          pHddCtx->ioctl_scan_mode = eSIR_ACTIVE_SCAN;
       }
       else if (strncmp(command, "SCAN-PASSIVE", 12) == 0)
       {
          hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
          hddLog(LOG1, FL("making default scan to PASSIVE"));
          pHddCtx->ioctl_scan_mode = eSIR_PASSIVE_SCAN;
       }
       else if (strncmp(command, "GETDWELLTIME", 12) == 0)
       {
           hdd_config_t *pCfg = (WLAN_HDD_GET_CTX(pAdapter))->cfg_ini;
           char extra[32];
           tANI_U8 len = 0;

           memset(extra, 0, sizeof(extra));
           ret = hdd_get_dwell_time(pCfg, command, extra, sizeof(extra), &len);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (ret != 0 || copy_to_user(priv_data.buf, &extra, len))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
           ret = len;
       }
       else if (strncmp(command, "SETDWELLTIME", 12) == 0)
       {
           ret = hdd_set_dwell_time(pAdapter, command);
       }
       else if ( strncasecmp(command, "MIRACAST", 8) == 0 )
       {
           VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                   "%s: Received MIRACAST command", __func__);

           if (hdd_drv_cmd_validate(command, 8))
               goto exit;

           ret = hdd_set_miracast_mode(pAdapter, command);
       }
       else if ((strncasecmp(command, "SETIBSSBEACONOUIDATA", 20) == 0) &&
                (WLAN_HDD_IBSS == pAdapter->device_mode))
       {
           int i = 0;
           uint8_t *ibss_ie;
           int32_t command_len;
           int32_t oui_length = 0;
           uint8_t *value = command;
           uint32_t ibss_ie_length;
           tSirModifyIE  ibssModifyIE;
           tCsrRoamProfile   *pRoamProfile;
           hdd_wext_state_t *pWextState =
                WLAN_HDD_GET_WEXT_STATE_PTR(pAdapter);

           int status;

           VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                     "%s: received command %s", __func__, ((char *) value));

           /* validate argument of command */
           if (strlen(value) <= 21) {
               hddLog(LOGE,
                   FL("No arguements in command length %zu"), strlen(value));
               ret = -EFAULT;
               goto exit;
           }

           if (hdd_drv_cmd_validate(command, 20))
               goto exit;

           /* moving to arguments of commands */
           value = value + 21;
           command_len = strlen(value);

           /* oui_data can't be less than 3 bytes */
           if (command_len <= (2 * WLAN_HDD_IBSS_MIN_OUI_DATA_LENGTH)) {
               hddLog(LOGE,
                     FL("Invalid SETIBSSBEACONOUIDATA command length %d"),
                     command_len);
               ret = -EFAULT;
               goto exit;
           }

           ibss_ie = vos_mem_malloc(command_len);
           if (!ibss_ie) {
               hddLog(LOGE,
                     FL("Could not allocate memory for command length %d"),
                     command_len);
               ret = -ENOMEM;
               goto exit;
           }
           vos_mem_zero(ibss_ie, command_len);

           ibss_ie_length = hdd_parse_set_ibss_oui_data_command(value, ibss_ie,
                                                      &oui_length, command_len);
           if (ibss_ie_length < (2 * WLAN_HDD_IBSS_MIN_OUI_DATA_LENGTH)) {
               hddLog(LOGE, FL("Could not parse command %s return length %d"),
                     value, ibss_ie_length);
               ret = -EFAULT;
               vos_mem_free(ibss_ie);
               goto exit;
           }

           pRoamProfile = &pWextState->roamProfile;

           vos_mem_copy(ibssModifyIE.bssid,
                        pRoamProfile->BSSIDs.bssid,
                        VOS_MAC_ADDR_SIZE);

           ibssModifyIE.smeSessionId = pAdapter->sessionId;
           ibssModifyIE.notify = TRUE;
           ibssModifyIE.ieID = IE_EID_VENDOR;
           ibssModifyIE.ieIDLen = ibss_ie_length;
           ibssModifyIE.ieBufferlength = ibss_ie_length;
           ibssModifyIE.pIEBuffer = ibss_ie;
           ibssModifyIE.oui_length = oui_length;

           hddLog(LOGW, FL("ibss_ie length %d oui_length %d ibss_ie:"),
                ibss_ie_length, oui_length);
           while (i < ibssModifyIE.ieBufferlength) {
              hddLog(LOGW, FL("0x%x"), ibss_ie[i++]);
           }

           /* Probe Bcn modification */
           sme_ModifyAddIE(WLAN_HDD_GET_HAL_CTX(pAdapter),
                                        &ibssModifyIE,
                                        eUPDATE_IE_PROBE_BCN);

           /* Populating probe resp frame */
           sme_ModifyAddIE(WLAN_HDD_GET_HAL_CTX(pAdapter),
                                       &ibssModifyIE,
                                       eUPDATE_IE_PROBE_RESP);
           /* free ibss_ie */
           vos_mem_free(ibss_ie);

           status = sme_SendCesiumEnableInd( (tHalHandle)(pHddCtx->hHal),
                         pAdapter->sessionId );
           if (VOS_STATUS_SUCCESS != status) {
                hddLog(LOGE, FL("cesium enable indication failed %d"),
                    status);
                ret = -EINVAL;
                goto exit;
           }

       }
       else if (strncasecmp(command, "SETRMCENABLE", 12) == 0)
       {
          tANI_U8 *value = command;
          tANI_U8 ucRmcEnable = 0;
          int  status;

          if ((WLAN_HDD_IBSS != pAdapter->device_mode) &&
              (WLAN_HDD_SOFTAP != pAdapter->device_mode))
          {
             VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "Received SETRMCENABLE command in invalid mode %d"
                "SETRMCENABLE command is only allowed in IBSS or SOFTAP mode",
                pAdapter->device_mode);
             ret = -EINVAL;
             goto exit;
          }

          status = hdd_parse_setrmcenable_command(value, &ucRmcEnable);
          if (status)
          {
             VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "Invalid SETRMCENABLE command ");
             ret = -EINVAL;
             goto exit;
          }

          VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
               "%s: ucRmcEnable %d ", __func__, ucRmcEnable);

          if (TRUE == ucRmcEnable) {
              status = sme_enable_rmc((tHalHandle)(pHddCtx->hHal),
                         pAdapter->sessionId);
          }
          else if(FALSE == ucRmcEnable) {
              status = sme_disable_rmc((tHalHandle)(pHddCtx->hHal),
                         pAdapter->sessionId);
          }
          else
          {
             VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "Invalid SETRMCENABLE command %d", ucRmcEnable);
             ret = -EINVAL;
             goto exit;
          }

          if (VOS_STATUS_SUCCESS != status)
          {
              VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "%s: SETRMC %d failed status %d", __func__, ucRmcEnable,
                 status);
              ret = -EINVAL;
              goto exit;
          }
       }
       else if (strncasecmp(command, "SETRMCACTIONPERIOD", 18) == 0)
       {
          tANI_U8 *value = command;
          tANI_U32 uActionPeriod = 0;
          int  status;

          if ((WLAN_HDD_IBSS != pAdapter->device_mode) &&
              (WLAN_HDD_SOFTAP != pAdapter->device_mode))
          {
             VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "Received SETRMC command in invalid mode %d"
                "SETRMC command is only allowed in IBSS or SOFTAP mode",
                pAdapter->device_mode);
             ret = -EINVAL;
             goto exit;
          }

          status = hdd_parse_setrmcactionperiod_command(value, &uActionPeriod);
          if (status)
          {
             VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "Invalid SETRMCACTIONPERIOD command ");
             ret = -EINVAL;
             goto exit;
          }

          VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
               "%s: uActionPeriod %d ", __func__, uActionPeriod);

          if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_RMC_ACTION_PERIOD_FREQUENCY,
                 uActionPeriod, NULL, eANI_BOOLEAN_FALSE))
          {
              VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "%s: Could not set SETRMCACTIONPERIOD %d", __func__, uActionPeriod);
              ret = -EINVAL;
              goto exit;
          }

          status = sme_SendRmcActionPeriod( (tHalHandle)(pHddCtx->hHal),
                         pAdapter->sessionId );
          if (VOS_STATUS_SUCCESS != status)
          {
             VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "Could not send cesium enable indication %d", status);
             ret = -EINVAL;
             goto exit;
          }

      }
      else if (strncasecmp(command, "GETIBSSPEERINFOALL", 18) == 0)
      {
         /* Peer Info All Command */
         int status = eHAL_STATUS_SUCCESS;
         hdd_station_ctx_t *pHddStaCtx = NULL;
         char *extra = NULL;
         int idx = 0, length = 0;
         uint8_t mac_addr[VOS_MAC_ADDR_SIZE];
         v_U32_t numOfBytestoPrint = 0;

         if (WLAN_HDD_IBSS == pAdapter->device_mode)
         {
            pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
         }
         else
         {
            VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: pAdapter is not valid for this device mode",
                      __func__);
            ret = -EINVAL;
            goto exit;
         }

         VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                 "%s: Received GETIBSSPEERINFOALL Command", __func__);

         /* Handle the command */
         status = hdd_cfg80211_get_ibss_peer_info_all(pAdapter);
         if (VOS_STATUS_SUCCESS == status)
         {
            /* The variable extra needed to be allocated on the heap since
             * amount of memory required to copy the data for 32 devices
             * exceeds the size of 1024 bytes of default stack size. On
             * 64 bit devices, the default max stack size of 2048 bytes
             */
            extra = vos_mem_malloc(WLAN_MAX_BUF_SIZE);

            if (NULL == extra)
            {
               VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                         "%s:kmalloc failed", __func__);
               ret = -EINVAL;
               goto exit;
            }

            /* Copy number of stations */
            length = scnprintf( extra, WLAN_MAX_BUF_SIZE, "%d ",
                             pHddStaCtx->ibss_peer_info.numPeers);
            numOfBytestoPrint = length;
            for (idx = 0; idx < pHddStaCtx->ibss_peer_info.numPeers; idx++) {
               int8_t rssi;
               uint32_t tx_rate;

               vos_mem_copy(mac_addr,
                  pHddStaCtx->ibss_peer_info.peerInfoParams[idx].mac_addr,
                  sizeof(mac_addr));

                  tx_rate = pHddStaCtx->ibss_peer_info.peerInfoParams[idx].txRate;
                  /* Only lower 3 bytes are rate info. Mask of the MSByte */
                  tx_rate &= 0x00FFFFFF;

                  rssi = pHddStaCtx->ibss_peer_info.peerInfoParams[idx].rssi;

                  length += scnprintf((extra + length),
                            WLAN_MAX_BUF_SIZE - length,
                            "%02x:%02x:%02x:%02x:%02x:%02x %d %d ",
                            mac_addr[0], mac_addr[1], mac_addr[2],
                            mac_addr[3], mac_addr[4], mac_addr[5],
                            tx_rate, rssi);
                 /*
                  * VOS_TRACE() macro has limitation of 512 bytes for the print
                  * buffer. Hence printing the data in two chunks. The first
                  * chunk will have the data for 16 devices and the second
                  * chunk will have the rest.
                  */
                  if (idx < NUM_OF_STA_DATA_TO_PRINT)
                       numOfBytestoPrint = length;
            }

            /*
             * Copy the data back into buffer, if the data to copy is
             * more than 512 bytes than we will split the data and do
             * it in two shots
             */
            if (copy_to_user(priv_data.buf, extra, numOfBytestoPrint))
            {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                          "%s: Copy into user data buffer failed ", __func__);
               ret = -EFAULT;
               goto mem_free;
            }
            /* This overwrites the last space, which we already copied */
            extra[numOfBytestoPrint - 1] = '\0';
            VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_MED,
                      "%s", extra);

            if (length > numOfBytestoPrint)
            {
                if (copy_to_user(priv_data.buf + numOfBytestoPrint,
                                 extra + numOfBytestoPrint,
                                 length - numOfBytestoPrint + 1))
                {
                    VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                              "%s: Copy into user data buffer failed ", __func__);
                    ret = -EFAULT;
                    goto mem_free;
                }
                VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_MED,
                          "%s", &extra[numOfBytestoPrint]);
            }
            ret = 0;
mem_free:
            /* Free temporary buffer */
            vos_mem_free(extra);
         }

         else
         {
            /* Command failed, log error */
            VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: GETIBSSPEERINFOALL command failed with status code %d",
                      __func__, status);
            ret = -EINVAL;
            goto exit;
         }
      }
      else if(strncasecmp(command, "GETIBSSPEERINFO", 15) == 0)
      {
         /* Peer Info <Peer Addr> command */
         tANI_U8 *value = command;
         VOS_STATUS status;
         hdd_station_ctx_t *pHddStaCtx = NULL;
         char extra[128] = { 0 };
         v_U32_t length = 0;
         v_U8_t staIdx = 0;
         v_MACADDR_t peerMacAddr;

         if (WLAN_HDD_IBSS == pAdapter->device_mode)
         {
            pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
         }
         else
         {
            VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: pAdapter is not valid for this device mode",
                      __func__);
            ret = -EINVAL;
            goto exit;
         }

         /* if there are no peers, no need to continue with the command */
         VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                 "%s: Received GETIBSSPEERINFO Command", __func__);

         if (eConnectionState_IbssConnected != pHddStaCtx->conn_info.connState)
         {
            VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s:No IBSS Peers coalesced", __func__);
            ret = -EINVAL;
            goto exit;
         }

         /* Parse the incoming command buffer */
         status = hdd_parse_get_ibss_peer_info(value, &peerMacAddr);
         if (VOS_STATUS_SUCCESS != status)
         {
            VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: Invalid GETIBSSPEERINFO command", __func__);
            ret = -EINVAL;
            goto exit;
         }

         /* Get station index for the peer mac address and sanitize it */
         hdd_get_peer_sta_id(pHddStaCtx, &peerMacAddr, &staIdx);

         if (staIdx > HDD_MAX_NUM_IBSS_STA)
         {
            VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: Invalid StaIdx %d returned", __func__, staIdx);
            ret = -EINVAL;
            goto exit;
         }

         /* Handle the command */
         status = hdd_cfg80211_get_ibss_peer_info(pAdapter, staIdx);
         if (VOS_STATUS_SUCCESS == status)
         {
           v_U32_t txRate = pHddStaCtx->ibss_peer_info.peerInfoParams[0].txRate;
           /* Only lower 3 bytes are rate info. Mask of the MSByte */
           txRate &= 0x00FFFFFF;

           length = scnprintf( extra, sizeof(extra), "%d %d", (int)txRate,
                      (int)pHddStaCtx->ibss_peer_info.peerInfoParams[0].rssi);

            /* Copy the data back into buffer */
            if (copy_to_user(priv_data.buf, &extra, length+ 1))
            {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: copy data to user buffer failed GETIBSSPEERINFO command",
                  __func__);
               ret = -EFAULT;
               goto exit;
            }
         }
         else
         {
            /* Command failed, log error */
            VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: GETIBSSPEERINFO command failed with status code %d",
                      __func__, status);
            ret = -EINVAL;
            goto exit;
         }

         /* Success ! */
         VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_MED,
                   "%s", priv_data.buf);
         ret = 0;
       }
       else if (strncmp(command, "SETRMCTXRATE", 12) == 0)
       {
          tANI_U8 *value = command;
          tANI_U32 uRate = 0;
          tTxrateinfoflags txFlags = 0;
          tSirRateUpdateInd rateUpdateParams = {0};
          int  status;
          hdd_config_t *pConfig = pHddCtx->cfg_ini;

          if ((WLAN_HDD_IBSS != pAdapter->device_mode) &&
              (WLAN_HDD_SOFTAP != pAdapter->device_mode))
          {
             VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "Received SETRMCTXRATE command in invalid mode %d"
                "SETRMC command is only allowed in IBSS or SOFTAP mode",
                pAdapter->device_mode);
             ret = -EINVAL;
             goto exit;
          }

          status = hdd_parse_setrmcrate_command(value, &uRate, &txFlags);
          if (status)
          {
             VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "Invalid SETRMCTXRATE command ");
             ret = -EINVAL;
             goto exit;
          }

          VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
               "%s: uRate %d ", __func__, uRate);

          /* -1 implies ignore this param */
          rateUpdateParams.ucastDataRate = -1;

          /*
           * Fill the user specified RMC rate param
           * and the derived tx flags.
           */
          rateUpdateParams.nss = (pConfig->enable2x2 == 0) ? 0 : 1;
          rateUpdateParams.reliableMcastDataRate = uRate;
          rateUpdateParams.reliableMcastDataRateTxFlag = txFlags;
          rateUpdateParams.dev_mode = pAdapter->device_mode;
          rateUpdateParams.bcastDataRate = -1;
          memcpy(rateUpdateParams.bssid, pAdapter->macAddressCurrent.bytes,
             sizeof(rateUpdateParams.bssid));
          status = sme_SendRateUpdateInd((tHalHandle)(pHddCtx->hHal),
                      &rateUpdateParams);
       }
       else if (strncasecmp(command, "SETIBSSTXFAILEVENT", 18) == 0)
       {
           char *value;
           tANI_U8 tx_fail_count = 0;
           tANI_U16 pid = 0;

           value = command;

           ret = hdd_ParseIBSSTXFailEventParams(value, &tx_fail_count, &pid);

           if (0 != ret)
           {
              hddLog(VOS_TRACE_LEVEL_INFO,
                     "%s: Failed to parse SETIBSSTXFAILEVENT arguments",
                     __func__);
              goto exit;
           }

           hddLog(VOS_TRACE_LEVEL_INFO, "%s: tx_fail_cnt=%hhu, pid=%hu",
                   __func__, tx_fail_count, pid);

           if (0 == tx_fail_count)
           {
               // Disable TX Fail Indication
               if (eHAL_STATUS_SUCCESS  ==
                   sme_TXFailMonitorStartStopInd(pHddCtx->hHal,
                                                 tx_fail_count,
                                                NULL))
               {
                   cesium_pid = 0;
               }
               else
               {
                   VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                             "%s: failed to disable TX Fail Event ", __func__);
                  ret = -EINVAL;
               }
           }
           else
           {
               if (eHAL_STATUS_SUCCESS  ==
                   sme_TXFailMonitorStartStopInd(pHddCtx->hHal,
                                                 tx_fail_count,
                                           (void*)hdd_tx_fail_ind_callback))
               {
                   cesium_pid = pid;
                   VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                             "%s: Registered Cesium pid %u", __func__,
                             cesium_pid);
               }
               else
               {
                   VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                             "%s: Failed to enable TX Fail Monitoring", __func__);
                   ret = -EINVAL;
               }
           }
       }
#if defined(FEATURE_WLAN_ESE) && defined(FEATURE_WLAN_ESE_UPLOAD)
       else if (strncmp(command, "SETCCXROAMSCANCHANNELS", 22) == 0)
       {
           tANI_U8 *value = command;
           tANI_U8 ChannelList[WNI_CFG_VALID_CHANNEL_LIST_LEN] = {0};
           tANI_U8 numChannels = 0;
           eHalStatus status;
           ret = hdd_parse_channellist(value, ChannelList, &numChannels);
           if (ret)
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: Failed to parse channel list information", __func__);
               goto exit;
           }
           if (numChannels > WNI_CFG_VALID_CHANNEL_LIST_LEN)
           {
               VOS_TRACE( VOS_MODULE_ID_HDD,
                  VOS_TRACE_LEVEL_ERROR,
                  "%s: number of channels (%d) supported exceeded max (%d)",
                  __func__,
                  numChannels,
                  WNI_CFG_VALID_CHANNEL_LIST_LEN);
               ret = -EINVAL;
               goto exit;
           }
           status = sme_SetEseRoamScanChannelList(pHddCtx->hHal,
                                                  pAdapter->sessionId,
                                                  ChannelList,
                                                  numChannels);
           if (eHAL_STATUS_SUCCESS != status)
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: Failed to update channel list information", __func__);
               ret = -EINVAL;
               goto exit;
           }
       }
       else if (strncmp(command, "GETTSMSTATS", 11) == 0)
       {
           tANI_U8            *value = command;
           char                extra[128] = {0};
           int                 len = 0;
           tANI_U8             tid = 0;
           hdd_station_ctx_t  *pHddStaCtx = NULL;
           tAniTrafStrmMetrics tsmMetrics;

           if (hdd_drv_cmd_validate(command, 11))
               goto exit;

           pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
           /* if not associated, return error */
           if (eConnectionState_Associated != pHddStaCtx->conn_info.connState)
           {
               VOS_TRACE(VOS_MODULE_ID_HDD,
                         VOS_TRACE_LEVEL_ERROR,
                         "%s:Not associated!",
                         __func__);
               ret = -EINVAL;
               goto exit;
           }
           /* Move pointer to ahead of GETTSMSTATS<delimiter> */
           value = value + 12;
           /* Convert the value from ascii to integer */
           ret = kstrtou8(value, 10, &tid);
           if (ret < 0)
           {
               /* If the input value is greater than max value of datatype,
                * then also
                * kstrtou8 fails
                */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed range [%d - %d]", __func__,
                      TID_MIN_VALUE,
                      TID_MAX_VALUE);
               ret = -EINVAL;
               goto exit;
           }
           if ((tid < TID_MIN_VALUE) || (tid > TID_MAX_VALUE))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "tid value %d is out of range"
                      " (Min: %d Max: %d)", tid,
                      TID_MIN_VALUE,
                      TID_MAX_VALUE);
               ret = -EINVAL;
               goto exit;
           }
           VOS_TRACE( VOS_MODULE_ID_HDD,
                      VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to get tsm stats tid = %d",
                      __func__,
                      tid);
           if (VOS_STATUS_SUCCESS != hdd_get_tsm_stats(pAdapter, tid, &tsmMetrics))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to get tsm stats", __func__);
               ret = -EFAULT;
               goto exit;
           }
           VOS_TRACE( VOS_MODULE_ID_HDD,
                      VOS_TRACE_LEVEL_INFO,
                      "UplinkPktQueueDly(%d)"
                      "UplinkPktQueueDlyHist[0](%d)"
                      "UplinkPktQueueDlyHist[1](%d)"
                      "UplinkPktQueueDlyHist[2](%d)"
                      "UplinkPktQueueDlyHist[3](%d)"
                      "UplinkPktTxDly(%u)"
                      "UplinkPktLoss(%d)"
                      "UplinkPktCount(%d)"
                      "RoamingCount(%d)"
                      "RoamingDly(%d)",
                      tsmMetrics.UplinkPktQueueDly,
                      tsmMetrics.UplinkPktQueueDlyHist[0],
                      tsmMetrics.UplinkPktQueueDlyHist[1],
                      tsmMetrics.UplinkPktQueueDlyHist[2],
                      tsmMetrics.UplinkPktQueueDlyHist[3],
                      tsmMetrics.UplinkPktTxDly,
                      tsmMetrics.UplinkPktLoss,
                      tsmMetrics.UplinkPktCount,
                      tsmMetrics.RoamingCount,
                      tsmMetrics.RoamingDly);
           /* Output TSM stats is of the format
            * GETTSMSTATS [PktQueueDly]
            * [PktQueueDlyHist[0]]:[PktQueueDlyHist[1]] ...[RoamingDly]
            * eg., GETTSMSTATS 10 1:0:0:161 20 1 17 8 39800 */
           len = scnprintf(extra,
                           sizeof(extra),
                           "%s %d %d:%d:%d:%d %u %d %d %d %d",
                           command,
                           tsmMetrics.UplinkPktQueueDly,
                           tsmMetrics.UplinkPktQueueDlyHist[0],
                           tsmMetrics.UplinkPktQueueDlyHist[1],
                           tsmMetrics.UplinkPktQueueDlyHist[2],
                           tsmMetrics.UplinkPktQueueDlyHist[3],
                           tsmMetrics.UplinkPktTxDly,
                           tsmMetrics.UplinkPktLoss,
                           tsmMetrics.UplinkPktCount,
                           tsmMetrics.RoamingCount,
                           tsmMetrics.RoamingDly);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
       else if (strncmp(command, "SETCCKMIE", 9) == 0)
       {
           tANI_U8 *value = command;
           tANI_U8 *cckmIe = NULL;
           tANI_U8 cckmIeLen = 0;
           eHalStatus status = eHAL_STATUS_SUCCESS;
           status = hdd_parse_get_cckm_ie(value, &cckmIe, &cckmIeLen);
           if (eHAL_STATUS_SUCCESS != status)
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: Failed to parse cckm ie data", __func__);
               ret = -EINVAL;
               goto exit;
           }
           if (cckmIeLen > DOT11F_IE_RSN_MAX_LEN)
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: CCKM Ie input length is more than max[%d]", __func__,
                  DOT11F_IE_RSN_MAX_LEN);
               if (NULL != cckmIe)
               {
                   vos_mem_free(cckmIe);
                   cckmIe = NULL;
               }
               ret = -EINVAL;
               goto exit;
           }
           sme_SetCCKMIe(pHddCtx->hHal, pAdapter->sessionId, cckmIe, cckmIeLen);
           if (NULL != cckmIe)
           {
               vos_mem_free(cckmIe);
               cckmIe = NULL;
           }
       }
       else if (strncmp(command, "CCXBEACONREQ", 12) == 0)
       {
           tANI_U8 *value = command;
           tCsrEseBeaconReq eseBcnReq;
           eHalStatus status = eHAL_STATUS_SUCCESS;

           status = hdd_parse_ese_beacon_req(value, &eseBcnReq);
           if (eHAL_STATUS_SUCCESS != status)
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: Failed to parse ese beacon req", __func__);
               ret = -EINVAL;
               goto exit;
           }

           if (!hdd_connIsConnected(WLAN_HDD_GET_STATION_CTX_PTR(pAdapter))) {
               hddLog(VOS_TRACE_LEVEL_INFO, FL("Not associated"));
               hdd_indicateEseBcnReportNoResults (pAdapter,
                                      eseBcnReq.bcnReq[0].measurementToken,
                                      0x02,  //BIT(1) set for measurement done
                                      0);    // no BSS
               goto exit;
           }

           status = sme_SetEseBeaconRequest(pHddCtx->hHal,
                                            pAdapter->sessionId,
                                            &eseBcnReq);

           if (eHAL_STATUS_RESOURCES == status) {
               hddLog(VOS_TRACE_LEVEL_INFO,
                      FL("sme_SetEseBeaconRequest failed (%d),"
                      " a request already in progress"), status);
               ret = -EBUSY;
               goto exit;
           } else if (eHAL_STATUS_SUCCESS != status) {
               VOS_TRACE( VOS_MODULE_ID_HDD,
                          VOS_TRACE_LEVEL_ERROR,
                          "%s: sme_SetEseBeaconRequest failed (%d)",
                         __func__,
                         status);
               ret = -EINVAL;
               goto exit;
           }
       }
#endif /* FEATURE_WLAN_ESE && FEATURE_WLAN_ESE_UPLOAD */
       else if (strncmp(command, "SETMCRATE", 9) == 0)
       {
           tANI_U8 *value = command;
           int      targetRate;

           if (hdd_drv_cmd_validate(command, 9))
               goto exit;

           /* Move pointer to ahead of SETMCRATE<delimiter> */
           /* input value is in units of hundred kbps */
           value = value + 10;
           /* Convert the value from ascii to integer, decimal base */
           ret = kstrtouint(value, 10, &targetRate);
           ret = wlan_hdd_set_mc_rate(pAdapter, targetRate);
       }
       else if (strncmp(command, "MAXTXPOWER", 10) == 0)
       {
           int status;
           int txPower;
           VOS_STATUS vosStatus;
           eHalStatus smeStatus;
           tANI_U8 *value = command;
           hdd_adapter_t      *pAdapter;
           tSirMacAddr bssid = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
           tSirMacAddr selfMac = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
           hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;

           status = hdd_parse_setmaxtxpower_command(value, &txPower);
           if (status)
           {
             VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "Invalid MAXTXPOWER command ");
             ret = -EINVAL;
             goto exit;
           }

           vosStatus = hdd_get_front_adapter ( pHddCtx, &pAdapterNode );
           while ( NULL != pAdapterNode && VOS_STATUS_SUCCESS == vosStatus )
           {
               pAdapter = pAdapterNode->pAdapter;
               /* Assign correct self MAC address */
               vos_mem_copy(bssid, pAdapter->macAddressCurrent.bytes,
                   VOS_MAC_ADDR_SIZE);
               vos_mem_copy(selfMac, pAdapter->macAddressCurrent.bytes,
                   VOS_MAC_ADDR_SIZE);

               hddLog(VOS_TRACE_LEVEL_INFO, "Device mode %d max tx power %d"
                   " selfMac: " MAC_ADDRESS_STR " bssId: " MAC_ADDRESS_STR " ",
                   pAdapter->device_mode, txPower, MAC_ADDR_ARRAY(selfMac),
                   MAC_ADDR_ARRAY(bssid));
               smeStatus = sme_SetMaxTxPower((tHalHandle)(pHddCtx->hHal), bssid,
                                                              selfMac, txPower);
               if (eHAL_STATUS_SUCCESS != status)
               {
                   hddLog(VOS_TRACE_LEVEL_ERROR, "%s:Set max tx power failed",
                      __func__);
                   ret = -EINVAL;
                   goto exit;
               }
               hddLog(VOS_TRACE_LEVEL_INFO, "%s: Set max tx power success",
                   __func__);
               vosStatus = hdd_get_next_adapter(pHddCtx, pAdapterNode, &pNext);
               pAdapterNode = pNext;
           }
       }
       else if (strncmp(command, "SETDFSSCANMODE", 14) == 0)
       {
           tANI_U8 *value = command;
           tANI_U8 dfsScanMode = CFG_ROAMING_DFS_CHANNEL_DEFAULT;

           if (hdd_drv_cmd_validate(command, 14))
               goto exit;

           /* Move pointer to ahead of SETDFSSCANMODE<delimiter> */
           value = value + 15;
           /* Convert the value from ascii to integer */
           ret = kstrtou8(value, 10, &dfsScanMode);
           if (ret < 0)
           {
               /* If the input value is greater than max value of
                * datatype, then also kstrtou8 fails
                */
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: kstrtou8 failed range [%d - %d]", __func__,
                      CFG_ROAMING_DFS_CHANNEL_MIN,
                      CFG_ROAMING_DFS_CHANNEL_MAX);
               ret = -EINVAL;
               goto exit;
           }

           if ((dfsScanMode < CFG_ROAMING_DFS_CHANNEL_MIN) ||
               (dfsScanMode > CFG_ROAMING_DFS_CHANNEL_MAX))
           {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "dfsScanMode value %d is out of range"
                      " (Min: %d Max: %d)", dfsScanMode,
                      CFG_ROAMING_DFS_CHANNEL_MIN,
                      CFG_ROAMING_DFS_CHANNEL_MAX);
               ret = -EINVAL;
               goto exit;
           }
           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "%s: Received Command to Set DFS Scan Mode = %d",
                      __func__, dfsScanMode);

           /* When DFS scanning is disabled, the DFS channels need to be
            * removed from the operation of device.
            */
           ret = wlan_hdd_disable_dfs_chan_scan(pHddCtx, pAdapter,
                        (dfsScanMode == CFG_ROAMING_DFS_CHANNEL_DISABLED));
           if (ret < 0) {
               /* Some conditions prevented it from disabling DFS channels
                */
               hddLog(LOGE,
                   FL("disable/enable DFS channel request was denied"));
               goto exit;
           }

           pHddCtx->cfg_ini->allowDFSChannelRoam = dfsScanMode;
           sme_UpdateDFSScanMode(pHddCtx->hHal, pAdapter->sessionId,
                                 dfsScanMode);
       }
       else if (strncmp(command, "GETDFSSCANMODE", 14) == 0)
       {
           tANI_U8 dfsScanMode = sme_GetDFSScanMode(pHddCtx->hHal);
           char extra[32];
           tANI_U8 len = 0;

           len = scnprintf(extra, sizeof(extra), "%s %d", command, dfsScanMode);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
       else if (strncmp(command, "GETLINKSTATUS", 13) == 0) {
           int value = wlan_hdd_get_link_status(pAdapter);
           char extra[32];
           tANI_U8 len = 0;

           len = scnprintf(extra, sizeof(extra), "%s %d", command, value);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: failed to copy data to user buffer", __func__);
               ret = -EFAULT;
               goto exit;
           }
       }
#ifdef WLAN_FEATURE_EXTWOW_SUPPORT
       else if (strncmp(command, "ENABLEEXTWOW", 12) == 0) {

           tANI_U8 *value = command;
           int set_value;

           if (hdd_drv_cmd_validate(command, 12))
               goto exit;

           /* Move pointer to ahead of ENABLEEXTWOW*/
           value += 12;
           if (!(sscanf(value, "%d", &set_value))) {
                     VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                               FL("No input identified"));
                     ret = -EINVAL;
                     goto exit;
           }
           ret = hdd_enable_ext_wow_parser(pAdapter,
                               pAdapter->sessionId, set_value);

       } else if (strncmp(command, "SETAPP1PARAMS", 13) == 0) {
           tANI_U8 *value = command;

           if (hdd_drv_cmd_validate(command, 13))
               goto exit;

           /* Move pointer to ahead of SETAPP1PARAMS*/
           value += 13;
           ret = hdd_set_app_type1_parser(pAdapter,
                                         value, strlen(value));
           if (ret >= 0)
               pHddCtx->is_extwow_app_type1_param_set = TRUE;

       } else if (strncmp(command, "SETAPP2PARAMS", 13) == 0) {
           tANI_U8 *value = command;

           if (hdd_drv_cmd_validate(command, 13))
               goto exit;

           /* Move pointer to ahead of SETAPP2PARAMS*/
           value += 13;
           ret = hdd_set_app_type2_parser(pAdapter,
                                         value, strlen(value));
           if (ret >= 0)
               pHddCtx->is_extwow_app_type2_param_set = TRUE;
       }
#endif
#ifdef FEATURE_WLAN_TDLS
       else if (strncmp(command, "TDLSSECONDARYCHANNELOFFSET", 26) == 0) {
           tANI_U8 *value = command;
           int set_value;

           if (hdd_drv_cmd_validate(command, 26))
               goto exit;

           /* Move pointer to point the string */
           value += 26;
           if (!(sscanf(value, "%d", &set_value))) {
                     VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                               FL("No input identified"));
               ret = -EINVAL;
               goto exit;
           }
           VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                     FL("Tdls offchannel offset:%d"),
                     set_value);
           ret = hdd_set_tdls_secoffchanneloffset(pHddCtx, set_value);
       } else if (strncmp(command, "TDLSOFFCHANNELMODE", 18) == 0) {
           tANI_U8 *value = command;
           int set_value;

           if (hdd_drv_cmd_validate(command, 18))
               goto exit;

           /* Move pointer to point the string */
           value += 18;
           if (!(sscanf(value, "%d", &set_value))) {
                     VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                               FL("No input identified"));
               ret = -EINVAL;
               goto exit;
           }
           VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                     FL("Tdls offchannel mode:%d"),
                     set_value);
           ret = hdd_set_tdls_offchannelmode(pAdapter, set_value);
       } else if (strncmp(command, "TDLSOFFCHANNEL", 14) == 0) {
           tANI_U8 *value = command;
           int set_value;

           if (hdd_drv_cmd_validate(command, 14))
               goto exit;

           /* Move pointer to point the string */
           value += 14;
           ret = sscanf(value, "%d", &set_value);
           if (ret != 1) {
               VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                     "Wrong value is given for hdd_set_tdls_offchannel");
               ret = -EINVAL;
               goto exit;
           }

           if (VOS_IS_DFS_CH(set_value)) {
               hddLog(LOGE,
                     FL("DFS channel %d is passed for hdd_set_tdls_offchannel"),
                     set_value);
               ret = -EINVAL;
               goto exit;
           }
           VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                FL("Tdls offchannel num: %d"), set_value);
           ret = hdd_set_tdls_offchannel(pHddCtx, set_value);
       } else if (strncmp(command, "TDLSSCAN", 8) == 0) {
           uint8_t *value = command;
           int set_value;

           if (hdd_drv_cmd_validate(command, 8))
               goto exit;

           /* Move pointer to point the string */
           value += 8;
           ret = sscanf(value, "%d", &set_value);
           if (ret != 1) {
               VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                     "Wrong value is given for tdls_scan_type");
               ret = -EINVAL;
               goto exit;
           }
           hddLog(LOG1, FL("Tdls scan type val: %d"),
                  set_value);
           ret = hdd_set_tdls_scan_type(pHddCtx, set_value);
       }
#endif
       else if (strncasecmp(command, "RSSI", 4) == 0) {
           v_S7_t s7Rssi = 0;
           char extra[32];
           tANI_U8 len = 0;

           wlan_hdd_get_rssi(pAdapter, &s7Rssi);

           len = scnprintf(extra, sizeof(extra), "%s %d", command, s7Rssi);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               hddLog(LOGE, FL("Failed to copy data to user buffer"));
               ret = -EFAULT;
               goto exit;
           }
       } else if (strncasecmp(command, "LINKSPEED", 9) == 0) {
           uint32_t link_speed = 0;
           char extra[32];
           uint8_t len = 0;

           ret = wlan_hdd_get_link_speed(pAdapter, &link_speed);
           if (0 != ret)
               goto exit;

           len = scnprintf(extra, sizeof(extra), "%s %d", command, link_speed);
           len = VOS_MIN(priv_data.total_len, len + 1);
           if (copy_to_user(priv_data.buf, &extra, len)) {
               hddLog(LOGE, FL("Failed to copy data to user buffer"));
               ret = -EFAULT;
               goto exit;
           }
       } else if (strncasecmp(command, "SET_FCC_CHANNEL", 15) == 0) {
           /*
            * this command wld be called by user-space when it detects WLAN
            * ON after airplane mode is set. When APM is set, WLAN turns off.
            * But it can be turned back on. Otherwise; when APM is turned back
            * off, WLAN wld turn back on. So at that point the command is
            * expected to come down. 0 means disable, 1 means enable. The
            * constraint is removed when parameter 1 is set or different
            * country code is set
            */

           if (hdd_drv_cmd_validate(command, 15))
               goto exit;

           ret = drv_cmd_set_fcc_channel(pAdapter, command, 15);

       } else if (strncmp(command, "RXFILTER-REMOVE", 15) == 0) {

           if (hdd_drv_cmd_validate(command, 15))
               goto exit;

           ret = hdd_driver_rxfilter_comand_handler(command, pAdapter, false);

       } else if (strncmp(command, "RXFILTER-ADD", 12) == 0) {

           if (hdd_drv_cmd_validate(command, 12))
               goto exit;

           ret = hdd_driver_rxfilter_comand_handler(command, pAdapter, true);

       } else if (strncasecmp(command, "SETANTENNAMODE", 14) == 0) {
           ret = drv_cmd_set_antenna_mode(pAdapter, command, 14);
           hddLog(LOG1, FL("set antenna mode ret: %d"), ret);
       } else if (strncasecmp(command, "GETANTENNAMODE", 14) == 0) {

           ret = drv_cmd_get_antenna_mode(pAdapter, pHddCtx, command,
                                          14, &priv_data);
       } else if (strncmp(command, "STOP", 4) == 0) {
          hddLog(LOG1, FL("STOP command"));
          pHddCtx->driver_being_stopped = true;
       } else if ((strncmp(command, "BTCOEXSCAN", 10) == 0) &&
                  (((tANI_U8 *)strchrnul(command, ' ') - command) == 10)) {
          hddLog(LOG1, FL("ignore BTCOEXSCAN and return -ENOTSUPP"));
          ret = -ENOTSUPP;
       } else if ((strncmp(command, "RXFILTER", 8) == 0) &&
                  (((tANI_U8 *)strchrnul(command, ' ') - command) == 8)) {
          hddLog(LOG1, FL("ignore RXFILTER and return -ENOTSUPP"));
          ret = -ENOTSUPP;
       } else if ((strncmp(command, "RXFILTER-START", 14) == 0) &&
                  (((tANI_U8 *)strchrnul(command, ' ') - command) == 14)) {
          hddLog(LOG1, FL("ignore RXFILTER-START and return 0"));
          ret = 0;
       } else if ((strncmp(command, "RXFILTER-STOP", 13) == 0) &&
                  (((tANI_U8 *)strchrnul(command, ' ') - command) == 13)) {
          hddLog(LOG1, FL("ignore RXFILTER-STOP and return 0"));
          ret = 0;
       } else if ((strncmp(command, "BTCOEXSCAN-START", 16) == 0) &&
                  (((tANI_U8 *)strchrnul(command, ' ') - command) == 16)) {
          hddLog(LOG1, FL("ignore BTCOEXSCAN-START and return 0"));
          ret = 0;
       } else if ((strncmp(command, "BTCOEXSCAN-STOP", 15) == 0) &&
                  (((tANI_U8 *)strchrnul(command, ' ') - command) == 15)) {
          hddLog(LOG1, FL("ignore BTCOEXSCAN-STOP and return 0"));
          ret = 0;
       } else {
           MTRACE(vos_trace(VOS_MODULE_ID_HDD,
                            TRACE_CODE_HDD_UNSUPPORTED_IOCTL,
                            pAdapter->sessionId, 0));
           hddLog( VOS_TRACE_LEVEL_WARN, "%s: Unsupported GUI command %s",
                   __func__, command);
       }

   }
exit:
   if (command)
   {
       vos_mem_free(command);
   }
   EXIT();
   return ret;
}

#ifdef CONFIG_COMPAT
static int hdd_driver_compat_ioctl(hdd_adapter_t *pAdapter, void __user *data)
{
   struct {
      compat_uptr_t buf;
      int used_len;
      int total_len;
   } compat_priv_data;
   hdd_priv_data_t priv_data;
   int ret = 0;

   /*
    * Note that pAdapter and ifr have already been verified by caller,
    * and HDD context has also been validated
    */
   if (copy_from_user(&compat_priv_data, data,
                      sizeof(compat_priv_data))) {
       ret = -EFAULT;
       goto exit;
   }
   priv_data.buf = compat_ptr(compat_priv_data.buf);
   priv_data.used_len = compat_priv_data.used_len;
   priv_data.total_len = compat_priv_data.total_len;
   ret = hdd_driver_command(pAdapter, &priv_data);
 exit:
   return ret;
}
#else /* CONFIG_COMPAT */
static int hdd_driver_compat_ioctl(hdd_adapter_t *pAdapter, void __user *data)
{
   /* will never be invoked */
   return 0;
}
#endif /* CONFIG_COMPAT */

static int hdd_driver_ioctl(hdd_adapter_t *pAdapter, void __user *data)
{
   hdd_priv_data_t priv_data;
   int ret = 0;

   /*
    * Note that pAdapter and ifr have already been verified by caller,
    * and HDD context has also been validated
    */
   if (copy_from_user(&priv_data, data, sizeof(priv_data))) {
       ret = -EFAULT;
   } else {
      ret = hdd_driver_command(pAdapter, &priv_data);
   }
   return ret;
}

/**
 * __hdd_ioctl() - HDD ioctl handler
 * @dev: pointer to net_device structure
 * @ifr: pointer to ifreq structure
 * @cmd: ioctl command
 *
 * Return: 0 for success and error number for failure.
 */
static int __hdd_ioctl(struct net_device *dev, void __user *data, int cmd)
{
   hdd_adapter_t *pAdapter = WLAN_HDD_GET_PRIV_PTR(dev);
   hdd_context_t *pHddCtx;
   long ret = 0;

   ENTER();

   if (dev != pAdapter->dev) {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_FATAL,
                 "%s: HDD adapter/dev inconsistency", __func__);
      ret = -ENODEV;
      goto exit;
   }

   if (!data)
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "%s: invalid data", __func__);
      ret = -EINVAL;
      goto exit;
   }

#if  defined(QCA_WIFI_FTM) && defined(LINUX_QCMBR)
   if (VOS_FTM_MODE == hdd_get_conparam()) {
       if (SIOCIOCTLTX99 == cmd) {
           ret = wlan_hdd_qcmbr_unified_ioctl(pAdapter, data);
           goto exit;
       }
   }
#endif

   pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
   ret = wlan_hdd_validate_context(pHddCtx);
   if (ret) {
      ret = -EBUSY;
      goto exit;
   }

   switch (cmd) {
   case (SIOCDEVPRIVATE + 1):
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0)) && defined(CONFIG_X86_64)
      if (in_compat_syscall())
#else
      if (is_compat_task())
#endif
         ret = hdd_driver_compat_ioctl(pAdapter, data);
      else
         ret = hdd_driver_ioctl(pAdapter, data);
      break;
   default:
      hddLog(VOS_TRACE_LEVEL_ERROR, "%s: unknown ioctl %d",
             __func__, cmd);
      ret = -EINVAL;
      break;
   }
 exit:
   EXIT();
   return ret;
}

/**
 * hdd_ioctl() - Wrapper function to protect __hdd_ioctl() function from SSR
 * @dev: pointer to net_device structure
 * @ifr: pointer to ifreq structure
 * @cmd: ioctl command
 *
 * Return: 0 for success and error number for failure.
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
static int
hdd_dev_private_ioctl(struct net_device *dev, struct ifreq *ifr,
		      void __user *data, int cmd)
{
	int ret;

	vos_ssr_protect(__func__);
	ret = __hdd_ioctl(dev, data, cmd);
	vos_ssr_unprotect(__func__);

	return ret;
}
#else
static int
hdd_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	int ret;

	vos_ssr_protect(__func__);
	ret = __hdd_ioctl(dev, ifr->ifr_data, cmd);
	vos_ssr_unprotect(__func__);

	return ret;
}
#endif

/*
 * Mac address for multiple virtual interface is found as following
 * i) The mac address of the first interface is just the actual hw mac address.
 * ii) MSM 3 or 4 bits of byte5 of the actual mac address are used to
 *     define the mac address for the remaining interfaces and locally
 *     administered bit is set. INTF_MACADDR_MASK is based on the number of
 *     supported virtual interfaces, right now this is 0x07 (meaning 8 interface).
 *     Byte[3] of second interface will be hw_macaddr[3](bit5..7) + 1,
 *     for third interface it will be hw_macaddr[3](bit5..7) + 2, etc.
 */

void hdd_update_macaddr(hdd_config_t *cfg_ini, v_MACADDR_t hw_macaddr)
{
    int8_t i;
    u_int8_t macaddr_b3, tmp_br3;

    hddLog(VOS_TRACE_LEVEL_INFO,
           "hw update mac addr[0] from " MAC_ADDRESS_STR
           " to " MAC_ADDRESS_STR,
           MAC_ADDR_ARRAY(cfg_ini->intfMacAddr[0].bytes),
           MAC_ADDR_ARRAY(hw_macaddr.bytes));
    vos_mem_copy(cfg_ini->intfMacAddr[0].bytes, hw_macaddr.bytes,
                 VOS_MAC_ADDR_SIZE);
    for (i = 1; i < VOS_MAX_CONCURRENCY_PERSONA; i++) {
        hddLog(VOS_TRACE_LEVEL_INFO,
               "hw update mac addr[%d] from " MAC_ADDRESS_STR,
               i,
               MAC_ADDR_ARRAY(cfg_ini->intfMacAddr[i].bytes));
        vos_mem_copy(cfg_ini->intfMacAddr[i].bytes, hw_macaddr.bytes,
                     VOS_MAC_ADDR_SIZE);
        macaddr_b3 = cfg_ini->intfMacAddr[i].bytes[3];
        tmp_br3 = ((macaddr_b3 >> 4 & INTF_MACADDR_MASK) + i) &
                  INTF_MACADDR_MASK;
        macaddr_b3 += tmp_br3;

        /* XOR-ing bit-24 of the mac address. This will give enough
         * mac address range before collision
         */
        macaddr_b3 ^= (1 << 7);

        /* Set locally administered bit */
        cfg_ini->intfMacAddr[i].bytes[0] |= 0x02;
        cfg_ini->intfMacAddr[i].bytes[3] = macaddr_b3;
        hddLog(VOS_TRACE_LEVEL_INFO,
               " to " MAC_ADDRESS_STR,
               MAC_ADDR_ARRAY(cfg_ini->intfMacAddr[i].bytes));
        hddLog(VOS_TRACE_LEVEL_INFO, "cfg_ini->intfMacAddr[%d]: "
               MAC_ADDRESS_STR, i,
               MAC_ADDR_ARRAY(cfg_ini->intfMacAddr[i].bytes));
    }
}

static void hdd_update_tgt_services(hdd_context_t *hdd_ctx,
                                    struct hdd_tgt_services *cfg)
{
    hdd_config_t *cfg_ini = hdd_ctx->cfg_ini;

    /* Set up UAPSD */
    cfg_ini->apUapsdEnabled &= cfg->uapsd;

#ifdef WLAN_FEATURE_11AC
    /* 11AC mode support */
    if ((cfg_ini->dot11Mode == eHDD_DOT11_MODE_11ac ||
        cfg_ini->dot11Mode == eHDD_DOT11_MODE_11ac_ONLY) &&
                                               !cfg->en_11ac)
        cfg_ini->dot11Mode = eHDD_DOT11_MODE_AUTO;
#endif /* #ifdef WLAN_FEATURE_11AC */

    /* ARP offload: override user setting if invalid  */
    cfg_ini->fhostArpOffload &= cfg->arp_offload;

#ifdef FEATURE_WLAN_SCAN_PNO
    /* PNO offload */
    hddLog(VOS_TRACE_LEVEL_INFO_HIGH, "%s: PNO Capability in f/w = %d",
           __func__,cfg->pno_offload);
    if (cfg->pno_offload)
        cfg_ini->PnoOffload = TRUE;
#endif
    sme_set_lte_coex_supp(hdd_ctx->hHal,
                    cfg->lte_coex_ant_share);
    hdd_ctx->per_band_chainmask_supp = cfg->per_band_chainmask_supp;
    sme_set_per_band_chainmask_supp(hdd_ctx->hHal,
                    cfg->per_band_chainmask_supp);
#ifdef FEATURE_WLAN_TDLS
    cfg_ini->fEnableTDLSSupport &= cfg->en_tdls;
    cfg_ini->fEnableTDLSOffChannel = cfg_ini->fEnableTDLSOffChannel &&
                                     cfg->en_tdls_offchan;
    cfg_ini->fEnableTDLSBufferSta = cfg_ini->fEnableTDLSBufferSta &&
                                    cfg->en_tdls_uapsd_buf_sta;
    if (cfg_ini->fTDLSUapsdMask && cfg->en_tdls_uapsd_sleep_sta)
    {
        cfg_ini->fEnableTDLSSleepSta = TRUE;
    }
    else
    {
        cfg_ini->fEnableTDLSSleepSta = FALSE;
    }
#endif
    sme_set_bcon_offload_supp(hdd_ctx->hHal, cfg->beacon_offload);
#ifdef WLAN_FEATURE_ROAM_OFFLOAD
    cfg_ini->isRoamOffloadEnabled &= cfg->en_roam_offload;
#endif

#ifdef SAP_AUTH_OFFLOAD
    cfg_ini->enable_sap_auth_offload &= cfg->sap_auth_offload_service;
#endif
    cfg_ini->sap_get_peer_info &= cfg->get_peer_info_enabled;
    if (cfg_ini->wowEnable && (!cfg->wow_support)) {
        hdd_ctx->prevent_suspend = true;
        cfg_ini->wowEnable = 0;
    } else {
        hdd_ctx->prevent_suspend = false;
    }
}

/**
 * hdd_update_chain_mask_vdev_nss() - sets the chain mask and vdev nss
 * @hdd_ctx: HDD context
 * @cfg: Pointer to target services.
 *
 * Sets the chain masks for 2G and 5G bands based on target supported
 * values and INI values. And sets the Nss per vdev type based on INI
 * and configured chain mask value.
 *
 * Return: None
 */
static void hdd_update_chain_mask_vdev_nss(hdd_context_t *hdd_ctx,
		struct hdd_tgt_services *cfg)
{
	hdd_config_t *cfg_ini = hdd_ctx->cfg_ini;
	uint8_t chain_mask_rx, chain_mask_tx, ret;
	uint8_t max_supp_nss = 1;

	cfg_ini->enable2x2 = 0;
	chain_mask_rx = cfg->chain_mask_2g & cfg_ini->chain_mask_2g_rx;
	chain_mask_tx = cfg->chain_mask_2g & cfg_ini->chain_mask_2g_tx;
	if (!chain_mask_rx)
		chain_mask_rx = cfg->chain_mask_2g;
	if (!chain_mask_tx)
		chain_mask_tx = chain_mask_rx;
	hddLog(LOG1,
		FL("set 2G chain mask rx %d tx %d"),
		chain_mask_rx, chain_mask_tx);
	ret = process_wma_set_command(0, WMI_PDEV_PARAM_RX_CHAIN_MASK_2G,
			chain_mask_rx, PDEV_CMD);
	if (0 != ret) {
		hddLog(VOS_TRACE_LEVEL_ERROR,
			"%s: set WMI_PDEV_PARAM_RX_CHAIN_MASK_2G failed %d",
			__func__, ret);
	}
	ret = process_wma_set_command(0, WMI_PDEV_PARAM_TX_CHAIN_MASK_2G,
			chain_mask_tx, PDEV_CMD);
	if (0 != ret) {
		hddLog(VOS_TRACE_LEVEL_ERROR,
			"%s: WMI_PDEV_PARAM_TX_CHAIN_MASK_2G set failed %d",
			__func__, ret);
	}
	if (((chain_mask_rx & 0x3) == 0x3) ||
		((chain_mask_tx & 0x3) == 0x3))
		max_supp_nss++;

	if (max_supp_nss == 2)
		cfg_ini->enable2x2 = 1;
	sme_update_vdev_type_nss(hdd_ctx->hHal, max_supp_nss,
			cfg_ini->vdev_type_nss_2g, eCSR_BAND_24);

	if (chain_mask_rx >= chain_mask_tx)
		hdd_ctx->supp_2g_chain_mask = chain_mask_rx;
	else
		hdd_ctx->supp_2g_chain_mask = chain_mask_tx;

	max_supp_nss = 1;
	chain_mask_rx = cfg->chain_mask_5g & cfg_ini->chain_mask_5g_rx;
	chain_mask_tx = cfg->chain_mask_5g & cfg_ini->chain_mask_5g_tx;
	if (!chain_mask_rx)
		chain_mask_rx = cfg->chain_mask_5g;
	if (!chain_mask_tx)
		chain_mask_tx = chain_mask_rx;
	hddLog(LOG1,
		FL("set 5G chain mask rx %d tx %d"),
		chain_mask_rx, chain_mask_tx);
	ret = process_wma_set_command(0, WMI_PDEV_PARAM_RX_CHAIN_MASK_5G,
			chain_mask_rx, PDEV_CMD);
	if (0 != ret) {
		hddLog(VOS_TRACE_LEVEL_ERROR,
			"%s: set WMI_PDEV_PARAM_RX_CHAIN_MASK_5G failed %d",
			__func__, ret);
	}
	ret = process_wma_set_command(0, WMI_PDEV_PARAM_TX_CHAIN_MASK_5G,
			chain_mask_tx, PDEV_CMD);
	if (0 != ret) {
		hddLog(VOS_TRACE_LEVEL_ERROR,
			"%s: WMI_PDEV_PARAM_TX_CHAIN_MASK_5G set failed %d",
			__func__, ret);
	}
	if (((chain_mask_rx & 0x3) == 0x3) ||
		((chain_mask_tx & 0x3) == 0x3))
		max_supp_nss++;

	if (max_supp_nss == 2)
		cfg_ini->enable2x2 = 1;
	sme_update_vdev_type_nss(hdd_ctx->hHal, max_supp_nss,
			cfg_ini->vdev_type_nss_5g, eCSR_BAND_5G);
	if (chain_mask_rx >= chain_mask_tx)
		hdd_ctx->supp_5g_chain_mask = chain_mask_rx;
	else
		hdd_ctx->supp_5g_chain_mask = chain_mask_tx;
	hddLog(LOG1, FL("Supported chain mask 2G: %d 5G: %d"),
	       hdd_ctx->supp_2g_chain_mask,
	       hdd_ctx->supp_5g_chain_mask);
}

static void hdd_update_tgt_ht_cap(hdd_context_t *hdd_ctx,
                                  struct hdd_tgt_ht_cap *cfg)
{
    eHalStatus status;
    tANI_U32 value, val32;
    tANI_U16 val16;
    hdd_config_t *pconfig = hdd_ctx->cfg_ini;
    tSirMacHTCapabilityInfo *phtCapInfo;
    tANI_U8 mcs_set[SIZE_OF_SUPPORTED_MCS_SET];
    uint8_t enable_tx_stbc;

    /* check and update RX STBC */
    if (pconfig->enableRxSTBC && !cfg->ht_rx_stbc)
        pconfig->enableRxSTBC = cfg->ht_rx_stbc;

    /* get the MPDU density */
    status = ccmCfgGetInt(hdd_ctx->hHal, WNI_CFG_MPDU_DENSITY, &value);

    if (status != eHAL_STATUS_SUCCESS) {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                  "%s: could not get MPDU DENSITY",
                  __func__);
        value = 0;
    }

    /*
     * MPDU density:
     * override user's setting if value is larger
     * than the one supported by target
     */
    if (value > cfg->mpdu_density) {
        status = ccmCfgSetInt(hdd_ctx->hHal, WNI_CFG_MPDU_DENSITY,
                              cfg->mpdu_density,
                              NULL, eANI_BOOLEAN_FALSE);

        if (status == eHAL_STATUS_FAILURE)
            VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                      "%s: could not set MPDU DENSITY to CCM",
                      __func__);
    }

    /* get the HT capability info*/
    status = ccmCfgGetInt(hdd_ctx->hHal, WNI_CFG_HT_CAP_INFO, &val32);
    if (eHAL_STATUS_SUCCESS != status) {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                  "%s: could not get HT capability info",
                  __func__);
        return;
    }
    val16 = (tANI_U16)val32;
    phtCapInfo = (tSirMacHTCapabilityInfo *)&val16;

    /* Set the LDPC capability */
    phtCapInfo->advCodingCap = cfg->ht_rx_ldpc;


    if (pconfig->ShortGI20MhzEnable && !cfg->ht_sgi_20)
        pconfig->ShortGI20MhzEnable = cfg->ht_sgi_20;

    if (pconfig->ShortGI40MhzEnable && !cfg->ht_sgi_40)
        pconfig->ShortGI40MhzEnable = cfg->ht_sgi_40;

    hdd_ctx->num_rf_chains     = cfg->num_rf_chains;
    hdd_ctx->ht_tx_stbc_supported = cfg->ht_tx_stbc;

    enable_tx_stbc = pconfig->enableTxSTBC;

    if (pconfig->enable2x2 && (hdd_ctx->per_band_chainmask_supp ||
          (!hdd_ctx->per_band_chainmask_supp && (cfg->num_rf_chains == 2))))
    {
        pconfig->enable2x2 = 1;
    }
    else
    {
        pconfig->enable2x2 = 0;
        enable_tx_stbc = 0;

        /* 1x1 */
        /* Update Rx Highest Long GI data Rate */
        if (ccmCfgSetInt(hdd_ctx->hHal,
                    WNI_CFG_VHT_RX_HIGHEST_SUPPORTED_DATA_RATE,
                    VHT_RX_HIGHEST_SUPPORTED_DATA_RATE_1_1, NULL,
                    eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
        {
            hddLog(LOGE, "Could not pass on "
                    "WNI_CFG_VHT_RX_HIGHEST_SUPPORTED_DATA_RATE to CCM");
        }

        /* Update Tx Highest Long GI data Rate */
        if (ccmCfgSetInt(hdd_ctx->hHal, WNI_CFG_VHT_TX_HIGHEST_SUPPORTED_DATA_RATE,
                    VHT_TX_HIGHEST_SUPPORTED_DATA_RATE_1_1, NULL,
                    eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
        {
            hddLog(LOGE, "Could not pass on "
                    "HDD_VHT_RX_HIGHEST_SUPPORTED_DATA_RATE_1_1 to CCM");
        }
    }
    if (!(cfg->ht_tx_stbc && pconfig->enable2x2))
    {
        enable_tx_stbc = 0;
    }
    phtCapInfo->txSTBC = enable_tx_stbc;
    val32 = val16;
    status = ccmCfgSetInt(hdd_ctx->hHal, WNI_CFG_HT_CAP_INFO,
                          val32, NULL, eANI_BOOLEAN_FALSE);
    if (status != eHAL_STATUS_SUCCESS)
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                  "%s: could not set HT capability to CCM",
                  __func__);
#define WLAN_HDD_RX_MCS_ALL_NSTREAM_RATES 0xff
    value = SIZE_OF_SUPPORTED_MCS_SET;
    if (ccmCfgGetStr(hdd_ctx->hHal, WNI_CFG_SUPPORTED_MCS_SET, mcs_set,
                     &value) == eHAL_STATUS_SUCCESS) {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_INFO,
                  "%s: Read MCS rate set", __func__);

        if (pconfig->enable2x2)
        {
            for (value = 0; value < 2; value++)
                mcs_set[value] = WLAN_HDD_RX_MCS_ALL_NSTREAM_RATES;

            status = ccmCfgSetStr(hdd_ctx->hHal, WNI_CFG_SUPPORTED_MCS_SET,
                                  mcs_set, SIZE_OF_SUPPORTED_MCS_SET, NULL,
                                  eANI_BOOLEAN_FALSE);
            if (status == eHAL_STATUS_FAILURE)
                VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                          "%s: could not set MCS SET to CCM", __func__);
        }
    }
#undef WLAN_HDD_RX_MCS_ALL_NSTREAM_RATES
}

#ifdef WLAN_FEATURE_11AC
static void hdd_update_tgt_vht_cap(hdd_context_t *hdd_ctx,
                                   struct hdd_tgt_vht_cap *cfg)
{
    eHalStatus status;
    tANI_U32 value = 0;
    hdd_config_t *pconfig = hdd_ctx->cfg_ini;
    tANI_U32 temp = 0;
    tANI_U32 enable_tx_stbc;

    /* Get the current MPDU length */
    status = ccmCfgGetInt(hdd_ctx->hHal, WNI_CFG_VHT_MAX_MPDU_LENGTH, &value);

    if (status != eHAL_STATUS_SUCCESS) {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                  "%s: could not get MPDU LENGTH",
                  __func__);
        value = 0;
    }

    /*
     * VHT max MPDU length:
     * override if user configured value is too high
     * that the target cannot support
     */
    if (value > cfg->vht_max_mpdu) {
        status = ccmCfgSetInt(hdd_ctx->hHal,
                              WNI_CFG_VHT_MAX_MPDU_LENGTH,
                              cfg->vht_max_mpdu, NULL,
                              eANI_BOOLEAN_FALSE);

        if (status == eHAL_STATUS_FAILURE) {
            VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                      "%s: could not set VHT MAX MPDU LENGTH",
                      __func__);
        }
    }

    /* Get the current supported chan width */
    status = ccmCfgGetInt(hdd_ctx->hHal, WNI_CFG_VHT_SUPPORTED_CHAN_WIDTH_SET,
                          &value);

    if (status != eHAL_STATUS_SUCCESS) {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                  "%s: could not get MPDU LENGTH",
                  __func__);
        value = 0;
    }

    /*
     * Update VHT supported chan width:
     * if user setting is invalid, override it with
     * target capability
     */
    if ((value == eHT_CHANNEL_WIDTH_80MHZ &&
        !(cfg->supp_chan_width & eHT_CHANNEL_WIDTH_80MHZ)) ||
        (value == eHT_CHANNEL_WIDTH_160MHZ &&
        !(cfg->supp_chan_width & eHT_CHANNEL_WIDTH_160MHZ))) {
        u_int32_t width = eHT_CHANNEL_WIDTH_20MHZ;

        if (cfg->supp_chan_width & eHT_CHANNEL_WIDTH_160MHZ)
            width = eHT_CHANNEL_WIDTH_160MHZ;
        else if (cfg->supp_chan_width & eHT_CHANNEL_WIDTH_80MHZ)
            width = eHT_CHANNEL_WIDTH_80MHZ;

        status = ccmCfgSetInt(hdd_ctx->hHal,
                              WNI_CFG_VHT_SUPPORTED_CHAN_WIDTH_SET,
                              width, NULL, eANI_BOOLEAN_FALSE);

        if (status == eHAL_STATUS_FAILURE) {
            VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                      "%s: could not set VHT SUPPORTED CHAN WIDTH",
                      __func__);
        }
    }

    ccmCfgGetInt(hdd_ctx->hHal, WNI_CFG_VHT_BASIC_MCS_SET, &temp);
    temp = (temp & VHT_MCS_1x1) | pconfig->vhtRxMCS;

    if (pconfig->enable2x2)
            temp = (temp & VHT_MCS_2x2) | (pconfig->vhtRxMCS2x2 << 2);

    if (ccmCfgSetInt(hdd_ctx->hHal, WNI_CFG_VHT_BASIC_MCS_SET, temp, NULL,
                            eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE) {
        hddLog(LOGE, "Could not pass on WNI_CFG_VHT_BASIC_MCS_SET to CCM");
    }

    ccmCfgGetInt(hdd_ctx->hHal, WNI_CFG_VHT_RX_MCS_MAP, &temp);
    temp = (temp & VHT_MCS_1x1) | pconfig->vhtRxMCS;
    if (pconfig->enable2x2)
        temp = (temp & VHT_MCS_2x2) | (pconfig->vhtRxMCS2x2 << 2);

    if (ccmCfgSetInt(hdd_ctx->hHal, WNI_CFG_VHT_RX_MCS_MAP, temp, NULL,
                            eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE) {
        hddLog(LOGE, "Could not pass on WNI_CFG_VHT_RX_MCS_MAP to CCM");
    }

    ccmCfgGetInt(hdd_ctx->hHal, WNI_CFG_VHT_TX_MCS_MAP, &temp);
    temp = (temp & VHT_MCS_1x1) | pconfig->vhtTxMCS;
    if (pconfig->enable2x2)
        temp = (temp & VHT_MCS_2x2) | (pconfig->vhtTxMCS2x2 << 2);

    VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
                    "vhtRxMCS2x2 - %x temp - %u enable2x2 %d",
                    pconfig->vhtRxMCS2x2, temp, pconfig->enable2x2);

    if (ccmCfgSetInt(hdd_ctx->hHal, WNI_CFG_VHT_TX_MCS_MAP, temp, NULL,
                            eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE) {
        hddLog(LOGE, "Could not pass on WNI_CFG_VHT_TX_MCS_MAP to CCM");
    }
    /* Get the current RX LDPC setting */
    status = ccmCfgGetInt(hdd_ctx->hHal, WNI_CFG_VHT_LDPC_CODING_CAP, &value);

    if (status != eHAL_STATUS_SUCCESS) {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                  "%s: could not get VHT LDPC CODING CAP",
                  __func__);
        value = 0;
    }

    /* Set the LDPC capability */
    if (value && !cfg->vht_rx_ldpc) {
        status = ccmCfgSetInt(hdd_ctx->hHal,
                              WNI_CFG_VHT_LDPC_CODING_CAP,
                              cfg->vht_rx_ldpc, NULL,
                              eANI_BOOLEAN_FALSE);

        if (status == eHAL_STATUS_FAILURE) {
            VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                      "%s: could not set VHT LDPC CODING CAP to CCM",
                      __func__);
        }
    }

    /* Get current GI 80 value */
    status = ccmCfgGetInt(hdd_ctx->hHal, WNI_CFG_VHT_SHORT_GI_80MHZ, &value);

    if (status != eHAL_STATUS_SUCCESS) {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                  "%s: could not get SHORT GI 80MHZ",
                  __func__);
        value = 0;
    }

    /* set the Guard interval 80MHz */
    if (value && !cfg->vht_short_gi_80) {
        status = ccmCfgSetInt(hdd_ctx->hHal,
                              WNI_CFG_VHT_SHORT_GI_80MHZ,
                              cfg->vht_short_gi_80, NULL,
                              eANI_BOOLEAN_FALSE);

        if (status == eHAL_STATUS_FAILURE) {
            VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                      "%s: could not set SHORT GI 80MHZ to CCM",
                      __func__);
        }
    }

    /* Get current GI 160 value */
    status = ccmCfgGetInt(hdd_ctx->hHal,
                          WNI_CFG_VHT_SHORT_GI_160_AND_80_PLUS_80MHZ,
                          &value);

    if (status != eHAL_STATUS_SUCCESS) {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                  "%s: could not get SHORT GI 80 & 160",
                  __func__);
        value = 0;
    }

    /* set the Guard interval 160MHz */
    if (value && !cfg->vht_short_gi_160) {
        status = ccmCfgSetInt(hdd_ctx->hHal,
                              WNI_CFG_VHT_SHORT_GI_160_AND_80_PLUS_80MHZ,
                              cfg->vht_short_gi_160, NULL,
                              eANI_BOOLEAN_FALSE);

        if (status == eHAL_STATUS_FAILURE) {
            VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                      "%s: could not set SHORT GI 80 & 160 to CCM",
                      __func__);
        }
    }

    /* Get VHT TX STBC cap */
    enable_tx_stbc = pconfig->enableTxSTBC;
    if (!(cfg->vht_tx_stbc && pconfig->enable2x2))
        enable_tx_stbc = 0;
    VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_DEBUG,
			  "%s: vht stbc ini enableTxSTBC %x,target %x, 2x2 %d",
			  __func__, pconfig->enableTxSTBC, cfg->vht_tx_stbc,
			  pconfig->enable2x2);

    /* VHT TX STBC cap */

    status = ccmCfgSetInt(hdd_ctx->hHal, WNI_CFG_VHT_TXSTBC,
                          enable_tx_stbc, NULL,
                          eANI_BOOLEAN_FALSE);

    if (status == eHAL_STATUS_FAILURE) {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                  "%s: could not set the VHT TX STBC to CCM",
                  __func__);
    }

    /* Get VHT RX STBC cap */
    status = ccmCfgGetInt(hdd_ctx->hHal, WNI_CFG_VHT_RXSTBC, &value);

    if (status != eHAL_STATUS_SUCCESS) {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                  "%s: could not get VHT RX STBC",
                  __func__);
        value = 0;
    }

    /* VHT RX STBC cap */
    if (value && !cfg->vht_rx_stbc) {
        status = ccmCfgSetInt(hdd_ctx->hHal, WNI_CFG_VHT_RXSTBC,
                              cfg->vht_rx_stbc, NULL,
                              eANI_BOOLEAN_FALSE);

        if (status == eHAL_STATUS_FAILURE) {
            VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                      "%s: could not set the VHT RX STBC to CCM",
                      __func__);
        }
    }

    /* Get VHT SU Beamformer cap */
    status = ccmCfgGetInt(hdd_ctx->hHal, WNI_CFG_VHT_SU_BEAMFORMER_CAP,
                          &value);

    if (status != eHAL_STATUS_SUCCESS) {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                  "%s: could not get VHT SU BEAMFORMER CAP",
                  __func__);
        value = 0;
    }

    /* set VHT SU Beamformer cap */
    if (value && !cfg->vht_su_bformer) {
        status = ccmCfgSetInt(hdd_ctx->hHal,
                              WNI_CFG_VHT_SU_BEAMFORMER_CAP,
                              cfg->vht_su_bformer, NULL,
                              eANI_BOOLEAN_FALSE);

        if (status == eHAL_STATUS_FAILURE) {
            VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                      "%s: could not set VHT SU BEAMFORMER CAP",
                      __func__);
        }
    }

    /* check and update SU BEAMFORMEE capability*/
    if (pconfig->enableTxBF && !cfg->vht_su_bformee)
        pconfig->enableTxBF = cfg->vht_su_bformee;

    status = ccmCfgSetInt(hdd_ctx->hHal,
                          WNI_CFG_VHT_SU_BEAMFORMEE_CAP,
                          pconfig->enableTxBF, NULL,
                          eANI_BOOLEAN_FALSE);

    if (status == eHAL_STATUS_FAILURE) {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                  "%s: could not set VHT SU BEAMFORMEE CAP",
                  __func__);
    }

    /* Get VHT MU Beamformer cap */
    status = ccmCfgGetInt(hdd_ctx->hHal, WNI_CFG_VHT_MU_BEAMFORMER_CAP,
                          &value);

    if (status != eHAL_STATUS_SUCCESS) {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                  "%s: could not get VHT MU BEAMFORMER CAP",
                  __func__);
        value = 0;
    }

    /* set VHT MU Beamformer cap */
    if (value && !cfg->vht_mu_bformer) {
        status = ccmCfgSetInt(hdd_ctx->hHal,
                              WNI_CFG_VHT_MU_BEAMFORMER_CAP,
                              cfg->vht_mu_bformer, NULL,
                              eANI_BOOLEAN_FALSE);

        if (status == eHAL_STATUS_FAILURE) {
            VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                      "%s: could not set the VHT MU BEAMFORMER CAP to CCM",
                      __func__);
        }
    }

    /* Get VHT MU Beamformee cap */
    status = ccmCfgGetInt(hdd_ctx->hHal, WNI_CFG_VHT_MU_BEAMFORMEE_CAP,
                          &value);

    if (status != eHAL_STATUS_SUCCESS) {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                  "%s: could not get VHT MU BEAMFORMEE CAP",
                  __func__);
        value = 0;
    }

    /* check and update MU BEAMFORMEE capability*/
    if (pconfig->enableMuBformee && !cfg->vht_mu_bformee)
        pconfig->enableMuBformee = cfg->vht_mu_bformee;

    /* set VHT MU Beamformee cap */
    if (value && !cfg->vht_mu_bformee) {
        status = ccmCfgSetInt(hdd_ctx->hHal,
                              WNI_CFG_VHT_MU_BEAMFORMEE_CAP,
                              pconfig->enableMuBformee, NULL,
                              eANI_BOOLEAN_FALSE);

        if (status == eHAL_STATUS_FAILURE) {
            VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                      "%s: could not set VHT MU BEAMFORMER CAP",
                      __func__);
        }
    }

    /* Get VHT MAX AMPDU Len exp */
    status = ccmCfgGetInt(hdd_ctx->hHal, WNI_CFG_VHT_AMPDU_LEN_EXPONENT,
                          &value);

    if (status != eHAL_STATUS_SUCCESS) {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                  "%s: could not get VHT AMPDU LEN",
                  __func__);
        value = 0;
    }

    /*
     * VHT max AMPDU len exp:
     * override if user configured value is too high
     * that the target cannot support.
     * Even though Rome publish ampdu_len=7, it can
     * only support 4 because of some h/w bug.
     */

    if (value > cfg->vht_max_ampdu_len_exp) {
        status = ccmCfgSetInt(hdd_ctx->hHal,
                              WNI_CFG_VHT_AMPDU_LEN_EXPONENT,
                              cfg->vht_max_ampdu_len_exp, NULL,
                              eANI_BOOLEAN_FALSE);

        if (status == eHAL_STATUS_FAILURE) {
            VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                      "%s: could not set the VHT AMPDU LEN EXP",
                      __func__);
        }
    }

    /* Get VHT TXOP PS CAP */
    status = ccmCfgGetInt(hdd_ctx->hHal, WNI_CFG_VHT_TXOP_PS, &value);

    if (status != eHAL_STATUS_SUCCESS) {
        VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                  "%s: could not get VHT TXOP PS",
                  __func__);
        value = 0;
    }

    /* set VHT TXOP PS cap */
    if (value && !cfg->vht_txop_ps) {
        status = ccmCfgSetInt(hdd_ctx->hHal, WNI_CFG_VHT_TXOP_PS,
                              cfg->vht_txop_ps, NULL,
                              eANI_BOOLEAN_FALSE);

        if (status == eHAL_STATUS_FAILURE) {
            VOS_TRACE(VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
                      "%s: could not set the VHT TXOP PS",
                      __func__);
        }
    }

    hddLog(LOG1, "enable2x2 %d ", pconfig->enable2x2);
    if (pconfig->enable2x2)
    {
        if (ccmCfgSetInt(hdd_ctx->hHal,
                         WNI_CFG_VHT_NUM_SOUNDING_DIMENSIONS,
                         NUM_OF_SOUNDING_DIMENSIONS, NULL,
                         eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE) {
            hddLog(LOGE,
                   "Could not set WNI_CFG_VHT_NUM_SOUNDING_DIMENSIONS to CCM");
        }
    }
}
#endif  /* #ifdef WLAN_FEATURE_11AC */

#ifdef FEATURE_WLAN_RA_FILTERING
static void hdd_update_ra_rate_limit(hdd_context_t *hdd_ctx,
				     struct hdd_tgt_cfg *cfg)
{
    hdd_ctx->cfg_ini->IsRArateLimitEnabled = cfg->is_ra_rate_limit_enabled;
}
#else
static void hdd_update_ra_rate_limit(hdd_context_t *hdd_ctx,
				     struct hdd_tgt_cfg *cfg)
{
}
#endif

void hdd_update_tgt_cfg(void *context, void *param)
{
    hdd_context_t *hdd_ctx = (hdd_context_t *)context;
    struct hdd_tgt_cfg *cfg = (struct hdd_tgt_cfg *)param;
    tANI_U8 temp_band_cap;

    if (hdd_cfg_is_sub20_channel_width_enabled(hdd_ctx) &&
        cfg->sub_20_support == 0) {
            hddLog(VOS_TRACE_LEVEL_WARN,
                   FL("requests 5/10M, target not support"));
            hdd_ctx->cfg_ini->sub_20_channel_width = 0;
    }

    /* first store the INI band capability */
    temp_band_cap = hdd_ctx->cfg_ini->nBandCapability;

    hdd_ctx->cfg_ini->nBandCapability = cfg->band_cap;

    /* now overwrite the target band capability with INI
       setting if INI setting is a subset */

    if ((hdd_ctx->cfg_ini->nBandCapability == eCSR_BAND_ALL) &&
        (temp_band_cap != eCSR_BAND_ALL))
        hdd_ctx->cfg_ini->nBandCapability = temp_band_cap;
    else if ((hdd_ctx->cfg_ini->nBandCapability != eCSR_BAND_ALL) &&
             (temp_band_cap != eCSR_BAND_ALL) &&
             (hdd_ctx->cfg_ini->nBandCapability != temp_band_cap)) {
        hddLog(VOS_TRACE_LEVEL_WARN,
               FL("ini BandCapability not supported by the target"));
    }

    if (!hdd_ctx->isLogpInProgress) {
        hdd_ctx->reg.reg_domain = cfg->reg_domain;
        hdd_ctx->reg.eeprom_rd_ext = cfg->eeprom_rd_ext;
    }

    /* This can be extended to other configurations like ht, vht cap... */

    if (!vos_is_macaddr_zero(&cfg->hw_macaddr))
    {
        vos_mem_copy(&hdd_ctx->hw_macaddr, &cfg->hw_macaddr,
                     VOS_MAC_ADDR_SIZE);
        hddLog(VOS_TRACE_LEVEL_INFO,
               FL("hw update mac addr"));
    }
    else {
        hddLog(VOS_TRACE_LEVEL_ERROR,
               "%s: HW MAC is zero", __func__);
    }

    hdd_ctx->target_fw_version = cfg->target_fw_version;

    hdd_ctx->max_intf_count = cfg->max_intf_count;

#ifdef WLAN_FEATURE_LPSS
    hdd_ctx->lpss_support = cfg->lpss_support;
#endif

    wlan_hdd_set_egap_support(hdd_ctx, cfg);

    hdd_ctx->ap_arpns_support = cfg->ap_arpns_support;
    hdd_update_tgt_services(hdd_ctx, &cfg->services);
    if (hdd_ctx->per_band_chainmask_supp)
        hdd_update_chain_mask_vdev_nss(hdd_ctx, &cfg->services);
    else
        sme_set_vdev_nss(hdd_ctx->hHal, hdd_ctx->cfg_ini->enable2x2);

    hdd_update_tgt_ht_cap(hdd_ctx, &cfg->ht_cap);

#ifdef WLAN_FEATURE_11AC
    hdd_update_tgt_vht_cap(hdd_ctx, &cfg->vht_cap);
#endif  /* #ifdef WLAN_FEATURE_11AC */

    hdd_ctx->current_antenna_mode =
            hdd_is_supported_chain_mask_2x2(hdd_ctx) ?
            HDD_ANTENNA_MODE_2X2 : HDD_ANTENNA_MODE_1X1;
    hddLog(LOG1, FL("Current antenna mode: %d"),
           hdd_ctx->current_antenna_mode);
    hdd_ctx->cfg_ini->fine_time_meas_cap &= cfg->fine_time_measurement_cap;
    hdd_ctx->fine_time_meas_cap_target = cfg->fine_time_measurement_cap;
    hddLog(LOG1, FL("fine_time_measurement_cap: 0x%x"),
             hdd_ctx->cfg_ini->fine_time_meas_cap);

    hddLog(LOG1, FL("Target BPF %d Host BPF %d"),
             cfg->bpf_enabled, hdd_ctx->cfg_ini->bpf_packet_filter_enable);
    hdd_ctx->bpf_enabled = (cfg->bpf_enabled &&
                            hdd_ctx->cfg_ini->bpf_packet_filter_enable);
    hdd_update_ra_rate_limit(hdd_ctx, cfg);

    /*
     * If BPF is enabled, maxWowFilters set to WMA_STA_WOW_DEFAULT_PTRN_MAX
     * because we need atleast WMA_STA_WOW_DEFAULT_PTRN_MAX free slots to
     * configure the STA mode wow pattern.
     */
    if (hdd_ctx->bpf_enabled)
             hdd_ctx->cfg_ini->maxWoWFilters = WMA_STA_WOW_DEFAULT_PTRN_MAX;
    hdd_ctx->wmi_max_len = cfg->wmi_max_len;

    /* Configure NAN datapath features */
    hdd_nan_datapath_target_config(hdd_ctx, cfg);

    hdd_ctx->max_mc_addr_list = cfg->max_mc_addr_list;
}

void hdd_update_dfs_cac_block_tx_flag(void *context, bool cac_block_tx)
{
	hdd_context_t *hdd_ctx = (hdd_context_t *)context;
	hdd_adapter_list_node_t *adapter_node = NULL, *next = NULL;
	hdd_adapter_t *adapter;
	VOS_STATUS status;

	if (wlan_hdd_validate_context(hdd_ctx))
		return;
	if (hdd_ctx->cfg_ini->disableDFSChSwitch)
		return;
	status = hdd_get_front_adapter(hdd_ctx, &adapter_node);
	while (NULL != adapter_node && VOS_STATUS_SUCCESS == status) {
		adapter = adapter_node->pAdapter;
		if (WLAN_HDD_SOFTAP == adapter->device_mode ||
				WLAN_HDD_P2P_GO == adapter->device_mode)
			WLAN_HDD_GET_AP_CTX_PTR(adapter)->dfs_cac_block_tx =
					cac_block_tx;

		status = hdd_get_next_adapter(hdd_ctx, adapter_node, &next);
		adapter_node = next;
	}
}

/* This function is invoked in atomic context when a radar
 * is found on the SAP current operating channel and Data
 * Tx from netif has to be stopped to honor the DFS regulations.
 * Actions: Stop the netif Tx queues,Indicate Radar present
 * in HDD context for future usage.
 */
bool hdd_dfs_indicate_radar(void *context, void *param)
{
    hdd_context_t *pHddCtx= (hdd_context_t *)context;
    struct hdd_dfs_radar_ind *hdd_radar_event =
                              (struct hdd_dfs_radar_ind*)param;
    hdd_adapter_list_node_t *pAdapterNode = NULL,
                            *pNext = NULL;
    hdd_adapter_t *pAdapter;
    VOS_STATUS status;

    if (!pHddCtx || !hdd_radar_event || pHddCtx->cfg_ini->disableDFSChSwitch)
        return true;

    if (VOS_TRUE == hdd_radar_event->dfs_radar_status)
    {
        adf_os_spin_lock_bh(&pHddCtx->dfs_lock);
        if (pHddCtx->dfs_radar_found)
        {
            /* Application already triggered channel switch
             * on current channel, so return here
             */
            adf_os_spin_unlock_bh(&pHddCtx->dfs_lock);
            return false;
        }

        pHddCtx->dfs_radar_found = VOS_TRUE;
        adf_os_spin_unlock_bh(&pHddCtx->dfs_lock);

        status = hdd_get_front_adapter ( pHddCtx, &pAdapterNode );
        while ( NULL != pAdapterNode && VOS_STATUS_SUCCESS == status )
        {
            pAdapter = pAdapterNode->pAdapter;
            if (WLAN_HDD_SOFTAP == pAdapter->device_mode ||
                WLAN_HDD_P2P_GO == pAdapter->device_mode)
            {
                WLAN_HDD_GET_AP_CTX_PTR(pAdapter)->dfs_cac_block_tx = VOS_TRUE;
            }

            status = hdd_get_next_adapter ( pHddCtx, pAdapterNode, &pNext );
            pAdapterNode = pNext;
        }
    }

    return true;
}

/**---------------------------------------------------------------------------

  \brief hdd_is_valid_mac_address() - Validate MAC address

  This function validates whether the given MAC address is valid or not
  Expected MAC address is of the format XX:XX:XX:XX:XX:XX
  where X is the hexa decimal digit character and separated by ':'
  This algorithm works even if MAC address is not separated by ':'

  This code checks given input string mac contains exactly 12 hexadecimal digits.
  and a separator colon : appears in the input string only after
  an even number of hex digits.

  \param  - pMacAddr pointer to the input MAC address
  \return - 1 for valid and 0 for invalid

  --------------------------------------------------------------------------*/

v_BOOL_t hdd_is_valid_mac_address(const tANI_U8 *pMacAddr)
{
    int xdigit = 0;
    int separator = 0;
    while (*pMacAddr)
    {
        if (isxdigit(*pMacAddr))
        {
            xdigit++;
        }
        else if (':' == *pMacAddr)
        {
            if (0 == xdigit || ((xdigit / 2) - 1) != separator)
                break;

            ++separator;
        }
        else
        {
            /* Invalid MAC found */
            return 0;
        }
        ++pMacAddr;
    }
    return (xdigit == 12 && (separator == 5 || separator == 0));
}

#ifdef WLAN_FEATURE_USB_RECOVERY
extern bool hif_usb_check(void);
extern void hif_usb_recovery(void);

void hdd_usb_detect_work(struct work_struct *work)
{
    int status = 0;
    hdd_adapter_t *adapter = NULL;
    hdd_context_t *hdd_ctx = container_of(work,
                                      hdd_context_t,
                                      usb_detect_work.work);

    if (!hdd_ctx)
	return;

    mutex_lock(&hdd_ctx->usb_recovery_lock);

    if (! hif_usb_wlan_en_check() ) {
        hddLog(LOGE, FL("WLAN_EN PIN disabled by system, disable usb auto recovery now...\n"));
        mutex_unlock(&hdd_ctx->usb_recovery_lock);
        return;
    }

    if(!hif_usb_check()) {
        status = -1;
        goto usb_error;
    }

    if (wlan_hdd_validate_context(hdd_ctx) != 0) {
        hddLog(LOGE, FL("ctx not ready for recovery"));
        mutex_unlock(&hdd_ctx->usb_recovery_lock);
        return;
    }
    //printk(KERN_ERR "jiffies:%ld, usb detect work ...\n", jiffies);
    adapter = hdd_get_adapter_by_vdev(hdd_ctx, 0);
    if (!adapter) {
        hddLog(LOGE, FL("adapter is NULL"));
        mutex_unlock(&hdd_ctx->usb_recovery_lock);
        return;
    }

    if(wlan_hdd_get_fw_state(adapter)) {
        hddLog(VOS_TRACE_LEVEL_INFO, "%s: wlan fw alive...", __func__);
        schedule_delayed_work(&hdd_ctx->usb_detect_work, msecs_to_jiffies(3000));
        mutex_unlock(&hdd_ctx->usb_recovery_lock);
        return;
    }
usb_error:
    hddLog(LOGE, "usb auto recovery ..., status:%d\n",status);
    usb_recovery_status = 1;
    mutex_unlock(&hdd_ctx->usb_recovery_lock);
    hif_usb_recovery();
}
#endif

/**
 * __hdd_open() - HDD Open function
 * @dev: pointer to net_device structure
 *
 * This is called in response to ifconfig up
 *
 * Return: 0 for success and error number for failure
 */
static int __hdd_open(struct net_device *dev)
{
   hdd_adapter_t *pAdapter = WLAN_HDD_GET_PRIV_PTR(dev);
   hdd_context_t *pHddCtx =  WLAN_HDD_GET_CTX(pAdapter);
   hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
   VOS_STATUS status;
   v_BOOL_t in_standby = TRUE;

   MTRACE(vos_trace(VOS_MODULE_ID_HDD, TRACE_CODE_HDD_OPEN_REQUEST,
                    pAdapter->sessionId, pAdapter->device_mode));

   /* Don't validate for load/unload and logp as if we return
      failure we may endup in scan/connection related issues */
   if (NULL == pHddCtx || NULL == pHddCtx->cfg_ini) {
       hddLog(LOG1, FL("HDD context is Null"));
       return -ENODEV;
   }

   pHddCtx->driver_being_stopped = false;

   status = hdd_get_front_adapter (pHddCtx, &pAdapterNode);
   while ((NULL != pAdapterNode) && (VOS_STATUS_SUCCESS == status)) {
      if (test_bit(DEVICE_IFACE_OPENED, &pAdapterNode->pAdapter->event_flags)) {
         hddLog(LOG1, FL("chip already out of standby"));
         in_standby = FALSE;
         break;
      } else {
         status = hdd_get_next_adapter ( pHddCtx, pAdapterNode, &pNext );
         pAdapterNode = pNext;
      }
   }

   if (TRUE == in_standby) {
       if (VOS_STATUS_SUCCESS != wlan_hdd_exit_lowpower(pHddCtx, pAdapter)) {
           hddLog(LOGE, FL("Failed to bring wlan out of power save"));
           return -EINVAL;
       }
   }

   set_bit(DEVICE_IFACE_OPENED, &pAdapter->event_flags);
   if (hdd_connIsConnected(WLAN_HDD_GET_STATION_CTX_PTR(pAdapter))) {
       hddLog(LOG1, FL("Enabling Tx Queues"));
       /* Enable TX queues only when we are connected */
       wlan_hdd_netif_queue_control(pAdapter,
            WLAN_START_ALL_NETIF_QUEUE,
            WLAN_CONTROL_PATH);
   }

   /* Enable carrier and transmit queues for NDI */
   if (WLAN_HDD_IS_NDI(pAdapter)) {
       hddLog(LOG1, FL("Enabling Tx Queues"));
       wlan_hdd_netif_queue_control(pAdapter,
            WLAN_START_ALL_NETIF_QUEUE_N_CARRIER,
            WLAN_CONTROL_PATH);
   }

   return 0;
}

/**
 * hdd_open() - Wrapper function for __hdd_open to protect it from SSR
 * @dev: pointer to net_device structure
 *
 * This is called in response to ifconfig up
 *
 * Return: 0 for success and error number for failure
 */
static int hdd_open(struct net_device *dev)
{
	int ret;

	vos_ssr_protect(__func__);
	ret = __hdd_open(dev);
	vos_ssr_unprotect(__func__);

	return ret;
}

/**
 * __hdd_mon_open() - HDD monitor open
 * @dev: pointer to net_device
 *
 * Return: 0 for success and error number for failure
 */
static int __hdd_mon_open(struct net_device *dev)
{
	hdd_adapter_t *adapter = WLAN_HDD_GET_PRIV_PTR(dev);
	hdd_context_t *hdd_ctx =  WLAN_HDD_GET_CTX(adapter);
	int ret;
	VOS_STATUS vos_status;
	eHalStatus hal_status;
	WLAN_STADescType sta_desc = {0};

	MTRACE(vos_trace(VOS_MODULE_ID_HDD, TRACE_CODE_HDD_OPEN_REQUEST,
			 adapter->sessionId, adapter->device_mode));

	ret = wlan_hdd_validate_context(hdd_ctx);
	if (0 != ret)
		return ret;

	/* peer is created wma_vdev_attach->wma_create_peer */
	vos_status = WLANTL_RegisterSTAClient(hdd_ctx->pvosContext,
					      hdd_mon_rx_packet_cbk,
					      &sta_desc, 0);
	if (VOS_STATUS_SUCCESS != vos_status) {
		hddLog(LOGE,
		       FL("WLANTL_RegisterSTAClient() failed to register. Status= %d [0x%08X]"),
		       vos_status, vos_status);
		goto exit;
	}

	hal_status = sme_create_mon_session(hdd_ctx->hHal,
				     adapter->macAddressCurrent.bytes);
	if (eHAL_STATUS_SUCCESS != hal_status) {
		hddLog(LOGE,
		       FL("sme_create_mon_session() failed to register. Status= %d [0x%08X]"),
		       hal_status, hal_status);
		goto exit;
	}
	return ret;
exit:
	return -EIO;
}

/**
 * hdd_mon_open() - SSR wrapper function for __hdd_mon_open
 * @dev: pointer to net_device
 *
 * Return: 0 for success and error number for failure
 */
static int hdd_mon_open(struct net_device *dev)
{
	int ret;

	vos_ssr_protect(__func__);
	ret = __hdd_mon_open(dev);
	vos_ssr_unprotect(__func__);

	return ret;
}

int wlan_hdd_monitor_mode_enable(hdd_context_t *hdd_ctx, bool enable)
{
	int ret = 0;

	ret = process_wma_set_command(
		0,
		GEN_PDEV_MONITOR_MODE,
		enable,
		GEN_CMD);

	if (ret) {
		hddLog(LOGE,
		       FL("send monitor enable cmd fail, ret %d"),
		       ret);
		return ret;
	}

	hdd_ctx->is_mon_enable = enable;

	return ret;
}

/**
 * __hdd_vir_mon_open() - HDD monitor open
 * @dev: pointer to net_device
 *
 * Return: 0 for success and error number for failure
 */
static int __hdd_vir_mon_open(struct net_device *dev)
{
	hdd_adapter_t *adapter = WLAN_HDD_GET_PRIV_PTR(dev);
	hdd_context_t *hdd_ctx =  WLAN_HDD_GET_CTX(adapter);
	hdd_adapter_t *sta_adapter = NULL;
	int ret;
	VOS_STATUS vos_status;

	if (!hdd_ctx->cfg_ini->mon_on_sta_enable) {
		hddLog(LOGE,
		       FL("monitor feature for STA is not enabled"));
		goto exit;
	}

	MTRACE(vos_trace(VOS_MODULE_ID_HDD, TRACE_CODE_HDD_OPEN_REQUEST,
			 adapter->sessionId, adapter->device_mode));

	ret = wlan_hdd_validate_context(hdd_ctx);
	if (0 != ret)
		return ret;

	/* register monitor RX callback to TLSHIM and OL */
	vos_status = tl_register_vir_mon_cb(hdd_ctx->pvosContext,
					    hdd_vir_mon_rx_cbk);

	if (VOS_STATUS_SUCCESS != vos_status) {
		hddLog(LOGE,
		       FL("failed to register monitor cbk"));
		goto exit;
	}

	/* get STA adapter to check if STA is connected */
	sta_adapter = hdd_get_adapter(hdd_ctx, WLAN_HDD_INFRA_STATION);

	if ((NULL == sta_adapter) ||
	    (WLAN_HDD_ADAPTER_MAGIC != sta_adapter->magic)) {
		hddLog(LOGE,
		       FL("STA adapter is not existed."));
		return ret;
	}

	if (hdd_connIsConnected(WLAN_HDD_GET_STATION_CTX_PTR(sta_adapter)) &&
	    (false == hdd_ctx->is_mon_enable)) {
		/* send WMI cmd to enable monitor function */
		ret = wlan_hdd_monitor_mode_enable(hdd_ctx, true);

		/* disable Sta BMPS */
		if (hdd_ctx->cfg_ini->enablePowersaveOffload)
			sme_PsOffloadDisablePowerSave(
				WLAN_HDD_GET_HAL_CTX(sta_adapter),
				NULL,
				NULL,
				sta_adapter->sessionId);

#if defined(FEATURE_WLAN_LFR) && defined(WLAN_FEATURE_ROAM_SCAN_OFFLOAD)
		if (hdd_ctx->cfg_ini->isFastRoamIniFeatureEnabled &&
		    hdd_ctx->cfg_ini->isRoamOffloadScanEnabled)
			sme_stopRoaming(WLAN_HDD_GET_HAL_CTX(sta_adapter),
					sta_adapter->sessionId, 0);
#endif
	}

	return ret;
exit:
	return -EIO;
}

/**
 * __hdd_vir_mon_stop() - HDD monitor stop
 * @dev: pointer to net_device
 *
 * Return: 0 for success and error number for failure
 */
static int __hdd_vir_mon_stop(struct net_device *dev)
{
	hdd_adapter_t *adapter = WLAN_HDD_GET_PRIV_PTR(dev);
	hdd_context_t *hdd_ctx =  WLAN_HDD_GET_CTX(adapter);
	hdd_adapter_t *sta_adapter = NULL;
	int ret;
	VOS_STATUS vos_status = VOS_STATUS_SUCCESS;

	if (!hdd_ctx->cfg_ini->mon_on_sta_enable) {
		hddLog(LOGE,
		       FL("monitor feature for STA is not enabled"));
		goto exit;
	}

	MTRACE(vos_trace(VOS_MODULE_ID_HDD, TRACE_CODE_HDD_OPEN_REQUEST,
			 adapter->sessionId, adapter->device_mode));

	ret = wlan_hdd_validate_context(hdd_ctx);
	if (0 != ret)
		return ret;

	/* if monitor is enabled already,
	 * send WMI cmd to disable target monitor.
	 */
	if (true == hdd_ctx->is_mon_enable) {
		ret = wlan_hdd_monitor_mode_enable(hdd_ctx, false);

		/* get STA adapter to check if STA is connected */
		sta_adapter = hdd_get_adapter(hdd_ctx, WLAN_HDD_INFRA_STATION);

	    if (sta_adapter &&
		(WLAN_HDD_ADAPTER_MAGIC == sta_adapter->magic) &&
		hdd_connIsConnected(
				WLAN_HDD_GET_STATION_CTX_PTR(sta_adapter))) {
		/* enable BMPS power save */
		if (hdd_ctx->cfg_ini->enablePowersaveOffload)
			sme_PsOffloadEnablePowerSave(
					WLAN_HDD_GET_HAL_CTX(sta_adapter),
					sta_adapter->sessionId);

#if defined(FEATURE_WLAN_LFR) && defined(WLAN_FEATURE_ROAM_SCAN_OFFLOAD)
		/* enable roaming */
		if (hdd_ctx->cfg_ini->isFastRoamIniFeatureEnabled &&
		    hdd_ctx->cfg_ini->isRoamOffloadScanEnabled)
			sme_startRoaming(WLAN_HDD_GET_HAL_CTX(sta_adapter),
					 sta_adapter->sessionId,
					 REASON_CONNECT);
#endif
		}
	}

	/* Deregister monitor RX callback to TLSHIM and OL */
	vos_status = tl_deregister_vir_mon_cb(hdd_ctx->pvosContext);

	if (VOS_STATUS_SUCCESS != vos_status) {
		hddLog(LOGE,
		       FL("failed to deregister monitor cbk"));
		goto exit;
	}

	return ret;
exit:
	return -EIO;
}

/**
 * hdd_vir_mon_open()
 * @dev: pointer to net_device
 *
 * Return: 0 for success and error number for failure
 */
static int hdd_vir_mon_open(struct net_device *dev)
{
	int ret = 0;

	vos_ssr_protect(__func__);
	__hdd_vir_mon_open(dev);
	vos_ssr_unprotect(__func__);

	return ret;
}

/**
 * hdd_vir_mon_stop()
 * @dev: pointer to net_device
 *
 * Return: 0 for success and error number for failure
 */
static int hdd_vir_mon_stop(struct net_device *dev)
{
	int ret = 0;

	vos_ssr_protect(__func__);
	__hdd_vir_mon_stop(dev);
	vos_ssr_unprotect(__func__);

	return ret;
}

#ifdef MODULE
/**
 * wlan_hdd_stop_enter_lowpower() - Enter low power mode
 * @hdd_ctx:	HDD context
 *
 * For module, when all the interfaces are down, enter low power mode.
 */
void wlan_hdd_stop_enter_lowpower(hdd_context_t *hdd_ctx)
{
	hddLog(VOS_TRACE_LEVEL_INFO,
			"%s: All Interfaces are Down entering standby",
			__func__);
	if (VOS_STATUS_SUCCESS != wlan_hdd_enter_lowpower(hdd_ctx)) {
		hddLog(VOS_TRACE_LEVEL_ERROR,
				"%s: Failed to put wlan in power save",
				__func__);
	}
}

/**
 * wlan_hdd_stop_can_enter_lowpower() - Enter low power mode
 * @adapter:	Adapter context
 *
 * Check if hardware can enter low power mode when all the interfaces are down.
 */
static inline int wlan_hdd_stop_can_enter_lowpower(hdd_adapter_t *adapter)
{
	/* SoftAP ifaces should never go in power save mode making
	 * sure same here.
	 */
	if ((WLAN_HDD_SOFTAP == adapter->device_mode) ||
			(WLAN_HDD_MONITOR == adapter->device_mode) ||
			(WLAN_HDD_P2P_GO == adapter->device_mode))
		return 0;

	return 1;
}
#else

/**
 * kickstart_driver_handler() - Work queue handler for kickstart_driver
 *
 * Use worker queue to exit if it is not possible to call kickstart_driver()
 * directly in the caller context like in interface down context
 */
static void kickstart_driver_handler(struct work_struct *work)
{
	hdd_driver_exit();
	wlan_hdd_inited = 0;
}

static DECLARE_WORK(kickstart_driver_work, kickstart_driver_handler);

/**
 * kickstart_driver() - Initialize and Clean-up driver
 * @load:	True: initialize, False: Clean-up driver
 * @mode_change: tell if last mode and current mode is same or not
 *
 * Delayed driver initialization when driver is statically linked and Clean-up
 * when all the interfaces are down or any other condition which requires to
 * save power by bringing down hardware.
 * This routine is invoked when module parameter fwpath is modified from user
 * space to signal the initialization of the WLAN driver or when all the
 * interfaces are down and user space no longer need WLAN interfaces. Userspace
 * needs to write to fwpath again to get the WLAN interfaces
 *
 * Return: 0 on success, non zero on failure
 */
static int kickstart_driver(bool load, bool mode_change)
{
	int ret_status;

	pr_info("%s: load: %d wlan_hdd_inited: %d, mode_change: %d caller: %pf\n",
			 __func__, load, wlan_hdd_inited,
			mode_change, (void *)_RET_IP_);

	/* Make sure unload and load are synchronized */
	flush_work(&kickstart_driver_work);

	/* No-Op, If unload requested even though driver is not loaded */
	if (!load && !wlan_hdd_inited)
		return 0;

	/* Unload is requested */
	if (!load && wlan_hdd_inited) {
		schedule_work(&kickstart_driver_work);
		return 0;
	}

	if (!wlan_hdd_inited) {
		ret_status = hdd_driver_init();
		wlan_hdd_inited = ret_status ? 0 : 1;
		return ret_status;
	}

	if (load && wlan_hdd_inited && !mode_change) {
		ret_status = 0;
	} else {
		hdd_driver_exit();

		msleep(200);

		ret_status = hdd_driver_init();
		wlan_hdd_inited = ret_status ? 0 : 1;
	}

	return ret_status;
}

/**
 * wlan_hdd_stop_enter_lowpower() - Enter low power mode
 * @hdd_ctx:	HDD context
 *
 * For static driver, when all the interfaces are down, enter low power mode by
 * bringing down WLAN hardware.
 */
void wlan_hdd_stop_enter_lowpower(hdd_context_t *hdd_ctx)
{
	bool ready;

	/* Do not clean up n/w ifaces if we are in DRIVER STOP phase or else
	 * DRIVER START will fail and Wi-Fi will not resume successfully
	 */
	if (hdd_ctx && !hdd_ctx->driver_being_stopped) {
		ready = vos_is_load_unload_ready(__func__);
		if (!ready) {
			VOS_ASSERT(0);
			return;
		}

		vos_load_unload_protect(__func__);
		kickstart_driver(false, false);
		vos_load_unload_unprotect(__func__);
	}
}

/**
 * wlan_hdd_stop_can_enter_lowpower() - Enter low power mode
 * @adapter:	Adapter context
 *
 * Check if hardware can enter low power mode when all the interfaces are down.
 * For static driver, hardware can enter low power mode for all types of
 * interfaces.
 *
 * Return: true for power save allowed and false for power save not allowed
 */
static inline bool wlan_hdd_stop_can_enter_lowpower(hdd_adapter_t *adapter)
{
	hdd_context_t *hdd_ctx =  WLAN_HDD_GET_CTX(adapter);

	/* In static driver case, we need to distinguish between WiFi OFF and
	 * DRIVER STOP. In both cases "ifconfig down" is happening. In OFF case,
	 * want to allow lowest power mode and driver cleanup. In case of DRIVER
	 * STOP do not want to allow power collapse for GO/SAP case. STOP
	 * behavior is now identical across both DLKM and Static driver case.
	 */
	if (hdd_ctx && !hdd_ctx->driver_being_stopped)
		return true;
	else if ((WLAN_HDD_SOFTAP == adapter->device_mode) ||
		(WLAN_HDD_MONITOR == adapter->device_mode) ||
		(WLAN_HDD_P2P_GO == adapter->device_mode))
		return false;
	else
		return true;
}
#endif

/**
 * __hdd_stop() - HDD stop function
 * @dev: pointer to net_device structure
 *
 * This is called in response to ifconfig down
 *
 * Return: 0 for success and error number for failure
 */
static int __hdd_stop(struct net_device *dev)
{
   hdd_adapter_t *pAdapter = WLAN_HDD_GET_PRIV_PTR(dev);
   hdd_context_t *pHddCtx =  WLAN_HDD_GET_CTX(pAdapter);
   hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
   VOS_STATUS status;
   v_BOOL_t enter_standby = TRUE;
   int ret;

   ENTER();

   MTRACE(vos_trace(VOS_MODULE_ID_HDD, TRACE_CODE_HDD_STOP_REQUEST,
                    pAdapter->sessionId, pAdapter->device_mode));

   ret = wlan_hdd_validate_context(pHddCtx);
   if (0 != ret)
       return ret;

   /* Nothing to be done if the interface is not opened */
   if (VOS_FALSE == test_bit(DEVICE_IFACE_OPENED, &pAdapter->event_flags))
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "%s: NETDEV Interface is not OPENED", __func__);
      return -ENODEV;
   }

   /* Make sure the interface is marked as closed */
   clear_bit(DEVICE_IFACE_OPENED, &pAdapter->event_flags);
   hddLog(VOS_TRACE_LEVEL_INFO, "%s: Disabling OS Tx queues", __func__);

   /* Disable TX on the interface, after this hard_start_xmit() will not
    * be called on that interface
    */
   hddLog(LOG1, FL("Disabling queues"));
   wlan_hdd_netif_queue_control(pAdapter, WLAN_NETIF_TX_DISABLE_N_CARRIER,
                         WLAN_CONTROL_PATH);

   /*
    * NAN data interface is different in some sense. The traffic on NDI is
    * bursty in nature and depends on the need to transfer. The service layer
    * may down the interface after the usage and up again when required.
    * In some sense, the NDI is expected to be available (like SAP) iface
    * until NDI delete request is issued by the service layer.
    * Skip BSS termination and adapter deletion for NAN Data interface (NDI).
    */
    if (WLAN_HDD_IS_NDI(pAdapter))
       return 0;

   /* The interface is marked as down for outside world (aka kernel)
    * But the driver is pretty much alive inside. The driver needs to
    * tear down the existing connection on the netdev (session)
    * cleanup the data pipes and wait until the control plane is stabilized
    * for this interface. The call also needs to wait until the above
    * mentioned actions are completed before returning to the caller.
    * Notice that the hdd_stop_adapter is requested not to close the session
    * That is intentional to be able to scan if it is a STA/P2P interface
    */
   hdd_stop_adapter(pHddCtx, pAdapter, VOS_FALSE);

   /* DeInit the adapter. This ensures datapath cleanup as well */
   hdd_deinit_adapter(pHddCtx, pAdapter, true);

   /* SoftAP ifaces should never go in power save mode
      making sure same here. */
   if (!wlan_hdd_stop_can_enter_lowpower(pAdapter))
   {
      /* SoftAP mode, so return from here */
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                "%s: In SAP MODE", __func__);
      EXIT();
      return 0;
   }

   /* Find if any iface is up. If any iface is up then can't put device to
    * sleep/power save mode
    */
   status = hdd_get_front_adapter ( pHddCtx, &pAdapterNode );
   while ( (NULL != pAdapterNode) && (VOS_STATUS_SUCCESS == status) )
   {
      if (test_bit(DEVICE_IFACE_OPENED, &pAdapterNode->pAdapter->event_flags))
      {
         hddLog(VOS_TRACE_LEVEL_INFO, "%s: Still other ifaces are up cannot "
                "put device to sleep", __func__);
         enter_standby = FALSE;
         break;
      }
      else
      {
         status = hdd_get_next_adapter ( pHddCtx, pAdapterNode, &pNext );
         pAdapterNode = pNext;
      }
   }

   if (TRUE == enter_standby)
      wlan_hdd_stop_enter_lowpower(pHddCtx);

   EXIT();
   return 0;
}

/**
 * hdd_stop() - Wrapper function for __hdd_stop to protect it from SSR
 * @dev: pointer to net_device structure
 *
 * This is called in response to ifconfig down
 *
 * Return: 0 for success and error number for failure
 */
static int hdd_stop (struct net_device *dev)
{
	int ret;

	vos_ssr_protect(__func__);
	ret = __hdd_stop(dev);
	vos_ssr_unprotect(__func__);

	return ret;
}

/**
 * __hdd_uninit() - HDD uninit function
 * @dev: pointer to net_device structure
 *
 * This is called during the netdev unregister to uninitialize all data
 * associated with the device
 *
 * Return: none
 */
static void __hdd_uninit(struct net_device *dev)
{
	hdd_adapter_t *pAdapter = WLAN_HDD_GET_PRIV_PTR(dev);

	ENTER();

	if (WLAN_HDD_ADAPTER_MAGIC != pAdapter->magic) {
		hddLog(LOGP, FL("Invalid magic"));
		return;
	}

	if (NULL == pAdapter->pHddCtx) {
		hddLog(LOGP, FL("NULL pHddCtx"));
		return;
	}

	if (dev != pAdapter->dev)
		hddLog(LOGP, FL("Invalid device reference"));

	hdd_deinit_adapter(pAdapter->pHddCtx, pAdapter, true);

	/* After uninit our adapter structure will no longer be valid */
	pAdapter->dev = NULL;
	pAdapter->magic = 0;
	pAdapter->pHddCtx = NULL;

	EXIT();
}

/**
 * hdd_uninit() - Wrapper function to protect __hdd_uninit from SSR
 * @dev: pointer to net_device structure
 *
 * This is called during the netdev unregister to uninitialize all data
 * associated with the device
 *
 * Return: none
 */
static void hdd_uninit(struct net_device *dev)
{
	vos_ssr_protect(__func__);
	__hdd_uninit(dev);
	vos_ssr_unprotect(__func__);
}


/**---------------------------------------------------------------------------
     \brief hdd_full_pwr_cbk() - HDD full power callback function

      This is the function invoked by SME to inform the result of a full power
      request issued by HDD

     \param  - callback context - Pointer to cookie
               status - result of request

     \return - None

--------------------------------------------------------------------------*/
void hdd_full_pwr_cbk(void *callbackContext, eHalStatus status)
{
   hdd_context_t *pHddCtx = (hdd_context_t*)callbackContext;

   hddLog(VOS_TRACE_LEVEL_INFO_HIGH,"HDD full Power callback status = %d", status);
   if(pHddCtx)
   {
      complete(&pHddCtx->full_pwr_comp_var);
   }
}

static void hdd_tx_fail_ind_callback(v_U8_t *MacAddr, v_U8_t seqNo)
{
   int payload_len;
   struct sk_buff *skb;
   struct nlmsghdr *nlh;
   v_U8_t *data;

   payload_len = ETH_ALEN;

   if (0 == cesium_pid || cesium_nl_srv_sock == NULL) {
      hddLog(LOGE, FL("cesium process not registered, pid: %d, nl_sock: %pK"),
             cesium_pid, cesium_nl_srv_sock);
      return;
   }

   if ((skb = nlmsg_new(payload_len,GFP_ATOMIC)) == NULL)
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,
             "%s: nlmsg_new() failed for msg size[%d]",
             __func__, NLMSG_SPACE(payload_len));
      return;
   }

   nlh = nlmsg_put(skb, cesium_pid, seqNo, 0, payload_len, NLM_F_REQUEST);

   if (NULL == nlh)
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,
             "%s: nlmsg_put() failed for msg size[%d]",
             __func__, NLMSG_SPACE(payload_len));

      kfree_skb(skb);
      return;
   }

   data = nlmsg_data(nlh);
   memcpy(data, MacAddr, ETH_ALEN);

   if (nlmsg_unicast(cesium_nl_srv_sock, skb, cesium_pid) < 0)
   {
      hddLog(VOS_TRACE_LEVEL_ERROR, "%s: nlmsg_unicast() failed for msg size[%d]",
                                       __func__, NLMSG_SPACE(payload_len));
   }

   return;
}

/**---------------------------------------------------------------------------
     \brief hdd_ParseuserParams - return a pointer to the next argument

     \return - status

--------------------------------------------------------------------------*/
static int hdd_ParseUserParams(tANI_U8 *pValue, tANI_U8 **ppArg)
{
   tANI_U8 *pVal;

   pVal = strchr(pValue, ' ');

   if (NULL == pVal)
   {
      /* no argument remains */
      return -EINVAL;
   }
   else if (SPACE_ASCII_VALUE != *pVal)
   {
      /* no space after the current argument */
      return -EINVAL;
   }

   pVal++;

   /* remove empty spaces */
   while ((SPACE_ASCII_VALUE  == *pVal) && ('\0' !=  *pVal))
   {
      pVal++;
   }

   /* no argument followed by spaces */
   if ('\0' == *pVal)
   {
      return -EINVAL;
   }

   *ppArg = pVal;

   return 0;
}

/**----------------------------------------------------------------------------
     \brief hdd_ParseIBSSTXFailEventParams - Parse params for SETIBSSTXFAILEVENT

     \return - status

------------------------------------------------------------------------------*/
static int hdd_ParseIBSSTXFailEventParams(tANI_U8 *pValue,
                                          tANI_U8 *tx_fail_count,
                                          tANI_U16 *pid)
{
   tANI_U8 *param = NULL;
   int ret;

   ret = hdd_ParseUserParams(pValue, &param);

   if (0 == ret && NULL != param)
   {
      if (1 != sscanf(param, "%hhu", tx_fail_count))
      {
         ret = -EINVAL;
         goto done;
      }
   }
   else
   {
      goto done;
   }

   if (0 == *tx_fail_count)
   {
      *pid = 0;
      goto done;
   }

   pValue = param;
   pValue++;

   ret = hdd_ParseUserParams(pValue, &param);

   if (0 == ret)
   {
      if (1 != sscanf(param, "%hu", pid))
      {
         ret = -EINVAL;
         goto done;
      }
   }
   else
   {
      goto done;
   }

done:
   return ret;
}

static int hdd_open_cesium_nl_sock(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0))
   struct netlink_kernel_cfg cfg = {
          .groups = WLAN_NLINK_MCAST_GRP_ID,
          .input = NULL
          };
#endif
   int ret = 0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0))
   cesium_nl_srv_sock = netlink_kernel_create(&init_net, WLAN_NLINK_CESIUM,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0))
                                              THIS_MODULE,
#endif
                                              &cfg);
#else
   cesium_nl_srv_sock = netlink_kernel_create(&init_net, WLAN_NLINK_CESIUM,
                                        WLAN_NLINK_MCAST_GRP_ID, NULL, NULL, THIS_MODULE);
#endif

   if (cesium_nl_srv_sock == NULL)
   {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "NLINK:  cesium netlink_kernel_create failed");
       ret = -ECONNREFUSED;
   }

   return ret;
}

static void hdd_close_cesium_nl_sock(void)
{
   if (NULL != cesium_nl_srv_sock)
   {
      netlink_kernel_release(cesium_nl_srv_sock);
      cesium_nl_srv_sock = NULL;
   }
}

/**---------------------------------------------------------------------------

  \brief hdd_get_cfg_file_size() -

   This function reads the configuration file using the request firmware
   API and returns the configuration file size.

  \param  - pCtx - Pointer to the adapter .
              - pFileName - Pointer to the file name.
              - pBufSize - Pointer to the buffer size.

  \return - 0 for success, non zero for failure

  --------------------------------------------------------------------------*/

VOS_STATUS hdd_get_cfg_file_size(v_VOID_t *pCtx, char *pFileName, v_SIZE_t *pBufSize)
{
   int status;
   hdd_context_t *pHddCtx = (hdd_context_t*)pCtx;

   ENTER();

   status = qca_request_firmware(&pHddCtx->fw, pFileName, pHddCtx->parent_dev);

   if(status || !pHddCtx->fw || !pHddCtx->fw->data) {
      hddLog(VOS_TRACE_LEVEL_FATAL,"%s: CFG download failed",__func__);
      status = VOS_STATUS_E_FAILURE;
   }
   else {
      *pBufSize = pHddCtx->fw->size;
      hddLog(VOS_TRACE_LEVEL_INFO, "%s: CFG size = %d", __func__, *pBufSize);
      release_firmware(pHddCtx->fw);
      pHddCtx->fw = NULL;
   }

   EXIT();
   return VOS_STATUS_SUCCESS;
}

/**---------------------------------------------------------------------------

  \brief hdd_read_cfg_file() -

   This function reads the configuration file using the request firmware
   API and returns the cfg data and the buffer size of the configuration file.

  \param  - pCtx - Pointer to the adapter .
              - pFileName - Pointer to the file name.
              - pBuffer - Pointer to the data buffer.
              - pBufSize - Pointer to the buffer size.

  \return - 0 for success, non zero for failure

  --------------------------------------------------------------------------*/

VOS_STATUS hdd_read_cfg_file(v_VOID_t *pCtx, char *pFileName,
    v_VOID_t *pBuffer, v_SIZE_t *pBufSize)
{
   int status;
   hdd_context_t *pHddCtx = (hdd_context_t*)pCtx;

   ENTER();

   status = qca_request_firmware(&pHddCtx->fw, pFileName, pHddCtx->parent_dev);

   if(status || !pHddCtx->fw || !pHddCtx->fw->data) {
      hddLog(VOS_TRACE_LEVEL_FATAL,"%s: CFG download failed",__func__);
      return VOS_STATUS_E_FAILURE;
   }
   else {
      if(*pBufSize != pHddCtx->fw->size) {
         hddLog(VOS_TRACE_LEVEL_ERROR, "%s: Caller sets invalid CFG "
             "file size", __func__);
         release_firmware(pHddCtx->fw);
         pHddCtx->fw = NULL;
         return VOS_STATUS_E_FAILURE;
      }
        else {
         if(pBuffer) {
            vos_mem_copy(pBuffer,pHddCtx->fw->data,*pBufSize);
         }
         release_firmware(pHddCtx->fw);
         pHddCtx->fw = NULL;
        }
   }

   EXIT();

   return VOS_STATUS_SUCCESS;
}

/**
 * __hdd_set_mac_address() - HDD set mac address
 * @dev: pointer to net_device structure
 * @addr: Pointer to the sockaddr
 *
 * This function sets the user specified mac address using
 * the command ifconfig wlanX hw ether <mac adress>.
 *
 * Return: 0 for success.
 */
static int __hdd_set_mac_address(struct net_device *dev, void *addr)
{
	hdd_adapter_t *pAdapter = WLAN_HDD_GET_PRIV_PTR(dev);
	hdd_context_t *hdd_ctx;
	struct sockaddr *psta_mac_addr = addr;
	int ret;

	ENTER();

	hdd_ctx = WLAN_HDD_GET_CTX(pAdapter);
	ret = wlan_hdd_validate_context(hdd_ctx);
	if (0 != ret)
		return ret;

	memcpy(&pAdapter->macAddressCurrent, psta_mac_addr->sa_data, ETH_ALEN);
	memcpy(dev->dev_addr, psta_mac_addr->sa_data, ETH_ALEN);

	EXIT();
	return 0;
}

/**
 * hdd_set_mac_address() - Wrapper function to protect __hdd_set_mac_address()
 *			function from SSR
 * @dev: pointer to net_device structure
 * @addr: Pointer to the sockaddr
 *
 * This function sets the user specified mac address using
 * the command ifconfig wlanX hw ether <mac adress>.
 *
 * Return: 0 for success.
 */
static int hdd_set_mac_address(struct net_device *dev, void *addr)
{
	int ret;

	vos_ssr_protect(__func__);
	ret = __hdd_set_mac_address(dev, addr);
	vos_ssr_unprotect(__func__);

	return ret;
}

tANI_U8* wlan_hdd_get_intf_addr(hdd_context_t* pHddCtx)
{
   int i;
   for ( i = 0; i < VOS_MAX_CONCURRENCY_PERSONA; i++)
   {
      if( 0 == ((pHddCtx->cfg_ini->intfAddrMask) & (1 << i)))
         break;
   }

   if( VOS_MAX_CONCURRENCY_PERSONA == i)
      return NULL;

   pHddCtx->cfg_ini->intfAddrMask |= (1 << i);
   return &pHddCtx->cfg_ini->intfMacAddr[i].bytes[0];
}

void wlan_hdd_release_intf_addr(hdd_context_t* pHddCtx, tANI_U8* releaseAddr)
{
   int i;
   for ( i = 0; i < VOS_MAX_CONCURRENCY_PERSONA; i++)
   {
      if ( !memcmp(releaseAddr, &pHddCtx->cfg_ini->intfMacAddr[i].bytes[0], 6) )
      {
         pHddCtx->cfg_ini->intfAddrMask &= ~(1 << i);
         break;
      }
   }
   return;
}

#ifdef WLAN_FEATURE_PACKET_FILTERING
/**
 * __hdd_set_multicast_list() - set the multicast address list
 * @dev: pointer to net_device
 *
 * Return: none
 */
static void __hdd_set_multicast_list(struct net_device *dev)
{
   static const uint8_t ipv6_router_solicitation[] =
                         {0x33, 0x33, 0x00, 0x00, 0x00, 0x02};
   hdd_adapter_t *pAdapter;
   hdd_context_t *pHddCtx;
   int mc_count;
   int i = 0;
   struct netdev_hw_addr *ha;

   ENTER();

   if (VOS_FTM_MODE == hdd_get_conparam())
      return;

   pAdapter = WLAN_HDD_GET_PRIV_PTR(dev);
   pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
   if (0 != wlan_hdd_validate_context(pHddCtx))
      return;

   if (!pHddCtx->cfg_ini->fEnableMCAddrList) {
      hddLog(VOS_TRACE_LEVEL_ERROR, FL("mc addr list ini is disabled"));
      return;
   }

   /* Delete already configured multicast address list */
   if (0 < pAdapter->mc_addr_list.mc_cnt)
      if (wlan_hdd_set_mc_addr_list(pAdapter, false)) {
           hddLog(VOS_TRACE_LEVEL_ERROR, FL("failed to clear mc addr list"));
           return;
      }


   if (dev->flags & IFF_ALLMULTI)
   {
      hddLog(VOS_TRACE_LEVEL_INFO,
            "%s: allow all multicast frames", __func__);
      pAdapter->mc_addr_list.mc_cnt = 0;
   }
   else
   {
      mc_count = netdev_mc_count(dev);
      hddLog(VOS_TRACE_LEVEL_INFO,
            "%s: mc_count : %u, max_mc_addr_list : %d",
             __func__, mc_count, pHddCtx->max_mc_addr_list);

      if (mc_count > pHddCtx->max_mc_addr_list) {
         hddLog(VOS_TRACE_LEVEL_INFO,
                "%s: No free filter available; allow all multicast frames",
                __func__);
         pAdapter->mc_addr_list.mc_cnt = 0;
         return;
      }

      netdev_for_each_mc_addr(ha, dev) {
         hddLog(VOS_TRACE_LEVEL_INFO,
                FL("ha_addr[%d] "MAC_ADDRESS_STR),
                i, MAC_ADDR_ARRAY(ha->addr));

         if (i == mc_count)
            break;
         /*
          * Skip following addresses:
          * 1)IPv6 router solicitation address
          * 2)Any other address pattern if its set during RXFILTER REMOVE
          *   driver command based on addr_filter_pattern
          */
         if ((!memcmp(ha->addr, ipv6_router_solicitation, ETH_ALEN)) ||
             (pAdapter->addr_filter_pattern && (!memcmp(ha->addr,
                                 &pAdapter->addr_filter_pattern, 1)))) {
                hddLog(LOG1, FL("MC/BC filtering Skip addr ="MAC_ADDRESS_STR),
                     MAC_ADDR_ARRAY(ha->addr));

                continue;
         }

         memset(&(pAdapter->mc_addr_list.addr[i * ETH_ALEN]), 0, ETH_ALEN);
         memcpy(&(pAdapter->mc_addr_list.addr[i * ETH_ALEN]),
                ha->addr, ETH_ALEN);
         hddLog(VOS_TRACE_LEVEL_INFO, "%s: mlist[%d] = "MAC_ADDRESS_STR,
               __func__, i,
               MAC_ADDR_ARRAY(&pAdapter->mc_addr_list.addr[i * ETH_ALEN]));
         pAdapter->mc_addr_list.mc_cnt++;
         i++;
      }
   }

   /* Configure the updated multicast address list */
   if (wlan_hdd_set_mc_addr_list(pAdapter, true))
       hddLog(VOS_TRACE_LEVEL_INFO, FL("failed to set mc addr list"));

   EXIT();
   return;
}

/**
 * hdd_set_multicast_list() - SSR wrapper function for __hdd_set_multicast_list
 * @dev: pointer to net_device
 *
 * Return: none
 */
static void hdd_set_multicast_list(struct net_device *dev)
{
	vos_ssr_protect(__func__);
	__hdd_set_multicast_list(dev);
	vos_ssr_unprotect(__func__);
}
#endif

/**---------------------------------------------------------------------------

  \brief hdd_select_queue() -

   This function is registered with the Linux OS for network
   core to decide which queue to use first.

  \param  - dev - Pointer to the WLAN device.
              - skb - Pointer to OS packet (sk_buff).
  \return - ac, Queue Index/access category corresponding to UP in IP header

  --------------------------------------------------------------------------*/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
uint16_t hdd_select_queue(struct net_device *dev, struct sk_buff *skb,
			  struct net_device *sb_dev)
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
uint16_t hdd_select_queue(struct net_device *dev, struct sk_buff *skb,
			  struct net_device *sb_dev,
			  select_queue_fallback_t fallback)
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0))
uint16_t hdd_select_queue(struct net_device *dev, struct sk_buff *skb,
			  void *accel_priv, select_queue_fallback_t fallback)
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
uint16_t hdd_select_queue(struct net_device *dev, struct sk_buff *skb,
			  void *accel_priv)
#else
uint16_t hdd_select_queue(struct net_device *dev, struct sk_buff *skb)
#endif
{
	return hdd_wmm_select_queue(dev, skb);
}

#ifdef WLAN_FEATURE_TSF_PTP
static const struct ethtool_ops wlan_ethtool_ops = {
	.get_ts_info = wlan_get_ts_info,
};
#endif

static struct net_device_ops wlan_drv_ops = {
      .ndo_open = hdd_open,
      .ndo_stop = hdd_stop,
      .ndo_uninit = hdd_uninit,
      .ndo_start_xmit = hdd_hard_start_xmit,
      .ndo_tx_timeout = hdd_tx_timeout,
      .ndo_get_stats = hdd_stats,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
      .ndo_siocdevprivate = hdd_dev_private_ioctl,
#else
      .ndo_do_ioctl = hdd_ioctl,
#endif
      .ndo_set_mac_address = hdd_set_mac_address,
      .ndo_select_queue    = hdd_select_queue,
#ifdef WLAN_FEATURE_PACKET_FILTERING
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,1,0)) || defined(WITH_BACKPORTS)
      .ndo_set_rx_mode = hdd_set_multicast_list,
#else
      .ndo_set_multicast_list = hdd_set_multicast_list,
#endif //LINUX_VERSION_CODE
#endif
 };
 static struct net_device_ops wlan_mon_drv_ops = {
      .ndo_open = hdd_mon_open,
      .ndo_stop = hdd_stop,
#ifdef CONFIG_HL_SUPPORT
      .ndo_start_xmit = hdd_hard_start_xmit,
#endif
      .ndo_get_stats = hdd_stats,
};

static struct net_device_ops wlan_mon_dev_ops = {
	.ndo_open = hdd_vir_mon_open,
	.ndo_stop = hdd_vir_mon_stop,
};

#ifdef WLAN_FEATURE_TSF_PTP
void hdd_set_station_ops( struct net_device *pWlanDev )
{
	if (VOS_MONITOR_MODE == hdd_get_conparam())
		pWlanDev->netdev_ops = &wlan_mon_drv_ops;
	else
		pWlanDev->netdev_ops = &wlan_drv_ops;
	pWlanDev->ethtool_ops = &wlan_ethtool_ops;
}
#else
void hdd_set_station_ops( struct net_device *pWlanDev )
{
	if (VOS_MONITOR_MODE == hdd_get_conparam())
		pWlanDev->netdev_ops = &wlan_mon_drv_ops;
	else
		pWlanDev->netdev_ops = &wlan_drv_ops;
}
#endif

void hdd_set_monitor_ops(struct net_device *pwlan_dev)
{
	pwlan_dev->netdev_ops = &wlan_mon_dev_ops;
}

static void mon_mode_ether_setup(struct net_device *dev)
{
	dev->header_ops         = NULL;
	dev->type               = ARPHRD_IEEE80211_RADIOTAP;
	dev->hard_header_len    = ETH_HLEN;
	dev->mtu                = ETH_DATA_LEN;
	dev->addr_len           = ETH_ALEN;
	dev->tx_queue_len       = 1000; /* Ethernet wants good queues */
	dev->flags              = IFF_BROADCAST|IFF_MULTICAST;
	dev->priv_flags        |= IFF_TX_SKB_SHARING;

	memset(dev->broadcast, 0xFF, ETH_ALEN);
}

#ifdef FEATURE_RUNTIME_PM
/**
 * hdd_runtime_suspend_init() - API to initialize runtime pm context
 * @hdd_ctx: HDD Context
 *
 * The API initializes the context to prevent runtime pm for various use
 * cases like scan, roc, dfs.
 * This API can be extended to initialize the context to prevent runtime pm
 *
 * Return: void
 */
void hdd_runtime_suspend_init(hdd_context_t *hdd_ctx)
{
	struct hdd_runtime_pm_context *context = &hdd_ctx->runtime_context;

	context->scan = vos_runtime_pm_prevent_suspend_init("scan");
	context->roc = vos_runtime_pm_prevent_suspend_init("roc");
	context->dfs = vos_runtime_pm_prevent_suspend_init("dfs");
	context->obss = vos_runtime_pm_prevent_suspend_init("obss");
}

/**
 * hdd_runtime_suspend_deinit() - API to deinit runtime pm context
 * @hdd_ctx: HDD context
 *
 * The API deinit the context to prevent runtime pm.
 *
 * Return: void
 */
void hdd_runtime_suspend_deinit(hdd_context_t *hdd_ctx)
{
	struct hdd_runtime_pm_context *context = &hdd_ctx->runtime_context;

	vos_runtime_pm_prevent_suspend_deinit(context->scan);
	context->scan = NULL;
	vos_runtime_pm_prevent_suspend_deinit(context->roc);
	context->roc = NULL;
	vos_runtime_pm_prevent_suspend_deinit(context->dfs);
	context->dfs = NULL;
	vos_runtime_pm_prevent_suspend_deinit(context->obss);
	context->obss = NULL;
}

/**
 * hdd_adapter_runtime_suspend_init() - API to init runtime pm context/adapter
 * @adapter: Interface Adapter
 *
 * API is used to init the context to prevent runtime pm/adapter
 *
 * Return: void
 */
static void
hdd_adapter_runtime_suspend_init(hdd_adapter_t *adapter)
{
	struct hdd_adapter_pm_context *context = &adapter->runtime_context;

	context->connect = vos_runtime_pm_prevent_suspend_init("connect");
}

/**
 * hdd_adapter_runtime_suspend_denit() - API to deinit runtime pm/adapter
 * @adapter: Interface Adapter
 *
 * API is used to deinit the context to prevent runtime pm/adapter
 *
 * Return: void
 */
static void hdd_adapter_runtime_suspend_denit(hdd_adapter_t *adapter)
{
	struct hdd_adapter_pm_context *context = &adapter->runtime_context;

	vos_runtime_pm_prevent_suspend_deinit(context->connect);
	context->connect = NULL;
}

#else
void hdd_runtime_suspend_init(hdd_context_t *hdd_ctx) { }
void hdd_runtime_suspend_deinit(hdd_context_t *hdd_ctx) { }
static inline void
hdd_adapter_runtime_suspend_init(hdd_adapter_t *adapter) { }
static inline void
hdd_adapter_runtime_suspend_denit(hdd_adapter_t *adapter) { }
#endif

/**
 * hdd_adapter_init_action_frame_random_mac() - Initialze attributes needed for
 * randomization of SA in management action frames
 * @adapter: Pointer to adapter
 *
 * Return: None
 */
static void hdd_adapter_init_action_frame_random_mac(hdd_adapter_t *adapter)
{
	adf_os_spinlock_init(&adapter->random_mac_lock);
	vos_mem_zero(adapter->random_mac, sizeof(adapter->random_mac));
}

#ifdef SUPPORT_IFTYPE_P2P_DEVICE_VIF
static hdd_adapter_t*
hdd_alloc_p2p_device_adapter(hdd_context_t *pHddCtx, tSirMacAddr macAddr,
			     unsigned char name_assign_type, const char* name)
{
	hdd_adapter_t *pAdapter = NULL;
	/*
	* cfg80211 initialization and registration....
	*/
	pAdapter = vos_mem_malloc(sizeof(hdd_adapter_t));
	hddLog(VOS_TRACE_LEVEL_DEBUG, "%s:pAdapter=0x%lx, MacAddr="MAC_ADDRESS_STR,
	        __func__, (unsigned long)pAdapter, MAC_ADDR_ARRAY(macAddr));

	vos_mem_zero(pAdapter, sizeof(hdd_adapter_t));

	pAdapter->pHddCtx = pHddCtx;
	pAdapter->magic = WLAN_HDD_ADAPTER_MAGIC;
	vos_event_init(&pAdapter->scan_info.scan_finished_event);
	pAdapter->scan_info.scan_pending_option = WEXT_SCAN_PENDING_GIVEUP;
	pAdapter->offloads_configured = FALSE;
	pAdapter->isLinkUpSvcNeeded = FALSE;
	pAdapter->higherDtimTransition = eANI_BOOLEAN_TRUE;
	vos_mem_copy(pAdapter->macAddressCurrent.bytes, macAddr,
		     sizeof(tSirMacAddr));
	vos_mem_copy(pAdapter->wdev.address, macAddr, sizeof(tSirMacAddr));

	strlcpy(pAdapter->ifname, name, IFNAMSIZ);
	/*
	* kernel will consume ethernet header length buffer for hard_header,
	* so just reserve it
	*/
	pAdapter->wdev.wiphy = pHddCtx->wiphy;
	/* set pWlanDev's parent to underlying device */
	hdd_wmm_init(pAdapter);
	hdd_adapter_runtime_suspend_init(pAdapter);
	adf_os_spinlock_init(&pAdapter->pause_map_lock);
	pAdapter->last_tx_jiffies = jiffies;
	pAdapter->bug_report_count = 0;
	pAdapter->start_time = pAdapter->last_time = vos_system_ticks();

	return pAdapter;
}
#endif

static hdd_adapter_t* hdd_alloc_station_adapter(hdd_context_t *pHddCtx,
                                                tSirMacAddr macAddr,
                                                unsigned char name_assign_type,
                                                const char* name)
{
   struct net_device *pWlanDev = NULL;
   hdd_adapter_t *pAdapter = NULL;
   /*
    * cfg80211 initialization and registration....
    */
   pWlanDev = alloc_netdev_mq(sizeof( hdd_adapter_t ),
                              name,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,17,0)) || defined(WITH_BACKPORTS)
                              name_assign_type,
#endif
                              (VOS_MONITOR_MODE == vos_get_conparam()?
                              mon_mode_ether_setup : ether_setup),
                              NUM_TX_QUEUES);

   if(pWlanDev != NULL)
   {

      //Save the pointer to the net_device in the HDD adapter
      pAdapter = (hdd_adapter_t*) netdev_priv( pWlanDev );

      vos_mem_zero( pAdapter, sizeof( hdd_adapter_t ) );

      pAdapter->dev = pWlanDev;
      pAdapter->pHddCtx = pHddCtx;
      pAdapter->magic = WLAN_HDD_ADAPTER_MAGIC;

      vos_event_init(&pAdapter->scan_info.scan_finished_event);
      pAdapter->scan_info.scan_pending_option = WEXT_SCAN_PENDING_GIVEUP;

      pAdapter->offloads_configured = FALSE;
      pAdapter->isLinkUpSvcNeeded = FALSE;
      pAdapter->higherDtimTransition = eANI_BOOLEAN_TRUE;
      //Init the net_device structure
      strlcpy(pWlanDev->name, name, IFNAMSIZ);

#ifdef SUPPORT_IFTYPE_P2P_DEVICE_VIF
      strlcpy(pAdapter->ifname, name, IFNAMSIZ);
#endif

      vos_mem_copy(pWlanDev->dev_addr, (void *)macAddr, sizeof(tSirMacAddr));
      vos_mem_copy( pAdapter->macAddressCurrent.bytes, macAddr, sizeof(tSirMacAddr));
      pWlanDev->watchdog_timeo = HDD_TX_TIMEOUT;
      /*
       * kernel will consume ethernet header length buffer for hard_header,
       * so just reserve it
       */
      hdd_set_needed_headroom(pWlanDev, pWlanDev->hard_header_len);

      if (pHddCtx->cfg_ini->enableIPChecksumOffload)
         pWlanDev->features |= NETIF_F_HW_CSUM;
      else if (pHddCtx->cfg_ini->enableTCPChkSumOffld)
         pWlanDev->features |= NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
      pWlanDev->features |= NETIF_F_RXCSUM;
      hdd_set_station_ops( pAdapter->dev );

      hdd_dev_setup_destructor(pWlanDev);
      pWlanDev->ieee80211_ptr = &pAdapter->wdev ;
      pWlanDev->tx_queue_len = HDD_NETDEV_TX_QUEUE_LEN;
      pAdapter->wdev.wiphy = pHddCtx->wiphy;
      pAdapter->wdev.netdev =  pWlanDev;
      /* set pWlanDev's parent to underlying device */
      SET_NETDEV_DEV(pWlanDev, pHddCtx->parent_dev);
      hdd_wmm_init( pAdapter );
      hdd_adapter_runtime_suspend_init(pAdapter);
      adf_os_spinlock_init(&pAdapter->pause_map_lock);
      pAdapter->last_tx_jiffies = jiffies;
      pAdapter->bug_report_count = 0;
      pAdapter->start_time = pAdapter->last_time = vos_system_ticks();
   }

   return pAdapter;
}

static hdd_adapter_t *hdd_alloc_monitor_adapter(hdd_context_t *pHddCtx,
						tSirMacAddr macAddr,
						unsigned char name_assign_type,
						const char *name)
{
	struct net_device *pwlan_dev = NULL;
	hdd_adapter_t *pAdapter = NULL;
	/*
	 * cfg80211 initialization and registration....
	 */
	pwlan_dev = alloc_netdev_mq(sizeof(hdd_adapter_t),
				   name,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)) || defined(WITH_BACKPORTS)
				   name_assign_type,
#endif
				   mon_mode_ether_setup,
				   NUM_TX_QUEUES);

	if (pwlan_dev != NULL) {
	   /* Save the pointer to the net_device in the HDD adapter */
	   pAdapter = (hdd_adapter_t *)netdev_priv(pwlan_dev);

	   vos_mem_zero(pAdapter, sizeof(hdd_adapter_t));

	   pAdapter->dev = pwlan_dev;
	   pAdapter->pHddCtx = pHddCtx;
	   pAdapter->magic = WLAN_HDD_ADAPTER_MAGIC;

	   vos_event_init(&pAdapter->scan_info.scan_finished_event);
	   pAdapter->scan_info.scan_pending_option = WEXT_SCAN_PENDING_GIVEUP;

	   pAdapter->offloads_configured = FALSE;
	   pAdapter->isLinkUpSvcNeeded = FALSE;
	   pAdapter->higherDtimTransition = eANI_BOOLEAN_TRUE;
	   /* Init the net_device structure */
	   strlcpy(pwlan_dev->name, name, IFNAMSIZ);

	   vos_mem_copy(pwlan_dev->dev_addr,
			(void *)macAddr, sizeof(tSirMacAddr));
	   vos_mem_copy(pAdapter->macAddressCurrent.bytes,
			macAddr, sizeof(tSirMacAddr));
	   pwlan_dev->watchdog_timeo = HDD_TX_TIMEOUT;
	   /*
	    * kernel will consume ethernet header length buffer for hard_header,
	    * so just reserve it
	    */
	   hdd_set_needed_headroom(pwlan_dev, pwlan_dev->hard_header_len);

	   if (pHddCtx->cfg_ini->enableIPChecksumOffload)
		pwlan_dev->features |= NETIF_F_HW_CSUM;
	   else if (pHddCtx->cfg_ini->enableTCPChkSumOffld)
		pwlan_dev->features |= NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;
	   pwlan_dev->features |= NETIF_F_RXCSUM;
	   hdd_set_monitor_ops(pAdapter->dev);

	   hdd_dev_setup_destructor(pwlan_dev);
	   pwlan_dev->ieee80211_ptr = &pAdapter->wdev;
	   pwlan_dev->tx_queue_len = HDD_NETDEV_TX_QUEUE_LEN;
	   pAdapter->wdev.wiphy = pHddCtx->wiphy;
	   pAdapter->wdev.netdev =  pwlan_dev;
	   /* set pwlan_dev's parent to underlying device */
	   SET_NETDEV_DEV(pwlan_dev, pHddCtx->parent_dev);
	   hdd_wmm_init(pAdapter);
	   hdd_adapter_runtime_suspend_init(pAdapter);
	   adf_os_spinlock_init(&pAdapter->pause_map_lock);
	   pAdapter->last_tx_jiffies = jiffies;
	   pAdapter->bug_report_count = 0;
	   pAdapter->start_time = pAdapter->last_time = vos_system_ticks();
	}

	return pAdapter;
}

VOS_STATUS hdd_register_interface( hdd_adapter_t *pAdapter, tANI_U8 rtnl_lock_held )
{
   struct net_device *pWlanDev = pAdapter->dev;

   if( rtnl_lock_held )
   {
     if (strnchr(pWlanDev->name, strlen(pWlanDev->name), '%')) {
         if( dev_alloc_name(pWlanDev, pWlanDev->name) < 0 )
         {
            hddLog(VOS_TRACE_LEVEL_ERROR,"%s:Failed:dev_alloc_name",__func__);
            return VOS_STATUS_E_FAILURE;
         }
      }
      if (register_netdevice(pWlanDev))
      {
         hddLog(VOS_TRACE_LEVEL_ERROR,"%s:Failed:register_netdev",__func__);
         return VOS_STATUS_E_FAILURE;
      }
   }
   else
   {
      if(register_netdev(pWlanDev))
      {
         hddLog(VOS_TRACE_LEVEL_ERROR,"%s: Failed:register_netdev",__func__);
         return VOS_STATUS_E_FAILURE;
      }
   }
   set_bit(NET_DEVICE_REGISTERED, &pAdapter->event_flags);

   return VOS_STATUS_SUCCESS;
}

/**
 * hdd_smeCloseSessionCallback() - HDD callback function
 * @pContext: adapter context
 *
 * This is a callback function HDD registers with SME.
 *
 * Returns: eHalstatus
 */
eHalStatus hdd_smeCloseSessionCallback(void *pContext)
{
   hdd_adapter_t *pAdapter = pContext;

   if (NULL == pAdapter)
   {
      hddLog(VOS_TRACE_LEVEL_FATAL, "%s: NULL pAdapter", __func__);
      return eHAL_STATUS_INVALID_PARAMETER;
   }

   if (WLAN_HDD_ADAPTER_MAGIC != pAdapter->magic)
   {
      hddLog(VOS_TRACE_LEVEL_FATAL, "%s: Invalid magic", __func__);
      return eHAL_STATUS_NOT_INITIALIZED;
   }

   /*
    * For NAN Data interface, the close session results in the final
    * indication to the userspace
    */
   hdd_ndp_session_end_handler(pAdapter);

   clear_bit(SME_SESSION_OPENED, &pAdapter->event_flags);

#if !defined (CONFIG_CNSS) && \
    !defined (WLAN_OPEN_SOURCE)
   /* need to make sure all of our scheduled work has completed.
    * This callback is called from MC thread context, so it is safe to
    * to call below flush work queue API from here.
    *
    * Even though this is called from MC thread context, if there is a faulty
    * work item in the system, that can hang this call forever.  So flushing
    * this global work queue is not safe; and now we make sure that
    * individual work queues are stopped correctly. But the cancel work queue
    * is a GPL only API, so the proprietary  version of the driver would still
    * rely on the global work queue flush.
    */
   flush_scheduled_work();
#endif

   /* We can be blocked while waiting for scheduled work to be
    * flushed, and the adapter structure can potentially be freed, in
    * which case the magic will have been reset.  So make sure the
    * magic is still good, and hence the adapter structure is still
    * valid, before signalling completion */
   if (WLAN_HDD_ADAPTER_MAGIC == pAdapter->magic)
   {
      complete(&pAdapter->session_close_comp_var);
   }

   return eHAL_STATUS_SUCCESS;
}

/**
 * hdd_close_tx_queues() - close tx queues
 * @hdd_ctx: hdd global context
 *
 * Return: None
 */
static void hdd_close_tx_queues(hdd_context_t *hdd_ctx)
{
	VOS_STATUS status;
	hdd_adapter_t *adapter;
	hdd_adapter_list_node_t *adapter_node = NULL, *next_adapter = NULL;
	/* Not validating hdd_ctx as it's already done by the caller */
	ENTER();
	status = hdd_get_front_adapter(hdd_ctx, &adapter_node);
	while (NULL != adapter_node && VOS_STATUS_SUCCESS == status) {
		adapter = adapter_node->pAdapter;
		if (adapter && adapter->dev) {
			wlan_hdd_netif_queue_control(adapter,
				WLAN_NETIF_TX_DISABLE_N_CARRIER,
				WLAN_CONTROL_PATH);
		}
		status = hdd_get_next_adapter(hdd_ctx, adapter_node,
						&next_adapter);
		adapter_node = next_adapter;
	}
	EXIT();
}

/**
 * hdd_check_and_init_tdls() - check and init TDLS operation for desired mode
 * @adapter: pointer to device adapter
 * @type: type of interface
 *
 * This routine will check the mode of adapter and if it is required then it
 * will initialize the TDLS operations
 *
 * Return: VOS_STATUS
 */
#ifdef FEATURE_WLAN_TDLS
static VOS_STATUS hdd_check_and_init_tdls(hdd_adapter_t *adapter, uint32_t type)
{
	if (VOS_IBSS_MODE != type) {
		if (0 != wlan_hdd_tdls_init(adapter)) {
			hddLog(LOGE, FL("wlan_hdd_tdls_init failed"));
			return VOS_STATUS_E_FAILURE;
		}
		set_bit(TDLS_INIT_DONE, &adapter->event_flags);
	}
	return VOS_STATUS_SUCCESS;
}
#else
static VOS_STATUS hdd_check_and_init_tdls(hdd_adapter_t *adapter, uint32_t type)
{
	return VOS_STATUS_SUCCESS;
}
#endif

VOS_STATUS hdd_init_station_mode( hdd_adapter_t *pAdapter )
{
   struct net_device *pWlanDev = pAdapter->dev;
   hdd_station_ctx_t *pHddStaCtx = &pAdapter->sessionCtx.station;
   hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX( pAdapter );
   eHalStatus halStatus = eHAL_STATUS_SUCCESS;
   VOS_STATUS status = VOS_STATUS_E_FAILURE;
   tANI_U32 type, subType;
   unsigned long rc;
   int ret_val;

   INIT_COMPLETION(pAdapter->session_open_comp_var);
   sme_SetCurrDeviceMode(pHddCtx->hHal, pAdapter->device_mode);
   sme_set_pdev_ht_vht_ies(pHddCtx->hHal, pHddCtx->cfg_ini->enable2x2);
   status = vos_get_vdev_types(pAdapter->device_mode, &type, &subType);
   if (VOS_STATUS_SUCCESS != status)
   {
      hddLog(VOS_TRACE_LEVEL_ERROR, "failed to get vdev type");
      goto error_sme_open;
   }

   ret_val = process_wma_set_command((int)pAdapter->sessionId,
                         (int)WMI_PDEV_PARAM_BURST_ENABLE,
                         (int)pHddCtx->cfg_ini->enableSifsBurst,
                         PDEV_CMD);

   if (0 != ret_val) {
       hddLog(VOS_TRACE_LEVEL_ERROR,
                   "%s: WMI_PDEV_PARAM_BURST_ENABLE set failed %d",
                   __func__, ret_val);
   }

   //Open a SME session for future operation
   halStatus = sme_OpenSession( pHddCtx->hHal, hdd_smeRoamCallback, pAdapter,
         (tANI_U8 *)&pAdapter->macAddressCurrent, &pAdapter->sessionId,
         type, subType);
   if ( !HAL_STATUS_SUCCESS( halStatus ) )
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,
             "sme_OpenSession() failed with status code %08d [x%08x]",
                                                 halStatus, halStatus );
      status = VOS_STATUS_E_FAILURE;
      goto error_sme_open;
   }

   //Block on a completion variable. Can't wait forever though.
   rc = wait_for_completion_timeout(
                        &pAdapter->session_open_comp_var,
                        msecs_to_jiffies(WLAN_WAIT_TIME_SESSIONOPENCLOSE));
   if (!rc) {
      hddLog(VOS_TRACE_LEVEL_FATAL,
             FL("Session is not opened within timeout period code %ld"),
             rc );
      status = VOS_STATUS_E_FAILURE;
      goto error_sme_open;
   }

   // Register wireless extensions
   if( eHAL_STATUS_SUCCESS !=  (halStatus = hdd_register_wext(pWlanDev)))
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,
              "hdd_register_wext() failed with status code %08d [x%08x]",
                                                   halStatus, halStatus );
      status = VOS_STATUS_E_FAILURE;
      goto error_register_wext;
   }

   //Set the Connection State to Not Connected
   hdd_connSetConnectionState(pAdapter, eConnectionState_NotConnected);

   //Set the default operation channel
   pHddStaCtx->conn_info.operationChannel = pHddCtx->cfg_ini->OperatingChannel;

   /* Make the default Auth Type as OPEN*/
   pHddStaCtx->conn_info.authType = eCSR_AUTH_TYPE_OPEN_SYSTEM;

   if( VOS_STATUS_SUCCESS != ( status = hdd_init_tx_rx( pAdapter ) ) )
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,
            "hdd_init_tx_rx() failed with status code %08d [x%08x]",
                            status, status );
      goto error_init_txrx;
   }

   set_bit(INIT_TX_RX_SUCCESS, &pAdapter->event_flags);

   if( VOS_STATUS_SUCCESS != ( status = hdd_wmm_adapter_init( pAdapter ) ) )
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,
            "hdd_wmm_adapter_init() failed with status code %08d [x%08x]",
                            status, status );
      goto error_wmm_init;
   }

   set_bit(WMM_INIT_DONE, &pAdapter->event_flags);

   status = hdd_check_and_init_tdls(pAdapter, type);
   if (status != VOS_STATUS_SUCCESS)
       goto error_tdls_init;

   return VOS_STATUS_SUCCESS;

error_tdls_init:
   clear_bit(WMM_INIT_DONE, &pAdapter->event_flags);
   hdd_wmm_adapter_close(pAdapter);
error_wmm_init:
   clear_bit(INIT_TX_RX_SUCCESS, &pAdapter->event_flags);
   hdd_deinit_tx_rx(pAdapter);
error_init_txrx:
   hdd_UnregisterWext(pWlanDev);
error_register_wext:
   if (test_bit(SME_SESSION_OPENED, &pAdapter->event_flags))
   {
      INIT_COMPLETION(pAdapter->session_close_comp_var);
      if (eHAL_STATUS_SUCCESS == sme_CloseSession(pHddCtx->hHal,
                                    pAdapter->sessionId,
                                    hdd_smeCloseSessionCallback, pAdapter))
      {
         unsigned long rc;

         //Block on a completion variable. Can't wait forever though.
         rc = wait_for_completion_timeout(
                          &pAdapter->session_close_comp_var,
                          msecs_to_jiffies(WLAN_WAIT_TIME_SESSIONOPENCLOSE));
         if (rc <= 0)
            hddLog(VOS_TRACE_LEVEL_ERROR,
                   FL("Session is not opened within timeout period code %ld"),
                   rc);
      }
   }
error_sme_open:
   return status;
}

void hdd_cleanup_actionframe( hdd_context_t *pHddCtx, hdd_adapter_t *pAdapter )
{
   hdd_cfg80211_state_t *cfgState;

   cfgState = WLAN_HDD_GET_CFG_STATE_PTR( pAdapter );

   if( NULL != cfgState->buf )
   {
      unsigned long rc;
      INIT_COMPLETION(pAdapter->tx_action_cnf_event);
      rc = wait_for_completion_timeout(
                     &pAdapter->tx_action_cnf_event,
                     msecs_to_jiffies(ACTION_FRAME_TX_TIMEOUT));
      if (!rc)
      {
         VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                   "%s HDD Wait for Action Confirmation Failed!!",
                   __func__);
         /* Inform tx status as FAILURE to upper layer and free
          * cfgState->buf */
         hdd_sendActionCnf( pAdapter, FALSE );
      }
   }
   return;
}

void hdd_deinit_adapter(hdd_context_t *pHddCtx, hdd_adapter_t *pAdapter,
                        bool rtnl_held)
{
   ENTER();
   switch ( pAdapter->device_mode )
   {
      case WLAN_HDD_INFRA_STATION:
      case WLAN_HDD_P2P_CLIENT:
      case WLAN_HDD_P2P_DEVICE:
      {
         if (test_bit(INIT_TX_RX_SUCCESS, &pAdapter->event_flags))
         {
            hdd_deinit_tx_rx( pAdapter );
            clear_bit(INIT_TX_RX_SUCCESS, &pAdapter->event_flags);
         }

         if (test_bit(WMM_INIT_DONE, &pAdapter->event_flags))
         {
            hdd_wmm_adapter_close( pAdapter );
            clear_bit(WMM_INIT_DONE, &pAdapter->event_flags);
         }

         hdd_cleanup_actionframe(pHddCtx, pAdapter);
         wlan_hdd_tdls_exit(pAdapter);
         break;
      }

      case WLAN_HDD_SOFTAP:
      case WLAN_HDD_P2P_GO:
      {
         if (test_bit(INIT_TX_RX_SUCCESS, &pAdapter->event_flags))
         {
            hdd_softap_deinit_tx_rx(pAdapter);
            clear_bit(INIT_TX_RX_SUCCESS, &pAdapter->event_flags);
         }

         if (test_bit(WMM_INIT_DONE, &pAdapter->event_flags))
         {
            hdd_wmm_adapter_close( pAdapter );
            clear_bit(WMM_INIT_DONE, &pAdapter->event_flags);
         }
         wlan_hdd_undo_acs(pAdapter);

         hdd_cleanup_actionframe(pHddCtx, pAdapter);

         hdd_unregister_hostapd(pAdapter, rtnl_held);

         // set con_mode to STA only when no SAP concurrency mode
         if (!(hdd_get_concurrency_mode() & (VOS_SAP | VOS_P2P_GO)))
             hdd_set_conparam( 0 );
         break;
      }

      default:
      break;
   }

   EXIT();
}

void hdd_cleanup_adapter(hdd_context_t *pHddCtx, hdd_adapter_t *pAdapter,
                         tANI_U8 rtnl_held)
{
   struct net_device *pWlanDev = NULL;

   if (pAdapter)
      pWlanDev = pAdapter->dev;
   else {
      hddLog(LOGE, FL("pAdapter is Null"));
      return;
   }

    hdd_adapter_runtime_suspend_denit(pAdapter);
   /* The adapter is marked as closed. When hdd_wlan_exit() call returns,
    * the driver is almost closed and cannot handle either control
    * messages or data. However, unregister_netdevice() call above will
    * eventually invoke hdd_stop (ndo_close) driver callback, which attempts
    * to close the active connections (basically excites control path) which
    * is not right. Setting this flag helps hdd_stop() to recognize that
    * the interface is closed and restricts any operations on that
    */
   clear_bit(DEVICE_IFACE_OPENED, &pAdapter->event_flags);

   if (test_bit(NET_DEVICE_REGISTERED, &pAdapter->event_flags)) {
      if (rtnl_held) {
         unregister_netdevice(pWlanDev);
      } else {
         unregister_netdev(pWlanDev);
      }
      /* Note that the pAdapter is no longer valid at this point
         since the memory has been reclaimed */
   }

#ifdef SUPPORT_IFTYPE_P2P_DEVICE_VIF
   if (pAdapter->device_mode == WLAN_HDD_P2P_DEVICE) {
      if (!rtnl_is_locked()) {
         rtnl_lock();
         cfg80211_unregister_wdev(&(pAdapter->wdev));
         rtnl_unlock();
      } else
         cfg80211_unregister_wdev(&(pAdapter->wdev));
   }
#endif
}

void hdd_set_pwrparams(hdd_context_t *pHddCtx)
{
   VOS_STATUS status;
   hdd_adapter_t *pAdapter = NULL;
   hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;

   status =  hdd_get_front_adapter ( pHddCtx, &pAdapterNode );

   /*loop through all adapters.*/
   while ( NULL != pAdapterNode && VOS_STATUS_SUCCESS == status )
   {
       pAdapter = pAdapterNode->pAdapter;
       if ( (WLAN_HDD_INFRA_STATION != pAdapter->device_mode)
         && (WLAN_HDD_P2P_CLIENT != pAdapter->device_mode) )

       {  // we skip this registration for modes other than STA and P2P client modes.
           status = hdd_get_next_adapter ( pHddCtx, pAdapterNode, &pNext );
           pAdapterNode = pNext;
           continue;
       }

       //Apply Dynamic DTIM For P2P
       //Only if ignoreDynamicDtimInP2pMode is not set in ini
      if ((pHddCtx->cfg_ini->enableDynamicDTIM ||
           pHddCtx->cfg_ini->enableModulatedDTIM) &&
          ((WLAN_HDD_INFRA_STATION == pAdapter->device_mode) ||
          ((WLAN_HDD_P2P_CLIENT == pAdapter->device_mode) &&
          !(pHddCtx->cfg_ini->ignoreDynamicDtimInP2pMode))) &&
          (eANI_BOOLEAN_TRUE == pAdapter->higherDtimTransition) &&
          (eConnectionState_Associated ==
          (WLAN_HDD_GET_STATION_CTX_PTR(pAdapter))->conn_info.connState) &&
          (pHddCtx->cfg_ini->fIsBmpsEnabled))
      {
           tSirSetPowerParamsReq powerRequest = { 0 };

           powerRequest.uIgnoreDTIM = 1;
           powerRequest.uMaxLIModulatedDTIM = pHddCtx->cfg_ini->fMaxLIModulatedDTIM;

           if (pHddCtx->cfg_ini->enableModulatedDTIM)
           {
               powerRequest.uDTIMPeriod = pHddCtx->cfg_ini->enableModulatedDTIM;
               powerRequest.uListenInterval = pHddCtx->hdd_actual_LI_value;
           }
           else
           {
               powerRequest.uListenInterval = pHddCtx->cfg_ini->enableDynamicDTIM;
           }

           /* Update ignoreDTIM and ListedInterval in CFG to remain at the DTIM
            * specified during Enter/Exit BMPS when LCD off*/
            ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_IGNORE_DTIM, powerRequest.uIgnoreDTIM,
                       NULL, eANI_BOOLEAN_FALSE);
            ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_LISTEN_INTERVAL, powerRequest.uListenInterval,
                       NULL, eANI_BOOLEAN_FALSE);

           /* switch to the DTIM specified in cfg.ini */
            VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                      "Switch to DTIM %d", powerRequest.uListenInterval);
            sme_SetPowerParams( pHddCtx->hHal, &powerRequest, TRUE);
            break;

      }

      status = hdd_get_next_adapter ( pHddCtx, pAdapterNode, &pNext );
      pAdapterNode = pNext;
    }
}

void hdd_reset_pwrparams(hdd_context_t *pHddCtx)
{
   /*Switch back to DTIM 1*/
   tSirSetPowerParamsReq powerRequest = { 0 };

   powerRequest.uIgnoreDTIM = pHddCtx->hdd_actual_ignore_DTIM_value;
   powerRequest.uListenInterval = pHddCtx->hdd_actual_LI_value;
   powerRequest.uMaxLIModulatedDTIM = pHddCtx->cfg_ini->fMaxLIModulatedDTIM;

   /* Update ignoreDTIM and ListedInterval in CFG with default values */
   ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_IGNORE_DTIM, powerRequest.uIgnoreDTIM,
                    NULL, eANI_BOOLEAN_FALSE);
   ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_LISTEN_INTERVAL, powerRequest.uListenInterval,
                    NULL, eANI_BOOLEAN_FALSE);

   VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                  "Switch to DTIM%d",powerRequest.uListenInterval);
   sme_SetPowerParams( pHddCtx->hHal, &powerRequest, TRUE);

}

VOS_STATUS hdd_enable_bmps_imps(hdd_context_t *pHddCtx)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;

   if (0 != wlan_hdd_validate_context(pHddCtx))
       return VOS_STATUS_E_PERM;

   if(pHddCtx->cfg_ini->fIsBmpsEnabled)
   {
      sme_EnablePowerSave(pHddCtx->hHal, ePMC_BEACON_MODE_POWER_SAVE);
   }

   if(pHddCtx->cfg_ini->fIsAutoBmpsTimerEnabled)
   {
      sme_StartAutoBmpsTimer(pHddCtx->hHal);
   }

   if (pHddCtx->cfg_ini->fIsImpsEnabled)
   {
      sme_EnablePowerSave (pHddCtx->hHal, ePMC_IDLE_MODE_POWER_SAVE);
   }

   return status;
}

VOS_STATUS hdd_disable_bmps_imps(hdd_context_t *pHddCtx, tANI_U8 session_type)
{
   hdd_adapter_t *pAdapter = NULL;
   eHalStatus halStatus;
   VOS_STATUS status = VOS_STATUS_E_INVAL;
   v_BOOL_t disableBmps = FALSE;
   v_BOOL_t disableImps = FALSE;

   switch(session_type)
   {
       case WLAN_HDD_INFRA_STATION:
       case WLAN_HDD_SOFTAP:
       case WLAN_HDD_P2P_CLIENT:
       case WLAN_HDD_P2P_GO:
          //Exit BMPS -> Is Sta/P2P Client is already connected
          pAdapter = hdd_get_adapter(pHddCtx, WLAN_HDD_INFRA_STATION);
          if((NULL != pAdapter)&&
              hdd_connIsConnected( WLAN_HDD_GET_STATION_CTX_PTR(pAdapter)))
          {
             disableBmps = TRUE;
          }

          pAdapter = hdd_get_adapter(pHddCtx, WLAN_HDD_P2P_CLIENT);
          if((NULL != pAdapter)&&
              hdd_connIsConnected( WLAN_HDD_GET_STATION_CTX_PTR(pAdapter)))
          {
             disableBmps = TRUE;
          }

          //Exit both Bmps and Imps incase of Go/SAP Mode
          if((WLAN_HDD_SOFTAP == session_type) ||
              (WLAN_HDD_P2P_GO == session_type))
          {
             disableBmps = TRUE;
             disableImps = TRUE;
          }

          if(TRUE == disableImps)
          {
             if (pHddCtx->cfg_ini->fIsImpsEnabled)
             {
                sme_DisablePowerSave (pHddCtx->hHal, ePMC_IDLE_MODE_POWER_SAVE);
             }
          }

          if(TRUE == disableBmps)
          {
             if(pHddCtx->cfg_ini->fIsBmpsEnabled)
             {
                 halStatus = sme_DisablePowerSave(pHddCtx->hHal, ePMC_BEACON_MODE_POWER_SAVE);

                 if(eHAL_STATUS_SUCCESS != halStatus)
                 {
                    status = VOS_STATUS_E_FAILURE;
                    hddLog(VOS_TRACE_LEVEL_ERROR,"%s: Fail to Disable Power Save", __func__);
                    VOS_ASSERT(0);
                    return status;
                 }
              }

              if(pHddCtx->cfg_ini->fIsAutoBmpsTimerEnabled)
              {
                 halStatus = sme_StopAutoBmpsTimer(pHddCtx->hHal);

                 if(eHAL_STATUS_SUCCESS != halStatus)
                 {
                    status = VOS_STATUS_E_FAILURE;
                    hddLog(VOS_TRACE_LEVEL_ERROR,"%s: Fail to Stop Auto Bmps Timer", __func__);
                    VOS_ASSERT(0);
                    return status;
                 }
              }
          }

          if((TRUE == disableBmps) ||
              (TRUE == disableImps))
          {
              /* Now, get the chip into Full Power now */
              INIT_COMPLETION(pHddCtx->full_pwr_comp_var);
              halStatus = sme_RequestFullPower(pHddCtx->hHal, hdd_full_pwr_cbk,
                                   pHddCtx, eSME_FULL_PWR_NEEDED_BY_HDD);

              if(halStatus != eHAL_STATUS_SUCCESS)
              {
                 if(halStatus == eHAL_STATUS_PMC_PENDING)
                 {
                    unsigned long rc;
                    //Block on a completion variable. Can't wait forever though
                    rc = wait_for_completion_timeout(
                                   &pHddCtx->full_pwr_comp_var,
                                   msecs_to_jiffies(1000));
                    if (!rc) {
                       hddLog(VOS_TRACE_LEVEL_ERROR,
                              "%s: wait on full_pwr_comp_var failed",
                              __func__);
}
                 }
                 else
                 {
                    status = VOS_STATUS_E_FAILURE;
                    hddLog(VOS_TRACE_LEVEL_ERROR,"%s: Request for Full Power failed", __func__);
                    VOS_ASSERT(0);
                    return status;
                 }
              }

              status = VOS_STATUS_SUCCESS;
          }

          break;
   }
   return status;
}

VOS_STATUS hdd_check_for_existing_macaddr( hdd_context_t *pHddCtx,
                                           tSirMacAddr macAddr )
{
    hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
    hdd_adapter_t *pAdapter;
    VOS_STATUS status;

    status = hdd_get_front_adapter(pHddCtx, &pAdapterNode);

    while (NULL != pAdapterNode && VOS_STATUS_SUCCESS == status) {
        pAdapter = pAdapterNode->pAdapter;

        if (pAdapter && vos_mem_compare(pAdapter->macAddressCurrent.bytes,
                                         macAddr, sizeof(tSirMacAddr))) {
            return VOS_STATUS_E_FAILURE;
        }
        status = hdd_get_next_adapter(pHddCtx, pAdapterNode, &pNext);
        pAdapterNode = pNext;
    }

    return VOS_STATUS_SUCCESS;
}

uint32_t hdd_get_current_vdev_sta_count(hdd_context_t *hdd_ctx)
{
	hdd_adapter_t *adapter;
	hdd_adapter_list_node_t *hdd_adapter_node, *next;
	VOS_STATUS status;
	uint32_t vdev_sta_cnt = 0;

	status = hdd_get_front_adapter(hdd_ctx, &hdd_adapter_node);

	while (NULL != hdd_adapter_node && VOS_STATUS_SUCCESS == status) {
		adapter = hdd_adapter_node->pAdapter;
		if ((NULL != adapter) &&
		    ((WLAN_HDD_INFRA_STATION == adapter->device_mode) ||
		     (WLAN_HDD_P2P_CLIENT == adapter->device_mode) ||
		     (WLAN_HDD_IBSS == adapter->device_mode)))
			vdev_sta_cnt++;

		status = hdd_get_next_adapter(hdd_ctx, hdd_adapter_node, &next);
		hdd_adapter_node = next;
	}

	return vdev_sta_cnt;
}

hdd_adapter_t *hdd_open_adapter(hdd_context_t *hdd_ctx,
				uint8_t session_type,
				const char *iface_name,
				tSirMacAddr mac_addr,
				uint8_t name_assign_type,
				uint8_t rtnl_held)
{
	VOS_STATUS status, exit_bmps_status = VOS_STATUS_E_FAILURE;
	hdd_adapter_t *adapter;
	hdd_adapter_list_node_t *hdd_adapter_node;
	hdd_cfg80211_state_t *cfg_state;
	int ret;

	hddLog(VOS_TRACE_LEVEL_INFO_HIGH, "%s: iface =%s type = %d\n",
	       __func__, iface_name, session_type);

	if (hdd_ctx->current_intf_count >= hdd_ctx->max_intf_count) {
		/*
		 * Max limit reached on the number of vdevs
		 * configured by the host.
		 */
		hddLog(VOS_TRACE_LEVEL_ERROR,
		       "%s: Unable to add virtual intf: "
		       "current_vdev_cnt=%d,host_configured_vdev_cnt=%d",
		       __func__,
		       hdd_ctx->current_intf_count,
		       hdd_ctx->max_intf_count);
		return NULL;
	}

	if (((WLAN_HDD_INFRA_STATION == session_type) ||
	     (WLAN_HDD_P2P_CLIENT == session_type) ||
	     (WLAN_HDD_IBSS == session_type)) &&
	    (WLAN_HDD_VDEV_STA_MAX ==
	     hdd_get_current_vdev_sta_count(hdd_ctx))) {
		hddLog(VOS_TRACE_LEVEL_ERROR,
		       "%s: Unable to add sta interface: max sta cnt is %d",
		       __func__, WLAN_HDD_VDEV_STA_MAX);
		return NULL;
	}

	if (hdd_cfg_is_sub20_channel_width_enabled(hdd_ctx) &&
	    (hdd_ctx->current_intf_count >= 1)) {
		hddLog(VOS_TRACE_LEVEL_ERROR,
		       "%s: Unable add another virtual intf when sub20 enable",
		       __func__);
		return NULL;
	}

	if (NULL == mac_addr) {
		/* Not received valid macAddr */
		hddLog(VOS_TRACE_LEVEL_ERROR,
		       "%s:Unable to add virtual intf: Not able to get"
		       "valid mac address", __func__);
		return NULL;
	}

	status = hdd_check_for_existing_macaddr(hdd_ctx, mac_addr);
	if (VOS_STATUS_E_FAILURE == status) {
		hddLog(VOS_TRACE_LEVEL_ERROR,
		       "%s: Duplicate MAC addr: "MAC_ADDRESS_STR
		       " already exists",
		       __func__, MAC_ADDR_ARRAY(mac_addr));
		return NULL;
	}

	/*
	 * If Powersave Offload is enabled
	 * Fw will take care incase of concurrency
	 */
	if (!hdd_ctx->cfg_ini->enablePowersaveOffload) {
		/* Disable BMPS incase of Concurrency */
		exit_bmps_status = hdd_disable_bmps_imps(hdd_ctx, session_type);

		if (VOS_STATUS_E_FAILURE == exit_bmps_status) {
			/* Fail to Exit BMPS */
			hddLog(VOS_TRACE_LEVEL_ERROR, "%s: Fail to Exit BMPS",
			       __func__);
			VOS_ASSERT(0);
			return NULL;
		}
	}

	switch (session_type) {

	case WLAN_HDD_INFRA_STATION:
		 /* Reset locally administered bit if the device mode is STA */
		WLAN_HDD_RESET_LOCALLY_ADMINISTERED_BIT(mac_addr);
	/* fall through */
	case WLAN_HDD_P2P_CLIENT:
	case WLAN_HDD_P2P_DEVICE:
	case WLAN_HDD_OCB:
	case WLAN_HDD_NDI:
	{
#ifdef SUPPORT_IFTYPE_P2P_DEVICE_VIF
		if (session_type == WLAN_HDD_P2P_DEVICE)
			adapter = hdd_alloc_p2p_device_adapter(hdd_ctx,
							       mac_addr,
							       name_assign_type,
							       iface_name);
		else
#endif
			adapter = hdd_alloc_station_adapter(hdd_ctx,
							    mac_addr,
							    name_assign_type,
							    iface_name);

		if (NULL == adapter) {
			hddLog(VOS_TRACE_LEVEL_FATAL,
			       FL("%s: Failed to allocate adapter for session %d"),
			       __func__, session_type);
			return NULL;
		}

		if (0 != hdd_init_packet_filtering(hdd_ctx, adapter))
			goto err_init_packet_filtering;

		if (session_type == WLAN_HDD_P2P_CLIENT)
			adapter->wdev.iftype = NL80211_IFTYPE_P2P_CLIENT;
                else if (VOS_MONITOR_MODE == vos_get_conparam())
                        adapter->wdev.iftype = NL80211_IFTYPE_MONITOR;
#ifdef SUPPORT_IFTYPE_P2P_DEVICE_VIF
		else if (session_type == WLAN_HDD_P2P_DEVICE)
			adapter->wdev.iftype = NL80211_IFTYPE_P2P_DEVICE;
#endif
		else
			adapter->wdev.iftype = NL80211_IFTYPE_STATION;

		adapter->device_mode = session_type;

		hdd_initialize_adapter_common(adapter);
		if (WLAN_HDD_NDI == session_type)
			status = hdd_init_nan_data_mode(adapter);
		else
			status = hdd_init_station_mode(adapter);

		if (VOS_STATUS_SUCCESS != status)
			goto err_init_adapter_mode;

		/* initialize action frame random mac info */
		hdd_adapter_init_action_frame_random_mac(adapter);

		/* Workqueue which gets scheduled in IPv4
		 * notification callback
		 */
		vos_init_work(&adapter->ipv4NotifierWorkQueue,
			      hdd_ipv4_notifier_work_queue);

#ifdef WLAN_NS_OFFLOAD
		/* Workqueue which gets scheduled in IPv6
		 * notification callback
		 */
		vos_init_work(&adapter->ipv6NotifierWorkQueue,
			      hdd_ipv6_notifier_work_queue);
#endif

#ifdef SUPPORT_IFTYPE_P2P_DEVICE_VIF
		if (session_type != WLAN_HDD_P2P_DEVICE)
#endif
		{
			status = hdd_register_interface(adapter, rtnl_held);
			if (VOS_STATUS_SUCCESS != status)
				goto err_register_interface;
		}

		/* do not disable tx in monitor mode */
		if (VOS_MONITOR_MODE != vos_get_conparam()) {
			/* Stop the Interface TX queue */
			hddLog(LOG1, FL("Disabling queues"));
			wlan_hdd_netif_queue_control(adapter,
					WLAN_NETIF_TX_DISABLE_N_CARRIER,
					WLAN_CONTROL_PATH);
		}

#ifdef QCA_LL_TX_FLOW_CT
		/* SAT mode default TX Flow control instance
		 * This instance will be used for
		 * STA mode, IBSS mode and TDLS mode */
		if (adapter->tx_flow_timer_initialized == VOS_FALSE) {
			vos_timer_init(&adapter->tx_flow_control_timer,
				       VOS_TIMER_TYPE_SW,
				       hdd_tx_resume_timer_expired_handler,
				       adapter);
			adapter->tx_flow_timer_initialized = VOS_TRUE;
		}
		WLANTL_RegisterTXFlowControl(hdd_ctx->pvosContext,
					     hdd_tx_resume_cb,
					     adapter->sessionId,
					     (void *)adapter);
#endif /* QCA_LL_TX_FLOW_CT */

		break;
	}

	case WLAN_HDD_P2P_GO:
	case WLAN_HDD_SOFTAP:
	{
		adapter = hdd_wlan_create_ap_dev(hdd_ctx,
						 mac_addr,
						 name_assign_type,
						 (tANI_U8 *)iface_name);
		if (NULL == adapter) {
			hddLog(VOS_TRACE_LEVEL_FATAL,
			       FL("failed to allocate adapter for session %d"),
			       session_type);
			return NULL;
		}

		if (0 != hdd_init_packet_filtering(hdd_ctx, adapter))
			goto err_init_packet_filtering;

		adapter->wdev.iftype = (session_type == WLAN_HDD_SOFTAP) ?
				       NL80211_IFTYPE_AP :
				       NL80211_IFTYPE_P2P_GO;
		adapter->device_mode = session_type;

		hdd_initialize_adapter_common(adapter);
		status = hdd_init_ap_mode(adapter, false);
		if (VOS_STATUS_SUCCESS != status)
			goto err_init_adapter_mode;

		status = hdd_register_hostapd(adapter, rtnl_held);
		if (VOS_STATUS_SUCCESS != status)
			goto err_register_interface;

		hddLog(LOG1, FL("Disabling queues"));
		wlan_hdd_netif_queue_control(adapter,
					     WLAN_NETIF_TX_DISABLE_N_CARRIER,
					     WLAN_CONTROL_PATH);
		hdd_set_conparam(1);

		/* Workqueue which gets scheduled in IPv4
		 * notification callback
		 */
		vos_init_work(&adapter->ipv4NotifierWorkQueue,
			      hdd_ipv4_notifier_work_queue);

#ifdef WLAN_NS_OFFLOAD
		/* Workqueue which gets scheduled in IPv6
		 * notification callback
		 */
		vos_init_work(&adapter->ipv6NotifierWorkQueue,
			      hdd_ipv6_notifier_work_queue);
#endif

		break;
	}

	case WLAN_HDD_FTM:
	{
		adapter = hdd_alloc_station_adapter(hdd_ctx,
						    mac_addr,
						    name_assign_type,
						    iface_name);

		if (NULL == adapter) {
			hddLog(VOS_TRACE_LEVEL_FATAL,
			       FL("failed to allocate adapter for session %d"),
			       session_type);
			return NULL;
		}

		/* Assign NL80211_IFTYPE_STATION as interface type to resolve
		 * Kernel Warning message while loading driver in FTM mode.
		 */
		adapter->wdev.iftype = NL80211_IFTYPE_STATION;
		adapter->device_mode = session_type;
		status = hdd_register_interface(adapter, rtnl_held);
		if (VOS_STATUS_SUCCESS != status)
			goto err_register_interface;

		hdd_initialize_adapter_common(adapter);
		hdd_init_tx_rx(adapter);

		/* Stop the Interface TX queue */
		hddLog(LOG1, FL("Disabling queues"));
		wlan_hdd_netif_queue_control(adapter,
					     WLAN_NETIF_TX_DISABLE_N_CARRIER,
					     WLAN_CONTROL_PATH);

		break;
	}

	case WLAN_HDD_MONITOR:
	{
		adapter = hdd_alloc_monitor_adapter(hdd_ctx,
						    mac_addr,
						    name_assign_type,
						    iface_name);

		if (NULL == adapter) {
			hddLog(VOS_TRACE_LEVEL_FATAL,
			       FL("failed to allocate adapter for session %d"),
			       session_type);
			return NULL;
		}

		adapter->wdev.iftype = NL80211_IFTYPE_MONITOR;
		adapter->device_mode = session_type;
		status = hdd_register_interface(adapter, rtnl_held);
		if (VOS_STATUS_SUCCESS != status)
			goto err_register_interface;

		hdd_initialize_adapter_common(adapter);

		/* Stop the Interface TX queue */
		hddLog(LOG1, FL("Disabling queues"));
		wlan_hdd_netif_queue_control(adapter,
					     WLAN_NETIF_TX_DISABLE_N_CARRIER,
					     WLAN_CONTROL_PATH);

		break;
	}

	default:
	{
		hddLog(VOS_TRACE_LEVEL_FATAL, "%s Invalid session type %d",
		       __func__, session_type);
		VOS_ASSERT(0);
		return NULL;
	}

	}

	vos_init_work(&adapter->scan_block_work,
		      wlan_hdd_cfg80211_scan_block_cb);

	cfg_state = WLAN_HDD_GET_CFG_STATE_PTR(adapter);
	mutex_init(&cfg_state->remain_on_chan_ctx_lock);

#ifdef WLAN_FEATURE_MBSSID
	hdd_mbssid_apply_def_cfg_ini(adapter);
#endif
	/* Add it to the hdd's session list */
	hdd_adapter_node = vos_mem_malloc(sizeof(hdd_adapter_list_node_t));
	if (NULL == hdd_adapter_node)
		goto err_malloc_adapter_node;

	hdd_adapter_node->pAdapter = adapter;
	status = hdd_add_adapter_back(hdd_ctx,
				      hdd_adapter_node);

	if (VOS_STATUS_SUCCESS != status)
		goto err_add_adapter_back;

	wlan_hdd_set_concurrency_mode(hdd_ctx, session_type);

	/* Initialize the WoWL service */
	if (!hdd_init_wowl(adapter)) {
		hddLog(VOS_TRACE_LEVEL_FATAL,
		       "%s: hdd_init_wowl failed", __func__);
		goto err_post_add_adapter;
	}

	/* Adapter successfully added. Increment the vdev count  */
	hdd_ctx->current_intf_count++;

	adapter->tsf_id = HDD_TSF2;

	hddLog(VOS_TRACE_LEVEL_DEBUG,
	       "%s: current_intf_count=%d", __func__,
	       hdd_ctx->current_intf_count);
#ifdef FEATURE_WLAN_STA_AP_MODE_DFS_DISABLE
	if (vos_get_concurrency_mode() == VOS_STA_SAP) {
		hdd_adapter_t *ap_adapter;

		ap_adapter = hdd_get_adapter(hdd_ctx, WLAN_HDD_SOFTAP);
		if (ap_adapter != NULL &&
		    test_bit(SOFTAP_BSS_STARTED, &ap_adapter->event_flags) &&
		    VOS_IS_DFS_CH(ap_adapter->sessionCtx.ap.operatingChannel)) {

			hddLog(VOS_TRACE_LEVEL_WARN,
			       "STA-AP Mode DFS not supported. "
			       "Restart SAP with Non DFS ACS");
			ap_adapter->sessionCtx.ap.sapConfig.channel =
				AUTO_CHANNEL_SELECT;
			ap_adapter->sessionCtx.ap.sapConfig.acs_cfg.acs_mode =
				true;
			wlan_hdd_restart_sap(ap_adapter);
		}
	}
#endif

	if ((vos_get_conparam() != VOS_FTM_MODE) &&
	    (!hdd_ctx->cfg_ini->enable2x2)) {
#define HDD_DTIM_1CHAIN_RX_ID 0x5
#define HDD_SMPS_PARAM_VALUE_S 29

		/* Disable DTIM 1 chain Rx when in 1x1,
		 * we are passing two values as
		 * param_id << 29 | param_value.
		 * Below param_value = 0(disable)
		 */
		ret = process_wma_set_command((int)adapter->sessionId,
				(int)WMI_STA_SMPS_PARAM_CMDID,
				HDD_DTIM_1CHAIN_RX_ID << HDD_SMPS_PARAM_VALUE_S,
				VDEV_CMD);

		if (ret != 0) {
			hddLog(VOS_TRACE_LEVEL_ERROR,
			       "%s: DTIM 1 chain set failed %d",
			       __func__, ret);
			goto err_post_add_adapter;
		}

		if (!hdd_ctx->per_band_chainmask_supp) {
			ret = process_wma_set_command((int)adapter->sessionId,
					(int)WMI_PDEV_PARAM_TX_CHAIN_MASK,
					(int)hdd_ctx->cfg_ini->txchainmask1x1,
					PDEV_CMD);
			if (ret != 0) {
				hddLog(VOS_TRACE_LEVEL_ERROR,
				       "%s: PDEV_PARAM_TX_CHAIN_MASK set failed %d",
				       __func__, ret);
				goto err_post_add_adapter;
			}
			ret = process_wma_set_command((int)adapter->sessionId,
					(int)WMI_PDEV_PARAM_RX_CHAIN_MASK,
					(int)hdd_ctx->cfg_ini->rxchainmask1x1,
					PDEV_CMD);
			if (ret != 0) {
				hddLog(VOS_TRACE_LEVEL_ERROR,
				       "%s: WMI_PDEV_PARAM_RX_CHAIN_MASK set failed %d",
				       __func__, ret);
				goto err_post_add_adapter;
			}
		}
#undef HDD_DTIM_1CHAIN_RX_ID
#undef HDD_SMPS_PARAM_VALUE_S
	}

	if (VOS_FTM_MODE != vos_get_conparam()) {
		uint32_t cca_threshold;

		cca_threshold = hdd_ctx->cfg_ini->cca_threshold_2g |
				hdd_ctx->cfg_ini->cca_threshold_5g << 8;

		if (hdd_ctx->cfg_ini->cca_threshold_enable) {
			hddLog(VOS_TRACE_LEVEL_DEBUG,
			       "%s: CCA Threshold is enabled.", __func__);
			ret = process_wma_set_command((int)adapter->sessionId,
					WMI_PDEV_PARAM_CCA_THRESHOLD,
					cca_threshold,
					PDEV_CMD);
		} else {
			ret = 0;
		}

		if (ret != 0) {
			hddLog(VOS_TRACE_LEVEL_ERROR,
			       "%s: WMI_PDEV_PARAM_CCA_THRESHOLD set failed %d",
			       __func__, ret);
			goto err_post_add_adapter;
		}

		ret = process_wma_set_command((int)adapter->sessionId,
			      (int)WMI_PDEV_PARAM_HYST_EN,
			      (int)hdd_ctx->cfg_ini->enableMemDeepSleep,
			      PDEV_CMD);
		if (ret != 0) {
			hddLog(VOS_TRACE_LEVEL_ERROR,
			       "%s: WMI_PDEV_PARAM_HYST_EN set failed %d",
			       __func__, ret);
			goto err_post_add_adapter;
		}

		hddLog(LOG1, FL("SET AMSDU num %d"),
		       hdd_ctx->cfg_ini->max_amsdu_num);

		ret = process_wma_set_command((int)adapter->sessionId,
				(int)GEN_VDEV_PARAM_AMSDU,
				(int)hdd_ctx->cfg_ini->max_amsdu_num,
				GEN_CMD);
		if (ret != 0) {
			hddLog(VOS_TRACE_LEVEL_ERROR,
			       FL("GEN_VDEV_PARAM_AMSDU set failed %d"), ret);
			goto err_post_add_adapter;
		}
	}

#ifdef CONFIG_FW_LOGS_BASED_ON_INI

	/* Enable FW logs based on INI configuration */
	if ((VOS_FTM_MODE != vos_get_conparam()) &&
	    (hdd_ctx->cfg_ini->enablefwlog) &&
	    (hdd_ctx->current_intf_count == 1)) {
		uint8_t count = 0;
		uint32_t value = 0;
		uint8_t num_entries = 0;
		uint8_t module_loglevel[FW_MODULE_LOG_LEVEL_STRING_LENGTH];

		hdd_ctx->fw_log_settings.dl_type =
					hdd_ctx->cfg_ini->enableFwLogType;
		ret = process_wma_set_command((int)adapter->sessionId,
					(int)WMI_DBGLOG_TYPE,
					hdd_ctx->cfg_ini->enableFwLogType,
					DBG_CMD);
		if (ret != 0)
			hddLog(LOGE,
			       FL("Failed to enable FW log type ret %d"), ret);

		hdd_ctx->fw_log_settings.dl_loglevel =
					hdd_ctx->cfg_ini->enableFwLogLevel;
		ret = process_wma_set_command((int)adapter->sessionId,
					(int)WMI_DBGLOG_LOG_LEVEL,
					hdd_ctx->cfg_ini->enableFwLogLevel,
					DBG_CMD);
		if (ret != 0)
			hddLog(LOGE,
			       FL("Failed to enable FW log level ret %d"),
			       ret);

		hdd_string_to_u8_array(hdd_ctx->cfg_ini->enableFwModuleLogLevel,
				       module_loglevel,
				       &num_entries,
				       FW_MODULE_LOG_LEVEL_STRING_LENGTH);

		while (count < num_entries) {
			/* FW module log level input string looks like below:
			 * gFwDebugModuleLoglevel=
			 *	<FW Module ID>, <Log Level>, so on....
			 * For example:
			 * gFwDebugModuleLoglevel=
			 *	1,0,2,1,3,2,4,3,5,4,6,5,7,6,8,7
			 * Above input string means :
			 * For FW module ID 1 enable log level 0
			 * For FW module ID 2 enable log level 1
			 * For FW module ID 3 enable log level 2
			 * For FW module ID 4 enable log level 3
			 * For FW module ID 5 enable log level 4
			 * For FW module ID 6 enable log level 5
			 * For FW module ID 7 enable log level 6
			 * For FW module ID 8 enable log level 7
			 *
			 * FW expects WMI command value =
			 *	Module ID * 10 + Module Log level
			 */
			value = ((module_loglevel[count] * 10) +
						module_loglevel[count + 1]);
			ret = process_wma_set_command((int)adapter->sessionId,
						(int)WMI_DBGLOG_MOD_LOG_LEVEL,
						value, DBG_CMD);
			if (ret != 0)
				hddLog(LOGE, FL("Failed to enable FW module log level %d ret %d"),
				       value, ret);

			count += 2;
		}
	}

#endif

	ret = process_wma_set_command((int)adapter->sessionId,
				      (int)WMI_VDEV_PARAM_ENABLE_RTSCTS,
				      hdd_ctx->cfg_ini->rts_profile, VDEV_CMD);
	if (ret != 0)
		hddLog(LOGE, "FAILED TO SET RTSCTS Profile ret:%d", ret);

	hdd_create_tsf_file(adapter);

	return adapter;

err_post_add_adapter:
	hdd_remove_adapter(hdd_ctx, hdd_adapter_node);

err_add_adapter_back:
	vos_mem_free(hdd_adapter_node);

err_malloc_adapter_node:
	if (rtnl_held)
		unregister_netdevice(adapter->dev);
	else
		unregister_netdev(adapter->dev);

err_register_interface:
	/* close sme session to detach vdev */
	hdd_stop_adapter(hdd_ctx, adapter, VOS_TRUE);
	hdd_deinit_adapter(hdd_ctx, adapter, rtnl_held);

err_init_adapter_mode:
	hdd_deinit_packet_filtering(adapter);

err_init_packet_filtering:
	hdd_adapter_runtime_suspend_denit(adapter);

	wlan_hdd_release_intf_addr(hdd_ctx,
				   adapter->macAddressCurrent.bytes);
	free_netdev(adapter->dev);

	/* If bmps disabled enable it */
	if (!hdd_ctx->cfg_ini->enablePowersaveOffload) {
		if (VOS_STATUS_SUCCESS == exit_bmps_status) {
			if (hdd_ctx->hdd_wlan_suspended)
				hdd_set_pwrparams(hdd_ctx);
			hdd_enable_bmps_imps(hdd_ctx);
		}
	}

	return NULL;
}

VOS_STATUS hdd_close_adapter( hdd_context_t *pHddCtx, hdd_adapter_t *pAdapter,
                              tANI_U8 rtnl_held )
{
   hdd_adapter_list_node_t *pAdapterNode, *pCurrent, *pNext;
   VOS_STATUS status;

   status = hdd_get_front_adapter ( pHddCtx, &pCurrent );
   if( VOS_STATUS_SUCCESS != status )
   {
      hddLog(VOS_TRACE_LEVEL_WARN,"%s: adapter list empty %d",
             __func__, status);
      return status;
   }

   while ( pCurrent->pAdapter != pAdapter )
   {
      status = hdd_get_next_adapter ( pHddCtx, pCurrent, &pNext );
      if( VOS_STATUS_SUCCESS != status )
         break;

      pCurrent = pNext;
   }
   pAdapterNode = pCurrent;
   if( VOS_STATUS_SUCCESS == status )
   {
      hdd_remove_tsf_file(pAdapter);
      wlan_hdd_clear_concurrency_mode(pHddCtx, pAdapter->device_mode);
      hdd_deinit_packet_filtering(pAdapterNode->pAdapter);
      hdd_cleanup_adapter( pHddCtx, pAdapterNode->pAdapter, rtnl_held );
      hdd_remove_adapter( pHddCtx, pAdapterNode );
      vos_mem_free( pAdapterNode );
      pAdapterNode = NULL;

      /* Adapter removed. Decrement vdev count */
      if (pHddCtx->current_intf_count != 0)
        pHddCtx->current_intf_count--;

      /*
       * If Powersave Offload is enabled,
       * Fw will take care incase of concurrency
       */
      if(pHddCtx->cfg_ini->enablePowersaveOffload)
          return VOS_STATUS_SUCCESS;

      /* If there is a single session of STA/P2P client, re-enable BMPS */
      if ((!vos_concurrent_open_sessions_running()) &&
           ((pHddCtx->no_of_open_sessions[VOS_STA_MODE] >= 1) ||
           (pHddCtx->no_of_open_sessions[VOS_P2P_CLIENT_MODE] >= 1)))
      {
          if (pHddCtx->hdd_wlan_suspended)
          {
              hdd_set_pwrparams(pHddCtx);
          }
          hdd_enable_bmps_imps(pHddCtx);
      }

      return VOS_STATUS_SUCCESS;
   }

   return VOS_STATUS_E_FAILURE;
}

VOS_STATUS hdd_close_all_adapters( hdd_context_t *pHddCtx )
{
   hdd_adapter_list_node_t *pHddAdapterNode;
   VOS_STATUS status;

   ENTER();

   do
   {
      status = hdd_remove_front_adapter( pHddCtx, &pHddAdapterNode );
      if( pHddAdapterNode && VOS_STATUS_SUCCESS == status )
      {
         hdd_deinit_packet_filtering(pHddAdapterNode->pAdapter);
         hdd_cleanup_adapter( pHddCtx, pHddAdapterNode->pAdapter, FALSE );
         vos_mem_free( pHddAdapterNode );
      }
   }while( NULL != pHddAdapterNode && VOS_STATUS_E_EMPTY != status );

   EXIT();

   return VOS_STATUS_SUCCESS;
}

void wlan_hdd_reset_prob_rspies(hdd_adapter_t* pHostapdAdapter)
{
    tANI_U8 *bssid = NULL;
    tSirUpdateIE updateIE;
    switch (pHostapdAdapter->device_mode)
    {
    case WLAN_HDD_INFRA_STATION:
    case WLAN_HDD_P2P_CLIENT:
    {
        hdd_station_ctx_t * pHddStaCtx =
            WLAN_HDD_GET_STATION_CTX_PTR(pHostapdAdapter);
        bssid = (tANI_U8*)&pHddStaCtx->conn_info.bssId;
        break;
    }
    case WLAN_HDD_SOFTAP:
    case WLAN_HDD_P2P_GO:
    case WLAN_HDD_IBSS:
    {
        bssid = pHostapdAdapter->macAddressCurrent.bytes;
        break;
    }
    case WLAN_HDD_MONITOR:
    case WLAN_HDD_FTM:
    case WLAN_HDD_P2P_DEVICE:
    default:
        /*
         * wlan_hdd_reset_prob_rspies should not have been called
         * for these kind of devices
         */
        hddLog(LOGE, FL("Unexpected request for the current device type %d"),
               pHostapdAdapter->device_mode);
        return;
    }

    vos_mem_copy(updateIE.bssid, bssid, sizeof(tSirMacAddr));
        updateIE.smeSessionId =  pHostapdAdapter->sessionId;
        updateIE.ieBufferlength = 0;
        updateIE.pAdditionIEBuffer = NULL;
        updateIE.append = VOS_TRUE;
        updateIE.notify = VOS_FALSE;
    if (sme_UpdateAddIE(WLAN_HDD_GET_HAL_CTX(pHostapdAdapter),
            &updateIE, eUPDATE_IE_PROBE_RESP) == eHAL_STATUS_FAILURE) {
        hddLog(LOGE, FL("Could not pass on PROBE_RSP_BCN data to PE"));
    }
}

/**
 * hdd_wait_for_sme_close_sesion() - Close and wait for SME session close
 * @hdd_ctx: HDD context which is already NULL validated
 * @adapter: HDD adapter which is already NULL validated
 *
 * Close the SME session and wait for its completion, if needed.
 *
 * Return: None
 */
static void hdd_wait_for_sme_close_sesion(hdd_context_t *hdd_ctx,
        hdd_adapter_t *adapter)
{
    unsigned long rc;

    if (!test_bit(SME_SESSION_OPENED, &adapter->event_flags)) {
        hddLog(LOGE, FL("session is not opened:%d"), adapter->sessionId);
        return;
    }

    INIT_COMPLETION(adapter->session_close_comp_var);
    if (eHAL_STATUS_SUCCESS ==
            sme_CloseSession(hdd_ctx->hHal, adapter->sessionId,
                hdd_smeCloseSessionCallback,
                adapter)) {
#ifdef WLAN_FEATURE_USB_RECOVERY
        if ( hdd_in_recovery_state() ) {
            printk(KERN_ERR "%s ignore wait session close\n",__func__);
            return;
        }
#endif
        /*
         * Block on a completion variable. Can't wait
         * forever though.
         */
        rc = wait_for_completion_timeout(
                &adapter->session_close_comp_var,
                msecs_to_jiffies
                (WLAN_WAIT_TIME_SESSIONOPENCLOSE));
        if (!rc)
            hddLog(LOGE, FL("failure waiting for session_close_comp_var"));
    }
}

VOS_STATUS hdd_stop_adapter( hdd_context_t *pHddCtx, hdd_adapter_t *pAdapter,
                             const v_BOOL_t bCloseSession)
{
   eHalStatus halStatus = eHAL_STATUS_SUCCESS;
   hdd_wext_state_t *pWextState = WLAN_HDD_GET_WEXT_STATE_PTR(pAdapter);
   hdd_scaninfo_t *pScanInfo = NULL;
   union iwreq_data wrqu;
   tSirUpdateIE updateIE ;
   unsigned long rc;

   ENTER();

   pScanInfo = &pAdapter->scan_info;

   hddLog(LOG1, FL("Disabling queues"));
   wlan_hdd_netif_queue_control(pAdapter, WLAN_NETIF_TX_DISABLE_N_CARRIER,
                                    WLAN_CONTROL_PATH);
   switch(pAdapter->device_mode)
   {
      case WLAN_HDD_INFRA_STATION:
      case WLAN_HDD_IBSS:
      case WLAN_HDD_P2P_CLIENT:
      case WLAN_HDD_P2P_DEVICE:
      case WLAN_HDD_NDI:
         if ((WLAN_HDD_NDI == pAdapter->device_mode) ||
            hdd_connIsConnected(WLAN_HDD_GET_STATION_CTX_PTR(pAdapter)) ||
            hdd_is_connecting(WLAN_HDD_GET_STATION_CTX_PTR(pAdapter))) {
#ifdef WLAN_FEATURE_USB_RECOVERY
            if ( hdd_in_recovery_state() ) {
                wlan_hdd_cfg80211_indicate_disconnect(pAdapter->dev, false,
                                                      WLAN_REASON_UNSPECIFIED);
                goto _ignore_disconnect;
            }
#endif
            INIT_COMPLETION(pAdapter->disconnect_comp_var);
            /*
             * For NDI do not use pWextState from sta_ctx, if needed
             * extract from ndi_ctx.
             */
            if (WLAN_HDD_NDI == pAdapter->device_mode)
                halStatus = sme_RoamDisconnect(pHddCtx->hHal,
                                           pAdapter->sessionId,
                                           eCSR_DISCONNECT_REASON_NDI_DELETE);
            else if (pWextState->roamProfile.BSSType == eCSR_BSS_TYPE_START_IBSS)
                halStatus = sme_RoamDisconnect(pHddCtx->hHal,
                                             pAdapter->sessionId,
                                             eCSR_DISCONNECT_REASON_IBSS_LEAVE);
            else
                halStatus = sme_RoamDisconnect(pHddCtx->hHal,
                                            pAdapter->sessionId,
                                            eCSR_DISCONNECT_REASON_UNSPECIFIED);
            //success implies disconnect command got queued up successfully
            if(halStatus == eHAL_STATUS_SUCCESS)
            {
               rc = wait_for_completion_timeout(
                          &pAdapter->disconnect_comp_var,
                          msecs_to_jiffies(WLAN_WAIT_TIME_DISCONNECT));
               if (!rc) {
                  hddLog(VOS_TRACE_LEVEL_ERROR,
                         "%s: wait on disconnect_comp_var failed",
                         __func__);
               }
           }
           else
           {
               hddLog(LOGE, "%s: failed to post disconnect event to SME",
                      __func__);
           }
#ifdef WLAN_FEATURE_USB_RECOVERY
_ignore_disconnect:
#endif
           memset(&wrqu, '\0', sizeof(wrqu));
           wrqu.ap_addr.sa_family = ARPHRD_ETHER;
           memset(wrqu.ap_addr.sa_data,'\0',ETH_ALEN);
           wireless_send_event(pAdapter->dev, SIOCGIWAP, &wrqu, NULL);
         }

         if (pScanInfo != NULL && pScanInfo->mScanPending)
         {
            wlan_hdd_scan_abort(pAdapter);
         }
         if ((pAdapter->device_mode == WLAN_HDD_P2P_CLIENT) ||
              (pAdapter->device_mode == WLAN_HDD_P2P_DEVICE)) {
             wlan_hdd_cleanup_remain_on_channel_ctx(pAdapter);
         }

#ifdef WLAN_OPEN_SOURCE
         cancel_work_sync(&pAdapter->ipv4NotifierWorkQueue);
#endif

         wlan_hdd_clean_tx_flow_control_timer(pHddCtx, pAdapter);

#ifdef WLAN_NS_OFFLOAD
#ifdef WLAN_OPEN_SOURCE
         cancel_work_sync(&pAdapter->ipv6NotifierWorkQueue);
#endif
#endif

         /* It is possible that the caller of this function does not
          * wish to close the session
          */
         if (bCloseSession)
             hdd_wait_for_sme_close_sesion(pHddCtx, pAdapter);
         break;

      case WLAN_HDD_SOFTAP:
      case WLAN_HDD_P2P_GO:
         if (pHddCtx->cfg_ini->conc_custom_rule1 &&
             (WLAN_HDD_SOFTAP == pAdapter->device_mode)) {
             /*
              * Before stopping the sap adapter, lets make sure there is
              * no sap restart work pending.
              */
             vos_flush_work(&pHddCtx->sap_start_work);
             VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
                        FL("Canceled the pending SAP restart work"));
             hdd_change_ch_avoidance_status(pHddCtx, false);
             hdd_change_sap_restart_required_status(pHddCtx, false);
         }
         //Any softap specific cleanup here...
         if (pAdapter->device_mode == WLAN_HDD_P2P_GO) {
             wlan_hdd_cleanup_remain_on_channel_ctx(pAdapter);
         }

         hdd_set_sap_auth_offload(pAdapter, FALSE);

         wlan_hdd_clean_tx_flow_control_timer(pHddCtx, pAdapter);

         mutex_lock(&pHddCtx->sap_lock);
         if (test_bit(SOFTAP_BSS_STARTED, &pAdapter->event_flags))
         {
            VOS_STATUS status;
            hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
            hdd_hostapd_state_t *pHostapdState =
                  WLAN_HDD_GET_HOSTAP_STATE_PTR(pAdapter);
            vos_event_reset(&pHostapdState->stop_bss_event);

            //Stop Bss.
#ifdef WLAN_FEATURE_MBSSID
            status = WLANSAP_StopBss(WLAN_HDD_GET_SAP_CTX_PTR(pAdapter));
#else
            status = WLANSAP_StopBss(pHddCtx->pvosContext);
#endif

            if (VOS_IS_STATUS_SUCCESS(status))
            {
               status = vos_wait_single_event(&pHostapdState->stop_bss_event,
                                              10000);
               if (!VOS_IS_STATUS_SUCCESS(status))
               {
                  hddLog(LOGE, "%s: failure waiting for WLANSAP_StopBss %d",
                         __func__, status);
               }
            }
            else
            {
               hddLog(LOGE, "%s: failure in WLANSAP_StopBss", __func__);
            }
            clear_bit(SOFTAP_BSS_STARTED, &pAdapter->event_flags);
            wlan_hdd_decr_active_session(pHddCtx, pAdapter->device_mode);

            vos_mem_copy(updateIE.bssid, pAdapter->macAddressCurrent.bytes,
                   sizeof(tSirMacAddr));
            updateIE.smeSessionId = pAdapter->sessionId;
            updateIE.ieBufferlength = 0;
            updateIE.pAdditionIEBuffer = NULL;
            updateIE.append = VOS_FALSE;
            updateIE.notify = VOS_FALSE;
            if (pHddCtx->cfg_ini->apOBSSProtEnabled)
                vos_runtime_pm_allow_suspend(pHddCtx->runtime_context.obss);
            /* Probe bcn reset */
            if (sme_UpdateAddIE(WLAN_HDD_GET_HAL_CTX(pAdapter),
                              &updateIE, eUPDATE_IE_PROBE_BCN)
                              == eHAL_STATUS_FAILURE) {
                hddLog(LOGE, FL("Could not pass on PROBE_RSP_BCN data to PE"));
            }
            /* Assoc resp reset */
            if (sme_UpdateAddIE(WLAN_HDD_GET_HAL_CTX(pAdapter),
                    &updateIE, eUPDATE_IE_ASSOC_RESP) == eHAL_STATUS_FAILURE) {
                hddLog(LOGE, FL("Could not pass on ASSOC_RSP data to PE"));
            }

            // Reset WNI_CFG_PROBE_RSP Flags
            wlan_hdd_reset_prob_rspies(pAdapter);
            kfree(pAdapter->sessionCtx.ap.beacon);
            pAdapter->sessionCtx.ap.beacon = NULL;
#ifdef WLAN_FEATURE_SAP_TO_FOLLOW_STA_CHAN
            if (pAdapter->device_mode == WLAN_HDD_SOFTAP)
            {
                if(pHddCtx->ch_switch_ctx.chan_sw_timer_initialized == VOS_TRUE)
                {
                //Stop the channel switch timer
                    if (VOS_TIMER_STATE_RUNNING ==
			    vos_timer_getCurrentState(&pHddCtx->ch_switch_ctx.hdd_ap_chan_switch_timer))
		    {
			    vos_timer_stop(&pHddCtx->ch_switch_ctx.hdd_ap_chan_switch_timer);
		    }
			    //Destroy the channel switch timer
		    if (!VOS_IS_STATUS_SUCCESS(vos_timer_destroy(
					&pHddCtx->ch_switch_ctx.hdd_ap_chan_switch_timer)))
		    {
			    hddLog(LOGE, FL("Failed to destroy AP channel switch timer!!"));
		    }
		    pHddCtx->ch_switch_ctx.chan_sw_timer_initialized = VOS_FALSE;
                }
             }
#endif //WLAN_FEATURE_SAP_TO_FOLLOW_STA_CHAN
         }
         mutex_unlock(&pHddCtx->sap_lock);

         cancel_work_sync(&pAdapter->ipv4NotifierWorkQueue);

#ifdef WLAN_NS_OFFLOAD
         cancel_work_sync(&pAdapter->ipv6NotifierWorkQueue);
#endif
#ifdef FEATURE_WLAN_DISABLE_CHANNEL_SWITCH
         /*
          * If Do_Not_Break_Stream was enabled clear avoid channel list.
          */
         if (pHddCtx->restrict_offchan_flag)
             wlan_hdd_send_avoid_freq_for_dnbs(pHddCtx, 0);
#endif
         if (bCloseSession)
             hdd_wait_for_sme_close_sesion(pHddCtx, pAdapter);
         break;

      case WLAN_HDD_OCB:
         wlan_hdd_dsrc_deinit_chan_stats(pAdapter);
         hdd_disconnect_tx_rx(pAdapter);
         WLANTL_ClearSTAClient(WLAN_HDD_GET_CTX(pAdapter)->pvosContext,
            WLAN_HDD_GET_STATION_CTX_PTR(pAdapter)->conn_info.staId[0]);
         break;

      case WLAN_HDD_MONITOR:
         hdd_vir_mon_stop(pAdapter->dev);
         break;

      default:
         break;
   }

   EXIT();
   return VOS_STATUS_SUCCESS;
}

VOS_STATUS hdd_stop_all_adapters( hdd_context_t *pHddCtx )
{
   hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
   VOS_STATUS status;
   hdd_adapter_t      *pAdapter;

   ENTER();

   status = hdd_get_front_adapter ( pHddCtx, &pAdapterNode );

   while ( NULL != pAdapterNode && VOS_STATUS_SUCCESS == status )
   {
      pAdapter = pAdapterNode->pAdapter;
      hdd_stop_adapter( pHddCtx, pAdapter, VOS_TRUE );
      status = hdd_get_next_adapter ( pHddCtx, pAdapterNode, &pNext );
      pAdapterNode = pNext;
   }

   EXIT();

   return VOS_STATUS_SUCCESS;
}

#ifdef QCA_LL_TX_FLOW_CT
/**
 * hdd_adapter_abort_tx_flow() - Abort the tx flow control
 * @pAdapter: pointer to hdd_adapter_t
 *
 * Resume tx and stop the tx flow control timer if the tx is paused and the flow
 * control timer is running. This function is called by SSR to avoid the
 * inconsistency of tx status before and after SSR.
 *
 * Return: void
 */
static void hdd_adapter_abort_tx_flow(hdd_adapter_t *pAdapter)
{
	if ((pAdapter->hdd_stats.hddTxRxStats.is_txflow_paused == TRUE) &&
		(VOS_TIMER_STATE_RUNNING ==
		vos_timer_getCurrentState(&pAdapter->tx_flow_control_timer))) {
		hdd_tx_resume_timer_expired_handler(pAdapter);
		vos_timer_stop(&pAdapter->tx_flow_control_timer);
	}
}
#else
static void hdd_adapter_abort_tx_flow(hdd_adapter_t *pAdapter)
{
	return;
}
#endif

VOS_STATUS hdd_reset_all_adapters( hdd_context_t *pHddCtx )
{
   hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
   VOS_STATUS status;
   hdd_adapter_t *pAdapter;

   ENTER();

   status =  hdd_get_front_adapter ( pHddCtx, &pAdapterNode );

   while ( NULL != pAdapterNode && VOS_STATUS_SUCCESS == status )
   {
      pAdapter = pAdapterNode->pAdapter;

      hddLog(LOG1, FL("Disabling queues"));

      hdd_adapter_abort_tx_flow(pAdapter);

      if (pHddCtx->cfg_ini->sap_internal_restart &&
          pAdapter->device_mode == WLAN_HDD_SOFTAP) {
          hddLog(LOG1, FL("driver supports sap restart"));
          vos_flush_work(&pHddCtx->sap_start_work);
          wlan_hdd_netif_queue_control(pAdapter,
                                 WLAN_NETIF_TX_DISABLE,
                                  WLAN_CONTROL_PATH);
          hdd_sap_indicate_disconnect_for_sta(pAdapter);
          hdd_cleanup_actionframe(pHddCtx, pAdapter);
          hdd_sap_destroy_events(pAdapter);

      } else
          wlan_hdd_netif_queue_control(pAdapter,
              WLAN_NETIF_TX_DISABLE_N_CARRIER,
              WLAN_CONTROL_PATH);

      pAdapter->sessionCtx.station.hdd_ReassocScenario = VOS_FALSE;

      hdd_deinit_tx_rx(pAdapter);
      wlan_hdd_decr_active_session(pHddCtx, pAdapter->device_mode);
      if (test_bit(WMM_INIT_DONE, &pAdapter->event_flags))
      {
          hdd_wmm_adapter_close( pAdapter );
          clear_bit(WMM_INIT_DONE, &pAdapter->event_flags);
      }

      /*
       * If adapter is SAP, set session ID to invalid since SAP
       * session will be cleanup during SSR.
       */
      if (pAdapter->device_mode == WLAN_HDD_SOFTAP)
          wlansap_set_invalid_session(
#ifdef WLAN_FEATURE_MBSSID
                  WLAN_HDD_GET_SAP_CTX_PTR(pAdapter));
#else
                  (WLAN_HDD_GET_CTX(pAdapter))->pvosContext);
#endif

      status = hdd_get_next_adapter ( pHddCtx, pAdapterNode, &pNext );
      pAdapterNode = pNext;
   }

   EXIT();

   return VOS_STATUS_SUCCESS;
}

/**
 * hdd_get_bss_entry() - Get the bss entry matching the chan, bssid and ssid
 * @wiphy: wiphy
 * @channel: channel of the BSS to find
 * @bssid: bssid of the BSS to find
 * @ssid: ssid of the BSS to find
 * @ssid_len: ssid len of of the BSS to find
 *
 * The API is a wrapper to get bss from kernel matching the chan,
 * bssid and ssid
 *
 * Return: bss structure if found else NULL
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)) \
	&& !defined(WITH_BACKPORTS) && !defined(IEEE80211_PRIVACY)
struct cfg80211_bss *hdd_cfg80211_get_bss(struct wiphy *wiphy,
	  struct ieee80211_channel *channel,
	  const u8 *bssid, const u8 *ssid,
	  size_t ssid_len)
{
	return cfg80211_get_bss(wiphy, channel, bssid,
			ssid, ssid_len,
			WLAN_CAPABILITY_ESS,
			WLAN_CAPABILITY_ESS);
}
#else
struct cfg80211_bss *hdd_cfg80211_get_bss(struct wiphy *wiphy,
	  struct ieee80211_channel *channel,
	  const u8 *bssid, const u8 *ssid,
	  size_t ssid_len)
{
	return cfg80211_get_bss(wiphy, channel, bssid,
			ssid, ssid_len,
			IEEE80211_BSS_TYPE_ESS,
			IEEE80211_PRIVACY_ANY);
}
#endif

#if defined CFG80211_CONNECT_BSS
#if defined CFG80211_CONNECT_TIMEOUT_REASON_CODE || \
	(LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
/**
 * hdd_convert_timeout_reason() - Convert to kernel specific enum
 * @timeout_reason: reason for connect timeout
 *
 * This function is used to convert host timeout
 * reason enum to kernel specific enum.
 *
 * Return: nl timeout enum
 */
static enum nl80211_timeout_reason hdd_convert_timeout_reason(
	  tSirResultCodes timeout_reason)
{
	switch (timeout_reason) {
	case eSIR_SME_JOIN_TIMEOUT_RESULT_CODE:
		return NL80211_TIMEOUT_SCAN;
	case eSIR_SME_AUTH_TIMEOUT_RESULT_CODE:
		return NL80211_TIMEOUT_AUTH;
	case eSIR_SME_ASSOC_TIMEOUT_RESULT_CODE:
		return NL80211_TIMEOUT_ASSOC;
	default:
		return NL80211_TIMEOUT_UNSPECIFIED;
	}
}

#if defined CFG80211_CONNECT_TIMEOUT
/**
 * hdd_cfg80211_connect_timeout() - API to send connection timeout reason
 * @dev: network device
 * @bssid: bssid to which we want to associate
 * @timeout_reason: reason for connect timeout
 *
 * This API is used to send connection timeout reason to supplicant
 *
 * Return: void
 */
static void hdd_cfg80211_connect_timeout(struct net_device *dev,
					 const u8 *bssid,
					 tSirResultCodes timeout_reason)
{
	enum nl80211_timeout_reason nl_timeout_reason;
	nl_timeout_reason = hdd_convert_timeout_reason(timeout_reason);

	cfg80211_connect_timeout(dev, bssid, NULL, 0, GFP_KERNEL,
				 nl_timeout_reason);
}
#endif /* CFG80211_CONNECT_TIMEOUT */

/**
 * __hdd_connect_bss() - API to send connection status to supplicant
 * @dev: network device
 * @bssid: bssid to which we want to associate
 * @req_ie: Request Information Element
 * @req_ie_len: len of the req IE
 * @resp_ie: Response IE
 * @resp_ie_len: len of ht response IE
 * @status: status
 * @gfp: Kernel Flag
 * @timeout_reason: reason for connect timeout
 *
 * Return: void
 */
static void __hdd_connect_bss(struct net_device *dev, const u8 *bssid,
			      struct cfg80211_bss *bss, const u8 *req_ie,
			      size_t req_ie_len, const u8 *resp_ie,
			      size_t resp_ie_len, int status, gfp_t gfp,
			      tSirResultCodes timeout_reason)
{
	enum nl80211_timeout_reason nl_timeout_reason;
	nl_timeout_reason = hdd_convert_timeout_reason(timeout_reason);

	cfg80211_connect_bss(dev, bssid, bss, req_ie, req_ie_len,
			     resp_ie, resp_ie_len, status, gfp,
			     nl_timeout_reason);
}
#else /* CFG80211_CONNECT_TIMEOUT_REASON_CODE */
#if defined CFG80211_CONNECT_TIMEOUT
static void hdd_cfg80211_connect_timeout(struct net_device *dev,
					 const u8 *bssid,
					 tSirResultCodes timeout_reason)
{
	cfg80211_connect_timeout(dev, bssid, NULL, 0, GFP_KERNEL);
}
#endif /* CFG80211_CONNECT_TIMEOUT */
static void __hdd_connect_bss(struct net_device *dev, const u8 *bssid,
			      struct cfg80211_bss *bss, const u8 *req_ie,
			      size_t req_ie_len, const u8 *resp_ie,
			      size_t resp_ie_len, int status, gfp_t gfp,
			      tSirResultCodes timeout_reason)
{
	cfg80211_connect_bss(dev, bssid, bss, req_ie, req_ie_len,
			     resp_ie, resp_ie_len, status, gfp);
}
#endif /* CFG80211_CONNECT_TIMEOUT_REASON_CODE */
/**
 * hdd_connect_bss() - API to send connection status to supplicant
 * @dev: network device
 * @bssid: bssid to which we want to associate
 * @req_ie: Request Information Element
 * @req_ie_len: len of the req IE
 * @resp_ie: Response IE
 * @resp_ie_len: len of ht response IE
 * @status: status
 * @gfp: Kernel Flag
 * @connect_timeout: If timed out waiting for Auth/Assoc/Probe resp
 * @timeout_reason: reason for connect timeout
 *
 * The API is a wrapper to send connection status to supplicant
 *
 * Return: Void
 */
#if defined CFG80211_CONNECT_TIMEOUT
static void hdd_connect_bss(struct net_device *dev, const u8 *bssid,
			struct cfg80211_bss *bss, const u8 *req_ie,
			size_t req_ie_len, const u8 *resp_ie,
			size_t resp_ie_len, int status, gfp_t gfp,
			bool connect_timeout, tSirResultCodes timeout_reason)
{
	if (connect_timeout)
		hdd_cfg80211_connect_timeout(dev, bssid, timeout_reason);
	else
		__hdd_connect_bss(dev, bssid, bss, req_ie, req_ie_len,
			resp_ie, resp_ie_len, status, gfp, timeout_reason);
}
#else
static void hdd_connect_bss(struct net_device *dev, const u8 *bssid,
			struct cfg80211_bss *bss, const u8 *req_ie,
			size_t req_ie_len, const u8 *resp_ie,
			size_t resp_ie_len, int status, gfp_t gfp,
			bool connect_timeout, tSirResultCodes timeout_reason)
{
	__hdd_connect_bss(dev, bssid, bss, req_ie, req_ie_len,
		resp_ie, resp_ie_len, status, gfp, timeout_reason);
}
#endif

#ifdef WLAN_FEATURE_FILS_SK
#if (defined(CFG80211_CONNECT_DONE) || \
	(LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))) && \
	(LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0))
#if defined(CFG80211_FILS_SK_OFFLOAD_SUPPORT) || \
		 (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))
/**
 * hdd_populate_fils_params() - Populate FILS keys to connect response
 * @fils_params: connect response to supplicant
 * @fils_kek: FILS kek
 * @fils_kek_len: FILS kek length
 * @pmk: FILS PMK
 * @pmk_len: FILS PMK length
 * @pmkid: PMKID
 * @fils_seq_num: FILS Seq number
 *
 * Return: None
 */
static void hdd_populate_fils_params(struct cfg80211_connect_resp_params
                     *fils_params, const uint8_t *fils_kek,
                     size_t fils_kek_len, const uint8_t *pmk,
                     size_t pmk_len, const uint8_t *pmkid,
                     uint16_t fils_seq_num)
{
    /* Increament seq number to be used for next FILS */
    fils_params->fils_erp_next_seq_num = fils_seq_num + 1;
    fils_params->update_erp_next_seq_num = true;
    fils_params->fils_kek = fils_kek;
    fils_params->fils_kek_len = fils_kek_len;
    fils_params->pmk = pmk;
    fils_params->pmk_len = pmk_len;
    fils_params->pmkid = pmkid;
}
#else /* CFG80211_FILS_SK_OFFLOAD_SUPPORT */
static inline void hdd_populate_fils_params(struct cfg80211_connect_resp_params
                        *fils_params, const uint8_t
                        *fils_kek, size_t fils_kek_len,
                        const uint8_t *pmk, size_t pmk_len,
                        const uint8_t *pmkid,
                        uint16_t fils_seq_num)
{ }
#endif /* CFG80211_FILS_SK_OFFLOAD_SUPPORT */

#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
/**
 * hdd_populate_fils_params() - Populate FILS keys to connect response
 * @fils_params: connect response to supplicant
 * @fils_kek: FILS kek
 * @fils_kek_len: FILS kek length
 * @pmk: FILS PMK
 * @pmk_len: FILS PMK length
 * @pmkid: PMKID
 * @fils_seq_num: FILS Seq number
 *
 * Return: None
 */
static void hdd_populate_fils_params(struct cfg80211_connect_resp_params
				     *fils_params, const uint8_t *fils_kek,
				     size_t fils_kek_len, const uint8_t *pmk,
				     size_t pmk_len, const uint8_t *pmkid,
				     uint16_t fils_seq_num)
{
	/* Increament seq number to be used for next FILS */
	fils_params->fils.erp_next_seq_num = fils_seq_num + 1;
	fils_params->fils.update_erp_next_seq_num = true;
	fils_params->fils.kek = fils_kek;
	fils_params->fils.kek_len = fils_kek_len;
	fils_params->fils.pmk = pmk;
	fils_params->fils.pmk_len = pmk_len;
	fils_params->fils.pmkid = pmkid;
}
#endif /* CFG80211_CONNECT_DONE */

#if defined(CFG80211_CONNECT_DONE) || \
	(LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))

/**
 * hdd_connect_done() - Wrapper API to call cfg80211_connect_done
 * @dev: network device
 * @bssid: bssid to which we want to associate
 * @bss: cfg80211 bss info
 * @roam_info: information about connected bss
 * @req_ie: Request Information Element
 * @req_ie_len: len of the req IE
 * @resp_ie: Response IE
 * @resp_ie_len: len of ht response IE
 * @status: status
 * @gfp: allocation flags
 * @connect_timeout: If timed out waiting for Auth/Assoc/Probe resp
 * @timeout_reason: reason for connect timeout
 * @roam_fils_params: FILS join response params
 *
 * This API is used as wrapper to send FILS key/sequence number
 * params etc. to supplicant in case of FILS connection
 *
 * Return: None
 */
static void hdd_connect_done(struct net_device *dev, const u8 *bssid,
                 struct cfg80211_bss *bss, tCsrRoamInfo *roam_info,
                 const u8 *req_ie, size_t req_ie_len,
                 const u8 *resp_ie, size_t resp_ie_len, u16 status,
                 gfp_t gfp, bool connect_timeout, tSirResultCodes
                 timeout_reason, struct fils_join_rsp_params
                 *roam_fils_params)
{
    struct cfg80211_connect_resp_params fils_params;
    vos_mem_zero(&fils_params, sizeof(fils_params));

#ifdef CFG80211_SINGLE_NETDEV_MULTI_LINK_SUPPORT
    fils_params.links[0].bssid = bssid;
#else
    fils_params.bssid = bssid;
#endif
    if (!roam_fils_params) {
        fils_params.status = WLAN_STATUS_UNSPECIFIED_FAILURE;
        hdd_populate_fils_params(&fils_params, NULL, 0, NULL,
                     0, NULL, roam_info->fils_seq_num);
    } else {
        fils_params.status = status;
        fils_params.timeout_reason = timeout_reason;
        fils_params.req_ie = req_ie;
        fils_params.req_ie_len = req_ie_len;
        fils_params.resp_ie = resp_ie;
        fils_params.resp_ie_len = resp_ie_len;
#ifdef CFG80211_SINGLE_NETDEV_MULTI_LINK_SUPPORT
        fils_params.links[0].bss = bss;
#else
        fils_params.bss = bss;
#endif
        hdd_populate_fils_params(&fils_params, roam_fils_params->kek,
                     roam_fils_params->kek_len,
                     roam_fils_params->fils_pmk,
                     roam_fils_params->fils_pmk_len,
                     roam_fils_params->fils_pmkid,
                     roam_info->fils_seq_num);
    }
    hddLog(LOG1, "FILS indicate connect status %d",
          fils_params.status);

    cfg80211_connect_done(dev, &fils_params, gfp);

    /* Clear all the FILS key info */
    if (roam_fils_params && roam_fils_params->fils_pmk)
        vos_mem_free(roam_fils_params->fils_pmk);
    if (roam_fils_params)
        vos_mem_free(roam_fils_params);
    roam_info->fils_join_rsp = NULL;
}
#else /* CFG80211_CONNECT_DONE */
static inline void hdd_connect_done(struct net_device *dev, const u8 *bssid,
                    struct cfg80211_bss *bss, tCsrRoamInfo
                    *roam_info, const u8 *req_ie,
                    size_t req_ie_len, const u8 *resp_ie,
                    size_t resp_ie_len, u16 status, gfp_t gfp,
                    bool connect_timeout, tSirResultCodes
                    timeout_reason, struct fils_join_rsp_params
                    *roam_fils_params)
{ }
#endif /* CFG80211_CONNECT_DONE */
#endif /* WLAN_FEATURE_FILS_SK */

#if defined(CFG80211_CONNECT_DONE) && defined(WLAN_FEATURE_FILS_SK)
/**
 * hdd_fils_update_connect_results() - API to send fils connection status to
 * supplicant.
 * @dev: network device
 * @bssid: bssid to which we want to associate
 * @bss: cfg80211 bss info
 * @roam_info: information about connected bss
 * @req_ie: Request Information Element
 * @req_ie_len: len of the req IE
 * @resp_ie: Response IE
 * @resp_ie_len: len of ht response IE
 * @status: status
 * @gfp: allocation flags
 * @connect_timeout: If timed out waiting for Auth/Assoc/Probe resp
 * @timeout_reason: reason for connect timeout
 *
 * The API is a wrapper to send connection status to supplicant
 *
 * Return: 0 if success else failure
 */
static int hdd_fils_update_connect_results(struct net_device *dev,
            const u8 *bssid,
            struct cfg80211_bss *bss,
            tCsrRoamInfo *roam_info, const u8 *req_ie,
            size_t req_ie_len, const u8 *resp_ie,
            size_t resp_ie_len, u16 status, gfp_t gfp,
            bool connect_timeout,
            tSirResultCodes timeout_reason)
{
    ENTER();
    if (!roam_info || !roam_info->is_fils_connection)
        return -EINVAL;

    hdd_connect_done(dev, bssid, bss, roam_info, req_ie, req_ie_len,
             resp_ie, resp_ie_len, status, gfp, connect_timeout,
             timeout_reason, roam_info->fils_join_rsp);
    return 0;
}
#else /* CFG80211_CONNECT_DONE && WLAN_FEATURE_FILS_SK */
static inline int hdd_fils_update_connect_results(struct net_device *dev,
            const u8 *bssid,
            struct cfg80211_bss *bss,
            tCsrRoamInfo *roam_info, const u8 *req_ie,
            size_t req_ie_len, const u8 *resp_ie,
            size_t resp_ie_len, u16 status, gfp_t gfp,
            bool connect_timeout,
            tSirResultCodes timeout_reason)
{
    return -EINVAL;
}
#endif /* CFG80211_CONNECT_DONE && WLAN_FEATURE_FILS_SK */

/**
 * hdd_connect_result() - API to send connection status to supplicant
 * @dev: network device
 * @bssid: bssid to which we want to associate
 * @roam_info: information about connected bss
 * @req_ie: Request Information Element
 * @req_ie_len: len of the req IE
 * @resp_ie: Response IE
 * @resp_ie_len: len of ht response IE
 * @status: status
 * @gfp: Kernel Flag
 * @connect_timeout: If timed out waiting for Auth/Assoc/Probe resp
 * @timeout_reason: reason for connect timeout
 *
 * The API is a wrapper to send connection status to supplicant
 * and allow runtime suspend
 *
 * Return: Void
 */
void hdd_connect_result(struct net_device *dev,
			const u8 *bssid,
			tCsrRoamInfo *roam_info,
			const u8 *req_ie,
			size_t req_ie_len,
			const u8 *resp_ie,
			size_t resp_ie_len,
			u16 status,
			gfp_t gfp,
			bool connect_timeout, tSirResultCodes timeout_reason)
{
	hdd_adapter_t *padapter = (hdd_adapter_t *) netdev_priv(dev);
	struct cfg80211_bss *bss = NULL;
	if (WLAN_STATUS_SUCCESS == status) {
		struct ieee80211_channel *chan;
		int freq;
		int chan_no = roam_info->pBssDesc->channelId;;

		if (chan_no <= 14)
			freq = ieee80211_channel_to_frequency(chan_no,
						IEEE80211_BAND_2GHZ);
		else
			freq = ieee80211_channel_to_frequency(chan_no,
						IEEE80211_BAND_5GHZ);

		chan = ieee80211_get_channel(padapter->wdev.wiphy, freq);
		bss = hdd_cfg80211_get_bss(padapter->wdev.wiphy, chan, bssid,
			roam_info->u.pConnectedProfile->SSID.ssId,
			roam_info->u.pConnectedProfile->SSID.length);
	}

	if (hdd_fils_update_connect_results(dev, bssid, bss,
			roam_info, req_ie, req_ie_len, resp_ie,
			resp_ie_len, status, gfp, connect_timeout,
			timeout_reason) != 0) {
			hdd_connect_bss(dev, bssid, bss, req_ie,
				req_ie_len, resp_ie, resp_ie_len,
				status, gfp, connect_timeout, timeout_reason);
	}

	vos_runtime_pm_allow_suspend(padapter->runtime_context.connect);
}
#else /* CFG80211_CONNECT_BSS */
void hdd_connect_result(struct net_device *dev,
			const u8 *bssid,
			tCsrRoamInfo *roam_info,
			const u8 *req_ie,
			size_t req_ie_len,
			const u8 * resp_ie,
			size_t resp_ie_len,
			u16 status,
			gfp_t gfp,
			bool connect_timeout, tSirResultCodes timeout_reason)
{
	hdd_adapter_t *padapter = (hdd_adapter_t *) netdev_priv(dev);

	cfg80211_connect_result(dev, bssid, req_ie, req_ie_len,
				resp_ie, resp_ie_len, status, gfp);

	vos_runtime_pm_allow_suspend(padapter->runtime_context.connect);
}
#endif /* CFG80211_CONNECT_BSS */

VOS_STATUS hdd_start_all_adapters( hdd_context_t *pHddCtx )
{
   hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
   VOS_STATUS status;
   hdd_adapter_t      *pAdapter;
#if !defined(MSM_PLATFORM) || defined(WITH_BACKPORTS)
   v_MACADDR_t  bcastMac = VOS_MAC_ADDR_BROADCAST_INITIALIZER;
#endif
   eConnectionState  connState;

   ENTER();

   status = hdd_get_front_adapter ( pHddCtx, &pAdapterNode );

   while ( NULL != pAdapterNode && VOS_STATUS_SUCCESS == status )
   {
      pAdapter = pAdapterNode->pAdapter;

      hdd_wmm_init( pAdapter );

      switch(pAdapter->device_mode)
      {
         case WLAN_HDD_INFRA_STATION:
         case WLAN_HDD_P2P_CLIENT:
         case WLAN_HDD_P2P_DEVICE:

            connState = (WLAN_HDD_GET_STATION_CTX_PTR(pAdapter))->conn_info.connState;

            hdd_init_station_mode(pAdapter);
            /* Open the gates for HDD to receive Wext commands */
            pAdapter->isLinkUpSvcNeeded = FALSE;
            pAdapter->scan_info.mScanPending = FALSE;
            pAdapter->scan_info.waitScanResult = FALSE;

            //Indicate disconnect event to supplicant if associated previously
            if (eConnectionState_Associated == connState ||
                eConnectionState_IbssConnected == connState ||
                eConnectionState_NotConnected == connState ||
                eConnectionState_IbssDisconnected == connState ||
                eConnectionState_Disconnecting == connState)
            {
               union iwreq_data wrqu;
               memset(&wrqu, '\0', sizeof(wrqu));
               wrqu.ap_addr.sa_family = ARPHRD_ETHER;
               memset(wrqu.ap_addr.sa_data,'\0',ETH_ALEN);
               wireless_send_event(pAdapter->dev, SIOCGIWAP, &wrqu, NULL);
               pAdapter->sessionCtx.station.hdd_ReassocScenario = VOS_FALSE;

               /* indicate disconnected event to nl80211 */
               wlan_hdd_cfg80211_indicate_disconnect(pAdapter->dev, false,
                                                     WLAN_REASON_UNSPECIFIED);
            }
            else if (eConnectionState_Connecting == connState)
            {
              /*
               * Indicate connect failure to supplicant if we were in the
               * process of connecting
               */
               hdd_connect_result(pAdapter->dev, NULL, NULL,
                                       NULL, 0, NULL, 0,
                                       WLAN_STATUS_ASSOC_DENIED_UNSPEC,
                                       GFP_KERNEL, false, 0);
            }

#ifdef QCA_LL_TX_FLOW_CT
            if (pAdapter->tx_flow_timer_initialized == VOS_FALSE) {
                vos_timer_init(&pAdapter->tx_flow_control_timer,
                               VOS_TIMER_TYPE_SW,
                               hdd_tx_resume_timer_expired_handler,
                               pAdapter);
                pAdapter->tx_flow_timer_initialized = VOS_TRUE;
            }

            WLANTL_RegisterTXFlowControl(pHddCtx->pvosContext, hdd_tx_resume_cb,
                                         pAdapter->sessionId, (void *)pAdapter);
#endif

            break;

         case WLAN_HDD_SOFTAP:
            if (pHddCtx->cfg_ini->sap_internal_restart) {
                hdd_init_ap_mode(pAdapter, true);

#ifdef QCA_LL_TX_FLOW_CT
                if (pAdapter->tx_flow_timer_initialized == VOS_FALSE) {
                    vos_timer_init(&pAdapter->tx_flow_control_timer,
                                   VOS_TIMER_TYPE_SW,
                                   hdd_softap_tx_resume_timer_expired_handler,
                                   pAdapter);
                    pAdapter->tx_flow_timer_initialized = VOS_TRUE;
                }

                WLANTL_RegisterTXFlowControl(pHddCtx->pvosContext,
                          hdd_softap_tx_resume_cb,
                          pAdapter->sessionId,
                          (void *)pAdapter);
#endif

            }
            break;

         case WLAN_HDD_P2P_GO:
#if defined(MSM_PLATFORM) && !defined(WITH_BACKPORTS)
            hddLog(VOS_TRACE_LEVEL_ERROR, "%s [SSR] send stop ap to supplicant",
                                                       __func__);
            cfg80211_ap_stopped(pAdapter->dev, GFP_KERNEL);
#else
            hddLog(VOS_TRACE_LEVEL_ERROR, "%s [SSR] send restart supplicant",
                                                       __func__);
            /* event supplicant to restart */
            cfg80211_del_sta(pAdapter->dev,
                      (const u8 *)&bcastMac.bytes[0], GFP_KERNEL);
#endif
            break;

         case WLAN_HDD_MONITOR:
            /* monitor interface start */
            break;
         default:
            break;
      }

      status = hdd_get_next_adapter ( pHddCtx, pAdapterNode, &pNext );
      pAdapterNode = pNext;
   }

   EXIT();

   return VOS_STATUS_SUCCESS;
}

VOS_STATUS hdd_reconnect_all_adapters( hdd_context_t *pHddCtx )
{
   hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
   hdd_adapter_t *pAdapter;
   VOS_STATUS status;
   v_U32_t roamId;
   unsigned long rc;

   ENTER();

   status = hdd_get_front_adapter ( pHddCtx, &pAdapterNode );

   while ( NULL != pAdapterNode && VOS_STATUS_SUCCESS == status )
   {
      pAdapter = pAdapterNode->pAdapter;

      if( (WLAN_HDD_INFRA_STATION == pAdapter->device_mode) ||
             (WLAN_HDD_P2P_CLIENT == pAdapter->device_mode) )
      {
         hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
         hdd_wext_state_t *pWextState = WLAN_HDD_GET_WEXT_STATE_PTR(pAdapter);

         if (eConnectionState_Associated == pHddStaCtx->conn_info.connState) {
             wlan_hdd_decr_active_session(pHddCtx, pAdapter->device_mode);
         }
         hdd_connSetConnectionState(pAdapter, eConnectionState_NotConnected);
         init_completion(&pAdapter->disconnect_comp_var);
         sme_RoamDisconnect(pHddCtx->hHal, pAdapter->sessionId,
                             eCSR_DISCONNECT_REASON_UNSPECIFIED);

         rc = wait_for_completion_timeout(
                        &pAdapter->disconnect_comp_var,
                        msecs_to_jiffies(WLAN_WAIT_TIME_DISCONNECT));
         if (!rc)
            hddLog(LOGE, "%s: failure waiting for disconnect_comp_var",
                   __func__);
         pWextState->roamProfile.csrPersona = pAdapter->device_mode;
         pHddCtx->isAmpAllowed = VOS_FALSE;
         sme_RoamConnect(pHddCtx->hHal,
                         pAdapter->sessionId, &(pWextState->roamProfile),
                         &roamId);
      }

      status = hdd_get_next_adapter ( pHddCtx, pAdapterNode, &pNext );
      pAdapterNode = pNext;
   }

   EXIT();

   return VOS_STATUS_SUCCESS;
}

void hdd_dump_concurrency_info(hdd_context_t *pHddCtx)
{
   hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
   VOS_STATUS status;
   hdd_adapter_t *pAdapter;
   hdd_station_ctx_t *pHddStaCtx;
   hdd_ap_ctx_t *pHddApCtx;
   hdd_hostapd_state_t * pHostapdState;
   tCsrBssid staBssid = { 0 }, p2pBssid = { 0 };
   tCsrBssid apBssid = { 0 }, apBssid1 = { 0 }, apBssid2 = { 0 };
   v_U8_t staChannel = 0, p2pChannel = 0, apChannel = 0;
   v_U8_t apChannel1 = 0, apChannel2 = 0;
   const char *p2pMode = "DEV";
   bool mcc_mode = FALSE;

#ifdef QCA_LL_TX_FLOW_CT
   v_U8_t targetChannel = 0;
   v_U8_t preAdapterChannel = 0;
   v_U8_t channel24;
   v_U8_t channel5;
   hdd_adapter_t *preAdapterContext = NULL;
   hdd_adapter_t *pAdapter2_4 = NULL;
   hdd_adapter_t *pAdapter5 = NULL;
#endif /* QCA_LL_TX_FLOW_CT */

   status =  hdd_get_front_adapter ( pHddCtx, &pAdapterNode );
   while ( NULL != pAdapterNode && VOS_STATUS_SUCCESS == status )
   {
      pAdapter = pAdapterNode->pAdapter;
      switch (pAdapter->device_mode) {
      case WLAN_HDD_INFRA_STATION:
          pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
          if (eConnectionState_Associated == pHddStaCtx->conn_info.connState) {
              staChannel = pHddStaCtx->conn_info.operationChannel;
              memcpy(staBssid, pHddStaCtx->conn_info.bssId, sizeof(staBssid));
#ifdef QCA_LL_TX_FLOW_CT
              targetChannel = staChannel;
#endif /* QCA_LL_TX_FLOW_CT */
          }
          break;
      case WLAN_HDD_P2P_CLIENT:
          pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
          if (eConnectionState_Associated == pHddStaCtx->conn_info.connState) {
              p2pChannel = pHddStaCtx->conn_info.operationChannel;
              memcpy(p2pBssid, pHddStaCtx->conn_info.bssId, sizeof(p2pBssid));
              p2pMode = "CLI";
#ifdef QCA_LL_TX_FLOW_CT
              targetChannel = p2pChannel;
#endif /* QCA_LL_TX_FLOW_CT */
          }
          break;
      case WLAN_HDD_P2P_GO:
          pHddApCtx = WLAN_HDD_GET_AP_CTX_PTR(pAdapter);
          pHostapdState = WLAN_HDD_GET_HOSTAP_STATE_PTR(pAdapter);
          if (pHostapdState->bssState == BSS_START && pHostapdState->vosStatus==VOS_STATUS_SUCCESS) {
              p2pChannel = pHddApCtx->operatingChannel;
              memcpy(p2pBssid, pAdapter->macAddressCurrent.bytes, sizeof(p2pBssid));
#ifdef QCA_LL_TX_FLOW_CT
              targetChannel = p2pChannel;
#endif /* QCA_LL_TX_FLOW_CT */
          }
          p2pMode = "GO";
          break;
      case WLAN_HDD_SOFTAP:
          pHddApCtx = WLAN_HDD_GET_AP_CTX_PTR(pAdapter);
          pHostapdState = WLAN_HDD_GET_HOSTAP_STATE_PTR(pAdapter);
          if (pHostapdState->bssState == BSS_START && pHostapdState->vosStatus==VOS_STATUS_SUCCESS) {
              apChannel = pHddApCtx->operatingChannel;
              memcpy(apBssid, pAdapter->macAddressCurrent.bytes, sizeof(apBssid));
              if (!pHddApCtx->uBCStaId) {
                  apChannel1 = apChannel;
                  memcpy(apBssid1, apBssid, sizeof(apBssid));
              } else {
                  apChannel2 = apChannel;
                  memcpy(apBssid2, apBssid, sizeof(apBssid));
              }
#ifdef QCA_LL_TX_FLOW_CT
              targetChannel = apChannel;
#endif /* QCA_LL_TX_FLOW_CT */
          }
          break;
      case WLAN_HDD_IBSS:
#ifdef QCA_LL_TX_FLOW_CT
          pAdapter->tx_flow_low_watermark =
                       pHddCtx->cfg_ini->TxFlowLowWaterMark;
#endif
          return; /* skip printing station message below */
      default:
          break;
      }
#ifdef QCA_LL_TX_FLOW_CT
      if (targetChannel)
      {
         /* This is first adapter detected as active
          * set as default for none concurrency case */
         if (!preAdapterChannel)
         {
#ifdef IPA_UC_OFFLOAD
             /* If IPA UC data path is enabled,
              * target should reserve extra tx descriptors
              * for IPA WDI data path.
              * Then host data path should allow less TX packet pumping in case
              * IPA WDI data path enabled */
             if ((pHddCtx->cfg_ini->IpaUcOffloadEnabled) &&
                 (WLAN_HDD_SOFTAP == pAdapter->device_mode)) {
                pAdapter->tx_flow_low_watermark =
                       pHddCtx->cfg_ini->TxFlowLowWaterMark +
                       WLAN_TFC_IPAUC_TX_DESC_RESERVE;
             } else
#endif /* IPA_UC_OFFLOAD */
             {
                 pAdapter->tx_flow_low_watermark =
                       pHddCtx->cfg_ini->TxFlowLowWaterMark;
             }
             pAdapter->tx_flow_high_watermark_offset =
                       pHddCtx->cfg_ini->TxFlowHighWaterMarkOffset;
             WLANTL_SetAdapterMaxQDepth(pHddCtx->pvosContext,
                                        pAdapter->sessionId,
                                        pHddCtx->cfg_ini->TxFlowMaxQueueDepth);
             /* Temporary set log level as error
              * TX Flow control feature settled down, will lower log level */
             hddLog(LOG1,
                    "MODE %s(%d), CH %d, LWM %d, HWM %d, TXQDEP %d",
                    hdd_device_mode_to_string(pAdapter->device_mode),
                    pAdapter->device_mode,
                    targetChannel,
                    pAdapter->tx_flow_low_watermark,
                    pAdapter->tx_flow_low_watermark +
                    pAdapter->tx_flow_high_watermark_offset,
                    pHddCtx->cfg_ini->TxFlowMaxQueueDepth);
             preAdapterChannel = targetChannel;
             preAdapterContext = pAdapter;
         }
         else
         {
            /* SCC, disable TX flow control for both
             * SCC each adapter cannot reserve dedicated channel resource
             * as a result, if any adapter blocked OS Q by flow control,
             * blocked adapter will lost chance to recover  */
            if (preAdapterChannel == targetChannel)
            {
                /* Current adapter */
#ifdef CONFIG_PER_VDEV_TX_DESC_POOL
                pAdapter->tx_flow_low_watermark =
                       pHddCtx->cfg_ini->TxFlowLowWaterMark;
                pAdapter->tx_flow_high_watermark_offset =
                       pHddCtx->cfg_ini->TxFlowHighWaterMarkOffset;
#else
                pAdapter->tx_flow_low_watermark = 0;
                pAdapter->tx_flow_high_watermark_offset = 0;
#endif
                WLANTL_SetAdapterMaxQDepth(pHddCtx->pvosContext,
                                           pAdapter->sessionId,
                                           pHddCtx->cfg_ini->TxHbwFlowMaxQueueDepth);
                hddLog(LOG1,
                      "SCC: MODE %s(%d), CH %d, LWM %d, HWM %d, TXQDEP %d",
                      hdd_device_mode_to_string(pAdapter->device_mode),
                      pAdapter->device_mode,
                      targetChannel,
                      pAdapter->tx_flow_low_watermark,
                      pAdapter->tx_flow_low_watermark +
                      pAdapter->tx_flow_high_watermark_offset,
                      pHddCtx->cfg_ini->TxHbwFlowMaxQueueDepth);

                if (!preAdapterContext)
                {
                   hddLog(VOS_TRACE_LEVEL_ERROR,
                      "SCC: Previous adapter context NULL");
                   continue;
                }

                /* Previous adapter */
#ifdef CONFIG_PER_VDEV_TX_DESC_POOL
                preAdapterContext->tx_flow_low_watermark =
                       pHddCtx->cfg_ini->TxFlowLowWaterMark;
                preAdapterContext->tx_flow_high_watermark_offset =
                       pHddCtx->cfg_ini->TxFlowHighWaterMarkOffset;
#else
                preAdapterContext->tx_flow_low_watermark = 0;
                preAdapterContext->tx_flow_high_watermark_offset = 0;
#endif
                WLANTL_SetAdapterMaxQDepth(pHddCtx->pvosContext,
                                           preAdapterContext->sessionId,
                                           pHddCtx->cfg_ini->TxHbwFlowMaxQueueDepth);
                /* Temporary set log level as error
                 * TX Flow control feature settled down, will lower log level */
                hddLog(LOG1,
                      "SCC: MODE %s(%d), CH %d, LWM %d, HWM %d, TXQDEP %d",
                      hdd_device_mode_to_string(preAdapterContext->device_mode),
                      preAdapterContext->device_mode,
                      targetChannel,
                      preAdapterContext->tx_flow_low_watermark,
                      preAdapterContext->tx_flow_low_watermark +
                      preAdapterContext->tx_flow_high_watermark_offset,
                      pHddCtx->cfg_ini->TxHbwFlowMaxQueueDepth);
            }
            /* MCC, each adapter will have dedicated resource */
            else
            {
                /* current channel is 2.4 */
                if (targetChannel <= WLAN_HDD_TX_FLOW_CONTROL_MAX_24BAND_CH)
                {
                   channel24   = targetChannel;
                   channel5    = preAdapterChannel;
                   pAdapter2_4 = pAdapter;
                   pAdapter5   = preAdapterContext;
                }
                /* Current channel is 5 */
                else
                {
                   channel24   = preAdapterChannel;
                   channel5    = targetChannel;
                   pAdapter2_4 = preAdapterContext;
                   pAdapter5   = pAdapter;
                }

                if (!pAdapter5)
                {
                   hddLog(VOS_TRACE_LEVEL_ERROR,
                      "MCC: 5GHz adapter context NULL");
                   continue;
                }
                pAdapter5->tx_flow_low_watermark =
                       pHddCtx->cfg_ini->TxHbwFlowLowWaterMark;
                pAdapter5->tx_flow_high_watermark_offset =
                       pHddCtx->cfg_ini->TxHbwFlowHighWaterMarkOffset;
                WLANTL_SetAdapterMaxQDepth(pHddCtx->pvosContext,
                                        pAdapter5->sessionId,
                                        pHddCtx->cfg_ini->TxHbwFlowMaxQueueDepth);
                /* Temporary set log level as error
                 * TX Flow control feature settled down, will lower log level */
                hddLog(LOG1,
                    "MCC: MODE %s(%d), CH %d, LWM %d, HWM %d, TXQDEP %d",
                    hdd_device_mode_to_string(pAdapter5->device_mode),
                    pAdapter5->device_mode,
                    channel5,
                    pAdapter5->tx_flow_low_watermark,
                    pAdapter5->tx_flow_low_watermark +
                    pAdapter5->tx_flow_high_watermark_offset,
                    pHddCtx->cfg_ini->TxHbwFlowMaxQueueDepth);

                if (!pAdapter2_4)
                {
                   hddLog(VOS_TRACE_LEVEL_ERROR,
                      "MCC: 2.4GHz adapter context NULL");
                   continue;
                }
                pAdapter2_4->tx_flow_low_watermark =
                       pHddCtx->cfg_ini->TxLbwFlowLowWaterMark;
                pAdapter2_4->tx_flow_high_watermark_offset =
                       pHddCtx->cfg_ini->TxLbwFlowHighWaterMarkOffset;
                WLANTL_SetAdapterMaxQDepth(pHddCtx->pvosContext,
                                        pAdapter2_4->sessionId,
                                        pHddCtx->cfg_ini->TxLbwFlowMaxQueueDepth);
                /* Temporary set log level as error
                 * TX Flow control feature settled down, will lower log level */
                hddLog(LOG1,
                    "MCC: MODE %s(%d), CH %d, LWM %d, HWM %d, TXQDEP %d",
                    hdd_device_mode_to_string(pAdapter2_4->device_mode),
                    pAdapter2_4->device_mode,
                    channel24,
                    pAdapter2_4->tx_flow_low_watermark,
                    pAdapter2_4->tx_flow_low_watermark +
                    pAdapter2_4->tx_flow_high_watermark_offset,
                    pHddCtx->cfg_ini->TxLbwFlowMaxQueueDepth);
            }
         }
      }
      targetChannel = 0;
#endif /* QCA_LL_TX_FLOW_CT */
      status = hdd_get_next_adapter ( pHddCtx, pAdapterNode, &pNext );
      pAdapterNode = pNext;
   }

   /*
    * Determine SCC/MSS
    * Remind that this only considered STA+AP and AP+AP concurrency
    * Need to expand for futher concurreny in the future
    */
   if (apChannel1 > 0 && apChannel2 > 0) {
       mcc_mode = apChannel1 != apChannel2;
   } else if (staChannel > 0 && (apChannel1 > 0 || p2pChannel > 0)) {
       mcc_mode = !(p2pChannel==staChannel || apChannel1==staChannel);
   }
   if (pHddCtx->mcc_mode != mcc_mode) {
#ifdef IPA_UC_STA_OFFLOAD
       /* Send SCC/MCC Switching event to IPA */
       hdd_ipa_send_mcc_scc_msg(pHddCtx, mcc_mode);
#endif
       pHddCtx->mcc_mode = mcc_mode;
   }
   hddLog(VOS_TRACE_LEVEL_ERROR, "wlan(%d) " MAC_ADDRESS_STR " %s",
                staChannel, MAC_ADDR_ARRAY(staBssid), mcc_mode ? "MCC" : "SCC");
   if (p2pChannel > 0) {
       hddLog(VOS_TRACE_LEVEL_ERROR, "p2p-%s(%d) " MAC_ADDRESS_STR,
                     p2pMode, p2pChannel, MAC_ADDR_ARRAY(p2pBssid));
   }
   if (apChannel1 > 0) {
       hddLog(VOS_TRACE_LEVEL_ERROR, "AP(%d) " MAC_ADDRESS_STR,
                     apChannel1, MAC_ADDR_ARRAY(apBssid1));
   }
   if (apChannel2 > 0) {
       hddLog(VOS_TRACE_LEVEL_ERROR, "AP(%d) " MAC_ADDRESS_STR,
                     apChannel2, MAC_ADDR_ARRAY(apBssid2));
   }

   if (p2pChannel > 0 && apChannel1 > 0) {
        hddLog(VOS_TRACE_LEVEL_ERROR,
            FL("Error concurrent SAP %d and P2P %d which is not support"),
            apChannel1, p2pChannel);
   }
}

bool hdd_is_ssr_required( void)
{
    return (isSsrRequired == HDD_SSR_REQUIRED);
}

/* Once SSR is disabled then it cannot be set. */
void hdd_set_ssr_required( e_hdd_ssr_required value)
{
    if (HDD_SSR_DISABLED == isSsrRequired)
        return;

    isSsrRequired = value;
}

VOS_STATUS hdd_get_front_adapter( hdd_context_t *pHddCtx,
                                  hdd_adapter_list_node_t** ppAdapterNode)
{
    VOS_STATUS status;
    adf_os_spin_lock_bh(&pHddCtx->hddAdapters.lock);
    status =  hdd_list_peek_front ( &pHddCtx->hddAdapters,
                   (hdd_list_node_t**) ppAdapterNode );
    adf_os_spin_unlock_bh(&pHddCtx->hddAdapters.lock);
    return status;
}

VOS_STATUS hdd_get_next_adapter( hdd_context_t *pHddCtx,
                                 hdd_adapter_list_node_t* pAdapterNode,
                                 hdd_adapter_list_node_t** pNextAdapterNode)
{
    VOS_STATUS status;
    adf_os_spin_lock_bh(&pHddCtx->hddAdapters.lock);
    status = hdd_list_peek_next ( &pHddCtx->hddAdapters,
                                  (hdd_list_node_t*) pAdapterNode,
                                  (hdd_list_node_t**)pNextAdapterNode );

    adf_os_spin_unlock_bh(&pHddCtx->hddAdapters.lock);
    return status;
}

VOS_STATUS hdd_remove_adapter( hdd_context_t *pHddCtx,
                               hdd_adapter_list_node_t* pAdapterNode)
{
    VOS_STATUS status;
    adf_os_spin_lock_bh(&pHddCtx->hddAdapters.lock);
    status =  hdd_list_remove_node ( &pHddCtx->hddAdapters,
                                     &pAdapterNode->node );
    adf_os_spin_unlock_bh(&pHddCtx->hddAdapters.lock);
    return status;
}

VOS_STATUS hdd_remove_front_adapter( hdd_context_t *pHddCtx,
                                     hdd_adapter_list_node_t** ppAdapterNode)
{
    VOS_STATUS status;
    adf_os_spin_lock_bh(&pHddCtx->hddAdapters.lock);
    status =  hdd_list_remove_front( &pHddCtx->hddAdapters,
                   (hdd_list_node_t**) ppAdapterNode );
    adf_os_spin_unlock_bh(&pHddCtx->hddAdapters.lock);
    return status;
}

VOS_STATUS hdd_add_adapter_back( hdd_context_t *pHddCtx,
                                 hdd_adapter_list_node_t* pAdapterNode)
{
    VOS_STATUS status;
    adf_os_spin_lock_bh(&pHddCtx->hddAdapters.lock);
    status =  hdd_list_insert_back ( &pHddCtx->hddAdapters,
                   (hdd_list_node_t*) pAdapterNode );
    adf_os_spin_unlock_bh(&pHddCtx->hddAdapters.lock);
    return status;
}

VOS_STATUS hdd_add_adapter_front( hdd_context_t *pHddCtx,
                                  hdd_adapter_list_node_t* pAdapterNode)
{
    VOS_STATUS status;
    adf_os_spin_lock_bh(&pHddCtx->hddAdapters.lock);
    status =  hdd_list_insert_front ( &pHddCtx->hddAdapters,
                   (hdd_list_node_t*) pAdapterNode );
    adf_os_spin_unlock_bh(&pHddCtx->hddAdapters.lock);
    return status;
}

hdd_adapter_t * hdd_get_adapter_by_macaddr( hdd_context_t *pHddCtx,
                                            tSirMacAddr macAddr )
{
   hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
   hdd_adapter_t *pAdapter;
   VOS_STATUS status;

   status = hdd_get_front_adapter ( pHddCtx, &pAdapterNode );

   while ( NULL != pAdapterNode && VOS_STATUS_SUCCESS == status )
   {
      pAdapter = pAdapterNode->pAdapter;

      if( pAdapter && vos_mem_compare( pAdapter->macAddressCurrent.bytes,
                                       macAddr, sizeof(tSirMacAddr) ) )
      {
         return pAdapter;
      }
      status = hdd_get_next_adapter ( pHddCtx, pAdapterNode, &pNext );
      pAdapterNode = pNext;
   }

   return NULL;

}

hdd_adapter_t * hdd_get_adapter_by_rand_macaddr(hdd_context_t *hdd_ctx,
						tSirMacAddr mac_addr)
{
	hdd_adapter_list_node_t *adapter_node = NULL, *next = NULL;
	hdd_adapter_t *adapter;
	VOS_STATUS status;

	status = hdd_get_front_adapter(hdd_ctx, &adapter_node);
	while (adapter_node && status == VOS_STATUS_SUCCESS) {
		adapter = adapter_node->pAdapter;
		if(adapter && hdd_check_random_mac(adapter, mac_addr))
			return adapter;
		status = hdd_get_next_adapter(hdd_ctx, adapter_node, &next);
		adapter_node = next;
	}

	return NULL;
}

hdd_adapter_t * hdd_get_adapter_by_name( hdd_context_t *pHddCtx, tANI_U8 *name )
{
   hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
   hdd_adapter_t *pAdapter;
   VOS_STATUS status;

   status = hdd_get_front_adapter ( pHddCtx, &pAdapterNode );

   while ( NULL != pAdapterNode && VOS_STATUS_SUCCESS == status )
   {
      pAdapter = pAdapterNode->pAdapter;

      if( pAdapter && !strncmp( pAdapter->dev->name, (const char *)name,
          IFNAMSIZ ) )
      {
         return pAdapter;
      }
      status = hdd_get_next_adapter ( pHddCtx, pAdapterNode, &pNext );
      pAdapterNode = pNext;
   }

   return NULL;

}

hdd_adapter_t *hdd_get_adapter_by_vdev( hdd_context_t *pHddCtx,
                                        tANI_U32 vdev_id )
{
    hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
    hdd_adapter_t *pAdapter;
    VOS_STATUS vos_status;


    vos_status = hdd_get_front_adapter( pHddCtx, &pAdapterNode);

    while ((NULL != pAdapterNode) && (VOS_STATUS_SUCCESS == vos_status))
    {
        pAdapter = pAdapterNode->pAdapter;

        if (pAdapter->sessionId == vdev_id)
            return pAdapter;

        vos_status = hdd_get_next_adapter(pHddCtx, pAdapterNode, &pNext);
        pAdapterNode = pNext;
    }

    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
              "%s: vdev_id %d does not exist with host",
              __func__, vdev_id);

    return NULL;
}

/**
 * hdd_get_adapter_by_sme_session_id() - Return adapter with
 * the sessionid
 * @hdd_ctx: hdd cntx.
 * @sme_session_id: sme session is for the adapter to get.
 *
 * This function is used to get the adapter with provided session id
 *
 * Return: adapter pointer if found
 *
 */
hdd_adapter_t *hdd_get_adapter_by_sme_session_id(hdd_context_t *hdd_ctx,
						uint32_t sme_session_id)
{
	hdd_adapter_list_node_t *adapter_node = NULL, *next = NULL;
	hdd_adapter_t *adapter;
	VOS_STATUS vos_status;


	vos_status = hdd_get_front_adapter(hdd_ctx, &adapter_node);

	while ((NULL != adapter_node) &&
			(VOS_STATUS_SUCCESS == vos_status)) {
		adapter = adapter_node->pAdapter;

		if (adapter &&
			 adapter->sessionId == sme_session_id)
			return adapter;

		vos_status =
			hdd_get_next_adapter(hdd_ctx,
				 adapter_node, &next);
		adapter_node = next;
	}
	return NULL;
}

hdd_adapter_t * hdd_get_adapter( hdd_context_t *pHddCtx, device_mode_t mode )
{
   hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
   hdd_adapter_t *pAdapter;
   VOS_STATUS status;

   status = hdd_get_front_adapter ( pHddCtx, &pAdapterNode );

   while ( NULL != pAdapterNode && VOS_STATUS_SUCCESS == status )
   {
      pAdapter = pAdapterNode->pAdapter;

      if( pAdapter && (mode == pAdapter->device_mode) )
      {
         return pAdapter;
      }
      status = hdd_get_next_adapter ( pHddCtx, pAdapterNode, &pNext );
      pAdapterNode = pNext;
   }

   return NULL;

}

hdd_adapter_t * hdd_get_adapter_by_wdev(struct wireless_dev *wdev)
{
	hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
	hdd_adapter_t *pAdapter;
	VOS_STATUS status;
	hdd_context_t * pHddCtx;

	if (!wdev)
		return NULL;

	pHddCtx = wiphy_priv(wdev->wiphy);

	status = hdd_get_front_adapter(pHddCtx, &pAdapterNode);

	while (pAdapterNode != NULL && status == VOS_STATUS_SUCCESS)
	{
		pAdapter = pAdapterNode->pAdapter;

		if (pAdapter && (wdev == &(pAdapter->wdev)))
			return pAdapter;

		status = hdd_get_next_adapter(pHddCtx, pAdapterNode, &pNext);
		pAdapterNode = pNext;
	}

	return NULL;
}
/**---------------------------------------------------------------------------

  \brief hdd_get_operating_channel() -

   This API returns the operating channel of the requested device mode

  \param  - pHddCtx - Pointer to the HDD context.
              - mode - Device mode for which operating channel is required
                supported modes - WLAN_HDD_INFRA_STATION, WLAN_HDD_P2P_CLIENT
                                 WLAN_HDD_SOFTAP, WLAN_HDD_P2P_GO.
  \return - channel number. "0" id the requested device is not found OR it is not connected.
  --------------------------------------------------------------------------*/
v_U8_t hdd_get_operating_channel( hdd_context_t *pHddCtx, device_mode_t mode )
{
   hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
   VOS_STATUS status;
   hdd_adapter_t      *pAdapter;
   v_U8_t operatingChannel = 0;

   status = hdd_get_front_adapter ( pHddCtx, &pAdapterNode );

   while ( NULL != pAdapterNode && VOS_STATUS_SUCCESS == status )
   {
      pAdapter = pAdapterNode->pAdapter;

      if( mode == pAdapter->device_mode )
      {
        switch(pAdapter->device_mode)
        {
          case WLAN_HDD_INFRA_STATION:
          case WLAN_HDD_P2P_CLIENT:
            if( hdd_connIsConnected( WLAN_HDD_GET_STATION_CTX_PTR( pAdapter )) )
              operatingChannel = (WLAN_HDD_GET_STATION_CTX_PTR(pAdapter))->conn_info.operationChannel;
            break;
          case WLAN_HDD_SOFTAP:
          case WLAN_HDD_P2P_GO:
            /*softap connection info */
            if(test_bit(SOFTAP_BSS_STARTED, &pAdapter->event_flags))
              operatingChannel = (WLAN_HDD_GET_AP_CTX_PTR(pAdapter))->operatingChannel;
            break;
          default:
            break;
        }

        break; //Found the device of interest. break the loop
      }

      status = hdd_get_next_adapter ( pHddCtx, pAdapterNode, &pNext );
      pAdapterNode = pNext;
   }
   return operatingChannel;
}

/**---------------------------------------------------------------------------

  \brief hdd_wlan_initial_scan() -

   This function triggers the initial scan

  \param  - pAdapter - Pointer to the HDD adapter.

  --------------------------------------------------------------------------*/
void hdd_wlan_initial_scan(hdd_adapter_t *pAdapter)
{
   tCsrScanRequest scanReq;
   tCsrChannelInfo channelInfo;
   eHalStatus halStatus;
   tANI_U32 scanId;
   hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);

   vos_mem_zero(&scanReq, sizeof(tCsrScanRequest));
   vos_mem_set(&scanReq.bssid, sizeof(tCsrBssid), 0xff);
   scanReq.BSSType = eCSR_BSS_TYPE_ANY;

   if(sme_Is11dSupported(pHddCtx->hHal))
   {
      halStatus = sme_ScanGetBaseChannels( pHddCtx->hHal, &channelInfo );
      if ( HAL_STATUS_SUCCESS( halStatus ) )
      {
         scanReq.ChannelInfo.ChannelList = vos_mem_malloc(channelInfo.numOfChannels);
         if( !scanReq.ChannelInfo.ChannelList )
         {
            hddLog(VOS_TRACE_LEVEL_ERROR, "%s kmalloc failed", __func__);
            vos_mem_free(channelInfo.ChannelList);
            channelInfo.ChannelList = NULL;
            return;
         }
         vos_mem_copy(scanReq.ChannelInfo.ChannelList, channelInfo.ChannelList,
            channelInfo.numOfChannels);
         scanReq.ChannelInfo.numOfChannels = channelInfo.numOfChannels;
         vos_mem_free(channelInfo.ChannelList);
         channelInfo.ChannelList = NULL;
      }

      scanReq.scanType = eSIR_PASSIVE_SCAN;
      scanReq.requestType = eCSR_SCAN_REQUEST_11D_SCAN;
      scanReq.maxChnTime = pHddCtx->cfg_ini->nPassiveMaxChnTime;
      scanReq.minChnTime = pHddCtx->cfg_ini->nPassiveMinChnTime;
   }
   else
   {
      scanReq.scanType = eSIR_ACTIVE_SCAN;
      scanReq.requestType = eCSR_SCAN_REQUEST_FULL_SCAN;
      scanReq.maxChnTime = pHddCtx->cfg_ini->nActiveMaxChnTime;
      scanReq.minChnTime = pHddCtx->cfg_ini->nActiveMinChnTime;
   }

   halStatus = sme_ScanRequest(pHddCtx->hHal, pAdapter->sessionId, &scanReq, &scanId, NULL, NULL);
   if ( !HAL_STATUS_SUCCESS( halStatus ) )
   {
      hddLog(VOS_TRACE_LEVEL_ERROR, "%s: sme_ScanRequest failed status code %d",
         __func__, halStatus );
   }

   if(sme_Is11dSupported(pHddCtx->hHal))
        vos_mem_free(scanReq.ChannelInfo.ChannelList);
}

/**---------------------------------------------------------------------------

  \brief hdd_full_power_callback() - HDD full power callback function

  This is the function invoked by SME to inform the result of a full power
  request issued by HDD

  \param  - callback context - Pointer to cookie
  \param  - status - result of request

  \return - None

  --------------------------------------------------------------------------*/
static void hdd_full_power_callback(void *callbackContext, eHalStatus status)
{
   struct statsContext *pContext = callbackContext;

   hddLog(VOS_TRACE_LEVEL_INFO,
          "%s: context = %pK, status = %d", __func__, pContext, status);

   if (NULL == callbackContext)
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,
             "%s: Bad param, context [%pK]",
             __func__, callbackContext);
      return;
   }

   /* there is a race condition that exists between this callback
      function and the caller since the caller could time out either
      before or while this code is executing.  we use a spinlock to
      serialize these actions */
   adf_os_spin_lock(&hdd_context_lock);

   if (POWER_CONTEXT_MAGIC != pContext->magic)
   {
      /* the caller presumably timed out so there is nothing we can do */
      adf_os_spin_unlock(&hdd_context_lock);
      hddLog(VOS_TRACE_LEVEL_WARN,
             "%s: Invalid context, magic [%08x]",
              __func__, pContext->magic);
      return;
   }

   /* context is valid so caller is still waiting */

   /* paranoia: invalidate the magic */
   pContext->magic = 0;

   /* notify the caller */
   complete(&pContext->completion);

   /* serialization is complete */
   adf_os_spin_unlock(&hdd_context_lock);
}

static inline VOS_STATUS hdd_UnregisterWext_all_adapters(hdd_context_t *pHddCtx)
{
    hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
    VOS_STATUS status;
    hdd_adapter_t      *pAdapter;

    ENTER();

    status = hdd_get_front_adapter(pHddCtx, &pAdapterNode);

    while ( NULL != pAdapterNode && VOS_STATUS_SUCCESS == status )
    {
        pAdapter = pAdapterNode->pAdapter;
        if ((pAdapter->device_mode == WLAN_HDD_INFRA_STATION) ||
                (pAdapter->device_mode == WLAN_HDD_P2P_CLIENT) ||
                (pAdapter->device_mode == WLAN_HDD_IBSS) ||
                (pAdapter->device_mode == WLAN_HDD_P2P_DEVICE) ||
                (pAdapter->device_mode == WLAN_HDD_SOFTAP) ||
                (pAdapter->device_mode == WLAN_HDD_P2P_GO)) {
            wlan_hdd_cfg80211_deregister_frames(pAdapter);
            hdd_UnregisterWext(pAdapter->dev);
        }
        status = hdd_get_next_adapter(pHddCtx, pAdapterNode, &pNext);
        pAdapterNode = pNext;
    }

    EXIT();

    return VOS_STATUS_SUCCESS;
}

VOS_STATUS hdd_abort_mac_scan_all_adapters(hdd_context_t *pHddCtx)
{
    hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
    VOS_STATUS status;
    hdd_adapter_t      *pAdapter;

    ENTER();

    status = hdd_get_front_adapter(pHddCtx, &pAdapterNode);

    while (NULL != pAdapterNode && VOS_STATUS_SUCCESS == status)
    {
        pAdapter = pAdapterNode->pAdapter;
        if ((pAdapter->device_mode == WLAN_HDD_INFRA_STATION) ||
            (pAdapter->device_mode == WLAN_HDD_P2P_CLIENT) ||
            (pAdapter->device_mode == WLAN_HDD_IBSS) ||
            (pAdapter->device_mode == WLAN_HDD_P2P_DEVICE) ||
            (pAdapter->device_mode == WLAN_HDD_SOFTAP) ||
            (pAdapter->device_mode == WLAN_HDD_P2P_GO)) {
            hdd_abort_mac_scan(pHddCtx, pAdapter->sessionId,
                               eCSR_SCAN_ABORT_DEFAULT);
        }
        status = hdd_get_next_adapter(pHddCtx, pAdapterNode, &pNext);
        pAdapterNode = pNext;
    }

    EXIT();

    return VOS_STATUS_SUCCESS;
}

/**
 * wlan_hdd_restart_init() - restart init
 * @pHddCtx:  Pointer to hdd context
 *
 * This function initializes restart timer/flag. An internal function.
 *
 * Return: None
 */
static void wlan_hdd_restart_init(hdd_context_t *pHddCtx)
{
   /* Initialize */
   pHddCtx->hdd_restart_retries = 0;
   atomic_set(&pHddCtx->isRestartInProgress, 0);
   vos_timer_init(&pHddCtx->hdd_restart_timer,
                     VOS_TIMER_TYPE_SW,
                     wlan_hdd_restart_timer_cb,
                     pHddCtx);
}

/**
 * wlan_hdd_restart_deinit() - restart deinit
 * @pHddCtx:  Pointer to hdd context
 *
 * This function cleans up the resources used. An internal function.
 *
 * Return: None
 */
static void wlan_hdd_restart_deinit(hdd_context_t* pHddCtx)
{
   VOS_STATUS vos_status;
   /* Block any further calls */
   atomic_set(&pHddCtx->isRestartInProgress, 1);
   /* Cleanup */
   vos_status = vos_timer_stop( &pHddCtx->hdd_restart_timer );
   if (!VOS_IS_STATUS_SUCCESS(vos_status))
          hddLog(LOGE, FL("Failed to stop HDD restart timer"));
   vos_status = vos_timer_destroy(&pHddCtx->hdd_restart_timer);
   if (!VOS_IS_STATUS_SUCCESS(vos_status))
          hddLog(LOGE, FL("Failed to destroy HDD restart timer"));
}

#ifdef WLAN_NS_OFFLOAD
/**
 * hdd_wlan_unregister_ip6_notifier() - unregister IP6 change notifier
 * @hdd_ctx: Pointer to hdd context
 *
 * Return: None
 */
static void hdd_wlan_unregister_ip6_notifier(hdd_context_t *hdd_ctx)
{
	unregister_inet6addr_notifier(&hdd_ctx->ipv6_notifier);

	return;
}

/**
 * hdd_wlan_register_ip6_notifier() - register IP6 change notifier
 * @hdd_ctx: Pointer to hdd context
 *
 * Return: None
 */
static void hdd_wlan_register_ip6_notifier(hdd_context_t *hdd_ctx)
{
	int ret;

	hdd_ctx->ipv6_notifier.notifier_call = wlan_hdd_ipv6_changed;
	ret = register_inet6addr_notifier(&hdd_ctx->ipv6_notifier);
	if (ret)
		hddLog(LOGE, FL("Failed to register IPv6 notifier"));
	else
		hddLog(LOG1, FL("Registered IPv6 notifier"));

	return;
}
#else
/**
 * hdd_wlan_unregister_ip6_notifier() - unregister IP6 change notifier
 * @hdd_ctx: Pointer to hdd context
 *
 * Return: None
 */
static void hdd_wlan_unregister_ip6_notifier(hdd_context_t *hdd_ctx)
{
}
/**
 * hdd_wlan_register_ip6_notifier() - register IP6 change notifier
 * @hdd_ctx: Pointer to hdd context
 *
 * Return: None
 */
static void hdd_wlan_register_ip6_notifier(hdd_context_t *hdd_ctx)
{
}
#endif

#ifdef WLAN_LOGGING_SOCK_SVC_ENABLE
/**
 * wlan_hdd_logging_sock_activate_svc() - Activate logging
 * @hdd_ctx: HDD context
 *
 * Activates the logging service
 *
 * Return: Zero in case of success, negative value otherwise
 */
static int wlan_hdd_logging_sock_activate_svc(hdd_context_t *hdd_ctx)
{
	if (hdd_ctx->cfg_ini->wlanLoggingEnable) {
		if (wlan_logging_sock_activate_svc(
				hdd_ctx->cfg_ini->wlanLoggingFEToConsole,
				hdd_ctx->cfg_ini->wlanLoggingNumBuf)) {
			hddLog(VOS_TRACE_LEVEL_ERROR,
				"%s: wlan_logging_sock_activate_svc failed",
				__func__);
			return -EINVAL;
		}
	}
	return 0;
}
/**
 * wlan_hdd_logging_sock_deactivate_svc() - Deactivate logging
 * @hdd_ctx: HDD context
 *
 * Deactivates the logging service
 *
 * Return: 0 on deactivating the logging service
 */
static int wlan_hdd_logging_sock_deactivate_svc(hdd_context_t *hdd_ctx)
{
	if (hdd_ctx && hdd_ctx->cfg_ini->wlanLoggingEnable)
		return wlan_logging_sock_deactivate_svc();

	return 0;
}
#else
static inline int wlan_hdd_logging_sock_activate_svc(hdd_context_t *hdd_ctx)
{
	return 0;
}

static inline int wlan_hdd_logging_sock_deactivate_svc(hdd_context_t *hdd_ctx)
{
	return 0;
}
#endif

/**
 * enum driver_unload_state - Various driver unload states
 * @unload_start: Driver starting unload module.
 * @unload_unregister_ip_notifier: Driver unregistering with ip notifiers.
 * @unload_unregister_wext_adpater: Driver unregistering wext adapter.
 * @unload_hdd_ftm_stop: Driver stopping ftm.
 * @unload_hdd_ftm_close: Driver closing ftm.
 * @unload_deregister_pm_ops:  Driver deregistering pm ops.
 * @unload_unregister_thermal_notify_cb: Driver unregistering thermal callback.
 * @unload_aborting_all_scan: Driver aborting all scan.
 * @unload_disable_pwr_save: Driver disable power save in firmware.
 * @unload_req_full_power: Driver requesting full power mode.
 * @unload_set_idle_ps_config: Driver setting imps false.
 * @unload_debugfs_exit: Driver exiting debugfs.
 * @unload_netdev_notifier: Driver unregistering with netdev notifiers.
 * @unload_stop_all_adapter: Driver stopping all adapters.
 * @unload_vos_stop: Driver stopping vos.
 * @unload_vos_sched_close: Driver closing scheduler.
 * @unload_vos_nv_close: Driver closing nv module.
 * @unload_vos_close: Driver closing vos module.
 * @unload_deinit_greep_ap: Driver deinit greep ap.
 * @unload_logging_sock_deactivate_svc: Driver deactivating logging socket.
 * @unload_hdd_send_status_pkg: Driver sending status packet to LPSS.
 * @unload_close_cesium_nl_sock: Driver closing cesium nl socket.
 * @unload_runtime_suspend_deinit: Driver deinit  runtime suspend.
 * @unload_close_all_adapters: Driver closing all adapter.
 * @unload_ipa_cleanup: Driver performing ipa cleanup.
 * @unload_flush_roc_work: Driver flush roc work.
 * @unload_nl_srv_exit: Driver exit netlink service.
 * @unload_wiphy_unregister: Driver unregister wiphy.
 * @unload_wiphy_free: Driver free wiphy.
 * @unload_subsystem_restart: Driver do subsystem restart.
 * @unload_finish: Driver completed unload.
 */
enum driver_unload_state{
	unload_start = 1,
	unload_unregister_ip_notifier,
	unload_unregister_wext_adpater,
	unload_hdd_ftm_stop,
	unload_hdd_ftm_close =5 ,
	unload_deregister_pm_ops,
	unload_unregister_thermal_notify_cb,
	unload_aborting_all_scan,
	unload_disable_pwr_save,
	unload_req_full_power = 10,
	unload_set_idle_ps_config,
	unload_debugfs_exit,
	unload_netdev_notifier,
	unload_stop_all_adapter,
	unload_vos_stop = 15,
	unload_vos_sched_close,
	unload_vos_nv_close,
	unload_vos_close,
	unload_deinit_greep_ap,
	unload_logging_sock_deactivate_svc = 20,
	unload_hdd_send_status_pkg,
	unload_close_cesium_nl_sock,
	unload_runtime_suspend_deinit,
	unload_close_all_adapters,
	unload_ipa_cleanup = 25,
	unload_flush_roc_work,
	unload_nl_srv_exit,
	unload_wiphy_unregister,
	unload_wiphy_free,
	unload_subsystem_restart = 30,

	/* keep it last*/
	unload_finish = 0xff
};

#ifdef FEATURE_BUS_BANDWIDTH
static void hdd_deinit_bus_bw_timer(hdd_context_t *hdd_ctx)
{
	VOS_TIMER_STATE vos_timer_state;

	vos_timer_state = vos_timer_getCurrentState(&hdd_ctx->bus_bw_timer);

	if (vos_timer_state == VOS_TIMER_STATE_UNUSED)
		return;

	if (VOS_TIMER_STATE_RUNNING == vos_timer_state) {
		vos_timer_stop(&hdd_ctx->bus_bw_timer);
		hdd_rst_tcp_delack(hdd_ctx);

		if (hdd_ctx->hbw_requested) {
			vos_remove_pm_qos();
			hdd_ctx->hbw_requested = false;
		}
	}

	if (!VOS_IS_STATUS_SUCCESS(vos_timer_destroy(&hdd_ctx->bus_bw_timer)))
		hddLog(VOS_TRACE_LEVEL_ERROR,
		       "%s: Cannot deallocate Bus bandwidth timer", __func__);
}
#else
static inline void hdd_deinit_bus_bw_timer(hdd_context_t *hdd_ctx) {}
#endif

/**---------------------------------------------------------------------------

  \brief hdd_wlan_exit() - HDD WLAN exit function

  This is the driver exit point (invoked during rmmod)

  \param  - pHddCtx - Pointer to the HDD Context

  \return - None

  --------------------------------------------------------------------------*/
void hdd_wlan_exit(hdd_context_t *pHddCtx)
{
   eHalStatus halStatus;
   v_CONTEXT_t pVosContext = pHddCtx->pvosContext;
   VOS_STATUS vosStatus;
   struct wiphy *wiphy = pHddCtx->wiphy;
   struct statsContext powerContext;
   unsigned long rc;
   hdd_config_t *pConfig = pHddCtx->cfg_ini;

   ENTER();
   TRACK_UNLOAD_STATUS(unload_start);

   hddLog(LOGE, FL("Unregister IPv6 notifier"));
   TRACK_UNLOAD_STATUS(unload_unregister_ip_notifier);
   hdd_wlan_unregister_ip6_notifier(pHddCtx);
   hddLog(LOGE, FL("Unregister IPv4 notifier"));
   unregister_inetaddr_notifier(&pHddCtx->ipv4_notifier);
   wlan_hdd_tsf_deinit(pHddCtx);

   if (VOS_FTM_MODE != hdd_get_conparam())
   {
      // Unloading, restart logic is no more required.
      wlan_hdd_restart_deinit(pHddCtx);

      //only do this in non-FTM Mode
#ifdef WLAN_FEATURE_USB_RECOVERY
      vos_flush_delayed_work(&pHddCtx->usb_detect_work);
#endif
   }
   TRACK_UNLOAD_STATUS(unload_unregister_wext_adpater);
   hdd_UnregisterWext_all_adapters(pHddCtx);
   if (VOS_FTM_MODE == hdd_get_conparam())
   {
      hddLog(VOS_TRACE_LEVEL_INFO, "%s: FTM MODE", __func__);
      TRACK_UNLOAD_STATUS(unload_hdd_ftm_stop);
#if  defined(QCA_WIFI_FTM)
      if (hdd_ftm_stop(pHddCtx))
      {
          hddLog(VOS_TRACE_LEVEL_FATAL,"%s: hdd_ftm_stop Failed",__func__);
          VOS_ASSERT(0);
      }
      pHddCtx->ftm.ftm_state = WLAN_FTM_STOPPED;
#endif
      TRACK_UNLOAD_STATUS(unload_hdd_ftm_close);
      wlan_hdd_ftm_close(pHddCtx);
      hddLog(VOS_TRACE_LEVEL_FATAL, "%s: FTM driver unloaded", __func__);
      goto free_hdd_ctx;
   }

   TRACK_UNLOAD_STATUS(unload_deregister_pm_ops);
   /* DeRegister with platform driver as client for Suspend/Resume */
   vosStatus = hddDeregisterPmOps(pHddCtx);
   if ( !VOS_IS_STATUS_SUCCESS( vosStatus ) )
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,"%s: hddDeregisterPmOps failed",__func__);
      VOS_ASSERT(0);
   }

   TRACK_UNLOAD_STATUS(unload_unregister_thermal_notify_cb);
   vosStatus = hddDevTmUnregisterNotifyCallback(pHddCtx);
   if ( !VOS_IS_STATUS_SUCCESS( vosStatus ) )
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,"%s: hddDevTmUnregisterNotifyCallback failed",__func__);
   }

   vosStatus = vos_timer_deinit(&pHddCtx->tdls_source_timer);
   if (!VOS_IS_STATUS_SUCCESS(vosStatus))
      hddLog(VOS_TRACE_LEVEL_FATAL,
             "%s: deinit tdls source timer failed", __func__);

   /*
    * Cancel any outstanding scan requests.  We are about to close all
    * of our adapters, but an adapter structure is what SME passes back
    * to our callback function.  Hence if there are any outstanding scan
    * requests then there is a race condition between when the adapter
    * is closed and when the callback is invoked.  We try to resolve that
    * race condition here by cancelling any outstanding scans before we
    * close the adapters.
    * Note that the scans may be cancelled in an asynchronous manner, so
    * ideally there needs to be some kind of synchronization.  Rather than
    * introduce a new synchronization here, we will utilize the fact that
    * we are about to Request Full Power, and since that is synchronized,
    * the expectation is that by the time Request Full Power has completed,
    * all scans will be cancelled.
    */
   TRACK_UNLOAD_STATUS(unload_aborting_all_scan);
   hdd_abort_mac_scan_all_adapters(pHddCtx);
   hdd_deinit_bus_bw_timer(pHddCtx);

#ifdef CONFIG_IXC_TIMER
      vos_timer_stop(&pHddCtx->set_ixc_prio_timer);
      vos_timer_destroy(&pHddCtx->set_ixc_prio_timer);
#endif
#ifdef FEATURE_WLAN_AP_AP_ACS_OPTIMIZE
   vosStatus = vos_timer_deinit(&pHddCtx->skip_acs_scan_timer);
   if (!VOS_IS_STATUS_SUCCESS(vosStatus))
       hddLog(VOS_TRACE_LEVEL_FATAL, "%s: deinit skip acs scan timer failed",
              __func__);

   adf_os_spin_lock(&pHddCtx->acs_skip_lock);
   vos_mem_free(pHddCtx->last_acs_channel_list);
   pHddCtx->last_acs_channel_list = NULL;
   pHddCtx->num_of_channels = 0;
   adf_os_spin_unlock(&pHddCtx->acs_skip_lock);
#endif


   if (pConfig && !pConfig->enablePowersaveOffload)
   {
      TRACK_UNLOAD_STATUS(unload_disable_pwr_save);
      //Disable IMPS/BMPS as we do not want the device to enter any power
      //save mode during shutdown
      sme_DisablePowerSave(pHddCtx->hHal, ePMC_IDLE_MODE_POWER_SAVE);
      sme_DisablePowerSave(pHddCtx->hHal, ePMC_BEACON_MODE_POWER_SAVE);
      sme_DisablePowerSave(pHddCtx->hHal, ePMC_UAPSD_MODE_POWER_SAVE);

      //Ensure that device is in full power as we will touch H/W during vos_Stop
      init_completion(&powerContext.completion);
      powerContext.magic = POWER_CONTEXT_MAGIC;

      TRACK_UNLOAD_STATUS(unload_req_full_power);
      halStatus = sme_RequestFullPower(pHddCtx->hHal, hdd_full_power_callback,
            &powerContext, eSME_FULL_PWR_NEEDED_BY_HDD);

      if (eHAL_STATUS_SUCCESS != halStatus)
      {
         if (eHAL_STATUS_PMC_PENDING == halStatus)
         {
            /* request was sent -- wait for the response */
            rc = wait_for_completion_timeout(
                  &powerContext.completion,
                  msecs_to_jiffies(WLAN_WAIT_TIME_POWER));
            if (!rc) {
               hddLog(VOS_TRACE_LEVEL_ERROR,
                     "%s: timed out while requesting full power",
                     __func__);
            }
         }
         else
         {
            hddLog(VOS_TRACE_LEVEL_ERROR,
                  "%s: Request for Full Power failed, status %d",
                  __func__, halStatus);
            /* continue -- need to clean up as much as possible */
         }
      }
      /* either we never sent a request, we sent a request and received a
         response or we sent a request and timed out.  if we never sent a
         request or if we sent a request and got a response, we want to
         clear the magic out of paranoia.  if we timed out there is a
         race condition such that the callback function could be
         executing at the same time we are. of primary concern is if the
         callback function had already verified the "magic" but had not
         yet set the completion variable when a timeout occurred. we
         serialize these activities by invalidating the magic while
         holding a shared spinlock which will cause us to block if the
         callback is currently executing */
      adf_os_spin_lock(&hdd_context_lock);
      powerContext.magic = 0;
      adf_os_spin_unlock(&hdd_context_lock);
   }
   else
   {
      /*
       * Powersave Offload Case
       * Disable Idle Power Save Mode
       */
      TRACK_UNLOAD_STATUS(unload_set_idle_ps_config);
      hdd_set_idle_ps_config(pHddCtx, FALSE);
   }

   hdd_spectral_deinit(pHddCtx);

   TRACK_UNLOAD_STATUS(unload_debugfs_exit);
   hdd_debugfs_exit(pHddCtx);

   // Unregister the Net Device Notifier
   TRACK_UNLOAD_STATUS(unload_netdev_notifier);
   unregister_netdevice_notifier(&hdd_netdev_notifier);

   /* Stop all adapters, this will ensure the termination of active
    * connections on the interface. Make sure the vos_scheduler is
    * still available to handle those control messages
    */
   TRACK_UNLOAD_STATUS(unload_stop_all_adapter);
   hdd_stop_all_adapters( pHddCtx );

#ifdef QCA_PKT_PROTO_TRACE
   if (VOS_FTM_MODE != hdd_get_conparam())
       vos_pkt_proto_trace_close();
#endif

   TRACK_UNLOAD_STATUS(unload_vos_stop);
   //Stop all the modules
   vosStatus = vos_stop( pVosContext );
   if (!VOS_IS_STATUS_SUCCESS(vosStatus))
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_FATAL,
         "%s: Failed to stop VOSS",__func__);
      VOS_ASSERT( VOS_IS_STATUS_SUCCESS( vosStatus ) );
   }

   //This requires pMac access, Call this before vos_close().
   hdd_unregister_mcast_bcast_filter(pHddCtx);

   TRACK_UNLOAD_STATUS(unload_vos_sched_close);
   //Close the scheduler before calling vos_close to make sure no thread is
   // scheduled after the each module close is called i.e after all the data
   // structures are freed.
   vosStatus = vos_sched_close( pVosContext );
   if (!VOS_IS_STATUS_SUCCESS(vosStatus))    {
      VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
         "%s: Failed to close VOSS Scheduler",__func__);
      VOS_ASSERT( VOS_IS_STATUS_SUCCESS( vosStatus ) );
   }
   /* Destroy the wake lock */
   vos_wake_lock_destroy(&pHddCtx->rx_wake_lock);
   /* Destroy the wake lock */
   vos_wake_lock_destroy(&pHddCtx->sap_wake_lock);

   hdd_hostapd_channel_wakelock_deinit(pHddCtx);

  TRACK_UNLOAD_STATUS(unload_vos_nv_close);
  vosStatus = vos_nv_close();
  if (!VOS_IS_STATUS_SUCCESS(vosStatus))
  {
     VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
         "%s: Failed to close NV", __func__);
     VOS_ASSERT( VOS_IS_STATUS_SUCCESS( vosStatus ) );
  }

   TRACK_UNLOAD_STATUS(unload_vos_close);
   //Close VOSS
   //This frees pMac(HAL) context. There should not be any call that requires pMac access after this.
   vos_close(pVosContext);

   TRACK_UNLOAD_STATUS(unload_deinit_greep_ap);
   hdd_wlan_green_ap_deinit(pHddCtx);

   hdd_easy_wow_deinit(pHddCtx);

   hdd_request_manager_deinit();

   //Close Watchdog
   if (pConfig && pConfig->fIsLogpEnabled)
      vos_watchdog_close(pVosContext);

   if (VOS_FTM_MODE != hdd_get_conparam()) {
       TRACK_UNLOAD_STATUS(unload_logging_sock_deactivate_svc);
       wlan_hdd_logging_sock_deactivate_svc(pHddCtx);
   }

#ifdef WLAN_FEATURE_LPSS
   TRACK_UNLOAD_STATUS(unload_hdd_send_status_pkg);
   wlan_hdd_send_status_pkg(NULL, NULL, 0, 0);
#endif
   TRACK_UNLOAD_STATUS(unload_close_cesium_nl_sock);
   hdd_close_cesium_nl_sock();

   TRACK_UNLOAD_STATUS(unload_runtime_suspend_deinit);
   hdd_runtime_suspend_deinit(pHddCtx);
   TRACK_UNLOAD_STATUS(unload_close_all_adapters);
   hdd_close_all_adapters( pHddCtx );

#ifdef IPA_OFFLOAD
   TRACK_UNLOAD_STATUS(unload_ipa_cleanup);
   hdd_ipa_cleanup(pHddCtx);
#endif

   /* free the power on lock from platform driver */
   if (free_riva_power_on_lock("wlan"))
   {
      hddLog(VOS_TRACE_LEVEL_ERROR, "%s: failed to free power on lock",
                                           __func__);
   }

   TRACK_UNLOAD_STATUS(unload_flush_roc_work);
   /* Free up RoC request queue and flush workqueue */
   vos_flush_work(&pHddCtx->rocReqWork);
   hdd_list_destroy(&pHddCtx->hdd_roc_req_q);

free_hdd_ctx:
   TRACK_UNLOAD_STATUS(unload_nl_srv_exit);
   nl_srv_exit();

   /* Free up dynamically allocated members inside HDD Adapter */
   if (pHddCtx->cfg_ini) {
       vos_mem_free(pHddCtx->cfg_ini);
       pHddCtx->cfg_ini= NULL;
   }

   wlan_hdd_deinit_chan_info(pHddCtx);
   wlan_hdd_deinit_tx_rx_histogram(pHddCtx);
   hdd_free_probe_req_ouis(pHddCtx);
   TRACK_UNLOAD_STATUS(unload_wiphy_unregister);
   wiphy_unregister(wiphy) ;
   wlan_hdd_cfg80211_deinit(wiphy);
   TRACK_UNLOAD_STATUS(unload_wiphy_free);
   wiphy_free(wiphy) ;
   if (hdd_is_ssr_required())
   {
#ifdef MSM_PLATFORM
#ifdef CONFIG_CNSS
       /* WDI timeout had happened during unload, so SSR is needed here */
       TRACK_UNLOAD_STATUS(unload_subsystem_restart);
       subsystem_restart("wcnss");
#endif
#endif
       msleep(5000);
   }
   hdd_set_ssr_required (VOS_FALSE);
   TRACK_UNLOAD_STATUS(unload_finish);
#ifdef CONFIG_VOS_MEM_PRE_ALLOC
   wcnss_prealloc_reset();
#endif /* CONFIG_VOS_MEM_PRE_ALLOC */
}

void __hdd_wlan_exit(void)
{
   hdd_context_t *pHddCtx = NULL;
   v_CONTEXT_t pVosContext = NULL;

   ENTER();

   //Get the global vos context
   pVosContext = vos_get_global_context(VOS_MODULE_ID_SYS, NULL);
   if (!pVosContext)
      return;
   if (WLAN_IS_EPPING_ENABLED(con_mode)) {
      epping_exit(pVosContext);
      return;
   }

   if(NULL == pVosContext) {
      VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_FATAL,
            "%s:Invalid global VOSS context", __func__);
      EXIT();
      return;
   }

   //Get the HDD context.
   pHddCtx = (hdd_context_t *)vos_get_context(VOS_MODULE_ID_HDD,
         pVosContext);

   if(NULL == pHddCtx) {
      VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_FATAL,
            "%s:Invalid HDD Context", __func__);
      EXIT();
      return;
   }

   /* module exit should never proceed if SSR is not completed */
   while(pHddCtx->isLogpInProgress){
      VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_FATAL,
            "%s:SSR in Progress; block rmmod for 1 second!!!",
            __func__);
      msleep(1000);
   }

   pHddCtx->isUnloadInProgress = TRUE;
   pHddCtx->driver_being_stopped = false;

   vos_set_load_unload_in_progress(VOS_MODULE_ID_VOSS, TRUE);
   vos_set_unload_in_progress(TRUE);

   hdd_close_tx_queues(pHddCtx);
   //Do all the cleanup before deregistering the driver
   hdd_driver_memdump_deinit();
/**
 * When rmmoding wlan driver with USB interface, crash_dump_exit
 * should be called in hif_usb_remove. Other interfaces do
 * crash_dump_exit here.
 */
#ifdef FW_RAM_DUMP_TO_PROC
#ifndef HIF_USB
   crash_dump_exit();
#endif
#endif
   hdd_wlan_exit(pHddCtx);
   EXIT();
}

#ifdef FEATURE_WLAN_AP_AP_ACS_OPTIMIZE
/**
 * hdd_skip_acs_scan_timer_handler() - skip ACS scan timer timeout handler
 * @data: pointer to hdd_context_t
 *
 * This function will reset acs_scan_status to eSAP_DO_NEW_ACS_SCAN.
 * Then new ACS request will do a fresh scan without reusing the cached
 * scan information.
 *
 * Return: void
 */
void hdd_skip_acs_scan_timer_handler(void * data)
{
	hdd_context_t *hdd_ctx = (hdd_context_t *) data;
	hdd_adapter_t *ap_adapter;

	hddLog(LOG1, FL("ACS Scan result expired. Reset ACS scan skip"));
	hdd_ctx->skip_acs_scan_status = eSAP_DO_NEW_ACS_SCAN;
	adf_os_spin_lock(&hdd_ctx->acs_skip_lock);
	vos_mem_free(hdd_ctx->last_acs_channel_list);
	hdd_ctx->last_acs_channel_list = NULL;
	hdd_ctx->num_of_channels = 0;
	adf_os_spin_unlock(&hdd_ctx->acs_skip_lock);

	/* Get first SAP adapter to clear results */
	ap_adapter = hdd_get_adapter(hdd_ctx, WLAN_HDD_SOFTAP);
	if (!hdd_ctx->hHal || !ap_adapter)
		return;
	sme_ScanFlushResult(hdd_ctx->hHal, ap_adapter->sessionId);
}
#endif

#ifdef QCA_HT_2040_COEX
/**--------------------------------------------------------------------------

  \brief notify FW with HT20/HT40 mode

  -------------------------------------------------------------------------*/
int hdd_wlan_set_ht2040_mode(hdd_adapter_t *pAdapter, v_U16_t staId,
                             v_MACADDR_t macAddrSTA, int channel_type)
{
   int status;
   VOS_STATUS vosStatus;
   hdd_context_t *pHddCtx = NULL;

   pHddCtx = WLAN_HDD_GET_CTX(pAdapter);

   status = wlan_hdd_validate_context(pHddCtx);
   if (0 != status)
       return -1;

   if (!pHddCtx->hHal)
      return -1;

   vosStatus = sme_notify_ht2040_mode(pHddCtx->hHal, staId, macAddrSTA,
                                      pAdapter->sessionId, channel_type);
   if (VOS_STATUS_SUCCESS != vosStatus) {
      hddLog(LOGE, "Fail to send notification with ht2040 mode\n");
      return -1;
   }

   return 0;
}
#endif


/**--------------------------------------------------------------------------

  \brief notify FW with modem power status

  -------------------------------------------------------------------------*/
int hdd_wlan_notify_modem_power_state(int state)
{
   int status;
   VOS_STATUS vosStatus;
   v_CONTEXT_t pVosContext = NULL;
   hdd_context_t *pHddCtx = NULL;

   pVosContext = vos_get_global_context(VOS_MODULE_ID_SYS, NULL);
   if (!pVosContext)
      return -1;

   pHddCtx = (hdd_context_t *)vos_get_context(VOS_MODULE_ID_HDD, pVosContext);

   status = wlan_hdd_validate_context(pHddCtx);
   if (0 != status)
       return -1;

   if (!pHddCtx->hHal)
      return -1;

   vosStatus = sme_notify_modem_power_state(pHddCtx->hHal, state);
   if (VOS_STATUS_SUCCESS != vosStatus) {
      hddLog(LOGE, "Fail to send notification with modem power state %d",
             state);
      return -1;
   }
   return 0;
}

/**---------------------------------------------------------------------------

  \brief hdd_post_voss_start_config() - HDD post voss start config helper

  \param  - pAdapter - Pointer to the HDD

  \return - None

  --------------------------------------------------------------------------*/
VOS_STATUS hdd_post_voss_start_config(hdd_context_t* pHddCtx)
{
   eHalStatus halStatus;
   v_U32_t listenInterval;
   tANI_U32    ignoreDtim;


   // Send ready indication to the HDD.  This will kick off the MAC
   // into a 'running' state and should kick off an initial scan.
   halStatus = sme_HDDReadyInd( pHddCtx->hHal );
   if ( !HAL_STATUS_SUCCESS( halStatus ) )
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: sme_HDDReadyInd() failed with status "
          "code %08d [x%08x]",__func__, halStatus, halStatus );
      return VOS_STATUS_E_FAILURE;
   }

   // Set default LI and ignoreDtim into HDD context,
   // otherwise under some race condition, HDD will set 0 LI value into RIVA,
   // And RIVA will crash
   wlan_cfgGetInt(pHddCtx->hHal, WNI_CFG_LISTEN_INTERVAL, &listenInterval);
   pHddCtx->hdd_actual_LI_value = listenInterval;
   wlan_cfgGetInt(pHddCtx->hHal, WNI_CFG_IGNORE_DTIM, &ignoreDtim);
   pHddCtx->hdd_actual_ignore_DTIM_value = ignoreDtim;


   return VOS_STATUS_SUCCESS;
}

/* wake lock APIs for HDD */
void hdd_prevent_suspend(uint32_t reason)
{
	vos_wake_lock_acquire(&wlan_wake_lock, reason);
}

void hdd_allow_suspend(uint32_t reason)
{
	vos_wake_lock_release(&wlan_wake_lock, reason);
}

void hdd_prevent_suspend_timeout(v_U32_t timeout, uint32_t reason)
{
	vos_wake_lock_timeout_acquire(&wlan_wake_lock, timeout,
                                      reason);
}

/**
 * hdd_wlan_wakelock_create() -Create wakelock named as wlan
 *
 * Return: none
 */
void hdd_wlan_wakelock_create(void)
{
	vos_wake_lock_init(&wlan_wake_lock, "wlan");
}

/**
 * hdd_wlan_wakelock_destroy() -Destroy wakelock named as wlan
 *
 * Return: none
 */
void hdd_wlan_wakelock_destroy(void)
{
	vos_wake_lock_destroy(&wlan_wake_lock);
}

/**
 * wlan_hdd_wakelocks_destroy() -Destroy all the wakelocks
 * @hdd_ctx: hdd context
 *
 * Return: none
 */
void wlan_hdd_wakelocks_destroy(hdd_context_t *hdd_ctx)
{
	if (hdd_ctx) {
		vos_wake_lock_destroy(&hdd_ctx->rx_wake_lock);
		vos_wake_lock_destroy(&hdd_ctx->sap_wake_lock);
		hdd_hostapd_channel_wakelock_deinit(hdd_ctx);
	}
}

/**
 * wlan_hdd_netdev_notifiers_cleanup() -unregister notifiers with kernel
 * @hdd_ctx: hdd context
 *
 * Return: none
 */
void wlan_hdd_netdev_notifiers_cleanup(hdd_context_t * hdd_ctx)
{
	if (hdd_ctx) {
		hddLog(LOGE, FL("Unregister IPv6 notifier"));
		hdd_wlan_unregister_ip6_notifier(hdd_ctx);
		hddLog(LOGE, FL("Unregister IPv4 notifier"));
		unregister_inetaddr_notifier(&hdd_ctx->ipv4_notifier);
	}
	unregister_netdevice_notifier(&hdd_netdev_notifier);
}

/**---------------------------------------------------------------------------

  \brief hdd_exchange_version_and_caps() - HDD function to exchange version and capability
                                                                 information between Host and Riva

  This function gets reported version of FW
  It also finds the version of Riva headers used to compile the host
  It compares the above two and prints a warning if they are different
  It gets the SW and HW version string
  Finally, it exchanges capabilities between host and Riva i.e. host and riva exchange a msg
  indicating the features they support through a bitmap

  \param  - pHddCtx - Pointer to HDD context

  \return -  void

  --------------------------------------------------------------------------*/

void hdd_exchange_version_and_caps(hdd_context_t *pHddCtx)
{

   tSirVersionType versionCompiled;
   tSirVersionType versionReported;
   tSirVersionString versionString;
   tANI_U8 fwFeatCapsMsgSupported = 0;
   VOS_STATUS vstatus;

   memset(&versionCompiled, 0, sizeof(versionCompiled));
   memset(&versionReported, 0, sizeof(versionReported));

   /* retrieve and display WCNSS version information */
   do {

      vstatus = sme_GetWcnssWlanCompiledVersion(pHddCtx->hHal,
                                                &versionCompiled);
      if (!VOS_IS_STATUS_SUCCESS(vstatus))
      {
         hddLog(VOS_TRACE_LEVEL_FATAL,
                "%s: unable to retrieve WCNSS WLAN compiled version",
                __func__);
         break;
      }

      vstatus = sme_GetWcnssWlanReportedVersion(pHddCtx->hHal,
                                                &versionReported);
      if (!VOS_IS_STATUS_SUCCESS(vstatus))
      {
         hddLog(VOS_TRACE_LEVEL_FATAL,
                "%s: unable to retrieve WCNSS WLAN reported version",
                __func__);
         break;
      }

      if ((versionCompiled.major != versionReported.major) ||
          (versionCompiled.minor != versionReported.minor) ||
          (versionCompiled.version != versionReported.version) ||
          (versionCompiled.revision != versionReported.revision))
      {
         pr_err("%s: WCNSS WLAN Version %u.%u.%u.%u, "
                "Host expected %u.%u.%u.%u\n",
                WLAN_MODULE_NAME,
                (int)versionReported.major,
                (int)versionReported.minor,
                (int)versionReported.version,
                (int)versionReported.revision,
                (int)versionCompiled.major,
                (int)versionCompiled.minor,
                (int)versionCompiled.version,
                (int)versionCompiled.revision);
      }
      else
      {
         pr_info("%s: WCNSS WLAN version %u.%u.%u.%u\n",
                 WLAN_MODULE_NAME,
                 (int)versionReported.major,
                 (int)versionReported.minor,
                 (int)versionReported.version,
                 (int)versionReported.revision);
      }

      vstatus = sme_GetWcnssSoftwareVersion(pHddCtx->hHal,
                                            versionString,
                                            sizeof(versionString));
      if (!VOS_IS_STATUS_SUCCESS(vstatus))
      {
         hddLog(VOS_TRACE_LEVEL_FATAL,
                "%s: unable to retrieve WCNSS software version string",
                __func__);
         break;
      }

      pr_info("%s: WCNSS software version %s\n",
              WLAN_MODULE_NAME, versionString);

      vstatus = sme_GetWcnssHardwareVersion(pHddCtx->hHal,
                                            versionString,
                                            sizeof(versionString));
      if (!VOS_IS_STATUS_SUCCESS(vstatus))
      {
         hddLog(VOS_TRACE_LEVEL_FATAL,
                "%s: unable to retrieve WCNSS hardware version string",
                __func__);
         break;
      }

      pr_info("%s: WCNSS hardware version %s\n",
              WLAN_MODULE_NAME, versionString);

      /* 1.Check if FW version is greater than 0.1.1.0. Only then send host-FW capability exchange message
         2.Host-FW capability exchange message  is only present on riva 1.1 so
            send the message only if it the riva is 1.1
            minor numbers for different riva branches:
                0 -> (1.0)Mainline Build
                1 -> (1.1)Mainline Build
                2->(1.04) Stability Build
       */
      if (((versionReported.major>0) || (versionReported.minor>1) ||
         ((versionReported.minor>=1) && (versionReported.version>=1)))
         && ((versionReported.major == 1) && (versionReported.minor >= 1)))
         fwFeatCapsMsgSupported = 1;

      if (fwFeatCapsMsgSupported)
      {
#ifdef WLAN_ACTIVEMODE_OFFLOAD_FEATURE
         if(!pHddCtx->cfg_ini->fEnableActiveModeOffload)
            sme_disableFeatureCapablity(WLANACTIVE_OFFLOAD);
#endif
         /* Indicate if IBSS heartbeat monitoring needs to be offloaded */
         if (!pHddCtx->cfg_ini->enableIbssHeartBeatOffload)
         {
            sme_disableFeatureCapablity(IBSS_HEARTBEAT_OFFLOAD);
         }

         sme_featureCapsExchange(pHddCtx->hHal);
      }

   } while (0);

}

/* Initialize channel list in sme based on the country code */
VOS_STATUS hdd_set_sme_chan_list(hdd_context_t *hdd_ctx)
{
    return sme_init_chan_list(hdd_ctx->hHal, hdd_ctx->reg.alpha2,
                              hdd_ctx->reg.cc_src);
}

void hdd_set_dfs_regdomain(hdd_context_t *phddctx, bool restore)
{
    if(!restore) {
        if (vos_nv_get_dfs_region(&phddctx->hdd_dfs_regdomain)) {
             hddLog(VOS_TRACE_LEVEL_FATAL,
                    "%s: unable to retrieve dfs region from hdd",
                    __func__);
        }
    }
    else {
        if (vos_nv_set_dfs_region(phddctx->hdd_dfs_regdomain)) {
             hddLog(VOS_TRACE_LEVEL_FATAL,
                    "%s: unable to set dfs region",
                    __func__);
        }
    }
}

/**
 * hdd_is_5g_supported() - to know if ini configuration supports 5GHz
 * @pHddCtx: Pointer to the hdd context
 *
 * Return: true if ini configuration supports 5GHz
 */
boolean hdd_is_5g_supported(hdd_context_t * pHddCtx)
{
	/**
	 * If wcnss_wlan_iris_xo_mode() returns WCNSS_XO_48MHZ(1);
	 * then hardware support 5Ghz.
	 */
	if(!pHddCtx || !pHddCtx->cfg_ini)
		return true;

	if (pHddCtx->cfg_ini->nBandCapability != eCSR_BAND_24)
		return true;
	else
		return false;
}

static VOS_STATUS wlan_hdd_reg_init(hdd_context_t *hdd_ctx)
{
   struct wiphy *wiphy;
   VOS_STATUS status = VOS_STATUS_SUCCESS;

   wiphy = hdd_ctx->wiphy;

   /* initialize the NV module. This is required so that
      we can initialize the channel information in wiphy
      from the NV.bin data. The channel information in
      wiphy needs to be initialized before wiphy registration */

   status = vos_init_wiphy_from_eeprom();
   if (!VOS_IS_STATUS_SUCCESS(status))
   {
      /* NV module cannot be initialized */
      hddLog( VOS_TRACE_LEVEL_FATAL,
            "%s: vos_init_wiphy failed", __func__);
      return status;
   }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,11,0)) || defined(WITH_BACKPORTS)
    wiphy->wowlan = &wowlan_support_reg_init;
    wiphy->wowlan_config = kmemdup(&wowlan_config, sizeof(wowlan_config), GFP_KERNEL);
#else
    wiphy->wowlan.flags = WIPHY_WOWLAN_ANY |
                          WIPHY_WOWLAN_MAGIC_PKT |
                          WIPHY_WOWLAN_DISCONNECT |
                          WIPHY_WOWLAN_SUPPORTS_GTK_REKEY |
                          WIPHY_WOWLAN_GTK_REKEY_FAILURE |
                          WIPHY_WOWLAN_EAP_IDENTITY_REQ |
                          WIPHY_WOWLAN_4WAY_HANDSHAKE |
                          WIPHY_WOWLAN_RFKILL_RELEASE;

    wiphy->wowlan.n_patterns = (WOW_MAX_FILTER_LISTS *
                          WOW_MAX_FILTERS_PER_LIST);
    wiphy->wowlan.pattern_min_len = WOW_MIN_PATTERN_SIZE;
    wiphy->wowlan.pattern_max_len = WOW_MAX_PATTERN_SIZE;
#endif

   return status;
}

#ifdef CONFIG_IXC_TIMER
#ifdef CONFIG_PERF_MODE
#define wmem_max "/proc/sys/net/core/wmem_max"
#define wmem_default "/proc/sys/net/core/wmem_default"
#define rmem_max "/proc/sys/net/core/rmem_max"
#define rmem_default "/proc/sys/net/core/rmem_default"
#define tcp_mem "/proc/sys/net/ipv4/tcp_mem"
#define tcp_rmem "/proc/sys/net/ipv4/tcp_rmem"
#define tcp_wmem "/proc/sys/net/ipv4/wcp_rmem"
#define tcp_limit_output_bytes "/proc/sys/net/ipv4/tcp_limit_output_bytes"
#define flush "/proc/sys/net/ipv4/route/flush"
#define tcp_sack "/proc/sys/net/ipv4/tcp_sack"
#define GET_INODE_FROM_FILEP(filp) \
	(filp)->f_path.dentry->d_inode

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
			ret = vfs_write(
				filp, wbuf, length, &filp->f_pos);
			if (ret < 0) {
				printk(KERN_ERR
					"_readwrite_file: Error 3\n");
				break;
			}
		} else {
			ret = vfs_read(
				filp, rbuf, length, &filp->f_pos);
			if (ret < 0) {
				printk(KERN_ERR
					"_readwrite_file: Error 4\n");
				break;
			}
		}
	} while (0);

	if (!IS_ERR(filp)) {
		filp_close(filp, NULL);
	}

	set_fs(oldfs);
	return ret;
}

void write_sys_file(const char* filename, char* buf, unsigned int len)
{
	int ret;
	ret = _readwrite_file(filename, NULL,
	                      buf,
	                      len,
	                      (O_WRONLY | O_APPEND));
	if (ret < 0) {
		printk("%s:%d: fail to write %s\n", __func__, __LINE__, filename);
	}
}

void enable_wlan_perf_mode(void)
{
	int i,cpu_num;
	char file_name[128];

	write_sys_file(wmem_max, "8000000", 7);
	write_sys_file(wmem_default, "8000000", 7);
	write_sys_file(rmem_max, "8000000", 7);
	write_sys_file(rmem_default, "8000000", 7);

	write_sys_file(tcp_mem, "8000000 8000000 8000000", 23);

	write_sys_file(tcp_rmem, "10240 87380 8000000", 19);
	write_sys_file(tcp_wmem, "10240 87380 8000000", 19);
	write_sys_file(tcp_limit_output_bytes, "1048576", 7);
	write_sys_file(flush, "1", 1);
	write_sys_file(tcp_sack, "0", 1);
	write_sys_file("/proc/sys/nbet/ipv4/tcp_use_rconfig", "1", 1);
	//write_sys_file("/proc/sys/nbet/ipv4/tcp_delack_seg", "60", 2);

	cpu_num = num_possible_cpus();
	for(i=0; i < cpu_num; i++) {
		snprintf(file_name, sizeof(file_name), "/sys/bus/cpu/devices/cpu%d/online", i);
		write_sys_file(file_name, "1", 1);
		snprintf(file_name, sizeof(file_name), "/sys/bus/cpu/devices/cpu%d/cpufreq/scaling_governor", i);
		write_sys_file(file_name, "performance", 11);
	}
}
#endif
static void hdd_set_ixc_prio(void *priv)
{
    int i;
    struct pid* pid;
    struct task_struct* task;
    char comm[16];
    struct sched_param param = {.sched_priority = 99};
    hdd_context_t *pHddCtx = (hdd_context_t *)priv;

    if(pHddCtx->ixc_pid != 0) {
        pid = find_get_pid(pHddCtx->ixc_pid);
        if(pid) {
            task = get_pid_task(pid, 0);
            if(task){
                memset(comm, 0, sizeof(comm));
                strlcpy(comm, task->comm, sizeof(comm));
                if(strstr(comm, "ixchariot") || strstr(comm, "iperf")) {
                    vos_timer_start(&pHddCtx->set_ixc_prio_timer, 10);
                    //we already set this task priority
                    return;
                }
            }
            task = NULL;
        }
    }
    for(i = 2; i < 32767; i++) {
        pid = find_get_pid(i);
        if(pid) {
            task = get_pid_task(pid, 0);
            if(task){
                memset(comm, 0, sizeof(comm));
                strlcpy(comm, task->comm, sizeof(comm));
                if(strstr(comm, "ixchariot") || strstr(comm, "iperf")) {
                    sched_setscheduler(task, SCHED_FIFO, &param);
#ifdef CONFIG_PERF_MODE
                    enable_wlan_perf_mode();
#endif
                    pHddCtx->ixc_pid = i;
                    printk("set ixc prio, pid is %d\n", pHddCtx->ixc_pid);
                    vos_timer_start(&pHddCtx->set_ixc_prio_timer, 10);
                    return;
                }
            }
            task = NULL;
        }
    }
    vos_timer_start(&pHddCtx->set_ixc_prio_timer, 10);
}
#endif
#ifdef FEATURE_BUS_BANDWIDTH
#ifdef QCA_SUPPORT_TXRX_HL_BUNDLE
static void hdd_set_bundle_require(uint16_t session_id, hdd_context_t *hdd_ctx,
				   uint64_t tx_bytes)
{
	tlshim_set_bundle_require(session_id, tx_bytes,
		hdd_ctx->cfg_ini->busBandwidthComputeInterval,
		hdd_ctx->cfg_ini->pkt_bundle_threshold_high,
		hdd_ctx->cfg_ini->pkt_bundle_threshold_low);

}
#else
static void hdd_set_bundle_require(uint16_t session_id, hdd_context_t *hdd_ctx,
				   uint64_t tx_bytes)
{
	return;
}
#endif


void hdd_cnss_request_bus_bandwidth(hdd_context_t *pHddCtx,
        const uint64_t tx_packets, const uint64_t rx_packets)
{
    uint64_t total = tx_packets + rx_packets;
    uint64_t temp_rx = 0;
    uint64_t temp_tx = 0;
    enum cnss_bus_width_type next_vote_level = CNSS_BUS_WIDTH_NONE;
    enum wlan_tp_level next_rx_level = WLAN_SVC_TP_NONE;
    enum wlan_tp_level next_tx_level = WLAN_SVC_TP_NONE;
    struct device *dev = pHddCtx->parent_dev;

    if (total > pHddCtx->cfg_ini->busBandwidthHighThreshold)
        next_vote_level = CNSS_BUS_WIDTH_HIGH;
    else if (total > pHddCtx->cfg_ini->busBandwidthMediumThreshold)
        next_vote_level = CNSS_BUS_WIDTH_MEDIUM;
    else if (total > pHddCtx->cfg_ini->busBandwidthLowThreshold)
        next_vote_level = CNSS_BUS_WIDTH_LOW;
    else
        next_vote_level = CNSS_BUS_WIDTH_NONE;

    pHddCtx->hdd_txrx_hist[pHddCtx->hdd_txrx_hist_idx].next_vote_level
                                                            = next_vote_level;

    if (pHddCtx->cur_vote_level != next_vote_level) {
        hddLog(VOS_TRACE_LEVEL_DEBUG,
               "%s: trigger level %d, tx_packets: %lld, rx_packets: %lld",
               __func__, next_vote_level, tx_packets, rx_packets);
        pHddCtx->cur_vote_level = next_vote_level;
        vos_request_bus_bandwidth(dev, next_vote_level);

        if (next_vote_level <= CNSS_BUS_WIDTH_LOW) {
            if (pHddCtx->hbw_requested) {
                vos_remove_pm_qos();
                pHddCtx->hbw_requested = false;
            }
            if (vos_sched_handle_throughput_req(false))
                hddLog(LOGE, FL("low bandwidth set rx affinity fail"));
        } else {
            if (!pHddCtx->hbw_requested) {
                vos_request_pm_qos_type(PM_QOS_CPU_DMA_LATENCY,
                                      DISABLE_KRAIT_IDLE_PS_VAL);
                pHddCtx->hbw_requested = true;
            }
            if (vos_sched_handle_throughput_req(true))
                hddLog(LOGE, FL("high bandwidth set rx affinity fail"));
        }
    }

    /* fine-tuning parameters for RX Flows */
    temp_rx = (rx_packets + pHddCtx->prev_rx) / 2;
    pHddCtx->prev_rx = rx_packets;
    if (temp_rx > pHddCtx->cfg_ini->tcpDelackThresholdHigh) {
        if ((pHddCtx->cur_rx_level != WLAN_SVC_TP_HIGH) &&
        (++pHddCtx->rx_high_ind_cnt == pHddCtx->cfg_ini->tcpDelackTimerCount)) {
            next_rx_level = WLAN_SVC_TP_HIGH;
        }
    } else {
            next_rx_level = WLAN_SVC_TP_LOW;
            pHddCtx->rx_high_ind_cnt = 0;
    }

    pHddCtx->hdd_txrx_hist[pHddCtx->hdd_txrx_hist_idx].next_rx_level
                                                          = next_rx_level;

    if (pHddCtx->cur_rx_level != next_rx_level) {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_DEBUG,
               "%s: TCP DELACK trigger level %d, average_rx: %llu",
               __func__, next_rx_level, temp_rx);
        pHddCtx->cur_rx_level = next_rx_level;
#ifdef QCA_SUPPORT_TXRX_DRIVER_TCP_DEL_ACK
        if (pHddCtx->cfg_ini->del_ack_enable)
            next_rx_level = WLAN_SVC_TP_LOW;
#endif
        wlan_hdd_send_svc_nlink_msg(pHddCtx->radio_index,
                                    WLAN_SVC_WLAN_TP_IND,
                                    &next_rx_level,
                                    sizeof(next_rx_level));
    }

    /* fine-tuning parameters for TX Flows */
    temp_tx = (tx_packets + pHddCtx->prev_tx) / 2;
    pHddCtx->prev_tx = tx_packets;
    if (temp_tx > pHddCtx->cfg_ini->tcp_tx_high_tput_thres)
        next_tx_level = WLAN_SVC_TP_HIGH;
    else
        next_tx_level = WLAN_SVC_TP_LOW;

    if (pHddCtx->cur_tx_level != next_tx_level) {
        hddLog(VOS_TRACE_LEVEL_DEBUG,
               "%s: change TCP TX trigger level %d, average_tx: %llu ",
               __func__, next_tx_level, temp_tx);
        pHddCtx->cur_tx_level = next_tx_level;
        wlan_hdd_send_svc_nlink_msg(pHddCtx->radio_index,
                                    WLAN_SVC_WLAN_TP_TX_IND,
                                    &next_tx_level,
                                    sizeof(next_tx_level));
    }

    pHddCtx->hdd_txrx_hist[pHddCtx->hdd_txrx_hist_idx].next_tx_level
                                                          = next_tx_level;

    pHddCtx->hdd_txrx_hist_idx++;
    pHddCtx->hdd_txrx_hist_idx &= NUM_TX_RX_HISTOGRAM_MASK;
}

static void hdd_bus_bw_compute_cbk(void *priv)
{
    hdd_context_t *pHddCtx = (hdd_context_t *)priv;
    hdd_adapter_t *pAdapter = NULL;
    uint64_t tx_packets = 0, rx_packets = 0, tx_bytes = 0;
    unsigned long fwd_tx_packets = 0, fwd_rx_packets = 0;
    unsigned long fwd_tx_packets_diff = 0, fwd_rx_packets_diff = 0;
    uint64_t total_tx = 0, total_rx = 0;
    hdd_adapter_list_node_t *pAdapterNode = NULL;
    VOS_STATUS status = 0;
    A_STATUS ret;
    v_BOOL_t connected = FALSE;
#ifdef IPA_UC_OFFLOAD
    uint32_t ipa_tx_packets = 0, ipa_rx_packets = 0;
    hdd_adapter_t *pValidAdapter = NULL;
#endif /* IPA_UC_OFFLOAD */

    if (wlan_hdd_validate_context(pHddCtx))
        return;

    for (status = hdd_get_front_adapter(pHddCtx, &pAdapterNode);
            NULL != pAdapterNode && VOS_STATUS_SUCCESS == status;
            status = hdd_get_next_adapter(pHddCtx, pAdapterNode, &pAdapterNode))
    {

        if ((pAdapter = pAdapterNode->pAdapter) == NULL)
            continue;
        /* Validate magic so we don't end up accessing a freed adapter.*/
        if (pAdapter->magic != WLAN_HDD_ADAPTER_MAGIC)
            continue;

#ifdef IPA_UC_OFFLOAD
        if (NULL == pValidAdapter)
                pValidAdapter = pAdapter;
#endif /* IPA_UC_OFFLOAD */

        if ((pAdapter->device_mode == WLAN_HDD_INFRA_STATION ||
                    pAdapter->device_mode == WLAN_HDD_P2P_CLIENT) &&
                WLAN_HDD_GET_STATION_CTX_PTR(pAdapter)->conn_info.connState
                != eConnectionState_Associated) {

            continue;
        }

        if ((pAdapter->device_mode == WLAN_HDD_SOFTAP ||
                    pAdapter->device_mode == WLAN_HDD_P2P_GO) &&
                WLAN_HDD_GET_AP_CTX_PTR(pAdapter)->bApActive == VOS_FALSE) {

            continue;
        }

        tx_packets += HDD_BW_GET_DIFF(pAdapter->stats.tx_packets,
                pAdapter->prev_tx_packets);
        tx_bytes += HDD_BW_GET_DIFF(pAdapter->stats.tx_bytes,
                pAdapter->prev_tx_bytes);
        rx_packets += HDD_BW_GET_DIFF(pAdapter->stats.rx_packets,
                pAdapter->prev_rx_packets);

        if (pAdapter->device_mode == WLAN_HDD_SOFTAP ||
            pAdapter->device_mode == WLAN_HDD_P2P_GO ||
            pAdapter->device_mode == WLAN_HDD_IBSS) {

            ret = tlshim_get_intra_bss_fwd_pkts_count(pAdapter->sessionId,
                             &fwd_tx_packets, &fwd_rx_packets);
            if (ret == A_OK) {
                fwd_tx_packets_diff += HDD_BW_GET_DIFF(fwd_tx_packets,
                    pAdapter->prev_fwd_tx_packets);
                fwd_rx_packets_diff += HDD_BW_GET_DIFF(fwd_tx_packets,
                    pAdapter->prev_fwd_rx_packets);
            }
        }

        hdd_set_bundle_require(pAdapter->sessionId, pHddCtx, tx_bytes);
        hdd_set_driver_del_ack_enable(pAdapter->sessionId, pHddCtx, rx_packets);

        total_rx += pAdapter->stats.rx_packets;
        total_tx += pAdapter->stats.tx_packets;


        adf_os_spin_lock_bh(&pHddCtx->bus_bw_lock);
        pAdapter->prev_tx_packets = pAdapter->stats.tx_packets;
        pAdapter->prev_tx_bytes = pAdapter->stats.tx_bytes;
        pAdapter->prev_rx_packets = pAdapter->stats.rx_packets;
        pAdapter->prev_fwd_tx_packets = fwd_tx_packets;
        pAdapter->prev_fwd_rx_packets = fwd_rx_packets;
        adf_os_spin_unlock_bh(&pHddCtx->bus_bw_lock);
        connected = TRUE;
    }

    pHddCtx->hdd_txrx_hist[pHddCtx->hdd_txrx_hist_idx].total_rx = total_rx;
    pHddCtx->hdd_txrx_hist[pHddCtx->hdd_txrx_hist_idx].total_tx = total_tx;
    pHddCtx->hdd_txrx_hist[pHddCtx->hdd_txrx_hist_idx].interval_rx = rx_packets;
    pHddCtx->hdd_txrx_hist[pHddCtx->hdd_txrx_hist_idx].interval_tx = tx_packets;

    /* add intra bss forwarded tx and rx packets */
    tx_packets += fwd_tx_packets_diff;
    rx_packets += fwd_rx_packets_diff;

#ifdef IPA_UC_OFFLOAD
    hdd_ipa_uc_stat_query(pHddCtx, &ipa_tx_packets, &ipa_rx_packets);
    tx_packets += (uint64_t)ipa_tx_packets;
    rx_packets += (uint64_t)ipa_rx_packets;

    if (pValidAdapter) {
        pValidAdapter->stats.tx_packets += ipa_tx_packets;
        pValidAdapter->stats.rx_packets += ipa_rx_packets;
    }
#endif /* IPA_UC_OFFLOAD */
    if (!connected) {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "bus bandwidth timer running in disconnected state");
        return;
    }

    hdd_cnss_request_bus_bandwidth(pHddCtx, tx_packets, rx_packets);

#ifdef IPA_OFFLOAD
    hdd_ipa_set_perf_level(pHddCtx, tx_packets, rx_packets);
#ifdef IPA_UC_OFFLOAD
    hdd_ipa_uc_stat_request(pValidAdapter, 2);
#endif /* IPA_UC_OFFLOAD */
#endif

    vos_timer_start(&pHddCtx->bus_bw_timer,
            pHddCtx->cfg_ini->busBandwidthComputeInterval);
}
#endif

/**
 * wlan_hdd_init_tx_rx_histogram() - init tx/rx histogram stats
 * @pHddCtx: hdd context
 *
 * Return: 0 for success
 */
int wlan_hdd_init_tx_rx_histogram(hdd_context_t *pHddCtx)
{
	pHddCtx->hdd_txrx_hist = vos_mem_malloc(
		 (sizeof(struct hdd_tx_rx_histogram) * NUM_TX_RX_HISTOGRAM));
	if (pHddCtx->hdd_txrx_hist == NULL) {
		hddLog(VOS_TRACE_LEVEL_FATAL,
			"%s: Failed malloc for hdd_txrx_hist",__func__);
		return -ENOMEM;
	}
	return 0;
}

/**
 * wlan_hdd_deinit_tx_rx_histogram() - deinit tx/rx histogram stats
 * @pHddCtx: hdd context
 *
 * Return: none
 */
void wlan_hdd_deinit_tx_rx_histogram(hdd_context_t *pHddCtx)
{
	if (pHddCtx->hdd_txrx_hist) {
		vos_mem_free(pHddCtx->hdd_txrx_hist);
		pHddCtx->hdd_txrx_hist = NULL;
	}
}

void wlan_hdd_display_tx_rx_histogram(hdd_context_t *pHddCtx)
{
    int i;

#ifdef FEATURE_BUS_BANDWIDTH
    hddLog(VOS_TRACE_LEVEL_ERROR, "BW Interval: %d curr_index %d",
                pHddCtx->cfg_ini->busBandwidthComputeInterval,
                pHddCtx->hdd_txrx_hist_idx);
    hddLog(VOS_TRACE_LEVEL_ERROR, "BW High TH: %d BW Med TH: %d BW Low TH: %d",
                pHddCtx->cfg_ini->busBandwidthHighThreshold,
                pHddCtx->cfg_ini->busBandwidthMediumThreshold,
                pHddCtx->cfg_ini->busBandwidthLowThreshold);
    hddLog(VOS_TRACE_LEVEL_ERROR, "TCP DEL High TH: %d TCP DEL Low TH: %d",
                pHddCtx->cfg_ini->tcpDelackThresholdHigh,
                pHddCtx->cfg_ini->tcpDelackThresholdLow);
#endif

    hddLog(VOS_TRACE_LEVEL_ERROR,
            "index, total_rx, interval_rx, total_tx, interval_tx, next_vote_level, next_rx_level, next_tx_level");
    for (i=0; i < NUM_TX_RX_HISTOGRAM; i++){
        hddLog(VOS_TRACE_LEVEL_ERROR,
               "%d: %llu, %llu, %llu, %llu, %d, %d, %d",
               i, pHddCtx->hdd_txrx_hist[i].total_rx,
               pHddCtx->hdd_txrx_hist[i].interval_rx,
               pHddCtx->hdd_txrx_hist[i].total_tx,
               pHddCtx->hdd_txrx_hist[i].interval_tx,
               pHddCtx->hdd_txrx_hist[i].next_vote_level,
               pHddCtx->hdd_txrx_hist[i].next_rx_level,
               pHddCtx->hdd_txrx_hist[i].next_tx_level);
    }
    return;
}

void wlan_hdd_clear_tx_rx_histogram(hdd_context_t *pHddCtx)
{
    pHddCtx->hdd_txrx_hist_idx = 0;
    vos_mem_zero(pHddCtx->hdd_txrx_hist,
        (sizeof(struct hdd_tx_rx_histogram) * NUM_TX_RX_HISTOGRAM));
}

/**
 * wlan_hdd_display_adapter_netif_queue_history() - display adapter's netif
 *                                                  queue operation history
 * @adapter: hdd adapter
 *
 * Return: none
 */
static inline void
wlan_hdd_display_adapter_netif_queue_history(hdd_adapter_t *adapter)
{
	int i;
	adf_os_time_t total, pause, unpause, curr_time;

	if (!adapter)
		return;
	hddLog(LOGE, FL("Session_id %d device mode %d"),
		adapter->sessionId, adapter->device_mode);

	hddLog(LOGE, "Netif queue operation statistics:");
	hddLog(LOGE, "Current pause_map value %x", adapter->pause_map);
	curr_time = vos_system_ticks();
	total = curr_time - adapter->start_time;
	if (adapter->pause_map) {
		pause = adapter->total_pause_time +
			curr_time - adapter->last_time;
		unpause = adapter->total_unpause_time;
	} else {
		unpause = adapter->total_unpause_time +
			curr_time - adapter->last_time;
		pause = adapter->total_pause_time;
	}
	hddLog(LOGE, "Total: %ums Pause: %ums Unpause: %ums",
		vos_system_ticks_to_msecs(total),
		vos_system_ticks_to_msecs(pause),
		vos_system_ticks_to_msecs(unpause));
	hddLog(LOGE, "  reason_type: pause_cnt: unpause_cnt");

	for (i = 0; i < WLAN_REASON_TYPE_MAX; i++) {
		hddLog(LOGE, "%s: %d: %d",
		       hdd_reason_type_to_string(i),
		       adapter->queue_oper_stats[i].pause_count,
		       adapter->queue_oper_stats[i].unpause_count);
	}

	hddLog(LOGE, "Netif queue operation history: current index %d",
		adapter->history_index);
	hddLog(LOGE, "index: time: action_type: reason_type: pause_map");

	for (i = 0; i < WLAN_HDD_MAX_HISTORY_ENTRY; i++) {
		hddLog(LOGE, "%d: %u: %s: %s: %x",
		       i, vos_system_ticks_to_msecs(
				adapter->queue_oper_history[i].time),
		       hdd_action_type_to_string(
				adapter->queue_oper_history[i].netif_action),
		       hdd_reason_type_to_string(
				adapter->queue_oper_history[i].netif_reason),
		       adapter->queue_oper_history[i].pause_map);
	}
}

/**
 * wlan_hdd_display_netif_queue_history() - display netif queue
 *                                          operation history
 * @hdd_ctx: hdd context
 *
 * Return: none
 */
void wlan_hdd_display_netif_queue_history(hdd_context_t *hdd_ctx)
{
	hdd_adapter_t *adapter;
	hdd_adapter_list_node_t *adapter_node = NULL, *next = NULL;
	VOS_STATUS status;

	status = hdd_get_front_adapter(hdd_ctx, &adapter_node);
	while (NULL != adapter_node && VOS_STATUS_SUCCESS == status) {
		adapter = adapter_node->pAdapter;

		wlan_hdd_display_adapter_netif_queue_history(adapter);

		status = hdd_get_next_adapter(hdd_ctx, adapter_node, &next);
		adapter_node = next;
	}
}

/**
 * wlan_hdd_clear_netif_queue_history() - clear netif queue operation history
 * @hdd_ctx: hdd context
 *
 * Return: none
 */
void wlan_hdd_clear_netif_queue_history(hdd_context_t *hdd_ctx)
{
	hdd_adapter_t *adapter = NULL;
	hdd_adapter_list_node_t *adapter_node = NULL, *next = NULL;
	VOS_STATUS status;

	status = hdd_get_front_adapter(hdd_ctx, &adapter_node);
	while (NULL != adapter_node && VOS_STATUS_SUCCESS == status) {
		adapter = adapter_node->pAdapter;

		vos_mem_zero(adapter->queue_oper_stats,
			     sizeof(adapter->queue_oper_stats));
		vos_mem_zero(adapter->queue_oper_history,
			     sizeof(adapter->queue_oper_history));

		adapter->history_index = 0;
		adapter->start_time = adapter->last_time = vos_system_ticks();
		adapter->total_pause_time = 0;
		adapter->total_unpause_time = 0;
		status = hdd_get_next_adapter(hdd_ctx, adapter_node, &next);
		adapter_node = next;
	}
}

/**---------------------------------------------------------------------------
  \brief hdd_11d_scan_done - callback to be executed when 11d scan is
                             completed to flush out the scan results

  11d scan is done during driver load and is a passive scan on all
  channels supported by the device, 11d scans may find some APs on
  frequencies which are forbidden to be used in the regulatory domain
  the device is operating in. If these APs are notified to the supplicant
  it may try to connect to these APs, thus flush out all the scan results
  which are present in SME after 11d scan is done.

  \return -  eHalStatus

  --------------------------------------------------------------------------*/
static eHalStatus hdd_11d_scan_done(tHalHandle halHandle, void *pContext,
                                    tANI_U8 sessionId, tANI_U32 scanId,
                                    eCsrScanStatus status)
{
    ENTER();

    sme_ScanFlushResult(halHandle, 0);

    EXIT();

    return eHAL_STATUS_SUCCESS;
}

#ifdef QCA_ARP_SPOOFING_WAR
int wlan_check_xxx(struct net_device *dev, int if_idex, void *data)
{
    hddLog(VOS_TRACE_LEVEL_INFO, "Checking for arp spoof packtes\n");
    return 0;
}

int hdd_filter_cb(tANI_U32 vdev_id, adf_nbuf_t skb, tANI_U32 type)
{
    hdd_adapter_t *pAdapter = NULL;
    hdd_context_t *pHddCtx = NULL;
    v_CONTEXT_t    pVosContext = NULL;
    int ret = 0;

    switch (type) {
        case RX_INTRA_BSS_FWD:
            pVosContext = vos_get_global_context(VOS_MODULE_ID_HDD, NULL);
            if(!pVosContext) {
               hddLog(VOS_TRACE_LEVEL_FATAL,"%s: Global VOS context is Null", __func__);
               goto out;
            }

            pHddCtx = (hdd_context_t *)vos_get_context(VOS_MODULE_ID_HDD, pVosContext);
            if(!pHddCtx) {
               hddLog(VOS_TRACE_LEVEL_FATAL,"%s: HDD context is Null",__func__);
               goto out;
            }

            pAdapter = hdd_get_adapter_by_vdev(pHddCtx, vdev_id);
            if (NULL == pAdapter) {
                hddLog(VOS_TRACE_LEVEL_FATAL,
                          "%s: vdev_id %d does not exist with host",
                          __func__, vdev_id);
                goto out;
            }

            if (*((unsigned short *)(skb->data + HDD_ARP_PACKET_TYPE_OFFSET))
                    == htons(ETH_P_ARP)) {

                ret = wlan_check_xxx(pAdapter->dev, pAdapter->dev->ifindex,
                        skb->data);
            }
            break;
        default:
            hddLog(VOS_TRACE_LEVEL_WARN, "Invalid filter type");
            goto out;
    }
out:
    return ret;
}
#endif
#if defined(CONFIG_HL_SUPPORT) && defined(QCA_BAD_PEER_TX_FLOW_CL)
/**
 * wlan_hdd_bad_peer_txctl() - HDD API to initialize the bad peer
 *                             tx flow control parameters
 * @pHddCtx:	HDD context which contains INI setting.
 *
 * Read configuation from INI setting, and then update the setting
 * of SME module.
 *
 * Return: NULL
 */
static void wlan_hdd_bad_peer_txctl(hdd_context_t *p_hdd_ctx)
{
	struct sme_bad_peer_txctl_param bad_peer_txctl;
	enum sme_max_bad_peer_thresh_levels level = IEEE80211_B_LEVEL;

	bad_peer_txctl.enabled        =
				p_hdd_ctx->cfg_ini->bad_peer_txctl_enable;
	bad_peer_txctl.period         =
				p_hdd_ctx->cfg_ini->bad_peer_txctl_prd;
	bad_peer_txctl.txq_limit      =
				p_hdd_ctx->cfg_ini->bad_peer_txctl_txq_lmt;
	bad_peer_txctl.tgt_backoff    =
				p_hdd_ctx->cfg_ini->bad_peer_tgt_backoff;
	bad_peer_txctl.tgt_report_prd =
				p_hdd_ctx->cfg_ini->bad_peer_tgt_report_prd;

	bad_peer_txctl.thresh[level].cond       =
				p_hdd_ctx->cfg_ini->bad_peer_cond_ieee80211b;
	bad_peer_txctl.thresh[level].delta      =
				p_hdd_ctx->cfg_ini->bad_peer_delta_ieee80211b;
	bad_peer_txctl.thresh[level].percentage =
				p_hdd_ctx->cfg_ini->bad_peer_pct_ieee80211b;
	bad_peer_txctl.thresh[level].thresh     =
				p_hdd_ctx->cfg_ini->bad_peer_tput_ieee80211b;
	bad_peer_txctl.thresh[level].limit      =
				p_hdd_ctx->cfg_ini->bad_peer_limit_ieee80211b;

	level++;
	bad_peer_txctl.thresh[level].cond       =
				p_hdd_ctx->cfg_ini->bad_peer_cond_ieee80211ag;
	bad_peer_txctl.thresh[level].delta      =
				p_hdd_ctx->cfg_ini->bad_peer_delta_ieee80211ag;
	bad_peer_txctl.thresh[level].percentage =
				p_hdd_ctx->cfg_ini->bad_peer_pct_ieee80211ag;
	bad_peer_txctl.thresh[level].thresh     =
				p_hdd_ctx->cfg_ini->bad_peer_tput_ieee80211ag;
	bad_peer_txctl.thresh[level].limit      =
				p_hdd_ctx->cfg_ini->bad_peer_limit_ieee80211ag;

	level++;
	bad_peer_txctl.thresh[level].cond       =
				p_hdd_ctx->cfg_ini->bad_peer_cond_ieee80211n;
	bad_peer_txctl.thresh[level].delta      =
				p_hdd_ctx->cfg_ini->bad_peer_delta_ieee80211n;
	bad_peer_txctl.thresh[level].percentage =
				p_hdd_ctx->cfg_ini->bad_peer_pct_ieee80211n;
	bad_peer_txctl.thresh[level].thresh     =
				p_hdd_ctx->cfg_ini->bad_peer_tput_ieee80211n;
	bad_peer_txctl.thresh[level].limit      =
				p_hdd_ctx->cfg_ini->bad_peer_limit_ieee80211n;

	level++;
	bad_peer_txctl.thresh[level].cond       =
				p_hdd_ctx->cfg_ini->bad_peer_cond_ieee80211ag;
	bad_peer_txctl.thresh[level].delta      =
				p_hdd_ctx->cfg_ini->bad_peer_delta_ieee80211ac;
	bad_peer_txctl.thresh[level].percentage =
				p_hdd_ctx->cfg_ini->bad_peer_pct_ieee80211ac;
	bad_peer_txctl.thresh[level].thresh     =
				p_hdd_ctx->cfg_ini->bad_peer_tput_ieee80211ac;
	bad_peer_txctl.thresh[level].limit      =
				p_hdd_ctx->cfg_ini->bad_peer_limit_ieee80211ac;

	if (eHAL_STATUS_SUCCESS !=
		sme_init_bad_peer_txctl_info(p_hdd_ctx->hHal, bad_peer_txctl)) {
		hddLog(VOS_TRACE_LEVEL_ERROR,
			"%s: Error while initializing bad peer Txctl infor",
			__func__);
	}
}
#else
static inline void wlan_hdd_bad_peer_txctl(hdd_context_t *p_hdd_ctx)
{
	/* no-op */
}
#endif /* defined(CONFIG_HL_SUPPORT) && defined(QCA_BAD_PEER_TX_FLOW_CL) */

#ifdef WLAN_FEATURE_OFFLOAD_PACKETS
/**
 * hdd_init_offloaded_packets_ctx() - Initialize offload packets context
 * @hdd_ctx: hdd global context
 *
 * Return: none
 */
static void hdd_init_offloaded_packets_ctx(hdd_context_t *hdd_ctx)
{
	uint8_t i;

	mutex_init(&hdd_ctx->op_ctx.op_lock);
	for (i = 0; i < MAXNUM_PERIODIC_TX_PTRNS; i++) {
		hdd_ctx->op_ctx.op_table[i].request_id = MAX_REQUEST_ID;
		hdd_ctx->op_ctx.op_table[i].pattern_id = i;
	}
}
#else
static void hdd_init_offloaded_packets_ctx(hdd_context_t *hdd_ctx)
{
}
#endif

#ifdef WLAN_FEATURE_WOW_PULSE
/**
* wlan_hdd_set_wow_pulse() - call SME to send wmi cmd of wow pulse
* @phddctx: hdd_context_t structure pointer
* @enable: enable or disable this behaviour
*
* Return: int
*/
static int wlan_hdd_set_wow_pulse(hdd_context_t *phddctx, bool enable)
{
	hdd_config_t *pcfg_ini = phddctx->cfg_ini;
	struct wow_pulse_mode wow_pulse_set_info;
	VOS_STATUS status;

	hddLog(LOG1, FL("wow pulse enable flag is %d"), enable);

	if (false == phddctx->cfg_ini->wow_pulse_support)
		return 0;

	/* prepare the request to send to SME */
	if (enable == true) {
		wow_pulse_set_info.wow_pulse_enable = true;
		wow_pulse_set_info.wow_pulse_pin =
				pcfg_ini->wow_pulse_pin;
		wow_pulse_set_info.wow_pulse_interval_low =
				pcfg_ini->wow_pulse_interval_low;
		wow_pulse_set_info.wow_pulse_interval_high=
				pcfg_ini->wow_pulse_interval_high;
		wow_pulse_set_info.wow_pulse_repeat_count=
				pcfg_ini->wow_pulse_repeat_count;
		wow_pulse_set_info.wow_pulse_init_state =
				pcfg_ini->wow_pulse_init_state;
	} else {
		wow_pulse_set_info.wow_pulse_enable = false;
		wow_pulse_set_info.wow_pulse_pin = 0;
		wow_pulse_set_info.wow_pulse_interval_low = 0;
		wow_pulse_set_info.wow_pulse_interval_high= 0;
		wow_pulse_set_info.wow_pulse_repeat_count= 0;
		wow_pulse_set_info.wow_pulse_init_state = 0;
	}
	hddLog(LOG1,"%s: enable %d pin %d low %d high %d count %d init %d",
		__func__, wow_pulse_set_info.wow_pulse_enable,
		wow_pulse_set_info.wow_pulse_pin,
		wow_pulse_set_info.wow_pulse_interval_low,
		wow_pulse_set_info.wow_pulse_interval_high,
		wow_pulse_set_info.wow_pulse_repeat_count,
		wow_pulse_set_info.wow_pulse_init_state);

	status = sme_set_wow_pulse(&wow_pulse_set_info);
	if (VOS_STATUS_E_FAILURE == status) {
		hddLog(LOGE,
			"%s: sme_set_wow_pulse failure!", __func__);
		return -EIO;
	}
	hddLog(LOG2,
		"%s: sme_set_wow_pulse success!", __func__);
	return 0;
}
#else
static int inline wlan_hdd_set_wow_pulse(hdd_context_t *phddctx, bool enable)
{
	return 0;
}
#endif

/**
* wlan_hdd_set_wakeup_gpio() - call SME to send wmi cmd of wakeup gpio
* @phddctx: hdd_context_t structure pointer
*
* Return: int
*/
static int wlan_hdd_set_wakeup_gpio(hdd_context_t *hddctx)
{
	hdd_config_t *cfg_ini = hddctx->cfg_ini;
	struct wakeup_gpio_mode wakeup_gpio_info;
	VOS_STATUS status;

	wakeup_gpio_info.host_wakeup_gpio = cfg_ini->host_wakeup_gpio;
	wakeup_gpio_info.host_wakeup_type = cfg_ini->host_wakeup_type;
	wakeup_gpio_info.target_wakeup_gpio = cfg_ini->target_wakeup_gpio;
	wakeup_gpio_info.target_wakeup_type = cfg_ini->target_wakeup_type;

	hddLog(LOG1, "%s:host_gpio %d host_type %d tar_gpio %d tar_type %d",
		__func__, wakeup_gpio_info.host_wakeup_gpio,
		wakeup_gpio_info.host_wakeup_type,
		wakeup_gpio_info.target_wakeup_gpio,
		wakeup_gpio_info.target_wakeup_type);

	status = sme_set_wakeup_gpio(&wakeup_gpio_info);
	if (VOS_STATUS_E_FAILURE == status) {
		hddLog(LOGE,
			"%s: sme_set_wakeup_gpio failure!", __func__);
		return -EIO;
	}
	hddLog(LOG1,
		"%s: sme_set_wakeup_gpio success!", __func__);
	return 0;
}

#ifdef FEATURE_PBM_MAGIC_WOW
void hdd_start_wow_nonos(hdd_adapter_t *pAdapter)
{
	hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);

	if (pHddCtx->isWiphySuspended) {
		hddLog(LOGE, "%s: Already suspended!", __func__);
		return;
	}

	pHddCtx->is_nonos_suspend = true;
	wlan_hdd_cfg80211_suspend_wlan(pHddCtx->wiphy, NULL);
	hif_bus_suspend_nonos();
}

void hdd_stop_wow_nonos(hdd_adapter_t *pAdapter)
{
	hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);

	if (!pHddCtx->isWiphySuspended) {
		hddLog(LOGE, "%s: Already awake!", __func__);
		return;
	}

	if (!pHddCtx->is_nonos_suspend) {
		hddLog(LOGE, "%s: wow stop without start", __func__);
		return;
	}

	hif_bus_resume_nonos();
	wlan_hdd_cfg80211_resume_wlan(pHddCtx->wiphy);
	pHddCtx->is_nonos_suspend = false;
}
#endif

/**
 * hdd_state_info_dump() - prints state information of hdd layer
 * @buf: buffer pointer
 * @size: size of buffer to be filled
 *
 * This function is used to dump state information of hdd layer
 *
 * Return: None
 */
static void hdd_state_info_dump(char **buf_ptr, uint16_t *size)
{
	v_CONTEXT_t vos_ctx_ptr;
	hdd_context_t *hdd_ctx_ptr;
	hdd_adapter_list_node_t *adapter_node = NULL, *next = NULL;
	VOS_STATUS status;
	hdd_station_ctx_t *hdd_sta_ctx;
	hdd_adapter_t *adapter;
	uint16_t len = 0;
	char *buf = *buf_ptr;

	/* get the global voss context */
	vos_ctx_ptr = vos_get_global_context(VOS_MODULE_ID_VOSS, NULL);

	if (NULL == vos_ctx_ptr) {
		VOS_ASSERT(0);
		return;
	}

	hdd_ctx_ptr = vos_get_context(VOS_MODULE_ID_HDD, vos_ctx_ptr);
	if (!hdd_ctx_ptr) {
		hddLog(LOGE, FL("Failed to get hdd context "));
		return;
	}

	hddLog(LOG1, FL("size of buffer: %d"), *size);

	len += scnprintf(buf + len, *size - len,
		"\n isWlanSuspended %d", hdd_ctx_ptr->isWlanSuspended);
	len += scnprintf(buf + len, *size - len,
		"\n isMcThreadSuspended %d",
		hdd_ctx_ptr->isMcThreadSuspended);

	status = hdd_get_front_adapter(hdd_ctx_ptr, &adapter_node);

	while (NULL != adapter_node && VOS_STATUS_SUCCESS == status) {
		adapter = adapter_node->pAdapter;
		if (adapter->dev)
			len += scnprintf(buf + len, *size - len,
				"\n device name: %s", adapter->dev->name);
		len += scnprintf(buf + len, *size - len,
				"\n device_mode: %d", adapter->device_mode);
		switch (adapter->device_mode) {
		case WLAN_HDD_INFRA_STATION:
		case WLAN_HDD_P2P_CLIENT:
			hdd_sta_ctx = WLAN_HDD_GET_STATION_CTX_PTR(adapter);
			len += scnprintf(buf + len, *size - len,
				"\n connState: %d",
				hdd_sta_ctx->conn_info.connState);
			break;

		default:
			break;
		}
		status = hdd_get_next_adapter(hdd_ctx_ptr, adapter_node, &next);
		adapter_node = next;
	}

	*size -= len;
	*buf_ptr += len;
}

/**
 * hdd_register_debug_callback() - registration function for hdd layer
 * to print hdd state information
 *
 * Return: None
 */
static void hdd_register_debug_callback(void)
{
	vos_register_debug_callback(VOS_MODULE_ID_HDD, &hdd_state_info_dump);
}

/**
 * hdd_populate_random_mac_addr() - API to populate random mac addresses
 * @hdd_ctx: HDD Context
 * @num: Number of random mac addresses needed
 *
 * Generate random addresses using bit manipulation on the base mac address
 *
 * Return: None
 */
static void hdd_populate_random_mac_addr(hdd_context_t *hdd_ctx, uint32_t num)
{
	uint32_t start_idx = VOS_MAX_CONCURRENCY_PERSONA - num;
	uint32_t iter;
	hdd_config_t *ini = hdd_ctx->cfg_ini;
	u8 *buf = NULL;
	u8 macaddr_b3, tmp_br3;
	u8 *src = ini->intfMacAddr[0].bytes;

	for (iter = start_idx; iter < VOS_MAX_CONCURRENCY_PERSONA; ++iter) {
		buf = ini->intfMacAddr[iter].bytes;
		vos_mem_copy(buf, src, VOS_MAC_ADDR_SIZE);
		macaddr_b3 = buf[3];
		tmp_br3 = ((macaddr_b3 >> 4 & INTF_MACADDR_MASK) + iter) &
			INTF_MACADDR_MASK;
		macaddr_b3 += tmp_br3;
		macaddr_b3 ^= (1 << INTF_MACADDR_MASK);
		buf[0] |= 0x02;
		buf[3] = macaddr_b3;
		hddLog(LOG1, FL(MAC_ADDRESS_STR), MAC_ADDR_ARRAY(buf));
	}
}

/**
 * hdd_cnss_wlan_mac() - API to get mac addresses from cnss platform driver
 * @hdd_ctx: HDD Context
 *
 * API to get mac addresses from platform driver and update the driver
 * structures and configure FW with the base mac address.
 *
 * Return: int
 */
static int hdd_cnss_wlan_mac(hdd_context_t *hdd_ctx)
{
	uint32_t no_of_mac_addr, iter;
	uint32_t max_mac_addr = VOS_MAX_CONCURRENCY_PERSONA;
	uint32_t mac_addr_size = VOS_MAC_ADDR_SIZE;
	u8 *addr, *buf;
	struct device *dev = hdd_ctx->parent_dev;
	hdd_config_t *ini = hdd_ctx->cfg_ini;
	tSirMacAddr customMacAddr;
	VOS_STATUS status;

	addr = vos_get_cnss_wlan_mac_buff(dev, &no_of_mac_addr);

	if (no_of_mac_addr == 0 || !addr) {
		hddLog(LOG1,
		       FL("Platform Driver Doesn't have wlan mac addresses"));
		return -EINVAL;
	}

	if (no_of_mac_addr > max_mac_addr)
		no_of_mac_addr = max_mac_addr;

	vos_mem_copy(&customMacAddr, addr, mac_addr_size);

	for (iter = 0; iter < no_of_mac_addr; ++iter, addr += mac_addr_size) {
		hddLog(VOS_TRACE_LEVEL_INFO,
		       "cnss update mac addr[%d] from " MAC_ADDRESS_STR
		       " to " MAC_ADDRESS_STR "\n",
		       iter,
		       MAC_ADDR_ARRAY(ini->intfMacAddr[iter].bytes),
		       MAC_ADDR_ARRAY(addr));
		buf = ini->intfMacAddr[iter].bytes;
		vos_mem_copy(buf, addr, VOS_MAC_ADDR_SIZE);
		hddLog(LOG1, FL(MAC_ADDRESS_STR), MAC_ADDR_ARRAY(buf));
	}

	status = sme_SetCustomMacAddr(customMacAddr);

	if (status != VOS_STATUS_SUCCESS)
		return -EAGAIN;

	if (no_of_mac_addr < max_mac_addr)
		hdd_populate_random_mac_addr(hdd_ctx, max_mac_addr -
					     no_of_mac_addr);
	return 0;
}

/**
 * hdd_initialize_mac_address() - API to get wlan mac addresses
 * @hdd_ctx: HDD Context
 *
 * Get MAC addresses from platform driver or wlan_mac.bin. If platform driver is
 * provisioned with mac addresses, driver uses it, else it will use wlan_mac.bin
 * to update HW MAC addresses.
 *
 * Return: 0 if success, errno otherwise
 */
static int hdd_initialize_mac_address(hdd_context_t *hdd_ctx)
{
	VOS_STATUS status;
	int ret;

	ret = hdd_cnss_wlan_mac(hdd_ctx);

	if (ret == 0) {
		hddLog(LOG1,
		       FL("cnss update mac addr"));
		return ret;
	}

	hddLog(LOGW, FL("Can't update MAC via platform driver ret: %d"), ret);

	if (!hdd_ctx->cfg_ini->skip_mac_config) {
		status = hdd_update_mac_config(hdd_ctx);
		if (status == VOS_STATUS_SUCCESS)
			return 0;

		hddLog(LOGW, FL("Can't update MAC from %s status: %d"),
				WLAN_MAC_FILE, status);
	}

	if (!vos_is_macaddr_zero(&hdd_ctx->hw_macaddr)) {
		hdd_update_macaddr(hdd_ctx->cfg_ini, hdd_ctx->hw_macaddr);
		hddLog(LOG1,FL("wlan_mac.bin update mac addr"));
	} else {
		tSirMacAddr customMacAddr;

		hddLog(VOS_TRACE_LEVEL_ERROR,
		       "%s: Invalid MAC passed from target, using MAC from ini"
		       MAC_ADDRESS_STR, __func__,
		       MAC_ADDR_ARRAY(hdd_ctx->cfg_ini->intfMacAddr[0].bytes));
		vos_mem_copy(&customMacAddr,
			     &hdd_ctx->cfg_ini->intfMacAddr[0].bytes,
			     VOS_MAC_ADDR_SIZE);
			     sme_SetCustomMacAddr(customMacAddr);
	}
	return 0;
}

static void hdd_get_DPD_Recaliberation_ini_param(tSmeDPDRecalParams  *pDPDParam,
						                         hdd_context_t *pHddCtx)
{
	pDPDParam->enable =
	  pHddCtx->cfg_ini->dpd_recalib_enabled;
	pDPDParam->delta_degreeHigh =
	  pHddCtx->cfg_ini->dpd_recalib_delta_degreehigh;
	pDPDParam->delta_degreeLow =
	  pHddCtx->cfg_ini->dpd_recalib_delta_degreelow;
	pDPDParam->cooling_time =
	  pHddCtx->cfg_ini->dpd_recalib_cooling_time;
	pDPDParam->dpd_dur_max =
	  pHddCtx->cfg_ini->dpd_recalib_duration_max;
}

#ifdef FEATURE_WLAN_THERMAL_SHUTDOWN
static VOS_STATUS hdd_init_thermal_ctx(hdd_context_t *pHddCtx)
{
	pHddCtx->system_suspended = false;
	pHddCtx->thermal_suspend_state = HDD_WLAN_THERMAL_ACTIVE;
	adf_os_spinlock_init(&pHddCtx->thermal_suspend_lock);
	INIT_DELAYED_WORK(&pHddCtx->thermal_suspend_work, hdd_thermal_suspend_work);
	pHddCtx->thermal_suspend_wq = create_singlethread_workqueue("thermal_wq");
	if (!pHddCtx->thermal_suspend_wq)
		return VOS_STATUS_E_INVAL;
	return VOS_STATUS_SUCCESS;
}

static void hdd_get_thermal_shutdown_ini_param(tSmeThermalParams   *pthermalParam,
						hdd_context_t    *pHddCtx)
{
	pthermalParam->thermal_shutdown_enabled =
	  pHddCtx->cfg_ini->thermal_shutdown_enabled;
	pthermalParam->thermal_shutdown_auto_enabled =
	  pHddCtx->cfg_ini->thermal_shutdown_auto_enabled;
	pthermalParam->thermal_resume_threshold =
	  pHddCtx->cfg_ini->thermal_resume_threshold;
	pthermalParam->thermal_warning_threshold =
	  pHddCtx->cfg_ini->thermal_warning_threshold;
	pthermalParam->thermal_suspend_threshold =
	  pHddCtx->cfg_ini->thermal_suspend_threshold;
	pthermalParam->thermal_sample_rate =
		pHddCtx->cfg_ini->thermal_sample_rate;
}
#else
static inline VOS_STATUS hdd_init_thermal_ctx(hdd_context_t *pHddCtx)
{
	return VOS_STATUS_SUCCESS;
}

static inline void hdd_get_thermal_shutdown_ini_param(tSmeThermalParams   *pthermalParam,
						hdd_context_t    *pHddCtx)
{
	return;
}
#endif

#ifdef WLAN_FEATURE_MOTION_DETECTION
VOS_STATUS hdd_mt_host_ev_cb(void *pcb_cxt, tSirMtEvent *pevent)
{
	hdd_context_t *hddctx;
	hdd_adapter_t *adapter;
	tHalHandle hHal;
	int status;
	tSirMotionDetEnable enable;

	if (pcb_cxt == NULL || pevent == NULL) {
		hddLog(VOS_TRACE_LEVEL_ERROR,
			FL("HDD context is not valid"));
			return VOS_STATUS_E_INVAL;
	}

	hddctx = (hdd_context_t *)pcb_cxt;
	status = wlan_hdd_validate_context(hddctx);
	if (0 != status)
		return VOS_STATUS_E_INVAL;

	adapter = hdd_get_adapter_by_vdev(hddctx, pevent->vdev_id);

	hddLog(VOS_TRACE_LEVEL_INFO,
		FL("hdd_mt_host_ev_cb vdev_id=%u, status=%u"),
		pevent->vdev_id, pevent->status);

	if (adapter->motion_detection_mode == 1) {
	    hHal = WLAN_HDD_GET_HAL_CTX(adapter);

	    enable.vdev_id = pevent->vdev_id;
	    enable.enable = 1;

	    hddLog(VOS_TRACE_LEVEL_INFO,
		FL("hdd_mt_host_ev_cb enable mt again"));

	    sme_MotionDetEnable(hHal, &enable);
	}

	return VOS_STATUS_SUCCESS;
}
#endif
/**---------------------------------------------------------------------------

  \brief hdd_wlan_startup() - HDD init function

  This is the driver startup code executed once a WLAN device has been detected

  \param  - dev - Pointer to the underlying device

  \return -  0 for success, < 0 for failure

  --------------------------------------------------------------------------*/

int hdd_wlan_startup(struct device *dev, v_VOID_t *hif_sc)
{
   VOS_STATUS status;
   hdd_adapter_t *pAdapter = NULL;
#ifdef WLAN_OPEN_P2P_INTERFACE
   hdd_adapter_t *pP2pAdapter = NULL;
#endif
   hdd_context_t *pHddCtx = NULL;
   v_CONTEXT_t pVosContext= NULL;
   eHalStatus hal_status;
   int ret;
   int i;
   struct wiphy *wiphy;
   unsigned long rc;
   tSmeThermalParams thermalParam;
   tSmeDPDRecalParams DPDParam;
   tSirTxPowerLimit *hddtxlimit;
#ifdef FEATURE_WLAN_CH_AVOID
#ifdef CONFIG_CNSS
   uint16_t unsafe_channel_count;
   int unsafeChannelIndex;
#endif
#endif
   tANI_U8 rtnl_lock_enable;
   tANI_U8 reg_netdev_notifier_done = FALSE;
   hdd_adapter_t *dot11_adapter = NULL;
#ifdef QCA_ARP_SPOOFING_WAR
   adf_os_device_t adf_ctx;
#endif
   int set_value;
   struct sme_5g_band_pref_params band_pref_params;

   ENTER();

   if (WLAN_IS_EPPING_ENABLED(con_mode)) {
       /* if epping enabled redirect start to epping module */
      ret = epping_wlan_startup(dev, hif_sc);
      EXIT();
      return ret;
   }
   /*
    * cfg80211: wiphy allocation
    */
   wiphy = wlan_hdd_cfg80211_wiphy_alloc(sizeof(hdd_context_t)) ;

   if(wiphy == NULL)
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: cfg80211 init failed", __func__);
      return -EIO;
   }

   pHddCtx = wiphy_priv(wiphy);

   //Initialize the adapter context to zeros.
   vos_mem_zero(pHddCtx, sizeof( hdd_context_t ));

   pHddCtx->wiphy = wiphy;
   pHddCtx->isLoadInProgress = TRUE;

   status = hdd_init_thermal_ctx(pHddCtx);
   if (VOS_STATUS_SUCCESS != status)
        goto err_free_hdd_context;

   pHddCtx->ioctl_scan_mode = eSIR_ACTIVE_SCAN;
   vos_set_wakelock_logging(false);

   vos_set_load_unload_in_progress(VOS_MODULE_ID_VOSS, TRUE);
   vos_set_load_in_progress(VOS_MODULE_ID_VOSS, TRUE);

   /*Get vos context here bcoz vos_open requires it*/
   pVosContext = vos_get_global_context(VOS_MODULE_ID_SYS, NULL);

   if(pVosContext == NULL)
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,"%s: Failed vos_get_global_context",__func__);
      goto err_free_hdd_context;
   }
#ifdef QCA_ARP_SPOOFING_WAR
   adf_ctx = vos_get_context(VOS_MODULE_ID_ADF, pVosContext);
   if (adf_ctx == NULL) {
       hddLog(VOS_TRACE_LEVEL_FATAL,"%s: Failed vos_get_global_context",__func__);
       goto err_free_hdd_context;
   }
   adf_ctx->filter_cb = (void *)hdd_filter_cb;
#endif

   //Save the Global VOSS context in adapter context for future.
   pHddCtx->pvosContext = pVosContext;

   //Save the adapter context in global context for future.
   ((VosContextType*)(pVosContext))->pHDDContext = (v_VOID_t*)pHddCtx;

   pHddCtx->parent_dev = dev;
   pHddCtx->last_scan_reject_session_id = 0xFF;
   pHddCtx->last_scan_reject_reason = 0;
   pHddCtx->last_scan_reject_timestamp = 0;
   pHddCtx->scan_reject_cnt = 0;

   init_completion(&pHddCtx->full_pwr_comp_var);
   init_completion(&pHddCtx->standby_comp_var);
   init_completion(&pHddCtx->req_bmps_comp_var);
   init_completion(&pHddCtx->mc_sus_event_var);
   init_completion(&pHddCtx->tx_sus_event_var);
   init_completion(&pHddCtx->rx_sus_event_var);
   init_completion(&pHddCtx->ready_to_suspend);
#ifdef WLAN_FEATURE_EXTWOW_SUPPORT
      init_completion(&pHddCtx->ready_to_extwow);
#endif
   hdd_init_bpf_completion();
#ifdef FEATURE_WLAN_EXTSCAN
   init_completion(&pHddCtx->ext_scan_context.response_event);
#endif /* FEATURE_WLAN_EXTSCAN */

   hdd_init_ll_stats_ctx(pHddCtx);

   init_completion(&pHddCtx->chain_rssi_context.response_event);

   adf_os_spinlock_init(&pHddCtx->schedScan_lock);

   hdd_list_init( &pHddCtx->hddAdapters, MAX_NUMBER_OF_ADAPTERS );

#ifdef FEATURE_WLAN_TDLS
   /* tdls_lock is initialized before an hdd_open_adapter ( which is
    * invoked by other instances also) to protect the concurrent
    * access for the Adapters by TDLS module.
    */
   mutex_init(&pHddCtx->tdls_lock);
#endif

   status = wlan_hdd_init_tx_rx_histogram(pHddCtx);
   if (status != 0) {
       goto err_free_hdd_context;
   }

#ifdef FEATURE_WLAN_DISABLE_CHANNEL_SWITCH
   adf_os_spinlock_init(&pHddCtx->restrict_offchan_lock);
   mutex_init(&pHddCtx->avoid_freq_lock);
#endif

   adf_os_spinlock_init(&pHddCtx->dfs_lock);
   adf_os_spinlock_init(&pHddCtx->sap_update_info_lock);
   adf_os_spinlock_init(&pHddCtx->sta_update_info_lock);
   hdd_init_offloaded_packets_ctx(pHddCtx);
   // Load all config first as TL config is needed during vos_open
   pHddCtx->cfg_ini = (hdd_config_t*) vos_mem_malloc(sizeof(hdd_config_t));
   if(pHddCtx->cfg_ini == NULL)
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,"%s: Failed kmalloc hdd_config_t",__func__);
      goto err_histogram;
   }

   vos_mem_zero(pHddCtx->cfg_ini, sizeof( hdd_config_t ));

   // Read and parse the qcom_cfg.ini file
   status = hdd_parse_config_ini( pHddCtx );
   if ( VOS_STATUS_SUCCESS != status )
   {
      hddLog(VOS_TRACE_LEVEL_FATAL, "%s: error parsing %s",
             __func__, WLAN_INI_FILE);
      goto err_config;
   }

   status = hdd_easy_wow_init(pHddCtx);
   if (VOS_STATUS_SUCCESS != status)
      goto err_config;

   /* If IPA HW is not existing, disable offload from INI */
   if (!hdd_ipa_is_present(pHddCtx))
      hdd_ipa_reset_ipaconfig(pHddCtx, 0);

   if (pHddCtx->cfg_ini->probe_req_ie_whitelist)
   {
      if (hdd_validate_prb_req_ie_bitmap(pHddCtx))
      {
         /* parse ini string probe req oui */
         status = hdd_parse_probe_req_ouis(pHddCtx);
         if (VOS_STATUS_SUCCESS != status)
         {
            hddLog(LOGE, FL("Error parsing probe req ouis - Ignoring them"
                            " disabling white list"));
            pHddCtx->cfg_ini->probe_req_ie_whitelist = false;
         }
      }
      else
      {
         hddLog(LOGE, FL("invalid probe req ie bitmap and ouis,"
                         " disabling white list"));
         pHddCtx->cfg_ini->probe_req_ie_whitelist = false;
      }
   }

   if (0 == pHddCtx->cfg_ini->max_go_peers)
      pHddCtx->cfg_ini->max_go_peers = pHddCtx->cfg_ini->max_sap_peers;

   pHddCtx->max_peers = MAX(pHddCtx->cfg_ini->max_sap_peers,
                            pHddCtx->cfg_ini->max_go_peers);

   ((VosContextType*)pVosContext)->pHIFContext = hif_sc;

   /* store target type and target version info in hdd ctx */
   pHddCtx->target_type = ((struct ol_softc *)hif_sc)->target_type;

   pHddCtx->current_intf_count=0;
   pHddCtx->max_intf_count = CSR_ROAM_SESSION_MAX;

   /* INI has been read, initialise the configuredMcastBcastFilter with
    * INI value as this will serve as the default value
    */
   pHddCtx->configuredMcastBcastFilter = pHddCtx->cfg_ini->mcastBcastFilterSetting;
   hddLog(VOS_TRACE_LEVEL_INFO, "Setting configuredMcastBcastFilter: %d",
                   pHddCtx->cfg_ini->mcastBcastFilterSetting);

   vos_set_fatal_event(pHddCtx->cfg_ini->enable_fatal_event);
   if (false == hdd_is_5g_supported(pHddCtx))
   {
      //5Ghz is not supported.
      if (1 != pHddCtx->cfg_ini->nBandCapability)
      {
         hddLog(VOS_TRACE_LEVEL_INFO,
                "%s: Setting pHddCtx->cfg_ini->nBandCapability = 1", __func__);
         pHddCtx->cfg_ini->nBandCapability = 1;
      }
   }

   if (pHddCtx->cfg_ini->fhostNSOffload)
       pHddCtx->ns_offload_enable = true;

   /*
    * If SNR Monitoring is enabled, FW has to parse all beacons
    * for calculating and storing the average SNR, so set Nth beacon
    * filter to 1 to enable FW to parse all the beacons
    */
   if (1 == pHddCtx->cfg_ini->fEnableSNRMonitoring)
   {
      /* The log level is deliberately set to WARN as overriding
       * nthBeaconFilter to 1 will increase power consumption and this
       * might just prove helpful to detect the power issue.
       */
      hddLog(VOS_TRACE_LEVEL_WARN,
             "%s: Setting pHddCtx->cfg_ini->nthBeaconFilter = 1", __func__);
      pHddCtx->cfg_ini->nthBeaconFilter = 1;
   }
   /*
    * cfg80211: Initialization  ...
    */
   if (0 < wlan_hdd_cfg80211_init(dev, wiphy, pHddCtx->cfg_ini)) {
          hddLog(LOGE,
                 "%s: wlan_hdd_cfg80211_init return failure", __func__);
          goto err_easy_wow;
   }

   /* Initialize struct for saving f/w log setting will be used
   after ssr */
   pHddCtx->fw_log_settings.enable = pHddCtx->cfg_ini->enablefwlog;
   pHddCtx->fw_log_settings.dl_type = 0;
   pHddCtx->fw_log_settings.dl_report = 0;
   pHddCtx->fw_log_settings.dl_loglevel = 0;
   pHddCtx->fw_log_settings.index = 0;
   for (i = 0; i < MAX_MOD_LOGLEVEL; i++) {
       pHddCtx->fw_log_settings.dl_mod_loglevel[i] = 0;
   }

   if (VOS_FTM_MODE != hdd_get_conparam()) {
       vos_set_multicast_logging(pHddCtx->cfg_ini->multicast_host_fw_msgs);

       if (wlan_hdd_logging_sock_activate_svc(pHddCtx) < 0)
           goto err_sock_activate;

       /*
        * Update VOS trace levels based upon the code. The multicast log
        * log levels of the code need not be set when the logger thread
        * is not enabled.
        */
       if (vos_is_multicast_logging())
           wlan_logging_set_log_level();
   }

   /*
    * Update VOS trace levels based upon the cfg.ini
    */
   hdd_vos_trace_enable(VOS_MODULE_ID_TL,
                        pHddCtx->cfg_ini->vosTraceEnableTL);
   hdd_vos_trace_enable(VOS_MODULE_ID_WDI,
                        pHddCtx->cfg_ini->vosTraceEnableWDI);
   hdd_vos_trace_enable(VOS_MODULE_ID_HDD,
                        pHddCtx->cfg_ini->vosTraceEnableHDD);
   hdd_vos_trace_enable(VOS_MODULE_ID_SME,
                        pHddCtx->cfg_ini->vosTraceEnableSME);
   hdd_vos_trace_enable(VOS_MODULE_ID_PE,
                        pHddCtx->cfg_ini->vosTraceEnablePE);
   hdd_vos_trace_enable(VOS_MODULE_ID_PMC,
                         pHddCtx->cfg_ini->vosTraceEnablePMC);
   hdd_vos_trace_enable(VOS_MODULE_ID_WDA,
                        pHddCtx->cfg_ini->vosTraceEnableWDA);
   hdd_vos_trace_enable(VOS_MODULE_ID_SYS,
                        pHddCtx->cfg_ini->vosTraceEnableSYS);
   hdd_vos_trace_enable(VOS_MODULE_ID_VOSS,
                        pHddCtx->cfg_ini->vosTraceEnableVOSS);
   hdd_vos_trace_enable(VOS_MODULE_ID_SAP,
                        pHddCtx->cfg_ini->vosTraceEnableSAP);
   hdd_vos_trace_enable(VOS_MODULE_ID_HDD_SOFTAP,
                        pHddCtx->cfg_ini->vosTraceEnableHDDSAP);
   hdd_vos_trace_enable(VOS_MODULE_ID_HDD_DATA,
                        pHddCtx->cfg_ini->vosTraceEnableHDDDATA);
   hdd_vos_trace_enable(VOS_MODULE_ID_HDD_SAP_DATA,
                        pHddCtx->cfg_ini->vosTraceEnableHDDSAPDATA);
   hdd_vos_trace_enable(VOS_MODULE_ID_HIF,
                        pHddCtx->cfg_ini->vosTraceEnableHIF);
   hdd_vos_trace_enable(VOS_MODULE_ID_TXRX,
                        pHddCtx->cfg_ini->vosTraceEnableTXRX);
   hdd_vos_trace_enable(VOS_MODULE_ID_HTC,
                        pHddCtx->cfg_ini->vosTraceEnableHTC);
   hdd_vos_trace_enable(VOS_MODULE_ID_ADF,
                        pHddCtx->cfg_ini->vosTraceEnableADF);
   hdd_vos_trace_enable(VOS_MODULE_ID_CFG,
                        pHddCtx->cfg_ini->vosTraceEnableCFG);

   print_hdd_cfg(pHddCtx);

   /* Initialize the nlink service */
   if (wlan_hdd_nl_init(pHddCtx) != 0) {
      hddLog(LOGP, FL("nl_srv_init failed"));
      goto err_logging_sock;
   }
   vos_set_radio_index(pHddCtx->radio_index);

   if (VOS_FTM_MODE == hdd_get_conparam()) {
      if ( VOS_STATUS_SUCCESS != wlan_hdd_ftm_open(pHddCtx) )
      {
          hddLog(VOS_TRACE_LEVEL_FATAL,
                 "%s: wlan_hdd_ftm_open Failed",__func__);
          goto err_nl_srv;
      }
      if (VOS_STATUS_SUCCESS != hdd_ftm_start(pHddCtx))
      {
          hddLog(VOS_TRACE_LEVEL_FATAL,"%s: hdd_ftm_start Failed",__func__);
          goto err_free_ftm_open;
      }
   } else {

      //Open watchdog module
      if(pHddCtx->cfg_ini->fIsLogpEnabled)
      {
         status = vos_watchdog_open(pVosContext,
            &((VosContextType*)pVosContext)->vosWatchdog,
            sizeof(VosWatchdogContext));

         if(!VOS_IS_STATUS_SUCCESS( status ))
         {
            hddLog(VOS_TRACE_LEVEL_FATAL,
                   "%s: vos_watchdog_open failed",__func__);
            goto err_nl_srv;
         }
      }

      pHddCtx->isLogpInProgress = FALSE;
      vos_set_logp_in_progress(VOS_MODULE_ID_VOSS, FALSE);

      status = vos_nv_open();
      if (!VOS_IS_STATUS_SUCCESS(status))
      {
         /* NV module cannot be initialized */
         hddLog( VOS_TRACE_LEVEL_FATAL,
               "%s: vos_nv_open failed", __func__);
         goto err_wdclose;
      }

      hdd_request_manager_init();
      hdd_wlan_green_ap_init(pHddCtx);

      status = vos_open( &pVosContext, 0);
      if ( !VOS_IS_STATUS_SUCCESS( status ))
      {
         hddLog(VOS_TRACE_LEVEL_FATAL, "%s: vos_open failed", __func__);
         goto err_vos_nv_close;
      }

      hif_init_pdev_txrx_handle(hif_sc,
                         vos_get_context(VOS_MODULE_ID_TXRX, pVosContext));

      pHddCtx->hHal = (tHalHandle)vos_get_context(VOS_MODULE_ID_SME,
                                                  pVosContext );

      if ( NULL == pHddCtx->hHal )
      {
         hddLog(VOS_TRACE_LEVEL_FATAL, "%s: HAL context is null", __func__);
         goto err_vosclose;
      }

      status = vos_preStart( pHddCtx->pvosContext );
      if ( !VOS_IS_STATUS_SUCCESS( status ) )
      {
         hddLog(VOS_TRACE_LEVEL_FATAL, "%s: vos_preStart failed", __func__);
         goto err_vosclose;
      }

      wlan_hdd_update_wiphy(wiphy, pHddCtx);

      if (sme_IsFeatureSupportedByFW(DOT11AC)) {
         hddLog(VOS_TRACE_LEVEL_INFO_HIGH, "%s: support 11ac", __func__);
      } else {
         hddLog(VOS_TRACE_LEVEL_INFO_HIGH, "%s: not support 11ac", __func__);
         if ((pHddCtx->cfg_ini->dot11Mode == eHDD_DOT11_MODE_11ac_ONLY)||
             (pHddCtx->cfg_ini->dot11Mode == eHDD_DOT11_MODE_11ac)) {

            pHddCtx->cfg_ini->dot11Mode = eHDD_DOT11_MODE_11n;
            pHddCtx->cfg_ini->sap_p2p_11ac_override = 0;
         }
      }

      if (0 != wlan_hdd_set_wow_pulse(pHddCtx, true)) {
         hddLog(VOS_TRACE_LEVEL_ERROR,
                "%s: Failed to set wow pulse", __func__);
      }

      if ((pHddCtx->cfg_ini->host_wakeup_gpio !=
				CFG_HOST_WAKEUP_GPIO_DEFAULT) ||
          (pHddCtx->cfg_ini->target_wakeup_gpio !=
				CFG_HOST_WAKEUP_GPIO_DEFAULT)) {
          if (0 != wlan_hdd_set_wakeup_gpio(pHddCtx)) {
             hddLog(VOS_TRACE_LEVEL_ERROR,
                "%s: Failed to set wakeup gpio", __func__);
          }
      }

      /* Set 802.11p config
       * TODO-OCB: This has been temporarily added here to ensure this paramter
       * is set in CSR when we init the channel list. This should be removed
       * once the 5.9 GHz channels are added to the regulatory domain.
       */
      hdd_set_dot11p_config(pHddCtx);

      if (0 == enable_dfs_chan_scan || 1 == enable_dfs_chan_scan)
      {
         pHddCtx->cfg_ini->enableDFSChnlScan = enable_dfs_chan_scan;
         hddLog(VOS_TRACE_LEVEL_INFO,
                "%s: module enable_dfs_chan_scan set to %d",
                __func__, enable_dfs_chan_scan);
      }
      if (0 == enable_11d || 1 == enable_11d)
      {
         pHddCtx->cfg_ini->Is11dSupportEnabled = enable_11d;
         hddLog(VOS_TRACE_LEVEL_INFO, "%s: module enable_11d set to %d",
                __func__, enable_11d);
      }

      /* Note that the vos_preStart() sequence triggers the cfg download.
         The cfg download must occur before we update the SME config
         since the SME config operation must access the cfg database */
      status = hdd_set_sme_config( pHddCtx );

      if ( VOS_STATUS_SUCCESS != status )
      {
         hddLog(VOS_TRACE_LEVEL_FATAL, "%s: Failed hdd_set_sme_config",
                __func__);
         goto err_vosclose;
      }

#ifdef CLD_REGDB
     if (wiphy && country_code)
         regulatory_hint(wiphy, country_code);
#endif

      status = wlan_hdd_reg_init(pHddCtx);
      if (status != VOS_STATUS_SUCCESS) {
         hddLog(VOS_TRACE_LEVEL_FATAL,
                "%s: Failed to init channel list", __func__);
         goto err_vosclose;
      }
   }

   /* registration of wiphy dev with cfg80211 */
   if (0 > wlan_hdd_cfg80211_register(pHddCtx->wiphy)) {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: wiphy register failed", __func__);
      status = VOS_STATUS_E_FAILURE;
      if (VOS_FTM_MODE == hdd_get_conparam())
         goto err_free_ftm_open;
      else
         goto err_vosclose;
   }

   if (VOS_FTM_MODE == hdd_get_conparam()) {
      vos_set_load_unload_in_progress(VOS_MODULE_ID_VOSS, FALSE);
      vos_set_load_in_progress(VOS_MODULE_ID_VOSS, FALSE);
      pHddCtx->isLoadInProgress = FALSE;

      hdd_driver_memdump_init();
      hddLog(LOGE, FL("FTM driver loaded"));
      wlan_comp.status = 0;
      complete(&wlan_comp.wlan_start_comp);
      return VOS_STATUS_SUCCESS;
   }

   ret = process_wma_set_command(0, WMI_PDEV_PARAM_TX_CHAIN_MASK_1SS,
                                 pHddCtx->cfg_ini->tx_chain_mask_1ss,
                                 PDEV_CMD);
   if (0 != ret) {
       hddLog(VOS_TRACE_LEVEL_ERROR,
              "%s: WMI_PDEV_PARAM_TX_CHAIN_MASK_1SS failed %d",
              __func__, ret);
   }

   ret = process_wma_set_command(0, WMI_PDEV_PARAM_TX_SCH_DELAY,
                                 pHddCtx->cfg_ini->tx_sch_delay,
                                 PDEV_CMD);
   if (0 != ret) {
       hddLog(VOS_TRACE_LEVEL_ERROR,
              "%s: WMI_PDEV_PARAM_TX_SCH_DELAY failed %d",
              __func__, ret);
   }

   if (pHddCtx->cfg_ini->sap_get_peer_info) {
       hddLog(VOS_TRACE_LEVEL_INFO,
              "%s Send WMI_PDEV_PARAM_PEER_STATS_INFO_ENABLE",
              __func__);
       ret = process_wma_set_command(0,
                                     WMI_PDEV_PARAM_PEER_STATS_INFO_ENABLE,
                                     pHddCtx->cfg_ini->sap_get_peer_info,
                                     PDEV_CMD);
       if (0 != ret) {
           hddLog(VOS_TRACE_LEVEL_ERROR,
                  "%s: WMI_PDEV_PARAM_PEER_STATS_INFO_ENABLE failed %d",
                  __func__, ret);
       }
   }
   ret = process_wma_set_command(0, WMI_PDEV_PARAM_ARP_AC_OVERRIDE,
                                 pHddCtx->cfg_ini->arp_ac_category, PDEV_CMD);
   if (0 != ret) {
       hddLog(LOGE,
              "%s: WMI_PDEV_PARAM_ARP_AC_OVERRIDE failed AC: %d ret: %d",
              __func__, pHddCtx->cfg_ini->arp_ac_category, ret);
   }

#ifdef CLD_REGDB
   if (country_code) {
      pHddCtx->reg.alpha2[0] = country_code[0];
      pHddCtx->reg.alpha2[1] = country_code[1];
      pHddCtx->reg.cc_src = NL80211_REGDOM_SET_BY_DRIVER;
      pHddCtx->reg.dfs_region = 0;
   }
#endif

   status = hdd_set_sme_chan_list(pHddCtx);
   if (status != VOS_STATUS_SUCCESS) {
      hddLog(VOS_TRACE_LEVEL_FATAL,
             "%s: Failed to init channel list", __func__);
      goto err_wiphy_unregister;
   }

   /* In the integrated architecture we update the configuration from
      the INI file and from NV before vOSS has been started so that
      the final contents are available to send down to the cCPU   */

   // Apply the cfg.ini to cfg.dat
   if (FALSE == hdd_update_config_dat(pHddCtx))
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,"%s: config update failed",__func__ );
      goto err_wiphy_unregister;
   }

   ret = hdd_initialize_mac_address(pHddCtx);
   if (!pHddCtx->cfg_ini->g_use_otpmac && ret) {
       hddLog(LOGE,
        FL("Failed to read MAC from platform driver or %s (driver load failed)"),
        WLAN_MAC_FILE);
       goto err_wiphy_unregister;
   }

   {
      eHalStatus halStatus;
      /* Set the MAC Address Currently this is used by HAL to
       * add self sta. Remove this once self sta is added as
       * part of session open.
       */
      halStatus = cfgSetStr( pHddCtx->hHal, WNI_CFG_STA_ID,
                             (v_U8_t *)&pHddCtx->cfg_ini->intfMacAddr[0],
                             sizeof( pHddCtx->cfg_ini->intfMacAddr[0]) );

      if (!HAL_STATUS_SUCCESS( halStatus ))
      {
         hddLog(VOS_TRACE_LEVEL_ERROR,"%s: Failed to set MAC Address. "
                "HALStatus is %08d [x%08x]",__func__, halStatus, halStatus );
         goto err_wiphy_unregister;
      }
   }

#ifdef IPA_OFFLOAD
#ifdef SYNC_IPA_READY
   /* Check if IPA is ready before calling any IPA API */
   if ((ret = ipa_register_ipa_ready_cb((void *)hdd_ipa_ready_cb,
                                      (void *)pHddCtx)) == -EEXIST) {
      hddLog(VOS_TRACE_LEVEL_FATAL, FL("IPA is ready"));
   } else if (ret >= 0) {
      hddLog(VOS_TRACE_LEVEL_FATAL,
             FL("IPA is not ready - wait until it is ready"));
      init_completion(&pHddCtx->ipa_ready);
      wait_for_completion(&pHddCtx->ipa_ready);
   } else {
      hddLog(VOS_TRACE_LEVEL_FATAL,
             FL("IPA is not ready - Fail to register IPA ready callback"));
      goto err_wiphy_unregister;
   }
#endif
   if (hdd_ipa_init(pHddCtx) == VOS_STATUS_E_FAILURE)
      goto err_wiphy_unregister;
#endif

   if (hdd_spectral_init(pHddCtx) == VOS_STATUS_E_FAILURE)
#ifdef IPA_OFFLOAD
      goto err_ipa_cleanup;
#else
      goto err_wiphy_unregister;
#endif

   /*Start VOSS which starts up the SME/MAC/HAL modules and everything else */
   status = vos_start( pHddCtx->pvosContext );
   if ( !VOS_IS_STATUS_SUCCESS( status ) )
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,"%s: vos_start failed",__func__);
      goto err_spectral_deinit;
   }

#ifdef FEATURE_WLAN_CH_AVOID
#ifdef CONFIG_CNSS
   vos_get_wlan_unsafe_channel(pHddCtx->unsafe_channel_list,
                                &(pHddCtx->unsafe_channel_count),
                                sizeof(v_U16_t) * NUM_20MHZ_RF_CHANNELS);
   hddLog(LOG1,"%s: num of unsafe channels is %d. ",
                __func__, pHddCtx->unsafe_channel_count);

   unsafe_channel_count = VOS_MIN((uint16_t)pHddCtx->unsafe_channel_count,
                              (uint16_t)NUM_20MHZ_RF_CHANNELS);

   for (unsafeChannelIndex = 0;
       unsafeChannelIndex < unsafe_channel_count;
       unsafeChannelIndex++) {
       hddLog(LOG1,"%s: channel %d is not safe. ",
         __func__, pHddCtx->unsafe_channel_list[unsafeChannelIndex]);
   }

   /* Plug in avoid channel notification callback */
   sme_AddChAvoidCallback(pHddCtx->hHal,
                          hdd_ch_avoid_cb);
#endif
#endif /* FEATURE_WLAN_CH_AVOID */

#ifdef WLAN_FEATURE_SAP_TO_FOLLOW_STA_CHAN
    /* Initialize the lock*/
   mutex_init(&pHddCtx->ch_switch_ctx.sap_ch_sw_lock);
    /*Register the CSA Notification callback*/
   sme_AddCSAIndCallback(pHddCtx->hHal, hdd_csa_notify_cb);
#endif//#ifdef WLAN_FEATURE_SAP_TO_FOLLOW_STA_CHAN

   status = hdd_post_voss_start_config( pHddCtx );
   if ( !VOS_IS_STATUS_SUCCESS( status ) )
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,"%s: hdd_post_voss_start_config failed",
         __func__);
      goto err_vosstop;
   }

#ifdef QCA_PKT_PROTO_TRACE
   /* Ensure pkt tracing happen only in Non FTM mode */
   vos_pkt_proto_trace_init();
#endif /* QCA_PKT_PROTO_TRACE */

#if defined(CONFIG_HDD_INIT_WITH_RTNL_LOCK)
   rtnl_lock();
   rtnl_lock_enable = TRUE;
#else
   rtnl_lock_enable = FALSE;
#endif

   /* Initialize the RoC Request queue and work. */
   hdd_list_init((&pHddCtx->hdd_roc_req_q), MAX_ROC_REQ_QUEUE_ENTRY);
   vos_init_delayed_work(&pHddCtx->rocReqWork, wlan_hdd_roc_request_dequeue);
   vos_init_work(&pHddCtx->sap_start_work, hdd_sap_restart_handle);
#ifdef WLAN_FEATURE_USB_RECOVERY
   vos_init_delayed_work(&pHddCtx->usb_detect_work, hdd_usb_detect_work);
   mutex_init(&pHddCtx->usb_recovery_lock);
#endif

   if (pHddCtx->cfg_ini->dot11p_mode == WLAN_HDD_11P_STANDALONE) {
       /* Create only 802.11p interface */
      pAdapter = hdd_open_adapter(pHddCtx, WLAN_HDD_OCB,"wlanocb%d",
                                  wlan_hdd_get_intf_addr(pHddCtx),
                                  NET_NAME_UNKNOWN,
                                  rtnl_lock_enable);
   } else {
#ifndef SUPPORT_P2P_BY_ONE_INTF_WLAN
      pAdapter = hdd_open_adapter(pHddCtx, WLAN_HDD_INFRA_STATION, "wlan%d",
                                  wlan_hdd_get_intf_addr(pHddCtx),
                                  NET_NAME_UNKNOWN,
                                  rtnl_lock_enable);
#endif

      if (pAdapter != NULL &&
          strlen(pHddCtx->cfg_ini->enable_concurrent_sta)) {
         pAdapter = hdd_open_adapter(pHddCtx, WLAN_HDD_INFRA_STATION,
                                     pHddCtx->cfg_ini->enable_concurrent_sta,
                                     wlan_hdd_get_intf_addr(pHddCtx),
                                     NET_NAME_UNKNOWN,
                                     rtnl_lock_enable);
      }

#ifdef WLAN_OPEN_P2P_INTERFACE
    if(VOS_MONITOR_MODE != vos_get_conparam()){
      /* Open P2P device interface */
#ifndef SUPPORT_P2P_BY_ONE_INTF_WLAN
      if (pAdapter != NULL &&
          !hdd_cfg_is_sub20_channel_width_enabled(pHddCtx)) {
#else
      {
#endif
         if (pHddCtx->cfg_ini->isP2pDeviceAddrAdministrated &&
             !(pHddCtx->cfg_ini->intfMacAddr[0].bytes[0] & 0x02)) {
            vos_mem_copy(pHddCtx->p2pDeviceAddress.bytes,
                         pHddCtx->cfg_ini->intfMacAddr[0].bytes,
                         sizeof(tSirMacAddr));

            /* Generate the P2P Device Address.  This consists of the device's
             * primary MAC address with the locally administered bit set.
             */
            pHddCtx->p2pDeviceAddress.bytes[0] |= 0x02;
         } else {
            uint8_t* p2p_dev_addr = wlan_hdd_get_intf_addr(pHddCtx);
            if (p2p_dev_addr != NULL) {
               vos_mem_copy(&pHddCtx->p2pDeviceAddress.bytes[0],
                            p2p_dev_addr, VOS_MAC_ADDR_SIZE);
            } else {
               hddLog(VOS_TRACE_LEVEL_FATAL,
                   FL("Failed to allocate mac_address for p2p_device"));
               goto err_close_adapter;
            }
         }

#ifndef SUPPORT_P2P_BY_ONE_INTF_WLAN
         pP2pAdapter = hdd_open_adapter(pHddCtx, WLAN_HDD_P2P_DEVICE, "p2p%d",
#else
         pP2pAdapter = hdd_open_adapter(pHddCtx, WLAN_HDD_INFRA_STATION, "wlan%d",
#endif
                                        &pHddCtx->p2pDeviceAddress.bytes[0],
                                        NET_NAME_UNKNOWN,
                                        rtnl_lock_enable);

         if (NULL == pP2pAdapter) {
            hddLog(VOS_TRACE_LEVEL_FATAL,
                FL("Failed to do hdd_open_adapter for P2P Device Interface"));
            goto err_close_adapter;
         }
      }
    }
#endif /* WLAN_OPEN_P2P_INTERFACE */

      /* Open 802.11p Interface */
      if (pAdapter != NULL) {
         if (pHddCtx->cfg_ini->dot11p_mode == WLAN_HDD_11P_CONCURRENT) {
            dot11_adapter = hdd_open_adapter(pHddCtx, WLAN_HDD_OCB, "wlanocb%d",
                                             wlan_hdd_get_intf_addr(pHddCtx),
                                             NET_NAME_UNKNOWN,
                                             rtnl_lock_enable);

            if (dot11_adapter == NULL) {
               hddLog(VOS_TRACE_LEVEL_FATAL,
                   FL("hdd_open_adapter() failed for 802.11p Interface"));
               goto err_close_adapter;
            }
         }
      }
   }

#ifdef SUPPORT_P2P_BY_ONE_INTF_WLAN
   pAdapter = pP2pAdapter;
#endif

   if( pAdapter == NULL )
   {
      hddLog(VOS_TRACE_LEVEL_ERROR, "%s: hdd_open_adapter failed", __func__);
      goto err_close_adapter;
   }


   /* Target hw version/revision would only be retrieved after
      firmware download */
   hif_get_hw_info(hif_sc, &pHddCtx->target_hw_version,
                   &pHddCtx->target_hw_revision);

   /* Get the wlan hw/fw version */
   hdd_wlan_get_version(pAdapter, NULL, NULL);

   /* pass target_fw_version to HIF layer */
   hif_set_fw_info(hif_sc, pHddCtx->target_fw_version);

   if (country_code)
   {
      eHalStatus ret;

      INIT_COMPLETION(pAdapter->change_country_code);
      hdd_checkandupdate_dfssetting(pAdapter, country_code);

      ret = sme_ChangeCountryCode(pHddCtx->hHal,
            wlan_hdd_change_country_code_callback,
            country_code, pAdapter, pHddCtx->pvosContext, eSIR_TRUE, eSIR_TRUE);
      if (eHAL_STATUS_SUCCESS == ret)
      {
          rc = wait_for_completion_timeout(
                &pAdapter->change_country_code,
                msecs_to_jiffies(WLAN_WAIT_TIME_COUNTRY));
          if (!rc) {
              hddLog(VOS_TRACE_LEVEL_ERROR,
                   "%s: SME while setting country code timed out", __func__);
          }
      }
      else
      {
          hddLog(VOS_TRACE_LEVEL_ERROR,
                 "%s: SME Change Country code from module param fail ret=%d", __func__, ret);
          ret = -EINVAL;
      }
   }


#ifdef WLAN_FEATURE_ROAM_SCAN_OFFLOAD
   if(!(IS_ROAM_SCAN_OFFLOAD_FEATURE_ENABLE))
   {
      hddLog(VOS_TRACE_LEVEL_DEBUG,"%s: ROAM_SCAN_OFFLOAD Feature not supported",__func__);
      pHddCtx->cfg_ini->isRoamOffloadScanEnabled = 0;
      sme_UpdateRoamScanOffloadEnabled((tHalHandle)(pHddCtx->hHal),
                       pHddCtx->cfg_ini->isRoamOffloadScanEnabled);
   }
#endif
#ifdef FEATURE_WLAN_SCAN_PNO
   /*SME must send channel update configuration to FW*/
   sme_UpdateChannelConfig(pHddCtx->hHal);
#endif
   sme_Register11dScanDoneCallback(pHddCtx->hHal, hdd_11d_scan_done);

   /* Fwr capabilities received, Set the Dot11 mode */
   sme_SetDefDot11Mode(pHddCtx->hHal);

   /* Register with platform driver as client for Suspend/Resume */
   status = hddRegisterPmOps(pHddCtx);
   if ( !VOS_IS_STATUS_SUCCESS( status ) )
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,"%s: hddRegisterPmOps failed",__func__);
      goto err_close_adapter;
   }

   /* Open debugfs interface */
   if (VOS_STATUS_SUCCESS != hdd_debugfs_init(pAdapter))
   {
      VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_ERROR,
                 "%s: hdd_debugfs_init failed!", __func__);
   }

   /* Register TM level change handler function to the platform */
   status = hddDevTmRegisterNotifyCallback(pHddCtx);
   if ( !VOS_IS_STATUS_SUCCESS( status ) )
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,"%s: hddDevTmRegisterNotifyCallback failed",__func__);
      goto err_unregister_pmops;
   }

   /* register for riva power on lock to platform driver */
   if (req_riva_power_on_lock("wlan"))
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,"%s: req riva power on lock failed",
                                     __func__);
      goto err_unregister_pmops;
   }
#if !defined(CONFIG_HDD_INIT_WITH_RTNL_LOCK)
   // register net device notifier for device change notification
   ret = register_netdevice_notifier(&hdd_netdev_notifier);

   if(ret < 0)
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: register_netdevice_notifier failed",__func__);
      goto err_free_power_on_lock;
   }
   reg_netdev_notifier_done = TRUE;
#endif

#ifdef WLAN_KD_READY_NOTIFIER
   pHddCtx->kd_nl_init = 1;
#endif /* WLAN_KD_READY_NOTIFIER */

#ifdef FEATURE_OEM_DATA_SUPPORT
   //Initialize the OEM service
   if (oem_activate_service(pHddCtx) != 0)
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,
             "%s: oem_activate_service failed", __func__);
      goto err_reg_netdev;
   }
#endif

#ifdef PTT_SOCK_SVC_ENABLE
   //Initialize the PTT service
   if(ptt_sock_activate_svc(pHddCtx) != 0)
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,"%s: ptt_sock_activate_svc failed",__func__);
      goto err_reg_netdev;
   }
#endif

   if (hdd_open_cesium_nl_sock() < 0)
      hddLog(VOS_TRACE_LEVEL_WARN, FL("hdd_open_cesium_nl_sock failed"));

   //Initialize the CNSS-DIAG service
   if (cnss_diag_activate_service() < 0)
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,
             "%s: cnss_diag_activate_service failed", __func__);
      goto err_close_cesium;
   }

   hdd_register_mcast_bcast_filter(pHddCtx);
   wlan_hdd_cfg80211_register_frames(pAdapter);

   mutex_init(&pHddCtx->sap_lock);

#if defined(CONFIG_HDD_INIT_WITH_RTNL_LOCK)
   if (rtnl_lock_enable == TRUE) {
      rtnl_lock_enable = FALSE;
      rtnl_unlock();
   }
   /* register net device notifier for device change notification */
   ret = register_netdevice_notifier(&hdd_netdev_notifier);
   if (ret < 0) {
      hddLog(VOS_TRACE_LEVEL_ERROR,"%s: register_netdevice_notifier failed",
            __func__);
      goto err_close_cesium;
   }
   reg_netdev_notifier_done = TRUE;
#endif

   /* Initialize the wake lcok */
   vos_wake_lock_init(&pHddCtx->rx_wake_lock,
           "qcom_rx_wakelock");
   /* Initialize the wake lcok */
   vos_wake_lock_init(&pHddCtx->sap_wake_lock,
           "qcom_sap_wakelock");

   hdd_hostapd_channel_wakelock_init(pHddCtx);

   // Initialize the restart logic
   wlan_hdd_restart_init(pHddCtx);

   if(pHddCtx->cfg_ini->enablePowersaveOffload)
   {
      hdd_set_idle_ps_config(pHddCtx, TRUE);
   }

   if ((pHddCtx->cfg_ini->enable_ac_txq_optimize >> 4) & 0x01)
      sme_set_ac_txq_optimize(pHddCtx->hHal,
                              &pHddCtx->cfg_ini->enable_ac_txq_optimize);

   if (pHddCtx->cfg_ini->enable_go_cts2self_for_sta)
       sme_set_cts2self_for_p2p_go(pHddCtx->hHal);

   /* Reset previous stats before turning on/off */
   vos_mem_set(&pAdapter->mib_stats,
                sizeof(pAdapter->mib_stats), 0);

   hal_status = sme_set_mib_stats_enable(pHddCtx->hHal,
                     pHddCtx->cfg_ini->mib_stats_enabled);

   if (eHAL_STATUS_SUCCESS != hal_status)
           hddLog(VOS_TRACE_LEVEL_ERROR, FL("set mib stats failed"));

#ifdef FEATURE_WLAN_AUTO_SHUTDOWN
   if (pHddCtx->cfg_ini->WlanAutoShutdown != 0)
       if (sme_set_auto_shutdown_cb(pHddCtx->hHal, wlan_hdd_auto_shutdown_cb)
           != eHAL_STATUS_SUCCESS)
           hddLog(LOGE, FL("Auto shutdown feature could not be enabled"));
#endif

#ifdef FEATURE_WLAN_AP_AP_ACS_OPTIMIZE
   status = vos_timer_init(&pHddCtx->skip_acs_scan_timer, VOS_TIMER_TYPE_SW,
                  hdd_skip_acs_scan_timer_handler, (void *)pHddCtx);
   if (!VOS_IS_STATUS_SUCCESS(status))
        hddLog(LOGE, FL("Failed to init ACS Skip timer\n"));
   adf_os_spinlock_init(&pHddCtx->acs_skip_lock);
#endif

#ifdef WLAN_FEATURE_NAN
    wlan_hdd_cfg80211_nan_init(pHddCtx);
#endif

   /* Thermal Mitigation */
   thermalParam.smeThermalMgmtEnabled =
       pHddCtx->cfg_ini->thermalMitigationEnable;
   thermalParam.smeThrottlePeriod = pHddCtx->cfg_ini->throttlePeriod;

   thermalParam.sme_throttle_duty_cycle_tbl[0]=
       pHddCtx->cfg_ini->throttle_dutycycle_level0;
   thermalParam.sme_throttle_duty_cycle_tbl[1]=
       pHddCtx->cfg_ini->throttle_dutycycle_level1;
   thermalParam.sme_throttle_duty_cycle_tbl[2]=
       pHddCtx->cfg_ini->throttle_dutycycle_level2;
   thermalParam.sme_throttle_duty_cycle_tbl[3]=
       pHddCtx->cfg_ini->throttle_dutycycle_level3;

   thermalParam.smeThermalLevels[0].smeMinTempThreshold =
       pHddCtx->cfg_ini->thermalTempMinLevel0;
   thermalParam.smeThermalLevels[0].smeMaxTempThreshold =
       pHddCtx->cfg_ini->thermalTempMaxLevel0;
   thermalParam.smeThermalLevels[1].smeMinTempThreshold =
       pHddCtx->cfg_ini->thermalTempMinLevel1;
   thermalParam.smeThermalLevels[1].smeMaxTempThreshold =
       pHddCtx->cfg_ini->thermalTempMaxLevel1;
   thermalParam.smeThermalLevels[2].smeMinTempThreshold =
       pHddCtx->cfg_ini->thermalTempMinLevel2;
   thermalParam.smeThermalLevels[2].smeMaxTempThreshold =
       pHddCtx->cfg_ini->thermalTempMaxLevel2;
   thermalParam.smeThermalLevels[3].smeMinTempThreshold =
       pHddCtx->cfg_ini->thermalTempMinLevel3;
   thermalParam.smeThermalLevels[3].smeMaxTempThreshold =
       pHddCtx->cfg_ini->thermalTempMaxLevel3;

   hdd_get_thermal_shutdown_ini_param(&thermalParam, pHddCtx);

   if (eHAL_STATUS_SUCCESS != sme_InitThermalInfo(pHddCtx->hHal,thermalParam))
   {
       hddLog(VOS_TRACE_LEVEL_ERROR,
               "%s: Error while initializing thermal information", __func__);
   }

   /* Runtime DPD Recaliberation config*/
   hdd_get_DPD_Recaliberation_ini_param(&DPDParam, pHddCtx);

   if (eHAL_STATUS_SUCCESS != sme_InitDPDRecalInfo(pHddCtx->hHal, DPDParam))
   {
       hddLog(VOS_TRACE_LEVEL_ERROR,
               "%s: Error while initializing Runtime DPD Recaliberation information", __func__);
   }

   /* Plug in set thermal level callback */
   sme_add_set_thermal_level_callback(pHddCtx->hHal,
                     hdd_set_thermal_level_cb);

   sme_add_thermal_temperature_ind_callback(pHddCtx->hHal,
                    (tSmeThermalTempIndCb)hdd_thermal_temp_ind_event_cb);

   /* Bad peer tx flow control */
   wlan_hdd_bad_peer_txctl(pHddCtx);

   /* SAR power limit */
   hddtxlimit = vos_mem_malloc(sizeof(tSirTxPowerLimit));
   if (!hddtxlimit)
   {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "%s: Memory allocation for TxPowerLimit "
                 "failed!", __func__);
       goto err_close_cesium;
   }
   hddtxlimit->txPower2g = pHddCtx->cfg_ini->TxPower2g;
   hddtxlimit->txPower5g = pHddCtx->cfg_ini->TxPower5g;

   if (eHAL_STATUS_SUCCESS != sme_TxpowerLimit(pHddCtx->hHal,hddtxlimit))
   {
        hddLog(VOS_TRACE_LEVEL_ERROR,
               "%s: Error setting txlimit in sme", __func__);
   }

   vos_timer_init(&pHddCtx->tdls_source_timer, VOS_TIMER_TYPE_SW,
                  wlan_hdd_change_tdls_mode, (void *)pHddCtx);

#ifdef CONFIG_IXC_TIMER
   pHddCtx->ixc_pid = 0;
   vos_timer_init(&pHddCtx->set_ixc_prio_timer,
                     VOS_TIMER_TYPE_SW,
                     hdd_set_ixc_prio,
                     (void *)pHddCtx);
   vos_timer_start(&pHddCtx->set_ixc_prio_timer, 10);
#endif
#ifdef FEATURE_BUS_BANDWIDTH
   adf_os_spinlock_init(&pHddCtx->bus_bw_lock);
   vos_timer_init(&pHddCtx->bus_bw_timer,
                     VOS_TIMER_TYPE_SW,
                     hdd_bus_bw_compute_cbk,
                     (void *)pHddCtx);
#endif

#ifdef WLAN_FEATURE_STATS_EXT
   wlan_hdd_cfg80211_stats_ext_init(pHddCtx);
#endif
#ifdef FEATURE_WLAN_EXTSCAN
    sme_ExtScanRegisterCallback(pHddCtx->hHal,
                                wlan_hdd_cfg80211_extscan_callback);
#endif /* FEATURE_WLAN_EXTSCAN */
    sme_chain_rssi_register_callback(pHddCtx->hHal,
                                wlan_hdd_cfg80211_chainrssi_callback);
    sme_set_rssi_threshold_breached_cb(pHddCtx->hHal, hdd_rssi_threshold_breached);
    wlan_hdd_cfg80211_link_layer_stats_init(pHddCtx);
    wlan_hdd_tsf_init(pHddCtx);

#ifdef WLAN_FEATURE_MOTION_DETECTION
    sme_set_mt_host_ev_cb(pHddCtx->hHal, hdd_mt_host_ev_cb, pHddCtx);
#endif

#ifdef WLAN_FEATURE_LPSS
   wlan_hdd_send_all_scan_intf_info(pHddCtx);
   wlan_hdd_send_version_pkg(pHddCtx->target_fw_version,
                             pHddCtx->target_hw_version,
                             pHddCtx->target_hw_name);
#endif

   if (WLAN_HDD_RX_HANDLE_RPS == pHddCtx->cfg_ini->rxhandle)
       hdd_set_rps_cpu_mask(pHddCtx);

   hal_status = sme_set_lost_link_info_cb(pHddCtx->hHal,
                                          hdd_lost_link_info_cb);
   /* print error and not block the startup process */
   if (eHAL_STATUS_SUCCESS != hal_status)
       hddLog(LOGE, "%s: set lost link info callback failed", __func__);

   hal_status = sme_set_smps_force_mode_cb(pHddCtx->hHal,
                                           hdd_smps_force_mode_cb);
   if (eHAL_STATUS_SUCCESS != hal_status)
       hddLog(LOGE, FL("set smps force mode callback failed"));

   hal_status = sme_bpf_offload_register_callback(pHddCtx->hHal,
                                    hdd_get_bpf_offload_cb);
   if (eHAL_STATUS_SUCCESS != hal_status)
       hddLog(LOGE, FL("set bpf offload callback failed"));

   wlan_hdd_dcc_register_for_dcc_stats_event(pHddCtx);

   hal_status = sme_register_aid_req_callback(pHddCtx->hHal,
                                              wlan_hdd_cfg80211_aid_req_callback);
   if (eHAL_STATUS_SUCCESS != hal_status)
       hddLog(LOGE, FL("set aid req callback failed"));

   wlan_hdd_init_chan_info(pHddCtx);

   /*
    * Register IPv6 notifier to notify if any change in IP
    * So that we can reconfigure the offload parameters
    */
   hdd_wlan_register_ip6_notifier(pHddCtx);

   /*
    * Register IPv4 notifier to notify if any change in IP
    * So that we can reconfigure the offload parameters
    */
   pHddCtx->ipv4_notifier.notifier_call = wlan_hdd_ipv4_changed;
   ret = register_inetaddr_notifier(&pHddCtx->ipv4_notifier);
   if (ret)
      hddLog(LOGE, FL("Failed to register IPv4 notifier"));
   else
      hddLog(LOG1, FL("Registered IPv4 notifier"));

   ol_pktlog_init(hif_sc);

   /*
    * Send btc page and wlan (p2p/sta/sap) interval to firmware if
    * relevant parameters set in ini file.
    */
   hdd_set_btc_bt_wlan_interval(pHddCtx);

   hdd_runtime_suspend_init(pHddCtx);
   pHddCtx->isLoadInProgress = FALSE;
   vos_set_load_unload_in_progress(VOS_MODULE_ID_VOSS, FALSE);
   vos_set_load_in_progress(VOS_MODULE_ID_VOSS, FALSE);

   if (pHddCtx->cfg_ini->fIsLogpEnabled) {
       vos_wdthread_init_timer_work(vos_process_wd_timer);
       /* Initialize the timer to detect thread stuck issues */
       vos_thread_stuck_timer_init(
               &((VosContextType*)pVosContext)->vosWatchdog);
   }

   if (pHddCtx->cfg_ini->enable_dynamic_sta_chainmask)
      hdd_decide_dynamic_chain_mask(pHddCtx,
                            HDD_ANTENNA_MODE_1X1);

   hdd_driver_memdump_init();

   if (pHddCtx->cfg_ini->goptimize_chan_avoid_event) {
       hal_status = sme_enable_disable_chanavoidind_event(pHddCtx->hHal, 0);
       if (eHAL_STATUS_SUCCESS != hal_status)
           hddLog(LOGE, FL("Failed to disable Chan Avoidance Indcation"));
   }
   if (pHddCtx->cfg_ini->enable_5g_band_pref) {
        band_pref_params.rssi_boost_threshold_5g =
                                  pHddCtx->cfg_ini->rssi_boost_threshold_5g;
        band_pref_params.rssi_boost_factor_5g =
                                  pHddCtx->cfg_ini->rssi_boost_factor_5g;
        band_pref_params.max_rssi_boost_5g =
                                  pHddCtx->cfg_ini->max_rssi_boost_5g;
        band_pref_params.rssi_penalize_threshold_5g =
                                  pHddCtx->cfg_ini->rssi_penalize_threshold_5g;
        band_pref_params.rssi_penalize_factor_5g =
                                  pHddCtx->cfg_ini->rssi_penalize_factor_5g;
        band_pref_params.max_rssi_penalize_5g =
                                  pHddCtx->cfg_ini->max_rssi_penalize_5g;
        sme_set_5g_band_pref(pHddCtx->hHal, &band_pref_params);
   }


   if (pHddCtx->cfg_ini->sifs_burst_duration) {
       set_value = (SIFS_BURST_DUR_MULTIPLIER) *
                    pHddCtx->cfg_ini->sifs_burst_duration;

       if ((set_value > 0) && (set_value <= SIFS_BURST_DUR_MAX))
           process_wma_set_command(0, (int)WMI_PDEV_PARAM_BURST_DUR,
                                          set_value, PDEV_CMD);
   }

#ifdef WLAN_FEATURE_USB_RECOVERY
   schedule_delayed_work(&pHddCtx->usb_detect_work, msecs_to_jiffies(6000));
#endif

   if (pHddCtx->cfg_ini->max_mpdus_inampdu) {
       set_value = pHddCtx->cfg_ini->max_mpdus_inampdu;
       process_wma_set_command(0, (int)WMI_PDEV_PARAM_MAX_MPDUS_IN_AMPDU,
                                      set_value, PDEV_CMD);
   }

   if (pHddCtx->cfg_ini->enable_rts_sifsbursting) {
       set_value = pHddCtx->cfg_ini->enable_rts_sifsbursting;
       process_wma_set_command(0, (int)WMI_PDEV_PARAM_ENABLE_RTS_SIFS_BURSTING,
                                      set_value, PDEV_CMD);

   }

   if (hdd_wlan_enable_egap(pHddCtx))
        hddLog(LOGE, FL("enhance green ap is not enabled"));

   /* set chip power save failure detected callback */
   sme_set_chip_pwr_save_fail_cb(pHddCtx->hHal,
                                 hdd_chip_pwr_save_fail_detected_cb);
   sme_enable_aid_by_user(pHddCtx->hHal, pHddCtx->cfg_ini->aid_by_user);

   wlan_comp.status = 0;
   complete(&wlan_comp.wlan_start_comp);
   goto success;

err_close_cesium:
   hdd_close_cesium_nl_sock();

err_reg_netdev:
   if (rtnl_lock_enable == TRUE) {
      rtnl_lock_enable = FALSE;
      rtnl_unlock();
   }
   if (reg_netdev_notifier_done == TRUE) {
      unregister_netdevice_notifier(&hdd_netdev_notifier);
      reg_netdev_notifier_done = FALSE;
   }

#if !defined(CONFIG_HDD_INIT_WITH_RTNL_LOCK)
err_free_power_on_lock:
#endif
   free_riva_power_on_lock("wlan");

err_unregister_pmops:
   hddDevTmUnregisterNotifyCallback(pHddCtx);
   hddDeregisterPmOps(pHddCtx);

   hdd_debugfs_exit(pHddCtx);

err_close_adapter:
#if defined(CONFIG_HDD_INIT_WITH_RTNL_LOCK)
   if (rtnl_lock_enable == TRUE) {
      rtnl_lock_enable = FALSE;
      rtnl_unlock();
   }
#endif
   hdd_close_all_adapters( pHddCtx );

#ifdef QCA_PKT_PROTO_TRACE
   if (VOS_FTM_MODE != hdd_get_conparam())
       vos_pkt_proto_trace_close();
#endif /* QCA_PKT_PROTO_TRACE */

err_vosstop:
   vos_stop(pVosContext);

err_spectral_deinit:
   hdd_spectral_deinit(pHddCtx);

#ifdef IPA_OFFLOAD
err_ipa_cleanup:
   hdd_ipa_cleanup(pHddCtx);
#endif

err_wiphy_unregister:
   wiphy_unregister(wiphy);

err_vosclose:
   status = vos_sched_close( pVosContext );
   if (!VOS_IS_STATUS_SUCCESS(status))    {
      VOS_TRACE( VOS_MODULE_ID_VOSS, VOS_TRACE_LEVEL_FATAL,
         "%s: Failed to close VOSS Scheduler", __func__);
      VOS_ASSERT( VOS_IS_STATUS_SUCCESS( status ) );
   }
   vos_close(pVosContext );

err_vos_nv_close:

   vos_nv_close();

   hdd_wlan_green_ap_deinit(pHddCtx);
   hdd_request_manager_deinit();

err_wdclose:
   if(pHddCtx->cfg_ini->fIsLogpEnabled)
      vos_watchdog_close(pVosContext);

if (VOS_FTM_MODE == hdd_get_conparam())
{
#if  defined(QCA_WIFI_FTM)
err_free_ftm_open:
   wlan_hdd_ftm_close(pHddCtx);
#endif
}

err_nl_srv:
   nl_srv_exit();

err_logging_sock:
   if (VOS_FTM_MODE != hdd_get_conparam())
       wlan_hdd_logging_sock_deactivate_svc(pHddCtx);

err_sock_activate:
   wlan_hdd_cfg80211_deinit(wiphy);

err_easy_wow:
   hdd_easy_wow_deinit(pHddCtx);

err_config:
   vos_mem_free(pHddCtx->cfg_ini);
   pHddCtx->cfg_ini= NULL;

err_histogram:
   wlan_hdd_deinit_tx_rx_histogram(pHddCtx);

err_free_hdd_context:
   /* wiphy_free() will free the HDD context so remove global reference */
   if (pVosContext) {
      hdd_free_probe_req_ouis(pHddCtx);
      ((VosContextType*)(pVosContext))->pHDDContext = NULL;
   }

   wiphy_free(wiphy) ;
   //kfree(wdev) ;
   VOS_BUG(1);

   if (hdd_is_ssr_required())
   {
#ifdef MSM_PLATFORM
#ifdef CONFIG_CNSS
       /* WDI timeout had happened during load, so SSR is needed here */
       subsystem_restart("wcnss");
#endif
#endif
       msleep(5000);
   }
   hdd_set_ssr_required (VOS_FALSE);

   wlan_comp.status = -EAGAIN;
   complete(&wlan_comp.wlan_start_comp);
   return -EIO;

success:
#ifdef WLAN_FEATURE_USB_RECOVERY
   usb_recovery_status = 0;
   pr_err("%s usb_recovery_status = 0\n", __func__);
#endif
   EXIT();
   return 0;
}

/* accommodate the request firmware bin time out 2 min */
#define REQUEST_FWR_TIMEOUT 120000
#define HDD_WLAN_START_WAIT_TIME (VOS_WDA_TIMEOUT + 5000 + REQUEST_FWR_TIMEOUT)
/**
 * hdd_hif_register_driver() - API for HDD to register with HIF
 *
 * API for HDD to register with HIF layer
 *
 * Return: success/failure
 */
int hdd_hif_register_driver(void)
{
	int ret;
	unsigned long rc, timeout;

	init_completion(&wlan_comp.wlan_start_comp);
	wlan_comp.status = 0;

	ret = hif_register_driver();

	if (ret) {
		hddLog(LOGE, FL("HIF registration failed"));
		return ret;
	}

	timeout = msecs_to_jiffies(HDD_WLAN_START_WAIT_TIME);

	rc = wait_for_completion_timeout(&wlan_comp.wlan_start_comp, timeout);

	if (!rc) {
		hddLog(LOGE, FL("hif registration timedout"));
		return -EAGAIN;
	}

	if (wlan_comp.status)
		hddLog(LOGE,
		       FL("hdd_wlan_startup failed status:%d jiffies_left:%lu"),
		       wlan_comp.status, rc);

	return wlan_comp.status;
}

#ifdef TIMER_MANAGER
static inline void hdd_timer_exit(void)
{
	vos_timer_exit();
}
#else
static inline void hdd_timer_exit(void)
{
}
#endif

#ifdef MEMORY_DEBUG
static inline void hdd_mem_exit(void)
{
	adf_net_buf_debug_exit();
	adf_nbuf_map_check_for_leaks();
	vos_mem_exit();
}
#else
static inline void hdd_mem_exit(void)
{
}
#endif

#ifdef WLAN_LOGGING_SOCK_SVC_ENABLE
static inline void hdd_logging_sock_deinit_svc(void)
{
	wlan_logging_sock_deinit_svc();
}
#else
static inline void hdd_logging_sock_deinit_svc(void)
{
}
#endif

static int hdd_register_fail_clean_up(v_CONTEXT_t vos_context)
{
	hif_unregister_driver();
	vos_preClose(&vos_context);
	hdd_timer_exit();
	hdd_mem_exit();
	hdd_logging_sock_deinit_svc();

	return -ENODEV;
}

/**---------------------------------------------------------------------------

  \brief hdd_driver_init() - Core Driver Init Function

   This is the driver entry point - called in different time line depending
   on whether the driver is statically or dynamically linked

  \param  - None

  \return - 0 for success, non zero for failure

  --------------------------------------------------------------------------*/
static int hdd_driver_init( void)
{
   VOS_STATUS status;
   v_CONTEXT_t pVosContext = NULL;
   int ret_status = 0;
   u_int64_t start;

   start = adf_get_boottime();

   adf_os_spinlock_init(&hdd_context_lock);
#ifdef WLAN_LOGGING_SOCK_SVC_ENABLE
   wlan_logging_sock_init_svc();
#endif

   ENTER();

#ifdef TIMER_MANAGER
      vos_timer_manager_init();
#endif

#ifdef MEMORY_DEBUG
      vos_mem_init();
      adf_net_buf_debug_init();
#endif

   hdd_wlan_wakelock_create();
   hdd_prevent_suspend(WIFI_POWER_EVENT_WAKELOCK_DRIVER_INIT);

   /*
    * The Krait is going to Idle/Stand Alone Power Save
    * more aggressively which is resulting in the longer driver load time.
    * The Fix is to not allow Krait to enter Idle Power Save during driver load.
    * at a time, and wait for the completion interrupt to start the next
    * transfer. During this phase, the KRAIT is entering IDLE/StandAlone(SA)
    * Power Save(PS). The delay incurred for resuming from IDLE/SA PS is
    * huge during driver load. So prevent APPS IDLE/SA PS during driver
    * load for reducing interrupt latency.
    */

   vos_request_pm_qos_type(PM_QOS_CPU_DMA_LATENCY, DISABLE_KRAIT_IDLE_PS_VAL);

   vos_ssr_protect_init();

   pr_info("%s: loading driver v%s\n", WLAN_MODULE_NAME,
           QWLAN_VERSIONSTR TIMER_MANAGER_STR MEMORY_DEBUG_STR);

#ifdef CONFIG_VOS_MEM_PRE_ALLOC
   wcnss_prealloc_reset();
#endif /* CONFIG_VOS_MEM_PRE_ALLOC */

   do {

#ifndef MODULE
      /* For statically linked driver, call hdd_set_conparam to update curr_con_mode
       */
      hdd_set_conparam((v_UINT_t)con_mode);
      if (WLAN_IS_EPPING_ENABLED(con_mode)) {
         ret_status =  epping_driver_init(con_mode, &wlan_wake_lock,
                          WLAN_MODULE_NAME);
         if (ret_status < 0) {
            vos_remove_pm_qos();
            hdd_allow_suspend(WIFI_POWER_EVENT_WAKELOCK_DRIVER_INIT);
            hdd_wlan_wakelock_destroy();
         }
         return ret_status;
      }
#else
      if (WLAN_IS_EPPING_ENABLED(hdd_get_conparam())) {
         ret_status = epping_driver_init(hdd_get_conparam(),
                         &wlan_wake_lock, WLAN_MODULE_NAME);
         if (ret_status < 0) {
            vos_remove_pm_qos();
            hdd_allow_suspend(WIFI_POWER_EVENT_WAKELOCK_DRIVER_INIT);
            hdd_wlan_wakelock_destroy();
         }
         return ret_status;
      }
#endif

#ifdef FEATURE_USB_WARM_RESET  //Only enabled when warm reset is enabled
      if (VOS_FTM_MODE == con_mode) {
            printk(KERN_ERR "%s calling hif_usb_recovery for FTM mode\n",__func__);
#ifdef WLAN_FEATURE_USB_RECOVERY
            hif_usb_recovery();
#endif
      }
#endif

      /* Preopen VOSS so that it is ready to start at least SAL */
      status = vos_preOpen(&pVosContext);

   if (!VOS_IS_STATUS_SUCCESS(status))
   {
         hddLog(VOS_TRACE_LEVEL_FATAL,"%s: Failed to preOpen VOSS", __func__);
         vos_remove_pm_qos();
         ret_status = -1;
         break;
   }

#ifdef HDD_TRACE_RECORD
   MTRACE(hddTraceInit());
#endif
   hdd_register_debug_callback();

   ret_status = hdd_hif_register_driver();
   vos_remove_pm_qos();

   if (ret_status == 0) {
      pr_info("%s: driver loaded in %lld\n", WLAN_MODULE_NAME,
             adf_get_boottime() - start);
      hdd_allow_suspend(WIFI_POWER_EVENT_WAKELOCK_DRIVER_INIT);
      return 0;
   }

   hddLog(VOS_TRACE_LEVEL_FATAL, "%s: WLAN Driver Initialization failed",
          __func__);

   ret_status = hdd_register_fail_clean_up(pVosContext);
   hdd_allow_suspend(WIFI_POWER_EVENT_WAKELOCK_DRIVER_INIT);
   hdd_wlan_wakelock_destroy();

   } while (0);

   EXIT();

   return ret_status;
}
#ifdef FEATURE_LARGE_PREALLOC
EXPORT_SYMBOL(hdd_driver_init);
#endif
/**---------------------------------------------------------------------------

  \brief hdd_module_init() - Init Function

   This is the driver entry point (invoked when module is loaded using insmod)

  \param  - None

  \return - 0 for success, non zero for failure

  --------------------------------------------------------------------------*/
#ifndef FEATURE_LARGE_PREALLOC
#ifdef MODULE
static int __init hdd_module_init ( void)
{
   return hdd_driver_init();
}
#else /* #ifdef MODULE */
static int __init hdd_module_init ( void)
{
   /* Driver initialization is delayed to fwpath_changed_handler */
   return 0;
}
#endif /* #ifdef MODULE */
#endif /* #ifndef FEATURE_LARGE_PREALLOC*/

static struct timer_list unload_timer;
static bool unload_timer_started;

#ifdef CONFIG_SLUB_DEBUG_ON
#define HDD_UNLOAD_WAIT_TIME 35000
#else
#define HDD_UNLOAD_WAIT_TIME 30000
#endif

/**
 * hdd_unload_timer_del() - API to Delete unload timer
 *
 * Delete unload timer
 *
 * Return: None
 */
static void hdd_unload_timer_del(void)
{
	adf_os_timer_cancel(&unload_timer);
	unload_timer_started = false;
}

/**
 * hdd_unload_timer_cb() - Unload timer callback function
 *
 * Unload timer callback function
 *
 * Return: None
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
static void hdd_unload_timer_cb(struct timer_list *t)
#else
static void hdd_unload_timer_cb(void *data)
#endif
{
	v_CONTEXT_t vos_context = NULL;
	hdd_context_t *hdd_ctx = NULL;

	pr_err("HDD unload timer expired!,current unload status: %d",
		g_current_unload_state);
	/* Get the global VOSS context. */
	vos_context = vos_get_global_context(VOS_MODULE_ID_SYS, NULL);
	if(vos_context) {
		hdd_ctx = (hdd_context_t *)vos_get_context(VOS_MODULE_ID_HDD,
						vos_context );
	if(hdd_ctx)
		pr_err("Driver loading: %d unloading:%d logp_in_progress: %d",
				hdd_ctx->isLoadInProgress,
				hdd_ctx->isUnloadInProgress,
			hdd_ctx->isLogpInProgress);
	} else
		pr_err("%s: Global VOS context is Null", __func__);

#ifdef CONFIG_SLUB_DEBUG_ON
	VOS_BUG(0);
#endif
}

/**
 * hdd_unload_timer_init() - API to initialize unload timer
 *
 * initialize unload timer
 *
 * Return: None
 */
static void hdd_unload_timer_init(void)
{
	adf_os_timer_init(NULL, &unload_timer,
			  hdd_unload_timer_cb, NULL, ADF_NON_DEFERRABLE_TIMER);
}

/**
 * hdd_unload_timer_start() - API to start unload timer
 * @msec: timer interval in msec units
 *
 * API to start unload timer
 *
 * Return: None
 */
static void hdd_unload_timer_start(int msec)
{
	if(unload_timer_started)
		hddLog(VOS_TRACE_LEVEL_FATAL,
			"%s: Starting unload timer when it's running!",
			__func__);
	adf_os_timer_start(&unload_timer, msec);
	unload_timer_started = true;
}
/**---------------------------------------------------------------------------

  \brief hdd_driver_exit() - Exit function

  This is the driver exit point (invoked when module is unloaded using rmmod
  or con_mode was changed by user space)

  \param  - None

  \return - None

  --------------------------------------------------------------------------*/
static void hdd_driver_exit(void)
{
   hdd_context_t *pHddCtx = NULL;
   int retry = 0;
   v_CONTEXT_t pVosContext = NULL;

   pr_err("%s: unloading driver v%s\n", WLAN_MODULE_NAME, QWLAN_VERSIONSTR);

   //Get the global vos context
   pVosContext = vos_get_global_context(VOS_MODULE_ID_SYS, NULL);

   if(!pVosContext)
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,"%s: Global VOS context is Null", __func__);
      goto done;
   }

   if (WLAN_IS_EPPING_ENABLED(con_mode)) {
      epping_driver_exit(pVosContext);
      goto done;
   }

   //Get the HDD context.
   pHddCtx = (hdd_context_t *)vos_get_context(VOS_MODULE_ID_HDD, pVosContext );

#define MAX_USB_RECOVERY_COUNT  100  /* 10s is enough fo recovery */

#ifdef WLAN_FEATURE_USB_RECOVERY
   if ( (VOS_FTM_MODE != hdd_get_conparam()) && pHddCtx ) {
       int cnt = 0;
       pr_err("%s: mutext lock\n", __func__);
       mutex_lock(&pHddCtx->usb_recovery_lock);
       pHddCtx->isUnloadInProgress = TRUE;
       while( usb_recovery_status && (cnt++ < MAX_USB_RECOVERY_COUNT) ) {
              msleep(100);
              pr_err("%s: usb_recovery_satus is 1  ##\n", __func__);
       }
       mutex_unlock(&pHddCtx->usb_recovery_lock);
       pr_err("%s: mutext unlock, cnt=%d\n", __func__, cnt);
   }
#endif

   if(!pHddCtx)
   {
      hddLog(VOS_TRACE_LEVEL_FATAL,"%s: module exit called before probe",__func__);
   }
   else
   {
       hdd_thermal_suspend_cleanup(pHddCtx);

      /*
       * Check IPA HW pipe shutdown properly or not
       * If not, force shut down HW pipe
       */
      hdd_ipa_uc_force_pipe_shutdown(pHddCtx);

      pHddCtx->driver_being_stopped = false;

      rtnl_lock();
      pHddCtx->isUnloadInProgress = TRUE;
      vos_set_load_unload_in_progress(VOS_MODULE_ID_VOSS, TRUE);
      vos_set_unload_in_progress(TRUE);
      rtnl_unlock();

      while(pHddCtx->isLogpInProgress ||
            vos_is_logp_in_progress(VOS_MODULE_ID_VOSS, NULL)) {
         VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_FATAL,
              "%s:SSR in Progress; block rmmod for 1 second!!!", __func__);
         msleep(1000);

         if (retry++ == HDD_MOD_EXIT_SSR_MAX_RETRIES) {
            VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_FATAL,
              "%s:SSR never completed, fatal error", __func__);
            VOS_BUG(0);
         }
      }
   }

   vos_wait_for_work_thread_completion(__func__);
   /* If unload never completes, then do kernel panic. */
   hdd_unload_timer_init();
   hdd_unload_timer_start(HDD_UNLOAD_WAIT_TIME);
   hif_unregister_driver();
   hdd_unload_timer_del();

   vos_preClose( &pVosContext );

#ifdef TIMER_MANAGER
   vos_timer_exit();
#endif
#ifdef CONFIG_VOS_MEM_PRE_ALLOC
   wcnss_prealloc_reset();
#endif
#ifdef MEMORY_DEBUG
   adf_net_buf_debug_exit();
   adf_nbuf_map_check_for_leaks();
   vos_mem_exit();
#endif

#ifdef WLAN_LOGGING_SOCK_SVC_ENABLE
   wlan_logging_sock_deinit_svc();
#endif

done:
   hdd_wlan_wakelock_destroy();
   pr_info("%s: driver unloaded\n", WLAN_MODULE_NAME);
}
#ifdef FEATURE_LARGE_PREALLOC
EXPORT_SYMBOL(hdd_driver_exit);
#endif

/**---------------------------------------------------------------------------

  \brief hdd_module_exit() - Exit function

  This is the driver exit point (invoked when module is unloaded using rmmod)

  \param  - None

  \return - None

  --------------------------------------------------------------------------*/
#ifndef FEATURE_LARGE_PREALLOC
static void __exit hdd_module_exit(void)
{
   hdd_driver_exit();
}

#ifdef MODULE
static int fwpath_changed_handler(const char *kmessage,
                                  const struct kernel_param *kp)
{
   return param_set_copystring(kmessage, kp);
}

#if ! defined(QCA_WIFI_FTM)
static int con_mode_handler(const char *kmessage,
                            const struct kernel_param *kp)
{
   return param_set_int(kmessage, kp);
}
#endif
#else /* #ifdef MODULE */

/**---------------------------------------------------------------------------

  \brief fwpath_changed_handler() - Handler Function

   Handle changes to the fwpath parameter

  \return - 0 for success, non zero for failure

  --------------------------------------------------------------------------*/
static int fwpath_changed_handler(const char *kmessage,
                                  const struct kernel_param *kp)
{
	int ret;
	bool mode_change;

	ret = param_set_copystring(kmessage, kp);

	if (!ret) {
		bool ready;

		ret = strncmp(fwpath_mode_local, kmessage , 3);
		mode_change = ret ? true : false;


		pr_info("%s : new_mode : %s, present_mode : %s\n", __func__,
			kmessage, fwpath_mode_local);

		strlcpy(fwpath_mode_local, kmessage,
			sizeof(fwpath_mode_local));

		ready = vos_is_load_unload_ready(__func__);

		if (!ready) {
			VOS_ASSERT(0);
			return -EINVAL;
		}

		vos_load_unload_protect(__func__);
		ret = kickstart_driver(true, mode_change);
		vos_load_unload_unprotect(__func__);
	}

	return ret;
}

#if ! defined(QCA_WIFI_FTM)
/**---------------------------------------------------------------------------

  \brief con_mode_handler() -

  Handler function for module param con_mode when it is changed by user space
  Dynamically linked - do nothing
  Statically linked - exit and init driver, as in rmmod and insmod

  \param  -

  \return -

  --------------------------------------------------------------------------*/
static int con_mode_handler(const char *kmessage,
                            const struct kernel_param *kp)
{
   int ret;

   ret = param_set_int(kmessage, kp);
   if (0 == ret)
      ret = kickstart_driver(true, false);
   return ret;
}
#endif
#endif /* #ifdef MODULE */

#endif
/**---------------------------------------------------------------------------

  \brief hdd_get_conparam() -

  This is the driver exit point (invoked when module is unloaded using rmmod)

  \param  - None

  \return - tVOS_CON_MODE

  --------------------------------------------------------------------------*/
tVOS_CON_MODE hdd_get_conparam ( void )
{
#ifdef MODULE
    return (tVOS_CON_MODE)con_mode;
#else
    return (tVOS_CON_MODE)curr_con_mode;
#endif
}
void hdd_set_conparam ( v_UINT_t newParam )
{
  con_mode = newParam;
#ifndef MODULE
  curr_con_mode = con_mode;
#endif
}
/**---------------------------------------------------------------------------

  \brief hdd_softap_sta_deauth() - function

  This to take counter measure to handle deauth req from HDD

  \param  - pAdapter - Pointer to the HDD

  \param  - enable - boolean value

  \return - None

  --------------------------------------------------------------------------*/

VOS_STATUS hdd_softap_sta_deauth(hdd_adapter_t *pAdapter,
                                 struct tagCsrDelStaParams *pDelStaParams)
{
#ifndef WLAN_FEATURE_MBSSID
    v_CONTEXT_t pVosContext = (WLAN_HDD_GET_CTX(pAdapter))->pvosContext;
#endif
    VOS_STATUS vosStatus = VOS_STATUS_E_FAULT;

    ENTER();

    hddLog(LOG1, "hdd_softap_sta_deauth:(%pK, false)",
           (WLAN_HDD_GET_CTX(pAdapter))->pvosContext);

    //Ignore request to deauth bcmc station
    if (pDelStaParams->peerMacAddr[0] & 0x1)
       return vosStatus;

#ifdef WLAN_FEATURE_MBSSID
    vosStatus = WLANSAP_DeauthSta(WLAN_HDD_GET_SAP_CTX_PTR(pAdapter),
                                  pDelStaParams);
#else
    vosStatus = WLANSAP_DeauthSta(pVosContext, pDelStaParams);
#endif

    EXIT();
    return vosStatus;
}

/**---------------------------------------------------------------------------

  \brief hdd_softap_sta_disassoc() - function

  This to take counter measure to handle deauth req from HDD

  \param  - pAdapter - Pointer to the HDD

  \param  - enable - boolean value

  \return - None

  --------------------------------------------------------------------------*/

void hdd_softap_sta_disassoc(hdd_adapter_t *pAdapter,
                             struct tagCsrDelStaParams *pDelStaParams)
{
#ifndef WLAN_FEATURE_MBSSID
    v_CONTEXT_t pVosContext = (WLAN_HDD_GET_CTX(pAdapter))->pvosContext;
#endif

    ENTER();

    hddLog( LOGE, "hdd_softap_sta_disassoc:(%pK, false)", (WLAN_HDD_GET_CTX(pAdapter))->pvosContext);

    //Ignore request to disassoc bcmc station
    if( pDelStaParams->peerMacAddr[0] & 0x1 )
       return;

#ifdef WLAN_FEATURE_MBSSID
    WLANSAP_DisassocSta(WLAN_HDD_GET_SAP_CTX_PTR(pAdapter), pDelStaParams);
#else
    WLANSAP_DisassocSta(pVosContext, pDelStaParams);
#endif
}

void hdd_softap_tkip_mic_fail_counter_measure(hdd_adapter_t *pAdapter,v_BOOL_t enable)
{
#ifndef WLAN_FEATURE_MBSSID
    v_CONTEXT_t pVosContext = (WLAN_HDD_GET_CTX(pAdapter))->pvosContext;
#endif

    ENTER();

    hddLog( LOGE, "hdd_softap_tkip_mic_fail_counter_measure:(%pK, false)", (WLAN_HDD_GET_CTX(pAdapter))->pvosContext);

#ifdef WLAN_FEATURE_MBSSID
    WLANSAP_SetCounterMeasure(WLAN_HDD_GET_SAP_CTX_PTR(pAdapter), (v_BOOL_t)enable);
#else
    WLANSAP_SetCounterMeasure(pVosContext, (v_BOOL_t)enable);
#endif
}

/**---------------------------------------------------------------------------
 *
 *   \brief hdd_get__concurrency_mode() -
 *
 *
 *   \param  - None
 *
 *   \return - CONCURRENCY MODE
 *
 * --------------------------------------------------------------------------*/
tVOS_CONCURRENCY_MODE hdd_get_concurrency_mode ( void )
{
    v_CONTEXT_t pVosContext = vos_get_global_context( VOS_MODULE_ID_HDD, NULL );
    hdd_context_t *pHddCtx;

    if (NULL != pVosContext)
    {
       pHddCtx = vos_get_context( VOS_MODULE_ID_HDD, pVosContext);
       if (NULL != pHddCtx)
       {
          hddLog(VOS_TRACE_LEVEL_INFO, "%s: concurrency_mode = 0x%x", __func__,
                                        pHddCtx->concurrency_mode);
          return (tVOS_CONCURRENCY_MODE)pHddCtx->concurrency_mode;
       }
    }

    /* we are in an invalid state :( */
    hddLog(LOGE, "%s: Invalid context", __func__);
    return VOS_STA;
}

/* Decides whether to send suspend notification to Riva
 * if any adapter is in BMPS; then it is required */
v_BOOL_t hdd_is_suspend_notify_allowed(hdd_context_t* pHddCtx)
{
    tPmcState pmcState = pmcGetPmcState(pHddCtx->hHal);
    hdd_config_t *pConfig = pHddCtx->cfg_ini;

    if (pConfig->fIsBmpsEnabled && (pmcState == BMPS))
    {
        return TRUE;
    }
    return FALSE;
}

void wlan_hdd_set_concurrency_mode(hdd_context_t *pHddCtx, tVOS_CON_MODE mode)
{
	switch (mode) {
		case VOS_STA_MODE:
		case VOS_P2P_CLIENT_MODE:
		case VOS_P2P_GO_MODE:
		case VOS_STA_SAP_MODE:
			pHddCtx->concurrency_mode |= (1 << mode);
			pHddCtx->no_of_open_sessions[mode]++;
			break;
		default:
			break;
	}

	hddLog(VOS_TRACE_LEVEL_INFO, FL("concurrency_mode = 0x%x, Number of open sessions for mode %d = %d"),
			pHddCtx->concurrency_mode, mode,
			pHddCtx->no_of_open_sessions[mode]);

	hdd_wlan_green_ap_start_bss(pHddCtx);
}


void wlan_hdd_clear_concurrency_mode(hdd_context_t *pHddCtx, tVOS_CON_MODE mode)
{
	switch (mode)  {
		case VOS_STA_MODE:
		case VOS_P2P_CLIENT_MODE:
		case VOS_P2P_GO_MODE:
		case VOS_STA_SAP_MODE:
			pHddCtx->no_of_open_sessions[mode]--;
			if (!(pHddCtx->no_of_open_sessions[mode]))
				pHddCtx->concurrency_mode &= (~(1 << mode));
			break;
		default:
			break;
	}

	hddLog(VOS_TRACE_LEVEL_INFO,
			FL("concurrency_mode = 0x%x, Number of open sessions for mode %d = %d"),
			pHddCtx->concurrency_mode, mode,
			pHddCtx->no_of_open_sessions[mode]);

	hdd_wlan_green_ap_start_bss(pHddCtx);
}

/**---------------------------------------------------------------------------
 *
 *   \brief wlan_hdd_incr_active_session()
 *
 *   This function increments the number of active sessions
 *   maintained per device mode
 *   Incase of STA/P2P CLI/IBSS upon connection indication it is incremented
 *   Incase of SAP/P2P GO upon bss start it is incremented
 *
 *   \param  pHddCtx - HDD Context
 *   \param  mode    - device mode
 *
 *   \return - None
 *
 * --------------------------------------------------------------------------*/
void wlan_hdd_incr_active_session(hdd_context_t *pHddCtx, tVOS_CON_MODE mode)
{
   switch (mode) {
   case VOS_STA_MODE:
   case VOS_P2P_CLIENT_MODE:
   case VOS_P2P_GO_MODE:
   case VOS_STA_SAP_MODE:
        pHddCtx->no_of_active_sessions[mode]++;
        break;
   default:
        break;
   }
   hddLog(VOS_TRACE_LEVEL_INFO, FL("No.# of active sessions for mode %d = %d"),
                                mode,
                                pHddCtx->no_of_active_sessions[mode]);
}

/**---------------------------------------------------------------------------
 *
 *   \brief wlan_hdd_decr_active_session()
 *
 *   This function decrements the number of active sessions
 *   maintained per device mode
 *   Incase of STA/P2P CLI/IBSS upon disconnection it is decremented
 *   Incase of SAP/P2P GO upon bss stop it is decremented
 *
 *   \param  pHddCtx - HDD Context
 *   \param  mode    - device mode
 *
 *   \return - None
 *
 * --------------------------------------------------------------------------*/
void wlan_hdd_decr_active_session(hdd_context_t *pHddCtx, tVOS_CON_MODE mode)
{
   switch (mode) {
   case VOS_STA_MODE:
   case VOS_P2P_CLIENT_MODE:
   case VOS_P2P_GO_MODE:
   case VOS_STA_SAP_MODE:
        if (pHddCtx->no_of_active_sessions[mode])
            pHddCtx->no_of_active_sessions[mode]--;
        break;
   default:
        break;
   }
   hddLog(VOS_TRACE_LEVEL_INFO, FL("No.# of active sessions for mode %d = %d"),
                                mode,
                                pHddCtx->no_of_active_sessions[mode]);
}

/**
 * wlan_hdd_get_active_session_count() - get active session
 * connection count
 * @hdd_ctx: Pointer to hdd context
 *
 * Return: count of active connections
 */
uint8_t wlan_hdd_get_active_session_count(hdd_context_t *hdd_ctx)
{
	uint8_t i = 0;
	uint8_t count = 0;

	for (i = 0; i < VOS_MAX_NO_OF_MODE; i++) {
		count += hdd_ctx->no_of_active_sessions[i];
	}
	return count;
}

/**
 * wlan_hdd_update_txrx_chain_mask() - updates the TX/RX chain
 * mask to FW
 * @hdd_ctx: Pointer to hdd context
 * @chain_mask : Vlaue of the chain_mask to be updated
 *
 * Return: 0 for success non-zero for failure
 */
int wlan_hdd_update_txrx_chain_mask(hdd_context_t *hdd_ctx,
				    uint8_t chain_mask)
{
	int ret;

	if (hdd_ctx->per_band_chainmask_supp == 1) {
		ret = process_wma_set_command(0,
				WMI_PDEV_PARAM_RX_CHAIN_MASK_2G,
				chain_mask, PDEV_CMD);
		if (0 != ret) {
			hddLog(LOGE, FL("Failed to set 2G RX chain mask: %d"),
			       chain_mask);
			return -EFAULT;
		}
		ret = process_wma_set_command(0,
				WMI_PDEV_PARAM_TX_CHAIN_MASK_2G,
				chain_mask, PDEV_CMD);
		if (0 != ret) {
			hddLog(LOGE, FL("Failed to set 2G TX chain mask: %d"),
			       chain_mask);
			return -EFAULT;
		}

		ret = process_wma_set_command(0,
				WMI_PDEV_PARAM_RX_CHAIN_MASK_5G,
				chain_mask, PDEV_CMD);
		if (0 != ret) {
			hddLog(LOGE, FL("Failed to set 5G RX chain mask: %d"),
			       chain_mask);
			return -EFAULT;
		}
		ret = process_wma_set_command(0,
				WMI_PDEV_PARAM_TX_CHAIN_MASK_5G,
				chain_mask, PDEV_CMD);
		if (0 != ret) {
			hddLog(LOGE, FL("Failed to set 5G TX chain mask: %d"),
			       chain_mask);
			return -EFAULT;
		}
	} else {
		ret = process_wma_set_command(0,
					WMI_PDEV_PARAM_RX_CHAIN_MASK,
					chain_mask, PDEV_CMD);
		if (0 != ret) {
			hddLog(LOGE, FL("Failed to set RX chain mask: %d"),
			       chain_mask);
			return -EFAULT;
		}

		ret = process_wma_set_command(0,
					WMI_PDEV_PARAM_TX_CHAIN_MASK,
					chain_mask, PDEV_CMD);
		if (0 != ret) {
			hddLog(LOGE, FL("Failed to set TX chain mask: %d"),
			       chain_mask);
			return -EFAULT;
		}
	}

	hddLog(LOG1, FL("Sucessfully updated the TX/RX chain mask to: %d"),
	       chain_mask);
	return 0;
}

/**---------------------------------------------------------------------------
 *
 *   \brief wlan_hdd_framework_restart
 *
 *   This function uses a cfg80211 API to start a framework initiated WLAN
 *   driver module unload/load.
 *
 *   Also this API keep retrying (WLAN_HDD_RESTART_RETRY_MAX_CNT).
 *
 *
 *   \param  - pHddCtx
 *
 *   \return - VOS_STATUS_SUCCESS: Success
 *             VOS_STATUS_E_EMPTY: Adapter is Empty
 *             VOS_STATUS_E_NOMEM: No memory

 * --------------------------------------------------------------------------*/

static VOS_STATUS wlan_hdd_framework_restart(hdd_context_t *pHddCtx)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
   int len = (sizeof (struct ieee80211_mgmt));
   struct ieee80211_mgmt *mgmt = NULL;

   /* Prepare the DEAUTH management frame with reason code */
   mgmt =  kzalloc(len, GFP_KERNEL);
   if(mgmt == NULL)
   {
      VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_FATAL,
            "%s: memory allocation failed (%d bytes)", __func__, len);
      return VOS_STATUS_E_NOMEM;
   }
   mgmt->u.deauth.reason_code = WLAN_REASON_DISASSOC_LOW_ACK;

   /* Iterate over all adapters/devices */
   status =  hdd_get_front_adapter ( pHddCtx, &pAdapterNode );
   if ((NULL == pAdapterNode) || (VOS_STATUS_SUCCESS != status)) {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 FL("fail to get adapter: %pK %d"), pAdapterNode, status);
       goto end;
   }
   do
   {
      if(pAdapterNode->pAdapter &&
           WLAN_HDD_ADAPTER_MAGIC == pAdapterNode->pAdapter->magic) {
         hddLog(LOGP,
               "restarting the driver(intf:\'%s\' mode:%s(%d) :try %d)",
               pAdapterNode->pAdapter->dev->name,
               hdd_device_mode_to_string(pAdapterNode->pAdapter->device_mode),
               pAdapterNode->pAdapter->device_mode,
               pHddCtx->hdd_restart_retries + 1);
         /*
          * CFG80211 event to restart the driver
          *
          * 'cfg80211_send_unprot_deauth' sends a
          * NL80211_CMD_UNPROT_DEAUTHENTICATE event to supplicant at any state
          * of SME(Linux Kernel) state machine.
          *
          * Reason code WLAN_REASON_DISASSOC_LOW_ACK is currently used to restart
          * the driver.
          *
          */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,11,0)) || defined(WITH_BACKPORTS)
         cfg80211_rx_unprot_mlme_mgmt(pAdapterNode->pAdapter->dev,
                                      (u_int8_t*)mgmt, len);
#else
         cfg80211_send_unprot_deauth(pAdapterNode->pAdapter->dev,
                                     (u_int8_t*)mgmt, len);
#endif
      }
      status = hdd_get_next_adapter ( pHddCtx, pAdapterNode, &pNext );
      pAdapterNode = pNext;
   } while((NULL != pAdapterNode) && (VOS_STATUS_SUCCESS == status));

   end:
   /* Free the allocated management frame */
   kfree(mgmt);

   /* Retry until we unload or reach max count */
   if(++pHddCtx->hdd_restart_retries < WLAN_HDD_RESTART_RETRY_MAX_CNT)
      vos_timer_start(&pHddCtx->hdd_restart_timer, WLAN_HDD_RESTART_RETRY_DELAY_MS);

   return status;

}
/**---------------------------------------------------------------------------
 *
 *   \brief wlan_hdd_restart_timer_cb
 *
 *   Restart timer callback. An internal function.
 *
 *   \param  - User data:
 *
 *   \return - None
 *
 * --------------------------------------------------------------------------*/

void wlan_hdd_restart_timer_cb(v_PVOID_t usrDataForCallback)
{
   hdd_context_t *pHddCtx = usrDataForCallback;
   wlan_hdd_framework_restart(pHddCtx);
   return;

}


/**---------------------------------------------------------------------------
 *
 *   \brief wlan_hdd_restart_driver
 *
 *   This function sends an event to supplicant to restart the WLAN driver.
 *
 *   This function is called from vos_wlanRestart.
 *
 *   \param  - pHddCtx
 *
 *   \return - VOS_STATUS_SUCCESS: Success
 *             VOS_STATUS_E_EMPTY: Adapter is Empty
 *             VOS_STATUS_E_ALREADY: Request already in progress

 * --------------------------------------------------------------------------*/
VOS_STATUS wlan_hdd_restart_driver(hdd_context_t *pHddCtx)
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;

   /* A tight check to make sure reentrancy */
   if(atomic_xchg(&pHddCtx->isRestartInProgress, 1))
   {
      VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_WARN,
            "%s: WLAN restart is already in progress", __func__);

      return VOS_STATUS_E_ALREADY;
   }
   /* Send reset FIQ to WCNSS to invoke SSR. */
#ifdef HAVE_WCNSS_RESET_INTR
   wcnss_reset_intr();
#endif

   return status;
}

/*
 * API to find if there is any STA or P2P-Client is connected
 */
VOS_STATUS hdd_issta_p2p_clientconnected(hdd_context_t *pHddCtx)
{
    return sme_isSta_p2p_clientConnected(pHddCtx->hHal);
}

#ifdef FEATURE_WLAN_CH_AVOID
/**
 * hdd_find_prefd_safe_chnl - Finds safe channel within preferred channel
 * @hdd_ctxt: hdd context pointer
 * @ap_adapter: hdd hostapd adapter pointer
 *
 * If auto channel selection enabled:
 * Preferred and safe channel should be used
 * If no overlapping, preferred channel should be used
 *
 * Return:
 * 1: found preferred safe channel
 * 0: could not found preferred safe channel
 */

uint8_t hdd_find_prefd_safe_chnl(hdd_context_t *hdd_ctxt,
                                                  hdd_adapter_t *ap_adapter)
{
   uint16_t             safe_channels[NUM_20MHZ_RF_CHANNELS];
   uint16_t             safe_channel_count;
   uint16_t             unsafe_channel_count;
   uint8_t              is_unsafe = 1;
   uint16_t             i;
   uint16_t             channel_loop;

   if (!hdd_ctxt || !ap_adapter) {
      hddLog(LOGE, "%s : Invalid arguments: hdd_ctxt=%pK, ap_adapter=%pK",
             __func__, hdd_ctxt, ap_adapter);
      return 0;
   }

   safe_channel_count = 0;
   unsafe_channel_count = VOS_MIN((uint16_t)hdd_ctxt->unsafe_channel_count,
                              (uint16_t)NUM_20MHZ_RF_CHANNELS);

   for (i = 0; i < NUM_20MHZ_RF_CHANNELS; i++) {
      is_unsafe = 0;
      for (channel_loop = 0;
           channel_loop < unsafe_channel_count; channel_loop++) {
         if (rfChannels[i].channelNum ==
             hdd_ctxt->unsafe_channel_list[channel_loop]) {
            is_unsafe = 1;
            break;
         }
      }
      if (!is_unsafe) {
         safe_channels[safe_channel_count] = rfChannels[i].channelNum;
         VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
             "safe channel %d", safe_channels[safe_channel_count]);
         safe_channel_count++;
      }
  }

   VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
             "perferred range %d - %d",
             ap_adapter->sessionCtx.ap.sapConfig.acs_cfg.start_ch,
             ap_adapter->sessionCtx.ap.sapConfig.acs_cfg.end_ch);
   for (i = 0; i < safe_channel_count; i++) {
      if ((safe_channels[i] >=
                    ap_adapter->sessionCtx.ap.sapConfig.acs_cfg.start_ch) &&
          (safe_channels[i] <=
                    ap_adapter->sessionCtx.ap.sapConfig.acs_cfg.end_ch)) {
         VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
             "safe channel %d is in perferred range", safe_channels[i]);
         return 1;
      }
   }

   return 0;
}

/**
 * hdd_unsafe_channel_restart_sap - restart sap if sap is on unsafe channel
 * @hdd_ctx: hdd context pointer
 *
 * hdd_unsafe_channel_restart_sap check all unsafe channel list
 * and if ACS is enabled, driver will ask userspace to restart the
 * sap. User space on LTE coex indication restart driver.
 *
 * Return - none
 */
void hdd_unsafe_channel_restart_sap(hdd_context_t *hdd_ctx)
{
	hdd_adapter_list_node_t *adapter_node = NULL, *next = NULL;
	eRfChannels channel_loop;
	hdd_adapter_t *adapter;
	VOS_STATUS status;

	status = hdd_get_front_adapter(hdd_ctx, &adapter_node);
	while (NULL != adapter_node && VOS_STATUS_SUCCESS == status) {
		adapter = adapter_node->pAdapter;

		if (!(adapter && (WLAN_HDD_SOFTAP == adapter->device_mode))) {
			status = hdd_get_next_adapter(hdd_ctx, adapter_node,
					&next);
			adapter_node = next;
			continue;
		}
		/*
		 * If auto channel select is enabled
		 * preferred channel is in safe channel,
		 * re-start softap interface with safe channel.
		 * no overlap with preferred channel and safe channel
		 * do not re-start softap interface
		 * stay current operating channel.
		 */
		if ((adapter->sessionCtx.ap.sapConfig.acs_cfg.acs_mode) &&
				(!hdd_find_prefd_safe_chnl(hdd_ctx, adapter)))
			return;

		hddLog(LOG1, FL("Current operation channel %d"),
			adapter->sessionCtx.ap.operatingChannel);

		for (channel_loop = 0;
			channel_loop < hdd_ctx->unsafe_channel_count;
			channel_loop++) {
			if (((hdd_ctx->unsafe_channel_list[channel_loop] ==
				adapter->sessionCtx.ap.operatingChannel)) &&
				(false == hdd_ctx->is_ch_avoid_in_progress) &&
				(adapter->sessionCtx.ap.sapConfig.acs_cfg.
				 acs_mode == true)) {
				hdd_change_ch_avoidance_status(hdd_ctx, true);

				vos_flush_work(
					&hdd_ctx->sap_start_work);

				/*
				 * current operating channel
				 * is un-safe channel, restart driver
				 */
				hddLog(LOGE,
					FL("Restarting SAP due to unsafe channel"));

				/* SAP restart due to unsafe channel. While
				 * restarting the SAP, makee sure to clear
				 * acs_channel, channel to reset to 0.
				 * Otherwise these settings will override the
				 * ACS while restart.
				 */
				hdd_ctx->acs_policy.acs_channel =
							AUTO_CHANNEL_SELECT;
				adapter->sessionCtx.ap.sapConfig.channel =
							AUTO_CHANNEL_SELECT;

				hddLog(LOG1, FL("set sapConfig.channel to %d"),
						AUTO_CHANNEL_SELECT);

				wlan_hdd_send_svc_nlink_msg(
						hdd_ctx->radio_index,
						WLAN_SVC_LTE_COEX_IND,
						NULL,
						0);

				hddLog(LOG1, FL("driver to start sap: %d"),
					hdd_ctx->cfg_ini->sap_internal_restart);
				if (hdd_ctx->cfg_ini->sap_internal_restart) {
					wlan_hdd_netif_queue_control(adapter,
							WLAN_NETIF_TX_DISABLE,
							WLAN_CONTROL_PATH);
					schedule_work(
					&hdd_ctx->sap_start_work);
				}
				else
					hdd_hostapd_stop(adapter->dev);

				return;
			}
		}
		status = hdd_get_next_adapter(hdd_ctx, adapter_node, &next);
		adapter_node = next;
	}
	return;
}

/**---------------------------------------------------------------------------

  \brief hdd_ch_avoid_cb() -

  Avoid channel notification from FW handler.
  FW will send un-safe channel list to avoid over wrapping.
  hostapd should not use notified channel

  \param  - pAdapter HDD adapter pointer
            indParam channel avoid notification parameter

  \return - None

  --------------------------------------------------------------------------*/
void hdd_ch_avoid_cb
(
   void *hdd_context,
   void *indi_param
)
{
   hdd_context_t      *hdd_ctxt;
   tSirChAvoidIndType *ch_avoid_indi;
   v_U8_t              range_loop;
   eRfChannels         channel_loop, start_channel_idx = INVALID_RF_CHANNEL,
                                     end_channel_idx = INVALID_RF_CHANNEL;
   v_U16_t             start_channel;
   v_U16_t             end_channel;
   v_CONTEXT_t         vos_context;
   tHddAvoidFreqList   hdd_avoid_freq_list;
   tANI_U32            i;

   /* Basic sanity */
   if (!hdd_context || !indi_param)
   {
      VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "%s : Invalid arguments", __func__);
      return;
   }

   hdd_ctxt    = (hdd_context_t *)hdd_context;
   ch_avoid_indi  = (tSirChAvoidIndType *)indi_param;
   vos_context = hdd_ctxt->pvosContext;

   /* Make unsafe channel list */
   VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
             "%s : band count %d",
             __func__, ch_avoid_indi->avoid_range_count);

   /* generate vendor specific event */
   vos_mem_zero((void *)&hdd_avoid_freq_list, sizeof(tHddAvoidFreqList));
   for (i = 0; i < ch_avoid_indi->avoid_range_count; i++)
   {
      hdd_avoid_freq_list.avoidFreqRange[i].startFreq =
            ch_avoid_indi->avoid_freq_range[i].start_freq;
      hdd_avoid_freq_list.avoidFreqRange[i].endFreq =
            ch_avoid_indi->avoid_freq_range[i].end_freq;
   }
   hdd_avoid_freq_list.avoidFreqRangeCount = ch_avoid_indi->avoid_range_count;


   /* clear existing unsafe channel cache */
   hdd_ctxt->unsafe_channel_count = 0;
   vos_mem_zero(hdd_ctxt->unsafe_channel_list,
                                        sizeof(hdd_ctxt->unsafe_channel_list));

   for (range_loop = 0; range_loop < ch_avoid_indi->avoid_range_count;
                                                             range_loop++) {
       if (hdd_ctxt->unsafe_channel_count >= NUM_20MHZ_RF_CHANNELS) {
           hddLog(LOGW, FL("LTE Coex unsafe channel list full"));
           break;
       }

       start_channel = ieee80211_frequency_to_channel(
                        ch_avoid_indi->avoid_freq_range[range_loop].start_freq);
       end_channel   = ieee80211_frequency_to_channel(
                          ch_avoid_indi->avoid_freq_range[range_loop].end_freq);
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                    "%s : start %d : %d, end %d : %d", __func__,
                    ch_avoid_indi->avoid_freq_range[range_loop].start_freq,
                    start_channel,
                    ch_avoid_indi->avoid_freq_range[range_loop].end_freq,
                    end_channel);

       /* do not process frequency bands that are not mapped to predefined
        * channels
        */
       if (start_channel == 0 || end_channel == 0)
           continue;

       for (channel_loop = MIN_20MHZ_RF_CHANNEL; channel_loop <=
                                       MAX_20MHZ_RF_CHANNEL; channel_loop++) {
           if (rfChannels[channel_loop].targetFreq >=
                       ch_avoid_indi->avoid_freq_range[range_loop].start_freq) {
               start_channel_idx = channel_loop;
               break;
            }
       }
       for (channel_loop = MIN_20MHZ_RF_CHANNEL; channel_loop <=
                                       MAX_20MHZ_RF_CHANNEL; channel_loop++) {
           if (rfChannels[channel_loop].targetFreq >=
                       ch_avoid_indi->avoid_freq_range[range_loop].end_freq) {
               end_channel_idx = channel_loop;
               if (rfChannels[channel_loop].targetFreq >
                   ch_avoid_indi->avoid_freq_range[range_loop].end_freq &&
                   end_channel_idx > 0)
                   end_channel_idx--;
               break;
            }
       }

       if (start_channel_idx == INVALID_RF_CHANNEL ||
                                         end_channel_idx == INVALID_RF_CHANNEL)
           continue;

       for (channel_loop = start_channel_idx; channel_loop <=
                                              end_channel_idx; channel_loop++) {
           hdd_ctxt->unsafe_channel_list[hdd_ctxt->unsafe_channel_count++]
                                      = rfChannels[channel_loop].channelNum;
           if (hdd_ctxt->unsafe_channel_count >= NUM_20MHZ_RF_CHANNELS) {
                hddLog(LOGW, FL("LTE Coex unsafe channel list full"));
                break;
           }
       }
   }

#ifdef CONFIG_CNSS
   VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
            "%s : number of unsafe channels is %d ",
            __func__,  hdd_ctxt->unsafe_channel_count);

   if (vos_set_wlan_unsafe_channel(hdd_ctxt->unsafe_channel_list,
                                hdd_ctxt->unsafe_channel_count)) {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "%s: Failed to set unsafe channel",
                __func__);

       /* clear existing unsafe channel cache */
       hdd_ctxt->unsafe_channel_count = 0;
       vos_mem_zero(hdd_ctxt->unsafe_channel_list,
           sizeof(v_U16_t) * NUM_20MHZ_RF_CHANNELS);

       return;
   }

   for (channel_loop = 0;
        channel_loop < hdd_ctxt->unsafe_channel_count;
        channel_loop++)
   {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                 "%s: channel %d is not safe ", __func__,
                 hdd_ctxt->unsafe_channel_list[channel_loop]);
   }
#endif

   /*
    * first update the unsafe channel list to the platform driver and
    * send the avoid freq event to the application
    */
   wlan_hdd_send_avoid_freq_event(hdd_ctxt, &hdd_avoid_freq_list);

   if (0 == hdd_ctxt->unsafe_channel_count)
       return;
   hdd_unsafe_channel_restart_sap(hdd_ctxt);
   return;
}
#endif /* FEATURE_WLAN_CH_AVOID */

#ifdef WLAN_FEATURE_LPSS
int wlan_hdd_gen_wlan_status_pack(struct wlan_status_data *data,
                                  hdd_adapter_t *pAdapter,
                                  hdd_station_ctx_t *pHddStaCtx,
                                  v_U8_t is_on,
                                  v_U8_t is_connected)
{
    hdd_context_t *pHddCtx = NULL;
    tANI_U8 buflen = WLAN_SVC_COUNTRY_CODE_LEN;

    if (!data) {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: invalid data pointer", __func__);
        return (-1);
    }
    if (!pAdapter) {
        if (is_on) {
            /* no active interface */
            data->lpss_support = 0;
            data->is_on = is_on;
            return 0;
        }
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: invalid adapter pointer", __func__);
        return (-1);
    }

    pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
    if (pHddCtx->lpss_support && pHddCtx->cfg_ini->enablelpasssupport)
        data->lpss_support = 1;
    else
        data->lpss_support = 0;
    data->numChannels = WLAN_SVC_MAX_NUM_CHAN;
    sme_GetCfgValidChannels(pHddCtx->hHal, data->channel_list,
                            &data->numChannels);
    sme_GetCountryCode(pHddCtx->hHal, data->country_code, &buflen);
    data->is_on = is_on;
    data->vdev_id = pAdapter->sessionId;
    data->vdev_mode = pAdapter->device_mode;
    if (pHddStaCtx) {
        data->is_connected = is_connected;
        data->rssi = pAdapter->rssi;
        data->freq = vos_chan_to_freq(pHddStaCtx->conn_info.operationChannel);
        if (WLAN_SVC_MAX_SSID_LEN >= pHddStaCtx->conn_info.SSID.SSID.length) {
            data->ssid_len = pHddStaCtx->conn_info.SSID.SSID.length;
            memcpy(data->ssid,
                   pHddStaCtx->conn_info.SSID.SSID.ssId,
                   pHddStaCtx->conn_info.SSID.SSID.length);
        }
        if (WLAN_SVC_MAX_BSSID_LEN >= sizeof(pHddStaCtx->conn_info.bssId))
            memcpy(data->bssid,
                   pHddStaCtx->conn_info.bssId,
                   sizeof(pHddStaCtx->conn_info.bssId));
    }
    return 0;
}

int wlan_hdd_gen_wlan_version_pack(struct wlan_version_data *data,
                                    v_U32_t fw_version,
                                    v_U32_t chip_id,
                                    const char *chip_name)
{
    if (!data) {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: invalid data pointer", __func__);
        return (-1);
    }

    data->chip_id = chip_id;
    strlcpy(data->chip_name, chip_name, WLAN_SVC_MAX_STR_LEN);
    if (strncmp(chip_name, "Unknown", 7))
        strlcpy(data->chip_from, "Qualcomm", WLAN_SVC_MAX_STR_LEN);
    else
        strlcpy(data->chip_from, "Unknown", WLAN_SVC_MAX_STR_LEN);
    strlcpy(data->host_version, QWLAN_VERSIONSTR, WLAN_SVC_MAX_STR_LEN);
    scnprintf(data->fw_version, WLAN_SVC_MAX_STR_LEN, "%d.%d.%d.%d",
              (fw_version & 0xf0000000) >> 28,
              (fw_version & 0xf000000) >> 24,
              (fw_version & 0xf00000) >> 20,
              (fw_version & 0x7fff));
    return 0;
}
#endif

#if defined(FEATURE_WLAN_LFR) && defined(WLAN_FEATURE_ROAM_SCAN_OFFLOAD)
/**---------------------------------------------------------------------------

  \brief wlan_hdd_disable_roaming()

  This function loop through each adapter and disable roaming on each STA
  device mode except the input adapter.
  Note: On the input adapter roaming is not enabled yet hence no need to
        disable.

  \param  - pAdapter HDD adapter pointer

  \return - None

  --------------------------------------------------------------------------*/
void wlan_hdd_disable_roaming(hdd_adapter_t *pAdapter)
{
    hdd_context_t           *pHddCtx      = WLAN_HDD_GET_CTX(pAdapter);
    hdd_adapter_t           *pAdapterIdx  = NULL;
    hdd_adapter_list_node_t *pAdapterNode = NULL;
    hdd_adapter_list_node_t *pNext        = NULL;
    VOS_STATUS status;

    if (pHddCtx->cfg_ini->isFastRoamIniFeatureEnabled &&
        pHddCtx->cfg_ini->isRoamOffloadScanEnabled &&
        WLAN_HDD_INFRA_STATION == pAdapter->device_mode &&
        vos_is_sta_active_connection_exists()) {
        hddLog(LOG1, FL("Connect received on STA sessionId(%d)"),
               pAdapter->sessionId);
        /* Loop through adapter and disable roaming for each STA device mode
           except the input adapter. */

        status = hdd_get_front_adapter (pHddCtx, &pAdapterNode);

        while (NULL != pAdapterNode && VOS_STATUS_SUCCESS == status) {
            pAdapterIdx = pAdapterNode->pAdapter;

            if (WLAN_HDD_INFRA_STATION == pAdapterIdx->device_mode &&
               pAdapter->sessionId != pAdapterIdx->sessionId) {
               hddLog(LOG1, FL("Disable Roaming on sessionId(%d)"),
                      pAdapterIdx->sessionId);
               sme_stopRoaming(WLAN_HDD_GET_HAL_CTX(pAdapterIdx),
                               pAdapterIdx->sessionId, 0);
            }

            status = hdd_get_next_adapter(pHddCtx, pAdapterNode, &pNext);
            pAdapterNode = pNext;
        }
    }
}

/**---------------------------------------------------------------------------

  \brief wlan_hdd_enable_roaming()

  This function loop through each adapter and enable roaming on each STA
  device mode except the input adapter.
  Note: On the input adapter no need to enable roaming because link got
        disconnected on this.

  \param  - pAdapter HDD adapter pointer

  \return - None

  --------------------------------------------------------------------------*/
void wlan_hdd_enable_roaming(hdd_adapter_t *pAdapter)
{
    hdd_context_t           *pHddCtx      = WLAN_HDD_GET_CTX(pAdapter);
    hdd_adapter_t           *pAdapterIdx  = NULL;
    hdd_adapter_list_node_t *pAdapterNode = NULL;
    hdd_adapter_list_node_t *pNext        = NULL;
    VOS_STATUS status;

    if (pHddCtx->cfg_ini->isFastRoamIniFeatureEnabled &&
        pHddCtx->cfg_ini->isRoamOffloadScanEnabled &&
        WLAN_HDD_INFRA_STATION == pAdapter->device_mode &&
        vos_is_sta_active_connection_exists()) {
        hddLog(LOG1, FL("Disconnect received on STA sessionId(%d)"),
               pAdapter->sessionId);
        /* Loop through adapter and enable roaming for each STA device mode
           except the input adapter. */

        status = hdd_get_front_adapter (pHddCtx, &pAdapterNode);

        while (NULL != pAdapterNode && VOS_STATUS_SUCCESS == status) {
            pAdapterIdx = pAdapterNode->pAdapter;

            if (WLAN_HDD_INFRA_STATION == pAdapterIdx->device_mode &&
               pAdapter->sessionId != pAdapterIdx->sessionId) {
               hddLog(LOG1, FL("Enabling Roaming on sessionId(%d)"),
                      pAdapterIdx->sessionId);
               sme_startRoaming(WLAN_HDD_GET_HAL_CTX(pAdapterIdx),
                               pAdapterIdx->sessionId,
                               REASON_CONNECT);
            }

            status = hdd_get_next_adapter(pHddCtx, pAdapterNode, &pNext);
            pAdapterNode = pNext;
        }
    }
}
#endif

/**
 * nl_srv_bcast_svc() - Wrapper function to send bcast msgs to SVC mcast group
 * @skb: sk buffer pointer
 *
 * Sends the bcast message to SVC multicast group with generic nl socket
 * if CNSS_GENL is enabled. Else, use the legacy netlink socket to send.
 *
 * Return: None
 */
static void nl_srv_bcast_svc(struct sk_buff *skb)
{
#ifdef CNSS_GENL
	nl_srv_bcast(skb, CLD80211_MCGRP_SVC_MSGS, WLAN_NL_MSG_SVC);
#else
	nl_srv_bcast(skb);
#endif
}

void wlan_hdd_send_svc_nlink_msg(int radio, int type, void *data, int len)
{
    struct sk_buff *skb;
    struct nlmsghdr *nlh;
    tAniMsgHdr *ani_hdr;
    void *nl_data = NULL;
    int flags = GFP_KERNEL;
    struct radio_index_tlv *radio_info;
    int tlv_len;


    if (in_interrupt() || irqs_disabled() || in_atomic())
        flags = GFP_ATOMIC;

    skb = alloc_skb(NLMSG_SPACE(WLAN_NL_MAX_PAYLOAD), flags);

    if(skb == NULL) {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "%s: alloc_skb failed", __func__);
        return;
    }

    nlh = (struct nlmsghdr *)skb->data;
    nlh->nlmsg_pid = 0;  /* from kernel */
    nlh->nlmsg_flags = 0;
    nlh->nlmsg_seq = 0;
    nlh->nlmsg_type = WLAN_NL_MSG_SVC;

    ani_hdr = NLMSG_DATA(nlh);
    ani_hdr->type = type;

    switch(type) {
        case WLAN_SVC_FW_CRASHED_IND:
        case WLAN_SVC_FW_SHUTDOWN_IND:
        case WLAN_SVC_LTE_COEX_IND:
        case WLAN_SVC_WLAN_AUTO_SHUTDOWN_IND:
        case WLAN_SVC_WLAN_AUTO_SHUTDOWN_CANCEL_IND:
            ani_hdr->length = 0;
            nlh->nlmsg_len = NLMSG_LENGTH((sizeof(tAniMsgHdr)));
            break;
        case WLAN_SVC_WLAN_STATUS_IND:
        case WLAN_SVC_WLAN_VERSION_IND:
        case WLAN_SVC_DFS_CAC_START_IND:
        case WLAN_SVC_DFS_CAC_END_IND:
        case WLAN_SVC_DFS_RADAR_DETECT_IND:
        case WLAN_SVC_DFS_ALL_CHANNEL_UNAVAIL_IND:
        case WLAN_SVC_WLAN_TP_IND:
        case WLAN_SVC_WLAN_TP_TX_IND:
        case WLAN_SVC_RPS_ENABLE_IND:
            ani_hdr->length = len;
            nlh->nlmsg_len = NLMSG_LENGTH((sizeof(tAniMsgHdr) + len));
            nl_data = (char *)ani_hdr + sizeof(tAniMsgHdr);
            memcpy(nl_data, data, len);
            break;
        default:
            VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                    "WLAN SVC: Attempt to send unknown nlink message %d", type);
            kfree_skb(skb);
            return;
    }

    /*
     * Add radio index at the end of the svc event in TLV format to maintain
     * the backward compatibilty with userspace applications.
     */

    tlv_len = 0;

    if ((sizeof(*ani_hdr) + len + sizeof(struct radio_index_tlv))
            < WLAN_NL_MAX_PAYLOAD) {
        radio_info  = (struct radio_index_tlv *)((char *) ani_hdr +
                sizeof (*ani_hdr) + len);
        radio_info->type = (unsigned short) WLAN_SVC_WLAN_RADIO_INDEX;
        radio_info->length = (unsigned short) sizeof(radio_info->radio);
        radio_info->radio = radio;
        tlv_len = sizeof(*radio_info);
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                "Added radio index tlv - radio index %d", radio_info->radio);
    }

    nlh->nlmsg_len += tlv_len;
    skb_put(skb, NLMSG_SPACE(sizeof(tAniMsgHdr) + len + tlv_len));

    nl_srv_bcast_svc(skb);

    return;
}

#ifdef WLAN_FEATURE_LPSS
void wlan_hdd_send_status_pkg(hdd_adapter_t *pAdapter,
                              hdd_station_ctx_t *pHddStaCtx,
                              v_U8_t is_on,
                              v_U8_t is_connected)
{
    int ret = 0;
    struct wlan_status_data data;
    hdd_context_t *hdd_ctx;
    v_PVOID_t vos_ctx;

    if (VOS_FTM_MODE == hdd_get_conparam())
        return;

    memset(&data, 0, sizeof(struct wlan_status_data));
    if (is_on)
        ret = wlan_hdd_gen_wlan_status_pack(&data, pAdapter, pHddStaCtx,
                                            is_on, is_connected);
    if (!ret) {
        if (pAdapter) {
            hdd_ctx = WLAN_HDD_GET_CTX(pAdapter);
        } else {
            vos_ctx = vos_get_global_context(VOS_MODULE_ID_HDD, NULL);
            hdd_ctx = vos_get_context(VOS_MODULE_ID_HDD, vos_ctx);
        }

        if (!hdd_ctx)
            return;

        wlan_hdd_send_svc_nlink_msg(hdd_ctx->radio_index,
                WLAN_SVC_WLAN_STATUS_IND,
                &data, sizeof(struct wlan_status_data));
    }
}

void wlan_hdd_send_version_pkg(v_U32_t fw_version,
                               v_U32_t chip_id,
                               const char *chip_name)
{
    int ret = 0;
    struct wlan_version_data data;
    v_PVOID_t vos_ctx = vos_get_global_context(VOS_MODULE_ID_HDD, NULL);
    hdd_context_t *hdd_ctx = vos_get_context(VOS_MODULE_ID_HDD, vos_ctx);

    if (VOS_FTM_MODE == hdd_get_conparam())
        return;

    if (!hdd_ctx)
        return;

    memset(&data, 0, sizeof(struct wlan_version_data));
    ret = wlan_hdd_gen_wlan_version_pack(&data, fw_version, chip_id,
                                         chip_name);
    if (!ret)
        wlan_hdd_send_svc_nlink_msg(hdd_ctx->radio_index,
                                    WLAN_SVC_WLAN_VERSION_IND,
                                    &data, sizeof(struct wlan_version_data));
}

void wlan_hdd_send_all_scan_intf_info(hdd_context_t *pHddCtx)
{
    hdd_adapter_t *pDataAdapter = NULL;
    hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
    v_BOOL_t scan_intf_found = VOS_FALSE;
    VOS_STATUS status;

    if (!pHddCtx) {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: NULL pointer for pHddCtx",
                  __func__);
        return;
    }

   status = hdd_get_front_adapter(pHddCtx, &pAdapterNode);
   while (NULL != pAdapterNode && VOS_STATUS_SUCCESS == status) {
       pDataAdapter = pAdapterNode->pAdapter;
       if (pDataAdapter) {
           if (pDataAdapter->device_mode == WLAN_HDD_INFRA_STATION ||
               pDataAdapter->device_mode == WLAN_HDD_P2P_CLIENT ||
               pDataAdapter->device_mode == WLAN_HDD_P2P_DEVICE) {
               scan_intf_found = VOS_TRUE;
               wlan_hdd_send_status_pkg(pDataAdapter, NULL, 1, 0);
           }
       }
       status = hdd_get_next_adapter(pHddCtx, pAdapterNode, &pNext);
       pAdapterNode = pNext;
   }

   if (!scan_intf_found)
       wlan_hdd_send_status_pkg(pDataAdapter, NULL, 1, 0);
}
#endif

#ifdef FEATURE_WLAN_AUTO_SHUTDOWN
v_VOID_t wlan_hdd_auto_shutdown_cb(v_VOID_t)
{
    v_PVOID_t vos_ctx = vos_get_global_context(VOS_MODULE_ID_HDD, NULL);
    hdd_context_t *hdd_ctx = vos_get_context(VOS_MODULE_ID_HDD, vos_ctx);

    if (!hdd_ctx)
        return;

    hddLog(LOGE, FL("%s: Wlan Idle. Sending Shutdown event.."),__func__);
    wlan_hdd_send_svc_nlink_msg(hdd_ctx->radio_index,
                                WLAN_SVC_WLAN_AUTO_SHUTDOWN_IND, NULL, 0);
}

void wlan_hdd_auto_shutdown_enable(hdd_context_t *hdd_ctx, v_BOOL_t enable)
{
    hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
    VOS_STATUS status;
    hdd_adapter_t      *pAdapter;
    v_BOOL_t ap_connected = VOS_FALSE, sta_connected = VOS_FALSE;
    tHalHandle hHal;

    hHal = hdd_ctx->hHal;
    if (hHal == NULL)
        return;

    if (hdd_ctx->cfg_ini->WlanAutoShutdown == 0)
        return;

    if (enable == VOS_FALSE) {
        if (sme_set_auto_shutdown_timer(hHal, 0) != eHAL_STATUS_SUCCESS) {
               hddLog(LOGE, FL("Failed to stop wlan auto shutdown timer"));
        }
        wlan_hdd_send_svc_nlink_msg(hdd_ctx->radio_index,
                        WLAN_SVC_WLAN_AUTO_SHUTDOWN_CANCEL_IND, NULL, 0);
        return;
    }

    /* To enable shutdown timer check concurrency */
    if (vos_concurrent_open_sessions_running()) {
        status = hdd_get_front_adapter ( hdd_ctx, &pAdapterNode );

        while ( NULL != pAdapterNode && VOS_STATUS_SUCCESS == status ) {
            pAdapter = pAdapterNode->pAdapter;
            if (pAdapter && pAdapter->device_mode == WLAN_HDD_INFRA_STATION) {
                if (WLAN_HDD_GET_STATION_CTX_PTR(pAdapter)->conn_info.connState
                                               == eConnectionState_Associated) {
                    sta_connected = VOS_TRUE;
                    break;
                }
            }
            if (pAdapter && pAdapter->device_mode == WLAN_HDD_SOFTAP) {
                if(WLAN_HDD_GET_AP_CTX_PTR(pAdapter)->bApActive == VOS_TRUE) {
                    ap_connected = VOS_TRUE;
                    break;
                }
            }
            status = hdd_get_next_adapter ( hdd_ctx, pAdapterNode, &pNext );
            pAdapterNode = pNext;
        }
    }

    if (ap_connected == VOS_TRUE || sta_connected == VOS_TRUE) {
            hddLog(LOG1, FL("CC Session active. Shutdown timer not enabled"));
            return;
    } else {
        if (sme_set_auto_shutdown_timer(hHal,
                hdd_ctx->cfg_ini->WlanAutoShutdown)
                != eHAL_STATUS_SUCCESS)
           hddLog(LOGE, FL("Failed to start wlan auto shutdown timer"));
        else
           hddLog(LOG1, FL("Auto Shutdown timer for %d seconds enabled"),
                                   hdd_ctx->cfg_ini->WlanAutoShutdown);

    }
}
#endif

hdd_adapter_t * hdd_get_con_sap_adapter(hdd_adapter_t *this_sap_adapter,
                                        bool check_start_bss)
{
    hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(this_sap_adapter);
    hdd_adapter_t *pAdapter, *con_sap_adapter;
    VOS_STATUS status = VOS_STATUS_SUCCESS;
    hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;

    con_sap_adapter = NULL;

    status = hdd_get_front_adapter ( pHddCtx, &pAdapterNode );
    while ( NULL != pAdapterNode && VOS_STATUS_SUCCESS == status ) {
        pAdapter = pAdapterNode->pAdapter;
        if (pAdapter && ((pAdapter->device_mode == WLAN_HDD_SOFTAP) ||
            (pAdapter->device_mode == WLAN_HDD_P2P_GO)) &&
            pAdapter != this_sap_adapter) {
            if (check_start_bss) {
                if (test_bit(SOFTAP_BSS_STARTED, &pAdapter->event_flags)) {
                    con_sap_adapter = pAdapter;
                    break;
                }
            } else {
                    con_sap_adapter = pAdapter;
                    break;
            }
        }
        status = hdd_get_next_adapter(pHddCtx, pAdapterNode, &pNext );
        pAdapterNode = pNext;
    }

    return con_sap_adapter;
}

#ifdef FEATURE_BUS_BANDWIDTH
void hdd_start_bus_bw_compute_timer(hdd_adapter_t *pAdapter)
{
    hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);

    if (VOS_TIMER_STATE_RUNNING ==
        vos_timer_getCurrentState(&pHddCtx->bus_bw_timer))
        return;

    vos_timer_start(&pHddCtx->bus_bw_timer,
            pHddCtx->cfg_ini->busBandwidthComputeInterval);
}

void hdd_stop_bus_bw_compute_timer(hdd_adapter_t *pAdapter)
{
    hdd_adapter_list_node_t *pAdapterNode = NULL, *pNext = NULL;
    VOS_STATUS status;
    v_BOOL_t can_stop = VOS_TRUE;
    hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);

    if (VOS_TIMER_STATE_RUNNING !=
        vos_timer_getCurrentState(&pHddCtx->bus_bw_timer)) {
        /* trying to stop timer, when not running is not good */
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                  "bus band width compute timer is not running");
        return;
    }

    if (vos_concurrent_open_sessions_running()) {
        status = hdd_get_front_adapter ( pHddCtx, &pAdapterNode );

        while ( NULL != pAdapterNode && VOS_STATUS_SUCCESS == status ) {
            pAdapter = pAdapterNode->pAdapter;
            if (pAdapter && (pAdapter->device_mode == WLAN_HDD_INFRA_STATION ||
                        pAdapter->device_mode == WLAN_HDD_P2P_CLIENT) &&
                    WLAN_HDD_GET_STATION_CTX_PTR(pAdapter)->conn_info.connState
                    == eConnectionState_Associated) {
                can_stop = VOS_FALSE;
                break;
            }
            if (pAdapter && (pAdapter->device_mode == WLAN_HDD_SOFTAP ||
                        pAdapter->device_mode == WLAN_HDD_P2P_GO) &&
                    WLAN_HDD_GET_AP_CTX_PTR(pAdapter)->bApActive == VOS_TRUE) {
                can_stop = VOS_FALSE;
                break;
            }
            status = hdd_get_next_adapter ( pHddCtx, pAdapterNode, &pNext );
            pAdapterNode = pNext;
        }
    }

    if (can_stop == VOS_TRUE) {
        vos_timer_stop(&pHddCtx->bus_bw_timer);
        if (pHddCtx->hbw_requested) {
            vos_remove_pm_qos();
            pHddCtx->hbw_requested = false;
        }
        /* reset the ipa perf level */
        hdd_ipa_set_perf_level(pHddCtx, 0, 0);
        hdd_rst_tcp_delack(pHddCtx);
        tlshim_reset_bundle_require();
        tlshim_driver_del_ack_disable();
    }
}
#endif


#ifdef FEATURE_WLAN_MCC_TO_SCC_SWITCH
void wlan_hdd_check_sta_ap_concurrent_ch_intf(void *data)
{
    hdd_adapter_t *ap_adapter = NULL, *sta_adapter = (hdd_adapter_t *)data;
    hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(sta_adapter);
    tHalHandle hHal;
    hdd_ap_ctx_t *pHddApCtx;
    uint16_t intf_ch = 0, vht_channel_width = 0;
    eCsrBand orig_band, new_band;
    uint16_t ch_width;
    hdd_adapter_list_node_t *adapter_node = NULL, *next = NULL;
    VOS_STATUS status;

   if ((pHddCtx->cfg_ini->WlanMccToSccSwitchMode == VOS_MCC_TO_SCC_SWITCH_DISABLE)
       || !(vos_concurrent_open_sessions_running()
       || !(vos_get_concurrency_mode() == VOS_STA_SAP)))
        return;

    ap_adapter = hdd_get_adapter(pHddCtx, WLAN_HDD_SOFTAP);
    if (ap_adapter == NULL)
        return;

    if (!test_bit(SOFTAP_BSS_STARTED, &ap_adapter->event_flags))
        return;

    pHddApCtx = WLAN_HDD_GET_AP_CTX_PTR(ap_adapter);
    hHal = WLAN_HDD_GET_HAL_CTX(ap_adapter);

    if (hHal == NULL)
        return;

    intf_ch = WLANSAP_CheckCCIntf(pHddApCtx->sapContext);
    vht_channel_width = wlan_sap_get_vht_ch_width(pHddApCtx->sapContext);
    if (intf_ch == 0)
        return;

    if (pHddApCtx->sapConfig.band_switch_enable) {
        if (pHddApCtx->sapConfig.channel > MAX_2_4GHZ_CHANNEL) {
            orig_band = eCSR_BAND_5G;
        } else {
            orig_band = eCSR_BAND_24;
        }

        if (intf_ch > MAX_2_4GHZ_CHANNEL) {
            new_band = eCSR_BAND_5G;
        } else {
            new_band = eCSR_BAND_24;
        }

        if (orig_band != new_band) {
            if (new_band == eCSR_BAND_5G) {
                pHddApCtx->sapConfig.ch_width_orig =
                    pHddApCtx->sapConfig.ch_width_5g_orig;
            } else {
                pHddApCtx->sapConfig.ch_width_orig =
                    pHddApCtx->sapConfig.ch_width_24g_orig;
            }
        }
    }
    ch_width = pHddApCtx->sapConfig.ch_width_orig;

    hddLog(VOS_TRACE_LEVEL_INFO,
        FL("SAP restarts due to MCC->SCC switch, orig chan: %d, new chan: %d"),
        pHddApCtx->sapConfig.channel, intf_ch);

    pHddApCtx->sapConfig.channel = intf_ch;
#ifdef WLAN_FEATURE_SAP_TO_FOLLOW_STA_CHAN
    if(vos_is_ch_switch_with_csa_enabled())
    {
        struct wlan_sap_csa_info csa_info;

        csa_info.sta_channel = intf_ch;

        hddLog(VOS_TRACE_LEVEL_INFO_HIGH,
	    "%s: Indicate connected event to HostApd Chan=%d",
	    __func__, csa_info.sta_channel);

	/*Indicate to HostApd about Station interface state change*/
        hdd_sta_state_sap_notify(pHddCtx, STA_NOTIFY_CONNECTED, csa_info);
    }else{
#endif
    status =  hdd_get_front_adapter (pHddCtx, &adapter_node);
    while (NULL != adapter_node && VOS_STATUS_SUCCESS == status) {
        ap_adapter = adapter_node->pAdapter;
        if (ap_adapter && ap_adapter->device_mode == WLAN_HDD_SOFTAP) {
            if (test_bit(SOFTAP_INIT_DONE, &ap_adapter->event_flags)) {
                pHddApCtx = WLAN_HDD_GET_AP_CTX_PTR(ap_adapter);
                hHal = WLAN_HDD_GET_HAL_CTX(ap_adapter);
                pHddApCtx->sapConfig.channel = intf_ch;
                pHddApCtx->bss_stop_reason = BSS_STOP_DUE_TO_MCC_SCC_SWITCH;
                sme_SelectCBMode(hHal,
                             pHddApCtx->sapConfig.SapHw_mode,
                             pHddApCtx->sapConfig.channel,
                             pHddApCtx->sapConfig.sec_ch,
                             &vht_channel_width, ch_width);
                wlan_sap_set_vht_ch_width(pHddApCtx->sapContext, vht_channel_width);
                hddLog(VOS_TRACE_LEVEL_INFO, FL("Restart prev SAP session "));
                wlan_hdd_restart_sap(ap_adapter);
            }
        }
        status = hdd_get_next_adapter (pHddCtx, adapter_node, &next);
        adapter_node = next;
    }
#ifdef WLAN_FEATURE_SAP_TO_FOLLOW_STA_CHAN
    }
#endif
}
#endif

#ifdef WLAN_FEATURE_SAP_TO_FOLLOW_STA_CHAN

void hdd_csa_notify_cb
(
   void *hdd_context,
   void *indi_param
)
{
   tpSmeCsaOffloadInd csa_params = NULL;
   hdd_context_t      *hdd_ctxt = NULL;
   struct wlan_sap_csa_info csa_info;
   v_U32_t ret = 0;
   /* Basic sanity */

   if(!vos_is_ch_switch_with_csa_enabled())
   {
      VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "%s : SAP channel switch with CSA not enabled", __func__);
      return;
   }

   if (!hdd_context || !indi_param)
   {
      VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "%s : Invalid arguments", __func__);
      return;
   }

   hdd_ctxt    = (hdd_context_t *)hdd_context;

   csa_params = (tpSmeCsaOffloadInd)indi_param;

   VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
             "%s : tbtt count %d Channel = %d",
             __func__,csa_params->tbtt_count, csa_params->channel);
   hdd_ctxt->ch_switch_ctx.tbtt_count = csa_params->tbtt_count - 1;  /* Will reduce the count by 1,
									as the switch might take time.
									Currently of No Use, as this will be
									override by ini g_sap_chanswitch_beacon_cnt */
   csa_info.sta_channel = csa_params->channel;
   ret = hdd_sta_state_sap_notify(hdd_ctxt, STA_NOTIFY_CSA, csa_info);
   if(ret != 0)
   {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
          "%s : Failed to trigger Channel Switch Ch:%d ret=%d",
          __func__,csa_params->channel, ret);
   }
}
#endif//#ifdef WLAN_FEATURE_SAP_TO_FOLLOW_STA_CHAN

/**
 * wlan_hdd_check_custom_con_channel_rules() - This function checks the sap's
 *                                             and sta's operating channel.
 * @sta_adapter:  Describe the first argument to foobar.
 * @ap_adapter:   Describe the second argument to foobar.
 * @roam_profile: Roam profile of AP to which STA wants to connect.
 * @concurrent_chnl_same: If both SAP and STA channels are same then
 *                        set this flag to true else false.
 *
 * This function checks the sap's operating channel and sta's operating channel.
 * if both are same then it will return false else it will restart the sap in
 * sta's channel and return true.
 *
 *
 * Return: VOS_STATUS_SUCCESS or VOS_STATUS_E_FAILURE.
 */
VOS_STATUS wlan_hdd_check_custom_con_channel_rules(hdd_adapter_t *sta_adapter,
                                                  hdd_adapter_t *ap_adapter,
                                                  tCsrRoamProfile *roam_profile,
                                                  tScanResultHandle *scan_cache,
                                                  bool *concurrent_chnl_same)
{
    hdd_ap_ctx_t *hdd_ap_ctx;
    uint8_t channel_id;
    VOS_STATUS status;
    device_mode_t device_mode = ap_adapter->device_mode;
    *concurrent_chnl_same = true;

    hdd_ap_ctx = WLAN_HDD_GET_AP_CTX_PTR(ap_adapter);
    status =
     sme_get_ap_channel_from_scan_cache(WLAN_HDD_GET_HAL_CTX(sta_adapter),
                                        roam_profile,
                                        scan_cache,
                                        &channel_id);
    if ((VOS_STATUS_SUCCESS == status)) {
        if ((WLAN_HDD_SOFTAP == device_mode) &&
            (channel_id < SIR_11A_CHANNEL_BEGIN)) {
             if (hdd_ap_ctx->operatingChannel != channel_id) {
                 *concurrent_chnl_same = false;
                  hddLog(VOS_TRACE_LEVEL_INFO_MED,
                            FL("channels are different"));
             }
        } else if ((WLAN_HDD_P2P_GO == device_mode) &&
                   (channel_id >= SIR_11A_CHANNEL_BEGIN)) {
             if (hdd_ap_ctx->operatingChannel != channel_id) {
                 *concurrent_chnl_same = false;
                 hddLog(VOS_TRACE_LEVEL_INFO_MED,
                           FL("channels are different"));
             }
        }
    } else {
        /*
         * Lets handle worst case scenario here, Scan cache lookup is failed
         * so we have to stop the SAP to avoid any channel discrepancy  between
         * SAP's channel and STA's channel. Return the status as failure so
         * caller function could know that scan look up is failed.
         */
        hddLog(VOS_TRACE_LEVEL_ERROR,
                    FL("Finding AP from scan cache failed"));
        return VOS_STATUS_E_FAILURE;
    }
    return VOS_STATUS_SUCCESS;
}

#ifdef WLAN_FEATURE_MBSSID
/**
 * wlan_hdd_stop_sap() - This function stops bss of SAP.
 * @ap_adapter: SAP adapter
 *
 * This function will process the stopping of sap adapter.
 *
 * Return: void.
 */
void wlan_hdd_stop_sap(hdd_adapter_t *ap_adapter)
{
    hdd_ap_ctx_t *hdd_ap_ctx;
    hdd_hostapd_state_t *hostapd_state;
    VOS_STATUS vos_status;
    hdd_context_t *hdd_ctx;
#ifdef CFG80211_DEL_STA_V2
    struct station_del_parameters delStaParams;
#endif

    if (NULL == ap_adapter) {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                    FL("ap_adapter is NULL here"));
        return;
    }

    hdd_ap_ctx = WLAN_HDD_GET_AP_CTX_PTR(ap_adapter);
    hdd_ctx = WLAN_HDD_GET_CTX(ap_adapter);
    if (0 != wlan_hdd_validate_context(hdd_ctx))
        return;

    mutex_lock(&hdd_ctx->sap_lock);
    if (test_bit(SOFTAP_BSS_STARTED, &ap_adapter->event_flags)) {
#ifdef CFG80211_DEL_STA_V2
        delStaParams.mac = NULL;
        delStaParams.subtype = SIR_MAC_MGMT_DEAUTH >> 4;
        delStaParams.reason_code = eCsrForcedDeauthSta;
        wlan_hdd_cfg80211_del_station(ap_adapter->wdev.wiphy, ap_adapter->dev,
                                      &delStaParams);
#else
        wlan_hdd_cfg80211_del_station(ap_adapter->wdev.wiphy, ap_adapter->dev,
                                      NULL);
#endif
        hdd_cleanup_actionframe(hdd_ctx, ap_adapter);
        hostapd_state = WLAN_HDD_GET_HOSTAP_STATE_PTR(ap_adapter);
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
                  FL("Now doing SAP STOPBSS"));
        if (VOS_STATUS_SUCCESS == WLANSAP_StopBss(hdd_ap_ctx->sapContext)) {
            vos_status = vos_wait_single_event(&hostapd_state->stop_bss_event,
                                               10000);
            if (!VOS_IS_STATUS_SUCCESS(vos_status)) {
                mutex_unlock(&hdd_ctx->sap_lock);
                VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                          FL("SAP Stop Failed"));
                return;
            }
        }
        clear_bit(SOFTAP_BSS_STARTED, &ap_adapter->event_flags);
        wlan_hdd_decr_active_session(hdd_ctx, ap_adapter->device_mode);
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
                  FL("SAP Stop Success"));
        if (hdd_ctx->cfg_ini->apOBSSProtEnabled)
            vos_runtime_pm_allow_suspend(hdd_ctx->runtime_context.obss);
    } else {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  FL("Can't stop ap because its not started"));
    }
    mutex_unlock(&hdd_ctx->sap_lock);
    return;
}

/**
 * wlan_hdd_start_sap() - This function starts bss of SAP.
 * @ap_adapter: SAP adapter
 *
 * This function will process the starting of sap adapter.
 *
 * Return: void.
 */
void wlan_hdd_start_sap(hdd_adapter_t *ap_adapter, bool reinit)
{
    hdd_ap_ctx_t *hdd_ap_ctx;
    hdd_hostapd_state_t *hostapd_state;
    VOS_STATUS vos_status;
    hdd_context_t *hdd_ctx;
    tsap_Config_t *pConfig;

    if (NULL == ap_adapter) {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                    FL("ap_adapter is NULL here"));
        return;
    }

    hdd_ctx = WLAN_HDD_GET_CTX(ap_adapter);
    hdd_ap_ctx = WLAN_HDD_GET_AP_CTX_PTR(ap_adapter);
    hostapd_state = WLAN_HDD_GET_HOSTAP_STATE_PTR(ap_adapter);
    pConfig = &ap_adapter->sessionCtx.ap.sapConfig;

    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
              FL("ssr in progress %d"), reinit);

     if (!reinit) {
        if (0 != wlan_hdd_validate_context(hdd_ctx))
            return;
    }

    mutex_lock(&hdd_ctx->sap_lock);
    if (test_bit(SOFTAP_BSS_STARTED, &ap_adapter->event_flags))
        goto end;

    if (0 != wlan_hdd_cfg80211_update_apies(ap_adapter)) {
        hddLog(LOGE, FL("SAP Not able to set AP IEs"));
        WLANSAP_ResetSapConfigAddIE(pConfig, eUPDATE_IE_ALL);
        goto end;
    }

    vos_event_reset(&hostapd_state->vosEvent);
    if (WLANSAP_StartBss(hdd_ap_ctx->sapContext, hdd_hostapd_SAPEventCB,
                         &hdd_ap_ctx->sapConfig, (v_PVOID_t)ap_adapter->dev)
                         != VOS_STATUS_SUCCESS) {
        goto end;
    }

    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
              FL("Waiting for SAP to start"));
    vos_status = vos_wait_single_event(&hostapd_state->vosEvent, 10000);
    if (!VOS_IS_STATUS_SUCCESS(vos_status)) {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  FL("SAP Start failed"));
        goto end;
    }
    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
              FL("SAP Start Success"));
    set_bit(SOFTAP_BSS_STARTED, &ap_adapter->event_flags);
    wlan_hdd_incr_active_session(hdd_ctx, ap_adapter->device_mode);
    hostapd_state->bCommit = TRUE;
    if (hdd_ctx->cfg_ini->apOBSSProtEnabled)
        vos_runtime_pm_prevent_suspend(hdd_ctx->runtime_context.obss);

end:
    mutex_unlock(&hdd_ctx->sap_lock);
    return;
}
#endif

/**
 * hdd_wlan_go_set_mcc_p2p_quota() - Function to set quota for P2P GO
 * @hostapd_adapter:	Pointer to HDD adapter
 * @set_value:		Qouta value for the interface
 *
 * This function is used to set the quota for P2P GO cases
 *
 * Return: Configuration message posting status, SUCCESS or Fail
 *
 */
int32_t hdd_wlan_go_set_mcc_p2p_quota(hdd_adapter_t *hostapd_adapter,
					uint32_t set_value)
{
	uint8_t first_adapter_operating_channel = 0;
	uint8_t second_adapter_opertaing_channel = 0;
	tVOS_CONCURRENCY_MODE concurrent_state = 0;
	hdd_adapter_t *staAdapter = NULL;
	int32_t ret = 0; /* success */

	/*
	 * Check if concurrency mode is active.
	 * Need to modify this code to support MCC modes other than
	 * STA/P2P GO
	 */

	concurrent_state = hdd_get_concurrency_mode();
	if (concurrent_state == (VOS_STA | VOS_P2P_GO)) {
		hddLog(LOG1, "%s: STA & P2P are both enabled", __func__);

		/*
		 * The channel numbers for both adapters and the time
		 * quota for the 1st adapter, i.e., one specified in cmd
		 * are formatted as a bit vector then passed on to WMA
		 * +************************************************+
		 * |bit 31-24 |bit 23-16  |  bits 15-8  |bits 7-0   |
		 * |  Unused  |  Quota for| chan. # for |chan. # for|
		 * |          |  1st chan.| 1st chan.   |2nd chan.  |
		 * +************************************************+
		 */

		/* Get the operating channel of the specified vdev */
		first_adapter_operating_channel =
			hdd_get_operating_channel(hostapd_adapter->pHddCtx,
			hostapd_adapter->device_mode);

		hddLog(LOG1, "%s: 1st channel No.:%d and quota:%dms",
			__func__, first_adapter_operating_channel,
			set_value);

		/* Move the time quota for first adapter to bits 15-8 */
		set_value = set_value << 8;
		/*
		 * Store the operating channel number of 1st adapter at
		 * the lower 8-bits of bit vector.
		 */
		set_value = set_value | first_adapter_operating_channel;
		if (hostapd_adapter->device_mode ==
				WLAN_HDD_INFRA_STATION) {
			/* iwpriv cmd issued on wlan0; get p2p0 vdev chan */
			if ((concurrent_state & VOS_P2P_CLIENT) != 0) {
				/* The 2nd MCC vdev is P2P client */
				staAdapter = hdd_get_adapter
					(
					 hostapd_adapter->pHddCtx,
					 WLAN_HDD_P2P_CLIENT
					);
			} else {
				/* The 2nd MCC vdev is P2P GO */
				staAdapter = hdd_get_adapter
					(
					 hostapd_adapter->pHddCtx,
					 WLAN_HDD_P2P_GO
					);
			}
		} else {
			/* iwpriv cmd issued on p2p0; get channel for wlan0 */
			staAdapter = hdd_get_adapter
				(
				 hostapd_adapter->pHddCtx,
				 WLAN_HDD_INFRA_STATION
				);
		}
		if (staAdapter != NULL) {
			second_adapter_opertaing_channel =
				hdd_get_operating_channel
				(
				 staAdapter->pHddCtx,
				 staAdapter->device_mode
				);
			hddLog(LOG1, "%s: 2nd vdev channel No. is:%d",
					__func__,
					second_adapter_opertaing_channel);

			if (second_adapter_opertaing_channel == 0 ||
					first_adapter_operating_channel == 0) {
				hddLog(LOGE, "Invalid channel");
				return -EINVAL;
			}

			/*
			 * Move the time quota and operating channel number
			 * for the first adapter to bits 23-16 & bits 15-8
			 * of set_value vector, respectively.
			 */
			set_value = set_value << 8;
			/*
			 * Store the channel number for 2nd MCC vdev at bits
			 * 7-0 of set_value vector as per the bit format above.
			 */
			set_value = set_value |
				second_adapter_opertaing_channel;
			ret = process_wma_set_command
				(
				 (int32_t)hostapd_adapter->sessionId,
				 (int32_t)WMA_VDEV_MCC_SET_TIME_QUOTA,
				 set_value,
				 VDEV_CMD
				);
		} else {
			hddLog(LOGE, "%s: NULL adapter handle. Exit",
					__func__);
		}
	} else {
		hddLog(LOG1, "%s: MCC is not active. "
				"Exit w/o setting latency", __func__);
	}
	return ret;
}

/**
 * hdd_wlan_set_mcc_p2p_quota() - Function to set quota for P2P
 * @hostapd_adapter:    Pointer to HDD adapter
 * @set_value:          Qouta value for the interface
 *
 * This function is used to set the quota for P2P cases
 *
 * Return: Configuration message posting status, SUCCESS or Fail
 *
 */
int32_t hdd_wlan_set_mcc_p2p_quota(hdd_adapter_t *hostapd_adapater,
					uint32_t set_value)
{
	uint8_t first_adapter_operating_channel = 0;
	uint8_t second_adapter_opertaing_channel = 0;
	hdd_adapter_t *staAdapter = NULL;
	int32_t ret = 0; /* success */

	tVOS_CONCURRENCY_MODE concurrent_state = hdd_get_concurrency_mode();
	hddLog(LOG1, "iwpriv cmd to set MCC quota with val %dms",
			set_value);
	/*
	 * Check if concurrency mode is active.
	 * Need to modify this code to support MCC modes other than STA/P2P
	 */
	if ((concurrent_state == (VOS_STA | VOS_P2P_CLIENT)) ||
			(concurrent_state == (VOS_STA | VOS_P2P_GO))) {
		hddLog(LOG1, "STA & P2P are both enabled");
		/*
		 * The channel numbers for both adapters and the time
		 * quota for the 1st adapter, i.e., one specified in cmd
		 * are formatted as a bit vector then passed on to WMA
		 * +***********************************************************+
		 * |bit 31-24  | bit 23-16  |   bits 15-8   |   bits 7-0       |
		 * |  Unused   | Quota for  | chan. # for   |   chan. # for    |
		 * |           | 1st chan.  | 1st chan.     |   2nd chan.      |
		 * +***********************************************************+
		 */
		/* Get the operating channel of the specified vdev */
		first_adapter_operating_channel =
			hdd_get_operating_channel
			(
			 hostapd_adapater->pHddCtx,
			 hostapd_adapater->device_mode
			);
		hddLog(LOG1, "1st channel No.:%d and quota:%dms",
				first_adapter_operating_channel, set_value);
		/* Move the time quota for first channel to bits 15-8 */
		set_value = set_value << 8;
		/*
		 * Store the channel number of 1st channel at bits 7-0
		 * of the bit vector
		 */
		set_value = set_value | first_adapter_operating_channel;
		/* Find out the 2nd MCC adapter and its operating channel */
		if (hostapd_adapater->device_mode == WLAN_HDD_INFRA_STATION) {
			/*
			 * iwpriv cmd was issued on wlan0;
			 * get p2p0 vdev channel
			 */
			if ((concurrent_state & VOS_P2P_CLIENT) != 0) {
				/* The 2nd MCC vdev is P2P client */
				staAdapter = hdd_get_adapter(
						hostapd_adapater->pHddCtx,
						WLAN_HDD_P2P_CLIENT);
			} else {
				/* The 2nd MCC vdev is P2P GO */
				staAdapter = hdd_get_adapter(
						hostapd_adapater->pHddCtx,
						WLAN_HDD_P2P_GO);
			}
		} else {
			/*
			 * iwpriv cmd was issued on p2p0;
			 * get wlan0 vdev channel
			 */
			staAdapter = hdd_get_adapter(hostapd_adapater->pHddCtx,
					WLAN_HDD_INFRA_STATION);
		}
		if (staAdapter != NULL) {
			second_adapter_opertaing_channel =
				hdd_get_operating_channel
				(
				 staAdapter->pHddCtx,
				 staAdapter->device_mode
				);
			hddLog(LOG1, "2nd vdev channel No. is:%d",
					second_adapter_opertaing_channel);

			if (second_adapter_opertaing_channel == 0 ||
					first_adapter_operating_channel == 0) {
				hddLog(LOGE, "Invalid channel");
				return -EINVAL;
			}
			/*
			 * Now move the time quota and channel number of the
			 * 1st adapter to bits 23-16 and bits 15-8 of the bit
			 * vector, respectively.
			 */
			set_value = set_value << 8;
			/*
			 * Store the channel number for 2nd MCC vdev at bits
			 * 7-0 of set_value
			 */
			set_value = set_value |
					second_adapter_opertaing_channel;
			ret = process_wma_set_command(
					(int32_t)hostapd_adapater->sessionId,
					(int32_t)WMA_VDEV_MCC_SET_TIME_QUOTA,
					set_value, VDEV_CMD);
		} else {
			hddLog(LOGE, "NULL adapter handle. Exit");
		}
	} else {
		hddLog(LOG1, "%s: MCC is not active. Exit w/o setting latency",
				__func__);
	}
	return ret;
}

/**
 * hdd_get_fw_version() - Get FW version
 * @hdd_ctx:     pointer to HDD context.
 * @major_spid:  FW version - major spid.
 * @minor_spid:  FW version - minor spid
 * @ssid:        FW version - ssid
 * @crmid:       FW version - crmid
 *
 * This function is called to get the firmware build version stored
 * as part of the HDD context
 *
 * Return:   None
 */

void hdd_get_fw_version(hdd_context_t *hdd_ctx,
			uint32_t *major_spid, uint32_t *minor_spid,
			uint32_t *siid, uint32_t *crmid)
{
	*major_spid = (hdd_ctx->target_fw_version & 0xf0000000) >> 28;
	*minor_spid = (hdd_ctx->target_fw_version & 0xf000000) >> 24;
	*siid = (hdd_ctx->target_fw_version & 0xf00000) >> 20;
	*crmid = hdd_ctx->target_fw_version & 0x7fff;
}

#ifdef QCA_CONFIG_SMP
int wlan_hdd_get_cpu()
{
	int cpu_index = get_cpu();
	put_cpu();
	return cpu_index;
}
#endif

/**
 * hdd_get_fwpath() - get framework path
 *
 * This function is used to get the string written by
 * userspace to start the wlan driver
 *
 * Return: string
 */
const char *hdd_get_fwpath(void)
{
	return fwpath.string;
}

/**
 * hdd_enable_disable_ca_event() - enable/disable channel avoidance event
 * @hddctx: pointer to hdd context
 * @set_value: enable/disable
 *
 * When Host sends vendor command enable, FW will send *ONE* CA ind to
 * Host(even though it is duplicate). When Host send vendor command
 * disable,FW doesn't perform any action. Whenever any change in
 * CA *and* WLAN is in SAP/P2P-GO mode, FW sends CA ind to host.
 *
 * return - 0 on success, appropriate error values on failure.
 */
int hdd_enable_disable_ca_event(hdd_context_t *hddctx, tANI_U8 set_value)
{
	eHalStatus status;
	int ret_val = 0;

	if (0 != wlan_hdd_validate_context(hddctx)) {
		ret_val = -EAGAIN;
		goto exit;
	}

	if (!hddctx->cfg_ini->goptimize_chan_avoid_event) {
		hddLog(VOS_TRACE_LEVEL_ERROR,
			FL("goptimize_chan_avoid_event ini param disabled"));
		ret_val = -EAGAIN;
		goto exit;
	}

	status = sme_enable_disable_chanavoidind_event(hddctx->hHal, set_value);
	if (status != eHAL_STATUS_SUCCESS) {
		hddLog(VOS_TRACE_LEVEL_ERROR,
			FL("Failed to send chan avoid command to SME"));
		ret_val = -EINVAL;
	}

exit:
	return ret_val;
}

#ifdef WLAN_FEATURE_PACKET_FILTERING
/**
 * hdd_init_packet_filtering - allocate packet filter MC address list
 * @hdd_ctx: pointer to hdd context
 * @adapter: pointer to hdd_adapter_t
 *
 * This function allocates memory for link layer MC address list which will
 * be communicated to firmware for packet filtering in HOST suspend state.
 *
 * Return: 0 on success, error number otherwise.
 */
int hdd_init_packet_filtering(hdd_context_t *hdd_ctx,
					hdd_adapter_t *adapter)
{
	adapter->mc_addr_list.addr =
			vos_mem_malloc(hdd_ctx->max_mc_addr_list * ETH_ALEN);

	if (NULL == adapter->mc_addr_list.addr) {
		hddLog(LOGE, FL("Could not allocate Memory"));
		return -ENOMEM;
	}

	vos_mem_zero(adapter->mc_addr_list.addr,
		(hdd_ctx->max_mc_addr_list * ETH_ALEN));

	return 0;
}

/**
 * hdd_deinit_packet_filtering - deallocate packet filter MC address list
 * @adapter: pointer to hdd_adapter_t
 *
 * Return: none
 */
void hdd_deinit_packet_filtering(hdd_adapter_t *adapter)
{
	vos_mem_free(adapter->mc_addr_list.addr);
	adapter->mc_addr_list.addr = NULL;
}
#endif

/**
 * wlan_hdd_get_dfs_mode() - get ACS DFS mode
 * @mode : cfg80211 DFS mode
 *
 * Return: return SAP ACS DFS mode else return ACS_DFS_MODE_NONE
 */
enum  sap_acs_dfs_mode wlan_hdd_get_dfs_mode(enum dfs_mode mode)
{
	switch (mode) {
	case DFS_MODE_ENABLE:
		return ACS_DFS_MODE_ENABLE;
		break;
	case DFS_MODE_DISABLE:
		return ACS_DFS_MODE_DISABLE;
		break;
	case DFS_MODE_DEPRIORITIZE:
		return ACS_DFS_MODE_DEPRIORITIZE;
		break;
	default:
		hddLog(VOS_TRACE_LEVEL_ERROR,
			FL("ACS dfs mode is NONE"));
		return  ACS_DFS_MODE_NONE;
	}
}


/**
 * hdd_set_rps_cpu_mask - set RPS CPU mask for interfaces
 * @hdd_ctx: pointer to hdd_context_t
 *
 * Return: none
 */
void hdd_set_rps_cpu_mask(hdd_context_t *hdd_ctx)
{
	hdd_adapter_t *adapter;
	hdd_adapter_list_node_t *adapter_node, *next;
	VOS_STATUS status = VOS_STATUS_SUCCESS;

	status = hdd_get_front_adapter (hdd_ctx, &adapter_node);
	while (NULL != adapter_node && VOS_STATUS_SUCCESS == status) {
		adapter = adapter_node->pAdapter;
		if (NULL != adapter)
			hdd_dp_util_send_rps_ind(adapter);
		status = hdd_get_next_adapter (hdd_ctx, adapter_node, &next);
		adapter_node = next;
	}
}

/**
 * hdd_initialize_adapter_common() - initialize completion variables
 * @adapter: pointer to hdd_adapter_t
 *
 * Return: none
 */
void hdd_initialize_adapter_common(hdd_adapter_t *adapter)
{
	if (NULL == adapter) {
		hddLog(VOS_TRACE_LEVEL_ERROR, "%s: adapter is NULL ", __func__);
		return;
	}
	init_completion(&adapter->session_open_comp_var);
	init_completion(&adapter->session_close_comp_var);
	init_completion(&adapter->disconnect_comp_var);
	init_completion(&adapter->linkup_event_var);
	init_completion(&adapter->cancel_rem_on_chan_var);
	init_completion(&adapter->rem_on_chan_ready_event);
	init_completion(&adapter->offchannel_tx_event);
	init_completion(&adapter->tx_action_cnf_event);
#ifdef FEATURE_WLAN_TDLS
	init_completion(&adapter->tdls_add_station_comp);
	init_completion(&adapter->tdls_del_station_comp);
	init_completion(&adapter->tdls_mgmt_comp);
	init_completion(&adapter->tdls_link_establish_req_comp);
#endif

#ifdef WLAN_FEATURE_RMC
	init_completion(&adapter->ibss_peer_info_comp);
#endif /* WLAN_FEATURE_RMC */
	init_completion(&adapter->ula_complete);
	init_completion(&adapter->change_country_code);
	init_completion(&adapter->smps_force_mode_comp_var);
	init_completion(&adapter->scan_info.scan_req_completion_event);
	init_completion(&adapter->scan_info.abortscan_event_var);

	return;
}

//Register the module init/exit functions
#ifndef FEATURE_LARGE_PREALLOC
module_init(hdd_module_init);
module_exit(hdd_module_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Qualcomm Atheros, Inc.");
MODULE_DESCRIPTION("WLAN HOST DEVICE DRIVER");

#if !defined(QCA_WIFI_FTM)
static const struct kernel_param_ops con_mode_ops = {
    .set = con_mode_handler,
    .get = param_get_int,
};
#endif

static const struct kernel_param_ops fwpath_ops = {
    .set = fwpath_changed_handler,
    .get = param_get_string,
};

#if  defined(QCA_WIFI_FTM)
module_param(con_mode, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#else
module_param_cb(con_mode, &con_mode_ops, &con_mode,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif

module_param_cb(fwpath, &fwpath_ops, &fwpath,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

module_param(enable_dfs_chan_scan, int,
             S_IRUSR | S_IRGRP | S_IROTH);

module_param(enable_11d, int,
             S_IRUSR | S_IRGRP | S_IROTH);

module_param(country_code, charp,
             S_IRUSR | S_IRGRP | S_IROTH);
#else /* FEATURE_LARGE_PREALLOC */

void register_wlan_module_parameters_callback(int con_mode_set,
                                char* country_code_set,
                                char* version_string_set
)
{
	con_mode = con_mode_set;
	country_code = country_code_set;
	version_string = version_string_set;
}
EXPORT_SYMBOL(register_wlan_module_parameters_callback);
#endif /* #ifndef FEATURE_LARGE_PREALLOC */
