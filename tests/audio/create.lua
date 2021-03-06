local drystal = require 'drystal'

local w = 800
local h = 500
local x = w/2
local y = h/2
local RATE = 44100

local current = 0

function play(time, ...)
	local freqs = {...}
	print("gen...")
	local new = drystal.load_sound(function(i)
		local s = 0
		for _, f in ipairs(freqs) do
			s = s + math.sin(i*f*math.pi / RATE)
		end
		s = s / #freqs
		return s
	end, time*RATE)
	new:play(0.15)
	print("play at", ...)
end

local cursor = 0
function music_callback(data, len)
	local freq = 1800 * x / w
	local tone = freq * math.pi / RATE
	for i = 1, len do
		data[i] = math.sin(cursor * tone)
		cursor = cursor + 1
	end
	return len
end

local music
function drystal.init()
	drystal.resize(w, h)
	drystal.set_music_volume(.2)
	music = drystal.load_music(music_callback, RATE)
	music:play()
end

function drystal.mouse_motion(xx, yy)
	x = xx
	y = yy
	music:set_pitch(3.5 * y / h + 0.5)
end

function drystal.key_press(k)
	if k == 'p' then
		music:stop()
	elseif k == 'o' then
		music:play()
	elseif k == 'a' then
		drystal.stop()
	elseif k == 's' then
		local f = {}
		for i = 1, 10 do
			f[i] = 1000 + i * 10.52
		end
		play(1, unpack(f))
	end
end

