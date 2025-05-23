/*	SCCS Id: @(#)wield.c	3.4	2003/01/29	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"

/* KMH -- Differences between the three weapon slots.
 *
 * The main weapon (uwep):
 * 1.  Is filled by the (w)ield command.
 * 2.  Can be filled with any type of item.
 * 3.  May be carried in one or both hands.
 * 4.  Is used as the melee weapon and as the launcher for
 *     ammunition.
 * 5.  Only conveys intrinsics when it is a weapon, weapon-tool,
 *     or artifact.
 * 6.  Certain cursed items will weld to the hand and cannot be
 *     unwielded or dropped.  See erodeable_wep() and will_weld()
 *     below for the list of which items apply.
 *
 * The secondary weapon (uswapwep):
 * 1.  Is filled by the e(x)change command, which swaps this slot
 *     with the main weapon.  If the "pushweapon" option is set,
 *     the (w)ield command will also store the old weapon in the
 *     secondary slot.
 * 2.  Can be field with anything that will fit in the main weapon
 *     slot; that is, any type of item.
 * 3.  Is usually NOT considered to be carried in the hands.
 *     That would force too many checks among the main weapon,
 *     second weapon, shield, gloves, and rings; and it would
 *     further be complicated by bimanual weapons.  A special
 *     exception is made for two-weapon combat.
 * 4.  Is used as the second weapon for two-weapon combat, and as
 *     a convenience to swap with the main weapon.
 * 5.  Never conveys intrinsics.
 * 6.  Cursed items never weld (see #3 for reasons), but they also
 *     prevent two-weapon combat.
 *
 * The quiver (uquiver):
 * 1.  Is filled by the (Q)uiver command.
 * 2.  Can be filled with any type of item.
 * 3.  Is considered to be carried in a special part of the pack.
 * 4.  Is used as the item to throw with the (f)ire command.
 *     This is a convenience over the normal (t)hrow command.
 * 5.  Never conveys intrinsics.
 * 6.  Cursed items never weld; their effect is handled by the normal
 *     throwing code.
 *
 * No item may be in more than one of these slots.
 */


STATIC_DCL int FDECL(ready_weapon, (struct obj *, boolean));

/* used by will_weld() */
/* probably should be renamed */
#define erodeable_wep(optr)	((optr)->oclass == WEAPON_CLASS \
				|| is_weptool(optr) \
				|| (optr)->otyp == BALL \
				|| (optr)->otyp == CHAIN)

/* used by welded(), and also while wielding */
#define will_weld(optr)		((optr)->cursed \
				&& !Weldproof\
				&& (erodeable_wep(optr) \
				   || (optr)->otyp == TIN_OPENER))


/*** Functions that place a given item in a slot ***/
/* Proper usage includes:
 * 1.  Initializing the slot during character generation or a
 *     restore.
 * 2.  Setting the slot due to a player's actions.
 * 3.  If one of the objects in the slot are split off, these
 *     functions can be used to put the remainder back in the slot.
 * 4.  Putting an item that was thrown and returned back into the slot.
 * 5.  Emptying the slot, by passing a null object.  NEVER pass
 *     zeroobj!
 *
 * If the item is being moved from another slot, it is the caller's
 * responsibility to handle that.  It's also the caller's responsibility
 * to print the appropriate messages.
 */
void
setuwep(obj)
register struct obj *obj;
{
	struct obj *olduwep = uwep;

	if (obj == uwep) return; /* necessary to not set unweapon */
	/* This message isn't printed in the caller because it happens
	 * *whenever* Sunsword is unwielded, from whatever cause.
	 */
	setworn(obj, W_WEP);
	if (uwep == obj && artifact_light(olduwep) && olduwep->lamplit) {
	    end_burn(olduwep, FALSE);
	    if (!Blind) pline("%s glowing.", Tobjnam(olduwep, "stop"));
	}
	/* Note: Explicitly wielding a pick-axe will not give a "bashing"
	 * message.  Wielding one via 'a'pplying it will.
	 * 3.2.2:  Wielding arbitrary objects will give bashing message too.
	 */
	if (obj) {
		unweapon = (obj->oclass == WEAPON_CLASS) ?
				(is_launcher(obj) || is_ammo(obj) ||
				 is_missile(obj) ||
				 (is_bad_melee_pole(obj)
#ifdef STEED
				&& !u.usteed
#endif
				&& !melee_polearms(youracedata)
				&& obj->otyp != AKLYS
				&& !is_cclub_able(obj)
				)) :
				!(is_weptool(obj) || obj->otyp == WIND_AND_FIRE_WHEELS);
	} else
		unweapon = TRUE;	/* for "bare hands" message */
        if (!restoring) {
            update_inventory();
        }
}

