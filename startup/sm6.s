;
; Demo startup (c) Jouni 'Mr.Spiv' Korhonen
; Version 6
;
; Also good for kick1.x with m680x0, where x>0
;

;; Exec Library
__LVOFindTask		equ	-294
__LVOWaitPort		equ	-384
__LVOGetMsg		equ	-372
__LVOOldOpenLibrary	equ	-408
__LVOCloseLibrary	equ	-414
__LVODisable		equ	-120
__LVOEnable		equ	-126
__LVOForbid		equ	-132
__LVOReplyMsg		equ	-378
__LVOOpenResource	equ	-498
__LVOSupervisor		equ	-30
__LVORawDoFmt		equ	-522

;; Execbase
__AttnFlags		equ	296

;; Task..
__pr_CLI		equ	172
__pr_MsgPort		equ	92

;; Graphics Library

__LVOLoadView		equ	-222
__LVOWaitTOF		equ	-270
__LVOOwnBlitter		equ	-456
__LVOWaitBlit		equ	-228
__LVODisownBlitter	equ	-462

;; gfxbase
__PowerSupplyFrequency	equ	531
__gb_LOFlist		equ	50
__gb_copinit		equ	38
__gb_ActiView		equ	34

;; resource Library
__LVOAbleICR		equ	-18


	IF 0


;-------------- Flags for different setups -----------------------------------

YES		equ	1
NO		equ	0
smPAL		equ	1			; for smTV
smDONTCARE	equ	0			;    ""
		;
		;
		;
smABS		equ	NO
smWBStartup	equ	YES			; include wbstartup..
smSetCaches	equ	YES			; set caches..
smCIAA		equ	NO			; timers..
smCIAB		equ	NO			; timers..
smDrivesOff	equ	NO			; switch off all internals..
smTV		equ	smDONTCARE		; tv system..
sm64		equ	YES			; level1 interrupts..
sm68		equ	YES			; level2 interrupts..
sm6c		equ	YES			; level3 interrupts..
sm78		equ	YES			; level6 interrupts..
smAllocate	equ	NO
smNOAGA		equ	YES

smCacheFlags	equ	0
smCacheMask	equ	$ffffffff

        ; For serial TRACE macro
smSerPer9600PAL		equ	368
smSerPer115200PAL	equ	29
smSerPer230400PAL	equ	14
smNOSYSSerPer		equ	smSerPer230400PAL

smNOSYSTRACE		equ	YES
	ENDIF

smbottom	equ	-72

;-----------------------------------------------------------------------------


demostartup:	movem.l	d1-a6,-(sp)
		move.l	$04.w,a6
		;link	a4,#smbottom

;-----------------------------------------------------------------------------
;- WBstartup code.. Include only to filal exe (jams when runned from compiler)
;-----------------------------------------------------------------------------
		IF	smWBStartup		* So called WBstartup..
		sub.l	a1,a1
		jsr	__LVOFindTask(a6)	; -294
		move.l	d0,a3
		moveq	#0,d0
		tst.l	__pr_CLI(a3)		; 172
		bne.s	.fromcli
.fromwb:	lea	__pr_MsgPort(a3),a0	; 92
		jsr	__LVOWaitPort(a6)	; -384
		lea	__pr_MsgPort(a3),a0	; 92
		jsr	__LVOGetMsg(a6)		; -372
.fromcli:	move.l	d0,-(sp)		; smmessage(a4)
		ENDIF

;-----------------------------------------------------------------------------
;- Allocate memory if wanted..
;-----------------------------------------------------------------------------

		lea	$bfd000,a2
		lea	$dff002,a3

;-----------------------------------------------------------------------------
;- Set caches as user wants them to be..
;-----------------------------------------------------------------------------
		IFNE	smSetCaches
		moveq	#smCacheFlags,d0
		move.l	#smCacheMask,d1
		bsr.w	F80C4C
		move.l	d0,-(sp)		; smoldcache(a4)
		ENDIF

