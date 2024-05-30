#include <stdint.h>
#include <stdio.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef u32 b32;

#define false 0
#define true 1
#define Assert(Expression) if(!(Expression)) {*(u32 *)0 = 0;}

typedef struct
{
    size_t FileSize;
    u8 *Contents;
} file_contents;

static file_contents
ReadEntireFileIntoMemory(char *FileName)
{
    file_contents Result = {0};

    FILE *File = fopen(FileName, "rb");
    if(File)
    {
        fseek(File,0,SEEK_END);
        Result.FileSize = ftell(File);
        fseek(File,0,SEEK_SET);

        Result.Contents = (u8 *)malloc(Result.FileSize);
        fread(Result.Contents, Result.FileSize, 1, File);

        fclose(File);

    }
    else
    {
        fprintf(stderr, "Unable to read file %s\n", FileName);
    }

    return Result;
}

static size_t
LZCompress(size_t InSize, u8 *InBase, size_t MaxOutSize, u8 *OutBase)
{
    u8 *Out = OutBase;
    u8 *In = InBase;

#define MAX_LOOKBACK_COUNT 255    
#define MAX_LITERAL_COUNT 255
#define MAX_RUN_COUNT 255
    int LiteralCount = 0;
    u8 Literals[MAX_LITERAL_COUNT];

    u8 *InEnd = In + InSize;
    
    while(In < InEnd)
    {
        size_t MaxLookBack = In - InBase;
        if(MaxLookBack > MAX_LOOKBACK_COUNT)
        {
            MaxLookBack = MAX_LOOKBACK_COUNT;
        }
        
        size_t BestRun = 0;
        size_t BestDistance = 0;
        for(u8 *WindowStart = In - MaxLookBack;
            WindowStart < In;
            ++WindowStart)
        {
            size_t WindowSize = InEnd - WindowStart;
            if(WindowSize > MAX_RUN_COUNT)
            {
                WindowSize = MAX_RUN_COUNT;
            }
            u8 *WindowEnd = WindowStart + WindowSize;

            u8 *TestIn = In;
            u8 *WindowsIn = WindowStart;
            size_t TestRun = 0;
            
            while((WindowsIn < WindowEnd) && (*TestIn++ == *WindowsIn++))
            {
                ++TestRun;
            }

            if(BestRun < TestRun)
            {
                BestRun = TestRun;
                BestDistance = In - WindowStart;
            }
        }
        

        b32 OutputRun = false;

        if(LiteralCount)
        {
            OutputRun =  (BestRun > 4);
        }
        else
        {
            OutputRun =  (BestRun > 2);
        }
           
        if(OutputRun || (LiteralCount == MAX_LITERAL_COUNT))
        {
            u8 LiteralCount8 = (u8)LiteralCount;
            Assert(LiteralCount8 == LiteralCount);
            if(LiteralCount8)
            {
                
                *Out++ = LiteralCount8;
                *Out++ = 0;
                
                for(int LiteralIndex = 0;
                    LiteralIndex < LiteralCount;
                    ++LiteralIndex)
                {
                    *Out++ = Literals[LiteralIndex];
                }
                LiteralCount = 0;
            }

            if(OutputRun)
            {
                u8 Run8 = (u8)BestRun;
                Assert(Run8 == BestRun);
            
                *Out++ = Run8;
                u8 Distance8 = (u8)BestDistance;
                Assert(Distance8 == BestDistance);
                *Out++ = Distance8;

                In += BestRun;
            }
        }
        else
        {
            Literals[LiteralCount++] = *In++;
           
        }
    }
#undef MAX_LITERAL_COUNT
#undef MAX_RUN_COUNT
#undef MAX_LOOKBACK_COUNT

    Assert(In == InEnd);

    size_t OutSize = Out - OutBase;
    Assert(OutSize <= MaxOutSize);

    return OutSize;
}

static void
LZDecompress(size_t InSize, u8 *In, size_t OutSize, u8 *Out)
{
    u8 *InEnd = In + InSize;
    while(In < InEnd)
    {
        int Count = *In++;
        u8 CopyDistance = *In++;


        u8 *Source = (Out - CopyDistance);
        if(CopyDistance == 0)
        {
            Source = In;
            In += Count;
        }
        
        while(Count--)
        {
            *Out++ = *Source++;
        }        
    }

    Assert(In == InEnd);
}

int
main(int ArgCount, char **Args)
{    
    if(ArgCount == 4)
    {
        size_t FinalOutputSize = 0;
        u8 *FinalOutputBuffer = 0;
        
        char *Command = Args[1];
        char *InFileName = Args[2];
        char *OutFileName = Args[3];
        file_contents InFile = ReadEntireFileIntoMemory(InFileName);
        
        if(strcmp(Command, "compress") == 0)
        {
            size_t OutBufferSize = InFile.FileSize*2;
            u8 *OutBuffer = (u8 *)malloc(OutBufferSize + 4);
            size_t CompressedSize = LZCompress(InFile.FileSize, InFile.Contents, OutBufferSize, OutBuffer + 4);
            *(u32 *)OutBuffer = (u32)InFile.FileSize;

            FinalOutputSize = CompressedSize + 4;
            FinalOutputBuffer = OutBuffer;
        }
        else if(strcmp(Command, "decompress") == 0)
        {
            if(InFile.FileSize >= 4)
            {
                size_t OutBufferSize = *(u32 *)InFile.Contents;
                u8 *OutBuffer = (u8 *)malloc(OutBufferSize);
                LZDecompress(InFile.FileSize - 4, InFile.Contents + 4, OutBufferSize, OutBuffer);

                FinalOutputSize = OutBufferSize;
                FinalOutputBuffer = OutBuffer;
            }
        }
        else
        {
            fprintf(stderr, "Usage: %s compress [raw filename] [compressed filename]\n", Args[0]);
            fprintf(stderr, "       %s decompress [compressed filename] [raw filename]\n", Args[0]);
        }
        
        if(FinalOutputBuffer)
        {
            FILE *OutFile = fopen(OutFileName, "wb");
            if(OutFile)
            {
                fwrite(FinalOutputBuffer, 1, FinalOutputSize, OutFile);
            }
            else
            {
                fprintf(stderr, "Unable to open output file %s\n", OutFileName);
            }
        }
    }
    else
    {
        fprintf(stderr, "Usage: %s compress [raw filename] [compressed filename]\n", Args[0]);
        fprintf(stderr, "       %s decompress [compressed filename] [raw filename]\n", Args[0]);
    }    
}