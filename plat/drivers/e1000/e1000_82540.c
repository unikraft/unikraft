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

/*
 * 82540EM Gigabit Ethernet Controller
 * 82540EP Gigabit Ethernet Controller
 * 82545EM Gigabit Ethernet Controller (Copper)
 * 82545EM Gigabit Ethernet Controller (Fiber)
 * 82545GM Gigabit Ethernet Controller
 * 82546EB Gigabit Ethernet Controller (Copper)
 * 82546EB Gigabit Ethernet Controller (Fiber)
 * 82546GB Gigabit Ethernet Controller
 */

#include <e1000/e1000_api.h>

static __s32  e1000_init_phy_params_82540(struct e1000_hw *hw);
static __s32  e1000_init_nvm_params_82540(struct e1000_hw *hw);
static __s32  e1000_init_mac_params_82540(struct e1000_hw *hw);
static __s32  e1000_adjust_serdes_amplitude_82540(struct e1000_hw *hw);
static void e1000_clear_hw_cntrs_82540(struct e1000_hw *hw);
static __s32  e1000_init_hw_82540(struct e1000_hw *hw);
static __s32  e1000_reset_hw_82540(struct e1000_hw *hw);
static __s32  e1000_set_phy_mode_82540(struct e1000_hw *hw);
static __s32  e1000_set_vco_speed_82540(struct e1000_hw *hw);
static __s32  e1000_setup_copper_link_82540(struct e1000_hw *hw);
static __s32  e1000_setup_fiber_serdes_link_82540(struct e1000_hw *hw);
static void e1000_power_down_phy_copper_82540(struct e1000_hw *hw);
static __s32  e1000_read_mac_addr_82540(struct e1000_hw *hw);

/**
 * e1000_init_phy_params_82540 - Init PHY func ptrs.
 * @hw: pointer to the HW structure
 **/
static __s32 e1000_init_phy_params_82540(struct e1000_hw *hw)
{
	struct e1000_phy_info *phy = &hw->phy;
	__s32 ret_val;

	debug_uk_pr_info("e1000_init_phy_params_82540\n");

	phy->addr		= 1;
	phy->autoneg_mask	= AUTONEG_ADVERTISE_SPEED_DEFAULT;
	phy->reset_delay_us	= 10000;
	phy->type		= e1000_phy_m88;

	/* Function Pointers */
	phy->ops.check_polarity	= e1000_check_polarity_m88;
	phy->ops.commit		= e1000_phy_sw_reset_generic;
	phy->ops.force_speed_duplex = e1000_phy_force_speed_duplex_m88;
	phy->ops.get_cable_length = e1000_get_cable_length_m88;
	phy->ops.get_cfg_done	= e1000_get_cfg_done_generic;
	phy->ops.read_reg	= e1000_read_phy_reg_m88;
	phy->ops.reset		= e1000_phy_hw_reset_generic;
	phy->ops.write_reg	= e1000_write_phy_reg_m88;
	phy->ops.get_info	= e1000_get_phy_info_m88;
	phy->ops.power_up	= e1000_power_up_phy_copper;
	phy->ops.power_down	= e1000_power_down_phy_copper_82540;

	ret_val = e1000_get_phy_id(hw);
	if (ret_val) {
		debug_uk_pr_info("e1000_get_phy_id = %d\n", ret_val);
		return ret_val;
	}

	/* Verify phy id */
	if (phy->id == M88E1011_I_PHY_ID) {
		uk_pr_info("phy->id == M88E1011_I_PHY_ID, ret_val = %d\n", ret_val);
		return ret_val;
	}
	
	debug_uk_pr_info("e1000_init_phy_params_82540 returning -E1000_ERR_PHY\n");
	return -E1000_ERR_PHY;
}

/**
 * e1000_init_nvm_params_82540 - Init NVM func ptrs.
 * @hw: pointer to the HW structure
 **/
