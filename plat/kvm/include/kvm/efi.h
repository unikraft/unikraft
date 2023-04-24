#ifndef __PLAT_CMN_EFI_H__
#define __PLAT_CMN_EFI_H__

#include <stdbool.h>
#include <uk/arch/limits.h>
#include <uk/arch/types.h>
#include <uk/bitcount.h>
#include <uk/essentials.h>

#if defined(__X86_64__)
#define __uk_efi_api __attribute__((ms_abi))
#else
#define __uk_efi_api
#endif

#define UK_EFI_SUCCESS			0
#define UK_EFI_UNSUPPORTED						\
	(3  | (1ULL << ((__SIZEOF_LONG__ << 3) - 1)))
#define UK_EFI_BUFFER_TOO_SMALL						\
	(5  | (1ULL << ((__SIZEOF_LONG__ << 3) - 1)))
#define UK_EFI_NOT_FOUND						\
	(14 | (1ULL << ((__SIZEOF_LONG__ << 3) - 1)))

typedef unsigned long uk_efi_status_t;
typedef __vaddr_t uk_efi_vaddr_t;
typedef __paddr_t uk_efi_paddr_t;
typedef __u64 uk_efi_uintn_t;
typedef void *uk_efi_event_t;
typedef void *uk_efi_hndl_t;
typedef __u64 uk_efi_tpl_t;

struct uk_efi_guid {
	__u32 b0_3;
	__u16 b4_5;
	__u16 b6_7;
	__u8 b8_15[8];
} __align(8);

enum uk_efi_if_type {
	EFI_NATIVE_INTERFACE
};

#define UK_EFI_VARIABLE_NON_VOLATILE				0x00000001
#define UK_EFI_VARIABLE_BOOTSERVICE_ACCESS			0x00000002
#define UK_EFI_VARIABLE_RUNTIME_ACCESS				0x00000004
#define UK_EFI_VARIABLE_HARDWARE_ERROR_RECORD			0x00000008
#define UK_EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS		0x00000010
#define UK_EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS	0x00000020
#define UK_EFI_VARIABLE_APPEND_WRITE				0x00000040
#define UK_EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS		0x00000080

#define UK_EFI_PAGE_SHIFT		12
#define UK_EFI_PAGE_SIZE		(1UL << UK_EFI_PAGE_SHIFT)
#define UK_EFI_PAGES_MAX		(__U64_MAX >> UK_EFI_PAGE_SHIFT)

#define UK_EFI_MEMORY_UC		0x0000000000000001
#define UK_EFI_MEMORY_WC		0x0000000000000002
#define UK_EFI_MEMORY_WT		0x0000000000000004
#define UK_EFI_MEMORY_WB		0x0000000000000008
#define UK_EFI_MEMORY_UCE		0x0000000000000010
#define UK_EFI_MEMORY_WP		0x0000000000001000
#define UK_EFI_MEMORY_RP		0x0000000000002000
#define UK_EFI_MEMORY_XP		0x0000000000004000
#define UK_EFI_MEMORY_NV		0x0000000000008000
#define UK_EFI_MEMORY_MORE_RELIABLE	0x0000000000010000
#define UK_EFI_MEMORY_RO		0x0000000000020000
#define UK_EFI_MEMORY_SP		0x0000000000040000
#define UK_EFI_MEMORY_CPU_CRYPTO	0x0000000000080000
#define UK_EFI_MEMORY_RUNTIME		0x8000000000000000
#define UK_EFI_MEMORY_ISA_VALID		0x4000000000000000
#define UK_EFI_MEMORY_ISA_MASK		0x0FFFF00000000000
enum uk_efi_mem_type {
	UK_EFI_RESERVED_MEMORY_TYPE,
	UK_EFI_LOADER_CODE,
	UK_EFI_LOADER_DATA,
	UK_EFI_BOOT_SERVICES_CODE,
	UK_EFI_BOOT_SERVICES_DATA,
	UK_EFI_RUNTIME_SERVICES_CODE,
	UK_EFI_RUNTIME_SERVICES_DATA,
	UK_EFI_CONVENTIONAL_MEMORY,
	UK_EFI_UNUSABLE_MEMORY,
	UK_EFI_ACPI_RECLAIM_MEMORY,
	UK_EFI_ACPI_MEMORY_NVS,
	UK_EFI_MEMORY_MAPPED_IO,
	UK_EFI_MEMORY_MAPPED_IO_PORT_SPACE,
	UK_EFI_PAL_CODE,
	UK_EFI_PERSISTENT_MEMORY,
	UK_EFI_UNACCEPTED_MEMORY_TYPE,
	UK_EFI_MAX_MEMORY_TYPE
};

enum uk_efi_alloc_type {
	UK_EFI_ALLOCATE_ANY_PAGES,
	UK_EFI_ALLOCATE_MAX_ADDRESS,
	UK_EFI_ALLOCATE_ADDRESS,
	UK_EFI_MAX_ALLOCATION_TYPE
};

enum uk_efi_timer_delay {
	UK_EFI_TIMER_CANCEL,
	UK_EFI_TIMER_PERIODIC,
	UK_EFI_TIMER_RELATIVE
};

enum uk_efi_locate_search_type {
	UK_EFI_ALL_HANDLES,
	UK_EFI_BY_REGISTER_NOTIFY,
	UK_EFI_BY_PROTOCOL
};

enum uk_efi_reset_type {
	UK_EFI_RESET_COLD,
	UK_EFI_RESET_WARM,
	UK_EFI_RESET_SHUTDOWN,
	UK_EFI_RESET_PLATFORM_SPECIFIC
};

enum uk_efi_graphics_pixel_format {
	UK_EFI_PIXEL_RED_GREEN_BLUE_RESERVED_8BIT_PER_COLOR,
	UK_EFI_PIXEL_BLUE_GREEN_RED_RESERVED_8BIT_PER_COLOR,
	UK_EFI_PIXEL_BIT_MASK,
	UK_EFI_PIXEL_BLT_ONLY,
	UK_EFI_PIXEL_FORMAT_MASK
};

enum uk_efi_graphics_output_blt_operation {
	UK_EFI_BLT_VIDEO_FILL,
	UK_EFI_BLT_VIDEO_TO_BLT_BUFFER,
	UK_EFI_BLT_BUFFER_TO_VIDEO,
	UK_EFI_BLT_VIDEO_TO_VIDEO,
	UK_EFI_GRAPHICS_OUTPUT_BLT_OPERATION_MAX
};

struct uk_efi_mem_desc {
	__u32 type;
	__u32 pad;
	uk_efi_paddr_t physical_start;
	uk_efi_vaddr_t virtual_start;
	__u64 number_of_pages;
	__u64 attribute;
} ;

struct uk_efi_dev_path_proto {
	__u8 type;
	__u8 subtype;
	__u16 len;
};

struct uk_efi_open_proto_info_entry {
	uk_efi_hndl_t agent_handle;
	uk_efi_hndl_t controller_handle;
	__u32 attributes;
	__u32 open_count;
};

struct uk_efi_tbl_hdr {
	__u64 signature;
	__u32 revision;
	__u32 header_size;
	__u32 crc32;
	__u32 reserved;
};

struct uk_efi_boot_services {
	struct uk_efi_tbl_hdr hdr;
	uk_efi_tpl_t
	(__uk_efi_api *raise_tpl)(uk_efi_tpl_t new_tpl);
	void
	(__uk_efi_api *restore_tpl)(uk_efi_tpl_t old_tpl);
	uk_efi_status_t
	(__uk_efi_api *allocate_pages)(enum uk_efi_alloc_type a_type,
				       enum uk_efi_mem_type m_type,
				       uk_efi_uintn_t num_pgs,
				       uk_efi_paddr_t *paddr);
	uk_efi_status_t
	(__uk_efi_api *free_pages)(uk_efi_paddr_t paddr,
				   uk_efi_uintn_t num_pgs);
	uk_efi_status_t
	(__uk_efi_api *get_memory_map)(uk_efi_uintn_t *map_sz,
				       struct uk_efi_mem_desc *map,
				       uk_efi_uintn_t *map_key,
				       uk_efi_uintn_t *desc_sz,
				       __u32 *desc_ver);
	uk_efi_status_t
	(__uk_efi_api *allocate_pool)(enum uk_efi_mem_type type,
				      uk_efi_uintn_t sz,
				      void **buf);
	uk_efi_status_t
	(__uk_efi_api *free_pool)(void *buf);
	uk_efi_status_t
	(__uk_efi_api *create_event)(__u32 type,
				    uk_efi_tpl_t notify_tpl,
				    void (*notify_func)(uk_efi_event_t event,
							void *ctx),
				    void *notify_ctx,
				    uk_efi_event_t *event);
	uk_efi_status_t
	(__uk_efi_api *set_timer)(uk_efi_event_t event,
				  enum uk_efi_timer_delay type,
				  __u64 time);
	uk_efi_status_t
	(__uk_efi_api *wait_for_event)(uk_efi_uintn_t num_events,
				       uk_efi_event_t *event,
				       uk_efi_uintn_t *idx);
	uk_efi_status_t
	(__uk_efi_api *signal_event)(uk_efi_event_t event);
	uk_efi_status_t
	(__uk_efi_api *close_event)(uk_efi_event_t event);
	uk_efi_status_t
	(__uk_efi_api *check_event)(uk_efi_event_t event);
	uk_efi_status_t
	(__uk_efi_api *install_protocol_interface)(uk_efi_hndl_t hndl,
						   struct uk_efi_guid *proto,
						   enum uk_efi_if_type if_type,
						   void *proto_if);
	uk_efi_status_t
	(__uk_efi_api *reinstall_protocol_interface)(uk_efi_hndl_t hndl,
						     struct uk_efi_guid *proto,
						     void *proto_if);
	uk_efi_status_t
	(__uk_efi_api *uninstall_protocol_interface)(uk_efi_hndl_t hndl,
						     struct uk_efi_guid *proto,
						     void *proto_if);
	uk_efi_status_t
	(__uk_efi_api *handle_protocol)(uk_efi_hndl_t hndl,
					struct uk_efi_guid *proto,
					void *proto_if);

