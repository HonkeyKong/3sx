#ifndef THREESX_LIBRETRO_SDL_COMPAT_H
#define THREESX_LIBRETRO_SDL_COMPAT_H

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int8_t Sint8; typedef uint8_t Uint8;
typedef int16_t Sint16; typedef uint16_t Uint16;
typedef int32_t Sint32; typedef uint32_t Uint32;
typedef int64_t Sint64; typedef uint64_t Uint64;
typedef struct SDL_IOStream SDL_IOStream;
typedef struct SDL_Window SDL_Window;
typedef struct { int type; } SDL_PathInfo;

#define SDL_PATHTYPE_FILE 1
#define SDL_IO_SEEK_SET SEEK_SET
#define SDL_IO_SEEK_CUR SEEK_CUR
#define SDL_IO_SEEK_END SEEK_END
#define SDL_LOG_CATEGORY_APPLICATION 0
#define SDL_LOG_PRIORITY_DEBUG 0
#define SDL_arraysize(a) (sizeof(a)/sizeof((a)[0]))
#define SDL_zero(x) memset(&(x),0,sizeof(x))
#define SDL_zerop(x) memset((x),0,sizeof(*(x)))
#define SDL_zeroa(x) memset((x),0,sizeof(x))
#define SDL_assert(x) assert(x)
#define SDL_malloc malloc
#define SDL_calloc calloc
#define SDL_free free
#define SDL_memcpy memcpy
#define SDL_memmove memmove
#define SDL_strlen strlen
#define SDL_strcmp strcmp
#define SDL_strncmp strncmp
#define SDL_snprintf snprintf
#define SDL_vsnprintf vsnprintf
#define SDL_atoi atoi
#define SDL_isdigit isdigit
#define SDL_isspace isspace
#define SDL_fabsf fabsf
#define SDL_floor floor
#define SDL_sqrt sqrt
#define SDL_cos cos
#define SDL_min(a,b) ((a)<(b)?(a):(b))
#define SDL_max(a,b) ((a)>(b)?(a):(b))
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SDL_Swap16BE(x) __builtin_bswap16((Uint16)(x))
#define SDL_Swap32BE(x) __builtin_bswap32((Uint32)(x))
#else
#define SDL_Swap16BE(x) ((Uint16)(x))
#define SDL_Swap32BE(x) ((Uint32)(x))
#endif

SDL_IOStream* SDL_IOFromFile(const char*,const char*);
SDL_IOStream* SDL_IOFromConstMem(const void*,size_t);
size_t SDL_ReadIO(SDL_IOStream*,void*,size_t);
size_t SDL_WriteIO(SDL_IOStream*,const void*,size_t);
Sint64 SDL_SeekIO(SDL_IOStream*,Sint64,int);
Sint64 SDL_GetIOSize(SDL_IOStream*);
bool SDL_CloseIO(SDL_IOStream*);
bool SDL_ReadU8(SDL_IOStream*,Uint8*);
bool SDL_ReadS8(SDL_IOStream*,Sint8*);
bool SDL_ReadU16BE(SDL_IOStream*,Uint16*);
bool SDL_ReadU32BE(SDL_IOStream*,Uint32*);
bool SDL_ReadU32LE(SDL_IOStream*,Uint32*);
char* SDL_strdup(const char*);
size_t SDL_strlcpy(char*,const char*,size_t);
char* SDL_strtok_r(char*,const char*,char**);
int SDL_asprintf(char**,const char*,...);
int SDL_vasprintf(char**,const char*,va_list);
bool SDL_CreateDirectory(const char*);
bool SDL_GetPathInfo(const char*,SDL_PathInfo*);
const char* SDL_GetBasePath(void);
const char* SDL_GetPrefPath(const char*,const char*);
const char* SDL_GetError(void);
void SDL_Log(const char*,...);
void SDL_LogMessage(int,int,const char*,...);

#endif
