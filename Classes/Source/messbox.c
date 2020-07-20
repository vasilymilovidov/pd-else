// based on entry

#include <m_pd.h>
#include <g_canvas.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#define FONT_NAME       "deja vu sans mono"
#define HANDLE_SIZE     15
#define MIN_WIDTH       60
#define MIN_HEIGHT      30
#define BORDER          5

typedef struct _messbox{
    t_object        x_obj;
    t_canvas       *x_canvas;
    t_glist        *x_glist;
    t_symbol       *x_bind_sym;
    int             x_height;
    int             x_open;
    int             x_width;
    int             x_resizing;
    int             x_active;
    char            x_fgcolor[8];
    unsigned int    x_fg[3];    // fg RGB color
    char            x_bgcolor[8];
    unsigned int    x_bg[3];    // bg RGB color
    int             x_font_size;
    t_symbol       *x_font_weight;
    int             x_selected;
    char           *tcl_namespace;
    char           *x_cv_id;
    char           *frame_id;
    char           *text_id;
    char           *handle_id;
    char           *window_tag;
    char           *all_tag;
    t_outlet       *x_data_outlet;
    t_clock        *x_clock;
}t_messbox;

static t_class *messbox_class;
static t_widgetbehavior messbox_widgetbehavior;

// widget helper functions
static void set_tk_widget_ids(t_messbox *x, t_canvas *canvas){
//    post("set_tk_widget_ids");
    char buf[MAXPDSTRING];
    x->x_canvas = canvas;
    sprintf(buf,".x%lx.c", (long unsigned int)canvas);
    x->x_cv_id = getbytes(strlen(buf)+1); // Tk ID for the current canvas this object belongs
    strcpy(x->x_cv_id, buf);
    sprintf(buf,"%s.frame%lx", x->x_cv_id, (long unsigned int)x);
    x->frame_id = getbytes(strlen(buf)+1); // Tk ID for the "frame" the other things are drawn in
    strcpy(x->frame_id, buf);
    sprintf(buf,"%s.text%lx", x->frame_id, (long unsigned int)x);
    x->text_id = getbytes(strlen(buf)+1); // Tk ID for the "text"
    strcpy(x->text_id, buf);
    sprintf(buf,"%s.window%lx", x->x_cv_id, (long unsigned int)x);
    x->window_tag = getbytes(strlen(buf)+1); // Tk ID for the resizing "window"
    strcpy(x->window_tag, buf);
    sprintf(buf,"%s.handle%lx", x->x_cv_id, (long unsigned int)x);
    x->handle_id = getbytes(strlen(buf)+1); // Tk ID for the resizing "handle"
    strcpy(x->handle_id, buf);
    sprintf(buf,"all%lx", (long unsigned int)x);
    x->all_tag = getbytes(strlen(buf)+1); // Tk ID for ALL
    strcpy(x->all_tag, buf);
}

static void draw_box(t_messbox *x){
    int xpos = text_xpix(&x->x_obj, x->x_glist), ypos = text_ypix(&x->x_obj, x->x_glist);
    sys_vgui("%s create rectangle %d %d %d %d -outline black -fill \"%s\" -width 1 -tags %x_outline\n",
        x->x_cv_id, xpos, ypos, xpos+x->x_width, ypos+x->x_height, x->x_bgcolor, x);
    sys_vgui("%s create rectangle %d %d %d %d -fill black -tags {%x_inlet %s}\n",
        x->x_cv_id, xpos, ypos, xpos+IOWIDTH, ypos+IHEIGHT, x, x->all_tag);
    sys_vgui("%s create rectangle %d %d %d %d -fill black -tags {%x_outlet %s}\n",
        x->x_cv_id, xpos, ypos+x->x_height-1, xpos+IOWIDTH, ypos+x->x_height-IHEIGHT-1, x, x->all_tag);
}

static void erase_box(t_messbox *x){
    sys_vgui("%s delete %x_outline\n", x->x_cv_id, x);
    sys_vgui("%s delete %x_inlet\n", x->x_cv_id, x);
    sys_vgui("%s delete %x_outlet\n", x->x_cv_id, x);
}

