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

#ifndef _E1000_HW_H_
#define _E1000_HW_H_

#include <pci/pci_bus.h>
#include <uk/netdev_core.h>

#include "e1000_osdep.h"
#include "e1000_regs.h"
#include "e1000_defines.h"
#include "e1000_hw.h"

struct e1000_hw;

#define E1000_DEV_ID_82545EM_COPPER		0x100F
#define E1000_DEV_ID_82545EM_FIBER		0x1011

#define E1000_REVISION_0	0
#define E1000_REVISION_1	1
#define E1000_REVISION_2	2
#define E1000_REVISION_3	3
#define E1000_REVISION_4	4

#define E1000_FUNC_0		0
#define E1000_FUNC_1		1
#define E1000_FUNC_2		2
#define E1000_FUNC_3		3

#define E1000_ALT_MAC_ADDRESS_OFFSET_LAN0	0
#define E1000_ALT_MAC_ADDRESS_OFFSET_LAN1	3
#define E1000_ALT_MAC_ADDRESS_OFFSET_LAN2	6
#define E1000_ALT_MAC_ADDRESS_OFFSET_LAN3	9

enum e1000_mac_type {
	e1000_82545,
	e1000_82545_rev_3,
};

enum e1000_media_type {
	e1000_media_type_unknown = 0,
	e1000_media_type_copper = 1,
	e1000_media_type_fiber = 2,
	e1000_media_type_internal_serdes = 3,
	e1000_num_media_types
};

enum e1000_nvm_type {
	e1000_nvm_unknown = 0,
	e1000_nvm_none,
	e1000_nvm_eeprom_spi,
	e1000_nvm_eeprom_microwire,
	e1000_nvm_flash_hw,
	e1000_nvm_invm,
	e1000_nvm_flash_sw
};

enum e1000_nvm_override {
	e1000_nvm_override_none = 0,
	e1000_nvm_override_spi_small,
	e1000_nvm_override_spi_large,
	e1000_nvm_override_microwire_small,
	e1000_nvm_override_microwire_large
};

enum e1000_phy_type {
	e1000_phy_unknown = 0,
	e1000_phy_none,
	e1000_phy_m88,
	e1000_phy_igp,
	e1000_phy_igp_2,
	e1000_phy_gg82563,
	e1000_phy_igp_3,
	e1000_phy_ife,
	e1000_phy_bm,
	e1000_phy_82578,
	e1000_phy_82577,
	e1000_phy_82579,
	e1000_phy_i217,
	e1000_phy_82580,
	e1000_phy_vf,
	e1000_phy_i210,
};

enum e1000_bus_type {
	e1000_bus_type_unknown = 0,
	e1000_bus_type_pci,
	e1000_bus_type_pcix,
	e1000_bus_type_pci_express,
	e1000_bus_type_reserved
};

enum e1000_bus_speed {
	e1000_bus_speed_unknown = 0,
	e1000_bus_speed_33,
	e1000_bus_speed_66,
	e1000_bus_speed_100,
	e1000_bus_speed_120,
	e1000_bus_speed_133,
	e1000_bus_speed_2500,
	e1000_bus_speed_5000,
	e1000_bus_speed_reserved
};

enum e1000_bus_width {
	e1000_bus_width_unknown = 0,
	e1000_bus_width_pcie_x1,
	e1000_bus_width_pcie_x2,
	e1000_bus_width_pcie_x4 = 4,
	e1000_bus_width_pcie_x8 = 8,
	e1000_bus_width_32,
	e1000_bus_width_64,
	e1000_bus_width_reserved
};

enum e1000_1000t_rx_status {
	e1000_1000t_rx_status_not_ok = 0,
	e1000_1000t_rx_status_ok,
	e1000_1000t_rx_status_undefined = 0xFF
};

enum e1000_rev_polarity {
	e1000_rev_polarity_normal = 0,
	e1000_rev_polarity_reversed,
	e1000_rev_polarity_undefined = 0xFF
};

enum e1000_fc_mode {
	e1000_fc_none = 0,
	e1000_fc_rx_pause,
	e1000_fc_tx_pause,
	e1000_fc_full,
	e1000_fc_default = 0xFF
};

enum e1000_ffe_config {
	e1000_ffe_config_enabled = 0,
	e1000_ffe_config_active,
	e1000_ffe_config_blocked
};

