// SPDX-License-Identifier: GPL-3.0-only
#include "Engine.h"
#include "Firmware.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace pf {
const std::vector<ParameterSpec>& parameterSpecs() {
    static const std::vector<ParameterSpec> specs{
#include "ParameterSpecs.inc"
    };
    return specs;
}
int safeParameter(int id,int v) { for(const auto& s:parameterSpecs()) if(s.id==id) return std::clamp(v,0,s.maximum); return 0; }
uint64_t Step::pack() const {
    uint64_t x=on?1:0; int b=1;
    auto add=[&](int v,int n,int max) { x |= uint64_t(std::clamp(v,0,max))<<b; b+=n; };
    add(velocity,7,127); add(probability,7,100); add(note,7,127); add(flam,3,5); add(shift+7,4,14);
    add(lock1,8,245); add(value1,7,127); add(lock2,8,245); add(value2,7,127); return x;
}
Step Step::unpack(uint64_t x) {
    Step s; s.on=(x&1)!=0; x>>=1;
    auto get=[&](int n) { int v=int(x&((uint64_t(1)<<n)-1)); x>>=n; return v; };
    s.velocity=get(7); s.probability=std::min(100,get(7)); s.note=get(7); s.flam=std::min(5,get(3)); s.shift=std::min(14,get(4))-7;
    s.lock1=get(8); s.value1=get(7); s.lock2=get(8); s.value2=get(7); return s;
}
Pattern::Pattern() { for(int t=0;t<tracks;++t) { length[t]=16; rotation[t]=0; mute[t]=0; for(auto& s:data[t]) s=Step{}.pack(); } }
Sequence::Sequence() { for(auto& s:song) s=0; for(auto& h:trackHead) h=-1; }
void Sequence::euclidean(int p,int t,int len,int hits,int rot) {
    if(p<0||p>=patterns||t<0||t>=tracks) return;
    len=std::clamp(len,1,64); hits=std::clamp(hits,0,len); rot=(rot%len+len)%len;
    bank[p].length[t]=len;
    for(int i=0;i<64;++i) { Step s; s.on=i<len && (((i-rot+len)%len)*hits%len<hits); bank[p].data[t][i]=s.pack(); }
}
struct Engine::Impl { Firmware fw{}; std::array<bool,512> locked{}; int64_t lockStep=std::numeric_limits<int64_t>::min(); };
Engine::Engine(Sequence& s):impl(std::make_unique<Impl>()),sequence(s) {
    parameters.fill(0); for(const auto& p:parameterSpecs()) parameters[p.id]=p.initial;
    lastPulse.fill(std::numeric_limits<int64_t>::min());
    prepare(48000);
}
Engine::~Engine()=default;
void Engine::prepare(double sr) {
    rate=std::clamp(sr,8000.0,384000.0); *impl=Impl{};
    auto& f=impl->fw;
    f.parameterArray_init(); f.initDrumVoice(); f.Snare_init(); f.Cymbal_init(); f.HiHat_init(); f.FxBus_init();
    for(auto& v:f.velocityModulators) f.modNode_init(&v);
    for(const auto& p:parameterSpecs()) setParameter(p.id,parameters[p.id]);
    cursor=64; phase=0; primed=false; ppq=0; running=false; wasRunning=false;
    hasMidi.fill(false); triggerCounts.fill(0); lastPulse.fill(std::numeric_limits<int64_t>::min());
}
void Engine::setParameter(int id,int v) {
    if(id<=0||id>=END_OF_SOUND_PARAMETERS) return;
    // Parameter ranges are validated by the caller; validate again at the boundary.
    v=safeParameter(id,v); parameters[id]=v;
    auto& f=impl->fw; f.activeParameters[id]=uint8_t(v);
    Firmware::MidiMsg m{}; m.status=uint8_t(id<128?MIDI_CC:MIDI_CC2); m.data1=uint8_t(id<128?((id+1)&127):(id-128)); m.data2=uint8_t(v);
    f.midiParser_ccHandler(m,0,0,0);
    // The hardware front panel translates a target menu index. Desktop projects
    // store stable parameter IDs and assign the original nodes directly.
    int destination=v;
    if(destination>=END_OF_SOUND_PARAMETERS || destination<0 ||
       (destination>=PAR_VEL_DEST_1 && destination<=PAR_VEL_DEST_6) ||
       (destination>=PAR_TARGET_LFO1 && destination<=PAR_TARGET_LFO6)) destination=0;
    if(id>=PAR_VEL_DEST_1 && id<=PAR_VEL_DEST_6)
        f.modNode_setDestination(&f.velocityModulators[id-PAR_VEL_DEST_1],uint16_t(destination));
    if(id>=PAR_TARGET_LFO1 && id<=PAR_TARGET_LFO6) {
        Firmware::Lfo* nodes[]={&f.voiceArray[0].lfo,&f.voiceArray[1].lfo,&f.voiceArray[2].lfo,&f.snareVoice.lfo,&f.cymbalVoice.lfo,&f.hatVoice.lfo};
        f.modNode_setDestination(&nodes[id-PAR_TARGET_LFO1]->modTarget,uint16_t(destination));
    }
    f.fxBus.audioRouting=Firmware::MIXER_ROUTING_DAC1_STEREO;
}
void Engine::beginBlock(Transport t,bool enabled,float swing) {
    const bool seek=t.positioned && std::abs(ppq-t.ppq)>std::max(0.02,4.0*increment);
    if(t.positioned) ppq=t.ppq;
    if((t.playing&&!wasRunning)||seek) lastPulse.fill(std::numeric_limits<int64_t>::min());
    wasRunning=t.playing; running=t.playing; patternMode=enabled;
    increment=std::clamp(t.bpm,20.0,400.0)/(60.0*rate); shuffle=std::clamp(swing,0.f,0.49f);
    const auto bpm=uint16_t(std::clamp(t.bpm,20.0,400.0));
    if(impl->fw.tempo!=bpm) { impl->fw.tempo=bpm; impl->fw.lfo_recalcSync(); }
}
void Engine::hit(int t,int v,int n) { if(t>=0&&t<tracks&&v>0) { hasMidi[t]=true; pendingMidi[t]={t,std::clamp(v,1,127),std::clamp(n,0,127)}; } }
void Engine::panic() {
    auto& f=impl->fw;
    for(auto& v:f.voiceArray) { v.oscVolEg.state=EG_STOPPED; v.ampFilterInput=0; v.lastGain=0; }
    f.snareVoice.oscVolEg.state=EG_STOPPED; f.cymbalVoice.oscVolEg.state=EG_STOPPED; f.hatVoice.oscVolEg.state=EG_STOPPED;
    f.snareVoice.egValueOscVol=0; f.cymbalVoice.egValueOscVol=0; f.hatVoice.egValueOscVol=0;
    std::fill(std::begin(f.FX_BUFFER),std::end(f.FX_BUFFER),Half{});
    audio={}; left={}; right={}; hasMidi.fill(false);
}
void Engine::trigger(int t,int v,int n) {
    auto& f=impl->fw; ++triggerCounts[size_t(t)];
    if(t<3) f.Drum_trigger(uint8_t(t),uint8_t(v),uint8_t(n));
    else if(t==3) f.Snare_trigger(uint8_t(v),uint8_t(n));
    else if(t==4) f.Cymbal_trigger(uint8_t(v),uint8_t(n));
    else f.HiHat_trigger(uint8_t(v),uint8_t(t==6),uint8_t(n));
}
void Engine::renderNativeBlock() {
    auto& f=impl->fw;
    std::array<bool,6> midiVoice{};
    for(int t=0;t<tracks;++t) if(hasMidi[t]) midiVoice[size_t(std::min(t,5))]=true;
    currentPattern=std::clamp(sequence.selected.load(),0,63);
    if(sequence.songEnabled.load()) {
        const int count=std::clamp(sequence.songLength.load(),1,64);
        // Each song entry spans the longest track, rounded up to complete bars.
        int songBar=int(std::floor(std::max(0.0,ppq)/4));
        int totalBars=0; std::array<int,64> bars{};
        for(int i=0;i<count;++i) { const int p=std::clamp(sequence.song[i].load(),0,63); int longest=1; for(auto& l:sequence.bank[p].length) longest=std::max(longest,l.load()); bars[i]=(longest+15)/16; totalBars+=bars[i]; }
        int at=songBar%std::max(1,totalBars);
        for(int i=0;i<count;++i) { if(at<bars[i]) { currentPattern=std::clamp(sequence.song[i].load(),0,63); break; } at-=bars[i]; }
    }
    sequence.soundingPattern=currentPattern;
    auto& pat=sequence.bank[currentPattern];
    const double stepPos=ppq*4.0;
    const int64_t base=int64_t(std::floor(stepPos));
    if(base!=impl->lockStep || !running) {
        for(int id=1;id<END_OF_SOUND_PARAMETERS;++id) if(impl->locked[id]) { setParameter(id,parameters[id]); impl->locked[id]=false; }
        impl->lockStep=base;
    }
    sequence.playhead=running&&patternMode?int((base%64+64)%64):-1;
    for(int t=0;t<tracks;++t) {
        const int len=std::clamp(pat.length[t].load(),1,64);
        sequence.trackHead[t]=running&&patternMode ? int(((base+pat.rotation[t].load())%len+len)%len) : -1;
    }
    // Native 64-frame scheduling window. A hit is quantised to the first native
    // block at/after its due time (0..1.45 ms), independent of host block size.
    const double width=64.0/nativeRate*increment*rate*4.0;
    if(running&&patternMode) for(int t=0;t<tracks;++t) {
        if(pat.mute[t].load()) continue;
        const int len=std::clamp(pat.length[t].load(),1,64), rot=pat.rotation[t].load();
        for(int64_t logical=base-1;logical<=base+1;++logical) {
            int ix=int(((logical+rot)%len+len)%len);
            Step s=Step::unpack(pat.data[t][ix].load()); if(!s.on) continue;
            static constexpr int repeats[]={1,2,3,4,6,8};
            int count=repeats[s.flam];
            for(int r=0;r<count;++r) {
                const double due=double(logical)+((logical&1)?shuffle:0.f)+s.shift/24.0+double(r)/count;
                const int64_t key=logical*8+r;
                if(due>stepPos+1.0e-8||due<=stepPos-width-1.0e-8||key==lastPulse[t]) continue;
                lastPulse[t]=key;
                // Stable probability across all flams belonging to one step.
                uint32_t h=uint32_t(logical)^uint32_t(t*0x9e3779b9u)^uint32_t(currentPattern*7919);
                h^=h>>16; h*=0x7feb352du; h^=h>>15;
                if(int(h%100)>=s.probability) continue;
                if(s.lock1>0&&s.lock1<END_OF_SOUND_PARAMETERS) { auto saved=parameters[s.lock1]; setParameter(s.lock1,s.value1); parameters[s.lock1]=saved; impl->locked[s.lock1]=true; }
                if(s.lock2>0&&s.lock2<END_OF_SOUND_PARAMETERS) { auto saved=parameters[s.lock2]; setParameter(s.lock2,s.value2); parameters[s.lock2]=saved; impl->locked[s.lock2]=true; }
                if(!midiVoice[size_t(std::min(t,5))]) trigger(t,s.velocity,s.note);
            }
        }
    }
    // Open hat first; closed hat takes priority if both arrive together.
    for(int t=6;t>=0;--t) if(hasMidi[t]) { auto h=pendingMidi[t]; trigger(h.track,h.velocity,h.note); }
    hasMidi.fill(false);
    f.modNode_resetTargets(); f.modNode_reassignVeloMod();
    for(auto& v:f.voiceArray) f.lfo_dispatchNextValue(&v.lfo);
    f.lfo_dispatchNextValue(&f.snareVoice.lfo); f.lfo_dispatchNextValue(&f.cymbalVoice.lfo); f.lfo_dispatchNextValue(&f.hatVoice.lfo);
    for(int t=0;t<3;++t) f.calcDrumVoiceAsync(uint8_t(t));
    f.Snare_calcAsync(); f.Cymbal_calcAsync(); f.HiHat_calcAsync();
    alignas(8) int16_t mono[64]{}, fx[128]{}, out[128]{}, other[128]{};
    audio={};
    for(int t=0;t<6;++t) {
        if(t<3) f.calcDrumVoiceSyncBlock(uint8_t(t),mono,64);
        else if(t==3) f.Snare_calcSyncBlock(mono,64);
        else if(t==4) f.Cymbal_calcSyncBlock(mono,64);
        else f.HiHat_calcSyncBlock(mono,64);
        const float panL=t<3?f.voiceArray[t].triggeredPanL:t==3?f.snareVoice.triggeredPanL:t==4?f.cymbalVoice.triggeredPanL:f.hatVoice.triggeredPanL;
        const float panR=t<3?f.voiceArray[t].triggeredPanR:t==3?f.snareVoice.triggeredPanR:t==4?f.cymbalVoice.triggeredPanR:f.hatVoice.triggeredPanR;
        for(int i=0;i<64;++i) {
            f.mixer_decimation_cnt[t]+=f.mixer_decimation_rate[t]*f.mixer_decimation_rate[6];
            if(f.mixer_decimation_cnt[t]>=1.f) { f.mixer_decimation_cnt[t]-=1.f; f.mixer_voice_samples[t]=mono[i]; }
            const auto sample=f.mixer_voice_samples[t];
            audio[i][2+t*2]=sample*panL/32768.f; audio[i][3+t*2]=sample*panR/32768.f;
            fx[i*2]=int16_t(__SSAT(fx[i*2]+sample*panL,16)); fx[i*2+1]=int16_t(__SSAT(fx[i*2+1]+sample*panR,16));
        }
    }
    // Original four effects operate on the stereo sum. Stems are pre-master-FX.
    f.FxBus_process(fx,other,other+1,out,out+1,64);
    for(int i=0;i<64;++i) { audio[i][0]=out[i*2]/32768.f; audio[i][1]=out[i*2+1]/32768.f; }
    cursor=0;
}
void Engine::advanceNative() { if(cursor>=64) renderNativeBlock(); left=right; right=audio[size_t(cursor++)]; }
std::array<float,14> Engine::nextSample() {
    if(!primed) { advanceNative(); left=right; primed=true; }
    std::array<float,14> result{};
    for(int i=0;i<14;++i) result[i]=left[i]+float(phase)*(right[i]-left[i]);
    phase+=nativeRate/rate;
    if(running) ppq+=increment;
    while(phase>=1.0) { phase-=1.0; advanceNative(); }
    return result;
}
}
