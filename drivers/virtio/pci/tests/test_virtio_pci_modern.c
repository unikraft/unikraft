/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <uk/bitops.h>
#include <uk/intctlr/limits.h>
#include <uk/test.h>

#include "../virtio_pci_private.h"

static void virtio_pci_test_put16(__u8 *cfg, __u8 pos, __u16 val)
{
	cfg[pos] = (__u8)val;
	cfg[pos + 1] = (__u8)(val >> 8);
}

static void virtio_pci_test_put32(__u8 *cfg, __u8 pos, __u32 val)
{
	cfg[pos] = (__u8)val;
	cfg[pos + 1] = (__u8)(val >> 8);
	cfg[pos + 2] = (__u8)(val >> 16);
	cfg[pos + 3] = (__u8)(val >> 24);
}

static void virtio_pci_test_cap(__u8 *cfg, __u8 pos, __u8 next,
				__u8 len, __u8 type, __u8 bar,
				__u32 off, __u32 cap_len)
{
	cfg[pos] = VIRTIO_PCI_CAP_VENDOR;
	cfg[pos + 1] = next;
	cfg[pos + 2] = len;
	cfg[pos + 3] = type;
	cfg[pos + 4] = bar;
	virtio_pci_test_put32(cfg, pos + 8, off);
	virtio_pci_test_put32(cfg, pos + 12, cap_len);
}

static void virtio_pci_test_valid_cfg(__u8 cfg[256],
				      struct pci_bar bars[PCI_BAR_COUNT])
{
	memset(cfg, 0, 256);
	memset(bars, 0, sizeof(struct pci_bar) * PCI_BAR_COUNT);
	virtio_pci_test_put16(cfg, PCI_STATUS, PCI_STATUS_CAP_LIST);
	cfg[PCI_CAPABILITIES_PTR] = 0x40;
	bars[0].type = PCI_BAR_MEM;
	bars[0].size = 0x1000;
	bars[0].pbase = 0x100000;
	bars[0].vbase = 0x100000;

	virtio_pci_test_cap(cfg, 0x40, 0x54, sizeof(struct virtio_pci_cap),
			    VIRTIO_PCI_CAP_COMMON_CFG, 0, 0x100,
			    VPCI_COMMON_CFG_MIN_LEN);
	virtio_pci_test_cap(cfg, 0x54, 0x6c,
			    sizeof(struct virtio_pci_notify_cap),
			    VIRTIO_PCI_CAP_NOTIFY_CFG, 0, 0x200, 0x100);
	virtio_pci_test_put32(cfg, 0x54 + 16, 4);
	virtio_pci_test_cap(cfg, 0x6c, 0x7c, sizeof(struct virtio_pci_cap),
			    VIRTIO_PCI_CAP_ISR_CFG, 0, 0x300, 1);
	virtio_pci_test_cap(cfg, 0x7c, 0, sizeof(struct virtio_pci_cap),
			    VIRTIO_PCI_CAP_DEVICE_CFG, 0, 0x400, 0x100);
}

#define VIRTIO_PCI_TEST_MAX_WRITES 16

struct virtio_pci_test_write {
	__u32 off;
	__u32 val;
};

struct virtio_pci_test_io {
	__u32 device_feature_select;
	__u32 driver_feature_select;
	__u32 device_feature[2];
	__u32 driver_feature[2];
	__u32 config[2];
	__u16 num_queues;
	__u16 queue_select;
	__u16 queue_size[2];
	__u16 queue_notify_off[2];
	__u16 queue_enable[2];
	__u8 config_generation;
	__u8 device_status;
	__u8 reject_features_ok;
	unsigned int writes;
	struct virtio_pci_test_write write[VIRTIO_PCI_TEST_MAX_WRITES];
};

static void virtio_pci_test_region(struct virtio_pci_region *region,
				   struct virtio_pci_test_io *io)
{
	memset(region, 0, sizeof(*region));
	region->type = PCI_BAR_MEM;
	region->length = VPCI_COMMON_CFG_MIN_LEN;
	region->test_cookie = io;
}

static __u8 virtio_pci_test_read8(void *cookie, __u32 off)
{
	struct virtio_pci_test_io *io = cookie;

	if (off == offsetof(struct virtio_pci_common_cfg, config_generation))
		return io->config_generation;
	if (off == offsetof(struct virtio_pci_common_cfg, device_status))
		return io->device_status;

	return 0;
}

