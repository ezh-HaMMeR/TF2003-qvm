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
 *  $Id: pyro.c,v 1.15 2006-11-29 23:19:23 AngelD Exp $
 */
#include "g_local.h"
#define FLAME_PLYRMAXTIME 45
#define CHAN_PYRO_BURN 5
#define PYRO_BURN_SOUND "ambience/fire1.wav"
#define PYRO_FLAME_MODEL "progs/flame2.mdl"
#define PYRO_STREAM_MODEL "progs/s_explod.spr"
void    NapalmGrenadeTouch(  );
void    NapalmGrenadeLaunch( vec3_t org, gedict_t * shooter );
void    Napalm_touch(  );
int     RemoveFlameFromQueue( int id_flame );
static int     num_world_flames = 0;
static float   pyro_flame_modelindex = 0;
static float   pyro_stream_modelindex = 0;

/* setmodel performs a VM-to-engine call and a precache lookup. Pyro creates
 * these two non-inline models frequently, so discover each index once per map
 * and populate subsequent fresh entities directly. Their first setorigin call
 * performs the required link. */
static void Pyro_SetCachedModel( gedict_t * entity, char *model, float *modelindex )
{
    if ( *modelindex == 0 )
    {
        setmodel( entity, model );
        *modelindex = entity->s.v.modelindex;
        return;
    }

    entity->s.v.model = model;
    entity->s.v.modelindex = *modelindex;
}

/* MVDSV's zero-length world trace marks every non-empty, non-solid BSP leaf
 * as trace_inwater.  Point contents returns that same leaf classification
 * without performing a full collision trace. */
static int Pyro_PointHasTraceInWater( const vec3_t origin )
{
    return trap_pointcontents( PASSVEC3( origin ) ) < CONTENT_SOLID;
}

static int FlameIsAttached( gedict_t * flame )
{
    return flame && ( streq( flame->flame_id, "1" ) || streq( flame->flame_id, "3" ) );
}

static int Pyro_IsPlayer( gedict_t * target )
{
    return target && target != world && streq( target->s.v.classname, "player" );
}

static void Pyro_StopPlayerBurnSound( gedict_t * target )
{
    if ( Pyro_IsPlayer( target ) )
        sound( target, CHAN_PYRO_BURN | CHAN_NO_PHS_ADD,
            PYRO_BURN_SOUND, 0, ATTN_NONE );
}

static void Pyro_ClearBurnState( gedict_t * target )
{
    if ( !target || target == world )
        return;

    target->numflames = 0;
    target->s.v.tfstate = ( int ) target->s.v.tfstate &
        ~( TFSTATE_BURNING | TFSTATE_MAX_FLAMES );
}

void Pyro_Reset(  )
{
    num_world_flames = 0;
    pyro_flame_modelindex = 0;
    pyro_stream_modelindex = 0;
}

//** different types of flames (decreasing priority)

// 1 : burning flames making light and damage (1 per players or monsters)
// 2 : exploding flames (grenade)
// 3 : burning flames (players, monsters)
// 4 : world flames (on ground)

// create a flame of a given type, maintaining the count for each type
gedict_t *FlameSpawn( int type, gedict_t * p_owner )
{
    if ( tf_data.cb_prematch_time > g_globalvars.time )
        return world;

    if ( type != 1 && !tfset(lan_mode) )
        return world;

    while ( num_world_flames >= FLAME_MAXWORLDNUM )
    {
        if ( !RemoveFlameFromQueue( type ) )
            return world;
    }
    newmis = spawn(  );
    if ( !newmis || newmis == world )
        return world;

    newmis->s.v.solid = SOLID_BBOX;
    Pyro_SetCachedModel( newmis, PYRO_FLAME_MODEL, &pyro_flame_modelindex );

    // to keep track of the number of each type of flames
    switch ( type )
    {
        case 1:
            newmis->s.v.movetype = MOVETYPE_FLYMISSILE;
            newmis->s.v.effects = 8;
            newmis->flame_id = "1";
            break;
        case 2:
            newmis->s.v.movetype = MOVETYPE_BOUNCE;
            newmis->flame_id = "2";
            newmis->s.v.frame = 1;
            break;
        case 3:
            newmis->s.v.movetype = MOVETYPE_FLYMISSILE;
            newmis->flame_id = "3";
            break;
        case 4:
            newmis->s.v.movetype = MOVETYPE_FLYMISSILE;
            newmis->flame_id = "4";
            newmis->s.v.frame = 1;
            break;
        default:
            dremove( newmis );
            return world;
    }
    newmis->s.v.owner = EDICT_TO_PROG( p_owner );
    num_world_flames = num_world_flames + 1;
    return newmis;
}

