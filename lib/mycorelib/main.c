#include "mycorelib.h"


// TODO 3: import the section's endpoints
extern const struct lib_tab my_section_start;
extern const struct lib_tab my_section_end;

// TODO 3: write a traversing macro for the session
//
#define for_each_entry(iter)                                    \
        for (iter = &my_section_start;                          \
                iter < &my_section_end;                         \
                iter++)

void my_init_function(void)
{
	const struct lib_tab *iter;

    for_each_entry(iter) {
        (iter->init_func)();
    }
	// TODO 3: Iterate and call only the init function
}


void my_end_function(void)
{
	const struct lib_tab *iter;
	
    for_each_entry(iter) {
        (iter->end_func)();
    }
	// TODO 3: Iterate and call only the end function
}
