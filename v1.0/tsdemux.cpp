/*

Copyright (C) 2009 Anton Burdinuk

clark15b@gmail.com

*/

#ifdef _WIN32
#include <windows.h>
#endif

#include <sys/types.h>
#include <stdio.h>
#include <map>
#include <string>
#include <list>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <dirent.h>
#include <getopt.h>
#else
#include <io.h>
#include <fcntl.h>
#include "getopt.h"
#endif
#include "mpls.h"

#ifdef _WIN32
typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned long u_int32_t;
typedef unsigned long long u_int64_t;
#endif

// TODO: GUI

namespace tsmux
{
    enum
    {
#ifndef _WIN32
    max_path_len=512,
#else
    max_path_len=_MAX_PATH,
#endif
    max_fast_parse_frames=4,
    max_table_len=512
    };

    struct stream
    {
        u_int16_t programm;                     // programm number (1,2 ...)
        u_int8_t  id;                           // stream number in programm
        u_int8_t  type;                         // 0xff                 - not ES
                                                // 0x01,0x02            - MPEG2 video
                                                // 0x80                 - MPEG2 video (for TS only, not M2TS)
                                                // 0x1b                 - H.264 video
                                                // 0xea                 - VC-1  video
                                                // 0x81,0x06            - AC3   audio
                                                // 0x03,0x04            - MPEG2 audio
                                                // 0x80                 - LPCM  audio

        u_int8_t stream_id;                     // MPEG stream id
        int content_type;                       // 1 - audio, 2 - video
        u_int64_t dts;                          // current MPEG stream DTS (presentation time for audio, decode time for video)
        u_int64_t first_pts;
        u_int64_t last_pts;
        u_int32_t frame_rate;                   // current time for show frame in ticks (90 ticks = 1 ms, 90000/frame_rate=fps)
        u_int64_t frame_num;                    // frame counter

        FILE* fp;                               // ES output file
        FILE* tmc_fp;                           // timecodes output file
        u_int64_t start_timecode;               // start timecode value for tmc file (ms)

        std::string filename;
        std::string tmc_filename;

        u_int32_t nal_ctx;
        u_int64_t nal_frame_num;                // JVT NAL (h.264) frame counter

        stream(void):programm(0xffff),id(0),type(0xff),stream_id(0),content_type(0),
            dts(0),first_pts(0),last_pts(0),frame_rate(0),frame_num(0),fp(0),tmc_fp(0),start_timecode(0),
                nal_ctx(0),nal_frame_num(0) {}
    };

    struct channel_data
    {
        u_int64_t beg;
        u_int64_t end;
        u_int64_t maximum;

        channel_data(void):beg(0),end(0),maximum(0) {}
    };

    struct table
    {
        unsigned char* ptr;
        int len;
        int offset;

        table(void):ptr(new unsigned char[max_table_len]),len(0),offset(0) {}
        ~table(void)
        {
            if(ptr)
                delete[] ptr;
        }

        void reset(int l) { len=l; offset=0; }
    };

    const char* output_dir      = ".";
    bool hdmv_mode              = false;
    bool parse_only             = false;
    bool fast_parse             = false;
    bool interlace_mode         = false;
    int channel                 = 1;            // -1 for all
    FILE* chapters_fp           = 0;
    int verb                    = 0;

    table pmt;

    namespace stream_type
    {
        enum
        {
            unknown             = 0,
            audio               = 1,
            video               = 2
        };

        enum
        {
            data                = 0,
            mpeg2_video         = 1,
            h264_video          = 2,
            vc1_video           = 3,
            ac3_audio           = 4,
            mpeg2_audio         = 5,
            lpcm_audio          = 6
        };
    }

    inline u_int16_t ntohs(u_int16_t v) { return ((v<<8)&0xff00)|((v>>8)&0x00ff); }

#ifdef _WIN32
    inline int strcasecmp(const char* s1,const char* s2) { return lstrcmpi(s1,s2); }
#endif

