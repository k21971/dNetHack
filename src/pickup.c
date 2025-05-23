/*	SCCS Id: @(#)pickup.c	3.4	2003/07/27	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *	Contains code for picking objects up, and container use.
 */

#include "hack.h"

STATIC_DCL void FDECL(simple_look, (struct obj *,BOOLEAN_P));
#ifndef GOLDOBJ
STATIC_DCL boolean FDECL(query_classes, (char *,boolean *,boolean *,
		const char *,struct obj *,BOOLEAN_P,BOOLEAN_P,int *));
#else
STATIC_DCL boolean FDECL(query_classes, (char *,boolean *,boolean *,
		const char *,struct obj *,BOOLEAN_P,int *));
#endif
STATIC_DCL void FDECL(check_here, (BOOLEAN_P));
STATIC_DCL boolean FDECL(n_or_more, (struct obj *, int));
STATIC_DCL boolean FDECL(all_but_uchain, (struct obj *, int));
#if 0 /* not used */
STATIC_DCL boolean FDECL(allow_cat_no_uchain, (struct obj *));
#endif
STATIC_DCL int FDECL(autopick, (struct obj*, int, menu_item **));
STATIC_DCL int FDECL(count_categories, (struct obj *,int));
STATIC_DCL long FDECL(carry_count,
		      (struct obj *,struct obj *,long,BOOLEAN_P,int *,int *));
STATIC_DCL int FDECL(lift_object, (struct obj *,struct obj *,long *,BOOLEAN_P));
STATIC_DCL boolean FDECL(mbag_explodes, (struct obj *,int));
STATIC_PTR int FDECL(in_container,(struct obj *));
STATIC_PTR int FDECL(ck_bag,(struct obj *));
STATIC_PTR int FDECL(out_container,(struct obj *));
STATIC_DCL long FDECL(mbag_item_gone, (int,struct obj *));
STATIC_DCL int FDECL(menu_loot, (int, struct obj *, BOOLEAN_P));
STATIC_DCL int FDECL(in_or_out_menu, (const char *,struct obj *, BOOLEAN_P, BOOLEAN_P));
STATIC_DCL void FDECL(tipcontainer, (struct obj *));
STATIC_DCL int NDECL(tiphat);
STATIC_DCL int FDECL(container_at, (int, int, BOOLEAN_P));
STATIC_DCL boolean FDECL(able_to_loot, (int, int, boolean));
STATIC_DCL boolean FDECL(mon_beside, (int, int));
STATIC_DCL int FDECL(use_lightsaber, (struct obj *));
STATIC_DCL char NDECL(pick_gemstone);
STATIC_DCL char NDECL(pick_bullet);
STATIC_DCL char NDECL(pick_coin);
STATIC_DCL struct obj * FDECL(pick_creatures_armor, (struct monst *, int *));
STATIC_DCL struct obj * FDECL(pick_armor_for_creature, (struct monst *));

/* define for query_objlist() and autopickup() */
#define FOLLOW(curr, flags) \
    (((flags) & BY_NEXTHERE) ? (curr)->nexthere : (curr)->nobj)

/*
 * Used by container code to check if they're dealing with the special magic
 * chest container.
 */
#define cobj_is_magic_chest(cobj) ((cobj)->otyp == MAGIC_CHEST)

/*
 *  How much the weight of the given container will change when the given
 *  object is removed from it.  This calculation must match the one used
 *  by weight() in mkobj.c.
 */
#define DELTA_CWT(cont,obj)		\
    ((cont)->cursed ? (obj)->owt * 2 :	\
		      1 + ((obj)->owt / ((cont)->blessed ? 4 : 2)))
#define GOLD_WT(n)		(((n) + 50L) / 100L)
/* if you can figure this out, give yourself a hearty pat on the back... */
#define GOLD_CAPACITY(w,n)	(((w) * -100L) - ((n) + 50L) - 1L)

static const char moderateloadmsg[] = "You have a little trouble lifting";
static const char nearloadmsg[] = "You have much trouble lifting";
static const char overloadmsg[] = "You have extreme difficulty lifting";

/* BUG: this lets you look at cockatrice corpses while blind without
   touching them */
/* much simpler version of the look-here code; used by query_classes() */
STATIC_OVL void
simple_look(otmp, here)
struct obj *otmp;	/* list of objects */
boolean here;		/* flag for type of obj list linkage */
{
	/* Neither of the first two cases is expected to happen, since
	 * we're only called after multiple classes of objects have been
	 * detected, hence multiple objects must be present.
	 */
	if (!otmp) {
	    impossible("simple_look(null)");
	} else if (!(here ? otmp->nexthere : otmp->nobj)) {
	    pline("%s", doname(otmp));
	} else {
	    winid tmpwin = create_nhwindow(NHW_MENU);
	    putstr(tmpwin, 0, "");
	    do {
		putstr(tmpwin, 0, doname(otmp));
		otmp = here ? otmp->nexthere : otmp->nobj;
	    } while (otmp);
	    display_nhwindow(tmpwin, TRUE);
	    destroy_nhwindow(tmpwin);
	}
}

#ifndef GOLDOBJ
int
collect_obj_classes(ilets, otmp, here, incl_gold, filter, itemcount)
char ilets[];
register struct obj *otmp;
boolean here, incl_gold;
boolean FDECL((*filter),(OBJ_P));
int *itemcount;
#else
int
collect_obj_classes(ilets, otmp, here, filter, itemcount)
char ilets[];
register struct obj *otmp;
boolean here;
boolean FDECL((*filter),(OBJ_P));
int *itemcount;
#endif
{
	register int iletct = 0;
	register char c;

	*itemcount = 0;
#ifndef GOLDOBJ
	if (incl_gold)
	    ilets[iletct++] = def_oc_syms[COIN_CLASS];
#endif
	ilets[iletct] = '\0'; /* terminate ilets so that index() will work */
	while (otmp) {
	    c = def_oc_syms[(int)otmp->oclass];
	    if (!index(ilets, c) && (!filter || (*filter)(otmp)))
		ilets[iletct++] = c,  ilets[iletct] = '\0';
	    *itemcount += 1;
	    otmp = here ? otmp->nexthere : otmp->nobj;
	}

	return iletct;
}

/*
 * Suppose some '?' and '!' objects are present, but '/' objects aren't:
 *	"a" picks all items without further prompting;
 *	"A" steps through all items, asking one by one;
 *	"?" steps through '?' items, asking, and ignores '!' ones;
 *	"/" becomes 'A', since no '/' present;
 *	"?a" or "a?" picks all '?' without further prompting;
 *	"/a" or "a/" becomes 'A' since there aren't any '/'
 *	    (bug fix:  3.1.0 thru 3.1.3 treated it as "a");
 *	"?/a" or "a?/" or "/a?",&c picks all '?' even though no '/'
 *	    (ie, treated as if it had just been "?a").
 */
#ifndef GOLDOBJ
STATIC_OVL boolean
query_classes(oclasses, one_at_a_time, everything, action, objs,
	      here, incl_gold, menu_on_demand)
char oclasses[];
boolean *one_at_a_time, *everything;
const char *action;
struct obj *objs;
boolean here, incl_gold;
int *menu_on_demand;
#else
STATIC_OVL boolean
query_classes(oclasses, one_at_a_time, everything, action, objs,
	      here, menu_on_demand)
char oclasses[];
boolean *one_at_a_time, *everything;
const char *action;
struct obj *objs;
boolean here;
int *menu_on_demand;
#endif
{
	char ilets[20], inbuf[BUFSZ];
	int iletct, oclassct;
	boolean not_everything;
	char qbuf[QBUFSZ];
	boolean m_seen;
	int itemcount;

	oclasses[oclassct = 0] = '\0';
	*one_at_a_time = *everything = m_seen = FALSE;
	iletct = collect_obj_classes(ilets, objs, here,
#ifndef GOLDOBJ
				     incl_gold,
#endif
				     (boolean FDECL((*),(OBJ_P))) 0, &itemcount);
	if (iletct == 0) {
		return FALSE;
	} else if (iletct == 1) {
		oclasses[0] = def_char_to_objclass(ilets[0]);
		oclasses[1] = '\0';
		if (itemcount && menu_on_demand) {
			ilets[iletct++] = 'm';
			*menu_on_demand = 0;
			ilets[iletct] = '\0';
		}
	} else  {	/* more than one choice available */
		const char *where = 0;
		register char sym, oc_of_sym, *p;
		/* additional choices */
		ilets[iletct++] = ' ';
		ilets[iletct++] = 'a';
		ilets[iletct++] = 'A';
		ilets[iletct++] = (objs == invent ? 'i' : ':');
		if (menu_on_demand) {
			ilets[iletct++] = 'm';
			*menu_on_demand = 0;
		}
		ilets[iletct] = '\0';
ask_again:
		oclasses[oclassct = 0] = '\0';
		*one_at_a_time = *everything = FALSE;
		not_everything = FALSE;
		Sprintf(qbuf,"What kinds of thing do you want to %s? [%s]",
			action, ilets);
		getlin(qbuf,inbuf);
		if (*inbuf == '\033') return FALSE;

		for (p = inbuf; (sym = *p++); ) {
		    /* new A function (selective all) added by GAN 01/09/87 */
		    if (sym == ' ') continue;
		    else if (sym == 'A') *one_at_a_time = TRUE;
		    else if (sym == 'a') *everything = TRUE;
		    else if (sym == ':') {
			simple_look(objs, here);  /* dumb if objs==invent */
			goto ask_again;
		    } else if (sym == 'i') {
			(void) display_inventory((char *)0, TRUE);
			goto ask_again;
		    } else if (sym == 'm') {
			m_seen = TRUE;
		    } else {
			oc_of_sym = def_char_to_objclass(sym);
			if (index(ilets,sym)) {
			    add_valid_menu_class(oc_of_sym);
			    oclasses[oclassct++] = oc_of_sym;
			    oclasses[oclassct] = '\0';
			} else {
			    if (!where)
				where = !strcmp(action,"pick up")  ? "here" :
					!strcmp(action,"take out") ?
							    "inside" : "";
			    if (*where)
				There("are no %c's %s.", sym, where);
			    else
				You("have no %c's.", sym);
			    not_everything = TRUE;
			}
		    }
		}
		if (m_seen && menu_on_demand) {
			*menu_on_demand = (*everything || !oclassct) ? -2 : -3;
			return FALSE;
		}
		if (!oclassct && (!*everything || not_everything)) {
		    /* didn't pick anything,
		       or tried to pick something that's not present */
		    *one_at_a_time = TRUE;	/* force 'A' */
		    *everything = FALSE;	/* inhibit 'a' */
		}
	}
	return TRUE;
}

/* look at the objects at our location, unless there are too many of them */
STATIC_OVL void
check_here(picked_some)
boolean picked_some;
{
	register struct obj *obj;
	register int ct = 0;

	/* count the objects here */
	for (obj = level.objects[u.ux][u.uy]; obj; obj = obj->nexthere) {
	    if (obj != uchain)
		ct++;
	}

	/* If there are objects here, take a look. */
	if (ct) {
	    if (flags.run) nomul(0, NULL);
	    flush_screen(1);
	    (void) look_here(ct, picked_some);
	} else {
	    read_engr_at(u.ux,u.uy);
	}
}

/* Value set by query_objlist() for n_or_more(). */
static long val_for_n_or_more;

/* query_objlist callback: return TRUE if obj's count is >= reference value */
STATIC_OVL boolean
n_or_more(struct obj *obj, int qflags)
{
	if(qflags&NO_EQUIPMENT && obj->owornmask)
		return FALSE;
    if (obj == uchain) return FALSE;
    return (obj->quan >= val_for_n_or_more);
}

/* List of valid menu classes for query_objlist() and allow_category callback */
static char valid_menu_classes[MAXOCLASSES + 2];

void
add_valid_menu_class(int c)
{
	static int vmc_count = 0;

	if (c == 0)  /* reset */
	  vmc_count = 0;
	else
	  valid_menu_classes[vmc_count++] = (char)c;
	valid_menu_classes[vmc_count] = '\0';
}

/* query_objlist callback: return TRUE if not uchain */
STATIC_OVL boolean
all_but_uchain(struct obj *obj, int qflags)
{
	if(qflags&NO_EQUIPMENT && obj->owornmask)
		return FALSE;
    return (obj != uchain);
}

/* query_objlist callback: return TRUE */
/*ARGSUSED*/
boolean
allow_all_nomods(struct obj *obj)
{
    return TRUE;
}

/* query_objlist callback: return TRUE */
/*ARGSUSED*/
boolean
allow_all(struct obj *obj, int qflags)
{
	if(qflags&NO_EQUIPMENT && obj->owornmask)
		return FALSE;
    return TRUE;
}

boolean
allow_category(struct obj *obj, int qflags)
{
	if(qflags&NO_EQUIPMENT && obj->owornmask)
		return FALSE;
    if (Role_if(PM_PRIEST)) obj->bknown = TRUE;
    if (((index(valid_menu_classes,'u') != (char *)0) && obj->unpaid) ||
	(index(valid_menu_classes, obj->oclass) != (char *)0))
	return TRUE;
    else if (((index(valid_menu_classes,'U') != (char *)0) &&
	(obj->oclass != COIN_CLASS && obj->bknown && !obj->blessed && !obj->cursed)))
	return TRUE;
    else if (((index(valid_menu_classes,'B') != (char *)0) &&
	(obj->oclass != COIN_CLASS && obj->bknown && obj->blessed)))
	return TRUE;
    else if (((index(valid_menu_classes,'C') != (char *)0) &&
	(obj->oclass != COIN_CLASS && obj->bknown && obj->cursed)))
	return TRUE;
    else if (((index(valid_menu_classes,'X') != (char *)0) &&
	(obj->oclass != COIN_CLASS && !obj->bknown)))
	return TRUE;
    else
	return FALSE;
}

#if 0 /* not used */
/* query_objlist callback: return TRUE if valid category (class), no uchain */
STATIC_OVL boolean
allow_cat_no_uchain(struct obj *obj, int qflags)
{
	if(qflags&NO_EQUIPMENT && obj->owornmask)
		return FALSE;
    if ((obj != uchain) &&
	(((index(valid_menu_classes,'u') != (char *)0) && obj->unpaid) ||
	(index(valid_menu_classes, obj->oclass) != (char *)0)))
	return TRUE;
    else
	return FALSE;
}
#endif

/* query_objlist callback: return TRUE if valid class and worn */
boolean
is_worn_by_type(struct obj *otmp, int qflags)
{
	return((boolean)(!!(otmp->owornmask &
			(W_ARMOR | W_RING | W_AMUL | W_BELT | W_TOOL | W_WEP | W_SWAPWEP | W_QUIVER)))
	        && (index(valid_menu_classes, otmp->oclass) != (char *)0));
}

/*
 * Have the hero pick things from the ground
 * or a monster's inventory if swallowed.
 *
 * Arg what:
 *	>0  autopickup
 *	=0  interactive
 *	<0  pickup count of something
 *
 * Returns 1 if tried to pick something up, whether
 * or not it succeeded.
 */
int
pickup(what)
int what;		/* should be a long */
{
	int i, n, res, count, n_tried = 0, n_picked = 0;
	menu_item *pick_list = (menu_item *) 0;
	boolean autopickup = what > 0;
	boolean skipmessages = what == 2;
	struct obj *objchain;
	int traverse_how;

	if (what < 0)		/* pick N of something */
	    count = -what;
	else			/* pick anything */
	    count = 0;

	if (!u.uswallow) {
		struct trap *ttmp = t_at(u.ux, u.uy);
		/* no auto-pick if no-pick move, nothing there, or in a pool */
		if (autopickup && (flags.nopick || !OBJ_AT(u.ux, u.uy) ||
			(is_pool(u.ux, u.uy, FALSE) && !Underwater) || (is_lava(u.ux, u.uy) && !likes_lava(youracedata)))) {
			read_engr_at(u.ux, u.uy);
			return (0);
		}

		/* no pickup if levitating & not on air or water level */
		if (!can_reach_floor()) {
		    if ((multi && !flags.run) || (autopickup && !flags.pickup))
			read_engr_at(u.ux, u.uy);
		    return (0);
		}
		/*	Bug fix, it used to be that you could yank statues off traps
			before they could activate.
			Now a statue trap aborts the grab attempt, then activates. */
		if(ttmp && ttmp->ttyp == STATUE_TRAP){
			return 0;
		}
		if (ttmp && ttmp->tseen) {
		    /* Allow pickup from holes and trap doors that you escaped
		     * from because that stuff is teetering on the edge just
		     * like you, but not pits, because there is an elevation
		     * discrepancy with stuff in pits.
		     */
		    if ((ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT) &&
			(!u.utrap || (u.utrap && u.utraptype != TT_PIT)) && !Flying) {
			read_engr_at(u.ux, u.uy);
			return(0);
		    }
		}
		/* multi && !flags.run means they are in the middle of some other
		 * action, or possibly paralyzed, sleeping, etc.... and they just
		 * teleported onto the object.  They shouldn't pick it up.
		 */
		if ((multi && !flags.run) || (autopickup && !flags.pickup)) {
		    if(!skipmessages) check_here(FALSE);
		    return (0);
		}

		/* if there's anything here, stop running */
		if (OBJ_AT(u.ux,u.uy) && flags.run && flags.run != 8 && !flags.nopick) nomul(0, NULL);
	}

	add_valid_menu_class(0);	/* reset */
	if (!u.uswallow) {
		objchain = level.objects[u.ux][u.uy];
		traverse_how = BY_NEXTHERE;
	} else {
		objchain = u.ustuck->minvent;
		traverse_how = 0|NO_EQUIPMENT;	/* nobj */
	}
	/*
	 * Start the actual pickup process.  This is split into two main
	 * sections, the newer menu and the older "traditional" methods.
	 * Automatic pickup has been split into its own menu-style routine
	 * to make things less confusing.
	 */
	if (autopickup) {
	    n = autopick(objchain, traverse_how, &pick_list);
	    goto menu_pickup;
	}

	if (flags.menu_style != MENU_TRADITIONAL || iflags.menu_requested) {

	    /* use menus exclusively */
	    if (count) {	/* looking for N of something */
		char buf[QBUFSZ];
		Sprintf(buf, "Pick %d of what?", count);
		val_for_n_or_more = count;	/* set up callback selector */
		n = query_objlist(buf, objchain,
			    traverse_how|AUTOSELECT_SINGLE|INVORDER_SORT,
			    &pick_list, PICK_ONE, n_or_more);
		/* correct counts, if any given */
		for (i = 0; i < n; i++)
		    pick_list[i].count = count;
	    } else {
		n = query_objlist("Pick up what?", objchain,
			traverse_how|AUTOSELECT_SINGLE|INVORDER_SORT|FEEL_COCKATRICE,
			&pick_list, PICK_ANY, all_but_uchain);
	    }
menu_pickup:
	    n_tried = n;
	    for (n_picked = i = 0 ; i < n; i++) {
		res = pickup_object(pick_list[i].item.a_obj,pick_list[i].count,
					FALSE);
		if (res < 0) break;	/* can't continue */
		n_picked += res;
	    }
	    if (pick_list) free((genericptr_t)pick_list);

	} else {
	    /* old style interface */
	    int ct = 0;
	    long lcount;
	    boolean all_of_a_type, selective;
	    char oclasses[MAXOCLASSES];
	    struct obj *obj, *obj2;

	    oclasses[0] = '\0';		/* types to consider (empty for all) */
	    all_of_a_type = TRUE;	/* take all of considered types */
	    selective = FALSE;		/* ask for each item */

	    /* check for more than one object */
	    for (obj = objchain; obj;
			obj = (traverse_how&BY_NEXTHERE) ? obj->nexthere : obj->nobj
		 ){
			if(traverse_how&NO_EQUIPMENT && obj->owornmask)
				continue;
			ct++;
		}

	    if (ct == 1 && count) {
		/* if only one thing, then pick it */
		obj = objchain;
		lcount = min(obj->quan, (long)count);
		n_tried++;
	    for (obj = objchain; obj;
			obj = (traverse_how&BY_NEXTHERE) ? obj->nexthere : obj->nobj
		 ){
			if(traverse_how&NO_EQUIPMENT && obj->owornmask)
				continue;
			if (pickup_object(obj, lcount, FALSE) > 0)
				n_picked++;	/* picked something */
			goto end_query;
		}

	    } else if (ct >= 2) {
		int via_menu = 0;

		There("are %s objects here.",
		      (ct <= 10) ? "several" : "many");
		if (!query_classes(oclasses, &selective, &all_of_a_type,
				   "pick up", objchain,
				   !!(traverse_how&BY_NEXTHERE),
#ifndef GOLDOBJ
				   FALSE,
#endif
				   &via_menu)) {
		    if (!via_menu) return (0);
		    n = query_objlist("Pick up what?",
				  objchain,
				  traverse_how|(selective ? 0 : INVORDER_SORT),
				  &pick_list, PICK_ANY,
				  via_menu == -2 ? allow_all : allow_category);
		    goto menu_pickup;
		}
	    }

	    for (obj = objchain; obj; obj = obj2) {
		if (traverse_how&BY_NEXTHERE)
			obj2 = obj->nexthere;	/* perhaps obj will be picked up */
		else
			obj2 = obj->nobj;
		lcount = -1L;

		if(traverse_how&NO_EQUIPMENT && obj->owornmask)
			continue;

		if (!selective && oclasses[0] && !index(oclasses,obj->oclass))
		    continue;

		if (!all_of_a_type) {
		    char qbuf[BUFSZ];
		    Sprintf(qbuf, "Pick up %s?",
			safe_qbuf("", sizeof("Pick up ?"), doname(obj), an(simple_typename(obj->otyp)), "something"));
		    switch ((obj->quan < 2L) ? ynaq(qbuf) : ynNaq(qbuf)) {
		    case 'q': goto end_query;	/* out 2 levels */
		    case 'n': continue;
		    case 'a':
			all_of_a_type = TRUE;
			if (selective) {
			    selective = FALSE;
			    oclasses[0] = obj->oclass;
			    oclasses[1] = '\0';
			}
			break;
		    case '#':	/* count was entered */
			if (!yn_number) continue; /* 0 count => No */
			lcount = (long) yn_number;
			if (lcount > obj->quan) lcount = obj->quan;
			/* fall thru */
		    default:	/* 'y' */
			break;
		    }
		}
		if (lcount == -1L) lcount = obj->quan;

		n_tried++;
		if ((res = pickup_object(obj, lcount, FALSE)) < 0) break;
		n_picked += res;
	    }
end_query:
	    ;	/* semicolon needed by brain-damaged compilers */
	}

	if (!u.uswallow) {
		if (!OBJ_AT(u.ux,u.uy)) u.uundetected = 0;

		/* position may need updating (invisible hero) */
		if (n_picked) newsym(u.ux,u.uy);

		/* see whether there's anything else here, after auto-pickup is done */
		if (autopickup && !skipmessages) check_here(n_picked > 0);
	}
	return (n_tried > 0);
}

