#include "hack.h"
#include "artifact.h"
#include "monflag.h"

#include "xhity.h"

extern int monstr[];

STATIC_DCL void FDECL(wildmiss, (struct monst *, struct attack *, struct obj *, boolean));
STATIC_DCL boolean FDECL(u_surprise, (struct monst *, boolean));
STATIC_DCL struct attack * FDECL(getnextspiritattack, (boolean));
STATIC_DCL void FDECL(xswingsy, (struct monst *, struct monst *, struct obj *, boolean));
STATIC_DCL void FDECL(xyhitmsg, (struct monst *, struct monst *, struct attack *));
STATIC_DCL void FDECL(noises, (struct monst *, struct attack *));
STATIC_DCL void FDECL(xymissmsg, (struct monst *, struct monst *, struct attack *, int, boolean));
STATIC_DCL int FDECL(do_weapon_multistriking_effects, (struct monst *, struct monst *, struct attack *, struct obj *, int));
STATIC_DCL int FDECL(xcastmagicy, (struct monst *, struct monst *, struct attack *, int, int));
STATIC_DCL int FDECL(xtinkery, (struct monst *, struct monst *, struct attack *, int));
STATIC_DCL int FDECL(xexplodey, (struct monst *, struct monst *, struct attack *, int));
STATIC_DCL int FDECL(hmoncore, (struct monst *, struct monst *, struct attack *, struct attack *, struct obj **, void *, int, int, int, boolean, int, boolean, int, unsigned long));
STATIC_DCL void FDECL(add_silvered_art_sear_adjectives, (char *, struct obj*));
STATIC_DCL int FDECL(shadow_strike, (struct monst *));
STATIC_DCL int FDECL(xpassivehity, (struct monst *, struct monst *, struct attack *, struct attack *, struct obj *, int, int, struct permonst *, boolean));

/* for long worms */
extern boolean notonhead;

/* to make acurr() use a different value for your STR score in specific circumstances */
extern int override_str;

/* global to help tell when something is making a sneak attack with the Lifehunt Scythe against a vulnerable target */
boolean lifehunt_sneak_attacking = FALSE;

/* Counterattack chance at skill level....  B:  S:  E:  */
static const int DjemSo_counterattack[] = {  5, 10, 20 };
static const int Shien_counterattack[]  = {  5, 10, 20 };
static const int Soresu_counterattack[] = { 10, 15, 25 };
/* Misc attacks */
const struct attack noattack = { 0, 0, 0, 0 };
struct attack basicattack  = { AT_WEAP, AD_PHYS, 1, 4 };
struct attack grapple = { AT_HUGS, AD_PHYS, 0, 6 };	/* for grappler's grasp */

int
check_subout(subout_list, subout)
int *subout_list;
int subout;
{
	if(subout >= MAX_SUBOUT || subout < 1){
		impossible("Attempting to check subout number %d?", subout);
		return FALSE;
	}
	return !!(subout_list[(subout-1)/16] & (0x1L << ((subout-1)%16)));
}

void
add_subout(subout_list, subout)
int *subout_list;
int subout;
{
	if(subout >= MAX_SUBOUT || subout < 1){
		impossible("Attempting to set subout number %d?", subout);
	}
	subout_list[(subout-1)/16] |= (0x1L << ((subout-1)%16));
}

void
remove_subout(subout_list, subout)
int *subout_list;
int subout;
{
	if(subout >= MAX_SUBOUT || subout < 1){
		impossible("Attempting to set subout number %d?", subout);
	}
	subout_list[(subout-1)/16] &= ~(0x1L << ((subout-1)%16));
}

void
zero_subout(subout_list)
int *subout_list;
{
	for(int i = 0; i < SUBOUT_ARRAY_SIZE; i++)
		subout_list[i] = 0;
}


/* getvis()
 * 
 * determines vis if needed.
 */
int
getvis(magr, mdef, tarx, tary)
struct monst * magr;
struct monst * mdef;
int tarx;
int tary;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	int vis = 0;
	
	if (!tarx && !tary) {
		tarx = x(mdef);
		tary = y(mdef);
	}

	if (youagr || youdef || (magr && cansee(x(magr), y(magr))) || cansee(tarx, tary))
	{
		if (youagr || (magr && cansee(x(magr), y(magr)) && canseemon(magr)))
			vis |= VIS_MAGR;
		if (youdef || (cansee(tarx, tary) && canseemon(mdef)))
			vis |= VIS_MDEF;
		if (youagr || youdef || (magr && canspotmon(magr)) || canspotmon(mdef))
			vis |= VIS_NONE;
	}
	else
		vis = 0;
	return vis;
}

/* attack2()
 * 
 * The bump attack function!
 * Player-only; u.dx and u.dy must already be set.
 * Returns FALSE if we moved to where the monster was.
 * TODO: remove old attack() function, rename this one
 */
boolean
attack2(mdef)
struct monst * mdef;
{
	int result;
	int attack_type;	/* uses ATTACKCHECKS defines */
	register struct permonst *pd = mdef->data;

	/* This section of code provides protection against accidentally
	 * hitting peaceful (like '@') and tame (like 'd') monsters.
	 * Protection is provided as long as player is not: blind, confused,
	 * hallucinating or stunned.
	 * changes by wwp 5/16/85
	 * More changes 12/90, -dkh-. if its tame and safepet, (and protected
	 * 07/92) then we assume that you're not trying to attack. Instead,
	 * you'll usually just swap places if this is a movement command
	 */
	/* Intelligent chaotic weapons (Stormbringer) want blood */
	if (is_safepet(mdef) && !flags.forcefight) {
		/* there are some additional considerations: this won't work
		* if in a shop or Punished or you miss a random roll or
		* if you can walk thru walls and your pet cannot (KAA) or
		* if your pet is a long worm (unless someone does better).
		* there's also a chance of displacing a "frozen" monster.
		* sleeping monsters might magically walk in their sleep.
		*/
		boolean foo = (Punished || !rn2(7) || is_longworm(mdef->data)),
			inshop = FALSE;
		char *p;

		for (p = in_rooms(x(mdef), y(mdef), SHOPBASE); *p; p++)
		if (tended_shop(&rooms[*p - ROOMOFFSET])) {
			inshop = TRUE;
			break;
		}

		if (inshop || foo ||
			(IS_ROCK(levl[u.ux][u.uy].typ) &&
			!mon_resistance(mdef, PASSES_WALLS))) {
			char buf[BUFSZ];

			monflee(mdef, rnd(6), FALSE, FALSE);
			Strcpy(buf, y_monnam(mdef));
			buf[0] = highc(buf[0]);
			You("stop.  %s is in the way!", buf);
			return(TRUE);
		}
		else if ((mdef->mfrozen || (!mdef->mcanmove)
			|| (mdef->data->mmove == 0)) && rn2(6)) {
			pline("%s doesn't seem to move!", Monnam(mdef));
			return(TRUE);
		}
		else return(FALSE);
	}

	/* attack_checks returns a truthy value if we can attack */
	if (!(attack_type = attack_checks(mdef, uwep))) {
		return TRUE;
	}
	//If the player can't attack and is wearing a straitjacket, redirect to the kick function
	if(Straitjacketed && !at_least_one_attack(&youmonst)){
		dokick_core(u.dx, u.dy);
		//Dokick can't move you
		return TRUE;
	}
	if (u.twoweap && !test_twoweapon())
		untwoweapon();

	if (unweapon) {
		unweapon = FALSE;
		if (flags.verbose) {
			if (uwep){
				if (uwep->oartifact == ART_LIECLEAVER)
					You("begin slashing monsters with your %s.", aobjnam(uwep, (char *)0));
				else if (uwep->otyp == CARCOSAN_STING || uwep->otyp == TWINGUN_SHANTA)
					You("begin stabbing monsters with your %s.", aobjnam(uwep, (char *)0));
				else if (uwep->otyp == SOLDIER_S_SABER || uwep->otyp == BLADED_BOW)
					You("begin slashing monsters with your %s.", aobjnam(uwep, (char *)0));
				else You("begin bashing monsters with your %s.",
					aobjnam(uwep, (char *)0));
			}
			else if (!you_cantwield(youracedata) && !Straitjacketed){
				if (u.specialSealsActive&SEAL_BLACK_WEB)
					You("begin slashing monsters with your shadow-blades.");
				else
					You("begin %sing monsters with your %s %s.",
					martial_bonus() ? "strik" : "bash",
					uarmg ? "gloved" : "bare",	/* Del Lamb */
					makeplural(body_part(HAND)));
			}
		}
	}
	exercise(A_STR, TRUE);		/* you're exercising muscles */
	/* andrew@orca: prevent unlimited pick-axe attacks */
	u_wipe_engr(3);

	/* Is the "it died" check actually correct? */
	if (pd->mlet == S_LEPRECHAUN && !mdef->mfrozen && !mdef->msleeping &&
		!mdef->mconf && !is_blind(mdef) && !rn2(7) &&
		(m_move(mdef, 0) == 2 ||			    /* it died */
		x(mdef) != u.ux + u.dx || y(mdef) != u.uy + u.dy)) /* it moved */
	{
		return(FALSE);
	}

	u.uattked = TRUE;
	check_caitiff(mdef);
	
	if ((u.specialSealsActive&SEAL_BLACK_WEB) && u.spiritPColdowns[PWR_WEAVE_BLACK_WEB] > moves + 20)
		weave_black_web(mdef);
	
	/* Do the attacks */
	bhitpos.x = u.ux + u.dx; bhitpos.y = u.uy + u.dy;
	if (!isok(bhitpos.x, bhitpos.y)) {
		if (u.uswallow) {
			bhitpos.x = u.ux;
			bhitpos.y = u.uy;
			u.uattked = TRUE;
		}
		else {
			return TRUE;
		}
	}
	notonhead = (bhitpos.x != x(mdef) || bhitpos.y != y(mdef));
	if (attack_type == ATTACKCHECK_BLDTHRST) {
		/* unintentional attacks only cause the one hit, no follow-ups */
		int vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
		/* must check that attacks are allowable */
		if (!magr_can_attack_mdef(&youmonst, mdef, bhitpos.x, bhitpos.y, FALSE))
			return FALSE;
		/* assumes the bloodthirst is caused by your mainhand weapon */
		Your("bloodthirsty weapon attacks!");
		result = xmeleehity(&youmonst, mdef, &basicattack, &uwep, VIS_MAGR, 0, FALSE);
	}
	else {
		result = xattacky(&youmonst, mdef, bhitpos.x, bhitpos.y);
		if (!DEADMONSTER(mdef) && u.sealsActive&SEAL_SHIRO){
			int i, dx, dy;
			struct obj *otmp;
			for (i = rnd(8); i>0 && !DEADMONSTER(mdef); i--){
				dx = rn2(3) - 1;
				dy = rn2(3) - 1;
				otmp = mksobj(ROCK, NO_MKOBJ_FLAGS);
				otmp->blessed = 0;
				otmp->cursed = 0;
				if ((dx || dy) && isok(x(mdef) + dx, y(mdef) + dy)){
					projectile(&youmonst, otmp, (void *)0, HMON_PROJECTILE|HMON_FIRED, x(mdef) + dx, y(mdef) + dy, -dx, -dy, 0, 1, TRUE, FALSE, FALSE);
				}
				else {
					projectile(&youmonst, otmp, (void *)0, HMON_PROJECTILE|HMON_FIRED, u.ux, u.uy, u.dx, u.dy, 0, 1, TRUE, FALSE, FALSE);
				}
			}
		}

		if (!DEADMONSTER(mdef) && u.sealsActive&SEAL_AHAZU){
			if ((*hp(mdef) < .1*(*hpmax(mdef))) && !(is_rider(pd) || pd->msound == MS_NEMESIS)){
#define MAXVALUE 24
				int value = min(monstr[monsndx(pd)] + 1, MAXVALUE);
				pline("%s sinks into your deep black shadow!", Monnam(mdef));
				cprefx(monsndx(pd), TRUE, TRUE);
				cpostfx(monsndx(pd), FALSE, TRUE, FALSE);
				if (godlist[u.ualign.god].anger) {
					godlist[u.ualign.god].anger -= ((value * (u.ualign.type == A_CHAOTIC ? 2 : 3)) / MAXVALUE);
					if (godlist[u.ualign.god].anger < 0) godlist[u.ualign.god].anger = 0;
				}
				else if (u.ualign.record < 0) {
					if (value > MAXVALUE) value = MAXVALUE;
					if (value > -u.ualign.record) value = -u.ualign.record;
					adjalign(value);
				}
				else if (u.ublesscnt > 0) {
					u.ublesscnt -=
						((value * (u.ualign.type == A_CHAOTIC ? 500 : 300)) / MAXVALUE);
					if (u.ublesscnt < 0) u.ublesscnt = 0;
				}
				mongone(mdef);
			}
			else if (!rn2(5)){
				Your("deep black shadow pools under %s.", mon_nam(mdef));
				mdef->movement -= 12;
			}
		}
	}

atk_done:
	/* see comment in attack_checks() */
	/* we only need to check for this if we did an attack_checks()
	* and it returned non-0 (it's okay to attack), and the monster didn't
	* evade.
	*/
	if (flags.forcefight && mdef && mdef->mhp > 0 && !canspotmon(mdef) &&
		!glyph_is_invisible(levl[u.ux + u.dx][u.uy + u.dy].glyph) &&
		!(u.uswallow && mdef == u.ustuck))
		map_invisible(u.ux + u.dx, u.uy + u.dy);

	return(TRUE);
}

/* xattacky()
 * 
 * Harmonized function for when a creature attempts to attack another creature.
 * 
 * Assumes that the agressor (now) knows the defender is there.
 * 
 * If the player is the aggressor, assumes melee range to the defender.
 * The player's ranged attacks must be handled elsewhere.
 * 
 * Monster to monster attacks.  When a monster attacks another (mattackm),
 * any or all of the following can be returned.  See mattackm() for more
 * details.
 * MM_MISS		0x00	aggressor missed
 * MM_HIT		0x01	aggressor hit defender
 * MM_DEF_DIED	0x02	defender died
 * MM_AGR_DIED	0x04	aggressor died
 * MM_AGR_STOP	0x08	aggressor must stop attacking for some reason
 * MM_DEF_LSVD	0x10	defender died and then revived
 * 
 * Inside this function itself, determines if/which attacks are allowed and
 * then farms each out to an appropriate function.
 * 
 * The farmed out attack-specific function is responsible for calling
 * the correct damaging function. For example, AT_GAZE attacks have
 * different effects than AT_CLAW attacks, even when with the same adtype.
 */
int
xattacky(magr, mdef, tarx, tary)
struct monst * magr;
struct monst * mdef;
int tarx;
int tary;
{
	/* if attacker doesn't exist or is trying to attack something that doesn't exist -- must be checked right away */
	if (!magr || !mdef)
		return(MM_MISS);		/* mike@genat */
	if ((magr != &youmonst && DEADMONSTER(magr)) ||
		(mdef != &youmonst && DEADMONSTER(mdef)))
		return MM_MISS;

	int	indexnum = 0,	/* loop counter */
		tohitmod = 0,	/* flat accuracy modifier for a specific attack */
		strike = 0,	/* hit this attack (default to 0) */
		aatyp = 0,	/* aatyp of current attack; used for brevity */
		adtyp = 0,	/* adtyp of current attack; used for brevity */
		struck = 0,	/* hit at least once */
		marinum = 0,/* number of AT_MARI weapons used */
		subout[SUBOUT_ARRAY_SIZE] = {0},	/* remembers what attack substitutions have been made for [magr]'s attack chain */
		mariarm = 0,/* remembers how many eilistran armor attacks have been made, so they can be charged for. */
		res[4];		/* results of previous 2 attacks ([0] -> current attack, [1] -> 1 ago, [2] -> 2 ago) -- this is dynamic! */
	int attacklimit = 0;
	int attacksmade = 0;
	struct attack *attk;
	struct attack prev_attk = noattack;
	struct obj *otmp;
	int vis = 0;
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	boolean missedyou = (!youagr && youdef && (tarx != u.ux || tary != u.uy));	/* monster tried to attack you, but it got your location wrong */
	boolean missedother = (!youagr && !youdef && mdef && (tarx != mdef->mx || tary != mdef->my));	/* monster tried to attack something, but it got the location wrong */
	boolean ranged = (distmin(x(magr), y(magr), tarx, tary) > 1);	/* is magr near its target? */
	boolean dopassive = FALSE;	/* whether or not to provoke a passive counterattack */
	/* if TRUE, don't make attacks that will be fatal to self (like touching a cockatrice) */
	boolean be_safe = (mdef && !(youagr ? (Hallucination || flags.forcefight || (!canspotmon(mdef) && mdef != u.ustuck)) :
		(magr->mcrazed || mindless_mon(magr) || (youdef && !mon_can_see_you(magr)) || (!youdef && !mon_can_see_mon(magr, mdef)))));
	/* if TRUE, don't make direct contact attacks against unknown monsters that may or may not be fatal to self */
	boolean be_safe_unknown = (mdef && youagr && u.uavoid_unsafetouch && !flags.forcefight &&
		((!canspotmon(mdef) && mdef != u.ustuck) || Hallucination));
#define skip_unsafe_attack(weapon) ((be_safe_unknown && !safe_attack(magr, mdef, attk, weapon, pa, NULL)) || \
				    (be_safe && !safe_attack(magr, mdef, attk, weapon, pa, pd)))

	/* set permonst pointers */
	struct permonst * pa = youagr ? youracedata : magr->data;
	struct permonst * pd = youdef ? youracedata : mdef->data;
	int result = 0;		/* result from current attack */
	int allres = 0;		/* cumulative results from all attacks; needed for passives */
	long slot;

	/* set notonhead */
	notonhead = (tarx != x(mdef) || tary != y(mdef));

	/*
	* Since some characters attack multiple times in one turn,
	* allow user to specify a count prefix for 'F' to limit
	* number of attack (to avoid possibly hitting a dangerous
	* monster multiple times in a row and triggering lots of
	* retalitory attacks).
	*/
	if (youagr) {
		/* kludge to work around parse()'s pre-decrement of `multi' */
		attacklimit = (multi || save_cm) ? multi + 1 : 0;
		multi = 0;		/* reset; it's been used up */
	}
	/* monsters always give their all */
	else 
	{
		attacklimit = 0;
	}

	/* cases where the agressor cannot make any attacks at all */
	if (!magr_can_attack_mdef(magr, mdef, tarx, tary, TRUE))
		return MM_MISS;

	/* Set up the visibility of action */
	vis = getvis(magr, mdef, tarx, tary);

	/*	Set flag indicating monster has moved this turn.  Necessary since a
	 *	monster might get an attack out of sequence (i.e. before its move) in
	 *	some cases, in which case this still counts as its move for the round
	 *	and it shouldn't move again.
	 */
	if(!youagr) magr->mlstmv = monstermoves;

	/* Weeping angels (pre-invocation) can only attack once per global turn */
	if (!(u.uevent.invoked) && is_weeping(pa))
		magr->movement = 0;

	/* monsters may be in for a surprise if attacking a hidden player */
	if (youdef && !ranged && !missedyou && !u.uswallow) {
		if (u_surprise(magr, canseemon(magr)))
			return MM_MISS;	/* they can't attack this turn */
	}

	/*	Special demon/minion handling code */
	/* mvu only; we don't want it mvm and player's is handled as an ability */
	if (youdef && !magr->cham && gates_in_help(pa) && !template_blocks_gate(magr)
		&& !ranged && !missedyou && (magr->summonpwr < magr->data->mlevel)) {
		 if (!magr->mcan && !rn2(13)) {
			 msummon(magr, (struct permonst *)0);
		 }
	}

	/*	Special lycanthrope handling code */
	if(youdef && !magr->cham && is_were(pa) && !ranged) {

	    if(is_human(pa)) {
			if(!rn2(5 - (night() * 2)) && !magr->mcan) new_were(magr);
	    } else if(!rn2(30) && !magr->mcan) new_were(magr);
	    pa = magr->data;

	    if(!rn2(10) && !magr->mcan) {
	    	int numseen, numhelp;
		char buf[BUFSZ], genericwere[BUFSZ];

		Strcpy(genericwere, "creature");
		numhelp = were_summon(magr, &numseen, genericwere);
		if (vis&VIS_MAGR) {
			pline("%s summons help!", Monnam(magr));
			if (numhelp > 0) {
			    if (numseen == 0)
				You_feel("hemmed in.");
			} else pline("But none comes.");
		} else {
			const char *from_nowhere;

			if (flags.soundok) {
				pline("%s %s!", Something,
					makeplural(growl_sound(magr)));
				from_nowhere = "";
			} else from_nowhere = " from nowhere";
			if (numhelp > 0) {
			    if (numseen < 1) You_feel("hemmed in.");
			    else {
				if (numseen == 1)
			    		Sprintf(buf, "%s appears",
							an(genericwere));
			    	else
			    		Sprintf(buf, "%s appear",
							makeplural(genericwere));
				pline("%s%s!", upstart(buf), from_nowhere);
			    }
				/* help came, that was their action */
				return MM_MISS;
			} /* else no help came; but you didn't know it tried */
		}
	    }
	}
	
	if(!youagr){
		magr->mprev_attk.x = sgn(x(mdef) - x(magr));
		magr->mprev_attk.y = sgn(y(mdef) - y(magr));
	}
#ifdef STEED
	/* monsters may target your steed */
	if (youdef && u.usteed && !missedyou) {
		if (magr == u.usteed)
			/* Your steed won't attack you */
			return MM_MISS;
		/* Orcs like to steal and eat horses and the like */
		if (!rn2(is_orc(magr->data) ? 2 : 4) &&
			distu(x(magr), y(magr)) <= 2) {
			/* Attack your steed instead */
			mdef = u.usteed;
			result = xattacky(magr, mdef, u.ux, u.uy);
			if (result != MM_MISS) {	/* needs to have made at least 1 attack */
				if (result & MM_AGR_DIED)
					return MM_AGR_DIED;
				if ((result & MM_DEF_DIED) || u.umoved)
					return MM_AGR_STOP;
				/* Let your steed maybe retaliate */
				if (mdef->movement >= NORMAL_SPEED) {
					int res2 = xattacky(mdef, magr, x(magr), y(magr));
					if (res2) {
						if (res2 & MM_DEF_DIED)
							result |= MM_AGR_DIED;
						if (res2 & MM_AGR_DIED)
							result |= MM_AGR_STOP;
						mdef->movement -= NORMAL_SPEED;
					}
				}
			}
			return result;
		}
	}
#endif

	/* set monster attacking flag */
	if (!youagr && youdef) {
		//It is going to try attacking you now, even if it fails for some reason, it tried.
		magr->mattackedu = TRUE; 
	}
	magr->mnoise = TRUE; 

	/* monsters can use offensive items */
	/* Unlike defensive stuff, don't let them use item _and_ attack. */
	if (!youagr && find_offensive(magr)) {
		int foo = use_offensive(magr);

		if (foo == 1)
			return MM_AGR_DIED;
		if (foo == 2)
			return MM_AGR_STOP;
	}


	/* lillends (that aren't you) can use masks */
	if (pa->mtyp == PM_LILLEND
		&& !youagr
		&& !(has_template(magr, ZOMBIFIED) || has_template(magr, SKELIFIED))
		&& rn2(2)){
		magr->mvar_attack_pm = monsndx(find_mask(magr));
		if (!Blind && pa->mtyp != PM_LILLEND && canseemon(magr))
			pline("%s uses a %s mask!", Monnam(magr), pa->mname);
	}
	
	/* deliriums use their apparent form */
	if (pa->mtyp == PM_WALKING_DELIRIUM && !youagr && magr->mappearance && magr->m_ap_type == M_AP_MONSTER){
		magr->mvar_attack_pm = magr->mappearance;
	}
	
	/* zero out res[] */
	res[0] = MM_MISS;
	res[1] = MM_MISS;
	res[2] = MM_MISS;
	res[3] = MM_MISS;
	/* Now perform all attacks. */
	do {
		boolean dopassive_local = FALSE;
		/* reset pd (in case player-defender rehumanized) */
		pd = youdef ? youracedata : mdef->data;
		/* reset otmp */
		otmp = (struct obj *)0;
		/* reset tohitmod */
		tohitmod = 0;
		/* reset result */
		result = 0;
		/* cycle res[] -- it should be the results of the previous 3 attacks */
		res[3] = res[2];
		res[2] = res[1];
		res[1] = res[0];
		res[0] = MM_MISS;
		/* get next attack */
		attk = getattk(magr, mdef, res, &indexnum, &prev_attk, FALSE, subout, &tohitmod);
		/* set aatyp, adtyp */
		aatyp = attk->aatyp;
		adtyp = attk->adtyp;
		/* maybe end attack loop early (mdef may have been forcibly moved!)*/
		if (
			(youdef
#ifdef STEED
			|| mdef == u.usteed
#endif
			) ? (!missedyou && (tarx != u.ux || tary != u.uy))
			: (!missedother && m_at(tarx, tary) != mdef && !(youagr && u.uswallow && mdef == u.ustuck))
		){
			result = MM_AGR_STOP;
			continue;
		}
		/* Some armor completely covers the face and prevents bite attacks*/
		if (aatyp == AT_BITE || aatyp == AT_LNCK || aatyp == AT_5SBT ||
			(aatyp == AT_ENGL && !(youdef && u.uswallow)) ||
			(aatyp == AT_TENT && is_mind_flayer(magr->data))
		){
			struct obj * helm = youagr ? uarmh : which_armor(magr, W_ARMH);
			struct obj * cloak = youagr ? uarmc : which_armor(magr, W_ARMC);
			if ((helm && FacelessHelm(helm)) ||
				(cloak && FacelessCloak(cloak)))
				continue;
		}
		/* avoid making unsafe attacks if you choose not to */
		if (youagr && u.uavoid_passives && !no_contact_attk(attk)) continue;

		/* Generalized offhand attack when not allowed */
		if ((attk->offhand) && (						// offhand attack
				(youagr && ((uarms && !activeFightingForm(FFORM_SHIELD_BASH)) || (uwep && bimanual(uwep, youracedata)))) ||	// player attacking with shield
				(!youagr && (which_armor(magr, W_ARMS) || (MON_WEP(magr) && bimanual(MON_WEP(magr), pa))))	// monster attacking with shield
				)
			) {
			continue;									// not allowed, don't attack
		}

		/* based on the attack type... */
		switch (aatyp)
		{
//////////////////////////////////////////////////////////////
// NON-ATTACK
//////////////////////////////////////////////////////////////
			/* non attacks, do not occur here */
		case AT_NONE:	// retaliatory when hit
		case AT_BOOM:	// retaliatory explosion when killed
			continue;
//////////////////////////////////////////////////////////////
// WEAPON
//////////////////////////////////////////////////////////////
			/* various weapon attacks */
		case AT_WEAP:	// mainhand
		case AT_DEVA:	// mainhand; can attack many times
		case AT_XWEP:	// no ranged (is done concurrently with AT_WEAP for offhanded blasters); offhand
		case AT_MARI:	// no ranged; from inventory
		case AT_HODS:	// no ranged; opponent's mainhand weapon

			/* there are cases where we can't attack at all */
			/* 1: Wielding a weapon */
			if (!youagr &&												// monster attacking
				(aatyp == AT_WEAP || aatyp == AT_DEVA) &&				// using a primary weapon attack
				(magr->weapon_check == NEED_WEAPON || !MON_WEP(magr))	// needs a weapon
				){
				/* what ranged weapon options do we have? */
				otmp = select_rwep(magr);

				/* pick appropriate weapon based on range to target */
				if (dist2(x(magr), y(magr), tarx, tary) <
					((otmp && is_bad_melee_pole(otmp) && !melee_polearms(pa)) ? 3 : m_pole_range(magr)))	// if we have a polearm, use it longer range
				{
					/* melee range */
					magr->combat_mode = HNDHND_MODE;
					magr->weapon_check = NEED_HTH_WEAPON;
				}
				else {
					/* long or polearm range */
					magr->combat_mode = RANGED_MODE;
					magr->weapon_check = NEED_RANGED_WEAPON;
				}
				if (mon_wield_item(magr) != 0)				// and try to wield something (did it take time?)
				{
					if (MON_WEP(magr) && is_pole(MON_WEP(magr))) {
						/* you can wield a polearm and attack in the same action */
					}
					else {
						mon_ranged_gazeonly = FALSE;
						allres |= MM_HIT;					// this xattacky() call took time
						continue;							// it took time, don't attack using this action
					}
				}
			}
			/* 2: Offhand attack when not allowed */
			if ((aatyp == AT_XWEP || aatyp == AT_XSPR) && (	// offhand attack
					(youagr && !(u.twoweap || activeFightingForm(FFORM_SHIELD_BASH))) ||				// player attacking and choosing not to twoweapon
					(!youagr && (which_armor(magr, W_ARMS) || (MON_WEP(magr) && bimanual(MON_WEP(magr), pa))))	// monster attacking and cannot twoweapon (wearing shield)
					)
				) {
				continue;									// not allowed, don't attack
			}
			/*3: ranged attack with incorrect aatyp */
			if (!youagr && ranged && (						// monster's ranged attack
				aatyp == AT_XWEP ||							// subject to change; may later allow monsters to fire guns from their offhand
				aatyp == AT_MARI ||
				aatyp == AT_HODS)
				){
				continue;									// not allowed, don't attack
			}
			/*4a: PC's ranged offhand */
			if(youagr && aatyp == AT_XWEP && (!uwep || !is_launcher(uwep))
				&& uswapwep && is_launcher(uswapwep)
				&& ((uquiver && ammo_and_launcher(uquiver, uswapwep)) || is_blaster(uswapwep))
			){
				//Handle after the end of the attack chain
				continue;
			}

			/* get correct weapon */
			if (aatyp == AT_WEAP || aatyp == AT_DEVA) {
				/* mainhand */
				otmp = (youagr) ? uwep : MON_WEP(magr);
			}
			else if (aatyp == AT_XWEP) {
				/* offhand */
				otmp = (youagr) ? (u.twoweap ? uswapwep : uarms) : MON_SWEP(magr);
			}
			else if (aatyp == AT_MARI) {
				if(youagr && !uwep && !(u.twoweap && uswapwep)){
					//The PC is fighting unarmed
					otmp = (struct obj *) 0;
				}
				else {
					/* from inventory */
					otmp = get_mariwep(magr, pa, marinum);
					marinum++;
				}
				if( (check_subout(subout, SUBOUT_MARIARM1) && mariarm < 1)
				 || (check_subout(subout, SUBOUT_MARIARM2) && mariarm < 2)
				){
					struct obj *armor = (youagr ? uarm : which_armor(magr, W_ARM));
					mariarm++;
					if(armor && armor->otyp == EILISTRAN_ARMOR){
						armor->ovar1_eilistran_charges--;
					}
				}
			}
			else if (aatyp == AT_HODS) {
				/* defender's mainhand weapon */
				otmp = (youdef) ? uwep : MON_WEP(mdef);
			}
			/* don't make self-fatal attacks -- being at a range implies safety */
			if (!ranged && skip_unsafe_attack(otmp))
				continue;

			/* make the attack */
			/* melee -- if attacking an adjacent square or thrusting a polearm */
			if (!ranged ||
				(otmp && is_pole(otmp) && dist2(x(magr), y(magr), tarx, tary) <= m_pole_range(magr))) {
				int devai = 0;
				/* they did do an attack */
				mon_ranged_gazeonly = FALSE;
				/* check for wild misses */
				if (missedyou) {
					wildmiss(magr, attk, otmp, ranged);
					attacksmade++;
					result |= MM_AGR_STOP;	/* it knows you aren't there */
					continue;
				}
				/* print message for magr swinging weapon (m only in function) */
				if (vis)
					xswingsy(magr, mdef, otmp, ranged);
				/* do{}while(); loop for AT_DEVA */
				boolean devaloop = (aatyp == AT_DEVA);
				do {
					bhitpos.x = tarx; bhitpos.y = tary;
					if(ranged && otmp && is_cclub_able(otmp) && Insight >= 15)
						otmp->otyp = CLAWED_HAND;
					result = xmeleehity(magr, mdef, attk, &otmp, vis, tohitmod, ranged);
					if(ranged && otmp && is_cclub_able(otmp) && Insight >= 15)
						otmp->otyp = otmp->oartifact == ART_AMALGAMATED_SKIES ? TWO_HANDED_SWORD : CLUB;
					/* Marionette causes an additional weapon strike to a monster behind the original target */
					/* this can attack peaceful/tame creatures without warning */
					if (youagr && !ranged && u.sealsActive&SEAL_MARIONETTE && (result != MM_MISS))
					{
						/* try to find direction (u.dx and u.dy may be incorrect) */
						int dx = sgn(tarx - x(magr));
						int dy = sgn(tary - y(magr));
						if (isok(tarx + dx, tary + dy) &&
							isok(tarx - dx, tary - dy) &&
							u.ux == tarx - dx &&
							u.uy == tary - dy
							)
						{
							struct monst *mdef2 = u.uswallow ? u.ustuck : 
													(dx || dy) ? m_at(tarx + dx, tary + dy) : 
													(struct monst *)0;
							if (mdef2 && (mdef2 != mdef) && !DEADMONSTER(mdef2)) {
								int vis2 = (VIS_MAGR | VIS_NONE) | (canseemon(mdef2) ? VIS_MDEF : 0);
								bhitpos.x = tarx + dx; bhitpos.y = tary + dy;
								(void)xmeleehity(magr, mdef2, attk, &otmp, vis2, tohitmod, TRUE);
								/* we aren't handling MM_AGR_DIED or MM_AGR_STOP; hopefully the attacker being a player covers those cases well enough */
							}
						}
					}
					/* Mercurial weapons may hit additional targets */
					if(!ranged && !(result&(MM_AGR_DIED|MM_AGR_STOP)) && otmp && is_streaming_merc(otmp)){
						if(magr && mlev(magr) > 20 && (
							(youagr && Insight > 20 && YOU_MERC_SPECIAL)
							|| (!youagr && insightful(magr->data) && is_chaotic_mon(magr))
						)){
							result |= hit_with_streaming(magr, otmp, tarx, tary, tohitmod, attk)&(MM_AGR_DIED|MM_AGR_STOP);
						}
					}
					/* Rakuyo hit additional targets, if your insight is high enough to percieve the blood */
					if(!ranged && !(result&(MM_AGR_DIED|MM_AGR_STOP)) && Insight >= 20 && otmp && rakuyo_prop(otmp)){
						result |= hit_with_rblood(magr, otmp, tarx, tary, tohitmod, attk)&(MM_AGR_DIED|MM_AGR_STOP);
					}
					/* Club-claw insight weapons strike additional targets if your insight is high enough to perceive the claw */
					if(!ranged && !(result&(MM_AGR_DIED|MM_AGR_STOP)) && Insight >= 15 && otmp && is_cclub_able(otmp)){
						result |= hit_with_cclaw(magr, otmp, tarx, tary, tohitmod, attk)&(MM_AGR_DIED|MM_AGR_STOP);
					}
					/* Isamusei hit additional targets, if your insight is high enough to percieve the distortions */
					if(!ranged && !(result&(MM_AGR_DIED|MM_AGR_STOP)) && Insight >= 22 && otmp && otmp->otyp == ISAMUSEI){
						result |= hit_with_iwarp(magr, otmp, tarx, tary, tohitmod, attk)&(MM_AGR_DIED|MM_AGR_STOP);
					}
					/* Soldier's katar may shoot additional targets */
					if(!ranged && result&(MM_HIT) && !(result&(MM_AGR_DIED|MM_AGR_STOP)) && otmp && otmp->otyp == TWINGUN_SHANTA){
						result |= shoot_with_gun_katar(magr, otmp, tarx, tary, tohitmod, attk)&(MM_AGR_DIED|MM_AGR_STOP);
					}
					/* Cleaving causes melee attacks to hit an additional neighboring monster */
					if ((youagr && !ranged && Cleaving)
						|| (!ranged && !(result&(MM_AGR_DIED|MM_AGR_STOP)) && otmp && otmp->oartifact 
							&& get_artifact(otmp)->inv_prop == RINGED_SPEAR 
							&& (artinstance[otmp->oartifact].RRSember >= moves || artinstance[otmp->oartifact].RRSlunar >= moves)
						)
					){
						int subresult = 0;
						/* try to find direction (u.dx and u.dy may be incorrect) */
						int dx = sgn(tarx - x(magr));
						int dy = sgn(tary - y(magr));
						int nx, ny;
						int i = 0;
						boolean target = FALSE;
						while(i < 2 && !target){
							if((monstermoves+indexnum+devai+i)&1){//Odd
								//-45 degree rotation
								nx = sgn(dx+dy);
								ny = sgn(dy-dx);
							} else {
								//45 degree rotation
								nx = sgn(dx-dy);
								ny = sgn(dx+dy);
							}
							if (isok(x(magr) + nx, y(magr) + ny))
							{
								struct monst *mdef2 = u.uswallow ? u.ustuck : 
														(nx || ny) ? m_at(x(magr) + nx, y(magr) + ny) : 
														(struct monst *)0;
								if (mdef2 && (mdef2 != mdef) && !DEADMONSTER(mdef2)
									&& !((youagr && mdef2->mpeaceful) || (!youagr && magr->mpeaceful == mdef2->mpeaceful))
								){
									target = TRUE;
									int vis2 = (VIS_MAGR | VIS_NONE) | (canseemon(mdef2) ? VIS_MDEF : 0);
									bhitpos.x = x(magr) + nx; bhitpos.y = y(magr) + ny;
									subresult = xmeleehity(magr, mdef2, attk, &otmp, vis2, tohitmod, TRUE);
									/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
									result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
								}
							}
							i++;
						}
					}
					/* Cleaving weapon trait can cause melee attacks to hit an additional neighboring monster, if the blow kills the primary target */
					if (!ranged && otmp && magr
						&& (result&MM_DEF_DIED)
						&& CHECK_ETRAIT(otmp, magr, ETRAIT_CLEAVE)
						&& ROLL_ETRAIT(otmp, magr, TRUE, !rn2(4))
					){
						int subresult = 0;
						/* try to find direction (u.dx and u.dy may be incorrect) */
						int dx = sgn(tarx - x(magr));
						int dy = sgn(tary - y(magr));
						int nx, ny;
						int i = 0;
						boolean target = FALSE;
						while(i < 2 && !target){
							if((monstermoves+indexnum+devai+i)&1){//Odd
								//-45 degree rotation
								nx = sgn(dx+dy);
								ny = sgn(dy-dx);
							} else {
								//45 degree rotation
								nx = sgn(dx-dy);
								ny = sgn(dx+dy);
							}
							i++;
							if (isok(x(magr) + nx, y(magr) + ny))
							{
								struct monst *mdef2 = (youagr && u.uswallow) ? u.ustuck : 
														(nx || ny) ? m_at(x(magr) + nx, y(magr) + ny) : 
														(struct monst *)0;
								if (mdef2 && (mdef2 != mdef) && !DEADMONSTER(mdef2)
									&& !((youagr && mdef2->mpeaceful) || (!youagr && magr->mpeaceful == mdef2->mpeaceful))
								){
									target = TRUE;
									int vis2 = (VIS_MAGR | VIS_NONE) | (canseemon(mdef2) ? VIS_MDEF : 0);
									bhitpos.x = x(magr) + nx; bhitpos.y = y(magr) + ny;
									subresult = xmeleehity(magr, mdef2, attk, &otmp, vis2, tohitmod, TRUE);
									/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
									result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
								}
							}
						}
					}
					/* Second weapon trait can cause melee attacks to hit an additional neighboring monster, if the blow kills the primary target */
					if (!ranged && magr && (result&MM_DEF_DIED)){
						struct obj *second = youagr ? (u.twoweap ? uswapwep : 0) : MON_SWEP(magr);
						if(second && CHECK_ETRAIT(second, magr, ETRAIT_SECOND) && ROLL_ETRAIT(second, magr, TRUE, !rn2(4))){
							struct attack secattk = {AT_XWEP, AD_PHYS, 0, 0}; 
							int subresult = 0;
							/* try to find direction (u.dx and u.dy may be incorrect) */
							int dx = sgn(x(magr) - tarx);
							int dy = sgn(y(magr) - tary);
							int nx, ny;
							int i = 0;
							boolean target = FALSE;
							while(i < 8 && !target){
								if((monstermoves+indexnum+devai)&1){//Odd
									//-45 degree rotation
									nx = sgn(dx+dy);
									ny = sgn(dy-dx);
								} else {
									//45 degree rotation
									nx = sgn(dx-dy);
									ny = sgn(dx+dy);
								}
								dx = nx;
								dy = ny;
								i++;
								if (isok(x(magr) + nx, y(magr) + ny))
								{
									struct monst *mdef2 = (youagr && u.uswallow) ? u.ustuck : 
															(nx || ny) ? m_at(x(magr) + nx, y(magr) + ny) : 
															(struct monst *)0;
									if (mdef2 && (mdef2 != mdef) && !DEADMONSTER(mdef2)
										&& !((youagr && mdef2->mpeaceful) || (!youagr && magr->mpeaceful == mdef2->mpeaceful))
									){
										target = TRUE;
										int vis2 = (VIS_MAGR | VIS_NONE) | (canseemon(mdef2) ? VIS_MDEF : 0);
										bhitpos.x = x(magr) + nx; bhitpos.y = y(magr) + ny;
										subresult = xmeleehity(magr, mdef2, &secattk, &second, vis2, tohitmod, TRUE);
										/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
										result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
									}
								}
							}
						}
					}
					/* Dancers hit additional targets */
					if(!ranged && !(result&(MM_AGR_DIED|MM_AGR_STOP)) && is_dancer(magr->data)){
						result |= hit_with_dance(magr, otmp, tarx, tary, tohitmod, attk)&(MM_AGR_DIED|MM_AGR_STOP);
					}
					/* Rejection antenae hit additional targets (last) */
					if(!(result&(MM_AGR_DIED|MM_AGR_STOP)) && otmp && check_oprop(otmp, OPROP_ANTAW) && check_reanimation(ANTENNA_REJECT)){
						result |= hit_with_rreject(&youmonst, otmp, tarx, tary, 0, attk);
					}
					/* AT_DEVA considerations -- either stop looping or decrease to-hit */
					if (devaloop) {
						if (result == MM_MISS || result&(MM_DEF_DIED|MM_DEF_LSVD|MM_AGR_DIED|MM_AGR_STOP))
							devaloop = FALSE;
						else
							tohitmod -= 4;	/* reduce accuracy on repeated AT_DEVA attacks */
					}
					devai++;
				} while (devaloop);

				if (!ranged)
					dopassive_local = TRUE;
				/* if the attack hits, or if the creature is able to notice it was attacked (but the attack missed) it wakes up */
				if (youdef ||(!(result&MM_DEF_DIED) && (result || (!mdef->msleeping && mdef->mcanmove))))
					wakeup2(mdef, youagr);
				/* increment number of attacks made */
				attacksmade++;
			}
			else {
				/* make ranged attack */
				/* players should not make ranged attacks here */
				if (youagr)
					continue;
				else if (m_online(magr, mdef, tarx, tary, (magr->mtame && !magr->mconf), TRUE) &&
					distmin(x(magr), y(magr), tarx, tary) <= BOLT_LIM) {
					if (mdofire(magr, mdef, tarx, tary)) {
						/* they did do a ranged attack */
						mon_ranged_gazeonly = FALSE;
						/* monsters figure out they don't know where you are */
						if (missedyou) {
							magr->mux = magr->muy = 0;
						}
						/* note: can't tell if mdef lifesaved */
						if (*hp(mdef) < 0)
							result |= MM_DEF_DIED;

						/* if the attack was made, defender can wake up (reduced chance vs melee) */
						if ((youdef || (!(result&MM_DEF_DIED) && result)) && !rn2(3))
							wakeup2(mdef, youagr);
						/* increment number of attacks made */
						attacksmade++;
					}
				}
			}
			break;
//////////////////////////////////////////////////////////////
// MELEE
//////////////////////////////////////////////////////////////
			/* basic hand-to-hand attacks, check covering for hitting *trices */
		case AT_CLAW:	// 
		case AT_KICK:	// 
		case AT_BITE:	// not used by vampires against creatures with deadly corpses
		case AT_OBIT:	//
		case AT_WBIT:	//
		case AT_STNG:	// 
		case AT_BUTT:	// 
		case AT_TAIL:	// 
		case AT_TONG:	// 
		case AT_TENT:	// 
		case AT_WHIP:	// 
		case AT_VINE:	// uses touch accuracy
		case AT_WISP:	// 
		case AT_HITS:	// always hits
		case AT_TUCH:	// uses touch accuracy
		case AT_VOMT:	// uses touch accuracy
		case AT_SRPR:	// uses touch accuracy
		case AT_XSPR:	// uses touch accuracy
		case AT_MSPR:	// uses touch accuracy
		case AT_DSPR:	// uses touch accuracy
		case AT_ESPR:	// uses touch accuracy
		case AT_REND:	// hits if previous 2 attacks hit
		case AT_HUGS:	// hits if previous 2 attacks hit, or if magr and mdef are stuck together
			/* not in range */
			if (ranged)
				continue;
			/* don't make self-fatal attacks */
			if (skip_unsafe_attack((struct obj *)0))
				continue;
			/* check for wild misses */
			if (missedyou) {
				wildmiss(magr, attk, otmp, ranged);
				attacksmade++;
				result |= MM_AGR_STOP;	/* it knows you aren't there */
				continue;
			}
			boolean devaloop = (aatyp == AT_DSPR);
			do {
				/* make the attack */
				bhitpos.x = tarx; bhitpos.y = tary;
				result = xmeleehity(magr, mdef, attk, (struct obj **)0, vis, tohitmod, ranged);
				if (!no_contact_attk(attk)) dopassive_local = TRUE;
				/* if the attack hits, or if the creature is able to notice it was attacked (but the attack missed) it wakes up */
				if (youdef || (!(result&MM_DEF_DIED) && (result || (!mdef->msleeping && mdef->mcanmove))))
					wakeup2(mdef, youagr);
				/* devaloop considerations -- either stop looping or decrease to-hit */
				if (devaloop) {
					if (result == MM_MISS || result&(MM_DEF_DIED|MM_DEF_LSVD|MM_AGR_DIED|MM_AGR_STOP))
						devaloop = FALSE;
					else
						tohitmod -= 4;	/* reduce accuracy on repeated AT_DEVA attacks */
				}
			} while (devaloop);
			/* increment number of attacks made */
			attacksmade++;
			break;

			/* engulfing attacks */
		case AT_ENGL:
		case AT_ILUR:	/* deprecated */
			/* not in range */
			if (ranged)
				continue;
			/* don't make self-fatal attacks */
			if (skip_unsafe_attack((struct obj *)0))
				continue;
			/* cannot swallow huge or larger */
			if (pd->msize >= MZ_HUGE)
				continue;
			/* cannot swallow anyone else while player is engulfed */
			if (u.uswallow && u.ustuck == magr && !youdef)
				continue;
			/* ahazu protects the player from engulfing */
			if (youdef && u.sealsActive&SEAL_AHAZU)
				continue;
			/* the player can out out of making engulf attacks */
			if (youagr && u.uavoid_englattk)
				continue;
			/* check for wild misses */
			if (missedyou) {
				wildmiss(magr, attk, otmp, ranged);
				attacksmade++;
				result |= MM_AGR_STOP;	/* it knows you aren't there */
				continue;
			}
			/* make the attack */
			if ((vis&VIS_MAGR) && magr->mappearance) seemimic_ambush(magr);	// its true form must be revealed
			result = xengulfhity(magr, mdef, attk, vis);
			/* if the attack hits, or if the creature is able to notice it was attacked (but the attack missed) it wakes up */
			if (youdef || (!(result&MM_DEF_DIED) && (result || (!mdef->msleeping && mdef->mcanmove))))
				wakeup2(mdef, youagr);
			/* increment number of attacks made */
			attacksmade++;
			break;

			/* explodes, killing itself */
		case AT_EXPL:
			/* not in range */
			if (ranged)
				continue;
			/* a monster exploding must not be cancelled */
			if (!youagr && magr->mcan)
				continue;
			/* explode -- this function handles wild misses */
			result = xexplodey(magr, mdef, attk, vis);
			/* increment number of attacks made */
			attacksmade++;
			break;

//////////////////////////////////////////////////////////////
// RANGED
//////////////////////////////////////////////////////////////

			/* ranged on-line attacks */
		case AT_BREA:	// breath attack, can be draconic
		case AT_BEAM:	// breath attack but a like a beam wand so it can't be reflected.
						// Maybe should be axed and rolled into AT_BREA; only uses AD_WET and AD_DRLI
		case AT_SPIT:	// spit venom
		case AT_ARRW:	// fire an arrow/boulder/etc; can be fired in close quarters (except for AD_SHDW bolts)
			/* these are all monster only -- players can activate these via #monster or 'f'iring with an empty quiver */
			if (youagr)
				continue;

			/* check line of fire to target -- this includes being on line, line of sight, and friendly fire */
			if (!m_online(magr, mdef, tarx, tary,
					(magr->mtame && !magr->mconf),		/* pets try to be safe with ranged attacks if they aren't confused */
					(aatyp == AT_BREA ? FALSE : TRUE)))	/* breath attacks overpenetrate targets */
				continue;
			
			if (aatyp == AT_BEAM && !(mdef && tarx == x(mdef) && tary == y(mdef)))
				continue; /* Blast attacks require the target's true location */
						  /* Other attacks launch an actual ray or projectile that may go sailing past */
			
			switch (aatyp) {
			case AT_BREA:
				if (ranged && !magr->mspec_used && (distmin(x(magr), y(magr), tarx, tary) <= BOLT_LIM) && rn2(3)) {	// not in melee, 2/3 chance when ready
					if ((result = xbreathey(magr, attk, tarx, tary))) {
						/* they did do a breath attack */
						mon_ranged_gazeonly = FALSE;
						/* monsters figure out they don't know where you are */
						if (missedyou) {
							magr->mux = magr->muy = 0;
						}
					}
				}
				break;
			case AT_BEAM:
				/* lowest effort ranged attack -- goes straight to melee damage */
				if (ranged && (distmin(x(magr), y(magr), tarx, tary) <= BOLT_LIM)) {
					mon_ranged_gazeonly = FALSE;
					result = xmeleehurty(magr, mdef, attk, attk, (struct obj **)0, TRUE, -1, rn1(18, 2), vis, ranged);
				}
				break;
			case AT_SPIT:
				if (!magr->mspec_used && !rn2(BOLT_LIM - distmin(x(magr), y(magr), tarx, tary))) {	// distance in 8 chance when ready
					if ((result = xspity(magr, attk, tarx, tary))) {
						/* they did spit */
						mon_ranged_gazeonly = FALSE;
						/* monsters figure out they don't know where you are */
						if (missedyou) {
							magr->mux = magr->muy = 0;
						}
					}
				}
				break;
			case AT_ARRW:
				if(adtyp == AD_PLYS){
					if(magr->mspec_used)
						break;
					else {
						// Breath timer
						magr->mspec_used = 10 + rn2(20);
					}
				}
					
				if ((adtyp != AD_SHDW || ranged)) {	// can be used in melee range, except for shadow
					/* fire d(n,d) projectiles */
					result |= xfirey(magr, attk, tarx, tary, d(attk->damn, attk->damd));
					if (result) {
						/* they did fire at least one projectile */
						mon_ranged_gazeonly = FALSE;
						/* monsters figure out they don't know where you are */
						if (missedyou) {
							magr->mux = magr->muy = 0;
						}
					}
				}
				break;
			}

			if (result) {
				/* increment number of attacks made */
				attacksmade++;
				/* note: can't tell if mdef lifesaved */
				if (*hp(mdef) < 1)
					result |= MM_DEF_DIED;
				/* defender can wake up (reduced chance vs melee) */
				if ((youdef || !(result&MM_DEF_DIED)) && !rn2(3))
					wakeup2(mdef, youagr);
			}

			break;

			/* ranged maybe-not-on-line attacks */
		case AT_BRSH:	// breath-splash attack
			/* check line of fire to target -- this includes being in AoE, line of sight, and friendly fire */
			if (!m_insplash(magr, mdef, tarx, tary,
					(magr->mtame && !magr->mconf))		/* pets try to be safe with ranged attacks if they aren't confused */
			)
				continue;
			if (!magr->mspec_used && (distmin(x(magr), y(magr), tarx, tary) <= 2) && rn2(3)) {	// 2/3 chance when ready
				if ((result = xbreathey(magr, attk, tarx, tary))) {
					/* they did do a breath attack */
					mon_ranged_gazeonly = FALSE;
					/* monsters figure out they don't know where you are */
					if (missedyou) {
						magr->mux = magr->muy = 0;
					}
				}
			}
			
			if (result) {
				/* increment number of attacks made */
				attacksmade++;
				/* note: can't tell if mdef lifesaved */
				if (*hp(mdef) < 1)
					result |= MM_DEF_DIED;
				/* defender can wake up (reduced chance vs melee) */
				if ((youdef || !(result&MM_DEF_DIED)) && !rn2(3))
					wakeup2(mdef, youagr);
			}

			break;

			/* ranged maybe-not-on-line attacks */
		case AT_LNCK:	// range 2 bite
		case AT_5SBT:	// range 5 bite
		case AT_LRCH:	// range 2 claw
		case AT_5SQR:	// range 5 touch
			/* don't make attacks that will kill oneself */
			if (skip_unsafe_attack((struct obj *)0))
				continue;
			/* must be in range */
			if (distmin(x(magr), y(magr), tarx, tary) > 
				((aatyp == AT_5SBT || aatyp == AT_5SQR) ? 5 : 2))
				continue;
			mon_ranged_gazeonly = FALSE;
			/* check for wild misses */
			if (missedyou) {
				wildmiss(magr, attk, otmp, ranged);
				attacksmade++;
				result |= MM_AGR_STOP;	/* it knows you aren't there */
				continue;
			}
			/* make the attack */
			bhitpos.x = tarx; bhitpos.y = tary;
			result = xmeleehity(magr, mdef, attk, (struct obj **)0, vis, tohitmod, ranged);
			if (distmin(x(magr), y(magr), tarx, tary) == 1)
				dopassive_local = TRUE;
			/* if the attack hits, or if the creature is able to notice it was attacked (but the attack missed) it wakes up */
			if (youdef || (!(result&MM_DEF_DIED) && (result || (!mdef->msleeping && mdef->mcanmove))))
				wakeup2(mdef, youagr);
			/* increment number of attacks made */
			attacksmade++;
			break;

			/* targeted gazes */
		case AT_GAZE:
			/* Weeping angels gaze during movemon(). */
			if (is_weeping(pa)) {
				continue;
			}

			if (youagr) {
				/* not done as part of the player's attack chain, use #monster */
				continue;
			}
			else {
				result = xgazey(magr, mdef, attk, vis);
				if (result) {
					/* increment number of attacks made */
					attacksmade++;
					if(attk->adtyp == AD_BDFN && distmin(x(magr), y(magr), x(mdef), y(mdef)) > 2){
						//Just sit back and gaze
						mon_ranged_gazeonly = FALSE;
					}
				}
			}
			break;
			/* wide (passive) gaze */
		case AT_WDGZ:
			/* these happen every turn and are NOT done here; they're somewhere in dochug and allmain? */
			continue;
			/* tinkering */
		case AT_TNKR:
			/* players can use #monster to tinker (thanks, Demo!) -- does nothing in the player's attack chain */
			if (youagr)
				continue;
			/* must be done at a range */
			if (!ranged)
				continue;
			/* do the tinkering */
			xtinkery(magr, mdef, attk, vis);
			/* increment number of attacks made */
			attacksmade++;
			break;
			/* magic */
		case AT_MAGC:
		case AT_MMGC:
			/* the player cannot cast AT_MMGC (monster-only-magic) */
			if (youagr && aatyp == AT_MMGC)
				continue;
			/* the player can opt-out of monster spellcasting, to save pw or not wake things up or w/e */
			if (youagr && u.uavoid_msplcast)
				continue;
			/* Demogorgon casts spells less frequently in melee range */
			if (pa->mtyp == PM_DEMOGORGON && !ranged && rn2(6) && !(youagr || magr->mflee))
				continue;

			result = xcastmagicy(magr, mdef, attk, vis, indexnum);

			if (result) {
				/* if the spell was successful, the defender may wake up (MM_MISS -> no spell cast, no chance to wake) */
				wakeup2(mdef, youagr);
				/* increment number of attacks made */
				attacksmade++;
				/* Asmodeus randomly stops his casting early? */
				if (pa->mtyp == PM_ASMODEUS && !rn2(3))
					result |= MM_AGR_STOP;
			}
			break;
		}//switch (aatyp)

//////////////////////////////////////////////////////////////
// AFTER THE ATTACK IS DONE
//////////////////////////////////////////////////////////////

		//DEBUG: attack chain ended reasons
		//if (res[0] & MM_DEF_DIED)
		//	pline("def died");
		//if(res[0] & MM_DEF_LSVD)
		//	pline("def lifesaved");
		//if(res[0] & MM_AGR_DIED)
		//	pline("agr died");
		//if(res[0] & MM_AGR_STOP)
		//	pline("agr stopped");

		/* make per-attack counterattacks */
		if (dopassive_local) {
			dopassive = TRUE;
			if(youdef && (result&MM_HIT) && u.uspellprot && artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_BALANCE){
				struct monst *mon;
				for(int i = 0; i < 8; i++){
					if(isok(x(mdef)+xdir[i], y(mdef)+ydir[i]) && (mon = m_at(x(mdef)+xdir[i], y(mdef)+ydir[i])) && !DEADMONSTER(mon) && !mon->mpeaceful)
						xdamagey(mdef, mon, (struct attack *) 0, 8);
				}
				if(DEADMONSTER(magr))
					result |= MM_AGR_DIED;
			}
			result = xpassivey(magr, mdef, attk, otmp, vis, result, pd, FALSE);
		}

		/* double check MM_DEF_DIED, MM_AGR_DIED */
		if (*hp(mdef) < 1)
			result |= MM_DEF_DIED;
		if (*hp(magr) < 1)
			result |= MM_AGR_DIED;

		/* save result to res, allres */
		res[0] = result;
		allres |= result;

	}while (!(
		(youagr && attacklimit && attacksmade >= attacklimit) ||	/* player wants to only attack N times */
		(result & MM_DEF_DIED) ||									/* defender died */
		(result & MM_DEF_LSVD) ||									/* defender lifesaved */
		(result & MM_AGR_DIED) ||									/* attacker died */
		(result & MM_AGR_STOP) ||									/* attacker stopped for some other reason */
		is_null_attk(attk)											/* no more attacks */
		));

	/*4b: PC's ranged offhand */
	if(youagr && u.twoweap && (!Upolyd || could_twoweap(youracedata)) && (!uwep || !is_launcher(uwep) || is_melee_launcher(uwep))
		&& uswapwep && is_launcher(uswapwep)
		&& ((uquiver && ammo_and_launcher(uquiver, uswapwep)) || is_blaster(uswapwep))
	){
		if (uquiver && ammo_and_launcher(uquiver, uswapwep)) {
			/* simply fire uquiver from the launcher */
			uthrow(uquiver, uswapwep, 0, FALSE);
		}
		else if (is_blaster(uswapwep)) {
			/* simply fire blaster */
			ufire_blaster(uswapwep, 0);
		}
		if(DEADMONSTER(mdef))
			allres |= MM_DEF_DIED;
	}
	/* make per-round counterattacks -- note that these cannot use otmp or attk, as those are per-attack */
	if (dopassive)
		allres = xpassivey(magr, mdef, (struct attack *)0, (struct obj *)0, vis, allres, pd, TRUE);
	
	/* reset lillend mask usage */
	if (!youagr && pa->mtyp == PM_LILLEND)
		magr->mvar_attack_pm = 0;
	if (!youagr && pa->mtyp == PM_WALKING_DELIRIUM)
		magr->mvar_attack_pm = 0;
	if (!youagr && (magr->mtyp == PM_SPELLWEAVER || magr->mtyp == PM_SPELLWEAVER_GODDESS_MOCKER)){
		magr->mvar_spellweaver_count += 1;
		if(magr->mtyp == PM_SPELLWEAVER && magr->mvar_spellweaver_count > 3){
			magr->mvar_spellweaver_count = 3;
		}
		else if(magr->mtyp == PM_SPELLWEAVER_GODDESS_MOCKER && magr->mvar_spellweaver_count > 6){
			magr->mvar_spellweaver_count = 6;
		}
		magr->mvar_spellweaver_last_cast = moves;
		magr->mvar_spellweaver_seed += 1;
	}

	/* do some things only if attacks were made */
	if (attacksmade > 0) {
		
		/* when the player is noticably attacked: */
		if (youdef && ((allres & MM_HIT) || (vis&VIS_MAGR)))
		{
			/* player multi-tile movements are interrupted */
			nomul(0, NULL);
			/* player panics after being attacked by a sea creature */
			if (is_aquatic(magr->data) && roll_madness(MAD_THALASSOPHOBIA)){
				You("panic after being attacked by a sea monster!");
				HPanicking += 1 + rnd(6);
			}
		}

		/* this must come after possibly interrupting player */
		/* signify that the attack action was indeed taken, even if no attacks hit */
		allres |= MM_HIT;
	}
	else {
		/* an attack round was attempted, but no attacks were made */
		if (youagr) {
			/* if this happens for the player as magr, probably safe_touch() aborted all attacks
			 * and it would make sense to print a message explaining why your turn did nothing */
			pline("That would be unwise.");
		}
	}
	return allres;
}// xattacky

/*
 * wildmiss()
 *
 * Monster is trying to attack player (with a melee attack) but doesn't know exactly where they are.
 * Prints a message
 */
void
wildmiss(magr, attk, otmp, ranged)
struct monst * magr;
struct attack * attk;
struct obj * otmp;
boolean ranged;
{
	boolean compat = (attk->adtyp == AD_SEDU || attk->adtyp == AD_SSEX || attk->adtyp == AD_LSEX) && could_seduce(magr, &youmonst, (struct attack *)0);
	
	/* verbose message */
	if (!flags.verbose)
		return;

	/* you have to be able to see magr */
	if (!canseemon(magr))
		return;

	/* magr must be aiming at least nearby you to message */
	if (dist2(magr->mux, magr->muy, u.ux, u.uy) > 13)
		return;

	/* Print message */
	if (Displaced) {
		if (compat) {
			pline("%s smiles %s at your %sdisplaced image...",
				Monnam(magr),
				compat == 2 ? "engagingly" : "seductively",
				Invis ? "invisible " : "");
		}
		else {
			pline("%s strikes at your %sdisplaced image and misses you!",
				/* Note: if you're both invisible and displaced,
				 * only monsters which see invisible will attack your
				 * displaced image, since the displaced image is also
				 * invisible.
				 */
				Monnam(magr),
				Invis ? "invisible " : "");
		}
	}
	else if (Underwater) {
		/* monsters may miss especially on water level where
		bubbles shake the player here and there */
		if (compat) {
			pline("%s reaches towards your distorted image.", Monnam(magr));
		}
		else {
			pline("%s is fooled by water reflections and misses!", Monnam(magr));
		}
	}
	else if (magr->mcrazed) {
		pline("%s flails around randomly.", Monnam(magr));
	}
	else if (ranged && otmp) {
		pline("%s %s %s wildly.", Monnam(magr), otmp->otyp == AKLYS ? "throws" : "thrusts",
			obj_is_pname(otmp) ? the(xname(otmp)) : an(xname(otmp)));
	}
	else {
		const char *swings =
			attk->aatyp == AT_BITE || attk->aatyp == AT_LNCK || attk->aatyp == AT_5SBT ? "snaps" :
			attk->aatyp == AT_KICK ? "kicks" :
			(attk->aatyp == AT_STNG ||
			attk->aatyp == AT_BUTT ||
			nolimbs(magr->data)) ? "lunges" : "swings";

		if (compat) {
			pline("%s tries to touch you and misses!", Monnam(magr));
		}
		else {
			switch (rn2(3)) {
			case 0: pline("%s %s wildly and misses!", Monnam(magr),
				swings);
				break;
			case 1: pline("%s attacks a spot beside you.", Monnam(magr));
				break;
			case 2: pline("%s strikes at %s!", Monnam(magr),
				((int)levl[magr->mux][magr->muy].typ) == WATER
				? "empty water" : "thin air");
				break;
			default:pline("%s %s wildly!", Monnam(magr), swings);
				break;
			}
		}
	}
	return;
}


/*
 * u_surprise()
 *
 * A monster tried to move onto your square while you were hidden there! Surprise!
 */
boolean
u_surprise(mtmp, youseeit)
struct monst * mtmp;
boolean youseeit;
{
	if (u.uundetected) {
		u.uundetected = 0;
		if (is_hider(youracedata)) {
			coord cc; /* maybe we need a unexto() function? */
			struct obj *obj;

			You("fall from the %s!", ceiling(u.ux, u.uy));
			if (enexto(&cc, u.ux, u.uy, youracedata)) {
				remove_monster(mtmp->mx, mtmp->my);
				newsym(mtmp->mx, mtmp->my);
				place_monster(mtmp, u.ux, u.uy);
				if (mtmp->wormno) worm_move(mtmp);
				teleds(cc.x, cc.y, TRUE);
				set_apparxy(mtmp);
				newsym(u.ux, u.uy);
			}
			else {
				pline("%s is killed by a falling %s (you)!",
					Monnam(mtmp), youracedata->mname);
				killed(mtmp);
				newsym(u.ux, u.uy);
				if (mtmp->mhp > 0) return 0;
				else return 1;
			}
			if (youracedata->mlet != S_PIERCER)
				return(0);	/* trappers don't attack */

			obj = which_armor(mtmp, WORN_HELMET);
			if (obj && is_hard(obj)) {
				Your("blow glances off %s helmet.",
					s_suffix(mon_nam(mtmp)));
			}
			else {
				if (3 + find_mac(mtmp) <= rnd(20)) {
					pline("%s is hit by a falling piercer (you)!",
						Monnam(mtmp));
					if ((mtmp->mhp -= d(3, 6)) < 1)
						killed(mtmp);
				}
				else
					pline("%s is almost hit by a falling piercer (you)!",
					Monnam(mtmp));
			}
		}
		else {
			if (!youseeit)
				pline("It tries to move where you are hiding.");
			else {
				/* Ugly kludge for eggs.  The message is phrased so as
				* to be directed at the monster, not the player,
				* which makes "laid by you" wrong.  For the
				* parallelism to work, we can't rephrase it, so we
				* zap the "laid by you" momentarily instead.
				*/
				struct obj *obj = level.objects[u.ux][u.uy];

				if (obj ||
					(is_underswimmer(youracedata) && is_pool(u.ux, u.uy, FALSE))) {
					int save_spe = 0; /* suppress warning */
					if (obj) {
						save_spe = obj->spe;
						if (obj->otyp == EGG) obj->spe = 0;
					}
					if (is_underswimmer(youracedata))
						pline("Wait, %s!  There's a hidden %s named %s there!",
						m_monnam(mtmp), youracedata->mname, plname);
					else
						pline("Wait, %s!  There's a %s named %s hiding under %s!",
						m_monnam(mtmp), youracedata->mname, plname,
						doname(level.objects[u.ux][u.uy]));
					if (obj) obj->spe = save_spe;
				}
				else
					impossible("hiding under nothing?");
			}
			newsym(u.ux, u.uy);
		}
		return(0);
	}

	/* mimicry */
	if (youracedata->mlet == S_MIMIC && youmonst.m_ap_type) {
		if (!youseeit) pline("It gets stuck on you.");
		else pline("Wait, %s!  That's a %s named %s!",
			m_monnam(mtmp), youracedata->mname, plname);
		u.ustuck = mtmp;
		youmonst.m_ap_type = M_AP_NOTHING;
		youmonst.mappearance = 0;
		newsym(u.ux, u.uy);
		return(0);
	}

	/* player might be mimicking an object (without being a proper mimic) */
	if (youmonst.m_ap_type == M_AP_OBJECT) {
		if (!youseeit)
			pline("%s %s!", Something,
			(likes_gold(mtmp->data) && youmonst.mappearance == GOLD_PIECE) ?
			"tries to pick you up" : "disturbs you");
		else pline("Wait, %s!  That %s is really %s named %s!",
			m_monnam(mtmp),
			mimic_obj_name(&youmonst),
			an(mons[u.umonnum].mname),
			plname);
		if (multi < 0) {	/* this should always be the case */
			char buf[BUFSZ];
			Sprintf(buf, "You appear to be %s again.",
				Upolyd ? (const char *)an(youracedata->mname) :
				(const char *) "yourself");
			unmul(buf);	/* immediately stop mimicking */
		}
		return 0;
	}
	
	return 0;
}


/* getnextspiritattack()
 *
 * Returns the next spirit attack the player will use
 * Uses a static int to cycle through the attacks, which is reset to 0 when [fresh] is true
 */
struct attack *
getnextspiritattack(fresh)
boolean fresh;
{
#define ATTK_AMON		0
#define ATTK_CHUPOCLOPS	1
#define ATTK_IRIS		2
#define ATTK_NABERIUS	3
#define ATTK_OTIAX		4
#define ATTK_SIMURGH	5
#define ATTK_MISKA_ARM	6
#define ATTK_MISKA_WOLF	7
#define PASV_SHADOW_WEB	8
#define PASV_ECHIDNA	9
#define PASV_FAFNIR		10
#define PASV_EDEN		11
#define NOATTACK		12
	static struct attack spiritattack[] =
	{
		{ AT_BUTT, AD_PHYS, 1, 9 },
		{ AT_BITE, AD_DRST, 2, 4 },
		{ AT_TENT, AD_IRIS, 1, 4 },
		{ AT_BITE, AD_NABERIUS, 1, 6 },
		{ AT_WISP, AD_OTIAX, 1, 5 },
		{ AT_CLAW, AD_SIMURGH, 1, 6 },
		{ AT_MARI, AD_PHYS, 1, 8 },
		{ AT_WBIT, AD_DRST, 2, 4 },
		{ AT_NONE, AD_SHDW, 4, 8 },
		{ AT_NONE, AD_ACID, 1, 1 },	/* actually 1d(spiritDsize) */
		{ AT_NONE, AD_FIRE, 1, 1 }, /* actually 1d(spiritDsize) */
		{ AT_NONE, AD_SLVR, 1, 20 },
		{ AT_NONE, AD_PHYS, 0, 0 } /* noattack */
	};
	int i;					/* loop counter */
	static int indexnum;	/* which attack index to return -- kept between calls of this function */
	int curindex = 0;		/* which attack index has been reached */

	/* possibly reset indexnum */
	if (fresh)
		indexnum = 0;
	/* increment indexnum */
	indexnum++;

	/* Amon */
	if (u.sealsActive&SEAL_AMON) {
		if (++curindex == indexnum)
			return &spiritattack[ATTK_AMON];
	}
	/* Chupoclops */
	if (u.sealsActive&SEAL_CHUPOCLOPS) {
		if (++curindex == indexnum)
			return &spiritattack[ATTK_CHUPOCLOPS];
	}
	/* Naberius */
	if (u.sealsActive&SEAL_NABERIUS) {
		if (++curindex == indexnum)
			return &spiritattack[ATTK_NABERIUS];
	}
	/* Simurgh */
	if (u.sealsActive&SEAL_SIMURGH) {
		if (++curindex == indexnum)
			return &spiritattack[ATTK_SIMURGH];
	}
	/* Miska */
	if (u.specialSealsActive&SEAL_MISKA){
		/* poison bite */
		if (u.ulevel >= 10) {
			if (++curindex == indexnum)
				return &spiritattack[ATTK_MISKA_WOLF];
		}
		/* poison bite */
		if (u.ulevel >= 18) {
			if (++curindex == indexnum)
				return &spiritattack[ATTK_MISKA_WOLF];
		}
		/* 2x non-wielded weapon */
		if (u.ulevel >= 26) {
			for (i = 0; i < 2; i++)
			if (++curindex == indexnum)
				return &spiritattack[ATTK_MISKA_ARM];
		}
	}
	/* Iris */
	if (u.sealsActive&SEAL_IRIS){
		if (++curindex == indexnum)
			return &spiritattack[ATTK_IRIS];

		if (u.twoweap || (uwep && bimanual(uwep, youracedata))){
			if (++curindex == indexnum)
				return &spiritattack[ATTK_IRIS];
		}
		if (u.specialSealsActive&SEAL_MISKA && u.ulevel >= 26){
			/* +2 */
			for (i = 0; i < 2; i++)
			if (++curindex == indexnum)
				return &spiritattack[ATTK_IRIS];
		}
		if (youracedata->mtyp == PM_MARILITH){
			/* +4 */
			for (i = 0; i < 4; i++)
			if (++curindex == indexnum)
				return &spiritattack[ATTK_IRIS];

		}
	}
	/* Otiax */
	if (u.sealsActive&SEAL_OTIAX){
		static int tendrils;
		/* on the first call reaching here, determine how many tendrils to use */
		if (curindex + 1 == indexnum)
			tendrils = min(rnd(5), spiritDsize());

		/* make that many attacks */
		for (i = 0; i < tendrils; i++)
		if (++curindex == indexnum) {
			return &spiritattack[ATTK_OTIAX];
		}
	}
	/* Passives! */
	if (u.specialSealsActive&SEAL_BLACK_WEB && u.spiritPColdowns[PWR_WEAVE_BLACK_WEB] > moves + 20)
	{
		if (++curindex == indexnum)
			return &spiritattack[PASV_SHADOW_WEB];
	}
	if (u.sealsActive&SEAL_ECHIDNA)
	{
		spiritattack[PASV_ECHIDNA].damd = spiritDsize();
		if (++curindex == indexnum)
			return &spiritattack[PASV_ECHIDNA];
	}
	if (u.sealsActive&SEAL_FAFNIR)
	{
		spiritattack[PASV_FAFNIR].damd = spiritDsize();
		if (++curindex == indexnum)
			return &spiritattack[PASV_FAFNIR];
	}
	if (u.sealsActive&SEAL_EDEN)
	{
		if (++curindex == indexnum)
			return &spiritattack[PASV_EDEN];
	}
		
	/* Default case: no attack */
	return &spiritattack[NOATTACK];
}

/* getattk()
 * 
 * Returns the [*indexnum] attack for [magr]
 * 
 * Uses a lot of int pointers because there is no guarantee that successive calls of getattk() will be made
 * by the same attacker -- counterattacks specifically cause this, which prevents the use of static variables
 * inside the function
 */
struct attack *
getattk(magr, mdef, prev_res, indexnum, prev_and_buf, by_the_book, subout, tohitmod)
struct monst * magr;			/* attacking monster */
struct monst * mdef;			/* defending monster -- OPTIONAL */
int * prev_res;					/* results of previous attacks; ignored if by_the_book is true */
int * indexnum;					/* index number to use, incremented HERE (if actually pulling the attack from the monster's index) */
struct attack * prev_and_buf;	/* double-duty pointer: 1st, is the previous attack; 2nd, is a pointer to allocated memory */
boolean by_the_book;			/* if true, gives the "standard" attacks for [magr]. Useful for the pokedex. */
int * subout;					/* records what attacks have been subbed out */
int * tohitmod;					/* some attacks are made with decreased accuracy */
{
	struct attack * attk;
	struct attack prev_attack = *prev_and_buf;
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	boolean fromlist;
	struct permonst * pa = youagr ? youracedata : magr->data;
	struct permonst * pd = youdef ? youracedata : mdef ? mdef->data : (struct permonst *)0;

	/* lillends are able to use the attacks of another monster */
	/* modify pa here, but only here (when getting attacks) */
	if ((pa->mtyp == PM_LILLEND || pa->mtyp == PM_WALKING_DELIRIUM) && magr->mvar_attack_pm) {
		pa = &mons[magr->mvar_attack_pm];
	}

	//Get next attack
	if (*indexnum < NATTK) {
		/* get the nth attack */
		attk = &(pa->mattk[*indexnum]);
		fromlist = TRUE;
		*prev_and_buf = *attk;
		attk = prev_and_buf;
	}
	else {
		/* possibly add additional attacks */
		attk = prev_and_buf;
		*attk = noattack;
		fromlist = FALSE;
	}

#define GETNEXT {(*indexnum)++; *prev_and_buf = prev_attack; return getattk(magr, mdef, prev_res, indexnum, prev_and_buf, by_the_book, subout, tohitmod);}
	/* unpolymorphed players get changes */
	if (youagr && !Upolyd && fromlist) {
		/* do NOT get permonst-inherent attacks, except for: */
		if (!is_null_attk(attk) && !(
			(*indexnum == 0) ||												/* first attack, HOPEFULLY a weapon attack! */
			(Race_if(PM_YUKI_ONNA) && (!uwep || attk->aatyp == AT_NONE)) ||	/* yuki-onna get their additional attacks when unarmed, and their passive always */
			(Race_if(PM_VAMPIRE)) ||
			(Race_if(PM_CHIROPTERAN))
		)){
			/* just get the next attack */
			GETNEXT
		}

		/* if twoweaponing... */
		/* while NOT polymorphed, the player will insert an additional xwep attack while twoweaponing */
		/* this compensates for some player-role-monsters having AT_WEAP+AT_XWEP and some not. */
		/* if the player IS polymorphed, they are limited to their polyform's attacks */
		/* some things give the player additional weapon attacks; they can reset SUBOUT_XWEP to allow another offhand hit if unpoly'd */
		/* stilettos and wind and fire wheels add extra damage instead */
		if (*indexnum > 0 && (u.twoweap || activeFightingForm(FFORM_SHIELD_BASH))
			&& !(uwep && (uwep->otyp == STILETTOS || uwep->otyp == WIND_AND_FIRE_WHEELS))
		) {
			/* follow a weapon attack with an offhand attack */
			if (prev_attack.aatyp == AT_WEAP && attk->aatyp != AT_XWEP
				&& !check_subout(subout, SUBOUT_XWEP)
			) {
				fromlist = FALSE;
				if(activeFightingForm(FFORM_SHIELD_BASH) && uarms && uarms->oartifact == ART_DRAGONHEAD_SHIELD){
					//Messages will be a bit janky :(
					attk->aatyp = AT_HITS;
					attk->adtyp = AD_PHYS;
					attk->damn = 5;
					attk->damd = 8;
				}
				else {
					attk->aatyp = AT_XWEP;
					attk->adtyp = AD_PHYS;
					attk->damn = 1;
					attk->damd = 4;
				}
				add_subout(subout, SUBOUT_XWEP);
			}
			/* fixup for black web, which replaces AT_WEAP with an AT_SRPR */
			if ((u.specialSealsActive & SEAL_BLACK_WEB)
				&& !check_subout(subout, SUBOUT_XWEP)
				&& prev_attack.aatyp == AT_SRPR && attk->aatyp != AT_XWEP
				) {
				fromlist = FALSE;
				attk->aatyp = AT_XWEP;
				attk->adtyp = AD_PHYS;
				attk->damn = 1;
				attk->damd = 4;
				add_subout(subout, SUBOUT_XWEP);
			}
		}
	}
	
	/* players sub out monster claw attacks for weapon attacks */
	if (youagr) {
		if (attk->polywep && (uwep || u.specialSealsActive&SEAL_BLACK_WEB)){
			attk->aatyp = AT_WEAP;
			attk->adtyp = AD_PHYS;
			attk->damn = 1;	// unused
			attk->damd = 4;	// unused
		}
	}
	/*Some attacks have level requirements*/
	/*The PC never gets level-gated monster attacks, they're assumed to represent something like a role*/
	if((youagr && attk->lev_req > 0) || attk->lev_req > mlev(magr)){
		GETNEXT
	}
	/*Some attacks have insight requirements*/
	/*PCs can gain monster insight attacks, since insight is always about the PC's perceptions*/
	if(attk->ins_req > Insight){
		GETNEXT
	}
	/*Some attacks have sanity requirements*/
	if(attk->san_req > 0){
		if(youdef || by_the_book){
			if(NightmareAware_Sanity < attk->san_req){
				GETNEXT
			}
		}
		else if(mdef){
			if(mdef->encouraged < attk->san_req/10){
				GETNEXT
			}
		}
		else {
			GETNEXT
		}
	}
	else if(attk->san_req < 0){
		if(youdef || by_the_book){
			if(NightmareAware_Sanity > -1*attk->san_req){
				GETNEXT
			}
		}
		else if(mdef){
			if(mdef->encouraged > -1*attk->san_req/10){
				GETNEXT
			}
		}
		else {
			GETNEXT
		}
	}
	/* khaamnun tanninim switch to sucking memories after dragging target in close */
	if (pa->mtyp == PM_KHAAMNUN_TANNIN
		&& mdef && distmin(x(magr),y(magr), x(mdef),y(mdef)) <= 1
		&& attk->adtyp == AD_PULL
	){
		attk->aatyp = AT_TENT;
		attk->adtyp = AD_MDWP;
		attk->damd += 10;
	}
	/* charybdisone switch to water damage after dragging target in close */
	else if (pa->mtyp == PM_CHARYBDISONE
		&& mdef && distmin(x(magr),y(magr), x(mdef),y(mdef)) <= 1
		&& attk->adtyp == AD_PULL
	){
		attk->aatyp = AT_TENT;
		attk->adtyp = AD_WET;
		attk->damn += 2;
		attk->damd += 2;
	}
	/* Iksh'na devas upgrade their weapon attack at max level. This may be switched back later if they're cancelled. */
	if(pa->mtyp == PM_IKSH_NA_DEVA && mlev(magr) >= 45 && attk->aatyp == AT_WEAP && *indexnum == 0){
		attk->aatyp = AT_DEVA;
	}
	/* Carcosan courtiers gain extra dice on their tentacles. */
	if(pa->mtyp == PM_CARCOSAN_COURTIER && attk->aatyp == AT_TENT && Insight > 5){
		if(Insight < 25){
			attk->damn = Insight/5;
		}
		else {
			attk->damn = 5;
		}
	}
	/* Dunwich horror can't cast if you don't have the correct mutation */
	if(pa->mtyp == PM_TWIN_SIBLING && !check_mutation(TWIN_MIND) && attk->aatyp == AT_MAGC){
		GETNEXT
	}
	/* Grue does not make its later attacks if its square is lit */
	if (pa->mtyp == PM_GRUE &&
		!by_the_book &&
		*indexnum >= 2 &&
		!isdark(x(magr), y(magr))
	) {
		*attk = noattack;
		return attk;
	}
	if(magr->mforgetful && (attk->adtyp == AD_MAGM || attk->adtyp == AD_SPEL)){
		GETNEXT
	}
	if(magr->mapostasy && (attk->adtyp == AD_CLRC || attk->adtyp == AD_HOLY)){
		GETNEXT
	}
	/* Shadowsmiths lose special attacks in the light */
	if(magr->mtyp == PM_SHADOWSMITH && (magr->mcan || dimness(magr->mx,magr->my) <= 0)){
		if(attk->aatyp == AT_SRPR || attk->aatyp == AT_XSPR){
			if(attk->aatyp == AT_XSPR)
				attk->offhand = 1;
			attk->aatyp = AT_CLAW;
			attk->adtyp = AD_PHYS;
			attk->damn = 1;
			attk->damd = 2;
		}
	}
	/* Magic blade attacks are changed or lost if the creature is canceled */
	else if (magr->mcan) {
		if(magr->mtyp == PM_ALIDER && magr->mcan && (attk->aatyp == AT_WEAP || attk->aatyp == AT_XWEP)){
			attk->damn = 1;
			attk->damd = 6;
		}
		else if(attk->aatyp == AT_SRPR){
			if(magr->mtyp == PM_JRT_NETJER){
				attk->aatyp = AT_WEAP;
				attk->adtyp = AD_PHYS;
				attk->damn = 1;
				attk->damd = 7;
			}
			else {
				attk->aatyp = (humanoid_upperbody(pa) && !nogloves(pa)) ? AT_WEAP : AT_CLAW;
				attk->adtyp = AD_PHYS;
				attk->damn = 1;
				attk->damd = 6;
			}
		}
		else if(attk->aatyp == AT_XSPR){
			if(magr->mtyp == PM_JRT_NETJER){
				attk->aatyp = AT_XWEP;
				attk->adtyp = AD_PHYS;
				attk->damn = 1;
				attk->damd = 7;
			}
			else {
				attk->aatyp = (humanoid_upperbody(pa) && !nogloves(pa)) ? AT_XWEP : AT_CLAW;
				attk->adtyp = AD_PHYS;
				attk->damn = 1;
				attk->damd = 6;
				if(attk->aatyp == AT_CLAW)
					attk->offhand = 1; /*Note: redundant with xwep but needed for claw*/
			}
		}
		else if(attk->aatyp == AT_MSPR){
			attk->aatyp = (humanoid_upperbody(pa) && pa->mtyp != PM_ALIDER && !nogloves(pa)) ? AT_MARI : AT_CLAW;
			attk->adtyp = AD_PHYS;
			attk->damn = 1;
			attk->damd = 6;
		}
		else if(attk->aatyp == AT_5SQR && attk->adtyp == AD_SHDW)
			GETNEXT
		else if(attk->aatyp == AT_DSPR)
			GETNEXT
		else if(attk->aatyp == AT_ESPR)
			GETNEXT
		else if(attk->aatyp == AT_DEVA){
			attk->aatyp = AT_WEAP;
		}
		else if(attk->aatyp == AT_ARRW){
			/*only some of these are magical*/
			if(attk->adtyp == AD_PLYS)
				GETNEXT
			else if(attk->adtyp == AD_SHDW)
				GETNEXT
			else if(attk->adtyp == AD_SOLR)
				GETNEXT
			else if(attk->adtyp == AD_SURY)
				GETNEXT
			else if(attk->adtyp == AD_LOAD)
				GETNEXT
		}
		else if(attk->aatyp == AT_BEAM && attk->adtyp != AD_WET)
				GETNEXT
	}
	else if(spirit_rapier_at(attk->aatyp) && attk->adtyp == AD_MOON){
		int pom = phase_of_the_moon();
		if(pom == 8){
			attk->damn += 2;
			if(attk->damd < 12) attk->damd = 12;
		}
		else {
			if(pom < 4){
				if(attk->aatyp == AT_SRPR){
					attk->aatyp = humanoid_upperbody(pa) ? AT_WEAP : AT_CLAW;
					attk->adtyp = AD_PHYS;
					attk->damn = 1;
					attk->damd = 6;
				}
				else if(attk->aatyp == AT_DSPR){
					attk->aatyp = AT_DEVA;
					attk->adtyp = AD_PHYS;
					attk->damn = 1;
					attk->damd = 6;
				}
				else if((attk->aatyp == AT_MSPR || attk->aatyp == AT_ESPR) && !(*indexnum%2)){
					if(attk->aatyp == AT_ESPR){
						GETNEXT
					}
					else {
						attk->aatyp = humanoid_upperbody(pa) ? AT_MARI : AT_CLAW;
						attk->adtyp = AD_PHYS;
						attk->damn = 1;
						attk->damd = 6;
					}
				}
			}
			if(pom > 4 || !pom){
				if(attk->aatyp == AT_XSPR){
					attk->aatyp = (humanoid_upperbody(pa) && !nogloves(pa)) ? AT_XWEP : AT_CLAW;
					attk->adtyp = AD_PHYS;
					attk->damn = 1;
					attk->damd = 6;
					if(attk->aatyp == AT_CLAW)
						attk->offhand = 1; /*Note: redundant with xwep but needed for claw*/
				}
				else if((attk->aatyp == AT_MSPR || attk->aatyp == AT_ESPR) && (*indexnum%2)){
					if(attk->aatyp == AT_ESPR){
						GETNEXT
					}
					else {
						attk->aatyp = humanoid_upperbody(pa) ? AT_MARI : AT_CLAW;
						attk->adtyp = AD_PHYS;
						attk->damn = 1;
						attk->damd = 6;
					}
				}
			}
		}
		//Most other poison template subs happen when the template is applied, but the moon phase stuff still needs to happen so this had to be postponed.
		if(has_template(magr, POISON_TEMPLATE)){
			if(attk->adtyp == AD_PHYS)
				attk->adtyp = AD_DRCO;
			else if(attk->adtyp == AD_MOON)
				attk->adtyp = AD_EDRC;
		}
	}

	if(magr->mtyp == PM_AVATAR_OF_LOLTH && attk->adtyp == AD_SSEX && (magr->mcan || Protection_from_shape_changers)){
		GETNEXT
	}
	/* Alabaster mummies:
	 * Spell glyphs result in spellcasting,
	 * Physical glyphs result in melee,
	 * Non-specific glyphs result in random choice
	 */
	if (!youagr && *indexnum == 1 && is_alabaster_mummy(pa) && magr->mvar_syllable){
		switch (magr->mvar_syllable){
		case SYLLABLE_OF_STRENGTH__AESH:
		case SYLLABLE_OF_GRACE__UUR:
			// no changes
			break;
		case SYLLABLE_OF_POWER__KRAU:
		case SYLLABLE_OF_THOUGHT__NAEN:
			attk->aatyp = AT_MAGC;
			attk->adtyp = AD_SPEL;
			attk->damn = 0;
			attk->damd = 4;
			break;
		default:
			if (magr->m_id % 2){
				attk->aatyp = AT_MAGC;
				attk->adtyp = AD_SPEL;
				attk->damn = 0;
				attk->damd = 4;
			}
			break;
		}
	}
	
	/*Waterspouts retreat when not casting*/
	if(!by_the_book && pa->mtyp == PM_WATERSPOUT && attk->aatyp == AT_MAGC &&
		(magr->mspec_used || magr->mcan)
	){
		GETNEXT
	}
	/* auto-tailslappers skip their tailslap in main combat sequence */
	if(!by_the_book && attk->aatyp == AT_TAIL && is_tailslap_mon(magr)) {
		GETNEXT
	}
	/* auto-snake-biters skip their other-snake-head attacks in main combat sequence */
	if(!by_the_book && attk->aatyp == AT_OBIT && is_snake_bite_mon(magr)) {
		GETNEXT
	}
	/* auto-rapier-slashers skip their floating rapier attacks in main combat sequence */
	if(!by_the_book && attk->aatyp == AT_ESPR && is_star_blades_mon(magr)) {
		GETNEXT
	}
	if(attk->aatyp == AT_ESPR && magr->mtyp == PM_PORO_AULON && magr->mhp >= magr->mhpmax/2) {
		GETNEXT
	}
	/* the Five Fiends spellcasting */
	if (!by_the_book && (
		(pa->mtyp == PM_LICH__THE_FIEND_OF_EARTH) ||
		(pa->mtyp == PM_KARY__THE_FIEND_OF_FIRE) ||
		(pa->mtyp == PM_KRAKEN__THE_FIEND_OF_WATER) ||
		(pa->mtyp == PM_TIAMAT__THE_FIEND_OF_WIND) ||
		(pa->mtyp == PM_CHAOS) ||
		(pa->mtyp == PM_CAILLEA_ELADRIN) ||
		(pa->mtyp == PM_GAE_ELADRIN)
		)){
		// first index -- determine if only using their spellcasting
		if (*indexnum == 0){
			if (
				(pa->mtyp == PM_LICH__THE_FIEND_OF_EARTH && rn2(4)) ||
				(pa->mtyp == PM_KARY__THE_FIEND_OF_FIRE && rn2(100)<37) ||
				(pa->mtyp == PM_KRAKEN__THE_FIEND_OF_WATER && rn2(100)<52) ||
				(pa->mtyp == PM_TIAMAT__THE_FIEND_OF_WIND && !rn2(4)) ||
				(pa->mtyp == PM_CHAOS && rn2(3)) ||
				(pa->mtyp == PM_DOOM_KNIGHT && !magr->mcan && !magr->mspec_used && !rn2(4)) ||
				(pa->mtyp == PM_GAE_ELADRIN && !magr->mcan && !magr->mspec_used && !rn2(3)) ||
				(pa->mtyp == PM_CAILLEA_ELADRIN && !magr->mcan && !magr->mspec_used)
				){
				add_subout(subout, SUBOUT_SPELLS);
			}
		}
		/* cast only spells if SUBOUT_SPELLS; cast no spells if !SUBOUT_SPELLS */
		if (!is_null_attk(attk) && ((attk->aatyp == AT_MAGC) == !check_subout(subout, SUBOUT_SPELLS))) {
			/* just get the next attack */
			GETNEXT
		}
		if(pa->mtyp == PM_CHAOS && !PURIFIED_FIRE){
			if(attk->aatyp == AT_CLAW && attk->adtyp == AD_SQUE)
				attk->aatyp = AT_MARI;
		}
	}
	/*Lilitus actually skip their spellcasting attack unless the target has their status ailment*/
	if(!by_the_book && pa->mtyp == PM_LILITU && attk->adtyp == AD_CLRC){
		if(!mdef)
			GETNEXT
		else if(youdef && !Doubt)
			GETNEXT
		else if(!youdef && !mdef->mdoubt)
			GETNEXT
	}
	/*Skip chuul tentacles unless the target is grappled*/
	if(!by_the_book && (attk->adtyp == AD_TSMI ||
		(is_chuul(pa) && attk->aatyp == AT_TENT)
	)){
		if(!mdef)
			GETNEXT
		else if(youdef && magr != u.ustuck)
			GETNEXT
	}
	/*Skip grey devourer second and third jaws vs. non-psionics*/
	if(pa->mtyp == PM_GRAY_DEVOURER && attk->aatyp == AT_BITE && attk->adtyp != AD_PHYS){
		if(youdef || !mdef){
			if(base_casting_stat() != A_CHA)
				GETNEXT
		}
		else {
			struct attack * aptr;
			aptr = permonst_dmgtype(mdef->data, AD_PSON);
			if(!(aptr || has_mind_blast_mon(mdef)))
				GETNEXT
		}
	}
	/*Skip spell weaver extra magic attacks until they build up energy */
	if((magr->mtyp == PM_SPELLWEAVER || magr->mtyp == PM_SPELLWEAVER_GODDESS_MOCKER) && attk->aatyp == AT_MAGC){
		if(*indexnum == 0){
			long delta = moves - magr->mvar_spellweaver_last_cast - 1;
			if(delta > 0){
				if(magr->mvar_spellweaver_count > delta)
					magr->mvar_spellweaver_count -= delta;
				else
					magr->mvar_spellweaver_count = 0;
			}
		}
		if(*indexnum > magr->mvar_spellweaver_count)
			GETNEXT
	}
	/* Nitocris uses clerical spells while wearing their Wrappings */
	if(!by_the_book && pa->mtyp == PM_NITOCRIS){
		if (attk->aatyp == AT_MAGC){
			if (which_armor(magr, W_ARMC) && which_armor(magr, W_ARMC)->oartifact == ART_SPELL_WARDED_WRAPPINGS_OF_){
				attk->adtyp = AD_CLRC;
			}
		}
	}
	/* Dream leech dream leeches disappear when cancelled */
	if(has_template(magr, DREAM_LEECH) && magr->mcan &&attk->aatyp == AT_TUCH && attk->adtyp == AD_DRIN){
		GETNEXT
	}
	/* Tettigon touch attack is magical and can be cancelled */
	if(magr->mtyp == PM_TETTIGON_LEGATUS && magr->mcan &&attk->aatyp == AT_TUCH){
		GETNEXT
	}
	/* Skip kicks with wounded legs */
	if(!youagr && magr->mwounded_legs && attk->aatyp == AT_KICK){
		GETNEXT
	}
	/* Skip attacks if stunned */
	if(!youagr && magr->mstun && !by_the_book && !rn2(6)){
		GETNEXT
	}
	/* Man flies actually have only a 1/10th chance to barf */
	if(pa->mtyp == PM_MAN_FLY && attk->aatyp == AT_VOMT && !by_the_book && rn2(10)){
		GETNEXT
	}
		
	if(!by_the_book && 
		(pa->mtyp == PM_SMALL_GOAT_SPAWN || pa->mtyp == PM_GOAT_SPAWN || pa->mtyp == PM_GIANT_GOAT_SPAWN || pa->mtyp == PM_XUENU_MONK)
	){
		// first index -- determine if using the alternate attack set (one seduction attack)
		if (*indexnum == 0){
			if (youdef){
				boolean engring = FALSE;
				if ((uleft  && uleft->otyp == find_engagement_ring()) ||
					(uright && uright->otyp == find_engagement_ring()))
					engring = TRUE;
				if(pd && (!(dmgtype(pd, AD_SEDU)
					|| dmgtype(pd, AD_SSEX)
					|| dmgtype(pd, AD_LSEX)
					|| !could_seduce(magr,mdef,0)
					|| !(uarm || uarmu || uarmh || uarmg || uarmf || uarmc || uwep || uswapwep)
				))){
					add_subout(subout, SUBOUT_GOATSPWN);
					attk->aatyp = AT_CLAW;
					attk->adtyp = AD_SEDU;
					attk->damn = 0;
					attk->damd = 0;
				}
				else if(attk->adtyp == AD_SEDU)
					GETNEXT;
			} else {
				if(pd && (!(dmgtype(pd, AD_SEDU)
					|| dmgtype(pd, AD_SSEX)
					|| dmgtype(pd, AD_LSEX)
					|| magr->mcan 
					|| !(mdef && could_seduce(magr,mdef,0))
					|| (mdef && !(which_armor(mdef, W_ARM) || which_armor(mdef, W_ARMU) || which_armor(mdef, W_ARMH) || 
						 which_armor(mdef, W_ARMG) || which_armor(mdef, W_ARMF) || which_armor(mdef, W_ARMC) ||
						 MON_WEP(mdef) || MON_SWEP(mdef)
					))
				))){
					add_subout(subout, SUBOUT_GOATSPWN);
					attk->aatyp = AT_CLAW;
					attk->adtyp = AD_SEDU;
					attk->damn = 0;
					attk->damd = 0;
				}
				else if(attk->adtyp == AD_SEDU)
					GETNEXT;
			}
		}
		else if (check_subout(subout, SUBOUT_GOATSPWN)){
			/* If spellcasting, stop after the first index */
			*attk = noattack;
			return attk;
		}
	}
	if(!by_the_book && attk->aatyp == AT_BKGT){
		switch(rnd(5)){
			case 1:
				attk->aatyp = AT_TUCH;
				//Use noted ad type
			break;
			case 2:
				attk->aatyp = AT_BITE;
				attk->adtyp = AD_PHYS;
			break;
			case 3:
				attk->aatyp = AT_KICK;
				attk->adtyp = AD_PHYS;
			break;
			case 4:
				attk->aatyp = AT_BUTT;
				attk->adtyp = AD_PHYS;
			break;
			case 5:
				attk->aatyp = AT_GAZE;
				attk->adtyp = AD_STDY;
			break;
		}
	}
	if(!by_the_book && attk->aatyp == AT_BKG2){
		switch(rnd(4)){
			case 1:
				attk->aatyp = AT_TENT;
				attk->adtyp = AD_DRST;
			break;
			case 2:
				attk->aatyp = AT_BUTT;
				attk->adtyp = AD_STUN;
			break;
			case 3:
				attk->aatyp = AT_GAZE;
				attk->adtyp = AD_STDY;
			break;
			case 4:
				attk->aatyp = AT_CLAW;
				attk->adtyp = AD_SEDU;
			break;
		}
	}
	/* Moon puppets actually have a variety of special attacks -- shown in pokedex */
	if(has_template(magr, TONGUE_PUPPET) && attk->aatyp == AT_TONG){
		switch(hash((unsigned long)magr->m_id)%6){
			case 0:
				//Use default
			break;
			case 1:
				attk->adtyp = AD_PAIN;
			break;
			case 2:
				attk->adtyp = AD_ECLD;
			break;
			case 3:
				attk->adtyp = AD_ACID;
			break;
			case 4:
				attk->adtyp = AD_VAMP;
			break;
			case 5:
				attk->adtyp = AD_DRIN;
			break;
		}
	}
	/* Bael has alternate attack routines -- not shown in pokedex */
	if (pa->mtyp == PM_BAEL && *indexnum < NATTK && !by_the_book) {
		static const struct attack marilithHands[NATTK] = {
			{ AT_MARI, AD_PHYS, 1, 15 },
			{ AT_MARI, AD_PHYS, 1, 15 },
			{ AT_MARI, AD_PHYS, 1, 15 },
			{ AT_MARI, AD_PHYS, 1, 15 },
			{ AT_MARI, AD_PHYS, 1, 15 },
			{ AT_MARI, AD_PHYS, 1, 15 }
		};
		static const struct attack swordArchon[NATTK] = {
			{ AT_CLAW, AD_ACFR, 3, 7 },
			{ AT_CLAW, AD_ACFR, 3, 7 },
			{ AT_REND, AD_DISN, 7, 1 },
			{ AT_BUTT, AD_EFIR, 9, 1 },
			{ AT_BITE, AD_POSN, 9, 1 },
			{ AT_GAZE, AD_STDY, 1, 9 }
		};
		// first index -- determine which attack form
		if (*indexnum == 0){
			if (!rn2(7)){		// 1/7 of sword archon
				add_subout(subout, SUBOUT_BAEL1);
			}
			else if (!rn2(6)){	// 1/7 of marilith-hands
				add_subout(subout, SUBOUT_BAEL2);
			}
			//else;				// 5/7 of normal
		}
		// If using marilith-hands or sword archon, sub out entire attack chain
		if (check_subout(subout, SUBOUT_BAEL1)){
			*attk = swordArchon[*indexnum];
		}
		else if (check_subout(subout, SUBOUT_BAEL2)){
			*attk = marilithHands[*indexnum];
		}
	}
	/* Blibdoolpoolp switches to a worse attack routine at high insight -- shown in pokedex */
	if (pa->mtyp == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH && *indexnum < NATTK && Insight >= 54) {
		static const struct attack blib_alternate[NATTK] = {
			{ AT_CLAW, AD_SQUE, 4, 8, 0, 0, 1 },
			{ AT_CLAW, AD_SQUE, 4, 8, 0, 1, 0 },
			{ AT_REND, AD_SHRD, 3, 12 },
			{ AT_HUGS, AD_PHYS, 3, 4 },
			{ AT_HITS, AD_TSMI, 0, 0 },
			{ AT_MAGC, AD_CLRC, 2, 8 },
			{ AT_NONE, AD_MROT, 0, 0 }
		};
		*attk = blib_alternate[*indexnum];
	}
	/* Lolth has alternate attack routines -- not shown in pokedex */
	if (pa->mtyp == PM_AVATAR_OF_LOLTH && *indexnum < NATTK && !by_the_book) {
		static const struct attack lolthHands[NATTK] = {
			{ AT_WEAP, AD_PHYS, 4, 8 },
			{ AT_XWEP, AD_PHYS, 4, 8 },
			{ AT_MARI, AD_PHYS, 2, 8 },
			{ AT_MARI, AD_PHYS, 2, 8 },
			{ AT_MARI, AD_PHYS, 1, 8 },
			{ AT_MARI, AD_PHYS, 1, 8 },
			{ AT_MARI, AD_PHYS, 1, 8 },
			{ AT_MARI, AD_PHYS, 1, 8 }
		};
		// first index -- determine which attack form
		if (*indexnum == 0){
			if (!magr->mcan && !Protection_from_shape_changers && rn2(2)){		// 1/2 of marilith-hands
				add_subout(subout, SUBOUT_LOLTH1);
			}
			//else;				// 1/2 of normal
		}
		// If using marilith-hands sub out entire attack chain
		if (check_subout(subout, SUBOUT_LOLTH1)){
			*attk = lolthHands[*indexnum];
		}
	}
	/* Blasphemous hands have lurker attacks if they can't abduct the target to their temple*/
	if(pa->mtyp == PM_BLASPHEMOUS_HAND && magr && *indexnum < NATTK && (In_endgame(&u.uz) || on_level(&bridge_temple, &u.uz))){
		static const struct attack altblas[NATTK] = {
			{ AT_TUCH, AD_PHYS, 4, 8 }
		};
		int atypes[] = {AD_DETH, AD_PEST, AD_FAMN, AD_CNFT};
		*attk = altblas[*indexnum];
		if(attk->aatyp == AT_TUCH)
			attk->adtyp = atypes[(int) magr->m_id % SIZE(atypes)];
	}

	/* Various weapons can cause an additional full attack to be made */
	/* This only works if it is wielded in the mainhand, and a weapon attack is being made with it (AT_WEAP, AT_DEVA) */
	/* not shown in pokedex */
	if ((attk->aatyp == AT_WEAP || attk->aatyp == AT_DEVA) && !check_subout(subout, SUBOUT_MAINWEPB) && !by_the_book) {
		/* get weapon */
		struct obj * otmp = (youagr ? uwep : MON_WEP(magr));
		/* continue checking conditions */
		if (otmp && (
			(otmp->oartifact == ART_STAFF_OF_TWELVE_MIRRORS)
			|| (otmp->oartifact == ART_QUICKSILVER && mlev(magr) > 15)
			// || (youagr && martial_bonus() && otmp->otyp == QUARTERSTAFF && P_SKILL(P_QUARTERSTAFF) >= P_EXPERT && P_SKILL(P_BARE_HANDED_COMBAT) >= P_EXPERT)
			)
		){
			/* make additional attacks */
			attk->aatyp = AT_WEAP;
			attk->adtyp = AD_PHYS;
			attk->damn = 1;
			attk->damd = 4;
			if (otmp->oartifact == ART_QUICKSILVER)
				*tohitmod = -15;
			fromlist = FALSE;
			remove_subout(subout, SUBOUT_XWEP); /* allow another followup offhand attack if twoweaponing (unpoly'd only) */
			add_subout(subout, SUBOUT_MAINWEPB);
		}
	}

	/* Unpolyed player Barbarians get up to 2 bonus attacks (and offhand attacks) */
	/* Would be shown in pokedex, if you could look up yourself in it. */
	if (youagr && !Upolyd && Role_if(PM_BARBARIAN) && (*indexnum > 0)
			&& ((is_null_attk(attk) || (attk->aatyp != AT_WEAP && attk->aatyp != AT_XWEP))
			&& (prev_attack.aatyp == AT_WEAP || prev_attack.aatyp == AT_XWEP))) {
		int nattacks = (u.ulevel >= 14) + (u.ulevel >= 30);
		int attacknum = (!!check_subout(subout, SUBOUT_BARB1) + !!check_subout(subout, SUBOUT_BARB2));

		if (attacknum < nattacks)
		{
			attk->aatyp = AT_WEAP;
			attk->adtyp = AD_PHYS;
			attk->damn = 1;
			attk->damd = 4;
			*tohitmod = -10 * (attacknum+1); //Correct off-by-one error, these are the *subout* attacks specifically
			fromlist = FALSE;
			if(check_subout(subout, SUBOUT_BARB1)){
				add_subout(subout, SUBOUT_BARB2);
			}
			else {
				add_subout(subout, SUBOUT_BARB1);
			}
			remove_subout(subout, SUBOUT_XWEP);	/* allow another followup offhand attack if twoweaponing */
		}
	}

	/* Some armor can insert attacks */
	/* Warn address doesn't like is_null_attk() with a local variable structs*/
#pragma GCC diagnostic ignored "-Waddress"
	boolean prevattacknull = is_null_attk(&prev_attack);
#pragma GCC diagnostic pop
	if ((is_null_attk(attk) || (attk->aatyp != AT_WEAP && attk->aatyp != AT_XWEP))
		&& (prevattacknull || prev_attack.aatyp == AT_WEAP || prev_attack.aatyp == AT_XWEP || prev_attack.aatyp == AT_MARI)
		&& !(check_subout(subout, SUBOUT_MARIARM1) && check_subout(subout, SUBOUT_MARIARM2))
	){
		struct obj * otmp = (youagr ? uarm : which_armor(magr, W_ARM));
		
		if(!otmp);//Nothing
		else if (otmp->otyp == EILISTRAN_ARMOR 
			&& otmp->altmode == EIL_MODE_ON
			&& otmp->ovar1_eilistran_charges > 0
		){
			attk->aatyp = AT_MARI;
			attk->adtyp = AD_PHYS;
			attk->damn = 2;
			attk->damd = 8;
			fromlist = FALSE;
			if(check_subout(subout, SUBOUT_MARIARM1))
				add_subout(subout, SUBOUT_MARIARM2);
			else
				add_subout(subout, SUBOUT_MARIARM1);
		}
	}

	if ((is_null_attk(attk) || (attk->aatyp != AT_WEAP && attk->aatyp != AT_XWEP))
		&& (prevattacknull || prev_attack.aatyp == AT_WEAP || prev_attack.aatyp == AT_XWEP || prev_attack.aatyp == AT_MARI)
		&& !check_subout(subout, SUBOUT_SHUBTONG)
	){
		if(youagr && check_mutation(MIND_STEALER) && !nomouth(youracedata->mtyp)){
			attk->aatyp = AT_TONG;
			attk->adtyp = AD_FATK;
			attk->damn = 1;
			attk->damd = 4;
			fromlist = FALSE;
			add_subout(subout, SUBOUT_SHUBTONG);
		}
	}

	if ((is_null_attk(attk) || (attk->aatyp != AT_WEAP && attk->aatyp != AT_XWEP))
		&& (prevattacknull || prev_attack.aatyp == AT_WEAP || prev_attack.aatyp == AT_XWEP || prev_attack.aatyp == AT_MARI || prev_attack.aatyp == AT_CLAW)
		&& !(check_subout(subout, SUBOUT_V_CLAWS1) && check_subout(subout, SUBOUT_V_CLAWS2))
	){
		struct obj * otmp = (youagr ? uarm : which_armor(magr, W_ARM));
		
		if(!otmp);//Nothing
		else if (otmp->oartifact == ART_SCORPION_CARAPACE 
			&& check_carapace_mod(otmp, CPROP_CLAWS)
		){
			*attk = *attacktype_fordmg(&mons[PM_VERMIURGE], AT_CLAW, AD_PHYS);
			fromlist = FALSE;
			if(check_subout(subout, SUBOUT_V_CLAWS1))
				add_subout(subout, SUBOUT_V_CLAWS2);
			else
				add_subout(subout, SUBOUT_V_CLAWS1);
		}
	}

	/*Weapon user, not as good without*/
	if (pa->mtyp == PM_DAO_LAO_GUI_MONK && attk->aatyp == AT_WEAP && (
		 (youagr && !uwep) ||
		 (!youagr && !MON_WEP(magr))
	)){
		attk->damn = 1;
		attk->damd = 8;
	}
	
	/*Strong unarmed fighter*/
	if (pa->mtyp == PM_FORMIAN_CRUSHER && (attk->aatyp == AT_WEAP || attk->aatyp == AT_XWEP) && (
		 (youagr && !uwep) ||
		 (!youagr && !MON_WEP(magr))
	)){
		attk->damn = 4;
		attk->damd = 8;
	}
	/* players with the Black Web Entity bound replace unarmed punches with shadow-blade attacks */
	if (youagr && u.specialSealsActive&SEAL_BLACK_WEB && !by_the_book) {
		if ((attk->aatyp == AT_WEAP && !uwep) ||
			(attk->aatyp == AT_XWEP && !uswapwep && u.twoweap) ||
			(attk->aatyp == AT_MARI && !is_android(youracedata))	/* (andr/gyn)oids' mari attacks are psi-held, not actual arms */
			){
			/* replace the attack */
			attk->aatyp = attk->aatyp == AT_WEAP ? AT_SRPR : 
						  attk->aatyp == AT_XWEP ? AT_XSPR : AT_MSPR;
			attk->adtyp = AD_SHDW;
			attk->damn = 4;
			attk->damd = 8;

			/* this is applied to all acceptable attacks; no subout marker is necessary */
		}
	}
	else if (youagr && uring_art(ART_STAR_EMPEROR_S_RING) && !by_the_book){
		if ((attk->aatyp == AT_WEAP && !uwep) ||
			(attk->aatyp == AT_XWEP && !uswapwep && u.twoweap) ||
			(attk->aatyp == AT_MARI && !is_android(youracedata))	/* (andr/gyn)oids' mari attacks are psi-held, not actual arms */
			){
			/* replace the attack */
			attk->aatyp = attk->aatyp == AT_WEAP ? AT_SRPR : 
						  attk->aatyp == AT_XWEP ? AT_XSPR : AT_MSPR;
			attk->adtyp = AD_STAR;
			attk->damn = 4;
			attk->damd = 8;
			/* this is applied to all acceptable attacks; no subout marker is necessary */
		}	
	}
	/* the bestial claw combined with the beast's embrace (or insanity for monsters) allows for a powerful extra claw attk */
	{
	struct obj * otmp = (youagr ? uwep : MON_WEP(magr));
	if(otmp){
		if (youagr && otmp->otyp == BESTIAL_CLAW && active_glyph(BEASTS_EMBRACE)) {
			if (attk->aatyp == AT_XWEP && !uswapwep && u.twoweap) {
				/* replace the attack */
				attk->aatyp = AT_CLAW;
				attk->adtyp = AD_PHYS;
				attk->damn = 1+u.ulevel/8; // from 1 to 4 dice, hitting 4 at xp 24+
				attk->offhand = 1;
				
				// scales inversely with sanity, sanity-based size is 0->11, 10->9, 25->6, 50->3, 75->1, 80->0
				// total dice assuming +7 and xp30 is 4d18 / 4d10 / 4d7 at sanity 15/50/80+
				attk->damd = max(otmp->spe, 1);
				attk->damd += (int)((Insanity/30.0)*(Insanity/30.0) + 0.5);

				/* this is applied to all acceptable attacks; no subout marker is necessary */
			}
		}
		else if (!youagr && otmp->otyp == BESTIAL_CLAW && magr->mcrazed) {
			if (attk->aatyp == AT_XWEP) {
				/* replace the attack */
				attk->aatyp = AT_CLAW;
				attk->adtyp = AD_PHYS;
				attk->damn = 1+min(5, magr->m_lev/8); // from 1 to 6 dice, hitting 6 at xp 40+
				attk->offhand = 1;
				
				attk->damd = max(otmp->spe, 1) + magr->mberserk ? 11 : 3;
				/* this is applied to all acceptable attacks; no subout marker is necessary */
			}	
		}
	}
	}
	

	/* creatures wearing the Grappler's Grasp and currently grappling something get a hug attack if they don't have one already */
	if (is_null_attk(attk) && !by_the_book && !dmgtype(pa, AT_HUGS) && !check_subout(subout, SUBOUT_GRAPPLE)) {
		struct obj * otmp = (youagr ? uarmg : which_armor(magr, W_ARMG));
		/* magr must already have hold of mdef, however, which makes it much less useful mvm */
		if (otmp && (otmp->oartifact == ART_GRAPPLER_S_GRASP || (otmp->otyp == IMPERIAL_ELVEN_GAUNTLETS && check_imp_mod(otmp, IEA_STRANGLE))) &&
			((youagr || youdef) && !u.uswallow && u.ustuck && u.ustuck == (youagr ? mdef : magr))) {
			*attk = grapple;
			attk->damn = youagr ? ((P_SKILL(P_BARE_HANDED_COMBAT) + 1) / 2 + martial_bonus()) : 2;
			add_subout(subout, SUBOUT_GRAPPLE);
			fromlist = FALSE;
		}
	}

	/* creatures wearing the Scorpion Carpace get a scorpion's sting */
	if (is_null_attk(attk) && !by_the_book && !check_subout(subout, SUBOUT_SCORPION)) {
		struct obj * otmp = (youagr ? uarm : which_armor(magr, W_ARM));
		if (otmp && otmp->oartifact == ART_SCORPION_CARAPACE) {
			if(check_carapace_mod(otmp, CPROP_ILL_STING)){
				*attk = *attacktype_fordmg(&mons[PM_SCORPIUS], AT_STNG, AD_DISE);
				add_subout(subout, SUBOUT_SCORPION);
			}
			else {
				*attk = *attacktype_fordmg(&mons[PM_SCORPION], AT_STNG, AD_DRST);
				add_subout(subout, SUBOUT_SCORPION);
			}
			fromlist = FALSE;
		}
	}

	/* Player may get rot stinger attack */
	if (youagr && is_null_attk(attk) && !by_the_book && check_rot(ROT_STING)
		&& !check_subout(subout, SUBOUT_ROT_STING)
	) {
		attk->aatyp = AT_STNG;
		attk->adtyp = AD_PFBT;
		attk->damn = 1;
		attk->damd = 4;
		fromlist = FALSE;
		add_subout(subout, SUBOUT_ROT_STING);
	}

	/* Player may get brain sucker attack */
	if (youagr && is_null_attk(attk) && !by_the_book && u.brainsuckers && active_glyph(LUMEN)
		&& !check_subout(subout, SUBOUT_BRAINSUCK)
	) {
		attk->aatyp = AT_TENT;
		attk->adtyp = AD_DRIN;
		attk->damn = u.brainsuckers;
		attk->damd = 2;
		fromlist = FALSE;
		add_subout(subout, SUBOUT_BRAINSUCK);
	}

	/* Player may get rot vomit attack */
	if (youagr && is_null_attk(attk) && !by_the_book && check_rot(ROT_VOMIT) && (umechanoid || u.uhs < WEAK)
		&& !check_subout(subout, SUBOUT_ROT_VOMIT)
	) {
		if(!rn2(10)){
			attk->aatyp = AT_VOMT;
			attk->adtyp = AD_DISE;
			attk->damn = 1;
			attk->damd = 1;
			fromlist = FALSE;
			make_sick(0L, (char *) 0, TRUE, SICK_VOMITABLE);
			if(!umechanoid) morehungry(20*get_uhungersizemod());
		}
		add_subout(subout, SUBOUT_ROT_VOMIT);
	}

	/* players can get a whole host of spirit attacks */
	if (youagr && is_null_attk(attk) && !by_the_book) {
		/* this assumes that getattk() will not be interrupted with youagr when already called with youagr */
		if (!check_subout(subout, SUBOUT_SPIRITS)) {
			*attk = *(getnextspiritattack(TRUE));
			add_subout(subout, SUBOUT_SPIRITS);
		}
		else {
			*attk = *(getnextspiritattack(FALSE));
		}
		fromlist = FALSE;
	}

	/* Player may get rot spores passive attack */
	if (youagr && is_null_attk(attk) && !by_the_book && check_rot(ROT_SPORES)
		&& !check_subout(subout, SUBOUT_ROT_SPORES)
	) {
		attk->aatyp = AT_NONE;
		attk->adtyp = AD_DISE;
		attk->damn = 1;
		attk->damd = 1;
		fromlist = FALSE;
		add_subout(subout, SUBOUT_ROT_SPORES);
	}
	/* some pseudonatural's claws become more-damaging tentacles */
	if (!youagr && has_template(magr, PSEUDONATURAL) && (
		attk->aatyp == AT_CLAW && (magr->m_id + *indexnum) % 4 == 0)
		){
		attk->aatyp = AT_TENT;
		if (attk->damn < 3)
			attk->damd += 2;
		else
			attk->damn++;
	}

	/* Undead damage multipliers -- note that these must be after actual replacements are done */
	/* zombies deal double damage, and all undead deal double damage at midnight (the midnight multiplier is not shown in the pokedex) */
	if (!youagr && (has_template(magr, ZOMBIFIED) || has_template(magr, M_BLACK_WEB)) && (is_undead(pa) && midnight() && !by_the_book))
		attk->damn *= 3;
	else if (!youagr && (has_template(magr, ZOMBIFIED) || has_template(magr, M_BLACK_WEB) || (is_undead(pa) && midnight() && !by_the_book)))
		attk->damn *= 2;

	/* Bandersnatches become frumious instead of fleeing, dealing double damage -- not shown in the pokedex */
	if (!youagr && magr->mflee && pa->mtyp == PM_BANDERSNATCH && !by_the_book)
		attk->damn *= 2;

	/* prevent a monster with two consecutive disease or hunger attacks
	from hitting with both of them on the same turn; if the first has
	already hit, switch to a stun attack for the second */
	if (!by_the_book && *indexnum > 0 && prev_res[1] &&
		(attk->adtyp == AD_DISE ||
		// attk->adtyp == AD_PEST ||
		// attk->adtyp == AD_FAMN ||
		pa->mtyp == PM_ASTRAL_DEVA) &&
		attk->adtyp == prev_attack.adtyp
		) {
		attk->adtyp = AD_STUN;
	}
	/* Specific cases that prevent attacks */
	if (!by_the_book && !is_null_attk(attk) && (
		/* If player is the target and is engulfed, only targetable by engulf attacks */
		(youdef && u.uswallow && (attk->aatyp != AT_ENGL && attk->aatyp != AT_ILUR)) ||
		/* If player was using 'k' to kick, they are only performing kick attacks (onlykicks is a state variable defined in dokick.c) */
		(youagr && onlykicks && attk->aatyp != AT_KICK) ||
		/* If player is stuck in a straitjacket */
		(youagr && 
			((Straitjacketed) ||
			 (uarmg && uarmg->otyp == SHACKLES && uarmg->cursed)
			) && (
			attk->aatyp == AT_WEAP || attk->aatyp == AT_XWEP || attk->aatyp == AT_WHIP
			|| attk->aatyp == AT_HODS || attk->aatyp == AT_MMGC 
			/* "Deva" rapiers are assumed to be the Masked Queen's lower arms, and "Energy" rapiers just sorta float or something */
			|| attk->aatyp == AT_SRPR || attk->aatyp == AT_XSPR || attk->aatyp == AT_MSPR
			|| attk->aatyp == AT_DEVA || attk->aatyp == AT_5SQR || attk->aatyp == AT_MARI
			|| (attk->aatyp == AT_MAGC && attk->adtyp != AD_PSON)
			|| (humanoid_torso(youracedata) && (
				attk->aatyp == AT_TUCH
				|| attk->aatyp == AT_LRCH
			))
			|| (humanoid(youracedata) && (
				attk->aatyp == AT_CLAW
				|| attk->aatyp == AT_HUGS
			))
		)) ||
		/* If monster is stuck in a straitjacket */
		(!youagr && 
			( straitjacketed_mon(magr) ) && 
			!(check_subout(subout, SUBOUT_BAEL1) || check_subout(subout, SUBOUT_BAEL2)) && (
			attk->aatyp == AT_WEAP || attk->aatyp == AT_XWEP || attk->aatyp == AT_WHIP
			|| attk->aatyp == AT_HODS || attk->aatyp == AT_MMGC 
			/* "Deva" rapiers are assumed to be the Masked Queen's lower arms, and "Energy" rapiers just sorta float or something */
			|| attk->aatyp == AT_SRPR || attk->aatyp == AT_XSPR || attk->aatyp == AT_MSPR
			|| attk->aatyp == AT_DEVA || attk->aatyp == AT_5SQR || attk->aatyp == AT_MARI
			|| ((attk->aatyp == AT_MAGC || attk->aatyp == AT_MMGC) && attk->adtyp != AD_PSON && !(magr->mtyp == PM_ITINERANT_PRIESTESS && has_template(magr, MISTWEAVER))) ||
			(humanoid(pa) && (
				attk->aatyp == AT_CLAW
				/* Note: Dream-leech "touch" attacks are the dream leeches eating your brain like a 'flayer */
				|| (attk->aatyp == AT_TUCH && !(attk->adtyp == AD_DRIN && has_template(magr, DREAM_LEECH)))
				|| attk->aatyp == AT_HUGS
				|| attk->aatyp == AT_LRCH
			))
		)) ||
		/* If player is wearing a faceless helm */
		(youagr && 
			(
			 (uarmh && FacelessHelm(uarmh)) ||
			 (uarmc && FacelessCloak(uarmc))
			) && (
			attk->aatyp == AT_BITE
		)) ||
		/* If monster is stuck in a straitjacket */
		(!youagr && 
			(
			 covered_face_mon(magr)
			) && (
			attk->aatyp == AT_BITE
		)) ||
		/* Illurien can only engulf targets she is stuck to */
		(youdef && mdef && pa->mtyp == PM_ILLURIEN_OF_THE_MYRIAD_GLIMPSES && attk->aatyp == AT_ENGL && (u.ustuck != magr)) ||
		/* Rend attacks only happen if the previous two attacks hit */
		(attk->aatyp == AT_REND && 
			(prev_res[1] == MM_MISS || prev_res[2] == MM_MISS || 
			(magr->mtyp == PM_SARTAN_TANNIN && prev_res[3] == MM_MISS))) ||
		/* Hugs attacks are similar, but will still happen if magr and mdef are stuck together */
		(attk->aatyp == AT_HUGS && (prev_res[1] == MM_MISS || prev_res[2] == MM_MISS)
			&& !(mdef && ((youdef && u.ustuck == magr) || (youagr && u.ustuck == mdef)))) ||
		/* Demogorgon's item-stealing gaze only happens if previous two gazes worked AND he is close to his target */
		(pa->mtyp == PM_DEMOGORGON && mdef && attk->aatyp == AT_GAZE && attk->adtyp == AD_SEDU && (
			prev_res[1] == MM_MISS || prev_res[2] == MM_MISS))
		))
	{
		/* just get the next attack */
		if (fromlist) {
			*indexnum += 1;
		}
		*prev_and_buf = prev_attack;
		return getattk(magr, mdef, prev_res, indexnum, prev_and_buf, by_the_book, subout, tohitmod);
	}

	/* possibly increment indexnum, if we want to move on in the monster's attack list 
	 * this is most of the time, except for when we have inserted attacks into [magr]'s chain */
	if (fromlist) {
		*indexnum += 1;
	}
	return attk;
}
#undef GETNEXT

/* noises()
 * prints noises from mvm combat
 */
void
noises(magr, attk)
struct monst * magr;
struct attack * attk;
{
	/* static variable prevent message spam */
	static boolean far_noise = FALSE;
	static long noisetime = 0;

	boolean farq = (distu(x(magr), y(magr)) > 15);

	if (flags.soundok && (farq != far_noise || moves - noisetime > 10)) {
		far_noise = farq;
		noisetime = moves;
		You_hear("%s%s.",
			(attk->aatyp == AT_EXPL) ? "an explosion" : "some noises",
			farq ? " in the distance" : "");
	}
}

/* xymissmsg()
 *
 * Prints an appropriate basic miss message (You miss foo, foo misses, foo misses bar)
 * Also does "noises" for when mvm combat isn't visible at all
 *
 * Also stops occupation() for the player
 */
void
xymissmsg(magr, mdef, attk, vis, nearmiss)
struct monst * magr;
struct monst * mdef;
struct attack * attk;
int vis;
boolean nearmiss;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	boolean compat = (could_seduce(magr, mdef, attk) && (youagr || (!magr->mcan && !magr->mspec_used)));

	if (youagr) {
		if (compat)
			You("pretend to be friendly to %s.", mon_nam(mdef));
		else if (youdef)
			You("fail to hit yourself. How embarrassing!");
		else if (flags.verbose)
			You("miss %s.", mon_nam(mdef));
		else
			You("miss it.");
	}
	else if (youdef) {
		if (!(vis&VIS_MAGR))
			map_invisible(x(magr), y(magr));

		if (compat)
			pline("%s pretends to be friendly.", Monnam(magr));
		else if (flags.verbose && nearmiss)
			pline("%s just misses!", Monnam(magr));
		else
			pline("%s misses.", Monnam(magr));
		/* interrupt player */
		stop_occupation();
	}
	else {
		if (vis) {
			if (!(vis&VIS_MAGR))
				map_invisible(x(magr), y(magr));
			if (!(vis&VIS_MDEF))
				map_invisible(x(mdef), y(mdef));

			pline("%s %s %s.",
				Monnam(magr),
				(compat ? "pretends to be friendly to" : "misses"),
				mon_nam_too(mdef, magr)
				);
		}
		else {
			noises(magr, attk);
		}
	}
	return;
}

/* xyhitmsg() 
 * 
 * Prints an appropriate basic hit message (foo hits bar, foo hits, you hit foo)
 * 
 * Do not call this if the attack shouldn't be visible!
 */
void
xyhitmsg(magr, mdef, attk)
struct monst *magr;
struct monst *mdef;
struct attack *attk;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct permonst * pa = youagr ? youracedata : magr->data;
	struct permonst * pd = youdef ? youracedata : mdef->data;
	int compat;

	/* AT_NONE is silent */
	if (attk->aatyp == AT_NONE)
		return;

	/* Note: if opposite gender, "seductively" */
	/* If same gender, "engagingly" for nymph, normal msg for others */
	if ((compat = could_seduce(magr, mdef, attk))
		&& !magr->mcan && !magr->mspec_used) {
		pline("%s %s %s %s.",
			(youagr ? "You" : Monnam(magr)),
			(youagr ?
				(!is_blind(mdef) ? "smile at" : !is_silent_mon(magr) ? "talk to" : "motion at")
				:
				(!is_blind(mdef) ? "smiles at" : !is_silent_mon(magr) ? "talks to" : "motions at")
				),
			((youdef && !youagr) ? "you" : mon_nam_too(mdef, magr)),
			compat == 2 ? "engagingly" : "seductively");
	}
	else {
		boolean specify_you = FALSE;
		const char * verb = (const char *)0;
		const char * ending = "!";
		/* You X foo!	(youagr)
		 * Foo Xs!		(youdef, !youagr)
		 * Foo Xs bar!	()
		 * 
		 * <agressor> X (maybe plural) <defender if not you> <rest of string>
		 */	
		if(attk->adtyp == AD_LICK){
			verb = "lick";
			if (youdef)
				specify_you = TRUE;
			pline("%s %s%s%s%s%s%s",
				(youagr ? "You" : Monnam(magr)),
				(is_weeping(pa) && !youagr && !youdef ? "is " : ""),
				(youagr || (is_weeping(pa) && !youdef) ? verb : makeplural(verb)),
				(is_weeping(pa) && !youagr && !youdef ? "ing" : ""),
				((youdef && !youagr && !specify_you) ? "" : " "),
				((youdef && !youagr && !specify_you) ? "" : mon_nam_too(mdef, magr)),
				ending
				);
		}
		else switch (attk->aatyp) {
		case AT_BEAM:
			if (!verb) verb = "blast";
			// fall through
		case AT_LNCK:
		case AT_BITE:
		case AT_5SBT:
			if (!verb) verb = "bite";
			// fall through
		case AT_KICK:
			if (!verb) verb = "kick";
			if (attk->aatyp == AT_KICK &&
				(thick_skinned(mdef->data) || (youdef && u.sealsActive&SEAL_ECHIDNA)))
				ending = ".";
			// fall through
		case AT_STNG:
			if (!verb) verb = "sting";
			// fall through
		case AT_BUTT:
			if (!verb) verb = "butt";
			// fall through
		case AT_SRPR:
		case AT_XSPR:
		case AT_MSPR:
		case AT_DSPR:
		case AT_ESPR:
				if (!verb){
					verb = "slash";
					ending = (attk->adtyp == AD_SHDW) ? " with bladed shadows!" :
						(attk->adtyp == AD_STAR) ? " with a starlight rapier!" :
						(attk->adtyp == AD_BSTR) ? " with a black-star rapier!" :
						(attk->adtyp == AD_MOON) ? " with a moonlight rapier!" :
						(attk->adtyp == AD_HOLY) ? " with a holy light-beam!" :
						(attk->adtyp == AD_UNHY) ? " with a unholy light-blade!" :
						(attk->adtyp == AD_HLUH) ? " with a corrupted light-blade!" :
						(attk->adtyp == AD_MERC) ? " with a blade of mercury!" :
						(attk->adtyp == AD_WET) ? " with a water-jet blade!" :
						(attk->adtyp == AD_PSON) ? " with a soul blade!" :
						(attk->adtyp == AD_BLUD) ? " with a blade of blood!" :
						(attk->adtyp == AD_EFIR) ? " with a blade of fire!" :
						(attk->adtyp == AD_EACD) ? " with a blade of acid!" :
						(attk->adtyp == AD_SVPN) ? " with a pitch-black blade!" :
						(attk->adtyp == AD_EDRC) ? " with a blade of poison!" : "!";
					if (youdef)
						specify_you = TRUE;
				}
		case AT_VINE:
				if (!verb){
					verb = "touch";
					ending = (attk->adtyp == AD_DRLI) ? " with withering vines!" :
						(attk->adtyp == AD_ECLD) ? " with hoarfrosted vines!" :
						(attk->adtyp == AD_SHRD) ? " with growing vines!" :
						(attk->adtyp == AD_POLN) ? " with flowering vines!" : 
						" with crawling vines!";
					if (youdef)
						specify_you = TRUE;
				}
		case AT_5SQR:
		case AT_TUCH:
			if (!verb) {
				if (attk->adtyp == AD_SHDW){
					verb = "slash";
					ending = " with bladed shadows!";
					if (youdef)
						specify_you = TRUE;
				} else if(has_template(magr, DREAM_LEECH) && attk->adtyp == AD_DRIN){
					pline("%s ghostly leeches suck %s!",
						(youagr ? "Your" : s_suffix(Monnam(magr))),
						((youdef && !youagr) ? "you" : mon_nam_too(mdef, magr))
						);
					break;
				} else {
					verb = "touch";
					if (youdef)
						specify_you = TRUE;
				}
			}
			/* print the message */
			/* weeping angels are present tense "The weeping angel is touching foo" only if you are neither magr nor mdef */
			pline("%s %s%s%s%s%s%s",
				(youagr ? "You" : Monnam(magr)),
				(is_weeping(pa) && !youagr && !youdef ? "is " : ""),
				(youagr || (is_weeping(pa) && !youdef) ? verb : makeplural(verb)),
				(is_weeping(pa) && !youagr && !youdef ? "ing" : ""),
				((youdef && !youagr && !specify_you) ? "" : " "),
				((youdef && !youagr && !specify_you) ? "" : mon_nam_too(mdef, magr)),
				ending
				);
			break;
			/* some more special cases */
		case AT_TENT:
			pline("%s tentacles %s %s!",
				(youagr ? "Your" : s_suffix(Monnam(magr))),
				(attk->adtyp == AD_PAIN ? "sting" : "suck"),
				((youdef && !youagr) ? "you" : mon_nam_too(mdef, magr))
				);
			break;
		case AT_OBIT:
			pline("%s %s bites %s!",
				(youagr ? "Your" : s_suffix(Monnam(magr))),
				(attk->adtyp == AD_MAGM ? "skirt" : magr->mtyp == PM_MEDUSA ? "hair" : magr->mtyp == PM_MOON_S_CHOSEN ? "cranial wolfpack" : magr->mtyp == PM_HYGIEIAN_ARCHON ? "snake" : magr->mtyp == PM_ANCIENT_NAGA ? "canopy" : attk->adtyp == AD_DISE ? "centipede" : "snake head"),
				((youdef && !youagr) ? "you" : mon_nam_too(mdef, magr))
				);
			break;
		case AT_TAIL:
			pline("%s slap%s %s with %s tail!",
				(youagr ? "You" : Monnam(magr)),
				(youagr ? "" : "s"),
				((youdef && !youagr) ? "you" : mon_nam_too(mdef, magr)),
				(youagr ? "your" : mhis(magr))
				);
			break;
		case AT_VOMT:
			pline("%s vomit%s %s %s!",
				(youagr ? "You" : Monnam(magr)),
				(youagr ? "" : "s"),
				(pd->msize > pa->msize ? "on" : "all over"),
				((youdef && !youagr) ? "you" : mon_nam_too(mdef, magr))
				);
			break;
		case AT_TONG:
			if(attk->adtyp == AD_FATK
			|| attk->adtyp == AD_PAIN
			|| attk->adtyp == AD_DRIN
			){
				pline("%s slip%s %s needle-thin %s into %s %s through %s %s!",
					(youagr ? "You" : Monnam(magr)),
					(youagr ? "" : "s"),
					(youagr ? "your" : mhis(magr)),
					(youagr ? body_part(TONGUE) : mbodypart(mdef, TONGUE)),
					(youdef ? "your" : mhis(mdef)),
					(youdef ? body_part(BRAIN) : mbodypart(mdef, BRAIN)),
					(youdef ? "your" : mhis(mdef)),
					(youdef ? body_part(EAR) : mbodypart(mdef, EAR))
					);
			}
			else if(attk->adtyp == AD_VAMP){
				pline("%s stab%s %s needle-thin %s into %s %s!",
					(youagr ? "You" : Monnam(magr)),
					(youagr ? "" : "s"),
					(youagr ? "your" : mhis(magr)),
					(youagr ? body_part(TONGUE) : mbodypart(mdef, TONGUE)),
					(youdef ? "your" : mhis(mdef)),
					(youdef ? body_part(NECK) : mbodypart(mdef, NECK))
					);
			}
			else if(attk->adtyp == AD_ECLD
			|| attk->adtyp == AD_ACID
			){
				pline("%s lick%s %s with %s impossibly-long tongue!",
					(youagr ? "You" : Monnam(magr)),
					(youagr ? "" : "s"),
					((youdef && !youagr) ? "you" : mon_nam_too(mdef, magr)),
					(youagr ? "your" : mhis(magr))
					);
			}
			else if(attk->adtyp == AD_PLYS){
				pline("%s lick%s %s!",
					(youagr ? "You" : Monnam(magr)),
					(youagr ? "" : "s"),
					((youdef && !youagr) ? "you" : mon_nam_too(mdef, magr))
					);
			}
			else {
				pline("%s lash%s %s with %s tongue!",
					(youagr ? "You" : Monnam(magr)),
					(youagr ? "" : "es"),
					((youdef && !youagr) ? "you" : mon_nam_too(mdef, magr)),
					(youagr ? "your" : mhis(magr))
					);
			}
			break;
		case AT_WBIT:
			pline("%s waist-wolf bites %s!",
				(youagr ? "Your" : s_suffix(Monnam(magr))),
				((youdef && !youagr) ? "you" : mon_nam_too(mdef, magr))
				);
			break;
		case AT_WHIP:
			pline("%s barbed whips lash %s!",
				(youagr ? "Your" : s_suffix(Monnam(magr))),
				((youdef && !youagr) ? "you" : mon_nam_too(mdef, magr))
				);
			break;
		case AT_HUGS:
			pline("%s %s being %s.",
				(youdef ? "You" : Monnam(mdef)),
				(youdef ? "are" : "is"),
				(pa->mtyp == PM_ROPE_GOLEM ? "choked" : "crushed")
				);
			break;
		case AT_EXPL:
		case AT_BOOM:
			pline("%s %s!",
				(youagr ? "You" : Monnam(magr)),
				(youagr ? "explode" : makeplural("explode"))
				);
			break;
		case AT_NONE:
			break;
		default:
			pline("%s %s%s%s!",
				(youagr ? "You" : Monnam(magr)),
				(youagr ? "hit" : makeplural("hit")),
				((youdef && !youagr) ? "" : " "),
				((youdef && !youagr) ? "" : mon_nam_too(mdef, magr))
				);
			break;
		}
	}
	return;
}

void
xswingsy(magr, mdef, otmp, ranged)
struct monst *magr, *mdef;
struct obj *otmp;
boolean ranged;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	char buf[BUFSZ];
	char buf2[BUFSZ];

	/* don't make a "You swing" message */
	if (youagr)
		return;
	/* don't message if there is no weapon */
	if (!otmp)
		return;
	/* don't message if you're blind or can't see magr (Nero asks: why the blind check?) */
	if (Blind || !canseemon(magr))
		return;
	/* don't message if we are not being verbose about combat, unless it's a thrust polearm */
	if (!flags.verbose && !ranged)
		return;
	/* edge case: hmon takes care of these special hitmessages */
	if (otmp->otyp == EGG)
		return;

	if (!ranged) {
		char *swingwords[] = {"thrusts", "swings"};
		int swingindex;
		int mask = attack_mask(otmp, 0, 0, magr);
		
		if(!(mask&PIERCE))
			swingindex = 1;
		else if(weapon_type(otmp) == P_PICK_AXE || weapon_type(otmp) == P_MORNING_STAR || weapon_type(otmp) == P_FLAIL || weapon_type(otmp) == P_WHIP)
			swingindex = 1;
		else if(mask == PIERCE)
			swingindex = 0;
		else if(resist_pierce(mdef->data) && ((mask&WHACK && !resist_blunt(mdef->data)) || (mask&SLASH && !resist_slash(mdef->data))))
			swingindex = 1;
		else if(!resist_pierce(mdef->data) && !((mask&WHACK && !resist_blunt(mdef->data)) || (mask&SLASH && !resist_slash(mdef->data))))
			swingindex = 0;
		else
			swingindex = !rn2(3) ? 0 : 1;
		Sprintf(buf, "%s %s %s %s",
			Monnam(magr),
			swingwords[swingindex],
			mhis(magr),
			singular(otmp, xname)
			);
	}
	else {
		Sprintf(buf, "%s %s %s", Monnam(magr), otmp->otyp == AKLYS ? "throws" : "thrusts",
			obj_is_pname(otmp) ? the(xname(otmp)) : an(xname(otmp)));
	}

	if (youdef) {
		Sprintf(buf2, "%s!", buf);
	}
	else {
		Sprintf(buf2, "%s at %s.", buf, mon_nam(mdef));
	}

	pline("%s", buf2);
	return;
}

/* slips_free() 
 * 
 * Checks whether slippery clothing protects from hug or wrap attack.
 * 
 * Prints messages.
 */
boolean
slips_free(magr, mdef, attk, vis)
struct monst * magr;
struct monst * mdef;
struct attack * attk;
int vis;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct obj *obj = (struct obj *)0;

	if (vis == -1)
		vis = getvis(magr, mdef, 0, 0);
	
	switch (attk->adtyp) {
		/* INT attacks always target heads */
	case AD_DRIN:
		obj = (youdef ? uarmh : which_armor(mdef, W_ARMH));
		break;
		/* this is assumed to be an attack to the hands, draining DEX? */
	case AD_DRDX:
		obj = (youdef ? uarmg : which_armor(mdef, W_ARMG));
		break;
		/* attack to the legs */
	case AD_LEGS:
		obj = (youdef ? uarmf : which_armor(mdef, W_ARMF));
		break;
		/* wrap attacks are specifically blocked by mud boots, in addition to body armors */
	case AD_WRAP:
		obj = (youdef ? uarmf : which_armor(mdef, W_ARMF));
		if (obj && obj->otyp == find_mboots())
			break;
		else {
			obj = (struct obj *)0;
			/*fall through to default (body armors)*/
		}
	default:
		if (!obj) obj = (youdef ? uarmc : which_armor(mdef, W_ARMC));
		if (!obj) obj = (youdef ? uarm : which_armor(mdef, W_ARM));
		if (!obj) obj = (youdef ? uarmu : which_armor(mdef, W_ARMU));
	}
	
	//Star spawn bypass most helmets.
	if(magr && (magr->mtyp == PM_STAR_SPAWN || magr->mtyp == PM_DREAM_EATER || magr->mtyp == PM_VEIL_RENDER) && obj && obj->oartifact != ART_APOTHEOSIS_VEIL)
		return FALSE;

	if (obj && (
		(obj->greased || obj->otyp == OILSKIN_CLOAK) ||			/* greased (or oilskin) armor */
		(attk->adtyp == AD_WRAP && obj->otyp == find_mboots())	/* mud boots vs wrap attacks */
		)
		&&
		!(obj->cursed && !rn2(3))							/* 1/3 chance to fail when cursed */
		) {
		/* print appropriate message */
		if (vis&VIS_MDEF) {
			pline("%s %s%s%s %s %s %s!",
				(youagr ? "You" : Monnam(magr)),
				(attk->adtyp == AD_WRAP ? "slip" : "grab"),
				(youagr ? "" : "s"),
				(attk->adtyp == AD_WRAP ? " off of" : ", but cannot hold onto"),
				(youdef ? "your" : s_suffix(mon_nam(mdef))),
				(obj->greased ? "greased" : "slippery"),
				((obj->otyp == OILSKIN_CLOAK && !objects[obj->otyp].oc_name_known)
				? cloak_simple_name(obj) : obj->otyp == find_mboots() ? "mud boots" : xname(obj))
				);
		}
		/* remove grease (50% odds) */
		if (obj->greased && !rn2(2)) {
			if (vis&VIS_MDEF) {
				pline_The("grease wears off.");
			}
			obj->greased = 0;
		}
		return TRUE;
	}
	return FALSE;
}


/* heal()
 * 
 * increases [mon]'s hp by [amnt], up to [mon]'s maxhp
 * sets flags.botl when applicable
 */
void
heal(mon, amnt)
struct monst * mon;
int amnt;
{
	*hp(mon) += amnt;
	if (*hp(mon) > *hpmax(mon))
		*hp(mon) = *hpmax(mon);
	if (mon == &youmonst)
		flags.botl = 1;
	return;
}


/* xdamagey()
 * 
 * Deals [dmg] damage to [mdef].
 *
 * Rewards [magr] as appropriate.
 * 
 * Returns:
 * MM_HIT		0x01	(possible) aggressor hit defender (usually, except when defender lifesaved)
 * MM_DEF_DIED	0x02	(possible) defender died (defender died and was not lifesaved)
 * MM_AGR_DIED	0x04	(never) aggressor died
 * MM_AGR_STOP	0x08	(never) aggressor should stop attacking for some reason
 * 
 */
int
xdamagey(magr, mdef, attk, dmg)
struct monst * magr;	/* attacker (might not exist) */
struct monst * mdef;	/* defender */
struct attack * attk;	/* attack used to deal damage (might not exist) */
int dmg;				/* damage to deal */
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);

	const char * oldkiller = killer;
	killer = 0;

	if(dmg > 0 && youdef && youagr){
		dmg = (dmg - ACURR(A_DEX))/2;
		dmg = max(1, dmg);
	}

	/* if defender is already dead, avoid re-killing them; just note that they are dead */
	if (*hp(mdef) < 1) {
		return (MM_HIT|MM_DEF_DIED);
	}
	/* brainblooms replicate */
	if(!youdef && magr && mdef && magr->mtyp == PM_BRAINBLOSSOM_PATCH && !mindless_mon(mdef)){
		mdef->brainblooms = 1;
	}
	/* debug */
	if (wizard && (iflags.wizcombatdebug & WIZCOMBATDEBUG_DMG) && WIZCOMBATDEBUG_APPLIES(magr, mdef))
		pline("(dmg = %d)", dmg);
	/* deal damage */
	*hp(mdef) -= dmg;
	if (*hp(mdef) > *hpmax(mdef))
		*hp(mdef) = *hpmax(mdef);

	/* mhitu */
	if (youdef) {
		stop_occupation();
		flags.botl = 1;
		u.total_damage += dmg;
		if (dmg > 0 && magr)
			magr->mhurtu = TRUE;
		/* the golden knight saves you from dying from hp loss */
		if (uarms && uarms->oartifact == ART_GOLDEN_KNIGHT && u.uhp < 1 && (u.uhp*-2 < u.uen) && !Upolyd)
		{	
			Your("power pours into your shield, and your mortal wounds close!");
			healup(u.uen, 0, FALSE, FALSE); losepw(u.uen);
			change_gevurah(1); //cheated death.
		}
		/* messages */
		if ((dmg > 0) && (*hp(mdef) > 0) && (*hp(mdef) * 10 < *hpmax(mdef)) && !(Upolyd && !Unchanging))
			maybe_wail();
		if (*hp(mdef) < 1) {
			if (Upolyd && !Unchanging){
				rehumanize();
				change_gevurah(1); //cheated death.
			}
			else if (magr && !oldkiller)
				done_in_by(magr);
			else {
				killer = oldkiller;
				You("die...");
				if (!u.uconduct.killer && !youagr){
					//Pcifist PCs aren't combatants so if something kills them up "killed peaceful" type impurities
					IMPURITY_UP(u.uimp_murder)
					IMPURITY_UP(u.uimp_bloodlust)
				}
				done(DIED);
				killer = 0;
			}
			if (*hp(mdef) > 0)
				return (MM_HIT|MM_DEF_LSVD);	/* you lifesaved or rehumanized */
			else
				return (MM_HIT|MM_DEF_DIED);
		}
	}
	/* uhitm */
	else if (youagr) {
		/* abuse pets */
		if (mdef->mtame && (!mdef->mflee || mdef->mfleetim) && dmg > 0) {
			abuse_dog(mdef);
			monflee(mdef, 10 * rnd(dmg), FALSE, FALSE);
		}

		/* set uhurtm */
		if (dmg > 0) {
			mdef->uhurtm = TRUE;
		}

		if (*hp(mdef) < 1) {
			int nocorpse = (attk && (attk->adtyp == AD_DGST || attk->adtyp == AD_DISN)) ? 0x2 : 0;
			/* killed a pet by accident */
			if (mdef->mtame && !cansee(mdef->mx, mdef->my)) {
				You_feel("embarrassed for a moment.");
				if (dmg) xkilled(mdef, 0); /* !dmg but hp<1: already killed */
			}
			/* killed monster */
			else {
				/* non-verbose */
				if (!flags.verbose) {
					You("destroy it!");
					if (dmg) xkilled(mdef, 0|nocorpse);
				}
				/* verbose */
				else {
					if (dmg) xkilled(mdef, 1|nocorpse);
				}
			}
			if (*hp(mdef) > 0)
				return (MM_HIT|MM_DEF_LSVD); /* mdef lifesaved */
			else
				return (MM_HIT|MM_DEF_DIED);
		}
	}
	/* mhitm */
	else {
		if (*hp(mdef)< 1) {
			monkilled(mdef, "", attk ? (int)attk->adtyp : 0);
			if (*hp(mdef) > 0)
				return (MM_HIT|MM_DEF_LSVD); /* mdef lifesaved */
			else
				return (MM_HIT | MM_DEF_DIED | (!magr || grow_up(magr, mdef) ? 0 : MM_AGR_DIED));
		}
	}
	return MM_HIT;
}

/* xstoney()
 * 
 * Attempts to turn [mdef] to stone.
 * 
 * Returns:
 * MM_MISS		0x00	(possible) aggressor missed (caused by lifesaved mondef)
 * MM_HIT		0x01	(possible) aggressor hit defender (defender poly'd when stoned OR youdef)
 * MM_DEF_DIED	0x02	(possible) defender died (mondef turned to stone OR died while unstoning)
 * MM_AGR_DIED	0x04	(possible) aggressor died (monagr got XP for the kill and leveled up into something genocided)
 * MM_AGR_STOP	0x08	(never) aggressor should stop attacking for some reason
 * 
 */
int
xstoney(magr, mdef)
struct monst * magr;
struct monst * mdef;
{

	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct permonst * pa = !magr ? (struct permonst *)0 : youagr ? youracedata : magr->data;
	struct permonst * pd = youdef ? youracedata : mdef->data;

	if (!Stone_res(mdef)
		&& !(youdef && Stoned))
	{
		/* wake the defender */
		wakeup2(mdef, youagr);

		/* vs player */
		if (youdef) {
			if (!(poly_when_stoned(youracedata) && polymon(PM_STONE_GOLEM))) {
				Stoned = 5;
				if (magr) {
					delayed_killer = pa->mname;
					if (pa->geno & G_UNIQ) {
						if (!type_is_pname(pa)) {
							static char kbuf[BUFSZ];
							/* "the" buffer may be reallocated */
							Strcpy(kbuf, the(delayed_killer));
							delayed_killer = kbuf;
						}
						killer_format = KILLED_BY;
					}
					else
						killer_format = KILLED_BY_AN;
				}
				else {
					delayed_killer = "object";
					killer_format = KILLED_BY_AN;
				}
			}
		}
		/* vs monsters */
		else {
			/* try to unstone itself */
			if (!munstone(mdef, youagr)) {
				minstapetrify(mdef, youagr);
			}
			/* some monsters poly when stoned */
			if (Stone_res(mdef)) {
				/* don't return MM_DEF_LSVD -- lifesaving didn't happen */
				return MM_HIT;
			}
			/* survived */
			if (*hp(mdef) > 0)
				return (MM_HIT|MM_DEF_LSVD);
			/* died */
			else {
				if (mdef->mtame && !youagr)
					You("have a peculiarly sad feeling for a moment, then it passes.");
				return (MM_HIT | (MM_DEF_DIED | (youagr || !magr || grow_up(magr, mdef) ? 0 : MM_AGR_DIED)));
			}
		}
	}
	return MM_HIT;
}

/* do_weapon_multistriking_effects()
 * 
 * Performs specific artifact-level special effects caused by multistriking weapons
 * 
 * This is performed ONCE per attack action, and only if [weapon]->ostriking != 0
 * This is in contrast to artifacthit, which is applied additionally for each [weapon]->ostriking
 *
 * Assumes defender is alive
 */
int
do_weapon_multistriking_effects(magr, mdef, attk, weapon, vis)
struct monst * magr;
struct monst * mdef;
struct attack * attk;
struct obj * weapon;
int vis;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct permonst * pa = youagr ? youracedata : magr->data;
	struct permonst * pd = youdef ? youracedata : mdef->data;

	int result = 0;

	if (arti_threeHead(weapon)) {
		/* only has special effects when all 3 heads hit */
		if (weapon->ostriking + 1 == 3) {
			/* message */
			if (youdef) {
				You("stagger!");
			}
			else if (canseemon(mdef)) {
				pline("%s %s!", 
					Monnam(mdef),
					makeplural(stagger(mdef, "stagger"))
					);
			}
			/* apply stun */
			if (youdef) {
				make_stunned((HStun)+d(3, 3), FALSE);
			}
			else {
				mdef->mconf = 1;
				mdef->mstun = 1;
			}
		}
	}

	if (arti_tentRod(weapon)) {
		if (weapon->ostriking + 1 >= 3)
		{
			switch (rn2(3))
			{
			case 0:/* Blind */
				if (youdef) {
					if(!Blind)
						You("go blind!");
					make_blinded(Blinded + d(3, 3), FALSE);
				}
				else {
					if (canseemon(mdef) && !is_blind(mdef))
						pline("%s goes blind!", Monnam(mdef));
					mdef->mcansee = 0;
					mdef->mblinded = d(3, 3);
				}
				break;
			case 1:/* Stun */
				if (youdef) {
					You("stagger!");
					make_stunned((HStun)+d(3, 3), FALSE);
				}
				else {
					if (canseemon(mdef))
						pline("%s is stunned!", Monnam(mdef));
					mdef->mstun = 1;
				}
				break;
			case 2:/* Conf */
				if (youdef) {
					You("are confused!");
					make_confused(HConfusion + d(3, 3), FALSE);
				}
				else {
					if (canseemon(mdef))
						pline("%s %s!", Monnam(mdef), makeplural(stagger(mdef, "stagger")));
					mdef->mconf = 1;
				}
				break;
			}
		}
		/* not elseif, this is additive */
		if (weapon->ostriking + 1 >= 6)
		{
			switch (rn2(3))
			{
			case 0:/* Slow */
				if (youdef) {
					u_slow_down();
				}
				else {
					if (canseemon(mdef))
						pline("%s slows down!", Monnam(mdef));
					mdef->mspeed = MSLOW;
					mdef->permspeed = MSLOW;
				}
				break;
			case 1:/* Paralyze */
				if (youdef) {
					You("can't move!");
					nomul(-1 * d(3, 3), "paralyzed by the Tentacle Rod.");
				}
				else {
					if (canseemon(mdef))
						pline("%s is frozen!", Monnam(mdef));
					mdef->mcanmove = 0;
					mdef->mfrozen = d(1, 6);
				}
				break;
			case 2:/* Craze */
				if (youdef) {
					You("go insane!");
					make_confused(100, FALSE);
					change_usanity(-1*d(10,6), TRUE);
				}
				else if(!mindless_mon(mdef)){
					if (canseemon(mdef))
						pline("%s goes insane!", Monnam(mdef));
					mdef->mcrazed = 1;
					mdef->mberserk = 1;
					(void) set_apparxy(mdef);
					if(!rn2(4)){
						mdef->mconf = 1;
						(void) set_apparxy(mdef);
					}
					if(!rn2(10)){
						mdef->mnotlaugh=0;
						mdef->mlaughing=rnd(5);
					}
				}
				break;
			}
		}
		/* not elseif */
		if (weapon->ostriking + 1 == 7)
		{
			/* message */
			if (vis&VIS_MAGR) {
				pline_The("%s hits %s with a storm of energy!",
					xname(weapon),
					mon_nam(mdef));
			}
			/* 1d7 bonus damage per element, checking that mdef hasn't died partway through */
			if (!(result&(MM_DEF_DIED|MM_AGR_DIED|MM_DEF_LSVD)) && !Fire_res(mdef))		result |= xdamagey(magr, mdef, attk, d(1, 7));
			if (!(result&(MM_DEF_DIED|MM_AGR_DIED|MM_DEF_LSVD)) && !Cold_res(mdef))		result |= xdamagey(magr, mdef, attk, d(1, 7));
			if (!(result&(MM_DEF_DIED|MM_AGR_DIED|MM_DEF_LSVD)) && !Shock_res(mdef))	result |= xdamagey(magr, mdef, attk, d(1, 7));
			if (!(result&(MM_DEF_DIED|MM_AGR_DIED|MM_DEF_LSVD)) && !Acid_res(mdef))		result |= xdamagey(magr, mdef, attk, d(1, 7));
			if (!(result&(MM_DEF_DIED|MM_AGR_DIED|MM_DEF_LSVD)) && !Magic_res(mdef))	result |= xdamagey(magr, mdef, attk, d(1, 7));
			if (!(result&(MM_DEF_DIED|MM_AGR_DIED|MM_DEF_LSVD)) && !Poison_res(mdef))	result |= xdamagey(magr, mdef, attk, d(1, 7));
			if (!(result&(MM_DEF_DIED|MM_AGR_DIED|MM_DEF_LSVD)) && !Drain_res(mdef))	result |= xdamagey(magr, mdef, attk, d(1, 7));
			if (result&(MM_DEF_DIED|MM_AGR_DIED|MM_DEF_LSVD)) {
				return result;
			}
		}
	}
	/* default return: no one died */
	return MM_HIT;
}

/*
 * tohitval()
 *
 * Very general to-hit-bonus finding function.
 * 
 * [magr] might not exist!
 *
 */
int
tohitval(magr, mdef, attk, weapon, vpointer, hmoncode, flat_acc, shield_margin)
struct monst * magr;
struct monst * mdef;
struct attack * attk;
struct obj * weapon;
void * vpointer;				/* additional /whatever/, type based on hmoncode. */
int hmoncode;					/* what kind of pointer is vpointer, and what is it doing? (hack.h) */
int flat_acc;
int *shield_margin;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct permonst * pa = (magr ? (youagr ? youracedata : magr->data) : (struct permonst *)0);
	struct permonst * pd = youdef ? youracedata : mdef->data;
	struct obj * otmp;

	boolean melee = (hmoncode & HMON_WHACK);
	boolean thrust = (hmoncode & HMON_THRUST);
	boolean fired = (hmoncode & HMON_FIRED);
	boolean thrown = (hmoncode & HMON_PROJECTILE);
	boolean trapped = (hmoncode & HMON_TRAP);
	boolean misthrown = (thrown && weapon && is_ammo(weapon) && weapon->oclass != GEM_CLASS && !fired);

	struct obj * launcher = (struct obj *)(fired ? vpointer : 0);
	struct trap * trap = (struct trap *)(trapped ? vpointer : 0);
	if (trap) launcher = 0; /* trap takes precedence over launcher */

	/* partial accuracy counters */
	int base_acc = 0;	/* accuracy from leveling up */
	int bons_acc = 0;	/* attacker's misc accuracy bonuses */
	int rang_acc = 0;	/* ranged-attack specific accuracy bonuses */
	int stdy_acc = 0;	/* defender's study amount */
	int vdef_acc = 0;	/* accuracy bonuses versus that specific defender */
	int wepn_acc = 0;	/* accuracy of the weapon -- dependent on attk */
	int defn_acc = 0;	/* defender's defense -- dependent on attk */
	int totl_acc = 0;	/* sum of above partial accuracy counters */

	/* base accuracy due to level -- requires attacker */
	if (magr) {
		/* Some player roles have better ranged accuracy with certain weapons than their BAB */
		if (youagr && thrown && weapon &&
			(Role_if(PM_RANGER) ||
			(u.sealsActive&SEAL_EVE) ||
			(weapon->otyp == DAGGER && Role_if(PM_ROGUE)) ||
			(weapon->otyp == DART && Role_if(PM_TOURIST)) ||
			(weapon->otyp == BALL && Role_if(PM_CONVICT))
			)) {
			base_acc = mlev(magr);
		}
		else {
			base_acc = mlev(magr) * (youagr ? BASE_ATTACK_BONUS(weapon) : thrown ? 0.67 : MON_BAB(magr));
		}
		if(!thrown && weapon && weapon->o_e_trait&ETRAIT_FOCUS_FIRE && CHECK_ETRAIT(weapon, magr, ETRAIT_FOCUS_FIRE)){
			//-2 to hit if expert, -5 if skilled
			base_acc -= ROLL_ETRAIT(weapon, magr, 2, 5);
		}

		if(youagr){
			static long warnpanic = 0;
			if(Panicking){
				if(warnpanic != moves){
					pline("You swing wildly in your panic!");
				}
				warnpanic = moves;
				base_acc = -5;
			} else warnpanic = 0L;

			if(weapon && weapon->otyp == LONG_SWORD && activeFightingForm(FFORM_HALF_SWORD)){
				base_acc += 2; //Dagger bonus
			}
		}
	}
	/* other attacker-related accuracy bonuses */
	if (magr) {
		/* Small monsters are more accurate */
		switch(pa->msize){
			case MZ_TINY:
				bons_acc += 5;
			break;
			case MZ_SMALL:
				bons_acc += 2;
			break;
		}
		/* player-only accuracy bonuses */
		if (youagr) {
			/* base +1/-1 for no reason */
			bons_acc += (thrown ? -1 : +1);
			/* Stat (DEX, STR) */
			if (!thrown || !launcher || objects[launcher->otyp].oc_skill == P_SLING){
				/* firing ranged attacks without a laucher (ex manticore tail spikes) can use STR */
				/* hack: if wearing kicking boots, you effectively have 25 STR for kicked objects */
				if (youagr && hmoncode & HMON_KICKED && uarmf && (uarmf->otyp == KICKING_BOOTS || (uarmf->otyp == IMPERIAL_ELVEN_BOOTS && check_imp_mod(uarmf, IEA_KICKING))))
					override_str = STR19(25);	/* 25 STR */
				bons_acc += abon();
				override_str = 0;
			}
			else {
				if (ACURR(A_DEX) < 4)			bons_acc -= 3;
				else if (ACURR(A_DEX) < 6)		bons_acc -= 2;
				else if (ACURR(A_DEX) < 8)		bons_acc -= 1;
				else if (ACURR(A_DEX) >= 14)	bons_acc += (ACURR(A_DEX) - 14);
			}
			if((active_glyph(SIGHT) || youracedata->mtyp == PM_GROTESQUE_PEEPER) && canseemon(mdef) && !mindless_mon(mdef))
				bons_acc += rnd(20);
			if(active_glyph(SIGHT))
				bons_acc += base_acc*.3;

			if(Role_if(PM_ANACHRONONAUT) && !quest_status.leader_is_dead && Is_qhome(&u.uz)){
				bons_acc += min(u.ulevel, 20);
			}

			/* Stat (INT) (from Dantalion vs telepathically sensed enemies) */
			if (u.sealsActive&SEAL_DANTALION && tp_sensemon(mdef))
				bons_acc += max(0, (ACURR(A_INT) - 10) / 2);

			if(attk && is_bite_at(attk->aatyp)
				&& mdef && (!vegetarian(mdef->data) || your_race(mdef->data))
				&& mad_turn(MAD_CANNIBALISM)
			){
				int can_bon = str_abon();

				if (ACURR(A_CHA) == 25) can_bon += 8;
				else can_bon += max(0, (ACURR(A_CHA) - 10) / 2);

				if(your_race(mdef->data))
					can_bon *= 2;

				bons_acc += can_bon;
			}
			if(attk && is_insight_tentacle_at(attk->aatyp)
				&& (u.sealsActive&SEAL_OSE)
			){
				if(Insight)
					bons_acc += rnd(min(Insight, mlev(magr)));
				if (ACURR(A_CHA) == 25) bons_acc += 8;
				else bons_acc += max(0, (ACURR(A_CHA) - 10) / 2);
			}
			/* intrinsic accuracy bonuses */
			bons_acc += u.uhitinc;
			if(uarmg && uarmg->otyp == IMPERIAL_ELVEN_HELM && check_imp_mod(uarmg, IEA_INC_ACC))
				bons_acc += uarmg->spe;
			/* Malphas' bonus accuracy from having nearby crows -- btw Chris this is horribly named */
			bons_acc += u.spiritAttk;
			/* Uur (active) */
			if (u.uuur_duration)
				bons_acc += 10;
			/* luck */
			if (Luck)
				bons_acc += sgn(Luck)*rnd(abs(Luck));
			/* Bard */
			bons_acc += u.uencouraged;
			/* Singing Sword */
			if (uwep && uwep->oartifact == ART_SINGING_SWORD){
				if (uwep->osinging == OSING_LIFE) {
					if (youagr)
						bons_acc += uwep->spe + 1;
				}
				if (uwep->osinging == OSING_COURAGE){
					if (!youagr && magr->mtame && !mindless_mon(magr) && !is_deaf(magr))
						bons_acc += uwep->spe + 1;
				}
			}
			/* carrying too much */
			if (near_capacity())
				bons_acc -= ((near_capacity() * 2) - 1);
			/* trapped */
			if (u.utrap)
				bons_acc -= 3;
			/* swallowed */
			if (u.uswallow && u.ustuck == mdef)
				bons_acc += 1000;
		}
		/* monster-only accuracy bonuses */
		else {
			/* martial-trained foes are accurate */
			if(magr->mformication || magr->mscorpions || magr->mcaterpillars)
				bons_acc -= 2;
			else switch(m_martial_skill(pa)) {
			case P_UNSKILLED: bons_acc += 0; break;
			case P_BASIC:     bons_acc += 1; break;
			case P_SKILLED:   bons_acc += 2; break;
			case P_EXPERT:    bons_acc += 5; break;
			}
			/* predators have keen instincts */
			if(carnivorous(pa)&&!herbivorous(pa))
				bons_acc += 2;
			/* these guys are extra accurate */
			if (is_uvuudaum(pa) || pa->mtyp == PM_CLAIRVOYANT_CHANGED || pa->mtyp == PM_NORN)
				bons_acc += 20;
			if (pa->mtyp == PM_DRACAE_ELADRIN)
				bons_acc += rnd(12);
			if (pa->mtyp == PM_GROTESQUE_PEEPER){
				if((youdef && mon_can_see_you(magr))
				 ||(!youdef && mon_can_see_mon(magr,mdef) && !mindless_mon(mdef))
				)
				bons_acc += rnd(20);
			}
			if(pa->mtyp == PM_INDEX_WOLF){
				bons_acc -= 2; //Bad depth perception
			}
			if (pa->mtyp == PM_DANCING_BLADE)
				bons_acc += 7;
			if (pa->mtyp == PM_CHOKHMAH_SEPHIRAH)
				bons_acc += u.chokhmah;

			if(Role_if(PM_ANACHRONONAUT) && !quest_status.leader_is_dead && Is_qhome(&u.uz) && magr->mfaction == QUEST_FACTION){
				if(magr->mtyp != PM_SARA__THE_LAST_ORACLE){
					bons_acc += min(magr->m_lev, 20);
				}
			}

			/* simulate accuracy from stat bonuses from gloves */
			if ((otmp = which_armor(magr, W_ARMG))) {
				if (otmp->oartifact == ART_GODHANDS)
					bons_acc += 11;
				else if (otmp->otyp == GAUNTLETS_OF_DEXTERITY || otmp->oartifact == ART_PREMIUM_HEART)
					bons_acc += otmp->spe;
				if (otmp->otyp == GAUNTLETS_OF_POWER || (otmp->otyp == IMPERIAL_ELVEN_GAUNTLETS && check_imp_mod(otmp, IEA_GOPOWER)))
					bons_acc += 3;
			}
			if ((otmp = which_armor(magr, W_ARMH))) {
				if(otmp && otmp->otyp == IMPERIAL_ELVEN_HELM && check_imp_mod(otmp, IEA_INC_ACC))
					bons_acc += otmp->spe;
			}
			if(magr->msciaphilia && unshadowed_square(magr->mx, magr->my)){
				bons_acc -= min(20, magr->m_lev);
			}
#ifdef STEED
			/* Your steed gets a skill-based boost */
			if (magr == u.usteed)
				bons_acc += mountedCombat();
#endif
			/* All of your pets get a skill-based boost */
			if (magr->mtame){
				bons_acc += beastmastery();
				if (uarm && uarm->oartifact == ART_BEASTMASTER_S_DUSTER && is_animal(magr->data))
					bons_acc += beastmastery(); // double for the beastmaster's duster
				if(artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_STEEL)
					bons_acc += 1;
				if(is_vampire(magr->data) && check_vampire(VAMPIRE_MASTERY))
					bons_acc += 5;
			}
			/* Bard */
			if(!(youdef && Nightmare && u.umadness&MAD_RAGE))
				bons_acc += magr->encouraged;
			if(magr->mtyp == PM_LUCKSUCKER)
				bons_acc += 13+magr->mvar_lucksucker;
			/* Singing Sword */
			if (uwep && uwep->oartifact == ART_SINGING_SWORD && !mindless_mon(magr) && !is_deaf(magr)){
				if (uwep->osinging == OSING_DIRGE && !magr->mtame){
					bons_acc -= uwep->spe + 1;
				}
			}
			/* Priests of Asmodeus */
			if(flags.spriest_level && is_demon(magr->data) && is_lawful_mon(magr) && !magr->mpeaceful)
				bons_acc += 9;
			/* trapped */
			if (magr->mtrapped)
				bons_acc -= 2;
		}
	}
	/* traps have accuracy, too */
	else if (trap) {
		switch (trap->ttyp)
		{
		case ROCKTRAP:
			bons_acc += 1000;	/* guarantee hit */
			break;
		case ARROW_TRAP:
		case DART_TRAP:
			bons_acc += level_difficulty()/4 + 4;
			break;
		case ROLLING_BOULDER_TRAP:
			bons_acc += level_difficulty()/2;
			break;
		}
	}

	/* ranged-specific accuracy modifiers */
	if (thrown)
	{
		if (magr) {
			/* accuracy is reduced by range (or increased, for sniper rifles) */
			int dist_penalty = max(-4, distmin(x(magr), y(magr), x(mdef), y(mdef)) - 3);
			if (launcher && launcher->otyp == SNIPER_RIFLE)
				rang_acc += dist_penalty;
			else
				rang_acc -= dist_penalty;

			/* elves are especially accurate with bows, as are samurai */
			if ((launcher && objects[launcher->otyp].oc_skill == P_BOW) && 
				(youagr ?
					((Race_if(PM_ELF) || Role_if(PM_SAMURAI)) && (!Upolyd || your_race(youmonst.data))) :
					(is_elf(pa) || pa->mtyp == PM_SAMURAI))
				)
			{
				rang_acc++;
				/* even more so with their special bow */
				if (((youagr ? (Race_if(PM_ELF)) : is_elf(pa)) && launcher->otyp == ELVEN_BOW) ||
					((youagr ? Role_if(PM_SAMURAI) : pa->mtyp == PM_SAMURAI) && launcher->otyp == YUMI))
					rang_acc++;
			}
		}

		/* gloves are a hinderance to proper use of bows */
		if (magr && launcher && objects[launcher->otyp].oc_skill == P_BOW) {
			struct obj * gloves;
			gloves = (youagr ? uarmg : which_armor(magr, W_ARMG));
			if (gloves){
				switch (gloves->otyp) {
				case HARMONIUM_GAUNTLETS:
				case ORIHALCYON_GAUNTLETS:    /* metal */
				case GAUNTLETS_OF_POWER:    /* metal */
				case GAUNTLETS:
				case CRYSTAL_GAUNTLETS:
					rang_acc -= 2;
					break;
				case GAUNTLETS_OF_FUMBLING:
					rang_acc -= 3;
					break;
				case ARCHAIC_GAUNTLETS:
				case PLASTEEL_GAUNTLETS:
					rang_acc -= 1;
					break;
				case LONG_GLOVES:
				case GLOVES:
				case GAUNTLETS_OF_DEXTERITY:
				case HIGH_ELVEN_GAUNTLETS:
				case IMPERIAL_ELVEN_GAUNTLETS:
					break;
				default:
					impossible("Unknown type of gloves (%d)", gloves->otyp);
					break;
				}
			}
		}
		/* some objects are more likely to hit than others */
		switch (weapon->otyp) {
		case BALL:
			if (weapon != uball)
				rang_acc += 2;
			break;
		case BOULDER:
		case STATUE:
			if (is_boulder(weapon))
				rang_acc += 6;
			break;
		case MASS_OF_STUFF:
			rang_acc += 9;
		break;
		case BALL_OF_WEBBING:
			/* balls of webbing should always miss */
			rang_acc -= 2000;
			break;
		}
	}

	/* study */
	if (youdef)
		stdy_acc += u.ustdy;
	if (!youdef)
		stdy_acc += mdef->mstdy;
	if (youagr)
		stdy_acc += mdef->ustdym;

	/* Adjust vs defender state; do not modify */
	if (youdef ? Stunned : mdef->mstun)
		vdef_acc += 2;
	if (youdef ? u.usleep : mdef->msleeping)
		vdef_acc += 2;
	if (youdef ||
		!(pd->mtyp == PM_GIANT_TURTLE && mdef->mflee && !mdef->mcanmove)) {	// don't penalize enshelled turtles
		if (youdef ? FALSE : mdef->mflee)
			vdef_acc += 2;
		if (youdef ? multi < 0 : !mdef->mcanmove)
			vdef_acc += 4;
	}
	/* Elves hate orcs, and the devs like elves. */
	if (magr && is_orc(pd) && is_elf(pa))
		vdef_acc += 1;
	/* ranged attacks are affected by target size */
	if (thrown)
	{
		vdef_acc += (pd->msize - MZ_MEDIUM);
	}
	/* Smaug is vulnerable to stabbings */
	if (pd->mtyp == PM_SMAUG && fired && weapon && launcher &&
		is_stabbing(weapon) && is_ammo(weapon))
	{
		vdef_acc += 20;
	}
	/* Shades (and other insubstantial creatures) edge-case handler */
	/* generally, miss_via_insubstantial() should do this, but some edge cases (like rolling boulder traps) are missed */
	if (insubstantial(pd) && !hits_insubstantial(magr, mdef, attk, weapon))
		vdef_acc -= 2000;

	/* weapon accuracy -- only applies for a weapon attack OR a properly-thrown object */
	if ((attk && weapon_aatyp(attk->aatyp))
		|| fired)
	{
		if (weapon) {
			/* specific bonuses of weapon vs mdef */
			wepn_acc += hitval(weapon, mdef, magr);
			/* Houchou always hits when thrown */
			if (weapon->oartifact == ART_HOUCHOU && thrown)
				wepn_acc += 1000;

			/* -4 accuracy per weapon size too large (not for thrown objects) */
			if (!thrown){
				int size_penalty = weapon->objsize - pa->msize;
				if (youagr){
					if (Role_if(PM_CAVEMAN))
						size_penalty = max(0, size_penalty-1);
					if (u.sealsActive&SEAL_YMIR)
						size_penalty = max(0, size_penalty-1);
					if (is_cclub_able(weapon))
						size_penalty = max(0, size_penalty-1);
				}
				
				wepn_acc += -4 * max(0, size_penalty);
			}
			/* fencing gloves increase weapon accuracy when you have a free off-hand */
			if (!thrown && !bimanual(weapon, magr->data) && !which_armor(magr, W_ARMS)) {
				struct obj * otmp = which_armor(magr, W_ARMG);
				if (otmp && otmp->otyp == find_fgloves())
					wepn_acc += 2;
			}
			
			/* ranged attacks also get their launcher's accuracy */
			if (fired && launcher) {
				/* enchantment, erosion */
				wepn_acc += launcher->spe;
				wepn_acc -= greatest_erosion(launcher);
				/* artifact bonus */
				if (launcher->oartifact){
					wepn_acc += spec_abon(launcher, mdef, youagr);
				}

				if (is_insight_weapon(launcher) && (youagr ? (Role_if(PM_MADMAN) || u.sealsActive&SEAL_OSE || mvitals[PM_MOON_S_CHOSEN].died) : insightful(magr->data))){
					if(youagr){
						if(Insight)
							wepn_acc += rnd(min(Insight, mlev(magr)));
					}
					else {
						wepn_acc += rnd(mlev(magr));
					}
				}
			}
			/* mis-used ammo */
			else if (misthrown) {
				wepn_acc -= 4;
			}
			/* other thrown things */
			else if (fired)
			{
				if (is_boomerang(weapon))			/* arbitrary */
					wepn_acc += 4;
				else if (throwing_weapon(weapon))	/* meant to be thrown */
					wepn_acc += 2;
				else								/* not meant to be thrown at all */
					wepn_acc -= 2;
			}

			/* Shii Cho lightsaber form is not meant for fighting other lightsaber users */
			if (youagr && is_lightsaber(weapon) && litsaber(weapon)){
				if (activeFightingForm(FFORM_SHII_CHO) && MON_WEP(mdef) && is_lightsaber(MON_WEP(mdef)) && litsaber(MON_WEP(mdef))){
					wepn_acc -= 5;
				}
			}
		}
		/* gnomes are especially accurate, with weapons or ranged or any other "proper" attacks... not a zombie's claw attack. */
		if (magr && (youagr ? Race_if(PM_GNOME) : is_gnome(pa)))
			wepn_acc += 2;

		/* skill bonus (player-only; applies without a weapon as well) */
		if (youagr) {
			int wtype;
			/* get simple weapon skill associated with the weapon, not including twoweapon */
			wtype = weapon_skill_type(weapon, launcher, fired);

			if (fired && launcher)
				wepn_acc += weapon_hit_bonus(launcher, wtype);
			else if (!misthrown)
				wepn_acc += weapon_hit_bonus(weapon, wtype);
		}
		/* monk accuracy bonus/penalty (player-only) (melee) */
		if (youagr && melee && Role_if(PM_MONK) && !Upolyd) {
			static boolean armmessage = TRUE;
			static boolean urmmessage = TRUE;
			boolean resunderwear = uarmu && uarmu->otyp == VICTORIAN_UNDERWEAR;
			boolean resarmor = uarm && uarm->otyp != WAISTCLOTH && uarm->otyp != HAWAIIAN_SHORTS;
			if (resarmor || resunderwear) {
				if(resarmor && resunderwear && armmessage && urmmessage){
				if (armmessage) Your("armor and underwear are rather cumbersome...");
					armmessage = FALSE;
					urmmessage = FALSE;
				}
				if (resarmor && armmessage){
					Your("armor is rather cumbersome...");
					armmessage = FALSE;
				}
				else if (resunderwear && urmmessage){
					Your("underwear is rather restrictive...");
					urmmessage = FALSE;
				}
				wepn_acc -= 20; /*flat -20 for monks in armor*/
			}
			else {
				if (!armmessage) armmessage = TRUE;
				if (!urmmessage) urmmessage = TRUE;
				if (!uwep && !uarms) {
					wepn_acc += (u.ulevel / 3) + 2;
				}
			}
		}
		/* Some madnesses give accuracy bonus/penalty (player-only) (melee) */
		if(youagr && melee && u.umadness){
			/* nudist accuracy bonus/penalty */
			if (u.umadness&MAD_NUDIST && !BlockableClearThoughts && NightmareAware_Sanity < 100){
				int delta = NightmareAware_Insanity;
				int discomfort = u_clothing_discomfort();
				static boolean clothmessage = TRUE;
				if (discomfort) {
					if (clothmessage) Your("clothing is rather uncomfortable...");
					clothmessage = FALSE;
					wepn_acc -= (discomfort * delta) / 10;
				}
				else {
					if (!clothmessage) clothmessage = TRUE;
					if (!uwep && !uarms) {
						wepn_acc += delta / 10;
					}
				}
			}
			/* Sciaphilia accuracy bonus/penalty */
			if (u.umadness&MAD_SCIAPHILIA && !BlockableClearThoughts && NightmareAware_Sanity < 96){
				int delta = NightmareAware_Insanity;
				boolean discomfort = unshadowed_square(u.ux, u.uy);
				static boolean shadowmessage = TRUE;
				if (discomfort) {
					if (shadowmessage) You("long for the flickering shadows!");
					shadowmessage = FALSE;
					wepn_acc -= delta / 5;
				}
				else if (!shadowmessage) shadowmessage = TRUE;
			}
		}
	}

	/* combat boots increase accuracy */
	if (magr) {
		otmp = (youagr ? uarmf : which_armor(magr, W_ARMF));
		if (otmp && otmp->otyp == find_cboots())
			wepn_acc++;
	}
	

	if(youdef && youagr){
		bons_acc += 2000; //Auto hit.
	}


	/* find defender's AC */
	/* ignore worn armor? */
	if ((youagr && u.sealsActive&SEAL_CHUPOCLOPS && (melee || thrust)) ||
		(!youagr && magr && mad_monster_turn(magr, MAD_NON_EUCLID)) ||
		(weapon && (arti_phasing(weapon) || (is_lightsaber(weapon) && litsaber(weapon)))) ||
		(melee && youagr && weapon && weapon->otyp == LONG_SWORD && activeFightingForm(FFORM_HALF_SWORD)) ||
		(melee && attk->aatyp == AT_TUCH) ||
		(melee && attk->aatyp == AT_VINE) ||
		(melee && attk->aatyp == AT_VOMT) ||
		(melee && spirit_rapier_at(attk->aatyp)) ||
		(weapon && !valid_weapon(weapon))	/* potions, cream pies, rubber chickens, eggs, etc. */
	) {
		if (youdef) {
			defn_acc += AC_VALUE(base_uac() + u.uspellprot) - u.uspellprot;
		}
		else {
			defn_acc += base_mac(mdef);
		}
		if(shield_margin) *shield_margin = -1;
		if(melee && attk->aatyp == AT_VOMT){
			defn_acc += 5;
		}
	}
	/* do not ignore worn armor */
	else {
		if (youdef){
			defn_acc += AC_VALUE(u.uac + u.uspellprot) - u.uspellprot;
			if(shield_margin) {
				if(uarms){
					*shield_margin = max(0, arm_ac_bonus(uarms) + (uarms->objsize - youracedata->msize)) + shield_skill(uarms);
					if(uwep && (objects[uwep->otyp].oc_skill == P_SPEAR || objects[uwep->otyp].oc_skill == P_POLEARMS
						|| objects[uwep->otyp].oc_skill == P_TRIDENT || objects[uwep->otyp].oc_skill == P_LANCE)
					)
						*shield_margin += 2;
				}
				else if(uarm && uarm->oartifact == ART_SCORPION_CARAPACE && check_carapace_mod(uarm, CPROP_SHIELDS)){
					*shield_margin = 1 + shield_skill(uarm);
					if(uwep && (objects[uwep->otyp].oc_skill == P_SPEAR || objects[uwep->otyp].oc_skill == P_POLEARMS
						|| objects[uwep->otyp].oc_skill == P_TRIDENT || objects[uwep->otyp].oc_skill == P_LANCE)
					)
						*shield_margin += 3;//+1 bonus to shield size
				}
				else *shield_margin = -1;
			}
		}
		else {
			defn_acc += find_mac(mdef);
			if(shield_margin) {
				if(which_armor(mdef, W_ARMS)) *shield_margin = arm_ac_bonus(which_armor(mdef, W_ARMS));
				else *shield_margin = -1;
			}
		}
	}

	/* determine if the attack hits */
	totl_acc = base_acc
		+ rang_acc
		+ bons_acc
		+ stdy_acc
		+ vdef_acc
		+ wepn_acc
		+ defn_acc
		+ flat_acc;

	if (wizard && (iflags.wizcombatdebug & WIZCOMBATDEBUG_ACCURACY) && WIZCOMBATDEBUG_APPLIES(magr, mdef)) {
		pline("Accuracy = %d+%d+%d+%d+%d+%d+%d+%d=%d",
			base_acc,
			rang_acc,
			bons_acc,
			stdy_acc,
			vdef_acc,
			wepn_acc,
			defn_acc,
			flat_acc,
			totl_acc
			);
	}

	/* return our to-hit -- if this is greater than a d20, it hits */
	return totl_acc;
}

/* xmeleehity()
 * 
 * Called when a creature attempts to attack another creature with a specific melee attack.
 * 
 * Assumes both the attacker and defender are alive and existant when called
 * 
 * Inside the function itself, determines if the attacks hits
 * Performs counterattacks as well
 * 
 * Hit / miss messages are handled here and below
 *
 * Uses MM_ return values
 */
int
xmeleehity(magr, mdef, attk, weapon_p, vis, flat_acc, ranged)
struct monst * magr;
struct monst * mdef;
struct attack * attk;
struct obj ** weapon_p;
int vis;
int flat_acc;
boolean ranged;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct obj * weapon = weapon_p ? *(weapon_p) : (struct obj *)0;
	struct attack alt_attk = *attk;
	struct permonst * pa = youagr ? youracedata : magr->data;
	struct permonst * pd = youdef ? youracedata : mdef->data;
	struct obj * otmp;
	char buf[BUFSZ];
	int result;			/* did attack hit, miss, defender live, die, agressor die, stop? */

	int dieroll;				/* rolled accuracy */
	int accuracy;				/* accuracy of attack; if this is less than dieroll, the attack hits */
	int shield_margin = -1;		/* contribution of shield to AC (-1 means "none") */
	boolean hit = FALSE;		/* whether or not the attack hit */
	boolean miss = FALSE;		/* counterpart to hit */
	boolean domissmsg = TRUE;	/* FALSE if a message has already been printed about a miss */

	/* if it is the player's pet attacking and it is in LoS or the PC can sense its mind, set flag to train Beast Mastery skill */
	if (!youagr && magr->mtame && (canseemon(magr) || tp_sensemon(magr) || beastMateryRadius(magr))) {
		u.petattacked = TRUE;
	}

	if (vis == -1)
		vis = getvis(magr, mdef, 0, 0);
	
	/* Some things cause immediate misses */
	if (miss_via_insubstantial(magr, mdef, attk, weapon, vis)) {
		miss = TRUE;
		domissmsg = FALSE;
	}
	/* mindless monsters and soul blades */
	if (!miss && spirit_rapier_at(attk->aatyp) && attk->adtyp == AD_PSON && ((!youdef && mindless_mon(mdef)) || Catapsi)) {
		/* Print message */
		if (vis&VIS_MAGR) {
			Sprintf(buf, "%s", ((!weapon || valid_weapon(weapon)) ? "attack" : cxname(weapon)));
			pline("%s %s %s harmlessly through %s.",
				(youagr ? "Your" : s_suffix(Monnam(magr))),
				buf,
				vtense(buf, "pass"),
				(youdef ? "you" : mon_nam(mdef))
				);
		}
		domissmsg = FALSE;
		miss = TRUE;
	}
	/* Otiax protects you from being hit (1/5) */
	if (youdef && u.sealsActive&SEAL_OTIAX && !rn2(5))
	{
		/* No special message, they just miss? */
		miss = TRUE;
	}

	/* get accuracy of attack */
	if (miss){
		accuracy = 0;
		shield_margin = -1;
	}
	else
		accuracy = tohitval(magr, mdef, attk, weapon, (void *)0, (ranged ? HMON_THRUST : HMON_WHACK), flat_acc, &shield_margin);

	/* roll to-hit die */
	dieroll = rnd(20);
	
	if (wizard && (iflags.wizcombatdebug & WIZCOMBATDEBUG_ACCURACY) && WIZCOMBATDEBUG_APPLIES(magr, mdef)) {
		pline("accuracy = %d, die roll = %d", accuracy, dieroll);
	}
	/* Diverge on aatyp */
	switch (attk->aatyp)
	{
//////////////////////////////////////////////////////////////
// WEAPON
//////////////////////////////////////////////////////////////
		/* various weapon attacks */
	case AT_WEAP:
	case AT_DEVA:
	case AT_XWEP:
	case AT_MARI:
	case AT_HODS:
		
		/* if the player is attacking while swallowed, guaranteed hit */
		if (youagr && u.uswallow && !miss) {
			hit = TRUE;
		}
		
		if ((accuracy > dieroll || (dieroll == 1 && accuracy > -10)) && !miss) {
			hit = TRUE;
		}
		/* multistriking weapons need to determine how many hit, and set ostriking */
		if (weapon && is_multi_hit(weapon) && !miss) {
			weapon->ostriking = 0;
			int attempts = rn2(multistriking(weapon) + 1) + multi_ended(weapon);	/* ex: multistriking == 2 for 1-3 hits.*/
			int subroll;
			if(u.twoweap && (weapon->otyp == STILETTOS || weapon->otyp == WIND_AND_FIRE_WHEELS))
				attempts++;
			for (; attempts && weapon->ostriking < 7; attempts--) {
				if (accuracy > (subroll = rnd(20)) || subroll == 1)
					weapon->ostriking++;
				dieroll = min(dieroll, subroll);
			}
			/* if [dieroll] was not high enough to hit, reduce ostriking by 1 and make [hit] true */
			if (!hit && weapon->ostriking) {
				weapon->ostriking--;
				hit = TRUE;
			}
		}
		break;
//////////////////////////////////////////////////////////////
// MELEE
//////////////////////////////////////////////////////////////
		/* basic hand-to-hand attacks */
	case AT_CLAW:
	case AT_KICK:
	case AT_BITE:
	case AT_OBIT:
	case AT_WBIT:
	case AT_STNG:
	case AT_BUTT:
	case AT_TAIL:
	case AT_TONG:
	case AT_TENT:
	case AT_WHIP:
	case AT_VINE:	// uses touch accuracy
	case AT_WISP:
	case AT_HITS:	// always hits
	case AT_TUCH:	// uses touch accuracy
	case AT_SRPR:	// uses touch accuracy
	case AT_XSPR:	// uses touch accuracy
	case AT_MSPR:	// uses touch accuracy
	case AT_DSPR:	// uses touch accuracy
	case AT_ESPR:	// uses touch accuracy
	case AT_VOMT:	// uses touch accuracy
	/* ranged attack types that are also melee */
	case AT_LNCK:
	case AT_5SBT:
	case AT_LRCH:
	case AT_5SQR:
		/* determine if the attack hits */
		if (attk->aatyp == AT_HITS && !miss) {
			hit = TRUE;
		}
		if ((accuracy > dieroll || (dieroll == 1 && accuracy > -10)) && !miss) {
			hit = TRUE;
		}
		break;
		/* hits if previous 2 attacks hit; always contacts */
	case AT_REND:
	case AT_HUGS:	// also a guaranteed hit if the player is involved and the player and other creature are stuck together
		/* xattacky() would have not called xmeleehity() if the conditions to make either aatyp weren't satisfied
		 * We do not need to check previous attacks
		 */
		if (!miss) {
			hit = TRUE;
		}
		break;
	default:
		/* complain */
		impossible("bad aatyp for xmeleehity (%d)", attk->aatyp);
		return 0;
	}//switch(aatyp)

	/* if we haven't confirmed a hit yet, we missed */
	if (!hit){
		miss = TRUE;
		/* train player's Shield skill if applicable */
		if (youdef && uarms && (dieroll-accuracy <= shield_margin))
			use_skill(P_SHIELD, 1);
	}

	/* AT_DEVA attacks shouldn't print a miss message if it is a subsequent attack that misses */
	/* Hack this in by knowing that repeated AT_DEVA attacks have a flat_acc penalty */
	long longslash = 0;
	if (attk->aatyp == AT_DEVA && flat_acc < 0)
		domissmsg = FALSE;

	if (hit && weapon && magr
		&& CHECK_ETRAIT(weapon, magr, ETRAIT_LONG_SLASH)
	){
		int longdie = dieroll;
		while(longslash < 4 && ROLL_ETRAIT(weapon, magr, (accuracy > (longdie + 20)), (accuracy > (longdie + 40 - rnd(20))))){
			longdie += 20;
			longslash++;
		}
	}

	boolean graze = FALSE;
	int graze_dmg = 0;
	if (miss && weapon && magr
		&& CHECK_ETRAIT(weapon, magr, ETRAIT_GRAZE)
		&& ROLL_ETRAIT(weapon, magr, (accuracy > (dieroll - 20)), (accuracy > (dieroll + 10 - rnd(20))))
	){
		struct weapon_dice wdice;
		/* grab the weapon dice from dmgval_core */
		dmgval_core(&wdice, bigmonst(pd), weapon, weapon->otyp, magr);
		/* add to the bonsdmg counter */
		graze_dmg = wdice.oc_damn + wdice.bon_damn + wdice.flat + weapon->spe;
		
		if(youagr){
			if (flags.verbose)
				You("graze %s.", mon_nam(mdef));
			else
				You("graze it.");
		}
		else {
			if (vis) {
				if (!(vis&VIS_MAGR))
					map_invisible(x(magr), y(magr));
				if (!(vis&VIS_MDEF))
					map_invisible(x(mdef), y(mdef));

				pline("%s grazes %s.",
					Monnam(magr),
					mon_nam_too(mdef, magr)
					);
			}
			else {
				noises(magr, attk);
			}
		}
		graze = TRUE;
		miss = FALSE;
		domissmsg = FALSE;
	}
	/* print a "miss" message */
	if (miss && domissmsg) {
		xymissmsg(magr, mdef, attk, vis, (accuracy == dieroll));
	}

	/* misc after-attack things to deal with */

	/* show I for invisible attackers to the player */
	if (youdef && !canspotmon(magr))
		map_invisible(x(magr), y(magr));

	/* Nearby monsters may be awakened */
	if (!(is_silent_mon(magr) && (cantmove(mdef) || is_silent_mon(mdef)))	/* suppressed entirely if attacker and defender are silent */
		&& rn2(10)															/* suppressed entirely 1/10 times */
		&& !((youagr || youdef) && Stealth && !rn2(3)))						/* suppressed entirely 1/3 times if the player is attacking/defending and stealthy */
		wake_nearto(x(mdef), y(mdef),										/* call function to wake nearby monsters */
		combatNoise(pa) / (((youagr || youdef) && Stealth) ? 2 : 1));		/* range is reduced if the player is involved and stealthy */

	/* show mimics on successful attack */
	if (mdef->mappearance && hit) {
		if (youagr) seemimic_ambush(mdef); else see_passive_mimic(mdef);
	}
	/* If the monster is undetected and hits you, you should know where the attack came from. */
	if (youdef && magr->mundetected && hit && (hides_under(pa) || is_underswimmer(pa))) {
		magr->mundetected = 0;
		if (!(Blind ? Blind_telepat : Unblind_telepat)) {
			struct obj *obj = level.objects[x(magr)][y(magr)];
			const char *what = "nothing at all";

			if (is_pool(x(magr), y(magr), TRUE) && !Underwater)
				what = "the water";
			else if (Blind && obj && obj->dknown)
				what = something;
			else if (obj)
				what = doname(obj);
			pline("%s was hidden under %s!", Amonnam(magr), what);
			newsym(x(magr), y(magr));
		}
	
	}
	/* otherwise, if the magr OR mdef is undetected, reveal them */
	else if (vis && hit && ((!youdef && !youagr && mdef->mundetected) || (!youagr && magr->mundetected))) {
		if (!youdef && mdef->mundetected) {
			mdef->mundetected = 0;
			newsym(x(mdef), y(mdef));
			if (canseemon(mdef) && !sensemon(mdef)) {
				if (u.usleep) You("dream of %s.",
					(mdef->data->geno & G_UNIQ) ?
					a_monnam(mdef) : makeplural(m_monnam(mdef)));
				else pline("Suddenly, you notice %s.", a_monnam(mdef));
			}
		}
		if (!youagr && magr->mundetected) {
			magr->mundetected = 0;
			newsym(x(magr), y(magr));
			if (canseemon(magr) && !sensemon(magr)) {
				if (u.usleep) You("dream of %s.",
					(magr->data->geno & G_UNIQ) ?
					a_monnam(magr) : makeplural(m_monnam(magr)));
				else pline("Suddenly, you notice %s.", a_monnam(magr));
			}
		}
	}
	// /* blast the player sometimes if they're wearing blasty artifact body armor */
	// if (youdef && uarm && uarm->oartifact && !rn2(10))
		// touch_artifact(uarm, &youmonst, FALSE);

	/* if we hit... */
	if (hit) {
		/* DEAL THE DAMAGE */
		result = xmeleehurty_core(magr, mdef, attk, attk, weapon_p, TRUE, -1, dieroll, vis, ranged, longslash);

		/* the player exercises dexterity when hitting */
		if (youagr)
			exercise(A_DEX, TRUE);
	}
	else if(graze){
		result = hmon_general(magr, mdef, attk, attk, weapon_p, (struct obj *)0, (weapon && ranged) ? HMON_THRUST : HMON_WHACK, graze_dmg, 0, FALSE, dieroll, FALSE, vis);
	}
	else {
		result = MM_MISS;
	}

	/* if the attack hits, or if the creature is able to notice it was attacked (but the attack missed) it wakes up */
	if (youdef || (hit || (!mdef->msleeping && mdef->mcanmove)))
		wakeup2(mdef, youagr);

	return result;
}

int
xmeleehurty(struct monst *magr, struct monst *mdef, struct attack *attk, struct attack *originalattk, struct obj **weapon_p, boolean dohitmsg, int flatdmg, int dieroll, int vis, boolean ranged)
{
	return xmeleehurty_core(magr, mdef, attk, originalattk, weapon_p, dohitmsg, flatdmg, dieroll, vis, ranged, 0L);
}

/* xmeleehurty()
 *
 * Called when a creature is to deal damage to another creature with a melee attack.
 * It is assumed that the attack will hit, only damage and effects are in play.
 *
 * Assumes both the attacker and defender are alive and existant when called
 *
 * Monster to monster attacks.  When a monster attacks another (mattackm),
 * any or all of the following can be returned.  See mattackm() for more
 * details.
 * MM_MISS		0x00	aggressor missed
 * MM_HIT		0x01	aggressor hit defender
 * MM_DEF_DIED	0x02	defender died
 * MM_AGR_DIED	0x04	aggressor died
 *
 * Inside the function itself, determines if the attacks hits and
 * what damage was dealt and who survived.
 */
int
xmeleehurty_core(struct monst *magr, struct monst *mdef, struct attack *attk, struct attack *originalattk, struct obj **weapon_p, boolean dohitmsg, int flatdmg, int dieroll, int vis, boolean ranged, unsigned long modifier_flags)
{
	int dmg = 0,					// damage that will be dealt
		ptmp = 0,					// poison type
		result = 0,					// result from self-referential calls of xmeleehurty()
		armpro = 0;					// MC of defender's armor
	boolean uncancelled = TRUE,		// if the attack's specials are not cancelled (probably via armor)
			notmcan = TRUE,			// if the attacker is not cancelled (so it's the player attacking or an uncancelled monster)
			armuncancel = FALSE;	// if armor is responsible for cancelling attack specials
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct obj * weapon = weapon_p ? *(weapon_p) : (struct obj *)0;
	boolean spec = FALSE;			// general-purpose special flag
	struct attack alt_attk = *attk;	// buffer space to modify attacks in 
	struct permonst * pa = youagr ? youracedata : magr->data;
	struct permonst * pd = youdef ? youracedata : mdef->data;
	struct obj * otmp;
	struct monst * mtmp;
	char buf[BUFSZ];
	int damn = attk->damn;
	int damd = attk->damd;
	
	/* Monsters with physical attack scaling treat the physical damage component of their attacks much like spellcasting damage */
	/*  This represents skill, so the player doesn't get the bonus. */
	if(!youagr && attk->adtyp == AD_PHYS && has_phys_scaling(pa) &&
		/*Weapon user, not as good without*/
		!(pa->mtyp == PM_DAO_LAO_GUI_MONK && attk->aatyp == AT_WEAP && !MON_WEP(magr))
	){
		if(flatdmg >= 0){
			flatdmg += d(min(MAX_BONUS_DICE, mlev(magr)/3), attk->damd <= 1 ? 6 : attk->damd);
		}
		else {
			damn += min(MAX_BONUS_DICE, mlev(magr)/3);
			if(damn < 1)
				damn = 1;
			if(damd <= 1)
				damd = 6;
		}
	}
	
	/* First determine the base damage done */
	if (flatdmg >= 0) {
		dmg = flatdmg;
	}
	else if (damn && damd) {
		dmg = d(damn, damd);
	}
	else if(damd && attk->adtyp != AD_PHYS){
		damn = min(MAX_BONUS_DICE, mlev(magr)/3);
		if(damn < 1) damn = 1;
		dmg = d(damn, damd);
	}
	else {
		dmg = 0;
	}
	if(attk->adtyp == AD_PERH){
		dmg *= youdef ? u.ulevel : mdef->m_lev;
	}
	if(pa->mtyp == PM_MOON_S_CHOSEN && attk->aatyp == AT_LRCH && !ranged){
		dmg *= 2;
		dmg -= rn2(2);
	}
	/*Monsters get a small bonus for using a two-handed weapon if it means forgoing offhand weapon attacks*/
	if(!youagr && attk->aatyp == AT_WEAP 
	&& (MON_WEP(magr) && bimanual(MON_WEP(magr), pa))
	&& (attacktype(pa, AT_XWEP))
	){
		dmg += (dmg+1)/2;
	}
	/* worms get increased damage on their bite if they are lined up with momentum */
	if(!youagr && pa->mtyp == PM_LONG_WORM && magr->wormno && attk->aatyp == AT_BITE){
		if(wormline(magr, bhitpos.x, bhitpos.y))
			dmg += d(2,4); //Add segment damage
	}
	/* madness can make the player take more damage */
	if (youdef) {
		if (u.umadness&MAD_SUICIDAL && !BlockableClearThoughts){
			dmg += ((NightmareAware_Insanity)*u.ulevel) / 200;
		}

		if ((pa->mlet == S_SNAKE
			|| pa->mlet == S_NAGA
			|| pa->mtyp == PM_COUATL
			|| pa->mtyp == PM_LILLEND
			|| pa->mtyp == PM_MEDUSA
			|| pa->mtyp == PM_MARILITH
			|| pa->mtyp == PM_MAMMON
			|| pa->mtyp == PM_SHAKTARI
			|| pa->mtyp == PM_DEMOGORGON
			|| pa->mtyp == PM_GIANT_EEL
			|| pa->mtyp == PM_ELECTRIC_EEL
			|| pa->mtyp == PM_KRAKEN
			|| pa->mtyp == PM_SALAMANDER
			|| pa->mtyp == PM_KARY__THE_FIEND_OF_FIRE
			|| pa->mtyp == PM_CATHEZAR
			) && u.umadness&MAD_OPHIDIOPHOBIA && !BlockableClearThoughts && NightmareAware_Sanity < 100){
			dmg += (NightmareAware_Insanity) / 5;
		}

		if ((pa->mlet == S_WORM
			|| mon_attacktype(magr, AT_TENT)
			) && u.umadness&MAD_HELMINTHOPHOBIA && !BlockableClearThoughts && NightmareAware_Sanity < 100){
			dmg += (NightmareAware_Insanity) / 5;
		}

		if (!magr->female && humanoid_upperbody(pa) && u.umadness&MAD_ARGENT_SHEEN && !BlockableClearThoughts && NightmareAware_Sanity < 100){
			dmg += (NightmareAware_Insanity) / 5;
		}

		if ((is_insectoid(pa) || is_arachnid(pa)) && u.umadness&MAD_ENTOMOPHOBIA && !BlockableClearThoughts && NightmareAware_Sanity < 100){
			dmg += (NightmareAware_Insanity) / 5;
		}

		if (is_aquatic(pa) && u.umadness&MAD_THALASSOPHOBIA && !BlockableClearThoughts && NightmareAware_Sanity < 100){
			dmg += (NightmareAware_Insanity) / 5;
		}

		if ((is_spider(pa)
			|| pa->mtyp == PM_SPROW
			|| pa->mtyp == PM_DRIDER
			|| pa->mtyp == PM_PRIESTESS_OF_GHAUNADAUR
			|| pa->mtyp == PM_AVATAR_OF_LOLTH
			) && u.umadness&MAD_ARACHNOPHOBIA && !BlockableClearThoughts && NightmareAware_Sanity < 100){
			dmg += (NightmareAware_Insanity) / 5;
		}

		if (magr->female && humanoid_upperbody(pa) && u.umadness&MAD_ARACHNOPHOBIA && !BlockableClearThoughts && NightmareAware_Sanity < 100){
			dmg += (NightmareAware_Insanity) / 5;
		}
	}

	/*	Next a cancellation factor	*/
	/*	Use uncancelled when the cancellation factor takes into account certain
	 *	armor's special magic protection.  Otherwise just use !mtmp->mcan.
	 */
	armpro = magic_negation(mdef);
	armuncancel = ((rn2(3) >= armpro) || !rn2(10));
	/* hack: elemental gaze attacks call this function with their AT_GAZE; we want that to ignore armor cancellation */
	if (attk->aatyp == AT_GAZE || attk->aatyp == AT_WDGZ)
		armuncancel = TRUE;
	notmcan = (youagr || !magr->mcan);
	/* if we're called with attk->aatyp==AT_NONE, this is some kind of extra effect of magical origin that will bypass armuncancel */
	uncancelled = notmcan && (armuncancel || attk->aatyp == AT_NONE);


	/* Do stuff based on damage type 
	 *  
	 * Physical Damage
	 *   hitmsg
	 *   damage
	 *   modifiers
	 *   more modifiers
	 *   on-hit effects
	 *   RETURN
	 * 
	 * Elemental Damage
	 *   if called directly (AT != AT_NONE)
	 *      makes 0d0 physical attack (hitmsg)
	 *      maybe return
	 *      then makes AT_NONE elemental damage attack
	 *   if called indirectly (AT_NONE)
	 *      elemental effects
	 *      damage (not reduced by DR or halfphysdamage)
	 *      RETURN
	 *   
	 * Status Effects [includes teleportation]
	 *   physical attack (hitmsg)
	 *   maybe return
	 *   special effects
	 * 
	 * Modified Attacks [includes seduction]
	 *   hitmsg
	 *   special effects
	 *   physical attack (no hitmsg)
	 * 
	 * Misc
	 *   hitmsg
	 *   special effects
	 *   followed by physical attack (no hitmsg)
	 *   
	 */

	/* intercept attacks dealing elemental damage to split them apart */
	if (attk->aatyp != AT_NONE && attk->aatyp != AT_GAZE && attk->aatyp != AT_WDGZ) {
		switch (attk->adtyp)
		{
		case AD_MAGM:
		case AD_EFIR:
		case AD_FIRE:
		case AD_ECLD:
		case AD_COLD:
		case AD_EELC:
		case AD_ELEC:
		case AD_EACD:
		case AD_ACID:
		case AD_PSON:
		case AD_DISE:
		case AD_POSN:
		case AD_DARK:
		case AD_LUCK:
			/* make a 0 damage physical attack 
			 * This prints hitmsg and applies on-hit effects of any weapon
			 */
			alt_attk.aatyp = attk->aatyp;
			alt_attk.adtyp = AD_PHYS;
			/* special case: [salamanders'] fire hugs shouldn't print out a "you are being crushed" message, 
			   as they print a "being roasted" message */
			if (originalattk->aatyp == AT_HUGS && (originalattk->adtyp == AD_FIRE || originalattk->adtyp == AD_EFIR))
				dohitmsg = FALSE;
			result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, 0, dieroll, vis, ranged);
			/* return early if cannot continue the attack */
			if (result&(MM_DEF_DIED|MM_DEF_LSVD)) return result;
			/* then, make the elemental attack */
			alt_attk = *attk;
			alt_attk.aatyp = AT_NONE;
			return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);
		default:
			break;
		}
	}

	/* intercept hug attacks to make them try to do a grab (if possible) */
	if (attk->aatyp == AT_HUGS
		&& attk->adtyp != AD_WRAP)
	{
		/* are grabs possible? */
		if (!(youagr || youdef) || sticks(mdef) || (youagr && u.uavoid_grabattk)){
			/* no grabs allowed, substitute basic claw attack */
			alt_attk.aatyp = AT_CLAW;
			return xmeleehurty(magr, mdef, &alt_attk, &alt_attk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		}
		else if (!(u.ustuck && u.ustuck == (youagr ? mdef : magr)))
		{
			/* if we aren't already stuck, try to grab them */
			alt_attk.adtyp = AD_WRAP;
			return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		}
		/* else continue on with the grab attack */
	}

	boolean weaponattk = weapon_aatyp(attk->aatyp);

	switch (attk->adtyp)
	{
//////////////////////////////////////////////////////////////
// BASIC DAMAGE AND SLIGHTLY MODIFIED DAMAGE TYPES
//////////////////////////////////////////////////////////////
		/* 
		 * What almost every damage type ends up being
		 */
	case AD_PHYS:
	case AD_HODS:	/* should be deprecated in favour of just physical damage */
	case AD_SHDW:	/* poisoned, phases (blade of shadow) */
	case AD_STAR:	/* silvered, phases (silver starlight rapier) */
	case AD_BSTR:	/* phases (black-star rapier) */
	case AD_MOON:	/* silvered, phases (silver moonlight rapier) */
	case AD_BLUD:	/* bloodied, phases (blade of blood) */
	case AD_MERC:	/* poisoned, cold, phases (blade of mercury) */
	case AD_GLSS:	/* silvered (mirror-shards) */
	case AD_PERH:	/* physical damage */

		/* abort if called with AT_NONE -- the attack was meant to only do special effects of the adtype. */
		if (attk->aatyp == AT_NONE)
			return result;
		
		/* The Tentacle Rod has a unique hitmessage which will replace the usual hitmsg */
		if (vis&VIS_MAGR && weapon && arti_tentRod(weapon) && dohitmsg) {
			int extrahit = weapon->ostriking + 1;
			pline("The Tentacle Rod hits %s %d time%s%s",
				(youdef ? "you" : mon_nam(mdef)),
				extrahit,
				extrahit > 1 ? "s" : "",
				extrahit < 3 ? "." : extrahit < 7 ? "!" : "!!");
			dohitmsg = FALSE;
		}
		/* special effects of black star rapiers */
		if(attk->adtyp == AD_BSTR){
			if(youdef || !resist(mdef, WEAPON_CLASS, 0, TRUE)){
				*hp(mdef) -= *hp(mdef)/5; /*Percentage based damage, will not kill*/
			}
			if (youdef) {
				if (rndcurse())
					You_feel("as if you need some help.");
				stop_occupation();
			}
			else {
				if (mrndcurse(mdef) && (youagr || canseemon(mdef)))
					You_feel("as though %s needs some help.", mon_nam(mdef));
			}
		}
		/* hit with [weapon] */
		result = hmon_general_modifiers(magr, mdef, attk, originalattk, weapon_p, (struct obj *)0, (weapon && ranged) ? HMON_THRUST : HMON_WHACK, 0, dmg, dohitmsg, dieroll, FALSE, vis, modifier_flags);
		if (weapon_p) weapon = *weapon_p;
		if (result&(MM_DEF_DIED|MM_DEF_LSVD|MM_AGR_DIED|MM_AGR_STOP))
			return result;
		if (weapon && is_multi_hit(weapon) && weapon->ostriking) {
			int i;
			for (i = 0; weapon && (i < weapon->ostriking); i++) {
				result = hmon_general_modifiers(magr, mdef, attk, originalattk, weapon_p, (struct obj *)0, (weapon && ranged) ? HMON_THRUST : HMON_WHACK, 0, 0, FALSE, dieroll, TRUE, vis, modifier_flags);
				if (weapon_p) weapon = *weapon_p;
				if (result&(MM_DEF_DIED|MM_DEF_LSVD|MM_AGR_DIED|MM_AGR_STOP))
					return result;
			}
			do_weapon_multistriking_effects(magr, mdef, attk, weapon, vis);
		}
		return result;
//////////////////////////////////////////////////////////////
// PURE ELEMENTAL DAMAGE
//////////////////////////////////////////////////////////////
		/* 
		 * The damage can be resisted usually in full.
		 * For capital-E "Elemental" damage, resistance tends
		 * to be parial.
		 */
		/* magic */
	case AD_MAGM:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* does it do anything? Nullmagic gives utter and total immunity. MR gives immunity to the damage. */
		if (notmcan
			&& !(youdef && Nullmagic)
			&& !(!youdef && mon_resistance(mdef, NULLMAGIC))){

			/* message */
			if (vis) {
				pline("%s%s lanced by magic!",
					(youdef ? "You" : Monnam(mdef)),
					(youdef ? "'re" : " is"));
			}

			if (Magic_res(mdef))
			{
				/* full resistance */
				dmg = 0;
				/* messages for resisted damage */
				if (youdef) {
					pline_The("magic bounces off!");
				}
				else if (vis) {
					pline_The("magic bounces off %s!",
						mon_nam(mdef));
					shieldeff(mdef->mx, mdef->my);
				}
			}
		}
		else
			dmg = 0;
		return xdamagey(magr, mdef, attk, dmg);
		/* fire */
	case AD_EFIR:	/* elemental version, partially ignores resistance */
	case AD_FIRE:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* does it do anything? */
		if (uncancelled || attk->adtyp == AD_EFIR)
		{
			int olddmg = dmg;	// hack to remember dmg before it is reduced by resistances, so golemeffects() gets a good amount
			/* someone's on fire -- print onfire message! */
			if (vis) {
				pline("%s%s%s!",
					(youdef ? "You" : Monnam(mdef)),
					(youdef ? "'re " : " is "),
					on_fire(pd, originalattk));
			}

			/* burn away slime (player-only) */
			if (youdef){
				burn_away_slime();
				melt_frozen_air();
			}

			/* destory items in inventory */
			/* damage can only kill the player, right now, but it will injure monsters */
			if (!UseInvFire_res(mdef)){
				if ((int)mlev(magr) > rn2(20))
					destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
				if ((int)mlev(magr) > rn2(20))
					destroy_item(mdef, POTION_CLASS, AD_FIRE);
				if ((int)mlev(magr) > rn2(25))
					destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
			}
			/* reduce damage via resistance OR instakill */
			if (Fire_res(mdef))
			{
				if (attk->adtyp == AD_EFIR)	{ /* elemental, is reduced by resistance */
					/* resistances */
					if (youdef)	{
						/* you get 1/2 for intrinsic/extrinsic, 1/4 for both */
						if (HFire_resistance) dmg /= 2;
						if (EFire_resistance) dmg /= 2;
					}
					else {
						/* monsters just get 1/2, unless their species resists (below) */
						dmg /= 2;
					}
					/* immunities */
					if ((species_resists_fire(mdef))
						|| (ward_at(x(mdef), y(mdef)) == SIGIL_OF_CTHUGHA)
						|| (youdef && ((Race_if(PM_HALF_DRAGON) && flags.HDbreath == AD_FIRE)))
						|| (!youdef && is_half_dragon(pd) && mdef->mvar_hdBreath == AD_FIRE)
						|| (youdef && u.sealsActive&SEAL_FAFNIR))
						dmg = 0;
				}
				else {	/* standard, immunity from any resistance */
					dmg = 0;
				}

				/* messages for resisted damage */
				if (dmg == 0) {
					if (youdef) {
						pline_The("fire doesn't feel hot!");
					}
					else if (vis) {
						pline_The("fire doesn't seem to burn %s!",
							mon_nam(mdef));
						shieldeff(mdef->mx, mdef->my);
					}
				}
				else if (attk->adtyp == AD_EFIR && youdef) {
					pline_The("fire still feels hot%s",
						(dmg > 2 ? "!" : "."));
				}
			}
			else {	/* potential instakills to some creatures */
				if (pd->mtyp == PM_STRAW_GOLEM ||
					pd->mtyp == PM_PAPER_GOLEM ||
					pd->mtyp == PM_SPELL_GOLEM) {
					if (youdef) {
						/* assumes the player was polyed and not in natural form */
						You("burn up!");
						rehumanize();
						change_gevurah(1); //cheated death.
						return (MM_HIT | MM_DEF_LSVD);
					}
					else {
						if (vis)
							pline("%s burns completely!", Monnam(mdef));
						if (youagr)
							killed(mdef);
						else
							mondied(mdef);
						if (mdef->mhp > 0)
							return (MM_HIT|MM_DEF_LSVD);	/* lifesaved? */
						else if (mdef->mtame && !vis)
							pline("May %s roast in peace.", mon_nam(mdef));
						return (MM_HIT | MM_DEF_DIED | ((youagr || grow_up(magr, mdef)) ? 0 : MM_AGR_DIED));	/* grow_up might kill magr */
					}
				}
				else if (pd->mtyp == PM_MIGO_WORKER) {
					if (youdef) {
						/* assumes the player was polyed and not in natural form */
						You("burn up!");
						rehumanize();
						change_gevurah(1); //cheated death.
						break;
					}
					else {
						if (vis) pline("%s brain melts!", s_suffix(Monnam(mdef)));
						if (youagr)
							killed(mdef);
						else
							mondied(mdef);
						if (mdef->mhp > 0)
							return (MM_HIT|MM_DEF_LSVD);	/* lifesaved? */
						else if (mdef->mtame && !vis)
							pline("May %s roast in peace.", mon_nam(mdef));
						return (MM_HIT | MM_DEF_DIED | ((youagr || grow_up(magr, mdef)) ? 0 : MM_AGR_DIED));	/* grow_up might kill magr */
					}
				}
			}
			/* some golems heal from fire damage */
			if (youdef)
				ugolemeffects(AD_FIRE, olddmg);
			else
				golemeffects(mdef, AD_FIRE, olddmg);
		}
		else
			dmg = 0;
		return xdamagey(magr, mdef, attk, dmg);
		/* cold */
	case AD_ECLD:
	case AD_COLD:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* does it do anything? */
		if (uncancelled || attk->adtyp == AD_ECLD)
		{
			int olddmg = dmg;	// hack to remember dmg before it is reduced by resistances, so golemeffects() gets a good amount

			/* someone's freezing -- print mesage message! */
			if (vis) {
				pline("%s%s covered in frost!",
					(youdef ? "You" : Monnam(mdef)),
					(youdef ? "'re" : " is"));
			}
			/* madness: cold */
			if (youdef)
				roll_frigophobia();
			/* destory items in inventory */
			/* damage can only kill the player, right now, but it will injure monsters */
			if (!UseInvCold_res(mdef)){
				if ((int)mlev(magr) > rn2(20))
					destroy_item(mdef, POTION_CLASS, AD_COLD);
			}
			/* reduce damage via resistance */
			if (Cold_res(mdef))
			{
				if (attk->adtyp == AD_ECLD)	{ /* elemental, is reduced by resistance */
					/* resistances */
					if (youdef)	{
						if (HCold_resistance) dmg /= 2;
						if (ECold_resistance) dmg /= 2;
					}
					else {
						dmg /= 2;
					}
					/* immunities */
					if ((species_resists_cold(mdef))
						|| (ward_at(x(mdef), y(mdef)) == BRAND_OF_ITHAQUA)
						|| (youdef && ((Race_if(PM_HALF_DRAGON) && flags.HDbreath == AD_COLD)))
						|| (!youdef && is_half_dragon(pd) && mdef->mvar_hdBreath == AD_COLD)
						|| (youdef && u.sealsActive&SEAL_AMON))
						dmg = 0;
				}
				else {	/* standard, immunity from any resistance */
					dmg = 0;
				}

				/* messages for resisted damage */
				if (dmg == 0) {
					if (youdef) {
						pline_The("frost doesn't seem cold!");
					}
					else if (vis) {
						pline_The("frost doesn't seem to chill %s!",
							mon_nam(mdef));
						shieldeff(mdef->mx, mdef->my);
					}
				}
				else if (attk->adtyp == AD_ECLD && youdef) {
					pline_The("frost still feels cold%s",
						(dmg > 2 ? "!" : "."));
				}
			}
			/* some golems are affected by cold damage
			* it doesn't appear to be a thing for player-polyed golems? */
			if (youdef)
				ugolemeffects(AD_COLD, olddmg);
			else
				golemeffects(mdef, AD_COLD, olddmg);
		}
		else
			dmg = 0;
		return xdamagey(magr, mdef, attk, dmg);
		/* shock */
	case AD_EELC:
	case AD_ELEC:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* does it do anything? */
		if (uncancelled || attk->adtyp == AD_EELC)
		{
			int olddmg = dmg;	// hack to remember dmg before it is reduced by resistances, so golemeffects() gets a good amount

			/* someone's shocked -- print mesage message! */
			if (vis) {
				pline("%s %s zapped!",
					(youdef ? "You" : Monnam(mdef)),
					(youdef ? "get" : "gets"));
			}

			/* destory items in inventory */
			/* damage can only kill the player, right now, but it will injure monsters */
			if (!UseInvShock_res(mdef)){
				if ((int)mlev(magr) > rn2(20))
					destroy_item(mdef, WAND_CLASS, AD_ELEC);
			}
			/* reduce damage via resistance */
			if (Shock_res(mdef))
			{
				if (attk->adtyp == AD_EELC)	{ /* elemental, is reduced by resistance */
					/* resistances */
					if (youdef)	{
						if (HShock_resistance) dmg /= 2;
						if (EShock_resistance) dmg /= 2;
					}
					else {
						dmg /= 2;
					}
					/* immunities */
					if ((species_resists_elec(mdef))
						|| (ward_at(x(mdef), y(mdef)) == TRACERY_OF_KARAKAL)
						|| (youdef && ((Race_if(PM_HALF_DRAGON) && flags.HDbreath == AD_ELEC)))
						|| (!youdef && is_half_dragon(pd) && mdef->mvar_hdBreath == AD_ELEC)
						|| (youdef && u.sealsActive&SEAL_ASTAROTH))
						dmg = 0;
				}
				else {	/* standard, immunity from any resistance */
					dmg = 0;
				}

				/* messages for resisted damage */
				if (dmg == 0) {
					if (youdef) {
						pline_The("zap doesn't shock you!");
					}
					else if (vis) {
						pline_The("zap doesn't shock %s!",
							mon_nam(mdef));
						shieldeff(mdef->mx, mdef->my);
					}
				}
				else if (attk->adtyp == AD_EELC && youdef) {
					pline_The("zap still shocks you%s",
						(dmg > 2 ? "!" : "."));
				}
			}
			/* some golems are affected by elec damage
			* it doesn't appear to be a thing for player-polyed golems? */
			if (youdef)
				ugolemeffects(AD_ELEC, olddmg);
			else
				golemeffects(mdef, AD_ELEC, olddmg);
		}
		else
			dmg = 0;
		return xdamagey(magr, mdef, attk, dmg);
		/* acid */
	case AD_EACD:
	case AD_ACID:
	case AD_OMUD:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* active? */
		if ((notmcan && !rn2(3)) || attk->adtyp == AD_EACD) {
			/* someone's splashed -- print mesage message! */
			if (vis) {
				Sprintf(buf, "%s%s covered in %s",
					(youdef ? "You" : Monnam(mdef)),
					(youdef ? "'re" : " is"),
					(attk->adtyp == AD_OMUD ? "writhing tarry mud" : "acid")
				);
			}
			if (Acid_res(mdef)) {
				if (attk->adtyp == AD_EACD)
					dmg /= 2;
				else
					dmg = 0;
			}
			/* print message */
			if (youdef){
				if (dmg == 0) {
					if(attk->adtyp == AD_OMUD)
						Strcat(buf, ".");
					else
						Strcat(buf, ", but it seems harmless.");
				}
				else if (Acid_res(mdef)) {
					Strcat(buf, "! It stings!");
				}
				else {
					Strcat(buf, "! It burns!");
				}
			}
			else {
				if (dmg == 0) {
					Strcat(buf, ", but it seems unharmed.");
				}
				///* random flavour -- a monster nearly killed by acid will scream */
				//else if (dmg > mdef->mhp * 2/3 && !is_silent_mon(mdef)){
				//	Sprintf(buf, "%s! %s screams!", buf, Monnam(mdef));
				//	wake_nearto(x(mdef), y(mdef), combatNoise(pd));
				//}
				else
					Strcat(buf, "!");
			}
			if (youdef || vis) {
				pline("%s", buf);
				if (dmg == 0 && !youdef)
					shieldeff(mdef->mx, mdef->my);
				if(attk->adtyp == AD_OMUD){
					pline("It begins stabbing %s with bone daggers!", youdef ? "you" : mon_nam(mdef));
				}
			}
			if(attk->adtyp == AD_OMUD){
				mdef->momud = TRUE;
			}

			/* erode armor, if inventory isn't protected */
			if (!UseInvAcid_res(mdef)) {
				if (!rn2(10) || attk->adtyp == AD_EACD) {
					erode_armor(mdef, TRUE);
				}
			}
			/* abuse STR */
			if (youdef && !Acid_res(mdef))
				exercise(A_STR, FALSE);
		}
		else
			dmg = 0;
		return xdamagey(magr, mdef, attk, dmg);
	/* psionic damage */
	case AD_PSON:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* active? */
		if (notmcan && (youdef || !mindless_mon(mdef)) && !Catapsi) {
			dmg = reduce_dmg(mdef,dmg,FALSE,TRUE);
			/* print message */
			if (youdef) {
				pline("You get a %sache!", body_part(HEAD));
			}

			/* abuse mental attributes */
			if (youdef){
				exercise(A_INT, FALSE);
				exercise(A_WIS, FALSE);
				exercise(A_CHA, FALSE);
			}
		}
		else
			dmg = 0;
		return xdamagey(magr, mdef, attk, dmg);
	/* sickness damage */
	case AD_DISE:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* asymetric: diseasemu prints out messages, applies sickness to player*/
		if(originalattk->aatyp == AT_VOMT){
			mdef->mcaterpillars = TRUE;
		}
		if (youdef) {
			if (!diseasemu(pa))
				dmg = 0;
		}
		else {
			if (Sick_res(mdef)) {
				if (vis) {
					pline("%s seems unharmed by the disease.",
						Monnam(mdef));
					shieldeff(mdef->mx, mdef->my);
				}
				dmg = 0;
			}
			else {
				if (vis) {
					pline("%s is afflicted by disease!",
						Monnam(mdef));
				}
				if(!rn2(10)) dmg += 100;
				if(!youagr && mdef->mhp < mdef->mhpmax/2 && ((magr->mspores && !rn2(20)) || has_template(magr, SPORE_ZOMBIE) || has_template(magr, CORDYCEPS)))
					mdef->mspores = TRUE;
			}
		}
		return xdamagey(magr, mdef, attk, dmg);
		/* Only poison damage
		 * Not to be confused with a poisonous attack that adds poison bonus damage to a physical strike */
	case AD_POSN:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}

		if (Poison_res(mdef)){
			if (youdef) {
				pline("You're covered in poison, but it seems harmless.");
			}
			else if (vis) {
				pline("%s is covered in poison, but it seems unharmed.",
					Monnam(mdef));
				shieldeff(mdef->mx, mdef->my);
			}
			dmg = 0;
		}
		else {
			if (youdef) {
				pline("You're covered in poison! You don't feel so good!");
				exercise(A_STR, FALSE);
				exercise(A_INT, FALSE);
				exercise(A_CON, FALSE);
				exercise(A_DEX, FALSE);
			}
			else if (vis) {
				pline("%s is covered in poison!",
					Monnam(mdef));
			}
		}
		return xdamagey(magr, mdef, attk, dmg);

	case AD_FATK:{
		boolean exit = FALSE;
		if(!youdef && mindless_mon(mdef))
			return MM_MISS;
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		mindstealer_conflict(mdef, magr);
		if(DEADMONSTER(mdef)){
			result |= MM_DEF_DIED;
			exit = TRUE;
		}
		else if(MIGRATINGMONSTER(mdef)){
			result |= MM_AGR_STOP;
			exit = TRUE;
		}

		if(DEADMONSTER(magr)){
			result |= MM_AGR_DIED;
			exit = TRUE;
		}
		else if(MIGRATINGMONSTER(magr)){
			result |= MM_AGR_STOP;
			exit = TRUE;
		}
		if(exit)
			return result;

		return xdamagey(magr, mdef, attk, dmg);
	}

//////////////////////////////////////////////////////////////
// PHYSICAL DAMAGE BEFORE NON-LETHAL SPECIAL EFFECTS
//////////////////////////////////////////////////////////////
		/* These deal a physical attack with their given dice,
		 * then apply a non-lethal effect, often a staus effect using the same sized dice.
		 * The physical damage is often minor, or non-existant (thus only printing a hit message)
		 */

	case AD_BLND:
		/* make physical attack */
		alt_attk.adtyp = AD_PHYS;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		/* return early if cannot continue the attack */
		if (result&(MM_DEF_DIED|MM_DEF_LSVD))
			return result;

		if (can_blnd(magr, mdef, attk->aatyp, (struct obj*)0)) {
			if (youdef){
				if (!Blind) pline("%s blinds you!", Monnam(magr));
				make_blinded(Blinded + (long)dmg, FALSE);
				if (!Blind) Your1(vision_clears);
			}
			else {
				if (canseemon(mdef) && !is_blind(mdef))
					pline("%s is blinded!", Monnam(mdef));

				int rnd_tmp = d(damn, damd);
				if ((rnd_tmp += mdef->mblinded) > 127) rnd_tmp = 127;
				mdef->mblinded = rnd_tmp;
				mdef->mcansee = 0;
				mdef->mstrategy &= ~STRAT_WAITFORU;
			}
		}
		return result;

	case AD_STUN:
		/* make physical attack */
		alt_attk.adtyp = AD_PHYS;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		/* return early if cannot continue the attack */
		if (result&(MM_DEF_DIED|MM_DEF_LSVD))
			return result;

		if (notmcan && !rn2(4)) {
			if (youdef) {
				make_stunned(HStun + dmg, TRUE);
			}
			else
			{
				if (vis&VIS_MDEF) {
					pline("%s %s for a moment.",
						Monnam(mdef),
						makeplural(stagger(mdef, "stagger")));
				}
				mdef->mstun = 1;
			}
		}
		return result;

	case AD_CONF:
		/* make physical attack */
		alt_attk.adtyp = AD_PHYS;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		/* return early if cannot continue the attack */
		if (result&(MM_DEF_DIED|MM_DEF_LSVD))
			return result;

		if (notmcan && !rn2(4) && (youagr || !magr->mspec_used)) {
			/* put on cooldown */
			if (!youagr)
				magr->mspec_used = magr->mspec_used + (dmg + rn2(6));

			if (youdef) {
				if (Confusion)
					You("are getting even more confused.");
				else
					You("are getting confused.");
				make_confused(HConfusion + dmg, FALSE);
			}
			else
			{
				if (vis&VIS_MDEF)
					pline("%s looks confused.", Monnam(mdef));

				mdef->mconf = 1;
				mdef->mstrategy &= ~STRAT_WAITFORU;
			}
		}
		return result;

	case AD_HALU:
		/* make physical attack */
		alt_attk.adtyp = AD_PHYS;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		/* return early if cannot continue the attack */
		if (result&(MM_DEF_DIED|MM_DEF_LSVD))
			return result;

		if (notmcan && !rn2(4) && (youagr || !magr->mspec_used)) {
			/* put on cooldown */
			if (!youagr)
				magr->mspec_used = magr->mspec_used + (dmg + rn2(6));

			if (youdef) {
				if (!hallucinogenic(pd)) {
					boolean chg;
					if (!Hallucination)
						Your("mind is filled with kaleidoscopic light!");
					chg = make_hallucinated(HHallucination + (long)dmg, FALSE, 0L);
					if (chg && Hallucination && magr->mtyp == PM_DAUGHTER_OF_BEDLAM){
						u.umadness |= MAD_DELUSIONS;
						change_usanity(-1*rnd(6), FALSE); //Deals sanity damage
					}
					You("%s.", chg ? "are freaked out" : "seem unaffected");
				}
			}
			/* monsters get confused and berserked by AD_HALU */
			else if (!mon_resistance(mdef, HALLUC_RES)) {
				if (vis&VIS_MDEF)
					pline("%s looks confused.", Monnam(mdef));

				mdef->mconf = 1;
				mdef->mberserk = 1;
				mdef->mstrategy &= ~STRAT_WAITFORU;
			}
		}
		return result;

	case AD_SLOW:
		/* make physical attack */
		alt_attk.adtyp = AD_PHYS;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		/* return early if cannot continue the attack */
		if (result&(MM_DEF_DIED|MM_DEF_LSVD))
			return result;

		if (uncancelled && !rn2(4)) {
			/* player */
			if (youdef){
				if (HFast) {
					u_slow_down();
				}
			}
			/* monsters */
			else {
				unsigned int oldspeed = mdef->mspeed;

				mon_adjust_speed(mdef, -1, (struct obj *)0, TRUE);
				mdef->mstrategy &= ~STRAT_WAITFORU;
				if (mdef->mspeed != oldspeed && canseemon(mdef))
					pline("%s slows down.", Monnam(mdef));
			}
		}
		return result;

	case AD_SLEE:
		/* make physical attack */
		alt_attk.adtyp = AD_PHYS;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		/* return early if cannot continue the attack */
		if (result&(MM_DEF_DIED|MM_DEF_LSVD))
			return result;

		if (uncancelled
			&& !Sleep_res(mdef)
			&& !cantmove(mdef)
			&& !rn2(7 - mlev(magr))) {
			if (youdef) {
				fall_asleep(-rnd(10), TRUE);
				if (Blind) You("are put to sleep!");
				else You("are put to sleep by %s!", mon_nam(magr));
			}
			else {
				if (sleep_monst(mdef, rnd(10), -1)) {
					pline("%s falls asleep!",
						Monnam(mdef));
					mdef->mstrategy &= ~STRAT_WAITFORU;
					slept_monst(mdef);
				}
			}
		}
		return result;

	case AD_PLYS:
		if(cantmove(mdef) && is_chuul(pa)){
			/*Steal items instead in this case*/
			alt_attk.aatyp = AT_TUCH;
			alt_attk.adtyp = AD_SITM;
			return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		}
		/* make physical attack */
		alt_attk.adtyp = AD_PHYS;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		/* return early if cannot continue the attack */
		if (result&(MM_DEF_DIED|MM_DEF_LSVD))
			return result;
		
		if (uncancelled && !cantmove(mdef) && !rn2(3)) {
			if (youdef){
				if (Free_action) {
					You("momentarily stiffen.");
				}
				else {
					if (Blind) You("are frozen!");
					else You("are frozen by %s!", mon_nam(magr));
					nomovemsg = 0;	/* default: "you can move again" */
					nomul(-dmg, "paralyzed by a monster");
					exercise(A_DEX, FALSE);
				}
			}
			else {
				if (mon_resistance(mdef, FREE_ACTION)) {
					if (canseemon(mdef)) {
						pline("%s momentarily stiffens.", Monnam(mdef));
					}
				}
				else {
					if (canseemon(mdef)) {
						pline("%s freezes!", Monnam(mdef));
					}
					mdef->mcanmove = 0;
					mdef->mfrozen = rnd(10);
					mdef->mstrategy &= ~STRAT_WAITFORU;
				}
			}
		}
		return result;

	case AD_STCK:
		/* make physical attack */
		alt_attk.adtyp = AD_PHYS;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		/* return early if cannot continue the attack */
		if (result&(MM_DEF_DIED|MM_DEF_LSVD))
			return result;

		if (uncancelled
			&& !(result & MM_AGR_DIED)
			&& (youagr || youdef)					/* the player must be involved in a sticking situation (gameplay limitation) */
			&& !u.ustuck							/* can't already be stuck */
			&& !(sticks(magr) && sticks(mdef))		/* creatures can't grab other grabbers (gameplay limitation)  */
			&& !(youagr && u.uavoid_grabattk))		/* the player can opt out of grabbing monsters */
		{
			if (pa->mtyp == PM_TOVE)
				pline("%s %s much too slithy to stick to %s!",
				(youagr ? "You" : Monnam(magr)),
				(youagr ? "are" : "is"),
				((youdef && !youagr) ? "you" : mon_nam_too(mdef, magr))
				);
			else if (pd->mtyp == PM_TOVE)
				pline("%s %s much too slithy to get stuck!",
				(youagr ? "You" : Monnam(magr)),
				(youagr ? "are" : "is")
				);
			else
				u.ustuck = (youagr ? mdef : youdef ? magr : u.ustuck);
		}
		return result;

	case AD_WERE:
		/* make physical attack */
		alt_attk.adtyp = AD_PHYS;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		/* return early if cannot continue the attack */
		if (result&(MM_DEF_DIED|MM_DEF_LSVD))
			return result;

		/* sometimes infect */
		if (uncancelled && !rn2(4)){
			/* only vs players*/
			if (youdef) {
				if (u.ulycn == NON_PM
					&& !Protection_from_shape_changers
					&& !arti_worn_prop(uwep, ARTP_NOWERE)
					&& !umechanoid
					&& is_were(pa)	/* or else Lillends can transmit lycanthropy via mask */
					) {
					You_feel("feverish.");
					exercise(A_CON, FALSE);
					u.ulycn = transmitted_were(pa->mtyp);
				}
			}
			else
				/* not implemented vs monsters */;
		}
		return result;

	case AD_POLY:
		/* make physical attack */
		alt_attk.adtyp = AD_PHYS;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		/* return early if cannot continue the attack */
		if (result&(MM_DEF_DIED|MM_DEF_LSVD))
			return result;
		/* polymorph? */
		if (uncancelled
			&& !(youdef ? Unchanging : mon_resistance(mdef, UNCHANGING))
			&& !(is_rider(pd) || resists_poly(pd))
			){
			/* forced polyself */
			if (youdef) {
				Your("DNA has been altered!");
				polyself(FALSE);
				if (rnd(100) < 15){
					Your("cellular structure degenerates.");
					losexp("cellular degeneration", TRUE, TRUE, FALSE);
				}
			}
			/* monsters only polymorph; never system shock or degenerate */
			else {
				newcham(mdef, NON_PM, FALSE, vis);
			}
		}
		return result;

	case AD_DREN:
		/* we do NOT want to make a physical attack with these damage dice -- they tend to be quite large! */
		alt_attk.adtyp = AD_PHYS;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, 0, dieroll, vis, ranged);
		/* return early if cannot continue the attack */
		if (result&(MM_DEF_DIED|MM_DEF_LSVD))
			return result;

		if ((uncancelled) || pa->mtyp == PM_PALE_NIGHT) {
			if (youdef)
				drain_en(dmg);
			else
				mdef->mspec_used += (dmg+9)/10;
		}
		return result;

	case AD_POLN:
		/* make physical attack */
		if (vis && dohitmsg){
			xyhitmsg(magr, mdef, originalattk);
		}
		alt_attk.adtyp = AD_PHYS;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);
		/* return early if cannot continue the attack */
		if (result&(MM_DEF_DIED|MM_DEF_LSVD))
			return result;
		
		if(youdef){
			You("are covered in pollen!");
			if(!Breathless && !nonliving(pd)){
				You("start sneezing uncontrollably!");
				nomul(-1, "paralyzed by an allergy fit");
			}
		}
		else {
			if(canseemon(mdef))
				pline("%s is covered in pollen!", Monnam(mdef));
			if(!breathless_mon(mdef) && !nonliving(mdef->data)){
				if(canseemon(mdef))
					pline("%s starts sneezing uncontrollably!", Monnam(mdef));
				mdef->mcanmove = 0;
				mdef->mfrozen = rnd(6);
				mdef->mstrategy &= ~STRAT_WAITFORU;
			}
		}
		
		return result;

//////////////////////////////////////////////////////////////
// PHYSICAL DAMAGE AFTER SPECIAL EFFECTS
//////////////////////////////////////////////////////////////
		/* Prints the hitmessage first,
		 * Then does special effects
		 * If the special effects are outright lethal, returns early.
		 * Otherwise, continues on to deal physical damage (without hitmsg) and use its result.
		 */

	case AD_RUST:
		/* print hitmessage */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}

		/* no special effect if cancelled */
		if (notmcan){
			/* instakills */
			if (is_iron(pd)) {
				if (youdef) {
					You("rust!");
					/* KMH -- this is okay with unchanging */
					rehumanize();
					change_gevurah(1); //cheated death.
					return 0;
				}
				else {
					if (vis)
						pline("%s falls to pieces!", Monnam(mdef));

					if (youagr)
						killed(mdef);
					else
						mondied(mdef);

					if (mdef->mhp > 0)
						return (MM_HIT|MM_DEF_LSVD);
					else if (mdef->mtame && !vis && !youagr)
						pline("May %s rust in peace.", mon_nam(mdef));
					return (MM_HIT | MM_DEF_DIED | (youagr || grow_up(magr, mdef) ? 0 : MM_AGR_DIED));
				}
			}

			/* rust armor */
			if (youdef) {
				hurtarmor(AD_RUST, is_dnoble(pa));
			}
			else {
				hurtmarmor(mdef, AD_RUST, is_dnoble(pa));
				mdef->mstrategy &= ~STRAT_WAITFORU;
			}
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_DCAY:
		/* print hitmessage */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}

		/* no special effect if cancelled */
		if (notmcan){
			/* instakills */
			if (pd->mtyp == PM_WOOD_GOLEM ||
				pd->mtyp == PM_GROVE_GUARDIAN ||
				pd->mtyp == PM_LIVING_LECTERN ||
				pd->mtyp == PM_LEATHER_GOLEM)  {

				if (youdef) {
					You("rot!");
					/* KMH -- this is okay with unchanging */
					rehumanize();
					change_gevurah(1); //cheated death.
					return 0;
				}
				else {
					if (vis)
						pline("%s falls to pieces!", Monnam(mdef));

					if (youagr)
						killed(mdef);
					else
						mondied(mdef);
					if (mdef->mhp > 0)
						return 0;
					else if (mdef->mtame && !vis && !youagr)
						pline("May %s rot in peace.", mon_nam(mdef));
					return (MM_HIT | MM_DEF_DIED | (youagr || grow_up(magr, mdef) ? 0 : MM_AGR_DIED));
				}
			}

			/* rot armor */
			if (youdef) {
				hurtarmor(AD_DCAY, is_dnoble(pa));
			}
			else {
				hurtmarmor(mdef, AD_DCAY, is_dnoble(pa));
				mdef->mstrategy &= ~STRAT_WAITFORU;
			}
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_CORR:
		/* print hitmessage */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* corrode armor */
		if (youdef) {
			hurtarmor(AD_CORR, is_dnoble(pa));
		}
		else {
			hurtmarmor(mdef, AD_CORR, is_dnoble(pa));
			mdef->mstrategy &= ~STRAT_WAITFORU;
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_WET:
	case AD_LETHE:
		/* print hitmessage */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
			if (youdef)
				You("are soaking wet!");
			else if (vis&VIS_MDEF)
				pline("%s is soaking wet!", Monnam(mdef));
		}
		/* water/lethe damage */
		water_damage((youdef ? invent : mdef->minvent), FALSE, FALSE, ((pa && pa->mtyp == PM_MOON_FLEA) ? WD_BLOOD : 0 )|(attk->adtyp == AD_LETHE ? WD_LETHE : 0), mdef);
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_ENCH:
		/* print hitmessage */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* disenchant */
		if (uncancelled) {
			otmp = some_armor(mdef);

			if (drain_item(otmp)) {
				if (youdef) {
					Your("%s less effective.",
						aobjnam(otmp, "seem"));
				}
				else if (vis&VIS_MDEF) {
					pline("%s %s less effective.",
						s_suffix(Monnam(mdef)),
						aobjnam(otmp, "seem"));
				}
			}
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

		/* various poisons */
	case AD_PFBT:
	case AD_SVPN:
	case AD_DRST:
	case AD_DRDX:
	case AD_EDRC:
	case AD_DRCO:
		/* select poison type */
		switch (attk->adtyp)
		{
		case AD_SVPN:	ptmp = !rn2(3) ? A_STR : rn2(2) ? A_DEX : A_CON; break;
		case AD_PFBT:
		case AD_DRST:	ptmp = A_STR; break;
		case AD_DRDX:	ptmp = A_DEX; break;
		case AD_EDRC:
		case AD_DRCO:	ptmp = A_CON; break;
		default:		ptmp = A_STR; break;
		}
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* only activates 1/8 times even when uncancelled */
		if (uncancelled && !rn2(8)) {
			/* NOTE: poison acts quite differently betwen player and monster!!! */
			/* vs Player  */
			if (youdef) {
				/* rely on poisoned(), from mon.c */
				Sprintf(buf, "%s %s",
					s_suffix(Monnam(magr)), mpoisons_subj(magr, attk));
				poisoned(buf, ptmp, pa->mname, 30, attk->adtyp == AD_SVPN);
			}
			/* vs Monster */
			else {
				/* print message, maybe */
				if (vis) {
					const char * s = mpoisons_subj(magr, attk);
					pline("%s %s %s poisoned!",
						(youagr ? "Your" : s_suffix(Monnam(magr))),
						s,
						vtense(s, "were")
						);
				}
				/* resistance */
				if (Poison_res(mdef) && attk->adtyp != AD_SVPN) {
					if (vis)
						pline_The("poison doesn't seem to affect %s.",
						mon_nam(mdef));
				}
				else {
					/* 9/10 odds of small bonus damage */
					if (rn2((attk->adtyp != AD_SVPN || Poison_res(mdef)) ? 10 : 5))
						mdef->mhp -= rn1(10, 6);	/* note that this is BONUS damage */
					/* 1/10 of deadly */
					else {
						if (vis)
							pline_The("poison was deadly...");
						if(youagr){
							IMPURITY_UP(u.uimp_poison)
						}
						mdef->mhp = 0;
					}
					/* if the poison killed, deal with the maybe-dead monster and return early */
					if (mdef->mhp < 1) {
						if (youagr)
							killed(mdef);
						else
							monkilled(mdef, "", attk->adtyp);
						/* is it dead, or was it lifesaved? */
						if (mdef->mhp > 0)
							return (MM_HIT|MM_DEF_LSVD);	/* lifesaved */
						else
							return (MM_HIT | MM_DEF_DIED | ((youagr || grow_up(magr, mdef)) ? 0 : MM_AGR_DIED));
					}
				}
			}
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = attk->adtyp == AD_PFBT ? AD_DISE : AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_VAMP:
	case AD_DRLI:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		ptmp = min(*hp(mdef), d(2, 6));	/* amount of draining damage */

		/* Vampiric attacks drain blood, even if those that aren't bites */
		if (attk->adtyp == AD_VAMP
			&& has_blood_mon(mdef)
			&& !(pa->mtyp == PM_VAMPIRE_BAT && !(youdef ? u.usleep : mdef->msleeping))	/* vampire bats need sleeping victims */
		) {
			/* message for a player being drained */
			if (youdef) {
				Your("blood is being drained!");
			}
			if(youdef || youagr){
				IMPURITY_UP(u.uimp_blood)
			}

			/* blood bloaters heal*/
			if (pa->mtyp == PM_BLOOD_BLOATER) {
				/* aggressor lifesteals by dmg dealt */
				heal(magr, min(dmg, *hp(mdef)));
			}

			if(((youagr && u.specialSealsActive&SEAL_YOG_SOTHOTH) || pa->mtyp == PM_TWIN_SIBLING) && !youdef){
				yog_credit(mdef->data->cnutrit/500, FALSE);
			}
			/* Player vampires are smart enough not to feed while
			   biting if they might have trouble getting it down */
			int bite_threshold = 1420;
			if(get_uhungersizemod() > 1)
				bite_threshold *= get_uhungersizemod();
			if (youagr && !Race_if(PM_INCANTIFIER) && is_vampire(youracedata)
				&& u.uhunger <= bite_threshold && attk->aatyp == AT_BITE) {
				/* For the life of a creature is in the blood (Lev 17:11) */
				if (flags.verbose)
				    You("feed on the lifeblood.");
				/* [ALI] Biting monsters does not count against
				   eating conducts. The draining of life is
				   considered to be primarily a non-physical
				   effect */
				lesshungry(ptmp * 6);
			}
			/* tame vampires gain nutrition */
			if (uncancelled && !youagr && get_mx(magr, MX_EDOG))
				EDOG(magr)->hungrytime += ((int)((mdef->data)->cnutrit / 20) + 1);
		}
		/* level-draining effect doesn't actually need blood, it drains life force */
		if ((uncancelled || (attk->adtyp == AD_VAMP && notmcan))
			&& !Drain_res(mdef)
			&& !(pa->mtyp == PM_VAMPIRE_BAT && !(youdef ? u.usleep : mdef->msleeping))	/* vampire bats need sleeping victims */
			&& !rn2(3)
		) {
			if(attk->aatyp == AT_VINE && youdef && !HSterile){
				You_feel("old.");
				HSterile |= TIMEOUT_INF;
				alt_attk.adtyp = AD_PHYS;
				/* make attack without hitmsg */
				return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);
			}
			/* blood bloaters split (but not the player) */
			if (!youagr && pa->mtyp == PM_BLOOD_BLOATER){
				(void)split_mon(magr, 0);
			}

			if(attk->adtyp == AD_VAMP && (youagr || pa->mtyp == PM_TWIN_SIBLING) && !youdef && u.specialSealsActive&SEAL_YOG_SOTHOTH){
				yog_credit(max(mdef->m_lev, mdef->data->cnutrit/50), FALSE);
			}
			/* metroids gain life (but not the player) */
			if (!youagr && is_metroid(pa)) {
				*hpmax(magr) += d(1, 4);
				if(ptmp < 0){
					impossible("Monster (%s) has negative current hp in metroid code (hp: %d ptmp: %d)!", noit_mon_nam(mdef), mdef->mhp, ptmp);
				}
				heal(magr, ptmp/2);
				/* tame metroids gain nutrition (does not stack with for-vampires above, since they have lifedrain instead of bloodsuck attacks) */
				if (get_mx(magr, MX_EDOG)){
					EDOG(magr)->hungrytime += pd->cnutrit / 4;  //400/4 = human nut/4
				}
			}

			/* this line should NOT be displayed in addition to "your blood is being drained" */
			if(youdef){
				if(attk->aatyp == AT_VINE && attk->adtyp == AD_DRLI){
					if(dohitmsg) Your("life force withers as well!");
				} else if (!(attk->adtyp == AD_VAMP && has_blood_mon(mdef)))
					pline("%s feeds on your life force!", Monnam(magr));
			}

			/* drain life! */
			if (youdef) {
				/* the player has a handy level-drain function */
				if(magr->mtyp == PM_STAR_VAMPIRE && !youagr){
					if(rn2(2)){
						losexp("life force drain", FALSE, FALSE, FALSE);
						magr->mvar_star_vampire_blood += 10;
					}
					if(rn2(2)){
						losexp("life force drain", FALSE, FALSE, FALSE);
						magr->mvar_star_vampire_blood += 10;
					}
				}
				losexp("life force drain", TRUE, FALSE, FALSE);
				if(magr->mtyp == PM_STAR_VAMPIRE && !youagr){
					magr->mvar_star_vampire_blood += 10;
					if(magr->perminvis){
						magr->minvis = FALSE;
						magr->perminvis = FALSE;
						newsym(magr->mx,magr->my);
					}
				}
			}
			else {
				/* print message first -- this should happen before the victim is drained/dies */
				if (vis&VIS_MDEF)
					pline("%s suddenly seems weaker!", Monnam(mdef));

				/* kill if this will level-drain below 0 m_lev, or lifedrain below 1 maxhp */
				if (mlev(mdef) == 0 || *hpmax(mdef) <= ptmp) {
					/* clean up the maybe-dead monster, return early */
					if (youagr)
						killed(mdef);
					else
						monkilled(mdef, "", attk->adtyp);
					/* is it dead, or was it lifesaved? */
					if (mdef->mhp > 0)
						return (MM_HIT|MM_DEF_LSVD);	/* lifesaved */
					else
						return (MM_HIT | MM_DEF_DIED | ((youagr || grow_up(magr, mdef)) ? 0 : MM_AGR_DIED));
				}
				else {
					/* drain stats */
					mdef->m_lev--;
					mdef->mhpmax -= ptmp;
					mdef->mhp = min(mdef->mhpmax, mdef->mhp);
				}
			}
		}
		if (attk->adtyp == AD_VAMP
			&& has_blood_mon(mdef)
			&& youagr
			&& !youdef
			&& check_vampire(VAMPIRE_BLOOD_RIP)
			&& !rn2(10)
		){
			You("shape %s blood into spears that pierce %s %s!", s_suffix(mon_nam(mdef)), mhis(mdef), mbodypart(mdef, BODY_SKIN));
			dmg += max(400, 3*mdef->mhpmax/10);
		}

		/* wraithworms have poisonous negative-energy bites */
		if (pa->mtyp == PM_WRAITHWORM
			|| pa->mtyp == PM_FIRST_WRAITHWORM
			|| pa->mtyp == PM_DEMONIC_BLACK_WIDOW) {
			alt_attk.adtyp = AD_DRST;
		}
		else {
			alt_attk.adtyp = AD_PHYS;
		}

		/* make attack without hitmsg */
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_DRHP:
		if(!youdef){
			//The player is more detailed. Vs. a monster convert this into a life drain attack.
			alt_attk.adtyp = AD_DRLI;
			return xmeleehurty(magr, mdef, &alt_attk, &alt_attk, weapon_p, dohitmsg, flatdmg, dieroll, vis, ranged);
		}
		//else drain an extra-healing's worth of bonus HP
		if(u.uhpbonus > 5){
			u.uhpbonus = u.uhpbonus-5;
			dmg += 5;
		}
		else{
			dmg += u.uhpbonus;
			u.uhpbonus = 0;
		}
		calc_total_maxhp();

		/*Use a physical attack to do damage.*/
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);
	case AD_MDWP:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}

		/* level-draining effect caused by memory loss */
		if ((uncancelled || (attk->aatyp == AT_BITE && notmcan))
			&& (youdef || !mindless_mon(mdef)) /* The player is never mindless */
		){
			ptmp = min(*hp(mdef), d(2, 6));	/* amount of draining damage */
			/* drain life! */
			if (youdef) {
				/* the player has a handy level-drain function */
				forget(100/u.ulevel); //drain some proportion of your memory
				losexp("memory loss", TRUE, TRUE, TRUE);
			}
			else {
				/* print message first -- this should happen before the victim is drained/dies */
				if (vis&VIS_MDEF)
					pline("%s suddenly seems confused!", Monnam(mdef));

				/* kill if this will level-drain below 0 m_lev, or lifedrain below 1 maxhp */
				if (mlev(mdef) == 0 || *hpmax(mdef) <= ptmp) {
					/* clean up the maybe-dead monster, return early */
					if (youagr)
						killed(mdef);
					else
						monkilled(mdef, "", attk->adtyp);
					/* is it dead, or was it lifesaved? */
					if (mdef->mhp > 0)
						return (MM_HIT|MM_DEF_LSVD);	/* lifesaved */
					else
						return (MM_HIT | MM_DEF_DIED | ((youagr || grow_up(magr, mdef)) ? 0 : MM_AGR_DIED));
				}
				else {
					/* drain stats */
					mdef->m_lev--;
					mdef->mhpmax -= ptmp;
					mdef->mhp = min(mdef->mhpmax, mdef->mhp);
				}
			}
		}

		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_DESC:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		if (nonliving(pd) || is_anhydrous(pd)){
			if (vis) {
				shieldeff(x(mdef), y(mdef));
			}
			dmg = 0;
		}
		else {
			/* message */
			if (vis) {
				pline("%s %s dehydrated!",
					(youdef ? "You" : Monnam(mdef)),
					(youdef ? "are" : "is")
					);
			}
			/* 2x damage to watery creatures */
			if (is_watery(pd))
				dmg *= 2;
			/* aggressor lifesteals by dmg dealt */
			heal(magr, min(dmg, *hp(mdef)));
		}
		/* make attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_STON:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}

		/* 1/3 chance of special effects */
		if (!rn2(3)) {
			/* a cancelled *trice only coughs*/
			if (!notmcan) {
				/* note: since the player cannot be cancelled, there is no cough for them. */
				/* message */
				if (vis) {
					You_hear("a cough from %s!", mon_nam(magr));
				}
				else if (dist2(u.ux, u.uy, x(magr), y(magr)) < 25) {	/* utterly arbitrary distance to hear cough */
					You_hear("a nearby cough.");
				}
			}
			/* hissing cockatrice */
			else {
				/* message */
				if (youagr) {
					You("hiss!");
				}
				else if (dist2(u.ux, u.uy, x(magr), y(magr)) < 25) {	/* utterly arbitrary distance to hear hissing */
					if (vis&VIS_MAGR)
						You_hear("%s hissing!", s_suffix(mon_nam(magr)));
					else
						You_hear("nearby hissing.");
				}
				/* stoning */
				if (!rn2(10) ||
					(flags.moonphase == NEW_MOON && (youdef && !have_lizard()))
					){
					/* do stone */
					result = xstoney(magr, mdef);
					if (result&(MM_DEF_DIED | MM_DEF_LSVD | MM_AGR_DIED))
						return result;
				}
			}
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_SSTN:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}

		/* 1/3 chance of special effects */
		if (!rn2(3) && notmcan) {
			/* stoning */
			if (!rn2(10) || (youdef && !have_lizard())){
				/* do stone */
				result = xstoney(magr, mdef);
				if (result&(MM_DEF_DIED | MM_DEF_LSVD | MM_AGR_DIED))
					return result;
			}
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_SLIM:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		if (uncancelled) {
			/* flaming immunity to slime */
			if (flaming(pd)
				|| pd->mtyp == PM_RED_DRAGON
				|| wearing_dragon_armor(mdef, PM_RED_DRAGON)
				) {
				/* message if visible */
				if (vis) {
					pline_The("slime burns away!");
				}
			}
			/* unchanging immunity to slime (retests a few of the above but meh) */
			else if (Slime_res(mdef)) {
				/* only message for the player defending */
				if (youdef) {
					You("are unaffected.");
				}
			}
			/* affected! */
			else {
				/* you are slimed */
				if (youdef) {
					if (!Slimed) {
						You("don't feel very well.");
						Slimed = 10L;
						flags.botl = 1;
						killer_format = KILLED_BY_AN;
						delayed_killer = magr->data->mname;
					}
					else {
						pline("Yuck!");
					}
				}
				/* you slimed it */
				else if (youagr) {
					You("turn %s into slime.", mon_nam(mdef));
					monslime(mdef);
					if(mdef->mtyp != PM_GREEN_SLIME)
						pline("...Or maybe not.");
				}
				/* monster slimed it */
				else {
					monslime(mdef);
					mdef->mstrategy &= ~STRAT_WAITFORU;
				}
			}
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_WISD:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* maybe drain WIS */
		if (uncancelled && !rn2(8)) {
			/* player-only */
			if (youdef) {
				pline("%s assaults your sanity!", Monnam(magr));
				if (u.sealsActive&SEAL_HUGINN_MUNINN){
					unbind(SEAL_HUGINN_MUNINN, TRUE);
				}
				else {
					(void)adjattrib(A_WIS, -dmg / 10 + 1, FALSE);
					forget(10);	/* lose 10% of memory */
					exercise(A_WIS, FALSE);
				}
			}
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

		/* "not-poison" drain stat attacks */
	case AD_NPDS:
	case AD_NPDD:
	case AD_NPDC:
	case AD_NPDR:
	case AD_NPDA:
		/* select stat to drain */
		switch (attk->adtyp)
		{
		case AD_NPDS:	ptmp = A_STR; break;
		case AD_NPDD:	ptmp = A_DEX; break;
		case AD_NPDC:	ptmp = A_CON; break;
		case AD_NPDR:	ptmp = A_CHA; break;
		case AD_NPDA:	ptmp = A_STR; break;	/* Special case: drain ALL stats, by two, starting with STR */
		}
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* maybe drain stat */
		if (notmcan) {
			int count = 0;
			int amnt = (attk->adtyp == AD_NPDA) ? 2 : 1;
			do {
				/* player-only */
				if (youdef && ACURR(ptmp) > 3) {
					(void)adjattrib(ptmp, -amnt, FALSE);
				}
				/* monsters take (amnt)d10 damage */
				else {
					result = xdamagey(magr, mdef, attk, d(amnt, 10));
					/* return early if cannot continue the attack */
					if (result&(MM_DEF_DIED|MM_DEF_LSVD))
						return result;
					/* otherwise continue to finish the attack (with physical damage) */
				}

				/* used only in drain-all: change what ptmp is */
				ptmp = (ptmp+1)%A_MAX;
				count++;
			}while(attk->adtyp == AD_NPDA && count < A_MAX);
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_DOBT:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* may inject doubt (blocked by MC or cancelling the monster) */
		if (uncancelled) {
			/* player is more detailed */
			if (youdef) {
				make_doubtful(itimeout_incr(HDoubt, dmg*20), TRUE);
			}
			else {
				mdef->mdoubt = TRUE;
			}
		}
		/* make poison/physical attack without hitmsg */
		alt_attk.adtyp = AD_DRST;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_APCS:{
		boolean hits;
		/* print a hit message */
		if(!youdef){
			if(is_deaf(mdef) || !intelligent_mon(mdef)){
				return MM_MISS;
			}
		}
		if(!notmcan){
			if(vis && dohitmsg){
				if(youagr)
					pline("You whisper banal platitudes in %s %s.", s_suffix(mon_nam(mdef)), mbodypart(mdef, EAR));
				else if(youdef)
					pline("%s whispers banal platitudes in your %s.", Monnam(magr), body_part(EAR));
				else
					pline("%s whispers banal platitudes in %s %s.", Monnam(magr), s_suffix(mon_nam(mdef)), mbodypart(mdef, EAR));
			}
			return MM_MISS;
		}
		if(vis && dohitmsg){
			if(youagr)
				pline("You whisper terrible truths in %s %s.", s_suffix(mon_nam(mdef)), mbodypart(mdef, EAR));
			else if(youdef)
				pline("%s whispers terrible truths in your %s.", Monnam(magr), body_part(EAR));
			else
				pline("%s whispers terrible truths in %s %s.", Monnam(magr), s_suffix(mon_nam(mdef)), mbodypart(mdef, EAR));
		}

		/*Check insight*/
		if(youdef){
			//Lifts the veil
			lift_veil();
			hits = rn2(Insight) >= 10;
		}
		else {
			//Just do a level check for monsters
			hits = rn2(mdef->m_lev) >= 10;
		}

		if(!hits){
			if(youdef){
				pline("But you fail to understand!");
				change_uinsight(1);
				make_confused(HConfusion + 10L, TRUE);
			}
			else {
				if(vis)
					pline("%s staggers!", Monnam(mdef));
				mdef->mstun = 1;
				mdef->mconf = 1;
			}
		}
		else {
			if(youdef){
				dmg = Insanity/5;
				if(Insanity%5 > rn2(5))
					dmg++;
			}
			else {
				if(youagr)
					dmg = rnd(u.ulevel);
				else
					dmg = rnd(monstr[magr->mtyp]);
				if(mm_resist(mdef, magr, 0, NOTELL))
					dmg = max(1, dmg/2);
			}
			switch(rn2(9)){
				/*Sow doubt*/
				case 0:
					/* player is more detailed */
					if (!youdef) {
						if(canseemon(mdef))
							pline("%s looks doubtful.", Monnam(mdef));
						mdef->mdoubt = TRUE;
					}
					else {
						pline("Doubt clouds your heart.");
						make_doubtful(itimeout_incr(HDoubt, dmg*50), TRUE);
					}
					return MM_HIT;
				/*Blaspheme*/
				case 1:
					/* player is more detailed */
					if (!youdef){
						if(canseemon(mdef))
							pline("%s looks doubtful.", Monnam(mdef));
						if(rn2(10)){
							struct attack fakespell = { AT_MAGC, AD_CLRC, 10, 7 };
							cast_spell(magr, mdef, &fakespell, LIGHTNING, mdef->mx, mdef->my);
							return MM_HIT;
						}
						else {
							alt_attk.aatyp = AT_HITS;
							alt_attk.adtyp = AD_DISN;
							alt_attk.damn = 7;
							alt_attk.damd = 1;
							
							return xmeleehurty(magr, mdef, &alt_attk, &alt_attk, (struct obj **) 0, FALSE, 7, 20, vis, TRUE);
						}
					}
					else {
						/* angers a random pantheon god */
						pline("Blasphemous thoughts fill your mind!");
						if(Infuture){
							u.ualign.record -= rnd(20);
							u.ualign.sins++;
							godlist[GOD_ILSENSINE].anger++;
							angrygods(GOD_ILSENSINE);
						}
						else {
							int angrygod = align_to_god(A_CHAOTIC + rn2(3));
							u.ualign.record -= rnd(20);
							u.ualign.sins++;
							change_hod(rnd(20));
							godlist[angrygod].anger++;
							angrygods(angrygod);
						}
					}
					return MM_HIT;
				/*Lower Sanity*/
				case 2:
					/* player is more detailed */
					if (!youdef) {
						if(canseemon(mdef))
							pline("%s looks confused!", Monnam(mdef));
						mdef->mstun = 1;
						mdef->mconf = 1;
					}
					else {
						pline("The truth shakes the foundations of your mind!");
						change_usanity(u_sanity_loss_minor(magr), TRUE);
					}
					return MM_HIT;
				/*Tear at self*/
				case 3:
					if(!youdef){
						if(canseemon(mdef))
							pline("%s tears at %sself.", Monnam(mdef), mhis(mdef));
						xdamagey(magr, mdef, attk, d(dmg,10));
					}
					else {
						if (u.sealsActive&SEAL_HUGINN_MUNINN){
							unbind(SEAL_HUGINN_MUNINN, TRUE);
						}
						else {
							pline("The truth tears down the foundations of your mind!");
							while (!(ABASE(A_WIS) <= ATTRMIN(A_WIS)) && dmg > 0) {
								dmg--;
								(void)adjattrib(A_WIS, -1, TRUE);
								forget(4);	/* lose 4% of memory per point lost*/
								exercise(A_WIS, FALSE);
							}
							if (dmg > 0) {
								You("tear at yourself in horror!"); //assume always able to damage self
								xdamagey(magr, mdef, attk, d(dmg,10));
							}
						}
					}
					return MM_HIT;
				/*Prophesize doom*/
				case 4:{
					int maxDoom = youagr ? u.ulevel : monstr[magr->mtyp];
					pline("%s shares a prophecy of doom!", Monnam(magr));
					if(!youdef){
						if(canseemon(mdef))
							pline("%s looks despondent.", Monnam(mdef));
						mdef->encouraged -= dmg;
						if(mdef->mstdy > -maxDoom){
							mdef->mstdy = max(mdef->mstdy-dmg, -maxDoom);
						}
					}
					else {
						You_feel("despondent.");
						u.uencouraged -= dmg;
						if(u.ustdy > -maxDoom){
							u.ustdy = max(u.ustdy-dmg, -maxDoom);
						}
					}
					return MM_HIT;
				}
				/*Rider attack*/
				case 5:
					switch(rn2(4)){
						case 0:
							alt_attk.adtyp = AD_DETH;
							alt_attk.damn = 8;
							alt_attk.damd = 8;
							return xmeleehurty(magr, mdef, &alt_attk, originalattk, (struct obj **) 0, TRUE, 0, 20, vis, TRUE);
						case 1:
							alt_attk.adtyp = AD_FAMN;
							alt_attk.damn = 8;
							alt_attk.damd = 8;
							return xmeleehurty(magr, mdef, &alt_attk, originalattk, (struct obj **) 0, TRUE, 0, 20, vis, TRUE);
						case 2:
							alt_attk.adtyp = AD_PEST;
							alt_attk.damn = 8;
							alt_attk.damd = 8;
							return xmeleehurty(magr, mdef, &alt_attk, originalattk, (struct obj **) 0, TRUE, 0, 20, vis, TRUE);
						case 3:
							alt_attk.adtyp = AD_CNFT;
							alt_attk.damn = 8;
							alt_attk.damd = 8;
							return xmeleehurty(magr, mdef, &alt_attk, originalattk, (struct obj **) 0, TRUE, 0, 20, vis, TRUE);
					}
					//Not reached
					return MM_MISS;
				/*Lower protection*/
				case 6:
					if(youdef){
						pline("%s shares a prophecy of your death!", Monnam(magr));
						u.uacinc -= dmg;
						u.ublessed = max(0, u.ublessed-dmg);
					}
					else {
						if(canseemon(mdef))
							pline("%s looks fearful.", Monnam(mdef));
						//Monsters are less detailed, just discourage them.
						mdef->encouraged = max(-dmg, mdef->encouraged-dmg);
					}
					return MM_HIT;
				/*Lower luck*/
				case 7:
					if(youdef){
						pline("%s shares a prophecy of great misfortune!", Monnam(magr));
						change_luck(-dmg);
					}
					else {
						if(canseemon(mdef))
							pline("%s looks fearful.", Monnam(mdef));
						//Monsters are less detailed, just discourage them.
						mdef->encouraged = max(-dmg, mdef->encouraged-dmg);
					}
					return MM_HIT;
				/*Water to blood*/
				case 8:
					pline("Space bleeds around %s.", youdef ? "you" : mon_nam(mdef));
					(void) water_damage(youdef ? invent : mdef->minvent, FALSE, FALSE, WD_BLOOD, mdef);
					return MM_HIT;
			}
		}
		if(alt_attk.damn || alt_attk.damd){
			/* make physical attack without hitmsg */
			alt_attk.adtyp = AD_PHYS;
			return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);
		}
		else return MM_HIT;
	}
	case AD_LRVA:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* lay an egg */
		if (notmcan) {
			/* player */
			if (youdef) {
				u.utaneggs++;
			}
			else {
				mdef->mtaneggs = min(100, mdef->mtaneggs+1);
			}
		}
		/* make poison/physical attack without hitmsg */
		alt_attk.adtyp = AD_DRCO;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_CURS:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		if ((notmcan && (!rn2(10) || pa->mtyp == PM_PALE_NIGHT))
			&& !(pa->mtyp == PM_GREMLIN && !night())
			)
		{
			/* message */
			if (flags.soundok) {
				if (youagr)
				{
					You("chuckle.");
				}
				else if (vis) {
					if (Blind) You_hear("laughter.");
					else       pline("%s chuckles.", Monnam(magr));
				}
			}
			/* effect */
			if (youdef) {
				IMPURITY_UP(u.uimp_theft);
				if (u.umonnum == PM_CLAY_GOLEM || u.umonnum == PM_SPELL_GOLEM) {
					pline("Some writing vanishes from your head!");
					/* KMH -- this is okay with unchanging */
					rehumanize();
					change_gevurah(1); //cheated death.
					return (MM_HIT|MM_DEF_LSVD);	/* You died but didn't actually die. Lifesaved. */
				}
				attrcurse();
			}
			else {
				if(youagr)
					IMPURITY_UP(u.uimp_theft);
				set_mcan(mdef, TRUE); /* cancelled regardless of lifesave */
				mdef->mstrategy &= ~STRAT_WAITFORU;
				if (is_were(pd) && pd->mlet != S_HUMAN)
					were_change(mdef);
				if (pd->mtyp == PM_CLAY_GOLEM || pd->mtyp == PM_SPELL_GOLEM) {
					if (vis) {
						pline("Some writing vanishes from %s head!",
							s_suffix(mon_nam(mdef)));
						pline("%s is destroyed!", Monnam(mdef));
					}
					if (youagr)
						xkilled(mdef, 0);
					else
						mondied(mdef);

					if (mdef->mhp > 0)
						return (MM_HIT|MM_DEF_LSVD);
					else if (mdef->mtame && !vis && !youagr)
						You("have a strangely sad feeling for a moment, then it passes.");
					return (MM_HIT | MM_DEF_DIED | ((youagr || grow_up(magr, mdef)) ? 0 : MM_AGR_DIED));
				}
			}
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_VORP:
		/* do NOT immediately print a basic hit message -- vorpality can cause a miss */

		/* 1/20 chance of a vorpal strike (or 20/20 vs jabberwocks) */
		if (!rn2(20) || pd->mtyp == PM_JABBERWOCK)
			spec = TRUE;

		if (spec &&
			(!has_head(pd))) {
			if (vis){
				pline("Somehow, %s %s %s wildly.",
					(youagr ? "you" : mon_nam(magr)),
					(youagr ? "miss" : "misses"),
					(youdef ? "you" : mon_nam(mdef))
					);
			}
			return MM_MISS;
		}
		else {
			/* print basic hit message */
			if (vis && dohitmsg) {
				xyhitmsg(magr, mdef, originalattk);
			}
		}

		/* spec is true if vorpal */
		if (spec){
			if (noncorporeal(pd) || amorphous(pd)) {
				if (vis) {
					pline("%s slice%s through %s %s.",
						(youagr ? "You" : Monnam(magr)),
						(youagr ? "" : "s"),
						(youdef ? "your" : s_suffix(mon_nam(mdef))),
						(youdef ? body_part(NECK): mbodypart(mdef, NECK))
						);
				}
			}
			else if(!check_res_engine(mdef, AD_VORP)){
				/* find helmet */
				otmp = (youdef ? uarmh : which_armor(mdef, W_ARMH));

				/* no helmet... DECAPITATION! */
				if (!otmp) {
					/* message */
					if (vis) {
						pline("%s decapitate%s %s!",
							(youagr ? "You" : Monnam(magr)),
							(youagr ? "" : "s"),
							(youdef ? "you" : mon_nam(mdef))
							);
					}
					/* kill */
					Sprintf(buf, "decapitated by %s", mon_nam(magr));
					killer = buf;
					killer_format = NO_KILLER_PREFIX;
					return xdamagey(magr, mdef, attk, FATAL_DAMAGE_MODIFIER);
				}
				else if(!Preservation){
					/* destroy the helmet */
					if (!otmp->oartifact || (pa->mtyp == PM_DEMOGORGON)){
						if (youdef)
							claws_destroy_arm(otmp);
						else
							claws_destroy_marm(mdef, otmp);
					}
					/* double damage! */
					dmg *= 2;
				}
			}
		}

		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_SHRD:
		/* get a piece of worn armor */
		otmp = some_armor(mdef);

		if(!check_res_engine(mdef, AD_SHRD)){
			/* armor protects ye */
			if (otmp){
				/* print custom message */
				if (vis && dohitmsg) {
					switch (attk->aatyp)
					{
					case AT_LNCK:
					case AT_BITE:
					case AT_5SBT:
					case AT_OBIT:
					case AT_WBIT:
						pline("%s teeth catch on %s armor!",
							(youagr ? "Your" : s_suffix(Monnam(magr))),
							(youdef ? "your" : s_suffix(mon_nam(mdef)))
							);
						break;
					case AT_STNG:
						pline("%s stinger catches on %s armor!",
							(youagr ? "Your" : s_suffix(Monnam(magr))),
							(youdef ? "your" : s_suffix(mon_nam(mdef)))
							);
						break;
					case AT_BUTT:
						pline("%s %s catch%s on %s armor!",
							(youagr ? "Your" : s_suffix(Monnam(magr))),
							(num_horns(pa) == 0 ? "head" : num_horns(pa) == 1 ? "horn" : "horns"),
							(num_horns(pa) == 1 ? "es" : ""),
							(youdef ? "your" : s_suffix(mon_nam(mdef)))
							);
						break;
					case AT_TENT:
						pline("%s tentacles catch on %s armor!",
							(youagr ? "Your" : s_suffix(Monnam(magr))),
							(youdef ? "your" : s_suffix(mon_nam(mdef)))
							);
						break;
					case AT_VINE:
						pline("%s vines grow into %s armor!",
							(youagr ? "Your" : s_suffix(Monnam(magr))),
							(youdef ? "your" : s_suffix(mon_nam(mdef)))
							);
						break;
					case AT_EXPL:
					case AT_BOOM:
						pline("%s shrapnel hits %s armor!",
							(youagr ? "Your" : s_suffix(Monnam(magr))),
							(youdef ? "your" : s_suffix(mon_nam(mdef)))
							);
						break;
					case AT_WEAP:
					case AT_XWEP:
					case AT_MARI:
						pline("%s %s %s armor!",
							(youagr ? "Your" : s_suffix(Monnam(magr))),
							(weapon ? "weapon strikes" : "claws catch on"),
							(youdef ? "your" : s_suffix(mon_nam(mdef)))
							);
						break;
					case AT_TAIL:
						pline("%s scales catch on %s armor!",
							(youagr ? "Your" : s_suffix(Monnam(magr))),
							(youdef ? "your" : s_suffix(mon_nam(mdef)))
							);
						break;
					case AT_TONG:
						pline("%s tongue catches on %s armor!",
							(youagr ? "Your" : s_suffix(Monnam(magr))),
							(youdef ? "your" : s_suffix(mon_nam(mdef)))
							);
						break;
					default:
						pline("%s claws catch on %s armor!",
							(youagr ? "Your" : s_suffix(Monnam(magr))),
							(youdef ? "your" : s_suffix(mon_nam(mdef)))
							);
						break;
					}
				}
				if(Preservation){
					pline("But, no harm is done!");
				} else {
					int i = 1;
					if (pa->mtyp == PM_DEMOGORGON || pa->mtyp == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH)
						i += rnd(4);

					for (; i>0; i--){
						if (otmp->spe > -1 * a_acdr(objects[(otmp)->otyp])){
							damage_item(otmp);
						}
						else if (!otmp->oartifact || (pa->mtyp == PM_DEMOGORGON && !rn2(10))){
							if (youdef)
								claws_destroy_arm(otmp);
							else
								claws_destroy_marm(mdef, otmp);
							/* exit armor-destroying loop*/
							break;
						}
					}
				}
				/* make a physical attack without hitmsg */
				alt_attk.adtyp = AD_PHYS;
				return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);
			}
			/* no armor */
			else {
				/* Demogorgon tries to kill */
				if (pa->mtyp == PM_DEMOGORGON || pa->mtyp == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH) {
					if (noncorporeal(pd) || amorphous(pd)) {
						/* custom hit message */
						if (vis && dohitmsg) {
							pline("%s %s to rip %s apart!",
								(youagr ? "You" : Monnam(magr)),
								(youagr ? "try" : "tries"),
								(youdef ? "you" : mon_nam(mdef))
								);
						}
						/* Double damage */
						dmg *= 2;
						/* make a physical attack without hitmsg */
						alt_attk.adtyp = AD_PHYS;
						return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);
					}
					else {
						/* custom hit message */
						if (vis && dohitmsg) {
							pline("%s rip%s %s apart!",
								(youagr ? "You" : Monnam(magr)),
								(youagr ? "" : "s"),
								(youdef ? "you" : mon_nam(mdef))
								);
						}
						/* kill */
						killer = pa->mtyp == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH ? "ripped apart by Blibdoolpoolp" : "ripped apart by Demogorgon";
						killer_format = NO_KILLER_PREFIX;
						return xdamagey(magr, mdef, attk, FATAL_DAMAGE_MODIFIER);
					}
				}
			}
		}
		/* make physical attack WITH hitmsg, since none of the cases above applied */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);

	case AD_MALK:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* attempt to stick */
		if ((youagr || youdef)					/* the player must be involved in a sticking situation (gameplay limitation) */
			&& !u.ustuck						/* can't already be stuck */
			&& !(sticks(magr) && sticks(mdef))	/* grabbers can't grab other grabbers (gameplay limitation)  */
			&& !(youagr && u.uavoid_grabattk)	/* the player can opt out of grabbing monsters */
		){
			if (pd->mtyp == PM_TOVE)
			{
				/* note: assumes player is either agr or def, vis is true */
				pline("%s, %s %s much too slithy to grab!",
					(youagr ? "Unfortunately" : "Fortunately"),
					(youagr ? mon_nam(mdef) : "you"),
					(youagr ? "is" : "are")
					);
			}
			else
				u.ustuck = (youdef ? magr : youagr ? mdef : u.ustuck); /* it is now stuck to you, and you to it */
		}
		/* electric damage */
		if (uncancelled) {
			dmg = d(1, 4);
			int old_aatyp = alt_attk.aatyp;	/* save aatyp to put back after making ELEC attack */
			alt_attk.aatyp = AT_NONE;
			alt_attk.adtyp = AD_ELEC;
			result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);
			if (result&(MM_DEF_DIED | MM_DEF_LSVD | MM_AGR_DIED))
				return result;
			alt_attk.aatyp = old_aatyp;
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);


	case AD_TCKL:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* weeping angels are entirely immune to the special effects */
		if (!is_weeping(pd)) {
			/* very different behaviour between mhitu and xhitm */
			if (youdef) {
				if (armuncancel && multi >= 0) {
					if (Free_action)
						You_feel("horrible fingers probing your flesh!  But you are not distracted!");
					else {
						if (Blind) You("are mercilessly tickled!");
						else You("are mercilessly tickled by %s!", mon_nam(magr));
						nomovemsg = 0;	/* default: "you can move again" */
						if (!rn2(3)) nomul(-rnd(10), "being tickled to death");
						else nomul(-1, "being tickled to death");
						exercise(A_DEX, FALSE);
						exercise(A_CON, FALSE);
						if ((uwep && !welded(uwep)) || (u.twoweap && uswapwep)){
							if (d(1, 100) - min(ACURR(A_DEX), ACURR(A_CON)) > 0){
								if (u.twoweap && uswapwep){//You may be twoweaponing offhand martial arts.
									You("lose hold of your weapons.");
									u.twoweap = FALSE;
									otmp = uswapwep;
									setuswapwep((struct obj *)0);
									freeinv(otmp);
									(void)dropy(otmp);
								}
								else You("lose hold of your weapon.");
								otmp = uwep;
								uwepgone();
								freeinv(otmp);
								(void)dropy(otmp);
								if (roll_madness(MAD_TALONS)){
									You("panic after having dropping your weapon!");
									HPanicking += 1+rnd(6);
								}
							}
							else{
								You("keep a tight grip on your %s!", u.twoweap ? "weapons" : "weapon");
							}
						}
					}
				}
				else if (uarmc){
					You_feel("horrible fingers tug at your cloak.");
					if (d(1, 100) - ACURR(A_DEX) > 0){
						pline("The horrible fingers pull off your cloak!");
						otmp = uarmc;
						if (donning(otmp)) cancel_don();
						(void)Cloak_off();
						freeinv(otmp);
						(void)dropy(otmp);
						if (roll_madness(MAD_TALONS)){
							You("panic after having your cloak taken!");
							HPanicking += 1+rnd(6);
						}
					}
				}
				else if (uwep && uwep->oartifact == ART_TENSA_ZANGETSU){
					You_feel("horrible fingers tug at your shihakusho.");
				}
				else{
					if (uswapwep && !u.twoweap && uswapwep != uball){
						if (d(1, 100) - ACURR(A_DEX) > 0){
							You_feel("your %s being stealthily drawn out of your belt.", doname(uswapwep));
							otmp = uswapwep;
							setuswapwep((struct obj *)0);
							freeinv(otmp);
							(void)mpickobj(magr, otmp);
							if (roll_madness(MAD_TALONS)){
								You("panic after having your property stolen!");
								HPanicking += 1+rnd(6);
							}
						}
					}
					else if (uarm){
						You_feel("horrible fingers work at your armor.");
						if (d(1, 100) - ACURR(A_DEX) > 0 && d(1, 4) == 4){
							pline("The horrible fingers unfasten your armor!");
							otmp = uarm;
							if (donning(otmp)) cancel_don();
							(void)Armor_gone();
							freeinv(otmp);
							(void)dropy(otmp);
							if (roll_madness(MAD_TALONS)){
								You("panic after having your armor taken!");
								HPanicking += 1+rnd(6);
							}
						}
					}
				}
			}
			/* vs monsters, only tickles them mercilessly */
			else {
				if (mdef->mcanmove) {
					if (vis) {
						pline("%s mercilessly %s %s.",
							(youagr ? "You" : Monnam(magr)),
							(youagr ? "tickle" : "tickles"),
							(youdef ? "you" : mon_nam(mdef))
							);
					}
					mdef->mcanmove = 0;
					mdef->mfrozen = rnd(10);
					mdef->mstrategy &= ~STRAT_WAITFORU;
				}
			}
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_SUCK:
		/* does nothing at all to noncorporeal or amoprhous creatures */
		if (noncorporeal(pd) || amorphous(pd))
		{
			/* no hitmessage, either */
			return MM_MISS;
		}
		else {
			/* chance for vorpal-esque head removal */
			otmp = (youdef ? uarmh : which_armor(mdef, W_ARMH));
			if (has_head(pd)
				&& !otmp
				&& !rn2(20)
				&& (*hp(mdef) < 100)
				){
				/* message */
				if (vis) {
					pline("%s suck%s %s %s off!",
						(youagr ? "You" : Monnam(magr)),
						(youagr ? "" : "s"),
						(youdef ? "your" : s_suffix(mon_nam(mdef))),
						(youdef ? body_part(HEAD) : mbodypart(mdef, HEAD))
						);
				}
				/* kill */
				killer = "forcible head removal";
				killer_format = KILLED_BY;
				return xdamagey(magr, mdef, attk, FATAL_DAMAGE_MODIFIER);
			}
			/* most commonly, this path is taken */
			else {
				/* special hit message */
				if (youdef && dohitmsg) {
					You_feel("%s trying to suck your extremities off!",
						mon_nam(magr));
				}
				else if (vis && dohitmsg) {
					if(youagr) pline("You try to suck %s extremities off!", s_suffix(mon_nam(mdef)));
					else pline("%s is trying to suck %s extremities off!", Monnam(magr), s_suffix(mon_nam(mdef)));
				}
				/* 1/10 chance to twist legs */
				if (!rn2(10)) {
					if(youdef){
						Your("%s twist from the suction!", makeplural(body_part(LEG)));
						set_wounded_legs(RIGHT_SIDE, rnd(60 - ACURR(A_DEX)));
						set_wounded_legs(LEFT_SIDE, rnd(60 - ACURR(A_DEX)));
						exercise(A_STR, FALSE);
						exercise(A_DEX, FALSE);
					}
					else {
						if(vis) pline("%s %s twist from the suction!", s_suffix(Monnam(mdef)), makeplural(mbodypart(mdef, LEG)));
						mdef->movement = max(mdef->movement - 12, -12);
					}
				}
				/* 1/6 chance to disarm */
				otmp = (youdef ? uwep : MON_WEP(mdef));
				if (otmp && !rn2(6)){
					/* message for players */
					if (youdef) {
						You_feel("%s pull on your weapon!", mon_nam(magr));
					}
					/* 1d130 > STR */
					if (d(1, 50) > (youdef ? ACURR(A_STR) : ACURR_MON(A_STR, mdef))){
						if (youdef) {
							if(uwep == uball){
								pline("Fortunately, you are chained to your weapon!");
							}
							else {
								Your("weapon is sucked out of your grasp!");
								uwepgone();
								freeinv(otmp);
								(void)mpickobj(magr, otmp);
							}
						}
						else {
							if (vis) {
								pline("%s weapon is sucked out of %s grasp!",
									s_suffix(Monnam(mdef)),
									hisherits(mdef)
									);
							}
							obj_extract_self(otmp);
							possibly_unwield(mdef, FALSE);
							setmnotwielded(mdef, otmp);
							if (youagr) {
								(void)hold_another_object(otmp, "You drop %s!", doname(otmp), (const char *)0);
							}
							else {
								(void)mpickobj(magr, otmp);
							}
						}
					}
					else if (youdef) {
						You("keep a tight grip on your weapon!");
					}
				}
				/* 1/10 chance to suck off boots */
				otmp = (youdef ? uarmf : which_armor(mdef, W_ARMF));
				if (otmp
					&& otmp->otyp != find_bboots()
					&& !rn2(10)
					) {
					if (youdef) {
						Your("boots are sucked off!");
						if (donning(otmp)) cancel_don();
						(void)Boots_off();
						freeinv(otmp);
						(void)mpickobj(magr, otmp);
					}
					else {
						if (vis) {
							pline("%s boots are sucked off!",
								s_suffix(Monnam(mdef)));
						}
						obj_extract_self(otmp);
						otmp->owornmask = 0L;
						update_mon_intrinsics(mdef, otmp, FALSE, FALSE);
						if (youagr) {
							(void)hold_another_object(otmp, "You drop %s!", doname(otmp), (const char *)0);
						}
						else {
							(void)mpickobj(magr, otmp);
						}
					}
				}
				/* 1/6 chance to suck off gloves */
				otmp = (youdef ? uarmg : which_armor(mdef, W_ARMG));
				if (otmp
					&& !rn2(6)
					) {
					/* message for players */
					if (youdef) {
						You_feel("%s pull on your gloves!", mon_nam(magr));
					}
					/* 1d40 > STR */
					if (d(1, 30) > (youdef ? ACURR(A_STR) : ACURR_MON(A_STR, mdef))){
						if (youdef) {
							Your("gloves are sucked off!");
							if (donning(otmp)) cancel_don();
							(void)Gloves_off();
							freeinv(otmp);
							(void)mpickobj(magr, otmp);
						}
						else {
							if (vis) {
								pline("%s gloves are sucked off!",
									s_suffix(Monnam(mdef)));
							}
							obj_extract_self(otmp);
							otmp->owornmask = 0L;
							update_mon_intrinsics(mdef, otmp, FALSE, FALSE);
							if (youagr) {
								(void)hold_another_object(otmp, "You drop %s!", doname(otmp), (const char *)0);
							}
							else {
								(void)mpickobj(magr, otmp);
							}
						}

					}
					else if (youdef) {
						You("keep your %s closed.", makeplural(body_part(HAND)));
					}
				}
				/* 1/8 chance of sucking off shields */
				otmp = (youdef ? uarms : which_armor(mdef, W_ARMS));
				if (otmp
					&& !rn2(8)
					) {
					/* message for players */
					if (youdef) {
						You_feel("%s pull on your shield!", mon_nam(magr));
					}
					/* 1d150 > STR */
					if (d(1, 80) > (youdef ? ACURR(A_STR) : ACURR_MON(A_STR, mdef))){
						if (youdef) {
							Your("shield is sucked out of your grasp!");
							if (donning(otmp)) cancel_don();
							(void)Shield_off();
							freeinv(otmp);
							(void)mpickobj(magr, otmp);
						}
						else {
							if (vis) {
								pline("%s shield is sucked out of %s grasp!",
									s_suffix(Monnam(mdef)),
									hisherits(mdef)
									);
							}
							obj_extract_self(otmp);
							otmp->owornmask = 0L;
							update_mon_intrinsics(mdef, otmp, FALSE, FALSE);
							if (youagr) {
								(void)hold_another_object(otmp, "You drop %s!", doname(otmp), (const char *)0);
							}
							else {
								(void)mpickobj(magr, otmp);
							}
						}

					}
					else if (youdef) {
						You("keep a tight grip on your shield!");
					}
				}
				/* 1/4 chance to suck off helmets */
				otmp = (youdef ? uarmh : which_armor(mdef, W_ARMH));
				if (otmp
					&& !rn2(4)
					) {
					if (youdef) {
						Your("helmet is sucked off!");
						if (donning(otmp)) cancel_don();
						(void)Helmet_off();
						freeinv(otmp);
						(void)mpickobj(magr, otmp);
					}
					else {
						if (vis) {
							pline("%s helmet is sucked off!",
								s_suffix(Monnam(mdef)));
						}
						obj_extract_self(otmp);
						otmp->owornmask = 0L;
						update_mon_intrinsics(mdef, otmp, FALSE, FALSE);
						if (youagr) {
							(void)hold_another_object(otmp, "You drop %s!", doname(otmp), (const char *)0);
						}
						else {
							(void)mpickobj(magr, otmp);
						}
					}
				}
			}
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_UVUU:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* chance for vorpal-esque headsmashing */
		if (!rn2(20)){
			/* lack of head is only a minor issue */
			if (!has_head_mon(mdef)) {
				/* 2x damage */
				dmg *= 2;
			}
			/* so is having a pass-through head */
			else if (noncorporeal(pd) || amorphous(pd)) {
				/* message */
				if (vis) {
					pline("%s %s through %s %s.",
						(youagr ? "You" : Monnam(magr)),
						(youagr ? "pass" : "passes"),
						(youdef ? "your" : hisherits(mdef)),
						(youdef ? body_part(HEAD) : mbodypart(mdef, HEAD))
						);
				}
				/* 2x damage */
				dmg *= 2;
			}
			/* smash! */
			else {
				otmp = (youdef ? uarmh : which_armor(mdef, W_ARMH));

				/* no helmet... SMASH! */
				if (!otmp) {
					/* message */
					if (vis) {
						pline("%s %s %s %s!",
							(youagr ? "You" : Monnam(magr)),
							(youagr ? "smash" : "smashes"),
							(youdef ? "your" : s_suffix(mon_nam(mdef))),
							(youdef ? body_part(HEAD) : mbodypart(mdef, HEAD))
							);
					}
					/* kill */
					Sprintf(buf, "%s headspike", s_suffix(mon_nam(magr)));
					killer = buf;
					killer_format = KILLED_BY_AN;
					return xdamagey(magr, mdef, attk, FATAL_DAMAGE_MODIFIER);
				}
				else {
					/* helmet protected */
					/* this doesn't destroy the helmet? */
					if (vis) {
						pline("%s headspike hits %s %s!",
							(youagr ? "Your" : s_suffix(Monnam(magr))),
							(youdef ? "your" : s_suffix(mon_nam(mdef))),
							xname(otmp)
							);
					}
					/* no bonus effects */
				}
			}
		}
		/* con-draining (player only?) */
		if (youdef) {
			Sprintf(buf, "%s %s",
				s_suffix(Monnam(magr)), mpoisons_subj(magr, attk));
			poisoned(buf, A_CON, pa->mname, 60, TRUE);
		}
		/* wis-draining (player only) */
		if (youdef) {
			int wisdmg = (dmg / 6) + 1;

			if (Poison_resistance)
				wisdmg -= ACURR(A_CON) / 2;

			if (wisdmg > 0){
				if (u.sealsActive&SEAL_HUGINN_MUNINN){
					unbind(SEAL_HUGINN_MUNINN, TRUE);
				}
				else {
					while (ABASE(A_WIS) > ATTRMIN(A_WIS) && wisdmg > 0){
						wisdmg--;
						(void)adjattrib(A_WIS, -1, TRUE);
						forget(4);
						exercise(A_WIS, FALSE);
					}
					if (AMAX(A_WIS) > ATTRMIN(A_WIS) &&
						ABASE(A_WIS) < AMAX(A_WIS) / 2) AMAX(A_WIS) -= 1; //permanently drain wisdom
					if (wisdmg){
						boolean chg;
						chg = make_hallucinated(HHallucination + (long)(wisdmg * 5), FALSE, 0L);
					}
				}
			}
		}
		/* energy draining (player only) */
		if (youdef) {
			drain_en((int)(dmg / 2));
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_TENT:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* the tentacle attack is only implemented vs the player */
		if (youdef) {
			u.ustuck = magr; //can steal stickage from other monsters.
#ifdef SEDUCE
			dotent(magr, dmg);
#endif
			change_usanity(u_sanity_loss_minor(magr), TRUE);
		}
		/* Might be technically incorrect to make the player also take an AT_TENT AD_PHYS attack afterwards
		 * but it really simplifies the flow to use the standard behaviour of [special effects] -> [basic damage]
		 */
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_TSMI:{
		struct obj *inventory = youdef ? invent : mdef->minvent;
		if(!inventory)
			return MM_MISS;
		//Note: with no inventory return silently.
		if (vis && dohitmsg) {
			if(youdef){
				if(canseemon(magr)){
					pline("A fan of strange tentacles unfolds from %s %s!", !youagr ? mon_nam(magr) : "your", !youagr ? mbodypart(magr, HEAD) : body_part(HEAD));
					pline("The tentacles investigate your belongings!");
				}
				else pline("Strange tentacles investigate your belongings!");
			}
			else if(youagr){
				pline("A fan of strange tentacles unfolds from your %s!", body_part(HEAD));
				pline("Your tentacles investigate %s belongings!", s_suffix(mon_nam(mdef)));
			}
			else {
				if(canseemon(magr)){
					pline("A fan of strange tentacles unfolds from %s %s!", mon_nam(magr), mbodypart(magr, HEAD));
					pline("The tentacles investigate %s belongings!", s_suffix(mon_nam(mdef)));
				}
			}
		}
		dotsmi_theft(magr, mdef, inventory, (struct obj *)0);
		
		return MM_MISS;
	}
	case AD_WEBS:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		if (!t_at(x(mdef), y(mdef))) {
			struct trap *ttmp2 = maketrap(x(mdef), y(mdef), WEB);
			if (ttmp2) {
				if (youdef) {
					pline_The("webbing sticks to you. You're caught!");
					dotrap(ttmp2, NOWEBMSG);
#ifdef STEED
					if (u.usteed && u.utrap) {
						/* you, not steed, are trapped */
						dismount_steed(DISMOUNT_FELL);
					}
#endif
				}
				else {
					mintrap(mdef);
				}
			}
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_HOOK:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		if (!t_at(x(mdef), y(mdef)) 
			&& !(amorphous(pd) || is_whirly(pd) || unsolid(pd))
			&& levl[x(magr)][y(magr)].typ != AIR
		) {
			struct trap *ttmp2 = maketrap(x(mdef), y(mdef), FLESH_HOOK);
			if (ttmp2) {
				if (youdef) {
					pline_The("hook-tip breaks off and attaches to the %s!", surface(x(mdef), y(mdef)));
					dotrap(ttmp2, 0);
#ifdef STEED
					if (u.usteed && u.utrap) {
						/* you, not steed, are trapped */
						dismount_steed(DISMOUNT_FELL);
					}
#endif
				}
				else {
					mintrap(mdef);
				}
			}
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_PULL:
	case AD_PUSH:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		int dx = x(magr) - x(mdef);
		int dy = y(magr) - y(mdef);
		int dist = 1;
		
		if(attk->adtyp == AD_PUSH){
			dx *= -1;
			dy *= -1;
			dist = rnd(3);
		}
		
		if(youdef)
			hurtle(sgn(dx), sgn(dy), dist, FALSE, FALSE);
		else {
			mhurtle(mdef, sgn(dx), sgn(dy), dist, FALSE);
			if (DEADMONSTER(mdef))
				return MM_DEF_DIED;
			if(MIGRATINGMONSTER(mdef))
				return MM_AGR_STOP;
		}
		
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_LICK:{
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		switch(rnd(4)){
			case 1:
				if(distmin(x(magr), y(magr), x(mdef), y(mdef)) > 1){
					alt_attk.adtyp = AD_PULL;
					return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);
				}
				//else fall through to cold
			case 2:
				alt_attk.adtyp = AD_ECLD;
				alt_attk.damn = 10;
				alt_attk.damd = 8;
				return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);
			case 3:
				alt_attk.adtyp = AD_ACID;
				alt_attk.damn = 10;
				alt_attk.damd = 8;
				return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);
			case 4:
				// continue
			break;
		}
		if(youdef){
			You("are glued to %s by %s saliva!", the(surface(x(mdef), y(mdef))), s_suffix(mon_nam(magr)));
			u.utrap += rnd(12);
			u.utraptype = TT_SALIVA;
		}
		else {
			//Monsters are less detailed
			pline("%s is glued to %s by %s saliva!", Monnam(mdef), the(surface(x(mdef), y(mdef))), s_suffix(mon_nam(magr)));
			mdef->mcanmove = FALSE;
			mdef->mfrozen = max(mdef->mfrozen, d(2,4)); //max value 2^7-1, 8 is well shy of that.
		}

		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);
	}
	case AD_BYAK:{
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		
		struct obj *otmp = some_armor(mdef);
		if(otmp){
			add_byakhee_to_obj(otmp);
		}
		else {
			if(youagr ? flags.female : magr->female){
				otmp = mksobj_at(EGG, x(mdef), y(mdef), MKOBJ_NOINIT);
				if(otmp){
					otmp->spe = youagr; //yours?
					otmp->quan = 1;
					otmp->owt = weight(otmp);
					otmp->corpsenm = PM_BYAKHEE;
					attach_egg_hatch_timeout(otmp);
				}
			}
			else {
				struct monst *larva = makemon(&mons[PM_STRANGE_LARVA], x(mdef), y(mdef), MM_ADJACENTOK|NO_MINVENT|MM_NOCOUNTBIRTH|(youagr ? MM_EDOG : 0));
				if(larva){
					larva->mvar_tanninType = PM_BYAKHEE;
					if(youagr){
						initedog(larva);
					}
					else if(magr->mpeaceful){
						larva->mpeaceful = TRUE;
						set_faction(larva, magr->mfaction);
					}
					else {
						larva->mpeaceful = FALSE;
						set_faction(larva, magr->mfaction);
					}
					newsym(larva->mx, larva->my);
					set_malign(larva);
				}
			}
		}

		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);
	}
	case AD_STDY:
		/* study before doing the attack */
		if (notmcan) {
			int * stdy = (youdef ? &(u.ustdy) : &(mdef->mstdy));
			/* only print message or increase study if it is an increase */
			if (dmg > *stdy){
				/* message */
				if (is_orc(pa)) {
					pline("%s %s and urges %s followers on.",
						(youagr ? "You" : Monnam(magr)),
						(youagr ? "curse" : "curses"),
						(youagr ? "your" : mhis(magr)));
				}
				else if (pa->mtyp == PM_LEGION || pa->mtyp == PM_LEGIONNAIRE); //no message
				else if (youagr || canseemon(magr)) {
					pline("%s %s %s with a level stare.",
						(youagr ? "You" : Monnam(magr)),
						(youagr ? "study" : "studies"),
						(youdef ? "you" : mon_nam(mdef))
						);
				}
				/* increase study */
				(*stdy) = dmg;
			}
		}
		/* make physical attack */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);


//////////////////////////////////////////////////////////////
// SEDUCTION / ITEM-STEALING
//////////////////////////////////////////////////////////////
		/* except when aimed at the player, these 
		 * are identical */
	case AD_SITM:
	case AD_SEDU:
#ifdef SEDUCE
	case AD_SSEX:
#endif
	{
		boolean goatspawn = (pa->mtyp == PM_SMALL_GOAT_SPAWN || pa->mtyp == PM_GOAT_SPAWN || pa->mtyp == PM_GIANT_GOAT_SPAWN || pa->mtyp == PM_BLESSED || pa->mtyp == PM_XUENU_MONK);
		boolean noflee = (magr->isshk && magr->mpeaceful);
		if(attk->damn > 0 || attk->damd > 0){
			/* make physical attack */
			alt_attk.adtyp = AD_PHYS;
			result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		}
		else if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* return early if cannot continue the attack */
		if (result&(MM_DEF_DIED|MM_DEF_LSVD))
			return result;

		if (youdef) {
			static int engagering1 = 0;
			static int engagering4 = 0;
			boolean engring = FALSE;

			/* spaghetti code alert: many paths of code in here return early */
			switch (attk->adtyp) {
			case AD_SITM:
				if (!(u.sealsActive&SEAL_ANDROMALIUS)
					&& !check_mutation(TENDRIL_HAIR)
					&& notmcan
				){
					switch (steal(magr, buf, FALSE, TRUE))
					{
					case -1:
						return MM_AGR_DIED;
					case 0:
						m_dowear(magr, FALSE);
						return MM_HIT;
					default:
						if (!noflee) {
							if (*buf) {
								if (canseemon(magr))
									pline("%s tries to %s away with %s.",
									Monnam(magr),
									locomotion(magr, "run"),
									buf);
							}
							monflee(magr, 0, FALSE, FALSE);
						}
						if(magr->mtyp == PM_ETHEREAL_FILCHER)
							return MM_HIT;
						else
							return MM_AGR_STOP;
					}
				}
				break;

			case AD_SEDU:
				if ((uleft  && uleft->otyp == find_engagement_ring()) ||
					(uright && uright->otyp == find_engagement_ring()))
					engring = TRUE;
				if (u.sealsActive&SEAL_ANDROMALIUS) break;
				//pline("test string!");
				if (pa->mtyp == PM_DEMOGORGON){
					buf[0] = '\0';
					steal(magr, buf, FALSE, FALSE);
					m_dowear(magr, FALSE);
					return MM_HIT;
				}
				if ((pa->mtyp == PM_FIERNA || pa->mtyp == PM_PALE_NIGHT) && rnd(20) < 15) return MM_HIT;
				if (((MON_WEP(magr)) && pa->mtyp == PM_ALRUNES) && !rn2(20)) return MM_HIT;
				if ((pa->mflagsa&pd->mflagsa) != 0 && (dmgtype(youracedata, AD_SEDU)
					|| dmgtype(youracedata, AD_SSEX) || dmgtype(youracedata, AD_LSEX)
					)) {
					pline("%s %s.", Monnam(magr), magr->minvent ?
						"brags about the goods some dungeon explorer provided" :
						"makes some remarks about how difficult theft is lately");
					if (!tele_restrict(magr)) (void)rloc(magr, TRUE);
					magr->mpeaceful = TRUE;
					newsym(magr->mx, magr->my);
					set_malign(magr);
					return MM_AGR_STOP;
				}
				else if (magr->mcan || engring || Chastity) {
					if(magr->mappearance) seemimic_ambush(magr);
					if (!Blind) {
						pline("%s tries to %s you, but you seem %s.",
							Adjmonnam(magr, "plain"),
							(is_neuter(magr->data) || flags.female == magr->female) ? "charm" : "seduce",
							(is_neuter(magr->data) || flags.female == magr->female) ? "unaffected" : "uninterested");
					}
					if (rn2(3)) {
						if (!noflee && !tele_restrict(magr)) (void)rloc(magr, TRUE);
						return MM_AGR_STOP;
					}
					break;
				}
				buf[0] = '\0';
				if(magr->mappearance) seemimic_ambush(magr);
				switch (steal(magr, buf, FALSE, FALSE)) {
				case -1:
					return MM_AGR_DIED;
				case 0:
					m_dowear(magr, FALSE);
					return MM_HIT;
				default:
					if(goatspawn){
						return MM_AGR_STOP;
					} else {
						if (!noflee) {
							if (!tele_restrict(magr))
								(void)rloc(magr, TRUE);
							monflee(magr, 0, FALSE, FALSE);
						}
						return MM_AGR_STOP;
					}
				}
				break;

#ifdef SEDUCE
			case AD_SSEX:
				/* see seduce.c */
				if(Chastity)
					break;

				if ((uleft  && uleft->otyp == find_engagement_ring()) ||
					(uright && uright->otyp == find_engagement_ring()))
					break;

				if (notmcan && could_seduce(magr, &youmonst, attk) == 1) {
					if(magr->mappearance) seemimic_ambush(magr);
					if (doseduce(magr))
						return MM_AGR_STOP;
				}
				break;
#endif /* SEDUCE */
			}
		}
		/* the player just steals an item from a monster */
		else if (youagr) {
			steal_it(mdef, attk);
		}
		/* monsters just steal items from each other */
		else if(notmcan){
			if(msteal_m(magr, mdef, attk, &result))
				return result;
		}
		return result;
	} //end of seduction block
		/* steal gold is... similar in that it steals items? */
	case AD_SGLD:
		/* make physical attack */
		alt_attk.adtyp = AD_PHYS;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		/* return early if cannot continue the attack */
		if (result&(MM_DEF_DIED|MM_DEF_LSVD))
			return result;

		/* youdef vs youagr vs mvm are all separate */
		if(youdef){
			if(notmcan
				&& !(pd->mlet == pa->mlet)
				&& !(u.sealsActive&SEAL_ANDROMALIUS)
				&& !check_mutation(TENDRIL_HAIR)
			){
				stealgold(magr);
			}
		}
		else if (youagr) {
#ifndef GOLDOBJ
			if (mdef->mgold) {
				u.ugold += mdef->mgold;
				mdef->mgold = 0;
				Your("purse feels heavier.");
				IMPURITY_UP(u.uimp_theft)
			}
#else
			/* This you as a leprechaun, so steal
			real gold only, no lesser coins */
			{
				struct obj *mongold = findgold(mdef->minvent);
				if (mongold) {
					obj_extract_self(mongold);
					if (merge_choice(invent, mongold) || inv_cnt() < 52) {
						addinv(mongold);
						Your("purse feels heavier.");
						IMPURITY_UP(u.uimp_theft)
					}
					else {
						You("grab %s's gold, but find no room in your knapsack.", mon_nam(mdef));
						dropy(mongold);
					}
				}
			}
#endif
			exercise(A_DEX, TRUE);
		}
		else {
			if (notmcan) {
#ifndef GOLDOBJ
				if (mdef->mgold) {
					/* technically incorrect; no check for stealing gold from
					 * between mdef's feet...
					 */
					magr->mgold += mdef->mgold;
					mdef->mgold = 0;
#else
					/* technically incorrect; no check for stealing gold from
					* between mdef's feet...
					*/
					struct obj *gold = findgold(mdef->minvent);
					if (gold) {
						obj_extract_self(gold);
						add_to_minv(magr, gold);
					}
#endif
					mdef->mstrategy &= ~STRAT_WAITFORU;
					if (vis) {
						Strcpy(buf, Monnam(magr));
						pline("%s steals some gold from %s.", buf, mon_nam(mdef));
					}
					if (!tele_restrict(magr)) {
						(void)rloc(magr, TRUE);
						result |= MM_AGR_STOP;
						if (vis && !canspotmon(magr))
							pline("%s suddenly disappears!", Monnam(magr));
					}
				}
			}
		}
		return result;

	case AD_SAMU:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* only special vs the player */
		if (youdef) {
			/* when the Wiz hits, 1/20 steals the amulet */
			if (u.uhave.amulet ||
				u.uhave.bell || u.uhave.book || u.uhave.menorah
				) /* carrying the Quest Artifact */
			if (!rn2(20)) stealamulet(magr);
		}
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_SQUE:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* only special vs the player */
		if (youdef) {
			/* when the Nemeses hits, 1/10 steals the something special */
			if (u.uhave.amulet ||
				u.uhave.bell || u.uhave.book || u.uhave.menorah
				|| u.uhave.questart) /* carrying the Quest Artifact */
			if (!rn2(10)) stealquestart(magr);
		}
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_STTP:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}

		otmp = some_armor(mdef);
		if (otmp)
			teleport_arm(otmp, mdef);

		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

//////////////////////////////////////////////////////////////
// TELEPORTING ATTACKS
//////////////////////////////////////////////////////////////
		/* Deal physical damage first.
		 * The defender needs to be alive to be teleported.
		 */

	case AD_TLPT:
	case AD_ABDC:
		/* make physical attack */
		alt_attk.adtyp = AD_PHYS;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		/* return early if cannot continue the attack */
		if (result&(MM_DEF_DIED|MM_DEF_LSVD))
			return result;

		/* mhitu */
		if (youdef) {
			/* sadly needs a hack to determine if the player was teleported to a new location,
			 * which would interrupt the attacker's attack chain */
			int oldux = u.ux, olduy = u.uy;
			switch (attk->adtyp)
			{
			case AD_TLPT:
				if (uncancelled) {
					if (flags.verbose)
						Your("position suddenly seems very uncertain!");
					tele();
				}
				break;
			case AD_ABDC:
				if (armuncancel 
					|| (pa->mtyp == PM_PALE_NIGHT && !rn2(3))
					|| (pa->mtyp == PM_BLASPHEMOUS_HAND && !rn2(3))
				) {
					if(pa->mtyp == PM_BLASPHEMOUS_HAND && !In_endgame(&u.uz) && !on_level(&bridge_temple, &u.uz)){
						d_level newlev;
						newlev.dnum = rlyeh_dnum;
						newlev.dlevel = bridge_temple.dlevel;
						
						pline("The hand grabs you!");
						schedule_goto(&newlev, FALSE, FALSE, FALSE,
								  Blind ? "You feel weightless for a moment." : "The hand drags you into a starry void!",
								  "Awful tentacles suck at you as you fall!", d(8,8), u_sanity_loss(magr));
						TRANSCENDENCE_IMPURITY_UP(FALSE)
					}
					else
						(void)safe_teleds(FALSE);
				}
				break;
			}
			/* Were you moved? If so, take note. */
			if (oldux != u.ux || olduy != u.uy)
				result |= MM_AGR_STOP;	/* defender moved */
		}
		/* uhitm */
		else if (youagr) {
			if (uncancelled) {
				char nambuf[BUFSZ];
				boolean u_saw_mon = canseemon(mdef) ||
					(u.uswallow && u.ustuck == mdef);
				/* record the name before losing sight of monster */
				Strcpy(nambuf, Monnam(mdef));
				if (u_teleport_mon(mdef, FALSE)) {
					result |= MM_AGR_STOP;	/* defender moved */
					if (u_saw_mon && !canseemon(mdef)) {
						pline("%s suddenly disappears!", nambuf);
					}
				}
			}
		}
		/* mhitm */
		else {
			if (uncancelled && !tele_restrict(mdef)) {
				char mdef_Monnam[BUFSZ];
				/* save the name before monster teleports, otherwise
				we'll get "it" in the suddenly disappears message */
				if (vis) Strcpy(mdef_Monnam, Monnam(mdef));
				mdef->mstrategy &= ~STRAT_WAITFORU;
				(void)rloc(mdef, TRUE);
				result |= MM_AGR_STOP;	/* defender moved */
				if (vis && !canspotmon(mdef)
#ifdef STEED
					&& mdef != u.usteed
#endif
					)
					pline("%s suddenly disappears!", mdef_Monnam);
			}
		}
		return result;

	case AD_LVLT:	/* only perform the levelport, no draining or other weeping effects */
	case AD_WEEP:	/* significantly different when targeting player */
		/* make physical attack */
		alt_attk.adtyp = AD_PHYS;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		/* return early if cannot continue the attack */
		if (result&(MM_DEF_DIED|MM_DEF_LSVD))
			return result;

		/* only works when uncancelled */
		if (uncancelled) {
			/* mhitu, AD_WEEP is different from AD_LVLT */
			if (youdef) {
				switch (attk->adtyp)
				{
				case AD_LVLT:
					level_tele();
					break;
				case AD_WEEP:
					/* before endgame, tries to levelport you to drain your potential */
					if (!u.uevent.invoked){
						/* tport control is as good protection as drain res here, it needs to teleport you to drain you. */
						if (Teleport_control || Drain_resistance) {
							if (flags.verbose) You("feel like you could have lost some potential.");
						}
						/* levelported and drained */
						else {
							int potentialLost = 0;
							level_tele();
							You("suddenly feel like you've lost some potential.");
							potentialLost = min(abs(u.uz.dlevel - u.utolev.dlevel), u.ulevel - 1) / 2 + 1;
							for (; potentialLost>0; potentialLost--) losexp("loss of potential", FALSE, TRUE, TRUE); /*not verbose, force drain, drain exp also*/
							dmg = 0;
							result |= MM_AGR_STOP; /*You teleported, monster should stop attacking.*/
						}
					}
					/* during endgame, just drains levels (since levelporting is a no-go?) */
					else{
						if (Drain_resistance) {
							if (flags.verbose) You("feel like you could have lost some potential.");
						}
						else {
							You("suddenly feel like you've lost some potential.");
							losexp("loss of potential", FALSE, TRUE, TRUE); /*not verbose, force drain, drain exp also*/
						}
					}
					break;
				}//switch(adtyp)
			}
			/* uhitm and mhitm are both almost identically AD_LVLT */
			else {
				/* grab the name of mdef while it's visible */
				char mdef_Monnam[BUFSZ];
				if (vis)
					Strcpy(mdef_Monnam, Monnam(mdef));
				mdef->mstrategy &= ~STRAT_WAITFORU;
				/* Once the player performs the Invocation, weeping angels (that aren't you) will
				 * be too interested in your potential to feed off the potential of monsters */
				if (!youagr && u.uevent.invoked && attk->adtyp == AD_WEEP) {
					if (vis && canspotmon(magr) && flags.verbose)
						pline("%s is glancing at you with a hungry stare.", Monnam(magr));
				}
				/* levelport target */
				else {
#ifdef STEED
					if (u.usteed == mdef) {
						pline("%s vanishes from underneath you.", Monnam(mdef));
						dismount_steed(DISMOUNT_VANISHED);
					}
					else {
#endif
						if (vis && canspotmon(mdef) && flags.verbose)
							pline("%s vanishes before your eyes.", Monnam(mdef));
#ifdef STEED
					}
#endif
					int nlev;
					d_level flev;
					nlev = random_teleport_level();
					get_level(&flev, nlev);
					migrate_to_level(mdef, ledger_no(&flev), MIGR_RANDOM,
						(coord *)0);
					result |= MM_AGR_STOP;	/* defender gone */
				}
			}
		}
		return result;
//////////////////////////////////////////////////////////////
// MISC
//////////////////////////////////////////////////////////////
	case AD_LEGS:
		/* this really only makes sense vs the player, sadly */
		/* if not attacking player, try to make a simple physical attack instead */
		if (!youdef) {
			/* if cancelled, deal no damage */
			if (!notmcan) {
				alt_attk.damn = 0;
				alt_attk.damd = 0;
			}
			else {
				mdef->mwounded_legs = TRUE;
			}
			alt_attk.adtyp = AD_PHYS;

			/* do this new attack */
			return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		}
		else {
			long side = rn2(2) ? RIGHT_SIDE : LEFT_SIDE;
			const char *sidestr = (side == RIGHT_SIDE) ? "right" : "left";

			/* This case is too obvious to ignore, but Nethack is not in
			* general very good at considering height--most short monsters
			* still _can_ attack you when you're flying or mounted.
			* [FIXME: why can't a flying attacker overcome this?]
			*/
			if (
#ifdef STEED
				u.usteed ||
#endif
				Levitation || Flying) {
				pline("%s tries to reach your %s %s!", Monnam(magr),
					sidestr, body_part(LEG));
				return MM_MISS;
			}
			else if (magr->mcan) {
				pline("%s nuzzles against your %s %s!", Monnam(magr),
					sidestr, body_part(LEG));
				return MM_MISS;
			}
			else {
				if (uarmf) {
					if (rn2(2) && (uarmf->otyp == LOW_BOOTS ||
						uarmf->otyp == SHOES))
						pline("%s pricks the exposed part of your %s %s!",
						Monnam(magr), sidestr, body_part(LEG));
					else if (uarmf->otyp != find_jboots() && !rn2(5))
						pline("%s pricks through your %s boot!",
						Monnam(magr), sidestr);
					else {
						pline("%s scratches your %s boot!", Monnam(magr),
							sidestr);
						alt_attk.damn = 0;
						alt_attk.damd = 0;
					}
				}
				else pline("%s pricks your %s %s!", Monnam(magr),
					sidestr, body_part(LEG));
				set_wounded_legs(side, rnd(60 - ACURR(A_DEX)));
				exercise(A_STR, FALSE);
				exercise(A_DEX, FALSE);
			}
		}

		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_DISN:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* maybe print glowy message */
		if (!Blind && (youdef || canseemon(mdef))){
			const char * glow = ((pa->mtyp == PM_SWORD_ARCHON || pa->mtyp == PM_BAEL) ? "faintly blue"
				: (pa->mtyp == PM_FLAXEN_STARSHADOW || pa->mtyp == PM_FLAXEN_STAR_PHANTOM) ? "bilious yellow" 
				: (pa->mtyp == PM_SILVERFIRE_SHADOW_S_WRAITH) ? "shadowy silver" 
				: (pa->mtyp == PM_CHORISTER_JELLY) ? "inky shadows" 
				: "sickly green");
			if (youdef)
				You("%s %s!", (pa->mtyp == PM_CHORISTER_JELLY) ? "radiate" : "glow", glow);
			else
				pline("%s %s %s!", Monnam(mdef), (pa->mtyp == PM_CHORISTER_JELLY) ? "radiates" : "glows", glow);
		}
		/* disintegrate! */
		for (; dmg > 0; dmg--)
		{
			/* grab a random piece of equipped armor */
			otmp = some_armor(mdef);

			if (otmp){
				if (obj_resists(otmp, 0, 80)){
					break; //End loop
				}
				else {
					if (oresist_disintegration(otmp))
						continue;
					if (otmp->spe > -1 * a_acdr(objects[otmp->otyp])){
						damage_item(otmp);
					}
					else if (!otmp->oartifact){
						destroy_marm(mdef, otmp);
					}
				}
			}
			else {
				if (!Disint_res(mdef)){
					/* you are disintegrated */
					if (youdef) {
						You("disintegrate!");
						killer_format = KILLED_BY;
						Sprintf(killer_buf, "disintegrated by %s", 
							type_is_pname(pa) ? pa->mname : an(pa->mname));
						killer = killer_buf;
						/* when killed by disintegration, don't leave corpse */
						u.ugrave_arise = NON_PM;
						if (!u.uconduct.killer && !youagr){
							//Pcifist PCs aren't combatants so if something kills them up "killed peaceful" type impurities
							IMPURITY_UP(u.uimp_murder)
							IMPURITY_UP(u.uimp_bloodlust)
						}
						done(DISINTEGRATED);
						You("reintegrate!");//lifesaved
						return (MM_HIT|MM_DEF_LSVD);
					}
					/* monster is disintegrated */
					else {
						if(vis) pline("%s disintegrates!", Monnam(mdef));
						if (youagr)
							xkilled(mdef, 2);
						else
							monkilled(mdef, "", AD_DISN);
						if (mdef->mhp > 0)
							return (MM_HIT|MM_DEF_LSVD);	/* lifesaved */
						else
							return (MM_HIT | MM_DEF_DIED | ((youagr || grow_up(magr, mdef)) ? 0 : MM_AGR_DIED));	/* grow_up might kill magr */
					}
				}
			}
		}
		return MM_HIT;

	case AD_DRIN:
		/* Special case for Migo.
		 * Migo only scoop out brains some of the time (1/20)
		 * Otherwise, they do a basic physical attack */
		if (pa->mtyp == PM_MIGO_PHILOSOPHER || pa->mtyp == PM_MIGO_QUEEN) {
			if (rn2(20)) {
				/* make physical attack */
				alt_attk.adtyp = AD_PHYS;
				return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
			}
			else {
				/* do the AD_DRIN attack, noting that we aren't eating brains */
				spec = TRUE;
			}
		}

		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* entirely unharmed */
		if (!has_head_mon(mdef)
			|| (youdef && uclockwork)
			|| (is_android(pd) && (is_mind_flayer(pd) || attk->aatyp != AT_TENT))
			/*|| notonhead*/ //damnit what
			) {
			if (vis) {
				pline("%s %s entirely unharmed.",
					(youdef ? "You" : Monnam(mdef)),
					(youdef ? "seem" : "seems")
					);
			}
			if (youdef && attk->aatyp == AT_TENT && roll_madness(MAD_HELMINTHOPHOBIA)){
				You("panic from the squrming tentacles!");
				HPanicking += 1+rnd(3);
			}
			/* don't do any further damage or anything, but do trigger retaliation attacks */
			return MM_HIT;
		}

		if (slips_free(magr, mdef, attk, vis)) {
			/* don't do any further damage or anything, but do trigger retaliation attacks */
			return MM_HIT;
		}

		/* protected by helmets */
		otmp = (youdef ? uarmh : which_armor(mdef, W_ARMH));
		if((pa->mtyp == PM_STAR_SPAWN || pa->mtyp == PM_DREAM_EATER || pa->mtyp == PM_VEIL_RENDER) && otmp && is_hard(otmp) && otmp->oartifact != ART_APOTHEOSIS_VEIL){
			if (youdef) {
				pline("%s somehow reaches right past your helmet!", Monnam(magr));
			}
			else if (vis) {
				pline("%s somehow reach%s right past %s helmet!",
					(youagr ? "You" : s_suffix(Monnam(magr))),
					(youagr ? "" : "es"),
					s_suffix(mon_nam(mdef))
					);
			}
		} else if (otmp && is_hard(otmp) && 
			((FacelessHelm(otmp) && (otmp->otyp != IMPERIAL_ELVEN_HELM || check_imp_mod(otmp, IEA_BLIND_RES)))
			|| rn2(8))
		){
			if (youdef) {
				/* not body_part(HEAD) */
				Your("helmet blocks the attack to your head.");
			}
			else if (vis) {
				pline("%s helmet blocks %s attack to %s head.",
					s_suffix(Monnam(mdef)),
					(youagr ? "your" : s_suffix(mon_nam(magr))),
					mhis(mdef)
					);
			}
			/* don't do any further damage or anything, but do trigger retaliation attacks */
			return MM_HIT;
		}

		/* reduce damage through half-phys-damage */
		dmg = reduce_dmg(mdef,dmg,TRUE,FALSE);

		/* brains will be attacked now */
		if(is_android(pd) && !is_mind_flayer(pd)){
			int newres;
			newres = android_braindamage(dmg, magr, mdef, vis);
			if (youdef && attk->aatyp == AT_TENT && roll_madness(MAD_HELMINTHOPHOBIA)){
				You("panic from the burrowing tentacles!");
				HPanicking += 1+rnd(6);
			}
			/* don't do any further damage or anything, may trigger retaliation attacks */
			return newres;
		}
		/* mhitu */
		if (youdef) {
			if (u.sealsActive&SEAL_HUGINN_MUNINN){
				unbind(SEAL_HUGINN_MUNINN, TRUE);
			}
			else {
				(void)adjattrib(A_INT, -dmg, FALSE);
				while (dmg--){
					forget(4);	/* lose 4% of memory per point lost*/
					exercise(A_WIS, FALSE);
				}
			}
			/* dunce caps prevent brain-eating messages, and brainlessness death */
			if (!(uarmh && uarmh->otyp == DUNCE_CAP)) {
				if (spec)
					Your("brain has been removed!");
				else
					Your("brain is eaten!");
				/* No such thing as mindless players... */
				check_brainlessness();
				if (youdef && attk->aatyp == AT_TENT && roll_madness(MAD_HELMINTHOPHOBIA)){
					You("panic from the burrowing tentacles!");
					HPanicking += 1+rnd(6);
				}
				/* if a migo scooped out your brain, it stops attacking */
				if (pa->mtyp == PM_MIGO_PHILOSOPHER || pa->mtyp == PM_MIGO_QUEEN)
					return MM_AGR_STOP;
			}
		}
		/* uhitm */
		else if (youagr) {
			int intdrain = dmg;	/* if we are actually tracking monster INT stat, use this */
			dmg += d(dmg, 10); /* fakery, since monsters lack INT scores. NOTE: 1d10 PER POINT OF INT */

			/* message */
			You("%s %s brain!",
				(spec ? "remove" : "eat"),
				s_suffix(mon_nam(mdef))
				);
			/* don't check this part if we aren't eating the brain */
			if (!spec) {
				u.uconduct.food++;
				if (touch_petrifies(mdef->data) && !Stone_resistance && !Stoned) {
					Stoned = 5;
					killer_format = KILLED_BY_AN;
					delayed_killer = mdef->data->mname;
				}
				if (!vegan(mdef->data))
					u.uconduct.unvegan++;
				if (!vegetarian(mdef->data))
					violated_vegetarian();
			}
			/* if victim was mindless, return early */
			if (mindless_mon(mdef)) {
				pline("%s doesn't notice.", Monnam(mdef));
				return MM_HIT;
			}
			/* don't check this part if we aren't eating the brain */
			if (!spec) {
				morehungry(-rnd(dmg * 6)); /* cannot choke; aprox 30 points of hunger per point of 'int' drained */
				if (ABASE(A_INT) < AMAX(A_INT)) {
					ABASE(A_INT) += rnd(4);
					if (ABASE(A_INT) > AMAX(A_INT))
						ABASE(A_INT) = AMAX(A_INT);
					flags.botl = 1;
				}
				exercise(A_WIS, TRUE);
				exercise(A_INT, TRUE);
			}

			/* kills if pretend-brain-eating damage would reduce hp below zero */
			mdef->mhp -= dmg;
			if (mdef->mhp < 1) {
				if (vis) {
					pline("%s last thought fades away...",
						s_suffix(Monnam(mdef)));
				}
				/* use killed, player was responsible */
				killed(mdef);
				if (mdef->mhp > 0)
					return (MM_HIT|MM_DEF_LSVD);	/* lifesaved */
				else
					return (MM_HIT|MM_DEF_DIED);
			}
		}
		/* mhitm */
		else {
			int intdrain = dmg;	/* if we are actually tracking monster INT stat, use this */
			dmg += d(dmg, 10); /* fakery, since monsters lack INT scores. NOTE: 1d10 PER POINT OF INT */

			if (vis)
				pline("%s brain is eaten!", s_suffix(Monnam(mdef)));
			if (mindless_mon(mdef)) {
				if (vis)
					pline("%s doesn't notice.", Monnam(mdef));
				return MM_HIT;
			}
			/* tame creatures get nutrition */
			if (get_mx(magr, MX_EDOG)) {
				EDOG(magr)->hungrytime += dmg * 11; /*aprox 60 points of hunger per 1d10*/
				magr->mconf = 0;
			}
			/* kills if pretend-brain-eating damage would reduce hp below zero */
			mdef->mhp -= dmg;
			if (mdef->mhp < 1) {
				if (vis) {
					pline("%s last thought fades away...",
						s_suffix(Monnam(mdef)));
				}
				/* use monkilled, player was not responsible */
				monkilled(mdef, "", AD_DRIN);
				if (mdef->mhp > 0)
					return (MM_HIT|MM_DEF_LSVD);	/* lifesaved */
				else
					return (MM_HIT | MM_DEF_DIED | ((youagr || grow_up(magr, mdef)) ? 0 : MM_AGR_DIED));	/* grow_up might kill magr */
			}
		}
		/* don't do other damage modifers, we're done here. */
		return MM_HIT;

	case AD_HEAL:
		/* for hostiles, deal damage, not heal */
		if (((youdef || youagr) && !magr->mpeaceful) ||
			(magr->mpeaceful != mdef->mpeaceful)) {
			/* make physical attack */
			alt_attk.adtyp = AD_PHYS;
			result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		}
		else {
			/* don't print a basic hitmessage */
			result = MM_HIT;
		}
		/* from this point on, assumes non-hostility */
		/* mhitu */
		if (youdef) {
			/* to operate on the player, the player must be disrobed */
			if (!uarm && !uarmu) {

				pline("%s hits!  (I hope you don't mind.)", Monnam(magr));

				/* heal,
				 * increase maxhp using healup()'s diminishing returns,
				 * cure sickness
				 */
				healup(dmg, dmg/4, TRUE, FALSE);

				/* sometimes exercise STR, CON */
				if (!rn2(3)) exercise(A_STR, TRUE);
				if (!rn2(3)) exercise(A_CON, TRUE);

				/* and sometimes go away */
				if (!rn2(33)) {
					/* run/teleport away */
					if (!tele_restrict(magr))
						(void)rloc(magr, TRUE);
					monflee(magr, d(3, 6), TRUE, FALSE);
					return MM_AGR_STOP;	/* maybe teleported away, definitely not continuing to attack */
				}
				else if (!rn2(333)) {
					/* poof out of existance*/
					mongone(magr);
					return MM_AGR_DIED;
				}
			}
			/* no healing for you! */
			else {
				/* complain to a fellow healer */
				if (flags.soundok && !(moves % 5)) {
					if (Role_if(PM_HEALER))
						verbalize("Doc, I can't help you unless you cooperate.");
				}
				/* give the player some space */
				monflee(magr, d(3, 6), TRUE, FALSE);
				/* and miss */
				return (MM_MISS);
			}
		}
		/* uhitm, mhitm */
		else {
			/* my guess: uhitm behaviour is likely rendered useless by other
			 * circumstances and effects, but oh well */
			/* heals patient by dmg, up to its maxhp */
			mdef->mhp += dmg;
			if (mdef->mhpmax < mdef->mhp)
				mdef->mhp = mdef->mhpmax;
			/* message if patient is seen */
			if (canseemon(mdef))
				pline("%s looks better!", mon_nam(mdef));
		}
		return result;

	case AD_WRAP:
		/* Monsters cannot grab each other
		 * If the player isn't involved,
		 * or if either creature could be the one doing the grabbing,
		 * substitute simple physical damage */
		if (!(youagr || youdef) || (sticks(mdef)) || (youagr && u.uavoid_grabattk)){
			/* make physical attack */
			alt_attk.adtyp = AD_PHYS;
			return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		}
		else {
			/* figure out who the other creature is */
			if (youagr)
				mtmp = mdef;
			else if (youdef)
				mtmp = magr;
			else {
				impossible("who is the other one being stuck?");
				return MM_MISS;
			}
		}

		/* is the player stuck to the other creature? */
		if (notmcan || (u.ustuck == mtmp)) {
			/* if not attached to anything, attempt to attach to the other creature*/
			if (!u.ustuck && (rn2(4) || attk->aatyp == AT_TUCH || attk->aatyp == AT_HUGS || attk->aatyp == AT_TENT)) {
				if (slips_free(magr, mdef, attk, vis)) {
					/* message was printed (if visible) */
					/* nothing happens */
					return MM_MISS;
				}
				else {
					/* get stuck */
					/* print a message (vis is assumed, since the player is involved) */
					if (attk->aatyp == AT_HUGS) {
						pline("%s grab%s %s!",
							(youagr ? "You" : Monnam(mtmp)),
							(youagr ? "" : "s"),
							(youagr ? mon_nam(mtmp) : "you")
							);
					}
					else if (attk->aatyp == AT_TUCH) {
						pline("%s stick%s to %s!",
							(youagr ? "You" : Monnam(mtmp)),
							(youagr ? "" : "s"),
							(youagr ? mon_nam(mtmp) : "you")
							);
					}
					else if (attk->aatyp == AT_TENT) {
						pline("%s swing%s %s tentacles around %s!",
							(youagr ? "You" : Monnam(mtmp)),
							(youagr ? "" : "s"),
							(youagr ? "your" : "its"),
							(youagr ? mon_nam(mtmp) : "you")
							);
					}
					else {
						pline("%s swing%s %s around %s!",
							(youagr ? "You" : Monnam(mtmp)),
							(youagr ? "" : "s"),
							(youagr ? "yourself" : "itself"),
							(youagr ? mon_nam(mtmp) : "you")
							);
					}

					if (pd->mtyp == PM_TOVE)
					{
						pline("%s, %s %s much too slithy to grab!",
							(youagr ? "Unfortunately" : "Fortunately"),
							(youagr ? mon_nam(mtmp) : "you"),
							(youagr ? "is" : "are")
							);
					}
					else
						u.ustuck = mtmp; /* it is now stuck to you, and you to it */
				}
			}
			/* if you are attached to the other creature, do the thing! */
			else if (u.ustuck == mtmp) {
				/* drowning? */
				if ((is_pool(x(magr), y(magr), FALSE) || pa->mtyp == PM_DAUGHTER_OF_NAUNET)
					&& !(youdef ? Swimming : mon_resistance(mdef, SWIMMING))
					&& !(youdef ? Breathless : breathless_mon(mdef))
					&& !(amphibious(pd))	/* Note: Amphibious is magical breathing, Swimming, or amphibious(). 
											   Breathless checks magical breathing (and breathless()) and swimming should be skipped here,
											   leaving only amphibious() */
				){
					int ltyp = levl[x(magr)][y(magr)].typ;
					boolean moat =
						(ltyp != POOL) &&
						(ltyp != WATER) &&
						!Is_medusa_level(&u.uz) &&
						!Is_waterlevel(&u.uz);
					boolean daughter = pa->mtyp == PM_DAUGHTER_OF_NAUNET;

					/* water damage to drownee's inventory */
					water_damage((youdef ? invent : mdef->minvent), FALSE, FALSE, level.flags.lethe, mdef);

					if (youdef) {
						if (u.divetimer > 0){
							if(daughter)
								pline("%s flows all around you!", Monnam(mtmp));
							else
								pline("%s pulls you into the %s!", Monnam(mtmp), moat ? "moat" : "pool of water");
							if (!uarm || is_light_armor(uarm)){
								pline("%s is crushing the breath out of you!", Monnam(mtmp));
								u.divetimer -= dmg;
							}
							else if (is_medium_armor(uarm)){
								pline("%s is squeezing the breath out of you!", Monnam(mtmp));
								u.divetimer -= dmg / 2 + 1;
							}
							else {
								u.divetimer -= dmg / 4 + 1;
							}
							if (u.divetimer < 0) u.divetimer = 0;
						}
						else { /*out of air*/
							pline("%s drowns you...", Monnam(mtmp));
							killer_format = KILLED_BY_AN;
							Sprintf(buf, "%s by %s",
								moat ? "moat" : "pool of water",
								an(mtmp->data->mname));
							killer = buf;
							if (!u.uconduct.killer && !youagr){
								//Pcifist PCs aren't combatants so if something kills them up "killed peaceful" type impurities
								IMPURITY_UP(u.uimp_murder)
								IMPURITY_UP(u.uimp_bloodlust)
							}
							done(DROWNING);
							return (MM_HIT|MM_DEF_DIED);
						}
					}
					else if (youagr) {
						You("drown %s...", mon_nam(mdef));
						/* kill */
						return xdamagey(magr, mdef, attk, FATAL_DAMAGE_MODIFIER);
					}
					else {
						impossible("Player must be youagr or youdef for drowning.");
						return MM_HIT;
					}
				}
				/* crushing bearhug? */
				else if (attk->aatyp == AT_HUGS){
					pline("%s is %s.",
						Monnam(mtmp),
						(youagr ? "being crushed" : "crushing you")
						);
				}
				/* Gentle squeeze. Restore 1d3 points of sanity. Just kidding. */
				else {
					dmg = 0;
					pline("%s is %s.",
						Monnam(mtmp),
						(youagr ? "wrapped up in you" : "wrapped tight around you")
						);
				}
			}
			/* nothing really happens */
			else {
				if (flags.verbose) {
					if (youagr) {
						You("brush against %s %s.",
							s_suffix(mon_nam(mdef)),
							mbodypart(mdef, LEG));
					}
					else if (youdef) {
						pline("%s brushes against your %s.", Monnam(mtmp),
							body_part(LEG));
					}
				}
				/* return miss -- nothing happens */
				return MM_MISS;
			}
		}
		/* make physical attack without hitmsg */
		/* This CANNOT be an AT_HUGS attack, because then it will attempt to do a wrap attack again */
		if (alt_attk.aatyp == AT_HUGS)
			alt_attk.aatyp = AT_CLAW;
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_CHKH:
		/* make modified physical attack */
		/* gains much more damage vs player */
		if (youdef) {
			dmg += u.chokhmah * 5;
		}
		else {
			dmg += u.chokhmah;
		}
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);


	case AD_CHRN:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* vs player, do special effects like as a unicorn horn */
		if (youdef) {
			long lcount = (long)rnd(100);

			switch (rn2(6)) {
			case 0:
				make_sick(Sick ? Sick / 3L + 1L : (long)rn1(ACURR(A_CON), 20),
					"cursed unicorn's horn", TRUE, SICK_NONVOMITABLE);
				break;
			case 1:
				make_blinded(Blinded + lcount, TRUE);
				break;
			case 2: if (!Confusion)
				You("suddenly feel %s.",
				Hallucination ? "trippy" : "confused");
				make_confused(HConfusion + lcount, TRUE);
				break;
			case 3:
				make_stunned(HStun + lcount, TRUE);
				break;
			case 4:
				(void)adjattrib(rn2(A_MAX), -1, FALSE);
				break;
			case 5:
				(void)make_hallucinated(HHallucination + lcount, TRUE, 0L);
				break;
			}

			alt_attk.adtyp = AD_PHYS;
		}
		/* vs monsters, make an appropriate damage+status effect bundled attack */
		else {
			switch (rn2(6)) {
			case 0:
				/* technically incorrect; this is pure sickness damage */
				alt_attk.adtyp = AD_DISE;
				break;
			case 1:
				alt_attk.adtyp = AD_BLND;
				break;
			case 2: 
				alt_attk.adtyp = AD_CONF;
				break;
			case 3:
				alt_attk.adtyp = AD_STUN;
				break;
			case 4:
				/* no good substitute for random stat drain */
				alt_attk.adtyp = AD_PHYS;
				break;
			case 5:
				alt_attk.adtyp = AD_HALU;
				break;
			}
		}
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_TELE:
		/* hitter tries to teleport without making an attack */
		if (!youagr) {
			if (!tele_restrict(magr)) {
				(void)rloc(magr, TRUE);
				return MM_AGR_STOP;
			}
		}
		else {
			/* sadly needs a hack to determine if the player was teleported to a new location,
			* which would interrupt the attacker's attack chain */
			int oldux = u.ux, olduy = u.uy;
			tele();
			/* Were you moved? If so, stop attacking. */
			if (oldux != u.ux || olduy != u.uy)
				return MM_AGR_STOP;	/* defender moved */
		}
		/* if that failed, make a physical attack instead */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);

	case AD_AXUS:
		/* fancy hitmsg */
		if (vis && dohitmsg) {
			pline("%s reaches out with %s %s!  A corona of dancing energy surrounds the %s!",
				Monnam(magr), mhis(magr), mbodypart(magr, ARM), mbodypart(magr, HAND));
		}
		/* "Positive energy": shock / fire / stun combo */
		/* completely resisted by Shock res */
		if (!Shock_res(mdef)) {
			alt_attk.aatyp = AT_NONE;
			alt_attk.adtyp = AD_ELEC;
			result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);		/* elec damage */
			if (result&(MM_DEF_DIED | MM_DEF_LSVD)) return result;
			alt_attk.adtyp = AD_FIRE;
			result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);		/* fire damage */
			if (result&(MM_DEF_DIED | MM_DEF_LSVD)) return result;
			alt_attk.adtyp = AD_STUN;
			result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, 3, dieroll, vis, ranged);		/* 3 turn stun, minor physical damage */
			if (result&(MM_DEF_DIED | MM_DEF_LSVD)) return result;
		}
		else {
			/* 1-damage physical touch */
			alt_attk.aatyp = AT_TUCH;
			alt_attk.adtyp = AD_PHYS;
			result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, 1, dieroll, vis, ranged);
			if (result&(MM_DEF_DIED | MM_DEF_LSVD)) return result;
		}

		/* but wait, there's more! */
		if (vis && dohitmsg) {
			pline("%s reaches out with %s other %s!  A penumbra of shadows surrounds the %s!",
				Monnam(magr), mhis(magr), mbodypart(magr, ARM), mbodypart(magr, HAND));
		}
		/* "Negative energy": cold / drain combo */
		/* completely resisted by Drain res */
		if (!Drain_res(mdef)) {
			alt_attk.aatyp = AT_NONE;
			alt_attk.adtyp = AD_COLD;
			result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);		/* cold damage */
			if (result&(MM_DEF_DIED | MM_DEF_LSVD)) return result;
			alt_attk.adtyp = AD_DRLI;
			result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, 0, dieroll, vis, ranged);		/* level drain */
			if (result&(MM_DEF_DIED | MM_DEF_LSVD)) return result;
		}
		else {
			/* 1-damage physical touch */
			alt_attk.aatyp = AT_TUCH;
			alt_attk.adtyp = AD_PHYS;
			result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, 1, dieroll, vis, ranged);
			if (result&(MM_DEF_DIED | MM_DEF_LSVD)) return result;
		}

		return result;

	case AD_ACFR:
		/* make a physical attack */
		alt_attk.adtyp = AD_PHYS;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		if (result&(MM_DEF_DIED|MM_DEF_LSVD)) return result;
		alt_attk.aatyp = AT_NONE;
		/* add fire damage, if not resistant */
		if (!Fire_res(mdef)) {
			alt_attk.adtyp = AD_FIRE;
			result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);
			if (result&(MM_DEF_DIED|MM_DEF_LSVD)) return result;
		}
		/* add holy damage */
		if (hates_holy_mon(mdef)) {
			if (vis) {
				pline("The holy flames sear %s!",
					(youdef ? "your flesh" : mon_nam(mdef))
					);
			}
			result = xdamagey(magr, mdef, &alt_attk, dmg);
			if (result&(MM_DEF_DIED|MM_DEF_LSVD)) return result;
		}

		return result;

	case AD_HOLY:
		/* make a physical attack */
		alt_attk.adtyp = AD_PHYS;
		alt_attk.damn = 1;
		alt_attk.damd = 1;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		if (result&(MM_DEF_DIED|MM_DEF_LSVD)) return result;
		/* add holy damage */
		if (youdef ? (hates_holy(pd)) : (hates_holy_mon(mdef))) {
			if (vis) {
				pline("The holy light sears %s!",
					(youdef ? "your flesh" : mon_nam(mdef))
					);
			}
			dmg *= 2;
		}
		alt_attk.adtyp = AD_HOLY;
		/* apply half-magic */
		dmg = reduce_dmg(mdef,dmg,FALSE,TRUE);

		result = xdamagey(magr, mdef, &alt_attk, dmg);
		return result;

	case AD_UNHY:
		/* make a physical attack */
		alt_attk.adtyp = AD_PHYS;
		alt_attk.damn = 1;
		alt_attk.damd = 1;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		if (result&(MM_DEF_DIED|MM_DEF_LSVD)) return result;
		/* add unholy damage */
		if (youdef ? (hates_unholy(pd)) : (hates_unholy_mon(mdef))) {
			if (vis) {
				pline("The unholy light sears %s!",
					(youdef ? "your flesh" : mon_nam(mdef))
					);
			}
			dmg *= 2;
		}
		alt_attk.adtyp = AD_UNHY;
		/* apply half-magic */
		dmg = reduce_dmg(mdef,dmg,FALSE,TRUE);

		result = xdamagey(magr, mdef, &alt_attk, dmg);
		return result;

	case AD_HLUH:
		/* make a physical attack */
		alt_attk.adtyp = AD_PHYS;
		alt_attk.damn = 1;
		alt_attk.damd = 1;
		result = xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		if (result&(MM_DEF_DIED|MM_DEF_LSVD)) return result;
		/* add unholy damage */
		if (youdef ? (hates_unholy(pd) || hates_holy(pd)) : (hates_unholy_mon(mdef) || hates_holy_mon(mdef))) {
			if (vis) {
				pline("The corrupted light sears %s!",
					(youdef ? "your flesh" : mon_nam(mdef))
					);
			}
			if(youdef ? (hates_unholy(pd) && hates_holy(pd)) : (hates_unholy_mon(mdef) && hates_holy_mon(mdef)))
				dmg *= 3;
			else
				dmg *= 2;
		}
		alt_attk.adtyp = AD_HLUH;
		/* apply half-magic */
		dmg = reduce_dmg(mdef,dmg,FALSE,TRUE);

		result = xdamagey(magr, mdef, &alt_attk, dmg);
		return result;

	case AD_FRWK:
		/* creates many explosions! */
		/* no hit message? */
		{
		int i = 1 + rnd(3);
		int x, y;
		for (; i > 0; i--) {
			x = x(mdef) + rn2(3) - 1;
			y = y(mdef) + rn2(3) - 1;
			if(!isok(x,y)){
				x = x(mdef);
				y = y(mdef);
			}
			explode(
				x,						/* x center coord */
				y,						/* y center coord */
				AD_PHYS,				/* damage type  */
				-1,						/* object causing explosion (none) */
				dmg,					/* damage per explosion */
				rn2(EXPL_MAX),			/* color */
				1);						/* radius */
		}
		int retval = MM_HIT;
		if (DEADMONSTER(mdef))
			retval |= MM_DEF_DIED;
		if (DEADMONSTER(magr)) //Blew itself up (likely)
			retval |= MM_AGR_DIED;

		return retval;
		}
		
	case AD_PAIN:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* big picture: can stunlock monsters, can't stunlock you because it uses the Screaming status effect */
		if((!nonliving(pd) || is_android(pd)) 
			&& pd->mflagsa != MA_ELEMENTAL /*not a PURE elemental like a vortex, sphere, or elemental*/
			&& !has_template(mdef, TOMB_HERD) /*not a statue-piloting thingy */
			&& !is_great_old_one(pd)
		){
			static long ulastscreamed = 0;
			static long lastscreamed = 0;
			static struct monst *lastmon = 0;
			//Big picture note: monsters can be stunlocked this way, you can't since it uses Screaming statues
			if (youdef) {
				if(ulastscreamed < monstermoves){
					ulastscreamed = monstermoves;
					if (!is_silent(pd)){
						You("%s from the pain!", humanoid_torso(pd) ? "scream" : "shriek");
					}
					else {
						You("writhe in pain!");
					}
				}
				HScreaming += 2;
			}
			else {
				if(lastscreamed < monstermoves || lastmon != mdef){
					lastscreamed = monstermoves;
					lastmon = mdef;
					if (!is_silent_mon(mdef)){
						if (canseemon(mdef))
							pline("%s %s in pain!", Monnam(mdef), humanoid_torso(mdef->data) ? "screams" : "shrieks");
						else You_hear("%s %s in pain!", mdef->mtame ? noit_mon_nam(mdef) : mon_nam(mdef), humanoid_torso(mdef->data) ? "screaming" : "shrieking");
					}
					else {
						if (canseemon(mdef))
							pline("%s writhes in pain!", Monnam(mdef));
					}
				}
				mdef->movement = max(mdef->movement - 6, -12);
			}
		}
		/* make posion/physical attack without hitmsg */
		alt_attk.adtyp = AD_DRST;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_MROT:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* Do the curse */
		result |= mummy_curses_x(magr, mdef);
		if(result&MM_DEF_DIED)
			return result;
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return (result|xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged));
	case AD_LAVA:
		
		if(distmin(x(mdef), y(mdef), x(magr), y(magr)) > 1)
			return MM_MISS;
	
		if(youdef){
			if(u.ustuck != magr ){
				if(!sticks(mdef)){
					pline("%s begins to ooze around you!", Monnam(magr));
					u.ustuck = magr;
				}
				else pline("%s oozes around you!", Monnam(magr));
			}
			else {
				pline("%s is crushing you!", Monnam(magr));
			}
			burn_away_slime();
			melt_frozen_air();
		}
		else if(youagr){
			if(u.ustuck != mdef ){
				if(!sticks(mdef) && !(youagr && u.uavoid_grabattk)){
					pline("You begin to ooze around %s!", mon_nam(mdef));
					u.ustuck = magr;
				}
				else pline("You ooze around %s!", mon_nam(mdef));
			}
			else {
				pline("You are crushing %s!", mon_nam(mdef));
			}
		}
		else {
			if(vis)
				pline("%s oozes around %s!", Monnam(magr), mon_nam(mdef));
		}

		if(!UseInvFire_res(mdef)){
			burnarmor(mdef, TRUE);
			destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
			destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
			destroy_item(mdef, POTION_CLASS, AD_FIRE);
			burnarmor(mdef, TRUE);
		}

		alt_attk.adtyp = AD_EFIR;
		alt_attk.damn = mlev(magr)/3 + attk->damn;
		return xmeleehurty(magr, mdef, &alt_attk, &alt_attk, weapon_p, FALSE, -1, dieroll, vis, ranged);

//////////////////////////////////////////////////////////////
// JUST CHANGING THE DAMAGE TYPE
//////////////////////////////////////////////////////////////
	case AD_OONA:
		/* use correct damage type */
		/* note: replaces originalattk */
		alt_attk.adtyp = u.oonaenergy;
		return xmeleehurty(magr, mdef, &alt_attk, &alt_attk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);

	case AD_SESN:
		/* use random damage type */
		/* note: replaces originalattk */
		switch(rnd(4)){
			case 1:
				//Winter: Frozen
				alt_attk.adtyp = AD_ECLD;
			break;
			case 2:
				//Spring: Growing
				alt_attk.adtyp = AD_SHRD;
			break;
			case 3:
				//Summer: 
				alt_attk.adtyp = AD_POLN;
			break;
			case 4:
				//Fall: Withering
				alt_attk.adtyp = AD_DRLI;
			break;
		}
		return xmeleehurty(magr, mdef, &alt_attk, &alt_attk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);

	case AD_PYCL:
		/* use random damage type */
		/* note: replaces originalattk */
		switch(rnd(3)){
			case 1:
				alt_attk.adtyp = AD_FIRE;
			break;
			case 2:
				alt_attk.adtyp = AD_PHYS;
			break;
			case 3:
				alt_attk.adtyp = AD_DRCO;
			break;
		}
		return xmeleehurty(magr, mdef, &alt_attk, &alt_attk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);

	case AD_HDRG:
		/* use halfdragon's breath type */
		/* note: replaces originalattk */
		if (youagr && Race_if(PM_HALF_DRAGON))
			alt_attk.adtyp = flags.HDbreath;
		else if (is_half_dragon(pa))
			alt_attk.adtyp = magr->mvar_hdBreath;
		else
			alt_attk.adtyp = AD_COLD;
		return xmeleehurty(magr, mdef, &alt_attk, &alt_attk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);

	case AD_RBRE:	/* should actually be breath-only */
	case AD_RETR:
		/* random between fire/cold/elec damage */
		/* note: replaces originalattk */
		switch (rn2(3))
		{
		case 0: alt_attk.adtyp = AD_FIRE; break;
		case 1: alt_attk.adtyp = AD_COLD; break;
		case 2: alt_attk.adtyp = AD_ELEC; break;
		}
		return xmeleehurty(magr, mdef, &alt_attk, &alt_attk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);

//////////////////////////////////////////////////////////////
// BINDER SPIRIT ATTACKS
//////////////////////////////////////////////////////////////
	case AD_IRIS:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* note when the player last made an Iris attack */
		if (youagr) {
			u.irisAttack = moves;
		}
		/* 1/20 chance of shriveling opponent */
		if (!rn2(20)){
			if (!nonliving(pd) &&
				!is_anhydrous(pd)){
				/* message */
				if (vis) {
					pline("%s %s at the touch of %s iridescent tentacles!",
						(youdef ? "You" : Monnam(mdef)),
						(youdef ? "shrivel" : "shrivels"),
						(youagr ? "your" : s_suffix(mon_nam(magr)))
						);
				}
				/* bonus damage for the player (5 spirit dice) */
				if (youagr) {
					dmg += d(5, spiritDsize());
				}
				/* aggressor lifesteals by dmg dealt */
				heal(magr, min(dmg, *hp(mdef)));
			}
			else {
				shieldeff(x(mdef), y(mdef));
			}
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_NABERIUS:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		if (!youdef && mdef->mflee) {	/* what would be an acceptable condition for the player to be affected? */
			if ((*hp(magr) < *hpmax(magr)) && vis) {
				pline("%s crimson fangs run red with blood.",
					(youagr ? "Your" : s_suffix(Monnam(magr))));
			}
			/* healup by d8 */
			heal(magr, rnd(8));
		}
		if (!youdef && mdef->mpeaceful) {	/* what would be an acceptable condition for the player to be affected? */
			if (vis) {
				pline("%s tarnished fangs %s in the %s.", 
					(youagr ? "Your" : s_suffix(Monnam(magr))),
					(levl[x(magr)][y(magr)].lit ? "shine" : "glimmer"),
					(levl[x(magr)][y(magr)].lit ? "light" : "dark")
					);
				if (youdef) {
					long lcount = (long)rnd(8);
					make_confused(HConfusion + lcount, FALSE);
					make_stunned(HStun + lcount, FALSE);
				}
				else {
					mdef->mconf = 1;
					mdef->mstun = 1;
				}
			}
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_OTIAX:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}
		/* note when the player last made an Otiax attack */
		if (youagr) {
			u.otiaxAttack = moves;
		}

		/* only implemented for uhitm :( */
		if (youagr && !youdef) {
			/* 1/10 chance of stealing items */
			if (!rn2(10)){
				struct obj *otmp2, **minvent_ptr;
				long unwornmask = 0L;

				/* Don't steal worn items, and downweight wielded items */
				if ((otmp2 = mdef->minvent) != 0) {
					while (otmp2 &&
						otmp2->owornmask&W_ARMOR &&
						!((otmp2->owornmask&W_WEP) && !rn2(10))
						) otmp2 = otmp2->nobj;
				}
				/* Still has handling for worn items, incase that changes */
				if (otmp2 != 0){
					int dx, dy;
					/* take the object away from the monster */
					if (otmp2->quan > 1L){
						otmp2 = splitobj(otmp2, 1L);
						obj_extract_self(otmp2); //wornmask is cleared by splitobj
					}
					else{
						obj_extract_self(otmp2);
						if ((unwornmask = otmp2->owornmask) != 0L) {
							mdef->misc_worn_check &= ~unwornmask;
							if (otmp2->owornmask & W_WEP) {
								setmnotwielded(mdef, otmp2);
								MON_NOWEP(mdef);
							}
							if (otmp2->owornmask & W_SWAPWEP){
								setmnotwielded(mdef, otmp2);
								MON_NOSWEP(mdef);
							}
							otmp2->owornmask = 0L;
							update_mon_intrinsics(mdef, otmp2, FALSE, FALSE);
						}
					}
					Your("mist tendrils free %s.", doname(otmp2));
					mdrop_obj(mdef, otmp2, FALSE);
					/* more take-away handling, after theft message */
					if (unwornmask & W_WEP) {		/* stole wielded weapon */
						possibly_unwield(mdef, FALSE);
					}
					else if (unwornmask & W_ARMG) {	/* stole worn gloves */
						mselftouch(mdef, (const char *)0, TRUE);
						if (mdef->mhp <= 0)	/* it's now a statue */
							return 1; /* monster is dead */
					}
				}
			}
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_SIMURGH:
		/* print a basic hit message */
		if (vis && dohitmsg) {
			xyhitmsg(magr, mdef, originalattk);
		}

		/* 4/5 chance of just iron-hating damage */
		if (rn2(5)){
			/* only iron */
			if (hates_iron(pd)) {
				if (vis) {
					pline("%s cold iron quills brush %s.",
						(youagr ? "Your" : s_suffix(Monnam(magr))),
						(youdef ? "you" : mon_nam(mdef))
						);
				}
				mdef->mironmarked = TRUE;
				result = xdamagey(magr, mdef, attk, d(rnd(5), (mlev(mdef) + 1) / 2));
				if (result&(MM_DEF_DIED|MM_DEF_LSVD)) return result;
			}
		}
		/* 1/5 chance of radiant feathers */
		else {
			/* message */
			if (vis) {
				pline("Radiant feathers slice through %s.",
					(youdef ? "you" : mon_nam(mdef)));
			}
			/* iron */
			if (hates_iron(pd)){
				if (vis) {
					pline("The cold iron rachises sear %s.",
						(youdef ? "you" : mon_nam(mdef)));
				}
				mdef->mironmarked = TRUE;
				result = xdamagey(magr, mdef, attk, d(5, mlev(mdef)));
				if (result&(MM_DEF_DIED|MM_DEF_LSVD)) return result;
			}
			/* check random "resistances" */
			int i;
			boolean hurts, extrahurts;
			for (i = 0; i < 5; i++){
				hurts = FALSE;
				extrahurts = FALSE;
				switch (rn2(18))
				{
				case 0:		hurts = !Fire_res(mdef); extrahurts = Cold_res(mdef); break;
				case 1:		hurts = !Cold_res(mdef); extrahurts = Fire_res(mdef); break;
				case 2:		hurts = !Disint_res(mdef); break;
				case 3:		hurts = pm_invisible(pd); break;
				case 4:		hurts = !Shock_res(mdef); break;
				case 5:		hurts = !Poison_res(mdef); break;
				case 6:		hurts = !Acid_res(mdef); break;
				case 7:		hurts = !Stone_res(mdef); break;
				case 8:		hurts = !Drain_res(mdef); break;
				case 9:		hurts = !Sick_res(mdef); break;
				case 10:	hurts = is_undead(pd); break;
				case 11:	hurts = is_fungus(pd); break;
				case 12:	hurts = infravision(pd); break;
				case 13:	hurts = opaque(pd); break;
				case 14:	hurts = !breathless_mon(mdef); break;
				case 15:	hurts = !(insubstantial(pd) || noncorporeal(pd) || (youdef ? Passes_walls : mon_resistance(mdef, PASSES_WALLS))); break;
				case 16:	hurts = is_whirly(pd); break;
				case 17:	hurts = !mindless_mon(mdef); break;
				}
				if (hurts) {
					/* in this case, non-players arbitrarily get a spiritDsize of 5 */
					result = xdamagey(magr, mdef, attk, rnd((extrahurts ? 2 : 1) * (youagr ? spiritDsize() : 5)));
					if (result&(MM_DEF_DIED|MM_DEF_LSVD)) return result;
				}
				else {
					if (vis) {
						shieldeff(x(mdef), y(mdef));
					}
				}
			}
			/* stunning radiance */
			if (!resists_blnd(mdef)) {
				if (youdef) {
					make_stunned(HStun + rnd(5), TRUE);
				}
				else {
					mdef->mstun = 1;
				}
			}
		}
		/* make physical attack without hitmsg */
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);
	
//////////////////////////////////////////////////////////////
// THE RIDERS' ATTACKS
//////////////////////////////////////////////////////////////
		/* riders' */
	case AD_DETH:
		/* fancy hitmsg */
		if (vis && dohitmsg) {
			pline("%s %s out with %s deadly touch.",
				(youagr ? "You" : Monnam(magr)),
				(youagr ? "reach" : "reaches"),
				(youagr ? "your" : mhis(magr))
				);
		}
		if(youdef || youagr){
			IMPURITY_UP(u.uimp_death_magic)
		}
		/* undead are immune to the special effect */
		if (is_undead(pd) || (youdef && u.sealsActive&SEAL_OSE)) {
			if (youdef) {
				pline("Was that the touch of death?");
			}
			else if (vis) {
				pline("%s looks no deader than before.", Monnam(mdef));
			}
		}
		else {
			/*  3/20 chance of instakill; falls through to lifedrain */
			/* 15/20 chance of lifedrain */
			/*  remainder (5/20) no effect */
			int chance = rn2(20);
			if (chance < 3) {
				/* instakill */
				if (youdef) {
					if (!Antimagic) {
						/* heal Death to full */
						heal(magr, *hpmax(magr));
						/* instakill */
						killer_format = KILLED_BY_AN;
						killer = "touch of death";
						done(DIED);
						return (MM_HIT|MM_DEF_LSVD);	/* must have lifesaved */
					}
				}
				else {
					if (!resists_magm(mdef) && !mm_resist(mdef, magr, 0, 0)) {
						/* heal Death to full */
						heal(magr, *hpmax(magr));
						/* instakill */
						*hp(mdef) = 0;
						if (youagr)
							killed(mdef);
						else
							monkilled(mdef, "", AD_DETH);

						if (*hp(mdef) > 0)
							return (MM_HIT|MM_DEF_LSVD); /* mdef lifesaved */
						else
							return (MM_HIT | MM_DEF_DIED | ((youagr || grow_up(magr, mdef)) ? 0 : MM_AGR_DIED));
					}
				}
			}
			if (chance < 15){
				/* message */
				if (youdef) {
					You_feel("your life force draining away...");
				}
				else if (vis) {
					pline("%s looks weaker!", Monnam(mdef));
				}
				/* aggressor lifesteals by damage dealt */
				heal(magr, min(dmg, *hp(mdef)));
				/* reduce defender's maxhp */
				int hpdrain = rn2(dmg / 2 + 1);
				/* vs player */
				if (youdef) {
					int minhp;
					/*
					* Apply some of the damage to permanent hit points:
					*	polymorphed	    100% against poly'd hpmax
					*	hpmax > 25*lvl	    100% against normal hpmax
					*	hpmax > 10*lvl	50..100%
					*	hpmax >  5*lvl	25..75%
					*	otherwise	 0..50%
					* Never reduces hpmax below 1 hit point per level.
					*/
					if (Upolyd || u.uhpmax > 25 * u.ulevel) hpdrain = dmg;
					else if (u.uhpmax > 10 * u.ulevel) hpdrain += dmg / 2;
					else if (u.uhpmax > 5 * u.ulevel) hpdrain += dmg / 4;

					if (Upolyd) {
						/* [can't use youmonst.m_lev] */
						minhp = min((int)youracedata->mlevel, u.ulevel);
					}
					else {
						minhp = u.ulevel;
					}
					/* reduce maxhp */
					*hpmax(mdef) -= hpdrain;
					if (*hpmax(mdef) < minhp)
						*hpmax(mdef) = minhp;
					flags.botl = 1;
				}
				/* vs monsters */
				else {
					/* check if that would kill */
					if (*hp(mdef) <= hpdrain || *hpmax(mdef) <= hpdrain)
					{
						*hp(mdef) = 0;
						if (youagr)
							killed(mdef);
						else
							monkilled(mdef, "", AD_DETH);
						if (*hp(mdef) > 0)
							return (MM_HIT|MM_DEF_LSVD); /* mdef lifesaved */
						else
							return (MM_HIT | MM_DEF_DIED | ((youagr || grow_up(magr, mdef)) ? 0 : MM_AGR_DIED));
					}
					else {
						/* drain the hp */
						*hp(mdef) -= hpdrain;
						*hpmax(mdef) -= hpdrain;
						dmg -= hpdrain;	/* don't double up on the maxhpdrain damage */
					}
				}
			}
			else {
				/* shield effect */
				if (Magic_res(mdef))
					shieldeff(x(mdef), y(mdef));
				/* message */
				if (youdef) {
					pline("Lucky for you, it didn't work!");
				}
				else {
					if (vis)
						pline("That didn't work...");
				}
				/* deal no damage */
				return MM_HIT;
			}
		}
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_PEST:
		/* fancy hitmsg */
		if(youagr){
			//youdef handled by diseasemu
			IMPURITY_UP(u.uimp_illness)
		}
		if (vis && dohitmsg) {
			pline("%s %s out, and %s %s.",
				(youagr ? "You" : Monnam(magr)),
				(youagr ? "reach" : "reaches"),
				(youdef ? "you" : mon_nam(mdef)),
				(youdef ? "feel fever and chills" : "looks rather ill")
				);
		}
		/* very very different effect between mhitu and xhitm */
		if (youdef) {
			(void)diseasemu(pa); /* plus the normal damage */
			/* healup based on how sick you are? */
			if (!Sick_res(mdef)) {
				heal(magr, (*hpmax(magr) / Sick));
			}
		}
		else {
			/* arbitrarily reduce defender's hp (by 1/2 if not sickres, by 3/4 if sickres) */
			if (!mm_resist(mdef, magr, 0, NOTELL)) {
				*hpmax(mdef) = min(*hpmax(mdef) * (Sick_res(mdef) ? 2 : 3) / 4, mlev(mdef));
				if (*hp(mdef) > *hpmax(mdef))
					*hp(mdef) = *hpmax(mdef);
			}
			/* healup */
			if (!Sick_res(mdef)) {
				heal(magr, (*hpmax(magr) / 10));
			}
		}
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_FAMN:
		if(youdef || youagr){
			IMPURITY_UP(u.uimp_disaster)
		}
		/* fancy hitmsg */
		if (vis && dohitmsg) {
			pline("%s %s out, and %s body shrivels.",
				(youagr ? "You" : Monnam(magr)),
				(youagr ? "reach" : "reaches"),
				(youdef ? "your" : s_suffix(mon_nam(mdef)))
				);
		}
		if (youdef){
			exercise(A_CON, FALSE);
			if (!is_fainted()){
				int hungr = rn1(40, 40);
				morehungry(hungr*get_uhungersizemod());
				//heal by the amount of HP it would heal by resting for that nutr worth of turns
				heal(magr, (mlev(magr)*hungr / HEALCYCLE));
			}
		}
		else {
			/* pets can get hungry */
			if (get_mx(mdef, MX_EDOG)){
				int hungr = rn1(120, 120);
				EDOG(mdef)->hungrytime -= hungr;
				//heal by the amount of HP it would heal by resting for that nutr worth of turns
				heal(magr, (mlev(magr)*hungr / 30));
			}
			/* other creatures don't have hungrytime */
			else {
				dmg += rnd(10);
				/* if the attack would kill, give a special message and heal Famine */
				if (*hp(mdef) <= dmg)
				{
					/* message */
					if (vis) {
						pline("%s starves.", Monnam(mdef));
					}
					/* fully heal Famine */
					heal(magr, *hpmax(magr));
					/* kill */
					*hp(mdef) = 0;
					if (youagr)
						xkilled(mdef, 0);
					else
						monkilled(mdef, "", AD_FAMN);
					if (*hp(mdef) > 0)
						return (MM_HIT|MM_DEF_LSVD); /* mdef lifesaved */
					else
						return (MM_HIT | MM_DEF_DIED | ((youagr || grow_up(magr, mdef)) ? 0 : MM_AGR_DIED));
				}
				else {
					/* aggressor lifesteals by dmg dealt */
					heal(magr, min(dmg, *hp(mdef)));
				}
			}
		}
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);

	case AD_CNFT:
		{
		static boolean in_conflict = FALSE;

		if (in_conflict)
		{
			/* avoid infinite loops; make a basic melee attack */
			alt_attk.adtyp = AD_PHYS;
			return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, dohitmsg, dmg, dieroll, vis, ranged);
		}
		else
		{
			struct monst *tmpm, *nmon;

			in_conflict = TRUE;
			/* fancy hitmsg */
			if (vis && dohitmsg) {
				pline("%s %s out, and conflict surrounds %s.",
					(youagr ? "You" : Monnam(magr)),
					(youagr ? "reach" : "reaches"),
					(youdef ? "you" : mon_nam(mdef))
					);
			}
			/* player-only: abuse CHA */
			if (youdef)
				exercise(A_CHA, FALSE);
			/* you are made to attempt to attack the target, but only in melee (for sanity's sake) */
			if (dist2(u.ux, u.uy, x(mdef), y(mdef)) <= 2 && !youagr && !youdef) {
				result = xattacky(&youmonst, mdef, x(mdef), y(mdef));
				/* possibly return early if def died */
				if (result&(MM_DEF_DIED | MM_DEF_LSVD))
				{
					in_conflict = FALSE;
					return result;
				}
			}
			/* all monsters on the level attempt to attack the target */
			for (tmpm = fmon; tmpm; tmpm = nmon){
				nmon = tmpm->nmon;
				if (!DEADMONSTER(tmpm) && tmpm != mdef && tmpm != magr)
					result = xattacky(tmpm, mdef, x(mdef), y(mdef));
				/* possibly return early if def died */
				if (result&(MM_DEF_DIED|MM_DEF_LSVD))
				{
					in_conflict = FALSE;
					return result;
				}
			}
			in_conflict = FALSE;
		}
		}// close scope for in_conflict
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);
		break;
	case AD_LUCK:
		//Use encouragement code to give monster target a -1
		if (!youdef){
			if(vis&VIS_MAGR && magr->mtyp == PM_LUCKSUCKER){
				pline("%s sucks %s luck!", Monnam(magr), s_suffix(mon_nam(mdef)));
				magr->encouraged = min(magr->encouraged+1,+13);
			}
			mdef->encouraged = max(mdef->encouraged-1,-13);
		}
		else {
			/* assumes you are defending */
			if(magr->mtyp == PM_LUCKSUCKER)
				pline("%s sucks your luck!", Monnam(magr));

			/* misc protections */
			if ((uwep && !uwep->cursed && confers_luck(uwep)) ||
				(stone_luck(TRUE) > 0 && rn2(4))) {
				pline("Luckily, you are not affected.");
			}
			else {
				int old_luck = u.uluck;
				You_feel("your luck running out.");
				change_luck(-1 * dmg);
				if(magr->mtyp == PM_LUCKSUCKER)
					magr->mvar_lucksucker += old_luck - u.uluck;
			}
			stop_occupation();
		}
		alt_attk.adtyp = AD_PHYS;
		return xmeleehurty(magr, mdef, &alt_attk, originalattk, weapon_p, FALSE, dmg, dieroll, vis, ranged);
		break;
//////////////////////////////////////////////////////////////
// NOT IMPLEMENTED FOR XMELEEHURTY
//////////////////////////////////////////////////////////////
		/* retaliatory only, AT_NONE */
	case AD_BARB:
	case AD_HLBD:
	case AD_UNKNWN:
		/* explosion only, AT_EXPL */
	case AD_FNEX:
		/* on-death only, AT_BOOM */
	case AD_SPNL:
	case AD_GARO:
	case AD_GARO_MASTER:
		/* on-death only, AT_NONE */
	case AD_GROW:
	case AD_SOUL:
	case AD_KAOS:
	case AD_MAND:
		/* engulf only, AT_ENGL */
	case AD_DGST:
	case AD_ILUR:
		/* spell only, AT_MAGC or AT_MMGC */
	case AD_CLRC:
	case AD_SPEL:
		/* gaze only, AT_GAZE or AT_WDGZ */
	case AD_SSUN:
	case AD_SPOR:
	case AD_BLNK:
	case AD_WTCH:
	case AD_RGAZ:
	case AD_MIST:
	case AD_DEAD:
	case AD_CNCL:
		/* breath only, AT_BREA */
	case AD_GOLD:
		/* tinker only, AT_TNKR */
	case AD_TNKR:
		/* AT_ARRW specifiers */
	case AD_SLVR:
	case AD_BALL:
	case AD_BLDR:
	case AD_VBLD:
	case AD_SURY:
	case AD_LOAD:
	case AD_SOLR:
		/* missiles (completely unused) */
	case AD_CMSL:
	case AD_FMSL:
	case AD_EMSL:
	case AD_SMSL:
		/* other deprecated or unusued damage types */
	case AD_NTZC:
	case AD_LSEX:
	case AD_BIST:
	case AD_DUNSTAN:
	case AD_WMTG:
	case AD_JAILER:
		impossible("Attack type not coded for xmeleehurty (%d)!", attk->adtyp);
		break;
	default:
		impossible("Bad attack type (%d)!", attk->adtyp);
	}
	impossible("Reached end of xmeleehurty -- report this to Nero on ##nethack-variants.");
	return MM_MISS;
}

int
xcastmagicy(struct monst *magr, struct monst *mdef, struct attack *attk, int vis, int i)
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct permonst * pa = youagr ? youracedata : magr->data;
	boolean can_target;
	int result = MM_MISS;
	int distance;
	int range;
	int tarx = (youdef ? magr->mux : x(mdef));
	int tary = (youdef ? magr->muy : y(mdef));

	/* find distance between magr and mdef */
	distance = dist2(
		x(magr),
		y(magr),
		(youdef ? magr->mux : x(mdef)),
		(youdef ? magr->muy : y(mdef))
		);

	/* determine spellcasting range of attacker */
	if (is_orc(pa) ||
		pa->mtyp == PM_HEDROW_WARRIOR ||
		pa->mtyp == PM_MINOTAUR_PRIESTESS
		)
		range = (BOLT_LIM*BOLT_LIM) / 2;
	else if (pa->mtyp == PM_CHROMATIC_DRAGON || pa->mtyp == PM_PLATINUM_DRAGON)
		range = 2;
	else
		range = (BOLT_LIM*BOLT_LIM);

	/* determine if magr can target mdef */
	if ((clear_path(x(magr), y(magr), tarx, tary)) &&	/* clear line of sight */
		dist2(x(magr), y(magr), tarx, tary) <= range)	/* within range */
		can_target = TRUE;
	else
		can_target = FALSE;

	if (can_target)
		result = xcasty(magr, mdef, attk, tarx, tary, i);
	else if (attk->adtyp == AD_SPEL || attk->adtyp == AD_CLRC || attk->adtyp == AD_PSON)
		result = xcasty(magr, (struct monst *)0, attk, 0, 0, i);
	else
		result = MM_MISS;	/* nothing to cast */

	return result;
}


/* tinker, mvu only right now */
int
xtinkery(magr, mdef, attk, vis)
struct monst * magr;
struct monst * mdef;
struct attack * attk;
int vis;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct permonst * pa = youagr ? youracedata : magr->data;
	struct permonst * pd = youdef ? youracedata : mdef->data;
	int result = MM_MISS;
	
	/* mvm behaviour is undefined right now -- do nothing */
	if (!youagr && !youdef)
		return MM_MISS;

	int tarx = (youdef ? magr->mux : x(mdef));
	int tary = (youdef ? magr->muy : y(mdef));

	/* only mvu behaviour is implemented */
	if (youdef) {
		/* tinkering requires mspec available */
		if (!magr->mspec_used) {
			struct monst *mlocal;
			int dx = 0, dy = 0, i;
			int monid;
			int mmflags = (MM_ADJACENTOK|MM_ADJACENTSTRICT|MM_NOCOUNTBIRTH);
			if (get_mx(magr, MX_ESUM)) mmflags |= MM_ESUM;

			/* get dx, dy */
			if		(tarx - x(magr) < 0) dx = -1;
			else if (tarx - x(magr) > 0) dx = +1;
			if		(tary - y(magr) < 0) dy = -1;
			else if (tary - y(magr) > 0) dy = +1;

			/* get monid */
			if (magr->mtyp == PM_HOOLOOVOO){
				if (rn2(4))
					monid = PM_GOLDEN_HEART;
				else
					monid = PM_ID_JUGGERNAUT;
			}
			else if (magr->mtyp == PM_PORO_AULON){
				monid = PM_FABERGE_SPHERE;
			}
			else {
				if (mlev(mdef) > 10 && !rn2(10))
					monid = PM_FIREWORK_CART;
				else if(Is_minetown_level(&u.uz))
					monid = rn2(2) ? PM_CLOCKWORK_SOLDIER : PM_FABERGE_SPHERE;
				else
					monid = PM_CLOCKWORK_SOLDIER + rn2(3);
			}
			/* make the monster */
			mlocal = makemon(&mons[monid],
				x(magr) + dx,
				y(magr) + dy,
				mmflags);

			/* set the creation's direction */
			if (mlocal){
				for (i = 0; i<8; i++) if (xdir[i] == dx && ydir[i] == dy) break;
				mlocal->mvar_vector = i;

				magr->mspec_used = rnd(6);
				result = MM_HIT;

				if (mmflags&MM_ESUM)
					mark_mon_as_summoned(mlocal, magr, ESUMMON_PERMANENT, 0);
			}
			else
				result = MM_MISS;
		}
		else
			result = MM_MISS;
		magr->mflee = 1;
	}
	return result;
}

int
xengulfhity(magr, mdef, attk, vis)
struct monst * magr;
struct monst * mdef;
struct attack * attk;
int vis;
{
	/* don't attempt to eat your steed out from under you */
	if (mdef == u.usteed)
		mdef = &youmonst;
	
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct permonst * pa = youagr ? youracedata : magr->data;
	struct permonst * pd = youdef ? youracedata : mdef->data;
	int result = MM_MISS;
	struct trap *trap = t_at(x(mdef), y(mdef));
	struct obj * otmp;
	int dmg = d((int)attk->damn, (int)attk->damd);

	if (youdef) {
		/* swallows you */
		if (!u.uswallow) {
			boolean pluck = FALSE;
			if ((trap && ((trap->ttyp == PIT) || (trap->ttyp == SPIKED_PIT))) &&
				boulder_at(u.ux, u.uy))
				return MM_MISS;

			/* remove ball and chain */
			if (Punished)
				unplacebc();

			/* do we move onto player, or pluck them? */
			if (stationary_mon(magr) || (is_animal(pa) && u.usteed)) {
				pluck = TRUE;
			}
			/* maybe move attacker */
			if (!pluck) {
				remove_monster(x(magr), y(magr));
				newsym(x(magr), y(magr));
				magr->mtrapped = 0;		/* no longer on old trap */
				place_monster(magr, u.ux, u.uy);
				newsym(x(magr), y(magr));
			}
			else {
				/* or maybe pluck the player */
				u.ux0 = u.ux;
				u.uy0 = u.uy;
			}
			u.ustuck = magr;
#ifdef STEED
			if (pluck && u.usteed) {
				/* Too many quirks presently if hero and steed
				* are swallowed. Pretend purple worms don't
				* like horses for now :-)
				*/
				pline("%s lunges forward and plucks you off %s!",
					Monnam(magr), mon_nam(u.usteed));
				dismount_steed(DISMOUNT_ENGULFED);
			}
			else
#endif
				pline("%s engulfs you!", Monnam(magr));

			if (pluck) {
				/* move player -- this must come after dismount_steed */
				u.ux = x(magr);
				u.uy = y(magr);
			}
			stop_occupation();
			reset_occupations();	/* behave as if you had moved */

			if (u.utrap) {
				You("are released from the %s!",
					u.utraptype == TT_WEB ? "web" : u.utraptype == TT_SALIVA ? "saliva" : "trap");
				u.utrap = 0;
			}

			int i = number_leashed();
			if (i > 0) {
				const char *s = (i > 1) ? "leashes" : "leash";
				pline_The("%s %s loose.", s, vtense(s, "snap"));
				unleash_all();
			}

			if (touch_petrifies(youracedata) && !resists_ston(magr)) {
				expels(magr, pa, FALSE);
				minstapetrify(magr, TRUE);
				if (magr->mhp > 0)
					return MM_AGR_STOP;
				else
					return MM_AGR_DIED;
			}

			display_nhwindow(WIN_MESSAGE, FALSE);
			vision_recalc(2);	/* hero can't see anything */
			u.uswallow = 1;

			/* for digestion, shorter time is more dangerous;
			for other swallowings, longer time means more
			chances for the swallower to attack */
			int tim_tmp;
			if (attk->adtyp == AD_DGST) {
				tim_tmp = 25 - mlev(magr);
				if (tim_tmp > 0)
					tim_tmp = rnd(tim_tmp) / 2;
				else if (tim_tmp < 0)
					tim_tmp = -(rnd(-tim_tmp) / 2);
				/* having good armor & high constitution makes
				it take longer for you to be digested, but
				you'll end up trapped inside for longer too */
				tim_tmp += -u.uac + 10 + (ACURR(A_CON) / 3 - 1);
				if(magr->mtyp == PM_NAMELESS_GNAWER){
					int nid = u.ualign.sins + u.uimpurity;
					if(u.ualign.record < 0)
						nid -= u.ualign.record; //Increases score
					if(u.ualign.record >= 20)
						tim_tmp += 1;
					for (; nid > 0 && tim_tmp > 2; nid/=2){//Digests faster
						tim_tmp--;
					}
					
				}
				if(thick_skinned(youracedata) || u.sealsActive&SEAL_ECHIDNA)
					tim_tmp += 2;
			}
			else {
				/* higher level attacker takes longer to eject hero */
				tim_tmp = rnd((int)mlev(magr) + 10 / 2);
			}
			/* u.uswldtim always set > 1 */
			u.uswldtim = (unsigned)((tim_tmp < 2) ? 2 : tim_tmp);
			swallowed(1);

			/* snuff player's carried lightsources */
			if (!is_whirly(u.ustuck->data))
				for (otmp = invent; otmp; otmp = otmp->nobj)
						(void)snuff_lit(otmp);
		}
		/* player should be swallowed now */
		if (magr != u.ustuck)
			return MM_MISS;
		/* decrement swallow time remaining */
		if (u.uswldtim > 0)
			u.uswldtim -= 1;
		/* deal damage and other effects */
		result = xengulfhurty(magr, mdef, attk, vis);

		/* after engulfing, possibly regurgitate you */
		if (touch_petrifies(youracedata) && !resists_ston(magr)) {
			pline("%s very hurriedly %s you!", Monnam(magr),
				is_animal(pa) ? "regurgitates" : "expels");
			expels(magr, pa, FALSE);
		}
		else if (!u.uswldtim || youracedata->msize >= MZ_HUGE) {
			You("get %s!", is_animal(pa) ? "regurgitated" : "expelled");
			if (flags.verbose && (is_animal(pa) ||
				(dmgtype(pa, AD_DGST) && Slow_digestion)))
				pline("Obviously %s doesn't like your taste.", mon_nam(magr));
			expels(magr, pa, FALSE);
		}
		else if (!dmgtype_fromattack(pa, attk->adtyp, attk->aatyp)) {
			/* engulf attack isn't natural to the monster */
			pline("%s can't seem to keep you %s.", Monnam(magr), is_animal(pa) ? "down" : "in");
			delay_output();
			delay_output();
			expels(magr, pa, FALSE);
		}
	}
	else {
		boolean did_tmp_at = FALSE;
		/* message */
		if (youagr) {
			You("%s %s!",
				(is_animal(pa) ? "swallow" : "engulf"),
				mon_nam(mdef));
		}
		else if (vis) {
			pline("%s %s %s.",
				Monnam(magr),
				(is_animal(pa) ? "swallows" : "engulfs"),
				mon_nam(mdef));
		}
		/* snuff mdef's carried lightsources */
		if (!is_whirly(pa))
			for (otmp = mdef->minvent; otmp; otmp = otmp->nobj)
				(void)snuff_lit(otmp);
		
		/* visually move the agressor over defender */
		if (youagr ? (!Invisible) : canspotmon(magr)) {
			map_location(x(magr), y(magr), TRUE);
			/* SCOPECREEP: get the correct glyph for pets/peacefuls/zombies/detected/etc */
			tmp_at(DISP_FLASH, mon_to_glyph(magr));
			tmp_at(x(mdef), y(mdef));
			did_tmp_at = TRUE;
			delay_output();
			delay_output();
		}
		/* deal damage and other effects */
		result = xengulfhurty(magr, mdef, attk, vis);
		/* check for this fun case: mdef dies, causing an explosion, which kills magr */
		if (DEADMONSTER(magr)) result |= MM_AGR_DIED;

		/* if defender died and aggressor isn't stationary, move agressor to defender's coord */
		/* if mdef was your steed, you are still there, so magr can't take your spot! */
		if (!stationary_mon(magr) && result&MM_DEF_DIED
#ifdef STEED
			&& !(mdef == u.usteed)
#endif
			) {
			/* sanity check */
			if (*hp(mdef) > 0)
				impossible("dead engulfee still alive?");

			if (youagr) {
				remove_monster(x(mdef), y(mdef));
				teleds(x(mdef), y(mdef), FALSE);
			}
			else {
				remove_monster(x(magr), y(magr));
				remove_monster(x(mdef), y(mdef));
				if (!(result & MM_AGR_DIED))
					place_monster(magr, x(mdef), y(mdef));
			}
		}
		/* remap the agressor's old location */
		if (did_tmp_at) {
			tmp_at(DISP_END, 0);
		}
		if (youagr ? (!Invisible) : canspotmon(magr)) {
			newsym(x(magr), y(magr));
		}
	}

	return result;
}

int
xengulfhurty(magr, mdef, attk, vis)
struct monst * magr; /* May be NULL if being used for generic cloud damage handling (engulfed by a cloud) */
struct monst * mdef;
struct attack * attk;
int vis;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct permonst * pa = magr ? (youagr ? youracedata : magr->data) : (struct permonst *) 0;
	struct permonst * pd = youdef ? youracedata : mdef->data;
	int result = MM_MISS;
	int dmg = d(attk->damn, attk->damd);
	int fulldmg = dmg;
	static char buf[BUFSZ]; /* Used to hold variable messages to be displayed later */
	struct obj * otmp;

	/* apply half-phys */
	dmg = reduce_dmg(mdef,dmg,TRUE,FALSE);

	switch (attk->adtyp)
	{
		/* the most important and most special case */
	case AD_DGST:
		if (youdef) {
			if (pa && pa->mtyp == PM_METROID_QUEEN && !Drain_resistance) {
				losexp("life force drain", TRUE, FALSE, FALSE);
				magr->mhpmax += d(1, (hd_size(youracedata)+1)/2);
				magr->mhp += d(1, hd_size(youracedata));
				if (magr->mhp > magr->mhpmax) magr->mhp = magr->mhpmax;
				if (magr->mtame){
					EDOG(magr)->hungrytime += 100;  //400/4 = human nut/4
				}
			}
			if (Slow_digestion || is_indigestible(youracedata)) {
				/* Messages are handled in caller */
				u.uswldtim = 0;
				result = MM_HIT;
			}
			else if (u.uswldtim == 0) {
				pline("%s totally digests you!", Monnam(magr));
				result = xdamagey(magr, mdef, attk, FATAL_DAMAGE_MODIFIER);
			}
			else {
				*hp(mdef) -= *hp(mdef)/(u.uswldtim+1); //Floored and current HP, so never fatal. 
				pline("%s%s digests you!", magr ? Monnam(magr): "Something",
					(u.uswldtim == 2) ? " thoroughly" :
					(u.uswldtim == 1) ? " utterly" : "");
				exercise(A_STR, FALSE);
				result = MM_HIT;
			}
		}
		else if (youagr) {
			/* eating a Rider or its corpse is fatal */
			if (is_rider(mdef->data)) {
				pline("Unfortunately, digesting any of it is fatal.");
				Sprintf(buf, "unwisely tried to eat %s",
					mdef->data->mname);
				killer = buf;
				killer_format = NO_KILLER_PREFIX;
				done(DIED);
				result = MM_AGR_STOP;		/* lifesaved */
			}
			else if(is_indigestible(mdef->data)){
				You("regurgitate %s.", mon_nam(mdef));
				dmg = 0;
				break;
			}
			else if(is_delouseable(mdef->data)){
				mdef = delouse(mdef, AD_DGST);
				You("regurgitate %s.", mon_nam(mdef));
				dmg = 0;
				result |= MM_AGR_STOP;
				break;
			}
			else {
				int resist = -1*full_marmorac(mdef) + 10;
				resist += mdef->data->nac;
				if(!mdef->mcan)
					resist += mdef->data->pac;
				if(thick_skinned(pd))
					resist += 2;
				if(resist > 0){
					if(rnd(20) < resist){
						You("regurgitate %s.", mon_nam(mdef));
						dmg = 0;
						break;
					}
					//else continue
				}
				if (Slow_digestion) {
					dmg = 0;
					break;
				}

				/* KMH, conduct */
				u.uconduct.food++;
				if (!vegan(pd))
					u.uconduct.unvegan++;
				if (!vegetarian(pd))
					violated_vegetarian();

				newuhs(FALSE);

				/* kludge to hide monster from view, to avoid seeing messages about its lifesaving */
				char cantseethis = (viz_array[y(mdef)][x(mdef)] & (IN_SIGHT | COULD_SEE));
				viz_array[y(mdef)][x(mdef)] &= ~(IN_SIGHT | COULD_SEE);
				xkilled(mdef, 2);
				viz_array[y(mdef)][x(mdef)] |= cantseethis;

				if (*hp(mdef) > 0) { /* monster lifesaved */
					You("hurriedly regurgitate the sizzling in your %s.",
						body_part(STOMACH));
					result |= MM_DEF_LSVD;
				}
				else {
					result |= MM_DEF_DIED;
					int digest_time = 1 + (pd->cwt >> 8);
					if (corpse_chance(mdef, &youmonst, TRUE) &&
						!((mvitals[monsndx(pd)].mvflags & G_NOCORPSE)
							|| get_mx(mdef, MX_ESUM))) {
						/* nutrition only if there can be a corpse */
						if (Race_if(PM_INCANTIFIER)) u.uen += mlev(mdef);
						else u.uhunger += (pd->cnutrit + 1) / 2;
					}
					else digest_time = 0;
					Sprintf(buf, "You totally digest %s.",
						mon_nam(mdef));
					if (digest_time != 0) {
						/* setting afternmv = end_engulf is tempting,
						* but will cause problems if the player is
						* attacked (which uses his real location) or
						* if his See_invisible wears off
						*/
						You("digest %s.", mon_nam(mdef));
						if (Slow_digestion) digest_time *= 2;
						nomul(-digest_time, "digesting a victim");
						nomovemsg = buf;
					}
					else pline("%s", buf);
					if (pd->mtyp == PM_GREEN_SLIME || pd->mtyp == PM_FLUX_SLIME) {
						Sprintf(buf, "%s isn't sitting well with you.",
							The(pd->mname));
						if (!Unchanging && !GoodHealth) {
							Slimed = 5L;
							flags.botl = 1;
						}
					}
					else
						exercise(A_CON, TRUE);
				}
			}
		}
		else { //mvm
			/* eating a Rider or its corpse is fatal */
			if (is_rider(mdef->data) && magr) {
				if (vis)
					pline("%s %s!", Monnam(magr),
					mdef->mtyp == PM_FAMINE ?
					"belches feebly, shrivels up and dies" :
					mdef->mtyp == PM_PESTILENCE ?
					"coughs spasmodically and collapses" :
					"vomits violently and drops dead");
				mondied(magr);
				if (magr->mhp > 0) {
					result = MM_AGR_STOP;	/* lifesaved */
				}
				else {
					if (magr->mtame && !vis)
						You("have a queasy feeling for a moment, then it passes.");
					result = MM_AGR_DIED;
				}
			}
			else if(is_indigestible(mdef->data)){
				if(magr && (canspotmon(magr) || canspotmon(mdef))){
					pline("%s regurgitates %s.", Monnam(magr), mon_nam(mdef));
				}
				dmg = 0;
				break;
			}
			else if(is_delouseable(mdef->data)){
				mdef = delouse(mdef, AD_DGST);
				if(magr && (canspotmon(magr) || canspotmon(mdef))){
					pline("%s regurgitates %s.", Monnam(magr), mon_nam(mdef));
				}
				dmg = 0;
				result |= MM_AGR_STOP;
				break;
			}
			else {
				int resist = -1*full_marmorac(mdef) + 10;
				resist += mdef->data->nac;
				if(!mdef->mcan)
					resist += mdef->data->pac;
				if(thick_skinned(pd))
					resist += 2;
				if(resist > 0){
					if(resist >= 20)
						dmg = 0;
					else
						dmg = mdef->mhpmax*(20-resist)/20;
					if(dmg < mdef->mhp){
						mdef->mhp -= dmg;//Will not kill defender
						if(magr && (canspotmon(magr) || canspotmon(mdef))){
							pline("%s regurgitates %s.", Monnam(magr), mon_nam(mdef));
						}
						dmg = 0;
						break;
					}
					//else continue
				}
				if (flags.verbose && flags.soundok)
					verbalize("Burrrrp!");

				/* kill */
				/* kludge to hide monster from view, to avoid seeing messages about its lifesaving */
				char cantseethis = (viz_array[y(mdef)][x(mdef)] & (IN_SIGHT | COULD_SEE));
				viz_array[y(mdef)][x(mdef)] &= ~(IN_SIGHT | COULD_SEE);
				result = xdamagey(magr, mdef, attk, FATAL_DAMAGE_MODIFIER);
				viz_array[y(mdef)][x(mdef)] |= cantseethis;

				/* if they survivied, by some miracle, end */
				if (result&MM_DEF_LSVD) {
					break;
				}

				/* Is a corpse for nutrition possible?  It may kill magr */
				if (magr && !corpse_chance(mdef, magr, TRUE)) {
					break;
				}
				if (magr && *hp(magr) < 1) {
					result |= MM_AGR_DIED;
					break;
				}
				/* Pets get nutrition from swallowing monster whole.
				* No nutrition from G_NOCORPSE monster, eg, undead.
				* DGST monsters don't die from undead corpses
				*/
				int num = monsndx(mdef->data);
				if (magr && get_mx(magr, MX_EDOG) && 
					!((mvitals[num].mvflags & G_NOCORPSE) || get_mx(mdef, MX_ESUM))) {
					struct obj *virtualcorpse = mksobj(CORPSE, MKOBJ_NOINIT);
					int nutrit;

					virtualcorpse->corpsenm = num;
					virtualcorpse->owt = weight(virtualcorpse);
					nutrit = dog_nutrition(magr, virtualcorpse);
					dealloc_obj(virtualcorpse);

					/* only 50% nutrition, 25% of normal eating time */
					if (magr->meating > 1) magr->meating = (magr->meating + 3) / 4;
					if (nutrit > 1) nutrit /= 2;
					EDOG(magr)->hungrytime += nutrit;
				}
			}
		}
		break;

	case AD_WET:
		/* print hitmessage */
		if (youdef)
			You("are soaking wet!");
		else if (vis&VIS_MDEF)
			pline("%s is soaking wet!", Monnam(mdef));
		/* water/lethe damage */
		water_damage((youdef ? invent : mdef->minvent), FALSE, FALSE, FALSE, mdef);
		result = MM_HIT;
		break;
	case AD_LETHE:
		/* very asymetric effects */
		if (youdef) {
			pline("The waters of the Lethe wash over you!");
			if (u.sealsActive&SEAL_HUGINN_MUNINN){
				unbind(SEAL_HUGINN_MUNINN, TRUE);
			}
			else {
				(void)adjattrib(A_INT, -dmg, FALSE);
				forget(25);
				water_damage(invent, FALSE, FALSE, TRUE, &youmonst);

				exercise(A_WIS, FALSE);
			}
			if (ABASE(A_INT) <= 3) {
				int lifesaved = 0;
				struct obj *wore_amulet = uamul;

				while (1) {
					/* avoid looping on "die(y/n)?" */
					if (lifesaved && (discover || wizard)) {
						if (wore_amulet && !uamul) {
							/* used up AMULET_OF_LIFE_SAVING; still
							subject to dying from brainlessness */
							wore_amulet = 0;
						}
						else {
							/* explicitly chose not to die;
							arbitrarily boost intelligence */
							ABASE(A_INT) = ATTRMIN(A_INT) + 2;
							You_feel("like a scarecrow.");
							break;
						}
					}
					if (lifesaved)
						pline("Unfortunately your mind is still gone.");
					else
						Your("last thought drifts away.");
					killer = "memory loss";
					killer_format = KILLED_BY;
					done(DIED);
					lifesaved++;
					result |= MM_DEF_LSVD;
				}
			}
		}
		else {
			if (vis&VIS_MDEF)
				pline("%s is soaking wet!", Monnam(mdef));
			water_damage(mdef->minvent, FALSE, FALSE, TRUE, mdef);
		}
		result |= MM_HIT;
		break;
	case AD_BLND:
		if (can_blnd(magr, mdef, attk->aatyp, (struct obj*)0)) {
			if (youdef) {
			
				if (uarmh && uarmh->otyp == SHEMAGH && magr && 
					(magr->mtyp == PM_DUST_VORTEX || magr->mtyp == PM_SINGING_SAND || magr->mtyp == PM_PYROCLASTIC_VORTEX))
				{
					pline("The %s protects you from the dust!", simple_typename(uarmh->otyp));
				} else if (!Blind) {
					You_cant("see in here!");
					make_blinded((long)dmg, FALSE);
					if (!Blind)
						Your1(vision_clears);
				}
				else
					/* keep player blind until disgorged */
					make_blinded(Blinded + 1, FALSE);
			}
			else {
				struct obj *otmp = which_armor(mdef, W_ARMH);
				
				if (otmp && otmp->otyp == SHEMAGH && magr && (magr->mtyp == PM_DUST_VORTEX || magr->mtyp == PM_SINGING_SAND)){
					if (vis&VIS_MDEF)
						pline("The %s protects %s from the dust!", simple_typename(otmp->otyp), Monnam(mdef));
				} else {
					if (vis&VIS_MDEF && mdef->mcansee)
						pline("%s can't see in there!", Monnam(mdef));
					mdef->mcansee = 0;
					mdef->mblinded = min(127, dmg + mdef->mblinded);
				}
			}
		}
		result = MM_HIT;
		break;
	case AD_STDY:
		if (youdef) {
			if (dmg > u.ustdy) {
				You("are studied from all directions!");
				u.ustdy = dmg;
			}
		}
		else {
			/* no message? */
			if (dmg > mdef->mstdy)
				mdef->mstdy = dmg;
		}
		break;
		/* Illurien's unique engulf attack */
	case AD_ILUR:
		/* undo half-phys-damage; this is quite magical */
		dmg = fulldmg;
		
		if (has_blood(pd) || !is_anhydrous(pd)) {
			dmg *= 2;
			pline("A mist of %s from %s %s and swept into the cloud!",
				(has_blood(pd) ? "blood is torn" : "water is drawn"),
				(youdef ? "your" : s_suffix(mon_nam(mdef))),
				(youdef ? body_part(BODY_SKIN) : mbodypart(mdef, BODY_SKIN))
				);
			if (youdef) {
				if (u.sealsActive&SEAL_HUGINN_MUNINN){
					unbind(SEAL_HUGINN_MUNINN, TRUE);
				}
				else {
					int perc = (*hpmax(mdef) - (*hp(mdef) - dmg)) * 10 / (*hpmax(mdef));
					if (perc > 10) perc = 10;
					if (perc > 0) forget(perc);
				}
			}
			result = xdamagey(magr, mdef, attk, dmg);
		}
		else {
			if (is_iron(pd)) {
				if (youdef) {
					You("are laden with moisture and rust away!");
					/* KMH -- this is okay with unchanging */
					rehumanize();
					change_gevurah(1); //cheated death.
					result = MM_DEF_LSVD;
					if(magr){
						magr->mhp = 1;
						expels(magr, pa, FALSE);
						pline("Rusting your iron body took a severe toll on the cloud!");
					}
				}
				else {
					if (vis&VIS_MDEF) {
						pline("%s falls to pieces!",
							Monnam(mdef));
					}
					if (youagr)
						killed(mdef);
					else
						mondied(mdef);

					if (mdef->mhp > 0)
						result = MM_DEF_LSVD;
					else {
						if (mdef->mtame && !vis && !youagr)
							pline("May %s rust in peace.", mon_nam(mdef));
						result = (MM_HIT | MM_DEF_DIED);
						if(magr)
							result |= ((youagr || grow_up(magr, mdef)) ? 0 : MM_AGR_DIED);
					}
				}
			}
			else {
				if (youdef) {
					You("are laden with moisture and %s",
						flaming(youracedata) ? "are smoldering out!" :
						Breathless ? "find it mildly uncomfortable." :
						amphibious(youracedata) ? "feel comforted." :
						"can barely breathe!");
					/* NB: Amphibious includes Breathless */
					if (Amphibious && !flaming(youracedata)) dmg = 0;
					else exercise(A_CON, FALSE);
				}
				else {
					if (vis&VIS_MDEF) {
						pline("%s is laden with moisture.",
							Monnam(mdef));
					}
					if (amphibious_mon(mdef) && !flaming(pd)) {
						dmg = 0;
						if (vis&VIS_MDEF)
							pline("%s seems unharmed.", Monnam(mdef));
					}
				}
				result = xdamagey(magr, mdef, attk, dmg);
			}
		}
		/* rust armor */
		if (youdef) {
			hurtarmor(AD_RUST, is_dnoble(pa));
		}
		else {
			hurtmarmor(mdef, AD_RUST, is_dnoble(pa));
		}
		break;
		/* basic damage engulf types */
	case AD_PHYS:
		if (pa && (pa->mtyp == PM_FOG_CLOUD || pa->mtyp == PM_MIST_CLOUD || pa->mtyp == PM_STEAM_VORTEX)) {
			if (youdef) {
				You("are laden with moisture and %s",
					flaming(youracedata) ? "are smoldering out!" :
					Breathless ? "find it mildly uncomfortable." :
					amphibious(youracedata) ? "feel comforted." :
					"can barely breathe!");
				/* NB: Amphibious includes Breathless */
				if (Amphibious && !flaming(youracedata)) dmg = 0;
				else exercise(A_CON, FALSE);
			}
			else {
				if (vis&VIS_MDEF) {
					pline("%s is laden with moisture.",
						Monnam(mdef));
				}
				if (amphibious_mon(mdef) &&	!flaming(pd)) {
					dmg = 0;
					if (vis&VIS_MDEF)
						pline("%s seems unharmed.", Monnam(mdef));
				}
			}
		}
		else if (pa && pa->mtyp == PM_DREADBLOSSOM_SWARM) {
			if (youdef) {
				You("are sliced by the whirling stems!");
				exercise(A_DEX, FALSE);
			}
			else if (vis&VIS_MDEF) {
				pline("%s is sliced by whirling stems!",
					Monnam(mdef));
			}
		}
		else if (pa && pa->mtyp == PM_LUMINESCENT_SWARM) {
			if (youdef) {
				You("are bitten and stung by the swarming insects!");
				exercise(A_STR, FALSE);
				exercise(A_DEX, FALSE);
				exercise(A_CON, FALSE);
			}
			else if (vis&VIS_MDEF) {
				pline("%s is bitten and stung by the swarming insects!",
					Monnam(mdef));
			}
		}
		else {
			if (youdef) {
				You("are pummeled with debris!");
				exercise(A_STR, FALSE);
			}
			else if (vis&VIS_MDEF) {
				pline("%s is pummeled with debris!",
					Monnam(mdef));
			}
		}
		/* deal damage (which can be 0 gracefully) */
		result = xdamagey(magr, mdef, attk, dmg);
		break;
	case AD_ACID:
	case AD_EACD:
		/* apply resistance */
		if (Acid_res(mdef)) {
			if (attk->adtyp == AD_EACD)
				dmg /= 2;
			else
				dmg = 0;
		}
		/* message, do special effects */
		if (dmg) {
			if (youdef) {
				if (Hallucination) pline("Ouch!  You've been slimed!");
				else You("are covered in acid! It burns!");
				exercise(A_STR, FALSE);
			}
			else if (vis&VIS_MDEF) {
				pline("%s is covered in acid!",
					Monnam(mdef));
			}
			if(!rn2(3))
				erode_armor(mdef, TRUE);
		}
		else {
			if (youdef) {
				You("are covered in seemingly harmless goo.");
			}
			else if (vis&VIS_MDEF) {
				pline("%s is covered in goo.",
					Monnam(mdef));
			}
		}
		/* set killer for cloud */
		if (!magr) {
			killer = "acidic cloud";
			killer_format = KILLED_BY_AN;
		}
		/* deal damage */
		result = xdamagey(magr, mdef, attk, dmg);
		break;
	case AD_ELEC:
	case AD_EELC:
		/* apply resistance */
		if (Shock_res(mdef)) {
			if (attk->adtyp == AD_EELC)
				dmg /= 2;
			else
				dmg = 0;
		}
		/* message, special effects */
		if (((attk->adtyp == AD_EELC) || (youagr || (!magr || !magr->mcan))) &&
			(rn2(2) || !youdef || !u.uswallow))
		{
			/* message */
			if (vis&VIS_MDEF) {
				pline_The("air around %s crackles with electricity.",
					(youdef ? "you" : mon_nam(mdef))
					);
				if (!dmg) {
					pline("%s %s unhurt.",
						(youdef ? "You" : Monnam(mdef)),
						(youdef ? "seem" : "seems")
						);
				}
			}
			/* destroy items */
			if (!UseInvShock_res(mdef)){
				if (magr ? mlev(magr) > rn2(20) : !rn2(4))
					destroy_item(mdef, WAND_CLASS, AD_ELEC);
				if (magr ? mlev(magr) > rn2(20) : !rn2(4))
					destroy_item(mdef, RING_CLASS, AD_ELEC);
			}
			/* golem effects */
			if (youdef)
				ugolemeffects(AD_ELEC, fulldmg);
			else
				golemeffects(mdef, AD_ELEC, fulldmg);
		}
		else
			dmg = 0;
		/* set killer for cloud */
		if (!magr) {
			killer = "shocking cloud";
			killer_format = KILLED_BY_AN;
		}
		/* deal damage */
		result = xdamagey(magr, mdef, attk, dmg);
		break;
	case AD_DREN:
		/* message */
		if (vis&VIS_MDEF) {
			pline_The("air around %s flashes with multicolored sparks.",
				(youdef ? "you" : mon_nam(mdef))
				);
		}
		if (youdef)
			drain_en(dmg);
		else
			mdef->mspec_used += (dmg+9)/10;
		break;
	case AD_COLD:
	case AD_ECLD:
		/* apply resistance */
		if (Cold_res(mdef)) {
			if (attk->adtyp == AD_ECLD)
				dmg /= 2;
			else
				dmg = 0;
		}
		/* message, special effects */
		if (((attk->adtyp == AD_ECLD) || (youagr || (!magr || !magr->mcan))) &&
			(rn2(2) || !youdef || !u.uswallow))
		{
			/* message */
			if (vis&VIS_MDEF) {
				if (dmg) {
					pline("%s %s freezing to death!",
						(youdef ? "You" : Monnam(mdef)),
						(youdef ? "are" : "is")
						);
				}
				else {
					pline("%s %s mildly chilly.",
						(youdef ? "You" : Monnam(mdef)),
						(youdef ? "feel" : "seems")
						);
				}
			}
			/* destroy items */
			if (!UseInvCold_res(mdef)){
				if (magr ? mlev(magr) > rn2(20): !rn2(4))
					destroy_item(mdef, POTION_CLASS, AD_COLD);
			}
			/* golem effects */
			if (youdef)
				ugolemeffects(AD_COLD, fulldmg);
			else
				golemeffects(mdef, AD_COLD, fulldmg);
		}
		else
			dmg = 0;
		/* set killer for cloud */
		if (!magr) {
			killer = "freezing cloud";
			killer_format = KILLED_BY_AN;
		}
		/* deal damage */
		result = xdamagey(magr, mdef, attk, dmg);
		break;
	case AD_FIRE:
	case AD_EFIR:
	case AD_ACFR:
		{ boolean holysear = FALSE, firedmg = TRUE;
		/* apply resistance/vulnerability */
		if (attk->adtyp == AD_ACFR) {
			if (youdef ? (hates_holy(pd)) : (hates_holy_mon(mdef)))
			{
				dmg *= 2;
				holysear = TRUE;
			}
		}
		if (Fire_res(mdef)) {
			if (attk->adtyp == AD_EFIR)
				dmg /= 2;
			else if (attk->adtyp == AD_ACFR && holysear) {
				firedmg = FALSE;
				dmg /= 2;
			}
			else {
				firedmg = FALSE;
				dmg = 0;
			}
		}
		/* message, special effects */
		/* This seems buggy mvm. They're really cold if YOU'RE swallowed! */
		if (((attk->adtyp == AD_EFIR || attk->adtyp == AD_ACFR) || (youagr || (!magr || !magr->mcan))) &&
			(rn2(2) || !youdef || !u.uswallow))
		{
			/* message */
			if (vis&VIS_MDEF) {
				if (dmg) {
					if (firedmg) {
						pline("%s %s burning to a crisp!",
							(youdef ? "You" : Monnam(mdef)),
							(youdef ? "are" : "is")
							);
					}
					if (holysear) {
						pline("%s %s seared by the holy flames!",
							(youdef ? "You" : Monnam(mdef)),
							(youdef ? "are" : "is")
							);
					}
				}
				else {
					pline("%s %s mildly hot.",
						(youdef ? "You" : Monnam(mdef)),
						(youdef ? "feel" : "seems")
						);
				}
			}
			/* destroy items */
			if (!UseInvFire_res(mdef)) {
				if (magr ? mlev(magr) > rn2(20) : !rn2(4))
					destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
				if (magr ? mlev(magr) > rn2(20): !rn2(4))
					destroy_item(mdef, POTION_CLASS, AD_FIRE);
				if (magr? mlev(magr) > rn2(25): !rn2(5))
					destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
			}
			/* golem effects */
			if (youdef)
				ugolemeffects(AD_FIRE, fulldmg);
			else
				golemeffects(mdef, AD_FIRE, fulldmg);
			/* burn away slime */
			if (youdef){
				burn_away_slime();
				melt_frozen_air();
			}
		}
		else
			dmg = 0;
		/* set killer for cloud */
		if (!magr) {
			killer = "flaming cloud";
			killer_format = KILLED_BY_AN;
		}
		/* deal damage */
		result = xdamagey(magr, mdef, attk, dmg);
		}
		break;
	case AD_DARK:
		/* apply resistance */
		if (Dark_res(mdef)) {
			dmg = 0;
		}
		else if(Dark_vuln(mdef)){
			dmg *= 2;
		}
		/* message, special effects */
		if (youagr || (!magr || !magr->mcan)){
			/* message */
			if (vis&VIS_MDEF) {
				if (dmg) {
					pline("%s %s engulfed in darkness!",
						(youdef ? "You" : Monnam(mdef)),
						(youdef ? "are" : "is")
						);
					if(youdef && Dark_vuln(mdef))
						You("are gripped by the fear of death!");
				}
			}
		}
		else
			dmg = 0;
		/* deal damage */
		result = xdamagey(magr, mdef, attk, dmg);
		break;
	case AD_DESC:
		/* apply resistance/vulnerability */
		if (nonliving(pd) || is_anhydrous(pd)){
			shieldeff(x(mdef), y(mdef));
			dmg = 0;
		}
		if (is_watery(pd))
			dmg *= 2;
		/* message, special effects */
		if (dmg > 0) {
			/* message */
			if (vis&VIS_MDEF) {
				pline("%s %s dehydrated!",
					(youdef ? "You" : Monnam(mdef)),
					(youdef ? "are" : "is")
					);
			}
			/* heal from damage dealt*/
			if(magr)
				heal(magr, min(*hp(mdef), dmg));
		}
		/* deal damage */
		result = xdamagey(magr, mdef, attk, dmg);
		break;
	case AD_DRST:
		/* unbreathing provides total immunity */
		if (youdef ? Breathless : breathless_mon(mdef))
			break;
		/* apply resistance */
		if (Poison_res(mdef)) {
			dmg = 0;
		}
		/* message */
		if (dmg) {
			if (youdef) {
				pline("%s is burning your %s!", Something, makeplural(body_part(LUNG)));
				You("cough and spit blood!");
			}
			else if (vis&VIS_MDEF) {
				pline("%s coughs!",
					Monnam(mdef));
			}
		}
		else {
			if (youdef) {
				You("cough!");
			}
		}
		/* blind defender (even if poison resistant) */
		if (youdef) {
			if (!Blind)
				make_blinded(1L, FALSE);
		}
		else {
			mdef->mcansee = 0;
			if (!mdef->mblinded)
				mdef->mblinded = 1;
		}
		/* deal damage */
		result = xdamagey(magr, mdef, attk, dmg);
		break;
	case AD_DISE:	/* damage/effect ? */
		if (youdef) {
			diseasemu(pa);
		}
		else {
			if (!Sick_res(mdef)) {
				if (vis&VIS_MDEF)
					pline("%s is afflicted by disease!", Monnam(mdef));
				if(!rn2(10)) dmg += 100;
			}
		}
		if(pa->mtyp == PM_JUIBLEX){
			if(!is_indigestible(youdef ? youracedata : mdef->data))
				dmg += *hp(mdef)/2;
			if(Sick_res(mdef))
				dmg /= 2;
			if(Acid_res(mdef))
				dmg /= 2;
		}
		/* deal damage */
		result = xdamagey(magr, mdef, attk, dmg);
		break;
		case AD_PYCL:{	/* Immediately recurse and return */
			struct attack alt_attk = *attk;
			switch(rnd(4)){
				case 1:
					alt_attk.adtyp = AD_FIRE;
				break;
				case 2:
					alt_attk.adtyp = AD_PHYS;
				break;
				case 3:
					alt_attk.adtyp = AD_DRST;
				break;
				case 4:
					alt_attk.adtyp = AD_BLND;
				break;
			}
			return xengulfhurty(magr, mdef, &alt_attk, vis);
		}break;
	}
	return result;
}

boolean
Curse_res(mon, verbose)
struct monst *mon;
boolean verbose;
{
	struct obj *otmp;
	static const char mal_aura[] = "feel a malignant aura surround %s.";
	if(mon == &youmonst){
		if (youracedata->mtyp == PM_PARASITIC_WALL_HUGGER){
			if(verbose) You(mal_aura, "your bloated body");
			return TRUE;
		}
		if (uamul && (uamul->otyp == AMULET_VERSUS_CURSES)) {
			if(verbose) You(mal_aura, "your amulet");
			return TRUE;
		} else if ((uarmc && (uarmc->otyp == PRAYER_WARDED_WRAPPING || uarmc->oartifact == ART_SPELL_WARDED_WRAPPINGS_OF_))
				|| (uarmg && (uarmg->oartifact == ART_WRAPPINGS_OF_THE_SACRED_FI))
		) {
			if(verbose) You(mal_aura, "your wrappings");
			return TRUE;
		} else if (uwep && (uwep->oartifact == ART_MAGICBANE)) {
			if(verbose) You(mal_aura, "the magic-absorbing blade");
			return TRUE;
		} else if (uwep && (uwep->oartifact == ART_STAFF_OF_NECROMANCY)) {
			if(verbose) You(mal_aura, "the skeletal staff");
			return TRUE;
		} else if (uwep && (uwep->oartifact == ART_TECPATL_OF_HUHETOTL)) {
			if(verbose) You(mal_aura, "the bloodstained dagger");
			return TRUE;
		} else if((uwep && (uwep->oartifact == ART_TENTACLE_ROD))
			|| (uswapwep && (uswapwep->oartifact == ART_TENTACLE_ROD))
		){
			if(verbose) You(mal_aura, "the languid tentacles");
			return TRUE;
		}
		for(otmp = invent; otmp; otmp=otmp->nobj){
			if(otmp->oartifact == ART_HELPING_HAND && (otmp->owornmask || rn2(20))){
				if(verbose){
					You_feel("as if you need some help.");
					You_feel("something lend you some help!");
				}
				return TRUE;
			}
			else if(check_oprop(otmp, OPROP_BCRS) && otmp->owornmask){
				if(verbose) You(mal_aura, the(xname(otmp)));
				return TRUE;
			}
		}
		if(uarm && check_oprop(uarm, OPROP_CGLZ)){
			if(verbose)
				You_feel("a malignant aura burn in the silver light.");
			return TRUE;
		}

		if(u.ukinghill){
			otmp = 0;
			for(otmp = invent; otmp; otmp=otmp->nobj)
				if(otmp->oartifact == ART_TREASURY_OF_PROTEUS)
					break;
			if(!otmp){
				impossible("Treasury not actually in inventory??");
			}
			else if(otmp->owornmask || !otmp->cursed || rn2(20)){
				if(verbose) You(mal_aura, "the cursed treasure chest");
				else if(otmp->blessed)
					unbless(otmp);
				else
					curse(otmp);
				update_inventory();	
				return TRUE;
			}
		}
	}
	else {
		static const char mons_item_mal_aura[] = "feel a malignant aura surround %s %s.";
		boolean visible = canseemon(mon);
		if (mon->mtyp == PM_PARASITIC_WALL_HUGGER){
			if (visible && verbose) You(mons_item_mal_aura, s_suffix(mon_nam(mon)), "bloated body");
			return TRUE;
		}
		if(has_template(mon, ILLUMINATED)){
			if(visible && verbose) You("feel a malignant aura burn away in the Light.");
			return TRUE;
		}
		
		if (which_armor(mon, W_AMUL) && (which_armor(mon, W_AMUL)->otyp == AMULET_VERSUS_CURSES)) {
			if (visible && verbose) You(mons_item_mal_aura, s_suffix(mon_nam(mon)), "amulet");
			return TRUE;
		}
		if (which_armor(mon, W_ARMC) && (which_armor(mon, W_ARMC)->otyp == PRAYER_WARDED_WRAPPING || which_armor(mon, W_ARMC)->oartifact == ART_SPELL_WARDED_WRAPPINGS_OF_)) {
			if (visible && verbose) You(mons_item_mal_aura, s_suffix(mon_nam(mon)), "wrappings");
			return TRUE;
		}
		if (MON_WEP(mon) &&
			(MON_WEP(mon)->oartifact == ART_MAGICBANE)) {
			if (visible && verbose) You(mons_item_mal_aura, s_suffix(mon_nam(mon)), "magic-absorbing blade");
			return TRUE;
		}
		if (MON_WEP(mon) &&
			(MON_WEP(mon)->oartifact == ART_STAFF_OF_NECROMANCY)) {
			if (visible && verbose) You(mons_item_mal_aura, s_suffix(mon_nam(mon)), "skeletal staff");
			return TRUE;
		}
		if (MON_WEP(mon) &&
			(MON_WEP(mon)->oartifact == ART_TECPATL_OF_HUHETOTL)) {
			if (visible && verbose) You(mons_item_mal_aura, s_suffix(mon_nam(mon)), "bloodstained dagger");
			return TRUE;
		}
		if ((MON_WEP(mon) && (MON_WEP(mon)->oartifact == ART_TENTACLE_ROD))
			|| (MON_SWEP(mon) && (MON_SWEP(mon)->oartifact == ART_TENTACLE_ROD))
		) {
			if (visible && verbose) You(mons_item_mal_aura, s_suffix(mon_nam(mon)), "languid tentacles");
			return TRUE;
		}
		//curse proof glaze
		if((otmp = which_armor(mon, W_ARM)) && check_oprop(otmp, OPROP_CGLZ) && arm_blocks_upper_body(otmp->otyp)){
			return TRUE;
		}

		for(otmp = mon->minvent; otmp; otmp=otmp->nobj)
			if(otmp->oartifact == ART_TREASURY_OF_PROTEUS)
				break;
		if(otmp && (otmp->owornmask || !otmp->cursed || rn2(20))){
			if (visible && verbose) You(mons_item_mal_aura, s_suffix(mon_nam(mon)), "cursed treasure chest");
			if(otmp->blessed)
				unbless(otmp);
			else
				curse(otmp);
			return TRUE;
		}
		for(otmp = mon->minvent; otmp; otmp=otmp->nobj){
			if(otmp->oartifact == ART_HELPING_HAND && (otmp->owornmask || rn2(20))){
				if (visible && verbose)
					You(mons_item_mal_aura, s_suffix(mon_nam(mon)), "helpful hand");
				return TRUE;
			}
			else if(check_oprop(otmp, OPROP_BCRS) && otmp->owornmask){
				if (visible && verbose)
					You(mons_item_mal_aura, s_suffix(mon_nam(mon)), the(xname(otmp)));
				return TRUE;
			}
		}
	}
	return FALSE;
}

int
xexplodey(magr, mdef, attk, vis)
struct monst * magr;
struct monst * mdef;
struct attack * attk;
int vis;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct permonst * pa = youagr ? youracedata : magr->data;
	struct permonst * pd = youdef ? youracedata : mdef->data;
	int result = MM_MISS;

	int dmg = d((int)attk->damn, (int)attk->damd);
	int fulldmg = dmg;			/* original unreduced damage */

	/* message */
	if (vis)
		xyhitmsg(magr, mdef, attk);
	else
		noises(magr, attk);

	/* directed at player, wild miss */
	if (youdef && (magr->mux != u.ux || magr->muy != u.uy)) {
		/* give message regardless of vis -- it was targeting the player */
		pline("%s explodes at a spot in %s!",
			Monnam(magr),
			(((int)levl[magr->mux][magr->muy].typ) == WATER ? "empty water" : "thin air")
			);
		/* skip most effects. Go to killing magr */
	}
	else {
		switch (attk->adtyp)
		{
		case AD_PHYS:
			if(magr->mtyp == PM_FABERGE_SPHERE){
				break;
			}
			else
				goto expl_common;
		case AD_FNEX:
			/* fern spores are extra special */
			/* need to die before their explosion, so that a new monster can be placed there */
			/* they also create real explosions */
			if(!youagr)
				mondead(magr);
			if (pa->mtyp == PM_SWAMP_FERN_SPORE)
				explode(x(magr), y(magr), AD_DISE, MON_EXPLODE, dmg, EXPL_MAGICAL, 1);
			else if (pa->mtyp == PM_BURNING_FERN_SPORE)
				explode(x(magr), y(magr), AD_PHYS, MON_EXPLODE, dmg, EXPL_YELLOW, 1);
			else
				explode(x(magr), y(magr), AD_ACID, MON_EXPLODE, dmg, EXPL_NOXIOUS, 1);
			/* players, on the other hand, shouldn't be rehumanized before the explosion (since it will hurt them too) */
			if (youagr && Upolyd){
				rehumanize();
				change_gevurah(1); //cheated death.
			}
			return (*hp(magr) > 0 ? MM_AGR_STOP : MM_AGR_DIED) | (*hp(mdef) > 0 ? MM_HIT : MM_DEF_DIED);

			/* special explosions */
		case AD_BLND:
			if (!resists_blnd(mdef)) {
				if (youdef) {
					/* sometimes you're affected even if it's invisible */
					if (mon_visible(magr) || (rnd(dmg /= 2) > u.ulevel)) {
						You("are blinded by a blast of light!");
						make_blinded((long)dmg, FALSE);
						if (!Blind) Your("%s", vision_clears);
					}
					else if (flags.verbose)
						You("get the impression it was not terribly bright.");
				}
				else {
					if (youagr && vis&VIS_MDEF) {
						pline("%s is blinded by your flash of light!", Monnam(mdef));
					}
					else if (vis&VIS_MDEF) {
						pline("%s is blinded.", Monnam(mdef));
					}
					mdef->mblinded = min((int)mdef->mblinded + dmg, 127);
					mdef->mcansee = 0;
				}
			}
			else {
				/* message */
				if (youdef) {
					You("seem unaffected.");
				}
				else if (vis&VIS_MDEF) {
					pline("%s seems unaffected.", Monnam(mdef));
				}
			}
			break;
		
		case AD_HALU:
			/* asymetric conditions for this, sadly */
			if (!hallucinogenic(pd) &&
				(youdef ? (!NoLightBlind) : (haseyes(pd) && mdef->mcansee))
				) {
				if (youdef) {
					boolean chg;
					if (!Hallucination)
						You("are caught in a blast of kaleidoscopic light!");
					chg = make_hallucinated(HHallucination + (long)dmg, FALSE, 0L);
					You("%s.", chg ? "are freaked out" : "seem unaffected");
					if (chg && Hallucination && magr->mtyp == PM_DAUGHTER_OF_BEDLAM){
						u.umadness |= MAD_DELUSIONS;
						change_usanity(-1*rnd(6), FALSE); //Deals sanity damage
					}
				}
				else {
					if (youagr && vis&VIS_MDEF) {
						pline("%s is affected by your flash of light!", Monnam(mdef));
					}
					else if (vis&VIS_MDEF) {
						pline("%s looks confused.", Monnam(mdef));
					}
					mdef->mconf = 1;
					mdef->mstrategy &= ~STRAT_WAITFORU;
				}
			}
			else {
				/* message */
				if (youdef) {
					You("seem unaffected.");
				}
				else if (vis&VIS_MDEF) {
					pline("%s seems unaffected.", Monnam(mdef));
				}
			}
			break;

			/* basic damage types */
		case AD_COLD:
			if (Cold_res(mdef))
				dmg = 0;
			goto expl_common;
		case AD_ECLD:
			if (Cold_res(mdef))
				dmg /= 2;
			goto expl_common;
		case AD_FIRE:
			if (Fire_res(mdef))
				dmg = 0;
			goto expl_common;
		case AD_EFIR:
			if (Fire_res(mdef))
				dmg /= 2;
			goto expl_common;
		case AD_ACFR:
			if (Fire_res(mdef) && !(youdef ? hates_holy(pd) : hates_holy_mon(mdef)))
				dmg = 0;
			goto expl_common;
		case AD_ELEC:
			if (Shock_res(mdef))
				dmg = 0;
			goto expl_common;
		case AD_EELC:
			if (Shock_res(mdef))
				dmg /= 2;
			goto expl_common;
		case AD_DARK:
			if (Dark_res(mdef))
				dmg = 0;
			goto expl_common;
		case AD_DESC:
			if (is_anhydrous(pd) || is_undead(pd))
				dmg = 0;
			goto expl_common;
expl_common:
			/* dmg has been (maybe fully) reduced based on element */

			/* apply half phys damage */
			dmg = reduce_dmg(mdef,dmg,TRUE,FALSE);

			if (dmg > 0) {
				/* damage dealt */
				/* message */
				if (youdef) {
					/* player can "dodge" based on DEX */
					if (ACURR(A_DEX) > rnd(20)) {
						You("duck some of the blast.");
						dmg = (dmg + 1) / 2;
					}
					else {
						if (flags.verbose) You("get blasted!");
					}
				}
				else if (vis) {
					pline("%s gets blasted!", Monnam(mdef));
				}
				/* spores */
				if(magr->mtyp == PM_BALLISTOSPORE && !Sick_res(mdef)){
					if(youdef)
						diseasemu(pa);
					else if(mdef->mhp < mdef->mhpmax/2)
						mdef->mspores = TRUE;
				}
				/* deal damage */
				result = xdamagey(magr, mdef, attk, dmg);
			}
			else {
				/* damage resisted */
				/* message */
				if (youdef) {
					You("are unhurt.");
				}
				else if (vis&VIS_MDEF) {
					/* we can only tell damage wasn't dealt if we can see mdef */
					pline_The("blast doesn't seem to hurt %s.", mon_nam(mdef));
				}
				else if (vis) {
					/* intentionally duplicate the blast-hurt message -- we cannot see mdef */
					pline("%s gets blasted!", Monnam(mdef));
				}
				/* golem effects */
				if (is_golem(pd)) {
					int golem_ad = 0;
					switch (attk->adtyp) {
					case AD_COLD:
					case AD_ECLD:
						roll_frigophobia();
						golem_ad = AD_COLD;
						break;
					case AD_FIRE:
					case AD_EFIR:
					case AD_ACFR:
						golem_ad = AD_FIRE;
						break;
					case AD_ELEC:
					case AD_EELC:
						golem_ad = AD_ELEC;
						break;
					}
					if (golem_ad) {
						if (youdef)
							ugolemeffects(golem_ad, fulldmg);
						else
							golemeffects(mdef, golem_ad, fulldmg);
					}
				}
			}
			/* damage inventory */
			if (attk->adtyp == AD_FIRE || attk->adtyp == AD_EFIR || attk->adtyp == AD_ACFR){
				if (!UseInvFire_res(mdef)){
					if (mlev(magr) > rn2(20))
						destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
					if (mlev(magr) > rn2(20))
						destroy_item(mdef, POTION_CLASS, AD_FIRE);
					if (mlev(magr) > rn2(25))
						destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
				}
			}
			else if (attk->adtyp == AD_ELEC || attk->adtyp == AD_EELC){
				if (!UseInvShock_res(mdef)){
					if (mlev(magr) > rn2(20))
						destroy_item(mdef, WAND_CLASS, AD_ELEC);
					if (mlev(magr) > rn2(20))
						destroy_item(mdef, RING_CLASS, AD_ELEC);
				}
			}
			else if (attk->adtyp == AD_COLD || attk->adtyp == AD_ECLD){
				if (!UseInvCold_res(mdef)){
					if (mlev(magr) > rn2(20))
						destroy_item(mdef, POTION_CLASS, AD_COLD);
				}
			}
			break;
		}
	}
	/* kill exploder */
	if (youagr) {
		rehumanize();
		change_gevurah(1); //cheated death.
		result |= MM_AGR_STOP;
	}
	else {
		/* avoid double-killing magr, if it was slain by retaliatory damage from its attack, perhaps */ 
		if (!DEADMONSTER(magr)){
			if(magr->mtyp == PM_FABERGE_SPHERE)
				mondied(magr);
			else
				mondead(magr);
		}

		if (*hp(magr) > 0)
			result |= MM_AGR_STOP;
		else
			result |= MM_AGR_DIED;

		/* give this message even if it was visible */
		if (magr->mtame)
			You("have a melancholy feeling for a moment, then it passes.");
	}
	/* wake nearby monsters -- it's a loud boom */
	wake_nearto_noisy(x(magr), y(magr), 7 * 7);

	return result;
}

void
getgazeinfo(int aatyp, int adtyp, struct permonst *pa, struct monst *magr, struct monst *mdef, boolean *needs_magr_eyes, boolean *needs_mdef_eyes, boolean *needs_magr_head, boolean *needs_uncancelled)
{
#define maybeset(b, tf) if(b) {*(b)=tf;}
	boolean adjacent = FALSE;
	if(magr && mdef && distmin(x(magr), y(magr), x(mdef), y(mdef)) <= 1)
		adjacent = TRUE;
	/* figure out if gaze requires eye-contact or not */
	switch (adtyp)
	{
		/* seeing the monster is dangerous (wide-angle gaze only) */
	case AD_CONF:
	case AD_WISD:
	case AD_BLND:
	case AD_HALU:
	case AD_STON:
		if (aatyp == AT_WDGZ){
			/* These relate to the natural form of the monster, and can't be canceled*/
			if (adtyp == AD_CONF
				|| adtyp == AD_WISD
				|| (adtyp == AD_BLND && pa->mtyp == PM_BLESSED)
				|| (adtyp == AD_STON && pa->mtyp == PM_MEDUSA)	/* acutally can be cancelled, but we want to print a special message */
				)
				maybeset(needs_uncancelled, FALSE);
			maybeset(needs_magr_eyes, FALSE);
			if((pa->mtyp == PM_GREAT_CTHULHU && adjacent) ||
				(pa->mtyp == PM_MEDUSA)){	/* actually is protected from being blind, but we want it to be reflectable */
				maybeset(needs_mdef_eyes, FALSE);
			}
			else maybeset(needs_mdef_eyes, TRUE);
			break;
		}
		/* else fall through */
		/* meeting the gaze of the monster is dangerous */
	case AD_DEAD:
	case AD_PLYS:
	case AD_LUCK:
	case AD_SLOW:
	case AD_STUN:
	case AD_SLEE:
	case AD_BLNK:
	case AD_SSEX:
	case AD_SEDU:
	case AD_VAMP:
	case AD_UNRV:
		maybeset(needs_magr_eyes, TRUE);
		maybeset(needs_mdef_eyes, TRUE);
		break;
		/* seeing the monster's face is dangerous */
	case AD_PAIN:
		maybeset(needs_magr_eyes, FALSE);
		maybeset(needs_mdef_eyes, TRUE);
		maybeset(needs_magr_head, TRUE);
		break;
		/* the monster staring *at* something is dangerous */
	case AD_FIRE:
	case AD_COLD:
	case AD_ELEC:
	case AD_DARK:
	case AD_DRLI:
	case AD_CNCL:
	case AD_ENCH:
	case AD_STDY:
	case AD_BLAS:
	case AD_BDFN:
	case AD_SPHR:
		maybeset(needs_magr_eyes, TRUE);
		maybeset(needs_mdef_eyes, FALSE);
		break;
		/* these adtyps are just using gaze as a convenient way of causing something non-gaze-y to happen */
	case AD_SSUN:
	case AD_WTCH:
	case AD_MIST:
	case AD_SPOR:
		maybeset(needs_magr_eyes, FALSE);
		maybeset(needs_mdef_eyes, FALSE);
		maybeset(needs_uncancelled, FALSE);
		break;
	default:
		impossible("unhandled gaze type %d from %s in getgazeinfo", adtyp, !magr ? "no magr??" : mon_nam(magr));
		break;
	}
	if (pa->mtyp == PM_DEMOGORGON) {					// Demogorgon is special
		maybeset(needs_mdef_eyes, TRUE);
		maybeset(needs_uncancelled, FALSE);
	}
	return;
#undef maybeset
}

/* still needed to support old AD_SEDU gaze attack */
boolean
umetgaze(mtmp)
struct monst *mtmp;
{
	return (canseemon_eyes(mtmp) && couldsee(mtmp->mx, mtmp->my) && !Gaze_immune && multi>=0);
}


/* xgazey()
 * 
 * A creature uses its gaze attack (either active or passive) on another.
 *
 * Returns MM_MISS if this failed and took no time (though a player attempting to #monster gaze still used their turn)
 *
 * Otherwise returns MM hitflags as usual.
 */
int
xgazey(magr, mdef, attk, vis)
struct monst * magr;
struct monst * mdef;
struct attack * attk;
int vis;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct permonst * pa = (youagr ? youracedata : magr->data);
	struct permonst * pd = (youdef ? youracedata : mdef->data);

	boolean needs_magr_eyes = TRUE;		/* when TRUE, mdef is protected if magr is blind */
	boolean needs_mdef_eyes = TRUE;		/* when TRUE, mdef is protected by being blind */
	boolean needs_magr_head = FALSE;		/* when TRUE, mdef is protected if magr's head is covered */
	boolean needs_uncancelled = TRUE;	/* when TRUE, attack cannot happen when cancelled */
	boolean maybe_not = (!youagr && pa->mtyp != PM_DEMOGORGON);		/* when TRUE, occasionally doesn't use gaze attack at all */
	boolean cooldown = TRUE;			/* when TRUE, attack may set cooldown */

	static const int randomgazeattacks[] = { AD_DEAD, AD_CNCL, AD_PLYS, AD_DRLI, AD_ENCH, AD_STON, AD_LUCK,
		AD_CONF, AD_SLOW, AD_STUN, AD_BLND, AD_FIRE, AD_FIRE,
		AD_COLD, AD_COLD, AD_ELEC, AD_ELEC, AD_HALU, AD_SLEE };
	static const int elementalgazeattacks[] = { AD_FIRE, AD_COLD, AD_ELEC };
	struct obj *ahelm = youagr ? uarmh : which_armor(magr, W_ARMH);
	struct obj *acloak = youagr ? uarmc : which_armor(magr, W_ARMC);

	char buf[BUFSZ];
	struct attack alt_attk;

	int result = MM_MISS;
	int adtyp = attk->adtyp;
	int dmg = d((int)attk->damn, (int)attk->damd);
	int fulldmg = dmg;			/* original unreduced damage */

	if (vis == -1)
		vis = getvis(magr, mdef, 0, 0);

	/* Hamsa ward protects from gazes */
	if (ward_at(x(mdef), y(mdef)) == HAMSA)
		return MM_MISS;
	/* at the very least, all gaze attacks need a clear line of sight */
	if (!clear_path(x(magr), y(magr), x(mdef), y(mdef)))
		return MM_MISS;
	if (youdef && Invulnerable)
		return MM_MISS;

	/* fix up adtyps for some gazes */
	switch (adtyp)
	{
	case AD_RGAZ:
		adtyp = randomgazeattacks[rn2(SIZE(randomgazeattacks))];
		break;
	case AD_RBRE:
	case AD_RETR:
		adtyp = elementalgazeattacks[rn2(SIZE(elementalgazeattacks))];
		break;
	case AD_WISD:
		if (!youdef)
			adtyp = AD_CONF;
		break;
	}
	/* get eyes, uncancelledness */
	getgazeinfo(attk->aatyp, adtyp, pa, magr, mdef, &needs_magr_eyes, &needs_mdef_eyes, &needs_magr_head, &needs_uncancelled);

	/* widegazes cannot fail, and don't use mspec_used */
	if (attk->aatyp == AT_WDGZ){
		maybe_not = FALSE;
		cooldown = FALSE;
	}

	/* these gazes are actually hacks and only work vs the player */
	if (!youdef && (adtyp == AD_WTCH || adtyp == AD_MIST))
		return MM_MISS;
	if (adtyp == AD_SSUN && ((youdef ? Invis : mdef->minvis) && !(youagr ? See_invisible(x(mdef), y(mdef)) : mon_resistance(magr, SEE_INVIS)) ))
		return MM_MISS;
	if (/* needs_magr_eyes:   magr must have eyes and can actively see mdef */
		(needs_magr_eyes && !(
			(haseyes(pa)) &&
			(!(youagr ? Blind : is_blind(magr))) &&
			(!(youdef ? Invis : mdef->minvis) || (youagr ? See_invisible(x(mdef), y(mdef)) : mon_resistance(magr, SEE_INVIS))) &&
			(youagr ? canseemon(mdef) : youdef ? mon_can_see_you(magr) : mon_can_see_mon(magr, mdef)) &&
			(!(youagr ? Sleeping : magr->msleeping)) &&
			(!Gaze_res(mdef))
		))
		||
		/* needs_mdef_eyes:   mdef must have eyes and can actively see magr */
		(needs_mdef_eyes && !(
			(haseyes(pd)) &&
			(!(youdef ? Blind : is_blind(mdef))) &&
			(!(youagr ? Invis : magr->minvis) || (youdef ? See_invisible(x(magr), y(magr)) : mon_resistance(mdef, SEE_INVIS))) &&
			(youdef ? canseemon(magr) : youagr ? mon_can_see_you(mdef) : mon_can_see_mon(mdef, magr)) &&
			(!(youdef ? Sleeping : mdef->msleeping)) &&
			(!Gaze_res(mdef)) /* wearing the Eyes, nearly anything is safe to see */
		))
		||
		/* needs_mdef_eyes:   mdef must have eyes and can actively see magr */
		(needs_magr_head && (
			( (ahelm && FacelessHelm(ahelm)) || (acloak && FacelessCloak(acloak)) ) || /* wearing a faceless helm or cloak (cloak even if headless) */
			(Gaze_res(mdef)) /* wearing the Eyes, nearly anything is safe to see */
		))
		){
		/* gaze fails because the appropriate gazer/gazee eye (contact?) is not available */
		return MM_MISS;
	}
	if (needs_uncancelled && !(
		youagr ||
		!magr->mcan
		)) {
		/* gaze fails because magr is cancelled */
		return MM_MISS;
	}

	/* Do the appropriate stuff -- function often returns in this switch statement */
	switch (adtyp)
	{
		/* elemental gazes */
	case AD_FIRE:
	case AD_COLD:
	case AD_ELEC:
	case AD_DARK:
		/* 4/5 chance to succeed */
		if (maybe_not && !rn2(5))
			return MM_MISS;
		/* message */
		if (vis&VIS_MAGR) {
			switch (adtyp) {
			case AD_FIRE:	Sprintf(buf, "fiery");		break;
			case AD_COLD:	Sprintf(buf, "icy");		break;
			case AD_ELEC:	Sprintf(buf, "shocking");	break;
			case AD_DARK:	Sprintf(buf, "dark");	break;
			}
			pline("%s attack%s %s with a %s stare.",
				(youagr ? "You" : Monnam(magr)),
				(youagr ? "" : "s"),
				(youdef ? "you" : mon_nam(mdef)),
				buf
				);
		}
		/* re-use xmeleehurty -- it will print a "X is on fire"-esque message, deal damage, and all other good stuff */
		alt_attk = *attk;
		alt_attk.aatyp = AT_GAZE;
		alt_attk.adtyp = adtyp;
		result = xmeleehurty(magr, mdef, &alt_attk, attk, (struct obj **)0, FALSE, dmg, 0, vis, TRUE);
		wakeup2(mdef, youagr);
		break;

	case AD_BDFN:
		if (has_blood_mon(mdef)){
			if (vis&VIS_MDEF) pline("A thin spear of %s %s pierces %s %s.",
				(youdef ? "your" : s_suffix(mon_nam(mdef))),
				mbodypart(mdef, BLOOD),
				(youdef ? "your" : hisherits(mdef)),
				mbodypart(mdef, BODY_SKIN)
				);
			if (youdef){
				change_usanity(-1 * dmg, FALSE);
				exercise(A_CON, FALSE);
				u.umadness |= MAD_FRENZY;
			}
			else {
				if (mdef->mconf && !rn2(10)){
					if (mdef->mstun && !rn2(10)){
						if (vis&VIS_MDEF) pline("%s %s leaps through %s %s!", s_suffix(Monnam(mdef)), mbodypart(mdef, BLOOD), hisherits(mdef), mbodypart(mdef, BODY_SKIN));
						//reduce current HP by 30% (round up, guaranteed nonfatal)
						mdef->mhp = mdef->mhp*.7 + 1;
						if (mdef->mhpmax > mdef->mhp){
							if (mdef->mhpmax > mdef->mhp + mdef->m_lev)
								mdef->mhpmax -= mdef->m_lev;
							else
								mdef->mhpmax--;
						}
					}
					else {
						mdef->mstun = 1;
					}
				}
				else {
					mdef->mconf = 1;
				}
			}
			alt_attk = *attk;
			alt_attk.aatyp = AT_NONE;
			alt_attk.adtyp = AD_PHYS;
			result = xmeleehurty(magr, mdef, &alt_attk, attk, (struct obj **)0, FALSE, dmg, 0, FALSE, TRUE);
			wakeup2(mdef, youagr);
		}
		break;
		/* lifedrain gazes */
	case AD_VAMP:
	case AD_DRLI:
		/* Demogorgon's gaze is special, of course*/
		if (youdef && pa->mtyp == PM_DEMOGORGON){
			if (!Drain_resistance || !rn2(3)){
				You("meet the gaze of Hethradiah, right head of Demogorgon!");
				You("feel a primal darkness fall upon your soul!");
				losexp("primal darkness", FALSE, !rn2(3), FALSE);
				losexp("primal darkness", FALSE, !rn2(3), FALSE);
				losexp("primal darkness", TRUE, TRUE, FALSE);
				if (u.sealsActive&SEAL_HUGINN_MUNINN){
					unbind(SEAL_HUGINN_MUNINN, TRUE);
				}
				else {
					forget(13);
				}
			}
			else {
				You("avoid the gaze of the right head of Demogorgon!");
				return MM_MISS;
			}
		}
		else {
			/* 1/3 chance to succeed */
			if (maybe_not && rn2(3))
				return MM_MISS;

			/* Don't waste turns trying to drain the life from a resistant target */
			if (Drain_res(mdef))
				return MM_MISS;

			/* message */
			if (youdef) {
				if (vis&VIS_MAGR) {
					if (adtyp == AD_VAMP)
						pline("%s hungry eyes feed on your life force!", Monnam(magr));
					else
						You("feel your life force wither before the gaze of %s!", mon_nam(magr));
				}
				else
					You("feel your life force wither!");
			}
			else if (vis&VIS_MAGR && vis&VIS_MDEF) {
				pline("%s gaze %ss%s %s's life force!",
					(youagr ? "Your" : s_suffix(Monnam(magr))),
					(adtyp == AD_VAMP ? "feed" : "wither"),
					(adtyp == AD_VAMP ? " on" : ""),
					mon_nam(mdef)	/* note: cannot use two s_suffix() calls in same pline. Grr. */
					);
			}
			/* drain life! */
			if (youdef) {
				/* the player has a handy level-drain function */
				losexp("life force drain", TRUE, FALSE, FALSE);
			}
			else {
				/* print message first -- this should happen before the victim is drained/dies */
				if (vis&VIS_MDEF)
					pline("%s suddenly seems weaker!", Monnam(mdef));

				/* for monsters, we need to make something up -- drain 2d6 maxhp, 1 level */
				dmg = d(2, 6);

				/* kill if this will level-drain below 0 m_lev, or lifedrain below 1 maxhp */
				if (mlev(mdef) == 0 || *hpmax(mdef) <= dmg) {
					/* clean up the maybe-dead monster, return early */
					if (youagr)
						killed(mdef);
					else
						monkilled(mdef, "", attk->adtyp);
					/* is it dead, or was it lifesaved? */
					if (mdef->mhp > 0)
						return (MM_HIT | MM_DEF_LSVD);	/* lifesaved */
					else
						return (MM_HIT | MM_DEF_DIED | ((youagr || grow_up(magr, mdef)) ? 0 : MM_AGR_DIED));
				}
				else {
					/* drain stats */
					mdef->m_lev--;
					mdef->mhpmax -= dmg;
				}
			}
		}
		if (*hp(mdef) > 0)
			wakeup2(mdef, youagr);
		break;
	case AD_SSUN:
		/* requires reflectable light */
		if (dimness(x(magr), y(magr))  > 0)
			return MM_MISS;
		
		if (!levl[x(magr)][y(magr)].lit)
			dmg = (dmg+1)/2;

		/* message and blind */
		if (youdef) {
			pline("%s attacks you with a beam of reflected light!", Monnam(magr));
			stop_occupation();

			if (canseemon(magr) && !resists_blnd(&youmonst)) {
				You("are blinded by %s beam!", s_suffix(mon_nam(magr)));
				make_blinded((long)dmg, FALSE);
			}
			if (Fire_resistance) {
				pline_The("beam doesn't feel hot!");
				dmg = 0;
			}
			else if (Reflecting){
				if (canseemon(magr)) ureflects("%s beam is reflected by your %s.", s_suffix(Monnam(magr)));
				dmg = 0;
			}
		}
		else {
			if (vis&VIS_MAGR && vis&VIS_MDEF) {
				pline("%s attack%s %s with a beam of reflected light!",
					(youagr ? "You" : Monnam(magr)),
					(youagr ? "" : "s"),
					mon_nam(mdef)
					);
			}
			if (can_blnd(magr, mdef, attk->aatyp, (struct obj *)0)) {
				if (vis&VIS_MDEF)
					pline("%s is blinded!", Monnam(mdef));
				mdef->mblinded = dmg;
				mdef->mcansee = 0;
			}
			if (Fire_res(mdef))
				dmg = 0;
			else if (mon_reflects(mdef, "The beam is reflected by %s %s!"))
				dmg = 0;
		}
		/* damage inventory */
		if (levl[x(magr)][y(magr)].lit && !UseInvFire_res(mdef) && !(youdef ? Reflecting : mon_resistance(mdef, REFLECTING))) {
			if ((int)mlev(magr) > rn2(20))
				destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
			if ((int)mlev(magr) > rn2(20))
				destroy_item(mdef, POTION_CLASS, AD_FIRE);
			if ((int)mlev(magr) > rn2(25))
				destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
		}

		if (youdef){
			burn_away_slime();
			melt_frozen_air();
		}

		if (dmg)
			result = xdamagey(magr, mdef, attk, dmg);

		if (*hp(mdef) > 0)
			wakeup2(mdef, youagr);
		break;
		/* deathgaze */
	case AD_DEAD:
		/* message */
		if (youdef) {
			pline("Oh no, you meet %s gaze of death!",
				s_suffix(mon_nam(magr)));
		}
		else if (vis&VIS_MDEF && vis&VIS_MAGR) {
			pline("%s meets %s gaze of death!",
				Monnam(mdef),
				(youagr ? "your" : s_suffix(mon_nam(magr)))
				);
		}
		/* effect */
		if (youdef) {
			if (nonliving(pd) || is_demon(pd)) {
				You("seem no deader than before.");
			}
			else if (Magic_res(mdef) || (u.sealsActive&SEAL_OSE)) {
				if (Magic_res(mdef))
					shieldeff(u.ux, u.uy);
				pline("Lucky for you, it didn't work!");
			}
			else {
				if (Hallucination) {
					You("have an out of body experience.");
				}
				else {
					killer_format = KILLED_BY_AN;
					killer = "gaze of death";
					if (!u.uconduct.killer && !youagr){
						//Pcifist PCs aren't combatants so if something kills them up "killed peaceful" type impurities
						IMPURITY_UP(u.uimp_murder)
						IMPURITY_UP(u.uimp_bloodlust)
					}
					done(DIED);

					if (*hp(mdef) > 0)
						return (MM_HIT | MM_DEF_LSVD);				/* you lifesaved */
					else
						return (MM_HIT | MM_DEF_DIED);	/* moot */
				}
			}
		}
		else {
			if (nonliving(mdef->data) || is_demon(pd)) {
				if (vis&VIS_MDEF && vis&VIS_MAGR) {
					pline("%s seems no deader than before.",
						Monnam(mdef));
				}
			}
			else if (Magic_res(mdef)) {
				if (vis&VIS_MDEF && vis&VIS_MAGR) {
					pline("It didn't seem to work.");
				}
			}
			else {
				/* no hallucination protection for monsters */
				/* instakill */
				*hp(mdef) = 0;
				if (youagr)
					killed(mdef);
				else
					monkilled(mdef, "", AD_DETH);

				if (*hp(mdef) > 0)
					return (MM_HIT | MM_DEF_LSVD); /* mdef lifesaved */
				else
					return (MM_HIT | MM_DEF_DIED | ((youagr || grow_up(magr, mdef)) ? 0 : MM_AGR_DIED));
			}
		}
		break;
		/* stonegaze */
	case AD_STON:
		/* special-case for medusa */
		if (pa->mtyp == PM_MEDUSA) {
			static boolean medusa_canc = FALSE;
			static boolean medusa_uref = FALSE;
			static boolean medusa_mref = FALSE;
			static boolean medusa_nnot = FALSE;
			static boolean medusa_rref = FALSE;
			boolean printed_ref = FALSE;

			/* cancelled Medusa does nothing */
			if (!youagr && magr->mcan) {
				/* message for player */
				if (youdef && (vis&VIS_MAGR) && !medusa_canc) {
					pline("%s doesn't look all that ugly.", Monnam(magr));
					medusa_canc = TRUE;
				}
				return MM_HIT;	/* no further effects */
			}
			else medusa_canc = FALSE;

			/* Medusa's gaze can be safely reflected back at her */
			if (youdef ? Reflecting : mon_resistance(mdef, REFLECTING)) {
				if (youdef) {
					if (!medusa_uref && (vis&VIS_MAGR)) {
						(void)ureflects("%s image is reflected by your %s.",
							s_suffix(Monnam(magr)));
						medusa_uref = TRUE;
						printed_ref = TRUE;
					}
				}
				else {
					if (!medusa_mref && (vis&VIS_MDEF) && (vis&VIS_MAGR)) {
						Sprintf(buf, "%s image is reflected by %%s %%s.",
							Monnam(magr));
						mon_reflects(mdef, buf);
						medusa_mref = TRUE;
						printed_ref = TRUE;
					}
				}
				/* Medusa's gaze was reflected, it may turn Medusa to stone */
				/* check if medusa can see her reflection */
				if ((youagr ? Blind : is_blind(magr))
					|| (
					((youagr ? Invis : magr->minvis) || (youdef ? Invis : mdef->minvis))
					&& !(youagr ? See_invisible(x(mdef), y(mdef)) : mon_resistance(magr, SEE_INVIS))
					))
				{
					if (!youagr && (vis&VIS_MAGR) && !medusa_nnot) {
						if (!printed_ref) {
							if (youdef) {
								Sprintf(buf, "%%s doesn't seem to notice that %s image was reflected by your %%s.",
									mhis(magr));
								ureflects(buf, Monnam(magr));
							}
							else {
								Sprintf(buf, "%s doesn't seem to notice that %s image was reflected by %%s %%s.",
									Monnam(magr),
									mhis(magr));
								mon_reflects(mdef, buf);
							}
						}
						else {
							Strcpy(buf, mhe(magr));
							pline("%s doesn't seem to notice.",
								upstart(buf));
						}
						medusa_nnot = TRUE;
					}
					return MM_HIT;	/* no further effects */
				}
				else if(printed_ref) medusa_nnot = FALSE;

				/* check if medusa re-reflected her image */
				if (youagr ? Reflecting : mon_resistance(magr, REFLECTING)) {
					if ((vis&VIS_MAGR) && !medusa_rref) {
						if (!printed_ref) {
							if (youagr) {
								ureflects("Your image is reflected back at you by %s, which you reflect away with your %s!", mon_nam(magr));
							}
							else {
								Sprintf(buf, "%%s image is reflected back at %s by %s, which %s reflects away with %s %%s!",
									mhis(magr),
									youdef ? "you" : mon_nam(mdef),
									mhe(magr),
									mhis(magr)
									);
								mon_reflects(magr, buf);
							}
						}
						else {
							if (youagr) {
								ureflects("Your %s%s reflects your image away!", "");
							}
							else {
								Sprintf(buf, "%%s %%s reflects %s image away!",
									mhis(magr)
									);
								mon_reflects(magr, buf);
							}
						}
						medusa_rref = TRUE;
					}
					return MM_HIT;	/* no further effects */
				}
				else if (printed_ref) medusa_rref = FALSE;

				/* otherwise, medusa gets affected by her own image! */
				medusa_canc = FALSE;
				medusa_uref = FALSE;
				medusa_mref = FALSE;
				medusa_nnot = FALSE;
				medusa_rref = FALSE;
				if (youagr) {
					/* don't use xstoney, which is a delayed stoning effect */
					if (!Stone_resistance) {
						if (!printed_ref) {
							mon_reflects(mdef, "You see your image reflected in %s %s!");
						}
						else {
							You("see your reflected image!");
						}
						stop_occupation();
						if (poly_when_stoned(youracedata) && polymon(PM_STONE_GOLEM)) break;
						You("turn to stone...");
						killer_format = KILLED_BY;
						killer = "Poseidon's curse";
						done(STONING);
						return (MM_HIT | MM_AGR_STOP);	/* you lifesaved */
					}
				}
				else {
					if (!printed_ref) {
						if (youdef) {
							Sprintf(buf, "%s sees %s image reflected in your %%s%%s!",
								Monnam(magr),
								mhis(magr)
								);
							ureflects(buf, "");
						}
						else {
							Sprintf(buf, "%s sees %s image reflected in %%s %%s!",
								Monnam(magr),
								mhis(magr)
								);
							mon_reflects(mdef, buf);
						}
					}
					pline("%s is turned to stone!", Monnam(magr));
					/* Medusa doesn't get any of the usual anti-stoning methods to protect her from her own curse */
					stoned = TRUE;
					if (youdef)
						killed(magr);
					else
						monkilled(magr, "", AD_STON);
					stoned = FALSE;
					return (MM_HIT | (*hp(magr) > 0 ? MM_AGR_STOP : MM_AGR_DIED));
				}
			}
			else {
				if (youdef)
					medusa_uref = FALSE;
				else
					medusa_mref = FALSE;
			}

			/* now handle being unable to see Medusa */
			/* copy-paste of needs_mdef_eyes from above :( */
			if(!(
				(haseyes(pd)) &&
				(!(youdef ? Blind : is_blind(mdef))) &&
				(!(youagr ? Invis : magr->minvis) || (youdef ? See_invisible(x(magr), y(magr)) : mon_resistance(mdef, SEE_INVIS))) &&
				(youdef ? canseemon(magr) : youagr ? mon_can_see_you(mdef) : mon_can_see_mon(mdef, magr)) &&
				(!(youdef ? Sleeping : mdef->msleeping)) &&
				(!Gaze_res(mdef)) /* wearing the Eyes, nearly anything is safe to see */
				))
				return MM_MISS;

			/* reflection handled; we already knew mdef can see magr to get here; attempt to stone mdef */
			if (!Stone_res(mdef)) {
				/* don't use xstoney, which is a delayed stoning effect vs player */
				if (youdef) {
					You("see %s.", mon_nam(magr));
					stop_occupation();
					if (poly_when_stoned(youracedata) && polymon(PM_STONE_GOLEM)) break;
					You("turn to stone...");
					killer_format = KILLED_BY;
					killer = "Poseidon's curse";
					done(STONING);
					return (MM_HIT | MM_DEF_LSVD);	/* if you didn't lifesave, it's a moot point */
				}
				else {
					if (!munstone(mdef, youagr))
					{
						if (poly_when_stoned(pd)) {
							mon_to_stone(mdef);
							return (MM_HIT | MM_DEF_LSVD);
						}
						if ((vis&VIS_MDEF) && (vis&VIS_MAGR))
							pline("%s sees %s and turns to stone!", Monnam(mdef), mon_nam(magr));
						else if ((vis&VIS_MDEF))
							pline("%s turns to stone!", Monnam(mdef));
						monstone(mdef);
					}
					if (mdef->mhp > 0)
						return (MM_HIT|MM_DEF_LSVD);
					else if (mdef->mtame && !vis)
						You("have a peculiarly sad feeling for a moment, then it passes.");

					return (MM_DEF_DIED | (grow_up(magr, mdef) ? 0 : MM_AGR_DIED));
				}
			}
		}/* end medusa stoning gaze */

		/* all other stoning gazes are delayed stoning that require full magr-mdef eye contact */
		/* the eye-contact requirement should have been handled already */
		if ((vis&VIS_MDEF) && (vis&VIS_MAGR)) {
			/* change message based on expected effectiveness of xstoney() */
			static long lastwarned = 0L;
			if (Stone_res(mdef) || (youdef && Stoned)) {
				if(moves - lastwarned > 4){
					if(pa->mtyp == PM_MEDUSA){
						pline("%s %s look all that ugly to %s.",
							youagr ? "You" : Monnam(magr),
							youagr ? "don't" : "doesn't",
							youdef ? "you" : mon_nam(mdef)
							);
					} else {
						pline("%s gaze%s ineffectively at %s.",
							youagr ? "You" : Monnam(magr),
							youagr ? "" : "s",
							youdef ? "you" : mon_nam(mdef)
							);
					}
				}
				lastwarned = moves;
			}
			else {
				pline("%s meet%s %s petrifying gaze!",
					youdef ? "You" : Monnam(mdef),
					youdef ? "" : "s",
					youagr ? "your" : s_suffix(mon_nam(magr))
					);
			}
		}
		/* do stone */
		return xstoney(magr, mdef);

		/* confusion */
	case AD_CONF:
		/* 4/5 chance to succeed */
		if (maybe_not && !rn2(5))
			return MM_MISS;
		/* default confusion time: 3d4 */
		if (!dmg)
			dmg = d(3, 4);
		/* simulate cooldown to avoid frequent re-application */
		if (!youagr && cooldown && !rn2(6)) {
			if(youdef && Confusion)
				return MM_MISS;
			else if(!youdef && mdef->mconf)
				return MM_MISS;
		}

		if (youdef) {
			if (Confusion) {
				You("are getting more and more confused.");
				/* if magr is confusing us every turn, let's not stack confusion too high */
				if (!cooldown && (HConfusion > 0))
					dmg = min(dmg*dmg / HConfusion, dmg);
			}
			else {
				pline("%s %s confuses you!",
					s_suffix(Monnam(magr)),
					(attk->aatyp == AT_WDGZ) ? "form" : "gaze"
					);
			}
			make_confused(HConfusion + dmg, FALSE);
			stop_occupation();
			if(youdef && attk->aatyp == AT_WDGZ && rn2(100) < Insanity){
				change_usanity(-rnd(4), TRUE);
			}
		}
		else
		{
			if (!mdef->mconf) {
				if (vis&VIS_MDEF && vis&VIS_MAGR) {
					pline("%s looks confused by %s %s.",
						Monnam(mdef),
						(youagr ? "your" : s_suffix(mon_nam(magr))),
						((is_uvuudaum(pa) || attk->adtyp == AD_WISD) ? "form" : "gaze")
						);
				}
				else if (vis&VIS_MDEF) {
					pline("%s looks confused.", Monnam(mdef));
				}
				mdef->mconf = 1;
				mdef->mstrategy &= ~STRAT_WAITFORU;
			}
		}
		break;
		/* paralysis */
	case AD_PLYS:
		/* Demogorgon's gaze is special, of course*/
		if (youdef && pa->mtyp == PM_DEMOGORGON){
			if (!cantmove(mdef) && (!Free_action || rn2(2)) && (!Sleep_resistance || rn2(4))){
				You("meet the gaze of Aameul, left head of Demogorgon!");
				You("are mesmerized!");
				nomovemsg = 0;	/* default: "you can move again" */
				if (!Free_action && !Sleep_resistance) nomul(-rn1(5, 2), "mesmerized by Aameul");
				else if (!Free_action || !Sleep_resistance) nomul(-1, "mesmerized by Aameul");
				else youmonst.movement -= 6;
				exercise(A_DEX, FALSE);
			}
			else {
				You("avoid the gaze of the left head of Demogorgon!");
				return MM_MISS;
			}
		}
		else {
			/* 1/3 chance to succeed */
			if (maybe_not && rn2(3))
				return MM_MISS;

			/* calc max paralysis time, and default time is 1d10 */
			int maxdmg = attk->damn * attk->damd;
			if (!dmg) {
				dmg = rnd(10);
				maxdmg = 10;
			}

			if(cantmove(mdef))
					return MM_MISS;

			/* split between player and monster */
			if (youdef) {
				Sprintf(buf, "%%s gaze is %sreflected by your %%s%s",
					(magr->mtyp == PM_AXUS) ? "only partially " : "",
					(magr->mtyp == PM_AXUS) ? "!" : "."
					);
				if (magr->data->mlet == S_EYE && attk->aatyp == AT_WDGZ &&
					ureflects(buf,s_suffix(Monnam(magr)))){
					/* ureflects() prints message */
					if (magr->mtyp == PM_AXUS)
						dmg = dmg/2 + 1;
					else
						dmg = 0;
				}
				if (dmg > 0 && Free_action) {
					You("momentarily stiffen.");
					dmg = 0;
				}
				if (dmg > 0) {
					You("are mesmerized by %s!", mon_nam(magr));
					nomovemsg = 0;	/* default: "you can move again" */
					nomul(-dmg, "mesmerized by a monster");
					exercise(A_DEX, FALSE);
				}
			}
			else {
				Sprintf(buf, "%s gaze is %sreflected by %%s %%s%s",
					s_suffix(Monnam(mdef)),
					(magr->mtyp == PM_AXUS) ? "partially " : "",
					(magr->mtyp == PM_AXUS) ? "!" : ".");
				if (magr->data->mlet == S_EYE && attk->aatyp == AT_WDGZ &&
					mon_reflects(magr, canseemon(magr) ? buf : (char *)0)
				) {
					/* mon_reflects() prints message */
					if (magr->mtyp == PM_AXUS)
						dmg = dmg/2 + 1;
					else
						dmg = 0;
				}
				if (dmg > 0 && mon_resistance(mdef, FREE_ACTION)) {
					if (vis&VIS_MDEF) {
						pline("%s momentarily stiffens.", Monnam(mdef));
					}
					dmg = 0;
				}
				if (dmg > 0) {
					if (vis&VIS_MDEF) {
						pline("%s freezes!", Monnam(mdef));
					}
					mdef->mcanmove = 0;
					mdef->mfrozen = dmg;
					mdef->mstrategy &= ~STRAT_WAITFORU;
				}
			}
		}
		break;

	case AD_ENCH:
		if (youdef && pa->mtyp == PM_DEMOGORGON){		/* has this been depricated? */
			struct obj *obj = some_armor(&youmonst);
			if (drain_item(obj)) {
				You("meet Demogorgon's gaze!");
				Your("%s less effective.", aobjnam(obj, "seem"));
			}
		}
		else {
			/* 1/4 chance to succeed */
			if (maybe_not && rn2(4))
				return MM_MISS;

			struct obj * otmp = some_armor(mdef);

			if (youagr)
				You("stare at %s.", mon_nam(mdef));

			if (drain_item(otmp)) {
				if (youdef) {
					if (vis&VIS_MAGR)
						pline("%s stares at you.", Monnam(magr));
					else
						You_feel("watched.");
					Your("%s less effective.",
						aobjnam(otmp, "seem"));
				}
				else if (vis&VIS_MDEF) {
					pline("%s %s less effective.",
						s_suffix(Monnam(mdef)),
						aobjnam(otmp, "seem"));
				}
			}
		}
		break;
		/* slow */
	case AD_SLOW:
		/* 4/5 chance to succeed */
		if (maybe_not && !rn2(5))
			return MM_MISS;

		if (youdef) {
			if (vis&VIS_MAGR)
				pline("%s stares piercingly at you!", Monnam(magr));
			else
				You_feel("watched.");
			u_slow_down();
			stop_occupation();
		}
		else {
			unsigned int oldspeed = mdef->mspeed;

			if (vis&VIS_MAGR && vis&VIS_MDEF)
				pline("%s stare%s piercingly at %s!",
				(youagr ? "You" : Monnam(magr)),
				(youagr ? "" : "s"),
				mon_nam(mdef)
				);

			mon_adjust_speed(mdef, -1, (struct obj *)0, TRUE);
			mdef->mstrategy &= ~STRAT_WAITFORU;
			if (mdef->mspeed != oldspeed && vis&VIS_MDEF)
				pline("%s slows down.", Monnam(mdef));
		}
		break;
		/* stun */
	case AD_STUN:
		/* 4/5 chance to succeed */
		if (maybe_not && !rn2(5))
			return MM_MISS;
		/* default stun time: 2d6 */
		if (!dmg)
			dmg = d(2, 6);
		/* simulate cooldown to avoid frequent re-application */
		if (!youagr && cooldown && !rn2(6)) {
			if(youdef && Stunned)
				return MM_MISS;
			else if(!youdef && mdef->mstun)
				return MM_MISS;
		}

		if (youdef) {
			if (vis&VIS_MAGR)
				pline("%s stares piercingly at you!", Monnam(magr));
			else
				You_feel("watched.");
			make_stunned(HStun + dmg, TRUE);
			stop_occupation();
		}
		else {
			if (vis&VIS_MAGR && vis&VIS_MDEF)
				pline("%s stare%s piercingly at %s!",
				(youagr ? "You" : Monnam(magr)),
				(youagr ? "" : "s"),
				mon_nam(mdef)
				);
			if (vis&VIS_MDEF) {
				pline("%s %s for a moment.",
					Monnam(mdef),
					makeplural(stagger(mdef, "stagger")));
			}
			mdef->mstun = 1;
		}
		break;
		/* blinding (radiance and gaze) */
	case AD_BLND:
		/* there is an existing can-blind check, yay! */
		if (!can_blnd(magr, mdef, attk->aatyp, (struct obj *)0))
			return MM_MISS;

		/* assumes that angels with AD_BLND have a blinding radiance, which is limited range and stunning */
		if (is_angel(pa) || pa->mtyp == PM_BLESSED) {
			if (dist2(x(magr), y(magr), x(mdef), y(mdef)) > BOLT_LIM*BOLT_LIM)
				return MM_MISS;
			if (youdef) {
				You("are blinded by %s radiance!", s_suffix(mon_nam(magr)));
				make_blinded((long)dmg, FALSE);
				stop_occupation();
				make_stunned((long)d(1, 3), TRUE);
			}
			else {
				if (vis&VIS_MDEF && vis&VIS_MAGR) {
					pline("%s is blinded by %s radiance!",
						Monnam(mdef),
						(youagr ? "your" : s_suffix(mon_nam(magr)))
						);
				}


				if (vis&VIS_MDEF) {
					pline("%s %s for a moment.",
						Monnam(mdef),
						makeplural(stagger(mdef, "stagger")));
				}
				mdef->mstun = 1;
			}
		}
		/* any other blinding gazes */
		else {
			/* 4/5 chance to succeed */
			if (maybe_not && !rn2(5))
				return MM_MISS;
			/* default blind time: 2d6 */
			if (!dmg)
				dmg = d(2, 6);
			/* simulate cooldown to avoid frequent re-application */
			if (!youagr && cooldown && !rn2(6)) {
				if(youdef && Blind)
					return MM_MISS;
				else if(!youdef && mdef->mblinded)
					return MM_MISS;
			}

			if (youdef)
			{
				You("are blinded by %s gaze!", s_suffix(mon_nam(magr)));
				make_blinded((long)dmg, FALSE);
				stop_occupation();
			}
			else {
				if (vis&VIS_MDEF && vis&VIS_MAGR) {
					pline("%s is blinded by %s gaze!",
						Monnam(mdef),
						(youagr ? "your" : s_suffix(mon_nam(magr)))
						);
				}
				mdef->mblinded = dmg;
				mdef->mcansee = 0;
			}
		}
		break;

		/* hallucination */
	case AD_HALU:
		/* 4/5 chance to succeed */
		if (maybe_not && !rn2(5))
			return MM_MISS;
		/* default hallu time: 1d12 */
		if (!dmg)
			dmg = rnd(12);
		/* simulate cooldown to avoid frequent re-application */
		if (!youagr && cooldown && !rn2(6)) {
			if(youdef && Hallucination)
				return MM_MISS;
			else if(!youdef && mdef->mconf)
				return MM_MISS;
		}

		if (youdef) {
			if (!hallucinogenic(pd)) {
				boolean chg;
				if (vis&VIS_MAGR)
					pline("%s attacks you with a kaleidoscopic gaze!", Monnam(magr));
				else if (!Hallucination)
					Your("mind is filled with kaleidoscopic light!");
				chg = make_hallucinated(HHallucination + (long)dmg, FALSE, 0L);
				You("%s.", chg ? "are freaked out" : "seem unaffected");
			}
		}
		/* monsters get confused by AD_HALU */
		else {
			if (vis&VIS_MDEF && vis&VIS_MAGR) {
				pline("%s looks confused by %s gaze.",
					Monnam(mdef),
					(youagr ? "your" : s_suffix(mon_nam(magr)))
					);
			}
			else if (vis&VIS_MDEF) {
				pline("%s looks confused.", Monnam(mdef));
			}

			mdef->mconf = 1;
			mdef->mstrategy &= ~STRAT_WAITFORU;
		}
		break;
		/* sleep */
	case AD_SLEE:
		/* 4/5 chance to succeed */
		if (maybe_not && !rn2(5))
			return MM_MISS;
		/* no effect on sleeping or immune targets */
		if (Sleep_res(mdef) || (youdef ? multi >= 0 : mdef->msleeping))
			return MM_MISS;
		/* default sleep time: 1d10 */
		if (!dmg)
			dmg = rnd(10);
		/* simulate cooldown to avoid frequent re-application */
		if (!youagr && cooldown && !rn2(6)) {
			if(youdef && Sleeping)
				return MM_MISS;
			else if(!youdef && mdef->msleeping)
				return MM_MISS;
		}

		if (youdef) {
			pline("%s gaze makes you very sleepy...",
				s_suffix(Monnam(magr)));
			fall_asleep(-dmg, TRUE);
		}
		else if (sleep_monst(mdef, dmg, -1)) {
			if (vis&VIS_MAGR) {
				if (attk->aatyp == AT_GAZE)
					Sprintf(buf, "is put to sleep by %s gaze.",
					(youagr ? "your" : s_suffix(mon_nam(magr))));
				else if (attk->aatyp == AT_WDGZ)
					Sprintf(buf, "is put to sleep under %s gaze.",
					(youagr ? "your" : s_suffix(mon_nam(magr))));
			}
			else
				Sprintf(buf, "falls asleep!");
			if (vis&VIS_MDEF) {
				pline("%s %s", Monnam(mdef), buf);
			}
			mdef->mstrategy &= ~STRAT_WAITFORU;
			slept_monst(mdef);
		}
		break;

		/* cancellation */
	case AD_CNCL:
		if (cancel_monst(mdef, (struct obj *)0, FALSE, TRUE, FALSE, !rn2(4) ? rnd(mlev(magr)) : 0)) {
			if (youdef) {
				if (vis&VIS_MAGR)
				{
					pline("%s stares at you.", Monnam(magr));
					pline("Your magic fades.");
				}
				else
					You_feel("your magic fade.");
			}
			else {
				/* no message for monsters being cancelled??*/;
			}
		}
		break;

		/* study */
	case AD_STDY:
		if (!youagr) {
			int * study = (youdef ? &(u.ustdy) : &(mdef->mstdy));

			if (dmg > *study) {	// reduce message spam by only showing when study is actually increased
				if (is_orc(pa))
					pline("%s curses and urges %s followers on.", Monnam(magr), mhis(magr));
				else if (pa->mtyp == PM_LEGION || pa->mtyp == PM_LEGIONNAIRE)
					/* no message */;
				else if (vis&VIS_MAGR) {
					pline("%s studies %s with a level stare.",
						Monnam(magr),
						(youdef ? "you" : mon_nam(mdef))
						);
				}
				//else no message
				/* add to study */
				*study = dmg;
			}
		}
		else if (vis&VIS_MDEF) {	/* how can you study something if you can't see it? */
			/* assumed that you studying a monster is *not* something that can or will be spammed all the time */
			You("study %s intently.", mon_nam(mdef));
			mdef->mstdy = max(mdef->mstdy, dmg);
		}
		else
			return MM_MISS;
		break;

		/* unnerve (bad moral) */
	case AD_UNRV:
		{
			int * moral = (youdef ? &(u.uencouraged) : &(mdef->encouraged));

			if (dmg > -1*(*moral)) {	// reduce message spam by only showing when study is actually increased
				if(youdef){
					pline("%s looks unnervingly familliar!", Monnam(magr));
				}
				else if(vis&VIS_MAGR){
					pline("%s looks unnerved.", Monnam(mdef));
				}
				//else no message
				/* add to moral */
				*moral = max(-dmg, *moral-dmg);
			}
		}
		break;

		/* luck drain */
	case AD_LUCK:
		/* 4/5 chance to succeed */
		if (maybe_not && !rn2(5))
			return MM_MISS;
		/* no effect on monsters */
		//Use encouragement code to give monster target a -1
		if (!youdef){
			if(vis&VIS_MAGR){
				pline("%s glares ominously at %s!", Monnam(magr), mon_nam(mdef));
			}
			mdef->encouraged = max(mdef->encouraged-1,-13);
		}
		else {
			/* assumes you are defending */
			pline("%s glares ominously at you!", Monnam(magr));

			/* misc protections */
			if ((uwep && !uwep->cursed && confers_luck(uwep)) ||
				(stone_luck(TRUE) > 0 && rn2(4))) {
				pline("Luckily, you are not affected.");
			}
			else {
				You_feel("your luck running out.");
				change_luck(-1 * dmg);
			}
			stop_occupation();
		}
		break;

		/* weeping angel gaze */
	case AD_BLNK:
		/* special case: Weeping angels using their gaze attack on each other has unfortunate effects for both of them */
		if (is_weeping(pd)) {
			if (vis&VIS_MAGR && vis&VIS_MDEF) {
				pline("%s and %s are permanently quantum-locked!", Monnam(mdef), mon_nam(magr));
			}
			monstone(mdef);
			monstone(magr);
			return (MM_DEF_DIED | MM_AGR_DIED);
		}
		/* otherwise, weeping angels' gazes only affect the player */
		if (!youdef)
			return MM_MISS;
		/* 4/5 chance to succeed */
		if (maybe_not && !rn2(5))
			return MM_MISS;
		/* put on cooldown */
		/* In practice, this will be zeroed when a new movement ration is handed out, and acts to make sure Blink can only be used once per round. */
		if (cooldown && !youagr) {
			if (magr->mspec_used)
				return MM_MISS;
			else
				magr->mspec_used = 10;
		}

		dmg = d(1, 4);
		if (!Reflecting) {
			pline("%s reflection in your mind weakens you.", s_suffix(Monnam(magr)));
			stop_occupation();
			exercise(A_INT, TRUE);
			exercise(A_WIS, FALSE);
		}
		else {
			static long lastverbed = 0L;
			if (flags.verbose && lastverbed + 10 < moves)
				/* Since this message means the player is unaffected, limit
				its occurence to preserve flavor but avoid message spam */
				pline("%s is covering its face.", Monnam(magr));
			lastverbed = moves;
			dmg = 0;
		}
		if (dmg) {
			int temparise = u.ugrave_arise;
			u.wimage += dmg;
			if (u.wimage > 10) u.wimage = 10;
			u.ugrave_arise = PM_WEEPING_ANGEL;
			killer = "the sight of a weeping angel";
			killer_format = KILLED_BY;
			int result = xdamagey(magr, mdef, attk, dmg);
			/*If the player surived the gaze attack, restore the value of arise*/
			u.ugrave_arise = temparise;
			return result;
		}
		break;

	case AD_WISD:
		/* cancels if no damage? */
		if (!dmg)
			return MM_MISS;
		/* non-players should get hit by a basic confusion gaze */
		/* that swap should have happened already */
		if (!youdef) {
			impossible("AD_WISD gaze not being swapped out vs non-player?");
			return MM_MISS;
		}
		/* put on cooldown */
		/* In practice, this will be zeroed when a new movement ration is handed out, and acts to make sure this can only be used once per round. */
		if (!youagr) {
			if (magr->mspec_used)
				return MM_MISS;
			else
				magr->mspec_used = 4;
		}

		/* assumes only player defending now */
		pline("Blasphemous geometries assault your sanity!");
		if (u.sealsActive&SEAL_HUGINN_MUNINN){
			unbind(SEAL_HUGINN_MUNINN, TRUE);
		}
		else {
			while (!(ABASE(A_WIS) <= ATTRMIN(A_WIS)) && dmg > 0) {
				dmg--;
				(void)adjattrib(A_WIS, -1, TRUE);
				change_usanity(-1, FALSE);
				forget(4);	/* lose 4% of memory per point lost*/
				exercise(A_WIS, FALSE);
				/* Great Cthulhu permanently drains wisdom */
				if ((pa->mtyp == PM_GREAT_CTHULHU) && (AMAX(A_WIS) > ATTRMIN(A_WIS)))
					AMAX(A_WIS) -= 1;
			}
			if (dmg > 0) {
				You("tear at yourself in horror!"); //assume always able to damage self
				change_usanity(-1*dmg, TRUE);
				xdamagey(magr, mdef, attk, dmg*10);
			}
			if(magr->mtyp == PM_GREAT_CTHULHU)
				u.umadness |= MAD_SPIRAL;
		}
		break;

	/*crushing pain*/
	case AD_PAIN:
		if (distmin(x(magr), y(magr), x(mdef), y(mdef)) > BOLT_LIM)
			return MM_MISS;

		if (youdef) {
			Your("mind is imploding from the sight of %s visage!", s_suffix(mon_nam(magr)));
			// make_blinded((long)dmg, FALSE);
			// stop_occupation();
			// make_stunned((long)d(1, 3), TRUE);
			if(!HScreaming){
				if (!is_silent(pd)){
					You("%s from the pain!", humanoid_torso(pd) ? "scream" : "shriek");
				}
				else {
					You("writhe in pain!");
				}
				change_usanity(-dmg, TRUE); //May result in screaming being set, or other minor madness
				HScreaming += dmg;
			}
			else {
				HScreaming += 2;
				change_usanity(-2, FALSE);
			}
			if(!u.veil && !mvitals[monsndx(magr->data)].vis_insight){
				mvitals[monsndx(magr->data)].vis_insight = TRUE;
				uchar insight = u_insight_gain(magr)+1;
				mvitals[monsndx(magr->data)].insight_gained += insight;
				change_uinsight(insight);
			}
			stop_occupation();
		}
		else {
			if((nonliving(pd) && !is_android(pd)) 
				|| has_template(mdef, TOMB_HERD) /*not a statue-piloting thingy */
				|| is_primordial(pd)
				|| is_alienist(pd)
				|| is_great_old_one(pd)
			){
				return MM_MISS;
			}
			unsigned int oldspeed = mdef->mspeed;
			if(mdef->movement > 0 && mdef->mcanmove){
				if (!is_silent_mon(mdef)){
					if (canseemon(mdef))
						pline("%s %s in pain!", Monnam(mdef), humanoid_torso(mdef->data) ? "screams" : "shrieks");
					else You_hear("%s %s in pain!", mdef->mtame ? noit_mon_nam(mdef) : mon_nam(mdef), humanoid_torso(mdef->data) ? "screaming" : "shrieking");
				}
				else {
					if (canseemon(mdef))
						pline("%s writhes in pain!", Monnam(mdef));
				}
			}

			mdef->movement = max(mdef->movement - 2*dmg, -12);
		}
		/* apply half damage (both) */
		dmg = reduce_dmg(mdef,dmg,TRUE,TRUE);
		return xdamagey(magr, mdef, attk, dmg);
		break;

	case AD_SEDU:
		if (!youdef)
			return MM_MISS;
		/* STRAIGHT COPY-PASTE FROM ORIGINAL */
		else {
			boolean engring = FALSE;
			if ((uleft  && uleft->otyp == find_engagement_ring()) ||
				(uright && uright->otyp == find_engagement_ring()))
				engring = TRUE;
			if (u.sealsActive&SEAL_ANDROMALIUS) break;
			if (distu(magr->mx, magr->my) > 1 ||
				magr->mcan ||
				!umetgaze(magr) ||
				is_blind(magr)
				) return MM_MISS;//fail
			//else
			if (pa->mtyp == PM_DEMOGORGON){
				You("have met the twin gaze of Demogorgon, Prince of Demons!");
				You("feel his command within you!");
				buf[0] = '\0';
				steal(magr, buf, FALSE, FALSE);
				m_dowear(magr, FALSE);
				return MM_HIT;
			}
			if ((pa->mtyp == PM_FIERNA || pa->mtyp == PM_PALE_NIGHT) && rnd(20) < 15) return MM_HIT;
			if (pa->mtyp == PM_ALRUNES){
				if (MON_WEP(magr) && rn2(20)) return MM_HIT;
			}
			if (is_animal(pa)) {
				xyhitmsg(magr, mdef, attk);
				if (magr->mcan) break;
				/* Continue below */
			}
			else if (dmgtype(youracedata, AD_SEDU)
#ifdef SEDUCE
				|| dmgtype(youracedata, AD_SSEX) || dmgtype(youracedata, AD_LSEX)
#endif
				) {
				pline("%s %s.", Monnam(magr), magr->minvent ?
					"brags about the goods some dungeon explorer provided" :
					"makes some remarks about how difficult theft is lately");
				if (!tele_restrict(magr)) (void)rloc(magr, TRUE);
				return MM_AGR_STOP;
			}
			else if (magr->mcan || engring || Chastity) {
				if (!Blind) {
					pline("%s tries to %s you, but you seem %s.",
						Adjmonnam(magr, "plain"),
						(is_neuter(pa) || flags.female == magr->female) ? "charm" : "seduce",
						(is_neuter(pa) || flags.female == magr->female) ? "unaffected" : "uninterested");
				}
				if (rn2(3)) {
					if (!tele_restrict(magr)) (void)rloc(magr, TRUE);
					return MM_AGR_STOP;
				}
				break;
			}
			buf[0] = '\0';
			switch (steal(magr, buf, FALSE, FALSE)) {
			case -1:
				return MM_AGR_DIED;
			case 0:
				break;
			default:
				if (!is_animal(pa) && !tele_restrict(magr))
					(void)rloc(magr, TRUE);
				if (is_animal(pa) && *buf) {
					if (canseemon(magr))
						pline("%s tries to %s away with %s.",
						Monnam(magr),
						locomotion(magr, "run"),
						buf);
				}
				monflee(magr, 0, FALSE, FALSE);
				return MM_AGR_STOP;
			}
			m_dowear(magr, FALSE);
		}
		break;
	case AD_SSEX:
		if (!youdef)
			return MM_MISS;
		/* STRAIGHT COPY-PASTE FROM ORIGINAL */
		else {
			if(Chastity)
				break;
			if ((uleft  && uleft->otyp == find_engagement_ring()) ||
				(uright && uright->otyp == find_engagement_ring()))
				break;
			if (could_seduce(magr, &youmonst, attk) == 1
				&& !magr->mcan
				&& !is_blind(magr)
				&& umetgaze(magr)
				&& distu(magr->mx, magr->my) == 1)
			if (doseduce(magr))
				return MM_AGR_STOP;
		}
		break;

	case AD_BLAS:
		/* 4/5 chance to succeed */
		if (maybe_not && !rn2(5))
			return MM_MISS;
		/* put on cooldown */
		/* In practice, this will be zeroed when a new movement ration is handed out, and acts to make sure it can only be used once per round. */
		if (cooldown && !youagr) {
			if (magr->mspec_used)
				return MM_MISS;
			else
				magr->mspec_used = 7;
		}
		/* the player is more detailed */
		if (!youdef){
			if(rn2(10)){
				struct attack fakespell = { AT_MAGC, AD_CLRC, 10, 7 };
				cast_spell(magr, mdef, &fakespell, LIGHTNING, mdef->mx, mdef->my);
				return MM_HIT;
			}
			else {
				struct attack alt_attk = {AT_HITS, AD_DISN, 7, 1};
				
				return xmeleehurty(magr, mdef, &alt_attk, &alt_attk, (struct obj **) 0, FALSE, 7, 20, vis, TRUE);
			}
		}
		else {
			/* angers a random pantheon god */
			int angrygod = align_to_god(A_CHAOTIC + rn2(3));
			u.ualign.record -= rnd(20);
			u.ualign.sins++;
			change_hod(rnd(20));
			godlist[angrygod].anger++;
			angrygods(angrygod);
		}
		break;

		/* MONSTER GENERATING GAZES, MVU ONLY */

	case AD_MIST:  // mi-go mist projector
		/* 1/5 chance to succeed */
		if (maybe_not && rn2(5))
			return MM_MISS;
		else {
			int typ = NON_PM;
			int quan;

			switch(pa->mtyp)
			{
				case PM_MIGO_SOLDIER:
					quan = rnd(4);
					typ = PM_FOG_CLOUD;
					if (cansee(magr->mx, magr->my)) You("see fog billow out from around %s.", mon_nam(magr));
					break;
				case PM_MIGO_PHILOSOPHER:
					quan = rnd(4);
					typ = PM_ICE_VORTEX;
					if (cansee(magr->mx, magr->my)) You("see whirling snow swirl out from around %s.", mon_nam(magr));
					break;
				case PM_MIGO_QUEEN:
					quan = rnd(2);
					typ = PM_STEAM_VORTEX;
					if (cansee(magr->mx, magr->my)) You("see scalding steam swirl out from around %s.", mon_nam(magr));
					break;
				case PM_ANCIENT_TEMPEST:
					quan = 1;
					switch(rn2(4)) {
						case 0:
							typ = PM_AIR_ELEMENTAL;
							if (cansee(magr->mx, magr->my)) You("see a whisp of cloud swirl out from %s.", mon_nam(magr));
							break;
						case 1:
							typ = PM_WATER_ELEMENTAL;
							if (cansee(magr->mx, magr->my)) You("see rain coalesce and stride out from %s.", mon_nam(magr));
							break;
						case 2:
							typ = PM_LIGHTNING_PARAELEMENTAL;
							if (cansee(magr->mx, magr->my)) You("see lightning coalesce and strike out from %s.", mon_nam(magr));
							break;
						case 3:
							typ = PM_ICE_PARAELEMENTAL;
							if (cansee(magr->mx, magr->my)) You("see hail coalesce and stride out from %s.", mon_nam(magr));
							break;
					}
				default:
					quan = 1;
					typ = PM_FOG_CLOUD;
					if (cansee(magr->mx, magr->my)) You("see fog billow out from around %s.", mon_nam(magr));
					break;
			}

			if (typ != NON_PM) {
				struct monst * mtmp;
				int mmflags = MM_ADJACENTOK|MM_ADJACENTSTRICT;
				if (get_mx(magr, MX_ESUM)) mmflags |= MM_ESUM;

				while(quan--) {
					mtmp = makemon(&mons[typ], magr->mx, magr->my, mmflags);
					if (mtmp && (mmflags&MM_ESUM))
						mark_mon_as_summoned(mtmp, magr, ESUMMON_PERMANENT, 0);
				}
			}

			return MM_AGR_STOP; // if a mi-go fires a mist projector, it can take no further actions that turn
		}
		break;
	case AD_SPOR:
		/* release a spore */
		if (!magr->mcan && 
			!is_fern_sprout(pa) ? !rn2(2) : !rn2(4)) {
			coord mm;
			int typ;
			if(has_template(magr, CORDYCEPS)){
				typ = PM_BALLISTOSPORE;
			}
			else switch (pa->mtyp) {
				case PM_DUNGEON_FERN:
				case PM_DUNGEON_FERN_SPROUT:
				case PM_BAELNORN:
					typ = PM_DUNGEON_FERN_SPORE;
					break;
				case PM_SWAMP_FERN:
				case PM_SWAMP_FERN_SPROUT:
					typ = PM_SWAMP_FERN_SPORE;
					break;
				case PM_BURNING_FERN:
				case PM_BURNING_FERN_SPROUT:
					typ = PM_BURNING_FERN_SPORE;
					break;
				default:
					typ = NON_PM;
					break;
			}
			if (typ != NON_PM) {
				struct monst * mtmp;
				int mmflags = MM_ADJACENTOK|MM_ADJACENTSTRICT;
				mm.x = magr->mx; mm.y = magr->my;
				enexto(&mm, mm.x, mm.y, &mons[typ]);
				if (get_mx(magr, MX_ESUM)) mmflags |= MM_ESUM;

				mtmp = makemon(&mons[typ], mm.x, mm.y, mmflags);

				if (mtmp) {
					if (mmflags&MM_ESUM)
						mark_mon_as_summoned(mtmp, magr, ESUMMON_PERMANENT, 0);
					if (canseemon(magr))
						pline("%s releases a spore!", Monnam(magr));
				}
			}
		}
		break;
	case AD_WTCH:{
		 //Watcher in the Water's tentacle spawn and retreat behavior
		 int ltnt = 0, stnt = 0;
		 struct monst *tmon;
		 if (distmin(u.ux, u.uy, magr->mx, magr->my) <= 2 && !(magr->mflee)){
			 magr->mflee = 1;
			 magr->mfleetim = 2;
		 }
		 for (tmon = fmon; tmon; tmon = tmon->nmon){
			 if (pa->mtyp == PM_WATCHER_IN_THE_WATER){
				 if (tmon->mtyp == PM_SWARM_OF_SNAKING_TENTACLES) stnt++;
				 else if (tmon->mtyp == PM_LONG_SINUOUS_TENTACLE) ltnt++;
			 }
			 else if (pa->mtyp == PM_KETO){
				 if (tmon->mtyp == PM_WIDE_CLUBBED_TENTACLE) ltnt++;
			 }
		 }
		 if (pa->mtyp == PM_WATCHER_IN_THE_WATER){
			 if (stnt<6){
				 makemon(&mons[PM_SWARM_OF_SNAKING_TENTACLES], magr->mx, magr->my, MM_ADJACENTOK | MM_ADJACENTSTRICT | MM_NOCOUNTBIRTH);
			 }
			 else if (ltnt<2){
				 makemon(&mons[PM_LONG_SINUOUS_TENTACLE], magr->mx, magr->my, MM_ADJACENTOK | MM_ADJACENTSTRICT | MM_NOCOUNTBIRTH);
			 }
		 }
		 else if (pa->mtyp == PM_KETO){
			 if (ltnt<2){
				 makemon(&mons[PM_WIDE_CLUBBED_TENTACLE], magr->mx, magr->my, MM_ADJACENTOK | MM_ADJACENTSTRICT | MM_NOCOUNTBIRTH);
			 }
		 }
	}break;
	case AD_SPHR:  // create spheres
		if (magr->mspec_used)
			return MM_MISS;
		else {
			int i = 0;
			int n;
			int mid;
			struct monst *mtmp;
			int maketame = ((magr->mtame || youagr) ? MM_EDOG : 0);
			int makesum = MM_ESUM;
			if(has_template(magr, CORDYCEPS)){
				mid = PM_BALLISTOSPORE;
				makesum = 0;
			}
			else if(pa->mtyp == PM_CANDLE_TREE || pa->mtyp == PM_FLAMING_ORB){
				mid = PM_FLAMING_SPHERE;
			}
			else {
				switch(rnd(3)){
					case 1:
						mid = PM_FLAMING_SPHERE;
					break;
					case 2:
						mid = PM_SHOCKING_SPHERE;
					break;
					case 3:
						mid = PM_FREEZING_SPHERE;
					break;
				}
			}
			for(n = dmg; n > 0; n--){
				mtmp = makemon(&mons[mid], x(magr), y(magr), MM_ADJACENTOK|MM_ADJACENTSTRICT|maketame|makesum);
				if (mtmp) {
					/* time out */
					if(makesum)
						mark_mon_as_summoned(mtmp, magr, mlev(magr) + rnd(mlev(magr)), 0);
					/* can be peaceful */
					if(magr->mpeaceful)
						mtmp->mpeaceful = TRUE;
					/* can be tame */
					if (maketame) {
						initedog(mtmp);
					}
					/* bonus movement */
					mtmp->movement = 3*NORMAL_SPEED;
				}
			}
			//Breath timer
			if(dmg > 1)
				magr->mspec_used = 10 + rn2(20);
		}
		break;
	default:
		impossible("unhandled gaze type %d from %s in xgazey", adtyp, !magr ? "no magr??" : mon_nam(magr));
	}
	/* if we got to the end, we didn't abort early, which means something should have happened */
	result |= MM_HIT;
	return result;
}

int
apply_hit_effects(magr, mdef, otmp, msgr, basedmg, plusdmgptr, truedmgptr, dieroll, hittxt, printmessages, direct_weapon)
struct monst * magr;
struct monst * mdef;
struct obj * otmp;
struct obj * msgr;
int basedmg;
int * plusdmgptr;
int * truedmgptr;
int dieroll;
boolean * hittxt;
boolean printmessages;
boolean direct_weapon;
{
	int result = MM_HIT;
	int tmpplusdmg;
	int tmptruedmg;

	/* artifact and object properties and misc */
	tmpplusdmg = tmptruedmg = 0;
	result = special_weapon_hit(magr, mdef, otmp, msgr, basedmg, &tmpplusdmg, &tmptruedmg, dieroll, hittxt, printmessages);
	*plusdmgptr += tmpplusdmg;
	*truedmgptr += tmptruedmg;
	if ((result & (MM_DEF_DIED | MM_DEF_LSVD | MM_AGR_STOP)) || (result == MM_MISS))
		return result;

	/* otyp */
	if (spec_prop_otyp(otmp)) {
		tmpplusdmg = tmptruedmg = 0;
		otyp_hit(magr, mdef, otmp, basedmg, &tmpplusdmg, &tmptruedmg, dieroll, hittxt, printmessages, direct_weapon);
		*plusdmgptr += tmpplusdmg;
		*truedmgptr += tmptruedmg;
		if ((result & (MM_DEF_DIED | MM_DEF_LSVD | MM_AGR_STOP)) || (result == MM_MISS))
			return result;
	}
	return result;
}


/* helpful hmon callers */

/* hit mdef with some object that was launched in some way with no attacker of any sort */
int
hmon_with_unowned_obj(mdef, obj_p, dieroll)
struct monst * mdef;
struct obj ** obj_p;
int dieroll;
{
	return hmon_general(
		(struct monst *)0,	/* no attacker */
		mdef,				/* mdef is the defender */
		(struct attack *)0,	/* no attack */
		(struct attack *)0,	/* no attack */
		obj_p,				/* hitting mdef with obj */
		(void *)0,			/* no launcher*/
		HMON_PROJECTILE|HMON_FIRED,			/* obj should deal full thrown/fired damage */
		0,					/* no damage override */
		0,					/* no bonus damage */
		TRUE,				/* yes, print hit message */
		dieroll,			/* use given dieroll */
		FALSE,				/* not recursed */
		-1);				/* calculate visibility */
}
/* hit mdef with a trap */
int
hmon_with_trap(mdef, obj_p, trap, type, dieroll)
struct monst * mdef;
struct obj ** obj_p;
struct trap * trap;
int type;
int dieroll;
{
	/* melee traps print their own messages, while ranged traps rely on hmon to print hitmessages */
	boolean printmsg;
	if (type&HMON_PROJECTILE)
		printmsg = TRUE;
	else if (type&HMON_WHACK)
		printmsg = FALSE;
	else {
		impossible("hmon_with_trap called with neither WHACK nor FIRED, %d", type);
		printmsg = TRUE;
	}

	return hmon_general(
		(struct monst *)0,	/* no attacker */
		mdef,				/* mdef is the defender */
		&basicattack,		/* get attack-damage bonuses, etc. Does not use the 1d4. */
		&basicattack,		/* get attack-damage bonuses, etc. Does not use the 1d4. */
		obj_p,				/* hitting mdef with obj */
		trap,				/* trap that did the hitting */
		HMON_TRAP|type,		/* trap responsible, using given type */
		0,					/* no damage override */
		0,					/* no bonus damage */
		printmsg,			/* maybe print hit message */
		dieroll,			/* use given dieroll */
		FALSE,				/* not recursed */
		-1);				/* calculate visibility */
}

/* hmon_general
 * 
 * Like as it was in uhitm.c, this is a wrapper so that ghod_hitsu() and angry_guards()
 * are called after the player hits, while letting hmoncore have messy returns wherever it wants
 */
int
hmon_general(magr, mdef, attk, originalattk, weapon_p, vpointer, hmoncode, flatbasedmg, monsdmg, dohitmsg, dieroll, recursed, vis)
struct monst * magr;			/* attacker */
struct monst * mdef;			/* defender */
struct attack * attk;			/* attack structure to use -- if this does not exist, we MUST have a weapon */
struct attack * originalattk;	/* original attack structure, used for messages */
struct obj ** weapon_p;			/* pointer to weapon to hit with */
void * vpointer;				/* additional /whatever/, type based on hmoncode. */
int hmoncode;					/* what kind of pointer is vpointer, and what is it doing? (hack.h) */
int flatbasedmg;				/* if >0, REPLACE basedmg with this value -- typically used for unusual weapon hits like throwing something upwards */
int monsdmg;					/* flat damage amount to add onto other effects -- for monster attacks */
boolean dohitmsg;				/* print hit message? */
int dieroll;					/* 1-20 accuracy dieroll, used for special effects */
boolean recursed;				/* True for all but one attacks when 1 object is hitting >1 times in 1 attack. If so, avoid duplicating some messages and effects. */
int vis;						/* True if action is at all visible to the player */
{
	return hmon_general_modifiers(magr, mdef, attk, originalattk, weapon_p, vpointer, hmoncode, flatbasedmg, monsdmg, dohitmsg, dieroll, recursed, vis, 0L);
}

// magr;			/* attacker */
// mdef;			/* defender */
// attk;			/* attack structure to use */
// originalattk;	/* original attack structure, used for messages */
// weapon_p;			/* pointer to weapon to hit with */
// vpointer;				/* additional /whatever/, type based on hmoncode. */
// hmoncode;					/* what kind of pointer is vpointer, and what is it doing? (hack.h) */
// flatbasedmg;				/* if >0, REPLACE basedmg with this value -- currently unused. SCOPECREEP: use hmon for things like throwing an object upwards */
// monsdmg;					/* flat damage amount to add onto other effects -- for monster attacks */
// dohitmsg;				/* print hit message? */
// dieroll;					/* 1-20 accuracy dieroll, used for special effects */
// recursed;				/* True for all but one attacks when 1 object is hitting >1 times in 1 attack. If so, avoid duplicating some messages and effects. */
// vis;						/* True if action is at all visible to the player */
int
hmon_general_modifiers(struct monst *magr, struct monst *mdef, struct attack *attk, struct attack *originalattk, struct obj **weapon_p, void *vpointer, int hmoncode, int flatbasedmg, int monsdmg, boolean dohitmsg, int dieroll, boolean recursed, int vis, unsigned long modifier_flags)
{
	int result;
	boolean u_anger_guards;

	if (magr == &youmonst &&
		mdef->mpeaceful &&
		(mdef->ispriest || mdef->isshk ||
		mdef->mtyp == PM_WATCHMAN ||
		mdef->mtyp == PM_WATCH_CAPTAIN)
		)
		u_anger_guards = TRUE;
	else
		u_anger_guards = FALSE;

	result = hmoncore(magr, mdef, attk, originalattk, weapon_p, vpointer, hmoncode, flatbasedmg, monsdmg, dohitmsg, dieroll, recursed, vis, modifier_flags);

	/* reset killer */
	killer = 0;

	if (magr == &youmonst && mdef->ispriest && !rn2(2))
		ghod_hitsu(mdef);
	if (u_anger_guards)
		(void)angry_guards(!flags.soundok);

	return result;
}

// magr;			/* attacker */
// mdef;			/* defender */
// attk;			/* attack structure to use */
// originalattk;	/* original attack structure, used for messages */
// weapon_p;			/* pointer to weapon to hit with */
// vpointer;				/* additional /whatever/, type based on hmoncode. */
// hmoncode;					/* what kind of pointer is vpointer, and what is it doing? (hack.h) */
// flatbasedmg;				/* if >0, REPLACE basedmg with this value -- currently unused. SCOPECREEP: use hmon for things like throwing an object upwards */
// monsdmg;					/* flat damage amount to add onto other effects -- for monster attacks */
// dohitmsg;				/* print hit message? */
// dieroll;					/* 1-20 accuracy dieroll, used for special effects */
// recursed;				/* True for all but one attacks when 1 object is hitting >1 times in 1 attack. If so, avoid duplicating some messages and effects. */
// vis;						/* True if action is at all visible to the player */

int
hmoncore(struct monst *magr, struct monst *mdef, struct attack *attk, struct attack *originalattk, struct obj **weapon_p, void *vpointer, int hmoncode, int flatbasedmg, int monsdmg, boolean dohitmsg, int dieroll, boolean recursed, int vis, unsigned long modifier_flags)
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct permonst * pa = (magr ? (youagr ? youracedata : magr->data) : (struct permonst *)0);
	struct permonst * pd = youdef ? youracedata : mdef->data;
	struct obj * weapon = weapon_p ? *(weapon_p) : (struct obj *)0;

	boolean staggering_strike = FALSE;
	boolean shattering_strike = FALSE;
	boolean disarming_strike = FALSE;
	boolean stunning_strike = FALSE;
	boolean braced_weapon = FALSE;
	int jousting = 0;		/* can be 1 (joust), 0 (ordinary hit), -1 (joust and lance breaks) */
	int sneak_dice = 0;
	int sneak_attack = 0;
#define SNEAK_HIDDEN	0x01
#define SNEAK_BEHIND	0x02
#define SNEAK_BLINDED	0x04
#define SNEAK_TRAPPED	0x08
#define SNEAK_HELPLESS	0x10
#define SNEAK_JUYO		0x20
#define SNEAK_SUICIDAL	0x40
#define SNEAK_OPEN	0x80
	long silverobj = 0L,
		jadeobj = 0L,
		ironobj = 0L,
		grnstlobj = 0L,
		holyobj = 0L,
		unholyobj = 0L,
		unblessedobj = 0L,
		uuvuglory = 0L,
		otherobj = 0L;
	int poisons_resisted = 0,
		poisons_minoreff = 0,
		poisons_majoreff = 0,
		poisons_wipedoff = 0;
	struct obj * poisonedobj;	/* object that is poisoned responsible for above poisons_X variables -- set once, should not be reset after */
	int poisons = 0;
	boolean swordofblood = FALSE;

	boolean resisted_weapon_attacks = FALSE;
	boolean resisted_attack_type = FALSE;
	boolean resisted_thick_skin = FALSE;
	boolean partly_resisted_thick_skin = FALSE;
	boolean misotheistic_major = FALSE;
	int attackmask = 0;
	static int warnedotyp = -1;
	static struct permonst *warnedptr = 0;

	static char killerbuf[BUFSZ];		/* only for use with killer */
	char buf[BUFSZ];

	boolean phase_armor = FALSE;
	boolean valid_weapon_attack = FALSE;
	boolean fake_valid_weapon_attack = FALSE;
	boolean invalid_weapon_attack = FALSE;
	boolean unarmed_punch = FALSE;
	boolean unarmed_kick = FALSE;
	boolean unarmed_butt = FALSE;
	boolean natural_strike = FALSE;
	boolean ulightsaberhit = FALSE;

	boolean destroy_one_magr_weapon = FALSE;	/* destroy one of magr's weapons */
	boolean destroy_all_magr_weapon = FALSE;	/* destroy all of magr's weapon */

	boolean hittxt = FALSE;
	boolean lethaldamage = FALSE;
	boolean mercy_blade = FALSE;
	boolean mind_blade = FALSE;

	boolean melee = (hmoncode & HMON_WHACK);
	boolean thrust = (hmoncode & HMON_THRUST);
	boolean fired = (hmoncode & HMON_FIRED);
	boolean thrown = (hmoncode & HMON_PROJECTILE);
	boolean trapped = (hmoncode & HMON_TRAP);
	boolean misthrown = (thrown && weapon && is_ammo(weapon) && weapon->oclass != GEM_CLASS && !fired);

	struct obj * launcher = (struct obj *)(fired ? vpointer : 0);
	struct trap * trap = (struct trap *)(trapped ? vpointer : 0);
	if (trap) launcher = 0; /* trap takes precedence over launcher */

	boolean real_attack = (attk && (attk->aatyp == AT_WEAP || attk->aatyp == AT_XWEP || attk->damn > 0 || attk->damd > 0));

	struct obj tempwep;	/* used to save the data of an object before it gets destroyed, for things like naming */
	struct obj * otmp;	/* generic object pointer -- variable */
	long slot = 0L;		/* slot, either the weapon pointer (W_WEP) or armor -- variable */
	long rslot = 0L;	/* slot, dedicated to rings (left and right) -- set at start, should not be reset */

	/* pick the most correct ring slot */
	if(!youagr || !magr)
		rslot = 0L; /* no attacker or the attacker isn't you (redundant for now) */
	else if(!melee || !attk || attk->aatyp == AT_MARI)
		rslot = 0L; /* no attack, not in melee, or marilith: not a ring-hand */
	else if(youagr ? uarms : which_armor(magr, W_ARMS))
		rslot = W_RINGR; /* sadly, all characters are right-handed */
	else if(u.twoweap){
		if(attk->aatyp == AT_XWEP || attk->aatyp == AT_XSPR || attk->offhand)
			rslot = W_RINGL;
		else 
			rslot = W_RINGR;
	}
	else rslot = rn2(2) ? W_RINGR : W_RINGL;

	int precision_mult = 0;	/* damage multiplier for precision weapons */

	if (vis == -1)
		vis = getvis(magr, mdef, 0, 0);

	/* partial damage counters */
	int basedmg = 0;	/* base weapon/unarmed damage */
	int artidmg = 0;	/* bonus damage from ARTIFACT HIT block */
	int bonsdmg = 0;	/* various bonus damage */
	int snekdmg = 0;	/* sneak attack bonus damage */
	int jostdmg = 0;	/* jousting bonus damage */
	int tratdmg = 0;	/* trait bonus damage */
	//  monsdmg			/* passed to this function */
	int subtotl = 0;	/* subtotal of above damages */
	int seardmg = 0;	/* X-hating */
	int poisdmg = 0;	/* poisons */
	int elemdmg = 0;	/* artifacts, objproperties, and clockwork heat */
	int specdmg = 0;	/* sword of blood; sword of mercury; clawmark thought */
	int totldmg = 0;	/* total of subtotal and below */
	
	int wepspe = weapon ? weapon->spe : 0;		/* enchantment of weapon, saved in case it goes poof. */

	int result;	/* value to return */

	/* FLOW MAP */
	/*
	 * Figure Out What Applies
	 *  - (in)valid weapon attack?
	 *  - unarmed punch or natural strike?
	 *  - find out if these apply:
	 *     - precision multiplier
	 *     - sneak attack (calc dmg)
	 *     - jousting (calc dmg)(uhitm)
	 *     - staggering strike (uhitm)
	 *     - shattering strike (uhitm)
	 *     - iron/silver/holy/unholy hating (calc dmg)
	 *     - poison (calc dmg vs monsters)
	 *     - clockwork heat (calc dmg)(uhitm)
	 *     - sword of blood (calc dmg)
	 *  - apply these with no message and note that they were applied:
	 *     - zombification (mhitm)
	 *
	 * Calculate Damage
	 *  - Physical Damage
	 *     - base damage
	 *  - Set Phasing
	 *  - ARTIFACT HIT - THIS PRINTS MESSAGES IN MANY CASES AND UNFORTUNATELY CAN'T BE SUPPRESSED
	 *     - add to base damage
	 *     - check if creature is still alive at this point
	 *  - Bonus Reducable Damage
	 *     - sneak attack
	 *     - jousting
	 *     - staggering
	 *     - shattering strike
	 *  - Reduce Damage
	 *     - insubstantial (note: we are already checking that this is hitting, but many things will only do X-hating damage -- catch this)
	 *     - weapon-resist
	 *     - half-phys damage  AND/OR  type-resistance
	 *     - DR
	 *  - Bonus Non-Reducable Damage
	 *     - iron/silver/holy/unholy hating (already calculated)
	 *     - poison (already calculated vs monsters, still don't call poisoned() since it will print messages)
	 *     - clockwork heat (already calculated)
	 *
	 * Hit Message
	 *  - suppress if any of the following apply:
	 *     - !dohitsmg
	 *     - artifacthit printed a message
	 *     - jousting (we always want the jousting message, and it to replace the hit message)
	 *     - staggering strike (we always want the staggering message, and it to replace the hit message)
	 *     - creature will be killed be calculated damage AND the player is attacking
	 *  - print hit message
	 *
	 * Additional Messages
	 *  - juyo sneak attack
	 *  - sneak attack (suppress if blow will kill)
	 *  - jousting
	 *  - shattering strike (suppress if blow will kill; don't shatter weapon if blow will kill)
	 *  - staggering strike (suppress if blow will kill)
	 *  - type-resistance (suppress if blow will kill)
	 *  - weapon-resistance (suppress if blow will kill)
	 *  - ~~~clockwork heat~~~ nevermind, this is silent
	 *  - iron/silver/holy/unholy hating
	 *  - poison (if vs player, NOW call poisoned() since it will print messages)
	 *
	 * Deal Damage
	 *  - possibly killing, with message
	 *
	 * Break Attacker's Weapon
	 *  - lances only
	 *
	 * Versus-living Effects
	 *  - confuse monster
	 *  - hurtle (staggering strike, jousting)
	 *  - pudding division
	 *  - uranium imp warping
	 *  - cutting worm tails
	 */

	/* set killer, if needed */
	if (!killer && (
		(!magr) ||				/* no attacker -- we really need a specific killer to avoid "killed by a died" */
		(thrown && weapon) ||	/* prefer "killed by an arrow" to "killed by a plains centaur" */
		(trapped)				/* prefer "killed by an arrow trap" to "killed by an arrow" */
		)) {
		if (trap) {
			killer_format = KILLED_BY;
			switch (trap->ttyp) {
			case ARROW_TRAP:
			case DART_TRAP:
				Sprintf(killerbuf, "%s trap", killer_xname(weapon));	/* killer_xname() adds a/an/the */
				killer = killerbuf;
				break;
			case BEAR_TRAP:
			case FLESH_HOOK:
				Sprintf(killerbuf, "%s", killer_xname(weapon));	/* killer_xname() adds a/an/the */
				killer = killerbuf;
				break;
			case ROCKTRAP:
				killer = "a falling rock trap";
				break;
			case ROLLING_BOULDER_TRAP:
				killer = "a rolling boulder trap";
				break;
			default:
				killer = "a trap";
				break;
			}
		}
		/* "killed by (a) <weapon> */
		else if (thrown && weapon) {
			killer_format = KILLED_BY;	/* killer_xname() adds a/an/the */
			Strcpy(killerbuf, killer_xname(weapon));
			killer = killerbuf;
		}
		/* nothing to work with, give basic message */
		else {
			killer = "died";
			killer_format = NO_KILLER_PREFIX;
		}
	}

	/* FIGURE OUT WHAT APPLIES */
	/* what kind of attack is being made? */
	if (weapon) {
		if(magr && CHECK_ETRAIT(weapon, magr, ETRAIT_BRACED)
			&& ROLL_ETRAIT(weapon, magr, TRUE, !rn2(10))
			&& magr != mdef
			&& ((!youdef && mdef->mprev_attk.x == sgn(x(magr) - x(mdef)) && mdef->mprev_attk.y == sgn(y(magr) - y(mdef)))
				|| (youdef && u.uattked && u.prev_dir.x == sgn(x(magr) - x(mdef)) && u.prev_dir.y == sgn(y(magr) - y(mdef)))
				)
		){
			braced_weapon = TRUE;
		}
		if (/* valid weapon */
			(valid_weapon(weapon)
				|| check_oprop(weapon, OPROP_BLADED)
				|| check_oprop(weapon, OPROP_SPIKED)
				|| weapon->oartifact == ART_WAND_OF_ORCUS
				|| weapon->otyp == WIND_AND_FIRE_WHEELS
			) &&
			/* being used with an attack action, or no attack action (which implies an oddly-launched object, like a falling boulder or something) */
			(!attk || weapon_aatyp(attk->aatyp)) &&
			/* isn't a misused launcher */
			(!is_launcher(weapon) ||
			is_melee_launcher(weapon)
			) &&
			/* isn't a misused polearm */
			(!is_bad_melee_pole(weapon) ||
			thrust ||
#ifdef STEED
			(youagr && u.usteed) ||
#endif
			(pa && melee_polearms(pa)) ||
			weapon->otyp == AKLYS ||
			check_oprop(weapon, OPROP_CCLAW)
			) &&
			/* isn't an unthrown missile */
			!((is_missile(weapon) || is_ammo(weapon)) && weapon->otyp != CHAKRAM && !thrown) &&
			/* isn't an unthrown Houchou */
			!(weapon->oartifact == ART_HOUCHOU && !thrown) &&
			/* isn't unthrowable ammo (ie, any ammo but rocks) being thrown but not fired*/
			!(misthrown)
			)
		{
			valid_weapon_attack = TRUE;

			if (youagr && is_lightsaber(weapon) && litsaber(weapon) && melee)
				ulightsaberhit = TRUE;
		}
		else {
			/* catch non-standard weapons here, rather than putting an || in the above if() */

			/* beartraps are real attacks */
			if (trap && melee && weapon)
				valid_weapon_attack = TRUE;
			/* Wand of Orcus makes for a melee weapon */
			else if (melee && weapon && weapon->oartifact == ART_WAND_OF_ORCUS)
				valid_weapon_attack = TRUE;
			/* Spellbooks are weapons if you have Paimon bound */
			else if (melee && weapon && weapon->oclass == SPBOOK_CLASS && youagr && u.sealsActive&SEAL_PAIMON)
				valid_weapon_attack = TRUE;
			/* shield bashes are possibly if and only if using the style (youagr only) */
			else if (melee && weapon && is_shield(weapon) && youagr && activeFightingForm(FFORM_SHIELD_BASH))
				valid_weapon_attack = TRUE;
			else if (melee && weapon && weapon->oartifact == ART_FINGERPRINT_SHIELD)
				valid_weapon_attack = TRUE;
			else
				invalid_weapon_attack = TRUE;
		}
	}
	else {
		/* unarmed punches are made with an attack action */
		if (
			(weapon_aatyp(attk->aatyp)) &&
			/* not thrown (how could this happen?) */
			melee)
			unarmed_punch = TRUE;
		/* monsdmg == 0 for a player's basic kick, monsdmg == -1 for a player's clumsy kick -- different from a horse's kick! */
		else if (attk->aatyp == AT_KICK && melee)
			unarmed_kick = TRUE;
		/* unlike kicks and weapon attacks, player headbutts get monsdmg and only use basedmg for enchantment */
		else if (attk->aatyp == AT_BUTT && melee) {
			unarmed_butt = TRUE;
			natural_strike = TRUE;
		}
		/* mercurial blades aren't spiritual rapiers */
		else if (attk->adtyp == AD_MERC && melee)
			fake_valid_weapon_attack = TRUE;
		/* monsters' "spiritual rapier" is a fake melee weapon */
		else if (spirit_rapier_at(attk->aatyp) && melee){
			natural_strike = TRUE;
			fake_valid_weapon_attack = TRUE;
		}
		/* standard monster attacks */
		else
			natural_strike = TRUE;
	}
	/* if the player is attacking with a wielded weapon, increment conduct */
	if (youagr && valid_weapon_attack && (melee || thrust)) {
		u.uconduct.weaphit++;
	}
	/* precision multiplier */
	if (fired && launcher &&								// Firing ammo from a launcher
		(objects[launcher->otyp].oc_skill == P_CROSSBOW	    // from a REAL crossbow (but not the Pen of the Void or the BFG, those would be brokenly strong)
		 || launcher->otyp == SNIPER_RIFLE					// or a sniper rifle
		 || (launcher->otyp == EVELYN && youagr && ACURR(A_DEX) > 18)	// or an evelyn at very high dex
		)
		&& !(noanatomy(pd))	// versus vulnerable targets
		){
		precision_mult = 1;
		/* R/U/B/S/E : 1x/1x/1x/2x/3x multiplier */
		if (youagr) {
			precision_mult += max(P_SKILL(objects[launcher->otyp].oc_skill) - 2, 0);
		}
		else {
			if(!magr->mformication && !magr->mscorpions && !magr->mcaterpillars)
				precision_mult += max(m_martial_skill(magr->data)-2, 0);
		}
		
		/* gnomes get bonus +1 mult */
		if (magr && (youagr ? Race_if(PM_GNOME) : is_gnome(pa)))
			precision_mult += 1;
		/* drow get bonus +1 mult for droven crossbows */
		if (magr && (youagr ? Race_if(PM_DROW) : is_drow(pa)) &&
			(launcher->otyp == DROVEN_CROSSBOW))
			precision_mult += 1;

		/* Wrathful Spider is not very precise. */
		if (launcher->oartifact == ART_WRATHFUL_SPIDER)
			precision_mult = min(precision_mult, 2);
	}
	//Bonus damage for shooting undead
	if(fired && launcher && youagr && Role_if(PM_UNDEAD_HUNTER) && !Upolyd
		&& (is_undead(pd) || is_were(pd))
	){
		bonsdmg += rnd(u.ulevel);
	}

	/* sneak attack -- attacker's number of dice */
	if (youagr) {
		/* player-only sources of sneak-attacking */
		if (Role_if(PM_ROGUE) && !Upolyd)
			sneak_dice++;
		if (Role_if(PM_UNDEAD_HUNTER) && !Upolyd)
			sneak_dice++;
		if (Role_if(PM_ANACHRONONAUT) && Race_if(PM_MYRKALFR) && !Upolyd)
			sneak_dice++;
		if (Role_if(PM_CONVICT) && !Upolyd && weapon && weapon->owornmask && weapon->otyp == SPOON)
			sneak_dice++;
		if (u.sealsActive&SEAL_ANDROMALIUS)
			sneak_dice++;
		if (ulightsaberhit && activeFightingForm(FFORM_JUYO))
			sneak_dice++;
		if (Role_if(PM_HEALER) && !Upolyd && weapon && weapon->owornmask && weapon->otyp == KNIFE)
			sneak_dice++;
	}
	if (magr && !youagr && is_backstabber(pa))
		sneak_dice++;
	if (weapon && weapon->owornmask && weapon->oartifact == ART_SPINESEEKER)
		sneak_dice++;
	if (weapon && weapon->owornmask && weapon->oartifact == ART_LOLTH_S_FANG)
		sneak_dice++;
	//Offhand attacks as well
	if (weapon && weapon->owornmask && weapon->otyp == BESTIAL_CLAW && active_glyph(BEASTS_EMBRACE))
		sneak_dice++;
	if (weapon && weapon->oartifact == ART_PEN_OF_THE_VOID && weapon->ovara_seals&SEAL_ANDROMALIUS)
		sneak_dice++;

	/* check sneak attack conditions -- defender's conditions must allow sneak attacking */
	if (magr &&
		!noanatomy(pd) &&
		!(youagr && u.uswallow) &&
		!(magr == mdef)
	){
		if (youagr) {
			if (!mdef->mcanmove || !mdef->mnotlaugh ||
				mdef->mstun || mdef->mconf || mdef->msleeping || mdef->mequipping)
				sneak_attack |= SNEAK_HELPLESS;
			if (mdef->msuicide)
				sneak_attack |= SNEAK_SUICIDAL;
			if (mdef->mopen)
				sneak_attack |= SNEAK_OPEN;
			if (is_blind(mdef))
				sneak_attack |= SNEAK_BLINDED;
			if (mdef->mtrapped)
				sneak_attack |= SNEAK_TRAPPED;
			if ((sgn(mdef->mx - u.ux) != sgn(mdef->mx - mdef->mux) && sgn(mdef->my - u.uy) != sgn(mdef->my - mdef->muy)) ||
				(mdef->mux == 0 && mdef->muy == 0) ||
				(mdef->mflee && pd->mtyp != PM_BANDERSNATCH))
				sneak_attack |= SNEAK_BEHIND;
			if (((mdef->mux != u.ux || mdef->muy != u.uy) &&
				((weapon && weapon == uwep && uwep->oartifact == ART_LIFEHUNT_SCYTHE && has_head(pd) && !is_unalive(pd))
				|| distmin(u.ux, u.uy, mdef->mx, mdef->my) > BOLT_LIM)))
				sneak_attack |= SNEAK_HIDDEN;
			if (ulightsaberhit && activeFightingForm(FFORM_JUYO))
				sneak_attack |= SNEAK_JUYO;	/* modifies a sneak attack; not sufficient on its own */
		}
		else if (youdef) {
			if (u.ustuck || u.utrap)
				sneak_attack |= SNEAK_TRAPPED;
			if (!canseemon(magr) && !sensemon(magr))
			{
				sneak_attack |= SNEAK_BEHIND;
				if (Blind)
					sneak_attack |= SNEAK_BLINDED;

			}
			if (multi || Stunned || Confusion || Sleeping)
				sneak_attack |= SNEAK_HELPLESS;
			if (mad_turn(MAD_SUICIDAL))
				sneak_attack |= SNEAK_SUICIDAL;
			if (youmonst.mopen)
				sneak_attack |= SNEAK_OPEN;
		}
		else {
			if (mdef->mflee && pd->mtyp != PM_BANDERSNATCH)
				sneak_attack |= SNEAK_BEHIND;
			if (is_blind(mdef))
				sneak_attack |= SNEAK_BLINDED;
			if (mdef->mtrapped)
				sneak_attack |= SNEAK_TRAPPED;
			if (!mdef->mcanmove || !mdef->mnotlaugh ||
				mdef->mstun || mdef->mconf || mdef->msleeping || mdef->mequipping)
				sneak_attack |= SNEAK_HELPLESS;
			if (mdef->msuicide)
				sneak_attack |= SNEAK_SUICIDAL;
			if (mdef->mopen)
				sneak_attack |= SNEAK_OPEN;
		}
		if(!recursed && weapon &&
			 magr && CHECK_ETRAIT(weapon, magr, ETRAIT_CREATE_OPENING) &&
			 ROLL_ETRAIT(weapon, magr, TRUE, !rn2(10))
		){
			mdef->mopen = 2; //includes youmonst.mopen
		}
	}
	/* if we have both a method (attack) and ability (dice) and this isn't a multihit, do the bonus */
	if ((sneak_attack&~SNEAK_JUYO) && sneak_dice && !recursed)
	{
		int snekdie = mlev(magr);
		if(snekdie > 30) snekdie = 30;
		
		/* some things increase sneak attack die size */
		if ((weapon && weapon->oartifact == ART_SILVER_STARLIGHT) ||
			(pa->mtyp == PM_CUPRILACH_RILMANI))
			snekdie = snekdie * 3 / 2;

		/* some things increase the number of sneak dice even further */
		if (weapon && weapon->owornmask && weapon->oartifact == ART_SPINESEEKER && (sneak_attack&SNEAK_BEHIND))
			sneak_dice++;
		if (weapon && weapon->owornmask && weapon->oartifact == ART_LOLTH_S_FANG && (sneak_attack&SNEAK_TRAPPED) && t_at(x(mdef), y(mdef)) && t_at(x(mdef), y(mdef))->ttyp == WEB)
			sneak_dice++;
		if (weapon && weapon->oartifact == ART_PEN_OF_THE_VOID && weapon->ovara_seals&SEAL_ANDROMALIUS && (mvitals[PM_ACERERAK].died > 0))
			sneak_dice++;

		/* some of the player's glyphs proc on sneak attacks */
		if (youagr) {
			if (active_glyph(BLOOD_RAPTURE))
				heal(&youmonst, 30);
			if (active_glyph(DEFILEMENT))
				heal(&youmonst, 10);
			if (active_glyph(WRITHE)){
				u.uen += 30;
				if (u.uen > u.uenmax)
					u.uen = u.uenmax;
			}
		}

		/* calculate snekdmg */
		/* Undead Hunters are not as good as Rogues, but any additional source of sneak attack damage upgrades their natural die to a regular-strength one */
		if(youagr && Role_if(PM_UNDEAD_HUNTER) && !Upolyd && sneak_dice == 1){
			snekdmg = rnd(2*snekdie/3);
		}
		else snekdmg = d(sneak_dice, snekdie);
		lifehunt_sneak_attacking = (weapon && weapon->oartifact == ART_LIFEHUNT_SCYTHE);
	}
	else {
		/* no sneak attack this time. We'll keep the info about what kind of sneak attack we might have made, though */
		snekdmg = 0;
		lifehunt_sneak_attacking = FALSE;
	}

	/* jousting */
#ifdef STEED
	if (youagr && melee && !recursed) {	/* do not joust in multihits */
		if ((u.usteed || centauroid(youracedata) || animaloid(youracedata)) && weapon &&
			(weapon_type(weapon) == P_LANCE ||
			(weapon->oartifact == ART_ROD_OF_SEVEN_PARTS) ||
			(weapon->oartifact == ART_PEN_OF_THE_VOID && weapon->ovara_seals&SEAL_BERITH) ||
			(weapon->oartifact == ART_SKY_REFLECTED && objects[(artinstance[ART_SKY_REFLECTED].ZerthOtyp)].oc_skill == P_LANCE)
			) &&
			mdef != u.ustuck &&
			!Fumbling &&
			!Stunned &&
			(weapon == uwep || (weapon == uswapwep && u.twoweap)))
		{
			/* if using two weapons, use worse of lance and two-weapon skills */
			jousting = 0;
			int joust_dieroll;
			int skill_rating = P_SKILL(weapon_type(weapon));	/* lance skill */
			if (u.twoweap && P_SKILL(P_TWO_WEAPON_COMBAT) < skill_rating)
				skill_rating = P_SKILL(P_TWO_WEAPON_COMBAT);
			if (skill_rating == P_ISRESTRICTED)
				skill_rating = P_UNSKILLED; /* 0=>1 */

			/* odds to joust are expert:80%, skilled:60%, basic:40%, unskilled:20% */
			if ((joust_dieroll = rn2(5)) < skill_rating) {
				if (!unsolid(pd) && !obj_resists(weapon, 0, 100)){
					if (weapon->otyp == DROVEN_LANCE && rnl(40) == (40 - 1))
						jousting = -1;	/* hit that breaks lance */
					else if (joust_dieroll == 0){ /* Droven lances are especially brittle */
						if (weapon->otyp == ELVEN_LANCE && rnl(75) == (75 - 1))
							jousting = -1;	/* hit that breaks lance */
						else if (rnl(50) == (50 - 1))
							jousting = -1;	/* hit that breaks lance */
					}
				}
				if (jousting != -1)
					jousting = 1;	/* successful joust */
			}
			else {
				jousting = 0;	/* did not joust */
			}
			/* exercise skill even for minimal damage hits */
			if (jousting) valid_weapon_attack = TRUE;
		}
	}
	if (jousting) {
		/* calculate bonus jousting damage */
		jostdmg += d(2, (weapon == uwep) ? 10 : 2);
	}
#endif
	// Knockback expert weapon trait
	if(!recursed && weapon && weapon->o_e_trait == ETRAIT_KNOCK_BACK &&
		 magr && CHECK_ETRAIT(weapon, magr, ETRAIT_KNOCK_BACK) &&
		 ROLL_ETRAIT(weapon, magr, !rn2(5), !rn2(20))
	){
		staggering_strike = TRUE;
	}
	// Stun expert weapon trait
	if(!recursed && weapon && weapon->o_e_trait == ETRAIT_STUNNING_STRIKE &&
		 magr && CHECK_ETRAIT(weapon, magr, ETRAIT_STUNNING_STRIKE) &&
		 ROLL_ETRAIT(weapon, magr, !rn2(2), !rn2(10))
	){
		stunning_strike = TRUE;
	}

	/* staggering strike */
	if (youagr && (melee || thrust) && !recursed) {
		if (
			// unarmed_punch (not two-weaponing)
			(unarmed_punch && !Upolyd && !bigmonst(pd) && !thick_skinned(pd) && !(u.twoweap) && 
			(dieroll < P_SKILL(P_BARE_HANDED_COMBAT)) &&
			((uarmg && uarmg->oartifact == ART_PREMIUM_HEART) || !rn2(5))) ||	// (odds reduced by 80% when not wearing Premium Heart)
			// Green Dragon Crescent Blade
			(weapon && weapon == uwep && uwep->oartifact == ART_GREEN_DRAGON_CRESCENT_BLAD &&
			(dieroll < P_SKILL(weapon_type(uwep)))) ||
			// Djem So
			(ulightsaberhit && activeFightingForm(FFORM_DJEM_SO) &&
			(dieroll < min(P_SKILL(P_DJEM_SO), P_SKILL(weapon_type(uwep)))) &&
			(mdef->mattackedu || !rn2(5))) ||	// (odds reduced by 80% when not counterattacking)
			// Juyo 
			(ulightsaberhit && activeFightingForm(FFORM_JUYO) &&
			(dieroll < min(P_SKILL(P_JUYO), P_SKILL(weapon_type(uwep)))) &&
			((sneak_attack&~SNEAK_JUYO) || (rn2(5) < 2)))	// (odds reduced by 60% when not sneak attacking)
			)
		{
			staggering_strike = TRUE;
		}
	}
	/* shattering strike */
	/* note: does not consider defender's weapon here */
	if (youagr && (melee || thrust) && !recursed) {
		if (
			// General Shattering Strike
			(
			(weapon && weapon == uwep) &&	// needs to be done with a mainhand weapon
			(dieroll <= (Role_if(PM_BARBARIAN) ? 4 : 2)) &&	// good roll
			(
			(weapon->oclass == WEAPON_CLASS && bimanual(weapon, youracedata)) ||	// twohanded weapon OR
			(Role_if(PM_SAMURAI) && (weapon->otyp == KATANA || weapon->otyp == CHIKAGE) && !uarms) ||			// samurai w/ a katana and no shield OR
			(weapon->oartifact == ART_PEN_OF_THE_VOID && weapon->ovara_seals&SEAL_BERITH)	// berith bound into the Pen
			) &&
			(weapon_type(weapon) != P_NONE) && (P_SKILL(weapon_type(weapon)) >= P_SKILLED) &&	// must be Skilled+
			(!u.twoweap)	// cannot be twoweaponing
			) ||
			// Lightsabers
			ulightsaberhit ||
			// Artifacts
			(weapon && arti_shattering(weapon)) ||
			// Green Dragon Crescent Blade
			(weapon && weapon == uwep && weapon->oartifact == ART_GREEN_DRAGON_CRESCENT_BLAD && (dieroll <= 2 + P_SKILL(weapon_type(weapon))))
		){
			shattering_strike = TRUE;
		}
	}
	/* disarming strike */
	/* note: does not consider defender's weapon here */
	if (youagr && valid_weapon_attack && (melee || thrust) && !recursed) {
		if ((((weapon && weapon == uwep) &&	// needs to be done with a mainhand weapon
			(dieroll <= (1 + P_SKILL(weapon_type(weapon))))) &&	// good roll (B:10%  S:15%  E:20%)
			(weapon->otyp == RANSEUR) &&	// can only be done with a ranseur
			(weapon_type(weapon) != P_NONE) && (P_SKILL(weapon_type(weapon)) >= P_BASIC) && // at least Basic skill
			(!u.twoweap)	// not twoweaponing
			)
			||
			arti_disarm(weapon)
			)
		{
			/* this conflicts with shattering strike -- either we are stealing the weapon or breaking it, not both! */
			/* if both apply, 50/50 odds which will be attempted */
			if (!shattering_strike || rn2(2)) {
				shattering_strike = FALSE;
				disarming_strike = TRUE;
			}
		}
	}
	/* Will eventually do a mercy blade attack after all messages are printed */
	if(valid_weapon_attack && (melee || thrust) && !recursed && mercy_blade_prop(weapon))
		mercy_blade = TRUE;
	if(valid_weapon_attack && (melee || thrust) && !recursed && weapon->oartifact == ART_DIRGE && youagr && check_mutation(MIND_STEALER))
		mind_blade = TRUE;
	/* X-hating */
	/* note: setting holyobj/etc affects messages later, but damage happens regardless of whether holyobj/etc is set correctly here */
	if (weapon)
		slot = W_WEP;	// note: the pointer <weapon>, which is not necessarily magr's wielded weapon
	else
		slot = attk_equip_slot(magr, attk->aatyp);

	switch (slot)
	{
	case W_ARMG:
		otmp = (youagr ? uarmg : which_armor(magr, slot));
		unarmed_punch = TRUE;
		break;
	case W_ARMF:
		otmp = (youagr ? uarmf : which_armor(magr, slot));
		unarmed_kick = TRUE;
		break;
	case W_ARMH:
		otmp = (youagr ? uarmh : which_armor(magr, slot));
		unarmed_butt = TRUE;
		break;
	case W_WEP:
		otmp = weapon;
		break;
	default:
		otmp = (struct obj *)0;
		break;
	}
	/* fake weapons */
	if (attk && (
		attk->adtyp == AD_GLSS
		|| attk->adtyp == AD_STAR
		|| attk->adtyp == AD_MOON
	)) {
		if (hates_silver(pd) && !(youdef && u.sealsActive&SEAL_EDEN)) {
			silverobj |= W_SKIN;
			seardmg += rnd(20);
		}
	}
	else if(attk && (
		attk->adtyp == AD_BSTR
	)){
		if (hates_unholy_mon(mdef) && !(youdef && u.sealsActive&SEAL_EDEN)) {
			unholyobj |= W_SKIN;
			seardmg += d(5,5);
		}
	}
	/* weapons/armor */
	else if (otmp &&
		// if using a weapon, only check that weapon (probably moot)
		(otmp == weapon ||
		// if armor harmlessly passes through mdef, skin/rings have an effect
		!insubstantial(pd) || hits_insubstantial(magr, mdef, attk, otmp))
		) {
		if (otmp->oartifact == ART_GLAMDRING &&
			(is_orc(pd) || is_demon(pd)))
			silverobj |= slot;
		if (hates_silver(pd) && !(youdef && u.sealsActive&SEAL_EDEN)) {
			if (obj_silver_searing(otmp) || check_oprop(otmp, OPROP_SFLMW))
				silverobj |= slot;
			if (obj_jade_searing(otmp))
				jadeobj |= slot;
		}
		artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_IRON;
		if (hates_iron(pd) &&
			obj_is_material(otmp, IRON) &&
			!(youdef && u.sealsActive&SEAL_EDEN) &&
			!(is_lightsaber(otmp) && litsaber(otmp))) {
			ironobj |= slot;
		}
		if ((hates_iron(pd) || hates_unholy_mon(mdef)) &&
			obj_is_material(otmp, GREEN_STEEL) &&
			!(is_lightsaber(otmp) && litsaber(otmp))) {
			grnstlobj |= slot;
		}
		
		if (hates_holy_mon(mdef) &&
			(otmp->known && (check_oprop(otmp, OPROP_HOLYW) || check_oprop(otmp, OPROP_LESSER_HOLYW) || check_oprop(otmp, OPROP_HOLY))) && /* message requires a particularly holy object */
			otmp->blessed) {
			holyobj |= slot;
		}
		if (hates_unholy_mon(mdef) &&
			is_unholy(otmp)) {
			unholyobj |= slot;
		}
		if (hates_unblessed_mon(mdef) &&
			!(is_unholy(otmp) || otmp->blessed)) {
			unblessedobj |= slot;
		}
		/* unusual case: wooden objects carved with the Veioistafur stave deal bonus damage to sea creatures */
		if (otmp->obj_material == WOOD && otmp->otyp != MOON_AXE &&
			(otmp->oward & WARD_VEIOISTAFUR) && mdef->data->mlet == S_EEL) {
			otherobj |= slot;
		}
		/* calculate sear damage */
		seardmg += hatesobjdmg(mdef, otmp, magr);
	}
	/* direct contact (includes rings) */
	else {
		/* skin/touch/etc */
		if (hates_silver(pd) && !(youdef && u.sealsActive&SEAL_EDEN)) {
			if (is_silver_mon(magr)) {
				silverobj |= W_SKIN;
				seardmg += rnd(20);
			}
			// no jade monsters (yet)
			else if (youagr && u.sealsActive&SEAL_EDEN) {
				/* Eden's silver hull, for the player */
				silverobj |= W_SKIN;
				seardmg += rnd(20);
			}
		}
		if (hates_iron(pd)){
			if(is_iron_mon(magr)) {
				ironobj |= W_SKIN;
				seardmg += rnd(mlev(mdef));
			}
			else if (youagr && unarmed_punch && u.sealsActive&SEAL_SIMURGH) {
				/* Simurgh's iron claws, for the player attacking with bared hands */
				ironobj |= W_SKIN;
				seardmg += rnd(mlev(mdef));
				mdef->mironmarked = TRUE;
			}
		}

		if (hates_holy_mon(mdef)){
			if(magr->mtyp == PM_UVUUDAUM){
				uuvuglory |= W_SKIN;
				seardmg += d(4, 9);
			}
			else if(is_holy_mon(magr)) {
				holyobj |= W_SKIN;
				seardmg += d(3, 7);
			} 
		}
		if (hates_unholy_mon(mdef)){
			if(magr->mtyp == PM_UVUUDAUM){
				uuvuglory |= W_SKIN;
				seardmg += d(3, 7);
			} 
			else if(is_unholy_mon(magr)) {
				unholyobj |= W_SKIN;
				seardmg += d(4, 9);
			}
			else if(magr->mtyp == PM_GREEN_STEEL_GOLEM) {
				grnstlobj |= W_SKIN;
				seardmg += d(2, 9);
			}
		}
		if (hates_unblessed_mon(mdef)){
			if(magr->mtyp == PM_UVUUDAUM){
				uuvuglory |= W_SKIN;
				seardmg += d(8, 3);
			}
			else if(is_unblessed_mon(magr)) {
				unblessedobj |= W_SKIN;
				seardmg += d(3, 8);
			}
		}

		/* rings, for an unarmed punch/claw attack */
		if (unarmed_punch) {
			/* only the player wears rings */
			if (youagr) {
				/* get correct ring */
				otmp = (rslot == W_RINGL) ? uleft
					: (rslot == W_RINGR) ? uright
					: (struct obj *)0;

				/* find what applies */
				if (otmp) {
					if (hates_silver(pd) && !(youdef && u.sealsActive&SEAL_EDEN)) {
						if (obj_silver_searing(otmp) || check_oprop(otmp, OPROP_SFLMW))
							silverobj |= rslot;
						if (obj_jade_searing(otmp))
							jadeobj |= rslot;
					}
					if (hates_iron(pd) && obj_is_material(otmp, IRON)) {
						ironobj |= rslot;
					}
					if ((hates_iron(pd) || hates_unholy_mon(mdef)) && obj_is_material(otmp, GREEN_STEEL)) {
						grnstlobj |= rslot;
					}
					if (hates_holy_mon(mdef) &&
						(otmp->known && (check_oprop(otmp, OPROP_HOLYW) || check_oprop(otmp, OPROP_LESSER_HOLYW) || check_oprop(otmp, OPROP_HOLY))) && /* message requires a particularly holy object */
						otmp->blessed) {
						holyobj |= rslot;
					}
					if (hates_unholy_mon(mdef) &&
						is_unholy(otmp)) {
						unholyobj |= rslot;
					}
					if (hates_unblessed_mon(mdef) &&
						!(is_unholy(otmp) || otmp->blessed)) {
						unblessedobj |= rslot;
					}
					/* calculate sear damage */
					seardmg += hatesobjdmg(mdef, otmp, magr);
				}
			}
		}
	}
	/* Find poisoned object, if any */
	poisonedobj = (struct obj *)0;
	if (valid_weapon_attack) {
		poisonedobj = weapon;
	}
	else if (unarmed_punch) {
		/* only the player wears rings */
		if (youagr && (!uarmg || !hits_insubstantial(magr, mdef, attk, uarmg))) {
			poisonedobj = (rslot == W_RINGL) ? uleft
				: (rslot == W_RINGR) ? uright
				: (struct obj *)0;
		}
		/* Spidersilk adds sleep poison to unarmed punches -- don't set poisonedobj, this is additional */
		otmp = (youagr ? uarm : which_armor(magr, W_ARM));
		if (otmp && otmp->oartifact == ART_SPIDERSILK && !rn2(5)) {
			poisons |= OPOISON_SLEEP;
		}
	}
	/* Apply object's poison */
	if (poisonedobj && (!insubstantial(pd) || hits_insubstantial(magr, mdef, attk, poisonedobj))) {
		poisons |= poisonedobj->opoisoned;
		if (arti_poisoned(poisonedobj))
			poisons |= OPOISON_BASIC;
		if (poisonedobj->oartifact == ART_WEBWEAVER_S_CROOK)
			poisons |= (OPOISON_SLEEP | OPOISON_BLIND | OPOISON_PARAL);
		if (is_wet_merc(poisonedobj) && !rn2(5))
			poisons |= (OPOISON_HALLU);
		if (poisonedobj->oartifact == ART_SUNBEAM)
			poisons |= OPOISON_FILTH;
		if (poisonedobj->oartifact == ART_MOONBEAM)
			poisons |= OPOISON_SLEEP;
		if (poisonedobj->oartifact == ART_DIRGE)
			poisons |= OPOISON_ACID;
		if (poisonedobj->otyp == FANG_OF_APEP)
			poisons |= OPOISON_DIRE;
		if (poisonedobj->otyp == TOOTH && poisonedobj->ovar1_tooth_type == SERPENT_TOOTH && Insight >= 20 && magr && poisonedobj->o_e_trait&ETRAIT_FOCUS_FIRE && CHECK_ETRAIT(poisonedobj, magr, ETRAIT_FOCUS_FIRE))
			poisons |= OPOISON_DIRE;
		if (Insight >= 40 && poisonedobj->oartifact == ART_LOLTH_S_FANG)
			poisons |= OPOISON_DIRE;
		if (poisonedobj->otyp == GREATCLUB){
			poisons |= OPOISON_BASIC;
			//All greatclubs upgrade to filth due to your influence on the world
			if(Insight >= 20 && u.uimpurity >= 20){
				poisons |= OPOISON_FILTH;
			}
		}
		if (poisonedobj->otyp == CHIKAGE && poisonedobj->obj_material == HEMARGYOS){
			poisons |= OPOISON_BASIC;
			if(youagr){
				if(Insight >= 20 && u.uimpurity >= 10){
					poisons |= OPOISON_FILTH;
				}
				if(Insight >= 50 && *hp(magr) <= (u.uimpurity*(*hpmax(magr)))/50){
					poisons |= OPOISON_DIRE;
				}
			}
		}
		/* Plague adds poisons to its launched ammo */
		if (launcher && launcher->oartifact == ART_PLAGUE) {
			if (monstermoves < artinstance[ART_PLAGUE].PlagueDuration)
				poisons |= OPOISON_FILTH;
			else
				poisons |= OPOISON_BASIC;
		}
		if (youagr && poisonedobj == uwep && activeFightingForm(FFORM_KNI_ELDRITCH)){
			if (u.ueldritch_style == SPE_POISON_SPRAY) poisons |= OPOISON_BASIC;
			if (u.ueldritch_style == SPE_ACID_SPLASH) poisons |= OPOISON_ACID;
		}
	}
	/* All AD_SHDW attacks are poisoned as well */
	if (attk && attk->adtyp == AD_SHDW) {
		poisons |= OPOISON_BASIC;
	}

	if (poisons)
	{
		/* Penalties for you using a poisoned weapon */
		if ((poisons & ~OPOISON_SILVER) && youagr && !recursed)
		{
			if Role_if(PM_SAMURAI) {
				if (!(uarmh && uarmh->oartifact && uarmh->oartifact == ART_HELM_OF_THE_NINJA)){
					adjalign(-sgn(u.ualign.type) * 5); //stiffer penalty

					if (sgn(u.ualign.type) > 0) {
						You("dishonorably use a poisoned weapon!");
						u.ualign.sins++;
						change_hod(1);
					}
				}
				else {
					static long dishonorable_ninja = -100L;

					if(dishonorable_ninja+100 < moves){
						You("dishonorably use a poisoned weapon!");
					}
					dishonorable_ninja = moves;
					adjalign(5);
				}
			}
			else if ((u.ualign.type == A_LAWFUL) && (Pantheon_if(PM_KNIGHT) || Pantheon_if(PM_VALKYRIE)) && (u.ualign.record > -10)) {
				You_feel("like an evil coward for using a poisoned weapon.");
				adjalign(-2); //stiffer penalty
				change_hod(1);
			}
		}

		/* which poisons need resist messages, and which will take effect? */
		int i, n;
		for (n = 0; n < NUM_POISONS; n++)
		{
			boolean resists = FALSE;
			boolean majoreff = FALSE;
			i = (1 << n);

			if (!(poisons & i))
				continue;
			/* determine which resistance is being checked and calculate the damage the poison will do */
			switch (i)
			{
			case OPOISON_BASIC:
				resists = Poison_res(mdef);
				majoreff = !rn2(10);
				break;
			case OPOISON_DIRE:
				resists = FALSE;
				majoreff = !rn2(10);
				break;
			case OPOISON_FILTH:
				resists = Sick_res(mdef);
				majoreff = !rn2(10);
				break;
			case OPOISON_SLEEP:
				resists = Sleep_res(mdef);
				majoreff = !rn2(2) || (poisonedobj && poisonedobj->oartifact == ART_MOONBEAM);
				break;
			case OPOISON_BLIND:
				resists = (Poison_res(mdef) || !haseyes(pd));
				majoreff = !rn2(2);
				break;
			case OPOISON_PARAL:
				resists = FALSE;
				majoreff = (!rn2(8) && !(youdef ? Free_action : mon_resistance(mdef, FREE_ACTION)));
				break;
			case OPOISON_AMNES:
				resists = mindless_mon(mdef);
				majoreff = !rn2(10);
				break;
			case OPOISON_ACID:
				resists = Acid_res(mdef);
				majoreff = TRUE;
				break;
			case OPOISON_SILVER:
				resists = !(hates_silver(pd) && !(youdef && u.sealsActive&SEAL_EDEN));
				if (!resists)
					silverobj |= slot;
				majoreff = TRUE;
				break;
			case OPOISON_HALLU:
				resists = youdef ? FALSE : (mindless_mon(mdef) || mon_resistance(mdef, HALLUC_RES));
				majoreff = !rn2(2);
				break;
			}
			if (majoreff && poisonedobj && (poisonedobj->opoisoned & i) && !rn2(20))
				poisons_wipedoff |= i;

			if (resists)
				poisons_resisted |= i;
			else
			{
				if (majoreff)
					poisons_majoreff |= i;
				else
					poisons_minoreff |= i;
			}
		}
		/* poison-injecting rings only ever do major effects */
		if (poisonedobj && poisonedobj->oclass == RING_CLASS) {
			poisons_resisted &= ~(poisons_majoreff);
			poisons_wipedoff = poisons_majoreff;
			poisons_minoreff = 0;
		}
		/* calculate poison damage */
		for (n = 0; n < NUM_POISONS; n++)
		{
			i = (1 << n);
			boolean major = (poisons_majoreff & i) != 0;
			boolean minor = (poisons_minoreff & i) != 0;
			if (!major && !minor)
				continue;
			/* calculate poison damage */
			switch (i)
			{
			case OPOISON_BASIC:
				poisdmg += (major) ? (youdef ? d(3, 6) : 80) : rnd(6);
				break;
			case OPOISON_FILTH:
				poisdmg += (major) ? (youdef ? d(3, 12) : 100) : rnd(12);
				break;
			case OPOISON_DIRE:
				if(Poison_res(mdef))
					poisdmg += (major) ? (youdef ? d(3, 6) : 80) : rn1(10, 6)/2;
				else
					poisdmg += (major) ? (youdef ? d(6, 6) : 160) : rn1(10, 6);
				break;
			case OPOISON_SLEEP:
				/* no damage */
				break;
			case OPOISON_BLIND:
				poisdmg += (major) ? 3 : rnd(3);
				break;
			case OPOISON_PARAL:
				poisdmg += (major) ? 6 : rnd(6);
				break;
			case OPOISON_AMNES:
				/* no damage */
				break;
			case OPOISON_ACID:
				poisdmg += rnd(10);
				break;
			case OPOISON_SILVER:
				poisdmg += rnd(20);
				break;
			}
		}
		/* if Plague is being used, note whether or not the current shot is filthed */
		if (launcher && launcher->oartifact == ART_PLAGUE && monstermoves < artinstance[ART_PLAGUE].PlagueDuration)
			artinstance[ART_PLAGUE].PlagueDoOnHit = !!(poisons_majoreff&OPOISON_FILTH);
	}

	/* Clockwork heat - player melee only */
	if (youagr && melee && !recursed) {
		if (uclockwork && !Fire_res(mdef) && u.utemp) {
			int heatdie = min(u.utemp, 20);
			int heatdice = (1 + (u.utemp >= MELTING) + (u.utemp >= MELTED));
			/* unarmed */
			if (unarmed_punch)
			{
				if (u.utemp >= BURNING_HOT && (!uarmg || is_metallic(uarmg))) {
					elemdmg += d(heatdice, heatdie);
				}
				else if (u.utemp >= HOT) {
					elemdmg += d(heatdice, heatdie / 2);
				}
			}
			/* using a metallic "weapon" -- a valid weapon attack isn't necessary */
			else if (weapon && is_metallic(weapon)) {
				elemdmg += d(heatdice, heatdie / 2);
			}
		}
	}

	/* Misotheistic weapon cancelling the divine (and also supressing magic) */
	if(weapon && weapon->otyp == ROD_OF_FORCE && !recursed && !litsaber(weapon)){
		int misdamage;
		if(youdef){
			if(base_casting_stat() == A_WIS || is_angel(pd)){
				if(!rn2(10)){
					misotheistic_major = TRUE;
					u.uen = max(u.uen - 400, min(u.uen, 0));
					make_doubtful(itimeout_incr(HDoubt, 13*50), FALSE);
					make_stunned(itimeout_incr(HStun, 13*50), FALSE);
					weapon->age = LIGHTSABER_MAX_CHARGE;
				}
				else {
					misdamage = rnd(13);
					u.uen = max(u.uen - misdamage*10, 0);
					weapon->age = min(weapon->age+misdamage*100, LIGHTSABER_MAX_CHARGE);
				}
			}
			else {
				misdamage = rnd(4);
				u.uen = max(u.uen - misdamage*10, 0);
				weapon->age = min(weapon->age+misdamage*100, LIGHTSABER_MAX_CHARGE);
			}
		}
		else {
			if(attacktype_fordmg(pd, AT_MAGC, AD_CLRC) || attacktype_fordmg(pd, AT_MMGC, AD_CLRC) || is_angel(pd)){
				if(!rn2(10)){
					misotheistic_major = TRUE;
					mdef->mdoubt = TRUE;
					mdef->mconf = TRUE;
					cancel_monst(mdef, weapon, youagr, FALSE, FALSE,0);
					weapon->age = LIGHTSABER_MAX_CHARGE;
				}
				else {
					misdamage = rnd(13);
					mdef->mspec_used = max(misdamage, mdef->mspec_used);
					weapon->age = min(weapon->age+misdamage*100, LIGHTSABER_MAX_CHARGE);
				}
			}
			else if(mon_attacktype(mdef, AT_MAGC) || mon_attacktype(mdef, AT_MMGC)){
				misdamage = rnd(4);
				mdef->mspec_used = max(misdamage, mdef->mspec_used);
				weapon->age = min(weapon->age+misdamage*100, LIGHTSABER_MAX_CHARGE);
			}
		}
	}
	/* set zombify resulting from melee mvm combat */
	if (magr && !youagr && !youdef && melee && !recursed) {
		if ((has_template(magr, ZOMBIFIED) || (has_template(magr, SKELIFIED) && !rn2(20))) && can_undead(mdef->data)){
			mdef->zombify = 1;
		}

		if ((magr->mtyp == PM_UNDEAD_KNIGHT
			|| magr->mtyp == PM_WARRIOR_OF_SUNLIGHT
			|| magr->mtyp == PM_UNDEAD_MAIDEN
			|| magr->mtyp == PM_KNIGHT_OF_THE_PRINCESS_S_GUARD
			|| magr->mtyp == PM_BLUE_SENTINEL
			|| magr->mtyp == PM_DARKMOON_KNIGHT
			|| magr->mtyp == PM_UNDEAD_REBEL
			|| magr->mtyp == PM_PARDONER
			|| magr->mtyp == PM_OCCULTIST
			|| magr->mtyp == PM_DREAD_SERAPH
			) && can_undead(mdef->data)){
			mdef->zombify = 1;
		}

		if (has_template(magr, FRACTURED) && is_kamerel(mdef->data)){
			mdef->zombify = 1;
		}
	}

	/* CALCULATE DAMAGE */
	/* Base Damage */
	/* case 1: valid melee weapon */
	/* case 2: invalid melee weapon */
	/* case 3: unarmed punch */
	/* case 4: unarmed kick */
	/* case 5: unarmed headbutt */
	/* case 6: none of the above */
	if(flatbasedmg){
		basedmg = flatbasedmg;
	}
	else if (valid_weapon_attack) {
		/* note: dmgval() includes enchantment and erosion of weapon */
		if ((weapon->oartifact == ART_PEN_OF_THE_VOID && weapon->ovara_seals&SEAL_MARIONETTE) ||
			(youagr && thrust && u.sealsActive&SEAL_MARIONETTE))
			basedmg = dmgval(weapon, mdef, SPEC_MARIONETTE, magr);
		else
			basedmg = dmgval(weapon, mdef, 0, magr);

		/* Liecleaver adds 1d10 damage to fired bolts */
		if (fired && launcher && launcher->oartifact == ART_LIECLEAVER)
			basedmg += rnd(10);
	}
	else if (invalid_weapon_attack) {
		long cnt = weapon->quan;

		switch (weapon->oclass)
		{
		case POTION_CLASS:
			{
			/* potionhit() requires our potion to be free, so don't use destroy_one_magr_weapon */
			if (cnt > 1L)
				otmp = splitobj(weapon, 1L);
			else {
				otmp = weapon;
			}
			int waswhere = otmp->where;
			
			switch(otmp->where) {
				case OBJ_FREE:
					break;
				case OBJ_INVENT:
					if (otmp == uwep) uwepgone();
					freeinv(otmp);
					break;
				case OBJ_MINVENT:
					if (otmp == MON_WEP(magr)) MON_WEP(magr) = (struct obj *)0;
					m_freeinv(otmp);
					break;
				default:
					impossible("how to free potion?");
					obj_extract_self(otmp);
					break;
			}

			if (otmp == weapon) {
				/* remember stuff about the potion so weapon can be used for xname() and such
				 * after potionhit() is called */
				*weapon_p = NULL;
				weapon = &tempwep;
				*weapon = *otmp;
			}

			/* do the potion effects */
			/* note: if player is defending, this assumes the potion was thrown */
			potionhit(mdef, otmp, youagr);

			/* corner-case: AT_HODS */
			if (waswhere == OBJ_INVENT && !youagr) {
				Your("%s vanishes!", makesingular(xname(weapon)));
			}
			else if (waswhere == OBJ_MINVENT && magr != weapon->ocarry) {
				pline("%s %s vanishes!", s_suffix(Monnam(weapon->ocarry)), makesingular(xname(weapon)));
			}
			/* check if defender was killed */
			if (*hp(mdef) < 1) {
				*weapon_p = NULL;
				return (MM_HIT|MM_DEF_DIED);
			}
			/* potionhit prints messages */
			hittxt = TRUE;
			/* in case potion effect causes transformation */
			pd = (youdef ? youracedata : mdef->data);
			/* set basedmg */
			basedmg = 1;
			}
			break;

		default:
			switch (weapon->otyp)
			{
			case MIRROR:
				/* with Nudzirath bound, thrown mirrors have a different behaviour */
				if (youagr && thrown && !weapon->oartifact && u.specialSealsActive&SEAL_NUDZIRATH) {

					/* remember stuff about the mirror so weapon can be used after nudzirath_shatter() is called */
					otmp = weapon;
					weapon = &tempwep;
					*weapon = *otmp;
					
					nudzirath_shatter(otmp, bhitpos.x, bhitpos.y);
					*weapon_p = NULL;

					/* check if defender was killed */
					if (*hp(mdef) < 1)
						return (MM_HIT|MM_DEF_DIED);
					/* nudzirath_shatter prints messages */
					hittxt = TRUE;
					real_attack = FALSE;
					basedmg = 1;
				}
				/* mirrors shatter */
				else if (breaktest(weapon)) {
					/* if break the mirror, it affects your luck. And prints a message. */
					if (youagr) {
						if (u.specialSealsActive&SEAL_NUDZIRATH){
							You("break %s mirror.  You feel a deep satisfaction!",
								shk_your(buf, weapon));
							change_luck(+2);
						}
						else {
							You("break %s mirror.  That's bad luck!",
								shk_your(buf, weapon));
							change_luck(-2);
						}
						hittxt = TRUE;
					}
					else if (magr && (vis&VIS_MAGR)) {
						pline("%s breaks %s mirror!", Monnam(magr),
							shk_mons(buf, weapon, magr));
						hittxt = TRUE;
					}
					destroy_one_magr_weapon = TRUE;
					real_attack = FALSE;
					/* set basedmg */
					basedmg = 1;
				}
				break;

			case EXPENSIVE_CAMERA:
				/* expensive cameras are destroyed */
				if (weapon->otyp == EXPENSIVE_CAMERA) {
					if (youagr) {
						You("succeed in destroying %s camera.  Congratulations!",
							shk_your(buf, weapon));
					}
					else if (magr && (vis&VIS_MAGR)) {
						pline("%s smashes %s camera!", Monnam(magr), shk_mons(buf, weapon, magr));
					}
					destroy_one_magr_weapon = TRUE;
					real_attack = FALSE;
					basedmg = 1;
				}
				break;

			case BOOMERANG:
				/* boomerangs can break when used as melee weapons by the player */
				if (youagr && (melee || thrust) &&
					rnl(4) == 4 - 1 && !weapon->oartifact) {
					if (dohitmsg) {
						pline("As you hit %s, %s%s %s breaks into splinters.",
							mon_nam(mdef), (cnt > 1L) ? "one of " : "",
							shk_your(buf, weapon), xname(weapon));

						hittxt = TRUE;
					}
					destroy_one_magr_weapon = TRUE;
					/* slightly more damage */
					basedmg = rnd(2) + 1;
				}
				else
					basedmg = rnd(2);
				break;

			case CORPSE:
				/* some corpses petrify */
				if (touch_petrifies(&mons[weapon->corpsenm])) {
					static const char withwhat[] = "corpse";
					if (dohitmsg) {
						if (thrown && vis) {
							pline_The("%s corpse hits %s%s",
								mons[weapon->corpsenm].mname,
								(youdef ? "you" : mon_nam(mdef)),
								(youdef ? "!" : ".")
								);
							hittxt = TRUE;
						}
						else if (youagr) {
							You("hit %s with %s %s.",
								mon_nam(mdef),
								weapon->dknown ? the(mons[weapon->corpsenm].mname) : an(mons[weapon->corpsenm].mname),
								(weapon->quan > 1) ? makeplural(withwhat) : withwhat
								);
							hittxt = TRUE;
						}
						else if (vis && magr) {
							pline("%s hits %s with %s %s %s%s",
								Monnam(magr),
								(youdef ? "you" : mon_nam(mdef)),
								hisherits(magr),
								mons[weapon->corpsenm].mname,
								((weapon->quan > 1) ? makeplural(withwhat) : withwhat),
								(youdef ? "!" : ".")
								);
							hittxt = TRUE;
						}
						else if (vis) {
							pline("%s %s hit with %s %s%s",
								(youdef ? "You" : Monnam(mdef)),
								(youdef ? "are" : "is"),
								an(mons[weapon->corpsenm].mname),
								((weapon->quan > 1) ? makeplural(withwhat) : withwhat),
								(youdef ? "!" : ".")
								);
							hittxt = TRUE;
						}
					}
					/* do stone */
					result = xstoney(magr, mdef);
					if (result&(MM_DEF_DIED | MM_DEF_LSVD | MM_AGR_DIED)) {
						return result;
					}
				}
				/* all corpses deal damage based on the size of the corpse */
				basedmg = (weapon->corpsenm >= LOW_PM ? mons[weapon->corpsenm].msize : 0) + 1;
				break;

			case EGG:
				basedmg = 1;/* nominal physical damage */
				real_attack = FALSE;

				/* luck penalty for killing your own eggs */
				if (youagr && weapon->spe && weapon->corpsenm >= LOW_PM) {
					if (cnt < 5)
						change_luck((schar)-(cnt));
					else
						change_luck(-5);
				}

				/* petrifying eggs */
				if (weapon->corpsenm != NON_PM && touch_petrifies(&mons[weapon->corpsenm])) {
					/*learn_egg_type(obj->corpsenm);*/
					if (vis) {
						if (magr) {
							pline("%s hit%s %s with %s %s egg%s!",
								(youagr ? "You" : Monnam(magr)),
								(youagr ? "" : "s"),
								(youdef ? "you" : mon_nam(mdef)),
								(weapon->known ? "the" : (cnt > 1L) ? "some" : "a"),
								(weapon->known ? mons[weapon->corpsenm].mname : "petrifying"),
								plur(cnt)
								);
						}
						else {
							pline("%s %s hit with %s %s egg%s!",
								(youdef ? "You" : Monnam(mdef)),
								(youdef ? "are" : "is"),
								(weapon->known ? "the" : (cnt > 1L) ? "some" : "a"),
								(weapon->known ? mons[weapon->corpsenm].mname : "petrifying"),
								plur(cnt)
								);
						}
						hittxt = TRUE;
					}
					if (vis)
						pline("Splat!");
					weapon->known = 1;	/* (not much point...) */
					destroy_all_magr_weapon = TRUE;

					result = xstoney(magr, mdef);
					/* return if we had a significant result from xstoney */
					if (result&(MM_DEF_DIED | MM_DEF_LSVD | MM_AGR_DIED))
					{	
						/* clean up the eggs right now*/
						if (weapon->where == OBJ_FREE)
							obfree(weapon, (struct obj *)0);
						else if (youagr)
							useupall(weapon);
						else if (magr) for (; cnt; cnt--)
							m_useup(magr, weapon);
						*weapon_p = NULL;
						return result;
					}
				}
				else {	/* ordinary egg(s) */
					const char *eggp =
						(weapon->corpsenm != NON_PM && weapon->known) ?
						the(mons[weapon->corpsenm].mname) :
						(cnt > 1L) ? "some" : "an";

					if (vis) {
						if (magr) {
							pline("%s hit%s %s with %s egg%s.",
								(youagr ? "You" : Monnam(magr)),
								(youagr ? "" : "s"),
								(youdef ? "you" : mon_nam(mdef)),
								eggp,
								plur(cnt)
								);
						}
						else {
							pline("%s %s hit with %s egg%s.",
								(youdef ? "You" : Monnam(mdef)),
								(youdef ? "are" : "is"),
								(weapon->known ? "the" : (cnt > 1L) ? "some" : "an"),
								plur(cnt)
								);
						}
						hittxt = TRUE;
					}

					if (touch_petrifies(pd) && !stale_egg(weapon)) {
						if (vis) {
							pline_The("egg%s %s alive any more...",
								plur(cnt),
								(cnt == 1L) ? "isn't" : "aren't");
						}
						if (weapon->timed) stop_all_timers(weapon->timed);
						weapon = poly_obj(weapon, ROCK);
						*weapon_p = weapon;
						weapon->oartifact = 0;
						weapon->spe = 0;
						weapon->known = weapon->dknown = weapon->bknown = 0;
						weapon->owt = weight(weapon);
						if (thrown) {
							place_object(weapon, x(mdef), y(mdef));
							*weapon_p = NULL;
						}
					}
					else {
						if (vis) {
							pline("Splat!");
						}
						destroy_all_magr_weapon = TRUE;
						if (youagr) {
							exercise(A_WIS, FALSE);
						}
					}
				}
				break;

			case CLOVE_OF_GARLIC:
				if (!youdef && is_undead(pd)) {/* no effect against demons */
					monflee(mdef, d(2, 4), FALSE, TRUE);
				}
				basedmg = 1;
				break;

			case CREAM_PIE:
			case BLINDING_VENOM:
				if (youdef) {
					/*I don't think anything goes here?*/;
				}
				else {
					mdef->msleeping = 0;
				}

				basedmg = 0;
				real_attack = FALSE;

				if (can_blnd(magr, mdef, (uchar)
					(weapon->otyp == BLINDING_VENOM
					? AT_SPIT : AT_WEAP), weapon)) {

					if (youdef) {
						int blindinc = rnd(25);
						hittxt = TRUE;
						if (weapon->otyp == CREAM_PIE) {
							if (!Blind) pline("Yecch!  You've been creamed.");
							else pline("There's %s sticky all over your %s.",
								something,
								body_part(FACE));
						}
						else if (weapon->otyp == BLINDING_VENOM) {
							int num_eyes = eyecount(youracedata);
							/* venom in the eyes */
							if (!Blind) pline_The("venom blinds you.");
							else Your("%s sting%s.",
								(num_eyes == 1) ? body_part(EYE) :
								makeplural(body_part(EYE)),
								(num_eyes == 1) ? "s" : "");
						}
						u.ucreamed += blindinc;
						make_blinded(Blinded + (long)blindinc, FALSE);
						if (!Blind)
							Your1(vision_clears);
					}
					else {
						if (youagr && Blind) {
							pline("%s!",
								weapon->otyp == CREAM_PIE ? "Splat" : "Splash");
						}
						else if (canseemon(mdef)) {
							hittxt = TRUE;
							if (weapon->otyp == BLINDING_VENOM) {
								pline_The("venom blinds %s%s!", mon_nam(mdef),
									mdef->mcansee ? "" : " further");
							}
							else if (weapon->otyp == CREAM_PIE) {
								char *whom = mon_nam(mdef);
								char *what = The(xname(weapon));
								if (melee && weapon->quan > 1)
									what = An(singular(weapon, xname));
								/* note: s_suffix returns a modifiable buffer */
								if (haseyes(pd) && pd->mtyp != PM_FLOATING_EYE)
									whom = strcat(strcat(s_suffix(whom), " "),
									mbodypart(mdef, FACE));
								pline("%s %s over %s!",
									what, vtense(what, "splash"), whom);
							}
						}

						if (youagr) {
							setmangry(mdef);
						}
						mdef->mcansee = 0;
						int tmp = rn1(25, 21);
						if (((int)mdef->mblinded + tmp) > 127)
							mdef->mblinded = 127;
						else
							mdef->mblinded += tmp;
					}
				}
				break;

			case ACID_VENOM:
				if (Acid_res(mdef)) {
					if (youdef || canseemon(mdef)) {
						hittxt = TRUE;
						if (youagr)
							Your("venom hits %s harmlessly.",
							mon_nam(mdef));
						else {
							pline("%s %s unaffected.",
								(youdef ? "You" : Monnam(mdef)),
								(youdef ? "are" : "is")
								);
						}
					}
					basedmg = 0;
				}
				else {
					if (youdef || canseemon(mdef)) {
						hittxt = TRUE;
						if (youagr)
							Your("venom burns %s!", mon_nam(mdef));
						else {
							pline("The venom burns%s%s!",
								(youdef ? "" : " "),
								(youdef ? "" : mon_nam(mdef))
								);
						}
					}
					basedmg = dmgval(weapon, mdef, 0, magr);
				}
				/* projectile should take care of it */
				//destroy_all_magr_weapon = TRUE;
				real_attack = FALSE;
				break;

			case STILETTOS:
				basedmg = rnd(bigmonst(pd) ? 2 : 6) + weapon->spe + dbon(weapon, magr);
				if (youagr && u.twoweap)
					basedmg += rnd(bigmonst(pd) ? 2 : 6) + weapon->spe;
				break;

			default:
				basedmg = weapon->owt / 100;
				if (basedmg < 1) basedmg = 1;
				else basedmg = rnd(basedmg);
				if (basedmg > 6) basedmg = 6;
				break;
			}
		}
	}
	else if (unarmed_punch) {
		struct obj * gloves = (youagr ? uarmg : which_armor(magr, W_ARMG));
		struct weapon_dice unarmed_dice;
		int unarmedMult;
		/* initialize struct */
		dmgval_core(&unarmed_dice, bigmonst(pd), (struct obj *)0, 0, magr);
		/* determine unarmedMult */
		if (youagr) {
			unarmedMult = Race_if(PM_HALF_DRAGON) ? 3 : u.sealsActive&SEAL_ECHIDNA ? 2 : 1;
			if(check_mutation(SHUB_CLAWS))
				unarmedMult++;
		}
		else {
			unarmedMult = 1;
		}
		if (gloves && gloves->oartifact == ART_GREAT_CLAWS_OF_URDLEN)
			unarmedMult += 2;

		/* base unarmed dice */
		if (youagr && martial_bonus())
			unarmed_dice.oc_damd = 4 * unarmedMult;
		else if (youagr && u.umaniac)
			unarmed_dice.oc_damd = 3 * unarmedMult;
		else
			unarmed_dice.oc_damd = 2 * unarmedMult;

		if(youagr && u.umaniac && weapon_dam_bonus((struct obj *) 0, P_BARE_HANDED_COMBAT) > 0)
			unarmed_dice.oc_damd += weapon_dam_bonus((struct obj *) 0, P_BARE_HANDED_COMBAT)*2;
		/* Eurynome causes exploding dice, sometimes larger dice */
		if (youagr && u.sealsActive&SEAL_EURYNOME) {
			unarmed_dice.exploding = TRUE;
			unarmed_dice.oc_damd = max(unarmed_dice.oc_damd,
				2 * rnd(5) + (martial_bonus() ? 2 * unarmedMult : 0));
		}
		/* Grandmaster's robe causes exploding dice, 50% chance of doubled dice */
		otmp = (youagr ? uarmc : which_armor(magr, W_ARMC));
		if (otmp && otmp->oartifact == ART_GRANDMASTER_S_ROBE) {
			unarmed_dice.exploding = TRUE;
			if (rn2(2)) {
				unarmed_dice.oc_damn *= 2;
			}
		}
		/* calculate dice and set basedmg */
		basedmg = weapon_dmg_roll(&unarmed_dice, FALSE);

		/* The Annulus is very stronk -- 2x base damage + 2x enchantment */
		/* yes, this can be redoubled by artifact gloves */
		/* it's so strong, its damage applies no matter which hand is punching */
		if (youagr) {	// only the player wears rings
			if (((otmp = uright) && otmp->oartifact == ART_ANNULUS) ||
				((otmp = uleft) && otmp->oartifact == ART_ANNULUS))
			{
				basedmg += weapon_dmg_roll(&unarmed_dice, FALSE);
				basedmg += otmp->spe * 2;
			}
		}

		/* fighting gloves give bonus damage */
		if (gloves && gloves->otyp == find_tgloves())
			basedmg += ((youagr && martial_bonus()) ? 3 : 1);

		
		if (gloves) {
			/* all gloves give their enchantment to melee damage */
			basedmg += gloves->spe;
		}
		/* no gloves? Look at rings -- which are player-only */
		else if (youagr &&
			(otmp = (rslot == W_RINGL) ? uleft
				: (rslot == W_RINGR) ? uright
				: (struct obj *)0))
		{

			/* edder-symbol signet rings increase melee damage */
			if (otmp
				&& otmp->ohaluengr
				&& (isEngrRing(otmp->otyp) || isSignetRing(otmp->otyp))
				&& otmp->oward == EDDER_SYMBOL)
			{
				basedmg += 5;
			}
			/* no other rings increase melee damage at this time */
			/* general handling for offensive artifact rings should be done in the artifact hit block */
			/* (the Annulus is supposed to increase melee damage even while worn under gloves, and act regardless of striking fist) */
		}
	}
	else if (unarmed_kick) {
		/* If a proper monster-style attack is being used, no stat-based damage! */
		if (monsdmg == 0) {
			/* Do monsters ever kick for 0 monsdmg? */
			if (youagr){
				real_attack = TRUE; /*Not sure if this should be true for the general case, but it should be for PCs*/
				if(u.umaniac){
					if(weapon_dam_bonus((struct obj *) 0, P_BARE_HANDED_COMBAT) > 0)
						basedmg = rnd((ACURRSTR + ACURR(A_DEX) + ACURR(A_CON) + ACURR(A_CHA)) / 15 + weapon_dam_bonus((struct obj *) 0, P_BARE_HANDED_COMBAT)*2);
					else 
						basedmg = rnd((ACURRSTR + ACURR(A_DEX) + ACURR(A_CON) + ACURR(A_CHA)) / 15);
				}
				else {
					if(weapon_dam_bonus((struct obj *) 0, P_BARE_HANDED_COMBAT) > 0)
						basedmg = rnd((ACURRSTR + ACURR(A_DEX) + ACURR(A_CON)) / 15) + weapon_dam_bonus((struct obj *) 0, P_BARE_HANDED_COMBAT)/2;
					else
						basedmg = rnd((ACURRSTR + ACURR(A_DEX) + ACURR(A_CON)) / 15);
				}
			}
			else
				basedmg = 1;
			/* martial players are much better at kicking */
			if (youagr){
				if(martial_bonus() || (uarmf && (uarmf->otyp == KICKING_BOOTS || (uarmf->otyp == IMPERIAL_ELVEN_BOOTS && check_imp_mod(uarmf, IEA_KICKING)))))
					basedmg += rn2(ACURR(A_DEX)/2 + 1);
				if(youracedata->mtyp == PM_SASQUATCH)
					basedmg += rn2(ACURR(A_CON)/2 + 1);
			}
		}

		/* boots can increase kicking damage */
		otmp = (youagr ? uarmf : which_armor(magr, W_ARMF));
		if (otmp) {
			basedmg += otmp->spe;
			if (otmp->otyp == KICKING_BOOTS){
				basedmg += rnd(6) + rnd(5) + (bigmonst(pd) ? 0 : 1);
			}
			if(otmp->otyp == IMPERIAL_ELVEN_BOOTS && check_imp_mod(otmp, IEA_KICKING)){
				basedmg += rnd(6) + (bigmonst(pd) ? 0 : 1);
				if(mdef && !resists_magm(mdef) && !resist(mdef, RING_CLASS, 0, NOTELL)){
					//Half spell etc.
					elemdmg += d(2,12) + otmp->spe;
				}
			}
			if (otmp->otyp == STILETTOS || otmp->otyp == HEELED_BOOTS){
				basedmg += rnd(bigmonst(pd) ? 2 : 6);
			}
			if (otmp->otyp == WIND_AND_FIRE_WHEELS){
				basedmg += rnd(bigmonst(pd) ? 9 : 14);
			}

			basedmg = max(basedmg, 1);
		}


		/* we are using monsdmg == -1 to signify a 'clumsy' kick that deals half damage */
		if (monsdmg == -1)
			basedmg /= 2;
	}
	else if (unarmed_butt) {
		/* helm enchantment increases headbutt damage */
		otmp = (youagr ? uarmh : which_armor(magr, W_ARMH));
		if (otmp) {
			basedmg += otmp->spe;
		}
	}
	else {
		basedmg = 0;
	}
	/* multiply by precision_mult, if it applies */
	basedmg *= (precision_mult ? precision_mult : 1);

	/* fakewep: Sword of Blood bonus damage */
	if (originalattk && spirit_rapier_at(originalattk->aatyp) && originalattk->adtyp == AD_BLUD)
	{
		if (has_blood(pd)) {
			specdmg += mlev(mdef);
			swordofblood = TRUE;	/* must come before "Set Phasing" */
			//pline("The blade of rotted blood tears through your veins!");
		}
		if (youagr){
			specdmg += u.uimpurity/2;
		}
	}
	/* fakewep: Sword of Mercury bonus effects */
	if (attk && attk->adtyp == AD_MERC)
	{
		if (!Cold_res(mdef)) {
			elemdmg += basedmg;
		}
		/* abuse player's stats */
		if (youdef) {
			exercise(A_INT, FALSE);
			exercise(A_WIS, FALSE);
			exercise(A_CHA, FALSE);
			if (!Poison_res(mdef)){
				exercise(A_INT, FALSE);
				exercise(A_WIS, FALSE);
				exercise(A_CHA, FALSE);
			}
		}
	}

	/* Set Phasing */
	phase_armor = (
		(weapon && arti_phasing(weapon)) ||
		(youagr && u.sealsActive&SEAL_CHUPOCLOPS) ||
		(youagr && weapon && weapon->otyp == LONG_SWORD && activeFightingForm(FFORM_HALF_SWORD)) ||
		(!youagr && magr && mad_monster_turn(magr, MAD_NON_EUCLID)) ||
		(originalattk && spirit_rapier_at(originalattk->aatyp) && originalattk->adtyp != AD_BLUD && originalattk->adtyp != AD_WET) ||
		(swordofblood) /* this touch adtyp is only conditionally phasing */
		);

	/* Get Other Bonus Damage */
	/* only applies once per attack */
	int skill_damage = 0;
	if (!recursed)
	{
		/* Study */
		if (youagr && mdef->ustdym){
			bonsdmg += rnd(mdef->ustdym);
		}

		if (youdef && u.ustdy) {
			bonsdmg += u.ustdy;
			u.ustdy /= 2;
		}
		if (!youdef && mdef->mstdy){/* note: negative mstdy is used for monster spell protection */
			bonsdmg += mdef->mstdy;
			if (mdef->mstdy > 0) mdef->mstdy /= 2;
			else mdef->mstdy += 1;
		}
		/* Bardic Encouragement */
		if (youagr)
			bonsdmg += u.uencouraged;
		else if (magr){
			if(!(youdef && Nightmare && u.umadness&MAD_RAGE))
				bonsdmg += magr->encouraged;
			if(magr->mtyp == PM_LUCKSUCKER)
				bonsdmg += magr->mvar_lucksucker;
			if(magr->mtame){
				if(uring_art(ART_NARYA))
					bonsdmg += narya();
				if(!youdef && (artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_POWER))
					bonsdmg += 8;
				if(!youdef && activeMentalEdge(GSTYLE_RESONANT))
					bonsdmg += u.usanity < 50 ? 0 : u.usanity < 75 ? 2 : u.usanity < 90 ? 5 : 8;
				if(!youdef && is_vampire(magr->data) && check_vampire(VAMPIRE_MASTERY))
					bonsdmg += 5;
			}
		}
		/* Singing Sword -- only works when the player is wielding it >_> */
		if (uwep && uwep->oartifact == ART_SINGING_SWORD) {
			if (uwep->osinging == OSING_LIFE) {
				if (youagr)
					bonsdmg += (uwep->spe + 1);
			}
			if (uwep->osinging == OSING_COURAGE){
				if (!youagr && magr && magr->mtame && !mindless_mon(magr) && !is_deaf(magr))
					bonsdmg += (uwep->spe + 1);
			}
			if (uwep->osinging == OSING_DIRGE) {
				if (!youagr && magr && !magr->mtame && !mindless_mon(magr) && !is_deaf(magr))
					bonsdmg -= (uwep->spe + 1);
			}
		}
		/* Priests of Asmodeus */
		if(magr && flags.spriest_level && is_demon(magr->data) && is_lawful_mon(magr) && !magr->mpeaceful)
			bonsdmg += 9;
		/* Dahlver Nar gives bonus damage to unarmed punches */
		if (youagr && unarmed_punch && u.specialSealsActive&SEAL_DAHLVER_NAR) {
			bonsdmg += d(2, 6) + min(u.ulevel / 2, (u.uhpmax - u.uhp) / 10);
		}
		/* Paimon increases damage dealt with spellbooks significantly */
		if (valid_weapon_attack && weapon && weapon->oclass == SPBOOK_CLASS && youagr && u.sealsActive&SEAL_PAIMON) {
			bonsdmg += objects[weapon->otyp].oc_level + spiritDsize();
		}
		/* general damage bonus */
		if(real_attack){
			if (magr && (valid_weapon_attack || fake_valid_weapon_attack || unarmed_punch || unarmed_kick || natural_strike)) {
				/* player-specific bonuses */
				if (youagr) {
					bonsdmg += u.udaminc;
					bonsdmg += aeshbon();
					
					if(uarmg && uarmg->otyp == IMPERIAL_ELVEN_GAUNTLETS && check_imp_mod(uarmg, IEA_INC_DAM))
						bonsdmg += uarmg->spe;

					/* due to lack of a nicer place, check for eldritch style bonuses here*/
#define eld_bon_dice ((FightingFormSkillLevel(FFORM_KNI_ELDRITCH) >= P_EXPERT) ? 6 : (FightingFormSkillLevel(FFORM_KNI_ELDRITCH) >= P_SKILLED ? 3 : 1))
#define eld_bon_size ((u.ueldritch_style == SPE_FINGER_OF_DEATH) ? 13 : (\
		 (u.ueldritch_style == SPE_FIRE_STORM || u.ueldritch_style == SPE_BLIZZARD || u.ueldritch_style == SPE_LIGHTNING_STORM) ? 8 : 6))
					if (weapon && weapon == uwep && activeFightingForm(FFORM_KNI_ELDRITCH)){
						boolean resist_check = FALSE;
						char *verb = "";
						int spell_id;
						if (u.ueldritch_style == SPE_FIREBALL || u.ueldritch_style == SPE_FIRE_STORM){
							verb = "burn";
							resist_check = Fire_res(mdef);
						} else if (u.ueldritch_style == SPE_CONE_OF_COLD || u.ueldritch_style == SPE_BLIZZARD){
							verb = "freeze";
							resist_check = Cold_res(mdef);
						} else if (u.ueldritch_style == SPE_LIGHTNING_BOLT || u.ueldritch_style == SPE_LIGHTNING_STORM){
							verb = "shock";
							resist_check = Shock_res(mdef);
						} else if (u.ueldritch_style == SPE_ACID_SPLASH){
							verb = "befoul";
							resist_check = Acid_res(mdef);
						} else if (u.ueldritch_style == SPE_POISON_SPRAY){
							verb = "poison";
							resist_check = Poison_res(mdef);
						} else if (u.ueldritch_style == SPE_FINGER_OF_DEATH){
							verb = "sap the life from";
							resist_check = Dark_res(mdef);
						}

						for (spell_id = 0; spell_id < MAXSPELL; spell_id++)
							if (spellid(spell_id) == u.ueldritch_style)
								break;

						if (spell_id < MAXSPELL && percent_success(spell_id) > rn2(100) && !resist_check){
							if (FightingFormSkillLevel(FFORM_KNI_ELDRITCH) >= P_BASIC){
								if (u.uen >= objects[u.ueldritch_style].oc_level){
									bonsdmg += d(eld_bon_dice, eld_bon_size);
									u.uen -= objects[u.ueldritch_style].oc_level;
									pline("Eldritch energies %s %s!", verb, mon_nam(mdef));
								}
							}
							use_skill(P_KNI_ELDRITCH, 1);
						}
					}

					/* when bound, Dantalion gives bonus "precision" damage based on INT; 1x for all melee and ranged */
					if ((u.sealsActive&SEAL_DANTALION) && !noanatomy(pd)) {
						if (ACURR(A_INT) == 25) bonsdmg += 8;
						else bonsdmg += max(0, (ACURR(A_INT) - 10) / 2);
					}
					if(attk && is_bite_at(attk->aatyp)
						&& mdef && (!vegetarian(mdef->data) || your_race(mdef->data))
						&& mad_turn(MAD_CANNIBALISM)
					){
						int can_bon = str_dbon(&youmonst);

						if (ACURR(A_CHA) == 25) can_bon += 8;
						else can_bon += max(0, (ACURR(A_CHA) - 10) / 2);

						if(your_race(mdef->data))
							can_bon *= 2;

						bonsdmg += can_bon;
					}
					if(attk && is_insight_tentacle_at(attk->aatyp)
						&& (u.sealsActive&SEAL_OSE)
					){
						if(Insight)
							bonsdmg += rnd(min(Insight, mlev(magr)));
						if (ACURR(A_CHA) == 25) bonsdmg += 8;
						else bonsdmg += max(0, (ACURR(A_CHA) - 10) / 2);
					}
				}
				//Monster specific bonuses
				else if(magr){
					struct obj *arm = which_armor(magr, W_ARMG);
					if(arm && arm->otyp == IMPERIAL_ELVEN_GAUNTLETS && check_imp_mod(arm, IEA_INC_DAM))
						bonsdmg += arm->spe;
				}

				/* If you throw using a propellor, you don't get a strength
				* bonus but you do get an increase-damage bonus.
				*/
				if (natural_strike || unarmed_punch || unarmed_kick || melee || thrust) {
					boolean usewep = (weapon && (melee || thrust) && !martial_aid(weapon));
					int tmp = dbon(usewep ? weapon : (struct obj *)0, magr);
					/* greatly reduced STR damage for offhand attacks */
					if (attk->aatyp == AT_XWEP || attk->aatyp == AT_MARI)
						tmp = min(0, tmp);
					bonsdmg += tmp;
				}
				else if (fired || thrown)
				{
					/* slings get STR bonus */
					if (launcher && objects[launcher->otyp].oc_skill == P_SLING)
						bonsdmg += dbon(launcher, magr);
					/* atlatls get 2x STR bonus */
					else if (launcher && launcher->otyp == ATLATL)
						bonsdmg += dbon(launcher, magr) * 2;
					/* Bonus impurity damage */
					else if (launcher && launcher->otyp == EVELYN)
						bonsdmg += u.uimpurity/2;
					/* other launchers get no STR bonus */
					else if (launcher)
						bonsdmg += 0;
					/* properly-used ranged attacks othersied get STR bonus */
					else {
						/* hack: if wearing kicking boots, you effectively have 25 STR for kicked objects */
						if (hmoncode & HMON_KICKED && youagr && uarmf && (uarmf->otyp == KICKING_BOOTS || (uarmf->otyp == IMPERIAL_ELVEN_BOOTS && check_imp_mod(uarmf, IEA_KICKING))))
							override_str = STR19(25);	/* 25 STR */
						bonsdmg += dbon(weapon, magr);
						override_str = 0;
					}
				}

			} else if (trap){
				/* some traps deal increased damage */
				if (trap->ttyp == ROLLING_BOULDER_TRAP)
					bonsdmg += d(2, level_difficulty()/3+1) - 5;	/* can be negative at early depths */
				if (trap->ttyp == ARROW_TRAP)
					bonsdmg += d(2, level_difficulty()/4+1);
				if (trap->ttyp == DART_TRAP)
					bonsdmg += d(1, level_difficulty()/8+1);
			}
		}
		/* skill damage bonus */
		if(youagr && (valid_weapon_attack || fake_valid_weapon_attack || unarmed_punch || unarmed_kick)){
			/* note: unarmed kicks do not get skill bonus damage */
			int wtype;

			/* get simple weapon skill associated with the weapon, not including twoweapon */
			wtype = weapon_skill_type(weapon, launcher, fired);
			
			/* ammo fired from a launcher */
			if (fired && launcher) {
				/* precision fired ammo gets skill bonuses, multiplied */
				if (is_ammo(weapon) && (precision_mult))
					skill_damage = weapon_dam_bonus(launcher, wtype) * precision_mult;
				/* non-precision guns (all but the sniper rifle) do not get skill damage */
				else if (is_firearm(launcher))
					skill_damage = 0;
				/* other fired ammo gets normal skill bonus */
				else
					skill_damage = weapon_dam_bonus(launcher, wtype);
			}
			/* things thrown with no launcher */
			else if (fired && !launcher) {
				/* ammo thrown without a launcher does not get skill bonuses*/
				if (is_ammo(weapon))
					skill_damage = 0;
				/* otherwise, they do get skill bonuses */
				else
					skill_damage = weapon_dam_bonus(weapon, wtype);
			}
			/* melee weapons */
			else if (melee || thrust) {
				skill_damage = weapon_dam_bonus(weapon, wtype);
			}

			/* Wrathful Spider halves damage from skill for fired bolts */
			if (launcher && launcher->oartifact == ART_WRATHFUL_SPIDER)
				skill_damage /= 2;

			/* add to the bonsdmg counter */
			bonsdmg += skill_damage;

			/* now, train skills */
			use_skill((melee && u.twoweap) ? P_TWO_WEAPON_COMBAT : wtype, 1);

			if (weapon && activeFightingForm(FFORM_GREAT_WEP) && (bimanual(weapon, youracedata) || bimanual_mod(weapon, &youmonst) > 1))
				use_skill(P_GREAT_WEP, 1);

			if (melee && weapon && is_lightsaber(weapon) && litsaber(weapon) && P_SKILL(wtype) >= P_BASIC){
				use_skill(P_SHII_CHO, 1);
				if (P_SKILL(P_SHII_CHO) >= P_BASIC || weapon->oartifact == ART_INFINITY_S_MIRRORED_ARC){
					if ((activeFightingForm(FFORM_SHII_CHO) ||
						(activeFightingForm(FFORM_MAKASHI))
						) &&
						!uarms && !u.twoweap && wtype == P_SABER
						) use_skill(P_MAKASHI, 1);
					if ((activeFightingForm(FFORM_SHII_CHO) ||
						(activeFightingForm(FFORM_ATARU))
						) &&
						u.lastmoved + 1 >= monstermoves
						) use_skill(P_ATARU, 1);
					if ((activeFightingForm(FFORM_SHII_CHO) ||
						(activeFightingForm(FFORM_DJEM_SO))
						) &&
						mdef->mattackedu
						) use_skill(P_DJEM_SO, 1);
					if ((activeFightingForm(FFORM_SHII_CHO) ||
						(activeFightingForm(FFORM_NIMAN))
						) &&
						u.lastcast >= monstermoves
						) use_skill(P_NIMAN, 1);
					if ((activeFightingForm(FFORM_SHII_CHO) ||
						(activeFightingForm(FFORM_JUYO))
						) &&
						(sneak_attack&~SNEAK_JUYO)	/* attacking a disadvantaged target, but might not have sneak dice */
						) use_skill(P_JUYO, 1);
				}
			}
		}
		/* misc: train player's Soresu skill if applicable */
		if (youdef && uwep && is_lightsaber(uwep) && litsaber(uwep) && magr && melee && P_SKILL(P_SHII_CHO) >= P_BASIC &&
			(activeFightingForm(FFORM_SHII_CHO) ||
			(activeFightingForm(FFORM_SORESU))
			)) use_skill(P_SORESU, 1);
		//Weapon traits
		if(weapon && magr){
			if(weapon->o_e_trait == ETRAIT_HEW
				&& CHECK_ETRAIT(weapon, magr, ETRAIT_HEW)
				&& (!youagr || (weapon == uwep && !u.twoweap))
				&& ROLL_ETRAIT(weapon, magr, TRUE, !rn2(10))
			){
				struct weapon_dice wdice;
				/* grab the weapon dice from dmgval_core */
				dmgval_core(&wdice, bigmonst(pd), weapon, weapon->otyp, magr);
				/* add to the tratdmg counter */
				tratdmg += weapon_dmg_roll(&wdice, youdef);
				if(youagr)
					tratdmg += weapon_dam_bonus(weapon, weapon_type(weapon));
			}
			if(weapon->o_e_trait == ETRAIT_FELL
				&& CHECK_ETRAIT(weapon, magr, ETRAIT_FELL)
				&& (!youagr || (weapon == uwep && !u.twoweap))
				&& ROLL_ETRAIT(weapon, magr, TRUE, !rn2(10))
			){
				if(youdef){
					if(!Wounded_legs){
						long side = rn2(2) ? RIGHT_SIDE : LEFT_SIDE;
						const char *sidestr = (side == RIGHT_SIDE) ? "right" : "left";
						if(u.usteed)
							Your("steed's %s is injured in the fighting!", mbodypart(u.usteed, LEG));
						else
							Your("%s %s is injured in the fighting!", sidestr, body_part(LEG));
						set_wounded_legs(side, rnd(60 - ACURR(A_DEX)));
					}
				}
				else {
					mdef->mfell += 1;
					mdef->movement -= min_ints(6, mdef->movement/(mdef->mfell+1));
					if(!mdef->mwounded_legs && !rn2(20)){
						mdef->mwounded_legs = 1;
						pline("%s %s is injured in the fighting!", s_suffix(Monnam(mdef)), mbodypart(mdef, LEG));
						struct weapon_dice wdice;
						/* grab the weapon dice from dmgval_core */
						dmgval_core(&wdice, bigmonst(pd), weapon, weapon->otyp, magr);
						/* add to the tratdmg counter */
						tratdmg += weapon_dmg_roll(&wdice, youdef);
						if(youagr)
							tratdmg += weapon_dam_bonus(weapon, weapon_type(weapon));
					}
				}
			}
			if((youdef ? has_blood(youracedata) : has_blood_mon(mdef))
				&& CHECK_ETRAIT(weapon, magr, ETRAIT_BLEED)
				&& ROLL_ETRAIT(weapon, magr, rn2(2), !rn2(10))
			){
				struct weapon_dice wdice;
				int bleeddmg;
				/* grab the weapon dice from dmgval_core */
				dmgval_core(&wdice, bigmonst(pd), weapon, weapon->otyp, magr);
				bleeddmg = wdice.oc_damn + wdice.bon_damn + wdice.flat + ROLL_ETRAIT(weapon, magr, 5, 2) + weapon->spe;
				/* add to the tratdmg counter */
				tratdmg += bleeddmg;
				if (wizard && (iflags.wizcombatdebug & WIZCOMBATDEBUG_DMG) && WIZCOMBATDEBUG_APPLIES(magr, mdef))
					pline("Bleeding wound!");
				if (vis) 
					pline("%s sustained a bleeding wound in the fighting!", youdef ? "You" : Monnam(mdef));
				if(youdef){
					mdef->mbleed += bleeddmg;
				}
				else {
					mdef->mbleed += bleeddmg;
				}
			}
			if(CHECK_ETRAIT(weapon, magr, ETRAIT_STOP_THRUST)
				&& ROLL_ETRAIT(weapon, magr, TRUE, !rn2(10))
				&& magr != mdef
				&& !((youdef || youagr) && (u.ustuck == magr || u.ustuck == mdef))
				&& ((!youdef && mdef->mprev_dir.x == sgn(x(magr) - x(mdef)) && mdef->mprev_dir.y == sgn(y(magr) - y(mdef)))
					|| (youdef && u.umoved && u.prev_dir.x == sgn(x(magr) - x(mdef)) && u.prev_dir.y == sgn(y(magr) - y(mdef)))
					)
			){
				struct weapon_dice wdice;
				if (wizard && (iflags.wizcombatdebug & WIZCOMBATDEBUG_DMG) && WIZCOMBATDEBUG_APPLIES(magr, mdef))
					pline("Stop thrust!");
				if(youagr)
					You("use %s momentum to your advantage!", s_suffix(mon_nam(mdef)));
				/* grab the weapon dice from dmgval_core */
				dmgval_core(&wdice, bigmonst(pd), weapon, weapon->otyp, magr);
				/* add to the tratdmg counter */
				tratdmg += weapon_dmg_roll(&wdice, youdef);
				if(youagr)
					tratdmg += weapon_dam_bonus(weapon, weapon_type(weapon));
			}
			if(CHECK_ETRAIT(weapon, magr, ETRAIT_BLADESONG)){
				struct weapon_dice wdice;
				/* grab the weapon dice from dmgval_core */
				dmgval_core(&wdice, bigmonst(pd), weapon, weapon->otyp, magr);
				/* Maybe add to the tratdmg counter */
				if(youagr){
					long timeout = u.bladesong + (Race_if(PM_ELF) ? 3 : 0);
					if(timeout >= monstermoves && 
						(Race_if(PM_ELF) ?
						 (timeout >= monstermoves+4 || ROLL_ETRAIT(weapon, magr, TRUE, rn2((monstermoves+6) - u.bladesong))):
						 (timeout >= monstermoves+10 || ROLL_ETRAIT(weapon, magr, TRUE, rn2((monstermoves+10) - u.bladesong)))
						)
					){
						tratdmg += weapon_dmg_roll(&wdice, youdef);
						tratdmg += weapon_dam_bonus(weapon, weapon_type(weapon));
						if (wizard && (iflags.wizcombatdebug & WIZCOMBATDEBUG_DMG) && WIZCOMBATDEBUG_APPLIES(magr, mdef))
							pline("Blade song!");
					}
				}
				else if(is_elf(pa) && magr->mspec_used && ROLL_ETRAIT(weapon, magr, TRUE, !rn2(3))){
					if (wizard && (iflags.wizcombatdebug & WIZCOMBATDEBUG_DMG) && WIZCOMBATDEBUG_APPLIES(magr, mdef))
						pline("Blade song!");
					tratdmg += weapon_dmg_roll(&wdice, youdef);
				}
			}
			if(CHECK_ETRAIT(weapon, magr, ETRAIT_BLADEDANCE)){
				struct weapon_dice wdice;
				/* grab the weapon dice from dmgval_core */
				dmgval_core(&wdice, bigmonst(pd), weapon, weapon->otyp, magr);
				/* Maybe add to the tratdmg counter */
				if(youagr){
					int dx1 = u.prev_dir.x;
					int dy1 = u.prev_dir.y;
					//Note: allows thrown etc.
					int dx = x(mdef) - x(magr);
					int dy = y(mdef) - y(magr);
					if((dx != dx1 || dy != dy1) && (dx1 || dy1)){
						int tmpdmg;
						tmpdmg = weapon_dmg_roll(&wdice, youdef);
						tmpdmg += weapon_dam_bonus(weapon, weapon_type(weapon));
						if(Race_if(PM_ELF))
							tmpdmg += (ACURR(A_CHA)+1)/2;
						tratdmg += ROLL_ETRAIT(weapon, magr, tmpdmg, (tmpdmg+2)/3);
						if (wizard && (iflags.wizcombatdebug & WIZCOMBATDEBUG_DMG) && WIZCOMBATDEBUG_APPLIES(magr, mdef))
							pline("Blade dance!");
						You("twirl and strike!");
					}
				}
				else if(is_elf(pa) && ROLL_ETRAIT(weapon, magr, rn2(3), !rn2(3))){
					tratdmg += weapon_dmg_roll(&wdice, youdef);
					if (wizard && (iflags.wizcombatdebug & WIZCOMBATDEBUG_DMG) && WIZCOMBATDEBUG_APPLIES(magr, mdef))
						pline("Blade dance!");
				}
			}
			if(braced_weapon){
				struct weapon_dice wdice;
				if (wizard && (iflags.wizcombatdebug & WIZCOMBATDEBUG_DMG) && WIZCOMBATDEBUG_APPLIES(magr, mdef))
					pline("Braced attack!");
				if(youagr)
					You("turn %s aggression to your advantage!", s_suffix(mon_nam(mdef)));
				/* grab the weapon dice from dmgval_core */
				dmgval_core(&wdice, bigmonst(pd), weapon, weapon->otyp, magr);
				/* add to the tratdmg counter */
				tratdmg += weapon_dmg_roll(&wdice, youdef);
				if(youagr)
					tratdmg += weapon_dam_bonus(weapon, weapon_type(weapon));
			}
			if(stunning_strike && has_head_mon(mdef)){
				struct weapon_dice wdice;
				if (wizard && (iflags.wizcombatdebug & WIZCOMBATDEBUG_DMG) && WIZCOMBATDEBUG_APPLIES(magr, mdef))
					pline("Stun bonus damage!");
				/* grab the weapon dice from dmgval_core */
				dmgval_core(&wdice, bigmonst(pd), weapon, weapon->otyp, magr);
				/* add to the tratdmg counter */
				tratdmg += weapon_dmg_roll(&wdice, youdef) + d(2,10);
				if(youagr)
					tratdmg += weapon_dam_bonus(weapon, weapon_type(weapon));
			}
			if(modifier_flags&MELEEHURT_LONGSLASH_MASK){
				struct weapon_dice wdice;
				long longcount = modifier_flags&MELEEHURT_LONGSLASH_MASK;
				if (wizard && (iflags.wizcombatdebug & WIZCOMBATDEBUG_DMG) && WIZCOMBATDEBUG_APPLIES(magr, mdef))
					pline("Long slash %ldx!", longcount);
				/* grab the weapon dice from dmgval_core */
				dmgval_core(&wdice, bigmonst(pd), weapon, weapon->otyp, magr);
				/* add to the tratdmg counter */
				for(; longcount > 0; longcount--){
					tratdmg += weapon_dmg_roll(&wdice, youdef);
					if(youagr)
						tratdmg += weapon_dam_bonus(weapon, weapon_type(weapon));
				}
			}
		}
	}
	/* ARTIFACT HIT BLOCK */
	/* this must come after skills are trained, as this can kill the defender and cause a return */
	if(youagr && melee){
		if(check_rot(ROT_FEAST))
			healup((*hpmax(magr))*.016, 0, TRUE, FALSE);
		if(valid_weapon_attack && weapon && weapon->oartifact == ART_DIRGE && check_mutation(CRAWLING_FLESH))
			healup(1, 0, FALSE, FALSE);
	}
	if (valid_weapon_attack || unarmed_punch || unarmed_kick || unarmed_butt)
	{
		int returnvalue = 0;
		boolean artif_hit = FALSE;
		/* use guidance glyph */
		if (youagr && melee){
			if(active_glyph(GUIDANCE))
				doguidance(mdef, basedmg);
			if(weapon){
				if(weapon->oartifact == ART_HOLY_MOONLIGHT_SWORD){
					if(Insight >= 40)
						doguidance(mdef, active_glyph(GUIDANCE) ? basedmg : basedmg/2);
					else if(active_glyph(GUIDANCE)){
						doguidance(mdef, (Insight*basedmg)/40);
					}
					else {
						doguidance(mdef, (Insight*basedmg)/80);
					}
				}
				else if(weapon->otyp == CHURCH_BLADE && Insight >= 40){
					doguidance(mdef, active_glyph(GUIDANCE) ? basedmg/2 : basedmg/4);
				}
			}
		}
		/* hits with a valid weapon proc effects of the weapon */
		if (valid_weapon_attack) {
			otmp = weapon;
			if (otmp) {
				returnvalue = apply_hit_effects(magr, mdef, otmp, weapon, basedmg, &artidmg, &elemdmg, dieroll, &hittxt, TRUE, TRUE);
				/* if the weapon caused a miss and we incremented u.uconduct.weaphit, decrement decrement it back */
				if (returnvalue == MM_MISS && youagr && (melee || thrust))
					u.uconduct.weaphit--;
				if (returnvalue == MM_MISS || (returnvalue & (MM_DEF_DIED|MM_DEF_LSVD|MM_AGR_STOP)))
					return returnvalue;
				if (otmp->oartifact)
					artif_hit = TRUE;
			}
			if(!fired && otmp->otyp == CARCOSAN_STING && otmp->ovar1_charges < 95)
				otmp->ovar1_charges += rnd(5);
		}
		/* ranged weapon attacks also proc effects of the launcher */
		if (fired && launcher && valid_weapon_attack) {
			otmp = launcher;
			if (otmp) {
				returnvalue = apply_hit_effects(magr, mdef, otmp, weapon, basedmg, &artidmg, &elemdmg, dieroll, &hittxt, FALSE, FALSE);
				if (returnvalue == MM_MISS || (returnvalue & (MM_DEF_DIED | MM_DEF_LSVD | MM_AGR_STOP)))
					return returnvalue;
				if (otmp->oartifact)
					artif_hit = TRUE;
			}
		}
		/* ranged weapon attacks also proc effects of The Helm of the Arcane Archer */
		if (fired && launcher && valid_weapon_attack &&
			((otmp = (youagr ? uarmh : which_armor(magr, W_ARMH))) &&
			otmp->oartifact == ART_HELM_OF_THE_ARCANE_ARCHER)) {
			if (otmp) {
				returnvalue = apply_hit_effects(magr, mdef, otmp, weapon, basedmg, &artidmg, &elemdmg, dieroll, &hittxt, FALSE, FALSE);
				if (returnvalue == MM_MISS || (returnvalue & (MM_DEF_DIED | MM_DEF_LSVD | MM_AGR_STOP)))
					return returnvalue;
				if (otmp->oartifact)
					artif_hit = TRUE;
			}
		}
		/* unarmed punches proc effects of worn gloves */
		if (unarmed_punch) {
			//Monsters have extra damage for their attacks, it makes sense to treat it as part of the unarmed damage.
			int unarmed_basedmg = basedmg + ((youagr && !natural_strike) ? 0 : monsdmg);
			otmp = (youagr ? uarmg : which_armor(magr, W_ARMG));
			if (otmp) {
				returnvalue = apply_hit_effects(magr, mdef, otmp, (struct obj *)0, unarmed_basedmg, &artidmg, &elemdmg, dieroll, &hittxt, TRUE, TRUE);
				if (returnvalue == MM_MISS || (returnvalue & (MM_DEF_DIED | MM_DEF_LSVD | MM_AGR_STOP)))
					return returnvalue;
				if (otmp->oartifact)
					artif_hit = TRUE;
			}
		}
		else if(magr){
			//Revenancer claws count as an artifact ring (Drains and procs cult oprops)
			otmp = (youagr ? uarmg : which_armor(magr, W_ARMG));
			if(otmp && otmp->oartifact == ART_CLAWS_OF_THE_REVENANCER){
				returnvalue = apply_hit_effects(magr, mdef, otmp, (struct obj *)0, basedmg, &artidmg, &elemdmg, dieroll, &hittxt, FALSE, FALSE);
				if (returnvalue == MM_MISS || (returnvalue & (MM_DEF_DIED | MM_DEF_LSVD | MM_AGR_STOP)))
					return returnvalue;
				artif_hit = TRUE;
			}
		}
		/* unarmed kicks proc effects of worn boots */
		if (unarmed_kick) {
			//Monsters have extra damage for their attacks, it makes sense to treat it as part of the unarmed damage.
			int unarmed_basedmg = basedmg + ((youagr && !natural_strike) ? 0 : monsdmg);
			otmp = (youagr ? uarmf : which_armor(magr, W_ARMF));
			if (otmp) {
				returnvalue = apply_hit_effects(magr, mdef, otmp, (struct obj *)0, unarmed_basedmg, &artidmg, &elemdmg, dieroll, &hittxt, TRUE, TRUE);
				if (returnvalue == MM_MISS || (returnvalue & (MM_DEF_DIED | MM_DEF_LSVD | MM_AGR_STOP)))
					return returnvalue;
				if (otmp->oartifact)
					artif_hit = TRUE;
			}
		}
		/* unarmed headbutts proc effects of worn helms */
		if (unarmed_butt) {
			//Monsters have extra damage for their attacks, it makes sense to treat it as part of the unarmed damage.
			int unarmed_basedmg = basedmg + ((youagr && !natural_strike) ? 0 : monsdmg);
			otmp = (youagr ? uarmh : which_armor(magr, W_ARMH));
			if (otmp) {
				returnvalue = apply_hit_effects(magr, mdef, otmp, (struct obj *)0, unarmed_basedmg, &artidmg, &elemdmg, dieroll, &hittxt, TRUE, TRUE);
				if (returnvalue == MM_MISS || (returnvalue & (MM_DEF_DIED | MM_DEF_LSVD | MM_AGR_STOP)))
					return returnvalue;
				if (otmp->oartifact)
					artif_hit = TRUE;
			}
		}
		/* all valid attacks proc effects of offensive rings */
		if(youagr){
			struct obj *rings[] = {uleft, uright};
			for(int i = 0 ; i < 2; i++){
				otmp = rings[i];
				if(!otmp)
					continue;
				// Note: artifact rings are currently set to always add their damage, but to only print the generic x hits messages when unarmed.
				returnvalue = apply_hit_effects(magr, mdef, otmp, (struct obj *)0, basedmg, &artidmg, &elemdmg, dieroll, &hittxt, unarmed_punch, unarmed_punch);
				if (returnvalue == MM_MISS || (returnvalue & (MM_DEF_DIED | MM_DEF_LSVD | MM_AGR_STOP)))
					return returnvalue;
				if (otmp->oartifact)
					artif_hit = TRUE;
			}
			if(Withering_stake && mdef && u.uevent.qcompleted){
				if(quest_status.time_doing_quest < UH_QUEST_TIME_2 && !mvitals[PM_MOON_S_CHOSEN].died){
					if(!Fire_res(mdef))
						elemdmg += rnd(species_resists_cold(mdef) ? 20 : 10);
				}
				else {
					if(!Cold_res(mdef))
						elemdmg += species_resists_fire(mdef) ? 2 : 1;
					if(hates_silver(pd))
						elemdmg += rnd(2);
				}
			}
		}

		/* must come after all apply_hit_effects */
		/* priests do extra damage with all artifacts */
		if (artif_hit && !recursed && magr && (youagr ? Role_switch : monsndx(magr->data)) == PM_PRIEST)
			artidmg += d(1, mlev(magr));
		/* madmen do extra damage with insight weapons */
		if (valid_weapon_attack && is_insight_weapon(weapon) && !recursed && magr && (youagr ? (Role_if(PM_MADMAN) || u.sealsActive&SEAL_OSE || mvitals[PM_MOON_S_CHOSEN].died) : insightful(magr->data))){
			if(youagr){
				if(Insight)
					bonsdmg += d(1, (min(Insight, (bimanual(weapon, youracedata) ? 2 : 1) * mlev(magr))+1)/2);
			}
			else {
				bonsdmg += d(1, ((bimanual(weapon, magr->data) ? 2 : 1) * mlev(magr) + 1)/2+1);
			}
		}
	}

	/* Sum reduceable damage */
	subtotl = basedmg
		+ artidmg
		+ bonsdmg
		+ ((youagr && !natural_strike) ? 0 : monsdmg)	/* only add monsdmg for monsters or a player making a monster attack */
		+ snekdmg
		+ jostdmg
		+ tratdmg;

	/*physical serration adjustment*/
	if(weapon && is_serrated(weapon) && is_serration_vulnerable(mdef)){
		subtotl *= 1.2;
	}
	//There is something in the tip that, when driven deep, is deleterious to beasts and the ritually impure
	if(weapon 
		&& (weapon->otyp == CHURCH_PICK || (weapon->otyp == CHURCH_SHORTSWORD && !(resist_pierce(pd) && !resist_slash(pd))))
		&& (is_animal(pd) || (youdef && u.uimpurity > 10)
			|| pd->mtyp == PM_DEEP_ONE || pd->mtyp == PM_DEEPER_ONE
			|| pd->mtyp == PM_KUO_TOA || pd->mtyp == PM_KUO_TOA_WHIP
			|| pd->mtyp == PM_BEING_OF_IB || pd->mtyp == PM_PRIEST_OF_IB
			|| is_mind_flayer(pd) || is_were(pd)
			|| pd->mtyp == PM_BEFOULED_WRAITH || mdef->mtraitor || mdef->mferal
		)
	){
		subtotl *= Insight >= 40 ? 1.5 : 1.2;
	}


	/* If the character is panicking, all their attacks do half damage */
	if(Panicking){
		subtotl = subtotl/2+1;
	}
	
	/* Reduce Incoming Damage */
	/* min 1 base damage (0 if real_attack is false) */
	if (subtotl < 1)
		subtotl = (real_attack ? 1 : 0);

	/* some attacks only deal searing damage to insubstantial creatures */
	if (insubstantial(pd) && hits_insubstantial(magr, mdef, attk, weapon) == 1) {
		subtotl = 0;
	}
	
	/* Apply DR before multiplicative defences/vulnerabilites */
	int hit_slot = ROLL_SLOT;
	if (subtotl > 0){
		int dr = 0;
		if (phase_armor){
			dr = (youdef ? (base_udr() + base_nat_udr()) : (base_mdr(mdef) + base_nat_mdr(mdef)));
		}
		else if(!attk){
			dr = (youdef ? roll_udr(magr, ROLL_SLOT) : roll_mdr(mdef, magr, ROLL_SLOT));
		}
		else if(attk->adtyp == AD_LEGS)
		{
			dr = (youdef ? roll_udr(magr, LEG_DR) : roll_mdr(mdef, magr, LEG_DR));
		}
		else if(weapon && magr){
			if(weapon->o_e_trait == ETRAIT_HEW && CHECK_ETRAIT(weapon, magr, ETRAIT_HEW)){
				int hewslots[] = {HEAD_DR, UPPER_TORSO_DR, ARM_DR};
				hit_slot = ROLL_FROM(hewslots);
			}
			else if(weapon->o_e_trait == ETRAIT_FELL && CHECK_ETRAIT(weapon, magr, ETRAIT_FELL)){
				int hewslots[] = {LEG_DR, LOWER_TORSO_DR};
				hit_slot = ROLL_FROM(hewslots);
			}
			else if(weapon->o_e_trait&ETRAIT_FOCUS_FIRE && CHECK_ETRAIT(weapon, magr, ETRAIT_FOCUS_FIRE)){
				if(ROLL_ETRAIT(weapon, magr, TRUE, !rn2(5))){
					int min_slot = UPPER_TORSO_DR;
					int min_dr = (youdef ? roll_udr(magr, UPPER_TORSO_DR) : roll_mdr(mdef, magr, UPPER_TORSO_DR));
					for(hit_slot = LOWER_TORSO_DR; hit_slot <= ARM_DR; hit_slot = hit_slot<<1){
						dr = (youdef ? roll_udr(magr, hit_slot) : roll_mdr(mdef, magr, hit_slot));
						if(dr < min_dr){
							min_dr = dr;
							min_slot = hit_slot;
						}
					}
					//Targets lowest
					hit_slot = min_slot;
				}
			}
			else if(is_lightsaber(weapon) && litsaber(weapon)){
				int saber_slots[] = {HEAD_DR, UPPER_TORSO_DR, ARM_DR, LEG_DR, LOWER_TORSO_DR};
				hit_slot = ROLL_FROM(saber_slots);
			}
			dr = (youdef ? roll_udr(magr, hit_slot) : roll_mdr(mdef, magr, hit_slot));
		}
		else {
			dr = (youdef ? roll_udr(magr, ROLL_SLOT) : roll_mdr(mdef, magr, ROLL_SLOT));
		}
		
		//Give spears a slight advantage vs. armor.
		if(valid_weapon_attack && weapon && is_spear(weapon) && dr)
			dr = max(dr-2, 0);

		//Give skilled rangers (and others) some help vs. armor
		if(fired && launcher && valid_weapon_attack && weapon && is_aimable(weapon, attackmask) && dr)
			dr = max(dr-skill_damage, 0);
		//Armor-penetrating weapons do 0-1x or 1-2x bonus skill damage to dr (up to +10 vs dr)
		if(valid_weapon_attack && weapon && magr && CHECK_ETRAIT(weapon, magr, ETRAIT_PENETRATE_ARMOR) && ROLL_ETRAIT(weapon, magr, TRUE, rn2(2))){
			dr = max_ints(dr - (ROLL_ETRAIT(weapon, magr, rnd(2), 1) * skill_damage), 0);
		}
		
		subtotl -= dr;
		
		/* can only reduce damage to 1 */
		if (subtotl < 1)
			subtotl = 1;
	}

	/* some creatures resist weapon attacks to the extreme */
	if (resist_attacks(pd) && (pd->mtyp != PM_ASPECT_OF_THE_SILENCE || !youagr || !u.uaesh_duration)
		&& (unarmed_punch || unarmed_kick || valid_weapon_attack || invalid_weapon_attack)
	) {
		if (subtotl > 0) {
			resisted_weapon_attacks = TRUE;
			subtotl = 1;
		}
	}
	/* other creatures resist specific types of attacks */
	else if (unarmed_punch || unarmed_kick || valid_weapon_attack || invalid_weapon_attack) {
		/* attackmask has a larger scope so it can be referenced in the resist message later */
		int resistmask = 0;

		/* get attackmask */
		if (weapon && (valid_weapon_attack || invalid_weapon_attack)) {
			attackmask = attack_mask(weapon, 0, 0, magr);
			otmp = weapon;
		}
		else if (unarmed_punch) {
			//Can always whack someone
			attackmask = WHACK;
			/* gloves */
			otmp = (youagr ? uarmg : which_armor(magr, W_ARMG));
			if(otmp)
				attackmask = attack_mask(otmp, 0, 0, magr);

			if (/* claw attacks are slashing (even while wearing gloves?) */
				(attk && attk->aatyp == AT_CLAW) ||
				/* and so are the player's punches, as a half-dragon or chiropteran */
				(youagr && (Race_if(PM_HALF_DRAGON) || (!Upolyd && Race_if(PM_CHIROPTERAN))))
				)
				attackmask |= SLASH;

			if (youagr && check_mutation(SHUB_CLAWS))
				attackmask |= SLASH|PIERCE;

			if (/* claw attacks are slashing (even while wearing gloves?) */
				(youagr && u.sealsActive&SEAL_EURYNOME)
			){
				if(rn2(2))
					attackmask |= SLASH;
				if(rn2(2))
					attackmask |= PIERCE;
				//Less often, the limb is thin and non-whacky.
				if(attackmask&(PIERCE|SLASH) && !rn2(3))
					attackmask &= ~WHACK;
			}
		}
		else if (unarmed_kick) {
			//Can always whack someone
			attackmask = WHACK;

			otmp = (youagr ? uarmf : which_armor(magr, W_ARMF));
			if(otmp)
				attackmask = attack_mask(otmp, 0, 0, magr);
		}
		else {
			/* something odd -- maybe it was a weapon attack and the weapon was destroyed earlier than usual? */
			otmp = (struct obj *)0;
			/* lets just ignore resistances */
			attackmask |= (WHACK | SLASH | PIERCE);
		}

		/* get resistmask */
		if (resist_blunt(pd)){
			resistmask |= WHACK;
		}
		if (resist_pierce(pd)){
			resistmask |= PIERCE;
		}
		if (resist_slash(pd)){
			resistmask |= SLASH;
		}

		if ((thick_skinned(pd) || (youdef && u.sealsActive&SEAL_ECHIDNA)) && (
			(unarmed_kick && !(youagr && !Upolyd && Race_if(PM_CLOCKWORK_AUTOMATON)) && !(otmp && (
				otmp->otyp == STILETTOS || otmp->otyp == WIND_AND_FIRE_WHEELS || otmp->otyp == HEELED_BOOTS || otmp->otyp == KICKING_BOOTS 
				|| is_metallic(otmp) || (otmp->otyp == IMPERIAL_ELVEN_BOOTS && check_imp_mod(otmp, IEA_KICKING))))
			) || (otmp && (valid_weapon_attack || invalid_weapon_attack) && (otmp->obj_material <= LEATHER) && !litsaber(otmp))
			))
		{
			if(otmp && otmp->oartifact == ART_IBITE_ARM){
				if(otmp->otyp == CLAWED_HAND); /*Ghost hand doesn't notice it's too soft to harm the target*/
				else {
					/* Ghost elbow not so much: damage entirely mitigated */
					subtotl = 1;
					resisted_thick_skin = TRUE;
				}
			}
			else if(otmp && (otmp->oartifact || check_oprop(otmp, OPROP_FLAYW))){
				/* damage partly mitigated */
				subtotl /= 4;
				partly_resisted_thick_skin = TRUE;
			}
			else if(otmp && check_oprop(otmp, OPROP_LESSER_FLAYW)){
				/* damage partly mitigated */
				subtotl /= 6;
				partly_resisted_thick_skin = TRUE;
			}
			else {
				/* damage entirely mitigated */
				subtotl = 0;
				resisted_thick_skin = TRUE;
			}
		}
		if ((attackmask & ~(resistmask)) == 0L && !(otmp && spec_applies(otmp, mdef, TRUE)) && (subtotl > 0)) {
			/* damage reduced by 75% */
			subtotl /= 4;
			resisted_attack_type = TRUE;
			/* can only reduce damage to 1 */
			if (subtotl < 1)
				subtotl = 1;
		}
		else if (subtotl > 0 && vulnerable_mask(resistmask) && !(attackmask & EXPLOSION) && (attackmask & ~(resistmask)) != 0L) {
			/* 2x damage for attacking a vulnerability */
			subtotl *= 2;
		}
	}

	/* Half Physical Damage -- does not stack with damage-type resistance */
	if (!resisted_attack_type && (subtotl > 0)) {
		subtotl = reduce_dmg(mdef,subtotl,TRUE,FALSE);
		/* can only reduce damage to 1 */
		if (subtotl < 1)
			subtotl = 1;
	}

	/* thalassophobic players can deal greatly reduced damage to sea creatures */
	if (youagr && is_aquatic(pd) && roll_madness(MAD_THALASSOPHOBIA)){
		subtotl = (subtotl + 9)/10;
	}

	/* deep dwellers resist attacks, but have a 1/10 chance of being slain outright */
	if (pd->mtyp == PM_DEEP_DWELLER && !rn2(10)){
		/*Brain struck.  Ouch.*/
		if (youdef)
			pline("Your brain-organ is struck!");
		else if (canseemon(mdef))
			pline("%s brain-organ is struck!", s_suffix(Monnam(mdef)));
		*hp(mdef) = 1;
		partly_resisted_thick_skin = FALSE;
		resisted_weapon_attacks = FALSE;
		resisted_attack_type = FALSE;
		resisted_thick_skin = FALSE;
	}

	/* hack to enhance mm_aggression(); we don't want purple
	worm's bite attack to kill a shrieker because then it
	won't swallow the corpse; but if the target survives,
	the subsequent engulf attack should accomplish that */
	if (magr && melee && !youdef && !youagr &&
		pa->mtyp == PM_PURPLE_WORM &&
		pd->mtyp == PM_SHRIEKER &&
		(subtotl > *hp(mdef)))
		subtotl = max(*hp(mdef) - 1, 1);

	/*non-physical serration adjustment*/
	if(weapon && is_serrated(weapon) && is_serration_vulnerable(mdef)){
		elemdmg *= 1.2;
		specdmg *= 1.2;
		//Subtotal done before DR
	}

	/*clawmark adjustment*/
	if(youagr && sneak_attack && active_glyph(CLAWMARK)){
		specdmg += 3*(subtotl + seardmg + elemdmg + poisdmg + specdmg)/10;
	}

	/* Final sum of damage */
	totldmg = subtotl + seardmg + elemdmg + poisdmg + specdmg;
	/* Tobiume can adjust this sum */
	if(weapon && weapon->oartifact == ART_TOBIUME && !Fire_res(mdef)){
		if((*hp(mdef) - totldmg) <= (6 + weapon->spe)){
			elemdmg += 6 + weapon->spe;
			totldmg += 6 + weapon->spe;
		}
	}
	/* Is the damage lethal? */
	lethaldamage = (totldmg >= *hp(mdef));

	/* Debug mode: report sum components */
	if (wizard && (iflags.wizcombatdebug & WIZCOMBATDEBUG_FULLDMG) && WIZCOMBATDEBUG_APPLIES(magr, mdef)) {
		pline("dmg = (b:%d + art:%d + bon:%d + mon:%d + s/j/t:%d - defense) = %d; + add:%d = %d",
			basedmg,
			artidmg,
			bonsdmg,
			((youagr && !natural_strike) ? 0 : monsdmg),	/* only add monsdmg for monsters or a player making a monster attack */
			snekdmg + jostdmg + tratdmg,
			subtotl,
			seardmg + elemdmg + poisdmg + specdmg,
			totldmg
			);
	}
	
	/* Now that all the damage has FINALLY been calculated, Tobiume should do its thing. Since this prints a message it blocks message printing */
	if (vis && weapon && weapon->oartifact == ART_TOBIUME && lethaldamage && magr){
		int dx = x(mdef) - x(magr);
		int dy = y(mdef) - y(magr);
		if(youdef || canseemon(mdef)){
			pline("%s %s %s by %s blow!",
				(youdef ? "You" : Monnam(mdef)),
				(youdef ? "are" : "is"),
				(mdef->data->msize < MZ_HUGE ? "thrown" : "stunned"),
				(youagr ? "your" : (magr && (vis&VIS_MAGR)) ? s_suffix(mon_nam(magr)) : magr ? "a" : "the")
				);
			dohitmsg = FALSE;
		}
		if (youdef)
			hurtle(dx, dy, BOLT_LIM, FALSE, TRUE);
		else {
			mhurtle(mdef, dx, dy, BOLT_LIM, FALSE);
			if (DEADMONSTER(mdef))
				return MM_DEF_DIED;
			if(MIGRATINGMONSTER(mdef))
				return MM_AGR_STOP;
		}
		
		if(x(mdef)) explode(x(mdef), y(mdef),
			AD_FIRE, 0,
			d(6, 6),
			EXPL_FIERY, 1);
	}
	if(hit_slot != ROLL_SLOT && weapon && magr && is_lightsaber(weapon) && litsaber(weapon)){
		saber_damage_slot(mdef, weapon, hit_slot, lethaldamage, vis, &hittxt);
	}
	/* PRINT HIT MESSAGE. MAYBE. */
	if (dohitmsg && vis){
		if (thrown && !hittxt)
		{
			if (youdef) {
				if (!Blind) {
					pline("%s %s you!", The(mshot_xname(weapon)), vtense(mshot_xname(weapon), "hit"));
				}
				else {
					pline("You are hit!");
				}
			}
			else {
				if ((!cansee(bhitpos.x, bhitpos.y) && !canspotmon(mdef) &&
					!(u.uswallow && mdef == u.ustuck))
					|| !flags.verbose)
				{
					pline("%s %s it.", The(mshot_xname(weapon)), vtense(mshot_xname(weapon), "hit"));
				}
				else {
					if (vis&VIS_MAGR) {
						pline("%s %s %s%s", The(mshot_xname(weapon)), vtense(mshot_xname(weapon), "hit"),
							mon_nam(mdef), exclam(totldmg));
					}
					else {
						pline("%s is hit by a %s!", Monnam(mdef), mshot_xname(weapon));
					}
				}
			}
		}
		else {
			if (!hittxt &&
				!jousting &&
				!staggering_strike &&
				!stunning_strike &&
				!(youagr && lethaldamage) &&
				!(youagr && snekdmg))
			{
				xyhitmsg(magr, mdef, originalattk);
			}
		}
	}

	/* Print additional messages and do more effects */
	/*
	 * Additional Messages
	 *  - juyo sneak attack
	 *  - sneak attack (suppress if blow will kill)
	 *  - jousting
	 *  - shattering strike (suppress if blow will kill)
	 *  - staggering strike (suppress if blow will kill)
	 *  - type-resistance (suppress if blow will kill)
	 *  - weapon-resistance (suppress if blow will kill)
	 *  - clockwork heat
	 *  - iron/silver/holy/unholy hating
	 *  - poison (if vs player, NOW call poisoned() since it will print messages)
	 *  - sword of blood
	 *  - blade of mercy conflict
	 */

	/* sneak attack messages only if the player is attacking */
	if (youagr) {
		if (snekdmg && (sneak_attack & SNEAK_JUYO)) {
			/* always message, because... */
			if (stationary_mon(mdef) || sessile(pd))		You("rain blows on the immobile %s%s", l_monnam(mdef), exclam(subtotl));
			else if (sneak_attack & SNEAK_BEHIND)	You("rain blows on %s from behind%s", mon_nam(mdef), exclam(subtotl));
			else if (sneak_attack & SNEAK_BLINDED)	You("rain blows on the blinded %s%s", l_monnam(mdef), exclam(subtotl));
			else if (sneak_attack & SNEAK_TRAPPED)	You("rain blows on the trapped %s%s", l_monnam(mdef), exclam(subtotl));
			else if (sneak_attack & SNEAK_HELPLESS) You("rain blows on the helpless %s%s", l_monnam(mdef), exclam(subtotl));
			else if (sneak_attack & SNEAK_SUICIDAL) You("rain blows on the suicidal %s%s", l_monnam(mdef), exclam(subtotl));
			else if (sneak_attack & SNEAK_OPEN) 	You("rain blows on the open %s%s", l_monnam(mdef), exclam(subtotl));
			else									You("rain blows on %s%s", mon_nam(mdef), exclam(subtotl));
			/* ...player gets bonus movement points! */
			switch (min(P_SKILL(P_JUYO), P_SKILL(weapon_type(weapon)))){
			case P_BASIC:	youmonst.movement += NORMAL_SPEED / 4;	break;
			case P_SKILLED:	youmonst.movement += NORMAL_SPEED / 3;	break;
			case P_EXPERT:	youmonst.movement += NORMAL_SPEED / 2;	break;
			}
		}
		else if (snekdmg) {
			/* don't message if the attack is lethal, or if the attack dealt no damage (excluding X-hating/poison/etc) */
			if (!lethaldamage && (subtotl > 0)) {
				if (sneak_attack & SNEAK_HIDDEN)		You("%s the flat-footed %s%s",
					(distmin(x(magr), y(magr), x(mdef), y(mdef)) > BOLT_LIM) ? "snipe" : "strike", l_monnam(mdef), exclam(subtotl));
				else if (sneak_attack & SNEAK_BEHIND)	You("strike %s from behind%s", mon_nam(mdef), exclam(subtotl));
				else if (sneak_attack & SNEAK_BLINDED)	You("strike the blinded %s%s", l_monnam(mdef), exclam(subtotl));
				else if (sneak_attack & SNEAK_TRAPPED)	You("strike the trapped %s%s", l_monnam(mdef), exclam(subtotl));
				else if (sneak_attack & SNEAK_HELPLESS)	You("strike the helpless %s%s", l_monnam(mdef), exclam(subtotl));
				else if (sneak_attack & SNEAK_SUICIDAL)	You("strike the suicidal %s%s", l_monnam(mdef), exclam(subtotl));
				else if (sneak_attack & SNEAK_OPEN)		You("strike the open %s%s", l_monnam(mdef), exclam(subtotl));
				else									You("strike %s%s", mon_nam(mdef), exclam(subtotl));
			}
		}
	}
	/* jousting -- message, break lance */
	if (jousting) {
		/* message */
		if (youagr) {
			You("joust %s%s",
				mon_nam(mdef), canseemon(mdef) ? exclam(subtotl) : ".");
			if (jousting < 0) {
				Your("%s shatters on impact!", xname(weapon));
				destroy_one_magr_weapon = TRUE;
				/* (must be either primary or secondary weapon to get here) */
				u.twoweap = FALSE;	/* untwoweapon() is too verbose here */
				if (weapon == uwep)
					uwepgone();		/* set unweapon */
				if (weapon == uswapwep)
					uswapwepgone();	/* set unweapon */

				/* useup() will be called later */
			}
			if(lethaldamage && u.usteed){
				grow_up(u.usteed, mdef);
			}
		}
		else {
			impossible("Monsters jousting? Someone forgot to fully implement this...");
		}
	}
	/* shattering strike -- attempt to destroy the defender's weapon */
	if (shattering_strike) {
		if (youagr) {
			otmp = MON_WEP(mdef);
			if (otmp &&
				!is_flimsy(otmp) &&
				!obj_resists(otmp, 0, 100)
				) {
				setmnotwielded(mdef, otmp);
				MON_NOWEP(mdef);
				mdef->weapon_check = NEED_WEAPON;
				if (is_lightsaber(weapon)) Your("energy blade slices %s %s in two!",
					s_suffix(mon_nam(mdef)), xname(otmp));
				else pline("%s %s %s from the force of your blow!",
					s_suffix(Monnam(mdef)), xname(otmp),
					otense(otmp, "shatter"));
				m_useup(mdef, otmp);
				/* If someone just shattered MY weapon, I'd flee! */
				if (rn2(4) && !mindless_mon(mdef)) {
					monflee(mdef, d(2, 3), TRUE, TRUE);
				}
			}
		}
		else {
			impossible("Monsters using shattering strikes? Someone forgot to fully implement this...");
		}
	}
	/* staggering strike */
	if (staggering_strike && !lethaldamage) {
		if (youagr) {
			if (canspotmon(mdef)) {
				pline("%s %s from your powerful %s%s!",
					Monnam(mdef),
					makeplural(stagger(mdef, "stagger")),
					((weapon && weapon->oartifact == ART_GREEN_DRAGON_CRESCENT_BLAD) ? "blow" : "strike"),
					(snekdmg && (sneak_attack & SNEAK_JUYO) ? "s" : "")
					);
			}
		}
		else {
			You("%s from %s powerful %s%s!",
				makeplural(stagger(mdef, "stagger")),
				s_suffix(mon_nam(magr)),
				((weapon && weapon->oartifact == ART_GREEN_DRAGON_CRESCENT_BLAD) ? "blow" : "strike"),
				(snekdmg && (sneak_attack & SNEAK_JUYO) ? "s" : "")
				);
		}
	}
	if (stunning_strike && !lethaldamage) {
		if (youagr) {
			if (canspotmon(mdef)) {
				pline("%s is stunned by your powerful %s%s!",
					Monnam(mdef),
					((weapon && weapon->oartifact == ART_GREEN_DRAGON_CRESCENT_BLAD) ? "blow" : "strike"),
					(snekdmg && (sneak_attack & SNEAK_JUYO) ? "s" : "")
					);
			}
		}
		else {
			You("are stunned by %s powerful %s%s!",
				s_suffix(mon_nam(magr)),
				((weapon && weapon->oartifact == ART_GREEN_DRAGON_CRESCENT_BLAD) ? "blow" : "strike"),
				(snekdmg && (sneak_attack & SNEAK_JUYO) ? "s" : "")
				);
		}
	}
	if(weapon && weapon->obj_material == HEMARGYOS){
		if(youagr && !youdef){
			if (has_blood_mon(mdef)
				&& check_vampire(VAMPIRE_BLOOD_RIP)
				&& !rn2(10)
			){
				pline("%s blood forms spears that pierce %s %s!", s_suffix(Monnam(mdef)), mhis(mdef), mbodypart(mdef, BODY_SKIN));
				totldmg += max(400, 3*mdef->mhpmax/10);
			}
			else if(!mdef->mstun && mdef->mattackedu /* || !resist_impurity(mdef, magr)*/){
				pline("%s %s!", Monnam(mdef), makeplural(stagger(mdef, "stagger")));
				mdef->mstun = TRUE;
			}
		}
		// else if(!resist_impurity(mdef, magr)){
		// }
	}
	/* disarming strike */
	if (disarming_strike && !lethaldamage) {
		if (youagr) {
			otmp = MON_WEP(mdef);
			if (otmp &&
				otmp->oartifact != ART_GLAMDRING &&
				!(otmp->cursed && !is_weldproof_mon(mdef)))
			{
				if (otmp->quan > 1L){
					otmp = splitobj(otmp, 1L);
					obj_extract_self(otmp); //wornmask is cleared by splitobj
				}
				else{
					long unwornmask;
					if ((unwornmask = otmp->owornmask) != 0L) {
						mdef->misc_worn_check &= ~unwornmask;
					}
					setmnotwielded(mdef, otmp);
					MON_NOWEP(mdef);
					mdef->weapon_check = NEED_WEAPON;
					obj_extract_self(otmp);
					otmp->owornmask = 0L;
					update_mon_intrinsics(mdef, otmp, FALSE, FALSE);
				}
				pline("%s %s is snagged by your %s.",
					s_suffix(Monnam(mdef)), xname(otmp), xname(weapon));
				getdir((char *)0);
				if (u.dx || u.dy){
					You("toss it away.");
					projectile(&youmonst, otmp, (void *)0, HMON_PROJECTILE, u.ux, u.uy, u.dx, u.dy, 0, (int)((ACURRSTR) / 2 - otmp->owt / 40 + weapon->spe), FALSE, FALSE, FALSE);
				}
				else{
					You("drop it at your feet.");
					(void)dropy(otmp);
				}
				nomul(0, NULL);
				if (mdef->mhp <= 0) /* flung weapon killed monster */
					return (MM_HIT|MM_DEF_DIED);
			}
		}
		else {
			impossible("Monsters using disarming strikes? Someone forgot to fully implement this...");
		}
	}

	/* damage resistance -- only print messages for the player, and only set/reset once per turn */
	/* also don't print message (or unset warning) if the attack is lethal anyways */
	if (youagr && !lethaldamage)
	{
		static long lastwarning = 0;

		if (lastwarning < moves) {
			if (warnedotyp != (weapon ? weapon->otyp : 0) || warnedptr != pd) {
				lastwarning = moves;
				/* warn the player that their attacks are futile */
				if (resisted_weapon_attacks) {
					pline("%s is resistant to attacks.",
						Monnam(mdef));
				}
				else if (partly_resisted_thick_skin) {
					pline("Your attack is ineffective against %s thick skin.",
						s_suffix(mon_nam(mdef)));
				}
				else if (resisted_thick_skin) {
					pline("%s thick skin nullified your attack.",
						s_suffix(Monnam(mdef)));
				}
				else if (resisted_attack_type){
					/* warn of one of your damage types */
					/* not perfectly balanced; will favour one type (P>S, S>B, B>P) 2:1 if an attack has 2 types */
					int i, j;
					static const char * damagetypes[] = { "blunt force", "sharp point", "cutting edge" };
					for (i = 0, j = rn2(3); i < 3; i++) {
						if (attackmask & (1 << (i + j) % 3)) {
							pline("The %s is ineffective against %s.",
								damagetypes[(i + j) % 3],
								mon_nam(mdef));
							break;
						}
					}
				}
				if (resisted_weapon_attacks || partly_resisted_thick_skin || resisted_thick_skin || resisted_attack_type) {
					warnedotyp = (weapon ? weapon->otyp : 0);
					warnedptr = pd;
				}
				else {
					/* reset notification */
					warnedotyp = -1;
					warnedptr = 0;
				}
			}
		}
	}

	/* fakewep: sword of blood message */
	if (swordofblood) {
		if (youdef) {
			pline("The blade of rotted blood tears through your %s!", body_part(BLOOD));
		}
		else if (vis&VIS_MDEF) {
			pline("The blade of rotted blood tears through %s %s!", s_suffix(mon_nam(mdef)), mbodypart(mdef, BLOOD));
		}
	}
	/* fakewep: sword of mercury message and inventory damage */
	if (attk && attk->adtyp == AD_MERC)
	{
		/* message */
		if (youdef) {
			pline("The metallic blade is covered in frost!");
			if (Cold_res(mdef)) {
				You("aren't cold!");
			}
		}
		else if (vis&VIS_MDEF) {
			if (!Cold_res(mdef)) {
				pline("The metallic blade freezes %s!",
					mon_nam(mdef));
			}
		}
		/* inventory damage */
		if (!UseInvCold_res(mdef)) {
			if (mlev(magr) > rn2(20))
				destroy_item(mdef, POTION_CLASS, AD_COLD);
		}
	}

	if(uuvuglory){
		pline("%s's glory sears %s!", Monnam(magr), youdef ? "you" : mon_nam(mdef));
	}
	
	/* Searing messages */
	if ((silverobj || jadeobj || ironobj || grnstlobj || holyobj || unholyobj || unblessedobj || otherobj) && (youdef || canseemon(mdef)) && !recursed) {
		long active_slots = (silverobj | jadeobj | grnstlobj | ironobj | holyobj | unholyobj | unblessedobj | otherobj);
		char buf[BUFSZ];
		char * obuf;
		boolean plural = FALSE;
		char searcount = 0;
		/* Examples:
		 * Your silver skin and blessed silver ring sear Foo's flesh!
		 * Bar's white-burning blade sears Foo!
		 * Foo's cold-iron long sword sears your flesh!
		 * Foo's cold-iron quarterstaff sears your flesh!
		 * Your cursed gloves sear Foo's flesh!
		 * Your silver small silver quarterstaff sears Foo's flesh! >_>
		 */
		/* Attacker */
		Sprintf(buf, "%s ", (magr ? (youagr ? "Your" : s_suffix(Monnam(magr))) : "The"));
		/* skin / simurgh claws */
		slot = W_SKIN;
		if (active_slots & slot) {
			searcount++;
			if (holyobj & slot)
				Strcat(buf, "glorious ");
			if (unholyobj & slot)
				Strcat(buf, "cursed ");
			if (unblessedobj & slot)
				Strcat(buf, "concordant ");
			/* special cases */
			if (attk && attk->adtyp == AD_STAR)
			{
				Strcat(buf, "starlight rapier");
			}
			else if (attk && attk->adtyp == AD_BSTR)
			{
				Strcat(buf, "black-star rapier");
			}
			else if (attk && attk->adtyp == AD_MOON)
			{
				Strcat(buf, "moonlight rapier");
			}
			else if (attk && attk->adtyp == AD_GLSS)
			{
				Strcat(buf, "shards of broken mirrors");
			}
			else if (youagr && u.sealsActive&SEAL_SIMURGH && u.sealsActive&SEAL_EDEN)
			{
				Strcat(buf, "cold-iron claws and silver skin");
			}
			else {
				if (silverobj & slot)
					Strcat(buf, "silver ");
				if (jadeobj & slot)
					Strcat(buf, "jade ");
				if (ironobj & slot)
					Strcat(buf, "cold-iron ");
				if (grnstlobj & slot)
					Strcat(buf, "green-steel ");
				Strcat(buf, 
					(youagr && u.sealsActive&SEAL_SIMURGH) ? "claws"
					: (youagr ? body_part(BODY_SKIN) : mbodypart(magr, BODY_SKIN)));
			}
		}
		/* weapon */
		slot = W_WEP;
		if (active_slots & slot) {
			searcount++;
			otmp = weapon;
			obuf = xname(otmp);

			if (active_slots & W_SKIN)
				Strcat(buf, " and ");

			if (holyobj & slot)
				Strcat(buf,
				(otmp->known && (check_oprop(otmp, OPROP_HOLYW) || check_oprop(otmp, OPROP_HOLY))) ? "holy " : 
				(otmp->known && check_oprop(otmp, OPROP_LESSER_HOLYW)) ? "consecrated " : "blessed "
				);
			if (unholyobj & slot)
				Strcat(buf,
				(otmp->known && (check_oprop(otmp, OPROP_UNHYW) || check_oprop(otmp, OPROP_UNHY))) ? "unholy " : 
				(otmp->known && check_oprop(otmp, OPROP_LESSER_UNHYW)) ? "desecrated " : "cursed "
				);
			if (unblessedobj & slot)
				Strcat(buf,
				(otmp->known && (check_oprop(otmp, OPROP_CONCW) || check_oprop(otmp, OPROP_CONC))) ? "concordant " : 
				(otmp->known && check_oprop(otmp, OPROP_LESSER_CONCW)) ? "accordant " : "uncursed "
				);
			/* special cases */
			if (otmp->oartifact == ART_SUNSWORD && (silverobj&slot)) {
				Strcat(buf, "burning-white blade");
			}
			else if (otmp->oartifact == ART_GLAMDRING && (silverobj&slot)) {
				Strcat(buf, "white-burning blade");
			}
			else {
				if (silverobj & slot){
					if (!strstri(obuf, "silver")){
						if(otmp->obj_material != SILVER && arti_silvered(otmp))
							add_silvered_art_sear_adjectives(buf, otmp);
						else Strcat(buf, (otmp->obj_material != SILVER ? "silvered " : "silver "));
					}
				}
				if (jadeobj & slot) {
					if (!strstri(obuf, "jade "))
						Strcat(buf, "jade ");
				}
				if (ironobj & slot) {
					if (!strncmpi(obuf, "iron ", 5))
						Strcat(buf, "cold-");
					else if (!strstri(obuf, "iron "))
						Strcat(buf, "cold-iron ");
				}
				if (grnstlobj & slot){
					if (!strstri(obuf, "green-steel "))
						Strcat(buf, "green-steel ");
				}
				if (otherobj & slot) {
					if (otmp->obj_material == WOOD && otmp->otyp != MOON_AXE &&
						(otmp->oward&WARD_VEIOISTAFUR) && pd->mlet == S_EEL &&
						(u.wardsknown&WARD_VEIOISTAFUR))		/* you need to know the ward to recognize it */
					{
						/* only will specifically modify "carved" */
						if (!strncmpi(obuf, "carved ", 7))
							Strcat(buf, "veioistafur-");
					}
						
				}
				Strcat(buf, xname(otmp));
			}
		}
		/* rings -- don't use xname(); "ring" is fine. */
		slot = rslot;
		if (active_slots & slot) {
			if (active_slots & (W_SKIN|W_WEP))
				Strcat(buf, " and ");
			/* only the player wears rings */
			/* get correct ring */
			otmp = (rslot == W_RINGL) ? uleft
				: (rslot == W_RINGR) ? uright
				: (struct obj *)0;

			if (otmp)
			{
				searcount++;
				if (holyobj & slot)
					Strcat(buf,
					(otmp->known && (check_oprop(otmp, OPROP_HOLYW) || check_oprop(otmp, OPROP_HOLY))) ? "holy " : 
					(otmp->known && check_oprop(otmp, OPROP_LESSER_HOLYW)) ? "consecrated " : "blessed "
					);
				if (unholyobj & slot)
					Strcat(buf,
					(otmp->known && (check_oprop(otmp, OPROP_UNHYW) || check_oprop(otmp, OPROP_UNHY))) ? "unholy " : 
					(otmp->known && check_oprop(otmp, OPROP_LESSER_UNHYW)) ? "desecrated " : "cursed "
					);
				if (unblessedobj & slot)
					Strcat(buf,
					(otmp->known && (check_oprop(otmp, OPROP_CONCW) || check_oprop(otmp, OPROP_CONC))) ? "concordant " : 
					(otmp->known && check_oprop(otmp, OPROP_LESSER_CONCW)) ? "accordant " : "uncursed "
					);
				if (silverobj & slot){
					if(otmp->obj_material != SILVER && arti_silvered(otmp))
						add_silvered_art_sear_adjectives(buf, otmp);
					else Strcat(buf, (otmp->obj_material != SILVER || (jadeobj&slot) || (ironobj&slot) ? "silvered " : "silver "));
				}
				if (jadeobj & slot) {
					Strcat(buf, "jade ");
				}
				if (ironobj & slot) {
					Strcat(buf, "cold-iron ");
				}

				Strcat(buf, "ring");
			}
			else {
				active_slots &= ~rslot;
			}
		}

		/* gloves/boots/helmet -- assumes only one of the three will be used. */
		if (magr && attk && !weapon)
			slot = attk_equip_slot(magr, attk->aatyp);
		else
			slot = 0L;
		switch (slot)
		{
		case W_ARMG:
			otmp = (youagr ? uarmg : which_armor(magr, slot));
			plural = TRUE;
			break;
		case W_ARMF:
			otmp = (youagr ? uarmf : which_armor(magr, slot));
			plural = TRUE;
			break;
		case W_ARMH:
			otmp = (youagr ? uarmh : which_armor(magr, slot));
			break;
		default:
			otmp = (struct obj *)0;
			break;
		}
		if (otmp && (active_slots & slot)) {
			searcount++;
			if(plural)
				searcount++;
			obuf = xname(otmp);
			if (active_slots & (W_SKIN|W_WEP|rslot))
				Strcat(buf, " and ");

			if (holyobj & slot)
				Strcat(buf,
				(otmp->known && (check_oprop(otmp, OPROP_HOLYW) || check_oprop(otmp, OPROP_HOLY))) ? "holy " : 
				(otmp->known && check_oprop(otmp, OPROP_LESSER_HOLYW)) ? "consecrated " : "blessed "
				);
			if (unholyobj & slot)
				Strcat(buf,
				(otmp->known && (check_oprop(otmp, OPROP_UNHYW) || check_oprop(otmp, OPROP_UNHY))) ? "unholy " : 
				(otmp->known && check_oprop(otmp, OPROP_LESSER_UNHYW)) ? "desecrated " : "cursed "
				);
			if (unblessedobj & slot)
				Strcat(buf,
				(otmp->known && (check_oprop(otmp, OPROP_CONCW) || check_oprop(otmp, OPROP_CONC))) ? "concordant " : 
				(otmp->known && check_oprop(otmp, OPROP_LESSER_CONCW)) ? "accordant " : "uncursed "
				);
			if (silverobj & slot){
				if (!strstri(obuf, "silver")){
					if(otmp->obj_material != SILVER && arti_silvered(otmp))
						add_silvered_art_sear_adjectives(buf, otmp);
					else Strcat(buf, (otmp->obj_material != SILVER ? "silvered " : "silver "));
				}
			}
			if (jadeobj & slot) {
				if (!strstri(obuf, "jade "))
					Strcat(buf, "jade ");
			}
			if (ironobj & slot) {
				if (!strncmpi(obuf, "iron ", 5))
					Strcat(buf, "cold-");
				else if (!strstri(obuf, "iron "))
					Strcat(buf, "cold-iron ");
			}
			if (grnstlobj & slot){
				if (!strstri(obuf, "green-steel "))
					Strcat(buf, "green-steel ");
			}
			Strcat(buf, obuf);
		}
		/* defender */
		char seared[BUFSZ];
		if (noncorporeal(pd)) {
			Strcpy(seared, 
				youdef ? "you" : mon_nam(mdef));
		}
		else {
			Sprintf(seared, "%s %s",
				(youdef ? "your" : s_suffix(mon_nam(mdef))),
				(youdef ? body_part(BODY_FLESH) : mbodypart(mdef, BODY_FLESH))
				);
		}
		pline("%s sear%s %s!", buf, (searcount > 1) ? "" : "s", seared);
	}

	/* poison */
	if (poisons_resisted || poisons_majoreff || poisons_minoreff || poisons_wipedoff) {
		otmp = poisonedobj;
		int i, n, r;
		char poisons_str[BUFSZ];
		/* poison resist messages -- should only appear once, as resistivity should be constant between hits */
		if (poisons_resisted && (vis&VIS_MDEF) && !recursed) {
			r = 0;	/* # of poisons resisted */
			for (n = 0; n < NUM_POISONS; n++)
			{
				i = (1 << n);
				if (!(poisons_resisted & i))
					continue;
				switch (i)
				{
				case OPOISON_BASIC:
				case OPOISON_BLIND:
				case OPOISON_PARAL:
				case OPOISON_DIRE:
					Sprintf(poisons_str, "poison");
					break;
				case OPOISON_FILTH:
					Sprintf(poisons_str, "filth");
					break;
				case OPOISON_SLEEP:
					Sprintf(poisons_str, "drug");
					break;
				case OPOISON_HALLU:
					Sprintf(poisons_str, "hallucinogen");
					break;
				case OPOISON_AMNES:
					Sprintf(poisons_str, "lethe-rust");
					break;
				case OPOISON_ACID:
					Sprintf(poisons_str, "acid-coating");
					break;
				case OPOISON_SILVER:
					/* no message */
					r--;
					break;
				}
				r++;
			}
			if (r) {
				pline_The("%s %s seem to affect %s.",
					((r > 1) ? "coatings" : poisons_str),
					((r > 1) ? "don't" : "doesn't"),
					(youdef ? "you" : mon_nam(mdef))
					);
			}
		}
		/* poison major effects and their messages -- can happen multiple times */
		for (n = 0; n < NUM_POISONS; n++)
		{
			i = (1 << n);
			if (!(poisons_majoreff & i))
				continue;
			switch (i)
			{
			case OPOISON_BASIC:
				if (youdef) {
					int attrib = (
						!rn2(3) ?	A_STR :
						!rn2(2) ?	A_CON :
									A_DEX);
					int amnt = rnd(ACURR(attrib) / 5);

					if (adjattrib(attrib, -amnt, 1))
						pline_The("poison was quite debilitating...");
					IMPURITY_UP(u.uimp_poison)
				}
				else if ((vis&VIS_MDEF) && lethaldamage) {
					pline_The("poison was deadly...");
					if(youagr){
						IMPURITY_UP(u.uimp_poison)
					}
				}
				break;
			case OPOISON_DIRE:
				if (youdef) {
					int attrib = (
						!rn2(3) ?	A_STR :
						!rn2(2) ?	A_CON :
									A_DEX);
					if (!rn2(10) && attrib != A_CHA) {
						int drain = attrib == A_CON ? -2 : -rn1(3, 3);
						if(Poison_resistance)
							drain = (drain - 1)/2;
						else
							drain -= 4;

						adjattrib(A_CON, drain, 1);
						IMPURITY_UP(u.uimp_poison)
					}
					int amnt = rn1(3, 3);
					if(Poison_resistance)
						amnt = (amnt + 1)/2;
					else
						amnt += 2;

					if (adjattrib(attrib, -amnt, 1))
						pline_The("poison was quite debilitating...");
				}
				else if ((vis&VIS_MDEF) && lethaldamage){
					pline_The("poison was deadly...");
					if(youagr){
						IMPURITY_UP(u.uimp_poison)
					}
				}
				break;
			case OPOISON_FILTH:
				if (youdef) {
					/* resistance should have already been checked */
					make_sick(Sick ? Sick / 2L + 1L : (long)rn1(ACURR(A_CON), 20),
						"filth-coated weapon", TRUE, SICK_NONVOMITABLE);
				}
				else if ((vis&VIS_MDEF) && lethaldamage){
					pline_The("tainted filth was deadly...");
					if(youagr){
						IMPURITY_UP(u.uimp_dirtiness)
					}
				}
				break;
			case OPOISON_SLEEP:
				if (youdef) {
					You("suddenly fall asleep!");
					fall_asleep(-rn1(2, 6), TRUE);
				}
				else if (sleep_monst(mdef, rnd(12), POTION_CLASS)) {
					if (canseemon(mdef))
						pline("%s falls asleep.", Monnam(mdef));
					slept_monst(mdef);
				}
				break;
			case OPOISON_BLIND:
				if (youdef) {
					make_blinded(rn1(20, 25), (boolean)!Blind);
				}
				else {
					if (canseemon(mdef) && !is_blind(mdef))
						pline("It seems %s has gone blind!", mon_nam(mdef));

					register int btmp = 64 + rn2(32) +
						rn2(32) * !resist(mdef, POTION_CLASS, 0, NOTELL);
					btmp += mdef->mblinded;
					mdef->mblinded = min(btmp, 127);
					mdef->mcansee = 0;
				}
				break;
			case OPOISON_PARAL:
				if (youdef) {
					int dur = rnd(25 - rnd(ACURR(A_CON)));
					if (Poison_res(mdef))
						dur = (dur + 2) / 3;
					nomul(-dur, "immobilized by paralysis venom");
				}
				else {
					int dur = rnd(25);
					if (Poison_res(mdef))
						dur = (dur + 2) / 3;
					if (canseemon(mdef) && mdef->mcanmove)
						pline("%s stops moving!", Monnam(mdef));
					if (mdef->mcanmove) {
						mdef->mcanmove = 0;
						mdef->mfrozen = dur;
					}
				}
				break;
			case OPOISON_AMNES:
				if (youdef) {
					if (u.sealsActive&SEAL_HUGINN_MUNINN){
						unbind(SEAL_HUGINN_MUNINN, TRUE);
					}
					else {
						forget(1);	/* lose 1% of memory per point lost*/
						// forget_traps();		/* lose memory of all traps*/
					}
				}
				else {
					if (canseemon(mdef) && (mdef->mtame || !mdef->mpeaceful))
						pline("%s looks around as if awakening from a dream.", Monnam(mdef));
					mdef->mamnesia = TRUE;
				}
				break;
			case OPOISON_HALLU:
				if (youdef) {
					boolean oldh = Hallucination;
					boolean olds = Stunned;
					make_hallucinated(itimeout_incr(HHallucination, 200), FALSE, 0L);
					make_stunned(itimeout_incr(HStun, 200), FALSE);
					if(Hallucination)
						You("are freaked out!");
					if(Stunned)
						You("stagger.");

				}
				else {
					if (canseemon(mdef))
						pline("%s looks freaked out!", Monnam(mdef));
					mdef->mberserk = TRUE;
					mdef->mconf = TRUE;
				}
				break;
			case OPOISON_ACID:
			case OPOISON_SILVER:
				/* no message, no additional effects */
				break;
			}
		}
		/* ophidiophobia -- fear of snakes (and also poison, in dnethack) */
		if (youdef && !recursed && roll_madness(MAD_OPHIDIOPHOBIA)) {
			if (poisons_majoreff || poisons_minoreff) {
				You("panic!");
				HPanicking += 1+rnd(6);
			}
			else if (poisons_resisted) {
				You("panic anyway!");
				HPanicking += 1+rnd(3);
			}
		}

		/* poisons wiped off */
		if (otmp && poisons_wipedoff) {
			/* rings subtract from corpsenm */
			if (otmp->oclass == RING_CLASS) {
				if (--otmp->opoisonchrgs <= 0)
					otmp->opoisoned = OPOISON_NONE;
			}
			/* viperwhips also subtract from corpsenm, but do so with a message */
			else if (otmp->otyp == VIPERWHIP && otmp->opoisonchrgs) {
				if (youagr) {
					pline("Poison from the internal reservoir coats the fangs of your %s.", xname(otmp));
				}
				otmp->opoisonchrgs--;
			}
			/* normal poisoned object behaviour */
			else {
				if (otmp->quan > 1){
					struct obj *unpoisd = splitobj(otmp, 1L);
					unpoisd->opoisoned &= ~(poisons_wipedoff);
					if (youagr) {
						pline("The coating on your %s has worn off.", xname(unpoisd));
						obj_extract_self(unpoisd);	/* free from inv */
						/* shouldn't merge */
						unpoisd = hold_another_object(unpoisd, "You drop %s!",
							doname(unpoisd), (const char *)0);
					}
					else {
						obj_extract_self(unpoisd);
						mpickobj(magr, unpoisd);
					}
				}
				else {
					if (youagr) {
						pline("The coating on your %s has worn off.", xname(otmp));
					}
					otmp->opoisoned &= ~(poisons_wipedoff);
				}
			}
		}
	}

	if(misotheistic_major && !lethaldamage){
		if(youdef){
			You("are stunned by loss and doubt!");
		}
		else if(canseemon(mdef) && humanoid_upperbody(mdef->data)){
			pline("%s looks stunned by doubt and grief!", Monnam(mdef));
		}
	}
	/* silently handle destroyed weapons */
	if (destroy_one_magr_weapon || destroy_all_magr_weapon) {
		boolean deallocweapon = (weapon->quan == 1L || destroy_all_magr_weapon );
		Strcpy(buf, destroy_all_magr_weapon ? xname(weapon) : makesingular(xname(weapon)));
		switch(weapon->where)
		{
			case OBJ_FREE:
				obfree(weapon, (struct obj *)0);
				break;
			case OBJ_INVENT:
				if (!youagr) {
					Your("%s %s!", buf, vtense(buf, "vanish"));
				}
				if (deallocweapon) {
					if (weapon == uwep)
						uwepgone();
					else if (weapon == uswapwep)
						uswapwepgone();
				}
				if (destroy_all_magr_weapon)
					useupall(weapon);
				else
					useup(weapon);
				break;
			case OBJ_MINVENT:
				if (magr != weapon->ocarry) {
					pline("%s %s %s!", s_suffix(Monnam(weapon->ocarry)), buf, vtense(buf, "vanish"));
				}
				if (destroy_all_magr_weapon)
				{
					m_freeinv(weapon);
					obfree(weapon, (struct obj *)0);
				}
				else
					m_useup(magr, weapon);
				break;
			default:
				impossible("useup weapon from where? (%d)", weapon->where);
				break;
		}
		if (deallocweapon) {
			weapon = (struct obj *)0;
			*weapon_p = NULL;
		}
	}

	/* Use the mercy blade */
	/* this can print a message, can possibly kill monster, returning immediately */
	if(mercy_blade){
		if(Insight >= 50 && (youdef || lethaldamage || !resist(mdef, youagr ? SPBOOK_CLASS : WEAPON_CLASS, 0, TRUE))){
			mercy_blade_conflict(mdef, magr, wepspe, lethaldamage);
		}
		//Might have died in mvm combat, for example, attacking a cockatrice.
		if(DEADMONSTER(mdef))
			return MM_DEF_DIED;
		//Don't think this can happen, but better safe than sorry.
		if(MIGRATINGMONSTER(mdef))
			return MM_AGR_STOP;
	}
	if(mind_blade && (lethaldamage || !resist(mdef, WEAPON_CLASS, 0, TRUE))){
		mind_blade_conflict(mdef, magr, wepspe, lethaldamage);
	}
	/* Deal Damage */
	/* this can possibly kill, returning immediately */
	result = xdamagey(magr, mdef, attk, totldmg);
	if (result&(MM_DEF_DIED|MM_DEF_LSVD))
		return result;

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	/* ALL EFFECTS AFTER THIS POINT REQUIRE THE DEFENDER TO SURVIVE THE ATTACK (not including lifesaving) */
	////////////////////////////////////////////////////////////////////////////////////////////////////////

	/* apply confuse monster (player only) */
	if (youagr && u.umconf && !mdef->mconf) {
		char *hands = makeplural(body_part(HAND));
		/* message, reduce your hand glowiness */
		if (u.umconf == 1) {
			if (Blind)
				Your("%s stop tingling.", hands);
			else
				Your("%s stop glowing %s.", hands, hcolor(NH_RED));
		}
		else {
			if (Blind)
				pline_The("tingling in your %s lessens.", hands);
			else
				Your("%s no longer glow so brightly %s.", hands,
				hcolor(NH_RED));
		}
		u.umconf--;
		/* apply the confusion */
		if (!resist(mdef, SPBOOK_CLASS, 0, NOTELL)) {
			mdef->mconf = 1;
			if (!mdef->mstun && mdef->mcanmove && !mdef->msleeping && canseemon(mdef))
				pline("%s appears confused.", Monnam(mdef));
		}
	}

	/* hurtle mdef */
	if (staggering_strike || jousting 
		|| (fired && weapon && is_boulder(weapon))
		|| pd->mtyp == PM_KHAAMNUN_TANNIN
		|| has_template(mdef, TONGUE_PUPPET)
	){
		int dx, dy;
		int ix = x(mdef), iy = y(mdef);
		/* in what direction? */
		if (magr) {
			dx = sgn(x(mdef)-x(magr));
			dy = sgn(y(mdef)-y(magr));
		}
		else if (fired && weapon && is_boulder(weapon)) {
			/* assumes that the boulder's ox/oy are accurate to where it started moving from */
			dx = sgn(x(mdef)-weapon->ox);
			dy = sgn(y(mdef)-weapon->oy);
		}
		else if(pd->mtyp == PM_KHAAMNUN_TANNIN || has_template(mdef, TONGUE_PUPPET)){
			//Evade in a random direction
			do{
				dx = rn2(3)-1;
				dy = rn2(3)-1;
			} while(!(dx || dy));
		}
		else {
			impossible("hurtle with no direction");
			dx = dy = 0;
		}
		boolean evading = (pd->mtyp == PM_KHAAMNUN_TANNIN || has_template(mdef, TONGUE_PUPPET)) && (ix != x(mdef) || iy != y(mdef));

		/* boulders can knock to the side as well -- 2/3 chance to move out of the way, 1/3 to go straight back and be struck again*/
		if (fired && weapon && is_boulder(weapon)) {
			int tx = dx, ty = dy;
			dx = (tx&&!ty) ? tx : (!tx&&ty) ? rn2(3)-1 : tx*(!rn2(3));
			dy = (ty&&!tx) ? ty : (!ty&&tx) ? rn2(3)-1 : ty*(!rn2(3));
		}

		if (youdef) {
			hurtle(dx, dy, evading ? 4 : 1, FALSE, FALSE);
			if(evading)
				You("%s away.", (pd->mtyp == PM_KHAAMNUN_TANNIN) ? "jet" : "swing");
			if (staggering_strike)
				make_stunned(HStun + rnd(10), TRUE);
			nomul(0, "being knocked back");
		}
		else {
			mhurtle(mdef, dx, dy, evading ? 4 : 1, TRUE);
			if(evading)
				pline("%s %s away.", Monnam(mdef), (pd->mtyp == PM_KHAAMNUN_TANNIN) ? "jets" : "swings");
			if (DEADMONSTER(mdef))
				return MM_DEF_DIED;
			if(MIGRATINGMONSTER(mdef))
				return MM_AGR_STOP;
			if (staggering_strike)
				mdef->mstun = TRUE;
			pd = mdef->data; /* in case of a polymorph trap */
		}
	}
	if(stunning_strike){
		if(youdef){
			make_stunned((HStun)+d(3, 3), FALSE);
		}
		else{
			mdef->mstun = TRUE;
			mdef->mconf = TRUE;
		}
	}

	/* pudding division */
	if ((pd->mtyp == PM_BLACK_PUDDING || pd->mtyp == PM_BROWN_PUDDING || pd->mtyp == PM_DARKNESS_GIVEN_HUNGER)
		&& weapon && (valid_weapon_attack || invalid_weapon_attack)
		&& is_iron_obj(weapon)
		&& !litsaber(weapon)
		&& melee && (youdef || !mdef->mcan)) {
		if (youdef) {
			if (totldmg > 1)
				exercise(A_STR, FALSE);
			if (cloneu())
				You("divide!");
		}
		else {
			if (clone_mon(mdef, 0, 0)) {
				if (vis&VIS_MDEF) {
					pline("%s divides!", Monnam(mdef));
				}
			}
		}
	}

	/* worm cutting -- assumes bhitpos is set! */
	if (isok(bhitpos.x, bhitpos.y) && m_at(bhitpos.x, bhitpos.y) == mdef) {
		cutworm(mdef, bhitpos.x, bhitpos.y, weapon);
	}

	/* uranium imp warping -- not implemented for the player defending */
	if (pd->mtyp == PM_URANIUM_IMP && !mdef->mcan && !youdef){
		/* avoid mysterious force message by not using tele_restrict() */
		if (canseemon(mdef)) {
			pline("%s %s reality!",
				Monnam(mdef),
				notel_level() ? "tries to warp" : "warps"
				);
		}
		if (!notel_level()) {
			coord cc;
			if (magr) {
				if (youagr) {
					tele();
				}
				else {
					rloc(magr, TRUE);
				}
				if (enexto(&cc, x(magr), y(magr), &mons[PM_URANIUM_IMP])) {
					rloc_to(mdef, cc.x, cc.y);
				}
			}
			else {
				/* if no attacker, the uranium imp teleports at random */
				rloc(mdef, TRUE);
			}
			
			return MM_AGR_STOP;
		}
	}

	return result;
}

void
add_silvered_art_sear_adjectives(buf, otmp)
char *buf;
struct obj *otmp;
{
	switch(otmp->oartifact){
		default:
			if (!strstri(xname(otmp), "silver"))
				Strcat(buf, "silvered ");
		break;
		case ART_NODENSFORK:
			Strcat(buf, "silver-waved ");
		break;
		case ART_PEACE_KEEPER:
			Strcat(buf, "minute-silver-runed ");
		break;
		case ART_SKY_RENDER:
			Strcat(buf, "half-silver ");
		break;
		case ART_WATER_FLOWERS:
			Strcat(buf, "silver-flowered ");
		break;
		case ART_GAUNTLETS_OF_SPELL_POWER:
			Strcat(buf, "silver-runed ");
		break;
		case ART_LOLTH_S_FANG:
			Strcat(buf, "silver-edged ");
		break;
		case ART_WEB_OF_LOLTH:
			Strcat(buf, "silver-starred ");
		break;
		case ART_RUINOUS_DESCENT_OF_STARS:
			Strcat(buf, "silver-spiked ");
		break;
		case ART_STAFF_OF_AESCULAPIUS:
			Strcat(buf, "silver-snaked ");
		break;
		case ART_DURIN_S_AXE:
			Strcat(buf, "silver-inlaid ");
		break;
		case ART_WEB_OF_THE_CHOSEN:
			Strcat(buf, "silver-dewed ");
		break;
		case ART_YENDORIAN_EXPRESS_CARD:
			Strcat(buf, "silver-writing-embossed ");
		break;
		case ART_UNBLEMISHED_SOUL:
			Strcat(buf, "silver-spattered ");
		break;
		case ART_WRATHFUL_WIND:
			Strcat(buf, "silver-clouded ");
		break;
	}
}

/* shadow_strike()
 *
 * The player attacks [mdef] with one AT_TUCH AD_SHDW attack.
 */
int
shadow_strike(mdef)
struct monst * mdef;
{
	static struct attack shadowblade = { AT_ESPR, AD_SHDW, 4, 8 };
	int tohitmod = 0;	/* necessary to call xmeleehity */

	if (mdef && magr_can_attack_mdef(&youmonst, mdef, x(mdef), y(mdef), FALSE)){
		return xmeleehity(&youmonst,
			mdef,
			&shadowblade,
			(struct obj **)0,
			((canseemon(mdef) ? VIS_MDEF : 0) | VIS_MAGR | VIS_NONE),
			tohitmod,
			FALSE);
	}
	else
		return MM_MISS;
}

/* weave_black_web()
 * 
 * The player makes a shadowblade attack against all adjacent enemies, except for mexclude
 */
void
weave_black_web(mexclude)
struct monst * mexclude;
{
	struct monst *mdef;	/* defender */
	int i;				/* loop counter */

	for (i = 0; i<8; i++){
		if (isok(u.ux + xdir[i], u.uy + ydir[i])){
			mdef = m_at(u.ux + xdir[i], u.uy + ydir[i]);
			if (mdef && !mdef->mpeaceful && mdef != mexclude
				&& magr_can_attack_mdef(&youmonst, mdef, u.ux + xdir[i], u.uy + ydir[i], FALSE)){
				(void)shadow_strike(mdef);
			}
		}
	}
}

/* xpassivey()
 * 
 * [mdef] was hit by [magr] and is now counterattacking.
 * This may seem backwards in this function, but it keeps [magr] and [mdef] consistent
 *
 * Does not include AT_BOOM effects, which are handled in mon.c for monsters.
 * SCOPECREEP: handle AT_BOOM effects for players somewhere/somehow.
 * 
 * Call with [attk] and [weapon] (if applicable) with [endofchain]=FALSE for each indiviudal attack
 * and then once [attk]=NULL and [endofchain]=TRUE, if there are multiple attacks being made at once
 * 
 * Call with [attk] and [endofchain=TRUE] if there is a single attack being made and retaliated against
 */
int
xpassivey(magr, mdef, attk, weapon, vis, result, pd, endofchain)
struct monst * magr;		/* original attacker, being affected by the passive */
struct monst * mdef;		/* original defender, whose passive is being proced */
struct attack * attk;		/* attacker's attack */
struct obj * weapon;		/* attacker's weapon */
int vis;					/* visiblity of original attack */
int result;					/* if attack hit / def died / agr died / agr-def moved */
struct permonst * pd;		/* defender's pd; cannot use mdef->data when player rehumanizes */
boolean endofchain;			/* if the attacker has finished their attack chain */
{
	int i;					/* loop counter */
	int newres;
	int dmg;
	int indexnum = 0;
	int subout[SUBOUT_ARRAY_SIZE] = {0};
	int tohitmod = 0;
	int res[4];
	long slot = 0L;
	struct obj * otmp;
	boolean usedmask = FALSE;		/* whether a message has been printed about a lillend using a mask to make passive attacks */
	struct attack * passive;
	struct attack alt_attk;
	struct attack prev_attk = noattack;
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	/* set permonst pointers */
	struct permonst * pa = youagr ? youracedata : magr->data;

	/* Handle contact impurity */
	if(magr && youdef && !youagr && endofchain){
		if(has_template(magr, ZOMBIFIED) || has_template(magr, SKELIFIED)){
			IMPURITY_UP(u.uimp_bodies)
		}
		if(magr->mtyp == PM_DEEP_ONE
		|| magr->mtyp == PM_DEEPER_ONE
		|| (rn2(2) && magr->mtyp == PM_DEEPEST_ONE)
		/*NOT Dagon and Hydra*/
		){
			IMPURITY_UP(u.uimp_deep_one)
		}
		if(magr->mtyp == PM_KUO_TOA
		|| magr->mtyp == PM_KUO_TOA_WHIP
		){
			IMPURITY_UP(u.uimp_kuo_toa)
		}
		if(magr->mtyp == PM_BEING_OF_IB
		|| magr->mtyp == PM_PRIEST_OF_IB
		){
			IMPURITY_UP(u.uimp_ibite)
		}
		if(is_mind_flayer(magr->data)){
			IMPURITY_UP(u.uimp_mind_flayers)
		}
		if(magr->mtyp == PM_BEFOULED_WRAITH || magr->mtraitor || magr->mferal){
			IMPURITY_UP(u.uimp_betrayal)
		}
	}
	/* check that magr is still alive */
	if (DEADMONSTER(magr))
		return result;

	if (vis == -1)
		vis = getvis(magr, mdef, 0, 0);

	/* Lillends can use masks to counterattack (but only at the end of the chain) */
	if (!youdef && pd->mtyp == PM_LILLEND && !(has_template(mdef, ZOMBIFIED) || has_template(mdef, SKELIFIED)) && rn2(2) && endofchain){
		pd = find_mask(mdef);
		/* message moved to AFTER getting passive attack, to avoid printing a useless message 
		 * like "The lillend uses a rothe mask!" */
	}

	/* SIMILAR STRUCTURE TO XATTACKY */

	/* zero out res[] */
	res[0] = MM_MISS;
	res[1] = MM_MISS;
	res[2] = MM_MISS;
	res[3] = MM_MISS;
	do {
		/* cycle res[] -- it should be the results of the previous 3 attacks */
		res[3] = res[2];
		res[2] = res[1];
		res[1] = res[0];
		res[0] = MM_MISS;
		/* get next attack */
		passive = getattk(mdef, magr, res, &indexnum, &prev_attk, FALSE, subout, &tohitmod);
		/* if we don't have a passive attack, continue */
		if (is_null_attk(passive) || passive->aatyp != AT_NONE)
			continue;

		/* Only make noise about the lillend mask if applicable */
		if (!youdef && mdef->mtyp == PM_LILLEND && passive != &noattack && pd != mdef->data && !Blind && canseemon(mdef) &&
			!usedmask) {
			usedmask = TRUE;
			pline("%s uses a %s mask!", Monnam(mdef), pd->mname);
		}
		/* do the passive */
		res[0] = xpassivehity(magr, mdef, passive, attk, weapon, vis, result, pd, endofchain);
		result |= res[0];

	} while (!(
		(res[0] & MM_DEF_DIED) ||	/* attacker died */
		(res[0] & MM_DEF_LSVD) ||	/* attacker lifesaved */
		(res[0] & MM_AGR_DIED) ||	/* defender died */
		(res[0] & MM_AGR_STOP) ||	/* defender stopped for some other reason */
		is_null_attk(passive)		/* no more attacks */
		));
	
	/* passives NOT from a creature's attacks */
	/* per-attack */
	/* no-contact attacks are excluded from this even when they _maybe_ shouldn't be. counters yes, iris no? */
	if (attk && !no_contact_attk(attk)) {
		/* Iris unbinds on attacking a reflective creature */
		if (youagr && u.sealsActive&SEAL_IRIS &&
			!(result&MM_DEF_DIED) &&
			!Blind && canseemon(mdef) && !Invisible && !is_vampire(pa)
			&& mon_reflects(mdef, "You catch a glimpse of a stranger's reflection in %s %s.")
			){
			unbind(SEAL_IRIS, TRUE);
			result |= MM_AGR_STOP;	/* your attack was interrupted! */
		}
		/* Argent Sheen madness can make you stop at reflecting monsters */
		else if (youagr && !Blind && canseemon(mdef) && !Invisible
			&& !is_vampire(youracedata)
			&& roll_madness(MAD_ARGENT_SHEEN)
			&& mon_reflects(mdef, "You admire your reflection in %s %s.")
			){
			nomul(-1 * rnd(6), "posing in front of a mirror.");
		}
		/* Vampires biting a deadly creature die. */
		if ((result&MM_HIT) && attk->aatyp == AT_BITE && is_vampire(pa)) {
			if (youagr) {
				if (bite_monster(mdef))
					result |= MM_AGR_STOP;	/* attacker lifesaved */
			}
			else {
				if (is_deadly(pd)) {
					/* kill */
					if (youagr)
						pline("Draining from %s is instantly fatal.", mon_nam(mdef));
					char buf[BUFSZ];
					Sprintf(buf, "unwisely drained %s", pd->mname);
					killer = buf;
					killer_format = NO_KILLER_PREFIX;
					struct attack na = noattack;
					newres = xdamagey(mdef, magr, &na, 9999);
					if (newres&MM_DEF_DIED)
						result |= MM_AGR_DIED;	/* attacker died */
					if (newres&MM_DEF_LSVD)
						result |= MM_AGR_STOP;	/* attacker lifesaved */
				}
				else if (pd->mtyp == PM_GREEN_SLIME || pd->mtyp == PM_FLUX_SLIME) {
					/* slime it */
					if (!Slime_res(magr)) {
						if (youdef) {
							Your("%s turns %s into slime.", body_part(BODY_FLESH), mon_nam(mdef));
							(void)newcham(mdef, PM_GREEN_SLIME, FALSE, FALSE);
						}
						else {
							monslime(mdef);
							mdef->mstrategy &= ~STRAT_WAITFORU;
						}
					}
				}
			}
		}

		/* lightsaber form counterattack (per-attack) (player-only) */
		if (magr_can_attack_mdef(mdef, magr, x(magr), y(magr), FALSE)) {
			if (youdef && uwep && is_lightsaber(uwep) && litsaber(uwep) &&			/* player with a lightsaber in their main hand */
				(multi >= 0) &&														/* not paralyzed */
				distmin(x(magr), y(magr), x(mdef), y(mdef)) == 1 &&					/* in close quarters */
				!(result&MM_AGR_DIED) &&											/* attacker is still alive */
				(activeFightingForm(FFORM_DJEM_SO) || activeFightingForm(FFORM_SORESU))	/* proper forms */
				){
				int chance = 0;

				/* determine chance of counterattacking */
				if(activeFightingForm(FFORM_DJEM_SO)){
					if (!uarm || is_light_armor(uarm) || is_medium_armor(uarm))
						chance = DjemSo_counterattack[(min(P_SKILL(P_DJEM_SO), P_SKILL(weapon_type(uwep))) - 2)];
				}
				if(activeFightingForm(FFORM_SORESU)){
					if (!uarm || is_light_armor(uarm) || is_medium_armor(uarm))
						chance = Soresu_counterattack[(min(P_SKILL(P_SORESU), P_SKILL(weapon_type(uwep))) - 2)];
				}

				/* maybe make the counterattack */
				if (rn2(100) < chance) {
					int newvis = vis&VIS_NONE;
					if (vis&VIS_MAGR)
						newvis |= VIS_MDEF;
					if (vis&VIS_MDEF)
						newvis |= VIS_MAGR;

					You("counterattack!");
					if(activeFightingForm(FFORM_DJEM_SO))
						use_skill(P_DJEM_SO, 1);
					if(activeFightingForm(FFORM_SORESU))
						use_skill(P_SORESU, 1);

					newres = xmeleehity(mdef, magr, &basicattack, &uwep, newvis, 0, FALSE);
					if (newres&MM_DEF_DIED)
						result |= MM_AGR_DIED;	/* attacker died */
					if (newres&MM_DEF_LSVD)
						result |= MM_AGR_STOP;	/* attacker lifesaved */
				}
			}
		}
	}
	/* per round -- note cannot access attk or weapon */
	else {
		/* Mirrorbright has a limited nightmare effect */
		struct obj *shield = youdef ? uarms : which_armor(mdef, W_ARMS);
		//Technically a monster can attack itself, don't proc if so.
		if(shield && shield->oartifact == ART_MIRRORBRIGHT && magr != mdef){
			if(youagr){
				if(canseemon(mdef)){
					pline("The bright-copper shield shines in your %s!", makeplural(body_part(EYE)));
					make_confused(HConfusion + d(3, 3), FALSE);
					change_usanity(-1, TRUE);
				}
			}
			else if(youdef){
				if(mon_can_see_you(magr) && !mindless_mon(magr) && !magr->mtame && !is_render(magr->mtyp)){
					you_inflict_madness(magr);
					magr->mconf = TRUE;
				}
			}
			else {
				if(mon_can_see_mon(magr, mdef) && !mindless_mon(magr) && !magr->mtame && !is_render(magr->mtyp)){
					magr->mconf = TRUE;
				}
			}
		}
		/* Gauntlets of the Divine Disciple's divine retribution -- player only */
		if (youdef && uarmg && uarmg->oartifact == ART_GAUNTLETS_OF_THE_DIVINE_DI && !rn2(4)){
			if (!Shock_res(magr)) {
				pline("A bolt of divine energy strikes %s from the heavens!", mon_nam(magr));
				killer = "bolt of divine energy";
				killer_format = KILLED_BY_AN;
				newres = xdamagey(mdef, magr, passive, d(2, 12));
				if (newres&MM_DEF_DIED)
					result |= MM_AGR_DIED;	/* attacker died */
				if (newres&MM_DEF_LSVD)
					result |= MM_AGR_STOP;	/* attacker lifesaved */
			}
		}
		/* Counterattacks */
		if (magr_can_attack_mdef(mdef, magr, x(magr), y(magr), FALSE))
		{
			/* get defender's weapon */
			otmp = (youdef ? uwep : MON_WEP(mdef));
			if (otmp &&
				!(result&(MM_AGR_DIED | MM_DEF_DIED | MM_DEF_LSVD)) &&
				distmin(x(magr), y(magr), x(mdef), y(mdef)) == 1 &&
				!cantmove(mdef)) {
				int chance = 0;
				/* lightsaber forms (per-round) (player-only) */
				if (youdef &&
					is_lightsaber(otmp) && litsaber(otmp) &&
					activeFightingForm(FFORM_SHIEN))
				{
					chance += Shien_counterattack[(min(P_SKILL(P_SHIEN), P_SKILL(weapon_type(uwep))) - 2)];
				}
				/* Sansara Mirror */
				if (otmp->oartifact == ART_SANSARA_MIRROR) {
					chance += 50;
				}
				/* Pen of the Void */
				if (otmp->oartifact == ART_PEN_OF_THE_VOID &&
					otmp->ovara_seals&SEAL_EURYNOME) {
					chance += 10;
					if (quest_status.killed_nemesis && Role_if(PM_EXILE))
						chance += 10;
				}

				/* maybe make the counterattack */
				if (rn2(100) < chance) {
					struct attack * counter;
					int newvis = vis&VIS_NONE;
					if (vis&VIS_MAGR)
						newvis |= VIS_MDEF;
					if (vis&VIS_MDEF)
						newvis |= VIS_MAGR;

					/* get 1st weapon attack defender has */
					int subout2[SUBOUT_ARRAY_SIZE] = {0};
					int indexnum2 = 0;
					/* grab the first weapon attack mdef has, or else use a basic 1d4 attack */
					do {
						/* we'll ignore res[], tohitmod, and prev_attk, resusing them from earlier */
						counter = getattk(mdef, magr, res, &indexnum2, &prev_attk, FALSE, subout2, &tohitmod);
					} while (counter->aatyp != AT_WEAP && !is_null_attk(counter));
					if (is_null_attk(counter))
						counter = &basicattack;

					/* make the counterattack */
					if (youdef) {
						You("counterattack!");
					}
					else if (newvis&VIS_MAGR) {
						pline("%s counterattacks!", Monnam(mdef));
					}
					/* train lightsaber skill if applicable */
					if (youdef && activeFightingForm(FFORM_SHIEN))
						use_skill(P_SHIEN, 1);

					/* make the attack */
					newres = xmeleehity(mdef, magr, counter, &otmp, newvis, 0, FALSE);
					if (newres&MM_DEF_DIED)
						result |= MM_AGR_DIED;	/* attacker died */
					if (newres&MM_DEF_LSVD)
						result |= MM_AGR_STOP;	/* attacker lifesaved */
				}
			}

			/* Eurynome (player-only) */
			if (youdef && u.sealsActive&SEAL_EURYNOME &&							/* player with Eurynome bound */
				(multi >= 0) &&														/* not paralyzed */
				distmin(x(magr), y(magr), x(mdef), y(mdef)) == 1 &&					/* in close quarters */
				!(result&MM_AGR_DIED)												/* attacker is still alive */
				){
				/* chance of counterattacking is 20% */
				/* maybe make the counterattack */
				if (rn2(100) < 20) {
					int newvis = vis&VIS_NONE;
					if (vis&VIS_MAGR)
						newvis |= VIS_MDEF;
					if (vis&VIS_MDEF)
						newvis |= VIS_MAGR;
					int i;

					You("counterattack!");

					/* counterattack with two unarmed strikes, regardless of free-hand-ed-ness */
					for (i = 0; i < 2; i++) {
						newres = xmeleehity(mdef, magr, &basicattack, (struct obj **)0, newvis, 0, FALSE);
						if (newres&MM_DEF_DIED)
							result |= MM_AGR_DIED;	/* attacker died */
						if (newres&MM_DEF_LSVD)
							result |= MM_AGR_STOP;	/* attacker lifesaved */
						/* if (original attacker) died, don't keep attacking */
						if (result&(MM_AGR_DIED | MM_AGR_STOP))
							break;
					}
				}
			}
		}
	}
	return result;
}


int
xpassivehity(magr, mdef, passive, attk, weapon, vis, result, pd, endofchain)
struct monst * magr;		/* original attacker, being affected by the passive */
struct monst * mdef;		/* original defender, whose passive is being proced */
struct attack * passive;	/* defender's passive being used */
struct attack * attk;		/* attacker's attack */
struct obj * weapon;		/* attacker's weapon */
int vis;					/* visiblity of original attack */
int result;					/* if attack hit / def died / agr died / agr-def moved */
struct permonst * pd;		/* defender's pd; cannot use mdef->data when player rehumanizes */
boolean endofchain;			/* if the passive is occuring at the end of aggressor's attack chain */
{
	int newres;
	int dmg;
	long slot = 0L;
	struct monst * mtmp = (struct monst *)0;
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	/* set permonst pointers */
	struct permonst * pa = youagr ? youracedata : magr->data;
	int maketame = ((youdef || mdef->mtame) ? MM_EDOG : 0);


	/* Get damage of passive */
	/* Note: not always used */
	if (passive->damn)
		dmg = d(passive->damn, passive->damd);
	else if (passive->damd)
		dmg = d(mlev(mdef)/3 + 1, passive->damd);
	else
		dmg = 0;

	/* These passives specifically happen per attack. */
	/* necessary for passives that interact with [attk] */
	/* Try not to have passives deal damage like this, because that adds up to a lot quickly. */
	if (attk) {
		/* passives from statblock */
		if (passive != &noattack && result&MM_HIT) {
			switch (passive->adtyp)
			{
			case AD_STON:
				slot = attk_protection(attk->aatyp);
				/* Touching is fatal */
				if (touch_petrifies(pd) && !(Stone_res(magr))
					&& badtouch(magr, mdef, attk, weapon))
				{
					if (youagr) {
						/* don't call xstoney, we want instant-stoning for the player */
						if (poly_when_stoned(youracedata) && polymon(PM_STONE_GOLEM)) {
							/* polyd into a stone golem */
							result |= MM_AGR_STOP;
						}
						else {
							/* stoned */
							You("turn to stone...");
							done_in_by(mdef);
							result |= MM_AGR_DIED;
						}
					}
					else {
						result |= xstoney(mdef, magr);
					}
				}
				break;
			case AD_SLVR:
				/* Eden's silver body sears attackers */
				slot = attk_protection(attk->aatyp);
				if (hates_silver(pa)
					&& badtouch(magr, mdef, attk, weapon))
				{
					if (youdef) {
						Your("silver body sears %s!", mon_nam(magr));
					}
					else if (youagr || canseemon(magr)) {
						pline("%s %s seared by %s silver body!",
							(youagr ? "You" : Monnam(magr)),
							(youagr ? "are" : "is"),
							s_suffix(mon_nam(mdef))
							);
					}
					newres = xdamagey(mdef, magr, passive, dmg);
					if (newres&MM_DEF_DIED)
						result |= MM_AGR_DIED;	/* attacker died */
					if (newres&MM_DEF_LSVD)
						result |= MM_AGR_STOP;	/* attacker lifesaved */
				}
				break;
			case AD_RUST:
				if (youdef || !mdef->mcan) {
					/* naiads have additional limitations */
					if (pd->mtyp == PM_NAIAD){
						if (((*hp(mdef) < *hpmax(mdef) / 2) && rn2(3)) ||
							result&MM_DEF_DIED) {
							if (youdef) {
								You("collapse into a puddle of water!");
								rehumanize();
								change_gevurah(1); //cheated death.
							}
							else {
								if (canseemon(mdef))
									pline("%s collapses into a puddle of water!", Monnam(mdef));
								if (!(result&MM_DEF_DIED)) {
									newres = xdamagey(magr, mdef, passive, *hp(mdef));
									if (newres&MM_DEF_DIED)
										result |= MM_DEF_DIED;	/* defender died */
									if (newres&MM_DEF_LSVD)
										result |= MM_AGR_STOP;	/* defender lifesaved */
								}
							}
							/* fall through, rusting weapon */
						}
						/* if the naiad didn't collapse into a puddle, no rusting happens */
						else break;
					}
					/* rust weapon */
					passive_obj2(magr, mdef, weapon, attk, passive);
				}
				break;

			case AD_CORR:
				if (youdef || !mdef->mcan) {
					/* corrode weapon */
					passive_obj2(magr, mdef, weapon, attk, passive);
				}
				break;

			case AD_ENCH:
				if (youdef || !mdef->mcan) {
					/* disenchant weapon */
					passive_obj2(magr, mdef, weapon, attk, passive);
				}
				break;
			case AD_EACD:
			case AD_ACID:
				/* possibly corrode weapon -- this is independent of the attacker itself being splashed with acid */
				if (!rn2(passive->adtyp == AD_EACD ? 2 : 6)) {
					/* call dedicated passive-affects-object function */
					passive_obj2(magr, mdef, weapon, attk, passive);
				}
				break;
			}
		}
	}
	/* these passives happen at the end of the attack round */
	/* cannot access attk or weapon */
	else {
		/* These passives happen, hit or miss, even if the defender died. */
		if (passive != &noattack) {
			switch (passive->adtyp)
			{
			case AD_MROT:
				// Usually does nothing, this effect is handled in the experience gain functions xkilled and grow_up.
				if(!DEADMONSTER(mdef) && mdef->mtyp == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH){
					//This function's defender is the one doing the cursing, so they are passed in as the agressor.
					result |= mummy_curses_x(mdef, magr);
				}
			break;
			case AD_MAGM:
				/* wrath of gods for attacking Oracle */
				if (Magic_res(magr)) {
					dmg = 0;
					if (youagr) {
						shieldeff(u.ux, u.uy);
						pline("A hail of magic missiles narrowly misses you!");
					}
					else if (canseemon(magr)) {
						shieldeff(x(magr), y(magr));
						pline("%s is missed by a hail of magic missiles.", Monnam(magr));
					}
				}
				else {
					if (youagr) {
						You("are hit by magic missiles appearing from thin air!");
					}
					else if (canseemon(magr)) {
						pline("%s is hit by a hail of magic missiles.", Monnam(magr));
					}
					newres = xdamagey(mdef, magr, passive, dmg);
					if (newres&MM_DEF_DIED)
						result |= MM_AGR_DIED;	/* attacker died */
					if (newres&MM_DEF_LSVD)
						result |= MM_AGR_STOP;	/* attacker lifesaved */
				}
				break;

			case AD_SHDW:
				if (youagr) {
					pline("Its bladed shadow falls on you!");
				}
				else if (youdef) {
					pline("Your bladed shadow falls on %s!", mon_nam(magr));
				}
				else if (vis) {
					pline("%s's bladed shadow falls on %s!", Monnam(mdef), mon_nam(magr));
				}
				newres = xdamagey(mdef, magr, passive, dmg);
				if (newres&MM_DEF_DIED)
					result |= MM_AGR_DIED;	/* attacker died */
				if (newres&MM_DEF_LSVD)
					result |= MM_AGR_STOP;	/* attacker lifesaved */

				if (!rn2(8) && !(result&(MM_AGR_DIED | MM_AGR_STOP)))
				{
					if (youagr) {
						char buf[BUFSZ];
						Sprintf(buf, "%s shadow", s_suffix(Monnam(mdef)));
						poisoned(buf, A_STR, pd->mname, 30, FALSE);
					}
					else {
						if (!Poison_res(magr)) {
							newres = xdamagey(mdef, magr, passive, rn1(10, 6));
							if (newres&MM_DEF_DIED)
								result |= MM_AGR_DIED;	/* attacker died */
							if (newres&MM_DEF_LSVD)
								result |= MM_AGR_STOP;	/* attacker lifesaved */
						}
					}
				}
				break;
			}
		}

		/* These passives happen even if the defender died, but require a hit */
		if ((result&MM_HIT) && passive != &noattack) {
			switch (passive->adtyp)
			{
			case AD_BARB:
				/* message */
				if (youagr) {
					if (pd->mtyp == PM_RAZORVINE) You("are hit by the springing vines!");
					else You("are hit by %s barbs!", s_suffix(mon_nam(mdef)));
				}
				else if (vis) {
					if (pd->mtyp == PM_RAZORVINE) {
						pline("%s is hit by %s springing vines!",
							Monnam(magr),
							(youdef ? "your" : s_suffix(mon_nam(mdef)))
							);
					}
					else {
						pline("%s is hit by %s barbs!",
							Monnam(magr),
							(youdef ? "your" : s_suffix(mon_nam(mdef)))
							);
					}
				}
				/* damage (reduced by DR, half-phys damage, min 1) */
				dmg -= (youagr ? roll_udr(mdef, ROLL_SLOT) : roll_mdr(magr, mdef, ROLL_SLOT));
				if (dmg < 1)
					dmg = 1;
				dmg = reduce_dmg(mdef,dmg,TRUE,FALSE);
				struct attack na = noattack;
				newres = xdamagey(mdef, magr, &na, dmg);
				if (newres&MM_DEF_DIED)
					result |= MM_AGR_DIED;	/* attacker died */
				if (newres&MM_DEF_LSVD)
					result |= MM_AGR_STOP;	/* attacker lifesaved */
				break;

			case AD_EACD:
			case AD_ACID:
				/* attacker is splashed 1/2 the time */
				if (rn2(2)) {
					/* maybe reduce damage (affects messages) */
					if (Acid_res(magr))
						dmg = (passive->adtyp == AD_EACD ? dmg / 2 : 0);
					/* message */
					if (youagr) {
						if (Blind || !flags.verbose) {
							You("are splashed%s",
								exclam(dmg)
								);
						}
						else {
							You("are %ssplashed by %s acid%s",
								(dmg == 0 ? "harmlessly " : ""),
								s_suffix(mon_nam(mdef)),
								exclam(dmg)
								);
						}
					}
					else if (vis) {
						pline("%s is %ssplashed by %s acid%s",
							Monnam(magr),
							(dmg == 0 ? "harmlessly " : ""),
							(youdef ? "your" : s_suffix(mon_nam(mdef))),
							exclam(dmg)
							);
					}
					/* damage */
					if (dmg) {
						newres = xdamagey(mdef, magr, passive, dmg);
						if (newres&MM_DEF_DIED)
							result |= MM_AGR_DIED;	/* attacker died */
						if (newres&MM_DEF_LSVD)
							result |= MM_AGR_STOP;	/* attacker lifesaved */
					}
					/* potentially erode worn armor */
					if (!rn2(passive->adtyp == AD_EACD ? 6 : 30))
						erode_armor(magr, TRUE);
				}
				break;

			case AD_WEBS:
				if (!t_at(x(magr), y(magr))) {
					struct trap *ttmp2 = maketrap(x(magr), y(magr), WEB);
					if (ttmp2) {
						if (youagr) {
							pline_The("webbing sticks to you. You're caught!");
							dotrap(ttmp2, NOWEBMSG);
#ifdef STEED
							if (u.usteed && u.utrap) {
								/* you, not steed, are trapped */
								dismount_steed(DISMOUNT_FELL);
							}
#endif
						}
						else {
							mintrap(magr);
						}
					}
				}
				break;
			case AD_HLBD:
				/* Legion gets many zombies and legionnaires */
				if (pd->mtyp == PM_LEGION) {
					int n = rnd(4);
					for (; n > 0; n--) {
						mtmp = (rn2(7) ? makemon(mkclass(S_ZOMBIE, G_NOHELL | G_HELL), x(mdef), y(mdef), NO_MINVENT|MM_ADJACENTOK|MM_ADJACENTSTRICT|maketame)
							: makemon(&mons[PM_LEGIONNAIRE], x(mdef), y(mdef), NO_MINVENT|MM_ADJACENTOK|MM_ADJACENTSTRICT|maketame));
						if (mtmp) {
							/* Legion's summons don't time out */
							/* Although this currently is impossible, we should handle tame/selfpolyd Legion */
							if (maketame) {
								initedog(mtmp);
							}
						}
					}
				}
				/* Others (Asmodeus, Verier) get Devils */
				else {
					if (*hp(mdef) > *hpmax(mdef) * 3 / 4)		mtmp = makemon(&mons[PM_LEMURE], x(mdef), y(mdef), MM_ADJACENTOK|maketame);
					else if (*hp(mdef) > *hpmax(mdef) * 2 / 4)	mtmp = makemon(&mons[PM_HORNED_DEVIL], x(mdef), y(mdef), MM_ADJACENTOK|maketame);
					else if (*hp(mdef) > *hpmax(mdef) * 1 / 4)	mtmp = makemon(&mons[PM_BARBED_DEVIL], x(mdef), y(mdef), MM_ADJACENTOK|maketame);
					else if (*hp(mdef) > *hpmax(mdef) * 0 / 4)	mtmp = makemon(&mons[PM_PIT_FIEND], x(mdef), y(mdef), MM_ADJACENTOK|maketame);
					if (mtmp) {
						/* Asmodeus's and Verier's summons don't time out */
						/* Although this currently is impossible, we should handle tame/selfpolyd Asmo/Verier */
						if (maketame) {
							initedog(mtmp);
						}
					}
				}
				break;
			case AD_OONA:
				/* */
				if(!DEADMONSTER(mdef)){
					if (u.oonaenergy == AD_FIRE){
						if (rn2(2)) mtmp = makemon(&mons[PM_FLAMING_SPHERE], x(mdef), y(mdef), MM_ADJACENTOK|maketame|MM_ESUM);
						else		mtmp = makemon(&mons[PM_FIRE_VORTEX], x(mdef), y(mdef), MM_ADJACENTOK|maketame|MM_ESUM);
					}
					else if (u.oonaenergy == AD_COLD){
						if (rn2(2)) mtmp = makemon(&mons[PM_FREEZING_SPHERE], x(mdef), y(mdef), MM_ADJACENTOK|maketame|MM_ESUM);
						else		mtmp = makemon(&mons[PM_ICE_VORTEX], x(mdef), y(mdef), MM_ADJACENTOK|maketame|MM_ESUM);
					}
					else if (u.oonaenergy == AD_ELEC){
						if (rn2(2)) mtmp = makemon(&mons[PM_SHOCKING_SPHERE], x(mdef), y(mdef), MM_ADJACENTOK|maketame|MM_ESUM);
						else		mtmp = makemon(&mons[PM_ENERGY_VORTEX], x(mdef), y(mdef), MM_ADJACENTOK|maketame|MM_ESUM);
					}
					/* Oona's summons time out and vanish */
					if (mtmp) {
						mark_mon_as_summoned(mtmp, mdef, mlev(mdef) + rnd(mlev(mdef)), 0);
						/* can be tame */
						if (maketame) {
							initedog(mtmp);
						}
					}
				}
				break;
			case AD_DISE:
				if (youdef || !mdef->mcan) {
					/* swamp nymphs have additional limitations, like naiads */
					if (pd->mtyp == PM_SWAMP_NYMPH){
						if (((*hp(mdef) < *hpmax(mdef) / 2) && rn2(3)) ||
							result&MM_DEF_DIED) {
							if (youdef) {
								You("collapse into a puddle of noxious fluid!");
								rehumanize();
								change_gevurah(1); //cheated death.
							}
							else {
								if (canseemon(mdef))
									pline("%s collapses into a puddle of noxious fluid!", Monnam(mdef));
								if (!(result&MM_DEF_DIED)) {
									newres = xdamagey(magr, mdef, passive, *hp(mdef));
									if (newres&MM_DEF_DIED)
										result |= MM_DEF_DIED;	/* defender died */
									if (newres&MM_DEF_LSVD)
										result |= MM_AGR_STOP;	/* defender lifesaved */
								}
							}
							/* fall through, diseasing attacker */
						}
						/* if the swamp nymph didn't collapse into a puddle, nothing happens */
						else break;
					}
					/* other creatures have other messages and conditions */
					else
					{
						if ((youdef && !rn2(4)) || !mdef->mspec_used) {
							if (!rn2(3) ||
								pd->mtyp == PM_ZUGGTMOY) {
								if (canseemon(mdef))
									pline("A cloud of spores is released!");
								if (!youdef)
									mdef->mspec_used = 1;
							}
							else break;
						}
					}
					/* if this point has been reached, do the disease */
					if (youagr) {
						diseasemu(pd);
					}
					else {
						if (!Sick_res(mdef))
						{
							if (canseemon(magr))
								pline("%s looks very sick!", Monnam(magr));
							*hp(magr) = (*hp(magr) + 1) / 2;
						}
					}
				}
				break;
			}

			/* These passives only happen if the defender lived, and also require a hit */
			if ((result&MM_HIT) && !(result&MM_DEF_DIED) && passive != &noattack &&
				rn2(3) && (youdef || !mdef->mcan)) {
				switch (passive->adtyp)
				{
				case AD_AXUS:
					/* perform a gaze paralysis attack */
					if (TRUE) {
						/* AD_PLYS gaze has a special case for magr=Axus to pierce reflection */
						/* use widegaze to avoid checking cooldown, failure chance */
						struct attack plys_gaze = {AT_WDGZ, AD_PLYS, passive->damn, passive->damd};
						xgazey(mdef, magr, &plys_gaze, vis);
					}
					/* anger autons */
					if (youagr) {
						int mndx;
						for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
							mndx = monsndx(mtmp->data);
							if (mndx <= PM_QUINON && mndx >= PM_MONOTON && mtmp->mpeaceful){
								pline("%s gets angry...", Amonnam(mtmp));
								untame(mtmp, 0);
							}
						}
					}
					break;
				case AD_PLYS:
					if (pd->mlet == S_EYE || pd->mtyp == PM_IKSH_NA_DEVA) {	/* assumed to be gaze */
						/* the eye can't be blinded */
						if (youdef ? Blind : is_blind(mdef)) {
							if (youagr && pd->mtyp == PM_FLOATING_EYE) {
								pline("%s cannot defend itself.",
									Adjmonnam(mdef, "blind"));
								if (!rn2(500))
									change_luck(-1);
							}
						}
						else {
							/* perform a gaze paralysis attack */
							/* use widegaze to avoid checking cooldown, failure chance */
							struct attack plys_gaze = {AT_WDGZ, AD_PLYS, passive->damn, passive->damd};
							xgazey(mdef, magr, &plys_gaze, vis);
						}
					}
					/* not eyes/gazes, assumed to be touch */
					else {

						if (youagr) {
							if (Free_action) {
								You("momentarily stiffen.");
							}
							else {
								You("are frozen by %s!", mon_nam(mdef));
								nomovemsg = 0;	/* default: "you can move again" */
								nomul(-dmg, "frozen by a monster");
								exercise(A_DEX, FALSE);
							}
						}
						else {
							if (mon_resistance(magr, FREE_ACTION)) {
								if (canseemon(magr)) {
									pline("%s momentarily stiffens.",
										Monnam(magr));
								}
							}
							else {
								if (canseemon(magr)) {
									pline("%s is frozen by %s!",
										Monnam(magr), mon_nam(mdef));
								}
								magr->mcanmove = 0;
								magr->mfrozen = dmg;
							}
						}
					}
					break;
				case AD_SSEX:
					result = xmeleehurty(mdef, magr, passive, passive, (struct obj **)0, FALSE, dmg, 0, vis, FALSE);
					break;
				case AD_STUN:
					if (youdef) {
						if (!Stunned)
							make_stunned((long)dmg, TRUE);
					}
					else {
						if (canseemon(magr)){
							pline("%s %s",
								Monnam(magr),
								makeplural(stagger(magr, "stagger"))
								);
						}
						magr->mstun = 1;
					}
					break;
				case AD_PHYS:
					/* no message */
					/* damage (reduced by DR, half-phys damage, min 1) */
					dmg -= (youagr ? roll_udr(mdef, ROLL_SLOT) : roll_mdr(magr, mdef, ROLL_SLOT));
					if (dmg < 1)
						dmg = 1;
					dmg = reduce_dmg(mdef,dmg,TRUE,FALSE);

					struct attack na = noattack;
					newres = xdamagey(mdef, magr, &na, dmg);
					if (newres&MM_DEF_DIED)
						result |= MM_AGR_DIED;	/* attacker died */
					if (newres&MM_DEF_LSVD)
						result |= MM_AGR_STOP;	/* attacker lifesaved */
					break;
				case AD_COLD:
					/* resistance */
					if (Cold_res(magr)) {
						if (youagr) {
							shieldeff(u.ux, u.uy);
							You_feel("a mild chill.");
							ugolemeffects(AD_COLD, dmg);
						}
						else {
							golemeffects(magr, AD_COLD, dmg);
						}
					}
					/* otherwise, damage */
					else {
						if (youagr) {
							You("are suddenly very cold!");
							roll_frigophobia();
						}
						else if canseemon(magr) {
							pline("%s is suddenly very cold!",
								Monnam(magr));
						}
						newres = xdamagey(mdef, magr, passive, dmg);
						if (newres&MM_DEF_DIED)
							result |= MM_AGR_DIED;	/* attacker died */
						if (newres&MM_DEF_LSVD)
							result |= MM_AGR_STOP;	/* attacker lifesaved */

						/* some monsters get stronger, and split! */
						if (pd->mtyp == PM_BROWN_MOLD ||
							pd->mtyp == PM_BLUE_JELLY ||
							pd->mtyp == PM_ASPECT_OF_THE_SILENCE
							) {
							*hp(mdef) += dmg / 2;
							if (*hpmax(mdef) < *hp(mdef))
								*hpmax(mdef) = *hp(mdef);
							/* at a certain point, the monster will reproduce! */
							if (*hpmax(mdef) >((int)(mlev(mdef) + 1) * 8)){
								(void)split_mon(mdef, magr);
							}
						}
					}
					break;
				case AD_FIRE:
					/* resistance */
					if (Fire_res(magr)) {
						if (youagr) {
							shieldeff(u.ux, u.uy);
							You_feel("mildly warm.");
							ugolemeffects(AD_FIRE, dmg);
						}
						else {
							golemeffects(magr, AD_FIRE, dmg);
						}
					}
					/* otherwise, damage */
					else {
						if (youagr) {
							You("are suddenly very hot!");
						}
						else if canseemon(magr) {
							pline("%s is suddenly very hot!",
								Monnam(magr));
						}
						newres = xdamagey(mdef, magr, passive, dmg);
						if (newres&MM_DEF_DIED)
							result |= MM_AGR_DIED;	/* attacker died */
						if (newres&MM_DEF_LSVD)
							result |= MM_AGR_STOP;	/* attacker lifesaved */
					}
					break;
				case AD_ELEC:
					/* resistance */
					if (Shock_res(magr)) {
						if (youagr) {
							shieldeff(u.ux, u.uy);
							You_feel("a mild tingle.");
							ugolemeffects(AD_ELEC, dmg);
						}
						else {
							golemeffects(magr, AD_ELEC, dmg);
						}
					}
					/* otherwise, damage */
					else {
						if (youagr) {
							You("are jolted with electricity!");
						}
						else if canseemon(magr) {
							pline("%s is jolted with electricity!",
								Monnam(magr));
						}
						newres = xdamagey(mdef, magr, passive, dmg);
						if (newres&MM_DEF_DIED)
							result |= MM_AGR_DIED;	/* attacker died */
						if (newres&MM_DEF_LSVD)
							result |= MM_AGR_STOP;	/* attacker lifesaved */
					}
					break;
				case AD_DARK:
					/* resistance */
					if (Dark_res(magr)) {
						if (youagr) {
							shieldeff(u.ux, u.uy);
							You_feel("unbothered.");
						}
					}
					/* otherwise, damage */
					else {
						if (youagr) {
							if(Mortal_race){
								You("are suddenly gripped by the terror of death!");
								dmg *= 2;
							} else {
								You("are shrouded in dark!");
							}
						}
						else if canseemon(magr) {
							if(mortal_race(magr))
								dmg *= 2;
							pline("%s is shrouded in dark!",
								Monnam(magr));
						}
						newres = xdamagey(mdef, magr, passive, dmg);
						if (newres&MM_DEF_DIED)
							result |= MM_AGR_DIED;	/* attacker died */
						if (newres&MM_DEF_LSVD)
							result |= MM_AGR_STOP;	/* attacker lifesaved */
					}
					break;
				}
			}
		}
	}
	return result;
}

void
passive_obj2(magr, mdef, otmp, attk, passive)
struct monst * magr;		/* creature who was attacking first */
struct monst * mdef;		/* creature whose passive attack is being used */
struct obj * otmp;			/* object that is being affected by the passive attack */
struct attack * attk;		/* attack aggressor used, which is now being counterattacked against */
struct attack * passive;	/* specific passive attack being used */
{
	int i;
	struct attack passive_buffer = noattack;

	/* if not given, grab passive attack from permonst */
	if (!passive || is_null_attk(passive))
	{
		/* we need a defender to do this */
		if (!mdef)
			return;

		struct permonst * pd = ((mdef == &youmonst) ? youracedata : mdef->data);
		passive = &passive_buffer;
		for (i = 0; i < NATTK; i++) {
			if (pd->mattk[i].aatyp == AT_NONE &&
				!is_null_attk(&(pd->mattk[i]))) {
				passive = &pd->mattk[i];
				break;
			}
		}
		/* if we failed to get a passive attack, nothing happens. */
		if (is_null_attk(passive))
			return;
	}
	/* if not given, get otmp based on attk and magr */
	if (!otmp) {
		/* we need both magr and attk to do this */
		if (!magr || !attk || is_null_attk(attk))
			return;
		boolean youagr = (magr == &youmonst);
		long slot = attk_equip_slot(magr, attk->aatyp);

		switch (slot)
		{
		case W_ARMG:
			otmp = (youagr ? uarmg : which_armor(magr, slot));
			break;
		case W_ARMF:
			otmp = (youagr ? uarmf : which_armor(magr, slot));
			break;
		case W_ARMH:
			otmp = (youagr ? uarmh : which_armor(magr, slot));
			break;
		default:
			otmp = (struct obj *)0;
			break;
		}
		/* if we failed to get an object, nothing happens. */
		if (!otmp)
			return;
	}

	switch (passive->adtyp)
	{
	case AD_EACD:
	case AD_ACID:
		if (!rn2(passive->adtyp == AD_EACD ? 2 : 6)) {
			erode_obj(otmp, TRUE, FALSE);
		}
		break;
	case AD_RUST:
		if (!mdef || !mdef->mcan) {
			erode_obj(otmp, FALSE, FALSE);
		}
		break;
	case AD_CORR:
		if (!mdef || !mdef->mcan) {
			erode_obj(otmp, TRUE, FALSE);
		}
		break;
	case AD_ENCH:
		if (!mdef || !mdef->mcan) {
			if (drain_item(otmp) && carried(otmp) &&
				(otmp->known || otmp->oclass == ARMOR_CLASS)) {
				/* carried() already assured us the player is involved */
				Your("%s less effective.", aobjnam(otmp, "seem"));
			}
			break;
		}
	default:
		break;
	}

	if (carried(otmp))
		update_inventory();
	return;
}

void
wakeup2(mdef, your_fault)
struct monst * mdef;
boolean your_fault;
{
	if (mdef == &youmonst)
	{
		if (u.usleep && u.usleep < monstermoves && rn2(20) < ACURR(A_WIS)) {
			multi = -1;
			nomovemsg = "The combat suddenly awakens you.";
		}
	}
	else
	{
		if (mdef && !DEADMONSTER(mdef))
			wakeup(mdef, your_fault);
	}
	return;
}

/* android_combo()
 *
 * Perform's the player's android weapon combo dependent on their mainhand weapon
 *
 * Returns FALSE if this was cancelled before doing anything.
 */
int
android_combo()
{
	struct monst * mdef;
	int vis;

	if (!uandroid) {
		pline("You aren't an android!");
		return MOVE_CANCELLED;
	}

	if(Straitjacketed){
		You("can't do a combo while your arms are bound!");
		return MOVE_CANCELLED;
	}

	if(uwep && is_ammo(uwep)){
		You("can't do a combo using ammo!");
		return MOVE_CANCELLED;
	}

	static struct attack weaponhit =	{ AT_WEAP, AD_PHYS, 0, 0 };
	static struct attack kickattack =	{ AT_KICK, AD_PHYS, 1, 2 };
	static struct attack finisher =		{ AT_CLAW, AD_PHYS,16, 8 };

#define STILLVALID(mdef) (!DEADMONSTER(mdef) && mdef == m_at(u.ux + u.dx, u.uy + u.dy))

	/* unarmed */
	if (!uwep || (is_lightsaber(uwep) && !litsaber(uwep))){
		if (!getdir((char *)0))
			return MOVE_CANCELLED;
		if (u.ustuck && u.uswallow)
			mdef = u.ustuck;
		else
			mdef = m_at(u.ux + u.dx, u.uy + u.dy);

		if (!mdef)
			You("swing wildly!");
		else {
			vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
			xmeleehity(&youmonst, mdef, &weaponhit, (struct obj **)0, vis, 0, FALSE);
			if(STILLVALID(mdef))
				xmeleehity(&youmonst, mdef, &weaponhit, (struct obj **)0, vis, 0, FALSE);
		}
		u.uen--;
		flags.botl = 1;
		if (P_SKILL(P_BARE_HANDED_COMBAT) >= P_SKILLED && u.uen > 0){
			if (dokick() != MOVE_CANCELLED){
				u.uen--;
				flags.botl = 1;
			}
			else return MOVE_ATTACKED;
		}
		if (P_SKILL(P_BARE_HANDED_COMBAT) >= P_EXPERT && u.uen > 0){
			int j = jump(1) != MOVE_CANCELLED;
			int k = dokick() != MOVE_CANCELLED;
			if (j || k){
				u.uen--;
				flags.botl = 1;
			}
			else return MOVE_ATTACKED;
		}
		if (P_SKILL(P_BARE_HANDED_COMBAT) >= P_MASTER && u.uen > 0){
			int j = jump(1) != MOVE_CANCELLED;
			int d = getdir((char *)0);
			if (!j && !d) return MOVE_ATTACKED;
			u.uen--;
			flags.botl = 1;
			if (d){
				if (u.ustuck && u.uswallow)
					mdef = u.ustuck;
				else
					mdef = m_at(u.ux + u.dx, u.uy + u.dy);
				if (!mdef || DEADMONSTER(mdef))
					You("swing wildly!");
				else {
					vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
					xmeleehity(&youmonst, mdef, &weaponhit, (struct obj **)0, vis, 0, FALSE);
					if(STILLVALID(mdef))
						xmeleehity(&youmonst, mdef, &weaponhit, (struct obj **)0, vis, 0, FALSE);
					if(STILLVALID(mdef))
						xmeleehity(&youmonst, mdef, &kickattack, (struct obj **)0, vis, 0, FALSE);
					if(STILLVALID(mdef))
						xmeleehity(&youmonst, mdef, &kickattack, (struct obj **)0, vis, 0, FALSE);
				}
			}
		}
		if(P_SKILL(P_BARE_HANDED_COMBAT) >= P_GRAND_MASTER && u.uen > 0){
			if(!getdir((char *)0)) return MOVE_ATTACKED;
			u.uen--;
			flags.botl = 1;
			if (u.ustuck && u.uswallow)
				mdef = u.ustuck;
			else
				mdef = m_at(u.ux + u.dx, u.uy + u.dy);
			if (!mdef || DEADMONSTER(mdef))
				You("swing wildly!");
			else {
				vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
				xmeleehity(&youmonst, mdef, &finisher, (struct obj **)0, vis, 0, FALSE);
			}
		}
		return MOVE_ATTACKED;
	}
	else if (is_lightsaber(uwep) && litsaber(uwep)){ //!uwep handled above
		/* get direction of attack */
		if (!getdir((char *)0))
			return MOVE_CANCELLED;
		/* get defender */
		if (u.ustuck && u.uswallow)
			mdef = u.ustuck;
		else
			mdef = m_at(u.ux + u.dx, u.uy + u.dy);

		/* attack (melee twice OR throw lightsaber) */
		if (!mdef) {
			if(uwep)
				projectile(&youmonst, uwep, (void *)0, HMON_PROJECTILE|HMON_FIRED, u.ux, u.uy, u.dx, u.dy, u.dz, 10, FALSE, TRUE, FALSE);
		}
		else {
			vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
			xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
			if(STILLVALID(mdef))
				xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
		}
		u.uen--;
		flags.botl = 1;
		if (uwep && P_SKILL(weapon_type(uwep)) >= P_SKILLED && u.uen > 0){
			int a;
			int k;
			/* get direction */
			a = getdir((char *)0);
			if (a && !u.dz){
				/* get defender */
				if (u.ustuck && u.uswallow)
					mdef = u.ustuck;
				else
					mdef = m_at(u.ux + u.dx, u.uy + u.dy);
				/* attack (melee once OR throw lightsaber) */
				if (!mdef) {
					if(uwep)
						projectile(&youmonst, uwep, (void *)0, HMON_PROJECTILE|HMON_FIRED, u.ux, u.uy, u.dx, u.dy, u.dz, 10, FALSE, TRUE, FALSE);
				}
				else {
					vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
					xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
				}
			}
			k = dokick() != MOVE_CANCELLED;
			if ((a && !u.dz) || k){
				u.uen--;
			}
			else return MOVE_ATTACKED;
		}
		if (uwep && P_SKILL(weapon_type(uwep)) >= P_EXPERT && u.uen > 0){
			int j = jump(1) != MOVE_CANCELLED;
			int d = getdir((char *)0);
			if (!j && (!d || u.dz))
				return MOVE_ATTACKED;
			u.uen--;
			flags.botl = 1;
			if (d){
				/* get defender */
				if (u.ustuck && u.uswallow)
					mdef = u.ustuck;
				else
					mdef = m_at(u.ux + u.dx, u.uy + u.dy);
				/* attack (melee twice OR throw lightsaber) */
				if (!mdef) {
					if(uwep)
						projectile(&youmonst, uwep, (void *)0, HMON_PROJECTILE|HMON_FIRED, u.ux, u.uy, u.dx, u.dy, u.dz, 10, FALSE, TRUE, FALSE);
				}
				else {
					vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
					xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
					if(STILLVALID(mdef))
						xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
				}
			}
		}
		return MOVE_ATTACKED;
	}
	else if (weapon_type(uwep) == P_SPEAR || weapon_type(uwep) == P_LANCE){ //!uwep handled above
		boolean attacked = FALSE;
		int n = 1;
		
		if (uwep && P_SKILL(weapon_type(uwep)) >= P_SKILLED)
			n++;
		if (uwep && P_SKILL(weapon_type(uwep)) >= P_EXPERT)
			n++;

		while (n > 0 && u.uen > 0){
			/* get direction of attack; if first time, cancelling will take no time */
			if (!getdir((char *)0) || u.dz)
				return attacked ? MOVE_ATTACKED : MOVE_CANCELLED;
			/* get defender */
			if (u.ustuck && u.uswallow)
				mdef = u.ustuck;
			else
				mdef = m_at(u.ux + u.dx, u.uy + u.dy);
			/* attack */
			if (!mdef)
				You("stab wildly!");
			else {
				vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
				xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
				if(STILLVALID(mdef))
					xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
			}
			//lunge in the direction attacked. 
			if(!u.ustuck && !u.utrap && goodpos(u.ux+u.dx, u.uy+u.dy, &youmonst, 0)){
				hurtle(u.dx, u.dy, 1, FALSE, FALSE);
				spoteffects(TRUE);
			}
			n--;
			u.uen--;
			flags.botl = 1;
			attacked = TRUE;
		}
		return MOVE_ATTACKED;
	}
	else if (weapon_type(uwep) == P_WHIP){ //!uwep handled above
		/* get direction of attack */
		if (!getdir((char *)0) || u.dz)
			return MOVE_CANCELLED;
		/* get defender */
		if (u.ustuck && u.uswallow)
			mdef = u.ustuck;
		else
			mdef = m_at(u.ux + u.dx, u.uy + u.dy);
		/* attack (twice) */
		if (!mdef)
			You("swing wildly!");
		else {
			vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
			xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
			if(STILLVALID(mdef))
				xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
		}
		u.uen--;
		flags.botl = 1;

		if (uwep && P_SKILL(weapon_type(uwep)) >= P_SKILLED && u.uen > 0){
			/* get direction AND do whip things */
			if (!use_whip(uwep) || !uwep)
				return MOVE_ATTACKED;
			/* get defender */
			if (u.ustuck && u.uswallow)
				mdef = u.ustuck;
			else
				mdef = m_at(u.ux + u.dx, u.uy + u.dy);
			/* attack (once) */
			if (!mdef)
				You("swing wildly!");
			else {
				vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
				xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
			}
			u.uen--;
			flags.botl = 1;
		}


		if (uwep && P_SKILL(weapon_type(uwep)) >= P_EXPERT && u.uen > 0){
			/* get direction AND do whip things */
			if (!use_whip(uwep) || !uwep)
				return MOVE_ATTACKED;
			if (uwep->otyp == FORCE_WHIP){
				/* turn it into a sword */
				use_force_sword(uwep);
				/* get defender */
				if (u.ustuck && u.uswallow)
					mdef = u.ustuck;
				else
					mdef = m_at(u.ux + u.dx, u.uy + u.dy);
				/* attack (twice) */
				if (!mdef)
					You("swing wildly!");
				else {
					vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
					xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
					if(STILLVALID(mdef))
						xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
				}
			}
			else {
				/* get defender */
				if (u.ustuck && u.uswallow)
					mdef = u.ustuck;
				else
					mdef = m_at(u.ux + u.dx, u.uy + u.dy);
				/* attack (once) */
				if (!mdef)
					You("swing wildly!");
				else {
					vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
					xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
				}
			}
			u.uen--;
			flags.botl = 1;
		}
		return MOVE_ATTACKED;
	}
	else if (!bimanual(uwep, youracedata)){ //!uwep handled above
		/* get direction of attack */
		if (!getdir((char *)0) || u.dz)
			return MOVE_CANCELLED;
		
		/* get defender */
		if (u.ustuck && u.uswallow)
			mdef = u.ustuck;
		else
			mdef = m_at(u.ux + u.dx, u.uy + u.dy);
		/* attack (twice) */
		if (!mdef)
			You("swing wildly!");
		else {
			vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
			xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
			if(STILLVALID(mdef))
			xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
		}
		u.uen--;
		flags.botl = 1;

		if (uwep && P_SKILL(weapon_type(uwep)) >= P_SKILLED && u.uen > 0){
			//Two throws. Aborting either one ends the combo.
			if(dofire() == MOVE_CANCELLED)
				return MOVE_ATTACKED;
			//Charge energy for continuing combo after first throw.
			u.uen--;
			flags.botl = 1;
			//Now do second throw. Aborting either one ends the combo.
			if(dofire() == MOVE_CANCELLED)
				return MOVE_ATTACKED;
		}
		if (uwep && P_SKILL(weapon_type(uwep)) >= P_EXPERT && u.uen > 0){
			//One throw, followed by a moving attack.  Aborting either one ends the combo.
			if(dofire() != MOVE_CANCELLED){
				//Charge energy for continuing the combo after the throw
				u.uen--;
				flags.botl = 1;
				if (uwep){
					/* get direction of attack */
					if (!getdir((char *)0) || u.dz)
						return MOVE_ATTACKED;
					/* Lunge in indicated direction */
					if(!u.ustuck && !u.utrap && goodpos(u.ux+u.dx, u.uy+u.dy, &youmonst, 0)){
						hurtle(u.dx, u.dy, 1, FALSE, FALSE);
						spoteffects(TRUE);
					}
					/* get defender */
					if (u.ustuck && u.uswallow)
						mdef = u.ustuck;
					else
						mdef = m_at(u.ux + u.dx, u.uy + u.dy);
					/* attack (twice) */
					if (!mdef)
						You("leap and swing wildly!");
					else {
						vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
						xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
					}
					if(uwep)
						projectile(&youmonst, uwep, (void *)0, HMON_PROJECTILE|HMON_FIRED, u.ux, u.uy, u.dx, u.dy, u.dz, 10, FALSE, TRUE, FALSE);
				}
			}
			else return MOVE_ATTACKED;
		}
		return MOVE_ATTACKED;
	}
	else if (bimanual(uwep, youracedata)){ //!uwep handled above
		int i, j;
		/* get direction of attack */
		if (!getdir((char *)0) || u.dz)
			return MOVE_CANCELLED;

		if (u.dz) {
			/* if getdir() was gived u.dz != 0, we will just pick a random direction to start hitting */
			i = rn2(8);
		}
		else {
			/* get your targetted direction's index */
			for (i = 0; i < 8; i++)
			if (xdir[i] == u.dx && ydir[i] == u.dy)
				break;
		}
		/* attack counterclockwise, hitting first direction twice (first and last hits) */
		for (j = 8; j >= 0; j--){
			if (u.ustuck && u.uswallow)
				mdef = u.ustuck;
			else
				mdef = m_at(u.ux + xdir[(i + j) % 8], u.uy + ydir[(i + j) % 8]);
			/* isn't that nice, we don't attack pets (even when confused?) */
			if (mdef && !mdef->mtame){
				vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
				xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
			}
		}
		u.uen--;
		flags.botl = 1;
		youmonst.movement -= 3;
		if (uwep && P_SKILL(weapon_type(uwep)) >= P_SKILLED && u.uen > 0){
			/* use a kick to get direction */
			if (dokick() == MOVE_CANCELLED)
				return MOVE_ATTACKED;
			/* get your targetted direction's index */
			for (i = 0; i < 8; i++)
			if (xdir[i] == u.dx && ydir[i] == u.dy)
				break;
			/* attack clockwise, hitting first direction twice (first and last hits) */
			for (j = 0; j <= 8; j++){
				if (u.ustuck && u.uswallow)
					mdef = u.ustuck;
				else
					mdef = m_at(u.ux + xdir[(i + j) % 8], u.uy + ydir[(i + j) % 8]);
				/* isn't that nice, we don't attack pets (even when confused?) */
				if (mdef && !mdef->mtame){
					vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
					xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
				}
			}
			u.uen--;
			flags.botl = 1;
			youmonst.movement -= 3;
		}
		if (uwep && P_SKILL(weapon_type(uwep)) >= P_EXPERT && u.uen > 0){
			if (!getdir((char *)0) || u.dz)
				return MOVE_ATTACKED;
			/* attack counterclockwise, hitting first direction once (and then throw) */
			for (j = 8; j > 0; j--){
				if (u.ustuck && u.uswallow)
					mdef = u.ustuck;
				else
					mdef = m_at(u.ux + xdir[(i + j) % 8], u.uy + ydir[(i + j) % 8]);
				/* isn't that nice, we don't attack pets (even when confused?) */
				if (mdef && !mdef->mtame){
					vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
					xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
				}
			}
			if(uwep)
				projectile(&youmonst, uwep, (void *)0, HMON_PROJECTILE|HMON_FIRED, u.ux, u.uy, u.dx, u.dy, u.dz, 10, FALSE, TRUE, FALSE);
			u.uen--;
			flags.botl = 1;
			youmonst.movement -= 3;
		}
		return MOVE_ATTACKED;
	}
	/* This should never be reached */
	return MOVE_CANCELLED;
}


/* movement_combos()
 * 
 * assigns/clears prev_dir,
 * checks if a special move should be made based on prev_dir and current movement,
 * and makes it if so.
 */
void
movement_combos()
{
	boolean did_combo = FALSE;

	/* ignore cancelled and instant actions */
	if (you_action_cost(flags.move, FALSE) == 0)
		return;

	/* only movements and attacks count for setting prev_dir,
	 * and being paralyzed or moving with 'm' cancels it as well */
	if (!(u.umoved || u.uattked) || flags.nopick || (multi<0)) {
		u.prev_dir.x = 0;
		u.prev_dir.y = 0;
		return;
	}

	/* Monk Moves */
	if (Role_if(PM_MONK) && !Upolyd) {
		int moveID = check_monk_move();
		if (moveID != 0 && perform_monk_move(moveID)) {
			nomul(0, NULL);
			u.uattked = TRUE;
			did_combo = TRUE;
			/* takes at least as long as a standard action */
			flags.move |= MOVE_STANDARD;
		}
	}
	/* Expert Moves (lunges and knockback charges) */
	if((!did_combo || (uwep && is_monk_weapon(uwep))) && u.umoved && !flags.nopick && (multi >= 0)){
		if (perform_expert_move()) {
			nomul(0, NULL);
			u.uattked = TRUE;
			did_combo = TRUE;
			/* takes at least as long as a standard action */
			flags.move |= MOVE_STANDARD;
		}
	}
	extern coord save_d;
	if (did_combo) {
		/* successfully doing a combo clears prev_dir */
		u.prev_dir.x = 0;
		u.prev_dir.y = 0;
	}
	else if(u.dx || u.dy){
		/* otherwise, build prev_dir */
		u.prev_dir.x = u.dx;
		u.prev_dir.y = u.dy;
	}
	else {
		u.prev_dir.x = save_d.x;
		u.prev_dir.y = save_d.y;
	}
	save_d.x = 0;
	save_d.y = 0;
}

#define peace_check_move(mon) ((canspotmon(mon) || mon_warning(mon)) && (Hallucination || !mon->mpeaceful) && !nonthreat(mon))

struct monst *
adjacent_move_target(arm)
struct obj *arm;
{
	struct monst *mon;
	//Same rules as kicking. Whirly monsters allow moves, solid ones do not.
	if(u.ustuck && u.uswallow){
		if(is_whirly(u.ustuck->data))
			return u.ustuck;
		else return (struct monst *)0;
	}
	if(!isok(u.ux+u.dx, u.uy+u.dy))
		return (struct monst *)0;
	mon = m_at(u.ux+u.dx, u.uy+u.dy);
	if(!mon)
		return (struct monst *)0;
	if((touch_petrifies(mon->data)
		|| mon->mtyp == PM_MEDUSA)
	 && !(Stone_resistance || arm))
		return (struct monst *)0;
	if(peace_check_move(mon))
		return mon;
	return (struct monst *)0;
}

boolean
circle_monk_target(arm)
struct obj *arm;
{
	struct monst *mon;
	int i, j;
	//Same rules as kicking. Whirly monsters allow moves, solid ones do not.
	if(u.ustuck && u.uswallow){
		if(is_whirly(u.ustuck->data))
			return TRUE;
		else return FALSE;
	}
	for(i = u.ux-1; i < u.ux+2; i++) for(j = u.uy-1; j < u.uy+2; j++){
		if(i == u.ux && j == u.uy)
			continue;
		if(!isok(i,j))
			continue;
		mon = m_at(i, j);
		if(!mon)
			continue;
		if((touch_petrifies(mon->data)
			|| mon->mtyp == PM_MEDUSA)
		 && !(Stone_resistance || arm))
			continue;
		if(peace_check_move(mon))
			return TRUE;
	}
	return FALSE;
}

boolean
beam_monk_target()
{
	struct monst *mon;
	int i;
	int ix = u.ux, iy = u.uy;
	boolean at_least_one = FALSE;
	//Same rules as kicking. Whirly monsters allow moves, solid ones do not.
	if(u.ustuck && u.uswallow){
		if(is_whirly(u.ustuck->data))
			return TRUE;
		else return FALSE;
	}
	for(i = 1; i < BOLT_LIM; i++){
		ix += u.dx;
		iy += u.dy;
		if(!isok(ix,iy))
			return at_least_one;
		mon = m_at(ix, iy);
		if(!mon){
			if(!ZAP_POS(levl[ix][iy].typ) || closed_door(ix, iy))
				return at_least_one;
			continue;
		}
		if(!peace_check_move(mon)){
			//We'll call this the Monk's "sixth sense" talking <_<'
			if(mon->mpeaceful && !Hallucination)
				return FALSE;
			else continue;
		}
		else at_least_one = TRUE;
		if(!ZAP_POS(levl[ix][iy].typ) || closed_door(ix, iy))
			return at_least_one;
	}
	return at_least_one;
}

void
monk_aura_bolt()
{
	struct zapdata zapdat = { 0 };
	basiczap(&zapdat, u.ualign.record < -3 ? AD_UNHY : AD_HOLY, ZAP_SPELL, (u.ulevel+2) / 3 );
	zapdat.damd = 4;
	if(uarmg && uarmg->oartifact == ART_WRAPPINGS_OF_THE_SACRED_FI)
		zapdat.damd *= 2;
	zapdat.affects_floor = FALSE;
	zapdat.phase_armor = TRUE;
	//Currently these cook off without the player's explicit say-so
	zapdat.no_bounce = TRUE;
	zapdat.unreflectable = ZAP_REFL_NEVER;
	zap(&youmonst, u.ux, u.uy, u.dx, u.dy, BOLT_LIM, &zapdat);
}

void
monk_meteor_drive(mdef)
struct monst *mdef;
{
	int tmp, weight = mdef->data->cwt;
	struct trap *chasm;
	static struct attack grab =	{ AT_TUCH, AD_PHYS, 1, 1 };
	int res;
	boolean dodig = FALSE;
	
	res = xmeleehity(&youmonst, mdef, &grab, (struct obj **)0, FALSE, 0, FALSE);
	if(res&(MM_MISS|MM_DEF_DIED))
		return;
	
	// if(weight > 2*weight_cap() - inv_weight()){
		// pline("Too heavy!");
		// return;
	// }
	
	/* Slam the monster into the ground */
	if (!mdef || u.uswallow) return;

	You("hurl %s downwards...", mon_nam(mdef));
	if(!(levl[x(mdef)][y(mdef)].typ == ROOM
	 ||levl[x(mdef)][y(mdef)].typ == CORR
	 ||levl[x(mdef)][y(mdef)].typ == SOIL
	 ||levl[x(mdef)][y(mdef)].typ == SAND
	 ||levl[x(mdef)][y(mdef)].typ == GRASS
	 ||levl[x(mdef)][y(mdef)].typ == DOOR
	))
		return;

	tmp = weight >= WT_GIGANTIC ? 12 : 
		  weight >= WT_HUGE ? 10 :
		  weight >= WT_LARGE ? 8 :
		  weight >= WT_MEDIUM ? 6 :
		  weight >= WT_SMALL ? 4 :
		  weight >= WT_TINY ? 2 :
		  weight >= WT_DIMINUTIVE ? 1 : 0;
	chasm = t_at(x(mdef), y(mdef));	
	if(weight >= WT_MEDIUM && (!chasm || chasm->ttyp == PIT)){
		pline("%s slams into the %s, creating a crater!", Monnam(mdef), surface(u.ux + u.dx, u.uy + u.dy));
		tmp *= 2;
		dodig = TRUE;
	}
	else {
		if(tmp)
			pline("%s slams into the %s!", Monnam(mdef), surface(u.ux + u.dx, u.uy + u.dy));
		else
			pline("%s bounces lightly off the %s.", Monnam(mdef), surface(u.ux + u.dx, u.uy + u.dy));
	}

	mselftouch(mdef, "Falling, ", TRUE);
	if(!tmp)
		return; //Too light to do damage :(
	if (!DEADMONSTER(mdef) && tmp) {
		int nd = dbon((struct obj *)0, &youmonst);
		nd = max(nd, 1);
		xdamagey(&youmonst, mdef, (struct attack *)0, d(nd,tmp));
	}

	/* do pit/hole digging after applying damage - holes can migrate monsters, and we cannot kill migrating monsters */
	if (dodig) {
		digfarhole(!chasm, x(mdef), y(mdef), FALSE);
		chasm = t_at(x(mdef), y(mdef));
		if (chasm){
			if(chasm->ttyp == PIT && !DEADMONSTER(mdef) && !MIGRATINGMONSTER(mdef)) {
				if (!mon_resistance(mdef,FLYING) && !is_clinger(mdef->data))
					mdef->mtrapped = 1;
			}
			chasm->tseen = 1;
			levl[u.ux + u.dx][u.uy + u.dy].doormask = 0;
		}
	}

	return;
}

#undef peace_check_move


/* perform_expert_move()
 *
 * Attempts to perform any applicable expert weapon trait move, given that a target is found.
 *
 * Returns TRUE if a move goes off.
 */
boolean
perform_expert_move()
{
	struct monst *mdef;
#define STILLVALID(mdef) (!DEADMONSTER(mdef) && mdef == m_at(u.ux + u.dx, u.uy + u.dy))
	struct attack lungehit =	{ AT_WEAP, AD_PHYS, 0, 0 };
	struct attack pushhit =	{ AT_WEAP, AD_PUSH, 0, 0 };
	boolean messaged = FALSE;
	if(!uwep)
		return FALSE;
	if(CHECK_ETRAIT(uwep, &youmonst, ETRAIT_LUNGE)
		&& ROLL_ETRAIT(uwep, &youmonst, TRUE, rn2(2))
	) {
		mdef = adjacent_move_target(uwep);
		if(mdef){
			if(wizard)
				pline("Lunge!");
			else if(!messaged){
				You("charge %s.", mon_nam(mdef));
				messaged = TRUE;
			}
			boolean vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
			xmeleehity(&youmonst, mdef, &lungehit, &uwep, vis, 0, FALSE);
		}
	}
	if(CHECK_ETRAIT(uwep, &youmonst, ETRAIT_KNOCK_BACK_CHARGE)
		&& ROLL_ETRAIT(uwep, &youmonst, TRUE, rn2(2))
		&& (uwep->o_e_trait&ETRAIT_KNOCK_BACK)
	) {
		mdef = adjacent_move_target(uwep);
		if(mdef){
			if(wizard)
				pline("Push!");
			else if(!messaged){
				You("charge %s.", mon_nam(mdef));
				messaged = TRUE;
			}
			boolean vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
			xmeleehity(&youmonst, mdef, &pushhit, &uwep, vis, 0, FALSE);
		}
	}
	return FALSE;
}

/* perform_monk_move()
 *
 * Attempts to perform the given monk move, given that a target is found and energy is sufficient.
 *
 * Returns TRUE if a move goes off.
 */
boolean
perform_monk_move(moveID)
int moveID;
{
	struct monst *mdef;
#define STILLVALID(mdef) (!DEADMONSTER(mdef) && mdef == m_at(u.ux + u.dx, u.uy + u.dy))
	static struct attack weaponhit =	{ AT_WEAP, AD_PHYS, 0, 0 };
	static struct attack xweponhit =	{ AT_XWEP, AD_PHYS, 0, 0 };
	static struct attack bitehit =	{ AT_BITE, AD_PHYS, 1, 6 };
	if(!moveID)
		return FALSE;
	if(!monk_style_active(moveID)) return FALSE;
	switch(moveID){
		case DIVE_KICK:
		if(Race_if(PM_CHIROPTERAN)){
			if((mdef = adjacent_move_target(uarmf))){
				pline("Swoop!");
				boolean vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
				if(!uwep || is_monk_weapon(uwep))
					xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
				if(u.twoweap && STILLVALID(mdef) && (!uswapwep || is_monk_weapon(uswapwep)))
					xmeleehity(&youmonst, mdef, &xweponhit, &uswapwep, vis, 0, FALSE);
				if(STILLVALID(mdef))
					xmeleehity(&youmonst, mdef, &bitehit, (struct obj **)0, vis, 0, FALSE);
				return TRUE;
			}
		}
		else {
			if(!EWounded_legs && (mdef = adjacent_move_target(uarmf)) && near_capacity() <= SLT_ENCUMBER){
				pline("Dive kick!");
				dive_kick_monster(mdef);
				return TRUE;
			}
		}
		break;
		case AURA_BOLT:
		if((!uwep || is_monk_weapon(uwep)) 
			&& (!(uswapwep && u.twoweap) || is_monk_weapon(uswapwep)) 
			&& (u.ualign.record < -3 || u.ualign.record > 3)
			&& !Is_spire(&u.uz) 
			&& beam_monk_target()
		){
			pline("Aura bolt!");
			monk_aura_bolt();
			return TRUE;
		}
		break;
		case BIRD_KICK:
		if(!EWounded_legs && circle_monk_target(uarmf) && near_capacity() <= SLT_ENCUMBER){
			if(Race_if(PM_CHIROPTERAN)){
				pline("Wing storm!");
				wing_storm_monsters();
			}
			else {
				pline("Bird kick!");
				bird_kick_monsters();
			}
			return TRUE;
		}
		break;
		case METODRIVE:
		if(!uwep && !(uswapwep && u.twoweap) && inv_weight() < 0 && !u.uswallow && (mdef = adjacent_move_target(uarmg))){
			pline("Meteor drive!");
			monk_meteor_drive(mdef);
			return TRUE;
		}
		break;
		case PUMMEL:
		if((!uwep || is_monk_weapon(uwep)) && (!(uswapwep && u.twoweap) || is_monk_weapon(uswapwep)) && (mdef = adjacent_move_target(uarmg))){
			if(uwep)
				pline("Flurry!");
			else
				pline("Pummel!");
			boolean vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
			for(int i = 0; i < 2 && STILLVALID(mdef); i++){
				if((i&0x1) && u.twoweap) /*Odd attacks with valid offhand*/
					xmeleehity(&youmonst, mdef, &xweponhit, &uswapwep, vis, 0, FALSE);
				else
					xmeleehity(&youmonst, mdef, &weaponhit, &uwep, vis, 0, FALSE);
			}
			return TRUE;
		}
		break;
	}
	return FALSE;
}

/* check_monk_move()
 *
 * Based on prev_dir and current u.dx/dy, determine which monk move (if any) should happen.
 *
 * Returns the move ID or 0 if no ID can be chosen.
 */
int
check_monk_move()
{
	int i;
	int dx1 = u.prev_dir.x;
	int dy1 = u.prev_dir.y;
	int dx2 = u.dx;
	int dy2 = u.dy;
	if(!dx2 && !dy2){
		extern coord save_d;
		dx2 = save_d.x;
		dy2 = save_d.y;
	}
	//If there haven't been two inputs for some reason, return false.
	if(!(dx1 || dy1) || !(dx2 || dy2))
		return 0;
	///Adjust first vector to first quadrent (favoring up over right)
	//Reflect over y axis
	if(dx1 < 0){
		dx1 *= -1;
		dx2 *= -1;
	}
	//Reflect over x axis
	if(dy1 < 0){
		dy1 *= -1;
		dy2 *= -1;
	}
	//Reflect over y = x
	if(dx1 == 1 && dy1 == 0){
		i = dx1;
		dx1 = dy1;
		dy1 = i;
		i = dx2;
		dx2 = dy2;
		dy2 = i;
	}
	///Mirror to face right
	//Reflect over first vector
	if(dx2 < 0){
		//if vector 1 is diagonal, reflect over x = y
		if(dx1 == 1 && dy1 == 1){
			i = dx2;
			dx2 = dy2;
			dy2 = i;
		}
		//Vector 1 should be verticle, reflect over y axis
		else {
			dx2 *= -1;
		}
	}
	if(dx1 == 1 && dy1 == 1 && dx2 == 1 && dy2 == 0){
		i = dx2;
		dx2 = dy2;
		dy2 = i;
	}
	//Find move
	if(dx1 == 1 && dy1 == 1){
		/*
			..D
			.eA
			PMB
		 */
		if(dx2 == 1 && dy2 == 1){
			return DIVE_KICK;
		}
		if(dx2 == 0 && dy2 == 1){
			return AURA_BOLT;
		}
		if(dx2 == 1 && dy2 == -1){
			return BIRD_KICK;
		}
		if(dx2 == 0 && dy2 == -1){
			return METODRIVE;
		}
		if(dx2 == -1 && dy2 == -1){
			return PUMMEL;
		}
	}
	else {
		/*
			DIVE_KICK, AURA_BOLT,
			error, BIRD_KICK,
			PUMMEL, METODRIVE
		 */
		if(dx2 == 0 && dy2 == 1){
			return DIVE_KICK;
		}
		if(dx2 == 1 && dy2 == 1){
			return AURA_BOLT;
		}
		if(dx2 == 1 && dy2 == 0){
			return BIRD_KICK;
		}
		if(dx2 == 1 && dy2 == -1){
			return METODRIVE;
		}
		if(dx2 == 0 && dy2 == -1){
			return PUMMEL;
		}
	}
	impossible("Unhandled move inputs (%d, %d)to(%d, %d), (%d, %d)to(%d, %d)", u.prev_dir.x, u.prev_dir.y, u.dx, u.dy, dx1, dy1, dx2, dy2);
	return 0;
}

/* u_pole_pound()
 * 
 * hits monster at bhitpos with your (mainhand) polearm
 */
int
u_pole_pound(mdef)
struct monst * mdef;
{
	int vis = (VIS_MAGR | VIS_NONE) | (canseemon(mdef) ? VIS_MDEF : 0);
	int res;
	notonhead = (bhitpos.x != x(mdef) || bhitpos.y != y(mdef));
	if(check_oprop(uwep, OPROP_CCLAW))
		uwep->otyp = CLAWED_HAND;
	res = xmeleehity(&youmonst, mdef, &basicattack, &uwep, vis, 0, TRUE);
	if(check_oprop(uwep, OPROP_CCLAW))
		uwep->otyp = uwep->oartifact == ART_AMALGAMATED_SKIES ? TWO_HANDED_SWORD : CLUB;
	return res;
}

/* mummy_curses_x()
 * 
 * Mummy curses (for use in various contexts). Returns result flags.
 */
int
mummy_curses_x(magr, mdef)
struct monst * magr;
struct monst * mdef;
{
	int cnum;
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	boolean visible = (youdef || canseemon(mdef));
	struct permonst *pd, *pa;
	struct obj *otmp;
	
	pd = youdef ? youracedata : mdef->data;
	pa = youagr ? youracedata : magr->data;
	// Defender is already dead
	if(DEADMONSTER(mdef))
		return MM_MISS;
	//Check curse resistance
	if(Curse_res(mdef, TRUE))
		return MM_MISS;
	if(youdef){
		IMPURITY_UP(u.uimp_curse)
	}
	//Roll type
	switch(magr->mtyp){
		case PM_HMNYW_PHARAOH:
			cnum = 5 + rnd(3);
		break;
		case PM_PHARAOH:
			cnum = 3 + rnd(3);
		break;
		case PM_PRIEST_MUMMY:
			cnum = rnd(5);
		break;
		case PM_ANCIENT_OF_THE_BURNING_WASTES:
			cnum = 5;
		break;
		case PM_SOLDIER_MUMMY:
			cnum = rnd(3);
		break;
		case PM_STRANGER:
			if(rn2(5))
				cnum = rn2(4);
			else
				cnum = 8;
		break;
		case PM_KUO_TOA:
			cnum = 9;
		break;
		case PM_KUO_TOA_WHIP:
			cnum = rn2(20) ? 9 : 10;
		break;
		case PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH:
			cnum = 10;
		break;
		default:
			cnum = 0;
		break;
	}
	//Do curse
	switch(cnum){
		//Minor damage
		case 0:
			//Should never kill target
			*hp(mdef) = max_ints(*hp(mdef) - (magr->data->mlevel), *hp(mdef)/2+1);
			if (youdef){
				stop_occupation();
				flags.botl = 1;
				You_feel("intense pain!");
			}
		break;
		//Bad Luck
		case 1:
			if(youdef){
				if(magr->mtyp == PM_SOLDIER_MUMMY){
					You_feel("an ill fate swirl around you.");
					change_luck(-rnd(7));
				} else {
					You_feel("an ill fate settle over you.");
					change_luck(-26);
				}
			}
			else {
				if(magr->mtyp == PM_SOLDIER_MUMMY)
					mdef->encouraged = max_ints(-13, mdef->encouraged-rnd(7));
				else
					mdef->encouraged = max(-13, mdef->encouraged-26);
			}
		break;
		//Curse Equipment
		case 2:{
			boolean messaged = FALSE;
			for (otmp = youdef ? invent : mdef->minvent; otmp; otmp = otmp->nobj) {
#ifdef GOLDOBJ
				if (otmp->oclass == COIN_CLASS) continue;
#endif
				if (otmp->cursed || rn2(4)) continue;

				if(otmp->oartifact && arti_gen_prop(otmp, ARTG_MAJOR) &&
				   rn2(10) < 8) {
					if (visible) pline("%s!", Tobjnam(otmp, "resist"));
					continue;
				}
				if(!messaged){
					if(youdef)
						You_feel("as if you need some help.");
					else if(youagr && visible)
						You_feel("as though %s needs some help.", mon_nam(mdef));
					
					messaged = TRUE;
				}
				if(otmp->blessed)
					unbless(otmp);
				else
					curse(otmp);
			}
			update_inventory();
		}
		break;
		//Pain
		case 3:
			//Should never kill target
			*hp(mdef) = *hp(mdef)/2 + 1;
			if (youdef) {
				stop_occupation();
				flags.botl = 1;
				if (!is_silent(pd)){
					You("%s from the pain!", humanoid_torso(pd) ? "scream" : "shriek");
				}
				else {
					You("writhe in pain!");
				}
				HScreaming += 2;
			}
			else {
				if (!is_silent_mon(mdef)){
					if (canseemon(mdef))
						pline("%s %s in pain!", Monnam(mdef), humanoid_torso(mdef->data) ? "screams" : "shrieks");
					else You_hear("%s %s in pain!", mdef->mtame ? noit_mon_nam(mdef) : mon_nam(mdef), humanoid_torso(mdef->data) ? "screaming" : "shrieking");
				}
				else {
					if (canseemon(mdef))
						pline("%s writhes in pain!", Monnam(mdef));
				}
			}
		break;
		//Insect (sickness)
		case 4:
			if(Sick_res(mdef))
				break;
			if(youdef){
				if (!umechanoid) {
					You("are stung by a tiny insect!");
					make_sick(Sick ? Sick / 3L + 1L : (long)rn1(ACURR(A_CON), 20),
						pa->mname, TRUE, SICK_NONVOMITABLE);
				}
			}
			else {
				/* 1/10 chance of instakill */
				if (!rn2(10)){
					if (youagr) killed(mdef);
					else monkilled(mdef, "", AD_SPEL);
					/* instakill */
					return ((*hp(mdef) > 0 ? MM_DEF_LSVD : MM_DEF_DIED) | MM_HIT);
				}
				else {
					return xdamagey(magr, mdef, (struct attack *)0, rnd(12));
				}
			}
		break;
		//Mummy Rot
		case 5:
			if (youdef) {
				if(!u.umummyrot){
					You("begin crumbling to dust!");
					u.umummyrot = TRUE;
				}
				else {
					if(rn2(2))
						(void)adjattrib(A_CON, -rnd(6), FALSE);
					else
						(void)adjattrib(A_CHA, -rnd(6), FALSE);
				}
			}
			else {
				if(visible)
					pline("%s is crumbling to dust!", Monnam(mdef));
				return xdamagey(magr, mdef, (struct attack *)0, d(d(2,6), 10));
			}
		break;
		//Heart Attack
		case 6:
			if (!nonliving(pd) || !has_blood_mon(mdef) || (youdef && (u.sealsActive & SEAL_OSE)) || resists_death(mdef))
				break;
			else if (*hp(mdef) >= 100){
				*hp(mdef) -= d(10, 8);
				stop_occupation();
				flags.botl = 1;
				if(youdef)
					Your("%s stops!  When it finally beats again, it is weak and thready.", body_part(HEART));
			}
			else {
				if(youdef){
					killer_format = KILLED_BY_AN;
					killer = "heart attack";
					if (!u.uconduct.killer && !youagr){
						//Pcifist PCs aren't combatants so if something kills them up "killed peaceful" type impurities
						IMPURITY_UP(u.uimp_murder)
						IMPURITY_UP(u.uimp_bloodlust)
					}
					done(DIED);
				}
				else {
					if (is_delouseable(pd)){
						pline("The parasite dies!");
						delouse(mdef, AD_DEAD);
					}
					else {
						mdef->mhp = -1;
						if (youagr) killed(mdef);
						else monkilled(mdef, "", AD_SPEL);
					}
					return ((*hp(mdef)>0 ? MM_DEF_LSVD : MM_DEF_DIED) | MM_HIT);
				}
			}
		break;
		//Remove Protection
		case 7:
			if(youdef){
				You_feel("a dire fate settle over you.");
				u.uacinc = min(u.uacinc-4, u.uacinc/2);
				u.ublessed = max(0, min(u.ublessed-4, u.ublessed/2));
			}
			else {
				//Monsters are less detailed, just do luck effect again.
				mdef->encouraged = max(-13, mdef->encouraged-26);
			}
		break;
		//Remove HP
		case 8:
			if(youdef){
				u.uhpbonus = min(u.uhpbonus-25, u.uhpbonus/2);
				calc_total_maxhp();
			}
			else {
				mdef->mhpmax = max(mdef->m_lev, max(mdef->mhpmax-25, mdef->mhpmax/2+1));
				mdef->mhp = min(mdef->mhp, mdef->mhpmax);
			}
			//Should never kill target
			*hp(mdef) = *hp(mdef)/3 + 1;
			if (youdef) {
				stop_occupation();
				flags.botl = 1;
				if (!is_silent(pd)){
					You("%s in agony!", humanoid_torso(pd) ? "scream" : "shriek");
				}
				else {
					You("writhe in agony!");
				}
				HScreaming += 2;
			}
			else {
				if (!is_silent_mon(mdef)){
					if (canseemon(mdef))
						pline("%s %s in agony!", Monnam(mdef), humanoid_torso(mdef->data) ? "screams" : "shrieks");
					else You_hear("%s %s in agony!", mdef->mtame ? noit_mon_nam(mdef) : mon_nam(mdef), humanoid_torso(mdef->data) ? "screaming" : "shrieking");
				}
				else {
					if (canseemon(mdef))
						pline("%s writhes in agony!", Monnam(mdef));
				}
			}
		break;
		//Liquid Sparks
		case 9:{
			struct obj *armh = youdef ? uarmh : which_armor(mdef, W_ARMH);
			int dmg;
			if(armh && is_wide_helm(armh) && !is_metallic(uarmh)){
				if(youdef){
					if(!Blind)
						pline("Bright liquid sparks rain down on your %s.", xname(armh));
				}
				else {
					if(canseemon(mdef))
						pline("Bright liquid sparks rain down on %s %s.", s_suffix(mon_nam(mdef)), xname(armh));
				}
				break;
			}
			else if(Water_res(mdef) || Shock_res(mdef)){
				shieldeff(x(mdef), y(mdef));
				if(youdef){
					pline("Bright liquid sparks bounce off your %s.", body_part(HEAD));
				}
				else {
					if(canseemon(mdef))
						pline("Bright liquid sparks bounce off %s %s.", s_suffix(mon_nam(mdef)) , mbodypart(mdef, HEAD));
				}
				break;
			}
			//else
			if(youdef){
				pline("Bright liquid sparks smite your %s!", body_part(HEAD));
			}
			else {
				if(canseemon(mdef))
					pline("Bright liquid sparks smite %s %s!", s_suffix(mon_nam(mdef)) , mbodypart(mdef, HEAD));
			}
			dmg = d(min((mlev(magr)+2)/3, MAX_BONUS_DICE), 6);
			dmg = reduce_dmg(mdef,dmg,FALSE,TRUE);

			/* damage inventory */
			if (!UseInvShock_res(mdef)){
				destroy_item(mdef, WAND_CLASS, AD_ELEC);
			}
			water_damage(youdef ? invent : mdef->minvent, FALSE, FALSE, FALSE, mdef);
			if(youdef)
				losehp(dmg, "lightning sparks", KILLED_BY);
			else
				m_losehp(mdef, dmg, youagr, "lightning sparks");
		}break;
		//Liquid Bolt
		case 10:{
			struct obj *armh = youdef ? uarmh : which_armor(mdef, W_ARMH);
			int dmg;
			int resistances = 0;
			if(armh && is_wide_helm(armh) && !is_metallic(uarmh)){
				if(youdef){
					if(!Blind)
						pline("A bright surging torrent pours down on your %s!", xname(armh));
				}
				else {
					if(canseemon(mdef))
						pline("A bright surging torrent pours down on %s %s!", s_suffix(mon_nam(mdef)), xname(armh));
				}
				resistances++;
			}
			else {
				if(youdef){
					pline("A bright surging torrent pours down on your %s!", body_part(HEAD));
				}
				else {
					if(canseemon(mdef))
						pline("A bright surging torrent pours down on %s %s!", s_suffix(mon_nam(mdef)) , mbodypart(mdef, HEAD));
				}
			}
			if(Water_res(mdef)){
				resistances++;
			}
			if(Shock_res(mdef)){
				resistances++;
			}
			if(resistances > 2){
				shieldeff(x(mdef), y(mdef));
				if(youdef){
					You("seem unharmed.");
				}
				else {
					if(canseemon(mdef))
						pline("%s seems unharmed.", Monnam(mdef));
				}
				break;
			}
			//else
			dmg = d(min((mlev(magr)+2)/3, MAX_BONUS_DICE), 12);
			if (resistances == 2)
				dmg = (dmg + 3) / 4;
			else if (resistances == 1)
				dmg = (dmg + 1) / 2;
			
			dmg = reduce_dmg(mdef,dmg,FALSE,TRUE);

			/* damage inventory */
			if (!UseInvShock_res(mdef)){
				destroy_item(mdef, WAND_CLASS, AD_ELEC);
			}
			if(!Water_res(mdef) && !(armh && is_wide_helm(armh)))
				water_damage(youdef ? invent : mdef->minvent, FALSE, FALSE, FALSE, mdef);
			if(youdef)
				losehp(dmg, "liquid lightning", KILLED_BY);
			else
				m_losehp(mdef, dmg, youagr, "liquid lightning");
		}break;
	}
	return MM_MISS;
}

int
reduce_dmg(mdef,dmg,physical,magical)
struct monst *mdef;
int dmg;
boolean physical;
boolean magical;
{
	if (physical && Half_phys(mdef))
		dmg = (dmg + 1) / 2;
	if (magical && Half_spel(mdef))
		dmg = (dmg + 1) / 2;
	if (mdef->mtyp == PM_CENTER_OF_ALL && Insight < 32)
		dmg = (dmg + 1) / 2;
	if (mdef == &youmonst && u.uvaul_duration){
		if(physical && magical)
			dmg = (dmg + 3) / 4;
		else
			dmg = (dmg + 1) / 2;
	}
	if (mdef == &youmonst && u.uspellprot){
		if(magical && artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_WILL)
			dmg = (dmg + 1) / 2;
	}
	/* Priests of Asmodeus */
	if(mdef != &youmonst && flags.spriest_level && is_demon(mdef->data) && is_lawful_mon(mdef) && !mdef->mpeaceful)
		dmg = (dmg + 1) / 2;
	return dmg;
}
