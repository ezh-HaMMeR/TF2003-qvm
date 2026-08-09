/*
 *  QWProgs-TF2003
 *  Copyright (C) 2004  [sd] angel
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
 *  $Id: g_utils.c,v 1.16 2006-11-29 23:19:23 AngelD Exp $
 */

#include "g_local.h"


int NUM_FOR_EDICT( gedict_t * e )
{
	int     b;

	b = ( byte * ) e - ( byte * ) g_edicts;
	b = b / sizeof( gedict_t );

	if ( b < 0 || b >= MAX_EDICTS )
		G_Error( "NUM_FOR_EDICT: bad pointer" );
	return b;
}

float bound( float a, float b, float c )
{
	return ( a >= c ? a : b < a ? a : b > c ? c : b);
}


float g_random(  )
{
	return ( rand(  ) & 0x7fff ) / ( ( float ) 0x8000 );
}

float crandom(  )
{
	return 2 * ( g_random(  ) - 0.5 );
}

int i_rnd( int from, int to )
{
	float r;

	if ( from >= to )
		return from;

	r = (int)(from + (1.0 + to - from) * g_random());

	return bound(from, r, to);
}


#ifdef idx64
// Should be "" but too many references in code simply checking for 0 to mean null string...
#define        FOFS_s(x) ((intptr_t)&(((gedict_t *)0)->s.v.x))
#define PR2SetStringFieldOffset(ent, field) \
       ent->s.v.field ## _ = NUM_FOR_EDICT(ent) * sizeof(gedict_t) + FOFS_s(field); \
       ent->s.v.field = 0;

#define PR2SetFuncFieldOffset(ent, field) \
       ent->s.v.field ## _ = NUM_FOR_EDICT(ent) * sizeof(gedict_t) + FOFS_s(field); \
       ent->s.v.field = (func_t) SUB_Null;
#endif

void initialise_spawned_ent(gedict_t* ent)
{
#ifdef idx64
       PR2SetStringFieldOffset(ent, classname);
       PR2SetStringFieldOffset(ent, model);
       PR2SetFuncFieldOffset(ent, touch);
       PR2SetFuncFieldOffset(ent, use);
       PR2SetFuncFieldOffset(ent, think);
       PR2SetFuncFieldOffset(ent, blocked);
       PR2SetStringFieldOffset(ent, weaponmodel);
       PR2SetStringFieldOffset(ent, netname);
       PR2SetStringFieldOffset(ent, target);
       PR2SetStringFieldOffset(ent, targetname);
       PR2SetStringFieldOffset(ent, message);
       PR2SetStringFieldOffset(ent, noise);
       PR2SetStringFieldOffset(ent, noise1);
       PR2SetStringFieldOffset(ent, noise2);
       PR2SetStringFieldOffset(ent, noise3);
#endif
}

gedict_t *spawn(  )
{
	gedict_t *t = &g_edicts[trap_spawn(  )];

	if ( !t || t == world )
		DebugTrap( "spawn return world\n" );
    initialise_spawned_ent(t);
	return t;
}

/*int rint(float f)
{
	if(f > 0)
		return (int)( f + 0.5);
	else
		return (int)( f - 0.5);

}
*/
void ent_remove( gedict_t * t )
{
	if ( !t || t == world )
		DebugTrap( "BUG BUG BUG remove world\n" );
	trap_remove( NUM_FOR_EDICT( t ) );
}



gedict_t *nextent( gedict_t * ent )
{
	int     entn;

	if ( !ent )
		G_Error( "nextent: NULL start\n" );
	entn = trap_nextent( NUM_FOR_EDICT( ent ) );
	if ( entn )
		return &g_edicts[entn];
	else
		return NULL;
}

gedict_t *find( gedict_t * start, int fieldoff, const char *str )
{
    return (gedict_t*) trap_find( start, fieldoff, str );
}


void normalize( vec3_t value, vec3_t newvalue )
{
	float   new;


	new = value[0] * value[0] + value[1] * value[1] + value[2] * value[2];
	new = sqrt( new );

	if ( new == 0 )
		value[0] = value[1] = value[2] = 0;
	else
	{
		new = 1 / new;
		newvalue[0] = value[0] * new;
		newvalue[1] = value[1] * new;
		newvalue[2] = value[2] * new;
	}

}
void aim( vec3_t ret )
{
	VectorCopy( g_globalvars.v_forward, ret );
}