#ifdef AUTOPICKUP_EXCEPTIONS
boolean
is_autopickup_exception(obj, grab)
struct obj *obj;
boolean grab;	 /* forced pickup, rather than forced leave behind? */
{
	/*
	 *  Does the text description of this match an exception?
	 */
	char *objdesc = makesingular(doname(obj));
	struct autopickup_exception *ape = (grab) ?
					iflags.autopickup_exceptions[AP_GRAB] :
					iflags.autopickup_exceptions[AP_LEAVE];
	while (ape) {
	    if (ape->is_regexp) {
		if (regexec(&ape->match, objdesc, 0, NULL, 0) == 0) return TRUE;
	    } else {
		if (pmatch(ape->pattern, objdesc)) return TRUE;
	    }
		ape = ape->next;
	}
	return FALSE;
}
#endif /* AUTOPICKUP_EXCEPTIONS */

STATIC_OVL boolean
autopick_testobj(otmp, calc_costly)
struct obj *otmp;
boolean calc_costly;
{
    static boolean costly = FALSE;
    const char *otypes = flags.pickup_types;
    boolean pickit = FALSE;

    if (calc_costly) {
	costly = otmp->where == OBJ_FLOOR
	    && costly_spot(otmp->ox, otmp->oy);
    }

    if (costly && !otmp->no_charge)
	return FALSE;

    pickit = !*otypes || index(otypes, otmp->oclass);

#ifdef AUTOPICKUP_EXCEPTIONS
    if (!pickit)
	pickit = is_autopickup_exception(otmp, TRUE);

    pickit = pickit && !is_autopickup_exception(otmp, FALSE);
#endif

    if (!pickit)
	pickit = iflags.pickup_thrown && otmp->was_thrown;

    return pickit;
}

/*
 * Pick from the given list using flags.pickup_types.  Return the number
 * of items picked (not counts).  Create an array that returns pointers
 * and counts of the items to be picked up.  If the number of items
 * picked is zero, the pickup list is left alone.  The caller of this
 * function must free the pickup list.
 */
STATIC_OVL int
autopick(olist, follow, pick_list)
struct obj *olist;	/* the object list */
int follow;		/* how to follow the object list */
menu_item **pick_list;	/* list of objects and counts to pick up */
{
	menu_item *pi;	/* pick item */
	struct obj *curr;
	int n;
	boolean check_costly = TRUE;

	/* first count the number of eligible items */
	for (n = 0, curr = olist; curr; curr = FOLLOW(curr, follow)) {
           if (autopick_testobj(curr, check_costly))
		n++;
	   check_costly = FALSE;
	}
	    

	if (n) {
	    *pick_list = pi = (menu_item *) alloc(sizeof(menu_item) * n);
	    for (n = 0, curr = olist; curr; curr = FOLLOW(curr, follow))
               if (autopick_testobj(curr, FALSE)) {
		    pi[n].item.a_obj = curr;
		    pi[n].count = curr->quan;
		    n++;
		}
	}
	return n;
}


/*
 * Put up a menu using the given object list.  Only those objects on the
 * list that meet the approval of the allow function are displayed.  Return
 * a count of the number of items selected, as well as an allocated array of
 * menu_items, containing pointers to the objects selected and counts.  The
 * returned counts are guaranteed to be in bounds and non-zero.
 *
 * Query flags:
 *	BY_NEXTHERE	  - Follow object list via nexthere instead of nobj.
 *	AUTOSELECT_SINGLE - Don't ask if only 1 object qualifies - just
 *			    use it.
 *	USE_INVLET	  - Use object's invlet.
 *	INVORDER_SORT	  - Use hero's pack order.
 *	SIGNAL_NOMENU	  - Return -1 rather than 0 if nothing passes "allow".
 *	SIGNAL_ESCAPE	  - Return -2 if menu was escaped.
 */
int
query_objlist(qstr, olist, qflags, pick_list, how, allow)
const char *qstr;		/* query string */
struct obj *olist;		/* the list to pick from */
int qflags;			/* options to control the query */
menu_item **pick_list;		/* return list of items picked */
int how;			/* type of query */
boolean FDECL((*allow), (OBJ_P, int));/* allow function */
{
#ifdef SORTLOOT
	int i, j;
#endif
	int n;
	winid win;
	struct obj *curr, *last;
#ifdef SORTLOOT
	struct obj **oarray;
#endif
	char *pack;
	anything any;
	boolean printed_type_name;

	*pick_list = (menu_item *) 0;
	if (!olist) return 0;

	/* count the number of items allowed */
	for (n = 0, last = 0, curr = olist; curr; curr = FOLLOW(curr, qflags))
	    if ((*allow)(curr, qflags)) {
		last = curr;
		n++;
	    }

	if (n == 0)	/* nothing to pick here */
	    return (qflags & SIGNAL_NOMENU) ? -1 : 0;

	if (n == 1 && (qflags & AUTOSELECT_SINGLE)) {
	    *pick_list = (menu_item *) alloc(sizeof(menu_item));
	    (*pick_list)->item.a_obj = last;
	    (*pick_list)->count = last->quan;
	    return 1;
	}

#ifdef SORTLOOT
	/* Make a temporary array to store the objects sorted */
	oarray = (struct obj **)alloc(n*sizeof(struct obj*));

	/* Add objects to the array */
	i = 0;
	for (curr = olist; curr; curr = FOLLOW(curr, qflags)) {
	  if ((*allow)(curr, qflags)) {
	    if (iflags.sortloot == 'f' ||
		(iflags.sortloot == 'l' && !(qflags & USE_INVLET)))
	      {
		/* Insert object at correct index */
		for (j = i; j; j--)
		  {
		    if (sortloot_cmp(curr, oarray[j-1])>0) break;
		    oarray[j] = oarray[j-1];
		  }
		oarray[j] = curr;
		i++;
	      } else {
		/* Just add it to the array */
		oarray[i++] = curr;
	      }
	  }
	}
#endif /* SORTLOOT */

	win = create_nhwindow(NHW_MENU);
	start_menu(win);
	any.a_obj = (struct obj *) 0;

	/*
	 * Run through the list and add the objects to the menu.  If
	 * INVORDER_SORT is set, we'll run through the list once for
	 * each type so we can group them.  The allow function will only
	 * be called once per object in the list.
	 */
	pack = flags.inv_order;
	do {
	    printed_type_name = FALSE;
#ifdef SORTLOOT
	    for (i = 0; i < n; i++) {
		curr = oarray[i];
#else /* SORTLOOT */
	    for (curr = olist; curr; curr = FOLLOW(curr, qflags)) {
#endif /* SORTLOOT */
		if ((qflags & FEEL_COCKATRICE) && curr->otyp == CORPSE &&
		     will_feel_cockatrice(curr, FALSE)) {
			destroy_nhwindow(win);	/* stop the menu and revert */
			(void) look_here(0, FALSE);
			return 0;
		}
		if ((!(qflags & INVORDER_SORT) || curr->oclass == *pack)
							&& (*allow)(curr, qflags)) {

		    /* if sorting, print type name (once only) */
		    if (qflags & INVORDER_SORT && !printed_type_name) {
				any.a_obj = (struct obj *) 0;
				add_menu(win, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
						Hallucination ? rand_class_name() : let_to_name(*pack, FALSE, iflags.show_obj_sym), MENU_UNSELECTED);
				printed_type_name = TRUE;
		    }

		    any.a_obj = curr;
		    add_menu(win, obj_to_glyph(curr), &any,
			    qflags & USE_INVLET ? curr->invlet : 0,
			    def_oc_syms[(int)objects[curr->otyp].oc_class],
			    ATR_NONE, Hallucination ? an(rndobjnam()) : doname_with_price(curr), MENU_UNSELECTED);
		}
	    }
	    pack++;
	} while (qflags & INVORDER_SORT && *pack);

#ifdef SORTLOOT
	free(oarray);
#endif
	end_menu(win, qstr);
	n = select_menu(win, how, pick_list);
	destroy_nhwindow(win);

	if (n > 0) {
	    menu_item *mi;
	    int i;

	    /* fix up counts:  -1 means no count used => pick all */
	    for (i = 0, mi = *pick_list; i < n; i++, mi++)
		if (mi->count == -1L || mi->count > mi->item.a_obj->quan)
		    mi->count = mi->item.a_obj->quan;
	} else if (n < 0) {
	    n = (qflags & SIGNAL_ESCAPE) ? -2 : 0;
	}
	return n;
}

/*
 * allow menu-based category (class) selection (for Drop,take off etc.)
 *
 */
int
query_category(qstr, olist, qflags, pick_list, how)
const char *qstr;		/* query string */
struct obj *olist;		/* the list to pick from */
int qflags;			/* behaviour modification flags */
menu_item **pick_list;		/* return list of items picked */
int how;			/* type of query */
{
	int n;
	winid win;
	struct obj *curr;
	char *pack;
	anything any;
	boolean collected_type_name;
	char invlet;
	int ccount;
	boolean do_unpaid = FALSE;
	boolean do_blessed = FALSE, do_cursed = FALSE, do_uncursed = FALSE,
	    do_buc_unknown = FALSE;
	int num_buc_types = 0;

	*pick_list = (menu_item *) 0;
	if (!olist) return 0;
	if ((qflags & UNPAID_TYPES) && count_unpaid(olist)) do_unpaid = TRUE;
	if ((qflags & BUC_BLESSED) && count_buc(olist, BUC_BLESSED)) {
	    do_blessed = TRUE;
	    num_buc_types++;
	}
	if ((qflags & BUC_CURSED) && count_buc(olist, BUC_CURSED)) {
	    do_cursed = TRUE;
	    num_buc_types++;
	}
	if ((qflags & BUC_UNCURSED) && count_buc(olist, BUC_UNCURSED)) {
	    do_uncursed = TRUE;
	    num_buc_types++;
	}
	if ((qflags & BUC_UNKNOWN) && count_buc(olist, BUC_UNKNOWN)) {
	    do_buc_unknown = TRUE;
	    num_buc_types++;
	}

	ccount = count_categories(olist, qflags);
	/* no point in actually showing a menu for a single category */
	if (ccount == 1 && !do_unpaid && num_buc_types <= 1 && !(qflags & BILLED_TYPES)) {
	    for (curr = olist; curr; curr = FOLLOW(curr, qflags)) {
		if ((qflags & WORN_TYPES) &&
		    !(curr->owornmask & (W_ARMOR|W_RING|W_AMUL|W_BELT|W_TOOL|W_WEP|W_SWAPWEP|W_QUIVER)))
		    continue;
		break;
	    }
	    if (curr) {
		*pick_list = (menu_item *) alloc(sizeof(menu_item));
		(*pick_list)->item.a_int = curr->oclass;
		return 1;
	    } else {
#ifdef DEBUG
		impossible("query_category: no single object match");
#endif
	    }
	    return 0;
	}

	win = create_nhwindow(NHW_MENU);
	start_menu(win);
	pack = flags.inv_order;
	if ((qflags & ALL_TYPES) && (ccount > 1)) {
		invlet = 'a';
		any.a_void = 0;
		any.a_int = ALL_TYPES_SELECTED;
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
		       (qflags & WORN_TYPES) ? "All worn types" : "All types",
			MENU_UNSELECTED);
		invlet = 'b';
	} else
		invlet = 'a';
	do {
	    collected_type_name = FALSE;
	    for (curr = olist; curr; curr = FOLLOW(curr, qflags)) {
		if (curr->oclass == *pack) {
		   if ((qflags & WORN_TYPES) &&
		   		!(curr->owornmask & (W_ARMOR | W_RING | W_AMUL | W_BELT | W_TOOL |
		    	W_WEP | W_SWAPWEP | W_QUIVER)))
			 continue;
		   if (!collected_type_name) {
			any.a_void = 0;
			any.a_int = curr->oclass;
			add_menu(win, NO_GLYPH, &any, invlet++,
				def_oc_syms[(int)objects[curr->otyp].oc_class],
				 ATR_NONE, let_to_name(*pack, FALSE, iflags.show_obj_sym),
				MENU_UNSELECTED);
			collected_type_name = TRUE;
		   }
		}
	    }
	    pack++;
	    if (invlet >= 'u') {
		impossible("query_category: too many categories");
		destroy_nhwindow(win);
		return 0;
	    }
	} while (*pack);
	/* unpaid items if there are any */
	if (do_unpaid) {
		invlet = 'u';
		any.a_void = 0;
		any.a_int = 'u';
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
			"Unpaid items", MENU_UNSELECTED);
	}
	/* billed items: checked by caller, so always include if BILLED_TYPES */
	if (qflags & BILLED_TYPES) {
		invlet = 'x';
		any.a_void = 0;
		any.a_int = 'x';
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
			 "Unpaid items already used up", MENU_UNSELECTED);
	}
	if (qflags & CHOOSE_ALL) {
		invlet = 'A';
		any.a_void = 0;
		any.a_int = 'A';
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
			(qflags & WORN_TYPES) ?
			"Auto-select every item being worn" :
			"Auto-select every item", MENU_UNSELECTED);
	}
	/* items with b/u/c/unknown if there are any */
	if (do_blessed) {
		invlet = 'B';
		any.a_void = 0;
		any.a_int = 'B';
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
			"Items known to be Blessed", MENU_UNSELECTED);
	}
	if (do_cursed) {
		invlet = 'C';
		any.a_void = 0;
		any.a_int = 'C';
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
			"Items known to be Cursed", MENU_UNSELECTED);
	}
	if (do_uncursed) {
		invlet = 'U';
		any.a_void = 0;
		any.a_int = 'U';
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
			"Items known to be Uncursed", MENU_UNSELECTED);
	}
	if (do_buc_unknown) {
		invlet = 'X';
		any.a_void = 0;
		any.a_int = 'X';
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
			"Items of unknown B/C/U status",
			MENU_UNSELECTED);
	}
	end_menu(win, qstr);
	n = select_menu(win, how, pick_list);
	destroy_nhwindow(win);
	if (n < 0)
	    n = 0;	/* caller's don't expect -1 */
	return n;
}

STATIC_OVL int
count_categories(olist, qflags)
struct obj *olist;
int qflags;
{
	char *pack;
	boolean counted_category;
	int ccount = 0;
	struct obj *curr;

	pack = flags.inv_order;
	do {
	    counted_category = FALSE;
	    for (curr = olist; curr; curr = FOLLOW(curr, qflags)) {
		if (curr->oclass == *pack) {
		   if ((qflags & WORN_TYPES) &&
		    	!(curr->owornmask & (W_ARMOR | W_RING | W_AMUL | W_BELT | W_TOOL |
		    	W_WEP | W_SWAPWEP | W_QUIVER)))
			 continue;
		   if (!counted_category) {
			ccount++;
			counted_category = TRUE;
		   }
		}
	    }
	    pack++;
	} while (*pack);
	return ccount;
}

