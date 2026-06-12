;TITLE: sum.asm																																
;AUTHOR:   Keshav Raheja
;Roll No:  2401CS10
;
;Declaration of Authorship
;This test file, is part of the miniproject of CS2206 at the department of Computer Science and Engg, IIT Patna . 
;*****************************************************************************/
; outline of calculating sum of digits

;Example: 9876 → 9+8+7+6 = 30

        ldc     0x1000
        a2sp
        adj     -1

        ldc     num
        ldnl    0               ; A = number
        stl     0

        adj     -1
        call    sumdigits
        adj     1

        ldc     result
        stnl    0
        HALT

; stack frame:
;   SP+0 = return address
;   SP+1 = sum
;   SP+2 = n
;   SP+3 = digit (remainder)
;   SP+4 = quotient

sumdigits:
        adj     -4
        stl     0               ; save return address

        ldl     5               ; A = n from caller
        stl     2               ; SP+2 = n

        ldc     0
        stl     1               ; sum = 0

loop:   ldl     2
        brz     done            ; n == 0 → done

        ; ── get digit = n mod 10 ──────────────────
        ldc     0
        stl     4               ; quotient = 0

        ldl     2               ; A = n
        stl     3               ; remainder = n

modloop:
        ldl     3               ; A = remainder
        adc     -10             ; A = remainder - 10
        brlz    modend          ; remainder < 10 → done

        stl     3               ; remainder = remainder - 10

        ldl     4
        adc     1
        stl     4               ; quotient++

        br      modloop

modend:
        ; SP+3 = digit (n mod 10)
        ; SP+4 = n / 10

        ldl     3               ; A = digit
        ldl     1               ; B = digit, A = sum
        add                     ; A = sum + digit
        stl     1               ; save new sum

        ldl     4               ; A = quotient = n/10
        stl     2               ; n = n/10

        br      loop

done:   ldl     1               ; A = sum
        ldl     0               ; B = sum, A = return address
        adj     4
        return

num:    data    9876
result: data    0

;n = 9876
;subtract 10 repeatedly:  9876 → 9866 → ... → 6   (remainder = digit = 6)
;count how many times:    quotient = 987 = new n