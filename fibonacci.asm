;TITLE: fibonacci.asm
;AUTHOR: Keshav Raheja
;Roll No: 2401CS10
;
;Declaration of Authorship
;This test file, is part of the miniproject of CS2206 at the
;department of Computer Science and Engg, IIT Patna.
;*****************************************************************************/
; Computes fib(n) iteratively
; fib(0)=0, fib(1)=1, fib(10)=55

        ldc     0x1000          ; initialise stack pointer
        a2sp
        adj     -1

        ldc     n               ; load address of n
        ldnl    0               ; A = value of n
        stl     0               ; push n onto stack

        call    fib             ; result comes back in A

        adj     1

        ldc     result
        stnl    0               ; store result
        HALT

; fib(n) -- n at SP+3 on entry, result returned in A
; stack frame: SP+0 = return addr
;              SP+1 = a (fib value i-2)
;              SP+2 = b (fib value i-1)
;              SP+3 = n (counter)

fib:    adj     -3
        stl     0               ; save return address

        ldl     3               ; A = n
        brz     fib0            ; n == 0 → return 0
        adc     -1
        brz     fib1            ; n == 1 → return 1

        ; general case: iterative loop
        ; a = 0, b = 1, loop n-1 times
        ldc     0
        stl     1               ; SP+1 = a = 0
        ldc     1
        stl     2               ; SP+2 = b = 1

        ldl     3               ; load n
        adc     -1
        stl     3               ; n = n - 1 (already used one step)

fibloop:
        ldl     3               ; load counter
        brz     fibdone         ; counter == 0 → done

        ldl     1               ; A = a
        ldl     2               ; B = a, A = b
        add                     ; A = a + b  (new b)
        ldl     2               ; B = a+b, A = old b  → new a
        stl     1               ; SP+1 = new a
        stl     2               ; SP+2 = new b

        ldl     3
        adc     -1
        stl     3               ; counter--
        br      fibloop

fibdone:
        ldl     2               ; A = b = fib(n)
        ldl     0               ; B = fib(n), A = return address
        adj     3
        return

fib0:   ldc     0               ; fib(0) = 0
        ldl     0               ; B = 0, A = return address
        adj     3
        return

fib1:   ldc     1               ; fib(1) = 1
        ldl     0               ; B = 1, A = return address
        adj     3
        return

; data section — change n to compute any fibonacci number
; fib(0)=0  fib(1)=1  fib(2)=1  fib(5)=5  fib(10)=55
n:      data    10
result: data    0