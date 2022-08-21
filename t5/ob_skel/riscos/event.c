/************************************************************************/
/* © Acorn Computers Ltd, 1992.                                         */
/*                                                                      */
/* This file forms part of an unsupported source release of RISC_OSLib. */
/*                                                                      */
/* It may be freely used to create executable images for saleable       */
/* products but cannot be sold in source form or as an object library   */
/* without the prior written consent of Acorn Computers Ltd.            */
/*                                                                      */
/* If this file is re-distributed (even if modified) it should retain   */
/* this copyright notice.                                               */
/*                                                                      */
/************************************************************************/

/* Title: c.event
 * Purpose: central processing for window sytem events.
 * History: IDJ: 06-Feb-92: prepared for source release
 *
 */

#if RISCOS

static
struct EVENT_STATICS
{
    abstract_menu_handle current_menu;
    event_menu_maker current_menu_maker;
    event_menu_proc  current_menu_event;
    void *           current_menu_handle;

    BOOL current_menu_real;

    BOOL recreation;
    BOOL recreatepending;

    struct _event_single_click_cache_data
    {
        WimpMouseClickEvent mce;
    } single_click;

    struct _event_triple_click_cache_data
    {
        WimpMouseClickEvent mce;
        MONOTIME      t;
    } triple_click;

    struct _event_menu_click_cache_data
    {
        WimpMouseClickEvent mce;
        BOOL valid;
    } menu_click;

    struct _event_submenu_opening_cache_data
    {
        WimpMenu *  m;
        int         x;
        int         y;
        BOOL        valid;
    } submenu;
}
event_;

typedef struct _mstr
{
    abstract_menu_handle m;
    event_menu_maker maker;
    event_menu_proc  event;
    void *           handle;
}
mstr;

static BOOL
event__attach(
    _InVal_     wimp_w window_handle,
    _InVal_     wimp_i icon_handle,
    abstract_menu_handle m,
    event_menu_maker menumaker,
    event_menu_proc eventproc,
    void * handle)
{
    mstr * p = winx_menu_get_handle(window_handle, icon_handle);

    if(!m  &&  !menumaker)
    {
        /* cancelling */
        if(p)
        {
            winx_menu_set_handle(window_handle, icon_handle, NULL);
            al_ptr_dispose(P_P_ANY_PEDANTIC(&p));
        }
        else
            trace_0(TRACE_RISCOS_HOST, TEXT("no menu processor/already freed"));
    }
    else
    {
        if(NULL == p)
        {
            STATUS status;
            if(NULL == (p = al_ptr_calloc_elem(mstr, 1, &status)))
                return(FALSE);
        }
        p->m        = m;
        p->maker    = menumaker;
        p->event    = eventproc;
        p->handle   = handle;
        winx_menu_set_handle(window_handle, icon_handle, p);
    }

    return(TRUE);
}

/* ------------------------ event_attachmenumaker -------------------------
 * Description:   Attach to the given window, a function which makes a menu
 *                when the user invokes a menu

 * Parameters:    event_w -- the window to which the menu maker should be
 *                           attached
 *                event_menu_maker -- the menu maker function
 *                event_menu_proc -- handler for the menu
 *                void *handle -- caller-defined handle
 * Returns:       TRUE if able to attach menu maker
 * Other Info:    This works similarly to event_attachmenu, except that it
 *                allows you to make any last minute changes to flags in the
 *                menu etc. (e.g. ticks/fades), before displaying it. Call
 *                with event_menu_maker==0 removes attachment

 */

extern BOOL
event_register_window_menumaker(
    _InVal_     wimp_w window_handle,
    event_menu_maker menumaker,
    event_menu_proc eventproc,
    void * handle)
{
    return(event__attach(window_handle, (wimp_i) -1 /*WORKAREA*/, NULL, menumaker, eventproc, handle));
}

extern BOOL
event_register_iconbar_menumaker(
    _InVal_     wimp_i icon_handle,
    event_menu_maker menumaker,
    event_menu_proc eventproc,
    void * handle)
{
    return(event__attach(ICONBAR_WIMP_W, icon_handle, NULL, menumaker, eventproc, handle));
}

/* -------- Processing Events. -------- */

