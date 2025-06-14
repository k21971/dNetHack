/*	SCCS Id: @(#)invent.c	3.4	2003/12/02	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include <math.h>
#include "hack.h"
#include "artifact.h"

#define NOINVSYM	'#'
#define CONTAINED_SYM	'>'	/* designator for inside a container */

#ifdef OVL1
STATIC_DCL void NDECL(reorder_invent);
STATIC_DCL boolean FDECL(mergable,(struct obj *,struct obj *));
STATIC_DCL void FDECL(invdisp_nothing, (const char *,const char *));
STATIC_DCL boolean FDECL(worn_wield_only, (struct obj *, int));
STATIC_DCL boolean FDECL(only_here, (struct obj *, int));
#endif /* OVL1 */
STATIC_DCL void FDECL(compactify,(char *));
STATIC_DCL boolean FDECL(taking_off, (const char *));
STATIC_DCL boolean FDECL(putting_on, (const char *));
STATIC_PTR int FDECL(ckunpaid,(struct obj *));
STATIC_PTR int FDECL(ckvalidcat,(struct obj *));
#ifdef DUMP_LOG
static char FDECL(display_pickinv,
		 (const char *,BOOLEAN_P, long *, BOOLEAN_P, BOOLEAN_P));
#else
static char FDECL(display_pickinv, (const char *,BOOLEAN_P, long *));
#endif /* DUMP_LOG */
#ifdef OVLB
STATIC_DCL boolean FDECL(this_type_only, (struct obj *, int));
STATIC_DCL void NDECL(dounpaid);
STATIC_DCL struct obj *FDECL(find_unpaid,(struct obj *,struct obj **));
STATIC_DCL void FDECL(menu_identify, (int));
STATIC_DCL boolean FDECL(tool_in_use, (struct obj *));
#endif /* OVLB */
STATIC_DCL char FDECL(obj_to_let,(struct obj *));
STATIC_PTR int FDECL(u_material_next_to_skin,(int));
STATIC_PTR int FDECL(u_bcu_next_to_skin,(int));
STATIC_PTR int FDECL(mon_material_next_to_skin,(struct monst *, int));
STATIC_PTR int FDECL(mon_bcu_next_to_skin,(struct monst *, int));
STATIC_DCL int FDECL(itemactions,(struct obj *));

#ifdef OVLB

static int lastinvnr = 51;	/* 0 ... 51 (never saved&restored) */

#ifdef WIZARD
/* wizards can wish for venom, which will become an invisible inventory
 * item without this.  putting it in inv_order would mean venom would
 * suddenly become a choice for all the inventory-class commands, which
 * would probably cause mass confusion.  the test for inventory venom
 * is only WIZARD and not wizard because the wizard can leave venom lying
 * around on a bones level for normal players to find.
 */
static char venom_inv[] = { VENOM_CLASS, 0 };	/* (constant) */
#endif

void
assigninvlet(otmp)
register struct obj *otmp;
{
	boolean inuse[52];
	register int i;
	register struct obj *obj;

#ifdef GOLDOBJ
        /* There is only one of these in inventory... */        
        if (otmp->oclass == COIN_CLASS) {
	    otmp->invlet = GOLD_SYM;
	    return;
	}
#endif

	for(i = 0; i < 52; i++) inuse[i] = FALSE;
	for(obj = invent; obj; obj = obj->nobj) if(obj != otmp) {
		i = obj->invlet;
		if('a' <= i && i <= 'z') inuse[i - 'a'] = TRUE; else
		if('A' <= i && i <= 'Z') inuse[i - 'A' + 26] = TRUE;
		if(i == otmp->invlet) otmp->invlet = 0;
	}
	if((i = otmp->invlet) &&
	    (('a' <= i && i <= 'z') || ('A' <= i && i <= 'Z')))
		return;
	for(i = lastinvnr+1; i != lastinvnr; i++) {
		if(i >= 52) { i = -1; continue; }
		if(!inuse[i]) break;
	}
	otmp->invlet = ((i >= 52 || inuse[i]) ? NOINVSYM :
			(i < 26) ? ('a'+i) : ('A'+i-26));
	lastinvnr = i % 52;
}

#endif /* OVLB */
#ifdef OVL1

/* note: assumes ASCII; toggling a bit puts lowercase in front of uppercase */
#define inv_rank(o) ((o)->invlet ^ 040)

/* sort the inventory; used by addinv() and doorganize() */
STATIC_OVL void
reorder_invent()
{
	struct obj *otmp, *prev, *next;
	boolean need_more_sorting;

	do {
	    /*
	     * We expect at most one item to be out of order, so this
	     * isn't nearly as inefficient as it may first appear.
	     */
	    need_more_sorting = FALSE;
	    for (otmp = invent, prev = 0; otmp; ) {
		next = otmp->nobj;
		if (next && inv_rank(next) < inv_rank(otmp)) {
		    need_more_sorting = TRUE;
		    if (prev) prev->nobj = next;
		    else      invent = next;
		    otmp->nobj = next->nobj;
		    next->nobj = otmp;
		    prev = next;
		} else {
		    prev = otmp;
		    otmp = next;
		}
	    }
	} while (need_more_sorting);
}

#undef inv_rank

/* scan a list of objects to see whether another object will merge with
   one of them; used in pickup.c when all 52 inventory slots are in use,
   to figure out whether another object could still be picked up */
struct obj *
merge_choice(objlist, obj)
struct obj *objlist, *obj;
{
	struct monst *shkp;
	int save_nocharge;

	if (obj->otyp == SCR_SCARE_MONSTER)	/* punt on these */
	    return (struct obj *)0;
	/* if this is an item on the shop floor, the attributes it will
	   have when carried are different from what they are now; prevent
	   that from eliciting an incorrect result from mergable() */
	save_nocharge = obj->no_charge;
	if (objlist == invent && obj->where == OBJ_FLOOR &&
		(shkp = shop_keeper(inside_shop(obj->ox, obj->oy))) != 0) {
	    if (obj->no_charge) obj->no_charge = 0;
	    /* A billable object won't have its `unpaid' bit set, so would
	       erroneously seem to be a candidate to merge with a similar
	       ordinary object.  That's no good, because once it's really
	       picked up, it won't merge after all.  It might merge with
	       another unpaid object, but we can't check that here (depends
	       too much upon shk's bill) and if it doesn't merge it would
	       end up in the '#' overflow inventory slot, so reject it now. */
	    else if (inhishop(shkp)) return (struct obj *)0;
	}
	while (objlist) {
	    if (mergable(objlist, obj)) break;
	    objlist = objlist->nobj;
	}
	obj->no_charge = save_nocharge;
	return objlist;
}

/* merge obj with otmp and delete obj if types agree */
int
merged(potmp, pobj)
struct obj **potmp, **pobj;
{
	register struct obj *otmp = *potmp, *obj = *pobj;

	if(mergable(otmp, obj)) {
		/* Approximate age: we do it this way because if we were to
		 * do it "accurately" (merge only when ages are identical)
		 * we'd wind up never merging any corpses.
		 * otmp->age = otmp->age*(1-proportion) + obj->age*proportion;
		 *
		 * Don't do the age manipulation if lit.  We would need
		 * to stop the burn on both items, then merge the age,
		 * then restart the burn.
		 */
		if(obj->otyp == CORPSE && otmp->otyp == CORPSE) otmp->ovar1_corpseRumorCooldown = max(otmp->ovar1_corpseRumorCooldown,obj->ovar1_corpseRumorCooldown);
		if (!obj->lamplit)
		    otmp->age = ((otmp->age*otmp->quan) + (obj->age*obj->quan))
			    / (otmp->quan + obj->quan);

		otmp->quan += obj->quan;
		
		otmp->rknown |= obj->rknown;
		otmp->dknown |= obj->dknown;
#ifdef GOLDOBJ
                /* temporary special case for gold objects!!!! */
#endif
		if (otmp->oclass == COIN_CLASS) otmp->owt = weight(otmp);
		else otmp->owt += obj->owt;
		if(!get_ox(otmp, OX_ENAM) && get_ox(obj, OX_ENAM))
			otmp = *potmp = oname(otmp, ONAME(obj));
		obj_extract_self(obj);

		/* really should merge the timeouts */
		if (obj->lamplit) obj_merge_light_sources(obj, otmp);
		if (obj->timed) stop_all_timers(obj->timed);	/* follows lights */

		/* fixup for `#adjust' merging wielded darts, daggers, &c */
		if (obj->owornmask && carried(otmp)) {
		    long wmask = otmp->owornmask | obj->owornmask;

		    /* Both the items might be worn in competing slots;
		       merger preference (regardless of which is which):
			 primary weapon + alternate weapon -> primary weapon;
			 primary weapon + quiver -> primary weapon;
			 alternate weapon + quiver -> alternate weapon.
		       (Prior to 3.3.0, it was not possible for the two
		       stacks to be worn in different slots and `obj'
		       didn't need to be unworn when merging.) */
		    if (wmask & W_WEP) wmask = W_WEP;
		    else if (wmask & W_SWAPWEP) wmask = W_SWAPWEP;
		    else if (wmask & W_QUIVER) wmask = W_QUIVER;
		    else {
			impossible("merging strangely worn items (%lx)", wmask);
			wmask = otmp->owornmask;
		    }
		    if ((otmp->owornmask & ~wmask) != 0L) setnotworn(otmp);
		    setworn(otmp, wmask);
		    setnotworn(obj);
		}
#if 0
		/* (this should not be necessary, since items
		    already in a monster's inventory don't ever get
		    merged into other objects [only vice versa]) */
		else if (obj->owornmask && mcarried(otmp)) {
		    if (obj == MON_WEP(otmp->ocarry)) {
			MON_WEP(otmp->ocarry) = otmp;
			otmp->owornmask = W_WEP;
			update_mon_intrinsics(mon, otmp, TRUE, FALSE);
		    }
		}
#endif /*0*/

		obfree(obj,otmp);	/* free(obj), bill->otmp */
		return(1);
	}
	return 0;
}

/*
Adjust hero intrinsics as if this object was being added to the hero's
inventory.  Called _before_ the object has been added to the hero's
inventory.

This is called when adding objects to the hero's inventory normally (via
addinv) or when an object in the hero's inventory has been polymorphed
in-place.

It may be valid to merge this code with with addinv_core2().
*/
void
addinv_core1(obj)
struct obj *obj;
{
	if (obj->oclass == COIN_CLASS) {
#ifndef GOLDOBJ
		u.ugold += obj->quan;
#else
		flags.botl = 1;
#endif
	} else if (obj->otyp == AMULET_OF_YENDOR) {
		if (u.uhave.amulet) impossible("already have amulet?");
		u.uhave.amulet = 1;
#ifdef RECORD_ACHIEVE
		achieve.get_amulet = 1;
#endif
	} else if (obj->otyp == CANDELABRUM_OF_INVOCATION) {
		if (u.uhave.menorah) impossible("already have candelabrum?");
		u.uhave.menorah = 1;
#ifdef RECORD_ACHIEVE
		achieve.get_candelabrum = 1;
#endif
	} else if (obj->otyp == BELL_OF_OPENING || obj->oartifact == ART_ANNULUS) {
		if (u.uhave.bell) impossible("already have silver bell?");
		u.uhave.bell = 1;
#ifdef RECORD_ACHIEVE
		achieve.get_bell = 1;
		give_quest_trophy();
#endif
	} else if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
		if (u.uhave.book) impossible("already have the book?");
		u.uhave.book = 1;
#ifdef RECORD_ACHIEVE
		achieve.get_book = 1;
#endif
	}
	if (obj->oartifact) {
		if (is_quest_artifact(obj)) {
		    if (u.uhave.questart){
				struct obj *otherquestart;
				for(otherquestart = invent; otherquestart; otherquestart = otherquestart->nobj)
					if(is_quest_artifact(otherquestart))
						break;
				if(!otherquestart) impossible("already have quest artifact?");
			}
		    u.uhave.questart = 1;
		    artitouch();
		}
		if(obj->oartifact == ART_TREASURY_OF_PROTEUS){
			u.ukinghill = TRUE;
//ifdef RECORD_ACHIEVE Record_achieve now mission-critical for Binder, so....
		} else if(obj->oartifact == ART_SILVER_KEY){
			achieve.get_skey = TRUE;
		} else if(obj->oartifact >= ART_FIRST_KEY_OF_LAW && obj->oartifact <= ART_THIRD_KEY_OF_NEUTRALITY){
			achieve.get_keys |= (1 << (obj->oartifact - ART_FIRST_KEY_OF_LAW));
//endif
		}
		if(Role_if(PM_EXILE) && 
				(achieve.get_keys&0x002) && //Second key #1 (was 0x007)
				(achieve.get_keys&0x010) && //Second key #2 (was 0x038)
				(achieve.get_keys&0x080) && //Second key #3 (was 0x1C0)
				!(u.specialSealsKnown&SEAL_ALIGNMENT_THING)
		){
			You("realize that, taken together, the patterns on the three keys form a seal!");
			u.specialSealsKnown |= SEAL_ALIGNMENT_THING;
		}
		set_artifact_intrinsic(obj, 1, W_ART);
	}

	/* Picking up dead bodies increases impurity */
	if (obj->otyp == CORPSE && !vegan(&mons[obj->corpsenm]) && !obj->invlet){
		IMPURITY_UP(u.uimp_bodies)
	}

#ifdef RECORD_ACHIEVE
	if(obj->otyp == LUCKSTONE && obj->record_achieve_special) {
			achieve.get_luckstone = 1;
			obj->record_achieve_special = 0;
	} else if((obj->otyp == AMULET_OF_REFLECTION ||
			   obj->otyp == BAG_OF_HOLDING) &&
			  obj->record_achieve_special) {
			achieve.finish_sokoban = 1;
			obj->record_achieve_special = 0;
			livelog_write_string("completed Sokoban");
	}
	if(obj->otyp == CLOAK_OF_MAGIC_RESISTANCE && obj->record_achieve_special){
		obj->record_achieve_special = 0;
		give_law_trophy();
	}
#endif /* RECORD_ACHIEVE */

}

/*
Adjust hero intrinsics as if this object was being added to the hero's
inventory.  Called _after_ the object has been added to the hero's
inventory.

This is called when adding objects to the hero's inventory normally (via
addinv) or when an object in the hero's inventory has been polymorphed
in-place.
*/
void
addinv_core2(obj)
struct obj *obj;
{
	if (confers_luck(obj)) {
		/* new luckstone must be in inventory by this point
		 * for correct calculation */
		set_moreluck();
	}
}

/*
Add obj to the hero's inventory.  Make sure the object is "free".
Adjust hero attributes as necessary.
*/
struct obj *
addinv(obj)
struct obj *obj;
{
	struct obj *otmp, *prev;

	if (obj->where != OBJ_FREE)
	    panic("addinv: obj not free");
	obj->no_charge = 0;	/* not meaningful for invent */

	addinv_core1(obj);
#ifndef GOLDOBJ
	/* if handed gold, we're done */
	if (obj->oclass == COIN_CLASS)
	    return obj;
#endif

	/* merge if possible; find end of chain in the process */
	for (prev = 0, otmp = invent; otmp; prev = otmp, otmp = otmp->nobj)
	    if (merged(&otmp, &obj)) {
		obj = otmp;
		goto added;
	    }
	/* didn't merge, so insert into chain */
	if (flags.invlet_constant || !prev) {
	    if (flags.invlet_constant) assigninvlet(obj);
	    obj->nobj = invent;		/* insert at beginning */
	    invent = obj;
	    if (flags.invlet_constant) reorder_invent();
	} else {
	    prev->nobj = obj;		/* insert at end */
	    obj->nobj = 0;
	}
	obj->where = OBJ_INVENT;

added:
	addinv_core2(obj);
	carry_obj_effects(obj);		/* carrying affects the obj */
	update_inventory();
	return(obj);
}

/*
 * Some objects are affected by being carried.
 * Make those adjustments here. Called _after_ the object
 * has been added to the hero's or monster's inventory,
 * and after hero's intrinsics have been updated.
 */
void
carry_obj_effects(obj)
struct obj *obj;
{
	/* Cursed figurines can spontaneously transform
	   when carried. */
	if (obj->otyp == FIGURINE) {
		if (obj->cursed
	    	    && obj->corpsenm != NON_PM
	    	    && !dead_species(obj->corpsenm,TRUE)) {
			attach_fig_transform_timeout(obj);
		    }
	}
}

#endif /* OVL1 */
#ifdef OVLB

/* Add an item to the inventory unless we're fumbling or it refuses to be
 * held (via touch_artifact), and give a message.
 * If there aren't any free inventory slots, we'll drop it instead.
 * If both success and failure messages are NULL, then we're just doing the
 * fumbling/slot-limit checking for a silent grab.  In any case,
 * touch_artifact will print its own messages if they are warranted.
 */
struct obj *
hold_another_object(obj, drop_fmt, drop_arg, hold_msg)
struct obj *obj;
const char *drop_fmt, *drop_arg, *hold_msg;
{
	char buf[BUFSZ];

	if (!Blind || (obj->oclass == SCROLL_CLASS && check_oprop(obj, OPROP_TACTB))) obj->dknown = 1;	/* maximize mergibility */
	if (obj->oartifact) {
	    /* place_object may change these */
	    boolean crysknife = (obj->otyp == CRYSKNIFE);
	    int oerode = obj->oerodeproof;
	    boolean wasUpolyd = Upolyd;

	    /* in case touching this object turns out to be fatal */
	    place_object(obj, u.ux, u.uy);

	    if (!touch_artifact(obj, &youmonst, FALSE)) {
		obj_extract_self(obj);	/* remove it from the floor */
		dropy(obj);		/* now put it back again :-) */
		return obj;
	    } else if (wasUpolyd && !Upolyd) {
		/* loose your grip if you revert your form */
		if (drop_fmt) pline(drop_fmt, drop_arg);
		obj_extract_self(obj);
		dropy(obj);
		return obj;
	    }
	    obj_extract_self(obj);
	    if (crysknife) {
		obj->otyp = CRYSKNIFE;
		obj->oerodeproof = oerode;
	    }
	}
	if (Fumbling) {
	    if (drop_fmt) pline(drop_fmt, drop_arg);
	    dropy(obj);
	} else {
	    long oquan = obj->quan;
	    int prev_encumbr = near_capacity();	/* before addinv() */

	    /* encumbrance only matters if it would now become worse
	       than max( current_value, stressed ) */
	    if (prev_encumbr < MOD_ENCUMBER) prev_encumbr = MOD_ENCUMBER;
	    /* addinv() may redraw the entire inventory, overwriting
	       drop_arg when it comes from something like doname() */
	    if (drop_arg) drop_arg = strcpy(buf, drop_arg);

	    obj = addinv(obj);
	    if (inv_cnt() > 52
		    || ((obj->otyp != LOADSTONE || !obj->cursed)
			&& near_capacity() > prev_encumbr)) {
		if (drop_fmt) pline(drop_fmt, drop_arg);
		/* undo any merge which took place */
		if (obj->quan > oquan) obj = splitobj(obj, oquan);
		dropx(obj);
	    } else {
		if (flags.autoquiver && !uquiver && !obj->owornmask &&
			(is_missile(obj) ||
			    ammo_and_launcher(obj, uwep) ||
			    ammo_and_launcher(obj, uswapwep)))
		    setuqwep(obj);
		if (hold_msg || drop_fmt) prinv(hold_msg, obj, oquan);
	    }
	}
	return obj;
}

/* useup() all of an item regardless of its quantity */
void
useupall(obj)
struct obj *obj;
{
	boolean gloves = !!(obj->owornmask&W_ARMG);
	setnotworn(obj);
	freeinv(obj);
	obfree(obj, (struct obj *)0);	/* deletes contents also */
	if(gloves)
		selftouch("With your hand-protection gone, you");
}

void
useup(obj)
register struct obj *obj;
{
	/*  Note:  This works correctly for containers because they */
	/*	   (containers) don't merge.			    */
	if (obj->quan > 1L) {
		obj->in_use = FALSE;	/* no longer in use */
		obj->quan--;
		obj->owt = weight(obj);
		update_inventory();
	} else {
		useupall(obj);
	}
}

/* use one charge from an item and possibly incur shop debt for it */
void
consume_obj_charge(obj, maybe_unpaid)
struct obj *obj;
boolean maybe_unpaid;	/* false if caller handles shop billing */
{
	if (maybe_unpaid) check_unpaid(obj);
	obj->spe -= 1;
	if (obj->known) update_inventory();
}

#endif /* OVLB */
#ifdef OVL3

/*
Adjust hero's attributes as if this object was being removed from the
hero's inventory.  This should only be called from freeinv() and
where we are polymorphing an object already in the hero's inventory.

Should think of a better name...
*/
void
freeinv_core(obj)
struct obj *obj;
{
	if(u.uentangled_oid == obj->o_id){
		u.uentangled_oid = 0;
		u.uentangled_otyp = 0;
	}
	if (obj->oclass == COIN_CLASS) {
#ifndef GOLDOBJ
		u.ugold -= obj->quan;
		obj->in_use = FALSE;
#endif
		flags.botl = 1;
		return;
	} else if (obj->otyp == AMULET_OF_YENDOR) {
		if (!u.uhave.amulet) impossible("don't have amulet?");
		u.uhave.amulet = 0;
		if (uwep && uwep->oartifact == ART_KUSANAGI_NO_TSURUGI && u.ulevel < 30){
			char buf[BUFSZ];
			You("are blasted by %s power!", s_suffix(the(xname(uwep))));
			Sprintf(buf, "touching %s", artiname(uwep->oartifact));
			losehp(d((Antimagic ? 2 : 4), 10), buf, KILLED_BY);

			setuwep((struct obj *) 0);
			pline("Without the Amulet of Yendor, you are no longer worthy of wielding this sword and must sheathe it.");
		}

	} else if (obj->otyp == CANDELABRUM_OF_INVOCATION) {
		if (!u.uhave.menorah) impossible("don't have candelabrum?");
		u.uhave.menorah = 0;
	} else if (obj->otyp == BELL_OF_OPENING || obj->oartifact == ART_ANNULUS) {
		if (!u.uhave.bell) impossible("don't have silver bell?");
		u.uhave.bell = 0;
	} else if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
		if (!u.uhave.book) impossible("don't have the book?");
		u.uhave.book = 0;
	}
	if (obj->oartifact) {
		if (is_quest_artifact(obj)) {
			struct obj *otherquestart;
		    if (!u.uhave.questart)
				impossible("don't have quest artifact?");
			for(otherquestart = invent; otherquestart; otherquestart = otherquestart->nobj)
				if(is_quest_artifact(otherquestart))
					break;
			if(!otherquestart) u.uhave.questart = 0;
		}
		if(obj->oartifact == ART_TREASURY_OF_PROTEUS){
			u.ukinghill = FALSE;
		}
		set_artifact_intrinsic(obj, 0, W_ART);
	}

	if (obj->otyp == LOADSTONE) {
		curse(obj);
	} else if (confers_luck(obj)) {
		set_moreluck();
		flags.botl = 1;
	} else if (obj->otyp == FIGURINE && obj->timed) {
		(void) stop_timer(FIG_TRANSFORM, obj->timed);
	}
}

/* remove an object from the hero's inventory */
void
freeinv(obj)
register struct obj *obj;
{
	extract_nobj(obj, &invent);
	freeinv_core(obj);
	update_inventory();
}

/* remove an object from a monster's inventory */
void
m_freeinv(obj)
struct obj* obj;
{
	if(obj->ocarry->entangled_oid == obj->o_id){
		obj->ocarry->entangled_oid = 0;
		obj->ocarry->entangled_otyp = 0;
	}
	extract_nobj(obj, &obj->ocarry->minvent);
	update_mon_intrinsics(obj->ocarry, obj, FALSE, FALSE);
	return;
}


void
delallobj(x, y)
int x, y;
{
	struct obj *otmp, *otmp2;

	for (otmp = level.objects[x][y]; otmp; otmp = otmp2) {
		if (otmp == uball)
			unpunish();
		/* after unpunish(), or might get deallocated chain */
		otmp2 = otmp->nexthere;
		if (otmp == uchain)
			continue;
		delobj(otmp);
	}
}

#endif /* OVL3 */
#ifdef OVL2

/* destroy object in fobj chain (if unpaid, it remains on the bill) */
void
delobj(obj)
register struct obj *obj;
{
	boolean update_map;

	if (is_asc_obj(obj)) {
		/* player might be doing something stupid, but we
		 * can't guarantee that.  assume special artifacts
		 * are indestructible via drawbridges, and exploding
		 * chests, and golem creation, and ...
		 */
		return;
	}

	if (obj == uwep) uwepgone();
	else if (obj == uswapwep) uswapwepgone();
	else if (obj == uquiver) uqwepgone();
	else if (obj == uarm) setnotworn(obj);
	else if (obj == uarmc) setnotworn(obj);
	else if (obj == uarmh) setnotworn(obj);
	else if (obj == uarms) setnotworn(obj);
	else if (obj == uarmg) setnotworn(obj);
#ifdef TOURIST
	else if (obj == uarmu) setnotworn(obj);
#endif
	else if (obj == uarmf) setnotworn(obj);

	update_map = (obj->where == OBJ_FLOOR);
	obj_extract_self(obj);
	if (update_map) newsym(obj->ox, obj->oy);
	obfree(obj, (struct obj *) 0);	/* frees contents also */
}

#endif /* OVL2 */
#ifdef OVL0

