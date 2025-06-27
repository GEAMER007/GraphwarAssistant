#include "gui.hpp"
#include "getwindow.hpp"

HWND g_target_window;
void wait_for_graphwar( )
{
	static unsigned int check_iteration = 0;
	while ( !check_graphwar_running( g_target_window ) )
	{
		Sleep( 1000 ); // Wait for GraphWar to start
		printf( "\r[%03d] Graphwar checked", check_iteration++ );
		fflush( stdout );
	}
	printf( "\n> Graphwar window found\n" );
}

int main()
{
	wait_for_graphwar( );
	//check_graphwar_running( g_target_window );
	auto thread = CreateThread( NULL, NULL, init_gui, nullptr, NULL, NULL ); // Create the GUI thread
	WaitForSingleObject( thread, INFINITE );
}
