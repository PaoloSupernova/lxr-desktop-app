// SPDX-License-Identifier: GPL-3.0-only
#include "Editor.h"
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
namespace {
const juce::Colour bg{0xff11171b}, panel{0xff1a2329}, line{0xff303d45}, ink{0xffe9e6db}, muted{0xff899ba3}, accent{0xfff2a55e}, mint{0xff89c7b6};
const char* trackNames[]={"01  DRUM ONE","02  DRUM TWO","03  DRUM THREE","04  SNARE","05  CYMBAL","06  CLOSED HAT","07  OPEN HAT"};
void linear(juce::Slider& s,double lo,double hi,double interval) { s.setSliderStyle(juce::Slider::LinearHorizontal); s.setTextBoxStyle(juce::Slider::TextBoxRight,false,48,20); s.setRange(lo,hi,interval); }
}
PulseLook::PulseLook() {
    setColour(juce::ResizableWindow::backgroundColourId,bg); setColour(juce::Label::textColourId,ink);
    setColour(juce::TextButton::textColourOffId,ink); setColour(juce::TextButton::textColourOnId,bg);
    setColour(juce::TextButton::buttonColourId,panel); setColour(juce::TextButton::buttonOnColourId,accent);
    setColour(juce::ComboBox::backgroundColourId,panel); setColour(juce::ComboBox::textColourId,ink); setColour(juce::ComboBox::outlineColourId,line);
    setColour(juce::Slider::textBoxTextColourId,ink); setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack); setColour(juce::Slider::thumbColourId,accent);
    setColour(juce::Slider::trackColourId,accent); setColour(juce::Slider::backgroundColourId,line);
    setColour(juce::PopupMenu::backgroundColourId,panel); setColour(juce::PopupMenu::textColourId,ink); setColour(juce::PopupMenu::highlightedBackgroundColourId,line);
    setColour(juce::ToggleButton::textColourId,ink); setColour(juce::ToggleButton::tickColourId,accent);
    setColour(juce::TextEditor::backgroundColourId,bg); setColour(juce::TextEditor::textColourId,ink); setColour(juce::TextEditor::outlineColourId,line);
}
void PulseLook::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider&) {
    auto r=juce::Rectangle<float>(float(x),float(y),float(w),float(h)).reduced(8); float radius=std::min(r.getWidth(),r.getHeight())/2; auto c=r.getCentre();
    juce::Path arc; arc.addCentredArc(c.x,c.y,radius,radius,0,start,end,true); g.setColour(line); g.strokePath(arc,juce::PathStrokeType(3));
    juce::Path active; active.addCentredArc(c.x,c.y,radius,radius,0,start,start+pos*(end-start),true); g.setColour(accent); g.strokePath(active,juce::PathStrokeType(3));
    g.setColour(juce::Colour(0xff0c1114)); g.fillEllipse(c.x-radius+7,c.y-radius+7,2*radius-14,2*radius-14);
    g.setColour(juce::Colour(0xff38444a)); g.drawEllipse(c.x-radius+7,c.y-radius+7,2*radius-14,2*radius-14,1);
    float a=start+pos*(end-start)-juce::MathConstants<float>::halfPi;
    g.setColour(ink); g.drawLine(c.x+std::cos(a)*(radius-20),c.y+std::sin(a)*(radius-20),c.x+std::cos(a)*(radius-10),c.y+std::sin(a)*(radius-10),2);
}
void PulseLook::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour& colour,bool hover,bool down) {
    auto r=b.getLocalBounds().toFloat().reduced(0.5f); auto c=b.getToggleState()?b.findColour(juce::TextButton::buttonOnColourId):colour;
    if(hover) c=c.brighter(0.12f); if(down) c=c.darker(0.1f);
    g.setColour(c); g.fillRoundedRectangle(r,4); g.setColour(line); g.drawRoundedRectangle(r,4,1);
}
PulseEditor::PulseEditor(PulseProcessor& p):AudioProcessorEditor(p),processor(p) {
    setLookAndFeel(&look); setWantsKeyboardFocus(true); setResizable(true,true); setResizeLimits(1024,720,1920,1350);
    for(auto* c:std::initializer_list<juce::Component*>{&run,&panic,&save,&load,&settings,&kitSave,&kitLoad,&generate,&clear,&pattern,&mode,&section,&advanced,&flam,&lock1,&lock2,&lfoTarget,&velocityTarget,&status,&tempo,&swing,&master,&trackLength,&trackRotation,&velocity,&probability,&note,&shift,&lockValue1,&lockValue2,&euclidHits,&song,&chain}) addAndMakeVisible(c);
    run.setClickingTogglesState(true); playAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"play",run);
    run.setEnabled(juce::JUCEApplicationBase::isStandaloneApp()); run.setTooltip("Standalone transport. The VST3 follows Ableton's transport.");
    panic.onClick=[this]{processor.panicRequested=1;};
    save.onClick=[this]{fileDialog(true,false);}; load.onClick=[this]{fileDialog(false,false);}; kitSave.onClick=[this]{fileDialog(true,true);}; kitLoad.onClick=[this]{fileDialog(false,true);};
    settings.onClick=[this]{ if(auto* h=juce::StandalonePluginHolder::getInstance()) h->showAudioSettingsDialog(); };
    settings.setVisible(juce::JUCEApplicationBase::isStandaloneApp());
    linear(tempo,20,400,.1); linear(swing,0,49,.1); linear(master,0,1,.001);
    tempoAttachment=std::make_unique<Attachment>(p.state,"tempo",tempo); swingAttachment=std::make_unique<Attachment>(p.state,"swing",swing); masterAttachment=std::make_unique<Attachment>(p.state,"master",master);
    tempo.setEnabled(juce::JUCEApplicationBase::isStandaloneApp());
    mode.addItem("Patterns + MIDI",1); mode.addItem("MIDI only",2); modeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"mode",mode);
    for(int i=0;i<64;++i) pattern.addItem("PATTERN "+juce::String(i+1).paddedLeft('0',2),i+1);
    pattern.setSelectedId(p.sequence.selected.load()+1); pattern.onChange=[this]{processor.sequence.selected=pattern.getSelectedId()-1; updateInspector();};
    section.addItemList({"OSCILLATORS","AMPLITUDE","FILTER / DRIVE","TRANSIENT","MODULATION","MASTER FX","ALL PARAMETERS"},1); section.setSelectedId(1); section.onChange=[this]{bindKnobs();};
    for(const auto& spec:pf::parameterSpecs()) if(spec.maximum>0) { advanced.addItem(juce::String(spec.name),spec.id+1); lock1.addItem(juce::String(spec.name),spec.id+1); lock2.addItem(juce::String(spec.name),spec.id+1); }
    lfoTarget.addItem("Off",1); velocityTarget.addItem("Off",1);
    for(const auto& spec:pf::parameterSpecs()) if(spec.maximum>0 && !(spec.id>=pf::PAR_VEL_DEST_1&&spec.id<=pf::PAR_VEL_DEST_6) && !(spec.id>=pf::PAR_TARGET_LFO1&&spec.id<=pf::PAR_TARGET_LFO6)) {
        const auto label=juce::String(spec.name);lfoTarget.addItem(label,spec.id+1);velocityTarget.addItem(label,spec.id+1);
    }
    lfoTarget.onChange=[this]{if(!updating)processor.parameter(pf::PAR_TARGET_LFO1+std::min(selectedTrack,5),float(lfoTarget.getSelectedId()-1));};
    velocityTarget.onChange=[this]{if(!updating)processor.parameter(pf::PAR_VEL_DEST_1+std::min(selectedTrack,5),float(velocityTarget.getSelectedId()-1));};
    lock1.addItem("No lock",1); lock2.addItem("No lock",1); advanced.setSelectedItemIndex(0); advanced.onChange=[this]{if(section.getSelectedId()==7)bindKnobs();};
    flam.addItemList({"Off","x2","x3","x4","x6","x8"},1);
    linear(trackLength,1,64,1); linear(trackRotation,0,63,1); linear(velocity,1,127,1); linear(probability,0,100,1); linear(note,0,127,1); linear(shift,-7,7,1); linear(lockValue1,0,127,1); linear(lockValue2,0,127,1); linear(euclidHits,0,64,1); euclidHits.setValue(4);
    trackLength.onValueChange=[this]{if(!updating)processor.sequence.bank[processor.sequence.selected.load()].length[selectedTrack]=int(trackLength.getValue());};
    trackRotation.onValueChange=[this]{if(!updating)processor.sequence.bank[processor.sequence.selected.load()].rotation[selectedTrack]=int(trackRotation.getValue());};
    for(auto* s:{&velocity,&probability,&note,&shift,&lockValue1,&lockValue2}) s->onValueChange=[this]{changeStep();};
    flam.onChange=[this]{changeStep();}; lock1.onChange=[this]{changeStep();}; lock2.onChange=[this]{changeStep();};
    generate.onClick=[this]{processor.sequence.euclidean(processor.sequence.selected.load(),selectedTrack,int(trackLength.getValue()),int(euclidHits.getValue()),int(trackRotation.getValue())); updateInspector();};
    clear.onClick=[this]{for(auto& s:processor.sequence.bank[processor.sequence.selected.load()].data[selectedTrack])s=pf::Step{}.pack(); updateInspector();};
    song.setToggleState(p.sequence.songEnabled.load()!=0,juce::dontSendNotification); song.onClick=[this]{processor.sequence.songEnabled=song.getToggleState()?1:0;};
    chain.setText("1",false); chain.setTooltip("Pattern numbers separated by commas, up to 64 entries; the song loops.");
    chain.onReturnKey=[this]{ auto tokens=juce::StringArray::fromTokens(chain.getText(),", ",""); tokens.removeEmptyStrings(); if(tokens.isEmpty()||tokens.size()>64){status.setText("Use 1-64 pattern numbers.",juce::dontSendNotification);return;} for(auto s:tokens)if(s.getIntValue()<1||s.getIntValue()>64){status.setText("Pattern numbers must be 1-64.",juce::dontSendNotification);return;} for(int i=0;i<tokens.size();++i)processor.sequence.song[i]=tokens[i].getIntValue()-1; processor.sequence.songLength=tokens.size();status.setText("Song chain updated",juce::dontSendNotification);};
    for(int t=0;t<7;++t) {
        addAndMakeVisible(pads[t]); addAndMakeVisible(mutes[t]); pads[t].setButtonText(trackNames[t]); mutes[t].setButtonText("M"); mutes[t].setClickingTogglesState(true);
        pads[t].onClick=[this,t]{selectedTrack=t;processor.audition(t);bindKnobs();updateInspector();};
        mutes[t].onClick=[this,t]{processor.sequence.bank[processor.sequence.selected.load()].mute[t]=mutes[t].getToggleState()?1:0;};
        for(int s=0;s<16;++s) {auto& b=grid[t*16+s];addAndMakeVisible(b);b.setButtonText(juce::String(s+1));b.onClick=[this,t,s]{selectedTrack=t;selectedStep=page*16+s;auto& cell=processor.sequence.bank[processor.sequence.selected.load()].data[t][selectedStep];auto v=pf::Step::unpack(cell.load());v.on=!v.on;cell=v.pack();bindKnobs();updateInspector();};b.setTooltip("Toggle step; the inspector edits the selected step.");}
    }
    for(int i=0;i<4;++i){addAndMakeVisible(pages[i]);pages[i].setButtonText(juce::String(i*16+1)+"-"+juce::String(i*16+16));pages[i].onClick=[this,i]{page=i;selectedStep=page*16;updateInspector();};}
    for(int i=0;i<8;++i){addAndMakeVisible(knobs[i]);addAndMakeVisible(knobLabels[i]);knobs[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);knobs[i].setTextBoxStyle(juce::Slider::TextBoxBelow,false,58,20);knobLabels[i].setJustificationType(juce::Justification::centred);knobLabels[i].setFont(juce::Font(11.f));}
    status.setFont(juce::Font(12.f)); status.setColour(juce::Label::textColourId,muted); status.setText("Modified firmware port | GPLv3 | Unofficial",juce::dontSendNotification);
    setSize(1280,900);bindKnobs();updateInspector();timerCallback();startTimerHz(25);
}
PulseEditor::~PulseEditor(){stopTimer();setLookAndFeel(nullptr);}
juce::Rectangle<int> PulseEditor::scaled(int x,int y,int w,int h) const { return {int(x*scale),int(y*scale),int(w*scale),int(h*scale)}; }
void PulseEditor::resized(){
    scale=std::min(getWidth()/1280.f,getHeight()/900.f);
    run.setBounds(scaled(730,28,72,34));panic.setBounds(scaled(810,28,72,34));load.setBounds(scaled(894,28,66,34));save.setBounds(scaled(968,28,126,34));settings.setBounds(scaled(1102,28,146,34));
    tempo.setBounds(scaled(92,105,180,30));swing.setBounds(scaled(357,105,170,30));master.setBounds(scaled(630,105,170,30));mode.setBounds(scaled(855,104,185,32));
    pattern.setBounds(scaled(32,167,170,32));section.setBounds(scaled(220,167,220,32));advanced.setBounds(scaled(460,167,240,32));kitLoad.setBounds(scaled(1018,167,104,30));kitSave.setBounds(scaled(1132,167,104,30));
    lfoTarget.setBounds(scaled(495,282,138,32));velocityTarget.setBounds(scaled(1095,282,138,32));
    for(int i=0;i<8;++i){knobLabels[i].setBounds(scaled(40+i*150,222,140,22));knobs[i].setBounds(scaled(52+i*150,250,116,122));}
    for(int i=0;i<4;++i)pages[i].setBounds(scaled(842+i*100,406,90,28));
    for(int t=0;t<7;++t){pads[t].setBounds(scaled(32,452+t*43,162,34));mutes[t].setBounds(scaled(202,452+t*43,30,34));for(int s=0;s<16;++s)grid[t*16+s].setBounds(scaled(250+s*62,452+t*43,53,34));}
    trackLength.setBounds(scaled(87,771,140,24));trackRotation.setBounds(scaled(292,771,140,24));euclidHits.setBounds(scaled(495,771,118,24));generate.setBounds(scaled(630,767,118,30));clear.setBounds(scaled(760,767,116,30));song.setBounds(scaled(900,767,75,30));chain.setBounds(scaled(982,768,250,29));
    velocity.setBounds(scaled(72,822,115,24));probability.setBounds(scaled(248,822,115,24));note.setBounds(scaled(405,822,105,24));flam.setBounds(scaled(566,820,85,28));shift.setBounds(scaled(713,822,110,24));
    lock1.setBounds(scaled(842,815,178,22));lockValue1.setBounds(scaled(1024,815,110,22));lock2.setBounds(scaled(842,842,178,22));lockValue2.setBounds(scaled(1024,842,110,22));status.setBounds(scaled(28,870,1200,22));
}
void PulseEditor::paint(juce::Graphics& graphics){
    graphics.fillAll(bg);juce::Graphics::ScopedSaveState saveState(graphics);graphics.addTransform(juce::AffineTransform::scale(scale));auto& g=graphics;
    g.setColour(panel);g.fillRoundedRectangle(20,88,1240,63,7);g.fillRoundedRectangle(20,210,1240,177,7);g.fillRoundedRectangle(20,442,1240,313,7);
    g.setColour(accent);g.fillRect(32,28,5,35);g.setFont(juce::Font(30.f,juce::Font::bold));g.drawText("PULSE FOUNDRY",50,24,350,40,juce::Justification::centredLeft);
    g.setColour(muted);g.setFont(juce::Font(11.f));g.drawText("SIX VOICES  /  DIGITAL PERCUSSION LAB",51,64,430,16,juce::Justification::centredLeft);
    g.drawText("BPM",36,111,54,20,juce::Justification::centredLeft);g.drawText("SWING",298,111,60,20,juce::Justification::centredLeft);g.drawText("MASTER",562,111,70,20,juce::Justification::centredLeft);
    g.setColour(line);g.fillRoundedRectangle(1070,112,164,12,3);g.setColour(processor.peak.load()>.95f?accent:mint);g.fillRoundedRectangle(1070,112,164*std::clamp(processor.peak.load(),0.f,1.f),12,3);
    g.setColour(ink);g.setFont(juce::Font(17.f,juce::Font::bold));g.drawText("STEP SEQUENCER",32,404,240,32,juce::Justification::centredLeft);g.setColour(muted);g.setFont(juce::Font(11.f));g.drawText("7 TRACKS  /  64 STEPS  /  64 PATTERNS",270,412,360,20,juce::Justification::centredLeft);
    g.drawText("LENGTH",32,773,60,20,juce::Justification::centredLeft);g.drawText("ROTATE",237,773,60,20,juce::Justification::centredLeft);g.drawText("HITS",445,773,50,20,juce::Justification::centredLeft);
    g.drawText("VEL",32,824,40,20,juce::Justification::centredLeft);g.drawText("PROB %",195,824,58,20,juce::Justification::centredLeft);g.drawText("NOTE",365,824,45,20,juce::Justification::centredLeft);g.drawText("FLAM",522,824,45,20,juce::Justification::centredLeft);g.drawText("SHIFT",665,824,50,20,juce::Justification::centredLeft);
    g.setColour(accent);g.drawText("TRACK "+juce::String(selectedTrack+1)+"  /  STEP "+juce::String(selectedStep+1),1142,821,100,40,juce::Justification::centred);
}
void PulseEditor::bindKnobs(){
    using namespace pf;const int v=std::min(selectedTrack,5);int ids[8]{};
    const int wave[]={PAR_OSC_WAVE_DRUM1,PAR_OSC_WAVE_DRUM2,PAR_OSC_WAVE_DRUM3,PAR_OSC_WAVE_SNARE,PAR_WAVE1_CYM,PAR_WAVE1_HH};
    const int attack[]={PAR_VELOA1,PAR_VELOA2,PAR_VELOA3,PAR_VELOA4,PAR_VELOA5,PAR_VELOA6};
    const int decay[]={PAR_VELOD1,PAR_VELOD2,PAR_VELOD3,PAR_VELOD4,PAR_VELOD5,PAR_VELOD6_CLOSED};
    const int modWave[]={PAR_MOD_WAVE_DRUM1,PAR_MOD_WAVE_DRUM2,PAR_MOD_WAVE_DRUM3,PAR_OSC_WAVE_SNARE,PAR_WAVE2_CYM,PAR_WAVE2_HH};
    const int fm[]={PAR_FMAMNT1,PAR_FMAMNT2,PAR_FMAMNT3,PAR_MIX1,PAR_MOD_OSC_GAIN1_CYM,PAR_MOD_OSC_GAIN1};
    switch(section.getSelectedId()){
    case 1: {int a[]={wave[v],PAR_COARSE1+v*2,PAR_FINE1+v*2,modWave[v],fm[v],PAR_PWM_1+v,PAR_VOL1+v,PAR_PAN1+v+(v>=3?2:0)};std::copy(a,a+8,ids);break;}
    case 2: {int a[]={attack[v],selectedTrack==6?PAR_VELOD6_OPEN:decay[v],PAR_VOL_SLOPE1+v,PAR_VOL1+v,PAR_VOLUME_MOD_ON_OFF1+v,PAR_VELO_MOD_AMT_1+v,v<4?PAR_MOD_EG1+v:0,v<4?PAR_MODAMNT1+v:0};std::copy(a,a+8,ids);break;}
    case 3: {int a[]={PAR_FILTER_FREQ_1+v,PAR_RESO_1+v,PAR_FILTER_TYPE_1+v,PAR_FILTER_DRIVE_1+v,PAR_DRIVE1+v,PAR_VOICE_DECIMATION1+v,PAR_VOICE_DECIMATION_ALL,PAR_PWM_1+v};std::copy(a,a+8,ids);break;}
    case 4: {int a[]={PAR_TRANS1_WAVE+v,PAR_TRANS1_VOL+v,PAR_TRANS1_FREQ+v,PAR_VOL1+v,PAR_PWM_1+v,attack[v],decay[v],PAR_VOL_SLOPE1+v};std::copy(a,a+8,ids);break;}
    case 5: {int a[]={PAR_FREQ_LFO1+v,PAR_WAVE_LFO1+v,PAR_AMOUNT_LFO1+v,PAR_TARGET_LFO1+v,PAR_SYNC_LFO1+v,PAR_RETRIGGER_LFO1+v,PAR_OFFSET_LFO1+v,PAR_VEL_DEST_1+v};std::copy(a,a+8,ids);break;}
    case 6: {int a[]={PAR_FX_TYPE,PAR_FX_DRY_WET,PAR_FX_P1,PAR_FX_P2,PAR_FX_P3,PAR_FX_P4,PAR_FX_DIST_TYPE,PAR_FX_DELAY_TYPE};std::copy(a,a+8,ids);break;}
    case 7: ids[0]=advanced.getSelectedId()-1;break;
    }
    advanced.setVisible(section.getSelectedId()==7);
    const bool modulation=section.getSelectedId()==5;
    lfoTarget.setVisible(modulation);velocityTarget.setVisible(modulation);
    updating=true;
    lfoTarget.setSelectedId(int(processor.parameter(PAR_TARGET_LFO1+v))+1,juce::dontSendNotification);
    velocityTarget.setSelectedId(int(processor.parameter(PAR_VEL_DEST_1+v))+1,juce::dontSendNotification);
    updating=false;
    const char* labels[6][8]={
        {"WAVEFORM","COARSE TUNE","FINE TUNE","MODULATOR WAVE","FM / NOISE MIX","PULSE WIDTH","VOICE LEVEL","PAN"},
        {"ATTACK","DECAY","ENVELOPE SLOPE","VOICE LEVEL","VELOCITY ON","VELOCITY AMOUNT","PITCH DECAY","PITCH AMOUNT"},
        {"CUTOFF","RESONANCE","FILTER TYPE","FILTER DRIVE","DISTORTION","SAMPLE REDUCTION","MASTER REDUCTION","PULSE WIDTH"},
        {"TRANSIENT TYPE","TRANSIENT LEVEL","TRANSIENT PITCH","VOICE LEVEL","PULSE WIDTH","ATTACK","DECAY","ENVELOPE SLOPE"},
        {"RATE","WAVEFORM","DEPTH","LFO DESTINATION","SYNC DIVISION","RETRIGGER SOURCE","START PHASE","VELOCITY DESTINATION"},
        {"EFFECT TYPE","DRY / WET","CONTROL 1","CONTROL 2","CONTROL 3","CONTROL 4","DISTORTION TYPE","DELAY TYPE"}
    };
    lastFx=int(processor.parameter(PAR_FX_TYPE));
    for(int i=0;i<8;++i){knobAttachments[i].reset();bool valid=ids[i]>0&&processor.state.getParameter(PulseProcessor::pid(ids[i]));knobs[i].setVisible(valid);knobLabels[i].setVisible(valid);if(valid){for(auto& s:pf::parameterSpecs())if(s.id==ids[i]){knobs[i].setRange(0,s.maximum,1);knobLabels[i].setText(juce::String(s.name),juce::dontSendNotification);knobs[i].setTooltip(juce::String(s.name)+" | firmware units");}knobAttachments[i]=std::make_unique<Attachment>(processor.state,PulseProcessor::pid(ids[i]),knobs[i]);}}
    if(section.getSelectedId()<=6) for(int i=0;i<8;++i) knobLabels[i].setText(labels[section.getSelectedId()-1][i],juce::dontSendNotification);
    if(modulation) { knobs[3].setVisible(false);knobs[7].setVisible(false); }
    if(section.getSelectedId()==6) {
        const char* fxLabels[5][4]={{"CONTROL 1","CONTROL 2","CONTROL 3","CONTROL 4"},{"DRIVE","SHAPE","BIAS","TONE"},{"FREQUENCY","-","-","-"},{"ATTACK","RELEASE","THRESHOLD","MAKEUP GAIN"},{"TIME","TIME RANGE","FILTER","FEEDBACK"}};
        for(int i=0;i<4;++i) knobLabels[i+2].setText(fxLabels[std::clamp(lastFx,0,4)][i],juce::dontSendNotification);
    }
}
void PulseEditor::updateInspector(){
    updating=true;auto& p=processor.sequence.bank[processor.sequence.selected.load()];auto s=pf::Step::unpack(p.data[selectedTrack][selectedStep].load());
    trackLength.setValue(p.length[selectedTrack].load(),juce::dontSendNotification);trackRotation.setValue(p.rotation[selectedTrack].load(),juce::dontSendNotification);
    velocity.setValue(s.velocity,juce::dontSendNotification);probability.setValue(s.probability,juce::dontSendNotification);note.setValue(s.note,juce::dontSendNotification);shift.setValue(s.shift,juce::dontSendNotification);flam.setSelectedId(s.flam+1,juce::dontSendNotification);lock1.setSelectedId(s.lock1+1,juce::dontSendNotification);lock2.setSelectedId(s.lock2+1,juce::dontSendNotification);lockValue1.setValue(s.value1,juce::dontSendNotification);lockValue2.setValue(s.value2,juce::dontSendNotification);updating=false;repaint();
}
void PulseEditor::changeStep(){if(updating)return;auto& cell=processor.sequence.bank[processor.sequence.selected.load()].data[selectedTrack][selectedStep];auto s=pf::Step::unpack(cell.load());s.velocity=int(velocity.getValue());s.probability=int(probability.getValue());s.note=int(note.getValue());s.shift=int(shift.getValue());s.flam=flam.getSelectedId()-1;s.lock1=lock1.getSelectedId()-1;s.lock2=lock2.getSelectedId()-1;s.value1=int(lockValue1.getValue());s.value2=int(lockValue2.getValue());cell=s.pack();}
void PulseEditor::timerCallback(){
    if(section.getSelectedId()==6&&lastFx!=int(processor.parameter(pf::PAR_FX_TYPE)))bindKnobs();
    const int p=processor.sequence.selected.load();if(pattern.getSelectedId()!=p+1){pattern.setSelectedId(p+1,juce::dontSendNotification);updateInspector();}
    for(int t=0;t<7;++t){pads[t].setToggleState(t==selectedTrack,juce::dontSendNotification);mutes[t].setToggleState(processor.sequence.bank[p].mute[t].load()!=0,juce::dontSendNotification);for(int s=0;s<16;++s){auto& b=grid[t*16+s];int ix=page*16+s;bool on=pf::Step::unpack(processor.sequence.bank[p].data[t][ix].load()).on;int len=processor.sequence.bank[p].length[t].load();bool current=p==processor.sequence.soundingPattern.load()&&processor.sequence.trackHead[t].load()==ix;b.setButtonText(juce::String(ix+1)); b.setToggleState(on,juce::dontSendNotification);b.setColour(juce::TextButton::buttonColourId,current?line:panel);b.setColour(juce::TextButton::buttonOnColourId,current?mint:accent);b.setAlpha(ix<len?1.f:.25f);}}
    for(int i=0;i<4;++i)pages[i].setToggleState(i==page,juce::dontSendNotification);
    song.setToggleState(processor.sequence.songEnabled.load()!=0,juce::dontSendNotification);
    if(!chain.hasKeyboardFocus(true)) { juce::String text; for(int i=0;i<processor.sequence.songLength.load();++i){if(i)text+=", ";text+=juce::String(processor.sequence.song[i].load()+1);} if(text!=chain.getText())chain.setText(text,false); }
    repaint(scaled(1060,100,180,36));
}
bool PulseEditor::keyPressed(const juce::KeyPress& k){const auto text=juce::String("asdfghj");auto c=juce::CharacterFunctions::toLowerCase(k.getTextCharacter());int i=text.indexOfChar(c);if(i>=0){processor.audition(i);return true;}if(k.getKeyCode()==juce::KeyPress::spaceKey&&run.isEnabled()){run.triggerClick();return true;}return false;}
void PulseEditor::fileDialog(bool writing,bool kit){
    chooser=std::make_unique<juce::FileChooser>(writing?"Save Pulse Foundry session":"Open Pulse Foundry session",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(kit?"Init.pfkit":"Session.pfproject"),kit?"*.pfkit":"*.pfproject");
    auto flags=writing?(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::warnAboutOverwriting):(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles);
    chooser->launchAsync(flags,[safe=juce::Component::SafePointer<PulseEditor>(this),writing,kit](const juce::FileChooser& f){if(!safe)return;auto file=f.getResult();if(file==juce::File{})return;if(writing)file=file.withFileExtension(kit?"pfkit":"pfproject");juce::String err;bool ok=writing?safe->processor.saveProject(file,kit,err):safe->processor.loadProject(file,kit,err);safe->status.setText(ok?(writing?"Saved ":"Loaded ")+file.getFileName():err,juce::dontSendNotification);if(ok&&!writing){safe->bindKnobs();safe->updateInspector();}});
}
