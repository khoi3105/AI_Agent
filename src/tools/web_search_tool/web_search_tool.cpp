
#include "web_search_tool.h"

#include "../../utils/url_encode.h"
#include "../../utils/http_client.h"

#include <gumbo.h>
#include <nlohmann/json.hpp>

#include <string>
#include <vector>

// Parse HTML to title, url, snippet 
namespace {
    struct SearchResult
    {
        std::string title;
        std::string url;
        std::string snippet;
    };

    bool hasClass(const GumboNode* node, const std::string& className) {
        if (node->type != GUMBO_NODE_ELEMENT)
            return false;

        const GumboAttribute* attribute =
            gumbo_get_attribute(&node->v.element.attributes, "class");

        if (!attribute)
            return false;

        return std::string(attribute->value).find(className) !=
               std::string::npos;
    }

    std::string getText(const GumboNode* node)
    {
        if (!node)
            return "";

        if (node->type == GUMBO_NODE_TEXT)
            return node->v.text.text;

        if (node->type != GUMBO_NODE_ELEMENT &&
            node->type != GUMBO_NODE_DOCUMENT)
            return "";

        std::string result;

        const GumboVector* children =
            &node->v.element.children;

        for (unsigned int i = 0; i < children->length; ++i)
        {
            result += getText(
                static_cast<GumboNode*>(children->data[i])
            );
        }

        return result;
    }

    const GumboNode* findElementByClass(
        const GumboNode* node,
        const std::string& tag,
        const std::string& className)
    {
        if (!node)
            return nullptr;

        if (node->type == GUMBO_NODE_ELEMENT)
        {
            if (node->v.element.tag == gumbo_tag_enum(tag.c_str()) &&
                hasClass(node, className))
            {
                return node;
            }

            const GumboVector* children =
                &node->v.element.children;

            for (unsigned int i = 0; i < children->length; ++i)
            {
                const GumboNode* result =
                    findElementByClass(
                        static_cast<GumboNode*>(children->data[i]),
                        tag,
                        className
                    );

                if (result)
                    return result;
            }
        }

        return nullptr;
    }

    void findResultNodes(
        const GumboNode* node,
        std::vector<const GumboNode*>& results)
    {
        if (!node)
            return;

        if (node->type == GUMBO_NODE_ELEMENT)
        {
            if (hasClass(node, "result"))
            {
                results.push_back(node);
            }

            const GumboVector* children =
                &node->v.element.children;

            for (unsigned int i = 0; i < children->length; ++i)
            {
                findResultNodes(
                    static_cast<GumboNode*>(children->data[i]),
                    results
                );
            }
        }
    }

    const GumboNode* findChildByClass(
        const GumboNode* node,
        const std::string& className)
    {
        if (!node)
            return nullptr;

        if (node->type == GUMBO_NODE_ELEMENT &&
            hasClass(node, className))
        {
            return node;
        }

        if (node->type != GUMBO_NODE_ELEMENT)
            return nullptr;

        const GumboVector* children =
            &node->v.element.children;

        for (unsigned int i = 0; i < children->length; ++i)
        {
            const GumboNode* result =
                findChildByClass(
                    static_cast<GumboNode*>(children->data[i]),
                    className
                );

            if (result)
                return result;
        }

        return nullptr;
    }

    const GumboNode* findFirstTag(
        const GumboNode* node,
        GumboTag tag)
    {
        if (!node)
            return nullptr;

        if (node->type == GUMBO_NODE_ELEMENT)
        {
            if (node->v.element.tag == tag)
                return node;

            const GumboVector* children =
                &node->v.element.children;

            for (unsigned int i = 0; i < children->length; ++i)
            {
                const GumboNode* result =
                    findFirstTag(
                        static_cast<GumboNode*>(children->data[i]),
                        tag
                    );

                if (result)
                    return result;
            }
        }

        return nullptr;
    }

    std::vector<SearchResult> parseSearchResults(
        const std::string& html)
    {
        std::vector<SearchResult> results;

        GumboOutput* output =
            gumbo_parse(html.c_str());

        if (!output)
            return results;

        std::vector<const GumboNode*> resultNodes;

        findResultNodes(output->root, resultNodes);

        for (const GumboNode* resultNode : resultNodes)
        {
            const GumboNode* titleNode =
                findChildByClass(resultNode, "result__a");

            const GumboNode* snippetNode =
                findChildByClass(resultNode, "result__snippet");

            if (!titleNode)
                continue;

            const GumboNode* linkNode =
                findFirstTag(resultNode, GUMBO_TAG_A);

            SearchResult result;

            result.title = getText(titleNode);

            if (linkNode)
            {
                const GumboAttribute* href =
                    gumbo_get_attribute(
                        &linkNode->v.element.attributes,
                        "href"
                    );

                if (href)
                    result.url = href->value;
            }

            if (snippetNode)
                result.snippet = getText(snippetNode);

            if (!result.title.empty())
                results.push_back(result);
        }

        gumbo_destroy_output(
            &kGumboDefaultOptions,
            output
        );

        return results;
    }
}

std::string WebSearchTool::getName() const
{
    return "web_search";
}

std::string WebSearchTool::getDescription() const
{
    return "Search the web using DuckDuckGo.";
}

std::string WebSearchTool::execute(const nlohmann::json& args)
{
    try
    {
        std::string query = args["query"].get<std::string>();

        std::string url = "https://html.duckduckgo.com/html/?q=" +
                          agent::utils::urlEncode(query);

        auto response = agent::utils::HttpClient::get(url);

        if (!response)
        {
            return "HTTP request failed: " + response.error();
        }

        auto results = parseSearchResults(response.value());

        nlohmann::json output = nlohmann::json::array();

        for (const auto& result : results)
        {
            output.push_back({
                {"title", result.title},
                {"url", result.url},
                {"snippet", result.snippet}
            });
        }

        return output.dump(2);
    }
    catch (const std::exception& e)
    {
        return std::string("WebSearchTool error: ") + e.what();
    }
}

nlohmann::json WebSearchTool::get_schema() const
{
    return {
        {
            "type", "tool_call"
        },
        {
            "tool_call", {
                {
                    "name", "web_search"
                },
                {
                    "description",
                    "Search the web using DuckDuckGo. ALWAYS use this tool to search the internet/web for general knowledge, external facts, real-time news, or official documentation."
                },
                {
                    "parameters", {
                        {
                            "type", "object"
                        },
                        {
                            "properties", {
                                {
                                    "query", {
                                        {"type", "string"},
                                        {"description", "The search query"}
                                    }
                                }
                            }
                        },
                        {
                            "required", {"query"}
                        }
                    }
                }
            }
        }
    };
}