struct obj *
sobj_at(n,x,y)
register int n, x, y;
{
	register struct obj *otmp;

	for(otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
		if(otmp->otyp == n)
		    return(otmp);
	return((struct obj *)0);
}

struct obj *
boulder_at(x,y)
register int x, y;
{
	register struct obj *otmp;

	for(otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
		if(	is_boulder(otmp) ) return(otmp);
	return((struct obj *)0);
}

struct obj *
toustefna_at(x,y)
register int x, y;
{
	register struct obj *otmp;

	for(otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
		if(otmp->oclass == WEAPON_CLASS && (otmp)->obj_material == WOOD && (otmp->oward & WARD_TOUSTEFNA))
		    return(otmp);
	return((struct obj *)0);
}

struct obj *
dreprun_at(x,y)
register int x, y;
{
	register struct obj *otmp;

	for(otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
		if(otmp->oclass == WEAPON_CLASS && (otmp)->obj_material == WOOD && (otmp->oward & WARD_DREPRUN))
		    return(otmp);
	return((struct obj *)0);
}

struct obj *
veioistafur_at(x,y)
register int x, y;
{
	register struct obj *otmp;

	for(otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
		if(otmp->oclass == WEAPON_CLASS && (otmp)->obj_material == WOOD && (otmp->oward & WARD_VEIOISTAFUR))
		    return(otmp);
	return((struct obj *)0);
}

struct obj *
thjofastafur_at(x,y)
register int x, y;
{
	register struct obj *otmp;

	for(otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
		if(otmp->oclass == WEAPON_CLASS && (otmp)->obj_material == WOOD && (otmp->oward & WARD_THJOFASTAFUR))
		    return(otmp);
	return((struct obj *)0);
}

struct obj *
fear_arti_at(x,y)
register int x, y;
{
	register struct obj *otmp;

	for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere) {
		if (arti_is_prop(otmp, ARTI_FEAR)
			&&
			(
			(artilist[(int)(otmp)->oartifact].alignment == A_NONE)
				||
			(artilist[(int)(otmp)->oartifact].alignment == u.ualign.type && u.ualign.record > 0)
			)
			)
			return(otmp);
	}
	return((struct obj *)0);
}

#endif /* OVL0 */
#ifdef OVLB

struct obj *
carrying(type)
register int type;
{
	register struct obj *otmp;

	for(otmp = invent; otmp; otmp = otmp->nobj)
		if(otmp->otyp == type)
			return(otmp);
	return((struct obj *) 0);
}

char
carrying_applyable_ring()
{
	register struct obj *otmp;

	for(otmp = invent; otmp; otmp = otmp->nobj)
		if(otmp->oclass == RING_CLASS && (isEngrRing((otmp)->otyp) || isSignetRing((otmp)->otyp)) && otmp->oward)
			return TRUE;
	return FALSE;
}

char
carrying_applyable_amulet()
{
	register struct obj *otmp;

	for(otmp = invent; otmp; otmp = otmp->nobj)
		if(otmp->oclass == AMULET_CLASS && otmp->oward)
			return TRUE;
	return FALSE;
}

/* note: does not include gray stones */
char
carrying_applyable_gem()
{
	register struct obj *otmp;

	for(otmp = invent; otmp; otmp = otmp->nobj)
		if(otmp->otyp == ANTIMAGIC_RIFT
		|| otmp->otyp == CATAPSI_VORTEX
		|| (Role_if(PM_ANACHRONONAUT) && !otmp->oartifact && otmp->otyp == DILITHIUM_CRYSTAL)
		)
			return TRUE;
	return FALSE;
}

char
carrying_readable_weapon()
{
	register struct obj *otmp;

	for(otmp = invent; otmp; otmp = otmp->nobj)
		if((otmp->oclass == WEAPON_CLASS && otmp->obj_material == WOOD && otmp->oward) || 
			(otmp->oartifact && (
				otmp->oartifact == ART_EXCALIBUR || 
				otmp->oartifact == ART_GLAMDRING || 
				otmp->oartifact == ART_ROD_OF_SEVEN_PARTS ||
				otmp->oartifact == ART_BOW_OF_SKADI ||
				otmp->oartifact == ART_STAFF_OF_AESCULAPIUS ||
				otmp->oartifact == ART_GUNGNIR ||
				otmp->oartifact == ART_PEN_OF_THE_VOID ||
				otmp->oartifact == ART_HOLY_MOONLIGHT_SWORD ||
				otmp->oartifact == ART_ESSCOOAHLIPBOOURRR ||
				otmp->oartifact == ART_RED_CORDS_OF_ILMATER ||
				otmp->oartifact == ART_STAFF_OF_NECROMANCY
			))
		)
			return TRUE;
	return FALSE;
}

char
carrying_readable_tool()
{
	register struct obj *otmp;

	for(otmp = invent; otmp; otmp = otmp->nobj)
		if(otmp->otyp == CANDLE_OF_INVOCATION
			|| otmp->otyp == LIGHTSABER
			|| otmp->otyp == BEAMSWORD
			|| otmp->otyp == MISOTHEISTIC_PYRAMID
			|| otmp->otyp == MISOTHEISTIC_FRAGMENT
		)
			return TRUE;
	return FALSE;
}

char
carrying_readable_armor()
{
	register struct obj *otmp;

	for(otmp = invent; otmp; otmp = otmp->nobj)
		if(otmp->oclass == ARMOR_CLASS
			&& ((otmp->ohaluengr
					&& is_readable_armor(otmp)
				) || (otmp->oartifact && (
					otmp->oartifact == ART_ITLACHIAYAQUE
				 )
				)
			   )
		)
			return TRUE;
	return FALSE;
}

struct obj *
carrying_art(artnum)
register int artnum;
{
	register struct obj *otmp;

	for(otmp = invent; otmp; otmp = otmp->nobj)
		if(otmp->oartifact == artnum)
			return(otmp);
	return((struct obj *) 0);
}

const char *
currency(amount)
long amount;
{
	if (amount == 1L) return "zorkmid";
	else return "zorkmids";
}

boolean
have_lizard()
{
	register struct obj *otmp;

	for(otmp = invent; otmp; otmp = otmp->nobj)
		if(otmp->otyp == CORPSE && (otmp->corpsenm == PM_LIZARD || otmp->corpsenm == PM_BABY_CAVE_LIZARD ||  otmp->corpsenm == PM_SMALL_CAVE_LIZARD ||  otmp->corpsenm == PM_CAVE_LIZARD ||  otmp->corpsenm == PM_LARGE_CAVE_LIZARD))
			return(TRUE);
	return(FALSE);
}

struct obj *
o_on(id, objchn)
unsigned int id;
register struct obj *objchn;
{
	struct obj *temp;

	while(objchn) {
		if(objchn->o_id == id) return(objchn);
		if (Has_contents(objchn) && (temp = o_on(id,objchn->cobj)))
			return temp;
		objchn = objchn->nobj;
	}
	return((struct obj *) 0);
}

boolean
obj_here(obj, x, y)
register struct obj *obj;
int x, y;
{
	register struct obj *otmp;

	for(otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
		if(obj == otmp) return(TRUE);
	return(FALSE);
}

#endif /* OVLB */
#ifdef OVL2

struct obj *
g_at(x,y)
register int x, y;
{
	register struct obj *obj = level.objects[x][y];
	while(obj) {
	    if (obj->oclass == COIN_CLASS) return obj;
	    obj = obj->nexthere;
	}
	return((struct obj *)0);
}

#endif /* OVL2 */
#ifdef OVLB
#ifndef GOLDOBJ
/* Make a gold object from the hero's gold. */
struct obj *
mkgoldobj(q)
register long q;
{
	register struct obj *otmp;

	otmp = mksobj(GOLD_PIECE, MKOBJ_NOINIT);
	u.ugold -= q;
	otmp->quan = q;
	otmp->owt = weight(otmp);
	flags.botl = 1;
	return(otmp);
}
#endif
#endif /* OVLB */
#ifdef OVL1

STATIC_OVL void
compactify(buf)
register char *buf;
/* compact a string of inventory letters by dashing runs of letters */
{
	register int i1 = 1, i2 = 1;
	register char ilet, ilet1, ilet2;

	ilet2 = buf[0];
	ilet1 = buf[1];
	buf[++i2] = buf[++i1];
	ilet = buf[i1];
	while(ilet) {
		if(ilet == ilet1+1) {
			if(ilet1 == ilet2+1)
				buf[i2 - 1] = ilet1 = '-';
			else if(ilet2 == '-') {
				buf[i2 - 1] = ++ilet1;
				buf[i2] = buf[++i1];
				ilet = buf[i1];
				continue;
			}
		}
		ilet2 = ilet1;
		ilet1 = ilet;
		buf[++i2] = buf[++i1];
		ilet = buf[i1];
	}
}

/* match the prompt for either 'T' or 'R' command */
STATIC_OVL boolean
taking_off(action)
const char *action;
{
    return !strcmp(action, "take off") || !strcmp(action, "remove");
}

/* match the prompt for either 'W' or 'P' command */
STATIC_OVL boolean
putting_on(action)
const char *action;
{
    return !strcmp(action, "wear") || !strcmp(action, "put on");
}

static struct obj *nextgetobj = 0;

/** Returns the object to use in the inventory usage menu.
 * nextgetobj is set to NULL before the pointer of the item is returned. */
struct obj*
getnextgetobj()
{
	if (nextgetobj) {
		struct obj* ptr = nextgetobj;
		nextgetobj = NULL;
		return ptr;
	}
	return NULL;
}

/*
 * getobj returns:
 *	struct obj *xxx:	object to do something with.
 *	(struct obj *) 0	error return: no object.
 *	&zeroobj		explicitly no object (as in w-).
#ifdef GOLDOBJ
!!!! test if gold can be used in unusual ways (eaten etc.)
!!!! may be able to remove "usegold"
#endif
 */
struct obj *
getobj(let,word)
register const char *let,*word;
{
	register struct obj *otmp;
	register char ilet;
	char buf[BUFSZ], qbuf[QBUFSZ];
	char lets[BUFSZ], altlets[BUFSZ], *ap;
	register int foo = 0;
	register char *bp = buf;
	xchar allowcnt = 0;	/* 0, 1 or 2 */
#ifndef GOLDOBJ
	boolean allowgold = FALSE;	/* can't use gold because they don't have any */
#endif
	boolean usegold = FALSE;	/* can't use gold because its illegal */
	boolean allowall = FALSE;
	boolean allownone = FALSE;
	boolean useboulder = FALSE;
	boolean usethrowing = FALSE;
	boolean usemirror = FALSE;
	boolean phlebot_kit = !!find_object_type(invent, PHLEBOTOMY_KIT);
	xchar foox = 0;
	long cnt;
	boolean prezero = FALSE;
	long dummymask;

	if(nextgetobj) return getnextgetobj();

	if(*let == ALLOW_COUNT) let++, allowcnt = 1;
#ifndef GOLDOBJ
	if(*let == COIN_CLASS) let++,
		usegold = TRUE, allowgold = (u.ugold ? TRUE : FALSE);
#else
	if(*let == COIN_CLASS) let++, usegold = TRUE;
#endif

	/* Equivalent of an "ugly check" for gold */
	if (usegold && !strcmp(word, "eat") &&
	    (!metallivorous(youracedata)
	     || youracedata->mtyp == PM_RUST_MONSTER))
#ifndef GOLDOBJ
		usegold = allowgold = FALSE;
#else
		usegold = FALSE;
#endif

	if(*let == ALL_CLASSES) let++, allowall = TRUE;
	if(*let == ALLOW_NONE) let++, allownone = TRUE;
	/* "ugly check" for reading fortune cookies, part 1 */
	/* The normal 'ugly check' keeps the object on the inventory list.
	 * We don't want to do that for shirts/cookies, so the check for
	 * them is handled a bit differently (and also requires that we set
	 * allowall in the caller)
	 */
	if(allowall && !strcmp(word, "read")) allowall = FALSE;

	/* another ugly check: show boulders (not statues) */
	if(*let == WEAPON_CLASS &&
	   !strcmp(word, "throw") && (throws_rocks(youracedata) || (u.sealsActive&SEAL_YMIR)))
	    useboulder = TRUE;

	if(*let == WEAPON_CLASS && !strcmp(word, "throw"))
	    usethrowing = TRUE;

	if(*let == WEAPON_CLASS && u.specialSealsActive&SEAL_NUDZIRATH)
	    usemirror = TRUE;

	if(allownone) *bp++ = '-';
#ifndef GOLDOBJ
	if(allowgold) *bp++ = def_oc_syms[COIN_CLASS];
#endif
	if(bp > buf && bp[-1] == '-') *bp++ = ' ';
	ap = altlets;
	
	ilet = 'a';
	for (otmp = invent; otmp; otmp = otmp->nobj) {
	    if (!flags.invlet_constant)
#ifdef GOLDOBJ
		if (otmp->invlet != GOLD_SYM) /* don't reassign this */
#endif
		otmp->invlet = ilet;	/* reassign() */
	    if (!*let || index(let, otmp->oclass)
#ifdef GOLDOBJ
		|| (usegold && otmp->invlet == GOLD_SYM)
#endif
		|| (useboulder && is_boulder(otmp))
		|| (usethrowing && (otmp->otyp == ROPE_OF_ENTANGLING || otmp->otyp == BANDS || otmp->otyp == RAZOR_WIRE))
		|| (usemirror && otmp->otyp == MIRROR)
		) {
		register int otyp = otmp->otyp;
		bp[foo++] = otmp->invlet;

		/* ugly check: remove inappropriate things */
		if ((taking_off(word) &&
		    (!(otmp->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL | W_BELT))
		     || (otmp==uarm && uarmc && arm_blocks_upper_body(uarm->otyp))
#ifdef TOURIST
		     || (otmp==uarmu && ((uarm && arm_blocks_upper_body(uarm->otyp)) || uarmc))
#endif
		    ))
		|| (putting_on(word) &&
		     (otmp->owornmask & (W_ARMOR | W_RING | W_AMUL | W_BELT | W_TOOL)))
							/* already worn */
#if 0	/* 3.4.1 -- include currently wielded weapon among the choices */
		|| (!strcmp(word, "wield") &&
		    (otmp->owornmask & W_WEP))
#endif
		|| (!strcmp(word, "ready") &&
		    (otmp == uwep || (otmp == uswapwep && u.twoweap)))
		    ) {
			foo--;
			foox++;
		}

		/* Second ugly check; unlike the first it won't trigger an
		 * "else" in "you don't have anything else to ___".
		 */
		else if ((!strcmp(word, "put on") &&
		    ((otmp->oclass == FOOD_CLASS && otmp->otyp != MEAT_RING) ||
		    (otmp->oclass == TOOL_CLASS &&
		     otyp != BLINDFOLD && otyp != MASK && otyp != R_LYEHIAN_FACEPLATE && 
			 otyp != TOWEL && otyp != ANDROID_VISOR && otyp != LIVING_MASK && otyp != LENSES && otyp != SUNGLASSES && otyp != SOUL_LENS) ||
			 (otmp->oclass == CHAIN_CLASS)
			))
		|| (!strcmp(word, "wield") &&
		    ((otmp->oclass == TOOL_CLASS && !is_weptool(otmp)) ||
			(otmp->oclass == CHAIN_CLASS && otmp->otyp != CHAIN)))
		|| (!strcmp(word, "resize") && !(otmp->oclass == ARMOR_CLASS || otmp->otyp == LENSES || otmp->otyp == SUNGLASSES || otmp->otyp == SOUL_LENS))
		|| (!strcmp(word, "trephinate") && !(otmp->otyp == CRYSTAL_SKULL))
		|| (!strcmp(word, "eat") && !is_edible(otmp))
		|| (!strcmp(word, "contribute for scrap iron") && (otmp->obj_material != IRON || otmp->owornmask))
		|| (!strcmp(word, "contribute for scrap green steel") && (otmp->obj_material != GREEN_STEEL || otmp->owornmask))
		|| (!strcmp(word, "contribute for scrap bronze") && (otmp->obj_material != COPPER || otmp->owornmask))
		|| (!strcmp(word, "contribute for scrap silver") && (otmp->obj_material != SILVER || otmp->owornmask))
		|| (!strcmp(word, "contribute for scrap gold") && (otmp->obj_material != GOLD || otmp->owornmask))
		|| (!strcmp(word, "contribute for scrap lead") && (otmp->obj_material != LEAD || otmp->owornmask))
		|| (!strcmp(word, "contribute for scrap mithril") && (otmp->obj_material != MITHRIL || otmp->owornmask))
		|| (!strcmp(word, "contribute for scrap fossil dark") && otmp->obj_material != SHADOWSTEEL && otmp->otyp != CHUNK_OF_FOSSIL_DARK)
		|| (!strcmp(word, "contribute for scrap platinum") && (otmp->obj_material != PLATINUM || otmp->owornmask))
		|| (!strcmp(word, "zap") &&
		    !(otmp->oclass == WAND_CLASS 
				|| (otmp->oclass == TOOL_CLASS && otmp->otyp == ROD_OF_FORCE)
				|| (otmp->oclass == ARMOR_CLASS && otmp->otyp == IMPERIAL_ELVEN_GAUNTLETS && check_imp_mod(otmp, IEA_BOLTS))
				|| (otmp->oartifact == ART_STAR_EMPEROR_S_RING)
			))
		|| (!strcmp(word, "inject") && !(otmp->otyp == HYPOSPRAY_AMPULE && otmp->spe > 0))
		|| (!strcmp(word, "give the tear to") &&
			!(otmp->otyp == BROKEN_ANDROID && otmp->ovar1_insightlevel == 0) &&
			!(otmp->otyp == BROKEN_GYNOID && otmp->ovar1_insightlevel == 0) &&
			!(otmp->otyp == LIFELESS_DOLL && otmp->ovar1_insightlevel == 0)
		)
		|| (!strcmp(word, "install dilithim in") &&
			!(otmp->otyp == BROKEN_ANDROID) &&
			!(otmp->otyp == BROKEN_GYNOID)
		)
		|| (!strcmp(word, "wind with") && ((otmp->oclass == TOOL_CLASS &&
		     otyp != SKELETON_KEY) ||
			(otmp->oclass == CHAIN_CLASS)))
		|| (!strcmp(word, "sacrifice") &&
		    (otyp != CORPSE &&
		     otyp != SEVERED_HAND &&                    
		     otyp != EYEBALL &&	/* KMH -- fixed */
		     otyp != AMULET_OF_YENDOR && otyp != FAKE_AMULET_OF_YENDOR))
		|| (!strcmp(word, "research") &&
		    ((otyp != CORPSE &&
		      otyp != SEVERED_HAND &&                    
		      otyp != EYEBALL &&	/* KMH -- fixed */
		      otyp != AMULET_OF_YENDOR && otyp != FAKE_AMULET_OF_YENDOR
			) || otmp->researched
		   ))
		|| (!strcmp(word, "write with") &&
		    ((otmp->oclass == TOOL_CLASS &&
		     otyp != MAGIC_MARKER && otyp != TOWEL 
		     && otyp != ROD_OF_FORCE
			 && otyp != LIGHTSABER && otyp != BEAMSWORD && otyp != DOUBLE_LIGHTSABER && otyp != KAMEREL_VAJRA 
			 && !arti_is_prop(otmp, ARTI_ENGRAVE)) ||
			(otmp->oclass == CHAIN_CLASS)))
		|| (!strcmp(word, "tin") &&
		    (otyp != CORPSE || !tinnable(otmp)))
		|| (!strcmp(word, "feed to the fabber") &&
		    (otyp != ROCK && otyp != SILVER_SLINGSTONE && otyp != FLINT && otyp != LOADSTONE
			&& otyp != BULLET && otyp != SHOTGUN_SHELL && otyp != SILVER_BULLET && otyp != ROCKET))
		|| (!strcmp(word, "rub") &&
		    ((otmp->oclass == TOOL_CLASS &&
		      otyp != OIL_LAMP && otyp != MAGIC_LAMP &&
		      otyp != LANTERN) ||
		     (otmp->oclass == GEM_CLASS && !is_graystone(otmp)) ||
			(otmp->oclass == CHAIN_CLASS)))
		|| (!strncmp(word, "rub on the stone", 16) &&
		    *let == GEM_CLASS &&	/* using known touchstone */
		    otmp->dknown && objects[otyp].oc_name_known)
		|| (!strncmp(word, "replace with", 12)
		    && otmp->otyp != HELLFIRE_COMPONENT
			&& otmp->otyp != CLOCKWORK_COMPONENT)
		|| (!strncmp(word, "forge with", 10)
		    && (otmp->otyp != INGOT || !is_metallic(otmp) ))
		|| (!strncmp(word, "salve", 5) && !salve_target(otmp))
		|| ((!strcmp(word, "use or apply") ||
			!strcmp(word, "untrap with")) &&
		     /* Picks, axes, pole-weapons, bullwhips */
		    ((otmp->oclass == WEAPON_CLASS && !is_pick(otmp) &&
//ifdef FIREARMS
		      otyp != BFG &&
		      otyp != RAYGUN &&
		      otyp != SUBMACHINE_GUN &&
		      otyp != AUTO_SHOTGUN &&
		      otyp != ASSAULT_RIFLE &&
		      otyp != ARM_BLASTER &&
		      otyp != FRAG_GRENADE &&
		      otyp != GAS_GRENADE &&
		      otyp != STICK_OF_DYNAMITE &&
			  otmp->oartifact != ART_STAFF_OF_AESCULAPIUS &&
			  otmp->oartifact != ART_ESSCOOAHLIPBOOURRR &&
//endif
		      !is_axe(otmp) && !is_pole(otmp) && 
			  otyp != BULLWHIP && otyp != VIPERWHIP && otyp != FORCE_WHIP &&
			  otyp != NUNCHAKU && 
			  !is_knife(otmp) && otmp->oartifact != ART_SILVER_STARLIGHT &&
			  !(otmp->oartifact == ART_HOLY_MOONLIGHT_SWORD && !u.veil) &&
			  otmp->otyp != RAKUYO && otmp->otyp != RAKUYO_SABER && 
			  otmp->otyp != BLADE_OF_MERCY && otmp->otyp != BLADE_OF_GRACE && 
			  otmp->otyp != DOUBLE_FORCE_BLADE && otmp->otyp != FORCE_BLADE &&
			  otmp->otyp != HUNTER_S_AXE && otmp->otyp != HUNTER_S_LONG_AXE &&
			  otmp->otyp != SAW_CLEAVER && otmp->otyp != RAZOR_CLEAVER &&
			  otmp->otyp != SAW_SPEAR && otmp->otyp != LONG_SAW &&
			  otmp->otyp != SOLDIER_S_RAPIER && otmp->otyp != SOLDIER_S_SABER &&
			  otmp->otyp != SHANTA_PATA && otmp->otyp != TWINGUN_SHANTA &&
			  otmp->otyp != BOW_BLADE && otmp->otyp != BLADED_BOW &&
			  otmp->otyp != CANE && otmp->otyp != WHIP_SAW &&
			  otmp->otyp != CHIKAGE &&
			  otmp->otyp != TONITRUS &&
			  otmp->otyp != HUNTER_S_LONGSWORD && otmp->otyp != CHURCH_BLADE && otmp->otyp != CHURCH_SHEATH &&
			  otmp->otyp != HUNTER_S_SHORTSWORD && otmp->otyp != CHURCH_HAMMER && otmp->otyp != CHURCH_BRICK &&
			  otmp->otyp != BEAST_CRUSHER && otmp->otyp != BEAST_CUTTER && otmp->otyp != DEVIL_FIST &&
			  otmp->otyp != DEMON_CLAW && otmp->otyp != CHURCH_SHORTSWORD &&
			  otmp->otyp != SMITHING_HAMMER &&
			  !(otmp->oartifact == ART_SKY_REFLECTED && carrying_art(ART_SILVER_SKY)) &&
			  !(otmp->oartifact == ART_SILVER_SKY && carrying_art(ART_SKY_REFLECTED)) &&
			  otmp->otyp != MASS_SHADOW_PISTOL
			 ) ||
			 (otmp->oclass == CHAIN_CLASS && 
				!(otyp == CLOCKWORK_COMPONENT || otyp == SUBETHAIC_COMPONENT 
				  || otyp == HELLFIRE_COMPONENT || otyp == SCRAP 
				  || otyp == LIFELESS_DOLL) /* Note: Joke */
			 ) ||
		     (otmp->oclass == POTION_CLASS &&
		     ((otyp != POT_OIL && otyp != POT_BLOOD &&
			   otyp != POT_WATER) || !otmp->dknown
		      || (otyp == POT_OIL && !objects[POT_OIL].oc_name_known)
		      || (otyp == POT_WATER && !objects[POT_WATER].oc_name_known)
		      || (otyp == POT_BLOOD && !(phlebot_kit && objects[POT_BLOOD].oc_name_known))
			 )
			  ) ||
		     (otmp->oclass == FOOD_CLASS &&
		      otyp != CREAM_PIE && otyp != EUCALYPTUS_LEAF) ||
		     /* MRKR: mining helmets */
		     (otmp->oclass == ARMOR_CLASS &&
		      otyp != LANTERN_PLATE_MAIL &&
		      otyp != EILISTRAN_ARMOR &&
		      otyp != DWARVISH_HELM &&
		      otyp != DROVEN_CLOAK &&
			  otyp != GNOMISH_POINTY_HAT &&
			  otmp->oartifact != ART_AEGIS &&
			  otmp->oartifact != ART_RED_CORDS_OF_ILMATER &&
			  otmp->oartifact != ART_GREAT_CLAWS_OF_URDLEN
			  ) || 
		     (otmp->oclass == GEM_CLASS && !is_graystone(otmp)
				&& otyp != CATAPSI_VORTEX && otyp != ANTIMAGIC_RIFT
				&& otyp != VITAL_SOULSTONE && otyp != SPIRITUAL_SOULSTONE
				&& !(otyp == CRYSTAL && otmp->obj_material == FLESH)
				&& !(otyp == DILITHIUM_CRYSTAL && Role_if(PM_ANACHRONONAUT) && !otmp->oartifact)
			 )))
		|| (!strcmp(word, "invoke") &&
		    (!otmp->oartifact && !objects[otyp].oc_unique &&
		     (otyp != FAKE_AMULET_OF_YENDOR || otmp->known) &&
		     (otyp != CRYSTAL_BALL) &&	/* #invoke synonym for apply */
			 (otyp != RIN_WISHES && (otmp->dknown && objects[RIN_WISHES].oc_name_known)) &&	/* grants a wish (if charged) */
			 (otyp != CANDLE_OF_INVOCATION && (otmp->dknown && objects[CANDLE_OF_INVOCATION].oc_name_known)) && /* opens a gate or grants a wish */
		   /* note: presenting the possibility of invoking non-artifact
		      mirrors and/or lamps is a simply a cruel deception... */
		     otyp != MIRROR && otyp != MAGIC_LAMP &&
		     (otyp != OIL_LAMP ||	/* don't list known oil lamp */
		      (otmp->dknown && objects[OIL_LAMP].oc_name_known))))
		|| (!strcmp(word, "untrap with") &&
		    ((otmp->oclass == TOOL_CLASS && otyp != CAN_OF_GREASE) ||
			(otmp->oclass == CHAIN_CLASS)))
		|| (!strcmp(word, "charge") && !is_chargeable(otmp))
		|| (!strcmp(word, "give the Goat's bite") &&
			!goat_acidable(otmp))
		|| (!strcmp(word, "give the Goat's hunger") &&
			!goat_droolable(otmp))
		|| (!strcmp(word, "reveal the Minor Stars") &&
			!yog_magicable(otmp))
		|| (!strcmp(word, "show distant vistas") &&
			!yog_windowable(otmp))
		|| (!strcmp(word, "offer to the flame") && 
			!sflm_offerable(otmp))
		|| (!strcmp(word, "mirror-finish") && 
			!sflm_mirrorable(otmp))
		|| (!strcmp(word, "curse-proof glaze") && 
			!sflm_glazeable(otmp))
		|| (!strcmp(word, "reveal mortality") && 
			!sflm_mortalable(otmp))
		|| (!strcmp(word, "reveal true death") && 
			!sflm_truedeathable(otmp))
		|| (!strcmp(word, "reveal the unworthy") && 
			!sflm_unworthyable(otmp))
		|| (!strcmp(word, "reveal righteous wrath") && 
			!sflm_wrathable(otmp))
		|| (!strcmp(word, "share burdens") && 
			!sflm_burdenable(otmp))
		|| (!strcmp(word, "preserve life") && 
			!sflm_lifeable(otmp))
		|| (!strcmp(word, "smelt silver in the silver light") && 
			!sflm_smeltable_silver(otmp))
		|| (!strcmp(word, "smelt platinum in the silver light") && 
			!sflm_smeltable_platinum(otmp))
		|| (!strcmp(word, "smelt mithril in the silver light") && 
			!sflm_smeltable_mithril(otmp))
		|| (!strcmp(word, "burn in the silver flame") && 
			!(otmp->blessed || otmp->cursed))
		|| (!strcmp(word, "armor piece to repair") &&
		    (!is_imperial_elven_armor(otmp)))
		|| (!strcmp(word, "repair the helm with") &&
		    (!helm_upgrade_obj(otmp)))
		|| (!strcmp(word, "repair the gauntlets with") &&
		    (!gauntlets_upgrade_obj(otmp)))
		|| (!strcmp(word, "repair the armor with") &&
		    (!armor_upgrade_obj(otmp)))
		|| (!strcmp(word, "repair the boots with") &&
		    (!boots_upgrade_obj(otmp)))
		|| (!strcmp(word, "upgrade your stove with") &&
		    (otyp != TINNING_KIT))
		|| (!strcmp(word, "upgrade your switch with") &&
		    (otyp != CROSSBOW))
		|| (!strcmp(word, "upgrade your spring with") &&
			(otyp != CLOCKWORK_COMPONENT))
		|| (!strcmp(word, "upgrade your armor with") &&
		    (otyp != ARCHAIC_PLATE_MAIL))
		|| (!strcmp(word, "build a phase engine with") &&
		    (otyp != SUBETHAIC_COMPONENT))
		|| (!strcmp(word, "build a magic furnace with") &&
		    (otyp != WAN_DRAINING))
		|| (!strcmp(word, "build a hellfire furnace with") &&
		    (otyp != HELLFIRE_COMPONENT))
		|| (!strcmp(word, "build a scrap maw with") &&
		    (otyp != SCRAP))
		|| (!strcmp(word, "make a skeletal minion of") &&
		    !(otyp == CORPSE))
		|| (!strcmp(word, "feed blood to") &&
		    !(otyp == CORPSE && otmp->odrained && (peek_at_iced_corpse_age(otmp) + 20) >= monstermoves))
		    )
			foo--;
		/* ugly check for unworn armor that can't be worn */
		else if (putting_on(word) && *let == ARMOR_CLASS &&
			 !canwearobj(otmp, &dummymask, FALSE)) {
			foo--;
			allowall = TRUE;
			*ap++ = otmp->invlet;
		}
	    } else {

		/* "ugly check" for reading fortune cookies, part 2 */
		if ((!strcmp(word, "read") &&
		    (otmp->otyp == FORTUNE_COOKIE
			|| otmp->otyp == T_SHIRT
		    )
		)){
			bp[foo++] = otmp->invlet;
			allowall = TRUE;
		}
	    }
		
		//Make exceptions for gemstone items made of specific gems
		if (otmp->obj_material == GEMSTONE && otmp->sub_material
			&& (!objects[otmp->sub_material].oc_name_known || !otmp->dknown)
			&& !strncmp(word, "rub on the stone", 16)) {
			bp[foo++] = otmp->invlet;
			allowall = TRUE;
		}
	    if(ilet == 'z') ilet = 'A'; else ilet++;
	}
	bp[foo] = 0;
	if(foo == 0 && bp > buf && bp[-1] == ' ') *--bp = 0;
	Strcpy(lets, bp);	/* necessary since we destroy buf */
	if(foo > 5)			/* compactify string */
		compactify(bp);
	*ap = '\0';

#ifndef GOLDOBJ
	if(!foo && !allowall && !allowgold && !allownone) {
#else
	if(!foo && !allowall && !allownone) {
#endif
		You("don't have anything %sto %s.",
			foox ? "else " : "", word);
		return((struct obj *)0);
	}
	for(;;) {
		cnt = 0;
		if (allowcnt == 2) allowcnt = 1;  /* abort previous count */
		if(!buf[0]) {
			Sprintf(qbuf, "What do you want to %s? [*]", word);
		} else {
			Sprintf(qbuf, "What do you want to %s? [%s or ?*]",
				word, buf);
		}
#ifdef REDO
		if (in_doagain)
		    ilet = readchar();
		else
#endif
		    ilet = yn_function(qbuf, (char *)0, '\0');
		if(ilet == '0') prezero = TRUE;
		while(digit(ilet) && allowcnt) {
#ifdef REDO
			if (ilet != '?' && ilet != '*')	savech(ilet);
#endif
			cnt = 10*cnt + (ilet - '0');
			allowcnt = 2;	/* signal presence of cnt */
			ilet = readchar();
		}
		if(digit(ilet)) {
			pline("No count allowed with this command.");
			continue;
		}
		if(index(quitchars,ilet)) {
		    if(flags.verbose)
			pline1(Never_mind);
		    return((struct obj *)0);
		}
		if(ilet == '-') {
			return(allownone ? &zeroobj : (struct obj *) 0);
		}
		if(ilet == def_oc_syms[COIN_CLASS]) {
			if (!usegold) {
			    if (!strncmp(word, "rub on ", 7)) {
				/* the dangers of building sentences... */
				You("cannot rub gold%s.", word + 3);
			    } else {
				You("cannot %s gold.", word);
			    }
			    return(struct obj *)0;
#ifndef GOLDOBJ
			} else if (!allowgold) {
				You("are not carrying any gold.");
				return(struct obj *)0;
#endif
			} 
			if(cnt == 0 && prezero) return((struct obj *)0);
			/* Historic note: early Nethack had a bug which was
			 * first reported for Larn, where trying to drop 2^32-n
			 * gold pieces was allowed, and did interesting things
			 * to your money supply.  The LRS is the tax bureau
			 * from Larn.
			 */
			if(cnt < 0) {
	pline_The("LRS would be very interested to know you have that much.");
				return(struct obj *)0;
			}

#ifndef GOLDOBJ
			if(!(allowcnt == 2 && cnt < u.ugold))
				cnt = u.ugold;
			if (cnt)
			    return(mkgoldobj(cnt));
			else return((struct obj *)0);
#endif
		}
		if(ilet == '?' || ilet == '*') {
		    char *allowed_choices = (ilet == '?') ? lets : (char *)0;
		    long ctmp = 0;

		    if (ilet == '?' && !*lets && *altlets)
			allowed_choices = altlets;
		    ilet = display_pickinv(allowed_choices, TRUE,
					   allowcnt ? &ctmp : (long *)0
#ifdef DUMP_LOG
					   , FALSE, TRUE
#endif
					   );
		    if(!ilet) continue;
		    if (allowcnt && ctmp >= 0) {
			cnt = ctmp;
			if (!cnt) prezero = TRUE;
			allowcnt = 2;
		    }
		    if(ilet == '\033') {
			if(flags.verbose)
			    pline1(Never_mind);
			return((struct obj *)0);
		    }
		    /* they typed a letter (not a space) at the prompt */
		}
#ifdef REDO
		savech(ilet);
#endif
		for (otmp = invent; otmp; otmp = otmp->nobj)
			if (otmp->invlet == ilet) break;
		if(!otmp) {
			You("don't have that object.");
#ifdef REDO
			if (in_doagain) return((struct obj *) 0);
#endif
			continue;
		} else if (allowcnt == 2 && !strcmp(word,"throw") && cnt > 1 && !(
#ifdef GOLDOBJ
			(ilet == def_oc_syms[COIN_CLASS]) ||
#endif
			(otmp->oartifact == ART_FLUORITE_OCTAHEDRON)
			)) {
			You("can only throw one item at a time.");
			allowcnt = 1;
			if (cnt == 0 && prezero) return((struct obj *)0);
			continue;
		} else if (cnt < 0 || otmp->quan < cnt) {
			You("don't have that many!  You have only %ld.",
			    otmp->quan);
#ifdef REDO
			if (in_doagain) return((struct obj *) 0);
#endif
			continue;
		}
		break;
	}
	if(!allowall && let && !index(let,otmp->oclass)
#ifdef GOLDOBJ
	   && !(usegold && otmp->oclass == COIN_CLASS)
#endif
	   ) {
		silly_thing(word, otmp);
		return((struct obj *)0);
	}
	if(allowcnt == 2) {	/* cnt given */
	    if(cnt == 0) return (struct obj *)0;
	    if(cnt != otmp->quan) {
		/* don't split a stack of cursed loadstones */
		if (otmp->otyp == LOADSTONE && otmp->cursed)
		    /* kludge for canletgo()'s can't-drop-this message */
		    otmp->corpsenm = (int) cnt;
		else
		    otmp = splitobj(otmp, cnt);
	    }
	}
	return(otmp);
}

void
silly_thing(word, otmp)
const char *word;
struct obj *otmp;
{
	const char *s1, *s2, *s3, *what;
	int ocls = otmp->oclass, otyp = otmp->otyp;

	s1 = s2 = s3 = 0;
	/* check for attempted use of accessory commands ('P','R') on armor
	   and for corresponding armor commands ('W','T') on accessories */
	if ((ocls == RING_CLASS || otyp == MEAT_RING) ||
		ocls == AMULET_CLASS ||
		is_belt(otmp) ||
		(is_worn_tool(otmp))) {
	    if (!strcmp(word, "wear"))
		s1 = "P", s2 = "put", s3 = " on";
	    else if (!strcmp(word, "take off"))
		s1 = "R", s2 = "remove", s3 = "";
	}
	else if (ocls == ARMOR_CLASS) {
	    if (!strcmp(word, "put on"))
		s1 = "W", s2 = "wear", s3 = "";
	    else if (!strcmp(word, "remove"))
		s1 = "T", s2 = "take", s3 = " off";
	}
	if (s1) {
	    what = "that";
	    /* quantity for armor and accessory objects is always 1,
	       but some things should be referred to as plural */
	    if (otyp == LENSES || otyp == SUNGLASSES || is_gloves(otmp) || is_boots(otmp))
		what = "those";
	    pline("Use the '%s' command to %s %s%s.", s1, s2, what, s3);
	} else {
	    pline(silly_thing_to, word);
	}
}

#endif /* OVL1 */
#ifdef OVLB

STATIC_PTR int
ckvalidcat(otmp)
register struct obj *otmp;
{
	/* use allow_category() from pickup.c */
	return((int)allow_category(otmp, 0));
}

STATIC_PTR int
ckunpaid(otmp)
register struct obj *otmp;
{
	return((int)(otmp->unpaid));
}

boolean
wearing_armor()
{
	return((boolean)(uarm || uarmc || uarmf || uarmg || uarmh || uarms
#ifdef TOURIST
		|| uarmu
#endif
		));
}

boolean
is_worn_no_flags(struct obj *otmp)
{
    return is_worn(otmp, 0);
}

boolean
is_worn(struct obj *otmp, int qflags)
{
    return((boolean)(!!(otmp->owornmask & (W_ARMOR | W_RING | W_AMUL | W_BELT | W_TOOL |
#ifdef STEED
			W_SADDLE |
#endif
			W_WEP | W_SWAPWEP | W_QUIVER))));
}

static NEARDATA const char removeables[] =
	{ ARMOR_CLASS, WEAPON_CLASS, RING_CLASS, AMULET_CLASS, TOOL_CLASS, 0 };

/* interactive version of getobj - used for Drop, Identify and */
/* Takeoff (A). Return the number of times fn was called successfully */
/* If combo is TRUE, we just use this to get a category list */
int
ggetobj(word, fn, mx, combo, resultflags)
const char *word;
int FDECL((*fn),(OBJ_P)), mx;
boolean combo;		/* combination menu flag */
unsigned *resultflags;
{
	int FDECL((*ckfn),(OBJ_P)) = (int FDECL((*),(OBJ_P))) 0;
	boolean FDECL((*filter),(OBJ_P)) = (boolean FDECL((*),(OBJ_P))) 0;
	boolean takeoff, ident, allflag, m_seen;
	int itemcount;
#ifndef GOLDOBJ
	int oletct, iletct, allowgold, unpaid, oc_of_sym;
#else
	int oletct, iletct, unpaid, oc_of_sym;
#endif
	char sym, *ip, olets[MAXOCLASSES+5], ilets[MAXOCLASSES+5];
	char extra_removeables[3+1];	/* uwep,uswapwep,uquiver */
	char buf[BUFSZ], qbuf[QBUFSZ];

	if (resultflags) *resultflags = 0;
#ifndef GOLDOBJ
	allowgold = (u.ugold && !strcmp(word, "drop")) ? 1 : 0;
#endif
	takeoff = ident = allflag = m_seen = FALSE;
#ifndef GOLDOBJ
	if(!invent && !allowgold){
#else
	if(!invent){
#endif
		You("have nothing to %s.", word);
		return(0);
	}
	add_valid_menu_class(0);	/* reset */
	if (taking_off(word)) {
	    takeoff = TRUE;
	    filter = is_worn_no_flags;
	} else if (!strcmp(word, "identify")) {
	    ident = TRUE;
	    filter = not_fully_identified;
	}

	iletct = collect_obj_classes(ilets, invent,
				     	FALSE,
#ifndef GOLDOBJ
					(allowgold != 0),
#endif
					filter, &itemcount);
	unpaid = count_unpaid(invent);

	if (ident && !iletct) {
	    return -1;		/* no further identifications */
	} else if (!takeoff && (unpaid || invent)) {
	    ilets[iletct++] = ' ';
	    if (unpaid) ilets[iletct++] = 'u';
	    if (count_buc(invent, BUC_BLESSED))  ilets[iletct++] = 'B';
	    if (count_buc(invent, BUC_UNCURSED)) ilets[iletct++] = 'U';
	    if (count_buc(invent, BUC_CURSED))   ilets[iletct++] = 'C';
	    if (count_buc(invent, BUC_UNKNOWN))  ilets[iletct++] = 'X';
	    if (invent) ilets[iletct++] = 'a';
	} else if (takeoff && invent) {
	    ilets[iletct++] = ' ';
	}
	ilets[iletct++] = 'i';
	if (!combo)
	    ilets[iletct++] = 'm';	/* allow menu presentation on request */
	ilets[iletct] = '\0';

	for (;;) {
	    Sprintf(qbuf,"What kinds of thing do you want to %s? [%s]",
		    word, ilets);
	    getlin(qbuf, buf);
	    if (buf[0] == '\033') return(0);
	    if (index(buf, 'i')) {
		if (display_inventory((char *)0, TRUE) == '\033') return 0;
	    } else
		break;
	}

	extra_removeables[0] = '\0';
	if (takeoff) {
	    /* arbitrary types of items can be placed in the weapon slots
	       [any duplicate entries in extra_removeables[] won't matter] */
	    if (uwep) (void)strkitten(extra_removeables, uwep->oclass);
	    if (uswapwep) (void)strkitten(extra_removeables, uswapwep->oclass);
	    if (uquiver) (void)strkitten(extra_removeables, uquiver->oclass);
	}

	ip = buf;
	olets[oletct = 0] = '\0';
	while ((sym = *ip++) != '\0') {
	    if (sym == ' ') continue;
	    oc_of_sym = def_char_to_objclass(sym);
	    if (takeoff && oc_of_sym != MAXOCLASSES) {
		if (index(extra_removeables, oc_of_sym)) {
		    ;	/* skip rest of takeoff checks */
		} else if (!index(removeables, oc_of_sym)) {
		    pline("Not applicable.");
		    return 0;
		} else if (oc_of_sym == ARMOR_CLASS && !wearing_armor()) {
		    You("are not wearing any armor.");
		    return 0;
		} else if (oc_of_sym == WEAPON_CLASS &&
			!uwep && !uswapwep && !uquiver) {
		    You("are not wielding anything.");
		    return 0;
		} else if (oc_of_sym == RING_CLASS && !uright && !uleft) {
		    You("are not wearing rings.");
		    return 0;
		} else if (oc_of_sym == AMULET_CLASS && !uamul) {
		    You("are not wearing an amulet.");
		    return 0;
		} else if (oc_of_sym == TOOL_CLASS && !ublindf) {
		    You("are not wearing a blindfold.");
		    return 0;
		}
	    }

	    if (oc_of_sym == COIN_CLASS && !combo) {
#ifndef GOLDOBJ
		if (allowgold == 1)
		    (*fn)(mkgoldobj(u.ugold));
		else if (!u.ugold)
		    You("have no gold.");
		allowgold = 2;
#else
		flags.botl = 1;
#endif
	    } else if (sym == 'a') {
		allflag = TRUE;
	    } else if (sym == 'A') {
		/* same as the default */ ;
	    } else if (sym == 'u') {
		add_valid_menu_class('u');
		ckfn = ckunpaid;
	    } else if (sym == 'B') {
	    	add_valid_menu_class('B');
	    	ckfn = ckvalidcat;
	    } else if (sym == 'U') {
	    	add_valid_menu_class('U');
	    	ckfn = ckvalidcat;
	    } else if (sym == 'C') {
	    	add_valid_menu_class('C');
		ckfn = ckvalidcat;
	    } else if (sym == 'X') {
	    	add_valid_menu_class('X');
		ckfn = ckvalidcat;
	    } else if (sym == 'm') {
		m_seen = TRUE;
	    } else if (oc_of_sym == MAXOCLASSES) {
		You("don't have any %c's.", sym);
	    } else if (oc_of_sym != VENOM_CLASS) {	/* suppress venom */
		if (!index(olets, oc_of_sym)) {
		    add_valid_menu_class(oc_of_sym);
		    olets[oletct++] = oc_of_sym;
		    olets[oletct] = 0;
		}
	    }
	}

	if (m_seen)
	    return (allflag || (!oletct && ckfn != ckunpaid)) ? -2 : -3;
	else if (flags.menu_style != MENU_TRADITIONAL && combo && !allflag)
	    return 0;
#ifndef GOLDOBJ
	else if (allowgold == 2 && !oletct)
	    return 1;	/* you dropped gold (or at least tried to) */
	else {
#else
	else /*!!!! if (allowgold == 2 && !oletct)
	    !!!! return 1;	 you dropped gold (or at least tried to) 
            !!!! test gold dropping
	else*/ {
#endif
	    int cnt = askchain(&invent, olets, allflag, fn, ckfn, mx, word); 
	    /*
	     * askchain() has already finished the job in this case
	     * so set a special flag to convey that back to the caller
	     * so that it won't continue processing.
	     * Fix for bug C331-1 reported by Irina Rempt-Drijfhout. 
	     */
	    if (combo && allflag && resultflags)
		*resultflags |= ALL_FINISHED; 
	    return cnt;
	}
}

/*
 * Walk through the chain starting at objchn and ask for all objects
 * with olet in olets (if nonNULL) and satisfying ckfn (if nonnull)
 * whether the action in question (i.e., fn) has to be performed.
 * If allflag then no questions are asked. Max gives the max nr of
 * objects to be treated. Return the number of objects treated.
 */
int
askchain(objchn, olets, allflag, fn, ckfn, mx, word)
struct obj **objchn;
register int allflag, mx;
register const char *olets, *word;	/* olets is an Obj Class char array */
register int FDECL((*fn),(OBJ_P)), FDECL((*ckfn),(OBJ_P));
{
	struct obj *otmp, *otmp2, *otmpo;
	register char sym, ilet;
	register int cnt = 0, dud = 0, tmp;
	boolean takeoff, nodot, ident, ininv;
	char qbuf[QBUFSZ];

	takeoff = taking_off(word);
	ident = !strcmp(word, "identify");
	nodot = (!strcmp(word, "nodot") || !strcmp(word, "drop") ||
		 ident || takeoff);
	ininv = (*objchn == invent);
	/* Changed so the askchain is interrogated in the order specified.
	 * For example, if a person specifies =/ then first all rings will be
	 * asked about followed by all wands -dgk
	 */
nextclass:
	ilet = 'a'-1;
	if (*objchn && (*objchn)->oclass == COIN_CLASS)
		ilet--;		/* extra iteration */
	for (otmp = *objchn; otmp; otmp = otmp2) {
		if(ilet == 'z') ilet = 'A'; else ilet++;
		otmp2 = otmp->nobj;
		if (olets && *olets && otmp->oclass != *olets) continue;
		if (takeoff && !is_worn(otmp, 0)) continue;
		if (ident && !not_fully_identified(otmp)) continue;
		if (ckfn && !(*ckfn)(otmp)) continue;
		if (!allflag) {
			Strcpy(qbuf, !ininv ? doname(otmp) :
				xprname(otmp, (char *)0, ilet, !nodot, 0L, 0L));
			Strcat(qbuf, "?");
			sym = (takeoff || ident || otmp->quan < 2L) ?
				nyaq(qbuf) : nyNaq(qbuf);
		}
		else	sym = 'y';

		otmpo = otmp;
		if (sym == '#') {
		 /* Number was entered; split the object unless it corresponds
		    to 'none' or 'all'.  2 special cases: cursed loadstones and
		    welded weapons (eg, multiple daggers) will remain as merged
		    unit; done to avoid splitting an object that won't be
		    droppable (even if we're picking up rather than dropping).
		  */
		    if (!yn_number)
			sym = 'n';
		    else {
			sym = 'y';
			if (yn_number < otmp->quan && !welded(otmp) &&
			    (!otmp->cursed || otmp->otyp != LOADSTONE)) {
			    otmp = splitobj(otmp, yn_number);
			}
		    }
		}
		switch(sym){
		case 'a':
			allflag = 1;
		case 'y':
			tmp = (*fn)(otmp);
			if(tmp < 0) {
			    if (otmp != otmpo) {
				/* split occurred, merge again */
				(void) merged(&otmpo, &otmp);
			    }
			    goto ret;
			}
			cnt += tmp;
			if(--mx == 0) goto ret;
		case 'n':
			if(nodot) dud++;
		default:
			break;
		case 'q':
			/* special case for seffects() */
			if (ident) cnt = -1;
			goto ret;
		}
	}
	if (olets && *olets && *++olets)
		goto nextclass;
	if(!takeoff && (dud || cnt)) pline("That was all.");
	else if(!dud && !cnt) pline("No applicable objects.");
ret:
	return(cnt);
}


/*
 *	Object identification routines:
 */

/* make an object actually be identified; no display updating */
void
fully_identify_obj(otmp)
struct obj *otmp;
{
    makeknown(otmp->otyp);
	if (otmp->obj_material == GEMSTONE && otmp->sub_material)
		makeknown(otmp->sub_material);
    if (otmp->oartifact) discover_artifact(otmp->oartifact);
    otmp->known = otmp->dknown = otmp->bknown = otmp->rknown = otmp->sknown = 1;
    if (otmp->otyp == EGG && otmp->corpsenm != NON_PM)
	learn_egg_type(otmp->corpsenm);
}

/* ggetobj callback routine; identify an object and give immediate feedback */
int
identify(otmp)
struct obj *otmp;
{
    fully_identify_obj(otmp);
    prinv((char *)0, otmp, 0L);
    return 1;
}

/* menu of unidentified objects; select and identify up to id_limit of them */
STATIC_OVL void
menu_identify(id_limit)
int id_limit;
{
    menu_item *pick_list;
    int n, i, first = 1;
    char buf[BUFSZ];
    /* assumptions:  id_limit > 0 and at least one unID'd item is present */

    while (id_limit) {
	Sprintf(buf, "What would you like to identify %s?",
		first ? "first" : "next");
	n = query_objlist(buf, invent, SIGNAL_NOMENU|USE_INVLET|INVORDER_SORT|SIGNAL_ESCAPE,
		&pick_list, PICK_ANY, not_fully_identified_dummy_flags);

	if (n > 0) {
	    if (n > id_limit) n = id_limit;
	    for (i = 0; i < n; i++, id_limit--){
			(void) identify(pick_list[i].item.a_obj);
			u.uconduct.IDs++;
		}
	    first = 0;
	    free((genericptr_t) pick_list);
	    mark_synch(); /* Before we loop to pop open another menu */
	} else if (n < 0) {
	    if (n == -1) pline("That was all.");
	    id_limit = 0; /* Stop now */
	}
    }
}

/* dialog with user to identify a given number of items; 0 means all */
void
identify_pack(id_limit)
int id_limit;
{
    struct obj *obj, *the_obj;
    int n, unid_cnt;

    unid_cnt = 0;
    the_obj = 0;		/* if unid_cnt ends up 1, this will be it */
    for (obj = invent; obj; obj = obj->nobj)
	if (not_fully_identified(obj)) ++unid_cnt, the_obj = obj;

    if (!unid_cnt) {
	You("have already identified all of your possessions.");
    } else if (!id_limit) {
	/* identify everything */
	if (unid_cnt == 1) {
	    (void) identify(the_obj);
		u.uconduct.IDs++;
	} else {

	    /* TODO:  use fully_identify_obj and cornline/menu/whatever here */
	    for (obj = invent; obj; obj = obj->nobj){
			if (not_fully_identified(obj)){
				(void) identify(obj);
				u.uconduct.IDs++;
			}
		}

	}
    } else {
	/* identify up to `id_limit' items */
	n = 0;
	if (flags.menu_style == MENU_TRADITIONAL)
	    do {
		n = ggetobj("identify", identify, id_limit, FALSE, (unsigned *)0);
		if (n < 0) break; /* quit or no eligible items */
	    } while ((id_limit -= n) > 0);
	if (n == 0 || n < -1)
	    menu_identify(id_limit);
    }
    update_inventory();
}

#endif /* OVLB */
#ifdef OVL2

STATIC_OVL char
obj_to_let(obj)	/* should of course only be called for things in invent */
register struct obj *obj;
{
#ifndef GOLDOBJ
	if (obj->oclass == COIN_CLASS)
		return GOLD_SYM;
#endif
	if (!flags.invlet_constant) {
		obj->invlet = NOINVSYM;
		reassign();
	}
	return obj->invlet;
}

/*
 * Print the indicated quantity of the given object.  If quan == 0L then use
 * the current quantity.
 */
void
prinv(prefix, obj, quan)
const char *prefix;
register struct obj *obj;
long quan;
{
	if (!prefix) prefix = "";
	pline("%s%s%s",
	      prefix, *prefix ? " " : "",
	      xprname(obj, (char *)0, obj_to_let(obj), TRUE, 0L, quan));
}

#endif /* OVL2 */
#ifdef OVL1

char *
xprname(obj, txt, let, dot, cost, quan)
struct obj *obj;
const char *txt;	/* text to print instead of obj */
char let;		/* inventory letter */
boolean dot;		/* append period; (dot && cost => Iu) */
long cost;		/* cost (for inventory of unpaid or expended items) */
long quan;		/* if non-0, print this quantity, not obj->quan */
{
#ifdef LINT	/* handle static char li[BUFSZ]; */
    char li[BUFSZ];
#else
    static char li[BUFSZ];
#endif
    boolean use_invlet = flags.invlet_constant && let != CONTAINED_SYM;
    long savequan = 0;

    if (quan && obj) {
	savequan = obj->quan;
	obj->quan = quan;
    }

    /*
     * If let is:
     *	*  Then obj == null and we are printing a total amount.
     *	>  Then the object is contained and doesn't have an inventory letter.
     */
    if (cost != 0 || let == '*') {
	/* if dot is true, we're doing Iu, otherwise Ix */
	Sprintf(li, "%c - %-45s %6ld %s",
		(dot && use_invlet ? obj->invlet : let),
		(txt ? txt : doname(obj)), cost, currency(cost));
#ifndef GOLDOBJ
    } else if (obj && obj->oclass == COIN_CLASS) {
	Sprintf(li, "%ld gold piece%s%s", obj->quan, plur(obj->quan),
		(dot ? "." : ""));
#endif
    } else {
	/* ordinary inventory display or pickup message */
	Sprintf(li, "%c - %s%s",
		(use_invlet ? obj->invlet : let),
		(txt ? txt : doname(obj)), (dot ? "." : ""));
    }
    if (savequan) obj->quan = savequan;

    return li;
}

#endif /* OVL1 */
#ifdef OVLB

/* the 'i' command */
int
ddoinv()
{
	char c;
	struct obj *otmp;
	c = display_inventory((char *)0, TRUE);
	if (!c) return 0;
	for (otmp = invent; otmp; otmp = otmp->nobj)
		if (otmp->invlet == c) break;
		if (otmp)
		{
			if (iflags.item_use_menu)
				return itemactions(otmp);
			else
			{
				winid datawin = create_nhwindow(NHW_MENU);
				putstr(datawin, ATR_NONE, doname(otmp));
				describe_item(otmp, otmp->otyp, otmp->oartifact, &datawin);
				checkfile(xname_bland(otmp), 0, FALSE, TRUE, &datawin);
				display_nhwindow(datawin, TRUE);
				destroy_nhwindow(datawin);
				return MOVE_INSTANT;
			}
		}
	return MOVE_INSTANT;
}

/** Puts up a menu asking what to do with an object;
   sends the object to the appropriate command, if one is selected.
   Each command that can affect the object is listed, but only one
   out of a set of synonyms is given.
   Returns 1 if it consumes time, 0 otherwise. */
int
itemactions(obj)
struct obj *obj;
{
	winid win;
	int n;
	int NDECL((*feedback_fn)) = 0;
	anything any;
	menu_item *selected = 0;
	boolean phlebot_kit = !!find_object_type(invent, PHLEBOTOMY_KIT);

	struct monst *mtmp;
	char prompt[BUFSIZ];

	win = create_nhwindow(NHW_MENU);
	start_menu(win);
	/* (a)pply: tools, eucalyptus, cream pie, oil, hooks/whips
	   Exceptions: applying stones is on V; breaking wands is on V;
	   equipment-tools are on W; tin openers are on w. */
	any.a_void = (genericptr_t)doapply;
	/* Rather a mess for 'a', as it means so many different things
	   with so many different objects */
	if (obj->otyp == CREAM_PIE)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Hit yourself with this cream pie", MENU_UNSELECTED);
	else if (obj->otyp == BULLWHIP || obj->otyp == VIPERWHIP)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Lash out with this whip", MENU_UNSELECTED);
	else if (obj->otyp == FORCE_WHIP || obj->otyp == WHIP_SAW)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Lock or lash out with this whip", MENU_UNSELECTED);
	else if (obj->otyp == GRAPPLING_HOOK)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Grapple something with this hook", MENU_UNSELECTED);
	else if (obj->otyp == BAG_OF_TRICKS && obj->known)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Reach into this bag", MENU_UNSELECTED);
	else if (Is_container(obj) || obj->otyp == BAG_OF_TRICKS)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Open this container", MENU_UNSELECTED);
	else if (obj->otyp == CAN_OF_GREASE)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Use the can to grease an item", MENU_UNSELECTED);
	else if (obj->otyp == LOCK_PICK ||
#ifdef TOURIST
			obj->otyp == CREDIT_CARD ||
#endif
			obj->otyp == SKELETON_KEY)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Use this tool to pick a lock", MENU_UNSELECTED);
	else if (obj->otyp == TINNING_KIT)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Use this kit to tin a corpse", MENU_UNSELECTED);
	else if (obj->otyp == DISSECTION_KIT)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Use this kit to dissect a corpse", MENU_UNSELECTED);
	else if (obj->otyp == LEASH)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Tie a pet to this leash", MENU_UNSELECTED);
	else if (obj->otyp == SADDLE)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Place this saddle on a pet", MENU_UNSELECTED);
	else if (obj->otyp == MAGIC_WHISTLE || obj->otyp == WHISTLE)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Blow this whistle", MENU_UNSELECTED);
	else if (obj->otyp == EUCALYPTUS_LEAF)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Use this leaf as a whistle", MENU_UNSELECTED);
	else if (obj->otyp == STETHOSCOPE)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Listen through the stethoscope", MENU_UNSELECTED);
	else if (obj->otyp == MIRROR)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Show something its reflection", MENU_UNSELECTED);
	else if (obj->otyp == BELL || obj->otyp == BELL_OF_OPENING)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Ring the bell", MENU_UNSELECTED);
	else if (obj->otyp == CANDELABRUM_OF_INVOCATION)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Light or extinguish the candelabrum", MENU_UNSELECTED);
	else if ((obj->otyp == WAX_CANDLE || obj->otyp == TALLOW_CANDLE) &&
			carrying(CANDELABRUM_OF_INVOCATION))
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Attach this candle to the candelabrum", MENU_UNSELECTED);
	else if (obj->otyp == WAX_CANDLE || obj->otyp == TALLOW_CANDLE || obj->otyp == CANDLE_OF_INVOCATION)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Light or extinguish this candle", MENU_UNSELECTED);
	else if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP ||
			obj->otyp == MAGIC_LAMP || obj->otyp == LANTERN_PLATE_MAIL)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Light or extinguish this light source", MENU_UNSELECTED);
	else if (obj->otyp == POT_OIL && objects[obj->otyp].oc_name_known)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Light or extinguish this oil", MENU_UNSELECTED);
	else if (obj->otyp == POT_BLOOD && phlebot_kit && objects[obj->otyp].oc_name_known)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Inject yourself with this vial of blood.", MENU_UNSELECTED);
#if 0 /* TODO */
	else if (obj->oclass == POTION_CLASS) {
		any.a_void = (genericptr_t) dodip;
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Dip something into this potion", MENU_UNSELECTED);
	}
#endif
#ifdef TOURIST
	else if (obj->otyp == EXPENSIVE_CAMERA)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Take a photograph", MENU_UNSELECTED);
#endif
	else if (obj->otyp == TOWEL)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Clean yourself off with this towel", MENU_UNSELECTED);
	else if (obj->otyp == CRYSTAL_BALL)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Peer into this crystal ball", MENU_UNSELECTED);
	else if (obj->otyp == MAGIC_MARKER)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Write on something with this marker", MENU_UNSELECTED);
	else if (obj->otyp == FIGURINE)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Make this figurine transform", MENU_UNSELECTED);
	else if (obj->otyp == CRYSTAL_SKULL)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Use this crystal skull", MENU_UNSELECTED);
	else if (obj->otyp == UNICORN_HORN)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Squeeze the unicorn horn tightly", MENU_UNSELECTED);
	else if (obj->otyp == MIST_PROJECTOR)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Use this mist projector", MENU_UNSELECTED);
	else if (obj->otyp == HOLY_SYMBOL_OF_THE_BLACK_MOTHE)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Commune with the Black Mother", MENU_UNSELECTED);
	else if (obj->otyp == HYPERBOREAN_DIAL)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Try to solve this puzzle", MENU_UNSELECTED);
	else if ((obj->otyp >= FLUTE && obj->otyp <= DRUM_OF_EARTHQUAKE) ||
			(obj->otyp == HORN_OF_PLENTY && !obj->known))
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Play this musical instrument", MENU_UNSELECTED);
	else if (obj->otyp == HORN_OF_PLENTY)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Blow into the horn of plenty", MENU_UNSELECTED);
	else if (obj->otyp == LAND_MINE || obj->otyp == BEARTRAP)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Arm this trap", MENU_UNSELECTED);
	else if (obj->otyp == PICK_AXE || obj->otyp == DWARVISH_MATTOCK)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Dig with this digging tool", MENU_UNSELECTED);
	else if (obj->oclass == WAND_CLASS)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Break this wand", MENU_UNSELECTED);
	else if (obj->otyp == UPGRADE_KIT)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Use the upgrade kit", MENU_UNSELECTED);
	else if (obj->oartifact == ART_DARKWEAVER_S_CLOAK)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Release or absorb darkness", MENU_UNSELECTED);
	else if (obj->otyp == DROVEN_CLOAK)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Spin out or sweep up a web", MENU_UNSELECTED);
	else if (obj->otyp == EILISTRAN_ARMOR)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Recharge armor or switch arms on/off", MENU_UNSELECTED);
	else if (obj->oartifact == ART_AEGIS)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Change Aegis' form", MENU_UNSELECTED);
	else if (obj->otyp == RAKUYO || obj->otyp == RAKUYO_SABER)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Latch or unlatch your rakuyo", MENU_UNSELECTED);
	else if (obj->otyp == BLADE_OF_MERCY || obj->otyp == BLADE_OF_GRACE)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Latch or unlatch your blade of mercy", MENU_UNSELECTED);
	else if (obj->otyp == DOUBLE_FORCE_BLADE || obj->otyp == FORCE_BLADE)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Latch or unlatch your force blade", MENU_UNSELECTED);
	else if (obj->otyp == FORCE_SWORD)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Unlock your force whip", MENU_UNSELECTED);
	else if (obj->otyp == CANE)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Unlock your whip saw", MENU_UNSELECTED);
	else if (obj->otyp == HUNTER_S_AXE || obj->otyp == HUNTER_S_LONG_AXE)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Extend or collapse your hunter's axe", MENU_UNSELECTED);
	else if (obj->otyp == SAW_CLEAVER || obj->otyp == RAZOR_CLEAVER)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Open or close your cleaver", MENU_UNSELECTED);
	else if (obj->otyp == LONG_SAW || obj->otyp == SAW_SPEAR)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Open or close your saw", MENU_UNSELECTED);
	else if (obj->otyp == BOW_BLADE || obj->otyp == BLADED_BOW)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Open or close your bow", MENU_UNSELECTED);
	else if (obj->otyp == SHANTA_PATA || obj->otyp == TWINGUN_SHANTA)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Open or close your pata", MENU_UNSELECTED);
	else if (obj->otyp == SOLDIER_S_RAPIER || obj->otyp == SOLDIER_S_SABER)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Latch or unlatch your rapier", MENU_UNSELECTED);
	else if (obj->otyp == CHURCH_HAMMER || obj->otyp == HUNTER_S_SHORTSWORD || obj->otyp == CHURCH_BRICK
		|| obj->otyp == CHURCH_BLADE || obj->otyp == HUNTER_S_LONGSWORD || obj->otyp == CHURCH_SHEATH
		|| obj->otyp == CHIKAGE
	)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Sheath or unsheath your sword", MENU_UNSELECTED);
	else if (obj->oartifact == ART_SILVER_SKY)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Merge skies", MENU_UNSELECTED);
	else if (obj->oartifact == ART_SKY_REFLECTED)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Merge skies", MENU_UNSELECTED);
	else if (obj->otyp == TORCH || obj->otyp == SHADOWLANDER_S_TORCH || obj->otyp == MAGIC_TORCH)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Light or snuff this torch", MENU_UNSELECTED);
	else if (obj->otyp == SUNROD && !obj->lamplit)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Light this sunrod", MENU_UNSELECTED);
	else if (obj->otyp == TONITRUS)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Strike this tonitrus", MENU_UNSELECTED);
	else if (obj->otyp == SENSOR_PACK)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Use this sensor pack", MENU_UNSELECTED);
	else if (obj->otyp == HYPOSPRAY)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Inject an ampule with this hypospray", MENU_UNSELECTED);
	else if ((is_knife(obj) && !(obj->oartifact == ART_PEN_OF_THE_VOID && obj->ovara_seals&SEAL_MARIONETTE))
		&& (u.wardsknown & (WARD_TOUSTEFNA | WARD_DREPRUN | WARD_OTTASTAFUR | WARD_KAUPALOKI | WARD_VEIOISTAFUR | WARD_THJOFASTAFUR)))
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Carve a stave with this knife", MENU_UNSELECTED);
	else if (is_lightsaber(obj) && obj->oartifact != ART_INFINITY_S_MIRRORED_ARC && obj->otyp != KAMEREL_VAJRA)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Ignite or deactivate this lightsaber", MENU_UNSELECTED);
	else if (obj->oclass == SCOIN_CLASS)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Crush this soul coin", MENU_UNSELECTED);
	else if ((obj->otyp == VITAL_SOULSTONE || obj->otyp == SPIRITUAL_SOULSTONE) && objects[obj->otyp].oc_name_known)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Crush this soulstone", MENU_UNSELECTED);
	else if (obj->otyp == DIMENSIONAL_LOCK)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Activate this lock", MENU_UNSELECTED);
	else if (obj->otyp == ANTIMAGIC_RIFT || obj->otyp == CATAPSI_VORTEX)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Crush this flawed gem", MENU_UNSELECTED);
	else if (obj->otyp == CRYSTAL && obj->obj_material == FLESH)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Contemplate this chrysalis", MENU_UNSELECTED);
	else if (obj->otyp == MISOTHEISTIC_PYRAMID || obj->otyp == MISOTHEISTIC_FRAGMENT)
		add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
				"Shatter this pyramid", MENU_UNSELECTED);
	/* d: drop item, works on everything */
	any.a_void = (genericptr_t)dodrop;
	add_menu(win, NO_GLYPH, &any, 'd', 0, ATR_NONE,
			"Drop this item", MENU_UNSELECTED);
	/* e: eat item; eat.c provides is_edible to check */
	any.a_void = (genericptr_t)doeat;
	if (obj->otyp == TIN && uwep && uwep->otyp == TIN_OPENER)
		add_menu(win, NO_GLYPH, &any, 'e', 0, ATR_NONE,
				"Open and eat this tin with your tin opener", MENU_UNSELECTED);
	else if (obj->otyp == TIN)
		add_menu(win, NO_GLYPH, &any, 'e', 0, ATR_NONE,
				"Open and eat this tin", MENU_UNSELECTED);
	else if (is_edible(obj))
		add_menu(win, NO_GLYPH, &any, 'e', 0, ATR_NONE,
				"Eat this item", MENU_UNSELECTED);
	/* E: engrave with item */
	any.a_void = (genericptr_t)doengrave;
	if (obj->otyp == TOWEL)
		add_menu(win, NO_GLYPH, &any, 'E', 0, ATR_NONE,
				"Wipe the floor with this towel", MENU_UNSELECTED);
	else if (obj->otyp == MAGIC_MARKER)
		add_menu(win, NO_GLYPH, &any, 'E', 0, ATR_NONE,
				"Scribble graffiti on the floor", MENU_UNSELECTED);
	else if (obj->oclass == WEAPON_CLASS || obj->oclass == WAND_CLASS ||
			obj->oclass == GEM_CLASS || obj->oclass == RING_CLASS)
		add_menu(win, NO_GLYPH, &any, 'E', 0, ATR_NONE,
				"Write on the floor with this object", MENU_UNSELECTED);
	/* I: describe item, works on any items whose actual name is known */
	any.a_void = (genericptr_t)dotypeinv;
	if (objects[obj->otyp].oc_name_known)
		add_menu(win, NO_GLYPH, &any, 'I', 0, ATR_NONE,
				"Describe this item", MENU_UNSELECTED);
	/* p: pay for unpaid items */
	any.a_void = (genericptr_t)dopay;
	if ((mtmp = shop_keeper(*in_rooms(u.ux, u.uy, SHOPBASE))) &&
			inhishop(mtmp) && obj->unpaid)
		add_menu(win, NO_GLYPH, &any, 'p', 0, ATR_NONE,
				"Buy this unpaid item", MENU_UNSELECTED);
	/* q: drink item; strangely, this one seems to have no exceptions */
	any.a_void = (genericptr_t)dodrink;
	if (obj->oclass == POTION_CLASS)
		add_menu(win, NO_GLYPH, &any, 'q', 0, ATR_NONE,
				"Quaff this potion", MENU_UNSELECTED);
	/* Q: quiver throwable item
	   (Why are weapons not designed for throwing included, I wonder?) */
	any.a_void= (genericptr_t)dowieldquiver;
	if (obj->oclass == GEM_CLASS || obj->oclass == WEAPON_CLASS)
		add_menu(win, NO_GLYPH, &any, 'Q', 0, ATR_NONE,
				"Quiver this item for easy throwing", MENU_UNSELECTED);
	/* r: read item */
	any.a_void = (genericptr_t)doread;
	if (obj->otyp == FORTUNE_COOKIE)
		add_menu(win, NO_GLYPH, &any, 'r', 0, ATR_NONE,
				"Read the message inside this cookie", MENU_UNSELECTED);
	else if (obj->otyp == T_SHIRT)
		add_menu(win, NO_GLYPH, &any, 'r', 0, ATR_NONE,
				"Read the slogan on the shirt", MENU_UNSELECTED);
	else if (obj->oclass == SCROLL_CLASS)
		add_menu(win, NO_GLYPH, &any, 'r', 0, ATR_NONE,
				"Cast the spell on this scroll", MENU_UNSELECTED);
	else if (obj->oclass == TILE_CLASS){
		if(obj->otyp >= SYLLABLE_OF_STRENGTH__AESH && obj->otyp <= SYLLABLE_OF_SPIRIT__VAUL)
			add_menu(win, NO_GLYPH, &any, 'r', 0, ATR_NONE,
					"Speak the glyph on this tile", MENU_UNSELECTED);
		else if(obj->otyp >= ANTI_CLOCKWISE_METAMORPHOSIS_G && obj->otyp <= ORRERY_GLYPH)
			add_menu(win, NO_GLYPH, &any, 'r', 0, ATR_NONE,
					"Study the glyph on this shard", MENU_UNSELECTED);
		else if(obj->otyp >= APHANACTONAN_RECORD && obj->otyp <= APHANACTONAN_ARCHIVE)
			add_menu(win, NO_GLYPH, &any, 'r', 0, ATR_NONE,
					"Study the glyphs on this disk", MENU_UNSELECTED);
		else if(is_slab(obj))
			add_menu(win, NO_GLYPH, &any, 'r', 0, ATR_NONE,
					"Study the glyph on this slab", MENU_UNSELECTED);
	}
	else if (obj->oclass == SPBOOK_CLASS)
		add_menu(win, NO_GLYPH, &any, 'r', 0, ATR_NONE,
				"Study this spellbook", MENU_UNSELECTED);
	else if (obj->oartifact == ART_EXCALIBUR || obj->oartifact == ART_GLAMDRING || obj->oartifact == ART_ROD_OF_SEVEN_PARTS)
				add_menu(win, NO_GLYPH, &any, 'r', 0, ATR_NONE,
				"Read the inscription on this weapon", MENU_UNSELECTED);
	else if (obj->oartifact == ART_BOW_OF_SKADI)
				add_menu(win, NO_GLYPH, &any, 'r', 0, ATR_NONE,
				"Study the runes on this bow", MENU_UNSELECTED);
	else if (obj->oartifact == ART_GUNGNIR)
				add_menu(win, NO_GLYPH, &any, 'r', 0, ATR_NONE,
				"Study the runes on this spear", MENU_UNSELECTED);
	else if (obj->oartifact == ART_ITLACHIAYAQUE)
				add_menu(win, NO_GLYPH, &any, 'r', 0, ATR_NONE,
				"Read into the smoky depths of this shield", MENU_UNSELECTED);
	else if (obj->oartifact == ART_STAFF_OF_NECROMANCY)
				add_menu(win, NO_GLYPH, &any, 'r', 0, ATR_NONE,
				"Study the forbidden secrets of necromancy", MENU_UNSELECTED);
	else if (obj->oartifact == ART_STAFF_OF_AESCULAPIUS || obj->oartifact == ART_ESSCOOAHLIPBOOURRR)
				add_menu(win, NO_GLYPH, &any, 'r', 0, ATR_NONE,
				"Study the grand magic of healing", MENU_UNSELECTED);
	else if (obj->oartifact == ART_HOLY_MOONLIGHT_SWORD && obj->lamplit)
				add_menu(win, NO_GLYPH, &any, 'r', 0, ATR_NONE,
				"Study the curious depths of this sword", MENU_UNSELECTED);
	else if (obj->oartifact == ART_PEN_OF_THE_VOID)
				add_menu(win, NO_GLYPH, &any, 'r', 0, ATR_NONE,
				(mvitals[PM_ACERERAK].died > 0) ? "Inspect the twin blades" : "Inspect the blade", MENU_UNSELECTED);
	/* R: rub */
	any.a_void = (genericptr_t)dorub;
	if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP)
		add_menu(win, NO_GLYPH, &any, 'R', 0, ATR_NONE,
				"Rub this lamp", MENU_UNSELECTED);
	else if (obj->otyp == LANTERN)
		add_menu(win, NO_GLYPH, &any, 'R', 0, ATR_NONE,
				"Rub this lantern", MENU_UNSELECTED);
#if 0 /* TODO */
	else if (obj->oclass == GEM_CLASS && is_graystone(obj))
		add_menu(win, NO_GLYPH, &any, 'R', 0, ATR_NONE,
				"Rub something on this stone", MENU_UNSELECTED);
#endif
	/* t: throw item, works on everything */
	any.a_void = (genericptr_t)dothrow;
	add_menu(win, NO_GLYPH, &any, 't', 0, ATR_NONE,
			"Throw this item", MENU_UNSELECTED);
	/* T: unequip worn item */
	if ((obj->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL | W_BELT))) {
	    if ((obj->owornmask & (W_ARMOR)))
		any.a_void = (genericptr_t)dotakeoff;
	    if ((obj->owornmask & (W_RING | W_AMUL | W_TOOL | W_BELT)))
		any.a_void = (genericptr_t)doremring;
	    add_menu(win, NO_GLYPH, &any, 'T', 0, ATR_NONE,
		     "Unequip this equipment", MENU_UNSELECTED);
	}
	/* V: invoke, rub, or break */
	any.a_void = (genericptr_t)doinvoke;
	if ((obj->otyp == FAKE_AMULET_OF_YENDOR && !obj->known) ||
			obj->oartifact || objects[obj->otyp].oc_unique ||
			(obj->otyp == RIN_WISHES && objects[obj->otyp].oc_name_known && (obj->owornmask & W_RING)) ||
			(obj->otyp == CANDLE_OF_INVOCATION && obj->lamplit) ||
			obj->otyp == MIRROR) /* wtf NetHack devteam? */
		add_menu(win, NO_GLYPH, &any, 'V', 0, ATR_NONE,
				"Try to invoke a unique power of this object", MENU_UNSELECTED);
	/* w: hold in hands, works on everything but with different
	   advice text; not mentioned for things that are already
	   wielded */
	any.a_void = (genericptr_t)dowield;
	if (obj == uwep) {}
	else if (obj->oclass == WEAPON_CLASS || is_weptool(obj))
		add_menu(win, NO_GLYPH, &any, 'w', 0, ATR_NONE,
				"Wield this as your weapon", MENU_UNSELECTED);
	else if (obj->otyp == TIN_OPENER)
		add_menu(win, NO_GLYPH, &any, 'w', 0, ATR_NONE,
				"Hold the tin opener to open tins", MENU_UNSELECTED);
	else
		add_menu(win, NO_GLYPH, &any, 'w', 0, ATR_NONE,
				"Hold this item in your hands", MENU_UNSELECTED);
	/* W: Equip this item */
	if (!(obj->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL | W_BELT))) {
	    any.a_void = (genericptr_t)dowear;
	    if (obj->oclass == ARMOR_CLASS)
		add_menu(win, NO_GLYPH, &any, 'W', 0, ATR_NONE,
				"Wear this armor", MENU_UNSELECTED);
	    else if (obj->oclass == RING_CLASS || obj->otyp == MEAT_RING) {
		any.a_void = (genericptr_t)doputon;
		add_menu(win, NO_GLYPH, &any, 'W', 0, ATR_NONE,
				"Put this ring on", MENU_UNSELECTED);
	    } else if (obj->oclass == AMULET_CLASS) {
		any.a_void = (genericptr_t)doputon;
		add_menu(win, NO_GLYPH, &any, 'W', 0, ATR_NONE,
				"Put this amulet on", MENU_UNSELECTED);
	    } else if (obj->otyp == TOWEL || obj->otyp == BLINDFOLD || obj->otyp == R_LYEHIAN_FACEPLATE || obj->otyp == ANDROID_VISOR) {
		any.a_void = (genericptr_t)doputon;
		add_menu(win, NO_GLYPH, &any, 'W', 0, ATR_NONE,
				"Use this to blindfold yourself", MENU_UNSELECTED);
	    } else if (obj->otyp == LENSES || obj->otyp == SUNGLASSES) {
		any.a_void = (genericptr_t)doputon;
		add_menu(win, NO_GLYPH, &any, 'W', 0, ATR_NONE,
				"Put these lenses on", MENU_UNSELECTED);
	    } else if (obj->otyp == SOUL_LENS) {
		any.a_void = (genericptr_t)doputon;
		add_menu(win, NO_GLYPH, &any, 'W', 0, ATR_NONE,
				"Put this lens on", MENU_UNSELECTED);
	    } else if (obj->otyp == MASK || obj->otyp == LIVING_MASK) {
		any.a_void = (genericptr_t)doputon;
		add_menu(win, NO_GLYPH, &any, 'W', 0, ATR_NONE,
				"Put this mask on", MENU_UNSELECTED);
	    }
	}
	/* x: Swap main and readied weapon */
	any.a_void = (genericptr_t)doswapweapon;
	if (obj == uwep && uswapwep)
		add_menu(win, NO_GLYPH, &any, 'x', 0, ATR_NONE,
				"Swap this with your alternate weapon", MENU_UNSELECTED);
	else if (obj == uwep)
		add_menu(win, NO_GLYPH, &any, 'x', 0, ATR_NONE,
				"Ready this as an alternate weapon", MENU_UNSELECTED);
	else if (obj == uswapwep)
		add_menu(win, NO_GLYPH, &any, 'x', 0, ATR_NONE,
				"Swap this with your main weapon", MENU_UNSELECTED);
	/* z: Zap wand */
	any.a_void = (genericptr_t)dozap;
	if (obj->oclass == WAND_CLASS)
		add_menu(win, NO_GLYPH, &any, 'z', 0, ATR_NONE,
				"Zap this wand to release its magic", MENU_UNSELECTED);
	/* S: Sacrifice object (should be > but that causes problems) */
#if 0 /* TODO */
	any.a_void = (genericptr_t)doterrain;
	if (IS_ALTAR(levl[u.ux][u.uy].typ) && !u.uswallow) {
		if (obj->otyp == CORPSE)
			add_menu(win, NO_GLYPH, &any, 'S', 0, ATR_NONE,
					"Sacrifice this corpse at this altar", MENU_UNSELECTED);
		else if (obj->otyp == AMULET_OF_YENDOR ||
				obj->otyp == FAKE_AMULET_OF_YENDOR)
			add_menu(win, NO_GLYPH, &any, 'S', 0, ATR_NONE,
					"Sacrifice this amulet at this altar", MENU_UNSELECTED);
	}
#endif

	Sprintf(prompt, "Do what with %s?", the(cxname(obj)));
	end_menu(win, prompt);

	n = select_menu(win, PICK_ONE, &selected);
	destroy_nhwindow(win);
	if (n == 1) feedback_fn = (int NDECL((*)))selected[0].item.a_void;
	if (n == 1) free((genericptr_t) selected);

	if (!feedback_fn) return 0;
#if 0
	/* dodip() is special, because it takes the item to dip first, and
	   the item to dip /into/ second. */
	if (feedback_fn == dodip) {
		setnextdodipinto(obj);
		return dodip();
	}
#endif
	/* dotypeinv() means that we want the item described. Just do it
	   directly rather than fighting with a multiselect menu. */
	if (feedback_fn == dotypeinv) {
		winid datawin = create_nhwindow(NHW_MENU);
		putstr(datawin, ATR_NONE, doname(obj));
		describe_item(obj, obj->otyp, obj->oartifact, &datawin);
		checkfile(xname_bland(obj), 0, FALSE, TRUE, &datawin);
		display_nhwindow(datawin, TRUE);
		destroy_nhwindow(datawin);
		return 0;
	}
	/* In most cases, we can just set getobj's result directly.
	   (This works even for commands that take no arguments, because
	   they don't call getobj at all. */
	nextgetobj = obj;
	n = (*feedback_fn)();
	nextgetobj = 0;

	return n;
}
/* 
 * describe_item()
 * 
 * Prints lines describing the given object to the passed nhwindow reference
 * Requires that the player knows the name of the object (ie, objects[obj->otyp].oc_name_known == TRUE)
 *   or that obj doesn't exist (ie we are describing what a "cloak of magic resistance" typically is like)
 * Requires an actual object to be passed, as many object-related functions require an obj pointer
 */
void
describe_item(obj, otyp, oartifact, datawin)
struct obj *obj;
int otyp;
int oartifact;
winid *datawin;
{
	if (obj)
	{
		otyp = obj->otyp;
		if(obj->known)
			oartifact = obj->oartifact;
		else oartifact = 0;
	}
	struct objclass oc = objects[otyp];
	char olet = oc.oc_class;
	char buf[BUFSZ];
	char buf2[BUFSZ];
	const char* dir = (oc.oc_dir == NODIR ? "Non-directional"
		: (oc.oc_dir == IMMEDIATE ? "Beam"
		: "Ray"));
	int goatweaponturn = 0;
	int sothweaponturn = 0;
	boolean printed_type = FALSE;
	boolean has_artidmg = oartifact && (artilist[oartifact].adtyp || artilist[oartifact].damage || artilist[oartifact].accuracy);
	
	if(check_oprop(obj,OPROP_GOATW) && !(obj->where == OBJ_INVENT && GOAT_BAD))
		goatweaponturn = goat_weapon_damage_turn(obj);
	if(check_oprop(obj,OPROP_SOTHW) && !(obj->where == OBJ_INVENT && YOG_BAD))
		sothweaponturn = soth_weapon_damage_turn(obj);


#define OBJPUTSTR(str) putstr(*datawin, ATR_NONE, str)
#define ADDCLASSPROP(cond, str)         \
	if (cond) {							\
	if (*buf) { Strcat(buf, ", "); }    \
	Strcat(buf, str);                   \
	}

	/* check that we aren't giving away information about a real object here */
	if (obj && !oc.oc_name_known)
	{
		OBJPUTSTR("You don't know much about this item yet.");
		return;
	}

	/* hide artifact properties if we haven't discovered the artifact yet, and we aren't speaking in hypotheticals */
	if (obj && undiscovered_artifact(oartifact))
		oartifact = 0;

	/* type name */
	if (obj) {
		Sprintf(buf, "(%s)", obj_descname(obj));
		OBJPUTSTR(buf);
	}
	else {
		/* assume we are asking about a "cloak of magic resistance", not a "piece of cloth" */
		boolean savestate = objects[otyp].oc_name_known;
		objects[otyp].oc_name_known = TRUE;
		Sprintf(buf, "(%s)", artiadjusted_objnam(simple_typename(otyp), oartifact));
		objects[otyp].oc_name_known = savestate;
		OBJPUTSTR(buf);
	}

	/* Object classes currently with no special messages here: amulets. */
	if (olet == WEAPON_CLASS || (olet == TOOL_CLASS && oc.oc_skill) || otyp == BALL || olet == GEM_CLASS || has_artidmg) {
		int mask = attack_mask(obj, otyp, oartifact, &youmonst);
		boolean otyp_is_blaster = (otyp == CARCOSAN_STING || otyp == HAND_BLASTER || otyp == ARM_BLASTER || otyp == MASS_SHADOW_PISTOL || otyp == CUTTING_LASER || otyp == RAYGUN);
		boolean otyp_is_launcher = (((oc.oc_skill >= P_BOW && oc.oc_skill <= P_CROSSBOW) || otyp == ATLATL) && !otyp_is_blaster);
		/* armor and rings don't have meaningful base damage, but can have artifact bonus damage */
		boolean artidmg_only = olet == ARMOR_CLASS || olet == RING_CLASS;

		/* print type */
		if (!printed_type && !artidmg_only) {
			buf2[0] = '\0';
			int i;
			static const char * damagetypes[] = { "blunt", "piercing", "slashing", "energy" };
			for (i = 0; i < 4; i++) {
				if (mask & (1 << i)) {
					if (eos(buf2) != buf2)
						Strcat(buf2, "/");
					Strcat(buf2, damagetypes[i]);
				}
			}
			if (otyp == FRAG_GRENADE || otyp == GAS_GRENADE || otyp == ROCKET || otyp == STICK_OF_DYNAMITE)
				Strcpy(buf2, "explosive");
			Strcat(buf2, " ");

			if (oc.oc_skill > 0) {
				if (obj) {
					Sprintf(buf, "%s-handed %s%s%s.", 
						((obj ? bimanual(obj, youracedata) : oc.oc_bimanual) ? "Two" : "One"),
						(otyp_is_blaster || otyp_is_launcher) ? "" : buf2,
						(otyp_is_blaster ? "blaster" : otyp_is_launcher ? "launcher" : "weapon"),
						((obj && is_weptool(obj)) && !otyp_is_launcher ? "-tool" : "")
						);
				}
				else {
					Sprintf(buf, "%s-handed %s%s%s.",
						(oc.oc_bimanual ? "Two" : "Single"),
						(otyp_is_blaster || otyp_is_launcher) ? "" : buf2,
						(otyp_is_launcher ? "launcher" : "weapon"),
						((olet == TOOL_CLASS && oc.oc_skill) ? "-tool" : ""));
				}
			}
			else if (oc.oc_skill <= -P_BOW && oc.oc_skill >= -P_CROSSBOW) {
				Sprintf(buf, "%sammunition.", upstart(buf2));
			}
			else if (olet == WAND_CLASS) {
				Sprintf(buf, "%s wand.", dir);
			}
			else {
				Sprintf(buf, "Thrown %smissile.", buf2);
			}
			/* special cases */
			if (oartifact == ART_PEN_OF_THE_VOID && obj && (obj->ovara_seals & SEAL_EVE))
				Strcpy(eos(buf)-1, ", and launcher.");
			if (oartifact == ART_LIECLEAVER || oartifact == ART_ROGUE_GEAR_SPIRITS || oartifact == ART_WAND_OF_ORCUS || otyp == CARCOSAN_STING)
				Sprintf(eos(buf)-1, ", and %smelee weapon.", buf2);
			OBJPUTSTR(buf);
			printed_type = TRUE;
		}

		/* what skill does it use? */
		if (obj ? weapon_type(obj) : oc.oc_skill != P_NONE && !artidmg_only) {
			if (obj) {
				Sprintf(buf, "Uses your %s skill", P_NAME(abs(weapon_type(obj))));
				/* special cases */
				switch (oartifact) {
					case ART_LIECLEAVER:
						Strcpy(buf2, " at range, and your scimitar skill in melee.");
						break;
					case ART_ROGUE_GEAR_SPIRITS:
						Strcpy(buf2, " at range, and your pickaxe skill in melee.");
						break;
					case ART_PEN_OF_THE_VOID:
						if(obj->ovara_seals & SEAL_EVE) {
							Strcpy(buf2, " in melee, and your ammo's skill at range.");
						}
						else
							Strcpy(buf2, ".");
						break;
					default:
						Strcpy(buf2, ".");
				}
				if(obj->otyp == CARCOSAN_STING){
					Strcpy(buf2, " at range, and your dagger skill in melee.");
				}
				Strcat(buf, buf2);
			} else {
				Sprintf(buf, "Uses the %s skill.", P_NAME(abs(oc.oc_skill)));
			}
			OBJPUTSTR(buf);
		}

		/* weapon dice! */
		/* Does not apply for launchers. */
		/* the melee-weapon artifact launchers need obj to exist because dmgval_core needs obj to find artifact. */
		if ((!otyp_is_launcher && !otyp_is_blaster && !artidmg_only) || (
			(otyp == CARCOSAN_STING) ||
			(obj && oartifact == ART_LIECLEAVER) ||
			(obj && oartifact == ART_WAND_OF_ORCUS) ||
			(obj && oartifact == ART_ROGUE_GEAR_SPIRITS)
			))
		{
			// note: dmgval_core can handle not being given an obj; it will attempt to use otyp instead
			// impractical edge case for varying spe_mult with size
			struct weapon_dice wdice[2];
			int spe_mult = dmgval_core(&wdice[0], FALSE, obj, otyp, &youmonst);	// small dice
			int lspe_mult = dmgval_core(&wdice[1], TRUE, obj, otyp, &youmonst);		// large dice
			int enc_bonus = (obj) ? (obj->spe) : 0;
			if (otyp == CRYSTAL_SWORD) enc_bonus += enc_bonus / 3;
			if (otyp == SEISMIC_HAMMER) {
				wdice[0].oc_damd += 3*enc_bonus;
				wdice[1].oc_damd += 3*enc_bonus;
			}

			Sprintf(buf, "Damage: ");

			if (wdice[0].oc_damn && wdice[0].oc_damd)
			{
				Sprintf(buf2, "%dd%d", wdice[0].oc_damn, wdice[0].oc_damd);
				Strcat(buf, buf2);
			}
			if (wdice[0].bon_damn && wdice[0].bon_damd)
			{
				Sprintf(buf2, "+%dd%d", wdice[0].bon_damn, wdice[0].bon_damd);
				Strcat(buf, buf2);
			}
			if (wdice[0].flat || enc_bonus)
			{
				Sprintf(buf2, "%s", sitoa(wdice[0].flat + enc_bonus*spe_mult));
				Strcat(buf, buf2);
			}
			Strcat(buf, " versus small and ");
			/* is there a difference between large and small dice? */
			if (wdice[0].oc_damn != wdice[1].oc_damn ||
				wdice[0].oc_damd != wdice[1].oc_damd ||
				wdice[0].bon_damn != wdice[1].bon_damn ||
				wdice[0].bon_damd != wdice[1].bon_damd ||
				wdice[0].flat != wdice[1].flat)
			{
				if (wdice[1].oc_damn && wdice[1].oc_damd)
				{
					Sprintf(buf2, "%dd%d", wdice[1].oc_damn, wdice[1].oc_damd);
					Strcat(buf, buf2);
				}
				if (wdice[1].bon_damn && wdice[1].bon_damd)
				{
					Sprintf(buf2, "+%dd%d", wdice[1].bon_damn, wdice[1].bon_damd);
					Strcat(buf, buf2);
				}
				if (wdice[1].flat || enc_bonus)
				{
					Sprintf(buf2, "%s", sitoa(wdice[1].flat + enc_bonus*lspe_mult));
					Strcat(buf, buf2);
				}
				Strcat(buf, " versus ");
			}
			Strcat(buf, "large monsters.");
			OBJPUTSTR(buf);
		}
		/* artifact bonus damage (artifacts only) */
		if (has_artidmg)
		{
			register const struct artifact *oart = &artilist[oartifact];
			/* bonus damage, or double damage? We already checked that oart->attk exists */
			if (oart->damage && !(
				(obj && oartifact == ART_ANNULUS && !is_lightsaber(obj))	/* annulus is 2x damage if not in lightsaber form */
			))
			{// 1dX bonus damage
				if (oart->damage > 1)
				{
					Sprintf(buf, "Deals %dd%d bonus ",
						((obj && obj->oartifact == ART_SILVER_STARLIGHT) ? 4 : (obj && is_lightsaber(obj) && litsaber(obj)) ? 3 : 1) * (double_bonus_damage_artifact(oartifact) ? 2 : 1),
						oart->damage);
				}
				else
				{
					Sprintf(buf, "Deals %d bonus ",
						((obj && is_lightsaber(obj) && litsaber(obj)) ? 3 : 1) * (double_bonus_damage_artifact(oartifact) ? 2 : 1));
				}
			}
			else
			{// double damage (except when it's triple)
				Sprintf(buf, "Deals %s ",
					double_bonus_damage_artifact(oartifact) ? "triple" : "double");
			}

			/* now figure out who it deals the bonus damage to */
			switch (oartifact)
			{
			case ART_HOLY_MOONLIGHT_SWORD:
				Strcat(buf, "magic damage, while lit.");
				break;
			case ART_PEN_OF_THE_VOID:
				/* overwrite the previous mention of how much the bonus is*/
				if (mvitals[PM_ACERERAK].died > 0)
				{
					Sprintf(buf, "Deals double damage");
					if (obj->ovara_seals)
						Strcat(buf, ", and enhanced spirit bonus damage.");
					else
						Strcat(buf, ".");
				}
				else
				{
					if (obj->ovara_seals)
						Sprintf(buf, "Deals bonus damage from the spirit bound into it.");
					else
						buf[0] = '\0';
				}
				break;
			case ART_LIFEHUNT_SCYTHE:
			case ART_PROFANED_GREATSCYTHE:
				Strcat(buf, "damage to living and undead.");
				break;
			case ART_GIANTSLAYER:
				Strcat(buf, "damage to large creatures.");
				break;
			case ART_FALLINGSTAR_MANDIBLES:
				Strcat(buf, "magic damage, doubled against those who came from the stars.");
				break;
			default:
				switch (oart->adtyp) {
					case AD_FIRE: Strcat(buf, "fire damage"); break;
					case AD_COLD: Strcat(buf, "cold damage"); break;
					case AD_ELEC: Strcat(buf, "lightning damage"); break;
					case AD_ACID: Strcat(buf, "acid damage"); break;
					case AD_MAGM: Strcat(buf, "magic damage"); break;
					case AD_DRST: Strcat(buf, "poison damage"); break;
					case AD_DRLI: Strcat(buf, "life drain damage"); break;
					case AD_STON: Strcat(buf, "petrifying damage"); break;
					case AD_DARK: Strcat(buf, "dark damage"); break;
					case AD_BLUD: Strcat(buf, "blood damage"); break;
					case AD_HOLY: Strcat(buf, "holy damage"); break;
					case AD_STDY: Strcat(buf, "study damage"); break;
					case AD_HLUH: Strcat(buf, "corrupted holy damage"); break;
					case AD_STAR: Strcat(buf, "silver damage"); break;
					case AD_SLEE: Strcat(buf, "sleep damage"); break;
					case AD_PHYS: Strcat(buf, "damage"); break;
					default: break;
				}
				if (oart->aflags&ARTA_HATES){
					Strcat(buf, " to ");
					/* SMOP: this should be made into a list somewhere and used for specific warning messages as well,
					 * ie, "Warned of fey and magic-item users." instead of "warned of demiliches"
					 * Update 2024 - flag_to_word(flag, cat) now exists, but it's missing a good way to handle msym
					 * pluralizations and would lose the fun flavor-ish messages for things like nodens, etc.
					 */

					switch (oartifact){
					case ART_ORCRIST:					Strcat(buf, "orcs and demons.");							break;
					case ART_STING:						Strcat(buf, "orcs and spiders.");							break;
					case ART_GRIMTOOTH:					Strcat(buf, "humans, elves, and dwarves.");					break;
					case ART_CARNWENNAN:				Strcat(buf, "fey and magic-item users.");					break;
					case ART_SLAVE_TO_ARMOK:			Strcat(buf, "nobility, elves, orcs, and the innocent.");	break;
					case ART_CLAIDEAMH:					Strcat(buf, "those bound by iron and ancient laws.");		break;
					case ART_DRAGONLANCE:				Strcat(buf, "dragons.");									break;
					case ART_NODENSFORK:				Strcat(buf, "the eldritch, the telepathic, the deep.");		break;
					case ART_GAIA_S_FATE:				Strcat(buf, "things attuned to nature.");					break;
					case ART_DEMONBANE:					Strcat(buf, "demons.");										break;
					case ART_WEREBANE:					Strcat(buf, "were-beasts and demihumans.");					break;
					case ART_GIANTSLAYER:				Strcat(buf, "giants.");										break;
					case ART_VAMPIRE_KILLER:			Strcat(buf, "demons, were-beasts and the undead.");			break;
					case ART_KINGSLAYER:				Strcat(buf, "lords and ladies and kings and queens.");		break;
					case ART_PEACE_KEEPER:				Strcat(buf, "those that always seek violence.");			break;
					case ART_OGRESMASHER:				Strcat(buf, "ogres.");										break;
					case ART_TROLLSBANE:				Strcat(buf, "trolls and all who regenerate quickly.");		break;
					case ART_SUNSWORD:					Strcat(buf, "demons and the undead.");						break;
					case ART_ATMA_WEAPON:				Strcat(buf, "worthy foes.");								break;
					case ART_ROD_OF_SEVEN_PARTS:		Strcat(buf, "any who stand against law.");					break;
					case ART_WEREBUSTER:				Strcat(buf, "were-beasts.");								break;
					case ART_BLACK_CRYSTAL:				Strcat(buf, "non-chaotics.");								break;
					case ART_CLARENT:					Strcat(buf, "the thick-skinned");							break;
					case ART_WEB_OF_LOLTH:				Strcat(buf, "elves, demons, and godly minions.");			break;
					case ART_MITRE_OF_HOLINESS:			Strcat(buf, "undead.");										break;
					case ART_ICONOCLAST:				Strcat(buf, "humans, elves, dwarves, and gnomes.");			break;
					case ART_SCOURGE_OF_LOLTH:			Strcat(buf, "elves.");										break;
					default:
						if (oart->aflags&ARTA_CROSSA) Strcat(buf, "cross-aligned foes.");
						else Strcat(buf, "hated foes.");
						break;
					}
				} else {
					Strcat(buf, ".");
				}
			break;
			}
			if (buf[0] != '\0')
				OBJPUTSTR(buf);
		}

		/* other weapon special effects */
		if(obj){
			int ldamd = objects[otyp].oc_wldam.oc_damd;
			int sdamd = objects[otyp].oc_wsdam.oc_damd;
			if(obj->otyp == TOOTH && Insight >= 20 && obj->o_e_trait&ETRAIT_FOCUS_FIRE && CHECK_ETRAIT(obj, &youmonst, ETRAIT_FOCUS_FIRE)){
				if(obj->ovar1_tooth_type == MAGMA_TOOTH){
					Sprintf(buf2, "Deals +5d10%s fire damage.", (obj->spe ? sitoa(obj->spe) : ""));
					OBJPUTSTR(buf2);
				}
				else if(obj->ovar1_tooth_type == VOID_TOOTH){
					Sprintf(buf2, "Drains three levels from the target and deals +3d3%s cold damage.", (obj->spe ? sitoa(obj->spe) : ""));
					OBJPUTSTR(buf2);
				}
				else if(obj->ovar1_tooth_type == SERPENT_TOOTH){
					Sprintf(buf2, "Injects dire poison and deals +1d8%s poison and +1d8%s acid damage.",(obj->spe ? sitoa(obj->spe) : ""),(obj->spe ? sitoa(obj->spe) : ""));
					OBJPUTSTR(buf2);
				}
			}
			if(obj->otyp == TORCH){
				Sprintf(buf2, "When lit, deals +1d10%s fire damage.", (obj->spe ? sitoa(obj->spe) : ""));
				OBJPUTSTR(buf2);
			}
			if(obj->otyp == MAGIC_TORCH){
				Sprintf(buf2, "When lit, deals +1d8%s fire damage.", (obj->spe ? sitoa(2*obj->spe) : ""));
				OBJPUTSTR(buf2);
			}
			if(obj->otyp == SHADOWLANDER_S_TORCH){
				Sprintf(buf2, "When lit, deals +1d10%s cold damage.", (obj->spe ? sitoa(obj->spe) : ""));
				OBJPUTSTR(buf2);
			}
			if(obj->otyp == SUNROD){
				Sprintf(buf2, "When lit, deals +1d10%s lightning and acid damage and may blind struck targets.", (obj->spe ? sitoa(obj->spe) : ""));
				OBJPUTSTR(buf2);
			}
			if(obj->otyp == TONITRUS){
				Sprintf(buf2, "When lit, deals +1d10%s lightning damage.", (obj->spe ? sitoa(obj->spe) : ""));
				OBJPUTSTR(buf2);
			}
			if(obj->otyp == KAMEREL_VAJRA){
				Sprintf(buf2, "When lit, deals +2d6 lightning damage, or +6d6 if wielded by an Ara Kamerel, and may blind struck targets.");
				OBJPUTSTR(buf2);
			}
			if(obj->otyp == VIPERWHIP && obj->ovar1_heads > 1){
				Sprintf(buf2, "May strike with up to %d heads at once, multiplying the damage accordingly.", (int)(obj->ovar1_heads));
				OBJPUTSTR(buf2);
			}
			if(obj->otyp == MIRRORBLADE){
				Sprintf(buf2, "Uses a weapon-wielding opponent's own weapon against them, if their damage is higher.");
				OBJPUTSTR(buf2);
			}
			if(obj->otyp == CRYSTAL_SWORD && obj->spe >= 3){
				Sprintf(buf2, "Adds an extra %s to enchantment for damage calculations.", sitoa(obj->spe/3));
				OBJPUTSTR(buf2);
			}
			if(force_weapon(obj)){
				if (obj->otyp == FORCE_WHIP)
					Sprintf(buf2, "When charged, deals +1d%d%s energy damage versus small and +2d%d%s versus large.",
						sdamd, sitoa((sdamd+1)/2+2), ldamd, sitoa((ldamd+1)/2+2));
				else
					Sprintf(buf2, "When charged, deals +1d%d%s energy damage versus small and +1d%d%s versus large.",
						sdamd, sitoa((sdamd+1)/2), ldamd, sitoa((ldamd+1)/2));
				OBJPUTSTR(buf2);
			}
			if(obj->otyp == DOUBLE_FORCE_BLADE){
				Sprintf(buf2, "Doubles base damage if wielded without two-weaponing, at the cost of an extra 1/4 move.");
				OBJPUTSTR(buf2);
			}
			if(obj->otyp == RAKUYO){
				Sprintf(buf2, "Deals +1d%d%s base damage vs small and +1d%d%s vs large if wielded without two-weaponing, at the cost of an extra 1/4 move.", 
					(4 + 2*(obj->objsize - MZ_MEDIUM)), (obj->spe ? sitoa(obj->spe) : ""),
					(3 + 2*(obj->objsize - MZ_MEDIUM)), (obj->spe ? sitoa(obj->spe) : ""));
				OBJPUTSTR(buf2);
			}
			if(obj->otyp == DISKOS && Insight >= 15){
				int dice = (Insight >= 45) ? 3 : ((Insight >= 20) ? 2 : 1);
				int lflat = (Insight >= 50) ? ldamd : 0;
				int sflat = (Insight >= 50) ? sdamd : 0;

				if(u.ualign.record < -3 && Insanity > 50){
					lflat += ldamd*(50-u.usanity)/50;
					sflat += sdamd*(50-u.usanity)/50;
				}
				else if(u.ualign.record > 3 && u.usanity > 90){
					lflat += ldamd*(10-Insanity)/10;
					sflat += sdamd*(10-Insanity)/10;
				}

				Sprintf(buf2, "Deals +%dd%d%s %senergy damage versus small and +%dd%d%s versus large.",
					dice, sdamd, (sflat ? sitoa(sflat) : ""), (u.ualign.record < -3) ? "unholy " : ((u.ualign.record > 3) ? "holy " : ""), 
					dice, ldamd, (lflat ? sitoa(lflat) : ""));
				OBJPUTSTR(buf2);
			}
			if(mercy_blade_prop(obj) & !u.veil){
				Sprintf(buf2, "Deals extra damage scaled by insight%s, currently %d%% extra damage.",
					(Insight >= 25) ? " and charisma" : "",
					(min(Insight, 50) + ((Insight >= 25) ? min((Insight-25)/2, ACURR(A_CHA)) : 0))*2);
				OBJPUTSTR(buf2);

				if (Insight >= 25){
					Sprintf(buf2, "Lowers struck targets' morale based on your charisma, currently -%d per hit, capped at -%d.",
						ACURR(A_CHA)/5, (obj->spe + ACURR(A_CHA)));
					OBJPUTSTR(buf2);
				}
			}
			if(obj->otyp == CROW_QUILL || obj->otyp == SET_OF_CROW_TALONS){
				Sprintf(buf2, "Makes struck targets vulnerable, adding %d stacks per hit.", (obj->otyp == CROW_QUILL) ? 4 : 3);
				OBJPUTSTR(buf2);
			}
			if(obj->otyp == BESTIAL_CLAW && active_glyph(BEASTS_EMBRACE)){
				Sprintf(buf2, "Makes struck targets vulnerable, adding stacks equal to 10%% of damage, capped at %d (scaling inversely with insight).",
					(int)(30*pow(.97, Insight)));
				OBJPUTSTR(buf2);
			}
			if(obj->otyp == ISAMUSEI){
				int factor = 20;
				if(Insight >= 70){
					factor = 4;
				}
				else if(Insight >= 57){
					factor = 5;
				}
				else if(Insight >= 45){
					factor = 6;
				}
				else if(Insight >= 33){
					factor = 8;
				}
				else if(Insight >= 22){
					factor = 10;
				}
				Sprintf(buf2, "Attempts to lower the target's current health by %d%% of its current value.", 100/factor);
				OBJPUTSTR(buf2);
			}
			if(obj->otyp == PINCER_STAFF && Insight >= 10){
				Sprintf(buf2, "Deals double base damage %s%son consecutive attacks against the same target.",
					(obj->oartifact == ART_FALLINGSTAR_MANDIBLES) ? "plus 1d12 magic damage " : "",
					(Insight >= 50) ? "and attempts to steal worn armor " : "");
				OBJPUTSTR(buf2);
			}
			if(obj->oartifact == ART_ESSCOOAHLIPBOOURRR){
				Sprintf(buf2, "Deals double base damage on consecutive attacks against the same target.");
				OBJPUTSTR(buf2);
			}
			if(obj->oartifact == ART_ROD_OF_SEVEN_PARTS){
				Sprintf(buf2, "Gains +1 enchantment for every 7 kills, capped at %d.", min(u.ulevel/3, 7));
				OBJPUTSTR(buf2);
			}
			if(obj->oartifact == ART_RUINOUS_DESCENT_OF_STARS){
				if (Insight >= 6){
					Sprintf(buf2, "Deals +%dd6 fire damage, scaling with your insight.", min(6, Insight/6));
					OBJPUTSTR(buf2);
				}
				if (NightmareAware_Insanity >= 4){
					Sprintf(buf2, "Deals +%dd%d acid damage, scaling with your missing sanity.",
						ClearThoughts ? 1 : 2, min(12, NightmareAware_Insanity/4));
					OBJPUTSTR(buf2);
				}
				if (Insight >= 42){
					Sprintf(buf2, "Randomly drops falling stars on a hit, causing up to %d physical & fiery explosions centered anywhere on the level.",
						min(6, (Insight - 36)/6));
					OBJPUTSTR(buf2);
				}
			}
			if(obj->oartifact == ART_ARYVELAHR_KERYM){
				Sprintf(buf2, "Deals +2d10 damage to orcs, demons, drow, and the undead.");
				OBJPUTSTR(buf2);
			}
			if(obj->oartifact == ART_SILVER_STARLIGHT){
				Sprintf(buf2, "Deals an extra base die + 1d4 bonus precision damage.");
				OBJPUTSTR(buf2);
			}
			if (obj->oartifact == ART_SHADOWLOCK){
				Sprintf(buf2, "Deals +2d6%s cold damage, but also deals 4d6%s physical and 2d6%s cold damage to the wielder every hit.",
					(obj->spe ? sitoa(obj->spe) : ""), (obj->spe ? sitoa(obj->spe) : ""), (obj->spe ? sitoa(obj->spe) : ""));
				OBJPUTSTR(buf2);
			}
			if(obj->oartifact == ART_TROLLSBANE){
				Sprintf(buf2, "Deals +2d20%s to covetous monsters.", (obj->spe ? sitoa(obj->spe*2) : ""));
				OBJPUTSTR(buf2);
			}
			if(obj->oartifact == ART_MASAMUNE){
				int experts = -2;
				for(int skl = 1; skl < P_NUM_SKILLS; skl++){
					if(P_SKILL(skl) >= P_EXPERT)
						experts += 2;
				}
				Sprintf(buf2, "Deals +2 damage for every expert weapon skill past your first, currently %s.", sitoa(max(0, experts)));
				OBJPUTSTR(buf2);
			}
			if(obj->oartifact == ART_KUSANAGI_NO_TSURUGI){
				Sprintf(buf2, "Instantly kills struck elementals and vortices.");
				OBJPUTSTR(buf2);
			}
			if(obj->oartifact == ART_GIANTSLAYER){
				Sprintf(buf2, "Hamstrings giants & other boulder-throwers, reducing their speed by 6 movement points.");
				OBJPUTSTR(buf2);
			}
			if(obj->oartifact == ART_TECPATL_OF_HUHETOTL){
				Sprintf(buf2, "Drinks the blood of targets that have any, dealing +2d4 damage and potentially sacrificing slain foes.");
				OBJPUTSTR(buf2);
			}
			if(obj->oartifact == ART_PLAGUE && monstermoves < artinstance[ART_PLAGUE].PlagueDuration){
				Sprintf(buf2, "Terminally ill targets explode in a cloud of virulent toxins, damaging those around them.");
				OBJPUTSTR(buf2);
			}
			if(obj->oartifact == ART_WEBWEAVER_S_CROOK){
				Sprintf(buf2, "Traps struck targets in webs.");
				OBJPUTSTR(buf2);
			}
			if(obj->oartifact == ART_ATMA_WEAPON && !Drain_resistance){
				Sprintf(buf2, "Scales base damage by your current health percentage, currently %d%%.",
					Upolyd ? (u.mh * 100) / u.mhmax : (u.uhp * 100) / u.uhpmax);
				OBJPUTSTR(buf2);
			}
			if(fast_weapon(obj)){
				Sprintf(buf2, "Is 1/6th faster to swing than other weapons.");
				OBJPUTSTR(buf2);
			}
			if(pure_weapon(obj) && obj->spe >= 6){
				Sprintf(buf2, "Deals 20%% extra damage to all targets when the wielder is at full health.");
				OBJPUTSTR(buf2);
			}
			if(dark_weapon(obj) && obj->spe >= 6){
				Sprintf(buf2, "Deals 20%% extra damage to all targets when the wielder is at 30%% health or lower.");
				OBJPUTSTR(buf2);
			}
			if(is_vibroweapon(obj)){
				Sprintf(buf2, "Drains one charge per hit and deals less damage when uncharged.");
				OBJPUTSTR(buf2);
			}
		}
		/* poison */
		if (obj) {
			int poisons = obj->opoisoned;
			//int artipoisons = 0;
			if (arti_poisoned(obj))
				poisons |= OPOISON_BASIC;
			if (oartifact == ART_WEBWEAVER_S_CROOK)
				poisons |= (OPOISON_SLEEP | OPOISON_BLIND | OPOISON_PARAL);
			if (oartifact == ART_SUNBEAM)
				poisons |= OPOISON_FILTH;
			if (oartifact == ART_MOONBEAM)
				poisons |= OPOISON_SLEEP;
			if (oartifact == ART_DIRGE)
				poisons |= OPOISON_ACID;
			if (check_oprop(obj, OPROP_RLYHW) && Insight && rnd(Insight) >= 44)
				poisons |= OPOISON_ACID;
			
			if (poisons) {
				/* special cases */
				if (poisons == OPOISON_BASIC) {
					OBJPUTSTR("Coated with poison.");
				}
				else if (poisons == OPOISON_ACID) {
					OBJPUTSTR("Coated in acid.");
				}
				else if (poisons == OPOISON_SILVER) {
					OBJPUTSTR("Coated with silver.");
				}
				else if (poisons == OPOISON_HALLU) {
					OBJPUTSTR("Coated with hallucinogens.");
				}
				/* general mash-em-together poison */
				else {
					buf[0] = '\0';
					if (poisons&OPOISON_BASIC)  {Strcat(buf, "harmful "  );}
					if (poisons&OPOISON_FILTH)  {Strcat(buf, "sickening ");}
					if (poisons&OPOISON_SLEEP)  {Strcat(buf, "sleeping " );}
					if (poisons&OPOISON_BLIND)  {Strcat(buf, "blinding " );}
					if (poisons&OPOISON_PARAL)  {Strcat(buf, "paralytic ");}
					if (poisons&OPOISON_AMNES)  {Strcat(buf, "amnesiac " );}
					if (poisons&OPOISON_ACID)   {Strcat(buf, "acidic "   );}
					if (poisons&OPOISON_SILVER) {Strcat(buf, "silver "   );}
					if (poisons&OPOISON_HALLU) {Strcat(buf, "hallucinogenic "   );}
					
					Sprintf(buf2, "Coated with %spoison.", an(buf));
					OBJPUTSTR(buf2);
				}
			}
		}

		/* to-hit */

		int hitbon = oc.oc_hitbon;
		if (obj){
			int size_penalty = obj->objsize - youracedata->msize;
			if (Role_if(PM_CAVEMAN))
				size_penalty = max(0, size_penalty - 1);
			if (u.sealsActive&SEAL_YMIR)
				size_penalty = max(0, size_penalty - 1);
			if (check_oprop(obj, OPROP_CCLAW))
				size_penalty = max(0, size_penalty - 1);
			
			if(size_penalty < 0) size_penalty = 0;
			hitbon -= size_penalty * 4;
		} 
		
		if (hitbon != 0)
		{
			Sprintf(buf, "Has a %s %s to hit.",
				sitoa(hitbon),
				(hitbon >= 0 ? "bonus" : "penalty"));
			OBJPUTSTR(buf);
		}

		/* Damage bonus from attributes */
#define dbon_to_word(num) ((num == 0) ? "no " : ((num == 0.5) ? "half " : ((num == 1.5) ? "half again " : ((num == 2) ? "double " : "full "))))
#define atr_to_word(atr) ((atr == A_STR) ? "strength" : ((atr == A_DEX) ? "dexterity" : ((atr == A_CON) ? "constitution" \
						: ((atr == A_INT) ? "intelligence" : ((atr == A_WIS) ? "wisdom" : "charisma")))))
		if (obj){
			float atrdbon = 0;
			boolean has_before = FALSE;
			Sprintf(buf, "Gains ");
			for (int i = A_STR; i < A_MAX; i++){
				atrdbon = atr_dbon(obj, &youmonst, i);
				if ((atrdbon == 0 && i == A_STR) || atrdbon > 0){
					Sprintf(buf2, "%sbonus %sfrom %s, ", dbon_to_word(atrdbon), (has_before) ? "" : "damage ", atr_to_word(i));
					Strcat(buf, buf2);
					has_before = TRUE;
				}
			}
			Sprintf(buf2, "currently %d.", dbon(obj, &youmonst));
			Strcat(buf, buf2);
			OBJPUTSTR(buf);
		}
#undef dbon_to_word
#undef atr_to_word
	}
	/* object properties (objects only) */
	if(!check_oprop(obj, OPROP_NONE)){
		/* holy/unholy bonus damage */
		buf[0] = '\0';
		ADDCLASSPROP(check_oprop(obj, OPROP_HOLYW) && obj->blessed, "holy");
		ADDCLASSPROP(check_oprop(obj, OPROP_UNHYW) && obj->cursed, "unholy");
		if (buf[0] != '\0')
		{
			Sprintf(buf2, "Deals double %s damage.", buf);
			OBJPUTSTR(buf2);
		}
		buf[0] = '\0';
		ADDCLASSPROP(check_oprop(obj, OPROP_LESSER_HOLYW) && obj->blessed, "holy");
		ADDCLASSPROP(check_oprop(obj, OPROP_LESSER_UNHYW) && obj->cursed, "unholy");
		if (buf[0] != '\0')
		{
			Sprintf(buf2, "Deals 2d6 bonus %s damage.", buf);
			OBJPUTSTR(buf2);
		}
		/* simple damage properties */
		buf[0] = '\0';
		ADDCLASSPROP(check_oprop(obj, OPROP_FIREW), "fire");
		ADDCLASSPROP(check_oprop(obj, OPROP_COLDW), "cold");
		ADDCLASSPROP(check_oprop(obj, OPROP_WATRW), "water");
		ADDCLASSPROP(check_oprop(obj, OPROP_ELECW), "lightning");
		ADDCLASSPROP(check_oprop(obj, OPROP_ACIDW) || goatweaponturn == AD_EACD, "acid");
		ADDCLASSPROP(goatweaponturn == AD_STDY, "study");
		ADDCLASSPROP(check_oprop(obj, OPROP_MAGCW), "magic");
		if (buf[0] != '\0')
		{
			Sprintf(buf2, "Deals double %s damage.", buf);
			OBJPUTSTR(buf2);
		}
		buf[0] = '\0';
		
		if (goatweaponturn == AD_DRST)
		{
			Sprintf(buf2, "Deals double poison damage plus 4d4 physical.");
			OBJPUTSTR(buf2);
		}
		if(sothweaponturn == AD_VAMP)
		{
			Sprintf(buf2, "Drinks target's blood.");
			OBJPUTSTR(buf2);
		}
		if(sothweaponturn == AD_FIRE)
		{
			Sprintf(buf2, "Deals fire, physical, and fear damage.");
			OBJPUTSTR(buf2);
		}
		if(sothweaponturn == AD_DESC)
		{
			Sprintf(buf2, "Deals +50%% desiccation damage.");
			OBJPUTSTR(buf2);
		}
		if(sothweaponturn == AD_MAGM)
		{
			Sprintf(buf2, "Deals 4d4 + enchantment magic damage.");
			OBJPUTSTR(buf2);
		}
		if(sothweaponturn == AD_MADF)
		{
			Sprintf(buf2, "Exposes the target to the wielder's burning madness.");
			OBJPUTSTR(buf2);
		}
		if(sothweaponturn == AD_DRST)
		{
			Sprintf(buf2, "Deals 1d6 stench damage and may cause vomiting.");
			OBJPUTSTR(buf2);
		}
		if(sothweaponturn == AD_POLY)
		{
			Sprintf(buf2, "Polymorphs the target into earlier forms.");
			OBJPUTSTR(buf2);
		}
		if(sothweaponturn == AD_STTP)
		{
			Sprintf(buf2, "Teleports away target's armor or deals double damage.");
			OBJPUTSTR(buf2);
		}
		if (check_oprop(obj, OPROP_MORTW) && !FLAME_BAD)
		{
			Sprintf(buf2, "Drains 1d2 levels from living intelligent targets.");
			OBJPUTSTR(buf2);
		}

		if (check_oprop(obj, OPROP_TDTHW) && !FLAME_BAD)
		{
			Sprintf(buf2, "Deals double damage plus 2d7 to undead.");
			OBJPUTSTR(buf2);
		}

		if (check_oprop(obj, OPROP_SFUWW) && !FLAME_BAD)
		{
			Sprintf(buf2, "Deals double disintegration damage to spiritual beings.");
			OBJPUTSTR(buf2);
		}

		ADDCLASSPROP(check_oprop(obj, OPROP_PSIOW), "psionic");
		if (buf[0] != '\0')
		{
			Sprintf(buf2, "Deals double-plus-enchantment %s damage.", buf);
			OBJPUTSTR(buf2);
		}
		/* simple lesser damage properties */
		buf[0] = '\0';
		ADDCLASSPROP(check_oprop(obj, OPROP_LESSER_FIREW), "fire");
		ADDCLASSPROP(check_oprop(obj, OPROP_LESSER_COLDW), "cold");
		ADDCLASSPROP(check_oprop(obj, OPROP_LESSER_WATRW), "water");
		ADDCLASSPROP(check_oprop(obj, OPROP_LESSER_ELECW), "lightning");
		ADDCLASSPROP(check_oprop(obj, OPROP_LESSER_ACIDW), "acid");
		if (buf[0] != '\0')
		{
			Sprintf(buf2, "Deals 2d6 bonus %s damage.", buf);
			OBJPUTSTR(buf2);
		}
		buf[0] = '\0';
		
		ADDCLASSPROP(check_oprop(obj, OPROP_OONA_FIREW), "fire");
		ADDCLASSPROP(check_oprop(obj, OPROP_OONA_COLDW), "cold");
		ADDCLASSPROP(check_oprop(obj, OPROP_OONA_ELECW), "lightning");
		if (buf[0] != '\0')
		{
			Sprintf(buf2, "Deals 1d8 bonus %s damage.", buf);
			OBJPUTSTR(buf2);
		}
		buf[0] = '\0';
		
		ADDCLASSPROP(goatweaponturn == AD_COLD, "cold");
		ADDCLASSPROP(goatweaponturn == AD_ELEC, "lightning");
		if (buf[0] != '\0')
		{
			Sprintf(buf2, "Deals 3d8 bonus %s damage.", buf);
			OBJPUTSTR(buf2);
		}
		buf[0] = '\0';
		
		ADDCLASSPROP(goatweaponturn == AD_FIRE, "fire");
		if (buf[0] != '\0')
		{
			Sprintf(buf2, "Deals 3d10 bonus %s damage.", buf);
			OBJPUTSTR(buf2);
		}
		buf[0] = '\0';
		
		ADDCLASSPROP(goatweaponturn == AD_ACID, "acid");
		if (buf[0] != '\0')
		{
			Sprintf(buf2, "Deals 4d4 bonus %s damage.", buf);
			OBJPUTSTR(buf2);
		}
		buf[0] = '\0';
		
		ADDCLASSPROP(check_oprop(obj, OPROP_LESSER_MAGCW), "magic");
		if (buf[0] != '\0')
		{
			Sprintf(buf2, "Deals 3d4 bonus %s damage.", buf);
			OBJPUTSTR(buf2);
		}
		buf[0] = '\0';
		ADDCLASSPROP(check_oprop(obj, OPROP_LESSER_PSIOW), "psionic");
		if (buf[0] != '\0')
		{
			Sprintf(buf2, "Deals 2d12 bonus %s damage.", buf);
			OBJPUTSTR(buf2);
		}
		/* alignment damage properties */
		buf[0] = '\0';
		ADDCLASSPROP(check_oprop(obj, OPROP_ANARW), "lawful and neutral creatures");
		ADDCLASSPROP(check_oprop(obj, OPROP_CONCW), "lawful and chaotic creatures");
		ADDCLASSPROP(check_oprop(obj, OPROP_AXIOW), "neutral and chaotic creatures");
		if (buf[0] != '\0')
		{
			Sprintf(buf2, "Deals double damage to %s.", buf);
			OBJPUTSTR(buf2);
		}
		buf[0] = '\0';
		ADDCLASSPROP(check_oprop(obj, OPROP_LESSER_ANARW), "lawful and neutral creatures");
		ADDCLASSPROP(check_oprop(obj, OPROP_LESSER_CONCW), "lawful and chaotic creatures");
		ADDCLASSPROP(check_oprop(obj, OPROP_LESSER_AXIOW), "neutral and chaotic creatures");
		if (buf[0] != '\0')
		{
			Sprintf(buf2, "Deals 2d6 bonus damage to %s.", buf);
			OBJPUTSTR(buf2);
		}
		buf[0] = '\0';
		ADDCLASSPROP((check_oprop(obj, OPROP_OONA_FIREW) || check_oprop(obj, OPROP_OONA_COLDW) || check_oprop(obj, OPROP_OONA_ELECW)), "neutral and chaotic creatures");
		if (buf[0] != '\0')
		{
			Sprintf(buf2, "Deals 1d8 bonus damage to %s.", buf);
			OBJPUTSTR(buf2);
		}
		if (check_oprop(obj, OPROP_OCLTW))
		{
			Sprintf(buf2, "Deals bonus magic damage and extra damage to divine minions.");
			OBJPUTSTR(buf2);
		}
		/* other stuff
		 */
		buf[0] = '\0';
		ADDCLASSPROP((check_oprop(obj, OPROP_DEEPW) && obj->spe < 8), "telepathically lashes out");
		ADDCLASSPROP((check_oprop(obj, OPROP_VORPW)), "is vorpal");
		ADDCLASSPROP((check_oprop(obj, OPROP_MORGW)), "inflicts unhealing wounds while cursed");
		ADDCLASSPROP((check_oprop(obj, OPROP_FLAYW)), "destroys armor");
		ADDCLASSPROP((check_oprop(obj, OPROP_RETRW)), "returns when thrown");
		if (buf[0] != '\0')
		{
			buf[0] = buf[0] + 'A' - 'a';
			Sprintf(buf2, "%s.", buf);
			OBJPUTSTR(buf2);
		}

		if (check_oprop(obj, OPROP_SFLMW) && !FLAME_BAD)
		{
			Sprintf(buf2, "Offers slain targets to the Silver Flame.");
			OBJPUTSTR(buf2);
		}

		if (check_oprop(obj, OPROP_GOATW) && !GOAT_BAD)
		{
			Sprintf(buf2, "Feeds slain foes to the Black Mother.");
			OBJPUTSTR(buf2);
		}

		if (check_oprop(obj, OPROP_SOTHW) && !YOG_BAD)
		{
			Sprintf(buf2, "Slakes the thirst of Yog-Sothoth.");
			OBJPUTSTR(buf2);
		}
		if (check_oprop(obj, OPROP_RWTH))
		{
			Sprintf(buf2, "Channels the wrath of the Silver Flame.");
			OBJPUTSTR(buf2);
		}
		if (check_oprop(obj, OPROP_RBRD))
		{
			Sprintf(buf2, "The Silver Flame shares burdens.");
			OBJPUTSTR(buf2);
		}
		if (check_oprop(obj, OPROP_SLIF))
		{
			Sprintf(buf2, "The Silver Flame will save the wearer's life.");
			OBJPUTSTR(buf2);
		}
		if(check_oprop(obj, OPROP_ANTAW)){
			Sprintf(buf2, "Conducts arcane forces.");
			OBJPUTSTR(buf2);
		}
	}
	if(obj && obj->expert_traits){
		buf[0] = '\0';
#define	EXPERTTRAITS(trait, string)	\
	ADDCLASSPROP(CHECK_ETRAIT(obj, &youmonst, trait), string);
		EXPERTTRAITS(ETRAIT_HEW, "can deliver powerful overhead blows");
		EXPERTTRAITS(ETRAIT_FELL, "can disrupt enemy movement");
		EXPERTTRAITS(ETRAIT_KNOCK_BACK, (obj->expert_traits&ETRAIT_KNOCK_BACK_CHARGE) ? "can charge and knock enemies back" : "can knock enemies back");
		EXPERTTRAITS(ETRAIT_FOCUS_FIRE, "can target gaps in enemy armor");
		EXPERTTRAITS(ETRAIT_STUNNING_STRIKE, "can deliver powerful stunning blows");
		EXPERTTRAITS(ETRAIT_GRAZE, "may graze foes on a near miss");
		EXPERTTRAITS(ETRAIT_STOP_THRUST, "can harness enemy momentum to deliver powerful blows");
		EXPERTTRAITS(ETRAIT_PENETRATE_ARMOR, "penetrates enemy armor");
		EXPERTTRAITS(ETRAIT_LONG_SLASH, "deals extra damage against lightly-armored enemies");
		EXPERTTRAITS(ETRAIT_BLEED, "may deliver bleeding wounds");
		EXPERTTRAITS(ETRAIT_CLEAVE, "cleaves through slain enemies");
		EXPERTTRAITS(ETRAIT_LUNGE, "can be used for lunging attacks");
		EXPERTTRAITS(ETRAIT_QUICK, "strikes quickly");
		EXPERTTRAITS(ETRAIT_SECOND, "when wielded in the off-hand strikes a second foe after killing the first");
		EXPERTTRAITS(ETRAIT_CREATE_OPENING, "creates openings for sneak attacks");
		EXPERTTRAITS(ETRAIT_BRACED, "delivers powerful counterattacks");
		EXPERTTRAITS(ETRAIT_BLADESONG, "delivers powerful blows when combined with songs or spells");
		EXPERTTRAITS(ETRAIT_BLADEDANCE, "delivers powerful blows when moving and striking erratically");
		if(buf[0] != '\0')
			Sprintf(buf2, "Expert traits: %s.", buf);
		else
			Sprintf(buf2, "No expert traits unlocked.");
		OBJPUTSTR(buf2);
	}
	else {
		Sprintf(buf2, "No expert traits.");
		OBJPUTSTR(buf2);
	}
	/* other artifact weapon effects */
	if (oartifact) {
		register const struct artifact *oart = &artilist[oartifact];
		buf[0] = '\0';
		ADDCLASSPROP((oart->aflags&ARTA_POIS), "always poisoned");
		if(oartifact != ART_AMALGAMATED_SKIES)
			ADDCLASSPROP((oart->aflags&ARTA_SILVER), "silvered");
		ADDCLASSPROP((oart->aflags&ARTA_VORPAL), "vorpal");
		ADDCLASSPROP((oart->aflags&ARTA_CANCEL), "canceling");
		ADDCLASSPROP((oart->aflags&ARTA_MAGIC), "magic-flourishing");
		ADDCLASSPROP((oart->aflags&ARTA_DRAIN), "draining");
		ADDCLASSPROP((oart->aflags&ARTA_BRIGHT), "illuminating");
		ADDCLASSPROP((oart->aflags&ARTA_BLIND), "blinding");
		ADDCLASSPROP((oart->aflags&ARTA_PHASING), "armor-phasing");
		ADDCLASSPROP((oart->aflags&ARTA_SHATTER), "shattering");
		ADDCLASSPROP((oart->aflags&ARTA_DISARM), "disarming");
		ADDCLASSPROP((oart->aflags&ARTA_STEAL), "thieving");
		ADDCLASSPROP((oart->aflags&(ARTA_KNOCKBACK|ARTA_KNOCKBACKX)), "kinetic");
		if (buf[0] != '\0')
		{
			Sprintf(buf2, "Attacks are %s.", buf);
			OBJPUTSTR(buf2);
		}

		buf[0] = '\0';
		ADDCLASSPROP((oart->aflags&ARTA_DLUCK), "luck-biased");
		ADDCLASSPROP((oart->aflags&ARTA_DEXPL), "exploding");
		if (buf[0] != '\0')
		{
			Sprintf(buf2, "Attacks use %s dice.", buf);
			OBJPUTSTR(buf2);
		}

		buf[0] = '\0';
		ADDCLASSPROP((oart->aflags&ARTA_EXPLFIRE), "fiery");
		ADDCLASSPROP((oart->aflags&ARTA_EXPLCOLD), "freezing");
		ADDCLASSPROP((oart->aflags&ARTA_EXPLELEC), "electrical");
		if (buf[0] != '\0')
		{
			Sprintf(buf2, "Attacks may cause %s explosions.", buf);
			OBJPUTSTR(buf2);
		}

		buf[0] = '\0';
		ADDCLASSPROP((oart->aflags&ARTA_EXPLFIREX), "fiery");
		ADDCLASSPROP((oart->aflags&ARTA_EXPLCOLDX), "freezing");
		ADDCLASSPROP((oart->aflags&ARTA_EXPLELECX), "electrical");
		if (buf[0] != '\0')
		{
			Sprintf(buf2, "Attacks cause %s explosions.", buf);
			OBJPUTSTR(buf2);
		}

		buf[0] = '\0';
		ADDCLASSPROP((oart->aflags&ARTA_RETURNING), "returns when thrown");
		ADDCLASSPROP((oart->aflags&ARTA_HASTE), "hastens the wielder's attacks");
		if (buf[0] != '\0')
		{
			buf[0] = buf[0] + 'A' - 'a';
			Sprintf(buf2, "%s.", buf);
			OBJPUTSTR(buf2);
		}
		if(oartifact == ART_AMALGAMATED_SKIES || oartifact == ART_SKY_REFLECTED){
			buf[0] = '\0';
#define	ZERTHPROPS(prop, string)	\
	ADDCLASSPROP(artinstance[ART_SKY_REFLECTED].ZerthUpgrades&prop, string);
			ZERTHPROPS(ZPROP_WRATH, "missiles of wrath");
			ZERTHPROPS(ZPROP_STEEL, "scripture of steel");
			ZERTHPROPS(ZPROP_WILL, "submerge the will");
			ZERTHPROPS(ZPROP_VILQUAR, "Vilquar's eye");
			ZERTHPROPS(ZPROP_POWER, "power of one");
			ZERTHPROPS(ZPROP_BALANCE, "balance in all things");
			ZERTHPROPS(ZPROP_PATIENCE, "work of patience");
			ZERTHPROPS(ZPROP_FOCUS, "Zerthimon's focus");
			if(buf[0] != '\0')
				Sprintf(buf2, "Known mantras: %s.", buf);
			else
				Sprintf(buf2, "No mantras known.");
			OBJPUTSTR(buf2);
			buf[0] = '\0';
			if(oartifact == ART_AMALGAMATED_SKIES)
				ADDCLASSPROP(TRUE, "silver");
			ADDCLASSPROP(TRUE, "iron");
#define	ZERTHMATS(prop, string)	\
	ADDCLASSPROP(artinstance[ART_SKY_REFLECTED].ZerthMaterials&prop, string);
			ZERTHMATS(ZMAT_GREEN, "green steel");
			ZERTHMATS(ZMAT_GOLD, "gold");
			ZERTHMATS(ZMAT_PLATINUM, "platinum");
			ZERTHMATS(ZMAT_MITHRIL, "mithril");
			Sprintf(buf2, "Amalgamated metals: %s.", buf);
			OBJPUTSTR(buf2);
		}
	}
	if (olet == ARMOR_CLASS || olet == BELT_CLASS) {
		/* Armor type */
		/* Indexes here correspond to ARM_SHIELD, etc; not the W_* masks.
		* Expects ARM_SUIT = 0, all the way up to ARM_SHIRT = 6. */
		if (!printed_type) {
			const char* armorslots[] = {
				"torso", "shield", "helm", "gloves", "boots", "cloak", "shirt", "belt"
			};
			if (obj) {
				Sprintf(buf, "%s, worn in the %s slot.",
					(oc.oc_armcat != ARM_SUIT ? "Armor" :
					is_light_armor(obj) ? "Light armor" :
					is_medium_armor(obj) ? "Medium armor" :
					"Heavy armor"),
					armorslots[oc.oc_armcat]);
			}
			else {
				/* currently, the is_x_armor checks wouldn't actually need an obj,
				* but there's no sense in rewriting perfectly good code to fit
				* immediate needs and possibly prevent future changes that would
				* like details from the obj itself 
				*/
				Sprintf(buf, "Armor, worn in the %s slot.",
					armorslots[oc.oc_armcat]);
			}
			OBJPUTSTR(buf);
			printed_type = TRUE;
		}
		/* Defense */
		if (obj && obj->known) {// calculate the actual AC and DR this armor gives
			if(is_shield(obj) && obj->objsize != youracedata->msize){
				Sprintf(buf, "Is worth %d AC (%d to you, due to its size) and %d DR.",
					arm_ac_bonus(obj), max(0, arm_ac_bonus(obj) + (obj->objsize - youracedata->msize)), arm_dr_bonus(obj));
			} else {
				Sprintf(buf, "Is worth %d AC and %d DR.",
					arm_ac_bonus(obj), arm_dr_bonus(obj));
			}
		}
		else {// say what the base stats are
			Sprintf(buf, "Base %d AC and %d DR.",
				oc.a_ac, oc.a_dr);
		}
		OBJPUTSTR(buf);
		/* Magic cancellation */
		if (oc.a_can > 0)
		{
			Sprintf(buf, "Grants magic cancellation, level %d.", oc.a_can);
			OBJPUTSTR(buf);
		}
		/* Don/Doff time */
		Sprintf(buf, "Takes %d turn%s to put on or remove.",
			max(1, oc.oc_delay), (max(1, oc.oc_delay) == 1 ? "" : "s"));
		OBJPUTSTR(buf);

		/* Enchantment limit */
		if (obj && is_plussev_armor(obj))
			OBJPUTSTR("Holds enchantments well.");
	}
	/* Enchantment limit */
	if (obj && is_plusten(obj))
		OBJPUTSTR("Holds enchantments extremely well.");
	if (olet == FOOD_CLASS) {
		if (otyp == TIN) {
			if (obj && obj->known)
			{
				if (obj->spe > 0)
				{ // spinach
					OBJPUTSTR("Comestible providing 600 nutrition.");
					OBJPUTSTR("Takes various amounts of turns to open and eat.");
					OBJPUTSTR("Is vegan.");
				}
				else if (obj->corpsenm == NON_PM)
				{ // empty
					OBJPUTSTR("Comestible providing no nutrition.");
					OBJPUTSTR("Takes various amounts of turns to open.");
				}
				else
				{ // contains a monster
					OBJPUTSTR("Comestible providing varied nutrition.");
					OBJPUTSTR("Takes various amounts of turns to eat.");
					if (vegan(&mons[obj->corpsenm]))
						OBJPUTSTR("Is vegan.");
					else if (vegetarian(&mons[obj->corpsenm]))
						OBJPUTSTR("Is vegetarian but not vegan.");
					else
						OBJPUTSTR("Is not vegetarian.");
				}
			}
			else
			{
				OBJPUTSTR("Comestible providing varied nutrition.");
				OBJPUTSTR("Takes various amounts of turns to eat.");
				OBJPUTSTR("May or may not be vegetarian.");
			}
		}
		else if (otyp == CORPSE) {
			Sprintf(buf, "Comestible providing %d nutrition%s.", mons[obj->corpsenm].cnutrit, (obj && obj->oeaten ? " when eaten whole" : ""));
			OBJPUTSTR(buf);
			OBJPUTSTR("Takes various amounts of turns to eat.");
			if (obj)
			{
				if (vegan(&mons[obj->corpsenm]))
					OBJPUTSTR("Is vegan.");
				else if (vegetarian(&mons[obj->corpsenm]))
					OBJPUTSTR("Is vegetarian but not vegan.");
				else
					OBJPUTSTR("Is not vegetarian.");
			}
			else {
				OBJPUTSTR("May or may not be vegetarian.");
			}
		}
		else {
			Sprintf(buf, "Comestible providing %d nutrition%s.", oc.oc_nutrition, (obj && obj->oeaten ? " when eaten whole" : ""));
			OBJPUTSTR(buf);
			Sprintf(buf, "Takes %d turn%s to eat.", oc.oc_delay,
				(oc.oc_delay == 1 ? "" : "s"));
			OBJPUTSTR(buf);
			/* TODO: put special-case VEGGY foods in a list which can be
			* referenced by doeat(), so there's no second source for this. */
			if (oc.oc_material == FLESH && otyp != EGG) {
				OBJPUTSTR("Is not vegetarian.");
			}
			else {
				/* is either VEGGY food or egg */
				switch (otyp) {
				case PANCAKE:
				case FORTUNE_COOKIE:
				case EGG:
				case CREAM_PIE:
				case CANDY_BAR:
				case HONEYCOMB:
				case LUMP_OF_ROYAL_JELLY:
				case LUMP_OF_SOLDIER_S_JELLY:
				case LUMP_OF_DANCER_S_JELLY:
				case LUMP_OF_PHILOSOPHER_S_JELLY:
				case LUMP_OF_PRIESTESS_S_JELLY:
				case LUMP_OF_RHETOR_S_JELLY:
					OBJPUTSTR("Is vegetarian but not vegan.");
					break;
				default:
					OBJPUTSTR("Is vegan.");
				}
			}
		}
	}
	if (olet == POTION_CLASS) {
		/* nothing special */;
	}
	if (olet == SCROLL_CLASS) {
		/* nothing special */;
	}
	if (olet == TILE_CLASS) {
		switch (otyp) {
		case SYLLABLE_OF_STRENGTH__AESH:
			OBJPUTSTR("Read to gain temporary and permanent bonuses.");
			OBJPUTSTR("Temporarily increases physical attack damage by 10 points.");
			OBJPUTSTR("Permanently increases physical attack damage by 1/3rd point.");
		break;
		case SYLLABLE_OF_POWER__KRAU:
			OBJPUTSTR("Read to gain temporary and permanent bonuses.");
			OBJPUTSTR("Temporarily empower damaging magic to 150\% of normal strength.");
			OBJPUTSTR("Permanently increases spell damage bonus by 1/3rd point.");
		break;
		case SYLLABLE_OF_LIFE__HOON:
			OBJPUTSTR("Read to gain temporary and permanent bonuses.");
			OBJPUTSTR("Temporarily regenerate an additional 10 HP per turn.");
			OBJPUTSTR("Permanently increase natural regeneration by 1 point per 90 turns.");
		break;
		case SYLLABLE_OF_GRACE__UUR:
			OBJPUTSTR("Read to gain temporary and permanent bonuses.");
			OBJPUTSTR("Temporarily gain +10 AC, +10 to-hit, and +6 movement per turn.");
			OBJPUTSTR("Permanently increase AC by 1/2 point.");
		break;
		case SYLLABLE_OF_THOUGHT__NAEN:
			OBJPUTSTR("Read to gain temporary and permanent bonuses.");
			OBJPUTSTR("Temporarily regenerate an additional 10 power per turn and 0\% spell failure.");
			OBJPUTSTR("Permanently increase natural power regeneration by 1 point per 90 turns.");
		break;
		case SYLLABLE_OF_SPIRIT__VAUL:
			OBJPUTSTR("Read to gain temporary and permanent bonuses.");
			OBJPUTSTR("Temporarily gain displacement and take half damage from attacks.");
			OBJPUTSTR("Permanently increase DR by 1/5th point.");
		break;
		case FIRST_WORD:
			OBJPUTSTR("Read to permanently learn the First Word of Creation.");
			OBJPUTSTR("Adds a new ability to the #monster powers menu.");
			OBJPUTSTR("Permanently grants intrinsic flying.");
		break;
		case DIVIDING_WORD:
			OBJPUTSTR("Read to permanently learn the Dividing Word of Creation.");
			OBJPUTSTR("Adds a new ability to the #monster powers menu.");
			OBJPUTSTR("Permanently grants intrinsic water resistance.");
		break;
		case NURTURING_WORD:
			OBJPUTSTR("Read to permanently learn the Nurturing Word of Creation.");
			OBJPUTSTR("Adds a new ability to the #monster powers menu.");
			OBJPUTSTR("Permanently grants regeneration to all pets.");
		break;
		case WORD_OF_KNOWLEDGE:
			OBJPUTSTR("Read to permanently learn the Word of Knowledge.");
			OBJPUTSTR("Adds a new ability to the #monster powers menu.");
			OBJPUTSTR("Permanently grants plus three to pet cap.");
		break;
		case APHANACTONAN_RECORD:
			OBJPUTSTR("Read to identify some random item types.");
		break;
		case APHANACTONAN_ARCHIVE:
			OBJPUTSTR("Read to identify some random item types and other knowledge.");
		break;
		default:
			OBJPUTSTR("Read to acquire the thought symbolized by this glyph.");
		break;
		}
	}
	if (olet == SPBOOK_CLASS) {
		if (!printed_type) {
			switch(otyp) {
			case SPE_BLANK_PAPER:
				OBJPUTSTR("Empty book.");
				break;
			case SPE_BOOK_OF_THE_DEAD:
				OBJPUTSTR("Ancient tome.");
				break;
			case SPE_SECRETS:
				OBJPUTSTR("Ancient tome.");
				break;
			default:
				Sprintf(buf, "Level %d spellbook, in the %s school. %s spell.",
					oc.oc_level, spelltypemnemonic(oc.oc_skill), dir);
				OBJPUTSTR(buf);
				break;
			}
			printed_type = TRUE;
		}
		switch (oartifact)
		{
		case ART_NECRONOMICON:
			OBJPUTSTR("What dark secrets still lurk in its pages?");
			break;
		case ART_BOOK_OF_LOST_NAMES:
			OBJPUTSTR("Spirits from the void call out from its pages.");
			break;
		case ART_BOOK_OF_INFINITE_SPELLS:
			OBJPUTSTR("Arcane magics fill its endless pages.");
			break;
		}
	}
	if (olet == WAND_CLASS) {
		if (!printed_type) {
			Sprintf(buf, "%s wand.", dir);
			OBJPUTSTR(buf);
			printed_type = TRUE;
		}
		if (obj) {
			int freechance = 100 - zapcostchance(obj, &youmonst);
			if (freechance > 0) {
				Sprintf(buf, "You have a %d%% chance not to use a charge each zap.", freechance);
				OBJPUTSTR(buf);
			}
		}
		else {
			struct obj * fakeobj = mksobj(otyp, MKOBJ_NOINIT);
			fakeobj->oartifact = oartifact;

			int freechance = 100 - zapcostchance(fakeobj, (struct monst *)0);
			if (freechance > 0) {
				Sprintf(buf, "%d%% chance not to use a charge each zap.", freechance);
				OBJPUTSTR(buf);
			}
			delobj(fakeobj);
		}
	}
	if (olet == RING_CLASS) {
		if (oc.oc_charged && otyp != RIN_WISHES)
			OBJPUTSTR("Chargeable.");
		if (oc.oc_tough) {
			OBJPUTSTR("Is made of a hard material.");
		}
	}
	if (olet == GEM_CLASS) {
		if (otyp == ANTIMAGIC_RIFT || otyp == CATAPSI_VORTEX) {
			Sprintf(buf, "Apply to crush the gem and unleash the contained %s.", otyp == ANTIMAGIC_RIFT ? "rift" : "vortex");
			OBJPUTSTR(buf);
		}
		else if (otyp == VITAL_SOULSTONE || otyp == SPIRITUAL_SOULSTONE) {
			Sprintf(buf, "Passively increases %s regeneration while carried.", otyp == VITAL_SOULSTONE ? "health" : "magic");
			OBJPUTSTR(buf);
			OBJPUTSTR("Apply to crush the stone and expend the trapped soul.");
		}
		else if (oc.oc_material == MINERAL && !printed_type) {
			OBJPUTSTR("Type of stone.");
			printed_type = TRUE;
		}
		else if (oc.oc_material == GLASS && !printed_type) {
			OBJPUTSTR("Piece of colored glass.");
			printed_type = TRUE;
		}
		else if (!printed_type) {
			OBJPUTSTR("Precious gem.");
			printed_type = TRUE;
		}
	}
	if (olet == SCOIN_CLASS) {
		switch(otyp) {
		case WAGE_OF_PRIDE:
			OBJPUTSTR("Apply to crush the coin and harness the trapped soul's weighty ego.");
			OBJPUTSTR("Weakens a thinking creature, and causes an Angel to fall from grace.");
			break;
		case WAGE_OF_ENVY:
			OBJPUTSTR("Apply to crush the coin and harness the trapped soul's material jealousy.");
			OBJPUTSTR("Makes a thinking creature discard all its things in desire of others'.");
			break;
		case WAGE_OF_LUST:
			OBJPUTSTR("Apply to crush the coin and harness the trapped soul's aimless bluster.");
			OBJPUTSTR("Creates a hurricane of wind around you.");
			break;
		case WAGE_OF_WRATH:
			OBJPUTSTR("Apply to crush the coin and harness the trapped soul's spilled blood.");
			OBJPUTSTR("Fills the lungs of a creature with enraging blood.");
			break;
		case WAGE_OF_GLUTTONY:
			OBJPUTSTR("Apply to crush the coin and harness the trapped soul's endless hunger.");
			OBJPUTSTR("Causes an ediate creature to starve.");
			break;
		case WAGE_OF_GREED:
			OBJPUTSTR("Apply to crush the coin and harness the trapped soul's hoarded things.");
			OBJPUTSTR("Creates a mass of random objects to throw at a location.");
			break;
		case WAGE_OF_SLOTH:
			OBJPUTSTR("Apply to crush the coin and harness the trapped soul's wasted time.");
			OBJPUTSTR("Gives you a brief period of accelerated time.");
			break;
		}
	}
	if (olet == TOOL_CLASS && !printed_type) {
		const char* subclass = "tool";
		switch (otyp) {
		case BLINDFOLD:
		case TOWEL:
		case R_LYEHIAN_FACEPLATE:
		case MASK:
		case LENSES:
		case SUNGLASSES:
		case SOUL_LENS:
		case LIVING_MASK:
			subclass = "facial accessory";
			break;
		case BOX:
		case SARCOPHAGUS:
		case CHEST:
		case MAGIC_CHEST:
		case MASSIVE_STONE_CRATE:
		case ICE_BOX:
		case SACK:
		case OILSKIN_SACK:
		case BAG_OF_HOLDING:
			subclass = "container";
			break;
		case SKELETON_KEY:
		case UNIVERSAL_KEY:
		case LOCK_PICK:
		case CREDIT_CARD:
			subclass = "unlocking tool";
			break;
		case TALLOW_CANDLE:
		case WAX_CANDLE:
		case CANDLE_OF_INVOCATION:
		case TORCH: //actually over-ridden by being a weapon-tool
		case SUNROD: //actually over-ridden by being a weapon-tool
		case MAGIC_TORCH: //actually over-ridden by being a weapon-tool
		case LANTERN:
		case OIL_LAMP:
		case MAGIC_LAMP:
		case CANDELABRUM_OF_INVOCATION:
			subclass = "light source";
			break;
		case SHADOWLANDER_S_TORCH:	//actually over-ridden by being a weapon-tool
			subclass = "dark source";
			break;
		case LAND_MINE:
		case BEARTRAP:
			subclass = "trap which can be set";
			break;
		case WHISTLE:
		case MAGIC_WHISTLE:
		case BELL:
		case BELL_OF_OPENING:
		case DRUM:
		case DRUM_OF_EARTHQUAKE:
			subclass = "atonal instrument";
			break;
		case BUGLE:
		case MAGIC_FLUTE:
		case FLUTE:
		case TOOLED_HORN:
		case FIRE_HORN:
		case FROST_HORN:
		case HARP:
		case MAGIC_HARP:
			subclass = "tonal instrument";
			break;
		case HYPOSPRAY:
		case SENSOR_PACK:
		case POWER_PACK:
		case BULLET_FABBER:
			subclass = "future-tech tool";
			break;
		case DIMENSIONAL_LOCK:
			subclass = "";
			OBJPUTSTR("Can be applied to temporarily prevent summoning.");
			break;
		}
		if (subclass[0] != '\0') {
			Sprintf(buf, "%s%s.", (oc.oc_charged ? "chargeable " : ""), subclass);
			/* capitalize first letter of buf */
			buf[0] -= ('a' - 'A');
			OBJPUTSTR(buf);
		}
	}

	/* cost, wt should go next */
	if (obj) {
		Sprintf(buf, "Costs %d zorkmid%s. Weighs %d aum.%s",
			(int)getprice(obj, FALSE, FALSE),
			(int)getprice(obj, FALSE, FALSE) != 1 ? "s" : "",
			(int)(weight(obj) / max(1, obj->quan)),
			((obj->quan != 1) ? " (per item)" : ""));
	}
	else {
		Sprintf(buf, "Base cost %d. Base weight %d aum.",
			(int)((oartifact) ? artilist[oartifact].cost : oc.oc_cost),
			(int)(oc.oc_weight));
	}
	OBJPUTSTR(buf);

	/* powers conferred */
	extern const struct propname {
		int prop_num;
		const char* prop_name;
	} propertynames[]; /* located in timeout.c */
	int i;

	int properties_item[LAST_PROP];
	get_item_property_list(properties_item, obj, otyp);
	int properties_art[LAST_PROP];
	get_art_property_list(properties_art, oartifact, FALSE);
	int properties_art_carried[LAST_PROP];
	get_art_property_list(properties_art_carried, oartifact, TRUE);

	for (i = 0; propertynames[i].prop_name; i++) {
		boolean got_prop = FALSE, while_carried = FALSE;
		int j = 0;

		while(oc.oc_oprop[j] && !got_prop) {
			if (oc.oc_oprop[j] == propertynames[i].prop_num)
				got_prop = TRUE;
			j++;
		}

		j = 0;
		while (properties_item[j] && !got_prop) {
			if (properties_item[j] == propertynames[i].prop_num)
				got_prop = TRUE;
			j++;
		}
		j = 0;
		if (oartifact)
		{
			while (properties_art[j] && !got_prop) {
				if (properties_art[j] == propertynames[i].prop_num)
					got_prop = TRUE;
				j++;
			}
		}
		j = 0;
		if (oartifact)
		{
			while (properties_art_carried[j] && !while_carried) {
				if (properties_art_carried[j] == propertynames[i].prop_num)
				{
					got_prop = TRUE;
					while_carried = TRUE;
				}
				j++;
			}
		}

		if (got_prop) {
			/* proper grammar */
			const char* confers = "Makes you";
			const char* effect = propertynames[i].prop_name;
			switch (propertynames[i].prop_num) {
				/* special overrides because prop_name is bad */
			case STRANGLED:
				effect = "choke";
				break;
			case FLYING:
				effect = "fly";
				break;
			case NULLMAGIC:
				effect = "apart from magic";
				break;
			case WATERPROOF:
				effect = "waterproof";
				break;
			case LIFESAVED:
				effect = "life saving";
				/* FALLTHRU */
				/* for things that don't work with "Makes you" */
			case GLIB:
			case WOUNDED_LEGS:
			case DETECT_MONSTERS:
			case SEE_INVIS:
			case HUNGER:
			case WARNING:
			case WARN_OF_MON:
			case SEARCHING:
			case INFRAVISION:
			case AGGRAVATE_MONSTER:
			case CONFLICT:
			case JUMPING:
			case TELEPORT_CONTROL:
			case SWIMMING:
			case SLOW_DIGESTION:
			case HALF_SPDAM:
			case HALF_PHDAM:
			case REGENERATION:
			case ENERGY_REGENERATION:
			case PROTECTION:
			case PROT_FROM_SHAPE_CHANGERS:
			case POLYMORPH_CONTROL:
			case FREE_ACTION:
			case FIXED_ABIL:
			case CLEAR_THOUGHTS:
			case MAGICAL_BREATHING:
				confers = "Confers";
				break;
			default:
				break;
			}
			if (strstri(propertynames[i].prop_name, "resistance"))
				confers = "Confers";
			Sprintf(buf, "%s %s%s.", confers, effect, (while_carried ? " while carried" : ""));
			OBJPUTSTR(buf);
		}
	}
	/* Other magical properties while worn that aren't covered by prop.h (or rather, propertynames[]) -- artifacts may cause more than one to apply, so don't if-else chain */
	if (otyp == HELM_OF_OPPOSITE_ALIGNMENT)	OBJPUTSTR("Forces an alignment change.");
	if (otyp == AMULET_OF_CHANGE)				OBJPUTSTR("Forces a change of gender.");
	if (otyp == DUNCE_CAP)						OBJPUTSTR("Greatly reduces INT.");
	if (otyp == GAUNTLETS_OF_POWER ||
		oartifact == ART_SCEPTRE_OF_MIGHT ||
		oartifact == ART_STORMBRINGER ||
		oartifact == ART_GOLDEN_KNIGHT ||
		oartifact == ART_OGRESMASHER)			OBJPUTSTR("Greatly increases STR.");
	if (oartifact == ART_STORMBRINGER ||
		oartifact == ART_GREAT_CLAWS_OF_URDLEN ||
		oartifact == ART_OGRESMASHER)			OBJPUTSTR("Greatly increases CON.");
	if (oartifact == ART_GODHANDS)		OBJPUTSTR("Greatly increases DEX.");
	if (oartifact == ART_PREMIUM_HEART)		OBJPUTSTR("Increases DEX.");
	if (otyp == KICKING_BOOTS)					OBJPUTSTR("Improves kicking.");
	if (otyp == MUMMY_WRAPPING || otyp == PRAYER_WARDED_WRAPPING)	OBJPUTSTR("Prevents invisibility.");
	if (otyp == RIN_GAIN_STRENGTH)				OBJPUTSTR("Increases STR by its enchantment.");
	if (otyp == GAUNTLETS_OF_DEXTERITY)		OBJPUTSTR("Increases DEX by its enchantment.");
	if (otyp == RIN_GAIN_CONSTITUTION)			OBJPUTSTR("Increases CON by its enchantment.");
	if (otyp == HELM_OF_BRILLIANCE)			OBJPUTSTR("Increases INT and WIS by its enchantment.");
	if (otyp == RIN_INCREASE_DAMAGE)			OBJPUTSTR("Increases your weapon damage.");
	if (otyp == RIN_INCREASE_ACCURACY)			OBJPUTSTR("Increases your to-hit modifier.");
	if (otyp == AMULET_VERSUS_CURSES ||
		otyp == PRAYER_WARDED_WRAPPING ||
		check_oprop(obj, OPROP_BCRS) ||
		check_oprop(obj, OPROP_CGLZ) ||
		oartifact == ART_HELPING_HAND ||
		oartifact == ART_STAFF_OF_NECROMANCY ||
		oartifact == ART_TREASURY_OF_PROTEUS ||
		oartifact == ART_TENTACLE_ROD ||
		oartifact == ART_WRAPPINGS_OF_THE_SACRED_FI ||
		oartifact == ART_SPELL_WARDED_WRAPPINGS_OF_ ||
		oartifact == ART_MAGICBANE)			OBJPUTSTR("Protects your inventory from being cursed.");

	/* Effects based on the base description of the item -- only one will apply, so an if-else chain is appropriate */
	/* randomized appearance items */
	if      (otyp == find_gcirclet())			OBJPUTSTR("Increases CHA by its enchantment.");
//	else if (otyp ==  find_ogloves())			OBJPUTSTR("Erosion resistant.");				// We don't comment on erosion-proofing elsewhere, either
	else if (otyp ==  find_tgloves())			OBJPUTSTR("Slightly increases unarmed damage.");
//	else if (otyp ==  find_pgloves())			OBJPUTSTR("Slightly increased defense.");		// Increased defensive stats are shown in the AC/DR section
	else if (otyp ==  find_fgloves())			OBJPUTSTR("Increases to-hit when fighting with a free off-hand.");
	else if (otyp ==   find_skates())			OBJPUTSTR("Prevents slipping on ice.");
	else if (otyp ==   find_cboots())			OBJPUTSTR("Slightly increases to-hit.");
	else if (otyp ==   find_mboots())			OBJPUTSTR("Protects against drowning attacks.");
	else if (otyp ==   find_hboots())			OBJPUTSTR("Increases carrying capacity.");
	else if (otyp ==   find_bboots())			OBJPUTSTR("Cannot be pulled straight off of your legs.");
	else if (otyp ==   find_jboots())			OBJPUTSTR("Reduces the severity of leg wounds.");
	else if (otyp ==    find_vhelm())			OBJPUTSTR("Protects from blinding claws and venom.");
	else if (otyp == find_engagement_ring())	OBJPUTSTR("Protects from unwanted advances.");
	else if (isSignetRing(otyp))				OBJPUTSTR("Can be poisoned.");
	/* non-randomized appearance items */
	else if (otyp == HARMONIUM_BOOTS ||
		otyp == HARMONIUM_SCALE_MAIL ||
		otyp == HARMONIUM_PLATE ||
		otyp == HARMONIUM_GAUNTLETS ||
		otyp == HARMONIUM_HELM)					OBJPUTSTR("Slightly increased defense against non-lawfuls.");
	else if (otyp == PLASTEEL_HELM ||
		otyp == CRYSTAL_HELM ||
		otyp == PONTIFF_S_CROWN ||
		otyp == FACELESS_HELM ||
		otyp == FACELESS_HOOD ||
		otyp == IMPERIAL_ELVEN_HELM ||
		otyp == WHITE_FACELESS_ROBE ||
		otyp == BLACK_FACELESS_ROBE ||
		otyp == SMOKY_VIOLET_FACELESS_ROBE)		OBJPUTSTR("Covers the face entirely.");

	buf[0] = '\0';
	ADDCLASSPROP(oartifact, "an artifact");
	ADDCLASSPROP((oc.oc_magic && !oartifact), "inherently magical");		// overkill to say an artifact is inherently magical, and makes it weird when one isn't
	ADDCLASSPROP((oc.oc_nowish || (artilist[oartifact].gflags&ARTG_NOWISH)), "not wishable");
	if (*buf) {
		Sprintf(buf2, "Is %s.", buf);
		OBJPUTSTR(buf2);
	}

	/* Material.
	 * Note that we should not show the material of certain objects if they are
	 * subject to description shuffling that includes materials. If the player
	 * has already discovered this object, though, then it's fine to show the
	 * material.
	 * Edit by Nero: dnh is assuming that oc_name_known == TRUE if this function is called. 
	 *
	 * Finally, this requires an object. Dnethack does some funny things with a few items
	 * to show adjectives, like the default material of sabers being metal, so showing
	 * what material items are "normally" made of could be misleading.
	 */
	if (obj) {
		Strcpy(buf2, material_name(obj, FALSE));

		Sprintf(buf, "Made of %s.", buf2);
		OBJPUTSTR(buf);
	}
	/* Approximate size */
	if (obj) {
		static const char * sizestrings[] = {
			"tiny",				/* index 0; size 0.0 */
			"very small",		/* index 1; size 0.5 */
			"small",			/* index 2; size 1.0 */
			"somewhat small",	/* index 3; size 1.5 */
			"medium-sized",		/* index 4; size 2.0 */
			"somewhat large",	/* index 5; size 2.5 */
			"large",			/* index 6; size 3.0 */
			"very large",		/* index 7; size 3.5 */
			"huge",				/* index 8; size 4.0 */
			"gigantic"			/* index 9; size 7.0 */
		};
		int sz = max(0, obj->objsize * 2 + (oc.oc_size - 2));
		if (sz >= 14)
			sz = 9;
		else if (sz > 8)
			sz = 8;
		Sprintf(buf, "Is a %s item.", sizestrings[sz]);
		OBJPUTSTR(buf);
	}
	if (obj && is_evaporable(obj)) {
		OBJPUTSTR("Evaporates in light.");
	}

	/* Full-line remarks */
	if (oc.oc_merge && !oartifact) {	// even when an artifact can merge with identical items (ie Sunbeam), it sounds really weird
		OBJPUTSTR("Merges with identical items.");
	}
	if (oc.oc_unique) {
		OBJPUTSTR("Unique item.");
	}
}

