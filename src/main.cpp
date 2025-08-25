#include "gui.hpp"

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
#ifdef _DEBUG
    AllocConsole( );

    freopen_s( reinterpret_cast< FILE ** >( stdout ), "CONOUT$", "w", stdout );
    freopen_s( reinterpret_cast< FILE ** >( stderr ), "CONOUT$", "w", stderr );
    freopen_s( reinterpret_cast< FILE ** >( stdin ), "CONIN$", "r", stdin );
#endif
    
	return init_gui( nullptr );
}
