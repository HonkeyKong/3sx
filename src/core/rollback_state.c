#include "core/rollback_state.h"
#include "sf33rd/Source/Game/engine/workuser.h"
#include "sf33rd/Source/Game/system/work_sys.h"
#include <string.h>

static void clean_work_pointers(WORK* work) {
    work->target_adrs=NULL; work->hit_adrs=NULL; work->dmg_adrs=NULL; work->suzi_offset=NULL;
    memset(work->char_table,0,sizeof(work->char_table)); work->se_random_table=NULL;
    work->step_xy_table=NULL; work->move_xy_table=NULL; work->overlap_char_tbl=NULL;
    work->olc_ix_table=NULL; work->rival_catch_tbl=NULL; work->curr_rca=NULL;
    work->set_char_ad=NULL; work->hit_ix_table=NULL; work->body_adrs=NULL; work->h_bod=NULL;
    work->hand_adrs=NULL; work->h_han=NULL; work->dumm_adrs=NULL; work->h_dumm=NULL;
    work->catch_adrs=NULL; work->h_cat=NULL; work->caught_adrs=NULL; work->h_cau=NULL;
    work->attack_adrs=NULL; work->h_att=NULL; work->h_eat=NULL; work->hosei_adrs=NULL;
    work->h_hos=NULL; work->att_ix_table=NULL; work->my_effadrs=NULL;
    work->current_colcd=0; work->colcd=0; work->extra_col=0; work->extra_col_2=0;
}

static void canonicalize(RollbackState* state) {
    GameState* game=&state->payload.game; EffectState* effects=&state->payload.effects;
    for(int i=0;i<2;i++){
        clean_work_pointers(&game->plw[i].wu); game->plw[i].cp=NULL; game->plw[i].dm_step_tbl=NULL;
        game->plw[i].as=NULL; game->plw[i].sa=NULL; game->plw[i].cb=NULL; game->plw[i].py=NULL; game->plw[i].rp=NULL;
        for(int j=0;j<56;j++)game->waza_work[i][j].w_ptr=NULL;
        game->spg_dat[i].spgtbl_ptr=NULL; game->spg_dat[i].spgptbl_ptr=NULL;
    }
    for(int i=0;i<EFFECT_MAX;i++){WORK*work=(WORK*)effects->frw[i];clean_work_pointers(work);((WORK_Other*)effects->frw[i])->my_master=NULL;}
    for(size_t i=0;i<sizeof(game->bg_w.bgw)/sizeof(game->bg_w.bgw[0]);i++){
        game->bg_w.bgw[i].bg_address=NULL; game->bg_w.bgw[i].suzi_adrs=NULL; game->bg_w.bgw[i].start_suzi=NULL;
        game->bg_w.bgw[i].suzi_adrs2=NULL; game->bg_w.bgw[i].start_suzi2=NULL; game->bg_w.bgw[i].deff_rl=NULL;
        game->bg_w.bgw[i].deff_plus=NULL; game->bg_w.bgw[i].deff_minus=NULL;
    }
    game->ci_pointer=NULL; for(size_t i=0;i<sizeof(game->task)/sizeof(game->task[0]);i++)game->task[i].func_adrs=NULL;
    for(size_t i=0;i<sizeof(game->Demo_Ptr)/sizeof(game->Demo_Ptr[0]);i++)game->Demo_Ptr[i]=NULL;
}

void RollbackState_Save(RollbackState* state) {
    memset(state,0,sizeof(*state)); state->magic=ROLLBACK_STATE_MAGIC; state->version=ROLLBACK_STATE_VERSION; state->total_size=sizeof(*state);
    state->reserved=ROLLBACK_STATE_FULL;
    state->payload.interrupt_timer=Interrupt_Timer; InputHistory_Save(&state->payload.input_history); GameState_Save(&state->payload.game);
    EffectState* e=&state->payload.effects; memcpy(e->frw,frw,sizeof(frw)); memcpy(e->exec_tm,exec_tm,sizeof(exec_tm));
    memcpy(e->frwque,frwque,sizeof(frwque)); memcpy(e->head_ix,head_ix,sizeof(head_ix)); memcpy(e->tail_ix,tail_ix,sizeof(tail_ix));
    e->frwctr=frwctr; e->frwctr_min=frwctr_min;
}

void RollbackState_SaveInputOnly(RollbackState* state) {
    memset(state,0,sizeof(*state)); state->magic=ROLLBACK_STATE_MAGIC; state->version=ROLLBACK_STATE_VERSION; state->total_size=sizeof(*state);
    state->reserved=ROLLBACK_STATE_INPUT_ONLY; state->payload.interrupt_timer=Interrupt_Timer; InputHistory_Save(&state->payload.input_history);
}

RollbackStateKind RollbackState_GetKind(const RollbackState* state,size_t size) {
    if(!state||size!=sizeof(*state)||state->magic!=ROLLBACK_STATE_MAGIC||state->version!=ROLLBACK_STATE_VERSION||state->total_size!=sizeof(*state))return 0;
    if(state->reserved!=ROLLBACK_STATE_INPUT_ONLY&&state->reserved!=ROLLBACK_STATE_FULL)return 0;
    return (RollbackStateKind)state->reserved;
}

bool RollbackState_Load(const RollbackState* state,size_t size) {
    const RollbackStateKind kind=RollbackState_GetKind(state,size); if(!kind)return false;
    Interrupt_Timer=state->payload.interrupt_timer; InputHistory_Load(&state->payload.input_history); if(kind==ROLLBACK_STATE_INPUT_ONLY)return true;
    GameState_Load(&state->payload.game); const EffectState*e=&state->payload.effects;
    memcpy(frw,e->frw,sizeof(frw)); memcpy(exec_tm,e->exec_tm,sizeof(exec_tm)); memcpy(frwque,e->frwque,sizeof(frwque));
    memcpy(head_ix,e->head_ix,sizeof(head_ix)); memcpy(tail_ix,e->tail_ix,sizeof(tail_ix)); frwctr=e->frwctr; frwctr_min=e->frwctr_min;
    return true;
}

uint32_t RollbackState_Hash(const RollbackState* state,size_t size) {
    if(!RollbackState_GetKind(state,size))return 0;
    RollbackState copy; memcpy(&copy,state,sizeof(copy)); canonicalize(&copy);
    const uint8_t*p=(const uint8_t*)&copy; uint32_t hash=2166136261u; for(size_t i=0;i<sizeof(copy);i++){hash^=p[i];hash*=16777619u;} return hash;
}
