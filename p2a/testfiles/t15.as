	lw	0	1	numA
	lw	0	2	numB
	lw	0	3	zero
	lw	0	4	zero
	lw	0	5	one
multip	beq	4	2	end
	add	3	1	3
	add	4	5	4
	beq	0	0	multip
end	sw	0	3	result
	halt
numA	.fill	4
numB	.fill	3
one	.fill	1
zero	.fill	0
result	.fill	0
