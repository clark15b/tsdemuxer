#include "mpls.h"

namespace mpls
{
    inline u_int32_t get_ui32(unsigned char* p)
    {
        u_int32_t rc=((u_int32_t)p[0])<<24;
        rc+=((u_int32_t)p[1])<<16;
        rc+=((u_int32_t)p[2])<<8;
        rc+=p[3];

        return rc;
    }

    inline u_int16_t get_ui16(unsigned char* p)
    {
        u_int32_t rc=((u_int32_t)p[0])<<8;
        rc+=p[1];

        return rc;
    }
}



int mpls::parse(const char* filename,std::list<int>& playlist,std::map<int,std::string>& datetime,int verb)
{
    FILE* fp=fopen(filename,"rb");

    if(!fp)
        return -1;

    struct stat st;

    if(fstat(fileno(fp),&st)==-1)
    {
        fclose(fp);
        return -1;
    }

    unsigned int len=st.st_size;

    std::auto_ptr<unsigned char> p(new unsigned char[len]);

    unsigned char* ptr=p.get();

    if(!ptr)
    {
        fclose(fp);
        return -1;
    }

    if(fread(ptr,1,len,fp)!=len)
    {
        fclose(fp);
        return -1;
    }

    fclose(fp);

    if(memcmp(ptr,"MPLS0",5))
        return -1;

    std::vector<int> chapters;
    chapters.reserve(512);

    u_int32_t playlist_offset=get_ui32(ptr+8);
    u_int32_t playlist_mark_offset=get_ui32(ptr+12);
    u_int32_t playlist_ext_offset=get_ui32(ptr+16);

    if(playlist_offset>len || playlist_mark_offset>len || playlist_ext_offset>len)
        return -1;

    if(verb)
        fprintf(stderr,"=== clips from playlist %s ===\n",filename);

    if(playlist_offset)
    {
        unsigned char* p=ptr+playlist_offset;

        u_int32_t l=get_ui32(p);
        p+=4;

        unsigned char* p2=p+l;

        p+=2;                                   // reserved

        u_int16_t n=get_ui16(p);
        p+=4;

        for(u_int16_t i=0;i<n;i++)
        {
            if(p>p2)
                return -1;

            u_int16_t item_len=get_ui16(p);
            p+=2;

            int clip=0;

            for(int j=0;j<5;j++)
            clip=clip*10+(p[j]-48);

            chapters.push_back(clip);
            playlist.push_back(clip);

            p+=item_len;
        }
    }

    if(playlist_mark_offset)
    {
        // skip section
    }

    if(playlist_ext_offset)
    {
        unsigned char* p=ptr+playlist_ext_offset;

        u_int32_t l=get_ui32(p);
        p+=4;

        unsigned char* p2=p+l;

        if(p+4<=p2 && !memcmp(p+20,"PLEX",4))
        {
            p+=348;

            if(p+2<=p2)
            {
                u_int16_t n=get_ui16(p);
                p+=2;

                for(u_int16_t i=0;i<n;i++)
                {
                    if(p+66>p2)
                        break;

                    // CA or DA
                    if((p[44] == 'C' || p[44] == 'D') && p[45] == 'A')
                    {
                        char tmp[64];

                        sprintf(tmp,"20%.2x-%.2x-%.2x %.2x:%.2x:%.2x",p[12],p[13],p[14],p[15],p[16],p[17]);

                        datetime[chapters[i]]=tmp;

                        if(verb)
                            fprintf(stderr,"* %.5i: %s\n",chapters[i],tmp);
                    }

                    p+=66;
                }
            }
        }

    }else
    {
        if(verb)
            for(int i=0;i<chapters.size();i++)
                fprintf(stderr,"* %.5i\n",chapters[i]);
    }

    return 0;
}