STATIC_OVL int
ready_weapon(wep, quietly)
struct obj *wep;
boolean quietly;	/* hide the basic message saying what you are now wielding */
{
	/* Separated function so swapping works easily */
	int res = 0;
	boolean stealthy = Stealth;
	
	
	if (!wep) {
	    /* No weapon */
		if (uwep) {
			if (!quietly) You("are empty %s.", body_part(HANDED));
			setuwep((struct obj *) 0);
			if (!stealthy && Stealth) pline("Now you can move stealthily.");
			res++;
		}
		else
			if (!quietly) You("are already empty %s.", body_part(HANDED));
	} else if (!uarmg && !Stone_resistance && wep->otyp == CORPSE
				&& touch_petrifies(&mons[wep->corpsenm])) {
	    /* Prevent wielding cockatrice when not wearing gloves --KAA */
	    char kbuf[BUFSZ];

	    You("wield the %s corpse in your bare %s.",
		mons[wep->corpsenm].mname, makeplural(body_part(HAND)));
	    Sprintf(kbuf, "%s corpse", an(mons[wep->corpsenm].mname));
	    instapetrify(kbuf);
	} else if (uarms && bimanual(wep,youracedata)){
		char* term = is_sword(wep) ? "sword" : wep->otyp == BATTLE_AXE ? "axe" : "weapon";
	    You("cannot wield %s in both hands while wearing a shield.", an(term));
	} else if (wep->otyp == ARM_BLASTER && uarmg && is_metallic(uarmg))
		You("cannot fit the bracer over such bulky, rigid gloves.");
	else if (wep->oartifact == ART_KUSANAGI_NO_TSURUGI && !(u.ulevel >= 30 || u.uhave.amulet)) {
	    pline("Only a Shogun, or a bearer of the Amulet of Yendor, is truly worthy of wielding this sword.");
	    res++;	/* takes a turn even though it doesn't get wielded */
	} else if (wep->oartifact && !touch_artifact(wep, &youmonst, FALSE)) {
	    res++;	/* takes a turn even though it doesn't get wielded */
	} else {
	    /* Weapon WILL be wielded after this point */
	    res++;
	    if (will_weld(wep)) {
		const char *tmp = xname(wep), *thestr = "The ";
		if (strncmp(tmp, thestr, 4) && !strncmp(The(tmp),thestr,4))
		    tmp = thestr;
		else tmp = "";
		pline("%s%s %s to your %s!", tmp, aobjnam(wep, "weld"),
			(wep->quan == 1L) ? "itself" : "themselves", /* a3 */
			bimanual(wep,youracedata) ?
				(const char *)makeplural(body_part(HAND))
				: body_part(HAND));
		wep->bknown = TRUE;
	    } else {
		/* The message must be printed before setuwep (since
		 * you might die and be revived from changing weapons),
		 * and the message must be before the death message and
		 * Lifesaved rewielding.  Yet we want the message to
		 * say "weapon in hand", thus this kludge.
		 */
			if (!quietly) {
				long dummy = wep->owornmask;
				wep->owornmask |= W_WEP;
				prinv((char *)0, wep, 0L);
				wep->owornmask = dummy;
			}
	    }
	    setuwep(wep);

	    /* KMH -- Talking artifacts are finally implemented */
		if (!quietly) arti_speak(wep);

	    if (artifact_light(wep) && !wep->lamplit) {
			begin_burn(wep);
		if (!Blind)
			pline("%s to %s%s!", Tobjnam(wep, "begin"),
				(wep->blessed ? "shine very" : "glow"), (wep->cursed ? "" : " brilliantly"));
		}
		if(stealthy && wep->otyp == KHAKKHARA) (wep->oartifact == ART_ANNULUS) ? 
			pline("The hollow silver rod chimes at the slightest touch.") : 
			(wep->oartifact == ART_STAFF_OF_TWELVE_MIRRORS) ? 
			pline("The mirrored disks chime together.") :
			pline("The silver rings chime together.");
		else if(!stealthy && Stealth) pline("Now you can move stealthily.");

#if 0
	    /* we'll get back to this someday, but it's not balanced yet */
	    if (Race_if(PM_ELF) && !wep->oartifact &&
			    is_iron_obj(wep)) {
		/* Elves are averse to wielding cold iron */
		You("have an uneasy feeling about wielding cold iron.");
		change_luck(-1);
	    }
#endif

	    if (wep->unpaid && !quietly) {
		struct monst *this_shkp;

		if ((this_shkp = shop_keeper(inside_shop(u.ux, u.uy))) !=
		    (struct monst *)0) {
		    pline("%s says \"You be careful with my %s!\"",
			  shkname(this_shkp),
			  xname(wep));
		}
	    }
	}
	return (res) ? MOVE_STANDARD : MOVE_CANCELLED;
}

void
setuqwep(obj)
register struct obj *obj;
{
	setworn(obj, W_QUIVER);
	update_inventory();
}

void
setuswapwep(obj)
register struct obj *obj;
{
	setworn(obj, W_SWAPWEP);
	update_inventory();
}


/*** Commands to change particular slot(s) ***/

static NEARDATA const char wield_objs[] =
	{ ALL_CLASSES, ALLOW_NONE, WEAPON_CLASS, TOOL_CLASS, 0 };
