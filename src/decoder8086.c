#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define Kilobytes(x) (x*1024)


// What information do i need to produce an assembly file from a binary???
// which registers?
// 
typedef enum op8086_kind op8086_kind;
enum op8086_kind
{
    Op8086_Kind_Mov,
    Op8086_Kind_Push,
    Op8086_Kind_Pop,
    Op8086_Kind_Cmp,
};

typedef struct decoder_state decoder_state; 
struct decoder_state
{
    op8086_kind OpKind;
    uint32_t OpSize;
};
#include "helpers.h"
// Mov instruction page number: pg.164 / listing 4-22
// Byte 1:
// Instruction Prefix: is it a mov? what type a mov? is it some other instruction?
// w: Is the instruction we are operating on wide addresses?
// d: What is the location of the src and dest reigsters

// Byte 2:
// Mod: what are the things we are moving? reg to reg? address that needs to be calculated first? what is the size off the constant that we are using for the calculation
// Reg: what is the actual register we are reading or writing from
// 

// EAC: Effective Address Calculation
// The bit market is literally 0b01 = 1byte | 0b10 = 2 byte
#define NO_FIELDS_REMAINING (UINT32_MAX)
#define MOD_REG_TO_REG (0x3)
#define MOD_EAC_DISP8  (0x1)
#define MOD_EAC_DISP16 (0x2)

uint32_t DecodePrefix(decoder_state * State, uint8_t *MemOp)
{
    
    // MOVE
    // Immediate to register              >>>>(1011) 4bits
    // Register/memory to/from register   >>(100010) 6bits 
    // Immediate to register/memory       >(1100011) 7bits
    // Memory to accumlator               >(1010000) 7bits
    // Accumlator to memory               >(1010001) 7bits
    // Reg/mem to seg reg                 (10001110) 8bits
    // seg reg to Reg/mem                 (10001100) 8bits
    uint32_t ShiftToNextField = 0;
    uint8_t Prefix = 0;
    // TODO(MIGUEL): Could this be a table?
    // move: immediate to reg
    Prefix = (MemOp[0]>>4);
    if(Prefix == 0x0b) {
        ShiftToNextField = 3;
        goto DecodePrefixFinalize;
    }
    // move: reg/mem to/from reg
    Prefix = (MemOp[0]>>2);
    if(Prefix == 0x62) {
        ShiftToNextField = 1; 
        goto DecodePrefixFinalize;
    }
    // move: immediate to reg/mem move
    Prefix = (MemOp[0]>>1);
    if(Prefix == 0x63) {
        ShiftToNextField = 0; 
        goto DecodePrefixFinalize;
    }
    // move: accumalator move
    Prefix = (MemOp[0]>>1);
    if(Prefix == 0x51) {
        ShiftToNextField = 0; 
        goto DecodePrefixFinalize;
    }
    if(Prefix == 0x50) {
        ShiftToNextField = 0; 
        goto DecodePrefixFinalize;
    }
    // move: segmented registr
    Prefix = (MemOp[0]>>0);
    if(Prefix == 0x8c) {
        ShiftToNextField = NO_FIELDS_REMAINING; 
    }
    if(Prefix == 0x8e) {
        ShiftToNextField = NO_FIELDS_REMAINING; 
        
    }
    DecodePrefixFinalize:
    
    return ShiftToNextField;
}

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
        // NOTE(MIGUEL): We need to decode the prefix the first as it detrmines what type of mov
        //               and if we the ohter fields like d or where to read the reg register.
        
        // Starting with the smalllest prefiex
        //- Prefiex Decoding
        decoder_state DecoderState = {0};
        uint32_t ShiftToNextField = DecodePrefix(&DecoderState, MemOp);
        //-
        uint8_t Dest = ((MemOp[0]>>1)&0x1);
        uint8_t Wide = ((MemOp[0]>>0)&0x1);
        
        uint8_t Mod  = ((MemOp[1]>>6)&0x3);
        uint8_t Reg  = ((MemOp[1]>>3)&0x7);
        uint8_t RM   = ((MemOp[1]>>0)&0x7);
        
        //PrintBinaryBytes(&Prefix, 1, 1,1);
        PrintBinaryBytes(&Dest, 1, 1,1);
        PrintBinaryBytes(&Wide, 1, 1,1);
        printf("\n");
        PrintBinaryBytes(&Mod, 1, 1,1);
        PrintBinaryBytes(&Reg, 1, 1,1);
        PrintBinaryBytes(&RM , 1, 1,1);
        
        const char *Op          = NULL;
        const char *OperandDest = NULL;
        const char *OperandSrc  = NULL;
        // TODO(MIGUEL): Decode an immediateto register move
        // for immediate to reg move depending on the w bit a byte or word string will be output as nasm in square bracketes. denotes how much memory is useed to encodea an immediate???
        
        // dest v      v src <- can be changed with the d(Dest) bit? 
        //  reg v         v this is the displacment
        // mov bx, [bp + 75]
        // Decoding this give us an idea of how long the instruction is 
        switch(Mod)
        {
            case MOD_REG_TO_REG:
            {
                //Op type register to register
                const char *RegNameReg = Wide==1?RegTableWide[Reg]:RegTable[Reg];
                const char *RegNameRM  = Wide==1?RegTableWide[RM ]:RegTable[RM ];
                OperandSrc  = Dest==1?RegNameRM :RegNameReg;
                OperandDest = Dest==1?RegNameReg:RegNameRM;
            }break;
            case MOD_EAC_DISP8:
            {
                // effective address calculation
            }break;
            case MOD_EAC_DISP16:
            {
                // direct address  calctulation
            }break;
            default:
            {
                
            }break;
        }
        if(Prefix == 0x22) Op = "mov";
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