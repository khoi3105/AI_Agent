#include "weather_tool.h"
#include "../../utils/env_utils.h"
#include <iostream>

int main()
{

    WeatherTool weather;

    nlohmann::json args = {
        {"city", "Ho Chi Minh"}
    };

    std::cout << weather.execute(args.dump()) << '\n';

    return 0;
}