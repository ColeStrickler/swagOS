target remote localhost:1234
symbol-file myos.bin

b isr_errorcode_8
b isr_errorcode_10
b isr_errorcode_11
b isr_errorcode_12
b isr_errorcode_13
b isr_errorcode_14
b isr_0
b isr_32



set output-radix 16
layout regs
layout split
layout regs
layout src


define q
    stepi
    x/4x $rsp
    x/10i $pc
end

define w
    ni 1
    x/4x $rsp
    x/10i $pc
end