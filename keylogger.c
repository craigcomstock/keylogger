#include "keylogger.h"
#include <stdio.h>
#include <string.h>

CGEventFlags lastFlags = 0;
char keyname[256];

void dump_stats(int sig) {
  // we read in stats at beginning so overwrite with an updated full set of
  // stats
  statsfile = fopen(statsfilelocation, "w");

  if (!statsfile) {
    fprintf(stderr,
            "ERROR: Unable to open stats file. Ensure that you have the "
            "proper permissions.\n");
    exit(1);
  }

  for (int i = 0; i < 256; i++) {
    if (stats[i].keyname != NULL) {
      fprintf(statsfile, "%s %d\n", stats[i].keyname, stats[i].n);
    }
  }
  exit(0);
}

void loadFile(char *path, char *dest, int max) {
  //	printf("loadFile\n");
  FILE *fp;
  fp = fopen(path, "r");
  if (fp == NULL) {
    fprintf(stderr, "couldn't open gestures file:%s\n", path);
    exit(3);
  }
  int c;
  int i = 0;
  strcpy(dest, "");

  while ((c = getc(fp)) != EOF) {
    if (i > max - 1) {
      fprintf(stderr, "buffer not big enough for gestures file.\n");
      exit(4);
    }
    dest[i] = (char)c;
    i++;
  }
  fclose(fp);
}

void read_stats() {
  statsfile = fopen(statsfilelocation, "r");
  // TODO maybe the file doesn't exist yet, that's ok. :)
  if (!statsfile) {
    /*
    fprintf(stderr,
            "ERROR: Unable to open stats file. Ensure that you have the "
            "proper permissions.\n");
    */
    return;
    // exit(1);
  }

  char buffer[8000];
  int max = 8000;
  char *str = buffer;
  int i = 0;
  int c;

  while ((c = getc(statsfile)) != EOF) {
    if (i > max - 1) {
      fprintf(stderr, "buffer not big enough for stats file.\n");
      exit(2);
    }
    buffer[i] = (char)c;
    i++;
  }
  fclose(statsfile);

  char *strptr, *line, *key, *value, *file_parser, *line_parser;
  strptr = str;
  ENTRY e, *ep;
  stat_ptr = stats;
  for (;;) {
    line = strtok_r(strptr, "\n", &file_parser);
    // fprintf(stderr, "strptr %p, file_parser %p\n", strptr, file_parser);
    strptr = NULL;
    if (line == NULL)
      break;
    if (line[0] == '#') {
      // fprintf(stderr, "comment line: '%s'\n", line);
      continue;
    }
    // fprintf(stderr, "line is '%s'\n", line);
    strptr = line;
    key = strtok_r(strptr, " ", &line_parser);
    if (key == NULL) {
      break;
    }
    strptr = NULL;
    value = strtok_r(strptr, " ", &line_parser);
    if (value == NULL) {
      break;
    }

    stat_ptr++;
    stat_ptr->keyname = strdup(key);
    stat_ptr->n = atoi(value);
    e.key = strdup(key);
    e.data = stat_ptr;
    hsearch(e, ENTER);
  }
}

int main(int argc, const char *argv[]) {
  // add a signal handler to dump stats at end
  signal(SIGINT, dump_stats);
  signal(SIGTERM, dump_stats);
  signal(SIGBUS, dump_stats);
  signal(SIGUSR1, dump_stats);
  signal(SIGUSR2, dump_stats);

  // Create an event tap to retrieve keypresses.
  // CGEventMask eventMask = CGEventMaskBit(kCGEventKeyDown);
  CGEventMask eventMask =
      CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventFlagsChanged);
  CFMachPortRef eventTap =
      CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0, eventMask,
                       CGEventCallback, NULL);

  // Exit the program if unable to create the event tap.
  if (!eventTap) {
    fprintf(stderr, "ERROR: Unable to create event tap.\n");
    exit(1);
  }

  // Create a run loop source and add enable the event tap.
  CFRunLoopSourceRef runLoopSource =
      CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource,
                     kCFRunLoopCommonModes);
  CGEventTapEnable(eventTap, true);

  // Clear the statsfile if clear argument used or log to specific file if
  // given.
  if (argc == 2) {
    if (strcmp(argv[1], "clear") == 0) {
      unlink(statsfilelocation);
      exit(1);
    } else {
      statsfilelocation = argv[1];
    }
  }

  // setup the stats map
  hdestroy();
  hcreate(256);
  read_stats();

  // Get the current time and open the statsfile.
  time_t result = time(NULL);

  // Display the location of the statsfile and start the loop.
  printf("Collecting stats in: %s\n", statsfilelocation);
  fflush(stdout);
  CFRunLoopRun();

  return 0;
}

