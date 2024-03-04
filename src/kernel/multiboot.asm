section .multiboot
header_start:
align 8
    dd 0xe85250d6                ; magic number (multiboot 2)
    dd 0                         ; architecture 0 (protected mode i386)
    dd header_end - header_start ; header length
    ; checksum
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))
framebuffer_tag_start:
    dw  0x05    ;Type: framebuffer
    dw  0x01    ;Optional tag
    dd  framebuffer_tag_end - framebuffer_tag_start ;size
    dd  0   ;Width - if 0 we let the bootloader decide
    dd  0   ;Height - same as above
    dd  0   ;Depth  - same as above
framebuffer_tag_end:
align 8
    ; required end tag
    dw 0    ; type
    dw 0    ; flags
    dd 8    ; size
header_end:

