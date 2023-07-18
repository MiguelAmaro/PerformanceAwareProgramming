# Computer Enhance



Mov instruction has 28 forms

Size varies from 1 to 6 bytes

Register to Register movs are 1 byte long (simplest to decode)

First 2 bytes have the information to decode longer instructions

```nasm
8086 decoder
file size: 2 bytes
mov cx, bx
10001001 11011001
mov bit pattern: 100010
d|w bits: 01 (reg not dest | 16bit mov)
mod bits: 11 (reg to reg op)
reg bits: 011
r/m bits: 001
```
