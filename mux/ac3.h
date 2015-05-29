#ifndef __AC3_H
#define __AC3_H

#include "reader.h"

namespace ac3
{
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

    inline int get_ac3_sample_rate(const char* p) { return (((unsigned char*)p)[4]>>6)&0x03; }
    const char* get_ac3_sample_rate_name(const char* p);

    extern const short bit_rate[];

    inline int get_ac3_bit_rate(const char* p) { return bit_rate[((unsigned char*)p)[4]&0x3f]; }

    int get_ac3_frame_len(const char* p);
}

#endif
