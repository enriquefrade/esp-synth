#pragma once

// Serial command parser. No Arduino dependencies, so it can be tested on the host.
// Commands are one per line. Voices are numbered 1-4 in the protocol.

namespace cmd {

enum class Type {
  None,
  Help,
  Status,
  On,       // on <v>
  Off,      // off <v>
  Wave,     // wave <v> saw|square|tri
  Freq,     // freq <v> <hz>        (override pot)
  Note,     // note <v> <midi>      (override pot)
  Level,    // level <v> <0..1>     (override pot)
  Detune,   // detune <v> <cents>   (override pot)
  Select,   // sel <v>
  Sync,     // sync phase|tempo
  Pitch,    // pitch notes|hz
  Free,     // free <v>             (clear overrides, pots control again)
  Pots,     // pots                 raw 12-bit pot readings
  Audio,    // audio                codec write stats
  Codec,    // codec                ES8311 register dump + PA pin
  Invalid,
};

struct Command {
  Type type = Type::None;
  int voice = -1;        // 0-based, -1 if not used
  int wave = -1;         // 0 saw, 1 square, 2 triangle
  float value = 0.0f;
  bool flag = false;     // sync: tempo; pitch: hz
};

// Parses one line. Returns Type::None for an empty line.
// On a bad command, returns Type::Invalid and writes a reason to `error` (if non-null).
Command parse(const char* line, const char** error = nullptr);

}  // namespace cmd
