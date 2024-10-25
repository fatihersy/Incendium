#include "collision.h"

collision* collision_system_initialize(i16 amouth) {

    collision* coll = (collision*)malloc(sizeof(collision));
    coll->rect = (Rectangle*)malloc(amouth * sizeof(Rectangle));

    coll->is_active = true;

    return coll;
}

collision* resize_collision_rect(collision* _collision, i16 amouth) 
{
    collision temp = *_collision;
    free(_collision->rect);

    _collision->rect = (Rectangle*)malloc(amouth *  sizeof(Rectangle));
    _collision->amount = amouth;

    for (size_t i = 0; i < temp.amount; i++)
    {
        _collision->rect[i] = temp.rect[i];
    }
    
    return _collision;
}
