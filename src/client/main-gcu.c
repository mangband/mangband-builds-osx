/* File: main-gcu.c */

/*
 * Copyright (c) 1997 Ben Harrison, and others
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

/* Purpose: Allow use of Unix "curses" with Angband -BEN- */


/*
 * To use this file, you must define "USE_GCU" in the Makefile.
 *
 * Hack -- note that "angband.h" is included AFTER the #ifdef test.
 * This was necessary because of annoying "curses.h" silliness.
 *
 * Note that this file is not "intended" to support non-Unix machines,
 * nor is it intended to support VMS or other bizarre setups.
 *
 * Also, this package assumes that the underlying "curses" handles both
 * the "nonl()" and "cbreak()" commands correctly, see the "OPTION" below.
 *
 * This code should work with most versions of "curses" or "ncurses",
 * and the "main-ncu.c" file (and USE_NCU define) are no longer used.
 *
 * See also "USE_CAP" and "main-cap.c" for code that bypasses "curses"
 * and uses the "termcap" information directly, or even bypasses the
 * "termcap" information and sends direct vt100 escape sequences.
 *
 * XXX XXX XXX This file provides only a single "term" window.
 *
 * The "init" and "nuke" hooks are built so that only the first init and
 * the last nuke actually do anything, but the other functions are not
 * "correct" for multiple windows.  Minor changes would also be needed
 * to allow the system to handle the "locations" of the various windows.
 *
 * But in theory, it should be possible to allow a 50 line screen to be
 * split into two (or more) sub-screens.
 *
 * XXX XXX XXX Consider the use of "savetty()" and "resetty()".
 */

#include "c-angband.h"


#ifdef USE_GCU


/*
 * Hack -- play games with "bool"
 */
#undef bool

/*
 * Include the proper "header" file
 */
#ifdef HAVE_LIBNCURSES
# include <ncurses.h>
#else
# include <curses.h>
#endif

typedef struct term_data term_data;

struct term_data
{
	term t;

	int rows;
	int cols;

	WINDOW *win;
};

#define MAX_TERM_DATA 4

static term_data tdata[MAX_TERM_DATA];


/*
 * Hack -- try to guess which systems use what commands
 * Hack -- allow one of the "USE_Txxxxx" flags to be pre-set.
 * Mega-Hack -- try to guess when "POSIX" is available.
 * If the user defines two of these, we will probably crash.
 */
#if !defined(USE_TPOSIX)
# if !defined(USE_TERMIO) && !defined(USE_TCHARS)
#  if defined(_POSIX_VERSION)
#   define USE_TPOSIX
#  else
#   if defined(USG) || defined(linux) || defined(SOLARIS)
#    define USE_TERMIO
#   else
#    define USE_TCHARS
#   endif
#  endif
# endif
#endif

/*
 * Hack -- Amiga uses "fake curses" and cannot do any of this stuff
 */
#if defined(AMIGA)
# undef USE_TPOSIX
# undef USE_TERMIO
# undef USE_TCHARS
#endif




/*
 * POSIX stuff
 */
#ifdef USE_TPOSIX
# include <sys/ioctl.h>
# include <termios.h>
#endif

/*
 * One version needs this file
 */
#ifdef USE_TERMIO
# include <sys/ioctl.h>
# include <termio.h>
#endif

/*
 * The other needs this file
 */
#ifdef USE_TCHARS
# include <sys/ioctl.h>
# include <sys/resource.h>
# include <sys/param.h>
# include <sys/file.h>
# include <sys/types.h>
#endif


/*
 * XXX XXX Hack -- POSIX uses "O_NONBLOCK" instead of "O_NDELAY"
 *
 * They should both work due to the "(i != 1)" test below.
 */
#ifndef O_NDELAY
# define O_NDELAY O_NONBLOCK
#endif


/*
 * OPTION: some machines lack "cbreak()"
 * On these machines, we use an older definition
 */
/* #define cbreak() crmode() */


/*
 * OPTION: some machines cannot handle "nonl()" and "nl()"
 * On these machines, we can simply ignore those commands.
 */
/* #define nonl() */
/* #define nl() */


/*
 * Save the "normal" and "angband" terminal settings
 */

#ifdef USE_TPOSIX

