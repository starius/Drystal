/**
 * This file is part of Drystal.
 *
 * Drystal is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Drystal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Drystal.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <cstring>
#include <cmath>
#include <cassert>

#include <lua.hpp>

#include "engine.hpp"
#include "log.hpp"
#include "lua_functions.hpp"
#include "luafiles.hpp"
#include "module.hpp"
#include "all_api.hpp"

static int luaopen_drystal(lua_State*); // defined at the end of this file

DECLARE_PUSHPOP(Shader, shader)
DECLARE_PUSHPOP2(Surface, surface)
DECLARE_PUSHPOP(Buffer, buffer)

LuaFunctions::LuaFunctions(const char *_filename) :
	L(luaL_newstate()),
	drystal_table_ref(LUA_NOREF),
	filename(_filename),
	library_loaded(false)
{
	luaL_openlibs(L);
}

void LuaFunctions::free()
{
	luaL_unref(L, LUA_REGISTRYINDEX, drystal_table_ref);
	lua_close(L);
}

int traceback(lua_State *L)
{
	// from lua/src/lua.c
	const char *msg = lua_tostring(L, 1);
	if (msg)
		luaL_traceback(L, L, msg, 1);
	else if (!lua_isnoneornil(L, 1)) {  /* is there an error object? */
		if (!luaL_callmeta(L, 1, "__tostring"))  /* try its 'tostring' metamethod */
			lua_pushliteral(L, "(no error message)");
	}
	return 1;
}

/**
 * Search for a function named 'name' in the drystal table.
 * Return true if found and keep the function in the lua stack
 * Otherwise, return false (stack is cleaned as needed).
 */
bool LuaFunctions::get_function(const char* name) const
{
	assert(name);

	lua_rawgeti(L, LUA_REGISTRYINDEX, drystal_table_ref);
	lua_pushstring(L, name);
	lua_rawget(L, -2);
	if (lua_isfunction(L, -1)) {
		return true;
	}
	lua_pop(L, 2);
	return false;
}

void LuaFunctions::remove_userpackages() const
{
	printf("Removing old packages: ");
	const char* kept[] = {
		"_G",
		LUA_COLIBNAME,
		LUA_TABLIBNAME,
		LUA_IOLIBNAME,
		LUA_OSLIBNAME,
		LUA_STRLIBNAME,
		LUA_BITLIBNAME,
		LUA_MATHLIBNAME,
		LUA_DBLIBNAME,
		LUA_LOADLIBNAME,
		"drystal",
	};
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "loaded");
	lua_pushnil(L);
	while (lua_next(L, -2)) {
		bool remove = true;
		const char* name = lua_tostring(L, -2);
		for (unsigned long i = 0; i < sizeof(kept) / sizeof(const char*) && remove; i++) {
			remove = remove && strcmp(name, kept[i]);
		}
		if (remove) {
			lua_pushnil(L);
			printf("%s ", name);
			lua_setfield(L, -4, name);
		}
		lua_pop(L, 1);
	}
	printf("\n");
}

bool LuaFunctions::load_code()
{
	assert(lua_gettop(L) == 0);
	lua_pushcfunction(L, traceback); // used by lua_pcall
	if (!library_loaded) {
		// add drystal lib
		luaL_requiref(L, "drystal", luaopen_drystal, 0 /* not as global */);
		register_modules();
		lua_pop(L, 1);  /* remove lib */

		if (!load_luafiles(L)) {
			fprintf(stderr, "[ERROR] cannot run script: %s\n", lua_tostring(L, -1));
			lua_pop(L, 2);
			return false;
		}
	}

	if (luaL_loadfile(L, filename) || lua_pcall(L, 0, 0, -2)) {
		fprintf(stderr, "[ERROR] cannot run script: %s\n", lua_tostring(L, -1));
		lua_pop(L, 2);
		return false;
	}

	lua_pop(L, 1);
	assert(lua_gettop(L) == 0);
	return true;
}

bool LuaFunctions::reload_code()
{
	if (get_function("prereload")) {
		CALL(0, 0);
	}
	remove_userpackages();

	printf("Reloading code...\n");
	bool ok = load_code() && call_init();
	if (ok) {
		if (get_function("postreload")) {
			CALL(0, 0);
		}
	}
	return ok;
}

