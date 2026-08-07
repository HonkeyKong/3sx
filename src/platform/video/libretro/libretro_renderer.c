#include "platform/video/libretro/libretro_renderer.h"
#include "platform/video/opengl/glad/glad.h"
#include "sf33rd/AcrSDK/ps2/foundaps2.h"
#include "sf33rd/AcrSDK/ps2/flps2etc.h"
#include <libgraph.h>
#include <stdlib.h>
#include <string.h>

#define W 384
#define H 224
#define MAX_QUADS 2048

enum TextureKind { TEX_NONE, TEX_DIRECT, TEX_INDEX4, TEX_INDEX8 };
typedef struct { GLuint id; int w,h; enum TextureKind kind; } LRTexture;
typedef struct { float x[4],y[4],s[4],t[4],z; uint32_t color; int tex,pal; } Draw;
typedef struct { float x,y,z,s,t,r,g,b,a; } LRVertex;

static LRTexture textures[FL_TEXTURE_MAX];
static GLuint palettes[FL_PALETTE_MAX];
static Draw draws[MAX_QUADS];
static unsigned draw_count;
static int current_tex=-1,current_pal=-1;
static GLuint vao,vbo,ebo,solid_program,direct_program,index4_program,index8_program;
static uintptr_t (*current_framebuffer)(void);
static bool context_ready;

static const char *vertex_source =
    "#version 300 es\nprecision highp float;"
    "layout(location=0)in vec3 position;layout(location=1)in vec2 texcoord;layout(location=2)in vec4 color;"
    "out vec2 uv;out vec4 tint;void main(){gl_Position=vec4(position,1.0);uv=texcoord;tint=color;}";
static const char *solid_source =
    "#version 300 es\nprecision mediump float;in vec4 tint;out vec4 out_color;void main(){out_color=tint;}";
static const char *direct_source =
    "#version 300 es\nprecision mediump float;in vec2 uv;in vec4 tint;out vec4 out_color;uniform sampler2D image;"
    "void main(){out_color=texture(image,uv)*tint;if(out_color.a==0.0)discard;}";
static const char *index8_source =
    "#version 300 es\nprecision highp float;precision highp usampler2D;in vec2 uv;in vec4 tint;out vec4 out_color;"
    "uniform usampler2D indices;uniform sampler2D palette;void main(){uint i=texture(indices,uv).r;out_color=texelFetch(palette,ivec2(int(i),0),0)*tint;if(out_color.a==0.0)discard;}";
static const char *index4_source =
    "#version 300 es\nprecision highp float;precision highp usampler2D;in vec2 uv;in vec4 tint;out vec4 out_color;"
    "uniform usampler2D indices;uniform sampler2D palette;uniform ivec2 image_size;"
    "void main(){ivec2 p=clamp(ivec2(uv*vec2(image_size)),ivec2(0),image_size-1);uint v=texelFetch(indices,ivec2(p.x/2,p.y),0).r;uint i=(p.x&1)==0?v&15u:v>>4;out_color=texelFetch(palette,ivec2(int(i),0),0)*tint;if(out_color.a==0.0)discard;}";

static GLuint compile_shader(GLenum type,const char *source){GLuint s=glCreateShader(type);glShaderSource(s,1,&source,NULL);glCompileShader(s);GLint ok;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){glDeleteShader(s);return 0;}return s;}
static GLuint make_program(const char *fragment){GLuint v=compile_shader(GL_VERTEX_SHADER,vertex_source),f=compile_shader(GL_FRAGMENT_SHADER,fragment);if(!v||!f)return 0;GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);glDeleteShader(v);glDeleteShader(f);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){glDeleteProgram(p);return 0;}return p;}
static float sx(float x){return x/192.0f-1.0f;}
static float sy(float y){return 1.0f-y/112.0f;}
static void color(uint32_t c,LRVertex *v){v->r=((c>>16)&255)/255.0f;v->g=((c>>8)&255)/255.0f;v->b=(c&255)/255.0f;v->a=(c>>24)/255.0f;}
static void rgba16(const uint16_t*source,uint8_t*dest,int count){for(int i=0;i<count;i++){uint16_t v=source[i];dest[i*4]=(uint8_t)((v&31)*255/31);dest[i*4+1]=(uint8_t)(((v>>5)&31)*255/31);dest[i*4+2]=(uint8_t)(((v>>10)&31)*255/31);dest[i*4+3]=(v&0x8000)?255:0;}}

