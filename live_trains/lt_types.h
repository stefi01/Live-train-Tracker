#pragma once
#include <stdint.h>

#define MAX_T 40
#define MAX_TL 8
#define MAX_TN 18

struct Stop {
  const char *code, *city;
  float lat, lon;
};

struct Train {
  char num[12];
  char route[28];
  char orig[24];
  char dest[24];
  char next[24];
  char prov[8];
  char head[4];
  char state[16];
  char stops[16][5];
  uint8_t nStops;
  float lat, lon;
  int mph;
  int lateMin;
};

struct TubeNode {
  char code[5];
  char name[18];
  float lat, lon;
  int x, y;
};

struct TubeLine {
  char name[28];
  uint16_t col;
  int ang;
  int off;
  TubeNode n[MAX_TN];
  int nn;
};

#define RAIL_END 999.0f

struct RailPt {
  float lat, lon;
  const char *name;
};

struct TagBox {
  int x, y, w, h;
};
