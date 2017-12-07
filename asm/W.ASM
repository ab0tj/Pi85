;***************************************************
; Write a file from CP/M out to the Pi's filesystem
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
	
	MVI A, 'w'			; write mode
	STA BUFF+1			; save to command line buffer
	LXI H, BUFF
	SHLD SHM$DATA		; tell the pi where to find the filename
	MVI A, CMD$OPENF	; open the file on the Pi side
	CALL DOCMD
	LDA SHM$DATA		; get the result
	ANA A
	JZ OPENLCL			; 0=success
	LXI H, STR$REMERR
	CALL STROUT
	RST 0
	
OPENLCL:
	LXI D, FCB
	MVI C, OPENF		; open the file on the CP/M side
	CALL BDOS
	ANA A
	JZ DOXFER			; 0=success
	LXI H, STR$LCLERR	; couldn't open the file in CP/M
	CALL STROUT
	MVI A, CMD$CLOSEF	; close the pi side file
	CALL DOCMD
	RST 0
	
DOXFER:
	LXI D, BUFF
	MVI C, SETDMA		; set dma address to buffer area
	CALL BDOS
	
XFERLP:
	MVI C, READF
	LXI D, FCB
	CALL BDOS			; read a block from the CP/M file
	ANA A
	JZ DOWRT			; keep going if successful
	CPI 1
	JZ DONE				; finish up if there are no more blocks to copy
	LXI H, STR$RDERR
	CALL STROUT			; otherwise display error
	JMP DONE			; and clean up

DOWRT:
	MVI A, CMD$WRITEF	; write to Pi file
	CALL DOCMD
	LDA SHM$DATA		; get the result
	ANA A
	JNZ XFERLP			; grab another block if successful
	LXI H, STR$WRTERR
	CALL STROUT			; otherwise print error
						; and fall through to cleanup
DONE:
	MVI A, CMD$CLOSEF
	CALL DOCMD			; close pi side file
	MVI C, CLOSEF
	LXI D, FCB
	CALL BDOS			; close cp/m side file
	RST 0				; quit
	
	
DOCMD:
	STA SHM$CMD			; save the command from a
	OUT 0				; set request flag
DOCMD$W:
	IN 0				; wait for request flag to be reset
	ANI 1
	JNZ DOCMD$W
	RET
	
STROUT:
	SHLD SHM$DATA		; hl=address if string
	MVI A, CMD$STROUT	; print string command
	CALL DOCMD
	RET
	
STR$REMERR: DB 'ERROR OPENING REMOTE FILE',0
STR$LCLERR: DB 'ERROR OPENING LOCAL FILE',0
STR$RDERR: DB 'ERROR READING LOCAL FILE',0
STR$WRTERR: DB 'ERROR WRITING REMOTE FILE',0

DS 32
STACK:

END
