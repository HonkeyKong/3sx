#include <SDL3/SDL.h>
#include "port/paths.h"
#include "port/utils.h"
#include <errno.h>
#include <sys/stat.h>

struct SDL_IOStream { FILE* file; const Uint8* memory; size_t size,pos; bool owned; };
static char error_text[128];
static void error_errno(void){snprintf(error_text,sizeof(error_text),"%s",strerror(errno));}
SDL_IOStream* SDL_IOFromFile(const char*p,const char*m){FILE*f=fopen(p,m);if(!f){error_errno();return NULL;}SDL_IOStream*s=calloc(1,sizeof(*s));if(!s){fclose(f);return NULL;}s->file=f;s->owned=true;return s;}
SDL_IOStream* SDL_IOFromConstMem(const void*p,size_t n){SDL_IOStream*s=calloc(1,sizeof(*s));if(s){s->memory=p;s->size=n;}return s;}
size_t SDL_ReadIO(SDL_IOStream*s,void*p,size_t n){if(!s)return 0;if(s->file)return fread(p,1,n,s->file);if(n>s->size-s->pos)n=s->size-s->pos;memcpy(p,s->memory+s->pos,n);s->pos+=n;return n;}
size_t SDL_WriteIO(SDL_IOStream*s,const void*p,size_t n){return s&&s->file?fwrite(p,1,n,s->file):0;}
Sint64 SDL_SeekIO(SDL_IOStream*s,Sint64 off,int whence){if(!s)return -1;if(s->file){if(fseeko(s->file,(off_t)off,whence))return -1;return ftello(s->file);}size_t base=whence==SEEK_SET?0:whence==SEEK_CUR?s->pos:s->size;if(off<0&&(uint64_t)(-off)>base)return -1;size_t pos=base+off;if(pos>s->size)return -1;s->pos=pos;return pos;}
Sint64 SDL_GetIOSize(SDL_IOStream*s){if(!s)return -1;if(!s->file)return s->size;off_t p=ftello(s->file);fseeko(s->file,0,SEEK_END);off_t n=ftello(s->file);fseeko(s->file,p,SEEK_SET);return n;}
bool SDL_CloseIO(SDL_IOStream*s){if(!s)return false;int rc=s->owned?fclose(s->file):0;free(s);return rc==0;}
bool SDL_ReadU8(SDL_IOStream*s,Uint8*v){return SDL_ReadIO(s,v,1)==1;}bool SDL_ReadS8(SDL_IOStream*s,Sint8*v){return SDL_ReadIO(s,v,1)==1;}
bool SDL_ReadU16BE(SDL_IOStream*s,Uint16*v){Uint8 b[2];if(SDL_ReadIO(s,b,2)!=2)return false;*v=(Uint16)(b[0]<<8|b[1]);return true;}
bool SDL_ReadU32BE(SDL_IOStream*s,Uint32*v){Uint8 b[4];if(SDL_ReadIO(s,b,4)!=4)return false;*v=(Uint32)b[0]<<24|(Uint32)b[1]<<16|(Uint32)b[2]<<8|b[3];return true;}
bool SDL_ReadU32LE(SDL_IOStream*s,Uint32*v){Uint8 b[4];if(SDL_ReadIO(s,b,4)!=4)return false;*v=(Uint32)b[3]<<24|(Uint32)b[2]<<16|(Uint32)b[1]<<8|b[0];return true;}
char* SDL_strdup(const char*s){size_t n=strlen(s)+1;char*d=malloc(n);if(d)memcpy(d,s,n);return d;}
size_t SDL_strlcpy(char*d,const char*s,size_t n){size_t z=strlen(s);if(n){size_t c=z<n-1?z:n-1;memcpy(d,s,c);d[c]=0;}return z;}
char* SDL_strtok_r(char*s,const char*d,char**save){return strtok_r(s,d,save);}
int SDL_vasprintf(char**out,const char*fmt,va_list ap){va_list cp;va_copy(cp,ap);int n=vsnprintf(NULL,0,fmt,cp);va_end(cp);if(n<0)return n;*out=malloc((size_t)n+1);if(!*out)return -1;return vsnprintf(*out,(size_t)n+1,fmt,ap);}
int SDL_asprintf(char**out,const char*fmt,...){va_list ap;va_start(ap,fmt);int n=SDL_vasprintf(out,fmt,ap);va_end(ap);return n;}
bool SDL_CreateDirectory(const char*p){return mkdir(p,0755)==0||errno==EEXIST;}
bool SDL_GetPathInfo(const char*p,SDL_PathInfo*i){struct stat s;if(stat(p,&s))return false;i->type=S_ISREG(s.st_mode)?SDL_PATHTYPE_FILE:0;return true;}
const char* SDL_GetBasePath(void){return Paths_GetBasePath();}const char* SDL_GetPrefPath(const char*a,const char*b){(void)a;(void)b;return Paths_GetPrefPath();}
const char* SDL_GetError(void){return error_text;}
void SDL_Log(const char*fmt,...){va_list ap;va_start(ap,fmt);Log_WriteV(LOG_LEVEL_INFO,fmt,ap);va_end(ap);}void SDL_LogMessage(int c,int p,const char*fmt,...){(void)c;va_list ap;va_start(ap,fmt);Log_WriteV(p<=SDL_LOG_PRIORITY_DEBUG?LOG_LEVEL_DEBUG:LOG_LEVEL_INFO,fmt,ap);va_end(ap);}
