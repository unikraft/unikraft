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

#include <e1000/e1000_mbx.h>

/**
 *  e1000_null_mbx_check_for_flag - No-op function, return 0
 *  @hw: pointer to the HW structure
 **/
STATIC __s32 e1000_null_mbx_check_for_flag(struct e1000_hw __unused *hw,
					 __u16 __unused mbx_id)
{
	uk_pr_info("e1000_null_mbx_check_flag\n");
	UNREFERENCED_2PARAMETER(hw, mbx_id);

	return E1000_SUCCESS;
}

/**
 *  e1000_null_mbx_transact - No-op function, return 0
 *  @hw: pointer to the HW structure
 **/
STATIC __s32 e1000_null_mbx_transact(struct e1000_hw __unused *hw,
				   __u32 __unused *msg,
				   __u16 __unused size,
				   __u16 __unused mbx_id)
{
	uk_pr_info("e1000_null_mbx_rw_msg\n");
	UNREFERENCED_4PARAMETER(hw, msg, size, mbx_id);

	return E1000_SUCCESS;
}

/**
 *  e1000_read_mbx - Reads a message from the mailbox
 *  @hw: pointer to the HW structure
 *  @msg: The message buffer
 *  @size: Length of buffer
 *  @mbx_id: id of mailbox to read
 *
 *  returns SUCCESS if it successfully read message from buffer
 **/
__s32 e1000_read_mbx(struct e1000_hw *hw, __u32 *msg, __u16 size, __u16 mbx_id)
{
	struct e1000_mbx_info *mbx = &hw->mbx;
	__s32 ret_val = -E1000_ERR_MBX;

	uk_pr_info("e1000_read_mbx\n");

	/* limit read to size of mailbox */
	if (size > mbx->size)
		size = mbx->size;

	if (mbx->ops.read)
		ret_val = mbx->ops.read(hw, msg, size, mbx_id);

	return ret_val;
}

/**
 *  e1000_write_mbx - Write a message to the mailbox
 *  @hw: pointer to the HW structure
 *  @msg: The message buffer
 *  @size: Length of buffer
 *  @mbx_id: id of mailbox to write
 *
 *  returns SUCCESS if it successfully copied message into the buffer
 **/
__s32 e1000_write_mbx(struct e1000_hw *hw, __u32 *msg, __u16 size, __u16 mbx_id)
{
	struct e1000_mbx_info *mbx = &hw->mbx;
	__s32 ret_val = E1000_SUCCESS;

	uk_pr_info("e1000_write_mbx\n");

	if (size > mbx->size)
		ret_val = -E1000_ERR_MBX;

	else if (mbx->ops.write)
		ret_val = mbx->ops.write(hw, msg, size, mbx_id);

	return ret_val;
}

/**
 *  e1000_check_for_msg - checks to see if someone sent us mail
 *  @hw: pointer to the HW structure
 *  @mbx_id: id of mailbox to check
 *
 *  returns SUCCESS if the Status bit was found or else ERR_MBX
 **/
__s32 e1000_check_for_msg(struct e1000_hw *hw, __u16 mbx_id)
{
	struct e1000_mbx_info *mbx = &hw->mbx;
	__s32 ret_val = -E1000_ERR_MBX;

	uk_pr_info("e1000_check_for_msg\n");

	if (mbx->ops.check_for_msg)
		ret_val = mbx->ops.check_for_msg(hw, mbx_id);

	return ret_val;
}

/**
 *  e1000_check_for_ack - checks to see if someone sent us ACK
 *  @hw: pointer to the HW structure
 *  @mbx_id: id of mailbox to check
 *
 *  returns SUCCESS if the Status bit was found or else ERR_MBX
 **/
__s32 e1000_check_for_ack(struct e1000_hw *hw, __u16 mbx_id)
{
	struct e1000_mbx_info *mbx = &hw->mbx;
	__s32 ret_val = -E1000_ERR_MBX;

	uk_pr_info("e1000_check_for_ack\n");

	if (mbx->ops.check_for_ack)
		ret_val = mbx->ops.check_for_ack(hw, mbx_id);

	return ret_val;
}

/**
 *  e1000_check_for_rst - checks to see if other side has reset
 *  @hw: pointer to the HW structure
 *  @mbx_id: id of mailbox to check
 *
 *  returns SUCCESS if the Status bit was found or else ERR_MBX
 **/
__s32 e1000_check_for_rst(struct e1000_hw *hw, __u16 mbx_id)
{
	struct e1000_mbx_info *mbx = &hw->mbx;
	__s32 ret_val = -E1000_ERR_MBX;

	uk_pr_info("e1000_check_for_rst\n");

	if (mbx->ops.check_for_rst)
		ret_val = mbx->ops.check_for_rst(hw, mbx_id);

	return ret_val;
}

