#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "gtksheet.h"
#include "gtkitementry.h"
#include <string.h>
#include <assert.h>
#include <malloc.h>

#include "main.h"
#include "display.h"
#include "cat.h"
#include "version.h"

#define K_HELP 1
#define K_ABOUT 2

static GtkWidget *content;
static GtkWidget *message;
static GtkWidget *edit;
static GtkWidget *edit_prompt;
static GtkWidget *edit_text;
static GtkWidget *menubar;
static GtkWidget *grid;
static GtkWidget *vbox;
static GtkWidget *scroller;
static GtkWidget *main_window;
static GMainLoop *input = NULL;
static GdkColor markbg = {0, 0, 0, 0x7fff};
static GdkColor markfg = {0, 0xffff, 0xffff, 0xffff};
static GdkColor normbg = {0, 0xffff, 0xffff, 0xffff};
static GdkColor normfg = {0, 0, 0, 0};
static gboolean has_mark = FALSE;

static void update_content(Sheet *sheet);

static gboolean handle_sheetcmd(Sheet *sheet, Key action, int moveonly)
{
	GtkSheetRange range;

	if (action == K_NAME) {
		do_sheetcmd(sheet, action, moveonly);
		action = K_SAVE;
	}
	/* TODO: Intercept K_FPAGE/K_BPAGE and implement a non-broken way to navigate */
	if (do_sheetcmd(sheet, action, moveonly) && doanyway(sheet, LEAVE))
	{
		if (input) g_main_loop_quit(input);
		gtk_main_quit();
		return TRUE;
	}

	while (sheet->curx > 0 && shadowed(sheet,sheet->curx,sheet->cury,sheet->curz)) sheet->curx--;
	if (sheet->offx > sheet->curx) sheet->offx = sheet->curx;
	if (sheet->offy > sheet->cury) sheet->offy = sheet->cury;
	if (sheet->offx+sheet->width <= sheet->curx) sheet->offx = sheet->curx-sheet->width;
	if (sheet->offy+sheet->maxy-3 <= sheet->cury) sheet->offy = sheet->cury-sheet->maxy+3;
	if (sheet->offx < 0) sheet->offx = 0;
	if (sheet->offy < 0) sheet->offy = 0;
	gtk_sheet_moveto(GTK_SHEET(grid), sheet->offy, sheet->offx, 0, 0.1);

	gtk_sheet_get_visible_range(GTK_SHEET(grid), &range);
	sheet->width = sheet->maxx = range.coli-range.col0 - 1;
	sheet->maxy = range.rowi-range.row0+3;

	if (sheet->marking || (sheet->mark1x < 0 && has_mark)) redraw_sheet(sheet);
	else redraw_cell(sheet, sheet->curx, sheet->cury, sheet->curz);
	return FALSE;
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	handle_sheetcmd((Sheet*)data, K_QUIT, 0);
	return TRUE;
}

void display_main(Sheet *cursheet)
{
	cursheet->maxx = 4;
	cursheet->maxy = 4;

	gtk_widget_show_all(main_window);
	gtk_widget_hide(edit);
	update_content(cursheet);
	redraw_sheet(cursheet);

	gtk_main();
}

static const gchar *helpmsg = ""
#include "help.h"
"";

static void do_action(gpointer data, guint action, GtkWidget *menu_item)
{
	if (action == K_ABOUT)
	{
		const gchar *authors[] = { "Michael Haardt <michael@moria.de>", "Jörg Walter <info@syntax-k.de>", NULL };
		gtk_show_about_dialog(GTK_WINDOW(main_window),
			"name", "gTeapot",
			"program-name", "gTeapot",
			"version", VERSION,
			"comments", "Table Editor And Planner, GTK+ version",
			"copyright", "Copyright © 2003 Michael Haardt,\nGTK+ version by Jörg Walter",
			"website", "http://www.moria.de/~michael/teapot/",
			"authors", authors,
			NULL);
		return;
	}

	if (action == K_HELP)
	{
		GtkWidget *scroller, *label;
		GtkWidget *dialog = gtk_dialog_new_with_buttons("gTeapot Help", GTK_WINDOW(main_window), 0, NULL);
		gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
		gtk_widget_modify_bg(dialog, GTK_STATE_NORMAL, &normbg);
		gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);

		label = gtk_label_new("");
		gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
		gtk_label_set_width_chars(GTK_LABEL(label), 80);
		gtk_widget_set_size_request(label, 780, -1);
		gtk_label_set_markup(GTK_LABEL(label), helpmsg);

		scroller = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroller), label);
		gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroller), GTK_SHADOW_NONE);
		gtk_viewport_set_shadow_type(GTK_VIEWPORT(gtk_bin_get_child(GTK_BIN(scroller))), GTK_SHADOW_NONE);
		gtk_widget_modify_bg(gtk_bin_get_child(GTK_BIN(scroller)), GTK_STATE_NORMAL, &normbg);
		gtk_widget_modify_fg(gtk_bin_get_child(GTK_BIN(scroller)), GTK_STATE_NORMAL, &normfg);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), scroller, TRUE, TRUE, 0);

		g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
		gtk_widget_show_all(dialog);
		gtk_window_resize(GTK_WINDOW(dialog), 800, 600);
		return;
	}
	handle_sheetcmd((Sheet*)data, action, ((Sheet*)data)->moveonly);
}