// The following callback method is invoked on every keypress.
CGEventRef CGEventCallback(CGEventTapProxy proxy, CGEventType type,
                           CGEventRef event, void *refcon) {
  // printf("CGEventCallback()\n");
  if (type != kCGEventKeyDown && type != kCGEventFlagsChanged &&
      type != kCGEventScrollWheel) {
    return event;
  }

  CGEventFlags flags = CGEventGetFlags(event);

  // Retrieve the incoming keycode.
  CGKeyCode keyCode =
      (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);

  // printf("keyCode=%d\n", keyCode);
  //  Calculate key up/down.
  bool down = false;
  if (type == kCGEventFlagsChanged) {
    switch (keyCode) {
    case 54: // [right-cmd]
    case 55: // [left-cmd]
      down = (flags & kCGEventFlagMaskCommand) &&
             !(lastFlags & kCGEventFlagMaskCommand);
      break;
    case 56: // [left-shift]
    case 60: // [right-shift]
      down = (flags & kCGEventFlagMaskShift) &&
             !(lastFlags & kCGEventFlagMaskShift);
      break;
    case 58: // [left-option]
    case 61: // [right-option]
      down = (flags & kCGEventFlagMaskAlternate) &&
             !(lastFlags & kCGEventFlagMaskAlternate);
      break;
    case 59: // [left-ctrl]
    case 62: // [right-ctrl]
      down = (flags & kCGEventFlagMaskControl) &&
             !(lastFlags & kCGEventFlagMaskControl);
      break;
    case 57: // [caps]
      down = (flags & kCGEventFlagMaskAlphaShift) &&
             !(lastFlags & kCGEventFlagMaskAlphaShift);
      break;
    default:
      break;
    }
  } else if (type == kCGEventKeyDown) {
    down = true;
  }
  lastFlags = flags;

  // Only log key down events.
  if (!down) {
    return event;
  }

  // TODO
  // add one to the human readable key name stat
  bool shift = flags & kCGEventFlagMaskShift;
  bool caps = flags & kCGEventFlagMaskAlphaShift;

  const char *keyname;
  keyname = convertKeyCode(keyCode, shift, caps);
  // printf("keyname=%s\n", keyname);
  entry.key = strdup(keyname);
  found = hsearch(entry, FIND);
  // printf("found=%p\n", found);
  int stat = 1;
  if (found == NULL) {
    // printf("found NULL so add entry\n");
    stat_ptr++;
    // printf("stat_ptr = %p\n", stat_ptr);
    stat_ptr->keyname = strdup(keyname);
    stat_ptr->n = stat;
    // printf("stat_ptr->n = %d\n", stat_ptr->n);
    entry.data = stat_ptr;
    // printf("entry.data = %p\n", entry.data);
    hsearch(entry, ENTER);
  } else {
    stat = ((struct stat *)found->data)->n;
    // printf("update\n");
    stat += 1;
    // printf("stat = %d\n", stat);
    ((struct stat *)found->data)->n += 1;
    // printf("entry updated\n");
  }
  // printf("stat for '%s' updated to %d\n", keyname, stat);
  return event;
}

