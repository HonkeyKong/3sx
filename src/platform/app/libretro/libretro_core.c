/* 3SX libretro frontend. Copyright (C) 2026 3SX contributors. AGPL-3.0-or-later. */
#include "libretro/libretro.h"
#include "main.h"
#include "platform/app/libretro/online_start.h"
#include "platform/input/libretro/libretro_input.h"
#include "platform/video/libretro/libretro_renderer.h"
#include "port/io/afs.h"
#include "port/paths.h"
#include "port/resources.h"
#include "port/sound/adx.h"
#include "port/sound/spu.h"
#include "port/config/config.h"
#include "arcade/arcade_balance.h"
#include "core/rollback_state.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define FPS 59.59949
#define RATE 48000.0
static retro_environment_t environ_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static void (*log_cb)(int,const char*,...);
static bool loaded;
static bool start_mode_configured;
static double audio_remainder;
static int16_t audio_buffer[2048*2];
static char save_root[4096], system_root[4096];
static struct retro_hw_render_callback hw;
RETRO_API void retro_unload_game(void);

static void configure_start_mode(void){struct retro_variable var={"3sx_start_mode",NULL};bool enabled=environ_cb&&environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE,&var)&&var.value&&strcmp(var.value,"Online Only")==0;LibretroOnlineStart_SetEnabled(enabled);start_mode_configured=true;}

