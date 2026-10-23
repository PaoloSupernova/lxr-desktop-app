// SPDX-License-Identifier: GPL-3.0-only
#include "Engine.h"
#include <iostream>
#include <cmath>
int main() {
    auto sequence=std::make_unique<pf::Sequence>(); pf::Engine engine(*sequence);
    for(const auto& spec:pf::parameterSpecs()) for(int value:{0,spec.maximum}) {
        engine.prepare(48000); engine.setParameter(spec.id,value);
        engine.beginBlock({false,false,0,120},false,0);
        for(int track=0;track<7;++track) engine.hit(track);
        for(int sample=0;sample<512;++sample) {
            auto output=engine.nextSample();
            for(auto v:output) if(!std::isfinite(v)) { std::cerr<<spec.name<<" produced non-finite audio\n"; return 1; }
        }
    }
    std::cout<<"PASS: all exposed parameter boundaries produce finite audio\n";
}
