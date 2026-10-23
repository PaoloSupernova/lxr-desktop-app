// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstdint>
#include <cstring>
#include <algorithm>
namespace pf {
inline int32_t __SSAT(double x, int bits) {
    const auto hi = (int64_t(1) << (bits-1))-1, lo = -(int64_t(1) << (bits-1));
    return static_cast<int32_t>(std::max(double(lo), std::min(double(hi), x)));
}
inline uint32_t __USAT(double x, int bits) { return uint32_t(std::max(0.0, std::min(double((uint64_t(1)<<bits)-1), x))); }
inline uint32_t __CLZ(uint32_t x) { uint32_t n=32; while(x) { --n; x >>= 1; } return n; }
inline uint32_t __PKHBT(uint32_t a, uint32_t b, int shift) { return (a & 65535u) | ((b << shift) & 0xffff0000u); }
inline uint32_t __QADD16(uint32_t a, uint32_t b) { return __PKHBT(uint16_t(__SSAT(int16_t(a)+int16_t(b),16)),uint16_t(__SSAT(int16_t(a>>16)+int16_t(b>>16),16)),16); }
inline uint32_t __QSUB16(uint32_t a, uint32_t b) { return __PKHBT(uint16_t(__SSAT(int16_t(a)-int16_t(b),16)),uint16_t(__SSAT(int16_t(a>>16)-int16_t(b>>16),16)),16); }
// Portable IEEE binary16 storage for the original delay RAM, including ties-to-even.
struct Half {
    uint16_t bits = 0;
    Half() = default;
    Half(float f) { *this = f; }
    Half& operator=(float f) {
        uint32_t u; std::memcpy(&u,&f,4);
        auto sign=uint16_t((u>>16)&0x8000u); int exp=int((u>>23)&255)-127+15; uint32_t m=u&0x7fffffu;
        if(exp>=31) bits=uint16_t(sign|0x7c00u|(m && exp==143 ? 0x200u:0));
        else if(exp<=0) {
            if(exp < -10) bits=sign;
            else { m|=0x800000u; int shift=14-exp; uint32_t q=m>>shift, r=m&((1u<<shift)-1); q += r>(1u<<(shift-1)) || (r==(1u<<(shift-1)) && (q&1)); bits=uint16_t(sign|q); }
        } else { uint32_t q=m>>13, r=m&8191; q += r>4096 || (r==4096 && (q&1)); bits=uint16_t(sign|(uint32_t(exp)<<10)); bits=uint16_t(bits+q); }
        return *this;
    }
    operator float() const {
        uint32_t sign=uint32_t(bits&0x8000)<<16, m=bits&1023, exp=(bits>>10)&31, u;
        if(exp==0) { if(m==0) u=sign; else { int e=-14; while(!(m&1024)) { m<<=1; --e; } u=sign|(uint32_t(e+127)<<23)|((m&1023)<<13); } }
        else if(exp==31) u=sign|0x7f800000u|(m<<13);
        else u=sign|((exp+112)<<23)|(m<<13);
        float f; std::memcpy(&f,&u,4); return f;
    }
};
}
