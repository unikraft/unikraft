/* SPDX-License-Identifier: MIT */
/*
 * 9pfs.h -- Xen 9PFS transport
 *
 * Refer to docs/misc/9pfs.markdown for the specification
 *
 * Copyright (C) 2017 Stefano Stabellini <stefano@aporeto.com>
 */

#ifndef __XEN_PUBLIC_IO_9PFS_H__
#define __XEN_PUBLIC_IO_9PFS_H__

#include "../grant_table.h"
#include "ring.h"

/*
 * See docs/misc/9pfs.pandoc in xen.git for the full specification:
 * https://xenbits.xen.org/docs/unstable/misc/9pfs.html
 */

/*
 ******************************************************************************
 *                                  Xenstore
 ******************************************************************************
 *
 * The frontend and the backend connect via xenstore to exchange
 * information. The toolstack creates front and back nodes with state
 * XenbusStateInitialising. The protocol node name is **9pfs**.
 *
 * Multiple rings are supported for each frontend and backend connection.
 *
 ******************************************************************************
 *                            Backend XenBus Nodes
 ******************************************************************************
 *
 * Backend specific properties, written by the backend, read by the
 * frontend:
 *
 *    versions
 *         Values:         <string>
 *
 *         List of comma separated protocol versions supported by the backend.
 *         For example "1,2,3". Currently the value is just "1", as there is
 *         only one version. N.B.: this is the version of the Xen transport
 *         protocol, not the version of 9pfs supported by the server.
 *
 *    max-rings
 *         Values:         <uint32_t>
 *
 *         The maximum supported number of rings per frontend.
 *
 *    max-ring-page-order
 *         Values:         <uint32_t>
 *
 *         The maximum supported size of a memory allocation in units of
 *         log2n(machine pages), e.g. 1 = 2 pages, 2 == 4 pages, etc. It
 *         must be at least 1.
 *
 * Backend configuration nodes, written by the toolstack, read by the
 * backend:
 *
 *    path
 *         Values:         <string>
 *
 *         Host filesystem path to share.
 *
 *    security_model
 *         Values:         "none"
 *
 *         *none*: files are stored using the same credentials as they are
 *                 created on the guest (no user ownership squash or remap)
 *         Only "none" is supported in this version of the protocol.
 *
 *    max-files
 *         Values:        <uint32_t>
 *
 *         The maximum number of files (including directories) allowed for
 *         this device. Backend support of this node is optional. If the node
 *         is not present or the value is zero the number of files is not
 *         limited.
 *
 *    max-open-files
 *         Values:        <uint32_t>
 *
 *         The maximum number of files the guest is allowed to have opened
 *         concurrently. Multiple concurrent opens of the same file are counted
 *         individually. Backend support of this node is optional. If the node
 *         is not present or the value is zero a backend specific default is
 *         applied.
 *
 *    max-space
 *         Values:        <uint32_t>
 *
 *         The maximum file space in MiBs the guest is allowed to use for this
 *         device. Backend support of this node is optional. If the node is
 *         not present or the value is zero the space is not limited.
 *
 *    auto-delete
 *         Values:        <bool>
 *
 *         When set to "1" the backend will delete the file with the oldest
 *         modification date below <path> in case the allowed maximum file
 *         space (see <max-space>) or file number (see <max-files>) is being
 *         exceeded due to guest activity (creation or extension of files).
 *         Files currently opened by the guest won't be deleted. Backend
 *         support of this node is optional.
 *
 ******************************************************************************
 *                            Frontend XenBus Nodes
 ******************************************************************************
 *
 *    version
 *         Values:         <string>
 *
 *         Protocol version, chosen among the ones supported by the backend
 *         (see **versions** under [Backend XenBus Nodes]). Currently the
 *         value must be "1".
 *
 *    num-rings
 *         Values:         <uint32_t>
 *
 *         Number of rings. It needs to be lower or equal to max-rings.
 *
 *    event-channel-<num> (event-channel-0, event-channel-1, etc)
 *         Values:         <uint32_t>
 *
 *         The identifier of the Xen event channel used to signal activity
 *         in the ring buffer. One for each ring.
 *
 *    ring-ref<num> (ring-ref0, ring-ref1, etc)
 *         Values:         <uint32_t>
 *
 *         The Xen grant reference granting permission for the backend to
 *         map a page with information to setup a share ring. One for each
 *         ring.
 *
 *    tag
 *         Values:         <string>
 *
 *         Alphanumeric tag that identifies the 9pfs share. The client needs
 *         to know the tag to be able to mount it.
 *
 ******************************************************************************
 *                              State Machine
 ******************************************************************************
 *
 * Initialization:
 *
 *    *Front*                               *Back*
 *    XenbusStateInitialising               XenbusStateInitialising
 *                                          - Query backend device
 *                                            identification data.
 *                                          - Publish backend features
 *                                            and transport parameters.
 *                                                         |
 *                                                         |
 *                                                         V
 *                                                  XenbusStateInitWait
 *
 *    - Query virtual device
 *      properties.
 *    - Query backend features and
 *      transport parameters.
 *    - Setup OS device instance.
 *    - Allocate and initialize the
 *      request ring(s) and
 *      event-channel(s).
 *    - Publish transport parameters
 *      that will be in effect during
 *      this connection.
 *                 |
 *                 |
 *                 V
 *       XenbusStateInitialised
 *
 *                                          - Query frontend transport
 *                                            parameters.
 *                                          - Connect to the request ring(s)
 *                                            and event channel(s).
 *                                                         |
 *                                                         |
 *                                                         V
 *                                                 XenbusStateConnected
 *
 *    - Query backend device properties.
 *    - Finalize OS virtual device
 *      instance.
 *                |
 *                |
 *                V
 *       XenbusStateConnected
 *
 * Once frontend and backend are connected, they have a shared page per
 * ring, which are used to setup the rings, and an event channel per ring,
 * which are used to send notifications.
 *
 * Shutdown:
 *
 *    *Front*                            *Back*
 *    XenbusStateConnected               XenbusStateConnected
 *                |
 *                |
 *                V
 *       XenbusStateClosing
 *
 *                                       - Unmap grants
 *                                       - Unbind evtchns
 *                                                 |
 *                                                 |
 *                                                 V
 *                                         XenbusStateClosing
 *
 *    - Unbind evtchns
 *    - Free rings
 *    - Free data structures
 *               |
 *               |
 *               V
 *       XenbusStateClosed
 *
 *                                       - Free remaining data structures
 *                                                 |
 *                                                 |
 *                                                 V
 *                                         XenbusStateClosed
 *
 ******************************************************************************
 */

DEFINE_XEN_FLEX_RING_AND_INTF(xen_9pfs);

#endif

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
