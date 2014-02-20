/******************************************************************************
 *
 * Copyright(c) 2007 - 2012 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *
 ******************************************************************************/
#define  _IOCTL_CFG80211_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtw_ioctl.h>
#include <rtw_ioctl_set.h>
#include <xmit_osdep.h>

#ifdef CONFIG_IOCTL_CFG80211

#include "ioctl_cfg80211.h"

#define RTW_MAX_MGMT_TX_CNT (8)

#define RTW_SCAN_IE_LEN_MAX      2304
#define RTW_MAX_REMAIN_ON_CHANNEL_DURATION 65535 /* ms */
#define RTW_MAX_NUM_PMKIDS 4

#define RTW_CH_MAX_2G_CHANNEL               14      /* Max channel in 2G band */

static const u32 rtw_cipher_suites[] = {
	WLAN_CIPHER_SUITE_WEP40,
	WLAN_CIPHER_SUITE_WEP104,
	WLAN_CIPHER_SUITE_TKIP,
	WLAN_CIPHER_SUITE_CCMP,
};

#define RATETAB_ENT(_rate, _rateid, _flags) \
	{								\
		.bitrate	= (_rate),				\
		.hw_value	= (_rateid),				\
		.flags		= (_flags),				\
	}

#define CHAN2G(_channel, _freq, _flags) {			\
	.band			= IEEE80211_BAND_2GHZ,		\
	.center_freq		= (_freq),			\
	.hw_value		= (_channel),			\
	.flags			= (_flags),			\
	.max_antenna_gain	= 0,				\
	.max_power		= 30,				\
}

#define CHAN5G(_channel, _flags) {				\
	.band			= IEEE80211_BAND_5GHZ,		\
	.center_freq		= 5000 + (5 * (_channel)),	\
	.hw_value		= (_channel),			\
	.flags			= (_flags),			\
	.max_antenna_gain	= 0,				\
	.max_power		= 30,				\
}

static struct ieee80211_rate rtw_rates[] = {
	RATETAB_ENT(10,  0x1,   0),
	RATETAB_ENT(20,  0x2,   0),
	RATETAB_ENT(55,  0x4,   0),
	RATETAB_ENT(110, 0x8,   0),
	RATETAB_ENT(60,  0x10,  0),
	RATETAB_ENT(90,  0x20,  0),
	RATETAB_ENT(120, 0x40,  0),
	RATETAB_ENT(180, 0x80,  0),
	RATETAB_ENT(240, 0x100, 0),
	RATETAB_ENT(360, 0x200, 0),
	RATETAB_ENT(480, 0x400, 0),
	RATETAB_ENT(540, 0x800, 0),
};

#define rtw_a_rates		(rtw_rates + 4)
#define RTW_A_RATES_NUM	8
#define rtw_g_rates		(rtw_rates + 0)
#define RTW_G_RATES_NUM	12

#define RTW_2G_CHANNELS_NUM 14
#define RTW_5G_CHANNELS_NUM 37

static struct ieee80211_channel rtw_2ghz_channels[] = {
	CHAN2G(1, 2412, 0),
	CHAN2G(2, 2417, 0),
	CHAN2G(3, 2422, 0),
	CHAN2G(4, 2427, 0),
	CHAN2G(5, 2432, 0),
	CHAN2G(6, 2437, 0),
	CHAN2G(7, 2442, 0),
	CHAN2G(8, 2447, 0),
	CHAN2G(9, 2452, 0),
	CHAN2G(10, 2457, 0),
	CHAN2G(11, 2462, 0),
	CHAN2G(12, 2467, 0),
	CHAN2G(13, 2472, 0),
	CHAN2G(14, 2484, 0),
};

static struct ieee80211_channel rtw_5ghz_a_channels[] = {
	CHAN5G(34, 0),		CHAN5G(36, 0),
	CHAN5G(38, 0),		CHAN5G(40, 0),
	CHAN5G(42, 0),		CHAN5G(44, 0),
	CHAN5G(46, 0),		CHAN5G(48, 0),
	CHAN5G(52, 0),		CHAN5G(56, 0),
	CHAN5G(60, 0),		CHAN5G(64, 0),
	CHAN5G(100, 0),		CHAN5G(104, 0),
	CHAN5G(108, 0),		CHAN5G(112, 0),
	CHAN5G(116, 0),		CHAN5G(120, 0),
	CHAN5G(124, 0),		CHAN5G(128, 0),
	CHAN5G(132, 0),		CHAN5G(136, 0),
	CHAN5G(140, 0),		CHAN5G(149, 0),
	CHAN5G(153, 0),		CHAN5G(157, 0),
	CHAN5G(161, 0),		CHAN5G(165, 0),
	CHAN5G(184, 0),		CHAN5G(188, 0),
	CHAN5G(192, 0),		CHAN5G(196, 0),
	CHAN5G(200, 0),		CHAN5G(204, 0),
	CHAN5G(208, 0),		CHAN5G(212, 0),
	CHAN5G(216, 0),
};

void rtw_2g_channels_init(struct ieee80211_channel *channels)
{
	memcpy((void*)channels, (void*)rtw_2ghz_channels,
		sizeof(struct ieee80211_channel)*RTW_2G_CHANNELS_NUM
	);
}

void rtw_5g_channels_init(struct ieee80211_channel *channels)
{
	memcpy((void*)channels, (void*)rtw_5ghz_a_channels,
		sizeof(struct ieee80211_channel)*RTW_5G_CHANNELS_NUM
	);
}

void rtw_2g_rates_init(struct ieee80211_rate *rates)
{
	memcpy(rates, rtw_g_rates,
		sizeof(struct ieee80211_rate)*RTW_G_RATES_NUM
	);
}

void rtw_5g_rates_init(struct ieee80211_rate *rates)
{
	memcpy(rates, rtw_a_rates,
		sizeof(struct ieee80211_rate)*RTW_A_RATES_NUM
	);
}

struct ieee80211_supported_band *rtw_spt_band_alloc(
	enum ieee80211_band band
	)
{
	struct ieee80211_supported_band *spt_band = NULL;
	int n_channels, n_bitrates;
	int len;

	if (band == IEEE80211_BAND_2GHZ)
	{
		n_channels = RTW_2G_CHANNELS_NUM;
		n_bitrates = RTW_G_RATES_NUM;
	}
	else if (band == IEEE80211_BAND_5GHZ)
	{
		n_channels = RTW_5G_CHANNELS_NUM;
		n_bitrates = RTW_A_RATES_NUM;
	}
	else
	{
		goto exit;
	}

	len = sizeof(struct ieee80211_supported_ban, GFP_KERNEL) +
	      sizeof(struct ieee80211_channel) * n_channels +
	      sizeof(struct ieee80211_rate) * n_bitrates;
	spt_band = (struct ieee80211_supported_band *)kzalloc(len, GFP_KERNEL);
	if (!spt_band)
		goto exit;

	spt_band->channels = (struct ieee80211_channel*)(((u8*)spt_band)+sizeof(struct ieee80211_supported_band));
	spt_band->bitrates= (struct ieee80211_rate*)(((u8*)spt_band->channels)+sizeof(struct ieee80211_channel)*n_channels);
	spt_band->band = band;
	spt_band->n_channels = n_channels;
	spt_band->n_bitrates = n_bitrates;

	if (band == IEEE80211_BAND_2GHZ)
	{
		rtw_2g_channels_init(spt_band->channels);
		rtw_2g_rates_init(spt_band->bitrates);
	}
	else if (band == IEEE80211_BAND_5GHZ)
	{
		rtw_5g_channels_init(spt_band->channels);
		rtw_5g_rates_init(spt_band->bitrates);
	}

	/* spt_band.ht_cap */

exit:

	return spt_band;
}

void rtw_spt_band_free(struct ieee80211_supported_band *spt_band)
{
	u32 size;

	if (!spt_band)
		return;

	if (spt_band->band == IEEE80211_BAND_2GHZ)
	{
		size = sizeof(struct ieee80211_supported_band)
			+ sizeof(struct ieee80211_channel)*RTW_2G_CHANNELS_NUM
			+ sizeof(struct ieee80211_rate)*RTW_G_RATES_NUM;
	}
	else if (spt_band->band == IEEE80211_BAND_5GHZ)
	{
		size = sizeof(struct ieee80211_supported_band)
			+ sizeof(struct ieee80211_channel)*RTW_5G_CHANNELS_NUM
			+ sizeof(struct ieee80211_rate)*RTW_A_RATES_NUM;
	}
	else
	{

	}
	kfree(spt_band);
}

static const struct ieee80211_txrx_stypes
rtw_cfg80211_default_mgmt_stypes[NUM_NL80211_IFTYPES] = {
	[NL80211_IFTYPE_ADHOC] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4)
	},
	[NL80211_IFTYPE_STATION] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
	},
	[NL80211_IFTYPE_AP] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		BIT(IEEE80211_STYPE_DISASSOC >> 4) |
		BIT(IEEE80211_STYPE_AUTH >> 4) |
		BIT(IEEE80211_STYPE_DEAUTH >> 4) |
		BIT(IEEE80211_STYPE_ACTION >> 4)
	},
	[NL80211_IFTYPE_AP_VLAN] = {
		/* copy AP */
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		BIT(IEEE80211_STYPE_DISASSOC >> 4) |
		BIT(IEEE80211_STYPE_AUTH >> 4) |
		BIT(IEEE80211_STYPE_DEAUTH >> 4) |
		BIT(IEEE80211_STYPE_ACTION >> 4)
	},
	[NL80211_IFTYPE_P2P_CLIENT] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
	},
	[NL80211_IFTYPE_P2P_GO] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		BIT(IEEE80211_STYPE_DISASSOC >> 4) |
		BIT(IEEE80211_STYPE_AUTH >> 4) |
		BIT(IEEE80211_STYPE_DEAUTH >> 4) |
		BIT(IEEE80211_STYPE_ACTION >> 4)
	},
};

static int rtw_ieee80211_channel_to_frequency(int chan, int band)
{
	/* see 802.11 17.3.8.3.2 and Annex J
	* there are overlapping channel numbers in 5GHz and 2GHz bands */

	if (band == IEEE80211_BAND_5GHZ) {
	if (chan >= 182 && chan <= 196)
			return 4000 + chan * 5;
             else
                    return 5000 + chan * 5;
       } else { /* IEEE80211_BAND_2GHZ */
		if (chan == 14)
			return 2484;
             else if (chan < 14)
			return 2407 + chan * 5;
             else
			return 0; /* not supported */
	}
}

#define MAX_BSSINFO_LEN 1000
static int rtw_cfg80211_inform_bss(_adapter *padapter, struct wlan_network *pnetwork)
{
	int ret=0;
	struct ieee80211_channel *notify_channel;
	struct cfg80211_bss *bss;
	u16 channel;
	u32 freq;
	u64 notify_timestamp;
	u16 notify_capability;
	u16 notify_interval;
	u8 *notify_ie;
	size_t notify_ielen;
	s32 notify_signal;
	u8 buf[MAX_BSSINFO_LEN], *pbuf;
	size_t len,bssinf_len=0;
	struct rtw_ieee80211_hdr *pwlanhdr;
	unsigned short *fctrl;
	u8	bc_addr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	struct wireless_dev *wdev = padapter->rtw_wdev;
	struct wiphy *wiphy = wdev->wiphy;
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);

	bssinf_len = pnetwork->network.IELength+sizeof (struct rtw_ieee80211_hdr_3addr);
	if (bssinf_len > MAX_BSSINFO_LEN) {
		DBG_871X("%s IE Length too long > %d byte\n",__func__,MAX_BSSINFO_LEN);
		goto exit;
	}

	channel = pnetwork->network.Configuration.DSConfig;
	if (channel <= RTW_CH_MAX_2G_CHANNEL)
		freq = rtw_ieee80211_channel_to_frequency(channel, IEEE80211_BAND_2GHZ);
	else
		freq = rtw_ieee80211_channel_to_frequency(channel, IEEE80211_BAND_5GHZ);

	notify_channel = ieee80211_get_channel(wiphy, freq);

	notify_timestamp = jiffies_to_msecs(jiffies)*1000; /* uSec */

	notify_interval = le16_to_cpu(*(u16*)rtw_get_beacon_interval_from_ie(pnetwork->network.IEs));
	notify_capability = le16_to_cpu(*(u16*)rtw_get_capability_from_ie(pnetwork->network.IEs));

	notify_ie = pnetwork->network.IEs+_FIXED_IE_LENGTH_;
	notify_ielen = pnetwork->network.IELength-_FIXED_IE_LENGTH_;

	/* We've set wiphy's signal_type as CFG80211_SIGNAL_TYPE_MBM: signal strength in mBm (100*dBm) */
	if (check_fwstate(pmlmepriv, _FW_LINKED)== true &&
		is_same_network(&pmlmepriv->cur_network.network, &pnetwork->network)) {
		notify_signal = 100*translate_percentage_to_dbm(padapter->recvpriv.signal_strength);/* dbm */
	} else {
		notify_signal = 100*translate_percentage_to_dbm(pnetwork->network.PhyInfo.SignalStrength);/* dbm */
	}

/*
	DBG_8192C("bssid: %2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X\n",
                        pnetwork->network.MacAddress[0], pnetwork->network.MacAddress[1], pnetwork->network.MacAddress[2],
                        pnetwork->network.MacAddress[3], pnetwork->network.MacAddress[4], pnetwork->network.MacAddress[5]);
	DBG_8192C("Channel: %d(%d)\n", channel, freq);
	DBG_8192C("Capability: %X\n", notify_capability);
	DBG_8192C("Beacon interval: %d\n", notify_interval);
	DBG_8192C("Signal: %d\n", notify_signal);
	DBG_8192C("notify_timestamp: %#018llx\n", notify_timestamp);
*/

	pbuf = buf;

	pwlanhdr = (struct rtw_ieee80211_hdr *)pbuf;
	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	SetSeqNum(pwlanhdr, 0/*pmlmeext->mgnt_seq*/);

	if (pnetwork->network.Reserved[0] == 1) { /*  WIFI_BEACON */
		memcpy(pwlanhdr->addr1, bc_addr, ETH_ALEN);
		SetFrameSubType(pbuf, WIFI_BEACON);
	} else {
		memcpy(pwlanhdr->addr1, myid(&(padapter->eeprompriv)), ETH_ALEN);
		SetFrameSubType(pbuf, WIFI_PROBERSP);
	}

	memcpy(pwlanhdr->addr2, pnetwork->network.MacAddress, ETH_ALEN);
	memcpy(pwlanhdr->addr3, pnetwork->network.MacAddress, ETH_ALEN);

	pbuf += sizeof(struct rtw_ieee80211_hdr_3addr);
	len = sizeof (struct rtw_ieee80211_hdr_3addr);

	memcpy(pbuf, pnetwork->network.IEs, pnetwork->network.IELength);
	len += pnetwork->network.IELength;

	/* ifdef CONFIG_P2P */

	bss = cfg80211_inform_bss_frame(wiphy, notify_channel, (struct ieee80211_mgmt *)buf,
		len, notify_signal, GFP_ATOMIC);
	if (unlikely(!bss)) {
		DBG_8192C("rtw_cfg80211_inform_bss error\n");
		return -EINVAL;
	}

	cfg80211_put_bss(bss);
exit:
	return ret;
}

void rtw_cfg80211_indicate_connect(_adapter *padapter)
{
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct wlan_network  *cur_network = &(pmlmepriv->cur_network);
	struct wireless_dev *pwdev = padapter->rtw_wdev;
#ifdef CONFIG_P2P
	struct wifidirect_info *pwdinfo= &(padapter->wdinfo);
#endif

	DBG_8192C("%s(padapter=%p)\n", __func__, padapter);

	if (pwdev->iftype != NL80211_IFTYPE_STATION &&
	    pwdev->iftype != NL80211_IFTYPE_P2P_CLIENT)
		return;

	if (check_fwstate(pmlmepriv, WIFI_AP_STATE) == true)
		return;

#ifdef CONFIG_P2P
	if (!rtw_p2p_chk_state(pwdinfo, P2P_STATE_NONE))
	{
		rtw_p2p_set_pre_state(pwdinfo, rtw_p2p_state(pwdinfo));
		rtw_p2p_set_role(pwdinfo, P2P_ROLE_CLIENT);
		rtw_p2p_set_state(pwdinfo, P2P_STATE_GONEGO_OK);
		DBG_8192C("%s, role=%d, p2p_state=%d, pre_p2p_state=%d\n", __func__, rtw_p2p_role(pwdinfo), rtw_p2p_state(pwdinfo), rtw_p2p_pre_state(pwdinfo));
	}
#endif /* CONFIG_P2P */

	#ifdef CONFIG_LAYER2_ROAMING
	if (rtw_to_roaming(padapter) > 0) {
		struct wiphy *wiphy = pwdev->wiphy;
		struct ieee80211_channel *notify_channel;
		u32 freq;
		u16 channel = cur_network->network.Configuration.DSConfig;

		if (channel <= RTW_CH_MAX_2G_CHANNEL)
			freq = rtw_ieee80211_channel_to_frequency(channel, IEEE80211_BAND_2GHZ);
		else
			freq = rtw_ieee80211_channel_to_frequency(channel, IEEE80211_BAND_5GHZ);

		notify_channel = ieee80211_get_channel(wiphy, freq);

		DBG_871X("%s call cfg80211_roamed\n", __func__);
		cfg80211_roamed(padapter->pnetdev, notify_channel,
				cur_network->network.MacAddress,
				pmlmepriv->assoc_req +
				sizeof(struct rtw_ieee80211_hdr_3addr) + 2,
				pmlmepriv->assoc_req_len -
				sizeof(struct rtw_ieee80211_hdr_3addr) - 2,
				pmlmepriv->assoc_rsp +
				sizeof(struct rtw_ieee80211_hdr_3addr) + 6,
				pmlmepriv->assoc_rsp_len -
				sizeof(struct rtw_ieee80211_hdr_3addr) - 6,
				GFP_ATOMIC);
	} else
	#endif
	{
		DBG_8192C("pwdev->sme_state(b)=%d\n", pwdev->sme_state);
		cfg80211_connect_result(padapter->pnetdev, cur_network->network.MacAddress
			, pmlmepriv->assoc_req+sizeof(struct rtw_ieee80211_hdr_3addr)+2
			, pmlmepriv->assoc_req_len-sizeof(struct rtw_ieee80211_hdr_3addr)-2
			, pmlmepriv->assoc_rsp+sizeof(struct rtw_ieee80211_hdr_3addr)+6
			, pmlmepriv->assoc_rsp_len-sizeof(struct rtw_ieee80211_hdr_3addr)-6
			, WLAN_STATUS_SUCCESS, GFP_ATOMIC);
		DBG_8192C("pwdev->sme_state(a)=%d\n", pwdev->sme_state);
	}
}

void rtw_cfg80211_indicate_disconnect(_adapter *padapter)
{
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct wireless_dev *pwdev = padapter->rtw_wdev;
#ifdef CONFIG_P2P
	struct wifidirect_info *pwdinfo= &(padapter->wdinfo);
#endif

	DBG_8192C("%s(padapter=%p)\n", __func__, padapter);

	if (pwdev->iftype != NL80211_IFTYPE_STATION &&
	    pwdev->iftype != NL80211_IFTYPE_P2P_CLIENT)
		return;

	if (check_fwstate(pmlmepriv, WIFI_AP_STATE) == true)
		return;

#ifdef CONFIG_P2P
	if (!rtw_p2p_chk_state(pwdinfo, P2P_STATE_NONE))
	{
		_cancel_timer_ex(&pwdinfo->find_phase_timer);
		_cancel_timer_ex(&pwdinfo->restore_p2p_state_timer);
		_cancel_timer_ex(&pwdinfo->pre_tx_scan_timer);

		rtw_p2p_set_state(pwdinfo, rtw_p2p_pre_state(pwdinfo));
		rtw_p2p_set_role(pwdinfo, P2P_ROLE_DEVICE);

		DBG_8192C("%s, role=%d, p2p_state=%d, pre_p2p_state=%d\n", __func__, rtw_p2p_role(pwdinfo), rtw_p2p_state(pwdinfo), rtw_p2p_pre_state(pwdinfo));
	}
#endif /* CONFIG_P2P */

	if (!padapter->mlmepriv.not_indic_disco) {
		DBG_8192C("pwdev->sme_state(b)=%d\n", pwdev->sme_state);

		if (pwdev->sme_state==CFG80211_SME_CONNECTING)
			cfg80211_connect_result(padapter->pnetdev, NULL, NULL, 0, NULL, 0,
				WLAN_STATUS_UNSPECIFIED_FAILURE, GFP_ATOMIC/*GFP_KERNEL*/);
		else if (pwdev->sme_state==CFG80211_SME_CONNECTED)
			cfg80211_disconnected(padapter->pnetdev, 0, NULL, 0, GFP_ATOMIC);

		DBG_8192C("pwdev->sme_state(a)=%d\n", pwdev->sme_state);
	}
}

#ifdef CONFIG_92D_AP_MODE
static u8 set_pairwise_key(_adapter *padapter, struct sta_info *psta)
{
	struct cmd_obj*			ph2c;
	struct set_stakey_parm	*psetstakey_para;
	struct cmd_priv				*pcmdpriv=&padapter->cmdpriv;
	u8	res=_SUCCESS;

	ph2c = (struct cmd_obj*)kzalloc(sizeof(struct cmd_obj), GFP_KERNEL);
	if (ph2c == NULL) {
		res= _FAIL;
		goto exit;
	}

	psetstakey_para = (struct set_stakey_parm*)kzalloc(sizeof(struct set_stakey_parm), GFP_KERNEL);
	if (psetstakey_para==NULL) {
		kfree(ph2c);
		res=_FAIL;
		goto exit;
	}

	init_h2fwcmd_w_parm_no_rsp(ph2c, psetstakey_para, _SetStaKey_CMD_);

	psetstakey_para->algorithm = (u8)psta->dot118021XPrivacy;

	memcpy(psetstakey_para->addr, psta->hwaddr, ETH_ALEN);

	memcpy(psetstakey_para->key, &psta->dot118021x_UncstKey, 16);

	res = rtw_enqueue_cmd(pcmdpriv, ph2c);

exit:

	return res;
}