bool LuaFunctions::call_init() const
{
	if (get_function("init")) {
		CALL(0, 0);
	}
	return true;
}

void LuaFunctions::call_update(float dt) const
{
	if (get_function("update")) {
		lua_pushnumber(L, dt);
		CALL(1, 0);
	}
}

void LuaFunctions::call_draw() const
{
	if (get_function("draw")) {
		CALL(0, 0);
	}
}

void LuaFunctions::call_atexit() const
{
	if (get_function("atexit")) {
		CALL(0, 0);
	}
}

static int mlua_stop(lua_State*)
{
	Engine &engine = get_engine();
	engine.stop();
	return 0;
}

static int mlua_reload(lua_State*)
{
	Engine &engine = get_engine();
	engine.lua.reload_code();
	return 0;
}

static int mlua_set_color(lua_State* L)
{
	assert(L);

	if (lua_istable(L, 1)) {
		lua_rawgeti(L, 1, 1);
		lua_rawgeti(L, 1, 2);
		lua_rawgeti(L, 1, 3);
	}
	int r = luaL_checkint(L, -3);
	int g = luaL_checkint(L, -2);
	int b = luaL_checkint(L, -1);
	Engine &engine = get_engine();
	engine.display.set_color(r, g, b);
	return 0;
}

static int mlua_set_alpha(lua_State* L)
{
	assert(L);

	int alpha = luaL_checkint(L, 1);
	Engine &engine = get_engine();
	engine.display.set_alpha(alpha);
	return 0;
}

static int mlua_set_point_size(lua_State* L)
{
	assert(L);

	lua_Number point_size = luaL_checknumber(L, 1);
	Engine &engine = get_engine();
	engine.display.set_point_size(point_size);
	return 0;
}

static int mlua_set_line_width(lua_State* L)
{
	assert(L);

	lua_Number width = luaL_checknumber(L, 1);
	Engine &engine = get_engine();
	engine.display.set_line_width(width);
	return 0;
}

static int mlua_set_title(lua_State* L)
{
	assert(L);

	const char *title = luaL_checkstring(L, 1);
	Engine &engine = get_engine();
	engine.display.set_title(title);
	return 0;
}

static int mlua_set_blend_mode(lua_State* L)
{
	assert(L);

	BlendMode mode = static_cast<BlendMode>(luaL_checknumber(L, 1));
	Engine &engine = get_engine();
	engine.display.set_blend_mode(mode);
	return 0;
}

static int mlua_camera__newindex(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	const char * name = luaL_checkstring(L, 2);
	if (!strcmp(name, "x")) {
		lua_Number dx = luaL_checknumber(L, 3);
		engine.display.set_camera_position(dx, engine.display.get_camera().dy);
	} else if (!strcmp(name, "y")) {
		lua_Number dy = luaL_checknumber(L, 3);
		engine.display.set_camera_position(engine.display.get_camera().dx, dy);
	} else if (!strcmp(name, "angle")) {
		lua_Number angle = luaL_checknumber(L, 3);
		engine.display.set_camera_angle(angle);
	} else if (!strcmp(name, "zoom")) {
		lua_Number zoom = luaL_checknumber(L, 3);
		engine.display.set_camera_zoom(zoom);
	} else {
		lua_rawset(L, 1);
	}
	return 0;
}

static int mlua_camera__index(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	const char * name = luaL_checkstring(L, 2);
	if (!strcmp(name, "x")) {
		lua_Number dx = engine.display.get_camera().dx;
		lua_pushnumber(L, dx);
		return 1;
	} else if (!strcmp(name, "y")) {
		lua_Number dy = engine.display.get_camera().dy;
		lua_pushnumber(L, dy);
		return 1;
	} else if (!strcmp(name, "angle")) {
		lua_Number angle = engine.display.get_camera().angle;
		lua_pushnumber(L, angle);
		return 1;
	} else if (!strcmp(name, "zoom")) {
		lua_Number zoom = engine.display.get_camera().zoom;
		lua_pushnumber(L, zoom);
		return 1;
	}
	return 0;
}