static void
event__createmenu(
    WimpMenu * m)
{
    /* allow icon bar recreates not to restore menu position */
    if((event_.menu_click.mce.window_handle == ICONBAR_WIMP_W)  &&  !event_.recreation)
    {
        /* move icon bar menus up to standard position. */
        WimpMenuItem * mi = &m->items[0];

        event_.menu_click.mce.mouse_y = wimp_iconbar_height;

        trace_0(TRACE_RISCOS_HOST, TEXT("positioning icon bar menu"));
        do  {
            event_.menu_click.mce.mouse_y += m->item_height + m->gap;
        }
        while(!((mi++)->flags & WimpMenuItem_Last));
    }

    (void) event_create_menu(m, event_.menu_click.mce.mouse_x - 64, event_.menu_click.mce.mouse_y);
}

extern void
event_popup_menu(
    event_menu_maker maker,
    event_menu_proc proc,
    void * handle)
{
    event_.menu_click.valid = TRUE;
    event_.recreation = FALSE;

    event_.current_menu_maker  = maker;
    event_.current_menu_event  = proc;
    event_.current_menu_handle = handle;

    event_.current_menu_real = 0;
    event_.current_menu = (* event_.current_menu_maker) (event_.current_menu_handle);
    assert(event_.current_menu_real);
    event__createmenu((WimpMenu *) event_.current_menu);

    event_.menu_click.valid = FALSE;
}

/* A hit on the submenu pointer may be interpreted as slightly different
 * from actually clicking on the menu entry with the arrow. The most
 * important example of this is the standard "save" menu item, where clicking
 * on "Save" causes a save immediately while following the arrow gets
 * the standard save dbox up.
*/

static void
event_preprocess_Message_MenuWarning(
    _Inout_     int * p_event_code,
    _Inout_     WimpPollBlock * const p_event_data)
{
    const WimpMenuWarningMessage * const p_wimp_message_menu_warning = (const WimpMenuWarningMessage *) &p_event_data->user_message.data;
    int i;

    trace_1(TRACE_RISCOS_HOST, TEXT("event_do_process(%s)"), report_wimp_event(*p_event_code, p_event_data));
    trace_0(TRACE_RISCOS_HOST, TEXT("hit on submenu => pointer, fake menu hit"));

    /* cache submenu opening info for use later on this event */
    event_.submenu.m = p_wimp_message_menu_warning->submenu.m;
    event_.submenu.x = p_wimp_message_menu_warning->x;
    event_.submenu.y = p_wimp_message_menu_warning->y;

    *p_event_code = Wimp_EMenuSelection;

    i = 0;
    do  {
        p_event_data->words[i] = p_wimp_message_menu_warning->menu[i];

        if(i == 9)
        {   /* ensure we don't walk off end if badly terminated */
            p_event_data->words[i] = -1;
            break;
        }
    }
    while(p_event_data->words[i++] != -1);
}

/* Menu hit */

