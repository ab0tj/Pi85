shm_base equ 0fff0h
shm_chrin equ shm_base
shm_cmd equ shm_base + 1
shm_data equ shm_base + 2
shm_track equ shm_base + 4
shm_sect equ shm_base + 6
shm_dma equ shm_base + 8
cmd_return equ 0
cmd_chrout equ 1
cmd_strout equ 2
cmd_readsec equ 4
cmd_writesec equ 5
cmd_seldisk equ 6

ccp equ 0dc00h
bios equ 0f200h
numsects equ 48

.org 0h

lxi sp, 01000h

xra a
sta shm_data
mvi a, cmd_seldisk
call docmd

mvi d, numsects+1

lxi h, 0
shld shm_track
inx h
inx h
shld shm_sect

lxi h, ccp
shld shm_dma

readlp:
dcr d
jz bios
mvi a, cmd_readsec
call docmd
lhld shm_sect
inx h
shld shm_sect
mov a,l
cpi 33
jnz next
mvi l,1
shld shm_sect
lhld shm_track
inx h
shld shm_track
next:
lhld shm_dma
lxi b, 128
dad b
shld shm_dma
jmp readlp

docmd:
sta shm_cmd
out 0
docmd_w:
in 0
ani 1
jnz docmd_w
ret