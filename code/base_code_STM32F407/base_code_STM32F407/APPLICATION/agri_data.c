/**
 * @file    agri_data.c
 * @brief   环境数据结构与枚举的辅助实现
 */
#include "agri_data.h"
#include "string.h"

void AgriData_Init(AgriData *d)
{
    if (d == 0) {
        return;
    }
    memset(d, 0, sizeof(AgriData));
    d->source = SRC_SENSOR;
}

const char *ControlState_Name(ControlState st)
{
    switch (st) {
        case ST_IDLE:       return "IDLE";
        case ST_IRRIGATING: return "IRRIGATING";
        case ST_COOLDOWN:   return "COOLDOWN";
        case ST_FAULT:      return "FAULT";
        default:            return "UNKNOWN";
    }
}

const char *FaultCode_Name(FaultCode f)
{
    switch (f) {
        case FAULT_NONE:           return "NONE";
        case FAULT_SOIL_RANGE:     return "SOIL_RANGE";
        case FAULT_SOIL_TIMEOUT:   return "SOIL_TIMEOUT";
        case FAULT_SENSOR_TIMEOUT: return "SENSOR_TIMEOUT";
        case FAULT_PUMP_OVERTIME:  return "PUMP_OVERTIME";
        case FAULT_SERIAL_FORMAT:  return "SERIAL_FORMAT";
        default:                   return "UNKNOWN";
    }
}

const char *DataSource_Name(DataSource s)
{
    return (s == SRC_SERIAL) ? "SERIAL" : "SENSOR";
}
