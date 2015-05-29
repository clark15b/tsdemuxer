#ifndef __READER_H
#define __READER_H

#include <stdio.h>

namespace demux
{
    struct stream
    {
        virtual ~stream() {}

        virtual int getch(void)=0;
    };

    struct parser
    {
        virtual ~parser() {}

        virtual void set_reader(stream*)=0;
        virtual void reset(void)=0;
        virtual int get_next_unit(char*,int)=0;
    };

    class file : public stream
    {
    protected:
        int fd;                         // source file
        int opened;

        enum { max_len=2048 };
        unsigned int offset;
        unsigned int len;
        char buf[max_len];

        void reset(void) { offset=len=0; }

        int read(char* dst,int len);
    public:
        enum { eof=0xffffffff };

        file(void):fd(-1),opened(0),offset(0),len(0) {}
        ~file(void) { close(); }

        int fdopen(int _fd);
        int open(const char* name);
        void close(void);

        int getch(void);
    };


}


#endif