enum e1000_dsp_config {
	e1000_dsp_config_disabled = 0,
	e1000_dsp_config_enabled,
	e1000_dsp_config_activated,
	e1000_dsp_config_undefined = 0xFF
};

enum e1000_ms_type {
	e1000_ms_hw_default = 0,
	e1000_ms_force_master,
	e1000_ms_force_slave,
	e1000_ms_auto
};

enum e1000_smart_speed {
	e1000_smart_speed_default = 0,
	e1000_smart_speed_on,
	e1000_smart_speed_off
};

enum e1000_serdes_link_state {
	e1000_serdes_link_down = 0,
	e1000_serdes_link_autoneg_progress,
	e1000_serdes_link_autoneg_complete,
	e1000_serdes_link_forced_up
};

#define __le16 __u16
#define __le32 __u32
#define __le64 __u64
/* Receive Descriptor */
struct e1000_rx_desc {
	__le64 buffer_addr; /* Address of the descriptor's data buffer */
	__le16 length;      /* Length of data DMAed into data buffer */
	__le16 csum; /* Packet checksum */
	__u8  status;  /* Descriptor status */
	__u8  errors;  /* Descriptor Errors */
	__le16 special;
};

/* Receive Descriptor - Extended */
union e1000_rx_desc_extended {
	struct {
		__le64 buffer_addr;
		__le64 reserved;
	} read;
	struct {
		struct {
			__le32 mrq; /* Multiple Rx Queues */
			union {
				__le32 rss; /* RSS Hash */
				struct {
					__le16 ip_id;  /* IP id */
					__le16 csum;   /* Packet Checksum */
				} csum_ip;
			} hi_dword;
		} lower;
		struct {
			__le32 status_error;  /* ext status/error */
			__le16 length;
			__le16 vlan; /* VLAN tag */
		} upper;
	} wb;  /* writeback */
};

#define MAX_PS_BUFFERS 4

/* Number of packet split data buffers (not including the header buffer) */
#define PS_PAGE_BUFFERS	(MAX_PS_BUFFERS - 1)

/* Receive Descriptor - Packet Split */
union e1000_rx_desc_packet_split {
	struct {
		/* one buffer for protocol header(s), three data buffers */
		__le64 buffer_addr[MAX_PS_BUFFERS];
	} read;
	struct {
		struct {
			__le32 mrq;  /* Multiple Rx Queues */
			union {
				__le32 rss; /* RSS Hash */
				struct {
					__le16 ip_id;    /* IP id */
					__le16 csum;     /* Packet Checksum */
				} csum_ip;
			} hi_dword;
		} lower;
		struct {
			__le32 status_error;  /* ext status/error */
			__le16 length0;  /* length of buffer 0 */
			__le16 vlan;  /* VLAN tag */
		} middle;
		struct {
			__le16 header_status;
			/* length of buffers 1-3 */
			__le16 length[PS_PAGE_BUFFERS];
		} upper;
		__le64 reserved;
	} wb; /* writeback */
};

/* Transmit Descriptor */
struct e1000_tx_desc {
	__le64 buffer_addr;   /* Address of the descriptor's data buffer */
	union {
		__le32 data;
		struct {
			__le16 length;  /* Data buffer length */
			__u8 cso;  /* Checksum offset */
			__u8 cmd;  /* Descriptor control */
		} flags;
	} lower;
	union {
		__le32 data;
		struct {
			__u8 status; /* Descriptor status */
			__u8 css;  /* Checksum start */
			__le16 special;
		} fields;
	} upper;
};

/* Offload Context Descriptor */
struct e1000_context_desc {
	union {
		__le32 ip_config;
		struct {
			__u8 ipcss;  /* IP checksum start */
			__u8 ipcso;  /* IP checksum offset */
			__le16 ipcse;  /* IP checksum end */
		} ip_fields;
	} lower_setup;
	union {
		__le32 tcp_config;
		struct {
			__u8 tucss;  /* TCP checksum start */
			__u8 tucso;  /* TCP checksum offset */
			__le16 tucse;  /* TCP checksum end */
		} tcp_fields;
	} upper_setup;
	__le32 cmd_and_length;
	union {
		__le32 data;
		struct {
			__u8 status;  /* Descriptor status */
			__u8 hdr_len;  /* Header length */
			__le16 mss;  /* Maximum segment size */
		} fields;
	} tcp_seg_setup;
};

