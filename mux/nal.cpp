#include "nal.h"
#include <string.h>

int h264::parser::get_next_unit(char* dst,int len)
{
    const char nal_start_code[3]={ 0x00, 0x00, 0x01 };

    int offset=0;

    for(;;)
    {
        int ch=reader->getch();

        if(ch==demux::file::eof)
            return offset;

        ctx=(ctx<<8)+(unsigned char)ch;

        if((ctx&0x00ffffff)==0x00000001)
        {
            if(!st)
            {
                st=1;
                continue;
            }else
            {
                st=1;
                return offset-2;
            }
        }

        if(st==1)
        {
            if(len-offset<sizeof(nal_start_code))
                return -1;

            memcpy(dst+offset,nal_start_code,sizeof(nal_start_code));
            offset+=sizeof(nal_start_code);
            st=2;
        }

        if(st==2)
        {
            if(len-offset<=0)
                return -2;

            ((unsigned char*)dst)[offset++]=ch;
        }
    }

    return 0;
}

const char* h264::get_nal_unit_type_name(const char* p)
{
    static const char* names[]=
    {
        "Unspecified",
        "Coded slice of a non-IDR picture",
        "Coded slice data partition A",
        "Coded slice data partition B",
        "Coded slice data partition C",
        "Coded slice of an IDR picture",
        "Supplemental enhancement information (SEI)",
        "Sequence parameter set",
        "Picture parameter set",
        "Access unit delimiter",
        "End of sequence",
        "End of stream",
        "Filler data",
        "Sequence parameter set extension *",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Coded slice of an auxiliary coded picture without partitioning *",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Unspecified",
        "Unspecified",
        "Unspecified",
        "Unspecified",
        "Unspecified",
        "Unspecified",
        "Unspecified",
        "Unspecified"
    };

    int n=get_nal_unit_type(p);

    if(n<0 || n>31)
        return "Unknown NAL unit";

    return names[n];
}

const char* h264::get_nal_access_unit_type_name(const char* p)
{
    static const char* names[]={ "I", "I,P", "I,P,B", "SI", "SI,SP", "I,SI", "I,SI,P,SP", "I,SI,P,SP,B" };

    int n=get_nal_access_unit_type(p);

    if(n<0 || n>7)
        return "???";

    return names[n];
}

const char* h264::get_nal_seq_p_h264_profile_name(const char* p)
{
    switch(get_nal_seq_p_h264_profile(p))
    {
    case 66:  return "Baseline Profile";
    case 77:  return "Main Profile";
    case 88:  return "Extended Profile";
    case 100: return "High Profile";
    case 110: return "High 10 Profile";
    case 122: return "High 4:2:2 Profile";
    case 144: return "High 4:4:4 Profile";
    }
    return "Unknown Profile";
}

const char* h264::get_nal_unit_slice_type_name(const char* p)
{
    static const char* names[]={ "P", "B", "I", "SP", "SI", "P", "B", "I", "SP", "SI" };

    int n=get_nal_unit_slice_type(p);

    if(n<0 || n>9)
        return "???";

    return names[n];
}