static NEARDATA const char ready_objs[] =
	{ ALL_CLASSES, ALLOW_NONE, WEAPON_CLASS, 0 };
static NEARDATA const char bullets[] =	/* (note: different from dothrow.c) */
	{ ALL_CLASSES, ALLOW_NONE, GEM_CLASS, WEAPON_CLASS, 0 };

int
dowield()
{
	register struct obj *wep, *oldwep;
	int result;

	/* May we attempt this? */
	multi = 0;
	if (you_cantwield(youracedata)) {
		if(!nohands(youracedata))
			Your("%s aren't dexterous enough to properly wield weapons!", makeplural(body_part(HAND)));
		else
			pline("Don't be ridiculous!");
		return MOVE_CANCELLED;
	}
	
	if (!freehand()) {
		You("have no free %s to wield anything!", body_part(HAND));
		return MOVE_CANCELLED;
	}

	/* Prompt for a new weapon */
	if (!(wep = getobj(wield_objs, "wield")))
		/* Cancelled */
		return MOVE_CANCELLED;
	else if (wep == uwep) {
	    You("are already wielding that!");
	    if (is_weptool(wep)) unweapon = FALSE;	/* [see setuwep()] */
		return MOVE_CANCELLED;
	} else if (welded(uwep)) {
		weldmsg(uwep);
		/* previously interrupted armor removal mustn't be resumed */
		reset_remarm();
		return MOVE_INSTANT;
	}

	/* Handle no object, or object in other slot */
	if (wep == &zeroobj)
		wep = (struct obj *) 0;
	else if (wep == uswapwep){
		if(wep->ostolen && u.sealsActive&SEAL_ANDROMALIUS) unbind(SEAL_ANDROMALIUS, TRUE);
		return (doswapweapon());
	} else if (wep == uquiver){
		if(wep->ostolen && u.sealsActive&SEAL_ANDROMALIUS) unbind(SEAL_ANDROMALIUS, TRUE);
		setuqwep((struct obj *) 0);
	} else if (wep->owornmask & (W_ARMOR | W_RING | W_AMUL | W_BELT | W_TOOL
#ifdef STEED
			| W_SADDLE
#endif
			)) {
		You("cannot wield that!");
		return MOVE_CANCELLED;
	}

	/* Set your new primary weapon */
	oldwep = uwep;
	result = ready_weapon(wep, FALSE);
	if (flags.pushweapon && oldwep && uwep != oldwep)
		setuswapwep(oldwep);
	
	if (u.twoweap && !test_twoweapon())
		untwoweapon();

	if(uwep && uwep->ostolen && u.sealsActive&SEAL_ANDROMALIUS) unbind(SEAL_ANDROMALIUS, TRUE);
	return (result);
}

int
doswapweapon()
{
	register struct obj *oldwep, *oldswap;
	int result = MOVE_INSTANT;


	/* May we attempt this? */
	multi = 0;
	if (you_cantwield(youracedata)) {
		if(!nohands(youracedata))
			Your("%s aren't dexterous enough to properly wield weapons!", makeplural(body_part(HAND)));
		else
			pline("Don't be ridiculous!");
		return MOVE_CANCELLED;
	}
	if (welded(uwep)) {
		weldmsg(uwep);
		return MOVE_INSTANT;
	}

	/* Unwield your current secondary weapon */
	oldwep = uwep;
	oldswap = uswapwep;
	setuswapwep((struct obj *) 0);

	/* Set your new primary weapon */
	result = ready_weapon(oldswap, FALSE);

	/* Set your new secondary weapon */
	if (uwep == oldwep)
		/* Wield failed for some reason */
		setuswapwep(oldswap);
	else {
		setuswapwep(oldwep);
		if (uswapwep)
			prinv((char *)0, uswapwep, 0L);
		else
			You("have no secondary weapon readied.");
	}

	if (u.twoweap && !test_twoweapon())
		untwoweapon();

	// return (result);
	return MOVE_INSTANT;
}

