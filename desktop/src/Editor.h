// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "Processor.h"
class PulseLook final: public juce::LookAndFeel_V4 {
public:
    PulseLook();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
};
class PulseEditor final: public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit PulseEditor(PulseProcessor&);
    ~PulseEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
private:
    using Attachment=juce::AudioProcessorValueTreeState::SliderAttachment;
    PulseProcessor& processor;
    PulseLook look;
    juce::TextButton run{"PLAY"},panic{"PANIC"},save{"SAVE PROJECT"},load{"OPEN"},settings{"AUDIO / MIDI"},kitSave{"SAVE KIT"},kitLoad{"LOAD KIT"},generate{"EUCLIDEAN"},clear{"CLEAR TRACK"};
    juce::ComboBox pattern,mode,section,advanced,flam,lock1,lock2,lfoTarget,velocityTarget;
    juce::Label status;
    juce::Slider tempo,swing,master,trackLength,trackRotation,velocity,probability,note,shift,lockValue1,lockValue2,euclidHits;
    juce::ToggleButton song{"SONG"};
    juce::TextEditor chain;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> playAttachment;
    std::unique_ptr<Attachment> tempoAttachment,swingAttachment,masterAttachment;
    std::array<juce::TextButton,7> pads,mutes;
    std::array<juce::TextButton,112> grid;
    std::array<juce::TextButton,4> pages;
    std::array<juce::Slider,8> knobs;
    std::array<juce::Label,8> knobLabels;
    std::array<std::unique_ptr<Attachment>,8> knobAttachments;
    std::unique_ptr<juce::FileChooser> chooser;
    int selectedTrack=0,selectedStep=0,page=0,lastFx=-1;
    bool updating=false;
    float scale=1;
    void timerCallback() override;
    void bindKnobs();
    void updateInspector();
    void changeStep();
    void fileDialog(bool writing,bool kit);
    juce::Rectangle<int> scaled(int x,int y,int w,int h) const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulseEditor)
};
