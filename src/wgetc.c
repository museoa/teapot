/* #includes */ /*{{{C}}}*//*{{{*/
#ifndef NO_POSIX_SOURCE
#undef _POSIX_SOURCE
#define _POSIX_SOURCE   1
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "display.h"
#include "wgetc.h"
/*}}}*/

/* wgetc */ /*{{{*/
chtype wgetc(void)
{
  /* variables */ /*{{{*/
  chtype c;
  /*}}}*/

  begin:
  doupdate();
  refresh();
  switch (c=wgetch(stdscr))
  {
    /* LEFT */ /*{{{*/
    case '\02': return KEY_LEFT;
    /*}}}*/
    /* RIGHT */ /*{{{*/
    case '\06': return KEY_RIGHT;
    /*}}}*/
    /* UP */ /*{{{*/
    case '\020': return KEY_UP;
    /*}}}*/
    /* DOWN */ /*{{{*/
    case '\016': return KEY_DOWN;
    /*}}}*/
    /* BACKSPACE */ /*{{{*/
    case '\010': return KEY_BACKSPACE;
    /*}}}*/
    /* DC */ /*{{{*/
    case '\04':
    case '\177': return KEY_DC;
    /*}}}*/
    /* CANCEL */ /*{{{*/
    case '\03':
    case '\07': return KEY_CANCEL;
    /*}}}*/
    /* ENTER */ /*{{{*/
    case KEY_ENTER:
    case '\r':
    case '\n': return KEY_ENTER;
    /*}}}*/
    /* BEG */ /*{{{*/
    case '\01': return KEY_BEG;
    /*}}}*/
    /* END */ /*{{{*/
    case '\05': return KEY_END;
    /*}}}*/
    /* DL */ /*{{{*/
    case '\013': return KEY_DL;
    /*}}}*/
    /* NPAGE */ /*{{{*/
    case '\026': return KEY_NPAGE;
    /*}}}*/
    /* Control L, refresh screen */ /*{{{*/
    case '\014': redraw(); goto begin;
    /*}}}*/
    /* Control X, get one more key */ /*{{{*/
    case '\030':
    {
      switch (wgetch(stdscr))
      {
        /* C-x <   -- BPAGE */ /*{{{*/
        case '<': return KEY_BPAGE;
        /*}}}*/
        /* C-x >   -- FPAGE */ /*{{{*/
        case '>': return KEY_FPAGE;
        /*}}}*/
        /* C-x C-c -- KEY_QUIT */ /*{{{*/
        case '\03': return KEY_QUIT;
        /*}}}*/
        /* C-x C-s -- KEY_SAVE */ /*{{{*/
        case '\023': return KEY_SAVE;
        /*}}}*/
        /* C-x C-r -- KEY_LOAD */ /*{{{*/
        case '\022': return KEY_LOAD;
        /*}}}*/
        /* default -- KEY_MAX, general invalid value */ /*{{{*/
        default: return KEY_MAX;
        /*}}}*/
      }
    }
    /*}}}*/
    /* ESC, get one more key */ /*{{{*/
    case '\033':
    {
      switch (wgetch(stdscr))
      {
        /* M-v     -- PPAGE */ /*{{{*/
        case 'v': return KEY_PPAGE;
        /*}}}*/
        /* M-Enter -- MENTER */ /*{{{*/
        case KEY_ENTER:
        case '\r':
        case '\n': return KEY_MENTER;
        /*}}}*/
        /* M-z     -- SAVEQUIT */ /*{{{*/
        case 'z': return KEY_SAVEQUIT;
        /*}}}*/
        /* default -- KEY_MAX, general invalid value */ /*{{{*/
        default: return KEY_MAX;
        /*}}}*/
      }
    }
    /*}}}*/
    /* default */ /*{{{*/
    default: return c;
    /*}}}*/
  }
}
/*}}}*/
