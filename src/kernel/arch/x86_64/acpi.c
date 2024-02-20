#include <acpi.h>
#include <kernel.h>
#include <string.h>
#include <serial.h>
#include <apic.h>

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


MADT* retrieveMADT(bool use_xsdt, void* sdp)
{
    XSDT* xsdt = (XSDT*)(((XSDP_t*)sdp)->XsdtAddress + KERNEL_HH_START);
    RSDT* rsdt = (RSDT*)(((RSDP_t*)sdp)->RsdtAddress + KERNEL_HH_START);
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
        madt = (ACPISDTHeader*)((use_xsdt ? xsdt->sdtAddresses[i] + KERNEL_HH_START: (uint64_t)rsdt->sdtAddresses[i]) + KERNEL_HH_START);
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
            /*
            log_to_serial("Item->type: ");
            log_int_to_serial(item->type);
            log_to_serial(" item index: ");
            log_int_to_serial((int)item_index);
            log_to_serial(" curr index: ");
            log_int_to_serial((int)curr_index);
            log_to_serial("\n");
            */
            if (item_index == curr_index)
                return item;
            curr_index++;
        }
        
        item = (MADT_ITEM*)((uint64_t)item + (uint64_t)item->record_length);
        curr_length += item->record_length;
    }
    return (void*)0x0;
}