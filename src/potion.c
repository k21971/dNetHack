/*	SCCS Id: @(#)potion.c	3.4	2002/10/02	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "xhity.h"

#ifdef OVLB
boolean notonhead = FALSE;

static NEARDATA int nothing, unkn;
static NEARDATA const char beverages[] = { POTION_CLASS, 0 };

STATIC_DCL long FDECL(itimeout, (long));
STATIC_DCL void NDECL(ghost_from_bottle);
STATIC_DCL short FDECL(mixtype, (struct obj *,struct obj *));

#define polypotion(obj) \
	((obj)->otyp == POT_POLYMORPH || \
	((obj)->otyp == POT_BLOOD && (\
		(obj)->corpsenm == PM_CHAMELEON || \
		(obj)->corpsenm == PM_SMALL_MIMIC || \
		(obj)->corpsenm == PM_LARGE_MIMIC || \
		(obj)->corpsenm == PM_GIANT_MIMIC)\
	))

/* force `val' to be within valid range for intrinsic timeout value */
STATIC_OVL long
itimeout(val)
long val;
{
    if (val >= TIMEOUT) val = TIMEOUT;
    else if (val < 1) val = 0;

    return val;
}

/* increment `old' by `incr' and force result to be valid intrinsic timeout */
long
itimeout_incr(old, incr)
long old;
long incr;
{
    return itimeout((old & TIMEOUT) + incr);
}

/* set the timeout field of intrinsic `which' */
void
set_itimeout(which, val)
long *which, val;
{
    *which &= ~TIMEOUT;
    *which |= itimeout(val);
}

/* increment the timeout field of intrinsic `which' */
void
incr_itimeout(which, incr)
long *which;
long incr;
{
    set_itimeout(which, itimeout_incr(*which, incr));
}

void
make_confused(xtime,talk)
long xtime;
boolean talk;
{
	long old = HConfusion;

	if (!xtime && old) {
		if (talk)
		    You_feel("less %s now.",
			Hallucination ? "trippy" : "confused");
	}
	if ((xtime && !old) || (!xtime && old)) flags.botl = TRUE;

	set_itimeout(&HConfusion, xtime);
}

void
make_stunned(xtime,talk)
long xtime;
boolean talk;
{
	long old = HStun;

	if (!xtime && old) {
		if (talk)
		    You_feel("%s now.",
			Hallucination ? "less wobbly" : "a bit steadier");
	}
	if (xtime && !old) {
		if (talk) {
#ifdef STEED
			if (u.usteed)
				You("wobble in the saddle.");
			else
#endif
			You("%s...", stagger(&youmonst, "stagger"));
		}
	}
	if ((!xtime && old) || (xtime && !old)) flags.botl = TRUE;

	set_itimeout(&HStun, xtime);
}

void
make_doubtful(xtime,talk)
long xtime;
boolean talk;
{
	long old = HDoubt;

	if (!xtime && old) {
		if (talk)
		    You("%s.",
			Hallucination ? "less wobbly" : "have resolved your crisis of faith");
	}
	if (xtime && !old) {
		if (talk) {
			You("suddenly suffer a crisis of faith!");
		}
	}
	if ((!xtime && old) || (xtime && !old)) flags.botl = TRUE;

	set_itimeout(&HDoubt, xtime);
}

void
make_invulnerable(xtime,talk)
long xtime;
boolean talk;
{
	long old = HSanctuary;

	if (!xtime && old) {
		if (talk){
			if(!Blind)
				pline("The shimmering light fades.");
			else You_feel("exposed to harm once more.");
		}
	}
	if (xtime && !old) {
		if (talk) {
			if(!Blind)
				pline("Faint shimmering light surrounds you.");
			else You_feel("shielded from harm.");
		}
	}
	if ((!xtime && old) || (xtime && !old)) flags.botl = TRUE;

	set_itimeout(&HSanctuary, xtime);
}

void
make_sick(xtime, cause, talk, type)
long xtime;
const char *cause;	/* sickness cause */
boolean talk;
int type;
{
	long old = Sick;

	if (xtime > 0L) {
	    if (Sick_resistance) return;
		IMPURITY_UP(u.uimp_illness)
	    if (!old) {
		/* newly sick */
		if(talk) You_feel("deathly sick.");
	    } else {
		/* already sick */
		if (talk) You_feel("%s worse.",
			      xtime <= Sick/2L ? "much" : "even");
	    }
	    set_itimeout(&Sick, xtime);
	    u.usick_type |= type;
	    flags.botl = TRUE;
	} else if (old && (type & u.usick_type)) {
	    /* was sick, now not */
	    u.usick_type &= ~type;
	    if (u.usick_type) { /* only partly cured */
		if (talk) You_feel("somewhat better.");
		set_itimeout(&Sick, Sick * 2); /* approximation */
	    } else {
		if (talk) pline("What a relief!");
		Sick = 0L;		/* set_itimeout(&Sick, 0L) */
	    }
	    flags.botl = TRUE;
	}

	if (Sick) {
	    exercise(A_CON, FALSE);
	    if (cause) {
		(void) strncpy(u.usick_cause, cause, sizeof(u.usick_cause));
		u.usick_cause[sizeof(u.usick_cause)-1] = 0;
		}
	    else
		u.usick_cause[0] = 0;
	} else
	    u.usick_cause[0] = 0;
}

void
make_vomiting(xtime, talk)
long xtime;
boolean talk;
{
	long old = Vomiting;

	if(!xtime && old)
	    if(talk) You_feel("much less nauseated now.");

	set_itimeout(&Vomiting, xtime);
}

static const char vismsg[] = "vision seems to %s for a moment but is %s now.";
static const char eyemsg[] = "%s momentarily %s.";

void
make_blinded(xtime, talk)
long xtime;
boolean talk;
{
	long old = Blinded;
	boolean u_could_see, can_see_now;
	int eyecnt;
	char buf[BUFSZ];

	/* we need to probe ahead in case the Eye of the Overworld
	   are or will be overriding blindness */
	u_could_see = !Blind;
	Blinded = xtime ? 1L : 0L;
	can_see_now = !Blind;
	Blinded = old;		/* restore */

	if (u.usleep) talk = FALSE;

	if (can_see_now && !u_could_see) {	/* regaining sight */
	    if (talk) {
		if (Hallucination)
		    pline("Far out!  Everything is all cosmic again!");
		else
		    You("can see again.");
	    }
	} else if (old && !xtime) {
	    /* clearing temporary blindness without toggling blindness */
	    if (talk) {
		if (!haseyes(youracedata)) {
		    strange_feeling((struct obj *)0, (char *)0);
		} else if (Blindfolded) {
		    Strcpy(buf, body_part(EYE));
		    eyecnt = eyecount(youracedata);
		    Your(eyemsg, (eyecnt == 1) ? buf : makeplural(buf),
			 (eyecnt == 1) ? "itches" : "itch");
		} else {	/* Eye of the Overworld */
		    Your(vismsg, "brighten",
			 Hallucination ? "sadder" : "normal");
		}
	    }
	}

	if (u_could_see && !can_see_now) {	/* losing sight */
	    if (talk) {
		if (Hallucination)
		    pline("Oh, bummer!  Everything is dark!  Help!");
		else
		    pline("A cloud of darkness falls upon you.");
	    }
	    /* Before the hero goes blind, set the ball&chain variables. */
	    if (Punished) set_bc(0);
	} else if (!old && xtime) {
	    /* setting temporary blindness without toggling blindness */
	    if (talk) {
		if (!haseyes(youracedata)) {
		    strange_feeling((struct obj *)0, (char *)0);
		} else if (Blindfolded) {
		    Strcpy(buf, body_part(EYE));
		    eyecnt = eyecount(youracedata);
		    Your(eyemsg, (eyecnt == 1) ? buf : makeplural(buf),
			 (eyecnt == 1) ? "twitches" : "twitch");
		} else {	/* Eye of the Overworld */
		    Your(vismsg, "dim",
			 Hallucination ? "happier" : "normal");
		}
	    }
	}

	set_itimeout(&Blinded, xtime);

	if (u_could_see ^ can_see_now) {  /* one or the other but not both */
	    flags.botl = 1;
	    vision_full_recalc = 1;	/* blindness just got toggled */
	    if (Blind_telepat || Infravision) see_monsters();
	}
}

boolean
make_hallucinated(xtime, talk, mask)
long xtime;	/* nonzero if this is an attempt to turn on hallucination */
boolean talk;
long mask;	/* nonzero if resistance status should change by mask */
{
	long old = HHallucination;
	boolean changed = 0;
	const char *message, *verb;

	message = (!xtime) ? "Everything %s SO boring now." :
			     "Oh wow!  Everything %s so cosmic!";
	verb = (!Blind) ? "looks" : "feels";

	if (mask) {
	    if (HHallucination) changed = TRUE;

	    if (!xtime) EHalluc_resistance |= mask;
	    else EHalluc_resistance &= ~mask;
	} else {
	    if (!Halluc_resistance && (!!HHallucination != !!xtime))
		changed = TRUE;
	    set_itimeout(&HHallucination, xtime);

	    /* clearing temporary hallucination without toggling vision */
	    if (!changed && !HHallucination && old && talk) {
		if (!haseyes(youracedata)) {
		    strange_feeling((struct obj *)0, (char *)0);
		} else if (Blind) {
		    char buf[BUFSZ];
		    int eyecnt = eyecount(youracedata);

		    Strcpy(buf, body_part(EYE));
		    Your(eyemsg, (eyecnt == 1) ? buf : makeplural(buf),
			 (eyecnt == 1) ? "itches" : "itch");
		} else {	/* Grayswandir */
		    Your(vismsg, "flatten", "normal");
		}
	    }
	}

	if (changed) {
	    if (u.uswallow) {
		swallowed(0);	/* redraw swallow display */
	    } else {
		/* The see_* routines should be called *before* the pline. */
		see_monsters();
		see_objects();
		see_traps();
		see_altars();
	    }

	    /* for perm_inv and anything similar
	    (eg. Qt windowport's equipped items display) */
	    update_inventory();

	    flags.botl = 1;
	    if (talk) pline(message, verb);
	}
	return changed;
}

STATIC_OVL void
ghost_from_bottle()
{
	struct monst *mtmp = makemon(&mons[PM_GHOST], u.ux, u.uy, NO_MM_FLAGS);

	if (!mtmp) {
		pline("This bottle turns out to be empty.");
		return;
	}
	if (Blind) {
		pline("As you open the bottle, %s emerges.", something);
		return;
	}
	pline("As you open the bottle, an enormous %s emerges!",
		Hallucination ? rndmonnam() : (const char *)"ghost");
	if(flags.verbose)
	    You("are frightened to death, and unable to move.");
	nomul(-3, "being frightened to death");
	nomovemsg = "You regain your composure.";
}

/* "Quaffing is like drinking, except you spill more."  -- Terry Pratchett
 */
int
dodrink()
{
	register struct obj *otmp;
	const char *potion_descr;

	if (Strangled) {
		pline("If you can't breathe air, how can you drink liquid?");
		return MOVE_INSTANT;
	}
	
	if (uarmh && FacelessHelm(uarmh) && ((uarmh->cursed && !Weldproof) || !freehand())){
		pline("The %s covers your whole face.", xname(uarmh));
		display_nhwindow(WIN_MESSAGE, TRUE);    /* --More-- */
		return MOVE_INSTANT;
	}
	if (uarmc && FacelessCloak(uarmc) && ((uarmc->cursed && !Weldproof) || !freehand())){
		pline("The %s covers your whole face.", xname(uarmc));
		display_nhwindow(WIN_MESSAGE, TRUE);    /* --More-- */
		return MOVE_INSTANT;
	}
	/* Is there a fountain to drink from here? */
	if (IS_FOUNTAIN(levl[u.ux][u.uy].typ) && !Levitation) {
		if(yn("Drink from the fountain?") == 'y') {
			drinkfountain();
			return MOVE_QUAFFED;
		}
	}
#ifdef SINKS
	/* Or a kitchen sink? */
	if (IS_SINK(levl[u.ux][u.uy].typ)) {
		if (yn("Drink from the sink?") == 'y') {
			drinksink();
			return MOVE_QUAFFED;
		}
	}
#endif

    /* Or a forge? */
    if (IS_FORGE(levl[u.ux][u.uy].typ)
        /* not as low as floor level but similar restrictions apply */
        && can_reach_floor()
	) {
        if (yesno("Drink from the forge?", TRUE) == 'y') {
            drinkforge();
            return 1;
        }
    }
	/* Or are you surrounded by water? */
	if (Underwater || IS_PUDDLE(levl[u.ux][u.uy].typ) ||
			(is_pool(u.ux,u.uy, FALSE) && Wwalking)) {
		char buf[BUFSZ], buf2[BUFSZ];

		Sprintf(buf,"at your %s", makeplural(body_part(FOOT)));
		Sprintf(buf2,"Drink the water %s?", 
			(Underwater || (IS_PUDDLE(levl[u.ux][u.uy].typ) &&
			verysmall(youracedata) && !Wwalking)) ? "around you"
								: buf);
		if (yn(buf2) == 'y') {
		    pline("Do you know what lives in this water?!");
		}
	}

	otmp = getobj(beverages, "drink");
	if(!otmp) return MOVE_CANCELLED;
	if(otmp->ostolen){
		if(u.sealsActive&SEAL_ANDROMALIUS)
			unbind(SEAL_ANDROMALIUS, TRUE);
		/*stealing is impure*/
		IMPURITY_UP(u.uimp_theft)
	}

	/* quan > 1 used to be left to useup(), but we need to force
	   the current potion to be unworn, and don't want to do
	   that for the entire stack when starting with more than 1.
	   [Drinking a wielded potion of polymorph can trigger a shape
	   change which causes hero's weapon to be dropped.  In 3.4.x,
	   that led to an "object lost" panic since subsequent useup()
	   was no longer dealing with an inventory item.  Unwearing
	   the current potion is intended to keep it in inventory.] */
	if (otmp->quan > 1L) {
	    otmp = splitobj(otmp, 1L);
	    otmp->owornmask = 0L;	/* rest of original stuck unaffected */
	} else if (otmp->owornmask) {
	    remove_worn_item(otmp, FALSE);
	}
	
	otmp->in_use = TRUE;		/* you've opened the stopper */
	if (otmp->oartifact)
		otmp->in_use = FALSE;
	

#define POTION_OCCUPANT_CHANCE(n) (13 + 2*(n))	/* also in muse.c */

	potion_descr = OBJ_DESCR(objects[otmp->otyp]);
	if (potion_descr && !otmp->oartifact) {
	    if (!strcmp(potion_descr, "milky") &&
		    flags.ghost_count < MAXMONNO &&
		    !rn2(POTION_OCCUPANT_CHANCE(flags.ghost_count))) {
		ghost_from_bottle();
		useup(otmp);
		return MOVE_STANDARD;
	    } else if (!strcmp(potion_descr, "smoky") &&
		    flags.djinni_count < MAXMONNO &&
		    !rn2(POTION_OCCUPANT_CHANCE(flags.djinni_count))) {
		djinni_from_bottle(otmp);
		useup(otmp);
		return MOVE_STANDARD;
	    }
	}
	return dopotion(otmp, FALSE);
}

int
dopotion(otmp, force)
register struct obj *otmp;
boolean force;
{
	int retval;

	if(!force && (otmp->otyp == POT_GOAT_S_MILK || otmp->otyp == POT_PRIMORDIAL_WATERS) && u.veil){
		You("feel reality threatening to slip away from the mere scent of the potion!");
		if (yn("Are you sure you want to drink it?") != 'y'){
			return(0);
		}
		else pline("So be it.");
		//Note: the veil is not lifted until the potion is actually drunk.
	}

	otmp->in_use = TRUE;
	if (otmp->oartifact)
		otmp->in_use = FALSE;
	
	nothing = unkn = 0;
	if((retval = peffects(otmp, force)) >= 0) return(retval);

	if(nothing) {
	    unkn++;
	    You("have a %s feeling for a moment, then it passes.",
		  Hallucination ? "normal" : "peculiar");
	}
	if(otmp->dknown && !objects[otmp->otyp].oc_name_known) {
		if(!unkn) {
			makeknown(otmp->otyp);
			more_experienced(0,10);
		} else if(!objects[otmp->otyp].oc_uname)
			docall(otmp);
	}
	if(!otmp->oartifact) useup(otmp);
	return MOVE_QUAFFED;
}

