#pragma once
#include "globals.hpp"

#define in_box(px,py,bx,by,bw,bh) (px >= bx && px <= bx + bw && py >= by && py <= by + bh)

inline bool get_mouse_position( HWND hwnd, int &x, int &y )
{
	POINT pt;
	if ( GetCursorPos( &pt ) ) {
		ScreenToClient( hwnd, &pt );
		x = pt.x;
		y = pt.y;
		return true;
	}
	return false;
}

inline bool clicked_this_frame( HWND& hwnd )
{
	static bool lb_wasclicked = false;

	if ( GetKeyState( VK_LBUTTON ) & 0x8000 )
	{
		if ( !lb_wasclicked )
		{
			lb_wasclicked = true;
			return GetForegroundWindow( ) == hwnd;
		}
	}
	else
	{
		lb_wasclicked = false;
	}

	return false;
}

struct int2
{
	int x, y;
};

inline bool ctrl_c( unsigned long bytes, const char* string )
{
    if ( !OpenClipboard( g_hwnd ) ) 
    {
        MessageBoxA( g_hwnd, "Failed to open clipboard", "Error", MB_OK | MB_ICONERROR );
        return false;
    }

    EmptyClipboard( );

    HGLOBAL hg = GlobalAlloc( GMEM_MOVEABLE, bytes );
    if ( !hg ) 
    {
        MessageBoxA( g_hwnd, "Failed to allocate memory for clipboard", "Error", MB_OK | MB_ICONERROR );
        CloseClipboard( );
        return false;
    }

    void *pg = GlobalLock( hg );
    if ( !pg ) 
    {
        GlobalFree( hg );
        CloseClipboard( );
        std::wcerr << L"GlobalLock failed.\n";
        return false;
    }

    memcpy( pg, string, bytes );
    GlobalUnlock( hg );

    if ( !SetClipboardData( CF_TEXT, hg ) )
    {
        GlobalFree( hg );
        CloseClipboard( );
        MessageBoxA( g_hwnd, "Failed to set clipboard data", "Error", MB_OK | MB_ICONERROR );

        return false;
    }

    CloseClipboard( );
    return true;
}