/*
 * find_unpaid()
 *
 * Scan the given list of objects.  If last_found is NULL, return the first
 * unpaid object found.  If last_found is not NULL, then skip over unpaid
 * objects until last_found is reached, then set last_found to NULL so the
 * next unpaid object is returned.  This routine recursively follows
 * containers.
 */
STATIC_OVL struct obj *
find_unpaid(list, last_found)
    struct obj *list, **last_found;
{
    struct obj *obj;

    while (list) {
	if (list->unpaid) {
	    if (*last_found) {
		/* still looking for previous unpaid object */
		if (list == *last_found)
		    *last_found = (struct obj *) 0;
	    } else
		return (*last_found = list);
	}
	if (Has_contents(list)) {
	    if ((obj = find_unpaid(list->cobj, last_found)) != 0)
		return obj;
	}
	list = list->nobj;
    }
    return (struct obj *) 0;
}

#ifdef SORTLOOT
void
munge_objnames(obj, buffer)
	struct obj *obj;
	char *buffer;
{
  if (obj->otyp == POT_WATER && obj->bknown
	  && (obj->blessed || obj->cursed)) {
	  Strcpy(buffer, cxname2(obj));
	  (void) strsubst(buffer, obj->blessed ? "holy ": "unholy ", "");
  }
  else if (obj->otyp == POT_BLOOD) {
	  Strcpy(buffer, "potion of blood");
  }
  else {
	  Strcpy(buffer, cxname2(obj));
  }
}

