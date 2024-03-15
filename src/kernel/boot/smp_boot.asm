


HH_VA           equ             0xFFFFFFFF80000000 
SMP_INFO_STRUCT equ             0x7000
SMP_INIT_ADDR   equ             0x2000
OFFSET_GDT      equ             0x0
OFFSET_PML4T    equ             0x4
OFFSET_KSTACK   equ             0x8
OFFSET_ENTRY    equ             0x10
OFFSET_MAGIC    equ             0x18
OFFSET_32       equ             0x1C
OFFSET_64       equ             0x20
SMP_INFO_MAGIC  equ             0x6969
global smp_init
global smp_init_end
global smp_32bit_init
global smp_64bit_init
extern pml4t


SMP_TRAMPOLINE_ENTRY64 equ (smp_64bit_init - smp_init + SMP_INIT_ADDR)
PTR_GDT equ (GDT-smp_init+SMP_INIT_ADDR)
PML4T_PHYSICAL equ (pml4t - HH_VA)


; WE COPY ALL OF THIS TO PHYSICAL ADDRESS 0x2000
section .text
[bits 16]
smp_init:
    cli
    cld
    ; we need to set up a stack here

    ;mov dword [SMP_INFO_STRUCT + OFFSET_MAGIC], SMP_INFO_MAGIC  
    mov eax, cr4
    or eax, 1 << 5  ; Set PAE bit
    mov cr4, eax

    mov eax, dword [SMP_INFO_STRUCT + OFFSET_PML4T]
    mov cr3, eax
    
    mov ecx, 0xC0000080 ; EFER Model Specific Register
    rdmsr               ; Read from the MSR 
    or eax, 1 << 8
    ;or eax, 1 ; syscall enable
    wrmsr

    mov eax, cr0
    or eax, 0x80000001 ; Paging, Protected Mode
    mov cr0, eax
    lgdt [gdt_ptr-smp_init+SMP_INIT_ADDR]
    

    jmp 0x8:(SMP_TRAMPOLINE_ENTRY64)
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

    mov rsp, [SMP_INFO_STRUCT + OFFSET_KSTACK]
    call [HH_VA + SMP_INFO_STRUCT + OFFSET_ENTRY] ; SMP entry point
    hlt
    jmp $

smp_init_end:

PRESENT        equ 1 << 7
NOT_SYS        equ 1 << 4
EXEC           equ 1 << 3
DC             equ 1 << 2
RW             equ 1 << 1
ACCESSED       equ 1 << 0
 
; Flags bits
GRAN_4K       equ 1 << 7
SZ_32         equ 1 << 6
LONG_MODE     equ 1 << 5

; local copy because I am lazy and tired of arithmetic, a few bytes never hurt anybody
GDT:
    .Null: equ $ - GDT
        dq 0
    .Code: equ $ - GDT
        dd 0xFFFF                                   ; Limit & Base (low, bits 0-15)
        db 0                                        ; Base (mid, bits 16-23)
        db PRESENT | NOT_SYS | EXEC | RW            ; Access
        db GRAN_4K | LONG_MODE | 0xF                ; Flags & Limit (high, bits 16-19)
        db 0                                        ; Base (high, bits 24-31)
    .Data: equ $ - GDT
        dd 0xFFFF                                   ; Limit & Base (low, bits 0-15)
        db 0                                        ; Base (mid, bits 16-23)
        db PRESENT | NOT_SYS | RW                   ; Access
        db GRAN_4K | SZ_32 | 0xF                    ; Flags & Limit (high, bits 16-19)
        db 0                                        ; Base (high, bits 24-31)
    .TSS: equ $ - GDT
        dd 0x00000068
        dd 0x00CF8900
gdt_ptr:
        dw $ - GDT - 1
        dq PTR_GDT