	void *reserved;
	uk_efi_status_t
	(__uk_efi_api *register_protocol_notify)(struct uk_efi_guid *proto,
						 uk_efi_event_t event,
						 void **registration);
	uk_efi_status_t
	(__uk_efi_api *locate_handle)(enum uk_efi_locate_search_type type,
				      struct uk_efi_guid *proto,
				      void *key,
				      uk_efi_uintn_t *buf_sz,
				      uk_efi_hndl_t *buf);
	uk_efi_status_t
	(__uk_efi_api *locate_device_path)(struct uk_efi_guid *proto,
					   struct uk_efi_dev_path_proto **dev_path,
					   uk_efi_hndl_t *dev);
	uk_efi_status_t
	(__uk_efi_api *install_configuration_table)(struct uk_efi_guid *guid,
						    void *tbl);
	uk_efi_status_t
	(__uk_efi_api *load_image)(bool boot_policy,
				   uk_efi_hndl_t parent_img_hndl,
				   struct uk_efi_dev_path_proto *file_path,
				   void *src_buf,
				   uk_efi_uintn_t src_sz,
				   uk_efi_hndl_t *img_hndl);
	uk_efi_status_t
	(__uk_efi_api *start_image)(uk_efi_hndl_t img_hndl,
				    uk_efi_uintn_t *exit_data_sz,
				    __s16 **exit_data);
	uk_efi_status_t __noreturn
	(__uk_efi_api *exit)(uk_efi_hndl_t img_hndl,
			     uk_efi_status_t exit_status,
			     uk_efi_uintn_t exit_data_size,
			     __s16 *exit_data);
	uk_efi_status_t
	(__uk_efi_api *unload_image)(uk_efi_hndl_t img_hndl);
	uk_efi_status_t
	(__uk_efi_api *exit_boot_services)(uk_efi_hndl_t img_hndl,
					   uk_efi_uintn_t map_key);
	uk_efi_status_t
	(__uk_efi_api *get_next_monotonic_count)(__u64 *count);
	uk_efi_status_t
	(__uk_efi_api *stall)(uk_efi_uintn_t ms);
	uk_efi_status_t
	(__uk_efi_api *set_watchdog_timer)(uk_efi_uintn_t timeout,
					   __u64 wdt_code,
					   uk_efi_uintn_t data_sz,
					   __s16 *wdt_data);
	uk_efi_status_t
	(__uk_efi_api *connect_controller)(uk_efi_hndl_t ctlr_hndl,
					   uk_efi_hndl_t *drv_img_hndl,
					   struct uk_efi_dev_path_proto *rem_dev_path,
					   bool recursive);
	uk_efi_status_t
	(__uk_efi_api *disconnect_controller)(uk_efi_hndl_t ctlr_hndl,
					      uk_efi_hndl_t drv_img_hndl,
					      uk_efi_hndl_t child_hndl);
	uk_efi_status_t
	(__uk_efi_api *open_protocol)(uk_efi_hndl_t hndl,
				      struct uk_efi_guid *proto,
				      void **proto_if,
				      uk_efi_hndl_t agent_hndl,
				      uk_efi_hndl_t ctlr_hndl,
				      __u32 attr);
	uk_efi_status_t
	(__uk_efi_api *close_protocol)(uk_efi_hndl_t hndl,
				       struct uk_efi_guid *proto,
				       uk_efi_hndl_t agent_hndl,
				       uk_efi_hndl_t ctlr_hndl);
	uk_efi_status_t
	(__uk_efi_api *open_protocol_information)(uk_efi_hndl_t hndl,
						  struct uk_efi_guid *proto,
						  struct uk_efi_open_proto_info_entry *buf,
						  uk_efi_uintn_t num_info);
	uk_efi_status_t
	(__uk_efi_api *protocols_per_handle)(uk_efi_hndl_t hndl,
					     struct uk_efi_guid ***proto_buf,
					     uk_efi_uintn_t *num_proto_buf);
	uk_efi_status_t
	(__uk_efi_api *locate_handle_buffer)(enum uk_efi_locate_search_type type,
					     struct uk_efi_guid *proto,
					     void *key,
					     uk_efi_uintn_t *no_hndls,
					     uk_efi_hndl_t **buf);
	uk_efi_status_t
	(__uk_efi_api *locate_protocol)(struct uk_efi_guid *proto,
					void *registration,
					void **proto_if);
	uk_efi_status_t
	(__uk_efi_api *install_multiple_protocol_interfaces)(uk_efi_hndl_t *hndl,
							     ...);
	uk_efi_status_t
	(__uk_efi_api *uninstall_multiple_protocol_interfaces)(uk_efi_hndl_t *hndl,
							       ...);
	uk_efi_status_t
	(__uk_efi_api *calculate_crc32)(void *data,
					uk_efi_uintn_t data_sz,
					__u32 *crc32);