int
sortloot_cmp(obj1, obj2)
     struct obj *obj1;
     struct obj *obj2;
{
  int val1 = 0;
  int val2 = 0;

  char name1[BUFSZ];
  char name2[BUFSZ];

  munge_objnames(obj1, name1);
  munge_objnames(obj2, name2);

  /* Sort object names in lexicographical order. */
  int name_cmp = strcmpi(name1, name2);

  if (name_cmp != 0) {
    return name_cmp;
  }

  /* Sort potions of blood by the corpse they represent */
  if (obj1->otyp == POT_BLOOD && obj2->otyp == POT_BLOOD) {
	  const char *corpse_name1 = obj1->corpsenm == NON_PM
		  ? "" : mons[obj1->corpsenm].mname;
	  const char *corpse_name2 = obj2->corpsenm == NON_PM
		  ? "" : mons[obj2->corpsenm].mname;
	  name_cmp = strcmpi(corpse_name1, corpse_name2);
	  if (name_cmp) return name_cmp;
  }

  /* Sort by BUC. Map blessed to 4, uncursed to 2, cursed to 1, and unknown to 0. */
  val1 = obj1->bknown ? (obj1->blessed << 2) + ((!obj1->blessed && !obj1->cursed) << 1) + obj1->cursed : 0;
  val2 = obj2->bknown ? (obj2->blessed << 2) + ((!obj2->blessed && !obj2->cursed) << 1) + obj2->cursed : 0;
  if (val1 != val2) {
    return val2 - val1; /* Because bigger is better. */
  }

