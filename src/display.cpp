#ifndef EMSCRIPTEN
#include <GLES2/gl2.h>
#endif

#include <SDL/SDL.h>

#include <iostream>
#include <cassert>
#include <cmath>

extern "C" {
#define STB_NO_HDR
#include "stb_image.c"
}

#ifdef EMSCRIPTEN
#include "emscripten.h" // see show_cursor
#endif

#include "log.hpp"
#include "display.hpp"

const char* DEFAULT_VERTEX_SHADER = R"(
#ifdef GL_ES
precision mediump float;
#endif

attribute vec2 position;	// position of the vertice
attribute vec4 color;		// color of the vertice
attribute vec2 texCoord;	// texture coordinates
attribute float pointSize;	// size of points

varying vec4 fColor;
varying vec2 fTexCoord;

uniform float dx;
uniform float dy;
uniform float zoom;
uniform mat2 rotationMatrix;
mat2 cameraMatrix = rotationMatrix * zoom;

void main()
{
	gl_PointSize = pointSize * zoom;
	vec2 position2d = cameraMatrix  * (position - vec2(dx, dy));
	gl_Position = vec4(position2d, 0.0, 1.0);
	fColor = color;
	fTexCoord = texCoord;
}
)";

const char* DEFAULT_FRAGMENT_SHADER_COLOR = R"(
#ifdef GL_ES
precision mediump float;
#endif

varying vec4 fColor;
varying vec2 fTexCoord;

void main()
{
	gl_FragColor = fColor;
}
)";

const char* DEFAULT_FRAGMENT_SHADER_TEX = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D tex;

varying vec4 fColor;
varying vec2 fTexCoord;

void main()
{
	vec4 color;
	vec4 texval = texture2D(tex, fTexCoord);
	color.rgb = mix(texval.rgb, fColor.rgb, vec3(1.)-fColor.rgb);
	color.a = texval.a * fColor.a;
	gl_FragColor = color;
}
)";

Display::Display()
	: resizable(false),
	  sdl_screen(NULL),
	  screen(NULL),
	  default_shader(NULL),
	  current(NULL),
	  current_from(NULL),
	  current_buffer(&default_buffer),
	  r(1),
	  g(1),
	  b(1),
	  alpha(1),
	  available(false)
{
	int err = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	if (err)
	{
		available = false;
		fprintf(stderr, "[ERROR] cannot initialize SDL\n");
		return;
	}

	default_buffer.use_camera(&camera);

	available = true;
}

Display::~Display()
{
	if (sdl_screen) {
		SDL_FreeSurface(sdl_screen);
		sdl_screen = nullptr;
	}
	if (screen) {
		delete screen;
		screen = nullptr;
	}
	if (default_shader) {
		free_shader(default_shader);
		default_shader = nullptr;
	}
}

bool Display::is_available() const
{
	return available;
}

/**
 * Screen
 */

void Display::set_resizable(bool b)
{
	if (b != resizable) {
		resizable = b;
		if (screen)
			resize(size_x, size_y);
	}
}

void Display::resize(int w, int h)
{
	w = w > 0 ? w : 1;
	h = h > 0 ? h : 1;
	DEBUG("");
	Surface* old = screen;
	size_x = w;
	size_y = h;
	sdl_screen = SDL_SetVideoMode(size_x, size_y, 32,
			SDL_OPENGL| (resizable ? SDL_VIDEORESIZE : 0));
#ifndef EMSCRIPTEN
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
#endif
	assert(sdl_screen);

	if (screen)
		delete screen;
	screen = new Surface;
	screen->w = sdl_screen->w;
	screen->h = sdl_screen->h;
	screen->texw = sdl_screen->w;
	screen->texh = sdl_screen->h;
	screen->fbo = 0; // back buffer

	if (current == old) {
		current = nullptr; // force update
		draw_on(screen);
	}

	// regenerate shader (lost with the GL context)
	if (default_shader)
		free_shader(default_shader);
	default_shader = create_default_shader();
	use_shader(default_shader);

	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

#ifdef EMSCRIPTEN
	//glEnable(0x8642);
#else
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
//	glEnable(GL_POINT_SMOOTH);
//	glEnable(GL_POINT_SPRITE);

	default_buffer.reallocate();
	// TODO: handle other buffers
	// TODO: invalidate surfaces
	DEBUG("end");
}

void Display::screen2scene(float x, float y, float * tx, float * ty)
{
	float z = camera.zoom;
	x -= camera.dx;
	y -= camera.dy;
	*tx = (camera.matrix[0] * x + camera.matrix[2] * y) / z;
	*ty = (camera.matrix[1] * x + camera.matrix[3] * y) / z;
}

void Display::show_cursor(bool b)
{
#ifndef EMSCRIPTEN
	SDL_ShowCursor(b);
#else
	if (b) {
		emscripten_run_script("Module['canvas'].style.cursor='default';");
	} else {
		emscripten_run_script("Module['canvas'].style.cursor='none';");
	}
#endif
}

void Display::draw_background() const
{
	glClearColor(r, g, b, alpha);
	glClear(GL_COLOR_BUFFER_BIT);
}

void Display::flip()
{
	DEBUG("");
	default_buffer.assert_empty();
	glBindFramebuffer(GL_FRAMEBUFFER, screen->fbo);
	glFlush();
	SDL_GL_SwapBuffers();
	glBindFramebuffer(GL_FRAMEBUFFER, current->fbo);
	DEBUG("end");
}

Surface* Display::get_screen() const
{
	return screen;
}

/**
 * State
 */

void Display::set_color(int r, int g, int b)
{
	this->r = r / 255.;
	this->g = g / 255.;
	this->b = b / 255.;
}

void Display::set_alpha(int a)
{
	this->alpha = a / 255.;
}

void Display::set_point_size(float size)
{
	this->point_size = size;
}

void Display::set_line_width(float width)
{
	glLineWidth(width);
}

void Display::set_blend_mode(BlendMode mode)
{
	current_buffer->assert_empty();

	switch (mode) {
		case ALPHA:
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			glBlendEquation(GL_FUNC_ADD);
			break;
		case MULT:
			glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
			glBlendEquation(GL_FUNC_ADD);
			break;
		case ADD:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glBlendEquation(GL_FUNC_ADD);
			break;
		case EXPERIMENTAL:
			break;
	}
}
void Display::set_filter_mode(FilterMode mode)
{
	filter_mode = mode;
}

void Display::reset_camera()
{
	current_buffer->assert_empty();

	camera.dx = camera.dy = 0;
	camera.dx_transformed = camera.dy_transformed = 0;
	camera.angle = 0;
	camera.zoom = 1;
	update_camera_matrix();
}

void Display::set_camera_position(float dx, float dy)
{
	current_buffer->assert_empty();

	camera.dx = dx;
	camera.dy = dy;

	update_camera_matrix();
}

void Display::set_camera_angle(float angle)
{
	current_buffer->assert_empty();

	camera.angle = angle;

	update_camera_matrix();
}

void Display::set_camera_zoom(float zoom)
{
	current_buffer->assert_empty();

	camera.zoom = zoom;
}

void Display::update_camera_matrix()
{
	assert(current);

	float angle = camera.angle;

	float ratio = (float) current->w / current->h;
	camera.matrix[0] = cos(angle);
	camera.matrix[1] = sin(angle) * ratio;
	camera.matrix[2] = -sin(angle) / ratio;
	camera.matrix[3] = cos(angle);

	float ddx = 2 * (- camera.dx / current->w);
	float ddy = 2 * (- camera.dy / current->h);
	if (current == screen)
		ddy *= -1.0;
	camera.dx_transformed = ddx;
	camera.dy_transformed = ddy;
}

void Display::draw_from(const Surface* surf)
{
	DEBUG("");
	assert(surf);
	if (current_from != surf) {
		default_buffer.assert_empty();
		this->current_from = surf;
		glBindTexture(GL_TEXTURE_2D, current_from->tex);
	}
}

void Display::draw_on(const Surface* surf)
{
	DEBUG("");
	assert(surf);
	if (current != surf) {
		default_buffer.assert_empty();
		this->current = surf;
		glBindFramebuffer(GL_FRAMEBUFFER, current->fbo);

		int w = surf->w;
		int h = surf->h;
		glViewport(0, 0, w, h);
		update_camera_matrix();
	}
}

/**
 * Surface
 */

Surface* Display::create_surface(int w, int h, int texw, int texh, unsigned char* pixels) const
{
	// gen texture
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texw, texh, 0,
				 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	GLDEBUG();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_mode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_mode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// gen framebuffer object
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
	GLDEBUG();

	GLenum status;
	status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	assert(status == GL_FRAMEBUFFER_COMPLETE);

	Surface* surface = new Surface;
	surface->tex = tex;
	surface->w = w;
	surface->h = h;
	surface->texw = texw;
	surface->texh = texh;
	surface->fbo = fbo;

	glBindFramebuffer(GL_FRAMEBUFFER, current->fbo);
	glBindTexture(GL_TEXTURE_2D, current_from ? current_from->tex : 0);
	DEBUG("end");
	return surface;
}