int
dowieldquiver()
{
	register struct obj *newquiver;
	const char *quivee_types = (uslinging() ||
		  (uswapwep && objects[uswapwep->otyp].oc_skill == P_SLING)) ?
				  bullets : ready_objs;

	/* Since the quiver isn't in your hands, don't check cantwield(), */
	/* will_weld(), touch_petrifies(), etc. */
	multi = 0;

	/* Because 'Q' used to be quit... */
	if (flags.suppress_alert < FEATURE_NOTICE_VER(3,3,0))
		pline("Note: Please use #quit if you wish to exit the game.");

	/* Prompt for a new quiver */
	if (!(newquiver = getobj(quivee_types, "ready")))
		/* Cancelled */
		return MOVE_CANCELLED;

	/* Handle no object, or object in other slot */
	/* Any type is okay, since we give no intrinsics anyways */
	if (newquiver == &zeroobj) {
		/* Explicitly nothing */
		if (uquiver) {
			You("now have no ammunition readied.");
			setuqwep(newquiver = (struct obj *) 0);
		} else {
			You("already have no ammunition readied!");
			return MOVE_CANCELLED;
		}
	} else if (newquiver == uquiver) {
		pline("That ammunition is already readied!");
		return MOVE_CANCELLED;
	} else if (newquiver == uwep) {
		/* Prevent accidentally readying the main weapon */
		pline("%s already being used as a weapon!",
		      !is_plural(uwep) ? "That is" : "They are");
		return MOVE_CANCELLED;
	} else if (newquiver->owornmask & (W_ARMOR | W_RING | W_AMUL | W_BELT | W_TOOL
#ifdef STEED
			| W_SADDLE
#endif
			)) {
		You("cannot ready that!");
		return MOVE_CANCELLED;
	} else {
		long dummy;


		/* Check if it's the secondary weapon */
		if (newquiver == uswapwep) {
			setuswapwep((struct obj *) 0);
			if (u.twoweap && !test_twoweapon())
				untwoweapon();
		}

		/* Okay to put in quiver; print it */
		dummy = newquiver->owornmask;
		newquiver->owornmask |= W_QUIVER;
		prinv((char *)0, newquiver, 0L);
		newquiver->owornmask = dummy;
	}

	/* Finally, place it in the quiver */
	setuqwep(newquiver);
	/* Take no time since this is a convenience slot */
	return MOVE_INSTANT;
}
/* use to re-wield a returned thrown weapon */
void
rewield(obj, slot)
struct obj * obj;
long slot;
{
	switch (slot) {
	case W_WEP:
		ready_weapon(obj, TRUE);
		break;
	case W_SWAPWEP:
		setuswapwep(obj);
		break;
	case W_QUIVER:
		setuqwep(obj);
		break;
	}
	return;
}

/* used for #rub and for applying pick-axe, whip, grappling hook, or polearm */
/* (moved from apply.c) */
boolean
wield_tool(obj, verb)
struct obj *obj;
const char *verb;	/* "rub",&c */
{
    const char *what;
    boolean more_than_1;

    if (obj == uwep) return TRUE;   /* nothing to do if already wielding it */

    if (!verb) verb = "wield";
    what = xname(obj);
    more_than_1 = (obj->quan > 1L ||
		   strstri(what, "pair of ") != 0 ||
		   strstri(what, "s of ") != 0);

    if (obj->owornmask & (W_ARMOR|W_RING|W_AMUL|W_BELT|W_TOOL)) {
	char yourbuf[BUFSZ];

	You_cant("%s %s %s while wearing %s.",
		 verb, shk_your(yourbuf, obj), what,
		 more_than_1 ? "them" : "it");
	return FALSE;
    }
    if (welded(uwep)) {
	if (flags.verbose) {
	    const char *hand = body_part(HAND);

	    if (bimanual(uwep,youracedata)) hand = makeplural(hand);
	    if (strstri(what, "pair of ") != 0) more_than_1 = FALSE;
	    pline(
	     "Since your weapon is welded to your %s, you cannot %s %s %s.",
		  hand, verb, more_than_1 ? "those" : "that", xname(obj));
	} else {
	    You_cant("do that.");
	}
	return FALSE;
    }
	if(Straitjacketed){
	    pline(
	     "Since your %s are bound, you cannot %s %s %s.",
		  makeplural(body_part(HAND)), verb, more_than_1 ? "those" : "that", xname(obj));
		return FALSE;
	}
    if (you_cantwield(youracedata)) {
	You_cant("hold %s strongly enough.", more_than_1 ? "them" : "it");
	return FALSE;
    }
    /* check shield */
    if (uarms && bimanual(obj,youracedata)) {
	You("cannot %s a two-handed %s while wearing a shield.",
	    verb, (obj->oclass == WEAPON_CLASS) ? "weapon" : "tool");
	return FALSE;
    }
    if (uquiver == obj) setuqwep((struct obj *)0);
    if (uswapwep == obj) {
	(void) doswapweapon();
	/* doswapweapon might fail */
	if (uswapwep == obj) return FALSE;
    } else {
	You("now wield %s.", doname(obj));
	setuwep(obj);
    }
    if (uwep != obj) return FALSE;	/* rewielded old object after dying */
    /* applying weapon or tool that gets wielded ends two-weapon combat */
    if (u.twoweap && !test_twoweapon())
		untwoweapon();
    if (obj->oclass != WEAPON_CLASS)
	unweapon = TRUE;
    return TRUE;
}

