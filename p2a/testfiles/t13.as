        lw      0       1       Num  //test case 5 and 6       
        lw      0       2       Count    
Loop    beq     1       2       End         
        add     2       3       2           
        beq     0       0       Loop        
End     sw      0       2       Result      
        halt                                
        .fill   Start                       
Num     .fill   5                           
Result  .fill   0                           
