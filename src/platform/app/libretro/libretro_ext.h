#ifndef THREESX_LIBRETRO_EXT_H
#define THREESX_LIBRETRO_EXT_H
#include <stdbool.h>
#include <stdint.h>
#if defined(_WIN32)
#define LIBRETRO_EXT_VISIBILITY __declspec(dllexport)
#else
#define LIBRETRO_EXT_VISIBILITY __attribute__((visibility("default")))
#endif
#if defined(__cplusplus)
#define LIBRETRO_EXT_EXPORT extern "C" LIBRETRO_EXT_VISIBILITY
#else
#define LIBRETRO_EXT_EXPORT LIBRETRO_EXT_VISIBILITY
#endif
enum libretro_ext_watch_access { LIBRETRO_EXT_WATCH_READ=1, LIBRETRO_EXT_WATCH_WRITE=2 };
enum libretro_ext_cpu_state_id { LIBRETRO_EXT_STATE_GENPC=-1, LIBRETRO_EXT_STATE_GENPCBASE=-2, LIBRETRO_EXT_STATE_GENFLAGS=-3 };
struct libretro_ext_watch_hit { bool hit; char cpuTag[64]; uint64_t address,pc,frame,totalCycles; uint32_t value; uint8_t access,width; uint32_t historyCount; uint64_t pcHistory[256]; };
#define LIBRETRO_EXT_DIP_NAME_LEN 128
#define LIBRETRO_EXT_DIP_PORT_TAG_LEN 64
#define LIBRETRO_EXT_DIP_MAX_SETTINGS 64
#define LIBRETRO_EXT_STATIC_REGION_NAME_LEN 128
#define LIBRETRO_EXT_STATIC_REGION_TAG_LEN 64
#define LIBRETRO_EXT_STATIC_REGION_CPU_TAG_LEN 64
#define LIBRETRO_EXT_STATIC_REGION_SPACE_LEN 32
#define LIBRETRO_EXT_SHARE_TAG_LEN 64
struct libretro_ext_dip_setting { char name[128]; uint32_t value; };
struct libretro_ext_dip_info { char name[128],port_tag[64]; uint32_t mask,current_value,default_value,setting_count; struct libretro_ext_dip_setting settings[64]; };
struct libretro_ext_static_region_info { char name[128],region_tag[64],cpu_tag[64],space[32]; uint64_t offset,size; uint32_t flags; };
struct libretro_ext_share_info { char share_tag[64]; uint64_t size; uint32_t flags; };
/* ABI v5 is append-only. Never reorder these fields. */
struct libretro_ext_api {
 uint32_t abi_version,sizeof_struct;
 const char*(*get_driver_name)(void); int(*get_cpu_count)(void); const char*(*get_cpu_tag)(int); uint64_t(*get_cpu_pc)(int);
 uint8_t(*read_u8)(const char*,const char*,uint64_t); uint16_t(*read_u16)(const char*,const char*,uint64_t); uint32_t(*read_u32)(const char*,const char*,uint64_t);
 void(*write_u8)(const char*,const char*,uint64_t,uint8_t); void(*write_u16)(const char*,const char*,uint64_t,uint16_t); void(*write_u32)(const char*,const char*,uint64_t,uint32_t);
 int(*get_region_count)(void); const char*(*get_region_tag)(int); uint64_t(*get_region_size)(const char*); uint64_t(*read_region)(const char*,uint64_t,void*,uint64_t); uint64_t(*write_region)(const char*,uint64_t,const void*,uint64_t);
 uint64_t(*get_frame_number)(void); uint64_t(*get_time_attoseconds)(void); uint64_t(*get_cpu_total_cycles_by_tag)(const char*);
 void(*clear_watch_rules)(void); void(*add_watch_rule)(const char*,uint64_t,uint64_t,uint8_t,uint8_t); bool(*get_last_watch_hit)(struct libretro_ext_watch_hit*); void(*clear_last_watch_hit)(void); void(*check_watch_hit)(const char*,uint64_t,uint64_t,uint32_t,uint8_t,uint8_t,uint64_t);
 int(*get_dip_count)(void); bool(*get_dip_info)(int,struct libretro_ext_dip_info*); bool(*set_dip_value)(int,uint32_t); void(*set_debug_extensions_enabled)(bool); bool(*get_debug_extensions_enabled)(void); bool(*trigger_timing_capture)(void);
 uint64_t(*get_cpu_pc_by_tag)(const char*); bool(*read_cpu_state_u64_by_index)(int,int,uint64_t*); bool(*read_cpu_state_u64_by_tag)(const char*,int,uint64_t*); bool(*read_cpu_register_by_tag)(const char*,const char*,uint64_t*);
 bool(*write_cpu_state_u64_by_index)(int,int,uint64_t); bool(*write_cpu_state_u64_by_tag)(const char*,int,uint64_t); bool(*write_cpu_register_by_tag)(const char*,const char*,uint64_t);
 void(*add_inject_rule)(const char*,uint64_t,uint64_t,uint32_t,uint8_t,bool); void(*clear_inject_rules)(void); void(*add_inject_rule_ex)(const char*,uint64_t,uint64_t,uint32_t,uint8_t,bool,bool,uint32_t,uint32_t);
 int(*get_static_region_count)(void); bool(*get_static_region_info)(int,struct libretro_ext_static_region_info*); int(*get_share_count)(void); const char*(*get_share_tag)(int); uint64_t(*get_share_size)(const char*); uint32_t(*get_share_flags)(const char*); bool(*get_share_info)(int,struct libretro_ext_share_info*); uint64_t(*read_share)(const char*,uint64_t,void*,uint64_t); uint64_t(*write_share)(const char*,uint64_t,const void*,uint64_t);
 uint64_t(*get_rollback_serialize_size)(void); bool(*rollback_serialize)(void*,uint64_t); bool(*rollback_unserialize)(const void*,uint64_t); uint32_t(*get_rollback_state_interface_version)(void); uint32_t(*get_rollback_state_format_version)(void); uint64_t(*get_rollback_state_compatibility_id)(void); uint32_t(*get_rollback_state_flags)(void); uint32_t(*get_rollback_preferred_delta_block_size)(void); uint64_t(*rollback_build_delta)(const void*,const void*,uint64_t,void*,uint64_t); bool(*rollback_apply_delta)(void*,uint64_t,const void*,uint64_t);
};
LIBRETRO_EXT_EXPORT const struct libretro_ext_api* libretro_ext_get_api(void);
LIBRETRO_EXT_EXPORT const struct libretro_ext_api* libretro_ext_get_api_v5(void);
LIBRETRO_EXT_EXPORT uint32_t libretro_ext_get_api_abi_version(void);
LIBRETRO_EXT_EXPORT uint32_t libretro_ext_get_api_struct_size(void);
void sx3_ext_game_loaded(void); void sx3_ext_game_unloaded(void); void sx3_ext_frame_complete(void); void sx3_ext_state_restored(void);
#endif