/*Contains those parts of can_twoweapon() that DON'T change the game state.  Can be called anywhere the code needs to know if the player is capable of wielding two weapons*/
#define NOT_WEAPON(obj) (obj && !is_weptool(obj) && obj->oclass != WEAPON_CLASS && obj->otyp != STILETTOS && obj->otyp != WIND_AND_FIRE_WHEELS && obj->oartifact != ART_WAND_OF_ORCUS)
int
test_twoweapon()
{
	struct obj *otmp;

	/* all non-polyd players can twoweapon, albeit possibly very badly */
	if (Upolyd && !could_twoweap(youracedata)) {
		You_cant("use two weapons in your current form.");
	}
	/* you need to have weapons for both hands (in most cases) */
	else if ((!uwep || !uswapwep) && !(
		/* exceptions: */
			/* martial arts training let you use unarmed with either hand, and to supplement weapons */
			(martial_bonus() || u.umaniac) ||
			/* having the black web bound lets you use your shadowblades with either hand, and to supplement weapons */
			(u.specialSealsActive&SEAL_BLACK_WEB) ||
			/* wielding a bestial claw lets you replace offhand unarmed with an AT_CLAW attack */
			(uwep && uwep->otyp == BESTIAL_CLAW && active_glyph(BEASTS_EMBRACE))
		)){
		Your("%s%s%s empty.", uwep ? "off " : uswapwep ? "main " : "",
			body_part(HAND), (!uwep && !uswapwep) ? "s are" : " is");
	}
	/* Objects must not be non-weapons */
	else if ((NOT_WEAPON(uwep) || NOT_WEAPON(uswapwep))) {
		otmp = NOT_WEAPON(uwep) ? uwep : uswapwep;
		pline("%s %s.", Yname2(otmp),
		    is_plural(otmp) ? "aren't weapons" : "isn't a weapon");
	}
	/* not twohanded */
	else if ((
		/* twohanded (can be paired with punches) */
		(uwep && bimanual(uwep,youracedata) && !((martial_bonus() || u.umaniac) && !uswapwep)) || 
		(uswapwep && bimanual(uswapwep,youracedata) && !((martial_bonus() || u.umaniac) && !uwep))
		) &&
		/* Exception: Friede's Scythe can be offhanded with the (twohanded) Profaned Greatscythe or Lifehunt Scythe (or other farm implement). */
		!(uwep && uswapwep && is_farm(uwep) && uswapwep->oartifact == ART_FRIEDE_S_SCYTHE)
	) {
		otmp = bimanual(uwep,youracedata) ? uwep : uswapwep;
		if(otmp) pline("%s isn't one-handed.", Yname2(otmp));
		else You_cant("fight two-handed while wielding this.");
	}
	/* not a launcher (guns and blasters are ok) */
	else if (
		(uwep && is_launcher(uwep) && !is_firearm(uwep) && !is_melee_launcher(uwep)) ||
		(uswapwep && is_launcher(uswapwep) && !is_firearm(uswapwep) && !is_melee_launcher(uswapwep)))
	{
		You_cant("fight two-handed with this.");
	}
	/* cannot be wearing a shield */
	else if (uarms && activeFightingForm(FFORM_SHIELD_BASH))
		You("are already using a shield to bash enemies.");
	else if (uarms)
		You_cant("use two weapons while wearing a shield.");
	/* cannot fit armblaster over metal gloves */
	else if (uswapwep && uswapwep->otyp == ARM_BLASTER && uarmg && is_metallic(uarmg))
		You("cannot fit the bracer over such bulky, rigid gloves.");
	/* some artifacts resist being offhanded */
	else if (uswapwep && uswapwep->oartifact && !is_twoweapable_artifact(uswapwep))
		pline("%s %s being held second to another weapon!",
			Yname2(uswapwep), otense(uswapwep, "resist"));
	else
		return (TRUE);
	return (FALSE);
}

/*Contains those parts of can_twoweapon() that CAN change the game state.  Should only be called when the player commits to wielding two weapons*/
int
starting_twoweapon()
{
	struct obj *otmp;

	if (!test_twoweapon())
		; //NoOp, test_twoweapon prints output :/
	else if (!uarmg && !Stone_resistance && (uswapwep && uswapwep->otyp == CORPSE &&
		    touch_petrifies(&mons[uswapwep->corpsenm]))) {
		char kbuf[BUFSZ];

		You("wield the %s corpse with your bare %s.",
		    mons[uswapwep->corpsenm].mname, body_part(HAND));
		Sprintf(kbuf, "%s corpse", an(mons[uswapwep->corpsenm].mname));
		instapetrify(kbuf);
	} else if (uswapwep && (Glib || (!Weldproof && uswapwep->cursed))) {
		if (!Glib)
			uswapwep->bknown = TRUE;
		drop_uswapwep();
	} else
		return (TRUE);
	return (FALSE);
}

void
drop_uswapwep()
{
	char str[BUFSZ];
	struct obj *obj = uswapwep;

	/* Avoid trashing makeplural's static buffer */
	Strcpy(str, makeplural(body_part(HAND)));
	Your("%s from your %s!",  aobjnam(obj, "slip"), str);
	dropx(obj);
}

int
dotwoweapon()
{
	/* You can always toggle it off */
	if (u.twoweap) {
		You("switch to your primary weapon.");
		u.twoweap = 0;
		update_inventory();
		return MOVE_INSTANT;
	}

	/* May we use two weapons? */
	if (starting_twoweapon()) {
		/* Success! */
		You("begin two-weapon combat.");
		u.twoweap = 1;
		update_inventory();
		return (rnd(20) > ACURR(A_DEX)) ? MOVE_STANDARD : MOVE_INSTANT;
	}
	return MOVE_INSTANT;
}