static __s32 e1000_init_nvm_params_82540(struct e1000_hw *hw)
{
	struct e1000_nvm_info *nvm = &hw->nvm;
	__u32 eecd;
	
	debug_uk_pr_info("e1000_init_nvm_params_82540\n");
	eecd = E1000_READ_REG(hw, E1000_EECD);

	nvm->type = e1000_nvm_eeprom_microwire;
	nvm->delay_usec = 50;
	nvm->opcode_bits = 3;
	switch (nvm->override) {
	case e1000_nvm_override_microwire_large:
		nvm->address_bits = 8;
		nvm->word_size = 256;
		break;
	case e1000_nvm_override_microwire_small:
		nvm->address_bits = 6;
		nvm->word_size = 64;
		break;
	default:
		nvm->address_bits = eecd & E1000_EECD_SIZE ? 8 : 6;
		nvm->word_size = eecd & E1000_EECD_SIZE ? 256 : 64;
		break;
	}

	/* Function Pointers */
	nvm->ops.acquire	= e1000_acquire_nvm_generic;
	nvm->ops.read		= e1000_read_nvm_microwire;
	nvm->ops.release	= e1000_release_nvm_generic;
	nvm->ops.update		= e1000_update_nvm_checksum_generic;
	nvm->ops.valid_led_default = e1000_valid_led_default_generic;
	nvm->ops.validate	= e1000_validate_nvm_checksum_generic;
	nvm->ops.write		= e1000_write_nvm_microwire;

	return E1000_SUCCESS;
}

/**
 * e1000_init_mac_params_82540 - Init MAC func ptrs.
 * @hw: pointer to the HW structure
 **/
static __s32 e1000_init_mac_params_82540(struct e1000_hw *hw)
{
	struct e1000_mac_info *mac = &hw->mac;
	__s32 ret_val = E1000_SUCCESS;

	debug_uk_pr_info("e1000_init_mac_params_82540\n");

	/* Set media type */
	hw->phy.media_type = e1000_media_type_copper;

	/* Set mta register count */
	mac->mta_reg_count = 128;
	/* Set rar entry count */
	mac->rar_entry_count = E1000_RAR_ENTRIES;

	/* Function pointers */

	/* bus type/speed/width */
	mac->ops.get_bus_info = e1000_get_bus_info_pci_generic;
	/* function id */
	mac->ops.set_lan_id = e1000_set_lan_id_multi_port_pci;
	/* reset */
	mac->ops.reset_hw = e1000_reset_hw_82540;
	/* hw initialization */
	mac->ops.init_hw = e1000_init_hw_82540;
	/* link setup */
	mac->ops.setup_link = e1000_setup_link_generic;
	/* physical interface setup */
	mac->ops.setup_physical_interface =
		(hw->phy.media_type == e1000_media_type_copper)
			? e1000_setup_copper_link_82540
			: e1000_setup_fiber_serdes_link_82540;
	/* check for link */
	switch (hw->phy.media_type) {
	case e1000_media_type_copper:
		// TODO: should be only this
		mac->ops.check_for_link = e1000_check_for_copper_link_generic;
		break;
	case e1000_media_type_fiber:
		mac->ops.check_for_link = e1000_check_for_fiber_link_generic;
		break;
	case e1000_media_type_internal_serdes:
		mac->ops.check_for_link = e1000_check_for_serdes_link_generic;
		break;
	default:
		uk_pr_crit("ERRORRRRRR!!!!!!\n");
		ret_val = -E1000_ERR_CONFIG;
		goto out;
		break;
	}
	/* link info */
	mac->ops.get_link_up_info =
		(hw->phy.media_type == e1000_media_type_copper)
			? e1000_get_speed_and_duplex_copper_generic
			: e1000_get_speed_and_duplex_fiber_serdes_generic;
	/* multicast address update */
	mac->ops.update_mc_addr_list = e1000_update_mc_addr_list_generic;
	/* read mac address */
	mac->ops.read_mac_addr = e1000_read_mac_addr_82540;
	/* ID LED init */
	mac->ops.id_led_init = e1000_id_led_init_generic;
	/* setup LED */
	mac->ops.setup_led = e1000_setup_led_generic;
	/* cleanup LED */
	mac->ops.cleanup_led = e1000_cleanup_led_generic;
	/* turn on/off LED */
	mac->ops.led_on = e1000_led_on_generic;
	mac->ops.led_off = e1000_led_off_generic;
	/* clear hardware counters */
	mac->ops.clear_hw_cntrs = e1000_clear_hw_cntrs_82540;

out:
	return ret_val;
}