// destroy a given flame, maintaining counters and links in the queue
void FlameDestroy( gedict_t * this )
{
    gedict_t *enemy;

    if ( !this || this == world || this->is_removed || strnull( this->flame_id ) )
        return;

    if ( FlameIsAttached( this ) )
    {
        enemy = PROG_TO_EDICT( this->s.v.enemy );
        if ( enemy && enemy != world )
        {
            /* Non-player targets keep the sound on the master flame entity. */
            if ( this->s.v.effects == EF_DIMLIGHT && !Pyro_IsPlayer( enemy ) )
                sound( this, CHAN_VOICE | CHAN_NO_PHS_ADD,
                    PYRO_BURN_SOUND, 0, ATTN_NONE );

            if ( enemy->numflames > 0 )
            {
                enemy->numflames = enemy->numflames - 1;
                if ( enemy->numflames <= 0 )
                    Pyro_StopPlayerBurnSound( enemy );
            }
            if ( enemy->numflames < 4 )
                enemy->s.v.tfstate = ( int ) enemy->s.v.tfstate & ~TFSTATE_MAX_FLAMES;
            if ( enemy->numflames <= 0 )
                Pyro_ClearBurnState( enemy );
        }
    }

    if ( num_world_flames > 0 )
        num_world_flames = num_world_flames - 1;
    else
        num_world_flames = 0;

    // Stop queue searches from finding an entity pending engine removal.
    this->flame_id = NULL;
    dremove( this );
}

static void Pyro_RemoveAttachedFlames( gedict_t *target, const char *id )
{
    gedict_t *flame;
    gedict_t *next;

    flame = trap_find( world, FOFS( flame_id ), id );
    while ( flame )
    {
        /* Find the successor before FlameDestroy invalidates this entity. */
        next = trap_find( flame, FOFS( flame_id ), id );
        if ( PROG_TO_EDICT( flame->s.v.enemy ) == target )
            FlameDestroy( flame );
        flame = next;
    }
}

void Pyro_ExtinguishPlayer( gedict_t * target )
{
    int was_burning;

    if ( !target || target == world )
        return;

    was_burning = target->numflames > 0 ||
        ( ( int ) target->s.v.tfstate & TFSTATE_BURNING );
    if ( was_burning )
        Pyro_StopPlayerBurnSound( target );

    // Clear gameplay state first, then remove every attached visual entity.
    // FlameDestroy is idempotent and therefore safe for the current thinker too.
    Pyro_ClearBurnState( target );

    Pyro_RemoveAttachedFlames( target, "1" );
    Pyro_RemoveAttachedFlames( target, "3" );
}

int RemoveFlameFromQueue( int id_flame )
{
    gedict_t *tmp = NULL;
    const char *flame_id = NULL;

    if ( num_world_flames < FLAME_MAXWORLDNUM )
    {
        G_conprintf( "ERROR in RemoveFlameFromQueue\n" );
        return 0;
    }


    switch ( id_flame )
    {
        case 1:
            flame_id = "1";
            break;
        case 2:
            flame_id = "2";
            break;
        case 3:
            flame_id = "3";
            break;
        case 4:
            flame_id = "4";
            break;
    }
    if ( flame_id )
        tmp = trap_find( world, FOFS( flame_id ), flame_id );
    if ( !tmp )
    {
        G_conprintf( "\n\nRemoveFlameFromQueue():BOOM!\n" );
        return 0;
    }
    if ( id_flame == 1 && PROG_TO_EDICT( tmp->s.v.enemy ) != world )
        Pyro_ExtinguishPlayer( PROG_TO_EDICT( tmp->s.v.enemy ) );
    else
        FlameDestroy( tmp );
    return 1;
}

void Remove(  )
{
    FlameDestroy( self );
}

