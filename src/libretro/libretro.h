/* Minimal declarations from the public-domain libretro API. https://www.libretro.com/ */
#ifndef LIBRETRO_H__
#define LIBRETRO_H__
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define RETRO_API_VERSION 1
#define RETRO_DEVICE_JOYPAD 1
#define RETRO_MEMORY_SAVE_RAM 0
#define RETRO_MEMORY_SYSTEM_RAM 2
#define RETRO_REGION_NTSC 0
#define RETRO_PIXEL_FORMAT_XRGB8888 1
#define RETRO_ENVIRONMENT_SET_MESSAGE 6
#define RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL 8
#define RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY 9
#define RETRO_ENVIRONMENT_SET_PIXEL_FORMAT 10
#define RETRO_ENVIRONMENT_SET_HW_RENDER 14
#define RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME 18
#define RETRO_ENVIRONMENT_GET_LOG_INTERFACE 27
#define RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY 31
#define RETRO_ENVIRONMENT_SET_CONTROLLER_INFO 35
#define RETRO_LOG_DEBUG 0
#define RETRO_LOG_INFO 1
#define RETRO_LOG_WARN 2
#define RETRO_LOG_ERROR 3
#define RETRO_HW_FRAME_BUFFER_VALID ((void*)-1)
enum retro_hw_context_type { RETRO_HW_CONTEXT_NONE, RETRO_HW_CONTEXT_OPENGL,
 RETRO_HW_CONTEXT_OPENGLES2, RETRO_HW_CONTEXT_OPENGL_CORE,
 RETRO_HW_CONTEXT_OPENGLES3, RETRO_HW_CONTEXT_OPENGLES_VERSION };
typedef void (*retro_proc_address_t)(void);
typedef retro_proc_address_t (*retro_hw_get_proc_address_t)(const char*);
typedef uintptr_t (*retro_hw_get_current_framebuffer_t)(void);
struct retro_hw_render_callback {
 enum retro_hw_context_type context_type; void (*context_reset)(void);
 retro_hw_get_current_framebuffer_t get_current_framebuffer;
 retro_hw_get_proc_address_t get_proc_address;
 bool depth,stencil,bottom_left_origin; unsigned version_major,version_minor;
 bool cache_context; void (*context_destroy)(void); bool debug_context;
};
enum retro_device_id_joypad { RETRO_DEVICE_ID_JOYPAD_B,RETRO_DEVICE_ID_JOYPAD_Y,RETRO_DEVICE_ID_JOYPAD_SELECT,RETRO_DEVICE_ID_JOYPAD_START,RETRO_DEVICE_ID_JOYPAD_UP,RETRO_DEVICE_ID_JOYPAD_DOWN,RETRO_DEVICE_ID_JOYPAD_LEFT,RETRO_DEVICE_ID_JOYPAD_RIGHT,RETRO_DEVICE_ID_JOYPAD_A,RETRO_DEVICE_ID_JOYPAD_X,RETRO_DEVICE_ID_JOYPAD_L,RETRO_DEVICE_ID_JOYPAD_R,RETRO_DEVICE_ID_JOYPAD_L2,RETRO_DEVICE_ID_JOYPAD_R2,RETRO_DEVICE_ID_JOYPAD_L3,RETRO_DEVICE_ID_JOYPAD_R3 };
struct retro_game_info { const char *path; const void *data; size_t size; const char *meta; };
struct retro_system_info { const char *library_name,*library_version,*valid_extensions; bool need_fullpath,block_extract; };
struct retro_game_geometry { unsigned base_width,base_height,max_width,max_height; float aspect_ratio; };
struct retro_system_timing { double fps,sample_rate; };
struct retro_system_av_info { struct retro_game_geometry geometry; struct retro_system_timing timing; };
struct retro_message { const char *msg; unsigned frames; };
struct retro_log_callback { void (*log)(int,const char*,...); };
struct retro_controller_description { const char *desc; unsigned id; };
struct retro_controller_info { const struct retro_controller_description *types; unsigned num_types; };
typedef bool (*retro_environment_t)(unsigned,void*);
typedef void (*retro_video_refresh_t)(const void*,unsigned,unsigned,size_t);
typedef void (*retro_audio_sample_t)(int16_t,int16_t);
typedef size_t (*retro_audio_sample_batch_t)(const int16_t*,size_t);
typedef void (*retro_input_poll_t)(void);
typedef int16_t (*retro_input_state_t)(unsigned,unsigned,unsigned,unsigned);
#if defined(_WIN32)
#define RETRO_API __declspec(dllexport)
#elif defined(__GNUC__)
#define RETRO_API __attribute__((visibility("default")))
#else
#define RETRO_API
#endif
#endif