int
peffects(otmp, force)
register struct obj	*otmp;
boolean force;
{
	register int i, ii, lim;
    boolean enhanced;

	switch(otmp->otyp){
	case POT_RESTORE_ABILITY:
		/* Restore one lost level if blessed */
		if (otmp->blessed && u.ulevel < u.ulevelmax) {
		    ///* when multiple levels have been lost, drinking
		    //   multiple potions will only get half of them back */
		    // u.ulevelmax -= 1;
		    pluslvl(FALSE);
		}
			/* Dissolve one morgul blade shard if blessed*/
		if(otmp->blessed && u.umorgul>0){
			u.umorgul--;
			if(u.umorgul)
				You_feel("the chill of death lessen.");
			else
				You_feel("the chill of death fade away.");
		}
		if(otmp->blessed && u.umummyrot){
			u.umummyrot = 0;
			You("stop shedding dust.");
		}
		if(!otmp->cursed){
			//Restore sanity if blessed or uncursed
			if(otmp->blessed)
				change_usanity(20, FALSE);
			else
				change_usanity(5, FALSE);
			if(youmonst.mbleed)
				Your("bleeding wound closes up.");
			youmonst.mbleed = 0;
		}
	case SPE_RESTORE_ABILITY:
		unkn++;
		if(otmp->cursed) {
		    pline("Ulch!  This makes you feel mediocre!");
		    break;
		} else {
		    pline("Wow!  This makes you feel %s!",
			  (otmp->blessed) ?
				(unfixable_trouble_count(FALSE) ? "better" : "great")
			  : "good");
		    i = rn2(A_MAX);		/* start at a random point */
		    for (ii = 0; ii < A_MAX; ii++) {
			lim = AMAX(i);
			if (i == A_STR && u.uhs >= 3) --lim;	/* WEAK */
			if (ABASE(i) < lim) {
			    ABASE(i) = lim;
			    flags.botl = 1;
			    /* only first found if not blessed */
			    if (!otmp->blessed) break;
			}
			if(++i >= A_MAX) i = 0;
		    }
		}
		break;
	case POT_HALLUCINATION:
		if (Hallucination || Halluc_resistance) nothing++;
		(void) make_hallucinated(itimeout_incr(HHallucination,
					   rn1(200, 600 - 300 * bcsign(otmp))),
				  TRUE, 0L);
		//Bad drugs: inflict brain damage
		if(otmp->cursed){
			if(u.usanity > 0)
				change_usanity(-1, TRUE);
			if(u.uinsight > 0)
				change_uinsight(-1);
			exercise(A_WIS, FALSE);
			exercise(A_INT, FALSE);
		}
		break;
	case POT_AMNESIA:
		pline(Hallucination? "This tastes like champagne!" :
			"This liquid bubbles and fizzes as you drink it.");
		forget(otmp->cursed ? 25 : otmp->blessed ? 0 : 10);
		if (Hallucination)
		    pline("Hakuna matata!");
		else
		    You_feel("your memories dissolve.");

		/* Blessed amnesia makes you forget lycanthropy, sickness */
		if (otmp->blessed) {
		    if (u.ulycn >= LOW_PM && !Race_if(PM_HUMAN_WEREWOLF)) {
			You("forget your affinity to %s!",
					makeplural(mons[u.ulycn].mname));
			if (youracedata->mtyp == u.ulycn)
			    you_unwere(FALSE);
			u.ulycn = NON_PM;	/* cure lycanthropy */
		    }
		    make_sick(0L, (char *) 0, TRUE, SICK_ALL);

		    /* You feel refreshed */
		    if(Race_if(PM_INCANTIFIER)) u.uen += 50 + rnd(50);
		    else u.uhunger += 50 + rnd(50);
			
			if(u.uhunger > get_uhungermax()) u.uhunger = get_uhungermax();
		    
		    newuhs(FALSE);
		} else {
			if(Role_if(PM_MADMAN)){
				You_feel("ashamed of wiping your own memory.");
				change_hod(otmp->cursed ? 5 : 2);
			}
		    exercise(A_WIS, FALSE);
		}
		
		//All amnesia causes you to forget your crisis of faith
		if(Doubt)
			You("forget your doubts.");
		make_doubtful(0L, FALSE);
		break;
	case POT_WATER:
		if(uclockwork){
			if(u.clockworkUpgrades&(OIL_STOVE|WOOD_STOVE|HELLFIRE_FURNACE)){
				You("pour the water into your boiler.");
				if(u.uboiler < MAX_BOILER) u.uboiler = min(MAX_BOILER, u.uboiler+400);
				else pline("The boiler overflows and water spills over your mechanisms");
			} else {
				pline("The water runs across your mechanisms.");
			}
		} else {
			if(!otmp->blessed && !otmp->cursed) {
				pline("This tastes like water.");
				if(!Race_if(PM_INCANTIFIER)) u.uhunger += rnd(10);
				
				if(u.uhunger > get_uhungermax()) u.uhunger = get_uhungermax();
		    
				newuhs(FALSE);
				break;
			}
			unkn++;
			if(is_undead(youracedata) || is_demon(youracedata) ||
					(u.ualign.type == A_CHAOTIC || u.ualign.type == A_NONE)) {
				if(otmp->blessed) {
				pline("This burns like acid!");
				exercise(A_CON, FALSE);
				if (u.ulycn >= LOW_PM) {
					Your("affinity to %s disappears!",
					 makeplural(mons[u.ulycn].mname));
					if (youracedata->mtyp == u.ulycn)
					you_unwere(FALSE);
					u.ulycn = NON_PM;	/* cure lycanthropy */
				}
				losehp(d(2,6), "potion of holy water", KILLED_BY_AN);
				} else if(otmp->cursed) {
				You_feel("quite proud of yourself.");
				healup(d(2,6),0,0,0);
				if (u.ulycn >= LOW_PM && !Upolyd) you_were();
				exercise(A_CON, TRUE);
				}
			} else {
				if(otmp->blessed) {
				You_feel("full of awe.");
				make_sick(0L, (char *) 0, TRUE, SICK_ALL);
				exercise(A_WIS, TRUE);
				exercise(A_CON, TRUE);
				if (u.ulycn >= LOW_PM)
					you_unwere(TRUE);	/* "Purified" */
				/* make_confused(0L,TRUE); */
				if(u.sealsActive&SEAL_MARIONETTE) unbind(SEAL_MARIONETTE,TRUE);
				} else {
				if(u.ualign.type == A_LAWFUL) {
					pline("This burns like acid!");
					losehp(d(2,6), "potion of unholy water",
					KILLED_BY_AN);
				} else
					You_feel("full of dread.");
				if (u.ulycn >= LOW_PM && !Upolyd) you_were();
				exercise(A_CON, FALSE);
				}
			}
		}
	break;
	case POT_STARLIGHT:
		if(uclockwork){
			if(u.clockworkUpgrades&(OIL_STOVE|WOOD_STOVE|HELLFIRE_FURNACE)){
				You("pour the water into your boiler.");
				if(u.uboiler < MAX_BOILER) u.uboiler = min(MAX_BOILER, u.uboiler+400);
				else pline("The boiler overflows and water spills over your mechanisms");
			} else {
				pline("The water runs across your mechanisms.");
			}
		} else {
			if(hates_silver(youracedata)) {
				pline("This burns like acid!");
				exercise(A_CON, FALSE);
				if (u.ulycn >= LOW_PM) {
					Your("affinity to %s disappears!",
					 makeplural(mons[u.ulycn].mname));
					if (youracedata->mtyp == u.ulycn)
					you_unwere(FALSE);
					u.ulycn = NON_PM;	/* cure lycanthropy */
				}
				losehp(d(3,20), "potion of silver starlight", KILLED_BY_AN);
			} else {
				You_feel("full of awe.");
				make_sick(0L, (char *) 0, TRUE, SICK_ALL);
				exercise(A_WIS, TRUE);
				exercise(A_CON, TRUE);
				if (u.ulycn >= LOW_PM)
					you_unwere(TRUE);	/* "Purified" */
				/* make_confused(0L,TRUE); */
				if(u.sealsActive&SEAL_MARIONETTE) unbind(SEAL_MARIONETTE,TRUE);
			}
		}
		/* Dissolve one morgul blade shard if blessed*/
		if(otmp->blessed && u.umorgul>0){
			u.umorgul--;
			if(u.umorgul)
				You_feel("the chill of death lessen.");
			else
				You_feel("the chill of death fade away.");
		}
	break;
	case POT_BOOZE:
		unkn++;
		if(uclockwork){ /* Note: Does not include Androids */
			pline("It would seem you just drank a bottle of industrial solvent.");
			if(u.sealsActive&SEAL_ENKI && u.uhp < u.uhpmax){
				pline("The fruits of civilization give you strength!");
				u.uhp += u.ulevel*10;
				if (u.uhp > u.uhpmax) u.uhp = u.uhpmax;
				if(u.udrunken < u.ulevel*3){//Enki allows clockworks to benefit from booze quaffing
					u.udrunken++;
					change_usanity(5, FALSE);
				} else {
					if(u.usanity < 50){
						change_usanity(min(5, 50 - u.usanity), FALSE);
					}
				}
				check_drunkard_trophy();
			}
		} else { /* Note: Androids can get drunk */
			pline("Ooph!  This tastes like %s%s!",
			      otmp->odiluted ? "watered down " : "",
			      Hallucination ? "dandelion wine" : "liquid fire");
			if(u.sealsActive&SEAL_ENKI && u.uhp < u.uhpmax){
				pline("The fruits of civilization give you strength!");
				u.uhp += u.ulevel*10;
				if (u.uhp > u.uhpmax) u.uhp = u.uhpmax;
			}
			if(u.udrunken < u.ulevel*3){
				u.udrunken++;
				change_usanity(5, FALSE);
			} else {
				if(u.usanity < 50){
					change_usanity(min(5, 50 - u.usanity), FALSE);
				}
			}
			check_drunkard_trophy();
			if (!otmp->blessed)
			    make_confused(itimeout_incr(HConfusion, d(3,8)), FALSE);
			/* the whiskey makes us feel better */
			if (!otmp->odiluted) healup(u.ulevel, 0, FALSE, FALSE);
			if(!Race_if(PM_INCANTIFIER) && !umechanoid) u.uhunger += 130 + 10 * (2 + bcsign(otmp));
			newuhs(FALSE);
		}
		if (!umechanoid){
			if(u.uhunger > get_uhungermax()){
				u.uhunger = get_uhungermax() - d(2,20);
				vomit();
				exercise(A_WIS, FALSE);
			}
			exercise(A_WIS, FALSE);
			if(otmp->cursed) {
				You("pass out.");
				multi = -rnd(15);
				nomovemsg = "You awake with a headache.";
				if (!umechanoid){
					make_vomiting(Vomiting+15+d(5,4), TRUE);
				}
			}
		}
		break;
	case POT_ENLIGHTENMENT:
		if(otmp->cursed) {
			unkn++;
			You("have an uneasy feeling...");
			exercise(A_WIS, FALSE);
			//Malign enlightenment
			change_uinsight(1);
		} else {
			if (otmp->blessed) {
				(void) adjattrib(A_INT, 1, FALSE);
				(void) adjattrib(A_WIS, 1, FALSE);
			}
			You_feel("self-knowledgeable...");
			display_nhwindow(WIN_MESSAGE, FALSE);
			doenlightenment();
			pline_The("feeling subsides.");
			exercise(A_WIS, TRUE);
		}
		break;
	case SPE_INVISIBILITY:
		/* spell cannot penetrate mummy wrapping */
		if (BInvis && is_mummy_wrap(uarmc)) {
			You_feel("rather itchy under your %s.", xname(uarmc));
			break;
		}
		/* FALLTHRU */
	case POT_INVISIBILITY:
		if (Invis || Blind || BInvis) {
		    nothing++;
		} else {
		    self_invis_message();
		}
		if (otmp->blessed) HInvis |= TIMEOUT_INF;
		else incr_itimeout(&HInvis, rn1(15,31));
		newsym(u.ux,u.uy);	/* update position */
		if(otmp->cursed) {
		    pline("For some reason, you feel your presence is known.");
		    aggravate();
		}
		break;
	case POT_SEE_INVISIBLE:
		/* tastes like fruit juice in Rogue */
	case POT_FRUIT_JUICE:
	    {
		int msg = Invisible && !Blind;

		unkn++;
		if (otmp->cursed)
		    pline("Yecch!  This tastes %s.",
			  Hallucination ? "overripe" : "rotten");
		else
		    pline(Hallucination ?
		      "This tastes like 10%% real %s%s all-natural beverage." :
				"This tastes like %s%s.",
			  otmp->odiluted ? "reconstituted " : "",
			  fruitname(TRUE));
		if (otmp->otyp == POT_FRUIT_JUICE) {
		    if(!Race_if(PM_INCANTIFIER) && !umechanoid) u.uhunger += (otmp->odiluted ? 40 : 100) + 10 * (2 + bcsign(otmp));
            if(u.uhunger > get_uhungermax()) u.uhunger = get_uhungermax();
		    newuhs(FALSE);
		    break;
		}
		if (!otmp->cursed) {
			/* Tell them they can see again immediately, which
			 * will help them identify the potion...
			 */
			make_blinded(0L,TRUE);
		} else {
			//Cursed may raise insight if it's low
			if(!rn2(u.uinsight))
				change_uinsight(1);
		}
		if (otmp->blessed)
			HSee_invisible |= TIMEOUT_INF;
		else
			incr_itimeout(&HSee_invisible, rn1(100,750));
		set_mimic_blocking(); /* do special mimic handling */
		see_monsters();	/* see invisible monsters */
		newsym(u.ux,u.uy); /* see yourself! */
		if (msg && !Blind) { /* Blind possible if polymorphed */
		    You("can see through yourself, but you are visible!");
		    unkn--;
		}
		break;
	    }
	case POT_PARALYSIS:
		if (Free_action)
		    You("stiffen momentarily.");
		else {
		    if (Levitation || Weightless||Is_waterlevel(&u.uz))
			You("are motionlessly suspended.");
#ifdef STEED
		    else if (u.usteed)
			You("are frozen in place!");
#endif
		    else
			Your("%s are frozen to the %s!",
			     makeplural(body_part(FOOT)), surface(u.ux, u.uy));
		    nomul(-(rn1(10, 25 - 12*bcsign(otmp))), "frozen by a potion");
		    nomovemsg = You_can_move_again;
		    exercise(A_DEX, FALSE);
		}
		break;
	case POT_SLEEPING:
		if(Sleep_resistance || Free_action)
		    You("yawn.");
		else {
		    You("suddenly fall asleep!");
		    fall_asleep(-rn1(10, 25 - 12*bcsign(otmp)), TRUE);
		}
		//Sedative
		change_usanity(5 + 10*bcsign(otmp), FALSE);
		break;
	case POT_MONSTER_DETECTION:
	case SPE_DETECT_MONSTERS:
		if (otmp->blessed) {
		    int x, y;

		    if (Detect_monsters) nothing++;
		    unkn++;
		    /* after a while, repeated uses become less effective */
		    if (HDetect_monsters >= 300L)
			i = 1;
		    else
			i = rn1(40,21);
		    incr_itimeout(&HDetect_monsters, i);
		    for (x = 1; x < COLNO; x++) {
			for (y = 0; y < ROWNO; y++) {
			    if (levl[x][y].glyph == GLYPH_INVISIBLE) {
				unmap_object(x, y);
				newsym(x,y);
			    }
			    if (MON_AT(x,y)) unkn = 0;
			}
		    }
		    see_monsters();
		    if (unkn) You_feel("lonely.");
		    break;
		}
		if (monster_detect(otmp, 0))
			return MOVE_QUAFFED;		/* nothing detected */
		exercise(A_WIS, TRUE);
		break;
	case POT_OBJECT_DETECTION:
	case SPE_DETECT_TREASURE:
		if (object_detect(otmp, 0))
			return MOVE_QUAFFED;		/* nothing detected */
		exercise(A_WIS, TRUE);
		break;
	case POT_SICKNESS:
		pline("Yecch!  This stuff tastes like poison.");
		if (otmp->blessed) {
		    pline("(But in fact it was mildly stale %s.)",
			  fruitname(TRUE));
		    if (!Role_if(PM_HEALER)) {
			/* NB: blessed otmp->fromsink is not possible */
			losehp(1, "mildly contaminated potion", KILLED_BY_AN);
		    }
		} else {
		    if(Poison_resistance){
			pline(
			  "(But in fact it was biologically contaminated %s.)",
			      fruitname(TRUE));
		    if (Role_if(PM_HEALER))
				pline("Fortunately, you have been immunized.");
			} else {
			int typ = rn2(A_MAX);

			if (!Fixed_abil) {
			    poisontell(typ);
			    (void) adjattrib(typ,
			    		Poison_resistance ? -1 : -rn1(4,3),
			    		TRUE);
			}
			if(!Poison_resistance) {
			    if (otmp->fromsink)
				losehp(rnd(10)+5*!!(otmp->cursed),
				       "contaminated tap water", KILLED_BY);
			    else
				losehp(rnd(10)+5*!!(otmp->cursed),
				       "contaminated potion", KILLED_BY_AN);
			}
			exercise(A_CON, FALSE);
		    }
		}
		if(Hallucination) {
			You("are shocked back to your senses!");
			(void) make_hallucinated(0L,FALSE,0L);
		}
		if(u.sealsActive&SEAL_YMIR && !otmp->blessed) goto as_extra_healing;
		break;
	case POT_CONFUSION:
		if(!Confusion)
		    if (Hallucination) {
			pline("What a trippy feeling!");
			unkn++;
		    } else
			pline("Huh, What?  Where am I?");
		else	nothing++;
		make_confused(itimeout_incr(HConfusion,
					    rn1(7, 16 - 8 * bcsign(otmp))),
			      FALSE);
		break;
	case POT_GAIN_ABILITY:
		if(otmp->cursed) {
		    pline("Ulch!  That potion tasted foul!");
		    unkn++;
		} else if (Fixed_abil) {
		    nothing++;
		} else {      /* If blessed, increase all; if not, try up to */
		    int itmp; /* 6 times to find one which can be increased. */
		    i = -1;		/* increment to 0 */
		    for (ii = A_MAX; ii > 0; ii--) {
			i = (otmp->blessed ? i + 1 : rn2(A_MAX));
			/* only give "your X is already as high as it can get"
			   message on last attempt (except blessed potions) */
			itmp = (otmp->blessed || ii == 1) ? 0 : -1;
			if (adjattrib(i, 1, itmp) && !otmp->blessed)
			    break;
		    }
		}
	break;
	case POT_SPEED:
		if(Wounded_legs && !otmp->cursed
#ifdef STEED
		   && !u.usteed	/* heal_legs() would heal steeds legs */
#endif
						) {
			heal_legs();
			unkn++;
			break;
		}
	    if (!(HFast & INTRINSIC)) {
			if (!Fast) You("speed up.");
			else {
				Your("quickness feels more natural.");
				unkn++;
			}
			HFast |= TIMEOUT_INF;
	    } else nothing++;
		exercise(A_DEX, TRUE);
	break;
	case POT_BLINDNESS:
		if(Blind) nothing++;
		make_blinded(itimeout_incr(Blinded,
					   rn1(200, 250 - 125 * bcsign(otmp))),
			     (boolean)!Blind);
		break;
	case POT_GAIN_LEVEL:
		if (otmp->cursed) {
			unkn++;
			/* they went up a level */
			if((ledger_no(&u.uz) == 1 && u.uhave.amulet
				&& !(Role_if(PM_UNDEAD_HUNTER) && philosophy_index(u.ualign.god) && (!quest_status.moon_close || research_incomplete() || u.veil))
			) ||
				Can_rise_up(u.ux, u.uy, &u.uz)) {
			    const char *riseup ="rise up, through the %s!";
			    if(ledger_no(&u.uz) == 1) {
			        You(riseup, ceiling(u.ux,u.uy));
				goto_level(&earth_level, FALSE, FALSE, FALSE);
			    } else {
			        register int newlev = depth(&u.uz)-1;
				d_level newlevel;

				get_level(&newlevel, newlev);
				if(on_level(&newlevel, &u.uz)) {
				    pline("It tasted bad.");
				    break;
				} else You(riseup, ceiling(u.ux,u.uy));
				goto_level(&newlevel, FALSE, FALSE, FALSE);
			    }
			}
			else You("have an uneasy feeling.");
			break;
		}
		pluslvl(FALSE);
		if (otmp->blessed)
			/* blessed potions place you at a random spot in the
			 * middle of the new level instead of the low point
			 */
			u.uexp = rndexp(TRUE);
		break;
	case POT_HEALING:
		You_feel("better.");
        enhanced = uarmg && uarmg->oartifact == ART_GAUNTLETS_OF_THE_HEALING_H;
		healup(d((enhanced ? 2 : 1) * (6 + 2 * bcsign(otmp)), 4)+ (enhanced ? 2 : 1) * mlev(&youmonst),
		       !(get_ox(otmp, OX_ESUM)) * ((enhanced ? 2 : 1) * (!otmp->cursed ? 1 : 0)),
			   otmp->blessed, !otmp->cursed);
		exercise(A_CON, TRUE);
		break;
	case POT_EXTRA_HEALING:
as_extra_healing:
		You_feel("much better.");
        enhanced = uarmg && uarmg->oartifact == ART_GAUNTLETS_OF_THE_HEALING_H;
		healup(d((enhanced ? 2 : 1) * (6 + 2 * bcsign(otmp)), 8)+d((enhanced ? 2 : 1) * max(1, mlev(&youmonst)),8),
		       !(get_ox(otmp, OX_ESUM)) * (enhanced ? 2 : 1) * (otmp->blessed ? 5 : !otmp->cursed ? 2 : 0),
		       !otmp->cursed, TRUE);
		(void) make_hallucinated(0L,TRUE,0L);
		exercise(A_CON, TRUE);
		exercise(A_STR, TRUE);
		break;
	case POT_FULL_HEALING:
		You_feel("completely healed.");
        enhanced = uarmg && uarmg->oartifact == ART_GAUNTLETS_OF_THE_HEALING_H;
		healup(enhanced ? 800 : 400, 
				!(get_ox(otmp, OX_ESUM)) * (enhanced ? 2 : 1) * (4+4*bcsign(otmp)),
				!otmp->cursed, TRUE);
		/* Restore one lost level if blessed */
		if (otmp->blessed && u.ulevel < u.ulevelmax) {
		    ///* when multiple levels have been lost, drinking
		    //   multiple potions will only get half of them back */
		    // u.ulevelmax -= 1;
		    pluslvl(FALSE);
		}
		/* Dissolve one morgul blade shard if blessed*/
		if(otmp->blessed && u.umorgul>0){
			u.umorgul--;
			if(u.umorgul)
				You_feel("the chill of death lessen.");
			else
				You_feel("the chill of death fade away.");
		}
		if(otmp->blessed && u.umummyrot){
			u.umummyrot = 0;
			You("stop shedding dust.");
		}
		if(youmonst.mbleed)
			Your("bleeding wound closes up.");
		youmonst.mbleed = 0;
		(void) make_hallucinated(0L,TRUE,0L);
		exercise(A_STR, TRUE);
		exercise(A_CON, TRUE);
		break;
	case POT_GOAT_S_MILK:
        enhanced = uarmg && uarmg->oartifact == ART_GAUNTLETS_OF_THE_HEALING_H;
		u.shubbie_mutagen++;
		if(otmp->cursed){
			pline("Yecch! That was vile!");
			losehp(40, "spoiled milk", KILLED_BY);
			make_sick(Sick ? Sick / 3L + 1L : (long)rn1(ACURR(A_CON), 20),
				"spoiled milk", TRUE, SICK_VOMITABLE);
			break;
		} else {
			You_feel("completely healed.");
			healup(enhanced ? 800 : 400, 
					!(get_ox(otmp, OX_ESUM)) * (enhanced ? 2 : 1) * (4+4*bcsign(otmp)),
					TRUE, TRUE);
		}
		/* Restore lost levels */
		if (u.ulevel < u.ulevelmax) {
			if(otmp->blessed){
				pluslvl(FALSE);
				if(u.ulevel < u.ulevelmax)
					u.uexp = newuexp(u.ulevel) - 1;
			} else {
				pluslvl(FALSE);
			}
		}
		/* Dissolve all morgul blade shards */
		if(u.umorgul>0){
			u.umorgul = 0;
			You_feel("the chill of death fade away.");
		}
		if(otmp->blessed && u.umummyrot){
			u.umummyrot = 0;
			You("stop shedding dust.");
		}
		if(youmonst.mbleed)
			Your("bleeding wound closes up.");
		youmonst.mbleed = 0;
		for (int i = 0; i < A_MAX; i++) {
			lim = AMAX(i);
			if (i == A_STR && u.uhs >= 3) --lim;	/* WEAK */
			if (ABASE(i) < lim) {
				ABASE(i) = lim;
				flags.botl = 1;
				/* only first found if not blessed */
				if (!otmp->blessed) break;
			}
		}
		(void) make_hallucinated(0L,TRUE,0L);
		exercise(A_STR, TRUE);
		exercise(A_CON, TRUE);
		//Makes you crazy
		change_usanity(-1*d(4,4), FALSE);
		u.umadness |= MAD_GOAT_RIDDEN;
		lift_veil();
		break;
	case POT_SPACE_MEAD:
		unkn++;
		if(uclockwork){ /* Note: Does not include Androids */
			pline("It would seem you just drank a bottle of industrial solvent.");
			if(u.sealsActive&SEAL_ENKI && u.uhp < u.uhpmax){
				pline("The fruits of civilization give you strength!");
				u.uhp += u.ulevel*10;
				if (u.uhp > u.uhpmax) u.uhp = u.uhpmax;
				if(u.udrunken < u.ulevel*3) u.udrunken++; //Enki allows clockworks to benefit from booze quaffing
				check_drunkard_trophy();
			}
		} else { /* Note: Androids can get drunk */
			pline("Ooph!  This tastes like %s%s!",
				  otmp->odiluted ? "watered down " : "",
				  Hallucination ? "dandelion wine" : "liquid fire");
			if(u.sealsActive&SEAL_ENKI && u.uhp < u.uhpmax){
				pline("The fruits of civilization give you strength!");
				u.uhp += u.ulevel*10;
				if (u.uhp > u.uhpmax) u.uhp = u.uhpmax;
			}
			if(u.udrunken < u.ulevel*3){
				u.udrunken++;
				check_drunkard_trophy();
				change_usanity(5, FALSE);
			} else {
				if(u.usanity < 50){
					change_usanity(min(5, 50 - u.usanity), FALSE);
				}
			}
			if (!otmp->blessed)
				make_confused(itimeout_incr(HConfusion, d(3,8)), FALSE);
			/* the whiskey makes us feel better */
			if (!otmp->odiluted) healup(u.ulevel, 0, FALSE, FALSE);
			if(!Race_if(PM_INCANTIFIER) && !umechanoid) u.uhunger += 130 + 10 * (2 + bcsign(otmp));
			newuhs(FALSE);
		}
		if (!umechanoid){
			if(u.uhunger > get_uhungermax()){
				u.uhunger = get_uhungermax() - d(2,20);
				vomit();
				exercise(A_WIS, FALSE);
			}
			exercise(A_WIS, FALSE);
			if(otmp->cursed) {
				You("pass out.");
				multi = -rnd(15);
				nomovemsg = "You awake with a headache.";
				if (!umechanoid){
					make_vomiting(Vomiting+15+d(5,4), TRUE);
				}
			}
		}
		give_intrinsic(SLOW_DIGESTION, 5000L);
		if(!Breathless)
			You("no longer feel the need to breathe!");
		give_intrinsic(MAGICAL_BREATHING, 5000L);
		
		break;
	case POT_LEVITATION:
	case SPE_LEVITATION:
		if (otmp->cursed) HLevitation &= ~I_SPECIAL;
		if(!Levitation) {
			/* kludge to ensure proper operation of float_up() */
			HLevitation = 1;
			float_up();
			/* reverse kludge */
			HLevitation = 0;
			if (otmp->cursed && !Is_waterlevel(&u.uz)) {
				if((u.ux != xupstair || u.uy != yupstair)
				   && (u.ux != sstairs.sx || u.uy != sstairs.sy || !sstairs.up)
				   && (!xupladder || u.ux != xupladder || u.uy != yupladder)
				) {
					You("hit your %s on the %s.",
						body_part(HEAD),
						ceiling(u.ux,u.uy));
					losehp(uarmh ? 1 : rnd(10),
						"colliding with the ceiling",
						KILLED_BY);
				} else (void) doup();
			}
		} else
			nothing++;
		if (otmp->blessed) {
		    incr_itimeout(&HLevitation, rn1(50,250));
		    HLevitation |= I_SPECIAL;
		} else incr_itimeout(&HLevitation, rn1(140,10));
		spoteffects(FALSE);	/* for sinks */
		break;
	case POT_GAIN_ENERGY:			/* M. Stephenson */
		{	register int num;
			num = rnd(5) + 5 * otmp->blessed + 1;
			u.uenbonus += (otmp->cursed) ? -num : num;
			calc_total_maxen();
			u.uen += (otmp->cursed) ? -100 : (otmp->blessed) ? 200 : 100;
			if(u.uen > u.uenmax) u.uen = u.uenmax;
			if(u.uen <= 0 && !Race_if(PM_INCANTIFIER)) u.uen = 0;
			flags.botl = 1;
			if(!otmp->cursed) exercise(A_WIS, TRUE);
			//Doing the print last causes the bottom line update to show the changed energy scores.
			if(otmp->cursed)
			    You_feel("lackluster.");
			else
			    pline("Magical energies course through your body.");
		}
		break;
	case POT_OIL:				/* P. Winner */
		{
			boolean good_for_you = FALSE;

			if (otmp->lamplit) {
			    if (likes_fire(youracedata)) {
				pline("Ahh, a refreshing drink.");
				healup(d(4 + 2 * bcsign(otmp), 8),
					   !otmp->cursed ? 1 : 0, !!otmp->blessed, !otmp->cursed);
				if(u.uhs > SATIATED) lesshungry(400);
				good_for_you = TRUE;
			    } else {
				You("burn your %s.", body_part(FACE));
				if(!(HFire_resistance || u.sealsActive&SEAL_FAFNIR)) losehp(d(Fire_resistance ? 1 : 3, 4),
				       "burning potion of oil", KILLED_BY_AN);
			    }
			} else if(otmp->cursed){
				pline("This tastes like castor oil.");
				if(uclockwork && u.clockworkUpgrades&OIL_STOVE){
					u.ustove += 200;
					good_for_you = TRUE;
				}
			} else {
			    pline("That was smooth!");
				if(uclockwork){
					good_for_you = TRUE;
					healup(d(4 + 2 * bcsign(otmp), 4),
						   !otmp->cursed ? 1 : 0, !!otmp->blessed, !otmp->cursed);
					if(u.clockworkUpgrades&OIL_STOVE) u.ustove += 400;
					else if(u.uhs < WEAK && u.uhs > SATIATED) lesshungry(20);
				}
			}
			exercise(A_WIS, good_for_you);
		}
		break;
	case POT_ACID:
		if (Acid_resistance)
			/* Not necessarily a creature who _likes_ acid */
			pline("This tastes %s.", Hallucination ? "tangy" : "sour");
		else {
			pline("This burns%s!", otmp->blessed ? " a little" :
					otmp->cursed ? " a lot" : " like acid");
			losehp(d(otmp->cursed ? 2 : 1, otmp->blessed ? 4 : 8),
					"potion of acid", KILLED_BY_AN);
			exercise(A_CON, FALSE);
		}
		if (Stoned || Golded || Salted) fix_petrification();
		unkn++; /* holy/unholy water can burn like acid too */
		break;
	case POT_PRIMORDIAL_WATERS:{
		u.yog_sothoth_mutagen++;
		if (Acid_resistance)
			pline("This tastes like water.");
		else {
			pline("This burns%s!", otmp->blessed ? " a little" :
					otmp->cursed ? " a lot" : " like acid");
			losehp(d(otmp->cursed ? 2 : 1, otmp->blessed ? 4 : 8),
					"primordial water", KILLED_BY);
			exercise(A_CON, FALSE);
		}
		if (Stoned || Golded || Salted) fix_petrification();

		int num;
		num = rnd(5) + 5 * otmp->blessed + 1;
		if(otmp->cursed)
			num = -num;
		if(num < 0)
			u.uenbonus += num;
		else if(u.uenbonus < 0)
			u.uenbonus = min(0, u.uenbonus + num);
		calc_total_maxen();
		u.uen += (otmp->cursed) ? -100 : (otmp->blessed) ? 200 : 100;
		if(u.uenmax <= 0) u.uenmax = 0;
		if(u.uen > u.uenmax) u.uen = u.uenmax;
		if(u.uen <= 0 && !Race_if(PM_INCANTIFIER)) u.uen = 0;
		flags.botl = 1;
		if(!otmp->cursed) exercise(A_WIS, TRUE);
		if(!otmp->cursed) exercise(A_INT, TRUE);
		if(!otmp->cursed) exercise(A_CHA, TRUE);
		//Doing the print last causes the bottom line update to show the changed energy scores.
		if(otmp->cursed)
			You_feel("lackluster.");
		else
			pline("Magical energies course through your body.");
		}break;
	case POT_MIDAS:
		if (!Golded && !(Stone_resistance && youracedata->mtyp != PM_STONE_GOLEM)
			&& !is_gold(youracedata)
			&& !(poly_when_golded(youracedata) && polymon(PM_GOLD_GOLEM))
			) {
			Golded = 9;
			delayed_killer = "the draught of Midas";
			killer_format = KILLED_BY;
			You("are turning to gold!");
		} else
			You_feel("shiny inside.");
		break;
	case POT_POLYMORPH:
		You_feel("a little %s.", Hallucination ? "normal" : "strange");
		if (!Unchanging) polyself(FALSE);
		break;
	case POT_BLOOD:
		unkn++;
		if(!force && your_race(&mons[otmp->corpsenm]) && !is_animal(&mons[otmp->corpsenm]) && !mindless(&mons[otmp->corpsenm]) && !CANNIBAL_ALLOWED() 
			&& (u.ualign.record >= 20 || ACURR(A_WIS) >= 20 || u.ualign.record >= rnd(20-ACURR(A_WIS)))
		){
			char buf[BUFSZ];
			Sprintf(buf, "You feel a deep sense of kinship to %s!  Drink %s anyway?",
				the(xname(otmp)), (otmp->quan == 1L) ? "it" : "one");
			if (yn_function(buf,ynchars,'n')=='n'){
				otmp->in_use = FALSE;
				return MOVE_CANCELLED;
			}
		}
		if (is_vampire(youracedata) || (carnivorous(youracedata) && !herbivorous(youracedata))) {
			pline("It smells like %s%s.", 
					(!type_is_pname(&mons[otmp->corpsenm]) &&
					 (mons[otmp->corpsenm].geno & G_UNIQ)) ||
					Hallucination ? 
						"the " : 
						"", 
					Hallucination ? 
						makeplural(rndmonnam()) : 
						mons[otmp->corpsenm].geno & G_UNIQ ? 
						mons[otmp->corpsenm].mname : 
						makeplural(mons[otmp->corpsenm].mname)
			);
			if(!Hallucination) otmp->known = TRUE;
			if (!force && yn("Drink it?") == 'n') {
				break;
			}else{
				violated_vegetarian();
				u.uconduct.unvegan++;
				if (otmp->blessed)
				pline("Yecch!  This %s.", Hallucination ?
				"liquid could do with a good stir" : "blood has congealed");
				else pline(Hallucination ?
				  "The %s liquid stirs memories of home." :
				  "The %s blood tastes delicious.",
				  otmp->odiluted ? "watery" : "thick");
				if (!otmp->blessed && !Race_if(PM_INCANTIFIER) && !umechanoid)
					lesshungry((otmp->odiluted ? 1 : 2) *
						(otmp->cursed ? mons[(otmp)->corpsenm].cnutrit*1.5/5 : mons[(otmp)->corpsenm].cnutrit/5 ));
			}

		} else {
		    violated_vegetarian();
			u.uconduct.unvegan++;
		    pline("Ugh.  That was vile.");
		    if(!umechanoid && !Race_if(PM_INCANTIFIER)){
				make_vomiting(Vomiting+d(10,8), TRUE);
				if (!otmp->cursed)
				lesshungry((otmp->odiluted ? 1 : 2) *
					(otmp->blessed ? mons[(otmp)->corpsenm].cnutrit*1.5/5 : mons[(otmp)->corpsenm].cnutrit/5 ));
			}
		}
		IMPURITY_UP(u.uimp_blood)
		//Note: clockworks that use potions of blood as oil gain the intrinsics!
		//Incantifiers likewise gain intrinsics but not nutrition.
		cprefx(otmp->corpsenm, TRUE, FALSE);
	    cpostfx(otmp->corpsenm, FALSE, FALSE, FALSE);
	break;
	default:
		impossible("What a funny potion! (%u)", otmp->otyp);
		return MOVE_INSTANT;
	}
	return(-1);	/* will be handled further in dopotion() */
}