static struct termios  norm_termios;

static struct termios  game_termios;

#endif

#ifdef USE_TERMIO

static struct termio  norm_termio;

static struct termio  game_termio;

#endif

static mmask_t game_mmask;
static mmask_t norm_mmask;

#ifdef USE_TCHARS

static struct ltchars norm_special_chars;
static struct sgttyb  norm_ttyb;
static struct tchars  norm_tchars;
static int            norm_local_chars;

static struct ltchars game_special_chars;
static struct sgttyb  game_ttyb;
static struct tchars  game_tchars;
static int            game_local_chars;

#endif



/*
 * Hack -- Number of initialized "term" structures
 */
static int active = 0;


/*
 * The main screen information
 */
/*static term term_screen_body;*/


#ifdef A_COLOR

/*
 * Hack -- define "A_BRIGHT" to be "A_BOLD", because on many
 * machines, "A_BRIGHT" produces ugly "inverse" video.
 */
#ifndef A_BRIGHT
# define A_BRIGHT A_BOLD
#endif

/*
 * Software flag -- we are allowed to use color
 */
static int can_use_color = TRUE;

/*
 * Software flag -- we are allowed to change the colors
 */
static int can_fix_color = FALSE;

/*
 * Simple Angband to Curses color conversion table
 */
static int colortable[16];

#endif



/*
 * Place the "keymap" into its "normal" state
 */
static void keymap_norm(void)
{

#ifdef USE_TPOSIX

	/* restore the saved values of the special chars */
	(void)tcsetattr(0, TCSAFLUSH, &norm_termios);

#endif

#ifdef USE_TERMIO

	/* restore the saved values of the special chars */
	(void)ioctl(0, TCSETA, (char *)&norm_termio);

#endif

#ifdef USE_TCHARS

	/* restore the saved values of the special chars */
	(void)ioctl(0, TIOCSLTC, (char *)&norm_special_chars);
	(void)ioctl(0, TIOCSETP, (char *)&norm_ttyb);
	(void)ioctl(0, TIOCSETC, (char *)&norm_tchars);
	(void)ioctl(0, TIOCLSET, (char *)&norm_local_chars);

#endif

}



/*
 * Place the "keymap" into the "game" state
 */
static void keymap_game(void)
{

#ifdef USE_TPOSIX

	/* restore the saved values of the special chars */
	(void)tcsetattr(0, TCSAFLUSH, &game_termios);

#endif

#ifdef USE_TERMIO

	/* restore the saved values of the special chars */
	(void)ioctl(0, TCSETA, (char *)&game_termio);

#endif

#ifdef USE_TCHARS

	/* restore the saved values of the special chars */
	(void)ioctl(0, TIOCSLTC, (char *)&game_special_chars);
	(void)ioctl(0, TIOCSETP, (char *)&game_ttyb);
	(void)ioctl(0, TIOCSETC, (char *)&game_tchars);
	(void)ioctl(0, TIOCLSET, (char *)&game_local_chars);

#endif

}


/*
 * Save the normal keymap
 */
static void keymap_norm_prepare(void)
{

#ifdef USE_TPOSIX

	/* Get the normal keymap */
	tcgetattr(0, &norm_termios);

#endif

#ifdef USE_TERMIO

	/* Get the normal keymap */
	(void)ioctl(0, TCGETA, (char *)&norm_termio);

#endif

#ifdef USE_TCHARS

	/* Get the normal keymap */
	(void)ioctl(0, TIOCGETP, (char *)&norm_ttyb);
	(void)ioctl(0, TIOCGLTC, (char *)&norm_special_chars);
	(void)ioctl(0, TIOCGETC, (char *)&norm_tchars);
	(void)ioctl(0, TIOCLGET, (char *)&norm_local_chars);

#endif

}


/*
 * Save the keymaps (normal and game)
 */
