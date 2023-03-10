#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define Kilobytes(x) (x*1024)

void PrintBinaryBytes(uint8_t *Buffer, uint32_t Size,
                      uint32_t ByteSplitMod, uint32_t ByteNewlineMod)
{
    if(!(ByteNewlineMod && ByteSplitMod)) return;
    for(uint32_t ByteId=0;ByteId<Size;ByteId++)
    {
        uint8_t Byte = Buffer[ByteId];
        printf("%c%c%c%c%c%c%c%c", 
               (0x1&(Byte>>7))==1?'1':'0',
               (0x1&(Byte>>6))==1?'1':'0',
               (0x1&(Byte>>5))==1?'1':'0',
               (0x1&(Byte>>4))==1?'1':'0',
               (0x1&(Byte>>3))==1?'1':'0',
               (0x1&(Byte>>2))==1?'1':'0',
               (0x1&(Byte>>1))==1?'1':'0',
               (0x1&(Byte>>0))==1?'1':'0');
        
        if     (((ByteId+1)%ByteNewlineMod)==0) {printf("\n");}
        else if(((ByteId+1)%ByteSplitMod  )==0) {printf(" " );}
    }
    //printf("\n");
    return;
}
void PrintBinaryBits(uint8_t *Buffer, uint32_t Size, 
                     uint32_t BitOffset, uint32_t BitCount,
                     uint32_t BitSplitMod, uint32_t BitNewlineMod)
{
    if(!((BitOffset+(BitCount-1))<(Size*8))) return;
    
    for(uint32_t BitId=BitOffset, PenPos=0;
        BitId<(BitOffset+BitCount);
        BitId++, PenPos++)
    {
        uint32_t ByteId = BitId/8;
        uint8_t  Byte   = Buffer[ByteId];
        uint8_t  ByteRelBitId = BitId%8;
        uint8_t  Shift        = 7-(ByteRelBitId);
        printf("%c", (0x1&(Byte>>Shift))==1?'1':'0');
        if     (((PenPos+1)%BitNewlineMod)==0) {printf("\n");}
        else if(((PenPos+1)%BitSplitMod  )==0) {printf(" " );}
    }
    printf("\n");
    return;
}
uint32_t GetFileSize(FILE *File)
{
    fseek(File, 0, SEEK_END);
    uint32_t Size = ftell(File);
    fseek(File, 0, SEEK_SET);
    return Size;
}
int main()
{
    printf("8086 decoder\n");
    FILE *File = fopen(".\\decodeme", "rb");
    
    uint32_t Size = GetFileSize(File);
    printf("file size: %u bytes\n", Size);
    
    uint8_t *Buffer = malloc(Kilobytes(1));
    fread(Buffer, Size, 1, File);
    PrintBinaryBytes(Buffer, Size, 1, 2);
    printf("mov bit pattern: "); PrintBinaryBits (Buffer, Size, 0, 6, 8, 16);
    printf("d|w bits: "); PrintBinaryBits (Buffer, Size, 6, 2, 8, 16);
    printf("mod bits: "); PrintBinaryBits (Buffer, Size, 8, 2, 8, 16);
    printf("reg bits: "); PrintBinaryBits (Buffer, Size, 10, 3, 8, 16);
    printf("r/m bits: "); PrintBinaryBits (Buffer, Size, 13, 3, 8, 16);
    
    printf("done...\n");
    
    return 0;
}