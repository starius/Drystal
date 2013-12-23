local drystal = require 'drystal'
local web = require 'web'

local sounds = {
	'jump1.wav',
	'jump2.wav',
	'hurt1.wav',
	'coin1.wav',
	'coin2.wav',
}

local loaded = {}
for i, s in pairs(sounds) do
	if not drystal.file_exists(s) then
		web.wget('gamedata/' .. s, s, function ()
			sound = drystal.load_sound(s)
			loaded[i] = sound
		end, function() print('can\'t download sound', s) end)
	else
		sound = drystal.load_sound(s)
		print(sound)
		loaded[i] = sound
	end
end

function drystal.init()
	drystal.resize(400, 400)
	drystal.set_sound_volume(0.1)
end

function drystal.mouse_motion()
	drystal.play_sound(loaded[math.random(#loaded)])
end

function quick_play(str)
	local sound = drystal.load_sound(str)
	drystal.play_sound(sound)
end

function drystal.key_press(k)
	if k == 't' then
		quick_play(sounds[math.random(#loaded)])
	elseif k == 'escape' or k == 'a' then
		drystal.stop()
	end
end
