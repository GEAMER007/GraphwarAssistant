#include "pathfinder.hpp"
#include "unordered_map"

int block_arena_width = 49, block_arena_height = 29;
int block_size = 16;
int block_shift = 8;
int block_shift_2 = block_shift >> 1;

int scale_level = 4;
char **simplified_map = nullptr;
char *simplified_map_proxyed = nullptr;
void set_simplified_scale( int new_scale )
{
	auto scale_diff = new_scale - scale_level;
    scale_level = new_scale;

    block_shift_2 += scale_diff;
    block_shift = block_shift_2 << 1;

    if ( scale_diff >= 0 )
    {
        block_size <<= scale_diff;
        block_arena_width >>= scale_diff;
        block_arena_height >>= scale_diff;
    }
    else
    {
        block_size >>= -scale_diff;
        block_arena_width <<= -scale_diff;
        block_arena_height <<= -scale_diff;
    }


    if ( simplified_map_proxyed )
    {
        free( simplified_map_proxyed );
		free( simplified_map );
    }
    auto min_size = block_arena_width * block_arena_height;
    auto total_size = min_size & 7 ? ( ( min_size >> 3 ) + 1 ) << 3 : min_size;
	// makes sure size is multiple of 8 for alignment

    simplified_map_proxyed = reinterpret_cast< char * >( malloc( total_size ) );
    simplified_map = reinterpret_cast< char ** >( malloc( ( block_arena_height + 1 ) * sizeof( char * ) ) );

	for ( int i = 0; i < block_arena_height + 1; i++ )
	{
		simplified_map[ i ] = simplified_map_proxyed + i * block_arena_width;
	}
	
}