/**
 *  e1000_poll_for_msg - Wait for message notification
 *  @hw: pointer to the HW structure
 *  @mbx_id: id of mailbox to write
 *
 *  returns SUCCESS if it successfully received a message notification
 **/
STATIC __s32 e1000_poll_for_msg(struct e1000_hw *hw, __u16 mbx_id)
{
	struct e1000_mbx_info *mbx = &hw->mbx;
	int countdown = mbx->timeout;

	uk_pr_info("e1000_poll_for_msg\n");

	if (!countdown || !mbx->ops.check_for_msg)
		goto out;

	while (countdown && mbx->ops.check_for_msg(hw, mbx_id)) {
		countdown--;
		if (!countdown)
			break;
		usec_delay(mbx->usec_delay);
	}

	/* if we failed, all future posted messages fail until reset */
	if (!countdown)
		mbx->timeout = 0;
out:
	return countdown ? E1000_SUCCESS : -E1000_ERR_MBX;
}

/**
 *  e1000_poll_for_ack - Wait for message acknowledgement
 *  @hw: pointer to the HW structure
 *  @mbx_id: id of mailbox to write
 *
 *  returns SUCCESS if it successfully received a message acknowledgement
 **/
STATIC __s32 e1000_poll_for_ack(struct e1000_hw *hw, __u16 mbx_id)
{
	struct e1000_mbx_info *mbx = &hw->mbx;
	int countdown = mbx->timeout;

	uk_pr_info("e1000_poll_for_ack\n");

	if (!countdown || !mbx->ops.check_for_ack)
		goto out;

	while (countdown && mbx->ops.check_for_ack(hw, mbx_id)) {
		countdown--;
		if (!countdown)
			break;
		usec_delay(mbx->usec_delay);
	}

	/* if we failed, all future posted messages fail until reset */
	if (!countdown)
		mbx->timeout = 0;
out:
	return countdown ? E1000_SUCCESS : -E1000_ERR_MBX;
}

/**
 *  e1000_read_posted_mbx - Wait for message notification and receive message
 *  @hw: pointer to the HW structure
 *  @msg: The message buffer
 *  @size: Length of buffer
 *  @mbx_id: id of mailbox to write
 *
 *  returns SUCCESS if it successfully received a message notification and
 *  copied it into the receive buffer.
 **/
__s32 e1000_read_posted_mbx(struct e1000_hw *hw, __u32 *msg, __u16 size, __u16 mbx_id)
{
	struct e1000_mbx_info *mbx = &hw->mbx;
	__s32 ret_val = -E1000_ERR_MBX;

	uk_pr_info("e1000_read_posted_mbx\n");

	if (!mbx->ops.read)
		goto out;

	ret_val = e1000_poll_for_msg(hw, mbx_id);

	/* if ack received read message, otherwise we timed out */
	if (!ret_val)
		ret_val = mbx->ops.read(hw, msg, size, mbx_id);
out:
	return ret_val;
}

/**
 *  e1000_write_posted_mbx - Write a message to the mailbox, wait for ack
 *  @hw: pointer to the HW structure
 *  @msg: The message buffer
 *  @size: Length of buffer
 *  @mbx_id: id of mailbox to write
 *
 *  returns SUCCESS if it successfully copied message into the buffer and
 *  received an ack to that message within delay * timeout period
 **/
__s32 e1000_write_posted_mbx(struct e1000_hw *hw, __u32 *msg, __u16 size, __u16 mbx_id)
{
	struct e1000_mbx_info *mbx = &hw->mbx;
	__s32 ret_val = -E1000_ERR_MBX;

	uk_pr_info("e1000_write_posted_mbx\n");

	/* exit if either we can't write or there isn't a defined timeout */
	if (!mbx->ops.write || !mbx->timeout)
		goto out;

	/* send msg */
	ret_val = mbx->ops.write(hw, msg, size, mbx_id);

	/* if msg sent wait until we receive an ack */
	if (!ret_val)
		ret_val = e1000_poll_for_ack(hw, mbx_id);
out:
	return ret_val;
}

/**
 *  e1000_init_mbx_ops_generic - Initialize mbx function pointers
 *  @hw: pointer to the HW structure
 *
 *  Sets the function pointers to no-op functions
 **/
