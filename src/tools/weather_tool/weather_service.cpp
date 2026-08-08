#include "weather_service.h"
#include "../../utils/http_client.h"
#include "../../utils/url_encode.h"
#include "../../utils/env_utils.h"
#include <nlohmann/json.hpp>
#include <sstream>

WeatherService::WeatherService(){
    api_key_ = getEnvVar("WEATHER_API_KEY");
}

std::expected<std::string, std::string>
WeatherService::getWeather(const std::string& city) const
{
    std::string url =
        "https://api.openweathermap.org/data/2.5/weather?"
        "q=" + agent::utils::urlEncode(city) +
        "&appid=" + api_key_ +
        "&units=metric";
    auto response = agent::utils::HttpClient::get(url);
    if (!response)
        return std::unexpected(response.error());
    try
    {
        auto json = nlohmann::json::parse(response.value());
        std::stringstream ss;
        ss << "City: " << json["name"] << '\n';
        ss << "Weather: " << json["weather"][0]["description"] << '\n';
        ss << "Temperature: " << json["main"]["temp"] << " °C\n";
        ss << "Feels Like: " << json["main"]["feels_like"] << " °C\n";
        ss << "Humidity: " << json["main"]["humidity"] << "%\n";
        ss << "Wind Speed: " << json["wind"]["speed"] << " m/s";
        return ss.str();
    }
    catch (const std::exception& e)
    {
        return std::unexpected(
            std::string("Thất bại khi gọi thời tiết: ") + e.what());
    }
}