static BOOL
event_process_Menu_Selection(
    _In_        const WimpPollBlock * const p_event_data,
    _InVal_     BOOL submenu_fake_hit)
{
    char hit[20];
    int i;
    mstr * p = NULL;

    /*if(event_.menuclick.m.w != ICONBAR_WIMP_W)*/
        p = winx_menu_get_handle(event_.menu_click.mce.window_handle, event_.menu_click.mce.icon_handle);

    if(NULL == p)
        p = winx_menu_get_handle(event_.menu_click.mce.window_handle, BAD_WIMP_I);

    trace_0(TRACE_RISCOS_HOST, TEXT("menu hit|"));

    if(NULL != p)
    {
        assert(p->event  == event_.current_menu_event);
        assert(p->handle == event_.current_menu_handle);
    }

    if(!submenu_fake_hit)
    {
        event_.recreatepending = winx_adjustclicked();
    }
    else
    {
        /* say the submenu opening cache is valid */
        event_.submenu.valid = TRUE;
        event_.recreatepending = FALSE;
    }

    /* form array of one-based menu hits ending in 0 */
    i = 0;
    do  {
        hit[i] = (char) (p_event_data->words[i] + 1); /* convert hit on 0 to 1 etc. */
        trace_1(TRACE_RISCOS_HOST, TEXT("| [%d]|"), hit[i]);
        if(i == 9)
        {   /* ensure we don't walk off end if badly terminated */
            hit[i] = 0;
            break;
        }
    }
    while(p_event_data->words[i++] != -1);

    trace_1(TRACE_RISCOS_HOST, TEXT("| Adjust = %s"), report_boolstring(event_.recreatepending));

    /* allow access to initial click cache during handler */
    event_.menu_click.valid = TRUE;

    if(! (* event_.current_menu_event) (event_.current_menu_handle, hit, submenu_fake_hit))
    {
        if(submenu_fake_hit)
        {
            /* handle unprocessed submenu open events */
            WimpMenu * submenu;
            int x, y;
            status_assert(event_read_submenudata(&submenu, &x, &y));
            void_WrapOsErrorReporting(event_create_submenu(submenu, x, y));
        }
    }

    /* submenu opening cache no longer valid */
    event_.submenu.valid = FALSE;

    if(event_.recreatepending)
    {
        /* Twas an ADJ-hit on a menu item.
         * The menu should be recreated.
        */
        trace_0(TRACE_RISCOS_HOST, TEXT("menu hit caused by Adjust - recreating menu"));
        event_.menu_click.valid = TRUE;
        event_.recreation = 1;

        event_.current_menu_real = 0;
        event_.current_menu = (* event_.current_menu_maker) (event_.current_menu_handle);
        assert(event_.current_menu_real);
        event__createmenu((WimpMenu *) event_.current_menu);
    }
#if TRACE_ALLOWED
    else if(submenu_fake_hit)
        trace_0(TRACE_RISCOS_HOST, TEXT("menu hit was faked"));
    else
        trace_0(TRACE_RISCOS_HOST, TEXT("menu hit caused by Select - let tree collapse"));
#endif

    /* initial click cache no longer valid */
    event_.menu_click.valid = FALSE;

    return(FALSE);
}

/*
SKS: process event: returns true if idle
*/

extern BOOL
event_do_process(
    _In_        int event_code,
    _Inout_     WimpPollBlock * const p_event_data)
{
    BOOL submenu_fake_hit = FALSE;

    /* Look for submenu requests, and if found turn them into menu hits. */
    /* People wishing to respond can pick up the original from wimpt. */
    if((event_code == Wimp_EUserMessage)  &&  (p_event_data->user_message.hdr.action_code == Wimp_MMenuWarning))
    {
        event_preprocess_Message_MenuWarning(&event_code, p_event_data);
        submenu_fake_hit = TRUE;
    }

    /* Look for events to do with menus */
    if(event_code == Wimp_EMouseClick)
    {
        wimp_w window_handle = p_event_data->mouse_click.window_handle;
        wimp_i icon_handle = p_event_data->mouse_click.icon_handle;
        mstr * p = NULL;

        trace_1(TRACE_RISCOS_HOST, TEXT("event_do_process(%s)"), report_wimp_event(event_code, p_event_data));

        if((int) window_handle < 0)
            window_handle = ICONBAR_WIMP_W;

        if(p_event_data->mouse_click.buttons & Wimp_MouseButtonMenu)
        {
            /* don't use the Menu button to get menus for registered to icons; fake work area */
            if(!p && (window_handle == ICONBAR_WIMP_W))
                p = winx_menu_get_handle(window_handle, icon_handle);

            if(NULL == p)
            {
                icon_handle = (wimp_i) -1 /*WORKAREA*/;

                p = winx_menu_get_handle(window_handle, BAD_WIMP_I);
            }
        }
        else if((window_handle != ICONBAR_WIMP_W) && (icon_handle != (wimp_i) -1 /*WORKAREA*/))
            /* look for menu registered to that icon in that window */
            p = winx_menu_get_handle(window_handle, icon_handle);

        /* note the click info anyway for event_popup_menu() */
        event_.menu_click.mce.window_handle = window_handle; /* may have mutated into ICONBAR_WIMP_W */
        event_.menu_click.mce.icon_handle = icon_handle; /* may have mutated too */
        event_.menu_click.mce.mouse_x = p_event_data->mouse_click.mouse_x;
        event_.menu_click.mce.mouse_y = p_event_data->mouse_click.mouse_y;
        /*event_.menu_click.mce.buttons = e->data.mouse_click.buttons;*/

        if(p)
        {
            event_.menu_click.valid = TRUE;
            event_.recreation = FALSE;

            event_.current_menu        = p->m;
            event_.current_menu_maker  = p->maker;
            event_.current_menu_event  = p->event;
            event_.current_menu_handle = p->handle;

            assert(!event_.current_menu);
            assert(event_.current_menu_maker); /* else registered menu with no means of creation!!! */

            event_.current_menu_real = 0;
            event_.current_menu = (* event_.current_menu_maker) (event_.current_menu_handle);
            assert(event_.current_menu_real);
            event__createmenu((WimpMenu *) event_.current_menu);

            event_.menu_click.valid = FALSE;

            trace_0(TRACE_RISCOS_HOST, TEXT("menu created"));
            return(FALSE);
        }
    }

    if((event_code == Wimp_EMenuSelection)  &&  event_.current_menu)
    {
        trace_1(TRACE_RISCOS_HOST, TEXT("event_do_process(%s)"), report_wimp_event(event_code, p_event_data));
        return(event_process_Menu_Selection(p_event_data, submenu_fake_hit));
    }

    /* now dispatch the event for processing */
    return(winx_dispatch_event(event_code, p_event_data));
}