void
healup(nhp, nxtra, curesick, cureblind)
	int nhp, nxtra;
	register boolean curesick, cureblind;
{
	int * hpmax;
	int * hp;
	if(active_glyph(RADIANCE))
		nhp *= 1.3;
	if (nhp) {
		if (Upolyd) {
			hp = &u.mh;
			hpmax = &u.mhmax;
		} else {
			hp = &u.uhp;
			hpmax = &u.uhpmax;
		}
		*hp += nhp;
		if (*hp > *hpmax){
			if(u.uhpmod < 0){
				if(u.uhpmod + nxtra > 0){
					nxtra += u.uhpmod;
					u.uhpmod = 0;
					 u.uhpbonus += nxtra;
				} else {
					u.uhpmod += nxtra;
				}
			}
			else u.uhpbonus += nxtra;
			calc_total_maxhp();
			*hp = *hpmax;
		}
	}
	if(cureblind)	make_blinded(0L,TRUE);
	if(curesick)	make_sick(0L, (char *) 0, TRUE, SICK_ALL);
	flags.botl = 1;
	return;
}

void
strange_feeling(obj,txt)
register struct obj *obj;
register const char *txt;
{
	if (flags.beginner || !txt) {
		You("have a %s feeling for a moment, then it passes.",
		Hallucination ? "normal" : "strange");
	} else {
		pline1(txt);
	}

	if(!obj)	/* e.g., crystal ball finds no traps */
		return;

	if(obj->dknown && !objects[obj->otyp].oc_name_known &&
						!objects[obj->otyp].oc_uname)
		docall(obj);
	if(!(obj->oartifact)) useup(obj);
}