static GtkItemFactoryEntry menu[] = {
{ "/_File",            NULL,             NULL,      0,          "<Branch>" },
{ "/File/_Open...",    "<CTRL>O",        do_action, K_LOADMENU, "<StockItem>", GTK_STOCK_OPEN },
{ "/File/_Save",       "<CTRL>S",        do_action, K_SAVE,     "<StockItem>", GTK_STOCK_SAVE },
{ "/File/Save _As...", "<CTRL><SHIFT>S", do_action, K_NAME,     "<StockItem>", GTK_STOCK_SAVE_AS },
{ "/File/sep1",        NULL,             NULL,      0,          "<Separator>" },
{ "/File/_Export...",  "<CTRL>E",        do_action, K_SAVEMENU, "<Item>" },
{ "/File/sep1",        NULL,             NULL,      0,          "<Separator>" },
{ "/File/_Quit",       "<CTRL>Q",        do_action, K_QUIT,     "<StockItem>", GTK_STOCK_QUIT },
{ "/_Block",           NULL,             NULL,      0,          "<Branch>" },
{ "/Block/_Insert",    "<ALT><SHIFT>I",  do_action, BLOCK_INSERT, "<Item>" },
{ "/Block/_Delete",    "<ALT><SHIFT>D",  do_action, BLOCK_DELETE, "<Item>" },
{ "/Block/sep1",       NULL,             NULL,      0,            "<Separator>" },
{ "/Block/_Move",      "<ALT><SHIFT>M",  do_action, BLOCK_MOVE,   "<Item>" },
{ "/Block/_Copy",      "<ALT><SHIFT>C",  do_action, BLOCK_COPY,   "<Item>" },
{ "/Block/_Fill",      "<ALT><SHIFT>F",  do_action, BLOCK_FILL,   "<Item>" },
{ "/Block/sep1",       NULL,             NULL,      0,            "<Separator>" },
{ "/Block/C_lear",     "<ALT><SHIFT>L",  do_action, BLOCK_CLEAR,  "<Item>" },
{ "/Block/_Sort",      "<ALT><SHIFT>S",  do_action, BLOCK_SORT,   "<Item>" },
{ "/Block/Mi_rror",    "<ALT><SHIFT>R",  do_action, BLOCK_MIRROR, "<Item>" },
{ "/_View",            NULL,             NULL,      0,          "<Branch>" },
{ "/View/Column _Width...", "<ALT>W",    do_action, K_COLWIDTH, "<Item>" },
{ "/View/_Goto...",         "<CTRL>G",   do_action, K_GOTO,     "<Item>" },
{ "/F_ormat",              NULL,         NULL,      0,                  "<Branch>" },
{ "/Format/L_abel...",     "<ALT>A",     do_action, ADJUST_LABEL,       "<Item>" },
{ "/Format/sep1",          NULL,         NULL,      0,                  "<Separator>" },
{ "/Format/_Left",         "<ALT>L",     do_action, ADJUST_LEFT,        "<Item>" },
{ "/Format/_Right",        "<ALT>R",     do_action, ADJUST_RIGHT,       "<Item>" },
{ "/Format/_Center",       "<ALT>C",     do_action, ADJUST_CENTER,      "<Item>" },
{ "/Format/sep1",          NULL,         NULL,      0,                  "<Separator>" },
{ "/Format/_Scientific",   "<ALT>S",     do_action, ADJUST_SCIENTIFIC_ON,  "<Item>" },
{ "/Format/_Decimal",      "<ALT>D",     do_action, ADJUST_SCIENTIFIC_OFF, "<Item>" },
{ "/Format/_Precision...", "<ALT>P",     do_action, ADJUST_PRECISION,   "<Item>" },
{ "/Format/sep1",          NULL,         NULL,      0,                  "<Separator>" },
{ "/Format/Shadow_ed",     "<ALT>E",     do_action, ADJUST_SHADOW,      "<Item>" },
{ "/Format/_Transparent",  "<ALT>T",     do_action, ADJUST_TRANSPARENT, "<Item>" },
{ "/Format/sep1",          NULL,         NULL,      0,                  "<Separator>" },
{ "/Format/Lo_ck",         "<ALT>C",     do_action, ADJUST_LOCK,        "<Item>" },
{ "/Format/_Ignore",       "<ALT>I",     do_action, ADJUST_IGNORE,      "<Item>" },
{ "/_Help",            NULL,             NULL,      0,          "<LastBranch>" },
{ "/Help/_Manual",         "F1",         do_action, K_HELP,     "<Item>" },
{ "/Help/_About...",       NULL,         do_action, K_ABOUT,     "<Item>" }
};