static void virtio_pci_test_write8(void *cookie, __u32 off, __u8 val)
{
	struct virtio_pci_test_io *io = cookie;

	if (off == offsetof(struct virtio_pci_common_cfg, device_status)) {
		if (io->reject_features_ok)
			val &= ~VIRTIO_CONFIG_STATUS_FEATURES_OK;
		io->device_status = val;
	}
}

static __u16 virtio_pci_test_read16(void *cookie, __u32 off)
{
	struct virtio_pci_test_io *io = cookie;
	__u16 queue = io->queue_select;

	if (off == offsetof(struct virtio_pci_common_cfg, num_queues))
		return io->num_queues;
	if (queue >= 2)
		return 0;
	if (off == offsetof(struct virtio_pci_common_cfg, queue_size))
		return io->queue_size[queue];
	if (off == offsetof(struct virtio_pci_common_cfg, queue_notify_off))
		return io->queue_notify_off[queue];
	if (off == offsetof(struct virtio_pci_common_cfg, queue_enable))
		return io->queue_enable[queue];

	return 0;
}

static void virtio_pci_test_write16(void *cookie, __u32 off, __u16 val)
{
	struct virtio_pci_test_io *io = cookie;

	if (off == offsetof(struct virtio_pci_common_cfg, queue_select))
		io->queue_select = val;
}

static __u32 virtio_pci_test_read32(void *cookie, __u32 off)
{
	struct virtio_pci_test_io *io = cookie;

	if (off == offsetof(struct virtio_pci_common_cfg, device_feature))
		return io->device_feature[io->device_feature_select & 1];

	return 0;
}

static __u32 virtio_pci_test_config_read32(void *cookie, __u32 off)
{
	struct virtio_pci_test_io *io = cookie;

	if (off == 0)
		return io->config[0];
	if (off == sizeof(__u32))
		return io->config[1];

	return 0;
}

static void virtio_pci_test_write32(void *cookie, __u32 off, __u32 val)
{
	struct virtio_pci_test_io *io = cookie;

	if (io->writes < VIRTIO_PCI_TEST_MAX_WRITES) {
		io->write[io->writes].off = off;
		io->write[io->writes].val = val;
	}
	io->writes++;

	if (off == offsetof(struct virtio_pci_common_cfg,
			    device_feature_select)) {
		io->device_feature_select = val;
		return;
	}
	if (off == offsetof(struct virtio_pci_common_cfg,
			    driver_feature_select)) {
		io->driver_feature_select = val;
		return;
	}
	if (off == offsetof(struct virtio_pci_common_cfg, driver_feature))
		io->driver_feature[io->driver_feature_select & 1] = val;
}

static int virtio_pci_test_modern_rc;
static const struct virtio_pci_modern_caps *virtio_pci_test_caps;

static int virtio_pci_test_modern_add_dev(struct pci_device *pci_dev,
					  struct virtio_pci_dev *vpci_dev)
{
	if (virtio_pci_test_modern_rc)
		return virtio_pci_test_modern_rc;
	if (!virtio_pci_test_caps)
		return -ENODEV;

	return virtio_pci_modern_init_dev(pci_dev, vpci_dev,
					  virtio_pci_test_caps);
}

static void virtio_pci_test_pdev(struct pci_device *pdev, __u16 device_id,
				 __u16 subsystem_device_id,
				 enum pci_bar_type bar_type, void *bar_mem)
{
	memset(pdev, 0, sizeof(*pdev));
	pdev->id.vendor_id = VIRTIO_PCI_VENDOR_ID;
	pdev->id.device_id = device_id;
	pdev->id.subsystem_device_id = subsystem_device_id;
	pdev->base = (unsigned long)bar_mem;
	pdev->bar[0].type = bar_type;
	pdev->bar[0].index = 0;
	pdev->bar[0].pbase = (unsigned long)bar_mem;
	pdev->bar[0].vbase = (unsigned long)bar_mem;
	pdev->bar[0].size = 0x1000;
}

