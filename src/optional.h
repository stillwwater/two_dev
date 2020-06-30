// Copyright (c) 2020 stillwwater
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#ifndef TWO_OPTIONAL_H
#define TWO_OPTIONAL_H

#include <utility>

#include "debug.h"

namespace two {

// A simple optional type in order to support C++ 11.
template <typename T>
class Optional {
public:
    bool has_value;

    Optional(const T &value) : has_value{true}, val{value} {}
    Optional(T &&value) : has_value{true}, val{std::move(value)} {}
    Optional() : has_value{false} {}

    T &&value() {
        ASSERT(has_value);
        return std::move(val);
    }

private:
    T val;
};

} // two

#endif // TWO_OPTIONAL_H