char    null_str[] = "";

int streq( const char *s1, const  char *s2 )
{
	if ( !s1 )
		s1 = null_str;
	if ( !s2 )
		s2 = null_str;
	return ( !strcmp( s1, s2 ) );
}

int strneq( const char *s1, const char *s2 )
{
	if ( !s1 )
		s1 = null_str;
	if ( !s2 )
		s2 = null_str;

	return ( strcmp( s1, s2 ) );
}

int strnull( const char *s1 )
{
	return (!s1 || !*s1);
}

/*
=================
vlen

scalar vlen(vector)
=================
*/
float vlen( vec3_t value1 )
{
	float   new;


	new = value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2];
	new = sqrt( new );

	return new;
}

float vectoyaw( vec3_t value1 )
{
	float   yaw;


	if ( value1[1] == 0 && value1[0] == 0 )
		yaw = 0;
	else
	{
		yaw = ( int ) ( atan2( value1[1], value1[0] ) * 180 / M_PI );
		if ( yaw < 0 )
			yaw += 360;
	}

	return yaw;
}


void vectoangles( vec3_t value1, vec3_t ret )
{
	float   forward;
	float   yaw, pitch;


	if ( value1[1] == 0 && value1[0] == 0 )
	{
		yaw = 0;
		if ( value1[2] > 0 )
			pitch = 90;
		else
			pitch = 270;
	} else
	{
		yaw = ( int ) ( atan2( value1[1], value1[0] ) * 180 / M_PI );
		if ( yaw < 0 )
			yaw += 360;

		forward = sqrt( value1[0] * value1[0] + value1[1] * value1[1] );
		pitch = ( int ) ( atan2( value1[2], forward ) * 180 / M_PI );
		if ( pitch < 0 )
			pitch += 360;
	}


	ret[0] = pitch;
	ret[1] = yaw;
	ret[2] = 0;
}

/*
=================
PF_findradius

Returns a chain of entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
gedict_t *findradius( gedict_t * start, vec3_t org, float rad )
{
        return trap_findradius( start, org, rad);
}
#if 0 //odlversion
gedict_t *findradius( gedict_t * start, vec3_t org, float rad )
{
	gedict_t *ent;
	vec3_t  eorg;
	int     j;

	for ( ent = nextent( start ); ent; ent = nextent( ent ) )
	{
		if ( ent->s.v.solid == SOLID_NOT )
			continue;
		for ( j = 0; j < 3; j++ )
			eorg[j] = org[j] - ( ent->s.v.origin[j] + ( ent->s.v.mins[j] + ent->s.v.maxs[j] ) * 0.5 );
		if ( VectorLength( eorg ) > rad )
			continue;
		return ent;

	}
	return NULL;
}
#endif

/*
==============
changeyaw

This was a major timewaster in progs, so it was converted to C
==============
*/
void changeyaw( gedict_t * ent )
{
	float   ideal, current, move, speed;

	current = anglemod( ent->s.v.angles[1] );
	ideal = ent->s.v.ideal_yaw;
	speed = ent->s.v.yaw_speed;

	if ( current == ideal )
		return;
	move = ideal - current;
	if ( ideal > current )
	{
		if ( move >= 180 )
			move = move - 360;
	} else
	{
		if ( move <= -180 )
			move = move + 360;
	}
	if ( move > 0 )
	{
		if ( move > speed )
			move = speed;
	} else
	{
		if ( move < -speed )
			move = -speed;
	}

	ent->s.v.angles[1] = anglemod( current + move );
}

/*
==============
PF_makevectors

Writes new values for v_forward, v_up, and v_right based on angles
makevectors(vector)
==============
*/

#if 0 //old version
void makevectors( vec3_t vector )
{
         AngleVectors( vector, g_globalvars.v_forward, g_globalvars.v_right, g_globalvars.v_up );
}
#endif
void makevectors( vec3_t vector )
{
         trap_makevectors(vector);
}


