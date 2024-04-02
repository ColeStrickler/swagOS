[bits 64]

extern isr_handler
; please see https://wiki.osdev.org/Interrupt_Service_Routines, https://wiki.osdev.org/Interrupts_Tutorial

%macro isr 1
[global isr_%1]
isr_%1:
    ; This entry stub is called with the following registers already on the stack:
    ; ss, rsp, rflags, cs(padded to make qword), rip  --> in this order
    ; These registers are pushed and popped automatically by the CPU during ISR invocation
    push 0x0                ; we push a dummy error code to maintain stack equivalence with isr_errocode_%1
    push %1                 ; save interrupt # for later use

    save_gpr                ; push all general purpose registers to stack
    mov rdi, rsp            ; with System V ABI the first parameter to a function is in rdi. We pass the stack pointer here
    cld                     ; C code following the sysV ABI requires DF to be clear on function entry
    call isr_handler        
    restore_gpr             ; restore all general purpose registers from stack
    add rsp, 16             ; reclaim both the error code and the interrupt # previously pushed onto the stack
    iretq                    ; interrupt return
%endmacro

%macro isr_errorcode 1
[global isr_errorcode_%1]
isr_errorcode_%1:
    ; This entry stub is called with the following registers already on the stack:
    ; ss, rsp, rflags, cs(padded to make qword), rip  --> in this order
    ; These registers are pushed and popped automatically by the CPU during ISR invocation

    ; push errorcode is already done and does not need to be done here
    push %1                 ; save interrupt # for later use
    save_gpr                ; push all general purpose registers to stack

    mov rdi, rsp            ; with System V ABI the first parameter to a function is in rdi. We pass the stack pointer here
    cld                     ; C code following the sysV ABI requires DF to be clear on function entry
    call isr_handler        
    restore_gpr             ; restore all general purpose registers from stack
    add rsp, 16             ; reclaim both the error code and the interrupt # previously pushed onto the stack
    iretq                    ; interrupt return
%endmacro



%macro save_gpr 0
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro restore_gpr 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

;
; We now use our previously defined macros to declare whichever interrupt routines we want to support 
;

isr 0
isr 1
isr 2
isr 3
isr 4
isr 5
isr 6
isr 7
isr_errorcode 8
isr 9
isr_errorcode 10
isr_errorcode 11
isr_errorcode 12
isr_errorcode 13
isr_errorcode 14
isr 15
isr 16
isr_errorcode 17
isr 18
isr 19
isr 20
isr 21
isr 22
isr 23
isr 24
isr 25
isr 26
isr 27
isr 28
isr 29
isr 30
isr 31
isr 32
isr 33
isr 34
isr 46
isr 255