/**
 * e1000_init_function_pointers_82540 - Init func ptrs.
 * @hw: pointer to the HW structure
 *
 * Called to initialize all function pointers and parameters.
 **/
void e1000_init_function_pointers_82540(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_init_function_pointers_82540\n");

	hw->mac.ops.init_params = e1000_init_mac_params_82540;
	hw->nvm.ops.init_params = e1000_init_nvm_params_82540;
	hw->phy.ops.init_params = e1000_init_phy_params_82540;
}

/**
 *  e1000_reset_hw_82540 - Reset hardware
 *  @hw: pointer to the HW structure
 *
 *  This resets the hardware into a known state.
 **/
static __s32 e1000_reset_hw_82540(struct e1000_hw *hw)
{
	__u32 ctrl, manc;
	__s32 ret_val = E1000_SUCCESS;

	debug_uk_pr_info("e1000_reset_hw_82540\n");

	uk_pr_debug("Masking off all interrupts\n");
	E1000_WRITE_REG(hw, E1000_IMC, 0xFFFFFFFF);

	E1000_WRITE_REG(hw, E1000_RCTL, 0);
	E1000_WRITE_REG(hw, E1000_TCTL, E1000_TCTL_PSP);
	E1000_WRITE_FLUSH(hw);

	/*
	 * Delay to allow any outstanding PCI transactions to complete
	 * before resetting the device.
	 */
	msec_delay(10);

	ctrl = E1000_READ_REG(hw, E1000_CTRL);

	uk_pr_debug("Issuing a global reset to 82540/82545/82546 MAC\n");
	E1000_WRITE_REG(hw, E1000_CTRL_DUP, ctrl | E1000_CTRL_RST);


	/* Wait for EEPROM reload */
	msec_delay(5);

	/* Disable HW ARPs on ASF enabled adapters */
	manc = E1000_READ_REG(hw, E1000_MANC);
	manc &= ~E1000_MANC_ARP_EN;
	E1000_WRITE_REG(hw, E1000_MANC, manc);

	E1000_WRITE_REG(hw, E1000_IMC, 0xffffffff);
	E1000_READ_REG(hw, E1000_ICR);

	return ret_val;
}

/**
 *  e1000_init_hw_82540 - Initialize hardware
 *  @hw: pointer to the HW structure
 *
 *  This inits the hardware readying it for operation.
 **/
static __s32 e1000_init_hw_82540(struct e1000_hw *hw)
{
	struct e1000_mac_info *mac = &hw->mac;
	__u32 txdctl;
	__s32 ret_val;
	__u16 i;

	debug_uk_pr_info("e1000_init_hw_82540\n");

	/* Initialize identification LED */
	ret_val = mac->ops.id_led_init(hw);
	if (ret_val) {
		uk_pr_debug("Error initializing identification LED\n");
		/* This is not fatal and we should not stop init due to this */
	}

	/* Disabling VLAN filtering */
	uk_pr_debug("Initializing the IEEE VLAN\n");
	if (mac->type < e1000_82545_rev_3)
		E1000_WRITE_REG(hw, E1000_VET, 0);

	/* Setup the receive address. */
	e1000_init_rx_addrs_generic(hw, mac->rar_entry_count);

	/* Zero out the Multicast HASH table */
	uk_pr_debug("Zeroing the MTA\n");
	for (i = 0; i < mac->mta_reg_count; i++) {
		E1000_WRITE_REG_ARRAY(hw, E1000_MTA, i, 0);
		/*
		 * Avoid back to back register writes by adding the register
		 * read (flush).  This is to protect against some strange
		 * bridge configurations that may issue Memory Write Block
		 * (MWB) to our register space.  The *_rev_3 hardware at
		 * least doesn't respond correctly to every other dword in an
		 * MWB to our register space.
		 */
		E1000_WRITE_FLUSH(hw);
	}

	if (mac->type < e1000_82545_rev_3)
		e1000_pcix_mmrbc_workaround_generic(hw);

	/* Setup link and flow control */
	ret_val = mac->ops.setup_link(hw);

	txdctl = E1000_READ_REG(hw, E1000_TXDCTL(0));
	txdctl = (txdctl & ~E1000_TXDCTL_WTHRESH) |
		  E1000_TXDCTL_FULL_TX_DESC_WB;
	E1000_WRITE_REG(hw, E1000_TXDCTL(0), txdctl);

	/*
	 * Clear all of the statistics registers (clear on read).  It is
	 * important that we do this after we have tried to establish link
	 * because the symbol error count will increment wildly if there
	 * is no link.
	 */
	e1000_clear_hw_cntrs_82540(hw);

	return ret_val;
}

