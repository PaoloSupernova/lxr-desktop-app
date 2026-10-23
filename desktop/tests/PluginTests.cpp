// SPDX-License-Identifier: GPL-3.0-only
#include "Processor.h"
#include <iostream>
#include <cstdlib>
#include <new>
static thread_local bool watchAllocations=false;static size_t renderAllocations=0;
void* operator new(std::size_t n){if(watchAllocations)++renderAllocations;if(auto* p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
static void check(bool ok,const char* msg){if(!ok){std::cerr<<"FAIL: "<<msg<<'\n';std::exit(1);}}
class PlayHead:public juce::AudioPlayHead{public:double ppq=0;bool running=true;juce::Optional<PositionInfo> getPosition()const override{PositionInfo p;p.setBpm(120);p.setPpqPosition(ppq);p.setIsPlaying(running);return p;}};
int main(){juce::ScopedJuceInitialiser_GUI gui;auto p=std::make_unique<PulseProcessor>();check(p->enableAllBuses(),"enable seven output buses");p->prepareToPlay(48000,256);p->sequence.selected=13;pf::Step s;s.on=true;s.flam=3;s.shift=-2;s.lock1=pf::PAR_VOL1;s.value1=99;p->sequence.bank[13].data[2][31]=s.pack();p->sequence.bank[13].length[2]=47;p->parameter(pf::PAR_COARSE1,44);p->sequence.songEnabled=1;p->sequence.songLength=3;p->sequence.song[1]=13;juce::MemoryBlock saved;p->getStateInformation(saved);auto q=std::make_unique<PulseProcessor>();q->setStateInformation(saved.getData(),int(saved.getSize()));check(q->sequence.selected==13&&q->sequence.bank[13].data[2][31]==s.pack()&&q->sequence.bank[13].length[2]==47,"complete sequence state roundtrip");check(q->parameter(pf::PAR_COARSE1)==44,"sound state roundtrip");check(q->sequence.songLength==3&&q->sequence.song[1]==13,"song state roundtrip");juce::String err;const char bad[]={1,2,3};check(!q->restore(bad,3,false,err),"reject corrupted state");check(q->parameter(pf::PAR_COARSE1)==44,"rejected state leaves sound unchanged");
    p->sequence.songEnabled=0;PlayHead ph;p->setPlayHead(&ph);p->state.getParameter("mode")->setValueNotifyingHost(1);juce::AudioBuffer<float> audio(14,256);juce::MidiBuffer midi;midi.addEvent(juce::MidiMessage::noteOn(1,36,juce::uint8(110)),100);double energy=0;for(int b=0;b<90;++b){p->processBlock(audio,midi);for(int i=0;i<256;++i){for(int c=0;c<14;++c)check(std::isfinite(audio.getSample(c,i)),"finite plugin audio");energy+=std::abs(audio.getSample(2,i));for(int c=4;c<14;++c)check(audio.getSample(c,i)==0,"isolated enabled stems");}ph.ppq+=256.0/24000;}check(energy>0,"MIDI creates routed audio");check(p->engine.triggerCount(0)==1,"MIDI-only disables pattern playback");check(p->engine.triggerCount(3)==0,"MIDI-only silences internal snare pattern");p->setPlayHead(nullptr);
    auto kit=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("PulseFoundry-test",".pfkit",false);
    check(q->saveProject(kit,true,err),"save desktop kit"); q->parameter(pf::PAR_COARSE1,77); q->sequence.selected=7;
    q->state.getParameter("tempo")->setValueNotifyingHost(q->state.getParameter("tempo")->convertTo0to1(151));
    check(q->loadProject(kit,true,err),"load desktop kit");check(q->parameter(pf::PAR_COARSE1)==44&&q->sequence.selected==7,"kit restores sound and preserves sequence");
    check(q->state.getRawParameterValue("tempo")->load()==151,"kit preserves standalone tempo");kit.deleteFile();
    q->sequence.selected=0;q->sequence.songEnabled=0;q->enableAllBuses();q->prepareToPlay(48000,256);ph.ppq=0;ph.running=true;q->setPlayHead(&ph);
    for(int b=0;b<375;++b){q->processBlock(audio,midi);ph.ppq+=256.0/24000;}
    check(q->engine.triggerCount(0)==4,"host PPQ sync produces four-on-floor at 120 BPM");
    auto before=q->engine.triggerCount(0);ph.running=false;
    watchAllocations=true;for(int b=0;b<25;++b)q->processBlock(audio,midi);watchAllocations=false;
    check(renderAllocations==0,"no allocation in complete plugin audio callback");check(q->engine.triggerCount(0)==before,"host stop halts sequencer");
    ph.running=true;ph.ppq=0;q->processBlock(audio,midi);check(q->engine.triggerCount(0)==before+1,"transport seek restarts first step");q->setPlayHead(nullptr);
    std::cout<<"PASS: full plugin state, rejected state, song state, kit isolation, MIDI-only, multi-bus routing, host sync/stop/seek and callback allocation guard\n";}