/* could we carry `obj'? if not, could we carry some of it/them? */
STATIC_OVL long
carry_count(obj, container, count, telekinesis, wt_before, wt_after)
struct obj *obj, *container;	/* object to pick up, bag it's coming out of */
long count;
boolean telekinesis;
int *wt_before, *wt_after;
{
    boolean adjust_wt = container && carried(container) && !cobj_is_magic_chest(container),
	    is_gold = obj->oclass == COIN_CLASS;
    int wt, iw, ow, oow;
    long qq, savequan;
#ifdef GOLDOBJ
    long umoney = money_cnt(invent);
#endif
    unsigned saveowt;
    const char *verb, *prefx1, *prefx2, *suffx;
    char obj_nambuf[BUFSZ], where[BUFSZ];

    savequan = obj->quan;
    saveowt = obj->owt;

    iw = max_capacity();

    if (count != savequan) {
	obj->quan = count;
	obj->owt = (unsigned)weight(obj);
    }
    wt = iw + (int)obj->owt;
    if (adjust_wt)
	wt -= (container->otyp == BAG_OF_HOLDING) ?
		(int)DELTA_CWT(container, obj) : (int)obj->owt;
#ifndef GOLDOBJ
    if (is_gold)	/* merged gold might affect cumulative weight */
	wt -= (GOLD_WT(u.ugold) + GOLD_WT(count) - GOLD_WT(u.ugold + count));
#else
    /* This will go with silver+copper & new gold weight */
    if (is_gold)	/* merged gold might affect cumulative weight */
	wt -= (GOLD_WT(umoney) + GOLD_WT(count) - GOLD_WT(umoney + count));
#endif
    if (count != savequan) {
	obj->quan = savequan;
	obj->owt = saveowt;
    }
    *wt_before = iw;
    *wt_after  = wt;

    /* special case the iron ball here, it actually improves carrycap */
    if (wt < 0 || obj->oartifact == ART_IRON_BALL_OF_LEVITATION)
	return count;

    /* see how many we can lift */
    if (is_gold) {
#ifndef GOLDOBJ
	iw -= (int)GOLD_WT(u.ugold);
	if (!adjust_wt) {
	    qq = GOLD_CAPACITY((long)iw, u.ugold);
	} else {
	    oow = 0;
	    qq = 50L - (u.ugold % 100L) - 1L;
#else
	iw -= (int)GOLD_WT(umoney);
	if (!adjust_wt) {
	    qq = GOLD_CAPACITY((long)iw, umoney);
	} else {
	    oow = 0;
	    qq = 50L - (umoney % 100L) - 1L;
#endif
	    if (qq < 0L) qq += 100L;
	    for ( ; qq <= count; qq += 100L) {
		obj->quan = qq;
		obj->owt = (unsigned)GOLD_WT(qq);
#ifndef GOLDOBJ
		ow = (int)GOLD_WT(u.ugold + qq);
#else
		ow = (int)GOLD_WT(umoney + qq);
#endif
		ow -= (container->otyp == BAG_OF_HOLDING) ?
			(int)DELTA_CWT(container, obj) : (int)obj->owt;
		if (iw + ow >= 0) break;
		oow = ow;
	    }
	    iw -= oow;
	    qq -= 100L;
	}
	if (qq < 0L) qq = 0L;
	else if (qq > count) qq = count;
#ifndef GOLDOBJ
	wt = iw + (int)GOLD_WT(u.ugold + qq);
#else
	wt = iw + (int)GOLD_WT(umoney + qq);
#endif
    } else if (count > 1 || count < obj->quan) {
	/*
	 * Ugh. Calc num to lift by changing the quan of of the
	 * object and calling weight.
	 *
	 * This works for containers only because containers
	 * don't merge.		-dean
	 */
	for (qq = 1L; qq <= count; qq++) {
	    obj->quan = qq;
	    obj->owt = (unsigned)(ow = weight(obj));
	    if (adjust_wt)
		ow -= (container->otyp == BAG_OF_HOLDING) ?
			(int)DELTA_CWT(container, obj) : (int)obj->owt;
	    if (iw + ow >= 0)
		break;
	    wt = iw + ow;
	}
	--qq;
    } else {
	/* there's only one, and we can't lift it */
	qq = 0L;
    }
    obj->quan = savequan;
    obj->owt = saveowt;

    if (qq < count) {
	/* some message will be given */
	Strcpy(obj_nambuf, doname(obj));
	if (container) {
	    Sprintf(where, "in %s", the(xname(container)));
	    verb = "carry";
	} else {
	    Strcpy(where, "lying here");
	    verb = telekinesis ? "acquire" : "lift";
	}
    } else {
	/* lint supppression */
	*obj_nambuf = *where = '\0';
	verb = "";
    }
    /* we can carry qq of them */
    if (qq > 0) {
	if (qq < count)
	    You("can only %s %s of the %s %s.",
		verb, (qq == 1L) ? "one" : "some", obj_nambuf, where);
	*wt_after = wt;
	return qq;
    }

    if (!container) Strcpy(where, "here");  /* slightly shorter form */
#ifndef GOLDOBJ
    if (invent || u.ugold) {
#else
    if (invent || umoney) {
#endif
	prefx1 = "you cannot ";
	prefx2 = "";
	suffx  = " any more";
    } else {
	prefx1 = (obj->quan == 1L) ? "it " : "even one ";
	prefx2 = "is too heavy for you to ";
	suffx  = "";
    }
    There("%s %s %s, but %s%s%s%s.",
	  otense(obj, "are"), obj_nambuf, where,
	  prefx1, prefx2, verb, suffx);

 /* *wt_after = iw; */
    return 0L;
}

/* determine whether character is able and player is willing to carry `obj' */
STATIC_OVL
int 
lift_object(obj, container, cnt_p, telekinesis)
struct obj *obj, *container;	/* object to pick up, bag it's coming out of */
long *cnt_p;
boolean telekinesis;
{
    int result, old_wt, new_wt, prev_encumbr, next_encumbr;

    if (is_boulder(obj) && In_sokoban(&u.uz)) {
	You("cannot get your %s around this %s.",
			body_part(HAND), xname(obj));
	return -1;
    }
    if (obj->otyp == LOADSTONE
		 || (obj->o_id == u.uentangled_oid)
	     || (is_boulder(obj) && (throws_rocks(youracedata) || (u.sealsActive&SEAL_YMIR))))
	return 1;		/* lift regardless of current situation */

    *cnt_p = carry_count(obj, container, *cnt_p, telekinesis, &old_wt, &new_wt);
    if (*cnt_p < 1L) {
	result = -1;	/* nothing lifted */
#ifndef GOLDOBJ
    } else if (obj->oclass != COIN_CLASS && inv_cnt() >= 52 &&
		!merge_choice(invent, obj)) {
#else
    } else if (inv_cnt() >= 52 && !merge_choice(invent, obj)) {
#endif
	Your("knapsack cannot accommodate any more items.");
	result = -1;	/* nothing lifted */
    } else {
	result = 1;
	prev_encumbr = near_capacity();
	if (prev_encumbr < flags.pickup_burden)
		prev_encumbr = flags.pickup_burden;
	next_encumbr = calc_capacity(new_wt - old_wt);
	/* Special case iron ball because in principle one can always pick it up */
	if (next_encumbr > prev_encumbr && obj->oartifact != ART_IRON_BALL_OF_LEVITATION) {
	    if (telekinesis) {
		result = 0;	/* don't lift */
	    } else {
		char qbuf[BUFSZ];
		long savequan = obj->quan;

		obj->quan = *cnt_p;
		Strcpy(qbuf,
			(next_encumbr > HVY_ENCUMBER) ? overloadmsg :
			(next_encumbr > MOD_ENCUMBER) ? nearloadmsg :
			moderateloadmsg);
		Sprintf(eos(qbuf), " %s. Continue?",
			safe_qbuf(qbuf, sizeof(" . Continue?"),
				doname(obj), an(simple_typename(obj->otyp)), "something"));
		obj->quan = savequan;
		switch (ynq(qbuf)) {
		case 'q':  result = -1; break;
		case 'n':  result =  0; break;
		default:   break;	/* 'y' => result == 1 */
		}
		clear_nhwindow(WIN_MESSAGE);
	    }
	}
    }

    if (obj->otyp == SCR_SCARE_MONSTER && result <= 0 && !container)
	obj->spe = 0;
    return result;
}

/* To prevent qbuf overflow in prompts use planA only
 * if it fits, or planB if PlanA doesn't fit,
 * finally using the fallback as a last resort.
 * last_restort is expected to be very short.
 */
const char *
safe_qbuf(qbuf, padlength, planA, planB, last_resort)
const char *qbuf, *planA, *planB, *last_resort;
unsigned padlength;
{
	/* convert size_t (or int for ancient systems) to ordinary unsigned */
	unsigned len_qbuf = (unsigned)strlen(qbuf),
	         len_planA = (unsigned)strlen(planA),
	         len_planB = (unsigned)strlen(planB),
	         len_lastR = (unsigned)strlen(last_resort);
	unsigned textleft = QBUFSZ - (len_qbuf + padlength);

	if (len_lastR >= textleft) {
	    impossible("safe_qbuf: last_resort too large at %u characters.",
		       len_lastR);
	    return "";
	}
	return (len_planA < textleft) ? planA :
		    (len_planB < textleft) ? planB : last_resort;
}

/*
 * Pick up <count> of obj from the ground and add it to the hero's inventory.
 * Returns -1 if caller should break out of its loop, 0 if nothing picked
 * up, 1 if otherwise.
 */
int
pickup_object(obj, count, telekinesis)
struct obj *obj;
long count;
boolean telekinesis;	/* not picking it up directly by hand */
{
	int res, nearload;
#ifndef GOLDOBJ
	const char *where = (obj->ox == u.ux && obj->oy == u.uy) ?
			    "here" : "there";
#endif

	if (obj->quan < count) {
	    impossible("pickup_object: count %ld > quan %ld?",
		count, obj->quan);
	    return 0;
	}

	/* In case of auto-pickup, where we haven't had a chance
	   to look at it yet; affects docall(SCR_SCARE_MONSTER). */
	if (!Blind)
#ifdef INVISIBLE_OBJECTS
		if (!obj->oinvis || See_invisible(obj->ox,obj->oy))
#endif
		obj->dknown = 1;

	// if(obj->shopOwned && !(obj->unpaid)){ /* shop stock is outside shop */
		// if(obj->sknown && !(obj->ostolen) ) obj->sknown = FALSE; /*don't automatically know that you found a stolen item.*/
		// obj->ostolen = TRUE; /* object was apparently stolen by someone (not necessarily the player) */
	// }
	if (obj == uchain) {    /* do not pick up attached chain */
	    return 0;
	} else if (obj->oartifact && !touch_artifact(obj, &youmonst, FALSE)) {
	    return 0;
#ifndef GOLDOBJ
	} else if (obj->oclass == COIN_CLASS) {
	    /* Special consideration for gold pieces... */
	    long iw = (long)max_capacity() - GOLD_WT(u.ugold);
	    long gold_capacity = GOLD_CAPACITY(iw, u.ugold);

	    if (gold_capacity <= 0L) {
		pline(
	       "There %s %ld gold piece%s %s, but you cannot carry any more.",
		      otense(obj, "are"),
		      obj->quan, plur(obj->quan), where);
		return 0;
	    } else if (gold_capacity < count) {
		You("can only %s %s of the %ld gold pieces lying %s.",
		    telekinesis ? "acquire" : "carry",
		    gold_capacity == 1L ? "one" : "some", obj->quan, where);
		pline("%s %ld gold piece%s.",
		    nearloadmsg, gold_capacity, plur(gold_capacity));
		costly_gold(obj->ox, obj->oy, gold_capacity);
		u.ugold += gold_capacity;
		obj->quan -= gold_capacity;
	    } else {
		if ((nearload = calc_capacity(GOLD_WT(count))) != 0)
		    pline("%s %ld gold piece%s.",
			  nearload < MOD_ENCUMBER ?
			  moderateloadmsg : nearloadmsg,
			  count, plur(count));
		else
		    prinv((char *) 0, obj, count);
		costly_gold(obj->ox, obj->oy, count);
		u.ugold += count;
		if (count == obj->quan)
		    delobj(obj);
		else
		    obj->quan -= count;
	    }
	    flags.botl = 1;
	    if (flags.run) nomul(0, NULL);
	    return 1;
#endif
	} else if (obj->otyp == CORPSE) {
	    if ( (touch_petrifies(&mons[obj->corpsenm])) && !uarmg
				&& !Stone_resistance && !telekinesis) {
		if (poly_when_stoned(youracedata) && polymon(PM_STONE_GOLEM))
		    display_nhwindow(WIN_MESSAGE, FALSE);
		else {
			char kbuf[BUFSZ];

			Strcpy(kbuf, an(corpse_xname(obj, TRUE)));
			pline("Touching %s is a fatal mistake.", kbuf);
			instapetrify(kbuf);
		    return -1;
		}
	    } else if (is_rider(&mons[obj->corpsenm])) {
		pline("At your %s, the corpse suddenly moves...",
			telekinesis ? "attempted acquisition" : "touch");
		(void) revive_corpse(obj, REVIVE_MONSTER);
		exercise(A_WIS, FALSE);
		return -1;
	    }
	} else  if (obj->otyp == SCR_SCARE_MONSTER) {
	    if (obj->blessed) obj->blessed = 0;
	    else if (!obj->spe && !obj->cursed) obj->spe = 1;
	    else {
		pline_The("scroll%s %s to dust as you %s %s up.",
			plur(obj->quan), otense(obj, "turn"),
			telekinesis ? "raise" : "pick",
			(obj->quan == 1L) ? "it" : "them");
		if (!(objects[SCR_SCARE_MONSTER].oc_name_known) &&
				    !(objects[SCR_SCARE_MONSTER].oc_uname))
		    docall(obj);
		useupf(obj, obj->quan);
		return 1;	/* tried to pick something up and failed, but
				   don't want to terminate pickup loop yet   */
	    }
	} else if (obj->otyp == MAGIC_CHEST && obj->obolted) {
		pline_The("chest is bolted down!");
		return 1;	/* tried to pick something up and failed, but
				   don't want to terminate pickup loop yet   */
	}

	if ((res = lift_object(obj, (struct obj *)0, &count, telekinesis)) <= 0)
	    return res;

#ifdef GOLDOBJ
        /* Whats left of the special case for gold :-) */
	if (obj->oclass == COIN_CLASS) flags.botl = 1;
#endif
	if (obj->quan != count && obj->otyp != LOADSTONE)
	    obj = splitobj(obj, count);

	obj = pick_obj(obj);
	obj->was_thrown = 0;

	if (uwep && uwep == obj) mrg_to_wielded = TRUE;
	nearload = near_capacity();
	prinv(nearload == SLT_ENCUMBER ? moderateloadmsg : (char *) 0,
	      obj, count);
	mrg_to_wielded = FALSE;
	return 1;
}

/*
 * Do the actual work of picking otmp from the floor or monster's interior
 * and putting it in the hero's inventory.  Take care of billing.  Return a
 * pointer to the object where otmp ends up.  This may be different
 * from otmp because of merging.
 *
 * Gold never reaches this routine unless GOLDOBJ is defined.
 */
struct obj *
pick_obj(otmp)
struct obj *otmp;
{
	obj_extract_and_unequip_self(otmp);
	if (!u.uswallow && otmp != uball && costly_spot(otmp->ox, otmp->oy)) {
	    char saveushops[5], fakeshop[2];

	    /* addtobill cares about your location rather than the object's;
	       usually they'll be the same, but not when using telekinesis
	       (if ever implemented) or a grappling hook */
	    Strcpy(saveushops, u.ushops);
	    fakeshop[0] = *in_rooms(otmp->ox, otmp->oy, SHOPBASE);
	    fakeshop[1] = '\0';
	    Strcpy(u.ushops, fakeshop);
	    /* sets obj->unpaid if necessary */
	    addtobill(otmp, TRUE, FALSE, FALSE);
		if(otmp->shopOwned && !(otmp->unpaid)){ /* shop stock is outside shop */
			if(otmp->sknown && !(otmp->ostolen) ) otmp->sknown = FALSE; /*don't automatically know that you found a stolen item.*/
			otmp->ostolen = TRUE; /* object was apparently stolen by someone (not necessarily the player) */
		}
	    Strcpy(u.ushops, saveushops);
	    /* if you're outside the shop, make shk notice */
	    if (!index(u.ushops, *fakeshop)) {
			remote_burglary(otmp);
		}
	}
	if (otmp->no_charge)	/* only applies to objects outside invent */
	    otmp->no_charge = 0;
	newsym(otmp->ox, otmp->oy);
	return addinv(otmp);	/* might merge it with other objects */
}

/*
 * prints a message if encumbrance changed since the last check and
 * returns the new encumbrance value (from near_capacity()).
 */
int
encumber_msg()
{
    static int oldcap = UNENCUMBERED;
    int newcap = near_capacity();

    if(oldcap < newcap) {
	if(oldcap == UNENCUMBERED && u.uinwater && !u.usubwater) drown();
	switch(newcap) {
	case 1: Your("movements are slowed slightly because of your load.");
		break;
	case 2: You("rebalance your load.  Movement is difficult.");
		break;
	case 3: You("%s under your heavy load.  Movement is very hard.",
		    stagger(&youmonst, "stagger"));
		break;
	default: You("%s move a handspan with this load!",
		     newcap == 4 ? "can barely" : "can't even");
		break;
	}
	flags.botl = 1;
    } else if(oldcap > newcap) {
	switch(newcap) {
	case 0: Your("movements are now unencumbered.");
		break;
	case 1: Your("movements are only slowed slightly by your load.");
		break;
	case 2: You("rebalance your load.  Movement is still difficult.");
		break;
	case 3: You("%s under your load.  Movement is still very hard.",
		    stagger(&youmonst, "stagger"));
		break;
	}
	flags.botl = 1;
    }

    oldcap = newcap;
    return (newcap);
}

/* Is there a container at x,y. Optional: return count of containers at x,y */
STATIC_OVL int
container_at(x, y, countem)
int x,y;
boolean countem;
{
	struct obj *cobj, *nobj;
	int container_count = 0;
	
	for(cobj = level.objects[x][y]; cobj; cobj = nobj) {
		nobj = cobj->nexthere;
		if(Is_container(cobj) || 
			is_gemable_lightsaber(cobj)
			|| cobj->otyp == MASS_SHADOW_PISTOL
			|| cobj->otyp == DEMON_CLAW
		) {
			container_count++;
			if (!countem) break;
		}
	}
	return container_count;
}

STATIC_OVL boolean
able_to_loot(x, y, looting)
int x, y;
boolean looting;
{
	const char *verb = looting ? "loot" : "tip";
	if (!can_reach_floor()) {
#ifdef STEED
		if (u.usteed && P_SKILL(P_RIDING) < P_BASIC)
			rider_cant_reach(); /* not skilled enough to reach */
		else
#endif
			You("cannot reach the %s.", surface(x, y));
		return FALSE;
	} else if ((is_pool(x, y, FALSE) && (looting || !Underwater)) || is_lava(x, y)) {
		/* at present, can't loot in water even when Underwater */
		You("cannot %s things that are deep in the %s.", verb,
		    is_lava(x, y) ? "lava" : "water");
		return FALSE;
	} else if (looting && nolimbs(youracedata)) {
		pline("Without limbs, you cannot %s anything.", verb);
		return FALSE;
	} else if (looting && !freehand()) {
		pline("Without a free %s, you cannot %s anything.",
			body_part(HAND), verb);
		return FALSE;
	}
	return TRUE;
}

STATIC_OVL boolean
mon_beside(x,y)
int x, y;
{
	int i,j,nx,ny;
	for(i = -1; i <= 1; i++)
	    for(j = -1; j <= 1; j++) {
	    	nx = x + i;
	    	ny = y + j;
		if(isok(nx, ny) && MON_AT(nx, ny))
			return TRUE;
	    }
	return FALSE;
}

int
do_loot_cont(cobj, noit)
struct obj *cobj;
boolean noit;
{
    if (!cobj) return 0;

	if(is_gemable_lightsaber(cobj)){
		You("carefully open %s...",the(xname(cobj)));
		return use_lightsaber(cobj);
	} else if(cobj->otyp == MASS_SHADOW_PISTOL){
		You("carefully open %s...",the(xname(cobj)));
		return use_massblaster(cobj);
	} else if(cobj->otyp == DEMON_CLAW){
		You("carefully reach into %s...",the(xname(cobj)));
		return use_demon_claw(cobj);
	}
    if (cobj->olocked) {
		pline("Hmmm, %s seems to be locked.", noit ? the(xname(cobj)) : "it");
		struct obj *unlocktool;
		if (flags.autounlock && (unlocktool = autokey(TRUE)) != 0) {
			if (cobj->otyp == BOX || cobj->otyp == CHEST) {
				/* pass ox and oy to avoid direction prompt */
				return (pick_lock(unlocktool, cobj->ox, cobj->oy, cobj) != 0);
			}
		} else { return MOVE_CANCELLED; }
    }
    if (cobj->otyp == BAG_OF_TRICKS) {
	int tmp;
	You("carefully open the bag...");
	pline("It develops a huge set of teeth and bites you!");
	tmp = rnd(10);
	tmp = reduce_dmg(&youmonst,tmp,TRUE,FALSE);
	losehp(tmp, "carnivorous bag", KILLED_BY_AN);
	makeknown(BAG_OF_TRICKS);
	return MOVE_STANDARD;
    }

	if(cobj->spe == 5){
	    pline("%s is wrapped around with a rope motif, and the face seems to be screaming.", The(xname(cobj)));
		if(yn("Are you sure you want to open it?") != 'y'){
			return MOVE_CANCELLED;
		}
	}
    You("carefully open %s...", the(xname(cobj)));
    return use_container(cobj, 0);
}


int
doloot()	/* loot a container on the floor or loot saddle from mon. */
{
    register struct obj *cobj, *nobj;
    register int c = -1;
    int timepassed = MOVE_CANCELLED;
    coord cc;
    boolean underfoot = TRUE;
    const char *dont_find_anything = "don't find anything";
    struct monst *mtmp;
    char qbuf[BUFSZ];
    int prev_inquiry = 0;
    boolean prev_loot = FALSE;
	boolean any = FALSE;
    int num_cont = 0;

    if (check_capacity((char *)0)) {
	/* "Can't do that while carrying so much stuff." */
	return MOVE_CANCELLED;
    }
    if (nolimbs(youracedata)) {
	You("have no limbs!");	/* not `body_part(HAND)' */
	return MOVE_CANCELLED;
    }
    cc.x = u.ux; cc.y = u.uy;

lootcont:

	if (container_at(cc.x, cc.y, FALSE)) {

		if (!able_to_loot(cc.x, cc.y, TRUE)) return MOVE_CANCELLED;

		for (cobj = level.objects[cc.x][cc.y]; cobj; cobj = cobj->nexthere) {
			if (Is_container(cobj)
				|| cobj->otyp == MASS_SHADOW_PISTOL
				|| is_gemable_lightsaber(cobj)
				|| (cobj->otyp == DEMON_CLAW)
			) num_cont++;
		}

		if (num_cont > 1) {
			/* use a menu to loot many containers */
			int n, i;

			winid win;
			anything any;
			menu_item *pick_list;

			timepassed = MOVE_CANCELLED;

			any.a_void = 0;
			win = create_nhwindow(NHW_MENU);
			start_menu(win);

			for (cobj = level.objects[cc.x][cc.y]; cobj; cobj = cobj->nexthere) {
			if (Is_container(cobj) || 
				cobj->otyp == MASS_SHADOW_PISTOL
				|| is_gemable_lightsaber(cobj)
				|| (cobj->otyp == DEMON_CLAW)
			) {
				any.a_obj = cobj;
				add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, doname(cobj),
					 MENU_UNSELECTED);
			}
			}
			end_menu(win, "Loot which containers?");
			n = select_menu(win, PICK_ANY, &pick_list);
			destroy_nhwindow(win);

			if (n > 0) {
			for (i = 0; i < n; i++) {
				timepassed |= do_loot_cont(pick_list[i].item.a_obj, TRUE);
				if (multi < 0) {/* chest trap, stop looting */
					free((genericptr_t) pick_list);
					return MOVE_STANDARD;
				}
			}
			}
			if (pick_list)
			free((genericptr_t) pick_list);
			if (n != 0) c = 'y';
		} else {
			for (cobj = level.objects[cc.x][cc.y]; cobj; cobj = nobj) {
				nobj = cobj->nexthere;

				if (Is_container(cobj)) {
					Sprintf(qbuf, "There is %s here, loot it?",
						safe_qbuf("", sizeof("There is  here, loot it?"),
							 doname(cobj), an(simple_typename(cobj->otyp)), "a container"));
					c = ynq(qbuf);
					if (c == 'q') return (timepassed==MOVE_CANCELLED ? MOVE_CANCELLED : (timepassed&~MOVE_CANCELLED));
					if (c == 'n') continue;
					any = TRUE;

					if (cobj->olocked) {
						struct obj *unlocktool;
						pline("Hmmm, it seems to be locked.");
						if (flags.autounlock && (unlocktool = autokey(TRUE)) != 0) {
							if (cobj->otyp == BOX || cobj->otyp == CHEST) {
								/* pass ox and oy to avoid direction prompt */
								return (pick_lock(unlocktool, cobj->ox, cobj->oy, cobj) != 0);
							}
						} else { continue; }
					}
					if (cobj->otyp == BAG_OF_TRICKS) {
						int tmp;
						You("carefully open the bag...");
						pline("It develops a huge set of teeth and bites you!");
						tmp = rnd(10);
						tmp = reduce_dmg(&youmonst,tmp,TRUE,FALSE);
						losehp(tmp, "carnivorous bag", KILLED_BY_AN);
						makeknown(BAG_OF_TRICKS);
						timepassed = MOVE_STANDARD;
						continue;
					}

					if(cobj->spe == 5){
						pline("%s is wrapped around with a rope motif, and the face seems to be screaming.", The(xname(cobj)));
						if(yn("Are you sure you want to open it?") != 'y'){
							return MOVE_CANCELLED;
						}
					}
					You("carefully open %s...", the(xname(cobj)));
					timepassed |= use_container(cobj, 0);
					if (multi < 0) return MOVE_STANDARD;		/* chest trap */
			    } else if(is_gemable_lightsaber(cobj)){
					Sprintf(qbuf, "There is %s here, open it?",an(xname(cobj)));
					c = ynq(qbuf);
					if (c == 'q') return (timepassed==MOVE_CANCELLED ? MOVE_CANCELLED : (timepassed&~MOVE_CANCELLED));
					if (c == 'n') continue;
					timepassed |= use_lightsaber(cobj);
					if(timepassed & MOVE_STANDARD) underfoot = TRUE;
				} else if(cobj->otyp == MASS_SHADOW_PISTOL){
					Sprintf(qbuf, "There is %s here, open it?",an(xname(cobj)));
					c = ynq(qbuf);
					if (c == 'q') return (timepassed==MOVE_CANCELLED ? MOVE_CANCELLED : (timepassed&~MOVE_CANCELLED));
					if (c == 'n') continue;
					timepassed |= use_massblaster(cobj);
					if(timepassed & MOVE_STANDARD) underfoot = TRUE;
				} else if(cobj->otyp == DEMON_CLAW){
					Sprintf(qbuf, "There is %s here, exchange coin?",an(xname(cobj)));
					c = ynq(qbuf);
					if (c == 'q') return (timepassed==MOVE_CANCELLED ? MOVE_CANCELLED : (timepassed&~MOVE_CANCELLED));
					if (c == 'n') continue;
					timepassed |= use_demon_claw(cobj);
					if(timepassed & MOVE_STANDARD) underfoot = TRUE;
				}
			}
		}
	}
	if (any) c = 'y';
    else if (Confusion) {
#ifndef GOLDOBJ
	if (u.ugold){
	    long contribution = rnd((int)min(LARGEST_INT,u.ugold));
	    struct obj *goldob = mkgoldobj(contribution);
#else
	struct obj *goldob;
	/* Find a money object to mess with */
	for (goldob = invent; goldob; goldob = goldob->nobj) {
	    if (goldob->oclass == COIN_CLASS) break;
	}
	if (goldob){
	    long contribution = rnd((int)min(LARGEST_INT, goldob->quan));
	    if (contribution < goldob->quan)
		goldob = splitobj(goldob, contribution);
	    freeinv(goldob);
#endif
	    if (IS_THRONE(levl[u.ux][u.uy].typ)){
			struct obj *coffers;
			int pass;
			/* find the original coffers chest, or any chest */
			for (pass = 2; pass > -1; pass -= 2)
				for (coffers = fobj; coffers; coffers = coffers->nobj)
				if (coffers->otyp == CHEST && coffers->spe == pass)
					goto gotit;	/* two level break */
gotit:
			if (coffers) {
			verbalize("Thank you for your contribution to reduce the debt.");
				(void) add_to_container(coffers, goldob);
				coffers->owt = weight(coffers);
			} else {
				struct monst *mon = makemon(courtmon(0),
							u.ux, u.uy, NO_MM_FLAGS);
				if (mon) {
#ifndef GOLDOBJ
					mon->mgold += goldob->quan;
					delobj(goldob);
					pline("The exchequer accepts your contribution.");
				} else {
					dropx(goldob);
				}
			}
	    } else {
			dropx(goldob);
#else
					add_to_minv(mon, goldob);
					pline("The exchequer accepts your contribution.");
				} else {
					dropy(goldob);
				}
			}
	    } else {
			dropy(goldob);
#endif
			pline("Ok, now there is loot here.");
	    }
	}
    } else if (IS_GRAVE(levl[cc.x][cc.y].typ)) {
		You("need to dig up the grave to effectively loot it...");
    }
    /*
     * 3.3.1 introduced directional looting for some things.
     */
    if (c != 'y' && mon_beside(u.ux, u.uy)) {
	if (!get_adjacent_loc("Loot in what direction?", "Invalid loot location",
			u.ux, u.uy, &cc)) return MOVE_CANCELLED;
	if (cc.x == u.ux && cc.y == u.uy) {
	    underfoot = TRUE;
	    if (container_at(cc.x, cc.y, FALSE))
		goto lootcont;
	} else
	    underfoot = FALSE;
	if (u.dz < 0) {
	    You("%s to loot on the %s.", dont_find_anything,
		ceiling(cc.x, cc.y));
	    timepassed = MOVE_STANDARD;
	    return timepassed;
	}
	if(u.dz > 0)
		mtmp = u.usteed;
	else
		mtmp = m_at(cc.x, cc.y);
	if (mtmp) {
		if(mtmp->mtyp == PM_JRT_NETJER && has_template(mtmp, POISON_TEMPLATE) && canseemon(mtmp)){
			struct obj *armor;
			if((armor = which_armor(mtmp, W_ARM)) && arm_blocks_upper_body(armor->otyp))
				/*no message*/;
			else if((armor = which_armor(mtmp, W_ARMU)) && arm_blocks_upper_body(armor->otyp))
				/*no message*/;
			else {
				pline("A broken-off fang is embedded in %s chest. It seems to have pierced %s heart!", s_suffix(mon_nam(mtmp)), mhis(mtmp));
				if(!helpless_still(mtmp) && !TimeStop){
					pline("%s moves too quickly for you to grasp the fang.", Monnam(mtmp));
				}
				else if(yn("Attempt to remove the fang?")=='y'){
					/* Don't really want the solution to be wages of sloth */
					if(TimeStop){
						pline("The flesh around %s wound is too unyielding in your accelerated time frame.", s_suffix(mon_nam(mtmp)));
					}
					else {
						timepassed = you_remove_jrt_fang(mtmp, (struct obj *)0);
					}
				}
			}
		}
		else if (costly_spot(mtmp->mx, mtmp->my)) {
			verbalize("Not in my store!");
			timepassed = MOVE_STANDARD;
		}
		else {
			timepassed = loot_mon(mtmp, &prev_inquiry, &prev_loot);
		}
	}

	/* Preserve pre-3.3.1 behaviour for containers.
	 * Adjust this if-block to allow container looting
	 * from one square away to change that in the future.
	 */
	if (!underfoot) {
	    if (container_at(cc.x, cc.y, FALSE)) {
		if (mtmp) {
		    You_cant("loot anything %sthere with %s in the way.",
			    prev_inquiry ? "else " : "", mon_nam(mtmp));
		    return (timepassed==MOVE_CANCELLED ? MOVE_CANCELLED : (timepassed&~MOVE_CANCELLED));
		} else {
		    You("have to be at a container to loot it.");
		}
	    } else {
		You("%s %sthere to loot.", dont_find_anything,
			(prev_inquiry || prev_loot) ? "else " : "");
		return (timepassed==MOVE_CANCELLED ? MOVE_CANCELLED : (timepassed&~MOVE_CANCELLED));
	    }
	}
    } else if (c != 'y' && c != 'n') {
	You("%s %s to loot.", dont_find_anything,
		    underfoot ? "here" : "there");
    }
    return (timepassed==MOVE_CANCELLED ? MOVE_CANCELLED : (timepassed&~MOVE_CANCELLED));
}