UK_TESTCASE(virtio_pci_modern, parse_valid_caps)
{
	struct virtio_pci_modern_caps caps;
	struct pci_bar bars[PCI_BAR_COUNT];
	__u8 cfg[256];

	virtio_pci_test_valid_cfg(cfg, bars);
	UK_TEST_EXPECT(virtio_pci_modern_parse_caps_raw(cfg, bars,
							&caps) == 0);
	UK_TEST_EXPECT(caps.has_common);
	UK_TEST_EXPECT(caps.has_notify);
	UK_TEST_EXPECT(caps.has_isr);
	UK_TEST_EXPECT(caps.has_device);
}

UK_TESTCASE(virtio_pci_modern, parse_missing_required_caps)
{
	struct virtio_pci_modern_caps caps;
	struct pci_bar bars[PCI_BAR_COUNT];
	__u8 cfg[256];

	virtio_pci_test_valid_cfg(cfg, bars);
	cfg[PCI_CAPABILITIES_PTR] = 0x54;
	UK_TEST_EXPECT(virtio_pci_modern_parse_caps_raw(cfg, bars,
							&caps) == -ENODEV);

	virtio_pci_test_valid_cfg(cfg, bars);
	cfg[0x40 + 1] = 0x6c;
	UK_TEST_EXPECT(virtio_pci_modern_parse_caps_raw(cfg, bars,
							&caps) == -ENODEV);

	virtio_pci_test_valid_cfg(cfg, bars);
	cfg[0x54 + 1] = 0x7c;
	UK_TEST_EXPECT(virtio_pci_modern_parse_caps_raw(cfg, bars,
							&caps) == -ENODEV);
}

UK_TESTCASE(virtio_pci_modern, parse_reserved_and_malformed_caps)
{
	struct virtio_pci_modern_caps caps;
	struct pci_bar bars[PCI_BAR_COUNT];
	__u8 cfg[256];

	virtio_pci_test_valid_cfg(cfg, bars);
	virtio_pci_test_cap(cfg, 0x40, 0x54, sizeof(struct virtio_pci_cap),
			    0xff, 0, 0x100, 0x10);
	virtio_pci_test_cap(cfg, 0x54, 0x68, sizeof(struct virtio_pci_cap),
			    VIRTIO_PCI_CAP_COMMON_CFG, 0, 0x100,
			    VPCI_COMMON_CFG_MIN_LEN);
	virtio_pci_test_cap(cfg, 0x68, 0x80,
			    sizeof(struct virtio_pci_notify_cap),
			    VIRTIO_PCI_CAP_NOTIFY_CFG, 0, 0x200, 0x100);
	virtio_pci_test_put32(cfg, 0x68 + 16, 4);
	virtio_pci_test_cap(cfg, 0x80, 0, sizeof(struct virtio_pci_cap),
			    VIRTIO_PCI_CAP_ISR_CFG, 0, 0x300, 1);
	UK_TEST_EXPECT(virtio_pci_modern_parse_caps_raw(cfg, bars,
							&caps) == 0);

	virtio_pci_test_valid_cfg(cfg, bars);
	cfg[0x40 + 2] = sizeof(struct virtio_pci_cap) - 1;
	UK_TEST_EXPECT(virtio_pci_modern_parse_caps_raw(cfg, bars,
							&caps) == -EINVAL);

	virtio_pci_test_valid_cfg(cfg, bars);
	cfg[0x40 + 2] = sizeof(struct virtio_pci_cap) + 8;
	UK_TEST_EXPECT(virtio_pci_modern_parse_caps_raw(cfg, bars,
							&caps) == 0);

	virtio_pci_test_valid_cfg(cfg, bars);
	cfg[0x40 + 1] = 0x54;
	cfg[0x54 + 1] = 0x40;
	UK_TEST_EXPECT(virtio_pci_modern_parse_caps_raw(cfg, bars,
							&caps) == -EINVAL);

	virtio_pci_test_valid_cfg(cfg, bars);
	cfg[0x40 + 4] = PCI_BAR_COUNT;
	UK_TEST_EXPECT(virtio_pci_modern_parse_caps_raw(cfg, bars,
							&caps) == -ENODEV);

	virtio_pci_test_valid_cfg(cfg, bars);
	cfg[PCI_CAPABILITIES_PTR] = 0xf4;
	cfg[0xf4] = VIRTIO_PCI_CAP_VENDOR;
	cfg[0xf5] = 0;
	cfg[0xf6] = sizeof(struct virtio_pci_cap);
	UK_TEST_EXPECT(virtio_pci_modern_parse_caps_raw(cfg, bars,
							&caps) == -EINVAL);
}