static void keymap_game_prepare(void)
{

#ifdef USE_TPOSIX

	/* Acquire the current mapping */
	tcgetattr(0, &game_termios);

	/* Force "Ctrl-C" to interupt */
	game_termios.c_cc[VINTR] = (char)3;

	/* Force "Ctrl-Z" to suspend */
	game_termios.c_cc[VSUSP] = (char)26;

	/* Hack -- Leave "VSTART/VSTOP" alone */

	/* Disable the standard control characters */
	game_termios.c_cc[VQUIT] = (char)-1;
	/* game_termios.c_cc[VERASE] = (char)-1; // allowing BACKSPACE */
	game_termios.c_cc[VKILL] = (char)-1;
	game_termios.c_cc[VEOF] = (char)-1;
	game_termios.c_cc[VEOL] = (char)-1;

	/* Normally, block until a character is read */
	game_termios.c_cc[VMIN] = 1;
	game_termios.c_cc[VTIME] = 0;

#endif

#ifdef USE_TERMIO

	/* Acquire the current mapping */
	(void)ioctl(0, TCGETA, (char *)&game_termio);

	/* Force "Ctrl-C" to interupt */
	game_termio.c_cc[VINTR] = (char)3;

	/* Force "Ctrl-Z" to suspend */
	game_termio.c_cc[VSUSP] = (char)26;

	/* Hack -- Leave "VSTART/VSTOP" alone */

	/* Disable the standard control characters */
	game_termio.c_cc[VQUIT] = (char)-1;
	/* game_termio.c_cc[VERASE] = (char)-1; // allowing BACKSPACE */
	game_termio.c_cc[VKILL] = (char)-1;
	game_termio.c_cc[VEOF] = (char)-1;
	game_termio.c_cc[VEOL] = (char)-1;

#if 0
	/* Disable the non-posix control characters */
	game_termio.c_cc[VEOL2] = (char)-1;
	game_termio.c_cc[VSWTCH] = (char)-1;
	game_termio.c_cc[VDSUSP] = (char)-1;
	game_termio.c_cc[VREPRINT] = (char)-1;
	game_termio.c_cc[VDISCARD] = (char)-1;
	game_termio.c_cc[VWERASE] = (char)-1;
	game_termio.c_cc[VLNEXT] = (char)-1;
	game_termio.c_cc[VSTATUS] = (char)-1;
#endif

	/* Normally, block until a character is read */
	game_termio.c_cc[VMIN] = 1;
	game_termio.c_cc[VTIME] = 0;

#endif

#ifdef USE_TCHARS

	/* Get the default game characters */
	(void)ioctl(0, TIOCGETP, (char *)&game_ttyb);
	(void)ioctl(0, TIOCGLTC, (char *)&game_special_chars);
	(void)ioctl(0, TIOCGETC, (char *)&game_tchars);
	(void)ioctl(0, TIOCLGET, (char *)&game_local_chars);

	/* Force suspend (^Z) */
	game_special_chars.t_suspc = (char)26;

	/* Cancel some things */
	game_special_chars.t_dsuspc = (char)-1;
	game_special_chars.t_rprntc = (char)-1;
	game_special_chars.t_flushc = (char)-1;
	game_special_chars.t_werasc = (char)-1;
	game_special_chars.t_lnextc = (char)-1;

	/* Force interupt (^C) */
	game_tchars.t_intrc = (char)3;

	/* Force start/stop (^Q, ^S) */
	game_tchars.t_startc = (char)17;
	game_tchars.t_stopc = (char)19;

	/* Cancel some things */
	game_tchars.t_quitc = (char)-1;
	game_tchars.t_eofc = (char)-1;
	game_tchars.t_brkc = (char)-1;

#endif

}

/* If "best cursor" mode is on, use it */
static void curs_reset(term_data *td)
{
	if (Term->scr->bcv)
	{
		wmove(td->win, Term->scr->bcy, Term->scr->bcx);
		curs_set(1);
	}
}


/*
 * Suspend/Resume
 */