// function used by the flames spawned when the grenade explode : killed in water or when stopped
void NapalmGrenadeFollow(  )
{
    if ( Pyro_PointHasTraceInWater( self->s.v.origin ) )
    {
        sound( self, 2, "misc/vapeur2.wav", 1, 1 );
        FlameDestroy( self );
        return;
    }
    if ( VectorCompareF( self->s.v.velocity, 0, 0, 0 ) )
    {
        FlameDestroy( self );
        return;
    }
    self->s.v.nextthink = g_globalvars.time + 0.2;
}

void NapalmGrenadeTouch(  )
{
    sound( self, 1, "weapons/bounce.wav", 1, 1 );
    if ( VectorCompareF( self->s.v.velocity, 0, 0, 0 ) )
        SetVector( self->s.v.avelocity, 0, 0, 0 );
}

void NapalmGrenadeNetThink(  )
{
    gedict_t *head;

    self->s.v.nextthink = g_globalvars.time + 1;
    for ( head = world; (head = trap_findradius( head, self->s.v.origin, 180 )); )
    {
        if ( head->s.v.takedamage )
        {
            tf_data.deathmsg = DMSG_FLAME;
            TF_T_Damage( head, self, PROG_TO_EDICT( self->s.v.owner ), 20, TF_TD_NOTTEAM, TF_TD_FIRE );
            other = head;
            Napalm_touch(  );
        }
    }
    TempEffectCoord(  self->s.v.origin , TE_EXPLOSION );
    self->heat = self->heat + 1;
    if ( self->heat > 7 )
    {
        dremove( PROG_TO_EDICT( self->s.v.enemy ) );
        dremove( self );
    }
}

void NapalmGrenadeExplode(  )
{
    //      float   i;
    gedict_t *head;
    float   i;
    vec3_t  tmp;
    if( !tfset(lan_mode) )
    {
        sound( self, 0, "weapons/flmgrexp.wav", 1, 1 );
        if ( Pyro_PointHasTraceInWater( self->s.v.origin ) )
        {
            dremove( self );
            return;
        }
        // do an incendiary-cannon explosion instead
        self->s.v.effects = ( int ) self->s.v.effects | EF_DIMLIGHT;

        // don't do radius damage to the other, because all the damage
        // was done in the impact
        for ( head = world; (head = trap_findradius( head, self->s.v.origin, 180 )); )
        {
            if ( head->s.v.takedamage )
            {
                tf_data.deathmsg = DMSG_FLAME;
                TF_T_Damage( head, self, PROG_TO_EDICT( self->s.v.owner ), 40, TF_TD_NOTTEAM, TF_TD_FIRE );
                // set 'em on fire
                other = head;// i can't believe this works!
                Napalm_touch(  );
            }
        }
        TempEffectCoord(  self->s.v.origin , TE_EXPLOSION );
        head = spawn(  );
        head->s.v.think = ( func_t ) NapalmGrenadeNetThink;
        head->s.v.nextthink = g_globalvars.time + 1;
        head->heat = 0;
        VectorCopy( self->s.v.origin, head->s.v.origin );
        head->s.v.owner = self->s.v.owner;
        head->team_no = PROG_TO_EDICT( self->s.v.owner )->team_no;
        head->s.v.enemy = EDICT_TO_PROG( self );
        ///Napalm fix
        self->s.v.movetype = MOVETYPE_NONE;
        SetVector( self->s.v.velocity, 0, 0, 0 );
    }else
    {
        sound( self, 0, "weapons/flmgrexp.wav", 1, 1 );
        if ( Pyro_PointHasTraceInWater( self->s.v.origin ) )
        {
            dremove( self );
            return;
        }
        tf_data.deathmsg = DMSG_FLAME;
        T_RadiusDamage( self, PROG_TO_EDICT( self->s.v.owner ), 20, world );
        i = 0;
        VectorCopy( self->s.v.origin, tmp );
        tmp[2] += 5;
        while ( i < 15 )
        {
            NapalmGrenadeLaunch( tmp, PROG_TO_EDICT( self->s.v.owner ) );
            i = i + 1;
        }
        self->s.v.solid = SOLID_NOT;
        BecomeExplosion(  );
    }
}