#define max(x,y) ((x)>(y)?(x):(y))
static void newsheet(Sheet *sheet)
{
	GtkSheetRange range;
	int x = max(100, sheet->dimx);
	int y = max(100, sheet->dimy);
	int cur;

	range.col0 = 0;
	range.row0 = 0;
	range.coli = sheet->dimx-1;
	range.rowi = sheet->dimy-1;

	cur = gtk_sheet_get_columns_count(GTK_SHEET(grid));
	if (x > cur) gtk_sheet_add_column(GTK_SHEET(grid), x-cur);
	if (x < cur) gtk_sheet_delete_columns(GTK_SHEET(grid), cur-x, x);

	cur = gtk_sheet_get_rows_count(GTK_SHEET(grid));
	if (y > cur) gtk_sheet_add_row(GTK_SHEET(grid), x-cur);
	if (y < cur) gtk_sheet_delete_rows(GTK_SHEET(grid), cur-y, y);

	if (has_mark) {
		gtk_sheet_range_set_background(GTK_SHEET(grid), &range, &normbg);
		gtk_sheet_range_set_foreground(GTK_SHEET(grid), &range, &normfg);
		has_mark = FALSE;
	}

	gtk_sheet_range_set_border_color(GTK_SHEET(grid), &range, &normbg);
}

#define SEL_COPY  1
#define SEL_CLEAR K_DC
#define SEL_CUT   3

static Key translate_event(GdkEventKey *event)
{
	int ctrl = (event->state&GDK_CONTROL_MASK);
	int alt = (event->state&GDK_META_MASK);
	int shift = (event->state&GDK_SHIFT_MASK);
	int c;

	c = gdk_keyval_to_unicode(event->keyval);
	if (!c) c = K_INVALID;
	switch (event->keyval)
	{
		case GDK_Escape: return K_INVALID;
		case GDK_BackSpace: return K_BACKSPACE;
		case GDK_F8: return ctrl?K_RECALC:K_CLOCK;
		case GDK_F9: return ctrl?K_RECALC:K_CLOCK;
		case GDK_F10: return '/';
		case GDK_Return:
		case GDK_KP_Enter: return alt?K_MENTER:K_ENTER;
		case GDK_c: return ctrl?SEL_COPY:c;
		case GDK_v: return ctrl?K_COPY:c;
		case GDK_x: return ctrl?SEL_CUT:c;
		case GDK_Insert: return shift?K_COPY:ctrl?SEL_COPY:K_NONE;
		case GDK_Delete: return shift?SEL_CUT:SEL_CLEAR;
		case GDK_Home: return ctrl?K_FIRSTL:shift?K_FSHEET:K_HOME;
		case GDK_End:  return ctrl?K_LASTL:shift?K_LSHEET:K_END;
		case GDK_Up:    return shift?K_INVALID:ctrl?K_PPAGE:K_UP;
		case GDK_Down:  return shift?K_INVALID:ctrl?K_NPAGE:K_DOWN;
		case GDK_Right: return shift?K_INVALID:ctrl?K_FPAGE:K_RIGHT;
		case GDK_Left:  return shift?K_INVALID:ctrl?K_BPAGE:K_LEFT;
		case GDK_Page_Down: return shift?K_NSHEET:ctrl?K_LASTL :K_NPAGE;
		case GDK_Page_Up:   return shift?K_PSHEET:ctrl?K_FIRSTL:K_PPAGE;
		default: return c;
	}
}