/*
==============
print functions
==============
*/
//===================================================================
void G_dprintf( const char *fmt, ... )
{
	va_list         argptr;
	char text[1024];

	va_start( argptr, fmt );
	_vsnprintf( text, sizeof(text), fmt, argptr );
	va_end( argptr );

	trap_DPrintf( text );
}

void G_conprintf( const char *fmt, ... )
{
	va_list         argptr;
	char text[1024];

	va_start( argptr, fmt );
	_vsnprintf( text, sizeof(text), fmt, argptr );
	va_end( argptr );

	trap_conprint( text );
}

void G_Error( const char *fmt, ... )
{
	va_list         argptr;
	char text[1024];

	va_start( argptr, fmt );
	_vsnprintf( text, sizeof(text), fmt, argptr );
	va_end( argptr );

	trap_Error( text );
}

void G_sprint( gedict_t * ed, int level, const char *fmt, ... )
{
	va_list argptr;
	char text[1024];

	va_start( argptr, fmt );
	_vsnprintf( text, sizeof(text), fmt, argptr );
	va_end( argptr );

	trap_SPrint( NUM_FOR_EDICT( ed ), level, text, 0 );
}

void G_sprint_nodemo( gedict_t * ed, int level, const char *fmt, ... )
{
	va_list argptr;
	char text[1024];

	va_start( argptr, fmt );
	_vsnprintf( text, sizeof(text), fmt, argptr );
	va_end( argptr );

	/* The live recipient still sees the message, but MVDSV does not create
	 * a view-dependent dem_single copy for it. */
	trap_SPrint( NUM_FOR_EDICT( ed ), level, text, SPRINT_IGNOREINDEMO );
}

void G_bprint( int level, const char *fmt, ... )
{
	va_list argptr;
	char text[1024];

	va_start( argptr, fmt );
	_vsnprintf( text, sizeof(text), fmt, argptr );
	va_end( argptr );

	trap_BPrint( level, text, 0  );
}

void G_demoprint( int level, const char *fmt, ... )
{
	va_list argptr;
	char text[1024];

	va_start( argptr, fmt );
	_vsnprintf( text, sizeof(text), fmt, argptr );
	va_end( argptr );

	/* MVDSV records this as one dem_all message without sending a duplicate
	 * to live clients or printing it in the server console. */
	trap_BPrint( level, text, BPRINT_IGNORECLIENTS | BPRINT_IGNORECONSOLE );
}

void G_centerprint( gedict_t * ed, const char *fmt, ... )
{
	va_list argptr;
	char text[1024];

	va_start( argptr, fmt );
	_vsnprintf( text, sizeof(text), fmt, argptr );
	va_end( argptr );

	trap_CenterPrint( NUM_FOR_EDICT( ed ), text );
}

void localcmd( const char *fmt, ... )
{
	va_list argptr;
	char    text[1024];

	va_start( argptr, fmt );
	_vsnprintf( text, sizeof(text), fmt, argptr );
	va_end( argptr );

	trap_localcmd( text );
}

void stuffcmd( gedict_t * ed, const char *fmt, ... )
{
	va_list argptr;
	char    text[1024];

	va_start( argptr, fmt );
	_vsnprintf( text, sizeof(text), fmt, argptr );
	va_end( argptr );

	trap_stuffcmd( NUM_FOR_EDICT( ed ), text, 0 );
}


void setorigin( gedict_t * ed, float origin_x, float origin_y, float origin_z )
{
	trap_setorigin( NUM_FOR_EDICT( ed ), origin_x, origin_y, origin_z );
}

void setsize( gedict_t * ed, float min_x, float min_y, float min_z, float max_x, float max_y, float max_z )
{
	trap_setsize( NUM_FOR_EDICT( ed ), min_x, min_y, min_z, max_x, max_y, max_z );
}

void setmodel( gedict_t * ed, char *model )
{
	trap_setmodel( NUM_FOR_EDICT( ed ), model );
}

void sound( gedict_t * ed, int channel, char *samp, float vol, float att )
{
	trap_sound( NUM_FOR_EDICT( ed ), channel, samp, vol, att );
}