//=========================================================================
// Launch a flame foe the grenade explosion
void NapalmGrenadeLaunch( vec3_t org, gedict_t * shooter )
{
    float   xdir;
    float   ydir;
    float   zdir;
    float   spin;

    xdir = 150 * g_random(  ) - 75;
    ydir = 150 * g_random(  ) - 75;
    zdir = 40 * g_random(  );
    newmis = FlameSpawn( 2, shooter );
    g_globalvars.newmis = EDICT_TO_PROG( newmis );

    if ( newmis == world )
        return;
    self->s.v.touch = ( func_t ) SUB_Null;
    newmis->s.v.classname = "fire";
    newmis->s.v.touch = ( func_t ) Napalm_touch;
    newmis->s.v.think = ( func_t ) NapalmGrenadeFollow;
    newmis->s.v.nextthink = g_globalvars.time + 0.1;
    newmis->s.v.enemy = shooter->s.v.owner;
    newmis->s.v.velocity[0] = xdir * 2;
    newmis->s.v.velocity[1] = ydir * 2;
    newmis->s.v.velocity[2] = zdir * 15;
    spin = ( int ) ( g_random(  ) * 10 / 2 );
    if ( spin <= 0 )
        SetVector( newmis->s.v.avelocity, 250, 300, 400 );
    if ( spin == 1 )
        SetVector( newmis->s.v.avelocity, 400, 250, 300 );
    if ( spin == 2 )
        SetVector( newmis->s.v.avelocity, 300, 400, 250 );
    if ( spin == 3 )
        SetVector( newmis->s.v.avelocity, 300, 300, 300 );
    if ( spin >= 4 )
        SetVector( newmis->s.v.avelocity, 400, 250, 400 );
    setorigin( newmis, PASSVEC3( org ) );
    setsize( newmis, 0, 0, 0, 0, 0, 0 );
}


void    OnPlayerFlame_touch(  );
void    FlameFollow(  );

/* The first attached-flame think performs the one-time physics transition.
 * Later 100 ms ticks can run the smaller steady-state callback directly. */
static void FlameFollowStart(  )
{
    self->s.v.solid = SOLID_NOT;
    self->s.v.movetype = MOVETYPE_NONE;
    self->s.v.think = ( func_t ) FlameFollow;
    FlameFollow(  );
}

void FlameFollow(  )
{
    vec3_t  dir;
    vec3_t  vtemp;
    vec3_t  boundsize;
    float   damage;
    gedict_t *enemy = PROG_TO_EDICT( self->s.v.enemy );

    if ( !enemy->numflames )
    {
        Pyro_ExtinguishPlayer( enemy );
        return;
    }
    if ( enemy->s.v.health < 1 )
    {
        tf_data.deathmsg = DMSG_FLAME;
        T_RadiusDamage( self, self, 10, self );
        Pyro_ExtinguishPlayer( enemy );
        return;
    }
    if ( ( enemy->armorclass & AT_SAVEFIRE ) && enemy->s.v.armorvalue > 0 )
        self->s.v.health = 0;

    if ( enemy->s.v.tfstate & TFSTATE_MAX_FLAMES )
    {
        self->s.v.health = FLAME_PLYRMAXTIME;
        enemy->s.v.tfstate = enemy->s.v.tfstate - ( enemy->s.v.tfstate & TFSTATE_MAX_FLAMES );
    }
    if ( self->s.v.health < 1 )
    {
        // only remove the flame if it is not the master flame, or if it is the last flame
        if ( self->s.v.effects != EF_DIMLIGHT || enemy->numflames <= 1 )
        {
            Pyro_ExtinguishPlayer( enemy );
            return;
        }
    }
    self->s.v.health = self->s.v.health - 1;
    if ( DotProduct( enemy->s.v.velocity, enemy->s.v.velocity ) < 2500 )
    {
        VectorCopy( enemy->s.v.absmin, vtemp );
        VectorCopy( enemy->s.v.size, boundsize );
        dir[0] = g_random(  ) * boundsize[0] / 2 + boundsize[0] / 4;
        dir[1] = g_random(  ) * boundsize[1] / 2 + boundsize[1] / 4;
        dir[2] = g_random(  ) * boundsize[2] / 3 + boundsize[2] / 2;
        VectorAdd( vtemp, dir, vtemp );
        setorigin( self, PASSVEC3( vtemp ) );
        if ( self->s.v.modelindex == 0 )
            Pyro_SetCachedModel( self, PYRO_FLAME_MODEL, &pyro_flame_modelindex );
    } else
    {
        if ( self->s.v.modelindex != 0 )
        {
            self->s.v.model = "";
            self->s.v.modelindex = 0;
        }
    }
    if ( enemy->s.v.waterlevel > 1 )
    {
        sound( self, 2, "misc/vapeur2.wav", 1, 1 );
        Pyro_ExtinguishPlayer( enemy );
        return;
    }
    self->s.v.nextthink = g_globalvars.time + 0.1;
    if ( self->s.v.effects == EF_DIMLIGHT && self->heat >= 3 )
    {
        damage = enemy->numflames * 0.3 * 3;
        if ( damage < 1 )
            damage = 1;
        self->heat = 1;
        tf_data.deathmsg = DMSG_FLAME;
        TF_T_Damage( enemy, self, PROG_TO_EDICT( self->s.v.owner ), damage, TF_TD_NOTTEAM | TF_TD_MOREKICK, TF_TD_FIRE );
    } else
    {
        if ( self->s.v.effects == EF_DIMLIGHT )
            self->heat = self->heat + 1;
    }
}