/*** Functions to empty a given slot ***/
/* These should be used only when the item can't be put back in
 * the slot by life saving.  Proper usage includes:
 * 1.  The item has been eaten, stolen, burned away, or rotted away.
 * 2.  Making an item disappear for a bones pile.
 */
void
uwepgone()
{
	if (uwep) {
		if (artifact_light(uwep) && uwep->lamplit) {
		    end_burn(uwep, FALSE);
		    if (!Blind) pline("%s glowing.", Tobjnam(uwep, "stop"));
		}
		setworn((struct obj *)0, W_WEP);
		unweapon = TRUE;
		update_inventory();
	}
}

void
uswapwepgone()
{
	if (uswapwep) {
		setworn((struct obj *)0, W_SWAPWEP);
		update_inventory();
	}
}

void
uqwepgone()
{
	if (uquiver) {
		setworn((struct obj *)0, W_QUIVER);
		update_inventory();
	}
}

void
untwoweapon()
{
	if (u.twoweap) {
		You("can no longer use two weapons at once.");
		u.twoweap = FALSE;
		update_inventory();
	}
	return;
}

/* Maybe rust object, or corrode it if acid damage is called for */
void
erode_obj(target, acid_dmg, fade_scrolls)
struct obj *target;		/* object (e.g. weapon or armor) to erode */
boolean acid_dmg;
boolean fade_scrolls;
{
	int erosion;
	struct monst *victim;
	boolean vismon;
	boolean visobj;

	if (!target)
	    return;
	victim = carried(target) ? &youmonst :
	    mcarried(target) ? target->ocarry : (struct monst *)0;
	vismon = victim && (victim != &youmonst) && canseemon(victim);
	visobj = !victim && cansee(bhitpos.x, bhitpos.y); /* assume thrown */

	erosion = acid_dmg ? target->oeroded2 : target->oeroded;

	if (target->greased) {
	    grease_protect(target,(char *)0,victim);
	} else if (target->oclass == SCROLL_CLASS) {
	    if(fade_scrolls && target->otyp != SCR_BLANK_PAPER
#ifdef MAIL
	    && target->otyp != SCR_MAIL
#endif
					)
	    {
		if (!Blind) {
		    if (victim == &youmonst)
			Your("%s.", aobjnam(target, "fade"));
		    else if (vismon)
			pline("%s's %s.", Monnam(victim),
			      aobjnam(target, "fade"));
		    else if (visobj)
			pline_The("%s.", aobjnam(target, "fade"));
		}
		target->otyp = SCR_BLANK_PAPER;
		remove_oprop(target, OPROP_TACTB);
		target->spe = 0;
	    }
	} else if (target->oerodeproof ||
		(acid_dmg ? !is_corrodeable(target) : !is_rustprone(target))) {
	    if (flags.verbose || !(target->oerodeproof && target->rknown)) {
		if (victim == &youmonst)
		    Your("%s not affected.", aobjnam(target, "are"));
		else if (vismon)
		    pline("%s's %s not affected.", Monnam(victim),
			aobjnam(target, "are"));
		/* no message if not carried */
	    }
		if (target->oerodeproof && (acid_dmg ? is_corrodeable(target) : is_rustprone(target))) target->rknown = TRUE;
	} else if (erosion < MAX_ERODE) {
	    if (victim == &youmonst)
		Your("%s%s!", aobjnam(target, acid_dmg ? "corrode" : "rust"),
		    erosion+1 == MAX_ERODE ? " completely" :
		    erosion ? " further" : "");
	    else if (vismon)
		pline("%s's %s%s!", Monnam(victim),
		    aobjnam(target, acid_dmg ? "corrode" : "rust"),
		    erosion+1 == MAX_ERODE ? " completely" :
		    erosion ? " further" : "");
	    else if (visobj)
		pline_The("%s%s!",
		    aobjnam(target, acid_dmg ? "corrode" : "rust"),
		    erosion+1 == MAX_ERODE ? " completely" :
		    erosion ? " further" : "");
	    if (acid_dmg)
		target->oeroded2++;
	    else
		target->oeroded++;
	} else {
	    if (flags.verbose) {
		if (victim == &youmonst)
		    Your("%s completely %s.",
			aobjnam(target, Blind ? "feel" : "look"),
			acid_dmg ? "corroded" : "rusty");
		else if (vismon)
		    pline("%s's %s completely %s.", Monnam(victim),
			aobjnam(target, "look"),
			acid_dmg ? "corroded" : "rusty");
		else if (visobj)
		    pline_The("%s completely %s.",
			aobjnam(target, "look"),
			acid_dmg ? "corroded" : "rusty");
	    }
	}
}

