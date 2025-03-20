	lw	0	1	N
	lw	0	2	sumAdd
	lw	0	3	zero
	lw	0	4	one
	lw	0	5	one
loop	beq	5	1	end
	add	3	5	3
	add	5	4	5
	beq	0	0	loop
end	sw	2	3	0
	halt
N	.fill	10
sumAdd	.fill	result
one	.fill	1
zero	.fill	0
result	.fill	0
