local drystal = require 'drystal'

if not sys1 then -- for livecoding
	sys1 = drystal.new_system(100, 550)
end

sys1:set_sizes {
	[0]=20,
	[.2]={7, 10},
	[1]=5,
}
sys1:set_colors {
	[0.05]=drystal.colors.red:darker(),
	[0.4]={'orange', 'yellow'},
	[0.9]='black',
}

sys1:set_lifetime(3)

sys1:set_direction(- math.pi / 2 - math.pi/12, -math.pi/2 + math.pi/12)
sys1:set_initial_velocity(100)
sys1:set_initial_acceleration(0)
sys1:set_emission_rate(100)
tex = drystal.load_surface('spritesheet.png')
sys1:set_texture(tex, 64, 0)

function drystal.init()
	drystal.resize(600, 600)
	sys1:start()
end

function drystal.update(dt)
	if dt > .06 then
		dt = .06
	end
	sys1:update(dt)
end

function drystal.draw()
	drystal.set_color(0, 0, 0)
	drystal.draw_background()

	sys1:draw()
end

function drystal.key_press(k)
	if k == 'a' then
		drystal.stop()
	end
end

