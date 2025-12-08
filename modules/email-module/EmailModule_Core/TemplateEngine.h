#pragma once
#include <string>
#include <map>

std::string RenderTemplate(
    const std::string& tpl,
    const std::map<std::string, std::string>& vars
);