static errr Term_xtra_gcu_alive(int v)
{
	int x, y;


	/* Suspend */
	if (!v)
	{
		/* Go to normal keymap mode */
		keymap_norm();

		/* Restore modes */
		nocbreak();
		echo();
		nl();

		/* Restore mouse */
		keypad(stdscr, FALSE);
		mousemask(norm_mmask, &game_mmask);

		/* Hack -- make sure the cursor is visible */
		Term_xtra(TERM_XTRA_SHAPE, 1);

		/* Flush the curses buffer */
		(void)refresh();

		/* Get current cursor position */
		getyx(curscr, y, x);

		/* Move the cursor to the bottom right corner */
		mvcur(y, x, LINES - 1, 0);

		/* Exit curses */
		endwin();

		/* Flush the output */
		(void)fflush(stdout);
	}

	/* Resume */
	else
	{
		/* Refresh */
		/* (void)touchwin(curscr); */
		/* (void)wrefresh(curscr); */

		/* Restore the settings */
		cbreak();
		noecho();
		nonl();

		/* Restore mouse */
		keypad(stdscr, TRUE);
		mousemask(game_mmask, &norm_mmask);

		/* Go to angband keymap mode */
		keymap_game();
	}

	/* Success */
	return (0);
}




/*
 * Init the "curses" system
 */
static void Term_init_gcu(term *t)
{
	term_data *td = (term_data *)(t->data);

	/* Count init's, handle first */
	if (active++ != 0) return;

	/* Erase the screen */
	(void)wclear(td->win);

	/* Reset the cursor */
	(void)wmove(td->win, 0, 0);

	/* Flush changes */
	(void)wrefresh(td->win);

	/* Enable \e in macro triggers */
	escape_in_macro_triggers = TRUE;

	/* Game keymap */
	keymap_game();
}


/*
 * Nuke the "curses" system
 */
static void Term_nuke_gcu(term *t)
{
	int x, y;
	term_data *td = (term_data *)(t->data);

	/* Delete this window */
	delwin(td->win);

	/* Count nuke's, handle last */
	if (--active != 0) return;

	/* Hack -- make sure the cursor is visible */
	Term_xtra(TERM_XTRA_SHAPE, 1);

	/* Get current cursor position */
	getyx(curscr, y, x);
	
	/* Move the cursor to bottom right corner */
	mvcur(y, x, LINES - 1, 0);

	/* Flush the curses buffer */
	(void)refresh();

	/* Exit curses */
	endwin();

	/* Flush the output */
	(void)fflush(stdout);

	/* Disable \e in macro triggers */
	escape_in_macro_triggers = FALSE;

	/* Normal keymap */
	keymap_norm();
}




#ifdef USE_GETCH

/*
 * Process events, with optional wait
 */
static errr Term_xtra_gcu_event(int v)
{
	int i, k;

	/* Wait */
	if (v)
	{
		/* Paranoia -- Wait for it */
		nodelay(stdscr, FALSE);
		//halfdelay(2);

		/* Get a keypress */
		i = getch();

		/* Mega-Hack -- allow graceful "suspend" */
		while (i == ERR) i = getch();

		/* Broken input is special */
		if (i == ERR) exit(0);
		if (i == EOF) exit(0);

		cbreak();
	}

	/* Do not wait */
	else
	{
		/* Do not wait for it */
		nodelay(stdscr, TRUE);

		/* Check for keypresses */
		i = getch();

		/* Wait for it next time */
		nodelay(stdscr, FALSE);

		/* None ready */
		if (i == ERR) return (1);
		if (i == EOF) return (1);
	}

	/* Handle mouse */
	if (i == KEY_MOUSE)
	{
		MEVENT event;
		if (getmouse(&event) != OK) return (1);
		if (event.bstate & BUTTON1_CLICKED)
		{
			int button = 1;
			if (event.bstate & BUTTON_CTRL)  button |= 16;
			/* XXX -- hack -- ALT should be 64 and SHIFT 32 !!! */
			if (event.bstate & BUTTON_ALT)   button |= 32;
			if (event.bstate & BUTTON_SHIFT) button |= 64;

			Term_mousepress(event.x, event.y, button);
		}
		else if (event.bstate & REPORT_MOUSE_POSITION)
		{
			Term_mousepress(event.x, event.y, 0);
		}
		return (0);
	}

#if 0
/* Debug keypresses */
mvprintw(1, 0, "Key: %3x %s\n", i, keyname(i));
#endif

/* XXX XXX XXX */
	/* Note: in Angband 3.5.1, there's a switch here, used to translate
	 * putty-numpad-keys into actual direction keys.
	 * For example, Putty will send \eOt for numpad '4'.
	 * Similarly, I've seen xterm send \eOj for numpad '/'.
	 * I don't think we should actually be doing anything here, though,
	 * as those sequences could be macroed to something useful.
	 */ /* However, if we DO include this, we might actually be able to
	     * remove `escape_in_macros` hack, as those are the last few keys
	     * that use escape sequences! */
/* XXX XXX XXX */


	/* Handle keypad mode / function keys (arrows, f1-f12, etc) */
	if (i >= 127)
	{
		char buf[1024];
		size_t len, j;
		/*sprintf(buf, "%c_%3x%c", 31, i, 13);*/
		/* Instead of using codes, let's abuse keyname */
		sprintf(buf, "%c_%s%c", 31, keyname(i), 13);
		len = strlen(buf);
		for (j = 0; j < len; j++)
		{
			Term_keypress(buf[j]);
		}
		return (0);
	}

	/* Enqueue the keypress */
	Term_keypress(i);

	/* Success */
	return (0);
}