    int scan_dir(const char* path,std::list<std::string>& l);
    int get_clip_number_by_filename(const std::string& s);

    u_int64_t decode_pts(const unsigned char* p);

    const char* get_stream_type_name(u_int8_t type_id,int* type);
    int get_stream_type(u_int8_t type_id);
    const char* get_file_ext_by_stream_type(u_int8_t type_id);

    const char* timecode_to_time(u_int64_t timecode);
    int calc_timecodes(channel_data& ch,stream& s);

    int demux_packet(const unsigned char* ptr,int len,std::map<u_int16_t,stream>& pids,bool& fast_parse_done);
    int demux_file(const char* name,std::map<u_int16_t,tsmux::stream>& pids,std::map<int,std::string>& datetime,int chapter);
    int demux_file(const char* name,FILE* fp,std::map<u_int16_t,tsmux::stream>& pids,std::map<int,std::string>& datetime,
        int chapter);
}

int tsmux::get_clip_number_by_filename(const std::string& s)
{
    int ll=s.length();
    const char* p=s.c_str();

    while(ll>0)
    {
        if(p[ll-1]=='/' || p[ll-1]=='\\')
            break;
        ll--;
    }

    p+=ll;

    int cn=0;

    const char* pp=strchr(p,'.');

    if(pp)
    {
        for(int i=0;i<pp-p;i++)
            cn=cn*10+(p[i]-48);
    }
    return cn;
}

#ifdef _WIN32
int tsmux::scan_dir(const char* path,std::list<std::string>& l)
{
    _finddata_t fileinfo;

    intptr_t dir=_findfirst((std::string(path)+"\\*.*").c_str(),&fileinfo);

    if(dir==-1)
        perror(path);
    else
    {
        while(!_findnext(dir,&fileinfo))
            if(!(fileinfo.attrib&_A_SUBDIR) && *fileinfo.name!='.')
            {
                char p[max_path_len];

                int n=sprintf(p,"%s\\%s",path,fileinfo.name);

                l.push_back(std::string(p,n));
            }
    }

    _findclose(dir);

    return l.size();
}
#else
int tsmux::scan_dir(const char* path,std::list<std::string>& l)
{
    DIR* dir=opendir(path);

    if(!dir)
        perror(path);
    else
    {
        dirent* d;

        while((d=readdir(dir)))
        {
            if(*d->d_name!='.')
            {
                char p[max_path_len];

                int n=sprintf(p,"%s/%s",path,d->d_name);

                l.push_back(std::string(p,n));
            }
        }

        closedir(dir);
    }

    return l.size();
}
#endif

const char* tsmux::get_stream_type_name(u_int8_t type_id,int* type)
{
    int t=get_stream_type(type_id);

    static const char* list[7]=
        { "Data","MPEG2 video","H.264 video","VC-1 video","AC3 audio","MPEG2 audio","LPCM audio" };

    static const int tlist[7]=
        { stream_type::unknown, stream_type::video, stream_type::video, stream_type::video,
            stream_type::audio,stream_type::audio,stream_type::audio };

    if(type)
        *type=tlist[t];

    return list[t];
}

int tsmux::get_stream_type(u_int8_t type_id)
{
    switch(type_id)
    {
    case 0x01:
    case 0x02:
        return stream_type::mpeg2_video;
    case 0x80:
        return hdmv_mode?stream_type::lpcm_audio:stream_type::mpeg2_video;
    case 0x1b:
        return stream_type::h264_video;
    case 0xea:
        return stream_type::vc1_video;
    case 0x81:
    case 0x06:
        return stream_type::ac3_audio;
    case 0x03:
    case 0x04:
        return stream_type::mpeg2_audio;
    }

    return stream_type::data;
}

const char* tsmux::get_file_ext_by_stream_type(u_int8_t type_id)
{
    static const char* list[7]= { "bin", "m2v", "264", "vc1", "ac3", "m2a", "pcm" };

    return list[type_id];
}


