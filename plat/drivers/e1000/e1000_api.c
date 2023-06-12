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

#include <e1000/e1000_api.h>

/**
 *  e1000_init_mac_params - Initialize MAC function pointers
 *  @hw: pointer to the HW structure
 *
 *  This function initializes the function pointers for the MAC
 *  set of functions.  Called by drivers or by e1000_setup_init_funcs.
 **/
__s32 e1000_init_mac_params(struct e1000_hw *hw)
{
	__s32 ret_val = E1000_SUCCESS;

	if (hw->mac.ops.init_params) {
		ret_val = hw->mac.ops.init_params(hw);
		if (ret_val) {
			uk_pr_err("MAC Initialization Error\n");
			goto out;
		}
	} else {
		uk_pr_err("mac.init_mac_params was NULL\n");
		ret_val = -E1000_ERR_CONFIG;
	}

out:
	return ret_val;
}

/**
 *  e1000_init_nvm_params - Initialize NVM function pointers
 *  @hw: pointer to the HW structure
 *
 *  This function initializes the function pointers for the NVM
 *  set of functions.  Called by drivers or by e1000_setup_init_funcs.
 **/
__s32 e1000_init_nvm_params(struct e1000_hw *hw)
{
	__s32 ret_val = E1000_SUCCESS;

	if (hw->nvm.ops.init_params) {
		ret_val = hw->nvm.ops.init_params(hw);
		if (ret_val) {
			uk_pr_err("NVM Initialization Error\n");
			goto out;
		}
	} else {
		uk_pr_err("nvm.init_nvm_params was NULL\n");
		ret_val = -E1000_ERR_CONFIG;
	}

out:
	return ret_val;
}

/**
 *  e1000_init_phy_params - Initialize PHY function pointers
 *  @hw: pointer to the HW structure
 *
 *  This function initializes the function pointers for the PHY
 *  set of functions.  Called by drivers or by e1000_setup_init_funcs.
 **/
__s32 e1000_init_phy_params(struct e1000_hw *hw)
{
	__s32 ret_val = E1000_SUCCESS;

	debug_uk_pr_info("e1000_init_phy_params\n");

	if (hw->phy.ops.init_params) {
		ret_val = hw->phy.ops.init_params(hw);
		if (ret_val) {
			uk_pr_info("PHY Initialization Error\n");
			goto out;
		}
	} else {
		uk_pr_info("phy.init_phy_params was NULL\n");
		ret_val =  -E1000_ERR_CONFIG;
	}

out:
	return ret_val;
}

/**
 *  e1000_init_mbx_params - Initialize mailbox function pointers
 *  @hw: pointer to the HW structure
 *
 *  This function initializes the function pointers for the PHY
 *  set of functions.  Called by drivers or by e1000_setup_init_funcs.
 **/
__s32 e1000_init_mbx_params(struct e1000_hw *hw)
{
	__s32 ret_val = E1000_SUCCESS;

	debug_uk_pr_info("e1000_init_mbx_params\n");

	if (hw->mbx.ops.init_params) {
		ret_val = hw->mbx.ops.init_params(hw);
		if (ret_val) {
			uk_pr_err("Mailbox Initialization Error\n");
			goto out;
		}
	} else {
		uk_pr_err("mbx.init_mbx_params was NULL\n");
		ret_val =  -E1000_ERR_CONFIG;
	}

out:
	return ret_val;
}

/**
 *  e1000_set_mac_type - Sets MAC type
 *  @hw: pointer to the HW structure
 *
 *  This function sets the mac type of the adapter based on the
 *  device ID stored in the hw structure.
 *  MUST BE FIRST FUNCTION CALLED (explicitly or through
 *  e1000_setup_init_funcs()).
 **/
__s32 e1000_set_mac_type(struct e1000_hw *hw)
{
	struct e1000_mac_info *mac = &hw->mac;
	__s32 ret_val = E1000_SUCCESS;

	debug_uk_pr_info("e1000_set_mac_type\n");

    mac->type = e1000_82545;

	return ret_val;
}

