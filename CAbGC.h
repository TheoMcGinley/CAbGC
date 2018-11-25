#ifndef CABGC_H
#define CABGC_H

// INCLUDES {{{
#include <X11/Xlib.h>
#include <stdio.h>  // used for: printf
#include <unistd.h> // used for: NULL, exit, fork, sleep
#include <stdlib.h> // used for: NULL, malloc, free, exit, system
#include <paths.h>  // used for: ???
// END_INCLUDES }}}

// DEFINES {{{

#define NORMALBORDERCOLOR 2367780
#define FOCUSBORDERCOLOR 16777215

#define NUMACTIVITIES 20
#define BORDERTHICKNESS 10

// definitions for hiding - can't find where to import them from
#define WithdrawnState 0
#define NormalState 1
#define IconicState 3

// END_DEFINES }}}

// GLOBALS {{{

extern Display * dpy;
extern int currentActivity;

// END_GLOBALS }}}

// FUNCTIONS {{{

// key.c
extern void keyPress(XKeyEvent *);

// map.c
extern void addToMap(Window, int);
extern void removeFromMap(Window);
extern int  getActivityID(Window);
extern int  existsInMap(Window);
extern void freeMap();
extern void switchToActivity(int);
extern void moveFocusedToActivity(int);


// mouse.c
extern void mousePress(XButtonEvent *);
extern void mouseRelease();
extern void mouseMotion(XMotionEvent *);

// utils.c 
extern void executeCommand(const char *);
extern void giveBorder(Window);
extern void destroyFocusedWindow();
extern Window getFocusedWindow();

// window.c
extern void windowMap(XMapEvent *);
extern void windowUnmap(XUnmapEvent *);
extern void windowDestroy(XDestroyWindowEvent *);
extern void windowMessage(XClientMessageEvent *);
extern void focusWindowByID(Window);
extern void focusWindowByNumber(int);

// END_FUNCTIONS }}}
#endif // CABGC_H