#define RGBA_SIZE 4
Surface* Display::load_surface(const char * filename) const
{
	assert(filename);
	int w, h;
	int n;
	unsigned char *data = stbi_load(filename, &w, &h, &n, RGBA_SIZE);

	if (not data)
		return nullptr;

	int potw = pow(2, ceil(log(w)/log(2)));
	int poth = pow(2, ceil(log(h)/log(2)));

	Surface* surface;

	if (potw != w or poth != h) {
		unsigned char *pixels = new unsigned char[potw * poth * RGBA_SIZE];
		memset(pixels, 0, potw * poth * RGBA_SIZE);
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				int is = (x + y * w) * RGBA_SIZE;
				int id = (x + y * potw) * RGBA_SIZE;
				memcpy(pixels + id, data + is, RGBA_SIZE);
			}
		}
		surface = create_surface(w, h, potw, poth, pixels);
		delete[] pixels;
	} else {
		surface = create_surface(w, h, w, h, data);
	}

	stbi_image_free(data);

	return surface;
}

Surface* Display::new_surface(int w, int h) const
{
	assert(w > 0);
	assert(h > 0);
	int potw = pow(2, ceil(log(w)/log(2)));
	int poth = pow(2, ceil(log(h)/log(2)));

	unsigned char *pixels = new unsigned char[potw * poth * RGBA_SIZE];
	memset(pixels, 0, potw * poth * RGBA_SIZE);

	Surface *surface = create_surface(w, h, potw, poth, pixels);
	delete[] pixels;
	return surface;
}

void Display::free_surface(Surface* surface)
{
	assert(surface);
	if (surface == current_from) {
		default_buffer.assert_not_use_texture();
		glBindTexture(GL_TEXTURE_2D, 0);
		current_from = NULL;
	}
	if (surface == current) {
		glBindFramebuffer(GL_FRAMEBUFFER, screen->fbo);
		current = NULL;
	}
	glDeleteTextures(1, &surface->tex);
	glDeleteFramebuffers(1, &surface->fbo);
	delete surface;
}

void Display::surface_size(Surface* surface, int *w, int *h)
{
	assert(surface);
	assert(w);
	assert(h);
	*w = surface->w;
	*h = surface->h;
}

/**
 * Primitive drawing
 */

void Display::draw_point(float x, float y)
{
	DEBUG("");
	float xx, yy;
	convert_coords(x, y, &xx, &yy);

	current_buffer->assert_type(POINT_BUFFER);
	current_buffer->assert_not_use_texture();

	current_buffer->push_vertex(xx, yy);
	current_buffer->push_point_size(point_size);
	current_buffer->push_color(r, g, b, alpha);
}
void Display::draw_point_tex(float xi, float yi, float xd, float yd)
{
	DEBUG("");
	float xxd, yyd;
	convert_coords(xd, yd, &xxd, &yyd);

	float xxi, yyi;
	convert_texcoords(xi, yi, &xxi, &yyi);

	current_buffer->assert_type(POINT_BUFFER);
	current_buffer->assert_use_texture();

	current_buffer->push_vertex(xxd, yyd);
	current_buffer->push_tex_coord(xxi, yyi);
	current_buffer->push_point_size(point_size);
	current_buffer->push_color(r, g, b, alpha);
}

void Display::draw_line(float x1, float y1, float x2, float y2)
{
	DEBUG("");
	// glLineWidth(2);
	float xx1, xx2;
	float yy1, yy2;
	convert_coords(x1, y1, &xx1, &yy1);
	convert_coords(x2, y2, &xx2, &yy2);

	current_buffer->assert_type(LINE_BUFFER);
	current_buffer->assert_not_use_texture();
	current_buffer->push_vertex(xx1, yy1);
	current_buffer->push_vertex(xx2, yy2);
	for (int i = 0; i < 2; i++)
		current_buffer->push_color(r, g, b, alpha);
}