static int set_group_key(_adapter *padapter, u8 *key, u8 alg, int keyid)
{
	u8 keylen;
	struct cmd_obj* pcmd;
	struct setkey_parm *psetkeyparm;
	struct cmd_priv	*pcmdpriv=&(padapter->cmdpriv);
	int res=_SUCCESS;

	DBG_8192C("%s\n", __func__);

	pcmd = (struct cmd_obj*)kzalloc(sizeof(struct cmd_obj), GFP_KERNEL);
	if (pcmd==NULL) {
		res= _FAIL;
		goto exit;
	}
	psetkeyparm=(struct setkey_parm*)kzalloc(sizeof(struct setkey_parm), GFP_KERNEL);
	if (psetkeyparm==NULL) {
		kfree(pcmd);
		res= _FAIL;
		goto exit;
	}

	memset(psetkeyparm, 0, sizeof(struct setkey_parm));

	psetkeyparm->keyid=(u8)keyid;

	psetkeyparm->algorithm = alg;

	psetkeyparm->set_tx = 1;

	switch (alg)
	{
		case _WEP40_:
			keylen = 5;
			break;
		case _WEP104_:
			keylen = 13;
			break;
		case _TKIP_:
		case _TKIP_WTMIC_:
		case _AES_:
			keylen = 16;
		default:
			keylen = 16;
	}

	memcpy(&(psetkeyparm->key[0]), key, keylen);

	pcmd->cmdcode = _SetKey_CMD_;
	pcmd->parmbuf = (u8 *)psetkeyparm;
	pcmd->cmdsz =  (sizeof(struct setkey_parm));
	pcmd->rsp = NULL;
	pcmd->rspsz = 0;

	INIT_LIST_HEAD(&pcmd->list);

	res = rtw_enqueue_cmd(pcmdpriv, pcmd);

exit:

	return res;

}

static int set_wep_key(_adapter *padapter, u8 *key, u8 keylen, int keyid)
{
	u8 alg;

	switch (keylen)
	{
		case 5:
			alg =_WEP40_;
			break;
		case 13:
			alg =_WEP104_;
			break;
		default:
			alg =_NO_PRIVACY_;
	}

	return set_group_key(padapter, key, alg, keyid);
}

static int rtw_cfg80211_ap_set_encryption(struct net_device *dev, struct ieee_param *param, u32 param_len)
{
	int ret = 0;
	u32 wep_key_idx, wep_key_len,wep_total_len;
	struct sta_info *psta = NULL, *pbcmc_sta = NULL;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct mlme_priv	*pmlmepriv = &padapter->mlmepriv;
	struct security_priv* psecuritypriv=&(padapter->securitypriv);
	struct sta_priv *pstapriv = &padapter->stapriv;

	DBG_8192C("%s\n", __func__);

	param->u.crypt.err = 0;
	param->u.crypt.alg[IEEE_CRYPT_ALG_NAME_LEN - 1] = '\0';

	/* sizeof(struct ieee_param) = 64 bytes; */
	if (param_len !=  sizeof(struct ieee_param) + param->u.crypt.key_len)
	{
		ret =  -EINVAL;
		goto exit;
	}

	if (param->sta_addr[0] == 0xff && param->sta_addr[1] == 0xff &&
	    param->sta_addr[2] == 0xff && param->sta_addr[3] == 0xff &&
	    param->sta_addr[4] == 0xff && param->sta_addr[5] == 0xff)
	{
		if (param->u.crypt.idx >= WEP_KEYS)
		{
			ret = -EINVAL;
			goto exit;
		}
	}
	else
	{
		psta = rtw_get_stainfo(pstapriv, param->sta_addr);
		if (!psta)
		{
			DBG_8192C("rtw_set_encryption(), sta has already been removed or never been added\n");
			goto exit;
		}
	}

	if (strcmp(param->u.crypt.alg, "none") == 0 && (psta==NULL))
	{
		/* todo:clear default encryption keys */

		DBG_8192C("clear default encryption keys, keyid=%d\n", param->u.crypt.idx);

		goto exit;
	}

	if (strcmp(param->u.crypt.alg, "WEP") == 0 && (psta==NULL))
	{
		DBG_8192C("r871x_set_encryption, crypt.alg = WEP\n");

		wep_key_idx = param->u.crypt.idx;
		wep_key_len = param->u.crypt.key_len;

		DBG_8192C("r871x_set_encryption, wep_key_idx=%d, len=%d\n", wep_key_idx, wep_key_len);

		if ((wep_key_idx >= WEP_KEYS) || (wep_key_len<=0))
		{
			ret = -EINVAL;
			goto exit;
		}

		if (wep_key_len > 0)
		{
			wep_key_len = wep_key_len <= 5 ? 5 : 13;
		}

		if (psecuritypriv->bWepDefaultKeyIdxSet == 0)
		{
			/* wep default key has not been set, so use this key index as default key. */

			psecuritypriv->ndisencryptstatus = Ndis802_11Encryption1Enabled;
			psecuritypriv->dot11PrivacyAlgrthm=_WEP40_;
			psecuritypriv->dot118021XGrpPrivacy=_WEP40_;

			if (wep_key_len == 13)
			{
				psecuritypriv->dot11PrivacyAlgrthm=_WEP104_;
				psecuritypriv->dot118021XGrpPrivacy=_WEP104_;
			}

			psecuritypriv->dot11PrivacyKeyIndex = wep_key_idx;
		}

		memcpy(&(psecuritypriv->dot11DefKey[wep_key_idx].skey[0]), param->u.crypt.key, wep_key_len);

		psecuritypriv->dot11DefKeylen[wep_key_idx] = wep_key_len;

		set_wep_key(padapter, param->u.crypt.key, wep_key_len, wep_key_idx);

		goto exit;

	}

	if (!psta && check_fwstate(pmlmepriv, WIFI_AP_STATE)) /*  group key */
	{
		if (param->u.crypt.set_tx == 0) /* group key */
		{
			if (strcmp(param->u.crypt.alg, "WEP") == 0)
			{
				DBG_8192C("%s, set group_key, WEP\n", __func__);

				memcpy(psecuritypriv->dot118021XGrpKey[param->u.crypt.idx].skey,  param->u.crypt.key, (param->u.crypt.key_len>16 ?16:param->u.crypt.key_len));

				psecuritypriv->dot118021XGrpPrivacy = _WEP40_;
				if (param->u.crypt.key_len==13)
				{
						psecuritypriv->dot118021XGrpPrivacy = _WEP104_;
				}

			}
			else if (strcmp(param->u.crypt.alg, "TKIP") == 0)
			{
				DBG_8192C("%s, set group_key, TKIP\n", __func__);

				psecuritypriv->dot118021XGrpPrivacy = _TKIP_;

				memcpy(psecuritypriv->dot118021XGrpKey[param->u.crypt.idx].skey,  param->u.crypt.key, (param->u.crypt.key_len>16 ?16:param->u.crypt.key_len));

				/* set mic key */
				memcpy(psecuritypriv->dot118021XGrptxmickey[param->u.crypt.idx].skey, &(param->u.crypt.key[16]), 8);
				memcpy(psecuritypriv->dot118021XGrprxmickey[param->u.crypt.idx].skey, &(param->u.crypt.key[24]), 8);

				psecuritypriv->busetkipkey = true;

			}
			else if (strcmp(param->u.crypt.alg, "CCMP") == 0)
			{
				DBG_8192C("%s, set group_key, CCMP\n", __func__);

				psecuritypriv->dot118021XGrpPrivacy = _AES_;

				memcpy(psecuritypriv->dot118021XGrpKey[param->u.crypt.idx].skey,  param->u.crypt.key, (param->u.crypt.key_len>16 ?16:param->u.crypt.key_len));
			}
			else
			{
				DBG_8192C("%s, set group_key, none\n", __func__);

				psecuritypriv->dot118021XGrpPrivacy = _NO_PRIVACY_;
			}

			psecuritypriv->dot118021XGrpKeyid = param->u.crypt.idx;

			psecuritypriv->binstallGrpkey = true;

			psecuritypriv->dot11PrivacyAlgrthm = psecuritypriv->dot118021XGrpPrivacy;/*  */

			set_group_key(padapter, param->u.crypt.key, psecuritypriv->dot118021XGrpPrivacy, param->u.crypt.idx);

			pbcmc_sta=rtw_get_bcmc_stainfo(padapter);
			if (pbcmc_sta)
			{
				pbcmc_sta->ieee8021x_blocked = false;
				pbcmc_sta->dot118021XPrivacy= psecuritypriv->dot118021XGrpPrivacy;/* rx will use bmc_sta's dot118021XPrivacy */
			}

		}

		goto exit;

	}

	if (psecuritypriv->dot11AuthAlgrthm == dot11AuthAlgrthm_8021X && psta) /*  psk/802_1x */
	{
		if (check_fwstate(pmlmepriv, WIFI_AP_STATE))
		{
			if (param->u.crypt.set_tx ==1) /* pairwise key */
			{
				memcpy(psta->dot118021x_UncstKey.skey,  param->u.crypt.key, (param->u.crypt.key_len>16 ?16:param->u.crypt.key_len));

				if (strcmp(param->u.crypt.alg, "WEP") == 0)
				{
					DBG_8192C("%s, set pairwise key, WEP\n", __func__);

					psta->dot118021XPrivacy = _WEP40_;
					if (param->u.crypt.key_len==13)
					{
						psta->dot118021XPrivacy = _WEP104_;
					}
				}
				else if (strcmp(param->u.crypt.alg, "TKIP") == 0)
				{
					DBG_8192C("%s, set pairwise key, TKIP\n", __func__);

					psta->dot118021XPrivacy = _TKIP_;

					/* set mic key */
					memcpy(psta->dot11tkiptxmickey.skey, &(param->u.crypt.key[16]), 8);
					memcpy(psta->dot11tkiprxmickey.skey, &(param->u.crypt.key[24]), 8);

					psecuritypriv->busetkipkey = true;

				}
				else if (strcmp(param->u.crypt.alg, "CCMP") == 0)
				{

					DBG_8192C("%s, set pairwise key, CCMP\n", __func__);

					psta->dot118021XPrivacy = _AES_;
				}
				else
				{
					DBG_8192C("%s, set pairwise key, none\n", __func__);

					psta->dot118021XPrivacy = _NO_PRIVACY_;
				}

				set_pairwise_key(padapter, psta);

				psta->ieee8021x_blocked = false;

				psta->bpairwise_key_installed = true;

			}
			else/* group key??? */
			{
				if (strcmp(param->u.crypt.alg, "WEP") == 0)
				{
					memcpy(psecuritypriv->dot118021XGrpKey[param->u.crypt.idx].skey,  param->u.crypt.key, (param->u.crypt.key_len>16 ?16:param->u.crypt.key_len));

					psecuritypriv->dot118021XGrpPrivacy = _WEP40_;
					if (param->u.crypt.key_len==13)
					{
						psecuritypriv->dot118021XGrpPrivacy = _WEP104_;
					}
				}
				else if (strcmp(param->u.crypt.alg, "TKIP") == 0)
				{
					psecuritypriv->dot118021XGrpPrivacy = _TKIP_;

					memcpy(psecuritypriv->dot118021XGrpKey[param->u.crypt.idx].skey,  param->u.crypt.key, (param->u.crypt.key_len>16 ?16:param->u.crypt.key_len));

					/* set mic key */
					memcpy(psecuritypriv->dot118021XGrptxmickey[param->u.crypt.idx].skey, &(param->u.crypt.key[16]), 8);
					memcpy(psecuritypriv->dot118021XGrprxmickey[param->u.crypt.idx].skey, &(param->u.crypt.key[24]), 8);

					psecuritypriv->busetkipkey = true;

				}
				else if (strcmp(param->u.crypt.alg, "CCMP") == 0)
				{
					psecuritypriv->dot118021XGrpPrivacy = _AES_;

					memcpy(psecuritypriv->dot118021XGrpKey[param->u.crypt.idx].skey,  param->u.crypt.key, (param->u.crypt.key_len>16 ?16:param->u.crypt.key_len));
				}
				else
				{
					psecuritypriv->dot118021XGrpPrivacy = _NO_PRIVACY_;
				}

				psecuritypriv->dot118021XGrpKeyid = param->u.crypt.idx;

				psecuritypriv->binstallGrpkey = true;

				psecuritypriv->dot11PrivacyAlgrthm = psecuritypriv->dot118021XGrpPrivacy;/*  */

				set_group_key(padapter, param->u.crypt.key, psecuritypriv->dot118021XGrpPrivacy, param->u.crypt.idx);

				pbcmc_sta=rtw_get_bcmc_stainfo(padapter);
				if (pbcmc_sta)
				{
					pbcmc_sta->ieee8021x_blocked = false;
					pbcmc_sta->dot118021XPrivacy= psecuritypriv->dot118021XGrpPrivacy;/* rx will use bmc_sta's dot118021XPrivacy */
				}

			}

		}

	}

exit:

	return ret;
}
#endif

static int rtw_cfg80211_set_encryption(struct net_device *dev, struct ieee_param *param, u32 param_len)
{
	int ret = 0;
	u32 wep_key_idx, wep_key_len,wep_total_len;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(dev);
	struct mlme_priv	*pmlmepriv = &padapter->mlmepriv;
	struct security_priv *psecuritypriv = &padapter->securitypriv;
#ifdef CONFIG_P2P
	struct wifidirect_info* pwdinfo = &padapter->wdinfo;
#endif /* CONFIG_P2P */

	DBG_8192C("%s\n", __func__);

	param->u.crypt.err = 0;
	param->u.crypt.alg[IEEE_CRYPT_ALG_NAME_LEN - 1] = '\0';

	if (param_len < (u32) ((u8 *) param->u.crypt.key - (u8 *) param) + param->u.crypt.key_len)
	{
		ret =  -EINVAL;
		goto exit;
	}

	if (param->sta_addr[0] == 0xff && param->sta_addr[1] == 0xff &&
	    param->sta_addr[2] == 0xff && param->sta_addr[3] == 0xff &&
	    param->sta_addr[4] == 0xff && param->sta_addr[5] == 0xff)
	{
		if (param->u.crypt.idx >= WEP_KEYS)
		{
			ret = -EINVAL;
			goto exit;
		}
	} else {
		ret = -EINVAL;
		goto exit;
	}

	if (strcmp(param->u.crypt.alg, "WEP") == 0)
	{
		RT_TRACE(_module_rtl871x_ioctl_os_c,_drv_err_,("wpa_set_encryption, crypt.alg = WEP\n"));
		DBG_8192C("wpa_set_encryption, crypt.alg = WEP\n");

		wep_key_idx = param->u.crypt.idx;
		wep_key_len = param->u.crypt.key_len;

		if ((wep_key_idx > WEP_KEYS) || (wep_key_len <= 0))
		{
			ret = -EINVAL;
			goto exit;
		}

		if (psecuritypriv->bWepDefaultKeyIdxSet == 0)
		{
			/* wep default key has not been set, so use this key index as default key. */

			wep_key_len = wep_key_len <= 5 ? 5 : 13;

		psecuritypriv->ndisencryptstatus = Ndis802_11Encryption1Enabled;
			psecuritypriv->dot11PrivacyAlgrthm = _WEP40_;
			psecuritypriv->dot118021XGrpPrivacy = _WEP40_;

			if (wep_key_len==13)
			{
				psecuritypriv->dot11PrivacyAlgrthm = _WEP104_;
				psecuritypriv->dot118021XGrpPrivacy = _WEP104_;
			}

			psecuritypriv->dot11PrivacyKeyIndex = wep_key_idx;
		}

		memcpy(&(psecuritypriv->dot11DefKey[wep_key_idx].skey[0]), param->u.crypt.key, wep_key_len);

		psecuritypriv->dot11DefKeylen[wep_key_idx] = wep_key_len;

		rtw_set_key(padapter, psecuritypriv, wep_key_idx, 0);

		goto exit;
	}

	if (padapter->securitypriv.dot11AuthAlgrthm == dot11AuthAlgrthm_8021X) /*  802_1x */
	{
		struct sta_info * psta,*pbcmc_sta;
		struct sta_priv * pstapriv = &padapter->stapriv;

		if (check_fwstate(pmlmepriv, WIFI_STATION_STATE | WIFI_MP_STATE) == true) /* sta mode */
		{
			psta = rtw_get_stainfo(pstapriv, get_bssid(pmlmepriv));
			if (psta == NULL) {
				DBG_8192C("%s, : Obtain Sta_info fail\n", __func__);
			}
			else
			{
				/* Jeff: don't disable ieee8021x_blocked while clearing key */
				if (strcmp(param->u.crypt.alg, "none") != 0)
					psta->ieee8021x_blocked = false;

				if ((padapter->securitypriv.ndisencryptstatus == Ndis802_11Encryption2Enabled)||
						(padapter->securitypriv.ndisencryptstatus ==  Ndis802_11Encryption3Enabled))
				{
					psta->dot118021XPrivacy = padapter->securitypriv.dot11PrivacyAlgrthm;
				}

				if (param->u.crypt.set_tx ==1)/* pairwise key */
				{

					DBG_8192C("%s, : param->u.crypt.set_tx ==1\n", __func__);

					memcpy(psta->dot118021x_UncstKey.skey,  param->u.crypt.key, (param->u.crypt.key_len>16 ?16:param->u.crypt.key_len));

					if (strcmp(param->u.crypt.alg, "TKIP") == 0)/* set mic key */
					{
						memcpy(psta->dot11tkiptxmickey.skey, &(param->u.crypt.key[16]), 8);
						memcpy(psta->dot11tkiprxmickey.skey, &(param->u.crypt.key[24]), 8);

						padapter->securitypriv.busetkipkey=false;
					}

					DBG_871X(" ~~~~set sta key:unicastkey\n");

					rtw_setstakey_cmd(padapter, (unsigned char *)psta, true);
				}
				else/* group key */
				{
					memcpy(padapter->securitypriv.dot118021XGrpKey[param->u.crypt.idx].skey,  param->u.crypt.key,(param->u.crypt.key_len>16 ?16:param->u.crypt.key_len));
					memcpy(padapter->securitypriv.dot118021XGrptxmickey[param->u.crypt.idx].skey,&(param->u.crypt.key[16]),8);
					memcpy(padapter->securitypriv.dot118021XGrprxmickey[param->u.crypt.idx].skey,&(param->u.crypt.key[24]),8);
                                        padapter->securitypriv.binstallGrpkey = true;
					DBG_871X(" ~~~~set sta key:groupkey\n");

					padapter->securitypriv.dot118021XGrpKeyid = param->u.crypt.idx;

					rtw_set_key(padapter,&padapter->securitypriv,param->u.crypt.idx, 1);
#ifdef CONFIG_P2P
					if (rtw_p2p_chk_state(pwdinfo, P2P_STATE_PROVISIONING_ING))
					{
						rtw_p2p_set_state(pwdinfo, P2P_STATE_PROVISIONING_DONE);
					}
#endif /* CONFIG_P2P */

				}
			}

			pbcmc_sta=rtw_get_bcmc_stainfo(padapter);
			if (pbcmc_sta==NULL)
			{
			}
			else
			{
				/* Jeff: don't disable ieee8021x_blocked while clearing key */
				if (strcmp(param->u.crypt.alg, "none") != 0)
					pbcmc_sta->ieee8021x_blocked = false;

				if ((padapter->securitypriv.ndisencryptstatus == Ndis802_11Encryption2Enabled)||
						(padapter->securitypriv.ndisencryptstatus ==  Ndis802_11Encryption3Enabled))
				{
					pbcmc_sta->dot118021XPrivacy = padapter->securitypriv.dot11PrivacyAlgrthm;
				}
			}
		}
		else if (check_fwstate(pmlmepriv, WIFI_ADHOC_STATE)) /* adhoc mode */
		{
		}
	}

exit:

	DBG_8192C("%s, ret=%d\n", __func__, ret);

	return ret;
}

static int cfg80211_rtw_add_key(struct wiphy *wiphy, struct net_device *ndev,
				u8 key_index, bool pairwise, const u8 *mac_addr,
				struct key_params *params)
{
	char *alg_name;
	u32 param_len;
	struct ieee_param *param = NULL;
	int ret=0;
	struct wireless_dev *rtw_wdev = wiphy_to_wdev(wiphy);
	_adapter *padapter = wiphy_to_adapter(wiphy);
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;

	DBG_871X(FUNC_NDEV_FMT" adding key for %pM\n", FUNC_NDEV_ARG(ndev), mac_addr);
	DBG_871X("cipher=0x%x\n", params->cipher);
	DBG_871X("key_len=0x%x\n", params->key_len);
	DBG_871X("seq_len=0x%x\n", params->seq_len);
	DBG_871X("key_index=%d\n", key_index);
	DBG_871X("pairwise=%d\n", pairwise);

	param_len = sizeof(struct ieee_param) + params->key_len;
	param = (struct ieee_param *)kmalloc(param_len, GFP_KERNEL);
	if (param == NULL)
		return -1;

	memset(param, 0, param_len);

	param->cmd = IEEE_CMD_SET_ENCRYPTION;
	memset(param->sta_addr, 0xff, ETH_ALEN);

	switch (params->cipher) {
	case IW_AUTH_CIPHER_NONE:
		/* todo: remove key */
		/* remove = 1; */
		alg_name = "none";
		break;
	case WLAN_CIPHER_SUITE_WEP40:
	case WLAN_CIPHER_SUITE_WEP104:
		alg_name = "WEP";
		break;
	case WLAN_CIPHER_SUITE_TKIP:
		alg_name = "TKIP";
		break;
	case WLAN_CIPHER_SUITE_CCMP:
		alg_name = "CCMP";
		break;
	default:
		return -ENOTSUPP;
	}

	strncpy((char *)param->u.crypt.alg, alg_name, IEEE_CRYPT_ALG_NAME_LEN);

	if (!mac_addr || is_broadcast_ether_addr(mac_addr))
	{
		param->u.crypt.set_tx = 0; /* for wpa/wpa2 group key */
	} else {
		param->u.crypt.set_tx = 1; /* for wpa/wpa2 pairwise key */
	}

	param->u.crypt.idx = key_index;

	if (params->seq_len && params->seq)
	{
		memcpy(param->u.crypt.seq, params->seq, params->seq_len);
	}

	if (params->key_len && params->key)
	{
		param->u.crypt.key_len = params->key_len;
		memcpy(param->u.crypt.key, params->key, params->key_len);
	}

	if (check_fwstate(pmlmepriv, WIFI_STATION_STATE) == true)
	{
		ret =  rtw_cfg80211_set_encryption(ndev, param, param_len);
	}
	else if (check_fwstate(pmlmepriv, WIFI_AP_STATE) == true)
	{
#ifdef CONFIG_92D_AP_MODE
		if (mac_addr)
			memcpy(param->sta_addr, (void*)mac_addr, ETH_ALEN);

		ret = rtw_cfg80211_ap_set_encryption(ndev, param, param_len);
#endif
	} else {
		DBG_8192C("error! fw_state=0x%x, iftype=%d\n", pmlmepriv->fw_state, rtw_wdev->iftype);
	}