	void
	(__uk_efi_api *copy_mem)(void *dest,
				 const void *src,
				 uk_efi_uintn_t len);

	void
	(__uk_efi_api *set_mem)(void *buf,
				uk_efi_uintn_t sz,
				__u8 val);

	void
	(__uk_efi_api *create_event_ex)(__u32 type,
					uk_efi_tpl_t notify_tpl,
					void (*notify_func)(uk_efi_event_t event,
							    void *ctx),
					const void *notify_ctx,
					const struct uk_efi_guid *event_group,
					uk_efi_event_t *event);
};

struct uk_efi_time_caps {
	__u32 resolution;
	__u32 accuracy;
	bool sets_to_zero;
};

struct uk_efi_time {
	__u16 year;
	__u8 month;
	__u8 day;
	__u8 hour;
	__u8 minute;
	__u8 second;
	__u8 pad1;
	__u32 nanosecond;
	__s16 time_zone;
	__u8 daylight;
	__u8 pad2;
};

struct uk_efi_capsule_hdr {
	struct uk_efi_guid capsule_guid;
	__u32 header_size;
	__u32 flags;
	__u32 capsule_image_size;
};

#define MEMORY_ONLY_RESET_CONTROL_GUID					\
	(&(struct uk_efi_guid){						\
		.b0_3 = 0xe20939be,					\
		.b4_5 = 0x32d4,						\
		.b6_7 = 0x41be,						\
		.b8_15 = {0xa1, 0x50, 0x89, 0x7f,			\
			  0x85, 0xd4, 0x98, 0x29},			\
	})
struct uk_efi_runtime_services {
	struct uk_efi_tbl_hdr hdr;
	uk_efi_status_t
	(__uk_efi_api *get_time)(struct uk_efi_time *time,
				 struct uk_efi_time_caps *caps);
	uk_efi_status_t
	(__uk_efi_api *set_time)(struct uk_efi_time *time);
	uk_efi_status_t
	(__uk_efi_api *get_wakeup_time)(bool *enabled,
					bool *pending,
					struct uk_efi_time *time);
	uk_efi_status_t
	(__uk_efi_api *set_wakeup_time)(bool enabled,
					struct uk_efi_time *time);
	uk_efi_status_t
	(__uk_efi_api *set_virtual_address_map)(uk_efi_uintn_t mem_map_sz,
						uk_efi_uintn_t desc_sz,
						__u32 desc_ver,
						struct uk_efi_mem_desc *vmap);
	uk_efi_status_t
	(__uk_efi_api *convert_pointer)(uk_efi_uintn_t dbg_disposition,
					void **addr);
	uk_efi_status_t
	(__uk_efi_api *get_variable)(__s16 *var_name,
				     struct uk_efi_guid *vendor_guid,
				     __u32 *attr,
				     uk_efi_uintn_t *data_sz,
				     void *data);
	uk_efi_status_t
	(__uk_efi_api *get_next_variable_name)(uk_efi_uintn_t *var_name_sz,
					       __s16 *var_name,
					       struct uk_efi_guid *vendor_guid);
	uk_efi_status_t
	(__uk_efi_api *set_variable)(__s16 *var_name,
				     struct uk_efi_guid *vendor_guid,
				     __u32 attr,
				     uk_efi_uintn_t data_sz,
				     void *data);

