;TITLE: GCD.asm																																
;AUTHOR:   Keshav Raheja
;Roll No:  2401CS10
;
;Declaration of Authorship
;This test file, is part of the miniproject of CS2206 at the department of Computer Science and Engg, IIT Patna . 
;*****************************************************************************/
; outline of calculating gcd using euclid algorithm

;TITLE: GCD.asm
;outline of calculating gcd using euclid algorithm

        ldc     0x1000          ; initialise stack pointer
        a2sp
        adj     -1

        ldc     a               ; load address of a
        ldnl    0               ; A = value of a (48)
        stl     0               ; push a onto stack

        ldc     b               ; load address of b
        ldnl    0               ; A = value of b (18)
        stl     -1              ; push b onto stack

        adj     -2
        call    gcd             ; call gcd, result comes back in A

        adj     2

        ldc     result          ; A = address of result
        stnl    0               ; mem[result] = gcd value
        HALT

; gcd function
; a at SP+4, b at SP+5 on entry
gcd:    adj     -3
        stl     0               ; save return address
        stl     2               ; save B

        ldl     4               ; A = a
        stl     1               ; SP+1 = local a
        ldl     5               ; A = b
        stl     2               ; SP+2 = local b

loop:   ldl     1               ; A = a
        ldl     2               ; B = a, A = b
        sub                     ; A = a - b
        brz     done
        brlz    b_gt_a

a_gt_b: ldl     1
        ldl     2
        sub                     ; A = a - b
        stl     1               ; a = a - b
        br      loop

b_gt_a: ldl     2
        ldl     1
        sub                     ; A = b - a
        stl     2               ; b = b - a
        br      loop

done:   ldl     1               ; A = gcd
        ldl     0               ; B = gcd, A = return address
        adj     3
        return

; data section — change a and b to compute any GCD
; gcd(48, 18) = 6
; gcd(100, 75) = 25
a:      data    100
b:      data    150
result: data    0