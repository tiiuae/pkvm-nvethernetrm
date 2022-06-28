/*
 * Copyright (c) 2018-2022, NVIDIA CORPORATION. All rights reserved.
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

#include "../osi/common/common.h"
#include "dma_local.h"
#include "eqos_dma.h"
#include "../osi/common/type.h"

/**
 * @brief eqos_dma_safety_config - EQOS MAC DMA safety configuration
 */
static struct dma_func_safety eqos_dma_safety_config;

/**
 * @brief Write to safety critical register.
 *
 * @note
 * Algorithm:
 *  - Acquire RW lock, so that eqos_validate_dma_regs does not run while
 *    updating the safety critical register.
 *  - call osi_writel() to actually update the memory mapped register.
 *  - Store the same value in eqos_dma_safety_config->reg_val[idx], so that
 *    this latest value will be compared when eqos_validate_dma_regs is
 *    scheduled.
 *
 * @param[in] osi_dma: OSI DMA private data structure.
 * @param[in] val: Value to be written.
 * @param[in] addr: memory mapped register address to be written to.
 * @param[in] idx: Index of register corresponding to enum func_safety_dma_regs.
 *
 * @pre MAC has to be out of reset, and clocks supplied.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: Yes
 */
static inline void eqos_dma_safety_writel(struct osi_dma_priv_data *osi_dma,
					  nveu32_t val, void *addr,
					  nveu32_t idx)
{
	struct dma_func_safety *config = &eqos_dma_safety_config;

	osi_lock_irq_enabled(&config->dma_safety_lock);
	osi_writela(osi_dma->osd, val, addr);
	config->reg_val[idx] = (val & config->reg_mask[idx]);
	osi_unlock_irq_enabled(&config->dma_safety_lock);
}

/**
 * @brief Initialize the eqos_dma_safety_config.
 *
 * @param[in] osi_dma: OSI DMA private data structure.
 *
 * @note
 * Algorithm:
 *  - Populate the list of safety critical registers and provide
 *  - the address of the register
 *  - Register mask (to ignore reserved/self-critical bits in the reg).
 *    See eqos_validate_dma_regs which can be invoked periodically to compare
 *    the last written value to this register vs the actual value read when
 *    eqos_validate_dma_regs is scheduled.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 */
static void eqos_dma_safety_init(struct osi_dma_priv_data *osi_dma)
{
	struct dma_func_safety *config = &eqos_dma_safety_config;
	nveu8_t *base = (nveu8_t *)osi_dma->base;
	nveu32_t val;
	nveu32_t i, idx;

	/* Initialize all reg address to NULL, since we may not use
	 * some regs depending on the number of DMA chans enabled.
	 */
	for (i = EQOS_DMA_CH0_CTRL_IDX; i < EQOS_MAX_DMA_SAFETY_REGS; i++) {
		config->reg_addr[i] = OSI_NULL;
	}

	for (i = 0U; i < osi_dma->num_dma_chans; i++) {
		idx = osi_dma->dma_chans[i];
#if 0
		CHECK_CHAN_BOUND(idx);
#endif
		config->reg_addr[EQOS_DMA_CH0_CTRL_IDX + idx] = base +
						EQOS_DMA_CHX_CTRL(idx);
		config->reg_addr[EQOS_DMA_CH0_TX_CTRL_IDX + idx] = base +
						EQOS_DMA_CHX_TX_CTRL(idx);
		config->reg_addr[EQOS_DMA_CH0_RX_CTRL_IDX + idx] = base +
						EQOS_DMA_CHX_RX_CTRL(idx);
		config->reg_addr[EQOS_DMA_CH0_TDRL_IDX + idx] = base +
						EQOS_DMA_CHX_TDRL(idx);
		config->reg_addr[EQOS_DMA_CH0_RDRL_IDX + idx] = base +
						EQOS_DMA_CHX_RDRL(idx);
		config->reg_addr[EQOS_DMA_CH0_INTR_ENA_IDX + idx] = base +
						EQOS_DMA_CHX_INTR_ENA(idx);

		config->reg_mask[EQOS_DMA_CH0_CTRL_IDX + idx] =
						EQOS_DMA_CHX_CTRL_MASK;
		config->reg_mask[EQOS_DMA_CH0_TX_CTRL_IDX + idx] =
						EQOS_DMA_CHX_TX_CTRL_MASK;
		config->reg_mask[EQOS_DMA_CH0_RX_CTRL_IDX + idx] =
						EQOS_DMA_CHX_RX_CTRL_MASK;
		config->reg_mask[EQOS_DMA_CH0_TDRL_IDX + idx] =
						EQOS_DMA_CHX_TDRL_MASK;
		config->reg_mask[EQOS_DMA_CH0_RDRL_IDX + idx] =
						EQOS_DMA_CHX_RDRL_MASK;
		config->reg_mask[EQOS_DMA_CH0_INTR_ENA_IDX + idx] =
						EQOS_DMA_CHX_INTR_ENA_MASK;
	}

	/* Initialize current power-on-reset values of these registers. */
	for (i = EQOS_DMA_CH0_CTRL_IDX; i < EQOS_MAX_DMA_SAFETY_REGS; i++) {
		if (config->reg_addr[i] == OSI_NULL) {
			continue;
		}
		val = osi_readl((nveu8_t *)config->reg_addr[i]);
		config->reg_val[i] = val & config->reg_mask[i];
	}

	osi_lock_init(&config->dma_safety_lock);
}

