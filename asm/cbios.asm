shm$base equ 0fff0h
shm$chrin equ shm$base
shm$cmd equ shm$base + 1
shm$data equ shm$base + 2
shm$track equ shm$base + 4
shm$sect equ shm$base + 6
shm$dma equ shm$base + 8
cmd$return equ 0
cmd$chrout equ 1
cmd$strout equ 2
cmd$readsec equ 4
cmd$writesec equ 5
cmd$seldisk equ 6

;	CBIOS for Pi85 System
;
msize	equ	62	;cp/m version memory size in kilobytes
				; TODO: 64 here puts the BIOS at FA00. Need to relocate CP/M?
				; http://obsolescence.wixsite.com/obsolescence/cpm-internals says we should be at F200
;
;	"bias" is address offset from 3400H for memory systems
;	than 16K (referred to as "b" throughout the text).
;
bias	equ	(msize-20)*1024
ccp	equ	3400H+bias	;base of ccp
bdos	equ	ccp+806h	;base of bdos
bios	equ	ccp+1600h	;base of bios
cdisk	equ	0004H	;current disk number 0=A,...,15=P
iobyte	equ	0003h	;intel i/o byte
;
	org	bios	;origin of this program
nsects	equ	($-ccp)/128	;warm start sector count
;
;	jump vector for individual subroutines
	jmp	boot		;cold start
wboote:	jmp	wboot		;warm start
	jmp	const		;console status
	jmp	conin		;console character in
	jmp	conout		;console character out
	jmp	list		;list character out
	jmp	punch		;punch character out
	jmp	reader		;reader character out
	jmp	home		;move head to home position
	jmp	seldsk		;select disk
	jmp	settrk		;set track number
	jmp	setsec		;set sector number
	jmp	setdma		;set dma address
	jmp	read		;read disk
	jmp	write		;write disk
	jmp	listst		;return list status
	jmp	sectran		;sector translate
;
;	4x 8MB removable drive devices
;	disk parameter header for disk 00
dpbase:	dw	0000H,0000H		; storage medium is a flat file
	dw	0000H,0000H
	dw	dirbf,dpblk
	dw	chk00,all00
;	disk parameter header for disk 01
	dw	0000H,0000H
	dw	0000H,0000H
	dw	dirbf,dpblk
	dw	chk01,all01
;	disk parameter header for disk 02
	dw	0000H,0000H
	dw	0000H,0000H
	dw	dirbf,dpblk
	dw	chk02,all02
;	disk parameter header for disk 03
	dw	0000H,0000H
	dw	0000H,0000H
	dw	dirbf,dpblk
	dw	chk03,all03
;
;
dpblk:	;disk parameter block, common to all disks
	dw	32		;sectors per track
	db	5		;block shift factor
	db	31		;block mask
	db	1		;extent mask
	dw	2041	;disk size-1
	dw	1023	;directory max
	db	0ffh	;alloc 0
	db	0		;alloc 1
	dw	256		;check size
	dw	6		;track offset
;
;	end of fixed tables
;
;	individual subroutines to perform each function
boot:	;simplest case is to just perform parameter initialization
	lxi h, signon$str
	call print$str
	
	xra	a			;zero in the accum
	sta	iobyte		;clear the iobyte
	sta	cdisk		;select disk zero
	jmp	gocpm		;initialize and go to cp/m
;
wboot:	;simplest case is to read the disk until all sectors loaded
	lxi	sp,80h		;use space below buffer for stack
	mvi	c,0		;select disk 0
	call	seldsk
	call	home		;go to track 00
;
	mvi	b,nsects	;b counts # of sectors to load
	mvi	c,0		;c has the current track number
	mvi	d,2		;d has the next sector to read
;	note that we begin by reading track 0, sector 2 since sector 1
;	contains the cold start loader, which is skipped in a warm start
	lxi	h,ccp		;base of cp/m (initial load point)
load1:	;load one more sector
	push	b	;save sector count, current track
	push	d	;save next sector to read
	push	h	;save dma address
	mov	c,d	;get sector address to register c
	call	setsec	;set sector address from register c
	pop	b	;recall dma address to b,c
	push	b	;replace on stack for later recall
	call	setdma	;set dma address from b,c
;
;	drive set to 0, track set, sector set, dma address set
	call	read
	cpi	00h	;any errors?
	jnz	loaderr	;abandon the boot if an error occurs
;
;	no error, move to next sector
	pop	h	;recall dma address
	lxi	d,128	;dma=dma+128
	dad	d	;new dma address is in h,l
	pop	d	;recall sector address
	pop	b	;recall number of sectors remaining, and current trk
	dcr	b	;sectors=sectors-1
	jz	gocpm	;transfer to cp/m if all have been loaded
;
;	more sectors remain to load, check for track change
	inr	d
	mov	a,d	;sector=33?, if so, change tracks
	cpi	33
	jc	load1	;carry generated if sector<33
;
;	end of current track, go to next track
	mvi	d,1	;begin with first sector of next track
	inr	c	;track=track+1
;
;	save register state, and change tracks
	push	b
	push	d
	push	h
	mvi b, 0
	call	settrk	;track address set from register c
	pop	h
	pop	d
	pop	b
	jmp	load1	;for another sector
;
;	end of load operation, set parameters and go to cp/m
gocpm:
	mvi	a,0c3h	;c3 is a jmp instruction
	sta	0	;for jmp to wboot
	lxi	h,wboote	;wboot entry point
	shld	1	;set address field for jmp at 0