/**
 *  e1000_setup_init_funcs - Initializes function pointers
 *  @hw: pointer to the HW structure
 *  @init_device: true will initialize the rest of the function pointers
 *		  getting the device ready for use.  false will only set
 *		  MAC type and the function pointers for the other init
 *		  functions.  Passing false will not generate any hardware
 *		  reads or writes.
 *
 *  This function must be called by a driver in order to use the rest
 *  of the 'shared' code files. Called by drivers only.
 **/
__s32 e1000_setup_init_funcs(struct e1000_hw *hw, bool init_device)
{
	__s32 ret_val;

	debug_uk_pr_info("e1000_setup_init_funcs\n");

	/* Can't do much good without knowing the MAC type. */
	ret_val = e1000_set_mac_type(hw);
	if (ret_val) {
		uk_pr_err("MAC type could not be set properly.\n");
		goto out;
	}

	if (!hw->hw_addr) {
		uk_pr_err("Registers not mapped\n");
		ret_val = -E1000_ERR_CONFIG;
		goto out;
	}

	/*
	 * Init function pointers to generic implementations. We do this first
	 * allowing a driver module to override it afterward.
	 */
	e1000_init_mac_ops_generic(hw);
	e1000_init_phy_ops_generic(hw);
	e1000_init_nvm_ops_generic(hw);
	e1000_init_mbx_ops_generic(hw);

	/*
	 * Set up the init function pointers. These are functions within the
	 * adapter family file that sets up function pointers for the rest of
	 * the functions in that family.
	 */
	uk_pr_info("calling e1000_init_function_pointers_82540\n");
    e1000_init_function_pointers_82540(hw);
	ret_val = e1000_init_mac_params(hw);
	ret_val = e1000_init_nvm_params(hw);


	/*
	 * Initialize the rest of the function pointers. These require some
	 * register reads/writes in some cases.
	 */
	if (!(ret_val) && init_device) {
		ret_val = e1000_init_mac_params(hw);
		if (ret_val) {
			uk_pr_info("e1000_init_mac_params: ret_val %d\n", ret_val);
			goto out;
		}

		ret_val = e1000_init_nvm_params(hw);
		if (ret_val) {
			uk_pr_info("e1000_init_nvm_params: ret_val %d\n", ret_val);
			goto out;
		}

		ret_val = e1000_init_phy_params(hw);
		if (ret_val) {
			uk_pr_info("e1000_init_phy_params: ret_val %d\n", ret_val);
			goto out;
		}

		ret_val = e1000_init_mbx_params(hw);
		if (ret_val) {
			uk_pr_info("e1000_init_mbx_params: ret_val %d\n", ret_val);
			goto out;
		}
	}

out:
	uk_pr_info("out: ret_val %d\n", ret_val);
	return ret_val;
}

/**
 *  e1000_get_bus_info - Obtain bus information for adapter
 *  @hw: pointer to the HW structure
 *
 *  This will obtain information about the HW bus for which the
 *  adapter is attached and stores it in the hw structure. This is a
 *  function pointer entry point called by drivers.
 **/
__s32 e1000_get_bus_info(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_get_bus_info\n");
	if (hw->mac.ops.get_bus_info) {
		return hw->mac.ops.get_bus_info(hw);
	}
	
	return E1000_SUCCESS;
}


/**
 *  e1000_update_mc_addr_list - Update Multicast addresses
 *  @hw: pointer to the HW structure
 *  @mc_addr_list: array of multicast addresses to program
 *  @mc_addr_count: number of multicast addresses to program
 *
 *  Updates the Multicast Table Array.
 *  The caller must have a packed mc_addr_list of multicast addresses.
 **/