static void selection_to_mark(Sheet *sheet)
{
	GtkSheetRange *range = &GTK_SHEET(grid)->range;

	sheet->mark1x = range->col0;
	sheet->mark2x = range->coli;
	sheet->mark1y = range->row0;
	sheet->mark2y = range->rowi;
	sheet->mark1z = sheet->mark2z = sheet->curz;
	sheet->marking = 0;
	gtk_sheet_unselect_range(GTK_SHEET(grid));	
	redraw_sheet(sheet);
}

static gboolean key_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	Sheet *sheet = (Sheet*)data;
	GtkSheetRange *range = &GTK_SHEET(grid)->range;

	Key k = translate_event(event);
	if (k == K_INVALID) return FALSE;

	if (k == SEL_CLEAR && GTK_SHEET(grid)->state != GTK_SHEET_NORMAL) {
		selection_to_mark(sheet);
		k = BLOCK_CLEAR;
	}
	if (k == SEL_COPY || k == SEL_CUT) {
		selection_to_mark(sheet);
		return TRUE;
	}
	//if (range->col0 >= 0) gtk_sheet_unselect_range(GTK_SHEET(grid));
	handle_sheetcmd(sheet, k, sheet->moveonly);
	return TRUE;
}

static void size_changed(GtkWidget *widget, GtkAllocation *alloc, gpointer data)
{
	Sheet *sheet = (Sheet*)data;
	GtkSheetRange range;

	gtk_sheet_get_visible_range(GTK_SHEET(grid), &range);
	sheet->width = sheet->maxx = range.coli-range.col0 - 1;
	sheet->maxy = range.rowi-range.row0+3;
}

static gboolean cell_activated(GtkSheet *grid, int row, int column, gpointer data)
{
	Sheet *sheet = (Sheet*)data;

	if (column > 0 && shadowed(sheet,column,row,sheet->curz))
	{
		sheet->curx = column;
		sheet->cury = row;
		handle_sheetcmd(sheet, K_INVALID, 0);
		return TRUE;
	}

	if (sheet->curx == column && sheet->cury == row) return TRUE;

	sheet->curx = column;
	sheet->cury = row;
	redraw_cell(sheet, column, row, sheet->curz);
	return TRUE;
}

static gboolean hide_entry(GtkWidget *grid, GdkEvent *ev, gpointer data)
{
	gtk_widget_hide(grid);
	return TRUE;
}

void display_init(Sheet *cursheet, int always_redraw)
{
	GtkItemFactory *factory;
	GtkAccelGroup *accel_group;
	GtkWidget *hbox;

	gtk_init(0, NULL);
	gdk_colormap_alloc_color(gdk_colormap_get_system(), &markfg, TRUE, TRUE);
	gdk_colormap_alloc_color(gdk_colormap_get_system(), &markbg, TRUE, TRUE);
	gdk_colormap_alloc_color(gdk_colormap_get_system(), &normfg, TRUE, TRUE);
	gdk_colormap_alloc_color(gdk_colormap_get_system(), &normbg, TRUE, TRUE);

	main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(main_window), 600, 400);
	gtk_window_set_title (GTK_WINDOW(main_window), "gTeapot");

	g_signal_connect(G_OBJECT(main_window), "delete_event", G_CALLBACK(delete_event), cursheet);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(main_window), vbox);

	accel_group = gtk_accel_group_new();
	factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<TeapotMain>", accel_group);
	gtk_item_factory_create_items(factory, sizeof(menu)/sizeof(menu[0]), menu, cursheet);
	menubar = gtk_item_factory_get_widget(factory, "<TeapotMain>");
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 2);

	grid = gtk_sheet_new_browser(2, 2, "MainGrid");
	scroller = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scroller), grid);
	gtk_box_pack_start(GTK_BOX(vbox), scroller, TRUE, TRUE, 2);
	gtk_sheet_rows_set_resizable(GTK_SHEET(grid), FALSE);
	gtk_sheet_set_autoresize(GTK_SHEET(grid), FALSE);
	gtk_sheet_set_autoscroll(GTK_SHEET(grid), TRUE);
	gtk_widget_show(grid);

	g_signal_connect(G_OBJECT(gtk_sheet_get_entry(GTK_SHEET(grid))),  "map", G_CALLBACK(hide_entry), cursheet);
	g_signal_connect(G_OBJECT(gtk_sheet_get_entry(GTK_SHEET(grid))),  "key-press-event", G_CALLBACK(key_event), cursheet);
	g_signal_connect(G_OBJECT(grid),  "size-allocate", G_CALLBACK(size_changed), cursheet);
	g_signal_connect(G_OBJECT(grid),  "key-press-event", G_CALLBACK(key_event), cursheet);
	g_signal_connect(G_OBJECT(grid),  "activate", G_CALLBACK(cell_activated), cursheet);

	content = gtk_label_new("");
    gtk_label_set_justify(GTK_LABEL(content), GTK_JUSTIFY_LEFT);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), content, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

	message = gtk_label_new("gTeapot ready.");
    gtk_label_set_justify(GTK_LABEL(message), GTK_JUSTIFY_LEFT);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), message, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

    edit = gtk_hbox_new(FALSE, 0);
	edit_prompt = gtk_label_new("Value:");
	gtk_box_pack_start(GTK_BOX(edit), edit_prompt, FALSE, FALSE, 2);
	edit_text = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(edit), edit_text, TRUE, TRUE, 2);
	gtk_box_pack_end(GTK_BOX(vbox), edit, FALSE, FALSE, 2);

	gtk_window_add_accel_group(GTK_WINDOW(main_window), accel_group);
}

