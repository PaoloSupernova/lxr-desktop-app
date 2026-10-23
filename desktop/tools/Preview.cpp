// SPDX-License-Identifier: GPL-3.0-only
#include "Processor.h"
#include <iostream>
int main(int argc,char** argv){juce::ScopedJuceInitialiser_GUI gui;auto p=std::make_unique<PulseProcessor>();auto editor=std::unique_ptr<juce::AudioProcessorEditor>(p->createEditor());editor->setSize(1280,900);auto image=editor->createComponentSnapshot(editor->getLocalBounds());juce::PNGImageFormat png;juce::File output(juce::String::fromUTF8(argc>1?argv[1]:"/tmp/PulseFoundry-preview.png"));auto stream=output.createOutputStream();if(!stream)return 1;stream->setPosition(0);stream->truncate();png.writeImageToStream(image,*stream);std::cout<<output.getFullPathName()<<'\n';}
