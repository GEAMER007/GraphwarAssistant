#pragma once
#include "equations.hpp"

struct int2
{
    int x, y;

    inline bool operator==( const int2 &other ) const
    {
        return x == other.x && y == other.y;
    }
    inline int2 operator+( const int2 &other ) const
    {
        return { x + other.x,y + other.y };
    }
    inline static int dist_sq( const int2 &a, const int2 &b )
    {
		return ( a.x - b.x ) * ( a.x - b.x ) + ( a.y - b.y ) * ( a.y - b.y );
    }
};

namespace std {
    template <>
    struct hash<int2>
    {
        std::size_t operator()( const int2 &v ) const noexcept
        {
            std::size_t h1 = std::hash<int>( )( v.x );
            std::size_t h2 = std::hash<int>( )( v.y );
            return h1 ^ ( h2 << 1 ); // simple hash combine
        }
    };
}

struct trajectory
{
    int state = 0;
    char dir;
    int ox, oy;
    bool next_up = false;
    std::vector<int2> vertices;
};

//================================//
//===========GUI utils============//
//================================//

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

inline int color_dist_sq( uint32_t c1, uint32_t c2 )
{
    int r1 = ( c1 >> 16 ) & 0xFF;
    int g1 = ( c1 >> 8 ) & 0xFF;
    int b1 = c1 & 0xFF;
    int r2 = ( c2 >> 16 ) & 0xFF;
    int g2 = ( c2 >> 8 ) & 0xFF;
    int b2 = c2 & 0xFF;
	return ( r1 - r2 ) * ( r1 - r2 ) + ( g1 - g2 ) * ( g1 - g2 ) + ( b1 - b2 ) * ( b1 - b2 );
}

inline int is_grayscale( uint32_t color )
{
    int r = ( color >> 16 ) & 0xFF;
    int g = ( color >> 8 ) & 0xFF;
    int b = color & 0xFF;
    return ( r == g && g == b );
}

//================================//
//=====Coordinate translators=====//
//================================//

inline double s2a_x( int x )
{
    return ( x - l_arena_x - arena_w / 2 ) / 15.4;
}

inline double s2a_y( int y )
{
    return ( y - l_arena_y - arena_h / 2 ) / -15.4;
}

static void trajectory_to_steps( trajectory &tr, std::string &equation )
{
    auto i = 1;
    auto vs = tr.vertices.begin( );

    for ( ; i < tr.vertices.size( ) - 1; i += 2 )
    {
        auto c = -s2a_x( ( vs + i )->x );
        auto k = s2a_y( ( vs + i + 1 )->y ) - s2a_y( ( vs + i )->y );
        equation += "+" + make_step( k, c );
    }
}