#include "msix_x86.h"

#include <string.h>
#include <uk/bus/msix.h>

#define X86_APIC_MSI_BASE_ADDRESS		0xfeeU

static void pci_msix_encode_entry_x86(struct pci_msix_table_entry *entry,
				      struct pci_msix_table_entry_x86 *x86_entry)
{
	memset(entry, 0, sizeof(*entry));
	entry->addr_low =
		X86_APIC_MSI_BASE_ADDRESS << 20 |
		(x86_entry->dest_id & 0xff) << 12 |
		(x86_entry->redirection_hint & 0x1) << 3 |
		(x86_entry->destination_mode & 0x1) << 2;
	entry->msg_data =
		((x86_entry->vector + 32) & 0xff) |
		(x86_entry->delivery_mode & 0x3) << 8 |
		(x86_entry->level & 0x1) << 14 |
		(x86_entry->trigger & 0x1) << 15;
}

void ukplat_pci_msix_setup_table_entry(struct pci_msix_table_entry *entry,
				       unsigned int vector)
{
	struct pci_msix_table_entry_x86 x86_entry = {
		.dest_id = 0,
		.redirection_hint = 0,
		.destination_mode = PCI_MSIX_X86_DEST_MODE_PHYSICAL,
		.vector = vector,
		.delivery_mode = PCI_MSIX_X86_DELIVERY_FIXED,
		.trigger = PCI_MSIX_X86_TRIGGER_EDGE,
	};

	pci_msix_encode_entry_x86(entry, &x86_entry);
}