UK_TESTCASE(virtio_pci_modern, parse_capability_alternatives)
{
	struct virtio_pci_modern_caps caps;
	struct pci_bar bars[PCI_BAR_COUNT];
	__u8 cfg[256];

	virtio_pci_test_valid_cfg(cfg, bars);
	memset(cfg + 0x40, 0, 0x50);
	virtio_pci_test_put16(cfg, PCI_STATUS, PCI_STATUS_CAP_LIST);
	cfg[PCI_CAPABILITIES_PTR] = 0x40;
	virtio_pci_test_cap(cfg, 0x40, 0x50, sizeof(struct virtio_pci_cap),
			    VIRTIO_PCI_CAP_COMMON_CFG, 0, 0x100, 4);
	virtio_pci_test_cap(cfg, 0x50, 0x60, sizeof(struct virtio_pci_cap),
			    VIRTIO_PCI_CAP_COMMON_CFG, 0, 0x100,
			    VPCI_COMMON_CFG_MIN_LEN);
	virtio_pci_test_cap(cfg, 0x60, 0x74,
			    sizeof(struct virtio_pci_notify_cap),
			    VIRTIO_PCI_CAP_NOTIFY_CFG, 0, 0x200, 0x100);
	virtio_pci_test_put32(cfg, 0x60 + 16, 3);
	virtio_pci_test_cap(cfg, 0x74, 0, sizeof(struct virtio_pci_cap),
			    VIRTIO_PCI_CAP_ISR_CFG, 0, 0x300, 1);
	UK_TEST_EXPECT(virtio_pci_modern_parse_caps_raw(cfg, bars,
							&caps) == 0);
	UK_TEST_EXPECT(caps.common.length == VPCI_COMMON_CFG_MIN_LEN);
	UK_TEST_EXPECT(caps.notify.notify_off_multiplier == 3);

	virtio_pci_test_valid_cfg(cfg, bars);
	virtio_pci_test_put32(cfg, 0x7c + 12, 0);
	UK_TEST_EXPECT(virtio_pci_modern_parse_caps_raw(cfg, bars,
							&caps) == 0);
	UK_TEST_EXPECT(!caps.has_device);

	virtio_pci_test_valid_cfg(cfg, bars);
	virtio_pci_test_put32(cfg, 0x7c + 8, 0xff0);
	UK_TEST_EXPECT(virtio_pci_modern_parse_caps_raw(cfg, bars,
							&caps) == 0);
	UK_TEST_EXPECT(!caps.has_device);

	virtio_pci_test_valid_cfg(cfg, bars);
	virtio_pci_test_put32(cfg, 0x40 + 8, 0xff0);
	UK_TEST_EXPECT(virtio_pci_modern_parse_caps_raw(cfg, bars,
							&caps) == -EINVAL);
}

