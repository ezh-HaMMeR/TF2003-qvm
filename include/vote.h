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
 *  $Id: vote.h,v 1.2 2005-05-16 09:35:43 AngelD Exp $
 */

typedef struct {
	char		*command;
	int		  (*VoteInit) ();
	void		(*VoteRun) ();
	int		timeout;
	int		pause;
}vote_t;

extern const vote_t votes[];
extern int current_vote;

void Vote_Cmd( void );
void Vote_Menu_Cmd( void );
void Vote_Menu_Main( menunum_t menu );
void Vote_Menu_Main_Input( int inp );
void Vote_Menu_Players( menunum_t menu );
void Vote_Menu_Players_Input( int inp );
void Vote_Menu_Timelimit( menunum_t menu );
void Vote_Menu_Timelimit_Input( int inp );
void Vote_Menu_Maps( menunum_t menu );
void Vote_Menu_Maps_Input( int inp );
void Vote_Menu_Active( menunum_t menu );
void Vote_Menu_Active_Input( int inp );
