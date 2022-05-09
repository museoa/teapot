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
Key wgetc(void)
{
  /* variables */ /*{{{*/
  chtype c;
  /*}}}*/

  doupdate();
  refresh();
  switch (c=wgetch(stdscr))
  {
    /* LEFT */ /*{{{*/
    case KEY_LEFT:
    case '\02': return K_LEFT;
    /*}}}*/
    /* RIGHT */ /*{{{*/
    case KEY_RIGHT:
    case '\06': return K_RIGHT;
    /*}}}*/
    /* UP */ /*{{{*/
    case KEY_UP:
    case '\020': return K_UP;
    /*}}}*/
    /* DOWN */ /*{{{*/
    case KEY_DOWN:
    case '\016': return K_DOWN;
    /*}}}*/
    /* BACKSPACE */ /*{{{*/
    case KEY_BACKSPACE:
    case '\010': return K_BACKSPACE;
    /*}}}*/
    /* DC */ /*{{{*/
    case KEY_DC:
    case '\04':
    case '\177': return K_DC;
    /*}}}*/
    /* CANCEL */ /*{{{*/
    case '\03':
    case '\07': return KEY_CANCEL;
    /*}}}*/
    /* ENTER */ /*{{{*/
    case KEY_ENTER:
    case '\r':
    case '\n': return K_ENTER;
    /*}}}*/
    /* HOME */ /*{{{*/
    case KEY_HOME:
    case '\01': return K_HOME;
    /*}}}*/
    /* END */ /*{{{*/
    case KEY_END:
    case '\05': return K_END;
    /*}}}*/
    /* DL */ /*{{{*/
    case '\013': return KEY_DL;
    /*}}}*/
    /* NPAGE */ /*{{{*/
    case KEY_NPAGE:
    case '\026': return K_NPAGE;
    /*}}}*/
    /* PPAGE */ /*{{{*/
    case KEY_PPAGE: return K_PPAGE;
    /*}}}*/
    /* Control Y, copy */ /*{{{*/
    case '\031': return K_COPY;
    /*}}}*/
    /* Control R, recalculate sheet */ /*{{{*/
    case '\022': return K_RECALC;
    /*}}}*/
    /* Control S, clock sheet */ /*{{{*/
    case '\023': return K_CLOCK;
    /*}}}*/
    /* Control X, get one more key */ /*{{{*/
    case '\030':
    {
      switch (wgetch(stdscr))
      {
        /* C-x <   -- BPAGE */ /*{{{*/
        case KEY_PPAGE:
        case '<': return K_BPAGE;
        /*}}}*/
        /* C-x >   -- FPAGE */ /*{{{*/
        case KEY_NPAGE:
        case '>': return K_FPAGE;
        /*}}}*/
        /* C-x C-c -- QUIT */ /*{{{*/
        case '\03': return K_QUIT;
        /*}}}*/
        /* C-x C-s -- SAVE */ /*{{{*/
        case '\023': return K_SAVE;
        /*}}}*/
        /* C-x C-r -- LOAD */ /*{{{*/
        case '\022': return K_LOAD;
        /*}}}*/
        /* default -- INVALID, general invalid value */ /*{{{*/
        default: return K_INVALID;
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
        case 'v': return K_PPAGE;
        /*}}}*/
        /* M-Enter -- MENTER */ /*{{{*/
        case KEY_ENTER:
        case '\r':
        case '\n': return K_MENTER;
        /*}}}*/
        /* M-z     -- SAVEQUIT */ /*{{{*/
        case 'z': return K_SAVEQUIT;
        /*}}}*/
        /* default -- INVALID, general invalid value */ /*{{{*/
        default: return K_INVALID;
        /*}}}*/
      }
    }
    /*}}}*/
    /* LOADMENU */ /*{{{*/
    case KEY_F(2): return K_LOADMENU;
    /*}}}*/
    /* SAVEMENU */ /*{{{*/
    case KEY_F(3): return K_SAVEMENU;
    /*}}}*/
    /* default */ /*{{{*/
    default: return c;
    /*}}}*/
  }
}
/*}}}*/