  /* Sort by greasing. This will put the objects in degreasing order. */
  val1 = obj1->greased;
  val2 = obj2->greased;
  if (val1 != val2) {
    return val2 - val1; /* Because bigger is better. */
  }

  /* Sort by erosion. The effective amount is what matters. */
  val1 = greatest_erosion(obj1);
  val2 = greatest_erosion(obj2);
  if (val1 != val2) {
    return val1 - val2; /* Because bigger is WORSE. */
  }

  /* Sort by erodeproofing. Map known-invulnerable to 1, and both
   * known-vulnerable and unknown-vulnerability to 0, because that's how they're displayed. */
  val1 = obj1->rknown && obj1->oerodeproof;
  val2 = obj2->rknown && obj2->oerodeproof;
  if (val1 != val2) {
    return val2 - val1; /* Because bigger is better. */
  }
  /* Sort by enchantment. Map unknown to -1000, which is comfortably below the range of ->spe. */
  val1 = obj1->known ? obj1->spe : -1000;
  val2 = obj2->known ? obj2->spe : -1000;
  if (val1 != val2) {
    return val2 - val1; /* Because bigger is better. */
  }

  return 0; /* They're identical, as far as we're concerned. */
}
#endif


/*
 * Internal function used by display_inventory and getobj that can display
 * inventory and return a count as well as a letter. If out_cnt is not null,
 * any count returned from the menu selection is placed here.
 */
