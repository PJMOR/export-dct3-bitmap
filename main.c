#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

void main()
{
    FILE *flash;
    unsigned long lenFlash;

    flash = fopen("input.fls", "rb");
    fseek(flash, 0, SEEK_END);
    lenFlash = ftell(flash);
    rewind(flash);

    unsigned char bytesFlash[lenFlash]; //array with file's bytes
    fread(bytesFlash, lenFlash, 1, flash);
    fclose(flash);

    unsigned char subFind[3] = {0xD1, 0x07, 0x1C};                                    //3 bytes to be found from the subroutine
    long subLoc = (long)memmem(bytesFlash, lenFlash, subFind, 3) - (long)&bytesFlash; //first possible subroutine location

    while (1)
    {
        if ((bytesFlash[subLoc - 1] == 0x0C && bytesFlash[subLoc + 3] == 0x01
          || bytesFlash[subLoc - 1] == 0x00 && bytesFlash[subLoc + 3] == 0x28
          || bytesFlash[subLoc - 1] == 0x00 && bytesFlash[subLoc + 3] == 0x29
          || bytesFlash[subLoc - 1] == 0x03 && bytesFlash[subLoc + 3] == 0x20)
          && bytesFlash[subLoc + 4] == 0x1C && bytesFlash[subLoc + 5] >= 0x41)   //bytes to check whether it's the subroutine
        {
            break;
        }

        else
        {
            subLoc = (long)memmem(bytesFlash + subLoc + 1, lenFlash - subLoc, subFind, 3) - (long)&bytesFlash;
            if (subLoc < 0 && subFind[1] == 0x07)
            {
                subLoc = 0;
                subFind[1] = 0x06;      //if it isn't D1 07 1C, check D1 06 1C
            }

            if (subLoc < 0 && subFind[1] == 0x06)
            {
                printf("Error: it's not D1 07 1C or D1 06 1C\n");
            }
        }
    }
    //printf("subLoc: %li\n", subLoc);

    int x = subLoc;
    int xDiff = -2;
    unsigned long offset[3] = {0};   //offset[0] = ff block offset, offset[1] = list block offset, offset[2] = bitmap metadata block offset
    unsigned char offsetCount = 0;
    unsigned short mode = 1;
    int i, j, k, l;

    while (1)
    {
        if (bytesFlash[x] == 0xb5)          //if subroutine beginning found, stop searching backwards, search forwards
        {
            x = subLoc + 10;
            xDiff = 2;
        }
        else if (bytesFlash[x] == 0xbd)     //if subroutine end found, break loop
        {
            break;
        }
        else if (bytesFlash[x] < 0x48 || bytesFlash[x] > 0x4F)  //check if instruction isn't a PC-relative LDR (0b01001xxxxxxxxxxx)
        {
            if (bytesFlash[x] == 0xd0)                          //if a BEQ label (0x11010000xxxxxxxx) is found, switch pixel data to mode 0
            {
                mode = 0;
            }
            x += xDiff;
        }
        else                                                    //PC-relative load found
        {
            if (bytesFlash[x + (((bytesFlash[x + 1]) << 2) + 4) + 1 - (x & 2)] >= 0x20)   //check if value at address pointed by LDR points into ROM (beyond 0x200000) or RAM (below 0x200000)
            {
                for (i = 0; i < 4; i++)                                     //concatenate 4 bytes following the address pointed by LDR and save into array
                {
                    offset[offsetCount] += bytesFlash[x + (((bytesFlash[x + 1]) << 2) + 4 - (x & 2)) + i] << ((3 - i) * 8);
                }
                offset[offsetCount] -= 0x200000;         //subtract RAM offset
                //printf("%lx\n", offset[offsetCount]);
                offsetCount++;
            }
            x += xDiff;
        }
    }

    if (offset[2] == 0)                 //some firmwares don't point to the metadata chunk offset within the subroutine
    {
        unsigned long metaLoc = 0;
        for (i = 3; i >= 0; i--)        //the offset can be found at an address pointed to within the metadata
        {
            metaLoc += bytesFlash[offset[0] - 17 - i] << (i * 8);
        }
        metaLoc -= 0x200000;
        //printf("%lx", metaLoc);
        for (i = 4; i > 0; i--)         //this is the offset for the metadata chunk beginning
        {
            offset[2] += bytesFlash[metaLoc - i] << ((i - 1) * 8);
        }
        offset[2] -= 0x200000;
        //printf("\n%lx\n", offset[2]);
    }

    printf("ff block offset:   0x%06lx\n", offset[0]);
    printf("list block offset: 0x%06lx\n", offset[1]);
    printf("meta block offset: 0x%06lx\n", offset[2]);

    unsigned short bitmapNo = (offset[0] - offset[2]) / 12;        //the metadata is split into smaller chunks of 12 bytes, each for a bitmap
    unsigned short height;
    unsigned short width;
    unsigned long bitmapOffset;

    printf("\nNumber of bitmaps: %i\n", bitmapNo);
    FILE *output;
    mkdir("bitmaps", 0777);

    for (i = 0; i < bitmapNo; i++)
    {
        bitmapOffset = 0;                               //each 12-byte bitmap metadata contains the memory offset for the pixel data
        for (j = 3; j >= 0; j--)
        {
            bitmapOffset += bytesFlash[offset[2] + (12 * i) + 3 - j] << (j * 8);
        }
        //printf("\n%lx", bitmapOffset);
        if (bitmapOffset > 0)                           //there are a couple of 12-byte chunks that don't point to any bitmaps, the address is 0
        {
            bitmapOffset -= 0x200000;
            width = bytesFlash[offset[2] + (12 * i) + 8];
            height = bytesFlash[offset[2] + (12 * i) + 9];
            //printf(" %x %x", width, height);

            unsigned char bitmapName[20];

            sprintf(bitmapName, "./bitmaps/%04i.pbm", i);
            output = fopen(bitmapName, "wb");
            fprintf(output, "P1\n %d\n %d\n", width, height);

            unsigned char pixelBuffer[864] = {0};
            unsigned short countNo = 0;            //number of "count" bytes
            unsigned short pixelDataLen = 0;       //sum of nibbles within "count" bytes
            k = 0;

            if (mode == 0)      //mode = 0 when pixel data is split in a pattern of 1 byte, followed by 2 bytes
            {                   //The first byte is split into 2 nibbles, each specifying how many times the following 2 bytes are repeated
                while (1)
                {
                    pixelDataLen += ((bytesFlash[bitmapOffset + (3 * countNo)] & 0xF0) >> 4) + (bytesFlash[bitmapOffset + (3 * countNo)] & 0x0F);   //add together first 4 bits and second 4 bits of the count bytes

                    for (j = 0; j < ((bytesFlash[bitmapOffset + (3 * countNo)] & 0xF0) >> 4); j++)      //repeat first byte after each count byte by a factor determined by the first 4 bits of the count byte
                    {
                        pixelBuffer[k + j] = bytesFlash[bitmapOffset + (3 * countNo) + 1];
                    }
                    k += ((bytesFlash[bitmapOffset + (3 * countNo)] & 0xF0) >> 4);

                    for (j = 0; j < (bytesFlash[bitmapOffset + (3 * countNo)] & 0x0F); j++)             //repeat second byte after each count byte by a factor determined by the second 4 bits of the count byte
                    {
                        pixelBuffer[k + j] = bytesFlash[bitmapOffset + (3 * countNo) + 2];
                    }
                    k += (bytesFlash[bitmapOffset + (3 * countNo)] & 0x0F);

                    countNo++;
                    if (pixelDataLen == (((height + 7) / 8) * width))               //((height + 7) / 8) is an upper rounding of (height / 8)
                    {
                        //printf(", %x, %i", pixelDataLen / ((height + 7) / 8), countNo);
                        break;
                    }
                }
            }

            else if (mode == 1)     //mode = 1 when pixel data isn't split
            {
                for (j = 0; j < (((height + 7) / 8) * width); j++)
                {
                    pixelBuffer[j] = bytesFlash[bitmapOffset + j];
                }
            }

            for (j = 0; j < (((height + 7) / 8) * width); j += width)       //the bitmaps are split into bytes, each 8 pixel columns, from left to right, top to bottom
            {
                for (k = 0; k < 8; k++)
                {
                    for (l = j; l < j + width; l++)
                    {
                        int result = 1;
                        for (int exp = k; exp != 0; exp--)
                        {
                            result *= 2;
                        }
                        fprintf(output, "%i ", ((pixelBuffer[l] & result) >> k));
                    };
                };
            };
            fclose(output);
        }
    }
}
