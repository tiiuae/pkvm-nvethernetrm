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
#ifdef OSD_DUMMY
#include <osd.h>
#include "../osi/common/type.h"

void osd_usleep_range(nveu64_t umin, nveu64_t umax)
{
}

void osd_msleep(nveu32_t msec)
{
}

void osd_udelay(nveu64_t usec)
{
}

void osd_receive_packet(void *priv, void *rxring, nveu32_t chan,
			nveu32_t dma_buf_len, void *rxpkt_cx,
			void *rx_pkt_swcx)
{
}

void osd_transmit_complete(void *priv, void *buffer, nveu64_t dmaaddr,
			   nveu32_t len, void *txdone_pkt_cx)
{
}

void osd_log(void *priv,
	     const nve8_t *func,
	     nveu32_t line,
	     nveu32_t level,
	     nveu32_t type,
	     const nve8_t *err,
	     nveul64_t loga)
{
}
#endif