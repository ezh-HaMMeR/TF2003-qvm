#include "g_local.h"

void func_rotatable_think() {
	self->s.v.angles[(int)self->style] = anglemod(anglemod(self->s.v.angles[(int)self->style]) + self->s.v.yaw_speed);
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

void SP_func_rotatable() {
	if (!CheckExistence()) {
		dremove(self);
		return;
	}

	if ((int)self->style < 0) {
		self->style = 0;
	}

	if ((int)self->style > 2) {
		self->style = 2;
	}

	self->s.v.movetype = MOVETYPE_PUSH;
	self->s.v.solid = SOLID_BSP;
	setmodel(self, self->s.v.model);
	VectorCopy(self->s.v.origin, self->pos1);

	self->s.v.think = (func_t)func_rotatable_think;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

void point_ambient_generic_use() {
	sound(self, CHAN_VOICE, self->s.v.noise, 1, ATTN_NORM);
}

void SP_point_ambient_generic() {
	if (!CheckExistence()) {
		dremove(self);
		return;
	}

	self->s.v.classname = "point_ambient_generic";
	trap_precache_sound(self->s.v.noise);
	self->s.v.use = (func_t)point_ambient_generic_use;
}

void SP_point_ambient() {
	if (!CheckExistence()) {
		dremove(self);
		return;
	}

	trap_precache_sound(self->s.v.noise);
	sound(self, CHAN_VOICE, self->s.v.noise, 1, ATTN_NORM);
}