#include "weather_tool.h"

#include <nlohmann/json.hpp>

WeatherTool::WeatherTool() : _service(){}

std::string WeatherTool::getName() const
{
    return "weather";
}

std::string WeatherTool::getDescription() const
{
    return "Get current weather information by city name.";
}

std::string WeatherTool::execute(const std::string& args)
{
    try
    {
        auto json = nlohmann::json::parse(args);

        if (!json.contains("city"))
            return "Missing required parameter: city";

        auto result = _service.getWeather(json["city"]);

        if (!result)
            return result.error();

        return result.value();
    }
    catch (const std::exception& e)
    {
        return std::string("WeatherTool Error: ") + e.what();
    }
}

nlohmann::json WeatherTool::get_schema() const
{
    return {
        {"type", "function"},
        {"function",
            {
                {"name", getName()},
                {"description", getDescription()},
                {"parameters",
                    {
                        {"type", "object"},
                        {"properties",
                            {
                                {"city",
                                    {
                                        {"type", "string"},
                                        {"description", "City name"}
                                    }
                                }
                            }
                        },
                        {"required", {"city"}}
                    }
                }
            }
        }
    };
}