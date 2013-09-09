	.file	"output.bc"
	.text
	.align	16, 0x90
	.type	.L__languagekit_constants_output.bc,@function
.L__languagekit_constants_output.bc:    # @__languagekit_constants_output.bc
	.cfi_startproc
# BB#0:                                 # %entry
	pushq	%rax
.Ltmp1:
	.cfi_def_cfa_offset 16
	callq	objc_autoreleasePoolPush
	movq	%rax, %rdi
	callq	objc_autoreleasePoolPop
	popq	%rax
	ret
.Ltmp2:
	.size	.L__languagekit_constants_output.bc, .Ltmp2-.L__languagekit_constants_output.bc
	.cfi_endproc

	.align	16, 0x90
	.type	_i_STTest__run,@function
_i_STTest__run:                         # @_i_STTest__run
	.cfi_startproc
	.cfi_personality 3, __LanguageKitEHPersonalityRoutine
.Leh_func_begin1:
	.cfi_lsda 3, .Lexception1
# BB#0:                                 # %entry
	subq	$40, %rsp
.Ltmp7:
	.cfi_def_cfa_offset 48
	movq	%rdi, 32(%rsp)
	movq	%rsi, 24(%rsp)
	movb	$0, 23(%rsp)
	movq	$0, 8(%rsp)
.Ltmp3:
	movl	$.objc_str, %edi
	movl	$.objc_selector_list, %esi
	callq	objc_msgSend
.Ltmp4:
# BB#1:                                 # %invoke_continue
	movq	32(%rsp), %rax
.LBB1_3:                                # %finish
	movb	23(%rsp), %al
	testb	%al, %al
	je	.LBB1_6
# BB#4:                                 # %exception_handler
	cmpb	$1, 22(%rsp)
	jne	.LBB1_7
# BB#5:                                 # %non_local_return
	movq	8(%rsp), %rsi
	leaq	(%rsp), %rdi
	xorl	%edx, %edx
	callq	__LanguageKitTestNonLocalReturn
.LBB1_6:                                # %return
	addq	$40, %rsp
	ret
.LBB1_2:                                # %non_local_return_handler
.Ltmp5:
	movq	%rdx, %rcx
	movq	%rax, 8(%rsp)
	andl	$1, %ecx
	movb	%cl, 22(%rsp)
	movb	$1, 23(%rsp)
	jmp	.LBB1_3
.LBB1_7:                                # %rethrow
	movq	8(%rsp), %rax
	movq	%rax, %rdi
	callq	_Unwind_Resume_or_Rethrow
.Ltmp8:
	.size	_i_STTest__run, .Ltmp8-_i_STTest__run
	.cfi_endproc
.Leh_func_end1:
	.section	.gcc_except_table,"a",@progbits
	.align	4
GCC_except_table1:
.Lexception1:
	.byte	255                     # @LPStart Encoding = omit
	.byte	3                       # @TType Encoding = udata4
	.asciz	 "\242\200\200"         # @TType base offset
	.byte	3                       # Call site Encoding = udata4
	.byte	26                      # Call site table length
.Lset0 = .Ltmp3-.Leh_func_begin1        # >> Call Site 1 <<
	.long	.Lset0
.Lset1 = .Ltmp4-.Ltmp3                  #   Call between .Ltmp3 and .Ltmp4
	.long	.Lset1
.Lset2 = .Ltmp5-.Leh_func_begin1        #     jumps to .Ltmp5
	.long	.Lset2
	.byte	1                       #   On action: 1
.Lset3 = .Ltmp4-.Leh_func_begin1        # >> Call Site 2 <<
	.long	.Lset3
.Lset4 = .Leh_func_end1-.Ltmp4          #   Call between .Ltmp4 and .Leh_func_end1
	.long	.Lset4
	.long	0                       #     has no landing pad
	.byte	0                       #   On action: cleanup
	.byte	1                       # >> Action Record 1 <<
                                        #   Catch TypeInfo 1
	.byte	0                       #   No further actions
                                        # >> Catch TypeInfos <<
	.long	__LanguageKitNonLocalReturn # TypeInfo 1
	.align	4

	.text
	.align	16, 0x90
	.type	.objc_load_function,@function
.objc_load_function:                    # @.objc_load_function
	.cfi_startproc
# BB#0:                                 # %entry
	pushq	%rax
.Ltmp10:
	.cfi_def_cfa_offset 16
	movl	$__unnamed_1, %edi
	callq	__objc_exec_class
	popq	%rax
	ret
.Ltmp11:
	.size	.objc_load_function, .Ltmp11-.objc_load_function
	.cfi_endproc

	.type	__unnamed_2,@object     # @0
	.section	.rodata,"a",@progbits
	.align	16
__unnamed_2:
	.asciz	 "Hello World from Smalltalk!"
	.size	__unnamed_2, 28

	.type	.objc_str,@object       # @.objc_str
	.data
	.align	16
.objc_str:
	.quad	0
	.quad	__unnamed_2
	.long	27                      # 0x1b
	.zero	4
	.size	.objc_str, 24

	.type	__objc_class_name_STTest,@object # @__objc_class_name_STTest
	.bss
	.globl	__objc_class_name_STTest
	.align	8