	if (param)
		kfree(param);

	return ret;
}

static int cfg80211_rtw_get_key(struct wiphy *wiphy, struct net_device *ndev,
				u8 key_index, bool pairwise, const u8 *mac_addr,
				void *cookie,
				void (*callback)(void *cookie,
						 struct key_params*))
{
	DBG_871X(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));
	return 0;
}

static int cfg80211_rtw_del_key(struct wiphy *wiphy, struct net_device *ndev,
				u8 key_index, bool pairwise, const u8 *mac_addr)
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct security_priv *psecuritypriv = &padapter->securitypriv;

	DBG_871X(FUNC_NDEV_FMT" key_index=%d\n", FUNC_NDEV_ARG(ndev), key_index);

	if (key_index == psecuritypriv->dot11PrivacyKeyIndex)
	{
		/* clear the flag of wep default key set. */
		psecuritypriv->bWepDefaultKeyIdxSet = 0;
	}

	return 0;
}

static int cfg80211_rtw_set_default_key(struct wiphy *wiphy,
					struct net_device *ndev, u8 key_index,
					bool unicast, bool multicast)
{
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct security_priv *psecuritypriv = &padapter->securitypriv;

	DBG_871X(FUNC_NDEV_FMT" key_index=%d, unicast=%d, multicast=%d.\n",
		 FUNC_NDEV_ARG(ndev), key_index, unicast, multicast);

	if ((key_index < WEP_KEYS) && ((psecuritypriv->dot11PrivacyAlgrthm == _WEP40_) || (psecuritypriv->dot11PrivacyAlgrthm == _WEP104_))) /* set wep default key */
	{
		psecuritypriv->ndisencryptstatus = Ndis802_11Encryption1Enabled;

		psecuritypriv->dot11PrivacyKeyIndex = key_index;

		psecuritypriv->dot11PrivacyAlgrthm = _WEP40_;
		psecuritypriv->dot118021XGrpPrivacy = _WEP40_;
		if (psecuritypriv->dot11DefKeylen[key_index] == 13)
		{
			psecuritypriv->dot11PrivacyAlgrthm = _WEP104_;
			psecuritypriv->dot118021XGrpPrivacy = _WEP104_;
		}

		psecuritypriv->bWepDefaultKeyIdxSet = 1; /* set the flag to represent that wep default key has been set */
	}

	return 0;
}

static int cfg80211_rtw_get_station(struct wiphy *wiphy,
				    struct net_device *ndev,
				    u8 *mac, struct station_info *sinfo)
{
	int ret = 0;
	_adapter *padapter = wiphy_to_adapter(wiphy);
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct sta_info *psta = NULL;
	struct sta_priv *pstapriv = &padapter->stapriv;

	sinfo->filled = 0;

	if (!mac) {
		DBG_871X(FUNC_NDEV_FMT" mac==%p\n", FUNC_NDEV_ARG(ndev), mac);
		ret = -ENOENT;
		goto exit;
	}

	psta = rtw_get_stainfo(pstapriv, mac);
	if (psta == NULL) {
		DBG_8192C("%s, sta_info is null\n", __func__);
		ret = -ENOENT;
		goto exit;
	}

#ifdef CONFIG_DEBUG_CFG80211
	DBG_871X(FUNC_NDEV_FMT" mac=%pM\n", FUNC_NDEV_ARG(ndev), mac);
#endif

	/* for infra./P2PClient mode */
	if (	check_fwstate(pmlmepriv, WIFI_STATION_STATE)
		&& check_fwstate(pmlmepriv, _FW_LINKED)
	)
	{
		struct wlan_network  *cur_network = &(pmlmepriv->cur_network);

		if (_rtw_memcmp(mac, cur_network->network.MacAddress, ETH_ALEN) == false) {
			DBG_871X("%s, mismatch bssid=%pM\n", __func__, cur_network->network.MacAddress);
			ret = -ENOENT;
			goto exit;
		}

		sinfo->filled |= STATION_INFO_SIGNAL;
		sinfo->signal = translate_percentage_to_dbm(padapter->recvpriv.signal_strength);

		sinfo->filled |= STATION_INFO_TX_BITRATE;
		sinfo->txrate.legacy = rtw_get_cur_max_rate(padapter);

		sinfo->filled |= STATION_INFO_RX_PACKETS;
		sinfo->rx_packets = sta_rx_data_pkts(psta);

		sinfo->filled |= STATION_INFO_TX_PACKETS;
		sinfo->tx_packets = psta->sta_stats.tx_pkts;

	}

	/* for Ad-Hoc/AP mode */
	if ((check_fwstate(pmlmepriv, WIFI_ADHOC_STATE)
			||check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE)
			||check_fwstate(pmlmepriv, WIFI_AP_STATE))
		&& check_fwstate(pmlmepriv, _FW_LINKED)
	)
	{
		/* TODO: should acquire station info... */
	}

exit:
	return ret;
}

extern int netdev_open(struct net_device *pnetdev);
#ifdef CONFIG_CONCURRENT_MODE
extern int netdev_if2_open(struct net_device *pnetdev);
#endif

static int cfg80211_rtw_change_iface(struct wiphy *wiphy,
				     struct net_device *ndev,
				     enum nl80211_iftype type, u32 *flags,
				     struct vif_params *params)
{
	enum nl80211_iftype old_type;
	NDIS_802_11_NETWORK_INFRASTRUCTURE networkType ;
	_adapter *padapter = wiphy_to_adapter(wiphy);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct wireless_dev *rtw_wdev = wiphy_to_wdev(wiphy);
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	_queue *queue = &pmlmepriv->scanned_queue;
#ifdef CONFIG_P2P
	struct wifidirect_info *pwdinfo= &(padapter->wdinfo);
#endif
	int ret = 0;
	u8 change = false;

#ifdef CONFIG_CONCURRENT_MODE
	if (padapter->adapter_type == SECONDARY_ADAPTER)
	{
		DBG_871X(FUNC_NDEV_FMT" call netdev_if2_open\n", FUNC_NDEV_ARG(ndev));
		if (netdev_if2_open(ndev) != 0) {
			ret= -EPERM;
			goto exit;
		}
	}
	else if (padapter->adapter_type == PRIMARY_ADAPTER)
#endif /* CONFIG_CONCURRENT_MODE */
	{
		DBG_871X(FUNC_NDEV_FMT" call netdev_open\n", FUNC_NDEV_ARG(ndev));
		if (netdev_open(ndev) != 0) {
			ret= -EPERM;
			goto exit;
		}
	}

	if (_FAIL == rtw_pwr_wakeup(padapter)) {
		ret= -EPERM;
		goto exit;
	}

	old_type = rtw_wdev->iftype;
	DBG_871X(FUNC_NDEV_FMT" old_iftype=%d, new_iftype=%d\n",
		FUNC_NDEV_ARG(ndev), old_type, type);

	if (old_type != type)
	{
		change = true;
		pmlmeext->action_public_rxseq = 0xffff;
		pmlmeext->action_public_dialog_token = 0xff;
	}

	switch (type) {
	case NL80211_IFTYPE_ADHOC:
		networkType = Ndis802_11IBSS;
		break;
	case NL80211_IFTYPE_P2P_CLIENT:
	case NL80211_IFTYPE_STATION:
		networkType = Ndis802_11Infrastructure;
		#ifdef CONFIG_P2P
		if (change && rtw_p2p_chk_role(pwdinfo, P2P_ROLE_GO))
		{
			_cancel_timer_ex(&pwdinfo->find_phase_timer);
			_cancel_timer_ex(&pwdinfo->restore_p2p_state_timer);
			_cancel_timer_ex(&pwdinfo->pre_tx_scan_timer);

			/* it means remove GO and change mode from AP(GO) to station(P2P DEVICE) */
			rtw_p2p_set_role(pwdinfo, P2P_ROLE_DEVICE);
			rtw_p2p_set_state(pwdinfo, rtw_p2p_pre_state(pwdinfo));

			DBG_8192C("%s, role=%d, p2p_state=%d, pre_p2p_state=%d\n", __func__, rtw_p2p_role(pwdinfo), rtw_p2p_state(pwdinfo), rtw_p2p_pre_state(pwdinfo));
		}
		#endif /* CONFIG_P2P */
		break;
	case NL80211_IFTYPE_P2P_GO:
	case NL80211_IFTYPE_AP:
		networkType = Ndis802_11APMode;
		#ifdef CONFIG_P2P
		if (change && !rtw_p2p_chk_state(pwdinfo, P2P_STATE_NONE))
		{
			/* it means P2P Group created, we will be GO and change mode from  P2P DEVICE to AP(GO) */
			rtw_p2p_set_role(pwdinfo, P2P_ROLE_GO);
		}
		#endif /* CONFIG_P2P */
		break;
	default:
		return -EOPNOTSUPP;
	}

	rtw_wdev->iftype = type;

	spin_lock_bh(&pmlmepriv->lock);
	spin_lock_bh(&queue->lock);

	if (rtw_set_802_11_infrastructure_mode(padapter, networkType) ==false)
	{
		rtw_wdev->iftype = old_type;
		ret = -EPERM;
		spin_unlock_bh(&queue->lock);
		spin_unlock_bh(&pmlmepriv->lock);
		goto exit;
	}
	spin_unlock_bh(&queue->lock);
	spin_unlock_bh(&pmlmepriv->lock);

	rtw_setopmode_cmd(padapter, networkType);

exit:

	return ret;
}

void rtw_cfg80211_indicate_scan_done(struct rtw_wdev_priv *pwdev_priv, bool aborted)
{

	spin_lock_bh(&pwdev_priv->scan_req_lock);
	if (pwdev_priv->scan_request != NULL)
	{
		#ifdef CONFIG_DEBUG_CFG80211
		DBG_871X("%s with scan req\n", __func__);
		#endif

		/* avoid WARN_ON(request != wiphy_to_dev(request->wiphy)->scan_req); */
		if (pwdev_priv->scan_request->wiphy != pwdev_priv->rtw_wdev->wiphy)
		{
			DBG_8192C("error wiphy compare\n");
		}
		else
		{
			cfg80211_scan_done(pwdev_priv->scan_request, aborted);
		}

		pwdev_priv->scan_request = NULL;

	} else {
		#ifdef CONFIG_DEBUG_CFG80211
		DBG_871X("%s without scan req\n", __func__);
		#endif
	}
	spin_unlock_bh(&pwdev_priv->scan_req_lock);
}

void rtw_cfg80211_surveydone_event_callback(_adapter *padapter)
{
	_list					*plist, *phead;
	struct	mlme_priv	*pmlmepriv = &(padapter->mlmepriv);
	_queue				*queue	= &(pmlmepriv->scanned_queue);
	struct	wlan_network	*pnetwork = NULL;
	u32 cnt=0;
	u32 wait_for_surveydone;
	int wait_status;
#ifdef CONFIG_P2P
	struct	wifidirect_info*	pwdinfo = &padapter->wdinfo;
#endif /* CONFIG_P2P */
	struct rtw_wdev_priv *pwdev_priv = wdev_to_priv(padapter->rtw_wdev);
	struct pwrctrl_priv *pwrpriv = &padapter->pwrctrlpriv;

#ifdef CONFIG_DEBUG_CFG80211
	DBG_8192C("%s\n", __func__);
#endif

	spin_lock_bh(&(pmlmepriv->scanned_queue.lock));

	phead = get_list_head(queue);
	plist = phead->next;

	while (1)
	{
		if (rtw_end_of_queue_search(phead,plist)== true)
			break;

		pnetwork = container_of(plist, struct wlan_network, list);

		/* report network only if the current channel set contains the channel to which this network belongs */
		if (rtw_ch_set_search_ch(padapter->mlmeextpriv.channel_set, pnetwork->network.Configuration.DSConfig) >= 0
			#ifdef CONFIG_VALIDATE_SSID
			&& true == rtw_validate_ssid(&(pnetwork->network.Ssid))
			#endif
		)
		{
			/* ev=translate_scan(padapter, a, pnetwork, ev, stop); */
			rtw_cfg80211_inform_bss(padapter, pnetwork);
		}

		plist = plist->next;

	}

	spin_unlock_bh(&(pmlmepriv->scanned_queue.lock));

	/* call this after other things have been done */
	rtw_cfg80211_indicate_scan_done(wdev_to_priv(padapter->rtw_wdev), false);
}

static int rtw_cfg80211_set_probe_req_wpsp2pie(_adapter *padapter, char *buf, int len)
{
	int ret = 0;
	uint wps_ielen = 0;
	u8 *wps_ie;
	u32	p2p_ielen = 0;
	u8 *p2p_ie;
	u32	wfd_ielen = 0;
	u8 *wfd_ie;
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);

#ifdef CONFIG_DEBUG_CFG80211
	DBG_8192C("%s, ielen=%d\n", __func__, len);
#endif

	if (len>0)
	{
		if ((wps_ie = rtw_get_wps_ie(buf, len, NULL, &wps_ielen)))
		{
			#ifdef CONFIG_DEBUG_CFG80211
			DBG_8192C("probe_req_wps_ielen=%d\n", wps_ielen);
			#endif

			if (pmlmepriv->wps_probe_req_ie)
			{
				u32 free_len = pmlmepriv->wps_probe_req_ie_len;
				pmlmepriv->wps_probe_req_ie_len = 0;
				kfree(pmlmepriv->wps_probe_req_ie);
				pmlmepriv->wps_probe_req_ie = NULL;
			}

			pmlmepriv->wps_probe_req_ie = kmalloc(wps_ielen, GFP_KERNEL);
			if (pmlmepriv->wps_probe_req_ie == NULL) {
				DBG_8192C("%s()-%d: kmalloc() ERROR!\n", __func__, __LINE__);
				return -EINVAL;

			}
			memcpy(pmlmepriv->wps_probe_req_ie, wps_ie, wps_ielen);
			pmlmepriv->wps_probe_req_ie_len = wps_ielen;
		}

		#ifdef CONFIG_P2P
		if ((p2p_ie=rtw_get_p2p_ie(buf, len, NULL, &p2p_ielen)))
		{
			#ifdef CONFIG_DEBUG_CFG80211
			DBG_8192C("probe_req_p2p_ielen=%d\n", p2p_ielen);
			#endif

			if (pmlmepriv->p2p_probe_req_ie)
			{
				u32 free_len = pmlmepriv->p2p_probe_req_ie_len;
				pmlmepriv->p2p_probe_req_ie_len = 0;
				kfree(pmlmepriv->p2p_probe_req_ie);
				pmlmepriv->p2p_probe_req_ie = NULL;
			}

			pmlmepriv->p2p_probe_req_ie = kmalloc(p2p_ielen, GFP_KERNEL);
			if (pmlmepriv->p2p_probe_req_ie == NULL) {
				DBG_8192C("%s()-%d: kmalloc() ERROR!\n", __func__, __LINE__);
				return -ENOMEM;

			}
			memcpy(pmlmepriv->p2p_probe_req_ie, p2p_ie, p2p_ielen);
			pmlmepriv->p2p_probe_req_ie_len = p2p_ielen;
		}
		#endif /* CONFIG_P2P */

		#ifdef CONFIG_WFD
		if (rtw_get_wfd_ie(buf, len, NULL, &wfd_ielen))
		{
			#ifdef CONFIG_DEBUG_CFG80211
			DBG_8192C("probe_req_wfd_ielen=%d\n", wfd_ielen);
			#endif

			if (pmlmepriv->wfd_probe_req_ie)
			{
				u32 free_len = pmlmepriv->wfd_probe_req_ie_len;
				pmlmepriv->wfd_probe_req_ie_len = 0;
				kfree(pmlmepriv->wfd_probe_req_ie);
				pmlmepriv->wfd_probe_req_ie = NULL;
			}

			pmlmepriv->wfd_probe_req_ie = kmalloc(wfd_ielen, GFP_KERNEL);
			if (pmlmepriv->wfd_probe_req_ie == NULL) {
				DBG_8192C("%s()-%d: kmalloc() ERROR!\n", __func__, __LINE__);
				return -EINVAL;

			}
			rtw_get_wfd_ie(buf, len, pmlmepriv->wfd_probe_req_ie, &pmlmepriv->wfd_probe_req_ie_len);
		}
		#endif /* CONFIG_WFD */

	}

	return ret;
}

static int cfg80211_rtw_scan(struct wiphy *wiphy,
			     struct cfg80211_scan_request *request)
{
	int i;
	u8 _status = false;
	int ret = 0;
	_adapter *padapter = wiphy_to_adapter(wiphy);
	struct mlme_priv *pmlmepriv= &padapter->mlmepriv;
	struct ndis_802_11_ssid ssid[RTW_SSID_SCAN_AMOUNT];
	struct rtw_ieee80211_channel ch[RTW_CHANNEL_SCAN_AMOUNT];
	u8 *wps_ie=NULL;
	uint wps_ielen=0;
	u8 *p2p_ie=NULL;
	uint p2p_ielen=0;
#ifdef CONFIG_P2P
	struct wifidirect_info *pwdinfo= &(padapter->wdinfo);
#endif /* CONFIG_P2P */
	struct rtw_wdev_priv *pwdev_priv = wdev_to_priv(padapter->rtw_wdev);
	struct cfg80211_ssid *ssids = request->ssids;
	int social_channel = 0, j = 0;
	bool need_indicate_scan_done = false;
#ifdef CONFIG_CONCURRENT_MODE
	PADAPTER pbuddy_adapter = NULL;
	struct mlme_priv *pbuddy_mlmepriv = NULL;
#endif /* CONFIG_CONCURRENT_MODE */

#ifdef CONFIG_DEBUG_CFG80211
	DBG_871X(FUNC_ADPT_FMT"\n", FUNC_ADPT_ARG(padapter));
#endif

#ifdef CONFIG_CONCURRENT_MODE
	if (rtw_buddy_adapter_up(padapter))
	{
		pbuddy_adapter = padapter->pbuddy_adapter;
		pbuddy_mlmepriv = &(pbuddy_adapter->mlmepriv);
	}
#endif /* CONFIG_CONCURRENT_MODE */

	spin_lock_bh(&pwdev_priv->scan_req_lock);
	pwdev_priv->scan_request = request;
	spin_unlock_bh(&pwdev_priv->scan_req_lock);

	if (check_fwstate(pmlmepriv, WIFI_AP_STATE) == true)
	{

#ifdef CONFIG_DEBUG_CFG80211
		DBG_871X("%s under WIFI_AP_STATE\n", __func__);
#endif
	}

	if (_FAIL == rtw_pwr_wakeup(padapter)) {
		need_indicate_scan_done = true;
		goto check_need_indicate_scan_done;
	}

	#ifdef CONFIG_P2P
	if (ssids->ssid != NULL
		&& _rtw_memcmp(ssids->ssid, "DIRECT-", 7)
		&& rtw_get_p2p_ie((u8 *)request->ie, request->ie_len, NULL, NULL)
	)
	{
		if (rtw_p2p_chk_state(pwdinfo, P2P_STATE_NONE))
		{
			rtw_p2p_enable(padapter, P2P_ROLE_DEVICE);
			wdev_to_priv(padapter->rtw_wdev)->p2p_enabled = true;
		}
		else
		{
			rtw_p2p_set_pre_state(pwdinfo, rtw_p2p_state(pwdinfo));
			#ifdef CONFIG_DEBUG_CFG80211
			DBG_8192C("%s, role=%d, p2p_state=%d\n", __func__, rtw_p2p_role(pwdinfo), rtw_p2p_state(pwdinfo));
			#endif
		}
		rtw_p2p_set_state(pwdinfo, P2P_STATE_LISTEN);

		if (request->n_channels == 3 &&
			request->channels[0]->hw_value == 1 &&
			request->channels[1]->hw_value == 6 &&
			request->channels[2]->hw_value == 11
		)
		{
			social_channel = 1;
		}
	}
	#endif /* CONFIG_P2P */

	if (request->ie && request->ie_len>0)
	{
		rtw_cfg80211_set_probe_req_wpsp2pie(padapter, (u8 *)request->ie, request->ie_len);
	}

	if (pmlmepriv->LinkDetectInfo.bBusyTraffic == true)
	{
		DBG_8192C("%s, bBusyTraffic == true\n", __func__);
		need_indicate_scan_done = true;
		goto check_need_indicate_scan_done;
	}
	if (rtw_is_scan_deny(padapter)) {
		DBG_871X(FUNC_ADPT_FMT  ": scan deny\n", FUNC_ADPT_ARG(padapter));
		need_indicate_scan_done = true;
		goto check_need_indicate_scan_done;
	}

#ifdef CONFIG_CONCURRENT_MODE
	if (pbuddy_mlmepriv && (pbuddy_mlmepriv->LinkDetectInfo.bBusyTraffic == true))
	{
		DBG_8192C("%s, bBusyTraffic == true at buddy_intf\n", __func__);
		need_indicate_scan_done = true;
		goto check_need_indicate_scan_done;
	}
#endif /* CONFIG_CONCURRENT_MODE */

	if (check_fwstate(pmlmepriv, _FW_UNDER_SURVEY|_FW_UNDER_LINKING) == true)
	{
		DBG_8192C("%s, fwstate=0x%x\n", __func__, pmlmepriv->fw_state);
		need_indicate_scan_done = true;
		goto check_need_indicate_scan_done;
	}

#ifdef CONFIG_CONCURRENT_MODE
	if (check_buddy_fwstate(padapter,
		_FW_UNDER_SURVEY|_FW_UNDER_LINKING|WIFI_UNDER_WPS) == true)
	{
		if (check_buddy_fwstate(padapter, _FW_UNDER_SURVEY))
		{
			DBG_8192C("scanning_via_buddy_intf\n");
			pmlmepriv->scanning_via_buddy_intf = true;
		}

		DBG_8192C("buddy_intf's mlme state:0x%x\n", pbuddy_mlmepriv->fw_state);

		need_indicate_scan_done = true;
		goto check_need_indicate_scan_done;
	}
#endif

#ifdef CONFIG_P2P
	if (!rtw_p2p_chk_state(pwdinfo, P2P_STATE_NONE) && !rtw_p2p_chk_state(pwdinfo, P2P_STATE_IDLE))
	{
		rtw_p2p_set_state(pwdinfo, P2P_STATE_FIND_PHASE_SEARCH);
		rtw_free_network_queue(padapter, true);

		if (social_channel == 0)
			rtw_p2p_findphase_ex_set(pwdinfo, P2P_FINDPHASE_EX_NONE);
		else
			rtw_p2p_findphase_ex_set(pwdinfo, P2P_FINDPHASE_EX_SOCIAL_LAST);
	}
#endif /* CONFIG_P2P */