static int mlua_camera_reset(lua_State*)
{
	Engine &engine = get_engine();
	engine.display.reset_camera();
	return 0;
}

static int mlua_show_cursor(lua_State* L)
{
	assert(L);

	bool show = lua_toboolean(L, 1);
	Engine &engine = get_engine();
	engine.display.show_cursor(show);
	return 0;
}

static int mlua_resize(lua_State* L)
{
	assert(L);

	int w = luaL_checkint(L, 1);
	int h = luaL_checkint(L, 2);
	Engine &engine = get_engine();
	engine.display.resize(w, h);

	// make sure we don't free the screen until the next resize
	push_surface(L, engine.display.get_screen());
	lua_setfield(L, LUA_REGISTRYINDEX, "screen");
	return 0;
}

static int mlua_screen2scene(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	lua_Number x = luaL_checknumber(L, 1);
	lua_Number y = luaL_checknumber(L, 2);
	float tx, ty;
	engine.display.screen2scene(x, y, &tx, &ty);
	lua_pushnumber(L, tx);
	lua_pushnumber(L, ty);
	return 2;
}

static int mlua_surface_class_index(lua_State* L)
{
	assert(L);

	Surface* surface = pop_surface(L, 1);
	const char* index = luaL_checkstring(L, 2);
	if (strcmp(index, "w") == 0) {
		lua_pushnumber(L, surface->w);
	} else if (strcmp(index, "h") == 0) {
		lua_pushnumber(L, surface->h);
	} else {
		lua_getmetatable(L, 1);
		lua_getfield(L, -1, index);
	}
	return 1;
}

static int mlua_load_surface(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	const char * filename = luaL_checkstring(L, 1);
	Surface* surface = engine.display.load_surface(filename);
	if (surface) {
		push_surface(L, surface);
		return 1;
	}
	return luaL_fileresult(L, 0, filename);
}

static int mlua_new_surface(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	int w = luaL_checkint(L, 1);
	int h = luaL_checkint(L, 2);
	bool force_npot = lua_toboolean(L, 3);
	Surface* surface = engine.display.new_surface(w, h, force_npot);
	push_surface(L, surface);
	return 1;
}

static int mlua_free_surface(lua_State* L)
{
	assert(L);

	DEBUG("");
	Engine &engine = get_engine();
	Surface* surface = pop_surface(L, 1);
	engine.display.free_surface(surface);
	return 0;
}

static int mlua_draw_on(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	Surface* old = engine.display.get_draw_on();
	Surface* surface = pop_surface(L, 1);
	engine.display.draw_on(surface);

	if (old) {
		push_surface(L, old);
		return 1;
	}
	return 0;
}
int(*mlua_draw_on_surface)(lua_State* L) = mlua_draw_on;

static int mlua_draw_from(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	Surface* old = engine.display.get_draw_from();
	Surface* surface = pop_surface(L, 1);
	engine.display.draw_from(surface);

	if (old) {
		push_surface(L, old);
		return 1;
	}
	return 0;
}
int(*mlua_draw_from_surface)(lua_State* L) = mlua_draw_from;

static int mlua_set_filter_surface(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	Surface* surface = pop_surface(L, 1);
	FilterMode mode = static_cast<FilterMode>(luaL_checknumber(L, 2));
	engine.display.set_filter(surface, mode);
	return 0;
}

static int mlua_draw_background(lua_State*)
{
	Engine &engine = get_engine();
	engine.display.draw_background();
	return 0;
}

static int mlua_draw_point(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	lua_Number x = luaL_checknumber(L, 1);
	lua_Number y = luaL_checknumber(L, 2);
	engine.display.draw_point(x, y);
	return 0;
}

static int mlua_draw_point_tex(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	lua_Number xi = luaL_checknumber(L, 1);
	lua_Number yi = luaL_checknumber(L, 2);
	lua_Number xd = luaL_checknumber(L, 3);
	lua_Number yd = luaL_checknumber(L, 4);
	engine.display.draw_point_tex(xi, yi, xd, yd);
	return 0;
}

static int mlua_draw_line(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	lua_Number x1 = luaL_checknumber(L, 1);
	lua_Number y1 = luaL_checknumber(L, 2);
	lua_Number x2 = luaL_checknumber(L, 3);
	lua_Number y2 = luaL_checknumber(L, 4);
	engine.display.draw_line(x1, y1, x2, y2);
	return 0;
}