const char *bottlenames[] = {
	"bottle", "phial", "flagon", "carafe", "flask", "jar", "vial"
};


const char *
bottlename()
{
	return bottlenames[rn2(SIZE(bottlenames))];
}

void
potionhit(mon, obj, your_fault)
register struct monst *mon;
register struct obj *obj;
boolean your_fault;
{
	register const char *botlnam = bottlename();
	boolean isyou = (mon == &youmonst);
	int distance;

	if(isyou) {
		distance = 0;
		pline_The("%s crashes on your %s and breaks into shards.",
			botlnam, body_part(HEAD));
		losehp(rnd(2), "thrown potion", KILLED_BY_AN);
	} else {
		distance = distu(mon->mx,mon->my);
		if (!cansee(mon->mx,mon->my)) pline("Crash!");
		else {
		    char *mnam = mon_nam(mon);
		    char buf[BUFSZ];

		    if(has_head_mon(mon)) {
			Sprintf(buf, "%s %s",
				s_suffix(mnam),
				(notonhead ? "body" : "head"));
		    } else {
			Strcpy(buf, mnam);
		    }
		    pline_The("%s crashes on %s and breaks into shards.",
			   botlnam, buf);
		}
		if(rn2(5) && mon->mhp > 1)
			mon->mhp--;
	}

	/* oil and blood don't instantly evaporate */
	if (obj->otyp != POT_OIL && obj->otyp != POT_BLOOD && cansee(mon->mx,mon->my))
		pline("%s.", Tobjnam(obj, "evaporate"));

    if (isyou) {
	switch (obj->otyp) {
	case POT_OIL:
		if (obj->lamplit)
		    splatter_burning_oil(u.ux, u.uy);
	break;
	case POT_POLYMORPH:
		You_feel("a little %s.", Hallucination ? "normal" : "strange");
		if (!Unchanging && !Antimagic) polyself(FALSE);
	break;
	case POT_ACID:
		if (!Acid_resistance) {
		    pline("This burns%s!", obj->blessed ? " a little" :
				    obj->cursed ? " a lot" : "");
		    losehp(d(obj->cursed ? 2 : 1, obj->blessed ? 4 : 8),
				    "potion of acid", KILLED_BY_AN);
		}
	break;
	case POT_BLOOD:{
		int mtyp = obj->corpsenm;
		if(acidic(&mons[mtyp]) && !Acid_resistance){
		    pline("This burns%s!", obj->blessed ? " a little" :
				    obj->cursed ? " a lot" : "");
		    losehp(d(obj->cursed ? 2 : 1, obj->blessed ? 4 : 8),
				    "potion of acidic blood", KILLED_BY_AN);
		}
		if(freezing(&mons[mtyp]) && !Cold_resistance){
		    pline("This burns%s!", obj->blessed ? " a little" :
				    obj->cursed ? " a lot" : "");
		    losehp(d(obj->cursed ? 2 : 1, obj->blessed ? 4 : 8),
				    "potion of cryonic blood", KILLED_BY_AN);
		}
		if(burning(&mons[mtyp]) && !Fire_resistance){
		    pline("This burns%s!", obj->blessed ? " a little" :
				    obj->cursed ? " a lot" : "");
		    losehp(d(obj->cursed ? 2 : 1, obj->blessed ? 4 : 8),
				    "potion of scalding blood", KILLED_BY_AN);
		}
		if(poisonous(&mons[mtyp]) && !Poison_resistance){
			if (Upolyd) {
			    if (u.mh <= 5) u.mh = 1; else u.mh -= 5;
			} else {
			    if (u.uhp <= 5) u.uhp = 1; else u.uhp -= 5;
			}
			pline("Ecch - this must be poisonous!");
			losestr(1);
			exercise(A_CON, FALSE);
		}
		if (touch_petrifies(&mons[mtyp])) {
		    if (!Stone_resistance &&
			!(poly_when_stoned(youracedata) && polymon(PM_STONE_GOLEM))) {
			if (!Stoned) Stoned = 5;
			killer_format = KILLED_BY;
			Sprintf(killer_buf, "%s blood", mons[mtyp].mname);
			delayed_killer = killer_buf;
		    }
		}
		IMPURITY_UP(u.uimp_blood)
	}break;
	case POT_AMNESIA:
		/* Uh-oh! */
		if (uarmh && is_helmet(uarmh) && 
			rn2(10 - (uarmh->cursed? 8 : 0)))
		    get_wet(uarmh, TRUE);
	break;
	case POT_MIDAS:
		if (!Golded && !(Stone_resistance && youracedata->mtyp != PM_STONE_GOLEM)
			&& !is_gold(youracedata)
			&& !(poly_when_golded(youracedata) && polymon(PM_GOLD_GOLEM))
			) {
			Golded = 9;
			delayed_killer = "the draught of Midas";
			killer_format = KILLED_BY;
			You("are turning to gold!");
		} else
			You_feel("shiny inside.");
		break;
	}
    } else {
	boolean angermon = TRUE;

	if (!your_fault) angermon = FALSE;
	switch (obj->otyp) {
	case POT_HEALING:
	case POT_EXTRA_HEALING:
	case POT_FULL_HEALING:
		if (mon->mtyp == PM_PESTILENCE) goto do_illness;
		/*FALLTHRU*/
	case POT_GOAT_S_MILK:
	case POT_RESTORE_ABILITY:
	case POT_GAIN_ABILITY:
 do_healing:
		angermon = FALSE;
		if(mon->mhp < mon->mhpmax) {
		    mon->mhp = min(mon->mhpmax,mon->mhp+400);
		    if (canseemon(mon))
			pline("%s looks sound and hale again.", Monnam(mon));
		}
		break;
	case POT_SICKNESS:
		if (mon->mtyp == PM_PESTILENCE) goto do_healing;
		if (dmgtype(mon->data, AD_DISE) ||
			   dmgtype(mon->data, AD_PEST) || /* won't happen, see prior goto */
			   resists_poison(mon)) {
		    if (canseemon(mon))
			pline("%s looks unharmed.", Monnam(mon));
		    break;
		}
 do_illness:
		if((mon->mhpmax > 3) && !resist(mon, POTION_CLASS, 0, NOTELL))
			mon->mhpmax /= 2;
		if((mon->mhp > 2) && !resist(mon, POTION_CLASS, 0, NOTELL))
			mon->mhp /= 2;
		if (mon->mhp > mon->mhpmax) mon->mhp = mon->mhpmax;
		if (canseemon(mon))
		    pline("%s looks rather ill.", Monnam(mon));
		break;
	case POT_CONFUSION:
	case POT_BOOZE:
		if(!resist(mon, POTION_CLASS, 0, NOTELL))  mon->mconf = TRUE;
		break;
	case POT_INVISIBILITY:
		angermon = FALSE;
		mon_set_minvis(mon);
		break;
	case POT_SLEEPING:
		/* wakeup() doesn't rouse victims of temporary sleep */
		if (sleep_monst(mon, rnd(12), POTION_CLASS)) {
		    pline("%s falls asleep.", Monnam(mon));
		    slept_monst(mon);
		}
		break;
	case POT_PARALYSIS:
		if (mon->mcanmove) {
			if (mon_resistance(mon, FREE_ACTION)) {
				pline("%s stiffens momentarily.", Monnam(mon));
			} else {
				mon->mcanmove = 0;
				/* really should be rnd(5) for consistency with players
				 * breathing potions, but...
				 */
				mon->mfrozen = rnd(25);
			}
		}
		break;
	case POT_SPEED:
		angermon = FALSE;
		mon_adjust_speed(mon, 1, obj, TRUE);
		break;
	case POT_BLINDNESS:
		if(!resists_blnd(mon)) {
		    register int btmp = 64 + rn2(32) +
			rn2(32) * !resist(mon, POTION_CLASS, 0, NOTELL);
		    btmp += mon->mblinded;
		    mon->mblinded = min(btmp,127);
		    mon->mcansee = 0;
		}
		break;
	case POT_HALLUCINATION:
		if(!resist(mon, POTION_CLASS, 0, NOTELL) &&
		   !mon_resistance(mon, HALLUC_RES)) {
			mon->mconf = TRUE;
			mon->mberserk = TRUE;
		}
		break;
	case POT_WATER:
		if (is_undead(mon->data) || is_demon(mon->data) ||
			is_were(mon->data)) {
		    if (obj->blessed) {
			pline("%s %s in pain!", Monnam(mon),
			      is_silent_mon(mon) ? "writhes" : "shrieks");
			mon->mhp -= d(2,6);
			//Slashem
			if (mon->mhp < 1) {
			    if (your_fault)
				killed(mon);
			    else
				monkilled(mon, "", AD_ACID);
			}
			else if (is_were(mon->data) && !is_human(mon->data))
			    new_were(mon);	/* revert to human */
		    } else if (obj->cursed) {
			angermon = FALSE;
			if (canseemon(mon))
			    pline("%s looks healthier.", Monnam(mon));
			mon->mhp += d(2,6);
			if (mon->mhp > mon->mhpmax) mon->mhp = mon->mhpmax;
			if (is_were(mon->data) && is_human(mon->data) &&
				!Protection_from_shape_changers)
			    new_were(mon);	/* transform into beast */
		    }
		} else if(mon->mtyp == PM_GREMLIN) {
		    angermon = FALSE;
		    (void)split_mon(mon, (struct monst *)0);
		} else if(mon->mtyp == PM_FLAMING_SPHERE ||
			is_iron(mon)) {
		    if (canseemon(mon))
			pline("%s %s.", Monnam(mon),
				is_iron(mon) ?
				"rusts" : "flickers");
		    mon->mhp -= d(1,6);
		    if (mon->mhp < 1) {
			if (your_fault)
			    killed(mon);
			else
			    monkilled(mon, "", AD_ACID);
		    }
		}
		break;
	case POT_STARLIGHT:
		if (hates_silver(mon->data)) {
			pline("%s %s in pain!", Monnam(mon),
			      is_silent_mon(mon) ? "writhes" : "shrieks");
			mon->mhp -= d(3,20);
			//Slashem
			if (mon->mhp < 1) {
			    if (your_fault)
				killed(mon);
			    else
				monkilled(mon, "", AD_ACID);
			}
			else if (is_were(mon->data) && !is_human(mon->data))
			    new_were(mon);	/* revert to human */
		} else if(mon->mtyp == PM_GREMLIN) {
		    angermon = FALSE;
		    (void)split_mon(mon, (struct monst *)0);
		} else if(mon->mtyp == PM_FLAMING_SPHERE ||
			is_iron(mon)) {
		    if (canseemon(mon))
			pline("%s %s.", Monnam(mon),
				is_iron(mon) ?
				"rusts" : "flickers");
		    mon->mhp -= d(1,6);
		    if (mon->mhp < 1) {
			if (your_fault)
			    killed(mon);
			else
			    monkilled(mon, "", AD_ACID);
		    }
		}
		break;
	case POT_AMNESIA:
		switch (monsndx(mon->data)) {
		case PM_GREMLIN:
		    /* Gremlins multiply... */
		    mon->mtame = FALSE;	
		    (void)split_mon(mon, (struct monst *)0);
		    break;
		case PM_FLAMING_SPHERE:
		case PM_IRON_GOLEM:
		case PM_GREEN_STEEL_GOLEM:
		case PM_CHAIN_GOLEM:
		    if (canseemon(mon)) pline("%s %s.", Monnam(mon),
			    is_iron(mon) ?
			    "rusts" : "flickers");
		    mon->mhp -= d(1,6);
		    if (mon->mhp < 1)
			if (your_fault)
			    killed(mon);
			else
			    monkilled(mon, "", AD_ACID);
		    else
			mon->mtame = FALSE;
			mon->mpeaceful = TRUE;
		    break;
		case PM_WIZARD_OF_YENDOR:
		    if (your_fault) {
			if (canseemon(mon)) 
			    pline("%s laughs at you!", Monnam(mon));
			forget(1);
		    }
		    break;
		case PM_MEDUSA:
		    if (canseemon(mon))
			pline("%s looks like %s's having a bad hair day!", 
					Monnam(mon), mhe(mon));
		    break;
		case PM_CROESUS:
		    if (canseemon(mon))
		        pline("%s says: 'My gold! I must count my gold!'", 
					Monnam(mon));
		    break;
		case PM_DEATH:
 		    if (canseemon(mon))
		        pline("%s pauses, then looks at you thoughtfully!", 
					Monnam(mon));
		    break;
		case PM_FAMINE:
		    if (canseemon(mon))
		        pline("%s looks unusually hungry!", Monnam(mon));
		    break;
		case PM_PESTILENCE:
		    if (canseemon(mon))
		        pline("%s looks unusually well!", Monnam(mon));
		    break;
		default:
		    if (mon->data->msound == MS_NEMESIS && canseemon(mon)
				    && your_fault
			){
				pline("%s curses your ancestors!", Monnam(mon));
			}
		    else if (mon->isshk) {
				angermon = FALSE;
				if (canseemon(mon))
					pline("%s looks at you curiously!", 
							Monnam(mon));
				mon->mamnesia = TRUE;
		    } else if (!is_covetous(mon->data) &&
				    !resist(mon, POTION_CLASS, 0, 0)) {
				angermon = FALSE;
				if (canseemon(mon)) {
					if (mon->msleeping) {
					wakeup(mon, FALSE);
					pline("%s wakes up looking bewildered!", 
							Monnam(mon));
					} else
						pline("%s looks bewildered!", Monnam(mon));
				}
				mon->mamnesia = TRUE;
		    }
		    break;
		}
		break;
	case POT_OIL:
		if (obj->lamplit)
			splatter_burning_oil(mon->mx, mon->my);
		break;
	case POT_ACID:
		if (!resists_acid(mon) && !resist(mon, POTION_CLASS, 0, NOTELL)) {
		    pline("%s %s in pain!", Monnam(mon),
			  is_silent_mon(mon) ? "writhes" : "shrieks");
		    mon->mhp -= d(obj->cursed ? 2 : 1, obj->blessed ? 4 : 8);
		    if (mon->mhp < 1) {
			if (your_fault)
			    killed(mon);
			else
			    monkilled(mon, "", AD_ACID);
		    }
		}
		break;
	case POT_PRIMORDIAL_WATERS:
		if (!resists_acid(mon) && !resist(mon, POTION_CLASS, 0, NOTELL)) {
		    pline("%s %s in pain!", Monnam(mon),
			  is_silent_mon(mon) ? "writhes" : "shrieks");
		    mon->mhp -= d(obj->cursed ? 2 : 1, obj->blessed ? 4 : 8);
		    if (mon->mhp < 1) {
			if (your_fault)
			    killed(mon);
			else
			    monkilled(mon, "", AD_ACID);
		    }
		}
		mon->mspec_used = 0;
		if(mon->mcan)
			set_mcan(mon, FALSE);
		break;
	case POT_BLOOD:{
		int mtyp = obj->corpsenm;
		if(acidic(&mons[mtyp]) && !resists_acid(mon)){
		    pline("%s %s in pain!", Monnam(mon),
			  is_silent_mon(mon) ? "writhes" : "shrieks");
		    mon->mhp -= d(obj->cursed ? 2 : 1, obj->blessed ? 4 : 8);
		    if (mon->mhp < 1) {
			if (your_fault)
			    killed(mon);
			else
			    monkilled(mon, "", AD_ACID);
			}
		}
		if(freezing(&mons[mtyp]) && !resists_cold(mon)){
		    pline("%s %s in pain!", Monnam(mon),
			  is_silent_mon(mon) ? "writhes" : "shrieks");
		    mon->mhp -= d(obj->cursed ? 2 : 1, obj->blessed ? 4 : 8);
		    if (mon->mhp < 1) {
			if (your_fault)
			    killed(mon);
			else
			    monkilled(mon, "", AD_COLD);
			}
		}
		if(burning(&mons[mtyp]) && !resists_fire(mon)){
		    pline("%s %s in pain!", Monnam(mon),
			  is_silent_mon(mon) ? "writhes" : "shrieks");
		    mon->mhp -= d(obj->cursed ? 2 : 1, obj->blessed ? 4 : 8);
		    if (mon->mhp < 1) {
			if (your_fault)
			    killed(mon);
			else
			    monkilled(mon, "", AD_FIRE);
			}
		}
		if(poisonous(&mons[mtyp]) && !resists_poison(mon)){
			if((mon->mhpmax > 3) && !resist(mon, POTION_CLASS, 0, NOTELL))
				mon->mhpmax /= 2;
			if((mon->mhp > 2) && !resist(mon, POTION_CLASS, 0, NOTELL))
				mon->mhp /= 2;
			if (mon->mhp > mon->mhpmax) mon->mhp = mon->mhpmax;
			if (canseemon(mon))
				pline("%s looks rather ill.", Monnam(mon));
		}
		if (touch_petrifies(&mons[mtyp]) && !resists_ston(mon)) {
			minstapetrify(mon, TRUE);
		}
	}break;
	case POT_POLYMORPH:
		(void) bhitm(mon, obj);
		break;
	case POT_MIDAS:
		if (!resists_ston(mon) && !is_gold(mon->data)) {
			minstagoldify(mon, TRUE);
		}
		break;
/*
	case POT_GAIN_LEVEL:
	case POT_LEVITATION:
	case POT_SPACE_MEAD:
	case POT_FRUIT_JUICE:
	case POT_MONSTER_DETECTION:
	case POT_OBJECT_DETECTION:
		break;
*/
	}
	wakeup(mon, angermon);
	}

	/* Note: potionbreathe() does its own docall() */
	if ((distance==0 || ((distance < 3) && rn2(5))) &&
	    (!breathless(youracedata) || haseyes(youracedata)))
		potionbreathe(obj);
	else if (obj->dknown && !objects[obj->otyp].oc_name_known &&
		   !objects[obj->otyp].oc_uname && cansee(mon->mx,mon->my))
		docall(obj);
	if(*u.ushops && obj->unpaid) {
	        register struct monst *shkp =
			shop_keeper(*in_rooms(u.ux, u.uy, SHOPBASE));

		if(!shkp)
		    obj->unpaid = 0;
		else {
		    (void)stolen_value(obj, u.ux, u.uy,
				 (boolean)shkp->mpeaceful, FALSE);
		    subfrombill(obj, shkp);
		}
	}
	obfree(obj, (struct obj *)0);
}