	void
	(__uk_efi_api *get_next_high_monotonic_count)(__u32 *hi_count);

	void
	(__uk_efi_api *reset_system)(enum uk_efi_reset_type type,
				     uk_efi_status_t status,
				     uk_efi_uintn_t data_sz,
				     void *reset_data);

	void
	(__uk_efi_api *update_capsule)(struct uk_efi_capsule_hdr **caps_hdr_arr,
				       uk_efi_uintn_t caps_count,
				       uk_efi_paddr_t sg_list);

	void
	(__uk_efi_api *query_capsule_capabilities)(struct uk_efi_capsule_hdr **caps_hdr_arr,
						   uk_efi_uintn_t caps_count,
						   __u64 max_caps_sz,
						   enum uk_efi_reset_type *rst_type);
};

struct uk_efi_input_key {
	__u16 scan_code;
	__s16 unicode_char;
};

struct uk_efi_simple_txt_in_proto {
	uk_efi_status_t
	(__uk_efi_api *reset)(struct uk_efi_simple_txt_in_proto *this,
			      bool xverif);
	uk_efi_status_t
	(__uk_efi_api *read_key_stroke)(struct uk_efi_simple_txt_in_proto *this,
					struct uk_efi_input_key *key);

	uk_efi_event_t wait_for_key;
};

struct uk_efi_simple_out_mode {
	__s32 max_mode;
	__s32 mode;
	__s32 attribute;
	__s32 cursor_column;
	__s32 cursor_row;
	bool cursor_visible;
};

struct uk_efi_simple_txt_out_proto {
	uk_efi_status_t
	(__uk_efi_api *reset)(struct uk_efi_simple_txt_out_proto *this,
			      bool xverif);
	uk_efi_status_t
	(__uk_efi_api *output_string)(struct uk_efi_simple_txt_out_proto *this,
				      __s16 *str);
	uk_efi_status_t
	(__uk_efi_api *test_string)(struct uk_efi_simple_txt_out_proto *this,
				    __s16 *str);
	uk_efi_status_t
	(__uk_efi_api *query_mode)(struct uk_efi_simple_txt_out_proto *this,
				   uk_efi_uintn_t mode_num,
				   uk_efi_uintn_t columns,
				   uk_efi_uintn_t rows);
	uk_efi_status_t
	(__uk_efi_api *set_mode)(struct uk_efi_simple_txt_out_proto *this,
				 uk_efi_uintn_t mode_num);
	uk_efi_status_t
	(__uk_efi_api *set_attribute)(struct uk_efi_simple_txt_out_proto *this,
				      uk_efi_uintn_t attr);
	uk_efi_status_t
	(__uk_efi_api *clear_screen)(struct uk_efi_simple_txt_out_proto *this);
	uk_efi_status_t
	(__uk_efi_api *set_cursor_position)(struct uk_efi_simple_txt_out_proto *this,
					    uk_efi_uintn_t column,
					    uk_efi_uintn_t row);
	uk_efi_status_t
	(__uk_efi_api *enable_cursor)(struct uk_efi_simple_txt_out_proto *this,
				      bool visible);

	struct uk_efi_simple_out_mode *mode;
};

#define UK_EFI_MEMORY_ATTRIBUTES_TABLE_GUID				\
	(&(struct uk_efi_guid){						\
		.b0_3 = 0xdcfa911d,					\
		.b4_5 = 0x26eb,						\
		.b6_7 = 0x469f,						\
		.b8_15 = {0xa2, 0x20, 0x38, 0xb7,			\
			  0xdc, 0x46, 0x12, 0x20},			\
	})

