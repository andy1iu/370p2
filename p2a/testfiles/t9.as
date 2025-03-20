        lw      0       1       siz     
        lw      0       2       negOne      
        nor     1       2       3           
        sw      0       3       res      
        lw      0       4       f   
        jalr    4       7                   
        halt                                
f       add     1       1       1           
        sw      0       1       res2     
        jalr    7       6                   
siz        .fill   6                       
negOne      .fill   -1                      
res      .fill   0                       
res2     .fill   0                       
