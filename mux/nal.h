#ifndef __NAL_H
#define __NAL_H

#include "reader.h"

namespace h264
{
    enum
    {
        fps_23_976,
        fps_24,
        fps_25,
        fps_29_97,
        fps_30
    };


    class timecode_gen
    {
    protected:
        unsigned short k,fps;
        unsigned long fn;
    public:
        timecode_gen(void):k(0),fps(0),fn(0) {}

        void set_fps(int _fps)
        {
            switch(_fps)
            {
            case fps_23_976:    k=1001; fps=24; break;
            case fps_24:        k=1000; fps=24; break;
            case fps_25:        k=1000; fps=25; break;
            case fps_29_97:     k=1001; fps=30; break;
            case fps_30:        k=1000; fps=30; break;
            }
        }

        unsigned long get_next_timecode(void)
        {
            unsigned long timecode=((double)fn)*k/fps;
            fn++;
            return timecode;
        }
    };


    class parser : public demux::parser
    {
    protected:
        unsigned int ctx;
        unsigned short st;

        demux::stream* reader;
    public:
        parser(demux::stream* _r=0):ctx(0),reader(_r),st(0) {}

        void set_reader(demux::stream* _r) { reader=_r; }

        void reset(void) { ctx=0; st=0; }

        int get_next_unit(char* dst,int len);
    };

    class bitstring
    {
    protected:
        unsigned const char* ptr;
        int len;
        unsigned char cur;
        unsigned short nbits;
    public:
        bitstring(const char* p,int l):ptr((const unsigned char*)p),len(l-1),cur(0),nbits(0) { cur=*ptr; }

        int get_next_bit(void)
        {
            if(nbits>7)
            {
                if(len<1)
                    return 1;
                ptr++;
                len--;
                nbits=0;
                cur=*ptr;
            }

            int rc=cur&0x80;

            cur<<=1;
            nbits++;

            return !!rc;
        }

        int get_next_number(void)
        {
            int len=0,rc=1;

            for(len=0;!get_next_bit();len++);

            for(;len>0;len--)
                rc=((rc<<1)&0xfffffffe)|get_next_bit();

            return rc-1;
        }
    };

    inline int get_nal_unit_type(const char* p) { return p[3]&0x1f; }
    const char* get_nal_unit_type_name(const char* p);

    inline int get_nal_access_unit_type(const char* p) { return (p[4]>>5)&0x07; }
    const char* get_nal_access_unit_type_name(const char* p);

    inline int get_nal_seq_p_h264_profile(const char* p) { return p[4]; }
    const char* get_nal_seq_p_h264_profile_name(const char* p);
    inline int get_nal_seq_p_h264_level(const char* p) { return p[6]; }
    inline void set_nal_seq_p_h264_level(char* p,int level) { ((unsigned char*)p)[6]=level; }

    inline int nal_unit_is_slice(const char* p)
    {
        register int n=get_nal_unit_type(p);

        if(n>0 && n<6)
            return 1;

        return 0;
    }

    inline int get_nal_unit_slice_type(const char* p)
    {
        bitstring s(p+4,8);
        s.get_next_number();    // skip first_mb_in_slice
        return s.get_next_number();
    }

    const char* get_nal_unit_slice_type_name(const char* p);

}

#endif