/**
 *  e1000_setup_copper_link_82540 - Configure copper link settings
 *  @hw: pointer to the HW structure
 *
 *  Calls the appropriate function to configure the link for auto-neg or forced
 *  speed and duplex.  Then we check for link, once link is established calls
 *  to configure collision distance and flow control are called.  If link is
 *  not established, we return -E1000_ERR_PHY (-2).
 **/
static __s32 e1000_setup_copper_link_82540(struct e1000_hw *hw)
{
	__u32 ctrl;
	__s32 ret_val;
	__u16 data;

	uk_pr_debug("e1000_setup_copper_link_82540\n");

	ctrl = E1000_READ_REG(hw, E1000_CTRL);
	ctrl |= E1000_CTRL_SLU;
	ctrl &= ~(E1000_CTRL_FRCSPD | E1000_CTRL_FRCDPX);
	E1000_WRITE_REG(hw, E1000_CTRL, ctrl);

	ret_val = e1000_set_phy_mode_82540(hw);
	if (ret_val)
		goto out;

	if (hw->mac.type == e1000_82545_rev_3) {
		ret_val = hw->phy.ops.read_reg(hw, M88E1000_PHY_SPEC_CTRL,
					       &data);
		if (ret_val)
			goto out;
		data |= 0x00000008;
		ret_val = hw->phy.ops.write_reg(hw, M88E1000_PHY_SPEC_CTRL,
						data);
		if (ret_val)
			goto out;
	}

	ret_val = e1000_copper_link_setup_m88(hw);
	if (ret_val)
		goto out;

	ret_val = e1000_setup_copper_link_generic(hw);

out:
	return ret_val;
}

/**
 *  e1000_setup_fiber_serdes_link_82540 - Setup link for fiber/serdes
 *  @hw: pointer to the HW structure
 *
 *  Set the output amplitude to the value in the EEPROM and adjust the VCO
 *  speed to improve Bit Error Rate (BER) performance.  Configures collision
 *  distance and flow control for fiber and serdes links.  Upon successful
 *  setup, poll for link.
 **/
static __s32 e1000_setup_fiber_serdes_link_82540(struct e1000_hw *hw)
{
	__s32 ret_val = E1000_SUCCESS;

	uk_pr_debug("e1000_setup_fiber_serdes_link_82540\n");

	if (hw->phy.media_type == e1000_media_type_internal_serdes) {
		/*
			* If we're on serdes media, adjust the output
			* amplitude to value set in the EEPROM.
			*/
		ret_val = e1000_adjust_serdes_amplitude_82540(hw);
		if (ret_val)
			goto out;
	}
	/* Adjust VCO speed to improve BER performance */
	ret_val = e1000_set_vco_speed_82540(hw);
	if (ret_val)
		goto out;


	ret_val = e1000_setup_fiber_serdes_link_generic(hw);

out:
	return ret_val;
}

/**
 *  e1000_adjust_serdes_amplitude_82540 - Adjust amplitude based on EEPROM
 *  @hw: pointer to the HW structure
 *
 *  Adjust the SERDES output amplitude based on the EEPROM settings.
 **/
static __s32 e1000_adjust_serdes_amplitude_82540(struct e1000_hw *hw)
{
	__s32 ret_val;
	__u16 nvm_data;

	debug_uk_pr_info("e1000_adjust_serdes_amplitude_82540\n");

	ret_val = hw->nvm.ops.read(hw, NVM_SERDES_AMPLITUDE, 1, &nvm_data);
	if (ret_val)
		goto out;

	if (nvm_data != NVM_RESERVED_WORD) {
		/* Adjust serdes output amplitude only. */
		nvm_data &= NVM_SERDES_AMPLITUDE_MASK;
		ret_val = hw->phy.ops.write_reg(hw, M88E1000_PHY_EXT_CTRL,
						nvm_data);
		if (ret_val)
			goto out;
	}

out:
	return ret_val;
}

