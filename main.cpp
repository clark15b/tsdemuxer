/*

 Copyright (C) 2009 Anton Burdinuk

 clark15b@gmail.com

*/


#include "ts.h"
#include "mpls.h"

namespace ts
{
    class ts_file_info
    {
    public:
        std::string filename;
        u_int64_t first_dts;
        u_int64_t first_pts;
        u_int64_t last_pts;

        ts_file_info(void):first_dts(0),first_pts(0),last_pts(0) {}
    };

#ifdef _WIN32
    inline int strcasecmp(const char* s1,const char* s2) { return lstrcmpi(s1,s2); }
#endif

    int scan_dir(const char* path,std::list<std::string>& l);
    void load_playlist(const char* path,std::list<std::string>& l,std::map<int,std::string>& d);
    int get_clip_number_by_filename(const std::string& s);

    bool is_ts_filename(const std::string& s)
    {
        if(!s.length())
            return false;

        if(s[s.length()-1]=='/' || s[s.length()-1]=='\\')
            return false;

        std::string::size_type n=s.find_last_of('.');

        if(n==std::string::npos)
            return false;

        std::string ext=s.substr(n+1);

        if(!strcasecmp(ext.c_str(),"ts") || !strcasecmp(ext.c_str(),"m2ts"))
            return true;

        return false;
    }

    std::string trim_slash(const std::string& s)
    {
        const char* p=s.c_str()+s.length();

        while(p>s.c_str() && (p[-1]=='/' || p[-1]=='\\'))
            p--;

        return s.substr(0,p-s.c_str());
    }

    const char* timecode_to_time(u_int32_t timecode)
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

}

#ifdef _WIN32
int ts::scan_dir(const char* path,std::list<std::string>& l)
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
                char p[512];

                int n=sprintf(p,"%s\\%s",path,fileinfo.name);

                l.push_back(std::string(p,n));
            }
    }

    _findclose(dir);

    return l.size();
}
#else
int ts::scan_dir(const char* path,std::list<std::string>& l)
{
    DIR* dir=opendir(path);

    if(!dir)
        perror(path);
    else
    {
        dirent* d;

        while((d=readdir(dir)))
        {
            if (d->d_type == DT_UNKNOWN) {
                char p[512];

                if (snprintf(p, sizeof(p), "%s/%s", path, d->d_name) > 0) {
                    struct stat st;

                    if (stat(p, &st) != -1) {
                        if (S_ISREG(st.st_mode))
                            d->d_type = DT_REG;
                        else if (S_ISLNK(st.st_mode))
                            d->d_type = DT_LNK;
                    }
                }
            }
            if(*d->d_name!='.' && (d->d_type==DT_REG || d->d_type==DT_LNK))
            {
                char p[512];

                int n=sprintf(p,"%s/%s",path,d->d_name);

                l.push_back(std::string(p,n));
            }
        }

        closedir(dir);
    }

    return l.size();
}
#endif

void ts::load_playlist(const char* path,std::list<std::string>& l,std::map<int,std::string>& d)
{
    FILE* fp=fopen(path,"r");

    char buf[512];

    while(fgets(buf,sizeof(buf),fp))
    {
        char* p=buf;
        while(*p && (*p==' ' || *p=='\t'))
            p++;

        int len=0;

        char* p2=strpbrk(p,"#\r\n");
        if(p2)
            len=p2-p;
        else
            len=strlen(p);

        while(len>0 && (p[len-1]==' ' || p[len-1]=='\t'))
            len--;

        if(len>0)
        {
            l.push_back(std::string());

            std::string& s=l.back();

            std::string ss(p,len);
            std::string::size_type n=ss.find_first_of(';');
            if(n==std::string::npos)
                s.swap(ss);
            else
            {
                s=ss.substr(0,n);
                d[get_clip_number_by_filename(s)]=ss.substr(n+1);
            }
        }
    }
}


int ts::get_clip_number_by_filename(const std::string& s)
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