void e1000_init_mbx_ops_generic(struct e1000_hw *hw)
{
	struct e1000_mbx_info *mbx = &hw->mbx;
	mbx->ops.init_params = e1000_null_ops_generic;
	mbx->ops.read = e1000_null_mbx_transact;
	mbx->ops.write = e1000_null_mbx_transact;
	mbx->ops.check_for_msg = e1000_null_mbx_check_for_flag;
	mbx->ops.check_for_ack = e1000_null_mbx_check_for_flag;
	mbx->ops.check_for_rst = e1000_null_mbx_check_for_flag;
	mbx->ops.read_posted = e1000_read_posted_mbx;
	mbx->ops.write_posted = e1000_write_posted_mbx;
}

/**
 *  e1000_read_v2p_mailbox - read v2p mailbox
 *  @hw: pointer to the HW structure
 *
 *  This function is used to read the v2p mailbox without losing the read to
 *  clear status bits.
 **/
STATIC __u32 e1000_read_v2p_mailbox(struct e1000_hw *hw)
{
	__u32 v2p_mailbox = E1000_READ_REG(hw, E1000_V2PMAILBOX(0));

	// v2p_mailbox |= hw->dev_spec.vf.v2p_mailbox;
	// hw->dev_spec.vf.v2p_mailbox |= v2p_mailbox & E1000_V2PMAILBOX_R2C_BITS;

	return v2p_mailbox;
}

/**
 *  e1000_check_for_bit_vf - Determine if a status bit was set
 *  @hw: pointer to the HW structure
 *  @mask: bitmask for bits to be tested and cleared
 *
 *  This function is used to check for the read to clear bits within
 *  the V2P mailbox.
 **/
STATIC __s32 e1000_check_for_bit_vf(struct e1000_hw *hw, __u32 mask)
{
	__u32 v2p_mailbox = e1000_read_v2p_mailbox(hw);
	__s32 ret_val = -E1000_ERR_MBX;

	if (v2p_mailbox & mask)
		ret_val = E1000_SUCCESS;

	// hw->dev_spec.vf.v2p_mailbox &= ~mask;

	return ret_val;
}

/**
 *  e1000_check_for_msg_vf - checks to see if the PF has sent mail
 *  @hw: pointer to the HW structure
 *  @mbx_id: id of mailbox to check
 *
 *  returns SUCCESS if the PF has set the Status bit or else ERR_MBX
 **/
STATIC __s32 e1000_check_for_msg_vf(struct e1000_hw *hw,
				  __u16 __unused mbx_id)
{
	__s32 ret_val = -E1000_ERR_MBX;

	UNREFERENCED_1PARAMETER(mbx_id);
	uk_pr_info("e1000_check_for_msg_vf\n");

	if (!e1000_check_for_bit_vf(hw, E1000_V2PMAILBOX_PFSTS)) {
		ret_val = E1000_SUCCESS;
	}

	return ret_val;
}

/**
 *  e1000_check_for_ack_vf - checks to see if the PF has ACK'd
 *  @hw: pointer to the HW structure
 *  @mbx_id: id of mailbox to check
 *
 *  returns SUCCESS if the PF has set the ACK bit or else ERR_MBX
 **/
STATIC __s32 e1000_check_for_ack_vf(struct e1000_hw *hw,
				  __u16 __unused mbx_id)
{
	__s32 ret_val = -E1000_ERR_MBX;

	UNREFERENCED_1PARAMETER(mbx_id);
	uk_pr_info("e1000_check_for_ack_vf\n");

	if (!e1000_check_for_bit_vf(hw, E1000_V2PMAILBOX_PFACK)) {
		ret_val = E1000_SUCCESS;
	}

	return ret_val;
}

/**
 *  e1000_check_for_rst_vf - checks to see if the PF has reset
 *  @hw: pointer to the HW structure
 *  @mbx_id: id of mailbox to check
 *
 *  returns true if the PF has set the reset done bit or else false
 **/
STATIC __s32 e1000_check_for_rst_vf(struct e1000_hw *hw,
				  __u16 __unused mbx_id)
{
	__s32 ret_val = -E1000_ERR_MBX;

	UNREFERENCED_1PARAMETER(mbx_id);
	uk_pr_info("e1000_check_for_rst_vf\n");

	if (!e1000_check_for_bit_vf(hw, (E1000_V2PMAILBOX_RSTD |
					 E1000_V2PMAILBOX_RSTI))) {
		ret_val = E1000_SUCCESS;
	}

	return ret_val;
}

/**
 *  e1000_obtain_mbx_lock_vf - obtain mailbox lock
 *  @hw: pointer to the HW structure
 *
 *  return SUCCESS if we obtained the mailbox lock
 **/
