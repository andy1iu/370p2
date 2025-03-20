        lw      0       1       aSize   
        lw      0       2       aAddr   
        lw      0       6       one         
        add     0       0       3           
        add     0       0       4           
loop    beq     4       1       done        
        lw      2       5       0           
        add     3       5       3           
        add     2       6       2           
        add     4       6       4           
        beq     0       0       loop        
done    halt                                
aSize   .fill   5                       
aAddr   .fill   array                   
one         .fill   1                       
array       .fill   11                      
            .fill   18                      
            .fill   23                      
            .fill   44                      
            .fill   49                      