#ifdef DUMP_LOG
static char
display_pickinv(lets, want_reply, out_cnt, want_dump, want_disp)
register const char *lets;
boolean want_reply;
long* out_cnt;
boolean want_dump;
boolean want_disp;
#else
static char
display_pickinv(lets, want_reply, out_cnt)
register const char *lets;
boolean want_reply;
long* out_cnt;
#endif
{
	struct obj *otmp;
#ifdef SORTLOOT
	struct obj **oarray;
	int i, j;
#endif
	char ilet, ret = '\0';
	char *invlet = flags.inv_order;
	int n, classcount;
	winid win;				/* windows being used */
	static winid local_win = WIN_ERR;	/* window for partial menus */
	anything any;
	menu_item *selected;

#ifdef DUMP_LOG
	if (want_disp) {
#endif
	/* overriden by global flag */
	if (flags.perm_invent) {
	    win = (lets && *lets) ? local_win : WIN_INVEN;
	    /* create the first time used */
	    if (win == WIN_ERR)
		win = local_win = create_nhwindow(NHW_MENU);
	} else
	    win = WIN_INVEN;

#ifdef DUMP_LOG
	}
	if (want_dump)   dump("", "Your inventory");
#endif

	/*
	Exit early if no inventory -- but keep going if we are doing
	a permanent inventory update.  We need to keep going so the
	permanent inventory window updates itself to remove the last
	item(s) dropped.  One down side:  the addition of the exception
	for permanent inventory window updates _can_ pop the window
	up when it's not displayed -- even if it's empty -- because we
	don't know at this level if its up or not.  This may not be
	an issue if empty checks are done before hand and the call
	to here is short circuited away.
	*/
	if (!invent && !(flags.perm_invent && !lets && !want_reply)) {
#ifdef DUMP_LOG
	  if (want_disp) {
#endif
#ifndef GOLDOBJ
	    pline("Not carrying anything%s.", u.ugold ? " except gold" : "");
#else
	    pline("Not carrying anything.");
#endif
#ifdef DUMP_LOG
	  }
	  if (want_dump) {
#ifdef GOLDOBJ
	    dump("  ", "Not carrying anything");
#else
	    dump("  Not carrying anything", u.ugold ? " except gold." : ".");
#endif
	  }
#endif
	    return 0;
	}

	/* oxymoron? temporarily assign permanent inventory letters */
	if (!flags.invlet_constant) reassign();

	if (lets && strlen(lets) == 1) {
	    /* when only one item of interest, use pline instead of menus;
	       we actually use a fake message-line menu in order to allow
	       the user to perform selection at the --More-- prompt for tty */
	    for (otmp = invent; otmp; otmp = otmp->nobj) {
		if (otmp->invlet == lets[0]) {
#ifdef DUMP_LOG
		  if (want_disp) {
#endif
		    ret = message_menu(lets[0],
			  want_reply ? PICK_ONE : PICK_NONE,
			  xprname(otmp, (char *)0, lets[0], TRUE, 0L, 0L));
		    if (out_cnt) *out_cnt = -1L;	/* select all */
#ifdef DUMP_LOG
		  }
		  if (want_dump) {
		    char letbuf[7];
		    sprintf(letbuf, "  %c - ", lets[0]);
		    dump(letbuf,
			 xprname(otmp, (char *)0, lets[0], TRUE, 0L, 0L));
		  }
#endif
		    break;
		}
	    }
	    return ret;
	}

#ifdef SORTLOOT
	/* count the number of items */
	for (n = 0, otmp = invent; otmp; otmp = otmp->nobj)
	  if(!lets || !*lets || index(lets, otmp->invlet)) n++;

	/* Make a temporary array to store the objects sorted */
	oarray = (struct obj **)alloc(n*sizeof(struct obj*));

	/* Add objects to the array */
	i = 0;
	for(otmp = invent; otmp; otmp = otmp->nobj)
	  if(!lets || !*lets || index(lets, otmp->invlet)) {
	    if (iflags.sortloot == 'f') {
	      /* Insert object at correct index */
	      for (j = i; j; j--) {
		if (sortloot_cmp(otmp, oarray[j-1]) > 0) break;
		oarray[j] = oarray[j-1];
	      }
	      oarray[j] = otmp;
	      i++;
	    } else {
	      /* Just add it to the array */
	      oarray[i++] = otmp;
	    }
	  }
#endif /* SORTLOOT */

#ifdef DUMP_LOG
	if (want_disp){
#endif
		start_menu(win);
#ifdef DUMP_LOG
		if (!lets){
			any.a_void = 0;
			char invheading[QBUFSZ];
			int wcap = weight_cap();
			Sprintf(invheading, "Inventory: %d/%d weight (%d/52 slots)", inv_weight() + wcap, wcap, inv_cnt());
			add_menu(win, NO_GLYPH, &any, 0, 0, ATR_BOLD, invheading, MENU_UNSELECTED);
		}
	}
#endif
nextclass:
	classcount = 0;
	any.a_void = 0;		/* set all bits to zero */
#ifdef SORTLOOT
	for(i = 0; i < n; i++) {
	  otmp = oarray[i];
	  ilet = otmp->invlet;
	  if (!flags.sortpack || otmp->oclass == *invlet) {
	    if (flags.sortpack && !classcount) {
	      any.a_void = 0;             /* zero */
#ifdef DUMP_LOG
	      if (want_dump)
		  dump("  ", let_to_name(*invlet, FALSE, FALSE));
	      if (want_disp)
#endif
	      add_menu(win, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
		       let_to_name(*invlet, FALSE, FALSE), MENU_UNSELECTED);
	      classcount++;
	    }
	    any.a_char = ilet;
#ifdef DUMP_LOG
	    if (want_dump) {
	      char letbuf[7];
	      sprintf(letbuf, "  %c - ", ilet);
	      dump(letbuf, doname(otmp));
	    }
	    if (want_disp)
#endif
	    add_menu(win, obj_to_glyph(otmp),
		     &any, ilet, 0, ATR_NONE, doname(otmp),
		     MENU_UNSELECTED);
	  }
	}
#else /* SORTLOOT */
	for(otmp = invent; otmp; otmp = otmp->nobj) {
		ilet = otmp->invlet;
		if(!lets || !*lets || index(lets, ilet)) {
			if (!flags.sortpack || otmp->oclass == *invlet) {
			    if (flags.sortpack && !classcount) {
				any.a_void = 0;		/* zero */
#ifdef DUMP_LOG
				if (want_dump)
				    dump("  ", let_to_name(*invlet, FALSE, FALSE));
				if (want_disp)
#endif
				add_menu(win, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
					 let_to_name(*invlet, FALSE, FALSE), MENU_UNSELECTED);
				classcount++;
			    }
			    any.a_char = ilet;
#ifdef DUMP_LOG
			    if (want_dump) {
			      char letbuf[7];
			      sprintf(letbuf, "  %c - ", ilet);
			      dump(letbuf, doname(otmp));
			    }
			    if (want_disp)
#endif
			    add_menu(win, obj_to_glyph(otmp),
					&any, ilet, 0, ATR_NONE, doname(otmp),
					MENU_UNSELECTED);
			}
		}
	}
#endif /* SORTLOOT */
	if (flags.sortpack) {
		if (*++invlet) goto nextclass;
#ifdef WIZARD
		if (--invlet != venom_inv) {
			invlet = venom_inv;
			goto nextclass;
		}
#endif
	}
#ifdef SORTLOOT
	free(oarray);
#endif
#ifdef DUMP_LOG
	if (want_disp) {
#endif
	end_menu(win, (char *) 0);

	n = select_menu(win, want_reply ? PICK_ONE : PICK_NONE, &selected);
	if (n > 0) {
	    ret = selected[0].item.a_char;
	    if (out_cnt) *out_cnt = selected[0].count;
	    free((genericptr_t)selected);
	} else
	    ret = !n ? '\0' : '\033';	/* cancelled */
#ifdef DUMP_LOG
	} /* want_disp */
	if (want_dump)  dump("", "");
#endif

	return ret;
}

