// SPDX-License-Identifier: GPL-3.0-only
#include "Engine.h"
#include <fstream>
#include <iostream>
static void w16(std::ofstream& f,uint16_t x){f.put(char(x));f.put(char(x>>8));}static void w32(std::ofstream& f,uint32_t x){w16(f,uint16_t(x));w16(f,uint16_t(x>>16));}
int main(int argc,char** argv){auto s=std::make_unique<pf::Sequence>();for(int t=0;t<6;++t)s->euclidean(0,t,16,t==0?4:t==3?2:t==5?8:3,t);pf::Engine e(*s);e.prepare(48000);e.beginBlock({true,true,0,120},true,.12f);std::ofstream out(argc>1?argv[1]:"PulseFoundry-demo.wav",std::ios::binary);constexpr int n=48000*8;out.write("RIFF",4);w32(out,36+n*4);out.write("WAVEfmt ",8);w32(out,16);w16(out,1);w16(out,2);w32(out,48000);w32(out,192000);w16(out,4);w16(out,16);out.write("data",4);w32(out,n*4);for(int i=0;i<n;++i){auto a=e.nextSample();w16(out,uint16_t(int16_t(a[0]*24575)));w16(out,uint16_t(int16_t(a[1]*24575)));}std::cout<<"Rendered 8-second original-firmware groove\n";}
