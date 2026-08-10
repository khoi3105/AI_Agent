#ifndef WEATHER_TOOL_H
#define WEATHER_TOOL_H

#include "../tool.h"
#include "weather_service.h"

class WeatherTool : public Tool
{
public:
    explicit WeatherTool();
    std::string getName() const override;
    std::string getDescription() const override;
    std::string execute(const nlohmann::json& args) override;
    nlohmann::json get_schema() const override;

private:
    WeatherService _service;
};

#endif