static void messbox_draw(t_messbox *x, t_glist *glist){
//    post("messbox_draw");
    set_tk_widget_ids(x, glist_getcanvas(glist));
    // I guess this is for fine-tuning of the rect size based on width and height
    int xpos = text_xpix(&x->x_obj, glist), ypos = text_ypix(&x->x_obj, glist);
    sys_vgui("namespace eval messbox%lx {}\n", x);
    sys_vgui("destroy %s\n", x->frame_id);  // Seems we gotta delete it if it exists
    sys_vgui("frame %s\n", x->frame_id);
    sys_vgui("text %s -font {{%s} %d %s} -highlightthickness 0 -bg \"%s\" -fg \"%s\"\n",
        x->text_id, FONT_NAME, x->x_font_size, x->x_font_weight->s_name,
        x->x_bgcolor, x->x_fgcolor);
    sys_vgui("pack %s -side left -fill both -expand 1\n", x->text_id);
    sys_vgui("pack %s -side bottom -fill both -expand 1\n", x->frame_id);
//    bind_button_events;
    sys_vgui("bind %s <Button> {pdtk_canvas_mouse %s \
        [expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] %%b 0}\n",
        x->text_id, x->x_cv_id, x->x_cv_id, x->x_cv_id);
    sys_vgui("bind %s <ButtonRelease> {pdtk_canvas_mouseup %s \
        [expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] %%b}\n",
        x->text_id, x->x_cv_id, x->x_cv_id, x->x_cv_id);
    sys_vgui("bind %s <Button-2> {pdtk_canvas_rightclick %s \
        [expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] %%b}\n",
        x->text_id, x->x_cv_id, x->x_cv_id, x->x_cv_id);
    sys_vgui("bind %s <Button-3> {pdtk_canvas_rightclick %s \
        [expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] %%b}\n",
        x->text_id, x->x_cv_id, x->x_cv_id, x->x_cv_id);
    sys_vgui("bind %s <Shift-Button> {pdtk_canvas_mouse %s \
        [expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] %%b 1}\n",
        x->text_id, x->x_cv_id, x->x_cv_id, x->x_cv_id);
// mouse motion
    sys_vgui("bind %s <Motion> {pdtk_canvas_motion %s \
        [expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] 0}\n",
        x->text_id, x->x_cv_id, x->x_cv_id, x->x_cv_id);
    sys_vgui("%s create window %d %d -anchor nw -window %s -tags {%s %s} -width %d -height %d\n",
        x->x_cv_id, xpos+BORDER, ypos+BORDER, x->frame_id, x->window_tag, x->all_tag,
        x->x_width-BORDER*2, x->x_height-BORDER*2);
    draw_box(x);
}

static void messbox_erase(t_messbox* x,t_glist* glist){
//    post("messbox_erase");
    set_tk_widget_ids(x, glist_getcanvas(glist));
    sys_vgui("%s delete %x_outline\n", x->x_cv_id, x);
    sys_vgui("%s delete %x_inlet\n", x->x_cv_id, x);
    sys_vgui("%s delete %x_outlet\n", x->x_cv_id, x);
    sys_vgui("destroy %s\n", x->frame_id);
    sys_vgui("%s delete %s\n", x->x_cv_id, x->all_tag);
}

// ------------------------ text widgetbehaviour------------------------------------------------
static void messbox_getrect(t_gobj *z, t_glist *owner, int *xp1, int *yp1, int *xp2, int *yp2){
//  (post("messbox_getrect")
    t_messbox *x = (t_messbox*)z;
    *xp1 = text_xpix(&x->x_obj, owner);
    *yp1 = text_ypix(&x->x_obj, owner);
    *xp2 = *xp1 + x->x_width;
    *yp2 = *yp1 + x->x_height; 
}

static void messbox_displace(t_gobj *z, t_glist *glist, int dx, int dy){
//    post("messbox_displace");
    t_messbox *x = (t_messbox *)z;
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
    if(glist_isvisible(glist)){
        set_tk_widget_ids(x, glist_getcanvas(glist));
        sys_vgui("%s move %s %d %d\n", x->x_cv_id, x->all_tag, dx, dy);
        sys_vgui("%s move %x_outline %d %d\n", x->x_cv_id, x, dx, dy);
        sys_vgui("%s move RSZ %d %d\n", x->x_cv_id, dx, dy);
        canvas_fixlinesfor(glist, (t_text*)x);
    }
}