static void fallback_log(int level,const char*fmt,...){(void)level;va_list ap;va_start(ap,fmt);vfprintf(stderr,fmt,ap);va_end(ap);}
static void message(const char*s){struct retro_message m={s,300};if(environ_cb)environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE,&m);log_cb(RETRO_LOG_ERROR,"3SX: %s\n",s);}
static void context_reset(void){if(!LibretroRenderer_ContextReset((void *(*)(const char *))hw.get_proc_address,hw.get_current_framebuffer))message("Could not initialize GLES renderer");}
static void context_destroy(void){LibretroRenderer_Quit();}
static bool button(unsigned p,unsigned id){return input_state_cb&&input_state_cb(p,RETRO_DEVICE_JOYPAD,0,id)!=0;}
static void poll_input(void){if(input_poll_cb)input_poll_cb();for(unsigned p=0;p<2;p++){Input_ButtonState s={0};s.dpad_up=button(p,RETRO_DEVICE_ID_JOYPAD_UP);s.dpad_down=button(p,RETRO_DEVICE_ID_JOYPAD_DOWN);s.dpad_left=button(p,RETRO_DEVICE_ID_JOYPAD_LEFT);s.dpad_right=button(p,RETRO_DEVICE_ID_JOYPAD_RIGHT);s.start=button(p,RETRO_DEVICE_ID_JOYPAD_START);s.back=button(p,RETRO_DEVICE_ID_JOYPAD_SELECT);s.west=button(p,RETRO_DEVICE_ID_JOYPAD_Y);s.north=button(p,RETRO_DEVICE_ID_JOYPAD_X);s.left_shoulder=button(p,RETRO_DEVICE_ID_JOYPAD_L);s.south=button(p,RETRO_DEVICE_ID_JOYPAD_B);s.east=button(p,RETRO_DEVICE_ID_JOYPAD_A);s.right_shoulder=button(p,RETRO_DEVICE_ID_JOYPAD_R);LibretroInput_SetState(p,&s);}}
RETRO_API unsigned retro_api_version(void){return RETRO_API_VERSION;}
RETRO_API void retro_set_environment(retro_environment_t cb){environ_cb=cb;bool no=false;cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME,&no);static const struct retro_variable vars[]={{"3sx_start_mode","Start Mode; Normal|Online Only"},{NULL,NULL}};cb(RETRO_ENVIRONMENT_SET_VARIABLES,(void*)vars);}
RETRO_API void retro_set_video_refresh(retro_video_refresh_t cb){video_cb=cb;}
RETRO_API void retro_set_audio_sample(retro_audio_sample_t cb){audio_cb=cb;}
RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb){audio_batch_cb=cb;}
RETRO_API void retro_set_input_poll(retro_input_poll_t cb){input_poll_cb=cb;}
RETRO_API void retro_set_input_state(retro_input_state_t cb){input_state_cb=cb;}
RETRO_API void retro_init(void){log_cb=fallback_log;struct retro_log_callback l;if(environ_cb&&environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE,&l))log_cb=l.log;unsigned level=3;environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL,&level);int fmt=RETRO_PIXEL_FORMAT_XRGB8888;if(!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT,&fmt))message("Frontend does not support XRGB8888");static const struct retro_controller_description pads[]={{"3SX Fight Pad",RETRO_DEVICE_JOYPAD}};static const struct retro_controller_info ports[]={{pads,1},{pads,1},{NULL,0}};environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO,(void*)ports);start_mode_configured=false;}
RETRO_API void retro_deinit(void){if(loaded)retro_unload_game();LibretroOnlineStart_Reset();environ_cb=NULL;log_cb=fallback_log;}
RETRO_API void retro_get_system_info(struct retro_system_info*i){memset(i,0,sizeof(*i));i->library_name="3SX";i->library_version="0.1";i->valid_extensions="afs";i->need_fullpath=true;i->block_extract=true;}
RETRO_API void retro_get_system_av_info(struct retro_system_av_info*i){memset(i,0,sizeof(*i));i->geometry=(struct retro_game_geometry){384,224,384,224,4.0f/3.0f};i->timing=(struct retro_system_timing){FPS,RATE};}
RETRO_API void retro_set_controller_port_device(unsigned port,unsigned device){(void)port;(void)device;}
RETRO_API void retro_reset(void){message("Reset requires reloading content in this version");}
RETRO_API bool retro_load_game(const struct retro_game_info*g){if(!g||!g->path){message("Select a legally obtained SF33RD.AFS file");return false;}memset(&hw,0,sizeof(hw));hw.context_type=RETRO_HW_CONTEXT_OPENGLES3;hw.context_reset=context_reset;hw.context_destroy=context_destroy;hw.depth=true;hw.bottom_left_origin=true;hw.version_major=3;hw.version_minor=0;if(!environ_cb(RETRO_ENVIRONMENT_SET_HW_RENDER,&hw)){message("Frontend does not support OpenGL ES 3");return false;}const char*sys=NULL,*save=NULL;if(environ_cb){environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY,&sys);environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY,&save);}snprintf(system_root,sizeof(system_root),"%.*s/3sx/",(int)sizeof(system_root)-6,sys?sys:"");snprintf(save_root,sizeof(save_root),"%.*s/3sx/",(int)sizeof(save_root)-6,save?save:system_root);SDL_CreateDirectory(save_root);Paths_SetOverrides(save_root,system_root);Resources_SetAFSPath(g->path);if(!Resources_Check()){message("Missing or invalid SF33RD.AFS");goto fail;}if(!LibretroRenderer_Init()){message("GLES renderer initialization failed");goto fail;}Config_Init();ArcadeBalance_Init();if(!AFS_Init(g->path,256*1024)){message("Could not open SF33RD.AFS");Config_Destroy();LibretroRenderer_Quit();goto fail;}LibretroInput_Reset();Main_Init();audio_remainder=0;loaded=true;return true;fail:Resources_Reset();Paths_ResetOverrides();return false;}
RETRO_API void retro_unload_game(void){if(!loaded)return;Main_Shutdown();AFS_Finish();Config_Destroy();LibretroRenderer_Quit();LibretroInput_Reset();LibretroOnlineStart_Reset();Resources_Reset();Paths_ResetOverrides();loaded=false;start_mode_configured=false;}
RETRO_API void retro_run(void){if(!loaded||!LibretroRenderer_ContextReady())return;if(!start_mode_configured)configure_start_mode();poll_input();AFS_RunServer();LibretroOnlineStart_Tick();Main_StepFrame();ADX_ProcessTracks();LibretroRenderer_Present();Main_FinishFrame();if(video_cb)video_cb(RETRO_HW_FRAME_BUFFER_VALID,384,224,0);double exact=RATE/FPS+audio_remainder;unsigned frames=(unsigned)exact;audio_remainder=exact-frames;if(frames>2048)frames=2048;SPU_RenderSamples(audio_buffer,frames);if(audio_batch_cb)audio_batch_cb(audio_buffer,frames);else if(audio_cb)for(unsigned i=0;i<frames;i++)audio_cb(audio_buffer[i*2],audio_buffer[i*2+1]);}
RETRO_API unsigned retro_get_region(void){return RETRO_REGION_NTSC;}
RETRO_API size_t retro_serialize_size(void){return sizeof(RollbackState);}
RETRO_API bool retro_serialize(void*d,size_t s){if(!loaded||!mpp_w.inGame||!d||s<sizeof(RollbackState))return false;RollbackState_Save(d);return true;}
RETRO_API bool retro_unserialize(const void*d,size_t s){if(!loaded||!mpp_w.inGame)return false;return RollbackState_Load(d,s);}
RETRO_API uint32_t retro_get_state_hash(const void*d,size_t s){return RollbackState_Hash(d,s);}
RETRO_API void* retro_get_memory_data(unsigned id){(void)id;return NULL;}
RETRO_API size_t retro_get_memory_size(unsigned id){(void)id;return 0;}
RETRO_API void retro_cheat_reset(void){}
RETRO_API void retro_cheat_set(unsigned i,bool e,const char*c){(void)i;(void)e;(void)c;}
