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

std::string WeatherTool::execute(const nlohmann::json& args)
{
    try
    {
        if (!args.contains("city") || !args["city"].is_string())
            return "Missing required parameter: city";

        auto result = _service.getWeather(args["city"].get<std::string>());

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
        {"type", "tool_call"},
        {"tool_call",
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