static void messbox_select(t_gobj *z, t_glist *glist, int state){
//    post("messbox_select: state %d", state);
    t_messbox *x = (t_messbox *)z;
    if((state) && (!x->x_selected)){
        if(!x->x_selected){
            sys_vgui("%s configure -fg blue -state disabled -cursor $cursor_editmode_nothing\n", x->text_id);
            x->x_selected = 1;
        }
        int x1, y1, x2, y2;
        messbox_getrect(z, glist, &x1, &y1, &x2, &y2);
        sys_vgui("canvas %s -width %d -height %d -bg #ddd -bd 0 \
            -highlightthickness 3 -highlightcolor {#f00} -cursor bottom_right_corner\n",
            x->handle_id, HANDLE_SIZE, HANDLE_SIZE);
        int handle_x1 = x2 - HANDLE_SIZE;
        int handle_y1 = y2 - HANDLE_SIZE;
        sys_vgui("%s create window %d %d -anchor nw -width %d -height %d -window %s -tags RSZ\n",
            x->x_cv_id, handle_x1-2, handle_y1-2, HANDLE_SIZE, HANDLE_SIZE, x->handle_id);
        sys_vgui("bind %s <Button> {pdsend {%s _resize 1}}\n",
            x->handle_id, x->x_bind_sym->s_name);
        sys_vgui("bind %s <ButtonRelease> {pdsend {%s _resize 0}}\n",
            x->handle_id, x->x_bind_sym->s_name);
        sys_vgui("bind %s <Motion> {pdsend {%s _motion %%x %%y }}\n",
            x->handle_id, x->x_bind_sym->s_name);
    }
    else{
        sys_vgui("%s configure -fg %s -state normal -cursor xterm\n", x->text_id, x->x_fgcolor);
        sys_vgui("destroy %s\n", x->handle_id);
        x->x_selected = 0;
    }
}

static void messbox_delete(t_gobj *z, t_glist *glist){
//    post("messbox_delete");
    t_text *x = (t_text *)z;
    canvas_deletelinesfor(glist, x);
}

static void messbox_vis(t_gobj *z, t_glist *gl, int vis){
//    post("messbox_vis: vis %d", vis);
    t_messbox *x = (t_messbox*)z;
    vis ? messbox_draw(x, gl) : messbox_erase(x, gl);
}

/////////// METHODS !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
static void messbox_output(t_messbox* x, t_symbol *s, int argc, t_atom *argv){
    if(!argc)
        outlet_bang(x->x_data_outlet);
    else if(argv->a_type == A_FLOAT)
        outlet_list(x->x_data_outlet, s, argc, argv);
    else
        outlet_anything(x->x_data_outlet, atom_getsymbol(argv), argc-1, argv+1);
//    outlet_anything(x->x_data_outlet, s, ac, av);
}

static void messbox_anything(t_messbox* x, t_symbol *s, int argc, t_atom *argv){
//    post("messbox_anything x->x_open = %d", x->x_open);
    if(x->x_open)
        outlet_anything(x->x_data_outlet, s, argc, argv);
}

static void messbox_tick(t_messbox *x){
    x->x_open = 0;
//    post("messbox_tick x->x_open = %d", x->x_open);
}

static void messbox_bang(t_messbox* x){ // Escape "," and ";"
//    sys_vgui("pdsend \"%s output [string map {\",\" \"\\\\,\" \";\" \"\\\\;\"} [%s get 0.0 end]] \"\n",
    x->x_open = 1;
//    post("messbox_bang x->x_open = %d", x->x_open);
    sys_vgui("pdsend \"%s output [%s get 0.0 end] \"\n",
        x->x_bind_sym->s_name, x->text_id); // send to messbox_output above
    clock_delay(x->x_clock, 5);
}

// Clear
static void messbox_do_clear(t_messbox* x){
//    post("messbox_do_clear");
    sys_vgui("%s configure -state normal\n", x->text_id);
    sys_vgui("%s delete 0.0 end \n", x->text_id);
    if(!x->x_active)
        sys_vgui("%s configure -state disabled\n", x->text_id);
}