// The following method converts the key code returned by each keypress as
// a human readable key code in const char format.
const char *convertKeyCode(int keyCode, bool shift, bool caps) {
  switch ((int)keyCode) {
  case 0:
    return shift || caps ? "A" : "a";
  case 1:
    return shift || caps ? "S" : "s";
  case 2:
    return shift || caps ? "D" : "d";
  case 3:
    return shift || caps ? "F" : "f";
  case 4:
    return shift || caps ? "H" : "h";
  case 5:
    return shift || caps ? "G" : "g";
  case 6:
    return shift || caps ? "Z" : "z";
  case 7:
    return shift || caps ? "X" : "x";
  case 8:
    return shift || caps ? "C" : "c";
  case 9:
    return shift || caps ? "V" : "v";
  case 11:
    return shift || caps ? "B" : "b";
  case 12:
    return shift || caps ? "Q" : "q";
  case 13:
    return shift || caps ? "W" : "w";
  case 14:
    return shift || caps ? "E" : "e";
  case 15:
    return shift || caps ? "R" : "r";
  case 16:
    return shift || caps ? "Y" : "y";
  case 17:
    return shift || caps ? "T" : "t";
  case 18:
    return shift ? "!" : "1";
  case 19:
    return shift ? "@" : "2";
  case 20:
    return shift ? "#" : "3";
  case 21:
    return shift ? "$" : "4";
  case 22:
    return shift ? "^" : "6";
  case 23:
    return shift ? "%" : "5";
  case 24:
    return shift ? "+" : "=";
  case 25:
    return shift ? "(" : "9";
  case 26:
    return shift ? "&" : "7";
  case 27:
    return shift ? "_" : "-";
  case 28:
    return shift ? "*" : "8";
  case 29:
    return shift ? ")" : "0";
  case 30:
    return shift ? "}" : "]";
  case 31:
    return shift || caps ? "O" : "o";
  case 32:
    return shift || caps ? "U" : "u";
  case 33:
    return shift ? "{" : "[";
  case 34:
    return shift || caps ? "I" : "i";
  case 35:
    return shift || caps ? "P" : "p";
  case 37:
    return shift || caps ? "L" : "l";
  case 38:
    return shift || caps ? "J" : "j";
  case 39:
    return shift ? "\"" : "'";
  case 40:
    return shift || caps ? "K" : "k";
  case 41:
    return shift ? ":" : ";";
  case 42:
    return shift ? "|" : "\\";
  case 43:
    return shift ? "<" : ",";
  case 44:
    return shift ? "?" : "/";
  case 45:
    return shift || caps ? "N" : "n";
  case 46:
    return shift || caps ? "M" : "m";
  case 47:
    return shift ? ">" : ".";
  case 50:
    return shift ? "~" : "`";
  case 65:
    return "[decimal]";
  case 67:
    return "[asterisk]";
  case 69:
    return "[plus]";
  case 71:
    return "[clear]";
  case 75:
    return "[divide]";
  case 76:
    return "[enter]";
  case 78:
    return "[hyphen]";
  case 81:
    return "[equals]";
  case 82:
    return "0";
  case 83:
    return "1";
  case 84:
    return "2";
  case 85:
    return "3";
  case 86:
    return "4";
  case 87:
    return "5";
  case 88:
    return "6";
  case 89:
    return "7";
  case 91:
    return "8";
  case 92:
    return "9";
  case 36:
    return "[return]";
  case 48:
    return "[tab]";
  case 49:
    return "[space]"; // need this visible so I can "see" it in stats file
  case 51:
    return "[del]";
  case 53:
    return "[esc]";
  case 54:
    return "[right-cmd]";
  case 55:
    return "[left-cmd]";
  case 56:
    return "[left-shift]";
  case 57:
    return "[caps]";
  case 58:
    return "[left-option]";
  case 59:
    return "[left-ctrl]";
  case 60:
    return "[right-shift]";
  case 61:
    return "[right-option]";
  case 62:
    return "[right-ctrl]";
  case 63:
    return "[fn]";
  case 64:
    return "[f17]";
  case 72:
    return "[volup]";
  case 73:
    return "[voldown]";
  case 74:
    return "[mute]";
  case 79:
    return "[f18]";
  case 80:
    return "[f19]";
  case 90:
    return "[f20]";
  case 96:
    return "[f5]";
  case 97:
    return "[f6]";
  case 98:
    return "[f7]";
  case 99:
    return "[f3]";
  case 100:
    return "[f8]";
  case 101:
    return "[f9]";
  case 103:
    return "[f11]";
  case 105:
    return "[f13]";
  case 106:
    return "[f16]";
  case 107:
    return "[f14]";
  case 109:
    return "[f10]";
  case 111:
    return "[f12]";
  case 113:
    return "[f15]";
  case 114:
    return "[help]";
  case 115:
    return "[home]";
  case 116:
    return "[pgup]";
  case 117:
    return "[fwddel]";
  case 118:
    return "[f4]";
  case 119:
    return "[end]";
  case 120:
    return "[f2]";
  case 121:
    return "[pgdown]";
  case 122:
    return "[f1]";
  case 123:
    return "[left]";
  case 124:
    return "[right]";
  case 125:
    return "[down]";
  case 126:
    return "[up]";
  }
  return "[unknown]";
}