__objc_class_name_STTest:
	.quad	0                       # 0x0
	.size	__objc_class_name_STTest, 8

	.type	.super_class_name,@object # @.super_class_name
	.section	.rodata,"a",@progbits
.super_class_name:
	.asciz	 "NSObject"
	.size	.super_class_name, 9

	.type	.class_name,@object     # @.class_name
.class_name:
	.asciz	 "STTest"
	.size	.class_name, 7

	.type	__unnamed_3,@object     # @1
__unnamed_3:
	.asciz	 "run"
	.size	__unnamed_3, 4

	.type	__unnamed_4,@object     # @2
__unnamed_4:
	.asciz	 "v16@0:8"
	.size	__unnamed_4, 8

	.type	.objc_method_list,@object # @.objc_method_list
	.data
	.align	16
.objc_method_list:
	.quad	0
	.long	1                       # 0x1
	.zero	4
	.quad	__unnamed_3
	.quad	__unnamed_4
	.quad	_i_STTest__run
	.size	.objc_method_list, 40

	.type	.objc_method_list1,@object # @.objc_method_list1
	.local	.objc_method_list1
	.comm	.objc_method_list1,16,8
	.type	__unnamed_5,@object     # @3
	.section	.rodata,"a",@progbits
__unnamed_5:
	.asciz	 "STTest"
	.size	__unnamed_5, 7

	.type	_OBJC_METACLASS_STTest,@object # @_OBJC_METACLASS_STTest
	.data
	.globl	_OBJC_METACLASS_STTest
	.align	16
_OBJC_METACLASS_STTest:
	.quad	0
	.quad	.super_class_name
	.quad	__unnamed_5
	.quad	0                       # 0x0
	.quad	18                      # 0x12
	.quad	0                       # 0x0
	.quad	0
	.quad	.objc_method_list1
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0
	.quad	0                       # 0x0
	.quad	0
	.quad	0
	.size	_OBJC_METACLASS_STTest, 128

	.type	.objc_protocol_list,@object # @.objc_protocol_list
	.local	.objc_protocol_list
	.comm	.objc_protocol_list,16,8
	.type	__unnamed_6,@object     # @4
	.section	.rodata,"a",@progbits
__unnamed_6:
	.asciz	 "STTest"
	.size	__unnamed_6, 7

	.type	_OBJC_CLASS_STTest,@object # @_OBJC_CLASS_STTest
	.data
	.globl	_OBJC_CLASS_STTest
	.align	16
_OBJC_CLASS_STTest:
	.quad	_OBJC_METACLASS_STTest
	.quad	.super_class_name
	.quad	__unnamed_6
	.quad	0                       # 0x0
	.quad	17                      # 0x11
	.quad	0                       # 0x0
	.quad	0
	.quad	.objc_method_list
	.quad	0
	.quad	0
	.quad	0
	.quad	.objc_protocol_list
	.quad	0
	.quad	0                       # 0x0
	.quad	0
	.quad	0
	.size	_OBJC_CLASS_STTest, 128

	.type	.objc_static_class_name,@object # @.objc_static_class_name
	.section	.rodata,"a",@progbits
	.align	16
.objc_static_class_name:
	.asciz	 "NSConstantString"
	.size	.objc_static_class_name, 17

	.type	.objc_statics,@object   # @.objc_statics
	.data
	.align	16
.objc_statics:
	.quad	.objc_static_class_name
	.quad	.objc_str
	.quad	0
	.size	.objc_statics, 24

	.type	.objc_statics_ptr,@object # @.objc_statics_ptr
	.align	8
.objc_statics_ptr:
	.quad	.objc_statics
	.quad	0
	.size	.objc_statics_ptr, 16

	.type	objc_sel_name,@object   # @objc_sel_name
	.section	.rodata,"a",@progbits
objc_sel_name:
	.asciz	 "log"
	.size	objc_sel_name, 4

	.type	.objc_sel_types,@object # @.objc_sel_types
.objc_sel_types:
	.asciz	 "v16@0:8"
	.size	.objc_sel_types, 8

	.type	.objc_selector_list,@object # @.objc_selector_list
	.data
	.align	16
.objc_selector_list:
	.quad	objc_sel_name
	.quad	.objc_sel_types
	.zero	16
	.size	.objc_selector_list, 32

	.type	__unnamed_7,@object     # @5
	.align	16
__unnamed_7:
	.quad	1                       # 0x1
	.quad	.objc_selector_list
	.short	1                       # 0x1
	.short	0                       # 0x0
	.zero	4
	.quad	_OBJC_CLASS_STTest
	.quad	.objc_statics_ptr
	.quad	0
	.size	__unnamed_7, 48

	.type	__unnamed_1,@object     # @6
	.align	16
__unnamed_1:
	.quad	9                       # 0x9
	.quad	32                      # 0x20
	.quad	0
	.quad	__unnamed_7
	.size	__unnamed_1, 32

	.section	.ctors,"aw",@progbits
	.align	8
	.quad	.objc_load_function
	.quad	.L__languagekit_constants_output.bc

	.section	".note.GNU-stack","",@progbits
