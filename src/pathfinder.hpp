#include "helpers.hpp"

extern int scale_level;

void set_simplified_scale( int new_scale );
bool parse_arena( HDC &hdc, bool &found, std::vector<int2> &path, bool view_only );