UK_TESTCASE(virtio_pci_modern, id_and_notify_helpers)
{
	struct virtio_pci_dev vpdev;
	unsigned long invalid_irq = UK_INTCTLR_MAX_IRQ + 1UL;
	__u16 id;
	__u64 addr = 0;

	UK_TEST_EXPECT(virtio_pci_modern_virtio_id(0x1041, &id) == 0);
	UK_TEST_EXPECT(id == 1);
	UK_TEST_EXPECT(virtio_pci_modern_virtio_id(0x1042, &id) == 0);
	UK_TEST_EXPECT(id == 2);
	UK_TEST_EXPECT(virtio_pci_modern_virtio_id(0x1040, &id) == -EINVAL);
	UK_TEST_EXPECT(virtio_pci_modern_virtio_id(0x1080, &id) == -EINVAL);
	UK_TEST_EXPECT(virtio_pci_device_id_supported(0x1000));
	UK_TEST_EXPECT(virtio_pci_device_id_supported(0x103f));
	UK_TEST_EXPECT(!virtio_pci_device_id_supported(0x1040));
	UK_TEST_EXPECT(virtio_pci_device_id_supported(0x107f));
	UK_TEST_EXPECT(!virtio_pci_device_id_supported(0x0fff));
	UK_TEST_EXPECT(!virtio_pci_device_id_supported(0x1080));
	UK_TEST_EXPECT(vpci_intx_routing_valid(1, 0) == 0);
	UK_TEST_EXPECT(vpci_intx_routing_valid(4, UK_INTCTLR_MAX_IRQ) == 0);
	UK_TEST_EXPECT(vpci_intx_routing_valid(0, 1) == -EINVAL);
	UK_TEST_EXPECT(vpci_intx_routing_valid(5, 1) == -EINVAL);
	UK_TEST_EXPECT(vpci_intx_routing_valid(1, invalid_irq) == -EINVAL);

	memset(&vpdev, 0, sizeof(vpdev));
	vpdev.notify_cfg.base = 0x1000;
	vpdev.notify_cfg.length = 0x100;
	vpdev.notify_off_multiplier = 0;
	UK_TEST_EXPECT(virtio_pci_modern_notify_addr(&vpdev, 7, &addr) == 0);
	UK_TEST_EXPECT(addr == 0x1000);

	vpdev.notify_off_multiplier = 4;
	UK_TEST_EXPECT(virtio_pci_modern_notify_addr(&vpdev, 7, &addr) == 0);
	UK_TEST_EXPECT(addr == 0x101c);

	vpdev.notify_off_multiplier = 3;
	UK_TEST_EXPECT(virtio_pci_modern_notify_addr(&vpdev, 2, &addr) == 0);
	UK_TEST_EXPECT(addr == 0x1006);
	UK_TEST_EXPECT(virtio_pci_modern_notify_addr(&vpdev, 1,
						     &addr) == -EINVAL);

	vpdev.notify_cfg.base = __U64_MAX - 1;
	vpdev.notify_off_multiplier = 4;
	UK_TEST_EXPECT(virtio_pci_modern_notify_addr(&vpdev, 1,
						     &addr) == -EINVAL);
}

UK_TESTCASE(virtio_pci_modern, feature_banks)
{
	struct virtio_pci_test_io io;
	struct virtio_pci_dev vpdev;
	__u64 features;
	int rc;

	memset(&io, 0, sizeof(io));
	memset(&vpdev, 0, sizeof(vpdev));
	io.device_feature[0] = 0x55aa00ff;
	io.device_feature[1] = 0x80000001;
	virtio_pci_test_region(&vpdev.common_cfg, &io);
	vpdev.common_cfg.test_read32 = virtio_pci_test_read32;
	vpdev.common_cfg.test_write32 = virtio_pci_test_write32;

	features = vpci_modern_features_get(&vpdev.vdev);
	UK_TEST_EXPECT(features == (((__u64)0x80000001 << 32) | 0x55aa00ff));
	UK_TEST_EXPECT(io.writes == 2);
	UK_TEST_EXPECT(io.write[0].off ==
		       offsetof(struct virtio_pci_common_cfg,
				device_feature_select));
	UK_TEST_EXPECT(io.write[0].val == 0);
	UK_TEST_EXPECT(io.write[1].off ==
		       offsetof(struct virtio_pci_common_cfg,
				device_feature_select));
	UK_TEST_EXPECT(io.write[1].val == 1);

	memset(&io, 0, sizeof(io));
	virtio_pci_test_region(&vpdev.common_cfg, &io);
	io.device_feature[0] = 0xa5a5;
	io.device_feature[1] = 1;
	vpdev.common_cfg.test_read32 = virtio_pci_test_read32;
	vpdev.common_cfg.test_write32 = virtio_pci_test_write32;
	vpdev.vdev.features = UK_BIT_ULL(VIRTIO_F_VERSION_1) | 0xa5a5;
	vpci_modern_features_set(&vpdev.vdev);
	UK_TEST_EXPECT(vpdev.vdev.features ==
			       (UK_BIT_ULL(VIRTIO_F_VERSION_1) | 0xa5a5));
	UK_TEST_EXPECT(vpdev.features_written);
	UK_TEST_EXPECT(io.writes == 6);
	UK_TEST_EXPECT(io.write[0].off ==
			       offsetof(struct virtio_pci_common_cfg,
					device_feature_select));
	UK_TEST_EXPECT(io.write[0].val == 0);
	UK_TEST_EXPECT(io.write[1].off ==
			       offsetof(struct virtio_pci_common_cfg,
					device_feature_select));
	UK_TEST_EXPECT(io.write[1].val == 1);
	UK_TEST_EXPECT(io.write[2].off ==
			       offsetof(struct virtio_pci_common_cfg,
					driver_feature_select));
	UK_TEST_EXPECT(io.write[2].val == 0);
	UK_TEST_EXPECT(io.write[3].off ==
			       offsetof(struct virtio_pci_common_cfg,
					driver_feature));
	UK_TEST_EXPECT(io.write[3].val == 0xa5a5);
	UK_TEST_EXPECT(io.write[4].off ==
			       offsetof(struct virtio_pci_common_cfg,
					driver_feature_select));
	UK_TEST_EXPECT(io.write[4].val == 1);
	UK_TEST_EXPECT(io.write[5].off ==
			       offsetof(struct virtio_pci_common_cfg,
					driver_feature));
	UK_TEST_EXPECT(io.write[5].val == 1);

	memset(&io, 0, sizeof(io));
	memset(&vpdev, 0, sizeof(vpdev));
	io.device_feature[1] = 1;
	virtio_pci_test_region(&vpdev.common_cfg, &io);
	vpdev.common_cfg.test_read8 = virtio_pci_test_read8;
	vpdev.common_cfg.test_write8 = virtio_pci_test_write8;
	vpdev.common_cfg.test_read32 = virtio_pci_test_read32;
	vpdev.common_cfg.test_write32 = virtio_pci_test_write32;
	vpdev.vdev.cops = &vpci_modern_ops;
	vpdev.vdev.features = UK_BIT_ULL(VIRTIO_F_VERSION_1) | 1;
	vpci_modern_features_set(&vpdev.vdev);
	UK_TEST_EXPECT(!vpdev.features_written);
	rc = virtio_dev_status_update(&vpdev.vdev,
				      VIRTIO_CONFIG_STATUS_FEATURES_OK);
	UK_TEST_EXPECT(rc == -EINVAL);
	UK_TEST_EXPECT(!(io.device_status & VIRTIO_CONFIG_STATUS_FEATURES_OK));
}

