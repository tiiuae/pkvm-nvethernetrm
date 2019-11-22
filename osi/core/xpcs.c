/*
 * Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
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

#include "xpcs.h"

/**
 * @brief xpcs_poll_for_an_complete - Polling for AN complete.
 *
 * Algorithm: This routine poll for AN completion status from
 *		XPCS IP.
 *
 * @param[in] osi_core: OSI core data structure.
 * @param[out] an_status: AN status from XPCS
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static inline int xpcs_poll_for_an_complete(struct osi_core_priv_data *osi_core,
					    unsigned int *an_status)
{
	void *xpcs_base = osi_core->xpcs_base;
	unsigned int status = 0;
	unsigned int retry = 1000;
	unsigned int count;
	int cond = 1;

	/* 14. Poll for AN complete */
	cond = 1;
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_HW_FAIL,
				     "XPCS AN completion timed out\n", 0ULL);
			return -1;
		}

		count++;

		status = xpcs_read(xpcs_base, XPCS_VR_MII_AN_INTR_STS);
		if ((status & XPCS_VR_MII_AN_INTR_STS_CL37_ANCMPLT_INTR) == 0U) {
			/* autoneg not completed - poll */
			osi_core->osd_ops.udelay(1000U);
		} else {
			/* 15. clear interrupt */
			status &= ~XPCS_VR_MII_AN_INTR_STS_CL37_ANCMPLT_INTR;
			xpcs_write(xpcs_base, XPCS_VR_MII_AN_INTR_STS, status);
			cond = 0;
		}
	}

	if ((status & XPCS_USXG_AN_STS_SPEED_MASK) == 0U) {
		OSI_CORE_ERR(OSI_NULL, OSI_LOG_ARG_HW_FAIL,
			     "XPCS AN completed with zero speed\n", 0ULL);
		return -1;
	}

	*an_status = status;
	return 0;
}

/**
 * @brief xpcs_set_speed - Set speed at XPCS
 *
 * Algorithm: This routine program XPCS speed based on AN status.
 *
 * @param[in] xpcs_base: XPCS base virtual address.
 * @param[in] status: Autonegotation Status.
 */
static inline void xpcs_set_speed(void *xpcs_base,
				  unsigned int status)
{
	unsigned int speed = status & XPCS_USXG_AN_STS_SPEED_MASK;
	unsigned int ctrl = 0;

	ctrl = xpcs_read(xpcs_base, XPCS_SR_MII_CTRL);

	switch (speed) {
	case XPCS_USXG_AN_STS_SPEED_2500:
		/* 2.5Gbps */
		ctrl |= XPCS_SR_MII_CTRL_SS5;
		ctrl &= ~(XPCS_SR_MII_CTRL_SS6 | XPCS_SR_MII_CTRL_SS13);
		break;
	case XPCS_USXG_AN_STS_SPEED_5000:
		/* 5Gbps */
		ctrl |= (XPCS_SR_MII_CTRL_SS5 | XPCS_SR_MII_CTRL_SS13);
		ctrl &= ~XPCS_SR_MII_CTRL_SS6;
		break;
	case XPCS_USXG_AN_STS_SPEED_10000:
	default:
		/* 10Gbps */
		ctrl |= (XPCS_SR_MII_CTRL_SS6 | XPCS_SR_MII_CTRL_SS13);
		ctrl &= ~XPCS_SR_MII_CTRL_SS5;
		break;
	}

	xpcs_write(xpcs_base, XPCS_SR_MII_CTRL, ctrl);
}

