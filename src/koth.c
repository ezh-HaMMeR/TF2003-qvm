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

int pointvsprism(vec3_t point, vec3_t* points, int len) {
	int counter = 0;
  int i;
  double xinters;
  vec3_t point1, point2;

  VectorCopy(points[0], point1);
  for (i = 1; i <= len; i++) {
    VectorCopy(points[i % len], point2);
    if (point[1] > min(point1[1], point2[1])) {
      if (point[1] <= max(point1[1], point2[1])) {
        if (point[0] <= max(point1[0], point2[0])) {
          if (point1[1] != point2[1]) {
            xinters = (point[1] - point1[1])*(point2[0] - point1[0])/(point2[1] - point1[1])+point1[0];
            if (point1[0] == point2[0] || point[0] <= xinters) counter++;
          }
        }
      }
    }
    VectorCopy(point2, point1);
  }

  return counter % 2;
}

int boxvsprism(vec3_t mns, vec3_t mxs, vec3_t* points, int len, float height) {
	vec3_t point;
	if (max(mns[2], points[0][2]) > min(mxs[2], points[0][2] + height)) return 0;

	VectorCopy(mns, point);
	if (pointvsprism(point, points, len)) return 1;

	VectorCopy(mxs, point);
	if (pointvsprism(point, points, len)) return 1;

	point[0] = mns[0];
	point[1] = mxs[1];
	if (pointvsprism(point, points, len)) return 1;

	point[0] = mxs[0];
	point[1] = mns[1];
	if (pointvsprism(point, points, len)) return 1;

	return 0;
}

void SP_capture_point_think() {
	gedict_t* te = world;
	int blue, red;
	blue = red = 0;

	if (g_globalvars.time < tf_data.cb_prematch_time) {
		self->s.v.nextthink = tf_data.cb_prematch_time;
		return;
	}

	for (te = world; (te = trap_find(te, FOFS(s.v.classname), "player"));) {
		if (te->s.v.deadflag || !te->playerclass) continue;
    if (boxvsprism(te->s.v.absmin, te->s.v.absmax, self->cp_points, self->cp_points_amount, self->cp_height)) {
    	if (te->team_no == 1) {
    		blue++;
    	} else if (te->team_no == 2) {
    		red++;
    	}
    }
  }

  if (blue > 0 && !red) {
  	if (self->cap_team == 0 && self->team_no != 1) {
  		self->cap_team = 1;
  		G_bprint(2, "Blue team is contesting the point!\n");
  		for (te = world; (te = trap_find(te, FOFS(s.v.classname), "player"));) {
	      stuffcmd(te, "play koth/%s.wav\n", (te->team_no != self->cap_team ? "cap_enemy" : "cap_friendly"));
	    }
  	}
  	
  	if (self->cap_team == 1) {
  		self->cap_progress += min(blue, self->cp_max_cap_mult) / 10.0;
  	} else {
  		self->cap_progress -= (self->team_no == 2 ? 1.35 : 1) / 10.0;
  	}
  } else if (red > 0 && !blue) {
  	if (self->cap_team == 0 && self->team_no != 2) {
  		self->cap_team = 2;
  		G_bprint(2, "Red team is contesting the point!\n");
  		for (te = world; (te = trap_find(te, FOFS(s.v.classname), "player"));) {
	      stuffcmd(te, "play koth/%s.wav\n", (te->team_no != self->cap_team ? "cap_enemy" : "cap_friendly"));
	    }
  	}

  	if (self->cap_team == 2) {
  		self->cap_progress += min(red, self->cp_max_cap_mult) / 10.0;
  	} else {
  		self->cap_progress -= (self->team_no == 1 ? 1.35 : 1) / 10.0;
  	}
  } else if (blue == 0 && red == 0) {
  	if (self->cap_progress > 0) {
  		self->cap_progress -= 1 / 10.0;
  	} else {
  		self->cap_team = 0;
  	}
  }

  if (self->cap_progress < 0) self->cap_progress = 0;
  
  if (self->cap_progress >= self->cp_cap_time) {
  	self->team_no = self->cap_team;
  	self->cap_team = 0;
  	self->cap_progress = 0;
  	FlagColor(self->team_no);
  	G_bprint(2, "%s team has the point!\n", (self->team_no == 1 ? "Blue" : "Red"));

  	for (te = world; (te = trap_find(te, FOFS(s.v.classname), "player"));) {
      stuffcmd(te, "play koth/cap_finished.wav\n");
    }
  }

  if (g_globalvars.time - self->last_score_think >= 1.0) {
  	self->last_score_think = g_globalvars.time;
  	if (self->team_no != 0) {
  		teamscores[self->team_no] += 1;
  		if (CheckWinConditions()) {
  			return;
  		}
  	}
  }

  self->s.v.nextthink = g_globalvars.time + 0.1;
}