	memset(ssid, 0, sizeof(struct ndis_802_11_ssid)*RTW_SSID_SCAN_AMOUNT);
	/* parsing request ssids, n_ssids */
	for (i = 0; i < request->n_ssids && i < RTW_SSID_SCAN_AMOUNT; i++) {
		#ifdef CONFIG_DEBUG_CFG80211
		DBG_8192C("ssid=%s, len=%d\n", ssids[i].ssid, ssids[i].ssid_len);
		#endif
		memcpy(ssid[i].Ssid, ssids[i].ssid, ssids[i].ssid_len);
		ssid[i].SsidLength = ssids[i].ssid_len;
	}

	/* parsing channels, n_channels */
	memset(ch, 0, sizeof(struct rtw_ieee80211_channel)*RTW_CHANNEL_SCAN_AMOUNT);
	if (request->n_channels == 1)
	for (i=0;i<request->n_channels && i<RTW_CHANNEL_SCAN_AMOUNT;i++) {
		#ifdef CONFIG_DEBUG_CFG80211
		DBG_871X(FUNC_ADPT_FMT CHAN_FMT"\n", FUNC_ADPT_ARG(padapter), CHAN_ARG(request->channels[i]));
		#endif
		ch[i].hw_value = request->channels[i]->hw_value;
		ch[i].flags = request->channels[i]->flags;
	}

	spin_lock_bh(&pmlmepriv->lock);
	if (request->n_channels == 1) {
		memcpy(&ch[1], &ch[0], sizeof(struct rtw_ieee80211_channel));
		memcpy(&ch[2], &ch[0], sizeof(struct rtw_ieee80211_channel));
		_status = rtw_sitesurvey_cmd(padapter, ssid, RTW_SSID_SCAN_AMOUNT, ch, 3);
	} else {
		_status = rtw_sitesurvey_cmd(padapter, ssid, RTW_SSID_SCAN_AMOUNT, NULL, 0);
	}
	spin_unlock_bh(&pmlmepriv->lock);

	if (_status == false)
	{
		ret = -1;
	}

check_need_indicate_scan_done:
	if (need_indicate_scan_done)
		rtw_cfg80211_surveydone_event_callback(padapter);

exit:

	return ret;
}

static int cfg80211_rtw_set_wiphy_params(struct wiphy *wiphy, u32 changed)
{
	DBG_8192C("%s\n", __func__);
	return 0;
}

static int cfg80211_rtw_join_ibss(struct wiphy *wiphy, struct net_device *ndev,
				  struct cfg80211_ibss_params *params)
{
	DBG_871X(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));
	return 0;
}

static int cfg80211_rtw_leave_ibss(struct wiphy *wiphy, struct net_device *ndev)
{
	DBG_871X(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));
	return 0;
}

static int rtw_cfg80211_set_wpa_version(struct security_priv *psecuritypriv, u32 wpa_version)
{
	DBG_8192C("%s, wpa_version=%d\n", __func__, wpa_version);

	if (!wpa_version) {
		psecuritypriv->ndisauthtype = Ndis802_11AuthModeOpen;
		return 0;
	}

	if (wpa_version & (NL80211_WPA_VERSION_1 | NL80211_WPA_VERSION_2))
	{
		psecuritypriv->ndisauthtype = Ndis802_11AuthModeWPAPSK;
	}

/*
	if (wpa_version & NL80211_WPA_VERSION_2)
	{
		psecuritypriv->ndisauthtype = Ndis802_11AuthModeWPA2PSK;
	}
*/

	return 0;
}

static int rtw_cfg80211_set_auth_type(struct security_priv *psecuritypriv,
			     enum nl80211_auth_type sme_auth_type)
{
	DBG_8192C("%s, nl80211_auth_type=%d\n", __func__, sme_auth_type);

	switch (sme_auth_type) {
	case NL80211_AUTHTYPE_AUTOMATIC:

		psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_Auto;

		break;
	case NL80211_AUTHTYPE_OPEN_SYSTEM:

		psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_Open;

		if (psecuritypriv->ndisauthtype>Ndis802_11AuthModeWPA)
			psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_8021X;

		break;
	case NL80211_AUTHTYPE_SHARED_KEY:

		psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_Shared;

		psecuritypriv->ndisencryptstatus = Ndis802_11Encryption1Enabled;

		break;
	default:
		psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_Open;
		/* return -ENOTSUPP; */
	}

	return 0;
}

static int rtw_cfg80211_set_cipher(struct security_priv *psecuritypriv, u32 cipher, bool ucast)
{
	u32 ndisencryptstatus = Ndis802_11EncryptionDisabled;

	u32 *profile_cipher = ucast ? &psecuritypriv->dot11PrivacyAlgrthm :
		&psecuritypriv->dot118021XGrpPrivacy;

	DBG_8192C("%s, ucast=%d, cipher=0x%x\n", __func__, ucast, cipher);

	if (!cipher) {
		*profile_cipher = _NO_PRIVACY_;
		psecuritypriv->ndisencryptstatus = ndisencryptstatus;
		return 0;
	}

	switch (cipher) {
	case IW_AUTH_CIPHER_NONE:
		*profile_cipher = _NO_PRIVACY_;
		ndisencryptstatus = Ndis802_11EncryptionDisabled;
		break;
	case WLAN_CIPHER_SUITE_WEP40:
		*profile_cipher = _WEP40_;
		ndisencryptstatus = Ndis802_11Encryption1Enabled;
		break;
	case WLAN_CIPHER_SUITE_WEP104:
		*profile_cipher = _WEP104_;
		ndisencryptstatus = Ndis802_11Encryption1Enabled;
		break;
	case WLAN_CIPHER_SUITE_TKIP:
		*profile_cipher = _TKIP_;
		ndisencryptstatus = Ndis802_11Encryption2Enabled;
		break;
	case WLAN_CIPHER_SUITE_CCMP:
		*profile_cipher = _AES_;
		ndisencryptstatus = Ndis802_11Encryption3Enabled;
		break;
	default:
		DBG_8192C("Unsupported cipher: 0x%x\n", cipher);
		return -ENOTSUPP;
	}

	if (ucast)
		psecuritypriv->ndisencryptstatus = ndisencryptstatus;

	return 0;
}

static int rtw_cfg80211_set_key_mgt(struct security_priv *psecuritypriv, u32 key_mgt)
{
	DBG_8192C("%s, key_mgt=0x%x\n", __func__, key_mgt);

	if (key_mgt == WLAN_AKM_SUITE_8021X)
		psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_8021X;
	else if (key_mgt == WLAN_AKM_SUITE_PSK) {
		psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_8021X;
	} else {
		DBG_8192C("Invalid key mgt: 0x%x\n", key_mgt);
	}

	return 0;
}

static int rtw_cfg80211_set_wpa_ie(_adapter *padapter, u8 *pie, size_t ielen)
{
	u8 *buf=NULL, *pos=NULL;
	u32 left;
	int group_cipher = 0, pairwise_cipher = 0;
	int ret = 0;
	int wpa_ielen=0;
	int wpa2_ielen=0;
	u8 *pwpa, *pwpa2;
	u8 null_addr[]= {0,0,0,0,0,0};

	if (pie == NULL || !ielen) {
		/* Treat this as normal case, but need to clear WIFI_UNDER_WPS */
		_clr_fwstate_(&padapter->mlmepriv, WIFI_UNDER_WPS);
		goto exit;
	}

	if (ielen > MAX_WPA_IE_LEN+MAX_WPS_IE_LEN+MAX_P2P_IE_LEN) {
		ret = -EINVAL;
		goto exit;
	}

	buf = kzalloc(ielen, GFP_KERNEL);
	if (buf == NULL) {
		ret =  -ENOMEM;
		goto exit;
	}

	memcpy(buf, pie , ielen);

	/* dump */
	{
		int i;
		DBG_8192C("set wpa_ie(length:%zu):\n", ielen);
		for (i=0;i<ielen;i=i+8)
			DBG_8192C("0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x\n",buf[i],buf[i+1],buf[i+2],buf[i+3],buf[i+4],buf[i+5],buf[i+6],buf[i+7]);
	}

	pos = buf;
	if (ielen < RSN_HEADER_LEN) {
		RT_TRACE(_module_rtl871x_ioctl_os_c,_drv_err_,("Ie len too short %d\n", ielen));
		ret  = -1;
		goto exit;
	}

	pwpa = rtw_get_wpa_ie(buf, &wpa_ielen, ielen);
	if (pwpa && wpa_ielen>0)
	{
		if (rtw_parse_wpa_ie(pwpa, wpa_ielen+2, &group_cipher, &pairwise_cipher) == _SUCCESS)
		{
			padapter->securitypriv.dot11AuthAlgrthm= dot11AuthAlgrthm_8021X;
			padapter->securitypriv.ndisauthtype=Ndis802_11AuthModeWPAPSK;
			memcpy(padapter->securitypriv.supplicant_ie, &pwpa[0], wpa_ielen+2);

			DBG_8192C("got wpa_ie, wpa_ielen:%u\n", wpa_ielen);
		}
	}

	pwpa2 = rtw_get_wpa2_ie(buf, &wpa2_ielen, ielen);
	if (pwpa2 && wpa2_ielen>0)
	{
		if (rtw_parse_wpa2_ie(pwpa2, wpa2_ielen+2, &group_cipher, &pairwise_cipher) == _SUCCESS)
		{
			padapter->securitypriv.dot11AuthAlgrthm= dot11AuthAlgrthm_8021X;
			padapter->securitypriv.ndisauthtype=Ndis802_11AuthModeWPA2PSK;
			memcpy(padapter->securitypriv.supplicant_ie, &pwpa2[0], wpa2_ielen+2);

			DBG_8192C("got wpa2_ie, wpa2_ielen:%u\n", wpa2_ielen);
		}
	}

	if (group_cipher == 0)
	{
		group_cipher = WPA_CIPHER_NONE;
	}
	if (pairwise_cipher == 0)
	{
		pairwise_cipher = WPA_CIPHER_NONE;
	}

	switch (group_cipher)
	{
		case WPA_CIPHER_NONE:
			padapter->securitypriv.dot118021XGrpPrivacy=_NO_PRIVACY_;
			padapter->securitypriv.ndisencryptstatus=Ndis802_11EncryptionDisabled;
			break;
		case WPA_CIPHER_WEP40:
			padapter->securitypriv.dot118021XGrpPrivacy=_WEP40_;
			padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption1Enabled;
			break;
		case WPA_CIPHER_TKIP:
			padapter->securitypriv.dot118021XGrpPrivacy=_TKIP_;
			padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption2Enabled;
			break;
		case WPA_CIPHER_CCMP:
			padapter->securitypriv.dot118021XGrpPrivacy=_AES_;
			padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption3Enabled;
			break;
		case WPA_CIPHER_WEP104:
			padapter->securitypriv.dot118021XGrpPrivacy=_WEP104_;
			padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption1Enabled;
			break;
	}

	switch (pairwise_cipher)
	{
		case WPA_CIPHER_NONE:
			padapter->securitypriv.dot11PrivacyAlgrthm=_NO_PRIVACY_;
			padapter->securitypriv.ndisencryptstatus=Ndis802_11EncryptionDisabled;
			break;
		case WPA_CIPHER_WEP40:
			padapter->securitypriv.dot11PrivacyAlgrthm=_WEP40_;
			padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption1Enabled;
			break;
		case WPA_CIPHER_TKIP:
			padapter->securitypriv.dot11PrivacyAlgrthm=_TKIP_;
			padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption2Enabled;
			break;
		case WPA_CIPHER_CCMP:
			padapter->securitypriv.dot11PrivacyAlgrthm=_AES_;
			padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption3Enabled;
			break;
		case WPA_CIPHER_WEP104:
			padapter->securitypriv.dot11PrivacyAlgrthm=_WEP104_;
			padapter->securitypriv.ndisencryptstatus = Ndis802_11Encryption1Enabled;
			break;
	}

	{/* handle wps_ie */
		uint wps_ielen;
		u8 *wps_ie;

		wps_ie = rtw_get_wps_ie(buf, ielen, NULL, &wps_ielen);
		if (wps_ie && wps_ielen > 0) {
			DBG_8192C("got wps_ie, wps_ielen:%u\n", wps_ielen);
			padapter->securitypriv.wps_ie_len = wps_ielen<MAX_WPS_IE_LEN?wps_ielen:MAX_WPS_IE_LEN;
			memcpy(padapter->securitypriv.wps_ie, wps_ie, padapter->securitypriv.wps_ie_len);
			set_fwstate(&padapter->mlmepriv, WIFI_UNDER_WPS);
		} else {
			_clr_fwstate_(&padapter->mlmepriv, WIFI_UNDER_WPS);
		}
	}

	#ifdef CONFIG_P2P
	{/* check p2p_ie for assoc req; */
		uint p2p_ielen=0;
		u8 *p2p_ie;
		struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);

		if ((p2p_ie=rtw_get_p2p_ie(buf, ielen, NULL, &p2p_ielen)))
		{
			#ifdef CONFIG_DEBUG_CFG80211
			DBG_8192C("%s p2p_assoc_req_ielen=%d\n", __func__, p2p_ielen);
			#endif

			if (pmlmepriv->p2p_assoc_req_ie)
			{
				u32 free_len = pmlmepriv->p2p_assoc_req_ie_len;
				pmlmepriv->p2p_assoc_req_ie_len = 0;
				kfree(pmlmepriv->p2p_assoc_req_ie);
				pmlmepriv->p2p_assoc_req_ie = NULL;
			}

			pmlmepriv->p2p_assoc_req_ie = kmalloc(p2p_ielen, GFP_KERNEL);
			if (pmlmepriv->p2p_assoc_req_ie == NULL) {
				DBG_8192C("%s()-%d: kmalloc() ERROR!\n", __func__, __LINE__);
				goto exit;
			}
			memcpy(pmlmepriv->p2p_assoc_req_ie, p2p_ie, p2p_ielen);
			pmlmepriv->p2p_assoc_req_ie_len = p2p_ielen;
		}
	}
	#endif /* CONFIG_P2P */

	#ifdef CONFIG_WFD
	{/* check wfd_ie for assoc req; */
		uint wfd_ielen=0;
		u8 *wfd_ie;
		struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);

		if (rtw_get_wfd_ie(buf, ielen, NULL, &wfd_ielen))
		{
			#ifdef CONFIG_DEBUG_CFG80211
			DBG_8192C("%s wfd_assoc_req_ielen=%d\n", __func__, wfd_ielen);
			#endif

			if (pmlmepriv->wfd_assoc_req_ie)
			{
				u32 free_len = pmlmepriv->wfd_assoc_req_ie_len;
				pmlmepriv->wfd_assoc_req_ie_len = 0;
				kfree(pmlmepriv->wfd_assoc_req_ie);
				pmlmepriv->wfd_assoc_req_ie = NULL;
			}

			pmlmepriv->wfd_assoc_req_ie = kmalloc(wfd_ielen, GFP_KERNEL);
			if (pmlmepriv->wfd_assoc_req_ie == NULL) {
				DBG_8192C("%s()-%d: kmalloc() ERROR!\n", __func__, __LINE__);
				goto exit;
			}
			rtw_get_wfd_ie(buf, ielen, pmlmepriv->wfd_assoc_req_ie, &pmlmepriv->wfd_assoc_req_ie_len);
		}
	}
	#endif /* CONFIG_WFD */

	/* TKIP and AES disallow multicast packets until installing group key */
	if (padapter->securitypriv.dot11PrivacyAlgrthm == _TKIP_
		|| padapter->securitypriv.dot11PrivacyAlgrthm == _TKIP_WTMIC_
		|| padapter->securitypriv.dot11PrivacyAlgrthm == _AES_)
		/* WPS open need to enable multicast */
		rtw_hal_set_hwreg(padapter, HW_VAR_OFF_RCR_AM, null_addr);

	RT_TRACE(_module_rtl871x_ioctl_os_c, _drv_info_,
		("rtw_set_wpa_ie: pairwise_cipher=0x%08x padapter->securitypriv.ndisencryptstatus=%d padapter->securitypriv.ndisauthtype=%d\n",
		pairwise_cipher, padapter->securitypriv.ndisencryptstatus, padapter->securitypriv.ndisauthtype));

exit:
	if (buf)
		kfree(buf);
	if (ret)
		_clr_fwstate_(&padapter->mlmepriv, WIFI_UNDER_WPS);
	return ret;
}

static int cfg80211_rtw_connect(struct wiphy *wiphy, struct net_device *ndev,
				 struct cfg80211_connect_params *sme)
{
	int ret=0;
	_list *phead;
	struct wlan_network *pnetwork = NULL;
	NDIS_802_11_AUTHENTICATION_MODE authmode;
	struct ndis_802_11_ssid ndis_ssid;
	u8 *dst_ssid, *src_ssid;
	u8 *dst_bssid, *src_bssid;
	u8 matched=false;
	_adapter *padapter = wiphy_to_adapter(wiphy);
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct security_priv *psecuritypriv = &padapter->securitypriv;
	_queue *queue = &pmlmepriv->scanned_queue;

	DBG_871X("=>"FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));
	DBG_871X("privacy=%d, key=%p, key_len=%d, key_idx=%d\n",
		sme->privacy, sme->key, sme->key_len, sme->key_idx);

	if (wdev_to_priv(padapter->rtw_wdev)->block == true)
	{
		ret = -EBUSY;
		DBG_871X("%s wdev_priv.block is set\n", __func__);
		goto exit;
	}

	if (_FAIL == rtw_pwr_wakeup(padapter)) {
		ret= -EPERM;
		goto exit;
	}

	if (check_fwstate(pmlmepriv, WIFI_AP_STATE)) {
		ret = -EPERM;
		goto exit;
	}

#ifdef CONFIG_CONCURRENT_MODE
	if (check_buddy_fwstate(padapter, _FW_UNDER_LINKING) == true) {
		DBG_8192C("%s, but buddy_intf is under linking\n", __func__);
		ret = -EINVAL;
		goto exit;
	}
	if (check_buddy_fwstate(padapter, _FW_UNDER_SURVEY) == true) {
		rtw_scan_abort(padapter->pbuddy_adapter);
	}
#endif

	if (!sme->ssid || !sme->ssid_len)
	{
		ret = -EINVAL;
		goto exit;
	}

	if (sme->ssid_len > IW_ESSID_MAX_SIZE) {

		ret= -E2BIG;
		goto exit;
	}

	memset(&ndis_ssid, 0, sizeof(struct ndis_802_11_ssid));
	ndis_ssid.SsidLength = sme->ssid_len;
	memcpy(ndis_ssid.Ssid, sme->ssid, sme->ssid_len);

	DBG_8192C("ssid=%s, len=%zu\n", ndis_ssid.Ssid, sme->ssid_len);

	if (sme->bssid)
		DBG_8192C("bssid=%pM\n", sme->bssid);

	if (check_fwstate(pmlmepriv, _FW_UNDER_LINKING) == true) {
		ret = -EBUSY;
		DBG_8192C("%s, fw_state=0x%x, goto exit\n", __func__, pmlmepriv->fw_state);
		goto exit;
	}
	if (check_fwstate(pmlmepriv, _FW_UNDER_SURVEY) == true) {
		rtw_scan_abort(padapter);
	}

	spin_lock_bh(&pmlmepriv->lock);
	spin_lock_bh(&queue->lock);

	phead = get_list_head(queue);
	pmlmepriv->pscanned = phead->next;

	while (1)
	{
		if (rtw_end_of_queue_search(phead, pmlmepriv->pscanned) == true)
		{
			break;
		}

		pnetwork = container_of(pmlmepriv->pscanned, struct wlan_network, list);
		pmlmepriv->pscanned = pmlmepriv->pscanned->next;

		dst_ssid = pnetwork->network.Ssid.Ssid;
		dst_bssid = pnetwork->network.MacAddress;

		if (sme->bssid)  {
			if (_rtw_memcmp(pnetwork->network.MacAddress, sme->bssid, ETH_ALEN) == false)
				continue;
		}

		if (sme->ssid && sme->ssid_len) {
			if (	pnetwork->network.Ssid.SsidLength != sme->ssid_len
				|| _rtw_memcmp(pnetwork->network.Ssid.Ssid, sme->ssid, sme->ssid_len) == false
			)
				continue;
		}

		if (sme->bssid)
		{
			src_bssid = sme->bssid;

			if ((_rtw_memcmp(dst_bssid, src_bssid, ETH_ALEN)) == true)
			{
				DBG_8192C("matched by bssid\n");

				ndis_ssid.SsidLength = pnetwork->network.Ssid.SsidLength;
				memcpy(ndis_ssid.Ssid, pnetwork->network.Ssid.Ssid, pnetwork->network.Ssid.SsidLength);

				matched=true;
				break;
			}

		}
		else if (sme->ssid && sme->ssid_len)
		{
			src_ssid = ndis_ssid.Ssid;

			if ((_rtw_memcmp(dst_ssid, src_ssid, ndis_ssid.SsidLength) == true) &&
				(pnetwork->network.Ssid.SsidLength==ndis_ssid.SsidLength))
			{
				DBG_8192C("matched by ssid\n");
				matched=true;
				break;
			}
		}

	}

	if ((matched == false) || (pnetwork== NULL))
	{
		ret = -ENOENT;
		DBG_8192C("connect, matched == false, goto exit\n");
		spin_unlock_bh(&queue->lock);
		spin_unlock_bh(&pmlmepriv->lock);
		goto exit;
	}

	if (rtw_set_802_11_infrastructure_mode(padapter, pnetwork->network.InfrastructureMode) == false)
	{
		ret = -EPERM;
		spin_unlock_bh(&queue->lock);
		spin_unlock_bh(&pmlmepriv->lock);
		goto exit;
	}
	spin_unlock_bh(&queue->lock);
	spin_unlock_bh(&pmlmepriv->lock);

	psecuritypriv->ndisencryptstatus = Ndis802_11EncryptionDisabled;
	psecuritypriv->dot11PrivacyAlgrthm = _NO_PRIVACY_;
	psecuritypriv->dot118021XGrpPrivacy = _NO_PRIVACY_;
	psecuritypriv->dot11AuthAlgrthm = dot11AuthAlgrthm_Open; /* open system */
	psecuritypriv->ndisauthtype = Ndis802_11AuthModeOpen;

	ret = rtw_cfg80211_set_wpa_version(psecuritypriv, sme->crypto.wpa_versions);
	if (ret < 0)
		goto exit;

	ret = rtw_cfg80211_set_auth_type(psecuritypriv, sme->auth_type);
	if (ret < 0)
		goto exit;

	DBG_8192C("%s, ie_len=%zu\n", __func__, sme->ie_len);

	ret = rtw_cfg80211_set_wpa_ie(padapter, sme->ie, sme->ie_len);
	if (ret < 0)
		goto exit;

	if (sme->crypto.n_ciphers_pairwise) {
		ret = rtw_cfg80211_set_cipher(psecuritypriv, sme->crypto.ciphers_pairwise[0], true);
		if (ret < 0)
			goto exit;
	}

	/* For WEP Shared auth */
	if ((psecuritypriv->dot11AuthAlgrthm == dot11AuthAlgrthm_Shared
		|| psecuritypriv->dot11AuthAlgrthm == dot11AuthAlgrthm_Auto) && sme->key
	)
	{
		u32 wep_key_idx, wep_key_len,wep_total_len;
		NDIS_802_11_WEP	 *pwep = NULL;
		DBG_871X("%s(): Shared/Auto WEP\n",__func__);

		wep_key_idx = sme->key_idx;
		wep_key_len = sme->key_len;

		if (sme->key_idx > WEP_KEYS) {
			ret = -EINVAL;
			goto exit;
		}

		if (wep_key_len > 0)
		{
			wep_key_len = wep_key_len <= 5 ? 5 : 13;
			wep_total_len = wep_key_len + FIELD_OFFSET(NDIS_802_11_WEP, KeyMaterial);
			pwep =(NDIS_802_11_WEP	 *) kmalloc(wep_total_len, GFP_KERNEL);
			if (pwep == NULL) {
				DBG_871X(" wpa_set_encryption: pwep allocate fail !!!\n");
				ret = -ENOMEM;
				goto exit;
			}

			memset(pwep, 0, wep_total_len);

			pwep->KeyLength = wep_key_len;
			pwep->Length = wep_total_len;

			if (wep_key_len==13)
			{
				padapter->securitypriv.dot11PrivacyAlgrthm=_WEP104_;
				padapter->securitypriv.dot118021XGrpPrivacy=_WEP104_;
			}
		}
		else {
			ret = -EINVAL;
			goto exit;
		}

		pwep->KeyIndex = wep_key_idx;
		pwep->KeyIndex |= 0x80000000;

		memcpy(pwep->KeyMaterial,  (void *)sme->key, pwep->KeyLength);

		if (rtw_set_802_11_add_wep(padapter, pwep) == (u8)_FAIL)
			ret = -EOPNOTSUPP ;

		if (pwep)
			kfree((u8 *)pwep);

		if (ret < 0)
			goto exit;
	}

	ret = rtw_cfg80211_set_cipher(psecuritypriv, sme->crypto.cipher_group, false);
	if (ret < 0)
		return ret;

	if (sme->crypto.n_akm_suites) {
		ret = rtw_cfg80211_set_key_mgt(psecuritypriv, sme->crypto.akm_suites[0]);
		if (ret < 0)
			goto exit;
	}

	authmode = psecuritypriv->ndisauthtype;
	rtw_set_802_11_authentication_mode(padapter, authmode);

	if (rtw_set_802_11_ssid(padapter, &ndis_ssid) == false) {
		ret = -1;
		goto exit;
	}

	DBG_8192C("set ssid:dot11AuthAlgrthm=%d, dot11PrivacyAlgrthm=%d, dot118021XGrpPrivacy=%d\n", psecuritypriv->dot11AuthAlgrthm, psecuritypriv->dot11PrivacyAlgrthm, psecuritypriv->dot118021XGrpPrivacy);