static int mlua_draw_triangle(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	lua_Number x1 = luaL_checknumber(L, 1);
	lua_Number y1 = luaL_checknumber(L, 2);
	lua_Number x2 = luaL_checknumber(L, 3);
	lua_Number y2 = luaL_checknumber(L, 4);
	lua_Number x3 = luaL_checknumber(L, 5);
	lua_Number y3 = luaL_checknumber(L, 6);
	engine.display.draw_triangle(x1, y1, x2, y2, x3, y3);
	return 0;
}

static int mlua_draw_surface(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	lua_Number i1 = luaL_checknumber(L, 1);
	lua_Number i2 = luaL_checknumber(L, 2);
	lua_Number i3 = luaL_checknumber(L, 3);
	lua_Number i4 = luaL_checknumber(L, 4);
	lua_Number i5 = luaL_checknumber(L, 5);
	lua_Number i6 = luaL_checknumber(L, 6);
	lua_Number o1 = luaL_checknumber(L, 7);
	lua_Number o2 = luaL_checknumber(L, 8);
	lua_Number o3 = luaL_checknumber(L, 9);
	lua_Number o4 = luaL_checknumber(L, 10);
	lua_Number o5 = luaL_checknumber(L, 11);
	lua_Number o6 = luaL_checknumber(L, 12);
	engine.display.draw_surface(i1, i2, i3, i4, i5, i6,
	                            o1, o2, o3, o4, o5, o6);
	return 0;
}

static int mlua_draw_quad(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	lua_Number i1 = luaL_checknumber(L, 1);
	lua_Number i2 = luaL_checknumber(L, 2);
	lua_Number i3 = luaL_checknumber(L, 3);
	lua_Number i4 = luaL_checknumber(L, 4);
	lua_Number i5 = luaL_checknumber(L, 5);
	lua_Number i6 = luaL_checknumber(L, 6);
	lua_Number i7 = luaL_checknumber(L, 7);
	lua_Number i8 = luaL_checknumber(L, 8);
	lua_Number o1 = luaL_checknumber(L, 9);
	lua_Number o2 = luaL_checknumber(L, 10);
	lua_Number o3 = luaL_checknumber(L, 11);
	lua_Number o4 = luaL_checknumber(L, 12);
	lua_Number o5 = luaL_checknumber(L, 13);
	lua_Number o6 = luaL_checknumber(L, 14);
	lua_Number o7 = luaL_checknumber(L, 15);
	lua_Number o8 = luaL_checknumber(L, 16);
	engine.display.draw_quad(i1, i2, i3, i4, i5, i6, i7, i8,
	                         o1, o2, o3, o4, o5, o6, o7, o8);
	return 0;
}

static int mlua_new_shader(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	const char *vert = NULL, *frag_color = NULL, *frag_tex = NULL;
	// strings can be nil
	if (lua_gettop(L) >= 1) { // one argument, it's the vertex shader
		vert = lua_tostring(L, 1);
	}
	if (lua_gettop(L) >= 2) {
		frag_color = lua_tostring(L, 2);
	}
	if (lua_gettop(L) >= 3) {
		frag_tex = lua_tostring(L, 3);
	}
	char* error;
	Shader* shader = engine.display.new_shader(vert, frag_color, frag_tex, &error);
	if (shader) {
		push_shader(L, shader);
		return 1;
	} else {
		lua_pushnil(L);
		lua_pushstring(L, error);
		delete[] error;
		return 2;
	}
}

static int mlua_use_shader(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	if (lua_gettop(L) == 0) { // use defaut shader
		engine.display.use_shader(NULL);
	} else {
		Shader* shader = pop_shader(L, -1);
		engine.display.use_shader(shader);
	}
	return 0;
}

static int mlua_feed_shader(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	Shader* shader = pop_shader(L, 1);
	const char* name = lua_tostring(L, 2);
	lua_Number value = luaL_checknumber(L, 3);
	engine.display.feed_shader(shader, name, value);
	return 0;
}

