/*
 *  QWProgs-TF2003
 *  Copyright (C) 2004  [sd] angel
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 */

// vote.q: voting functions and the optional active-vote centerprint
#include "g_local.h"

#define VOTE_MAP_NAME_SIZE   32

int Vote_ChangeMap_Init( void );
void Vote_ChangeMap_Run( void );
int Vote_Admin_Init( void );
void Vote_Admin_Run( void );
int Vote_Timelimit_Init( void );
void Vote_Timelimit_Run( void );
int Vote_Map_Init( void );
void Vote_Map_Run( void );
int Vote_Kick_Init( void );
void Vote_Kick_Run( void );

const vote_t votes[] =
{
    {"changemap", Vote_ChangeMap_Init, Vote_ChangeMap_Run, 60, 3},
    {"admin", Vote_Admin_Init, Vote_Admin_Run, 60, 3},
    {"timelimit", Vote_Timelimit_Init, Vote_Timelimit_Run, 60, 3},
    {"map", Vote_Map_Init, Vote_Map_Run, 60, 3},
    {"kick", Vote_Kick_Init, Vote_Kick_Run, 60, 3},
    {NULL}
};

static int k_vote;
int current_vote = -1;

static float elect_percentage;
static int elect_level;
static gedict_t *elect_player;
static int vote_timelimit;
static char vote_mapname[VOTE_MAP_NAME_SIZE];
static int vote_kick_userid = -1;
static char vote_kick_name[32];
static char vote_initiator[32];
static char vote_description[64];
static float vote_end_time;

void NextLevel( void );

static void Vote_SanitizeText( const char *source, char *dest, int dest_size )
{
    int i;
    unsigned char ch;

    if ( dest_size < 1 )
        return;

    for ( i = 0; i < dest_size - 1 && source && source[i]; i++ )
    {
        ch = (unsigned char)source[i];
        /* Quake names use the full 8-bit character set.  Centerprint is data,
         * not stufftext, so preserve those glyphs and strip only line breaks. */
        dest[i] = ( ch == '\n' || ch == '\r' ) ? ' ' : (char)ch;
    }
    dest[i] = 0;
}

static void Vote_SetDetails( const char *description )
{
    Vote_SanitizeText( self->s.v.netname, vote_initiator, sizeof( vote_initiator ) );
    Vote_SanitizeText( description, vote_description, sizeof( vote_description ) );
}

static gedict_t *Vote_FindPlayerByUserid( int userid )
{
    gedict_t *player;

    for ( player = world; ( player = G_NextPlayer( player ) ) != world; )
    {
        if ( player->isBot || !player->s.v.netname[0] )
            continue;
        if ( GetInfokeyInt( player, "*userid", NULL, -1 ) == userid )
            return player;
    }
    return world;
}

int CountPlayers( void )
{
    gedict_t *player;
    int num = 0;

    for ( player = world; ( player = G_NextPlayer( player ) ) != world; )
    {
        if ( player->s.v.netname[0] && !player->isBot )
            num++;
    }
    return num;
}

static int Vote_RequiredVotes( void )
{
    int required;

    required = (int)( CountPlayers() * elect_percentage / 100 ) + 1;
    if ( required < 1 )
        required = 1;
    if ( required > MAX_CLIENTS )
        required = MAX_CLIENTS;
    return required;
}

static void Vote_CloseMenus( void )
{
    gedict_t *player;

    for ( player = world; ( player = G_NextPlayer( player ) ) != world; )
    {
        if ( player->current_menu != VOTE_MENU_ACTIVE )
            continue;

        if ( !player->StatusBarSize )
            CenterPrint( player, "\n" );
        else
            player->StatusRefreshTime = g_globalvars.time + 0.1;
        player->menu_count = MENU_REFRESH_RATE;
        player->current_menu = MENU_DEFAULT;
    }
}

static void Vote_OpenActiveMenus( void )
{
    gedict_t *player;

    for ( player = world; ( player = G_NextPlayer( player ) ) != world; )
    {
        if ( !player->newvote || player->isBot )
            continue;
        player->current_menu = VOTE_MENU_ACTIVE;
        player->menu_count = MENU_REFRESH_RATE;
    }
}

void _clearVote( void )
{
    gedict_t *player;
    gedict_t *guard;

    k_vote = 0;
    current_vote = -1;
    vote_end_time = 0;
    for ( player = world; ( player = G_NextPlayer( player ) ) != world; )
        player->k_voted = 0;

    Vote_CloseMenus();

    guard = trap_find( world, FOFS( s.v.classname ), "voteguard" );
    if ( guard )
    {
        guard->s.v.classname = "";
        dremove( guard );
    }
}