/**
 * @brief eqos_configure_dma_channel - Configure DMA channel
 *
 * @note
 * Algorithm:
 *  - This takes care of configuring the  below
 *    parameters for the DMA channel
 *   - Enabling DMA channel interrupts
 *   - Enable 8xPBL mode
 *   - Program Tx, Rx PBL
 *   - Enable TSO if HW supports
 *   - Program Rx Watchdog timer
 *
 * @param[in] chan: DMA channel number that need to be configured.
 * @param[in] osi_dma: OSI DMA private data structure.
 *
 * @pre MAC has to be out of reset.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 */
static void eqos_configure_dma_channel(nveu32_t chan,
				       struct osi_dma_priv_data *osi_dma)
{
	nveu32_t value;
#if 0
	CHECK_CHAN_BOUND(chan);
#endif
	/* enable DMA channel interrupts */
	/* TIE - Transmit Interrupt Enable */
	/* RIE - Receive Interrupt Enable */
	value = osi_readl((nveu8_t *)osi_dma->base +
			  EQOS_DMA_CHX_INTR_ENA(chan));
	value |= (EQOS_DMA_CHX_INTR_TIE | EQOS_DMA_CHX_INTR_RIE);
	eqos_dma_safety_writel(osi_dma, value, (nveu8_t *)osi_dma->base +
			       EQOS_DMA_CHX_INTR_ENA(chan),
			       EQOS_DMA_CH0_INTR_ENA_IDX + chan);

	/* Enable 8xPBL mode */
	value = osi_readl((nveu8_t *)osi_dma->base +
			  EQOS_DMA_CHX_CTRL(chan));
	value |= EQOS_DMA_CHX_CTRL_PBLX8;
	eqos_dma_safety_writel(osi_dma, value, (nveu8_t *)osi_dma->base +
			       EQOS_DMA_CHX_CTRL(chan),
			       EQOS_DMA_CH0_CTRL_IDX + chan);

	/* Configure DMA channel Transmit control register */
	value = osi_readl((nveu8_t *)osi_dma->base +
			  EQOS_DMA_CHX_TX_CTRL(chan));
	/* Enable OSF mode */
	value |= EQOS_DMA_CHX_TX_CTRL_OSF;
	/* TxPBL = 32*/
	value |= EQOS_DMA_CHX_TX_CTRL_TXPBL_RECOMMENDED;
	/* enable TSO by default if HW supports */
	value |= EQOS_DMA_CHX_TX_CTRL_TSE;

	eqos_dma_safety_writel(osi_dma, value, (nveu8_t *)osi_dma->base +
			       EQOS_DMA_CHX_TX_CTRL(chan),
			       EQOS_DMA_CH0_TX_CTRL_IDX + chan);

	/* Configure DMA channel Receive control register */
	/* Select Rx Buffer size.  Needs to be rounded up to next multiple of
	 * bus width
	 */
	value = osi_readl((nveu8_t *)osi_dma->base +
			  EQOS_DMA_CHX_RX_CTRL(chan));

	/* clear previous Rx buffer size */
	value &= ~EQOS_DMA_CHX_RBSZ_MASK;

	value |= (osi_dma->rx_buf_len << EQOS_DMA_CHX_RBSZ_SHIFT);
	/* RXPBL = 12 */
	value |= EQOS_DMA_CHX_RX_CTRL_RXPBL_RECOMMENDED;
	eqos_dma_safety_writel(osi_dma, value, (nveu8_t *)osi_dma->base +
			       EQOS_DMA_CHX_RX_CTRL(chan),
			       EQOS_DMA_CH0_RX_CTRL_IDX + chan);

