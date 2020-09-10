//
// Created by Orange Juice on 27-08-2020.
//

#include "xml_to_dot.hpp"

auto main() -> int{
     constexpr auto xmlFileName = std::string_view{"metadata.xml"};
    return run(xmlFileName);
}