/* Offload data descriptor */
struct e1000_data_desc {
	__le64 buffer_addr;  /* Address of the descriptor's buffer address */
	union {
		__le32 data;
		struct {
			__le16 length;  /* Data buffer length */
			__u8 typ_len_ext;
			__u8 cmd;
		} flags;
	} lower;
	union {
		__le32 data;
		struct {
			__u8 status;  /* Descriptor status */
			__u8 popts;  /* Packet Options */
			__le16 special;
		} fields;
	} upper;
};

struct e1000_host_mng_dhcp_cookie {
	__u32 signature;
	__u8  status;
	__u8  reserved0;
	__u16 vlan_id;
	__u32 reserved1;
	__u16 reserved2;
	__u8  reserved3;
	__u8  checksum;
};

/* Host Interface "Rev 1" */
struct e1000_host_command_header {
	__u8 command_id;
	__u8 command_length;
	__u8 command_options;
	__u8 checksum;
};

#define E1000_HI_MAX_DATA_LENGTH	252
struct e1000_host_command_info {
	struct e1000_host_command_header command_header;
	__u8 command_data[E1000_HI_MAX_DATA_LENGTH];
};

/* Host Interface "Rev 2" */
struct e1000_host_mng_command_header {
	__u8  command_id;
	__u8  checksum;
	__u16 reserved1;
	__u16 reserved2;
	__u16 command_length;
};

#define E1000_HI_MAX_MNG_DATA_LENGTH	0x6F8
struct e1000_host_mng_command_info {
	struct e1000_host_mng_command_header command_header;
	__u8 command_data[E1000_HI_MAX_MNG_DATA_LENGTH];
};

#include "e1000_mac.h"
#include "e1000_phy.h"
#include "e1000_nvm.h"
#include "e1000_manage.h"
#include "e1000_mbx.h"

/* Function pointers for the MAC. */
struct e1000_mac_operations {
	__s32  (*init_params)(struct e1000_hw *);
	__s32  (*id_led_init)(struct e1000_hw *);
	__s32  (*blink_led)(struct e1000_hw *);
	bool (*check_mng_mode)(struct e1000_hw *);
	__s32  (*check_for_link)(struct e1000_hw *);
	__s32  (*cleanup_led)(struct e1000_hw *);
	void (*clear_hw_cntrs)(struct e1000_hw *);
	void (*clear_vfta)(struct e1000_hw *);
	__s32  (*get_bus_info)(struct e1000_hw *);
	void (*set_lan_id)(struct e1000_hw *);
	__s32  (*get_link_up_info)(struct e1000_hw *, __u16 *, __u16 *);
	__s32  (*led_on)(struct e1000_hw *);
	__s32  (*led_off)(struct e1000_hw *);
	void (*update_mc_addr_list)(struct e1000_hw *, __u8 *, __u32);
	__s32  (*reset_hw)(struct e1000_hw *);
	__s32  (*init_hw)(struct e1000_hw *);
	void (*shutdown_serdes)(struct e1000_hw *);
	void (*power_up_serdes)(struct e1000_hw *);
	__s32  (*setup_link)(struct e1000_hw *);
	__s32  (*setup_physical_interface)(struct e1000_hw *);
	__s32  (*setup_led)(struct e1000_hw *);
	void (*write_vfta)(struct e1000_hw *, __u32, __u32);
	void (*config_collision_dist)(struct e1000_hw *);
	int  (*rar_set)(struct e1000_hw *, __u8*, __u32);
	__s32  (*read_mac_addr)(struct e1000_hw *);
	__s32  (*validate_mdi_setting)(struct e1000_hw *);
	__s32  (*acquire_swfw_sync)(struct e1000_hw *, __u16);
	void (*release_swfw_sync)(struct e1000_hw *, __u16);
};

/* When to use various PHY register access functions:
 *
 *                 Func   Caller
 *   Function      Does   Does    When to use
 *   ~~~~~~~~~~~~  ~~~~~  ~~~~~~  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   X_reg         L,P,A  n/a     for simple PHY reg accesses
 *   X_reg_locked  P,A    L       for multiple accesses of different regs
 *                                on different pages
 *   X_reg_page    A      L,P     for multiple accesses of different regs
 *                                on the same page
 *
 * Where X=[read|write], L=locking, P=sets page, A=register access
 *
 */
