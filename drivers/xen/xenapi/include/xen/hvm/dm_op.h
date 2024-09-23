/*
 * Copyright (c) 2016, Citrix Systems Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __XEN_PUBLIC_HVM_DM_OP_H__
#define __XEN_PUBLIC_HVM_DM_OP_H__

#include "../xen.h"

#if defined(__XEN__) || defined(__XEN_TOOLS__)

#include "../event_channel.h"

#ifndef uint64_aligned_t
#define uint64_aligned_t uint64_t
#endif

/*
 * IOREQ Servers
 *
 * The interface between an I/O emulator an Xen is called an IOREQ Server.
 * A domain supports a single 'legacy' IOREQ Server which is instantiated if
 * parameter...
 *
 * HVM_PARAM_IOREQ_PFN is read (to get the gfn containing the synchronous
 * ioreq structures), or...
 * HVM_PARAM_BUFIOREQ_PFN is read (to get the gfn containing the buffered
 * ioreq ring), or...
 * HVM_PARAM_BUFIOREQ_EVTCHN is read (to get the event channel that Xen uses
 * to request buffered I/O emulation).
 *
 * The following hypercalls facilitate the creation of IOREQ Servers for
 * 'secondary' emulators which are invoked to implement port I/O, memory, or
 * PCI config space ranges which they explicitly register.
 */

typedef uint16_t ioservid_t;

/*
 * XEN_DMOP_create_ioreq_server: Instantiate a new IOREQ Server for a
 *                               secondary emulator.
 *
 * The <id> handed back is unique for target domain. The valur of
 * <handle_bufioreq> should be one of HVM_IOREQSRV_BUFIOREQ_* defined in
 * hvm_op.h. If the value is HVM_IOREQSRV_BUFIOREQ_OFF then  the buffered
 * ioreq ring will not be allocated and hence all emulation requests to
 * this server will be synchronous.
 */
#define XEN_DMOP_create_ioreq_server 1

struct xen_dm_op_create_ioreq_server {
    /* IN - should server handle buffered ioreqs */
    uint8_t handle_bufioreq;
    uint8_t pad[3];
    /* OUT - server id */
    ioservid_t id;
};

/*
 * XEN_DMOP_get_ioreq_server_info: Get all the information necessary to
 *                                 access IOREQ Server <id>.
 *
 * The emulator needs to map the synchronous ioreq structures and buffered
 * ioreq ring (if it exists) that Xen uses to request emulation. These are
 * hosted in the target domain's gmfns <ioreq_gfn> and <bufioreq_gfn>
 * respectively. In addition, if the IOREQ Server is handling buffered
 * emulation requests, the emulator needs to bind to event channel
 * <bufioreq_port> to listen for them. (The event channels used for
 * synchronous emulation requests are specified in the per-CPU ioreq
 * structures in <ioreq_gfn>).
 * If the IOREQ Server is not handling buffered emulation requests then the
 * values handed back in <bufioreq_gfn> and <bufioreq_port> will both be 0.
 */
#define XEN_DMOP_get_ioreq_server_info 2

struct xen_dm_op_get_ioreq_server_info {
    /* IN - server id */
    ioservid_t id;
    uint16_t pad;
    /* OUT - buffered ioreq port */
    evtchn_port_t bufioreq_port;
    /* OUT - sync ioreq gfn */
    uint64_aligned_t ioreq_gfn;
    /* OUT - buffered ioreq gfn */
    uint64_aligned_t bufioreq_gfn;
};

