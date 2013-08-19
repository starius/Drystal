#include <lua.hpp>

#ifndef EMSCRIPTEN
#include "file.hpp"
#endif

#include "engine.hpp"

// used to access some engine's fields from lua callbacks
static Engine *engine;

LuaFunctions::LuaFunctions(Engine& eng, const char *filename) :
	L(luaL_newstate()),
	filename(filename)
#ifndef EMSCRIPTEN
	, last_load(0)
#endif
{
	engine = &eng;
	luaL_openlibs(L);
	send_globals();
}

LuaFunctions::~LuaFunctions()
{
	lua_close(L);
}

bool LuaFunctions::load_code()
{
	if (luaL_dofile(L, filename)) {
		fprintf(stderr, "[ERROR] cannot run script: %s\n", lua_tostring(L, -1));
		return false;
	}

	lua_getglobal(L, "init");
	if (not lua_isfunction(L, -1)) {
		lua_pop(L, 1);
		fprintf(stderr, "[ERROR] cannot find init function in `%s'\n", filename);
		return false;
	} else if (lua_pcall(L, 0, 0, 0)) {
		fprintf(stderr, "[ERROR] cannot call init: %s\n", lua_tostring(L, -1));
		return false;
	}
#ifndef EMSCRIPTEN
	time_t last = last_modified(filename);
	last_load = last;
#endif
	return true;
}

bool LuaFunctions::reload_code()
{
#ifndef EMSCRIPTEN
	time_t last = last_modified(filename);
	if (last == 0) {
		perror("can't reload");
		return false;
	}
	if (last_load < last) {
		// reload the code
		printf("Reloading code...\n");
		return load_code();
	}
#endif
	return false;
}

void LuaFunctions::call_mouse_motion(int mx, int my, int dx, int dy) const
{
	lua_getglobal(L, "mouse_motion");
	if (not lua_isfunction(L, -1))
	{
		lua_pop(L, 1);
		return;
	}
	lua_pushnumber(L, mx);
	lua_pushnumber(L, my);
	lua_pushnumber(L, dx);
	lua_pushnumber(L, dy);
	if (lua_pcall(L, 4, 0, 0))
	{
		luaL_error(L, "error calling mouse_motion: %s", lua_tostring(L, -1));
	}
}

void LuaFunctions::call_mouse_press(int mx, int my, int button) const
{
	lua_getglobal(L, "mouse_press");
	if (not lua_isfunction(L, -1))
	{
		lua_pop(L, 1);
		return;
	}
	lua_pushnumber(L, mx);
	lua_pushnumber(L, my);
	lua_pushnumber(L, button);
	if (lua_pcall(L, 3, 0, 0))
	{
		luaL_error(L, "error calling mouse_press: %s", lua_tostring(L, -1));
	}
}

void LuaFunctions::call_mouse_release(int mx, int my, int button) const
{
	lua_getglobal(L, "mouse_release");
	if (not lua_isfunction(L, -1))
	{
		lua_pop(L, 1);
		return;
	}
	lua_pushnumber(L, mx);
	lua_pushnumber(L, my);
	lua_pushnumber(L, button);
	if (lua_pcall(L, 3, 0, 0))
	{
		luaL_error(L, "error calling mouse_release: %s", lua_tostring(L, -1));
	}
}

void LuaFunctions::call_key_press(const char* key_string) const
{
	lua_getglobal(L, "key_press");
	if (not lua_isfunction(L, -1))
	{
		lua_pop(L, 1);
		return;
	}
	lua_pushstring(L, key_string);
	if (lua_pcall(L, 1, 0, 0))
	{
		luaL_error(L, "error calling key_press: %s", lua_tostring(L, -1));
	}
}

void LuaFunctions::call_key_release(const char* key_string) const
{
	lua_getglobal(L, "key_release");
	if (not lua_isfunction(L, -1))
	{
		lua_pop(L, 1);
		return;
	}
	lua_pushstring(L, key_string);
	if (lua_pcall(L, 1, 0, 0))
	{
		luaL_error(L, "error calling key_release: %s", lua_tostring(L, -1));
	}
}