int
chwepon(otmp, amount)
register struct obj *otmp;
register int amount;
{
	const char *color = hcolor((amount < 0) ? NH_BLACK : NH_BLUE);
	const char *xtime;
	int otyp = STRANGE_OBJECT;
	int safelim;

	if(!uwep || (uwep->oclass != WEAPON_CLASS && !is_weptool(uwep)
				&& uwep->otyp != STILETTOS && uwep->otyp != WIND_AND_FIRE_WHEELS
	)) {
		char buf[BUFSZ];

		Sprintf(buf, "Your %s %s.", makeplural(body_part(HAND)),
			(amount >= 0) ? "twitch" : "itch");
		strange_feeling(otmp, buf);
		exercise(A_DEX, (boolean) (amount >= 0));
		return(0);
	}

	if (otmp && otmp->oclass == SCROLL_CLASS) otyp = otmp->otyp;

	if(uwep->otyp == WORM_TOOTH && amount >= 0 && (is_organic(uwep) || uwep->obj_material == MINERAL)) {
		uwep->otyp = CRYSKNIFE;
		uwep->oerodeproof = 0;
		Your("weapon seems sharper now.");
		uwep->cursed = 0;
		if (otyp != STRANGE_OBJECT) makeknown(otyp);
		return(1);
	}

	if(uwep->otyp == CRYSKNIFE && amount < 0) {
		uwep->otyp = WORM_TOOTH;
		uwep->oerodeproof = 0;
		Your("weapon seems duller now.");
		if (otyp != STRANGE_OBJECT && otmp->bknown) makeknown(otyp);
		return(1);
	}

	if (amount < 0 && uwep->oartifact && restrict_name(uwep, ONAME(uwep))) {
	    if (!Blind)
		Your("%s %s.", aobjnam(uwep, "faintly glow"), color);
	    return(1);
	}
	/* there is a (soft) upper and lower limit to uwep->spe */
	if(is_plusten(uwep))
		safelim = 9;
	else safelim = 5;
	if(((uwep->spe > safelim && amount >= 0) || (uwep->spe < -safelim && amount < 0))
								&& rn2(3) && uwep->oartifact != ART_ROD_OF_SEVEN_PARTS
								&& uwep->oartifact != ART_PEN_OF_THE_VOID
								&& uwep->oartifact != ART_ANNULUS
	) {
		if(uwep->oartifact || check_oprop(uwep, OPROP_DEEPW)){
			if (!Blind)
			Your("%s %s for a while and then %s.",
			 aobjnam(uwep, "violently glow"), color,
			 otense(uwep, "fade"));
			uwep->spe = 0;
		} else {
			if (!Blind)
			Your("%s %s for a while and then %s.",
			 aobjnam(uwep, "violently glow"), color,
			 otense(uwep, "evaporate"));
			else
			Your("%s.", aobjnam(uwep, "evaporate"));

			useupall(uwep);	/* let all of them disappear */
		}
		return(1);
	}
	if (!Blind) {
	    xtime = (amount*amount == 1) ? "moment" : "while";
	    Your("%s %s for a %s.",
		 aobjnam(uwep, amount == 0 ? "violently glow" : "glow"),
		 color, xtime);
	    if (otyp != STRANGE_OBJECT && uwep->known &&
		    (amount > 0 || (amount < 0 && otmp->bknown)))
		makeknown(otyp);
	}
	uwep->spe += amount;
	if(amount > 0) uwep->cursed = 0;

	/*
	 * Enchantment, which normally improves a weapon, has an
	 * addition adverse reaction on Magicbane whose effects are
	 * spe dependent.  Give an obscure clue here.
	 */
	if (uwep->oartifact == ART_MAGICBANE && uwep->spe >= 0) {
		Your("right %s %sches!",
			body_part(HAND),
			(((amount > 1) && (uwep->spe > 1)) ? "flin" : "it"));
	}

	/* an elven magic clue, cookie@keebler */
	/* elven weapons vibrate warningly when enchanted beyond a limit */
	if ((uwep->spe > safelim)
		&& uwep->oartifact != ART_PEN_OF_THE_VOID && uwep->oartifact != ART_ANNULUS &&
		(is_elven_weapon(uwep) || uwep->oartifact || !rn2(7)) &&
		uwep->oartifact != ART_ROD_OF_SEVEN_PARTS
	) Your("%s unexpectedly.",
		aobjnam(uwep, "suddenly vibrate"));
	
	if(uwep->oartifact == ART_ROD_OF_SEVEN_PARTS && uwep->spe > 7){
		uwep->spe = 7;
		Your("%s faintly for a moment.",aobjnam(uwep, "rattle"));
	}
	if(uwep->oartifact == ART_PEN_OF_THE_VOID && uwep->spe > 5 && !(quest_status.killed_nemesis && Role_if(PM_EXILE))){
		uwep->spe = 5;
	} else if(uwep->oartifact == ART_PEN_OF_THE_VOID && uwep->spe > 10){
		uwep->spe = 10;
	} else if(uwep->oartifact == ART_ANNULUS && uwep->spe > 7){
		uwep->spe = 7;
	}

	return(1);
}

int
welded(obj)
register struct obj *obj;
{
	if (obj && obj == uwep 
		&& will_weld(obj) && !Weldproof
	){
		obj->bknown = TRUE;
		return 1;
	}
	return 0;
}