/*
 * XEN_DMOP_map_io_range_to_ioreq_server: Register an I/O range for
 *                                        emulation by the client of
 *                                        IOREQ Server <id>.
 * XEN_DMOP_unmap_io_range_from_ioreq_server: Deregister an I/O range
 *                                            previously registered for
 *                                            emulation by the client of
 *                                            IOREQ Server <id>.
 *
 * There are three types of I/O that can be emulated: port I/O, memory
 * accesses and PCI config space accesses. The <type> field denotes which
 * type of range* the <start> and <end> (inclusive) fields are specifying.
 * PCI config space ranges are specified by segment/bus/device/function
 * values which should be encoded using the DMOP_PCI_SBDF helper macro
 * below.
 *
 * NOTE: unless an emulation request falls entirely within a range mapped
 * by a secondary emulator, it will not be passed to that emulator.
 */
#define XEN_DMOP_map_io_range_to_ioreq_server 3
#define XEN_DMOP_unmap_io_range_from_ioreq_server 4

struct xen_dm_op_ioreq_server_range {
    /* IN - server id */
    ioservid_t id;
    uint16_t pad;
    /* IN - type of range */
    uint32_t type;
# define XEN_DMOP_IO_RANGE_PORT   0 /* I/O port range */
# define XEN_DMOP_IO_RANGE_MEMORY 1 /* MMIO range */
# define XEN_DMOP_IO_RANGE_PCI    2 /* PCI segment/bus/dev/func range */
    /* IN - inclusive start and end of range */
    uint64_aligned_t start, end;
};

#define XEN_DMOP_PCI_SBDF(s,b,d,f) \
	((((s) & 0xffff) << 16) |  \
	 (((b) & 0xff) << 8) |     \
	 (((d) & 0x1f) << 3) |     \
	 ((f) & 0x07))

/*
 * XEN_DMOP_set_ioreq_server_state: Enable or disable the IOREQ Server <id>
 *
 * The IOREQ Server will not be passed any emulation requests until it is
 * in the enabled state.
 * Note that the contents of the ioreq_gfn and bufioreq_gfn (see
 * XEN_DMOP_get_ioreq_server_info) are not meaningful until the IOREQ Server
 * is in the enabled state.
 */
#define XEN_DMOP_set_ioreq_server_state 5

struct xen_dm_op_set_ioreq_server_state {
    /* IN - server id */
    ioservid_t id;
    /* IN - enabled? */
    uint8_t enabled;
    uint8_t pad;
};

/*
 * XEN_DMOP_destroy_ioreq_server: Destroy the IOREQ Server <id>.
 *
 * Any registered I/O ranges will be automatically deregistered.
 */
#define XEN_DMOP_destroy_ioreq_server 6

struct xen_dm_op_destroy_ioreq_server {
    /* IN - server id */
    ioservid_t id;
    uint16_t pad;
};

/*
 * XEN_DMOP_track_dirty_vram: Track modifications to the specified pfn
 *                            range.
 *
 * NOTE: The bitmap passed back to the caller is passed in a
 *       secondary buffer.
 */
#define XEN_DMOP_track_dirty_vram 7

struct xen_dm_op_track_dirty_vram {
    /* IN - number of pages to be tracked */
    uint32_t nr;
    uint32_t pad;
    /* IN - first pfn to track */
    uint64_aligned_t first_pfn;
};

/*
 * XEN_DMOP_set_pci_intx_level: Set the logical level of one of a domain's
 *                              PCI INTx pins.
 */
#define XEN_DMOP_set_pci_intx_level 8

struct xen_dm_op_set_pci_intx_level {
    /* IN - PCI INTx identification (domain:bus:device:intx) */
    uint16_t domain;
    uint8_t bus, device, intx;
    /* IN - Level: 0 -> deasserted, 1 -> asserted */
    uint8_t  level;
};

/*
 * XEN_DMOP_set_isa_irq_level: Set the logical level of a one of a domain's
 *                             ISA IRQ lines.
 */
#define XEN_DMOP_set_isa_irq_level 9

struct xen_dm_op_set_isa_irq_level {
    /* IN - ISA IRQ (0-15) */
    uint8_t  isa_irq;
    /* IN - Level: 0 -> deasserted, 1 -> asserted */
    uint8_t  level;
};