void display_end(void)
{
}

#define SHEET(s,x,y,z) (*(s->sheet+(x)*s->dimz*s->dimy+(y)*s->dimz+(z)))
static void updatecell(Sheet *sheet, int x, int y, int z, int force)
{
	char buf[1024], *old;

	if (!force && x < sheet->dimx && y < sheet->dimy && z < sheet->dimz && SHEET(sheet, x, y, z) && !SHEET(sheet, x, y, z)->updated) return;

	printvalue(buf,sizeof(buf),1,quote,getscientific(sheet,x,y,z),getprecision(sheet,x,y,z),sheet,x,y,z);
	old = gtk_sheet_cell_get_text(GTK_SHEET(grid), y, x);
	if (*buf && (!old || strcmp(buf, old))) gtk_sheet_set_cell_text(GTK_SHEET(grid), y, x, buf);
	else if (!*buf && old && *old) gtk_sheet_cell_clear(GTK_SHEET(grid), y, x);
}

void redraw_sheet(Sheet *sheet)
{
	int x, y, z = sheet->curz;
	int force = 0;
	char buf[32];
	const char *old;
	GtkSheetRange range;

	gtk_widget_hide(grid);
	update_content(sheet);
	newsheet(sheet);

	snprintf(buf, sizeof(buf), "[%i]", sheet->curz);
	old = gtk_button_get_label(GTK_BUTTON(GTK_SHEET(grid)->button));
	if (!old || strcmp(buf, old)) {
		force = 1;
		gtk_button_set_label(GTK_BUTTON(GTK_SHEET(grid)->button), buf);
		gtk_sheet_set_row_titles_width(GTK_SHEET(grid), 80);
	}

	for (x = 0; x < sheet->dimx; x++) {
		GdkRectangle area;
		int w = columnwidth(sheet, x, sheet->curz) * 10;
		gtk_sheet_get_cell_area(GTK_SHEET(grid), 0, x, &area);
		if (area.width != w) gtk_sheet_set_column_width(GTK_SHEET(grid), x, w);
	}

	if (sheet->mark1x >= 0 && z >= sheet->mark1z && z <= sheet->mark2z) {
		range.col0 = sheet->mark1x;
		range.coli = sheet->mark2x;
		range.row0 = sheet->mark1y;
		range.rowi = sheet->mark2y;
		gtk_sheet_range_set_background(GTK_SHEET(grid), &range, &markbg);
		gtk_sheet_range_set_foreground(GTK_SHEET(grid), &range, &markfg);
		has_mark = TRUE;
	}

	for (y = 0; y < sheet->dimy; y++)
		for (x = 0; x < sheet->dimx; x++)
		{
			updatecell(sheet, x, y, sheet->curz, force);
			if (!force) {
				range.col0 = range.coli = x;
				range.row0 = range.rowi = y;
				gtk_sheet_range_set_border(GTK_SHEET(grid), &range, shadowed(sheet,x,y,z)?GTK_SHEET_LEFT_BORDER:0, 1, GDK_LINE_SOLID);
			}
		}
	gtk_widget_show(grid);
}