static gedict_t* spawnFlameOnPlayer( gedict_t*self, gedict_t*other, int zdelta)
{
    gedict_t *flame;
    gedict_t *owner;
    int is_player;
    int was_burning;
    vec3_t  vtemp;

    if ( tf_data.cb_prematch_time > g_globalvars.time )
        return NULL;
    if ( other->numflames >= 4 )
    {
        other->s.v.tfstate = other->s.v.tfstate | TFSTATE_MAX_FLAMES;
        return NULL;
    }
    if ( ( other->armorclass & AT_SAVEFIRE ) && other->s.v.armorvalue > 0 )
        return NULL;

    owner = PROG_TO_EDICT( self->s.v.owner );
    is_player = streq( other->s.v.classname, "player" );
    was_burning = other->numflames > 0;
    if ( is_player )
    {
        if ( ( teamplay & TEAMPLAY_NOEXPLOSIVE ) && other->team_no > 0
                && other->team_no == owner->team_no )
            return NULL;
    }
    if ( other->numflames < 1 )
    {
        flame = FlameSpawn( 1, other );
    } else
        flame = FlameSpawn( 3, other );

    if ( !flame || flame == world )
        return NULL;

    flame->s.v.classname = "fire";
    other->numflames = other->numflames + 1;
    VectorCopy( other->s.v.velocity, flame->s.v.velocity );
    flame->s.v.enemy = EDICT_TO_PROG( other );
    flame->s.v.touch = ( func_t ) OnPlayerFlame_touch;
    flame->s.v.owner = self->s.v.owner;
    flame->s.v.think = ( func_t ) FlameFollowStart;
    flame->s.v.nextthink = g_globalvars.time + 0.1;

    flame->s.v.health = FLAME_PLYRMAXTIME;
    VectorCopy( self->s.v.origin, vtemp );
    vtemp[2] += zdelta;
    setorigin( flame, PASSVEC3( vtemp ) );

    if ( !was_burning )
    {
        if ( is_player )
            sound( other, CHAN_PYRO_BURN, PYRO_BURN_SOUND, 1, ATTN_NORM );
        else
            sound( flame, CHAN_VOICE, PYRO_BURN_SOUND, 1, ATTN_NORM );
    }

    if ( is_player )
    {
        other->s.v.tfstate = ( int ) other->s.v.tfstate | TFSTATE_BURNING;
        if ( !was_burning )
        {
            CenterPrint( other, "You are on fire!\n" );
            stuffcmd( other, "bf\n" );
        }
    }
    return flame;
}

// OnPlayerflame : no damage if enemy not dead, spawn flames if touched
void OnPlayerFlame_touch(  )
{
	if ( other != world && other->s.v.health > 0 && other != PROG_TO_EDICT( self->s.v.enemy ) )
        spawnFlameOnPlayer( self, other, 0);
}


void    WorldFlame_touch(  );