/* loot_mon() returns amount of time passed.
 */
int
loot_mon(mtmp, passed_info, prev_loot)
struct monst *mtmp;
int *passed_info;
boolean *prev_loot;
{
    int c = -1;
    int timepassed = 0;
#ifdef STEED
    struct obj *otmp;
    char qbuf[QBUFSZ];

    /* 3.3.1 introduced the ability to remove saddle from a steed             */
    /* 	*passed_info is set to TRUE if a loot query was given.               */
    /*	*prev_loot is set to TRUE if something was actually acquired in here. */
	if(mtmp 
		&& (mtmp->mtame || (urole.ldrnum == PM_OLD_FORTUNE_TELLER && mtmp->mpeaceful && quest_faction(mtmp)))
	){
	if((otmp = pick_creatures_armor(mtmp, passed_info))){
	long unwornmask;
		if (nolimbs(youracedata)) {
		    You_cant("do that without limbs."); /* not body_part(HAND) */
		    return MOVE_CANCELLED;
		}
		if (otmp->cursed && otmp->owornmask && !is_weldproof_mon(mtmp)) {
		    You("can't. It seems to be stuck to %s.",
			x_monnam(mtmp, ARTICLE_THE, (char *)0,
				SUPPRESS_SADDLE, FALSE));
			    
		    /* the attempt costs you time */
			return MOVE_STANDARD;
		}
		if (otmp->otyp == STATUE && (otmp->corpsenm == PM_PARASITIC_MIND_FLAYER || otmp->corpsenm == PM_PARASITIC_MASTER_MIND_FLAYER)){
		    You("can't. It's stuck in %s face.",
			s_suffix(x_monnam(mtmp, ARTICLE_THE, (char *)0,
				SUPPRESS_SADDLE, FALSE)));
			    
		    /* the attempt costs you time */
			return MOVE_STANDARD;
		}
		obj_extract_self(otmp);
		if ((unwornmask = otmp->owornmask) != 0L) {
		    mtmp->misc_worn_check &= ~unwornmask;
		    if (otmp->owornmask & W_WEP){
				setmnotwielded(mtmp,otmp);
				MON_NOWEP(mtmp);
			}
		    if (otmp->owornmask & W_SWAPWEP){
				setmnotwielded(mtmp,otmp);
				MON_NOSWEP(mtmp);
			}
		    otmp->owornmask = 0L;
		    update_mon_intrinsics(mtmp, otmp, FALSE, FALSE);
		}
		otmp = hold_another_object(otmp, "You drop %s!", doname(otmp),
					(const char *)0);
		timepassed = rnd(3) +  objects[otmp->otyp].oc_delay;
		mtmp->mequipping = timepassed;
		if (prev_loot) *prev_loot = TRUE;
	} else {
		return MOVE_CANCELLED;
	}
    }
#endif	/* STEED */
    /* 3.4.0 introduced the ability to pick things up from within swallower's stomach */
    if (u.uswallow) {
	int count = passed_info ? *passed_info : 0;
	timepassed = pickup(count);
    }
    return timepassed ? MOVE_STANDARD : MOVE_CANCELLED;
}

/* dopetequip() returns amount of time passed.
 */
int
dopetequip()
{
    int c = -1;
    int timepassed = 0;
	long flag;
	boolean unseen;
    coord cc;
    struct obj *otmp;
    char qbuf[QBUFSZ];
	struct monst *mtmp;
	char nambuf[BUFSZ];
	
	if (!get_adjacent_loc("Equip a pet in what direction?", "Invalid location",
		u.ux, u.uy, &cc)) return MOVE_CANCELLED;
	
	mtmp = m_at(cc.x, cc.y);
	
	if(!mtmp || !canspotmon(mtmp)){
		You_cant("find anyone to equip!");
		return MOVE_CANCELLED;
	}
	if(!mtmp->mtame
		&& !(urole.ldrnum == PM_OLD_FORTUNE_TELLER && mtmp->mpeaceful && quest_faction(mtmp))
	){
		pline("%s doesn't trust you enough for that!", Monnam(mtmp));
		return MOVE_CANCELLED;
	}

#ifdef STEED
	if(mtmp == u.usteed){
		You_cant("change the equipment of something you're riding!");
		return MOVE_CANCELLED;
	}
#endif	/* STEED */
	if (nolimbs(youracedata)) {
		You_cant("do that without limbs."); /* not body_part(HAND) */
		return MOVE_CANCELLED;
	}
	if(!freehand()){
		You("have no free %s to dress %s with!", body_part(HAND), mon_nam(mtmp));
		return MOVE_CANCELLED;
	}
	unseen = !canseemon(mtmp);

	/* Get a copy of monster's name before altering its visibility */
	Strcpy(nambuf, See_invisible(mtmp->mx,mtmp->my) ? Monnam(mtmp) : mon_nam(mtmp));
	
	if((otmp = pick_armor_for_creature(mtmp))){
		if(otmp->oclass == AMULET_CLASS){
			flag = W_AMUL;
		} else if(is_shirt(otmp)){
			flag = W_ARMU;
		} else if(is_cloak(otmp)){
			flag = W_ARMC;
		} else if(is_helmet(otmp)){
			flag = W_ARMH;
		} else if(is_shield(otmp)){
			flag = W_ARMS;
		} else if(is_gloves(otmp)){
			flag = W_ARMG;
		} else if(is_boots(otmp)){
			flag = W_ARMF;
		} else if(is_suit(otmp)){
			flag = W_ARM;
		} else if(is_worn_tool(otmp)){
			flag = W_TOOL;
		} else if(is_belt(otmp)){
			flag = W_BELT;
		} else {
			pline("Error: Unknown monster armor type!?");
			return MOVE_CANCELLED;
		}
		if(mtmp->mtyp == PM_HARROWER_OF_ZARIEL
		 && ((flag == W_ARM && arm_blocks_upper_body(otmp->otyp))
			|| flag == W_ARMU
		)){
			You_cant("fit %s on over the thicket of spears stuck through %s chest.", the(xname(otmp)), mhis(mtmp));
			return MOVE_CANCELLED;
		}
		if(otmp->unpaid)  addtobill(otmp, FALSE, FALSE, FALSE);
		You("equip %s with %s.", mon_nam(mtmp), the(xname(otmp)));
		freeinv(otmp);
		mpickobj(mtmp, otmp);
		mtmp->misc_worn_check |= flag;
		otmp->owornmask |= flag;
		update_mon_intrinsics(mtmp, otmp, TRUE, FALSE);
		if(check_oprop(otmp, OPROP_CURS)){
			if (!Blind && canseemon(mtmp))
				pline("%s %s for a moment.",
					  Tobjnam(otmp, "glow"), hcolor(NH_BLACK));
			curse(otmp);
		}
		/* if couldn't see it but now can, or vice versa, */
		if (unseen ^ !canseemon(mtmp)) {
			if (mtmp->minvis && !See_invisible(mtmp->mx,mtmp->my)) {
				pline("Suddenly you cannot see %s.", nambuf);
				makeknown(otmp->otyp);
			} /* else if (!mon->minvis) pline("%s suddenly appears!", Amonnam(mon)); */
		}
		timepassed = rnd(3) +  objects[otmp->otyp].oc_delay;
		if(unseen) timepassed += d(3,4);
		mtmp->mequipping = timepassed;
	} else {
		return MOVE_CANCELLED;
	}
    return timepassed ? MOVE_STANDARD : MOVE_CANCELLED;
}

/*
 * Decide whether an object being placed into a magic bag will cause
 * it to explode.  If the object is a bag itself, check recursively.
 */
STATIC_OVL boolean
mbag_explodes(obj, depthin)
    struct obj *obj;
    int depthin;
{
    /* these won't cause an explosion when they're empty */
    if ((obj->otyp == WAN_CANCELLATION || obj->otyp == BAG_OF_TRICKS) &&
	    obj->spe <= 0)
	return FALSE;

    /* odds: 1/1, 2/2, 3/4, 4/8, 5/16, 6/32, 7/64, 8/128, 9/128, 10/128,... */
	/* Wands of Cancellation now don't cause explosions if nested.  Bags of tricks still do */
    if ((Is_mbag(obj) || (obj->otyp == WAN_CANCELLATION && !depthin)) &&
	(rn2(1 << (depthin > 7 ? 7 : depthin)) <= depthin))
	return TRUE;
    else if (Has_contents(obj)) {
	struct obj *otmp;

	for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
	    if (mbag_explodes(otmp, depthin+1)) return TRUE;
    }
    return FALSE;
}

/* A variable set in use_container(), to be used by the callback routines   */
/* in_container(), and out_container() from askchain() and use_container(). */
static NEARDATA struct obj *current_container;
#define Icebox (current_container->otyp == ICE_BOX)

STATIC_OVL boolean
crystal_skull_overweight(cont, newobj)
struct obj *cont;
struct obj *newobj;
{
	long weight = newobj->owt;
	long weight_lim = 1000;
	if(Is_container(newobj))
		return TRUE;
	for(struct obj *otmp = cont->cobj; otmp; otmp = otmp->nobj){
		weight += objects[otmp->otyp].oc_weight;
	}
	return weight > weight_lim;
}

STATIC_OVL boolean
duplicate_item(cont, newobj)
struct obj *cont;
struct obj *newobj;
{
	for(struct obj *otmp = cont->cobj; otmp; otmp = otmp->nobj){
		if(otmp->otyp == newobj->otyp) return TRUE;
	}
	return FALSE;
}

