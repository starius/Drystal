#define LUA_API extern

#include <lua.hpp>

#include "box2d/Box2D/Box2D.h"

#define BODY_CLASS "__body_class"
#define SHAPE_CLASS "__shape_class"
#define JOINT_CLASS "__joint_class"

#define DECLARE_FUNCTION(x) {#x, x}
#define DECLARE_GETSET(x) DECLARE_FUNCTION(set_##x), DECLARE_FUNCTION(get_##x)

b2World* world;

int create_world(lua_State* L)
{
	lua_Number gravity_x = luaL_checknumber(L, 1);
	lua_Number gravity_y = luaL_checknumber(L, 2);
	world = new b2World(b2Vec2(gravity_x, gravity_y));
	return 0;
}

int update(lua_State* L)
{
	assert(world);
	lua_Number dt = luaL_checknumber(L, 1);

	int velocityIterations = 8;
	int positionIterations = 3;

	world->Step(dt, velocityIterations, positionIterations);
	return 0;
}

// Shape methods

int new_shape(lua_State* L)
{
	assert(world);

	const char* type = luaL_checkstring(L, 1);

	b2FixtureDef* fixtureDef = new b2FixtureDef;
	fixtureDef->density = 1.0f;

	if (!strcmp(type, "box")) {
		b2PolygonShape* polygon = new b2PolygonShape;
		lua_Number w = luaL_checknumber(L, 2) / 2;
		lua_Number h = luaL_checknumber(L, 3) / 2;
		polygon->SetAsBox(w, h);
		fixtureDef->shape = polygon;
	} else if (!strcmp(type, "circle")) {
		b2CircleShape* circle = new b2CircleShape;
		lua_Number radius = luaL_checknumber(L, 2);
		circle->m_radius = radius;
		fixtureDef->shape = circle;
	} else {
		assert(false);
		return 0;
	}

	lua_newtable(L);
	lua_pushlightuserdata(L, fixtureDef);
	lua_setfield(L, -2, "__self");
	luaL_getmetatable(L, SHAPE_CLASS);
	lua_setmetatable(L, -2);
	return 1;
}

static b2FixtureDef* luam_tofixture(lua_State* L, int index)
{
	luaL_checktype(L, index, LUA_TTABLE);
	lua_getfield(L, index, "__self");
	b2FixtureDef* shape = (b2FixtureDef*) lua_touserdata(L, -1);
	return shape;
}

#define SHAPE_GETSET_SOME_VALUE(value) \
	int set_##value(lua_State* L) \
	{ \
		b2FixtureDef* fixtureDef = luam_tofixture(L, 1); \
		lua_Number value = luaL_checknumber(L, 2); \
		fixtureDef->value = value; \
		return 0; \
	} \
	int get_##value(lua_State* L) \
	{ \
		b2FixtureDef* fixtureDef = luam_tofixture(L, 1); \
		lua_pushnumber(L, fixtureDef->value); \
		return 1; \
	}
SHAPE_GETSET_SOME_VALUE(density)
SHAPE_GETSET_SOME_VALUE(restitution)
SHAPE_GETSET_SOME_VALUE(friction)

int shape_gc(lua_State* L)
{
	b2FixtureDef* fixtureDef = luam_tofixture(L, 1);
	delete fixtureDef->shape;
	delete fixtureDef;
	return 0;
}

static const luaL_Reg __shape_class[] = {
	DECLARE_GETSET(density),
	DECLARE_GETSET(restitution),
	DECLARE_GETSET(friction),

	{"__gc", shape_gc},
	{NULL, NULL},
};


// Body methods

int new_body(lua_State* L)
{
	assert(world);

	b2FixtureDef* fixtureDef = luam_tofixture(L, 1);
	bool dynamic = lua_toboolean(L, 2);

	assert(fixtureDef);
	assert(fixtureDef->shape);

	b2BodyDef def;
	if (dynamic) {
		def.type = b2_dynamicBody;
	}

	b2Body* body = world->CreateBody(&def);
	body->CreateFixture(fixtureDef);

	lua_newtable(L);
	lua_pushlightuserdata(L, body);
	lua_setfield(L, -2, "__self");
	luaL_getmetatable(L, BODY_CLASS);
	lua_setmetatable(L, -2);
	return 1;
}

static b2Body* luam_tobody(lua_State* L, int index)
{
	luaL_checktype(L, index, LUA_TTABLE);
	lua_getfield(L, index, "__self");
	b2Body* body = (b2Body*) lua_touserdata(L, -1);
	return body;
}

#define BODY_GETSET_VEC2(value, get_expr, set_expr) \
	int set_##value(lua_State* L) \
	{ \
		b2Body* body = luam_tobody(L, 1); \
		lua_Number x = luaL_checknumber(L, 2); \
		lua_Number y = luaL_checknumber(L, 3); \
		b2Vec2 vector(x, y); \
		set_expr; \
		return 0; \
	} \
	int get_##value(lua_State* L) \
	{ \
		b2Body* body = luam_tobody(L, 1); \
		const b2Vec2 vector = get_expr; \
		lua_pushnumber(L, vector.x); \
		lua_pushnumber(L, vector.y); \
		return 2; \
	}

