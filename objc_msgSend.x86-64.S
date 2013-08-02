#define DTABLE_OFFSET  16
#define SMALLOBJ_MASK  7
#define SHIFT_OFFSET   2
#define DATA_OFFSET    8
#define SLOT_OFFSET    16

.macro MSGSEND receiver, sel
	.cfi_startproc                        # Start emitting unwind data.  We
	                                      # don't actually care about any of
	                                      # the stuff except the slow call,
	                                      # because that's the only one that
	                                      # can throw.

	test  \receiver, \receiver            # If the receiver is nil
	jz    4f                              # return nil
	movq  $SMALLOBJ_MASK, %r10            # Load the small object mask
	test  \receiver, %r10                 # Check if the receiver is a small object
	jnz   6f                              # Get the small object class

	mov   (\receiver), %r10               # Load the class from the receiver
1:                                        # classLoaded
	mov   DTABLE_OFFSET(%r10), %r10       # Load the dtable from the class
	
	push  %r12
	push  %r13

	mov   \sel, %r11                    # Load the selector index
	mov   SHIFT_OFFSET(%r10), %r13        # Load the shift (dtable size)
	mov   DATA_OFFSET(%r10), %r12         # load the address of the start of the array
	
	                                   # dtable16:
	mov   %r11, %r13
	and   $0xff00, %r13
	shrl  $5, %r13d
	add   %r13, %r12
	mov   (%r12), %r12
	mov   DATA_OFFSET(%r12), %r12
3:                                       # dtable8:
	mov   %r11, %r13
	and   $0xff, %r13
	shll  $3, %r13d
	add   %r13, %r12
	mov   (%r12), %r10
	pop   %r13
	pop   %r12
	test  %r10, %r10
	jz    5f                             # Nil slot - invoke some kind of forwarding mechanism
	mov   SLOT_OFFSET(%r10), %r10

7:
	jmp   *%r10

4:                                       # returnNil:
	                                     # Both of the return registers are
	                                     # callee-save on x86-64, so we can
	                                     # return 0 in both in the same code:
	xor   %rax, %rax                     # Return 0 as an integer
#	pxor  %xmm0, %xmm0                   # Return 0 as a floating point value
	ret
5:                                       # slowSend:
	push  %rax                           # We need to preserve all registers that may contain arguments:
	push  %rbx
	push  %rcx
	push  %r8
	push  %r9
	
#	sub $0x98, %rsp
#	movups	%xmm0, 0x80(%rsp)
#	movups	%xmm1, 0x70(%rsp)
#	movups	%xmm2, 0x60(%rsp)
#	movups	%xmm3, 0x50(%rsp)
#	movups	%xmm4, 0x40(%rsp)
#	movups	%xmm5, 0x30(%rsp)
#	movups	%xmm6, 0x20(%rsp)
#	movups	%xmm7, 0x10(%rsp)

#rdi rsi rdx
	# We're (potentially) modifying the self argument with the lookup, so we don't want to be 
.ifc "\receiver", "%rdi"
	push  %rdi
	mov   %rsp, %rdi
	push  %rsi                           # Save _cmd (not preserved across calls)
	push  %rdx
.else
	push  %rdi                           # Save the sret pointer
	push  %rsi                           # Save self where it can be modified
	mov   %rsp, %rdi
	push  %rdx
	mov   %rdx, %rsi                     # move _cmd to where the callee expects it to be
.endif

#TODO adjuct the 0xD8 - should be 0x40 (0xd8 - 0x98 == 0x40)
	.cfi_adjust_cfa_offset 0x40
	call  CDECL(slowMsgLookup)           # Call the slow lookup function
	mov   %rax, %r10                     # Load the returned IMP

	pop   %rdx
	pop   %rsi
	pop   %rdi

#	movups	0x80(%rsp), %xmm0
#	movups	0x70(%rsp), %xmm1
#	movups	0x60(%rsp), %xmm2
#	movups	0x50(%rsp), %xmm3
#	movups	0x40(%rsp), %xmm4
#	movups	0x30(%rsp), %xmm5
#	movups	0x20(%rsp), %xmm6
#	movups	0x10(%rsp), %xmm7
#	add   $0x98, %rsp

	pop   %r9
	pop   %r8
	pop   %rcx
	pop   %rbx
	pop   %rax
	jmp   7b
6:                                        # smallObject:
	and   \receiver, %r10                 # Find the small int type
	shll  $3, %r10d
	lea   CDECL(objc_small_object_classes)(%rip), %r11
	add   %r11, %r10
	mov   (%r10), %r10
	jmp   1b 
	.cfi_endproc
.endm
.globl CDECL(objc_msgSend)
TYPE_DIRECTIVE(CDECL(objc_msgSend), @function)
.globl CDECL(objc_msgSend_fpret)
TYPE_DIRECTIVE(CDECL(objc_msgSend_fpret), @function)
CDECL(objc_msgSend_fpret):
CDECL(objc_msgSend):
	MSGSEND %rdi, %rsi
.globl CDECL(objc_msgSend_stret)
TYPE_DIRECTIVE(CDECL(objc_msgSend_stret), @function)
CDECL(objc_msgSend_stret):
	MSGSEND %rsi, %rdx