int _checkVote( void )
{
    int needed_votes;

    needed_votes = Vote_RequiredVotes() - k_vote;
    if ( needed_votes < 1 )
        return 0;

    if ( needed_votes > 1 )
        G_bprint( 3, "%d more votes needed\n", needed_votes );
    else
        G_bprint( 3, "%d more vote needed\n", needed_votes );
    return needed_votes;
}

int _addVote( void )
{
    self->k_voted = 1;
    if ( k_vote > 0 )
        G_bprint( 3, "%s gives his vote\n", self->s.v.netname );
    k_vote++;

    if ( _checkVote() < 1 )
    {
        _clearVote();
        return 1;
    }
    return 0;
}

void _subVote( void )
{
    if ( !k_vote || !self->k_voted )
        return;

    G_bprint( 3, "%s withdraws his vote\n", self->s.v.netname );
    self->k_voted = 0;
    k_vote--;
    if ( k_vote < 1 )
    {
        G_bprint( 3, "Voting is closed\n" );
        _clearVote();
        return;
    }
    _checkVote();
}

void VoteThink( void )
{
    G_bprint( 3, "The voting has timed out.\n" );
    _clearVote();
}

void _startVote( void )
{
    gedict_t *voteguard;

    vote_end_time = g_globalvars.time + votes[current_vote].timeout;
    voteguard = spawn();
    voteguard->s.v.owner = EDICT_TO_PROG( world );
    voteguard->s.v.classname = "voteguard";
    voteguard->s.v.think = (func_t)VoteThink;
    voteguard->s.v.nextthink = vote_end_time;
}

int Vote_ChangeMap_Init( void )
{
    G_bprint( 3, "%s votes for mapchange\n", self->s.v.netname );
    Vote_SetDetails( "changemap" );
    return 1;
}

void Vote_ChangeMap_Run( void )
{
    G_bprint( 3, "Map changed by majority vote\n" );
    NextLevel();
}

static int Vote_SetTimelimit( int value )
{
    char description[40];

    if ( value < 5 || value > 60 )
    {
        G_sprint( self, 3, "usage cmd vote timelimit <5-60>\n" );
        return 0;
    }

    vote_timelimit = value;
    _snprintf( description, sizeof( description ), "timelimit %d", vote_timelimit );
    Vote_SetDetails( description );
    G_bprint( 3, "%s votes for timelimit %d\n", self->s.v.netname, vote_timelimit );
    return 1;
}

int Vote_Timelimit_Init( void )
{
    char value[10];

    if ( trap_CmdArgc() != 3 )
    {
        G_sprint( self, 3, "usage cmd vote timelimit <5-60>\n" );
        return 0;
    }
    trap_CmdArgv( 2, value, sizeof( value ) );
    return Vote_SetTimelimit( atoi( value ) );
}

void Vote_Timelimit_Run( void )
{
    G_bprint( 3, "Timelimit changed to %d\n", vote_timelimit );
    localcmd( "timelimit \"%d\"\n", vote_timelimit );
}

static qboolean Vote_IsSafeMapName( const char *name )
{
    int i;
    char ch;

    if ( !name || !name[0] )
        return false;
    for ( i = 0; name[i]; i++ )
    {
        ch = name[i];
        if ( i >= VOTE_MAP_NAME_SIZE - 1 )
            return false;
        if ( !( ( ch >= '0' && ch <= '9' )
                || ( ch >= 'a' && ch <= 'z' )
                || ( ch >= 'A' && ch <= 'Z' ) || ch == '_' ) )
            return false;
    }
    return true;
}

static int Vote_SetMap( const char *name )
{
    char path[64];
    char description[64];
    fileHandle_t handle = 0;
    int length;

    if ( !Vote_IsSafeMapName( name ) )
    {
        G_sprint( self, 3, "usage cmd vote map <mapname>\n" );
        return 0;
    }

    _snprintf( path, sizeof( path ), "maps/%s.bsp", name );
    length = trap_FS_OpenFile( path, &handle, FS_READ_BIN );
    if ( length < 0 )
    {
        G_sprint( self, 3, "map not found\n" );
        return 0;
    }
    trap_FS_CloseFile( handle );

    Q_strncpyz( vote_mapname, name, sizeof( vote_mapname ) );
    _snprintf( description, sizeof( description ), "map %s", vote_mapname );
    Vote_SetDetails( description );
    G_bprint( 3, "%s votes for map %s\n", self->s.v.netname, vote_mapname );
    return 1;
}

int Vote_Map_Init( void )
{
    char value[VOTE_MAP_NAME_SIZE];

    if ( trap_CmdArgc() != 3 )
    {
        G_sprint( self, 3, "usage cmd vote map <mapname>\n" );
        return 0;
    }
    trap_CmdArgv( 2, value, sizeof( value ) );
    return Vote_SetMap( value );
}

void Vote_Map_Run( void )
{
    G_bprint( 3, "Map changed by majority vote\n" );
    localcmd( "map \"%s\"\n", vote_mapname );
}

int Vote_Admin_Init( void )
{
    elect_percentage = GetSVInfokeyInt( "electpercentage", NULL, 50 );
    elect_level = GetSVInfokeyInt( "electlevel", NULL, 1 );
    elect_player = self;
    if ( !elect_percentage || !elect_level )
    {
        G_sprint( self, 2, "Admin election disabled\n" );
        return 0;
    }
    if ( self->is_admin >= elect_level )
    {
        G_sprint( self, 2, "You are already an admin\n" );
        return 0;
    }

    Vote_SetDetails( "admin" );
    return 1;
}

void Vote_Admin_Run( void )
{
    G_bprint( 2, "%s " _g _a _i _n " " _a _d _m _i _n " " _r _i _g _h _t _s "!\n",
              elect_player->s.v.netname );
    G_sprint( elect_player, 2,
              "Type " _c _m _d " " _a _d _m _i _n " for admin commands.\n" );
    elect_level = GetSVInfokeyInt( "electlevel", NULL, 1 );
    elect_player->is_admin = elect_level;
}

static int Vote_SetKickTarget( gedict_t *target )
{
    char description[64];

    if ( !target || target == world || target == self || target->isBot
         || !target->s.v.netname[0] )
    {
        G_sprint( self, 3, "Invalid kick target\n" );
        return 0;
    }

    vote_kick_userid = GetInfokeyInt( target, "*userid", NULL, -1 );
    if ( vote_kick_userid < 0 )
    {
        G_sprint( self, 3, "Invalid kick target\n" );
        return 0;
    }

    Vote_SanitizeText( target->s.v.netname, vote_kick_name, sizeof( vote_kick_name ) );
    _snprintf( description, sizeof( description ), "kick %s", vote_kick_name );
    Vote_SetDetails( description );
    G_bprint( 3, "%s votes to kick %s\n", self->s.v.netname, target->s.v.netname );
    return 1;
}

int Vote_Kick_Init( void )
{
    char value[16];
    gedict_t *target;

    if ( trap_CmdArgc() != 3 )
    {
        G_sprint( self, 3, "usage cmd vote kick <userid>\n" );
        return 0;
    }
    trap_CmdArgv( 2, value, sizeof( value ) );
    target = Vote_FindPlayerByUserid( atoi( value ) );
    return Vote_SetKickTarget( target );
}

void Vote_Kick_Run( void )
{
    gedict_t *target;
    gedict_t *saved_self;

    target = Vote_FindPlayerByUserid( vote_kick_userid );
    if ( target == world )
    {
        G_bprint( 3, "Kick vote cancelled: player is no longer connected\n" );
        return;
    }

    G_bprint( 3, "%s was kicked by majority vote\n", target->s.v.netname );
    saved_self = self;
    self = target;
    KickCheater( target );
    self = saved_self;
}

static void Vote_CommitStart( int index )
{
    current_vote = index;
    k_vote = 0;
    self->last_vote_time = g_globalvars.time + votes[index].pause * 60;
    if ( _addVote() )
    {
        votes[index].VoteRun();
        return;
    }

    G_bprint( 2, _c _m _d " " _v _o _t _e " " _y _e _s );
    G_bprint( 2, " in console to approve\n" );
    _startVote();
    Vote_OpenActiveMenus();
}

static void Vote_StartCommand( int index )
{
    current_vote = index;
    k_vote = 0;
    if ( votes[index].VoteInit() )
        Vote_CommitStart( index );
    else
        current_vote = -1;
}

static void Vote_CastYes( void )
{
    int index;

    if ( current_vote < 0 )
        return;
    if ( self->k_voted )
    {
        G_sprint( self, 3, "--- your vote is still good ---\n" );
        return;
    }

    index = current_vote;
    if ( _addVote() )
        votes[index].VoteRun();
}

static void Vote_CastNo( void )
{
    if ( self->k_voted )
        _subVote();
    G_sprint( self, 3, "--- your vote: no ---\n" );
}

void Vote_Cmd( void )
{
    char cmd_command[50];
    int argc;
    int i;
    const vote_t *ucmd;

    argc = trap_CmdArgc();
    if ( argc < 2 )
    {
        G_sprint( self, 3, "Available votes:\n" );
        for ( ucmd = votes; ucmd->command; ucmd++ )
            G_sprint( self, 3, "%s\n", ucmd->command );
        return;
    }

    trap_CmdArgv( 1, cmd_command, sizeof( cmd_command ) );
    elect_percentage = GetSVInfokeyInt( "electpercentage", NULL, 50 );
    if ( !elect_percentage )
    {
        G_sprint( self, 3, "Votes disabled\n" );
        return;
    }
    if ( elect_percentage < 5 || elect_percentage > 95 )
        elect_percentage = 50;

    if ( current_vote != -1 )
    {
        if ( !strcmp( cmd_command, "yes" ) )
        {
            Vote_CastYes();
            return;
        }
        if ( !strcmp( cmd_command, "no" ) )
        {
            Vote_CastNo();
            return;
        }
        G_sprint( self, 3, "Vote %s in progress\n", votes[current_vote].command );
        return;
    }

    if ( g_globalvars.time < self->last_vote_time )
    {
        G_sprint( self, 3, "You cannot vote at this time.\n" );
        return;
    }

    for ( ucmd = votes, i = 0; ucmd->command; ucmd++, i++ )
    {
        if ( !strcmp( cmd_command, ucmd->command ) )
        {
            Vote_StartCommand( i );
            return;
        }
    }
    G_sprint( self, 3, "Unknown vote.\n" );
}

static void Vote_FormatMenuNumber( int value, char *buffer, int buffer_size )
{
    char reverse[12];
    int count = 0;
    int i;

    if ( buffer_size < 1 )
        return;
    if ( value < 0 )
        value = 0;

    do
    {
        reverse[count++] = (char)( 0x92 + value % 10 );
        value /= 10;
    }
    while ( value && count < (int)sizeof( reverse ) );

    if ( count >= buffer_size )
        count = buffer_size - 1;
    for ( i = 0; i < count; i++ )
        buffer[i] = reverse[count - i - 1];
    buffer[count] = 0;
}

static void Vote_ColorObjective( const char *source, char *buffer, int buffer_size )
{
    unsigned char ch;
    int i;

    if ( buffer_size < 1 )
        return;
    for ( i = 0; source && source[i] && i < buffer_size - 1; i++ )
    {
        ch = (unsigned char)source[i];
        if ( ch >= '0' && ch <= '9' )
            buffer[i] = (char)( 0x92 + ch - '0' );
        else if ( ch > ' ' && ch < 127 )
            buffer[i] = (char)( ch | 0x80 );
        else
            buffer[i] = (char)ch;
    }
    buffer[i] = 0;
}

void Vote_Menu_Active( menunum_t menu )
{
    char colored_description[64];
    char current_votes[12];
    char required_votes[12];
    char time_left[12];
    int required;
    int seconds;

    if ( !self->newvote || current_vote == -1 )
    {
        ResetMenu();
        return;
    }

    required = Vote_RequiredVotes();
    seconds = (int)( vote_end_time - g_globalvars.time + 0.999 );
    if ( seconds < 0 )
        seconds = 0;

    Vote_ColorObjective( vote_description, colored_description,
                         sizeof( colored_description ) );
    Vote_FormatMenuNumber( k_vote, current_votes, sizeof( current_votes ) );
    Vote_FormatMenuNumber( required, required_votes, sizeof( required_votes ) );
    Vote_FormatMenuNumber( seconds, time_left, sizeof( time_left ) );

    CenterPrint( self,
        "%s votes for %s\n"
        "--------------------\n"
        "     " _M1 " Yes        \n"
        "     " _M2 " No         \n"
        "     " _M0 " Close      \n"
        "--------------------\n"
        "Votes: %s/%s\n"
        "Time left: %s sec\n",
        vote_initiator, colored_description, current_votes, required_votes, time_left );
}

void Vote_Menu_Active_Input( int inp )
{
    if ( inp == 1 )
        Vote_CastYes();
    else if ( inp == 2 )
        Vote_CastNo();
    else if ( inp == 10 )
        ResetMenu();
}
