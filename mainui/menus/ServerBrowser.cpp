/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "Framework.h"
#include "Bitmap.h"
#include "YesNoMessageBox.h"
#include "Table.h"
#include "keydefs.h"
#include "Switch.h"
#include "Field.h"
#include "utlvector.h"

#define ART_BANNER_INET		"gfx/shell/head_inetgames"
#define ART_BANNER_LAN		"gfx/shell/head_lan"
#define ART_BANNER_LOCK		"gfx/shell/lock"
#define ART_BANNER_DEDICATE "gfx/shell/dedicated"
#define ART_BANNER_WINDOWS  "gfx/shell/windows"
#define ART_BANNER_LINUX    "gfx/shell/linux"

struct server_t
{
	netadr_t adr;
	char os;
	char info[256];
	float ping;
	char name[64];
	char game[64];
	char mapname[64];
	char clientsstr[64];
	char botsstr[64];
	char pingstr[64];
	bool havePassword;
	bool IsDedicated;

	static int NameCmpAscend( const void *_a, const void *_b )
	{
		const server_t *a = (const server_t*)_a;
		const server_t *b = (const server_t*)_b;
		return colorstricmp( a->name, b->name );
	}
	static int NameCmpDescend( const void *a, const void *b )
	{
		return NameCmpAscend( b, a );
	}

	static int GameCmpAscend( const void *_a, const void *_b )
	{
		const server_t *a = (const server_t *)_a;
		const server_t *b = (const server_t *)_b;
		return stricmp( a->mapname, b->mapname );
	}
	static int GameCmpDescend( const void *a, const void *b )
	{
		return GameCmpAscend( b, a );
	}

	static int MapCmpAscend( const void *_a, const void *_b )
	{
		const server_t *a = (const server_t*)_a;
		const server_t *b = (const server_t*)_b;
		return stricmp( a->mapname, b->mapname );
	}
	static int MapCmpDescend( const void *a, const void *b )
	{
		return MapCmpAscend( b, a );
	}

	static int ClientCmpAscend( const void *_a, const void *_b )
	{
		const server_t *a = (const server_t*)_a;
		const server_t *b = (const server_t*)_b;

		int a_cl = atoi( Info_ValueForKey( a->info, "numcl" ));
		int b_cl = atoi( Info_ValueForKey( b->info, "numcl" ));

		if( a_cl > b_cl ) return 1;
		else if( a_cl < b_cl ) return -1;
		return 0;
	}
	static int ClientCmpDescend( const void *a, const void *b )
	{
		return ClientCmpAscend( b, a );
	}

	static int BotsCmpAscend( const void *_a, const void *_b )
	{
		const server_t *a = (const server_t *)_a;
		const server_t *b = (const server_t *)_b;

		int a_cl = atoi( Info_ValueForKey( a->info, "bots" ) );
		int b_cl = atoi( Info_ValueForKey( b->info, "bots" ) );

		if ( a_cl > b_cl )
			return 1;
		else if ( a_cl < b_cl )
			return -1;
		return 0;
	}
	static int BotsCmpDescend( const void *a, const void *b )
	{
		return BotsCmpAscend( b, a );
	}

	static int PingCmpAscend( const void *_a, const void *_b )
	{
		const server_t *a = (const server_t*)_a;
		const server_t *b = (const server_t*)_b;

		if( a->ping > b->ping ) return 1;
		else if( a->ping < b->ping ) return -1;
		return 0;
	}
	static int PingCmpDescend( const void *a, const void *b )
	{
		return PingCmpAscend( b, a );
	}
};

class CMenuGameListModel : public CMenuBaseModel
{
public:
	CMenuGameListModel() : CMenuBaseModel(), m_iSortingColumn(-1) {}

