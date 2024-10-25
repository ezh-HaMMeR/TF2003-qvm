/*
 *  QWProgs-TF2003
 *  Copyright (C) 2004  [sd] angel
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  $Id: buttons.c,v 1.4 2005-05-17 03:56:00 AngelD Exp $
 */
#include "g_local.h"

// Fucking bullshit not compiling in separate file
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
// End of bullshit code

void    button_return(  );

void button_wait(  )
{
	self->state = STATE_TOP;
	self->s.v.nextthink = self->s.v.ltime + self->wait;
	self->s.v.think = ( func_t ) button_return;
	activator = PROG_TO_EDICT( self->s.v.enemy );

	SUB_UseTargets(  );

	self->s.v.frame = 1;	// use alternate textures
}

void button_done(  )
{
	self->state = STATE_BOTTOM;
}

void button_return(  )
{
	self->goal_state = 2;

	self->state = STATE_DOWN;

	SUB_CalcMove( self->pos1, self->speed, button_done );

	self->s.v.frame = 0;	// use normal textures

	if ( self->s.v.health )
		self->s.v.takedamage = DAMAGE_YES;	// can be shot again
}

void button_blocked(  )
{				// do nothing, just don't ome all the way back out
}

void button_fire(  )
{
	if ( self->state == STATE_UP || self->state == STATE_TOP )
		return;

	sound( self, CHAN_VOICE, self->s.v.noise, 1, ATTN_NORM );

	self->state = STATE_UP;

	SUB_CalcMove( self->pos2, self->speed, button_wait );

}

void button_use(  )
{
	self->s.v.enemy = EDICT_TO_PROG( activator );
	button_fire(  );
}


void button_touch(  )
{
	gedict_t *te;

	if ( tf_data.cb_prematch_time > g_globalvars.time )
		return;

	if ( other->s.v.health <= 0 )
		return;

	if ( strneq( other->s.v.classname, "player" ) )
		return;

	if ( self->goal_activation & TFGA_SPANNER )
		return;
	if ( !other->playerclass )
		return;

	if ( !Activated( self, other ) )
	{
		if ( self->else_goal )
		{
			te = Findgoal( self->else_goal );
			if ( te )
				AttemptToActivate( te, other, self );
		}
		return;
	}
	self->s.v.enemy = EDICT_TO_PROG( other );
	button_fire(  );
}

void button_killed(  )
{
	if ( self->goal_activation & TFGA_SPANNER )
		return;
	self->s.v.enemy = EDICT_TO_PROG( damage_attacker );
	self->s.v.health = self->s.v.max_health;
	self->s.v.takedamage = DAMAGE_NO;	// wil be reset upon return

	button_fire(  );
}

/*QUAKED func_button (0 .5 .8) ?
When a button is touched, it moves some distance in the direction of it's angle, triggers all of it's targets, waits some time, then returns to it's original position where it can be triggered again.

"angle"  determines the opening direction
"target" all entities with a matching targetname will be used
"speed"  override the default 40 speed
"wait"  override the default 1 second wait (-1 = never return)
"lip"  override the default 4 pixel lip remaining at end of move
"health" if set, the button must be killed instead of touched
"sounds"
0) steam metal
1) wooden clunk
2) metallic click
3) in-out
*/

void SP_func_button(  )
{
	float   ftemp;

	if ( !CheckExistence(  ) )
	{
		dremove( self );
		return;
	}
	if ( self->s.v.sounds == 0 )
	{
		trap_precache_sound( "buttons/airbut1.wav" );
		self->s.v.noise = "buttons/airbut1.wav";
	}
	if ( self->s.v.sounds == 1 )
	{
		trap_precache_sound( "buttons/switch21.wav" );
		self->s.v.noise = "buttons/switch21.wav";
	}
	if ( self->s.v.sounds == 2 )
	{
		trap_precache_sound( "buttons/switch02.wav" );
		self->s.v.noise = "buttons/switch02.wav";
	}
	if ( self->s.v.sounds == 3 )
	{
		trap_precache_sound( "buttons/switch04.wav" );
		self->s.v.noise = "buttons/switch04.wav";
	}

	SetMovedir(  );

	self->s.v.movetype = MOVETYPE_PUSH;
	self->s.v.solid = SOLID_BSP;
	setmodel( self, self->s.v.model );

	self->s.v.blocked = ( func_t ) button_blocked;
	self->s.v.use = ( func_t ) button_use;

	if ( self->s.v.health )
	{
		self->s.v.max_health = self->s.v.health;
		self->th_die = button_killed;
		self->s.v.takedamage = DAMAGE_YES;
	} else
		self->s.v.touch = ( func_t ) button_touch;

	if ( !self->speed )
		self->speed = 40;

	if ( !self->wait )
		self->wait = 1;

	if ( !self->lip )
		self->lip = 4;

	self->state = STATE_BOTTOM;

	VectorCopy( self->s.v.origin, self->pos1 );
	ftemp = ( fabs( DotProduct( self->s.v.movedir, self->s.v.size ) ) - self->lip );

	self->pos2[0] = self->pos1[0] + self->s.v.movedir[0] * ftemp;
	self->pos2[1] = self->pos1[1] + self->s.v.movedir[1] * ftemp;
	self->pos2[2] = self->pos1[2] + self->s.v.movedir[2] * ftemp;

	if ( ( int ) ( self->s.v.spawnflags ) & 32 )
		button_fire(  );
}

