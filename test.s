.str_0:
    .string "a = "
.str_1:
    .string "\n"
.str_2:
    .string "b = "
.str_3:
    .string "a + b = "
.str_4:
    .string "a - b = "
.str_5:
    .string "a * b = "
.str_6:
    .string "x is greater than 5\n"
.str_7:
    .string "x is small\n"
.str_8:
    .string "Sum 1 to 5 = "
.str_9:
    .string "j = "

    .text
    .globl  main
    .type   main, @function

main:
    addi    sp, sp, -144
    sd      ra, 136(sp)
    sd      s0, 128(sp)
    addi    s0, sp, 144

    li      t0, 10
    sd      t0, -8(s0)
    li      t0, 3
    sd      t0, -16(s0)
    la      a0, .str_0
    call    printf
    la      a0, fmt_int
    ld      a1, -8(s0)
    call    printf
    la      a0, .str_1
    call    printf
    la      a0, .str_2
    call    printf
    la      a0, fmt_int
    ld      a1, -16(s0)
    call    printf
    la      a0, .str_1
    call    printf
    ld      t0, -8(s0)
    ld      t1, -16(s0)
    add     t2, t0, t1
    sd      t2, -24(s0)
    ld      t0, -24(s0)
    sd      t0, -32(s0)
    la      a0, .str_3
    call    printf
    la      a0, fmt_int
    ld      a1, -32(s0)
    call    printf
    la      a0, .str_1
    call    printf
    ld      t0, -8(s0)
    ld      t1, -16(s0)
    sub     t2, t0, t1
    sd      t2, -40(s0)
    ld      t0, -40(s0)
    sd      t0, -48(s0)
    la      a0, .str_4
    call    printf
    la      a0, fmt_int
    ld      a1, -48(s0)
    call    printf
    la      a0, .str_1
    call    printf
    ld      t0, -8(s0)
    ld      t1, -16(s0)
    mul     t2, t0, t1
    sd      t2, -56(s0)
    ld      t0, -56(s0)
    sd      t0, -64(s0)
    la      a0, .str_5
    call    printf
    la      a0, fmt_int
    ld      a1, -64(s0)
    call    printf
    la      a0, .str_1
    call    printf
    li      t0, 7
    sd      t0, -72(s0)
    ld      t0, -72(s0)
    li      t1, 5
    slt     t2, t1, t0
    sd      t2, -80(s0)
    ld      t0, -80(s0)
    beqz    t0, L1
    la      a0, .str_6
    call    printf
    j       L2
L1:
    la      a0, .str_7
    call    printf
L2:
    li      t0, 1
    sd      t0, -88(s0)
    li      t0, 0
    sd      t0, -96(s0)
L3:
    ld      t0, -88(s0)
    li      t1, 5
    slt     t2, t1, t0
    xori    t2, t2, 1
    sd      t2, -104(s0)
    ld      t0, -104(s0)
    beqz    t0, L4
    ld      t0, -96(s0)
    ld      t1, -88(s0)
    add     t2, t0, t1
    sd      t2, -112(s0)
    ld      t0, -112(s0)
    sd      t0, -96(s0)
    ld      t0, -88(s0)
    addi    t0, t0, 1
    sd      t0, -88(s0)
    j       L3
L4:
    la      a0, .str_8
    call    printf
    la      a0, fmt_int
    ld      a1, -96(s0)
    call    printf
    la      a0, .str_1
    call    printf
    li      t0, 1
    sd      t0, -120(s0)
L5:
    ld      t0, -120(s0)
    li      t1, 4
    slt     t2, t1, t0
    xori    t2, t2, 1
    sd      t2, -128(s0)
    ld      t0, -128(s0)
    beqz    t0, L6
    la      a0, .str_9
    call    printf
    la      a0, fmt_int
    ld      a1, -120(s0)
    call    printf
    la      a0, .str_1
    call    printf
    ld      t0, -120(s0)
    addi    t0, t0, 1
    sd      t0, -120(s0)
    j       L5
L6:
    li      a0, 0
    ld      ra, 136(sp)
    ld      s0, 128(sp)
    addi    sp, sp, 144
    ret
    li      a0, 0
    ld      ra, 136(sp)
    ld      s0, 128(sp)
    addi    sp, sp, 144
    ret
