#ifndef WEATHER_SERVICE_H
#define WEATHER_SERVICE_H

#include <expected>
#include <string>

class WeatherService
{
public:
    explicit WeatherService();
    std::expected<std::string, std::string>
    getWeather(const std::string& city) const;
private:
    std::string api_key_;
};
#endif