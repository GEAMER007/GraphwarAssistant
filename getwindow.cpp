#include "getwindow.hpp"

bool check_graphwar_running( HWND &final_hwnd )
{
    std::vector<HWND> matchingWindows;

    auto enumProcWithData = [ ] ( HWND hwnd, LPARAM lParam ) -> BOOL 
    {
        const wchar_t *target = L"Graphwar";
        const int bl = 512;
        wchar_t window[ bl ];

        auto len = GetWindowText( hwnd, window, bl );
        if ( IsWindowVisible( hwnd ) && len ) 
        {
            auto results = reinterpret_cast< std::vector<HWND> * >( lParam );
            if ( wcslen( target ) == len && memcmp( target, window, len ) == 0 )
            {
                results->push_back( hwnd );
            }
        }
        return TRUE;
    };

    EnumWindows( enumProcWithData, reinterpret_cast< LPARAM >( &matchingWindows ) );

    for ( HWND hwnd : matchingWindows ) 
    {
        wchar_t classname[ 12 ] = {};
        if ( GetClassNameW( hwnd, classname, 12 ) && memcmp( classname, L"SunAwtFrame", 22 ) == 0 )
        {
			final_hwnd = hwnd;
            return true;
        }
    }
    

    return 0;
}

bool copy_arena_buffer( HWND &hwnd, HDC &our_dc, int x, int y )
{
    static HDC hdc = GetDC( hwnd );
    if ( !hdc ) 
    {
        return false;
    }

    RECT rc;
    GetClientRect( hwnd, &rc );
    
	// if pixel at arena(0,0) is main-menu green then the game has not started yet
    auto arena_0_0 = GetPixel( hdc, arena_x + 250, arena_y);
    if ( arena_0_0 == 0x9B'D7'9E )
    {
        return false; 
	}

    BitBlt( our_dc, x, y, arena_w, arena_h, hdc, arena_x, arena_y, SRCCOPY );
    return true;
}
