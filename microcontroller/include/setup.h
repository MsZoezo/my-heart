#ifndef SETUP_H
#define SETUP_H

#include <stdbool.h>

/*
* Check if we need to run the setup.
* @returns If we should.
*/
bool setup_should_run();

void setup_run();

#endif