/*
 * XEN_DMOP_set_pci_link_route: Map a PCI INTx line to an IRQ line.
 */
#define XEN_DMOP_set_pci_link_route 10

struct xen_dm_op_set_pci_link_route {
    /* PCI INTx line (0-3) */
    uint8_t  link;
    /* ISA IRQ (1-15) or 0 -> disable link */
    uint8_t  isa_irq;
};

/*
 * XEN_DMOP_modified_memory: Notify that a set of pages were modified by
 *                           an emulator.
 *
 * DMOP buf 1 contains an array of xen_dm_op_modified_memory_extent with
 * @nr_extents entries.
 *
 * On error, @nr_extents will contain the index+1 of the extent that
 * had the error.  It is not defined if or which pages may have been
 * marked as dirty, in this event.
 */
#define XEN_DMOP_modified_memory 11

struct xen_dm_op_modified_memory {
    /*
     * IN - Number of extents to be processed
     * OUT -returns n+1 for failing extent
     */
    uint32_t nr_extents;
    /* IN/OUT - Must be set to 0 */
    uint32_t opaque;
};

struct xen_dm_op_modified_memory_extent {
    /* IN - number of contiguous pages modified */
    uint32_t nr;
    uint32_t pad;
    /* IN - first pfn modified */
    uint64_aligned_t first_pfn;
};

/*
 * XEN_DMOP_set_mem_type: Notify that a region of memory is to be treated
 *                        in a specific way. (See definition of
 *                        hvmmem_type_t).
 *
 * NOTE: In the event of a continuation (return code -ERESTART), the
 *       @first_pfn is set to the value of the pfn of the remaining
 *       region and @nr reduced to the size of the remaining region.
 */
#define XEN_DMOP_set_mem_type 12

struct xen_dm_op_set_mem_type {
    /* IN - number of contiguous pages */
    uint32_t nr;
    /* IN - new hvmmem_type_t of region */
    uint16_t mem_type;
    uint16_t pad;
    /* IN - first pfn in region */
    uint64_aligned_t first_pfn;
};

/*
 * XEN_DMOP_inject_event: Inject an event into a VCPU, which will
 *                        get taken up when it is next scheduled.
 *
 * Note that the caller should know enough of the state of the CPU before
 * injecting, to know what the effect of injecting the event will be.
 */
#define XEN_DMOP_inject_event 13

struct xen_dm_op_inject_event {
    /* IN - index of vCPU */
    uint32_t vcpuid;
    /* IN - interrupt vector */
    uint8_t vector;
    /* IN - event type (DMOP_EVENT_* ) */
    uint8_t type;
/* NB. This enumeration precisely matches hvm.h:X86_EVENTTYPE_* */
# define XEN_DMOP_EVENT_ext_int    0 /* external interrupt */
# define XEN_DMOP_EVENT_nmi        2 /* nmi */
# define XEN_DMOP_EVENT_hw_exc     3 /* hardware exception */
# define XEN_DMOP_EVENT_sw_int     4 /* software interrupt (CD nn) */
# define XEN_DMOP_EVENT_pri_sw_exc 5 /* ICEBP (F1) */
# define XEN_DMOP_EVENT_sw_exc     6 /* INT3 (CC), INTO (CE) */
    /* IN - instruction length */
    uint8_t insn_len;
    uint8_t pad0;
    /* IN - error code (or ~0 to skip) */
    uint32_t error_code;
    uint32_t pad1;
    /* IN - CR2 for page faults */
    uint64_aligned_t cr2;
};

/*
 * XEN_DMOP_inject_msi: Inject an MSI for an emulated device.
 */
#define XEN_DMOP_inject_msi 14

struct xen_dm_op_inject_msi {
    /* IN - MSI data (lower 32 bits) */
    uint32_t data;
    uint32_t pad;
    /* IN - MSI address (0xfeexxxxx) */
    uint64_aligned_t addr;
};