BODY_GETSET_VEC2(position, body->GetPosition(), body->SetTransform(vector, body->GetAngle()))
BODY_GETSET_VEC2(linear_velocity, body->GetLinearVelocity(), body->SetLinearVelocity(vector))

#define BODY_GETSET_FLOAT(value, get_expr, set_expr) \
	int set_##value(lua_State* L) \
	{ \
		b2Body* body = luam_tobody(L, 1); \
		lua_Number value = luaL_checknumber(L, 2); \
		set_expr; \
		return 0; \
	} \
	int get_##value(lua_State* L) \
	{ \
		b2Body* body = luam_tobody(L, 1); \
		const lua_Number value = get_expr; \
		lua_pushnumber(L, value); \
		return 1; \
	}

BODY_GETSET_FLOAT(angle, body->GetAngle(), body->SetTransform(body->GetPosition(), angle))
BODY_GETSET_FLOAT(angular_velocity, body->GetAngularVelocity(), body->SetAngularVelocity(angular_velocity))
BODY_GETSET_FLOAT(linear_damping, body->GetLinearDamping(), body->SetLinearDamping(linear_damping))
BODY_GETSET_FLOAT(angular_damping, body->GetAngularDamping(), body->SetAngularDamping(angular_damping))

int get_mass(lua_State* L)
{
	b2Body* body = luam_tobody(L, 1);
	const lua_Number mass = body->GetMass();
	lua_pushnumber(L, mass);
	return 1;
}

#define BODY_GETSET_BOOL(value, get_expr, set_expr) \
	int set_##value(lua_State* L) \
	{ \
		b2Body* body = luam_tobody(L, 1); \
		bool value = lua_toboolean(L, 2); \
		set_expr; \
		return 0; \
	} \
	int get_##value(lua_State* L) \
	{ \
		b2Body* body = luam_tobody(L, 1); \
		const bool value = get_expr; \
		lua_pushboolean(L, value); \
		return 1; \
	}

BODY_GETSET_BOOL(fixed_rotation, body->IsFixedRotation(), body->SetFixedRotation(fixed_rotation))

static int apply_force(lua_State* L)
{
	b2Body* body = luam_tobody(L, 1);
	lua_Number fx = luaL_checknumber(L, 2);
	lua_Number fy = luaL_checknumber(L, 3);
	b2Vec2 pos;
	if (lua_gettop(L) > 4) {
		lua_Number dx = luaL_checknumber(L, 4);
		lua_Number dy = luaL_checknumber(L, 5);
		body->ApplyForce(b2Vec2(fx, fy), b2Vec2(dx, dy), true);
	} else {
		pos = body->GetLocalCenter();
		body->ApplyForceToCenter(b2Vec2(fx, fy), true);
	}
	return 0;
}
static int apply_linear_impulse(lua_State* L)
{
	b2Body* body = luam_tobody(L, 1);
	lua_Number fx = luaL_checknumber(L, 2);
	lua_Number fy = luaL_checknumber(L, 3);
	b2Vec2 pos;
	if (lua_gettop(L) > 4) {
		lua_Number dx = luaL_checknumber(L, 4);
		lua_Number dy = luaL_checknumber(L, 5);
		pos = b2Vec2(dx, dy);
	} else {
		pos = body->GetLocalCenter();
	}
	body->ApplyLinearImpulse(b2Vec2(fx, fy), pos, true);
	return 0;
}
static int apply_angular_impulse(lua_State* L)
{
	b2Body* body = luam_tobody(L, 1);
	lua_Number angle = luaL_checknumber(L, 2);
	body->ApplyAngularImpulse(angle, true);
	return 0;
}
static int apply_torque(lua_State* L)
{
	b2Body* body = luam_tobody(L, 1);
	lua_Number torque = luaL_checknumber(L, 2);
	body->ApplyTorque(torque, true);
	return 0;
}

static int dump(lua_State* L)
{
	b2Body* body = luam_tobody(L, 1);
	body->Dump();
	return 0;
}


static const luaL_Reg __body_class[] = {
	DECLARE_GETSET(position),
	DECLARE_GETSET(angle),
	DECLARE_GETSET(linear_velocity),
	DECLARE_GETSET(angular_velocity),
	DECLARE_GETSET(linear_damping),
	DECLARE_GETSET(angular_damping),
	DECLARE_GETSET(fixed_rotation),
	DECLARE_FUNCTION(get_mass),
	{"apply_force", apply_force},
	{"apply_linear_impulse", apply_linear_impulse},
	{"apply_angular_impulse", apply_angular_impulse},
	{"apply_torque", apply_torque},
	{"dump", dump},
	{NULL, NULL},
};

// Joint methods

