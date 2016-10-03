/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016, djcj <djcj@gmx.de>
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
#include <FL/fl_ask.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Multi_Browser.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Window.H>

#include <iostream>
#include <string>

#include "fltk-dialog.hpp"
#include "misc/readstdio.hpp"

static Fl_Window        *textinfo_win;
static Fl_Return_Button *ti_but_ok;
static bool ti_checkbutton_set = false;

static void textinfo_close_cb(Fl_Widget *, long p)
{
  textinfo_win->hide();
  ret = (int) p;
}

static void ti_checkbutton_cb(Fl_Widget *)
{
  if (ti_checkbutton_set == true)
  {
    ti_checkbutton_set = false;
    ti_but_ok->deactivate();
  }
  else
  {
    ti_checkbutton_set = true;
    ti_but_ok->activate();
  }
}

int dialog_textinfo(       bool autoscroll,
                    std::string checkbox)
{
  Fl_Multi_Browser *ti_browser;
  Fl_Group         *buttongroup;
  Fl_Box           *dummy;
  Fl_Check_Button  *checkbutton;
  Fl_Button        *ti_but_cancel;

  std::string line;
  int browser_h, stdin;
  int linecount = 0;

  if (title == NULL)
  {
    title = (char *)"FLTK text info window";
  }

  if (checkbox == "")
  {
    browser_h = 444;
  }
  else
  {
    browser_h = 418;
  }

  textinfo_win = new Fl_Window(400, 500, title);
  {
    ti_browser = new Fl_Multi_Browser(10, 10, 380, browser_h);

    buttongroup = new Fl_Group(0, browser_h, 400, 500);
    {
      int but_x = 300;
      int but_y = browser_h + 20;
      if (checkbox == "")
      {
        textinfo_win->callback(textinfo_close_cb, 0);

        ti_but_ok = new Fl_Return_Button(but_x, but_y, 90, 26, fl_close);
        ti_but_ok->callback(textinfo_close_cb, 0);
      }
      else
      {
        but_x = 200;
        but_y = browser_h + 10;

        textinfo_win->callback(textinfo_close_cb, 1);

        checkbox = " " + checkbox;
        checkbutton = new Fl_Check_Button(10, but_y + 2, 380, 26, checkbox.c_str());
        checkbutton->callback(ti_checkbutton_cb);

        ti_but_ok = new Fl_Return_Button(but_x, but_y + 36, 90, 26, fl_ok);
        ti_but_ok->deactivate();
        ti_but_ok->callback(textinfo_close_cb, 0);
        ti_but_cancel = new Fl_Button(300, but_y + 36, 90, 26, fl_cancel);
        ti_but_cancel->callback(textinfo_close_cb, 1);
      }
      dummy = new Fl_Box(but_x - 1, but_y - 1, 1, 1);
      dummy->box(FL_NO_BOX);
    }
    buttongroup->resizable(dummy);
    buttongroup->end();
  }
  if (resizable)
  {
    textinfo_win->resizable(ti_browser);
  }
  textinfo_win->end();

  READSTDIO(stdin);
  if (stdin)
  {
    textinfo_win->wait_for_expose();
    Fl::flush();
    for (/**/; std::getline(std::cin, line); /**/)
    {
      ti_browser->add(line.c_str());
      if (autoscroll)
      {
        ++linecount;
        ti_browser->bottomline(linecount);
      }
      Fl::check();
    }
  }
  else if (stdin == -1)
  {
    ti_browser->add("error: select()");
  }
  else
  {
    ti_browser->add("error: no input");
  }

  textinfo_win->show();
  Fl::run();

  return ret;
}