void Display::draw_triangle(float x1, float y1, float x2, float y2, float x3, float y3)
{
	DEBUG("");
	float xx1, xx2, xx3;
	float yy1, yy2, yy3;
	convert_coords(x1, y1, &xx1, &yy1);
	convert_coords(x2, y2, &xx2, &yy2);
	convert_coords(x3, y3, &xx3, &yy3);

	current_buffer->assert_type(TRIANGLE_BUFFER);
	current_buffer->assert_not_use_texture();
	current_buffer->push_vertex(xx1, yy1);
	current_buffer->push_vertex(xx2, yy2);
	current_buffer->push_vertex(xx3, yy3);
	for (int i = 0; i < 3; i++)
		current_buffer->push_color(r, g, b, alpha);
}

void Display::draw_surface(float xi1, float yi1, float xi2, float yi2, float xi3, float yi3,
		float xo1, float yo1, float xo2, float yo2, float xo3, float yo3)
{
	assert(current_from);
	DEBUG("");
	float xxi1, xxi2, xxi3;
	float yyi1, yyi2, yyi3;
	convert_texcoords(xi1, yi1, &xxi1, &yyi1);
	convert_texcoords(xi2, yi2, &xxi2, &yyi2);
	convert_texcoords(xi3, yi3, &xxi3, &yyi3);

	float xxo1, xxo2, xxo3;
	float yyo1, yyo2, yyo3;
	convert_coords(xo1, yo1, &xxo1, &yyo1);
	convert_coords(xo2, yo2, &xxo2, &yyo2);
	convert_coords(xo3, yo3, &xxo3, &yyo3);

	current_buffer->assert_type(TRIANGLE_BUFFER);
	current_buffer->assert_use_texture();

	current_buffer->push_tex_coord(xxi1, yyi1);
	current_buffer->push_tex_coord(xxi2, yyi2);
	current_buffer->push_tex_coord(xxi3, yyi3);

	current_buffer->push_vertex(xxo1, yyo1);
	current_buffer->push_vertex(xxo2, yyo2);
	current_buffer->push_vertex(xxo3, yyo3);

	for (int i = 0; i < 3; i++)
		current_buffer->push_color(r, g, b, alpha);
}

void Display::draw_quad(float xi1, float yi1, float xi2, float yi2, float xi3, float yi3, float xi4, float yi4,
		float xo1, float yo1, float xo2, float yo2, float xo3, float yo3, float xo4, float yo4)
{
	draw_surface(xi1, yi1, xi2, yi2, xi3, yi3, xo1, yo1, xo2, yo2, xo3, yo3);
	draw_surface(xi1, yi1, xi3, yi3, xi4, yi4, xo1, yo1, xo3, yo3, xo4, yo4);
}


/**
 * Shader
 */
static void printLog(GLuint obj, const char* prefix)
{
	int infologLength = 0;
	int maxLength;

	if(glIsShader(obj))
		glGetShaderiv(obj,GL_INFO_LOG_LENGTH,&maxLength);
	else
		glGetProgramiv(obj,GL_INFO_LOG_LENGTH,&maxLength);

	char infoLog[maxLength];

	if (glIsShader(obj))
		glGetShaderInfoLog(obj, maxLength, &infologLength, infoLog);
	else
		glGetProgramInfoLog(obj, maxLength, &infologLength, infoLog);

	if (infologLength > 0)
		printf("Err %s: %s\n", prefix, infoLog);
}

Shader* Display::create_default_shader()
{
	const char* strvert = DEFAULT_VERTEX_SHADER;
	const char* strfragcolor = DEFAULT_FRAGMENT_SHADER_COLOR;
	const char* strfragtex = DEFAULT_FRAGMENT_SHADER_TEX;
	Shader* shader = new_shader(strvert, strfragcolor, strfragtex);
	assert(shader);
	return shader;
}