;-----------------------------------------------------------------------------
;- Open gfx.library & set views..
;-----------------------------------------------------------------------------
		lea	.gfxname(pc),a1
		jsr	__LVOOldOpenLibrary(a6)	; -408
		tst.l	d0
		beq	.gfxerror
		move.l	d0,a6
		move.l	__gb_copinit(a6),-(sp)	; smcopinit(a4)	; 38
		move.l	__gb_LOFlist(a6),-(sp)	; smloflist(a4)	; 50
		move.l	__gb_ActiView(a6),-(sp)	; smview(a4)	; 34
		move.l	d0,-(sp)		; smgfxbase(a4)

		sub.l	a1,a1
		jsr	__LVOLoadView(a6)	; -222
		jsr	__LVOWaitTOF(a6)	; -270
		jsr	__LVOWaitTOF(a6)	; -270
		jsr	__LVOOwnBlitter(a6)	; -456
		jsr	__LVOWaitBlit(a6)	; -228

;-----------------------------------------------------------------------------
;- Interrupts off.. Just in case..
;-----------------------------------------------------------------------------
		move.l	$04.w,a6
		jsr	__LVODisable(a6)	; -120

;-----------------------------------------------------------------------------
;- CIAs.. init ICRs to $7f..
;-----------------------------------------------------------------------------
		IFNE	smCIAA
		lea	.ciaaname(pc),a1
		moveq	#0,d0
		jsr	__LVOOpenResource(a6)	; -498
		move.l	d0,a6
		moveq	#$7f,d0
		jsr	__LVOAbleICR(a6)	; -18
		or.b	#$80,d0
		;move.b	d0,-(sp)		; smAicr(a4)
		move.w	d0,-(sp)		; smAicr(a4)
		moveq	#$10,d0
		move.b	$1e01(a2),-(sp)		; smAcra(a4)
		move.b	$1f01(a2),-(sp)		; smAcrb(a4)
		or.b	d0,$1e01(a2)		* force load
		move.b	$1401(a2),-(sp)		; smAtalo(a4)
		move.b	$1501(a2),-(sp)		; smAtahi(a4)
		or.b	d0,$1f01(a2)		* force load
		move.b	$1601(a2),-(sp)		; smAtblo(a4)
		move.b	$1701(a2),-(sp)		; smAtbhi(a4)
		move.l	a6,-(sp)		; smCIAAresbase(a4)
		ENDIF

		IFNE	smCIAB
		move.l	$04.w,a6
		lea	.ciabname(pc),a1
		moveq	#0,d0
		jsr	__LVOOpenResource(a6)	; -498
		move.l	d0,a6
		moveq	#$7f,d0
		jsr	__LVOAbleICR(a6)	; -18
		or.b	#$80,d0
		move.w	d0,-(sp)		; smBicr(a4)
		moveq	#$10,d0
		move.b	$e00(a2),-(sp)		; smBcra(a4)
		move.b	$f00(a2),-(sp)		; smBcrb(a4)
		or.b	d0,$e00(a2)		* force load
		move.b	$400(a2),-(sp)		; smBtalo(a4)
		move.b	$500(a2),-(sp)		; smBtahi(a4)
		or.b	d0,$f00(a2)		* force load
		move.b	$600(a2),-(sp)		; smBtblo(a4)
		move.b	$700(a2),-(sp)		; smBtbhi(a4)
		move.l	a6,-(sp)		; smCIABresbase(a4)
		ENDIF

;-----------------------------------------------------------------------------
;- VBR check..
;-----------------------------------------------------------------------------
		move.l	$04.w,a6
		bsr.w	smVBRcode
		move.l	d0,a5

