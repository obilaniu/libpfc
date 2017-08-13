.intel_syntax noprefix
.text
.global pfcSampleTBHITs

# Sample TurboBoost hits into a buffer.
pfcSampleTBHITs:
	push rbx
	mov  r8,   rdx
	test rsi,  rsi
	je   10f
0:
	# Enter Outer loop, or post-premption restart procedure.
	# At this stage, rdi points to the current position in the TBHIT buffer,
	# rsi > 0 indicates how many slots we must still fill in that buffers, and
	# r8 contains the THRESHold.
	mov  ecx,  0
	rdpmc
	shl  rdx,  32
	or   rdx,  rax
	mov  r9,   rdx
	rdtscp
	shl  rdx,  32
	or   rdx,  rax
	mov  r10,  rdx
	mov  r11,  rdx
1:
	# Core loop iteration.
	# At this stage:
	#   r8  contains the cycle threshold to consider as a TB preemption.
	#   r9  contains OS preemption count at entry into outer loop
	#   r10 contains timestamp           at entry into core  loop
	#   r11 contains timestamp           at entry into outer loop
	rdtscp
	shl  rdx,  32
	or   rdx,  rax
	mov  rbx,  rdx
	mov  ecx,  0
	rdpmc
	shl  rdx,  32
	or   rdx,  rax
	
	# Were we preempted by OS?
	cmp  rdx,  r9
	jne  0b
	
	# No OS preemption... but maybe TB preemption?
	xchg r10,  rbx
	neg  rbx
	add  rbx,  r10
	cmp  rbx,  r8
	jb   1b
	
	# We have been preempted for more than THRESH cycles. Record this event in the buffer.
	mov  [rdi+ 0], r11
	mov  [rdi+ 8], r10
	mov  [rdi+16], rbx
	add  rdi,  24
	dec  rsi
	jne  0b

10:
	# Return.
	pop  rbx
	retq

.att_syntax noprefix
