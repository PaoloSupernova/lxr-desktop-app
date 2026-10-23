// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "Parameters.h"
namespace pf {
static_assert(std::atomic<uint64_t>::is_always_lock_free, "The desktop sequencer requires lock-free 64-bit atomics.");
constexpr int voices=6, tracks=7, patterns=64, steps=64;
constexpr double nativeRate=44140.0;
struct ParameterSpec { int id; std::string name; int maximum; int initial; };
const std::vector<ParameterSpec>& parameterSpecs();
int safeParameter(int id, int value);
struct Step {
    bool on=false; int velocity=100, probability=100, note=60, flam=0, shift=0;
    int lock1=0, value1=0, lock2=0, value2=0;
    uint64_t pack() const;
    static Step unpack(uint64_t);
};
struct Pattern {
    std::array<std::array<std::atomic<uint64_t>,steps>,tracks> data{};
    std::array<std::atomic<int>,tracks> length{}, rotation{}, mute{};
    Pattern();
};
struct Sequence {
    std::array<Pattern,patterns> bank;
    std::array<std::atomic<int>,64> song{};
    std::atomic<int> songLength{1}, songEnabled{0}, selected{0};
    std::atomic<int> playhead{-1}, soundingPattern{0};
    std::array<std::atomic<int>,tracks> trackHead{};
    Sequence();
    void euclidean(int pattern, int track, int length, int hits, int rotation);
};
struct Transport { bool playing=false; bool positioned=false; double ppq=0, bpm=120; };
struct Hit { int track=0, velocity=100, note=60; };
class Engine {
public:
    explicit Engine(Sequence&);
    ~Engine();
    void prepare(double sampleRate);
    void setParameter(int id, int value);
    void beginBlock(Transport, bool patternEnabled, float swing);
    // Call before nextSample. Hits from MIDI take priority over sequence at the
    // same native block for the same physical voice; hats share one voice.
    void hit(int track, int velocity=100, int note=60);
    void panic();
    std::array<float,14> nextSample(); // master stereo + six dry voice stereo buses
    uint64_t triggerCount(int track) const { return triggerCounts[size_t(track)]; }
    double position() const { return ppq; }
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    Sequence& sequence;
    std::array<int,512> parameters{};
    std::array<uint64_t,tracks> triggerCounts{};
    std::array<Hit,tracks> pendingMidi{};
    std::array<bool,tracks> hasMidi{};
    double rate=48000, ppq=0, increment=0, phase=0;
    float shuffle=0;
    bool running=false, patternMode=true, wasRunning=false;
    int cursor=64;
    std::array<std::array<float,14>,64> audio{};
    std::array<float,14> left{}, right{};
    bool primed=false;
    std::array<int64_t,tracks> lastPulse{};
    int currentPattern=0;
    void renderNativeBlock();
    void advanceNative();
    void trigger(int track,int velocity,int note);
};
}