// Clear
static void messbox_clear(t_messbox* x, t_symbol *s, int argc, t_atom *argv){
//    post("messbox_clear");
//    post("x->x_open = %d", x->x_open);
    x->x_open ? outlet_anything(x->x_data_outlet, s, argc, argv) : messbox_do_clear(x);
}

static void messbox_append(t_messbox* x,  t_symbol *s, int argc, t_atom *argv){
//    DEBUG(post("messbox_append"););
    sys_vgui("%s configure -state normal\n", x->text_id);
    t_symbol *tmp_symbol = s; // get rid of the unused variable warning
    t_float tmp_float;
    for(int i = 0; i < argc; i++){
        tmp_symbol = atom_getsymbolarg(i, argc, argv);
        if(tmp_symbol == &s_){
            tmp_float = atom_getfloatarg(i, argc , argv);
            sys_vgui("lappend ::%s::list %g \n", x->tcl_namespace, tmp_float );
        }
        else
            sys_vgui("lappend ::%s::list %s \n", x->tcl_namespace, tmp_symbol->s_name );
    }
    sys_vgui("append ::%s::list \" \"\n", x->tcl_namespace);
    sys_vgui("%s insert end $::%s::list ; unset ::%s::list \n", 
        x->text_id, x->tcl_namespace, x->tcl_namespace );
    sys_vgui("%s yview end-2char \n", x->text_id );
    if(!x->x_active)
        sys_vgui("%s configure -state disabled\n", x->text_id);
}

// Set
static void messbox_set(t_messbox* x,  t_symbol *s, int argc, t_atom *argv){
//    DEBUG(post("messbox_set"););
    sys_vgui("%s configure -state normal\n", x->text_id);
    messbox_do_clear(x);
    messbox_append(x, s, argc, argv);
    if(!x->x_active)
        sys_vgui("%s configure -state disabled\n", x->text_id);
}

void messbox_bgcolor(t_messbox* x, t_floatarg r, t_floatarg g, t_floatarg b){
    x->x_bg[0] = r < 0 ? 0 : r > 255 ? 255 : (int)r;
    x->x_bg[1] = g < 1 ? 0 : g > 255 ? 255 : (int)g;
    x->x_bg[2] = b < 2 ? 0 : b > 255 ? 255 : (int)b;
    sprintf(x->x_bgcolor, "#%2.2x%2.2x%2.2x", x->x_bg[0], x->x_bg[1], x->x_bg[2]);
	sys_vgui("%s configure -background \"%s\"\n", x->text_id, x->x_bgcolor);
    sys_vgui("%s itemconfigure %x_outline -fill %s\n", x->x_cv_id, (unsigned long)x, x->x_bgcolor);
}

void messbox_fgcolor(t_messbox* x, t_floatarg r, t_floatarg g, t_floatarg b){
    x->x_fg[0] = r < 0 ? 0 : r > 255 ? 255 : (int)r;
    x->x_fg[1] = g < 1 ? 0 : g > 255 ? 255 : (int)g;
    x->x_fg[2] = b < 2 ? 0 : b > 255 ? 255 : (int)b;
    sprintf(x->x_fgcolor, "#%2.2x%2.2x%2.2x", x->x_fg[0], x->x_fg[1], x->x_fg[2]);
	sys_vgui("%s configure -foreground \"%s\" \n", x->text_id, x->x_fgcolor);
}

static void messbox_bold(t_messbox *x, t_float bold){
    x->x_font_weight = (int)(bold != 0) ? gensym("bold") : gensym("normal");
    sys_vgui("%s configure -font {{%s} %d %s}\n", x->text_id, FONT_NAME,
        x->x_font_size, x->x_font_weight->s_name);
}

static void messbox_fontsize(t_messbox *x, t_float font_size){
    x->x_font_size = font_size < 8 ? 8 : (int)font_size;
    sys_vgui("%s configure -font {{%s} %d %s}\n",
        x->text_id, FONT_NAME, x->x_font_size, x->x_font_weight->s_name);
}

