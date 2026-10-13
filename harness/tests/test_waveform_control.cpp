#include "emotiscope_control.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <initializer_list>
using namespace k1::emotiscope;
int main(int argc,char** argv){
 assert(argc==2);assert(k1_emotiscope_init(1));assert(k1_secondary_emotiscope_init(2));
 FILE* fp=std::fopen(argv[1],"rb");assert(fp);unsigned char bytes[568]{};const auto size=std::fread(bytes,1,sizeof(bytes),fp);std::fclose(fp);
 DonorIntent parsed{};assert(donorParse(bytes,size,&parsed)==0);assert(parsed.mode==8&&parsed.board==1&&parsed.mask_extra==7);assert(parsed.inhouse_sensitivity==2.4f&&parsed.inhouse_square_iter==1.f&&parsed.inhouse_injection_photons==1.f);
 for(unsigned board=0;board<2;++board)for(int mode:{32,18,8,7,11}){
   DonorDedup dedup{};DonorObserved observed{};observed.boot_id=42;observed.board=board;
   DonorIntent i{};i.sub=kDonorCfgPatchWaveforms;i.boot_id=42;i.request_id=1;i.board=board;i.mask=(1u<<8)|(std::uint64_t{1}<<63);i.mode=mode;i.inhouse_mood=.2f;
   if(mode==8){i.mask_extra=7;i.inhouse_sensitivity=3.2f;i.inhouse_square_iter=2.1f;i.inhouse_injection_photons=.4f;}else if(mode==7||mode==11){i.mask_extra=2;i.inhouse_square_iter=2.1f;}
   K1EmotiscopeConfig peer_before{},peer_after{};donorSceneGetConfig(1-board,&peer_before);
   DonorApplyResult result{};donorApply(&dedup,i,observed,&result);assert(result.status==0&&result.response_bytes==440);assert(donorGetU32(result.response+8)==2&&donorGetU32(result.response+60)==unsigned(mode));assert(result.affected_boards==(1u<<board));
   donorSceneGetConfig(1-board,&peer_after);assert(!std::memcmp(&peer_before,&peer_after,sizeof(peer_before)));
   if(mode==8){assert(result.inhouse_sensitivity_changed&&result.inhouse_square_iter_changed&&result.inhouse_injection_photons_changed);assert(donorGetF32(result.response+428)==3.2f);assert(!(donorGetU32(result.response+72)&(1u<<5)));}
   for(unsigned failure=0;failure<5;++failure){
     DonorDedup fresh{};auto bad=i;bad.request_id=10+failure;auto limited=observed;
     if(failure==0){bad.mode=9;}else if(failure==1){limited.board_capabilities=127;}else if(failure==2){bad.sub=kDonorCfgPatch;bad.mask_extra=0;}else if(failure==3){bad.mask_extra=8;}else {bad.mask_extra=7;bad.inhouse_sensitivity=21;}
     K1EmotiscopeConfig before{},after{};donorSceneGetConfig(board,&before);donorApply(&fresh,bad,limited,&result);assert(result.status==kDonorStatusInvalid&&!result.changed);donorSceneGetConfig(board,&after);assert(!std::memcmp(&before,&after,sizeof(before)));
   }
 }
 for(unsigned board=0;board<2;++board){
   DonorObserved observed{};observed.boot_id=42;observed.board=board;
   DonorIntent select{};select.sub=kDonorCfgPatchWaveforms;select.boot_id=42;select.request_id=100;select.board=board;select.mask=1u<<8;select.mode=18;
   DonorDedup cache{};DonorApplyResult result{};donorApply(&cache,select,observed,&result);assert(result.status==0);
   observed.revision=result.revision;
   auto legacy=select;legacy.sub=kDonorCfgPatchInhouse;legacy.request_id=101;legacy.expected_revision=observed.revision;legacy.mask=1;legacy.brightness=.31f;
   K1EmotiscopeConfig before{},after{};donorSceneGetConfig(board,&before);
   donorApply(&cache,legacy,observed,&result);assert(result.status==kDonorStatusInvalid&&!result.changed);
   donorSceneGetConfig(board,&after);assert(!std::memcmp(&before,&after,sizeof(before)));
   legacy.request_id=102;legacy.mask|=1u<<8;legacy.mode=0;
   donorApply(&cache,legacy,observed,&result);assert(result.status==0&&result.mode==0&&result.response_bytes==428);
   assert(donorGetU32(result.response+164)==255u);
 }
 // Complete saved composition recall includes inactive source parameters.
 for(unsigned board=0;board<2;++board)for(int destination:{0,1,2,3,4,5,6,32,18,7,11}){
   DonorObserved observed{};observed.boot_id=42;observed.board=board;
   DonorIntent edit{};edit.sub=kDonorCfgPatchWaveforms;edit.boot_id=42;edit.request_id=200;edit.board=board;
   edit.mask=(1u<<8)|(std::uint64_t{1}<<63);edit.mask_extra=7;edit.mode=8;
   edit.inhouse_mood=.7f;edit.inhouse_sensitivity=4.f;edit.inhouse_square_iter=3.f;edit.inhouse_injection_photons=.3f;
   DonorDedup cache{};DonorApplyResult result{};
   auto adopt=[&](){observed.revision=result.revision;observed.inhouse_mood=result.inhouse_mood;
     observed.inhouse_sensitivity=result.inhouse_sensitivity;observed.inhouse_square_iter=result.inhouse_square_iter;
     observed.inhouse_injection_photons=result.inhouse_injection_photons;};
   K1EmotiscopeConfig peer_before{},peer_after{};donorSceneGetConfig(1-board,&peer_before);
   donorApply(&cache,edit,observed,&result);assert(result.status==0);adopt();
   auto restore=edit;restore.request_id=201;restore.expected_revision=observed.revision;restore.mode=destination;
   restore.inhouse_mood=.05f;restore.inhouse_sensitivity=2.4f;restore.inhouse_square_iter=1.f;restore.inhouse_injection_photons=1.f;
   donorApply(&cache,restore,observed,&result);
   assert(result.status==0&&result.affected_boards==(1u<<board));
   assert(result.inhouse_mood_changed&&result.inhouse_sensitivity_changed&&result.inhouse_square_iter_changed&&result.inhouse_injection_photons_changed);adopt();
   DonorIntent get{};get.sub=kDonorCfgGetWaveforms;get.boot_id=42;get.request_id=202;get.board=board;
   donorApply(&cache,get,observed,&result);assert(result.status==0);
   assert(donorGetU32(result.response+60)==unsigned(destination));
   assert(donorGetF32(result.response+424)==.05f&&donorGetF32(result.response+428)==2.4f&&donorGetF32(result.response+432)==1.f&&donorGetF32(result.response+436)==1.f);
   for(unsigned field=0;field<4;++field){
     auto bad=restore;bad.request_id=210+field;bad.expected_revision=observed.revision;bad.mask|=1u;bad.brightness=.27f;
     if(field==0)bad.inhouse_mood=NAN;else if(field==1)bad.inhouse_sensitivity=21.f;else if(field==2)bad.inhouse_square_iter=-1.f;else bad.inhouse_injection_photons=INFINITY;
     K1EmotiscopeConfig before{},after{};donorSceneGetConfig(board,&before);
     donorApply(&cache,bad,observed,&result);assert(result.status==kDonorStatusInvalid&&!result.changed);
     assert(!result.inhouse_mood_changed&&!result.inhouse_sensitivity_changed&&!result.inhouse_square_iter_changed&&!result.inhouse_injection_photons_changed);
     donorSceneGetConfig(board,&after);assert(!std::memcmp(&before,&after,sizeof(before)));
   }
   donorSceneGetConfig(1-board,&peer_after);assert(!std::memcmp(&peer_before,&peer_after,sizeof(peer_before)));
 }
 assert(donorResponseBytes(kDonorCfgGetState)==208&&donorResponseBytes(kDonorCfgGetExtended)==424&&donorResponseBytes(kDonorCfgGetInhouse)==428&&donorResponseBytes(kDonorCfgGetWaveforms)==440);
 std::puts("waveform v8 control and both-edge atomic rejection passed");
}