exit:

	DBG_8192C("<=%s, ret %d\n",__func__, ret);

	return ret;
}

static int cfg80211_rtw_disconnect(struct wiphy *wiphy, struct net_device *ndev,
				   u16 reason_code)
{
	_adapter *padapter = wiphy_to_adapter(wiphy);

	DBG_871X(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	rtw_set_roaming(padapter, 0);

	if (check_fwstate(&padapter->mlmepriv, _FW_LINKED))
	{
		rtw_scan_abort(padapter);
		LeaveAllPowerSaveMode(padapter);
		rtw_disassoc_cmd(padapter, 500, false);

		DBG_871X("%s...call rtw_indicate_disconnect\n", __func__);

		padapter->mlmepriv.not_indic_disco = true;
		rtw_indicate_disconnect(padapter);
		padapter->mlmepriv.not_indic_disco = false;

		rtw_free_assoc_resources(padapter, 1);
	}

	return 0;
}

static int cfg80211_rtw_set_txpower(struct wiphy *wiphy,
				    struct wireless_dev *wdev,
				    enum nl80211_tx_power_setting type, int mbm)
{
	DBG_8192C("%s\n", __func__);
	return 0;
}

static int cfg80211_rtw_get_txpower(struct wiphy *wiphy,
				    struct wireless_dev *wdev, int *dbm)
{
	DBG_8192C("%s\n", __func__);
	*dbm = (12);
	return 0;
}

inline bool rtw_cfg80211_pwr_mgmt(_adapter *adapter)
{
	struct rtw_wdev_priv *rtw_wdev_priv = wdev_to_priv(adapter->rtw_wdev);
	return rtw_wdev_priv->power_mgmt;
}

static int cfg80211_rtw_set_power_mgmt(struct wiphy *wiphy,
				       struct net_device *ndev,
				       bool enabled, int timeout)
{
	_adapter *padapter = wiphy_to_adapter(wiphy);
	struct rtw_wdev_priv *rtw_wdev_priv = wdev_to_priv(padapter->rtw_wdev);

	DBG_871X(FUNC_NDEV_FMT" enabled:%u, timeout:%d\n", FUNC_NDEV_ARG(ndev),
		enabled, timeout);

	rtw_wdev_priv->power_mgmt = enabled;

	if (!enabled)
		LPS_Leave(padapter);
	return 0;
}

static int cfg80211_rtw_set_pmksa(struct wiphy *wiphy,
				  struct net_device *netdev,
				  struct cfg80211_pmksa *pmksa)
{
	u8	index,blInserted = false;
	_adapter	*padapter = wiphy_to_adapter(wiphy);
	struct security_priv	*psecuritypriv = &padapter->securitypriv;
	u8	strZeroMacAddress[ETH_ALEN] = { 0x00 };

	DBG_871X(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(netdev));

	if (_rtw_memcmp(pmksa->bssid, strZeroMacAddress, ETH_ALEN) == true)
	{
		return -EINVAL;
	}

	blInserted = false;

	/* overwrite PMKID */
	for (index=0 ; index<NUM_PMKID_CACHE; index++)
	{
		if (_rtw_memcmp(psecuritypriv->PMKIDList[index].Bssid, pmksa->bssid, ETH_ALEN) ==true)
		{ /*  BSSID is matched, the same AP => rewrite with new PMKID. */
			DBG_871X(FUNC_NDEV_FMT" BSSID exists in the PMKList.\n", FUNC_NDEV_ARG(netdev));

			memcpy(psecuritypriv->PMKIDList[index].PMKID, pmksa->pmkid, WLAN_PMKID_LEN);
			psecuritypriv->PMKIDList[index].bUsed = true;
			psecuritypriv->PMKIDIndex = index+1;
			blInserted = true;
			break;
		}
	}

	if (!blInserted)
	{
		/*  Find a new entry */
		DBG_871X(FUNC_NDEV_FMT" Use the new entry index = %d for this PMKID.\n",
			FUNC_NDEV_ARG(netdev), psecuritypriv->PMKIDIndex);

		memcpy(psecuritypriv->PMKIDList[psecuritypriv->PMKIDIndex].Bssid, pmksa->bssid, ETH_ALEN);
		memcpy(psecuritypriv->PMKIDList[psecuritypriv->PMKIDIndex].PMKID, pmksa->pmkid, WLAN_PMKID_LEN);

		psecuritypriv->PMKIDList[psecuritypriv->PMKIDIndex].bUsed = true;
		psecuritypriv->PMKIDIndex++ ;
		if (psecuritypriv->PMKIDIndex==16)
		{
			psecuritypriv->PMKIDIndex =0;
		}
	}

	return 0;
}

static int cfg80211_rtw_del_pmksa(struct wiphy *wiphy,
				  struct net_device *netdev,
				  struct cfg80211_pmksa *pmksa)
{
	u8	index, bMatched = false;
	_adapter	*padapter = wiphy_to_adapter(wiphy);
	struct security_priv	*psecuritypriv = &padapter->securitypriv;

	DBG_871X(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(netdev));

	for (index=0 ; index<NUM_PMKID_CACHE; index++)
	{
		if (_rtw_memcmp(psecuritypriv->PMKIDList[index].Bssid, pmksa->bssid, ETH_ALEN) ==true)
		{ /*  BSSID is matched, the same AP => Remove this PMKID information and reset it. */
			memset(psecuritypriv->PMKIDList[index].Bssid, 0x00, ETH_ALEN);
			memset(psecuritypriv->PMKIDList[index].PMKID, 0x00, WLAN_PMKID_LEN);
			psecuritypriv->PMKIDList[index].bUsed = false;
			bMatched = true;
			break;
		}
	}

	if (false == bMatched)
	{
		DBG_871X(FUNC_NDEV_FMT" do not have matched BSSID\n"
			, FUNC_NDEV_ARG(netdev));
		return -EINVAL;
	}

	return 0;
}

static int cfg80211_rtw_flush_pmksa(struct wiphy *wiphy,
				    struct net_device *netdev)
{
	_adapter	*padapter = wiphy_to_adapter(wiphy);
	struct security_priv	*psecuritypriv = &padapter->securitypriv;

	DBG_871X(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(netdev));

	memset(&psecuritypriv->PMKIDList[0], 0x00, sizeof(RT_PMKID_LIST) * NUM_PMKID_CACHE);
	psecuritypriv->PMKIDIndex = 0;

	return 0;
}

#ifdef CONFIG_92D_AP_MODE
void rtw_cfg80211_indicate_sta_assoc(_adapter *padapter, u8 *pmgmt_frame, uint frame_len)
{
	s32 freq;
	int channel;
	struct wireless_dev *pwdev = padapter->rtw_wdev;
	struct mlme_ext_priv *pmlmeext = &(padapter->mlmeextpriv);
	struct net_device *ndev = padapter->pnetdev;

	DBG_8192C("%s(padapter=%p,%s)\n", __func__, padapter, ndev->name);

#if defined(RTW_USE_CFG80211_STA_EVENT) || defined(COMPAT_KERNEL_RELEASE)
	{
		struct station_info sinfo;
		u8 ie_offset;
		if (GetFrameSubType(pmgmt_frame) == WIFI_ASSOCREQ)
			ie_offset = _ASOCREQ_IE_OFFSET_;
		else /*  WIFI_REASSOCREQ */
			ie_offset = _REASOCREQ_IE_OFFSET_;

		sinfo.filled = 0;
		sinfo.filled = STATION_INFO_ASSOC_REQ_IES;
		sinfo.assoc_req_ies = pmgmt_frame + WLAN_HDR_A3_LEN + ie_offset;
		sinfo.assoc_req_ies_len = frame_len - WLAN_HDR_A3_LEN - ie_offset;
		cfg80211_new_sta(ndev, GetAddr2Ptr(pmgmt_frame), &sinfo, GFP_ATOMIC);
	}
#else /* defined(RTW_USE_CFG80211_STA_EVENT) */
	channel = pmlmeext->cur_channel;
	if (channel <= RTW_CH_MAX_2G_CHANNEL)
		freq = rtw_ieee80211_channel_to_frequency(channel, IEEE80211_BAND_2GHZ);
	else
		freq = rtw_ieee80211_channel_to_frequency(channel, IEEE80211_BAND_5GHZ);

	#ifdef COMPAT_KERNEL_RELEASE
	rtw_cfg80211_rx_mgmt(padapter, freq, 0, pmgmt_frame, frame_len, GFP_ATOMIC);
	#elif !defined(CONFIG_CFG80211_FORCE_COMPATIBLE_2_6_37_UNDER)
	rtw_cfg80211_rx_mgmt(padapter, freq, 0, pmgmt_frame, frame_len, GFP_ATOMIC);
	#else /* COMPAT_KERNEL_RELEASE */
	{
		/* to avoid WARN_ON(wdev->iftype != NL80211_IFTYPE_STATION)  when calling cfg80211_send_rx_assoc() */
		pwdev->iftype = NL80211_IFTYPE_STATION;
		DBG_8192C("iftype=%d before call cfg80211_send_rx_assoc()\n", pwdev->iftype);
		rtw_cfg80211_send_rx_assoc(padapter, NULL, pmgmt_frame, frame_len);
		DBG_8192C("iftype=%d after call cfg80211_send_rx_assoc()\n", pwdev->iftype);
		pwdev->iftype = NL80211_IFTYPE_AP;
	}
	#endif /* COMPAT_KERNEL_RELEASE */
#endif /* defined(RTW_USE_CFG80211_STA_EVENT) */
}

void rtw_cfg80211_indicate_sta_disassoc(_adapter *padapter, unsigned char *da, unsigned short reason)
{
	s32 freq;
	int channel;
	u8 *pmgmt_frame;
	uint frame_len;
	struct rtw_ieee80211_hdr *pwlanhdr;
	unsigned short *fctrl;
	u8 mgmt_buf[128] = {0};
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct net_device *ndev = padapter->pnetdev;

	DBG_8192C("%s(padapter=%p,%s)\n", __func__, padapter, ndev->name);

#if defined(RTW_USE_CFG80211_STA_EVENT) || defined(COMPAT_KERNEL_RELEASE)
	xcfg80211_del_sta(ndev, da, GFP_ATOMIC);
#else /* defined(RTW_USE_CFG80211_STA_EVENT) */
	ychannel = pmlmeext->cur_channel;
	if (channel <= RTW_CH_MAX_2G_CHANNEL)
		freq = rtw_ieee80211_channel_to_frequency(channel, IEEE80211_BAND_2GHZ);
	else
		freq = rtw_ieee80211_channel_to_frequency(channel, IEEE80211_BAND_5GHZ);

	pmgmt_frame = mgmt_buf;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pmgmt_frame;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	memcpy(pwlanhdr->addr1, myid(&(padapter->eeprompriv)), ETH_ALEN);
	memcpy(pwlanhdr->addr2, da, ETH_ALEN);
	memcpy(pwlanhdr->addr3, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(pmgmt_frame, WIFI_DEAUTH);

	pmgmt_frame += sizeof(struct rtw_ieee80211_hdr_3addr);
	frame_len = sizeof(struct rtw_ieee80211_hdr_3addr);

	reason = cpu_to_le16(reason);
	pmgmt_frame = rtw_set_fixed_ie(pmgmt_frame, _RSON_CODE_ , (unsigned char *)&reason, &frame_len);

	#ifdef COMPAT_KERNEL_RELEASE
	rtw_cfg80211_rx_mgmt(padapter, freq, 0, mgmt_buf, frame_len, GFP_ATOMIC);
	#elif !defined(CONFIG_CFG80211_FORCE_COMPATIBLE_2_6_37_UNDER)
	rtw_cfg80211_rx_mgmt(padapter, freq, 0, mgmt_buf, frame_len, GFP_ATOMIC);
	#else /* COMPAT_KERNEL_RELEASE */
	cfg80211_send_disassoc(padapter->pnetdev, mgmt_buf, frame_len);
	#endif /* COMPAT_KERNEL_RELEASE */
#endif /* defined(RTW_USE_CFG80211_STA_EVENT) */
}

static int rtw_cfg80211_monitor_if_open(struct net_device *ndev)
{
	int ret = 0;

	DBG_8192C("%s\n", __func__);

	return ret;
}

static int rtw_cfg80211_monitor_if_close(struct net_device *ndev)
{
	int ret = 0;

	DBG_8192C("%s\n", __func__);

	return ret;
}

static int rtw_cfg80211_monitor_if_xmit_entry(struct sk_buff *skb, struct net_device *ndev)
{
	int ret = 0;
	int rtap_len;
	int qos_len = 0;
	int dot11_hdr_len = 24;
	int snap_len = 6;
	unsigned char *pdata;
	u16 frame_ctl;
	unsigned char src_mac_addr[6];
	unsigned char dst_mac_addr[6];
	struct ieee80211_hdr *dot11_hdr;
	struct ieee80211_radiotap_header *rtap_hdr;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);

	DBG_871X(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	if (unlikely(skb->len < sizeof(struct ieee80211_radiotap_header)))
		goto fail;

	rtap_hdr = (struct ieee80211_radiotap_header *)skb->data;
	if (unlikely(rtap_hdr->it_version))
		goto fail;

	rtap_len = ieee80211_get_radiotap_len(skb->data);
	if (unlikely(skb->len < rtap_len))
		goto fail;

	if (rtap_len != 14)
	{
		DBG_8192C("radiotap len (should be 14): %d\n", rtap_len);
		goto fail;
	}

	/* Skip the ratio tap header */
	skb_pull(skb, rtap_len);

	dot11_hdr = (struct ieee80211_hdr *)skb->data;
	frame_ctl = le16_to_cpu(dot11_hdr->frame_control);
	/* Check if the QoS bit is set */
	if ((frame_ctl & RTW_IEEE80211_FCTL_FTYPE) == RTW_IEEE80211_FTYPE_DATA) {
		/* Check if this ia a Wireless Distribution System (WDS) frame
		 * which has 4 MAC addresses
		 */
		if (dot11_hdr->frame_control & 0x0080)
			qos_len = 2;
		if ((dot11_hdr->frame_control & 0x0300) == 0x0300)
			dot11_hdr_len += 6;

		memcpy(dst_mac_addr, dot11_hdr->addr1, sizeof(dst_mac_addr));
		memcpy(src_mac_addr, dot11_hdr->addr2, sizeof(src_mac_addr));

		/* Skip the 802.11 header, QoS (if any) and SNAP, but leave spaces for
		 * for two MAC addresses
		 */
		skb_pull(skb, dot11_hdr_len + qos_len + snap_len - sizeof(src_mac_addr) * 2);
		pdata = (unsigned char*)skb->data;
		memcpy(pdata, dst_mac_addr, sizeof(dst_mac_addr));
		memcpy(pdata + sizeof(dst_mac_addr), src_mac_addr, sizeof(src_mac_addr));

		DBG_8192C("should be eapol packet\n");

		/* Use the real net device to transmit the packet */
		ret =  rtw_xmit_entry(skb, padapter->pnetdev);

		return ret;

	}
	else if ((frame_ctl & (RTW_IEEE80211_FCTL_FTYPE|RTW_IEEE80211_FCTL_STYPE))
		== (RTW_IEEE80211_FTYPE_MGMT|RTW_IEEE80211_STYPE_ACTION)
	)
	{
		/* only for action frames */
		struct xmit_frame		*pmgntframe;
		struct pkt_attrib	*pattrib;
		unsigned char	*pframe;
		struct rtw_ieee80211_hdr *pwlanhdr;
		struct xmit_priv	*pxmitpriv = &(padapter->xmitpriv);
		struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
		u8 *buf = skb->data;
		u32 len = skb->len;
		u8 category, action;
		int type = -1;

		if (rtw_action_frame_parse(buf, len, &category, &action) == false) {
			DBG_8192C(FUNC_NDEV_FMT" frame_control:0x%x\n", FUNC_NDEV_ARG(ndev),
				le16_to_cpu(((struct rtw_ieee80211_hdr_3addr *)buf)->frame_ctl));
			goto fail;
		}

		DBG_8192C("RTW_Tx:da=%pM via "FUNC_NDEV_FMT"\n",
			GetAddr1Ptr(buf), FUNC_NDEV_ARG(ndev));
		#ifdef CONFIG_P2P
		if ((type = rtw_p2p_check_frames(padapter, buf, len, true)) >= 0)
			goto dump;
		#endif
		if (category == RTW_WLAN_CATEGORY_PUBLIC)
			DBG_871X("RTW_Tx:%s\n", action_public_str(action));
		else
			DBG_871X("RTW_Tx:category(%u), action(%u)\n", category, action);

dump:
		/* starting alloc mgmt frame to dump it */
		if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
		{
			goto fail;
		}

		/* update attribute */
		pattrib = &pmgntframe->attrib;
		update_mgntframe_attrib(padapter, pattrib);
		pattrib->retry_ctrl = false;

		memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

		pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;

		memcpy(pframe, (void*)buf, len);
		#ifdef CONFIG_WFD
		if (type >= 0)
		{
			struct wifi_display_info		*pwfd_info;

			pwfd_info = padapter->wdinfo.wfd_info;

			if (true == pwfd_info->wfd_enable)
			{
				rtw_append_wfd_ie(padapter, pframe, &len);
			}
		}
		#endif /*  CONFIG_WFD */
		pattrib->pktlen = len;

		pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;
		/* update seq number */
		pmlmeext->mgnt_seq = GetSequence(pwlanhdr);
		pattrib->seqnum = pmlmeext->mgnt_seq;
		pmlmeext->mgnt_seq++;

		pattrib->last_txcmdsz = pattrib->pktlen;

		dump_mgntframe(padapter, pmgntframe);

	}
	else
	{
		DBG_8192C("frame_ctl=0x%x\n", frame_ctl & (RTW_IEEE80211_FCTL_FTYPE|RTW_IEEE80211_FCTL_STYPE));
	}

fail:

	dev_kfree_skb(skb);

	return 0;
}

static void rtw_cfg80211_monitor_if_set_multicast_list(struct net_device *ndev)
{
	DBG_8192C("%s\n", __func__);
}

static int rtw_cfg80211_monitor_if_set_mac_address(struct net_device *ndev, void *addr)
{
	int ret = 0;

	DBG_8192C("%s\n", __func__);

	return ret;
}

static const struct net_device_ops rtw_cfg80211_monitor_if_ops = {
	.ndo_open = rtw_cfg80211_monitor_if_open,
       .ndo_stop = rtw_cfg80211_monitor_if_close,
       .ndo_start_xmit = rtw_cfg80211_monitor_if_xmit_entry,
       .ndo_set_mac_address = rtw_cfg80211_monitor_if_set_mac_address,
};

static int rtw_cfg80211_add_monitor_if (_adapter *padapter, char *name, struct net_device **ndev)
{
	int ret = 0;
	struct net_device* mon_ndev = NULL;
	struct wireless_dev* mon_wdev = NULL;
	struct rtw_netdev_priv_indicator *pnpi;
	struct rtw_wdev_priv *pwdev_priv = wdev_to_priv(padapter->rtw_wdev);

	if (!name) {
		DBG_871X(FUNC_ADPT_FMT" without specific name\n", FUNC_ADPT_ARG(padapter));
		ret = -EINVAL;
		goto out;
	}

	if (pwdev_priv->pmon_ndev) {
		DBG_871X(FUNC_ADPT_FMT" monitor interface exist: "NDEV_FMT"\n",
			FUNC_ADPT_ARG(padapter), NDEV_ARG(pwdev_priv->pmon_ndev));
		ret = -EBUSY;
		goto out;
	}

	mon_ndev = alloc_etherdev(sizeof(struct rtw_netdev_priv_indicator));
	if (!mon_ndev) {
		DBG_871X(FUNC_ADPT_FMT" allocate ndev fail\n", FUNC_ADPT_ARG(padapter));
		ret = -ENOMEM;
		goto out;
	}

	mon_ndev->type = ARPHRD_IEEE80211_RADIOTAP;
	strncpy(mon_ndev->name, name, IFNAMSIZ);
	mon_ndev->name[IFNAMSIZ - 1] = 0;
	mon_ndev->destructor = rtw_ndev_destructor;

	mon_ndev->netdev_ops = &rtw_cfg80211_monitor_if_ops;

	pnpi = netdev_priv(mon_ndev);
	pnpi->priv = padapter;
	pnpi->sizeof_priv = sizeof(_adapter);

	/*  wdev */
	mon_wdev = (struct wireless_dev *)kzalloc(sizeof(struct wireless_dev), GFP_KERNEL);
	if (!mon_wdev) {
		DBG_871X(FUNC_ADPT_FMT" allocate mon_wdev fail\n", FUNC_ADPT_ARG(padapter));
		ret = -ENOMEM;
		goto out;
	}

	mon_wdev->wiphy = padapter->rtw_wdev->wiphy;
	mon_wdev->netdev = mon_ndev;
	mon_wdev->iftype = NL80211_IFTYPE_MONITOR;
	mon_ndev->ieee80211_ptr = mon_wdev;

	ret = register_netdevice(mon_ndev);
	if (ret) {
		goto out;
	}

	*ndev = pwdev_priv->pmon_ndev = mon_ndev;
	memcpy(pwdev_priv->ifname_mon, name, IFNAMSIZ+1);

out:
	if (ret && mon_wdev) {
		kfree(mon_wdev);
		mon_wdev = NULL;
	}

	if (ret && mon_ndev) {
		free_netdev(mon_ndev);
		*ndev = mon_ndev = NULL;
	}

	return ret;
}

static struct wireless_dev *add_virtual_intf(struct wiphy *wiphy,
					     const char *name,
					     enum nl80211_iftype type,
					     u32 *flags,
					     struct vif_params *params)
{
	int ret = 0;
	struct net_device* ndev = NULL;
	_adapter *padapter = wiphy_to_adapter(wiphy);

	DBG_871X(FUNC_ADPT_FMT " wiphy:%s, name:%s, type:%d\n",
		FUNC_ADPT_ARG(padapter), wiphy_name(wiphy), name, type);

	switch (type) {
	case NL80211_IFTYPE_ADHOC:
	case NL80211_IFTYPE_AP_VLAN:
	case NL80211_IFTYPE_WDS:
	case NL80211_IFTYPE_MESH_POINT:
		ret = -ENODEV;
		break;
	case NL80211_IFTYPE_MONITOR:
		ret = rtw_cfg80211_add_monitor_if (padapter, (char *)name, &ndev);
		break;
	case NL80211_IFTYPE_P2P_CLIENT:
	case NL80211_IFTYPE_STATION:
		ret = -ENODEV;
		break;
	case NL80211_IFTYPE_P2P_GO:
	case NL80211_IFTYPE_AP:
		ret = -ENODEV;
		break;
	default:
		ret = -ENODEV;
		DBG_871X("Unsupported interface type\n");
		break;
	}

	DBG_871X(FUNC_ADPT_FMT" ndev:%p, ret:%d\n", FUNC_ADPT_ARG(padapter), ndev, ret);

	return ndev ? ndev->ieee80211_ptr : ERR_PTR(ret);
}

static int cfg80211_rtw_del_virtual_intf(struct wiphy *wiphy,
					 struct wireless_dev *wdev)
{
	struct rtw_wdev_priv *pwdev_priv = (struct rtw_wdev_priv *)wiphy_priv(wiphy);
	struct net_device *ndev;
	ndev = wdev ? wdev->netdev : NULL;

	if (!ndev)
		goto exit;

	unregister_netdevice(ndev);

	if (ndev == pwdev_priv->pmon_ndev) {
		pwdev_priv->pmon_ndev = NULL;
		pwdev_priv->ifname_mon[0] = '\0';
		DBG_871X(FUNC_NDEV_FMT" remove monitor interface\n", FUNC_NDEV_ARG(ndev));
	}

exit:
	return 0;
}

static int rtw_add_beacon(_adapter *adapter, const u8 *head, size_t head_len, const u8 *tail, size_t tail_len)
{
	int ret=0;
	u8 *pbuf = NULL;
	uint len, wps_ielen=0;
	uint p2p_ielen=0;
	u8 *p2p_ie;
	u8 got_p2p_ie = false;
	struct mlme_priv *pmlmepriv = &(adapter->mlmepriv);

	DBG_8192C("%s beacon_head_len=%zu, beacon_tail_len=%zu\n", __func__, head_len, tail_len);

	if (check_fwstate(pmlmepriv, WIFI_AP_STATE) != true)
		return -EINVAL;

	if (head_len<24)
		return -EINVAL;

	pbuf = kzalloc(head_len+tail_len, GFP_KERNEL);
	if (!pbuf)
		return -ENOMEM;

	memcpy(pbuf, (void *)head+24, head_len-24);/*  24=beacon header len. */
	memcpy(pbuf+head_len-24, (void *)tail, tail_len);

	len = head_len+tail_len-24;

	/* check wps ie if inclued */
	if (rtw_get_wps_ie(pbuf+_FIXED_IE_LENGTH_, len-_FIXED_IE_LENGTH_, NULL, &wps_ielen))
		DBG_8192C("add bcn, wps_ielen=%d\n", wps_ielen);

#ifdef CONFIG_P2P
	/* check p2p ie if inclued */
	if (rtw_get_p2p_ie(pbuf+_FIXED_IE_LENGTH_, len-_FIXED_IE_LENGTH_, NULL, &p2p_ielen))
	{
		DBG_8192C("got p2p_ie, len=%d\n", p2p_ielen);
		got_p2p_ie = true;
	}
#endif

	rtw_ies_remove_ie(pbuf, &len, _BEACON_IE_OFFSET_, _VENDOR_SPECIFIC_IE_, P2P_OUI, 4);

	/*  pbss_network->IEs will not include p2p_ie */
	if (rtw_check_beacon_data(adapter, pbuf,  len) == _SUCCESS)
	{
#ifdef  CONFIG_P2P
		/* check p2p if enable */
		if (got_p2p_ie == true)
		{
			struct mlme_ext_priv *pmlmeext = &adapter->mlmeextpriv;
			struct wifidirect_info *pwdinfo= &(adapter->wdinfo);

			if (rtw_p2p_chk_state(pwdinfo, P2P_STATE_NONE))
			{
				DBG_8192C("Enable P2P function for the first time\n");
				rtw_p2p_enable(adapter, P2P_ROLE_GO);
				wdev_to_priv(adapter->rtw_wdev)->p2p_enabled = true;
			}
			else
			{
				_cancel_timer_ex(&pwdinfo->find_phase_timer);
				_cancel_timer_ex(&pwdinfo->restore_p2p_state_timer);
				_cancel_timer_ex(&pwdinfo->pre_tx_scan_timer);

				DBG_8192C("enter GO Mode, p2p_ielen=%d\n", p2p_ielen);

				rtw_p2p_set_role(pwdinfo, P2P_ROLE_GO);
				rtw_p2p_set_state(pwdinfo, P2P_STATE_GONEGO_OK);
				pwdinfo->intent = 15;
			}

			pwdinfo->operating_channel = pmlmeext->cur_channel;

		}
#endif /* CONFIG_P2P */

		ret = 0;

	}
	else
	{
		ret = -EINVAL;
	}

	kfree(pbuf);

	return ret;
}

static int cfg80211_rtw_start_ap(struct wiphy *wiphy, struct net_device *ndev,
								struct cfg80211_ap_settings *settings)
{
	int ret = 0;
	_adapter *adapter = wiphy_to_adapter(wiphy);

	DBG_871X(FUNC_NDEV_FMT" hidden_ssid:%d, auth_type:%d\n", FUNC_NDEV_ARG(ndev),
		settings->hidden_ssid, settings->auth_type);

	ret = rtw_add_beacon(adapter, settings->beacon.head, settings->beacon.head_len,
		settings->beacon.tail, settings->beacon.tail_len);

	adapter->mlmeextpriv.mlmext_info.hidden_ssid_mode = settings->hidden_ssid;

	if (settings->ssid && settings->ssid_len) {
		struct wlan_bssid_ex *pbss_network = &adapter->mlmepriv.cur_network.network;
		struct wlan_bssid_ex *pbss_network_ext = &adapter->mlmeextpriv.mlmext_info.network;

		memcpy(pbss_network->Ssid.Ssid, (void *)settings->ssid, settings->ssid_len);
		pbss_network->Ssid.SsidLength = settings->ssid_len;
		memcpy(pbss_network_ext->Ssid.Ssid, (void *)settings->ssid, settings->ssid_len);
		pbss_network_ext->Ssid.SsidLength = settings->ssid_len;
	}

	return ret;
}

static int cfg80211_rtw_change_beacon(struct wiphy *wiphy, struct net_device *ndev,
                                struct cfg80211_beacon_data *info)
{
	int ret = 0;
	_adapter *adapter = wiphy_to_adapter(wiphy);

	DBG_871X(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	ret = rtw_add_beacon(adapter, info->head, info->head_len, info->tail, info->tail_len);

	return ret;
}

static int cfg80211_rtw_stop_ap(struct wiphy *wiphy, struct net_device *ndev)
{
	DBG_871X(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));
	return 0;
}

static int cfg80211_rtw_add_station(struct wiphy *wiphy, struct net_device *ndev,
			       u8 *mac, struct station_parameters *params)
{
	DBG_871X(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	return 0;
}

static int	cfg80211_rtw_del_station(struct wiphy *wiphy, struct net_device *ndev,
			       u8 *mac)
{
	int ret=0;
	_list	*phead, *plist;
	u8 updated;
	struct sta_info *psta = NULL;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	struct sta_priv *pstapriv = &padapter->stapriv;

	DBG_871X("+"FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	if (check_fwstate(pmlmepriv, (_FW_LINKED|WIFI_AP_STATE)) != true)
	{
		DBG_8192C("%s, fw_state != FW_LINKED|WIFI_AP_STATE\n", __func__);
		return -EINVAL;
	}

	if (!mac)
	{
		DBG_8192C("flush all sta, and cam_entry\n");

		flush_all_cam_entry(padapter);	/* clear CAM */

		ret = rtw_sta_flush(padapter);

		return ret;
	}

	DBG_8192C("free sta macaddr = %pM\n", mac);

	if (mac[0] == 0xff && mac[1] == 0xff &&
	    mac[2] == 0xff && mac[3] == 0xff &&
	    mac[4] == 0xff && mac[5] == 0xff)
	{
		return -EINVAL;
	}

	spin_lock_bh(&pstapriv->asoc_list_lock);

	phead = &pstapriv->asoc_list;
	plist = phead->next;

	/* check asoc_queue */
	while ((rtw_end_of_queue_search(phead, plist)) == false)
	{
		psta = container_of(plist, struct sta_info, asoc_list);

		plist = plist->next;

		if (_rtw_memcmp(mac, psta->hwaddr, ETH_ALEN))
		{
			if (psta->dot8021xalg == 1 && psta->bpairwise_key_installed == false)
			{
				DBG_8192C("%s, sta's dot8021xalg = 1 and key_installed = false\n", __func__);
			}
			else
			{
				DBG_8192C("free psta=%p, aid=%d\n", psta, psta->aid);

				list_del_init(&psta->asoc_list);
				pstapriv->asoc_list_cnt--;

				updated = ap_free_sta(padapter, psta, true, WLAN_REASON_DEAUTH_LEAVING);

				psta = NULL;

				break;
			}

		}

	}

	spin_unlock_bh(&pstapriv->asoc_list_lock);

	associated_clients_update(padapter, updated);

	DBG_871X("-"FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	return ret;
}

static int	cfg80211_rtw_change_station(struct wiphy *wiphy, struct net_device *ndev,
				  u8 *mac, struct station_parameters *params)
{
	DBG_871X(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	return 0;
}

static int	cfg80211_rtw_dump_station(struct wiphy *wiphy, struct net_device *ndev,
			       int idx, u8 *mac, struct station_info *sinfo)
{
	DBG_871X(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	/* TODO: dump scanned queue */

	return -ENOENT;
}

static int	cfg80211_rtw_change_bss(struct wiphy *wiphy, struct net_device *ndev,
			      struct bss_parameters *params)
{
	u8 i;

	DBG_871X(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));
	return 0;
}

static int rtw_set_channel(struct wiphy *wiphy, struct net_device *ndev,
			   struct ieee80211_channel *chan,
			   enum nl80211_channel_type channel_type)
{
	DBG_871X(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	return 0;
}

static int	cfg80211_rtw_auth(struct wiphy *wiphy, struct net_device *ndev,
			struct cfg80211_auth_request *req)
{
	DBG_871X(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	return 0;
}

static int	cfg80211_rtw_assoc(struct wiphy *wiphy, struct net_device *ndev,
			 struct cfg80211_assoc_request *req)
{
	DBG_871X(FUNC_NDEV_FMT"\n", FUNC_NDEV_ARG(ndev));

	return 0;
}
#endif /* CONFIG_92D_AP_MODE */

void rtw_cfg80211_rx_action_p2p(_adapter *padapter, u8 *pmgmt_frame, uint frame_len)
{
	int type;
	s32 freq;
	int channel;
	struct mlme_ext_priv *pmlmeext = &(padapter->mlmeextpriv);
	u8 category, action;

	channel = rtw_get_oper_ch(padapter);

	DBG_8192C("RTW_Rx:cur_ch=%d\n", channel);
	#ifdef CONFIG_P2P
	type = rtw_p2p_check_frames(padapter, pmgmt_frame, frame_len, false);
	if (type >= 0)
		goto indicate;
	#endif
	rtw_action_frame_parse(pmgmt_frame, frame_len, &category, &action);
	DBG_871X("RTW_Rx:category(%u), action(%u)\n", category, action);

indicate:
	if (channel <= RTW_CH_MAX_2G_CHANNEL)
		freq = rtw_ieee80211_channel_to_frequency(channel, IEEE80211_BAND_2GHZ);
	else
		freq = rtw_ieee80211_channel_to_frequency(channel, IEEE80211_BAND_5GHZ);

	rtw_cfg80211_rx_mgmt(padapter, freq, 0, pmgmt_frame, frame_len, GFP_ATOMIC);
}

void rtw_cfg80211_rx_p2p_action_public(_adapter *padapter, u8 *pmgmt_frame, uint frame_len)
{
	int type;
	s32 freq;
	int channel;
	struct mlme_ext_priv *pmlmeext = &(padapter->mlmeextpriv);
	u8 category, action;

	channel = rtw_get_oper_ch(padapter);

	DBG_8192C("RTW_Rx:cur_ch=%d\n", channel);
	#ifdef CONFIG_P2P
	type = rtw_p2p_check_frames(padapter, pmgmt_frame, frame_len, false);
	if (type >= 0) {
		switch (type) {
		case P2P_GO_NEGO_CONF:
		case P2P_PROVISION_DISC_RESP:
			rtw_clear_scan_deny(padapter);
		}
		goto indicate;
	}
	#endif
	rtw_action_frame_parse(pmgmt_frame, frame_len, &category, &action);
	DBG_871X("RTW_Rx:category(%u), action(%u)\n", category, action);

indicate:
	if (channel <= RTW_CH_MAX_2G_CHANNEL)
		freq = rtw_ieee80211_channel_to_frequency(channel, IEEE80211_BAND_2GHZ);
	else
		freq = rtw_ieee80211_channel_to_frequency(channel, IEEE80211_BAND_5GHZ);

	rtw_cfg80211_rx_mgmt(padapter, freq, 0, pmgmt_frame, frame_len, GFP_ATOMIC);
}

void rtw_cfg80211_rx_action(_adapter *adapter, u8 *frame, uint frame_len, const char*msg)
{
	s32 freq;
	int channel;
	struct mlme_ext_priv *pmlmeext = &(adapter->mlmeextpriv);
	struct rtw_wdev_priv *pwdev_priv = wdev_to_priv(adapter->rtw_wdev);
	u8 category, action;

	channel = rtw_get_oper_ch(adapter);

	rtw_action_frame_parse(frame, frame_len, &category, &action);

	DBG_8192C("RTW_Rx:cur_ch=%d\n", channel);
	if (msg)
		DBG_871X("RTW_Rx:%s\n", msg);
	else
		DBG_871X("RTW_Rx:category(%u), action(%u)\n", category, action);

	if (channel <= RTW_CH_MAX_2G_CHANNEL)
		freq = rtw_ieee80211_channel_to_frequency(channel, IEEE80211_BAND_2GHZ);
	else
		freq = rtw_ieee80211_channel_to_frequency(channel, IEEE80211_BAND_5GHZ);

	rtw_cfg80211_rx_mgmt(adapter, freq, 0, frame, frame_len, GFP_ATOMIC);
}

#ifdef CONFIG_P2P
void rtw_cfg80211_issue_p2p_provision_request(_adapter *padapter, const u8 *buf, size_t len)
{
	u16	wps_devicepassword_id = 0x0000;
	uint	wps_devicepassword_id_len = 0;
	u8			wpsie[255] = { 0x00 }, p2p_ie[255] = { 0x00 };
	uint			p2p_ielen = 0;
	uint			wpsielen = 0;
	u32	devinfo_contentlen = 0;
	u8	devinfo_content[64] = { 0x00 };
	u16	capability = 0;
	uint capability_len = 0;

	unsigned char category = RTW_WLAN_CATEGORY_PUBLIC;
	u8			action = P2P_PUB_ACTION_ACTION;
	u8			dialogToken = 1;
	u32			p2poui = cpu_to_be32(P2POUI);
	u8			oui_subtype = P2P_PROVISION_DISC_REQ;
	u32			p2pielen = 0;
#ifdef CONFIG_WFD
	u32					wfdielen = 0;
#endif /* CONFIG_WFD */

	struct xmit_frame			*pmgntframe;
	struct pkt_attrib			*pattrib;
	unsigned char					*pframe;
	struct rtw_ieee80211_hdr	*pwlanhdr;
	unsigned short				*fctrl;
	struct xmit_priv			*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	struct wifidirect_info *pwdinfo = &(padapter->wdinfo);
	u8 *frame_body = (unsigned char *)(buf + sizeof(struct rtw_ieee80211_hdr_3addr));
	size_t frame_body_len = len - sizeof(struct rtw_ieee80211_hdr_3addr);

	DBG_871X("[%s] In\n", __func__);

	/* prepare for building provision_request frame */
	memcpy(pwdinfo->tx_prov_disc_info.peerIFAddr, GetAddr1Ptr(buf), ETH_ALEN);
	memcpy(pwdinfo->tx_prov_disc_info.peerDevAddr, GetAddr1Ptr(buf), ETH_ALEN);

	pwdinfo->tx_prov_disc_info.wps_config_method_request = WPS_CM_PUSH_BUTTON;

	rtw_get_wps_ie(frame_body + _PUBLIC_ACTION_IE_OFFSET_, frame_body_len - _PUBLIC_ACTION_IE_OFFSET_, wpsie, &wpsielen);
	rtw_get_wps_attr_content(wpsie, wpsielen, WPS_ATTR_DEVICE_PWID, (u8*) &wps_devicepassword_id, &wps_devicepassword_id_len);
	wps_devicepassword_id = be16_to_cpu(wps_devicepassword_id);

	switch (wps_devicepassword_id)
	{
		case WPS_DPID_PIN:
			pwdinfo->tx_prov_disc_info.wps_config_method_request = WPS_CM_LABEL;
			break;
		case WPS_DPID_USER_SPEC:
			pwdinfo->tx_prov_disc_info.wps_config_method_request = WPS_CM_DISPLYA;
			break;
		case WPS_DPID_MACHINE_SPEC:
			break;
		case WPS_DPID_REKEY:
			break;
		case WPS_DPID_PBC:
			pwdinfo->tx_prov_disc_info.wps_config_method_request = WPS_CM_PUSH_BUTTON;
			break;
		case WPS_DPID_REGISTRAR_SPEC:
			pwdinfo->tx_prov_disc_info.wps_config_method_request = WPS_CM_KEYPAD;
			break;
		default:
			break;
	}

	if (rtw_get_p2p_ie(frame_body + _PUBLIC_ACTION_IE_OFFSET_, frame_body_len - _PUBLIC_ACTION_IE_OFFSET_, p2p_ie, &p2p_ielen))
	{

		rtw_get_p2p_attr_content(p2p_ie, p2p_ielen, P2P_ATTR_DEVICE_INFO, devinfo_content, &devinfo_contentlen);
		rtw_get_p2p_attr_content(p2p_ie, p2p_ielen, P2P_ATTR_CAPABILITY, (u8*)&capability, &capability_len);

	}

	/* start to build provision_request frame */
	memset(wpsie, 0, sizeof(wpsie));
	memset(p2p_ie, 0, sizeof(p2p_ie));
	p2p_ielen = 0;

	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	/* update attribute */
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);

	memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;
	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	memcpy(pwlanhdr->addr1, pwdinfo->tx_prov_disc_info.peerDevAddr, ETH_ALEN);
	memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	memcpy(pwlanhdr->addr3, pwdinfo->tx_prov_disc_info.peerDevAddr, ETH_ALEN);

	SetSeqNum(pwlanhdr, pmlmeext->mgnt_seq);
	pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_ACTION);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pattrib->pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

	pframe = rtw_set_fixed_ie(pframe, 1, &(category), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(action), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 4, (unsigned char *) &(p2poui), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(oui_subtype), &(pattrib->pktlen));
	pframe = rtw_set_fixed_ie(pframe, 1, &(dialogToken), &(pattrib->pktlen));

	/* build_prov_disc_request_p2p_ie */
	/*	P2P OUI */
	p2pielen = 0;
	p2p_ie[p2pielen++] = 0x50;
	p2p_ie[p2pielen++] = 0x6F;
	p2p_ie[p2pielen++] = 0x9A;
	p2p_ie[p2pielen++] = 0x09;	/*	WFA P2P v1.0 */

	/*	Commented by Albert 20110301 */
	/*	According to the P2P Specification, the provision discovery request frame should contain 3 P2P attributes */
	/*	1. P2P Capability */
	/*	2. Device Info */
	/*	3. Group ID (When joining an operating P2P Group) */

	/*	P2P Capability ATTR */
	/*	Type: */
	p2p_ie[p2pielen++] = P2P_ATTR_CAPABILITY;

	/*	Length: */
	RTW_PUT_LE16(p2p_ie + p2pielen, 0x0002);
	p2pielen += 2;

	/*	Value: */
	/*	Device Capability Bitmap, 1 byte */
	/*	Group Capability Bitmap, 1 byte */
	memcpy(p2p_ie + p2pielen, &capability, 2);
	p2pielen += 2;

	/*	Device Info ATTR */
	/*	Type: */
	p2p_ie[p2pielen++] = P2P_ATTR_DEVICE_INFO;

	/*	Length: */
	/*	21 -> P2P Device Address (6bytes) + Config Methods (2bytes) + Primary Device Type (8bytes) */
	/*	+ NumofSecondDevType (1byte) + WPS Device Name ID field (2bytes) + WPS Device Name Len field (2bytes) */
	RTW_PUT_LE16(p2p_ie + p2pielen, devinfo_contentlen);
	p2pielen += 2;

	/*	Value: */
	memcpy(p2p_ie + p2pielen, devinfo_content, devinfo_contentlen);
	p2pielen += devinfo_contentlen;

	pframe = rtw_set_ie(pframe, _VENDOR_SPECIFIC_IE_, p2pielen, (unsigned char *) p2p_ie, &p2p_ielen);
	pattrib->pktlen += p2p_ielen;

	wpsielen = 0;
	/*	WPS OUI */
	*(u32*) (wpsie) = cpu_to_be32(WPSOUI);
	wpsielen += 4;

	/*	WPS version */
	/*	Type: */
	*(u16*) (wpsie + wpsielen) = cpu_to_be16(WPS_ATTR_VER1);
	wpsielen += 2;

	/*	Length: */
	*(u16*) (wpsie + wpsielen) = cpu_to_be16(0x0001);
	wpsielen += 2;

	/*	Value: */
	wpsie[wpsielen++] = WPS_VERSION_1;	/*	Version 1.0 */

	/*	Config Method */
	/*	Type: */
	*(u16*) (wpsie + wpsielen) = cpu_to_be16(WPS_ATTR_CONF_METHOD);
	wpsielen += 2;

	/*	Length: */
	*(u16*) (wpsie + wpsielen) = cpu_to_be16(0x0002);
	wpsielen += 2;

	/*	Value: */
	*(u16*) (wpsie + wpsielen) = cpu_to_be16(pwdinfo->tx_prov_disc_info.wps_config_method_request);
	wpsielen += 2;

	pframe = rtw_set_ie(pframe, _VENDOR_SPECIFIC_IE_, wpsielen, (unsigned char *) wpsie, &pattrib->pktlen);

#ifdef CONFIG_WFD
	wfdielen = build_provdisc_req_wfd_ie(pwdinfo, pframe);
	pframe += wfdielen;
	pattrib->pktlen += wfdielen;
#endif /* CONFIG_WFD */

	pattrib->last_txcmdsz = pattrib->pktlen;

	if (dump_mgntframe_and_wait_ack(padapter, pmgntframe) != _SUCCESS)
		DBG_8192C("%s, ack to\n", __func__);

}

static s32 cfg80211_rtw_remain_on_channel(struct wiphy *wiphy,
	struct wireless_dev *wdev,
	struct ieee80211_channel * channel,
	unsigned int duration, u64 *cookie)
{
	s32 err = 0;
	_adapter *padapter = wiphy_to_adapter(wiphy);
	struct rtw_wdev_priv *pwdev_priv = wdev_to_priv(padapter->rtw_wdev);
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;
	struct wifidirect_info *pwdinfo = &padapter->wdinfo;
	struct cfg80211_wifidirect_info *pcfg80211_wdinfo = &padapter->cfg80211_wdinfo;
	u8 remain_ch = (u8) ieee80211_frequency_to_channel(channel->center_freq);
	u8 ready_on_channel = false;

	DBG_871X(FUNC_ADPT_FMT" ch:%u duration:%d\n", FUNC_ADPT_ARG(padapter), remain_ch, duration);

	if (pcfg80211_wdinfo->is_ro_ch == true)
	{
		DBG_8192C("%s, cancel ro ch timer\n", __func__);

		_cancel_timer_ex(&padapter->cfg80211_wdinfo.remain_on_ch_timer);

#ifdef CONFIG_CONCURRENT_MODE
                atomic_set(&pwdev_priv->ro_ch_to, 1);
#endif /* CONFIG_CONCURRENT_MODE */

		p2p_protocol_wk_hdl(padapter, P2P_RO_CH_WK);
	}

	pcfg80211_wdinfo->is_ro_ch = true;

	if (_FAIL == rtw_pwr_wakeup(padapter)) {
		err = -EFAULT;
		goto exit;
	}

	memcpy(&pcfg80211_wdinfo->remain_on_ch_channel, channel, sizeof(struct ieee80211_channel));
	pcfg80211_wdinfo->remain_on_ch_cookie= *cookie;

	rtw_scan_abort(padapter);
#ifdef CONFIG_CONCURRENT_MODE
	if (rtw_buddy_adapter_up(padapter))
		rtw_scan_abort(padapter->pbuddy_adapter);
#endif /* CONFIG_CONCURRENT_MODE */

	if (rtw_p2p_chk_state(pwdinfo, P2P_STATE_NONE))
	{
		rtw_p2p_enable(padapter, P2P_ROLE_DEVICE);
		wdev_to_priv(padapter->rtw_wdev)->p2p_enabled = true;
	}
	else
	{
		rtw_p2p_set_pre_state(pwdinfo, rtw_p2p_state(pwdinfo));
#ifdef CONFIG_DEBUG_CFG80211
		DBG_8192C("%s, role=%d, p2p_state=%d\n", __func__, rtw_p2p_role(pwdinfo), rtw_p2p_state(pwdinfo));
#endif
	}

	rtw_p2p_set_state(pwdinfo, P2P_STATE_LISTEN);

	if (duration < 400)
		duration = duration*3;/* extend from exper. */

#ifdef	CONFIG_CONCURRENT_MODE
	if (check_buddy_fwstate(padapter, _FW_LINKED) &&
		(duration<pwdinfo->ext_listen_interval))
	{
		duration = duration +	pwdinfo->ext_listen_interval;
	}
#endif

	pcfg80211_wdinfo->restore_channel = pmlmeext->cur_channel;

	if (rtw_ch_set_search_ch(pmlmeext->channel_set, remain_ch) >= 0) {
#ifdef	CONFIG_CONCURRENT_MODE
		if (check_buddy_fwstate(padapter, _FW_LINKED))
		{
			PADAPTER pbuddy_adapter = padapter->pbuddy_adapter;
			struct mlme_ext_priv *pbuddy_mlmeext = &pbuddy_adapter->mlmeextpriv;

			if (remain_ch != pbuddy_mlmeext->cur_channel)
			{
				if (atomic_read(&pwdev_priv->switch_ch_to)==1 ||
					(remain_ch != pmlmeext->cur_channel))
				{
					DBG_8192C("%s, issue nulldata pwrbit=1\n", __func__);
					issue_nulldata(padapter->pbuddy_adapter, NULL, 1, 3, 500);

					atomic_set(&pwdev_priv->switch_ch_to, 0);

					DBG_8192C("%s, set switch ch timer, duration=%d\n", __func__, duration-pwdinfo->ext_listen_interval);
					_set_timer(&pwdinfo->ap_p2p_switch_timer, duration-pwdinfo->ext_listen_interval);
				}
			}

			ready_on_channel = true;
		} else
#endif /* CONFIG_CONCURRENT_MODE */
		if (remain_ch != pmlmeext->cur_channel)
		{
			ready_on_channel = true;
		}
	} else {
		DBG_871X("%s remain_ch:%u not in channel plan!!!!\n", __func__, remain_ch);
	}

	/* call this after other things have been done */
#ifdef	CONFIG_CONCURRENT_MODE
	if (atomic_read(&pwdev_priv->ro_ch_to)==1 ||
		(remain_ch != pmlmeext->cur_channel))
	{
		u8 co_channel = 0xff;
		atomic_set(&pwdev_priv->ro_ch_to, 0);
#endif

		if (ready_on_channel == true)
		{
			if (!check_fwstate(&padapter->mlmepriv, _FW_LINKED))
				pmlmeext->cur_channel = remain_ch;

#ifdef	CONFIG_CONCURRENT_MODE
			co_channel = rtw_get_oper_ch(padapter);

			if (co_channel !=remain_ch)
#endif
				set_channel_bwmode(padapter, remain_ch, HAL_PRIME_CHNL_OFFSET_DONT_CARE, HT_CHANNEL_WIDTH_20);
		}

		DBG_8192C("%s, set ro ch timer, duration=%d\n", __func__, duration);
		_set_timer(&pcfg80211_wdinfo->remain_on_ch_timer, duration);

#ifdef	CONFIG_CONCURRENT_MODE
	}
#endif

	rtw_cfg80211_ready_on_channel(padapter, *cookie, channel, channel_type, duration, GFP_KERNEL);

	pwdinfo->listen_channel = pmlmeext->cur_channel;

exit:
	if (err)
		pcfg80211_wdinfo->is_ro_ch = false;

	return err;
}

static s32 cfg80211_rtw_cancel_remain_on_channel(struct wiphy *wiphy,
	struct wireless_dev *wdev,
	u64 cookie)
{
	s32 err = 0;
	_adapter *padapter = wiphy_to_adapter(wiphy);
	struct rtw_wdev_priv *pwdev_priv = wdev_to_priv(padapter->rtw_wdev);
	struct wifidirect_info *pwdinfo = &padapter->wdinfo;
	struct pwrctrl_priv *pwrpriv = &padapter->pwrctrlpriv;
	struct cfg80211_wifidirect_info *pcfg80211_wdinfo = &padapter->cfg80211_wdinfo;

	DBG_871X(FUNC_ADPT_FMT"\n", FUNC_ADPT_ARG(padapter));

	if (pcfg80211_wdinfo->is_ro_ch == true) {
		DBG_8192C("%s, cancel ro ch timer\n", __func__);
		_cancel_timer_ex(&padapter->cfg80211_wdinfo.remain_on_ch_timer);
		#ifdef CONFIG_CONCURRENT_MODE
		atomic_set(&pwdev_priv->ro_ch_to, 1);
		#endif
		p2p_protocol_wk_hdl(padapter, P2P_RO_CH_WK);
	}

	 rtw_p2p_set_state(pwdinfo, rtw_p2p_pre_state(pwdinfo));
#ifdef CONFIG_DEBUG_CFG80211
	 DBG_8192C("%s, role=%d, p2p_state=%d\n", __func__, rtw_p2p_role(pwdinfo), rtw_p2p_state(pwdinfo));
#endif
	pcfg80211_wdinfo->is_ro_ch = false;

	return err;
}

#endif /* CONFIG_P2P */

static int _cfg80211_rtw_mgmt_tx(_adapter *padapter, u8 tx_ch, const u8 *buf, size_t len)
{
	struct xmit_frame	*pmgntframe;
	struct pkt_attrib	*pattrib;
	unsigned char	*pframe;
	int ret = _FAIL;
	bool ack = true;
	struct rtw_ieee80211_hdr *pwlanhdr;
	struct rtw_wdev_priv *pwdev_priv = wdev_to_priv(padapter->rtw_wdev);
	struct xmit_priv	*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct wifidirect_info *pwdinfo = &padapter->wdinfo;

	if (_FAIL == rtw_pwr_wakeup(padapter)) {
		ret = -EFAULT;
		goto exit;
	}

	rtw_set_scan_deny(padapter, 1000);

	rtw_scan_abort(padapter);
	#ifdef CONFIG_CONCURRENT_MODE
	if (rtw_buddy_adapter_up(padapter))
		rtw_scan_abort(padapter->pbuddy_adapter);
	#endif /* CONFIG_CONCURRENT_MODE */

	if (padapter->cfg80211_wdinfo.is_ro_ch == true) {
		#ifdef CONFIG_CONCURRENT_MODE
		DBG_8192C("%s, extend ro ch time\n", __func__);
		_set_timer(&padapter->cfg80211_wdinfo.remain_on_ch_timer, pwdinfo->ext_listen_period);
		#endif /* CONFIG_CONCURRENT_MODE */
	}

#ifdef CONFIG_CONCURRENT_MODE
	if (check_buddy_fwstate(padapter, _FW_LINKED)) {
		u8 co_channel=0xff;
		PADAPTER pbuddy_adapter = padapter->pbuddy_adapter;
		struct mlme_ext_priv *pbuddy_mlmeext = &pbuddy_adapter->mlmeextpriv;

		co_channel = rtw_get_oper_ch(padapter);

		if (tx_ch != pbuddy_mlmeext->cur_channel) {
			if (atomic_read(&pwdev_priv->switch_ch_to)==1) {
				DBG_8192C("%s, issue nulldata pwrbit=1\n", __func__);
				issue_nulldata(padapter->pbuddy_adapter, NULL, 1, 3, 500);

				atomic_set(&pwdev_priv->switch_ch_to, 0);

			}

			DBG_8192C("%s, set switch ch timer, period=%d\n", __func__, pwdinfo->ext_listen_period);
			_set_timer(&pwdinfo->ap_p2p_switch_timer, pwdinfo->ext_listen_period);
		}

		if (!check_fwstate(&padapter->mlmepriv, _FW_LINKED))
			pmlmeext->cur_channel = tx_ch;

		if (tx_ch != co_channel)
			set_channel_bwmode(padapter, tx_ch, HAL_PRIME_CHNL_OFFSET_DONT_CARE, HT_CHANNEL_WIDTH_20);
	} else
#endif /* CONFIG_CONCURRENT_MODE */
	if (tx_ch != rtw_get_oper_ch(padapter)) {
		if (!check_fwstate(&padapter->mlmepriv, _FW_LINKED))
			pmlmeext->cur_channel = tx_ch;
		set_channel_bwmode(padapter, tx_ch, HAL_PRIME_CHNL_OFFSET_DONT_CARE, HT_CHANNEL_WIDTH_20);
	}

	/* starting alloc mgmt frame to dump it */
	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		/* ret = -ENOMEM; */
		ret = _FAIL;
		goto exit;
	}

	/* update attribute */
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(padapter, pattrib);
	pattrib->retry_ctrl = false;

	memset(pmgntframe->buf_addr, 0, WLANHDR_OFFSET + TXDESC_OFFSET);

	pframe = (u8 *)(pmgntframe->buf_addr) + TXDESC_OFFSET;

	memcpy(pframe, (void*)buf, len);
	pattrib->pktlen = len;

	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;
	/* update seq number */
	pmlmeext->mgnt_seq = GetSequence(pwlanhdr);
	pattrib->seqnum = pmlmeext->mgnt_seq;
	pmlmeext->mgnt_seq++;

#ifdef CONFIG_WFD
	{
		struct wifi_display_info	*pwfd_info;

		pwfd_info = padapter->wdinfo.wfd_info;

		if (true == pwfd_info->wfd_enable)
		{
			rtw_append_wfd_ie(padapter, pframe, &pattrib->pktlen);
		}
	}
#endif /*  CONFIG_WFD */

	pattrib->last_txcmdsz = pattrib->pktlen;

	if (dump_mgntframe_and_wait_ack(padapter, pmgntframe) != _SUCCESS)
	{
		ack = false;
		ret = _FAIL;

		#ifdef CONFIG_DEBUG_CFG80211
		DBG_8192C("%s, ack == _FAIL\n", __func__);
		#endif
	}
	else
	{
		#ifdef CONFIG_DEBUG_CFG80211
		DBG_8192C("%s, ack=%d, ok!\n", __func__, ack);
		#endif
		ret = _SUCCESS;
	}

exit:

	#ifdef CONFIG_DEBUG_CFG80211
	DBG_8192C("%s, ret=%d\n", __func__, ret);
	#endif

	return ret;
}

static int cfg80211_rtw_mgmt_tx(struct wiphy *wiphy,
	struct wireless_dev *wdev,
	struct ieee80211_channel *chan,
	bool offchan,
	unsigned int wait,
	const u8 *buf, size_t len,
	bool no_cck,
	bool dont_wait_for_ack,
	u64 *cookie)
{
	_adapter *padapter = (_adapter *)wiphy_to_adapter(wiphy);
	struct rtw_wdev_priv *pwdev_priv = wdev_to_priv(padapter->rtw_wdev);
	int ret = 0;
	int tx_ret;
	u32 dump_limit = RTW_MAX_MGMT_TX_CNT;
	u32 dump_cnt = 0;
	bool ack = true;
	u8 tx_ch = (u8)ieee80211_frequency_to_channel(chan->center_freq);
	u8 category, action;
	int type = (-1);
	u32 start = rtw_get_current_time();

	/* cookie generation */
	*cookie = (unsigned long) buf;

#ifdef CONFIG_DEBUG_CFG80211
	DBG_871X(FUNC_ADPT_FMT" len=%zu, ch=%d\n",
		 FUNC_ADPT_ARG(padapter), len, tx_ch);
#endif /* CONFIG_DEBUG_CFG80211 */

	/* indicate ack before issue frame to avoid racing with rsp frame */
	rtw_cfg80211_mgmt_tx_status(padapter, *cookie, buf, len, ack, GFP_KERNEL);

	if (rtw_action_frame_parse(buf, len, &category, &action) == false) {
		DBG_8192C(FUNC_ADPT_FMT" frame_control:0x%x\n", FUNC_ADPT_ARG(padapter),
			le16_to_cpu(((struct rtw_ieee80211_hdr_3addr *)buf)->frame_ctl));
		goto exit;
	}

	DBG_8192C("RTW_Tx:tx_ch=%d, da=%pM\n", tx_ch, GetAddr1Ptr(buf));
	#ifdef CONFIG_P2P
	if ((type = rtw_p2p_check_frames(padapter, buf, len, true)) >= 0)
		goto dump;
	#endif
	if (category == RTW_WLAN_CATEGORY_PUBLIC)
		DBG_871X("RTW_Tx:%s\n", action_public_str(action));
	else
		DBG_871X("RTW_Tx:category(%u), action(%u)\n", category, action);

dump:
	do {
		dump_cnt++;
		tx_ret = _cfg80211_rtw_mgmt_tx(padapter, tx_ch, buf, len);
	} while (dump_cnt < dump_limit && tx_ret != _SUCCESS);

	if (tx_ret != _SUCCESS || dump_cnt > 1) {
		DBG_871X(FUNC_ADPT_FMT" %s (%d/%d) in %d ms\n", FUNC_ADPT_ARG(padapter),
			tx_ret==_SUCCESS?"OK":"FAIL", dump_cnt, dump_limit, rtw_get_passing_time_ms(start));
	}

	switch (type) {
	case P2P_GO_NEGO_CONF:
		rtw_clear_scan_deny(padapter);
		break;
	case P2P_INVIT_RESP:
		if (pwdev_priv->invit_info.flags & BIT(0)
			&& pwdev_priv->invit_info.status == 0)
		{
			DBG_871X(FUNC_ADPT_FMT" agree with invitation of persistent group\n",
				FUNC_ADPT_ARG(padapter));
			rtw_set_scan_deny(padapter, 5000);
			rtw_pwr_wakeup_ex(padapter, 5000);
			rtw_clear_scan_deny(padapter);
		}
		break;
	}

exit:
	return ret;
}

static void cfg80211_rtw_mgmt_frame_register(struct wiphy *wiphy,
	struct wireless_dev *wdev,
	u16 frame_type, bool reg)
{
	_adapter *adapter = wiphy_to_adapter(wiphy);

#ifdef CONFIG_DEBUG_CFG80211
	DBG_871X(FUNC_ADPT_FMT" frame_type:%x, reg:%d\n", FUNC_ADPT_ARG(adapter),
		frame_type, reg);
#endif

	if (frame_type != (IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_PROBE_REQ))
		return;

	return;
}

static int rtw_cfg80211_set_beacon_wpsp2pie(struct net_device *ndev, char *buf, int len)
{
	int ret = 0;
	uint wps_ielen = 0;
	u8 *wps_ie;
	u32	p2p_ielen = 0;
	u8 wps_oui[8]={0x0,0x50,0xf2,0x04};
	u8 *p2p_ie;
	u32	wfd_ielen = 0;
	u8 *wfd_ie;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(ndev);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	struct mlme_ext_priv *pmlmeext = &(padapter->mlmeextpriv);

	DBG_871X(FUNC_NDEV_FMT" ielen=%d\n", FUNC_NDEV_ARG(ndev), len);

	if (len>0)
	{
		if ((wps_ie = rtw_get_wps_ie(buf, len, NULL, &wps_ielen)))
		{
			#ifdef CONFIG_DEBUG_CFG80211
			DBG_8192C("bcn_wps_ielen=%d\n", wps_ielen);
			#endif

			if (pmlmepriv->wps_beacon_ie)
			{
				u32 free_len = pmlmepriv->wps_beacon_ie_len;
				pmlmepriv->wps_beacon_ie_len = 0;
				kfree(pmlmepriv->wps_beacon_ie);
				pmlmepriv->wps_beacon_ie = NULL;
			}

			pmlmepriv->wps_beacon_ie = kmalloc(wps_ielen, GFP_KERNEL);
			if (pmlmepriv->wps_beacon_ie == NULL) {
				DBG_8192C("%s()-%d: kmalloc() ERROR!\n", __func__, __LINE__);
				return -EINVAL;

			}

			memcpy(pmlmepriv->wps_beacon_ie, wps_ie, wps_ielen);
			pmlmepriv->wps_beacon_ie_len = wps_ielen;

			update_beacon(padapter, _VENDOR_SPECIFIC_IE_, wps_oui, true);

		}

		#ifdef CONFIG_P2P
		if ((p2p_ie=rtw_get_p2p_ie(buf, len, NULL, &p2p_ielen)))
		{
			#ifdef CONFIG_DEBUG_CFG80211
			DBG_8192C("bcn_p2p_ielen=%d\n", p2p_ielen);
			#endif

			if (pmlmepriv->p2p_beacon_ie)
			{
				u32 free_len = pmlmepriv->p2p_beacon_ie_len;
				pmlmepriv->p2p_beacon_ie_len = 0;
				kfree(pmlmepriv->p2p_beacon_ie);
				pmlmepriv->p2p_beacon_ie = NULL;
			}

			pmlmepriv->p2p_beacon_ie = kmalloc(p2p_ielen, GFP_KERNEL);
			if (pmlmepriv->p2p_beacon_ie == NULL) {
				DBG_8192C("%s()-%d: kmalloc() ERROR!\n", __func__, __LINE__);
				return -EINVAL;

			}

			memcpy(pmlmepriv->p2p_beacon_ie, p2p_ie, p2p_ielen);
			pmlmepriv->p2p_beacon_ie_len = p2p_ielen;

		}
		#endif /* CONFIG_P2P */

		#ifdef CONFIG_WFD
		if (rtw_get_wfd_ie(buf, len, NULL, &wfd_ielen))
		{
			#ifdef CONFIG_DEBUG_CFG80211
			DBG_8192C("bcn_wfd_ielen=%d\n", wfd_ielen);
			#endif

			if (pmlmepriv->wfd_beacon_ie)
			{
				u32 free_len = pmlmepriv->wfd_beacon_ie_len;
				pmlmepriv->wfd_beacon_ie_len = 0;
				kfree(pmlmepriv->wfd_beacon_ie);
				pmlmepriv->wfd_beacon_ie = NULL;
			}

			pmlmepriv->wfd_beacon_ie = kmalloc(wfd_ielen, GFP_KERNEL);
			if (pmlmepriv->wfd_beacon_ie == NULL) {
				DBG_8192C("%s()-%d: kmalloc() ERROR!\n", __func__, __LINE__);
				return -EINVAL;

			}
			rtw_get_wfd_ie(buf, len, pmlmepriv->wfd_beacon_ie, &pmlmepriv->wfd_beacon_ie_len);
		}
		#endif /* CONFIG_WFD */

		pmlmeext->bstart_bss = true;

	}

	return ret;
}

static int rtw_cfg80211_set_probe_resp_wpsp2pie(struct net_device *net, char *buf, int len)
{
	int ret = 0;
	uint wps_ielen = 0;
	u8 *wps_ie;
	u32	p2p_ielen = 0;
	u8 *p2p_ie;
	u32	wfd_ielen = 0;
	u8 *wfd_ie;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(net);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);

#ifdef CONFIG_DEBUG_CFG80211
	DBG_8192C("%s, ielen=%d\n", __func__, len);
#endif

	if (len>0)
	{
		if ((wps_ie = rtw_get_wps_ie(buf, len, NULL, &wps_ielen)))
		{
			uint	attr_contentlen = 0;
			u16	uconfig_method, *puconfig_method = NULL;

			#ifdef CONFIG_DEBUG_CFG80211
			DBG_8192C("probe_resp_wps_ielen=%d\n", wps_ielen);
			#endif

			if (pmlmepriv->wps_probe_resp_ie)
			{
				u32 free_len = pmlmepriv->wps_probe_resp_ie_len;
				pmlmepriv->wps_probe_resp_ie_len = 0;
				kfree(pmlmepriv->wps_probe_resp_ie);
				pmlmepriv->wps_probe_resp_ie = NULL;
			}

			pmlmepriv->wps_probe_resp_ie = kmalloc(wps_ielen, GFP_KERNEL);
			if (pmlmepriv->wps_probe_resp_ie == NULL) {
				DBG_8192C("%s()-%d: kmalloc() ERROR!\n", __func__, __LINE__);
				return -EINVAL;

			}

			/* add PUSH_BUTTON config_method by driver self in wpsie of probe_resp at GO Mode */
			if ((puconfig_method = (u16*)rtw_get_wps_attr_content(wps_ie, wps_ielen, WPS_ATTR_CONF_METHOD , NULL, &attr_contentlen)) != NULL)
			{
				uconfig_method = WPS_CM_PUSH_BUTTON;
				uconfig_method = cpu_to_be16(uconfig_method);

				*puconfig_method |= uconfig_method;
			}

			memcpy(pmlmepriv->wps_probe_resp_ie, wps_ie, wps_ielen);
			pmlmepriv->wps_probe_resp_ie_len = wps_ielen;

		}

		#ifdef CONFIG_P2P
		if ((p2p_ie=rtw_get_p2p_ie(buf, len, NULL, &p2p_ielen)))
		{
			u8 is_GO = false;
			u32 attr_contentlen = 0;
			u16 cap_attr=0;

			#ifdef CONFIG_DEBUG_CFG80211
			DBG_8192C("probe_resp_p2p_ielen=%d\n", p2p_ielen);
			#endif

			/* Check P2P Capability ATTR */
			if (rtw_get_p2p_attr_content(p2p_ie, p2p_ielen, P2P_ATTR_CAPABILITY, (u8*)&cap_attr, (uint*) &attr_contentlen))
			{
				u8 grp_cap=0;
				cap_attr = le16_to_cpu(cap_attr);
				grp_cap = (u8)((cap_attr >> 8)&0xff);

				is_GO = (grp_cap&BIT(0)) ? true:false;

				if (is_GO)
					DBG_8192C("Got P2P Capability Attr, grp_cap=0x%x, is_GO\n", grp_cap);
			}

			if (is_GO == false)
			{
				if (pmlmepriv->p2p_probe_resp_ie)
				{
					u32 free_len = pmlmepriv->p2p_probe_resp_ie_len;
					pmlmepriv->p2p_probe_resp_ie_len = 0;
					kfree(pmlmepriv->p2p_probe_resp_ie);
					pmlmepriv->p2p_probe_resp_ie = NULL;
				}

				pmlmepriv->p2p_probe_resp_ie = kmalloc(p2p_ielen, GFP_KERNEL);
				if (pmlmepriv->p2p_probe_resp_ie == NULL) {
					DBG_8192C("%s()-%d: kmalloc() ERROR!\n", __func__, __LINE__);
					return -EINVAL;

				}
				memcpy(pmlmepriv->p2p_probe_resp_ie, p2p_ie, p2p_ielen);
				pmlmepriv->p2p_probe_resp_ie_len = p2p_ielen;
			}
			else
			{
				if (pmlmepriv->p2p_go_probe_resp_ie)
				{
					u32 free_len = pmlmepriv->p2p_go_probe_resp_ie_len;
					pmlmepriv->p2p_go_probe_resp_ie_len = 0;
					kfree(pmlmepriv->p2p_go_probe_resp_ie);
					pmlmepriv->p2p_go_probe_resp_ie = NULL;
				}

				pmlmepriv->p2p_go_probe_resp_ie = kmalloc(p2p_ielen, GFP_KERNEL);
				if (pmlmepriv->p2p_go_probe_resp_ie == NULL) {
					DBG_8192C("%s()-%d: kmalloc() ERROR!\n", __func__, __LINE__);
					return -EINVAL;

				}
				memcpy(pmlmepriv->p2p_go_probe_resp_ie, p2p_ie, p2p_ielen);
				pmlmepriv->p2p_go_probe_resp_ie_len = p2p_ielen;
			}

		}
		#endif /* CONFIG_P2P */

		#ifdef CONFIG_WFD
		if (rtw_get_wfd_ie(buf, len, NULL, &wfd_ielen))
		{
			#ifdef CONFIG_DEBUG_CFG80211
			DBG_8192C("probe_resp_wfd_ielen=%d\n", wfd_ielen);
			#endif

			if (pmlmepriv->wfd_probe_resp_ie)
			{
				u32 free_len = pmlmepriv->wfd_probe_resp_ie_len;
				pmlmepriv->wfd_probe_resp_ie_len = 0;
				kfree(pmlmepriv->wfd_probe_resp_ie);
				pmlmepriv->wfd_probe_resp_ie = NULL;
			}

			pmlmepriv->wfd_probe_resp_ie = kmalloc(wfd_ielen, GFP_KERNEL);
			if (pmlmepriv->wfd_probe_resp_ie == NULL) {
				DBG_8192C("%s()-%d: kmalloc() ERROR!\n", __func__, __LINE__);
				return -EINVAL;

			}
			rtw_get_wfd_ie(buf, len, pmlmepriv->wfd_probe_resp_ie, &pmlmepriv->wfd_probe_resp_ie_len);
		}
		#endif /* CONFIG_WFD */

	}

	return ret;
}

static int rtw_cfg80211_set_assoc_resp_wpsp2pie(struct net_device *net, char *buf, int len)
{
	int ret = 0;
	_adapter *padapter = (_adapter *)rtw_netdev_priv(net);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);

	DBG_8192C("%s, ielen=%d\n", __func__, len);

	if (len>0)
	{
		if (pmlmepriv->wps_assoc_resp_ie)
		{
			u32 free_len = pmlmepriv->wps_assoc_resp_ie_len;
			pmlmepriv->wps_assoc_resp_ie_len = 0;
			kfree(pmlmepriv->wps_assoc_resp_ie);
			pmlmepriv->wps_assoc_resp_ie = NULL;
		}

		pmlmepriv->wps_assoc_resp_ie = kmalloc(len, GFP_KERNEL, GFP_KERNEL);
		if (pmlmepriv->wps_assoc_resp_ie == NULL) {
			DBG_8192C("%s()-%d: kmalloc() ERROR!\n", __func__, __LINE__);
			return -EINVAL;

		}
		memcpy(pmlmepriv->wps_assoc_resp_ie, buf, len);
		pmlmepriv->wps_assoc_resp_ie_len = len;
	}

	return ret;
}

int rtw_cfg80211_set_mgnt_wpsp2pie(struct net_device *net, char *buf, int len,
	int type)
{
	int ret = 0;
	uint wps_ielen = 0;
	u32	p2p_ielen = 0;

#ifdef CONFIG_DEBUG_CFG80211
	DBG_8192C("%s, ielen=%d\n", __func__, len);
#endif

	if (	(rtw_get_wps_ie(buf, len, NULL, &wps_ielen) && (wps_ielen>0))
		#ifdef CONFIG_P2P
		|| (rtw_get_p2p_ie(buf, len, NULL, &p2p_ielen) && (p2p_ielen>0))
		#endif
	)
	{
		if (net != NULL)
		{
			switch (type)
			{
				case 0x1: /* BEACON */
				ret = rtw_cfg80211_set_beacon_wpsp2pie(net, buf, len);
				break;
				case 0x2: /* PROBE_RESP */
				ret = rtw_cfg80211_set_probe_resp_wpsp2pie(net, buf, len);
				break;
				case 0x4: /* ASSOC_RESP */
				ret = rtw_cfg80211_set_assoc_resp_wpsp2pie(net, buf, len);
				break;
			}
		}
	}

	return ret;
}

static struct cfg80211_ops rtw_cfg80211_ops = {
	.change_virtual_intf = cfg80211_rtw_change_iface,
	.add_key = cfg80211_rtw_add_key,
	.get_key = cfg80211_rtw_get_key,
	.del_key = cfg80211_rtw_del_key,
	.set_default_key = cfg80211_rtw_set_default_key,
	.get_station = cfg80211_rtw_get_station,
	.scan = cfg80211_rtw_scan,
	.set_wiphy_params = cfg80211_rtw_set_wiphy_params,
	.connect = cfg80211_rtw_connect,
	.disconnect = cfg80211_rtw_disconnect,
	.join_ibss = cfg80211_rtw_join_ibss,
	.leave_ibss = cfg80211_rtw_leave_ibss,
	.set_tx_power = cfg80211_rtw_set_txpower,
	.get_tx_power = cfg80211_rtw_get_txpower,
	.set_power_mgmt = cfg80211_rtw_set_power_mgmt,
	.set_pmksa = cfg80211_rtw_set_pmksa,
	.del_pmksa = cfg80211_rtw_del_pmksa,
	.flush_pmksa = cfg80211_rtw_flush_pmksa,

#ifdef CONFIG_92D_AP_MODE
	.add_virtual_intf = add_virtual_intf,
	.del_virtual_intf = cfg80211_rtw_del_virtual_intf,

	#if !defined(COMPAT_KERNEL_RELEASE)
	.add_beacon = cfg80211_rtw_add_beacon,
	.set_beacon = cfg80211_rtw_set_beacon,
	.del_beacon = cfg80211_rtw_del_beacon,
	#else
	.start_ap = cfg80211_rtw_start_ap,
	.change_beacon = cfg80211_rtw_change_beacon,
	.stop_ap = cfg80211_rtw_stop_ap,
	#endif

	.add_station = cfg80211_rtw_add_station,
	.del_station = cfg80211_rtw_del_station,
	.change_station = cfg80211_rtw_change_station,
	.dump_station = cfg80211_rtw_dump_station,
	.change_bss = cfg80211_rtw_change_bss,
#endif /* CONFIG_92D_AP_MODE */

#ifdef CONFIG_P2P
	.remain_on_channel = cfg80211_rtw_remain_on_channel,
	.cancel_remain_on_channel = cfg80211_rtw_cancel_remain_on_channel,
#endif

	.mgmt_tx = cfg80211_rtw_mgmt_tx,
	.mgmt_frame_register = cfg80211_rtw_mgmt_frame_register,
};

static void rtw_cfg80211_init_ht_capab(struct ieee80211_sta_ht_cap *ht_cap, enum ieee80211_band band, u8 rf_type)
{

#define MAX_BIT_RATE_40MHZ_MCS15	300	/* Mbps */
#define MAX_BIT_RATE_40MHZ_MCS7		150	/* Mbps */

	ht_cap->ht_supported = true;

	ht_cap->cap = IEEE80211_HT_CAP_SUP_WIDTH_20_40 |
					IEEE80211_HT_CAP_SGI_40 | IEEE80211_HT_CAP_SGI_20 |
					IEEE80211_HT_CAP_DSSSCCK40 | IEEE80211_HT_CAP_MAX_AMSDU;

	/*
	 *Maximum length of AMPDU that the STA can receive.
	 *Length = 2 ^ (13 + max_ampdu_length_exp) - 1 (octets)
	 */
	ht_cap->ampdu_factor = IEEE80211_HT_MAX_AMPDU_64K;

	/*Minimum MPDU start spacing , */
	ht_cap->ampdu_density = IEEE80211_HT_MPDU_DENSITY_16;

	ht_cap->mcs.tx_params = IEEE80211_HT_MCS_TX_DEFINED;

	/*
	 *hw->wiphy->bands[IEEE80211_BAND_2GHZ]
	 *base on ant_num
	 *rx_mask: RX mask
	 *if rx_ant =1 rx_mask[0]=0xff;==>MCS0-MCS7
	 *if rx_ant =2 rx_mask[1]=0xff;==>MCS8-MCS15
	 *if rx_ant >=3 rx_mask[2]=0xff;
	 *if BW_40 rx_mask[4]=0x01;
	 *highest supported RX rate
	 */
	if (rf_type == RF_1T1R)
	{
		ht_cap->mcs.rx_mask[0] = 0xFF;
		ht_cap->mcs.rx_mask[1] = 0x00;
		ht_cap->mcs.rx_mask[4] = 0x01;

		ht_cap->mcs.rx_highest = MAX_BIT_RATE_40MHZ_MCS7;
	}
	else if ((rf_type == RF_1T2R) || (rf_type==RF_2T2R))
	{
		ht_cap->mcs.rx_mask[0] = 0xFF;
		ht_cap->mcs.rx_mask[1] = 0xFF;
		ht_cap->mcs.rx_mask[4] = 0x01;

		ht_cap->mcs.rx_highest = MAX_BIT_RATE_40MHZ_MCS15;
	}
	else
	{
		DBG_8192C("%s, error rf_type=%d\n", __func__, rf_type);
	}
}

void rtw_cfg80211_init_wiphy(_adapter *padapter)
{
	u8 rf_type;
	struct ieee80211_supported_band *bands;
	struct wireless_dev *pwdev = padapter->rtw_wdev;
	struct wiphy *wiphy = pwdev->wiphy;

	rtw_hal_get_hwreg(padapter, HW_VAR_RF_TYPE, (u8 *)(&rf_type));

	DBG_8192C("%s:rf_type=%d\n", __func__, rf_type);

	/* if (padapter->registrypriv.wireless_mode & WIRELESS_11G) */
	{
		bands = wiphy->bands[IEEE80211_BAND_2GHZ];
		if (bands)
			rtw_cfg80211_init_ht_capab(&bands->ht_cap, IEEE80211_BAND_2GHZ, rf_type);
	}

	/* if (padapter->registrypriv.wireless_mode & WIRELESS_11A) */
	{
		bands = wiphy->bands[IEEE80211_BAND_5GHZ];
		if (bands)
			rtw_cfg80211_init_ht_capab(&bands->ht_cap, IEEE80211_BAND_5GHZ, rf_type);
	}
}

static void rtw_cfg80211_preinit_wiphy(_adapter *padapter, struct wiphy *wiphy)
{

	wiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;

	wiphy->max_scan_ssids = RTW_SSID_SCAN_AMOUNT;
	wiphy->max_scan_ie_len = RTW_SCAN_IE_LEN_MAX;
	wiphy->max_num_pmkids = RTW_MAX_NUM_PMKIDS;

	wiphy->max_remain_on_channel_duration = RTW_MAX_REMAIN_ON_CHANNEL_DURATION;

	wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION) |
		 		 BIT(NL80211_IFTYPE_ADHOC) |
#ifdef CONFIG_92D_AP_MODE
				 BIT(NL80211_IFTYPE_AP) |
				 BIT(NL80211_IFTYPE_MONITOR) |
#endif
#if defined(CONFIG_P2P)
				 BIT(NL80211_IFTYPE_P2P_CLIENT) |
				 BIT(NL80211_IFTYPE_P2P_GO) |
#endif
				 0;

#ifdef CONFIG_92D_AP_MODE
	wiphy->mgmt_stypes = rtw_cfg80211_default_mgmt_stypes;
#endif /* CONFIG_92D_AP_MODE */

	wiphy->software_iftypes |= BIT(NL80211_IFTYPE_MONITOR);

	/*
	wiphy->iface_combinations = &rtw_combinations;
	wiphy->n_iface_combinations = 1;
	*/

	wiphy->cipher_suites = rtw_cipher_suites;
	wiphy->n_cipher_suites = ARRAY_SIZE(rtw_cipher_suites);

	/* if (padapter->registrypriv.wireless_mode & WIRELESS_11G) */
		wiphy->bands[IEEE80211_BAND_2GHZ] = rtw_spt_band_alloc(IEEE80211_BAND_2GHZ);
	/* if (padapter->registrypriv.wireless_mode & WIRELESS_11A) */
		wiphy->bands[IEEE80211_BAND_5GHZ] = rtw_spt_band_alloc(IEEE80211_BAND_5GHZ);

	wiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;
	wiphy->flags |= WIPHY_FLAG_OFFCHAN_TX | WIPHY_FLAG_HAVE_AP_SME;

	if (padapter->registrypriv.power_mgnt != PS_MODE_ACTIVE)
		wiphy->flags |= WIPHY_FLAG_PS_ON_BY_DEFAULT;
	else
		wiphy->flags &= ~WIPHY_FLAG_PS_ON_BY_DEFAULT;
}

int rtw_wdev_alloc(_adapter *padapter, struct device *dev)
{
	int ret = 0;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct rtw_wdev_priv *pwdev_priv;
	struct net_device *pnetdev = padapter->pnetdev;

	DBG_8192C("%s(padapter=%p)\n", __func__, padapter);

	/* wiphy */
	wiphy = wiphy_new(&rtw_cfg80211_ops, sizeof(struct rtw_wdev_priv));
	if (!wiphy) {
		DBG_8192C("Couldn't allocate wiphy device\n");
		ret = -ENOMEM;
		goto exit;
	}
	set_wiphy_dev(wiphy, dev);
	rtw_cfg80211_preinit_wiphy(padapter, wiphy);

	ret = wiphy_register(wiphy);
	if (ret < 0) {
		DBG_8192C("Couldn't register wiphy device\n");
		goto free_wiphy;
	}

	/*  wdev */
	wdev = (struct wireless_dev *)kzalloc(sizeof(struct wireless_dev), GFP_KERNEL);
	if (!wdev) {
		DBG_8192C("Couldn't allocate wireless device\n");
		ret = -ENOMEM;
		goto unregister_wiphy;
	}
	wdev->wiphy = wiphy;
	wdev->netdev = pnetdev;
	wdev->iftype = NL80211_IFTYPE_MONITOR; /*  for rtw_setopmode_cmd() in cfg80211_rtw_change_iface() */
	padapter->rtw_wdev = wdev;
	pnetdev->ieee80211_ptr = wdev;

	/* init pwdev_priv */
	pwdev_priv = wdev_to_priv(wdev);
	pwdev_priv->rtw_wdev = wdev;
	pwdev_priv->pmon_ndev = NULL;
	pwdev_priv->ifname_mon[0] = '\0';
	pwdev_priv->padapter = padapter;
	pwdev_priv->scan_request = NULL;
	spin_lock_init(&pwdev_priv->scan_req_lock);

	pwdev_priv->p2p_enabled = false;
	pwdev_priv->provdisc_req_issued = false;
	rtw_wdev_invit_info_init(&pwdev_priv->invit_info);

	pwdev_priv->bandroid_scan = false;

	if (padapter->registrypriv.power_mgnt != PS_MODE_ACTIVE)
		pwdev_priv->power_mgmt = true;
	else
		pwdev_priv->power_mgmt = false;

#ifdef CONFIG_CONCURRENT_MODE
	atomic_set(&pwdev_priv->switch_ch_to, 1);
	atomic_set(&pwdev_priv->ro_ch_to, 1);
#endif

	return ret;

	kfree(wdev);
unregister_wiphy:
	wiphy_unregister(wiphy);
 free_wiphy:
	wiphy_free(wiphy);
exit:
	return ret;
}

void rtw_wdev_free(struct wireless_dev *wdev)
{
	struct rtw_wdev_priv *pwdev_priv;

	DBG_8192C("%s(wdev=%p)\n", __func__, wdev);

	if (!wdev)
		return;

	pwdev_priv = wdev_to_priv(wdev);

	rtw_spt_band_free(wdev->wiphy->bands[IEEE80211_BAND_2GHZ]);
	rtw_spt_band_free(wdev->wiphy->bands[IEEE80211_BAND_5GHZ]);

	wiphy_free(wdev->wiphy);

	kfree(wdev);
}

void rtw_wdev_unregister(struct wireless_dev *wdev)
{
	struct rtw_wdev_priv *pwdev_priv;

	DBG_8192C("%s(wdev=%p)\n", __func__, wdev);

	if (!wdev)
		return;

	pwdev_priv = wdev_to_priv(wdev);

	rtw_cfg80211_indicate_scan_done(pwdev_priv, true);

	if (pwdev_priv->pmon_ndev) {
		DBG_8192C("%s, unregister monitor interface\n", __func__);
		unregister_netdev(pwdev_priv->pmon_ndev);
	}

	wiphy_unregister(wdev->wiphy);
}

#endif /* CONFIG_IOCTL_CFG80211 */