;
	sta	5	;for jmp to bdos
	lxi	h,bdos	;bdos entry point
	shld	6	;address field of jump at 5 to bdos
;
	lxi	b,80h	;default dma address is 80h
	call	setdma
;
	ei		;enable the interrupt system
	lda	cdisk	;get current disk number
	mov	c,a	;send to the ccp
	jmp	ccp	;go to cp/m for further processing
	
loaderr:
	lxi h, err$str
	call print$str
	hlt
;
;
;	simple i/o handlers (must be filled in by user)
;	in each case, the entry point is provided, with space reserved
;	to insert your own code
;
const:	;console status, return 0ffh if character ready, 00h if not
	lda shm$chrin	; check chrin byte
	ana a
	rz				; 0 means nothing came in
	mvi a, 0ffh
	ret
;
conin:	;console character into register a
	lda shm$chrin	; check chrin byte
	ana a
	jz conin		; wait until something appears there
	mov b,a			; save the byte
	xra a			; a=0
	sta shm$chrin	; zero the chrin byte so we know nothing is there next time
	mov a,b			; recall the byte
	ret
;
conout: ;console character output from register c
	mvi a, cmd$chrout	; chrout command
	sta shm$cmd		; save to command byte
	mov a, c		; get the char to send
	sta shm$data    ; save at data location
	out 0			; set the request flag
conout$w:
	in 0
	ani 1
	jnz conout$w	; wait for request to be processed
	ret
;
list:	;list character from register c
	mov	a,c	;character to register a
	ret		;null subroutine
;
listst:	;return list status (0 if not ready, 1 if ready)
	xra	a	;0 is always ok to return
	ret
;
punch:	;punch character from register c
	mov	a,c	;character to register a
	ret		;null subroutine
;
;
reader: ;read character into register a from reader device
	mvi	a,1ah	;enter end of file for now (replace later)
	ani	7fh	;remember to strip parity bit
	ret
;
;
;	i/o drivers for the disk follow
;	for now, we will simply store the parameters away for use
;	in the read and write subroutines
;
home:	;move to the track 00 position of current drive
;	translate this call into a settrk call with parameter 00
	mvi b,0
	mvi	c,0	;select track 0
	; fall through to settrk
;
settrk:	;set track given by register c
	mov l,c
	mov h,b
	shld shm$track	; save in memory for later
	ret
;
seldsk:	;select disk given by register C
	lxi	h,0000h	;error return code
	mov	a,c
	cpi	4	;must be between 0 and 3
	rnc		;no carry if 4,5,...
;	disk number is in the proper range
	sta shm$data		; track number in data field
	mvi a,cmd$seldisk	; check that we can select this disk
	sta shm$cmd
	out 0				; set request flag
seldsk$w:
	in 0
	ani 1				; wait for it to change
	jnz seldsk$w
	lda shm$data
	ana a				; 0=ok
	rnz
;	compute proper disk parameter header address
	mov	l,c	;L=disk number 0,1,2,3
	mvi	h,0	;high order zero
	dad	h	;*2
	dad	h	;*4
	dad	h	;*8
	dad	h	;*16 (size of each header)
	lxi	d,dpbase
	dad	d	;HL=.dpbase(diskno*16)
	ret
;
setsec:	;set sector given by register c
	mov l,c
	mvi h,0				; pi85 is capable of 16 bit sector- but is cp/m?
	shld shm$sect		; save in memory to be used later
	ret
;
sectran:
	;translate the sector given by BC using the
	;translate table given by DE
	mov h,b
	mov l,c				; don't translate as it doesn't help us here
	inx h
	ret		;with value in HL
;
setdma:	;set dma address given by registers b and c
	mov	l,c	;low order address
	mov	h,b	;high order address
	shld	shm$dma	;save the address
	ret
;
read:	;perform read operation (usually this is similar to write
;	so we will allow space to set up read command, then use
;	common code in write)
	mvi a,cmd$readsec
	jmp	waitio	;to perform the actual i/o
;
write:	;perform a write operation
	mvi a,cmd$writesec
;
waitio:	;enter here from read and write to perform the actual i/o 
;	operation.  return a 00h in register a if the operation completes
;	properly, and 01h if an error occurs during the read or write
;
	sta shm$cmd
	out 0		; by this point we should have already set params, so do the i/o
waitio$w:
	in 0
	ani 1
	jnz waitio$w
	lda shm$data
	ret		;replaced when filled-in
	
print$str:
	shld shm$data		; for cbios to print strings
	mvi a, cmd$strout
	sta shm$cmd
	out 0
print$str$lp:
	in 0
	ani 1
	jnz print$str$lp
	ret
	
signon$str: db '64K CP/M Version 2.2 (Pi85 BIOS 1.0 12/2/17)',13,10,0
err$str: db 'Error loading CP/M system!',0
;
;	the remainder of the CBIOS is reserved uninitialized
;	data area, and does not need to be a part of the
;	system memory image (the space must be available,
;	however, between "begdat" and "enddat").
;
;	scratch ram area for BDOS use
begdat	equ	$	;beginning of data area
dirbf:	ds	128	;scratch directory area
all00:	ds	256	;allocation vector 0
all01:	ds	256	;allocation vector 1
all02:	ds	256	;allocation vector 2
all03:	ds	256	;allocation vector 3
chk00:	ds	256	;check vector 0
chk01:	ds	256	;check vector 1
chk02:	ds	256	;check vector 2
chk03:	ds	256	;check vector 3
;
enddat	equ	$	;end of data area
datsiz	equ	$-begdat;size of data area
	end