	/* Set Receive Interrupt Watchdog Timer Count */
	/* conversion of usec to RWIT value
	 * Eg: System clock is 125MHz, each clock cycle would then be 8ns
	 * For value 0x1 in RWT, device would wait for 512 clk cycles with
	 * RWTU as 0x1,
	 * ie, (8ns x 512) => 4.096us (rounding off to 4us)
	 * So formula with above values is,ret = usec/4
	 */
	if ((osi_dma->use_riwt == OSI_ENABLE) &&
	    (osi_dma->rx_riwt < UINT_MAX)) {
		value = osi_readl((nveu8_t *)osi_dma->base +
				  EQOS_DMA_CHX_RX_WDT(chan));
		/* Mask the RWT and RWTU value */
		value &= ~(EQOS_DMA_CHX_RX_WDT_RWT_MASK |
			   EQOS_DMA_CHX_RX_WDT_RWTU_MASK);
		/* Conversion of usec to Rx Interrupt Watchdog Timer Count */
		value |= ((osi_dma->rx_riwt *
			 (EQOS_AXI_CLK_FREQ / OSI_ONE_MEGA_HZ)) /
			 EQOS_DMA_CHX_RX_WDT_RWTU) &
			 EQOS_DMA_CHX_RX_WDT_RWT_MASK;
		value |= EQOS_DMA_CHX_RX_WDT_RWTU_512_CYCLE;
		osi_writel(value, (nveu8_t *)osi_dma->base +
			   EQOS_DMA_CHX_RX_WDT(chan));
	}
}

/**
 * @brief eqos_init_dma_channel - DMA channel INIT
 *
 * @param[in] osi_dma: OSI DMA private data structure.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_init_dma_channel(struct osi_dma_priv_data *osi_dma)
{
	nveu32_t chinx;

	eqos_dma_safety_init(osi_dma);

	/* configure EQOS DMA channels */
	for (chinx = 0; chinx < osi_dma->num_dma_chans; chinx++) {
		eqos_configure_dma_channel(osi_dma->dma_chans[chinx], osi_dma);
	}

	return 0;
}

#ifndef OSI_STRIPPED_LIB
/**
 * @brief Read-validate HW registers for functional safety.
 *
 * @note
 * Algorithm:
 *  - Reads pre-configured list of MAC/MTL configuration registers
 *    and compares with last written value for any modifications.
 *
 * @param[in] osi_dma: OSI DMA private data structure.
 *
 * @pre
 *  - MAC has to be out of reset.
 *  - osi_hw_dma_init has to be called. Internally this would initialize
 *    the safety_config (see osi_dma_priv_data) based on MAC version and
 *    which specific registers needs to be validated periodically.
 *  - Invoke this call if (osi_dma_priv_data->safety_config != OSI_NULL)
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
static nve32_t eqos_validate_dma_regs(struct osi_dma_priv_data *osi_dma)
{
	struct dma_func_safety *config =
		(struct dma_func_safety *)osi_dma->safety_config;
	nveu32_t cur_val;
	nveu32_t i;

	osi_lock_irq_enabled(&config->dma_safety_lock);
	for (i = EQOS_DMA_CH0_CTRL_IDX; i < EQOS_MAX_DMA_SAFETY_REGS; i++) {
		if (config->reg_addr[i] == OSI_NULL) {
			continue;
		}

		cur_val = osi_readl((nveu8_t *)config->reg_addr[i]);
		cur_val &= config->reg_mask[i];

		if (cur_val == config->reg_val[i]) {
			continue;
		} else {
			/* Register content differs from what was written.
			 * Return error and let safety manager (NVGaurd etc.)
			 * take care of corrective action.
			 */
			osi_unlock_irq_enabled(&config->dma_safety_lock);
			return -1;
		}
	}
	osi_unlock_irq_enabled(&config->dma_safety_lock);

	return 0;
}

/**
 * @brief eqos_config_slot - Configure slot Checking for DMA channel
 *
 * @note
 * Algorithm:
 *  - Set/Reset the slot function of DMA channel based on given inputs
 *
 * @param[in] osi_dma: OSI DMA private data structure.
 * @param[in] chan: DMA channel number to enable slot function
 * @param[in] set: flag for set/reset with value OSI_ENABLE/OSI_DISABLE
 * @param[in] interval: slot interval from 0usec to 4095usec
 *
 * @pre
 *  - MAC should be init and started. see osi_start_mac()
 *  - OSD should be initialized
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: No
 */
