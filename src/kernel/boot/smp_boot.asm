
SMP_TRAMPOLINE_ENTRY64 equ (smp_64bit_init - smp_init + SMP_INIT_ADDR)

HH_VA           equ             0xFFFFFFFF80000000 
SMP_INFO_STRUCT equ             0x1000
SMP_INIT_ADDR   equ             0x2000
OFFSET_GDT      equ             0x0
OFFSET_PML4T    equ             0x4
OFFSET_KSTACK   equ             0x8
OFFSET_ENTRY    equ             0x10
OFFSET_MAGIC    equ             0x18
global smp_init
global smp_init_end

; WE COPY ALL OF THIS TO PHYSICAL ADDRESS 0x2000
section .text
[bits 16]
smp_init:
    cli
    ; we need to set up a stack here
    
   
    xor ax, ax

    mov ds, ax
    mov es, ax
    mov ss, ax
    mov cs, ax


    mov dword [SMP_INFO_STRUCT + OFFSET_MAGIC], 0x6969
    hlt
    mov eax, cr4
    or eax, 1 << 5  ; Set PAE bit
    mov cr4, eax

    mov eax, [SMP_INFO_STRUCT + OFFSET_PML4T]
    mov cr3, eax

    ; maybe need to mess with these addresses and add the KERNEL_HH_OFFSET?

    mov eax, cr0
    or eax, 0x80000001 ; Paging, Protected Mode
    mov cr0, eax

    lgdt [SMP_INFO_STRUCT + OFFSET_GDT]

    jmp 0x08:SMP_TRAMPOLINE_ENTRY64
    hlt



[bits 64]
smp_64bit_init:
    ; init data segment registers
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; may also need to check this?
    mov rsp, [HH_VA + SMP_INFO_STRUCT + OFFSET_KSTACK]
    
    call [HH_VA + SMP_INFO_STRUCT + OFFSET_ENTRY]


smp_init_end: