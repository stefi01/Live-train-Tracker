/* LIRR and Metro-North names and map lines.
   Built from MTA GTFS dated 2026-10-16.
   Port Jervis and Pascack Valley are NJ Transit and are not in this file. */
#pragma once

struct MtaStopNm {
  uint16_t id;
  const char *name;
};

struct MtaRouteNm {
  const char *id;
  const char *name;
};

static const MtaStopNm LIRR_STOPS[] = {
    {1, "Albertson"},
    {2, "Auburndale"},
    {4, "Amagansett"},
    {8, "Amityville"},
    {11, "Broadway"},
    {13, "Bridgehampton"},
    {14, "Stony Brook"},
    {16, "Bellmore"},
    {20, "Bethpage"},
    {21, "Bellport"},
    {23, "Bellerose"},
    {24, "Belmont Park"},
    {25, "Bayside"},
    {26, "Bay Shore"},
    {27, "Babylon"},
    {29, "Brentwood"},
    {31, "Centre Avenue"},
    {32, "Cedarhurst"},
    {33, "Central Islip"},
    {36, "Country Life Press"},
    {38, "Copiague"},
    {39, "Carle Place"},
    {40, "Cold Spring Harbor"},
    {42, "Douglaston"},
    {44, "Deer Park"},
    {48, "East Hampton"},
    {50, "East New York"},
    {51, "East Rockaway"},
    {52, "East Williston"},
    {55, "Forest Hills"},
    {56, "Flushing Main Street"},
    {59, "Farmingdale"},
    {63, "Floral Park"},
    {64, "Freeport"},
    {65, "Far Rockaway"},
    {66, "Gibson"},
    {67, "Glen Cove"},
    {68, "Garden City"},
    {71, "Glen Head"},
    {72, "Great Neck"},
    {73, "Greenport"},
    {74, "Great River"},
    {76, "Glen Street"},
    {77, "Greenvale"},
    {78, "Greenlawn"},
    {83, "Hampton Bays"},
    {84, "Hempstead"},
    {85, "Hempstead Gardens"},
    {86, "Hillside Facility"},
    {89, "Hollis"},
    {90, "Hunterspoint Avenue"},
    {91, "Huntington"},
    {92, "Hicksville"},
    {94, "Hewlett"},
    {99, "Island Park"},
    {100, "Islip"},
    {101, "Inwood"},
    {102, "Jamaica"},
    {107, "Kew Gardens"},
    {111, "Kings Park"},
    {113, "Long Beach"},
    {114, "Lawrence"},
    {117, "Lindenhurst"},
    {118, "Long Island City"},
    {119, "Locust Manor"},
    {120, "Little Neck"},
    {122, "Laurelton"},
    {123, "Locust Valley"},
    {124, "Lakeview"},
    {125, "Lynbrook"},
    {126, "Mattituck"},
    {127, "Merillon Avenue"},
    {129, "Medford"},
    {130, "Murray Hill"},
    {131, "Manhasset"},
    {132, "Mineola"},
    {135, "Massapequa Park"},
    {136, "Massapequa"},
    {140, "Mastic-Shirley"},
    {141, "Montauk"},
    {142, "Malverne"},
    {148, "Nostrand Avenue"},
    {149, "Nassau Boulevard"},
    {152, "New Hyde Park"},
    {153, "Northport"},
    {154, "Oyster Bay"},
    {155, "Oceanside"},
    {157, "Oakdale"},
    {162, "Plandome"},
    {163, "Patchogue"},
    {164, "Port Jefferson"},
    {165, "Pinelawn"},
    {171, "Port Washington"},
    {175, "Queens Village"},
    {176, "Riverhead"},
    {179, "Ronkonkoma"},
    {180, "Rosedale"},
    {182, "Roslyn"},
    {183, "Rockville Centre"},
    {184, "St. Albans"},
    {185, "Sea Cliff"},
    {187, "Seaford"},
    {190, "Southold"},
    {191, "Southampton"},
    {193, "St. James"},
    {195, "Stewart Manor"},
    {198, "Speonk"},
    {199, "Mets-Willets Point"},
    {202, "Smithtown"},
    {204, "Sayville"},
    {205, "Syosset"},
    {211, "Valley Stream"},
    {213, "Westbury"},
    {214, "Woodside"},
    {215, "Wantagh"},
    {216, "West Hempstead"},
    {217, "Woodmere"},
    {219, "Westwood"},
    {220, "Wyandanch"},
    {223, "Yaphank-BNL"},
    {225, "Baldwin"},
    {226, "Merrick"},
    {233, "Westhampton"},
    {237, "Penn Station"},
    {241, "Atlantic Terminal"},
    {349, "Grand Central"},
    {359, "Elmont-UBS Arena"},
};
static const int LIRR_STOPS_N = 127;

static const MtaStopNm MNR_STOPS[] = {
    {1, "Grand Central"},
    {4, "Harlem-125 St"},
    {9, "Morris Heights"},
    {10, "University Heights"},
    {11, "Marble Hill"},
    {14, "Spuyten Duyvil"},
    {16, "Riverdale"},
    {17, "Ludlow"},
    {18, "Yonkers"},
    {19, "Glenwood"},
    {20, "Greystone"},
    {22, "Hastings-on-Hudson"},
    {23, "Dobbs Ferry"},
    {24, "Ardsley-on-Hudson"},
    {25, "Irvington"},
    {27, "Tarrytown"},
    {29, "Philipse Manor"},
    {30, "Scarborough"},
    {31, "Ossining"},
    {33, "Croton-Harmon"},
    {37, "Cortlandt"},
    {39, "Peekskill"},
    {40, "Manitou"},
    {42, "Garrison"},
    {43, "Cold Spring"},
    {44, "Breakneck Ridge"},
    {46, "Beacon"},
    {49, "New Hamburg"},
    {51, "Poughkeepsie"},
    {54, "Melrose"},
    {55, "Tremont"},
    {56, "Fordham"},
    {57, "Botanical Garden"},
    {58, "Williams Bridge"},
    {59, "Woodlawn"},
    {61, "Wakefield"},
    {62, "Mt Vernon West"},
    {64, "Fleetwood"},
    {65, "Bronxville"},
    {66, "Tuckahoe"},
    {68, "Crestwood"},
    {71, "Scarsdale"},
    {72, "Hartsdale"},
    {74, "White Plains"},
    {76, "North White Plains"},
    {78, "Valhalla"},
    {79, "Mt Pleasant"},
    {80, "Hawthorne"},
    {81, "Pleasantville"},
    {83, "Chappaqua"},
    {84, "Mt Kisco"},
    {85, "Bedford Hills"},
    {86, "Katonah"},
    {88, "Goldens Bridge"},
    {89, "Purdy's"},
    {90, "Croton Falls"},
    {91, "Brewster"},
    {94, "Southeast"},
    {97, "Patterson"},
    {98, "Pawling"},
    {99, "Appalachian Trail"},
    {100, "Harlem Valley-Wingdale"},
    {101, "Dover Plains"},
    {105, "Mt Vernon East"},
    {106, "Pelham"},
    {108, "New Rochelle"},
    {110, "Larchmont"},
    {111, "Mamaroneck"},
    {112, "Harrison"},
    {114, "Rye"},
    {115, "Port Chester"},
    {116, "Greenwich"},
    {118, "Cos Cob"},
    {120, "Riverside"},
    {121, "Old Greenwich"},
    {124, "Stamford"},
    {127, "Noroton Heights"},
    {128, "Darien"},
    {129, "Rowayton"},
    {131, "South Norwalk"},
    {133, "East Norwalk"},
    {134, "Westport"},
    {136, "Green's Farms"},
    {137, "Southport"},
    {138, "Fairfield"},
    {140, "Bridgeport"},
    {143, "Stratford"},
    {145, "Milford"},
    {149, "New Haven"},
    {151, "New Haven-State St"},
    {153, "Glenbrook"},
    {154, "Springdale"},
    {155, "Talmadge Hill"},
    {157, "New Canaan"},
    {158, "Merritt 7"},
    {160, "Wilton"},
    {161, "Cannondale"},
    {162, "Branchville"},
    {163, "Redding"},
    {164, "Bethel"},
    {165, "Danbury"},
    {167, "Derby-Shelton"},
    {168, "Ansonia"},
    {169, "Seymour"},
    {170, "Beacon Falls"},
    {171, "Naugatuck"},
    {172, "Waterbury"},
    {176, "Tenmile River"},
    {177, "Wassaic"},
    {184, "Highbridge Yard"},
    {188, "Fairfield-Black Rock"},
    {190, "West Haven"},
    {622, "Yankees-E 153 St"},
    {651, "Springdale MW Facility"},
};
static const int MNR_STOPS_N = 114;

static const MtaRouteNm LIRR_ROUTES[] = {
    {"1", "Babylon"},
    {"2", "Hempstead"},
    {"3", "Oyster Bay"},
    {"4", "Ronkonkoma"},
    {"5", "Montauk"},
    {"6", "Long Beach"},
    {"7", "Far Rockaway"},
    {"8", "W Hempstead"},
    {"9", "Pt Washington"},
    {"10", "Pt Jefferson"},
    {"11", "Belmont"},
    {"12", "City Zone"},
    {"13", "Greenport"},
};
static const int LIRR_ROUTES_N = 13;

static const MtaRouteNm MNR_ROUTES[] = {
    {"1", "Hudson"},
    {"2", "Harlem"},
    {"3", "New Haven"},
    {"4", "New Canaan"},
    {"5", "Danbury"},
    {"6", "Waterbury"},
};
static const int MNR_ROUTES_N = 6;

static const RailPt LIRR_BABYLON[] = {
    {40.75059f, -73.99358f, "Penn Station"},
    {40.71958f, -73.84481f, nullptr},
    {40.70965f, -73.83089f, nullptr},
    {40.69961f, -73.80853f, nullptr},
    {40.69118f, -73.76551f, nullptr},
    {40.65606f, -73.67607f, nullptr},
    {40.65832f, -73.64655f, nullptr},
    {40.65673f, -73.60716f, nullptr},
    {40.65746f, -73.58232f, nullptr},
    {40.66380f, -73.55062f, nullptr},
    {40.66880f, -73.52886f, nullptr},
    {40.67299f, -73.50896f, nullptr},
    {40.67572f, -73.48657f, nullptr},
    {40.67693f, -73.46906f, nullptr},
    {40.67786f, -73.45474f, nullptr},
    {40.68025f, -73.42031f, nullptr},
    {40.68102f, -73.39834f, nullptr},
    {40.68827f, -73.36921f, nullptr},
    {40.70069f, -73.32406f, "Babylon"},
    {RAIL_END, 0, nullptr}};

static const RailPt LIRR_HEMPSTEAD[] = {
    {40.71330f, -73.62503f, "Hempstead"},
    {40.72146f, -73.62967f, nullptr},
    {40.72310f, -73.64036f, nullptr},
    {40.72296f, -73.66270f, nullptr},
    {40.72303f, -73.68102f, nullptr},
    {40.72464f, -73.70640f, nullptr},
    {40.72220f, -73.71665f, nullptr},
    {40.72007f, -73.72555f, nullptr},
    {40.71746f, -73.73646f, nullptr},
    {40.71018f, -73.76675f, nullptr},
    {40.70774f, -73.77696f, nullptr},
    {40.69961f, -73.80853f, nullptr},
    {40.70965f, -73.83089f, nullptr},
    {40.71958f, -73.84481f, nullptr},
    {40.75516f, -73.97545f, "Grand Central"},
    {RAIL_END, 0, nullptr}};

static const RailPt LIRR_OYSTER_BAY[] = {
    {40.87534f, -73.53403f, "Oyster Bay"},
    {40.87447f, -73.59830f, nullptr},
    {40.86583f, -73.61617f, nullptr},
    {40.85798f, -73.62122f, nullptr},
    {40.85237f, -73.62542f, nullptr},
    {40.83223f, -73.62612f, nullptr},
    {40.81572f, -73.62687f, nullptr},
    {40.79048f, -73.64326f, nullptr},
    {40.77206f, -73.64169f, nullptr},
    {40.75602f, -73.63941f, nullptr},
    {40.74035f, -73.64086f, nullptr},
    {40.69961f, -73.80853f, nullptr},
    {40.74239f, -73.94679f, nullptr},
    {40.74134f, -73.95764f, "Long Island City"},
    {RAIL_END, 0, nullptr}};

static const RailPt LIRR_RONKONKOMA[] = {
    {40.80809f, -73.10594f, "Ronkonkoma"},
    {40.79185f, -73.19486f, nullptr},
    {40.78083f, -73.24361f, nullptr},
    {40.76948f, -73.29356f, nullptr},
    {40.75480f, -73.35807f, nullptr},
    {40.73592f, -73.44124f, nullptr},
    {40.74304f, -73.48344f, nullptr},
    {40.76717f, -73.52853f, nullptr},
    {40.74035f, -73.64086f, nullptr},
    {40.73517f, -73.66252f, nullptr},
    {40.73076f, -73.68096f, nullptr},
    {40.70774f, -73.77696f, nullptr},
    {40.69961f, -73.80853f, nullptr},
    {40.70965f, -73.83089f, nullptr},
    {40.71958f, -73.84481f, nullptr},
    {40.75059f, -73.99358f, "Penn Station"},
    {RAIL_END, 0, nullptr}};

static const RailPt LIRR_MONTAUK[] = {
    {40.75059f, -73.99358f, "Penn Station"},
    {40.69961f, -73.80853f, nullptr},
    {40.70069f, -73.32406f, nullptr},
    {40.72443f, -73.25408f, nullptr},
    {40.73583f, -73.20932f, nullptr},
    {40.74044f, -73.17020f, nullptr},
    {40.74343f, -73.13244f, nullptr},
    {40.74035f, -73.08646f, nullptr},
    {40.76188f, -73.01574f, nullptr},
    {40.77374f, -72.94397f, nullptr},
    {40.79899f, -72.86442f, nullptr},
    {40.82132f, -72.70526f, nullptr},
    {40.83031f, -72.65032f, nullptr},
    {40.87661f, -72.52395f, nullptr},
    {40.89472f, -72.39012f, nullptr},
    {40.93898f, -72.31005f, nullptr},
    {40.96509f, -72.19324f, nullptr},
    {40.98004f, -72.13233f, nullptr},
    {41.04711f, -71.95388f, "Montauk"},
    {RAIL_END, 0, nullptr}};

static const RailPt LIRR_LONG_BEACH[] = {
    {40.59018f, -73.66482f, "Long Beach"},
    {40.60130f, -73.65474f, nullptr},
    {40.63472f, -73.65467f, nullptr},
    {40.64221f, -73.65822f, nullptr},
    {40.64832f, -73.66397f, nullptr},
    {40.65606f, -73.67607f, nullptr},
    {40.66152f, -73.70476f, nullptr},
    {40.66595f, -73.73555f, nullptr},
    {40.66848f, -73.75175f, nullptr},
    {40.67514f, -73.76504f, nullptr},
    {40.69961f, -73.80853f, nullptr},
    {40.70965f, -73.83089f, nullptr},
    {40.71958f, -73.84481f, nullptr},
    {40.74585f, -73.90298f, nullptr},
    {40.75059f, -73.99358f, "Penn Station"},
    {RAIL_END, 0, nullptr}};

static const RailPt LIRR_FAR_ROCKAWAY[] = {
    {40.60914f, -73.75054f, "Far Rockaway"},
    {40.61229f, -73.74418f, nullptr},
    {40.61573f, -73.73590f, nullptr},
    {40.62217f, -73.72618f, nullptr},
    {40.63134f, -73.71372f, nullptr},
    {40.63676f, -73.70514f, nullptr},
    {40.64925f, -73.70183f, nullptr},
    {40.66152f, -73.70476f, nullptr},
    {40.66595f, -73.73555f, nullptr},
    {40.66848f, -73.75175f, nullptr},
    {40.67514f, -73.76504f, nullptr},
    {40.69961f, -73.80853f, nullptr},
    {40.70965f, -73.83089f, nullptr},
    {40.71958f, -73.84481f, nullptr},
    {40.74585f, -73.90298f, nullptr},
    {40.75059f, -73.99358f, "Penn Station"},
    {RAIL_END, 0, nullptr}};

static const RailPt LIRR_W_HEMPSTEAD[] = {
    {40.75516f, -73.97545f, "Grand Central"},
    {40.71958f, -73.84481f, nullptr},
    {40.70965f, -73.83089f, nullptr},
    {40.69961f, -73.80853f, nullptr},
    {40.69118f, -73.76551f, nullptr},
    {40.66837f, -73.68121f, nullptr},
    {40.67548f, -73.66886f, nullptr},
    {40.68586f, -73.65214f, nullptr},
    {40.69491f, -73.64621f, nullptr},
    {40.70196f, -73.64164f, "West Hempstead"},
    {RAIL_END, 0, nullptr}};

static const RailPt LIRR_PT_WASHINGTON[] = {
    {40.75059f, -73.99358f, "Penn Station"},
    {40.74585f, -73.90298f, nullptr},
    {40.75240f, -73.84370f, nullptr},
    {40.75789f, -73.83135f, nullptr},
    {40.76271f, -73.81454f, nullptr},
    {40.76165f, -73.80177f, nullptr},
    {40.76144f, -73.78996f, nullptr},
    {40.76315f, -73.77125f, nullptr},
    {40.76807f, -73.74941f, nullptr},
    {40.77504f, -73.74065f, nullptr},
    {40.78722f, -73.72610f, nullptr},
    {40.79672f, -73.69990f, nullptr},
    {40.81070f, -73.69521f, nullptr},
    {40.82904f, -73.68740f, "Port Washington"},
    {RAIL_END, 0, nullptr}};

static const RailPt LIRR_PT_JEFFERSON[] = {
    {40.93455f, -73.05250f, "Port Jefferson"},
    {40.92032f, -73.12855f, nullptr},
    {40.88217f, -73.15951f, nullptr},
    {40.85655f, -73.19803f, nullptr},
    {40.88367f, -73.25625f, nullptr},
    {40.88065f, -73.32849f, nullptr},
    {40.86867f, -73.36285f, nullptr},
    {40.85301f, -73.40953f, nullptr},
    {40.76717f, -73.52853f, nullptr},
    {40.69961f, -73.80853f, nullptr},
    {40.74239f, -73.94679f, nullptr},
    {40.74134f, -73.95764f, "Long Island City"},
    {RAIL_END, 0, nullptr}};

static const RailPt LIRR_CITY_ZONE[] = {
    {40.75516f, -73.97545f, "Grand Central"},
    {40.74585f, -73.90298f, nullptr},
    {40.71958f, -73.84481f, nullptr},
    {40.70965f, -73.83089f, nullptr},
    {40.69961f, -73.80853f, "Jamaica"},
    {RAIL_END, 0, nullptr}};

static const RailPt LIRR_GREENPORT[] = {
    {41.09971f, -72.36310f, "Greenport"},
    {41.06632f, -72.42788f, nullptr},
    {40.99179f, -72.53606f, nullptr},
    {40.91984f, -72.66691f, nullptr},
    {40.84927f, -72.86447f, nullptr},
    {40.81740f, -72.99891f, nullptr},
    {40.80809f, -73.10594f, "Ronkonkoma"},
    {RAIL_END, 0, nullptr}};

static const RailPt MNR_HUDSON[] = {
    {40.75300f, -73.97706f, "Grand Central"},
    {40.80516f, -73.93915f, nullptr},
    {40.87433f, -73.91094f, nullptr},
    {40.87824f, -73.92145f, nullptr},
    {40.90398f, -73.91413f, nullptr},
    {40.93579f, -73.90267f, nullptr},
    {40.99411f, -73.88451f, nullptr},
    {41.01246f, -73.87949f, nullptr},
    {41.03999f, -73.87308f, nullptr},
    {41.07647f, -73.86456f, nullptr},
    {41.13576f, -73.86616f, nullptr},
    {41.15766f, -73.86928f, nullptr},
    {41.18990f, -73.88239f, nullptr},
    {41.24626f, -73.92188f, nullptr},
    {41.28596f, -73.93042f, nullptr},
    {41.38178f, -73.94720f, nullptr},
    {41.41528f, -73.95809f, nullptr},
    {41.50401f, -73.98453f, nullptr},
    {41.58745f, -73.94723f, nullptr},
    {41.70584f, -73.93795f, "Poughkeepsie"},
    {RAIL_END, 0, nullptr}};

static const RailPt MNR_HARLEM[] = {
    {40.75300f, -73.97706f, "Grand Central"},
    {40.80516f, -73.93915f, nullptr},
    {41.03259f, -73.77521f, nullptr},
    {41.04981f, -73.77314f, nullptr},
    {41.15801f, -73.77488f, nullptr},
    {41.29434f, -73.67766f, nullptr},
    {41.32578f, -73.65906f, nullptr},
    {41.34772f, -73.66227f, nullptr},
    {41.39447f, -73.61980f, nullptr},
    {41.41320f, -73.62379f, nullptr},
    {41.51183f, -73.60458f, nullptr},
    {41.56421f, -73.60052f, nullptr},
    {41.63752f, -73.57145f, nullptr},
    {41.74040f, -73.57650f, nullptr},
    {41.77994f, -73.55820f, nullptr},
    {41.81472f, -73.56220f, "Wassaic"},
    {RAIL_END, 0, nullptr}};

static const RailPt MNR_NEW_HAVEN[] = {
    {40.75300f, -73.97706f, "Grand Central"},
    {40.80516f, -73.93915f, nullptr},
    {40.86150f, -73.89058f, nullptr},
    {40.91216f, -73.83218f, nullptr},
    {40.91032f, -73.81024f, nullptr},
    {40.91161f, -73.78381f, nullptr},
    {40.93339f, -73.75979f, nullptr},
    {40.95406f, -73.73613f, nullptr},
    {40.96943f, -73.71296f, nullptr},
    {40.98592f, -73.68255f, nullptr},
    {41.00073f, -73.66470f, nullptr},
    {41.02128f, -73.62462f, nullptr},
    {41.03017f, -73.59831f, nullptr},
    {41.03168f, -73.58817f, nullptr},
    {41.03382f, -73.56586f, nullptr},
    {41.04661f, -73.54285f, nullptr},
    {41.06904f, -73.49788f, nullptr},
    {41.07691f, -73.47297f, nullptr},
    {41.07746f, -73.44553f, nullptr},
    {41.09673f, -73.42113f, nullptr},
    {41.10400f, -73.40459f, nullptr},
    {41.11893f, -73.37141f, nullptr},
    {41.12226f, -73.31541f, nullptr},
    {41.13484f, -73.28897f, nullptr},
    {41.14308f, -73.25774f, nullptr},
    {41.16100f, -73.23434f, nullptr},
    {41.17868f, -73.18708f, nullptr},
    {41.19425f, -73.13153f, nullptr},
    {41.22323f, -73.05765f, nullptr},
    {41.27142f, -72.96349f, nullptr},
    {41.29650f, -72.92829f, "New Haven"},
    {RAIL_END, 0, nullptr}};

static const RailPt MNR_NEW_CANAAN[] = {
    {40.75300f, -73.97706f, "Grand Central"},
    {40.80516f, -73.93915f, nullptr},
    {40.86150f, -73.89058f, nullptr},
    {40.91216f, -73.83218f, nullptr},
    {40.91032f, -73.81024f, nullptr},
    {40.91161f, -73.78381f, nullptr},
    {40.93339f, -73.75979f, nullptr},
    {40.95406f, -73.73613f, nullptr},
    {40.96943f, -73.71296f, nullptr},
    {40.98592f, -73.68255f, nullptr},
    {41.00073f, -73.66470f, nullptr},
    {41.02128f, -73.62462f, nullptr},
    {41.03017f, -73.59831f, nullptr},
    {41.03168f, -73.58817f, nullptr},
    {41.03382f, -73.56586f, nullptr},
    {41.04661f, -73.54285f, nullptr},
    {41.07055f, -73.52002f, nullptr},
    {41.08876f, -73.51783f, nullptr},
    {41.11601f, -73.49815f, nullptr},
    {41.14630f, -73.49563f, "New Canaan"},
    {RAIL_END, 0, nullptr}};

static const RailPt MNR_DANBURY[] = {
    {40.75300f, -73.97706f, "Grand Central"},
    {40.80516f, -73.93915f, nullptr},
    {41.02128f, -73.62462f, nullptr},
    {41.04661f, -73.54285f, nullptr},
    {41.06904f, -73.49788f, nullptr},
    {41.07691f, -73.47297f, nullptr},
    {41.07746f, -73.44553f, nullptr},
    {41.09673f, -73.42113f, nullptr},
    {41.14662f, -73.42786f, nullptr},
    {41.19620f, -73.43243f, nullptr},
    {41.21662f, -73.42670f, nullptr},
    {41.26763f, -73.44142f, nullptr},
    {41.32568f, -73.43380f, nullptr},
    {41.37622f, -73.41817f, nullptr},
    {41.39615f, -73.44879f, "Danbury"},
    {RAIL_END, 0, nullptr}};

static const RailPt MNR_WATERBURY[] = {
    {41.17868f, -73.18708f, "Bridgeport"},
    {41.19425f, -73.13153f, nullptr},
    {41.31972f, -73.08355f, nullptr},
    {41.34416f, -73.07989f, nullptr},
    {41.39514f, -73.07250f, nullptr},
    {41.44175f, -73.06359f, nullptr},
    {41.49420f, -73.05266f, nullptr},
    {41.55273f, -73.04613f, "Waterbury"},
    {RAIL_END, 0, nullptr}};

static const RailPt *const MTA_RAIL[] = {
    LIRR_BABYLON,
    LIRR_HEMPSTEAD,
    LIRR_OYSTER_BAY,
    LIRR_RONKONKOMA,
    LIRR_MONTAUK,
    LIRR_LONG_BEACH,
    LIRR_FAR_ROCKAWAY,
    LIRR_W_HEMPSTEAD,
    LIRR_PT_WASHINGTON,
    LIRR_PT_JEFFERSON,
    LIRR_CITY_ZONE,
    LIRR_GREENPORT,
    MNR_HUDSON,
    MNR_HARLEM,
    MNR_NEW_HAVEN,
    MNR_NEW_CANAAN,
    MNR_DANBURY,
    MNR_WATERBURY,
};
static const int MTA_RAIL_N = 18;

static const char *mtaFindStop(const MtaStopNm *tab, int n, const char *id) {
  if (!id || !id[0]) return "";
  int v = 0;
  for (const char *p = id; *p; p++) {
    if (*p < '0' || *p > '9') return "";
    v = v * 10 + (*p - '0');
    if (v > 65535) return "";
  }
  int lo = 0, hi = n - 1;
  while (lo <= hi) {
    int m = (lo + hi) >> 1;
    if (tab[m].id == (uint16_t)v) return tab[m].name;
    if (tab[m].id < (uint16_t)v) lo = m + 1;
    else hi = m - 1;
  }
  return "";
}

static const char *mtaFindRoute(const MtaRouteNm *tab, int n, const char *id) {
  if (!id || !id[0]) return "";
  for (int i = 0; i < n; i++)
    if (!strcmp(tab[i].id, id)) return tab[i].name;
  return "";
}

struct Pb {
  const uint8_t *s;
  int n;
  int i;
};

struct MtaVeh {
  char trip[28];
  char route[16];
  char label[12];
  char stop[8];
  float lat, lon, bearing;
  double speed;
  bool hasPos, hasBrg, hasSpd;
  uint32_t ts;
  int rel;
};

struct MtaTu {
  char trip[28];
  char route[16];
  char orig[8];
  char dest[8];
  int rel;
  int delay;
};

