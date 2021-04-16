/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2021 Winlin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <srs_core.hpp>

_SrsContextId::_SrsContextId()
{
}

_SrsContextId::_SrsContextId(const _SrsContextId& cp)
{
    v_ = cp.v_;
}

_SrsContextId& _SrsContextId::operator=(const _SrsContextId& cp)
{
    v_ = cp.v_;
    return *this;
}

_SrsContextId::~_SrsContextId()
{
}

const char* _SrsContextId::c_str() const
{
    return v_.c_str();
}

bool _SrsContextId::empty() const
{
    return v_.empty();
}

int _SrsContextId::compare(const _SrsContextId& to) const
{
    return v_.compare(to.v_);
}

_SrsContextId& _SrsContextId::set_value(const std::string& v)
{
    v_ = v;
    return *this;
}