/**
 *  e1000_set_vco_speed_82540 - Set VCO speed for better performance
 *  @hw: pointer to the HW structure
 *
 *  Set the VCO speed to improve Bit Error Rate (BER) performance.
 **/
static __s32 e1000_set_vco_speed_82540(struct e1000_hw *hw)
{
	__s32  ret_val;
	__u16 default_page = 0;
	__u16 phy_data;

	debug_uk_pr_info("e1000_set_vco_speed_82540\n");

	/* Set PHY register 30, page 5, bit 8 to 0 */

	debug_uk_pr_info("e1000_set_vco_speed_82540 read_reg\n");
	ret_val = hw->phy.ops.read_reg(hw, M88E1000_PHY_PAGE_SELECT,
				       &default_page);
	if (ret_val)
		goto out;

	debug_uk_pr_info("e1000_set_vco_speed_82540 write_reg\n");
	ret_val = hw->phy.ops.write_reg(hw, M88E1000_PHY_PAGE_SELECT, 0x0005);
	if (ret_val)
		goto out;

	debug_uk_pr_info("e1000_set_vco_speed_82540 read_reg\n");
	ret_val = hw->phy.ops.read_reg(hw, M88E1000_PHY_GEN_CONTROL, &phy_data);
	if (ret_val)
		goto out;

	phy_data &= ~M88E1000_PHY_VCO_REG_BIT8;
	debug_uk_pr_info("e1000_set_vco_speed_82540 write_reg\n");
	ret_val = hw->phy.ops.write_reg(hw, M88E1000_PHY_GEN_CONTROL, phy_data);
	if (ret_val)
		goto out;

	/* Set PHY register 30, page 4, bit 11 to 1 */

	debug_uk_pr_info("e1000_set_vco_speed_82540 write_reg\n");
	ret_val = hw->phy.ops.write_reg(hw, M88E1000_PHY_PAGE_SELECT, 0x0004);
	if (ret_val)
		goto out;

	debug_uk_pr_info("e1000_set_vco_speed_82540 read_reg\n");
	ret_val = hw->phy.ops.read_reg(hw, M88E1000_PHY_GEN_CONTROL, &phy_data);
	if (ret_val)
		goto out;

	phy_data |= M88E1000_PHY_VCO_REG_BIT11;
	debug_uk_pr_info("e1000_set_vco_speed_82540 write_reg\n");
	ret_val = hw->phy.ops.write_reg(hw, M88E1000_PHY_GEN_CONTROL, phy_data);
	if (ret_val)
		goto out;

	debug_uk_pr_info("e1000_set_vco_speed_82540 write_reg\n");
	ret_val = hw->phy.ops.write_reg(hw, M88E1000_PHY_PAGE_SELECT,
					default_page);

out:
	debug_uk_pr_info("e1000_set_vco_speed_82540 out\n");
	return ret_val;
}

/**
 *  e1000_set_phy_mode_82540 - Set PHY to class A mode
 *  @hw: pointer to the HW structure
 *
 *  Sets the PHY to class A mode and assumes the following operations will
 *  follow to enable the new class mode:
 *    1.  Do a PHY soft reset.
 *    2.  Restart auto-negotiation or force link.
 **/
static __s32 e1000_set_phy_mode_82540(struct e1000_hw *hw)
{
	__s32 ret_val = E1000_SUCCESS;
	__u16 nvm_data;

	debug_uk_pr_info("e1000_set_phy_mode_82540\n");

	if (hw->mac.type != e1000_82545_rev_3)
		goto out;

	ret_val = hw->nvm.ops.read(hw, NVM_PHY_CLASS_WORD, 1, &nvm_data);
	if (ret_val) {
		ret_val = -E1000_ERR_PHY;
		goto out;
	}

	if ((nvm_data != NVM_RESERVED_WORD) && (nvm_data & NVM_PHY_CLASS_A)) {
		ret_val = hw->phy.ops.write_reg(hw, M88E1000_PHY_PAGE_SELECT,
						0x000B);
		if (ret_val) {
			ret_val = -E1000_ERR_PHY;
			goto out;
		}
		ret_val = hw->phy.ops.write_reg(hw, M88E1000_PHY_GEN_CONTROL,
						0x8104);
		if (ret_val) {
			ret_val = -E1000_ERR_PHY;
			goto out;
		}

	}

out:
	return ret_val;
}