	void Update() override;
	int GetColumns() const override
	{
		return 9; // os, IsDedicated, havePassword, host, game, mapname, count, bots, ping
	}
	int GetRows() const override
	{
		return servers.Count();
	}
	ECellType GetCellType( int line, int column ) override
	{
		if( column == 0 || column == 1 || column == 2 )
			return CELL_IMAGE_ADDITIVE;
		return CELL_TEXT;
	}
	const char *GetCellText( int line, int column ) override
	{
		switch( column )
		{
		case 0: return servers[line].os == 'w' ? ART_BANNER_WINDOWS : ART_BANNER_LINUX;
		case 1: return servers[line].IsDedicated ? ART_BANNER_DEDICATE : NULL;
		case 2: return servers[line].havePassword ? ART_BANNER_LOCK : NULL;
		case 3: return servers[line].name;
		case 4: return servers[line].game;
		case 5: return servers[line].mapname;
		case 6: return servers[line].clientsstr;
		case 7: return servers[line].botsstr;
		case 8: return servers[line].pingstr;
		default: return NULL;
		}
	}

	bool GetCellColors( int line, int column, unsigned int &textColor, bool &force ) const override
	{
		CColor color = uiPromptTextColor;
		textColor = color;

		// allow colorstrings only in hostname
		if( column > 3 )
		{
			force = true;
			return true;
		}

		return false;
	}

	void OnActivateEntry(int line) override;

	void Flush()
	{
		servers.RemoveAll();
		serversRefreshTime = gpGlobals->time;
	}

	bool IsHavePassword( int line )
	{
		return servers[line].havePassword;
	}

	void AddServerToList( netadr_t adr, const char *info );

	bool Sort(int column, bool ascend) override;

	float serversRefreshTime;
	CUtlVector<server_t> servers;
private:
	int m_iSortingColumn;
	bool m_bAscend;
};

class CMenuServerBrowser: public CMenuFramework
{
public:
	CMenuServerBrowser() : CMenuFramework( "CMenuServerBrowser" ) { }
	void Draw() override;
	void Show() override;

	void SetLANOnly( bool lanOnly )
	{
		m_bLanOnly = lanOnly;
	}
	void GetGamesList( void );
	void ClearList( void );
	void RefreshList( void );
	void JoinGame( void );
	void ResetPing( void )
	{
		gameListModel.serversRefreshTime = Sys_DoubleTime();
	}

	void AddServerToList( netadr_t adr, const char *info );

	static void Connect( server_t &server );

	CMenuPicButton *joinGame;
	CMenuPicButton *createGame;
	CMenuPicButton *refresh;
	CMenuSwitch natOrDirect;

	CMenuYesNoMessageBox msgBox;
	CMenuTable	gameList;
	CMenuGameListModel gameListModel;

	CMenuYesNoMessageBox askPassword;
	CMenuField password;

	int	  refreshTime;
	int   refreshTime2;

	int   iServerCount;
	char  szServer[32];

	bool m_bLanOnly;
private:
	void _Init() override;
	void _VidInit() override;
};

static server_t staticServerSelect;
static bool staticWaitingPassword = false;

static CMenuServerBrowser	uiServerBrowser;

bool CMenuGameListModel::Sort(int column, bool ascend)
{
	m_iSortingColumn = column;
	if( column == -1 )
		return false; // disabled

	m_bAscend = ascend;
	switch( column )
	{
	case 0: // operating system
	case 1: // is dedicated
	case 2: // has password 
		return false;
	case 3:
		qsort( servers.Base(), servers.Count(), sizeof( server_t ),
			ascend ? server_t::NameCmpAscend : server_t::NameCmpDescend );
		return true;
	case 4:
		qsort( servers.Base(), servers.Count(), sizeof( server_t ),
			ascend ? server_t::GameCmpAscend : server_t::GameCmpDescend );
		return true;
	case 5:
		qsort( servers.Base(), servers.Count(), sizeof( server_t ),
			ascend ? server_t::MapCmpAscend : server_t::MapCmpDescend );
		return true;
	case 6:
		qsort( servers.Base(), servers.Count(), sizeof( server_t ),
			ascend ? server_t::ClientCmpAscend : server_t::ClientCmpDescend );
		return true;
	case 7:
		qsort( servers.Base(), servers.Count(), sizeof( server_t ),
			ascend ? server_t::BotsCmpAscend : server_t::BotsCmpDescend );
		return true;
	case 8:
		qsort( servers.Base(), servers.Count(), sizeof( server_t ),
			ascend ? server_t::PingCmpAscend : server_t::PingCmpDescend );
		return true;
	}

	return false;
}