u_int64_t tsmux::decode_pts(const unsigned char* p)
{
    u_int64_t pts=((p[0]&0xe)<<29);
    pts|=((p[1]&0xff)<<22);
    pts|=((p[2]&0xfe)<<14);
    pts|=((p[3]&0xff)<<7);
    pts|=((p[4]&0xfe)>>1);

    return pts;
}

int tsmux::demux_packet(const unsigned char* ptr,int len,std::map<u_int16_t,tsmux::stream>& pids,bool& fast_parse_done)
{
    if(len==192)                                // skip timecode from M2TS stream
    {
        ptr+=4;
        len-=4;
    }else if(len!=188)                          // invalid packet length
        return -1;

    if(ptr[0]!=0x47)                            // invalid packet sync byte
        return -2;

    u_int16_t pid=ntohs(*((u_int16_t*)(ptr+1)));

    if(pid&0x8000)                              // transport error
        return -3;

    bool payload_unit_start_indicator=pid&0x4000;

    u_int8_t flags=ptr[3];

    if(flags&0xc0)                              // scrambled
        return -4;

    bool adaptation_field_exist=flags&0x20;

    bool payload_data_exist=flags&0x10;

    pid&=0x1fff;

    if(pid!=0x1fff && payload_data_exist)
    {
        ptr+=4;
        len-=4;

        if(adaptation_field_exist)
        {
            int l=(*ptr)+1;

            if(l>len)
                return -5;

            ptr+=l;                             // skip adaptation field
            len-=l;
        }


        if(!pid)
        {
            // PAT table
            if(payload_unit_start_indicator)
            {
                if(len<1)
                    return -6;

                ptr+=1;                         // skip pointer field
                len-=1;
            }

            if(*ptr!=0x00)                      // is not PAT
                return 0;

            if(len<8)
                return -7;


            u_int16_t l=ntohs(*((u_int16_t*)(ptr+1)));

            if(l&0xb000!=0xb000)
                return -8;

            l&=0x0fff;

            len-=3;

            if(l>len)
                return -9;

            len-=5;
            ptr+=8;
            l-=5+4;

            if(l%4)
                return -10;

            int n=l/4;

            for(int i=0;i<n;i++)
            {
                u_int16_t programm=ntohs(*((u_int16_t*)ptr));

                u_int16_t pid=ntohs(*((u_int16_t*)(ptr+2)));

                if(pid&0xe000!=0xe000)
                    return -11;

                pid&=0x1fff;

                ptr+=4;

                stream& s=pids[pid];

                s.programm=programm;
                s.type=0xff;
            }
        }else
        {
            stream& s=pids[pid];

            if(s.programm!=0xffff)
            {
                if(s.type==0xff)
                {
                    // PMT table
                    if(payload_unit_start_indicator)
                    {
                        if(len<1)
                            return -12;

                        ptr+=1;                 // skip pointer field
                        len-=1;

                        if(*ptr!=0x02)          // is not PMT
                            return 0;

                        if(len<12)
                            return -13;

                        u_int16_t l=ntohs(*((u_int16_t*)(ptr+1)));

                        if(l&0x3000!=0x3000)
                            return -14;

                        l=(l&0x0fff)+3;

                        if(l>max_table_len)
                            return -141;

                        pmt.reset(l);

                        u_int16_t ll=len>l?l:len;

                        memcpy(pmt.ptr,ptr,ll);
                        pmt.offset+=ll;

                        if(pmt.offset<pmt.len)
                            return 0;           // wait next part
                    }else
                    {
                        if(!pmt.offset)
                            return -142;

                        u_int16_t l=pmt.len-pmt.offset;

                        u_int16_t ll=len>l?l:len;

                        memcpy(pmt.ptr+pmt.offset,ptr,ll);
                        pmt.offset+=ll;

                        if(pmt.offset<pmt.len)
                            return 0;           // wait next part
                    }

                    ptr=pmt.ptr;
                    u_int16_t l=pmt.len;

                    u_int16_t n=(ntohs(*((u_int16_t*)(ptr+10)))&0x0fff)+12;

                    if(n>l)
                        return -15;

                    ptr+=n;
                    len-=n;

                    l-=n+4;

                    while(l)
                    {
                        if(l<5)
                            return -16;

                        u_int8_t type=*ptr;
                        u_int16_t pid=ntohs(*((u_int16_t*)(ptr+1)));

                        if(pid&0xe000!=0xe000)
                            return -17;

                        pid&=0x1fff;

                        u_int16_t ll=(ntohs(*((u_int16_t*)(ptr+3)))&0x0fff)+5;

                        if(ll>l)
                            return -18;

                        ptr+=ll;
                        l-=ll;

                        stream& ss=pids[pid];

                        if(ss.programm!=s.programm || ss.type!=type)
                        {
                            ss.programm=s.programm;
                            ss.type=type;
                            ss.id=++s.id;
                            get_stream_type_name(type,&ss.content_type);
                        }
                    }

                    if(l>0)
                        return -19;

                }else
                {
                    // PES (Packetized Elementary Stream)
                    if(payload_unit_start_indicator)
                    {
                        // PES header

                        if(len<6)
                            return -20;

                        static const unsigned char start_code_prefix[]={0x00,0x00,0x01};

                        if(memcmp(ptr,start_code_prefix,sizeof(start_code_prefix)))
                            return -21;

                        u_int8_t stream_id=ptr[3];
                        u_int16_t l=ntohs(*((u_int16_t*)(ptr+4)));

                        ptr+=6;
                        len-=6;

                        if((stream_id>=0xbd && stream_id<=0xbf) || (stream_id>=0xc0 && stream_id<=0xdf) || (stream_id>=0xe0 && stream_id<=0xef)  || (stream_id>=0xfa && stream_id<=0xfe))
                        {
                            // PES header extension

                            if(len<3)
                                return -22;

                            u_int8_t bitmap=ptr[1];
                            u_int8_t hlen=ptr[2]+3;

                            if(len<hlen)
                                return -23;

                            if(l>0)
                                l-=hlen;

                            switch(bitmap&0xc0)
                            {
                            case 0x80:          // PTS only
                                if(hlen>=8)
                                {
                                    u_int64_t pts=decode_pts(ptr+3);

                                    if(s.dts>0 && pts>s.dts)
                                        s.frame_rate=pts-s.dts;

                                    s.dts=pts;

                                    if(pts>s.last_pts)
                                        s.last_pts=pts;

                                    if(!s.first_pts && s.frame_num==(s.content_type==stream_type::video?1:0))
                                        s.first_pts=pts;
                                }
                                break;
                            case 0xc0:          // PTS,DTS
                                if(hlen>=13)
                                {
                                    u_int64_t pts=decode_pts(ptr+3);
                                    u_int64_t dts=decode_pts(ptr+8);

                                    if(s.dts>0 && dts>s.dts)
                                        s.frame_rate=dts-s.dts;

                                    s.dts=dts;

                                    if(pts>s.last_pts)
                                        s.last_pts=pts;

                                    if(!s.first_pts && s.frame_num==(s.content_type==stream_type::video?1:0))
                                        s.first_pts=dts;
                                }
                                break;
                            }

                            ptr+=hlen;
                            len-=hlen;

                            s.stream_id=stream_id;

                            s.frame_num++;

                            if(fast_parse && s.content_type==stream_type::video && s.frame_num>max_fast_parse_frames)
                                fast_parse_done=true;

                        }else
                            s.stream_id=0;
                    }

                    if(!parse_only && s.stream_id)
                    {
                        if(channel==-1 || s.programm==channel)
                        {
                            if(!s.fp)
                            {
                                char tmp[max_path_len];

                                int n=sprintf(tmp,
#ifndef _WIN32
                                    "%s/channel_%.2x_track_%.2x_stream_%.2x.%s",
#else
                                    "%s\\channel_%.2x_track_%.2x_stream_%.2x.%s",
#endif
                                output_dir,s.programm,s.id,s.stream_id,get_file_ext_by_stream_type(get_stream_type(s.type)));

                                s.fp=fopen(tmp,"wb");

                                if(!s.fp)
                                    perror(tmp);
                                else
                                    s.filename.assign(tmp,n);

                                char* p=strrchr(tmp,'.');
                                if(p)
                                {
                                    strcpy(p+1,"tmc");

                                    s.tmc_fp=fopen(tmp,"w");

                                    if(!s.tmc_fp)
                                        perror(tmp);
                                    else
                                    {
                                        s.tmc_filename.assign(tmp,n);

                                        fprintf(s.tmc_fp,"# timecode format v2\n");
                                    }
                                }
                            }

                            if(s.fp)
                            {
                                if(s.type==0x1b)                                // JVT NAL (h.264)
                                {
                                    for(int i=0;i<len;i++)
                                    {
                                        s.nal_ctx=(s.nal_ctx<<8)+ptr[i];

                                        if((s.nal_ctx&0xffffff1f)==0x00000109)  // NAL access unit
                                            s.nal_frame_num++;
                                    }
                                }

                                if(!fwrite(ptr,len,1,s.fp))
                                    fprintf(stderr,"** elementary stream 0x%x (%i/%i) write error\n",s.stream_id,s.programm,s.id);
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}



int main(int argc,char** argv)
{
    fprintf(stderr,"tsdemux 1.02 AVCHD/Blu-Ray HDMV Transport Stream demultiplexer\n\nCopyright (C) 2009 Anton Burdinuk\n\nclark15b@gmail.com\nhttp://code.google.com/p/tsdemuxer\n\n");

    if(argc<2)
    {
        fprintf(stderr,"USAGE: ./tsdemux [-o dst_dir] [-d src_dir] [-l playlist_dir] [-c channel] [-h] [-i] [-p] [-m] [-v] [srcfile1.(ts|m2ts) ... srcfileN.(ts|m2ts)]\n");
        fprintf(stderr,"-o redirect output to another directory (default: './')\n");
        fprintf(stderr,"-l use AVCHD/Blu-Ray playlist file (*.mpl,*.mpls)\n");
        fprintf(stderr,"-d demux all (if not used '-l' switch) mts/m2ts/ts files from directory\n");
        fprintf(stderr,"-c channel number for demux (default: 1, 'any' for all channels)\n");
        fprintf(stderr,"-v turn on verbose output\n");
        fprintf(stderr,"-h force HDMV mode for STDIN input (192 byte packets)\n");
        fprintf(stderr,"-i force interlace mode for video stream (fps*2)\n");
        fprintf(stderr,"-p parse only, do not demux to video and audio files\n");
        fprintf(stderr,"-f fast parse\n");
        fprintf(stderr,"-m show mkvmerge command example\n");
        fprintf(stderr,"\ninput files can be *.m2ts, *.mts, *.ts or '-' for STDIN input\n");
        fprintf(stderr,"output elementary streams to *.bin, *.m2v, *.264, *.vc1, *.ac3, *.m2a and *.pcm files\n");
        fprintf(stderr,"*.tmc and chapters.xml for mkvtoolnix\n");
        fprintf(stderr,"\n");
        return 0;
    }

#ifdef _WIN32
    _setmode(0,_O_BINARY);
#endif

    using namespace tsmux;

    bool mkvmerge_opts=false;

    int opt;

    std::list<std::string>              playlist;               // playlist
    std::list<std::string>              cliplist;               // raw M2TS/TS clip files list
    std::map<int,std::string>           datetime;               // AVCHD clip date/time
    std::list<int>                      mpls;                   // list of clip id from mpls files

    std::map<u_int16_t,tsmux::stream>   pids;

    while((opt=getopt(argc,argv,"o:hipc:d:fml:v"))>=0)
        switch(opt)
        {
        case 'o':
            output_dir=optarg;
            break;
        case 'l':
            if(mpls_parse(optarg,mpls,datetime,verb))
                fprintf(stderr,"** %s: invalid playlist file format\n",optarg);
            break;
        case 'h':
            hdmv_mode=true;
            break;
        case 'i':
            interlace_mode=true;
            break;
        case 'p':
            parse_only=true;
            break;
        case 'm':
            mkvmerge_opts=true;
            break;
        case 'f':
            parse_only=true;
            fast_parse=true;
            break;
        case 'c':
            if(!strcasecmp(optarg,"any"))
                channel=-1;
            else
                channel=atoi(optarg);
            break;
        case 'd':
            {
                std::list<std::string> l;

                tsmux::scan_dir(optarg,l);

                l.sort();

                cliplist.merge(l);
            }
            break;
        case 'v':
            verb=1;
            break;
        }


    while(optind<argc)
    {
        cliplist.push_back(argv[optind]);
        optind++;
    }

    if(mpls.size())
    {
        std::map<int,std::string> clips;

        for(std::list<std::string>::iterator i=cliplist.begin();i!=cliplist.end();++i)
        {
            std::string& s=*i;

            clips[get_clip_number_by_filename(s)]=s;
        }

        for(std::list<int>::iterator i=mpls.begin();i!=mpls.end();++i)
        {
            std::string& s=clips[*i];
            if(s.length())
                playlist.push_back(s);
        }
    }else
        playlist=cliplist;


    int stdin_number=0;


    int chapter=0;

    std::string chap_file;

    if(!parse_only && channel!=-1)
    {
        char path[tsmux::max_path_len];

        int n=sprintf(path,
#ifndef _WIN32
            "%s/chapters.xml",
#else
            "%s\\chapters.xml",
#endif
            output_dir);

        chapters_fp=fopen(path,"w");

        if(chapters_fp)
        {
            chap_file.assign(path,n);

            fprintf(chapters_fp,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");
            fprintf(chapters_fp,"<!-- <!DOCTYPE Tags SYSTEM \"matroskatags.dtd\"> -->\n\n");
            fprintf(chapters_fp,"<Chapters>\n");
            fprintf(chapters_fp,"  <EditionEntry>\n");
        }
    }


    for(std::list<std::string>::iterator i=playlist.begin();i!=playlist.end();++i)
    {
        std::string& s=*i;

        if(s=="-")
        {
            if(!stdin_number)
            {
                demux_file("stdin",stdin,pids,datetime,++chapter);

                stdin_number++;
            }
        }else
            tsmux::demux_file(s.c_str(),pids,datetime,++chapter);
    }


    if(!fast_parse)
        fprintf(stderr,"\n---------------------------------------------------------------\n");

    for(std::map<u_int16_t,tsmux::stream>::iterator i=pids.begin();i!=pids.end();++i)
    {
        u_int16_t pid=i->first;
        tsmux::stream& s=i->second;

        if(s.stream_id)
        {
            int stream_type=0;

            const char* type_name=tsmux::get_stream_type_name(s.type,&stream_type);

            switch(stream_type)
            {
            case tsmux::stream_type::video:
                fprintf(stderr,"pid 0x%x, channel %i, track %i, stream 0x%x, type 0x%x (%s, fps=%.2f)\n",pid,s.programm,s.id,s.stream_id,s.type,type_name,90000./s.frame_rate);
                break;
            default:
                fprintf(stderr,"pid 0x%x, channel %i, track %i, stream 0x%x, type 0x%x (%s)\n",pid,s.programm,s.id,s.stream_id,s.type,type_name);
                break;
            }

            if(s.fp)
                fclose(s.fp);
            if(s.tmc_fp)
                fclose(s.tmc_fp);
        }
    }

    if(chapters_fp)
    {
        fprintf(chapters_fp,"  </EditionEntry>\n");
        fprintf(chapters_fp,"</Chapters>\n");
        fclose(chapters_fp);

        if(!chapter)
            remove(chap_file.c_str());
    }


    if(!parse_only && mkvmerge_opts)
    {
        fprintf(stdout,"\n# mkvmerge -o output.mkv ");

        if(chap_file.length())
            fprintf(stdout,"--chapters %s ",chap_file.c_str());

        for(std::map<u_int16_t,tsmux::stream>::iterator i=pids.begin();i!=pids.end();++i)
        {
            tsmux::stream& s=i->second;

            if(s.filename.length())
            {
                if(s.tmc_filename.length())
                    fprintf(stdout,"--timecodes 0:%s ",s.tmc_filename.c_str());
                fprintf(stdout,"%s ",s.filename.c_str());
            }
        }

        fprintf(stdout,"\n");
    }

    return 0;
}

int tsmux::demux_file(const char* name,std::map<u_int16_t,tsmux::stream>& pids,std::map<int,std::string>& datetime,
    int chapter)
{
    bool hdmv_mode_tmp=hdmv_mode;

    const char* p=strrchr(name,'.');

    if(p)
    {
        p++;

        if(!strcasecmp(p,"mts") || !strcasecmp(p,"m2ts"))
            hdmv_mode=true;
        else if(!strcasecmp(p,"ts"))
            hdmv_mode=false;
        else
            return -1;
    }else
        return -1;

    FILE* fp=fopen(name,"rb");

    if(!fp)
        perror(name);
    else
    {
        demux_file(name,fp,pids,datetime,chapter);

        fclose(fp);
    }

    hdmv_mode=hdmv_mode_tmp;

    return 0;
}

int tsmux::demux_file(const char* name,FILE* fp,std::map<u_int16_t,tsmux::stream>& pids,std::map<int,std::string>& datetime,
    int chapter)
{
    size_t l=hdmv_mode?192:188;

    bool fast_parse_done=false;

    char tmp[192];

    for(u_int32_t packet=0;!fast_parse_done;packet++)
    {
        size_t length=fread(tmp,1,l,fp);

        if(!length)
            break;

        if(length!=l)
        {
            fprintf(stderr,"** %s: incompleted TS packet\n",name);
            break;
        }

        int n;
        if((n=tsmux::demux_packet((unsigned char*)tmp,l,pids,fast_parse_done)))
        {
            fprintf(stderr,"** %s: invalid packet (%i)\n",name,n);
            break;
        }
    }


    // find first, last, max pts... any fixups
    std::map<u_int16_t,channel_data> chan;

    if(!fast_parse)
    {
        for(std::map<u_int16_t,tsmux::stream>::iterator i=pids.begin();i!=pids.end();++i)
        {
            tsmux::stream& s=i->second;

            channel_data& ch=chan[s.programm];

            if(s.stream_id && s.first_pts && s.last_pts)
            {
                if(!ch.beg || ch.beg>s.first_pts)
                    ch.beg=s.first_pts;

                u_int64_t end=s.last_pts+s.frame_rate;

                // fix last_pts if variable fps in chapter or it is join of many files (pts/dts start many times)
                u_int64_t t=s.first_pts+s.frame_num*s.frame_rate;
                if(t>end)
                {
                    if(verb)
                        fprintf(stderr,"** bad last PTS in stream 0x%x (%i/%i), use first PTS and current frame rate for fix it\n",s.stream_id,s.programm,s.id);
                    end=t;
                    s.last_pts=t-s.frame_rate;
                }

                if(!ch.end || ch.end>end)
                    ch.end=end;

                if(!ch.maximum || ch.maximum<end)
                    ch.maximum=end;
            }
        }
    }


    // show info
    for(std::map<u_int16_t,tsmux::stream>::iterator i=pids.begin();i!=pids.end();++i)
    {
        tsmux::stream& s=i->second;

        if(!fast_parse)
        {
            channel_data& ch=chan[s.programm];

            if(s.stream_id && s.first_pts && s.last_pts && s.frame_rate)
            {
                u_int64_t last_pts=s.last_pts+s.frame_rate;

                double delay=((double)(s.first_pts-ch.beg))/90.;
                double len=((double)(last_pts-s.first_pts))/90.;

                fprintf(stderr,"%s: channel %i, track %i, stream 0x%.2x, type \"%s\", length %.2fms",name,s.programm,s.id,s.stream_id,get_file_ext_by_stream_type(get_stream_type(s.type)),len);

                if(last_pts>ch.end)
                    fprintf(stderr," (+%.2fms)",(double)(last_pts-ch.end)/90.);

                if(delay)
                    fprintf(stderr,", delay %.2fms",delay);

                fprintf(stderr,"\n");

                fflush(stderr);

                // add chapter
                if(chapters_fp && s.programm==channel && s.id==1)
                {
                    int cn=get_clip_number_by_filename(std::string(name));

                    std::string& c=datetime[cn];


                    fprintf(chapters_fp,"    <ChapterAtom>\n");
                    fprintf(chapters_fp,"      <ChapterTimeStart>%s</ChapterTimeStart>\n",timecode_to_time(s.start_timecode));
                    fprintf(chapters_fp,"      <ChapterDisplay>\n");
                    if(c.length())
                        fprintf(chapters_fp,"        <ChapterString>%s</ChapterString>\n",c.c_str());
                    else
                        fprintf(chapters_fp,"        <ChapterString>Clip %i</ChapterString>\n",chapter);
                    fprintf(chapters_fp,"      </ChapterDisplay>\n");
                    fprintf(chapters_fp,"    </ChapterAtom>\n");
                }

                // write timecodes
                if(s.tmc_fp)
                    calc_timecodes(ch,s);
            }
        }

        s.first_pts=0;
        s.last_pts=0;
        s.frame_num=0;
        s.nal_ctx=0;
        s.nal_frame_num=0;
        s.dts=0;
    }


    return 0;
}

const char* tsmux::timecode_to_time(u_int64_t timecode)
{
    static char buf[128];

    int msec=timecode%1000;
    timecode/=1000;

    int sec=timecode%60;
    timecode/=60;

    int min=timecode%60;
    timecode/=60;

    sprintf(buf,"%.2i:%.2i:%.2i.%.3i",(int)timecode,min,sec,msec);

    return buf;
}


int tsmux::calc_timecodes(tsmux::channel_data& ch,tsmux::stream& s)
{
    FILE* fp=s.tmc_fp;

    // calc total chapter length
    u_int64_t total_len=ch.maximum-ch.beg;

    // align chapter length for fit to ms
    if(total_len%90)
    {
        total_len=(total_len/90)*90+90;

        if(verb)
            fprintf(stderr,"** align stream 0x%x (%i/%i) length to %.2fms\n",s.stream_id,s.programm,s.id,(double)total_len/90.);
    }

    u_int64_t pos=s.first_pts-ch.beg;
    u_int64_t pos_ms=pos/90;

    u_int64_t frame_num=s.frame_num;
    u_int32_t frame_rate_top=s.frame_rate;
    u_int32_t frame_rate_bottom=s.frame_rate;


    bool interlace=interlace_mode;

    if(s.type==0x1b)
    {
        if(s.nal_frame_num/2==s.frame_num)
            interlace=true;
    }

    if(s.content_type==stream_type::video && interlace)
    {
        frame_num*=2;
        frame_rate_top=s.frame_rate/2;
        frame_rate_bottom=s.frame_rate-frame_rate_top;
    }

    for(u_int64_t i=0;i<frame_num;i++)
    {
        fprintf(s.tmc_fp,"%lli\n",pos_ms+s.start_timecode);

        pos+=(i+1)%2?frame_rate_top:frame_rate_bottom;

        pos_ms=pos/90;
    }

    s.start_timecode+=total_len/90.;

    return 0;
}

