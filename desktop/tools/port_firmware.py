#!/usr/bin/env python3
"""Generate an instance-owned C++ DSP class from unchanged GPL firmware.
No text substitution is performed in the upstream tree. See docs/PORT.md.
"""
import pathlib, re, sys
root = pathlib.Path(__file__).resolve().parents[2]
out = pathlib.Path(sys.argv[1])
inc = root / 'Core/Inc'
src = root / 'Core/Src'
def clean(s):
    s = re.sub(r'//[^\n]*|/\*.*?\*/', '', s, flags=re.S)
    s = re.sub(r'^\s*#include[^\n]*', '', s, flags=re.M)
    return s
# Headers are flattened in dependency order. Their types are nested in Firmware;
# declarations are removed because the original definitions become inline members.
headers = ['Soundengine/config.h','Soundengine/datatypes.h','Parameters.h',
 'Soundengine/MIDI/MidiMessages.h','Soundengine/MIDI/ParameterArray.h',
 'Soundengine/DSPAudio/1PoleLp.h','Soundengine/DSPAudio/distortion.h',
 'Soundengine/DSPAudio/ResonantFilter.h','Soundengine/DSPAudio/Oscillator.h',
 'Soundengine/DSPAudio/Decay.h','Soundengine/DSPAudio/SlopeEg2.h',
 'Soundengine/DSPAudio/modulationNode.h','Soundengine/DSPAudio/lfo.h',
 'Soundengine/DSPAudio/transientGenerator.h','Soundengine/DSPAudio/snapEg.h',
 'Soundengine/DSPAudio/dither.h','Soundengine/DSPAudio/BufferTools.h','Soundengine/DSPAudio/DrumVoice.h',
 'Soundengine/DSPAudio/Snare.h','Soundengine/DSPAudio/CymbalVoice.h',
 'Soundengine/DSPAudio/HiHat.h','Soundengine/DSPAudio/FX/FxRamBuffer.h',
 'Soundengine/DSPAudio/FX/Distortion.h','Soundengine/DSPAudio/FX/Ringmod.h',
 'Soundengine/DSPAudio/FX/Compressor.h','Soundengine/DSPAudio/FX/Delay.h',
 'Soundengine/DSPAudio/mixer.h','Soundengine/FxBus.h']
pre = '''// GENERATED from Erica Synths firmware. GPL-3.0-only; see LICENSE and NOTICE.md.
// Modified desktop port, 2026-10-23. Regenerate with tools/port_firmware.py.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <climits>
#include <limits>
#include "PortableArm.h"
namespace pf {
'''
# All readonly sample/wave tables are shared, all mutable state belongs to Firmware.
tables = ''
for name in ['wavetable','transientTables']:
    tables += clean((inc/f'Soundengine/DSPAudio/{name}.h').read_text())
    tables += clean((src/f'Soundengine/DSPAudio/{name}.c').read_text())
tables = re.sub(r'\b(?:INDTCMZ?|IN_ITCM_RAM)\b', '', tables)
# Original headers use extern; the definitions are included immediately below.
tables = re.sub(r'(?:const\s+)?extern\s+[^;]+;', '', tables)
tables += clean((inc/'Soundengine/MIDI/MidiNoteNumbers.h').read_text())
tables += clean((inc/'Preset/InitKit.h').read_text())
tables = re.sub(r'(?:const\s+)?extern\s+[^;]+;', '', tables)
tables += clean((src/'Preset/initKit.c').read_text())
# C hex byte literals use modulo conversion to signed int8; make it explicit.
tables = re.sub(r'0x([89a-fA-F][0-9a-fA-F])\b', lambda m: str(int(m.group(1),16)-256), tables)
body = ''
for name in headers:
    t = clean((inc/name).read_text())
    t = re.sub(r'(?:const\s+)?extern\s+[^;]+;', '', t)
    t = re.sub(r'^\s*(?:[\w*]+\s+)+\w+\([^;{}]*\);', '', t, flags=re.M)
    t = re.sub(r'^\s*#define\s+(?:true|false|INDTCM|INDTCMZ|IN_ITCM_RAM)\b[^\n]*', '', t, flags=re.M)
    body += '\n// ' + name + '\n' + t
