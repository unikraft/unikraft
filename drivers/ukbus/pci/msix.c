#include <uk/bus/msix.h>

#include <uk/plat/common/cpu.h>
#include <uk/bus/pci.h>
#include <uk/intctlr.h>

size_t pci_msix_table_size(struct pci_device *dev)
{
	uint32_t table_size;
	if (dev->msix_cap_offset == 0)
		return 0;

	PCI_CONF_READ_HEADER(uint32_t, &table_size,
			     dev->config_addr | dev->msix_cap_offset,
			     MSIX_TBL_SIZE);
	return table_size + 1;
}

uint8_t pci_msix_table_bar(struct pci_device *dev)
{
	uint32_t mtab;
	if (dev->msix_cap_offset == 0)
		return UINT8_MAX;

	PCI_CONF_READ_HEADER(uint32_t, &mtab,
			     dev->config_addr | dev->msix_cap_offset,
			     MSIX_TABLE_BAR);
	return mtab;
}

uint32_t pci_msix_table_offset(struct pci_device *dev)
{
	uint32_t table_offset;
	if (dev->msix_cap_offset == 0)
		return 0;

	PCI_CONF_READ_HEADER(uint32_t, &table_offset,
			     dev->config_addr | dev->msix_cap_offset,
			     MSIX_TABLE_OFFSET);
	return table_offset;
}

static char *pci_get_bar(struct pci_device *dev, uint16_t idx)
{
	uint32_t addr, bar_low, bar_high;
	char *base;

	addr = dev->config_addr;

	PCI_CONF_READ_OFFSET(uint32_t, &bar_low, addr,
			     PCI_BASE_ADDRESS_0 + idx * 4, 0, UINT32_MAX);
	if (bar_low & 0x1) {
		/* The spec also seems to forbid I/O space BARs for MSI-X */
		uk_pr_err("I/O Space BARs are not supported\n");
		return NULL;
	}

	switch ((bar_low >> 1) & 0x3) {
	case 0x0:
		/* 32-bit base address */
		base = (char *)((uintptr_t)bar_low & ~0xf);
		break;
	case 0x2:
		/* 64-bit base address */
		PCI_CONF_READ_OFFSET(uint32_t, &bar_high, PCI_ENABLE_BIT | addr,
				     PCI_BASE_ADDRESS_0 + (idx + 1) * 4, 0,
				     UINT32_MAX);
		base = (char *)((uintptr_t)(bar_low & ~0xf)
				| ((uintptr_t)bar_high << 32));
		break;
	default:
		return NULL;
	}


	return base;
}

static int pci_msix_set_entry(struct pci_device *dev, uint16_t entry_id,
			      struct pci_msix_table_entry *entry)
{
	uint32_t msix_table_size, msix_bar, msix_table_offset;
	struct pci_msix_table_entry *msix_table;
	char *base;

	if (dev->msix_cap_offset == 0)
		return ENOENT;

	msix_table_offset = pci_msix_table_offset(dev);
	msix_bar = pci_msix_table_bar(dev);
	msix_table_size = pci_msix_table_size(dev);
	if (entry_id >= msix_table_size)
		return EINVAL;

	base = pci_get_bar(dev, msix_bar);
	if (base == NULL)
		return ENOTSUP;

	msix_table =
	    (struct pci_msix_table_entry *)(base + msix_table_offset);

	msix_table[entry_id] = *entry;
	barrier();

	return 0;
}

int pci_msix_enable(struct pci_device *dev, unsigned int *irqs, uint16_t count)
{
	int i, rc = 0;
	struct pci_msix_table_entry entry;

	if (dev->msix_cap_offset == 0)
		return ENOENT;

	/* Allocate vectors */
	rc = uk_intctlr_irq_alloc(irqs, count);
	if (rc)
		return rc;

	/* Activate MSI-X */
	PCI_CONF_WRITE_HEADER(uint32_t, dev->config_addr | dev->msix_cap_offset,
			      MSIX_ENABLE, 1);

	/* Configure table entries */
	for (i = 0; i < count; i++) {
		ukplat_pci_msix_setup_table_entry(&entry, irqs[i]);
		rc = pci_msix_set_entry(dev, i, &entry);
		if (rc) {
			uk_pr_err("unable to setup MSI-X entry: %d\n", rc);
			goto err;
		}
	}

	return 0;
err:
	memset(&entry, 0, sizeof(entry));
	for (; i > 0; i--) {
		pci_msix_set_entry(dev, i, &entry);
	}

	uk_intctlr_irq_free(irqs, count);

	return rc;
}

int pci_msix_disable(struct pci_device *dev, unsigned int *irqs, uint16_t count)
{
	int rc, i;
	struct pci_msix_table_entry entry;

	/* Disable MSI-X */
	PCI_CONF_WRITE_HEADER(uint32_t, dev->config_addr | dev->msix_cap_offset,
			      MSIX_ENABLE, 0);

	/* Reset table entries */
	memset(&entry, 0, sizeof(entry));
	for (i = 0; i < count; i++) {
		ukplat_pci_msix_setup_table_entry(&entry, irqs[i]);
		rc = pci_msix_set_entry(dev, i, &entry);
		if (rc)
			return rc;
	}

	return uk_intctlr_irq_free(irqs, count);
}
