#include "TemplateEngine.h"

std::string RenderTemplate(
    const std::string& tpl,
    const std::map<std::string, std::string>& vars
)
{
    std::string result;
    result.reserve(tpl.size()); // 简单预留一点空间，减少扩容次数

    const std::size_t n = tpl.size();
    std::size_t i = 0;

    while (i < n)
    {
        // 匹配 ${...} 形式的占位符
        if (tpl[i] == '$' && i + 1 < n && tpl[i + 1] == '{')
        {
            std::size_t closePos = tpl.find('}', i + 2);
            if (closePos != std::string::npos)
            {
                // 提取 key
                std::string key = tpl.substr(i + 2, closePos - (i + 2));

                auto it = vars.find(key);
                if (it != vars.end())
                {
                    // 找到了对应变量 → 用它的值替换整个 ${key}
                    result += it->second;
                }
                else
                {
                    // 没找到对应变量 → 原样保留 ${key}
                    result.append(tpl, i, closePos - i + 1);
                }

                i = closePos + 1;
                continue;
            }
            // 没找到 '}'，就把当前字符当普通字符处理
        }

        // 普通字符，直接复制
        result.push_back(tpl[i]);
        ++i;
    }

    return result;
}
