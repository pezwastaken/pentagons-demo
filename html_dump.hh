#pragma once

#include <cstdio>
#include <map>
#include <string>
#include <format>

template <StateDomain D>
bool html_dump_fixpoint(const CFG& cfg, const FixpointResult<D>& res) {

    using S = typename D::State;

    FILE* f = fopen("result.html", "w");
    if (!f) return false;

    const char* style_stuff = R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Fixpoint Visualizer</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif; 
            display: flex;
            height: 100vh;
            color: #222;
            background: #fff;
        }
        /* Left panel for buttons */
        .sidebar {
            width: 220px;
            border-right: 1px solid #ccc;
            padding: 12px;
            background: #f8f9fa;
            flex-shrink: 0;
            overflow-y: auto;
        }
        .sidebar button {
            display: block;
            width: 100%;
            padding: 10px 12px;
            margin-bottom: 8px;
            text-align: left;
            font-size: 14px;
            border: 1px solid #ccc;
            background: #fff;
            cursor: pointer;
            border-radius: 4px;
        }
        .sidebar button:hover { background: #e9ecef; }
        .sidebar button.active { background: #0066cc; color: #fff; border-color: #0066cc; }

        /* Right scrollable content panel */
        .content-panel {
            flex-grow: 1;
            padding: 32px 40px; /* Generous spacing between buttons and content */
            overflow-y: auto;
            line-height: 1.6;
        }
        .content-section { display: none; }
        .content-section.active { display: block; }
        .content-section h2 { margin-bottom: 12px; }
        .content-section p {
            margin-bottom: 16px;
            white-space: pre-wrap; /* Preserves formatting/newlines from state dumps */
            font-family: monospace; /* Great for dump visualization */
        }
    </style>
</head>
<body>
)";
    fputs(style_stuff, f);

    fputs("<div class=\"sidebar\">\n", f);
    bool is_first = true;

    // TODO: print the nodes in order

    for (const auto& [id, S] : res.node_states) {
        std::string b = std::format(
            "  <button onclick=\"showSection('section{}')\" id=\"btn-section{}\" class=\"{}\">node {}</button>\n",
            id, id, is_first ? "active" : "", id
        );
        fputs(b.c_str(), f);
        is_first = false;
    }
    fputs("</div>\n", f);

    fputs("<div class=\"content-panel\">\n", f);
    is_first = true;
    for (const auto& [id, s] : res.node_states) {
        std::string div_head = std::format(
            "  <div id=\"section{}\" class=\"content-section {}\">\n",
            id, is_first ? "active" : ""
        );
        fputs(div_head.c_str(), f);

        std::string content = "    <h2>Node " + std::to_string(id) + "</h2>\n    <p>" + D::dump_state(s) + "</p>\n";
        fputs(content.c_str(), f);

        fputs("  </div>\n", f);
        is_first = false;
    }

    const char* tail = R"(</div>
<script>
function showSection(sectionId) {
    const sections = document.querySelectorAll('.content-section');
    sections.forEach(section => { section.classList.remove('active'); });

    const buttons = document.querySelectorAll('.sidebar button');
    buttons.forEach(button => { button.classList.remove('active'); });

    const targetSection = document.getElementById(sectionId);
    if (targetSection) { targetSection.classList.add('active'); }

    const targetButton = document.getElementById('btn-' + sectionId);
    if (targetButton) { targetButton.classList.add('active'); }
}
</script>
</body>
</html>
)";
    fputs(tail, f);

    fclose(f);
    return true;
}