/**
 * @brief xpcs_start - Start XPCS
 *
 * Algorithm: This routine enables AN and set speed based on AN status
 *
 * @param[in] osi_core: OSI core data structure.
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
int xpcs_start(struct osi_core_priv_data *osi_core)
{
	void *xpcs_base = osi_core->xpcs_base;
	unsigned int an_status = 0;
	unsigned int retry = 1000;
	unsigned int count = 0;
	unsigned int ctrl = 0;
	int ret = 0;
	int cond = 1;

	ctrl = xpcs_read(xpcs_base, XPCS_SR_MII_CTRL);
	ctrl |= XPCS_SR_MII_CTRL_AN_ENABLE;
	xpcs_write(xpcs_base, XPCS_SR_MII_CTRL, ctrl);

	ret = xpcs_poll_for_an_complete(osi_core, &an_status);
	if (ret < 0) {
		return ret;
	}

	xpcs_set_speed(xpcs_base, an_status);

	/* USXGMII Rate Adaptor Reset before data transfer */
	ctrl = xpcs_read(xpcs_base, XPCS_VR_XS_PCS_DIG_CTRL1);
	ctrl |= XPCS_VR_XS_PCS_DIG_CTRL1_USRA_RST;
	xpcs_write(xpcs_base, XPCS_VR_XS_PCS_DIG_CTRL1, ctrl);
	while (cond == 1) {
		if (count > retry) {
			return -1;
		}

		count++;

		ctrl = xpcs_read(xpcs_base, XPCS_VR_XS_PCS_DIG_CTRL1);
		if ((ctrl & XPCS_VR_XS_PCS_DIG_CTRL1_USRA_RST) == 0U) {
			cond = 0;
		} else {
			osi_core->osd_ops.udelay(1000U);
		}
	}

	/* poll for Rx link up */
	cond = 1;
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			return -1;
		}

		count++;

		ctrl = xpcs_read(xpcs_base, XPCS_SR_XS_PCS_STS1);
		if ((ctrl & XPCS_SR_XS_PCS_STS1_RLU) ==
		    XPCS_SR_XS_PCS_STS1_RLU) {
			cond = 0;
		} else {
			osi_core->osd_ops.udelay(1000U);
		}
	}

	return 0;
}

/**
 * @brief xpcs_init - XPCS initialization
 *
 * Algorithm: This routine initialize XPCS in USXMII mode.
 *
 * @param[in] osi_core: OSI core data structure.
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
int xpcs_init(struct osi_core_priv_data *osi_core)
{
	void *xpcs_base = osi_core->xpcs_base;
	unsigned int retry = 1000;
	unsigned int count;
	unsigned int ctrl = 0;
	int cond = 1;

	/* Switching to USXGMII Mode based on
	 * XPCS programming guideline 7.6
	 */

	/* 1. switch DWC_xpcs to BASE-R mode */
	ctrl = xpcs_read(xpcs_base, XPCS_SR_XS_PCS_CTRL2);
	ctrl |= XPCS_SR_XS_PCS_CTRL2_PCS_TYPE_SEL_BASE_R;
	xpcs_write(xpcs_base, XPCS_SR_XS_PCS_CTRL2, ctrl);

	/* 2. enable USXGMII Mode inside DWC_xpcs */
	/* 3.  USXG_MODE = 10G - default it will be 10G mode */
	/* 4. Program PHY to operate at 10Gbps/5Gbps/2Gbps
         * this step not required since PHY speed programming
         * already done as part of phy INIT
	 */
	/* 5. Vendor specific software reset */
	ctrl = xpcs_read(xpcs_base, XPCS_VR_XS_PCS_DIG_CTRL1);
	ctrl |= XPCS_VR_XS_PCS_DIG_CTRL1_USXG_EN;
        ctrl |= XPCS_VR_XS_PCS_DIG_CTRL1_VR_RST;
	xpcs_write(xpcs_base, XPCS_VR_XS_PCS_DIG_CTRL1, ctrl);

	/* 6. Programming for Synopsys PHY - NA */

	/* 7. poll until vendor specific software reset */
	cond = 1;
	count = 0;
	while (cond == 1) {
		if (count > retry) {
			return -1;
		}

		count++;

		ctrl = xpcs_read(xpcs_base, XPCS_VR_XS_PCS_DIG_CTRL1);
		if ((ctrl & XPCS_VR_XS_PCS_DIG_CTRL1_VR_RST) == 0U) {
			cond = 0;
		} else {
			osi_core->osd_ops.udelay(1000U);
		}
	}

	/* 8. Backplane Ethernet PCS configurations
	 * clear AN_EN in SR_AN_CTRL
	 * set CL37_BP in VR_XS_PCS_DIG_CTRL1
	 */
	ctrl = xpcs_read(xpcs_base, XPCS_SR_AN_CTRL);
	ctrl &= ~XPCS_SR_AN_CTRL_AN_EN;
	xpcs_write(xpcs_base, XPCS_SR_AN_CTRL, ctrl);

	ctrl = xpcs_read(xpcs_base, XPCS_VR_XS_PCS_DIG_CTRL1);
	ctrl |= XPCS_VR_XS_PCS_DIG_CTRL1_CL37_BP;
	xpcs_write(xpcs_base, XPCS_VR_XS_PCS_DIG_CTRL1, ctrl);

	/* TODO: 9. MII_AN_INTR_EN to 1, to enable auto-negotiation
	 * complete interrupt */

	/* 10. (Optional step) Duration of link timer change */

	/* 11. XPCS configured as MAC-side USGMII - NA */

	/* 13.  TODO: If there is interrupt enabled for AN interrupt */

	return 0;
}