/* vapors are inhaled or get in your eyes */
void
potionbreathe(obj)
register struct obj *obj;
{
	register int i, ii, isdone, kn = 0;

	switch(obj->otyp) {
	case POT_RESTORE_ABILITY:
	case POT_GAIN_ABILITY:
		if(obj->cursed) {
		    if (!breathless(youracedata))
			pline("Ulch!  That potion smells terrible!");
		    else if (haseyes(youracedata)) {
			int numeyes = eyecount(youracedata);
			Your("%s sting%s!",
			     (numeyes == 1) ? body_part(EYE) : makeplural(body_part(EYE)),
			     (numeyes == 1) ? "s" : "");
		    }
		    break;
		} else {
		    i = rn2(A_MAX);		/* start at a random point */
		    for(isdone = ii = 0; !isdone && ii < A_MAX; ii++) {
			if(ABASE(i) < AMAX(i)) {
			    ABASE(i)++;
			    /* only first found if not blessed */
			    isdone = !(obj->blessed);
			    flags.botl = 1;
			}
			if(++i >= A_MAX) i = 0;
		    }
		}
		break;
	case POT_FULL_HEALING:
		if (Upolyd && u.mh < u.mhmax) u.mh++, flags.botl = 1;
		if (u.uhp < u.uhpmax) u.uhp++, flags.botl = 1;
		/*FALL THROUGH*/
	case POT_EXTRA_HEALING:
		if (Upolyd && u.mh < u.mhmax) u.mh++, flags.botl = 1;
		if (u.uhp < u.uhpmax) u.uhp++, flags.botl = 1;
		/*FALL THROUGH*/
	case POT_HEALING:
		if (Upolyd && u.mh < u.mhmax) u.mh++, flags.botl = 1;
		if (u.uhp < u.uhpmax) u.uhp++, flags.botl = 1;
		exercise(A_CON, TRUE);
		break;
	case POT_SICKNESS:
		if (!Role_if(PM_HEALER)) {
			if (Upolyd) {
			    if (u.mh <= 5) u.mh = 1; else u.mh -= 5;
			} else {
			    if (u.uhp <= 5) u.uhp = 1; else u.uhp -= 5;
			}
			flags.botl = 1;
			exercise(A_CON, FALSE);
		}
		break;
	case POT_HALLUCINATION:
		You("have a momentary vision.");
		break;
	case POT_CONFUSION:
	case POT_BOOZE:
		if(!Confusion)
			You_feel("somewhat dizzy.");
		make_confused(itimeout_incr(HConfusion, rnd(5)), FALSE);
		break;
	case POT_INVISIBILITY:
		if (!Blind && !Invis) {
		    kn++;
		    pline("For an instant you %s!",
			See_invisible(u.ux,u.uy) ? "could see right through yourself"
			: "couldn't see yourself");
		}
		break;
	case POT_PARALYSIS:
		kn++;
		if (!Free_action) {
		    pline("%s seems to be holding you.", Something);
		    nomul(-rnd(5), "frozen by a potion");
		    nomovemsg = You_can_move_again;
		    exercise(A_DEX, FALSE);
		} else You("stiffen momentarily.");
		break;
	case POT_SLEEPING:
		kn++;
		if (!Free_action && !Sleep_resistance) {
		    You_feel("rather tired.");
		    nomul(-rnd(5), "sleeping off a magical draught");
		    nomovemsg = You_can_move_again;
		    exercise(A_DEX, FALSE);
		} else You("yawn.");
		break;
	case POT_SPEED:
		if (!Fast) Your("knees seem more flexible now.");
		incr_itimeout(&HFast, rnd(5));
		exercise(A_DEX, TRUE);
		break;
	case POT_BLINDNESS:
		if (!Blind && !u.usleep) {
		    kn++;
		    pline("It suddenly gets dark.");
		}
		make_blinded(itimeout_incr(Blinded, rnd(5)), FALSE);
		if (!Blind && !u.usleep) Your1(vision_clears);
		break;
	case POT_WATER:
		if(u.umonnum == PM_GREMLIN) {
		    (void)split_mon(&youmonst, (struct monst *)0);
		} else if (u.ulycn >= LOW_PM) {
		    /* vapor from [un]holy water will trigger
		       transformation but won't cure lycanthropy */
		    if (obj->blessed && youmonst.data->mtyp == u.ulycn)
			you_unwere(FALSE);
		    else if (obj->cursed && !Upolyd)
			you_were();
		}
		break;
	case POT_STARLIGHT:
		if(u.umonnum == PM_GREMLIN) {
		    (void)split_mon(&youmonst, (struct monst *)0);
		} else if (u.ulycn >= LOW_PM) {
		    /* vapor from [un]holy water will trigger
		       transformation but won't cure lycanthropy */
		    if (youmonst.data->mtyp == u.ulycn)
				you_unwere(FALSE);
		}
		break;
	case POT_AMNESIA:
		if(u.umonnum == PM_GREMLIN)
		    (void)split_mon(&youmonst, (struct monst *)0);
		else if(u.umonnum == PM_FLAMING_SPHERE) {
		    You("flicker!");
		    losehp(d(1,6),"potion of amnesia", KILLED_BY_AN);
		} else if(is_iron(youracedata)) {
		    You("rust!");
		    losehp(d(1,6),"potion of amnesia", KILLED_BY_AN);
		}
		You_feel("dizzy!");
		forget(1 + rn2(5));
		break;
	case POT_ACID:
	case POT_POLYMORPH:
		exercise(A_CON, FALSE);
		break;
	case POT_MIDAS:
		You("taste gold flakes.");
		exercise(A_CON, FALSE);
		break;
	case POT_BLOOD:
		if (is_vampire(youracedata)) {
		    exercise(A_WIS, FALSE);
		    You_feel("a sense of loss.");
		} else
		    exercise(A_CON, FALSE);
		break;
/*
	case POT_GAIN_LEVEL:
	case POT_LEVITATION:
	case POT_SPACE_MEAD:
	case POT_FRUIT_JUICE:
	case POT_MONSTER_DETECTION:
	case POT_OBJECT_DETECTION:
	case POT_OIL:
	case POT_GOAT_S_MILK:
		break;
*/
	}
	/* note: no obfree() */
	//contains a minor blindness fix from Slashem
	if (obj->dknown) {
	    if (kn)
		makeknown(obj->otyp);
	    else if (!objects[obj->otyp].oc_name_known &&
						!objects[obj->otyp].oc_uname && !Blind)
		docall(obj);
	}
}

STATIC_OVL short
mixtype(o1, o2)
register struct obj *o1, *o2;
/* returns the potion type when o1 is dipped in o2 */
{
	/* cut down on the number of cases below */
	if (o1->oclass == POTION_CLASS &&
	    (o2->otyp == POT_GAIN_LEVEL ||
	     o2->otyp == POT_GAIN_ENERGY ||
	     o2->otyp == POT_HEALING ||
	     o2->otyp == POT_EXTRA_HEALING ||
	     o2->otyp == POT_FULL_HEALING ||
	     o2->otyp == POT_ENLIGHTENMENT ||
	     o2->otyp == POT_FRUIT_JUICE)) {
		struct obj *swp;

		swp = o1; o1 = o2; o2 = swp;
	}
	switch (o1->otyp) {
		case POT_HEALING:
			switch (o2->otyp) {
			    case POT_BLOOD:
					if(o2->corpsenm != PM_DEEP_DRAGON && o2->corpsenm != PM_NEWT) break;
			    case POT_SPEED:
			    case POT_GAIN_LEVEL:
			    case POT_GAIN_ENERGY:
				return POT_EXTRA_HEALING;
			}
		case POT_EXTRA_HEALING:
			switch (o2->otyp) {
			    case POT_BLOOD:
					if(o2->corpsenm != PM_DEEP_DRAGON && o2->corpsenm != PM_NEWT) break;
			    case POT_GAIN_LEVEL:
			    case POT_GAIN_ENERGY:
				return POT_FULL_HEALING;
			}
		case POT_FULL_HEALING:
			switch (o2->otyp) {
			    case POT_BLOOD:
					if(o2->corpsenm != PM_DEEP_DRAGON && o2->corpsenm != PM_NEWT) break;
			    case POT_GAIN_LEVEL:
			    case POT_GAIN_ENERGY:
				return POT_GAIN_ABILITY;
			}
		case UNICORN_HORN:
			switch (o2->otyp) {
			    case POT_SICKNESS:
				return POT_FRUIT_JUICE;
			    case POT_HALLUCINATION:
			    case POT_BLINDNESS:
			    case POT_CONFUSION:
			    case POT_BLOOD:
				return POT_WATER;
			}
		break;
		case AMETHYST:		/* "a-methyst" == "not intoxicated" */
			if (o2->otyp == POT_BOOZE)
			    return POT_FRUIT_JUICE;
		break;
		case POT_GAIN_LEVEL:
		case POT_GAIN_ENERGY:
			switch (o2->otyp) {
			    case POT_CONFUSION:
				return (rn2(3) ? POT_BOOZE : POT_ENLIGHTENMENT);
			    case POT_HEALING:
				return POT_EXTRA_HEALING;
			    case POT_EXTRA_HEALING:
				return POT_FULL_HEALING;
			    case POT_FULL_HEALING:
				return POT_GAIN_ABILITY;
			    case POT_FRUIT_JUICE:
				return POT_SEE_INVISIBLE;
			    case POT_BOOZE:
				return POT_HALLUCINATION;
			}
		break;
		case POT_FRUIT_JUICE:
			switch (o2->otyp) {
			    case POT_SICKNESS:
				return POT_SICKNESS;
			    case POT_BLOOD:
				return POT_BLOOD;
			    case POT_SPEED:
				return POT_BOOZE;
			    case POT_GAIN_LEVEL:
			    case POT_GAIN_ENERGY:
				return POT_SEE_INVISIBLE;
			}
		break;
		case POT_ENLIGHTENMENT:
			switch (o2->otyp) {
			    case POT_LEVITATION:
				if (rn2(3)) return POT_GAIN_LEVEL;
				break;
			    case POT_FRUIT_JUICE:
				return POT_BOOZE;
			    case POT_BOOZE:
				return POT_CONFUSION;
			}
		break;
		case POT_BLOOD:
			if(o1->corpsenm == PM_MAID) switch (o2->otyp) {
			    case POT_BLOOD:
					if(o2->corpsenm != PM_DEEP_DRAGON && o2->corpsenm != PM_NEWT) break;
			    case POT_GAIN_LEVEL:
			    case POT_GAIN_ENERGY:
					return POT_GAIN_ABILITY;
				break;
			}
			if(o1->corpsenm == PM_BEHOLDER) switch (o2->otyp) {
			    case POT_LEVITATION:
				if (rn2(3)) return POT_GAIN_LEVEL;
				break;
			    case POT_FRUIT_JUICE:
				return POT_BOOZE;
			    case POT_BOOZE:
				return POT_CONFUSION;
			}
			if(o1->corpsenm == PM_DEEP_DRAGON || o1->corpsenm == PM_NEWT) switch (o2->otyp) {
			    case POT_CONFUSION:
				return (rn2(3) ? POT_BOOZE : POT_ENLIGHTENMENT);
			    case POT_HEALING:
				return POT_EXTRA_HEALING;
			    case POT_EXTRA_HEALING:
				return POT_FULL_HEALING;
			    case POT_FULL_HEALING:
				return POT_GAIN_ABILITY;
			    case POT_FRUIT_JUICE:
				return POT_SEE_INVISIBLE;
			    case POT_BOOZE:
				return POT_HALLUCINATION;
			}
		break;
	}
	/* MRKR: Extra alchemical effects. */

	if (o2->otyp == POT_ACID && o1->oclass == GEM_CLASS) {
	  char *potion_descr = NULL;
	  /* Note: you can't create smoky, milky or clear potions */

	  switch (o1->otyp) {

	    /* white */

	  case DILITHIUM_CRYSTAL:
	    /* explodes - special treatment in dodip */
	    /* here we just want to return something non-zero */
	    return POT_WATER;
	    break;
	  case MAGICITE_CRYSTAL:
	  case DIAMOND:
	  default:
	    /* won't dissolve */
	    break;
	  case OPAL:
	    potion_descr = "cloudy";
	    break;

	    /* red */

	  case RUBY:
	    potion_descr = "ruby";
	    break;
	  case GARNET:
	    potion_descr = "pink";
	    break;
	  case JASPER:
	    potion_descr = "purple-red";
	    break;

	    /* orange */

	  case JACINTH:
	    potion_descr = "orange";
	    break;
	  case AGATE:
	    potion_descr = "swirly";
	    break;

	    /* yellow */

	  case CITRINE:
	    potion_descr = "yellow";
	    break;
	  case CHRYSOBERYL:
	    potion_descr = "golden";
	    break;

	    /* yellowish brown */

	  case AMBER:
	    potion_descr = "brown";
	    break;
	  case TOPAZ:
	    potion_descr = "murky";
	    break;
	    
	    /* green */

	  case EMERALD:
	    potion_descr = "emerald";
	    break;
	  case TURQUOISE:
	    potion_descr = "sky blue";
	    break;
	  case AQUAMARINE:
	    potion_descr = "cyan";
	    break;
	  case JADE:
	    potion_descr = "dark green";
	    break;

	    /* blue */

	  case SAPPHIRE:
	  case STAR_SAPPHIRE:
	    potion_descr = "brilliant blue";
	    break;

	    /* violet */
	    
	  case AMETHYST:
	    potion_descr = "magenta";
	    break;
	  case VIOLET_FLUORITE:
	  case BLUE_FLUORITE:
	  case GREEN_FLUORITE:
	  case WHITE_FLUORITE:
	    potion_descr = "white";
	    break;

	    /* black */

	  case BLACK_OPAL:
	    potion_descr = "black";
	    break;
	  case JET:
	    potion_descr = "dark";
	    break;
	  case OBSIDIAN:
	    potion_descr = "effervescent";
	    break;
	  }
	  
	  if (potion_descr) {
	    int typ;
	    
	    /* find a potion that matches the description */

	    for (typ = bases[POTION_CLASS]; 
		 objects[typ].oc_class == POTION_CLASS;
		 typ++) {
	      
	      if (strcmp(potion_descr, OBJ_DESCR(objects[typ])) == 0) {
		return typ;
	      }	     
	    }
	  }
	}
	 
	return 0;
}

