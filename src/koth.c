#include "g_local.h"

float min(float a, float b) {
	return (a < b ? a : b);
}

float max(float a, float b) {
	return (a > b ? a : b);
}

int boxvsbox(vec3_t mn1, vec3_t mx1, vec3_t mn2, vec3_t mx2) {
	return max(mn1.x, mn2.x) <= min(mx1.x, mx2.x) &&
				 max(mn1.y, mn2.y) <= min(mx1.y, mx2.y) &&
				 max(mn1.z, mn2.z) <= min(mx1.z, mx2.z);
}

void SP_trigger_koth_think() {
	gedict_t* te;
	int blue, red;
	blue = red = 0;
	for (te = world; (te = trap_find(te, FOFS(s.v.classname), "player"));) {
    if (boxvsbox(self->s.v.absmin, self->s.v.absmax, te->s.v.absmin, te->s.v.absmax)) {
    	if (te->team_no == 1) {
    		blue++;
    	} else {
    		red++;
    	}
    }
  }
  if (blue > 0 && !red) {
  	if (self->cap_team == 0 && self->team_no != 1) cap_team = 1;
  	if (cap_team == 1) {
  		self->cap_progress += 1;
  	} else {
  		self->cap_progress -= 2;
  	}
  } else if (red > 0 && !blue) {
  	if (self->cap_team == 0 && self->team_no != 2) cap_team = 2;
  	if (cap_team == 2) {
  		self->cap_progress += 1;
  	} else {
  		self->cap_progress -= 2;
  	}
  } else if (blue && red == 0) {
  	if (self->cap_progress >= 0) {
  		self->cap_progress -= 1;
  	} else {
  		self->cap_team = 0;
  	}
  }
  if (self->cap_progress < 0) self->cap_progress = 0;
  if (self->cap_progress >= 100) {
  	self->team_no = self->cap_team;
  	self->cap_team = 0;
  }

  if (g_globalvars.time - self->last_score_think >= 1.0) {
  	self->last_score_think = g_globalvars.time;
  	if (self->team_no != 0) {
  		teamscores[self->team_no] += 1;
  		if (teamscores[self->team_no] > 15) {
  			G_conprintf("Team no %d won!!!\n", self->team_no);
  		}
  	}
  }

  G_conprintf("team_no: %d; cap_team: %d; cap_progress: %d\n", self->team_no, self->cap_team, self->cap_progress);
  self->s.v.nextthink = g_globalvars.time;
}

void SP_trigger_koth() {
	if (!CheckExistence()) {
		dremove(self);
		return;
	}

	self->team_no = 0; // 0 -> No team
	self->cap_team = 0;
	self->s.v.think = (func_t)SP_trigger_koth_think;
}