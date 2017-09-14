/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016-2017, djcj <djcj@gmx.de>
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
#include <FL/Fl_Progress.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Double_Window.H>

#include <iostream>
#include <fstream>
#include <string>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "fltk-dialog.hpp"

static Fl_Double_Window *progress_win;
static Fl_Box           *progress_box;
static Fl_Return_Button *progress_but_ok;
static Fl_Button        *progress_but_cancel;
static Fl_Slider        *slider;
static Fl_Progress      *progress_bar;

static int pulsate_val = 0;
static int progress_percent = 0;
static bool progress_pulsate = false;
static bool progress_running = true;
static bool progress_autoclose = false;
static bool progress_hide_cancel = false;
static int progress_kill_pid = -1;

static void progress_close_cb(Fl_Widget *, long p)
{
  progress_win->hide();
  ret = (int) p;
}

static void progress_cancel_cb(Fl_Widget *o)
{
  if (progress_kill_pid > 1)
  {
    kill((pid_t) progress_kill_pid, 1);
  }
  progress_close_cb(o, 1);
}

static int check_pid(void)
{
  if (progress_kill_pid > 1 && kill((pid_t) progress_kill_pid, 0) == -1)
  {
    return 1;
  }
  return 0;
}

static void set_progress_box_label(const char *ch)
{
  if (ch[0] == '#' && ch[1] != '\0')
  {
    progress_box->label(strdup(ch + 1));
  }
  else if (!progress_pulsate && progress_running)
  {
    char tmp[3] = {0};
    char l[5] = {0};
    int i;

    snprintf(tmp, 3, "%s", ch);
    i = atoi(tmp);

    if (i > progress_percent && i <= 100)
    {
      progress_percent = i;
    }
    else if (i > 100)
    {
      progress_percent = 100;
      progress_running = false;
    }

    snprintf(l, 5, "%d%%", progress_percent);
    progress_bar->value(progress_percent);
    progress_bar->label(strdup(l));
  }

  Fl::redraw();
}

extern "C" void *progress_getline(void *p)
{
  const char *watchfile = (char *)p;
  std::fstream fs;
  std::string line;

  if (watchfile == NULL)
  {
    for (/**/; std::getline(std::cin, line); /**/)
    {
      Fl::lock();
      set_progress_box_label(line.c_str());
      Fl::unlock();
      Fl::awake(progress_win);
    }
  }
  else
  {
    fs.open(watchfile, std::fstream::in);
    for (/**/; std::getline(fs, line); /**/)
    {
      Fl::lock();
      set_progress_box_label(line.c_str());
      Fl::unlock();
      Fl::awake(progress_win);
    }
    fs.close();
  }

  return nullptr;
}

extern "C" void *pulsate_bar_thread(void *)
{
  while (progress_running)
  {
    if (progress_percent >= 100)
    {
      pulsate_val = -1;
    }
    else if (progress_percent <= 0)
    {
      pulsate_val = 1;
    }
    progress_percent += pulsate_val;

    Fl::lock();
    slider->value(progress_percent);

    if (check_pid() == 1)
    {
      progress_running = false;
    }

    Fl::unlock();
    Fl::awake(progress_win);

    usleep(10000);
  }

/*
  if (!progress_running)
  {
    if (pulsate)
    {
      slider->value(100);
      slider->deactivate();
    }

    if (autoclose)
    {
      progress_close_cb(nullptr, 0);
    }
    else
    {
      if (!hide_cancel)
      {
        progress_but_cancel->deactivate();
      }
      progress_but_ok->activate();
    }
  }
*/

  return nullptr;
}

extern "C" void *progress_bar_thread(void *)
{
  return nullptr;
}

int dialog_progress(bool pulsate, int kill_pid, std::string watchfile)
{
  Fl_Group *g;
  Fl_Box   *dummy;

  char *watchfile_c = NULL;
  pthread_t progress_thread_1, progress_thread_2;

  if (title == NULL)
  {
    title = "FLTK progress window";
  }

  if (msg == NULL)
  {
    msg = "Progress indicator";
  }

  if (watchfile != "")
  {
    watchfile_c = (char *)watchfile.c_str();
  }

  progress_pulsate = pulsate;
  progress_kill_pid = kill_pid;
  progress_autoclose = autoclose;
  progress_hide_cancel = hide_cancel;

  progress_win = new Fl_Double_Window(320, 140, title);
  progress_win->callback(progress_cancel_cb);
  {
    g = new Fl_Group(0, 0, 320, 140);
    {
      progress_box = new Fl_Box(0, 10, 10, 30, msg);
      progress_box->box(FL_NO_BOX);
      progress_box->align(FL_ALIGN_RIGHT);

      if (pulsate)
      {
        slider = new Fl_Slider(10, 50, 300, 30);
        slider->type(1);
        slider->minimum(0);
        slider->maximum(100);
        slider->color(fl_darker(FL_GRAY));
        slider->value(0);
        slider->slider_size(0.25);
      }
      else
      {
        progress_bar = new Fl_Progress(10, 50, 300, 30, "0%");
        progress_bar->minimum(0);
        progress_bar->maximum(100);
        progress_bar->color(fl_darker(FL_GRAY));
        progress_bar->selection_color(fl_lighter(FL_BLUE));
        progress_bar->labelcolor(FL_WHITE);
        progress_bar->value(0);
      }

      int but_ok_x = 210;

      if (!autoclose)
      {
        if (!hide_cancel)
        {
          progress_but_cancel = new Fl_Button(210, 104, 100, 26, fl_cancel);
          progress_but_cancel->callback(progress_cancel_cb);
          but_ok_x = 100;
        }
        progress_but_ok = new Fl_Return_Button(but_ok_x, 104, 100, 26, fl_ok);
        progress_but_ok->deactivate();
        progress_but_ok->callback(progress_close_cb, 0);
      }
      dummy = new Fl_Box(but_ok_x - 1, 103, 1, 1);
      dummy->box(FL_NO_BOX);
    }
    g->resizable(dummy);
    g->end();
  }
  set_size(progress_win, g);
  set_position(progress_win);
  progress_win->end();

  Fl::lock();

  set_taskbar(progress_win);
  progress_win->show();
  set_undecorated(progress_win);

  if (pulsate)
  {
    pthread_create(&progress_thread_1, 0, &pulsate_bar_thread, nullptr);
  }
  pthread_create(&progress_thread_2, 0, &progress_getline, watchfile_c);

  Fl::run();

  return ret;
}