void QW_Flame_ResetTouch(  )
{
    self->s.v.touch = ( func_t ) WorldFlame_touch;
    self->s.v.think = ( func_t ) Remove;
    self->s.v.nextthink = self->heat;
}
// worldflame : lot of damage, spawn flames if touched
void WorldFlame_touch(  )
{
	gedict_t *flame;

	tf_data.deathmsg = DMSG_FLAME;
	TF_T_Damage( other, self, PROG_TO_EDICT( self->s.v.enemy ), 10, TF_TD_NOTTEAM | TF_TD_MOREKICK, TF_TD_FIRE );
	self->s.v.touch = ( func_t ) SUB_Null;
	if ( self->heat > g_globalvars.time + 1 )
	{
		self->s.v.think = ( func_t ) QW_Flame_ResetTouch;
		self->s.v.nextthink = g_globalvars.time + 1;
	}

	if ( other != world && other->s.v.solid != SOLID_TRIGGER && other->s.v.health > 0 )
    {
        if(( flame = spawnFlameOnPlayer( self, other, 10)) )
        {
            flame->s.v.nextthink = g_globalvars.time + 0.15;
            flame->s.v.health = 0;
        }
    }
}

static void worldFlame_spawn( gedict_t* self, int time )
{
    vec3_t  vtemp;
    gedict_t *flame;

    flame = FlameSpawn( 4, other );
    if ( flame != world )
    {
        flame->s.v.touch = ( func_t ) WorldFlame_touch;
        flame->s.v.classname = "fire";
        VectorCopy( self->s.v.origin, vtemp );
        vtemp[2] += 10;
        setorigin( flame, PASSVEC3( vtemp ) );
        flame->s.v.nextthink = g_globalvars.time + time;
        flame->heat = flame->s.v.nextthink;
        flame->s.v.think = ( func_t ) Remove;
        flame->s.v.enemy = self->s.v.owner;
    }
}

// first touch : direct touch with the flamer stream or flame from grenade
void Flamer_stream_touch(  )
{
	vec3_t  vtemp;
	int pos;
    
	VectorCopy( self->s.v.origin , vtemp);
	vtemp[2]++;
	pos = trap_pointcontents( PASSVEC3( vtemp));

	if( pos == CONTENT_WATER || pos == CONTENT_SLIME )
	{
		dremove(self);
		return;
	}

	if ( streq( other->s.v.classname, "fire" ) )
		return;
	if ( other != world )
    {
        if( other->s.v.takedamage == DAMAGE_AIM && other->s.v.health > 0 )
        {
            tf_data.deathmsg = DMSG_FLAME;
            TF_T_Damage( other, self, PROG_TO_EDICT( self->s.v.owner ), 10, TF_TD_NOTTEAM | TF_TD_MOREKICK, TF_TD_FIRE );
            spawnFlameOnPlayer( self, other, 0 );
        }
	} else
	{
		if ( g_random(  ) < 0.3 || pos != CONTENT_EMPTY )
		{
			ent_remove( self );
//                        SetVector( self->s.v.velocity, 0, 0, 0 );
			return;
		}

        worldFlame_spawn( self, 8 );

		if(!tfset(pyrotype))
			ent_remove( self );
	}
}

void Napalm_touch(  )
{
	if ( streq( other->s.v.classname, "fire" ) )
		return;
	if ( other != world )
    {
        if( other->s.v.takedamage == DAMAGE_AIM && other->s.v.health > 0) 
        {
            tf_data.deathmsg = DMSG_FLAME;
            TF_T_Damage( other, self, PROG_TO_EDICT( self->s.v.owner ), 6, TF_TD_NOTTEAM | TF_TD_MOREKICK, TF_TD_FIRE );
            spawnFlameOnPlayer( self, other, 0 );
        }
	} else
	{
		if ( trap_pointcontents( PASSVEC3( self->s.v.origin ) + 1 ) != -1 )
		{
			FlameDestroy( self );
			return;
		}
        worldFlame_spawn( self, 20 );
		FlameDestroy( self );
	}
}