;-----------------------------------------------------------------------------
;- Hardware level installation..
;-----------------------------------------------------------------------------
		move.l	#$80008000,d1
		move.l	(a3),d0			* DMACON
		move.w	$01c-2(a3),d0		* INTENA
		or.l	d1,d0
		move.l	d0,-(sp)		; smdmacon(a4)
		move.w	d1,d0
		not.l	d1
		move.l	d1,$09a-2(a3)
		move.w	d1,$096-2(a3)
		or.w	$010-2(a3),d0
		move.w	d0,-(sp)		; smadkcon(a4)	* ADKCON
		move	d1,$09e-2(a3)

		IFNE	smNOAGA
		moveq	#0,d0
		move	d0,$104-2(a3)		* BPLCON2 in case
		move	#$0c00,$106-2(a3)	* BPLCON3 = AGA -> OLD
		move	#$0011,$10c-2(a3)	* BPLCON4 = AGA -> OLD
		move	d0,$1fc-2(a3)		* FMODE = AGA -> OLD
		ENDIF

		IFNE	smTV=smPAL
		move	#$0020,$1dc-2(a3)	* BEAMCON0
		ENDIF

		IFNE	smDrivesOff
		or.b	#%11111000,$100(a2)
		and.b	#%10000111,$100(a2)
		or.b	#%01111000,$100(a2)
		ENDIF

;-----------------------------------------------------------------------------
;- Check PAL/NTSC manually (in case of old chipset)..
;-----------------------------------------------------------------------------
		move.l	#1773447,d7		; 50 times per second (PAL)
		cmp.b	#50,__PowerSupplyFrequency(a6)	; 531
		beq.s	.pal
		add	#1789773-1773447,d7	; 50 times per second (NTSC)
.pal:

;-----------------------------------------------------------------------------
;- Interrupt vectors..
;-----------------------------------------------------------------------------
		IFNE	sm64
		move.l	$64(a5),-(sp)		; sm64base(a4)
		ENDIF
		IFNE	sm68
		move.l	$68(a5),-(sp)		; sm68base(a4)
		ENDIF
		IFNE	sm6c
		move.l	$6c(a5),-(sp) 		; sm6cbase(a4)
		ENDIF
		IFNE	sm78
		move.l	$78(a5),-(sp)		; sm78base(a4)
		ENDIF

;-----------------------------------------------------------------------------
;-
;- Call main program..
;-
;-----------------------------------------------------------------------------
		IFNE	smNOSYSTRACE
.wait_TSRE:
		btst	#4,$18-2(a3)	; TSRE = bit 12
		beq.b	.wait_TSRE
		move.w	#smNOSYSSerPer,$32-2(a3)
		ENDIF

		; Where these registers have following values
		;  d7 = timer value for correct PAL speed (music..)
		;  a2 = $bfd000
		;  a3 = $dff002
		;  a4 = variables in stack
		;  a5 = VBR
		;  a6 = ExecBase

		movem.l	a2-a6,-(sp)
		jsr	main			* call our program
		movem.l	(sp)+,a2-a6

;-----------------------------------------------------------------------------
;- Start restoring.. Interrupts, interrupt vectors, DMA, ADK
;-----------------------------------------------------------------------------
		move.w	#$7fff,d2
		moveq	#$7f,d3
		move.w	d2,$09e-2(a3)
		move.w	d2,$09c-2(a3)
		move.w	d2,$09a-2(a3)
		move.w	d2,$096-2(a3)
		IFNE	smCIAA
		move.b	d3,$1d01(a2)
		ENDIF
		IFNE	smCIAB
		move.b	d3,$d00(a2)
		ENDIF

		IFNE	sm78
		move.l	(sp)+,$78(a5)
		ENDIF
		IFNE	sm6c
		move.l	(sp)+,$6c(a5)
		ENDIF
		IFNE	sm68
		move.l	(sp)+,$68(a5)
		ENDIF
		IFNE	sm64
		move.l	(sp)+,$64(a5)
		ENDIF

		move	(sp)+,$09e-2(a3)	; smadkcon
		move	(sp)+,$096-2(a3)	; smdmacon
		move	(sp)+,$09a-2(a3)	; smintena

