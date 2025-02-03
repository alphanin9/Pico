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
	jmp r10 ; Best callback handler! Placeholder...
Pico_InstrumentationCallbacks_Receiver ENDP
END