static void upload_texture(int i){
    FLTexture *f=&flTexture[i];const void *pixels=flPS2GetSystemBuffAdrs(f->mem_handle);if(!pixels)return;
    if(textures[i].id)glDeleteTextures(1,&textures[i].id);glGenTextures(1,&textures[i].id);glBindTexture(GL_TEXTURE_2D,textures[i].id);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    textures[i].w=f->width;textures[i].h=f->height;
    if(f->format==SCE_GS_PSMT8){textures[i].kind=TEX_INDEX8;glTexImage2D(GL_TEXTURE_2D,0,GL_R8UI,f->width,f->height,0,GL_RED_INTEGER,GL_UNSIGNED_BYTE,pixels);}
    else if(f->format==SCE_GS_PSMT4){textures[i].kind=TEX_INDEX4;glTexImage2D(GL_TEXTURE_2D,0,GL_R8UI,(f->width+1)/2,f->height,0,GL_RED_INTEGER,GL_UNSIGNED_BYTE,pixels);}
    else if(f->format==SCE_GS_PSMCT16){int count=f->width*f->height;uint8_t*rgba=malloc((size_t)count*4);if(!rgba)return;rgba16(pixels,rgba,count);textures[i].kind=TEX_DIRECT;glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,f->width,f->height,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba);free(rgba);}
    else {textures[i].kind=TEX_DIRECT;glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,f->width,f->height,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);}
}
static void upload_palette(int i){FLTexture*f=&flPalette[i];const void*p=flPS2GetSystemBuffAdrs(f->mem_handle);int n=f->width*f->height;if(n>256)n=256;if(palettes[i])glDeleteTextures(1,&palettes[i]);glGenTextures(1,&palettes[i]);glBindTexture(GL_TEXTURE_2D,palettes[i]);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glPixelStorei(GL_UNPACK_ALIGNMENT,1);uint8_t rgba[256*4];if(f->format==SCE_GS_PSMCT16)rgba16(p,rgba,n);else{const uint8_t*bgra=p;for(int k=0;k<n;k++){rgba[k*4]=bgra[k*4+2];rgba[k*4+1]=bgra[k*4+1];rgba[k*4+2]=bgra[k*4];rgba[k*4+3]=bgra[k*4+3];}}glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,n,1,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba);}