// Slightly varied version of DEATHBUBBLES
void NewBubbles( float num_bubbles, vec3_t bub_origin )
{
	gedict_t *bubble_spawner;

	bubble_spawner = spawn(  );
	setorigin( bubble_spawner, PASSVEC3( bub_origin ) );
	bubble_spawner->s.v.movetype = MOVETYPE_NONE;
	bubble_spawner->s.v.solid = SOLID_NOT;
	bubble_spawner->s.v.nextthink = g_globalvars.time + 0.1;

	if ( streq( self->s.v.classname, "player" ) )
		bubble_spawner->s.v.owner = EDICT_TO_PROG( self );
	else
		bubble_spawner->s.v.owner = self->s.v.enemy;

	bubble_spawner->s.v.think = ( func_t ) DeathBubblesSpawn;
	bubble_spawner->bubble_count = num_bubbles;

	return;
}

/*void    s_explode1(  );
void    s_explode2(  );
void    s_explode3(  );
void    s_explode4(  );
void    s_explode5(  );
void    s_explode6(  );*/

static int FlameStreamRemoveInWater(  )
{
	if ( !Pyro_PointHasTraceInWater( self->s.v.origin ) )
		return 0;

	sound( self, 2, "misc/vapeur2.wav", 1, 1 );
	dremove( self );
	return 1;
}

static void FlameStreamThink(  )
{
	self->s.v.nextthink = g_globalvars.time + 0.1;

	if ( self->s.v.frame >= 5 )
	{
		if ( FlameStreamRemoveInWater(  ) )
			return;
		self->s.v.think = ( func_t ) SUB_Remove;
		return;
	}

	self->s.v.frame = self->s.v.frame + 1;
	FlameStreamRemoveInWater(  );
}

static void FlameStreamStart(  )
{
	/* Keep the exact set_think(0, 5, ...) schedule without its generic
	 * callback and frame_info dispatch overhead. */
	self->s.v.frame = 0;
	self->s.v.think = ( func_t ) FlameStreamThink;
	self->s.v.nextthink = g_globalvars.time + 0.1;
	FlameStreamRemoveInWater(  );
}
void W_FireFlame(  )
{
	gedict_t *flame;
	float   rn;
	vec3_t  vtemp;

	if ( self->s.v.waterlevel > 2 )
	{
		trap_makevectors( self->s.v.v_angle );
		VectorScale( g_globalvars.v_forward, 64, vtemp );
		VectorAdd( self->s.v.origin, vtemp, vtemp );
		NewBubbles( 2, vtemp );
		rn = g_random(  );
		if ( rn < 0.5 )
			sound( self, 1, "misc/water1.wav", 1, 1 );
		else
			sound( self, 1, "misc/water2.wav", 1, 1 );
		return;
	}
	if ( !tg_data.unlimit_ammo )
		self->s.v.currentammo = --( self->s.v.ammo_cells );

	sound( self, 0, "weapons/flmfire2.wav", 1, 1 );

	flame = spawn(  );
	flame->s.v.owner = EDICT_TO_PROG( self );
	flame->s.v.movetype = MOVETYPE_FLYMISSILE;
	flame->s.v.solid = SOLID_BBOX;
	flame->s.v.classname = "flamerflame";

	trap_makevectors( self->s.v.v_angle );
	VectorCopy( g_globalvars.v_forward, flame->s.v.velocity );
	VectorScale( flame->s.v.velocity, 600, flame->s.v.velocity );

	flame->s.v.touch = ( func_t ) Flamer_stream_touch;
	flame->s.v.think = ( func_t ) FlameStreamStart;
	flame->s.v.effects = EF_DIMLIGHT;
	flame->s.v.nextthink = g_globalvars.time + 0.15;

	Pyro_SetCachedModel( flame, PYRO_STREAM_MODEL, &pyro_stream_modelindex );

	VectorScale( g_globalvars.v_forward, 16, vtemp );
	VectorAdd( self->s.v.origin, vtemp, vtemp );
	vtemp[2] += 16;
	setorigin( flame, PASSVEC3( vtemp ) );
}

