// SPDX-License-Identifier: GPL-3.0-only
#include "Processor.h"
#include "Editor.h"
#include <cmath>
namespace {
constexpr int mappedNotes[]={36,38,43,40,49,42,46};
constexpr int magic=0x50465351;
}
juce::AudioProcessor::BusesProperties PulseProcessor::buses() {
    juce::AudioProcessor::BusesProperties b;
    b=b.withOutput("Master",juce::AudioChannelSet::stereo(),true);
    const char* names[]={"Drum 1","Drum 2","Drum 3","Snare","Cymbal","Hi-hats"};
    for(auto n:names) b=b.withOutput(n,juce::AudioChannelSet::stereo(),false);
    return b;
}
juce::AudioProcessorValueTreeState::ParameterLayout PulseProcessor::layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    for(const auto& s:pf::parameterSpecs()) if(s.maximum>0)
        l.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{pid(s.id),1},juce::String(s.name),0,s.maximum,s.initial));
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"tempo",1},"Tempo",juce::NormalisableRange<float>(20,400,0.1f),120));
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"swing",1},"Swing",juce::NormalisableRange<float>(0,49,0.1f),0));
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode",1},"Playback mode",juce::StringArray{"Patterns + MIDI","MIDI only"},0));
    l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"play",1},"Standalone run",false));
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"master",1},"Master gain",juce::NormalisableRange<float>(0,1,0.001f),0.75f));
    l.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"channel",1},"MIDI channel (0 = omni)",0,16,0));
    return l;
}
PulseProcessor::PulseProcessor():juce::AudioProcessor(buses()),engine(sequence),state(*this,nullptr,"PulseFoundry",layout()) {
    last.fill(-1);
    for(const auto& s:pf::parameterSpecs()) raw[s.id]=state.getRawParameterValue(pid(s.id));
    tempo=state.getRawParameterValue("tempo"); swing=state.getRawParameterValue("swing"); mode=state.getRawParameterValue("mode"); play=state.getRawParameterValue("play"); master=state.getRawParameterValue("master"); midiChannel=state.getRawParameterValue("channel");
    // A small starter groove. All other 63 pattern slots begin empty.
    for(int i=0;i<16;++i) { pf::Step s; s.on=(i%4==0); sequence.bank[0].data[0][i]=s.pack(); s.on=i%8==4; sequence.bank[0].data[3][i]=s.pack(); s.on=i%2==0; s.velocity=i%4==0?85:65; sequence.bank[0].data[5][i]=s.pack(); }
}
void PulseProcessor::prepareToPlay(double sr,int) { engine.prepare(sr); last.fill(-1); resetRequested=false; }
bool PulseProcessor::isBusesLayoutSupported(const BusesLayout& b) const {
    if(!b.inputBuses.isEmpty()||b.outputBuses.size()!=7||b.getMainOutputChannelSet()!=juce::AudioChannelSet::stereo()) return false;
    for(int i=1;i<b.outputBuses.size();++i) if(!b.outputBuses[i].isDisabled()&&b.outputBuses[i]!=juce::AudioChannelSet::stereo()) return false;
    return true;
}
void PulseProcessor::audition(int t,int v) {
    int a,n,b,m; hits.prepareToWrite(1,a,n,b,m);
    if(n) { hitData[size_t(a)]={t,v,60}; hits.finishedWrite(1); }
}
float PulseProcessor::parameter(int id) const { return raw[size_t(id)]?raw[size_t(id)]->load():0; }
void PulseProcessor::parameter(int id,float value) { if(auto* p=state.getParameter(pid(id))) p->setValueNotifyingHost(p->convertTo0to1(value)); }
void PulseProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    if(resetRequested.exchange(false)) { engine.panic(); last.fill(-1); }
    if(panicRequested.exchange(0)) engine.panic();
    for(const auto& s:pf::parameterSpecs()) if(auto* p=raw[s.id]) { int v=int(std::lround(p->load())); if(last[s.id]!=v) { engine.setParameter(s.id,v); last[s.id]=v; } }
    pf::Transport t; t.bpm=tempo->load(); t.playing=play->load()>0.5f;
    if(!juce::JUCEApplicationBase::isStandaloneApp()) {
        t.playing=false;
        if(auto* ph=getPlayHead()) if(auto pos=ph->getPosition()) {
            t.playing=pos->getIsPlaying(); if(auto bpm=pos->getBpm()) t.bpm=*bpm;
            if(auto p=pos->getPpqPosition()) { t.ppq=*p; t.positioned=true; }
        }
    }
    engine.beginBlock(t,mode->load()<0.5f,swing->load()/100.f);
    int a,n,b,m; hits.prepareToRead(hits.getNumReady(),a,n,b,m);
    for(int i=0;i<n;++i) { auto h=hitData[size_t(a+i)]; engine.hit(h.track,h.velocity,h.note); }
    for(int i=0;i<m;++i) { auto h=hitData[size_t(b+i)]; engine.hit(h.track,h.velocity,h.note); }
    hits.finishedRead(n+m);
    auto event=midi.begin(); auto end=midi.end();
    std::array<float*,14> outs{};
    for(int bus=0;bus<7;++bus) if(getBus(false,bus)->isEnabled()) {
        auto view=getBusBuffer(buffer,false,bus); outs[bus*2]=view.getWritePointer(0); outs[bus*2+1]=view.getWritePointer(1);
    }
    float p=0; const float gain=master->load();
    for(int i=0;i<buffer.getNumSamples();++i) {
        while(event!=end && (*event).samplePosition<=i) {
            const auto data=*event;
            // Read short MIDI bytes directly: no heap allocation for SysEx.
            if(data.numBytes>=3) {
                auto* bytes=data.data; const int status=bytes[0]&0xf0, chan=(bytes[0]&15)+1;
                if(int(midiChannel->load())==0||chan==int(midiChannel->load())) {
                    if(status==0x90&&bytes[2]>0) for(int track=0;track<7;++track) if(bytes[1]==mappedNotes[track]) engine.hit(track,bytes[2],60);
                    if(status==0xb0&&(bytes[1]==120||bytes[1]==123)) engine.panic();
                }
            }
            ++event;
        }
        auto samples=engine.nextSample();
        for(int c=0;c<14;++c) if(outs[c]) { float v=samples[c]*(c<2?gain:1.f); outs[c][i]=std::isfinite(v)?v:0; if(c<2) p=std::max(p,std::abs(v)); }
    }
    peak.store(p); midi.clear();
}
void PulseProcessor::getStateInformation(juce::MemoryBlock& target) {
    auto tree=state.copyState(); tree.setProperty("version",1,nullptr); tree.setProperty("name",presetName,nullptr);
    juce::MemoryOutputStream data;
    data.writeInt(magic); data.writeInt(1); data.writeInt(sequence.selected.load()); data.writeInt(sequence.songEnabled.load()); data.writeInt(sequence.songLength.load());
    for(auto& s:sequence.song) data.writeInt(s.load());
    for(auto& p:sequence.bank) for(int t=0;t<7;++t) { data.writeInt(p.length[t].load()); data.writeInt(p.rotation[t].load()); data.writeInt(p.mute[t].load()); for(auto& s:p.data[t]) data.writeInt64(juce::int64(s.load())); }
    tree.setProperty("sequence",data.getMemoryBlock().toBase64Encoding(),nullptr);
    juce::MemoryOutputStream out(target,false); tree.writeToStream(out);
}
bool PulseProcessor::restore(const void* ptr,int size,bool kitOnly,juce::String& error) {
    if(!ptr||size<=0||size>2*1024*1024) { error="Invalid project size."; return false; }
    auto tree=juce::ValueTree::readFromData(ptr,size);
    if(!tree.hasType("PulseFoundry")||int(tree.getProperty("version",0))!=1) { error="This is not a supported Pulse Foundry project."; return false; }
    // Validate every parameter before changing the running state. Reject missing,
    // duplicate and non-finite data; keep stable host parameter IDs.
    auto checked=state.copyState();
    for(auto child:checked) {
        const auto id=child.getProperty("id").toString(); if(kitOnly&&!id.startsWith("sound_")) continue; auto incoming=tree.getChildWithProperty("id",id);
        if(!incoming.isValid()) { error="Project is missing parameter "+id; return false; }
        const double v=double(incoming.getProperty("value")); auto* p=state.getParameter(id);
        if(!std::isfinite(v)||!p||v<p->getNormalisableRange().start||v>p->getNormalisableRange().end) { error="Invalid parameter "+id; return false; }
        child.setProperty("value",v,nullptr);
    }
    struct Snapshot { int selected=0, songOn=0, count=1; std::array<int,64> song{}; struct Track { int len=16,rot=0,mute=0; std::array<uint64_t,64> data{}; }; std::array<std::array<Track,7>,64> bank; };
    auto snap=std::make_unique<Snapshot>();
    if(!kitOnly) {
        juce::MemoryBlock encoded;
        if(!encoded.fromBase64Encoding(tree.getProperty("sequence").toString())) { error="Invalid sequence encoding."; return false; }
        constexpr size_t expected=5*4+64*4+64*7*(3*4+64*8);
        if(encoded.getSize()!=expected) { error="Sequence has the wrong size."; return false; }
        juce::MemoryInputStream in(encoded,false);
        if(in.readInt()!=magic||in.readInt()!=1) { error="Unknown sequence format."; return false; }
        snap->selected=in.readInt(); snap->songOn=in.readInt(); snap->count=in.readInt();
        if(snap->selected<0||snap->selected>63||snap->songOn<0||snap->songOn>1||snap->count<1||snap->count>64) { error="Invalid song settings."; return false; }
        for(auto& s:snap->song) { s=in.readInt(); if(s<0||s>63) { error="Invalid song pattern."; return false; } }
        for(auto& p:snap->bank) for(auto& t:p) {
            t.len=in.readInt(); t.rot=in.readInt(); t.mute=in.readInt();
            if(t.len<1||t.len>64||t.rot<0||t.rot>63||t.mute<0||t.mute>1) { error="Invalid track settings."; return false; }
            for(auto& s:t.data) { s=uint64_t(in.readInt64()); if(pf::Step::unpack(s).pack()!=s) { error="Invalid step data."; return false; } }
        }
    }
    checked.setProperty("version",1,nullptr);
    state.replaceState(checked);
    if(!kitOnly) {
        for(int p=0;p<64;++p) for(int t=0;t<7;++t) { const auto& s=snap->bank[p][t]; auto& dest=sequence.bank[p]; dest.length[t]=s.len; dest.rotation[t]=s.rot; dest.mute[t]=s.mute; for(int i=0;i<64;++i) dest.data[t][i]=s.data[i]; }
        for(int i=0;i<64;++i) sequence.song[i]=snap->song[i];
        sequence.selected=snap->selected; sequence.songEnabled=snap->songOn; sequence.songLength=snap->count;
    }
    presetName=tree.getProperty("name","Session").toString().substring(0,80);
    resetRequested=true; return true;
}
void PulseProcessor::setStateInformation(const void* p,int s) { juce::String error; restore(p,s,false,error); }
bool PulseProcessor::saveProject(const juce::File& file,bool kitOnly,juce::String& error) {
    juce::MemoryBlock block; getStateInformation(block);
    // Kits omit sequence bytes and leave desktop playback settings unchanged.
    if(kitOnly) { auto tree=juce::ValueTree::readFromData(block.getData(),block.getSize()); tree.removeProperty("sequence",nullptr); block.reset(); juce::MemoryOutputStream out(block,false); tree.writeToStream(out); }
    if(!file.replaceWithData(block.getData(),block.getSize())) { error="Could not write "+file.getFullPathName(); return false; } return true;
}
bool PulseProcessor::loadProject(const juce::File& file,bool kitOnly,juce::String& error) {
    if(file.getSize()>2*1024*1024) { error="Project exceeds the 2 MB limit."; return false; }
    juce::MemoryBlock block; if(!file.loadFileAsData(block)) { error="Could not read the selected file."; return false; }
    return restore(block.getData(),int(block.getSize()),kitOnly,error);
}
juce::AudioProcessorEditor* PulseProcessor::createEditor() { return new PulseEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PulseProcessor(); }