/**
 * e1000_power_down_phy_copper_82540 - Remove link in case of PHY power down
 * @hw: pointer to the HW structure
 *
 * In the case of a PHY power down to save power, or to turn off link during a
 * driver unload, or wake on lan is not enabled, remove the link.
 **/
static void e1000_power_down_phy_copper_82540(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_power_down_phy_copper_82540\n");

	/* If the management interface is not enabled, then power down */
	if (!(E1000_READ_REG(hw, E1000_MANC) & E1000_MANC_SMBUS_EN))
		e1000_power_down_phy_copper(hw);

	return;
}

/**
 *  e1000_clear_hw_cntrs_82540 - Clear device specific hardware counters
 *  @hw: pointer to the HW structure
 *
 *  Clears the hardware counters by reading the counter registers.
 **/
static void e1000_clear_hw_cntrs_82540(struct e1000_hw *hw)
{
	debug_uk_pr_info("e1000_clear_hw_cntrs_82540\n");

	e1000_clear_hw_cntrs_base_generic(hw);

	E1000_READ_REG(hw, E1000_PRC64);
	E1000_READ_REG(hw, E1000_PRC127);
	E1000_READ_REG(hw, E1000_PRC255);
	E1000_READ_REG(hw, E1000_PRC511);
	E1000_READ_REG(hw, E1000_PRC1023);
	E1000_READ_REG(hw, E1000_PRC1522);
	E1000_READ_REG(hw, E1000_PTC64);
	E1000_READ_REG(hw, E1000_PTC127);
	E1000_READ_REG(hw, E1000_PTC255);
	E1000_READ_REG(hw, E1000_PTC511);
	E1000_READ_REG(hw, E1000_PTC1023);
	E1000_READ_REG(hw, E1000_PTC1522);

	E1000_READ_REG(hw, E1000_ALGNERRC);
	E1000_READ_REG(hw, E1000_RXERRC);
	E1000_READ_REG(hw, E1000_TNCRS);
	E1000_READ_REG(hw, E1000_CEXTERR);
	E1000_READ_REG(hw, E1000_TSCTC);
	E1000_READ_REG(hw, E1000_TSCTFC);

	E1000_READ_REG(hw, E1000_MGTPRC);
	E1000_READ_REG(hw, E1000_MGTPDC);
	E1000_READ_REG(hw, E1000_MGTPTC);
}

/**
 *  e1000_read_mac_addr_82540 - Read device MAC address
 *  @hw: pointer to the HW structure
 *
 *  Reads the device MAC address from the EEPROM and stores the value.
 *  Since devices with two ports use the same EEPROM, we increment the
 *  last bit in the MAC address for the second port.
 *
 *  This version is being used over generic because of customer issues
 *  with VmWare and Virtual Box when using generic. It seems in
 *  the emulated 82545, RAR[0] does NOT have a valid address after a
 *  reset, this older method works and using this breaks nothing for
 *  these legacy adapters.
 **/
__s32 e1000_read_mac_addr_82540(struct e1000_hw *hw)
{
	__s32  ret_val = E1000_SUCCESS;
	__u16 offset, nvm_data, i;

	debug_uk_pr_info("e1000_read_mac_addr_82540\n");

	for (i = 0; i < ETH_ADDR_LEN; i += 2) {
		offset = i >> 1;
		ret_val = hw->nvm.ops.read(hw, offset, 1, &nvm_data);
		if (ret_val) {
			uk_pr_crit("NVM Read Error\n");
			goto out;
		}
		hw->mac.perm_addr[i] = (__u8)(nvm_data & 0xFF);
		hw->mac.perm_addr[i+1] = (__u8)(nvm_data >> 8);
	}

	/* Flip last bit of mac address if we're on second port */
	if (hw->bus.func == E1000_FUNC_1)
		hw->mac.perm_addr[5] ^= 1;

	for (i = 0; i < ETH_ADDR_LEN; i++) {
		hw->mac.addr[i] = hw->mac.perm_addr[i];
		uk_pr_crit("addr[%d] = %X\n", i, hw->mac.addr[i]);
	}

out:
	return ret_val;
}
