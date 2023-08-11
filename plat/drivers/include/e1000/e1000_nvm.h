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

#ifndef _E1000_NVM_H_
#define _E1000_NVM_H_

struct e1000_pba {
	__u16 word[2];
	__u16 *pba_block;
};

struct e1000_fw_version {
	__u32 etrack_id;
	__u16 eep_major;
	__u16 eep_minor;
	__u16 eep_build;

	__u8 invm_major;
	__u8 invm_minor;
	__u8 invm_img_type;

	bool or_valid;
	__u16 or_major;
	__u16 or_build;
	__u16 or_patch;
};


void e1000_init_nvm_ops_generic(struct e1000_hw *hw);
__s32  e1000_null_read_nvm(struct e1000_hw *hw, __u16 a, __u16 b, __u16 *c);
void e1000_null_nvm_generic(struct e1000_hw *hw);
__s32  e1000_null_led_default(struct e1000_hw *hw, __u16 *data);
__s32  e1000_null_write_nvm(struct e1000_hw *hw, __u16 a, __u16 b, __u16 *c);
__s32  e1000_acquire_nvm_generic(struct e1000_hw *hw);

__s32  e1000_poll_eerd_eewr_done(struct e1000_hw *hw, int ee_reg);
__s32  e1000_read_mac_addr_generic(struct e1000_hw *hw);
__s32  e1000_read_pba_num_generic(struct e1000_hw *hw, __u32 *pba_num);
__s32  e1000_read_pba_string_generic(struct e1000_hw *hw, __u8 *pba_num,
				   __u32 pba_num_size);
__s32  e1000_read_pba_length_generic(struct e1000_hw *hw, __u32 *pba_num_size);
__s32 e1000_read_pba_raw(struct e1000_hw *hw, __u16 *eeprom_buf,
		       __u32 eeprom_buf_size, __u16 max_pba_block_size,
		       struct e1000_pba *pba);
__s32 e1000_write_pba_raw(struct e1000_hw *hw, __u16 *eeprom_buf,
			__u32 eeprom_buf_size, struct e1000_pba *pba);
__s32 e1000_get_pba_block_size(struct e1000_hw *hw, __u16 *eeprom_buf,
			     __u32 eeprom_buf_size, __u16 *pba_block_size);
__s32  e1000_read_nvm_spi(struct e1000_hw *hw, __u16 offset, __u16 words, __u16 *data);
__s32  e1000_read_nvm_microwire(struct e1000_hw *hw, __u16 offset,
			      __u16 words, __u16 *data);
__s32  e1000_read_nvm_eerd(struct e1000_hw *hw, __u16 offset, __u16 words,
			 __u16 *data);
__s32  e1000_valid_led_default_generic(struct e1000_hw *hw, __u16 *data);
__s32  e1000_validate_nvm_checksum_generic(struct e1000_hw *hw);
__s32  e1000_write_nvm_microwire(struct e1000_hw *hw, __u16 offset,
			       __u16 words, __u16 *data);
__s32  e1000_write_nvm_spi(struct e1000_hw *hw, __u16 offset, __u16 words,
			 __u16 *data);
__s32  e1000_update_nvm_checksum_generic(struct e1000_hw *hw);
void e1000_stop_nvm(struct e1000_hw *hw);
void e1000_release_nvm_generic(struct e1000_hw *hw);
void e1000_get_fw_version(struct e1000_hw *hw,
			  struct e1000_fw_version *fw_vers);

#define E1000_STM_OPCODE	0xDB00

#endif
