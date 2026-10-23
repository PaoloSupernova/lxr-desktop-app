// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Engine.h"
class PulseProcessor final: public juce::AudioProcessor {
public:
    PulseProcessor();
    const juce::String getName() const override { return "Pulse Foundry"; }
    void prepareToPlay(double,int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Session"; }
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    bool loadProject(const juce::File&,bool kitOnly,juce::String& error);
    bool saveProject(const juce::File&,bool kitOnly,juce::String& error);
    bool restore(const void*,int, bool kitOnly, juce::String& error);
    void audition(int track,int velocity=110);
    float parameter(int id) const;
    void parameter(int id,float value);
    static juce::String pid(int id) { return "sound_"+juce::String(id); }
    pf::Sequence sequence;
    pf::Engine engine;
    juce::AudioProcessorValueTreeState state;
    std::atomic<float> peak{0};
    std::atomic<int> panicRequested{0};
    juce::String presetName="Init kit";
private:
    static BusesProperties buses();
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    std::array<std::atomic<float>*,512> raw{};
    std::array<int,512> last{};
    std::atomic<float> *tempo=nullptr,*swing=nullptr,*mode=nullptr,*play=nullptr,*master=nullptr,*midiChannel=nullptr;
    juce::AbstractFifo hits{256};
    std::array<pf::Hit,256> hitData{};
    std::atomic<bool> resetRequested{false};
    int pendingRestore=0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulseProcessor)
};