Shader* Display::new_shader(const char* strvert, const char* strfragcolor, const char* strfragtex)
{
	GLuint vert = 0;
	GLuint frag_color = 0;
	GLuint frag_tex = 0;
	GLuint prog_color;
	GLuint prog_tex;

	if (not strvert or not *strvert) {
		strvert = DEFAULT_VERTEX_SHADER;
	}
	if (not strfragcolor or not *strfragcolor) {
		strfragcolor = DEFAULT_FRAGMENT_SHADER_COLOR;
	}
	if (not strfragtex or not *strfragtex) {
		strfragtex = DEFAULT_FRAGMENT_SHADER_TEX;
	}

	vert = glCreateShader(GL_VERTEX_SHADER);
	assert(vert);
	glShaderSource(vert, 1, &strvert, NULL);
	glCompileShader(vert);

	frag_color = glCreateShader(GL_FRAGMENT_SHADER);
	assert(frag_color);
	glShaderSource(frag_color, 1, &strfragcolor, NULL);
	glCompileShader(frag_color);

	frag_tex = glCreateShader(GL_FRAGMENT_SHADER);
	assert(frag_tex);
	glShaderSource(frag_tex, 1, &strfragtex, NULL);
	glCompileShader(frag_tex);

	prog_color = glCreateProgram();
	assert(prog_color);
	glBindAttribLocation(prog_color, ATTR_POSITION_INDEX, "position");
	glBindAttribLocation(prog_color, ATTR_COLOR_INDEX, "color");
	glBindAttribLocation(prog_color, ATTR_TEXCOORD_INDEX, "texCoord");
	glBindAttribLocation(prog_color, ATTR_POINTSIZE_INDEX, "pointSize");
	glAttachShader(prog_color, vert);
	glAttachShader(prog_color, frag_color);
	glLinkProgram(prog_color);

	prog_tex = glCreateProgram();
	assert(prog_tex);
	glBindAttribLocation(prog_tex, ATTR_POSITION_INDEX, "position");
	glBindAttribLocation(prog_tex, ATTR_COLOR_INDEX, "color");
	glBindAttribLocation(prog_tex, ATTR_TEXCOORD_INDEX, "texCoord");
	glBindAttribLocation(prog_tex, ATTR_POINTSIZE_INDEX, "pointSize");
	glAttachShader(prog_tex, vert);
	glAttachShader(prog_tex, frag_tex);
	glLinkProgram(prog_tex);

	GLint status;
	glGetProgramiv(prog_color, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		printLog(vert, "vertex");
		printLog(frag_color, "fragment");
		printLog(prog_color, "program");
		return NULL;
	}

	glGetProgramiv(prog_tex, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		printLog(vert, "vertex");
		printLog(frag_tex, "fragment");
		printLog(prog_tex, "program");
		return NULL;
	}

	Shader* shader = new Shader;
	shader->prog_color = prog_color;
	shader->prog_tex = prog_tex;
	shader->vert = vert;
	shader->frag_color = frag_color;
	shader->frag_tex = frag_tex;
	return shader;
}

void Display::use_shader(Shader* shader)
{
	DEBUG();
	current_buffer->assert_empty();

	if (not shader) {
		shader = default_shader;
	}
	current_shader = shader;
	current_buffer->use_shader(shader);
}

void Display::feed_shader(Shader* shader, const char* name, float value)
{
	assert(shader);
	assert(name);

	GLint prog;
	glGetIntegerv(GL_CURRENT_PROGRAM, &prog);

	glUseProgram(shader->prog_color);
	GLint loc = glGetUniformLocation(shader->prog_color, name);
	if (loc >= 0)
		glUniform1f(loc, value);
	else
		printf("no location for %s\n", name);

	glUseProgram(shader->prog_tex);
	loc = glGetUniformLocation(shader->prog_tex, name);
	if (loc >= 0)
		glUniform1f(loc, value);
	else
		printf("no location for %s\n", name);

	glUseProgram(prog);
}

void Display::free_shader(Shader* shader)
{
	GLuint prog;
	glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&prog);
	if (prog == shader->prog_color)
		use_shader(default_shader);

	glDeleteShader(shader->vert);
	glDeleteShader(shader->frag_color);
	glDeleteShader(shader->frag_tex);
	glDeleteProgram(shader->prog_color);
	glDeleteProgram(shader->prog_tex);
	delete shader;
}

Buffer* Display::new_buffer(unsigned int size)
{
	Buffer* buffer = new Buffer(size);
	buffer->reallocate();
	buffer->use_camera(&camera);
	return buffer;
}
void Display::use_buffer(Buffer* buffer)
{
	if (not buffer) {
		current_buffer = &default_buffer;
	} else {
		current_buffer = buffer;
	}
	current_buffer->use_shader(current_shader);
}
void Display::draw_buffer(Buffer* buffer, float dx, float dy)
{
	dx /= current->w;
	dy /= current->h;
	current_buffer->assert_empty();
	buffer->draw(dx, dy);
}
void Display::reset_buffer(Buffer* buffer)
{
	buffer->reset();
}
void Display::upload_and_free_buffer(Buffer* buffer)
{
	buffer->upload_and_free();
}
void Display::free_buffer(Buffer* buffer)
{
	delete buffer;
}

