#include <acpi.h>
#include <kernel.h>
#include <string.h>
#include <serial.h>
#include <apic.h>
#include <pmm.h>
#include <panic.h>


bool doRSDPChecksum(char *ptr_rsdp, size_t size) {
    uint32_t sum = 0;
    for(int i = 0; i < size; i++) {
        sum += ptr_rsdp[i];
    }
    return (sum & 0xFF) == 0;
}



bool doSDTChecksum(ACPISDTHeader *tableHeader)
{
    
    unsigned char sum = 0;
 
    for (int i = 0; i < tableHeader->Length; i++)
    {
        sum += ((char *) tableHeader)[i];
    }
 
    return sum == 0;
}

extern uint64_t pml4t[512] __attribute__((aligned(0x1000))); 
MADT* retrieveMADT(bool use_xsdt, void* sdp)
{
    uint64_t a1 = ((XSDP_t*)sdp)->XsdtAddress;   
    uint64_t a2 = ((RSDP_t*)sdp)->RsdtAddress;

    XSDT* xsdt = (XSDT*)(a1);
    RSDT* rsdt = (RSDT*)(a2);
    log_hexval("rsdt", rsdt);
    if (a1 > 0x40000000 || a2 > 0x40000000)
    {
        /*
            We will just direct map these frames into the kernel since they are reserved memory anyways
        */
        if (use_xsdt)
            map_kernel_page(HUGEPGROUNDDOWN((uint64_t)xsdt), HUGEPGROUNDDOWN((uint64_t)xsdt));
        else
            map_kernel_page(HUGEPGROUNDDOWN((uint64_t)rsdt), HUGEPGROUNDDOWN((uint64_t)rsdt));

    }
    if (!is_frame_mapped_hugepages(rsdt, pml4t))
    {
        panic("not mapped");
    }
   
    ACPISDTHeader* madt;
    size_t number_of_items = use_xsdt ? (xsdt->sdtHeader.Length - sizeof(ACPISDTHeader)) / 8 : (rsdt->sdtHeader.Length - sizeof(ACPISDTHeader)) / 4;
    if (use_xsdt && !doSDTChecksum((ACPISDTHeader*)xsdt))
    {
        log_to_serial("Could not validate xsdt checksum!\n");
        return (void*)0x0;
    }
    else if (!use_xsdt && !doSDTChecksum((ACPISDTHeader*)rsdt))
    {
        log_to_serial("Could not validate rsdt checksum!\n");
        return (void*)0x0;
    }
    
    for (int i = 0; i < number_of_items; i++)
    {
        log_hex_to_serial(i);
        madt = (ACPISDTHeader*)(use_xsdt ? xsdt->sdtAddresses[i] : (uint64_t)rsdt->sdtAddresses[i]);

        /*
            We will just direct map these frames into the kernel since they are reserved memory anyways
        */
        if (!is_frame_mapped_hugepages(PGROUNDDOWN((uint64_t)madt), pml4t))
        {
            map_kernel_page(PGROUNDDOWN((uint64_t)madt), PGROUNDDOWN((uint64_t)madt));
        }

        if (!memcmp(madt->Signature, "APIC", 4) && doSDTChecksum(madt)) // look for MADT signature
        {
            return (MADT*)madt;
        }
    }
    
    return (void*)0x0;
}

MADT_ITEM* madt_get_item(MADT* madt, uint32_t item_type, uint32_t item_index)
{
    uint32_t total_length = madt->sdtHeader.Length;
    MADT_ITEM* item = (MADT_ITEM*)madt->fields;
    uint32_t curr_length = sizeof(ACPISDTHeader) + 2*sizeof(uint32_t);
    uint32_t curr_index = 0;

    while(curr_length < total_length && curr_index <= item_index)
    {
        if (item->type == item_type)
        {
            if (item_index == curr_index)
                return item;
            curr_index++;
        }
        
        item = (MADT_ITEM*)((uint64_t)item + (uint64_t)item->record_length);
        curr_length += item->record_length;
    }
    return (void*)0x0;
}