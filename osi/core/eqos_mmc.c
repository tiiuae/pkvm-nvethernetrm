/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <osi_common.h>
#include <osi_core.h>
#include <osd.h>
#include "eqos_mmc.h"
#include "eqos_core.h"

/**
 *	update_mmc_val - function to read resgister and return vlaue to callee
 *
 *	@osi_core: OSI core private data structure.
 *	@last_vlaue: previous value of stats variable.
 *	@offset: HW register offset
 *
 *	Algorithm: Read the registers, check for boundary, if more, reset
 *	counters else return same to caller.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *	2) osi_core->osd should be populated
 *
 *	Protection: None
 *
 *      Return: unsigned value
 */
static inline unsigned long update_mmc_val(struct osi_core_priv_data *osi_core,
					   unsigned long last_value,
					   unsigned long offset)
{
	unsigned long long temp;
	unsigned int value = osi_readl((unsigned char *)osi_core->base +
				       offset);

	temp = (unsigned long long)last_value;
	temp = temp + (unsigned long long)value;
	if (temp > ULONG_MAX) {
		osd_err(osi_core->osd, "Value overflow for offset = 0x%x resetting  all counters\n",
			offset);
		eqos_reset_mmc(osi_core);
	} else {
		return (unsigned long)temp;
	}

	return 0;
}

/**
 *	eqos_reset_mmc - To reset MMC registers and ether_mmc_counter structure
 *	variable
 *	@osi_core: OSI core private data structure.
 *
 *	Algorithm: reset HW counter and structure variable value.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *	2) osi_core->osd should be populated
 *
 *	Protection: None
 *
 *      Return: None
 */
void eqos_reset_mmc(struct osi_core_priv_data *osi_core)
{
	unsigned int value;

	value = osi_readl((unsigned char *)osi_core->base + EQOS_MMC_CNTRL);
	/* self-clear bit in one clock cycle */
	value |= EQOS_MMC_CNTRL_CNTRST;
	osi_writel(value, (unsigned char *)osi_core->base + EQOS_MMC_CNTRL);
	osi_memset(&osi_core->mmc, 0U, sizeof(struct osi_mmc_counters));
}

/**
 *	eqos_read_mmc - To read MMC registers and ether_mmc_counter structure
 *	variable
 *	@osi_core: OSI core private data structure.
 *
 *	Algorithm: Pass register offset and old value to helper function and
 *	update structure.
 *
 *	Dependencies:
 *	1) MAC should be init and started. see osi_start_mac()
 *	2) osi_core->osd should be populated
 *
 *	Protection: None
 *
 *      Return: None
 */
