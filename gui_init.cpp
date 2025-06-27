#include "gui.hpp"

HBRUSH   brush_background;
HBRUSH   brush_red;
HBRUSH   brush_transparent;
HPEN     pen_background;
HPEN     pen_green;
HDC      hdc_mem;
HBITMAP  hbm_mem;

void init_resources( )
{
    brush_background = CreateSolidBrush( COLOR_BG );
    pen_background = CreatePen( PS_SOLID, 1, COLOR_BG );
    pen_green = CreatePen( PS_SOLID, 1, 0x00'FF'00 );
    brush_red = ( HBRUSH ) CreateSolidBrush( RGB( 255, 0, 0 ) );
    brush_transparent = CreateSolidBrush( 0xdedbef );

}

void cleanup_resources( )
{
    DeleteObject( brush_background );
    DeleteObject( brush_red );
    DeleteObject( pen_green );
}

void init_double_buffer( HWND hWnd )
{
    init_resources( );

    HDC hdc_win = GetDC( hWnd );
    hdc_mem = CreateCompatibleDC( hdc_win );
    hbm_mem = CreateCompatibleBitmap( hdc_win, target_width, target_height );
    SelectObject( hdc_mem, hbm_mem );
    ReleaseDC( hWnd, hdc_win );

    RECT rc = { 0, 0, target_width, target_height };
    FillRect( hdc_mem, &rc, brush_transparent );

}

void cleanup_double_buffer( ) 
{
    DeleteDC( hdc_mem );
    DeleteObject( hbm_mem );
    cleanup_resources( );
}

unsigned long __stdcall init_gui( LPVOID lparam ) 
{
    HINSTANCE hInst = reinterpret_cast< HINSTANCE >( lparam );

    WNDCLASSEX wc = { sizeof( WNDCLASSEX ) };
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"loading_window";
    RegisterClassExW( &wc );

    int screen_x = ( GetSystemMetrics( SM_CXSCREEN ) - initial_width ) / 2;
    int screen_y = ( GetSystemMetrics( SM_CYSCREEN ) - initial_height ) / 2;
    g_hwnd = CreateWindowExW(
        WS_EX_LAYERED,
        wc.lpszClassName,
        L"",
        WS_POPUP | WS_VISIBLE,
        screen_x, screen_y,
        initial_width, initial_height,
        nullptr, nullptr, hInst, nullptr
    );

    SetLayeredWindowAttributes( g_hwnd, 0xdedbef, 255, LWA_COLORKEY );
    init_double_buffer( g_hwnd );
    SetTimer( g_hwnd, 1, DWORD( timer_interval ), nullptr );


    MSG msg;
    while ( GetMessageW( &msg, nullptr, 0, 0 ) ) 
    {
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
        if ( full_exit )
        {
            printf( "exiting\n" );
            exit( 0 );
        }
    }
    return 0;
}

HWND     g_hwnd;
LRESULT CALLBACK wnd_proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) 
{
    switch ( msg ) 
    {
        case WM_TIMER: 
        {
            InvalidateRect( hWnd, nullptr, FALSE );
            return 0;
        }

        case WM_ERASEBKGND:
        {
            return 1;
        }
        case WM_SETCURSOR:
        {
            SetCursor( LoadCursor( NULL, IDC_ARROW ) );
            return TRUE;
        }

        case WM_PAINT: 
        {
            delta_time = GetTickCount64( ) - last_render;
            last_render = GetTickCount64( );

            PAINTSTRUCT ps;
            HDC hdc_win = BeginPaint( hWnd, &ps );
            BitBlt( hdc_win, 0, 0, target_width, target_height, hdc_mem, 0, 0, SRCCOPY );

            SelectObject( hdc_mem, brush_background );

            if ( is_resizing )
            {
                auto offset_x = ( target_width - curr_width ) / 2;
                auto offset_y = ( target_height - curr_height ) / 2;
                RoundRect( hdc_mem, offset_x, offset_y, offset_x + curr_width, offset_y + curr_height, 20, 20 );
            }
            else
            {
                RoundRect( hdc_mem, 0, 0, curr_width, curr_height, 20, 20 );
                SetTextColor( hdc_mem, COLOR_WHITE );
                SetBkMode( hdc_mem, TRANSPARENT );
                draw_frame( hdc_mem );
            }

            EndPaint( hWnd, &ps );
            return 0;
        }

        case WM_CLOSE:
        case WM_DESTROY:
        {

            full_exit = true;
            KillTimer( hWnd, 1 );
            cleanup_double_buffer( );
            PostQuitMessage( 0 );
            return 0;
        }
        case WM_NCHITTEST:
        {
            LRESULT hit = DefWindowProcW( hWnd, msg, wParam, lParam );
            switch ( hit )
            {
                case HTNOWHERE:
                case HTRIGHT:
                case HTLEFT:
                case HTTOPLEFT:
                case HTTOP:
                case HTTOPRIGHT:
                case HTBOTTOMRIGHT:
                case HTBOTTOM:
                case HTBOTTOMLEFT:
                    return hit;
            }
            POINT pt = { ( int ) ( short ) LOWORD( lParam ), ( int ) ( short ) HIWORD( lParam ) };
            ScreenToClient( hWnd, &pt );

            if ( pt.x >= 0 && pt.x < ( curr_width * 0.75 ) && pt.y >= 0 && pt.y < 20 )
                return HTCAPTION;
            //}
            return hit;
        }

        default:
            return DefWindowProcW( hWnd, msg, wParam, lParam );
    }
}