/* Returns: -1 to stop, 1 item was inserted, 0 item was not inserted. */
STATIC_PTR int
in_container(obj)
register struct obj *obj;
{
	boolean floor_container = !cobj_is_magic_chest(current_container) && !carried(current_container);
	boolean was_unpaid = FALSE;
	char buf[BUFSZ];

	if (!current_container) {
		impossible("<in> no current_container?");
		return 0;
	} else if (obj == uball || obj == uchain) {
		You("must be kidding.");
		return 0;
	} else if (obj == current_container) {
		pline("That would be an interesting topological exercise.");
		return 0;
	} else if ((Is_mbag(obj) || obj->otyp == WAN_CANCELLATION)
		&& objects[obj->otyp].oc_name_known && obj->dknown && current_container->otyp == BAG_OF_HOLDING
	) {
		pline("That combination is a little too explosive.");
		return 0;
	} else if ((!is_magic_obj(obj) || is_asc_obj(obj))
		&& current_container->oartifact == ART_ESSCOOAHLIPBOOURRR
	) {
		pline("The artifact isn't interested in taking %s.", the(xname(obj)));
		return 0;
	} else if (obj->owornmask & (W_ARMOR | W_RING | W_AMUL | W_BELT | W_TOOL)) {
		Norep("You cannot %s %s you are wearing.",
			Icebox ? "refrigerate" : "stash", something);
		return 0;
	} else if ((obj->otyp == LOADSTONE) && obj->cursed) {
		obj->bknown = 1;
	      pline_The("stone%s won't leave your person.", plur(obj->quan));
		return 0;
	} else if ((current_container->otyp == CRYSTAL_SKULL) && (
		crystal_skull_overweight(current_container, obj)
		|| obj->otyp == TREPHINATION_KIT
		|| ensouled_item(obj)
		|| get_ox(obj, OX_ESUM)
		|| duplicate_item(current_container, obj)
		)
	) {
		pline("%s doesn't fit in the skull!", The(xname(obj)));
		return 0;
	} else if (is_asc_obj(obj) || obj->oartifact == ART_ESSCOOAHLIPBOOURRR) {
	/* Prohibit Amulets in containers; if you allow it, monsters can't
	 * steal them.  It also becomes a pain to check to see if someone
	 * has the Amulet.  Ditto for the Candelabrum, the Bell and the Book.
	 * 
	 * Esscoo can suck up BoH and wands of cancellation, causing bag-splosions
	 */
	    pline("%s cannot be confined in such trappings.", The(xname(obj)));
	    return 0;
	} else if (obj->otyp == LEASH && obj->leashmon != 0) {
		pline("%s attached to your pet.", Tobjnam(obj, "are"));
		return 0;
	} else if (obj == uwep) {
		if (welded(obj)) {
			weldmsg(obj);
			return 0;
		}
		setuwep((struct obj *) 0);
		if (uwep) return 0;	/* unwielded, died, rewielded */
	} else if (obj == uswapwep) {
		setuswapwep((struct obj *) 0);
		if (uswapwep) return 0;     /* unwielded, died, rewielded */
	} else if (obj == uquiver) {
		setuqwep((struct obj *) 0);
		if (uquiver) return 0;     /* unwielded, died, rewielded */
	}

	if (obj->otyp == CORPSE) {
	    if ( (touch_petrifies(&mons[obj->corpsenm])) && !uarmg
		 && !Stone_resistance) {
		if (poly_when_stoned(youracedata) && polymon(PM_STONE_GOLEM))
		    display_nhwindow(WIN_MESSAGE, FALSE);
		else {
		    char kbuf[BUFSZ];

		    Strcpy(kbuf, an(corpse_xname(obj, TRUE)));
		    pline("Touching %s is a fatal mistake.", kbuf);
		    instapetrify(kbuf);
		    return -1;
		}
	    }

	    // /*
	     // * Revive corpses timed to revive immediately when trying to put
	     // * them into a magic chest.
	     // *
	     // * Timers attached to objects on the magic_chest_objs chain are
	     // * considered global and therefore follow the hero from level to
	     // * level.  If the timeout happens to be REVIVE_MON (e.g. a troll
	     // * corpse), the revival code has no sensible place to put the
	     // * revived monster and the corpse simply vanishes.  To prevent
	     // * magic chests being exploited to get rid of reviving corpses,
	     // * such corpses are revived immediately upon trying to insert them
	     // * into a magic chest.
	     // */
	    // if (report_timer(level, REVIVE_MON, obj)) {
		// if (revive_corpse(obj, REVIVE_MONSTER)) {
		    // /* Stop any multi-looting. */
		    // nomul(-1, "startled by a reviving monster");
		    // nomovemsg = "";
		    // return -1;
		// }
	    // }
	}

	/* boxes, boulders, and big statues can't fit into any container */
	if (obj->otyp == ICE_BOX || Is_box(obj) || is_boulder(obj) ||
		(obj->otyp == STATUE && bigmonst(&mons[obj->corpsenm]))) {
		/*
		 *  xname() uses a static result array.  Save obj's name
		 *  before current_container's name is computed.  Don't
		 *  use the result of strcpy() within You() --- the order
		 *  of evaluation of the parameters is undefined.
		 */
		Strcpy(buf, the(xname(obj)));
		You("cannot fit %s into %s.", buf,
		    the(xname(current_container)));
		return 0;
	}

	freeinv(obj);

	if (obj_is_burning(obj))	/* this used to be part of freeinv() */
		(void) snuff_lit(obj);

	if (floor_container && costly_spot(u.ux, u.uy)) {
	    if (current_container->no_charge && !obj->unpaid) {
		/* don't sell when putting the item into your own container */
		obj->no_charge = 1;
	    } else if (obj->oclass != COIN_CLASS) {
		/* sellobj() will take an unpaid item off the shop bill
		 * note: coins are handled later */
		was_unpaid = obj->unpaid ? TRUE : FALSE;
		sellobj_state(SELL_DELIBERATE);
		sellobj(obj, u.ux, u.uy);
		sellobj_state(SELL_NORMAL);
	    }
	}
	if (Icebox && !age_is_relative(obj)) {
		obj->age = monstermoves - obj->age; /* actual age */
		/* stop any corpse timeouts when frozen */
		if (obj->otyp == CORPSE && obj->timed) {
			long rot_alarm = stop_timer(ROT_CORPSE, obj->timed);
			stop_corpse_timers(obj);
			/* mark a non-reviving corpse as such */
			if (rot_alarm) obj->norevive = 1;
		}
	} else if (Is_mbag(current_container) && mbag_explodes(obj, 0)) {
		/* explicitly mention what item is triggering the explosion */
		pline(
	      "As you put %s inside, you are blasted by a magical explosion!",
		      doname(obj));
		/* did not actually insert obj yet */
		if (was_unpaid) addtobill(obj, FALSE, FALSE, TRUE);
		obfree(obj, (struct obj *)0);
		delete_contents(current_container);
		if (!floor_container)
			useup(current_container);
		else if (obj_here(current_container, u.ux, u.uy))
			useupf(current_container, current_container->quan);
		else
			panic("in_container:  bag not found.");

		losehp(d(6,6),"magical explosion", KILLED_BY_AN);
		current_container = 0;	/* baggone = TRUE; */
	}

	if (current_container) {
	    Strcpy(buf, the(xname(current_container)));
	    You("put %s into %s.", doname(obj), buf);
		if(cobj_is_magic_chest(current_container))
			pline("The lock labeled '%d' is open.", (int)current_container->ovar1_mgclcknm);

	    /* gold in container always needs to be added to credit */
	    if (floor_container && obj->oclass == COIN_CLASS && !cobj_is_magic_chest(current_container))
			sellobj(obj, current_container->ox, current_container->oy);
		if(cobj_is_magic_chest(current_container)){
			add_to_magic_chest(obj,((int)(current_container->ovar1_mgclcknm))%10);
		} else {
			(void) add_to_container(current_container, obj);
			current_container->owt = weight(current_container);
		}
	}
	/* gold needs this, and freeinv() many lines above may cause
	 * the encumbrance to disappear from the status, so just always
	 * update status immediately.
	 */
	bot();

	return(current_container ? 1 : -1);
}

STATIC_PTR int
ck_bag(obj)
struct obj *obj;
{
	return current_container && obj != current_container;
}

/* Returns: -1 to stop, 1 item was removed, 0 item was not removed. */
STATIC_PTR int
out_container(obj)
register struct obj *obj;
{
	register struct obj *otmp;
	boolean is_gold = (obj->oclass == COIN_CLASS);
	int res, loadlev;
	long count;

	if (!current_container) {
		impossible("<out> no current_container?");
		return -1;
	} else if (is_gold) {
		obj->owt = weight(obj);
	}

	if(obj->oartifact && !touch_artifact(obj, &youmonst, FALSE)) return 0;
	// if(obj->oartifact && obj->oartifact == ART_PEN_OF_THE_VOID && !Role_if(PM_EXILE)) u.sealsKnown |= obj->ovara_seals;
	/*Handle the pen of the void here*/
	if(obj && obj->oartifact == ART_PEN_OF_THE_VOID){
		if(obj->ovara_seals && !Role_if(PM_EXILE)){
			long oldseals = u.sealsKnown;
			u.sealsKnown |= obj->ovara_seals;
			if(oldseals != u.sealsKnown) You("learned new seals.");
		}
		obj->ovara_seals = u.spiritTineA|u.spiritTineB;
		if(u.voidChime){
			int i;
			for(i=0; i<u.sealCounts; i++){
				obj->ovara_seals |= u.spirit[i];
			}
		}
	}
	
	if (obj->otyp == CORPSE) {
	    if ( (touch_petrifies(&mons[obj->corpsenm])) && !uarmg
		 && !Stone_resistance) {
		if (poly_when_stoned(youracedata) && polymon(PM_STONE_GOLEM))
		    display_nhwindow(WIN_MESSAGE, FALSE);
		else {
		    char kbuf[BUFSZ];

		    Strcpy(kbuf, an(corpse_xname(obj, TRUE)));
		    pline("Touching %s is a fatal mistake.", kbuf);
		    instapetrify(kbuf);
		    return -1;
		}
	    }
	}

	count = obj->quan;
	if ((res = lift_object(obj, current_container, &count, FALSE)) <= 0)
	    return res;

	if (obj->quan != count && obj->otyp != LOADSTONE)
	    obj = splitobj(obj, count);

	/* Remove the object from the list. */
	obj_extract_self(obj);
	if (!cobj_is_magic_chest(current_container))
	    current_container->owt = weight(current_container);

	if (Icebox && !age_is_relative(obj)) {
		obj->age = monstermoves - obj->age; /* actual age */
		if (obj->otyp == CORPSE)
			start_corpse_timeout(obj);
	}
	/* simulated point of time */

	if(!obj->unpaid && !cobj_is_magic_chest(current_container) && 
		!carried(current_container) && 
		costly_spot(current_container->ox, current_container->oy)
	) {
		obj->ox = current_container->ox;
		obj->oy = current_container->oy;
		addtobill(obj, FALSE, FALSE, FALSE);
	}
	if(obj->shopOwned && !(obj->unpaid)){ /* shop stock is outside shop */
		if(obj->sknown && !(obj->ostolen) ) obj->sknown = FALSE; /*don't automatically know that you found a stolen item.*/
		obj->ostolen = TRUE; /* object was apparently stolen by someone (not necessarily the player) */
	}
	if (is_pick(obj) && !obj->unpaid && *u.ushops && shop_keeper(*u.ushops))
		verbalize("You sneaky cad! Get out of here with that pick!");

	otmp = addinv(obj);
	loadlev = near_capacity();
	prinv(loadlev ?
	      (loadlev < MOD_ENCUMBER ?
	       "You have a little trouble removing" :
	       "You have much trouble removing") : (char *)0,
	      otmp, count);

	if (is_gold) {
#ifndef GOLDOBJ
		dealloc_obj(obj);
#endif
		bot();	/* update character's gold piece count immediately */
	}
	return 1;
}

/* an object inside a cursed bag of holding is being destroyed */
STATIC_OVL long
mbag_item_gone(held, item)
int held;
struct obj *item;
{
    struct monst *shkp;
    long loss = 0L;

    if (item->dknown)
	pline("%s %s vanished!", Doname2(item), otense(item, "have"));
    else
	You("%s %s disappear!", Blind ? "notice" : "see", doname(item));

    if (*u.ushops && (shkp = shop_keeper(*u.ushops)) != 0) {
	if (held ? (boolean) item->unpaid : costly_spot(u.ux, u.uy))
	    loss = stolen_value(item, u.ux, u.uy,
				(boolean)shkp->mpeaceful, TRUE);
    }
    obfree(item, (struct obj *) 0);
    return loss;
}

void
observe_quantum_cat(box, past)
struct obj *box;
boolean past;
{
    static NEARDATA const char sc[] = "Schroedinger's Cat";
    struct obj *deadcat;
    struct monst *livecat;
    xchar ox, oy;

    box->spe = 0;		/* box->owt will be updated below */
    if (get_obj_location(box, &ox, &oy, 0))
	box->ox = ox, box->oy = oy;	/* in case it's being carried */

    /* this isn't really right, since any form of observation
       (telepathic or monster/object/food detection) ought to
       force the determination of alive vs dead state; but basing
       it just on opening the box is much simpler to cope with */
    livecat = (rn2(2) && !past) ? makemon(&mons[PM_HOUSECAT],
			       box->ox, box->oy, NO_MINVENT) : 0;
    if (livecat) {
	livecat->mpeaceful = 1;
	set_malign(livecat);
	if (!canspotmon(livecat))
	    You("think %s brushed your %s.", something, body_part(FOOT));
	else
	    pline("%s inside the box is still alive!", Monnam(livecat));
	(void) christen_monst(livecat, sc);
    } else {
	deadcat = mk_named_object(CORPSE, &mons[PM_HOUSECAT],
				  box->ox, box->oy, sc);
	if (deadcat) {
	    obj_extract_self(deadcat);
	    (void) add_to_container(box, deadcat);
	}
	pline_The(past ? "%s that was inside the box is dead!" : "%s inside the box is dead!",
	    Hallucination ? rndmonnam() : "housecat");
    }
    box->owt = weight(box);
    return;
}

void
open_coffin(box, past)
struct obj *box;
boolean past;
{
//    static NEARDATA const char sc[] = "Schroedinger's Cat";
//		Would be nice to name the vampire and put the name on the coffin. But not today.
    struct monst *vampire;
    xchar ox, oy;

	pline(past ? "That wasn't %s, it was a coffin!" : "This isn't %s, it's a coffin!", an(simple_typename(box->otyp)));
    box->spe = 3;		/* box->owt will be updated below */
    if (get_obj_location(box, &ox, &oy, 0))
		box->ox = ox, box->oy = oy;	/* in case it's being carried */

    vampire = makemon(&mons[PM_VAMPIRE], box->ox, box->oy, NO_MM_FLAGS);
	if(vampire){
		set_malign(vampire);
		if (!canspotmon(vampire)){
		    You("think %s brushed against your %s.", something, body_part(HAND));
		}
		else{
		    if(Hallucination) 
				pline(past ? "There was a dark knight in the coffin. The dark knight rises!" : "There is a dark knight in the coffin. The dark knight rises!");
		    else pline(past ? "There was a vampire in the coffin! %s rises." : "There is a vampire in the coffin! %s rises.", Monnam(vampire));
		}
//		(void) christen_monst(vampire, sc);
	} else {
		pline(past ? "The coffin was empty." : "The coffin is empty.");
	}
    box->owt = weight(box);
    return;
}

void
open_giants_sack(box, past)
struct obj *box;
boolean past;
{
    struct monst *victim;
    xchar ox, oy;

	pline(past ? "That %s was being used to haul a plague victim!" : "This %s is being used to haul a plague victim!", simple_typename(box->otyp));
    box->spe = 0;		/* box->owt will be updated below */
    if (get_obj_location(box, &ox, &oy, 0))
		box->ox = ox, box->oy = oy;	/* in case it's being carried */
	if(urole.neminum == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH){
		int drow_plague_types[] = {
			PM_DWARF_QUEEN, PM_DWARF_KING, 
			PM_DWARF_SMITH,
			PM_ORC_CAPTAIN, PM_JUSTICE_ARCHON, PM_SHIELD_ARCHON, PM_SWORD_ARCHON,
			PM_MOVANIC_DEVA, PM_MONADIC_DEVA, PM_ASTRAL_DEVA, 
			PM_LILLEND, PM_COURE_ELADRIN, PM_NOVIERE_ELADRIN, PM_BRALANI_ELADRIN, PM_FIRRE_ELADRIN, PM_SHIERE_ELADRIN,
			PM_SPROW, PM_DRIDER, PM_PRIESTESS_OF_GHAUNADAUR,
			PM_SHADOWSMITH,
			PM_NURSE,
			PM_ELF_LORD, PM_ELF_LADY, PM_ELVENKING, PM_ELVENQUEEN,
			PM_TREESINGER, PM_MITHRIL_SMITH,
			PM_ANULO, PM_ANULO,
			PM_DROW_CAPTAIN, PM_HEDROW_WARRIOR, PM_HEDROW_WIZARD, PM_DROW_MATRON,
			PM_DROW_CAPTAIN, PM_HEDROW_WARRIOR, PM_HEDROW_WIZARD, PM_DROW_MATRON, PM_UNEARTHLY_DROW, PM_HEDROW_BLADEMASTER,
			PM_HEDROW_MASTER_WIZARD, PM_STJARNA_ALFR, PM_PEN_A_MENDICANT, PM_MENDICANT_SPROW, PM_MENDICANT_DRIDER,
			PM_SHADOWSMITH,
			PM_YOCHLOL, PM_LILITU, PM_MARILITH,
			PM_ALLIANCE_VANGUARD, PM_PAGE, PM_DWARF_WARRIOR,
			PM_BARBARIAN, PM_HALF_DRAGON, PM_BARD, PM_HEALER, PM_RANGER, PM_VALKYRIE,
			PM_HUMAN_SMITH
		};

		victim = makemon(&mons[ROLL_FROM(drow_plague_types)], box->ox, box->oy, MM_ADJACENTOK);
	}
	else {
		int plague_types[] = {
			PM_DWARF_LORD, PM_DWARF_CLERIC, PM_DWARF_QUEEN, PM_DWARF_KING, 
			PM_DWARF_SMITH,
			PM_DEEP_ONE, PM_WINGED_KOBOLD,
			PM_DEMINYMPH, PM_THRIAE, 
			PM_ORC_CAPTAIN, PM_JUSTICE_ARCHON, PM_SHIELD_ARCHON, PM_SWORD_ARCHON,
			PM_MOVANIC_DEVA, PM_MONADIC_DEVA, PM_ASTRAL_DEVA, 
			PM_LILLEND, PM_COURE_ELADRIN, PM_NOVIERE_ELADRIN, PM_BRALANI_ELADRIN, PM_FIRRE_ELADRIN, PM_SHIERE_ELADRIN,
			PM_CENTAUR_CHIEFTAIN,
			PM_DRIDER, PM_FORMIAN_CRUSHER, PM_FORMIAN_TASKMASTER,
			PM_MYRMIDON_YPOLOCHAGOS, PM_MYRMIDON_LOCHAGOS,
			PM_GNOME_KING, PM_GNOME_QUEEN,
			PM_HILL_GIANT, PM_MINOTAUR, PM_MINOTAUR_PRIESTESS,
			PM_VAMPIRE, PM_VAMPIRE_LORD, PM_VAMPIRE_LADY,
			PM_NURSE, PM_WATCH_CAPTAIN, 
			PM_GREY_ELF, PM_ELF_LORD, PM_ELF_LADY, PM_ELVENKING, PM_ELVENQUEEN,
			PM_TREESINGER, PM_MITHRIL_SMITH,
			PM_DROW_MATRON,
			PM_HORNED_DEVIL, PM_SUCCUBUS, PM_INCUBUS, PM_ERINYS, PM_VROCK, PM_BARBED_DEVIL,
			PM_LILITU,
			PM_BARBARIAN, PM_HALF_DRAGON, PM_BARD, PM_HEALER, PM_RANGER, PM_VALKYRIE,
			PM_GOAT_SPAWN, PM_GIANT_GOAT_SPAWN
		};

		victim = makemon(&mons[ROLL_FROM(plague_types)], box->ox, box->oy, MM_ADJACENTOK);
	}
	if(victim){
		struct obj *nobj, *obj;
		for(obj = victim->minvent; obj; obj = nobj){
			nobj = obj->nobj;
			victim->misc_worn_check &= ~obj->owornmask;
			update_mon_intrinsics(victim, obj, FALSE, FALSE);
			if (obj->owornmask & W_WEP){
				setmnotwielded(victim,obj);
				MON_NOWEP(victim);
			}
			if (obj->owornmask & W_SWAPWEP){
				setmnotwielded(victim,obj);
				MON_NOSWEP(victim);
			}
			obj->owornmask = 0L;
			obj_extract_self(obj);
			if(past)
				place_object(obj, victim->mx, victim->my);
			else
				add_to_container(box, obj);
		}
		obj = mongets(victim, SHACKLES, NO_MKOBJ_FLAGS);
		if(obj){
			victim->entangled_otyp = SHACKLES;
			victim->entangled_oid = obj->o_id;
		}
		//Note: these are "fresh" so they don't take the 1/3rd penalty to level
		set_template(victim, PLAGUE_TEMPLATE);
		victim->mpeaceful = 1;
		set_malign(victim);
	} else {
		pline(past ? "But the sack was now empty." : "But the sack is now empty.");
	}
    box->owt = weight(box);
    return;
}

void
kill_giants_sack(box)
struct obj *box;
{
    struct monst *victim;
    xchar ox, oy;
	struct obj *corpse;

    box->spe = 0;		/* box->owt will be updated below */
    if (get_obj_location(box, &ox, &oy, 0))
		box->ox = ox, box->oy = oy;	/* in case it's being carried */
	if(urole.neminum == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH){
		int drow_plague_types[] = {
			PM_DWARF_QUEEN, PM_DWARF_KING, 
			PM_DWARF_SMITH,
			PM_ORC_CAPTAIN, PM_JUSTICE_ARCHON, PM_SHIELD_ARCHON, PM_SWORD_ARCHON,
			PM_MOVANIC_DEVA, PM_MONADIC_DEVA, PM_ASTRAL_DEVA, 
			PM_LILLEND, PM_COURE_ELADRIN, PM_NOVIERE_ELADRIN, PM_BRALANI_ELADRIN, PM_FIRRE_ELADRIN, PM_SHIERE_ELADRIN,
			PM_SPROW, PM_DRIDER, PM_PRIESTESS_OF_GHAUNADAUR,
			PM_SHADOWSMITH,
			PM_NURSE,
			PM_ELF_LORD, PM_ELF_LADY, PM_ELVENKING, PM_ELVENQUEEN,
			PM_TREESINGER, PM_MITHRIL_SMITH,
			PM_ANULO, PM_ANULO,
			PM_DROW_CAPTAIN, PM_HEDROW_WARRIOR, PM_HEDROW_WIZARD, PM_DROW_MATRON,
			PM_DROW_CAPTAIN, PM_HEDROW_WARRIOR, PM_HEDROW_WIZARD, PM_DROW_MATRON, PM_UNEARTHLY_DROW, PM_HEDROW_BLADEMASTER,
			PM_HEDROW_MASTER_WIZARD, PM_STJARNA_ALFR, PM_PEN_A_MENDICANT, PM_MENDICANT_SPROW, PM_MENDICANT_DRIDER,
			PM_SHADOWSMITH,
			PM_YOCHLOL, PM_LILITU, PM_MARILITH,
			PM_ALLIANCE_VANGUARD, PM_PAGE, PM_DWARF_WARRIOR,
			PM_BARBARIAN, PM_HALF_DRAGON, PM_BARD, PM_HEALER, PM_RANGER, PM_VALKYRIE,
			PM_HUMAN_SMITH
		};

		victim = makemon(&mons[ROLL_FROM(drow_plague_types)], box->ox, box->oy, MM_ADJACENTOK);
	}
	else {
		int plague_types[] = {
			PM_DWARF_LORD, PM_DWARF_CLERIC, PM_DWARF_QUEEN, PM_DWARF_KING, 
			PM_DWARF_SMITH,
			PM_DEEP_ONE, PM_WINGED_KOBOLD,
			PM_DEMINYMPH, PM_THRIAE, 
			PM_ORC_CAPTAIN, PM_JUSTICE_ARCHON, PM_SHIELD_ARCHON, PM_SWORD_ARCHON,
			PM_MOVANIC_DEVA, PM_MONADIC_DEVA, PM_ASTRAL_DEVA, 
			PM_LILLEND, PM_COURE_ELADRIN, PM_NOVIERE_ELADRIN, PM_BRALANI_ELADRIN, PM_FIRRE_ELADRIN, PM_SHIERE_ELADRIN,
			PM_CENTAUR_CHIEFTAIN,
			PM_DRIDER, PM_FORMIAN_CRUSHER, PM_FORMIAN_TASKMASTER,
			PM_MYRMIDON_YPOLOCHAGOS, PM_MYRMIDON_LOCHAGOS,
			PM_GNOME_KING, PM_GNOME_QUEEN,
			PM_HILL_GIANT, PM_MINOTAUR, PM_MINOTAUR_PRIESTESS,
			PM_VAMPIRE, PM_VAMPIRE_LORD, PM_VAMPIRE_LADY,
			PM_NURSE, PM_WATCH_CAPTAIN, 
			PM_GREY_ELF, PM_ELF_LORD, PM_ELF_LADY, PM_ELVENKING, PM_ELVENQUEEN,
			PM_TREESINGER, PM_MITHRIL_SMITH,
			PM_DROW_MATRON,
			PM_HORNED_DEVIL, PM_SUCCUBUS, PM_INCUBUS, PM_ERINYS, PM_VROCK, PM_BARBED_DEVIL,
			PM_LILITU,
			PM_BARBARIAN, PM_HALF_DRAGON, PM_BARD, PM_HEALER, PM_RANGER, PM_VALKYRIE,
			PM_GOAT_SPAWN, PM_GIANT_GOAT_SPAWN
		};

		victim = makemon(&mons[ROLL_FROM(plague_types)], box->ox, box->oy, MM_ADJACENTOK);
	}
	if(victim){
		struct obj *nobj;
		struct obj *obj;
		for(obj = victim->minvent; obj; obj = nobj){
			nobj = obj->nobj;
			victim->misc_worn_check &= ~obj->owornmask;
			update_mon_intrinsics(victim, obj, FALSE, FALSE);
			if (obj->owornmask & W_WEP){
				setmnotwielded(victim,obj);
				MON_NOWEP(victim);
			}
			if (obj->owornmask & W_SWAPWEP){
				setmnotwielded(victim,obj);
				MON_NOSWEP(victim);
			}
			obj->owornmask = 0L;
			obj_extract_self(obj);
			add_to_container(box, obj);
		}
		obj = mksobj(SHACKLES, NO_MKOBJ_FLAGS);
		if(obj)
			add_to_container(box, obj);
		//Note: these are "fresh" so they don't take the 1/3rd penalty to level
		set_template(victim, PLAGUE_TEMPLATE);
		victim->mpeaceful = 1;
		set_malign(victim);
		if(!(mons[victim->mtyp].geno & (G_NOCORPSE))){
			corpse = mkcorpstat(CORPSE, victim, (struct permonst *)0, box->ox, box->oy, FALSE);
			if(corpse){
				obj_extract_self(corpse);
				add_to_container(box, corpse);
			}
		}
		mongone(victim);
	}
    box->owt = weight(box);
    return;
}