bool LibretroRenderer_ContextReset(void *(*get_proc_address)(const char *),uintptr_t (*get_framebuffer)(void)){
    if(!gladLoadGLLoader((GLADloadproc)get_proc_address))return false;solid_program=make_program(solid_source);direct_program=make_program(direct_source);index4_program=make_program(index4_source);index8_program=make_program(index8_source);if(!solid_program||!direct_program||!index4_program||!index8_program)return false;
    glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,sizeof(LRVertex)*4,NULL,GL_STREAM_DRAW);glGenBuffers(1,&ebo);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);const uint16_t ix[]={0,1,2,2,1,3};glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(ix),ix,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(LRVertex),(void*)0);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(LRVertex),(void*)(3*sizeof(float)));glVertexAttribPointer(2,4,GL_FLOAT,GL_FALSE,sizeof(LRVertex),(void*)(5*sizeof(float)));glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);glEnableVertexAttribArray(2);current_framebuffer=get_framebuffer;context_ready=true;
    memset(textures,0,sizeof(textures));memset(palettes,0,sizeof(palettes));for(int i=0;i<FL_TEXTURE_MAX;i++)if(flTexture[i].be_flag)upload_texture(i);for(int i=0;i<FL_PALETTE_MAX;i++)if(flPalette[i].be_flag)upload_palette(i);return true;
}
bool LibretroRenderer_ContextReady(void){return context_ready;}
bool LibretroRenderer_Init(void){draw_count=0;return true;}
void LibretroRenderer_Quit(void){if(context_ready){for(int i=0;i<FL_TEXTURE_MAX;i++)if(textures[i].id)glDeleteTextures(1,&textures[i].id);for(int i=0;i<FL_PALETTE_MAX;i++)if(palettes[i])glDeleteTextures(1,&palettes[i]);glDeleteBuffers(1,&vbo);glDeleteBuffers(1,&ebo);glDeleteVertexArrays(1,&vao);glDeleteProgram(solid_program);glDeleteProgram(direct_program);glDeleteProgram(index4_program);glDeleteProgram(index8_program);}context_ready=false;current_framebuffer=NULL;}
void LibretroRenderer_CreateTexture(unsigned h){int i=(h&0xffff)-1;if(context_ready&&i>=0&&i<FL_TEXTURE_MAX)upload_texture(i);}void LibretroRenderer_DestroyTexture(unsigned h){int i=(h&0xffff)-1;if(i>=0&&i<FL_TEXTURE_MAX&&textures[i].id){glDeleteTextures(1,&textures[i].id);memset(&textures[i],0,sizeof(LRTexture));}}
void LibretroRenderer_CreatePalette(unsigned h){int i=((h>>16)&0xffff)-1;if(context_ready&&i>=0&&i<FL_PALETTE_MAX)upload_palette(i);}void LibretroRenderer_DestroyPalette(unsigned h){int i=((h>>16)&0xffff)-1;if(i>=0&&i<FL_PALETTE_MAX&&palettes[i]){glDeleteTextures(1,&palettes[i]);palettes[i]=0;}}
void LibretroRenderer_SetTexture(unsigned h){current_tex=(h&0xffff)-1;current_pal=((h>>16)&0xffff)-1;}
static void add(const float*x,const float*y,const float*s,const float*t,float z,uint32_t c){if(draw_count>=MAX_QUADS)return;Draw*d=&draws[draw_count++];memcpy(d->x,x,sizeof(d->x));memcpy(d->y,y,sizeof(d->y));if(s){memcpy(d->s,s,sizeof(d->s));memcpy(d->t,t,sizeof(d->t));}else{memset(d->s,0,sizeof(d->s));memset(d->t,0,sizeof(d->t));}d->z=z;d->color=c;d->tex=current_tex;d->pal=current_pal;}
void LibretroRenderer_DrawTexturedQuad(const Sprite*q,unsigned c){float x[4],y[4],s[4],t[4];for(int i=0;i<4;i++){x[i]=q->v[i].x;y[i]=q->v[i].y;s[i]=q->t[i].s;t[i]=q->t[i].t;}add(x,y,s,t,q->v[0].z,c);}void LibretroRenderer_DrawSprite(const Sprite*q,unsigned c){float x[4]={q->v[0].x,q->v[3].x,q->v[0].x,q->v[3].x},y[4]={q->v[0].y,q->v[0].y,q->v[3].y,q->v[3].y},s[4]={q->t[0].s,q->t[3].s,q->t[0].s,q->t[3].s},t[4]={q->t[0].t,q->t[0].t,q->t[3].t,q->t[3].t};add(x,y,s,t,q->v[0].z,c);}void LibretroRenderer_DrawSprite2(const Sprite2*q){float x[4]={q->v[0].x,q->v[1].x,q->v[0].x,q->v[1].x},y[4]={q->v[0].y,q->v[0].y,q->v[1].y,q->v[1].y},s[4]={q->t[0].s,q->t[1].s,q->t[0].s,q->t[1].s},t[4]={q->t[0].t,q->t[0].t,q->t[1].t,q->t[1].t};add(x,y,s,t,q->v[0].z,q->vertex_color);}void LibretroRenderer_DrawSolidQuad(const Quad*q,unsigned c){float x[4],y[4];for(int i=0;i<4;i++){x[i]=q->v[i].x;y[i]=q->v[i].y;}int old=current_tex;current_tex=-1;add(x,y,NULL,NULL,q->v[0].z,c);current_tex=old;}
void LibretroRenderer_Present(void){
    glBindFramebuffer(GL_FRAMEBUFFER,(GLuint)current_framebuffer());glViewport(0,0,W,H);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glBindVertexArray(vao);
    for(unsigned n=0;n<draw_count;n++){Draw*d=&draws[n];LRVertex v[4];for(int k=0;k<4;k++){v[k]=(LRVertex){sx(d->x[k]),sy(d->y[k]),d->z,d->s[k],d->t[k],0,0,0,0};color(d->color,&v[k]);}glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(v),v);
        if(d->tex<0||d->tex>=FL_TEXTURE_MAX||!textures[d->tex].id)glUseProgram(solid_program);else{LRTexture*t=&textures[d->tex];GLuint p=t->kind==TEX_INDEX4?index4_program:t->kind==TEX_INDEX8?index8_program:direct_program;glUseProgram(p);if(t->kind==TEX_DIRECT){glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,t->id);glUniform1i(glGetUniformLocation(p,"image"),0);}else{glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,t->id);glUniform1i(glGetUniformLocation(p,"indices"),0);glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,d->pal>=0?palettes[d->pal]:0);glUniform1i(glGetUniformLocation(p,"palette"),1);if(t->kind==TEX_INDEX4)glUniform2i(glGetUniformLocation(p,"image_size"),t->w,t->h);}}
        glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,0);
    }draw_count=0;glBindVertexArray(0);
}
