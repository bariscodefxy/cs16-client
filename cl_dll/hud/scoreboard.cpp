/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// Scoreboard.cpp
//
// implementation of CHudScoreboard class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "triangleapi.h"
#include "com_weapons.h"
#include "cdll_dll.h"

#include <string.h>
#include <stdio.h>
#include "draw_util.h"

hud_player_info_t   g_PlayerInfoList[MAX_PLAYERS+1]; // player info from the engine
extra_player_info_t	g_PlayerExtraInfo[MAX_PLAYERS+1]; // additional player info sent directly to the client dll
team_info_t         g_TeamInfo[MAX_TEAMS+1];
hostage_info_t      g_HostageInfo[MAX_HOSTAGES+1];
int g_iUser1;
int g_iUser2;
int g_iUser3;
int g_iTeamNumber;


// X positions

int xstart, xend;
int ystart, yend;
// relative to the side of the scoreboard
inline int NAME_POS_START()		{ return xstart + 15; }
inline int NAME_POS_END()		{ return xend - 210; }

inline int ATTRIB_POS_START()	{ return xend - 420; }
inline int ATTRIB_POS_END()		{ return xend - 360; }

inline int HEALTH_POS_START()	{ return xend - 350; }
inline int HEALTH_POS_END()		{ return xend - 290; }

inline int MONEY_POS_START()	{ return xend - 280; }
inline int MONEY_POS_END()		{ return xend - 220; }

inline int KILLS_POS_START()	{ return xend - 210; }
inline int KILLS_POS_END()		{ return xend - 150; }

inline int DEATHS_POS_START()	{ return xend - 140; }
inline int DEATHS_POS_END()		{ return xend - 80; }

inline int PING_POS_START()		{ return xend - 70; }
inline int PING_POS_END()		{ return xend - 10; }

//#include "vgui_TeamFortressViewport.h"

DECLARE_COMMAND( m_Scoreboard, ShowScores )
DECLARE_COMMAND( m_Scoreboard, HideScores )
DECLARE_COMMAND( m_Scoreboard, ShowScoreboard2 )
DECLARE_COMMAND( m_Scoreboard, HideScoreboard2 )

DECLARE_MESSAGE( m_Scoreboard, ScoreInfo )
DECLARE_MESSAGE( m_Scoreboard, TeamInfo )
DECLARE_MESSAGE( m_Scoreboard, TeamScore )
DECLARE_MESSAGE( m_Scoreboard, HealthInfo )
DECLARE_MESSAGE( m_Scoreboard, Account )

int CHudScoreboard :: Init( void )
{
	gHUD.AddHudElem( this );

	// Hook messages & commands here
	HOOK_COMMAND( "+showscores", ShowScores );
	HOOK_COMMAND( "-showscores", HideScores );
	HOOK_COMMAND( "showscoreboard2", ShowScoreboard2 );
	HOOK_COMMAND( "hidescoreboard2", HideScoreboard2 );

	HOOK_MESSAGE( ScoreInfo );
	HOOK_MESSAGE( TeamScore );
	HOOK_MESSAGE( TeamInfo );
	HOOK_MESSAGE( HealthInfo );
	HOOK_MESSAGE( Account );

	InitHUDData();

	return 1;
}


int CHudScoreboard :: VidInit( void )
{
	xstart = ScreenWidth * 0.125f;
	xend = ScreenWidth - xstart;
	ystart = 100;
	yend = ScreenHeight - ystart;
	m_bForceDraw = false;

	// Load sprites here
	return 1;
}

void CHudScoreboard :: InitHUDData( void )
{
	memset( g_PlayerExtraInfo, 0, sizeof g_PlayerExtraInfo );
	m_iLastKilledBy = 0;
	m_fLastKillTime = 0;
	m_iPlayerNum = 0;
	m_iNumTeams = 0;
	memset( g_TeamInfo, 0, sizeof g_TeamInfo );

	m_iFlags &= ~HUD_DRAW;  // starts out inactive

	m_iFlags |= HUD_INTERMISSION; // is always drawn during an intermission
}