void eqos_read_mmc(struct osi_core_priv_data *osi_core)
{
	struct osi_mmc_counters *mmc = &osi_core->mmc;

	mmc->mmc_tx_octetcount_gb =
		update_mmc_val(osi_core, mmc->mmc_tx_octetcount_gb,
			       MMC_TXOCTETCOUNT_GB);
	mmc->mmc_tx_framecount_gb =
		update_mmc_val(osi_core, mmc->mmc_tx_framecount_gb,
			       MMC_TXPACKETCOUNT_GB);
	mmc->mmc_tx_broadcastframe_g =
		update_mmc_val(osi_core, mmc->mmc_tx_broadcastframe_g,
			       MMC_TXBROADCASTPACKETS_G);
	mmc->mmc_tx_multicastframe_g =
		update_mmc_val(osi_core, mmc->mmc_tx_multicastframe_g,
			       MMC_TXMULTICASTPACKETS_G);
	mmc->mmc_tx_64_octets_gb =
		update_mmc_val(osi_core, mmc->mmc_tx_64_octets_gb,
			       MMC_TX64OCTETS_GB);
	mmc->mmc_tx_65_to_127_octets_gb =
		update_mmc_val(osi_core, mmc->mmc_tx_65_to_127_octets_gb,
			       MMC_TX65TO127OCTETS_GB);
	mmc->mmc_tx_128_to_255_octets_gb =
		update_mmc_val(osi_core, mmc->mmc_tx_128_to_255_octets_gb,
			       MMC_TX128TO255OCTETS_GB);
	mmc->mmc_tx_256_to_511_octets_gb =
		update_mmc_val(osi_core, mmc->mmc_tx_256_to_511_octets_gb,
			       MMC_TX256TO511OCTETS_GB);
	mmc->mmc_tx_512_to_1023_octets_gb =
		update_mmc_val(osi_core, mmc->mmc_tx_512_to_1023_octets_gb,
			       MMC_TX512TO1023OCTETS_GB);
	mmc->mmc_tx_1024_to_max_octets_gb =
		update_mmc_val(osi_core, mmc->mmc_tx_1024_to_max_octets_gb,
			       MMC_TX1024TOMAXOCTETS_GB);
	mmc->mmc_tx_unicast_gb =
		update_mmc_val(osi_core, mmc->mmc_tx_unicast_gb,
			       MMC_TXUNICASTPACKETS_GB);
	mmc->mmc_tx_multicast_gb =
		update_mmc_val(osi_core, mmc->mmc_tx_multicast_gb,
			       MMC_TXMULTICASTPACKETS_GB);
	mmc->mmc_tx_broadcast_gb =
		update_mmc_val(osi_core, mmc->mmc_tx_broadcast_gb,
			       MMC_TXBROADCASTPACKETS_GB);
	mmc->mmc_tx_underflow_error =
		update_mmc_val(osi_core, mmc->mmc_tx_underflow_error,
			       MMC_TXUNDERFLOWERROR);
	mmc->mmc_tx_singlecol_g =
		update_mmc_val(osi_core, mmc->mmc_tx_singlecol_g,
			       MMC_TXSINGLECOL_G);
	mmc->mmc_tx_multicol_g =
		update_mmc_val(osi_core, mmc->mmc_tx_multicol_g,
			       MMC_TXMULTICOL_G);
	mmc->mmc_tx_deferred =
		update_mmc_val(osi_core, mmc->mmc_tx_deferred,
			       MMC_TXDEFERRED);
	mmc->mmc_tx_latecol =
		update_mmc_val(osi_core, mmc->mmc_tx_latecol,
			       MMC_TXLATECOL);
	mmc->mmc_tx_exesscol =
		update_mmc_val(osi_core, mmc->mmc_tx_exesscol,
			       MMC_TXEXESSCOL);
	mmc->mmc_tx_carrier_error =
		update_mmc_val(osi_core, mmc->mmc_tx_exesscol,
			       MMC_TXCARRIERERROR);
	mmc->mmc_tx_octetcount_g =
		update_mmc_val(osi_core, mmc->mmc_tx_octetcount_g,
			       MMC_TXOCTETCOUNT_G);
	mmc->mmc_tx_framecount_g =
		 update_mmc_val(osi_core, mmc->mmc_tx_framecount_g,
				MMC_TXPACKETSCOUNT_G);
	mmc->mmc_tx_excessdef =
		update_mmc_val(osi_core, mmc->mmc_tx_excessdef,
			       MMC_TXEXCESSDEF);
	mmc->mmc_tx_pause_frame =
		update_mmc_val(osi_core, mmc->mmc_tx_pause_frame,
			       MMC_TXPAUSEPACKETS);
	mmc->mmc_tx_vlan_frame_g =
		update_mmc_val(osi_core, mmc->mmc_tx_vlan_frame_g,
			       MMC_TXVLANPACKETS_G);
	mmc->mmc_tx_osize_frame_g =
		update_mmc_val(osi_core, mmc->mmc_tx_osize_frame_g,
			       MMC_TXOVERSIZE_G);
	mmc->mmc_rx_framecount_gb =
		update_mmc_val(osi_core, mmc->mmc_rx_framecount_gb,
			       MMC_RXPACKETCOUNT_GB);
	mmc->mmc_rx_octetcount_gb =
		update_mmc_val(osi_core, mmc->mmc_rx_octetcount_gb,
			       MMC_RXOCTETCOUNT_GB);
	mmc->mmc_rx_octetcount_g =
		update_mmc_val(osi_core, mmc->mmc_rx_octetcount_g,
			       MMC_RXOCTETCOUNT_G);
	mmc->mmc_rx_broadcastframe_g =
		update_mmc_val(osi_core, mmc->mmc_rx_broadcastframe_g,
			       MMC_RXBROADCASTPACKETS_G);
	mmc->mmc_rx_multicastframe_g =
		update_mmc_val(osi_core, mmc->mmc_rx_multicastframe_g,
			       MMC_RXMULTICASTPACKETS_G);
	mmc->mmc_rx_crc_error =
		update_mmc_val(osi_core, mmc->mmc_rx_crc_error,
			       MMC_RXCRCERROR);
	mmc->mmc_rx_align_error =
		update_mmc_val(osi_core, mmc->mmc_rx_align_error,
			       MMC_RXALIGNMENTERROR);
	mmc->mmc_rx_runt_error =
		update_mmc_val(osi_core, mmc->mmc_rx_runt_error,
			       MMC_RXRUNTERROR);
	mmc->mmc_rx_jabber_error =
		update_mmc_val(osi_core, mmc->mmc_rx_jabber_error,
			       MMC_RXJABBERERROR);
	mmc->mmc_rx_undersize_g =
		update_mmc_val(osi_core, mmc->mmc_rx_undersize_g,
			       MMC_RXUNDERSIZE_G);
	mmc->mmc_rx_oversize_g =
		update_mmc_val(osi_core, mmc->mmc_rx_oversize_g,
			       MMC_RXOVERSIZE_G);
	mmc->mmc_rx_64_octets_gb =
		update_mmc_val(osi_core, mmc->mmc_rx_64_octets_gb,
			       MMC_RX64OCTETS_GB);
	mmc->mmc_rx_65_to_127_octets_gb =
		update_mmc_val(osi_core, mmc->mmc_rx_65_to_127_octets_gb,
			       MMC_RX65TO127OCTETS_GB);
	mmc->mmc_rx_128_to_255_octets_gb =
		update_mmc_val(osi_core, mmc->mmc_rx_128_to_255_octets_gb,
			       MMC_RX128TO255OCTETS_GB);
	mmc->mmc_rx_256_to_511_octets_gb =
		update_mmc_val(osi_core, mmc->mmc_rx_256_to_511_octets_gb,
			       MMC_RX256TO511OCTETS_GB);
	mmc->mmc_rx_512_to_1023_octets_gb =
		update_mmc_val(osi_core, mmc->mmc_rx_512_to_1023_octets_gb,
			       MMC_RX512TO1023OCTETS_GB);
	mmc->mmc_rx_1024_to_max_octets_gb =
		update_mmc_val(osi_core, mmc->mmc_rx_1024_to_max_octets_gb,
			       MMC_RX1024TOMAXOCTETS_GB);
	mmc->mmc_rx_unicast_g =
		update_mmc_val(osi_core, mmc->mmc_rx_unicast_g,
			       MMC_RXUNICASTPACKETS_G);
	mmc->mmc_rx_length_error =
		update_mmc_val(osi_core, mmc->mmc_rx_length_error,
			       MMC_RXLENGTHERROR);
	mmc->mmc_rx_outofrangetype =
		update_mmc_val(osi_core, mmc->mmc_rx_outofrangetype,
			       MMC_RXOUTOFRANGETYPE);
	mmc->mmc_rx_pause_frames =
		update_mmc_val(osi_core, mmc->mmc_rx_pause_frames,
			       MMC_RXPAUSEPACKETS);
	mmc->mmc_rx_fifo_overflow =
		update_mmc_val(osi_core, mmc->mmc_rx_fifo_overflow,
			       MMC_RXFIFOOVERFLOW);
	mmc->mmc_rx_vlan_frames_gb =
		update_mmc_val(osi_core, mmc->mmc_rx_vlan_frames_gb,
			       MMC_RXVLANPACKETS_GB);
	mmc->mmc_rx_watchdog_error =
		update_mmc_val(osi_core, mmc->mmc_rx_watchdog_error,
			       MMC_RXWATCHDOGERROR);
	mmc->mmc_rx_receive_error =
		update_mmc_val(osi_core, mmc->mmc_rx_receive_error,
			       MMC_RXRCVERROR);
	mmc->mmc_rx_ctrl_frames_g =
		update_mmc_val(osi_core, mmc->mmc_rx_ctrl_frames_g,
			       MMC_RXCTRLPACKETS_G);
	mmc->mmc_rx_ipv4_gd =
		update_mmc_val(osi_core, mmc->mmc_rx_ipv4_gd,
			       MMC_RXIPV4_GD_PKTS);
	mmc->mmc_rx_ipv4_hderr =
		update_mmc_val(osi_core, mmc->mmc_rx_ipv4_hderr,
			       MMC_RXIPV4_HDRERR_PKTS);
	mmc->mmc_rx_ipv4_nopay =
		update_mmc_val(osi_core, mmc->mmc_rx_ipv4_nopay,
			       MMC_RXIPV4_NOPAY_PKTS);
	mmc->mmc_rx_ipv4_frag =
		update_mmc_val(osi_core, mmc->mmc_rx_ipv4_frag,
			       MMC_RXIPV4_FRAG_PKTS);
	mmc->mmc_rx_ipv4_udsbl =
		update_mmc_val(osi_core, mmc->mmc_rx_ipv4_udsbl,
			       MMC_RXIPV4_UBSBL_PKTS);
	mmc->mmc_rx_ipv6_gd_octets =
		update_mmc_val(osi_core, mmc->mmc_rx_ipv6_gd_octets,
			       MMC_RXIPV6_GD_PKTS);
	mmc->mmc_rx_ipv6_hderr_octets =
		update_mmc_val(osi_core, mmc->mmc_rx_ipv6_hderr_octets,
			       MMC_RXIPV6_HDRERR_PKTS);
	mmc->mmc_rx_ipv6_nopay_octets =
		update_mmc_val(osi_core, mmc->mmc_rx_ipv6_nopay_octets,
			       MMC_RXIPV6_NOPAY_PKTS);
	mmc->mmc_rx_udp_gd =
		update_mmc_val(osi_core, mmc->mmc_rx_udp_gd,
			       MMC_RXUDP_GD_PKTS);
	mmc->mmc_rx_udp_err =
		update_mmc_val(osi_core, mmc->mmc_rx_udp_err,
			       MMC_RXUDP_ERR_PKTS);
	mmc->mmc_rx_tcp_gd =
		update_mmc_val(osi_core, mmc->mmc_rx_tcp_gd,
			       MMC_RXTCP_GD_PKTS);
	mmc->mmc_rx_tcp_err =
		update_mmc_val(osi_core, mmc->mmc_rx_tcp_err,
			       MMC_RXTCP_ERR_PKTS);
	mmc->mmc_rx_icmp_gd =
		update_mmc_val(osi_core, mmc->mmc_rx_icmp_gd,
			       MMC_RXICMP_GD_PKTS);
	mmc->mmc_rx_icmp_err =
		update_mmc_val(osi_core, mmc->mmc_rx_icmp_err,
			       MMC_RXICMP_ERR_PKTS);
	mmc->mmc_rx_ipv4_gd_octets =
		update_mmc_val(osi_core, mmc->mmc_rx_ipv4_gd_octets,
			       MMC_RXIPV4_GD_OCTETS);
	mmc->mmc_rx_ipv4_hderr_octets =
		update_mmc_val(osi_core, mmc->mmc_rx_ipv4_hderr_octets,
			       MMC_RXIPV4_HDRERR_OCTETS);
	mmc->mmc_rx_ipv4_nopay_octets =
		update_mmc_val(osi_core, mmc->mmc_rx_ipv4_nopay_octets,
			       MMC_RXIPV4_NOPAY_OCTETS);
	mmc->mmc_rx_ipv4_frag_octets =
		update_mmc_val(osi_core, mmc->mmc_rx_ipv4_frag_octets,
			       MMC_RXIPV4_FRAG_OCTETS);
	mmc->mmc_rx_ipv4_udsbl_octets =
		update_mmc_val(osi_core, mmc->mmc_rx_ipv4_udsbl_octets,
			       MMC_RXIPV4_UDSBL_OCTETS);
	mmc->mmc_rx_udp_gd_octets =
		update_mmc_val(osi_core, mmc->mmc_rx_udp_gd_octets,
			       MMC_RXIPV6_GD_OCTETS);
	mmc->mmc_rx_ipv6_hderr =
		update_mmc_val(osi_core, mmc->mmc_rx_ipv6_hderr,
			       MMC_RXIPV6_HDRERR_OCTETS);
	mmc->mmc_rx_ipv6_nopay =
		update_mmc_val(osi_core, mmc->mmc_rx_ipv6_nopay,
			       MMC_RXIPV6_NOPAY_OCTETS);
	mmc->mmc_rx_udp_gd_octets =
		update_mmc_val(osi_core, mmc->mmc_rx_udp_gd_octets,
			       MMC_RXUDP_GD_OCTETS);
	mmc->mmc_rx_udp_err_octets =
		update_mmc_val(osi_core, mmc->mmc_rx_udp_err_octets,
			       MMC_RXUDP_ERR_OCTETS);
	mmc->mmc_rx_tcp_gd_octets =
		update_mmc_val(osi_core, mmc->mmc_rx_tcp_gd_octets,
			       MMC_RXTCP_GD_OCTETS);
	mmc->mmc_rx_tcp_err_octets =
		update_mmc_val(osi_core, mmc->mmc_rx_tcp_err_octets,
			       MMC_RXTCP_ERR_OCTETS);
	mmc->mmc_rx_icmp_gd_octets =
		update_mmc_val(osi_core, mmc->mmc_rx_icmp_gd_octets,
			       MMC_RXICMP_GD_OCTETS);
	mmc->mmc_rx_icmp_err_octets =
		update_mmc_val(osi_core, mmc->mmc_rx_icmp_err_octets,
			       MMC_RXICMP_ERR_OCTETS);
}