#include "core/event.h"

#include "core/fmemory.h"
#include <vector>

typedef struct registered_event {
    PFN_on_event callback;
} registered_event;

typedef struct event_code_entry {
    std::vector<registered_event> events;
} event_code_entry;


#define MAX_MESSAGE_CODES 16384


typedef struct event_system_state {
    event_code_entry registered[MAX_MESSAGE_CODES];
} event_system_state;

static event_system_state* state_ptr;

bool event_system_initialized = false;

void event_system_initialize(void) {
    if (event_system_initialized) return;
    
    state_ptr = (event_system_state*)allocate_memory_linear(sizeof(event_system_state), true);
    event_system_initialized = true;
}

void event_system_shutdown(void) {
    if (state_ptr) {
        for (u16 i = 0; i < MAX_MESSAGE_CODES; ++i) {
            // Vector's destructor will handle cleanup automatically
            state_ptr->registered[i].events.clear();
        }
    }
    state_ptr = 0;
}

bool event_register(u16 code, PFN_on_event on_event) {
    if (!state_ptr) {
        return false;
    }
    
    registered_event event;
    event.callback = on_event;
    state_ptr->registered[code].events.push_back(event);
    return true;
}

bool event_unregister(u16 code, PFN_on_event on_event) {
    if (!state_ptr) {
        return false;
    }
    
    std::vector<registered_event>& events = state_ptr->registered[code].events;
    if (events.empty()) {
        // TODO: warn
        return false;
    }
    
    for (size_t i = 0; i < events.size(); ++i) {
        if (events[i].callback == on_event) {
            events.erase(events.begin() + i);
            return true;
        }
    }

    return false;
}

bool event_fire(u16 code, event_context context) {
    if (!state_ptr) {
        return false;
    }

    const std::vector<registered_event>& events = state_ptr->registered[code].events;
    if (events.empty()) {
        return false;
    }

    for (size_t i = 0; i < events.size(); ++i) {
        if (events[i].callback(code, context)) {
            return true;
        }
    }

    return false;
}