/* Bills an object that's about to be downgraded, assuming that's not already
 * been done */
STATIC_OVL
void
pre_downgrade_obj(obj, used)
register struct obj *obj;
boolean *used;
{
    boolean dummy = FALSE;

    if (!used) used = &dummy;
    if (!*used) Your("%s for a moment.", aobjnam(obj, "sparkle"));
    if(obj->unpaid && costly_spot(u.ux, u.uy) && !*used) {
	You("damage it, you pay for it.");
	bill_dummy_object(obj);
    }
    *used = TRUE;
}

/* Implements the downgrading effect of potions of amnesia and Lethe water */
STATIC_OVL
void
downgrade_obj(obj, nomagic, used)
register struct obj *obj;
int nomagic;	/* The non-magical object to downgrade to */
boolean *used;
{
    pre_downgrade_obj(obj, used);
    obj->otyp = nomagic;
    obj->spe = 0;
    obj->owt = weight(obj);
    flags.botl = TRUE;
}

//Lethe patch by way of Slashem
boolean
get_wet(obj, amnesia)
register struct obj *obj;
boolean amnesia;
/* returns TRUE if something happened (potion should be used up) */
{
	char Your_buf[BUFSZ];
	boolean used = FALSE;

	if (snuff_lit(obj)) return(TRUE);

	if (obj->greased) {
		grease_protect(obj,(char *)0,&youmonst);
		return(FALSE);
	}
	(void) Shk_Your(Your_buf, obj);
	/* (Rusting shop goods ought to be charged for.) */
	switch (obj->oclass) {
	    case POTION_CLASS:
		if (obj->otyp == POT_WATER) {
		    if (amnesia) {
			Your("%s to sparkle.", aobjnam(obj,"start"));
			obj->odiluted 	= 0;
			obj->otyp 	= POT_AMNESIA;
			used 		= TRUE;
			break;
		    }
		    return FALSE;
		}

		/* Diluting a !ofAmnesia just gives water... */
//Actually, the waters of the lethe shouldn't dilute
		if (obj->otyp == POT_AMNESIA) {
			return FALSE;
			// Your("%s flat.", aobjnam(obj, "become"));
			// obj->odiluted = 0;
			// obj->otyp = POT_WATER;
			// used = TRUE;
			// break;
		}

		/* KMH -- Water into acid causes an explosion */
		if (obj->otyp == POT_ACID || (obj->otyp == POT_BLOOD && acidic(&mons[obj->corpsenm]))) {
			pline("It boils vigorously!");
			You("are caught in the explosion!");
			losehp(Acid_resistance ? rnd(5) : rnd(10),
			       "elementary chemistry", KILLED_BY);
			if (amnesia) {
			    You_feel("a momentary lapse of reason!");
			    forget(2 + rn2(3));
			}
			makeknown(obj->otyp);
			used = TRUE;
			break;
		}
		if (amnesia)
		    pline("%s %s completely.", Your_buf, aobjnam(obj,"dilute"));
		else
		    pline("%s %s%s.", Your_buf, aobjnam(obj,"dilute"),
		      		obj->odiluted ? " further" : "");
		if(obj->unpaid && costly_spot(u.ux, u.uy)) {
		    You("dilute it, you pay for it.");
		    bill_dummy_object(obj);
		}
		if (obj->odiluted || amnesia) {
			obj->odiluted = 0;
#ifdef UNIXPC
			obj->blessed = FALSE;
			obj->cursed = FALSE;
#else
			obj->blessed = obj->cursed = FALSE;
#endif
			if (obj->otyp == POT_STARLIGHT)
				end_burn(obj, FALSE);
			obj->otyp = POT_WATER;
			set_object_color(obj);
		} else obj->odiluted++;
		used = TRUE;
		break;
	    case SCROLL_CLASS:
		if (obj->otyp != SCR_BLANK_PAPER  && !obj->oartifact && obj->otyp != SCR_GOLD_SCROLL_OF_LAW
#ifdef MAIL
		    && obj->otyp != SCR_MAIL
#endif
		    ) {
			if (!Blind) {
				boolean oq1 = obj->quan == 1L;
				pline_The("scroll%s %s.",
					  oq1 ? "" : "s", otense(obj, "fade"));
			}
			if(obj->unpaid && costly_spot(u.ux, u.uy)) {
			    You("erase it, you pay for it.");
			    bill_dummy_object(obj);
			}
			obj->otyp = SCR_BLANK_PAPER;
			remove_oprop(obj, OPROP_TACTB);
			obj->spe = 0;
			obj->oward = 0;
			used = TRUE;
		} 
		break;
	    case SPBOOK_CLASS:
		if (obj->otyp != SPE_BLANK_PAPER) {
			if (obj->otyp == SPE_BOOK_OF_THE_DEAD || obj->oartifact) {
	pline("%s suddenly heats up; steam rises and it remains dry.",
				The(xname(obj)));
			} else {
			    if (!Blind) {
				    boolean oq1 = obj->quan == 1L;
				    pline_The("spellbook%s %s.",
					oq1 ? "" : "s", otense(obj, "fade"));
			    }
			    if(obj->unpaid) {
				subfrombill(obj, shop_keeper(*u.ushops));
			        You("erase it, you pay for it.");
			        bill_dummy_object(obj);
			    }
			    obj->otyp = SPE_BLANK_PAPER;
				obj->obj_color = objects[SPE_BLANK_PAPER].oc_color;
				remove_oprop(obj, OPROP_TACTB);
			}
			used = TRUE;
		}
		break;
	    case GEM_CLASS:
		if (amnesia && (obj->otyp == LUCKSTONE ||
			obj->otyp == LOADSTONE || 
			obj->otyp == TOUCHSTONE))
		    downgrade_obj(obj, FLINT, &used);
		break;
	    case TOOL_CLASS:
		/* Artifacts aren't downgraded by amnesia */
		if (amnesia && !obj->oartifact) {
		    switch (obj->otyp) {
			case MAGIC_LAMP:
			    /* Magic lamps forget their djinn... */
			    downgrade_obj(obj, OIL_LAMP, &used);
			    break;
			case DRUM_OF_EARTHQUAKE:
			    downgrade_obj(obj, DRUM, &used);
			    break;
			case MAGIC_WHISTLE:
			    /* Magic whistles lose their powers... */
			    downgrade_obj(obj, WHISTLE, &used);
			    break;
			case MAGIC_FLUTE:
			    /* Magic flutes sound normal again... */
			    downgrade_obj(obj, FLUTE, &used);
			    break;
			case MAGIC_HARP:
			    /* Magic harps sound normal again... */
			    downgrade_obj(obj, HARP, &used);
			    break;
			case FIRE_HORN:
			case FROST_HORN:
			case HORN_OF_PLENTY:
			    downgrade_obj(obj, TOOLED_HORN, &used);
			    break;
			case MAGIC_MARKER:
			    /* Magic markers run... */
			    if (obj->spe > 0) {
				pre_downgrade_obj(obj, &used);
				if ((obj->spe -= (3 + rn2(10))) < 0) 
				    obj->spe = 0;
			    }
			    break;
		    }
		}

		/* The only other tools that can be affected are pick axes and 
		 * unicorn horns... */
		if (!is_weptool(obj)) break;
		/* Drop through for disenchantment and rusting... */
		/* fall through */
	    case ARMOR_CLASS:
	    case WEAPON_CLASS:
	    case WAND_CLASS:
	    case RING_CLASS:
	    /* Just "fall through" to generic rustprone check for now. */
	    /* fall through */
	    default:
		switch(artifact_wet(obj, FALSE)) {
		    case -1: break;
		    default:
			return TRUE;
		}
		/* !ofAmnesia acts as a disenchanter... */
		if (amnesia && obj->spe > 0) {
		    pre_downgrade_obj(obj, &used);
		    drain_item(obj);
		}
		if (!obj->oerodeproof && is_rustprone(obj) &&
		    (obj->oeroded < MAX_ERODE) && !rn2(2)) {
			pline("%s %s some%s.",
			      Your_buf, aobjnam(obj, "rust"),
			      obj->oeroded ? " more" : "what");
			obj->oeroded++;
			if(obj->unpaid && costly_spot(u.ux, u.uy) && !used) {
			    You("damage it, you pay for it.");
			    bill_dummy_object(obj);
			}
			used = TRUE;
		} 
		break;
	}
	/* !ofAmnesia might strip away fooproofing... */
	if (amnesia && obj->oerodeproof && !rn2(13)) {
	    pre_downgrade_obj(obj, &used);
	    obj->oerodeproof = FALSE;
	}

	/* !ofAmnesia also strips blessed/cursed status... */

	if (amnesia && (obj->cursed || obj->blessed)) {
	    /* Blessed objects are valuable, cursed objects aren't, unless
	     * they're water.
	     */
	    if (obj->blessed || obj->otyp == POT_WATER)
		pre_downgrade_obj(obj, &used);
	    else if (!used) {
		Your("%s for a moment.", aobjnam(obj, "sparkle"));
		used = TRUE;
	    }
	    uncurse(obj);
	    unbless(obj);
	}

	if (used) 
	    update_inventory();
	else {
	    pline("%s %s wet.", Your_buf, aobjnam(obj,"get"));
		if(obj->opoisoned && obj->otyp != VIPERWHIP){
			if(obj->opoisoned&OPOISON_ACID){
				pline("Its acid coating boils vigorously!");
				You("are caught in the explosion!");
				losehp(Acid_resistance ? rnd(2) : rnd(5),
					   "elementary chemistry", KILLED_BY);
				obj->opoisoned &= ~OPOISON_ACID;
			}
			if(obj->opoisoned&(~OPOISON_AMNES)){
				if(obj->opoisoned&OPOISON_BASIC && !rn2(10)){
					pline("The poison coating washes off.");
					obj->opoisoned &= ~OPOISON_BASIC;
				}
				else if(obj->opoisoned&OPOISON_DIRE && !rn2(10)){
					pline("The poison coating washes off.");
					obj->opoisoned &= ~OPOISON_DIRE;
				}
				if(obj->opoisoned&OPOISON_FILTH && !rn2(10)){
					pline("The crusted filth washes off.");
					obj->opoisoned &= ~OPOISON_FILTH;
				}
				if(obj->opoisoned&OPOISON_SLEEP && rn2(2)){
					pline("The drug washes off.");
					obj->opoisoned &= ~OPOISON_SLEEP;
				}
				if(obj->opoisoned&OPOISON_BLIND && rn2(2)){
					pline("The stain washes off.");
					obj->opoisoned &= ~OPOISON_BLIND;
				}
				if(obj->opoisoned&OPOISON_PARAL && rn2(8)){
					pline("The venom washes off.");
					obj->opoisoned &= ~OPOISON_PARAL;
				}
				if(obj->opoisoned&OPOISON_SILVER){
					pline("The silver dew washes off.");
					obj->opoisoned &= ~OPOISON_SILVER;
				}
				if(obj->opoisoned&OPOISON_HALLU && rn2(2)){
					pline("The hallucinogen washes off.");
					obj->opoisoned &= ~OPOISON_HALLU;
				}
			}
		}
	}

	return used;
}