gedict_t *checkclient(  )
{
	return &g_edicts[trap_checkclient(  )];
}
void traceline( float v1_x, float v1_y, float v1_z, float v2_x, float v2_y, float v2_z, int nomonst, gedict_t * ed )
{
	trap_traceline( v1_x, v1_y, v1_z, v2_x, v2_y, v2_z, nomonst, NUM_FOR_EDICT( ed ) );
}

void TraceCapsule( float v1_x, float v1_y, float v1_z, float v2_x, float v2_y, float v2_z, int nomonst, gedict_t * ed ,
			float min_x, float min_y, float min_z, 
			float max_x, float max_y, float max_z)
{
	trap_TraceCapsule( v1_x, v1_y, v1_z, v2_x, v2_y, v2_z, nomonst, NUM_FOR_EDICT( ed ) ,
	min_x, min_y, min_z, max_x, max_y, max_z);
}

int droptofloor( gedict_t * ed )
{
	return trap_droptofloor( NUM_FOR_EDICT( ed ) );
}

int checkbottom( gedict_t * ed )
{
	return trap_checkbottom( NUM_FOR_EDICT( ed ) );
}

void makestatic( gedict_t * ed )
{
	trap_makestatic( NUM_FOR_EDICT( ed ) );
}

void setspawnparam( gedict_t * ed )
{
	trap_setspawnparam( NUM_FOR_EDICT( ed ) );
}

void logfrag( gedict_t * killer, gedict_t * killee )
{
	trap_logfrag( NUM_FOR_EDICT( killer ), NUM_FOR_EDICT( killee ) );
}

void infokey( gedict_t * ed, const char *key, char *valbuff, int sizebuff )
{
	trap_infokey( NUM_FOR_EDICT( ed ), key, valbuff, sizebuff );
}

void WriteEntity( int to, gedict_t * ed )
{
	trap_WriteEntity( to, NUM_FOR_EDICT( ed ) );
}

void disableupdates( gedict_t * ed, float time )
{
	trap_disableupdates( NUM_FOR_EDICT( ed ), time );
}

int walkmove( gedict_t * ed, float yaw, float dist )
{
	gedict_t*saveself,*saveother,*saveactivator;
	int retv;

	saveself	= self ;
	saveother	= other;
	saveactivator = activator;

	retv = trap_walkmove( NUM_FOR_EDICT( ed ), yaw,  dist );

	self 	= saveself;
	other	= saveother;
	activator= saveactivator;
	return retv;
}
float cvar( const char *var )
{
	if ( strnull( var ) )
		G_Error("cvar null");

	return trap_cvar ( var );
}

char *cvar_string( const char *var )
{
	static char		string[MAX_STRINGS][1024];
	static int		index = 0;

	index %= MAX_STRINGS;

	trap_cvar_string( var, string[index], sizeof( string[0] ) );

	return string[index++];
}

void cvar_set( const char *var, const char *val )
{
	if ( strnull( var ) || val == NULL )
		G_Error("cvar_set null");

	trap_cvar_set( var, val );
}

void cvar_fset( const char *var, float val )
{
	if ( strnull( var ) )
		G_Error("cvar_fset null");

	trap_cvar_set_float(var, val);
}

intptr_t setuserinfo(gedict_t* ed, const char* varname, const char* value, intptr_t flags) {
	return trap_SetUserInfo(NUM_FOR_EDICT(ed), varname, value, 0);
}

void StopDemoRecord()
{
    if (!strnull(cvar_string("serverdemo"))) {
        localcmd("sv_demostop\n");  // demo is recording, cancel before new one
    }
}
void StartDemoRecord(int init)
{
    static char date[64];
    extern float starttime;

    if (init) {
	    if (!QVMstrftime (date, sizeof (date), "%Y%m%d-%H%M%S", 0))
	       _snprintf (date, sizeof (date), "%d", (int)(starttime + i_rnd (0, 9999)));
	}

    //G_dprintf("Start demo record %f\n", cvar("demo_tmp_record"));
	if ( cvar( "demo_tmp_record" ) )
	{ 
        
        localcmd("sv_demoeasyrecord \"%s-%s-round%d\"\n", date, mapname, ad_roundnum + 1);
    }
}

