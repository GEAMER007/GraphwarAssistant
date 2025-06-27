#include "gui.hpp"
#include "getwindow.hpp"
#include "helpers.hpp"
#include "equations.hpp"

int confirm_shooter_find_origin( HDC &hdc, int &cx, int &cy, int &ox, int &oy, char &dir )
{
	// player radius is 9 pixels
	struct
	{
		int found = 0;
		int x;
		int y;
	} eye; 

	for ( int y = cy - 9; y < cy + 9; ++y ) 
	{
		for ( int x = cx-9; x < cx + 9; ++x ) 
		{
			COLORREF color = GetPixel( hdc, x, y );
			if ( color == 0x79'79'79 )
			{
				eye.found++;
				eye.x = x;
				eye.y = y;
			}
		}
	}

	if ( eye.found == 1 )
	{
		dir = eye.x > cx ? 1 : -1; // shooters can face left or right. Origin depends on that

		int active = false;
		int ax = eye.x + 4 * dir; // approximate outline position
		int ay = eye.y + 2 * dir;

		for ( int y = ay - 2; y < ay + 2; ++y )
		{
			for ( int x = ax - 2; x < ax + 2; ++x )
			{
				COLORREF color = GetPixel( hdc, x, y );
				if ( color == 0x39'39'FF )
				{
					ox = ax;
					oy = ay;
					active++;
					break;
				}
			}
		}
		return active ? 0 : 2;
	}
	return 1;
}

const wchar_t *s_waiting = L"Waiting for game...";
const wchar_t *s_pickplayer = L"Click on shooter...";

const int l_arena_x = 10, l_arena_y = 25;

inline double s2a_x( int x )
{
	return ( x - l_arena_x - arena_w / 2 ) / 15.4;
}

inline double s2a_y( int y )
{
	return ( y - l_arena_y - arena_h / 2 ) / -15.0;
}

struct trajectory
{
	int state = 0;
	char dir;
	int ox, oy;
	bool next_up = false;
	std::vector<int2> vertices;
};

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


void draw_frame( HDC &hdc )
{

	static int mx, my;
	
	get_mouse_position( g_hwnd, mx, my );

	bool click = clicked_this_frame( g_hwnd );

	auto old_brush = SelectObject( hdc, brush_red );
	RoundRect( hdc, curr_width - 20, 0, curr_width, 20, 10, 10 );
	TextOutA( hdc, curr_width - 15, 3, "X", 1 );
	if ( click && in_box( mx, my, curr_width-20, 0, curr_width, 20 ) )
	{
		exit( 0 );
	}
	SelectObject( hdc, old_brush );

	auto is_in_game = copy_arena_buffer( g_target_window, hdc, l_arena_x, l_arena_y );

	if ( !is_in_game )
	{
		TextOut( hdc, 10, 10, s_waiting, lstrlenW( s_waiting ) );
		return;
	}

	if ( in_box( mx, my, l_arena_x, l_arena_y, arena_w, arena_h ) )
	{
		static wchar_t buffer[ 32 ] = {};
		swprintf( buffer, 32, L"x:%f y:%f", s2a_x( mx ), s2a_y( my ) );
		TextOut( hdc, 10, l_arena_y + arena_h + 10, buffer, lstrlenW( buffer ) );
	}
	else
	{
		click = false;

	}
	
	static trajectory tr;

	switch ( tr.state )
	{
		case 0:
		{
			TextOut( hdc, 10, 10, s_pickplayer, lstrlenW( s_pickplayer ) );

			if ( click )
			{
				auto fail = confirm_shooter_find_origin( hdc, mx, my, tr.ox, tr.oy, tr.dir );
				tr.state = 1;
				if ( fail )
				{
					MessageBoxA( g_hwnd, fail == 1 ? "Shooter not found on cursor" : "Shooter is not active",  "Bad selection", MB_OK | MB_ICONERROR );
				}
			}
			break;
		}
		case 1:
		{
			if ( GetKeyState( VK_SPACE ) & 0x8000 )
			{
				tr.vertices.clear( );
				tr.state = 0;
				return;
			}

			if ( GetKeyState( VK_RETURN ) & 0x8000 )
			{
				if ( tr.vertices.size( ) < 2 )
				{
					MessageBoxA( g_hwnd, "Not enough vertices", "Cant plot", MB_ICONERROR | MB_OK );
					Sleep( 200 );
					return;
				}
				tr.vertices.insert( tr.vertices.begin( ), { tr.ox, tr.oy } );
				tr.next_up = false;
				tr.state = 2;

				std::string equation = "0";
				trajectory_to_steps( tr, equation );
				
				if ( ctrl_c( equation.size( ) + 1, equation.data( ) ) )
				{
					MessageBoxA( g_hwnd, "Trajectory copied to clipboard", "Success", MB_OK | MB_ICONINFORMATION );
				}

				return;
			}

			static bool back_pressed = false;
			if ( GetKeyState( VK_BACK ) & 0x8000 )
			{
				if ( !back_pressed && !tr.vertices.empty( ) )
				{
					back_pressed = true;
					tr.vertices.pop_back( );
					tr.next_up = !tr.next_up;
				}
			}
			else
			{
				back_pressed = false;
			}

			auto old_pen = ( HPEN ) SelectObject( hdc, GetStockObject( DC_PEN ) );
			SetDCPenColor( hdc, 0xFF'00'00 );

			MoveToEx( hdc, tr.ox, tr.oy, nullptr );

			for ( auto &v : tr.vertices )
			{
				LineTo( hdc, v.x, v.y );
			}

			auto tx = mx, ty = my;

			int2 last_vertex = tr.vertices.empty( ) ? int2{tr.ox, tr.oy} : tr.vertices.back( );
			tx = last_vertex.x * tr.next_up + mx * !tr.next_up;
			ty = last_vertex.y * !tr.next_up + my * tr.next_up;
			
			if ( tr.next_up || last_vertex.x * tr.dir < tx * tr.dir )
			{
			    SetDCPenColor( hdc, 0x00'FF'00 );
			    LineTo( hdc, tx, ty );
				if ( click )
				{
					tr.vertices.push_back( { tx, ty } );
					tr.next_up = !tr.next_up;
				}
			}
			else if ( click )
			{
				MessageBoxA( g_hwnd, "Cant go backwards", "Wrong direction", MB_OK | MB_ICONERROR );
			}

			SelectObject( hdc, old_pen );
			break;
		}
		case 2:
		{
			if ( GetKeyState( VK_SPACE ) & 0x8000 )
			{
				tr.vertices.clear( );
				tr.state = 0;
				return;
			}
			std::string equation = "0";

			auto old_pen = ( HPEN ) SelectObject( hdc, GetStockObject( DC_PEN ) );
			SetDCPenColor( hdc, 0xFF'00'00 );

			MoveToEx( hdc, tr.ox, tr.oy, nullptr );

			for ( auto &v : tr.vertices )
			{
				LineTo( hdc, v.x, v.y );
			}

			SelectObject( hdc, old_pen );

			trajectory_to_steps( tr, equation );
			
			TextOutA( hdc, 10, l_arena_y + arena_h + 30, equation.c_str(), equation.length() );
			break;
		}

		default:
			break;
	}
}