/*======================
Touch function for incendiary cannon rockets
======================*/
void T_IncendiaryTouch(  )
{
	float   damg;

//      float   points;
	gedict_t *head, *owner;
	vec3_t  vtemp;

	owner = PROG_TO_EDICT( self->s.v.owner );
	if ( other == owner ) // don't explode on owner
		return;

	if ( trap_pointcontents( PASSVEC3( self->s.v.origin ) ) == -6 )
	{
		ent_remove( self );
		return;
	}

	self->s.v.effects = ( int ) self->s.v.effects | 4;
	damg = 30 + g_random(  ) * 20;

	if ( other->s.v.health )
	{
		tf_data.deathmsg = DMSG_FLAME;
		TF_T_Damage( other, self, owner, damg, TF_TD_NOTTEAM, TF_TD_FIRE );
	}

	// don't do radius damage to the other, because all the damage
	// was done in the impact
	for ( head = world; (head = trap_findradius( head, self->s.v.origin, 180 )); )
	{
		if ( head->s.v.takedamage )
		{
			traceline( PASSVEC3( self->s.v.origin ), PASSVEC3( head->s.v.origin ), 1, self );
			VectorSubtract( self->s.v.origin, head->s.v.origin, vtemp );
			if ( g_globalvars.trace_fraction == 1
			     || DotProduct( vtemp, vtemp ) <= 4096 )
			{
				tf_data.deathmsg = DMSG_FLAME;
				TF_T_Damage( head, self, owner, 10, TF_TD_NOTTEAM, TF_TD_FIRE );
				other = head;
				if( !g_globalvars.trace_inwater)
					Napalm_touch(  );
			}
		}
	}
	normalize( self->s.v.velocity, vtemp );
	VectorScale( vtemp, 8, vtemp );
	VectorSubtract( self->s.v.origin, vtemp, self->s.v.origin );
	TempEffectCoord(  self->s.v.origin , TE_EXPLOSION );
	dremove( self );
}

/*
================
W_FireIncendiaryCannon
================
*/
void W_FireIncendiaryCannon(  )
{
	vec3_t  vtemp;

	if ( self->s.v.ammo_rockets < 3 )
		return;
	if ( !tg_data.unlimit_ammo )
		self->s.v.currentammo = self->s.v.ammo_rockets = self->s.v.ammo_rockets - 3;

	sound( self, 1, "weapons/sgun1.wav", 1, 1 );
	KickPlayer( -3, self );
	newmis = spawn(  );

	g_globalvars.newmis = EDICT_TO_PROG( newmis );
	newmis->s.v.owner = EDICT_TO_PROG( self );
	newmis->s.v.movetype = MOVETYPE_FLYMISSILE;
	newmis->s.v.solid = SOLID_BBOX;

	trap_makevectors( self->s.v.v_angle );
	aim( newmis->s.v.velocity );
	VectorScale( newmis->s.v.velocity, 600, newmis->s.v.velocity );

	vectoangles( newmis->s.v.velocity, newmis->s.v.angles );

	newmis->s.v.classname = "rocket";
	newmis->s.v.touch = ( func_t ) T_IncendiaryTouch;
	newmis->s.v.nextthink = g_globalvars.time + 5;
	newmis->s.v.think = ( func_t ) SUB_Remove;
	newmis->s.v.weapon = DMSG_INCENDIARY;

	setmodel( newmis, "progs/missile.mdl" );
	setsize( newmis, 0, 0, 0, 0, 0, 0 );

	VectorScale( g_globalvars.v_forward, 8, vtemp );
	VectorAdd( vtemp, self->s.v.origin, vtemp );
	vtemp[2] += 16;
	setorigin( newmis, PASSVEC3( vtemp ) );
}

//=========================================================================
// Incendiary cannon selection function
void TeamFortress_IncendiaryCannon(  )
{
	if ( !( self->weapons_carried & WEAP_INCENDIARY ) )
		return;
	if ( self->s.v.ammo_rockets < 3 )
	{
		G_sprint( self, 2, "not enough ammo.\n" );
		return;
	}
	self->current_weapon = WEAP_INCENDIARY;
    self->s.v.currentclip = GetClipSize(self);
	W_SetCurrentAmmo(  );
}

// Flamethrower selection function
void TeamFortress_FlameThrower(  )
{
	if ( !( self->weapons_carried & WEAP_FLAMETHROWER ) )
		return;
	if ( self->s.v.ammo_cells < 1 )
	{
		G_sprint( self, 2, "not enough ammo.\n" );
		return;
	}
	self->current_weapon = WEAP_FLAMETHROWER;
    self->s.v.currentclip = GetClipSize(self);
	W_SetCurrentAmmo(  );
}