int new_joint(lua_State* L)
{
	assert(world);

	b2JointDef* joint_def;

	const char * type = luaL_checkstring(L, 1);
	int i = 2;
	if (!strcmp(type, "mouse")) {
		b2MouseJointDef* def = new b2MouseJointDef;
		def->bodyA = luam_tobody(L, i++);
		def->bodyB = luam_tobody(L, i++);
		def->maxForce = lua_tonumber(L, i++);
		def->target = def->bodyB->GetWorldCenter();
		joint_def = def;
	} else if (!strcmp(type, "distance")) {
		b2DistanceJointDef* def = new b2DistanceJointDef;
		b2Body* b1 = luam_tobody(L, i++);
		b2Body* b2 = luam_tobody(L, i++);
		def->Initialize(b1, b2, b1->GetWorldCenter(), b2->GetWorldCenter());
		joint_def = def;
	} else if (!strcmp(type, "rope")) {
		b2RopeJointDef* def = new b2RopeJointDef;
		def->bodyA = luam_tobody(L, i++);
		def->bodyB = luam_tobody(L, i++);
		joint_def = def;
	} else {
		assert(false);
	}

	if (lua_gettop(L) >= i) {
		bool collide = lua_toboolean(L, i++);
		joint_def->collideConnected = collide;
	}

	assert(joint_def->bodyA);
	assert(joint_def->bodyB);
	b2Joint* joint = world->CreateJoint(joint_def);

	delete joint_def;

	lua_newtable(L);
	lua_pushlightuserdata(L, joint);
	lua_setfield(L, -2, "__self");
	luaL_getmetatable(L, JOINT_CLASS);
	lua_setmetatable(L, -2);
	return 1;
}

static b2Joint* luam_tojoint(lua_State* L, int index)
{
	luaL_checktype(L, index, LUA_TTABLE);
	lua_getfield(L, index, "__self");
	b2Joint* joint = (b2Joint*) lua_touserdata(L, -1);
	return joint;
}
inline static b2MouseJoint* luam_tomousejoint(lua_State* L, int index)
{
	return (b2MouseJoint*) luam_tojoint(L, index);
}
inline static b2DistanceJoint* luam_todistancejoint(lua_State* L, int index)
{
	return (b2DistanceJoint*) luam_tojoint(L, index);
}
inline static b2RopeJoint* luam_toropejoint(lua_State* L, int index)
{
	return (b2RopeJoint*) luam_tojoint(L, index);
}

int set_target(lua_State* L)
{
	b2MouseJoint* joint = luam_tomousejoint(L, 1);
	lua_Number x = luaL_checknumber(L, 2);
	lua_Number y = luaL_checknumber(L, 3);
	joint->SetTarget(b2Vec2(x, y));
	return 0;
}

int set_length(lua_State* L)
{
	b2DistanceJoint* joint = luam_todistancejoint(L, 1);
	lua_Number length = luaL_checknumber(L, 2);
	joint->SetLength(length);
	return 0;
}
int set_frequency(lua_State* L)
{
	b2DistanceJoint* joint = luam_todistancejoint(L, 1);
	lua_Number freq = luaL_checknumber(L, 2);
	joint->SetFrequency(freq);
	return 0;
}

int set_max_length(lua_State* L)
{
	b2RopeJoint* joint = luam_toropejoint(L, 1);
	lua_Number maxlength = luaL_checknumber(L, 2);
	joint->SetMaxLength(maxlength);
	return 0;
}


int destroy(lua_State* L)
{
	b2Joint* joint = luam_tojoint(L, 1);
	world->DestroyJoint(joint);
	return 0;
}

static const luaL_Reg __joint_class[] = {
	// joint
	DECLARE_FUNCTION(destroy),
	// mouse joint
	DECLARE_FUNCTION(set_target),
	// distance joint
	DECLARE_FUNCTION(set_length),
	DECLARE_FUNCTION(set_frequency),
	// rope joint
	DECLARE_FUNCTION(set_max_length),
	{NULL, NULL},
};

// Physic module

static const luaL_Reg lib[] =
{
	DECLARE_FUNCTION(create_world),

	DECLARE_FUNCTION(new_shape),
	DECLARE_FUNCTION(new_body),
	DECLARE_FUNCTION(new_joint),

	DECLARE_FUNCTION(update),

	{NULL, NULL}
};

LUA_API "C" int luaopen_physic(lua_State *L)
{
	luaL_newlib(L, lib);
	luaL_setfuncs(L, lib, 0);

	// register BODY_CLASS
	luaL_newmetatable(L, BODY_CLASS);
	luaL_setfuncs(L, __body_class, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_setfield(L, -2, BODY_CLASS);

	// register SHAPE_CLASS
	luaL_newmetatable(L, SHAPE_CLASS);
	luaL_setfuncs(L, __shape_class, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_setfield(L, -2, SHAPE_CLASS);

	// register JOINT_CLASS
	luaL_newmetatable(L, JOINT_CLASS);
	luaL_setfuncs(L, __joint_class, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_setfield(L, -2, JOINT_CLASS);

	return 1;
}