// Y positions
#define ROW_GAP  15

int CHudScoreboard :: Draw( float flTime )
{
	if( !m_bForceDraw )
	{
		if ( (!m_bShowscoresHeld && gHUD.m_Health.m_iHealth > 0 && !gHUD.m_iIntermission) )
			return 1;
		else
		{
			xstart     = 0.125f * ScreenWidth;
			xend       = ScreenWidth - xstart;
			ystart     = 90;
			yend       = ScreenHeight - ystart;
			m_colors.r = 0;
			m_colors.g = 0;
			m_colors.b = 0;
			m_colors.a = 153;
			m_bDrawStroke = false;
		}
	}

	return DrawScoreboard(flTime);
}

int CHudScoreboard :: DrawScoreboard( float fTime )
{
	GetAllPlayersInfo();
	char ServerName[90];

	// just sort the list on the fly
	// list is sorted first by frags, then by deaths
	float list_slot = 0;

	// print the heading line

	DrawUtils::DrawRectangle(xstart, ystart, xend - xstart, yend - ystart,
		m_colors.r, m_colors.g, m_colors.b, m_colors.a, m_bDrawStroke);

	int ypos = ystart + (list_slot * ROW_GAP) + 10;

	if( gHUD.m_szServerName[0] )
		_snprintf(ServerName, 64, "%s", gHUD.m_szServerName);

	DrawUtils::DrawHudString( NAME_POS_START(), ypos, NAME_POS_END(), ServerName, 255, 140, 0 );
	DrawUtils::DrawHudStringReverse( HEALTH_POS_END(), ypos, 0, "HP", 255, 140, 0 );
	DrawUtils::DrawHudStringReverse( MONEY_POS_END(), ypos, MONEY_POS_START(), "Money", 255, 140, 0 );
	DrawUtils::DrawHudStringReverse( KILLS_POS_END(), ypos, 0, "Score", 255, 140, 0 );
	DrawUtils::DrawHudStringReverse( DEATHS_POS_END(), ypos, DEATHS_POS_START(), "Deaths", 255, 140, 0 );
	DrawUtils::DrawHudStringReverse( PING_POS_END(), ypos, PING_POS_START(), "Latency", 255, 140, 0 );

	list_slot += 2;
	ypos = ystart + (list_slot * ROW_GAP);
	//FillRGBA( xstart, ypos, xend - xstart, 1, 255, 140, 0, 255);  // draw the separator line
	
	list_slot += 0.8;

	if ( gHUD.m_Teamplay )
	{
		DrawTeams( list_slot );
	}
	else
	{
		// it's not teamplay,  so just draw a simple player list
		DrawPlayers( list_slot );
	}
	return 1;
}

