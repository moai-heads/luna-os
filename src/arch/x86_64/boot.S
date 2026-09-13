.section .text
.global _start
.type _start, @function
.extern kmain

_start:
    cli
    lea stack_top(%rip), %rsp
    andq $-16, %rsp
    call kmain

.hang:
    hlt
    jmp .hang

.section .bss
.align 16
stack_bottom:
    .skip 65536
stack_top:
