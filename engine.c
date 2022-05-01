#define ENGINE
#include "data.h"
#include "parser.c"
#include <string.h>

lua_State *L = NULL;
int buttons[4] = {0,0,0,0};

static Spritesheet spritesheet;
static Spritesheet fontsheet;
static uint8_t map_data[32 * 128];

static uint8_t original_palette[][3] = {
    {0, 0, 0}, //	black
    {29, 43, 83}, //	dark-blue
    {126, 37, 83}, //	dark-purple
    {0, 135, 81}, //	dark-green
    {171, 82, 54}, //	brown
    {95, 87, 79}, //	dark-grey
    {194, 195, 199}, //	light-grey
    {255, 241, 232}, //	white
    {255, 0, 77}, //	red
    {255, 163, 0}, //	orange
    {255, 236, 39}, //	yellow
    {0, 228, 54}, //	green
    {41, 173, 255}, //	blue
    {131, 118, 156}, //	lavender
    {255, 119, 168}, //	pink
    {255, 204, 170}, //	light-peach 
};
static uint8_t palette[][3] = {
    {0, 0, 0}, //	black
    {29, 43, 83}, //	dark-blue
    {126, 37, 83}, //	dark-purple
    {0, 135, 81}, //	dark-green
    {171, 82, 54}, //	brown
    {95, 87, 79}, //	dark-grey
    {194, 195, 199}, //	light-grey
    {255, 241, 232}, //	white
    {255, 0, 77}, //	red
    {255, 163, 0}, //	orange
    {255, 236, 39}, //	yellow
    {0, 228, 54}, //	green
    {41, 173, 255}, //	blue
    {131, 118, 156}, //	lavender
    {255, 119, 168}, //	pink
    {255, 204, 170}, //	light-peach 
};

void render(Spritesheet* s, uint8_t n, uint8_t x0, uint8_t y0, int paletteIdx);
static inline void put_pixel(uint8_t x, uint8_t y, const uint8_t* p);
static void gfx_map(uint8_t mapX, uint8_t mapY,
		    uint8_t screenX, uint8_t screenY,
		    uint8_t cellW, uint8_t cellH, uint8_t layerFlags);
void gfx_cls();
void gfx_rectfill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t* color);
void gfx_circle(int32_t centreX, int32_t centreY, int32_t radius, uint8_t* color);
bool init_video();
bool handle_input();
void delay(uint8_t ms);
void gfx_flip();
void video_close();
uint64_t now();

static void gfx_map(uint8_t mapX, uint8_t mapY,
		    uint8_t screenX, uint8_t screenY,
		    uint8_t cellW, uint8_t cellH, uint8_t layerFlags) {

    for(uint8_t y = mapY; y < mapY+cellH; y++) {
	for(uint8_t x = mapX; x < mapX+cellW; x++) {
	    render(&spritesheet, map_data[x+y*128], screenX+(x-mapX)*8, screenY+(y-mapY)*8, -1);
	}
    }
}

bool _lua_fn_exists(char* fn) {
    lua_getglobal(L, fn);
    if (lua_isfunction(L, -1)) {
	    return true;
    } else {
	    return false;
    }
}
void _to_lua_call(char* fn) {
	lua_getglobal(L, fn);
	if (lua_pcall(L, 0, 1, 0) == LUA_OK) {
		lua_pop(L, lua_gettop(L));
	} else {
		puts(lua_tostring(L, lua_gettop(L)));
	}
}
lua_State* init_lua(char* script_text) {
    lua_State *state = luaL_newstate();
    luaL_openlibs(state);

    if (luaL_dostring(state, script_text) == LUA_OK) {
	lua_pop(state, lua_gettop(state));
	return state;
    }
    puts(lua_tostring(state, lua_gettop(state)));
    lua_close(state);
    return NULL;
}
int _lua_btn() {
    int idx = luaL_checkinteger(L, 1);
    lua_pushboolean(L, buttons[idx]);
    return 1;
}

void fontParser(char* text) {
    int spriteCount = 0;
    char* buf = malloc(129);
    do {
	readLine(&text, buf);
	gfxParser(buf, spriteCount, &fontsheet);
	spriteCount++;
    } while (*text != 0);
    free(buf);
}

void cartParser(char* text) {
    uint8_t section = 0;
    uint32_t spriteCount = 0;
    char* buf = malloc(257);
    memset(buf, 0, 257);
    memset(cart.code, 0, sizeof(cart.code));

    uint32_t lineLen = 0;
    uint32_t bytesRead = 0;
    do {
	lineLen = readLine(&text, buf);
	if (strncmp(buf, "__lua__", 7) == 0) {
	    section = 1;
	    bytesRead = 0;
	    continue;
	}
	if (strncmp(buf, "__gfx__", 7) == 0) {
	    section = 2;
	    spriteCount = 0;
	    bytesRead = 0;
	    continue;
	}
	if (strncmp(buf, "__map__", 7) == 0) {
	    section = 3;
	    spriteCount = 0;
	    bytesRead = 0;
	    continue;
	}
	switch (section) {
	    case 1:
		memcpy(cart.code+bytesRead, buf, lineLen);
		break;
	    case 2:
		gfxParser(buf, spriteCount, &spritesheet);
		spriteCount++;
		break;
	    case 3:
		mapParser(buf, spriteCount, map_data);
		spriteCount++;
		break;
	}
	bytesRead += lineLen;
    } while (*text != 0);
    free(buf);
}

void render(Spritesheet* s, uint8_t n, uint8_t x0, uint8_t y0, int paletteIdx) {
    const uint8_t sprite_count = 16;
    const uint8_t xIndex = n % sprite_count;
    const uint8_t yIndex = n / sprite_count;

    const int offset = (yIndex * 128 + xIndex) * 8;
    uint8_t idx, val;

    for (uint8_t y=0; y<8; y++) {
	for (uint8_t x=0; x<8; x++) {
	    val = s->sprite_data[offset + y*128 + x];
	    if (paletteIdx != -1) {
		    idx = paletteIdx;
	    } else {
		    idx = val;
	    }
	    if (val != 0){
		    const uint8_t* p = palette[idx];
		    put_pixel(x0+x-drawstate.camera_x, y0+y-drawstate.camera_y, p);
	    }
	}
    }
}
