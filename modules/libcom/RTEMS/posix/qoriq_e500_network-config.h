/*
 * Copyright (c) 2013 embedded brains GmbH.  All rights reserved.
 *
 *  embedded brains GmbH
 *  Dornierstr. 4
 *  82178 Puchheim
 *  Germany
 *  <rtems@embedded-brains.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _RTEMS_BSD_TEST_NETWORK_CONFIG_H_
#define _RTEMS_BSD_TEST_NETWORK_CONFIG_H_

#include <bsp.h>

#if defined(LIBBSP_ARM_ALTERA_CYCLONE_V_BSP_H)
  #define NET_CFG_INTERFACE_0 "dwc0"
#elif defined(LIBBSP_ARM_REALVIEW_PBX_A9_BSP_H)
  #define NET_CFG_INTERFACE_0 "smc0"
#elif defined(LIBBSP_ARM_XILINX_ZYNQ_BSP_H)
  #define NET_CFG_INTERFACE_0 "cgem0"
#elif defined(LIBBSP_M68K_GENMCF548X_BSP_H)
  #define NET_CFG_INTERFACE_0 "fec0"
#elif defined(LIBBSP_ARM_LPC32XX_BSP_H)
  #define NET_CFG_INTERFACE_0 "lpe0"
#elif defined(LIBBSP_POWERPC_QORIQ_BSP_H)
  #if QORIQ_CHIP_IS_T_VARIANT(QORIQ_CHIP_VARIANT)
    #define NET_CFG_INTERFACE_0 "fm1m3"
  #else
    #define NET_CFG_INTERFACE_0 "tsec0"
  #endif
#elif defined(LIBBSP_ARM_ATSAM_BSP_H)
  #define NET_CFG_INTERFACE_0 "if_atsam0"
#else
  #define NET_CFG_INTERFACE_0 "lo0"
#endif

#define NET_CFG_SELF_IP "141.14.128.89"

#define NET_CFG_NETMASK "255.255.0.0"

#define NET_CFG_PEER_IP "192.168.100.11"

#define NET_CFG_GATEWAY_IP "141.14.128.128"

#endif /* _RTEMS_BSD_TEST_NETWORK_CONFIG_H_ */
