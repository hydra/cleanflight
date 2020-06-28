#undef USE_BARO_MS5611

#undef USE_SERIALRX_SPEKTRUM
#undef USE_SERIALRX_CRSF       // Team Black Sheep Crossfire protocol
#undef USE_SERIALRX_IBUS       // FlySky and Turnigy receivers
#undef USE_SERIALRX_SUMD       // Graupner Hott protocol

#undef USE_SERVOS

#define USE_RC_SMOOTHING_FILTER

#include "target.h"
