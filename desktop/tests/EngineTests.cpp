// SPDX-License-Identifier: GPL-3.0-only
#include "Engine.h"
#include "PortableArm.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <new>
#include <thread>
static bool watch=false;static size_t allocations=0;
void* operator new(std::size_t n){if(watch)++allocations;if(auto* p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}void operator delete(void* p,std::size_t) noexcept{std::free(p);}
static void check(bool ok,const char* msg){if(!ok){std::cerr<<"FAIL: "<<msg<<'\n';std::exit(1);}}
int main(){
    pf::Step s;s.on=true;s.flam=5;s.shift=-7;s.lock1=234;s.value1=90;check(pf::Step::unpack(s.pack()).pack()==s.pack(),"step roundtrip");
    check(float(pf::Half(1.f))==1.f&&float(pf::Half(-0.5f))==-0.5f,"half storage");check(pf::__QADD16(0x7fff7fff,0x00010001)==0x7fff7fff,"ARM saturating lanes");
    auto seq=std::make_unique<pf::Sequence>();seq->euclidean(0,0,16,5,0);int count=0;for(int i=0;i<16;++i)count+=pf::Step::unpack(seq->bank[0].data[0][i]).on;check(count==5,"Euclidean density");
    for(auto& c:seq->bank[0].data[0])c=pf::Step{}.pack();
    pf::Engine e(*seq);e.prepare(48000);e.beginBlock({false,false,0,120},false,0);
    for(int t=0;t<7;++t){e.panic();e.hit(t);double energy=0,leak=0;for(int i=0;i<24000;++i){auto a=e.nextSample();for(auto v:a)check(std::isfinite(v),"finite output");for(int c=2;c<14;++c){if((c-2)/2==std::min(t,5))energy+=a[c]*a[c];else leak+=a[c]*a[c];}}check(energy>0.001,"each original voice produces sound");check(leak<1e-7,"stem isolation after panic");}
    // One four-on-floor bar; random probability is disabled by 100%.
    for(int i=0;i<16;i+=4){pf::Step hit;hit.on=true;seq->bank[0].data[0][i]=hit.pack();}
    e.prepare(48000);e.beginBlock({true,true,0,120},true,0);
    watch=true;for(int i=0;i<96000;++i)e.nextSample();watch=false;
    check(allocations==0,"no allocations during rendering");check(e.triggerCount(0)==4,"four triggers at 120 BPM over one bar");
    // Repeat count and negative microtiming survive a bar boundary.
    seq->bank[0].data[0][0]=pf::Step{}.pack();pf::Step repeated;repeated.on=true;repeated.flam=5;seq->bank[0].data[0][4]=repeated.pack();
    e.prepare(48000);e.beginBlock({true,true,0,120},true,0);for(int i=0;i<48000;++i)e.nextSample();check(e.triggerCount(0)==8,"eight flams in the second beat");
    repeated.probability=0;seq->bank[0].data[0][4]=repeated.pack();e.prepare(48000);e.beginBlock({true,true,0,120},true,0);for(int i=0;i<48000;++i)e.nextSample();check(e.triggerCount(0)==0,"zero probability never triggers");
    for(auto& cell:seq->bank[0].data[0])cell=pf::Step{}.pack();pf::Step h;h.on=true;seq->bank[0].data[0][0]=h.pack();
    e.prepare(48000);e.beginBlock({true,true,0,120},true,0);e.hit(0,110);for(int i=0;i<100;++i)e.nextSample();check(e.triggerCount(0)==1,"MIDI supersedes simultaneous pattern hit");
    auto otherSeq=std::make_unique<pf::Sequence>();pf::Engine other(*otherSeq);other.prepare(48000);other.beginBlock({false,false,0,120},false,0);double silent=0;for(int i=0;i<1000;++i){e.nextSample();auto a=other.nextSample();for(auto v:a)silent+=std::abs(v);}check(silent==0,"multiple engine instances remain independent");
    // Sample rates and effects: exercise the original algorithms at safe settings.
    for(double sr:{44100.,48000.,96000.})for(int fx=0;fx<=4;++fx){other.prepare(sr);other.setParameter(pf::PAR_FX_TYPE,fx);other.setParameter(pf::PAR_FX_DRY_WET,50);other.setParameter(pf::PAR_FX_P1,64);other.setParameter(pf::PAR_FX_P2,64);other.setParameter(pf::PAR_FX_P3,64);other.setParameter(pf::PAR_FX_P4,64);other.beginBlock({false,false,0,120},false,0);other.hit(0);double energy=0;for(int i=0;i<int(sr/2);++i){auto a=other.nextSample();check(std::isfinite(a[0])&&std::abs(a[0])<=1.01f,"finite bounded FX master");energy+=a[0]*a[0];}check(energy>0.0001,"each FX renders nonzero master");}
    std::cout<<"PASS: voices, stems, sequencing, probability, flams, MIDI priority, instance isolation, sample rates, FX, allocation guard\n";
}