void redraw_cell(Sheet *sheet, int x, int y, int z)
{
	int cur, w;
	GdkRectangle area;

	if (z != sheet->curz) return;
	if (x == sheet->curx && y == sheet->cury) update_content(sheet);

	cur = gtk_sheet_get_columns_count(GTK_SHEET(grid));
	if (sheet->dimx > cur) gtk_sheet_add_column(GTK_SHEET(grid), sheet->dimx-cur);
	cur = gtk_sheet_get_rows_count(GTK_SHEET(grid));
	if (sheet->dimy > cur) gtk_sheet_add_row(GTK_SHEET(grid), sheet->dimy-cur);

	w = columnwidth(sheet, x, sheet->curz) * 10;
	gtk_sheet_get_cell_area(GTK_SHEET(grid), 0, x, &area);
	if (x < sheet->dimx && area.width != w) gtk_sheet_set_column_width(GTK_SHEET(grid), x, w);

	updatecell(sheet, x, y, z, 1);

	if (sheet->mark1x >= 0
		&& x >= sheet->mark1x && x <= sheet->mark2x
		&& y >= sheet->mark1y && y <= sheet->mark2y
		&& z >= sheet->mark1z && z <= sheet->mark2z)
	{
		GtkSheetRange range;
		range.col0 = x;
		range.row0 = y;
		range.coli = x;
		range.rowi = y;
		gtk_sheet_range_set_background(GTK_SHEET(grid), &range, &markbg);
		gtk_sheet_range_set_foreground(GTK_SHEET(grid), &range, &markfg);
	}
}

static void update_content(Sheet *sheet)
{
	char buf[1024];
	const char *label;
	char *err;
	char moveonly;
	int x, y;

	while (sheet->curx > 0 && shadowed(sheet,sheet->curx,sheet->cury,sheet->curz)) sheet->curx--;

	label = getlabel(sheet,sheet->curx,sheet->cury,sheet->curz);
	assert(label!=(const char*)0);

	moveonly=sheet->moveonly ? *MOVEONLY : *MOVEEDIT;

	if (*label=='\0') snprintf(buf, sizeof(buf), "%c @(%d,%d,%d)=",moveonly,sheet->curx,sheet->cury,sheet->curz);
	else snprintf(buf, sizeof(buf), "%c @(%s)=",moveonly,label);

	gtk_sheet_get_active_cell(GTK_SHEET(grid), &y, &x);
	if (x != sheet->curx || y != sheet->cury) gtk_sheet_set_active_cell(GTK_SHEET(grid), sheet->cury, sheet->curx);

	if ((err=geterror(sheet,sheet->curx,sheet->cury,sheet->curz)))
	{
		strncpy(buf+strlen(buf),err,sizeof(buf)-strlen(buf));
		free(err);
		buf[strlen(buf)-1]='\0';
	}
	else
	{
		print(buf+strlen(buf),sizeof(buf)-strlen(buf),0,1,getscientific(sheet,sheet->curx,sheet->cury,sheet->curz),-1,getcont(sheet,sheet->curx,sheet->cury,sheet->curz,0));
		if (getcont(sheet,sheet->curx,sheet->cury,sheet->curz,1) && strlen(buf)<(size_t)(sheet->maxx+1-4))
		{
			snprintf(buf+strlen(buf),sizeof(buf)-strlen(buf)," -> ");
			print(buf+strlen(buf),sizeof(buf)-strlen(buf),0,1,getscientific(sheet,sheet->curx,sheet->cury,sheet->curz),-1,getcont(sheet,sheet->curx,sheet->cury,sheet->curz,1));
		}
	}
	gtk_label_set_text(GTK_LABEL(content), buf);
}