static void messbox_size(t_messbox *x, t_float width, t_float height){
//    DEBUG(post("messbox_size"););
    x->x_height = height < MIN_HEIGHT ? MIN_HEIGHT : (int)height;
    x->x_width = width < MIN_WIDTH ? MIN_WIDTH : (int)width;
    if(glist_isvisible(x->x_glist)){
        sys_vgui("%s itemconfigure %s -width %d -height %d\n",
            x->x_cv_id, x->window_tag, x->x_width-BORDER*2, x->x_height-BORDER*2);
        canvas_fixlinesfor(x->x_glist, (t_text *)x);
        erase_box(x), draw_box(x);
    }
}

static void messbox_key(void *z, t_floatarg f){
    t_messbox *x = z;
    char c = (char)f;
//    post("c = %d", (int)c);
    if(c == 0){
//        post("%s disabled", x->text_id);
        sys_vgui("%s configure -state disabled\n", x->text_id);
        x->x_active = 0;
    }
    if(c == 10){
        // messbox_bang(x);
    }
}

static int messbox_click(t_gobj *z, t_glist *glist, int xpix, int ypix, int shift, int alt, int dbl, int doit){
    xpix = ypix = alt = dbl = 0;
    if(doit){
        t_messbox *x = (t_messbox *)z;
        x->x_active = 1;
//        sys_vgui("%s focus %s{}\n", x->x_cv_id, x->text_id);
        sys_vgui("%s configure -state normal\n", x->text_id);
        glist_grab(glist, (t_gobj *)x, 0, messbox_key, 0, 0);
        if(shift)
            messbox_bang(x);
    }
    return(1);
}

// callback functions
static void messbox_resize_callback(t_messbox *x, t_floatarg f){ // ????
    x->x_resizing = (int)f;
}

static void messbox_motion_callback(t_messbox *x, t_floatarg f1, t_floatarg f2){
//    post("messbox_motion_callback");
    if(x->x_resizing){
        int dx = (int)f1, dy = (int)f2;
        int w = x->x_width, h = x->x_height;
        if(glist_isvisible(x->x_glist)){
            if(w+dx < MIN_WIDTH){
                x->x_width = MIN_WIDTH;
                dx = MIN_WIDTH - w;
            }
            else
                x->x_width+=dx;
            if(h+dy < MIN_HEIGHT){
                x->x_height = MIN_HEIGHT;
                dy = MIN_HEIGHT-h;
            }
            else
                x->x_height+=dy;
            sys_vgui("%s itemconfigure %s -width %d -height %d\n",
                x->x_cv_id, x->window_tag, x->x_width-BORDER*2, x->x_height-BORDER*2);
            sys_vgui("%s move RSZ %d %d\n", x->x_cv_id, dx, dy);
            erase_box(x), draw_box(x);
            canvas_fixlinesfor(x->x_glist, (t_text *)x);
        }
    }
}

static void messbox_save(t_gobj *z, t_binbuf *b){
    t_messbox *x = (t_messbox *)z;
    binbuf_addv(b, "ssiisiiiiiiiiii;", &s__X, gensym("obj"),
        x->x_obj.te_xpix, x->x_obj.te_ypix, atom_getsymbol(binbuf_getvec(x->x_obj.te_binbuf)),
        x->x_width, x->x_height, x->x_bg[0], x->x_bg[1], x->x_bg[2],
        x->x_fg[0], x->x_fg[1], x->x_fg[2],
        x->x_font_weight == gensym("bold"), x->x_font_size);
}

static void messbox_free(t_messbox *x){
    pd_unbind(&x->x_obj.ob_pd, x->x_bind_sym);
    if(x->x_clock)
            clock_free(x->x_clock);
}

