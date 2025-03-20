	lw	0	1	Ivanc	// line 1: references label Ivanc (undefined)
	add	1	1	1	// line 2
	beq	1	2	2	// line 3: branch to "ivanc" (all lowercase)
	add	1	1	1	// line 4
	jalr	1	2		// line 5
	add	2	2	2	// line 6
	halt				// line 7
	lw	0	1	Data	// line 8: extra instruction referencing Data
Data	.fill	5			// line 9: data definition for Data
Hello	.fill	Geo
HELP	.fill	PEOP
yell	.fill	PIP
