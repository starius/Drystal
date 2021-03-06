local drystal = require 'drystal'

local _draw_quad = drystal.draw_quad

function drystal.draw_rect(x, y, w, h)
	drystal.draw_triangle(x, y, x, y+h, x+w, y)
	drystal.draw_triangle(x+w, y, x+w, y+h, x, y+h)
end

function drystal.draw_image(x, y, w, h, dx, dy, dw, dh)
	dw = dw or w
	dh = dh or h
	_draw_quad(x, y,  x+w,   y,  x+w,   y+h,   x,  y+h,
				dx, dy, dx+dw, dy, dx+dw, dy+dh, dx, dy+dh)
end

local notransform = {
	angle=0, -- in radians
	wfactor=1, hfactor=1 -- can be negative to flip
}
function drystal.draw_sprite_simple(sprite, x, y)
	local w = sprite.w
	local h = sprite.h
	local x1, y1 = x, y
	local x2, y2 = x+w, y+h

	local xi = sprite.x
	local yi = sprite.y
	local xi2 = sprite.x + sprite.w
	local yi2 = sprite.y + sprite.h

	_draw_quad(xi, yi, xi2, yi, xi2, yi2, xi, yi2,
				  x1, y1, x2,  y1, x2,  y2,  x1, y2)
end
function drystal.draw_sprite(sprite, x, y, transform, hx, hy)
	if not transform then
		drystal.draw_sprite_simple(sprite, x, y)
		return
	end

	local w = sprite.w * math.abs(transform.wfactor)
	local h = sprite.h * math.abs(transform.hfactor)
	if not hx then hx = w / 2 end
	if not hy then hy = h / 2 end
	local centerx, centery = x + hx, y + hy
	local angle = transform.angle
	local cos = math.cos(angle)
	local sin = math.sin(angle)
	local function rotate(_x, _y)
		return _x * cos - _y * sin,
			   _x * sin + _y * cos
	end
	local function translate(_x, _y)
		return centerx + _x, centery + _y
	end

	local x1, y1 = translate(rotate(x - centerx, y - centery))
	local x2, y2 = translate(rotate(x + w - centerx, y - centery))
	local x3, y3 = translate(rotate(x + w - centerx, y + h - centery))
	local x4, y4 = translate(rotate(x - centerx, y + h - centery))

	local xi = sprite.x
	local yi = sprite.y
	local xi2 = sprite.x + sprite.w
	local yi2 = sprite.y + sprite.h

	if transform.wfactor < 0 then
		xi, xi2 = xi2, xi
	end
	if transform.hfactor < 0 then
		yi, yi2 = yi2, yi
	end

	_draw_quad(xi, yi, xi2, yi, xi2, yi2, xi, yi2,
				  x1, y1, x2,  y2, x3,  y3,  x4, y4)
end
function drystal.draw_sprite_rotated(sprite, x, y, angle, hx, hy)
	local transform = {
		angle=angle,
		wfactor=1, hfactor=1
	}
	drystal.draw_sprite(sprite, x, y, transform, hx, hy)
end
function drystal.draw_sprite_resized(sprite, x, y, w, h)
	drystal.draw_image(sprite.x, sprite.y, sprite.w, sprite.h, x, y, w, h)
end

function drystal.draw_circle(cx, cy, r)
	-- http://slabode.exofire.net/circle_draw.shtml

	local num_segments = math.max(10, math.floor(10*math.sqrt(r)))
	local theta = 2 * math.pi / num_segments
	local c = math.cos(theta) -- precalculate the sine and cosine
	local s = math.sin(theta)
	local t

	local x = r -- we start at angle = 0
	local y = 0

	local oldx, oldy
	local nx, ny
	for ii = 0, num_segments do
		oldx = nx
		oldy = ny

		-- apply the rotation matrix
		t = x;
		x = c * x - s * y;
		y = s * t + c * y;

		nx = x + cx
		ny = y + cy

		if ii > 0 then
			drystal.draw_triangle(cx, cy, nx, ny, oldx, oldy)
		end
	end
end

function drystal.draw_polygon(...)
	local coords = {...}
	local cx, cy = 0, 0
	for i = 1, #coords, 2 do
		cx = cx + coords[i]
		cy = cy + coords[i + 1]
	end
	cx = cx / (#coords / 2)
	cy = cy / (#coords / 2)
	for i = 1, #coords - 2, 2 do
		drystal.draw_triangle(coords[i], coords[i+1], coords[i+2], coords[i+3], cx, cy)
	end
	drystal.draw_triangle(coords[#coords - 1], coords[#coords], coords[1], coords[2], cx, cy)
end

function drystal.draw_polyline(loop, width, ...)
	local coords = {...}
	if #coords % 2 == 1 then
		table.insert(coords, 1, width)
		width = 1
	end
	for i = 1, #coords - 2, 2 do
		drystal.draw_line(coords[i], coords[i+1], coords[i+2], coords[i+3], width)
	end
	if loop then
		drystal.draw_line(coords[1], coords[2], coords[#coords-1], coords[#coords], width)
	end
end

function drystal.draw_square(x, y, w, h, width)
	width = width or 1
	drystal.draw_line(x, y, x+w, y, width)
	drystal.draw_line(x, y+h, x+w, y+h, width)
	drystal.draw_line(x, y, x, y+h, width)
	drystal.draw_line(x+w, y, x+w, y+h, width)
end

function drystal.draw_rect_rotated(x, y, w, h, angle, hx, hy)
	if not hx then hx = w / 2 end
	if not hy then hy = h / 2 end
	local centerx, centery = x + hx, y + hy

	local cos = math.cos(angle)
	local sin = math.sin(angle)

	local function rotate(_x, _y)
		return _x * cos - _y * sin,
			   _x * sin + _y * cos
	end
	local function translate(_x, _y)
		return centerx + _x, centery + _y
	end

	local x1, y1 = translate(rotate(x - centerx, y - centery))
	local x2, y2 = translate(rotate(x + w - centerx, y - centery))
	local x3, y3 = translate(rotate(x + w - centerx, y + h - centery))
	local x4, y4 = translate(rotate(x - centerx, y + h - centery))

	drystal.draw_triangle(x1, y1, x2, y2, x3, y3)
	drystal.draw_triangle(x1, y1, x4, y4, x3, y3)
end