int
dodip()
{
	struct obj *potion, *obj;
	struct obj *singlepotion;
	const char *tmp;
	uchar here;
	char allowall[2], qbuf[BUFSZ+QBUFSZ], Your_buf[BUFSZ];
	short mixture;
//	int res; //unreferenced?  added as part of lethe code?

	allowall[0] = ALL_CLASSES; allowall[1] = '\0';
	if(!(obj = getobj(allowall, "dip")))
		return MOVE_CANCELLED;

	here = levl[u.ux][u.uy].typ;
	/* Is there a fountain to dip into here? */
	if (IS_FOUNTAIN(here)) {
#ifdef PARANOID
		Sprintf(qbuf, "Dip %s into the fountain?", the(xname(obj)));
		if(yn(qbuf) == 'y')
#else
		if(yn("Dip it into the fountain?") == 'y')
#endif
		{
			dipfountain(obj);
			return MOVE_STANDARD;
		}
	}
	if (IS_FORGE(here)) {
#ifdef PARANOID
		Sprintf(qbuf, "Dip %s into the forge?", the(xname(obj)));
		if(yn(qbuf) == 'y')
#else
		if(yn("Dip it into the forge?") == 'y')
#endif
		{
			dipforge(obj);
			return MOVE_STANDARD;
		}
	}
	else if (is_pool(u.ux,u.uy, TRUE)) {
		tmp = waterbody_name(u.ux,u.uy);
#ifdef PARANOID
		Sprintf(qbuf, "Dip %s into the %s?", the(xname(obj)), tmp);
#else
		Sprintf(qbuf, "Dip it into the %s?", tmp);
#endif
		if (yn(qbuf) == 'y') {
		    if (Levitation) {
			floating_above(tmp);
#ifdef STEED
		    } else if (u.usteed && !mon_resistance(u.usteed,SWIMMING) &&
			    P_SKILL(P_RIDING) < P_BASIC) {
			rider_cant_reach(); /* not skilled enough to reach */
#endif
		    } else {
			(void) get_wet(obj, level.flags.lethe); //lethe
//			if (obj->otyp == POT_ACID) useup(obj); //Potential error here
		    }
		    return MOVE_STANDARD;
		}
	}

#ifdef PARANOID
	Sprintf(qbuf, "dip %s into", the(xname(obj)));
	if(!(potion = getobj(beverages, qbuf)))
#else
	if(!(potion = getobj(beverages, "dip into")))
#endif
		return MOVE_CANCELLED;
	if (potion == obj && potion->quan == 1L) {
		pline("That is a potion bottle, not a Klein bottle!");
		return MOVE_CANCELLED;
	}
	//from Slashem, modified
	if(!(potion->otyp == POT_WATER || potion->otyp == POT_ACID || polypotion(potion)) && obj->otyp == POT_WATER) {
	  /* swap roles, to ensure symmetry, but don't if the potion is polymorph or acid */
	  struct obj *otmp = potion;
	  potion = obj;
	  obj = otmp;
	}
	potion->in_use = TRUE;		/* assume it will be used up */
	if (potion->oartifact)
		potion->in_use = FALSE;
	
	if(potion->otyp == POT_WATER) {
		boolean useeit = !Blind;
		if (useeit) (void) Shk_Your(Your_buf, obj);
		if (potion->blessed) {
			if (obj->cursed) {
				if (useeit)
				    pline("%s %s %s.",
					  Your_buf,
					  aobjnam(obj, "softly glow"),
					  hcolor(NH_AMBER));
				uncurse(obj);
				obj->bknown=1;
	poof:
				if(!(objects[potion->otyp].oc_name_known) &&
				   !(objects[potion->otyp].oc_uname))
					docall(potion);
				useup(potion);
				return MOVE_STANDARD;
			} else if(!obj->blessed) {
				if (useeit) {
				    tmp = hcolor(NH_LIGHT_BLUE);
				    pline("%s %s with a%s %s aura.",
					  Your_buf,
					  aobjnam(obj, "softly glow"),
					  index(vowels, *tmp) ? "n" : "", tmp);
				}
				bless(obj);
				obj->bknown=1;
				goto poof;
			}
		} else if (potion->cursed) {
			if (obj->blessed) {
				if (useeit)
				    pline("%s %s %s.",
					  Your_buf,
					  aobjnam(obj, "glow"),
					  hcolor((const char *)"brown"));
				unbless(obj);
				obj->bknown=1;
				goto poof;
			} else if(!obj->cursed) {
				if (useeit) {
				    tmp = hcolor(NH_BLACK);
				    pline("%s %s with a%s %s aura.",
					  Your_buf,
					  aobjnam(obj, "glow"),
					  index(vowels, *tmp) ? "n" : "", tmp);
				}
				curse(obj);
				obj->bknown=1;
				goto poof;
			}
		} else {
			switch(artifact_wet(obj,TRUE)) {
				/* Assume ZT_xxx is AD_xxx-1 */
				case -1: break;
				default:
					zap_over_floor(u.ux, u.uy,
					  artifact_wet(obj,TRUE), WAND_CLASS, FALSE, NULL);
					break;
			}
			if (get_wet(obj, FALSE))
			    goto poof;
		}
	} else if (potion->otyp == POT_AMNESIA) {
	    if (potion == obj) {
			obj->in_use = FALSE;
			potion = splitobj(obj, 1L);
			potion->in_use = TRUE;
	    }
	    if(get_wet(obj, TRUE)){
			if(is_poisonable(obj)){
				if(obj->otyp == VIPERWHIP){
					if(obj->opoisonchrgs && obj->opoisoned == OPOISON_AMNES) obj->opoisonchrgs += 2;
					else obj->opoisonchrgs = 1;
				}
				obj->opoisoned = OPOISON_AMNES;
			}
			goto poof;
		}
	}
	/* WAC - Finn Theoderson - make polymorph and gain level msgs similar
	 * 	 Give out name of new object and allow user to name the potion
	 */
	else if (polypotion(obj) || polypotion(potion))
	{
	    /* some objects can't be polymorphed */
		if ((polypotion(obj) && polypotion(potion)) ||	/* both polypotions */
		    obj->otyp == WAN_POLYMORPH ||
		    obj->otyp == SPE_POLYMORPH ||
		    obj == uball || obj == uskin ||
			obj_resists(polypotion(obj) ?
				potion : obj, 0, 100)) {
			pline1(nothing_happens);
	    } else {
	    	boolean was_wep = FALSE, was_swapwep = FALSE, was_quiver = FALSE;
			short save_otyp = obj->otyp;
			/* KMH, conduct */
			u.uconduct.polypiles++;

			if (obj == uwep) was_wep = TRUE;
			else if (obj == uswapwep) was_swapwep = TRUE;
			else if (obj == uquiver) was_quiver = TRUE;

			obj = randpoly_obj(obj);

#ifndef GOLDOBJ
			if(obj->otyp == GOLD_PIECE){
				u.ugold += 2*obj->quan; //Gold piece handling disaster: useupall reduces the player's gold count by the ammount destroyed, so add the gold twice :(
				useupall(obj);
				obj = (struct obj *) 0;
			}
#endif
			if (was_wep) setuwep(obj);
			else if (was_swapwep) setuswapwep(obj);
			else if (was_quiver) setuqwep(obj);

			if (!obj || obj->otyp != save_otyp || (obj->otyp == HYPOSPRAY_AMPULE && objects[HYPOSPRAY_AMPULE].oc_name_known)) {
				if (save_otyp == POT_POLYMORPH || potion->otyp == POT_POLYMORPH)
					makeknown(POT_POLYMORPH);
				useup(potion);
				if(obj)
					prinv((char *)0, obj, 0L);
				return MOVE_STANDARD;
			} else {
				pline("Nothing seems to happen.");
				goto poof;
			}
	    }
	    potion->in_use = FALSE;	/* didn't go poof */
	    return MOVE_STANDARD;
	} else if(obj->oclass == POTION_CLASS && (obj->otyp != potion->otyp || (obj->otyp == POT_BLOOD && obj->corpsenm != potion->corpsenm))) {
		/* Mixing potions is dangerous... */
		pline_The("potions mix...");
		/* KMH, balance patch -- acid is particularly unstable */
		// Slashem tweak added
		if (obj->cursed || obj->otyp == POT_ACID || 
			(obj->otyp == POT_BLOOD && acidic(&mons[obj->corpsenm])) ||
		    potion->cursed || potion->otyp == POT_ACID || 
			(potion->otyp == POT_BLOOD && acidic(&mons[potion->corpsenm])) || 
			!rn2(10)
		) {
			pline("BOOM!  They explode!");
			exercise(A_STR, FALSE);
			if (!breathless(youracedata) || haseyes(youracedata))
				potionbreathe(obj);
			useup(obj);
			useup(potion);
			/* MRKR: an alchemy smock ought to be */
			/* some protection against this: */
			losehp(Acid_resistance ? rnd(5) : rnd(10),
			       "alchemic blast", KILLED_BY_AN);
			return MOVE_STANDARD;
		}

		obj->blessed = obj->cursed = obj->bknown = 0;
		if (Blind || Hallucination) obj->dknown = 0;


		if ((mixture = mixtype(obj, potion)) != 0) {
			if (obj->otyp == POT_STARLIGHT)
				end_burn(obj, FALSE);
			obj->otyp = mixture;
		} else {
		    switch (obj->odiluted ? 1 : rnd(8)) {
			case 1:
				if (obj->otyp == POT_STARLIGHT)
					end_burn(obj, FALSE);
				obj->otyp = POT_WATER;
				break;
			case 2:
			case 3:
				if (obj->otyp == POT_STARLIGHT)
					end_burn(obj, FALSE);
				obj->otyp = POT_SICKNESS;
				break;
			case 4:
				{
				  if (obj->otyp == POT_STARLIGHT)
				  	end_burn(obj, FALSE);
				  struct obj *otmp;
				  otmp = mkobj(POTION_CLASS,FALSE);
				  obj->otyp = otmp->otyp;
				  fix_object(obj);
				  obfree(otmp, (struct obj *)0);
				}
				break;
			default:
				if (!Blind)
			  pline_The("mixture glows brightly and evaporates.");
				useup(obj);
				useup(potion);
				return MOVE_STANDARD;
		    }
		}

		obj->odiluted = (obj->otyp != POT_WATER);

		if (obj->otyp == POT_WATER && !Hallucination) {
			pline_The("mixture bubbles%s.",
				Blind ? "" : ", then clears");
		} else if (!Blind) {
			pline_The("mixture looks %s.",
				hcolor(OBJ_DESCR(objects[obj->otyp])));
		}
		set_object_color(obj);
		useup(potion);
		return MOVE_STANDARD;
	}

#ifdef INVISIBLE_OBJECTS
	if (potion->otyp == POT_INVISIBILITY && !obj->oinvis) {
		obj->oinvis = TRUE;
		if (!Blind) {
		    if (!See_invisible(u.ux,u.uy)) pline("Where did %s go?",
		    		the(xname(obj)));
		    else You("notice a little haziness around %s.",
		    		the(xname(obj)));
		}
		goto poof;
	} else if (potion->otyp == POT_SEE_INVISIBLE && obj->oinvis) {
		obj->oinvis = FALSE;
		if (!Blind) {
		    if (!See_invisible(u.ux,u.uy)) pline("So that's where %s went!",
		    		the(xname(obj)));
		    else pline_The("haziness around %s disappears.",
		    		the(xname(obj)));
		}
		goto poof;
	}
#endif
	if(potion->otyp == POT_MIDAS && obj->obj_material != GOLD){
		pline("%s %s into gold.", The(xname(obj)), obj->quan != 1 ? "turn" : "turns");
		set_material(obj, GOLD);
		goto poof;
	}
	
	if( (potion->otyp == POT_ACID || 
			(potion->otyp == POT_BLOOD && acidic(&mons[potion->corpsenm]))) 
		&& (!(obj->opoisoned & OPOISON_ACID) || obj->otyp == VIPERWHIP)
	){
		if(is_corrodeable(obj) && !obj->oerodeproof && obj->oeroded2 < MAX_ERODE){
			int poofit = 1;
			if(obj->greased)
				poofit = 0;
			erode_obj(obj, TRUE, FALSE);
			if(poofit)
				goto poof;
		} else if(is_poisonable(obj)){
			char buf[BUFSZ];
			if (potion->quan > 1L)
				Sprintf(buf, "One of %s", the(xname(potion)));
			else
				Strcpy(buf, The(xname(potion)));
			if(obj->otyp != VIPERWHIP) obj->opoisoned = 0;
			if(obj->otyp == VIPERWHIP) pline("%s is drawn up into %s.",
				  buf, the(xname(obj)));
			else pline("%s forms a coating on %s.",
				  buf, the(xname(obj)));
			if(obj->otyp == VIPERWHIP){
				if(obj->opoisonchrgs && obj->opoisoned == OPOISON_ACID) obj->opoisonchrgs += 2;
				else obj->opoisonchrgs = 1;
			}
			obj->opoisoned = OPOISON_ACID;
			goto poof;
		}
	}
	
	if(is_poisonable(obj)) {
	    if( (potion->otyp == POT_SICKNESS || 
				(potion->otyp == POT_BLOOD && poisonous(&mons[potion->corpsenm]))) 
			&& (!(obj->opoisoned & OPOISON_BASIC || arti_poisoned(obj))
	    		|| obj->otyp == VIPERWHIP)
		){
			char buf[BUFSZ];
			if (potion->quan > 1L)
				Sprintf(buf, "One of %s", the(xname(potion)));
			else
				Strcpy(buf, The(xname(potion)));
			if(obj->otyp != VIPERWHIP) obj->opoisoned = 0;
			if(obj->otyp == VIPERWHIP) pline("%s is drawn up into %s.",
				  buf, the(xname(obj)));
			else pline("%s forms a coating on %s.",
				  buf, the(xname(obj)));
			if(obj->otyp == VIPERWHIP){
				if(obj->opoisonchrgs && obj->opoisoned == OPOISON_BASIC) obj->opoisonchrgs += 2;
				else obj->opoisonchrgs = 1;
			}
			obj->opoisoned = OPOISON_BASIC;
			goto poof;
	    } else if(potion->otyp == POT_SLEEPING && (!(obj->opoisoned & OPOISON_SLEEP) || obj->otyp == VIPERWHIP)) {
			char buf[BUFSZ];
			if (potion->quan > 1L)
				Sprintf(buf, "One of %s", the(xname(potion)));
			else
				Strcpy(buf, The(xname(potion)));
			obj->opoisoned = 0;
			if(obj->otyp != VIPERWHIP) obj->opoisoned = 0;
			if(obj->otyp == VIPERWHIP) pline("%s is drawn up into %s.",
				  buf, the(xname(obj)));
			else pline("%s forms a drug-coating on %s.",
				  buf, the(xname(obj)));
			if(obj->otyp == VIPERWHIP){
				if(obj->opoisonchrgs && obj->opoisoned == OPOISON_SLEEP) obj->opoisonchrgs += 2;
				else obj->opoisonchrgs = 1;
			}
			obj->opoisoned = OPOISON_SLEEP;
			goto poof;
	    } else if(potion->otyp == POT_BLINDNESS && (!(obj->opoisoned & OPOISON_BLIND) || obj->otyp == VIPERWHIP)) {
			char buf[BUFSZ];
			if (potion->quan > 1L)
				Sprintf(buf, "One of %s", the(xname(potion)));
			else
				Strcpy(buf, The(xname(potion)));
			obj->opoisoned = 0;
			if(obj->otyp != VIPERWHIP) obj->opoisoned = 0;
			if(obj->otyp == VIPERWHIP) pline("%s is drawn up into %s.",
				  buf, the(xname(obj)));
			else pline("%s forms a blinding coating on %s.",
				  buf, the(xname(obj)));
			if(obj->otyp == VIPERWHIP){
				if(obj->opoisonchrgs && obj->opoisoned == OPOISON_BLIND) obj->opoisonchrgs += 2;
				else obj->opoisonchrgs = 1;
			}
			obj->opoisoned = OPOISON_BLIND;
			goto poof;
	    } else if(potion->otyp == POT_PARALYSIS && (!(obj->opoisoned & OPOISON_PARAL) || obj->otyp == VIPERWHIP)) {
			char buf[BUFSZ];
			if (potion->quan > 1L)
				Sprintf(buf, "One of %s", the(xname(potion)));
			else
				Strcpy(buf, The(xname(potion)));
			obj->opoisoned = 0;
			if(obj->otyp != VIPERWHIP) obj->opoisoned = 0;
			if(obj->otyp == VIPERWHIP) pline("%s is drawn up into %s.",
				  buf, the(xname(obj)));
			else pline("%s forms a paralyzing coating on %s.",
				  buf, the(xname(obj)));
			if(obj->otyp == VIPERWHIP){
				if(obj->opoisonchrgs && obj->opoisoned == OPOISON_PARAL) obj->opoisonchrgs += 2;
				else obj->opoisonchrgs = 1;
			}
			obj->opoisoned = OPOISON_PARAL;
			goto poof;
	    } else if(potion->otyp == POT_STARLIGHT && obj->obj_material != SILVER &&
	    		(!(obj->opoisoned & OPOISON_SILVER)
	    		|| obj->otyp == VIPERWHIP)
	    	) {
			char buf[BUFSZ];
			if (potion->quan > 1L)
				Sprintf(buf, "One of %s", the(xname(potion)));
			else
				Strcpy(buf, The(xname(potion)));
			obj->opoisoned = 0;
			if(obj->otyp != VIPERWHIP) obj->opoisoned = 0;
			if(obj->otyp == VIPERWHIP) pline("%s is drawn up into %s.",
				  buf, the(xname(obj)));
			else pline("%s forms a silvery coating on %s.",
				  buf, the(xname(obj)));
			if(obj->otyp == VIPERWHIP){
				if(obj->opoisonchrgs && obj->opoisoned == OPOISON_SILVER) obj->opoisonchrgs += 2;
				else obj->opoisonchrgs = 1;
			}
			obj->opoisoned = OPOISON_SILVER;
			goto poof;
	    } else if(potion->otyp == POT_HALLUCINATION && (!(obj->opoisoned & OPOISON_HALLU) || obj->otyp == VIPERWHIP)) {
			char buf[BUFSZ];
			if (potion->quan > 1L)
				Sprintf(buf, "One of %s", the(xname(potion)));
			else
				Strcpy(buf, The(xname(potion)));
			obj->opoisoned = 0;
			if(obj->otyp != VIPERWHIP) obj->opoisoned = 0;
			if(obj->otyp == VIPERWHIP) pline("%s is drawn up into %s.",
				  buf, the(xname(obj)));
			else pline("%s forms a hallucinogenic coating on %s.",
				  buf, the(xname(obj)));
			if(obj->otyp == VIPERWHIP){
				if(obj->opoisonchrgs && obj->opoisoned == OPOISON_HALLU) obj->opoisonchrgs += 2;
				else obj->opoisonchrgs = 1;
			}
			obj->opoisoned = OPOISON_HALLU;
			goto poof;
	    } else if(obj->opoisoned &&
			  (potion->otyp == POT_HEALING ||
			   potion->otyp == POT_EXTRA_HEALING ||
			   potion->otyp == POT_FULL_HEALING)) {
			pline("A coating wears off %s.", the(xname(obj)));
			obj->opoisoned = 0;
			goto poof;
	    }
	}
	if(isSignetRing(obj->otyp)) {
	    if( (potion->otyp == POT_SICKNESS ||
				(potion->otyp == POT_BLOOD && poisonous(&mons[potion->corpsenm]))
			) && (!obj->opoisoned || obj->opoisoned & OPOISON_BASIC)){
			char buf[BUFSZ];
			if (potion->quan > 1L)
				Sprintf(buf, "One of %s", the(xname(potion)));
			else
				Strcpy(buf, The(xname(potion)));
			pline("%s is drawn up into %s.",
				  buf, the(xname(obj)));
			obj->opoisoned = OPOISON_BASIC;
			obj->opoisonchrgs = 30;
			goto poof;
	    } else if(potion->otyp == POT_SLEEPING && (!obj->opoisoned || obj->opoisoned & OPOISON_SLEEP)){
			char buf[BUFSZ];
			if (potion->quan > 1L)
				Sprintf(buf, "One of %s", the(xname(potion)));
			else
				Strcpy(buf, The(xname(potion)));
			pline("%s is drawn up into %s.",
				  buf, the(xname(obj)));
			obj->opoisoned = OPOISON_SLEEP;
			obj->opoisonchrgs = 30;
			goto poof;
	    } else if(potion->otyp == POT_BLINDNESS && (!obj->opoisoned || obj->opoisoned & OPOISON_BLIND)) {
			char buf[BUFSZ];
			if (potion->quan > 1L)
				Sprintf(buf, "One of %s", the(xname(potion)));
			else
				Strcpy(buf, The(xname(potion)));
			pline("%s is drawn up into %s.",
				  buf, the(xname(obj)));
			obj->opoisoned = OPOISON_BLIND;
			obj->opoisonchrgs = 30;
			goto poof;
	    } else if(potion->otyp == POT_PARALYSIS && (!obj->opoisoned || obj->opoisoned & OPOISON_PARAL)) {
			char buf[BUFSZ];
			if (potion->quan > 1L)
				Sprintf(buf, "One of %s", the(xname(potion)));
			else
				Strcpy(buf, The(xname(potion)));
			obj->opoisoned = 0;
			pline("%s is drawn up into %s.",
				  buf, the(xname(obj)));
			obj->opoisoned = OPOISON_PARAL;
			obj->opoisonchrgs = 30;
			goto poof;
	    } else if(potion->otyp == POT_HALLUCINATION && (!obj->opoisoned || obj->opoisoned & OPOISON_HALLU)) {
			char buf[BUFSZ];
			if (potion->quan > 1L)
				Sprintf(buf, "One of %s", the(xname(potion)));
			else
				Strcpy(buf, The(xname(potion)));
			obj->opoisoned = 0;
			pline("%s is drawn up into %s.",
				  buf, the(xname(obj)));
			obj->opoisoned = OPOISON_HALLU;
			obj->opoisonchrgs = 30;
			goto poof;
	    } else if((potion->otyp == POT_ACID ||
				(potion->otyp == POT_BLOOD && acidic(&mons[potion->corpsenm]))
			) && (!obj->opoisoned || obj->opoisoned & OPOISON_ACID)) {
			char buf[BUFSZ];
			if (potion->quan > 1L)
				Sprintf(buf, "One of %s", the(xname(potion)));
			else
				Strcpy(buf, The(xname(potion)));
			obj->opoisoned = 0;
			pline("%s is drawn up into %s.",
				  buf, the(xname(obj)));
			obj->opoisoned = OPOISON_ACID;
			obj->opoisonchrgs = 30;
			goto poof;
	    }
	}

	if (potion->otyp == POT_OIL) {
	    boolean wisx = FALSE;
	    if((obj->lamplit || potion->lamplit) 
			&& (obj->otyp == TORCH || obj->otyp == SHADOWLANDER_S_TORCH)
		) {
			// Note: magic torches don't burn items (magic. The torch itself is not burned, and it doesn't burn other things. It does burn creatures though!)
			if(!obj->lamplit)
				begin_burn(obj);
			//Note: done in THIS ORDER SPECIFICALLY to avoid the explosion using up the potion!
			// Requires duplicating the below useup and return here :(
			makeknown(potion->otyp);
			useup(potion);
			explode(u.ux, u.uy, AD_FIRE, 0, d(6,6), EXPL_FIERY, 1);
			exercise(A_WIS, FALSE);
			return MOVE_STANDARD;
	    } else if (potion->lamplit) {	/* burning */
			int omat = obj->obj_material;
			/* the code here should be merged with fire_damage */
			if (catch_lit(obj)) {
				/* catch_lit does all the work if true */
			} else if (obj->oerodeproof || obj_resists(obj, 5, 95) ||
					(obj->oeroded == MAX_ERODE && obj->oartifact) ||
				   !is_flammable(obj) || obj->oclass == FOOD_CLASS) {
				pline("%s %s to burn for a moment.",
				  Yname2(obj), otense(obj, "seem"));
			} else {
				if ((omat == PLASTIC || omat == PAPER) && !obj->oartifact)
				obj->oeroded = MAX_ERODE;
				pline_The("burning oil %s %s.",
					obj->oeroded == MAX_ERODE ? "destroys" : "damages",
					yname(obj));
				if (obj->oeroded == MAX_ERODE) {
				setnotworn(obj);
				obj_extract_self(obj);
				obfree(obj, (struct obj *)0);
				obj = (struct obj *) 0;
				} else {
				/* we know it's carried */
				if (obj->unpaid) {
					/* create a dummy duplicate to put on bill */
					verbalize("You burnt it, you bought it!");
					bill_dummy_object(obj);
				}
				obj->oeroded++;
				}
			}
	    } else if (potion->cursed) {
			pline_The("potion spills and covers your %s with oil.",
				  makeplural(body_part(FINGER)));
			incr_itimeout(&Glib, d(2,10));
	    } else if (obj->otyp == CLUB) {
			You("make a torch from your club and the oil.");
			obj->otyp = TORCH;
			obj->oclass = TOOL_CLASS;
			obj->age = (long)rn1(500, 1000);
			if (obj->oeroded || obj->oerodeproof || !is_flammable(obj)) {
				obj->age /= (1 << max(obj->oeroded, 4 * (obj->oerodeproof || !is_flammable(obj))));
				if (is_flammable(obj)) {
					obj->oeroded = 0;
					obj->oerodeproof = 0;
				}
			}
			if(potion->dknown && !objects[potion->otyp].oc_name_known)
				makeknown(potion->otyp);
			goto poof;
		/* Allow filling of MAGIC_LAMPs to prevent identification by player */
		/*  This is another one that can't use the general handling below and so has early returns */
		} else if ((obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP)) {
			/* Turn off engine before fueling, turn off fuel too :-)  */
			if (obj->lamplit || potion->lamplit) {
				useup(potion);
				explode(u.ux, u.uy, AD_FIRE, 0, d(6,6), EXPL_FIERY, 1);
				exercise(A_WIS, FALSE);
				return MOVE_STANDARD;
			}
			/* Adding oil to an empty magic lamp renders it into an oil lamp */
			if ((obj->otyp == MAGIC_LAMP) && obj->spe == 0) {
				obj->otyp = OIL_LAMP;
				obj->age = 0;
			}
			if (obj->age > 1000L) {
				pline("%s %s full.", Yname2(obj), otense(obj, "are"));
				potion->in_use = FALSE;	/* didn't go poof */
			} else {
				You("fill %s with oil.", yname(obj));
				check_unpaid(potion);	/* Yendorian Fuel Tax */
				obj->age += 2*potion->age;	/* burns more efficiently */
				if (obj->age > 1500L) obj->age = 1500L;
				useup(potion);
				exercise(A_WIS, TRUE);
			}
			makeknown(POT_OIL);
			obj->spe = 1;
			update_inventory();
			return MOVE_STANDARD;
	    } else if (is_rustprone(obj) && obj->oeroded > 0){
			pline("%s %s less rusty.", Yname2(obj), otense(obj, "are"));
			obj->oeroded--;
			wisx = TRUE;
	    } else if (is_corrodeable(obj) && obj->oeroded2 > 0){
			pline("%s %s less corroded.", Yname2(obj), otense(obj, "are"));
			obj->oeroded2--;
			wisx = TRUE;
	    } else {
			/* uses up potion */
			pline("%s %s with an oily sheen.",
				  Yname2(obj), otense(obj, "gleam"));
			if(!obj->greased){
				wisx = TRUE;
				obj->greased = TRUE;
			}
	    }
	    exercise(A_WIS, wisx);
	    makeknown(potion->otyp);
	    useup(potion);
	    return MOVE_STANDARD;
	}
    more_dips:

	if((obj->otyp == SUNROD)
		&& (potion->otyp == POT_ACID)
	) {
		obj->age = max(obj->age-100, 1);
		if(!obj->lamplit)
			begin_burn(obj);
		else {
			//May change radius, snuff and relight
			end_burn(obj, TRUE);
			begin_burn(obj);
		}
		useup(potion);
		if(!Shock_resistance){
			losehp(d(3,6) + 3*obj->spe, "discharging sunrod", KILLED_BY_AN);
		}
		if(!UseInvShock_res(&youmonst)){
			if (!rn2(3)) destroy_item(&youmonst, WAND_CLASS, AD_ELEC);
			if (!rn2(3)) destroy_item(&youmonst, RING_CLASS, AD_ELEC);
		}
		losehp(Acid_resistance ? rnd(5) : rnd(10),
			   "alchemic blast", KILLED_BY_AN);
		if (!resists_blnd(&youmonst)) {
			You("are blinded by the flash!");
			make_blinded((long)d(1,50),FALSE);
			if (!Blind) Your1(vision_clears);
		}
		exercise(A_WIS, FALSE);
		return MOVE_STANDARD;
	}
	
	potion->in_use = FALSE;		/* didn't go poof */
	if ((obj->otyp == UNICORN_HORN || (obj->oclass == GEM_CLASS && !obj->oartifact))
		&& !potion->oartifact
	    && (mixture = mixtype(obj, potion)) != 0
	) {
		char oldbuf[BUFSZ], newbuf[BUFSZ];
		short old_otyp = potion->otyp;
		boolean old_dknown = FALSE;
		boolean more_than_one = potion->quan > 1;

		oldbuf[0] = '\0';
		if (potion->dknown) {
		    old_dknown = TRUE;
		    Sprintf(oldbuf, "%s ",
			    hcolor(OBJ_DESCR(objects[potion->otyp])));
		}
		/* with multiple merged potions, split off one and
		   just clear it */
		if (potion->quan > 1L) {
		    singlepotion = splitobj(potion, 1L);
		} else singlepotion = potion;

		/* MRKR: Gems dissolve in acid to produce new potions */

		if (obj->oclass == GEM_CLASS && potion->otyp == POT_ACID) {

		  struct obj *singlegem = (obj->quan > 1L ? 
					   splitobj(obj, 1L) : obj);

		  if (potion->otyp == POT_ACID && 
		      (obj->otyp == DILITHIUM_CRYSTAL || 
		       potion->cursed || !rn2(10))) {
		    /* Just to keep them on their toes */
			
		    if (Hallucination && obj->otyp == DILITHIUM_CRYSTAL) {
		      /* Thanks to Robin Johnson */
		      pline("Warning, Captain! The warp core has been breached!");
		    }
		    pline("BOOM! %s explodes!", The(xname(singlegem)));
			if(obj->otyp == DILITHIUM_CRYSTAL) tele();
		    exercise(A_STR, FALSE);
		    if (!breathless(youracedata) || haseyes(youracedata))
		      potionbreathe(singlepotion);
		    useup(singlegem);
		    useup(singlepotion);
		    /* MRKR: an alchemy smock ought to be */
		    /* some protection against this: */
		    losehp(Acid_resistance ? rnd(5) : rnd(10), 
			   "alchemic blast", KILLED_BY_AN);
		    return MOVE_STANDARD;	  
		  }
		
		  pline("%s dissolves in %s.", The(xname(singlegem)), 
			the(xname(singlepotion)));
		  makeknown(POT_ACID);
		  useup(singlegem);
		}
		
		if(singlepotion->unpaid && costly_spot(u.ux, u.uy)) {
		    You("use it, you pay for it.");
		    bill_dummy_object(singlepotion);
		}
		
		if (singlepotion->otyp == mixture) {		  
		  /* no change - merge it back in */
		  if (more_than_one && !merged(&potion, &singlepotion)) {
		    /* should never happen */
		    impossible("singlepotion won't merge with parent potion.");
		  }
		}
		else {
  		singlepotion->otyp = mixture;
		singlepotion->blessed = 0;
		if (mixture == POT_WATER)
		    singlepotion->cursed = singlepotion->odiluted = 0;
		else
		    singlepotion->cursed = obj->cursed;  /* odiluted left as-is */
		singlepotion->bknown = FALSE;
		if (Blind) {
		    singlepotion->dknown = FALSE;
		} else {
		    singlepotion->dknown = !Hallucination;
		    if (mixture == POT_WATER && singlepotion->dknown)
			Sprintf(newbuf, "clears");
		    else
			Sprintf(newbuf, "turns %s",
				hcolor(OBJ_DESCR(objects[mixture])));
		    pline_The("%spotion%s %s.", oldbuf,
			      more_than_one ? " that you dipped into" : "",
			      newbuf);
		    if(!objects[old_otyp].oc_uname &&
			!objects[old_otyp].oc_name_known && old_dknown) {
			struct obj * fakeobj;
			fakeobj = mksobj(old_otyp, MKOBJ_NOINIT);
			fakeobj->dknown = 1;
			docall(fakeobj);
			delobj(fakeobj);
		    }
		}
		obj_extract_self(singlepotion);
		singlepotion = hold_another_object(singlepotion,
					"You juggle and drop %s!",
					doname(singlepotion), (const char *)0);
		update_inventory();
		}
		return MOVE_STANDARD;
	}

	pline("Interesting...");
	return MOVE_STANDARD;
}


