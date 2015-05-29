#include "reader.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int demux::file::fdopen(int _fd)
{
    close();

    fd=fd;

    opened=0;
}

int demux::file::open(const char* name)
{
    close();

    fd=::open(name,O_RDONLY|O_LARGEFILE);

    if(fd==-1)
        return -1;

    opened=1;

    return 0;
}

int demux::file::read(char* dst,int len)
{
    return ::read(fd,dst,len);
}

void demux::file::close(void)
{
    reset();

    if(fd!=-1)
    {
        if(opened)
        {
            ::close(fd);
            opened=0;
        }
        fd=-1;
    }
}


int demux::file::getch(void)
{
    if(offset>=len)
    {
        int n=read(buf,max_len);

        if(n<1)
            return eof;

        len=n;

        offset=0;
    }

    int ch=((unsigned char*)buf)[offset];

    offset++;

    return ch;
}

