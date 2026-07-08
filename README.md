# cppre: A Regular Expression Engine Based on Thompson's NFA

This is an implementation of a regular expression engine based on Russ Cox's article, [Regular Expression Matching: the Virtual Machine Approach](https://swtch.com/~rsc/regexp/regexp2.html).

It uses a virtual machine to interpret a Non-Deterministic Finite Automaton (NFA) constructed from a given regular expression. 

## Architecture
The engine is structured into distinct phases:
*   **Parsing:** The engine supports two interchangeable parsers to generate the AST. 
    *   A performant recursive descent parser (`ParseHandwritten.cpp`).
    *   A functional parser combinator engine (`ParseParserComb.cpp`) utilizing my custom parsing library [parser_comb](https://github.com/i-m-ag-e/parser_comb).
*   **Execution:** The AST is compiled into bytecode for a custom Virtual Machine, which executes the pattern matching against input strings.

## Building
This project can be built using CMake (requires >= C++20). External dependencies are managed automatically via CMake's `FetchContent`.

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### CMake Options

You can customize the build using the following CMake options:

* `-DCPPRE_USE_PARSER_COMB=ON/OFF` (Default: `OFF`): Toggles the parsing engine. When enabled, it fetches and links the [parser_comb](https://github.com/i-m-ag-e/parser_comb) library to parse the regex. When disabled, it falls back to the internal recursive descent parser.


* `-DBUILD_TESTING=ON/OFF` (Default: `ON`): Controls whether the GoogleTest suite is compiled.



## Supported Features

* Literal characters


* Concatenation


* Alternation (`|`)


* Kleene star (`*`)


* Plus (`+`)


* Optional (`?`)


* Character classes (e.g., `[a-z]`, `[^0-9]`)


* Grouping with parentheses (`()`)


* Grouping with non-capturing parentheses (`(?:...)`)


* Escape sequences (e.g., `\n`, `\t`, `\\`, etc.)


* Shorthand character classes (e.g., `\d`, `\w`, `\s`, etc.)


* Anchors (`^`, `$`, `\A`, `\Z`, `\b`, `\B`)


* Matching an exact string


* Searching for a pattern within a larger string



## Usage

The `Regex` class in `cppre/Regex.h` provides the functions `Regex::match` and `Regex::search` to match and search for patterns in strings. They return an `std::optional<Match>` object that contains information about the match if found.

The `Match` object provides the following methods:

* `bool matched() const`

* `std::string const& str() const`

* `size_t get_begin() const`

* `size_t get_end() const`

* `std::vector<Match> submatches() const`


Example usage:

```cpp
#include "cppre/Regex.h"
#include <iostream>

int main() {
    cppre::Regex regex("a(bc)*d");
    std::string text = "abcbcd";

    auto match = regex.search(text);
    if (match && match->matched()) {
        std::cout << "Matched: " << match->str() << "\n";
        std::cout << "At positions: " << match->get_begin() << " to " << match->get_end() << "\n";

        auto submatches = match->submatches();
        for (size_t i = 0; i < submatches.size(); ++i) {
            std::cout << "Submatch " << i << ": " << submatches[i].str() << "\n";
        }
    } else {
        std::cout << "No match found.\n";
    }

    return 0;
}

```

## Testing

The project includes a test suite covering basic execution, complex searching, matching, and parser validity. Tests are built automatically unless `BUILD_TESTING` is disabled and can be run using `ctest` in the build directory.

## TODO

* More comprehensive error handling in the parser


* Quantifiers with specific counts (e.g., `{m,n}`)



```