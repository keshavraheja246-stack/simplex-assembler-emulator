;TITLE: BubbleSort.asm
;AUTHOR: Keshav Raheja
;Roll No: 2401CS10
;
;Declaration of Authorship
;This test file, is part of the miniproject of CS2206 at the
;department of Computer Science and Engg, IIT Patna.
;*****************************************************************************/
; Sorts arr[0..N-1] in ascending order

        ldc     0x1000
        a2sp
        adj     -1

        ldc     N
        ldnl    0               ; A = N
        stl     0               ; Memory[SP] = N

        ldc     arr
        stl     -1              ; Memory[SP-1] = arr address
        adj     -1              ; SP--

        call    sort
        adj     2
        HALT

; Frame after adj -6:
;   SP+0 = return address
;   SP+1 = temp1 (arr[j] saved for swap)
;   SP+2 = temp2 (addr_j saved for swap)
;   SP+3 = i
;   SP+4 = j
;   SP+5 = arr base  (old SP+0)
;   SP+6 = N         (old SP+1)

sort:   adj     -6
        stl     0               ; save return address
        ldl     6               ; A = arr base (old SP+0)
        stl     5               ; SP+5 = arr base
        ldl     7               ; A = N (old SP+1)
        stl     6               ; SP+6 = N

        ldc     0
        stl     3               ; i = 0

outer:
        ldl     6               ; A = N
        adc     -1
        ldl     3               ; B = N-1, A = i
        sub                     ; A = i-(N-1)
        brz     done
        brlz    done

        ldc     0
        stl     4               ; j = 0

inner:
        ldl     6
        adc     -1
        ldl     3
        sub                     ; A = (N-1)-i
        ldl     4               ; B = N-i-1, A = j
        sub                     ; A = j-(N-i-1)
        brz     incdone
        brlz    incdone

        ; compute addr_j = base + j, save to SP+2
        ldl     5               ; A = base
        ldl     4               ; B = base, A = j
        add                     ; A = addr_j
        stl     2               ; SP+2 = addr_j

        ; load arr[j], save to SP+1
        ldl     2               ; A = addr_j
        ldnl    0               ; A = arr[j]
        stl     1               ; SP+1 = arr[j]

        ; load arr[j+1]
        ldl     2               ; A = addr_j
        adc     1               ; A = addr_j+1
        ldnl    0               ; A = arr[j+1]

        ; compare: SP+1=arr[j], A=arr[j+1]
        ; sub does B-A, need B=arr[j], A=arr[j+1]
        ldl     1               ; B=arr[j+1], A=arr[j]
        sub                     ; A = arr[j+1] - arr[j]  (sub does B-A)
        brlz    doswap          ; arr[j+1] < arr[j] → swap
        br      addy            ; arr[j] <= arr[j+1] → no swap

doswap:
        ; SP+1 = arr[j], SP+2 = addr_j
        ; Step 1: Memory[addr_j] = arr[j+1]
        ;   need B=arr[j+1], A=addr_j
        ;   arr[j+1]: load it so A=arr[j+1], then ldl 2 gives B=arr[j+1], A=addr_j
        ldl     2               ; A = addr_j  (B = arr[j]-arr[j+1], doesn't matter)
        adc     1               ; A = addr_j+1
        ldnl    0               ; A = arr[j+1]
        ldl     2               ; B = arr[j+1], A = addr_j   ← KEY: ldl sets B=old A
        stnl    0               ; Memory[addr_j + 0] = B = arr[j+1]  ✓

        ; Step 2: Memory[addr_j+1] = arr[j]
        ;   need B=arr[j], A=addr_j+1
        ;   SP+1=arr[j], so ldl 1 gives B=old A (addr_j), A=arr[j]
        ;   then we need addr_j+1 in A with arr[j] in B
        ldl     1               ; B=addr_j (from prev stnl left A=addr_j), A=arr[j]
        ldl     2               ; B=arr[j], A=addr_j   ← ldl 2 = addr_j
        adc     1               ; B=arr[j], A=addr_j+1
        stnl    0               ; Memory[addr_j+1] = B = arr[j]  ✓

addy:   ldl     4
        adc     1
        stl     4               ; j++
        br      inner

incdone:
        ldl     3
        adc     1
        stl     3               ; i++
        br      outer

done:   ldl     0
        adj     6
        return

N:      data    6
arr:    data    40
        data    10
        data    30
        data    20
        data    50
        data    5