;-----------------------------------------------------------------------------
;- Restore CIAs..
;-----------------------------------------------------------------------------
		IFNE	smCIAB
		move.l	(sp)+,a6		; smCIABresbase
		move.b	(sp)+,$700(a2)		; smBtbhi
		move.b	(sp)+,$600(a2)		; smBtblo
		move.b	(sp)+,$500(a2)		; smBtahi
		move.b	(sp)+,$400(a2)		; smBtalo
		move.b	(sp)+,$f00(a2)		; smBcrb
		move.b	(sp)+,$e00(a2)		; smBcra
		move.w	(sp)+,d0		; smBicr
		jsr	__LVOAbleICR(a6)	; -18
		ENDIF

		IFNE	smCIAA
		move.l	(sp)+,a6		; smCIAAresbase
		move.b	(sp)+,$1701(a2)		; smAtbhi
		move.b	(sp)+,$1601(a2)		; smAtblo
		move.b	(sp)+,$1501(a2)		; smAtahi
		move.b	(sp)+,$1401(a2)		; smAtalo
		move.b	(sp)+,$1f01(a2)		; smAcrb
		move.b	(sp)+,$1e01(a2)		; smAcra
		move.w	(sp)+,d0		; smAicr
		jsr	__LVOAbleICR(a6)	; -18
		ENDIF

;-----------------------------------------------------------------------------
;- Restore blitter, views, copperlists..
;-----------------------------------------------------------------------------
		move.l	$04.w,a6
		jsr	__LVOEnable(a6)		; -126
		move.l	(sp)+,a6		; smgfxbase
		jsr	__LVODisownBlitter(a6)	; -462

		move.l	(sp)+,a1		; smview
		jsr	__LVOLoadView(a6)	; -222
		move.l	(sp)+,$084-2(a3)	; smloflist
		move.l	(sp)+,$080-2(a3)	; smcopinit
		move	d0,$088-2(a3)

		move.l	a6,a1
		move.l	$04.w,a6
		jsr	__LVOCloseLibrary(a6)	; -414

;-----------------------------------------------------------------------------
;- Restore caches..
;-----------------------------------------------------------------------------
.gfxerror:
		IFNE	smSetCaches
		move.l	(sp)+,d0		; smoldcache
		move.l	#smCacheMask,d1
		bsr.b	F80C4C
		ENDIF

;-----------------------------------------------------------------------------
;- Reply to WBmessage..
;-----------------------------------------------------------------------------
		IFNE	smWBStartup
		move.l	(sp)+,d2		; smmessage
		beq.s	.nomessage
		jsr	__LVOForbid(a6)		; -132
		move.l	d2,a1
		jsr	__LVOReplyMsg(a6)	; -378
		ENDIF

.nomessage:	
		;unlk	a4
		movem.l	(sp)+,d1-a6
		moveq	#0,d0
		rts

.gfxname:	dc.b	"graphics.library",0
		IFNE	smCIAA
.ciaaname:	dc.b	"ciaa.resource",0
		ENDIF
		IFNE	smCIAB
.ciabname:	dc.b	"ciab.resource",0
		ENDIF
		even

;---------------------------------------------------------------------------
;- If 68010+ exists return VBR..
;---------------------------------------------------------------------------
smVBRcode:	movem.l	d2/a5/a6,-(sp)
		moveq	#0,d2
		move	__AttnFlags(a6),d0	; Execbase->AttnFlags
		lsr	#1,d0			; 296
		bcc.s	.exitvbr
		lea	.vbrcode(pc),a5
		jsr	__LVOSupervisor(a6)	; -30
.exitvbr:	move.l	d2,d0
		movem.l	(sp)+,d2/a5/a6
		rts

.vbrcode:	dc.l	$4E7A2801		* movec	VBR,d2
		rte