int log2powerof2( unsigned int v )
{
    //static const unsigned int b[] = {0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000};
    register unsigned int r = (v & 0xAAAAAAAA) != 0;
    r |= ((v & 0xFFFF0000) != 0) << 4;
    r |= ((v & 0xFF00FF00) != 0) << 3;
    r |= ((v & 0xF0F0F0F0) != 0) << 2;
    r |= ((v & 0xCCCCCCCC) != 0) << 1;
    //G_sprint( self, 2, "log %d %d\n", v, r);

    return r;
}

void _think_func(  )
{
	self->s.v.nextthink = g_globalvars.time + 0.1;
    self->s.v.think = (func_t)_think_func;
    if( self->s.v.frame >= self->frame_info.end ){
        if( self->frame_info.last_func )
            self->frame_info.last_func();
        if( self->frame_info.last_think )
            self->s.v.think = (func_t)self->frame_info.last_think;
    }else{
        self->s.v.frame += 1;
        if( self->frame_info.frame_func )
            self->frame_info.frame_func();
    }
}

void set_think( gedict_t* e, int start, int end, th_die_func_t frame_func, th_die_func_t last_func, th_die_func_t last_think )
{
	e->s.v.nextthink = g_globalvars.time + 0.1;
    e->s.v.think = (func_t)_think_func;
    e->s.v.frame = start;
    e->frame_info.start = start;
    e->frame_info.end = end;
    e->frame_info.frame_func = frame_func;
    e->frame_info.last_func = last_func;
    e->frame_info.last_think = last_think;
    if( e->frame_info.frame_func )
    {
        gedict_t*saveself = self;
        self = e;
        e->frame_info.frame_func();
        self = saveself;
    }
}

void updateicons(gedict_t* ent, int iconnum) {
	trap_updateicons(NUM_FOR_EDICT(ent), iconnum);
}

void sendtfinfo_single(gedict_t* e, gedict_t* to, int idx, int val) {
	trap_updatetfinfo(UPDATETFINFO_SINGLE, NUM_FOR_EDICT(e), NUM_FOR_EDICT(to), idx, val);
}

void sendtfinfo_broadcast(gedict_t* e, int idx, int val) {
	trap_updatetfinfo(UPDATETFINFO_BROADCAST, NUM_FOR_EDICT(e), idx, val);
}

#if TF2003_DAMAGE_STATS_ENABLED
#define TFINFO_DAMAGE_UPDATE_INTERVAL 0.1
static float next_damage_stat_update;
#endif

gedict_t *G_NextPlayer( gedict_t *player )
{
	int client_no;

	if ( !player || player == world )
		client_no = 1;
	else
		client_no = NUM_FOR_EDICT( player ) + 1;

	for ( ; client_no <= MAX_CLIENTS; client_no++ )
	{
		player = &g_edicts[client_no];
		if ( !player->is_removed && streq( player->s.v.classname, "player" ) )
			return player;
	}

	return world;
}

gedict_t *G_NextSpectator( gedict_t *spectator )
{
	int client_no;

	if ( !spectator || spectator == world )
		client_no = 1;
	else
		client_no = NUM_FOR_EDICT( spectator ) + 1;

	for ( ; client_no <= MAX_CLIENTS; client_no++ )
	{
		spectator = &g_edicts[client_no];
		if ( !spectator->is_removed && !spectator->has_disconnected
		     && spectator->isSpectator
		     && streq( spectator->s.v.classname, "spectator" ) )
			return spectator;
	}

	return world;
}

#define TEAMMATE_STATUS_UPDATE_INTERVAL 1.0
static float next_teammate_status_update;

static qboolean IsActiveTeammateStatusPlayer( gedict_t *player )
{
	return player && player != world
		&& !player->is_removed && !player->has_disconnected
		&& !player->isSpectator
		&& streq( player->s.v.classname, "player" )
		&& player->team_no > 0;
}

static int ClampTeammateStatusValue( float value )
{
	if ( value < 0 )
		return 0;
	if ( value > 999 )
		return 999;
	return ( int ) value;
}

