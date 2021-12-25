#include "json.hpp"
#include <string>
#include <iostream>
#include <assert.h>
int main() {
    tiny_json::Json json{};
    std::string arr = "[1,2,3]";
    std::string err{};
    std::cout << "arr size: " << arr.size() << "\n"; 

    auto res = json.parse(arr,err);
    for(auto &n : res.array_items()) {
        std::cout << n.int_value() << "\n";
    }
    tiny_json::Json::array arr1{{1,2,3}};
    std::cout << std::boolalpha << (res == arr1) << "\n";
    std::cout << res.dump() << "\n";

    {
        std::string value = "{\"name\":\"kk\"}";
        auto obj = json.parse(value,err);
        std::cout << obj["name"].string_value() << "\n";
        std::cout << obj.dump() << "\n";
    }
    {
        const tiny_json::Json sub = tiny_json::Json::object({
            { "k1", "v1" },
            { "k2", 42.0 },
            { "k3", tiny_json::Json::array({ "a", 123.0, true, false, nullptr })},
        });
        const tiny_json::Json obj = tiny_json::Json::object({
            { "k1", "v1" },
            { "k2", 42.0 },
            { "k3", tiny_json::Json::array({ "a", 123.0, true, false, nullptr })},
            { "k4", sub},
        });
        std::cout << obj.dump() << "\n";
    }
    
    return 0;
}