struct uk_efi_mem_attr_tbl {
	__u32 version;
	__u32 number_of_entries;
	__u32 descriptor_size;
	__u32 flags;
	struct uk_efi_mem_desc entry[1];
};

#define UK_EFI_ACPI10_TABLE_GUID					\
	(&(struct uk_efi_guid){						\
		.b0_3 = 0xeb9d2d30,					\
		.b4_5 = 0x2d88,						\
		.b6_7 = 0x11d3,						\
		.b8_15 = {0x9a, 0x16, 0x00, 0x90,			\
			  0x27, 0x3f, 0xc1, 0x4d},			\
	})
#define UK_EFI_ACPI20_TABLE_GUID					\
	(&(struct uk_efi_guid){						\
		.b0_3 = 0x8868e871,					\
		.b4_5 = 0xe4f1,						\
		.b6_7 = 0x11d3,						\
		.b8_15 = {0xbc, 0x22, 0x00, 0x80,			\
			  0xc7, 0x3c, 0x88, 0x81},			\
	})
struct uk_efi_cfg_tbl {
	struct uk_efi_guid vendor_guid;
	void *vendor_table;
};

struct uk_efi_sys_tbl {
	struct uk_efi_tbl_hdr hdr;
	__s16 *firmware_vendor;
	__u32 firmware_revision;
	uk_efi_hndl_t console_in_handle;
	struct uk_efi_simple_txt_in_proto *con_in;
	uk_efi_hndl_t console_out_handle;
	struct uk_efi_simple_txt_out_proto *con_out;
	uk_efi_hndl_t standad_error_handle;
	struct uk_efi_simple_txt_out_proto *std_err;
	struct uk_efi_runtime_services *runtime_services;
	struct uk_efi_boot_services *boot_services;
	uk_efi_uintn_t number_of_table_entries;
	struct uk_efi_cfg_tbl *configuration_table;
};

#define UK_EFI_LOADED_IMAGE_PROTOCOL_GUID				\
	(&(struct uk_efi_guid){						\
		.b0_3 = 0x5B1B31A1,					\
		.b4_5 = 0x9562,						\
		.b6_7 = 0x11d2,						\
		.b8_15 = {0x8E, 0x3F, 0x00, 0xA0,			\
			  0xC9, 0x69, 0x72, 0x3B},			\
	})
struct uk_efi_ld_img_hndl {
	__u32 revision;
	uk_efi_hndl_t parent_handle;
	struct uk_efi_sys_tbl *system_table;
	uk_efi_hndl_t device_handle;
	void *file_path;
	void *reserved;
	__u32 load_options_size;
	void *load_options;
	void *image_base;
	__u64 image_size;
	enum uk_efi_mem_type image_code_type;
	enum uk_efi_mem_type image_data_type;
	uk_efi_status_t	(__uk_efi_api *unload)(uk_efi_hndl_t img_hndl);
};

#define UK_EFI_FILE_INFO_ID_GUID					\
	(&(struct uk_efi_guid){						\
		.b0_3 = 0x09576e92,					\
		.b4_5 = 0x6d3f,						\
		.b6_7 = 0x11d2,						\
		.b8_15 = {0x8e, 0x39, 0x00, 0xa0,			\
			  0xc9, 0x69, 0x72, 0x3b},			\
	})
struct uk_efi_file_info_id {
	__u64 size;
	__u64 file_size;
	__u64 physical_size;
	struct uk_efi_time create_time;
	struct uk_efi_time last_access_time;
	struct uk_efi_time modification_time;
	__u64 attribute;
	__s16 file_name[];
};

#define UK_EFI_FILE_SYSTEM_INFO_ID_GUID					\
	(&(struct uk_efi_guid){						\
		.b0_3 = 0x09576e93,					\
		.b4_5 = 0x6d3f,						\
		.b6_7 = 0x11d2,						\
		.b8_15 = {0x8e, 0x39, 0x00, 0xa0,			\
			  0xc9, 0x69, 0x72, 0x3b},			\
	})
struct uk_efi_file_system_info_id {
	__u64 size;
	bool read_only;
	__u64 volume_size;
	__u64 free_space;
	__u32 block_size;
	__s16 volume_label[];
};