UK_TESTCASE(virtio_pci_modern, queue_requires_completed_features)
{
	struct virtio_pci_test_io io;
	struct virtio_pci_dev vpdev;
	struct virtqueue live_vq;
	struct virtqueue *vq;
	__u64 notify_addr = 0x2000;
	__u16 qdesc_size = 0;

	memset(&io, 0, sizeof(io));
	memset(&vpdev, 0, sizeof(vpdev));
	io.num_queues = 1;
	io.queue_size[0] = 8;
	virtio_pci_test_region(&vpdev.common_cfg, &io);
	vpdev.common_cfg.test_read8 = virtio_pci_test_read8;
	vpdev.common_cfg.test_read16 = virtio_pci_test_read16;
	vpdev.common_cfg.test_write16 = virtio_pci_test_write16;
	vpdev.notify_addrs = &notify_addr;
	vpdev.notify_addrs_count = 1;

	UK_TEST_EXPECT(vpci_modern_vq_find(&vpdev.vdev, 1, &qdesc_size) ==
		       -EINVAL);
	vq = vpci_modern_vq_setup(&vpdev.vdev, 0, 8, NULL, NULL);
	UK_TEST_EXPECT(PTRISERR(vq));
	UK_TEST_EXPECT(PTR2ERR(vq) == -EINVAL);

	vpdev.features_written = 1;
	io.device_status = VIRTIO_CONFIG_STATUS_FEATURES_OK;
	memset(&live_vq, 0, sizeof(live_vq));
	UK_TAILQ_INIT(&vpdev.vdev.vqs);
	UK_TAILQ_INSERT_TAIL(&vpdev.vdev.vqs, &live_vq, next);
	UK_TEST_EXPECT(vpci_modern_vq_find(&vpdev.vdev, 1, &qdesc_size) ==
		       -EBUSY);
	UK_TAILQ_REMOVE(&vpdev.vdev.vqs, &live_vq, next);
	vq = vpci_modern_vq_setup(&vpdev.vdev, 1, 8, NULL, NULL);
	UK_TEST_EXPECT(PTRISERR(vq));
	UK_TEST_EXPECT(PTR2ERR(vq) == -EINVAL);
}

