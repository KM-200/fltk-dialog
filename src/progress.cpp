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

// Tests:
// (echo 40; sleep 1; echo 80; sleep 1; echo 99; sleep 1; echo '#Done' && echo '100') | ./fltk-dialog --progress
// (echo '#Work in progress... ' && sleep 4 && echo '#Work in progress... done.' && echo 'STOP') | ./fltk-dialog --progress --pulsate

/*
(echo '#1/3'; echo 40; sleep 1; echo 80; sleep 1; echo 100; sleep 1; \
 echo '#2/3'; echo 40; sleep 1; echo 80; sleep 1; echo 100; sleep 1; \
 echo '#3/3'; echo 40; sleep 1; echo 80; sleep 1; echo '#Done.'; echo 100) | \
 ./fltk-dialog --progress --multi=3
*/

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Double_Window.H>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "fltk-dialog.hpp"

#define PULSATE_USLEEP 10000

class nograb_slider : public Fl_Slider
{
public:
  nograb_slider(int X, int Y, int W, int H)
      : Fl_Slider(X, Y, W, H) { }

  virtual ~nograb_slider() { }

  int handle(int) {
    /* don't do anything if
     * we grab the slider */
    return 0;
  };
};

static nograb_slider    *slider = NULL;
static Fl_Double_Window *win = NULL;
static Fl_Box           *box = NULL;
static Fl_Return_Button *but_ok = NULL;
static Fl_Button        *but_cancel = NULL;
static Fl_Progress      *bar = NULL, *bar_main = NULL;
static int ret = 1;

static int pulsate_val = 0;

static
unsigned int percent = 0
,            multi = 1
,            multi_percent = 0
,            iteration = 0;

static bool running = true
,           pulsate = false
,           autoclose = false
,           hide_cancel = false;

static long kill_pid = -1;

static void close_cb(Fl_Widget *, long p) {
  win->hide();
  ret = (int) p;
}

static void cancel_cb(Fl_Widget *o) {
  if (kill_pid > 1) {
    kill((pid_t) kill_pid, 1);
  }
  close_cb(o, 1);
}

static void progress_finished(void) {
  if (autoclose) {
    close_cb(nullptr, 0);
  } else {
    if (!hide_cancel) {
      but_cancel->deactivate();
    }
    but_ok->activate();
  }

  if (pulsate) {
    slider->value(100);
    slider->deactivate();
  }
}

static void parse_line(const char *ch)
{
  if (running) {
    if (ch[0] == '#' && ch[1] != '\0') {
      /* "#comment" line found, change the label */
      box->copy_label(ch + 1);
    } else if (!pulsate && ch[0] >= '0' && ch[0] <= '9') {
      /* number found, update the progress bar */
      std::stringstream ss;
      std::string l1, l2;
      percent = atoi(ch);
      if (percent >= 100) {
        percent = 100;
        running = (multi > 1) ? true : false;
        iteration++;
      }
      ss << percent;
      l1 = ss.str() + "%";
      bar->value(percent);
      bar->copy_label(l1.c_str());

      /* update the main progress bar too if --multi=n was given */
      if (multi > 1) {
        if (percent == 100) {
          /* reset % for next iteration */
          percent = 0;
        }
        multi_percent = iteration * 100 + percent;
        if (multi_percent >= multi * 100) {
          multi_percent = multi * 100;
          running = false;
        }
        ss.str(std::string());
        ss << (multi_percent / multi);
        l2 = ss.str() + "%";
        bar_main->value(multi_percent);
        bar_main->copy_label(l2.c_str());
      }
    } else if (pulsate && strcmp(ch, "STOP") == 0) {
      /* stop now */
      running = false;
    }
  }

  /* should be a separate "if"-statement, so we can finish
   * immediately after a "STOP" signal was sent */
  if (!running) {
    progress_finished();
  }

  Fl::redraw();
}

extern "C" void *progress_getline(void *)
{
  std::string line;
  while (std::getline(std::cin, line)) {
    Fl::lock();
    parse_line(line.c_str());
    Fl::unlock();
    Fl::awake(win);
  }
  return nullptr;
}

