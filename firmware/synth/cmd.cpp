#include "cmd.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

namespace cmd {

namespace {

// Splits `line` into up to 4 lowercase tokens. Returns the token count.
int tokenize(const char* line, char tokens[][24], int maxTokens) {
  int count = 0;
  const char* p = line;
  while (*p && count < maxTokens) {
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) break;
    int n = 0;
    while (*p && !isspace((unsigned char)*p) && n < 23) {
      tokens[count][n++] = (char)tolower((unsigned char)*p);
      p++;
    }
    tokens[count][n] = '\0';
    while (*p && !isspace((unsigned char)*p)) p++;  // skip overlong tails
    count++;
  }
  return count;
}

bool parseNumber(const char* s, float *out) {
  char *end = nullptr;
  float v = strtof(s, &end);
  if (end == s || *end != '\0') return false;
  *out = v;
  return true;
}

bool parseVoice(const char* s, int *out) {
  float v;
  if (!parseNumber(s, &v)) return false;
  int n = (int)v;
  if ((float)n != v || n < 1 || n > 4) return false;
  *out = n - 1;
  return true;
}

Command fail(const char** error, const char* reason) {
  if (error) *error = reason;
  Command c;
  c.type = Type::Invalid;
  return c;
}

}  // namespace

Command parse(const char* line, const char** error) {
  Command c;
  if (error) *error = nullptr;

  char tokens[4][24];
  int n = tokenize(line, tokens, 4);
  if (n == 0) return c;  // None

  const char* verb = tokens[0];

  if (!strcmp(verb, "help")) { c.type = Type::Help; return c; }
  if (!strcmp(verb, "status")) { c.type = Type::Status; return c; }
  if (!strcmp(verb, "pots")) { c.type = Type::Pots; return c; }
  if (!strcmp(verb, "audio")) { c.type = Type::Audio; return c; }
  if (!strcmp(verb, "codec")) { c.type = Type::Codec; return c; }

  if (!strcmp(verb, "on") || !strcmp(verb, "off") || !strcmp(verb, "sel") || !strcmp(verb, "free")) {
    if (n != 2) return fail(error, "usage: <cmd> <voice 1-4>");
    if (!parseVoice(tokens[1], &c.voice)) return fail(error, "voice must be 1-4");
    c.type = !strcmp(verb, "on")    ? Type::On
           : !strcmp(verb, "off")   ? Type::Off
           : !strcmp(verb, "sel")   ? Type::Select
                                    : Type::Free;
    return c;
  }

  if (!strcmp(verb, "wave")) {
    if (n != 3) return fail(error, "usage: wave <voice 1-4> saw|square|tri");
    if (!parseVoice(tokens[1], &c.voice)) return fail(error, "voice must be 1-4");
    if (!strcmp(tokens[2], "saw")) c.wave = 0;
    else if (!strcmp(tokens[2], "square") || !strcmp(tokens[2], "sqr")) c.wave = 1;
    else if (!strcmp(tokens[2], "tri") || !strcmp(tokens[2], "triangle")) c.wave = 2;
    else return fail(error, "wave must be saw, square or tri");
    c.type = Type::Wave;
    return c;
  }

  if (!strcmp(verb, "freq") || !strcmp(verb, "note") || !strcmp(verb, "level") || !strcmp(verb, "detune")) {
    if (n != 3) return fail(error, "usage: <cmd> <voice 1-4> <value>");
    if (!parseVoice(tokens[1], &c.voice)) return fail(error, "voice must be 1-4");
    if (!parseNumber(tokens[2], &c.value)) return fail(error, "value must be a number");

    if (!strcmp(verb, "freq")) {
      if (c.value < 1.0f || c.value > 12000.0f) return fail(error, "freq must be 1-12000 Hz");
      c.type = Type::Freq;
    } else if (!strcmp(verb, "note")) {
      if (c.value < 0.0f || c.value > 127.0f) return fail(error, "note must be 0-127");
      c.type = Type::Note;
    } else if (!strcmp(verb, "level")) {
      if (c.value < 0.0f || c.value > 1.0f) return fail(error, "level must be 0-1");
      c.type = Type::Level;
    } else {
      if (c.value < -100.0f || c.value > 100.0f) return fail(error, "detune must be -100..100 cents");
      c.type = Type::Detune;
    }
    return c;
  }

  if (!strcmp(verb, "sync")) {
    if (n != 2) return fail(error, "usage: sync phase|tempo");
    if (!strcmp(tokens[1], "phase")) c.flag = false;
    else if (!strcmp(tokens[1], "tempo")) c.flag = true;
    else return fail(error, "sync must be phase or tempo");
    c.type = Type::Sync;
    return c;
  }

  if (!strcmp(verb, "pitch")) {
    if (n != 2) return fail(error, "usage: pitch notes|hz");
    if (!strcmp(tokens[1], "notes")) c.flag = false;
    else if (!strcmp(tokens[1], "hz")) c.flag = true;
    else return fail(error, "pitch must be notes or hz");
    c.type = Type::Pitch;
    return c;
  }

  return fail(error, "unknown command (try: help)");
}

}  // namespace cmd
