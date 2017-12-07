;***************************************************
; Read a file into CP/M from the Pi's filesystem
;***************************************************

; PI85 STUFF
SHM$BASE 			EQU 0FFF0H
SHM$CHRIN 			EQU SHM$BASE
SHM$CMD 			EQU SHM$BASE + 1
SHM$DATA 			EQU SHM$BASE + 2
SHM$TRACK 			EQU SHM$BASE + 4
SHM$SECT 			EQU SHM$BASE + 6
SHM$DMA 			EQU SHM$BASE + 8
CMD$RETURN 			EQU 0
CMD$CHROUT 			EQU 1
CMD$STROUT 			EQU 2
CMD$READSEC 		EQU 4
CMD$WRITESEC 		EQU 5
CMD$SELDISK 		EQU 6
CMD$OPENF 			EQU 7
CMD$CLOSEF 			EQU 8
CMD$READF 			EQU 9
CMD$WRITEF 			EQU 10

; CP/M STUFF
REBOOT				EQU 0
BDOS    			EQU 5
CONIN				EQU 1
CONOUT				EQU 2
UNBUFIO				EQU 6
PSTRING				EQU 9
CSTAT				EQU 11
OPENF				EQU 15
CLOSEF				EQU 16
DELF				EQU 19
READF				EQU 20
WRITEF				EQU 21
MAKEF				EQU 22
SETDMA				EQU 26
FCB					EQU 5CH
FCBDN				EQU 5CH
FCBFN				EQU 5DH
FCBFT				EQU 66H
FCBRL				EQU 68H
FCBRC				EQU 6BH
FCBCR				EQU 7CH
FCBLN				EQU 7DH
BUFF				EQU 80H

ORG 100H

	LXI SP, STACK
	
	MVI A, 'r'			; read mode (pi side)
	STA BUFF+1			; save to buffer
	LXI H, BUFF
	SHLD SHM$DATA		; tell the pi where to find filename
	MVI A, CMD$OPENF
	CALL DOCMD			; open the file on the pi side
	LDA SHM$DATA
	ANA A				; 0=success
	JZ OPENLCL
	LXI H, STR$REMERR	; print error and quit if failure
	CALL STROUT
	RST 0
	
OPENLCL:
	LXI D, FCB
	MVI C, DELF			; delete cp/m side file if it exists
	CALL BDOS
	LXI D, FCB
	MVI C, MAKEF		; create cp/m side file
	CALL BDOS
	INR A				; 0xff = failure
	JNZ DOXFER
	LXI H, STR$LCLERR
	CALL STROUT			; print error string
	MVI A, CMD$CLOSEF	; close pi side file
	CALL DOCMD
	RST 0				; return to cp/m
	
DOXFER:
	LXI D, BUFF
	MVI C, SETDMA		; dma addr = buffer area
	CALL BDOS
	
XFERLP:
	MVI A, CMD$READF	; read a block from pi side
	CALL DOCMD
	LDA SHM$DATA		; get the result
	ANA A				; returns number of bytes read
	JZ DONE				; so zero means we're done
	
	MVI C, WRITEF
	LXI D, FCB
	CALL BDOS			; write the block to cp/m filesystem
	ANA A
	JZ XFERLP			; 0=success
	LXI H, STR$WRTERR
	CALL STROUT
	; FALL THROUGH TO DONE
	
DONE:
	MVI A, CMD$CLOSEF
	CALL DOCMD			; close pi side file
	MVI C, CLOSEF
	LXI D, FCB	
	CALL BDOS			; close cp/m side file
	RST 0
	
	
DOCMD:
	STA SHM$CMD			; save command to shm
	OUT 0				; set request flag
DOCMD$W:
	IN 0
	ANI 1
	JNZ DOCMD$W			; wait for request flag to reset
	RET
	
STROUT:
	SHLD SHM$DATA		; hl = string address
	MVI A, CMD$STROUT	; print string command
	CALL DOCMD
	RET
	
STR$REMERR: DB 'ERROR OPENING REMOTE FILE',0
STR$LCLERR: DB 'ERROR OPENING LOCAL FILE',0
STR$WRTERR: DB 'ERROR WRITING FILE',0

DS 32
STACK:

END