#else	/* USE_GETCH */

/*
 * Process events (with optional wait)
 */
static errr Term_xtra_gcu_event(int v)
{
	int i, k;

	char buf[2];

	/* Wait */
	if (v)
	{
		/* Wait for one byte */
		i = read(0, buf, 1);

		/* Hack -- Handle bizarre "errors" */
		if ((i <= 0) && (errno != EINTR)) exit(1);
	}

	/* Do not wait */
	else
	{
		/* Get the current flags for stdin */
		k = fcntl(0, F_GETFL, 0);

		/* Oops */
		if (k < 0) return (1);

		/* Tell stdin not to block */
		if (fcntl(0, F_SETFL, k | O_NDELAY) < 0) return (1);

		/* Read one byte, if possible */
		i = read(0, buf, 1);

		/* Replace the flags for stdin */
		if (fcntl(0, F_SETFL, k)) return (1);
	}

	/* Ignore "invalid" keys */
	if ((i != 1) || (!buf[0])) return (1);

	/* Enqueue the keypress */
	Term_keypress(buf[0]);

	/* Success */
	return (0);
}

#endif	/* USE_GETCH */


/*
 * Handle a "special request"
 */
static errr Term_xtra_gcu(int n, int v)
{
	term_data *td = (term_data *)(Term->data);

	/* Analyze the request */
	switch (n)
	{
		/* Clear screen */
		case TERM_XTRA_CLEAR:
		touchwin(td->win);
		(void)wclear(td->win);
		return (0);

		/* Make a noise */
		case TERM_XTRA_NOISE:
		(void)write(1, "\007", 1);
		return (0);

		/* Flush the Curses buffer */
		case TERM_XTRA_FRESH:
		(void)wrefresh(td->win);
		return (0);

#ifdef USE_CURS_SET

		/* Change the cursor visibility */
		case TERM_XTRA_SHAPE:
		curs_set(v);
		return (0);

#endif

		/* Suspend/Resume curses */
		case TERM_XTRA_ALIVE:
		return (Term_xtra_gcu_alive(v));

		/* Process events */
		case TERM_XTRA_EVENT:
		return (Term_xtra_gcu_event(v));

		/* Flush events */
		case TERM_XTRA_FLUSH:
		while (!Term_xtra_gcu_event(FALSE));
		return (0);

		/* Delay */
		case TERM_XTRA_DELAY:
		usleep(1000 * v);
		return (0);
	}

	/* Unknown */
	return (1);
}


/*
 * Actually MOVE the hardware cursor
 */
static errr Term_curs_gcu(int x, int y)
{
	term_data *td = (term_data *)(Term->data);

	/* Literally move the cursor */
	wmove(td->win, y, x);

	/* Hack -- reset cursor to "best" position */
	curs_reset(td);

	/* Success */
	return (0);
}


/*
 * Erase a grid of space
 * Hack -- try to be "semi-efficient".
 */
static errr Term_wipe_gcu(int x, int y, int n)
{
	term_data *td = (term_data *)(Term->data);

	/* Place cursor */
	wmove(td->win, y, x);

	/* Clear to end of line */
	if (x + n >= td->cols)
	{
		wclrtoeol(td->win);
	}

	/* Clear some characters */
	else
	{
		while (n-- > 0) waddch(td->win, ' ');
	}

	/* Success */
	return (0);
}






/*
 * Place some text on the screen using an attribute
 */