void
djinni_from_bottle(obj)
register struct obj *obj;
{
	struct monst *mtmp;
	struct monst *mtmp2 = (struct monst*)0;
	struct monst *mtmp3 = (struct monst*)0;

	int chance;

	if(!(mtmp = makemon(&mons[PM_DJINNI], u.ux, u.uy, NO_MM_FLAGS))){
		pline("It turns out to be empty.");
		return;
	}

	if (!Blind) {
		pline("In a cloud of smoke, %s emerges!", a_monnam(mtmp));
		pline("%s speaks.", Monnam(mtmp));
	} else {
		You("smell acrid fumes.");
		pline("%s speaks.", Something);
	}

	chance = rn2(5);
	if (obj->blessed) chance = (chance == 4) ? rnd(4) : 0;
	else if (obj->cursed) chance = (chance == 0) ? rn2(4) : 4;
	/* 0,1,2,3,4:  b=80%,5,5,5,5; nc=20%,20,20,20,20; c=5%,5,5,5,80 */

	switch (chance) {
	case 0 : verbalize("I am in your debt.  I will grant one wish!");
		if ((u.uevent.utook_castle & ARTWISH_EARNED) && !(u.uevent.utook_castle & ARTWISH_SPENT))
			mtmp2 = makemon(&mons[PM_PSYCHOPOMP], u.ux, u.uy, NO_MM_FLAGS);
		if ((u.uevent.uunknowngod & ARTWISH_EARNED) && !(u.uevent.uunknowngod & ARTWISH_SPENT))
			mtmp3 = makemon(&mons[PM_PRIEST_OF_AN_UNKNOWN_GOD], u.ux, u.uy, NO_MM_FLAGS);
		if ((mtmp2 && canseemon(mtmp2)) || (mtmp3 && canseemon(mtmp3))) {
			You("see %s%s%s appear at the djinni's words.",
				(mtmp2 && canseemon(mtmp2)) ? a_monnam(mtmp2) : "",
				((mtmp2 && canseemon(mtmp2)) && (mtmp3 && canseemon(mtmp3))) ? " and " : "",
				(mtmp3 && canseemon(mtmp3)) ? a_monnam(mtmp3) : "");
		}
		int artwishes = u.uconduct.wisharti;
		makewish(allow_artwish()|WISH_VERBOSE);
		if (u.uconduct.wisharti > artwishes) {
			/* made artifact wish */
			if (mtmp2) {
				pline("You feel %s presence fade.", s_suffix(mon_nam(mtmp2)));
				mongone(mtmp2);
				mtmp2 = (struct monst*) 0;
				u.uevent.utook_castle |= ARTWISH_SPENT;
			}
			else if (mtmp3) {
				pline("You feel %s presence fade.", s_suffix(mon_nam(mtmp3)));
				mongone(mtmp3);
				mtmp3 = (struct monst*) 0;
				u.uevent.uunknowngod |= ARTWISH_SPENT;
			}
		}
		pline("The djinni%s disappears with a puff of smoke.", (mtmp2 || mtmp3) ? " and their entourage" : "");
		mongone(mtmp);
		if (mtmp2)	mongone(mtmp2);
		if (mtmp3)	mongone(mtmp3);
		break;
	case 1 : verbalize("Thank you for freeing me!");
		(void) tamedog(mtmp, (struct obj *)0);
		break;
	case 2 : verbalize("You freed me!");
		mtmp->mpeaceful = TRUE;
		set_malign(mtmp);
		break;
	case 3 : verbalize("It is about time!");
		pline("%s vanishes.", Monnam(mtmp));
		mongone(mtmp);
		break;
	default: verbalize("You disturbed me, fool!");
		break;
	}
}

/* clone a gremlin or mold (2nd arg non-null implies heat as the trigger);
   hit points are cut in half (odd HP stays with original) */
struct monst *
split_mon(mon, mtmp)
struct monst *mon,	/* monster being split */
	     *mtmp;	/* optional attacker whose heat triggered it */
{
	struct monst *mtmp2;
	char reason[BUFSZ];

	reason[0] = '\0';
	if (mtmp) Sprintf(reason, " from %s heat",
			  (mtmp == &youmonst) ? (const char *)"your" :
			      (const char *)s_suffix(mon_nam(mtmp)));

	if (mon == &youmonst) {
	    mtmp2 = cloneu();
	    if (mtmp2) {
		mtmp2->mhpmax = u.mhmax / 2;
		u.mhmax -= mtmp2->mhpmax;
		flags.botl = 1;
		You("multiply%s!", reason);
	    }
	} else {
	    mtmp2 = clone_mon(mon, 0, 0);
	    if (mtmp2) {
		mtmp2->mhpmax = mon->mhpmax / 2;
		mon->mhpmax -= mtmp2->mhpmax;
		if (canspotmon(mon))
		    pline("%s multiplies%s!", Monnam(mon), reason);
	    }
	}
	return mtmp2;
}

#endif /* OVLB */

/*potion.c*/