;---------------------------------------------------------------------------
;- Proper cache routine for 68020+.. Same as _LVOCacheControl()
;---------------------------------------------------------------------------
		IFNE	smSetCaches
		cnop	0,4
F80C4C:		movem.l	d2-d4/a5-a6,-(a7)
		moveq	#0,d3
		move.w	__AttnFlags(a6),d4	; Execbase->AttnFlags
		btst	#1,d4			; 296
		beq.s	F80C6C
		and.l	d1,d0
		ori.w	#$808,d0
		not.l	d1
		lea	F80C74(pc),a5
		jsr	__LVOSupervisor(a6)	; -30
F80C6C:		move.l	d3,d0
		movem.l	(a7)+,d2-d4/a5-a6
		rts

F80C74: 	ori.w	#$700,sr
		dc.l	$4E7A2002
		btst	#3,d4
		beq.s	F80C88
		swap	d2
		ror.w	#8,d2
		rol.l	#1,d2
F80C88:		move.l	d2,d3
		and.l	d1,d2
		or.l	d0,d2
		btst	#3,d4
		beq.s	F80CA4
		ror.l	#1,d2
		rol.w	#8,d2
		swap	d2
		andi.l	#$80008000,d2
		nop
		dc.w	$F4F8
F80CA4:		nop
		dc.l	$4E7B2002
		nop
		rte
		ENDIF

;----------------------------------------------------------------------------
; Serial TRACE function codes
;
; Currently these macros functions assume that Execbase is still
; valid when the tracing is called..
;
		IFNE	smNOSYSTRACE

TRACE	MACRO	; fmt, ...
SIZE	SET	0

	IFNC	"\9",""
SIZE	SET	SIZE+4
	move.l	\9,-(sp)
	ENDC
	
	IFNC	"\8",""
SIZE	SET	SIZE+4
	move.l	\8,-(sp)
	ENDC

	IFNC	"\7",""
SIZE	SET	SIZE+4
	move.l	\7,-(sp)
	ENDC

	IFNC	"\6",""
SIZE	SET	SIZE+4
	move.l	\6,-(sp)
	ENDC

	IFNC	"\5",""
SIZE	SET	SIZE+4
	move.l	\5,-(sp)
	ENDC

	IFNC	"\4",""
SIZE	SET	SIZE+4
	move.l	\4,-(sp)
	ENDC

	IFNC	"\3",""
SIZE	SET	SIZE+4
	move.l	\3,-(sp)
	ENDC

	IFNC	"\2",""
SIZE	SET	SIZE+4
	move.l	\2,-(sp)
	ENDC

	IFGT	NARG-1
	move.l	sp,a1
	ENDC
	lea	__str\@(pc),a0
	jsr	HW_printf_pio

	add.w	#SIZE,sp
	bra.b	__skip\@

__str\@:	dc.b	\1,10,0
	even
__skip\@:
	ENDM

;
; A0=format string 
; A1=datas
;
; Trashes:
;  None
;

HW_printf_pio:
	movem.l	d0-d1/a0-a3/a6,-(sp)
	lea	NOSYS_send_pio(pc),a2

	move.l	$4.w,a6
	lea	$dff018,a3
	jsr	__LVORawDoFMT(a6)

	movem.l	(sp)+,d0-d1/a0-a3/a6
	rts

NOSYS_send_pio:
	and.w	#$00ff,d0
	or.w	#$0100,d0	; 8 data bits, 1 stop bit
.wait_TBE:
	btst	#5,(a3)		; TBE = bit 13
	beq.b	.wait_TBE

	move.w	d0,$030-$18(a3)
	rts	

		ENDIF

		IFEQ	smNOSYSTRACE

; Just an empty macro..
TRACE	MACRO	; fmt, ...
	ENDM

		ENDIF


;-----------------------------------------------------------------------------
;- End of C64 code..
;-----------------------------------------------------------------------------