struct e1000_phy_operations {
	__s32  (*init_params)(struct e1000_hw *);
	__s32  (*acquire)(struct e1000_hw *);
	__s32  (*cfg_on_link_up)(struct e1000_hw *);
	__s32  (*check_polarity)(struct e1000_hw *);
	__s32  (*check_reset_block)(struct e1000_hw *);
	__s32  (*commit)(struct e1000_hw *);
	__s32  (*force_speed_duplex)(struct e1000_hw *);
	__s32  (*get_cfg_done)(struct e1000_hw *hw);
	__s32  (*get_cable_length)(struct e1000_hw *);
	__s32  (*get_info)(struct e1000_hw *);
	__s32  (*set_page)(struct e1000_hw *, __u16);
	__s32  (*read_reg)(struct e1000_hw *, __u32, __u16 *);
	__s32  (*read_reg_locked)(struct e1000_hw *, __u32, __u16 *);
	__s32  (*read_reg_page)(struct e1000_hw *, __u32, __u16 *);
	void (*release)(struct e1000_hw *);
	__s32  (*reset)(struct e1000_hw *);
	__s32  (*set_d0_lplu_state)(struct e1000_hw *, bool);
	__s32  (*set_d3_lplu_state)(struct e1000_hw *, bool);
	__s32  (*write_reg)(struct e1000_hw *, __u32, __u16);
	__s32  (*write_reg_locked)(struct e1000_hw *, __u32, __u16);
	__s32  (*write_reg_page)(struct e1000_hw *, __u32, __u16);
	void (*power_up)(struct e1000_hw *);
	void (*power_down)(struct e1000_hw *);
	__s32 (*read_i2c_byte)(struct e1000_hw *, __u8, __u8, __u8 *);
	__s32 (*write_i2c_byte)(struct e1000_hw *, __u8, __u8, __u8);
};

/* Function pointers for the NVM. */
struct e1000_nvm_operations {
	__s32  (*init_params)(struct e1000_hw *);
	__s32  (*acquire)(struct e1000_hw *);
	__s32  (*read)(struct e1000_hw *, __u16, __u16, __u16 *);
	void (*release)(struct e1000_hw *);
	void (*reload)(struct e1000_hw *);
	__s32  (*update)(struct e1000_hw *);
	__s32  (*valid_led_default)(struct e1000_hw *, __u16 *);
	__s32  (*validate)(struct e1000_hw *);
	__s32  (*write)(struct e1000_hw *, __u16, __u16, __u16 *);
};

struct e1000_mac_info {
	struct e1000_mac_operations ops;
	__u8 addr[UK_ETH_ADDR_LEN];
	__u8 perm_addr[UK_ETH_ADDR_LEN];

	enum e1000_mac_type type;

	__u32 collision_delta;
	__u32 ledctl_default;
	__u32 ledctl_mode1;
	__u32 ledctl_mode2;
	__u32 mc_filter_type;
	__u32 tx_packet_delta;
	__u32 txcw;

	__u16 current_ifs_val;
	__u16 ifs_max_val;
	__u16 ifs_min_val;
	__u16 ifs_ratio;
	__u16 ifs_step_size;
	__u16 mta_reg_count;
	__u16 uta_reg_count;

	/* Maximum size of the MTA register table in all supported adapters */
#define MAX_MTA_REG 128
	__u32 mta_shadow[MAX_MTA_REG];
	__u16 rar_entry_count;

	__u8  forced_speed_duplex;

	bool adaptive_ifs;
	bool has_fwsm;
	bool arc_subsystem_valid;
	bool asf_firmware_present;
	bool autoneg;
	bool autoneg_failed;
	bool get_link_status;
	bool in_ifs_mode;
	bool report_tx_early;
	enum e1000_serdes_link_state serdes_link_state;
	bool serdes_has_link;
	bool tx_pkt_filtering;
};

struct e1000_phy_info {
	struct e1000_phy_operations ops;
	enum e1000_phy_type type;

	enum e1000_1000t_rx_status local_rx;
	enum e1000_1000t_rx_status remote_rx;
	enum e1000_ms_type ms_type;
	enum e1000_ms_type original_ms_type;
	enum e1000_rev_polarity cable_polarity;
	enum e1000_smart_speed smart_speed;

	__u32 addr;
	__u32 id;
	__u32 reset_delay_us; /* in usec */
	__u32 revision;

