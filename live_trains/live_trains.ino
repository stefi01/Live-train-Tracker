/*
  Live Trains — 7 in 8048S070C. New product. Do not edit Home Field Scope.
  USA Amtrak / VIA / Brightline, plus LIRR and Metro-North near New York.
  SEPTA, MARC, VRE, and Tri-Rail load only when home is a station on that line.
  LIRR / MNR use the public MTA feeds. No account and no key.
  Home is a station (FNO Fresno default). Set it to NYP, GCT, JAM, CHI, LAX, etc.
  JOIN WIFI same glass keyboard as the plane box.
*/

#include "lt_types.h"
#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <Update.h>
#include <Preferences.h>
#include "esp_ota_ops.h"
#include <SPI.h>
#include <SD.h>+
#include <Wire.h>
#include <TJpg_Decoder.h>
#include <math.h>
#include <string.h>
#include "lt_mta.h"
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define GFX_BL 2
#define PW 800
#define PH 480
#define TOUCH_SDA 19
#define TOUCH_SCL 20
#define TOUCH_RST 38
#define GT911_ADDR 0x5D
#define LT_VER "1.19"
#ifndef LT_GH_USER
#define LT_GH_USER "stefi01"
#endif
#ifndef LT_GH_REPO
#define LT_GH_REPO "Live-train-Tracker"
#endif
#ifndef SD_CS
#define SD_CS 10
#endif
#ifndef SD_MOSI
#define SD_MOSI 11
#endif
#ifndef SD_SCK
#define SD_SCK 12
#endif
#ifndef SD_MISO
#define SD_MISO 13
#endif

enum {
  PG_HOME = 0,
  PG_WIFI,
  PG_LIST,
  PG_DETAIL,
  PG_DEPOT,
  PG_CLOCK,
  PG_LINE,
  PG_SETTINGS,
  PG_ABOUT
};

Arduino_ESP32RGBPanel *rgbBus = nullptr;
Arduino_RGB_Display *gfx = nullptr;
Preferences prefs;
int page = PG_HOME;

char wifiSsid[64] = "";
char wifiPass[64] = "";
char wifiScan[16][33];
int nWifiScan = 0, wifiScanTop = 0, wifiFocus = 0;
bool wifiShift = false, wifiSym = false, wifiScanBusy = false;
char wifiMsg[28] = "";
char mtaKey[52] = "";
char mtaEdit[52] = "";
bool mtaKeyEdit = false;
uint32_t lastTouch = 0;
bool wifiOk = false;

float homeLat = 36.7381f;
float homeLon = -119.7856f;
float viewLat = 36.7381f;
float viewLon = -119.7856f;
int mapOx = 0, mapOy = 0;
char homeStop[8] = "FNO";
int rangeNm = 40;
bool mapOn = true, mapOk = false, dayMode = false;
int mapStyle = 1, mapHaveStyle = -1, mapNm = 80, blLevel = 2, setPane = 1, fieldPick = 0;
int hist[8], nHist = 0;
float pinchDist = 0;
int geoEdit = 0;
char geoBuf[18] = "";

uint16_t *mapFb = nullptr;
uint16_t *jpgSrc = nullptr;
const int SRC_W = 320, SRC_H = 176;
const int MAP_H = 352, ZOOM_Y = 352, MENU_Y = 400, KW = 80;
const int EDGE = 2;
const int CX = 400, CY = 176, RR = 160;
int mapSrcW = SRC_W, mapSrcH = SRC_H;

uint32_t lastFetch = 0, lastMap = 0, lastMapTry = 0, zoomNeed = 0, lastClock = 0;
float sweepDeg = 0, prevSweepDeg = 0;
uint32_t lastSweep = 0;

enum { NET_NONE = 0, NET_TRAINS, NET_MAP, NET_OTA };
volatile int netJob = NET_NONE, netPend = NET_NONE, netDoneJob = 0;
int dlBurst = 0;
char otaLatest[12] = "";
char otaMsg[44] = "";
volatile int otaPct = -1;
volatile bool otaBusy = false;
bool otaNewer = false;
uint8_t otaKind = 0;
uint32_t lastOtaDraw = 0;
bool sdOk = false;
volatile bool netBusy = false, netDone = false;

#define RGB565(r, g, b) (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b) >> 3))
uint16_t COL_BG, COL_DIM, COL_MID, COL_HOT, COL_TXT, COL_WHT, COL_AMB, COL_CYN, COL_YEL, COL_MAG;
const uint16_t COL_BLK = RGB565(0, 0, 0);
const uint16_t COL_ON = RGB565(80, 220, 120);
const uint16_t COL_LATE = RGB565(255, 120, 70);

const Stop stops[] = {
    {"FNO", "Fresno", 36.7381f, -119.7856f},
    {"BFD", "Bakersfield", 35.3729f, -119.0081f},
    {"HNF", "Hanford", 36.3275f, -119.6456f},
    {"MDR", "Madera", 36.9570f, -120.0610f},
    {"MER", "Merced", 37.3074f, -120.4803f},
    {"SAC", "Sacramento", 38.5840f, -121.5006f},
    {"OKJ", "Oakland", 37.7936f, -122.2714f},
    {"SFA", "San Francisco", 37.7763f, -122.3950f},
    {"EMY", "Emeryville", 37.8404f, -122.2918f},
    {"OAC", "Oakland Coliseum", 37.7525f, -122.1978f},
    {"BKY", "Berkeley", 37.8674f, -122.3000f},
    {"RIC", "Richmond CA", 37.9368f, -122.3535f},
    {"MTZ", "Martinez", 38.0190f, -122.1380f},
    {"SUI", "Suisun", 38.2432f, -122.0408f},
    {"DAV", "Davis", 38.5436f, -121.7378f},
    {"SKN", "Stockton", 37.9570f, -121.2780f},
    {"MOD", "Modesto", 37.6688f, -120.9940f},
    {"TRK", "Turlock", 37.4890f, -120.8480f},
    {"HAC", "Hayward", 37.6564f, -122.0952f},
    {"FMT", "Fremont", 37.5585f, -121.9968f},
    {"SJC", "San Jose", 37.3302f, -121.9026f},
    {"LAX", "Los Angeles", 34.0562f, -118.2346f},
    {"FUL", "Fullerton", 33.8687f, -117.9226f},
    {"ANA", "Anaheim", 33.8033f, -117.8765f},
    {"SAN", "San Diego", 32.7160f, -117.1650f},
    {"OLT", "Old Town", 32.7552f, -117.1995f},
    {"SOL", "Solana Beach", 32.9928f, -117.2712f},
    {"OSD", "Oceanside", 33.1924f, -117.3792f},
    {"SNC", "San Juan Cap", 33.5017f, -117.6636f},
    {"IRV", "Irvine", 33.6568f, -117.7336f},
    {"SNA", "Santa Ana", 33.7500f, -117.8560f},
    {"OXN", "Oxnard", 34.2292f, -119.1574f},
    {"VEC", "Ventura", 34.2769f, -119.2994f},
    {"SBA", "Santa Barbara", 34.4137f, -119.6902f},
    {"GAC", "Goleta", 34.4356f, -119.8430f},
    {"VAN", "Van Nuys", 34.1863f, -118.4487f},
    {"BUR", "Burbank", 34.1781f, -118.3516f},
    {"SEA", "Seattle", 47.5985f, -122.3280f},
    {"PDX", "Portland", 45.5189f, -122.6763f},
    {"EUG", "Eugene", 44.0551f, -123.0922f},
    {"DEN", "Denver", 39.7531f, -105.0002f},
    {"SLC", "Salt Lake", 40.7616f, -111.9080f},
    {"PHX", "Phoenix", 33.4489f, -112.0960f},
    {"TUS", "Tucson", 32.2236f, -110.9668f},
    {"ABQ", "Albuquerque", 35.0820f, -106.6480f},
    {"ELP", "El Paso", 31.7578f, -106.4950f},
    {"CHI", "Chicago", 41.8786f, -87.6400f},
    {"GLN", "Glenview", 42.0745f, -87.8056f},
    {"NPV", "Naperville", 41.7796f, -88.1458f},
    {"MSP", "St Paul", 44.9470f, -93.0860f},
    {"MKE", "Milwaukee", 43.0344f, -87.9170f},
    {"STL", "St Louis", 38.6250f, -90.2030f},
    {"KCY", "Kansas City", 39.0850f, -94.5850f},
    {"OMA", "Omaha", 41.2496f, -95.9270f},
    {"DFW", "Dallas", 32.7763f, -96.8075f},
    {"HOU", "Houston", 29.7674f, -95.3677f},
    {"AUS", "Austin", 30.2694f, -97.7569f},
    {"SAS", "San Antonio", 29.4194f, -98.4781f},
    {"NOL", "New Orleans", 29.9460f, -90.0780f},
    {"MEM", "Memphis", 35.1324f, -90.0593f},
    {"ATL", "Atlanta", 33.7990f, -84.3920f},
    {"BNA", "Nashville", 36.1670f, -86.7810f},
    {"CLT", "Charlotte", 35.2410f, -80.8220f},
    {"RMT", "Rocky Mount", 35.9380f, -77.7980f},
    {"JAX", "Jacksonville", 30.3650f, -81.7240f},
    {"ORL", "Orlando", 28.5260f, -81.3810f},
    {"TPA", "Tampa", 27.9520f, -82.4500f},
    {"MIA", "Miami", 25.8495f, -80.2570f},
    {"MIC", "Miami Airport", 25.79667f, -80.26139f},
    {"NYP", "New York", 40.7506f, -73.9936f},
    {"GCT", "Grand Central", 40.752998f, -73.977056f},
    {"JAM", "Jamaica", 40.699608f, -73.808530f},
    {"NWK", "Newark", 40.7342f, -74.1642f},
    {"EWR", "Newark Airport", 40.7044f, -74.1907f},
    {"MET", "Metropark", 40.5682f, -74.3296f},
    {"TRE", "Trenton", 40.2187f, -74.7541f},
    {"PHL", "Philadelphia", 39.9556f, -75.1820f},
    {"WIL", "Wilmington", 39.7370f, -75.5510f},
    {"WAS", "Washington", 38.8970f, -77.0063f},
    {"BAL", "Baltimore", 39.3074f, -76.6156f},
    {"BWI", "BWI Airport", 39.1924f, -76.6944f},
    {"NHV", "New Haven", 41.2975f, -72.9267f},
    {"STM", "Stamford", 41.0468f, -73.5422f},
    {"BOS", "Boston", 42.3523f, -71.0553f},
    {"BBY", "Boston Back Bay", 42.3473f, -71.0755f},
    {"PVD", "Providence", 41.8293f, -71.4135f},
    {"ALB", "Albany", 42.6510f, -73.7450f},
    {"BUF", "Buffalo", 42.8780f, -78.8730f},
    {"CLE", "Cleveland", 41.5000f, -81.6960f},
    {"PIT", "Pittsburgh", 40.4450f, -80.0020f},
    {"DET", "Detroit", 42.3679f, -83.0640f},
    {"TOL", "Toledo", 41.6380f, -83.5410f},
    {"CIN", "Cincinnati", 39.1100f, -84.5370f},
    {"IND", "Indianapolis", 39.7620f, -86.1600f},
    {"RVM", "Richmond VA", 37.5340f, -77.4290f},
    {"NFK", "Norfolk", 36.8430f, -76.2760f},
    {"FTW", "Fort Worth", 32.7520f, -97.3270f},
    {"LIT", "Little Rock", 34.7440f, -92.2870f},
    {"BHM", "Birmingham", 33.5120f, -86.8070f},
    {"SAV", "Savannah", 32.0810f, -81.0910f},
    {"RGH", "Raleigh", 35.7750f, -78.6440f},
    {"SYR", "Syracuse", 43.0510f, -76.1540f},
    {"ROC", "Rochester", 43.1540f, -77.6080f},
    {"RNO", "Reno", 39.5280f, -119.8120f},
    {"SPK", "Spokane", 47.6580f, -117.4260f},
    {"SLO", "San Luis Obispo", 35.2760f, -120.6550f},
    {"FLG", "Flagstaff", 35.1980f, -111.6510f},
    {"JAN", "Jackson MS", 32.2990f, -90.1910f},
    {nullptr, nullptr, 0, 0}};

Train trains[MAX_T];
int nTrains = 0, selected = -1, listTop = 0;
int tubeX[MAX_T], tubeY[MAX_T], tubeBX[MAX_T], tubeBY[MAX_T], tubeBW[MAX_T], tubeBH[MAX_T];
char trainsErr[28] = "";

const uint16_t TUBE_COL[] = {
    RGB565(220, 36, 31),
    RGB565(0, 25, 168),
    RGB565(0, 125, 50),
    RGB565(255, 206, 0),
    RGB565(155, 0, 88),
    RGB565(0, 152, 216),
    RGB565(178, 99, 0),
    RGB565(105, 80, 161)};
TubeLine tubeLines[MAX_TL];
int nTubeLines = 0;

void cols() {
  if (dayMode) {
    COL_BG = RGB565(232, 226, 214);
    COL_DIM = RGB565(210, 202, 186);
    COL_MID = RGB565(90, 94, 102);
    COL_HOT = RGB565(196, 132, 8);
    COL_TXT = RGB565(28, 30, 34);
    COL_WHT = RGB565(255, 255, 255);
    COL_AMB = RGB565(196, 100, 16);
    COL_CYN = RGB565(0, 90, 180);
    COL_YEL = RGB565(176, 120, 8);
    COL_MAG = RGB565(180, 40, 110);
  } else {
    COL_BG = RGB565(18, 22, 28);
    COL_DIM = RGB565(32, 40, 52);
    COL_MID = RGB565(200, 210, 220);
    COL_HOT = RGB565(255, 210, 40);
    COL_TXT = RGB565(250, 250, 250);
    COL_WHT = RGB565(255, 255, 255);
    COL_AMB = RGB565(255, 170, 40);
    COL_CYN = RGB565(40, 220, 255);
    COL_YEL = RGB565(255, 230, 60);
    COL_MAG = RGB565(255, 70, 180);
  }
}

void centre(const char *s, int cx, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  gfx->getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  gfx->setCursor(cx - (int)w / 2, y);
  gfx->print(s);
}

bool hit(int x, int y, int x0, int y0, int x1, int y1) {
  return x >= x0 && x <= x1 && y >= y0 && y <= y1;
}

void chip(int x, int y, int w, int h, const char *t, bool on) {
  gfx->fillRoundRect(x, y, w, h, 6, on ? COL_HOT : COL_DIM);
  gfx->setTextColor(on ? COL_BG : COL_TXT, on ? COL_HOT : COL_DIM);
  gfx->setTextSize(2);
  centre(t, x + w / 2, y + (h / 2) - 8);
}

void compassPill(int x, int y, const char *t) {
  gfx->fillRect(x - 14, y - 4, 28, 22, COL_BLK);
  gfx->setTextColor(COL_YEL, COL_BLK);
  gfx->setTextSize(2);
  centre(t, x, y);
}

void paintBg() {
  gfx->fillRect(EDGE, 0, PW - EDGE, PH, COL_BG);
}

void drawBackHome() {
  chip(8, 8, 140, 36, "< BACK", false);
  chip(652, 8, 140, 36, "HOME", true);
}

void statusLine(const char *s) {
  gfx->fillRect(80, 150, PW - 160, 36, COL_BG);
  gfx->setTextColor(COL_MID, COL_BG);
  gfx->setTextSize(2);
  centre(s, CX, 168);
  Serial.println(s);
}

bool wifiLinked() { return WiFi.status() == WL_CONNECTED; }

int findStop(const char *code) {
  for (int i = 0; stops[i].code; i++)
    if (!strcasecmp(stops[i].code, code)) return i;
  return -1;
}

void applyTz() {
  const char *tz = "PST8PDT,M3.2.0,M11.1.0";
  if (homeLon > -85.0f) tz = "EST5EDT,M3.2.0,M11.1.0";
  else if (homeLon > -102.0f) tz = "CST6CDT,M3.2.0,M11.1.0";
  else if (homeLon > -115.0f) tz = "MST7MDT,M3.2.0,M11.1.0";
  setenv("TZ", tz, 1);
  tzset();
}

void useStop(int i) {
  if (i < 0 || !stops[i].code) return;
  fieldPick = i;
  strlcpy(homeStop, stops[i].code, sizeof(homeStop));
  homeLat = stops[i].lat;
  homeLon = stops[i].lon;
  viewLat = homeLat;
  viewLon = homeLon;
  mapOx = mapOy = 0;
  applyTz();
}

void savePrefs() {
  prefs.begin("rails", false);
  prefs.putString("ssid", wifiSsid);
  prefs.putString("pass", wifiPass);
  prefs.putFloat("lat", homeLat);
  prefs.putFloat("lon", homeLon);
  prefs.putString("stop", homeStop);
  prefs.putInt("nm", rangeNm);
  prefs.putBool("map", mapOn);
  prefs.putInt("mapst", mapStyle);
  prefs.putBool("day", dayMode);
  prefs.putInt("bl", blLevel);
  prefs.putString("mtakey", mtaKey);
  prefs.end();
}

void loadPrefs() {
  prefs.begin("rails", true);
  strlcpy(wifiSsid, prefs.getString("ssid", "").c_str(), sizeof(wifiSsid));
  strlcpy(wifiPass, prefs.getString("pass", "").c_str(), sizeof(wifiPass));
  homeLat = prefs.getFloat("lat", 36.7381f);
  homeLon = prefs.getFloat("lon", -119.7856f);
  strlcpy(homeStop, prefs.getString("stop", "FNO").c_str(), sizeof(homeStop));
  rangeNm = prefs.getInt("nm", 40);
  mapOn = prefs.getBool("map", true);
  mapStyle = prefs.getInt("mapst", 1);
  dayMode = prefs.getBool("day", false);
  blLevel = prefs.getInt("bl", 2);
  strlcpy(mtaKey, prefs.getString("mtakey", "").c_str(), sizeof(mtaKey));
  prefs.end();
  if (rangeNm < 15) rangeNm = 15;
  if (rangeNm > 300) rangeNm = 300;
  int hit = findStop(homeStop);
  if (hit >= 0) fieldPick = hit;
  viewLat = homeLat;
  viewLon = homeLon;
  mapOx = mapOy = 0;
  applyTz();
  cols();
}

void applyBl() {
  static bool pwmOn = false;
  pinMode(GFX_BL, OUTPUT);
  if (blLevel >= 2) {
    if (pwmOn) {
      ledcDetach(GFX_BL);
      pwmOn = false;
      pinMode(GFX_BL, OUTPUT);
    }
    digitalWrite(GFX_BL, HIGH);
    return;
  }
  if (pwmOn) {
    ledcDetach(GFX_BL);
    pwmOn = false;
    pinMode(GFX_BL, OUTPUT);
  }
  digitalWrite(GFX_BL, HIGH);
  delay(30);
  int duty = blLevel <= 0 ? 120 : 190;
  analogWriteFrequency(GFX_BL, 1000);
  analogWrite(GFX_BL, duty);
  pwmOn = true;
}

void clearWifi() {
  wifiSsid[0] = wifiPass[0] = 0;
  wifiOk = false;
  savePrefs();
}

void factoryReset() {
  prefs.begin("rails", false);
  prefs.clear();
  prefs.end();
}

float distNm(float lat, float lon) {
  float dLat = (lat - homeLat) * 60.0f;
  float dLon = (lon - homeLon) * 60.0f * cosf(homeLat * 0.0174533f);
  return sqrtf(dLat * dLat + dLon * dLon);
}

float viewDist(float lat, float lon) {
  float dLat = (lat - viewLat) * 60.0f;
  float dLon = (lon - viewLon) * 60.0f * cosf(viewLat * 0.0174533f);
  return sqrtf(dLat * dLat + dLon * dLon);
}

float rangeDist(float lat, float lon) {
  float d = viewDist(lat, lon);
  if (strcasecmp(homeStop, "SFA")) return d;
  float dLat = (lat - 37.8404f) * 60.0f;
  float dLon = (lon + 122.2918f) * 60.0f * cosf(37.8404f * 0.0174533f);
  float emy = sqrtf(dLat * dLat + dLon * dLon);
  float homeD = distNm(lat, lon);
  if (homeD < d) d = homeD;
  if (emy < d) d = emy;
  return d;
}

void mapSpan(float &dLon, float &dLat, float nm) {
  float c = cosf(viewLat * 0.0174533f);
  if (c < 0.3f) c = 0.3f;
  dLon = (nm * 0.5f) / (60.0f * c);
  dLat = dLon / ((float)SRC_W / (float)SRC_H);
}

void llToXY(float lat, float lon, float &x, float &y) {
  float nm = (mapOk && mapNm > 0) ? (float)mapNm : (float)rangeNm;
  float dLon, dLat;
  mapSpan(dLon, dLat, nm);
  x = (lon - (viewLon - dLon)) / (2.0f * dLon) * (float)PW;
  y = ((viewLat + dLat) - lat) / (2.0f * dLat) * (float)MAP_H;
}

int headDeg(const char *h) {
  if (!h || !h[0]) return 0;
  if (!strcmp(h, "N")) return 0;
  if (!strcmp(h, "NE")) return 45;
  if (!strcmp(h, "E")) return 90;
  if (!strcmp(h, "SE")) return 135;
  if (!strcmp(h, "S")) return 180;
  if (!strcmp(h, "SW")) return 225;
  if (!strcmp(h, "W")) return 270;
  if (!strcmp(h, "NW")) return 315;
  return 0;
}

const char *headWord(const char *h) {
  if (!h || !h[0]) return "";
  if (!strcmp(h, "N")) return "north";
  if (!strcmp(h, "NE")) return "northeast";
  if (!strcmp(h, "E")) return "east";
  if (!strcmp(h, "SE")) return "southeast";
  if (!strcmp(h, "S")) return "south";
  if (!strcmp(h, "SW")) return "southwest";
  if (!strcmp(h, "W")) return "west";
  if (!strcmp(h, "NW")) return "northwest";
  return h;
}

const char *trainDir(const Train &t) {
  const char *w = headWord(t.head);
  if (w[0]) return w;
  return t.mph < 3 ? "stopped" : "moving";
}

static const char *UA =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36";

void *psAlloc(size_t n) {
  size_t bytes = (n + 63u) & ~63u;
  bytes += 1024;
  void *p = heap_caps_aligned_alloc(64, bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!p) p = ps_malloc(bytes);
  return p;
}

void ltHttp(HTTPClient &http, uint16_t timeout) {
  http.setTimeout(timeout);
  http.setConnectTimeout(4000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setReuse(false);
  http.setUserAgent(UA);
}

SemaphoreHandle_t httpMu = nullptr;
void httpLockInit() {
  if (!httpMu) httpMu = xSemaphoreCreateMutex();
}
bool httpLock() { return !httpMu || xSemaphoreTake(httpMu, pdMS_TO_TICKS(25000)) == pdTRUE; }
void httpUnlock() {
  if (httpMu) xSemaphoreGive(httpMu);
}

bool httpGetBin(const char *url, uint8_t **out, int *outLen) {
  *out = nullptr;
  *outLen = 0;
  if (!wifiLinked() || !httpLock()) return false;
  WiFiClientSecure c;
  c.setInsecure();
  HTTPClient http;
  ltHttp(http, 10000);
  bool ok = false;
  if (http.begin(c, url)) {
    http.addHeader("Accept", "image/jpeg,*/*");
    if (http.GET() == 200) {
      int hint = http.getSize();
      const int CAP = 280000;
      int cap = (hint > 400 && hint < CAP) ? hint : CAP;
      uint8_t *buf = (uint8_t *)psAlloc(cap);
      if (buf) {
        int got = 0;
        uint32_t t0 = millis();
        while (got < cap && millis() - t0 < 10000UL) {
          int n = http.getStream().available();
          if (n > 0) {
            if (n > 1024) n = 1024;
            if (got + n > cap) n = cap - got;
            int r = http.getStream().readBytes(buf + got, n);
            if (r <= 0) break;
            got += r;
            delay(3);
            continue;
          }
          if (!http.connected()) break;
          delay(2);
        }
        if (got >= 400) {
          *out = buf;
          *outLen = got;
          ok = true;
        } else
          free(buf);
      }
    }
    http.end();
  }
  c.stop();
  httpUnlock();
  return ok;
}

bool httpGetPs(const char *url, char **out, int *outLen, int cap, uint16_t timeout, const char *apiKey, bool plain) {
  *out = nullptr;
  *outLen = 0;
  if (!wifiLinked() || !httpLock()) return false;
  WiFiClientSecure c;
  c.setInsecure();
  HTTPClient http;
  ltHttp(http, timeout);
  bool ok = false;
  if (http.begin(c, url)) {
    http.addHeader("Accept", "application/json,*/*");
    if (plain) http.addHeader("Accept-Encoding", "identity");
    if (apiKey && apiKey[0]) http.addHeader("x-api-key", apiKey);
    if (http.GET() == 200) {
      char *buf = (char *)psAlloc(cap);
      if (buf) {
        int got = 0;
        uint32_t t0 = millis();
        while (got < cap - 1 && millis() - t0 < (uint32_t)timeout) {
          int n = http.getStream().available();
          if (n > 0) {
            if (n > 1024) n = 1024;
            if (got + n > cap - 1) n = cap - 1 - got;
            int r = http.getStream().readBytes(buf + got, n);
            if (r <= 0) break;
            got += r;
            delay(3);
            continue;
          }
          if (!http.connected()) break;
          delay(2);
        }
        buf[got] = 0;
        if (got > 80) {
          *out = buf;
          *outLen = got;
          ok = true;
        } else
          free(buf);
      }
    }
    http.end();
  }
  c.stop();
  httpUnlock();
  return ok;
}

bool httpGetSmall(const char *url, char *out, int outn, uint16_t timeout) {
  if (!out || outn < 8) return false;
  out[0] = 0;
  if (!wifiLinked() || !httpLock()) return false;
  WiFiClientSecure c;
  c.setInsecure();
  HTTPClient http;
  ltHttp(http, timeout);
  bool ok = false;
  if (http.begin(c, url)) {
    http.addHeader("Accept", "application/json,*/*");
    int code = http.GET();
    if (code == 200) {
      int got = http.getStream().readBytes(out, outn - 1);
      out[got] = 0;
      ok = got > 0;
    }
    http.end();
  }
  c.stop();
  httpUnlock();
  return ok;
}

int verPack(const char *s) {
  if (!s || !s[0]) return 0;
  int a = 0, b = 0;
  while (*s && !isdigit((unsigned char)*s)) s++;
  while (*s && isdigit((unsigned char)*s)) a = a * 10 + (*s++ - '0');
  if (*s == '.') s++;
  while (*s && isdigit((unsigned char)*s)) b = b * 10 + (*s++ - '0');
  return a * 100 + b;
}

void trimWord(char *s) {
  if (!s) return;
  char *a = s;
  while (*a && isspace((unsigned char)*a)) a++;
  char *e = a + strlen(a);
  while (e > a && isspace((unsigned char)e[-1])) e--;
  *e = 0;
  if (a != s) memmove(s, a, (size_t)(e - a) + 1);
}

void otaNote(const char *s) { strlcpy(otaMsg, s ? s : "", sizeof(otaMsg)); }

bool otaSlotOk() { return esp_ota_get_next_update_partition(NULL) != nullptr; }

void otaProgress(int cur, int tot) {
  if (tot > 0) otaPct = (int)((cur * 100L) / tot);
}

void initSd() {
  if (sdOk) {
    SD.end();
    sdOk = false;
  }
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  sdOk = SD.begin(SD_CS, SPI, 10000000);
  if (!sdOk) sdOk = SD.begin(SD_CS, SPI, 4000000);
  if (!sdOk) sdOk = SD.begin(SD_CS, SPI, 1000000);
}

void otaCheckGithub() {
  otaNewer = false;
  otaLatest[0] = 0;
  if (!wifiLinked()) {
    otaNote("join wifi first");
    return;
  }
  char url[220];
  snprintf(url, sizeof(url), "https://raw.githubusercontent.com/%s/%s/main/ota/version.txt",
           LT_GH_USER, LT_GH_REPO);
  char body[64];
  if (!httpGetSmall(url, body, sizeof(body), 6000)) {
    otaNote("no version on github");
    return;
  }
  trimWord(body);
  if (!body[0] || strlen(body) > 10) {
    otaNote("bad version file");
    return;
  }
  strlcpy(otaLatest, body, sizeof(otaLatest));
  if (verPack(otaLatest) > verPack(LT_VER)) {
    otaNewer = true;
    snprintf(otaMsg, sizeof(otaMsg), "new  %s  tap INSTALL", otaLatest);
  } else if (verPack(otaLatest) == verPack(LT_VER))
    otaNote("this is the latest");
  else
    snprintf(otaMsg, sizeof(otaMsg), "github has  %s", otaLatest);
}

void otaFlashGithub() {
  if (!wifiLinked()) {
    otaNote("join wifi first");
    return;
  }
  if (!otaSlotOk()) {
    otaNote("USB once: 16M 3MB APP/OTA");
    return;
  }
  otaNote("downloading  do not unplug");
  otaPct = 0;
  char url[220];
  snprintf(url, sizeof(url), "https://raw.githubusercontent.com/%s/%s/main/ota/lt_7in.bin",
           LT_GH_USER, LT_GH_REPO);
  if (!httpLock()) {
    otaNote("net busy");
    return;
  }
  WiFiClientSecure c;
  c.setInsecure();
  HTTPUpdate u(180000);
  u.rebootOnUpdate(false);
  u.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  u.onProgress(otaProgress);
  t_httpUpdate_return r = u.update(
      c, url, "", [](HTTPClient *http) {
        if (!http) return;
        http->addHeader("Accept", "application/octet-stream");
        http->setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        http->setTimeout(180000);
      });
  httpUnlock();
  if (r == HTTP_UPDATE_OK) {
    otaPct = 100;
    otaNote("done  rebooting");
    delay(800);
    ESP.restart();
  } else
    otaNote("download fail");
}

bool flashFromFile(File &f) {
  size_t sz = f.size();
  if (sz < 100000) {
    otaNote("file too small");
    return false;
  }
  if (!Update.begin(sz)) {
    otaNote(otaSlotOk() ? "flash begin fail" : "need USB OTA partition");
    return false;
  }
  uint8_t *buf = (uint8_t *)ps_malloc(4096);
  if (!buf) buf = (uint8_t *)malloc(4096);
  if (!buf) {
    Update.abort();
    otaNote("no ram");
    return false;
  }
  size_t got = 0;
  while (f.available()) {
    int n = f.read(buf, 4096);
    if (n <= 0) break;
    if (Update.write(buf, n) != (size_t)n) {
      free(buf);
      Update.abort();
      otaNote("flash write fail");
      return false;
    }
    got += (size_t)n;
    otaProgress((int)got, (int)sz);
  }
  free(buf);
  if (!Update.end()) {
    otaNote("flash end fail");
    return false;
  }
  return true;
}

void otaFlashSd() {
  if (!sdOk) initSd();
  if (!sdOk) {
    otaNote("no SD card");
    return;
  }
  if (!otaSlotOk()) {
    otaNote("USB once: 16M 3MB APP/OTA");
    return;
  }
  const char *names[] = {"/LT.BIN", "/lt.bin", "/lt_7in.bin", "/firmware.bin", nullptr};
  File f;
  for (int i = 0; names[i]; i++) {
    f = SD.open(names[i], FILE_READ);
    if (f) break;
  }
  if (!f) {
    otaNote("put LT.BIN on SD");
    return;
  }
  otaNote("writing SD  do not unplug");
  otaPct = 0;
  bool ok = flashFromFile(f);
  f.close();
  if (ok) {
    otaPct = 100;
    otaNote("done  rebooting");
    delay(800);
    ESP.restart();
  }
}

void runOtaJob() {
  otaBusy = true;
  if (otaKind == 1) otaCheckGithub();
  else if (otaKind == 2) otaFlashGithub();
  else if (otaKind == 3) otaFlashSd();
  otaBusy = false;
  if (otaKind != 2 && otaKind != 3) otaPct = -1;
  otaKind = 0;
}

void askOta(uint8_t kind) {
  if (otaBusy) return;
  otaKind = kind;
  otaPct = (kind == 1) ? -1 : 0;
  if (kind == 1) otaNote("checking github");
  else otaNote("starting update");
  askNet(NET_OTA);
}

const char *findIn(const char *s, int n, const char *pat) {
  int plen = (int)strlen(pat);
  if (!s || n < plen) return nullptr;
  const char *lim = s + n - plen;
  for (const char *p = s; p <= lim; p++)
    if (!memcmp(p, pat, (size_t)plen)) return p;
  return nullptr;
}

bool jsonStr(const char *s, int n, const char *key, char *out, int outn) {
  out[0] = 0;
  char pat[40];
  snprintf(pat, sizeof(pat), "\"%s\":\"", key);
  const char *p = findIn(s, n, pat);
  if (!p) return false;
  p += strlen(pat);
  const char *lim = s + n;
  int i = 0;
  while (p < lim && *p && *p != '"' && i < outn - 1) out[i++] = *p++;
  out[i] = 0;
  return i > 0;
}

bool jsonNum(const char *s, int n, const char *key, float &v) {
  char pat[36];
  snprintf(pat, sizeof(pat), "\"%s\":", key);
  const char *p = findIn(s, n, pat);
  if (!p) return false;
  v = (float)atof(p + strlen(pat));
  return true;
}

int lateFrom(const char *s, int n) {
  const char *p = findIn(s, n, "Minutes Late");
  if (!p) {
    p = findIn(s, n, "Minute Late");
    if (!p) return 0;
  }
  const char *q = p;
  while (q > s && (isdigit((int)q[-1]) || q[-1] == ' ' || q[-1] == '"')) q--;
  while (q < p && !isdigit((int)*q)) q++;
  return atoi(q);
}

void sortTrains() {
  for (int i = 0; i < nTrains; i++)
    for (int j = i + 1; j < nTrains; j++)
      if (distNm(trains[j].lat, trains[j].lon) < distNm(trains[i].lat, trains[i].lon)) {
        Train t = trains[i];
        trains[i] = trains[j];
        trains[j] = t;
      }
}

void grabStops(const char *s, int n, Train &t) {
  t.nStops = 0;
  const char *q = s;
  const char *lim = s + n;
  while (t.nStops < 16 && q < lim) {
    q = findIn(q, (int)(lim - q), "\"code\":\"");
    if (!q) break;
    q += 8;
    if (q + 3 < lim && q[3] == '"' && isupper((int)q[0]) && isupper((int)q[1]) && isupper((int)q[2])) {
      char code[5] = {q[0], q[1], q[2], 0, 0};
      if (findStop(code) >= 0) {
        memcpy(t.stops[t.nStops], code, 4);
        t.nStops++;
      }
    }
    q += 3;
  }
}

bool pbVar(Pb &p, uint64_t &v) {
  v = 0;
  int sh = 0;
  while (p.i < p.n && sh <= 63) {
    uint8_t b = p.s[p.i++];
    v |= (uint64_t)(b & 0x7f) << sh;
    if (!(b & 0x80)) return true;
    sh += 7;
  }
  return false;
}

bool pbKey(Pb &p, int &field, int &wt) {
  uint64_t k;
  if (!pbVar(p, k)) return false;
  wt = (int)(k & 7);
  field = (int)(k >> 3);
  return field > 0;
}

bool pbSkip(Pb &p, int wt) {
  uint64_t n;
  if (wt == 0) return pbVar(p, n);
  if (wt == 1) {
    if (p.i + 8 > p.n) return false;
    p.i += 8;
    return true;
  }
  if (wt == 5) {
    if (p.i + 4 > p.n) return false;
    p.i += 4;
    return true;
  }
  if (wt == 2) {
    if (!pbVar(p, n) || n > 800000 || p.i + (int)n > p.n) return false;
    p.i += (int)n;
    return true;
  }
  return false;
}

bool pbBytes(Pb &p, const uint8_t *&ptr, int &len) {
  uint64_t n;
  if (!pbVar(p, n) || n > 800000 || p.i + (int)n > p.n) return false;
  ptr = p.s + p.i;
  len = (int)n;
  p.i += (int)n;
  return true;
}

bool pbStr(Pb &p, char *out, int outn) {
  const uint8_t *ptr;
  int len;
  if (!pbBytes(p, ptr, len)) return false;
  if (len >= outn) len = outn - 1;
  if (len < 0) len = 0;
  memcpy(out, ptr, (size_t)len);
  out[len] = 0;
  return true;
}

float pbF32(Pb &p) {
  if (p.i + 4 > p.n) {
    p.i = p.n;
    return 0;
  }
  float f;
  memcpy(&f, p.s + p.i, 4);
  p.i += 4;
  return f;
}

double pbF64(Pb &p) {
  if (p.i + 8 > p.n) {
    p.i = p.n;
    return 0;
  }
  double d;
  memcpy(&d, p.s + p.i, 8);
  p.i += 8;
  return d;
}

void pbTrip(Pb p, char *trip, int tripn, char *route, int routen, int &rel) {
  while (p.i < p.n) {
    int field, wt;
    if (!pbKey(p, field, wt)) return;
    if (wt == 2 && field == 1) {
      pbStr(p, trip, tripn);
      continue;
    }
    if (wt == 2 && field == 2) {
      pbStr(p, route, routen);
      continue;
    }
    if (wt == 0 && field == 6) {
      uint64_t v;
      if (!pbVar(p, v)) return;
      rel = (int)(uint32_t)v;
      continue;
    }
    if (!pbSkip(p, wt)) return;
  }
}

bool parseVehicle(Pb p, MtaVeh &v) {
  memset(&v, 0, sizeof(v));
  v.rel = -1;
  while (p.i < p.n) {
    int field, wt;
    if (!pbKey(p, field, wt)) return false;
    if (wt == 2 && field == 1) {
      const uint8_t *ptr;
      int len;
      if (!pbBytes(p, ptr, len)) return false;
      Pb sub = {ptr, len, 0};
      pbTrip(sub, v.trip, (int)sizeof(v.trip), v.route, (int)sizeof(v.route), v.rel);
      continue;
    }
    if (wt == 2 && field == 2) {
      const uint8_t *ptr;
      int len;
      if (!pbBytes(p, ptr, len)) return false;
      Pb sub = {ptr, len, 0};
      while (sub.i < sub.n) {
        int f, w;
        if (!pbKey(sub, f, w)) break;
        if (w == 5 && f == 1) {
          v.lat = pbF32(sub);
          v.hasPos = true;
          continue;
        }
        if (w == 5 && f == 2) {
          v.lon = pbF32(sub);
          continue;
        }
        if (w == 5 && f == 3) {
          v.bearing = pbF32(sub);
          v.hasBrg = true;
          continue;
        }
        if (w == 1 && f == 4) {
          v.speed = pbF64(sub);
          v.hasSpd = true;
          continue;
        }
        if (!pbSkip(sub, w)) break;
      }
      continue;
    }
    if (wt == 2 && field == 7) {
      pbStr(p, v.stop, (int)sizeof(v.stop));
      continue;
    }
    if (wt == 2 && field == 8) {
      const uint8_t *ptr;
      int len;
      if (!pbBytes(p, ptr, len)) return false;
      Pb sub = {ptr, len, 0};
      while (sub.i < sub.n) {
        int f, w;
        if (!pbKey(sub, f, w)) break;
        if (w == 2 && f == 2) {
          pbStr(sub, v.label, (int)sizeof(v.label));
          continue;
        }
        if (!pbSkip(sub, w)) break;
      }
      continue;
    }
    if (wt == 0 && field == 5) {
      uint64_t n;
      if (!pbVar(p, n)) return false;
      v.ts = (uint32_t)n;
      continue;
    }
    if (!pbSkip(p, wt)) return false;
  }
  return true;
}

void parseStu(Pb p, uint32_t &seq, char *sid, int sidn, int &delay) {
  seq = 0;
  sid[0] = 0;
  delay = -100000;
  while (p.i < p.n) {
    int field, wt;
    if (!pbKey(p, field, wt)) return;
    if (wt == 0 && field == 1) {
      uint64_t v;
      if (!pbVar(p, v)) return;
      seq = (uint32_t)v;
      continue;
    }
    if (wt == 2 && field == 4) {
      pbStr(p, sid, sidn);
      continue;
    }
    if (wt == 2 && (field == 2 || field == 3)) {
      const uint8_t *ptr;
      int len;
      if (!pbBytes(p, ptr, len)) return;
      Pb sub = {ptr, len, 0};
      while (sub.i < sub.n) {
        int f, w;
        if (!pbKey(sub, f, w)) break;
        if (w == 0 && f == 1) {
          uint64_t v;
          if (!pbVar(sub, v)) break;
          delay = (int)(int32_t)(uint32_t)v;
          continue;
        }
        if (!pbSkip(sub, w)) break;
      }
      continue;
    }
    if (!pbSkip(p, wt)) return;
  }
}

bool parseTripUpdate(Pb p, MtaTu &t) {
  memset(&t, 0, sizeof(t));
  t.rel = -1;
  t.delay = -100000;
  uint32_t seqMin = 0xffffffffu, seqMax = 0;
  int delayAt = 0x7fffffff;
  while (p.i < p.n) {
    int field, wt;
    if (!pbKey(p, field, wt)) return false;
    if (wt == 2 && field == 1) {
      const uint8_t *ptr;
      int len;
      if (!pbBytes(p, ptr, len)) return false;
      Pb sub = {ptr, len, 0};
      pbTrip(sub, t.trip, (int)sizeof(t.trip), t.route, (int)sizeof(t.route), t.rel);
      continue;
    }
    if (wt == 0 && field == 5) {
      uint64_t v;
      if (!pbVar(p, v)) return false;
      t.delay = (int)(int32_t)(uint32_t)v;
      continue;
    }
    if (wt == 2 && field == 2) {
      const uint8_t *ptr;
      int len;
      if (!pbBytes(p, ptr, len)) return false;
      Pb sub = {ptr, len, 0};
      uint32_t seq = 0;
      char sid[8];
      int dly = -100000;
      parseStu(sub, seq, sid, (int)sizeof(sid), dly);
      if (sid[0]) {
        if (seq <= seqMin) {
          seqMin = seq;
          memcpy(t.orig, sid, sizeof(t.orig));
        }
        if (seq >= seqMax) {
          seqMax = seq;
          memcpy(t.dest, sid, sizeof(t.dest));
        }
        if (dly > -100000 && (int)seq < delayAt) {
          delayAt = (int)seq;
          if (t.delay < -99999) t.delay = dly;
        }
      }
      continue;
    }
    if (!pbSkip(p, wt)) return false;
  }
  return true;
}

void lirrNum(const char *trip, char *out, int n) {
  out[0] = 0;
  if (!trip || n < 2) return;
  int us = 0;
  const char *start = nullptr;
  const char *stop = nullptr;
  for (const char *p = trip; *p; p++) {
    if (*p == '_') {
      us++;
      if (us == 2) start = p + 1;
      if (us == 3) {
        stop = p;
        break;
      }
    }
  }
  if (!start) return;
  if (!stop) stop = start + strlen(start);
  int i = 0;
  while (start < stop && *start >= '0' && *start <= '9' && i < n - 1) out[i++] = *start++;
  out[i] = 0;
}

void bearHead(float deg, char *out, int n) {
  int d = (((int)(deg + 0.5f) % 360) + 360) % 360;
  const char *w[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
  strlcpy(out, w[((d + 22) / 45) & 7], n);
}

bool mtaNearHome() {
  float dLat = (homeLat - 40.7506f) * 60.0f;
  float dLon = (homeLon + 73.9936f) * 60.0f * cosf(40.7506f * 0.0174533f);
  return sqrtf(dLat * dLat + dLon * dLon) <= 160.0f;
}

bool mtaBox(float lat, float lon) {
  return lat >= 40.45f && lat <= 42.05f && lon >= -74.95f && lon <= -71.30f;
}

void mtaPut(Train *fresh, int *n, char tripId[][28], const Train &t, const char *trip, float dist) {
  if (trip && trip[0]) {
    for (int i = 0; i < *n; i++)
      if (tripId[i][0] && !strcmp(tripId[i], trip)) return;
  }
  int slot = -1;
  if (*n < MAX_T) slot = (*n)++;
  else {
    int far = 0;
    float bd = -1;
    for (int i = 0; i < *n; i++) {
      float d = rangeDist(fresh[i].lat, fresh[i].lon);
      if (d > bd) {
        bd = d;
        far = i;
      }
    }
    if (dist >= bd) return;
    slot = far;
  }
  fresh[slot] = t;
  tripId[slot][0] = 0;
  if (trip && trip[0]) strlcpy(tripId[slot], trip, 28);
}

int mtaMph(const char *trip, float lat, float lon, uint32_t ts, char *head, int headn) {
  struct Hp {
    char trip[28];
    float lat, lon;
    uint32_t ts;
  };
  static Hp h[160];
  if (!trip || !trip[0]) return 0;
  int freeSlot = -1, oldest = 0;
  for (int i = 0; i < 160; i++) {
    if (!h[i].trip[0]) {
      if (freeSlot < 0) freeSlot = i;
      continue;
    }
    if (!h[oldest].trip[0] || h[i].ts < h[oldest].ts) oldest = i;
    if (strcmp(h[i].trip, trip)) continue;
    float dLat = (lat - h[i].lat) * 60.0f;
    float dLon = (lon - h[i].lon) * 60.0f * cosf(lat * 0.0174533f);
    float miles = sqrtf(dLat * dLat + dLon * dLon) * 1.15078f;
    uint32_t dt = (ts && h[i].ts && ts > h[i].ts) ? (ts - h[i].ts) : 0;
    if (!dt || miles < 0.03f) return 0;
    int mph = 0;
    if (dt <= 600) {
      float v = miles * 3600.0f / (float)dt;
      if (v >= 4.0f && v <= 90.0f) mph = (int)(v + 0.5f);
    }
    if (mph >= 4 && head && headn > 1 && !head[0]) {
      float north = lat - h[i].lat;
      float east = (lon - h[i].lon) * cosf(lat * 0.0174533f);
      float deg = atan2f(east, north) * 57.2957795f;
      if (deg < 0) deg += 360.0f;
      bearHead(deg, head, headn);
    }
    h[i].lat = lat;
    h[i].lon = lon;
    h[i].ts = ts;
    return mph;
  }
  int slot = freeSlot >= 0 ? freeSlot : oldest;
  memset(&h[slot], 0, sizeof(h[slot]));
  strlcpy(h[slot].trip, trip, sizeof(h[slot].trip));
  h[slot].lat = lat;
  h[slot].lon = lon;
  h[slot].ts = ts;
  return 0;
}

void mtaScan(const uint8_t *buf, int len, Train *fresh, int *n, char tripId[][28], int kind, int pass) {
  const MtaStopNm *stops = kind ? MNR_STOPS : LIRR_STOPS;
  int nStops = kind ? MNR_STOPS_N : LIRR_STOPS_N;
  const MtaRouteNm *routes = kind ? MNR_ROUTES : LIRR_ROUTES;
  int nRoutes = kind ? MNR_ROUTES_N : LIRR_ROUTES_N;
  const char *prov = kind ? "MNR" : "LIRR";
  Pb feed = {buf, len, 0};
  while (feed.i < feed.n) {
    int field, wt;
    if (!pbKey(feed, field, wt)) return;
    if (!(wt == 2 && field == 2)) {
      if (!pbSkip(feed, wt)) return;
      continue;
    }
    const uint8_t *ep;
    int el;
    if (!pbBytes(feed, ep, el)) return;
    Pb ent = {ep, el, 0};
    while (ent.i < ent.n) {
      int f, w;
      if (!pbKey(ent, f, w)) break;
      if (pass == 0 && w == 2 && f == 4) {
        const uint8_t *vp;
        int vl;
        if (!pbBytes(ent, vp, vl)) break;
        MtaVeh v;
        Pb sub = {vp, vl, 0};
        if (!parseVehicle(sub, v) || !v.hasPos || v.rel == 3) continue;
        if (!mtaBox(v.lat, v.lon)) continue;
        float dist = rangeDist(v.lat, v.lon);
        if (dist > 150.0f) continue;
        Train t;
        memset(&t, 0, sizeof(t));
        if (!kind) lirrNum(v.trip, t.num, (int)sizeof(t.num));
        if (!t.num[0]) strlcpy(t.num, v.label[0] ? v.label : v.trip, sizeof(t.num));
        if (!t.num[0]) continue;
        const char *rn = mtaFindRoute(routes, nRoutes, v.route);
        strlcpy(t.route, rn[0] ? rn : prov, sizeof(t.route));
        strlcpy(t.prov, prov, sizeof(t.prov));
        const char *sn = mtaFindStop(stops, nStops, v.stop);
        if (sn[0]) strlcpy(t.next, sn, sizeof(t.next));
        t.lat = v.lat;
        t.lon = v.lon;
        if (v.hasBrg) bearHead(v.bearing, t.head, (int)sizeof(t.head));
        if (v.hasSpd && v.speed >= 0 && v.speed < 60.0) t.mph = (int)(v.speed * 2.23694f + 0.5f);
        else t.mph = mtaMph(v.trip, v.lat, v.lon, v.ts, t.head, (int)sizeof(t.head));
        mtaPut(fresh, n, tripId, t, v.trip, dist);
        continue;
      }
      if (pass == 1 && w == 2 && f == 3) {
        const uint8_t *tp;
        int tl;
        if (!pbBytes(ent, tp, tl)) break;
        MtaTu tu;
        Pb sub = {tp, tl, 0};
        if (!parseTripUpdate(sub, tu) || !tu.trip[0]) continue;
        int k = -1;
        for (int i = 0; i < *n; i++)
          if (tripId[i][0] && !strcmp(tripId[i], tu.trip)) {
            k = i;
            break;
          }
        if (k < 0) continue;
        if (tu.rel == 3) {
          fresh[k].lat = fresh[k].lon = 0;
          tripId[k][0] = 0;
          continue;
        }
        const char *rn = mtaFindRoute(routes, nRoutes, tu.route);
        if (rn[0]) strlcpy(fresh[k].route, rn, sizeof(fresh[k].route));
        const char *a = mtaFindStop(stops, nStops, tu.orig);
        const char *b = mtaFindStop(stops, nStops, tu.dest);
        if (a[0]) strlcpy(fresh[k].orig, a, sizeof(fresh[k].orig));
        if (b[0]) strlcpy(fresh[k].dest, b, sizeof(fresh[k].dest));
        if (tu.delay > -99999) fresh[k].lateMin = tu.delay >= 60 ? tu.delay / 60 : 0;
        continue;
      }
      if (!pbSkip(ent, w)) break;
    }
  }
}

void mtaAdd(Train *fresh, int *n) {
  static char tripId[MAX_T][28];
  memset(tripId, 0, sizeof(tripId));
  if (!mtaNearHome()) return;
  const char *url[2] = {
      "https://api-endpoint.mta.info/Dataservice/mtagtfsfeeds/lirr%2Fgtfs-lirr",
      "https://api-endpoint.mta.info/Dataservice/mtagtfsfeeds/mnr%2Fgtfs-mnr"};
  const char *who[2] = {"lirr no reply", "mnr no reply"};
  for (int kind = 0; kind < 2; kind++) {
    char *body = nullptr;
    int got = 0;
    if (!httpGetPs(url[kind], &body, &got, 262144, 12000, nullptr, true)) {
      Serial.println(who[kind]);
      continue;
    }
    if (got >= 2 && (uint8_t)body[0] == 0x1f && (uint8_t)body[1] == 0x8b) {
      Serial.println(kind ? "mnr gzip" : "lirr gzip");
      free(body);
      continue;
    }
    mtaScan((const uint8_t *)body, got, fresh, n, tripId, kind, 0);
    mtaScan((const uint8_t *)body, got, fresh, n, tripId, kind, 1);
    free(body);
  }
  int w = 0;
  for (int i = 0; i < *n; i++) {
    if (fresh[i].lat == 0 && fresh[i].lon == 0) continue;
    if (!fresh[i].orig[0] && (fresh[i].prov[0] == 'L' || fresh[i].prov[0] == 'M'))
      strlcpy(fresh[i].orig, fresh[i].route, sizeof(fresh[i].orig));
    if (!fresh[i].dest[0] && fresh[i].next[0] && (fresh[i].prov[0] == 'L' || fresh[i].prov[0] == 'M'))
      strlcpy(fresh[i].dest, fresh[i].next, sizeof(fresh[i].dest));
    if (w != i) fresh[w] = fresh[i];
    w++;
  }
  *n = w;
}

bool homeCode(const char *code) {
  return code && code[0] && !strcasecmp(homeStop, code);
}

const char *septaLine(const char *id) {
  struct Nm {
    const char *id, *name;
  };
  static const Nm tab[] = {
      {"AIR", "Airport"}, {"CHE", "Chestnut East"}, {"CHW", "Chestnut West"}, {"CYN", "Cynwyd"},
      {"FOX", "Fox Chase"}, {"LAN", "Lansdale"},    {"MED", "Media"},         {"NOR", "Norristown"},
      {"PAO", "Paoli"},     {"TRE", "Trenton"},      {"WAR", "Warminster"},    {"WIL", "Wilmington"},
      {"WTR", "West Trenton"}};
  if (!id) return "";
  for (int i = 0; i < (int)(sizeof(tab) / sizeof(tab[0])); i++)
    if (!strcmp(tab[i].id, id)) return tab[i].name;
  return id;
}

void railNum(const MtaVeh &v, char *num, int n) {
  num[0] = 0;
  if (n < 2) return;
  if (v.label[0] && !strchr(v.label, ',') && (int)strlen(v.label) < n) {
    strlcpy(num, v.label, n);
    return;
  }
  const char *p = v.trip;
  while (*p && (*p < '0' || *p > '9')) p++;
  int i = 0;
  while (*p >= '0' && *p <= '9' && i < n - 1) num[i++] = *p++;
  num[i] = 0;
  if (!num[0]) strlcpy(num, v.route[0] ? v.route : "rail", n);
}

void vpScan(const uint8_t *buf, int len, Train *fresh, int *n, char tripId[][28], const char *prov) {
  Pb feed = {buf, len, 0};
  while (feed.i < feed.n) {
    int field, wt;
    if (!pbKey(feed, field, wt)) return;
    if (!(wt == 2 && field == 2)) {
      if (!pbSkip(feed, wt)) return;
      continue;
    }
    const uint8_t *ep;
    int el;
    if (!pbBytes(feed, ep, el)) return;
    Pb ent = {ep, el, 0};
    while (ent.i < ent.n) {
      int f, w;
      if (!pbKey(ent, f, w)) break;
      if (w == 2 && f == 4) {
        const uint8_t *vp;
        int vl;
        if (!pbBytes(ent, vp, vl)) break;
        MtaVeh v;
        Pb sub = {vp, vl, 0};
        if (!parseVehicle(sub, v) || !v.hasPos || v.rel == 3) continue;
        float dist = rangeDist(v.lat, v.lon);
        if (dist > 150.0f) continue;
        Train t;
        memset(&t, 0, sizeof(t));
        railNum(v, t.num, (int)sizeof(t.num));
        if (!t.num[0]) continue;
        const char *line = (!strcmp(prov, "SEPTA")) ? septaLine(v.route) : v.route;
        strlcpy(t.route, line[0] ? line : prov, sizeof(t.route));
        strlcpy(t.prov, prov, sizeof(t.prov));
        t.lat = v.lat;
        t.lon = v.lon;
        if (v.hasBrg) bearHead(v.bearing, t.head, (int)sizeof(t.head));
        if (v.hasSpd && v.speed >= 0 && v.speed < 60.0) t.mph = (int)(v.speed * 2.23694f + 0.5f);
        mtaPut(fresh, n, tripId, t, v.trip[0] ? v.trip : t.num, dist);
        continue;
      }
      if (!pbSkip(ent, w)) break;
    }
  }
}

void oneRail(Train *fresh, int *n, char tripId[][28], const char *url, const char *prov) {
  char *body = nullptr;
  int got = 0;
  if (!httpGetPs(url, &body, &got, 262144, 12000, nullptr, true)) return;
  if (got >= 2 && (uint8_t)body[0] == 0x1f && (uint8_t)body[1] == 0x8b) {
    free(body);
    return;
  }
  vpScan((const uint8_t *)body, got, fresh, n, tripId, prov);
  free(body);
}

void extraRail(Train *fresh, int *n) {
  static char tripId[MAX_T][28];
  if (homeCode("PHL") || homeCode("WIL") || homeCode("TRE")) {
    memset(tripId, 0, sizeof(tripId));
    oneRail(fresh, n, tripId,
            "https://www3.septa.org/gtfsrt/septarail-pa-us/Vehicle/rtVehiclePosition.pb", "SEPTA");
  }
  if (homeCode("WAS") || homeCode("BAL") || homeCode("BWI")) {
    memset(tripId, 0, sizeof(tripId));
    oneRail(fresh, n, tripId, "https://mdotmta-gtfs-rt.s3.amazonaws.com/MARC+RT/marc-vp.pb", "MARC");
  }
  if (homeCode("WAS")) {
    memset(tripId, 0, sizeof(tripId));
    oneRail(fresh, n, tripId, "https://gtfs.vre.org/containercdngtfsupload/VehiclePositionFeed", "VRE");
  }
  if (homeCode("MIC")) {
    memset(tripId, 0, sizeof(tripId));
    oneRail(fresh, n, tripId, "https://gtfsr.tri-rail.com/download.aspx?file=position_updates.pb", "TRI");
  }
}

void commitTrains(Train *fresh, int n, const char *keep) {
  nTrains = n;
  if (n > 0) memcpy(trains, fresh, sizeof(Train) * (size_t)n);
  sortTrains();
  selected = -1;
  if (keep && keep[0])
    for (int i = 0; i < nTrains; i++)
      if (!strcmp(trains[i].num, keep)) {
        selected = i;
        break;
      }
  lastFetch = millis();
  if (nTrains)
    snprintf(trainsErr, sizeof(trainsErr), "LIVE %d", nTrains);
  else
    strlcpy(trainsErr, "LIVE  none", sizeof(trainsErr));
}

void fetchTrains() {
  static Train fresh[MAX_T];
  char keep[12] = "";
  if (selected >= 0 && selected < nTrains) strlcpy(keep, trains[selected].num, sizeof(keep));
  char *body = nullptr;
  int got = 0;
  strlcpy(trainsErr, "TRAINS ...", sizeof(trainsErr));
  if (!httpGetPs("https://api-v3.amtraker.com/v3/trains", &body, &got, 1100000, 25000, nullptr, false)) {
    if (nTrains > 0) {
      lastFetch = millis();
      snprintf(trainsErr, sizeof(trainsErr), "LIVE %d", nTrains);
      return;
    }
    if (!httpGetPs("https://api-v3.amtraker.com/v3/trains", &body, &got, 700000, 18000, nullptr, false)) {
      int n = 0;
      mtaAdd(fresh, &n);
      extraRail(fresh, &n);
      if (n > 0) {
        commitTrains(fresh, n, keep);
        return;
      }
      strlcpy(trainsErr, "train fetch fail", sizeof(trainsErr));
      lastFetch = millis() ? millis() : 1;
      return;
    }
  }
  int n = 0;
  const char *p = body;
  const char *bodyEnd = body + got;
  while (n < MAX_T && (p = strstr(p, "\"trainID\""))) {
    const char *obj = p;
    while (obj > body && *obj != '{') obj--;
    const char *nextId = strstr(p + 10, "\"trainID\"");
    const char *end = nextId ? nextId : bodyEnd;
    const char *st = findIn(obj, (int)(end - obj), "\"stations\"");
    int headWin = st ? (int)(st - obj) : (int)(end - obj);
    if (headWin < 80) {
      headWin = (int)(end - obj);
      if (headWin > 1200) headWin = 1200;
    }
    if (headWin > 1600) headWin = 1600;
    const char *tail = nullptr;
    if (st) tail = findIn(st, (int)(end - st), "\"heading\"");
    if (!tail || tail >= end) {
      tail = end - 2800;
      if (tail < obj) tail = obj;
    }
    int tailWin = (int)(end - tail);
    Train t;
    memset(&t, 0, sizeof(t));
    jsonStr(obj, headWin, "trainNum", t.num, sizeof(t.num));
    jsonStr(obj, headWin, "routeName", t.route, sizeof(t.route));
    jsonStr(obj, headWin, "provider", t.prov, sizeof(t.prov));
    jsonNum(obj, headWin, "lat", t.lat);
    jsonNum(obj, headWin, "lon", t.lon);
    jsonStr(tail, tailWin, "origName", t.orig, sizeof(t.orig));
    jsonStr(tail, tailWin, "destName", t.dest, sizeof(t.dest));
    jsonStr(tail, tailWin, "eventName", t.next, sizeof(t.next));
    if (!jsonStr(tail, tailWin, "heading", t.head, sizeof(t.head))) {
      float hd = -1;
      if (jsonNum(tail, tailWin, "heading", hd) && hd >= 0) {
        int d = (((int)(hd + 0.5f) % 360) + 360) % 360;
        const char *w[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
        strlcpy(t.head, w[((d + 22) / 45) & 7], sizeof(t.head));
      }
    }
    jsonStr(tail, tailWin, "trainState", t.state, sizeof(t.state));
    float vel = 0;
    jsonNum(tail, tailWin, "velocity", vel);
    t.mph = (int)(vel + 0.5f);
    t.lateMin = lateFrom(st ? st : tail, (int)(end - (st ? st : tail)));
    if (st && st < end) grabStops(st, (int)((tail > st ? tail : end) - st), t);
    p += 10;
    if (!t.num[0] || (t.lat == 0 && t.lon == 0)) continue;
    if (!strcasecmp(t.state, "Completed") || !strcasecmp(t.state, "Cancelled")) continue;
    if (rangeDist(t.lat, t.lon) > 150.0f) continue;
    fresh[n++] = t;
  }
  bool whole = false;
  if (got > 1) {
    int e = got - 1;
    while (e > 0 && (unsigned char)body[e] <= ' ') e--;
    whole = body[e] == '}' || body[e] == ']';
  }
  static char listedStop[8] = "";
  bool moved = strcasecmp(listedStop, homeStop) != 0;
  if (!moved && !whole && n < nTrains) {
    free(body);
    lastFetch = millis();
    snprintf(trainsErr, sizeof(trainsErr), "LIVE %d", nTrains);
    return;
  }
  free(body);
  mtaAdd(fresh, &n);
  extraRail(fresh, &n);
  if (moved || listTop >= n) listTop = 0;
  strlcpy(listedStop, homeStop, sizeof(listedStop));
  commitTrains(fresh, n, keep);
}

bool jpgPlot(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  if (!jpgSrc) return 0;
  for (uint16_t j = 0; j < h; j++) {
    int yy = y + j;
    if (yy < 0 || yy >= SRC_H) continue;
    for (uint16_t i = 0; i < w; i++) {
      int xx = x + i;
      if (xx < 0 || xx >= SRC_W) continue;
      jpgSrc[yy * SRC_W + xx] = bitmap[j * w + i];
    }
  }
  return 1;
}

bool ensureJpgSrc() {
  if (jpgSrc) return true;
  jpgSrc = (uint16_t *)ps_malloc((size_t)SRC_W * SRC_H * 2);
  return jpgSrc != nullptr;
}

bool ensureMapFb() {
  if (mapFb) return true;
  mapFb = (uint16_t *)ps_malloc((size_t)PW * MAP_H * 2);
  return mapFb != nullptr;
}

void scaleMapUp() {
  if (!jpgSrc || !mapFb) return;
  int sw = mapSrcW > 0 ? mapSrcW : SRC_W;
  int sh = mapSrcH > 0 ? mapSrcH : SRC_H;
  for (int y = 0; y < MAP_H; y++) {
    int sy = y * sh / MAP_H;
    if (sy >= sh) sy = sh - 1;
    for (int x = 0; x < PW; x++) {
      int sx = x * sw / PW;
      if (sx >= sw) sx = sw - 1;
      mapFb[y * PW + x] = jpgSrc[sy * SRC_W + sx];
    }
  }
}

void bbox(float &minLon, float &minLat, float &maxLon, float &maxLat) {
  float dLon, dLat;
  mapSpan(dLon, dLat, (float)rangeNm);
  minLon = viewLon - dLon;
  maxLon = viewLon + dLon;
  minLat = viewLat - dLat;
  maxLat = viewLat + dLat;
}

bool fetchJpegMap(const char *url) {
  if (!ensureJpgSrc() || !ensureMapFb()) return false;
  const uint16_t SENT = 0x0001;
  for (int i = 0; i < SRC_W * SRC_H; i++) jpgSrc[i] = SENT;
  uint8_t *buf = nullptr;
  int got = 0;
  if (!httpGetBin(url, &buf, &got)) return false;
  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(jpgPlot);
  TJpgDec.setSwapBytes(false);
  bool ok = TJpgDec.drawJpg(0, 0, buf, (uint32_t)got) == 0;
  if (!ok) {
    TJpgDec.setSwapBytes(true);
    ok = TJpgDec.drawJpg(0, 0, buf, (uint32_t)got) == 0;
  }
  free(buf);
  if (!ok) return false;
  int filled = 0;
  for (int i = 0; i < SRC_W * SRC_H; i++)
    if (jpgSrc[i] != SENT) filled++;
  if (filled < (SRC_W * SRC_H) / 20) return false;
  scaleMapUp();
  return true;
}

const char *mapService() {
  if (mapStyle) return "World_Imagery";
  if (!dayMode) return "Canvas/World_Dark_Gray_Base";
  return "Canvas/World_Light_Gray_Base";
}

bool fetchMap() {
  lastMapTry = millis();
  if (!mapOn || !wifiLinked()) return false;
  float minLon, minLat, maxLon, maxLat;
  bbox(minLon, minLat, maxLon, maxLat);
  mapSrcW = SRC_W;
  mapSrcH = SRC_H;
  char url[420];
  const char *trySvc[2];
  trySvc[0] = mapService();
  trySvc[1] = mapStyle ? "World_Street_Map" : "World_Imagery";
  for (int k = 0; k < 2; k++) {
    if (k && !strcmp(trySvc[0], trySvc[1])) continue;
    snprintf(url, sizeof(url),
             "https://server.arcgisonline.com/ArcGIS/rest/services/%s/MapServer/export"
             "?bbox=%.5f,%.5f,%.5f,%.5f&bboxSR=4326&imageSR=4326&size=%d,%d&format=jpg&f=image",
             trySvc[k], minLon, minLat, maxLon, maxLat, mapSrcW, mapSrcH);
    if (fetchJpegMap(url)) {
      mapOk = true;
      mapHaveStyle = mapStyle;
      mapNm = rangeNm;
      lastMap = millis();
      return true;
    }
  }
  return false;
}

bool onQuietPage() {
  if (page == PG_WIFI || geoEdit) return true;
  if (page == PG_SETTINGS && setPane == 1) return true;
  return false;
}

void askNet(int job) {
  if (dlBurst > 0)
    dlBurst--;
  else if (onQuietPage())
    return;
  if (netBusy || netJob) {
    if (job == NET_OTA) netPend = NET_OTA;
    else if (!netPend) netPend = job;
    return;
  }
  netJob = job;
}

void netTask(void *pv) {
  (void)pv;
  for (;;) {
    int job = netJob;
    if (job) {
      netBusy = true;
      netJob = NET_NONE;
      if (job == NET_TRAINS) fetchTrains();
      else if (job == NET_MAP) fetchMap();
      else if (job == NET_OTA) runOtaJob();
      netDoneJob = job;
      netDone = true;
      netBusy = false;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void drawZoomBar() {
  gfx->fillRect(EDGE, ZOOM_Y, PW - EDGE, MENU_Y - ZOOM_Y, COL_BG);
  gfx->fillRoundRect(16, ZOOM_Y + 8, 160, 32, 6, COL_DIM);
  gfx->fillRoundRect(PW - 176, ZOOM_Y + 8, 160, 32, 6, COL_DIM);
  gfx->setTextColor(COL_TXT, COL_DIM);
  gfx->setTextSize(2);
  centre("ZOOM -", 96, ZOOM_Y + 16);
  centre("ZOOM +", PW - 96, ZOOM_Y + 16);
  gfx->setTextColor(COL_YEL, COL_BG);
  char t[28];
  snprintf(t, sizeof(t), "ZOOM  %dNM", rangeNm);
  centre(t, CX, ZOOM_Y + 16);
}

void drawHomeMenu() {
  const char *lab[] = {"LINE", "LIST", "DEPOT", "CLOCK", "SETUP"};
  gfx->fillRect(EDGE, MENU_Y, PW - EDGE, PH - MENU_Y, COL_DIM);
  gfx->drawFastHLine(EDGE, MENU_Y, PW - EDGE, COL_MID);
  int bw = PW / 5;
  for (int i = 1; i < 5; i++)
    gfx->drawFastVLine(i * bw, MENU_Y + 1, PH - MENU_Y - 1, COL_MID);
  gfx->setTextColor(COL_WHT, COL_DIM);
  gfx->setTextSize(2);
  for (int i = 0; i < 5; i++)
    centre(lab[i], i * bw + bw / 2, MENU_Y + 28);
}

void blitMap() {
  const int x0 = EDGE, ww = PW - EDGE;
  if (!(mapOn && mapOk && mapFb)) {
    gfx->fillRect(x0, 0, ww, MAP_H, COL_BG);
    return;
  }
  static uint16_t strip[798 * 12];
  if (mapOx == 0 && mapOy == 0) {
    for (int y = 0; y < MAP_H; y += 12) {
      int rows = MAP_H - y;
      if (rows > 12) rows = 12;
      for (int r = 0; r < rows; r++)
        memcpy(strip + r * ww, mapFb + (y + r) * PW + EDGE, (size_t)ww * 2);
      gfx->draw16bitRGBBitmap(x0, y, strip, ww, rows);
    }
    return;
  }
  if (mapOx > 0) gfx->fillRect(x0, 0, mapOx, MAP_H, COL_BG);
  if (mapOx < 0) gfx->fillRect(PW + mapOx, 0, -mapOx, MAP_H, COL_BG);
  if (mapOy > 0) gfx->fillRect(x0, 0, ww, mapOy, COL_BG);
  if (mapOy < 0) gfx->fillRect(x0, MAP_H + mapOy, ww, -mapOy, COL_BG);
  int sx = EDGE, sy = 0, dx = mapOx + EDGE, dy = mapOy, dw = ww, dh = MAP_H;
  if (dx < EDGE) {
    sx += EDGE - dx;
    dw -= EDGE - dx;
    dx = EDGE;
  }
  if (dy < 0) {
    sy = -dy;
    dh += dy;
    dy = 0;
  }
  if (dx + dw > PW) dw = PW - dx;
  if (dy + dh > MAP_H) dh = MAP_H - dy;
  if (dw <= 0 || dh <= 0) return;
  for (int y = 0; y < dh; y += 12) {
    int rows = dh - y;
    if (rows > 12) rows = 12;
    for (int r = 0; r < rows; r++)
      memcpy(strip + r * dw, mapFb + (sy + y + r) * PW + sx, (size_t)dw * 2);
    gfx->draw16bitRGBBitmap(dx, dy + y, strip, dw, rows);
  }
}

void drawRings() {
  gfx->drawCircle(CX, CY, RR / 3, COL_MID);
  gfx->drawCircle(CX, CY, 2 * RR / 3, COL_MID);
  gfx->drawCircle(CX, CY, RR, COL_WHT);
  compassPill(CX, 16, "N");
  compassPill(CX, MAP_H - 28, "S");
  compassPill(24, CY - 8, "W");
  compassPill(PW - 24, CY - 8, "E");
}

static const RailPt RAIL_SURF[] = {
    {32.7160f, -117.1650f, "San Diego"},
    {32.7552f, -117.1995f, "Old Town"},
    {32.8480f, -117.2280f, nullptr},
    {32.9590f, -117.2620f, "Del Mar"},
    {32.9928f, -117.2680f, "Solana Beach"},
    {33.0370f, -117.2880f, nullptr},
    {33.1260f, -117.3230f, nullptr},
    {33.1600f, -117.3470f, "Carlsbad"},
    {33.1924f, -117.3750f, "Oceanside"},
    {33.2480f, -117.4220f, nullptr},
    {33.2980f, -117.4660f, nullptr},
    {33.3400f, -117.5120f, nullptr},
    {33.3720f, -117.5500f, nullptr},
    {33.4000f, -117.5880f, nullptr},
    {33.4260f, -117.6100f, "San Clemente"},
    {33.4580f, -117.6360f, nullptr},
    {33.5017f, -117.6580f, "San Juan Cap"},
    {33.5800f, -117.6960f, nullptr},
    {33.6568f, -117.7336f, "Irvine"},
    {33.7500f, -117.8560f, "Santa Ana"},
    {33.8033f, -117.8765f, "Anaheim"},
    {33.8687f, -117.9226f, "Fullerton"},
    {34.0562f, -118.2346f, "Los Angeles"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_COAST[] = {
    {34.0562f, -118.2346f, "Los Angeles"},
    {34.1781f, -118.3516f, "Burbank"},
    {34.1863f, -118.4487f, "Van Nuys"},
    {34.2200f, -118.6200f, nullptr},
    {34.2360f, -118.9000f, nullptr},
    {34.2292f, -119.1574f, "Oxnard"},
    {34.2600f, -119.2300f, nullptr},
    {34.2769f, -119.2920f, "Ventura"},
    {34.3400f, -119.4200f, nullptr},
    {34.3900f, -119.5600f, nullptr},
    {34.4137f, -119.6850f, "Santa Barbara"},
    {34.4356f, -119.8380f, "Goleta"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_JOAQ[] = {
    {35.3729f, -119.0081f, "Bakersfield"},
    {36.3275f, -119.6456f, "Hanford"},
    {36.7381f, -119.7856f, "Fresno"},
    {36.9570f, -120.0610f, "Madera"},
    {37.3074f, -120.4803f, "Merced"},
    {37.9570f, -121.2780f, "Stockton"},
    {38.5840f, -121.5006f, "Sacramento"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_JOAQ2[] = {
    {37.3074f, -120.4803f, "Merced"},
    {37.4890f, -120.8480f, "Turlock"},
    {37.6688f, -120.9940f, "Modesto"},
    {37.9570f, -121.2780f, "Stockton"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_CAP[] = {
    {38.5840f, -121.5006f, "Sacramento"},
    {38.5436f, -121.7378f, "Davis"},
    {38.2432f, -122.0408f, "Suisun"},
    {38.0190f, -122.1380f, "Martinez"},
    {37.9368f, -122.3535f, "Richmond"},
    {37.8674f, -122.3000f, "Berkeley"},
    {37.8404f, -122.2918f, "Emeryville"},
    {37.7936f, -122.2714f, "Oakland"},
    {37.7525f, -122.1978f, "Coliseum"},
    {37.6564f, -122.0952f, "Hayward"},
    {37.5585f, -121.9968f, "Fremont"},
    {37.3302f, -121.9026f, "San Jose"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_NEC[] = {
    {42.3523f, -71.0553f, "Boston"},
    {42.3473f, -71.0755f, "Back Bay"},
    {41.8293f, -71.4135f, "Providence"},
    {41.2975f, -72.9267f, "New Haven"},
    {41.0468f, -73.5422f, "Stamford"},
    {40.7506f, -73.9936f, "New York"},
    {40.7342f, -74.1642f, "Newark"},
    {40.5682f, -74.3296f, "Metropark"},
    {40.2187f, -74.7541f, "Trenton"},
    {39.9556f, -75.1820f, "Philadelphia"},
    {39.7370f, -75.5510f, "Wilmington"},
    {39.3074f, -76.6156f, "Baltimore"},
    {39.1924f, -76.6944f, "BWI"},
    {38.8970f, -77.0063f, "Washington"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_HIA[] = {
    {41.8786f, -87.6400f, "Chicago"},
    {42.0745f, -87.8056f, "Glenview"},
    {43.0344f, -87.9170f, "Milwaukee"},
    {RAIL_END, 0, nullptr}};
static const RailPt RAIL_CHI_W[] = {
    {41.8786f, -87.6400f, "Chicago"},
    {41.7796f, -88.1458f, "Naperville"},
    {RAIL_END, 0, nullptr}};
static const RailPt RAIL_CHI_SW[] = {
    {41.8786f, -87.6400f, "Chicago"},
    {38.6250f, -90.2030f, "St Louis"},
    {39.0850f, -94.5850f, "Kansas City"},
    {RAIL_END, 0, nullptr}};
static const RailPt RAIL_CHI_NW[] = {
    {41.8786f, -87.6400f, "Chicago"},
    {44.9470f, -93.0860f, "St Paul"},
    {RAIL_END, 0, nullptr}};
static const RailPt RAIL_CHI_E[] = {
    {41.8786f, -87.6400f, "Chicago"},
    {41.6380f, -83.5410f, "Toledo"},
    {42.3679f, -83.0640f, "Detroit"},
    {RAIL_END, 0, nullptr}};
static const RailPt RAIL_CHI_S[] = {
    {41.8786f, -87.6400f, "Chicago"},
    {39.7620f, -86.1600f, "Indianapolis"},
    {39.1100f, -84.5370f, "Cincinnati"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_STAR[] = {
    {47.5985f, -122.3280f, "Seattle"},
    {47.2410f, -122.4200f, nullptr},
    {46.7160f, -122.9540f, nullptr},
    {46.1420f, -122.9130f, nullptr},
    {45.6290f, -122.6860f, nullptr},
    {45.5189f, -122.6763f, "Portland"},
    {44.9320f, -123.0280f, nullptr},
    {44.6300f, -123.1030f, nullptr},
    {44.0551f, -123.0922f, "Eugene"},
    {43.2170f, -121.7830f, nullptr},
    {42.2256f, -121.7810f, nullptr},
    {41.2120f, -122.2710f, nullptr},
    {40.5830f, -122.3930f, nullptr},
    {39.7280f, -121.8370f, nullptr},
    {38.5840f, -121.5006f, "Sacramento"},
    {38.5436f, -121.7378f, nullptr},
    {38.0190f, -122.1380f, nullptr},
    {37.8404f, -122.2918f, "Emeryville"},
    {37.7936f, -122.2714f, "Oakland"},
    {37.3302f, -121.9026f, "San Jose"},
    {36.6777f, -121.6565f, nullptr},
    {35.6260f, -120.6910f, nullptr},
    {35.2760f, -120.6550f, "San Luis Obispo"},
    {34.4137f, -119.6850f, "Santa Barbara"},
    {34.1863f, -118.4487f, nullptr},
    {34.0562f, -118.2346f, "Los Angeles"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_CASC[] = {
    {47.9800f, -122.1970f, nullptr},
    {47.8110f, -122.3850f, nullptr},
    {47.5985f, -122.3280f, "Seattle"},
    {47.2410f, -122.4200f, nullptr},
    {46.1420f, -122.9130f, nullptr},
    {45.5189f, -122.6763f, "Portland"},
    {44.9320f, -123.0280f, nullptr},
    {44.0551f, -123.0922f, "Eugene"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_ZEPH[] = {
    {37.8404f, -122.2918f, "Emeryville"},
    {38.0190f, -122.1380f, nullptr},
    {38.5840f, -121.5006f, "Sacramento"},
    {39.3080f, -120.3750f, nullptr},
    {39.5280f, -119.8120f, "Reno"},
    {40.9690f, -117.7390f, nullptr},
    {40.8360f, -115.7630f, nullptr},
    {40.7616f, -111.9080f, "Salt Lake"},
    {40.2260f, -111.6610f, nullptr},
    {38.9920f, -110.1650f, nullptr},
    {39.0650f, -108.5670f, nullptr},
    {39.5480f, -107.3230f, nullptr},
    {39.7531f, -105.0002f, "Denver"},
    {40.2500f, -103.8030f, nullptr},
    {40.1970f, -100.6260f, nullptr},
    {40.8160f, -96.7110f, nullptr},
    {41.2496f, -95.9270f, "Omaha"},
    {41.0180f, -92.4110f, nullptr},
    {40.9450f, -90.3650f, nullptr},
    {41.8786f, -87.6400f, "Chicago"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_CHIEF[] = {
    {41.8786f, -87.6400f, "Chicago"},
    {41.7796f, -88.1458f, "Naperville"},
    {41.3500f, -88.8450f, nullptr},
    {40.9450f, -90.3650f, nullptr},
    {40.6300f, -91.3150f, nullptr},
    {39.0850f, -94.5850f, "Kansas City"},
    {39.0560f, -95.6770f, nullptr},
    {38.0680f, -97.3450f, nullptr},
    {37.7520f, -100.0180f, nullptr},
    {37.9850f, -103.5430f, nullptr},
    {36.9040f, -104.4390f, nullptr},
    {35.0820f, -106.6480f, "Albuquerque"},
    {35.5280f, -108.7420f, nullptr},
    {35.1980f, -111.6510f, "Flagstaff"},
    {35.1890f, -114.0530f, nullptr},
    {34.8380f, -114.6140f, nullptr},
    {34.8980f, -117.0250f, nullptr},
    {34.0000f, -117.3750f, nullptr},
    {33.8687f, -117.9226f, "Fullerton"},
    {34.0562f, -118.2346f, "Los Angeles"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_EAGLE[] = {
    {41.8786f, -87.6400f, "Chicago"},
    {41.5240f, -88.0790f, nullptr},
    {39.8010f, -89.6440f, nullptr},
    {38.6250f, -90.2030f, "St Louis"},
    {36.7600f, -90.3950f, nullptr},
    {34.7440f, -92.2870f, "Little Rock"},
    {33.4250f, -94.0430f, nullptr},
    {32.7763f, -96.8075f, "Dallas"},
    {32.7520f, -97.3270f, "Fort Worth"},
    {31.5490f, -97.1470f, nullptr},
    {30.2694f, -97.7569f, "Austin"},
    {29.4194f, -98.4781f, "San Antonio"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_SUN[] = {
    {29.9460f, -90.0780f, "New Orleans"},
    {29.7480f, -90.8160f, nullptr},
    {30.2270f, -93.2170f, nullptr},
    {29.7674f, -95.3677f, "Houston"},
    {29.4194f, -98.4781f, "San Antonio"},
    {29.3610f, -100.8960f, nullptr},
    {30.3600f, -103.6610f, nullptr},
    {31.7578f, -106.4950f, "El Paso"},
    {32.2680f, -107.7580f, nullptr},
    {32.2236f, -110.9668f, "Tucson"},
    {33.0580f, -112.0470f, nullptr},
    {32.6930f, -114.6150f, nullptr},
    {33.8240f, -116.5460f, nullptr},
    {34.0562f, -118.2346f, "Los Angeles"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_CONO[] = {
    {41.8786f, -87.6400f, "Chicago"},
    {41.5470f, -87.6690f, nullptr},
    {41.1190f, -87.8660f, nullptr},
    {40.1160f, -88.2430f, nullptr},
    {37.7270f, -89.2160f, nullptr},
    {35.1324f, -90.0593f, "Memphis"},
    {33.5180f, -90.1800f, nullptr},
    {32.2990f, -90.1910f, "Jackson MS"},
    {29.9460f, -90.0780f, "New Orleans"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_CRES[] = {
    {40.7506f, -73.9936f, "New York"},
    {39.9556f, -75.1820f, "Philadelphia"},
    {38.8970f, -77.0063f, "Washington"},
    {38.0300f, -78.4800f, nullptr},
    {37.4130f, -79.1460f, nullptr},
    {36.0730f, -79.7870f, nullptr},
    {35.2410f, -80.8220f, "Charlotte"},
    {34.8480f, -82.4130f, nullptr},
    {33.7990f, -84.3920f, "Atlanta"},
    {33.5120f, -86.8070f, "Birmingham"},
    {32.3640f, -88.7040f, nullptr},
    {29.9460f, -90.0780f, "New Orleans"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_SILV[] = {
    {40.7506f, -73.9936f, "New York"},
    {38.8970f, -77.0063f, "Washington"},
    {37.5340f, -77.4290f, "Richmond VA"},
    {35.9380f, -77.7980f, nullptr},
    {35.7750f, -78.6440f, "Raleigh"},
    {34.1950f, -79.7630f, nullptr},
    {32.8750f, -80.0080f, nullptr},
    {32.0810f, -81.0910f, "Savannah"},
    {30.3650f, -81.7240f, "Jacksonville"},
    {29.6470f, -81.6380f, nullptr},
    {28.5970f, -81.3510f, nullptr},
    {28.0440f, -81.9520f, nullptr},
    {27.4950f, -81.4410f, nullptr},
    {26.7120f, -80.0850f, nullptr},
    {26.1220f, -80.1700f, nullptr},
    {25.8495f, -80.2570f, "Miami"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_SSTAR[] = {
    {30.3650f, -81.7240f, "Jacksonville"},
    {28.5260f, -81.3810f, "Orlando"},
    {28.0440f, -81.9520f, nullptr},
    {27.9520f, -82.4500f, "Tampa"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_EMP[] = {
    {40.7506f, -73.9936f, "New York"},
    {41.1500f, -73.8680f, nullptr},
    {41.7000f, -73.9380f, nullptr},
    {42.6510f, -73.7450f, "Albany"},
    {43.1040f, -75.2230f, nullptr},
    {43.0510f, -76.1540f, "Syracuse"},
    {43.1540f, -77.6080f, "Rochester"},
    {42.8780f, -78.8730f, "Buffalo"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_LAKE[] = {
    {42.6510f, -73.7450f, "Albany"},
    {42.8780f, -78.8730f, "Buffalo"},
    {42.1290f, -80.0850f, nullptr},
    {41.5000f, -81.6960f, "Cleveland"},
    {41.6380f, -83.5410f, "Toledo"},
    {41.6190f, -86.7220f, nullptr},
    {41.8786f, -87.6400f, "Chicago"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_CAPL[] = {
    {38.8970f, -77.0063f, "Washington"},
    {39.6480f, -78.7580f, nullptr},
    {40.4450f, -80.0020f, "Pittsburgh"},
    {41.5000f, -81.6960f, "Cleveland"},
    {41.6380f, -83.5410f, "Toledo"},
    {41.8786f, -87.6400f, "Chicago"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_CARD[] = {
    {38.8970f, -77.0063f, "Washington"},
    {38.1490f, -79.0720f, nullptr},
    {37.7870f, -81.1830f, nullptr},
    {38.4190f, -82.4450f, nullptr},
    {39.1100f, -84.5370f, "Cincinnati"},
    {39.7620f, -86.1600f, "Indianapolis"},
    {40.4170f, -86.8930f, nullptr},
    {41.8786f, -87.6400f, "Chicago"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_WOLV[] = {
    {41.8786f, -87.6400f, "Chicago"},
    {41.5810f, -87.5000f, nullptr},
    {41.7210f, -86.9050f, nullptr},
    {42.2950f, -85.5840f, nullptr},
    {42.3180f, -85.1880f, nullptr},
    {42.2460f, -84.4010f, nullptr},
    {42.2880f, -83.7430f, nullptr},
    {42.3679f, -83.0640f, "Detroit"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_VA[] = {
    {38.8970f, -77.0063f, "Washington"},
    {38.1470f, -77.5100f, nullptr},
    {37.5340f, -77.4290f, "Richmond VA"},
    {37.2700f, -77.3120f, nullptr},
    {36.8430f, -76.2760f, "Norfolk"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_BUILD[] = {
    {41.8786f, -87.6400f, "Chicago"},
    {42.0745f, -87.8056f, "Glenview"},
    {43.0344f, -87.9170f, "Milwaukee"},
    {43.7500f, -87.7140f, nullptr},
    {44.9470f, -93.0860f, "St Paul"},
    {46.8770f, -96.7880f, nullptr},
    {48.2320f, -101.2960f, nullptr},
    {48.5540f, -109.6770f, nullptr},
    {48.3070f, -114.2640f, nullptr},
    {47.6580f, -117.4260f, "Spokane"},
    {47.4230f, -120.3100f, nullptr},
    {47.9800f, -122.1970f, nullptr},
    {47.5985f, -122.3280f, "Seattle"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_BUILD2[] = {
    {47.6580f, -117.4260f, "Spokane"},
    {46.2110f, -119.1370f, nullptr},
    {45.5189f, -122.6763f, "Portland"},
    {RAIL_END, 0, nullptr}};

static const RailPt RAIL_BRIGHT[] = {
    {25.8495f, -80.2570f, "Miami"},
    {26.1190f, -80.1700f, nullptr},
    {26.3450f, -80.1240f, nullptr},
    {26.7120f, -80.0850f, nullptr},
    {27.4460f, -80.3250f, nullptr},
    {28.0800f, -80.6300f, nullptr},
    {28.3920f, -80.7580f, "Cocoa"},
    {28.4500f, -80.9200f, nullptr},
    {28.5260f, -81.3810f, "Orlando"},
    {RAIL_END, 0, nullptr}};

static const RailPt *const RAIL_ALL[] = {
    RAIL_SURF, RAIL_COAST, RAIL_JOAQ, RAIL_JOAQ2, RAIL_CAP, RAIL_NEC, RAIL_HIA, RAIL_CHI_W,
    RAIL_STAR, RAIL_CASC, RAIL_ZEPH, RAIL_CHIEF, RAIL_EAGLE, RAIL_SUN, RAIL_CONO, RAIL_CRES,
    RAIL_SILV, RAIL_SSTAR, RAIL_EMP, RAIL_LAKE, RAIL_CAPL, RAIL_CARD, RAIL_WOLV, RAIL_VA,
    RAIL_BUILD, RAIL_BUILD2, RAIL_BRIGHT};

static const char *const RAIL_NAME[] = {
    "Surfliner", "Coast", "San Joaquin", "San Joaquin", "Capitol", "Northeast", "Hiawatha", "West",
    "Starlight", "Cascades", "Zephyr", "SW Chief", "Texas Eagle", "Sunset", "City of NO", "Crescent",
    "Silver", "Silver Star", "Empire", "Lake Shore", "Capitol Ltd", "Cardinal", "Wolverine", "Virginia",
    "Builder", "Builder", "Brightline"};

TagBox tagBox[48];
int nTagBox;
int nStationTags = 0;
int nMk = 0;
int mkGx[MAX_T], mkGy[MAX_T], mkGw[MAX_T], mkGh[MAX_T];
int mkTx[MAX_T], mkTy[MAX_T], mkTw[MAX_T], mkTh[MAX_T];
char mkNum[MAX_T][12];
int markX[36], markY[36], nMarks;

bool railNearHome(const RailPt *p) {
  float lim = (float)rangeNm + 60.0f;
  for (; p->lat < 900.0f; p++)
    if (viewDist(p->lat, p->lon) <= lim) return true;
  return false;
}

int clipCode(float x, float y) {
  int c = 0;
  if (x < 0) c |= 1;
  if (x > PW - 1) c |= 2;
  if (y < 0) c |= 4;
  if (y > MAP_H - 1) c |= 8;
  return c;
}

bool clipMap(float &x0, float &y0, float &x1, float &y1) {
  int c0 = clipCode(x0, y0), c1 = clipCode(x1, y1);
  for (int n = 0; n < 8; n++) {
    if (!(c0 | c1)) return true;
    if (c0 & c1) return false;
    int c = c0 ? c0 : c1;
    float x = x0, y = y0, dx = x1 - x0, dy = y1 - y0;
    if (c & 1) {
      if (dx == 0) return false;
      y = y0 + dy * (0 - x0) / dx;
      x = 0;
    } else if (c & 2) {
      if (dx == 0) return false;
      y = y0 + dy * ((float)(PW - 1) - x0) / dx;
      x = (float)(PW - 1);
    } else if (c & 4) {
      if (dy == 0) return false;
      x = x0 + dx * (0 - y0) / dy;
      y = 0;
    } else {
      if (dy == 0) return false;
      x = x0 + dx * ((float)(MAP_H - 1) - y0) / dy;
      y = (float)(MAP_H - 1);
    }
    if (c == c0) {
      x0 = x;
      y0 = y;
      c0 = clipCode(x0, y0);
    } else {
      x1 = x;
      y1 = y;
      c1 = clipCode(x1, y1);
    }
  }
  return false;
}

void railFat(int x0, int y0, int x1, int y1, uint16_t col, int half) {
  float dx = (float)(x1 - x0), dy = (float)(y1 - y0);
  float len = hypotf(dx, dy);
  if (len < 1) {
    int yy = y0;
    if (yy < 0) yy = 0;
    if (yy > MAP_H - 1) yy = MAP_H - 1;
    gfx->fillCircle(x0, yy, half + 1, col);
    return;
  }
  float nx = -dy / len, ny = dx / len;
  for (int i = -half; i <= half; i++) {
    int ay = y0 + (int)(ny * i);
    int by = y1 + (int)(ny * i);
    if (ay < 0) ay = 0;
    if (ay > MAP_H - 1) ay = MAP_H - 1;
    if (by < 0) by = 0;
    if (by > MAP_H - 1) by = MAP_H - 1;
    gfx->drawLine(x0 + (int)(nx * i), ay, x1 + (int)(nx * i), by, col);
  }
}

void strokeRail(float aLat, float aLon, float bLat, float bLon, uint16_t col) {
  float x0, y0, x1, y1;
  llToXY(aLat, aLon, x0, y0);
  llToXY(bLat, bLon, x1, y1);
  if (!clipMap(x0, y0, x1, y1)) return;
  int ax = (int)x0, ay = (int)y0, bx = (int)x1, by = (int)y1;
  railFat(ax, ay, bx, by, COL_BLK, 3);
  railFat(ax, ay, bx, by, col, 1);
  float dx = x1 - x0, dy = y1 - y0;
  float len = hypotf(dx, dy);
  if (len < 16) return;
  float ux = dx / len, uy = dy / len, px = -uy, py = ux;
  for (float s = 10; s < len; s += 18) {
    int cx = (int)(x0 + ux * s), cy = (int)(y0 + uy * s);
    if (cy < 3 || cy > MAP_H - 4) continue;
    gfx->drawLine(cx + (int)(px * 5), cy + (int)(py * 5), cx - (int)(px * 5), cy - (int)(py * 5), COL_BLK);
  }
}

void drawCorridor(const RailPt *p, uint16_t col) {
  if (!railNearHome(p)) return;
  const RailPt *prev = nullptr;
  for (; p->lat < 900.0f; p++) {
    if (prev) strokeRail(prev->lat, prev->lon, p->lat, p->lon, col);
    prev = p;
  }
}

bool tagHits(int x, int y, int w, int h) {
  for (int i = 0; i < nTagBox; i++)
    if (x < tagBox[i].x + tagBox[i].w + 4 && x + w + 4 > tagBox[i].x && y < tagBox[i].y + tagBox[i].h + 3 &&
        y + h + 3 > tagBox[i].y)
      return true;
  return false;
}

void tagAdd(int x, int y, int w, int h) {
  if (nTagBox >= 48) return;
  tagBox[nTagBox].x = x;
  tagBox[nTagBox].y = y;
  tagBox[nTagBox].w = w;
  tagBox[nTagBox].h = h;
  nTagBox++;
}

bool tagSlot(int ix, int iy, int w, int h, int xMin, int xMax, int yMin, int yMax, int &ox, int &oy) {
  int dx[8] = {16, -(w + 16), 16, -(w + 16), 18, -(w + 18), -(w / 2), -(w / 2)};
  int dy[8] = {-(h + 12), -(h + 12), 12, 12, -(h / 2), -(h / 2), -(h + 14), 14};
  for (int s = 0; s < 8; s++) {
    int x = constrain(ix + dx[s], xMin, xMax - w);
    int y = constrain(iy + dy[s], yMin, yMax - h);
    if (x < xMin || y < yMin || x + w > xMax || y + h > yMax) continue;
    if (tagHits(x, y, w, h)) continue;
    ox = x;
    oy = y;
    return true;
  }
  ox = constrain(ix + 16, xMin, xMax - w);
  oy = constrain(iy - h - 12, yMin, yMax - h);
  return true;
}

bool alreadyMarked(int ix, int iy) {
  for (int i = 0; i < nMarks; i++)
    if (abs(markX[i] - ix) + abs(markY[i] - iy) < 16) return true;
  return false;
}

void stampStation(int ix, int iy) {
  gfx->fillCircle(ix, iy, 7, COL_BLK);
  gfx->drawCircle(ix, iy, 7, COL_WHT);
  gfx->fillCircle(ix, iy, 3, COL_HOT);
  if (nMarks < 36) {
    markX[nMarks] = ix;
    markY[nMarks] = iy;
    nMarks++;
  }
}

const char *codeForStation(const char *name, float lat, float lon) {
  for (int i = 0; stops[i].code; i++)
    if (name && !strcasecmp(stops[i].city, name)) return stops[i].code;
  float bd = 8.0f;
  int k = -1;
  for (int i = 0; stops[i].code; i++) {
    float dLat = (stops[i].lat - lat) * 60.0f;
    float dLon = (stops[i].lon - lon) * 60.0f * cosf(lat * 0.0174533f);
    float d = sqrtf(dLat * dLat + dLon * dLon);
    if (d < bd) {
      bd = d;
      k = i;
    }
  }
  return k >= 0 ? stops[k].code : nullptr;
}

bool placeMapText(int ix, int iy, const char *text, int sz) {
  if (!text || !text[0]) return false;
  gfx->setTextSize(sz);
  int16_t x1, y1;
  uint16_t w, h;
  gfx->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int bw = (int)w + 8, bh = (int)h + 6;
  int bx, by;
  tagSlot(ix, iy, bw, bh, 6, PW - 6, 42, MAP_H - 4, bx, by);
  if (tagHits(bx, by, bw, bh) && nTagBox > 0) {
    if (sz > 1) return false;
  }
  tagAdd(bx, by, bw, bh);
  uint16_t ink = RGB565(255, 230, 70);
  gfx->fillRoundRect(bx - 3, by - 3, bw + 6, bh + 6, 3, COL_BLK);
  gfx->drawRoundRect(bx - 3, by - 3, bw + 6, bh + 6, 3, ink);
  gfx->setTextColor(ink, COL_BLK);
  gfx->setCursor(bx + 3, by + 3);
  gfx->print(text);
  return true;
}

void drawStationNames(const RailPt *p) {
  if (!railNearHome(p)) return;
  int sz = rangeNm > 50 ? 1 : 2;
  for (; p->lat < 900.0f; p++) {
    if (!p->name) continue;
    if (viewDist(p->lat, p->lon) > (float)rangeNm + 8.0f) continue;
    float x, y;
    llToXY(p->lat, p->lon, x, y);
    if (x < 16 || x > PW - 16 || y < 36 || y > MAP_H - 12) continue;
    int ix = (int)x, iy = (int)y;
    if (alreadyMarked(ix, iy)) continue;
    stampStation(ix, iy);
    if (placeMapText(ix, iy, p->name, sz)) continue;
    if (sz > 1 && placeMapText(ix, iy, p->name, 1)) continue;
    placeMapText(ix, iy, codeForStation(p->name, p->lat, p->lon), 1);
  }
}

float distPts(float lat1, float lon1, float lat2, float lon2) {
  float mid = 0.5f * (lat1 + lat2);
  float dLat = (lat2 - lat1) * 60.0f;
  float dLon = (lon2 - lon1) * 60.0f * cosf(mid * 0.0174533f);
  return sqrtf(dLat * dLat + dLon * dLon);
}

void drawLiveRails(uint16_t col) {
  for (int i = 0; i < nTrains; i++) {
    int prev = -1;
    for (int s = 0; s < trains[i].nStops; s++) {
      int k = findStop(trains[i].stops[s]);
      if (k < 0) continue;
      if (prev >= 0 && distPts(stops[prev].lat, stops[prev].lon, stops[k].lat, stops[k].lon) < 90.0f)
        strokeRail(stops[prev].lat, stops[prev].lon, stops[k].lat, stops[k].lon, col);
      prev = k;
    }
  }
}

void drawTracks() {
  uint16_t col = dayMode ? RGB565(255, 196, 0) : RGB565(255, 220, 48);
  nTagBox = 0;
  nMarks = 0;
  int n = (int)(sizeof(RAIL_ALL) / sizeof(RAIL_ALL[0]));
  for (int i = 0; i < n; i++) drawCorridor(RAIL_ALL[i], col);
  for (int i = 0; i < MTA_RAIL_N; i++) drawCorridor(MTA_RAIL[i], col);
  drawLiveRails(col);
  for (int i = 0; i < n; i++) drawStationNames(RAIL_ALL[i]);
  for (int i = 0; i < MTA_RAIL_N; i++) drawStationNames(MTA_RAIL[i]);
  nStationTags = nTagBox;
}

void drawTrainMark(const Train &t, bool sel) {
  float x, y;
  llToXY(t.lat, t.lon, x, y);
  if (x < 8 || x > PW - 8 || y < 8 || y > MAP_H - 8) return;
  int ix = (int)x, iy = (int)y;
  uint16_t col = COL_AMB;
  if (!strcasecmp(t.prov, "LIRR")) col = dayMode ? RGB565(0, 70, 170) : RGB565(70, 150, 255);
  else if (!strcasecmp(t.prov, "MNR")) col = dayMode ? RGB565(0, 120, 50) : RGB565(40, 210, 90);
  else if (!strcasecmp(t.prov, "SEPTA")) col = dayMode ? RGB565(0, 90, 160) : RGB565(80, 170, 255);
  else if (!strcasecmp(t.prov, "MARC")) col = dayMode ? RGB565(160, 30, 30) : RGB565(255, 80, 70);
  else if (!strcasecmp(t.prov, "VRE")) col = dayMode ? RGB565(140, 70, 0) : RGB565(255, 150, 40);
  else if (!strcasecmp(t.prov, "TRI")) col = dayMode ? RGB565(0, 110, 110) : RGB565(40, 210, 200);
  else if (!strcasecmp(t.prov, "Amtrak") || !t.prov[0]) col = COL_CYN;
  if (sel) gfx->drawCircle(ix, iy, 12, COL_MAG);
  float hd = headDeg(t.head) * 0.0174533f;
  int dx = (int)(sinf(hd) * 10), dy = (int)(-cosf(hd) * 10);
  gfx->drawLine(ix - dx, iy - dy, ix + dx, iy + dy, col);
  gfx->drawLine(ix - dx, iy - dy + 1, ix + dx, iy + dy + 1, col);
  gfx->fillCircle(ix, iy, 5, col);
  gfx->setTextSize(2);
  int16_t x1, y1;
  uint16_t tw, th;
  gfx->getTextBounds(t.num, 0, 0, &x1, &y1, &tw, &th);
  int bw = (int)tw + 8, bh = (int)th + 4;
  int bx, by;
  if (!tagSlot(ix, iy, bw, bh, 6, PW - 6, 42, MAP_H - 4, bx, by)) {
    bx = constrain(ix + 10, 6, PW - 6 - bw);
    by = constrain(iy - bh - 8, 42, MAP_H - 4 - bh);
  }
  tagAdd(bx, by, bw, bh);
  gfx->fillRoundRect(bx, by, bw, bh, 3, COL_BLK);
  gfx->setTextColor(COL_YEL, COL_BLK);
  gfx->setCursor(bx + 3, by + 2);
  gfx->print(t.num);
  if (nMk < MAX_T) {
    int pad = 14;
    strlcpy(mkNum[nMk], t.num, 12);
    mkGx[nMk] = ix - abs(dx) - pad;
    mkGy[nMk] = iy - abs(dy) - pad;
    mkGw[nMk] = abs(dx) * 2 + pad * 2;
    mkGh[nMk] = abs(dy) * 2 + pad * 2;
    mkTx[nMk] = bx;
    mkTy[nMk] = by;
    mkTw[nMk] = bw;
    mkTh[nMk] = bh;
    nMk++;
  }
}

bool onCompass(int x, int y) {
  if (abs(x - CX) <= 16 && y < 40) return true;
  if (abs(x - CX) <= 16 && y > MAP_H - 40) return true;
  if (x < 48 && abs(y - CY) <= 16) return true;
  if (x > PW - 48 && abs(y - CY) <= 16) return true;
  return false;
}

void mapPut(int x, int y) {
  if ((unsigned)x >= (unsigned)PW || (unsigned)y >= (unsigned)MAP_H) return;
  if (onCompass(x, y)) return;
  uint16_t c = COL_BG;
  if (mapOk && mapFb) c = mapFb[y * PW + x];
  gfx->drawPixel(x, y, c);
}

void restoreSweep(float deg) {
  float rad = deg * 0.0174533f;
  int x0 = CX, y0 = CY;
  int x1 = CX + (int)(sinf(rad) * RR);
  int y1 = CY - (int)(cosf(rad) * RR);
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  for (;;) {
    mapPut(x0, y0);
    mapPut(x0 + 1, y0);
    mapPut(x0, y0 + 1);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void strokeSweep(float deg) {
  float rad = deg * 0.0174533f;
  int x0 = CX, y0 = CY;
  int x1 = CX + (int)(sinf(rad) * RR);
  int y1 = CY - (int)(cosf(rad) * RR);
  uint16_t ink = dayMode ? RGB565(90, 48, 0) : COL_YEL;
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  for (;;) {
    if (!onCompass(x0, y0) && (unsigned)x0 < (unsigned)PW && (unsigned)y0 < (unsigned)MAP_H) {
      gfx->drawPixel(x0, y0, ink);
      gfx->drawPixel(x0 + 1, y0, ink);
    }
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void tickSweep() {
  restoreSweep(prevSweepDeg);
  prevSweepDeg = sweepDeg;
  strokeSweep(sweepDeg);
  gfx->fillCircle(CX, CY, 5, COL_MAG);
}

void drawHome();
void drawList();
void drawDetail();
void drawDepot();
void drawClock();
void drawTube();
void drawSettings();
void drawAbout();
void drawWifi();
void openWifi();

void paintHomeClock(bool force) {
  time_t now = time(nullptr);
  struct tm ti;
  lastClock = millis();
  char line[40];
  line[0] = 0;
  if (now >= 1600000000LL && localtime_r(&now, &ti)) {
    int hr = ti.tm_hour > 12 ? ti.tm_hour - 12 : (ti.tm_hour ? ti.tm_hour : 12);
    static const char *wd[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char *mo[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    snprintf(line, sizeof(line), "%d:%02d %s  %s %s %d", hr, ti.tm_min, ti.tm_hour >= 12 ? "PM" : "AM",
             wd[ti.tm_wday], mo[ti.tm_mon], ti.tm_mday);
  }
  static char shown[40] = "\n";
  if (!force && !strcmp(shown, line)) return;
  strlcpy(shown, line, sizeof(shown));
  gfx->fillRoundRect(400, 6, 392, 32, 4, COL_BLK);
  if (!line[0]) return;
  gfx->setTextColor(COL_YEL, COL_BLK);
  gfx->setTextSize(2);
  gfx->setCursor(410, 12);
  gfx->print(line);
}

void drawHomeLayer() {
  page = PG_HOME;
  blitMap();
  drawTracks();
  nMk = 0;
  for (int i = 0; i < nTrains; i++) drawTrainMark(trains[i], i == selected);
  int known = findStop(homeStop);
  gfx->fillRoundRect(8, 6, 300, 32, 4, COL_BLK);
  gfx->setTextColor(COL_YEL, COL_BLK);
  gfx->setTextSize(2);
  gfx->setCursor(16, 12);
  gfx->print(trainsErr[0] ? trainsErr : "LIVE");
  if (known >= 0) {
    gfx->setTextColor(COL_WHT, COL_BLK);
    gfx->setCursor(160, 12);
    gfx->print(stops[known].city);
  }
  paintHomeClock(true);
}

bool hitsStation(int x, int y, int w, int h) {
  for (int i = 0; i < nStationTags; i++) {
    int x0 = tagBox[i].x - 3, y0 = tagBox[i].y - 3;
    int x1 = tagBox[i].x + tagBox[i].w + 6, y1 = tagBox[i].y + tagBox[i].h + 6;
    if (x < x1 && x + w > x0 && y < y1 && y + h > y0) return true;
  }
  return false;
}

void restoreMk(int x, int y, int w, int h) {
  if (w < 1 || h < 1) return;
  if (x < EDGE) {
    w -= EDGE - x;
    x = EDGE;
  }
  if (y < 38) {
    h -= 38 - y;
    y = 38;
  }
  if (x + w > PW) w = PW - x;
  if (y + h > MAP_H) h = MAP_H - y;
  if (w < 1 || h < 1) return;
  if (hitsStation(x, y, w, h)) return;
  if (mapOn && mapOk && mapFb) {
    static uint16_t row[800];
    for (int r = 0; r < h; r++) {
      memcpy(row, mapFb + (size_t)(y + r) * PW + x, (size_t)w * 2);
      gfx->draw16bitRGBBitmap(x, y + r, row, w, 1);
    }
  } else
    gfx->fillRect(x, y, w, h, COL_BG);
  for (int s = 0; s < nMarks; s++) {
    if (markX[s] < x || markX[s] >= x + w || markY[s] < y || markY[s] >= y + h) continue;
    gfx->fillCircle(markX[s], markY[s], 7, COL_BLK);
    gfx->drawCircle(markX[s], markY[s], 7, COL_WHT);
    gfx->fillCircle(markX[s], markY[s], 3, COL_HOT);
  }
}

void refreshHomeTrains() {
  if (mapOx || mapOy) return;
  int nOld = nMk;
  if (nOld > MAX_T) nOld = MAX_T;
  bool keep[MAX_T];
  int matchAt[MAX_T];
  for (int i = 0; i < nOld; i++) keep[i] = false;
  int nNow = nTrains;
  if (nNow > MAX_T) nNow = MAX_T;
  for (int i = 0; i < nNow; i++) {
    matchAt[i] = -1;
    float fx, fy;
    llToXY(trains[i].lat, trains[i].lon, fx, fy);
    if (fx < 8 || fx > PW - 8 || fy < 8 || fy > MAP_H - 8) continue;
    int ix = (int)fx, iy = (int)fy;
    float hd = headDeg(trains[i].head) * 0.0174533f;
    int dx = (int)(sinf(hd) * 10), dy = (int)(-cosf(hd) * 10);
    int pad = 14;
    int gx = ix - abs(dx) - pad, gy = iy - abs(dy) - pad;
    int gw = abs(dx) * 2 + pad * 2, gh = abs(dy) * 2 + pad * 2;
    for (int k = 0; k < nOld; k++) {
      if (keep[k]) continue;
      if (strcmp(mkNum[k], trains[i].num)) continue;
      if (mkGx[k] != gx || mkGy[k] != gy || mkGw[k] != gw || mkGh[k] != gh) continue;
      keep[k] = true;
      matchAt[i] = k;
      break;
    }
  }
  for (int k = 0; k < nOld; k++) {
    if (keep[k]) continue;
    restoreMk(mkGx[k], mkGy[k], mkGw[k], mkGh[k]);
    restoreMk(mkTx[k], mkTy[k], mkTw[k], mkTh[k]);
  }
  int wri = 0;
  for (int k = 0; k < nOld; k++) {
    if (!keep[k]) continue;
    if (wri != k) {
      strlcpy(mkNum[wri], mkNum[k], 12);
      mkGx[wri] = mkGx[k];
      mkGy[wri] = mkGy[k];
      mkGw[wri] = mkGw[k];
      mkGh[wri] = mkGh[k];
      mkTx[wri] = mkTx[k];
      mkTy[wri] = mkTy[k];
      mkTw[wri] = mkTw[k];
      mkTh[wri] = mkTh[k];
    }
    for (int i = 0; i < nNow; i++)
      if (matchAt[i] == k) matchAt[i] = wri;
    wri++;
  }
  nMk = wri;
  nTagBox = nStationTags;
  for (int i = 0; i < nNow; i++) {
    if (matchAt[i] >= 0) continue;
    drawTrainMark(trains[i], i == selected);
  }
  char msg[24];
  snprintf(msg, sizeof(msg), "%-11s", trainsErr[0] ? trainsErr : "LIVE");
  static char liveShown[24] = "";
  if (strcmp(liveShown, msg)) {
    strlcpy(liveShown, msg, sizeof(liveShown));
    gfx->setTextColor(COL_YEL, COL_BLK);
    gfx->setTextSize(2);
    gfx->setCursor(16, 12);
    gfx->print(msg);
  }
}

void drawHome() {
  drawHomeLayer();
  drawZoomBar();
  drawHomeMenu();
}

void drawList() {
  page = PG_LIST;
  gfx->fillScreen(COL_BG);
  drawBackHome();
  gfx->setTextColor(COL_HOT, COL_BG);
  gfx->setTextSize(3);
  centre("CORRIDOR", CX, 16);
  if (!nTrains) {
    gfx->setTextColor(COL_MID, COL_BG);
    gfx->setTextSize(2);
    centre("none in range", CX, 200);
    return;
  }
  if (listTop < 0 || listTop >= nTrains) listTop = 0;
  int rows = min(8, nTrains - listTop);
  for (int i = 0; i < rows; i++) {
    Train &t = trains[listTop + i];
    int y = 64 + i * 48;
    gfx->setTextColor(COL_HOT, COL_BG);
    gfx->setTextSize(2);
    gfx->setCursor(24, y);
    gfx->print(t.num);
    gfx->setTextColor(COL_TXT, COL_BG);
    char line[64];
    snprintf(line, sizeof(line), "%s > %s", t.orig, t.dest);
    gfx->setCursor(24, y + 22);
    gfx->print(line);
    gfx->setTextColor(t.lateMin ? COL_LATE : COL_ON, COL_BG);
    gfx->setCursor(620, y);
    if (t.lateMin)
      gfx->printf("%d late", t.lateMin);
    else
      gfx->print("on time");
  }
  gfx->setTextColor(COL_MID, COL_BG);
  centre(nTrains > 8 ? "tap bottom for more" : "tap a row", CX, 450);
}

void drawDetail() {
  page = PG_DETAIL;
  gfx->fillScreen(COL_BG);
  drawBackHome();
  if (selected < 0 || selected >= nTrains) {
    gfx->setTextColor(COL_HOT, COL_BG);
    gfx->setTextSize(2);
    centre("no train", CX, 200);
    return;
  }
  Train &t = trains[selected];
  gfx->setTextColor(COL_HOT, COL_BG);
  gfx->setTextSize(4);
  centre(t.num, CX, 52);
  gfx->setTextColor(COL_TXT, COL_BG);
  gfx->setTextSize(2);
  centre(t.route, CX, 108);
  if (t.prov[0]) {
    gfx->setTextColor(COL_MID, COL_BG);
    centre(t.prov, CX, 138);
  }
  char line[64];
  gfx->setTextColor(COL_CYN, COL_BG);
  snprintf(line, sizeof(line), "departed from  %s", t.orig[0] ? t.orig : "----");
  centre(line, CX, 176);
  gfx->setTextColor(COL_TXT, COL_BG);
  snprintf(line, sizeof(line), "going to  %s", t.dest[0] ? t.dest : "----");
  centre(line, CX, 206);
  gfx->setTextColor(COL_YEL, COL_BG);
  snprintf(line, sizeof(line), "next stop  %s", t.next[0] ? t.next : "----");
  centre(line, CX, 238);
  gfx->setTextColor(COL_TXT, COL_BG);
  if (strcasecmp(t.prov, "LIRR") && strcasecmp(t.prov, "MNR")) {
    gfx->setTextColor(COL_AMB, COL_BG);
    if (t.head[0])
      snprintf(line, sizeof(line), "moving  %s", trainDir(t));
    else if (t.mph < 3)
      snprintf(line, sizeof(line), "stopped");
    else
      snprintf(line, sizeof(line), "moving");
    centre(line, CX, 268);
    gfx->setTextColor(COL_TXT, COL_BG);
    snprintf(line, sizeof(line), "SPEED  %d mph", t.mph);
    centre(line, CX, 300);
  }
  if (t.lateMin) {
    gfx->setTextColor(COL_LATE, COL_BG);
    gfx->setTextSize(3);
    snprintf(line, sizeof(line), "%d min late", t.lateMin);
    centre(line, CX, 350);
  } else {
    gfx->setTextColor(COL_ON, COL_BG);
    gfx->setTextSize(3);
    centre("ON TIME", CX, 350);
  }
}

void drawDepot() {
  page = PG_DEPOT;
  gfx->fillScreen(COL_BG);
  drawBackHome();
  gfx->setTextColor(COL_AMB, COL_BG);
  gfx->setTextSize(3);
  centre("DEPOT", CX, 12);
  int known = findStop(homeStop);
  gfx->fillRoundRect(16, 56, 370, 90, 6, COL_DIM);
  gfx->setTextColor(COL_YEL, COL_DIM);
  gfx->setTextSize(3);
  gfx->setCursor(28, 68);
  gfx->print(homeStop);
  gfx->setTextColor(COL_TXT, COL_DIM);
  gfx->setTextSize(2);
  gfx->setCursor(28, 116);
  gfx->print(known >= 0 ? stops[known].city : "home stop");
  gfx->fillRoundRect(404, 56, 380, 90, 6, COL_DIM);
  gfx->setTextColor(COL_AMB, COL_DIM);
  gfx->setTextSize(2);
  gfx->setCursor(420, 64);
  gfx->print("NEAREST");
  if (nTrains) {
    gfx->setTextColor(COL_HOT, COL_DIM);
    gfx->setCursor(420, 92);
    gfx->print(trains[0].num);
    char line[40];
    snprintf(line, sizeof(line), "%.1fNM  %s", distNm(trains[0].lat, trains[0].lon), trains[0].next);
    gfx->setTextColor(COL_TXT, COL_DIM);
    gfx->setCursor(420, 120);
    gfx->print(line);
  } else {
    gfx->setTextColor(COL_MID, COL_DIM);
    gfx->setCursor(420, 100);
    gfx->print("none in range");
  }
  int y = 164;
  for (int i = 0; i < nTrains && i < 5; i++) {
    Train &t = trains[i];
    uint16_t row = dayMode ? ((i % 2) ? RGB565(220, 212, 196) : RGB565(208, 200, 184))
                           : ((i % 2) ? RGB565(16, 24, 34) : RGB565(22, 32, 44));
    gfx->fillRect(0, y, PW, 56, row);
    gfx->setTextColor(COL_YEL);
    gfx->setTextSize(2);
    gfx->setCursor(24, y + 8);
    gfx->print(t.num);
    gfx->setTextColor(COL_TXT);
    gfx->setCursor(160, y + 8);
    gfx->printf("%s > %s", t.orig, t.dest);
    gfx->setTextColor(t.lateMin ? COL_LATE : COL_ON);
    gfx->setCursor(24, y + 32);
    if (t.lateMin)
      gfx->printf("%d late   next %s", t.lateMin, t.next);
    else
      gfx->printf("on time   next %s", t.next);
    y += 56;
  }
}

void paintClockTime(bool force) {
  time_t now = time(nullptr);
  struct tm ti;
  if (now < 1600000000LL || !localtime_r(&now, &ti)) return;
  lastClock = millis();
  int hr = ti.tm_hour > 12 ? ti.tm_hour - 12 : (ti.tm_hour ? ti.tm_hour : 12);
  char line[56];
  snprintf(line, sizeof(line), "%2d:%02d:%02d %s", hr, ti.tm_min, ti.tm_sec, ti.tm_hour >= 12 ? "PM" : "AM");
  static char shown[56] = "";
  if (!force && !strcmp(shown, line)) return;
  strlcpy(shown, line, sizeof(shown));
  gfx->setTextColor(COL_YEL, COL_BG);
  gfx->setTextSize(4);
  centre(line, CX, 92);
}

void drawClock() {
  page = PG_CLOCK;
  gfx->fillScreen(COL_BG);
  drawBackHome();
  gfx->setTextColor(COL_HOT, COL_BG);
  gfx->setTextSize(3);
  centre("CLOCK", CX, 12);
  time_t now = time(nullptr);
  struct tm ti;
  if (now < 1600000000LL || !localtime_r(&now, &ti)) {
    gfx->setTextColor(COL_MID, COL_BG);
    gfx->setTextSize(2);
    centre(wifiLinked() ? "waiting for live clock" : "join wifi for the clock", CX, 220);
    lastClock = millis();
    return;
  }
  paintClockTime(true);
  char line[56];
  static const char *wd[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  static const char *mo[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  snprintf(line, sizeof(line), "%s  %s %d", wd[ti.tm_wday], mo[ti.tm_mon], ti.tm_mday);
  gfx->setTextColor(COL_TXT, COL_BG);
  gfx->setTextSize(3);
  centre(line, CX, 160);
  int known = findStop(homeStop);
  gfx->fillRoundRect(80, 220, 640, 80, 8, COL_DIM);
  gfx->setTextColor(COL_AMB, COL_DIM);
  gfx->setTextSize(2);
  centre(homeStop, CX, 232);
  gfx->setTextColor(COL_YEL, COL_DIM);
  gfx->setTextSize(3);
  centre(known >= 0 ? stops[known].city : "home stop", CX, 264);
  gfx->setTextColor(COL_TXT, COL_BG);
  gfx->setTextSize(2);
  if (nTrains) {
    snprintf(line, sizeof(line), "nearest  %s   %.0f NM", trains[0].num, distNm(trains[0].lat, trains[0].lon));
    centre(line, CX, 330);
    if (trains[0].next[0]) {
      snprintf(line, sizeof(line), "next stop  %s", trains[0].next);
      gfx->setTextColor(COL_CYN, COL_BG);
      centre(line, CX, 366);
    }
  } else
    centre("no trains in range", CX, 330);
}

void fatLine(int x0, int y0, int x1, int y1, uint16_t c, int w) {
  float dx = (float)(x1 - x0), dy = (float)(y1 - y0);
  float len = hypotf(dx, dy);
  if (len < 1) {
    gfx->fillCircle(x0, y0, w / 2, c);
    return;
  }
  float nx = -dy / len, ny = dx / len;
  int h = w / 2;
  for (int i = -h; i <= h; i++)
    gfx->drawLine(x0 + (int)(nx * i), y0 + (int)(ny * i), x1 + (int)(nx * i), y1 + (int)(ny * i), c);
}

void octiLine(int x0, int y0, int x1, int y1, uint16_t c, int w) {
  int dx = x1 - x0, dy = y1 - y0;
  if (!dx || !dy || abs(abs(dx) - abs(dy)) < 6) {
    fatLine(x0, y0, x1, y1, c, w);
    return;
  }
  int mx, my;
  if (abs(dx) > abs(dy)) {
    int run = abs(dx) - abs(dy);
    mx = x0 + (dx > 0 ? run : -run);
    my = y0;
  } else {
    int rise = abs(dy) - abs(dx);
    mx = x0;
    my = y0 + (dy > 0 ? rise : -rise);
  }
  fatLine(x0, y0, mx, my, c, w);
  fatLine(mx, my, x1, y1, c, w);
}

bool tubeAdd(int li, const char *code) {
  if (li < 0 || li >= MAX_TL || !code || !code[0]) return false;
  TubeLine &L = tubeLines[li];
  if (L.nn >= MAX_TN) return false;
  for (int i = 0; i < L.nn; i++)
    if (!strcasecmp(L.n[i].code, code)) return false;
  int k = findStop(code);
  if (k < 0) return false;
  strlcpy(L.n[L.nn].code, stops[k].code, sizeof(L.n[L.nn].code));
  strlcpy(L.n[L.nn].name, stops[k].city, sizeof(L.n[L.nn].name));
  L.n[L.nn].lat = stops[k].lat;
  L.n[L.nn].lon = stops[k].lon;
  L.nn++;
  return true;
}

bool tubeAddPt(int li, const char *name, float lat, float lon) {
  if (li < 0 || li >= MAX_TL || !name || !name[0]) return false;
  TubeLine &L = tubeLines[li];
  if (L.nn >= MAX_TN) return false;
  for (int i = 0; i < L.nn; i++)
    if (!strcasecmp(L.n[i].name, name)) return false;
  int k = -1;
  float bd = 8.0f;
  for (int i = 0; stops[i].code; i++) {
    float d = distPts(lat, lon, stops[i].lat, stops[i].lon);
    if (d < bd) {
      bd = d;
      k = i;
    }
  }
  if (k >= 0) {
    for (int i = 0; i < L.nn; i++)
      if (!strcasecmp(L.n[i].code, stops[k].code)) return false;
    strlcpy(L.n[L.nn].code, stops[k].code, sizeof(L.n[L.nn].code));
    strlcpy(L.n[L.nn].name, stops[k].city, sizeof(L.n[L.nn].name));
    L.n[L.nn].lat = stops[k].lat;
    L.n[L.nn].lon = stops[k].lon;
  } else {
    L.n[L.nn].code[0] = 0;
    strlcpy(L.n[L.nn].name, name, sizeof(L.n[L.nn].name));
    L.n[L.nn].lat = lat;
    L.n[L.nn].lon = lon;
  }
  L.nn++;
  return true;
}

void tubeAddRail(int li, const RailPt *p) {
  for (; p->lat < 900.0f; p++)
    if (p->name) tubeAddPt(li, p->name, p->lat, p->lon);
}

bool trainNearRail(const Train &t, const RailPt *p) {
  for (; p->lat < 900.0f; p++)
    if (distPts(t.lat, t.lon, p->lat, p->lon) < 30.0f) return true;
  return false;
}

void tubeSort(int li) {
  if (li < 0 || li >= MAX_TL) return;
  TubeLine &L = tubeLines[li];
  if (L.nn < 2) return;
  float c = cosf(homeLat * 0.0174533f);
  float dE = (L.n[L.nn - 1].lon - L.n[0].lon) * 60.0f * c;
  float dN = (L.n[L.nn - 1].lat - L.n[0].lat) * 60.0f;
  if (hypotf(dE, dN) < 1) {
    dE = 1;
    dN = 0;
  }
  float len = hypotf(dE, dN);
  dE /= len;
  dN /= len;
  for (int i = 0; i < L.nn; i++)
    for (int j = i + 1; j < L.nn; j++) {
      float pi = (L.n[i].lon - homeLon) * 60.0f * c * dE + (L.n[i].lat - homeLat) * 60.0f * dN;
      float pj = (L.n[j].lon - homeLon) * 60.0f * c * dE + (L.n[j].lat - homeLat) * 60.0f * dN;
      if (pj < pi) {
        TubeNode t = L.n[i];
        L.n[i] = L.n[j];
        L.n[j] = t;
      }
    }
}

int tubeAnchor(int li) {
  if (li < 0 || li >= MAX_TL) return 0;
  TubeLine &L = tubeLines[li];
  int best = 0;
  float bd = 1e9f;
  for (int i = 0; i < L.nn; i++) {
    if (!strcasecmp(L.n[i].code, homeStop)) return i;
    float d = distNm(L.n[i].lat, L.n[i].lon);
    if (d < bd) {
      bd = d;
      best = i;
    }
  }
  return best;
}

int tubeSnap(float ang) {
  while (ang < 0) ang += 180;
  while (ang >= 180) ang -= 180;
  int snap = ((int)((ang + 22.5f) / 45.0f)) * 45;
  if (snap >= 180) snap = 0;
  return snap;
}

int tubeRoom(int n, float sx, float sy, int cx, int cy, int xMin, int xMax, int yMin, int yMax) {
  if (n < 1) return 800;
  float lim = 800;
  if (sx > 0.05f) lim = min(lim, (float)(xMax - cx) / (sx * (float)n));
  if (sx < -0.05f) lim = min(lim, (float)(cx - xMin) / (-sx * (float)n));
  if (sy > 0.05f) lim = min(lim, (float)(yMax - cy) / (sy * (float)n));
  if (sy < -0.05f) lim = min(lim, (float)(cy - yMin) / (-sy * (float)n));
  if (lim < 40) return 40;
  return (int)lim;
}

void tubeLayout() {
  const int xMin = 44, xMax = PW - 44, yMin = 90, yMax = 444;
  int used[4] = {0, 0, 0, 0};
  for (int i = 0; i < nTubeLines; i++) {
    TubeLine &L = tubeLines[i];
    if (L.nn < 1) continue;
    float c = cosf(homeLat * 0.0174533f);
    float dE = (L.n[L.nn - 1].lon - L.n[0].lon) * 60.0f * c;
    float dN = (L.n[L.nn - 1].lat - L.n[0].lat) * 60.0f;
    float ang = atan2f(-dN, dE) * 57.2958f;
    int snap = tubeSnap(ang);
    int slot = snap / 45;
    if (slot < 0) slot = 0;
    if (slot > 3) slot = 3;
    int tries = 0;
    while (used[slot] && tries < 4) {
      slot = (slot + 1) & 3;
      snap = slot * 45;
      tries++;
    }
    L.ang = snap;
    L.off = used[slot] * 22;
    used[slot]++;
    int anc = tubeAnchor(i);
    float rad = L.ang * 0.0174533f;
    float ux = cosf(rad), uy = sinf(rad);
    float px = -uy, py = ux;
    int cx = (xMin + xMax) / 2 + (int)(px * L.off);
    int cy = (yMin + yMax) / 2 + (int)(py * L.off);
    int nNeg = anc;
    int nPos = L.nn - 1 - anc;
    int step = tubeRoom(nPos, ux, uy, cx, cy, xMin, xMax, yMin, yMax);
    int step2 = tubeRoom(nNeg, -ux, -uy, cx, cy, xMin, xMax, yMin, yMax);
    if (step2 < step) step = step2;
    int xA = cx + (int)(ux * step * (0 - anc));
    int yA = cy + (int)(uy * step * (0 - anc));
    int xB = cx + (int)(ux * step * (L.nn - 1 - anc));
    int yB = cy + (int)(uy * step * (L.nn - 1 - anc));
    int shx = (xMin + xMax) / 2 - (xA + xB) / 2;
    int shy = (yMin + yMax) / 2 - (yA + yB) / 2;
    for (int k = 0; k < L.nn; k++) {
      int slotk = k - anc;
      L.n[k].x = constrain(cx + shx + (int)(ux * step * slotk), xMin, xMax);
      L.n[k].y = constrain(cy + shy + (int)(uy * step * slotk), yMin, yMax);
    }
  }
}

int tubeCount(const char *code) {
  int n = 0;
  for (int i = 0; i < nTubeLines; i++)
    for (int k = 0; k < tubeLines[i].nn; k++)
      if (!strcasecmp(tubeLines[i].n[k].code, code)) n++;
  return n;
}

void tubeBuild() {
  nTubeLines = 0;
  memset(tubeLines, 0, sizeof(tubeLines));
  int nRail = (int)(sizeof(RAIL_ALL) / sizeof(RAIL_ALL[0]));
  int score[32];
  for (int i = 0; i < nRail; i++) {
    score[i] = -1;
    if (!railNearHome(RAIL_ALL[i])) continue;
    score[i] = 1;
    for (int t = 0; t < nTrains; t++)
      if (trainNearRail(trains[t], RAIL_ALL[i])) score[i] += 12;
  }
  while (nTubeLines < MAX_TL) {
    int best = -1, bs = 0;
    for (int i = 0; i < nRail; i++) {
      if (score[i] <= bs) continue;
      bool used = false;
      for (int r = 0; r < nTubeLines; r++)
        if (!strcmp(tubeLines[r].name, RAIL_NAME[i])) used = true;
      if (used) continue;
      bs = score[i];
      best = i;
    }
    if (best < 0) break;
    int hit = nTubeLines++;
    strlcpy(tubeLines[hit].name, RAIL_NAME[best], sizeof(tubeLines[hit].name));
    tubeLines[hit].col = TUBE_COL[hit & 7];
    if (!dayMode && (hit & 7) == 3) tubeLines[hit].col = RGB565(255, 220, 40);
    tubeAddRail(hit, RAIL_ALL[best]);
    score[best] = -1;
  }
  int keep = 0;
  for (int r = 0; r < nTubeLines; r++)
    if (tubeLines[r].nn >= 2) tubeLines[keep++] = tubeLines[r];
  nTubeLines = keep;
  if (nTubeLines) return;
  memset(&tubeLines[0], 0, sizeof(tubeLines[0]));
  strlcpy(tubeLines[0].name, "local", sizeof(tubeLines[0].name));
  tubeLines[0].col = TUBE_COL[2];
  float rad = (float)rangeNm + 25.0f;
  if (rad < 70) rad = 70;
  tubeAdd(0, homeStop);
  for (int i = 0; stops[i].code; i++)
    if (distNm(stops[i].lat, stops[i].lon) <= rad) tubeAdd(0, stops[i].code);
  tubeSort(0);
  if (tubeLines[0].nn >= 2) nTubeLines = 1;
}

void tubePlaceTrain(int ti, int &sx, int &sy, uint16_t &col) {
  sx = -1;
  sy = -1;
  col = COL_CYN;
  if (ti < 0 || ti >= nTrains) return;
  const Train &t = trains[ti];
  int bestL = -1, seg = 0;
  float best = 1e9f, frac = 0;
  float c = cosf(homeLat * 0.0174533f);
  for (int r = 0; r < nTubeLines; r++) {
    const TubeLine &L = tubeLines[r];
    if (L.nn < 1) continue;
    if (L.nn == 1) {
      float d = hypotf(t.lat - L.n[0].lat, t.lon - L.n[0].lon);
      if (d < best) {
        best = d;
        bestL = r;
        seg = 0;
        frac = 0;
      }
      continue;
    }
    for (int k = 0; k < L.nn - 1; k++) {
      float dx = (L.n[k + 1].lon - L.n[k].lon) * 60.0f * c;
      float dy = (L.n[k + 1].lat - L.n[k].lat) * 60.0f;
      float px = (t.lon - L.n[k].lon) * 60.0f * c;
      float py = (t.lat - L.n[k].lat) * 60.0f;
      float d2 = dx * dx + dy * dy;
      float u = d2 < 0.01f ? 0 : (px * dx + py * dy) / d2;
      if (u < 0) u = 0;
      if (u > 1) u = 1;
      float ex = px - u * dx, ey = py - u * dy;
      float d = ex * ex + ey * ey;
      if (d < best) {
        best = d;
        bestL = r;
        seg = k;
        frac = u;
      }
    }
  }
  if (bestL < 0) return;
  const TubeLine &L = tubeLines[bestL];
  col = L.col;
  if (L.nn == 1) {
    sx = L.n[0].x;
    sy = L.n[0].y - 18;
    return;
  }
  sx = L.n[seg].x + (int)((L.n[seg + 1].x - L.n[seg].x) * frac);
  sy = L.n[seg].y + (int)((L.n[seg + 1].y - L.n[seg].y) * frac);
}

void drawTube() {
  page = PG_LINE;
  uint16_t paper = dayMode ? RGB565(236, 228, 208) : RGB565(20, 24, 30);
  uint16_t ink = dayMode ? RGB565(20, 20, 22) : COL_WHT;
  uint16_t ring = dayMode ? RGB565(10, 10, 12) : COL_WHT;
  gfx->fillScreen(paper);
  drawBackHome();
  gfx->setTextColor(dayMode ? RGB565(180, 20, 28) : COL_HOT, paper);
  gfx->setTextSize(3);
  int known = findStop(homeStop);
  centre(known >= 0 ? stops[known].city : homeStop, CX, 12);
  gfx->setTextColor(ink, paper);
  gfx->setTextSize(2);
  centre("LINE  live", CX, 42);
  gfx->fillTriangle(760, 70, 774, 52, 788, 70, ink);
  gfx->setTextSize(1);
  centre("N", 774, 74);

  tubeBuild();
  tubeLayout();

  for (int r = 0; r < nTubeLines; r++) {
    TubeLine &L = tubeLines[r];
    if (L.nn < 2) continue;
    int dx0 = L.n[1].x - L.n[0].x, dy0 = L.n[1].y - L.n[0].y;
    float len0 = hypotf((float)dx0, (float)dy0);
    if (len0 > 1) {
      int sx = constrain(L.n[0].x - (int)(dx0 / len0 * 44), 20, PW - 20);
      int sy = constrain(L.n[0].y - (int)(dy0 / len0 * 44), 88, 444);
      octiLine(sx, sy, L.n[0].x, L.n[0].y, L.col, 16);
    }
    for (int k = 0; k < L.nn - 1; k++)
      octiLine(L.n[k].x, L.n[k].y, L.n[k + 1].x, L.n[k + 1].y, L.col, 16);
    int dx1 = L.n[L.nn - 1].x - L.n[L.nn - 2].x, dy1 = L.n[L.nn - 1].y - L.n[L.nn - 2].y;
    float len1 = hypotf((float)dx1, (float)dy1);
    if (len1 > 1) {
      int ex = constrain(L.n[L.nn - 1].x + (int)(dx1 / len1 * 44), 20, PW - 20);
      int ey = constrain(L.n[L.nn - 1].y + (int)(dy1 / len1 * 44), 88, 444);
      octiLine(L.n[L.nn - 1].x, L.n[L.nn - 1].y, ex, ey, L.col, 16);
    }
  }

  nTagBox = 0;
  tagAdd(200, 0, 400, 80);
  tagAdd(8, 448, PW - 16, 32);
  for (int r = 0; r < nTubeLines; r++) {
    for (int k = 0; k < tubeLines[r].nn; k++) {
      TubeNode &nd = tubeLines[r].n[k];
      bool named = false;
      for (int pr = 0; pr < r && !named; pr++)
        for (int pk = 0; pk < tubeLines[pr].nn; pk++)
          if (!strcasecmp(tubeLines[pr].n[pk].code, nd.code) ||
              !strcasecmp(tubeLines[pr].n[pk].name, nd.name))
            named = true;
      if (named) continue;
      bool home = !strcasecmp(nd.code, homeStop);
      bool ixn = home || tubeCount(nd.code) > 1;
      int sz = (home || ixn) ? 2 : 1;
      gfx->setTextSize(sz);
      gfx->setTextColor(ink, paper);
      int16_t x1, y1;
      uint16_t w, h;
      gfx->getTextBounds(nd.name, 0, 0, &x1, &y1, &w, &h);
      int bx, by;
      if (!tagSlot(nd.x, nd.y, (int)w, (int)h, 4, PW - 4, 82, 444, bx, by)) {
        if (!nd.code[0]) continue;
        gfx->setTextSize(1);
        gfx->getTextBounds(nd.code, 0, 0, &x1, &y1, &w, &h);
        if (!tagSlot(nd.x, nd.y, (int)w, (int)h, 4, PW - 4, 82, 444, bx, by)) continue;
        tagAdd(bx, by, (int)w, (int)h);
        gfx->setCursor(bx, by);
        gfx->print(nd.code);
        continue;
      }
      tagAdd(bx, by, (int)w, (int)h);
      gfx->setCursor(bx, by);
      gfx->print(nd.name);
    }
  }
  for (int r = 0; r < nTubeLines; r++) {
    for (int k = 0; k < tubeLines[r].nn; k++) {
      TubeNode &nd = tubeLines[r].n[k];
      bool home = !strcasecmp(nd.code, homeStop);
      bool ix = home || tubeCount(nd.code) > 1;
      int rad = home ? 16 : (ix ? 12 : 9);
      gfx->fillCircle(nd.x, nd.y, rad, COL_WHT);
      gfx->drawCircle(nd.x, nd.y, rad, ring);
      if (home) gfx->drawCircle(nd.x, nd.y, rad - 5, ring);
    }
  }

  for (int i = 0; i < nTrains; i++) {
    tubeX[i] = -999;
    tubeY[i] = -999;
    tubeBX[i] = tubeBY[i] = 0;
    tubeBW[i] = tubeBH[i] = 0;
    int sx, sy;
    uint16_t col;
    tubePlaceTrain(i, sx, sy, col);
    if (sx < 0) continue;
    sx = constrain(sx, 40, PW - 40);
    sy = constrain(sy, 96, 430);
    tubeX[i] = sx;
    tubeY[i] = sy;
    char lab[20];
    const char *dir = trains[i].head[0] ? trains[i].head : "";
    if (dir[0])
      snprintf(lab, sizeof(lab), "%s %s", trains[i].num, dir);
    else
      strlcpy(lab, trains[i].num, sizeof(lab));
    gfx->setTextSize(2);
    int16_t x1, y1;
    uint16_t tw, th;
    gfx->getTextBounds(lab, 0, 0, &x1, &y1, &tw, &th);
    int bw = (int)tw + 14, bh = (int)th + 6;
    int bx, by;
    if (!tagSlot(sx, sy, bw, bh, 4, PW - 4, 82, 444, bx, by)) {
      bx = constrain(sx + 12, 4, PW - 4 - bw);
      by = constrain(sy - bh - 8, 82, 444 - bh);
    }
    tagAdd(bx, by, bw, bh);
    tubeBX[i] = bx;
    tubeBY[i] = by;
    tubeBW[i] = bw;
    tubeBH[i] = bh;
    gfx->fillRoundRect(bx, by, bw, bh, 4, COL_BLK);
    gfx->fillRect(bx, by, 6, bh, col);
    gfx->setTextColor(COL_YEL, COL_BLK);
    gfx->setCursor(bx + 10, by + 3);
    gfx->print(lab);
  }

  int lx = 12;
  for (int r = 0; r < nTubeLines && r < 4; r++) {
    int w = 12 + (int)strlen(tubeLines[r].name) * 12;
    if (w > 220) w = 220;
    if (lx + w > PW - 8) break;
    gfx->fillRoundRect(lx, 450, w, 28, 4, tubeLines[r].col);
    gfx->setTextColor(COL_BLK, tubeLines[r].col);
    gfx->setTextSize(2);
    gfx->setCursor(lx + 6, 456);
    gfx->print(tubeLines[r].name);
    lx += w + 8;
  }
  if (!nTubeLines) {
    gfx->setTextColor(ink, paper);
    gfx->setTextSize(2);
    centre("no corridor yet", CX, 220);
  }
}

void drawSetTabs() {
  const char *lab[] = {"DISPLAY", "LOCATION", "ABOUT"};
  for (int i = 0; i < 3; i++)
    chip(20 + i * 256, 52, 248, 36, lab[i], (page == PG_ABOUT && i == 2) || (page == PG_SETTINGS && setPane == i));
}

void drawOtaStatus() {
  if (page != PG_ABOUT) return;
  gfx->fillRect(EDGE, 392, PW - EDGE, 80, COL_BG);
  gfx->setTextColor(COL_MID, COL_BG);
  gfx->setTextSize(2);
  centre(otaMsg[0] ? otaMsg : "SETUP > ABOUT  then CHECK", CX, 400);
  if (otaPct >= 0) {
    int w = (720 * constrain(otaPct, 0, 100)) / 100;
    gfx->drawRect(40, 436, 720, 22, COL_DIM);
    if (w > 0) gfx->fillRect(40, 436, w, 22, COL_HOT);
  }
}

void drawAbout() {
  page = PG_ABOUT;
  gfx->fillScreen(COL_BG);
  drawBackHome();
  drawSetTabs();
  gfx->setTextColor(COL_HOT, COL_BG);
  gfx->setTextSize(3);
  centre("LIVE TRAINS", CX, 112);
  gfx->setTextColor(COL_TXT, COL_BG);
  gfx->setTextSize(2);
  char ver[36];
  snprintf(ver, sizeof(ver), "version  %s", LT_VER);
  centre(ver, CX, 156);
  if (otaLatest[0]) {
    snprintf(ver, sizeof(ver), "github  %s", otaLatest);
    centre(ver, CX, 188);
  } else
    centre("github  tap CHECK", CX, 188);
  gfx->setTextColor(COL_MID, COL_BG);
  centre("Amtrak VIA Brightline LIRR MNR", CX, 208);
  centre("SEPTA MARC VRE Tri-Rail", CX, 230);
  chip(40, 252, 350, 48, "CHECK", false);
  chip(410, 252, 350, 48, "INSTALL", otaNewer);
  chip(200, 316, 400, 48, "FROM SD", false);
  drawOtaStatus();
}

void drawSettings() {
  page = PG_SETTINGS;
  gfx->fillScreen(COL_BG);
  drawBackHome();
  drawSetTabs();
  char line[48];
  if (setPane == 0) {
    gfx->setTextColor(COL_MID, COL_BG);
    gfx->setTextSize(2);
    gfx->setCursor(24, 118);
    gfx->print("RANGE");
    chip(200, 108, 80, 40, "-", false);
    snprintf(line, sizeof(line), "%d NM", rangeNm);
    gfx->setTextColor(COL_YEL, COL_BG);
    gfx->setTextSize(3);
    centre(line, CX, 116);
    chip(520, 108, 80, 40, "+", false);
    gfx->setTextColor(COL_MID, COL_BG);
    gfx->setTextSize(2);
    gfx->setCursor(24, 168);
    gfx->print("MAP");
    chip(24, 196, 240, 40, "STREET", mapOn && mapStyle == 0);
    chip(280, 196, 240, 40, "SAT", mapOn && mapStyle == 1);
    chip(536, 196, 240, 40, "OFF", !mapOn);
    gfx->setCursor(24, 252);
    gfx->print("MODE");
    chip(200, 244, 180, 36, "DARK", !dayMode);
    chip(400, 244, 180, 36, "DAY", dayMode);
    gfx->setCursor(24, 300);
    gfx->print("LIGHT");
    chip(200, 292, 150, 36, "LOW", blLevel == 0);
    chip(364, 292, 150, 36, "MED", blLevel == 1);
    chip(528, 292, 150, 36, "BRIGHT", blLevel == 2);
    chip(24, 380, 200, 48, "WIFI", false);
  } else {
    gfx->setTextColor(COL_MID, COL_BG);
    gfx->setTextSize(2);
    centre("HOME STATION", CX, 108);
    for (int i = 0; i < 3; i++) {
      char ch[2] = {homeStop[i] ? homeStop[i] : '-', 0};
      chip(140 + i * 180, 140, 160, 52, ch, false);
    }
    int known = findStop(homeStop);
    chip(24, 208, 100, 44, "<", false);
    gfx->setTextColor(COL_TXT, COL_BG);
    gfx->setTextSize(3);
    centre(known >= 0 ? stops[known].city : "custom", CX, 216);
    chip(676, 208, 100, 44, ">", false);
    snprintf(line, sizeof(line), "LAT  %.4f", homeLat);
    gfx->setTextColor(COL_TXT, COL_BG);
    gfx->setTextSize(2);
    gfx->setCursor(200, 276);
    gfx->print(line);
    chip(24, 268, 160, 44, "TYPE LAT", false);
    snprintf(line, sizeof(line), "LON  %.4f", homeLon);
    gfx->setCursor(200, 336);
    gfx->print(line);
    chip(24, 328, 160, 44, "TYPE LON", false);
    gfx->setTextColor(COL_MID, COL_BG);
    centre("trains / depot / map use this", CX, 388);
    chip(220, 416, 360, 48, "APPLY STOP", true);
  }
}

void drawWifiField(int focus, const char *raw, const char *empty, int y) {
  bool on = wifiFocus == focus;
  gfx->fillRoundRect(120, y, 664, 40, 6, on ? COL_HOT : COL_DIM);
  gfx->setTextColor(on ? COL_BG : COL_TXT, on ? COL_HOT : COL_DIM);
  gfx->setTextSize(2);
  gfx->setCursor(132, y + 10);
  gfx->print(raw[0] ? raw : empty);
}

void drawKey(int x, int y, int w, int h, const char *t) {
  gfx->fillRoundRect(x, y, w, h, 4, COL_DIM);
  gfx->setTextColor(COL_TXT, COL_DIM);
  gfx->setTextSize(2);
  centre(t, x + w / 2, y + h / 2 - 8);
}

void drawRow(const char *s, int y) {
  int n = (int)strlen(s);
  for (int i = 0; i < n; i++) {
    char t[2] = {s[i], 0};
    drawKey(i * KW + 2, y, KW - 6, 40, t);
  }
}

void startWifiScan() {
  nWifiScan = 0;
  wifiScanTop = 0;
  wifiScanBusy = true;
  strlcpy(wifiMsg, "scanning", sizeof(wifiMsg));
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  WiFi.scanDelete();
  WiFi.scanNetworks(true, false);
}

void drawWifi() {
  if (mtaKeyEdit) {
    drawMtaKey();
    return;
  }
  page = PG_WIFI;
  gfx->fillScreen(COL_BG);
  chip(8, 8, 140, 36, "< BACK", false);
  gfx->setTextColor(COL_HOT, COL_BG);
  gfx->setTextSize(2);
  centre("JOIN WIFI", CX, 16);
  chip(660, 8, 132, 36, "SCAN", wifiScanBusy);
  chip(8, 52, 56, 36, "<", false);
  chip(736, 52, 56, 36, ">", false);
  for (int i = 0; i < 2; i++) {
    int idx = wifiScanTop + i;
    if (idx >= nWifiScan) break;
    char shown[18];
    strlcpy(shown, wifiScan[idx], sizeof(shown));
    chip(72 + i * 328, 52, 320, 36, shown, !strcmp(wifiScan[idx], wifiSsid));
  }
  if (wifiMsg[0] && nWifiScan == 0) {
    gfx->setTextColor(COL_YEL, COL_BG);
    gfx->setTextSize(2);
    centre(wifiMsg, CX, 56);
  }
  gfx->setTextColor(COL_MID, COL_BG);
  gfx->setTextSize(2);
  gfx->setCursor(16, 110);
  gfx->print("SSID");
  drawWifiField(0, wifiSsid, "tap net or type", 100);
  gfx->setTextColor(COL_MID, COL_BG);
  gfx->setCursor(16, 158);
  gfx->print("PASS");
  drawWifiField(1, wifiPass, "type password", 148);
  const char *r0 = "1234567890";
  const char *r1 = wifiSym ? "!@#$%^&*()" : (wifiShift ? "QWERTYUIOP" : "qwertyuiop");
  const char *r2 = wifiSym ? "-_=+[]{};'" : (wifiShift ? "ASDFGHJKL" : "asdfghjkl");
  const char *r3 = wifiSym ? ",.<>/?\\|~`" : (wifiShift ? "ZXCVBNM.@-" : "zxcvbnm.@-");
  drawRow(r0, 200);
  drawRow(r1, 248);
  drawRow(r2, 296);
  drawRow(r3, 344);
  drawKey(722, 296, 70, 40, "DEL");
  chip(8, 392, 120, 36, wifiShift ? "ABC" : "abc", wifiShift);
  chip(140, 392, 400, 36, "SPACE", false);
  chip(552, 392, 120, 36, wifiSym ? "ABC" : "!#", wifiSym);
  chip(8, 436, 252, 36, "SAVE", true);
  chip(274, 436, 252, 36, "RESET WIFI", false);
  chip(540, 436, 252, 36, "FACTORY", false);
}

void drawMtaKey() {
  page = PG_WIFI;
  gfx->fillScreen(COL_BG);
  chip(8, 8, 140, 36, "< BACK", false);
  gfx->setTextColor(COL_HOT, COL_BG);
  gfx->setTextSize(2);
  centre("MTA KEY", CX, 16);
  gfx->setTextColor(COL_MID, COL_BG);
  centre("free key from api.mta.info", CX, 64);
  wifiFocus = 0;
  drawWifiField(0, mtaEdit, "type the key", 100);
  const char *r0 = "1234567890";
  const char *r1 = wifiSym ? "!@#$%^&*()" : (wifiShift ? "QWERTYUIOP" : "qwertyuiop");
  const char *r2 = wifiSym ? "-_=+[]{};'" : (wifiShift ? "ASDFGHJKL" : "asdfghjkl");
  const char *r3 = wifiSym ? ",.<>/?\\|~`" : (wifiShift ? "ZXCVBNM.@-" : "zxcvbnm.@-");
  drawRow(r0, 200);
  drawRow(r1, 248);
  drawRow(r2, 296);
  drawRow(r3, 344);
  drawKey(722, 296, 70, 40, "DEL");
  chip(8, 392, 120, 36, wifiShift ? "ABC" : "abc", wifiShift);
  chip(140, 392, 400, 36, "SPACE", false);
  chip(552, 392, 120, 36, wifiSym ? "ABC" : "!#", wifiSym);
  chip(8, 436, 360, 36, "SAVE", true);
  chip(400, 436, 392, 36, "CLEAR", false);
}

bool mtaHitRow(int x, int y, const char *s, int y0) {
  if (y < y0 || y >= y0 + 44) return false;
  int i = x / KW;
  int nlen = (int)strlen(s);
  if (i < 0 || i >= nlen) return false;
  int n = (int)strlen(mtaEdit);
  if (n < 48) {
    mtaEdit[n] = s[i];
    mtaEdit[n + 1] = 0;
  }
  if (wifiShift && s[i] >= 'A' && s[i] <= 'Z') wifiShift = false;
  return true;
}

void tapMtaKey(int x, int y) {
  if (hit(x, y, 8, 436, 368, 472)) {
    trimWord(mtaEdit);
    strlcpy(mtaKey, mtaEdit, sizeof(mtaKey));
    mtaKeyEdit = false;
    savePrefs();
    drawSettings();
    if (wifiLinked()) askNet(NET_TRAINS);
    return;
  }
  if (hit(x, y, 400, 436, 792, 472)) {
    mtaEdit[0] = 0;
    drawMtaKey();
    return;
  }
  if (hit(x, y, 8, 392, 128, 428)) {
    wifiShift = !wifiShift;
    wifiSym = false;
    drawMtaKey();
    return;
  }
  if (hit(x, y, 140, 392, 540, 428)) {
    int n = (int)strlen(mtaEdit);
    if (n < 48) {
      mtaEdit[n] = ' ';
      mtaEdit[n + 1] = 0;
    }
    drawMtaKey();
    return;
  }
  if (hit(x, y, 552, 392, 672, 428)) {
    wifiSym = !wifiSym;
    drawMtaKey();
    return;
  }
  if (hit(x, y, 722, 296, 792, 336)) {
    int n = (int)strlen(mtaEdit);
    if (n) mtaEdit[n - 1] = 0;
    drawMtaKey();
    return;
  }
  const char *r0 = "1234567890";
  const char *r1 = wifiSym ? "!@#$%^&*()" : (wifiShift ? "QWERTYUIOP" : "qwertyuiop");
  const char *r2 = wifiSym ? "-_=+[]{};'" : (wifiShift ? "ASDFGHJKL" : "asdfghjkl");
  const char *r3 = wifiSym ? ",.<>/?\\|~`" : (wifiShift ? "ZXCVBNM.@-" : "zxcvbnm.@-");
  if (mtaHitRow(x, y, r0, 200) || mtaHitRow(x, y, r1, 248) || mtaHitRow(x, y, r2, 296) || mtaHitRow(x, y, r3, 344))
    drawMtaKey();
}

void openWifi() {
  mtaKeyEdit = false;
  wifiFocus = wifiSsid[0] ? 1 : 0;
  wifiMsg[0] = 0;
  startWifiScan();
  drawWifi();
}

void wifiAppend(char c) {
  char *d = wifiFocus ? wifiPass : wifiSsid;
  int n = (int)strlen(d);
  if (n < 62) {
    d[n] = c;
    d[n + 1] = 0;
  }
}

void wifiDel() {
  char *d = wifiFocus ? wifiPass : wifiSsid;
  int n = (int)strlen(d);
  if (n) d[n - 1] = 0;
}

bool hitRow(int x, int y, const char *s, int y0) {
  if (y < y0 || y >= y0 + 44) return false;
  int i = x / KW;
  int n = (int)strlen(s);
  if (i < 0 || i >= n) return false;
  wifiAppend(s[i]);
  if (wifiShift && s[i] >= 'A' && s[i] <= 'Z') wifiShift = false;
  return true;
}

void pollWifiScan() {
  if (!wifiScanBusy) return;
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) return;
  wifiScanBusy = false;
  if (n < 0) n = 0;
  nWifiScan = 0;
  for (int i = 0; i < n && nWifiScan < 16; i++) {
    String s = WiFi.SSID(i);
    if (s.length() == 0) continue;
    strlcpy(wifiScan[nWifiScan], s.c_str(), 33);
    nWifiScan++;
  }
  WiFi.scanDelete();
  wifiScanTop = 0;
  if (nWifiScan)
    wifiMsg[0] = 0;
  else
    strlcpy(wifiMsg, "none found  type name", sizeof(wifiMsg));
  if (page == PG_WIFI) drawWifi();
}

void applyWifiSave() {
  if (!wifiSsid[0]) {
    strlcpy(wifiMsg, "need wifi name", sizeof(wifiMsg));
    drawWifi();
    return;
  }
  savePrefs();
  gfx->fillScreen(COL_BG);
  gfx->setTextColor(COL_HOT, COL_BG);
  gfx->setTextSize(2);
  centre("saved", CX, 160);
  gfx->setTextColor(COL_TXT, COL_BG);
  centre(wifiSsid, CX, 200);
  centre("linking", CX, 240);
  delay(600);
  ESP.restart();
}

void pushHist() {
  if (nHist && hist[nHist - 1] == page) return;
  if (nHist >= 8) {
    for (int i = 0; i < 7; i++) hist[i] = hist[i + 1];
    nHist = 7;
  }
  hist[nHist++] = page;
}

void paintPage(int pg) {
  if (pg == PG_HOME) drawHome();
  else if (pg == PG_LIST) drawList();
  else if (pg == PG_DETAIL) drawDetail();
  else if (pg == PG_DEPOT) drawDepot();
  else if (pg == PG_CLOCK) drawClock();
  else if (pg == PG_LINE) drawTube();
  else if (pg == PG_SETTINGS) drawSettings();
  else if (pg == PG_ABOUT) drawAbout();
  else if (pg == PG_WIFI) drawWifi();
}

void openPage(int pg) {
  pushHist();
  page = pg;
  paintPage(pg);
}

void resetView() {
  viewLat = homeLat;
  viewLon = homeLon;
  mapOx = mapOy = 0;
}

void applyHoldPan(int dx, int dy, float holdLa, float holdLo) {
  float nm = (mapOk && mapNm > 0) ? (float)mapNm : (float)rangeNm;
  float c = cosf(holdLa * 0.0174533f);
  if (c < 0.3f) c = 0.3f;
  float dLon = (nm * 0.5f) / (60.0f * c);
  float dLat = dLon / ((float)SRC_W / (float)SRC_H);
  viewLon = holdLo - ((float)dx / (float)PW) * 2.0f * dLon;
  viewLat = holdLa + ((float)dy / (float)MAP_H) * 2.0f * dLat;
  if (viewLat > 49.5f) viewLat = 49.5f;
  if (viewLat < 24.0f) viewLat = 24.0f;
  if (viewLon < -125.5f) viewLon = -125.5f;
  if (viewLon > -66.5f) viewLon = -66.5f;
  mapOx = dx;
  mapOy = dy;
}

void goHome() {
  nHist = 0;
  resetView();
  drawHome();
}

void goBack() {
  if (nHist) {
    int p = hist[--nHist];
    page = p;
    paintPage(p);
  } else
    goHome();
}

void refreshGeo() {
  resetView();
  mapOk = false;
  lastMap = lastMapTry = 0;
  lastFetch = millis() ? millis() : 1;
  if (mapOn) {
    askNet(NET_MAP);
    if (!netPend) netPend = NET_TRAINS;
  } else
    askNet(NET_TRAINS);
}

void bumpZoom(int dir) {
  int step = rangeNm <= 30 ? 5 : (rangeNm <= 80 ? 10 : 20);
  int n = constrain(rangeNm + dir * step, 10, 300);
  if (n == rangeNm) return;
  rangeNm = n;
  savePrefs();
  zoomNeed = millis() ? millis() : 1;
  if (page == PG_HOME) {
    if (mapOn && mapOk) mapOk = false;
    drawHomeLayer();
    drawZoomBar();
  }
}

void bumpStopLetter(int i) {
  if (i < 0 || i > 2) return;
  char c = homeStop[i];
  if (c < 'A' || c > 'Z') c = 'A';
  else if (++c > 'Z') c = 'A';
  homeStop[i] = c;
  homeStop[3] = 0;
  int hit = findStop(homeStop);
  if (hit >= 0) useStop(hit);
}

int trainAt(int x, int y) {
  int best = -1, bd = 400;
  for (int i = 0; i < nTrains; i++) {
    float tx, ty;
    llToXY(trains[i].lat, trains[i].lon, tx, ty);
    int d = abs((int)tx - x) + abs((int)ty - y);
    if (d < 28 && d < bd) {
      bd = d;
      best = i;
    }
  }
  return best;
}

void tapSetTabs(int x, int y) {
  if (y < 52 || y > 88) return;
  if (x < 268) {
    page = PG_SETTINGS;
    setPane = 0;
    drawSettings();
  } else if (x < 524) {
    page = PG_SETTINGS;
    setPane = 1;
    drawSettings();
  } else
    drawAbout();
}

void tapAbout(int x, int y) {
  if (y >= 52 && y <= 88) {
    tapSetTabs(x, y);
    return;
  }
  if (otaBusy) return;
  if (hit(x, y, 40, 252, 390, 300)) {
    askOta(1);
    drawAbout();
  } else if (hit(x, y, 410, 252, 760, 300)) {
    if (!otaNewer) otaNote("CHECK first  or use SD");
    else askOta(2);
    drawAbout();
  } else if (hit(x, y, 200, 316, 600, 364)) {
    askOta(3);
    drawAbout();
  }
}

void tapSettings(int x, int y) {
  if (y >= 52 && y <= 88) {
    tapSetTabs(x, y);
    return;
  }
  if (setPane == 0) {
    if (hit(x, y, 200, 108, 280, 148)) {
      rangeNm = max(15, rangeNm - 10);
      savePrefs();
    } else if (hit(x, y, 520, 108, 600, 148)) {
      rangeNm = min(300, rangeNm + 10);
      savePrefs();
    } else if (hit(x, y, 24, 196, 264, 236)) {
      mapOn = true;
      mapStyle = 0;
      savePrefs();
      mapOk = false;
      askNet(NET_MAP);
    } else if (hit(x, y, 280, 196, 520, 236)) {
      mapOn = true;
      mapStyle = 1;
      savePrefs();
      mapOk = false;
      askNet(NET_MAP);
    } else if (hit(x, y, 536, 196, 776, 236)) {
      mapOn = false;
      savePrefs();
    } else if (hit(x, y, 200, 244, 380, 280)) {
      dayMode = false;
      cols();
      savePrefs();
      if (mapOn) {
        mapOk = false;
        askNet(NET_MAP);
      }
    } else if (hit(x, y, 400, 244, 580, 280)) {
      dayMode = true;
      cols();
      savePrefs();
      if (mapOn) {
        mapOk = false;
        askNet(NET_MAP);
      }
    } else if (hit(x, y, 200, 292, 350, 328)) {
      blLevel = 0;
      applyBl();
      savePrefs();
    } else if (hit(x, y, 364, 292, 514, 328)) {
      blLevel = 1;
      applyBl();
      savePrefs();
    } else if (hit(x, y, 528, 292, 678, 328)) {
      blLevel = 2;
      applyBl();
      savePrefs();
    } else if (hit(x, y, 24, 380, 224, 428)) {
      pushHist();
      openWifi();
      return;
    } else
      return;
    drawSettings();
  } else {
    for (int i = 0; i < 3; i++) {
      if (hit(x, y, 140 + i * 180, 140, 300 + i * 180, 192)) {
        bumpStopLetter(i);
        savePrefs();
        drawSettings();
        return;
      }
    }
    int n = 0;
    while (stops[n].code) n++;
    if (hit(x, y, 24, 208, 124, 252)) {
      useStop((fieldPick + n - 1) % n);
      savePrefs();
      drawSettings();
    } else if (hit(x, y, 676, 208, 776, 252)) {
      useStop((fieldPick + 1) % n);
      savePrefs();
      drawSettings();
    } else if (hit(x, y, 220, 416, 580, 464)) {
      savePrefs();
      gfx->fillScreen(COL_BG);
      gfx->setTextColor(COL_HOT, COL_BG);
      gfx->setTextSize(2);
      centre("saved", CX, 180);
      gfx->setTextColor(COL_TXT, COL_BG);
      centre(homeStop, CX, 220);
      delay(600);
      ESP.restart();
    }
  }
}

int gt911Touches(int *xs, int *ys, int maxN) {
  Wire.beginTransmission(GT911_ADDR);
  Wire.write(0x81);
  Wire.write(0x4E);
  if (Wire.endTransmission(false) != 0) return 0;
  if (Wire.requestFrom((uint8_t)GT911_ADDR, (uint8_t)1) != 1) return 0;
  uint8_t st = Wire.read();
  if ((st & 0x80) == 0) return 0;
  int n = st & 0x0F;
  if (n == 0) {
    Wire.beginTransmission(GT911_ADDR);
    Wire.write(0x81);
    Wire.write(0x4E);
    Wire.write(0);
    Wire.endTransmission();
    return 0;
  }
  if (n > maxN) n = maxN;
  if (n > 2) n = 2;
  Wire.beginTransmission(GT911_ADDR);
  Wire.write(0x81);
  Wire.write(0x50);
  if (Wire.endTransmission(false) != 0) return 0;
  uint8_t want = (uint8_t)(n * 8);
  if (Wire.requestFrom((uint8_t)GT911_ADDR, want) != want) {
    Wire.beginTransmission(GT911_ADDR);
    Wire.write(0x81);
    Wire.write(0x4E);
    Wire.write(0);
    Wire.endTransmission();
    return 0;
  }
  uint8_t p[16];
  for (int i = 0; i < want; i++) p[i] = Wire.read();
  Wire.beginTransmission(GT911_ADDR);
  Wire.write(0x81);
  Wire.write(0x4E);
  Wire.write(0);
  Wire.endTransmission();
  for (int i = 0; i < n; i++) {
    xs[i] = constrain(p[i * 8] | (p[i * 8 + 1] << 8), 0, PW - 1);
    ys[i] = constrain(p[i * 8 + 2] | (p[i * 8 + 3] << 8), 0, PH - 1);
  }
  return n;
}

void onTap(int x, int y) {
  if (otaBusy) return;
  if (page != PG_HOME && page != PG_WIFI && hit(x, y, 652, 8, 792, 44)) {
    goHome();
    return;
  }
  if (page != PG_HOME && hit(x, y, 8, 8, 148, 44)) {
    if (page == PG_WIFI) {
      if (mtaKeyEdit) {
        mtaKeyEdit = false;
        drawSettings();
        return;
      }
      goHome();
      return;
    }
    goBack();
    return;
  }
  if (page == PG_SETTINGS) {
    tapSettings(x, y);
    return;
  }
  if (page == PG_ABOUT) {
    tapAbout(x, y);
    return;
  }
  if (page == PG_HOME) {
    if (y >= ZOOM_Y && y < MENU_Y) {
      if (x < 176) bumpZoom(1);
      else if (x > PW - 176) bumpZoom(-1);
      return;
    }
    if (y >= MENU_Y) {
      int b = x / (PW / 5);
      if (b == 0) openPage(PG_LINE);
      else if (b == 1) openPage(PG_LIST);
      else if (b == 2) openPage(PG_DEPOT);
      else if (b == 3) openPage(PG_CLOCK);
      else {
        setPane = 1;
        openPage(PG_SETTINGS);
      }
      return;
    }
    if (y < 42 && x < 320) {
      if (fabsf(viewLat - homeLat) > 0.002f || fabsf(viewLon - homeLon) > 0.002f) {
        resetView();
        refreshGeo();
        drawHome();
        return;
      }
    }
    int i = trainAt(x, y);
    if (i >= 0) {
      selected = i;
      openPage(PG_DETAIL);
    }
    return;
  }
  if (page == PG_LINE) {
    int best = -1, bd = 9999;
    for (int i = 0; i < nTrains; i++) {
      if (tubeX[i] < 0) continue;
      bool onChip = tubeBW[i] > 0 && hit(x, y, tubeBX[i] - 12, tubeBY[i] - 12, tubeBX[i] + tubeBW[i] + 12,
                                         tubeBY[i] + tubeBH[i] + 12);
      int d = abs(tubeX[i] - x) + abs(tubeY[i] - y);
      if (onChip) d = 0;
      if (d < 70 && d < bd) {
        bd = d;
        best = i;
      }
    }
    if (best >= 0) {
      selected = best;
      openPage(PG_DETAIL);
    }
    return;
  }
  if (page == PG_LIST) {
    if (y >= 430 && nTrains > 8) {
      listTop += 8;
      if (listTop >= nTrains) listTop = 0;
      drawList();
      return;
    }
    if (y > 56 && y < 430 && nTrains) {
      int row = (y - 64) / 48;
      int idx = listTop + row;
      if (idx >= 0 && idx < nTrains) {
        selected = idx;
        openPage(PG_DETAIL);
      }
    }
    return;
  }
  if (page == PG_DEPOT) {
    if (y >= 164 && nTrains) {
      int row = (y - 164) / 56;
      if (row >= 0 && row < nTrains && row < 5) {
        selected = row;
        openPage(PG_DETAIL);
      }
    }
    return;
  }
  if (page != PG_WIFI) return;
  if (mtaKeyEdit) {
    tapMtaKey(x, y);
    return;
  }
  if (hit(x, y, 660, 8, 792, 44)) {
    startWifiScan();
    drawWifi();
    return;
  }
  if (hit(x, y, 8, 52, 64, 88) && wifiScanTop > 0) {
    wifiScanTop = max(0, wifiScanTop - 2);
    drawWifi();
    return;
  }
  if (hit(x, y, 736, 52, 792, 88) && wifiScanTop + 2 < nWifiScan) {
    wifiScanTop += 2;
    drawWifi();
    return;
  }
  for (int i = 0; i < 2; i++) {
    int idx = wifiScanTop + i;
    if (idx >= nWifiScan) break;
    if (hit(x, y, 72 + i * 328, 52, 392 + i * 328, 88)) {
      strlcpy(wifiSsid, wifiScan[idx], sizeof(wifiSsid));
      wifiFocus = 1;
      drawWifi();
      return;
    }
  }
  if (hit(x, y, 120, 100, 784, 140)) {
    wifiFocus = 0;
    drawWifi();
    return;
  }
  if (hit(x, y, 120, 148, 784, 188)) {
    wifiFocus = 1;
    drawWifi();
    return;
  }
  if (hit(x, y, 8, 436, 260, 472)) {
    applyWifiSave();
    return;
  }
  if (hit(x, y, 274, 436, 526, 472)) {
    clearWifi();
    strlcpy(wifiMsg, "wifi cleared", sizeof(wifiMsg));
    drawWifi();
    return;
  }
  if (hit(x, y, 540, 436, 792, 472)) {
    factoryReset();
    delay(200);
    ESP.restart();
    return;
  }
  if (hit(x, y, 8, 392, 128, 428)) {
    wifiShift = !wifiShift;
    wifiSym = false;
    drawWifi();
    return;
  }
  if (hit(x, y, 140, 392, 540, 428)) {
    wifiAppend(' ');
    drawWifi();
    return;
  }
  if (hit(x, y, 552, 392, 672, 428)) {
    wifiSym = !wifiSym;
    drawWifi();
    return;
  }
  if (hit(x, y, 722, 296, 792, 336)) {
    wifiDel();
    drawWifi();
    return;
  }
  const char *r0 = "1234567890";
  const char *r1 = wifiSym ? "!@#$%^&*()" : (wifiShift ? "QWERTYUIOP" : "qwertyuiop");
  const char *r2 = wifiSym ? "-_=+[]{};'" : (wifiShift ? "ASDFGHJKL" : "asdfghjkl");
  const char *r3 = wifiSym ? ",.<>/?\\|~`" : (wifiShift ? "ZXCVBNM.@-" : "zxcvbnm.@-");
  if (hitRow(x, y, r0, 200) || hitRow(x, y, r1, 248) || hitRow(x, y, r2, 296) || hitRow(x, y, r3, 344))
    drawWifi();
}

void takeNetResult() {
  if (!netDone) return;
  netDone = false;
  int j = netDoneJob;
  if (j == NET_TRAINS) {
    if (page == PG_HOME) refreshHomeTrains();
    else if (page == PG_LIST) drawList();
    else if (page == PG_DEPOT) drawDepot();
    else if (page == PG_CLOCK) drawClock();
    else if (page == PG_LINE) drawTube();
    else if (page == PG_DETAIL) drawDetail();
  } else if (j == NET_MAP && page == PG_HOME)
    drawHomeLayer();
  else if (j == NET_OTA && page == PG_ABOUT)
    drawAbout();
  if (netPend && !netBusy && !netJob && !(onQuietPage() && dlBurst <= 0)) {
    int pend = netPend;
    netPend = NET_NONE;
    askNet(pend);
  }
}

void connectWifi() {
  loadPrefs();
  if (wifiSsid[0] == 0) {
    openWifi();
    return;
  }
  gfx->fillScreen(COL_BG);
  statusLine("linking");
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsid, wifiPass);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) delay(50);
  wifiOk = wifiLinked();
  if (wifiOk) {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    applyTz();
  }
  drawHome();
  if (wifiOk) {
    askNet(NET_MAP);
    if (!netPend) netPend = NET_TRAINS;
  }
}

void setup() {
  cols();
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
  Serial.begin(115200);
  pinMode(TOUCH_RST, OUTPUT);
  digitalWrite(TOUCH_RST, LOW);
  delay(10);
  digitalWrite(TOUCH_RST, HIGH);
  delay(50);
  static uint8_t *screenGapBefore = nullptr;
  static uint8_t *screenGapAfter = nullptr;
  if (!screenGapBefore) screenGapBefore = (uint8_t *)psAlloc(1024);
  rgbBus = new Arduino_ESP32RGBPanel(
      41, 40, 39, 42,
      14, 21, 47, 48, 45,
      9, 46, 3, 8, 16, 1,
      15, 7, 6, 5, 4,
      0, 100, 30, 96,
      0, 12, 13, 10,
      1, 12000000, false, 0, 0, 800 * 30);
  gfx = new Arduino_RGB_Display(PW, PH, rgbBus, 0, true);
  if (!gfx->begin()) {
    Serial.println("gfx begin failed");
    return;
  }
  if (!screenGapAfter) screenGapAfter = (uint8_t *)psAlloc(1024);
  digitalWrite(GFX_BL, HIGH);
  gfx->setTextWrap(false);
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  initSd();
  httpLockInit();
  lastTouch = millis();
  xTaskCreatePinnedToCore(netTask, "ltnet", 32768, nullptr, 1, nullptr, 0);
  connectWifi();
  if (blLevel != 2) {
    blLevel = 2;
    savePrefs();
  }
  applyBl();
}

void loop() {
  if (otaBusy) {
    takeNetResult();
    if (page == PG_ABOUT && millis() - lastOtaDraw > 400UL) {
      lastOtaDraw = millis();
      drawOtaStatus();
    }
    delay(8);
    return;
  }
  pollWifiScan();
  int xs[2], ys[2];
  int nTouch = gt911Touches(xs, ys, 2);
  static bool mapHold = false, mapDrag = false;
  static int holdX = 0, holdY = 0;
  static float holdLa = 0, holdLo = 0;
  if (nTouch >= 2 && page == PG_HOME && ys[0] < ZOOM_Y && ys[1] < ZOOM_Y) {
    mapHold = false;
    mapDrag = false;
    float d = hypotf((float)(xs[1] - xs[0]), (float)(ys[1] - ys[0]));
    if (pinchDist < 1) pinchDist = d;
    else if (d > pinchDist + 48) {
      bumpZoom(-1);
      pinchDist = d;
    } else if (d < pinchDist - 48) {
      bumpZoom(1);
      pinchDist = d;
    }
  } else if (nTouch == 1 && page == PG_HOME && ys[0] < MAP_H) {
    pinchDist = 0;
    if (!mapHold) {
      mapHold = true;
      mapDrag = false;
      holdX = xs[0];
      holdY = ys[0];
      holdLa = viewLat;
      holdLo = viewLon;
    } else {
      int dx = xs[0] - holdX;
      int dy = ys[0] - holdY;
      if (!mapDrag && abs(dx) + abs(dy) > 16) mapDrag = true;
      if (mapDrag) {
        applyHoldPan(dx, dy, holdLa, holdLo);
        drawHomeLayer();
      }
    }
  } else {
    pinchDist = 0;
    if (mapHold) {
      if (mapDrag) {
        mapOx = mapOy = 0;
        zoomNeed = millis() ? millis() : 1;
        if (wifiLinked()) askNet(NET_TRAINS);
        drawHomeLayer();
      } else if (millis() - lastTouch > 160UL) {
        lastTouch = millis();
        onTap(holdX, holdY);
      }
      mapHold = false;
      mapDrag = false;
    } else if (nTouch == 1 && millis() - lastTouch > 160UL) {
      lastTouch = millis();
      onTap(xs[0], ys[0]);
    }
  }
  takeNetResult();
  if (onQuietPage()) {
    delay(8);
    return;
  }
  if (netPend && !netBusy && !netJob && !netDone) {
    int pend = netPend;
    netPend = NET_NONE;
    askNet(pend);
  }
  if (zoomNeed && millis() - zoomNeed > 700UL) {
    zoomNeed = 0;
    if (mapOn && !otaBusy) {
      if (netBusy || netJob) netPend = NET_MAP;
      else askNet(NET_MAP);
    }
  }
  if (wifiLinked() && page == PG_HOME && millis() - lastFetch > 30000UL && !netBusy)
    askNet(NET_TRAINS);
  if (page == PG_HOME && millis() - lastClock > 1000UL) paintHomeClock(false);
  if (page == PG_CLOCK && millis() - lastClock > 1000UL) paintClockTime(false);
  delay(8);
}
