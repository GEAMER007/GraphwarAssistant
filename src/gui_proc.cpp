#include "gui.hpp"
#include "getwindow.hpp"
#include "helpers.hpp"
#include "pathfinder.hpp"

unsigned long long curtime;

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

static bool maybe_check_graphwar( bool &update )
{
	static auto last_check = 0ull;
	static bool last_running = false;


	if ( curtime - last_check < 1000 )
	{
		update = false;
		return last_running;
	}
	last_check = curtime;
	update = true;
	return last_running = check_graphwar_running( g_target_window );
}

static void maybe_parse_arena( HDC& hdc )
{
	static auto last_check = 0ull;
	static bool last_running = false;


	if ( curtime - last_check < 1000 )
	{
		//update = false;
		//return last_running;
		return;
	}
	last_check = curtime;
	//update = true;
	//return last_running = check_graphwar_running( g_target_window );
	parse_arena( hdc );
}

const char *s_waiting = "Waiting for game...";
const char *s_pickplayer = "Click on shooter...";

void draw_frame( HDC &hdc )
{
	curtime = GetTickCount64( );

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

	static const char *message = nullptr;
	if ( message )
	{
		TextOutA( hdc, 10, 10, message, strlen( message ) );
	}
	
	static auto check_iteration = 0;
	bool update;
	if ( !maybe_check_graphwar( update ) )
	{
		static char s_scanning[ 64 ];
		if ( update || message != s_scanning )
		{
			check_iteration++;
			message = s_scanning;
		}
		if ( check_iteration == 999 )
		{
			exit( 5 );
		}
		g_target_window = nullptr;
		sprintf_s( s_scanning, 64, "[%03d] Scanning for Graphwar window", check_iteration );
		return;
	}
	else
	{
		check_iteration = 0;
	}

	auto is_in_game = copy_arena_buffer( g_target_window, hdc, l_arena_x, l_arena_y );

	if ( !is_in_game )
	{
		message = s_waiting;
		return;
		
	}

	if ( in_box( mx, my, l_arena_x, l_arena_y, arena_w, arena_h ) )
	{
		static char buffer[ 32 ] = {};
		printf_s( buffer, 32, "x:%f y:%f", s2a_x( mx ), s2a_y( my ) );
		TextOutA( hdc, 10, l_arena_y + arena_h + 10, buffer, strlen( buffer ) );
	}
	else
	{
		click = false;
	}

	// work in progress

	//old_brush = SelectObject( hdc, brush_red );
	//RoundRect( hdc, curr_width - 20, 30, curr_width, 50, 10, 10 );
	//TextOutA( hdc, curr_width - 15, 33, "A", 1 );
	//if ( click && in_box( mx, my, curr_width - 20, 30, curr_width, 50 ) )
	//{
	//	maybe_parse_arena( hdc );
	//}
	//SelectObject( hdc, old_brush );
	parse_arena( hdc );
	
	static trajectory tr;

	switch ( tr.state )
	{
		case 0:
		{
			message = s_pickplayer;
			if ( click )
			{
				auto fail = confirm_shooter_find_origin( hdc, mx, my, tr.ox, tr.oy, tr.dir );
				tr.state = 1;
				message = "";
				if ( fail )
				{
					message = fail == 1 ? "Shooter not found on cursor" : "Shooter is not active";
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
				tr.next_up = false;
				return;
			}

			if ( GetKeyState( VK_RETURN ) & 0x8000 )
			{
				if ( tr.vertices.size( ) < 2 )
				{
					message = "Not enough vertices";
					return;
				}
				tr.vertices.insert( tr.vertices.begin( ), { tr.ox, tr.oy } );
				tr.next_up = false;
				tr.state = 2;

				std::string equation = "0";
				trajectory_to_steps( tr, equation );
				
				if ( ctrl_c( equation.size( ) + 1, equation.data( ) ) )
				{
					message = "Trajectory copied to clipboard";
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

			int2 last_vertex = tr.vertices.empty( ) ? int2 { tr.ox, tr.oy } : tr.vertices.back( );
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
				message = "Cant go backwards";
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
				tr.next_up = false;

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