int main(int argc,char** argv)
{
    fprintf(stderr,"tsdemux 1.53 AVCHD/Blu-Ray HDMV Transport Stream demultiplexer\n\nCopyright (C) 2009 Anton Burdinuk\n\nclark15b@gmail.com\nhttp://code.google.com/p/tsdemuxer\n\n");

    if(argc<2)
    {
        fprintf(stderr,"USAGE: ./tsdemux [-d src] [-l mpls] [-s pls] [-o dst] [-c channel] [-u] [-j] [-m] [-z] [-p] [-e mode] [-v] *.ts|*.m2ts ...\n");
        fprintf(stderr,"-d demux all mts/m2ts/ts files from directory\n");
        fprintf(stderr,"-l use AVCHD/Blu-Ray playlist file (*.mpl,*.mpls)\n");
        fprintf(stderr,"-s playlist text file\n");
        fprintf(stderr,"-o redirect output to another directory or transport stream file\n");
        fprintf(stderr,"-c channel number for demux\n");
        fprintf(stderr,"-u demux unknown streams\n");
        fprintf(stderr,"-j join elementary streams\n");
        fprintf(stderr,"-m show mkvmerge command example\n");
        fprintf(stderr,"-z demux to PES streams (instead of elementary streams)\n");
        fprintf(stderr,"-p parse only\n");
        fprintf(stderr,"-e dump TS structure to STDOUT (mode=1: dump M2TS timecodes, mode=2: dump PTS/DTS, mode=3: human readable PTS/DTS dump)\n");
        fprintf(stderr,"-v turn on verbose output\n");
        fprintf(stderr,"\ninput files can be *.m2ts, *.mts or *.ts\n");
        fprintf(stderr,"output elementary streams to *.sup, *.m2v, *.264, *.vc1, *.ac3, *.m2a and *.pcm files\n");
        fprintf(stderr,"\n");
        return 0;
    }

    bool parse_only=false;
    int dump=0;
    bool av_only=true;
    bool join=false;
    int channel=0;
    bool pes=false;
    bool verb=false;
    std::string output;
    bool mkvmerge_opts=false;

    std::string mpls_file;                      // MPLS file

    std::list<std::string> playlist;            // playlist
    std::map<int,std::string> mpls_datetime;    // AVCHD clip date/time

    int opt;
    while((opt=getopt(argc,argv,"pe:ujc:zo:d:l:vms:"))>=0)
        switch(opt)
        {
        case 'p':
            parse_only=true;
            break;
        case 'e':
            dump=atoi(optarg);
            break;
        case 'u':
            av_only=false;
            break;
        case 'j':
            join=true;
            break;
        case 'c':
            channel=atoi(optarg);
            break;
        case 'z':
            pes=true;
            break;
        case 'o':
            output=ts::trim_slash(optarg);
            break;
        case 'd':
            {
                std::list<std::string> l;
                ts::scan_dir(ts::trim_slash(optarg).c_str(),l);
                l.sort();
                playlist.merge(l);
            }
            break;
        case 'l':
            mpls_file=optarg;
            break;
        case 's':
            {
                std::list<std::string> l;
                ts::load_playlist(optarg,l,mpls_datetime);
                playlist.merge(l);
            }
            break;
        case 'v':
            verb=true;
            break;
        case 'm':
            mkvmerge_opts=true;
            break;
        }

    while(optind<argc)
    {
        playlist.push_back(argv[optind]);
        optind++;
    }

    if(mpls_file.length())
    {
        std::list<int> mpls;                        // list of clip id from mpls files
        std::list<std::string> new_playlist;

        if(mpls::parse(mpls_file.c_str(),mpls,mpls_datetime,playlist.size()?verb:1))
            fprintf(stderr,"%s: invalid playlist file format\n",mpls_file.c_str());

        if(mpls.size())
        {
            std::map<int,std::string> clips;
            for(std::list<std::string>::iterator i=playlist.begin();i!=playlist.end();++i)
            {
                std::string& s=*i;
                clips[ts::get_clip_number_by_filename(s)]=s;
            }

            for(std::list<int>::iterator i=mpls.begin();i!=mpls.end();++i)
            {
                std::string& s=clips[*i];

                if(s.length())
                    new_playlist.push_back(s);
            }

            playlist.swap(new_playlist);
        }
    }

    time_t beg_time=time(0);

    if(!ts::is_ts_filename(output))
    {
        if(join)
        {
            if(playlist.size())
            {
                std::string chapters_filename=output.length()?(output+os_slash+"chapters.xml"):"chapters.xml";

                FILE* fp=0;

                if(channel)
                {
                    fp=fopen(chapters_filename.c_str(),"w");

                    if(!fp)
                        fprintf(stderr,"can`t create %s\n",chapters_filename.c_str());
                }

                if(fp)
                {
                    fprintf(fp,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n");
                    fprintf(fp,"<!-- <!DOCTYPE Tags SYSTEM \"matroskatags.dtd\"> -->\n\n");
                    fprintf(fp,"<Chapters>\n");
                    fprintf(fp,"  <EditionEntry>\n");
                }

                ts::demuxer demuxer;

                int cn=1;

                for(std::list<std::string>::iterator i=playlist.begin();i!=playlist.end();++i,cn++)
                {
                    const std::string& s=*i;

                    int clip=ts::get_clip_number_by_filename(s);
                    const std::string& date=mpls_datetime[clip];
                    u_int32_t offset=demuxer.base_pts/90;

                    if(verb)
                        fprintf(stderr,"%.5i: '%s', %u\n",clip,date.c_str(),offset);

                    if(fp)
                    {
                        fprintf(fp,"    <ChapterAtom>\n");
                        fprintf(fp,"      <ChapterTimeStart>%s</ChapterTimeStart>\n",ts::timecode_to_time(offset));
                        fprintf(fp,"      <ChapterDisplay>\n");
                        if(date.length())
                            fprintf(fp,"        <ChapterString>%s</ChapterString>\n",date.c_str());
                        else
                            fprintf(fp,"        <ChapterString>Clip %i</ChapterString>\n",cn);
                        fprintf(fp,"      </ChapterDisplay>\n");
                        fprintf(fp,"    </ChapterAtom>\n");
                    }

                    demuxer.av_only=av_only;
                    demuxer.channel=channel;
                    demuxer.pes_output=pes;
                    demuxer.dst=output;
                    demuxer.verb=verb;
                    demuxer.es_parse=true;

                    demuxer.demux_file(s.c_str());

                    demuxer.gen_timecodes(date);

                    demuxer.reset();
                }

                demuxer.show();

                if(mkvmerge_opts)
                {
                    fprintf(stdout,"\n# mkvmerge -o output.mkv ");
                    if(fp)
                        fprintf(stdout,"--chapters %s ",chapters_filename.c_str());
                    for(std::map<u_int16_t,ts::stream>::iterator i=demuxer.streams.begin();i!=demuxer.streams.end();++i)
                    {
                        ts::stream& s=i->second;

                        if(s.file.filename.length())
                        {
                            std::string::size_type n=s.file.filename.find_last_of('.');
                            if(n!=std::string::npos)
                            {
                                std::string filename=s.file.filename.substr(0,n);
                                filename+=".tmc";
                                fprintf(stdout,"--timecodes 0:%s ",filename.c_str());
                            }
                            fprintf(stdout,"%s ",s.file.filename.c_str());
                        }
                    }

                    if(demuxer.subs_filename.length())
                        fprintf(stdout,"%s",demuxer.subs_filename.c_str());

                    fprintf(stdout,"\n");
                }

                if(fp)
                {
                    fprintf(fp,"  </EditionEntry>\n");
                    fprintf(fp,"</Chapters>\n");
                    fclose(fp);
                }
            }
        }else
        {
            for(std::list<std::string>::iterator i=playlist.begin();i!=playlist.end();++i)
            {
                const std::string& s=*i;

                ts::demuxer demuxer;

                demuxer.parse_only=dump>0?true:parse_only;
                demuxer.es_parse=demuxer.parse_only;
                demuxer.dump=dump;
                demuxer.av_only=av_only;
                demuxer.channel=channel;
                demuxer.pes_output=pes;
                demuxer.dst=output;
                demuxer.verb=verb;

                demuxer.demux_file(s.c_str());

                demuxer.show();
            }
        }
    }else
    {
        // join to TS/M2TS file
        fprintf(stderr,"join to TS/M2TS is not implemented!\n");
/*
        std::list<ts::ts_file_info> info;

        if(!channel)
        {
            fprintf(stderr,"the channel is not chosen, set to 1\n");
            channel=1;
        }


        fprintf(stderr,"\nstep 1 - analyze a stream\n\n");

        for(std::list<std::string>::iterator i=playlist.begin();i!=playlist.end();++i)
        {
            const std::string& name=*i;

            ts::demuxer demuxer;

            demuxer.parse_only=true;
            demuxer.av_only=av_only;
            demuxer.channel=channel;

            demuxer.demux_file(name.c_str());
            demuxer.show();

            info.push_back(ts::ts_file_info());
            ts::ts_file_info& nfo=info.back();
            nfo.filename=name;

            for(std::map<u_int16_t,ts::stream>::const_iterator i=demuxer.streams.begin();i!=demuxer.streams.end();++i)
            {
                const ts::stream& s=i->second;

                if(s.type!=0xff)
                {
                    if(s.first_dts)
                    {
                        if(s.first_dts<nfo.first_dts || !nfo.first_dts)
                            nfo.first_dts=s.first_dts;
                    }

                    if(s.first_pts<nfo.first_pts || !nfo.first_pts)
                        nfo.first_pts=s.first_pts;

                    u_int64_t n=s.last_pts+s.frame_length;

                    if(n>nfo.last_pts || !nfo.last_pts)
                        nfo.last_pts=n;
                }
            }
        }

        fprintf(stderr,"\nstep 2 - remux\n\n");

        u_int64_t cur_pts=0;

        int fn=0;

        for(std::list<ts::ts_file_info>::iterator i=info.begin();i!=info.end();++i,fn++)
        {
            ts::ts_file_info& nfo=*i;

            u_int64_t first_pts=nfo.first_dts>0?(nfo.first_pts-nfo.first_dts):0;

            if(!fn)
                cur_pts=first_pts;

            fprintf(stderr,"%s: %llu (len=%llu)\n",nfo.filename.c_str(),cur_pts,nfo.last_pts-first_pts);

            // new_pes_pts=cur_pts+(pts_from_pes-first_pts)


            cur_pts+=nfo.last_pts-first_pts;
        }
*/
    }

    fprintf(stderr,"\ntime: %li sec\n",time(0)-beg_time);

    return 0;
}