void ResetTeammateStatusUpdates(  )
{
	next_teammate_status_update = 0;
}

void FlushTeammateStatusUpdates(  )
{
	gedict_t *recipient;
	gedict_t *source;
	int source_slot;

	/* Team status uses reliable stufftext, so update it centrally at most once
	 * per second instead of multiplying it through every PlayerPreThink. */
	if ( g_globalvars.time < next_teammate_status_update )
		return;
	next_teammate_status_update = g_globalvars.time + TEAMMATE_STATUS_UPDATE_INTERVAL;

	for ( recipient = world; ( recipient = G_NextPlayer( recipient ) ) != world; )
	{
		if ( !IsActiveTeammateStatusPlayer( recipient ) )
			continue;

		for ( source = world; ( source = G_NextPlayer( source ) ) != world; )
		{
			if ( !IsActiveTeammateStatusPlayer( source ) )
				continue;

			/* Privacy boundary: use only the mod's authoritative real team number.
			 * Allied teams, userinfo, colors, skins and Spy disguise never qualify. */
			if ( source->team_no != recipient->team_no )
				continue;

			/* Client edicts occupy slots 1..MAX_CLIENTS; //tinfo is zero-based. */
			source_slot = NUM_FOR_EDICT( source ) - 1;
			stuffcmd( recipient,
				"//tinfo %d %d %d %d %d %d %d \"\" %d %d %d %d\n",
				source_slot,
				( int ) source->s.v.origin[0],
				( int ) source->s.v.origin[1],
				( int ) source->s.v.origin[2],
				ClampTeammateStatusValue( source->s.v.health ),
				ClampTeammateStatusValue( source->s.v.armorvalue ),
				( int ) source->s.v.items,
				ClampTeammateStatusValue( source->s.v.ammo_shells ),
				ClampTeammateStatusValue( source->s.v.ammo_nails ),
				ClampTeammateStatusValue( source->s.v.ammo_rockets ),
				ClampTeammateStatusValue( source->s.v.ammo_cells ) );
		}
	}
}

#if TF2003_DAMAGE_STATS_ENABLED
/* Optional damage-stat batching. Re-enable with the switch in progs.h. */
void SendDamageStatUpdate( gedict_t *player )
{
	if ( !player || player == world )
		return;

	sendtfinfo_broadcast( player, TFINFO_DAMAGE, player->damage );
	player->damage_update_pending = 0;
}

void ResetDamageStatUpdates(  )
{
	next_damage_stat_update = 0;
}

void FlushDamageStatUpdates(  )
{
	gedict_t *player;
	int client_no;

	/* Accumulate hits for 100 ms before crossing the QVM/engine boundary.
	 * Explicit sends used by resets and disconnects remain immediate. */
	if ( g_globalvars.time < next_damage_stat_update )
		return;
	next_damage_stat_update = g_globalvars.time + TFINFO_DAMAGE_UPDATE_INTERVAL;

	/* All possible client edicts are inside slots 1..MAX_CLIENTS. Iterating
	 * this fixed range avoids a trap_find syscall for every connected player. */
	for ( client_no = 1; client_no <= MAX_CLIENTS; client_no++ )
	{
		player = &g_edicts[client_no];
		if ( !player->damage_update_pending )
			continue;
		if ( player->is_removed || strneq( player->s.v.classname, "player" ) )
		{
			player->damage_update_pending = 0;
			continue;
		}

		SendDamageStatUpdate( player );
	}
}
#endif

void sendinittfinfo(gedict_t* ply) {
	gedict_t* te;
	for (te = world; (te = G_NextPlayer(te)) != world;) {
		sendtfinfo_single(te, ply, TFINFO_TEAM, te->team_no);
		sendtfinfo_single(te, ply, TFINFO_PLAYERCLASS, te->playerclass);
		sendtfinfo_single(te, ply, TFINFO_TOUCHES, te->touches);
		sendtfinfo_single(te, ply, TFINFO_CAPS, te->caps);
#if TF2003_DAMAGE_STATS_ENABLED
		sendtfinfo_single(te, ply, TFINFO_DAMAGE, te->damage);
#endif
  	}
}
