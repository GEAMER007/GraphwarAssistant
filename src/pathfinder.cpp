#include "pathfinder.hpp"

//================================//
//========work in progress========//
//================================//

static void simplify( std::vector<uint32_t> &pixels, long &bmw, int2 &lc, int2 &rc )
{
    const int block_size = 16;
    const int block_shift = 8;
    for ( int by = lc.y; by < rc.y; by += block_size ) 
    {
        for ( int bx = lc.x; bx < rc.x; bx += block_size ) 
        {
            uint64_t r_sum = 0, g_sum = 0, b_sum = 0;

            for ( int y = by; y < by + block_size && y < rc.y; ++y ) 
            {
                for ( int x = bx; x < bx + block_size && x < rc.x; ++x ) 
                {
                    uint32_t &pixel = pixels[ y * bmw + x ];
                    uint8_t r = ( pixel >> 16 ) & 0xFF;
                    uint8_t g = ( pixel >> 8 ) & 0xFF;
                    uint8_t b = ( pixel >> 0 ) & 0xFF;

                    r_sum += r;
                    g_sum += g;
                    b_sum += b;
                }
            }


            uint8_t r_avg = static_cast< uint8_t >( r_sum >> block_shift );
            uint8_t g_avg = static_cast< uint8_t >( g_sum >> block_shift );
            uint8_t b_avg = static_cast< uint8_t >( b_sum >> block_shift );
            uint32_t avgColor = ( r_avg << 16 ) | ( g_avg << 8 ) | b_avg;

            for ( int y = by; y < by + block_size && y < rc.y; ++y ) 
            {
                for ( int x = bx; x < bx + block_size && x < rc.x; ++x )
                {
                    pixels[ y * bmw + x ] = ( avgColor >> 4 )<<4;
                }
            }
        }
    }

}

static void remove_axis( std::vector<uint32_t> &pixels, long &bmw, int2 &lc, int2 &rc )
{
    const int y_axis = ( lc.x + rc.x ) >> 1;
    for ( int y = lc.y; y < rc.y; y++ )
    {
        if ( pixels[ y * bmw + y_axis ] == 0x00000000 && pixels[ y * bmw + y_axis - 1 ] != 0x00000000 && pixels[ y * bmw + y_axis + 1 ] != 0x00000000 )
        {
            pixels[ y * bmw + y_axis ] = 0x00FFFFFF;
        }
    }

	const int offset = bmw * (( lc.y + rc.y ) >> 1);
    for ( int x = lc.x; x < rc.x; x++ )
    {
        if ( pixels[ offset + x ] == 0x00000000 && pixels[ offset + x - bmw ] != 0x00000000 && pixels[ offset + x + bmw ] != 0x00000000 )
        {
            pixels[ offset + x ] = 0x00FFFFFF;
        }
	}
}

static void obstacles_only( std::vector<uint32_t> &pixels, long &bmw, int2 &lc, int2 &rc )
{
    for ( int y = lc.y; y < rc.y; y++ )
    {
        for ( int x = lc.x; x < rc.x; x++ )
        {
            auto &pixel = pixels[ y * bmw + x ];
            if ( pixel != 0x00FFFFFF && pixel != 0x00000000 )
            {
                pixel = 0x00FFFFFF;
            }
        }
    }
}

static bool find_shooters( std::vector<uint32_t> &pixels, long &bmw, int2 &lc, int2 &rc, std::vector<int2> &players, int2 &active)
{
	std::vector<int2> interest_points;
    for ( int y = lc.y; y < rc.y; y++ )
    {
        for ( int x = lc.x; x < rc.x; x++ )
        {
			auto &pixel = pixels[ y * bmw + x ];
            if ( !is_grayscale( pixel ) && color_dist_sq( pixel, 0x00FFFF00 ) < 100)
            {
				interest_points.push_back( int2{ x, y } );
            }
        }
	}
    if ( interest_points.empty( ) )
    {
        return false;
	}

    std::vector<std::vector<int2>> clusters = {};
    for ( const auto &point : interest_points )
    {
        bool new_cluster = true;

        for ( auto &cluster : clusters )
        {
            if ( int2::dist_sq( point, cluster[ 0 ] ) < 205)
            {
                new_cluster = false;
                cluster.push_back( point );
                break;
            }
        }

        if ( new_cluster )
        {
            auto cluster = std::vector<int2>{ point };
            clusters.push_back( cluster );
		}
	}

    const int left_threshold = ( lc.x + rc.x ) >> 1;
    bool has_active = false;
    for ( auto &cluster : clusters )
    {
		const char dir = cluster[ 0 ].x > left_threshold ? 1 : -1;
        int2 edge = { 65536, 0 };

        for ( const auto &point : cluster )
        {
            int proj = static_cast< int >( point.x * dir );

            if ( proj < edge.x )
            {
                edge = point;
            }
        }
        edge.x += 7 * ( dir == -1 ) - 2 * ( dir == 1 );
        edge.y -= 7 * ( dir == -1 ) + -2 * ( dir == 1 );;

        players.push_back( edge );
        if ( color_dist_sq( pixels[ edge.y * bmw + edge.x ], 0x00FF3939 ) < 36 )
        {
            has_active = true;
			active = edge;
        }
    }
    return has_active;
}

bool parse_arena( HDC &hdc )
{
    auto hbmp = reinterpret_cast< HBITMAP >( GetCurrentObject( hdc, OBJ_BITMAP ) );
    auto bmp_info = BITMAP( );
    if ( GetObject( hbmp, sizeof( bmp_info ), &bmp_info ) == 0 )
    {
        return false;
    }
    std::vector<uint32_t> pixels( bmp_info.bmWidth * bmp_info.bmHeight );
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
    bmi.bmiHeader.biWidth = bmp_info.bmWidth;
    bmi.bmiHeader.biHeight = -bmp_info.bmHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    if ( !GetDIBits( hdc, hbmp, 0, bmp_info.bmHeight, pixels.data( ), &bmi, DIB_RGB_COLORS ) )
    {
        return false;
    }
	auto &bmw = bmp_info.bmWidth;
	auto lc = int2{ l_arena_x, l_arena_y }, rc = int2{ l_arena_x + arena_w, l_arena_y + arena_h };

	remove_axis( pixels, bmw, lc, rc );

	std::vector<int2> players;
    int2 muzzle;

    if ( find_shooters( pixels, bmw, lc, rc, players, muzzle ) )
    {
        obstacles_only( pixels, bmw, lc, rc );
        simplify( pixels, bmw, lc, rc );
        pixels[ muzzle.y * bmw + muzzle.x ] = 0x00FF0000;
        const int border = ( rc.x + lc.x ) >> 1;
        for ( auto &player : players )
        {
            if ( ( player.x < border && muzzle.x < border ) || ( player.x > border && muzzle.x > border ) )
            {
                continue;
            }
            pixels[ player.y * bmw + player.x ] = 0x00FF00FF;
        }
    }
    
    if ( !SetDIBits( hdc, hbmp, 0, bmp_info.bmHeight, pixels.data( ), &bmi, DIB_RGB_COLORS ) )
    {
        return false;
    }


    return true;
}