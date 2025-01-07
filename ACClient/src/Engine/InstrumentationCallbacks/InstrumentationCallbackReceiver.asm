; This actually has to be written in ASM, as it does not really have a call conv
; Instrumentation callbacks fire at pretty much every KM-UM transition to our process 
; (most system calls, APCs, exceptions, thread creation...)
; So we cannot rely on C++ in instrumentation callback land, 
; as these do not follow any call conv (outside of original RIP being in R10)
; So, we write a short stub to handle every callback we can get

INCLUDE ksamd64.inc

; LdrInitializeThunk
; EXTERNDEF LdrInitializeThunk:QWORD

; Context capture
EXTERNDEF __imp_RtlCaptureContext:QWORD

.CODE
Pico_InstrumentationCallbacks_Receiver PROC
	; Backup RSP and RIP to their intended places in TEB
	mov     gs:[2e0h], rsp            ; Windows::TEB64::InstrumentationCallbackPreviousSp
	mov     gs:[2d8h], r10            ; Windows::TEB64::InstrumentationCallbackPreviousPc

	; Note: should zero these out after callback has ran - no point leaking info
	
	
	
	; Capture context before checking this, otherwise things will get messy with flags and all
	mov r10, rcx					  ; Backup original RCX

	sub     rsp, 4d0h				  ; Setup stack space to fit CONTEXT structure
	and     rsp, -10h				  ; Align RSP
	mov     rcx, rsp				  ; Setup in param
	call    __imp_RtlCaptureContext	  ; Save context
	
	; Compare R10 to known user-mode exit spots
	; If we cannot resolve a known exit spot, default to system call handler callback
	; Callback logs things and restores context so we keep executing normally

	;cmp r10, LdrInitializeThunk
	;jz OnLdrInitializeThunk

	OnLdrInitializeThunk:
	

	
	
	; We should NOT be here
	int 3

	
Pico_InstrumentationCallbacks_Receiver ENDP
END