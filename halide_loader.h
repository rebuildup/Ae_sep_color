#pragma once

#include <stdbool.h>

// Ensures Halide runtime DLL is loaded on platforms that require it.
// Returns true if Halide is ready to use, false if unavailable.
bool EnsureHalideRuntimeLoaded();


