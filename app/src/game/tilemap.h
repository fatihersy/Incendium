
#ifndef TILEMAP_H
#define TILEMAP_H

#include <defines.h>

bool create_tilemap(u16 _origin_tilesize, Vector2 _position, u16 _grid_size, u16 _cell_size, Color _grid_color);

void FDrawGrid();

void update_tilemap();
void render_tilemap();

#endif