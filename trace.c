#include "trace_lbr.h"
#include "trace_lwp.h"
#include "kolibri.h"

void trace_begin() {
    if (get_lwp_event_size() > 0) {
        trace_lwp_begin();
    } else {
        trace_lbr_begin();
    }
}

void trace_end() {
    if (get_lwp_event_size() > 0) {
        trace_lwp_end();
    } else {
        trace_lbr_end();
    }
}