/*
=================
CMenuServerBrowser::GetGamesList
=================
*/
void CMenuGameListModel::Update( void )
{
	int		i;
	const char *info;
	char passwd[2], dedicated[2], os[2];

	// regenerate table data
	for ( i = 0; i < servers.Count( ); i++ )
	{
		info = servers[i].info;

		Q_strncpy( passwd, Info_ValueForKey( info, "password" ), sizeof( passwd ) );
		Q_strncpy( dedicated, Info_ValueForKey( info, "dedicated" ), sizeof( dedicated ) ); // added in 0.19.4
		Q_strncpy( os, Info_ValueForKey( info, "os" ), sizeof( os ) );

		Q_strncpy( servers[i].name, Info_ValueForKey( info, "host" ), sizeof( servers[i].name ) );
		Q_strncpy( servers[i].game, Info_ValueForKey( info, "game" ), sizeof( servers[i].game ) );
		Q_strncpy( servers[i].mapname, Info_ValueForKey( info, "map" ), sizeof( servers[i].mapname ) );
		snprintf( servers[i].clientsstr, 64, "%s\\%s", Info_ValueForKey( info, "numcl" ), Info_ValueForKey( info, "maxcl" ) );
		snprintf( servers[i].botsstr, 64, "%s", Info_ValueForKey( info, "bots" ) );
		snprintf( servers[i].pingstr, 64, "%.f ms", servers[i].ping * 1000 );

		servers[i].havePassword = passwd[0] && !stricmp( passwd, "1" );
		servers[i].IsDedicated  = dedicated[0] && !stricmp( dedicated, "1" );
		servers[i].os           = os[0];
	}

	if( servers.Count() )
	{
		uiServerBrowser.joinGame->SetGrayed( false );
		if( m_iSortingColumn != -1 )
			Sort( m_iSortingColumn, m_bAscend );
	}
}

void CMenuGameListModel::OnActivateEntry( int line )
{
	if( servers.Count() )
	{
		CMenuServerBrowser::Connect( servers[line] );
	}
	else
	{
		uiServerBrowser.joinGame->SetGrayed( true );
	}
}

void CMenuGameListModel::AddServerToList(netadr_t adr, const char *info)
{
	int i;
	char passwd[2], dedicated[2], os[2];

	// ignore if duplicated
	for( i = 0; i < servers.Count(); i++ )
	{
		if( !stricmp( servers[i].info, info ))
			return;
	}

	server_t server;

	Q_strncpy( passwd, Info_ValueForKey( info, "password" ), sizeof( passwd ) );
	Q_strncpy( dedicated, Info_ValueForKey( info, "dedicated" ), sizeof( dedicated ) );
	Q_strncpy( os, Info_ValueForKey( info, "os" ), sizeof( os ) );

	server.adr = adr;
	server.ping = Sys_DoubleTime() - serversRefreshTime;
	server.ping = bound( 0, server.ping, 9.999 );
	Q_strncpy( server.info, info, sizeof( server.info ));
	Q_strncpy( server.name, Info_ValueForKey( info, "host" ), sizeof( server.name ) );
	Q_strncpy( server.game, Info_ValueForKey( info, "game" ), sizeof( server.game ) );
	Q_strncpy( server.mapname, Info_ValueForKey( info, "map" ), sizeof( server.mapname ) );
	snprintf( server.clientsstr, 64, "%s\\%s", Info_ValueForKey( info, "numcl" ), Info_ValueForKey( info, "maxcl" ) );
	snprintf( server.botsstr, 64, "%s", Info_ValueForKey( info, "bots" ) );

	server.havePassword = passwd[0] && !stricmp( passwd, "1" );
	server.IsDedicated  = dedicated[0] && !stricmp( dedicated, "1" );
	server.os			= os[0];

	snprintf( server.pingstr, 64, "%.f ms", server.ping * 1000 );

	uiServerBrowser.iServerCount++;
	snprintf( uiServerBrowser.szServer, sizeof( uiServerBrowser.szServer ), "%s (%d)", L( "Name" ), uiServerBrowser.iServerCount );
	servers.AddToTail( server );

	if( m_iSortingColumn != -1 )
		Sort( m_iSortingColumn, m_bAscend );
}

