#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define Kilobytes(x) (x*1024)

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
typedef enum op_kind op_kind;
enum op_kind
{
  OpKind_Null = 0x00,
  OpKind_Mov,
  OpKind_Push,
  OpKind_Pop,
};
typedef enum operand_kind operand_kind;
enum operand_kind
{
  Operand_Src,
  Operand_Dest,
};
typedef enum op_subkind op_subkind;
enum op_subkind
{
  OpSubKind_ImmToRegMem,
  OpSubKind_ImmFromRegMem,
};

//~ TABLES
uint8_t *RegTable    [] =  { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
uint8_t *RegTableWide[] =  { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" };
uint64_t OpMnemonicStringTable[][2] = 
{
  { OpKind_Mov, (uint64_t)"mov" },
  { OpKind_Push, (uint64_t)"push" },
  { OpKind_Pop, (uint64_t)"pop" },
};
uint32_t OpcodePrefixTable8Bit[][2] = 
{
  //mov
  { 0x8e,  OpKind_Mov }, // Reg/mem to seg reg                 (10001110) 8bits
  { 0x8c,  OpKind_Mov }, // seg reg to Reg/mem                 (10001100) 8bits
};
uint32_t OpcodePrefixTable7Bit[][2] = 
{
  //mov
  { 0x63, OpKind_Mov }, // Immediate to register/memory       >(1100011) 7bits 0
  { 0x50, OpKind_Mov }, // Memory to accumlator               >(1010000) 7bits 0
  { 0x31, OpKind_Mov }, // Accumlator to memory               >(1010001) 7bits 0
};
uint32_t OpcodePrefixTable6Bit[][2] = 
{
  //mov
  { 0x22 , OpKind_Mov},// Register/memory to/from register   >>(100010) 6bits 0 - 0
};
uint32_t OpcodePrefixTable4Bit[][2] = 
{
  //mov
  { 0x0b, OpKind_Mov }, // Immediate to register              >>>>(1011) 4bits 0 - 000
};
//~ END TABLES


// What information do i need to produce an assembly file from a binary???
// which registers?
// 
typedef struct instruction instruction;
struct instruction
{
  //op related
  op_kind OpKind;
  uint32_t Size;
  uint32_t OpBitWidth;
  uint32_t Mod;        // mod
  uint32_t Reg;        // reg
  uint32_t RegMem;     // r/m
  uint32_t IsWide;     // w
  uint32_t IsRegDest;  // d
  uint32_t SignExt;    // s
  uint32_t LoopOnZero; // z
};
typedef struct decoder_state decoder_state; 
struct decoder_state
{
  //decoder related
  uint8_t *InstructionStream;
  uint32_t StreamOffset;
  uint64_t StreamSize;
};
void DecoderStateInit(decoder_state *DecoderState, uint8_t *Stream, uint64_t StreamSize)
{
  DecoderState->StreamOffset = 0;
  DecoderState->InstructionStream = Stream;
  DecoderState->StreamSize = StreamSize;
  return;
}
uint8_t *DecoderGetCurrentStreamByte(decoder_state *DecoderState)
{
  uint8_t *Result = &DecoderState->InstructionStream[DecoderState->StreamOffset];
  return Result;
}
void DecoderAdvanceStream(decoder_state *DecoderState, instruction *Instruction)
{
  DecoderState->StreamOffset += Instruction->Size;
  return;
}
uint32_t DecoderIsDecodingStream(decoder_state *DecoderState)
{
  uint32_t Result = (DecoderState->StreamOffset<DecoderState->StreamSize);
  return Result;
}
uint8_t *GeneratorGetMnemonic(instruction *Instruction)
{
  uint8_t *Result = NULL;
  for(uint32_t i=0; i<ArrayCount(OpMnemonicStringTable); i++)
  {
    if(OpMnemonicStringTable[i][0] == Instruction->OpKind)
    {
      Result = (uint8_t *)OpMnemonicStringTable[i][1];
    }
  }
  return Result;
}
uint8_t *GeneratorGetOperand(instruction *Instruction, operand_kind OperandKind)
{
  uint8_t *Result = NULL;
  uint8_t **Table = Instruction->IsWide==1?RegTableWide:RegTable;
  Result = (OperandKind==Operand_Dest?
            Instruction->IsRegDest==1?Table[Instruction->Reg]:Table[Instruction->RegMem]:
            Instruction->IsRegDest==0?Table[Instruction->Reg]:Table[Instruction->RegMem]);
  return Result;
}
void GeneratorWriteOp(uint8_t *Op, uint8_t *OperandDest, uint8_t *OperandSrc, FILE *OutFile)
{
  //Write to file
  if(Op && OperandDest && OperandSrc)
  {
    fputs(Op         , OutFile); fputs(" " , OutFile);
    fputs(OperandDest, OutFile); fputs(", ", OutFile);
    fputs(OperandSrc , OutFile); fputs("\n", OutFile);
  }
  return;
}
void DecodePrefix(decoder_state *DecoderState, instruction *Instruction)
{
  uint8_t *MemOp = DecoderGetCurrentStreamByte(DecoderState);
  uint8_t Prefix = 0;
  Prefix = (MemOp[0]>>4);
  for(uint32_t i=0; i<ArrayCount(OpcodePrefixTable4Bit); i++)
  {
    if(OpcodePrefixTable4Bit[i][0] == Prefix)
    {
      Instruction->OpBitWidth = 4;
      Instruction->OpKind = OpcodePrefixTable4Bit[i][1];
      goto DecodePrefixFinalize;
    }
  }
  Prefix = (MemOp[0]>>2);
  for(uint32_t i=0; i<ArrayCount(OpcodePrefixTable6Bit); i++)
  {
    if(OpcodePrefixTable6Bit[i][0] == Prefix)
    {
      Instruction->OpBitWidth = 6;
      Instruction->OpKind = OpcodePrefixTable6Bit[i][1];
      goto DecodePrefixFinalize;
    }
  }
  Prefix = (MemOp[0]>>1);
  for(uint32_t i=0; i<ArrayCount(OpcodePrefixTable7Bit); i++)
  {
    if(OpcodePrefixTable7Bit[i][0] == Prefix)
    {
      Instruction->OpBitWidth = 7;
      Instruction->OpKind = OpcodePrefixTable7Bit[i][1];
      goto DecodePrefixFinalize;
    }
  }
  Prefix = (MemOp[0]>>0);
  for(uint32_t i=0; i<ArrayCount(OpcodePrefixTable7Bit); i++)
  {
    if(OpcodePrefixTable7Bit[i][0] == Prefix)
    {
      Instruction->OpBitWidth = 8;
      Instruction->OpKind = OpcodePrefixTable7Bit[i][1];
      goto DecodePrefixFinalize;
    }
  }
  DecodePrefixFinalize:
  return;
}
void DecodeRemainingFields(decoder_state *DecoderState, instruction *Instruction)
{
  uint8_t *MemOp = DecoderGetCurrentStreamByte(DecoderState);
  //per instructhin there are 4 possible permutations with >10 instructions = 40 code paths
  if(Instruction->OpBitWidth == 8)
  {
    
  }
  if(Instruction->OpBitWidth == 7)
  {
    
  }
  if(Instruction->OpBitWidth == 6)
  {
    switch(Instruction->OpKind)
    {
      case OpKind_Mov:
      {
        //byte 1
        Instruction->IsRegDest = ((MemOp[0]>>1)&0x1);
        Instruction->IsWide    = ((MemOp[0]>>0)&0x1);
        //byte 2
        Instruction->Mod       = ((MemOp[1]>>6)&0x3);
        Instruction->Reg       = ((MemOp[1]>>3)&0x7);
        Instruction->RegMem    = ((MemOp[1]>>0)&0x7);
        Instruction->Size = 2;
      } break;
    }
  }
  if(Instruction->OpBitWidth == 4)
  {
    
  }
  
  return;
}
int main()
{
  //OPEN INFILE
  printf("8086 decoder\n");
  FILE *InFile = fopen(".\\decodeme_long", "rb"); //binary
  
  //GET MEMORY
  uint32_t Size = GetFileSize(InFile);
  printf("file size: %u bytes\n", Size);
  uint8_t *Buffer = malloc(Kilobytes(1));
  fread(Buffer, Size, 1, InFile);
  fclose(InFile);
  
  //MAKE OUTFILE
  FILE *OutFile = fopen("..\\data\\decoded_long.asm", "w+"); //asemmbly file
  fputs("bit 16\n\n", OutFile);
  
  decoder_state DecoderState = {0};
  DecoderStateInit(&DecoderState, Buffer, Size);
  PrintBinaryBytes(DecoderState.InstructionStream, (uint32_t)DecoderState.StreamSize, 1, 2);
  while(DecoderIsDecodingStream(&DecoderState))
  {
    uint8_t *MemOp  = DecoderGetCurrentStreamByte(&DecoderState);
    printf("\n/NEXT INSTRUCTION ==========================/\n");
    printf("entire instruction: "); PrintBinaryBits(MemOp, Size, 0, 16, 8, 16);
    printf("mov bit pattern: "); PrintBinaryBits(MemOp, Size, 0, 6, 8, 16);
    printf("d|w bits: "); PrintBinaryBits(MemOp, Size, 6, 2, 8, 16);
    printf("mod bits: "); PrintBinaryBits(MemOp, Size, 8, 2, 8, 16);
    printf("reg bits: "); PrintBinaryBits(MemOp, Size, 10, 3, 8, 16);
    printf("r/m bits: "); PrintBinaryBits(MemOp, Size, 13, 3, 8, 16);
    
    
    instruction Instruction = {0};
    DecodePrefix(&DecoderState, &Instruction);
    DecodeRemainingFields(&DecoderState, &Instruction);
    
    //CL UI
    PrintBinaryBytes(&Instruction.IsRegDest, 1, 1,1);
    PrintBinaryBytes(&Instruction.IsWide, 1, 1,1);
    printf("\n");
    PrintBinaryBytes(&Instruction.Mod, 1, 1,1);
    PrintBinaryBytes(&Instruction.Reg, 1, 1,1);
    PrintBinaryBytes(&Instruction.RegMem , 1, 1,1);
    //CL UI
    
    uint8_t *Op          = GeneratorGetMnemonic(&Instruction);
    uint8_t *OperandDest = NULL;
    uint8_t *OperandSrc  = NULL;
    switch(Instruction.Mod)
    {
      case MOD_REG_TO_REG:
      {
        //Op type register to register
        OperandDest = GeneratorGetOperand(&Instruction, Operand_Dest);
        OperandSrc  = GeneratorGetOperand(&Instruction, Operand_Src);
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
    GeneratorWriteOp(Op, OperandDest, OperandSrc, OutFile);
    
    DecoderAdvanceStream(&DecoderState, &Instruction);
  }
  fclose(OutFile);
  printf("done...\n");
  return 0;
}