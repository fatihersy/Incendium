#include "core/event.h"

#include "core/fmemory.h"
#include "containers/darray.h"

typedef struct registered_event {
    PFN_on_event callback;
} registered_event;

typedef struct event_code_entry {
    registered_event* events;
} event_code_entry;


#define MAX_MESSAGE_CODES 16384


typedef struct event_system_state {
    
    event_code_entry registered[MAX_MESSAGE_CODES];
} event_system_state;

static event_system_state* state_ptr;

bool event_system_initialized = false;

void event_system_initialize() {
    if (event_system_initialized) return;
    
    state_ptr = (event_system_state*)allocate_memory_linear(sizeof(event_system_state), true);

}

void event_system_shutdown() {
    if (state_ptr) {
        for (u16 i = 0; i < MAX_MESSAGE_CODES; ++i) {
            if (state_ptr->registered[i].events != 0) {
                darray_destroy(state_ptr->registered[i].events);
                state_ptr->registered[i].events = 0;
            }
        }
    }
    state_ptr = 0;
}

bool event_register(u16 code, PFN_on_event on_event) {
    if (!state_ptr) {
        return false;
    }
    if (state_ptr->registered[code].events == 0) {
        state_ptr->registered[code].events = darray_create(registered_event);
    }
    registered_event event;
    event.callback = on_event;
    darray_push(state_ptr->registered[code].events, event);
    return true;
}

bool event_unregister(u16 code, PFN_on_event on_event) {
    if (!state_ptr) {
        return false;
    }
    if (state_ptr->registered[code].events == 0) {
        // TODO: warn
        return false;
    }
    u64 registered_count = darray_length(state_ptr->registered[code].events);
    for (u64 i = 0; i < registered_count; ++i) {
        registered_event e = state_ptr->registered[code].events[i];
        if (e.callback == on_event) {
            registered_event popped_event;
            darray_pop_at(state_ptr->registered[code].events, i, &popped_event);
            return true;
        }
    }


    return false;
}

bool event_fire(u16 code, event_context context) {
    if (!state_ptr) {
        return false;
    }

    if (state_ptr->registered[code].events == 0) {
        return false;
    }

    u64 registered_count = darray_length(state_ptr->registered[code].events);
    for (u64 i = 0; i < registered_count; ++i) {
        registered_event e = state_ptr->registered[code].events[i];
        if (e.callback(code, context)) {
            return true;
        }
    }

    return false;
}