/**
 * @brief xpcs_eee - XPCS enable/disable EEE
 *
 * Algorithm: This routine update register related to EEE
 * for XPCS.
 *
 * @param[in] xpcs_base: XPCS virtual base address
 * @param[in] en_dis: enable - 1 or disable - 0
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
int xpcs_eee(void *xpcs_base, unsigned int en_dis)
{
	unsigned int val = 0x0U;

	if (en_dis != OSI_ENABLE && en_dis != OSI_DISABLE) {
		return  -1;
	}

	if (en_dis == OSI_DISABLE) {
		val = xpcs_read(xpcs_base, XPCS_VR_XS_PCS_EEE_MCTRL0);
		val &= ~XPCS_VR_XS_PCS_EEE_MCTRL0_LTX_EN;
		val &= ~XPCS_VR_XS_PCS_EEE_MCTRL0_LRX_EN;
		xpcs_write(xpcs_base, XPCS_VR_XS_PCS_EEE_MCTRL0, val);
		return 0;
	}

	/* 1. Check if DWC_xpcs supports the EEE feature by
	 * reading the SR_XS_PCS_EEE_ABL register
	 * 1000BASEX-Only is different config then else so can (skip) */

	/* 2. Program various timers used in the EEE mode depending on the
	 * clk_eee_i clock frequency. default times are same as IEEE std
	 * clk_eee_i() is 102MHz. MULT_FACT_100NS = 9 because 9.8ns*10 = 98
	 * which is between 80 and 120  this leads to default setting match */

	/* TODO for uFPGA */

	val = xpcs_read(xpcs_base, XPCS_VR_XS_PCS_EEE_MCTRL0);
	/* 3. If FEC is enabled in the KR mode (skip in FPGA)*/
	/* 4. enable the EEE feature on the Tx path and Rx path */
	//val &= ~XPCS_VR_XS_PCS_EEE_MCTRL0_EEE_SLR_BYP;
	val &= ~XPCS_VR_XS_PCS_EEE_MCTRL0_LTX_EN;
	val &= ~XPCS_VR_XS_PCS_EEE_MCTRL0_LRX_EN;

	val |= (XPCS_VR_XS_PCS_EEE_MCTRL0_LTX_EN |
	//	XPCS_VR_XS_PCS_EEE_MCTRL0_LTX_EN |
		XPCS_VR_XS_PCS_EEE_MCTRL0_LRX_EN);
	xpcs_write(xpcs_base, XPCS_VR_XS_PCS_EEE_MCTRL0, val);

	return 0;
}
