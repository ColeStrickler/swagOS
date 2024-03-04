#include "multiboot.h"

#include "kernel.h"
#include <apic.h>
#include <acpi.h>
#include <panic.h>
extern KernelSettings global_Settings;



RSDP_t* multiboot_retrieve_rsdp(uint64_t addr, bool* extended_rsdp)
{	
	if ((uint64_t)addr & 7)
	{
		log_to_serial("unaligned multiboot info.\n");
		return (void*)0x0;
	}

	struct multiboot_tag *tag;
	unsigned* size = *(unsigned *) addr;
	for (tag = (struct multiboot_tag *) (addr + 8); tag->type != MULTIBOOT_TAG_TYPE_END; tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7)))
	{
		if (tag->type == MULTIBOOT_TAG_TYPE_ACPI_NEW)
		{
			*extended_rsdp = true;
			return (RSDP_t*)((uint64_t)tag + 2*sizeof(uint32_t));
		}
		else if (tag->type == MULTIBOOT_TAG_TYPE_ACPI_OLD)
		{
			return (RSDP_t*)((uint64_t)tag + 2*sizeof(uint32_t));
		}
	}
	return (void*)0x0;
}


bool muiltiboot_retrieve_framebuffer(uint64_t addr)
{	
	if ((uint64_t)addr & 7)
	{
		log_to_serial("unaligned multiboot info.\n");
		return (void*)0x0;
	}

	struct multiboot_tag *tag;
	unsigned* size = *(unsigned *) addr;
	for (tag = (struct multiboot_tag *) (addr + 8); tag->type != MULTIBOOT_TAG_TYPE_END; tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7)))
	{
		if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER)
		{
			global_Settings.framebuffer = (struct multiboot_tag_framebuffer*)tag;
			return true;
		}
	}
	return false;
}






void parse_multiboot_info(uint64_t ptr_multiboot_info)
{
	bool useExtendedRSDP = false;
	MADT* madt_header = NULL;
	io_apic* io_apic_madt = NULL;
	io_apic_int_src_override* int_src_override = NULL;

	void* rsdp = multiboot_retrieve_rsdp(ptr_multiboot_info, &useExtendedRSDP);

	if (!doRSDPChecksum(rsdp, sizeof(RSDP_t)))
	{
		log_to_serial(((RSDP_t*)rsdp)->Signature);
		log_to_serial("Bad RSDP checksum.\n");
	}
	global_Settings.SystemDescriptorPointer = rsdp;

	if (!muiltiboot_retrieve_framebuffer(ptr_multiboot_info))
	{
		panic("parse_multiboot_info() --> could not retrieve framebuffer.\n");
	}

	if (!parse_multiboot_memorymap(ptr_multiboot_info))
	{
		panic("Error parsing multiboot memory map.\n");
	}

	if (!(madt_header = retrieveMADT(useExtendedRSDP, rsdp)))
	{
		panic("MADT not found!\n");
	}
	log_to_serial("searching ioapic\n");
	if (!(io_apic_madt = (io_apic*)madt_get_item(madt_header, MADT_ITEM_IO_APIC, 0)))
	{
		log_to_serial("Could not find IOAPIC entry in MADT.\n");
	}
	global_Settings.pIoApic = io_apic_madt;

	int i = 0;
	
	while ((int_src_override = (io_apic_int_src_override*)madt_get_item(madt_header, MADT_ITEM_IO_APIC_SRCOVERRIDE, i)))
	{
		uint32_t count = global_Settings.settings_x8664.interrupt_override_count;
		global_Settings.settings_x8664.interrupt_overrides[int_src_override->global_sys_int] = int_src_override;
		global_Settings.settings_x8664.interrupt_override_count++;
		i++;
	}
}