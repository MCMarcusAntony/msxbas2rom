10 T = TIME 
20 FOR A=0 TO 1000
30    B# = sin(a) 
40 NEXT A
50 T = TIME - T
60 PRINT "ELAPSED TIME:", T
70 PRINT "LAST SINE:", B#

