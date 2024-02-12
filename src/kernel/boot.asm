extern __higherhalf_stubentry

global pml4t
global pdpt
global pdt
global global_gdt_ptr_high
global global_stack_top
global start
section .bss
align 4096
; since we are going to do 2mb mapping initially, we will not need a PT and our page tables will only be 3 levels
pml4t:
    resb 4096
pdpt:
    resb 4096
pdt:
    resb 4096

stack_bottom:
    resb 128
stack_top:

section .rodata
; Higher half virtual address
HH_VA equ 0xFFFFFFFF80000000 
; Access bits
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
        dq GDT-HH_VA
gdt_ptr_high:
        dw gdt_ptr - GDT - 1
        dq GDT

section .data
global_gdt_ptr_high:
    dq gdt_ptr_high
global_stack_top:
    dq stack_top




section .text
[bits 32]


check_multiboot:
    cmp eax, 0x36d76289
    jne no_multiboot
    ret
no_multiboot:
    mov al, "0"
    jmp error


; Check if CPUID is supported by attempting to flip the ID bit (bit 21) in
; the FLAGS register. If we can flip it, CPUID is available.
check_cpuid:
    
    ; Copy FLAGS in to EAX via stack
    pushfd
    pop eax
    ; Copy to ECX as well for comparing later on
    mov ecx, eax
    ; Flip the ID bit
    xor eax, 1 << 21
    ; Copy EAX to FLAGS via the stack
    push eax
    popfd
    ; Copy FLAGS back to EAX (with the flipped bit if CPUID is supported)
    pushfd
    pop eax
 
    ; Restore FLAGS from the old version stored in ECX (i.e. flipping the ID bit
    ; back if it was ever flipped).
    push ecx
    popfd
 
    ; Compare EAX and ECX. If they are equal then that means the bit wasn't
    ; flipped, and CPUID isn't supported.
    xor eax, ecx
    jz NoCPUID
    ret
NoCPUID:
    mov al, "1"
    jmp error


check_longmode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb no_longmode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz no_longmode
    ret
no_longmode:
    mov al, "2"
    jmp error












start: 
    mov esp, stack_top - HH_VA
    call check_multiboot
    call check_cpuid
    call check_longmode
    

    ; using 2mb pages for our direct mapping will allow us to direct map the first 1gb of the kernel
pagetable_setup:

    ; set pml4t entry to point to pdpt
    mov eax, pdpt - HH_VA
    or eax, 0b11        ; set write and present bit
    mov [pml4t-HH_VA], eax
    
    ; set pdpt entry to point to pdt
    mov eax, pdt-HH_VA
    or eax, 0b11
    mov [pdpt-HH_VA], eax

    xor ecx, ecx

; we map a total of 1gb=1024mb
direct_map_pdte:    
    mov eax, 0x200000           ; 2mb
    mul ecx                     ; eax = 2mb*ecx, this is the mapped address
    or eax, 0b10000011          ; present + writable + huge
    mov [pdt-HH_VA + ecx * 8], eax    ; pdt[ecx] = eax
    inc ecx
    cmp ecx, 512
    jne direct_map_pdte

; now that pages are created we move the top level directory to cr3
    mov eax, pml4t-HH_VA
    mov cr3, eax
    
enable_pae_bit:
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

set_efer_msr:
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    
    
enable_paging:
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    


load_gdt:
    lgdt [gdt_ptr - HH_VA] 
    
    jmp GDT.Code:enter_longmode_success - HH_VA
    jmp error
[bits 64]
enter_longmode_success:
                              ; Clear the interrupt flag.
    mov ax, GDT.Data            ; set data segments
    mov ds, ax                    
    mov es, ax                    
    mov fs, ax                    
    mov gs, ax                    
    mov ss, ax                    
    mov rax, 0x2f592f412f4b2f4f

bp_test:
    mov qword [0xb8000], rax
    

setup_higher_half:

    mov rax, __higherhalf_stubentry
    sub rax, HH_VA
    call rax
    
    mov rax, 0x2f592f412f4b2f4f
    mov qword [0xb8000], rax
    jmp $
    



[bits 32]
error:
    mov dword [0xb8000], 0x4f524f45
    mov dword [0xb8004], 0x4f3a4f52
    mov dword [0xb8008], 0x4f204f20
    mov byte  [0xb800a], al
    cli
    jmp $