void
open_sarcophagus(box, past)
struct obj *box;
boolean past;
{
    struct monst *mummy;
    xchar ox, oy;

	if(box->otyp == SARCOPHAGUS)
		pline(past ? "That wasn't a sarcophagus, it was a prison!" : "This isn't a sarcophagus, it's a prison!");
	else
		pline(past ? "That wasn't %s, it was a sarcophagus!" : "This isn't %s, it's a sarcophagus!", an(simple_typename(box->otyp)));
    box->spe = 3;		/* box->owt will be updated below */
    if (get_obj_location(box, &ox, &oy, 0))
		box->ox = ox, box->oy = oy;	/* in case it's being carried */

    mummy = makemon(&mons[PM_NITOCRIS], box->ox, box->oy, NO_MM_FLAGS);
	if(mummy){
		set_malign(mummy);
		if (!canspotmon(mummy)){
		    You("think %s brushed against your %s.", something, body_part(HAND));
		}
		else{
		    if(Hallucination) 
				pline("You want your mummy. Fortunately, she's right here!");
		    else pline(past ? "There was a black-wrapped mummy in the sarcophagus! She rises." : "There is a black-wrapped mummy in the sarcophagus! She rises.");
		}
	} else {
		pline(past ? "The sarcophagus was empty." : "The sarcophagus is empty.");
	}
    box->owt = weight(box);
    return;
}

void
open_crazy_box(box, past)
struct obj *box;
boolean past;
{
	struct obj *otmp;
    struct monst *daughter;
    xchar ox, oy;
	int howMany = 0;
	pline(past ? "That %s was full of crazy!" : "This %s is full of crazy!", simple_typename(box->otyp));
    box->spe = 0;		/* box->owt will be updated below */
    if (get_obj_location(box, &ox, &oy, 0))
		box->ox = ox, box->oy = oy;	/* in case it's being carried */
	
	for(otmp = box->cobj; otmp; otmp = otmp->nobj){
		if(is_suit(otmp))
			howMany++;
	}
	
	if(!howMany)
		howMany++;
	
	while(howMany){
		howMany--;
		daughter = makemon(&mons[PM_DAUGHTER_OF_BEDLAM], box->ox, box->oy, MM_ADJACENTOK);
		if(!daughter){
			//empty;
			break;
		}
		set_malign(daughter);
		daughter->m_lev = 14;
		daughter->mhpmax = 13*8+4;
		daughter->mhp = daughter->mhpmax;
		if (!canspotmon(daughter)){
			You("think %s brushed against your %s.", something, body_part(HAND));
		}
		else{
			pline("%s climbs out of the %s!", An(daughter->data->mname), past ? "wreckage" : simple_typename(box->otyp));
		}
		for(otmp = box->cobj; otmp; otmp = otmp->nobj){
			if(!is_boots(otmp))
				continue;
			obj_extract_self(otmp);
			mpickobj(daughter, otmp);
			break; //Found boots.  Also, otmp->nobj should now be 0 anyway.
		}
		for(otmp = box->cobj; otmp; otmp = otmp->nobj){
			if(!is_gloves(otmp))
				continue;
			obj_extract_self(otmp);
			mpickobj(daughter, otmp);
			break; //Found gloves.  Also, otmp->nobj should now be 0 anyway.
		}
		for(otmp = box->cobj; otmp; otmp = otmp->nobj){
			if(!is_shirt(otmp))
				continue;
			obj_extract_self(otmp);
			mpickobj(daughter, otmp);
			break; //Found shirt.  Also, otmp->nobj should now be 0 anyway.
		}
		for(otmp = box->cobj; otmp; otmp = otmp->nobj){
			if(!is_suit(otmp))
				continue;
			obj_extract_self(otmp);
			mpickobj(daughter, otmp);
			break; //Found suit.  Also, otmp->nobj should now be 0 anyway.
		}
		for(otmp = box->cobj; otmp; otmp = otmp->nobj){
			if(!is_cloak(otmp))
				continue;
			obj_extract_self(otmp);
			mpickobj(daughter, otmp);
			break; //Found cloak.  Also, otmp->nobj should now be 0 anyway.
		}
		for(otmp = box->cobj; otmp; otmp = otmp->nobj){
			if(!is_helmet(otmp))
				continue;
			obj_extract_self(otmp);
			mpickobj(daughter, otmp);
			break; //Found helm.  Also, otmp->nobj should now be 0 anyway.
		}
		for(otmp = box->cobj; otmp; otmp = otmp->nobj){
			if(!is_shield(otmp))
				continue;
			obj_extract_self(otmp);
			mpickobj(daughter, otmp);
			break; //Found shield.  Also, otmp->nobj should now be 0 anyway.
		}
		m_dowear(daughter, TRUE);
		m_level_up_intrinsic(daughter);
	}
    box->owt = weight(box);
    return;
}

boolean
open_madstuff_box(box, past)
struct obj *box;
boolean past;
{
	You_feel("memories weakly stirring.");
	struct obj *otmp;
	for(otmp = box->cobj; otmp; otmp = otmp->nobj)
		knows_object(otmp->otyp);

	if (flags.descendant){
		for(otmp = box->cobj; otmp; otmp = otmp->nobj){
			if(otmp->oartifact == u.inherited){
				expert_weapon_skill(weapon_type(otmp));
				discover_artifact(u.inherited);
				break;
			}
		}
	}

	switch(urace.malenum){
		default:
		case PM_HALF_DRAGON:
			if(flags.initgend){
				//Read through and find mercurial weapon, grant expert skill
				for(otmp = box->cobj; otmp; otmp = otmp->nobj){
					if(otmp->obj_material == MERCURIAL){
						expert_weapon_skill(objects[otmp->otyp].oc_skill);
						free_skill_up(objects[otmp->otyp].oc_skill);
					}
				}
				expert_weapon_skill(P_ATTACK_SPELL);
				u.umartial = TRUE;
				gm_weapon_skill(P_BARE_HANDED_COMBAT);
			}
			else {
				expert_weapon_skill(P_TWO_HANDED_SWORD);
				free_skill_up(P_TWO_HANDED_SWORD);
				expert_weapon_skill(P_ATTACK_SPELL);
				skilled_weapon_skill(P_BEAST_MASTERY);
			}
		break;
		case PM_VAMPIRE:
			if(flags.initgend){
				skilled_weapon_skill(P_LONG_SWORD);
				free_skill_up(P_LONG_SWORD);
			}
			else {
				expert_weapon_skill(P_SABER);
				skilled_weapon_skill(P_FIREARM);
				free_skill_up(P_SABER);
			}
		break;
		case PM_HUMAN:
		case PM_INCANTIFIER:
			if(flags.initgend){
				expert_weapon_skill(P_DAGGER);
				free_skill_up(P_DAGGER);
				skilled_weapon_skill(P_SHORT_SWORD);
				free_skill_up(P_SHORT_SWORD);
				skilled_weapon_skill(P_TWO_WEAPON_COMBAT);
			}
			else {
				//Note: dagger and two weapon default to skilled
				expert_weapon_skill(P_SABER);
				free_skill_up(P_SABER);
			}
		break;
		case PM_GNOME:
			expert_weapon_skill(P_BROAD_SWORD);
			free_skill_up(P_BROAD_SWORD);
			expert_weapon_skill(P_BEAST_MASTERY);
			free_skill_up(P_BEAST_MASTERY);
			knows_object(GNOMISH_POINTY_HAT);
			knows_object(AKLYS);
			knows_object(DWARVISH_HELM);
			knows_object(DWARVISH_MATTOCK);
			knows_object(DWARVISH_CLOAK);
		break;
		case PM_DWARF:
			expert_weapon_skill(P_PICK_AXE);
			expert_weapon_skill(P_SHIELD);
			free_skill_up(P_SHIELD);
			knows_object(DWARVISH_SPEAR);
			knows_object(DWARVISH_SHORT_SWORD);
			knows_object(DWARVISH_MATTOCK);
			knows_object(DWARVISH_HELM);
			knows_object(DWARVISH_MITHRIL_COAT);
			knows_object(DWARVISH_CLOAK);
			knows_object(DWARVISH_ROUNDSHIELD);
		break;
		case PM_ELF:
			expert_weapon_skill(P_BROAD_SWORD);
			free_skill_up(P_BROAD_SWORD);
			skilled_weapon_skill(P_SHIELD);
			free_skill_up(P_SHIELD);
			skilled_weapon_skill(P_RIDING);
			free_skill_up(P_RIDING);
			expert_weapon_skill(P_WAND_POWER);
			skilled_weapon_skill(P_FIREARM);
			knows_object(ELVEN_SHORT_SWORD);
			knows_object(ELVEN_ARROW);
			knows_object(ELVEN_BOW);
			knows_object(ELVEN_SPEAR);
			knows_object(ELVEN_DAGGER);
			knows_object(ELVEN_BROADSWORD);
			knows_object(ELVEN_MACE);
			knows_object(ELVEN_LANCE);
			knows_object(ELVEN_MITHRIL_COAT);
			knows_object(ELVEN_HELM);
			knows_object(ELVEN_SHIELD);
			knows_object(ELVEN_BOOTS);
			knows_object(ELVEN_CLOAK);
			knows_object(IMPERIAL_ELVEN_BOOTS);
			knows_object(IMPERIAL_ELVEN_ARMOR);
			knows_object(IMPERIAL_ELVEN_GAUNTLETS);
			knows_object(IMPERIAL_ELVEN_HELM);
			You("vaguely recall how to perform field repairs on imperial elven armor.");
			u.uiearepairs = TRUE;
			//The PC was actually lawful, and changes back if they are uncrowned and still their starting alignment
			if(u.ugodbase[UGOD_CURRENT] == u.ugodbase[UGOD_ORIGINAL] && !u.uevent.uhand_of_elbereth){
				/* The player wears a helm of opposite alignment? */
				if (uarmh && uarmh->otyp == HELM_OF_OPPOSITE_ALIGNMENT)
					u.ugodbase[UGOD_ORIGINAL] = u.ugodbase[UGOD_CURRENT] = GOD_ZO_KALAR;
				else {
					u.ualign.god = u.ugodbase[UGOD_ORIGINAL] = u.ugodbase[UGOD_CURRENT] = GOD_ZO_KALAR;
					u.ualign.type = A_LAWFUL;
				}
				You("have a sudden sense of returning to an old direction.");
				flags.initalign = 0;
				flags.botl = TRUE;
				change_luck(-3);
				u.ublesscnt += 300;
				u.lastprayed = moves;
				u.reconciled = REC_NONE;
				u.lastprayresult = PRAY_CONV;
				adjalign(-1*u.ualign.record);
				if(godlist[u.ualign.god].anger) {
					pline("%s seems %s.", u_gname(),
					  Hallucination ? "groovy" : "mollified");
					godlist[u.ualign.god].anger = 0;
					if ((int)u.uluck < 0) u.uluck = 0;
				}
			}
		break;
		case PM_DROW:
			if(flags.initgend){
				expert_weapon_skill(P_SABER);
				expert_weapon_skill(P_WHIP);
				free_skill_up(P_WHIP);
				skilled_weapon_skill(P_BEAST_MASTERY);
			}
			else {
				expert_weapon_skill(P_SABER);
				expert_weapon_skill(P_MORNING_STAR);
				free_skill_up(P_MORNING_STAR);
				expert_weapon_skill(P_TWO_HANDED_SWORD);
			}
			knows_object(DROVEN_SHORT_SWORD);
			knows_object(DROVEN_BOLT);
			knows_object(DROVEN_CROSSBOW);
			knows_object(DROVEN_DAGGER);
			knows_object(DROVEN_GREATSWORD);
			knows_object(DROVEN_LANCE);
			knows_object(DROVEN_SPEAR);
			knows_object(DROVEN_CHAIN_MAIL);
			knows_object(DROVEN_PLATE_MAIL);
			knows_object(NOBLE_S_DRESS);
			knows_object(CONSORT_S_SUIT);
			knows_object(DROVEN_CLOAK);
			knows_object(find_signet_ring());
		break;
		case PM_ORC:
			expert_weapon_skill(P_BOW);
			expert_weapon_skill(P_SCIMITAR);
			skilled_weapon_skill(P_TWO_HANDED_SWORD);
			skilled_weapon_skill(P_RIDING);
			free_skill_up(P_RIDING);
			knows_object(ORCISH_SHORT_SWORD);
			knows_object(ORCISH_ARROW);
			knows_object(ORCISH_BOW);
			knows_object(ORCISH_SPEAR);
			knows_object(ORCISH_DAGGER);
			knows_object(ORCISH_CHAIN_MAIL);
			knows_object(ORCISH_RING_MAIL);
			knows_object(ORCISH_HELM);
			knows_object(ORCISH_SHIELD);
			knows_object(URUK_HAI_SHIELD);
			knows_object(ORCISH_CLOAK);

			knows_object(ELVEN_SHORT_SWORD);
			knows_object(ELVEN_ARROW);
			knows_object(ELVEN_BOW);
			knows_object(ELVEN_SPEAR);
			knows_object(ELVEN_DAGGER);
			knows_object(ELVEN_BROADSWORD);
			knows_object(ELVEN_MACE);
			knows_object(ELVEN_LANCE);
			knows_object(ELVEN_MITHRIL_COAT);
			knows_object(ELVEN_HELM);
			knows_object(ELVEN_SHIELD);
			knows_object(ELVEN_BOOTS);
			knows_object(ELVEN_CLOAK);
		break;
		case PM_YUKI_ONNA:
			expert_weapon_skill(P_LONG_SWORD);
			free_skill_up(P_LONG_SWORD);
			expert_weapon_skill(P_SHORT_SWORD);
			free_skill_up(P_SHORT_SWORD);
			expert_weapon_skill(P_TWO_WEAPON_COMBAT);
		break;
	}
	if(Insight >= 10){
		open_crazy_box(box, past);
		box->spe = 0;
		return TRUE;
	}
	box->spe = 0;
    return FALSE;
	
}

#undef Icebox
STATIC_OVL
char
pick_gemstone()
{
	winid tmpwin;
	int n=0, how,count=0;
	char buf[BUFSZ];
	struct obj *otmp;
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */
	
	Sprintf(buf, "Gems");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
	for(otmp = invent; otmp; otmp = otmp->nobj){
		if(valid_focus_gem(otmp)){
			Sprintf1(buf, doname(otmp));
			any.a_char = otmp->invlet;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				otmp->invlet, 0, ATR_NONE, buf,
				MENU_UNSELECTED);
			count++;
		}
	}
	end_menu(tmpwin, "Choose new focusing gem:");

	how = PICK_ONE;
	if(count) n = select_menu(tmpwin, how, &selected);
	else You("don't have any gems.");
	destroy_nhwindow(tmpwin);
	if(n > 0){
		char picked = selected[0].item.a_char;
		free(selected);
		return picked;
	}
	return 0;
}

STATIC_OVL
char
pick_bullet()
{
	winid tmpwin;
	int n=0, how,count=0;
	char buf[BUFSZ];
	struct obj *otmp;
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */
	
	Sprintf(buf, "Bullets");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
	for(otmp = invent; otmp; otmp = otmp->nobj){
		if((otmp->otyp >= MAGICITE_CRYSTAL && otmp->otyp <= ROCK) || (otmp->otyp >= BULLET && otmp->otyp <= GAS_GRENADE)){
			if (is_grenade(otmp) && otmp->oarmed)
				continue;

			Sprintf1(buf, doname(otmp));
			any.a_char = otmp->invlet;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				otmp->invlet, 0, ATR_NONE, buf,
				MENU_UNSELECTED);
			count++;
		}
	}
	end_menu(tmpwin, "Choose new mass source:");

	how = PICK_ONE;
	if(count) n = select_menu(tmpwin, how, &selected);
	else You("don't have any bullets.");
	destroy_nhwindow(tmpwin);
	if(n > 0){
		char picked = selected[0].item.a_char;
		free(selected);
		return picked;
	}
	return 0;
}


STATIC_OVL
char
pick_coin()
{
	winid tmpwin;
	int n=0, how,count=0;
	char buf[BUFSZ];
	struct obj *otmp;
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */
	
	Sprintf(buf, "Coins");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
	for(otmp = invent; otmp; otmp = otmp->nobj){
		if(otmp->otyp >= WAGE_OF_SLOTH && otmp->otyp <= WAGE_OF_PRIDE){
			Sprintf1(buf, doname(otmp));
			any.a_char = otmp->invlet;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				otmp->invlet, 0, ATR_NONE, buf,
				MENU_UNSELECTED);
			count++;
		}
	}
	end_menu(tmpwin, "Choose new coin:");

	how = PICK_ONE;
	if(count) n = select_menu(tmpwin, how, &selected);
	else You("don't have any suitable coins.");
	destroy_nhwindow(tmpwin);
	if(n > 0){
		char picked = selected[0].item.a_char;
		free(selected);
		return picked;
	}
	return 0;
}

STATIC_OVL
struct obj *
pick_creatures_armor(mon, passed_info)
struct monst *mon;
int *passed_info;
{
	winid tmpwin;
	int n=0, how,count=0;
	char buf[BUFSZ];
	char incntlet='a';
	struct obj *otmp;
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */
	
	Sprintf(buf, "Equipment");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
	for(otmp = mon->minvent; otmp; otmp = otmp->nobj){
		if((otmp->owornmask&W_SADDLE) && mon == u.usteed)
			continue;
		Sprintf1(buf, doname(otmp));
		any.a_obj = otmp;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet++;
		if(incntlet > 'z')
			incntlet = 'A';
		if(incntlet > 'Z' && incntlet < 'a')
			incntlet = 'a';
		count++;
	}
	end_menu(tmpwin, "What do you want to remove:");

	how = PICK_ONE;
	if(count){
		if (passed_info) *passed_info = 1;
		n = select_menu(tmpwin, how, &selected);
	} else pline("Nothing to remove!");
	destroy_nhwindow(tmpwin);
	if(n > 0){
		struct obj *picked = selected[0].item.a_obj;
		free(selected);
		return picked;
	}
	return 0;
}

