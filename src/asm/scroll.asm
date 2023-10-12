    assume adl=1

    section .text

    include 'include/ti84pceg.inc'

    public _scrollUp

_scrollUp:
    ld iy, 3
    add iy, sp
    ld hl, (iy + 12)
    ld a, h
    or a, l
    ret z
    ld de, (iy)
    ld hl, -ti.lcdWidth
    add hl, de
    ret c
    ld de, (iy + 3)
    ld hl, -ti.lcdHeight
    add hl, de
    ret c
    ld hl, (iy)
    ld de, (iy + 6)
    add hl, de
    ex de, hl
    ld hl, -ti.lcdWidth
    add hl, de
    jr nc, .goodXBounds
    ex de, hl
    ld hl, (iy + 6)
    or a, a
    sbc hl, de
    ld (iy + 6), hl

.goodXBounds:
    ld hl, (iy + 3)
    ld de, (iy + 9)
    add hl, de
    ex de, hl
    ld hl, -ti.lcdHeight
    add hl, de
    jr nc, .goodYBounds
    ex de, hl
    ld hl, (iy + 9)
    or a, a
    sbc hl, de
    ld (iy + 9), hl

.goodYBounds:
    ld hl, (iy + 6)
    ld a, h
    or a, l
    ret z
    ld hl, (iy + 9)
    ld a, h
    or a, l
    ret z
    ld a, (iy + 3)
    call .getVRAMaddr
    push hl
    ld hl, (iy + 12)
    ld de, (iy + 9)
    or a, a
    sbc hl, de
    pop hl
    ret nc
    push hl
    ld a, (iy + 12)
    add a, (iy + 3)
    call .getVRAMaddr
    ld bc, (iy + 6)
    dec bc
    ld a, b
    or a, c
    jr nz, $ + 8
    ld a, $A0
    ld (.smcLDI), a
    inc bc
    ld a, (iy + 9)
    sub a, (iy + 12)
    pop de

.shiftScreen:
    push bc
    push hl
    push de
    ldir
.smcLDI := $ - 1
    pop hl
    pop de
    ld bc, ti.lcdWidth
    add hl, bc
    ex de, hl
    add hl, bc
    pop bc
    dec a
    jr nz, .shiftScreen
    ret

.getVRAMaddr:
    ld e, a
    ld bc, (iy)
    ld d, ti.lcdWidth / 2
    mlt de
    ld hl, (ti.mpLcdLpbase)
    add hl, bc
    add hl, de
    add hl, de
    ret
