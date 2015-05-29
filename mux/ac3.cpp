#include "ac3.h"
#include <string.h>
#include <stdio.h>

namespace ac3
{
    inline unsigned int htonl(unsigned int s)
    {
        return ((s>>24)&0x000000ff)|((s>>8)&0x0000ff00)|((s<<8)&0x00ff0000)|((s<<24)&0xff000000);
    }

    const short bit_rate[]=
    {
        32,32,40,40,48,48,56,56,64,64,80,80,96,96,112,112,128,128,160,160,192,192,224,224,256,256,
        320,320,384,384,448,448,512,512,576,576,640,640
    };

    static const short frame_size_32khz[]=
    {
        96,96,120,120,144,144,168,168,192,192,240,240,288,288,336,336,384,384,480,480,576,576,672,672,768,768,960,
            960,1152,1152,1344,1344,1536,1536,1728,1728,1920,1920
    };

    static const short frame_size_44khz[]=
    {
        69,70,87,88,104,105,121,122,139,140,174,175,208,209,243,244,278,279,348,349,417,418,487,488,557,558,696,
            697,835,836,975,976,1114,1115,1253,1254,1393,1394
    };

    static const short frame_size_48khz[]=
    {
        64,64,80,80,96,96,112,112,128,128,160,160,192,192,224,224,256,256,320,320,384,384,448,448,512,512,640,640,
            768,768,896,896,1024,1024,1152,1152,1280,1280
    };

    static const short* frame_size[]=
    {
        frame_size_48khz,
        frame_size_44khz,
        frame_size_32khz,
        0
    };

}

int ac3::parser::get_next_unit(char* dst,int len)
{
    int offset=0;

    int unit_len=0;

    for(;;)
    {
        int ch=reader->getch();

        if(ch==demux::file::eof)
            return offset;

        ctx=(ctx<<8)+(unsigned char)ch;

        switch(st)
        {
        case 0:
            if((ctx&0xffff0000)==0x0b770000)
                st=1;
            break;
        case 1:
            {
                const short* s=frame_size[(ch>>6)&0x03];
                if(!s)
                    return -1;

                unit_len=s[ch&0x3f]*2;

                if(unit_len>len)
                    return -2;

                *((unsigned char*)dst)=0x0b;
                *((unsigned int*)(dst+1))=htonl(ctx);

                offset+=5;

                unit_len-=5;
            }
            st=2;
            break;
        case 2:
            ((unsigned char*)dst)[offset++]=ch;
            unit_len--;
            if(unit_len<1)
            {
                st=0;
                return offset;
            }
            break;
        }
    }

    return 0;
}

const char* ac3::get_ac3_sample_rate_name(const char* p)
{
    static const char* names[]=
    {
        "48 kHz",
        "44.1 kHz",
        "32 kHz",
        "?? kHz"
    };

    return names[get_ac3_sample_rate(p)];
}

int ac3::get_ac3_frame_len(const char* p)
{
    int n=((unsigned char*)p)[4];

    int m=n&0x3f;

    const short* s=frame_size[(n>>6)&0x03];
    int br=bit_rate[m];

    if(!s)
        return 0;

    int bits=s[m]*16;

//    int ticks=bits*90/br        // 1ms = 90 ticks

// frame_num * frame_len_bits * 90 / (bit_rate/1000) = timecode in ticks of frame

    return bits/br;
}