# Glue replaces physical I/O and clock calls. Sound parameter parsing is original.
body += '''
#define CC_2 MIDI_CC2
#define SEQ_DEFAULT_NOTE 60
#define FILTER_SHAPER -0.9f
#define SEMITONE_UP 1.0594630943592952645618252949463f
uint8_t activeParameters[512]{};
uint8_t midiParser_originalCcValues[512]{};
uint16_t midiParser_activeNrpnNumber = 0;
uint8_t mixer_audioRouting[6]{};
float mixer_decimation_rate[7] = {1,1,1,1,1,1,1};
float mixer_decimation_cnt[6]{};
int16_t mixer_voice_samples[6]{};
uint16_t tempo = 120;
uint16_t seq_getBpm() { return tempo; }
void seq_setMute(uint8_t, uint8_t) {} // Desktop sequencer owns track mute.
float adc_getFaderVolume(uint8_t) { return 1.0f; }
uint32_t rng = 0x9e3779b9;
uint32_t GetRngValue() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }
'''
files = ['1PoleLp','BufferTools','Oscillator','Decay','SlopeEg2','snapEg','dither',
 'distortion','ResonantFilter','transientGenerator','DrumVoice','Snare','CymbalVoice','HiHat',
 'lfo','modulationNode','FX/FxRamBuffer','FX/Distortion','FX/Ringmod','FX/Compressor','FX/Delay']
for name in files:
    body += '\n// Core/Src/Soundengine/DSPAudio/' + name + '.c\n' + clean((src/f'Soundengine/DSPAudio/{name}.c').read_text())
body += clean((src/'Soundengine/FxBus.c').read_text())
body += clean((src/'Soundengine/MIDI/ParameterArray.c').read_text())
# Only the CC-to-sound-parameter portion is needed; UART, routing and MTC excluded.
midi = clean((src/'Soundengine/MIDI/MidiParser.c').read_text())
a = midi.index('static inline float calcPitchModAmount')
b = midi.index('uint8_t midi_MidiChannels', a)
body += midi[a:b]
a = midi.index('float midiParser_calcDetune')
b = midi.index('void midiParser_checkMtc', a)
body += midi[a:b]
body += clean((inc/'Soundengine/MIDI/valueShaper.h').read_text())
# Hardware LCD notification has no sound side effects.
body = re.sub(r'if\(menu_activePage == FX_MODE_PAGE\)\s*\{.*?\}', '', body, flags=re.S)
body = re.sub(r'^\s*#pragma[^\n]*', '', body, flags=re.M)
# C99 runtime-sized scratch arrays become fixed stack buffers for MSVC and RT safety.
body = re.sub(r'\[size\]', '[OUTPUT_DMA_SIZE]', body)
# These are member functions and member variables, never process-wide statics.
body = re.sub(r'\b(?:INDTCMZ?|IN_ITCM_RAM|__inline|inline|static|register)\b', '', body)
body = body.replace('__fp16', 'Half')
body = body.replace('transientData[transient->waveform-2][phase]<<8', 'int32_t(transientData[transient->waveform-2][phase])*256')
body = body.replace('transientData[transient->waveform-1][phase]<<8', 'int32_t(transientData[transient->waveform-1][phase])*256')
# Shared, immutable distortion lookup tables in class use C++17 inline static.
body = re.sub(r'(?m)^\s*const (float|double) (\w+)\[(\d+)\]\s*=', r'inline static const \1 \2[\3] =', body)
body = body.replace('const double DC_OFFSET =', 'inline static constexpr double DC_OFFSET =')
# Scalar-only calcNoise is unused by block renderer, replace ARM APSR read.
body = body.replace('APSR_Type apsr;\n\tapsr.w = __get_APSR();', 'struct { struct { bool V = false; } b; } apsr;')
# Fix upstream interpolation reading element 1024 past a 1024-entry table.
body = re.sub(r'((?:table|sawTable)\[osc->tableOffset\])\[(itg2?)(\+\+)?\]', lambda m: m.group(1)+'[(('+m.group(2)+(m.group(3) or '')+') & 1023u)]', body)
# Original bare array definitions are value-initialized with Firmware{}.
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(pre + tables + '\nstruct Firmware {\n' + body + '\n};\n} // namespace pf\n')
# Expose enum names, safe ranges and init kit values to the desktop parameter model.
params = clean((inc/'Parameters.h').read_text())
params = re.sub(r'^\s*#(?:ifndef|define PARAMETERS_H_|endif)[^\n]*', '', params, flags=re.M)
out.with_name('Parameters.h').write_text('#pragma once\n#include <cstdint>\nnamespace pf {\n'+params+'\n}\n')
print('Generated', out)
# Generate readable parameter descriptors and conservative enum ranges.
enum = re.search(r'enum ParamEnums\s*\{(.*?)\};', params, re.S).group(1)
entries=[]; values={}; current=-1
for item in enum.split(','):
    item=item.strip()
    if not item: continue
    parts=item.split('='); name=parts[0].strip()
    if len(parts)>1: current=int(parts[1].strip()) if parts[1].strip().isdigit() else values[parts[1].strip()]
    else: current+=1
    values[name]=current
    if name=='END_OF_SOUND_PARAMETERS': break
    entries.append((name,current))