static errr Term_text_gcu(int x, int y, int n, byte a, cptr s)
{
	term_data *td = (term_data *)(Term->data);

	int i;

	char text[255];

	/* Obtain a copy of the text */
	for (i = 0; i < n; i++) text[i] = s[i];
	text[n] = 0;

	/* Move the cursor and dump the string */
	wmove(td->win, y, x);

#ifdef A_COLOR
	/* Set the color */
	if (can_use_color) wattrset(td->win, colortable[a & 0x0F]);
#endif

	/* Add the text */
	waddstr(td->win, text);

	/* Hack -- move cursor to best postion */
	curs_reset(td);

	/* Success */
	return (0);
}



static errr term_data_init(term_data *td, int rows, int cols, int y, int x)
{
	term *t = &td->t;

	/* Make sure the window has a positive size */
	if (rows <= 0 || cols <= 0) return (0);

	td->win = newwin(rows, cols, y, x);

	if (!td->win)
	{
		plog("Failed to setup curses window.");
		return (-1);
	}

	/* Store size */
	td->rows = rows;
	td->cols = cols;

	/* Initialize the term */
	term_init(t, cols, rows, 256);

	/* Avoid the bottom right corner */
	t->icky_corner = TRUE;

	/* Erase with "white space" */
	t->attr_blank = TERM_WHITE;
	t->char_blank = ' ';

	/* Set some hooks */
	t->init_hook = Term_init_gcu;
	t->nuke_hook = Term_nuke_gcu;

	/* Set some more hooks */
	t->text_hook = Term_text_gcu;
	t->wipe_hook = Term_wipe_gcu;
	t->curs_hook = Term_curs_gcu;
	t->xtra_hook = Term_xtra_gcu;

	/* Save the data */
	t->data = td;

	/* Activate it */
	Term_activate(t);


	/* Success */
	return (0);
}

const char help_gcu[] = "";

/*
 * Prepare "curses" for use by the file "term.c"
 *
 * Installs the "hook" functions defined above, and then activates
 * the main screen "term", which clears the screen and such things.
 *
 * Someone should really check the semantics of "initscr()"
 */
errr init_gcu(void)
{
	int i;

	/*term *t = &term_screen_body;*/

	int num_term = 4, next_win = 0;

	/* Extract the normal keymap */
	keymap_norm_prepare();


#if defined(USG)
	/* Initialize for USG Unix */
	if (initscr() == NULL) return (-1);
#else
	/* Initialize for others systems */
	if (initscr() == (WINDOW*)ERR) return (-1);
#endif


	/* Hack -- Require large screen, or Quit with message */
	i = ((LINES < 24) || (COLS < 80));
	if (i) quit("Angband needs an 80x24 'curses' screen");

	/* Enable mouse */
	keypad(stdscr, TRUE);
	mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, &norm_mmask);

#ifdef A_COLOR

	/*** Init the Color-pairs and set up a translation table ***/

	/* Do we have color, and enough color, available? */
	can_use_color = ((start_color() != ERR) && has_colors() &&
	                 (COLORS >= 8) && (COLOR_PAIRS >= 8));	             
	// APD test
	  COLORS=16;
	  COLOR_PAIRS=16;

#ifdef REDEFINE_COLORS
	/* Can we change colors? */
	can_fix_color = (can_use_color && can_change_color() &&
	                 (COLOR_PAIRS >= 16));