#define UK_EFI_FILE_SYSTEM_VOLUME_LABEL_ID_GUID				\
	(&(struct uk_efi_guid){						\
		.b0_3 = 0xdb47d7d3,					\
		.b4_5 = 0xfe81,						\
		.b6_7 = 0x11d3,						\
		.b8_15 = {0x9a, 0x35, 0x00, 0x90,			\
			  0x27, 0x3f, 0xc1, 0x4d},			\
	})
struct uk_efi_file_system_volume_label_id {
	__s16 volume_label[0];
};

struct uk_efi_file_io_token {
	uk_efi_event_t event;
	uk_efi_status_t status;
	uk_efi_uintn_t buffer_size;
	void *buffer;
};

#define UK_EFI_FILE_MODE_READ				0x0000000000000001
#define UK_EFI_FILE_MODE_WRITE				0x0000000000000002
#define UK_EFI_FILE_MODE_CREATE				0x8000000000000000

#define UK_EFI_FILE_READ_ONLY				0x0000000000000001
#define UK_EFI_FILE_HIDDEN				0x0000000000000002
#define UK_EFI_FILE_SYSTEM				0x0000000000000004
#define UK_EFI_FILE_RESERVED				0x0000000000000008
#define UK_EFI_FILE_DIRECTORY				0x0000000000000010
#define UK_EFI_FILE_ARCHIVE				0x0000000000000020
#define UK_EFI_FILE_VALID_ATTR				0x0000000000000037
struct uk_efi_file_proto {
	__u64 revision;
	uk_efi_status_t
	(__uk_efi_api *open)(struct uk_efi_file_proto *this,
			     struct uk_efi_file_proto **new_hndl,
			     __s16 *file_name,
			     __u64 open_mode,
			     __u64 attr);
	uk_efi_status_t
	(__uk_efi_api *close)(struct uk_efi_file_proto *this);
	uk_efi_status_t
	(__uk_efi_api *delete)(struct uk_efi_file_proto *this);
	uk_efi_status_t
	(__uk_efi_api *read)(struct uk_efi_file_proto *this,
			     uk_efi_uintn_t *buf_sz,
			     void *buf);
	uk_efi_status_t
	(__uk_efi_api *write)(struct uk_efi_file_proto *this,
			      uk_efi_uintn_t *buf_sz,
			      void *buf);
	uk_efi_status_t
	(__uk_efi_api *get_position)(struct uk_efi_file_proto *this,
				     __u64 *pos);
	uk_efi_status_t
	(__uk_efi_api *set_position)(struct uk_efi_file_proto *this,
				     __u64 *pos);
	uk_efi_status_t
	(__uk_efi_api *get_info)(struct uk_efi_file_proto *this,
				 struct uk_efi_guid *info_type,
				 uk_efi_uintn_t *buf_sz,
				 void *buf);
	uk_efi_status_t
	(__uk_efi_api *set_info)(struct uk_efi_file_proto *this,
				 struct uk_efi_guid *info_type,
				 uk_efi_uintn_t buf_sz,
				 void *buf);
	uk_efi_status_t
	(__uk_efi_api *flush)(struct uk_efi_file_proto *this);

	uk_efi_status_t
	(__uk_efi_api *open_ex)(struct uk_efi_file_proto *this,
				struct uk_efi_file_proto **new_hndl,
				__s16 *file_name,
				__u64 open_mode,
				__u64 attr,
				struct uk_efi_file_io_token *token);
	uk_efi_status_t
	(__uk_efi_api *read_ex)(struct uk_efi_file_proto *this,
				struct uk_efi_file_io_token *token);
	uk_efi_status_t
	(__uk_efi_api *write_ex)(struct uk_efi_file_proto *this,
				 struct uk_efi_file_io_token *token);
	uk_efi_status_t
	(__uk_efi_api *flush_ex)(struct uk_efi_file_proto *this,
				 struct uk_efi_file_io_token *token);
};

#define UK_EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID				\
	(&(struct uk_efi_guid){						\
		.b0_3 = 0x0964e5b22,					\
		.b4_5 = 0x6459,						\
		.b6_7 = 0x11d2,						\
		.b8_15 = {0x8e, 0x39, 0x00, 0xa0,			\
			  0xc9, 0x69, 0x72, 0x3b},			\
	})
struct uk_efi_simple_fs_proto {
	__u64 revision;
	uk_efi_status_t
	(__uk_efi_api *open_volume)(struct uk_efi_simple_fs_proto *this,
				    struct uk_efi_file_proto **root);
};

#endif /* __PLAT_CMN_EFI_H__ */
