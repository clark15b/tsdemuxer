#include <stdio.h>
#include "reader.h"
#include "nal.h"
#include "ac3.h"
#include <unistd.h>


void parse_ac3(demux::parser* p)
{
    char unit[4096];

    for(;;)
    {
        int n=p->get_next_unit(unit,sizeof(unit));

        if(n<1)
            break;

        printf("%i kbps, %s, %ims\n",ac3::get_ac3_bit_rate(unit),ac3::get_ac3_sample_rate_name(unit),ac3::get_ac3_frame_len(unit));
    }
}

void parse_h264(demux::parser* p)
{
    char unit[512000];

    for(;;)
    {
        int n=p->get_next_unit(unit,sizeof(unit));

        if(n<1)
            break;

        printf("%s\n",h264::get_nal_unit_type_name(unit));

        switch(h264::get_nal_unit_type(unit))
        {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            printf("   %s-frame\n",h264::get_nal_unit_slice_type_name(unit));
            break;
        case 7:
            printf("   %s %i.%i\n",h264::get_nal_seq_p_h264_profile_name(unit),h264::get_nal_seq_p_h264_level(unit)/10,
                h264::get_nal_seq_p_h264_level(unit)%10);
            break;
        case 9:
            printf("   %s\n",h264::get_nal_access_unit_type_name(unit));
            break;
        }
    }
}

void split_to_files(demux::parser* p,int n)
{
    char unit[512000];

    for(int i=0;;i++)
    {
        if(n!=-1 && i>=n)
            break;

        char name[512];
        sprintf(name,"unit_%.4x.bin",i);
        FILE* fp=fopen(name,"w");
        if(fp)
        {
            int n=p->get_next_unit(unit,sizeof(unit));
            fwrite(unit,n,1,fp);
            fclose(fp);
        }
    }
}

void write_to_file(demux::parser* p,const char* name)
{
    char unit[512000];

    FILE* fp=fopen(name,"w");
    if(fp)
    {
        for(;;)
        {
            int n=p->get_next_unit(unit,sizeof(unit));

            if(n<1)
            {
                if(n<0)
                    printf("ERR: %i\n",n);
                break;
            }

            fwrite(unit,n,1,fp);

        }
        fclose(fp);
    }
}


int main(int argc,char** argv)
{
/*    char buf[]={0x00,0x00,0x00,0x00,0x00};

    for(int i=0;i<3;i++)
        for(int j=0;j<38;j++)
        {
            ((unsigned char*)buf)[4]=((i<<6)&0xc0)|(j&0x3f);
            ac3::get_ac3_frame_len(buf);
//            printf("%i\n",ac3::get_ac3_frame_len(buf));
        }
return 0;
*/



    if(argc<2)
        return 0;

    demux::file file;

//    demux::parser* p=new ac3::parser;
    demux::parser* p=new h264::parser;

    p->set_reader(&file);


    if(!file.open(argv[1]))
    {
//        parse_ac3(p);
        parse_h264(p);
//        split_to_files(p,10);
//        write_to_file(p,"dump.bin");

        file.close();
    }

    if(p)
        delete p;

    return 0;
}
