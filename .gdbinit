target remote localhost:1234
symbol-file myos.bin

b *0x10160d

set output-radix 16
layout regs
layout split
layout regs


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