/* ----------------------- event_clear_current_menu ------------------------
 * Description:   Clears the current menu tree.

 * Parameters:    void.
 * Returns:       void.
 * Other Info:    To be used when you are not sure that all menus have been
 *                cleared from the screen.

 */

extern void
event_clear_current_menu(void)
{
    trace_0(TRACE_OUT | TRACE_ANY, TEXT("event_clear_current_menu"));

    (void) event_create_menu((WimpMenu *) -1, 0, 0);
}

/*
SKS: for win to zap pendingrecreate
*/

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
event_create_menu(
    WimpMenu * m,
    int x,
    int y)
{
    _kernel_oserror * e;

    event_.recreatepending = FALSE;

#if 1
    e = wimp_create_menu(m, x, y);
    return(NULL);
#else
    return(wimp_create_menu(m, x, y));
#endif
}

_Check_return_
_Ret_maybenull_
extern _kernel_oserror *
event_create_submenu(
    WimpMenu * m,
    int x,
    int y)
{
    _kernel_oserror * e;

    event_.recreatepending = FALSE;

#if 1
    e = wimp_create_submenu(m, x, y);
    return(NULL);
#else
    return(wimp_create_submenu(m, x, y));
#endif
}

/* ------------------ event_is_menu_being_recreated ------------------------
 * Description:   Informs caller if a menu is being recreated.

 * Parameters:    void.
 * Returns:       void.
 * Other Info:    Useful for when the RISC OS library is recreating a menu
 *                due to Adjust click (call it in your menu makers).

 */

extern BOOL
event_is_menu_being_recreated(void)
{
    return(event_.recreation);
}

/*
SKS: read the menu recreation pending state
*/

extern BOOL
event_is_menu_recreate_pending(void)
{
    return(event_.recreatepending);
}

/*
SKS: read the cache of submenu opening data - only valid during menu processing!
*/

_Check_return_
extern STATUS
event_read_submenudata(
    _Out_opt_   WimpMenu ** smp,
    _Out_opt_   int * const xp,
    _Out_opt_   int * const yp)
{
    if(!event_.submenu.valid)
        return(STATUS_FAIL);

    if(smp)
        *smp = event_.submenu.m;

    if(xp)
        *xp = event_.submenu.x;

    if(yp)
        *yp = event_.submenu.y;

    return(STATUS_OK);
}

extern int
event_query_submenudata_valid(void)
{
    return(event_.submenu.valid);
}

/*
SKS: inform event that menu passed back is a real live one suitable for Window Manager
*/

_Check_return_
extern abstract_menu_handle
event_return_real_menu(
    WimpMenu * p_menuhdr)
{
    event_.current_menu_real = 1;

    return((abstract_menu_handle) p_menuhdr);
}

#endif /* RISCOS */

/* end of event.c */
