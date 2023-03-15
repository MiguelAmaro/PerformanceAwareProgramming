#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define Kilobytes(x) (x*1024)


typedef enum
{
    Op_Cmp,
}
#include "helpers.h"

int main()
{
    printf("8086 decoder\n");
    FILE *InFile = fopen(".\\decodeme_long", "rb");
    
    uint32_t Size = GetFileSize(InFile);
    printf("file size: %u bytes\n", Size);
    
    uint8_t *Buffer = malloc(Kilobytes(1));
    fread(Buffer, Size, 1, InFile);
    fclose(InFile);
    
    FILE *OutFile = fopen("..\\data\\decoded_long.asm", "w+");
    fputs("bit 16\n\n", OutFile);
    uint32_t OpCount = Size/2;
    uint8_t *InstructionStream = (uint8_t *)Buffer;
    PrintBinaryBytes(InstructionStream, Size, 1, 2);
    for(uint32_t OpIdx=0;OpIdx<OpCount; OpIdx++)
    {
        uint8_t *MemOp  = &InstructionStream[OpIdx*2];
        printf("\n/NEXT INSTRUCTION ==========================/\n");
        printf("mov bit pattern: "); PrintBinaryBits(MemOp, Size, 0, 6, 8, 16);
        printf("d|w bits: "); PrintBinaryBits(MemOp, Size, 6, 2, 8, 16);
        printf("mod bits: "); PrintBinaryBits(MemOp, Size, 8, 2, 8, 16);
        printf("reg bits: "); PrintBinaryBits(MemOp, Size, 10, 3, 8, 16);
        printf("r/m bits: "); PrintBinaryBits(MemOp, Size, 13, 3, 8, 16);
        
        const char *RegTable    [] =  { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
        const char *RegTableWide[] =  { "ax", "cx", "dx", "bx", "sb", "bp", "si", "di" };
        uint8_t Instruction = (MemOp[0]>>2);
        uint8_t Dest = ((MemOp[0]>>1)&0x1);
        uint8_t Wide = ((MemOp[0]>>0)&0x1);
        
        uint8_t Mod  = ((MemOp[1]>>6)&0x3);
        uint8_t Reg  = ((MemOp[1]>>3)&0x7);
        uint8_t RM   = ((MemOp[1]>>0)&0x7);
        
        PrintBinaryBytes(&Instruction, 1, 1,1);
        PrintBinaryBytes(&Dest, 1, 1,1);
        PrintBinaryBytes(&Wide, 1, 1,1);
        printf("\n");
        PrintBinaryBytes(&Mod, 1, 1,1);
        PrintBinaryBytes(&Reg, 1, 1,1);
        PrintBinaryBytes(&RM , 1, 1,1);
        
        const char *Op          = NULL;
        const char *OperandDest = NULL;
        const char *OperandSrc  = NULL;
        if(Mod == 0x3)
        {
            //Op type register to register
            const char *RegNameReg = Wide==1?RegTableWide[Reg]:RegTable[Reg];
            const char *RegNameRM  = Wide==1?RegTableWide[RM ]:RegTable[RM ];
            OperandSrc  = Dest==1?RegNameRM :RegNameReg;
            OperandDest = Dest==1?RegNameReg:RegNameRM;
        }
        if(Instruction == 0x22) Op = "mov";
        
        //Write to file
        if(Op && OperandDest && OperandSrc)
        {
            fputs(Op         , OutFile); fputs(" " , OutFile);
            fputs(OperandDest, OutFile); fputs(", ", OutFile);
            fputs(OperandSrc , OutFile); fputs("\n", OutFile);
        }
    }
    fclose(OutFile);
    printf("done...\n");
    return 0;
}