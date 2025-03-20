	lw	0	1	aAddr
	lw	0	2	sVal
	lw	0	3	zero
	lw	0	5	aLen
	lw	0	6	one
	lw	0	4	sSub
	jalr	4	7
	sw	0	3	result
	jalr	4	7
	halt
sLoop	beq	3	5	notF
	add	1	3	7
	lw	7	4	0
	beq	4	2	F
	add	3	6	3
	beq	0	0	sLoop
F	jalr	7	6
notF	lw	0	3	negOne
	jalr	7	6
rLoop	add	3	0	6
	beq	6	5	rEnd
	add	1	3	7
	lw	7	4	0
	add	1	5	2
	lw	2	6	0
	sw	7	6	0
	sw	2	4	0
	beq	0	0	rLoop
rEnd	jalr	7	6
aAddr	.fill	a
sVal	.fill	7
aLen	.fill	5
result	.fill	0
one	.fill	1
zero	.fill	0
negOne	.fill	-1
sSub	.fill	sSub
a	.fill	3
	.fill	5
	.fill	7
	.fill	9
	.fill	11