UK_TESTCASE(virtio_pci_modern, queue_address_write64_order)
{
	struct virtio_pci_test_io io;
	struct virtio_pci_region region;

	memset(&io, 0, sizeof(io));
	virtio_pci_test_region(&region, &io);
	region.test_write32 = virtio_pci_test_write32;

	vpci_region_write64(&region,
			    offsetof(struct virtio_pci_common_cfg, queue_desc),
			    0x1122334455667788ULL);
	UK_TEST_EXPECT(io.writes == 2);
	UK_TEST_EXPECT(io.write[0].off ==
		       offsetof(struct virtio_pci_common_cfg, queue_desc));
	UK_TEST_EXPECT(io.write[0].val == 0x55667788);
	UK_TEST_EXPECT(io.write[1].off ==
		       offsetof(struct virtio_pci_common_cfg, queue_desc) +
		       sizeof(__u32));
	UK_TEST_EXPECT(io.write[1].val == 0x11223344);
}

UK_TESTCASE(virtio_pci_modern, config_get64)
{
	struct virtio_pci_test_io common_io;
	struct virtio_pci_test_io device_io;
	struct virtio_pci_dev vpdev;
	__u64 val = 0;

	memset(&common_io, 0, sizeof(common_io));
	memset(&device_io, 0, sizeof(device_io));
	memset(&vpdev, 0, sizeof(vpdev));
	common_io.config_generation = 7;
	device_io.config[0] = 0x55667788;
	device_io.config[1] = 0x11223344;
	virtio_pci_test_region(&vpdev.common_cfg, &common_io);
	vpdev.common_cfg.test_read8 = virtio_pci_test_read8;
	virtio_pci_test_region(&vpdev.device_cfg, &device_io);
	vpdev.device_cfg.length = sizeof(val);
	vpdev.device_cfg.test_read32 = virtio_pci_test_config_read32;

	UK_TEST_EXPECT(vpci_modern_config_get(&vpdev.vdev, 0, &val,
					      sizeof(val),
					      1) == 0);
	UK_TEST_EXPECT(val == 0x1122334455667788ULL);
}

