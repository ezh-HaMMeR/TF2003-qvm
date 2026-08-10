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
 *  $Id: progs.h,v 1.28 2006-11-26 21:33:24 AngelD Exp $
 */
#include "progdefs.h"

#define	MAX_ENT_LEAFS	16

/* Set to 1 to restore per-player damage accounting and TFINFO_DAMAGE
 * network updates. The implementation is intentionally kept behind this
 * switch so it can be re-enabled without reconstructing the old code. */
#define TF2003_DAMAGE_STATS_ENABLED 1

typedef struct shared_edict_s {
	entvars_t       v;	// C exported fields from progs
// other fields from progs come immediately after
} edict_t;

struct gedict_s;
typedef void (*th_die_func_t)();
typedef void (*th_pain_func_t)(struct gedict_s *, float);

struct thick_s {
    th_die_func_t frame_func, last_func, last_think;
    int start, end;
};
struct eff_info_s {
    int conc_idle;
    float dmg, dmg_curr;
};

typedef struct gedict_s {
	edict_t         s;
    struct thick_s frame_info;
    struct eff_info_s eff_info;
	//custom fields
	float	items2;				   // using  ZQ_ITEMS2 extension in mvdsv we can use per client sigils for runes
	int 	k_admin;         // if player is an admin, flags
	float 			brokenankle; // player can't jump for a while after falling and get damaged
	float           maxspeed, gravity;
	float           vw_index;
	int         hideentity;             // links to entity to hide in eye chasecam
	int			trackent;			// pseudo spectating for players.
	qboolean       hideplayers;            // if set, all players hidden (read by mvdsv)
// { highlights which clients this entity was visible to
	unsigned int visclients;
// }
	// let mvdsv know when player has teleported, and adjust for high-ping
	int          teleported;

	char           *mdl;
	char           *killtarget;
	int            worldtype;	// 0=medieval 1=metal 2=base
	char           *map;
//player
	int             walkframe;

//
// quakeed fields
//
	int             style;
	float           speed;
	int             state;
	int             voided;
//items.c
	float           healamount, healtype;
//ammo
	int             aflag;
//
// doors, etc
//
	vec3_t		movement;
	vec3_t          dest, dest1, dest2;
	vec3_t          pos1, pos2;
	vec3_t          mangle;
	float           t_length, t_width, height;
	float           wait;	// time from firing to restarting
	float           delay;	// time from activation to firing
	struct gedict_s *trigger_field;	// door's trigger entity
	struct gedict_s *movetarget;
	float           pausetime;
	char           *noise4;
	float           count;
	int		last_impulse;
//
// misc
//
	float           cnt;	// misc flag

	void            ( *think1 ) ();	//calcmove
	vec3_t          finaldest;
//hook  
	int	lefty;
//combat
	float           dmg;
	float           lip;
	float           attack_finished;
	float           pain_finished;

	float           invincible_finished;
	float           invisible_finished;
	float           super_damage_finished;
	float           radsuit_finished;


	float           invincible_time, invincible_sound;
	float           invisible_time, invisible_sound;
	float           super_time, super_sound;
	float           rad_time;
	float           fly_sound;

	float           axhitme;
	float           show_hostile;	// set to time+0.2 whenever a client fires a
	// weapon or takes damage.  Used to alert
	// monsters that otherwise would let the player go
	float           jump_flag;	// player jump flag
	float           swim_flag;	// player swimming sound flag
	float           air_finished;	// when time > air_finished, start drowning
	float           bubble_count;	// keeps track of the number of bubbles
	char           *deathtype;	// keeps track of how the player died
	float           dmgtime;

	void            ( *th_die ) ();
	void            ( *th_pain ) ( struct gedict_s *, float );

///team fortess
	int damage, touches, caps;
	int damage_update_pending; /* Used when TF2003_DAMAGE_STATS_ENABLED is 1. */
	int team_no, playerclass;
	int 		ready;
	int             nextpc;
	int             armorclass;
	int             tf_items, tf_items_flags;
	int             got_aliases;
	int             cheat_check;
	int             is_removed;
	int             is_undercover;
	int             is_building;
	int             is_detpacking;
	int             is_feigning;
	int             is_unabletospy;
	int             has_disconnected;
	int 		can_give_goal;
	struct gedict_s *linked_list, *observer_list;
	struct gedict_s* primed_grenade;
	/* Authoritative throw request; used only by live "primer" entities. */
	int             grenade_throw_requested;
	/* Redundant grenade input carried by usercmd.button1 (+use). */
	int             grenade_button1_state;
	int             grenade_button1_prime_impulse;
	int             grenade_button1_suppress_prime;

	int             maxammo_shells;
	int             maxammo_nails;
	int             maxammo_cells;
	int             maxammo_rockets;
	int             items_allowed;
	float           armor_allowed;
	int             maxarmor;
	int             maxfbspeed;
	int             maxstrafespeed;
	int             weaponmode;
	int             motd;
	int             current_menu;
	int             menu_count;
	int             menu_displaytime;

	int             lives, infection_team_no;
	struct gedict_s *building, *real_owner,*oldenemy;

	struct gedict_s *sentry, *dispenser;
	float           building_wait, respawn_time, suicide_time;
	int             has_dispenser;
	int             has_sentry;
	int             real_frags;
	int             weapons_carried;
	int             current_weapon;
	int             last_weapon;
	int             last_weaponmode;
	int             ammo_medikit;
	int             maxammo_medikit;
	int             ammo_detpack;
	int             maxammo_detpack;
	int             reload_shotgun;
	int             reload_super_shotgun;
	int             reload_grenade_launcher;
	int             reload_rocket_launcher;
	/* Authoritative reload state; TFSTATE_RELOADING is only its mirror. */
	struct gedict_s *reload_timer;
	float           heat;
	float           immune_to_check;
	float           last_saveme_sound;
	float           last_goal_give;
	int             no_active_nail_grens;
	/* Stable FIFO metadata for Demoman pipebomb overflow handling. */
	int             pipebomb_order;
	int             pipebomb_forced;
	float		waitmin,waitmax;

//goal fields
        int		has_abbreviated;
        int		distance;
        float		search_time;
	int             goal_no;
	int             group_no;
	int             goal_state;
	int             owned_by;
	int             goal_activation;
	int             goal_effects;
	int             goal_result;
	int             goal_group;
	int             else_goal;
	int             if_goal_is_active;
	int             if_goal_is_inactive;
	int             if_goal_is_removed;
	int             if_group_is_active;
	int             if_group_is_inactive;
	int             if_group_is_removed;
	int             activate_goal_no;
	int             inactivate_goal_no;
	int             remove_goal_no;
	int             restore_goal_no;
	int             activate_group_no;
	int             inactivate_group_no;
	int             remove_group_no;
	int             restore_group_no;
	vec3_t          goal_min, goal_max;
	int             has_item_from_group;
	int             hasnt_item_from_group;
	int             remove_item_group;
	int             return_item_no;
	int             if_item_has_moved;
	int             if_item_hasnt_moved;
	int             remove_spawnpoint;
	int             restore_spawnpoint;
	int             remove_spawngroup;
	int             restore_spawngroup;
	int             display_item_status1;
	int             display_item_status2;
	int             display_item_status3;
	int             display_item_status4;
	char           *team_str_home;
	char           *team_str_moved;
	char           *team_str_carried;
	char           *non_team_str_home;
	char           *non_team_str_moved;
	char           *non_team_str_carried;
	int             ex_skill_min;
	int             ex_skill_max;
	int             increase_team1;
	int             increase_team2;
	int             increase_team3;
	int             increase_team4;
	char           *broadcast;
	char           *team_broadcast;
	char           *non_team_broadcast;
	char           *owners_team_broadcast;
	char           *netname_broadcast;
	char           *netname_team_broadcast;
	char           *netname_non_team_broadcast;
	char           *netname_owners_team_broadcast;
	char           *team_drop;
	char           *non_team_drop;
	char           *netname_team_drop;
	char           *netname_non_team_drop;
	int             all_active;
	int             item_list;
	float           delay_time;
	int             dont_do_triggerwork;

	int             numflames;
	char*		flame_id;

// koth
	float 		cap_progress;
	int 		cap_team;
	float 		last_score_think;
	vec3_t 		cp_points[16];
	int 		cp_points_amount;
	int 		cp_height;
	int 		cp_cap_time;
	int 		cp_hold_time;
	int 		cp_max_cap_mult;
	string_t 	flag_pos;

//spy
	int             undercover_team;
	int             undercover_skin;
	char           *undercover_name;
	struct gedict_s *last_attacker;
	int last_dmsg;



	float           camdist;
	vec3_t          camangle;
	struct gedict_s *camera_list;

	int             hook_out;
	struct gedict_s *hook;
	int             on_hook;
	int             fire_held_down;

	vec3_t          head_shot_vector;
	int             leg_damage;
	int             cheat_level;
	float          	FlashTime;
	int             nailpos;
//player settings 
        int             isSpectator;
	float           StatusRefreshTime;
	int             StatusBarSize;
	int             StatusBarRes;
	int             ScannerOn;
	int             ScanRange;
	int		assault_min_shells;
	int             discard_shells;
	int             discard_nails;
	int             discard_rockets;
	int             discard_cells;
	int             settings_bits;
	int             internal_settings_bits;
	int             take_sshot;
	int 		ignoremapflaginfo;
	int 		useprimetothrow;
	int             newvote;
//player settings end
	int             tf_id;
	int             teamkills;
	int 		tf_impulse,imp1,imp2,imp3,imp4;
//admin
	int		is_admin;
	int		admin_mode;
	int		autokick_kills,autokick_time;
	struct gedict_s*	admin_use;
	float           auth_time; 

	float		allow_snip_time;
	int		has_fake_gas_setinfo;
	float		gren_eff_time;
	vec3_t		lastvel,accel;
	float		lasttime;

	int	 	k_voted;
	float		last_vote_time;
	int             vote_menu_page;
	int             vote_menu_slots[7];

//BotFields
        int            old_button0,old_button1,old_button2,old_keys;
	int            isBot;
	int            keys, nextimpulse;
	int            action,oldAction;
	int            bot_team,bot_skin;
	int            bot_strafe_state;
	int            bot_lastmenu,bot_lastmenucount;
	float          fBotMessageTime;
	vec3_t         botDirection,lastSighted;
	struct gedict_s*      target1,*target2,*target3;
	float          checkMovementTime;
	float          obs_time;
//movment
        float          botNoMoveTime;
        float          time_for_wpchange;

        const struct waypoint_s* wp;
        struct waypoint_s*   end_wp;
        const struct wp_link_s*  wp_link;
        struct wp_path_s      *wp_path;
///
#ifdef VWEP_TEST
	float   vw_index;
	float   vw_frame;
#endif
}gedict_t;