static int mlua_free_shader(lua_State* L)
{
	assert(L);

	DEBUG("");
	Engine &engine = get_engine();
	Shader* shader = pop_shader(L, 1);
	engine.display.free_shader(shader);
	return 0;
}

static int mlua_new_buffer(lua_State* L)
{
	Engine &engine = get_engine();
	Buffer* buffer;
	if (lua_gettop(L) == 1) {
		lua_Number size = luaL_checknumber(L, 1);
		buffer = engine.display.new_buffer(size);
	} else {
		buffer = engine.display.new_buffer(); // let Display choose a size
	}
	if (buffer) {
		push_buffer(L, buffer);
		return 1;
	}
	return 0; // returns nil
}

static int mlua_use_buffer(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	if (lua_gettop(L) == 0) { // use defaut buffer
		engine.display.use_buffer(NULL);
	} else {
		Buffer* buffer = pop_buffer(L, 1);
		engine.display.use_buffer(buffer);
	}
	return 0;
}

static int mlua_draw_buffer(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	Buffer* buffer = pop_buffer(L, 1);
	lua_Number dx = 0, dy = 0;
	if (lua_gettop(L) >= 2)
		dx = luaL_checknumber(L, 2);
	if (lua_gettop(L) >= 3)
		dy = luaL_checknumber(L, 3);
	engine.display.draw_buffer(buffer, dx, dy);
	return 0;
}

static int mlua_reset_buffer(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	Buffer* buffer = pop_buffer(L, 1);
	engine.display.reset_buffer(buffer);
	return 0;
}

static int mlua_upload_and_free_buffer(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	Buffer* buffer = pop_buffer(L, 1);
	engine.display.upload_and_free_buffer(buffer);
	return 0;
}

static int mlua_free_buffer(lua_State* L)
{
	assert(L);

	DEBUG("");
	Engine &engine = get_engine();
	Buffer* buffer = pop_buffer(L, 1);
	engine.display.free_buffer(buffer);
	return 0;
}

static int mlua_drystal_index(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();
	const char * name = luaL_checkstring(L, 2);
	if (!strcmp(name, "screen")) {
		Surface* surf = engine.display.get_screen();
		if (surf) {
			push_surface(L, surf);
			return 1;
		}
	} else if (!strcmp(name, "current_draw_on")) {
		Surface* surf = engine.display.get_draw_on();
		if (surf) {
			push_surface(L, surf);
			return 1;
		}
	} else if (!strcmp(name, "current_draw_from")) {
		Surface* surf = engine.display.get_draw_from();
		if (surf) {
			push_surface(L, surf);
			return 1;
		}
	}
	return 0;
}

void LuaFunctions::register_modules()
{
	REGISTER_MODULE(audio, L);
	REGISTER_MODULE(event, L);
	REGISTER_MODULE(net, L);
	REGISTER_MODULE(particle, L);
	REGISTER_MODULE(physic, L);
	REGISTER_MODULE(storage, L);
	REGISTER_MODULE(truetype, L);
	REGISTER_MODULE(web, L);
}

extern "C" {
	extern int json_encode(lua_State* L);
	extern int json_decode(lua_State* L);
	extern void lua_cjson_init();
}