void e1000_update_mc_addr_list(struct e1000_hw *hw, __u8 *mc_addr_list,
			       __u32 mc_addr_count)
{
	debug_uk_pr_info("e1000_update_mc_addr_list\n");
	if (hw->mac.ops.update_mc_addr_list)
		hw->mac.ops.update_mc_addr_list(hw, mc_addr_list,
						mc_addr_count);
}

/**
 *  e1000_force_mac_fc - Force MAC flow control
 *  @hw: pointer to the HW structure
 *
 *  Force the MAC's flow control settings. Currently no func pointer exists
 *  and all implementations are handled in the generic version of this
 *  function.
 **/
__s32 e1000_force_mac_fc(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_force_mac_fc\n");
	return e1000_force_mac_fc_generic(hw);
}

/**
 *  e1000_check_for_link - Check/Store link connection
 *  @hw: pointer to the HW structure
 *
 *  This checks the link condition of the adapter and stores the
 *  results in the hw->mac structure. This is a function pointer entry
 *  point called by drivers.
 **/
__s32 e1000_check_for_link(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_check_for_link\n");

	if (hw->mac.ops.check_for_link)
		return hw->mac.ops.check_for_link(hw);

	return -E1000_ERR_CONFIG;
}

/**
 *  e1000_check_mng_mode - Check management mode
 *  @hw: pointer to the HW structure
 *
 *  This checks if the adapter has manageability enabled.
 *  This is a function pointer entry point called by drivers.
 **/
bool e1000_check_mng_mode(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_check_mng_mode\n");
	if (hw->mac.ops.check_mng_mode)
		return hw->mac.ops.check_mng_mode(hw);

	return false;
}

/**
 *  e1000_reset_hw - Reset hardware
 *  @hw: pointer to the HW structure
 *
 *  This resets the hardware into a known state. This is a function pointer
 *  entry point called by drivers.
 **/
__s32 e1000_reset_hw(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_reset_hw\n");
	
	if (hw->mac.ops.reset_hw)
		return hw->mac.ops.reset_hw(hw);

	return -E1000_ERR_CONFIG;
}

/**
 *  e1000_init_hw - Initialize hardware
 *  @hw: pointer to the HW structure
 *
 *  This inits the hardware readying it for operation. This is a function
 *  pointer entry point called by drivers.
 **/
__s32 e1000_init_hw(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_init_hw\n");

	if (hw->mac.ops.init_hw)
		return hw->mac.ops.init_hw(hw);

	return -E1000_ERR_CONFIG;
}

/**
 *  e1000_setup_link - Configures link and flow control
 *  @hw: pointer to the HW structure
 *
 *  This configures link and flow control settings for the adapter. This
 *  is a function pointer entry point called by drivers. While modules can
 *  also call this, they probably call their own version of this function.
 **/
__s32 e1000_setup_link(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_setup_link\n");

	if (hw->mac.ops.setup_link)
		return hw->mac.ops.setup_link(hw);

	return -E1000_ERR_CONFIG;
}

/**
 *  e1000_get_speed_and_duplex - Returns current speed and duplex
 *  @hw: pointer to the HW structure
 *  @speed: pointer to a 16-bit value to store the speed
 *  @duplex: pointer to a 16-bit value to store the duplex.
 *
 *  This returns the speed and duplex of the adapter in the two 'out'
 *  variables passed in. This is a function pointer entry point called
 *  by drivers.
 **/
__s32 e1000_get_speed_and_duplex(struct e1000_hw *hw, __u16 *speed, __u16 *duplex)
{
	debug_uk_pr_info("e1000_get_speed_and_duplex\n");

	if (hw->mac.ops.get_link_up_info)
		return hw->mac.ops.get_link_up_info(hw, speed, duplex);

	return -E1000_ERR_CONFIG;
}

/**
 *  e1000_setup_led - Configures SW controllable LED
 *  @hw: pointer to the HW structure
 *
 *  This prepares the SW controllable LED for use and saves the current state
 *  of the LED so it can be later restored. This is a function pointer entry
 *  point called by drivers.
 **/