extern  float           sv_gravity;
extern  int             sv_watervis;
extern  int 		skill;
extern  int 		number_of_teams; 
extern  int             illegalclasses[5];
extern  int             civilianteams;		
extern  int             teamscores[5];
extern  int             teamfrags[5];
extern  int             teamlives[5];
extern	int             teammaxplayers[5];
//extern  float		teamadvantage[5];       
extern 	int 		teams_allied;

extern  int             CTF_Map;
extern  int             TF_MapHasFlagInfo;                       
extern  float           rj;                     
extern char           *team_menu_string;  

extern int FLAME_MAXWORLDNUM, MAX_WORLD_PIPEBOMBS, MAX_WORLD_AMMOBOXES, GR_TYPE_MIRV_NO, GR_TYPE_NAPALM_NO;

typedef enum sg_sfire_e
{
	SG_SFIRE_NEW,
	SG_SFIRE_281,
	SG_SFIRE_MTFL1,
	SG_SFIRE_MTFL2,
	SG_SFIRE_NUM
}sg_sfire_t;

typedef enum arena_modes_e
{
	ARENA_MODE_NONE = 0,
	ARENA_MODE_FFA,
	ARENA_MODE_DUEL
}arena_modes_t;

typedef enum flag_drop_modes_e
{
	FLAG_DROP_DEFAULT = 0,
	FLAG_DROP_DISABLED,
	FLAG_DROP_ENABLED,
	FLAG_DROP_PASS
}flag_drop_modes_t;

typedef struct tf_server_data_s {
    //int            toggleflags;
    //arena_modes_t  arena_mode;

    int            topcolor_check;

    float          cb_prematch_time;
    float          cb_ceasefire_time;

    float          clan_scores_dumped;
    int            deathmsg;
    int            already_chosen_map;
    int            last_id;
    //int            flagem_checked;
    int            cease_fire;
    int            botResupplyBetweenKills;
} tf_server_data_t;