	enum e1000_media_type media_type;

	__u16 autoneg_advertised;
	__u16 autoneg_mask;
	__u16 cable_length;
	__u16 max_cable_length;
	__u16 min_cable_length;

	__u8 mdix;

	bool disable_polarity_correction;
	bool is_mdix;
	bool polarity_correction;
	bool speed_downgraded;
	bool autoneg_wait_to_complete;
};

struct e1000_nvm_info {
	struct e1000_nvm_operations ops;
	enum e1000_nvm_type type;
	enum e1000_nvm_override override;

	__u32 flash_bank_size;
	__u32 flash_base_addr;

	__u16 word_size;
	__u16 delay_usec;
	__u16 address_bits;
	__u16 opcode_bits;
	__u16 page_size;
};

struct e1000_bus_info {
	enum e1000_bus_type type;
	enum e1000_bus_speed speed;
	enum e1000_bus_width width;

	__u16 func;
	__u16 pci_cmd_word;
};

struct e1000_fc_info {
	__u32 high_water;  /* Flow control high-water mark */
	__u32 low_water;  /* Flow control low-water mark */
	__u16 pause_time;  /* Flow control pause timer */
	__u16 refresh_time;  /* Flow control refresh timer */
	bool send_xon;  /* Flow control send XON */
	bool strict_ieee;  /* Strict IEEE mode */
	enum e1000_fc_mode current_mode;  /* FC mode in effect */
	enum e1000_fc_mode requested_mode;  /* FC mode requested by caller */
};

struct e1000_mbx_operations {
	__s32 (*init_params)(struct e1000_hw *hw);
	__s32 (*read)(struct e1000_hw *, __u32 *, __u16,  __u16);
	__s32 (*write)(struct e1000_hw *, __u32 *, __u16, __u16);
	__s32 (*read_posted)(struct e1000_hw *, __u32 *, __u16,  __u16);
	__s32 (*write_posted)(struct e1000_hw *, __u32 *, __u16, __u16);
	__s32 (*check_for_msg)(struct e1000_hw *, __u16);
	__s32 (*check_for_ack)(struct e1000_hw *, __u16);
	__s32 (*check_for_rst)(struct e1000_hw *, __u16);
};

struct e1000_mbx_info {
	struct e1000_mbx_operations ops;
	__u32 timeout;
	__u32 usec_delay;
	__u16 size;
};

struct e1000_shadow_ram {
	__u16  value;
	bool modified;
};

#define E1000_SHADOW_RAM_WORDS		2048

#ifdef ULP_SUPPORT
/* I218 PHY Ultra Low Power (ULP) states */
enum e1000_ulp_state {
	e1000_ulp_state_unknown,
	e1000_ulp_state_off,
	e1000_ulp_state_on,
};

#endif /* ULP_SUPPORT */

struct e1000_dev_spec_vf {
	__u32 vf_number;
	__u32 v2p_mailbox;
};

struct e1000_hw {
	void *back;

	/* Pci device information */
	struct pci_device *pdev;
	struct uk_netdev netdev;

	struct uk_alloc *a;

	struct e1000_adapter *adapter;

	__u8 *hw_addr;
	__u8 *flash_address;
	unsigned long io_base;

	struct e1000_mac_info  mac;
	struct e1000_fc_info   fc;
	struct e1000_phy_info  phy;
	struct e1000_nvm_info  nvm;
	struct e1000_bus_info  bus;
	struct e1000_mbx_info mbx;

	__u16 nb_rx_queues;
	__u16 nb_tx_queues;

	__u16 device_id;
	__u16 subsystem_vendor_id;
	__u16 subsystem_device_id;
	__u16 vendor_id;

	__u8  revision_id;

	struct uk_alloc *allocator;
};

/* These functions must be implemented by drivers */
void e1000_pci_clear_mwi(struct e1000_hw *hw);
void e1000_pci_set_mwi(struct e1000_hw *hw);
__s32  e1000_read_pcie_cap_reg(struct e1000_hw *hw, __u32 reg, __u16 *value);
__s32  e1000_write_pcie_cap_reg(struct e1000_hw *hw, __u32 reg, __u16 *value);
void e1000_read_pci_cfg(struct e1000_hw *hw, __u32 reg, __u16 *value);
void e1000_write_pci_cfg(struct e1000_hw *hw, __u32 reg, __u16 *value);

#endif