static int edit_rc = 0;
static gboolean edit_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	Sheet *sheet = (Sheet*)data;
	Key k;

	if (event->state == 0 && (event->keyval == GDK_Escape || event->keyval == GDK_Return || event->keyval == GDK_KP_Enter)) {
		if (sheet) sheet->moveonly = 0;
		edit_rc = (event->keyval == GDK_Escape?-1:0);
		g_main_loop_quit(input);
		return TRUE;
	} else if (sheet && ((event->state == GDK_CONTROL_MASK && event->keyval == GDK_o) || event->keyval == GDK_Tab)) {
		sheet->moveonly = !sheet->moveonly;
		return TRUE;
	} else if (sheet && sheet->moveonly) {
		if (event->state == 0 && (event->keyval == GDK_v || event->keyval == GDK_p)) {
			char valbuf[1024];
			gint pos;

			if (event->keyval == GDK_v)
				printvalue(valbuf,sizeof(valbuf),0,1,getscientific(sheet,sheet->curx,sheet->cury,sheet->curz),-1,sheet,sheet->curx,sheet->cury,sheet->curz);
			else
				sprintf(valbuf,"(%d,%d,%d)",sheet->curx,sheet->cury,sheet->curz);

			gtk_editable_delete_selection(GTK_EDITABLE(edit_text));
			pos = gtk_editable_get_position(GTK_EDITABLE(edit_text));
			gtk_editable_insert_text(GTK_EDITABLE(edit_text), valbuf, strlen(valbuf), &pos);
			gtk_editable_select_region(GTK_EDITABLE(edit_text), pos, pos);
			gtk_editable_set_position(GTK_EDITABLE(edit_text), pos);
		}

		k = translate_event(event);
		if (k == K_INVALID) return FALSE;
		handle_sheetcmd(sheet, k, 1);

		return TRUE;
	}
	return FALSE;
}

static gboolean keep_focus(GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	int pos = gtk_editable_get_position(GTK_EDITABLE(edit_text));
	gtk_widget_grab_focus(widget);
	gtk_editable_select_region(GTK_EDITABLE(edit_text), pos, pos);
	return TRUE;
}

int line_edit(Sheet *sheet, char *buf, size_t size, const char *prompt, size_t *x, size_t *offx)
{
	gulong conn[2] = { 0, 0 };

	if (input)
	{
		line_msg(NULL, "Action not possible at this time.");
		return -1;
	}

	gtk_label_set_text(GTK_LABEL(edit_prompt), prompt);
	gtk_entry_set_max_length(GTK_ENTRY(edit_text), size);
	gtk_entry_set_text(GTK_ENTRY(edit_text), buf);
	gtk_editable_set_position(GTK_EDITABLE(edit_text), *x);
	gtk_editable_select_region(GTK_EDITABLE(edit_text), *x, *x);
	gtk_widget_show(edit);
	gtk_widget_set_sensitive(menubar, FALSE);
	gtk_widget_grab_focus(edit_text);

	conn[0] = g_signal_connect(G_OBJECT(edit_text), "key-press-event", G_CALLBACK(edit_event), sheet);
	conn[1] = g_signal_connect(G_OBJECT(edit_text), "focus-out-event", G_CALLBACK(keep_focus), sheet);

	edit_rc = 0;
	input = g_main_loop_new(NULL, 0);
	g_main_loop_run(input);
	g_main_loop_unref(input);
	input = NULL;

	g_signal_handler_disconnect(G_OBJECT(edit_text), conn[0]);
	g_signal_handler_disconnect(G_OBJECT(edit_text), conn[1]);
	gtk_widget_set_sensitive(menubar, TRUE);

	gtk_widget_hide(edit);

	memcpy(buf, gtk_entry_get_text(GTK_ENTRY(edit_text)), size);
	return edit_rc;
}

int line_ok(const char *prompt, int curx)
{
	int rc = 0;

	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", prompt);
	rc = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES;
	gtk_widget_destroy (dialog);

	return rc;
}

void line_msg(const char *prompt, const char *msg)
{
	static char lmsg[1024];
	snprintf(lmsg, sizeof(lmsg), "[%s%s%s]", (prompt?prompt:""), (prompt?" ":""), msg);
    gtk_label_set_text(GTK_LABEL(message), lmsg);
}

int keypressed(void){return 0;}

Key show_menu(Sheet *cursheet)
{
	gtk_menu_shell_select_first(GTK_MENU_SHELL(menubar), TRUE);
	return K_INVALID;
}

int line_menu(const char *prompt, const MenuChoice *choice, int curx)
{
	int rc = 0;
	int n = 0;

	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, "%s", prompt);
	while (choice->str)
	{
		gtk_dialog_add_button(GTK_DIALOG(dialog), choice->str+1, n++);
		choice++;
	}
	rc = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy (dialog);

	return (rc < 0?-1:rc);
}