__s32 e1000_setup_led(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_setup_led\n");

	if (hw->mac.ops.setup_led)
		return hw->mac.ops.setup_led(hw);

	return E1000_SUCCESS;
}

/**
 *  e1000_cleanup_led - Restores SW controllable LED
 *  @hw: pointer to the HW structure
 *
 *  This restores the SW controllable LED to the value saved off by
 *  e1000_setup_led. This is a function pointer entry point called by drivers.
 **/
__s32 e1000_cleanup_led(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_cleanup_led\n");

	if (hw->mac.ops.cleanup_led)
		return hw->mac.ops.cleanup_led(hw);

	return E1000_SUCCESS;
}

/**
 *  e1000_blink_led - Blink SW controllable LED
 *  @hw: pointer to the HW structure
 *
 *  This starts the adapter LED blinking. Request the LED to be setup first
 *  and cleaned up after. This is a function pointer entry point called by
 *  drivers.
 **/
__s32 e1000_blink_led(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_blink_led\n");

	if (hw->mac.ops.blink_led)
		return hw->mac.ops.blink_led(hw);

	return E1000_SUCCESS;
}

/**
 *  e1000_id_led_init - store LED configurations in SW
 *  @hw: pointer to the HW structure
 *
 *  Initializes the LED config in SW. This is a function pointer entry point
 *  called by drivers.
 **/
__s32 e1000_id_led_init(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_id_led_init\n");

	if (hw->mac.ops.id_led_init)
		return hw->mac.ops.id_led_init(hw);

	return E1000_SUCCESS;
}

/**
 *  e1000_led_on - Turn on SW controllable LED
 *  @hw: pointer to the HW structure
 *
 *  Turns the SW defined LED on. This is a function pointer entry point
 *  called by drivers.
 **/
__s32 e1000_led_on(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_led_on\n");

	if (hw->mac.ops.led_on)
		return hw->mac.ops.led_on(hw);

	return E1000_SUCCESS;
}

/**
 *  e1000_led_off - Turn off SW controllable LED
 *  @hw: pointer to the HW structure
 *
 *  Turns the SW defined LED off. This is a function pointer entry point
 *  called by drivers.
 **/
__s32 e1000_led_off(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_led_off\n");

	if (hw->mac.ops.led_off)
		return hw->mac.ops.led_off(hw);

	return E1000_SUCCESS;
}

/**
 *  e1000_reset_adaptive - Reset adaptive IFS
 *  @hw: pointer to the HW structure
 *
 *  Resets the adaptive IFS. Currently no func pointer exists and all
 *  implementations are handled in the generic version of this function.
 **/
void e1000_reset_adaptive(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_reset_adaptive\n");
	e1000_reset_adaptive_generic(hw);
}

/**
 *  e1000_update_adaptive - Update adaptive IFS
 *  @hw: pointer to the HW structure
 *
 *  Updates adapter IFS. Currently no func pointer exists and all
 *  implementations are handled in the generic version of this function.
 **/
void e1000_update_adaptive(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_update_adaptive\n");
	e1000_update_adaptive_generic(hw);
}

/**
 *  e1000_disable_pcie_master - Disable PCI-Express master access
 *  @hw: pointer to the HW structure
 *
 *  Disables PCI-Express master access and verifies there are no pending
 *  requests. Currently no func pointer exists and all implementations are
 *  handled in the generic version of this function.
 **/
__s32 e1000_disable_pcie_master(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_disable_pcie_master\n");
	return e1000_disable_pcie_master_generic(hw);
}

/**
 *  e1000_config_collision_dist - Configure collision distance
 *  @hw: pointer to the HW structure
 *
 *  Configures the collision distance to the default value and is used
 *  during link setup.
 **/
void e1000_config_collision_dist(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_config_collision_dist\n");
	if (hw->mac.ops.config_collision_dist)
		hw->mac.ops.config_collision_dist(hw);
}

