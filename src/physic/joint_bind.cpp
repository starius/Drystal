#include <cassert>
#include <cstring>
#include <lua.hpp>

#include "macro.hpp"

BEGIN_DISABLE_WARNINGS;
DISABLE_WARNING_EFFCPP;
DISABLE_WARNING_STRICT_ALIASING;
#include <Box2D/Box2D.h>
END_DISABLE_WARNINGS;

#include "engine.hpp"
#include "joint_bind.hpp"

DECLARE_POP(RevoluteJoint, revolute_joint)
DECLARE_POP(MouseJoint, mouse_joint)
DECLARE_POP(DistanceJoint, distance_joint)
DECLARE_POP(RopeJoint, rope_joint)
DECLARE_POP(PrismaticJoint, prismatic_joint)

inline static b2MouseJoint* luam_tomousejoint(lua_State* L, int index)
{
	return (b2MouseJoint *) pop_mouse_joint(L, index)->joint;
}

inline static b2DistanceJoint* luam_todistancejoint(lua_State* L, int index)
{
	return (b2DistanceJoint *) pop_distance_joint(L, index)->joint;
}

inline static b2RopeJoint* luam_toropejoint(lua_State* L, int index)
{
	return (b2RopeJoint *) pop_rope_joint(L, index)->joint;
}

inline static b2RevoluteJoint* luam_torevolutejoint(lua_State* L, int index)
{
	return (b2RevoluteJoint *) pop_revolute_joint(L, index)->joint;
}

inline static b2PrismaticJoint* luam_toprismaticjoint(lua_State* L, int index)
{
	return (b2PrismaticJoint *) pop_prismatic_joint(L, index)->joint;
}

int mlua_set_target_mouse_joint(lua_State* L)
{
	assert(L);

	b2MouseJoint* joint = luam_tomousejoint(L, 1);
	lua_Number x = luaL_checknumber(L, 2);
	lua_Number y = luaL_checknumber(L, 3);
	joint->SetTarget(b2Vec2(x, y));
	return 0;
}

int mlua_set_length_distance_joint(lua_State* L)
{
	assert(L);

	b2DistanceJoint* joint = luam_todistancejoint(L, 1);
	lua_Number length = luaL_checknumber(L, 2);
	joint->SetLength(length);
	return 0;
}

int mlua_set_frequency_distance_joint(lua_State* L)
{
	assert(L);

	b2DistanceJoint* joint = luam_todistancejoint(L, 1);
	lua_Number freq = luaL_checknumber(L, 2);
	joint->SetFrequency(freq);
	return 0;
}

int mlua_set_max_length_rope_joint(lua_State* L)
{
	assert(L);

	b2RopeJoint* joint = luam_toropejoint(L, 1);
	lua_Number maxlength = luaL_checknumber(L, 2);
	joint->SetMaxLength(maxlength);
	return 0;
}

int mlua_set_angle_limits_revolute_joint(lua_State* L)
{
	assert(L);

	b2RevoluteJoint* joint = luam_torevolutejoint(L, 1);
	lua_Number min = luaL_checknumber(L, 2);
	lua_Number max = luaL_checknumber(L, 3);
	if (min != max) {
		joint->SetLimits(min, max);
		joint->EnableLimit(true);
	} else {
		joint->EnableLimit(false);
	}
	return 0;
}

int mlua_set_motor_speed_revolute_joint(lua_State* L)
{
	assert(L);

	b2RevoluteJoint* joint = luam_torevolutejoint(L, 1);
	lua_Number speed = luaL_checknumber(L, 2);
	lua_Number maxtorque = 20;
	if (lua_gettop(L) > 3)
		maxtorque = luaL_checknumber(L, 3);
	if (speed != 0) {
		joint->SetMotorSpeed(speed);
		joint->SetMaxMotorTorque(maxtorque);
		joint->EnableMotor(true);
	} else {
		joint->EnableMotor(false);
	}
	return 0;
}

int mlua_set_enable_motor_prismatic_joint(lua_State* L)
{
	b2PrismaticJoint* joint = luam_toprismaticjoint(L, 1);
	bool enable = lua_toboolean(L, 2);
	joint->EnableMotor(enable);

	return 0;
}

int mlua_set_motor_speed_prismatic_joint(lua_State* L)
{
	b2PrismaticJoint* joint = luam_toprismaticjoint(L, 1);
	lua_Number speed = luaL_checknumber(L, 2);
	joint->SetMotorSpeed(speed);

	return 0;
}

int mlua_set_enable_limit_prismatic_joint(lua_State* L)
{
	b2PrismaticJoint* joint = luam_toprismaticjoint(L, 1);
	bool enable = lua_toboolean(L, 2);
	joint->EnableLimit(enable);

	return 0;
}

int mlua_set_max_motor_force_prismatic_joint(lua_State* L)
{
	b2PrismaticJoint* joint = luam_toprismaticjoint(L, 1);
	lua_Number force = luaL_checknumber(L, 2);
	joint->SetMaxMotorForce(force);

	return 0;
}

int mlua_is_motor_enabled_prismatic_joint(lua_State* L)
{
	b2PrismaticJoint* joint = luam_toprismaticjoint(L, 1);
	lua_pushboolean(L, joint->IsMotorEnabled());

	return 1;
}

int mlua_is_limit_enabled_prismatic_joint(lua_State* L)
{
	b2PrismaticJoint* joint = luam_toprismaticjoint(L, 1);
	lua_pushboolean(L, joint->IsLimitEnabled());

	return 1;
}