int CHudScoreboard :: DrawTeams( float list_slot )
{
	int j;
	int ypos = ystart + (list_slot * ROW_GAP) + 5;

	// clear out team scores
	for ( int i = 1; i <= m_iNumTeams; i++ )
	{
		if ( !g_TeamInfo[i].scores_overriden )
			g_TeamInfo[i].frags = g_TeamInfo[i].deaths = 0;
		g_TeamInfo[i].sumping = 0;
		g_TeamInfo[i].players = 0;
		g_TeamInfo[i].already_drawn = FALSE;
	}

	// recalc the team scores, then draw them
	for ( int i = 1; i < MAX_PLAYERS; i++ )
	{
		if ( !g_PlayerInfoList[i].name || !g_PlayerInfoList[i].name[0] )
			continue; // empty player slot, skip

		if ( g_PlayerExtraInfo[i].teamname[0] == 0 )
			continue; // skip over players who are not in a team

		// find what team this player is in
		for ( j = 1; j <= m_iNumTeams; j++ )
		{
			if ( !stricmp( g_PlayerExtraInfo[i].teamname, g_TeamInfo[j].name ) )
				break;
		}

		if ( j > m_iNumTeams )  // player is not in a team, skip to the next guy
			continue;

		if ( !g_TeamInfo[j].scores_overriden )
		{
			g_TeamInfo[j].frags += g_PlayerExtraInfo[i].frags;
			g_TeamInfo[j].deaths += g_PlayerExtraInfo[i].deaths;
		}

		g_TeamInfo[j].sumping += g_PlayerInfoList[i].ping;

		if ( g_PlayerInfoList[i].thisplayer )
			g_TeamInfo[j].ownteam = TRUE;
		else
			g_TeamInfo[j].ownteam = FALSE;

		g_TeamInfo[j].players++;
	}

	// Draw the teams
	int iSpectatorPos = -1;

	while( true )
	{
		int highest_frags = -99999; int lowest_deaths = 99999;
		int best_team = 0;

		for ( int i = 1; i <= m_iNumTeams; i++ )
		{
			// don't draw team without players
			if ( g_TeamInfo[i].players <= 0 )
				continue;

			if (!strnicmp(g_TeamInfo[i].name, "SPECTATOR", MAX_TEAM_NAME))
			{
				iSpectatorPos = i;
				continue;
			}

			if ( !g_TeamInfo[i].already_drawn && g_TeamInfo[i].frags >= highest_frags )
			{
				if ( g_TeamInfo[i].frags > highest_frags || g_TeamInfo[i].deaths < lowest_deaths )
				{
					best_team = i;
					lowest_deaths = g_TeamInfo[i].deaths;
					highest_frags = g_TeamInfo[i].frags;
				}
			}
		}

		// draw the best team on the scoreboard
		if ( !best_team )
		{
			// if spectators is found and still not drawn
			if( iSpectatorPos != -1 && g_TeamInfo[iSpectatorPos].already_drawn == FALSE )
				best_team = iSpectatorPos;
			else break;
		}
		// draw out the best team
		team_info_t *team_info = &g_TeamInfo[best_team];

		// don't draw team without players
		if ( team_info->players <= 0 )
			continue;

		ypos = ystart + (list_slot * ROW_GAP);

		// check we haven't drawn too far down
		if ( ypos > yend )  // don't draw to close to the lower border
			break;

		int r, g, b;
		char teamName[64];

		GetTeamColor( r, g, b, team_info->teamnumber );
		switch( team_info->teamnumber )
		{
		case TEAM_TERRORIST:
			_snprintf(teamName, sizeof(teamName), "Terrorists   -   %i players", team_info->players);
			break;
		case TEAM_CT:
			_snprintf(teamName, sizeof(teamName), "Counter-Terrorists   -   %i players", team_info->players);
			break;
		case TEAM_SPECTATOR:
		case TEAM_UNASSIGNED:
			strncpy( teamName, "Spectators", sizeof(teamName) );
			break;
		}

		DrawUtils::DrawHudString( NAME_POS_START(), ypos, NAME_POS_END(),   teamName,   r, g, b );

		team_info->already_drawn = TRUE;  // set the already_drawn to be TRUE, so this team won't get drawn again

		// draw underline
		list_slot += 1.2f;
		FillRGBA( xstart, ystart + (list_slot * ROW_GAP), xend - xstart, 1, r, g, b, 255);

		list_slot += 0.4f;
		// draw all the players that belong to this team, indented slightly
		list_slot = DrawPlayers( list_slot, 10, team_info->name );
	}

	// draw all the players who are not in a team
	list_slot += 4.0f;
	DrawPlayers( list_slot, 0, "" );

	return 1;
}

// returns the ypos where it finishes drawing
int CHudScoreboard :: DrawPlayers( float list_slot, int nameoffset, const char *team )
{
	// draw the players, in order,  and restricted to team if set
	while ( 1 )
	{
		// Find the top ranking player
		int highest_frags = -99999;	int lowest_deaths = 99999;
		int best_player = 0;

		for ( int i = 1; i < MAX_PLAYERS; i++ )
		{
			if ( g_PlayerInfoList[i].name && g_PlayerExtraInfo[i].frags >= highest_frags )
			{
				if ( !(team && stricmp(g_PlayerExtraInfo[i].teamname, team)) )  // make sure it is the specified team
				{
					extra_player_info_t *pl_info = &g_PlayerExtraInfo[i];
					if ( pl_info->frags > highest_frags || pl_info->deaths < lowest_deaths )
					{
						best_player = i;
						lowest_deaths = pl_info->deaths;
						highest_frags = pl_info->frags;
					}
				}
			}
		}

		if ( !best_player )
			break;

		// draw out the best player
		hud_player_info_t *pl_info = &g_PlayerInfoList[best_player];

		int ypos = ystart + (list_slot * ROW_GAP);

		// check we haven't drawn too far down
		if ( ypos > yend )  // don't draw to close to the lower border
			break;

		int r = 255, g = 255, b = 255;
		float *colors = GetClientColor( best_player );
		r *= colors[0];
		g *= colors[1];
		b *= colors[2];

		if(pl_info->thisplayer) // hey, it's me!
		{
			FillRGBABlend( xstart, ypos, xend - xstart, ROW_GAP, 255, 255, 255, 15 );
		}

		DrawUtils::DrawHudString( NAME_POS_START() + nameoffset, ypos, NAME_POS_END(), pl_info->name, r, g, b );

		// draw bomb( if player have the bomb )
		if( g_PlayerExtraInfo[best_player].dead )
			DrawUtils::DrawHudString( ATTRIB_POS_START(), ypos, ATTRIB_POS_END(), "Dead", r, g, b );
		else if( g_PlayerExtraInfo[best_player].has_c4 )
			DrawUtils::DrawHudString( ATTRIB_POS_START(), ypos, ATTRIB_POS_END(), "Bomb", r, g, b );
		else if( g_PlayerExtraInfo[best_player].vip )
			DrawUtils::DrawHudString( ATTRIB_POS_START(), ypos, ATTRIB_POS_END(), "VIP",  r, g, b );

		// draw health
		if ( !g_PlayerExtraInfo[best_player].dead && g_PlayerExtraInfo[best_player].healthinfo > -1 )
		{
			DrawUtils::DrawHudNumberString( HEALTH_POS_END( ), ypos, HEALTH_POS_START( ), g_PlayerExtraInfo[best_player].healthinfo, r, g, b );
		}

		// draw money
		if ( g_PlayerExtraInfo[best_player].account != -1 )
		{
			char moneyInfo[32];
			_snprintf( moneyInfo, sizeof( moneyInfo ), "$%i", g_PlayerExtraInfo[best_player].account );
			DrawUtils::DrawHudStringReverse( MONEY_POS_END( ), ypos, MONEY_POS_START( ), moneyInfo, r, g, b );
		}

		// draw kills (right to left)
		DrawUtils::DrawHudNumberString( KILLS_POS_END(), ypos, KILLS_POS_START(), g_PlayerExtraInfo[best_player].frags, r, g, b );

		// draw deaths
		DrawUtils::DrawHudNumberString( DEATHS_POS_END(), ypos, DEATHS_POS_START(), g_PlayerExtraInfo[best_player].deaths, r, g, b );

		// draw ping & packetloss
		const char *value;
		if( pl_info->ping <= 5  // must be 0, until Xash's bug not fixed
			&& ( value = gEngfuncs.PlayerInfo_ValueForKey( best_player, "*bot" ) )
			&& atoi( value ) > 0 )
		{
			DrawUtils::DrawHudStringReverse( PING_POS_END(), ypos, PING_POS_START(), "BOT", r, g, b );
		}
		else
		{
			static char buf[64];
			sprintf( buf, "%d", pl_info->ping );
			DrawUtils::DrawHudStringReverse( PING_POS_END(), ypos, PING_POS_START(), buf, r, g, b );
		}
	
		pl_info->name = NULL;  // set the name to be NULL, so this client won't get drawn again
		list_slot++;
	}

	list_slot += 2.0f;

	return list_slot;
}


void CHudScoreboard :: GetAllPlayersInfo( void )
{
	for ( int i = 1; i < MAX_PLAYERS; i++ )
	{
		GetPlayerInfo( i, &g_PlayerInfoList[i] );

		if ( g_PlayerInfoList[i].thisplayer )
			m_iPlayerNum = i;  // !!!HACK: this should be initialized elsewhere... maybe gotten from the engine
	}
}

int CHudScoreboard :: MsgFunc_ScoreInfo( const char *pszName, int iSize, void *pbuf )
{
	m_iFlags |= HUD_DRAW;

	BufferReader reader( pszName, pbuf, iSize );
	short cl = reader.ReadByte();
	short frags = reader.ReadShort();
	short deaths = reader.ReadShort();
	short playerclass = reader.ReadShort();
	short teamnumber = reader.ReadShort();

	if ( cl > 0 && cl <= MAX_PLAYERS )
	{
		g_PlayerExtraInfo[cl].frags = frags;
		g_PlayerExtraInfo[cl].deaths = deaths;
		g_PlayerExtraInfo[cl].playerclass = playerclass;
		g_PlayerExtraInfo[cl].teamnumber = teamnumber;

		//gViewPort->UpdateOnPlayerInfo();
	}

	return 1;
}

// Message handler for TeamInfo message
// accepts two values:
//		byte: client number
//		string: client team name
int CHudScoreboard :: MsgFunc_TeamInfo( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	short cl = reader.ReadByte();
	int teamNumber = 0;

	if ( cl > 0 && cl <= MAX_PLAYERS )
	{
		// set the players team
		char teamName[MAX_TEAM_NAME];
		strncpy( teamName, reader.ReadString(), MAX_TEAM_NAME );

		if( !strcmp( teamName, "TERRORIST") )
			teamNumber = TEAM_TERRORIST;
		else if( !strcmp( teamName, "CT") )
			teamNumber = TEAM_CT;
		else if( !strcmp( teamName, "SPECTATOR" ) || !strcmp( teamName, "UNASSIGNED" ) )
		{
			teamNumber = TEAM_SPECTATOR;
			strncpy( teamName, "SPECTATOR", MAX_TEAM_NAME );
		}
		// just in case
		else teamNumber = TEAM_UNASSIGNED;

		strncpy( g_PlayerExtraInfo[cl].teamname, teamName, MAX_TEAM_NAME );
		g_PlayerExtraInfo[cl].teamnumber = teamNumber;
	}

	// rebuild the list of teams

	// clear out player counts from teams
	for ( int i = 1; i <= m_iNumTeams; i++ )
	{
		g_TeamInfo[i].players = 0;
	}

	// rebuild the team list
	GetAllPlayersInfo();
	m_iNumTeams = 0;

	for ( int i = 1; i < MAX_PLAYERS; i++ )
	{
		int j;
		//if ( g_PlayerInfoList[i].name == NULL )
		//	continue;

		if ( g_PlayerExtraInfo[i].teamname[0] == 0 )
			continue; // skip over players who are not in a team

		// is this player in an existing team?
		for ( j = 1; j <= m_iNumTeams; j++ )
		{
			if ( g_TeamInfo[j].name[0] == '\0' )
				break;

			if ( !stricmp( g_PlayerExtraInfo[i].teamname, g_TeamInfo[j].name ) )
				break;
		}

		if ( j > m_iNumTeams )
		{
			// they aren't in a listed team, so make a new one
			for ( j = 1; j <= m_iNumTeams; j++ )
			{
				if ( g_TeamInfo[j].name[0] == '\0' )
					break;
			}


			m_iNumTeams = max( j, m_iNumTeams );

			strncpy( g_TeamInfo[j].name, g_PlayerExtraInfo[i].teamname, MAX_TEAM_NAME );
			g_TeamInfo[j].teamnumber = g_PlayerExtraInfo[i].teamnumber;
			g_TeamInfo[j].players = 0;
		}

		g_TeamInfo[j].players++;
	}

	// clear out any empty teams
	for ( int i = 1; i <= m_iNumTeams; i++ )
	{
		if ( g_TeamInfo[i].players < 1 )
			memset( &g_TeamInfo[i], 0, sizeof(team_info_t) );
	}

	return 1;
}

// Message handler for TeamScore message
// accepts three values:
//		string: team name
//		short: teams kills
//		short: teams deaths 
// if this message is never received, then scores will simply be the combined totals of the players.
int CHudScoreboard :: MsgFunc_TeamScore( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	char *TeamName = reader.ReadString();
	int i;

	// find the team matching the name
	for ( i = 1; i <= m_iNumTeams; i++ )
	{
		if ( !stricmp( TeamName, g_TeamInfo[i].name ) )
			break;
	}
	if ( i > m_iNumTeams )
		return 1;

	// use this new score data instead of combined player scores
	g_TeamInfo[i].scores_overriden = TRUE;
	g_TeamInfo[i].frags = reader.ReadShort();
	g_TeamInfo[i].deaths = reader.ReadShort();
	
	return 1;
}

void CHudScoreboard :: DeathMsg( int killer, int victim )
{
	// if we were the one killed,  or the world killed us, set the scoreboard to indicate suicide
	if ( victim == m_iPlayerNum || killer == 0 )
	{
		m_iLastKilledBy = killer ? killer : m_iPlayerNum;
		m_fLastKillTime = gHUD.m_flTime + 10;	// display who we were killed by for 10 seconds

		if ( killer == m_iPlayerNum )
			m_iLastKilledBy = m_iPlayerNum;
	}
}

int CHudScoreboard::MsgFunc_HealthInfo(const char* pszName, int iSize, void* pbuf)
{
	BufferReader reader( pszName, pbuf, iSize );
	int i = reader.ReadByte( );
	long healthInfo = reader.ReadLong( );
	g_PlayerExtraInfo[i].healthinfo = healthInfo;
	if ( g_PlayerInfoList[i].thisplayer && g_PlayerExtraInfo[i].healthinfo > 255 && g_PlayerExtraInfo[i].healthinfo != gHUD.m_Health.m_iHealth )
	{
		gHUD.m_Health.m_fFade = FADE_TIME;
		gHUD.m_Health.m_iHealth = g_PlayerExtraInfo[i].healthinfo;
	}
	return 1;
}

int CHudScoreboard::MsgFunc_Account( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	int i = reader.ReadByte( );
	long account = reader.ReadLong( );
	g_PlayerExtraInfo[i].account = account;
	return 1;
}

void CHudScoreboard :: UserCmd_ShowScores( void )
{
	m_bForceDraw = false;
	m_bShowscoresHeld = true;
}

void CHudScoreboard :: UserCmd_HideScores( void )
{
	m_bForceDraw = m_bShowscoresHeld = false;
}


void CHudScoreboard	:: UserCmd_ShowScoreboard2()
{
	if( gEngfuncs.Cmd_Argc() != 9 )
	{
		ConsolePrint("showscoreboard2 <xstart> <xend> <ystart> <yend> <r> <g> <b> <a>");
	}

	xstart     = atof(gEngfuncs.Cmd_Argv(1)) * ScreenWidth;
	xend       = atof(gEngfuncs.Cmd_Argv(2)) * ScreenWidth;
	ystart     = atof(gEngfuncs.Cmd_Argv(3)) * ScreenHeight;
	yend       = atof(gEngfuncs.Cmd_Argv(4)) * ScreenHeight;
	m_colors.r = atoi(gEngfuncs.Cmd_Argv(5));
	m_colors.b = atoi(gEngfuncs.Cmd_Argv(6));
	m_colors.b = atoi(gEngfuncs.Cmd_Argv(7));
	m_colors.a = atoi(gEngfuncs.Cmd_Argv(8));
	m_bDrawStroke = false;
	m_bForceDraw = true;
}

void CHudScoreboard :: UserCmd_HideScoreboard2()
{
	m_bForceDraw = m_bShowscoresHeld = false; // and disable it
}
