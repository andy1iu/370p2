	lw	0	2	lbl
l01	sw	2	0	0
lbl	lw	1	3	2
	lw	0	0	l01
	sw	1	2	Glo
	lw	0	1	con
	add	1	3	3
	beq	3	1	brc
	noop
brc	lw	2	4	0
Glo	.fill	dbl
und	.fill	ref
dbl	.fill	und
ref	.fill	8
con	.fill	10
arr	.fill	20
	.fill	30
	.fill	40
	.fill	50
	.fill	60
