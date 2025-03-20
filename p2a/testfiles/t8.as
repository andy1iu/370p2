        lw      0       1       ten         
        lw      0       2       one         
        add     0       0       3           
loop    beq     1       0       done        
        add     3       2       3           
        add     1       1       1           
        beq     0       0       loop        
done    halt                                
ten     .fill   10                          
one     .fill   1                           