/**
 *  e1000_rar_set - Sets a receive address register
 *  @hw: pointer to the HW structure
 *  @addr: address to set the RAR to
 *  @index: the RAR to set
 *
 *  Sets a Receive Address Register (RAR) to the specified address.
 **/
int e1000_rar_set(struct e1000_hw *hw, __u8 *addr, __u32 index)
{
	debug_uk_pr_info("e1000_rar_set\n");
	if (hw->mac.ops.rar_set)
		return hw->mac.ops.rar_set(hw, addr, index);

	return E1000_SUCCESS;
}

/**
 *  e1000_validate_mdi_setting - Ensures valid MDI/MDIX SW state
 *  @hw: pointer to the HW structure
 *
 *  Ensures that the MDI/MDIX SW state is valid.
 **/
__s32 e1000_validate_mdi_setting(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_validate_mdi_setting\n");
	if (hw->mac.ops.validate_mdi_setting)
		return hw->mac.ops.validate_mdi_setting(hw);

	return E1000_SUCCESS;
}

/**
 *  e1000_hash_mc_addr - Determines address location in multicast table
 *  @hw: pointer to the HW structure
 *  @mc_addr: Multicast address to hash.
 *
 *  This hashes an address to determine its location in the multicast
 *  table. Currently no func pointer exists and all implementations
 *  are handled in the generic version of this function.
 **/
__u32 e1000_hash_mc_addr(struct e1000_hw *hw, __u8 *mc_addr)
{
	debug_uk_pr_info("e1000_hash_mc_addr\n");
	return e1000_hash_mc_addr_generic(hw, mc_addr);
}

/**
 *  e1000_check_reset_block - Verifies PHY can be reset
 *  @hw: pointer to the HW structure
 *
 *  Checks if the PHY is in a state that can be reset or if manageability
 *  has it tied up. This is a function pointer entry point called by drivers.
 **/
__s32 e1000_check_reset_block(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_check_reset_block\n");
	if (hw->phy.ops.check_reset_block)
		return hw->phy.ops.check_reset_block(hw);

	return E1000_SUCCESS;
}

/**
 *  e1000_read_phy_reg - Reads PHY register
 *  @hw: pointer to the HW structure
 *  @offset: the register to read
 *  @data: the buffer to store the 16-bit read.
 *
 *  Reads the PHY register and returns the value in data.
 *  This is a function pointer entry point called by drivers.
 **/
__s32 e1000_read_phy_reg(struct e1000_hw *hw, __u32 offset, __u16 *data)
{
	debug_uk_pr_info("e1000_read_phy_reg\n");
	if (hw->phy.ops.read_reg)
		return hw->phy.ops.read_reg(hw, offset, data);

	return E1000_SUCCESS;
}

/**
 *  e1000_write_phy_reg - Writes PHY register
 *  @hw: pointer to the HW structure
 *  @offset: the register to write
 *  @data: the value to write.
 *
 *  Writes the PHY register at offset with the value in data.
 *  This is a function pointer entry point called by drivers.
 **/
__s32 e1000_write_phy_reg(struct e1000_hw *hw, __u32 offset, __u16 data)
{
	debug_uk_pr_info("e1000_write_phy_reg\n");
	if (hw->phy.ops.write_reg)
		return hw->phy.ops.write_reg(hw, offset, data);

	return E1000_SUCCESS;
}

/**
 *  e1000_release_phy - Generic release PHY
 *  @hw: pointer to the HW structure
 *
 *  Return if silicon family does not require a semaphore when accessing the
 *  PHY.
 **/
void e1000_release_phy(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_release_phy\n");
	if (hw->phy.ops.release)
		hw->phy.ops.release(hw);
}

/**
 *  e1000_acquire_phy - Generic acquire PHY
 *  @hw: pointer to the HW structure
 *
 *  Return success if silicon family does not require a semaphore when
 *  accessing the PHY.
 **/
__s32 e1000_acquire_phy(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_acquire_phy\n");
	if (hw->phy.ops.acquire)
		return hw->phy.ops.acquire(hw);

	return E1000_SUCCESS;
}

