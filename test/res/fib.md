Sets up the tape and enters the main loop

```bf
++++++++++>>+>>+[
```

Goes through the tape and outputs each digit of the second to last calculated
Fibonacci number

```bf
[+++++[<++++++++>-]<.>++++++[<-------->-]+<<<]
```

Outputs a newline and moves to the last digits of the numbers and enters the
loop to add our current Fibonacci numbers

```bf
<.>>>>[
```

Moves the value of the smaller fib number to the flag cell

```bf
-<[>+<-]
```

Copies the larger fib number to the newly emptied cell while adding it to the
value stored in the flag cell

```bf
<[>+>+<<-]>>
```

Moves the value in the flag cell to where the larger fib number and while it's
moving the value it also checks to see if it's greater than nine

```bf
[<<+>>-[<<+>>-[<<+>>-[<<+>>-[<<+>>-[<<+>>-[<<+>>-[<<+>>-[<<+>>-[
```

If the value is greater than nine we move the remainder of the value in the flag
cell to the larger fib cell and carry the one and also make sure that the flag
bit of the next digit is set and then move back to where we were

```bf
-<<[-]>>[<<+>>-]>>>[-]+<+<<
```

Set the flag bit back to one and move to the next digit over

```bf
]]]]]]]]]]+>>>]
```

Move to the first digit in preparation of printing the number but also make sure
to move over a digit if the first digit would be a zero

```bf
<<<<[>>>]<<]
```