kit = [int(x,16) for x in re.findall(r'0x([0-9a-fA-F]+)', clean((src/'Preset/initKit.c').read_text()))]
assert values['END_OF_SOUND_PARAMETERS']==247, 'Review desktop parameter bounds for changed firmware'
specs=[]; seen=set()
for name,idx in entries:
    if name=='PAR_MOD_WHEEL': continue
    if idx in seen: continue
    seen.add(idx)
    # Exclude protocol placeholders and physical outputs; desktop always supplies
    # a full master plus independent stems. Morph kit destination has no meaning.
    if idx==0 or any(x in name for x in ['NRPN','UNUSED','AUDIO_OUT','FX_ROUTING','MORPH_KIT','FX_REVERB','VOICE_LFO']): continue
    maxval=127
    if 'WAVE' in name and 'TRANS' not in name and 'LFO' not in name: maxval=5
    if 'WAVE_LFO' in name: maxval=7
    if re.search(r'TRANS\d_WAVE',name): maxval=13 # 12 samples plus snap and offset modes
    if 'FILTER_TYPE' in name: maxval=8
    if 'MIX_MOD' in name or 'VOLUME_MOD_ON' in name: maxval=1
    if 'SYNC_LFO' in name: maxval=11
    if 'RETRIGGER_LFO' in name: maxval=7
    if name=='PAR_FX_TYPE': maxval=4
    if name=='PAR_FX_DRY_WET': maxval=100
    if name=='PAR_FX_DELAY_TYPE': maxval=1
    if name=='PAR_FX_RATIO': maxval=5
    if name=='PAR_FX_DIST_TYPE': maxval=2
    if 'VEL_DEST' in name or 'TARGET_LFO' in name: maxval=245 # desktop uses sound parameter IDs directly
    val=kit[idx+8] if idx+8<len(kit) else 0
    specs.append((idx,name[4:].replace('_',' ').title(),maxval,min(val,maxval)))
# C++17 std::vector descriptors; host and tests use the same safety ranges.
out.with_name('ParameterSpecs.inc').write_text('\n'.join('{%d, "%s", %d, %d},'%s for s in specs)+'\n')
