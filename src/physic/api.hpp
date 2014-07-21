#include "module.hpp"

BEGIN_MODULE(physic)
	DECLARE_FUNCTION(create_world)

	DECLARE_FUNCTION(new_shape)
	DECLARE_FUNCTION(new_body)
	DECLARE_FUNCTION(new_joint)

	DECLARE_FUNCTION(update_physic)
	DECLARE_FUNCTION(on_collision)

	DECLARE_FUNCTION(raycast)
	DECLARE_FUNCTION(query)

	BEGIN_CLASS(body)
		ADD_GETSET(body, position)
		ADD_GETSET(body, angle)
		ADD_GETSET(body, linear_velocity)
		ADD_GETSET(body, angular_velocity)
		ADD_GETSET(body, linear_damping)
		ADD_GETSET(body, angular_damping)
		ADD_GETSET(body, fixed_rotation)

		ADD_METHOD(body, set_active)
		ADD_METHOD(body, set_bullet)
		ADD_METHOD(body, get_mass)
		ADD_METHOD(body, set_mass_center)
		ADD_METHOD(body, apply_force)
		ADD_METHOD(body, apply_linear_impulse)
		ADD_METHOD(body, apply_angular_impulse)
		ADD_METHOD(body, apply_torque)
		ADD_METHOD(body, dump)
		ADD_METHOD(body, destroy)
	REGISTER_CLASS(body, "Body")
	/*
	 * Set field so gamedevs can do:
	 * local MyBody = setmetatable({
	 *     somevars...,
	 * }, physic.Body)
	 * MyBody.__index = MyBody
	 *
	 * ...
	 *
	 * local body = setmetatable(physic.new_body(.., shape), MyBody)
	 * body.somevars = ...
	 */

	BEGIN_CLASS(shape)
		ADD_GETSET(shape, density)
		ADD_GETSET(shape, restitution)
		ADD_GETSET(shape, friction)
		ADD_METHOD(shape, set_sensor)
		ADD_GC(gc_shape)
	REGISTER_CLASS(shape, "Shape")

	BEGIN_CLASS(joint)
		ADD_METHOD(joint, destroy)
		// mouse joint
		ADD_METHOD(joint, set_target)
		// distance joint
		ADD_METHOD(joint, set_length)
		ADD_METHOD(joint, set_frequency)
		// rope joint
		ADD_METHOD(joint, set_max_length)
		// revolute joint
		ADD_METHOD(joint, set_angle_limits)
		ADD_METHOD(joint, set_motor_speed)
	REGISTER_CLASS(joint, "Joint")
END_MODULE()