/**
 *  e1000_cfg_on_link_up - Configure PHY upon link up
 *  @hw: pointer to the HW structure
 **/
__s32 e1000_cfg_on_link_up(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_cfg_on_link_up\n");
	if (hw->phy.ops.cfg_on_link_up)
		return hw->phy.ops.cfg_on_link_up(hw);

	return E1000_SUCCESS;
}

/**
 *  e1000_get_cable_length - Retrieves cable length estimation
 *  @hw: pointer to the HW structure
 *
 *  This function estimates the cable length and stores them in
 *  hw->phy.min_length and hw->phy.max_length. This is a function pointer
 *  entry point called by drivers.
 **/
__s32 e1000_get_cable_length(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_get_cable_length\n");
	if (hw->phy.ops.get_cable_length)
		return hw->phy.ops.get_cable_length(hw);

	return E1000_SUCCESS;
}

/**
 *  e1000_get_phy_info - Retrieves PHY information from registers
 *  @hw: pointer to the HW structure
 *
 *  This function gets some information from various PHY registers and
 *  populates hw->phy values with it. This is a function pointer entry
 *  point called by drivers.
 **/
__s32 e1000_get_phy_info(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_get_phy_info\n");
	if (hw->phy.ops.get_info)
		return hw->phy.ops.get_info(hw);

	return E1000_SUCCESS;
}

/**
 *  e1000_phy_hw_reset - Hard PHY reset
 *  @hw: pointer to the HW structure
 *
 *  Performs a hard PHY reset. This is a function pointer entry point called
 *  by drivers.
 **/
__s32 e1000_phy_hw_reset(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_phy_hw_reset\n");
	if (hw->phy.ops.reset)
		return hw->phy.ops.reset(hw);

	return E1000_SUCCESS;
}

/**
 *  e1000_phy_commit - Soft PHY reset
 *  @hw: pointer to the HW structure
 *
 *  Performs a soft PHY reset on those that apply. This is a function pointer
 *  entry point called by drivers.
 **/
__s32 e1000_phy_commit(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_phy_commit\n");
	if (hw->phy.ops.commit)
		return hw->phy.ops.commit(hw);

	return E1000_SUCCESS;
}

/**
 *  e1000_set_d0_lplu_state - Sets low power link up state for D0
 *  @hw: pointer to the HW structure
 *  @active: boolean used to enable/disable lplu
 *
 *  Success returns 0, Failure returns 1
 *
 *  The low power link up (lplu) state is set to the power management level D0
 *  and SmartSpeed is disabled when active is true, else clear lplu for D0
 *  and enable Smartspeed.  LPLU and Smartspeed are mutually exclusive.  LPLU
 *  is used during Dx states where the power conservation is most important.
 *  During driver activity, SmartSpeed should be enabled so performance is
 *  maintained.  This is a function pointer entry point called by drivers.
 **/
__s32 e1000_set_d0_lplu_state(struct e1000_hw *hw, bool active)
{
	debug_uk_pr_info("e1000_set_d0_lplu_state\n");
	if (hw->phy.ops.set_d0_lplu_state)
		return hw->phy.ops.set_d0_lplu_state(hw, active);

	return E1000_SUCCESS;
}

/**
 *  e1000_set_d3_lplu_state - Sets low power link up state for D3
 *  @hw: pointer to the HW structure
 *  @active: boolean used to enable/disable lplu
 *
 *  Success returns 0, Failure returns 1
 *
 *  The low power link up (lplu) state is set to the power management level D3
 *  and SmartSpeed is disabled when active is true, else clear lplu for D3
 *  and enable Smartspeed.  LPLU and Smartspeed are mutually exclusive.  LPLU
 *  is used during Dx states where the power conservation is most important.
 *  During driver activity, SmartSpeed should be enabled so performance is
 *  maintained.  This is a function pointer entry point called by drivers.
 **/
