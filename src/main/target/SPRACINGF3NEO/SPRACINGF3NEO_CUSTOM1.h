
#undef USE_BRUSHED_ESC_AUTODETECT // NEO is a PDB

#undef USE_PPM
//#undef USE_SERIALRX_SPEKTRUM
#undef USE_SERIALRX_CRSF        // CRSF wasn't around when the NEO was released.
//#undef USE_SERIALRX_IBUS
//#undef USE_SERIALRX_SUMD
//#undef USE_SERIALRX_SBUS

// > 64KB
#undef USE_SERVOS

// #12
#undef USE_MSP_DISPLAYPORT
#undef USE_MSP_OVER_TELEMETRY
#undef USE_OSD_OVER_MSP_DISPLAYPORT

// #10
#undef USE_VIRTUAL_CURRENT_METER  // NEO has a current meter

// #9
#define USE_GYRO_LPF2

// #8
#define USE_DYN_LPF
#define USE_D_MIN

// #6
#define USE_ITERM_RELAX
#define USE_RC_SMOOTHING_FILTER
#define USE_THRUST_LINEARIZATION
#define USE_TPA_MODE

// > 256
#define USE_CMS_FAILSAFE_MENU
#define USE_BATTERY_VOLTAGE_SAG_COMPENSATION

#include "target.h"

// Disable settings that the target.h file might enable.
#undef USE_BARO
#undef USE_BARO_BMP280
#undef USE_BARO_MS5611
