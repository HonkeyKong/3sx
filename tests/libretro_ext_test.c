#include "platform/app/libretro/libretro_ext.h"
#include "main.h"
#include "sf33rd/Source/Game/engine/plcnt.h"
#include "sf33rd/Source/Game/engine/workuser.h"
#include "sf33rd/Source/Game/system/work_sys.h"
#include <assert.h>
#include <limits.h>
#include <string.h>

MPP mpp_w;
PLW plw[2];
struct _SAVE_W save_w[6];
PresentMode Present_Mode;
u8 Play_Type,PL_Wins[2],G_No[4],C_No[4],My_char[2],Conclusion_Flag,Continue_Coin[2];
u32 Score[2][3];
s8 round_timer;

int main(void){
 const struct libretro_ext_api*a=libretro_ext_get_api();
 assert(a==libretro_ext_get_api_v5());assert(a->abi_version==5);assert(a->sizeof_struct==sizeof(*a));
 assert(libretro_ext_get_api_abi_version()==5);assert(libretro_ext_get_api_struct_size()==sizeof(*a));
 assert(a->get_cpu_count()==0);assert(a->get_cpu_tag(0)==NULL);assert(a->read_u32(":maincpu","program",0)==0);
 Play_Type=0;Present_Mode=PRESENT_MODE_LOCAL;save_w[Present_Mode].Battle_Number[0]=1;mpp_w.inGame=true;G_No[1]=2;G_No[2]=1;
 Score[0][0]=0x12345678;Score[1][0]=900;PL_Wins[0]=1;PL_Wins[1]=0;My_char[0]=4;My_char[1]=7;plw[0].wu.vital_new=160;plw[1].wu.vital_new=1;round_timer=99;
 sx3_ext_game_loaded();assert(a->get_cpu_count()==1);assert(!strcmp(a->get_cpu_tag(0),":maincpu"));assert(a->get_cpu_tag(1)==NULL);
 assert(a->read_u32(":maincpu","program",0)==1);assert(a->read_u32(":maincpu","program",4)==0x50);assert(a->read_u32(":maincpu","program",0x14)==0x12345678);
 assert(a->read_u8(":maincpu","program",0x1c)==1);assert(a->read_u16(":maincpu","program",0x1c)==1);assert(a->read_u16(":maincpu","program",0x20)==160);
 assert(a->read_u8(":maincpu","program",0x4f)==0);assert(a->read_u16(":maincpu","program",0x4f)==0);assert(a->read_u32(":maincpu","program",0x4d)==0);assert(a->read_u32(":maincpu","program",UINT64_MAX)==0);
 assert(a->read_u8(":maincpu","data",0x1c)==0);assert(a->read_u8(":bad","program",0x1c)==0);
 a->write_u8(":maincpu","program",0x1c,9);a->write_u32(":maincpu","program",0x14,0);assert(a->read_u8(":maincpu","program",0x1c)==1);assert(a->read_u32(":maincpu","program",0x14)==0x12345678);
 PL_Wins[1]=1;Score[1][0]=901;sx3_ext_frame_complete();assert(a->get_frame_number()==1);assert(a->read_u8(":maincpu","program",0x1d)==1);assert(a->read_u32(":maincpu","program",0x18)==901);
 PL_Wins[1]=0;sx3_ext_state_restored();assert(a->read_u8(":maincpu","program",0x1d)==0);
 sx3_ext_game_unloaded();assert(a->get_cpu_count()==0);assert(a->get_cpu_tag(0)==NULL);assert(a->read_u8(":maincpu","program",0x1c)==0);
 return 0;
}