#define addArmorMenuOption	Sprintf1(buf, doname(otmp));\
							any.a_obj = otmp;\
							add_menu(tmpwin, NO_GLYPH, &any,\
							otmp->invlet, 0, ATR_NONE, buf,\
							MENU_UNSELECTED);\
							count++;


STATIC_OVL
struct obj *
pick_armor_for_creature(mon)
struct monst *mon;
{
	winid tmpwin;
	int n=0, how,count=0;
	char buf[BUFSZ];
	struct obj *otmp;
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */
	
	Sprintf(buf, "Equipment");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
	if(!(is_gaseous_noequip(mon->data) || noncorporeal(mon->data))) for(otmp = invent; otmp; otmp = otmp->nobj){
		if(!otmp->owornmask){
			if(otmp->oclass == AMULET_CLASS && !(mon->misc_worn_check&W_AMUL) &&
				can_wear_amulet(mon->data) && 
				objects[otmp->otyp].oc_name_known &&
			    is_museable_amulet(otmp->otyp)
			){
				addArmorMenuOption
			} else if(is_shirt(otmp) && !(mon->misc_worn_check&W_ARMU) && otmp->objsize == mon->data->msize && shirt_match(mon->data,otmp)){
				addArmorMenuOption
			} else if(is_cloak(otmp) && !(mon->misc_worn_check&W_ARMC) && (abs(otmp->objsize - mon->data->msize) <= 1)){
				addArmorMenuOption
			} else if(is_helmet(otmp) && !(mon->misc_worn_check&W_ARMH) && helm_match(mon->data,otmp) && helm_size_fits(mon->data,otmp)){
				addArmorMenuOption
			} else if(is_shield(otmp) && !(mon->misc_worn_check&W_ARMS) && !noshield(mon->data)){
				addArmorMenuOption
			} else if(is_gloves(otmp) && !(mon->misc_worn_check&W_ARMG) && otmp->objsize == mon->data->msize && can_wear_gloves(mon->data)){
				addArmorMenuOption
			} else if(is_boots(otmp) && !(mon->misc_worn_check&W_ARMF) && boots_size_fits(mon->data, otmp) && can_wear_boots(mon->data)){
				addArmorMenuOption
			} else if(is_suit(otmp) && !(mon->misc_worn_check&W_ARM) && arm_match(mon->data, otmp) && arm_size_fits(mon->data, otmp)){
				addArmorMenuOption
			} else if(is_worn_tool(otmp) && !(mon->misc_worn_check&W_TOOL) && can_wear_blindf(mon->data)){
				addArmorMenuOption
			} else if(is_belt(otmp) && !(mon->misc_worn_check&W_BELT) && can_wear_belt(mon->data)){
				addArmorMenuOption
			}
		}
	}
	end_menu(tmpwin, "What do you want to equip:");

	how = PICK_ONE;
	if(count){
		n = select_menu(tmpwin, how, &selected);
	} else pline("Nothing to equip!");
	destroy_nhwindow(tmpwin);
	if(n > 0){
		struct obj *picked = selected[0].item.a_obj;
		free(selected);
		return picked;
	}
	return 0;
}

STATIC_OVL int
use_lightsaber(obj)
register struct obj *obj;
{
	struct obj *otmp;
	char gemlet;
	gemlet = pick_gemstone();
	
	for (otmp = invent; otmp; otmp = otmp->nobj) {
		if(otmp->invlet == gemlet && valid_focus_gem(otmp)) break;
	}
	if(otmp){
		current_container = obj;
		if(obj->cobj){
			if(obj->cobj->oartifact == obj->oartifact)
				obj->oartifact = 0;
			out_container(obj->cobj);
		}
		if(otmp->quan > 1)
			otmp = splitobj(otmp, 1);
		if(!obj->cobj){
			in_container(otmp);
			if(otmp->oartifact && !obj->oartifact)
				obj->oartifact = otmp->oartifact;
		}
		return MOVE_STANDARD;
	} else return MOVE_CANCELLED;
}

int
use_massblaster(obj)
register struct obj *obj;
{
	struct obj *otmp;
	char gemlet;
	gemlet = pick_bullet();
	
	for (otmp = invent; otmp; otmp = otmp->nobj) {
		if(otmp->invlet == gemlet) break;
	}
	if(otmp){
		current_container = obj;
		if(obj->cobj) 
			out_container(obj->cobj);
		if(otmp->quan > 1)
			otmp = splitobj(otmp, 1);
		if(!obj->cobj)
			in_container(otmp);
		return MOVE_STANDARD;
	} else return MOVE_CANCELLED;
}

int
use_demon_claw(obj)
register struct obj *obj;
{
	struct obj *otmp;
	char coin;
	coin = pick_coin();
	
	if(!coin)
		return MOVE_CANCELLED;
	
	for (otmp = invent; otmp; otmp = otmp->nobj) {
		if(otmp->invlet == coin) break;
	}
	if(otmp){
		current_container = obj;
		if(obj->cobj) 
			out_container(obj->cobj);
		if(otmp->quan > 1)
			otmp = splitobj(otmp, 1);
		if(!obj->cobj)
			in_container(otmp);
		return MOVE_PARTIAL;
	} else return MOVE_CANCELLED;
}

int
use_container(obj, held)
register struct obj *obj;
register int held;
{
	struct obj *curr, *otmp;
#ifndef GOLDOBJ
	struct obj *u_gold = (struct obj *)0;
#endif
	boolean one_by_one, allflag, quantum_cat = FALSE,
		loot_out = FALSE, loot_in = FALSE, tip_over = FALSE;
	char select[MAXOCLASSES+1];
	char qbuf[BUFSZ], emptymsg[BUFSZ], pbuf[QBUFSZ];
	long loss = 0L;
	int cnt = 0, used = 0,
	    menu_on_request;

	emptymsg[0] = '\0';
	if (nohands(youracedata)) {
		You("have no hands!");	/* not `body_part(HAND)' */
		return MOVE_CANCELLED;
	} else if (!freehand()) {
		You("have no free %s.", body_part(HAND));
		return MOVE_CANCELLED;
	}
	if (obj->olocked) {
	    pline("%s to be locked.", Tobjnam(obj, "seem"));
	    if (held) You("must put it down to unlock.");
	    return MOVE_CANCELLED;
	} else if (obj->otrapped && (obj->otyp != MAGIC_CHEST)) {
	    if (held) You("open %s...", the(xname(obj)));
	    (void) chest_trap(obj, HAND, FALSE);
	    /* even if the trap fails, you've used up this turn */
	    if (multi >= 0) {	/* in case we didn't become paralyzed */
		nomul(-1, "opening a container");
		nomovemsg = "";
	    }
	    return MOVE_CONTAINER;
	}
	current_container = obj;	/* for use by in/out_container */
	if(Is_real_container(obj)){
		if (obj->spe == 1) {
			observe_quantum_cat(obj, FALSE); //FALSE: the box was not destroyed. Use present tense.
			used = 1;
			quantum_cat = TRUE;	/* for adjusting "it's empty" message */
		} else if(obj->spe == 4){
			open_coffin(obj, FALSE); //FALSE: the box was not destroyed. Use present tense.
			return MOVE_CONTAINER;
		} else if(obj->spe == 5){
			open_sarcophagus(obj, FALSE); //FALSE: the box was not destroyed. Use present tense.
			return MOVE_CONTAINER;
		} else if(obj->spe == 6 && Insight >= 10){
			open_crazy_box(obj, FALSE); //FALSE: the box was not destroyed. Use present tense.
			return MOVE_CONTAINER;
		} else if(obj->spe == 7){
			// Madman reclaims their stuff. Contents handled by the level loader.
			//FALSE: the box was not destroyed. Use present tense.
			if(open_madstuff_box(obj, FALSE)){
				return MOVE_CONTAINER;
			}
		} else if(obj->spe == 8){
			// Nothing. Fulvous desk spawns monsters.
		} else if(obj->spe == 9){
			open_giants_sack(obj, FALSE); //FALSE: not destroyed
			return MOVE_CONTAINER;
		}
	}
	/* Count the number of contained objects. Sometimes toss objects if a cursed magic bag. */
	if (cobj_is_magic_chest(obj))
	    curr = magic_chest_objs[((int)obj->ovar1_mgclcknm)%10];/*guard against polymorph related whoopies*/
	else
	    curr = obj->cobj;
	for (; curr; curr = otmp) {
	    otmp = curr->nobj;
	    if (Is_mbag(obj) && obj->cursed && !rn2(13)) {
		obj_extract_self(curr);
		loss += mbag_item_gone(held, curr);
		used = 1;
	    } else {
		cnt++;
	    }
	}

	if (loss)	/* magic bag lost some shop goods */
	    You("owe %ld %s for lost merchandise.", loss, currency(loss));
	if (!cobj_is_magic_chest(obj))
		obj->owt = weight(obj);	/* in case any items were lost */

	if (!cnt)
	    Sprintf(emptymsg, "%s is %sempty.", Yname2(obj),
		    quantum_cat ? "now " : "");

	if (cnt || flags.menu_style == MENU_FULL) {
	    Strcpy(qbuf, "Do you want to take something out of ");
	    Sprintf(eos(qbuf), "%s?",
		    safe_qbuf(qbuf, 1, yname(obj), ysimple_name(obj), "it"));
	    if (flags.menu_style != MENU_TRADITIONAL) {
			if (flags.menu_style == MENU_FULL) {
				int t;
				char menuprompt[BUFSZ];
				boolean outokay = (cnt != 0);
#ifndef GOLDOBJ
				boolean inokay = (invent != 0) || (u.ugold != 0);
#else
				boolean inokay = (invent != 0);
#endif
				if (!outokay && !inokay) {
					pline("%s", emptymsg);
					You("don't have anything to put in.");
					return used ? MOVE_CONTAINER : MOVE_CANCELLED;
				}
				menuprompt[0] = '\0';
				if (!cnt) Sprintf(menuprompt, "%s ", emptymsg);
				Strcat(menuprompt, "Do what?");
				t = in_or_out_menu(menuprompt, current_container, outokay, inokay);
				if (t <= 0) return MOVE_CANCELLED;
				loot_out = (t & 0x01) != 0;
				loot_in  = (t & 0x02) != 0;
				tip_over  = (t & 0x04) != 0;
			} else {	/* MENU_COMBINATION or MENU_PARTIAL */
				loot_out = (yn_function(qbuf, "ynq", 'n') == 'y');
			}
			if (tip_over) {
				tipcontainer(current_container);
				if(current_container->oartifact == ART_ESSCOOAHLIPBOOURRR)
					current_container->age = monstermoves + rnz(100);
				used |= MOVE_CONTAINER;
			}
			if (loot_out) {
				int sub_used;
				add_valid_menu_class(0);	/* reset */
				sub_used = menu_loot(0, current_container, FALSE) > 0;
				if(sub_used && current_container->oartifact == ART_ESSCOOAHLIPBOOURRR)
					current_container->age = monstermoves + rnz(100);
				used |= sub_used;
			}
	    } else {
		/* traditional code */
ask_again2:
		menu_on_request = 0;
		add_valid_menu_class(0);	/* reset */
		Strcpy(pbuf, ":ynq");
		if (cnt) Strcat(pbuf, "m");
		switch (yn_function(qbuf, pbuf, 'n')) {
		case ':':
		    container_contents(current_container, FALSE, FALSE);
		    goto ask_again2;
		case 'y':
		    if (query_classes(select, &one_by_one, &allflag,
				      "take out", current_container->cobj,
				      FALSE,
#ifndef GOLDOBJ
				      FALSE,
#endif
				      &menu_on_request)) {
			if (askchain((struct obj **)&current_container->cobj,
				     (one_by_one ? (char *)0 : select),
				     allflag, out_container,
				     (int FDECL((*),(OBJ_P)))0,
				     0, "nodot"))
			    used = 1;
		    } else if (menu_on_request < 0) {
			used |= menu_loot(menu_on_request,
					  current_container, FALSE) > 0;
		    }
		    /*FALLTHRU*/
		case 'n':
		    break;
		case 'm':
		    menu_on_request = -2; /* triggers ALL_CLASSES */
		    used |= menu_loot(menu_on_request, current_container, FALSE) > 0;
		    break;
		case 'q':
		default:
		    return used ? MOVE_CONTAINER : MOVE_CANCELLED;
		}
	    }
	} else {
	    pline("%s", emptymsg);		/* <whatever> is empty. */
	}

#ifndef GOLDOBJ
	if (!invent && u.ugold == 0)
#else
	if (!invent)
#endif
	{
	    /* nothing to put in, but some feedback is necessary */
	    You("don't have anything to put in.");
	    return used ? MOVE_CONTAINER : MOVE_CANCELLED;
	}
	if (flags.menu_style != MENU_FULL) {
	    Sprintf(qbuf, "Do you wish to put %s in?", something);
	    Strcpy(pbuf, ynqchars);
	    if (flags.menu_style == MENU_TRADITIONAL && invent && inv_cnt() > 0)
		Strcat(pbuf, "m");
	    switch (yn_function(qbuf, pbuf, 'n')) {
		case 'y':
		    loot_in = TRUE;
		    break;
		case 'n':
		    break;
		case 'm':
		    add_valid_menu_class(0);	  /* reset */
		    menu_on_request = -2; /* triggers ALL_CLASSES */
		    used |= menu_loot(menu_on_request, current_container, TRUE) > 0;
		    break;
		case 'q':
		default:
		    return used ? MOVE_CONTAINER : MOVE_CANCELLED;
	    }
	}
	/*
	 * Gone: being nice about only selecting food if we know we are
	 * putting things in an ice chest.
	 */
	if (loot_in) {
#ifndef GOLDOBJ
	    if (u.ugold) {
		/*
		 * Hack: gold is not in the inventory, so make a gold object
		 * and put it at the head of the inventory list.
		 */
		u_gold = mkgoldobj(u.ugold);	/* removes from u.ugold */
		u_gold->in_use = TRUE;
		u.ugold = u_gold->quan;		/* put the gold back */
		assigninvlet(u_gold);		/* might end up as NOINVSYM */
		u_gold->nobj = invent;
		invent = u_gold;
	    }
#endif
	    add_valid_menu_class(0);	  /* reset */
	    if (flags.menu_style != MENU_TRADITIONAL) {
			used |= menu_loot(0, current_container, TRUE) > 0;
	    } else {
			/* traditional code */
			menu_on_request = 0;
			if (query_classes(select, &one_by_one, &allflag, "put in",
					   invent, FALSE,
#ifndef GOLDOBJ
					(u.ugold != 0L),
#endif
					&menu_on_request)
			) {
				(void) askchain((struct obj **)&invent,
						(one_by_one ? (char *)0 : select), allflag,
						in_container, ck_bag, 0, "nodot");
				used = 1;
			} else if (menu_on_request < 0) {
				used |= menu_loot(menu_on_request,
						  current_container, TRUE) > 0;
			}
	    }
	}

#ifndef GOLDOBJ
	if (u_gold && invent && invent->oclass == COIN_CLASS) {
	    /* didn't stash [all of] it */
	    u_gold = invent;
	    invent = u_gold->nobj;
	    u_gold->in_use = FALSE;
	    dealloc_obj(u_gold);
	}
#endif
	return used ? MOVE_CONTAINER : MOVE_CANCELLED;
}

/* Loot a container (take things out, put things in), using a menu. */
STATIC_OVL int
menu_loot(retry, container, put_in)
int retry;
struct obj *container;
boolean put_in;
{
    int n, i, n_looted = 0;
    boolean all_categories = TRUE, loot_everything = FALSE;
    char buf[BUFSZ];
    const char *takeout = "Take out", *putin = "Put in";
    struct obj *otmp, *otmp2;
    menu_item *pick_list;
    int mflags, res;
    long count;

    if (retry) {
	all_categories = (retry == -2);
    } else if (flags.menu_style == MENU_FULL) {
	all_categories = FALSE;
	Sprintf(buf,"%s what type of objects?", put_in ? putin : takeout);
	mflags = put_in ? ALL_TYPES | BUC_ALLBKNOWN | BUC_UNKNOWN :
		          ALL_TYPES | CHOOSE_ALL | BUC_ALLBKNOWN | BUC_UNKNOWN;
	if (put_in) {
	    otmp = invent;
	} else {
	    if (cobj_is_magic_chest(container))
		otmp = magic_chest_objs[((int)(container->ovar1_mgclcknm))%10];
	    else
		otmp = container->cobj;
	}
	n = query_category(buf, otmp, mflags, &pick_list, PICK_ANY);
	if (!n) return 0;
	for (i = 0; i < n; i++) {
	    if (pick_list[i].item.a_int == 'A')
		loot_everything = TRUE;
	    else if (pick_list[i].item.a_int == ALL_TYPES_SELECTED)
		all_categories = TRUE;
	    else
		add_valid_menu_class(pick_list[i].item.a_int);
	}
	free((genericptr_t) pick_list);
    }

    if (loot_everything) {
	if (cobj_is_magic_chest(container))
	    otmp = magic_chest_objs[((int)(container->ovar1_mgclcknm))%10];
	else
	    otmp = container->cobj;
	for (; otmp; otmp = otmp2) {
	    otmp2 = otmp->nobj;
	    res = out_container(otmp);
	    if (res < 0) break;
	}
    } else {
	mflags = INVORDER_SORT;
	if (put_in && flags.invlet_constant) mflags |= USE_INVLET;
	Sprintf(buf,"%s what?", put_in ? putin : takeout);
	
	if (put_in) {
	    otmp = invent;
	} else {
	    if (cobj_is_magic_chest(container))
		otmp = magic_chest_objs[((int)(container->ovar1_mgclcknm))%10];
	    else
		otmp = container->cobj;
	}

	n = query_objlist(buf, otmp,
			  mflags, &pick_list, PICK_ANY,
			  all_categories ? allow_all : allow_category);
	if (n) {
		n_looted = n;
		for (i = 0; i < n; i++) {
		    otmp = pick_list[i].item.a_obj;
		    count = pick_list[i].count;
		    if (count > 0 && count < otmp->quan) {
			otmp = splitobj(otmp, count);
			/* special split case also handled by askchain() */
		    }
		    res = put_in ? in_container(otmp) : out_container(otmp);
		    if (res < 0) {
			if (otmp != pick_list[i].item.a_obj) {
			    /* split occurred, merge again */
			    (void) merged(&pick_list[i].item.a_obj, &otmp);
			}
			break;
		    }
		}
		free((genericptr_t)pick_list);
	}
    }
    return n_looted;
}

STATIC_OVL int
in_or_out_menu(prompt, obj, outokay, inokay)
const char *prompt;
struct obj *obj;
boolean outokay, inokay;
{
    winid win;
    anything any;
    menu_item *pick_list;
    char buf[BUFSZ];
    int n;
    const char *menuselector = iflags.lootabc ? "abcd" : "oibt";

    any.a_void = 0;
    win = create_nhwindow(NHW_MENU);
    start_menu(win);
    if (outokay) {
	any.a_int = 1;
	Sprintf(buf,"Take %s out of %s", something, the(xname(obj)));
	add_menu(win, NO_GLYPH, &any, *menuselector, 0, ATR_NONE,
			buf, MENU_UNSELECTED);
    }
    menuselector++;
    if (inokay) {
	any.a_int = 2;
	Sprintf(buf,"Put %s into %s", something, the(xname(obj)));
	add_menu(win, NO_GLYPH, &any, *menuselector, 0, ATR_NONE, buf, MENU_UNSELECTED);
    }
    menuselector++;
    if (outokay && inokay) {
	any.a_int = 3;
	add_menu(win, NO_GLYPH, &any, *menuselector, 0, ATR_NONE,
			"Both of the above", MENU_UNSELECTED);
    }
    menuselector++;
    if (outokay) {
	any.a_int = 4;
	add_menu(win, NO_GLYPH, &any, *menuselector, 0, ATR_NONE,
			"Tip out all contents", MENU_UNSELECTED);
    }
    end_menu(win, prompt);
    n = select_menu(win, PICK_ONE, &pick_list);
    destroy_nhwindow(win);
    if (n > 0) {
	n = pick_list[0].item.a_int;
	free((genericptr_t) pick_list);
    }
    return n;
}

static const char tippables[] = { ALL_CLASSES, TOOL_CLASS, 0 };