int luaopen_drystal(lua_State* L)
{
	assert(L);

	Engine &engine = get_engine();

	lua_newtable(L);
	luaL_newmetatable(L, "__objects");
	lua_pushstring(L, "v");
	lua_setfield(L, -2, "__mode");
	lua_setmetatable(L, -2);
	lua_setfield(L, LUA_REGISTRYINDEX, "objects");

#define EXPOSE_FUNCTION(name) {#name, mlua_##name}
	static const luaL_Reg lib[] = {
		{"engine_stop", mlua_stop},
		EXPOSE_FUNCTION(stop),
		EXPOSE_FUNCTION(reload),

		EXPOSE_FUNCTION(show_cursor),

		EXPOSE_FUNCTION(resize),
		EXPOSE_FUNCTION(set_title),
		EXPOSE_FUNCTION(screen2scene),

		/* DISPLAY SURFACE */
		EXPOSE_FUNCTION(load_surface),
		EXPOSE_FUNCTION(new_surface),
		EXPOSE_FUNCTION(draw_on),
		EXPOSE_FUNCTION(draw_from),

		/* DISPLAY DRAWERS */
		EXPOSE_FUNCTION(draw_background),
		EXPOSE_FUNCTION(draw_point),
		EXPOSE_FUNCTION(draw_point_tex),
		EXPOSE_FUNCTION(draw_line),
		EXPOSE_FUNCTION(draw_triangle),
		EXPOSE_FUNCTION(draw_surface),
		EXPOSE_FUNCTION(draw_quad),

		/* DISPLAY SETTERS */
		EXPOSE_FUNCTION(set_color),
		EXPOSE_FUNCTION(set_alpha),
		EXPOSE_FUNCTION(set_point_size),
		EXPOSE_FUNCTION(set_line_width),
		EXPOSE_FUNCTION(set_blend_mode),

		/* DISPLAY SHADER */
		EXPOSE_FUNCTION(new_shader),
		EXPOSE_FUNCTION(use_shader),

		/* DISPLAY BUFFER */
		EXPOSE_FUNCTION(new_buffer),
		EXPOSE_FUNCTION(use_buffer),

		/* SERIALIZER */
		{"serialize", json_encode},
		{"deserialize", json_decode},

		{NULL, NULL}
	};
#undef EXPOSE_FUNCTION

	luaL_newlib(L, lib);

	luaL_newmetatable(L, "_drystal");
	lua_pushcfunction(L, mlua_drystal_index);
	lua_setfield(L, -2, "__index");
	lua_setmetatable(L, -2);

	BEGIN_CLASS(surface)
	ADD_METHOD(surface, draw_on)
	ADD_METHOD(surface, draw_from)
	ADD_METHOD(surface, set_filter)
	ADD_GC(free_surface)
	REGISTER_CLASS_WITH_INDEX(surface, "__Surface")

	BEGIN_CLASS(buffer)
	ADD_METHOD(buffer, use)
	ADD_METHOD(buffer, draw)
	ADD_METHOD(buffer, reset)
	ADD_METHOD(buffer, upload_and_free)
	ADD_GC(free_buffer)
	REGISTER_CLASS(buffer, "__Buffer")

	BEGIN_CLASS(shader)
	ADD_METHOD(shader, use)
	ADD_METHOD(shader, feed)
	ADD_GC(free_shader)
	REGISTER_CLASS(shader, "__Shader")

	{
		// make sure we don't free the screen until the next resize
		push_surface(L, engine.display.get_screen());
		lua_setfield(L, LUA_REGISTRYINDEX, "screen");
	}
	{
		// blend modes
		lua_pushnumber(L, DEFAULT);
		lua_setfield(L, -2, "BLEND_DEFAULT");
		lua_pushnumber(L, ALPHA);
		lua_setfield(L, -2, "BLEND_ALPHA");
		lua_pushnumber(L, ADD);
		lua_setfield(L, -2, "BLEND_ADD");
		lua_pushnumber(L, MULT);
		lua_setfield(L, -2, "BLEND_MULT");
	}
	{
		// filter modes
		lua_pushnumber(L, NEAREST);
		lua_setfield(L, -2, "NEAREST");
		lua_pushnumber(L, LINEAR);
		lua_setfield(L, -2, "LINEAR");
		lua_pushnumber(L, BILINEAR);
		lua_setfield(L, -2, "BILINEAR");
		lua_pushnumber(L, TRILINEAR);
		lua_setfield(L, -2, "TRILINEAR");
	}

	{
		// camera
		lua_newtable(L);
		luaL_newmetatable(L, "__camera_class");
		lua_pushcfunction(L, mlua_camera__newindex);
		lua_setfield(L, -2, "__newindex");
		lua_pushcfunction(L, mlua_camera__index);
		lua_setfield(L, -2, "__index");
		lua_setmetatable(L, -2);
		lua_pushcfunction(L, mlua_camera_reset);
		lua_setfield(L, -2, "reset");

		// glue it on drystal table
		lua_setfield(L, -2, "camera");
	}

	lua_pushvalue(L, -1);
	engine.lua.drystal_table_ref = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_cjson_init();

	assert(lua_gettop(L) == 2);
	return 1;
}
