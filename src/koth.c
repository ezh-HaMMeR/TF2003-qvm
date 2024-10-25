#include "g_local.h"

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