static void simplify( std::vector<uint32_t> &pixels, long &bmw, int2 &lc, int2 &rc, char **map, bool apply)
{
    auto off_mid = block_size >> 1;
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

            if ( apply )
            {
                for ( int y = by; y < by + block_size && y < rc.y; ++y ) 
                {
                    for ( int x = bx; x < bx + block_size && x < rc.x; ++x )
                    {
                        pixels[ y * bmw + x ] = ( avgColor >> 4 ) << 4;
                    }
                }
            }
            else
            {
                map[ ( by - lc.y ) >> block_shift_2 ][ ( bx - lc.x ) >> block_shift_2 ] = ( ( ( ( short( r_avg ) + g_avg + b_avg ) / 3 ) & 255 ) >> 4 );
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

static void reconstruct_path( char **map, std::unordered_map<int2, int2> &came_from, int2 &start_node, int2 &lc, int2 &offset, int2 &correction, std::vector<int2> &useful )
{
    int2 current = useful.back();
    useful.pop_back( );
    std::vector<int2> raw = { int2{ lc.x + ( current.x << block_shift_2 ), lc.y + ( current.y << block_shift_2 ) } + offset };

    while ( came_from.contains( current ) )
    {
        auto &t = came_from[ current ];
        current = t;

        raw.push_back( int2( lc.x + ( t.x << block_shift_2 ), lc.y + ( t.y << block_shift_2 ) ) + offset );
        
    }
    std::reverse( raw.begin( ), raw.end( ) );

    if ( ( raw[ 1 ] - raw[ 0 ] ).y )
    {
        useful.push_back( raw[ 0 ] );
    }

	for ( int i = 1; i < raw.size( ) - 1; i++ )
	{
	    auto &current = raw[ i - 1 ];
        auto &p = raw[ i ];
        auto &p2 = raw[ i + 1 ];
        
		if ( p - current != p2 - p )
		{
			useful.push_back( p );
		}
	}
    int2 &penultimate = *( raw.end( ) - 2 );
    int2 &last = *( raw.end( ) - 1 );

    if ( ( penultimate - last ).y )
    {
        last.y = correction.y;
        last.x = penultimate.x;
    }
    else
    {
		last.x = correction.x;
		last.y = penultimate.y;
    }
    useful.push_back( last );
    useful.push_back( correction );

    return;

}

const int2 neighbor_offsets[ ] =
{
    {0,1}, {0,-1}, {1,0}
};
static bool pathfind( char **map, int2 &lc, int2 &start, int2 &end, std::vector<int2> &path )
{
    int2 start_node = { ( start.x - lc.x ) / block_size, ( start.y - lc.y ) / block_size };
	int2 end_node = { ( end.x - lc.x ) / block_size, ( end.y - lc.y ) / block_size };
    int2 offset = { ( start.x - lc.x ) % block_size, ( start.y - lc.y ) % block_size };

	auto open_set = std::vector<int2>{ start_node };

    auto came_from = std::unordered_map<int2, int2>{ };
    auto g_score = std::unordered_map<int2, float>{ };
    auto f_score = std::unordered_map<int2, float>{ };
    g_score[ start_node ] = 0.0f;
    f_score[ start_node ] = static_cast< float > ( sqrt( int2::dist_sq( start_node, end_node ) ) );
    while ( !open_set.empty( ) )
    {
        int2 current;
		int min_f_score = INT_MAX;
        for ( const auto &node : open_set )
        {
            if ( f_score[ node ] < min_f_score )
            {
                current = node;
                min_f_score = static_cast< int >( f_score[ node ] );
            }
		}

        if ( current == end_node )
        {
            path.push_back( end_node );
            reconstruct_path( map, came_from, start_node, lc, offset, end, path );
            return true;
        }
        auto it = std::find( open_set.begin( ), open_set.end( ), current );
        if ( it != open_set.end( ) )
        {
            open_set.erase( it );
        }
        else
        {
            throw std::runtime_error( "item not found?" );
        }

        for ( int i = 0; i < 3; i++ )
        {
			auto neighbor = current + neighbor_offsets[ i ];
            if ( neighbor.y < 0 || neighbor.y >= block_arena_height || neighbor.x < 0 || neighbor.x >= block_arena_width )
            {
				continue; // prevent out of bounds indexing
            }

            if ( map[ neighbor.y ][ neighbor.x ] != 0x0f ) 
            {
				continue; // avoid obstacles
			}
            
            auto tentative_gscore = g_score[ current ] + 0.5f;
            if ( came_from.count(current) && current - came_from[ current ] != neighbor )
            {
                tentative_gscore += 0.5f;
            }
            
            auto current_g = g_score.count( neighbor ) ? g_score[ neighbor ] : INT_MAX;
            if ( tentative_gscore < current_g )
            {
                came_from[ neighbor ] = current;
                g_score[ neighbor ] = tentative_gscore;
                f_score[ neighbor ] = tentative_gscore + static_cast< float > ( sqrt( int2::dist_sq( neighbor, end_node ) ) );
				
                bool found = false;
                
                for ( auto &node : open_set )
                {
                    if( node == neighbor )
                    {
                        found = true;
                        break;
					}
                }
                if( !found )
                {
                    open_set.push_back( neighbor );
				}
            }
        }
    }
	return false; // no path found

}

bool parse_arena( HDC &hdc, bool &found, std::vector<int2> &path, bool view_only )
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

    if ( view_only )
    {
        obstacles_only( pixels, bmw, lc, rc );
        simplify( pixels, bmw, lc, rc, simplified_map, true );
    }
    else if ( find_shooters( pixels, bmw, lc, rc, players, muzzle ) )
    {
        obstacles_only( pixels, bmw, lc, rc );
        simplify( pixels, bmw, lc, rc, simplified_map, false );
        pixels[ muzzle.y * bmw + muzzle.x ] = 0x00FF0000;
        const int border = ( rc.x + lc.x ) >> 1;
        path.push_back( muzzle );
		std::sort( players.begin( ), players.end( ), compare_int2_x );
        for ( auto &player : players )
        {
            if ( player.x < border )
            {
                continue;
            }
            pixels[ player.y * bmw + player.x ] = 0x00FF00FF;

            if ( pathfind( simplified_map, lc, muzzle, player, path ) )
            {
                found = true;
                muzzle = player + int2{ 4, 0 };
                path.push_back( player );
                path.push_back( muzzle );
            }
        }
    }
    
    if ( !SetDIBits( hdc, hbmp, 0, bmp_info.bmHeight, pixels.data( ), &bmi, DIB_RGB_COLORS ) )
    {
        return false;
    }

    return true;
}