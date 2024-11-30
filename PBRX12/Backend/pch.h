#pragma once
#define WIN32_LEAN_AND_MEAN

#include "iostream"


//Graphics && Rendering
#include "DirectXHelpers.h"


//lib --> Gateware
#include "Gateware.h"

#define GATEWARE_ENABLE_CORE // All libraries need this
#define GATEWARE_ENABLE_SYSTEM // Graphics libs require system level libraries
#define GATEWARE_ENABLE_GRAPHICS // Enables all Graphics Libraries

#define GATEWARE_DISABLE_GDIRECTX11SURFACE 
#define GATEWARE_DISABLE_GOPENGLSURFACE 
#define GATEWARE_DISABLE_GVULKANSURFACE 
#define GATEWARE_DISABLE_GRASTERSURFACE 

using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;

