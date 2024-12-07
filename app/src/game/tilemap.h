
#ifndef TILEMAP_H
#define TILEMAP_H

#include <defines.h>

bool create_tilemap(Vector2 position, u16 grid_size, u16 cell_size, Color grid_color);

void FDrawGrid();

void update_tilemap();
void render_tilemap();

#endif