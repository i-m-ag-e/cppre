# cppre: A basic regular expression engine based on a virtual-machine implementaion of Thompson's NFA

This is a basic implementation of a regular expression engine based on this article by Russ Cox

[Regular Expression Matching: the Virtual Machine Approach
](https://swtch.com/~rsc/regexp/regexp2.html).


It uses a virtual machine to interpret an NFA constructed from a given regular expression.

## Building
This project can be built using CMake (requires >= C++17).

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

Tests are built by default and can be run using `ctest` in the build directory.

If you wish to disable building tests, you can do so by passing `-DBUILD_TESTS=OFF` to the `cmake` command.

## Supported Features
- Literal characters
- Concatenation
- Alternation (`|`)
- Kleene star (`*`)
- Plus (`+`)
- Optional (`?`)
- Character classes (e.g., `[a-z]`, `[^0-9]`)
- Grouping with parentheses (`()`)
- Grouping with non-capturing parentheses (`(?:...)`)
- Escape sequences (e.g., `\n`, `\t`, `\\`, etc.)
- Shorthand character classes (e.g., `\d`, `\w`, `\s`, etc.)
- Matching an exact string
- Searching for a pattern within a larger string

## Usage
The `Regex` class in `cppre/Regex.h` provides the functions `Regex::match` and `Regex::search` to match and search for patterns in strings. They return an `std::optional<Match>` object that contains information about the match if found.

The `Match` object provides the following methods:
- `bool matched() const`
- `std::string const& str() const`
- `size_t get_begin() const`
- `size_t get_end() const`
- `std::vector<Match> submatches() const`


Example usage:

```cpp
#include "cppre/Regex.h"

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

## TODO
- Anchors (already exist internally)
- Word boundaries
- More comprehensive error handling in the parser
- Quantifiers with specific counts (e.g., `{m,n}`)