void SP_capture_point_init() {
	vec3_t center, tmp1, tmp2;
	gedict_t* te;
	char point_name[32];
	int point_name_size = 0, i, j;
	float cross;

	self->cp_points_amount = 0;
	for (i = 0; i < strlen(self->s.v.target); i++) {
		if (self->s.v.target[i] == ';') {
			point_name[point_name_size] = '\0';
			point_name_size = 0;
			te = world;
			te = trap_find(te, FOFS(s.v.targetname), point_name);
			if (!te) {
				self->s.v.nextthink = g_globalvars.time + 1;
				return;
			}
			VectorCopy(te->s.v.origin, self->cp_points[self->cp_points_amount]);
			self->cp_points_amount++;
		} else {
			point_name[point_name_size++] = self->s.v.target[i];
		}
	}

	if (point_name_size) {
		point_name[point_name_size] = '\0';
		point_name_size = 0;
		te = world;
		te = trap_find(te, FOFS(s.v.targetname), point_name);
		if (!te) {
			self->s.v.nextthink = g_globalvars.time + 1;
			return;
		}
		VectorCopy(te->s.v.origin, self->cp_points[self->cp_points_amount]);
		self->cp_points_amount++;
	}

	center[0] = center[1] = center[2] = 0;
	for (i = 0; i < self->cp_points_amount; i++) {
		center[0] += self->cp_points[i][0];
		center[1] += self->cp_points[i][1];
		center[2] += self->cp_points[i][2];
	}
	center[0] /= self->cp_points_amount;
	center[1] /= self->cp_points_amount;
	center[2] /= self->cp_points_amount;
	for (i = 0; i < self->cp_points_amount; i++) {
		self->cp_points[i][2] = center[2];
	}

	for (i = 0; i < self->cp_points_amount - 1; i++) {
		for (j = 0; j < self->cp_points_amount - i - 1; j++) {
			VectorSubtract(self->cp_points[j + 1], center, tmp1);
			VectorSubtract(self->cp_points[j], center, tmp2);
			cross = tmp1[0]*tmp2[1] - tmp1[1]*tmp2[0];
			if (cross > 0) {
				VectorAdd(tmp1, center, tmp1);
				VectorAdd(tmp2, center, tmp2);
				VectorCopy(tmp2, self->cp_points[j + 1]);
				VectorCopy(tmp1, self->cp_points[j]);
			}
		}
	}
	self->s.v.think = (func_t)SP_capture_point_think;
	self->s.v.nextthink = g_globalvars.time + 1;
}

void SP_capture_point() {
	self->team_no = 0; // 0 -> No team
	self->cap_team = 0;
	self->last_score_think = g_globalvars.time;
	self->s.v.think = (func_t)SP_capture_point_init;
	self->s.v.nextthink = g_globalvars.time + 1;
	self->s.v.classname = "control_point";

	self->cp_cap_time = (self->cp_cap_time ? self->cp_cap_time : 10);
	self->cp_hold_time = (self->cp_hold_time ? self->cp_hold_time : 300);
	self->cp_max_cap_mult = (self->cp_max_cap_mult ? self->cp_max_cap_mult : 3);
	self->cp_height = (self->cp_height ? self->cp_height : 128);

	trap_precache_model("progs/tf_stan.mdl");
	trap_precache_sound("koth/cap_finished.wav");
	trap_precache_sound("koth/cap_enemy.wav");
	trap_precache_sound("koth/cap_friendly.wav");
}

void FlagColor(int team) {
	int i;
	gedict_t* te, *flag_pos;
	vec3_t pos;

	te = world;
	te = trap_find(te, FOFS(s.v.classname), "control_point_flag");
	if (!te) {
		te = spawn();
		te->s.v.classname = "control_point_flag";
		te->s.v.solid = SOLID_NOT;

		pos[0] = pos[1] = pos[2] = 0;
		for (i = 0; i < self->cp_points_amount; i++) {
			pos[0] += self->cp_points[i][0];
			pos[1] += self->cp_points[i][1];
			pos[2] += self->cp_points[i][2];
		}
		pos[0] /= self->cp_points_amount;
		pos[1] /= self->cp_points_amount;
		pos[2] /= self->cp_points_amount;
		pos[2] += self->height;
		flag_pos = world;
		flag_pos = trap_find(flag_pos, FOFS(s.v.targetname), self->flag_pos);
		if (flag_pos) {
			VectorCopy(flag_pos->s.v.origin, pos);
		}
		setmodel(te, "progs/tf_stan.mdl");
		setsize(te, 0, 0, 0, 0, 0, 0);
		setorigin(te, PASSVEC3(pos));
	}
	te->s.v.skin = (team == 1 ? 1 : 2);
	te->s.v.effects = (int)te->s.v.effects & ~(EF_BLUE | EF_RED) | (team == 1 ? EF_BLUE : EF_RED);
}

int CheckWinConditions() {
	if (teamscores[1] >= self->cp_hold_time) {
		G_bprint(2, "Blue team won!\n");
		NextLevel();
		dremove(self);
		return 1;
	}
	if (teamscores[2] >= self->cp_hold_time) {
		G_bprint(2, "Red team won!\n");
		NextLevel();
		dremove(self);
		return 1;
	}
	return 0;
}