__s32 e1000_set_d3_lplu_state(struct e1000_hw *hw, bool active)
{
	debug_uk_pr_info("e1000_set_d3_lplu_state\n");
	if (hw->phy.ops.set_d3_lplu_state)
		return hw->phy.ops.set_d3_lplu_state(hw, active);

	return E1000_SUCCESS;
}

/**
 *  e1000_read_mac_addr - Reads MAC address
 *  @hw: pointer to the HW structure
 *
 *  Reads the MAC address out of the adapter and stores it in the HW structure.
 *  Currently no func pointer exists and all implementations are handled in the
 *  generic version of this function.
 **/
__s32 e1000_read_mac_addr(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_read_mac_addr\n");
	if (hw->mac.ops.read_mac_addr)
		return hw->mac.ops.read_mac_addr(hw);

	return e1000_read_mac_addr_generic(hw);
}

/**
 *  e1000_read_pba_string - Read device part number string
 *  @hw: pointer to the HW structure
 *  @pba_num: pointer to device part number
 *  @pba_num_size: size of part number buffer
 *
 *  Reads the product board assembly (PBA) number from the EEPROM and stores
 *  the value in pba_num.
 *  Currently no func pointer exists and all implementations are handled in the
 *  generic version of this function.
 **/
__s32 e1000_read_pba_string(struct e1000_hw *hw, __u8 *pba_num, __u32 pba_num_size)
{
	debug_uk_pr_info("e1000_read_pba_string\n");
	return e1000_read_pba_string_generic(hw, pba_num, pba_num_size);
}

/**
 *  e1000_read_pba_length - Read device part number string length
 *  @hw: pointer to the HW structure
 *  @pba_num_size: size of part number buffer
 *
 *  Reads the product board assembly (PBA) number length from the EEPROM and
 *  stores the value in pba_num.
 *  Currently no func pointer exists and all implementations are handled in the
 *  generic version of this function.
 **/
__s32 e1000_read_pba_length(struct e1000_hw *hw, __u32 *pba_num_size)
{
	debug_uk_pr_info("e1000_read_pba_length\n");
	return e1000_read_pba_length_generic(hw, pba_num_size);
}

/**
 *  e1000_read_pba_num - Read device part number
 *  @hw: pointer to the HW structure
 *  @pba_num: pointer to device part number
 *
 *  Reads the product board assembly (PBA) number from the EEPROM and stores
 *  the value in pba_num.
 *  Currently no func pointer exists and all implementations are handled in the
 *  generic version of this function.
 **/
__s32 e1000_read_pba_num(struct e1000_hw *hw, __u32 *pba_num)
{
	debug_uk_pr_info("e1000_read_pba_num\n");
	return e1000_read_pba_num_generic(hw, pba_num);
}

/**
 *  e1000_validate_nvm_checksum - Verifies NVM (EEPROM) checksum
 *  @hw: pointer to the HW structure
 *
 *  Validates the NVM checksum is correct. This is a function pointer entry
 *  point called by drivers.
 **/
__s32 e1000_validate_nvm_checksum(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_validate_nvm_checksum\n");
	if (hw->nvm.ops.validate)
		return hw->nvm.ops.validate(hw);

	return -E1000_ERR_CONFIG;
}

/**
 *  e1000_update_nvm_checksum - Updates NVM (EEPROM) checksum
 *  @hw: pointer to the HW structure
 *
 *  Updates the NVM checksum. Currently no func pointer exists and all
 *  implementations are handled in the generic version of this function.
 **/
__s32 e1000_update_nvm_checksum(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_update_nvm_checksum\n");
	if (hw->nvm.ops.update)
		return hw->nvm.ops.update(hw);

	return -E1000_ERR_CONFIG;
}

/**
 *  e1000_reload_nvm - Reloads EEPROM
 *  @hw: pointer to the HW structure
 *
 *  Reloads the EEPROM by setting the "Reinitialize from EEPROM" bit in the
 *  extended control register.
 **/
