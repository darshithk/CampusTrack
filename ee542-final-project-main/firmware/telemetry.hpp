#ifndef _TELEMETRY_HPP_H_
#define _TELEMETRY_HPP_H_

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

class TelemetryEntry {

    long datetime;
    double lat;
    double lon;
    double speed;

    std::string device_id;

public:
    TelemetryEntry(std::string device_id) : device_id(device_id), datetime(0), lat(0), lon(0), speed(0) {}
    TelemetryEntry(std::string device_id, long datetime, double lat, double lon, double speed) : device_id(device_id), datetime(datetime), lat(lat), lon(lon), speed(speed) {}

    void setValues(long datetime, double lat, double lon, double speed) {
        this->datetime = datetime;
        this->lat = lat;
        this->lon = lon;
        this->speed = speed;
    }

    int toBuffer(char* buf, std::size_t buf_size) {
        return snprintf(buf, buf_size, "%s,%lu,%lf,%lf,%lf", device_id.c_str(), datetime, lat, lon, speed);
    }

};

#endif // _TELEMETRY_HPP_H_
