#include "url_encode.h"
#include <curl/curl.h>

namespace agent::utils
{

std::string urlEncode(const std::string& text)
{
    CURL* curl = curl_easy_init();

    if (!curl)
        return text;

    char* encoded = curl_easy_escape(
        curl,
        text.c_str(),
        static_cast<int>(text.size()));

    if (encoded == nullptr)
    {
        curl_easy_cleanup(curl);
        return text;
    }

    std::string result(encoded);

    curl_free(encoded);
    curl_easy_cleanup(curl);

    return result;
}

} // namespace agent::utils