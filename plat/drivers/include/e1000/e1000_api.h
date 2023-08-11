/*******************************************************************************

Copyright (c) 2001-2015, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of the Intel Corporation nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#ifndef _E1000_API_H_
#define _E1000_API_H_

#include "e1000_hw.h"

#define DEBUG 0
#define debug_uk_pr_info(fmt, ...) \
            do { \
				if (DEBUG) \
					uk_pr_info(fmt, ##__VA_ARGS__); \
				} while (0)

extern void e1000_init_function_pointers_82542(struct e1000_hw *hw);
extern void e1000_init_function_pointers_82543(struct e1000_hw *hw);
extern void e1000_init_function_pointers_82540(struct e1000_hw *hw);
extern void e1000_init_function_pointers_82571(struct e1000_hw *hw);
extern void e1000_init_function_pointers_82541(struct e1000_hw *hw);
extern void e1000_init_function_pointers_vf(struct e1000_hw *hw);
extern void e1000_power_up_fiber_serdes_link(struct e1000_hw *hw);
extern void e1000_shutdown_fiber_serdes_link(struct e1000_hw *hw);
extern void e1000_init_function_pointers_i210(struct e1000_hw *hw);

__s32 e1000_set_obff_timer(struct e1000_hw *hw, __u32 itr);
__s32 e1000_set_mac_type(struct e1000_hw *hw);
__s32 e1000_setup_init_funcs(struct e1000_hw *hw, bool init_device);
__s32 e1000_init_mac_params(struct e1000_hw *hw);
__s32 e1000_init_nvm_params(struct e1000_hw *hw);
__s32 e1000_init_phy_params(struct e1000_hw *hw);
__s32 e1000_init_mbx_params(struct e1000_hw *hw);
__s32 e1000_get_bus_info(struct e1000_hw *hw);
__s32 e1000_force_mac_fc(struct e1000_hw *hw);
__s32 e1000_check_for_link(struct e1000_hw *hw);
__s32 e1000_reset_hw(struct e1000_hw *hw);
__s32 e1000_init_hw(struct e1000_hw *hw);
__s32 e1000_setup_link(struct e1000_hw *hw);
__s32 e1000_get_speed_and_duplex(struct e1000_hw *hw, __u16 *speed, __u16 *duplex);
__s32 e1000_disable_pcie_master(struct e1000_hw *hw);
void e1000_config_collision_dist(struct e1000_hw *hw);
int e1000_rar_set(struct e1000_hw *hw, __u8 *addr, __u32 index);
__u32 e1000_hash_mc_addr(struct e1000_hw *hw, __u8 *mc_addr);
void e1000_update_mc_addr_list(struct e1000_hw *hw, __u8 *mc_addr_list,
			       __u32 mc_addr_count);
__s32 e1000_setup_led(struct e1000_hw *hw);
__s32 e1000_cleanup_led(struct e1000_hw *hw);
__s32 e1000_check_reset_block(struct e1000_hw *hw);
__s32 e1000_blink_led(struct e1000_hw *hw);
__s32 e1000_led_on(struct e1000_hw *hw);
__s32 e1000_led_off(struct e1000_hw *hw);
__s32 e1000_id_led_init(struct e1000_hw *hw);
void e1000_reset_adaptive(struct e1000_hw *hw);
void e1000_update_adaptive(struct e1000_hw *hw);
__s32 e1000_get_cable_length(struct e1000_hw *hw);
__s32 e1000_validate_mdi_setting(struct e1000_hw *hw);
__s32 e1000_read_phy_reg(struct e1000_hw *hw, __u32 offset, __u16 *data);
__s32 e1000_write_phy_reg(struct e1000_hw *hw, __u32 offset, __u16 data);
__s32 e1000_write_8bit_ctrl_reg(struct e1000_hw *hw, __u32 reg, __u32 offset,
			      __u8 data);
__s32 e1000_get_phy_info(struct e1000_hw *hw);
void e1000_release_phy(struct e1000_hw *hw);
__s32 e1000_acquire_phy(struct e1000_hw *hw);
__s32 e1000_cfg_on_link_up(struct e1000_hw *hw);
__s32 e1000_phy_hw_reset(struct e1000_hw *hw);
__s32 e1000_phy_commit(struct e1000_hw *hw);
void e1000_power_up_phy(struct e1000_hw *hw);
void e1000_power_down_phy(struct e1000_hw *hw);
__s32 e1000_read_mac_addr(struct e1000_hw *hw);
__s32 e1000_read_pba_num(struct e1000_hw *hw, __u32 *part_num);
__s32 e1000_read_pba_string(struct e1000_hw *hw, __u8 *pba_num, __u32 pba_num_size);
__s32 e1000_read_pba_length(struct e1000_hw *hw, __u32 *pba_num_size);
void e1000_reload_nvm(struct e1000_hw *hw);
__s32 e1000_update_nvm_checksum(struct e1000_hw *hw);
__s32 e1000_validate_nvm_checksum(struct e1000_hw *hw);
__s32 e1000_read_nvm(struct e1000_hw *hw, __u16 offset, __u16 words, __u16 *data);
__s32 e1000_write_nvm(struct e1000_hw *hw, __u16 offset, __u16 words, __u16 *data);
__s32 e1000_set_d3_lplu_state(struct e1000_hw *hw, bool active);
__s32 e1000_set_d0_lplu_state(struct e1000_hw *hw, bool active);
bool e1000_check_mng_mode(struct e1000_hw *hw);
__u32  e1000_translate_register_82542(__u32 reg);



/*
 * TBI_ACCEPT macro definition:
 *
 * This macro requires:
 *      a = a pointer to struct e1000_hw
 *      status = the 8 bit status field of the Rx descriptor with EOP set
 *      errors = the 8 bit error field of the Rx descriptor with EOP set
 *      length = the sum of all the length fields of the Rx descriptors that
 *               make up the current frame
 *      last_byte = the last byte of the frame DMAed by the hardware
 *      min_frame_size = the minimum frame length we want to accept.
 *      max_frame_size = the maximum frame length we want to accept.
 *
 * This macro is a conditional that should be used in the interrupt
 * handler's Rx processing routine when RxErrors have been detected.
 *
 * Typical use:
 *  ...
 *  if (TBI_ACCEPT) {
 *      accept_frame = true;
 *      e1000_tbi_adjust_stats(adapter, MacAddress);
 *      frame_length--;
 *  } else {
 *      accept_frame = false;
 *  }
 *  ...
 */

/* The carrier extension symbol, as received by the NIC. */
#define CARRIER_EXTENSION   0x0F

#define TBI_ACCEPT(a, status, errors, length, last_byte, \
		   min_frame_size, max_frame_size) \
	(e1000_tbi_sbp_enabled_82543(a) && \
	 (((errors) & E1000_RXD_ERR_FRAME_ERR_MASK) == E1000_RXD_ERR_CE) && \
	 ((last_byte) == CARRIER_EXTENSION) && \
	 (((status) & E1000_RXD_STAT_VP) ? \
	  (((length) > ((min_frame_size) - VLAN_TAG_SIZE)) && \
	  ((length) <= ((max_frame_size) + 1))) : \
	  (((length) > (min_frame_size)) && \
	  ((length) <= ((max_frame_size) + VLAN_TAG_SIZE + 1)))))

#define E1000_MAX(a, b) ((a) > (b) ? (a) : (b))
#define E1000_DIVIDE_ROUND_UP(a, b)	(((a) + (b) - 1) / (b)) /* ceil(a/b) */
#endif /* _E1000_API_H_ */