void
weldmsg(obj)
register struct obj *obj;
{
	long savewornmask;

	savewornmask = obj->owornmask;
	Your("%s %s welded to your %s!",
		xname(obj), otense(obj, "are"),
		bimanual(obj,youracedata) ? (const char *)makeplural(body_part(HAND))
				: body_part(HAND));
	obj->owornmask = savewornmask;
}

/*
 * hand & a half weapons, or bonus strength dmg weapons in general.
 * basically, can this be two-handed if we have a free hand, for the purposes of extra str bonus
 * return str multiplier
 */
double
bimanual_mod(otmp, mon)
struct obj * otmp;
struct monst * mon;
{
	boolean youagr = (mon == &youmonst);
	struct obj *arms = (youagr ? uarms : which_armor(mon, W_ARMS));
	struct obj *swapwep = (youagr ? uswapwep : MON_SWEP(mon));
	
	if (!otmp)
		return 1;

	if (otmp->oartifact == ART_RUINOUS_DESCENT_OF_STARS)
		return 2;

	if (arms)
		return 1;


	/* monsters don't have a concept of swapwep outside two-weaponing,
	 * I believe, so assume if MON_SWEP then it's two-weaponing */
	if ((youagr && u.twoweap) || (!youagr && swapwep))
		return 1;

	if (bimanual(otmp, (youagr ? youracedata : mon->data)))
		return 2;

	if (otmp->oartifact==ART_PEN_OF_THE_VOID && otmp->ovara_seals&SEAL_MARIONETTE && mvitals[PM_ACERERAK].died > 0)
		return 2;

	if (otmp->otyp == FORCE_SWORD
		|| otmp->otyp == DISKOS
		|| otmp->otyp == ROD_OF_FORCE
		|| otmp->otyp == HUNTER_S_LONGSWORD
		|| otmp->oartifact == ART_HOLY_MOONLIGHT_SWORD
		|| (otmp->oartifact == ART_BLOODLETTER && artinstance[ART_BLOODLETTER].BLactive >= moves)
		|| (youagr && weapon_type(otmp) == P_QUARTERSTAFF)
	)
		return 2;

	if (is_spear(otmp))
		return 1.5;

	if (otmp->otyp == ISAMUSEI || otmp->otyp == CHIKAGE || otmp->otyp == KATANA || otmp->otyp == LONG_SWORD || is_vibrosword(otmp))
		return 1.5;
	
	return 1;
}

boolean
bimanual(otmp, ptr)
struct obj * otmp;
struct permonst * ptr;
{
	int wielder_size;	/* uses standard MZ_ values */
	int eff_size;		/* uses standard MZ_ values */
	int size_mod;		/* otyp's size modifier; halve */

	/* we *need* an object to get its size */
	if (!otmp)
		return FALSE;
	/* only weapons and tools are valid twohanded weapons */
	if (otmp->oclass != WEAPON_CLASS && otmp->oclass != TOOL_CLASS)
		return FALSE;
	/* get wielder's size -- optional, will assume medium (human) */
	wielder_size = (ptr ? ptr->msize : MZ_MEDIUM);
	
	if (ptr == youracedata){
		if (Role_if(PM_CAVEMAN))
			wielder_size += 1;
		if (u.sealsActive&SEAL_YMIR)
			wielder_size += 1;
	}

	/* Some creatures are specifically always able to wield any weapon in one hand */
	if (ptr && always_one_hand_mtyp(ptr))
		return FALSE;

	/* half-sword style requires both hands on the sword, regardless of size */
	if (ptr == youracedata && otmp->otyp == LONG_SWORD && selectedFightingForm(FFORM_HALF_SWORD))
		return TRUE;

	/* get object size */
	size_mod = objects[otmp->otyp].oc_size - MZ_MEDIUM;

	if (otmp->oartifact == ART_HOLY_MOONLIGHT_SWORD && otmp->lamplit)
		size_mod += 4;	/* medium (2)           -> huge  (4) */
	if (otmp->oartifact == ART_FRIEDE_S_SCYTHE)
		size_mod -= 1;	/* large (3)            -> somewhat large (2.5) */
	if (otmp->otyp == DOUBLE_LIGHTSABER)
		size_mod += 3;	/* somewhat small (1.5) -> large (3) */

	/* add size_mod/2 to the object's visible size, rounding down final effective size */
	eff_size = (2 * otmp->objsize + size_mod) / 2;
	
	/* bound to real sizes */
	if (eff_size < MZ_TINY)
		eff_size = MZ_TINY;
	if (eff_size > MZ_GIGANTIC)
		eff_size = MZ_GIGANTIC;
	if (eff_size > MZ_HUGE && eff_size != MZ_GIGANTIC)
		eff_size = MZ_HUGE;

	/* bimanual rule: Needs two hands if [object's size] > [wielder's size] by 1 full size */
	return (eff_size > wielder_size);
}

/*wield.c*/
