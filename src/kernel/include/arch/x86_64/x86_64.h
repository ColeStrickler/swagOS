/*
    Global settings for x86-64
*/
typedef struct x8664_Settings
{
    bool use_x2_apic;
} x8664_Settings;




/*
    cpuid format is cpuid_xxxx_xxxx_rrr

    where xxxx_xxxx is a 32 bit value that goes into eax and rrr is the register that the value is returned in

*/
/* used to check for APIC support*/
#define CPUID_PROC_FEAT_ID 0x00000001





/*
    Bit selectors for cpuid
*/
#define CPUID_PROC_FEAT_ID_EDX_FPU      1 << 0  // indicates x87 FPU is on chip and available
#define CPUID_PROC_FEAT_ID_EDX_PSE      1 << 3  // indicates page size extensions available
#define CPUID_PROC_FEAT_ID_EDX_MSR      1 << 5  // indicates support for AMD specific MSRs
#define CPUID_PROC_FEAT_ID_EDX_PAE      1 << 6  // indicates support for physical address extensions
#define CPUID_PROC_FEAT_ID_EDX_APIC     1 << 9  // indicates APIC exists and is enabled

#define CPUID_PROC_FEAT_ID_ECX_X2APIC   1 << 21 // indicates x2 APIC available 

/*
    Model Specific Registers(MSRs)
*/
#define IA32_APIC_BASE_MSR 0x1B



/*
    MSR Bit Selectors
*/
#define IA32_APIC_BASE_BSC              1 << 8  // bootstrap core
#define IA32_APIC_BASE_MSR_X2ENABLE     1 << 10 // enable X2APIC
#define IA32_APIC_BASE_MSR_ENABLE       1 << 11 
#define IA32_APIC_BASE_ADDRESS          0xFFFFFFFFFF000ULL