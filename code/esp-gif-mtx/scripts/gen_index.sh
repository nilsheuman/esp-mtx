#!/bin/bash

# Read the HTML file and escape quotes for C++ string
html_content=$(cat resources/index.html | sed 's/\\/\\\\/g')

# Write the header file with proper escaping
cat > src/index_html.h << EOL
#ifndef INDEX_HTML_H
#define INDEX_HTML_H

// THIS FILE HAS BEEN GENERATED, DO NOT EDIT!
// This files has been generated from resources/index.html with scripts/gen_index.sh
// Important: \`)"\` has to be written with a space like \`) "\` or escaping breaks, only the end of the html code should be written like \`)\`;
const char* htmlCode = R"(
$html_content
)";

#endif
EOL

echo "Generated src/index_html.h from resources/index.html"