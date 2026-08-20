#include "platform/app/libretro/libretro_ext.h"
#include "main.h"
#include "sf33rd/Source/Game/engine/plcnt.h"
#include "sf33rd/Source/Game/engine/workuser.h"
#include "sf33rd/Source/Game/system/work_sys.h"
#include "sf33rd/Source/Game/ui/count.h"
#include <stddef.h>
#include <string.h>

#define SX3_EXT_MAINCPU_VERSION 1u
enum sx3_ext_match_state { SX3_EXT_MATCH_NONE, SX3_EXT_MATCH_INTRO, SX3_EXT_MATCH_ACTIVE, SX3_EXT_MATCH_ROUND_END, SX3_EXT_MATCH_MATCH_END, SX3_EXT_MATCH_RESULTS };
typedef struct sx3_ext_maincpu_memory {
 uint32_t version,size,frame_number,game_state,match_state,p1_score,p2_score;
 uint8_t p1_rounds,p2_rounds,p1_character,p2_character;
 uint16_t p1_health,p2_health,timer,reserved0;
 uint32_t raw_game_state,raw_match_state;
 uint8_t reserved[32];
} sx3_ext_maincpu_memory;
#define OFFSET_ASSERT(f,o) _Static_assert(offsetof(sx3_ext_maincpu_memory,f)==o,#f " offset")
OFFSET_ASSERT(version,0x00); OFFSET_ASSERT(size,0x04); OFFSET_ASSERT(frame_number,0x08); OFFSET_ASSERT(game_state,0x0c); OFFSET_ASSERT(match_state,0x10);
OFFSET_ASSERT(p1_score,0x14); OFFSET_ASSERT(p2_score,0x18); OFFSET_ASSERT(p1_rounds,0x1c); OFFSET_ASSERT(p2_rounds,0x1d); OFFSET_ASSERT(p1_character,0x1e); OFFSET_ASSERT(p2_character,0x1f);
OFFSET_ASSERT(p1_health,0x20); OFFSET_ASSERT(p2_health,0x22); OFFSET_ASSERT(timer,0x24); OFFSET_ASSERT(raw_game_state,0x28); OFFSET_ASSERT(raw_match_state,0x2c);
_Static_assert(sizeof(sx3_ext_maincpu_memory)==0x50,"maincpu size");