void LuaFunctions::call_resize_event(int w, int h) const
{
	lua_getglobal(L, "resize_event");
	if (not lua_isfunction(L, -1))
	{
		lua_pop(L, 1);
		return;
	}
	lua_pushnumber(L, w);
	lua_pushnumber(L, h);
	if (lua_pcall(L, 2, 0, 0))
	{
		luaL_error(L, "error calling resize_event: %s", lua_tostring(L, -1));
	}
}

bool LuaFunctions::call_update(double dt)
{
	lua_getglobal(L, "update");
	if (not lua_isfunction(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	lua_pushnumber(L, dt);
	if (lua_pcall(L, 1, 0, 0)) {
		luaL_error(L, "error calling update: %s", lua_tostring(L, -1));
		return false;
	}
	return true;
}
bool LuaFunctions::call_draw()
{
	lua_getglobal(L, "draw");
	if (not lua_isfunction(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	if (lua_pcall(L, 0, 0, 0)) {
		luaL_error(L, "error calling draw: %s", lua_tostring(L, -1));
		return false;
	}
	return true;
}

static int mlua_engine_stop(lua_State*)
{
	engine->stop();
	return 0;
}

static int mlua_set_color(lua_State* L)
{
	int r = lua_tointeger(L, -3);
	int g = lua_tointeger(L, -2);
	int b = lua_tointeger(L, -1);
	engine->display.set_color(r, g, b);
	return 0;
}
static int mlua_set_alpha(lua_State* L)
{
	int alpha = lua_tointeger(L, -1);
	engine->display.set_alpha(alpha);
	return 0;
}

static int mlua_show_cursor(lua_State* L)
{
	int show = lua_toboolean(L, -1);
	engine->display.show_cursor(show);
	return 0;
}
static int mlua_grab_cursor(lua_State* L)
{
	int grab = lua_toboolean(L, -1);
	engine->event.grab_cursor(grab);
	return 0;
}
static int mlua_set_resizable(lua_State* L)
{
	int r = lua_toboolean(L, -1);
	engine->display.set_resizable(r);
	return 0;
}
static int mlua_resize(lua_State* L)
{
	int w = lua_tointeger(L, -2);
	int h = lua_tointeger(L, -1);
	engine->display.resize(w, h);
	lua_pushlightuserdata(L, engine->display.get_screen());
	lua_setglobal(L, "screen");
	return 0;
}
static int mlua_flip(lua_State*)
{
	engine->display.flip();
	return 0;
}
static int mlua_load_surface(lua_State* L)
{
	const char * filename = lua_tostring(L, -1);
	void* surface = engine->display.load_surface(filename);
	if (surface) {
		lua_pushlightuserdata(L, surface);
		return 1;
	}
	return 0;
}
static int mlua_new_surface(lua_State* L)
{
	int w = lua_tointeger(L, -2);
	int h = lua_tointeger(L, -1);
	void* surface = engine->display.new_surface(w, h);
	lua_pushlightuserdata(L, surface);
	return 1;
}
static int mlua_free_surface(lua_State* L)
{
	Surface* surface = (Surface*) lua_touserdata(L, -1);
	engine->display.free_surface(surface);
	return 0;
}
static int mlua_surface_size(lua_State* L)
{
	Surface* surface = (Surface *) lua_touserdata(L, -1);
	int w, h;
	engine->display.surface_size(surface, &w, &h);
	lua_pushnumber(L, w);
	lua_pushnumber(L, h);
	return 2;
}
static int mlua_draw_on(lua_State* L)
{
	Surface* surface = (Surface*) lua_touserdata(L, -1);
	engine->display.draw_on(surface);
	return 0;
}
static int mlua_draw_from(lua_State* L)
{
	Surface* surface = (Surface*) lua_touserdata(L, -1);
	engine->display.draw_from(surface);
	return 0;
}

static int mlua_draw_background(lua_State*)
{
	engine->display.draw_background();
	return 0;
}
static int mlua_draw_triangle(lua_State* L)
{
	int x1 = lua_tonumber(L, -6);
	int y1 = lua_tonumber(L, -5);
	int x2 = lua_tonumber(L, -4);
	int y2 = lua_tonumber(L, -3);
	int x3 = lua_tonumber(L, -2);
	int y3 = lua_tonumber(L, -1);
	engine->display.draw_triangle(x1, y1, x2, y2, x3, y3);
	return 0;
}
static int mlua_draw_line(lua_State* L)
{
	int x1 = lua_tonumber(L, -4);
	int y1 = lua_tonumber(L, -3);
	int x2 = lua_tonumber(L, -2);
	int y2 = lua_tonumber(L, -1);
	engine->display.draw_line(x1, y1, x2, y2);
	return 0;
}
static int mlua_draw_surface(lua_State* L)
{
	int i1 = lua_tonumber(L, -16);
	int i2 = lua_tonumber(L, -15);
	int i3 = lua_tonumber(L, -14);
	int i4 = lua_tonumber(L, -13);
	int i5 = lua_tonumber(L, -12);
	int i6 = lua_tonumber(L, -11);
	int i7 = lua_tonumber(L, -10);
	int i8 = lua_tonumber(L, -9);
	int o1 = lua_tonumber(L, -8);
	int o2 = lua_tonumber(L, -7);
	int o3 = lua_tonumber(L, -6);
	int o4 = lua_tonumber(L, -5);
	int o5 = lua_tonumber(L, -4);
	int o6 = lua_tonumber(L, -3);
	int o7 = lua_tonumber(L, -2);
	int o8 = lua_tonumber(L, -1);
	engine->display.draw_surface(i1, i2, i3, i4, i5, i6, i7, i8,
			o1, o2, o3, o4, o5, o6, o7, o8);
	return 0;
}


static int mlua_new_shader(lua_State* L)
{
	const char *vert, *frag;
	if (lua_gettop(L) == 1) { // one argument, it's the vertex shader
		vert = lua_tostring(L, -1);
		frag = nullptr;
	} else { // two arguments
		vert = lua_tostring(L, -2);
		frag = lua_tostring(L, -1);
	}
	// null code will be set to defaut shader
	Shader* shader = engine->display.new_shader(vert, frag);
	if (shader) {
		lua_pushlightuserdata(L, shader);
		return 1;
	}
	return 0; // returns nil
}
static int mlua_use_shader(lua_State* L)
{
	if (lua_gettop(L) == 0) { // use defaut shader
		engine->display.use_shader(nullptr);
	} else {
		Shader* shader = (Shader*) lua_touserdata(L, -1);
		engine->display.use_shader(shader);
	}
	return 0;
}

static int mlua_feed_shader(lua_State* L)
{
	Shader* shader = (Shader*) lua_touserdata(L, 1);
	const char* name = lua_tostring(L, 2);
	float value = lua_tonumber(L, 3);
	engine->display.feed_shader(shader, name, value);
	return 0;
}
static int mlua_free_shader(lua_State* L)
{
	Shader* shader = (Shader*) lua_touserdata(L, 1);
	engine->display.free_shader(shader);
	return 0;
}

static int mlua_new_buffer(lua_State* L)
{
	Buffer* buffer;
	if (lua_gettop(L) == 1) {
		lua_Number size = luaL_checknumber(L, 1);
		buffer = engine->display.new_buffer(size);
	} else {
		buffer = engine->display.new_buffer(); // let Display choose a size
	}
	if (buffer) {
		lua_pushlightuserdata(L, buffer);
		return 1;
	}
	return 0; // returns nil
}
static int mlua_use_buffer(lua_State* L)
{
	if (lua_gettop(L) == 0) { // use defaut buffer
		engine->display.use_buffer(nullptr);
	} else {
		Buffer* buffer = (Buffer*) lua_touserdata(L, -1);
		engine->display.use_buffer(buffer);
	}
	return 0;
}
static int mlua_draw_buffer(lua_State* L)
{
	Buffer* buffer = (Buffer*) lua_touserdata(L, 1);
	lua_Number dx = 0, dy = 0;
	if (lua_gettop(L) >= 2)
		dx = luaL_checknumber(L, 2);
	if (lua_gettop(L) >= 3)
		dy = luaL_checknumber(L, 3);
	engine->display.draw_buffer(buffer, dx, dy);
	return 0;
}
static int mlua_free_buffer(lua_State* L)
{
	Buffer* buffer = (Buffer*) lua_touserdata(L, -1);
	engine->display.free_buffer(buffer);
	return 0;
}

static int mlua_load_sound(lua_State *L)
{
	const char *filepath = lua_tostring(L, -1);
	void *chunk = engine->audio.load_sound(filepath);
	lua_pushlightuserdata(L, chunk);
	return 1;
}

static int mlua_play_sound(lua_State *L)
{
	Mix_Chunk *chunk = (Mix_Chunk *) lua_touserdata(L, 1);
	int times = 1;
	if (!lua_isnone(L, 2))
		times = lua_tonumber(L, 2);
	if (times == -1)
		times = 0;

	float volume = -1;
	if (!lua_isnone(L, 3))
		volume = lua_tonumber(L, 3);

	engine->audio.play_sound(chunk, times, volume);
	return 0;
}

static int mlua_free_sound(lua_State *L)
{
	Mix_Chunk *chunk = (Mix_Chunk *) lua_touserdata(L, -1);
	engine->audio.free_sound(chunk);
	return 0;
}

static int mlua_play_music_queued(lua_State *L)
{
	const char *filepath = lua_tostring(L, -1);
	engine->audio.play_music_queued(strdup(filepath));
	return 0;
}

static int mlua_play_music(lua_State *L)
{
	const char *filepath = lua_tostring(L, 1);
	int times = 1;
	if (!lua_isnone(L, 2))
		times = lua_tonumber(L, 2);
	engine->audio.play_music(filepath, times);
	return 0;
}

static int mlua_set_sound_volume(lua_State *L)
{
	float volume = lua_tonumber(L, 1);
	engine->audio.set_sound_volume(volume);
	return 0;
}

static int mlua_set_music_volume(lua_State *L)
{
	float volume = lua_tonumber(L, 1);
	engine->audio.set_music_volume(volume);
	return 0;
}

static int mlua_stop_music(lua_State* L)
{
	(void) L;
	engine->audio.stop_music();
	return 0;
}


//
// Lua load
//

void LuaFunctions::send_globals() const
{
	lua_pushlightuserdata(L, engine->display.get_screen());
	lua_setglobal(L, "screen");

	lua_register(L, "engine_stop", mlua_engine_stop);

	lua_register(L, "show_cursor", mlua_show_cursor);
	lua_register(L, "grab_cursor", mlua_grab_cursor);

	lua_register(L, "set_resizable", mlua_set_resizable);
	lua_register(L, "resize", mlua_resize);
	lua_register(L, "flip", mlua_flip);

	lua_register(L, "load_surface", mlua_load_surface);
	lua_register(L, "new_surface", mlua_new_surface);
	lua_register(L, "free_surface", mlua_free_surface);
	lua_register(L, "surface_size", mlua_surface_size);
	lua_register(L, "draw_on", mlua_draw_on);

	lua_register(L, "draw_from", mlua_draw_from);

	lua_register(L, "draw_background", mlua_draw_background);
	lua_register(L, "draw_triangle", mlua_draw_triangle);
	lua_register(L, "draw_line", mlua_draw_line);
	lua_register(L, "draw_surface", mlua_draw_surface);

	lua_register(L, "set_color", mlua_set_color);
	lua_register(L, "set_alpha", mlua_set_alpha);

	lua_register(L, "new_shader", mlua_new_shader);
	lua_register(L, "use_shader", mlua_use_shader);
	lua_register(L, "feed_shader", mlua_feed_shader);
	lua_register(L, "free_shader", mlua_free_shader);

	lua_register(L, "new_buffer", mlua_new_buffer);
	lua_register(L, "use_buffer", mlua_use_buffer);
	lua_register(L, "draw_buffer", mlua_draw_buffer);
	lua_register(L, "free_buffer", mlua_free_buffer);

	lua_register(L, "play_music", mlua_play_music);
	lua_register(L, "play_music_queued", mlua_play_music_queued);
	lua_register(L, "set_music_volume", mlua_set_music_volume);
	lua_register(L, "stop_music", mlua_stop_music);

	lua_register(L, "load_sound", mlua_load_sound);
	lua_register(L, "play_sound", mlua_play_sound);
	lua_register(L, "set_sound_volume", mlua_set_sound_volume);
	lua_register(L, "free_sound", mlua_free_sound);
}