/* #tip command -- empty container contents onto floor */
int
dotip()
{
	struct obj *cobj, *nobj;
	coord cc;
	int boxes;
	char c, buf[BUFSZ], qbuf[BUFSZ];
	const char *spillage = 0;

	/*
	 * doesn't require free hands;
	 * limbs are needed to tip floor containers
	 */

	/* at present, can only tip things at current spot, not adjacent ones */
	cc.x = u.ux, cc.y = u.uy;

	/* check floor container(s) first; at most one will be accessed */
	if ((boxes = container_at(cc.x, cc.y, TRUE)) > 0) {
		Sprintf(buf, "You can't tip %s while carrying so much.",
				!flags.verbose ? "a container" : (boxes > 1) ? "one" : "it");
		if (!check_capacity(buf) && able_to_loot(cc.x, cc.y, FALSE)) {
			if (boxes > 1 && (flags.menu_style != MENU_TRADITIONAL
							  || iflags.menu_requested)) {
				/* use menu to pick a container to tip */
				int n, i;
				winid win;
				anything any;
				menu_item *pick_list = (menu_item *) 0;
				struct obj dummyobj, *otmp;

				any = zeroany;
				win = create_nhwindow(NHW_MENU);
				start_menu(win);

				for (cobj = level.objects[cc.x][cc.y], i = 0; cobj;
					 cobj = cobj->nexthere)
					if (Is_container(cobj)) {
						++i;
						any.a_obj = cobj;
						add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
								 doname(cobj), MENU_UNSELECTED);
					}
				if (invent) {
					any = zeroany;
					add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
							 "", MENU_UNSELECTED);
					any.a_obj = &dummyobj;
					/* use 'i' for inventory unless there are so many
					   containers that it's already being used */
					i = (i <= 'i' - 'a' && !iflags.lootabc) ? 'i' : 0;
					add_menu(win, NO_GLYPH, &any, i, 0, ATR_NONE,
							 "tip something being carried", MENU_UNSELECTED);
				}
				end_menu(win, "Tip which container?");
				n = select_menu(win, PICK_ONE, &pick_list);
				destroy_nhwindow(win);
				/*
				 * Deal with quirk of preselected item in pick-one menu:
				 * n ==  0 => picked preselected entry, toggling it off;
				 * n ==  1 => accepted preselected choice via SPACE or RETURN;
				 * n ==  2 => picked something other than preselected entry;
				 * n == -1 => cancelled via ESC;
				 */
				otmp = (n <= 0) ? (struct obj *) 0 : pick_list[0].item.a_obj;
				if (n > 1 && otmp == &dummyobj)
					otmp = pick_list[1].item.a_obj;
				if (pick_list)
					free((genericptr_t) pick_list);
				if (otmp && otmp != &dummyobj) {
					tipcontainer(otmp);
					return MOVE_STANDARD;
				}
				if (n == -1)
					return MOVE_CANCELLED;
				/* else pick-from-invent below */
			} else {
				for (cobj = level.objects[cc.x][cc.y]; cobj; cobj = nobj) {
					nobj = cobj->nexthere;
					if (!Is_container(cobj))
						continue;
					Sprintf(qbuf, "You see here %s, tip it?", safe_qbuf(qbuf, sizeof("You see here , tip it?"), doname(cobj), xname(cobj), "container"));
					c = ynq(qbuf);
					if (c == 'q')
						return MOVE_CANCELLED;
					if (c == 'n')
						continue;
					tipcontainer(cobj);
					/* can only tip one container at a time */
					return MOVE_STANDARD;
				}
			}
		}
	}

	/* either no floor container(s) or couldn't tip one or didn't tip any */
	cobj = getobj(tippables, "tip");
	if (!cobj)
		goto tipmonster;

	/* normal case */
	if (Is_container(cobj) || cobj->otyp == HORN_OF_PLENTY) {
		tipcontainer(cobj);
		return MOVE_STANDARD;
	}
	/* assorted other cases */
	if (Is_candle(cobj) && cobj->lamplit) {
		/* note "wax" even for tallow candles to avoid giving away info */
		spillage = "wax";
	} else if ((cobj->otyp == POT_OIL && cobj->lamplit)
			   || (cobj->otyp == OIL_LAMP && cobj->age != 0L)
			   || (cobj->otyp == MAGIC_LAMP && cobj->spe != 0)) {
		spillage = "oil";
		/* todo: reduce potion's remaining burn timer or oil lamp's fuel */
	} else if (cobj->otyp == CAN_OF_GREASE && cobj->spe > 0) {
		/* charged consumed below */
		spillage = "grease";
	} else if (cobj->otyp == FOOD_RATION || cobj->otyp == CRAM_RATION
			   || cobj->otyp == LEMBAS_WAFER) {
		spillage = "crumbs";
	} else if (cobj->oclass == VENOM_CLASS) {
		spillage = "venom";
	}
	if (spillage) {
		buf[0] = '\0';
		if (is_pool(u.ux, u.uy, TRUE))
			Sprintf(buf, " and gradually %s", vtense(spillage, "dissipate"));
		else if (is_lava(u.ux, u.uy))
			Sprintf(buf, " and immediately %s away",
					vtense(spillage, "burn"));
		pline("Some %s %s onto the %s%s.", spillage,
			  vtense(spillage, "spill"), surface(u.ux, u.uy), buf);
		/* shop usage message comes after the spill message */
		if (cobj->otyp == CAN_OF_GREASE && cobj->spe > 0) {
			consume_obj_charge(cobj, TRUE);
		}
		/* something [useless] happened */
		return MOVE_STANDARD;
	}
	/* anything not covered yet */
	if (cobj->oclass == POTION_CLASS) /* can't pour potions... */
		pline_The("%s %s securely sealed.", xname(cobj), otense(cobj, "are"));
	else if (uarmh && cobj == uarmh)
		return tiphat();
	else if (cobj->otyp == STATUE)
		pline("Nothing interesting happens.");
	else
		pline1(nothing_happens);
tipmonster:
#define can_respond(mtmp) (mtmp && mtmp->mcanmove && !mtmp->msleeping && m_canseeu(mtmp))
	if (getdir("Tip a monster? (in what direction)")){
		struct monst* mtmp = m_at(u.ux+u.dx, u.uy+u.dy);
		if (!canspotmon(mtmp))
			mtmp = (struct monst *) 0;
		
		if (!mtmp)
			You("don't see anybody there to tip.");
		else if (mtmp->mtyp == PM_ROTHE){
			if (near_capacity() >= MOD_ENCUMBER)
				You("are carrying too much to do that without falling over.");
			else if (!(mtmp->mfrozen || mtmp->msleeping))
				pline("The rothe is awake and resists being toppled over!");
			else {
				You("try to push the rothe over.");
				// going to assume that even if you have no hands, you can still bodycheck
				// the rothe into wakefulness
				if (ACURRSTR < 14){
					pline("It's too heavy for you to budge.");
					if (rn2(2)){
						mtmp->msleeping = 0;
						mtmp->mfrozen = 0;
						pline("At your push, the rothe wakes up!");
					}
				} else {
					if (Hallucination)
						pline("The creature has fallen and can't get up.");
					else
						You("successfully tip the rothe onto its side, startling it awake!");
					
					mtmp->msleeping = 0;
					mtmp->mfrozen = 1;
				}
			}
		} else if (mtmp->isshk){
			if (!can_respond(mtmp))
				pline("%s doesn't react.", Monnam(mtmp));
#ifdef GOLDOBJ
			else if (money_cnt(invent))
#else
			else if (u.ugold){
#endif
				You("offer %s a tip for their excellent service.", Monnam(mtmp));
				if (strcmp(shkname(mtmp), "Izchak") == 0){
					pline("Izchak thanks you for your generous tip.");
					domonnoise(mtmp, TRUE);
				}
#ifdef GOLDOBJ
				money2mon(mtmp,1);
#else
				u.ugold -= 1;
				mtmp->mgold += 1;
#endif
			} else
				You("have no cash on you right now.");
		} else if (mtmp->mtyp == PM_LEPRECHAUN){
			if (!can_respond(mtmp))
				pline("%s doesn't react.", Monnam(mtmp));
#ifdef GOLDOBJ
			else if (!money_cnt(invent))
#else
			else if (!u.ugold)
#endif
				You("have no money to bribe with!");
			else {
				You("offer %s some gold to leave you alone.", mon_nam(mtmp));
				if (mtmp->mpeaceful)
					pline("%s accepts before you can change your mind.", Monnam(mtmp));
				else
					pline("%s greedily accepts.", Monnam(mtmp));
#ifdef GOLDOBJ
				int cash = min(money_cnt(invent), rnd(10));
				money2mon(mtmp, cash);
#else
				int cash = min(u.ugold, rnd(10));
				u.ugold -= cash;
				mtmp->mgold += cash;
#endif
				mtmp->mpeaceful = TRUE;
				
				if (!tele_restrict(mtmp)) {
					(void)rloc(mtmp, TRUE);
					if (canspotmon(mtmp))
						pline("%s suddenly disappears!", Monnam(mtmp));
				}
			}
		} else if (mtmp->mtyp == PM_MIGO_PHILOSOPHER){
#ifdef GOLDOBJ
			if (money_cnt(invent)){
#else
			if (u.ugold){
#endif
				You("offer %s a penny for their thoughts.", mon_nam(mtmp));
#ifdef GOLDOBJ
				money2mon(mtmp,1);
#else
				u.ugold -= 1;
				mtmp->mgold += 1;
#endif
				if (can_respond(mtmp))
					domonnoise(mtmp, TRUE);
			}
		} else if (is_mercenary(mtmp->data)){
			pline("If you want to bribe them, maybe try throwing them some gold instead.");
		} else {
			pline("That's a silly monster to tip.");
		}
		return MOVE_STANDARD;
			
	} 
	return MOVE_CANCELLED;
}

static void
tipcontainer(box)
struct obj *box; /* or bag */
{
	xchar ox = u.ux, oy = u.uy; /* #tip only works at hero's location */
	boolean empty_it = FALSE, maybeshopgoods;

	/* box is either held or on floor at hero's spot; no need to check for
	   nesting; when held, we need to update its location to match hero's;
	   for floor, the coordinate updating is redundant */
	if (get_obj_location(box, &ox, &oy, 0))
		box->ox = ox, box->oy = oy;

	/* Shop handling:  can't rely on the container's own unpaid
	   or no_charge status because contents might differ with it.
	   A carried container's contents will be flagged as unpaid
	   or not, as appropriate, and need no special handling here.
	   Items owned by the hero get sold to the shop without
	   confirmation as with other uncontrolled drops.  A floor
	   container's contents will be marked no_charge if owned by
	   hero, otherwise they're owned by the shop.  By passing
	   the contents through shop billing, they end up getting
	   treated the same as in the carried case.   We do so one
	   item at a time instead of doing whole container at once
	   to reduce the chance of exhausting shk's billing capacity. */
	maybeshopgoods = !carried(box) && costly_spot(box->ox, box->oy);

	/* caveat: this assumes that cknown, lknown, olocked, and otrapped
	   fields haven't been overloaded to mean something special for the
	   non-standard "container" horn of plenty */
	if (box->otyp == MAGIC_CHEST && box->obolted){
		pline("You can't tip something bolted down!");
		return;
	}
	char yourbuf[BUFSZ];
	if (box->olocked) {
		pline("It's locked.");
	} else if (box->otrapped) {
		/* we're not reaching inside but we're still handling it... */
		(void) chest_trap(box, HAND, FALSE);
		/* even if the trap fails, you've used up this turn */
		if (multi >= 0) { /* in case we didn't become paralyzed */
			nomul(-1, "tipping a container");
		}
	} else if (box->otyp == BAG_OF_TRICKS || box->otyp == HORN_OF_PLENTY) {
		boolean bag = box->otyp == BAG_OF_TRICKS;
		int old_spe = box->spe, seen = 0;

		if (maybeshopgoods && !box->no_charge)
			addtobill(box, FALSE, FALSE, TRUE);
		/* apply this bag/horn until empty or monster/object creation fails
		   (if the latter occurs, force the former...) */
		do {
			if (!(bag ? bagotricks(box, TRUE, &seen) : hornoplenty(box, TRUE))) 
				break;
		} while (box->spe > 0);

		if (box->spe < old_spe) {
			if (bag)
				pline((seen == 0) ? "Nothing seems to happen."
								  : (seen == 1) ? "A monster suddenly appears!"
												: "Monsters suddenly appear!");
			/* check_unpaid wants to see a non-zero charge count */
			box->spe = old_spe;
			check_unpaid_usage(box, TRUE);
			box->spe = 0; /* empty */
		}
		if (maybeshopgoods && !box->no_charge)
			subfrombill(box, shop_keeper(*in_rooms(ox, oy, SHOPBASE)));
	} else if (box->otyp == BOX && box->spe == 1) {
		observe_quantum_cat(box, TRUE);
		if (!Has_contents(box)) /* evidently a live cat came out */
			/* container type of "large box" is inferred */
			pline("%sbox is now empty.", Shk_Your(yourbuf, box));
		else /* holds cat corpse */
			empty_it = TRUE;
	} else if(Is_real_container(box) && box->spe == 4){
		open_coffin(box, FALSE); //FALSE: the box was not destroyed. Use present tense.

		if (!Has_contents(box))
			pline("%scoffin is now empty.", Shk_Your(yourbuf, box));
		else
			empty_it = TRUE;
	} else if(Is_real_container(box) && box->spe == 5){
		open_sarcophagus(box, FALSE); //FALSE: the box was not destroyed. Use present tense.

		if (!Has_contents(box))
			pline("%ssarcophagus is now empty.", Shk_Your(yourbuf, box));
		else
			empty_it = TRUE;
	} else if(Is_real_container(box) && box->spe == 6 && Insight >= 10){
		open_crazy_box(box, TRUE);

		if (!Has_contents(box))
			pline("%s%s is now empty.", Shk_Your(yourbuf, box), simple_typename(box->otyp));
		else
			empty_it = TRUE;
	} else if(Is_real_container(box) && box->spe == 7){
		// Madman reclaims their stuff. Contents handled by the level loader.
		//FALSE: the box was not destroyed. Use present tense.
		open_madstuff_box(box, TRUE);

		if (!Has_contents(box))
			pline("%s%s is now empty.", Shk_Your(yourbuf, box), simple_typename(box->otyp));
		else
			empty_it = TRUE;
	} else if(Is_real_container(box) && box->spe == 8){
		// Nothing. Fulvous desk spawns monsters.
		if (!Has_contents(box))
			pline("It's empty.");
		else
			empty_it = TRUE;
	} else if(Is_real_container(box) && box->spe == 9){
		open_giants_sack(box, TRUE);

		if (!Has_contents(box))
			pline("%s%s is now empty.", Shk_Your(yourbuf, box), simple_typename(box->otyp));
		else
			empty_it = TRUE;
	} else if (!Has_contents(box)) {
		pline("It's empty.");
	} else {
		empty_it = TRUE;
	}

	if (empty_it) {
		struct obj *otmp, *nobj;
		boolean terse, highdrop = !can_reach_floor(),
				altarizing = IS_ALTAR(levl[ox][oy].typ),
				cursed_mbag = (Is_mbag(box) && box->cursed);
		int held = carried(box);
		long loss = 0L;

		if (u.uswallow)
			highdrop = altarizing = FALSE;
		terse = !(highdrop || altarizing || costly_spot(box->ox, box->oy));
		/* Terse formatting is
		 * "Objects spill out: obj1, obj2, obj3, ..., objN."
		 * If any other messages intervene between objects, we revert to
		 * "ObjK drops to the floor.", "ObjL drops to the floor.", &c.
		 */
		pline("%s out%c",
			  box->cobj->nobj ? "Objects spill" : "An object spills",
			  terse ? ':' : '.');
		for (otmp = box->cobj; otmp; otmp = nobj) {
			nobj = otmp->nobj;
			obj_extract_self(otmp);
			otmp->ox = box->ox, otmp->oy = box->oy;

			if (box->otyp == ICE_BOX){
				if (!age_is_relative(otmp)) {
					otmp->age = monstermoves - otmp->age; /* actual age */
					if (otmp->otyp == CORPSE)
						start_corpse_timeout(otmp);
				}
			}
			else if (cursed_mbag && !rn2(13)) {
				loss += mbag_item_gone(held, otmp);
				/* abbreviated drop format is no longer appropriate */
				terse = FALSE;
				continue;
			}

			if (maybeshopgoods && !otmp->no_charge)
				addtobill(otmp, FALSE, FALSE, TRUE);;

			if (highdrop) {
				/* might break or fall down stairs; handles altars itself */
				bhitpos.x = u.ux; bhitpos.y = u.uy;
				otmp->ox = u.ux; otmp->oy = u.uy;
				hitfloor2(&youmonst, &otmp, (struct obj *)0, FALSE, FALSE);
			} else {
				if (altarizing) {
					doaltarobj(otmp, god_at_altar(ox, oy));
				} else if (!terse) {
					pline("%s %s to the %s.", Doname2(otmp),
						  otense(otmp, "drop"), surface(ox, oy));
				} else {
					pline("%s%c", doname(otmp), nobj ? ',' : '.');
				}
				dropy(otmp);
			}
		}
		if (loss) /* magic bag lost some shop goods */
			You("owe %ld %s for lost merchandise.", loss, currency(loss));
		box->owt = weight(box); /* mbag_item_gone() doesn't update this */
		if (held)
			(void) encumber_msg();
	}
	if (carried(box)) /* box is now empty with cknown set */
		update_inventory();
}

int
tiphat()
{
	struct monst *mtmp;
	struct obj *otmp;
	int x, y, range, glyph, vismon, unseen, statue, res;

	if (!uarmh) /* can't get here from there */
		return MOVE_CANCELLED;

	res = uarmh->bknown ? MOVE_INSTANT : MOVE_STANDARD;
	if (cursed(uarmh)) /* "You can't.  It is cursed." */
		return res; /* if learned of curse, use a move */

	/* might choose a position, but dealing with direct lines is simpler */
	if (!getdir("At whom? (in what direction)")) /* bail on ESC */
		return res; /* iffy; now know it's not cursed for sure (since we got
					 * past prior test) but might have already known that */
	res = MOVE_STANDARD; /* physical action is going to take place */

	/* most helmets have a short wear/take-off delay and we could set
	   'multi' to account for that, but we'll pretend that no extra time
	   beyond the current move is necessary */
	
	char * term = (uarmh && !is_metallic(uarmh)) ? "hat" : "helm";
	You("briefly doff your %s.", term);

	if (!u.dx && !u.dy) {
		if (u.usteed && u.dz > 0) {
			if (!u.usteed->mcanmove || u.usteed->msleeping)
				pline("%s doesn't notice.", Monnam(u.usteed));
			else
				(void) domonnoise(u.usteed, TRUE);
		} else if (u.dz) {
			pline("There's no one %s there.", (u.dz < 0) ? "up" : "down");
		} else {
			pline_The("lout here doesn't acknowledge you...");
		}
		return res;
	}

	mtmp = (struct monst *) 0;
	vismon = unseen = statue = 0;
	x = u.ux, y = u.uy;
	for (range = 1; range <= BOLT_LIM + 1; ++range) {
		x += u.dx, y += u.dy;
		if (!isok(x, y) || (range > 1 && !couldsee(x, y))) {
			/* switch back to coordinates for previous interation's 'mtmp' */
			x -= u.dx, y -= u.dy;
			break;
		}
		mtmp = m_at(x, y);
		vismon = (mtmp && canseemon(mtmp));
		if (vismon || (range == 1 && mtmp && can_respond(mtmp) && x == mtmp->mx && y == mtmp->my && !is_silent(mtmp->data)))
			break;
	}
	
	// statue interactions removed, since they make no sense when the obj has a randomized
	// glyph, and it would be annoying if a statue is between you and a target
	
	if (!mtmp || (mtmp && (!can_respond(mtmp) || (x != mtmp->mx || y != mtmp->my)))) {
		if (vismon)
			pline("%s seems not to notice you.", Monnam(mtmp));
		else
			goto nada;
	} else { // at this point, we have mtmp
		mtmp->mstrategy &= ~STRAT_WAITMASK;

		if (vismon && humanoid(mtmp->data) && mtmp->mpeaceful && !Conflict) {
			otmp = which_armor(mtmp, W_ARMH);
			term = (otmp && !is_metallic(otmp)) ? "hat" : "helm";
			if (otmp == 0) {
				pline("%s waves.", Monnam(mtmp));
			} else if (otmp->cursed && !is_weldproof_mon(mtmp)) {
				pline("%s grasps %s %s but can't remove it.", Monnam(mtmp),
					  mhis(mtmp), term);
				otmp->bknown = 1;
			} else if (otmp->otyp == STATUE && (otmp->corpsenm == PM_PARASITIC_MIND_FLAYER || otmp->corpsenm == PM_PARASITIC_MASTER_MIND_FLAYER)){
				pline("%s grasps the statue in %s face but can't remove it.", Monnam(mtmp), mhis(mtmp));
				otmp->bknown = 1;
			} else {
				pline("%s tips %s %s in response.", Monnam(mtmp),
					  mhis(mtmp), term);
			}
		} else if (vismon && humanoid(mtmp->data)) {
			static const char *const reaction[3] = {
				"curses", "gestures rudely", "gestures offensively",
			};
			int which = rn2(3);
			int twice = (rn2(3) || which > 0) ? 0 : rn1(2, 1);
			pline("%s %s%s%s at you...", Monnam(mtmp), reaction[which],
				  twice ? " and " : "", twice ? reaction[twice] : "");
		} else if (distu(x, y) <= 2 && domonnoise(mtmp, TRUE)) {
			if (!vismon)
				map_invisible(x, y);
		} else if (vismon) {
			pline("%s doesn't respond.", Monnam(mtmp));
		} else {
nada:
			pline("%s", nothing_happens);
		}
	}
#undef can_respond
	return res;
}


/*pickup.c*/