/*
 * If lets == NULL or "", list all objects in the inventory.  Otherwise,
 * list all objects with object classes that match the order in lets.
 *
 * Returns the letter identifier of a selected item, or 0 if nothing
 * was selected.
 */
char
display_inventory(lets, want_reply)
register const char *lets;
boolean want_reply;
{
	char retval;
	flags.disp_inv = TRUE;
	retval = display_pickinv(lets, want_reply, (long *)0
#ifdef DUMP_LOG
			       , FALSE , TRUE
#endif
	);
	flags.disp_inv = FALSE;
	return retval;
}

#ifdef DUMP_LOG
/* See display_inventory. This is the same thing WITH dumpfile creation */
char
dump_inventory(lets, want_reply, want_disp)
register const char *lets;
boolean want_reply, want_disp;
{
  return display_pickinv(lets, want_reply, (long *)0, TRUE, want_disp);
}
#endif

/*
 * Returns the number of unpaid items within the given list.  This includes
 * contained objects.
 */
int
count_unpaid(list)
    struct obj *list;
{
    int count = 0;

    while (list) {
	if (list->unpaid) count++;
	if (Has_contents(list))
	    count += count_unpaid(list->cobj);
	list = list->nobj;
    }
    return count;
}

/*
 * Checks if there is an item of matching otyp within the given list.  This includes
 * contained objects.
 */
boolean
has_object_type(list, otyp)
struct obj *list;
int otyp;
{
	while (list) {
		if (list->otyp == otyp) return TRUE;
		if (Has_contents(list))
			if(has_object_type(list->cobj, otyp))
				return TRUE;
		list = list->nobj;
	}
	return FALSE;
}

/*
 * Finds the first item of matching otyp within the given list. Does not check contained objects.
 */
struct obj *
find_object_type(list, otyp)
struct obj *list;
int otyp;
{
	while (list) {
		if (list->otyp == otyp) return list;
		list = list->nobj;
	}
	return (struct obj *) 0;
}

/*
 * Finds the first item of matching otyp within the given list for which spe > 0. Does not check contained objects.
 */
struct obj *
find_charged_object_type(list, otyp)
struct obj *list;
int otyp;
{
	while (list) {
		if (list->otyp == otyp && list->spe > 0) return list;
		list = list->nobj;
	}
	return (struct obj *) 0;
}

/*
 * Returns the number of items with b/u/c/unknown within the given list.  
 * This does NOT include contained objects.
 */
int
count_buc(list, type)
    struct obj *list;
    int type;
{
    int count = 0;

    while (list) {
	if (Role_if(PM_PRIEST)) list->bknown = TRUE;
	switch(type) {
	    case BUC_BLESSED:
		if (list->oclass != COIN_CLASS && list->bknown && list->blessed)
		    count++;
		break;
	    case BUC_CURSED:
		if (list->oclass != COIN_CLASS && list->bknown && list->cursed)
		    count++;
		break;
	    case BUC_UNCURSED:
		if (list->oclass != COIN_CLASS &&
			list->bknown && !list->blessed && !list->cursed)
		    count++;
		break;
	    case BUC_UNKNOWN:
		if (list->oclass != COIN_CLASS && !list->bknown)
		    count++;
		break;
	    default:
		impossible("need count of curse status %d?", type);
		return 0;
	}
	list = list->nobj;
    }
    return count;
}

STATIC_OVL void
dounpaid()
{
    winid win;
    struct obj *otmp, *marker;
    register char ilet;
    char *invlet = flags.inv_order;
    int classcount, count, num_so_far;
    int save_unpaid = 0;	/* lint init */
    long cost, totcost;

    count = count_unpaid(invent);

    if (count == 1) {
	marker = (struct obj *) 0;
	otmp = find_unpaid(invent, &marker);

	/* see if the unpaid item is in the top level inventory */
	for (marker = invent; marker; marker = marker->nobj)
	    if (marker == otmp) break;

	pline("%s", xprname(otmp, distant_name(otmp, doname),
			    marker ? otmp->invlet : CONTAINED_SYM,
			    TRUE, unpaid_cost(otmp), 0L));
	return;
    }

    win = create_nhwindow(NHW_MENU);
    cost = totcost = 0;
    num_so_far = 0;	/* count of # printed so far */
    if (!flags.invlet_constant) reassign();

    do {
	classcount = 0;
	for (otmp = invent; otmp; otmp = otmp->nobj) {
	    ilet = otmp->invlet;
	    if (otmp->unpaid) {
		if (!flags.sortpack || otmp->oclass == *invlet) {
		    if (flags.sortpack && !classcount) {
			putstr(win, 0, let_to_name(*invlet, TRUE, FALSE));
			classcount++;
		    }

		    totcost += cost = unpaid_cost(otmp);
		    /* suppress "(unpaid)" suffix */
		    save_unpaid = otmp->unpaid;
		    otmp->unpaid = 0;
		    putstr(win, 0, xprname(otmp, distant_name(otmp, doname),
					   ilet, TRUE, cost, 0L));
		    otmp->unpaid = save_unpaid;
		    num_so_far++;
		}
	    }
	}
    } while (flags.sortpack && (*++invlet));

    if (count > num_so_far) {
	/* something unpaid is contained */
	if (flags.sortpack)
	    putstr(win, 0, let_to_name(CONTAINED_SYM, TRUE, FALSE));
	/*
	 * Search through the container objects in the inventory for
	 * unpaid items.  The top level inventory items have already
	 * been listed.
	 */
	for (otmp = invent; otmp; otmp = otmp->nobj) {
	    if (Has_contents(otmp)) {
		marker = (struct obj *) 0;	/* haven't found any */
		while (find_unpaid(otmp->cobj, &marker)) {
		    totcost += cost = unpaid_cost(marker);
		    save_unpaid = marker->unpaid;
		    marker->unpaid = 0;    /* suppress "(unpaid)" suffix */
		    putstr(win, 0,
			   xprname(marker, distant_name(marker, doname),
				   CONTAINED_SYM, TRUE, cost, 0L));
		    marker->unpaid = save_unpaid;
		}
	    }
	}
    }

    putstr(win, 0, "");
    putstr(win, 0, xprname((struct obj *)0, "Total:", '*', FALSE, totcost, 0L));
    display_nhwindow(win, FALSE);
    destroy_nhwindow(win);
}


/* query objlist callback: return TRUE if obj type matches "this_type" */
static int this_type;

STATIC_OVL boolean
this_type_only(struct obj *obj, int qflags)
{
	if(qflags&NO_EQUIPMENT && obj->owornmask)
		return FALSE;
    return (obj->oclass == this_type);
}

/* the 'I' command */
int
dotypeinv()
{
	char c = '\0';
	int n, i = 0;
	char *extra_types, types[BUFSZ];
	int class_count, oclass, unpaid_count, itemcount;
	boolean billx = *u.ushops && doinvbill(0);
	menu_item *pick_list;
	boolean traditional = TRUE;
	const char *prompt = "What type of object do you want an inventory of?";

#ifndef GOLDOBJ
	if (!invent && !u.ugold && !billx) {
#else
	if (!invent && !billx) {
#endif
	    You("aren't carrying anything.");
	    return MOVE_CANCELLED;
	}
	unpaid_count = count_unpaid(invent);
	if (flags.menu_style != MENU_TRADITIONAL) {
	    if (flags.menu_style == MENU_FULL ||
				flags.menu_style == MENU_PARTIAL) {
		traditional = FALSE;
		i = UNPAID_TYPES;
		if (billx) i |= BILLED_TYPES;
		n = query_category(prompt, invent, i, &pick_list, PICK_ONE);
		if (!n) return MOVE_CANCELLED;
		this_type = c = pick_list[0].item.a_int;
		free((genericptr_t) pick_list);
	    }
	}
	if (traditional) {
	    /* collect a list of classes of objects carried, for use as a prompt */
	    types[0] = 0;
	    class_count = collect_obj_classes(types, invent,
					      FALSE,
#ifndef GOLDOBJ
					      (u.ugold != 0),
#endif
					      (boolean FDECL((*),(OBJ_P))) 0, &itemcount);
	    if (unpaid_count) {
		Strcat(types, "u");
		class_count++;
	    }
	    if (billx) {
		Strcat(types, "x");
		class_count++;
	    }
	    /* add everything not already included; user won't see these */
	    extra_types = eos(types);
	    *extra_types++ = '\033';
	    if (!unpaid_count) *extra_types++ = 'u';
	    if (!billx) *extra_types++ = 'x';
	    *extra_types = '\0';	/* for index() */
	    for (i = 0; i < MAXOCLASSES; i++)
		if (!index(types, def_oc_syms[i])) {
		    *extra_types++ = def_oc_syms[i];
		    *extra_types = '\0';
		}

	    if(class_count > 1) {
		c = yn_function(prompt, types, '\0');
#ifdef REDO
		savech(c);
#endif
		if(c == '\0') {
			clear_nhwindow(WIN_MESSAGE);
			return MOVE_CANCELLED;
		}
	    } else {
		/* only one thing to itemize */
		if (unpaid_count)
		    c = 'u';
		else if (billx)
		    c = 'x';
		else
		    c = types[0];
	    }
	}
	if (c == 'x') {
	    if (billx)
		(void) doinvbill(1);
	    else
		pline("No used-up objects on your shopping bill.");
	    return MOVE_CANCELLED;
	}
	if (c == 'u') {
	    if (unpaid_count)
		dounpaid();
	    else
		You("are not carrying any unpaid objects.");
	    return MOVE_CANCELLED;
	}
	if (traditional) {
	    oclass = def_char_to_objclass(c); /* change to object class */
	    if (oclass == COIN_CLASS) {
		return doprgold();
	    } else if (index(types, c) > index(types, '\033')) {
		You("have no such objects.");
		return MOVE_CANCELLED;
	    }
	    this_type = oclass;
	}
	if (query_objlist((char *) 0, invent,
		    (flags.invlet_constant ? USE_INVLET : 0)|INVORDER_SORT,
		    &pick_list, PICK_NONE, this_type_only) > 0)
	    free((genericptr_t)pick_list);
	return MOVE_CANCELLED;
}

/* return a string describing the dungeon feature at <x,y> if there
   is one worth mentioning at that location; otherwise null */
const char *
dfeature_at(x, y, buf)
int x, y;
char *buf;
{
	struct rm *lev = &levl[x][y];
	int ltyp = lev->typ, cmap = -1;
	const char *dfeature = 0;
	static char altbuf[BUFSZ];

	if (IS_DOOR(ltyp)) {
	    switch (lev->doormask) {
	    case D_NODOOR:	cmap = S_ndoor; break;	/* "doorway" */
	    case D_ISOPEN:	cmap = S_vodoor; break;	/* "open door" */
	    case D_BROKEN:	dfeature = "broken door"; break;
	    default:	cmap = S_vcdoor; break;	/* "closed door" */
	    }
	    /* override door description for open drawbridge */
	    if (is_drawbridge_wall(x, y) >= 0)
		dfeature = "open drawbridge portcullis",  cmap = -1;
	} else if (IS_FOUNTAIN(ltyp))
	    cmap = S_fountain;				/* "fountain" */
	else if (IS_FORGE(ltyp))
	    cmap = S_forge;				/* "forge" */
	else if (IS_THRONE(ltyp))
	    cmap = S_throne;				/* "opulent throne" */
	else if (is_lava(x,y))
	    cmap = S_lava;				/* "molten lava" */
	else if (is_ice(x,y))
	    cmap = S_ice;				/* "ice" */
	else if (is_pool(x,y, FALSE))
	    dfeature = "pool of water";
	else if (IS_PUDDLE(ltyp))
	    dfeature = "puddle of shallow water";
	else if (IS_GRASS(ltyp))
	    cmap = S_litgrass;				/* "grass" */
	else if (IS_SOIL(ltyp))
	    cmap = S_litsoil;				/* "soil" */
	else if (IS_SAND(ltyp))
	    cmap = S_litsand;				/* "sand" */
#ifdef SINKS
	else if (IS_SINK(ltyp))
	    cmap = S_sink;				/* "sink" */
#endif
	else if (IS_ALTAR(ltyp)) {
		if(a_gnum(x,y) != GOD_NONE) {
			Sprintf(altbuf, "altar to %s (%s)", a_gname(),
				align_str(a_align(x,y)));
		}
		else {
			Sprintf(altbuf, "%s altar", align_str(a_align(x,y)));
		}
	    dfeature = altbuf;
	} else if ((x == xupstair && y == yupstair) ||
		 (x == sstairs.sx && y == sstairs.sy && sstairs.up && !sstairs.u_traversed))
	    cmap = S_upstair;				/* "staircase up" */
	else if (x == sstairs.sx && y == sstairs.sy && sstairs.up && sstairs.u_traversed)
	    cmap = S_brupstair;				/* "staircase up" */
	else if ((x == xdnstair && y == ydnstair) ||
		 (x == sstairs.sx && y == sstairs.sy && !sstairs.up && !sstairs.u_traversed))
	    cmap = S_dnstair;				/* "staircase down" */
	else if (x == sstairs.sx && y == sstairs.sy && !sstairs.up && sstairs.u_traversed)
	    cmap = S_brdnstair;	
	else if (x == xupladder && y == yupladder)
	    cmap = S_upladder;				/* "ladder up" */
	else if (x == xdnladder && y == ydnladder)
	    cmap = S_dnladder;				/* "ladder down" */
	else if (ltyp == DRAWBRIDGE_DOWN)
	    cmap = S_vodbridge;			/* "lowered drawbridge" */
	else if (ltyp == DBWALL)
	    cmap = S_vcdbridge;			/* "raised drawbridge" */
	else if (IS_GRAVE(ltyp))
	    cmap = S_grave;				/* "grave" */
	else if (IS_SEAL(ltyp))
	    cmap = S_seal;				/* "seal" */
	else if (ltyp == TREE)
	    cmap = S_tree;				/* "tree" */
	else if (ltyp == IRONBARS)
	    dfeature = "set of iron bars";
	else if (ltyp == DEADTREE)
	    cmap = S_deadtree;

	if (cmap >= 0) dfeature = defsyms[cmap].explanation;
	if (dfeature) Strcpy(buf, dfeature);
	return dfeature;
}

/* look at what is here; if there are many objects (5 or more),
   don't show them unless obj_cnt is 0 */
int
look_here(obj_cnt, picked_some)
int obj_cnt;	/* obj_cnt > 0 implies that autopickup is in progess */
boolean picked_some;
{
	struct obj *otmp;
	struct trap *trap;
	const char *verb = Blind ? "feel" : "see";
	const char *dfeature = (char *)0;
	char fbuf[BUFSZ], fbuf2[BUFSZ];
	winid tmpwin;
	boolean skip_objects = (obj_cnt >= 5), felt_cockatrice = FALSE;

	if (u.uswallow && u.ustuck) {
	    struct monst *mtmp = u.ustuck;
	    Sprintf(fbuf, "Contents of %s %s",
		s_suffix(mon_nam(mtmp)), mbodypart(mtmp, STOMACH));
	    /* Skip "Contents of " by using fbuf index 12 */
	    You("%s to %s what is lying in %s.",
		Blind ? "try" : "look around", verb, &fbuf[12]);
	    otmp = mtmp->minvent;
	    if (otmp) {
		for ( ; otmp; otmp = otmp->nobj) {
			/* If swallower is an animal, it should have become stone but... */
			if (otmp->otyp == CORPSE) feel_cockatrice(otmp, FALSE);
		}
		if (Blind) Strcpy(fbuf, "You feel");
		Strcat(fbuf,":");
	    	(void) display_minventory(mtmp, MINV_ALL, fbuf);
	    } else {
		You("%s no objects here.", verb);
	    }
	    return (Blind ? MOVE_STANDARD : MOVE_INSTANT);
	}
	if (!skip_objects && (trap = t_at(u.ux,u.uy)) && trap->tseen)
		There("is %s here.",
			an(defsyms[trap_to_defsym(trap->ttyp)].explanation));

	otmp = level.objects[u.ux][u.uy];
	dfeature = dfeature_at(u.ux, u.uy, fbuf2);
	if (dfeature && !strcmp(dfeature, "pool of water") && Underwater)
		dfeature = 0;

	if (Blind) {
		boolean drift = Weightless || Is_waterlevel(&u.uz);
		if (dfeature && !strncmp(dfeature, "altar ", 6)) {
		    /* don't say "altar" twice, dfeature has more info */
		    You("try to feel what is here.");
		} else {
		    You("try to feel what is %s%s.",
			drift ? "floating here" : "lying here on the ",
			drift ? ""		: surface(u.ux, u.uy));
		}
		if (dfeature && !drift && !strcmp(dfeature, surface(u.ux,u.uy)))
			dfeature = 0;		/* ice already identifed */
		if (!can_reach_floor()) {
			pline("But you can't reach it!");
			return MOVE_INSTANT;
		}
	}

	if (dfeature)
		Sprintf(fbuf, "There is %s here.", an(dfeature));

	if (!otmp || (is_lava(u.ux,u.uy) && !likes_lava(youracedata)) || (is_pool(u.ux,u.uy, FALSE) && !Underwater)) {
		if (dfeature) pline1(fbuf);
		read_engr_at(u.ux, u.uy); /* Eric Backus */
		if (!skip_objects && (Blind || !dfeature))
		    You("%s no objects here.", verb);
		return (Blind ? MOVE_STANDARD : MOVE_INSTANT);
	}
	/* we know there is something here */

	if (skip_objects) {
	    if (dfeature) pline1(fbuf);
	    read_engr_at(u.ux, u.uy); /* Eric Backus */
	    There("are %s%s objects here.",
		  (obj_cnt <= 10) ? "several" : "many",
		  picked_some ? " more" : "");
	} else if (!otmp->nexthere) {
	    /* only one object */
	    if (dfeature) pline1(fbuf);
	    read_engr_at(u.ux, u.uy); /* Eric Backus */
#ifdef INVISIBLE_OBJECTS
		if (otmp->oinvis && !See_invisible(otmp->ox, otmp->oy)) verb = "feel";
#endif
	    You("%s here %s.", verb, Hallucination ? an(rndobjnam()) : doname_with_price(otmp));
	    if (otmp->otyp == CORPSE) feel_cockatrice(otmp, FALSE);
	} else {
	    display_nhwindow(WIN_MESSAGE, FALSE);
	    tmpwin = create_nhwindow(NHW_MENU);
	    if(dfeature) {
		putstr(tmpwin, 0, fbuf);
		putstr(tmpwin, 0, "");
	    }
	    putstr(tmpwin, 0, Blind ? "Things that you feel here:" :
				      "Things that are here:");
	    for ( ; otmp; otmp = otmp->nexthere) {
		if (otmp->otyp == CORPSE && will_feel_cockatrice(otmp, FALSE)) {
			char buf[BUFSZ];
			felt_cockatrice = TRUE;
			Strcpy(buf, Hallucination ? an(rndobjnam()) : doname(otmp));
			Strcat(buf, "...");
			putstr(tmpwin, 0, buf);
			break;
		}
		putstr(tmpwin, 0, Hallucination ? an(rndobjnam()) : doname_with_price(otmp));
	    }
	    display_nhwindow(tmpwin, TRUE);
	    destroy_nhwindow(tmpwin);
	    if (felt_cockatrice) feel_cockatrice(otmp, FALSE);
	    read_engr_at(u.ux, u.uy); /* Eric Backus */
	}
	return (Blind ? MOVE_STANDARD : MOVE_INSTANT);
}

/* explicilty look at what is here, including all objects */
int
dolook()
{
	return look_here(0, FALSE);
}

boolean
will_feel_cockatrice(otmp, force_touch)
struct obj *otmp;
boolean force_touch;
{
	if ((Blind || force_touch) && !uarmg && !Stone_resistance &&
		(otmp->otyp == CORPSE && touch_petrifies(&mons[otmp->corpsenm])))
			return TRUE;
	return FALSE;
}

void
feel_cockatrice(otmp, force_touch)
struct obj *otmp;
boolean force_touch;
{
	char kbuf[BUFSZ];

	if (will_feel_cockatrice(otmp, force_touch)) {
	    if(poly_when_stoned(youracedata))
			You("touched the %s corpse with your bare %s.",
				mons[otmp->corpsenm].mname, makeplural(body_part(HAND)));
	    else
			pline("Touching the %s corpse is a fatal mistake...",
				mons[otmp->corpsenm].mname);
		Sprintf(kbuf, "%s corpse", an(mons[otmp->corpsenm].mname));
		instapetrify(kbuf);
	}
}

#endif /* OVLB */
#ifdef OVL1

void
stackobj(obj)
struct obj *obj;
{
	struct obj *otmp;

	for(otmp = level.objects[obj->ox][obj->oy]; otmp; otmp = otmp->nexthere)
		if(otmp != obj && merged(&obj,&otmp))
			break;
	return;
}

STATIC_OVL boolean
mergable(otmp, obj)	/* returns TRUE if obj  & otmp can be merged */
	register struct obj *otmp, *obj;
{
	if (obj->otyp != otmp->otyp) return FALSE;
//#ifdef GOLDOBJ
	/* coins of the same kind will always merge */
	if (obj->oclass == COIN_CLASS) return TRUE;
//#endif
	if(!mergable_traits(otmp, obj))
		return FALSE;
	
	if(obj->sknown || otmp->sknown) obj->sknown = otmp->sknown = 1;
	
	if(obj->known == otmp->known ||
		!objects[otmp->otyp].oc_uses_known) {
		return((boolean)(objects[obj->otyp].oc_merge));
	} else return(FALSE);
}

boolean
mergable_traits(otmp, obj)	/* returns TRUE if obj  & otmp can be merged */
	register struct obj *otmp, *obj;
{
	if (obj->unpaid != otmp->unpaid ||
		obj->ostolen != otmp->ostolen ||
		(obj->ostolen && obj->sknown != otmp->sknown) ||
		(obj->ovar1 != otmp->ovar1 && obj->otyp != CORPSE && obj->otyp != FORCE_BLADE && obj->otyp != RAKUYO_DAGGER && obj->otyp != BLADE_OF_PITY) ||
		(obj->oward != otmp->oward) ||
	    obj->spe != otmp->spe || obj->dknown != otmp->dknown ||
	    (obj->bknown != otmp->bknown && !Role_if(PM_PRIEST)) ||
	    obj->cursed != otmp->cursed || obj->blessed != otmp->blessed ||
	    obj->no_charge != otmp->no_charge ||
	    obj->obroken != otmp->obroken ||
	    obj->objsize != otmp->objsize ||
	    obj->otrapped != otmp->otrapped ||
	    obj->lamplit != otmp->lamplit ||
#ifdef INVISIBLE_OBJECTS
		obj->oinvis != otmp->oinvis ||
#endif
	    obj->greased != otmp->greased ||
	    obj->oeroded != otmp->oeroded ||
	    obj->oeroded2 != otmp->oeroded2 ||
	    obj->oeroded3 != otmp->oeroded3 ||
		obj->odiluted != otmp->odiluted ||
	    obj->obj_material != otmp->obj_material ||
	    obj->bypass != otmp->bypass)
	    return(FALSE);
	
	/* cannot be marked nomerge */
	if (obj->nomerge || otmp->nomerge)
		return FALSE;

	if ((obj->oclass==WEAPON_CLASS || obj->oclass==ARMOR_CLASS) &&
	    (obj->oerodeproof!=otmp->oerodeproof))
	    return FALSE;

	if (obj->oclass == FOOD_CLASS && (obj->oeaten != otmp->oeaten ||
	  obj->odrained != otmp->odrained || obj->orotten != otmp->orotten))
	    return(FALSE);

	if (obj->otyp == CORPSE || obj->otyp == EYEBALL || obj->otyp == EGG || obj->otyp == TIN || obj->otyp == POT_BLOOD) {
		if (obj->corpsenm != otmp->corpsenm)
				return FALSE;
	}

	/* poisons must be the same */
	if (obj->opoisoned != otmp->opoisoned)
	    return FALSE;
	
	/* hatching eggs don't merge; ditto for revivable corpses */
	if ((obj->otyp == EGG && (obj->timed || otmp->timed)) ||
	    (obj->otyp == CORPSE && otmp->corpsenm >= LOW_PM &&
		is_reviver(&mons[otmp->corpsenm])))
	    return FALSE;

	/* allow candle merging only if their ages are close */
	/* see begin_burn() for a reference for the magic "25" */
	if ((Is_candle(obj) || Is_torch(obj)) && obj->age/25 != otmp->age/25)
	    return(FALSE);

	if (obj->otyp == SUNROD && obj->lamplit)
	    return(FALSE);

	if (obj->oartifact && obj->age/25 != otmp->age/25)
	    return(FALSE);

	/* burning potions of oil never merge */
	if (obj->otyp == POT_OIL && obj->lamplit)
	    return FALSE;

	/* don't merge surcharged item with base-cost item */
	if (obj->unpaid && !same_price(obj, otmp))
	    return FALSE;

	/* if they have names, make sure they're the same */
	if ((get_ox(obj, OX_ENAM) || get_ox(otmp, OX_ENAM)) &&
		(get_ox(obj, OX_ENAM) && get_ox(otmp, OX_ENAM))
			? strcmp(ONAME(obj), ONAME(otmp))	/* both named; stack unless different names */
			: obj->otyp == CORPSE				/* one named; stack unless corpse */
	)
		return FALSE;

	/* for the moment, any additional information is incompatible */
	if (get_ox(obj,  OX_EMON) || get_ox(obj,  OX_EMID) || get_ox(obj,  OX_ESUM) ||
		get_ox(otmp, OX_EMON) || get_ox(otmp, OX_EMID) || get_ox(otmp, OX_ESUM)) return FALSE;

	if(obj->oartifact != otmp->oartifact) return FALSE;
	
	if(!oprops_match(obj, otmp)) return FALSE;
	
	return TRUE;
}

int
doprgold()
{
	/* the messages used to refer to "carrying gold", but that didn't
	   take containers into account */
#ifndef GOLDOBJ
	if(!u.ugold)
	    Your("wallet is empty.");
	else
	    Your("wallet contains %ld gold piece%s.", u.ugold, plur(u.ugold));
#else
        long umoney = money_cnt(invent);
	if(!umoney)
	    Your("wallet is empty.");
	else
	    Your("wallet contains %ld %s.", umoney, currency(umoney));
#endif
	shopper_financial_report();
	return MOVE_CANCELLED;
}

#endif /* OVL1 */
#ifdef OVLB

int
doprwep()
{
	if (u.twoweap){
		if (!uwep) Your("main %s is empty.", body_part(HAND));
		else prinv((char *)0, uwep, 0L);
		if (!uswapwep) Your("other %s is %sempty.", body_part(HAND), !uwep ? "also " : "");
		else prinv((char *)0, uswapwep, 0L);
	} else {
		if (!uwep) You("are empty %s.", body_part(HANDED));
		else prinv((char *)0, uwep, 0L);
	}
    return MOVE_CANCELLED;
}

int
doprarm()
{
	if(!wearing_armor())
		You("are not wearing any armor.");
	else {
#ifdef TOURIST
		char lets[8];
#else
		char lets[7];
#endif
		register int ct = 0;

#ifdef TOURIST
		if(uarmu) lets[ct++] = obj_to_let(uarmu);
#endif
		if(uarm) lets[ct++] = obj_to_let(uarm);
		if(uarmc) lets[ct++] = obj_to_let(uarmc);
		if(uarmh) lets[ct++] = obj_to_let(uarmh);
		if(uarms) lets[ct++] = obj_to_let(uarms);
		if(uarmg) lets[ct++] = obj_to_let(uarmg);
		if(uarmf) lets[ct++] = obj_to_let(uarmf);
		lets[ct] = 0;
		(void) display_inventory(lets, FALSE);
	}
	udr_enlightenment();
	return MOVE_CANCELLED;
}

int
doprring()
{
	if(!uleft && !uright)
		You("are not wearing any rings.");
	else {
		char lets[3];
		register int ct = 0;

		if(uleft) lets[ct++] = obj_to_let(uleft);
		if(uright) lets[ct++] = obj_to_let(uright);
		lets[ct] = 0;
		(void) display_inventory(lets, FALSE);
	}
	return MOVE_CANCELLED;
}

int
dopramulet()
{
	if (!uamul)
		You("are not wearing an amulet.");
	else
		prinv((char *)0, uamul, 0L);
	return MOVE_CANCELLED;
}

STATIC_OVL boolean
tool_in_use(obj)
struct obj *obj;
{
	if ((obj->owornmask & (W_TOOL
#ifdef STEED
			| W_SADDLE
#endif
			)) != 0L) return TRUE;
	if (obj->oclass != TOOL_CLASS) return FALSE;
	return (boolean)(obj == uwep || obj->lamplit ||
				(obj->otyp == LEASH && obj->leashmon));
}

int
doprtool()
{
	struct obj *otmp;
	int ct = 0;
	char lets[52+1];

	for (otmp = invent; otmp; otmp = otmp->nobj)
	    if (tool_in_use(otmp))
		lets[ct++] = obj_to_let(otmp);
	lets[ct] = '\0';
	if (!ct) You("are not using any tools.");
	else (void) display_inventory(lets, FALSE);
	return MOVE_CANCELLED;
}

/* '*' command; combines the ')' + '[' + '=' + '"' + '(' commands;
   show inventory of all currently wielded, worn, or used objects */
int
doprinuse()
{
	struct obj *otmp;
	int ct = 0;
	char lets[52+1];

	for (otmp = invent; otmp; otmp = otmp->nobj)
	    if (is_worn(otmp, 0) || tool_in_use(otmp))
		lets[ct++] = obj_to_let(otmp);
	lets[ct] = '\0';
	if (!ct) You("are not wearing or wielding anything.");
	else (void) display_inventory(lets, FALSE);
	return MOVE_CANCELLED;
}

/*
 * uses up an object that's on the floor, charging for it as necessary
 */
void
useupf(obj, numused)
register struct obj *obj;
long numused;
{
	register struct obj *otmp;
	boolean at_u = (obj->ox == u.ux && obj->oy == u.uy);

	/* burn_floor_paper() keeps an object pointer that it tries to
	 * useupf() multiple times, so obj must survive if plural */
	if (obj->quan > numused)
		otmp = splitobj(obj, numused);
	else
		otmp = obj;
	if(costly_spot(otmp->ox, otmp->oy)) {
	    if(index(u.urooms, *in_rooms(otmp->ox, otmp->oy, 0)))
	        addtobill(otmp, FALSE, FALSE, FALSE);
	    else (void)stolen_value(otmp, otmp->ox, otmp->oy, FALSE, FALSE);
	}
	delobj(otmp);
	if (at_u && u.uundetected && hides_under(youracedata))
	    u.uundetected = OBJ_AT(u.ux, u.uy);
}

#endif /* OVLB */


#ifdef OVL1

/*
 * Conversion from a class to a string for printing.
 * This must match the object class order.
 */
STATIC_VAR NEARDATA const char *names[] = { 0,
	"Illegal objects", "Weapons", "Armor", "Rings", "Amulets",
	"Tools", "Comestibles", "Potions", "Scrolls", "Spellbooks",
	"Wands", "Coins", "Gems", "Boulders/Statues", "Iron balls",
	"Scrap", "Venoms", "Tiles", "Furnature", "Strange coins",
	"Belts"
};

STATIC_VAR NEARDATA const char *bogusclasses[] = {
	"Illegal objects", "Weapons", "Armor", "Rings", "Amulets",
	"Tools", "Comestibles", "Potions", "Scrolls", "Spellbooks",
	"Wands", "Coins", "Gems", "Boulders/Statues", "Iron balls",
	"Scrap", "Venoms","Tiles", "Furnature", "Strange coins",
	"Filler","Useless objects", "Artifacts", "Ascension kit items",
	"Staves", "Songs", "Drinks", "Grimoires", "Gears", "Cogs",
	"Marmosets", "Bugs", "Easter Eggs", "Tiny Monuments","Consumables",
	"Junk", "FOOs", "BARs", "Spoilers", "YANIs", "Splatbooks", 
	"Chains", "Paperwork", "Pop-culture references", "Dross",
	"Pokemon","Forgotten escape items","Useless flavor items",
	"SCPs","Bloat"
};

static NEARDATA const char oth_symbols[] = {
	CONTAINED_SYM,
	'\0'
};

static NEARDATA const char *oth_names[] = {
	"Bagged/Boxed items"
};

static NEARDATA char *invbuf = (char *)0;
static NEARDATA unsigned invbufsiz = 0;

char *
let_to_name(let,unpaid,showsym)
char let;
boolean unpaid;
boolean showsym;
{
	static const char *ocsymformat = "%s('%c')";
	const char *class_name;
	const char *pos;
	int oclass = (let >= 1 && let < MAXOCLASSES) ? let : 0;
	unsigned len;

	if (oclass)
	    class_name = names[oclass];
	else if ((pos = index(oth_symbols, let)) != 0)
	    class_name = oth_names[pos - oth_symbols];
	else
	    class_name = names[0];

	len = strlen(class_name) + (unpaid ? sizeof "unpaid_" : sizeof "") + 10;
	if (len > invbufsiz) {
	    if (invbuf) free((genericptr_t)invbuf);
	    invbufsiz = len + 10; /* add slop to reduce incremental realloc */
	    invbuf = (char *) alloc(invbufsiz);
	}
	if (unpaid)
	    Strcat(strcpy(invbuf, "Unpaid "), class_name);
	else
	    Strcpy(invbuf, class_name);
	if (oclass && showsym)
	    Sprintf(eos(invbuf), ocsymformat,
		    iflags.menu_tab_sep ? "\t" : "  ", def_oc_syms[(int)let]);
	return invbuf;
}

char *
rand_class_name()
{
       int name;
       const char *class_name;
 	   unsigned len;
       name = rn2(SIZE(bogusclasses));
	   class_name = bogusclasses[name];
	   len = strlen(class_name);
	   if (len > invbufsiz) {
			if (invbuf) free((genericptr_t)invbuf);
			invbufsiz = len + 10; /* add slop to reduce incremental realloc */
			invbuf = (char *) alloc(invbufsiz);
	   }
	   Strcpy(invbuf, class_name);
	   return invbuf;
}

void
free_invbuf()
{
	if (invbuf) free((genericptr_t)invbuf),  invbuf = (char *)0;
	invbufsiz = 0;
}

#endif /* OVL1 */
#ifdef OVLB

void
reassign()
{
	register int i;
	register struct obj *obj;

	for(obj = invent, i = 0; obj && i < 52; obj = obj->nobj, i++)
		obj->invlet = (i < 26) ? ('a'+i) : ('A'+i-26);

	/* If there are more than 52 items to assign, the rest are NOINVSYM (#) */
	for(; obj; obj = obj->nobj)
		obj->invlet = NOINVSYM;
	lastinvnr = i % 52;
}

#endif /* OVLB */
#ifdef OVL1

/** #adjust command
 *
 *      User specifies a 'from' slot for inventory stack to move,
 *      then a 'to' slot for its destination.  Open slots and those
 *      filled by compatible stacks are listed as likely candidates
 *      but user can pick any inventory letter (including 'from').
 *
 *  to == from, 'from' has a name
 *      All compatible items (same name or no name) are gathered
 *      into the 'from' stack.  No count is allowed.
 *  to == from, 'from' does not have a name
 *      All compatible items without a name are gathered into the
 *      'from' stack.  No count is allowed.  Compatible stacks with
 *      names are left as-is.
 *  to != from, no count
 *      Move 'from' to 'to'.  If 'to' is not empty, merge 'from'
 *      into it if possible, otherwise swap it with the 'from' slot.
 *  to != from, count given
 *      If the user specifies a count when choosing the 'from' slot,
 *      and that count is less than the full size of the stack,
 *      then the stack will be split.  The 'count' portion is moved
 *      to the destination, and the only candidate for merging with
 *      it is the stack already at the 'to' slot, if any.  When the
 *      destination is non-empty but won't merge, whatever is there
 *      will be moved to an open slot; if there isn't any open slot
 *      available, the adjustment attempt fails.
 *
 *      To minimize merging for 'from == to', unnamed stacks will
 *      merge with named 'from' but named ones won't merge with
 *      unnamed 'from'.  Otherwise attempting to collect all unnamed
 *      stacks would lump the first compatible named stack with them
 *      and give them its name.
 *
 *      To maximize merging for 'from != to', compatible stacks will
 *      merge when either lacks a name (or they already have the same
 *      name).  When no count is given and one stack has a name and
 *      the other doesn't, the merged result will have that name.
 *      However, when splitting results in a merger, the name of the
 *      destination overrides that of the source, even if destination
 *      is unnamed and source is named.
 */
int
doorganize(void) /* inventory organizer by Del Lamb */
{
	struct obj *obj, *otmp;
	int ix, cur;
	char let;
#define GOLD_INDX   0
#define GOLD_OFFSET 1
#define OVRFLW_INDX (GOLD_OFFSET + 52) /* past gold and 2*26 letters */
#define ALPHABET_SIZE 1 + 52 + 1 + 1
	char alphabet[ALPHABET_SIZE], buf[ALPHABET_SIZE]; /* room for '$a-zA-Z#\0' */
	char qbuf[QBUFSZ];
	char *objname, *otmpname;
	char allowallcnt[3];
	struct obj *splitting;
	const char *adj_type;

	/* when no invent, or just gold in '$' slot, there's nothing to adjust */
	if (!invent ||
		(invent->oclass == COIN_CLASS && invent->invlet == GOLD_SYM && !invent->nobj)) {
		You("aren't carrying anything %s.", !invent ? "to adjust" : "adjustable");
		return 0;
	}

	if (!flags.invlet_constant) {
		reassign();
	}
	/* get a pointer to the object the user wants to organize */
	allowallcnt[0] = ALLOW_COUNT;
	allowallcnt[1] = ALL_CLASSES;
	allowallcnt[2] = '\0';
	if (!(obj = getobj(allowallcnt, "adjust"))) {
		return 0;
	}
	/* figure out whether user gave a split count to getobj() */
	splitting = 0;
	for (otmp = invent; otmp; otmp = otmp->nobj) {
		if (otmp->nobj == obj) { /* knowledge of splitobj() operation */
			if (otmp->invlet == obj->invlet)
				splitting = otmp;
			break;
		}
	}

	/* initialize the list with all upper and lower case letters */
	alphabet[GOLD_INDX] = (obj->oclass == COIN_CLASS) ? GOLD_SYM : ' ';
	for (ix = GOLD_OFFSET, let = 'a'; let <= 'z';) {
		alphabet[ix++] = let++;
	}
	for (let = 'A'; let <= 'Z';) {
		alphabet[ix++] = let++;
	}
	alphabet[OVRFLW_INDX] = ' ';
	alphabet[sizeof alphabet - 1] = '\0';

	/* blank out all the letters currently in use in the inventory */
	/* except those that will be merged with the selected object   */
	for (otmp = invent; otmp; otmp = otmp->nobj) {
		if (otmp != obj && !mergable(otmp, obj)) {
			let = otmp->invlet;
			if (let >= 'a' && let <= 'z') {
				alphabet[GOLD_OFFSET + let - 'a'] = ' ';
			} else if (let >= 'A' && let <= 'Z') {
				alphabet[GOLD_OFFSET + let - 'A' + 26] = ' ';
			/* overflow defaults to off, but it we find a stack using that
			   slot, switch to on -- the opposite of normal invlet handling */
			} else if (let == NOINVSYM) {
				alphabet[OVRFLW_INDX] = NOINVSYM;
			}
		}
	}

	/* compact the list by removing all the blanks */
	for (ix = cur = 0; ix <= sizeof alphabet-1; ix++) {
		if (alphabet[ix] != ' ') {
			buf[cur++] = alphabet[ix];
		}
	}
	buf[cur] = '\0';

	/* and by dashing runs of letters */
	if (cur > 5) {
		compactify(buf);
	}

	/* get new letter to use as inventory letter */
	for (;;) {
		Sprintf(qbuf, "Adjust letter to what [%s]?", buf);
		let = yn_function(qbuf, (char *)0, '\0');
		if (index(quitchars, let)) {
			pline("%s", Never_mind);
			goto cleansplit;
		}
		if (let == '@' || !letter(let)) {
			pline("Select an inventory slot letter.");
		} else {
			break;
		}
	}

	/* change the inventory and print the resulting item */

	/*
	 * don't use freeinv/addinv to avoid double-touching artifacts,
	 * dousing lamps, losing luck, cursing loadstone, etc.
	 */
	extract_nobj(obj, &invent);

	for (otmp = invent; otmp && otmp->invlet != let;) {
		otmp = otmp->nobj;
	}
	/* Collecting: #adjust an inventory stack into its same slot;
	   keep it there and merge other compatible stacks into it.
	   Traditional inventory behavior is to merge unnamed stacks
	   with compatible named ones; we only want that if it is
	   the 'from' stack (obj) with a name and candidate (otmp)
	   without one, not unnamed 'from' with named candidate. */
	if (let == obj->invlet) {
		adj_type = "Collecting:";
		for (otmp = invent; otmp;) {
			/* it's tempting to pull this outside the loop, but merged() could
			   free ONAME(obj) [via obfree()] and replace it with ONAME(otmp) */
			objname = get_ox(obj, OX_ENAM) ? ONAME(obj) : (char *) 0;
			otmpname = get_ox(otmp, OX_ENAM) ? ONAME(otmp) : (char *) 0;
			if ((!otmpname || (objname && !strcmp(objname, otmpname)))
			    && merged(&otmp, &obj)) {
				obj = otmp;
				otmp = otmp->nobj;
				extract_nobj(obj, &invent);
				continue; /* otmp has already been updated */
			}
			otmp = otmp->nobj;
		}
	} else if (!otmp) {
		adj_type = splitting ? "Splitting:" : "Moving:";
	} else if (merged(&otmp, &obj)) {
		adj_type = "Merging:";
		obj = otmp;
		extract_nobj(obj, &invent);
	} else {
		struct obj *otmp2;
		for (otmp2 = invent; otmp2
			 && otmp2->invlet != obj->invlet;) {
			otmp2 = otmp2->nobj;
		}
		if (otmp2) {
			char oldlet = obj->invlet;

			adj_type = "Displacing:";

			/* Here be a nasty hack; solutions that don't
			 * require duplication of assigninvlet's code
			 * here are welcome.
			 */
			assigninvlet(obj);

			if (obj->invlet == NOINVSYM) {
				pline("There's nowhere to put that.");
				obj->invlet = oldlet;
				goto cleansplit;
			}
		} else {
			adj_type = "Swapping:";
		}
		otmp->invlet = obj->invlet;
	}

	/* inline addinv (assuming flags.invlet_constant and !merged) */
	obj->invlet = let;
	obj->nobj = invent; /* insert at beginning */
	obj->where = OBJ_INVENT;
	invent = obj;
	reorder_invent();

	prinv(adj_type, obj, 0L);
	update_inventory();
	return 0;
cleansplit:
	for (otmp = invent; otmp; otmp = otmp->nobj) {
		if (otmp != obj && otmp->invlet == obj->invlet) {
			merged(&otmp, &obj);
			break;
		}
	}

	return 0;
}

/* common to display_minventory and display_cinventory */
STATIC_OVL void
invdisp_nothing(hdr, txt)
const char *hdr, *txt;
{
	winid win;
	anything any;
	menu_item *selected;

	any.a_void = 0;
	win = create_nhwindow(NHW_MENU);
	start_menu(win);
	add_menu(win, NO_GLYPH, &any, 0, 0, iflags.menu_headings, hdr, MENU_UNSELECTED);
	add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
	add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, txt, MENU_UNSELECTED);
	end_menu(win, (char *)0);
	if (select_menu(win, PICK_NONE, &selected) > 0)
	    free((genericptr_t)selected);
	destroy_nhwindow(win);
	return;
}