static void eqos_config_slot(struct osi_dma_priv_data *osi_dma,
			     nveu32_t chan,
			     nveu32_t set,
			     nveu32_t interval)
{
	nveu32_t value;
	nveu32_t intr;

#if 0
	CHECK_CHAN_BOUND(chan);
#endif
	if (set == OSI_ENABLE) {
		/* Program SLOT CTRL register SIV and set ESC bit */
		value = osi_readl((nveu8_t *)osi_dma->base +
				  EQOS_DMA_CHX_SLOT_CTRL(chan));
		value &= ~EQOS_DMA_CHX_SLOT_SIV_MASK;
		/* remove overflow bits of interval */
		intr = interval & EQOS_DMA_CHX_SLOT_SIV_MASK;
		value |= (intr << EQOS_DMA_CHX_SLOT_SIV_SHIFT);
		/* Set ESC bit */
		value |= EQOS_DMA_CHX_SLOT_ESC;
		osi_writel(value, (nveu8_t *)osi_dma->base +
			   EQOS_DMA_CHX_SLOT_CTRL(chan));

	} else {
		/* Clear ESC bit of SLOT CTRL register */
		value = osi_readl((nveu8_t *)osi_dma->base +
				  EQOS_DMA_CHX_SLOT_CTRL(chan));
		value &= ~EQOS_DMA_CHX_SLOT_ESC;
		osi_writel(value, (nveu8_t *)osi_dma->base +
			   EQOS_DMA_CHX_SLOT_CTRL(chan));
	}
}
#endif /* !OSI_STRIPPED_LIB */

/**
 * @brief eqos_get_dma_safety_config - EQOS get DMA safety configuration
 */
void *eqos_get_dma_safety_config(void)
{
	return &eqos_dma_safety_config;
}

#ifdef OSI_DEBUG
/**
 * @brief Enable/disable debug interrupt
 *
 * @param[in] osi_dma: OSI DMA private data structure.
 *
 * Algorithm:
 * - if osi_dma->ioctl_data.arg_u32 == OSI_ENABLE enable debug interrupt
 * - else disable bebug inerrupts
 */
static void eqos_debug_intr_config(struct osi_dma_priv_data *osi_dma)
{
	nveu32_t chinx;
	nveu32_t chan;
	nveu32_t val;
	nveu32_t enable = osi_dma->ioctl_data.arg_u32;

	if (enable == OSI_ENABLE) {
		for (chinx = 0; chinx < osi_dma->num_dma_chans; chinx++) {
			chan = osi_dma->dma_chans[chinx];
			val = osi_readl((nveu8_t *)osi_dma->base +
					EQOS_DMA_CHX_INTR_ENA(chan));

			val |= (EQOS_DMA_CHX_INTR_AIE |
				EQOS_DMA_CHX_INTR_FBEE |
				EQOS_DMA_CHX_INTR_RBUE |
				EQOS_DMA_CHX_INTR_TBUE |
				EQOS_DMA_CHX_INTR_NIE);
			osi_writel(val, (nveu8_t *)osi_dma->base +
				   EQOS_DMA_CHX_INTR_ENA(chan));
		}

	} else {
		for (chinx = 0; chinx < osi_dma->num_dma_chans; chinx++) {
			chan = osi_dma->dma_chans[chinx];
			val = osi_readl((nveu8_t *)osi_dma->base +
					EQOS_DMA_CHX_INTR_ENA(chan));
			val &= (~EQOS_DMA_CHX_INTR_AIE &
				~EQOS_DMA_CHX_INTR_FBEE &
				~EQOS_DMA_CHX_INTR_RBUE &
				~EQOS_DMA_CHX_INTR_TBUE &
				~EQOS_DMA_CHX_INTR_NIE);
			osi_writel(val, (nveu8_t *)osi_dma->base +
				   EQOS_DMA_CHX_INTR_ENA(chan));
		}
	}
}
#endif

/*
 * @brief eqos_init_dma_chan_ops - Initialize EQOS DMA operations.
 *
 * @param[in] ops: DMA channel operations pointer.
 */
void eqos_init_dma_chan_ops(struct dma_chan_ops *ops)
{
	ops->init_dma_channel = eqos_init_dma_channel;
#ifndef OSI_STRIPPED_LIB
	ops->validate_regs = eqos_validate_dma_regs;
	ops->config_slot = eqos_config_slot;
#endif /* !OSI_STRIPPED_LIB */
#ifdef OSI_DEBUG
	ops->debug_intr_config = eqos_debug_intr_config;
#endif
}