STATIC __s32 e1000_obtain_mbx_lock_vf(struct e1000_hw *hw)
{
	__s32 ret_val = -E1000_ERR_MBX;
	int count = 10;

	uk_pr_info("e1000_obtain_mbx_lock_vf\n");

	do {
		/* Take ownership of the buffer */
		E1000_WRITE_REG(hw, E1000_V2PMAILBOX(0), E1000_V2PMAILBOX_VFU);

		/* reserve mailbox for vf use */
		if (e1000_read_v2p_mailbox(hw) & E1000_V2PMAILBOX_VFU) {
			ret_val = E1000_SUCCESS;
			break;
		}
		usec_delay(1000);
	} while (count-- > 0);

	return ret_val;
}

/**
 *  e1000_write_mbx_vf - Write a message to the mailbox
 *  @hw: pointer to the HW structure
 *  @msg: The message buffer
 *  @size: Length of buffer
 *  @mbx_id: id of mailbox to write
 *
 *  returns SUCCESS if it successfully copied message into the buffer
 **/
STATIC __s32 e1000_write_mbx_vf(struct e1000_hw *hw, __u32 *msg, __u16 size,
			      __u16 __unused mbx_id)
{
	__s32 ret_val;
	__u16 i;

	UNREFERENCED_1PARAMETER(mbx_id);

	uk_pr_info("e1000_write_mbx_vf\n");

	/* lock the mailbox to prevent pf/vf race condition */
	ret_val = e1000_obtain_mbx_lock_vf(hw);
	if (ret_val)
		goto out_no_write;

	/* flush msg and acks as we are overwriting the message buffer */
	e1000_check_for_msg_vf(hw, 0);
	e1000_check_for_ack_vf(hw, 0);

	/* copy the caller specified message to the mailbox memory buffer */
	for (i = 0; i < size; i++)
		E1000_WRITE_REG_ARRAY(hw, E1000_VMBMEM(0), i, msg[i]);

	/* Drop VFU and interrupt the PF to tell it a message has been sent */
	E1000_WRITE_REG(hw, E1000_V2PMAILBOX(0), E1000_V2PMAILBOX_REQ);

out_no_write:
	return ret_val;
}

/**
 *  e1000_read_mbx_vf - Reads a message from the inbox intended for vf
 *  @hw: pointer to the HW structure
 *  @msg: The message buffer
 *  @size: Length of buffer
 *  @mbx_id: id of mailbox to read
 *
 *  returns SUCCESS if it successfully read message from buffer
 **/
STATIC __s32 e1000_read_mbx_vf(struct e1000_hw *hw, __u32 *msg, __u16 size,
			     __u16 __unused mbx_id)
{
	__s32 ret_val = E1000_SUCCESS;
	__u16 i;

	uk_pr_info("e1000_read_mbx_vf\n");
	UNREFERENCED_1PARAMETER(mbx_id);

	/* lock the mailbox to prevent pf/vf race condition */
	ret_val = e1000_obtain_mbx_lock_vf(hw);
	if (ret_val)
		goto out_no_read;

	/* copy the message from the mailbox memory buffer */
	for (i = 0; i < size; i++)
		msg[i] = E1000_READ_REG_ARRAY(hw, E1000_VMBMEM(0), i);

	/* Acknowledge receipt and release mailbox, then we're done */
	E1000_WRITE_REG(hw, E1000_V2PMAILBOX(0), E1000_V2PMAILBOX_ACK);

out_no_read:
	return ret_val;
}

/**
 *  e1000_init_mbx_params_vf - set initial values for vf mailbox
 *  @hw: pointer to the HW structure
 *
 *  Initializes the hw->mbx struct to correct values for vf mailbox
 */
__s32 e1000_init_mbx_params_vf(struct e1000_hw *hw)
{
	struct e1000_mbx_info *mbx = &hw->mbx;

	/* start mailbox as timed out and let the reset_hw call set the timeout
	 * value to begin communications */
	mbx->timeout = 0;
	mbx->usec_delay = E1000_VF_MBX_INIT_DELAY;

	mbx->size = E1000_VFMAILBOX_SIZE;

	mbx->ops.read = e1000_read_mbx_vf;
	mbx->ops.write = e1000_write_mbx_vf;
	mbx->ops.read_posted = e1000_read_posted_mbx;
	mbx->ops.write_posted = e1000_write_posted_mbx;
	mbx->ops.check_for_msg = e1000_check_for_msg_vf;
	mbx->ops.check_for_ack = e1000_check_for_ack_vf;
	mbx->ops.check_for_rst = e1000_check_for_rst_vf;

	return E1000_SUCCESS;
}