static void *messbox_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_messbox *x = (t_messbox *)pd_new(messbox_class);
    char buf[MAXPDSTRING];
    x->x_font_size = 12;
    int weigth = 0;
    x->x_selected = 0;
    x->x_width = x->x_height = 130;
    x->x_bg[0] = x->x_bg[1] = x->x_bg[2] = 235;
    x->x_fg[0] = x->x_fg[1] = x->x_fg[2] = 0;
    if(argc){
		x->x_width = atom_getint(argv);
		x->x_height = atom_getint(argv+1);
		x->x_bg[0] = (unsigned int)atom_getfloatarg(2, argc, argv);
        x->x_bg[1] = (unsigned int)atom_getfloatarg(3, argc, argv);
        x->x_bg[2] = (unsigned int)atom_getfloatarg(4, argc, argv);
        x->x_fg[0] = (unsigned int)atom_getfloatarg(5, argc, argv);
        x->x_fg[1] = (unsigned int)atom_getfloatarg(6, argc, argv);
        x->x_fg[2] = (unsigned int)atom_getfloatarg(7, argc, argv);
        weigth = (int)(atom_getfloatarg(8, argc, argv) != 0);
        x->x_font_size = (int)(atom_getfloatarg(9, argc, argv));
    }
    if(x->x_font_size < 8)
        x->x_font_size = 8;
    sprintf(x->x_bgcolor, "#%2.2x%2.2x%2.2x", x->x_bg[0], x->x_bg[1], x->x_bg[2]);
    sprintf(x->x_fgcolor, "#%2.2x%2.2x%2.2x", x->x_fg[0], x->x_fg[1], x->x_fg[2]);
    x->x_font_weight = weigth ? gensym("bold") : gensym("normal");
    x->x_clock = clock_new(x, (t_method)messbox_tick);
    x->x_resizing = x->x_active = x->x_open = 0;
    x->x_data_outlet = outlet_new(&x->x_obj, &s_float);
    sprintf(buf,"messbox%lx",(long unsigned int)x);
    x->tcl_namespace = getbytes(strlen(buf)+1);
    strcpy(x->tcl_namespace, buf);
    sprintf(buf,"#%s", x->tcl_namespace);
    x->x_bind_sym = gensym(buf);
    pd_bind(&x->x_obj.ob_pd, x->x_bind_sym);
    x->x_glist = canvas_getcurrent();
    return(x);
}

void messbox_setup(void) {
    messbox_class = class_new(gensym("messbox"), (t_newmethod)messbox_new,
        (t_method)messbox_free, sizeof(t_messbox), 0, A_GIMME, 0);
	class_addbang(messbox_class, (t_method)messbox_bang);
    class_addanything(messbox_class, (t_method)messbox_anything);
    class_addmethod(messbox_class, (t_method)messbox_size, gensym("size"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(messbox_class, (t_method)messbox_fontsize, gensym("fontsize"), A_DEFFLOAT, 0);
    class_addmethod(messbox_class, (t_method)messbox_bold, gensym("bold"), A_DEFFLOAT, 0);
	class_addmethod(messbox_class, (t_method)messbox_set, gensym("set"), A_GIMME, 0);
	class_addmethod(messbox_class, (t_method)messbox_append, gensym("append"), A_GIMME, 0);
	class_addmethod(messbox_class, (t_method)messbox_clear, gensym("clear"), A_GIMME, 0);
	class_addmethod(messbox_class, (t_method)messbox_bgcolor, gensym("bgcolor"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
	class_addmethod(messbox_class, (t_method)messbox_fgcolor, gensym("fgcolor"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(messbox_class, (t_method)messbox_output, gensym("output"), A_GIMME, 0);
    class_addmethod(messbox_class, (t_method)messbox_click, gensym("click"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(messbox_class, (t_method)messbox_resize_callback, gensym("_resize"), A_FLOAT, 0);
    class_addmethod(messbox_class, (t_method)messbox_motion_callback, gensym("_motion"), A_FLOAT, A_FLOAT, 0);
    class_setwidget(messbox_class, &messbox_widgetbehavior);
    class_setsavefn(messbox_class, messbox_save);
    messbox_widgetbehavior.w_getrectfn  = messbox_getrect;
    messbox_widgetbehavior.w_displacefn = messbox_displace;
    messbox_widgetbehavior.w_selectfn   = messbox_select;
    messbox_widgetbehavior.w_activatefn = NULL;
    messbox_widgetbehavior.w_deletefn   = messbox_delete;
    messbox_widgetbehavior.w_visfn      = messbox_vis;
    messbox_widgetbehavior.w_clickfn    = (t_clickfn)messbox_click;
}
