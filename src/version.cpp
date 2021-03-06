/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016-2018, djcj <djcj@gmx.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <FL/Fl.H>
#include <string>

#ifdef FLTK_STATIC

#include <FL/Enumerations.H>
#include "fltk-dialog.hpp"

std::string get_fltk_version() {
  return XSTRINGIFY(FL_MAJOR_VERSION) "." XSTRINGIFY(FL_MINOR_VERSION) "." XSTRINGIFY(FL_PATCH_VERSION);
}

#else  /* !FLTK_STATIC */

#include <iostream>
#include <sstream>

std::string get_fltk_version()
{
  int version, major, minor, patch;
  std::stringstream ss;

  version = Fl::api_version();
  major = version / 10000;
  minor = (version % 10000) / 100;
  patch = version % 100;

  ss << major << "." << minor << "." << patch;
  return ss.str();
}

#endif  /* !FLTK_STATIC */