void CMenuServerBrowser::Connect( server_t &server )
{
	// prevent refresh during connect
	uiServerBrowser.refreshTime = uiStatic.realTime + 999999;

	// ask user for password
	if( server.havePassword )
	{
		// if dialog window is still open, then user have entered the password
		if( !staticWaitingPassword )
		{
			// save current select
			staticServerSelect = server;
			staticWaitingPassword = true;

			// show password request window
			uiServerBrowser.askPassword.Show();

			return;
		}
	}
	else
	{
		// remove password, as server don't require it
		EngFuncs::CvarSetString( "password", "" );
	}

	staticWaitingPassword = false;

	//BUGBUG: ClientJoin not guaranted to return, need use ClientCmd instead!!!
	//BUGBUG: But server addres is known only as netadr_t here!!!
	EngFuncs::ClientJoin( server.adr );
	UI_ConnectionProgress_Connect( "" );
}

/*
=================
CMenuServerBrowser::JoinGame
=================
*/
void CMenuServerBrowser::JoinGame()
{
	gameListModel.OnActivateEntry( gameList.GetCurrentIndex() );
}

void CMenuServerBrowser::ClearList()
{
	uiServerBrowser.iServerCount = 0;
	snprintf( szServer, sizeof( szServer ), "%s (0)", L( "Name" ) );

	gameListModel.Flush();
	joinGame->SetGrayed( true );
}

void CMenuServerBrowser::RefreshList()
{
	ClearList();

	if( m_bLanOnly )
	{
		EngFuncs::ClientCmd( FALSE, "localservers\n" );
	}
	else
	{
		if( uiStatic.realTime > refreshTime2 )
		{
			EngFuncs::ClientCmd( FALSE, "internetservers\n" );
			refreshTime2 = uiStatic.realTime + (EngFuncs::GetCvarFloat("cl_nat") ? 4000:1000);
			refresh->SetGrayed( true );
			if( uiStatic.realTime + 20000 < refreshTime )
				refreshTime = uiStatic.realTime + 20000;
		}
	}
}

/*
=================
UI_Background_Ownerdraw
=================
*/
void CMenuServerBrowser::Draw( void )
{
	CMenuFramework::Draw();

	if( uiStatic.realTime > refreshTime )
	{
		RefreshList();
		refreshTime = uiStatic.realTime + 20000; // refresh every 10 secs
	}

	if( uiStatic.realTime > refreshTime2 )
	{
		refresh->SetGrayed( false );
	}
}

/*
=================
CMenuServerBrowser::Init
=================
*/
void CMenuServerBrowser::_Init( void )
{
	AddItem( background );
	AddItem( banner );

	// Server count
	iServerCount = 0;
	snprintf( szServer, sizeof( szServer ), "%s (0)", L( "Name" ) );

	joinGame = AddButton( L( "Join game" ), L( "Join to selected game" ), PC_JOIN_GAME,
		VoidCb( &CMenuServerBrowser::JoinGame ), QMF_GRAYED );
	joinGame->onReleasedClActive = msgBox.MakeOpenEvent();

	createGame = AddButton( L( "GameUI_GameMenu_CreateServer" ), NULL, PC_CREATE_GAME );
	SET_EVENT_MULTI( createGame->onReleased,
	{
		if( ((CMenuServerBrowser*)pSelf->Parent())->m_bLanOnly )
			EngFuncs::CvarSetValue( "public", 0.0f );
		else
			EngFuncs::CvarSetValue( "public", 1.0f );

		EngFuncs::CvarSetValue( "sv_lan", ((CMenuServerBrowser*)pSelf->Parent())->m_bLanOnly );
		UI_CreateGame_Menu();
	});

	// TODO: implement!
	AddButton( L( "View game info" ), L( "Get detail game info" ), PC_VIEW_GAME_INFO, CEventCallback::NoopCb, QMF_GRAYED );

	refresh = AddButton( L( "Refresh" ), L( "Refresh servers list" ), PC_REFRESH, VoidCb( &CMenuServerBrowser::RefreshList ) );

	AddButton( L( "Done" ), L( "Return to main menu" ), PC_DONE, VoidCb( &CMenuServerBrowser::Hide ) );

	msgBox.SetMessage( L( "Join a network game will exit any current game, OK to exit?" ) );
	msgBox.SetPositiveButton( L( "GameUI_OK" ), PC_OK );
	msgBox.HighlightChoice( CMenuYesNoMessageBox::HIGHLIGHT_YES );
	msgBox.onPositive = VoidCb( &CMenuServerBrowser::JoinGame );
	msgBox.Link( this );

	gameList.SetCharSize( QM_SMALLFONT );
	gameList.SetupColumn( 0, NULL, 16.0f, true ); // operatingsystem
	gameList.SetupColumn( 1, NULL, 16.0f, true ); // isdedicated
	gameList.SetupColumn( 2, NULL, 16.0f, true ); // havepassword
	gameList.SetupColumn( 3, szServer, 0.38f );
	gameList.SetupColumn( 4, L( "Game" ), 0.20f );
	gameList.SetupColumn( 5, L( "GameUI_Map" ), 0.15f );
	gameList.SetupColumn( 6, L( "Players" ), 80.0f, true );
	gameList.SetupColumn( 7, L( "Bots" ), 60.0f, true );
	gameList.SetupColumn( 8, L( "Ping" ), 80.0f, true );
	gameList.SetModel( &gameListModel );
	gameList.bFramedHintText = true;
	gameList.bAllowSorting = true;

	natOrDirect.AddSwitch( L( "Direct" ) );
	natOrDirect.AddSwitch( L( "NAT" ) );							// Уже встречалось ранее.......
	natOrDirect.eTextAlignment = QM_CENTER;
	natOrDirect.bMouseToggle = false;
	natOrDirect.LinkCvar( "cl_nat" );
	natOrDirect.iSelectColor = uiInputFgColor;
	// bit darker
	natOrDirect.iFgTextColor = uiInputFgColor - 0x00151515;
	SET_EVENT_MULTI( natOrDirect.onChanged,
	{
		CMenuSwitch *self = (CMenuSwitch*)pSelf;
		CMenuServerBrowser *parent = (CMenuServerBrowser*)self->Parent();

		self->WriteCvar();
		parent->ClearList();
		parent->RefreshList();
	});

	// server.dll needs for reading savefiles or startup newgame
	if( !EngFuncs::CheckGameDll( ))
		createGame->SetGrayed( true );	// server.dll is missed - remote servers only

	password.bHideInput = true;
	password.bAllowColorstrings = false;
	password.bNumbersOnly = false;
	password.szName = L( "GameUI_Password" );
	password.iMaxLength = 16;
	password.SetRect( 188, 140, 270, 32 );

	SET_EVENT_MULTI( askPassword.onPositive,
	{
		CMenuServerBrowser *parent = (CMenuServerBrowser*)pSelf->Parent();

		EngFuncs::CvarSetString( "password", parent->password.GetBuffer() );
		parent->password.Clear(); // we don't need entered password anymore
		CMenuServerBrowser::Connect( staticServerSelect );
	});

	SET_EVENT_MULTI( askPassword.onNegative,
	{
		CMenuServerBrowser *parent = (CMenuServerBrowser*)pSelf->Parent();

		EngFuncs::CvarSetString( "password", "" );
		parent->password.Clear(); // we don't need entered password anymore
		staticWaitingPassword = false;
	});

	askPassword.SetMessage( L( "GameUI_PasswordPrompt" ) );
	askPassword.Link( this );
	askPassword.Init();
	askPassword.AddItem( password );

	AddItem( gameList );
	AddItem( natOrDirect );
}

/*
=================
CMenuServerBrowser::VidInit
=================
*/
void CMenuServerBrowser::_VidInit()
{
	if( m_bLanOnly )
	{
		banner.SetPicture( ART_BANNER_LAN );
		createGame->szStatusText = ( L( "Create new LAN game" ) );
		natOrDirect.Hide();
	}
	else
	{
		banner.SetPicture( ART_BANNER_INET );
		createGame->szStatusText = ( L( "Create new Internet game" ) );
		natOrDirect.Show();
	}

	gameList.SetRect( 300, 230, -20, 465 );
	natOrDirect.SetCoord( -20 - natOrDirect.size.w, gameList.pos.y - UI_OUTLINE_WIDTH - natOrDirect.size.h );

	refreshTime = uiStatic.realTime + 500; // delay before update 0.5 sec
	refreshTime2 = uiStatic.realTime + 500;
}

void CMenuServerBrowser::Show()
{
	CMenuFramework::Show();

	// clear out server table
	staticWaitingPassword = false;
	gameListModel.Flush();
	gameList.DisableSorting();
	joinGame->SetGrayed( true );
}

void CMenuServerBrowser::AddServerToList(netadr_t adr, const char *info)
{
	if( stricmp( gMenu.m_gameinfo.gamefolder, Info_ValueForKey( info, "gamedir" )) != 0 )
		return;

	if( !WasInit() )
		return;

	if( !IsVisible() )
		return;

	gameListModel.AddServerToList( adr, info );

	joinGame->SetGrayed( false );
}

/*
=================
CMenuServerBrowser::Precache
=================
*/
void UI_ServerBrowser_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER_INET );
	EngFuncs::PIC_Load( ART_BANNER_LAN );
}

/*
=================
CMenuServerBrowser::Menu
=================
*/
void UI_ServerBrowser_Menu( void )
{
	if ( gMenu.m_gameinfo.gamemode == GAME_SINGLEPLAYER_ONLY )
		return;

	// stop demos to allow network sockets to open
	if ( gpGlobals->demoplayback && EngFuncs::GetCvarFloat( "cl_background" ))
	{
		uiStatic.m_iOldMenuDepth = uiStatic.menu.Count();
		EngFuncs::ClientCmd( FALSE, "stop\n" );
		uiStatic.m_fDemosPlayed = true;
	}

	uiServerBrowser.Show();
}

void UI_InternetGames_Menu( void )
{
	uiServerBrowser.SetLANOnly( false );

	UI_ServerBrowser_Menu();
}

void UI_LanGame_Menu( void )
{
	uiServerBrowser.SetLANOnly( true );

	UI_ServerBrowser_Menu();
}
ADD_MENU( menu_langame, NULL, UI_LanGame_Menu );
ADD_MENU( menu_internetgames, UI_ServerBrowser_Precache, UI_InternetGames_Menu );

/*
=================
UI_AddServerToList
=================
*/
void UI_AddServerToList( netadr_t adr, const char *info )
{
	if( !uiStatic.initialized )
		return;

	uiServerBrowser.AddServerToList( adr, info );
}

/*
=================
UI_MenuResetPing_f
=================
*/
void UI_MenuResetPing_f( void )
{
	Con_Printf("UI_MenuResetPing_f\n");
	uiServerBrowser.ResetPing();
}
ADD_COMMAND( menu_resetping, UI_MenuResetPing_f );