void e1000_reload_nvm(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_reload_nvm\n");
	if (hw->nvm.ops.reload)
		hw->nvm.ops.reload(hw);
}

/**
 *  e1000_read_nvm - Reads NVM (EEPROM)
 *  @hw: pointer to the HW structure
 *  @offset: the word offset to read
 *  @words: number of 16-bit words to read
 *  @data: pointer to the properly sized buffer for the data.
 *
 *  Reads 16-bit chunks of data from the NVM (EEPROM). This is a function
 *  pointer entry point called by drivers.
 **/
__s32 e1000_read_nvm(struct e1000_hw *hw, __u16 offset, __u16 words, __u16 *data)
{
	debug_uk_pr_info("e1000_read_nvm\n");
	if (hw->nvm.ops.read)
		return hw->nvm.ops.read(hw, offset, words, data);

	return -E1000_ERR_CONFIG;
}

/**
 *  e1000_write_nvm - Writes to NVM (EEPROM)
 *  @hw: pointer to the HW structure
 *  @offset: the word offset to read
 *  @words: number of 16-bit words to write
 *  @data: pointer to the properly sized buffer for the data.
 *
 *  Writes 16-bit chunks of data to the NVM (EEPROM). This is a function
 *  pointer entry point called by drivers.
 **/
__s32 e1000_write_nvm(struct e1000_hw *hw, __u16 offset, __u16 words, __u16 *data)
{
	debug_uk_pr_info("e1000_write_nvm\n");
	if (hw->nvm.ops.write)
		return hw->nvm.ops.write(hw, offset, words, data);

	return E1000_SUCCESS;
}

/**
 *  e1000_write_8bit_ctrl_reg - Writes 8bit Control register
 *  @hw: pointer to the HW structure
 *  @reg: 32bit register offset
 *  @offset: the register to write
 *  @data: the value to write.
 *
 *  Writes the PHY register at offset with the value in data.
 *  This is a function pointer entry point called by drivers.
 **/
__s32 e1000_write_8bit_ctrl_reg(struct e1000_hw *hw, __u32 reg, __u32 offset,
			      __u8 data)
{
	debug_uk_pr_info("e1000_write_8bit_ctrl_reg\n");
	return e1000_write_8bit_ctrl_reg_generic(hw, reg, offset, data);
}

/**
 * e1000_power_up_phy - Restores link in case of PHY power down
 * @hw: pointer to the HW structure
 *
 * The phy may be powered down to save power, to turn off link when the
 * driver is unloaded, or wake on lan is not enabled (among others).
 **/
void e1000_power_up_phy(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_power_up_phy\n");
	if (hw->phy.ops.power_up)
		hw->phy.ops.power_up(hw);

	e1000_setup_link(hw);
}

/**
 * e1000_power_down_phy - Power down PHY
 * @hw: pointer to the HW structure
 *
 * The phy may be powered down to save power, to turn off link when the
 * driver is unloaded, or wake on lan is not enabled (among others).
 **/
void e1000_power_down_phy(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_power_down_phy\n");
	if (hw->phy.ops.power_down)
		hw->phy.ops.power_down(hw);
}

/**
 *  e1000_power_up_fiber_serdes_link - Power up serdes link
 *  @hw: pointer to the HW structure
 *
 *  Power on the optics and PCS.
 **/
void e1000_power_up_fiber_serdes_link(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_power_up_fiber_serdes_link\n");
	if (hw->mac.ops.power_up_serdes)
		hw->mac.ops.power_up_serdes(hw);
}

/**
 *  e1000_shutdown_fiber_serdes_link - Remove link during power down
 *  @hw: pointer to the HW structure
 *
 *  Shutdown the optics and PCS on driver unload.
 **/
void e1000_shutdown_fiber_serdes_link(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_shutdown_fiber_serdes_link\n");
	if (hw->mac.ops.shutdown_serdes)
		hw->mac.ops.shutdown_serdes(hw);
}