/* query_objlist callback: return things that could possibly be worn/wielded */
STATIC_OVL boolean
worn_wield_only(struct obj *obj, int qflags)
{
    return (obj->oclass == WEAPON_CLASS
		|| obj->oclass == ARMOR_CLASS
		|| obj->oclass == AMULET_CLASS
		|| obj->oclass == RING_CLASS
		|| obj->oclass == TOOL_CLASS);
}

/*
 * Display a monster's inventory.
 * Returns a pointer to the object from the monster's inventory selected
 * or NULL if nothing was selected.
 *
 * By default, only worn and wielded items are displayed.  The caller
 * can pick one.  Modifier flags are:
 *
 *	MINV_NOLET	- nothing selectable
 *	MINV_ALL	- display all inventory
 */
struct obj *
display_minventory(mon, dflags, title)
register struct monst *mon;
int dflags;
char *title;
{
	struct obj *ret;
#ifndef GOLDOBJ
	struct obj m_gold;
#endif
	char tmp[QBUFSZ];
	int n;
	menu_item *selected = 0;
#ifndef GOLDOBJ
	int do_all = (dflags & MINV_ALL) != 0,
	    do_gold = (do_all && mon->mgold);
#else
	int do_all = (dflags & MINV_ALL) != 0;
#endif

	Sprintf(tmp,"%s %s:", s_suffix(noit_Monnam(mon)),
		do_all ? "possessions" : "armament");

#ifndef GOLDOBJ
	if (do_all ? (mon->minvent || mon->mgold)
#else
	if (do_all ? (mon->minvent != 0)
#endif
		   : (mon->misc_worn_check || MON_WEP(mon) || MON_SWEP(mon))) {
	    /* Fool the 'weapon in hand' routine into
	     * displaying 'weapon in claw', etc. properly.
		 * Set back by set_uasmon() bellow
	     */
	    youmonst.data = mon->data;

#ifndef GOLDOBJ
	    if (do_gold) {
		/*
		 * Make temporary gold object and insert at the head of
		 * the mon's inventory.  We can get away with using a
		 * stack variable object because monsters don't carry
		 * gold in their inventory, so it won't merge.
		 */
		m_gold = zeroobj;
		m_gold.otyp = GOLD_PIECE;  m_gold.oclass = COIN_CLASS;
		m_gold.quan = mon->mgold;  m_gold.dknown = 1;
		m_gold.where = OBJ_FREE;
		/* we had better not merge and free this object... */
		if (add_to_minv(mon, &m_gold))
		    panic("display_minventory: static object freed.");
	    }

#endif
	    n = query_objlist(title ? title : tmp, mon->minvent, INVORDER_SORT, &selected,
			(dflags & MINV_NOLET) ? PICK_NONE : PICK_ONE,
			do_all ? allow_all : worn_wield_only);

#ifndef GOLDOBJ
	    if (do_gold) obj_extract_self(&m_gold);
#endif

	    set_uasmon();
	} else {
	    invdisp_nothing(title ? title : tmp, "(none)");
	    n = 0;
	}

	if (n > 0) {
	    ret = selected[0].item.a_obj;
	    free((genericptr_t)selected);
#ifndef GOLDOBJ
	    /*
	     * Unfortunately, we can't return a pointer to our temporary
	     * gold object.  We'll have to work out a scheme where this
	     * can happen.  Maybe even put gold in the inventory list...
	     */
	    if (ret == &m_gold) ret = (struct obj *) 0;
#endif
	} else
	    ret = (struct obj *) 0;
	return ret;
}

/*
 * Display the contents of a container in inventory style.
 * Currently, this is only used for statues, via wand of probing.
 */
struct obj *
display_cinventory(obj)
register struct obj *obj;
{
	struct obj *ret;
	char tmp[QBUFSZ];
	int n;
	menu_item *selected = 0;

	Sprintf(tmp,"Contents of %s:", doname(obj));

	if (obj->cobj) {
	    n = query_objlist(tmp, obj->cobj, INVORDER_SORT, &selected,
			    PICK_NONE, allow_all);
	} else {
	    invdisp_nothing(tmp, "(empty)");
	    n = 0;
	}
	if (n > 0) {
	    ret = selected[0].item.a_obj;
	    free((genericptr_t)selected);
	} else
	    ret = (struct obj *) 0;
	return ret;
}

/* query objlist callback: return TRUE if obj is at given location */
static coord only;

STATIC_OVL boolean
only_here(struct obj *obj, int qflags)
{
	if(qflags&NO_EQUIPMENT && obj->owornmask)
		return FALSE;
    return (obj->ox == only.x && obj->oy == only.y);
}

/*
 * Display a list of buried items in inventory style.  Return a non-zero
 * value if there were items at that spot.
 *
 * Currently, this is only used with a wand of probing zapped downwards.
 */
int
display_binventory(x, y, as_if_seen)
int x, y;
boolean as_if_seen;
{
	struct obj *obj;
	menu_item *selected = 0;
	int n;

	/* count # of objects here */
	for (n = 0, obj = level.buriedobjlist; obj; obj = obj->nobj)
	    if (obj->ox == x && obj->oy == y) {
		if (as_if_seen) obj->dknown = 1;
		n++;
	    }

	if (n) {
	    only.x = x;
	    only.y = y;
	    if (query_objlist("Things that are buried here:",
			      level.buriedobjlist, INVORDER_SORT,
			      &selected, PICK_NONE, only_here) > 0)
		free((genericptr_t)selected);
	    only.x = only.y = 0;
	}
	return n;
}

int
mon_healing_penalty(mon)
struct monst *mon;
{
	int penalty = 0;
	if(hates_silver(mon->data)){
		penalty += (20*mon_material_next_to_skin(mon, SILVER))/2;
	}
	if(hates_iron(mon->data)){
		penalty += (mon->m_lev * mon_material_next_to_skin(mon, IRON)+1)/2;
		penalty += (mon->m_lev * mon_material_next_to_skin(mon, GREEN_STEEL)+1)/2;
	}
	if(hates_unholy_mon(mon)){
		penalty += (9*mon_bcu_next_to_skin(mon, -1)+1)/2;
		penalty += 9*mon_material_next_to_skin(mon, GREEN_STEEL);
	}
	if(hates_unblessed_mon(mon)){
		penalty += (8*mon_bcu_next_to_skin(mon, 0)+1)/2;
	}
	if(hates_holy_mon(mon)){
		penalty += (4*mon_bcu_next_to_skin(mon, 1)+1)/2;
	}
	if(is_chaotic_mon(mon)){
		int plat_penalty = 5 * mon_material_next_to_skin(mon, PLATINUM);
		if(is_minion(mon->data) || is_demon(mon->data))
			plat_penalty *= 2;
		/* strongly chaotic beings are hurt more */
		if(mon->mtyp == PM_ANGEL || mon->data->maligntyp <= -10)
			plat_penalty *= 2;

		penalty += plat_penalty/2;
	}

	return penalty;
}

int
u_healing_penalty()
{
	int penalty = 0;
	if(hates_silver(youracedata)){
		penalty += (20*u_material_next_to_skin(SILVER)+1)/2;
	}
	if(hates_iron(youracedata)){
		penalty += (u.ulevel * u_material_next_to_skin(IRON)+1)/2;
		penalty += (u.ulevel * u_material_next_to_skin(GREEN_STEEL)+1)/2;
	}
	if(hates_unholy(youracedata)){
		penalty += (9*u_bcu_next_to_skin(-1)+1)/2;
		penalty += 9*u_material_next_to_skin(GREEN_STEEL);
	}
	if(hates_unblessed(youracedata)){
		penalty += (8*u_bcu_next_to_skin(0)+1)/2;
	}
	if(hates_holy(youracedata)){
		penalty += (4*u_bcu_next_to_skin(1)+1)/2;
	}
	if(u.ualign.type == A_CHAOTIC){
		int plat_penalty = 5 * u_material_next_to_skin(PLATINUM);
		if(is_minion(youracedata) || is_demon(youracedata))
			plat_penalty *= 2;
		/* strongly chaotic beings are hurt more */
		if(u.ualign.record >= 100)
			plat_penalty *= 2;

		penalty += plat_penalty/2;
	}
	if(u.umadness&MAD_NUDIST && !BlockableClearThoughts && NightmareAware_Sanity < 100){
		int delta = NightmareAware_Insanity;
		penalty += (u_clothing_discomfort() * delta)/10;
	}
	return penalty;
}

int
u_clothing_discomfort()
{
	int count = 0;
	if(uarmh){
		count++;
		if(uarmh->otyp ==  find_vhelm())
			count++;
		if(FacelessHelm(uarmh))
			count+=2;
	}
	if(uarm){
		count++;
		if(is_light_armor(uarm));//+0
		else if(is_medium_armor(uarm))
			count++;
		else
			count += 2;//Not medium or light, so heavy
	}
	else if(uwep && uwep->oartifact == ART_TENSA_ZANGETSU){
		count++;
	}
	if(uarmu){
		count++;
		if(uarmu->otyp == PLAIN_DRESS)
			count++;
		else if(uarmu->otyp == BODYGLOVE || uarmu->otyp == VICTORIAN_UNDERWEAR)
			count += 2;
	}
	if(uarmg){
		count++;
		if(uarmg->otyp == LONG_GLOVES)
			count++;
	}
	if(uarmf){
		count++;
		if(uarmf->otyp == HEELED_BOOTS)
			count += 2;
		else if(uarmf->otyp != LOW_BOOTS && uarmf->otyp == SHOES)
			count++;
	}
	if(uarmc){
		count+=3;
		if(uarmc->otyp == MUMMY_WRAPPING
			|| uarmc->otyp == PRAYER_WARDED_WRAPPING
			|| uarmc->otyp == WHITE_FACELESS_ROBE
			|| uarmc->otyp == BLACK_FACELESS_ROBE
			|| uarmc->otyp == SMOKY_VIOLET_FACELESS_ROBE
		)
			count += 3;
	}
	if(uamul){
		count++;
	}
	if(ubelt){
		count++;
	}
	if(uleft) count++;
	if(uright) count++;
	if(ublindf){
		count++;
		if(ublindf->otyp == BLINDFOLD
		|| ublindf->otyp == ANDROID_VISOR
		|| ublindf->otyp == TOWEL
		|| ublindf->otyp == MASK
		|| ublindf->otyp == LIVING_MASK
		)
			count++;
	}
	return count; //0-25
}

STATIC_OVL int
mon_material_next_to_skin(mon, material)
struct monst *mon;
int material;
{
	int count = 0;
	struct obj *curarm;
	boolean marm_blocks_ub = FALSE;
	boolean hasgloves = !!which_armor(mon, W_ARMG);
	boolean hasshirt = !!which_armor(mon, W_ARMU);
	boolean hasarm = !!which_armor(mon, W_ARMU);

	curarm = which_armor(mon, W_ARMU);
	if(curarm && curarm->obj_material == material)
		count += curarm->otyp == BODYGLOVE ? 5 : 1;
	if(curarm && curarm->otyp == BODYGLOVE)
		return count;

	if(MON_WEP(mon) && MON_WEP(mon)->obj_material == material && !hasgloves)
		count++;

	curarm = which_armor(mon, W_ARM);
	if(curarm && curarm->obj_material == material && !hasshirt)
		count++;
	if(curarm && arm_blocks_upper_body(curarm->otyp))
		marm_blocks_ub = TRUE;
	
	curarm = which_armor(mon, W_ARMC);
	if(curarm && curarm->obj_material == material && !hasshirt && !marm_blocks_ub)
		count++;

	curarm = which_armor(mon, W_ARMH);
	if(curarm && curarm->obj_material == material)
		count++;

	curarm = which_armor(mon, W_ARMS);
	if(curarm && curarm->obj_material == material && !hasgloves)
		count++;

	curarm = which_armor(mon, W_ARMG);
	if(curarm && curarm->obj_material == material)
		count++;

	curarm = which_armor(mon, W_ARMF);
	if(curarm && curarm->obj_material == material)
		count++;

	curarm = which_armor(mon, W_AMUL);
	if(curarm && curarm->obj_material == material && !hasshirt && !marm_blocks_ub)
		count++;

	curarm = which_armor(mon, W_BELT);
	if(curarm && curarm->obj_material == material && !hasshirt && !hasarm)
		count++;

	if(mon->entangled_oid && !hasshirt && !marm_blocks_ub && !which_armor(mon, W_ARMC) && entangle_material(mon, material))
		count++;

	curarm = which_armor(mon, W_TOOL);
	if(curarm && curarm->obj_material == material)
		count++;

	if(MON_SWEP(mon) && MON_SWEP(mon)->obj_material == material && !hasgloves)
		count++;
	return count;
}

STATIC_OVL int
mon_bcu_next_to_skin(mon, bcu)
struct monst *mon;
int bcu;
{
	#define bcu(otmp) (is_unholy(otmp) ? -1 : otmp->blessed ? 1 : 0)
	int count = 0;
	struct obj *curarm;
	boolean marm_blocks_ub = FALSE;
	boolean hasgloves = !!which_armor(mon, W_ARMG);
	boolean hasshirt = !!which_armor(mon, W_ARMU);
	boolean hasarm = !!which_armor(mon, W_ARM);

	curarm = which_armor(mon, W_ARMU);
	if(curarm && bcu(curarm) == bcu)
		count += curarm->otyp == BODYGLOVE ? 5 : 1;
	if(curarm && curarm->otyp == BODYGLOVE)
		return count;

	if(MON_WEP(mon) && bcu(MON_WEP(mon)) == bcu && !hasgloves)
		count++;

	curarm = which_armor(mon, W_ARM);
	if(curarm && bcu(curarm) == bcu && !hasshirt)
		count++;
	if(curarm && arm_blocks_upper_body(curarm->otyp))
		marm_blocks_ub = TRUE;

	curarm = which_armor(mon, W_ARMC);
	if(curarm && bcu(curarm) == bcu && !hasshirt && !marm_blocks_ub)
		count++;

	curarm = which_armor(mon, W_ARMH);
	if(curarm && bcu(curarm) == bcu)
		count++;

	curarm = which_armor(mon, W_ARMS);
	if(curarm && bcu(curarm) == bcu && !hasgloves)
		count++;

	curarm = which_armor(mon, W_ARMG);
	if(curarm && bcu(curarm) == bcu)
		count++;

	curarm = which_armor(mon, W_ARMF);
	if(curarm && bcu(curarm) == bcu)
		count++;

	curarm = which_armor(mon, W_AMUL);
	if(curarm && bcu(curarm) == bcu && !hasshirt && !marm_blocks_ub)
		count++;

	curarm = which_armor(mon, W_BELT);
	if(curarm && bcu(curarm) == bcu && !hasshirt && !hasarm)
		count++;

	if(mon->entangled_oid && !hasshirt && !marm_blocks_ub && !which_armor(mon, W_ARMC) && entangle_beatitude(mon, bcu))
		count++;

	curarm = which_armor(mon, W_TOOL);
	if(curarm && bcu(curarm) == bcu)
		count++;

	if(MON_SWEP(mon) && bcu(MON_SWEP(mon)) == bcu && !hasgloves)
		count++;
	return count;
}

STATIC_OVL int
u_material_next_to_skin(material)
int material;
{
	int count = 0;
	if(u.sealsActive&SEAL_EDEN)
		return 0;
	if(uarmu && uarmu->obj_material == material)
		count += uarmu->otyp == BODYGLOVE ? 5 : 1;
	if(uarmu && uarmu->otyp == BODYGLOVE)
		return count;
	if(uwep && uwep->obj_material == material && !uarmg)
		count++;
	if(uarm && uarm->obj_material == material && !uarmu)
		count++;
	if(uarmc && uarmc->obj_material == material && !uarmu && !(uarm && arm_blocks_upper_body(uarm->otyp)))
		count++;
	if(uarmh && uarmh->obj_material == material)
		count++;
	if(uarms && uarms->obj_material == material && !uarmg)
		count++;
	if(uarmg && uarmg->obj_material == material)
		count++;
	if(uarmf && uarmf->obj_material == material)
		count++;
	if(uleft && uleft->obj_material == material)
		count++;
	if(uright && uright->obj_material == material)
		count++;
	if(uamul && uamul->obj_material == material && !uarmu && !(uarm && arm_blocks_upper_body(uarm->otyp)))
		count++;
	if(ubelt && ubelt->obj_material == material && !uarmu && !uarm)
		count++;
	if(u.uentangled_oid && !uarmu && !uarm && !(uarm && arm_blocks_upper_body(uarm->otyp)) && entangle_material(&youmonst, material))
		count++;
	if(ublindf && ublindf->obj_material == material)
		count++;
	if(uchain && uchain->obj_material == material)
		count++;
	if(uswapwep && uswapwep->obj_material == material && u.twoweap && !uarmg)
		count++;
	return count;
}

STATIC_OVL int
u_bcu_next_to_skin(bcu)
int bcu;
{
	#define bcu(otmp) (is_unholy(otmp) ? -1 : otmp->blessed ? 1 : 0)
	int count = 0;
	if(u.sealsActive&SEAL_EDEN)
		return 0;
	if(uarmu && bcu(uarmu) == bcu)
		count += uarmu->otyp == BODYGLOVE ? 5 : 1;
	if(uarmu && uarmu->otyp == BODYGLOVE)
		return count;
	if(uwep && bcu(uwep) == bcu && !uarmg)
		count++;
	if(uarm && bcu(uarm) == bcu && !uarmu)
		count++;
	if(uarmc && bcu(uarmc) == bcu && !uarmu && !(uarm && arm_blocks_upper_body(uarm->otyp)))
		count++;
	if(uarmh && bcu(uarmh) == bcu)
		count++;
	if(uarms && bcu(uarms) == bcu && !uarmg)
		count++;
	if(uarmg && bcu(uarmg) == bcu)
		count++;
	if(uarmf && bcu(uarmf) == bcu)
		count++;
	if(uleft && bcu(uleft) == bcu)
		count++;
	if(uright && bcu(uright) == bcu)
		count++;
	if(uamul && bcu(uamul) == bcu && !uarmu && !(uarm && arm_blocks_upper_body(uarm->otyp)))
		count++;
	if(ubelt && bcu(ubelt) == bcu && !uarmu && !uarm)
		count++;
	if(u.uentangled_oid && !uarmu && !(uarm && arm_blocks_upper_body(uarm->otyp)) && !uarmc && entangle_beatitude(&youmonst, bcu))
		count++;
	if(ublindf && bcu(ublindf) == bcu)
		count++;
	if(uchain && bcu(uchain) == bcu)
		count++;
	if(uswapwep && bcu(uswapwep) == bcu && u.twoweap && !uarmg)
		count++;
	return count;
}

struct obj *
outermost_armor(mon)
struct monst *mon;
{
	if(mon == &youmonst){
		if(uarmc) return uarmc;
		if(uarm && !(!arm_blocks_upper_body(uarm->otyp) && uarmu && rn2(2))) return uarm;
		if(uarmu) return uarmu;
	} else {
		if(which_armor(mon, W_ARMC)) return which_armor(mon, W_ARMC);
		if(which_armor(mon, W_ARM) && !(!arm_blocks_upper_body(which_armor(mon, W_ARM)->otyp) && which_armor(mon, W_ARMU) && rn2(2))) return which_armor(mon, W_ARM);
		if(which_armor(mon, W_ARMU)) return which_armor(mon, W_ARMU);
	}
	return (struct obj *) 0;
}

struct obj *
get_most_complete_puzzle()
{
	struct obj *puzzle = 0, *otmp;
	for (otmp = invent; otmp; otmp = otmp->nobj){
		if(otmp->otyp == HYPERBOREAN_DIAL && otmp->ovar1_puzzle_steps < 6 && otmp->ovar1_puzzle_steps <= u.uhyperborean_steps && (!puzzle || puzzle->ovar1_puzzle_steps < otmp->ovar1_puzzle_steps))
			puzzle = otmp;
	}
	return puzzle;
}

#endif /* OVL1 */

/*invent.c*/