// KOTH
float min(float a, float b) {
	return (a < b ? a : b);
}

float max(float a, float b) {
	return (a > b ? a : b);
}

int boxvsbox(vec3_t mn1, vec3_t mx1, vec3_t mn2, vec3_t mx2) {
	return max(mn1[0], mn2[0]) <= min(mx1[0], mx2[0]) &&
		   max(mn1[1], mn2[1]) <= min(mx1[1], mx2[1]) &&
		   max(mn1[2], mn2[2]) <= min(mx1[2], mx2[2]);
}

void SP_capture_point_think() {
	gedict_t* te = world;
	int blue, red;
	blue = red = 0;

	te = trap_find(te, FOFS(s.v.classname), "kotharea_end");
	if (te) {
		self->point_abbmin[0] = min(self->s.v.origin[0], te->s.v.origin[0]);
		self->point_abbmin[1] = min(self->s.v.origin[1], te->s.v.origin[1]);
		self->point_abbmin[2] = min(self->s.v.origin[2], te->s.v.origin[2]);

		self->point_abbmax[0] = max(self->s.v.origin[0], te->s.v.origin[0]);
		self->point_abbmax[1] = max(self->s.v.origin[1], te->s.v.origin[1]);
		self->point_abbmax[2] = max(self->s.v.origin[2], te->s.v.origin[2]);

		dremove(te);
	}

	for (te = world; (te = trap_find(te, FOFS(s.v.classname), "player"));) {
    if (boxvsbox(self->point_abbmin, self->point_abbmax, te->s.v.absmin, te->s.v.absmax)) {
    	if (te->team_no == 1) {
    		blue++;
    	} else {
    		red++;
    	}
    }
  }
  if (blue > 0 && !red) {
  	if (self->cap_team == 0 && self->team_no != 1) self->cap_team = 1;
  	if (self->cap_team == 1) {
  		self->cap_progress += 1;
  	} else {
  		self->cap_progress -= (self->cap_team == 2 ? 2 : 1);
  	}
  } else if (red > 0 && !blue) {
  	if (self->cap_team == 0 && self->team_no != 2) self->cap_team = 2;
  	if (self->cap_team == 2) {
  		self->cap_progress += 1;
  	} else {
  		self->cap_progress -= (self->cap_team == 1 ? 2 : 1);
  	}
  } else if (blue == 0 && red == 0) {
  	if (self->cap_progress >= 0) {
  		self->cap_progress -= 1;
  	} else {
  		self->cap_team = 0;
  	}
  }
  if (self->cap_progress < 0) self->cap_progress = 0;
  if (self->cap_progress >= 15) {
  	self->team_no = self->cap_team;
  	self->cap_team = 0;
  }

  if (g_globalvars.time - self->last_score_think >= 1.0) {
  	self->last_score_think = g_globalvars.time;
  	if (self->team_no != 0) {
  		teamscores[self->team_no] += 1;
  		G_conprintf("teamscores: %d %d\n", teamscores[1], teamscores[2]);
  		if (teamscores[self->team_no] > 15) {
  			G_conprintf("Team no %d won!!!\n", self->team_no);
  		}
  	}
  }

  G_conprintf("team_no: %d; cap_team: %d; cap_progress: %d\n", self->team_no, self->cap_team, self->cap_progress);
  self->s.v.nextthink = g_globalvars.time + 1;
}

void SP_capture_point() {
	self->team_no = 0; // 0 -> No team
	self->cap_team = 0;
	self->last_score_think = g_globalvars.time;
	self->s.v.think = (func_t)SP_capture_point_think;
	self->s.v.nextthink = g_globalvars.time + 1;
}

void SP_capture_point_endarea() {
	self->s.v.classname = "kotharea_end";
}