UK_TESTCASE(virtio_pci_modern, probe_regressions)
{
	struct virtio_pci_modern_caps caps;
	struct virtio_pci_dev vpdev;
	struct pci_bar bars[PCI_BAR_COUNT];
	struct pci_device pdev;
	struct virtio_pci_test_io common_io;
	__u8 cfg[256];
	__u8 bar_mem[0x1000];
	__u16 *msix_vector;
	int rc;

	virtio_pci_test_valid_cfg(cfg, bars);
	UK_TEST_EXPECT(virtio_pci_modern_parse_caps_raw(cfg, bars,
							&caps) == 0);
	memset(bar_mem, 0, sizeof(bar_mem));
	memset(&vpdev, 0, sizeof(vpdev));
	virtio_pci_test_pdev(&pdev, 0x1041, 0, PCI_BAR_MEM, bar_mem);
	virtio_pci_test_modern_rc = 0;
	virtio_pci_test_caps = &caps;
	rc = virtio_pci_probe_transport(&pdev, &vpdev,
					virtio_pci_test_modern_add_dev);
	UK_TEST_EXPECT(rc == 0);
	UK_TEST_EXPECT(vpdev.transport == VIRTIO_PCI_TRANSPORT_MODERN);
	UK_TEST_EXPECT(vpdev.vdev.id.virtio_device_id == 1);
	msix_vector = (__u16 *)(bar_mem + caps.common.offset +
			offsetof(struct virtio_pci_common_cfg,
				 config_msix_vector));
	UK_TEST_EXPECT(*msix_vector == 0);

	memset(&common_io, 0, sizeof(common_io));
	common_io.device_status = 0xff;
	virtio_pci_test_region(&vpdev.common_cfg, &common_io);
	vpdev.common_cfg.test_read8 = virtio_pci_test_read8;
	vpdev.common_cfg.test_write8 = virtio_pci_test_write8;
	UK_TEST_EXPECT(vpdev.vdev.cops->device_reset(&vpdev.vdev) == 0);
	UK_TEST_EXPECT(common_io.device_status == 0);
	common_io.reject_features_ok = 1;
	vpdev.vdev.features = UK_BIT_ULL(VIRTIO_F_VERSION_1);
	vpdev.features_written = 1;
	rc = virtio_dev_status_update(&vpdev.vdev,
				      VIRTIO_CONFIG_STATUS_ACK |
				      VIRTIO_CONFIG_STATUS_DRIVER |
				      VIRTIO_CONFIG_STATUS_FEATURES_OK);
	UK_TEST_EXPECT(rc == -EINVAL);

	memset(bar_mem, 0, sizeof(bar_mem));
	memset(&vpdev, 0, sizeof(vpdev));
	virtio_pci_test_pdev(&pdev, 0x1000, 7, PCI_BAR_MEM, bar_mem);
	rc = virtio_pci_probe_transport(&pdev, &vpdev,
					virtio_pci_test_modern_add_dev);
	UK_TEST_EXPECT(rc == 0);
	UK_TEST_EXPECT(vpdev.transport == VIRTIO_PCI_TRANSPORT_MODERN);
	UK_TEST_EXPECT(vpdev.vdev.id.virtio_device_id == 7);

	memset(&vpdev, 0, sizeof(vpdev));
	virtio_pci_test_pdev(&pdev, 0x1000, 8, PCI_BAR_IO, bar_mem);
	virtio_pci_test_modern_rc = -ENODEV;
	virtio_pci_test_caps = NULL;
	rc = virtio_pci_probe_transport(&pdev, &vpdev,
					virtio_pci_test_modern_add_dev);
	UK_TEST_EXPECT(rc == 0);
	UK_TEST_EXPECT(vpdev.transport == VIRTIO_PCI_TRANSPORT_LEGACY);
	UK_TEST_EXPECT(vpdev.vdev.id.virtio_device_id == 8);

	memset(&vpdev, 0, sizeof(vpdev));
	vpdev.common_cfg.base = 0xdeadbeef;
	virtio_pci_test_pdev(&pdev, 0x1000, 9, PCI_BAR_IO, bar_mem);
	virtio_pci_test_modern_rc = -EINVAL;
	rc = virtio_pci_probe_transport(&pdev, &vpdev,
					virtio_pci_test_modern_add_dev);
	UK_TEST_EXPECT(rc == 0);
	UK_TEST_EXPECT(vpdev.transport == VIRTIO_PCI_TRANSPORT_LEGACY);
	UK_TEST_EXPECT(vpdev.vdev.id.virtio_device_id == 9);
	UK_TEST_EXPECT(vpdev.common_cfg.base == 0);

	memset(&vpdev, 0, sizeof(vpdev));
	virtio_pci_test_pdev(&pdev, 0x1041, 0, PCI_BAR_MEM, bar_mem);
	virtio_pci_test_modern_rc = -EINVAL;
	rc = virtio_pci_probe_transport(&pdev, &vpdev,
					virtio_pci_test_modern_add_dev);
	UK_TEST_EXPECT(rc == -EINVAL);
	UK_TEST_EXPECT(vpdev.vdev.cops != &vpci_legacy_ops);

	memset(&vpdev, 0, sizeof(vpdev));
	virtio_pci_test_pdev(&pdev, 0x1000, 10, PCI_BAR_NONE, NULL);
	pdev.base = 0;
	virtio_pci_test_modern_rc = -ENODEV;
	rc = virtio_pci_probe_transport(&pdev, &vpdev,
					virtio_pci_test_modern_add_dev);
	UK_TEST_EXPECT(rc == -EINVAL);

	memset(&vpdev, 0, sizeof(vpdev));
	virtio_pci_test_pdev(&pdev, 0x1000, 11, PCI_BAR_IO, bar_mem);
	virtio_pci_test_modern_rc = -ENOMEM;
	rc = virtio_pci_probe_transport(&pdev, &vpdev,
					virtio_pci_test_modern_add_dev);
	UK_TEST_EXPECT(rc == -ENOMEM);
	UK_TEST_EXPECT(vpdev.vdev.cops != &vpci_legacy_ops);

	virtio_pci_test_caps = NULL;
	virtio_pci_test_modern_rc = 0;
}

uk_testsuite_late_prio(virtio_pci_modern, NULL,
		       UK_PRIO_BEFORE(UK_PRIO_LATEST));