#endif

	/* Attempt to use customized colors */
	if (can_fix_color)
	{
		/* Prepare the color pairs */
		for (i = 0; i < 16; i++)
		{
			/* Reset the color */
			init_pair(i, i, i);

			/* Reset the color data */
			colortable[i] = (COLOR_PAIR(i) | A_NORMAL);
		}

		/* XXX XXX XXX Take account of "gamma correction" */

		/* Prepare the "Angband Colors" */
		init_color(0,     0,    0,    0);	/* Black */
		init_color(1,  1000, 1000, 1000);	/* White */
		init_color(2,   500,  500,  500);	/* Grey */
		init_color(3,  1000,  500,    0);	/* Orange */
		init_color(4,   750,    0,    0);	/* Red */
		init_color(5,     0,  500,  250);	/* Green */
		init_color(6,     0,    0, 1000);	/* Blue */
		init_color(7,   500,  250,    0);	/* Brown */
		init_color(8,   250,  250,  250);	/* Dark-grey */
		init_color(9,   750,  750,  750);	/* Light-grey */
		init_color(10, 1000,    0, 1000);	/* Purple */
		init_color(11, 1000, 1000,    0);	/* Yellow */
		init_color(12, 1000,    0,    0);	/* Light Red */
		init_color(13,    0, 1000,    0);	/* Light Green */
		init_color(14,    0, 1000, 1000);	/* Light Blue */
		init_color(15,  750,  500,  250);	/* Light Brown */
	}

	/* Attempt to use colors */
	/*else*/ if (can_use_color)
	{
		/* Color-pair 0 is *always* WHITE on BLACK */

		/* Prepare the color pairs */
		init_pair(1, COLOR_RED,     COLOR_BLACK);
		init_pair(2, COLOR_GREEN,   COLOR_BLACK);
		init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
		init_pair(4, COLOR_BLUE,    COLOR_BLACK);
		init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(6, COLOR_CYAN,    COLOR_BLACK);
		init_pair(7, COLOR_BLACK,   COLOR_BLACK);

		/* Prepare the "Angband Colors" -- Bright white is too bright */
		colortable[0] = (COLOR_PAIR(7) | A_NORMAL);	/* Black */
		colortable[1] = (COLOR_PAIR(0) | A_NORMAL);	/* White */
		colortable[2] = (COLOR_PAIR(6) | A_NORMAL);	/* Grey XXX */
		colortable[3] = (COLOR_PAIR(1) | A_BRIGHT);	/* Orange XXX */
		colortable[4] = (COLOR_PAIR(1) | A_NORMAL);	/* Red */
		colortable[5] = (COLOR_PAIR(2) | A_NORMAL);	/* Green */
		colortable[6] = (COLOR_PAIR(4) | A_NORMAL);	/* Blue */
		colortable[7] = (COLOR_PAIR(3) | A_NORMAL);	/* Umber */
		colortable[8] = (COLOR_PAIR(7) | A_BRIGHT);	/* Dark-grey XXX */
		colortable[9] = (COLOR_PAIR(6) | A_BRIGHT);	/* Light-grey XXX */
		colortable[10] = (COLOR_PAIR(5) | A_NORMAL);	/* Purple */
		colortable[11] = (COLOR_PAIR(3) | A_BRIGHT);	/* Yellow */
		colortable[12] = (COLOR_PAIR(5) | A_BRIGHT);	/* Light Red XXX */
		colortable[13] = (COLOR_PAIR(2) | A_BRIGHT);	/* Light Green */
		colortable[14] = (COLOR_PAIR(4) | A_BRIGHT);	/* Light Blue */
		colortable[15] = (COLOR_PAIR(3) | A_NORMAL);	/* Light Umber XXX */
	}

#endif


	/*** Low level preparation ***/

#ifdef USE_GETCH

	/* Paranoia -- Assume no waiting */
	nodelay(stdscr, FALSE);

#endif

	/* Prepare */
	cbreak();
	noecho();
	nonl();

	/* Extract the game keymap */
	keymap_game_prepare();


	/*** Now prepare the term(s) ***/

	for (i = 0; i < num_term; i++)
	{
		int rows, cols;
		int y, x;

		switch (i)
		{
			case 0: rows = 24;
				cols = 80;
				y = x = 0;
				break;
			case 1: rows = LINES - 25;
				cols = 80;
				y = 25;
				x = 0;
				break;
			case 2: rows = 24;
				cols = COLS - 81;
				y = 0;
				x = 81;
				break;
			case 3: rows = LINES - 25;
				cols = COLS - 81;
				y = 25;
				x = 81;
				break;
			default: rows = cols = 0;
				 y = x = 0;
				 break;
		}

		if (rows <= 0 || cols <= 0) continue;

		term_data_init(&tdata[next_win], rows, cols, y, x);

		ang_term[next_win] = Term;

		next_win++;
	}

	/* Activate the "Angband" window screen */
	Term_activate(&tdata[0].t);

	term_screen = &tdata[0].t;

	/* Success */
	return (0);
}


#endif /* USE_GCU */


