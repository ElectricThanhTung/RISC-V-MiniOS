
.section .text.vector_handler
.global SW_Handler
.balign 4
SW_Handler:
    addi    sp, sp, -(4 * 4)
    csrr    a5, mepc
    sw      a5, 0(sp)
    sw      s1, 4(sp)
    sw      fp, 8(sp)
    sw      tp, 12(sp)

    mv      a0, sp
    jal     MiniOS_NextStack
    mv      sp, a0

    lw      a5, 0(sp)
    csrw    mepc, a5
    lw      s1, 4(sp)
    lw      fp, 8(sp)
    lw      tp, 12(sp)
    addi    sp, sp, (4 * 4)

    mret

.section .text.MiniOS_SetSP
.global MiniOS_SetSP
.balign 4
MiniOS_SetSP:
    mv      sp, a0
    ret
