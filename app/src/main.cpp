#include "app.h"
#include <debugapi.h>
#include <eh.h>

#include "defines.h"

#ifdef _RELEASE
	#include "core/logger.h"
#endif

#include <steam/steam_api.h>


#ifdef PLATFORM_WINDOWS
  #include <windows.h>
#endif

int alert(const char* caption, const char* message) {
  #ifndef PLATFORM_WINDOWS
    fprintf( stderr, "Message: '%s', Detail: '%s'\n", caption, message );
	  return 0;
  #else
    return ::MessageBox( NULL, caption, message, MB_OK );
  #endif
}

//-----------------------------------------------------------------------------
// Purpose: callback hook for debug text emitted from the Steam API
//-----------------------------------------------------------------------------
extern "C" void __cdecl SteamAPIDebugTextHook( int nSeverity, const char *pchDebugText )
{
	// if you're running in the debugger, only warnings (nSeverity >= 1) will be sent
	// if you add -debug_steamapi to the command-line, a lot of extra informational messages will also be sent
	::OutputDebugString( pchDebugText );

	if ( nSeverity >= 1 )
	{
		// place to set a breakpoint for catching API errors
	}
}

int entry(void) 
{
	if ( SteamAPI_RestartAppIfNecessary( STEAM_APP_ID ) )
	{
		// if Steam is not running or the game wasn't started through Steam, SteamAPI_RestartAppIfNecessary starts the 
		// local Steam client and also launches this game again.
		
		// Once you get a public Steam AppID assigned for this game, you need to replace k_uAppIdInvalid with it and
		// removed steam_appid.txt from the game depot.

		return EXIT_FAILURE;
	}	

	// Init Steam CEG
	if ( !Steamworks_InitCEGLibrary() )
	{
		OutputDebugString( "Steamworks_InitCEGLibrary() failed\n" );
		alert( "Fatal Error", "Init Drm Library fail.\n" );
		return EXIT_FAILURE;
	}

	// Initialize SteamAPI, if this fails we bail out since we depend on Steam for lots of stuff.
	// You don't necessarily have to though if you write your code to check whether all the Steam
	// interfaces are NULL before using them and provide alternate paths when they are unavailable.
	//
	// This will also load the in-game steam overlay dll into your process.  That dll is normally
	// injected by steam when it launches games, but by calling this you cause it to always load,
	// even when not launched via steam.
	SteamErrMsg errMsg = { 0 };
	if ( SteamAPI_InitEx( &errMsg ) != k_ESteamAPIInitResult_OK )
	{
		OutputDebugString( "SteamAPI_Init() failed: " );
		OutputDebugString( errMsg );
		OutputDebugString( "\n" );

		alert( "Fatal Error", "Steam API Init fail.\n" );
		return EXIT_FAILURE;
	}

	// set our debug handler
	SteamClient()->SetWarningMessageHook( &SteamAPIDebugTextHook );

	// Ensure that the user has logged into Steam. This will always return true if the game is launched
	// from Steam, but if Steam is at the login prompt when you run your game from the debugger, it
	// will return false.
	if (not SteamUser()->BLoggedOn() )
	{
		OutputDebugString( "Steam user is not logged in\n" );
		alert( "Steam user must be logged in to play this game", "Fatal Error");
		return EXIT_FAILURE;
	}

	// do a DRM self check
	Steamworks_SelfCheck();

	if(not app_initialize(SteamApps()->GetAppBuildId())) {
		alert("main::entry()::App initialize return with failure", "Oh no!");
		return EXIT_FAILURE;
	}

  	while (window_should_close())
  	{
  	  app_update();
	
  	  app_render();
  	}

    // TODO: Destr

	// Shutdown the SteamAPI
	SteamAPI_Shutdown();

	// Shutdown Steam CEG
	Steamworks_TermCEGLibrary();
    
	return EXIT_SUCCESS;
}

#ifdef PLATFORM_WINDOWS

//-----------------------------------------------------------------------------
// Purpose: Wrapper around SteamAPI_WriteMiniDump which can be used directly 
// as a se translator
//-----------------------------------------------------------------------------
void MiniDumpFunction([[maybe_unused]] unsigned int nExceptionCode, [[maybe_unused]] EXCEPTION_POINTERS *pException )
{
	MessageBox( nullptr, GAME_TITLE "is crashing now!", "Unhandled Exception", MB_OK );

	// You can build and set an arbitrary comment to embed in the minidump here,
	// maybe you want to put what level the user was playing, how many players on the server,
	// how much memory is free, etc...

	#ifdef _RELEASE
		const char * last_log = get_last_log();
		if (last_log and last_log != nullptr) {
			SteamAPI_SetMiniDumpComment( last_log );
		}

		// The 0 here is a build ID, we don't set it
		SteamAPI_WriteMiniDump( nExceptionCode, pException, SteamApps()->GetAppBuildId() );
	#endif
}


int main(void) 
{
    if ( IsDebuggerPresent() ) {
        return entry();
    }

    _set_se_translator( MiniDumpFunction );
    try 
    {
        return entry();    
    } 
    catch ( ... ) 
    {
        return -1;
    }

    return -1;
}
#endif // PLATFORM_WINDOWS
