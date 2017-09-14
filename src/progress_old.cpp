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


/*****

# usage example 1:
( \
  sleep 3 && echo '5#Working on it...' && \
  sleep 1 && echo 30 && \
  sleep 2 && echo 91 && \
  sleep 1 && echo '100#Done.' \
) \
>/tmp/log &
pid=$!; ./fltk-dialog --progress --watch-file=/tmp/log --watch-pid=$pid


# usage example 2:
sleep 5 &
pid=$!; ./fltk-dialog --progress --pulsate --watch-pid=$pid


# usage example 3:
( \
  echo '#Working on it...' && \
  sleep 4 && \
  echo '#Working on it... almost finished...' && \
  sleep 2 && \
  echo 'EOL#Working on it... almost finished... Done.' \
) \
>/tmp/log &
./fltk-dialog --progress --pulsate --watch-file=/tmp/log

*****/


#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Double_Window.H>

#include <string>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "fltk-dialog.hpp"

#define FGETS_LIMIT 256
#define INTERVAL_SEC 0.01 /* 10ms; has an effect on pulsating speed */

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
static std::string progress_watchfile;

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

static int check_pid()
{
  if (progress_kill_pid > 1 && kill((pid_t) progress_kill_pid, 0) == -1)
  {
    return 1;
  }
  return 0;
}

static void set_progress_box_label(char *ch)
{
  for (int i = 0; (i <= 3 || ch[i] == '\0'); ++i)
  {
    if (ch[i] == '#')
    {
      char *l = ch + i + 1;
      progress_box->label(strdup(l));
      Fl::redraw();
      i = 3;
    }
  }
}

static void progress_cb(void *)
{
  char line[FGETS_LIMIT];

  if (progress_running)
  {
    std::string command = "test ! -f '" + progress_watchfile + "' || tail -n1 '" + progress_watchfile + "'";
    FILE *stream = popen(command.c_str(), "r");

    if (stream == NULL)
    {
      perror("problems with pipe");
      pclose(stream);
      progress_close_cb(nullptr, 1);
    }
    else
    {
      if (progress_pulsate)
      {
        while (fgets(line, FGETS_LIMIT, stream))
        {
          std::string s(line);
          s = s.substr(0, 3);

          if (s == "END" || s == "EOF" || s == "EOL" || s == "100")
          {
            progress_running = false;
          }

          set_progress_box_label(line);
        }

        if (progress_percent >= 100)
        {
          pulsate_val = -1;
        }
        else if (progress_percent <= 0)
        {
          pulsate_val = 1;
        }

        progress_percent += pulsate_val;
        slider->value(progress_percent);
      }
      else
      {
        while (fgets(line, FGETS_LIMIT, stream))
        {
          std::string s(line);
          s.substr(0, 3);
          int i = atoi(s.c_str());

          if (i > progress_percent && i <= 100)
          {
            progress_percent = i;
          }
          else if (i > 100)
          {
            progress_percent = 100;
          }

          set_progress_box_label(line);
        }

        if (progress_percent == 100)
        {
          progress_running = false;
        }

        char l[5] = {0};
        snprintf(l, 5, "%d%%", progress_percent);
        progress_bar->value(progress_percent);
        progress_bar->label(strdup(l));
      }
    }
    pclose(stream);

    if (check_pid() == 1)
    {
      progress_running = false;
    }
  }
  else
  {
    if (progress_pulsate)
    {
      slider->value(100);
      slider->deactivate();
    }

    if (progress_autoclose)
    {
      progress_close_cb(nullptr, 0);
    }
    else
    {
      if (!progress_hide_cancel)
      {
        progress_but_cancel->deactivate();
      }
      progress_but_ok->activate();
    }
  }

  Fl::repeat_timeout(INTERVAL_SEC, progress_cb);
}

int dialog_progress(bool pulsate, int kill_pid, std::string watchfile, bool autoclose, bool hide_cancel)
{
  Fl_Group *g;
  Fl_Box   *dummy;

  if (title == NULL)
  {
    title = "FLTK progress window";
  }

  if (msg == NULL)
  {
    msg = "Progress indicator";
  }

  progress_pulsate = pulsate;
  progress_kill_pid = kill_pid;
  progress_watchfile = watchfile;
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
  set_taskbar(progress_win);
  progress_win->show();
  set_undecorated(progress_win);

  Fl::add_timeout(INTERVAL_SEC, progress_cb);
  Fl::run();
  return ret;
}