static bool active,debug_enabled; static uint64_t frame; static uint8_t mem[sizeof(sx3_ext_maincpu_memory)];
static void put16(size_t o,uint16_t v){mem[o]=(uint8_t)v;mem[o+1]=(uint8_t)(v>>8);}
static void put32(size_t o,uint32_t v){mem[o]=(uint8_t)v;mem[o+1]=(uint8_t)(v>>8);mem[o+2]=(uint8_t)(v>>16);mem[o+3]=(uint8_t)(v>>24);}
static uint32_t match_state(void){
 if(!mpp_w.inGame)return SX3_EXT_MATCH_NONE;
 uint8_t needed=(uint8_t)(save_w[Present_Mode].Battle_Number[Play_Type]+1);
 if(PL_Wins[0]>=needed||PL_Wins[1]>=needed)return SX3_EXT_MATCH_MATCH_END;
 if(G_No[1]!=2)return SX3_EXT_MATCH_RESULTS;
 if(G_No[2]==0)return SX3_EXT_MATCH_INTRO;
 if(G_No[2]!=1)return SX3_EXT_MATCH_RESULTS;
 return Conclusion_Flag?SX3_EXT_MATCH_ROUND_END:SX3_EXT_MATCH_ACTIVE;
}
static void update(void){
 memset(mem,0,sizeof(mem)); if(!active)return;
 put32(0x00,SX3_EXT_MAINCPU_VERSION);put32(0x04,sizeof(mem));put32(0x08,(uint32_t)frame);put32(0x0c,mpp_w.inGame?1:0);put32(0x10,match_state());
 put32(0x14,Score[0][Play_Type]+Continue_Coin[0]);put32(0x18,Score[1][Play_Type]+Continue_Coin[1]);mem[0x1c]=PL_Wins[0];mem[0x1d]=PL_Wins[1];mem[0x1e]=My_char[0];mem[0x1f]=My_char[1];
 put16(0x20,(uint16_t)(plw[0].wu.vital_new>0?plw[0].wu.vital_new:0));put16(0x22,(uint16_t)(plw[1].wu.vital_new>0?plw[1].wu.vital_new:0));put16(0x24,(uint8_t)round_timer);
 put32(0x28,((uint32_t)G_No[0]<<24)|((uint32_t)G_No[1]<<16)|((uint32_t)G_No[2]<<8)|G_No[3]);put32(0x2c,((uint32_t)C_No[0]<<24)|((uint32_t)C_No[1]<<16)|((uint32_t)C_No[2]<<8)|C_No[3]);
}
void sx3_ext_game_loaded(void){active=true;frame=0;update();} void sx3_ext_game_unloaded(void){active=false;frame=0;memset(mem,0,sizeof(mem));}
void sx3_ext_frame_complete(void){if(active){++frame;update();}} void sx3_ext_state_restored(void){if(active)update();}
static bool bus(const char*c,const char*s){return active&&c&&s&&!strcmp(c,":maincpu")&&!strcmp(s,"program");}
static bool range(uint64_t a,size_t w){return a<=sizeof(mem)&&w<=sizeof(mem)-(size_t)a;}
static uint8_t r8(const char*c,const char*s,uint64_t a){return bus(c,s)&&range(a,1)?mem[a]:0;}
static uint16_t r16(const char*c,const char*s,uint64_t a){if(!bus(c,s)||!range(a,2))return 0;return (uint16_t)(mem[a]|((uint16_t)mem[a+1]<<8));}
static uint32_t r32(const char*c,const char*s,uint64_t a){if(!bus(c,s)||!range(a,4))return 0;return (uint32_t)mem[a]|((uint32_t)mem[a+1]<<8)|((uint32_t)mem[a+2]<<16)|((uint32_t)mem[a+3]<<24);}
static void w8(const char*c,const char*s,uint64_t a,uint8_t v){(void)c;(void)s;(void)a;(void)v;} static void w16(const char*c,const char*s,uint64_t a,uint16_t v){(void)c;(void)s;(void)a;(void)v;} static void w32(const char*c,const char*s,uint64_t a,uint32_t v){(void)c;(void)s;(void)a;(void)v;}
static const char*driver(void){return active?"SF33RD":NULL;} static int cpus(void){return active?1:0;} static const char*cpu(int i){return active&&i==0?":maincpu":NULL;} static uint64_t pc(int i){(void)i;return 0;} static uint64_t pct(const char*t){(void)t;return 0;} static uint64_t frame_no(void){return active?frame:0;}
static int zero_count(void){return 0;} static const char*null_tag(int i){(void)i;return NULL;} static uint64_t zero(void){return 0;} static uint64_t zero_tag(const char*t){(void)t;return 0;} static uint64_t zero_size(const char*t){(void)t;return 0;}
static uint64_t region_r(const char*t,uint64_t o,void*d,uint64_t s){(void)t;(void)o;(void)d;(void)s;return 0;} static uint64_t region_w(const char*t,uint64_t o,const void*d,uint64_t s){(void)t;(void)o;(void)d;(void)s;return 0;}
static void clear(void){} static void addwatch(const char*t,uint64_t s,uint64_t e,uint8_t a,uint8_t w){(void)t;(void)s;(void)e;(void)a;(void)w;} static bool gethit(struct libretro_ext_watch_hit*h){if(h)memset(h,0,sizeof(*h));return false;} static void checkhit(const char*t,uint64_t a,uint64_t p,uint32_t v,uint8_t x,uint8_t w,uint64_t c){(void)t;(void)a;(void)p;(void)v;(void)x;(void)w;(void)c;}
static bool dipinfo(int i,struct libretro_ext_dip_info*d){(void)i;(void)d;return false;} static bool dipset(int i,uint32_t v){(void)i;(void)v;return false;} static void setdebug(bool v){debug_enabled=v;} static bool getdebug(void){return debug_enabled;} static bool no(void){return false;}
static bool statei(int i,int s,uint64_t*v){(void)i;(void)s;if(v)*v=0;return false;} static bool statet(const char*t,int s,uint64_t*v){(void)t;(void)s;if(v)*v=0;return false;} static bool reg(const char*t,const char*r,uint64_t*v){(void)t;(void)r;if(v)*v=0;return false;} static bool statewi(int i,int s,uint64_t v){(void)i;(void)s;(void)v;return false;} static bool statewt(const char*t,int s,uint64_t v){(void)t;(void)s;(void)v;return false;} static bool regw(const char*t,const char*r,uint64_t v){(void)t;(void)r;(void)v;return false;}
static const struct libretro_ext_api api={
 .abi_version=5,.sizeof_struct=sizeof(struct libretro_ext_api),.get_driver_name=driver,.get_cpu_count=cpus,.get_cpu_tag=cpu,.get_cpu_pc=pc,.read_u8=r8,.read_u16=r16,.read_u32=r32,.write_u8=w8,.write_u16=w16,.write_u32=w32,
 .get_region_count=zero_count,.get_region_tag=null_tag,.get_region_size=zero_size,.read_region=region_r,.write_region=region_w,.get_frame_number=frame_no,.get_time_attoseconds=zero,.get_cpu_total_cycles_by_tag=zero_tag,
 .clear_watch_rules=clear,.add_watch_rule=addwatch,.get_last_watch_hit=gethit,.clear_last_watch_hit=clear,.check_watch_hit=checkhit,.get_dip_count=zero_count,.get_dip_info=dipinfo,.set_dip_value=dipset,.set_debug_extensions_enabled=setdebug,.get_debug_extensions_enabled=getdebug,.trigger_timing_capture=no,.get_cpu_pc_by_tag=pct,
 .read_cpu_state_u64_by_index=statei,.read_cpu_state_u64_by_tag=statet,.read_cpu_register_by_tag=reg,.write_cpu_state_u64_by_index=statewi,.write_cpu_state_u64_by_tag=statewt,.write_cpu_register_by_tag=regw
};
LIBRETRO_EXT_EXPORT const struct libretro_ext_api*libretro_ext_get_api(void){return &api;} LIBRETRO_EXT_EXPORT const struct libretro_ext_api*libretro_ext_get_api_v5(void){return &api;} LIBRETRO_EXT_EXPORT uint32_t libretro_ext_get_api_abi_version(void){return 5;} LIBRETRO_EXT_EXPORT uint32_t libretro_ext_get_api_struct_size(void){return sizeof(api);}
