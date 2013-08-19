print = require 'colorprint'
require 'drystal'
local physic = require 'physic'

local ground, ground2, ball
local joint
local mouse_joint

local R = 64 -- _ pixels = 1 meter

local function create_box(w, h, args, dynamic)
	local shape = physic.new_shape("box", w, h)
	for k, v in pairs(args) do
		shape['set_' .. k](shape, v)
	end
	local box = physic.new_body(shape, dynamic)
	box.width = w
	box.height = h
	function box:draw()
		local x, y = self:get_position()
		x = x * R
		y = y * R
		draw_rect_rotated(x - (w / 2) * R,
						y - (h / 2) * R,
						w * R, h * R,
						self:get_angle())
	end
	return box
end
local function create_circle(radius, args, dynamic)
	local shape = physic.new_shape("circle", radius)
	for k, v in pairs(args) do
		shape['set_' .. k](shape, v)
	end
	local circle = physic.new_body(shape, dynamic)
	circle.radius = radius
	function circle:draw()
		local angle = self:get_angle()
		x, y = self:get_position()
		draw_circle(x * R, y * R, self.radius * R)
		set_color(150, 150, 150)
		draw_line(x*R, y*R, x*R + self.radius*math.cos(angle)*R,
							y*R + ball.radius * math.sin(angle)*R)
	end
	return circle
end

function init()
	resize(600, 400)

	physic.create_world(0, 0.98)

	-- create ground
	ground = create_box(5, .2, {friction=5}, false)
	ground:set_position(5, 4.5)

	ground2 = create_box(2, .1, {}, false)
	ground2:set_position(3.5, 2.5)
	ground2:set_angle(math.pi/12)

	-- create ball
	ball = create_circle(0.2, {restitution=0.4}, true)
	ball:set_position(3.5, 0)

	ball2 = create_circle(0.2, {restitution=0.4}, true)
	ball2:set_position(4, 0)

	joint = physic.new_joint('distance', ball2, ball)
	joint:set_length(100/R)
	joint:set_frequency(0.9)

--	joint = physic.new_joint('rope', ball2, ball, true)
--	joint:set_max_length(100/R)
end

local dir = ''
local time = 0
function update(dt)
	delta = dt / 1000
	physic.update(delta * 2)
	time = time + delta

	if dir == 'left' then
		ball:set_angular_velocity(-6)
	elseif dir == 'right' then
		ball:set_angular_velocity(6)
	else
		ball:set_angular_velocity(0)
	end
end

function draw()
	set_color(120, 120, 120)
	draw_background()

	set_color(0, 0, 0)

	ground:draw()
	set_color(math.sin(time)*120, 120, 102)
	ground2:draw()
	set_color(0, 0, 0)

	ball:draw()
	ball2:draw()

	local x1, y1 = ball:get_position()
	local x2, y2 = ball2:get_position()
	draw_line(x1*R, y1*R, x2*R, y2*R)

	flip()
end

function key_press(key)
	if key == 'space' then
		ball:apply_linear_impulse(0, -0.3)
	elseif key == 'q' then
		ball:apply_angular_impulse(0.02)
	elseif key == 'left' then
		dir = key
	elseif key == 'right' then
		dir = key
	end
end
function key_release(key)
	if key == 'left' then
		dir = ''
	elseif key == 'right' then
		dir = ''
	end
end

function mouse_release(x, y, b)
	if b == 1 then
		mouse_joint:destroy()
		mouse_joint = nil
	end
end
function mouse_motion(x, y)
	if mouse_joint then
		mouse_joint:set_target(x/R, y/R)
	end
end
function mouse_press(x, y, b)
	if b == 1 then
		if not mouse_joint then
			mouse_joint = physic.new_joint('mouse', ground, ball, 7*ball:get_mass(), true)
		end
		mouse_joint:set_target(x/R, y/R)
	end
	if b == 3 then
		ball:set_position(x/R, y/R)
		ball:set_angular_velocity(0)
		ball:set_linear_velocity(0, 0)
	end
end