/*
 * XEN_DMOP_map_mem_type_to_ioreq_server : map or unmap the IOREQ Server <id>
 *                                      to specific memory type <type>
 *                                      for specific accesses <flags>
 *
 * For now, flags only accept the value of XEN_DMOP_IOREQ_MEM_ACCESS_WRITE,
 * which means only write operations are to be forwarded to an ioreq server.
 * Support for the emulation of read operations can be added when an ioreq
 * server has such requirement in future.
 */
#define XEN_DMOP_map_mem_type_to_ioreq_server 15

struct xen_dm_op_map_mem_type_to_ioreq_server {
    ioservid_t id;      /* IN - ioreq server id */
    uint16_t type;      /* IN - memory type */
    uint32_t flags;     /* IN - types of accesses to be forwarded to the
                           ioreq server. flags with 0 means to unmap the
                           ioreq server */

#define XEN_DMOP_IOREQ_MEM_ACCESS_READ (1u << 0)
#define XEN_DMOP_IOREQ_MEM_ACCESS_WRITE (1u << 1)

    uint64_t opaque;    /* IN/OUT - only used for hypercall continuation,
                           has to be set to zero by the caller */
};

/*
 * XEN_DMOP_remote_shutdown : Declare a shutdown for another domain
 *                            Identical to SCHEDOP_remote_shutdown
 */
#define XEN_DMOP_remote_shutdown 16

struct xen_dm_op_remote_shutdown {
    uint32_t reason;       /* SHUTDOWN_* => enum sched_shutdown_reason */
                           /* (Other reason values are not blocked) */
};

struct xen_dm_op {
    uint32_t op;
    uint32_t pad;
    union {
        struct xen_dm_op_create_ioreq_server create_ioreq_server;
        struct xen_dm_op_get_ioreq_server_info get_ioreq_server_info;
        struct xen_dm_op_ioreq_server_range map_io_range_to_ioreq_server;
        struct xen_dm_op_ioreq_server_range unmap_io_range_from_ioreq_server;
        struct xen_dm_op_set_ioreq_server_state set_ioreq_server_state;
        struct xen_dm_op_destroy_ioreq_server destroy_ioreq_server;
        struct xen_dm_op_track_dirty_vram track_dirty_vram;
        struct xen_dm_op_set_pci_intx_level set_pci_intx_level;
        struct xen_dm_op_set_isa_irq_level set_isa_irq_level;
        struct xen_dm_op_set_pci_link_route set_pci_link_route;
        struct xen_dm_op_modified_memory modified_memory;
        struct xen_dm_op_set_mem_type set_mem_type;
        struct xen_dm_op_inject_event inject_event;
        struct xen_dm_op_inject_msi inject_msi;
        struct xen_dm_op_map_mem_type_to_ioreq_server
                map_mem_type_to_ioreq_server;
        struct xen_dm_op_remote_shutdown remote_shutdown;
    } u;
};

#endif /* __XEN__ || __XEN_TOOLS__ */

struct xen_dm_op_buf {
    XEN_GUEST_HANDLE(void) h;
    xen_ulong_t size;
};
typedef struct xen_dm_op_buf xen_dm_op_buf_t;
DEFINE_XEN_GUEST_HANDLE(xen_dm_op_buf_t);

/* ` enum neg_errnoval
 * ` HYPERVISOR_dm_op(domid_t domid,
 * `                  unsigned int nr_bufs,
 * `                  xen_dm_op_buf_t bufs[])
 * `
 *
 * @domid is the domain the hypercall operates on.
 * @nr_bufs is the number of buffers in the @bufs array.
 * @bufs points to an array of buffers where @bufs[0] contains a struct
 * xen_dm_op, describing the specific device model operation and its
 * parameters.
 * @bufs[1..] may be referenced in the parameters for the purposes of
 * passing extra information to or from the domain.
 */

#endif /* __XEN_PUBLIC_HVM_DM_OP_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