extern "C" void *pulsate_bar_thread(void *)
{
  while (running) {
    if (percent >= 100) {
      pulsate_val = -1;  /* move from right to left */
    } else if (percent <= 0) {
      pulsate_val = 1;  /* move from left to right */
    }
    percent += pulsate_val;

    Fl::lock();
    slider->value(percent);

    if (kill_pid > 1 && kill((pid_t) kill_pid, 0) == -1) {
      running = false;  /* the watched process has stopped */
    }

    Fl::unlock();
    Fl::awake(win);

    /* has an effect on the pulsate speed */
    usleep(PULSATE_USLEEP);
  }

  if (!running) {
    progress_finished();
    Fl::redraw();
  }

  return nullptr;
}

int dialog_progress(bool pulsate_, unsigned int multi_, long kill_pid_, bool autoclose_, bool hide_cancel_)
{
  Fl_Group *g;
  Fl_Box *dummy;
  int h = 140, offset = 0;

  if (!title) {
    title = "FLTK progress window";
  }

  if (!msg) {
    msg = "Progress indicator";
  }

  pulsate = pulsate_;
  multi = pulsate ? 1 : multi_;
  kill_pid = kill_pid_;
  autoclose = autoclose_;
  hide_cancel = hide_cancel_;

  if (hide_cancel && autoclose) {
    h -= 36;
  }

  if (multi > 1) {
    offset = 40;
  }

  win = new Fl_Double_Window(320, h + offset, title);
  win->callback(cancel_cb);
  {
    g = new Fl_Group(0, 0, 320, h + offset);
    {
      box = new Fl_Box(0, 10, 10, 30, msg);
      box->box(FL_NO_BOX);
      box->align(FL_ALIGN_RIGHT);

      if (pulsate) {
        slider = new nograb_slider(10, 50, 300, 30);
        slider->type(1);
        slider->minimum(0);
        slider->maximum(100);
        slider->color(fl_darker(FL_GRAY));
        slider->value(0);
        slider->slider_size(0.1);
      } else {
        if (multi > 1) {
          bar_main = new Fl_Progress(10, 50, 300, 30, "0%");
          bar_main->minimum(0);
          bar_main->maximum(multi * 100);
          bar_main->color(fl_darker(FL_GRAY));
          bar_main->selection_color(fl_lighter(FL_BLUE));
          bar_main->labelcolor(FL_WHITE);
          bar_main->value(0);
        }
        bar = new Fl_Progress(10, 50 + offset, 300, 30, "0%");
        bar->minimum(0);
        bar->maximum(100);
        bar->color(fl_darker(FL_GRAY));
        bar->selection_color(fl_lighter(FL_BLUE));
        bar->labelcolor(FL_WHITE);
        bar->value(0);
      }

      if (hide_cancel && autoclose) {
        dummy = new Fl_Box(10, 81 + offset, 300, 1);
      } else {
        int but_w = 0;
        int but_x = win->w() - 10;

        if (!hide_cancel) {
          but_w = measure_button_width(fl_cancel, 20);
          but_cancel = new Fl_Button(win->w() - 10 - but_w, 104 + offset, but_w, 26, fl_cancel);
          but_cancel->callback(cancel_cb);
          but_x = but_cancel->x() - 1;
        }

        if (!autoclose) {
          but_w = measure_button_width(fl_ok, 40);
          but_x = hide_cancel ? win->w() : but_cancel->x();
          but_ok = new Fl_Return_Button(but_x - 10 - but_w, 104 + offset, but_w, 26, fl_ok);
          but_ok->deactivate();
          but_ok->callback(close_cb, 0);
          but_x = but_ok->x() - 1;
        }

        dummy = new Fl_Box(but_x, 103 + offset, 1, 1);
      }
      dummy->box(FL_NO_BOX);
    }
    g->resizable(dummy);
    g->end();
  }
  set_size(win, g);
  set_position(win);
  win->end();

  Fl::lock();

  set_taskbar(win);
  win->show();
  set_undecorated(win);
  set_always_on_top(win);

  pthread_t t1, t2;

  if (pulsate) {
    /* the pulsating bar is running in its own thread */
    pthread_create(&t1, 0, &pulsate_bar_thread, nullptr);
  }
  pthread_create(&t2, 0, &progress_getline, nullptr);

  Fl::run();

  return ret;
}

