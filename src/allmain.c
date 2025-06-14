/*	SCCS Id: @(#)allmain.c	3.4	2003/04/02	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* various code that was replicated in *main.c */

#include <math.h>
#include <time.h>
#include <errno.h>    
#include "hack.h"
#include "artifact.h"
#include "xhity.h"
#include "hashmap.h"
#include "hashutil.h"

#ifndef NO_SIGNAL
#include <signal.h>
#endif

#ifdef POSITIONBAR
STATIC_DCL void NDECL(do_positionbar);
#endif

STATIC_DCL void NDECL(mercurial_repair);
STATIC_DCL void NDECL(clothes_bite_you);
STATIC_DCL void NDECL(androidUpkeep);
STATIC_DCL void NDECL(printMons);
STATIC_DCL void NDECL(printMonNames);
STATIC_DCL void NDECL(printDPR);
STATIC_DCL void NDECL(printBodies);
STATIC_DCL void NDECL(printSanAndInsight);
STATIC_DCL void FDECL(printAttacks, (char *,struct permonst *));
STATIC_DCL void FDECL(resFlags, (char *,unsigned int));
STATIC_DCL int FDECL(find_preset_inherited, (char *));
STATIC_DCL int NDECL(do_inheritance_menu);
STATIC_DCL void FDECL(spot_monster, (struct monst *));
STATIC_DCL void NDECL(sense_nearby_monsters);
STATIC_DCL void NDECL(cthulhu_mind_blast);
STATIC_DCL void FDECL(unseen_actions, (struct monst *));
STATIC_DCL void FDECL(blessed_spawn, (struct monst *));
STATIC_DCL void FDECL(alkilith_spawn, (struct monst *));
STATIC_DCL void FDECL(incarnator_action, (struct monst *));
STATIC_DCL void FDECL(befouled_wraith, (struct monst *));
STATIC_DCL void FDECL(good_neighbor, (struct monst *));
STATIC_DCL void FDECL(dark_pharaoh, (struct monst *));
STATIC_DCL void FDECL(dark_pharaoh_visible, (struct monst *));
STATIC_DCL void FDECL(good_neighbor_visible, (struct monst *));
STATIC_DCL void FDECL(polyp_pickup, (struct monst *));
STATIC_DCL void FDECL(unbodied_heal, (struct monst *));
STATIC_DCL void FDECL(goat_sacrifice, (struct monst *));
STATIC_DCL void FDECL(palid_stranger, (struct monst *));
STATIC_DCL void FDECL(sib_follow, (struct monst *));
STATIC_DCL void FDECL(invisible_twin_act, (struct monst *));
void FDECL(make_rage_walker_polts, (int));

#ifdef OVL0

extern const int monstr[];

/* gosleep(): no more than 100 inputs per second */
int gosleep()
{
    struct timespec ts = {0, 10*1000*1000};
    // struct timespec ts = {1, 0};
    int res;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

int
filter_itemmap(void* const context, struct hashmap_element_s* const e)
{
	struct menucolor_attribs *stored = (struct menucolor_attribs *)e->data;
	if((moves - stored->lastused) > 1000){
		free(stored->key);
		free(e->data);
		return -1;
	}
	return 0;
}

STATIC_OVL void
digcrater(mtmp)
	struct monst *mtmp;
{
	int x,y,i,j;
	struct trap *ttmp;
	for(i=-5;i<=5;i++){
		x = mtmp->mx+i;
		for(j=-5;j<=5;j++){
			y = mtmp->my+j;
			if(isok(x,y)&& !(x == u.ux && y == u.uy)){
				ttmp = t_at(x, y);
				if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
					if(dist2(x,y,mtmp->mx,mtmp->my) <= 26){
						levl[x][y].typ = CORR;
						if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
						if(ttmp) delfloortrap(ttmp);
						if(dist2(x,y,mtmp->mx,mtmp->my) <= 9) levl[x][y].typ = LAVAPOOL;
						else if(dist2(x,y,mtmp->mx,mtmp->my) <= 26) digactualhole(x, y, mtmp, PIT, TRUE, FALSE);
					}
				}
			}
		}
	}
}

STATIC_OVL void
digcloudcrater(mtmp)
	struct monst *mtmp;
{
	int x,y,i,j;
	struct trap *ttmp;
	for(i=-2;i<=2;i++){
		x = mtmp->mx+i;
		for(j=-2;j<=2;j++){
			y = mtmp->my+j;
			if(isok(x,y) && dist2(x,y,mtmp->mx,mtmp->my) < 8){
				levl[x][y].typ = CLOUD;
			}
		}
	}
}

STATIC_OVL void
digXchasm(mtmp)
	struct monst *mtmp;
{
	int x,y;
	struct trap *ttmp;
	int inty = rn2(4)+2;
	int dy = rn2(2) ? 1 : -1;
	y = mtmp->my;
	x = mtmp->mx;
	if(isok(x,y)&& !(x == u.ux && y == u.uy)){
		ttmp = t_at(x, y);
		if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
			levl[x][y].typ = CORR;
			if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
			if(ttmp) delfloortrap(ttmp);
			digactualhole(x, y, mtmp, HOLE, TRUE, FALSE);
		}
	}
	y=y+1;
	if(isok(x,y)){
		ttmp = t_at(x, y);
		if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
			levl[x][y].typ = CORR;
			if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
			if(ttmp) delfloortrap(ttmp);
			digactualhole(x, y, mtmp, PIT, TRUE, FALSE);
		}
	}
	y=y-1;
	y=y-1;
	if(isok(x,y)){
		ttmp = t_at(x, y);
		if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
			levl[x][y].typ = CORR;
			if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
			if(ttmp) delfloortrap(ttmp);
			digactualhole(x, y, mtmp, PIT, TRUE, FALSE);
		}
	}
	y=y+1;
	for(x = mtmp->mx + 1; x < COLNO; x++){
		if(!(x%inty)) y += dy;
		if(isok(x,y) && !(x == u.ux && y == u.uy)){
			ttmp = t_at(x, y);
			if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
				levl[x][y].typ = CORR;
				if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
				if(ttmp) delfloortrap(ttmp);
				digactualhole(x, y, mtmp, HOLE, TRUE, FALSE);
			}
		}
		y=y+1;
		if(isok(x,y)){
			ttmp = t_at(x, y);
			if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
				levl[x][y].typ = CORR;
				if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
				if(ttmp) delfloortrap(ttmp);
				digactualhole(x, y, mtmp, PIT, TRUE, FALSE);
			}
		}
		y=y-1;
		y=y-1;
		if(isok(x,y)){
			ttmp = t_at(x, y);
			if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
				levl[x][y].typ = CORR;
			if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
				if(ttmp) delfloortrap(ttmp);
				digactualhole(x, y, mtmp, PIT, TRUE, FALSE);
			}
		}
		y=y+1;
	}
	y = mtmp->my;
	for(x = mtmp->mx - 1; x >= 0; x--){
		if(!(x%inty)) y -= dy;
		if(isok(x,y)&& !(x == u.ux && y == u.uy)){
			ttmp = t_at(x, y);
			if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
				levl[x][y].typ = CORR;
			if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
				if(ttmp) delfloortrap(ttmp);
				digactualhole(x, y, mtmp, HOLE, TRUE, FALSE);
			}
		}
		y=y+1;
		if(isok(x,y)){
			ttmp = t_at(x, y);
			if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
				levl[x][y].typ = CORR;
			if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
				if(ttmp) delfloortrap(ttmp);
				digactualhole(x, y, mtmp, PIT, TRUE, FALSE);
			}
		}
		y=y-1;
		y=y-1;
		if(isok(x,y)){
			ttmp = t_at(x, y);
			if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
				levl[x][y].typ = CORR;
			if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
				if(ttmp) delfloortrap(ttmp);
				digactualhole(x, y, mtmp, PIT, TRUE, FALSE);
			}
		}
		y=y+1;
	}
}

STATIC_OVL void
digYchasm(mtmp)
	struct monst *mtmp;
{
	int x,y;
	struct trap *ttmp;
	int intx = rn2(4)+2;
	int dx = rn2(2) ? 1 : -1;
	x = mtmp->mx;
	y = mtmp->my;
	if(isok(x,y)&& !(x == u.ux && y == u.uy)){
		ttmp = t_at(x, y);
		if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
			levl[x][y].typ = CORR;
			if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
			if(ttmp) delfloortrap(ttmp);
			digactualhole(x, y, mtmp, HOLE, TRUE, FALSE);
		}
	}
	x=x+1;
	if(isok(x,y)){
		ttmp = t_at(x, y);
		if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
			levl[x][y].typ = CORR;
			if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
			if(ttmp) delfloortrap(ttmp);
			digactualhole(x, y, mtmp, PIT, TRUE, FALSE);
		}
	}
	x=x-1;
	x=x-1;
	if(isok(x,y)){
		ttmp = t_at(x, y);
		if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
			levl[x][y].typ = CORR;
			if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
			if(ttmp) delfloortrap(ttmp);
			digactualhole(x, y, mtmp, PIT, TRUE, FALSE);
		}
	}
	x=x+1;
	for(y = mtmp->my + 1; y < COLNO; y++){
		if(!(y%intx)) x += dx;
		if(isok(x,y)&& !(x == u.ux && y == u.uy)){
			ttmp = t_at(x, y);
			if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
				levl[x][y].typ = CORR;
			if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
				if(ttmp) delfloortrap(ttmp);
				digactualhole(x, y, mtmp, HOLE, TRUE, FALSE);
			}
		}
		x=x+1;
		if(isok(x,y)){
			ttmp = t_at(x, y);
			if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
				levl[x][y].typ = CORR;
			if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
				if(ttmp) delfloortrap(ttmp);
				digactualhole(x, y, mtmp, PIT, TRUE, FALSE);
			}
		}
		x=x-1;
		x=x-1;
		if(isok(x,y)){
			ttmp = t_at(x, y);
			if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
				levl[x][y].typ = CORR;
			if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
				if(ttmp) delfloortrap(ttmp);
				digactualhole(x, y, mtmp, PIT, TRUE, FALSE);
			}
		}
		x=x+1;
	}
	x = mtmp->mx;
	for(y = mtmp->my - 1; y >= 0; y--){
		if(!(y%intx)) x -= dx;
		if(isok(x,y)&& !(x == u.ux && y == u.uy)){
			ttmp = t_at(x, y);
			if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
				levl[x][y].typ = CORR;
			if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
				if(ttmp) delfloortrap(ttmp);
				digactualhole(x, y, mtmp, HOLE, TRUE, FALSE);
			}
		}
		x=x+1;
		if(isok(x,y)){
			ttmp = t_at(x, y);
			if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
				levl[x][y].typ = CORR;
			if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
				if(ttmp) delfloortrap(ttmp);
				digactualhole(x, y, mtmp, PIT, TRUE, FALSE);
			}
		}
		x=x-1;
		x=x-1;
		if(isok(x,y)){
			ttmp = t_at(x, y);
			if(levl[x][y].typ <= SCORR || levl[x][y].typ == CORR || levl[x][y].typ == ROOM){
				levl[x][y].typ = CORR;
			if(!does_block(x,y,&levl[x][y])) unblock_point(x,y);	/* vision:  can see through */
				if(ttmp) delfloortrap(ttmp);
				digactualhole(x, y, mtmp, PIT, TRUE, FALSE);
			}
		}
		x=x+1;
	}
}
static int prev_hp_notify;

char *
hpnotify_format_str(char *str)
{
    static char buf[128];
    char *f, *p, *end;
    int ispercent = 0;

    buf[0] = '\0';

    if (!str) return NULL;

    f = str;
    p = buf;
    end = buf + sizeof(buf) - 10;

    while (*f) {
      if (ispercent) {
	switch (*f) {
	case 'a':
	    snprintf (p, end + 1 - p, "%d", abs(uhp()-prev_hp_notify));
	  while (*p != '\0')
	    p++;
	  break;
        case 'c':
	    snprintf (p, end + 1 - p, "%c", (prev_hp_notify > uhp() ? '-' : '+'));
	  p++;
	  break;
	case 'm':
	    snprintf (p, end + 1 - p, "%d", uhpmax());
	  while (*p != '\0')
	    p++;
	  break;
	case 'H':
	    if (uhp() == uhpmax()) {
	    snprintf (p, end + 1 - p, "%s", "max");
	  } else {
		snprintf (p, end + 1 - p, "%d", uhp());
	  }
	  while (*p != '\0')
	    p++;
	  break;
	case 'h':
	    snprintf (p, end + 1 - p, "%d", uhp());
	  while (*p != '\0')
	    p++;
	  break;
	default:
	  *p = *f;
	  if (p < end)
	    p++;
	}
	ispercent = 0;
      } else {
	if (*f == '%')
	  ispercent = 1;
	else {
	  *p = *f;
	  if (p < end)
	    p++;
	}
      }
      f++;
    }
    *p = '\0';

    return buf;
}

STATIC_OVL
void
androidUpkeep()
{
	//Pay unusual upkeep here, possibly pass out
	if(uandroid && !u.usleep){
		int mult = HEALCYCLE/u.ulevel;
		//Possibly pass out if you begin this step with 0 energy.
		if(u.uen == 0 && !rn2(10+u.ulevel) && moves >= u.nextsleep){
			int t = rn1(u.uenmax*mult+40, u.uenmax*mult+40);
			You("pass out from exhaustion!");
			u.nextsleep = moves+rnz(350)+t;
			u.lastslept = moves;
			fall_asleep(-t, TRUE);
			nomul(-1*u.uenmax, "passed out from exhaustion");
		}
		if(u.phasengn){
			losepw(10);
			if(u.uen <= 0){
				You("can no longer maintain phase!");
				u.phasengn = 0;
			}
		}
		if(u.ucspeed == HIGH_CLOCKSPEED){
			losepw(1);
			if(u.uen <= 0){
				You("can no longer maintain emergency speed!");
				u.ucspeed = NORM_CLOCKSPEED;
			}
		}
		//Exceeding operational time
		static char fatigue_warning = 0;
		if(moves >= u.nextsleep+1400 && u.uen > 10 && !fatigue_warning){
			fatigue_warning = 1;
			You_feel("fatigued.");
		}
		static char e_fatigued = 0;
		if(moves >= u.nextsleep+1400 && u.uen > 1 && u.uen <= 10 && !e_fatigued){
			e_fatigued = 1;
			You_feel("extremely fatigued.");
		}
		static char pass_warning = 0;
		if(moves >= u.nextsleep && u.uen <= 1 && !pass_warning){
			pass_warning = 1;
			You_feel("like you're about to pass out.");
		}
		if(moves > u.nextsleep+1400 && u.uen > 0){
			if(!(moves%20)) losepw(1);
		}
	}
}

STATIC_OVL
void
mercurial_repair()
{
	struct obj * uequip[] = WORN_SLOTS;
	int i;
	if(monstermoves%HEALCYCLE)
		return;
	int regto;
	if(u.ulevel < 3)
		regto = 0;
	else if(u.ulevel < 10)
		regto = 1;
	else if(u.ulevel < 18)
		regto = 2;
	else regto = 3;

	for (i = 0; i < SIZE(uequip); i++) {
		if (uequip[i] && obj_is_material(uequip[i], MERCURIAL)
			&& uequip[i]->spe < regto && is_enchantable(uequip[i])
			&& rnd(20) < ACURR(A_CHA)
		){
			uequip[i]->spe++;
		}
	}
}

STATIC_OVL
void
clear_stale_fforms()
{
	for(int fform = NO_FFORM+1; fform <= LAST_FFORM; fform++){
		if (!!(u.fightingForm[(fform-1)/16] & (0x1L << ((fform-1)%16))) && fform != FFORM_SHII_CHO && FightingFormSkillLevel(fform) <= P_ISRESTRICTED){
			unSetFightingForm(fform);
			You("readjust your stance.");
		}
	}
}


STATIC_OVL
void
clothes_bite_you()
{
	struct obj * uequip[] = WORN_SLOTS;
	int i;
	for (i = 0; i < SIZE(uequip); i++) {
		if (uequip[i] && (uequip[i]->olarva || uequip[i]->obyak || check_oprop(uequip[i], OPROP_BYAK))) {
			held_item_bites(&youmonst, uequip[i]);
			if(multi >= 0) {
				if (occupation)
					stop_occupation();
				else
					nomul(0, NULL);
			}
		}
	}
}

static long prev_dgl_extrainfo = 0;

/* returns the movement point cost of the requested action type 
 * affect_game_state should only be TRUE when called from the
 * main game loop
 */
int
you_action_cost(actiontype, affect_game_state)
int actiontype;
boolean affect_game_state;
{
	int actiontypes_remaining = actiontype; 
	int current_action, current_cost, highest_cost = 0;
	int i;

	/* 1. ignore the MOVE_FINISHED_OCCUPATION flag, it only says to end occupations
	 * 2. if no other types are specified 'default' means 'standard', otherwise is ignored
	 * 3. exceptions to 2. 'Partial' |'d with no others is Partial|Standard
	 */
	actiontypes_remaining &= ~MOVE_FINISHED_OCCUPATION;
	actiontypes_remaining = !(actiontypes_remaining & ~(MOVE_DEFAULT|MOVE_PARTIAL)) ? MOVE_STANDARD : (actiontypes_remaining&(~MOVE_DEFAULT));
	actiontypes_remaining |= (actiontype&MOVE_PARTIAL);

	/* loop through all flagged action types to determine which is the largest cost,
	 * and, if affect_game_state is TRUE, apply all necessary effects.
	 * 
	 * Most often, there is only one action type per player input.
	 * In very rare circumstances, two actions that should take time can happen simultaneously.
	 */
	//if(wizard && actiontypes_remaining != MOVE_CANCELLED && (actiontypes_remaining&MOVE_CANCELLED) && affect_game_state)
	//	impossible("Incompletely cancelled actions in you_action_cost: %d", actiontypes_remaining-MOVE_CANCELLED);
	for (i=0, current_action = MOVE_STANDARD;
		actiontypes_remaining != 0;
		current_action = 1<<i, i++)
	{
		if (!(actiontypes_remaining & current_action))
			continue;
		else
			actiontypes_remaining -= current_action;

		current_cost = NORMAL_SPEED;

		switch(current_action) {
		case MOVE_STANDARD:
			/* the default for any not-otherwise-handled non-instant action */
			break;

		case MOVE_INSTANT:
			current_cost = 0;
			break;
		
		case MOVE_PARTIAL:
			if (affect_game_state ? partial_action() : check_partial_action()) {
				if (affect_game_state)
					flags.movetoprint &= ~MOVE_PARTIAL;
				current_cost = highest_cost;
			}
			else {
				if (affect_game_state)
					flags.movetoprint |= MOVE_PARTIAL;
				return 0;
			}
			break;

		case MOVE_MOVED:
			/* to determine the fastest movement booster applicable */
#define MOVECOST(val) current_cost = min(val, current_cost)

			/* these only apply if you didn't attack this action */
			if (!u.uattked) {
				if(uwep && uwep->oartifact == ART_TENSA_ZANGETSU && !flags.beginner){
					MOVECOST(NORMAL_SPEED/12);
				} else if(uwep && uwep->oartifact == ART_SODE_NO_SHIRAYUKI && u.ulevel >= 14){
					MOVECOST(NORMAL_SPEED/4);
				} else if(uwep && uwep->oartifact == ART_TOBIUME && u.ulevel >= 14){
					if (affect_game_state) {
						if((HStealth&TIMEOUT) < 2)
							set_itimeout(&HStealth, 2L);
						if((HInvis&TIMEOUT) < 2){
							set_itimeout(&HInvis, 2L);
							newsym(u.ux, u.uy);
						}
					}
					MOVECOST(NORMAL_SPEED/4);
				}
				if(uandroid && u.ucspeed == HIGH_CLOCKSPEED){
					MOVECOST(NORMAL_SPEED/3);
				}
				if(uarmf && uarmf->oartifact == ART_SEVEN_LEAGUE_BOOTS){
					MOVECOST(NORMAL_SPEED - artinstance[ART_SEVEN_LEAGUE_BOOTS].LeagueMod);
					if(affect_game_state && artinstance[ART_SEVEN_LEAGUE_BOOTS].LeagueMod < 5*NORMAL_SPEED/6){
						artinstance[ART_SEVEN_LEAGUE_BOOTS].LeagueMod += NORMAL_SPEED/6;
					}
				}
				if((uarmf && uarmf->oartifact == ART_FENG_HUO_LUN)
				 || (uwep && uwep->oartifact == ART_FENG_HUO_LUN)
				){
					MOVECOST(NORMAL_SPEED - artinstance[ART_FENG_HUO_LUN].LeagueMod);
					if(affect_game_state && artinstance[ART_FENG_HUO_LUN].LeagueMod < NORMAL_SPEED/2){
						artinstance[ART_FENG_HUO_LUN].LeagueMod += NORMAL_SPEED/4;
					}
				}
			}
#undef MOVECOST
			break;

		case MOVE_ATTACKED:
			current_cost = NORMAL_SPEED;

			/* some weapons are faster */
			/*  Note: Fire brand is a fast weapon, and works with frost brand in the main hand */
			if ((uwep && fast_weapon(uwep))
				|| (!uwep && uarmg && fast_weapon(uarmg))
				|| (uwep && uwep->oartifact == ART_FROST_BRAND && u.twoweap && uswapwep && uswapwep->oartifact == ART_FIRE_BRAND && fast_weapon(uswapwep))
			){
				current_cost -= NORMAL_SPEED / 6;
			}
			if ((uwep && CHECK_ETRAIT(uwep, &youmonst, ETRAIT_QUICK))
				&& (!u.twoweap || (uswapwep && CHECK_ETRAIT(uswapwep, &youmonst, ETRAIT_QUICK)))
			){
				current_cost -= NORMAL_SPEED / ROLL_ETRAIT(uwep, &youmonst, 3, 4);
			}

			/* some weapons are slower */
			if (uwep
				&& (uwep->otyp == RAKUYO || uwep->otyp == DOUBLE_FORCE_BLADE || uwep->otyp == DOUBLE_SWORD || uwep->otyp == RAZOR_CLEAVER
					|| uwep->otyp == BLADED_BOW
					|| ((uwep->otyp == DOUBLE_LIGHTSABER || uwep->otyp == BEAMSWORD || uwep->otyp == LIGHTSABER || uwep->otyp == ROD_OF_FORCE) && uwep->altmode)
				)
				&& !u.twoweap
				){
				current_cost += NORMAL_SPEED / 4;
			}

			/* some artifacts are faster */
			// note - you don't have to actually be two-weaponing, and that's intentional,
			// but you must have another ARTA_HASTE wep offhanded (and there's only 2 so)
			if (uwep && uwep->oartifact && arti_attack_prop(uwep, ARTA_HASTE)){
				current_cost -= NORMAL_SPEED / 3;
				if (uswapwep && uswapwep->oartifact && arti_attack_prop(uwep, ARTA_HASTE))
					current_cost -= NORMAL_SPEED / 6;
			}
			break;

		case MOVE_QUAFFED:
			break;
		
		case MOVE_ZAPPED:
			if (QuickDraw) {
				current_cost = you_action_cost(MOVE_PARTIAL, affect_game_state);
			}
			break;

		case MOVE_READ:
			break;
		
		case MOVE_CASTSPELL:
			break;

		case MOVE_ATE:
			break;

		case MOVE_FIRED:
			break;

		case MOVE_CANCELLED:
			current_cost = 0;
			break;

		case MOVE_CONTAINER:
			if(QuickDraw)
				current_cost = 0;
			break;

		default:
			impossible("Unhandled MOVE_XYZ case (%d)", current_action);
			break;
		}
		/* assign as new highest cost, if applicable */
		highest_cost = max(current_cost, highest_cost);
	}

	return highest_cost;
}

void
you_calc_movement()
{
	int moveamt = 0;
	int nmonsclose,nmonsnear,enkispeedlim;
	int wtcap = near_capacity();
	struct monst *mtmp;
	
#ifdef STEED
	if (u.usteed && u.umoved) {
	/* your speed doesn't augment steed's speed */
	moveamt = mcalcmove(u.usteed);
	moveamt += P_SKILL(P_RIDING)-1; /* -1 to +3 */
	if(uclockwork){
		if(u.ucspeed == HIGH_CLOCKSPEED){
			/*You are still burning spring tension, even if it doesn't affect your speed!*/
			if(u.slowclock < 3) morehungry(3-u.slowclock);
			else if(!TimeStop && !(moves%(u.slowclock - 2))) morehungry(1);
		}
		if(u.phasengn){
			//Phasing mount as well
			morehungry(10);
		}
	}
	} else
#endif
	{
	//Override species-based speed penalties in some cases.
	if(!Upolyd && (
		Race_if(PM_GNOME)
		|| Race_if(PM_DWARF)
		|| Race_if(PM_ORC)
		|| (Race_if(PM_HALF_DRAGON) && Humanoid_half_dragon(urole.malenum))
	)) moveamt = 12;
	else moveamt = youmonst.data->mmove;
	if(Race_if(PM_HALF_DRAGON) && flags.HDbreath == AD_ELEC && u.ulevel >= 15)
		moveamt += 3;
	if(uarmf && !Flying && !Levitation){
		if(uarmf->otyp == STILETTOS)
			moveamt = (moveamt*5)/6;
		else if(uarmf->otyp == WIND_AND_FIRE_WHEELS)
			moveamt = (moveamt+1)/2;
	}
	
	if(u.sealsActive&SEAL_EURYNOME && IS_PUDDLE_OR_POOL(levl[u.ux][u.uy].typ)){
		if (Very_fast) {	/* speed boots or potion */
			/* average movement is 1.78 times normal */
			moveamt += 2*NORMAL_SPEED / 3;
			if (rn2(3) == 0) moveamt += NORMAL_SPEED / 3;
		} else if (Fast) {
			/* average movement is 1.56 times normal */
			moveamt += NORMAL_SPEED / 3;
			if (rn2(3) != 0) moveamt += NORMAL_SPEED / 3;
		} else {
			/* average movement is 1.33 times normal */
			if (rn2(3) != 0) moveamt += NORMAL_SPEED / 2;
		}
	} else {
		if (Very_fast) {	/* speed boots or potion */
			/* average movement is 1.67 times normal */
			moveamt += NORMAL_SPEED / 2;
			if (rn2(3) == 0) moveamt += NORMAL_SPEED / 2;
		} else if (Fast) {
			/* average movement is 1.33 times normal */
			if (rn2(3) != 0) moveamt += NORMAL_SPEED / 2;
		}
	}
	
	//Run away!
	if(Panicking){
		moveamt += 2;
	}
	
	if(uwep && uwep->oartifact == ART_SINGING_SWORD && uwep->osinging == OSING_HASTE){
		moveamt += 2;
	}
	
	if(u.specialSealsActive&SEAL_BLACK_WEB && u.utrap && u.utraptype == TT_WEB)
		moveamt += 8;
	
	if(u.sealsActive&SEAL_ENKI){
		nmonsclose = nmonsnear = 0;
		for (mtmp = fmon; mtmp; mtmp = mtmp->nmon){
			if(mtmp->mpeaceful) continue;
			if(distmin(u.ux, u.uy, mtmp->mx,mtmp->my) <= 1){
				nmonsclose++;
				nmonsnear++;
			} else if(distmin(u.ux, u.uy, mtmp->mx,mtmp->my) <= 2){
				nmonsnear++;
			}
		}
		enkispeedlim = u.ulevel/10+1;
		if(nmonsclose>1){
			moveamt += min(enkispeedlim,nmonsclose);
			nmonsnear -= min(enkispeedlim,nmonsclose);
		}
		if(nmonsnear>3) moveamt += min(enkispeedlim,nmonsnear-2);
	}
	if (uwep && uwep->oartifact == ART_GARNET_ROD) moveamt += NORMAL_SPEED / 2;
	if (uwep && uwep->oartifact == ART_TENSA_ZANGETSU){
		if(u.ulevel >= 14)
			moveamt += NORMAL_SPEED;
		if(artinstance[ART_TENSA_ZANGETSU].ZangetsuSafe-- < 1){
			if(ublindf && ublindf->otyp == MASK && ublindf->corpsenm != NON_PM && is_undead(&mons[ublindf->corpsenm])){
				artinstance[ART_TENSA_ZANGETSU].ZangetsuSafe = mons[ublindf->corpsenm].mlevel;
				if(ublindf->oeroded3>=3){
					Your("mask shatters!");
					useup(ublindf);
				} else {
					Your("mask cracks.");
					ublindf->oeroded3++;
				}
			} else {
				artinstance[ART_TENSA_ZANGETSU].ZangetsuSafe = 0;
				losehp(5, "inadvisable haste", KILLED_BY);
				if (Upolyd) {
					if(!TimeStop && u.mhmax > u.ulevel && moves % 2){
						u.uhpmod--;
						calc_total_maxhp();
					}
				}
				else{
					if(!TimeStop && u.uhpmax > u.ulevel && moves % 2){
						u.uhpmod--;
						calc_total_maxhp();
					}
				}
//					if( !(moves % 5) )
				You_feel("your %s %s!", 
					body_part(BONES), rnd(6) ? body_part(CREAK) : body_part(CRACK));
				exercise(A_CON, FALSE);
				exercise(A_STR, FALSE);
				stop_occupation();
				nomul(0, NULL);
			}
		}
	}
	else if(!TimeStop && artinstance[ART_TENSA_ZANGETSU].ZangetsuSafe < u.ulevel && !(moves%10)) artinstance[ART_TENSA_ZANGETSU].ZangetsuSafe++;
	
	int chikhp = 0;
	if(uwep && uwep->otyp == CHIKAGE && obj_is_material(uwep, HEMARGYOS))
		chikhp += *hpmax(&youmonst);
	if(uswapwep && u.twoweap && uswapwep->otyp == CHIKAGE && obj_is_material(uswapwep, HEMARGYOS))
		chikhp += *hpmax(&youmonst);
	if(chikhp > 0){
		int hploss = 85*chikhp/1000;
		if(HPoison_resistance)
			hploss /= 2;
		if(hploss < 10)
			hploss = 10;
		losehp(hploss/10, "poisoned blood", KILLED_BY);
		if(hploss%10){
			if (((moves)*(hploss%10)) / 10 >((moves - 1)*(hploss%10)) / 10)
				losehp(1, "poisoned blood", KILLED_BY);
		}
		stop_occupation();
		nomul(0, NULL);
	}
	
	if (u.usleep && u.usleep < monstermoves && roll_madness(MAD_FORMICATION)) {
		multi = -1;
		nomovemsg = "The crawling bugs awaken you.";
	}
	if(uclockwork && u.ucspeed == HIGH_CLOCKSPEED){
		int hungerup;
		moveamt *= 2;
		hungerup = 2*moveamt/NORMAL_SPEED - 1;
		if(u.slowclock < hungerup) morehungry(hungerup-u.slowclock);
		else if(!TimeStop && !(moves%(u.slowclock - hungerup + 1))) morehungry(1);
	}
	if(uandroid && u.ucspeed == HIGH_CLOCKSPEED){
		if (rn2(3) != 0) moveamt += NORMAL_SPEED / 2;
	}
	if(active_glyph(ANTI_CLOCKWISE_METAMORPHOSIS))
		moveamt += 3;
	if(u.uuur_duration)
		moveamt += 6;
	if(uwep && is_lightsaber(uwep) && litsaber(uwep) && activeFightingForm(FFORM_SORESU)){
		// switch(min(P_SKILL(P_SORESU), P_SKILL(weapon_type(uwep)))){
			// case P_BASIC:       moveamt = max(moveamt-6,1); break;
			// case P_SKILLED:     moveamt = max(moveamt-4,1); break;
			// case P_EXPERT:      moveamt = max(moveamt-3,1); break;
		// }
		moveamt = max(moveamt-6,1);
	}
	if(youracedata->mmove){
		if(moveamt < 1) moveamt = 1;
	} else {
		if(moveamt < 0) moveamt = 0;
	}
	}
	
	if(uclockwork && u.phasengn){
		morehungry(10);
	}
	
	if(uclockwork && u.ucspeed == SLOW_CLOCKSPEED)
		moveamt /= 2; /*Even if you are mounted, a slow clockspeed affects how 
						fast you can issue commands to the mount*/
	
	switch (wtcap) {
	case UNENCUMBERED: break;
	case SLT_ENCUMBER: moveamt -= (moveamt / 4); break;
	case MOD_ENCUMBER: moveamt -= (moveamt / 2); break;
	case HVY_ENCUMBER: moveamt -= ((moveamt * 3) / 4); break;
	case EXT_ENCUMBER: moveamt -= ((moveamt * 7) / 8); break;
	default: break;
	}
	
	if(u.umadness&MAD_NUDIST && !BlockableClearThoughts && NightmareAware_Sanity < 100){
		int delta = NightmareAware_Insanity;
		int discomfort = u_clothing_discomfort();
		discomfort = (discomfort * delta)/100;
		if (moveamt - discomfort < NORMAL_SPEED/2) {
			moveamt = min(moveamt, NORMAL_SPEED/2);
		}
		else moveamt -= discomfort;
	}
	
	if(In_fog_cloud(&youmonst)) moveamt = max(moveamt/3, 1);
	youmonst.movement += moveamt;
	//floor how far into movement-debt you can fall.
	if (youmonst.movement < -2*NORMAL_SPEED) youmonst.movement = -2*NORMAL_SPEED;
	
}

/* perform 1 turn's worth of time-dependent hp modification, mostly silently */
/* NOTES: can rehumanize(), can print You("pass out from exertion!") if moving when overloaded at 1 hp */
void
you_regen_hp()
{
	int wtcap = near_capacity();
	int perX = 0;
	int * hpmax;
	int * hp;
	boolean blockRegen = FALSE;
	boolean bleeding = FALSE;
	boolean air_drowning = FALSE;
	boolean disintegrating = FALSE;

	// set hp, maxhp pointers
	hp    = (Upolyd) ? (&u.mh)    : (&u.uhp);
	hpmax = (Upolyd) ? (&u.mhmax) : (&u.uhpmax);

	if (u.uinvulnerable) {
		/* prayer invulnerablity only */
		/* for the moment at least, you're in tiptop shape */
		/* do not regenerate any health */
		/* you also do not lose health (e.g. from being a fish out of water) */
		return;
	}

	// Sanity check
	if (Upolyd && (*hp < 1)){
		rehumanize();
		change_gevurah(1); //cheated death.
	}

	//Androids regenerate from active Hoon and healing doll, 
	////but not from other sources unless dormant
	if((*hp) <= (*hpmax)/2 && check_mutation(CRAWLING_FLESH) && check_insight()){
		morehungry(d(4,10)*10);
		(*hp) += (*hpmax)/2;
		struct monst *mon;
		Your("flesh crawls!");
		for(mon = fmon; mon; mon = mon->nmon){
			if(!DEADMONSTER(mon) && mon_can_see_you(mon) && 
				!insightful(mon->data) && 
				!mindless_mon(mon) && !resist(mon, '\0', 0, NOTELL)
			) {
				if(u.ulevel >= mon->m_lev-4){
					mon->mconf = 1;
					if(u.ulevel >= mon->m_lev+4){
						if(canseemon(mon)) pline("%s goes insane from the sight!", Monnam(mon));
						mon->mcrazed = 1;
					} else if(canseemon(mon)) pline("%s looks dizzy.", Monnam(mon));
				}
				if(!breathless_mon(mon) && (!inediate(mon->data) || is_android(mon->data) || mon->mtyp == PM_INCANTIFIER)){
					if(canseemon(mon)){
						if(is_android(mon->data))
							pline("%s retches!", Monnam(mon));
						else if(mon->mtyp == PM_INCANTIFIER)
							pline("%s pukes up a rainbow!", Monnam(mon));
						else pline("%s vomits!", Monnam(mon));
					}
					mon->mcanmove = 0;
					if ((mon->mfrozen + 4) > 127)
						mon->mfrozen = 127;
					else mon->mfrozen += 4;
					monflee(mon, 44, FALSE, FALSE);
				}
				else monflee(mon, 44, FALSE, FALSE);
			}
		}
	}
	if(u.uhoon_duration && (*hp) < (*hpmax)){
		flags.botl = 1;
		
		(*hp) += 10;

		if ((*hp) > (*hpmax))
			(*hp) = (*hpmax);
	}
	if(RapidHealing && (*hp) < (*hpmax)){
		//1/5th max hp
		(*hp) += (*hpmax)/5+1;
		
		if ((*hp) > (*hpmax))
			(*hp) = (*hpmax);
	}
	if(check_mutation(CRAWLING_FLESH) && roll_generic_flat_madness(FALSE) && !rn2(3)){
		(*hp)++;

		if ((*hp) > (*hpmax))
			(*hp) = (*hpmax);
	}
	
	//Androids regenerate from active Hoon, but not from other sources unless dormant
	// Notably, no bonus from passive Hoon
	if(uandroid && !u.usleep)
		return;
	
	// Previously used hoons
	perX += u.uhoon;
	// Carried vital soulstones
	perX += stone_health();
	
	// fish out of water
	if (youracedata->mlet == S_EEL && !is_pool(u.ux, u.uy, youracedata->msize == MZ_TINY) && !Is_waterlevel(&u.uz)) {
		if (is_pool(u.ux, u.uy, TRUE))
			perX -= HEALCYCLE * (youracedata->msize - 1) / (youracedata->msize);
		else
			perX -= HEALCYCLE;
		blockRegen = TRUE;
		air_drowning = TRUE;
	}

	// invidiaks out of dark
	if (youracedata->mtyp == PM_INVIDIAK && !isdark(u.ux, u.uy)) {
		perX -= HEALCYCLE;
		blockRegen = TRUE;
		disintegrating = TRUE;
	}

	// bleeding out
	if (youmonst.mbleed > 0) {
		youmonst.mbleed--;
		bleeding = TRUE;
		perX -= youmonst.mbleed*HEALCYCLE;
		blockRegen = TRUE;
	}

	// regeneration 'trinsic
	if (Regeneration){
		perX += HEALCYCLE;
	}
	// regeneration tech
	if (uarm && uarm->otyp == IMPERIAL_ELVEN_ARMOR && check_imp_mod(uarm, IEA_FAST_HEAL)){
		perX += u.ulevel;
	}
	// regeneration weapon
	if (uwep && uwep->oartifact == ART_CRUCIFIX_OF_THE_MAD_KING){
		perX += HEALCYCLE/2;
	}
	// Corruption thought
	if(active_glyph(CORRUPTION) && (*hp < (*hpmax)*.3)){
		perX += HEALCYCLE;
	}
	// Defilement thought
	if(active_glyph(DEFILEMENT)){
		if(*hp < (*hpmax)*.1)
			perX += HEALCYCLE;
		else if(*hp < (*hpmax)*.2)
			perX += 2*HEALCYCLE/3;
		else if(*hp < (*hpmax)*.3)
			perX += HEALCYCLE/3;
	}

	// "Natural" regeneration has stricter limitations
	if (blockRegen);		// regeneration blocked by inclement conditions this turn.
	else if (u.regen_blocked > 0) u.regen_blocked--;		// regenration blocked by a curse (NOTE: decremented here)
	else if (Race_if(PM_INCANTIFIER)){ // Incantifiers are special (including while polymorphed)
		//Incantifiers have slow and unpredictable but comparitively efficient magical healing
		// Healing spell is 3hp/power, this is 5hp/power
		// Hiding in a corner waiting is less efficient than healing with the heal spell.
		if(Race_if(PM_INCANTIFIER) && (*hp) < (*hpmax) && u.uen > 0){
			int quantity = maybe_polyd(youmonst.data->mlevel, u.ulevel)/5 + 1;
			do{
				if(!rn2(HEALCYCLE)){
					(*hp) += 5;
					u.uen--;
				}
			} while(--quantity > 0 && u.uen > 0 && (*hp) < (*hpmax));
			if((*hp) > (*hpmax))
				(*hp) = (*hpmax);
		}
	} else if (!(nonliving(youracedata) && !uandroid) &&	// not nonliving, however, androids auto-repair while asleep
		(wtcap < MOD_ENCUMBER || !u.umoved) &&		// not overloaded
		!(uwep && uwep->oartifact == ART_ATMA_WEAPON && uwep->lamplit && !Drain_resistance && rn2(4)) // 3 in 4 chance of being prevented by Atma Weapon
			// Question for Chris: what if instead, do "reglevel /= 4;" when atma weapon is active, placed either before or after the minimum reg 1 check?
	){
		int reglevel = maybe_polyd(youmonst.data->mlevel, u.ulevel);

		// CON bonus (while in natural form)
		if (!Upolyd)
			reglevel += ((int)ACURR(A_CON) - 10);
		// minimum 1
		if (reglevel < 1)
			reglevel = 1;

		// Sleeping
		if(u.usleep){
			// To be comfortable, a bed must be "on top" of any pile of items in the square.
			struct obj *bed = level.objects[u.ux][u.uy]; 
			// on a bed
			if(bed && bed->oclass == BED_CLASS){
				// A bedroll must be the only object in the square, other beds can have stuff tucked under them
				if((bed->otyp == BEDROLL && !bed->nexthere) || bed->otyp == GURNEY)
					reglevel *= 1.5;
				else if(bed->otyp == BED)
					reglevel *= 2;
				else if(bed->otyp == EXPENSIVE_BED)
					reglevel *= 10;
			}
			// restfully
			if(RestfulSleep){
				perX += HEALCYCLE;
			}
		}
		

		// Barbarian role bonus
		if (Role_if(PM_BARBARIAN) && !Upolyd)
			reglevel *= 3;
		// Melee roles can be slightly less concerned about taking hits
		else if(!Upolyd && (
			Role_if(PM_KNIGHT)
			|| Role_if(PM_PIRATE)
			|| Role_if(PM_SAMURAI)
			|| Role_if(PM_VALKYRIE)
			|| Role_if(PM_CONVICT)
			|| Role_if(PM_UNDEAD_HUNTER)
			|| (u.sealsActive&SEAL_BERITH)
		))
			reglevel *= 2;
		else if(!Upolyd && (
			Role_if(PM_MADMAN)
		))
			reglevel *= 1.5;

		//Additive bonuses
		// Elf bonus
		if ((Race_if(PM_ELF) || Race_if(PM_MYRKALFR)) && !Upolyd)
			reglevel += 7;
		if (Race_if(PM_DROW) && !Upolyd)
			reglevel += flags.female ? 4 : 8; //Note: blessing of Lolth
		// Healer role bonus
		if (Role_if(PM_HEALER) && !Upolyd)
			reglevel += 10;
		
		// penalty for being itchy
		reglevel -= u_healing_penalty();

		// minimum 1
		if (reglevel < 1)
			reglevel = 1;

		perX += reglevel;
	}

	// The Ring of Hygiene's Disciple
	if (!Upolyd &&	// Question for Chris: should this be enabled to also work while polymorphed?
		uring_art(ART_RING_OF_HYGIENE_S_DISCIPLE)
	){
		perX += HEALCYCLE * min(4, (*hpmax) / max((*hp), 1));
	}

	// Buer
	if (u.sealsActive&SEAL_BUER){
		int dsize = spiritDsize();

		if (uwep && uwep->oartifact == ART_ATMA_WEAPON && uwep->lamplit && !Drain_resistance)
			perX += dsize * 6 / 4;
		else
			perX += dsize * 6;
	}
	
	/* moving around while encumbered is hard work */
	if (wtcap > MOD_ENCUMBER && u.umoved) {
		if (*hp > 1)
		{
			if (wtcap < EXT_ENCUMBER)
				perX -= 1;
			else
				perX -= 3;
		}
		else
		{
			You("pass out from exertion!");
			exercise(A_CON, FALSE);
			fall_asleep(-10, FALSE);
		}
	}
	
	if(FaintingFits && rn2(100) >= NightmareAware_Sanity && multi >= 0){
		You("suddenly faint!");
		fall_asleep((NightmareAware_Sanity - 100)/10 - 1, FALSE);
	}

	//Worn Vilya bonus ranges from -4 (penalty) to +7 HP per 10 turns
	// If you're currently dying (negative HP regen) Vilya may make it worse,
	//  but vilya won't cause you to start dying if you aren't already.
	// Consequently, it should be applied after all other bonuses
	if(uring_art(ART_VILYA)){
		int vmod = heal_vilya()*HEALCYCLE/10;
		if(perX >= 0){
			perX = max(0, perX+vmod);
		}
		else {
			perX += vmod;
		}
	}

	if (((perX > 0) && ((*hp) < (*hpmax))) ||			// if regenerating
		((perX < 0))									// or dying
		)
	{
		// update bottom line
		flags.botl = 1;

		// modify by 1/HEALCYCLEth of perX per turn:
		*hp += perX / HEALCYCLE;
		// Now deal with any remainder
		if (((moves)*(abs(perX) % HEALCYCLE)) / HEALCYCLE >((moves - 1)*(abs(perX) % HEALCYCLE)) / HEALCYCLE)
			*hp += 1 * sgn(perX);
		
		// cap at maxhp
		if ((*hp) > (*hpmax))
			(*hp) = (*hpmax);

		// check for rehumanization
		if (*hp < 1){
			if(Upolyd){
				rehumanize();
				change_gevurah(1); //cheated death.
			}
			else {
				killer_format = KILLED_BY;
				if(bleeding)
					killer="blood loss";
				else if(air_drowning)
					killer="air-drowning";
				else if(disintegrating)
					killer="disintegrating in bright light";
				else
					killer="poor health, apparently";
				done(DIED);
			}
		}
	}
}

/* perform 1 turn's worth of time-dependent power modification, silently */
void
you_regen_pw()
{
	int wtcap = near_capacity();
	int perX = 0;

	// natural power regeneration
	if (wtcap < MOD_ENCUMBER) {	// not overly encumbered
		int reglevel = u.ulevel + (((int)ACURR(A_WIS)) - 10);
		// level + WISmod minimum 1
		if (reglevel < 1)
			reglevel = 1;

		// Sleeping
		if(u.usleep){
			// To be comfortable, a bed must be "on top" of any pile of items in the square.
			struct obj *bed = level.objects[u.ux][u.uy]; 
			// on a bed
			if(bed && bed->oclass == BED_CLASS){
				// A bedroll must be the only object in the square, other beds can have stuff tucked under them
				if((bed->otyp == BEDROLL && !bed->nexthere) || bed->otyp == GURNEY)
					reglevel *= 3;
				else if(bed->otyp == BED)
					reglevel *= 4;
				else if(bed->otyp == EXPENSIVE_BED)
					reglevel *= 20;
			}
			// restfully
			if(RestfulSleep){
				perX += HEALCYCLE;
			}
		}
		
		// role bonuses
		if(Role_if(PM_MONK) && u.unull){
			reglevel *= 2;
			reglevel += 8;
		}
		if (Role_if(PM_WIZARD)){
			reglevel *= 2;
			reglevel += 10;
		}
		if (Role_if(PM_MADMAN))   reglevel += 9;
		if (Role_if(PM_UNDEAD_HUNTER) && Insight > 10)
								  reglevel += 9;
		if (Role_if(PM_HEALER))   reglevel += 6;
		if (Role_if(PM_PRIEST))   reglevel += 6;
		if (Role_if(PM_VALKYRIE)) reglevel += 3;

		// cornuthaum bonus for wizards (but not incantifiers, since they don't naturally regenerate power at all)
		if (u.uen < u.uenmax && (Role_if(PM_WIZARD)) && uarmh && uarmh->otyp == CORNUTHAUM){
			reglevel += uarmh->spe;
		}
		//Additive bonuses
		// Elf bonus
		if ((Race_if(PM_ELF) || Race_if(PM_MYRKALFR)) && !Upolyd)
			reglevel += 7;
		if (Race_if(PM_DROW) && !Upolyd)
			reglevel += flags.female ? 8 : 4;//Note: blessing of Lolth
		if (Race_if(PM_GNOME) && !Upolyd)
			reglevel += 12;
		// penalty for being itchy
		reglevel -= u_healing_penalty();
		// penalty from spell protection interfering with natural pw regeneration
		if (u.uspellprot > 0)
			reglevel -= 10 + 2 * u.uspellprot;

		// minimum 1
		if (reglevel < 1)
			reglevel = 1;

		perX += reglevel;
	}

	// Carried spiritual soulstones
	perX += stone_energy();
	//Worn Nenya bonus ranges from -4 penalty to +7 HP per 10 turns
	if(uring_art(ART_NENYA))
		perX += en_nenya()*HEALCYCLE/10;
	
	// external power regeneration
	if (Energy_regeneration ||										// energy regeneration 'trinsic
		(martial_bonus() && !uarmf && IS_GRASS(levl[u.ux][u.uy].typ))	// or being a bare-foot martial-artist standing on grass
		)
	{
		perX += HEALCYCLE;
	}

	// Unknown God
	if (u.specialSealsActive&SEAL_UNKNOWN_GOD){
		perX += spiritDsize() * 10;
	}

	// power drain from maintained spells
	if (u.maintained_en_debt > 0)
	{
		int reglevel = u.maintained_en_debt / 3;
		int debtpaid = 0;

		if (perX > reglevel)
		{// can just subtract drain from pw regeneration and still have net positive
			perX -= reglevel;
			debtpaid += reglevel;
		}
		else
		{// either 0 pw regen or net drain
			// put the entirety of perX against the debt owed
			reglevel -= perX;
			debtpaid += perX;
			perX = 0;
			// drain further if the player has reserves to burn
			if (u.uen > 0 || Race_if(PM_INCANTIFIER))
			{
				debtpaid += reglevel;
				perX = -reglevel;
			}
		}
		// settle the payment
		u.maintained_en_debt -= debtpaid / HEALCYCLE;
		//Now deal with any remainder
		if (((moves)*(debtpaid % HEALCYCLE)) / HEALCYCLE >((moves - 1)*(debtpaid % HEALCYCLE)) / HEALCYCLE)
			u.maintained_en_debt -= 1;
		// minimum zero
		if (u.maintained_en_debt < 0)
			u.maintained_en_debt = 0;
	}

	//Naen fast energy regen applies to androids, too.
	if(u.unaen_duration && (u.uen < u.uenmax)){
		flags.botl = 1;
		u.uen += 10;
		if (u.uen > u.uenmax)
			u.uen = u.uenmax;
	}
	
	//Naen passive bonus only partially applies to androids and incantifiers
	if(Race_if(PM_INCANTIFIER)){
		if(perX < 0)
			perX += u.unaen;
		if(perX > 0)
			perX = 0;
	}
	else perX += u.unaen;
	
	//Androids only regen power while asleep, but allow their rate to offset spell maintenance
	if(uandroid && !u.usleep && perX > 0)
		return;

	if (((perX > 0) && (u.uen < u.uenmax)) ||							// if regenerating power
		((perX < 0) && ((u.uen > 0) || Race_if(PM_INCANTIFIER)))		// or draining power
		)
	{
		//update bottom line
		flags.botl = 1;

		// modify by 1/HEALCYCLEth of perX per turn:
		u.uen += perX / HEALCYCLE;
		// Now deal with any remainder
		if (((moves)*(abs(perX) % HEALCYCLE)) / HEALCYCLE >((moves - 1)*(abs(perX) % HEALCYCLE)) / HEALCYCLE)
			u.uen += 1 * sgn(perX);
		// cap at maxpw
		if (u.uen > u.uenmax)
			u.uen = u.uenmax;
		// and at 0 (for non-incantifiers)
		if (u.uen < 0 && !(Race_if(PM_INCANTIFIER)))
			u.uen = 0;
	}
}

int
san_threshhold()
{
	// Threshold levels based on wis.
	int reglevel = ACURR(A_WIS);
	int insight = Insight;

	// role bonuses
	if (Role_if(PM_BARBARIAN))   reglevel += 10;
	if (Role_if(PM_VALKYRIE)) reglevel += 9;
	if (Role_if(PM_TOURIST))     reglevel += 8;
	if (Role_if(PM_MONK))     reglevel += 8;
	if (Role_if(PM_PRIEST))   reglevel += 7;
	if (Role_if(PM_ANACHRONONAUT))   reglevel += 5;
	if (Role_if(PM_UNDEAD_HUNTER) && u.veil)   reglevel += 5;
	if (Role_if(PM_EXILE))    reglevel += spiritDsize();
	if (Role_if(PM_CONVICT))  reglevel -= 3;
	if (Role_if(PM_NOBLEMAN))  reglevel -= 5;
	if (Role_if(PM_MADMAN))   reglevel -= 10;
	if (Role_if(PM_UNDEAD_HUNTER) && (Insight >= 50 || quest_status.moon_close))   reglevel -= 10;
	//Maybe scale undead hunter based on how well they did on the quest as well.

	if (Race_if(PM_ELF))   reglevel += 3;
	if (Race_if(PM_ORC))   reglevel -= 3;

	// Unusually not-sane spirit
	if (u.sealsActive&SEAL_ORTHOS){
		reglevel -= 5;
	}

	// Bonus/penalty for insight
	if(u.veil)
		reglevel += 10;
	else while((insight = insight/2))
		reglevel--;

	if(active_glyph(TRANSPARENT_SEA))
		reglevel += 30;

	if(active_glyph(ROTTEN_EYES))
		reglevel += 10;

	//Worn Vilya bonus ranges from -4 (penalty) to +7
	if(uring_art(ART_VILYA))
		reglevel += heal_vilya();

	// penalty for certain areas
	if(Is_rlyeh(&u.uz)){
		reglevel -= 30;
	}
	else if(In_nkai(&u.uz)){
		reglevel -= 20;
	}
	else if(In_depths(&u.uz)){
		reglevel -= 10;
	}
	
	if(In_quest(&u.uz)){
		if(Role_if(PM_ANACHRONONAUT)){
			if(Race_if(PM_ANDROID)){
				// The Android PC's actual home is the locate level
				if(Is_qlocate(&u.uz)){
					reglevel -= 30; //net -25
				}
				else if(Is_qstart(&u.uz)){
					reglevel -= 15; //net -10
				}
				else {
					reglevel -= 10; //net -5, reversing bonus
				}
			}
			else {
				if(Is_qstart(&u.uz) && quest_status.leader_is_dead){
					reglevel -= 30; //net -25
				}
				else {
					reglevel -= 10; //net -5, reversing bonus
				}
			}
		}
		//Lots of stressful circumstances here!
		else if(Race_if(PM_DROW)){
			if(Role_if(PM_NOBLEMAN)){
				if(Is_nemesis(&u.uz)){
					reglevel -= 30;
				}
				else if(Is_qstart(&u.uz) && Role_if(PM_NOBLEMAN) && !flags.initgend){
					reglevel -= 30;
				}
			}
			else if(Is_nemesis(&u.uz) && !flags.initgend && (Role_if(PM_PRIEST) || Role_if(PM_ROGUE) || Role_if(PM_RANGER) || Role_if(PM_WIZARD))){
				reglevel -= 30;
			}
		}
	}
	
	// penalty for being itchy
	reglevel -= u_healing_penalty();
	
	return reglevel;
}

/* perform 1 turn's worth of time-dependent sanity modification, silently */
void
you_regen_san()
{
	if(u.veil && In_depths(&u.uz)){
		u.veil = FALSE;
		change_uinsight(1);
	}

	int reglevel = san_threshhold();

	// minimum 0
	if (reglevel <= 0)
		return;

	// Fast recover to 1x, slower recovery to 2x, slowest recover to 3x.
	if(u.usanity < reglevel){
		if(!(moves%10))
			change_usanity(1, FALSE);
	}
	else if(u.usanity < reglevel*2){
		if(!(moves%32))
			change_usanity(1, FALSE);
	}
	else if(u.usanity < reglevel*3){
		if(!(moves%100))
			change_usanity(1, FALSE);
	}
}

int
random_frequency()
{
	/*Turn off random generation on axus's level if lawful*/
	if(Is_illregrd(&u.uz) && u.ualign.type == A_LAWFUL && !u.uevent.uaxus_foe)
		return 0;

	/*Drastically reduce spawn rate if the painting is peaceful*/
	if(In_quest(&u.uz) && Race_if(PM_HALF_DRAGON) && Role_if(PM_NOBLEMAN) && flags.initgend && u.uevent.qcompleted && u.ualign.record > 4)
		return 210;

	if(u.uevent.udemigod)
		return 25;

	if(Infuture)
		return (!(Is_qstart(&u.uz) && !(quest_status.leader_is_dead)) ? 35 : ANA_HOME_PROB);

	if(depth(&u.uz) > depth(&stronghold_level))
		return 50;
	
	return 70;
}

int
spawn_random_monster()
{
	int difficulty = 0;
	struct monst *mtmp = 0;
	if(Is_ford_level(&u.uz)){
		if(rn2(2)){
			int x, y, tries = 200;
			do x = rn2(COLNO/2) + COLNO/2 + 1, y =  rn2(ROWNO-2)+1;
			while((!isok(x,y) || !(levl[x][y].typ == SOIL || levl[x][y].typ == ROOM)) && tries-- > 0);
			if(tries >= 0){
				mtmp = makemon(ford_montype(1), x, y, MM_ADJACENTOK);
			}
		} else {
			int x, y, tries = 200;
			do x = rn2(COLNO/2) + 1, y =  rn2(ROWNO-2)+1;
			while((!isok(x,y) || !(levl[x][y].typ == SOIL || levl[x][y].typ == ROOM)) && tries-- > 0);
			if(tries >= 0){
				mtmp = makemon(ford_montype(-1), x, y, MM_ADJACENTOK);
			}
		}
		if(mtmp)
			difficulty += monstr[mtmp->mtyp];
	} else {
		if(In_sokoban(&u.uz)){
			if(u.uz.dlevel != 1 && u.uz.dlevel != 4) mtmp = makemon((struct permonst *)0, xupstair, yupstair, MM_ADJACENTSTRICT|MM_ADJACENTOK);
			if(mtmp)
				difficulty += monstr[mtmp->mtyp];
		} else if(Infuture && Is_qstart(&u.uz) && !(quest_status.leader_is_dead)){
			mtmp = makemon((struct permonst *)0, xdnstair, ydnstair, MM_ADJACENTOK);
			if(mtmp)
				difficulty += monstr[mtmp->mtyp];
			if(ANA_SPAWN_TWO){
				mtmp = makemon((struct permonst *)0, xdnstair, ydnstair, MM_ADJACENTOK);
				if(mtmp)
					difficulty += monstr[mtmp->mtyp];
				if(ANA_SPAWN_THREE){
					mtmp = makemon((struct permonst *)0, xdnstair, ydnstair, MM_ADJACENTOK);
					if(mtmp)
						difficulty += monstr[mtmp->mtyp];
					if(ANA_SPAWN_FOUR) {
						mtmp = makemon((struct permonst *)0, xdnstair, ydnstair, MM_ADJACENTOK);
						if(mtmp)
							difficulty += monstr[mtmp->mtyp];
					}
				}
			}
		}
		else {
			mtmp = makemon((struct permonst *)0, 0, 0, NO_MM_FLAGS);
			if(mtmp)
				difficulty += monstr[mtmp->mtyp];
		}
	}
	return difficulty;
}

void
moveloop()
{
#if defined(MICRO) || defined(WIN32)
	char ch;
#endif
	int abort_lev, i, j;
    struct monst *mtmp, *nxtmon;
	struct obj *pobj;
    int wtcap = 0, change = 0;
    boolean didmove = FALSE, monscanmove = FALSE;
	int oldspiritAC=0;
	int tx,ty;
	static boolean oldBlind = 0, oldLightBlind = 0;
	int healing_penalty = 0;
    int hpDiff, movecost;
	static int oldsanity = 100;

    flags.moonphase = phase_of_the_moon();
    if(flags.moonphase == HUNTING_MOON) {
	pline("The moon hangs low.");
	change_luck(2);
    } else if(flags.moonphase == FULL_MOON) {
	You("are lucky!  Full moon tonight.");
	change_luck(1);
    } else if(flags.moonphase == NEW_MOON) {
	pline("Be careful!  New moon tonight.");
    }
    flags.friday13 = friday_13th();
    flags.tm_hour = getlt()->tm_hour;
    if (flags.friday13) {
	pline("Watch out!  Bad things can happen on Friday the 13th.");
	change_luck(-1);
    }

    initrack();


    /* Note:  these initializers don't do anything except guarantee that
	    we're linked properly.
    */
    decl_init();
    monst_init();
    monstr_init();	/* monster strengths */
    objects_init();

    commands_init();
#ifdef WIZARD
	// Called twice in dNAO?
    //if (wizard) add_debug_extended_commands();
#endif

    (void) encumber_msg(); /* in case they auto-picked up something */

    u.uz0.dlevel = u.uz.dlevel;
    youmonst.movement = NORMAL_SPEED;	/* give the hero some movement points */
	flags.move = MOVE_CANCELLED; /* From nethack 3.6.2 */
    prev_hp_notify = uhp();

	if(galign(u.ugodbase[UGOD_ORIGINAL]) == A_LAWFUL && flags.initalign != 0){
		flags.initalign = 0;
		impossible("Bad alignment initializer detected and fixed. Save and reload.");
	}
	if(galign(u.ugodbase[UGOD_ORIGINAL]) == A_NEUTRAL && flags.initalign != 1){
		flags.initalign = 1;
		impossible("Bad alignment initializer detected and fixed. Save and reload.");
	}
	if(galign(u.ugodbase[UGOD_ORIGINAL]) == A_CHAOTIC && flags.initalign != 2){
		flags.initalign = 2;
		impossible("Bad alignment initializer detected and fixed. Save and reload.");
	}
	if(galign(u.ugodbase[UGOD_ORIGINAL]) == A_NONE && flags.initalign != 3){
		flags.initalign = 3;
		impossible("Bad alignment initializer detected and fixed. Save and reload.");
	}
	if(galign(u.ugodbase[UGOD_ORIGINAL]) == A_VOID && flags.initalign != 4){
		flags.initalign = 4;
		impossible("Bad alignment initializer detected and fixed. Save and reload.");
	}
	// printMons();
	// printMonNames();
	// printDPR();
	// printBodies();
	// printSanAndInsight();
    for(;;) {/////////////////////////MAIN LOOP/////////////////////////////////
	if (!iflags.debug_fuzzer) gosleep();
    hpDiff = u.uhp;
	get_nh_event();
#ifdef POSITIONBAR
	do_positionbar();
#endif
	movement_combos();

	
	if ((flags.move & ~(MOVE_DEFAULT)) != MOVE_CANCELLED) {
		flags.movetoprint = !(flags.move & ~(MOVE_PARTIAL|MOVE_DEFAULT)) ? MOVE_STANDARD : flags.move;
		flags.movetoprintcost = you_action_cost(flags.move, FALSE);
	}
	if((movecost = you_action_cost(flags.move, TRUE)) > 0) {
		/* actual time passed */
		youmonst.movement -= movecost;
		didmove = TRUE;

		  /**************************************************/
		 /* things that respond to the player turn go here */
		/**************************************************/
		for (mtmp = fmon; mtmp; mtmp = nxtmon){
			nxtmon = mtmp->nmon;
			if(DEADMONSTER(mtmp))
				continue;
			if(mtmp->m_insight_level > Insight && !mtmp->mcan && mtmp->mtyp == PM_TRANSCENDENT_TETTIGON){
				set_mon_data(mtmp, PM_UNMASKED_TETTIGON);
				mtmp->m_insight_level -= 35;
				newsym(x(mtmp), y(mtmp));
			}
			if(mtmp->m_insight_level > Insight
			  || (mtmp->mtyp == PM_WALKING_DELIRIUM && BlockableClearThoughts)
			  || (mtmp->mtyp == PM_STRANGER && !quest_status.touched_artifact)
			  || ((mtmp->mtyp == PM_PUPPET_EMPEROR_XELETH || mtmp->mtyp == PM_PUPPET_EMPRESS_XEDALLI) && mtmp->mvar_yellow_lifesaved)
			  || (mtmp->mtyp == PM_TWIN_SIBLING && (mtmp->mvar_twin_lifesaved || !(u.specialSealsActive&SEAL_YOG_SOTHOTH)))
			){
				if(!(mtmp->mtrapped && t_at(mtmp->mx, mtmp->my) && t_at(mtmp->mx, mtmp->my)->ttyp == VIVI_TRAP)){
					if(mtmp->mvar1_tettigon_uncancel){
						mtmp->mvar1_tettigon_uncancel = FALSE;
						set_mcan(mtmp, FALSE);
					}
					insight_vanish(mtmp);
					continue;
				}
			}
			if(has_template(mtmp, DELOUSED)){
				delouse_tame(mtmp);
				continue;
			}
			if(mtmp->mamnesia){
				if(mtmp->isshk){
					make_happy_shk(mtmp, FALSE);
					ESHK(mtmp)->pbanned = FALSE;
					ESHK(mtmp)->signspotted = 0;
				}
				else {
					mtmp->mpeaceful = TRUE;
					mtmp->mtame = FALSE;
				}
				if(mtmp->ispriest){
					EPRI(mtmp)->pbanned = FALSE;
					EPRI(mtmp)->signspotted = 0;
				}
				mtmp->mamnesia = FALSE;
				newsym(mtmp->mx, mtmp->my);
			}
			if (mon_attacktype(mtmp, AT_WDGZ)
				&& !(controlledwidegaze(mtmp->data) && (mtmp->mpeaceful || mtmp->mtame || helpless(mtmp)))
				&& !(hideablewidegaze(mtmp->data) && hiddenwidegaze(mtmp))
				&& !vivitrapped(mtmp)
				&& couldsee(mtmp->mx, mtmp->my)
			) m_widegaze(mtmp);
		}
		if((is_ice(u.ux, u.uy) || mad_turn(MAD_COLD_NIGHT)) && roll_madness(MAD_FRIGOPHOBIA)){
			if(!Flying && !Levitation){
				You("panic from setting foot on ice!");
				HPanicking += 1+rnd(6);
			}
			else if(roll_madness(MAD_FRIGOPHOBIA)){//Very scared of ice
				You("panic from traveling over ice!");
				HPanicking += 1+rnd(3);
			}
		}
		if (u.uattked || !u.umoved) {
			if(uandroid && u.ucspeed == HIGH_CLOCKSPEED)
				u.ucspeed = NORM_CLOCKSPEED;
			artinstance[ART_SEVEN_LEAGUE_BOOTS].LeagueMod = 0;
			artinstance[ART_FENG_HUO_LUN].LeagueMod = 0;
		}
		
	    do { /* hero can't move this turn loop */
		wtcap = encumber_msg();

		flags.mon_moving = TRUE;
		do { /* Monster turn loop */
		    monscanmove = movemon();
			  /********************************************/
			 /*once-per-all-monsters-moved things go here*/
			/********************************************/
////////////////////////////////////////////////////////////////////////////////////////////////
			if (flags.run_timers){
				run_timers();
			}
////////////////////////////////////////////////////////////////////////////////////////////////
			if(u.specialSealsActive&SEAL_LIVING_CRYSTAL)
				average_dogs();
			for (mtmp = fmon; mtmp; mtmp = nxtmon){
				nxtmon = mtmp->nmon;
				if(DEADMONSTER(mtmp))
					continue;
				if(mtmp->mtyp == PM_HELLCAT){
					if(!isdark(mtmp->mx,mtmp->my) && !mtmp->minvis){
						mtmp->minvis = TRUE;
						mtmp->perminvis = TRUE;
						newsym(mtmp->mx,mtmp->my);
					} else if(isdark(mtmp->mx,mtmp->my) && mtmp->minvis){
						mtmp->minvis = FALSE;
						mtmp->perminvis = FALSE;
						newsym(mtmp->mx,mtmp->my);
					}
				} else if(mtmp->mtyp == PM_HUDOR_KAMEREL){
					if(is_pool(mtmp->mx,mtmp->my, TRUE) && !mtmp->minvis){
						mtmp->minvis = TRUE;
						mtmp->perminvis = TRUE;
						newsym(mtmp->mx,mtmp->my);
					} else if(!is_pool(mtmp->mx,mtmp->my, TRUE) && mtmp->minvis){
						mtmp->minvis = FALSE;
						mtmp->perminvis = FALSE;
						newsym(mtmp->mx,mtmp->my);
					}
				} else if(mtmp->mtyp == PM_GRUE || mtmp->mtyp == PM_INVIDIAK){
					if(isdark(mtmp->mx,mtmp->my) && !mtmp->minvis){
						mtmp->minvis = TRUE;
						mtmp->perminvis = TRUE;
						newsym(mtmp->mx,mtmp->my);
					} else if(!isdark(mtmp->mx,mtmp->my) && mtmp->minvis){
						mtmp->minvis = FALSE;
						mtmp->perminvis = FALSE;
						newsym(mtmp->mx,mtmp->my);
					}
				}
				if(mtmp->m_insight_level > Insight && !mtmp->mcan && mtmp->mtyp == PM_TRANSCENDENT_TETTIGON){
					set_mon_data(mtmp, PM_UNMASKED_TETTIGON);
					mtmp->m_insight_level -= 35;
					newsym(x(mtmp), y(mtmp));
				}
				if(mtmp->m_insight_level > Insight
				  || (mtmp->mtyp == PM_WALKING_DELIRIUM && BlockableClearThoughts)
				  || (mtmp->mtyp == PM_STRANGER && !quest_status.touched_artifact)
				  || ((mtmp->mtyp == PM_PUPPET_EMPEROR_XELETH || mtmp->mtyp == PM_PUPPET_EMPRESS_XEDALLI) && mtmp->mvar_yellow_lifesaved)
				  || (mtmp->mtyp == PM_TWIN_SIBLING && (mtmp->mvar_twin_lifesaved || !(u.specialSealsActive&SEAL_YOG_SOTHOTH)))
				){
					if(!(mtmp->mtrapped && t_at(mtmp->mx, mtmp->my) && t_at(mtmp->mx, mtmp->my)->ttyp == VIVI_TRAP)){
						insight_vanish(mtmp);
						continue;
					}
				}
				if(has_template(mtmp, DELOUSED)){
					delouse_tame(mtmp);
					continue;
				}
				if(mtmp->mamnesia){
					if(mtmp->isshk){
						make_happy_shk(mtmp, FALSE);
						ESHK(mtmp)->pbanned = FALSE;
						ESHK(mtmp)->signspotted = 0;
					}
					else {
						mtmp->mpeaceful = TRUE;
						mtmp->mtame = FALSE;
					}
					if(mtmp->ispriest){
						EPRI(mtmp)->pbanned = FALSE;
						EPRI(mtmp)->signspotted = 0;
					}
					mtmp->mamnesia = FALSE;
					newsym(mtmp->mx, mtmp->my);
				}
				if (mtmp->minvis){
					newsym(mtmp->mx, mtmp->my);
				}
				if (Blind && Bloodsense && has_blood_mon(mtmp)){
					newsym(mtmp->mx, mtmp->my);
				}
			}
////////////////////////////////////////////////////////////////////////////////////////////////
			if(Echolocation){
				for(i=1; i<COLNO; i++)
					for(j=0; j<ROWNO; j++)
						if(viz_array[j][i]&COULD_SEE)
							echo_location(i, j);
				see_monsters();
			}
			/*If anything a monster did caused us to get moved out of water, surface*/
			if(u.usubwater && !is_pool(u.ux, u.uy, FALSE)){
				u.usubwater = 0;
				vision_full_recalc = 1;
				vision_recalc(2);	/* unsee old position */
				doredraw();
			} else if(Is_waterlevel(&u.uz) && u.usubwater && !is_3dwater(u.ux, u.uy)){
				You("pop out into an airbubble!");
				u.usubwater = 0;
			} else if(Is_waterlevel(&u.uz) && !u.usubwater && is_3dwater(u.ux, u.uy)){
				Your("%s goes under water!", body_part(HEAD));
				if(!Breathless) You("can't breath.");
				u.usubwater = 1;
			}
////////////////////////////////////////////////////////////////////////////////////////////////
			switch (((u_healing_penalty() - healing_penalty) > 0) - ((u_healing_penalty() - healing_penalty) < 0))
			{
			case 0:
					break;
			case 1:
					if (!Hallucination) {
						You_feel("%s.", (healing_penalty) ? "itchier" : "itchy");
					}
					else {
						You_feel("%s.", (healing_penalty) ? "uglier" : "ugly");
					}
					healing_penalty = u_healing_penalty();
					break;
			case -1:
					if (!Hallucination) {
						You_feel("%s.", (u_healing_penalty()) ? "some relief" : "relieved");
					}
					else {
						You_feel("%s.", (u_healing_penalty()) ? "pretty" : "beautiful");
					}
					healing_penalty = u_healing_penalty();
					break;
			}
			/*
			if (healing_penalty != u_healing_penalty()) {
				if (!Hallucination){
					You_feel("%s.", (!healing_penalty) ? "itchy" : "relief");
				} else {
					You_feel("%s.", (!healing_penalty) ? (hates_silver(youracedata) ? "tarnished" :
						hates_iron(youracedata) ? "magnetic" : "like you are failing Organic Chemistry") : "like you are no longer failing Organic Chemistry");
				}
				
				
			}
			*/
////////////////////////////////////////////////////////////////////////////////////////////////
			if (!oldBlind ^ !Blind) {  /* one or the other but not both */
				see_monsters();
				flags.botl = 1;
				vision_full_recalc = 1;	/* blindness just got toggled */
				if (Blind_telepat || Infravision) see_monsters();
				oldBlind = !!Blind;
			}
////////////////////////////////////////////////////////////////////////////////////////////////
			if (!oldLightBlind ^ !LightBlind) {  /* one or the other but not both */
				see_monsters();
				flags.botl = 1;
				vision_full_recalc = 1;	/* blindness just got toggled */
				if (Blind_telepat || Infravision) see_monsters();
				oldLightBlind = !!LightBlind;
			}
////////////////////////////////////////////////////////////////////////////////////////////////
			calc_total_maxhp();
////////////////////////////////////////////////////////////////////////////////////////////////
			calc_total_maxen();
////////////////////////////////////////////////////////////////////////////////////////////////
			oldspiritAC = u.spiritAC;
			u.spiritAC = 0; /* reset temporary spirit AC bonuses. Do this once per monster turn */
			u.spiritAttk = 0;
			if(u.sealsActive){
				if(u.sealsActive&SEAL_MALPHAS){
					for (mtmp = fmon; mtmp; mtmp = mtmp->nmon){
						if(mtmp->mtyp == PM_CROW && mtmp->mtame && !um_dist(mtmp->mx,mtmp->my,1)){
							u.spiritAC++;
							u.spiritAttk++;
						}
					}
				}
				if(u.sealsActive&SEAL_ANDREALPHUS){
					if(isok(tx=u.ux-1,ty=u.uy-1) && IS_CORNER(levl[tx][ty].typ) 
						&& IS_WALL(levl[tx+1][ty].typ) && IS_WALL(levl[tx][ty+1].typ)) u.spiritAC += u.ulevel/2+1;
					else if(isok(tx=u.ux+1,ty=u.uy-1) && IS_CORNER(levl[tx][ty].typ) 
						&& IS_WALL(levl[tx-1][ty].typ) && IS_WALL(levl[tx][ty+1].typ)) u.spiritAC += u.ulevel/2+1;
					else if(isok(tx=u.ux+1,ty=u.uy+1) && IS_CORNER(levl[tx][ty].typ) 
						&& IS_WALL(levl[tx-1][ty].typ) && IS_WALL(levl[tx][ty-1].typ)) u.spiritAC += u.ulevel/2+1;
					else if(isok(tx=u.ux-1,ty=u.uy+1) && IS_CORNER(levl[tx][ty].typ) 
						&& IS_WALL(levl[tx+1][ty].typ) && IS_WALL(levl[tx][ty-1].typ)) u.spiritAC += u.ulevel/2+1;
				}
				if(u.sealsActive&SEAL_ENKI){
					for(tx=u.ux-1; tx<=u.ux+1;tx++){
						for(ty=u.uy-1; ty<=u.uy+1;ty++){
							if(isok(tx,ty) && (tx!=u.ux || ty!=u.uy) && IS_STWALL(levl[tx][ty].typ)) u.spiritAC += 1;
						}
					}
				}
				if(u.spiritAC!=oldspiritAC) flags.botl = 1;
			}
////////////////////////////////////////////////////////////////////////////////////////////////
			if(u.uhs == WEAK && u.sealsActive&SEAL_AHAZU) unbind(SEAL_AHAZU,TRUE);
#ifndef GOLDOBJ
			// if(u.sealsActive&SEAL_FAFNIR && u.ugold < u.ulevel*100) unbind(SEAL_FAFNIR,TRUE);
			if(u.sealsActive&SEAL_FAFNIR && u.ugold == 0) unbind(SEAL_FAFNIR,TRUE);
#else
			// if(u.sealsActive&SEAL_FAFNIR && money_cnt(invent) < u.ulevel*100) unbind(SEAL_FAFNIR,TRUE);
			if(u.sealsActive&SEAL_FAFNIR && money_cnt(invent) == 0) unbind(SEAL_FAFNIR,TRUE);
#endif
			if(u.sealsActive&SEAL_JACK && (Is_astralevel(&u.uz) || Inhell)) unbind(SEAL_JACK,TRUE);
			if(u.sealsActive&SEAL_NABERIUS && u.udrunken < u.ulevel/3) unbind(SEAL_NABERIUS,TRUE);
			if(u.specialSealsActive&SEAL_NUMINA && u.ulevel<30) unbind(SEAL_SPECIAL|SEAL_NUMINA,TRUE);
			if(u.sealsActive&SEAL_SHIRO && uarmc && is_mummy_wrap(uarmc)){
				struct obj *otmp = uarmc;
				pline("Hey, people might notice me with that!");
				if (donning(otmp)) cancel_don();
				(void) Cloak_off();
				if (!otmp->oartifact)
					useup(otmp);
				unbind(SEAL_SHIRO,TRUE);
			}
////////////////////////////////////////////////////////////////////////////////////////////////
			//These artifacts may need to respond to what monsters have done.
			///If the player no longer meets the kusanagi's requirements (ie, they lost the amulet)
			///blast 'em and drop to the ground.
			if (uwep && uwep->oartifact == ART_KUSANAGI_NO_TSURUGI){
				if(!touch_artifact(uwep,&youmonst, FALSE)){
					dropx(uwep);
				}
			}
			///If the Atma weapon becomes cursed, it drains life from the wielder to stay lit.
			if(uwep && uwep->oartifact == ART_ATMA_WEAPON &&
				uwep->cursed && 
				uwep->lamplit
			){
				if (!Drain_resistance) {
					pline("%s for a moment, then %s brightly.",
					  Tobjnam(uwep, "flicker"), otense(uwep, "burn"));
					losexp("life force drain",TRUE,TRUE,TRUE);
					uwep->cursed = FALSE;
				}
			}
////////////////////////////////////////////////////////////////////////////////////////////////
			sense_nearby_monsters();
////////////////////////////////////////////////////////////////////////////////////////////////
		    if (youmonst.movement >= NORMAL_SPEED)
				break;	/* it's now your turn */
			else if(HTimeStop){
				you_calc_movement();
				HTimeStop--;
				break;	/* it's now your turn */
			}
		} while (monscanmove);
		flags.mon_moving = FALSE;
		
		if (!monscanmove && youmonst.movement < NORMAL_SPEED) {
		    /* both you and the monsters are out of steam this round */
		    /* set up for a new turn */
		    mcalcdistress();	/* adjust monsters' trap, blind, etc */

		    /* reallocate movement rations to monsters */
			flags.goldka_level=0;
			flags.silence_level=0;
			flags.spore_level=0;
			flags.slime_level=0;
			flags.walky_level=0;
			flags.shade_level=0;
			flags.yello_level = 0;
			flags.spriest_level = 0;
		    for (mtmp = fmon; mtmp; mtmp = nxtmon){
				nxtmon = mtmp->nmon;
				/* check for bad swap weapons */
				if (mtmp->msw) {
					struct obj *obj;

					for(obj = mtmp->minvent; obj; obj = obj->nobj)
						if (obj->owornmask & W_SWAPWEP) break;
					if (!obj || mtmp->msw != obj){
						struct obj * obj2;
						int nswapweps = 0;
						boolean msw_carried_by_mon = FALSE;
						for(obj2 = mtmp->minvent; obj2; obj2 = obj2->nobj) {
							if (mtmp->msw == obj2)	msw_carried_by_mon = TRUE;
							if (obj2->owornmask & W_SWAPWEP) nswapweps++;
						}
						impossible("bad monster swap weapon detected (and fixed) (for devs: %d;%d)", msw_carried_by_mon, nswapweps);
						MON_NOSWEP(mtmp);
					}
				}
				/* Some bugs cause mtame and mpeaceful to diverge, fix w/ a warning */
				if(mtmp->mtame && !mtmp->mpeaceful){
					impossible("Hostile+tame monster state detected (and fixed)");
					mtmp->mpeaceful = TRUE;
				}
				//Remove after testing (can cause "re-trapping" of untrapped monsters)
				if(!mtmp->mtrapped && t_at(mtmp->mx, mtmp->my) && t_at(mtmp->mx, mtmp->my)->ttyp == VIVI_TRAP && !DEADMONSTER(mtmp) && mtmp != u.usteed && !(u.uswallow && mtmp == u.ustuck)){
					impossible("Re-trapping mon %s in vivi trap",noit_mon_nam(mtmp));
					mtmp->mtrapped = TRUE;
				}
				/* Spot the monster for sanity purposes */
				spot_monster(mtmp);
				/* Loyal monsters slowly recover tameness */
				if(mtmp->mtame && mtmp->mtame < 5 && get_mx(mtmp, MX_EDOG) && EDOG(mtmp)->loyal && !(moves%100))
					mtmp->mtame++;
				/* Beast masters slowly improve tameness */
				if(mtmp->mtame && !(moves%1000)){
					if(P_SKILL(P_BEAST_MASTERY)>P_UNSKILLED){
						if(get_mx(mtmp, MX_EDOG) && EDOG(mtmp)->loyal){
							if(mtmp->mtame < 5+P_SKILL(P_BEAST_MASTERY)-P_UNSKILLED)
								mtmp->mtame++;
						}
						else {
							if(mtmp->mtame < P_SKILL(P_BEAST_MASTERY)-P_UNSKILLED)
								mtmp->mtame++;
						}
					}
					if(u.usteed == mtmp && P_SKILL(P_RIDING)>P_UNSKILLED){
						if(get_mx(mtmp, MX_EDOG) && EDOG(mtmp)->loyal){
							if(mtmp->mtame < 5+P_SKILL(P_RIDING)-P_UNSKILLED)
								mtmp->mtame++;
						}
						else {
							if(mtmp->mtame < P_SKILL(P_RIDING)-P_UNSKILLED)
								mtmp->mtame++;
						}
					}
				}
				/* Dominated monsters stay tame */
				if(mtmp->mtame && get_mx(mtmp, MX_EDOG) && EDOG(mtmp)->dominated)
					mtmp->mtame = 100;
				if(mtmp->mtame && is_vampire(mtmp->data) && check_vampire(VAMPIRE_THRALLS))
					mtmp->mtame = 100;
				/*Tannin eggs may hatch, monster may die*/
				if(mtmp->mtaneggs){
					for(int i = mtmp->mtaneggs; i > 0; i--) if(!rn2(6)){
						mtmp->mtaneggs--;
						makemon(&mons[PM_STRANGE_LARVA], mtmp->mx, mtmp->my, MM_ADJACENTOK|MM_ADJACENTSTRICT|NO_MINVENT|MM_NOCOUNTBIRTH);
						mtmp->mhp -= rnd(6);
					}
					/*Died from the damage*/
					if(mtmp->mhp <= 0){
						mondied(mtmp);
						continue;
					}
				}
				if(mtmp->mtyp == PM_STRANGE_LARVA && !has_template(mtmp, TOMB_HERD)){
					grow_up(mtmp, (struct monst *)0);
					//grow up can kill monster.
					if(DEADMONSTER(mtmp))
						continue;
				}
				/*Monsters may have to skip turn*/
				if(noactions(mtmp)){
					/* Monsters in a essence trap can't move */
					if(mtmp->mtrapped && t_at(mtmp->mx, mtmp->my) && t_at(mtmp->mx, mtmp->my)->ttyp == VIVI_TRAP){
						mtmp->mhp = 1;
						mtmp->movement = 0;
						continue;
					}
					if(mtmp->entangled_oid){
						if(mtmp->entangled_otyp == SHACKLES){
							mtmp->mhp = min(mtmp->mhp, mtmp->mhpmax/2+1);
							mtmp->movement = 0;
							continue;
						}
						else if(mtmp->entangled_otyp == MUMMY_WRAPPING){
							if(which_armor(mtmp, W_ARMC) && which_armor(mtmp, W_ARMC)->otyp == MUMMY_WRAPPING){
								if(canseemon(mtmp)) pline("%s struggles against %s wrappings", Monnam(mtmp), hisherits(mtmp));
								mtmp->movement = 0;
								continue;
							} else {
								mtmp->entangled_otyp = 0;
								mtmp->entangled_oid = 0;
							}
						}
						else if(mtmp->entangled_otyp == PRAYER_WARDED_WRAPPING){
							if(which_armor(mtmp, W_ARMC) && which_armor(mtmp, W_ARMC)->otyp == PRAYER_WARDED_WRAPPING){
								if(canseemon(mtmp)) pline("%s struggles against %s wrappings", Monnam(mtmp), hisherits(mtmp));
								mtmp->movement = 0;
								continue;
							} else {
								mtmp->entangled_otyp = 0;
								mtmp->entangled_oid = 0;
							}
						}
						else if(!mbreak_entanglement(mtmp)){
							mtmp->movement = 0;
							mescape_entanglement(mtmp);
							continue;
						}
					}
				}
				/* Possibly wake up */
				if(mtmp->mtame && mtmp->mstrategy&STRAT_WAITMASK){
					mtmp->mstrategy &= ~STRAT_WAITMASK;
				}
				/* Possibly become hostile */
				if(mtmp->mpeacetime && !mtmp->mtame){
					mtmp->mpeacetime--;
					if(!mtmp->mpeacetime) mtmp->mpeaceful = FALSE;
				}
				/* Possibly change hostility */
				if(mtmp->mtyp == PM_SURYA_DEVA){
					struct monst *blade;
					for(blade = fmon; blade; blade = blade->nmon)
						if(blade->mtyp == PM_DANCING_BLADE && mtmp->m_id == blade->mvar_suryaID) break;
					if(blade){
						if(mtmp->mtame && !blade->mtame){
							if(blade == nxtmon) nxtmon = nxtmon->nmon;
							tamedog(blade, (struct obj *) 0);
						} else if(!mtmp->mtame && blade->mtame){
							untame(blade, mtmp->mpeaceful);
						}
						if(mtmp->mpeaceful != blade->mpeaceful){
							mtmp->mpeaceful = blade->mpeaceful;
						}
					}
				}
				/*Contaminated patients become something else*/
				if(mtmp->mtyp == PM_CONTAMINATED_PATIENT && !templated(mtmp) &&
					!mtmp->mpeaceful && rn2(mtmp->mhpmax) > mtmp->mhp && 
					mon_can_see_you(mtmp)
				){
					switch(rn2(6)){
						case 0:
							if(canseemon(mtmp))
								pline("Ghostly leeches burrow out of %s face!", s_suffix(mon_nam(mtmp)));
							set_mon_data(mtmp, PM_HUMAN);
							set_template(mtmp, DREAM_LEECH);
						break;
						case 1:
							if(canseemon(mtmp))
								pline("%s jaundices suddenly!", Monnam(mtmp));
							set_template(mtmp, YELLOW_TEMPLATE);
						break;
						case 2:
							if(canseemon(mtmp))
								pline("%s rots down to the bones!", Monnam(mtmp));
							set_mon_data(mtmp, PM_HUMAN);
							set_template(mtmp, SKELIFIED);
						break;
						case 3:
							if(canseemon(mtmp))
								pline("%s screams and writhes. You hear %s bones splintering!", Monnam(mtmp), mhis(mtmp));
							else You_hear("screaming.");
							set_mon_data(mtmp, PM_COILING_BRAWN);
							possibly_unwield(mtmp, FALSE);	/* might lose use of weapon */
							mon_break_armor(mtmp, FALSE);
						break;
						case 4:
							if(canseemon(mtmp))
								pline("%s head splits open in a profusion of fungal growths!", s_suffix(Monnam(mtmp)));
							set_mon_data(mtmp, PM_FUNGAL_BRAIN);
							possibly_unwield(mtmp, FALSE);	/* might lose use of weapon */
							mon_break_armor(mtmp, FALSE);
						break;
						case 5:
							if(canseemon(mtmp))
								pline("%s skin peels open!", Monnam(mtmp));
							set_mon_data(mtmp, PM_BYAKHEE);
							possibly_unwield(mtmp, FALSE);	/* might lose use of weapon */
							mon_break_armor(mtmp, FALSE);
						break;
					}
				}

				/*Reset fracture flag*/
				if(mtmp->zombify && is_kamerel(mtmp->data)) mtmp->zombify = 0;

				if(mtmp->mtyp == PM_ARA_KAMEREL) flags.goldka_level=1;
				if(mtmp->mtyp == PM_ASPECT_OF_THE_SILENCE){
					flags.silence_level=1;
					losepw(3);
				}
				if(mtmp->mtyp == PM_GRAY_DEVOURER && Catapsi < 13) incr_itimeout(&Catapsi, 13-Catapsi);
				if(mtmp->mtyp == PM_ZUGGTMOY) flags.spore_level=1;
				if(mtmp->mtyp == PM_JUIBLEX) flags.slime_level=1;
				if(mtmp->mtyp == PM_PALE_NIGHT || mtmp->mtyp == PM_DREAD_SERAPH || mtmp->mtyp == PM_LEGION) flags.walky_level=1;
				if(mtmp->mtyp == PM_ORCUS || mtmp->mtyp == PM_NAZGUL) flags.shade_level=1;
				if(mtmp->mtyp == PM_STRANGER) flags.yello_level=1;
				if(mtmp->mtyp == PM_SUZERAIN) flags.yello_level=1;
				if(mtmp->mtyp == PM_SERPENT_PRIEST || mtmp->mtyp == PM_FEATHERY_SERPENT_PRIESTESS) flags.spriest_level=1;
				if(mtmp->mtyp == PM_HMNYW_PHARAOH) dark_pharaoh_visible(mtmp);
				if(mtmp->mtyp == PM_GOOD_NEIGHBOR) good_neighbor_visible(mtmp);
				if(mtmp->mtyp == PM_DREAD_SERAPH && (mtmp->mstrategy & STRAT_WAITMASK) && (u.uevent.invoked || Infuture)){
					mtmp->mstrategy &= ~STRAT_WAITMASK;
					pline_The("entire %s is shaking around you!",
						   In_endgame(&u.uz) ? "plane" : "dungeon");
					do_earthquake(mtmp->mx, mtmp->my, min(((int)mtmp->m_lev - 1) / 3 + 1,24), 3, TRUE, mtmp);
					if(Infuture){
						digcrater(mtmp);
					} else if(Is_lolth_level(&u.uz)){
						digcloudcrater(mtmp);
					} else if(rn2(2)){ //Do for x
						digXchasm(mtmp);
					} else { //Do for y
						digYchasm(mtmp);
					}
				}
				
				if(mtmp->mtyp == PM_DEEPEST_ONE && !mtmp->female && u.uevent.ukilled_dagon && !(In_quest(&u.uz) && Role_if(PM_ANACHRONONAUT))){
					if(mtmp->m_lev < mons[PM_FATHER_DAGON].mlevel)
						mtmp->m_lev = mons[PM_FATHER_DAGON].mlevel;
					set_mon_data(mtmp, PM_FATHER_DAGON);
					mtmp->mhp = mtmp->mhpmax = (mtmp->m_lev*hd_size(mtmp->data))-1;
					u.uevent.ukilled_dagon = 0;
					if(canspotmon(mtmp)){
						TRANSCENDENCE_IMPURITY_UP(FALSE)
						IMPURITY_UP(u.uimp_deep_one)
					}
				}
				if(mtmp->mtyp == PM_DEEPEST_ONE && mtmp->female && u.uevent.ukilled_hydra && !(In_quest(&u.uz) && Role_if(PM_ANACHRONONAUT))){
					if(mtmp->m_lev < mons[PM_MOTHER_HYDRA].mlevel)
						mtmp->m_lev = mons[PM_MOTHER_HYDRA].mlevel;
					set_mon_data(mtmp, PM_MOTHER_HYDRA);
					mtmp->mhp = mtmp->mhpmax = (mtmp->m_lev*hd_size(mtmp->data))-1;
					u.uevent.ukilled_hydra = 0;
					if(canspotmon(mtmp)){
						TRANSCENDENCE_IMPURITY_UP(FALSE)
						IMPURITY_UP(u.uimp_deep_one)
					}
				}
				if(mtmp->mtyp == PM_GOLD_GOLEM){
					int golds = u.goldkamcount_tame + level.flags.goldkamcount_peace + level.flags.goldkamcount_hostile;
					if(golds > 0){
						if(canseemon_eyes(mtmp)){
							pline("%s blossoms into a swirl of mirrored arcs!", Monnam(mtmp));
							You("see the image of %s reflected in the golden mirrors!", an(mons[PM_ARA_KAMEREL].mname));
						}
						set_mon_data(mtmp, PM_ARA_KAMEREL);
						mtmp->m_lev = 15;
						mtmp->mhpmax = d(15, hd_size(mtmp->data));
						mtmp->mhp = mtmp->mhpmax;
						if(M_HAS_NAME(mtmp)) mtmp = christen_monst(mtmp, ""); //Now a different entity
						mtmp->movement = 9;//Don't pause for a turn
						golds = rnd(golds);
						
						golds -= u.goldkamcount_tame;
						if(golds <= 0){
							u.goldkamcount_tame--;
							if(!mtmp->mtame)
								mtmp = tamedog(mtmp, (struct obj *) 0);
							newsym(mtmp->mx,mtmp->my);
							if(!mtmp)
								continue; //something went wrong, and now mtmp is no good
							goto karemade;
						}
						golds -= level.flags.goldkamcount_peace;
						if(golds <= 0){
							level.flags.goldkamcount_peace--;
							untame(mtmp, TRUE);
							newsym(mtmp->mx,mtmp->my);
							goto karemade;
						}
						level.flags.goldkamcount_hostile--;
						untame(mtmp, FALSE);
						newsym(mtmp->mx,mtmp->my);
karemade:						
						set_malign(mtmp);
					}
				}

				if(BlowingWinds && !mtmp->mtame){
					static long lastbwmessage = 0L;
					if(lastbwmessage != monstermoves && canspotmon(mtmp)){
						lastbwmessage = monstermoves;
						pline("Hurricane-force winds surround you!");
					}
					mhurtle(mtmp, rn2(3)-1, rn2(3)-1, rnd(9), FALSE);
				}
				if(DEADMONSTER(mtmp) || MIGRATINGMONSTER(mtmp))
					continue;

				if(mtmp->mtyp == PM_WALKING_DELIRIUM && !mtmp->mtame && !BlockableClearThoughts) {
					static long lastusedmove = 0;
					if (lastusedmove != moves) {
						if (!mtmp->mappearance || (canseemon(mtmp) && distmin(mtmp->mx, mtmp->my, u.ux, u.uy) <= 1 && rn2(3))) {

							if(canseemon(mtmp) && distmin(mtmp->mx, mtmp->my, u.ux, u.uy) <= 1) {
								pline("%s takes on forms new and terrible!", Monnam(mtmp));
    						change_usanity(u_sanity_loss_minor(mtmp), !mtmp->mpeaceful);
							}
							lastusedmove = moves;
							mtmp->mappearance = select_newcham_form(mtmp);
							mtmp->m_ap_type = M_AP_MONSTER;
						}
					}
				}

				if(has_template(mtmp, MAD_TEMPLATE) && !BlockableClearThoughts && canseemon(mtmp) && dimness(mtmp->mx, mtmp->my) <= 0 && distmin(mtmp->mx, mtmp->my, u.ux, u.uy) <= 1){
					static long lastusedmove = 0;
					static int lastcost = 0;
					int sancost = u_sanity_loss_minor(mtmp);
					if(lastusedmove != moves){
						lastcost = 0;
					}
					if(sancost < lastcost){
						sancost = sancost - lastcost;
						lastcost += sancost;
						pline("%s shadow takes on forms new and terrible!", s_suffix(Monnam(mtmp)));
						lastusedmove = moves;
						change_usanity(sancost, TRUE);
					}
				}

				if(mtmp->mtyp == PM_DREADBLOSSOM_SWARM){
					if(canseemon(mtmp) || u.ustuck == mtmp) mtmp->movement += mcalcmove(mtmp);
					else {
						struct monst *tmpm;
						for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
							if(!(is_fey(tmpm->data) || is_plant(tmpm->data)) && mon_can_see_mon(tmpm,mtmp)){
								mtmp->movement += mcalcmove(mtmp);
								break;
							}
						}
					}
				} else mtmp->movement += mcalcmove(mtmp);

				if(mtmp->mtyp == PM_NITOCRIS){
					if(which_armor(mtmp, W_ARMC) && which_armor(mtmp, W_ARMC)->oartifact == ART_SPELL_WARDED_WRAPPINGS_OF_)
						mtmp->mspec_used = 1;
				}

				if(mtmp->moccupation && !occupation){
					mtmp->moccupation = 0;
					mtmp->mcanmove = 1;
				}
				if(!mtmp->mnotlaugh){
					if(!is_silent_mon(mtmp) && !mtmp->msleeping && !mtmp->mfrozen){
						wake_nearto_noisy(mtmp->mx, mtmp->my, combatNoise(mtmp->data));
						if(sensemon(mtmp) || (canseemon(mtmp) && !mtmp->mundetected)){
							pline("%s is laughing hysterically.", Monnam(mtmp));
						} else if(couldsee(mtmp->mx,mtmp->my)){
							static long lastHeard = 0;
							if(lastHeard < monstermoves){
								lastHeard = monstermoves;
								You_hear("hysterical laughter.");
							}
						} else {
							static long lastHeard = 0;
							if(lastHeard < monstermoves){
								You_hear("laughter in the distance.");
								lastHeard = monstermoves;
							}
						}
					} else if(sensemon(mtmp) || (canseemon(mtmp) && !mtmp->mundetected))
						pline("%s is trembling hysterically.", Monnam(mtmp));
				}
//ifdef BARD
				if (mtmp->encouraged && (!rn2(8))) {
					if(mtmp->encouraged > 0) mtmp->encouraged--;
					else mtmp->encouraged++;
					if (!(mtmp->encouraged) && canseemon(mtmp) && mtmp->mtame) 
						pline("%s looks calm again.", Monnam(mtmp));
				}
//endif
				if(mtmp->mtyp == PM_GREAT_CTHULHU || mtmp->mtyp == PM_ZUGGTMOY 
					|| mtmp->mtyp == PM_SWAMP_FERN) mtmp->mspec_used = 0;
				if(is_weeping(mtmp->data)) mtmp->mspec_used = 0;
				if(is_random_vectored_mtyp(mtmp->mtyp) && rn2(2))
					mtmp->mvar_vector = ((int)mtmp->mvar_vector + rn2(3) + 7)%8;
				if(is_guided_vectored_mtyp(mtmp->mtyp) && !rn2(3)){
					int mdx=0, mdy=0, i;
					if(mtmp->mux == 0 && mtmp->muy == 0){
						i = rn2(8);
						mdx = xdir[i];
						mdy = ydir[i];
					} else {
						if(mtmp->mux - mtmp->mx < 0) mdx = -1;
						else if(mtmp->mux - mtmp->mx > 0) mdx = +1;
						if(mtmp->muy - mtmp->my < 0) mdy = -1;
						else if(mtmp->muy - mtmp->my > 0) mdy = +1;
						for(i=0;i<8;i++) if(xdir[i] == mdx && ydir[i] == mdy) break;
					}
					if(mtmp->mvar_vector != i){
						if(sensemon(mtmp) || (canseemon(mtmp) && !mtmp->mundetected)){
							pline("%s turns to a new heading.", Monnam(mtmp));
						} else if(couldsee(mtmp->mx,mtmp->my)){
							You_hear("a loud scraping noise.");
						} else {
							You_hear("scraping in the distance.");
						}
						mtmp->mvar_vector = i;
						mtmp->movement = -12;
					}
				}
			} /* movement rations */
			
			static boolean LBbreach = FALSE;
			static boolean LBproxim = FALSE;
			static boolean LBperim = FALSE;
			if(Infuture && Is_qstart(&u.uz) && !(quest_status.leader_is_dead)){
				for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) if(!mtmp->mpeaceful && mtmp->mx <= 23) break;
				if(mtmp && !(LBbreach && moves%5)) {
					verbalize("**EMERGENCY ALERT: hostile entities detected within Last Bastion**");
					LBbreach = TRUE;
					if(!rn2(ANA_HOME_PROB)) (void) makemon((struct permonst *)0, xdnstair, ydnstair, MM_ADJACENTOK);
					for(int x = 0; x < COLNO; x++){
						for(int y = 0; y < ROWNO; y++){
							if(levl[x][y].typ == SDOOR){
								levl[x][y].typ = DOOR;
								levl[x][y].doormask = D_CLOSED | (levl[x][y].doormask & D_TRAPPED);
								newsym(x,y);
							}
						}
					}
				} else if(!mtmp) LBbreach = FALSE;
				
				for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) if(!mtmp->mpeaceful && mtmp->mx <= 26 && mtmp->mx > 23) break;
				if(mtmp && !LBbreach && !(LBproxim && moves%10)) {
					verbalize("*PROXIMITY ALERT: hostile entities detected outside Last Bastion*");
					LBproxim = TRUE;
				} else if(!mtmp) LBproxim = FALSE;
				
				for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) if(!mtmp->mpeaceful && mtmp->mx <= 55 && mtmp->mx > 26) break;
				if(mtmp && !LBperim) {
					if(mtmp) verbalize("*PERIMETER ALERT: hostile entities closing on Last Bastion*");
					LBperim = TRUE;
				} else if(!mtmp) LBperim = FALSE;
			}
			//Center of All may show up
			if(!Infuture && !mvitals[PM_CENTER_OF_ALL].died && !rn2(COA_PROB)){
				coa_arrive();
			}
			//Dream wolf attack
			if(!mvitals[PM_MOON_S_CHOSEN].died && urole.neminum == PM_INDEX_WOLF && check_insight()){
				You("are surrounded by howling wind!");
				dream_wolves(u.ux, u.uy);
			}
			//The current move is the last time we checked random monsters
			// Ana quest is in its own timeline
			if(Role_if(PM_ANACHRONONAUT) && Infuture)
				level.lastmove = quest_status.time_doing_quest;
			else level.lastmove = monstermoves;
			//One aspect always shows up right away
			int spawn_freq = random_frequency();
			//One aspect always shows up right away
			if(In_mithardir_terminus(&u.uz) &&
				mvitals[PM_ASPECT_OF_THE_SILENCE].born == 0 &&
				((u.ufirst_light && u.ufirst_sky)
				|| (u.ufirst_light && u.ufirst_life)
				|| (u.ufirst_sky && u.ufirst_life)
			)){
				makemon(&mons[PM_ASPECT_OF_THE_SILENCE], 0, 0, NO_MM_FLAGS);
			}
			//Random monster generation block
		    else if(spawn_freq && !rn2(spawn_freq)){
				spawn_random_monster();
				if (u.uevent.invoked && !Is_ford_level(&u.uz) && xupstair && rn2(10)) {
					(void) makemon((struct permonst *)0, xupstair, yupstair, MM_ADJACENTOK);
				} //TEAM ATTACKS
				if(!(mvitals[PM_HOUND_OF_TINDALOS].mvflags&G_GONE && !In_quest(&u.uz))
					&& (level_difficulty()+u.ulevel)/2+5 > monstr[PM_HOUND_OF_TINDALOS]
					&& !DimensionalLock
					&& check_insight()
				){
					int x, y;
					int cx, cy;
					char messaged = FALSE;
					for(x = 1; x < COLNO; x++)
						for(y = 0; y < ROWNO; y++){
							if(IS_CORNER(levl[x][y].typ) && rn2(20) < u.ulevel){
								//x,y is an angle, but we want to place the *monster* on a floor space NEXT to the corner.
								for(cx = x-1; cx < x+2; cx+=2)
									for(cy = y-1; cy < y+2; cy+=2){
										//If this "corner" is off map, continue
										if(!isok(cx,cy) || !IS_ROOM(levl[cx][cy].typ) || !couldsee(cx, cy))
											continue;
										//Is this not a corner?
										if(!IS_WALL(levl[x][cy].typ) || !IS_WALL(levl[cx][y].typ))
											continue;
										create_fog_cloud(cx, cy, 2, 30, FALSE);
										makemon(&mons[PM_HOUND_OF_TINDALOS], cx, cy, MM_ADJACENTOK);
									}
							}
						}
				}
				if(flat_mad_turn(MAD_REACHER) && check_insight() && !DimensionalLock){
					struct monst* mtmp;
					boolean messaged = FALSE;
					for(int i = 0; i < 7; i++){
						mtmp = makemon(&mons[rn2(3) ? PM_LURKING_HAND : PM_BLASPHEMOUS_HAND], u.ux-7+rn2(15), u.uy-7+rn2(15), MM_NOCOUNTBIRTH|MM_ADJACENTOK|NO_MINVENT|MM_ESUM);
						if(mtmp){
							if(canspotmon(mtmp) && !messaged){
								pline("A terrible hand reaches into the world!");
								messaged = TRUE;
								TRANSCENDENCE_IMPURITY_UP(FALSE)
							}
							mark_mon_as_summoned(mtmp, (struct monst *)0, NightmareAware_Insanity+Insight, 0);
						}
					}
				}
				if(u.uz.flags.walkers < 3 && rnd(100)+3 < u.uz.rage && roll_generic_flat_madness(TRUE) && rnd(88)+12 < Insight){
					mtmp = makemon(&mons[PM_RAGE_WALKER], 0, 0, MM_ADJACENTOK);
					if(mtmp){
						make_rage_walker_polts(u.uz.rage+3);
						u.uz.rage = 0;
					}
					int i;
					int vort[] = {PM_ICE_VORTEX, PM_ENERGY_VORTEX, PM_FIRE_VORTEX};
					for(i = d(3,3); i > 0; i--) makemon(&mons[ROLL_FROM(vort)], 0, 0, NO_MM_FLAGS);
					int sphere[] = {PM_FREEZING_SPHERE, PM_FLAMING_SPHERE, PM_SHOCKING_SPHERE};
					for(i = d(3,3); i > 0; i--) makemon(&mons[ROLL_FROM(sphere)], 0, 0, NO_MM_FLAGS);
				}
			}
			if(Infuture && !(Is_qstart(&u.uz) && !Race_if(PM_ANDROID)) && !rn2(35)){
				struct monst* mtmp = makemon(&mons[PM_SEMBLANCE], rn1(COLNO-3,2), rn1(ROWNO-3,2), MM_ADJACENTOK);
				//"Where stray illuminations from the Far Realm leak onto another plane, matter stirs at the beckoning of inexplicable urges before burning to ash."
				if(mtmp && canseemon(mtmp)) pline("The base matter of the world stirs at the beckoning of inexplicable urges, dancing with a semblance of life.");
			}
			/* Rage-walker rage quickly fades. */
			if(u.uz.rage > 0 && !rn2(u.uz.rage+9))
				u.uz.rage--;

		    /* reset summon monster block. */
			u.summonMonster = FALSE;

		    /* calculate how much time passed. */
			you_calc_movement();
			if(!(moves%10)){
				if(u.eurycounts) u.eurycounts--;
				if(u.orthocounts){
					if(u.nv_range){
						u.orthocounts -= u.nv_range;
						if(u.orthocounts < 0) u.orthocounts = 0;
					} else {
						u.orthocounts--;
					}
				}
				if(u.wimage){
					if(u.wimage >= 10){
						if(!Blind){
							exercise(A_INT, TRUE);
							exercise(A_WIS, FALSE);
						}
					} else if(!(moves%10)) u.wimage--;
				}
				if(u.umorgul){
					int i = A_CON;
					int n = u.umorgul;
					if(ACURR(A_WIS)>=ACURR(i))
						i = A_WIS;
					if(ACURR(A_CHA)>=ACURR(i))
						i = A_CHA;
					while(n-- > 0){
						exercise(i, FALSE);
					}
				}
				if(u.umummyrot){
					exercise(A_CHA, FALSE);
					exercise(A_CON, FALSE);
				}
			}
			
			//Spiraling madness: abuse Wis, and lower sanity on double-trigger
			if(roll_madness(MAD_SPIRAL)){
				exercise(A_WIS, FALSE);
				exercise(A_WIS, FALSE);
				exercise(A_WIS, FALSE);
				if(moves%5 && roll_madness(MAD_SPIRAL))
					change_usanity(-1, FALSE);
			}
			//Mind dissolution xp loss
			if(u.ulevel > 1 && u.umadness&MAD_FORGETFUL && !BlockableClearThoughts){
				int i;
				lose_experience(666*NightmareAware_Insanity*NightmareAware_Insanity/(100*100));
				for(i = 1; i < P_NUM_SKILLS; i++){
					if(roll_madness(MAD_FORGETFUL))
						lose_skill(i,1);
				}
			}
			
			if(mad_turn(MAD_HOST)){
				if(!Vomiting && roll_madness(MAD_HOST)){
					static const char *hoststrings[] = {
						"wiggling",
						"twitching",
						"squirming",
						"kicking",
						"vibrating",
						"rumbling",
						"turning",
						"quivering",
						"undulating",
						"convulsing",
						"breathing"
					};
					You("feel it %s inside your body!", hoststrings[rn2(SIZE(hoststrings))]);
					make_vomiting(Vomiting+49+d(4,11), TRUE);
				}
				if(Nightmare)
					dohost_mon(&youmonst);
			}

			if(roll_madness(MAD_ROTTING)){
				if(roll_madness(MAD_ROTTING))
					create_gas_cloud(u.ux+rn2(3)-1, u.uy+rn2(3)-1, 1, rnd(6), FALSE); //Longer-lived smoke
				else
					create_gas_cloud(u.ux+rn2(3)-1, u.uy+rn2(3)-1, 1, rnd(3), FALSE);
			}
			if(roll_madness(MAD_APOSTASY)){
				adjalign(-10);
				if(u.ualign.record < 0 && roll_madness(MAD_APOSTASY)){
					if(ALIGNLIM > 10){
						u.ualign.sins++;
					} else {
						gods_upset(u.ualign.god);
					}
				}
			}
			
			if(u.utaneggs){
				for(int i = u.utaneggs; i > 0; i--) if(!rn2(6)){
					u.utaneggs--;
					makemon(&mons[PM_STRANGE_LARVA], u.ux, u.uy, MM_ADJACENTOK|MM_ADJACENTSTRICT|NO_MINVENT|MM_NOCOUNTBIRTH);
					losehp(d(1,6), "hatching parasite", KILLED_BY_AN);
				}
			}
			
			if(u.sealsActive&SEAL_ASTAROTH && u.uinwater && !Waterproof){
				losehp(1, "rusting through", KILLED_BY);
			}
			if (u.sealsActive&SEAL_ORTHOS && !(Darksight || Catsight || Extramission)
				&& (!u.uswallow ? (dimness(u.ux, u.uy) > 0) : (uswallow_indark()))
			){
				if(Elfsight){
					if(++u.orthocounts>(5*3)) unbind(SEAL_ORTHOS,TRUE);
				} else if(Lowlightsight){
					if(++u.orthocounts>(5*2)) unbind(SEAL_ORTHOS,TRUE);
				} else {
					if(++u.orthocounts>5) unbind(SEAL_ORTHOS,TRUE);
				}
				if(u.sealsActive&SEAL_ORTHOS && Hallucination){ /*Didn't just unbind it*/
					if(u.orthocounts == 1) pline("It is now pitch black. You are likely to be eaten by a grue.");
					else pline("You are likely to be eaten by a grue.");
				} else You_feel("increasingly panicked about being in the dark!");
			    if (multi >= 0) {
					if (occupation)
						stop_occupation();
					else
						nomul(0, NULL);
				}
			}
			
			//Aprox one check per six monster-gen periods
			if(!rn2(70*6) && roll_madness(MAD_SPORES)
				&& !Race_if(PM_ANDROID) && !Race_if(PM_CLOCKWORK_AUTOMATON)
			){
				make_hallucinated(itimeout_incr(HHallucination, 100), TRUE, 0L);
				IMPURITY_UP(u.uimp_rot)
				if(roll_madness(MAD_SPORES)){//Second roll for more severe symptoms
					make_stunned(itimeout_incr(HStun, 100), TRUE);
					make_confused(itimeout_incr(HConfusion, 100), FALSE);
					losehp(u.ulevel, "brain-eating fungal spores", KILLED_BY);
					exercise(A_INT, FALSE);exercise(A_WIS, FALSE);exercise(A_CHA, FALSE);
					exercise(A_INT, FALSE);exercise(A_WIS, FALSE);exercise(A_CHA, FALSE);
					exercise(A_INT, FALSE);exercise(A_WIS, FALSE);exercise(A_CHA, FALSE);
					IMPURITY_UP(u.uimp_rot)
				}
				else {
					make_confused(itimeout_incr(HConfusion, 100), TRUE);
					losehp(u.ulevel/2, "brain-eating fungal spores", KILLED_BY);
					exercise(A_INT, FALSE);
					exercise(A_WIS, FALSE);
					exercise(A_CHA, FALSE);
				}
			}
			
			if(has_blood(youracedata) && u.usanity < 50 && roll_madness(MAD_FRENZY)){
				int *hp = (Upolyd) ? (&u.mh) : (&u.uhp);
				Your("%s leaps through your %s!", body_part(BLOOD), body_part(BODY_SKIN));
				//reduce current HP by 30% (round up, guranteed nonfatal)
				if(ACURR(A_CON) > 3)
					(void)adjattrib(A_CON, -1, FALSE);
				*hp = *hp*.7+1;
				u.umadness &= ~MAD_FRENZY;
				flags.botl = 1;
			} else if(u.umadness&MAD_FRENZY){
				change_usanity(-1, FALSE);
				if(!rn2(20)){
					u.umadness &= ~MAD_FRENZY;
				}
			}
			
			if(!u.uinvulnerable){
				if(u.umadness&MAD_SCORPIONS){
					change_usanity(-1, FALSE);
					phantom_scorpions_sting(&youmonst);
					if(!rn2(20)){
						u.umadness &= ~MAD_SCORPIONS;
					}
				}
				if(youmonst.mcaterpillars){
					rot_caterpillars_bite(&youmonst);
					pline_The("parasitic caterpillars have rotted to death!");
					if(!rn2(20)){
						youmonst.mcaterpillars = FALSE;
					}
				}
				if(youmonst.momud){
					orc_mud_stabs(&youmonst);
					if(!rn2(20)){
						pline_The("writhing mud covering you has died.");
						youmonst.momud = FALSE;
						struct obj *daggers = mksobj(ORCISH_DAGGER, NO_MKOBJ_FLAGS);
						curse(daggers);
						daggers->opoisoned = OPOISON_ACID;
						daggers->quan = d(3,8);
						set_obj_size(daggers, MZ_TINY);
						set_material_gm(daggers, BONE);
						place_object(daggers, u.ux, u.uy);
					}
				}
			}
			
			if(!rn2(8) && !flaming(youracedata) && roll_madness(MAD_COLD_NIGHT)){
				if(u.usanity <= 88 && !rn2(11)){
					if(!Breathless && !Blind)
						Your("breath crystallizes in the airless void!");
					FrozenAir += (100-u.usanity)/12;
					delayed_killer = "the cold night";
					TRANSCENDENCE_IMPURITY_UP(FALSE)
					nomul(0, NULL);
				}
				roll_av_frigophobia();
				if(!UseInvCold_res(&youmonst) || !rn2(11)){
					destroy_item(&youmonst, POTION_CLASS, AD_COLD);
				}
			} else if(FrozenAir){
				roll_av_frigophobia();
				if(!UseInvCold_res(&youmonst) || !rn2(11)){
					destroy_item(&youmonst, POTION_CLASS, AD_COLD);
				}
			}
			
			if(!rn2(9) && roll_madness(MAD_OVERLORD)){
				You("feel its burning gaze upon you!");
				u.ustdy += 9;
				if(!UseInvFire_res(&youmonst) || !rn2(11)){
					destroy_item(&youmonst, POTION_CLASS, AD_FIRE);
					destroy_item(&youmonst, SCROLL_CLASS, AD_FIRE);
				}
				if(!rn2(11) && roll_madness(MAD_OVERLORD)){
					u.ustdy += 90;
					destroy_item(&youmonst, SPBOOK_CLASS, AD_FIRE);
					TRANSCENDENCE_IMPURITY_UP(FALSE)
				}
				nomul(0, NULL);
			}

			if(!rn2(7) && (Is_bridge_temple(&u.uz) || !rn2(11)) && roll_madness(MAD_REACHER)){
				You("feel its multitudinous gaze upon you!");
				if(!UseInvShock_res(&youmonst) || (!rn2(11) && Is_bridge_temple(&u.uz))){
					destroy_item(&youmonst, WAND_CLASS, AD_ELEC);
					destroy_item(&youmonst, RING_CLASS, AD_ELEC);
				}
				if(roll_madness(MAD_REACHER)){
					explode_yours(u.ux, u.uy, AD_EELC, MON_EXPLODE, d(4,8), EXPL_BBLUE, 2, FALSE);
				}
				nomul(0, NULL);
			}

			if(!rn2(10) && NightmareAware_Insanity >= 10 && roll_madness(MAD_VERMIN)){
				int damage = d(NightmareAware_Insanity/10, 10);
				damage -= u.udr;
				damage = reduce_dmg(&youmonst, damage, TRUE, FALSE);
				if(damage > 0){
					You("are stung by swarming vermin!");
					losehp(damage,"swarming vermin",KILLED_BY);
				}
				nomul(0, NULL);
			}

			//Aprox one check per five monster-gen periods, or one per five while sleeping (averages one additional blast per sleep, so it's really bad.
			if(!Inhell && (u.usleep || !rn2(70)) && !rn2(5) && roll_madness(MAD_DREAMS)){
				cthulhu_mind_blast();
			}
			
			if(u.uentangled_oid){ //Note: the normal speed calculations include important hunger modifiers, so just calculate speed then 0 it out if needed.
				entangle_effects(&youmonst);
				uescape_entanglement();
			}
			
		    settrack();

		    monstermoves++;
		    moves++;
			if(Is_qstart(&u.uz) && !(monstermoves%20) && quest_status.time_on_home < MAX_HOME_TIMER)
				quest_status.time_on_home++;
			if(Role_if(PM_UNDEAD_HUNTER) && !mvitals[PM_MOON_S_CHOSEN].died){
				quest_status.time_doing_quest++;
				if(quest_status.time_doing_quest >= UH_QUEST_TIME_4 && !Is_astralevel(&u.uz) && !u.veil){
					quest_status.moon_close = TRUE; /*The moon draws close to the astral plane*/
				}
			}
			else if(Role_if(PM_ANACHRONONAUT) && Infuture){
				quest_status.time_doing_quest++;
			}
			nonce = rand();

		      /********************************/
		     /* once-per-turn things go here */
		    /********************************/
			/* Update the hour of the day */
			flags.tm_hour = getlt()->tm_hour;
			/* Environment effects */
			dust_storm();
			/* Unseen monsters may take action */
			if(check_reanimation(LAMP_PHASE)){
				for (struct obj *otmp = invent; otmp; otmp = otmp->nobj)
					if (otmp->lamplit && (
						otmp->otyp == TORCH
						|| otmp->otyp == SHADOWLANDER_S_TORCH
						|| otmp->otyp == SUNROD
						|| otmp->otyp == OIL_LAMP
						|| otmp->otyp == POT_OIL
						|| otmp->otyp == WAX_CANDLE
						|| otmp->otyp == TALLOW_CANDLE
						|| otmp->otyp == GNOMISH_POINTY_HAT
						|| otmp->otyp == LANTERN
						|| otmp->otyp == LANTERN_PLATE_MAIL
						|| otmp->otyp == DWARVISH_HELM
						)
					){
						if((HPasses_walls&TIMEOUT) < 2)
							incr_itimeout(&HPasses_walls, 2-(HPasses_walls&TIMEOUT));
						break;
					}
			}
			/* Unseen monsters may take action */
			for(mtmp = migrating_mons; mtmp; mtmp = nxtmon){
				nxtmon = mtmp->nmon;
				if(mtmp->mux == u.uz.dnum && mtmp->muy == u.uz.dlevel && mtmp->marriving)
					mon_arrive_on_level(mtmp);
				else
					unseen_actions(mtmp); //May cause mtmp to be removed from the migrating chain
			}
			
			/* Item attacks */
			living_items();
			dosymbiotic_equip();
			if(u.spiritPColdowns[PWR_PSEUDONATURAL_SURGE] >= moves+20)
				dopseudonatural();
			do_passive_attacks();
			if(Destruction)
				dodestruction();
			if(Mindblasting)
				domindblast_strong();
			/* Clouds on Lolth's level deal damage */
			if(Is_lolth_level(&u.uz) && levl[u.ux][u.uy].typ == CLOUD){
				if (!(nonliving(youracedata) || Breathless)){
					if (!Blind)
						make_blinded((long)rnd(4), FALSE);
					if (!Poison_resistance) {
						pline("%s is burning your %s!", Something, makeplural(body_part(LUNG)));
						You("cough and spit blood!");
						losehp(d(3,8) + ((Amphibious && !flaming(youracedata)) ? 0 : rnd(6)), "stinking cloud", KILLED_BY_AN);
					} else if (!(Amphibious && !flaming(youracedata))){
						You("are laden with moisture and %s",
							flaming(youracedata) ? "are smoldering out!" :
							Breathless ? "find it mildly uncomfortable." :
							amphibious(youracedata) ? "feel comforted." :
							"can barely breathe!");
						/* NB: Amphibious includes Breathless */
						if (!(Amphibious && !flaming(youracedata))) losehp(rnd(6), "suffocating in a cloud", KILLED_BY);
					} else {
						You("cough!");
					}
				} else {
					You("are laden with moisture and %s",
						flaming(youracedata) ? "are smoldering out!" :
						Breathless ? "find it mildly uncomfortable." :
						amphibious(youracedata) ? "feel comforted." :
						"can barely breathe!");
					/* NB: Amphibious includes Breathless */
					if (!(Amphibious && !flaming(youracedata))) losehp(rnd(6), "suffocating in a cloud", KILLED_BY);
				}
			}
			/* If the player has too many pets, untame them untill that is no longer the case */
			{
				// finds weakest pet, and if there's more than 6 pets that count towards your limit
				// it sets the weakest one friendly - dog.c
				enough_dogs(0);
				
				// if there's a spiritual pet that isn't already marked for vanishing,
				// give it 5 turns before it disappears. - dog.c
				vanish_dogs();
			}
			
			if(u.petattacked){
				u.petattacked = FALSE;
				use_skill(P_BEAST_MASTERY, 1);
			}
			if(u.pethped){
				u.pethped = FALSE;
				more_experienced(u.ulevel,0);
				newexplevel();
			}
			if (u.uencouraged && (!rn2(8))) {
				if(u.uencouraged > 0) u.uencouraged--;
				else u.uencouraged++;
				if (!(u.uencouraged)) 
					You_feel("calm again.");
			}

		    if (flags.bypasses) clear_bypasses();
		    if(Glib) glibr();
			if (uswapwep && !Weldproof && uswapwep->cursed && u.twoweap) {
				uswapwep->bknown = TRUE;
				drop_uswapwep();
			}
		    // if(StumbleBlind && rn2(100) >= NightmareAware_Sanity) bumbler();
		    nh_timeout();
		    run_regions();
		    run_maintained_spells();
			
			move_gliders();

		    if (u.ublesscnt)  u.ublesscnt--;
		    if(flags.time && !flags.run)
			flags.botl = 1;
			
		    if ((prev_dgl_extrainfo == 0) || (prev_dgl_extrainfo < (moves + 250))) {
				prev_dgl_extrainfo = moves;
				mk_dgl_extrainfo();
		    }
			if(uclockwork){
				if(u.ustove){
					if(u.uboiler){
						int steam = min(10,min(u.ustove,u.uboiler));
						lesshungry(steam);
						u.ustove -= min(10,u.ustove);
						u.uboiler-=steam;
						create_gas_cloud(u.ux+rn2(3)-1, u.uy+rn2(3)-1, 1, rnd(3), FALSE);
						if(u.utemp && moves%2) u.utemp--;
					} else {
						if(u.utemp<=WARM || 
							(u.utemp<=HOT && (moves%4)) ||
							(u.utemp>HOT && u.utemp<=BURNING_HOT && (moves%3)) ||
							(u.utemp>BURNING_HOT && u.utemp<=MELTING && (moves%2)) ||
							(u.utemp>MELTING && u.utemp<=MELTED && !(moves%3)) ||
							(u.utemp>MELTED && !(moves%4))
						) u.utemp++;
						create_gas_cloud(u.ux+rn2(3)-1, u.uy+rn2(3)-1, 1, rnd(6), FALSE); //Longer-lived smoke
						u.ustove -= min(10,u.ustove);
					}
				} else if(u.utemp) u.utemp--;
				if(u.utemp > BURNING_HOT){
					if (!InvFire_resistance){
						if((u.utemp-5)*2 > rnd(10)) destroy_item(&youmonst, SCROLL_CLASS, AD_FIRE);
						if((u.utemp-5)*2 > rnd(10)) destroy_item(&youmonst, POTION_CLASS, AD_FIRE);
						if((u.utemp-5)*2 > rnd(10)) destroy_item(&youmonst, SPBOOK_CLASS, AD_FIRE);
					}
					
					if(u.utemp >= MELTING && !(HFire_resistance || u.sealsActive&SEAL_FAFNIR)){
						Your("boiler is melting!");
						losehp(u.ulevel, "melting from extreme heat", KILLED_BY);
						if(u.utemp >= MELTED){
							Your("boiler has melted to slag!");
							if(Upolyd) losehp(u.mhmax*2, "melting to slag", KILLED_BY);
							else { /* May have been rehumanized by previous damage. In that case, still die from left over bronze on your skin! */
								losehp((Upolyd ? u.mhmax : u.uhpmax)*2, "melting to slag", KILLED_BY);
								u.utemp = 19; // don't get stuck in a loop of permanently dying, clear temp & stove
								u.ustove = 0; // this can still kill due to you taking damage, just no instadeath
							}
						}
					}
				}
			} else if(u.utemp) u.utemp = 0;
			
			if(u.uinwater && !u.umoved){
				if(uclockwork) u.uboiler = MAX_BOILER;
			}
			
			if(u.ukinghill){
				if(u.protean > 0) u.protean--;
				else{
					for(pobj = invent; pobj; pobj=pobj->nobj)
						if(pobj->oartifact == ART_TREASURY_OF_PROTEUS)
							break;
					if(!pobj){
						pline("Treasury not actually in inventory??");
						u.ukinghill = 0;
					}
					else if(pobj->cobj){
						arti_poly_contents(pobj);
					}
					u.protean = 100 + d(10,10);
					update_inventory();
				}
			}

			// Ymir's stat regeneration
			if (u.sealsActive&SEAL_YMIR && (wtcap < MOD_ENCUMBER || !u.umoved || Regeneration)){
				if ((u.ulevel > 9 && !(moves % 3)) ||
					(u.ulevel <= 9 && !(moves % ((MAXULEV + 12) / (u.ulevel + 2) + 1)))
					){
					int val_limit, idx;
					for (idx = 0; idx < A_MAX; idx++) {
						val_limit = AMAX(idx);
						/* don't recover strength lost from hunger */
						if (idx == A_STR && u.uhs >= WEAK) val_limit--;

						if (val_limit > ABASE(idx)) ABASE(idx)++;
					}
				}
			}

		    /* One possible result of prayer is healing.  Whether or
		     * not you get healed depends on your current hit points.
		     * If you are allowed to regenerate during the prayer, the
		     * end-of-prayer calculation messes up on this.
		     * Another possible result is rehumanization, which requires
		     * that encumbrance and movement rate be recalculated.
		     */
			do_your_auras();
			
			you_regen_hp();
			you_regen_pw();
			you_regen_san();
			androidUpkeep();
			clothes_bite_you();
			mercurial_repair();
			clear_stale_fforms();

			if(moves%1000){
				extern struct hashmap_s *itemmap;
				hashmap_iterate_pairs(itemmap, filter_itemmap, 0);
			}

		    if(!(Invulnerable)) {
			if(Teleportation && !rn2(85) && !(
#ifdef WIZARD
				(
#endif
				 (u.uhave.amulet || On_W_tower_level(&u.uz)
#ifdef STEED
				  || (u.usteed && mon_has_amulet(u.usteed))
#endif
				 )
#ifdef WIZARD
				 && (!wizard) )
#endif
			)) {
			    xchar old_ux = u.ux, old_uy = u.uy;
			    tele();
			    if (u.ux != old_ux || u.uy != old_uy) {
				if (!next_to_u()) {
				    check_leash(old_ux, old_uy);
				}
#ifdef REDO
				/* clear doagain keystrokes */
				pushch(0);
				savech(0);
#endif
			    }
			}
			/* delayed change may not be valid anymore */
			if ((change == 1 && !Polymorph) ||
			    (change == 2 && u.ulycn == NON_PM))
			    change = 0;
			if(Polymorph && !rn2(100))
			    change = 1;
			else if (u.ulycn >= LOW_PM && !Upolyd &&
				 !umechanoid&&
				 !rn2(80 - (20 * night())))
			    change = 2;
			if (change && !Unchanging) {
			    if (multi >= 0) {
				if (occupation)
				    stop_occupation();
				else
				    nomul(0, NULL);
				if (change == 1) polyself(FALSE);
				else you_were();
				change = 0;
			    }
			}
		    }

		    if(Searching && multi >= 0) (void) dosearch0(1);
		    dosounds();
		    do_storms();
		    gethungry();
		    age_spells();
		    exerchk();
		    invault();
		    if (u.uhave.amulet) amulet();
		    if (!rn2(40+(int)(ACURR(A_DEX)*3)))
			u_wipe_engr(rnd(3));
		    if (u.uevent.udemigod && !(Invulnerable)) {
				if (u.udg_cnt) u.udg_cnt--;
				if (!u.udg_cnt) {
					intervene();
					u.udg_cnt = rn1(200, 50);
				}
		    }
		    if (u.uevent.ukilled_illurien && !(Invulnerable)) {
				if (u.ill_cnt) u.ill_cnt--;
				if (!u.ill_cnt) {
					illur_intervene();
					u.ill_cnt = rn1(1000, 250);
				}
		    }
		    if ((!Role_if(PM_MADMAN) || quest_status.touched_artifact) && !mvitals[PM_STRANGER].died
				&& !(Invulnerable)
			) {
				if (!u.yel_cnt) {
					if(!yello_intervene())
						u.yel_cnt = rn1(51, 5);
					else
						u.yel_cnt = rn1(501, 55);
				}
		    }
		    restore_attrib();
		    /* underwater and waterlevel vision are done here */
		    if (Is_waterlevel(&u.uz))
			movebubbles();
		    else if (Underwater)
			under_water(0);
		    /* vision while buried done here */
		    else if (u.uburied) under_ground(0);

		    /* when immobile, count is in turns */
		    if(multi < 0) {
			if (++multi == 0) {	/* finished yet? */
			    unmul((char *)0);
			    /* if unmul caused a level change, take it now */
			    if (u.utotype) deferred_goto();
			}
		    }
		}
	    } while (youmonst.movement<NORMAL_SPEED); /* hero can't move loop */

	      /******************************************/
	     /* once-per-hero-took-time things go here */
	    /******************************************/
		
		if(u.ustdy > 0) u.ustdy -= 1;
		if(u.ustdy < 0) u.ustdy += 1;
		
		if(youmonst.mopen) youmonst.mopen--;
		
////////////////////////////////////////////////////////////////////////////////////////////////
		if (flags.run_timers){
			run_timers();
		}
////////////////////////////////////////////////////////////////////////////////////////////////
		if(u.specialSealsActive&SEAL_LIVING_CRYSTAL)
			average_dogs();
		
		//Mithardir portals
		if(In_mithardir_desert(&u.uz)){
			struct trap *ttmp;
			for(ttmp = ftrap; ttmp; ttmp = ttmp->ntrap) {
				if(ttmp->ttyp == MAGIC_PORTAL && distmin(u.ux, u.uy, ttmp->tx, ttmp->ty) < 3){
					if(!ttmp->tseen){
						ttmp->tseen = 1;
						newsym(ttmp->tx, ttmp->ty);
						if(cansee(ttmp->tx, ttmp->ty)){
							You("spot a swirling portal!");
						} else {
							You_hear("swirling dust!");
						}
					}
				}
			}
		}
		
		//Echolocation
		if(Echolocation){
			for(i=1; i<COLNO; i++)
				for(j=0; j<ROWNO; j++)
					if(viz_array[j][i]&COULD_SEE)
						echo_location(i, j);
			see_monsters();
		}
		if(u.utrap && u.utraptype == TT_LAVA) {
			if(!is_lava(u.ux,u.uy))
			    u.utrap = 0;
		    else if (!(Invulnerable)) {
			    u.utrap -= 1<<8;
				if(u.utrap < 1<<8) {
					killer_format = KILLED_BY;
					killer = "molten lava";
					You("sink below the surface and die.");
					done(DISSOLVED);
				} else if(didmove && !u.umoved) {
					Norep("You sink deeper into the lava.");
					lava_effects(FALSE);
					u.utrap += rnd(4);
				}
			}
	    }
		sense_nearby_monsters();
		if(u.uinwater){//Moving around will also call this, so your stuff degrades faster if you move
			water_damage(invent, FALSE, FALSE, level.flags.lethe, &youmonst);
		}

		hpDiff -= u.uhp;
		hpDiff = (hpDiff > 0) ? hpDiff : 0;
		if(uarmg && ART_GAUNTLETS_OF_THE_BERSERKER == uarmg->oartifact){
			float a = .1; /* closer to 1 -> discard older faster */
			long next = (long)(a * hpDiff + (1 - a) * uarmg->ovara_gober);
			next = (next > 10) ? 10 : next;
			long diff = next - uarmg->ovara_gober;
			uarmg->ovara_gober = next;
			//if(diff) adj_abon(uarmg, diff);
		}
		didmove = FALSE;
	} /* actual time passed */

	/****************************************/
	/* once-per-player-input things go here */
	/****************************************/
	find_ac();
////////////////////////////////////////////////////////////////////////////////////////////////
	if (flags.run_timers){
		run_timers();
	}
////////////////////////////////////////////////////////////////////////////////////////////////
	for (mtmp = fmon; mtmp; mtmp = nxtmon){
		nxtmon = mtmp->nmon;
		if(DEADMONSTER(mtmp))
			continue;
		if(mtmp->mtyp == PM_HELLCAT){
			if(!isdark(mtmp->mx,mtmp->my) && !mtmp->minvis){
				mtmp->minvis = TRUE;
				mtmp->perminvis = TRUE;
				newsym(mtmp->mx,mtmp->my);
			} else if(isdark(mtmp->mx,mtmp->my) && mtmp->minvis){
				mtmp->minvis = FALSE;
				mtmp->perminvis = FALSE;
				newsym(mtmp->mx,mtmp->my);
			}
		} else if(mtmp->mtyp == PM_HUDOR_KAMEREL){
			if(is_pool(mtmp->mx,mtmp->my, TRUE) && !mtmp->minvis){
				mtmp->minvis = TRUE;
				mtmp->perminvis = TRUE;
				newsym(mtmp->mx,mtmp->my);
			} else if(!is_pool(mtmp->mx,mtmp->my, TRUE) && mtmp->minvis){
				mtmp->minvis = FALSE;
				mtmp->perminvis = FALSE;
				newsym(mtmp->mx,mtmp->my);
			}
		} else if(mtmp->mtyp == PM_GRUE || mtmp->mtyp == PM_INVIDIAK){
			if(isdark(mtmp->mx,mtmp->my) && !mtmp->minvis){
				mtmp->minvis = TRUE;
				mtmp->perminvis = TRUE;
				newsym(mtmp->mx,mtmp->my);
			} else if(!isdark(mtmp->mx,mtmp->my) && mtmp->minvis){
				mtmp->minvis = FALSE;
				mtmp->perminvis = FALSE;
				newsym(mtmp->mx,mtmp->my);
			}
		}
		if(mtmp->m_ap_type == M_AP_MONSTER && (BlockableClearThoughts || (!mtmp->iswiz && !(u.umadness&MAD_DELUSIONS)))){
			mtmp->m_ap_type = M_AP_NOTHING;
			mtmp->mappearance = 0;
			newsym(mtmp->mx, mtmp->my);
		}
		if(mtmp->m_insight_level > Insight && !mtmp->mcan && mtmp->mtyp == PM_TRANSCENDENT_TETTIGON){
			set_mon_data(mtmp, PM_UNMASKED_TETTIGON);
			mtmp->m_insight_level -= 35;
			newsym(x(mtmp), y(mtmp));
		}
		if(mtmp->m_insight_level > Insight
		  || (mtmp->mtyp == PM_WALKING_DELIRIUM && BlockableClearThoughts)
		  || (mtmp->mtyp == PM_STRANGER && !quest_status.touched_artifact)
		  || ((mtmp->mtyp == PM_PUPPET_EMPEROR_XELETH || mtmp->mtyp == PM_PUPPET_EMPRESS_XEDALLI) && mtmp->mvar_yellow_lifesaved)
		  || (mtmp->mtyp == PM_TWIN_SIBLING && (mtmp->mvar_twin_lifesaved || !(u.specialSealsActive&SEAL_YOG_SOTHOTH)))
		){
			if(!(mtmp->mtrapped && t_at(mtmp->mx, mtmp->my) && t_at(mtmp->mx, mtmp->my)->ttyp == VIVI_TRAP)){
				insight_vanish(mtmp);
				continue;
			}
		}
		if(has_template(mtmp, DELOUSED)){
			delouse_tame(mtmp);
			continue;
		}
		if(mtmp->mamnesia){
			if(mtmp->isshk){
				make_happy_shk(mtmp, FALSE);
				ESHK(mtmp)->pbanned = FALSE;
				ESHK(mtmp)->signspotted = 0;
			}
			else {
				mtmp->mpeaceful = TRUE;
				mtmp->mtame = FALSE;
			}
			if(mtmp->ispriest){
				EPRI(mtmp)->pbanned = FALSE;
				EPRI(mtmp)->signspotted = 0;
			}
			mtmp->mamnesia = FALSE;
			newsym(mtmp->mx, mtmp->my);
		}
		if (mtmp->minvis){
			newsym(mtmp->mx, mtmp->my);
		}
		if (Blind && Bloodsense && has_blood_mon(mtmp)){
			newsym(mtmp->mx, mtmp->my);
		}
		if (uwep && uwep->oartifact == ART_SINGING_SWORD && !is_deaf(mtmp)){
			//quite noisy
			mtmp->mux = u.ux;
			mtmp->muy = u.uy;
		}
		if (Screaming && !Strangled && !BloodDrown && !FrozenAir && !is_deaf(mtmp)){
			//quite noisy
			mtmp->mux = u.ux;
			mtmp->muy = u.uy;
			if(distmin(u.ux, u.uy, mtmp->mx, mtmp->my) < (ACURR(A_CON)/3) && sensitive_ears(mtmp->data)){
				if (!resist(mtmp, TOOL_CLASS, 0, NOTELL)){
					mtmp->mstun = 1;
					mtmp->mconf = 1;
					mtmp->mcanhear = 0;
					mtmp->mdeafened = ACURR(A_CON)/3 - distmin(u.ux, u.uy, mtmp->mx, mtmp->my);
				}
			}
		}
		//Noise radius dependent on your lung capacity
		else if (Babble && !Strangled && !FrozenAir && !is_deaf(mtmp) && distmin(u.ux, u.uy, mtmp->mx, mtmp->my) < (ACURR(A_CON)/3)){
			//quite noisy
			mtmp->mux = u.ux;
			mtmp->muy = u.uy;
		}
	}
	if (Screaming && !Strangled && !BloodDrown && !FrozenAir){
		//quite noisy
		song_noise(ACURR(A_CON)*ACURR(A_CON));
	}
////////////////////////////////////////////////////////////////////////////////////////////////
	if(!flags.mv || Blind || oldBlind != (!!Blind)) {
	    /* redo monsters if hallu or wearing a helm of telepathy */
	    if (Hallucination) {	/* update screen randomly */
			see_altars();
			see_monsters();
			see_objects();
			see_traps();
			if (u.uswallow) swallowed(0);
		} else if (Unblind_telepat || goodsmeller(youracedata) || Warning || Warn_of_mon || NightmareAware_Sanity < 100 || oldsanity < 100) {
	     	see_monsters();
	    }
		
		oldsanity = NightmareAware_Sanity;

		switch (((u_healing_penalty() - healing_penalty) > 0) - ((u_healing_penalty() - healing_penalty) < 0))
		{
		case 0:
			break;
		case 1:
			if (!Hallucination){
				You_feel("%s.", (healing_penalty) ? "itchier" : "itchy");
			}
			else {
				You_feel("%s.", (healing_penalty) ? "uglier" : "ugly");
			}
			healing_penalty = u_healing_penalty();
			break;
		case -1:
			if (!Hallucination) {
				You_feel("%s.", (u_healing_penalty()) ? "some relief" : "relieved");
			}
			else {
				You_feel("%s.", (u_healing_penalty()) ? "pretty" : "beautiful");
			}
			healing_penalty = u_healing_penalty();
			break;
		}
		
		if (!oldBlind ^ !Blind) {  /* one or the other but not both */
			see_monsters();
			flags.botl = 1;
			vision_full_recalc = 1;	/* blindness just got toggled */
			if (Blind_telepat || Infravision) see_monsters();
		}
		
	    if (vision_full_recalc) vision_recalc(0);	/* vision! */
	}
	oldBlind = !!Blind;
////////////////////////////////////////////////////////////////////////////////////////////////
	if(!flags.mv || LightBlind || oldLightBlind != (!!LightBlind)) {
	    /* redo monsters if hallu or wearing a helm of telepathy */
	    if (Hallucination) {	/* update screen randomly */
			see_altars();
			see_monsters();
			see_objects();
			see_traps();
			if (u.uswallow) swallowed(0);
		} else if (Unblind_telepat || goodsmeller(youracedata) || Warning || Warn_of_mon || NightmareAware_Sanity < 100 || oldsanity < 100) {
	     	see_monsters();
	    }
		
		oldsanity = NightmareAware_Sanity;

		if (!oldLightBlind ^ !LightBlind) {  /* one or the other but not both */
			see_monsters();
			flags.botl = 1;
			vision_full_recalc = 1;	/* blindness just got toggled */
			if (Blind_telepat || Infravision) see_monsters();
		}
		
	    if (vision_full_recalc) vision_recalc(0);	/* vision! */
	}
	oldLightBlind = !!LightBlind;
////////////////////////////////////////////////////////////////////////////////////////////////
	calc_total_maxhp();
////////////////////////////////////////////////////////////////////////////////////////////////
	calc_total_maxen();
////////////////////////////////////////////////////////////////////////////////////////////////
	/*If anything we did caused us to get moved out of water, surface*/
	if(u.usubwater && !is_pool(u.ux, u.uy, FALSE)){
		u.usubwater = 0;
		vision_full_recalc = 1;
		vision_recalc(2);	/* unsee old position */
		doredraw();
	} else if(Is_waterlevel(&u.uz) && u.usubwater && !is_3dwater(u.ux, u.uy)){
		You("pop out into an airbubble!");
		u.usubwater = 0;
	} else if(Is_waterlevel(&u.uz) && !u.usubwater && is_3dwater(u.ux, u.uy)){
		Your("%s goes under water!", body_part(HEAD));
		u.usubwater = 1;
	}
////////////////////////////////////////////////////////////////////////////////////////////////
	
	
#ifdef REALTIME_ON_BOTL
	if(iflags.showrealtime) {
		/* Update the bottom line if the number of minutes has
		 * changed */
		if(get_realtime() / 60 != realtime_data.last_displayed_time / 60)
			flags.botl = 1;
	}
#endif
  
	if(flags.botl || flags.botlx) bot();

	if (iflags.hp_notify && (prev_hp_notify != uhp())) {
	  pline("%s", hpnotify_format_str(iflags.hp_notify_fmt ? iflags.hp_notify_fmt : "[HP%c%a=%h]"));
	  prev_hp_notify = uhp();
	}

	flags.move = MOVE_DEFAULT;

	if(multi >= 0 && occupation) {
#if defined(MICRO) || defined(WIN32)
	    abort_lev = 0;
	    if (kbhit()) {
		if ((ch = Getchar()) == ABORT)
		    abort_lev++;
# ifdef REDO
		else
		    pushch(ch);
# endif /* REDO */
	    }
	    if (!abort_lev && ((flags.move = (*occupation)()) & (MOVE_CANCELLED|MOVE_FINISHED_OCCUPATION)))
#else
	    if (((flags.move = (*occupation)()) & (MOVE_CANCELLED|MOVE_FINISHED_OCCUPATION)))
#endif
		occupation = 0;
	    if(
#if defined(MICRO) || defined(WIN32)
		   abort_lev ||
#endif
		   monster_nearby()) {
		stop_occupation();
		reset_eat();
	    }
#if defined(MICRO) || defined(WIN32)
	    if (!(++occtime % 7))
		display_nhwindow(WIN_MAP, FALSE);
#endif
	    continue;
	}

	if ((u.uhave.amulet || Clairvoyant) &&
	    !In_endgame(&u.uz) && !BClairvoyant &&
	    !(moves % 15) && !rn2(2))
		do_vicinity_map(u.ux,u.uy);

#ifdef WIZARD
	if (iflags.sanity_check || iflags.debug_fuzzer)
	    sanity_check();
#endif

#ifdef CLIPPING
	/* just before rhack */
	cliparound(u.ux, u.uy);
#endif

	u.umoved = FALSE;
	u.uattked = FALSE;
	u.unull = FALSE;

	if (multi > 0) {
	    lookaround();
	    if (!multi) {
		/* lookaround may clear multi */
		flags.move |= MOVE_CANCELLED;
		if (flags.time) flags.botl = 1;
		continue;
	    }
	    if (flags.mv) {
		if(multi < COLNO && !--multi)
		    flags.travel = iflags.travel1 = flags.mv = flags.run = 0;
		domove();
	    } else {
		--multi;
		rhack(save_cm);
	    }
	} else if (multi == 0) {
	  ck_server_admin_msg();
#ifdef MAIL
	    ckmailstatus();
#endif
	    rhack((char *)0);
	}
	if (u.utotype)		/* change dungeon level */
	    deferred_goto();	/* after rhack() */
	/* !flags.move here: multiple movement command stopped */
	else if (flags.time && (!flags.move || !flags.mv))
	    flags.botl = 1;

	if (vision_full_recalc) vision_recalc(0);	/* vision! */
	/* when running in non-tport mode, this gets done through domove() */
	if ((!flags.run || iflags.runmode == RUN_TPORT) &&
		(multi && (!flags.travel ? !(multi % 7) : !(moves % 7L)))) {
	    if (flags.time && flags.run) flags.botl = 1;
	    display_nhwindow(WIN_MAP, FALSE);
	}

	if (moves > last_clear_screen + 2000) doredraw();

    } //END MAIN LOOP
}

#endif /* OVL0 */
#ifdef OVL1

void
stop_occupation()
{
	struct monst *mtmp;
	
	flags.travel = iflags.travel1 = flags.mv = flags.run = 0;

	if(occupation) {
		if (!maybe_finished_meal(TRUE))
		    You("stop %s.", occtxt);
		occupation = 0;
		flags.botl = 1; /* in case u.uhs changed */
		for (mtmp = fmon; mtmp; mtmp = mtmp->nmon){
			if(mtmp->moccupation && !occupation){
				mtmp->moccupation = 0;
				mtmp->mcanmove = 1;
			}
		}
/* fainting stops your occupation, there's no reason to sync.
		sync_hunger();
*/
#ifdef REDO
		nomul(0, NULL);
		pushch(0);
#endif
	}
}

#endif /* OVL1 */
#ifdef OVLB

void
display_gamewindows()
{
    curses_stupid_hack = 0;
    WIN_MESSAGE = create_nhwindow(NHW_MESSAGE);
    WIN_STATUS = create_nhwindow(NHW_STATUS);
    WIN_MAP = create_nhwindow(NHW_MAP);
    WIN_INVEN = create_nhwindow(NHW_MENU);

#ifdef MAC
    /*
     * This _is_ the right place for this - maybe we will
     * have to split display_gamewindows into create_gamewindows
     * and show_gamewindows to get rid of this ifdef...
     */
	if ( ! strcmp ( windowprocs . name , "mac" ) ) {
	    SanePositions ( ) ;
	}
#endif

    /*
     * The mac port is not DEPENDENT on the order of these
     * displays, but it looks a lot better this way...
     */
    display_nhwindow(WIN_STATUS, FALSE);
    display_nhwindow(WIN_MESSAGE, FALSE);
    clear_glyph_buffer();
    display_nhwindow(WIN_MAP, FALSE);
}

void
newgame()
{
	int i;

#ifdef MFLOPPY
	gameDiskPrompt();
#endif

	flags.ident = 1;

	extern struct hashmap_s *itemmap;
	itemmap = malloc(sizeof(struct hashmap_s));
	hashmap_create(32, itemmap);

	for (i = 0; i < NUMMONS; i++)
		mvitals[i].mvflags = mons[i].geno & (G_NOCORPSE|G_SPCORPSE);

	init_objects();		/* must be before u_init() */
	init_gods();		/* probably will need to be before u_init */
	id_permonst();		/* must be before u_init() */
	
	flags.pantheon = -1;	/* role_init() will reset this */
	flags.panLgod = -1;	/* role_init() will reset this */
	flags.panNgod = -1;	/* role_init() will reset this */
	flags.panCgod = -1;	/* role_init() will reset this */
	flags.panVgod = -1;	/* role_init() will reset this */
	role_init(TRUE);		/* must be before init_dungeons(), u_init(),
				 * and init_artifacts() */
	
	init_dungeons();	/* must be before u_init() to avoid rndmonst()
				 * creating odd monsters for any tins and eggs
				 * in hero's initial inventory */
	init_artifacts();	/* before u_init() in case $WIZKIT specifies
				 * any artifacts */
	u_init();
	
	hack_artifacts();	/* recall after u_init() to fix up role specific artifacts */
	hack_objects();

#ifndef NO_SIGNAL
	(void) signal(SIGINT, (SIG_RET_TYPE) done1);
#endif
#ifdef NEWS
	if(iflags.news) display_file(NEWS, FALSE);
#endif
	load_qtlist();	/* load up the quest text info */
/*	quest_init();*/	/* Now part of role_init() */

	mklev();
	u_on_upstairs();
	vision_reset();		/* set up internals for level (after mklev) */
	check_special_room(FALSE);

	flags.botlx = 1;

	/* Move the monster from under you or else
	 * makedog() will fail when it calls makemon().
	 *			- ucsfcgl!kneller
	 */
	if(MON_AT(u.ux, u.uy)) mnexto(m_at(u.ux, u.uy));
	(void) makedog();
	if(Race_if(PM_ANDROID))
		scatter_weapons();
	docrt();
	if (Role_if(PM_CONVICT) || (Role_if(PM_MADMAN) && Race_if(PM_VAMPIRE))) {
		setworn(mkobj(CHAIN_CLASS, TRUE), W_CHAIN);
		setworn(mkobj(BALL_CLASS, TRUE), W_BALL);
		uball->spe = 1;
		if (Race_if(PM_CHIROPTERAN)) uball->owt = 1600;
		placebc();
		newsym(u.ux,u.uy);
	}

	if (flags.legacy) {
		flush_screen(1);
        if(Role_if(PM_EXILE)){
			com_pager(217);
#ifdef CONVICT
		} else if (Role_if(PM_CONVICT)) {
		    com_pager(199);
#endif /* CONVICT */
        // } else if(Race_if(PM_ELF)){
			// com_pager(211);
		} else if(Race_if(PM_ELF) && (Role_if(PM_PRIEST) || Role_if(PM_RANGER) || Role_if(PM_NOBLEMAN) || Role_if(PM_WIZARD))){
			com_pager(211);
		} else if(Role_if(PM_ANACHRONONAUT)){
			com_pager(218);
			if(Race_if(PM_ANDROID)){
				com_pager(222);
				com_pager(223);
				com_pager(224);
				com_pager(225);
			} else {
				com_pager(219);
				com_pager(220);
				com_pager(221);
			}
		} else if(Race_if(PM_WORM_THAT_WALKS)){
			if(Role_if(PM_CONVICT)){
				com_pager(214);
			} else if(Race_if(PM_ELF) && (Role_if(PM_PRIEST) || Role_if(PM_RANGER) || Role_if(PM_NOBLEMAN) || Role_if(PM_WIZARD))){
				com_pager(213);
			} else {
				com_pager(212);
			}
			com_pager(215);
			com_pager(216);
		} else {
			com_pager(1);
		}
	}
	if(Darksight) litroom(FALSE,NULL);
	if(flags.descendant){
		struct obj *otmp;
		int inher_arti = find_preset_inherited(inherited);

		while (!inher_arti) inher_arti = do_inheritance_menu();

		u.inherited = inher_arti;

		/* fix up artifact a little so we can use it fine */
		/* the alignment check should be unnecessary, but otherwise this prevents intelligents from evading */
		if (!Role_if(artilist[inher_arti].role)) artilist[inher_arti].role = ROLE_NONE;
		if (!Pantheon_if(artilist[inher_arti].role)) artilist[inher_arti].role = ROLE_NONE;
		if (!Race_if(artilist[inher_arti].race)) artilist[inher_arti].race = ROLE_NONE;
		if (artilist[inher_arti].alignment != u.ualign.type) artilist[inher_arti].alignment = A_NONE;
		artilist[inher_arti].gflags &= ~ARTG_GIFT;
		artilist[inher_arti].gflags &= ~ARTG_NAME;
		artilist[inher_arti].gflags |= ARTG_NOGEN;
		artilist[inher_arti].gflags |= ARTG_NOWISH;
		hack_artifacts();

		if (!Role_if(PM_MADMAN)){
			otmp = mksobj((int)artilist[inher_arti].otyp, MKOBJ_NOINIT);
			/* please do not have any artifacts where the otyp in artilist is not the same as the practical otyp after onaming */
			discover_artifact(inher_arti);
			if (!(Role_if(PM_CONVICT)
				|| (Role_if(PM_HEALER) && Race_if(PM_DROW))
			)){
				otmp = oname(otmp, artilist[inher_arti].name);
				fully_identify_obj(otmp);
				expert_weapon_skill(weapon_type(otmp));
				otmp = hold_another_object(otmp, "Oops!  %s to the floor!",
						   The(aobjnam(otmp, "slip")), (const char *)0);
			} else {
				//Create without creating
				otmp->oartifact = inher_arti;
				expert_weapon_skill(weapon_type(otmp));
				otmp->oartifact = 0;
				delobj(otmp);
			}
		}
	}

#ifdef INSURANCE
	save_currentstate();
#endif
	program_state.something_worth_saving++;	/* useful data now exists */

#if defined(RECORD_REALTIME) || defined(REALTIME_ON_BOTL)

	/* Start the timer here */
	realtime_data.realtime = (time_t)0L;

#if defined(BSD) && !defined(POSIX_TYPES)
	(void) time((long *)&realtime_data.restoretime);
#else
	(void) time(&realtime_data.restoretime);
#endif

#endif /* RECORD_REALTIME || REALTIME_ON_BOTL */

	/* Success! */
	welcome(TRUE);
	return;
}

STATIC_OVL int
find_preset_inherited(name)
	char * name;
{
	int i;
	char * aname;

	if(!strncmpi(name, "the ", 4)) name += 4;
	for (i = 1; i<=NROFARTIFACTS; i++)
	{
		if(artilist[i].gflags&ARTG_INHER
		&& !Role_if(artilist[i].role) && !Pantheon_if(artilist[i].role)
		&& !(urole.questarti == i)
		){
			aname = (char *)artilist[i].name;
			if(!strncmpi(aname, "the ", 4)) aname += 4;
			if(!strcmpi(name, aname)) return i;
		}
	}
	return 0;
}


STATIC_OVL int
do_inheritance_menu()
{
	winid tmpwin;
	int n, how, i;
	char buf[BUFSZ];
	char incntlet = 'a';
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */

	for (i = 1; i<=NROFARTIFACTS; i++)
	{
		// if ((artilist[i].spfx2) && artilist[i].spfx && artilist[i].spfx)
		if(artilist[i].gflags&ARTG_INHER
		&& !Role_if(artilist[i].role) && !Pantheon_if(artilist[i].role)
		&& !(urole.questarti == i)
		){
			Sprintf(buf, "%s", artilist[i].name);
			any.a_int = i;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				incntlet, 0, ATR_NONE, buf,
				MENU_UNSELECTED);
			incntlet = (incntlet == 'z') ? 'A' : (incntlet == 'Z') ? 'a' : (incntlet + 1);
		}
	}

	end_menu(tmpwin, "Which artifact did you inherit?");

	how = PICK_ONE;
	n = select_menu(tmpwin, how, &selected);
	destroy_nhwindow(tmpwin);
	if(n > 0){
		int inherited = selected[0].item.a_int;
		free(selected);
		return inherited;
	}
	return 0;
}

/* show "welcome [back] to nethack" message at program startup */
void
welcome(new_game)
boolean new_game;	/* false => restoring an old game */
{
    char buf[BUFSZ];
    boolean currentgend = Upolyd ? u.mfemale : flags.female;

    /*
     * The "welcome back" message always describes your innate form
     * even when polymorphed or wearing a helm of opposite alignment.
     * Alignment is shown unconditionally for new games; for restores
     * it's only shown if it has changed from its original value.
     * Sex is shown for new games except when it is redundant; for
     * restores it's only shown if different from its original value.
     */
    *buf = '\0';
    if (new_game || galign(u.ugodbase[UGOD_ORIGINAL]) != galign(u.ugodbase[UGOD_CURRENT]))
	Sprintf(eos(buf), " %s", align_str(galign(u.ugodbase[UGOD_CURRENT])));
    if (!urole.name.f &&
	    (new_game ? (urole.allow & ROLE_GENDMASK) == (ROLE_MALE|ROLE_FEMALE) :
	     currentgend != flags.initgend))
	Sprintf(eos(buf), " %s", genders[currentgend].adj);

    pline(new_game ? "%s %s, welcome to dNetHack!  You are a%s %s%s %s."
		   : "%s %s, the%s %s%s %s, welcome back to dNetHack!",
	  Hello((struct monst *) 0), plname, buf, urace.adj, (flags.descendant) ? " descendant" : "",
	  (currentgend && urole.name.f) ? urole.name.f : urole.name.m);
	if(iflags.dnethack_start_text){
	pline("Press Ctrl^W or type #ward to engrave a warding sign.");
	if(Role_if(PM_PIRATE)) You("can swim! Type #swim while swimming on the surface to dive down to the bottom.");
	else if(Role_if(PM_EXILE)){
		pline("Press Ctrl^E or type #seal to engrave a seal of binding.");
		pline("#chat to a fresh seal to contact the spirit beyond.");
		pline("Press Ctrl^F or type #power to fire active spirit powers!");
	}
	else if(Role_if(PM_MADMAN)){
		You("have psychic powers. Type #ability or press Shift-B to access your powers!");
	}
	if(Race_if(PM_DROW)){
		if(!(Role_if(PM_HEALER) || Role_if(PM_EXILE)))
			pline("Beware, droven armor evaporates in light!");
		pline("Use #monster to create a patch of darkness.");
	}
	if(Race_if(PM_ANDROID)){
		pline("Androids do not need to eat, but *do* need to sleep.");
		pline("Use #monster to access your innate abilities, including sleep.");
		pline("Use '.' to recover HP using magic energy.");
		pline("Sleep to recover magic energy.");
	}
	if(Race_if(PM_CLOCKWORK_AUTOMATON)){
		pline("Use #monster to adjust your clockspeed.");
		You("do not heal naturally. Use '.' to attempt repairs.");
	}
	if(Race_if(PM_INCANTIFIER)){
		pline("Incantifiers eat magic, not food, and do not heal naturally.");
	}
	}
}

#ifdef POSITIONBAR
STATIC_DCL void
do_positionbar()
{
	static char pbar[COLNO];
	char *p;

#define cmap_at_stair(stair) glyph_to_cmap(level.locations[stair.sx][stair.sy].glyph) 
	p = pbar;
	/* up stairway */
	if (upstair.sx && (cmap_at_stair(upstair) == S_upstair || cmap_at_stair(upstair) == S_upladder)) {
		*p++ = '<';
		*p++ = upstair.sx;
	}
	if (sstairs.sx && (cmap_at_stair(sstairs) == S_upstair || cmap_at_stair(sstairs) == S_upladder) 
		|| cmap_at_stair(sstairs) == S_brupstair)) {
		*p++ = '<';
		*p++ = sstairs.sx;
	}

	/* down stairway */
	if (dnstair.sx && (cmap_at_stair(dnstair) == S_dnstair || cmap_at_stair(dnstair) == S_dnladder)) {
		*p++ = '>';
		*p++ = dnstair.sx;
	}
	if (sstairs.sx && (cmap_at_stair(sstairs) == S_dnstair || cmap_at_stair(sstairs) == S_dnladder) 
		|| cmap_at_stair(sstairs) == S_brdnstair)) {
		*p++ = '>';
		*p++ = sstairs.sx;
	}

	/* hero location */
	if (u.ux) {
		*p++ = '@';
		*p++ = u.ux;
	}
	/* fence post */
	*p = 0;

	update_positionbar(pbar);
}
#endif

#if defined(REALTIME_ON_BOTL) || defined (RECORD_REALTIME)
time_t
get_realtime(void)
{
    time_t curtime;

    /* Get current time */
#if defined(BSD) && !defined(POSIX_TYPES)
    (void) time((long *)&curtime);
#else
    (void) time(&curtime);
#endif

    /* Since the timer isn't set until the game starts, this prevents us
     * from displaying nonsense on the bottom line before it does. */
    if(realtime_data.restoretime == 0) {
        curtime = realtime_data.realtime;
    } else {
        curtime -= realtime_data.restoretime;
        curtime += realtime_data.realtime;
    }
 
    return curtime;
}
#endif /* REALTIME_ON_BOTL || RECORD_REALTIME */

STATIC_DCL
void
printBodies(){
	FILE *rfile;
	register int i, j;
	char pbuf[BUFSZ];
	struct permonst *ptr;
	rfile = fopen_datafile("MonBodies.tab", "w", SCOREPREFIX);
	if (rfile) {
		Sprintf(pbuf,"Number\tName\tclass\thumanoid\tanimaloid\tserpentine\tcentauroid\tnaganoid\tleggedserpent\tNAoid\thumanoid torso\thumanoid upperbody\thead\thands\tgloves\tfeet\tboots\teyes\toldpolywep\thands but no polywep\n");
		fprintf(rfile, "%s", pbuf);
		fflush(rfile);
		for(j=0;j<NUMMONS;j++){
			ptr = &mons[j];
			pbuf[0] = 0;// n	nm	let	hm	anm	srp	cen	ng	lgs	hd	hn	ft	bt  ey
			Sprintf(pbuf,"%d	%s	%d	%d	%d	%d	%d	%d	%d	%d	%d	%d	%d	%d	%d	%d	%d	%d	%d	%d\n", 
						   j,	mons[j].mname, 
									mons[j].mlet,
										humanoid(ptr),
											animaloid(ptr),
												serpentine(ptr),
													centauroid(ptr),
														snakemanoid(ptr),
															leggedserpent(ptr),
																naoid(ptr),
																	humanoid_torso(ptr),
																		humanoid_upperbody(ptr),
																			has_head(ptr),
																				!nohands(ptr),
																					!nogloves(ptr),
																						!noboots(ptr),
																							can_wear_boots(ptr),
																								haseyes(ptr),
																									!nohands(ptr) && ((ptr->mattk[0].aatyp == AT_CLAW || (ptr->mattk[0].aatyp == AT_TUCH && ptr->mlet == S_LICH))
																									|| (ptr->mattk[1].aatyp == AT_CLAW && (ptr->mtyp == PM_INCUBUS || ptr->mtyp == PM_SUCCUBUS))),
																										!nohands(ptr) && you_cantwield(ptr)
																										);
			fprintf(rfile, "%s", pbuf);
			fflush(rfile);
		}
	}
	fclose(rfile);
}

STATIC_DCL
void
printSanAndInsight(){
	FILE *rfile;
	register int i, j;
	char pbuf[BUFSZ];
	struct permonst *ptr;
	rfile = fopen_datafile("MonSanAndInsight.tab", "w", SCOREPREFIX);
	if (rfile) {
		Sprintf(pbuf,"Number\tName\tclass\tslain insight\tseen insight\tsanity\tlocation\tto-see\n");
		fprintf(rfile, "%s", pbuf);
		fflush(rfile);
		for(j=0;j<NUMMONS;j++){
			ptr = &mons[j];
			pbuf[0] = 0;
			if(!taxes_sanity(&mons[j]) && !yields_insight(&mons[j]))
				continue;
			Sprintf(pbuf,"%d	%s	%d	%d	%d	%d	%s	%d\n", 
					j, mons[j].mname, mons[j].mlet,
						yields_insight(&mons[j]) ? (monstr[j]/10 > 1 ? monstr[j]/10-1 : max(1, monstr[j]/10)) : 0, 
						(yields_insight(&mons[j]) && monstr[j]/10 > 1) ? 1 : 0, 
						taxes_sanity(&mons[j]) ? monstr[j] : 0,
						mons[j].geno&G_UNIQ ? "Unique" :
						mons[j].geno&G_HELL ? "Hell" :
						mons[j].geno&G_PLANES ? "Planes" :
						mons[j].geno&G_DEPTHS ? "Depths" :
						!(mons[j].geno&G_NOGEN) ? "Main" :
						"",
						(int)G_C_INST(mons[j].geno));
			fprintf(rfile, "%s", pbuf);
			fflush(rfile);
		}
	}
	fclose(rfile);
}

STATIC_DCL
void
printDPR(){
	FILE *rfile;
	register int i, j, avdm, mdm, avgperhit, maxperhit;
	char pbuf[BUFSZ];
	struct permonst *ptr;
	struct attack *attk;
	rfile = fopen_datafile("MonDPR.tab", "w", SCOREPREFIX);
	if (rfile) {
		Sprintf(pbuf,"Number\tName\tclass\tdifficulty\taverage\tmax\tper-hit avg\tper-hit max\tspeed\talignment\tunique?\n");
		fprintf(rfile, "%s", pbuf);
		fflush(rfile);
		for(j=0;j<NUMMONS;j++){
			ptr = &mons[j];
			pbuf[0] = 0;
			avdm = 0;
			mdm = 0;
			avgperhit = 0;
			maxperhit = 0;
			for(i = 0; i<6; i++){
				attk = &ptr->mattk[i];
				if(attk->aatyp == 0 &&
					attk->adtyp == 0 &&
					attk->damn == 0 &&
					attk->damd == 0
				) break;
				else {
					avdm += attk->damn * (attk->damd + 1)/2;
					if(avgperhit < attk->damn * (attk->damd + 1)/2)
						avgperhit = attk->damn * (attk->damd + 1)/2;
					mdm += attk->damn * attk->damd;
					if(maxperhit < attk->damn * attk->damd)
						maxperhit = attk->damn * attk->damd;
				}
			}
			Sprintf(pbuf,"%d\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\n", j, mons[j].mname, mons[j].mlet,monstr[j],avdm, mdm, avgperhit, maxperhit, mons[j].mmove,ptr->maligntyp,(mons[j].geno&G_UNIQ) ? "unique":"");
			fprintf(rfile, "%s", pbuf);
			fflush(rfile);
		}
	}
	fclose(rfile);
}

STATIC_DCL
void
printMons(){
	FILE *rfile;
	register int i;
	char pbuf[BUFSZ*100];
	char attkbuf[BUFSZ];
	char sizebuf[BUFSZ];
	char resbuf[BUFSZ];
	char convbuf[BUFSZ];
	struct permonst *ptr;
	attkbuf[0]=0;
	sizebuf[0]=0;
	resbuf[0]=0;
	convbuf[0]=0;
	rfile = fopen_datafile("wikiMonsters.txt", "w", SCOREPREFIX);
	if (rfile) {
		char *colorstr;
		for(i=0;i<NUMMONS;i++){
			ptr = &mons[i];
			switch(mons[i].mcolor){
				case CLR_BLACK:
					colorstr = "black";
				break;
				case CLR_RED:
					colorstr = "red";
				break;
				case CLR_GREEN:
					colorstr = "green";
				break;
				case CLR_BROWN:
					colorstr = "brown";
				break;
				case CLR_BLUE:
					colorstr = "blue";
				break;
				case CLR_MAGENTA:
					colorstr = "magenta";
				break;
				case CLR_CYAN:
					colorstr = "cyan";
				break;
				case CLR_GRAY:
					colorstr = "gray";
				break;
				case CLR_ORANGE:
					colorstr = "orange";
				break;
				case CLR_BRIGHT_MAGENTA:
					colorstr = "brightmagenta";
				break;
				case CLR_BRIGHT_GREEN:
					colorstr = "brightgreen";
				break;
				case CLR_YELLOW:
					colorstr = "yellow";
				break;
				case CLR_BRIGHT_BLUE:
					colorstr = "brightblue";
				break;
				case CLR_BRIGHT_CYAN:
					colorstr = "brightcyan";
				break;
				case CLR_WHITE:
					colorstr = "white";
				break;
			}
			printAttacks(attkbuf,&mons[i]);
			switch(mons[i].msize){
				case MZ_TINY:
					Sprintf(sizebuf,"tiny");
				break;
				case MZ_SMALL:
					Sprintf(sizebuf,"small");
				break;
				case MZ_MEDIUM:
					Sprintf(sizebuf,"medium");
				break;
				case MZ_LARGE:
					Sprintf(sizebuf,"large");
				break;
				case MZ_HUGE:
					Sprintf(sizebuf,"huge");
				break;
				default:
					Sprintf(sizebuf,"gigantic");
				break;
			}
			resFlags(resbuf, mons[i].mresists);
			resFlags(convbuf, mons[i].mconveys);
			Sprintf(pbuf,"{{monster\n");
			Sprintf(eos(pbuf)," |name=%s\n", mons[i].mname);
			Sprintf(eos(pbuf)," |symbol={{%s|%c}}\n", colorstr, def_monsyms[(int)mons[i].mlet]);
			Sprintf(eos(pbuf)," |tile=\n");
			Sprintf(eos(pbuf)," |difficulty=%d\n", monstr[i]);
			Sprintf(eos(pbuf)," |level=%d\n", mons[i].mlevel);
			Sprintf(eos(pbuf)," |experience=%d\n", ptrexperience(ptr));
			Sprintf(eos(pbuf)," |speed=%d\n", mons[i].mmove);
			Sprintf(eos(pbuf)," |AC=%d\n", 10-(mons[i].nac+mons[i].dac+mons[i].pac));
			Sprintf(eos(pbuf)," |MR=%d\n", mons[i].mr);
			Sprintf(eos(pbuf)," |align=%d\n", mons[i].maligntyp);
			Sprintf(eos(pbuf)," |frequency=%d\n", (int)(mons[i].geno & G_FREQ));
			Sprintf(eos(pbuf)," |genocidable=%s\n", ((mons[i].geno & G_GENO) != 0) ? "Yes":"No");
			Sprintf(eos(pbuf)," |attacks=%s\n", attkbuf);
			Sprintf(eos(pbuf)," |weight=%d\n", (int) mons[i].cwt);
			Sprintf(eos(pbuf)," |nutr=%d\n", (int) mons[i].cnutrit);
			Sprintf(eos(pbuf)," |size=%s\n", sizebuf);
			Sprintf(eos(pbuf)," |resistances=%s\n", resbuf);
			Sprintf(eos(pbuf)," |resistances conveyed=%s\n", convbuf);
			Sprintf(eos(pbuf)," |attributes={{attributes\n");
				Sprintf(eos(pbuf)," |%s|=\n", Aptrnam(ptr));
			if(species_flies(&mons[i]))	Sprintf(eos(pbuf)," |fly=%s\n", species_flies(&mons[i]) ? "1":"");
			if(species_swims(&mons[i]))	Sprintf(eos(pbuf)," |swim=%s\n", species_swims(&mons[i]) ? "1":"");
			if(amorphous(ptr))	Sprintf(eos(pbuf)," |amorphous=%s\n", amorphous(ptr) ? "1":"");
			if(species_passes_walls(ptr))	Sprintf(eos(pbuf)," |wallwalk=%s\n", species_passes_walls(ptr) ? "1":"");
			if(is_clinger(ptr))	Sprintf(eos(pbuf)," |cling=%s\n", is_clinger(ptr) ? "1":"");
			if(tunnels(ptr))	Sprintf(eos(pbuf)," |tunnel=%s\n", tunnels(ptr) ? "1":"");
			if(needspick(ptr))	Sprintf(eos(pbuf)," |needpick=%s\n", needspick(ptr) ? "1":"");
			if(hides_under(ptr))	Sprintf(eos(pbuf)," |conceal=%s\n", hides_under(ptr) ? "1":"");
			if(is_hider(ptr))	Sprintf(eos(pbuf)," |hide=%s\n",  is_hider(ptr) ? "1":"");
			if( amphibious(ptr) )	Sprintf(eos(pbuf)," |amphibious=%s\n",  amphibious(ptr) ? "1":"");
			if(breathless(ptr))	Sprintf(eos(pbuf)," |breathless=%s\n",  breathless(ptr) ? "1":"");
			if(notake(ptr) )	Sprintf(eos(pbuf)," |notake=%s\n", notake(ptr) ? "1":"");
			if(!haseyes(ptr))	Sprintf(eos(pbuf)," |noeyes=%s\n", !haseyes(ptr) ? "1":"");
			if(nohands(ptr))	Sprintf(eos(pbuf)," |nohands=%s\n", nohands(ptr) ? "1":"");
			if(nolimbs(ptr))	Sprintf(eos(pbuf)," |nolimbs=%s\n", nolimbs(ptr) ? "1":"");
			if(!has_head(ptr))	Sprintf(eos(pbuf)," |nohead=%s\n", !has_head(ptr) ? "1":"");
			if(mindless(ptr))	Sprintf(eos(pbuf)," |mindless=%s\n", mindless(ptr) ? "1":"");
			if( humanoid(ptr))	Sprintf(eos(pbuf)," |humanoid=%s\n", humanoid(ptr) ? "1":"");
			if(is_animal(ptr))	Sprintf(eos(pbuf)," |animal=%s\n", is_animal(ptr) ? "1":"");
			if(slithy(ptr))	Sprintf(eos(pbuf)," |slithy=%s\n", slithy(ptr) ? "1":"");
			if(unsolid(ptr))	Sprintf(eos(pbuf)," |unsolid=%s\n", unsolid(ptr) ? "1":"");
			if(thick_skinned(ptr))	Sprintf(eos(pbuf)," |thick=%s\n", thick_skinned(ptr) ? "1":"");
			if(lays_eggs(ptr))	Sprintf(eos(pbuf)," |oviparous=%s\n", lays_eggs(ptr) ? "1":"");
			if(species_regenerates(ptr))	Sprintf(eos(pbuf)," |regen=%s\n", species_regenerates(ptr) ? "1":"");
			if(species_perceives(ptr))	Sprintf(eos(pbuf)," |seeinvis=%s\n", species_perceives(ptr) ? "1":"");
			if(species_teleports(ptr))	Sprintf(eos(pbuf)," |tport=%s\n", species_teleports(ptr) ? "1":"");
			if(species_controls_teleports(ptr))	Sprintf(eos(pbuf)," |tportcntrl=%s\n", species_controls_teleports(ptr) ? "1":"");
			if(acidic(ptr))	Sprintf(eos(pbuf)," |acid=%s\n", acidic(ptr) ? "1":"");
			if(poisonous(ptr) )	Sprintf(eos(pbuf)," |pois=%s\n", poisonous(ptr) ? "1":"");
			if(carnivorous(ptr)&&!herbivorous(ptr))	Sprintf(eos(pbuf)," |carnivore=%s\n", carnivorous(ptr)&&!herbivorous(ptr) ? "1":"");
			if(herbivorous(ptr)&&!carnivorous(ptr))	Sprintf(eos(pbuf)," |herbivore=%s\n", herbivorous(ptr)&&!carnivorous(ptr) ? "1":"");
			if(carnivorous(ptr)&&herbivorous(ptr))	Sprintf(eos(pbuf)," |omnivore=%s\n", carnivorous(ptr)&&herbivorous(ptr) ? "1":"");
			if(metallivorous(ptr))	Sprintf(eos(pbuf)," |metallivore=%s\n", metallivorous(ptr) ? "1":"");
			if(!polyok(ptr))	Sprintf(eos(pbuf)," |nopoly=%s\n", !polyok(ptr) ? "1":"");
			if(is_undead(ptr))	Sprintf(eos(pbuf)," |undead=%s\n", is_undead(ptr) ? "1":"");
			if(is_were(ptr))	Sprintf(eos(pbuf)," |were=%s\n", is_were(ptr) ? "1":"");
			if(is_human(ptr))	Sprintf(eos(pbuf)," |human=%s\n", is_human(ptr) ? "1":"");
			if(is_elf(ptr))	Sprintf(eos(pbuf)," |elf=%s\n", is_elf(ptr) ? "1":"");
			if(is_dwarf(ptr))	Sprintf(eos(pbuf)," |dwarf=%s\n", is_dwarf(ptr) ? "1":"");
			if(is_gnome(ptr))	Sprintf(eos(pbuf)," |gnome=%s\n", is_gnome(ptr) ? "1":"");
			if(is_orc(ptr))	Sprintf(eos(pbuf)," |orc=%s\n", is_orc(ptr) ? "1":"");
			if(is_demon(ptr))	Sprintf(eos(pbuf)," |demon=%s\n", is_demon(ptr) ? "1":"");
			if(is_mercenary(ptr))	Sprintf(eos(pbuf)," |merc=%s\n", is_mercenary(ptr) ? "1":"");
			if(is_lord(ptr))	Sprintf(eos(pbuf)," |lord=%s\n", is_lord(ptr) ? "1":"");
			if(is_prince(ptr))	Sprintf(eos(pbuf)," |prince=%s\n", is_prince(ptr) ? "1":"");
			if(is_giant(ptr) )	Sprintf(eos(pbuf)," |giant=%s\n", is_giant(ptr) ? "1":"");
			if(is_male(ptr))	Sprintf(eos(pbuf)," |male=%s\n", is_male(ptr) ? "1":"");
			if(is_female(ptr))	Sprintf(eos(pbuf)," |female=%s\n", is_female(ptr) ? "1":"");
			if(is_neuter(ptr))	Sprintf(eos(pbuf)," |neuter=%s\n", is_neuter(ptr) ? "1":"");
			if(always_hostile(ptr))	Sprintf(eos(pbuf)," |hostile=%s\n", always_hostile(ptr) ? "1":"");
			if(always_peaceful(ptr))	Sprintf(eos(pbuf)," |peaceful=%s\n", always_peaceful(ptr) ? "1":"");
			if(is_domestic(ptr))	Sprintf(eos(pbuf)," |domestic=%s\n", is_domestic(ptr) ? "1":"");
			if(is_wanderer(ptr))	Sprintf(eos(pbuf)," |wander=%s\n", is_wanderer(ptr) ? "1":"");
			if((((ptr)->mflagst & MT_STALK) != 0L))	Sprintf(eos(pbuf)," |stalk=%s\n", (((ptr)->mflagst & MT_STALK) != 0L) ? "1":"");
			if(extra_nasty(ptr))	Sprintf(eos(pbuf)," |nasty=%s\n", extra_nasty(ptr) ? "1":"");
			if(strongmonst(ptr) )	Sprintf(eos(pbuf)," |strong=%s\n", strongmonst(ptr) ? "1":"");
			if(throws_rocks(ptr))	Sprintf(eos(pbuf)," |rockthrow=%s\n", throws_rocks(ptr) ? "1":"");
			if(likes_gold(ptr))	Sprintf(eos(pbuf)," |greedy=%s\n", likes_gold(ptr) ? "1":"");
			if(likes_gems(ptr))	Sprintf(eos(pbuf)," |jewels=%s\n", likes_gems(ptr) ? "1":"");
			if(likes_objs(ptr))	Sprintf(eos(pbuf)," |collect=%s\n", likes_objs(ptr) ? "1":"");
			if(likes_magic(ptr))	Sprintf(eos(pbuf)," |magic=%s\n", likes_magic(ptr) ? "1":"");
			if(wants_amul(ptr))	Sprintf(eos(pbuf)," |wantsamul=%s\n", wants_amul(ptr) ? "1":"");
			if(wants_bell(ptr))	Sprintf(eos(pbuf)," |wantsbell=%s\n", wants_bell(ptr) ? "1":"");
			if(wants_book(ptr))	Sprintf(eos(pbuf)," |wantsbook=%s\n", wants_book(ptr) ? "1":"");
			if(wants_cand(ptr))	Sprintf(eos(pbuf)," |wantscand=%s\n", wants_cand(ptr) ? "1":"");
			if(wants_qart(ptr))	Sprintf(eos(pbuf)," |wantsarti=%s\n", wants_qart(ptr) ? "1":"");
			if(is_covetous(ptr))	Sprintf(eos(pbuf)," |wantsall=%s\n", is_covetous(ptr) ? "1":"");
			if((((ptr)->mflagst & MT_WAITFORU) != 0L))	Sprintf(eos(pbuf)," |waitsforu=%s\n", (((ptr)->mflagst & MT_WAITFORU) != 0L) ? "1":"");
			if((((ptr)->mflagst & MT_CLOSE) != 0L))	Sprintf(eos(pbuf)," |close=%s\n", (((ptr)->mflagst & MT_CLOSE) != 0L) ? "1":"");
			if(is_covetous(ptr))	Sprintf(eos(pbuf)," |covetous=%s\n", is_covetous(ptr) ? "1":"");
			if(infravision(ptr))	Sprintf(eos(pbuf)," |infravision=%s\n", infravision(ptr) ? "1":"");
			if(infravisible(ptr))	Sprintf(eos(pbuf)," |infravisible=%s\n", infravisible(ptr) ? "1":"");
			if((mons[i].geno & G_NOHELL))	Sprintf(eos(pbuf)," |nohell=%s\n", (mons[i].geno & G_NOHELL) ? "1":"");
			if(((mons[i].geno & G_HELL) != 0))	Sprintf(eos(pbuf)," |hell=%s\n", ((mons[i].geno & G_HELL) != 0) ? "1":"");
			// if(((mons[i].geno & G_PLANES) != 0))	Sprintf(eos(pbuf)," |hell=%s\n", ((mons[i].geno & G_PLANES) != 0) ? "1":"");
			// if()	Sprintf(eos(pbuf)," |moria=\n");
			if((mons[i].geno & G_SGROUP))	Sprintf(eos(pbuf)," |sgroup=%s\n", (mons[i].geno & G_SGROUP) ? "1":"");
			if((mons[i].geno & G_LGROUP))	Sprintf(eos(pbuf)," |lgroup=%s\n", (mons[i].geno & G_LGROUP) ? "1":"");
			// if()	Sprintf(eos(pbuf)," |vlgroup=\n");
			if((mons[i].geno & G_NOCORPSE))	Sprintf(eos(pbuf)," |nocorpse=%s\n", (mons[i].geno & G_NOCORPSE) ? "1":"");
			if((is_undead(ptr) && !(mons[i].geno & G_NOCORPSE)))	Sprintf(eos(pbuf)," |oldcorpse=%s\n", (is_undead(ptr) && !(mons[i].geno & G_NOCORPSE)) ? "1":"");
			if(emits_light(ptr))	Sprintf(eos(pbuf)," |light=%s\n", emits_light(ptr) ? "1":"");
			if((nonliving(ptr) || is_demon(ptr)))	Sprintf(eos(pbuf)," |death=%s\n", (nonliving(ptr) || is_demon(ptr)) ? "1":"");
			if((mons[i].mresists & MR_DRAIN))	Sprintf(eos(pbuf)," |drain=%s\n", (mons[i].mresists & MR_DRAIN) ? "1":"");
			// if()	Sprintf(eos(pbuf)," |plusone=\n");
			// if()	Sprintf(eos(pbuf)," |plustwo=\n");
			// if()	Sprintf(eos(pbuf)," |plusthree=\n");
			// if()	Sprintf(eos(pbuf)," |plusfour=\n");
			// if()	Sprintf(eos(pbuf)," |hitasone=\n");
			// if()	Sprintf(eos(pbuf)," |hitastwo=\n");
			// if()	Sprintf(eos(pbuf)," |hitasthree=\n");
			// if()	Sprintf(eos(pbuf)," |hitasfour=\n");
			if(is_vampire(ptr))	Sprintf(eos(pbuf)," |vampire=%s\n", is_vampire(ptr) ? "1":"");
			if(can_betray(ptr))	Sprintf(eos(pbuf)," |traitor=%s\n", can_betray(ptr) ? "1":"");
			if(is_untamable(ptr))	Sprintf(eos(pbuf)," |notame=%s\n", is_untamable(ptr) ? "1":"");
			if((emits_light(ptr) == 1))	Sprintf(eos(pbuf)," |light1=%s\n", (emits_light(ptr) == 1) ? "1":"");
			if((emits_light(ptr) == 2))	Sprintf(eos(pbuf)," |light2=%s\n", (emits_light(ptr) == 2) ? "1":"");
			if((emits_light(ptr) == 3))	Sprintf(eos(pbuf)," |light3=%s\n", (emits_light(ptr) == 3) ? "1":"");
			// if()	Sprintf(eos(pbuf)," |groupattack=\n");
			// if()	Sprintf(eos(pbuf)," |blinker=\n");
			if((!species_regenerates(ptr) && nonliving(ptr)))	Sprintf(eos(pbuf)," |noregen=%s\n", (!species_regenerates(ptr) && nonliving(ptr)) ? "1":"");
			if(stationary(ptr))	Sprintf(eos(pbuf)," |stationary=%s\n", stationary(ptr) ? "1":"");
			Sprintf(eos(pbuf)," }}\n");
			Sprintf(eos(pbuf)," |reference=\n");
			Sprintf(eos(pbuf),"}}\n\n");
			fprintf(rfile, "%s", pbuf);
			fflush(rfile);
		}
		for(i=0;i<NUMMONS;i++){
			switch(mons[i].mcolor){
				case CLR_BLACK:
					colorstr = "black";
				break;
				case CLR_RED:
					colorstr = "red";
				break;
				case CLR_GREEN:
					colorstr = "green";
				break;
				case CLR_BROWN:
					colorstr = "brown";
				break;
				case CLR_BLUE:
					colorstr = "blue";
				break;
				case CLR_MAGENTA:
					colorstr = "magenta";
				break;
				case CLR_CYAN:
					colorstr = "cyan";
				break;
				case CLR_GRAY:
					colorstr = "gray";
				break;
				case CLR_ORANGE:
					colorstr = "orange";
				break;
				case CLR_BRIGHT_MAGENTA:
					colorstr = "brightmagenta";
				break;
				case CLR_BRIGHT_GREEN:
					colorstr = "brightgreen";
				break;
				case CLR_YELLOW:
					colorstr = "yellow";
				break;
				case CLR_BRIGHT_BLUE:
					colorstr = "brightblue";
				break;
				case CLR_BRIGHT_CYAN:
					colorstr = "brightcyan";
				break;
				case CLR_WHITE:
					colorstr = "white";
				break;
			}
			if((int)mons[i].mlet == S_SHADE) Sprintf(pbuf,"[[%s|{{%s|&nbsp;}}]]", mons[i].mname, colorstr);
			else Sprintf(pbuf,"[[%s|{{%s|%c}}]]", mons[i].mname, colorstr, def_monsyms[(int)mons[i].mlet]);
			fprintf(rfile, "%s", pbuf);
			fflush(rfile);
			if(((i+1)%40) == 0){
				Sprintf(pbuf,"<br/>\n");
				fprintf(rfile, "%s", pbuf);
				fflush(rfile);
			}
		}
		while(((i+1)%40) != 0){
			Sprintf(pbuf,"{{black|&nbsp;}}");
			fprintf(rfile, "%s", pbuf);
			fflush(rfile);
			i++;
		}
	}
	fclose(rfile);
}

STATIC_DCL
void
printMonNames(){
	FILE *rfile;
	register int i;
	rfile = fopen_datafile("wikiNames.txt", "w", SCOREPREFIX);
	if (rfile) {
		for(i=0;i<NUMMONS;i++){
			fprintf(rfile, "==%s==\n\n\n", mons[i].mname);
			fflush(rfile);
		}
	}
	fclose(rfile);
}

STATIC_DCL
void
resFlags(buf, rflags)
	char *buf;
	unsigned int rflags;
{
	int i;
	boolean b = FALSE;
	static char *mrKey[] = {
		"[[fire]]",
		"[[cold]]",
		"[[sleep]]",
		"[[disintegration]]",
		"[[shock]]",
		"[[poison]]",
		"[[acid]]",
		"[[petrification]]",
		"[[level drain]]",
		"[[disease]]"
	};
	buf[0] = '\0'; //Empty buffer (contains previous monster's resistances)
	for(i = 0; i<10; i++){
		if(rflags & (1 << i)){
			if(!b){
				b = TRUE;
				Sprintf(buf, "%s", mrKey[i]);
			} else {
				Sprintf(eos(buf), ", %s", mrKey[i]);
			}
		}
	}
}

STATIC_DCL
void
printAttacks(buf, ptr)
	char *buf;
	struct permonst *ptr;
{
	int i;
	struct attack *attk;
	static char *attackKey[] = {
		"Passive",	/*0*/
		"Claw",		/*1*/
		"Bite",		/*2*/
		"Kick",		/*3*/
		"Butt",		/*4*/
		"Touch",	/*5*/
		"Sting",	/*6*/
		"Bearhug",	/*7*/
		"NA",		/*8*/
		"NA",		/*9*/
		"Spit",		/*10*/
		"Engulf",	/*11*/
		"Breath weapon",	/*12*/
		"Splashing Breath",	/*13*/
		"Suicidal explosion",	/*134*/
		"Explode on death",	/*15*/
		"Single-target (active) gaze attack",	/*16*/
		"Tentacle",	/*17*/
		"Arrow",	/*18*/
		"Whip",	/*19*/
		"Reach",	/*20*/
		"Your weapon",	/*21*/
		"Long-necked bite",	/*22*/
		"Magic",	/*23*/
		"Droplet storm",	/*24*/
		"Automatic hit",	/*25*/
		"Mist tendrils",	/*26*/
		"Tinker",	/*27*/
		"Magic blades",	/*28*/
		"Beam",	/*29*/
		"Deva Arms",	/*30*/
		"Five-square-reach touch",	/*31*/
		"Five-square-reach bite",	/*32*/
		"Wide-angle (passive) gaze",/*33*/
		"Rend",	/*34*/
		"Lashing vines",	/*35*/
		"Black goat (mist)",	/*36*/
		"Black goat (blessed)",	/*37*/
		"Magic blades (offhand)",	/*38*/
		"Magic blades (extra hand)",	/*39*/
		"Magic blades (deva arms)",	/*40*/
		"Magic blades (floating)",	/*41*/
		"Secondary bite",	/*42*/
		"Waist-wolf bite",	/*43*/
		"Tail slap",	/*44*/
		"Tongue",	/*45*/
		""
	};
	static char *damageKey[] = {
		"physical",				/*0*/
		"[[magic missile]]s",	/*1*/
		"[[fire]]",				/*2*/
		"[[cold]]",				/*3*/
		"[[sleep]]",			/*4*/
		"[[disintegration]]",	/*5*/
		"[[shock]]",			/*6*/
		"[[poison]] (strength)",/*7*/
		"[[acid]]",				/*8*/
		"Unused 1",				/*9*/
		"Unused 2",				/*10*/
		"[[blind]]ing",			/*11*/
		"[[stun]]ning",			/*12*/
		"[[slow]]ing",			/*13*/
		"[[paralysis]]",		/*14*/
		"[[life drain]]",		/*15*/
		"[[energy drain]]",		/*16*/
		"[[leg wounding]]",		/*17*/
		"[[petrifcation]]",		/*18*/
		"[[sticky]]",			/*19*/
		"[[steal gold]]",		/*20*/
		"[[steal item]]",		/*21*/
		"[[steal item|seduction]]",/*22*/
		"[[teleportation]]",	/*23*/
		"[[rust]]",				/*24*/
		"[[confusion]]",		/*25*/
		"[[digestion]]",		/*26*/
		"[[healing]]",			/*27*/
		"[[drowning]]",			/*28*/
		"[[lycanthropy]]",		/*29*/
		"[[poison]] (dexterity)",/*30*/
		"[[poison]] (constitution)",/*31*/
		"[[int drain]]",		/*32*/
		"[[disease]]",			/*33*/
		"[[rotting]]",			/*34*/
		"[[seduction]]",		/*35*/
		"[[hallucination]]",	/*36*/
		"[[Death's touch]]",	/*37*/
		"[[Pestilence]]",		/*38*/
		"[[Famine]]",			/*39*/
		"[[sliming]]",			/*40*/
		"[[disenchant]]",		/*41*/
		"[[corrosion]]",		/*42*/
		"[[poison]] (HP damage)",/*43*/
		"[[wis drain]]",		/*44*/
		"[[vorpal]]",			/*45*/
		"[[armor shredding]]",	/*46*/
		"[[silver]]",			/*47*/
		"[[cannon ball]]",		/*48*/
		"[[boulder]]",			/*49*/
		"[[random boulder]]",	/*50*/
		"[[tickling]]",			/*51*/
		"[[soaking]]",			/*52*/
		"[[lethe]]",			/*53*/
		"[[bisection]]",		/*54*/
		"NA",					/*55*/
		"NA",					/*56*/
		"NA",					/*57*/
		"NA",					/*58*/
		"NA",					/*59*/
		"[[cancellation]]",		/*60*/
		"[[deadly]]",			/*61*/
		"[[suction]]",			/*62*/
		"[[malkuth]]",			/*63*/
		"[[uvuudaum brainspike]]",/*64*/
		"[[abduction]]",		/*65*/
		"[[spawn Chaos]]",		/*66*/
		"[[seduction]]",		/*67*/
		"[[hellblood]]",		/*68*/
		"[[spawn Leviathan]]",	/*69*/
		"[[mist projection]]",	/*70*/
		"[[teleport away]]",	/*71*/
		"[[baleful polymorph]]",/*72*/
		"[[psionic]]",			/*73*/
		"[[promotion]]",		/*74*/
		"[[shared soul]]",		/*75*/
		"[[intrusion]]",		/*76*/
		"[[jailer]]",			/*77*/
		"[[special]]",			/*78*/
		"[[take artifact]]",	/*79*/
		"[[silver]]",			/*80*/
		"[[special]]",			/*81*/
		"[[your weapon]]",		/*82*/
		"[[cursed unicorn horn]]",/*83*/
		"[[loadstone]]",		/*84*/
		"[[garo report]]",		/*85*/
		"[[garo report]]",		/*86*/
		"[[level teleport]]",	/*87*/
		"[[blink]]",			/*88*/
		"[[angel's touch]]",	/*89*/
		"[[spore]]",			/*90*/
		"[[explosive spore]]",	/*91*/
		"[[sunlight]]",			/*92*/
		"[[deadly shriek]]",	/*93*/
		"[[barbs]]",			/*94*/
		"[[luck drain]]",		/*95*/
		"[[vampiric]]",			/*96*/
		"[[webbing]]",			/*97*/
		"[[special]]",			/*98*/
		"[[spawn gizmos]]",		/*99*/
		"[[fireworks]]",		/*100*/
		"[[study]]",			/*101*/
		"[[fire]], [[cold]], or [[shock]]",/*102*/
		"[[netzach]]",			/*103*/
		"[[special]]",			/*104*/
		"[[shadow]]",			/*105*/
		"[[armor teleportation]]",/*106*/
		"[[half-dragon breath]]",/*107*/
		"[[silver rapier]]",	/*108*/
		"elemental [[shock]]",	/*109*/
		"elemental [[fire]]",	/*110*/
		"elemental [[poison]]",	/*111*/
		"elemental [[cold]]",	/*112*/
		"elemental [[acid]]",	/*113*/
		"conflict",				/*114*/
		"blood blade",			/*115*/
		"Surya Deva arrow",		/*116*/
		"[[constitution]] drain",/*117*/
		"Silver mirror shards", /*118*/
		"Mercury blade",		/*119*/
		"Gold transmutation", 	/*120*/
		"[[Holy fire]]", 		/*121*/
		"[[Dessication]]",		/*122*/
		"[[Anger god]]",		/*123*/
		"[[four seasons]]",		/*124*/
		"[[pollen]]",			/*125*/
		"[[Blood frenzy]]",		/*126*/
		"[[Create spheres]]",	/*127*/
		"[[Dark]]",				/*128*/
		"[[Implant egg]]",		/*129*/
		"[[Flesh hook]]",		/*130*/
		"[[Mindwipe]]",			/*131*/
		"Slow [[petrification]]",/*132*/
		"[[strength]] drain",	/*133*/
		"[[dexterity]] drain",	/*134*/
		"[[charisma]] drain",	/*135*/
		"all attribute drain",	/*136*/
		"inflict [[agnosis]]",	/*137*/
		"revelatory whispers",	/*138*/
		"pull closer",			/*139*/
		"crippling pain",		/*140*/
		"inflict curses",		/*141*/
		"crushing lava",		/*142*/
		"pyroclastic",			/*143*/
		"silver moonlight",		/*144*/
		"holy energy",			/*145*/
		"unholy energy",		/*146*/
		"level-based damage",	/*147*/
		"[[severe poison]]",	/*148*/
		"corrupted holy energy",/*149*/
		"magic-item-stealing",	/*150*/
		"byakhee larvae",		/*151*/
		"black-star",			/*152*/
		"unnerving",			/*153*/
		"madness-fire",			/*154*/
		"force to attack",		/*155*/
		"drain bonus HP",		/*156*/
		"push",					/*157*/
		"moon-entity lick",		/*158*/
		"[[disease]] and [[poison]]",	/*159*/
		"[[orc spawn]]",		/*160*/
		// "[[ahazu abduction]]",	/**/
		"[[stone choir]]",		/* */
		"[[water vampire]]",	/* */
		"[[bloody fangs]]",		/* */
		"[[item freeing]]",		/* */
		"[[rainbow feathers]]",	/* */
		// "[[Vorlon explosion]]",	/*NA*/
		"[[cold explosion]]",	/* */
		"[[fire explosion]]",	/* */
		"[[shock explosion]]",	/* */
		"[[physical explosion]]",/* */
		// "[[Vorlon missile]]",	/*NA*/
		"[[Warmachine missile]]",/* */
		"[[clerical spell]]",	/* */
		"[[mage spell]]",		/* */
		"[[random breath type]]",/* */
		"[[random gaze type]]",	/* */
		"[[random elemental gaze]]",/* */
		"[[Amulet theft]]",		/* */
		"[[Intrinsic theft]]",	/* */
		"[[Quest Artifact theft]]"/* */
	};
	buf[0] = '\0';
	for(i = 0; i<10; i++){
		attk = &ptr->mattk[i];
		if(attk->aatyp == 0 &&
			attk->adtyp == 0 &&
			attk->damn == 0 &&
			attk->damd == 0
		) return;
		if(SIZE(attackKey) <= attk->aatyp 
			&& attk->aatyp != AT_WEAP 
			&& attk->aatyp != AT_XWEP
			&& attk->aatyp != AT_MARI
			&& attk->aatyp != AT_MAGC
		)
			impossible("attack key %d out of range %d on monster %s!", attk->aatyp, SIZE(attackKey), ptr->mname);
		if(SIZE(damageKey) <= attk->adtyp)
			impossible("damage key %d out of range on monster %s!", attk->adtyp, ptr->mname);
		if(!i){
			Sprintf(buf, "%s %dd%d %s",
				attk->aatyp == AT_WEAP ? "Weapon" :
				attk->aatyp == AT_XWEP ? "Offhand Weapon" :
				attk->aatyp == AT_MARI ? "Multiarm Weapon" :
				attk->aatyp == AT_MAGC ? "Cast" :
				attackKey[((int)attk->aatyp)],
				attk->damn,
				attk->damd,
				damageKey[((int)attk->adtyp)]
			);
		} else {
			Sprintf(eos(buf), ", %s %dd%d %s",
				attk->aatyp == AT_WEAP ? "Weapon" :
				attk->aatyp == AT_XWEP ? "Offhand Weapon" :
				attk->aatyp == AT_MARI ? "Multiarm Weapon" :
				attk->aatyp == AT_MAGC ? "Cast" :
				attackKey[((int)attk->aatyp)],
				attk->damn,
				attk->damd,
				damageKey[((int)attk->adtyp)]
			);
		}
		if(attk->lev_req > 0 || attk->ins_req > 0 || attk->san_req != 0){
			Sprintf(eos(buf), " (");
				if(attk->lev_req > 0){
					Sprintf(eos(buf), "level %d+", attk->lev_req);
				}
				if(attk->lev_req > 0 && (attk->ins_req > 0 || attk->san_req != 0)){
					Sprintf(eos(buf), " and ");
				}
				if(attk->ins_req > 0){
					Sprintf(eos(buf), "%d+ [[insight]]", attk->ins_req);
				}
				if((attk->lev_req > 0 || attk->ins_req > 0) && attk->san_req != 0){
					Sprintf(eos(buf), " and ");
				}
				if(attk->san_req < 0){
					Sprintf(eos(buf), "<%d [[sanity]]", -1*attk->san_req);
				}
				if(attk->san_req > 0){
					Sprintf(eos(buf), ">%d [[sanity]]", attk->san_req);
				}
			Sprintf(eos(buf), ")");
		}
	}
	return;
}

STATIC_OVL void
cthulhu_mind_blast()
{
	struct monst *mon, *nmon;
	int nd = 1;
	if(on_level(&rlyeh_level,&u.uz))
		nd = 5;
	if (!Tele_blind && (Unblind_telepat || (Blind_telepat && Blind) || (Blind_telepat && rn2(2)) || !rn2(10))) {
		int dmg;
		pline("It locks on to your %s!",
			(Unblind_telepat || (Blind_telepat && Blind)) ? "telepathy" :
			Blind_telepat ? "latent telepathy" : "mind");
		dmg = d(nd,15);
		dmg = reduce_dmg(&youmonst,dmg,FALSE,TRUE);
		losehp(dmg, "psychic blast", KILLED_BY_AN);
		make_stunned(itimeout_incr(HStun, dmg*10), TRUE);
		if (Sleep_resistance){
			if(!on_level(&rlyeh_level,&u.uz)) fall_asleep(-1*dmg, TRUE);
			if(!rn2(10)){
				change_usanity(-1, FALSE);
			}
		} else {
			if(!on_level(&rlyeh_level,&u.uz)) fall_asleep(-100*dmg, TRUE);
			change_usanity(-1, FALSE);
			TRANSCENDENCE_IMPURITY_UP(FALSE)
		}
	}
	for(mon=fmon; mon; mon = nmon) {
		nmon = mon->nmon;
		if (DEADMONSTER(mon)) continue;
		if (is_mind_flayer(mon->data)) continue;
		if (mindless_mon(mon)) continue;
		if(is_tettigon(mon->data) && rn2(20)) continue;
		if (mon_resistance(mon,TELEPAT) || !rn2(5)){
			mon->mhp -= d(nd,15);
			if (mon->mhp <= 0) mondied(mon);
			else {
				mon->mconf = 1;
				if(!resists_sleep(mon)){
					mon->msleeping = 1;
					slept_monst(mon);
				}
			}
		}
		else mon->msleeping = 0;
	}
}

void
spot_monster(mon)
struct monst *mon;
{
	if(couldsee(mon->mx, mon->my) &&
		distmin(mon->mx, mon->my, u.ux, u.uy) <= BOLT_LIM &&
		(canseemon(mon) && (!(mon->mappearance || mon->mundetected) || sensemon(mon)))
	){
		//May have already lost sanity from seeing it from a distance, or wiped the memory with amnesia.
		if(!mon->mtame){
			if(taxes_sanity(mon->data)){
				//Acute Sanity loss: Shock of seeing a new monster lowers sanity and may cause temporary breakdown
				if(mvitals[monsndx(mon->data)].san_lost == 0){
					mvitals[monsndx(mon->data)].san_lost = u_sanity_loss(mon);
					change_usanity(mvitals[monsndx(mon->data)].san_lost, !mon->mpeaceful);
				}
				//Stress-based Sanity loss: Stress of continuing to see a taxing monster may lower sanity. 
				else if(u.usanity > (100 + mvitals[monsndx(mon->data)].san_lost) && !save_vs_sanloss()){
					change_usanity(-1, FALSE);
				}
			}
			//May have already gained madness but re-giving it is harmless
			give_madness(mon);
		}
	}
}

STATIC_OVL void
sense_nearby_monsters()
{
	int dx, dy;
	struct monst *mtmp;
	for(dx=-1; dx<2; dx++) for(dy=-1; dy<2; dy++){
		if(!isok(u.ux+dx, u.uy+dy))
			continue;

		//Must be able to sense the monster.
		if((mtmp = m_at(u.ux+dx, u.uy+dy))
			&& (!(mtmp->mappearance || mtmp->mundetected) || sensemon(mtmp))
		){
			//May have already lost sanity from seeing it from a distance, or wiped the memory with amnesia.
			// Simple proximity is enough to cost sanity and give madnesses, but tame monsters don't.
			if(!mtmp->mtame){
				if(mvitals[monsndx(mtmp->data)].san_lost == 0 && taxes_sanity(mtmp->data)){
					mvitals[monsndx(mtmp->data)].san_lost = u_sanity_loss(mtmp);
					change_usanity(mvitals[monsndx(mtmp->data)].san_lost, !mtmp->mpeaceful);
				}
				//May have already gained madness but re-giving it is harmless
				give_madness(mtmp);
			}

			//Monster is visible.
			if(canseemon(mtmp)){
				if(!(mvitals[monsndx(mtmp->data)].seen)){
					mvitals[monsndx(mtmp->data)].seen = TRUE;
					if(Role_if(PM_TOURIST)){
						if(mtmp->mtyp == PM_STAR_ELF)
							u.uiearepairs = TRUE;
						more_experienced(experience(mtmp,0),0);
						newexplevel();
					}
				}
				if(!u.veil && !mvitals[monsndx(mtmp->data)].vis_insight && yields_insight(mtmp->data)){
					uchar insight;
					mvitals[monsndx(mtmp->data)].vis_insight = TRUE;
					insight = u_visible_insight(mtmp);
					mvitals[monsndx(mtmp->data)].insight_gained += insight;
					change_uinsight(insight);
				}
			}
		}
	}
}

STATIC_OVL
void
unseen_actions(mon)
struct monst *mon;
{
	//Note: May cause mon to change its state, including moving to a different monster chain.
	if(mon->mux == u.uz.dnum && mon->muy == u.uz.dlevel && mon->mtyp == PM_BLESSED)
		blessed_spawn(mon);
	else if(mon->mux == u.uz.dnum && mon->muy == u.uz.dlevel && mon->mtyp == PM_BEFOULED_WRAITH)
		befouled_wraith(mon);
	else if(mon->mux == u.uz.dnum && mon->muy == u.uz.dlevel && mon->mtyp == PM_GOOD_NEIGHBOR)
		good_neighbor(mon);
	else if(mon->mux == u.uz.dnum && mon->muy == u.uz.dlevel && mon->mtyp == PM_HMNYW_PHARAOH)
		dark_pharaoh(mon);
	else if(mon->mux == u.uz.dnum && mon->muy == u.uz.dlevel && mon->mtyp == PM_POLYPOID_BEING)
		polyp_pickup(mon);
	else if(mon->mux == u.uz.dnum && mon->muy == u.uz.dlevel && mon->mtyp == PM_UNBODIED)
		unbodied_heal(mon);
	else if(mon->mux == u.uz.dnum && mon->muy == u.uz.dlevel && mon->mtyp == PM_ALKILITH)
		alkilith_spawn(mon);
	else if(mon->mux == u.uz.dnum && mon->muy == u.uz.dlevel && mon->mtyp == PM_INCARNATOR_MAGGOT)
		incarnator_action(mon);
	else if(mon->mux == u.uz.dnum && mon->muy == u.uz.dlevel && mon->mtyp == PM_MOUTH_OF_THE_GOAT)
		goat_sacrifice(mon);
	else if(mon->mux == u.uz.dnum && mon->muy == u.uz.dlevel && mon->mtyp == PM_RAGE_WALKER && (check_insight() || (!rn2(u.uevent.udemigod ? 25 : 50) && roll_generic_madness(TRUE))))
		make_rage_walker_polts(u.uz.rage+3);
	else if(mon->mtyp == PM_STRANGER)
		palid_stranger(mon);
	else if(mon->mtyp == PM_PUPPET_EMPEROR_XELETH || mon->mtyp == PM_PUPPET_EMPRESS_XEDALLI)
		sib_follow(mon);
	else if(mon->mtyp == PM_TWIN_SIBLING)
		invisible_twin_act(mon);
}

static int goatkids[] = {PM_SMALL_GOAT_SPAWN, PM_GOAT_SPAWN, PM_GIANT_GOAT_SPAWN, 
						 PM_SWIRLING_MIST, PM_DUST_STORM, PM_ICE_STORM, PM_THUNDER_STORM, PM_FIRE_STORM, 
						 PM_PLAINS_CENTAUR, PM_FOREST_CENTAUR, PM_MOUNTAIN_CENTAUR,
						 PM_QUICKLING, PM_DRYAD, PM_NAIAD, PM_OREAD, PM_YUKI_ONNA, PM_DEMINYMPH};
STATIC_OVL
void
blessed_spawn(mon)
struct monst *mon;
{
	struct monst *mtmp;
	xchar xlocale, ylocale, xyloc;
	xyloc	= mon->mtrack[0].x;
	xlocale = mon->mtrack[1].x;
	ylocale = mon->mtrack[1].y;
	if(xyloc == MIGR_EXACT_XY && !mon->mpeaceful && !rn2(70)){
		mtmp = makemon(&mons[goatkids[rn2(SIZE(goatkids))]], xlocale, ylocale, MM_ADJACENTOK|NO_MINVENT|MM_NOCOUNTBIRTH);
		if(mtmp){
			mtmp->mpeaceful = 0;
			set_malign(mtmp);
		}
	}
}

STATIC_OVL
void
alkilith_spawn(struct monst *mon)
{
	struct monst *mtmp;
	xchar xlocale, ylocale, xyloc;
	xyloc	= mon->mtrack[0].x;
	xlocale = mon->mtrack[1].x;
	ylocale = mon->mtrack[1].y;
	if(xyloc == MIGR_EXACT_XY){
		if(!(moves%10)){
			for(mtmp = fmon; mtmp; mtmp = mtmp->nmon){
				if(mon->mpeaceful && mtmp->mpeaceful)
					continue;
				if(hates_holy_mon(mtmp) || taxes_sanity(mtmp->data))
					continue;
				if(!mtmp->mconf && dist2(xlocale, ylocale, mtmp->mx, mtmp->my) <= 36){
					if(!mm_resist(mtmp, mon, 0, FALSE)){
						if(canspotmon(mtmp)){
							pline("%s staggers!", Monnam(mtmp));
							mtmp->mconf = TRUE;
						}
					}
				}
			}
			if(!mon->mpeaceful){
				if(dist2(xlocale, ylocale, u.ux, u.uy) <= 36){
					if(!save_vs_sanloss()){
						You_hear("a strange buzzing!");
						change_usanity(-1, !save_vs_sanloss()); //Second save to avoid minor madness check
					}
				}
			}
		}
		if(!mon->mpeaceful && !rn2(66)){
			struct permonst *ptr = mkclass(rn2(2) ? S_DEMON : S_IMP, G_HELL);
			if(ptr){
				mtmp = makemon(ptr, xlocale, ylocale, MM_ADJACENTOK|MM_NOCOUNTBIRTH);
				if(mtmp){
					mtmp->mpeaceful = 0;
					set_malign(mtmp);
				}
			}
		}
	}
}

void
incarnator_spawn(xchar xlocale, xchar ylocale, boolean death)
{
	struct monst *mtmp;
	const static int orcs[] = {PM_HILL_ORC, PM_MORDOR_ORC, PM_ORC_SHAMAN, PM_ORC_CAPTAIN, PM_URUK_HAI, PM_URUK_CAPTAIN, PM_MORDOR_ORC_ELITE, PM_MORDOR_SHAMAN, PM_MORDOR_MARSHAL};
	int orc = ROLL_FROM(orcs);
	if(mvitals[orc].died > rnd(death ? 200 : 500)){
		mvitals[orc].died--;
		mtmp = makemon(&mons[orc], xlocale, ylocale, MM_ADJACENTOK|MM_NOCOUNTBIRTH|NO_MINVENT);
		if(mtmp){
			const static int orc_weapons[] = {ORCISH_DAGGER, ORCISH_SHORT_SWORD, ORCISH_SPEAR};
			struct obj *otmp;
			mtmp->mpeaceful = 0;
			set_malign(mtmp);

			otmp = mongets(mtmp, ROLL_FROM(orc_weapons), NO_MKOBJ_FLAGS);
			if(otmp){
				set_material_gm(otmp, rn2(3) ? BONE : MINERAL);
			}

			otmp = mongets(mtmp, ORCISH_HELM, NO_MKOBJ_FLAGS);
			if(otmp){
				set_material_gm(otmp, rn2(3) ? BONE : MINERAL);
			}

			otmp = mongets(mtmp, rn2(2) ? ORCISH_CHAIN_MAIL : ORCISH_RING_MAIL, NO_MKOBJ_FLAGS);
			if(otmp){
				set_material_gm(otmp, rn2(3) ? BONE : MINERAL);
			}

			otmp = mongets(mtmp, ORCISH_SHIELD, NO_MKOBJ_FLAGS);
			if(otmp){
				set_material_gm(otmp, rn2(3) ? BONE : MINERAL);
			}
		}
	}
}

STATIC_OVL
void
incarnator_action(struct monst *mon)
{
	xchar xlocale, ylocale, xyloc;
	xyloc	= mon->mtrack[0].x;
	xlocale = mon->mtrack[1].x;
	ylocale = mon->mtrack[1].y;
	if(xyloc == MIGR_EXACT_XY){
		if(!mon->mpeaceful && !rn2(6)){
			incarnator_spawn(xlocale, ylocale, FALSE);
		}
	}
}

STATIC_OVL
void
befouled_wraith(mon)
struct monst *mon;
{
	struct monst *mtmp;
	xchar xlocale, ylocale, xyloc;
	xyloc	= mon->mtrack[0].x;
	xlocale = mon->mtrack[1].x;
	ylocale = mon->mtrack[1].y;
	if(xyloc == MIGR_EXACT_XY){
		if(u.ux == xlocale && u.uy == ylocale && !mon->mpeaceful){
			if(!rn2(20)){
				if(rn2(10) || u.uhpbonus == 0){
					You_feel("someone step on your grave!");
					change_luck(-1);
				}
				else {
					You_feel("like you've been stabbed!");
					u.uhpbonus = max(u.uhpbonus-5, 0);
					calc_total_maxhp();
				}
			}
		}
		//Raise zombies
		else if(Role_if(PM_UNDEAD_HUNTER)
			&& In_quest(&u.uz)
			&& !m_at(xlocale, ylocale)
			&& (distmin(u.ux, u.uy, xlocale, ylocale) > 10)
			&& !rn2(800)
		){
			mtmp = makemon(&mons[PM_WOODLAND_ELF], xlocale, ylocale, MM_ADJACENTOK|MM_NOCOUNTBIRTH);
			if(mtmp){
				set_template(mtmp, ZOMBIFIED);
				set_faction(mtmp, MOON_FACTION);
				mtmp->mpeaceful = 0;
				set_malign(mtmp);
			}
		}
		xlocale += rn2(3) - 1;
		ylocale += rn2(3) - 1;
		if(isok(xlocale, ylocale)){
			mon->mtrack[1].x = xlocale;
			mon->mtrack[1].y = ylocale;
		}
	}
}

static int witchspawns[] = {PM_APPRENTICE_WITCH, PM_APPRENTICE_WITCH, PM_APPRENTICE_WITCH, PM_WITCH, 
						 PM_APPRENTICE_WITCH, PM_APPRENTICE_WITCH, PM_APPRENTICE_WITCH, PM_WITCH, 
						 PM_BABY_LONG_WORM, PM_BABY_PURPLE_WORM, PM_LONG_WORM, PM_PURPLE_WORM, PM_HUNTING_HORROR, 
						 PM_LARGE_CAT, PM_PANTHER, PM_TIGER, PM_GIANT_RAT, PM_ENORMOUS_RAT, PM_RODENT_OF_UNUSUAL_SIZE};
STATIC_OVL
void
good_neighbor(mon)
struct monst *mon;
{
	struct monst *mtmp;
	xchar xlocale, ylocale, xyloc;
	xyloc	= mon->mtrack[0].x;
	xlocale = mon->mtrack[1].x;
	ylocale = mon->mtrack[1].y;
	if(xyloc == MIGR_EXACT_XY){
		if(u.ux == xlocale && u.uy == ylocale && !mon->mpeaceful && !(uarmc && uarmc->oartifact == ART_SPELL_WARDED_WRAPPINGS_OF_)){
			You_feel("someone walking on your grave!");
			change_luck(-13);
		} else if(!mon->mpeaceful && !rn2(140)){
			mtmp = makemon(&mons[witchspawns[rn2(SIZE(witchspawns))]], xlocale, ylocale, MM_ADJACENTOK|MM_NOCOUNTBIRTH);
			if(mtmp){
				mtmp->mpeaceful = 0;
				set_malign(mtmp);
			}
		} else if((mtmp = m_at(xlocale, ylocale))){
			if(mtmp->mtyp == PM_APPRENTICE_WITCH
				|| mtmp->mtyp == PM_WITCH
				|| mtmp->mtyp == PM_COVEN_LEADER
				|| mtmp->mtyp == PM_WITCH_S_FAMILIAR
			){
				//Heal and fix troubles
				if(needs_familiar(mtmp)){
					struct monst *familiar;
					familiar = makemon(&mons[PM_WITCH_S_FAMILIAR], mtmp->mx, mtmp->my, MM_ADJACENTOK|MM_NOCOUNTBIRTH);
					if(familiar){
						//Sync new familiar
						familiar->m_lev = mtmp->m_lev;
						familiar->mhpmax = mtmp->mhpmax;
						familiar->mhp = familiar->mhpmax;
						familiar->mvar_witchID = (long)mtmp->m_id;
						familiar->mpeaceful = mtmp->mpeaceful;
						//Stop running
						if(mtmp->mflee && mtmp->mhp > mtmp->mhpmax/2){
							mtmp->mflee = 0;
							mtmp->mfleetim = 0;
						}
					}
				}
				if(canseemon(mtmp) && mtmp->mhp < mtmp->mhpmax)
					pline("%s looks much better!", Monnam(mtmp));
				mtmp->mhp = mtmp->mhpmax;
				mtmp->mspec_used = 0;
				mtmp->mstdy = 0;
				mtmp->ustdym = 0;
				set_mcan(mtmp, FALSE);
				
				mtmp->mflee = 0;
				mtmp->mfleetim = 0;
				mtmp->mcansee = 1;
				mtmp->mblinded = 0;
				mtmp->mcanhear = 1;
				mtmp->mdeafened = 0;
				mtmp->mcanmove = 1;
				mtmp->mfrozen = 0;
				mtmp->msleeping = 0;
				mtmp->mstun = 0;
				mtmp->mconf = 0;
				mtmp->mtrapped = 0;
				mtmp->entangled_otyp = 0;
				mtmp->entangled_oid = 0;
			}
		}
		xlocale += rn2(3) - 1;
		ylocale += rn2(3) - 1;
		if(isok(xlocale, ylocale) && !is_pool(xlocale, ylocale, FALSE) && ZAP_POS(levl[xlocale][ylocale].typ)){
			mon->mtrack[1].x = xlocale;
			mon->mtrack[1].y = ylocale;
		}
	}
}

STATIC_OVL
void
good_neighbor_visible(mon)
struct monst *mon;
{
	struct monst *mtmp;
	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon){
		if(DEADMONSTER(mtmp))
			continue;
		if(!(mtmp->mtyp == PM_APPRENTICE_WITCH
			|| mtmp->mtyp == PM_WITCH
			|| mtmp->mtyp == PM_COVEN_LEADER
			|| mtmp->mtyp == PM_WITCH_S_FAMILIAR
		)) continue;

		//Doesn't always help lower-ranked followers (always helps the coven leader)
		if(mtmp->mtyp == PM_APPRENTICE_WITCH && rn2(4)) continue;
		if(mtmp->mtyp == PM_WITCH && rn2(3)) continue;
		if(mtmp->mtyp == PM_WITCH_S_FAMILIAR && rn2(2)) continue;

		if(distmin(mon->mx, mon->my, mtmp->mx, mtmp->my) < 9){
			//Heal and fix troubles
			if(needs_familiar(mtmp)){
				struct monst *familiar;
				familiar = makemon(&mons[PM_WITCH_S_FAMILIAR], mon->mx, mon->my, MM_ADJACENTOK|MM_NOCOUNTBIRTH);
				if(familiar){
					if(canseemon(familiar))
						pline("%s creates %s!", Monnam(mon), mon_nam(familiar));
					//Sync new familiar
					familiar->m_lev = mtmp->m_lev;
					familiar->mhpmax = mtmp->mhpmax;
					familiar->mhp = familiar->mhpmax;
					familiar->mvar_witchID = (long)mtmp->m_id;
					familiar->mpeaceful = mtmp->mpeaceful;
					//Stop running
					if(mtmp->mflee && mtmp->mhp > mtmp->mhpmax/2){
						if(canseemon(mtmp))
							pline("%s stops fleeing!", Monnam(mtmp));
						mtmp->mflee = 0;
						mtmp->mfleetim = 0;
					}
				}
			}
			if(canseemon(mtmp) && mtmp->mhp < mtmp->mhpmax)
				pline("%s looks much better!", Monnam(mtmp));
			mtmp->mhp = mtmp->mhpmax;
			mtmp->mspec_used = 0;
			mtmp->mstdy = 0;
			mtmp->ustdym = 0;
			set_mcan(mtmp, FALSE);
			
			mtmp->mflee = 0;
			mtmp->mfleetim = 0;
			mtmp->mcansee = 1;
			mtmp->mblinded = 0;
			mtmp->mcanhear = 1;
			mtmp->mdeafened = 0;
			mtmp->mcanmove = 1;
			mtmp->mfrozen = 0;
			mtmp->msleeping = 0;
			mtmp->mstun = 0;
			mtmp->mconf = 0;
			mtmp->mtrapped = 0;
			mtmp->entangled_otyp = 0;
			mtmp->entangled_oid = 0;
			
			break; //ends the loop
		}
	}
}

static int pharaohspawns[] = {PM_COBRA, PM_COBRA, PM_COBRA, PM_SERPENT_NECKED_LIONESS, PM_HUNTING_HORROR,
							  PM_COBRA, PM_COBRA, PM_COBRA, PM_SERPENT_NECKED_LIONESS, PM_HUNTING_HORROR,
							  PM_HUMAN_MUMMY, PM_HUMAN_MUMMY, PM_HUMAN_MUMMY, PM_GIANT_MUMMY, PM_PHARAOH,
							  PM_ENERGY_VORTEX, PM_ENERGY_VORTEX, PM_ENERGY_VORTEX, PM_LIGHTNING_PARAELEMENTAL, PM_BLUE_DRAGON, PM_DAUGHTER_OF_NAUNET};

static int toughpharaohspawns[] = {PM_COBRA, PM_SERPENT_NECKED_LIONESS, PM_HUNTING_HORROR,
							  PM_GIANT_MUMMY, PM_PHARAOH,
							  PM_LIGHTNING_PARAELEMENTAL, PM_BLUE_DRAGON, PM_DAUGHTER_OF_NAUNET};


STATIC_OVL
void
dark_pharaoh(mon)
struct monst *mon;
{
	struct monst *mtmp;
	xchar xlocale, ylocale, xyloc;
	xyloc	= mon->mtrack[0].x;
	xlocale = mon->mtrack[1].x;
	ylocale = mon->mtrack[1].y;
	struct obj *otmp;
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon){
		if(mtmp->mtyp != PM_NITOCRIS && mtmp->mtyp != PM_GHOUL_QUEEN_NITOCRIS)
			continue;
		//found her
		if(xyloc == MIGR_EXACT_XY){
			mon->mtrack[1].x = mtmp->mx;
			mon->mtrack[1].y = mtmp->my;
			xlocale = mon->mtrack[1].x;
			ylocale = mon->mtrack[1].y;
			if(which_armor(mtmp, W_ARMC) && which_armor(mtmp, W_ARMC)->oartifact == ART_SPELL_WARDED_WRAPPINGS_OF_){
				//Directed to cast spells on her behalf
				mtmp->mspec_used = 0;
			} else if(mtmp->mtyp == PM_NITOCRIS){
				//No longer protected, kill her
				if(canseemon(mtmp)){
					pline("Dark waters pour into %s's mouth and throat!", mon_nam(mtmp));
					pline("%s drowns!", Monnam(mtmp));
				}
				mtmp->mhp = 0;
				mondied(mtmp);
			} else if(mtmp->mtyp == PM_GHOUL_QUEEN_NITOCRIS && !rn2(2)){
				if(mtmp->mhp < mtmp->mhpmax/2){
					mtmp->mhp = mtmp->mhpmax;
					if(canseemon(mtmp)) pline("Dark waters seal %s's wounds!", mon_nam(mtmp));
				} else {
					mtmp->mhp = min(mtmp->mhp+9, mtmp->mhpmax);
					mtmp->mspec_used = 0;
					mtmp->mstdy = 0;
					mtmp->ustdym = 0;
					set_mcan(mtmp, FALSE);
					
					mtmp->mflee = 0;
					mtmp->mfleetim = 0;
					mtmp->mcansee = 1;
					mtmp->mblinded = 0;
					mtmp->mcanhear = 1;
					mtmp->mdeafened = 0;
					mtmp->mcanmove = 1;
					mtmp->mfrozen = 0;
					mtmp->msleeping = 0;
					mtmp->mstun = 0;
					mtmp->mconf = 0;
					mtmp->mtrapped = 0;
					mtmp->entangled_otyp = 0;
					mtmp->entangled_oid = 0;
				}
			}
		}
		if(!rn2(140)){
			struct permonst *pm;
			pm = &mons[pharaohspawns[rn2(SIZE(pharaohspawns))]];
			mtmp = makemon(pm, xlocale, ylocale, MM_ADJACENTOK|MM_NOCOUNTBIRTH);
			if(mtmp){
				mtmp->mpeaceful = 0;
				set_malign(mtmp);
			}
			mtmp = makemon(pm, xlocale, ylocale, MM_ADJACENTOK|MM_NOCOUNTBIRTH);
			if(mtmp){
				mtmp->mpeaceful = 0;
				set_malign(mtmp);
			}
		}
		return;//No further action.
	}
	struct obj *obj = 0;
	for (obj = fobj; obj; obj = obj->nobj) {
		if(obj->otyp == CORPSE && (obj->corpsenm == PM_NITOCRIS || obj->corpsenm == PM_GHOUL_QUEEN_NITOCRIS))
			break;
	}
	if(!obj) for (obj = invent; obj; obj = obj->nobj) {
		if(obj->otyp == CORPSE && (obj->corpsenm == PM_NITOCRIS || obj->corpsenm == PM_GHOUL_QUEEN_NITOCRIS))
			break;
	}
	if(!obj) for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if(obj) break;
		if (DEADMONSTER(mtmp)) continue;
		for (obj = mtmp->minvent; obj; obj = obj->nobj) {
			if(obj->otyp == CORPSE && (obj->corpsenm == PM_NITOCRIS || obj->corpsenm == PM_GHOUL_QUEEN_NITOCRIS))
				break;
		}
	}
	if(obj && !rn2(2)){
		if(get_obj_location(obj, &xlocale, &ylocale, 0)){
			mon->mtrack[1].x = xlocale;
			mon->mtrack[1].y = ylocale;
		}
		if(cansee(xlocale, ylocale)) pline("Dark waters swallow Nitocris!");
		mtmp = revive(obj, FALSE);
		if(mtmp)
			rloc(mtmp, TRUE);
		return;//No further action.
	}
	if(!rn2(70)){
		struct permonst *pm;
		pm = &mons[pharaohspawns[rn2(SIZE(pharaohspawns))]];
		mtmp = makemon(pm, xlocale, ylocale, MM_ADJACENTOK|MM_NOCOUNTBIRTH);
		if(mtmp){
			mtmp->mpeaceful = 0;
			set_malign(mtmp);
		}
	}
}

STATIC_OVL
void
dark_pharaoh_visible(mon)
struct monst *mon;
{
	struct monst *mtmp;
	xchar xlocale, ylocale;
	struct obj *otmp;
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon){
		if(mtmp->mtyp != PM_NITOCRIS && mtmp->mtyp != PM_GHOUL_QUEEN_NITOCRIS)
			continue;
		//found her
		if(which_armor(mtmp, W_ARMC) && which_armor(mtmp, W_ARMC)->oartifact == ART_SPELL_WARDED_WRAPPINGS_OF_){
			static long nitocastturn = 0L;
			//Directed to cast spells on her behalf
			if(canseemon(mtmp) && canseemon(mon)){
				if((nitocastturn + 10) < monstermoves)
					pline("%s opens %s %s and a nimbus of dark mist forms around %s.",
						Monnam(mon), mhis(mon), mbodypart(mon, HAND), mon_nam(mtmp));
				nitocastturn = monstermoves;
			}
			else if(canseemon(mtmp)){
				if((nitocastturn + 10) < monstermoves)
					pline("A nimbus of dark mist forms around %s.", mon_nam(mtmp));
				nitocastturn = monstermoves;
			}
			mtmp->mspec_used = 0;
		} else if(mtmp->mtyp == PM_NITOCRIS){
			//No longer protected, kill her
			if(canseemon(mtmp) && canseemon(mon)){
				pline("%s smirks grotesquely and gestures for the dark waters to rise.", Monnam(mon));
				pline("The waters pour into %s's mouth and throat!", mon_nam(mtmp));
				pline("%s drowns!", Monnam(mtmp));
			}
			else if(canseemon(mtmp)){
				pline("Dark waters pour into %s's mouth and throat!", mon_nam(mtmp));
				pline("%s drowns!", Monnam(mtmp));
			}
			else if(canseemon(mon)){
				pline("%s smirks grotesquely and gestures for the dark waters to rise.", mon_nam(mtmp));
			}
			mtmp->mhp = 0;
			mondied(mtmp);
		} else if(mtmp->mtyp == PM_GHOUL_QUEEN_NITOCRIS && !rn2(2)){
			if(mtmp->mhp < mtmp->mhpmax/2){
				mtmp->mhp = mtmp->mhpmax;
				if(canseemon(mon) && canseemon(mtmp))
					pline("Dark waters rise at %s command and seal %s's wounds!", s_suffix(mon_nam(mon)), mon_nam(mtmp));
				else if(canseemon(mon))
					pline("Dark waters rise at %s command.", s_suffix(mon_nam(mon)));
				else if(canseemon(mtmp))
					pline("Dark waters seal %s's wounds!", mon_nam(mtmp));
			} else {
				mtmp->mhp = min(mtmp->mhp+9, mtmp->mhpmax);
				mtmp->mspec_used = 0;
				mtmp->mstdy = 0;
				mtmp->ustdym = 0;
				set_mcan(mtmp, FALSE);
				
				mtmp->mflee = 0;
				mtmp->mfleetim = 0;
				mtmp->mcansee = 1;
				mtmp->mblinded = 0;
				mtmp->mcanhear = 1;
				mtmp->mdeafened = 0;
				mtmp->mcanmove = 1;
				mtmp->mfrozen = 0;
				mtmp->msleeping = 0;
				mtmp->mstun = 0;
				mtmp->mconf = 0;
				mtmp->mtrapped = 0;
				mtmp->entangled_otyp = 0;
				mtmp->entangled_oid = 0;
			}
		}
	}
	struct obj *obj = 0;
	for (obj = fobj; obj; obj = obj->nobj) {
		if(obj->otyp == CORPSE && (obj->corpsenm == PM_NITOCRIS || obj->corpsenm == PM_GHOUL_QUEEN_NITOCRIS))
			break;
	}
	if(!obj) for (obj = invent; obj; obj = obj->nobj) {
		if(obj->otyp == CORPSE && (obj->corpsenm == PM_NITOCRIS || obj->corpsenm == PM_GHOUL_QUEEN_NITOCRIS))
			break;
	}
	if(!obj) for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if(obj) break;
		if (DEADMONSTER(mtmp)) continue;
		for (obj = mtmp->minvent; obj; obj = obj->nobj) {
			if(obj->otyp == CORPSE && (obj->corpsenm == PM_NITOCRIS || obj->corpsenm == PM_GHOUL_QUEEN_NITOCRIS))
				break;
		}
	}
	if(obj && !rn2(2)){
		if(canseemon(mon))
			pline("%s gestures upwards.", s_suffix(Monnam(mon)));
		if(get_obj_location(obj, &xlocale, &ylocale, 0)){
			if(cansee(xlocale, ylocale)) pline("Dark waters swallow Nitocris!");
			mtmp = revive(obj, FALSE);
			if(mtmp)
				rloc(mtmp, TRUE);
		}
		return;//No further action.
	}
	if(!rn2(7)){
		struct permonst *pm;
		pm = &mons[toughpharaohspawns[rn2(SIZE(toughpharaohspawns))]];
		mtmp = makemon(pm, mon->mx, mon->my, MM_ADJACENTOK|MM_NOCOUNTBIRTH);
		if(mtmp){
			mtmp->mpeaceful = 0;
			set_malign(mtmp);
			if(canseemon(mtmp) || canseemon(mon)){
				if(canseemon(mtmp) && canseemon(mon)){
					pline("%s beckons and %s rises from the black flowing water at %s feet!", Monnam(mon), a_monnam(mtmp), mhis(mon));
				}
				else if(canseemon(mon)){
					pline("%s beckons and the black waters flow in response!", Monnam(mon));
				}
				else if(canseemon(mtmp)){
					pline("%s rises from the black flowing water!", Amonnam(mtmp));
				}
			}
		}
	}
}

STATIC_OVL
void
polyp_pickup(mon)
struct monst *mon;
{
	struct obj *otmp, *otmp2;
	register struct monst *mtmp, *mtmp0 = 0, *mtmp2;
	xchar xlocale, ylocale, xyloc;
	xyloc	= mon->mtrack[0].x;
	xlocale = mon->mtrack[1].x;
	ylocale = mon->mtrack[1].y;
	if(xyloc == MIGR_EXACT_XY){
		if(m_at(xlocale, ylocale))
			return;
		if(xlocale == u.ux && ylocale == u.uy)
			return;
		if(!ZAP_POS(levl[xlocale][ylocale].typ))
			return;
		for(otmp = level.objects[xlocale][ylocale]; otmp; otmp = otmp2){
			otmp2 = otmp->nexthere;
			if(otmp->otyp == MASK && otmp->corpsenm != NON_PM && !otmp->oartifact && !(mons[otmp->corpsenm].geno&G_UNIQ)){
				obj_extract_self(otmp);
				/* unblock point after extract, before pickup */
				if (is_boulder(otmp)) /*Shouldn't be a boulder, but who knows if a huge mask will get invented*/
					unblock_point(xlocale,ylocale);	/* vision */
				if(otmp) (void) mpickobj(mon, otmp);	/* may merge and free otmp */
				newsym(xlocale,ylocale);
			}
		}
		for(otmp = mon->minvent; otmp; otmp = otmp->nobj){
			if(otmp->otyp == MASK && otmp->corpsenm != NON_PM && !otmp->oartifact && !(mons[otmp->corpsenm].geno&G_UNIQ)){
				for(mtmp = migrating_mons; mtmp; mtmp = mtmp2) {
					mtmp2 = mtmp->nmon;
					if (mtmp == mon) {
						if(!mtmp0)
							migrating_mons = mtmp->nmon;
						else
							mtmp0->nmon = mtmp->nmon;
						mon_arrive(mtmp, FALSE);
						break;
					} else
						mtmp0 = mtmp;
				}
				if(mtmp){
					/*mtmp and mon *should* now be the same.  However, only do the polymorph if we have successfully removed the monster from the migrating chain and placed it!*/
					int pm = otmp->corpsenm;
					if(canseemon(mon))
						pline("%s puts on a mask!", Monnam(mon));
					m_useup(mon, otmp);
					mon->ispolyp = TRUE;
					newcham(mon, pm, FALSE, FALSE);
					mon->m_insight_level = 0;
					m_dowear(mon, TRUE);
					init_mon_wield_item(mon);
					m_level_up_intrinsic(mon);

					/*Break out of loop. Warning Note: otmp is stale*/
					break;
				}
			}
		}
	}
}

STATIC_OVL
void
unbodied_heal(mon)
struct monst *mon;
{
	struct obj *otmp, *otmp2;
	register struct monst *mtmp, *mtmp0 = 0, *mtmp2;
	xchar xlocale, ylocale, xyloc;
	xyloc	= mon->mtrack[0].x;
	xlocale = mon->mtrack[1].x;
	ylocale = mon->mtrack[1].y;
	if(xyloc == MIGR_EXACT_XY){
		if(mon->mhp == mon->mhpmax){
			for(mtmp = migrating_mons; mtmp; mtmp = mtmp2) {
				mtmp2 = mtmp->nmon;
				if (mtmp == mon) {
					if(!mtmp0)
						migrating_mons = mtmp->nmon;
					else
						mtmp0->nmon = mtmp->nmon;
					mon_arrive(mtmp, FALSE);
					break;
				} else
					mtmp0 = mtmp;
			}
			if(mtmp){
				/*mtmp and mon *should* now be the same.  However, only do the polymorph if we have successfully removed the monster from the migrating chain and placed it!*/
				int pm = counter_were(monsndx(mon->data));
				if(!pm) {
					impossible("unknown lycanthrope %s.", mon->data->mname);
					return;
				}
				were_transform(mon, pm);
			}
		}
		else if(!mon->mcan){
			//Heal
			int healing = d(10,6);
			mon->mhp += healing;
			//Will reappear next turn
			if(mon->mhp > mon->mhpmax)
				mon->mhp = mon->mhpmax;
			for(mtmp = fmon; mtmp; mtmp = mtmp->nmon){
				if(mon->mpeaceful == mtmp->mpeaceful && !mm_grudge(mon, mtmp, FALSE)){
					if(mtmp->mhp < mtmp->mhpmax && dist2(xlocale, ylocale, mtmp->mx, mtmp->my) <= 3*3 + 1){
						mtmp->mhp += healing;
						if(mtmp->mhp > mtmp->mhpmax)
							mtmp->mhp = mtmp->mhpmax;
						if(canseemon(mtmp))
							pline("%s looks better!", Monnam(mtmp));
					}
				}
			}
			if (mon->mtame
				&& (*hp(&youmonst) < *hpmax(&youmonst))
				&& dist2(xlocale, ylocale, u.ux, u.uy) <= 3*3 + 1)
			{
				healup(healing, 0, FALSE, FALSE);
				You("feel better.");
			}
		}
		else mon->mhp++; //Won't go over mhpmax
	}
}

STATIC_OVL
void
goat_sacrifice(mon)
struct monst *mon;
{
	struct obj *otmp, *otmp2;
	struct monst *mtmp, *mtmp0 = 0, *mtmp2;
	xchar xlocale, ylocale, xyloc;
	xyloc	= mon->mtrack[0].x;
	xlocale = mon->mtrack[1].x;
	ylocale = mon->mtrack[1].y;
	if(xyloc == MIGR_EXACT_XY){
		for(otmp = level.objects[xlocale][ylocale]; otmp; otmp = otmp2){
			otmp2 = otmp->nexthere;
			if(otmp->otyp == CORPSE && !otmp->oartifact && !rn2(4)){
				goat_eat(otmp, GOAT_EAT_PASSIVE); //No matter what, the this function should remove this corpse.  Either via resurrection or destruction
				//Warning note: otmp is now stale
				return;
			}
		}
		if(!isok(xlocale, ylocale))
			return;
		//else
		if(xlocale == u.ux && ylocale == u.uy){
			if(!rn2(20)) switch(rnd(3)){
				case 1:
					if(!Blind){
						if(u.veil)
							pline("The mist moves oddly!");
						else pline("The shadowy mist forms briefly into a yawning maw!");
					}
				break;
				case 2:
					if(!Blind){
						if(u.veil)
							pline("The shadows writhe in the corners of your vision!");
						else pline("The dark forms into hooves and writhing tendrils!");
					}
				break;
				case 3:
					if(uarmh && uarmh->otyp == SEDGE_HAT){
						pline("A few drops of liquid hit your wide straw hat.");
					} else if(uarmh && uarmh->otyp == WAR_HAT) {
						pline("A few drops of liquid hit your wide helm.");
					} else if(uarmh && uarmh->otyp == WITCH_HAT) {
						pline("A few drops of liquid hit your wide conical hat.");
					} else if(uarmh && uarmh->otyp == WIDE_HAT) {
						pline("A few drops of liquid hit your wide hat.");
					} else if(magic_negation(&youmonst)){
						pline("A few drops of liquid drip onto your clothes.");
					} else {
						int dmg = d(1, 3);
						pline("A few drops of spittle drip onto you.");
						if (!Acid_resistance) {
							pline("It burns!");
							losehp(dmg, "hungry goat", KILLED_BY_AN);
						}
					}
				break;
			}
		} else if(m_at(xlocale, ylocale)){
			struct monst *mtmp = m_at(xlocale, ylocale);
			if(!rn2(60)){
				if(which_armor(mtmp, W_ARMH) && which_armor(mtmp, W_ARMH)->otyp == SEDGE_HAT){
					if(canseemon(mtmp))
						pline("A few drops of viscous liquid hit %s wide straw hat.", s_suffix(mon_nam(mtmp)));
				} else if(which_armor(mtmp, W_ARMH) && which_armor(mtmp, W_ARMH)->otyp == WAR_HAT) {
					if(canseemon(mtmp))
						pline("A few drops of viscous liquid hit %s wide helm.", s_suffix(mon_nam(mtmp)));
				} else if(which_armor(mtmp, W_ARMH) && which_armor(mtmp, W_ARMH)->otyp == WITCH_HAT) {
					if(canseemon(mtmp))
						pline("A few drops of viscous liquid hit %s wide conical hat.", s_suffix(mon_nam(mtmp)));
				} else if(which_armor(mtmp, W_ARMH) && which_armor(mtmp, W_ARMH)->otyp == WIDE_HAT) {
					if(canseemon(mtmp))
						pline("A few drops of viscous liquid hit %s wide hat.", s_suffix(mon_nam(mtmp)));
				} else if(magic_negation(mtmp)){
					if(canseemon(mtmp))
						pline("A few drops of viscous liquid hit %s clothes.", s_suffix(mon_nam(mtmp)));
				} else {
					int dmg = d(1, 3);
					if(canseemon(mtmp))
						pline("A few drops of viscous liquid hit %s.", mon_nam(mtmp));
					if (!resists_acid(mtmp)){
						if(mtmp->data->mmove) //Not naturally inactive
							pline("%s winces!",Monnam(mtmp));
						mtmp->mhp -= dmg;
						if(mtmp->mhp <= 0)
							mondied(mtmp);
					}
				}
			}
		} else {
			if(!rn2(60)){
				if(cansee(xlocale, ylocale)){
					You("see a viscous liquid dripping onto %s.", the(surface(xlocale, ylocale)));
					map_invisible(xlocale, ylocale);
				}
				else {
					You_hear("a viscous liquid dripping onto %s.", the(surface(xlocale, ylocale)));
				}
			}
		}
	}
}

STATIC_OVL
void
palid_stranger(mon)
struct monst *mon;
{
	struct obj *otmp, *otmp2;
	struct monst *mtmp, *mtmp0 = 0, *mtmp2;
	xchar xlocale, ylocale, xyloc;
	xyloc	= mon->mtrack[0].x;
	xlocale = mon->mtrack[1].x;
	ylocale = mon->mtrack[1].y;
	/* Madmen aren't followed untill they've touched the quest artifact (Other roles won't face the stranger except due to very rare bones files) */
	if(!Role_if(PM_MADMAN) || quest_status.touched_artifact){
		/* The Stranger will eventually follow between branches */
		if(mon->mux != u.uz.dnum){
			if(!rn2(555))
				mon->mux = u.uz.dnum;
			return;
		}
		/* The Stranger follows between levels */
		if(mon->muy != u.uz.dlevel){
			if(!rn2(55)){
				if(mon->muy > u.uz.dlevel)
					mon->muy--;
				else if(mon->muy < u.uz.dlevel)
					mon->muy++;
			}
			return;
		}
		/* The Stranger arrives from other levels and appears as soon as you gain enough insight */
		if(mon->m_insight_level <= Insight && quest_status.touched_artifact){
			for(mtmp = migrating_mons; mtmp; mtmp = mtmp2){
				mtmp2 = mtmp->nmon;
				if (mtmp == mon) {
					mtmp->mtrack[0].x = MIGR_RANDOM;
					if(!mtmp0)
						migrating_mons = mtmp->nmon;
					else
						mtmp0->nmon = mtmp->nmon;
					mon_arrive(mtmp, FALSE);
					mtmp->msleeping = mtmp->mtame = mtmp->mpeaceful = 0;
					set_malign(mtmp);
					break;
				} else
					mtmp0 = mtmp;
			}
			return;
		}
	}
	/* Otherwise, The Stranger acts against you */
	if(mon->mux == u.uz.dnum && mon->muy == u.uz.dlevel && xyloc == MIGR_EXACT_XY){
		flags.yello_level=1;
		//Smite counter runs
		if (u.yel_cnt && (!Role_if(PM_MADMAN) || quest_status.touched_artifact))
			u.yel_cnt--;
		if(rn2(5)){ /*Sometimes skip a turn so that it can be evaded*/
			if(u.ux == xlocale && u.uy == ylocale && !mon->mpeaceful){
				static long lastfelt = 0L;
				if((moves - lastfelt) > 55){
					You_feel("a stranger's gaze on your back!");
				}
				lastfelt = moves;
				u.ustdy = max_ints(u.ustdy, min_ints(5, u.ustdy+rnd(5)));
				if (!Role_if(PM_MADMAN) || quest_status.touched_artifact){
					if(u.yel_cnt < 4)
						u.yel_cnt = 0;
					else
						u.yel_cnt -= 4; //5 total
				}
			}
			else {
				xlocale += sgn(u.ux - xlocale);
				ylocale += sgn(u.uy - ylocale);
				if(isok(xlocale, ylocale) && (
					(!is_pool(xlocale, ylocale, FALSE) && ZAP_POS(levl[xlocale][ylocale].typ))
					|| is_pool(mon->mtrack[1].x, mon->mtrack[1].y, FALSE) 
					|| !ZAP_POS(levl[mon->mtrack[1].x][mon->mtrack[1].y].typ)
					|| ((!Role_if(PM_MADMAN) || quest_status.touched_artifact) && !rn2(5)) /* Sometimes phases through walls */
				)){
					mon->mtrack[1].x = xlocale;
					mon->mtrack[1].y = ylocale;
				}
			}
		}
	}
}

STATIC_OVL
void
sib_follow(mon)
struct monst *mon;
{
	struct obj *otmp, *otmp2;
	struct monst *mtmp, *mtmp0 = 0, *mtmp2;
	xchar xlocale, ylocale, xyloc;
	xyloc	= mon->mtrack[0].x;
	xlocale = mon->mtrack[1].x;
	ylocale = mon->mtrack[1].y;
	/* Will eventually follow between branches */
	if(mon->mux != u.uz.dnum){
		if(!rn2(55))
			mon->mux = u.uz.dnum;
		return;
	}

	/* Follows between levels */
	if(mon->muy != u.uz.dlevel){
		if(!rn2(5)){
			if(mon->muy > u.uz.dlevel)
				mon->muy--;
			else if(mon->muy < u.uz.dlevel)
				mon->muy++;
		}
		return;
	}
	/* Arrives from other levels (and from death) and appears as soon as you gain enough insight */
	if(mon->m_insight_level <= Insight && (!mon->mvar_yellow_lifesaved || !rn2(55))){
		mon->mvar_yellow_lifesaved = FALSE;
		for(mtmp = migrating_mons; mtmp; mtmp = mtmp2){
			mtmp2 = mtmp->nmon;
			if (mtmp == mon) {
				mtmp->mtrack[0].x = MIGR_RANDOM;
				if(!mtmp0)
					migrating_mons = mtmp->nmon;
				else
					mtmp0->nmon = mtmp->nmon;
				mon_arrive(mtmp, FALSE);
				mtmp->msleeping = mtmp->mtame = mtmp->mpeaceful = 0;
				set_malign(mtmp);
				break;
			} else
				mtmp0 = mtmp;
		}
		return;
	}
}

STATIC_OVL
void
invisible_twin_act(mon)
struct monst *mon;
{
	//No actions while "dead"
	if(mon->mvar_twin_lifesaved)
		return;
	//Drain monster behind you while 
	if(!(u.specialSealsActive&SEAL_YOG_SOTHOTH)){
		// if(utrack[utpnt].x
		///!!! Tracks are not persistent!
	}
	else if(mon->mwait){
		return;
	}
	else {
		struct obj *otmp, *otmp2;
		struct monst *mtmp, *mtmp0 = 0, *mtmp2;
		xchar xlocale, ylocale, xyloc;
		xyloc	= mon->mtrack[0].x;
		xlocale = mon->mtrack[1].x;
		ylocale = mon->mtrack[1].y;
		/* Will eventually follow between branches */
		if(mon->mux != u.uz.dnum){
			if(!rn2(66))
				mon->mux = u.uz.dnum;
			return;
		}
		/* Follows between levels */
		if(mon->muy != u.uz.dlevel){
			if(!rn2(6)){
				if(mon->muy > u.uz.dlevel)
					mon->muy--;
				else if(mon->muy < u.uz.dlevel)
					mon->muy++;
			}
			return;
		}
		/* Arrives from other levels and appears as soon as you gain enough insight */
		if(mon->m_insight_level <= Insight && u.specialSealsActive&SEAL_YOG_SOTHOTH){
			for(mtmp = migrating_mons; mtmp; mtmp = mtmp2){
				mtmp2 = mtmp->nmon;
				if (mtmp == mon) {
					mtmp->mtrack[0].x = MIGR_RANDOM;
					if(!mtmp0)
						migrating_mons = mtmp->nmon;
					else
						mtmp0->nmon = mtmp->nmon;
					mon_arrive(mtmp, FALSE);
					mtmp->msleeping = 0;
					break;
				} else
					mtmp0 = mtmp;
			}
		}
	}
}

void
dogoat()
{
	dogoat_mon(&youmonst);
}

void
dogoat_mon(magr)
struct monst *magr;
{
	struct monst *mdef;
	extern const int clockwisex[8];
	extern const int clockwisey[8];
	int i = rnd(8),j;
	int ax, ay;
	struct attack symbiote = { AT_TENT, AD_DRST, 4, 4 };
	boolean youagr = (magr == &youmonst);
	boolean youdef;
	struct permonst *pa;
	
	pa = youagr ? youracedata : magr->data;
		
	if(pa->mtyp == PM_MOTHERING_MASS){
		symbiote.aatyp = AT_TENT;
		symbiote.adtyp = AD_PHYS;
		symbiote.damn = 2;
		symbiote.damd = 6;
	} else if(pa->mtyp == PM_SWIRLING_MIST){
		symbiote.aatyp = AT_BKGT;
		symbiote.adtyp = AD_WET;
		symbiote.damn = 3;
		symbiote.damd = 6;
	} else if(pa->mtyp == PM_DUST_STORM){
		symbiote.aatyp = AT_BKGT;
		symbiote.adtyp = AD_BLND;
		symbiote.damn = 3;
		symbiote.damd = 8;
	} else if(pa->mtyp == PM_ICE_STORM){
		symbiote.aatyp = AT_BKGT;
		symbiote.adtyp = AD_COLD;
		symbiote.damn = 3;
		symbiote.damd = 8;
	} else if(pa->mtyp == PM_THUNDER_STORM){
		symbiote.aatyp = AT_BKGT;
		symbiote.adtyp = AD_ELEC;
		symbiote.damn = 3;
		symbiote.damd = 8;
	} else if(pa->mtyp == PM_FIRE_STORM){
		symbiote.aatyp = AT_BKGT;
		symbiote.adtyp = AD_FIRE;
		symbiote.damn = 3;
		symbiote.damd = 10;
	} else if(pa->mtyp == PM_MOUTH_OF_THE_GOAT){
		symbiote.aatyp = AT_BKGT;
		symbiote.adtyp = AD_EACD;
		//uses default 4d4 damage dice
	} else if(pa->mtyp == PM_BLESSED){
		//mostly uses default 4d4 damage dice
		switch(rnd(8)){
			default:
			case 1:
				symbiote.adtyp = AD_DRST;
			break;
			case 2:
				symbiote.aatyp = AT_GAZE;
				symbiote.adtyp = AD_STDY;
			break;
			case 3:
				symbiote.aatyp = AT_WEAP;
				symbiote.adtyp = AD_PHYS;
			break;
			case 4:
				symbiote.aatyp = AT_MAGC;
				symbiote.adtyp = AD_CLRC;
				symbiote.damn = 5;
				symbiote.damd = 8;
			break;
		}
	}
	//roll the bkgt attacks here, since it doesn't pass through the main subout code.
	if(symbiote.aatyp == AT_BKGT){
		switch(rnd(5)){
			case 1:
				symbiote.aatyp = AT_TUCH;
			break;
			case 2:
				symbiote.aatyp = AT_BITE;
				symbiote.adtyp = AD_PHYS;
			break;
			case 3:
				symbiote.aatyp = AT_KICK;
				symbiote.adtyp = AD_PHYS;
			break;
			case 4:
				symbiote.aatyp = AT_BUTT;
				symbiote.adtyp = AD_PHYS;
			break;
			case 5:
				symbiote.aatyp = AT_GAZE;
				symbiote.adtyp = AD_STDY;
			break;
		}
	}
	
	//Attack all surrounding foes
	for(j=8;j>=1;j--){
		ax = x(magr)+clockwisex[(i+j)%8];
		ay = y(magr)+clockwisey[(i+j)%8];
		if(youagr && u.ustuck && u.uswallow)
			mdef = u.ustuck;
		else if(!isok(ax, ay))
			continue;
		else if(onscary(ax, ay, magr))
			continue;
		else mdef = m_at(ax, ay);
		
		if(u.ux == ax && u.uy == ay)
			mdef = &youmonst;
		
		if(!mdef)
			continue;
		
		youdef = (mdef == &youmonst);

		if(youagr && (mdef->mpeaceful))
			continue;
		if(youdef && (magr->mpeaceful))
			continue;
		if(youdef && Invulnerable)
			continue;
		if(!youagr && !youdef && ((mdef->mpeaceful == magr->mpeaceful) || (!!mdef->mtame == !!magr->mtame)))
			continue;

		if(youdef && u.uswallow)
			continue;
		if(!youdef && nonthreat(mdef))
			continue;

		if(symbiote.aatyp != AT_MAGC && symbiote.aatyp != AT_GAZE){
			if((touch_petrifies(mdef->data)
				|| mdef->mtyp == PM_MEDUSA)
			 && (youagr ? !Stone_resistance : !resists_ston(magr))
			) continue;
			
			if(mdef->mtyp == PM_PALE_NIGHT)
				continue;
		}
		
		if(symbiote.aatyp == AT_MAGC)
			xcasty(magr, mdef, &symbiote, mdef->mx, mdef->my, 0);
		else if(symbiote.aatyp == AT_GAZE)
			xgazey(magr, mdef, &symbiote, -1);
		else
			xmeleehity(magr, mdef, &symbiote, (struct obj **)0, -1, 0, FALSE);
	}
}

void
dochaos_mon(magr)
struct monst *magr;
{
	struct monst *mdef;
	extern const int clockwisex[8];
	extern const int clockwisey[8];
	int i = rnd(8),j;
	int ax, ay;
	struct attack symbiote = { AT_BITE, AD_UNHY, 5, 4 };
	boolean youagr = (magr == &youmonst);
	boolean youdef;
	struct permonst *pa;
	
	pa = youagr ? youracedata : magr->data;

	//mostly uses default 5d4 damage dice
	switch(rnd(4)){
		//1: Bite
		case 2:
			symbiote.aatyp = AT_GAZE;
			symbiote.adtyp = AD_STDY;
		break;
		case 3:
			//Bad luck, curse items
			symbiote.aatyp = AT_MAGC;
			symbiote.adtyp = AD_CLRC;
		break;
		case 4:
			symbiote.aatyp = AT_SPIT;
			symbiote.adtyp = AD_BLND;
			symbiote.damn = 0;
			symbiote.damd = 0;
		break;
	}
	
	//Attack all surrounding foes
	for(j=8;j>=1;j--){
		ax = x(magr)+clockwisex[(i+j)%8];
		ay = y(magr)+clockwisey[(i+j)%8];
		if(youagr && u.ustuck && u.uswallow)
			mdef = u.ustuck;
		else if(!isok(ax, ay))
			continue;
		else if(onscary(ax, ay, magr))
			continue;
		else mdef = m_at(ax, ay);
		
		if(u.ux == ax && u.uy == ay)
			mdef = &youmonst;
		
		if(!mdef)
			continue;
		
		youdef = (mdef == &youmonst);

		if(youagr && (mdef->mpeaceful))
			continue;
		if(youdef && (magr->mpeaceful))
			continue;
		if(youdef && Invulnerable)
			continue;
		if(!youagr && !youdef && ((mdef->mpeaceful == magr->mpeaceful) || (!!mdef->mtame == !!magr->mtame)))
			continue;

		if(youdef && u.uswallow)
			continue;
		if(!youdef && nonthreat(mdef))
			continue;

		if(symbiote.aatyp != AT_MAGC && symbiote.aatyp != AT_GAZE && symbiote.aatyp != AT_SPIT){
			if((touch_petrifies(mdef->data)
				|| mdef->mtyp == PM_MEDUSA)
			 && (youagr ? !Stone_resistance : !resists_ston(magr))
			) continue;
			
			if(mdef->mtyp == PM_PALE_NIGHT)
				continue;
		}
		
		if(symbiote.aatyp == AT_MAGC)
			cast_spell(magr, mdef, &symbiote, rn2(2) ? EVIL_EYE : CURSE_ITEMS, x(mdef), y(mdef));
		else if(symbiote.aatyp == AT_GAZE)
			xgazey(magr, mdef, &symbiote, -1);
		else if(symbiote.aatyp == AT_SPIT)
			xspity(magr, &symbiote, ax, ay);
		else
			xmeleehity(magr, mdef, &symbiote, (struct obj **)0, -1, 0, FALSE);
	}
}

void
dohost_mon(magr)
struct monst *magr;
{
	struct monst *mdef;
	struct monst *nmon;
	int count_close = 0;
	int count_far = 0;
	int dist;
	boolean peace = 0;
	boolean youagr = &youmonst == magr;
	struct attack symbiote = { 0, 0, 6, 6 };
	// if(&youmonst == magr || magr->mpeaceful)
	if(youagr)
		peace = 1;
	else peace = magr->mpeaceful;

	//Loop init
	nmon = fmon;
	if(!nmon){
		if(!peace)
			nmon = &youmonst;
	}
	//mdef may be null (player attacking and no monsters on level
	while((mdef = nmon)){
		//Loop update
		if(mdef == &youmonst)
			nmon = (struct monst *)0;
		else if(mdef->nmon)
			nmon = mdef->nmon;
		else if(!peace)
			nmon = &youmonst;
		else
			nmon = (struct monst *)0;

		if(mdef->mpeaceful == peace)
			continue;
		dist = distmin(x(magr), y(magr), x(mdef), y(mdef));
		if(dist > BOLT_LIM)
			continue;
		if(youagr ? !couldsee(x(mdef), y(mdef)) : !clear_path(x(magr), y(magr), x(mdef), y(mdef)))
			continue;
		if(dist > 1)
			count_far++;
		if((touch_petrifies(mdef->data)
			|| mdef->mtyp == PM_MEDUSA)
		 && (youagr ? !Stone_resistance : !resists_ston(magr))
		) continue;
		
		if(mdef->mtyp == PM_PALE_NIGHT)
			continue;
		if(dist <= 2)
			count_close++;
	}

	switch(rn2(6)){
		//Acid blast
		case 0:
			// pline("acid blast");
			if(!count_far)
				return;
			count_far = rn2(count_far);
			//Loop init
			nmon = fmon;
			if(!nmon){
				if(!peace)
					nmon = &youmonst;
			}
			//mdef may be null (player attacking and no monsters on level
			while((mdef = nmon)){
				//Loop update
				if(mdef == &youmonst)
					nmon = (struct monst *)0;
				else if(mdef->nmon)
					nmon = mdef->nmon;
				else if(!peace)
					nmon = &youmonst;
				else
					nmon = (struct monst *)0;

				if(mdef->mpeaceful == peace)
					continue;
				dist = distmin(x(magr), y(magr), x(mdef), y(mdef));
				if(dist > BOLT_LIM)
					continue;
				if(youagr ? !couldsee(x(mdef), y(mdef)) : !clear_path(x(magr), y(magr), x(mdef), y(mdef)))
					continue;
				if(dist > 1){
					if(count_far-- > 0)
						continue;
					explode(x(mdef), y(mdef), AD_ACID, 0, d(6,6), EXPL_NOXIOUS, 1);
					return;
				}
			}
		break;
		//Sickness
		case 1:
			// pline("sickness");
			if(!count_far && !count_close)
				return;
			count_far = rn2(count_far+count_close);
			symbiote.aatyp = AT_MAGC;
			symbiote.adtyp = AD_CLRC;
			//Loop init
			nmon = fmon;
			if(!nmon){
				if(!peace)
					nmon = &youmonst;
			}
			//mdef may be null (player attacking and no monsters on level
			while((mdef = nmon)){
				//Loop update
				if(mdef == &youmonst)
					nmon = (struct monst *)0;
				else if(mdef->nmon)
					nmon = mdef->nmon;
				else if(!peace)
					nmon = &youmonst;
				else
					nmon = (struct monst *)0;

				if(mdef->mpeaceful == peace)
					continue;
				dist = distmin(x(magr), y(magr), x(mdef), y(mdef));
				if(dist > BOLT_LIM)
					continue;
				if(youagr ? !couldsee(x(mdef), y(mdef)) : !clear_path(x(magr), y(magr), x(mdef), y(mdef)))
					continue;
				if(count_far-- > 0)
					continue;
				cast_spell(magr, mdef, &symbiote, PLAGUE, x(mdef), y(mdef));
				return;
			}
		break;
		//Flesh hook
		case 2:
			// pline("flesh hook");
			if(!count_close)
				return;
			count_close = rn2(count_close);
			symbiote.aatyp = AT_LRCH;
			symbiote.adtyp = AD_HOOK;
		break;
		//Vampiric
		case 3:
			// pline("vampire");
			if(!count_close)
				return;
			count_close = rn2(count_close);
			symbiote.aatyp = AT_BITE;
			symbiote.adtyp = AD_VAMP;
		break;
		//Brain
		case 4:
			// pline("brain");
			if(!count_close)
				return;
			count_close = rn2(count_close);
			symbiote.aatyp = AT_TENT;
			symbiote.adtyp = AD_DRIN;
		break;
		//Acid slash
		case 5:
			// pline("acid slash");
			if(!count_close)
				return;
			count_close = rn2(count_close);
			symbiote.aatyp = AT_SRPR;
			symbiote.adtyp = AD_EACD;
		break;
	}
	//Loop init
	nmon = fmon;
	if(!nmon){
		if(!peace)
			nmon = &youmonst;
	}
	//mdef may be null (player attacking and no monsters on level
	while((mdef = nmon)){
		//Loop update
		if(mdef == &youmonst)
			nmon = (struct monst *)0;
		else if(mdef->nmon)
			nmon = mdef->nmon;
		else if(!peace)
			nmon = &youmonst;
		else
			nmon = (struct monst *)0;

		if(mdef->mpeaceful == peace)
			continue;
		dist = distmin(x(magr), y(magr), x(mdef), y(mdef));
		if(dist > BOLT_LIM)
			continue;
		if(youagr ? !couldsee(x(mdef), y(mdef)) : !clear_path(x(magr), y(magr), x(mdef), y(mdef)))
			continue;
		if((touch_petrifies(mdef->data)
			|| mdef->mtyp == PM_MEDUSA)
		 && (youagr ? !Stone_resistance : !resists_ston(magr))
		) continue;
		
		if(mdef->mtyp == PM_PALE_NIGHT)
			continue;
		if(dist <= 2){
			if(count_close-- > 0)
				continue;
			xmeleehity(magr, mdef, &symbiote, (struct obj **)0, -1, 0, FALSE);
			return;
		}
	}
}

void
donachash(magr)
struct monst *magr;
{
	struct monst *mdef;
	extern const int clockwisex[8];
	extern const int clockwisey[8];
	int i = rnd(8),j;
	int x, y;
	struct attack symbiote = { AT_ESPR, AD_SHDW, 1, 30 };
	boolean youagr = (magr == &youmonst);
	boolean youdef;
	struct permonst *pa;
	
	pa = youagr ? youracedata : magr->data;

	//Attack nearby foe and/or engulf items
	for(j=8;j>=1;j--){
		x = x(magr)+clockwisex[(i+j)%8];
		y = y(magr)+clockwisey[(i+j)%8];
		if(youagr && u.ustuck && u.uswallow)
			mdef = u.ustuck;
		else if(!isok(x, y))
			continue;
		else if(onscary(x, y, magr))
			continue;
		else mdef = m_at(x, y);
		
		if(u.ux == x && u.uy == y)
			mdef = &youmonst;
		
		if(!mdef && (!level.objects[x][y] || youagr))
			continue;
		
		youdef = (mdef == &youmonst);
		
		if(mdef){
			if(youagr && (mdef->mpeaceful))
				continue;
			if(youdef && (magr->mpeaceful))
				continue;
			if(!youagr && !youdef && ((mdef->mpeaceful == magr->mpeaceful) || (!!mdef->mtame == !!magr->mtame)))
				continue;

			if(youdef && u.uswallow)
				continue;
			if(!youdef && nonthreat(mdef))
				continue;

			if(mdef->mtyp == PM_PALE_NIGHT)
				continue;
			
			//Spiritual rapier means that touch petrifies monsters are safe to attack
			
			xmeleehity(magr, mdef, &symbiote, (struct obj **)0, -1, 0, FALSE);
		}
		if(!youagr)
			mvanishobj(magr, x, y);
		
		//Only send the shadow to one square per turn, if we're here we must have done SOMETHING
		return;
	}
}

void
dosnake(magr)
struct monst *magr;
{
	struct monst *mdef;
	extern const int clockwisex[8];
	extern const int clockwisey[8];
	int i = rnd(8),j;
	int mult = 1;
	int ax, ay;
	struct attack * attk;
	struct attack attkbuff = {0};
	boolean youagr = (magr == &youmonst);
	boolean youdef;
	struct permonst *pa;
	int max = 8;
	
	pa = youagr ? youracedata : magr->data;
	
	// get attack from statblock
	attk = mon_get_attacktype(magr, AT_OBIT, &attkbuff);
	if(!attk)
		return;
	
	if(pa->mtyp == PM_ANCIENT_NAGA){
		max = youagr ? 5 : magr->m_id%2 ? 7 : 5;
	}
	else if(pa->mtyp == PM_MEDUSA || pa->mtyp == PM_MOON_S_CHOSEN){
		//Medusa's hair bites 1-3 times
		// ditto the moon's chosen's cranial wolfpack
		mult = rnd(3);
	}
	
	max *= mult;
	
	//Attack all surrounding foes
	for(j=8*mult;j>=1;j--){
		ax = x(magr)+clockwisex[(i+j)%8];
		ay = y(magr)+clockwisey[(i+j)%8];
		if(youagr && u.ustuck && u.uswallow)
			mdef = u.ustuck;
		else if(!isok(ax, ay))
			continue;
		else if(onscary(ax, ay, magr))
			continue;
		else mdef = m_at(ax, ay);
		
		if(u.ux == ax && u.uy == ay)
			mdef = &youmonst;
		
		if(!mdef)
			continue;
		
		youdef = (mdef == &youmonst);

		if(youagr && (mdef->mpeaceful))
			continue;
		if(youdef && (magr->mpeaceful))
			continue;
		if(youdef && Invulnerable)
			continue;
		if(!youagr && !youdef && ((mdef->mpeaceful == magr->mpeaceful) || (!!mdef->mtame == !!magr->mtame)))
			continue;

		if(youdef && u.uswallow)
			continue;
		if(!youdef && nonthreat(mdef))
			continue;

		if(attk->aatyp != AT_MAGC && attk->aatyp != AT_GAZE){
			if((touch_petrifies(mdef->data)
				|| mdef->mtyp == PM_MEDUSA)
			 && (youagr ? !Stone_resistance : !resists_ston(magr))
			) continue;
			
			if(mdef->mtyp == PM_PALE_NIGHT)
				continue;
		}
		
		xmeleehity(magr, mdef, attk, (struct obj **)0, -1, 0, FALSE);
		// Nagas have 5 or 7 snake bites
		if(--max <= 0)
			return;
	}
}

boolean
doyog(magr)
struct monst *magr;
{
	struct monst *mdef;
	extern const int clockwisex[8];
	extern const int clockwisey[8];
	int i = rnd(8),j;
	int ax, ay;
	struct attack * attk;
	struct attack attkbuff = {0};
	boolean youagr = (magr == &youmonst);
	boolean youdef;
	boolean attacked = FALSE;
	struct permonst *pa;
	struct attack symbiote = { AT_TENT, AD_VAMP, 3, 3 };

	pa = youagr ? youracedata : magr->data;

	//Changes to match your gender
	if(!youagr)
		magr->female = flags.female;

	// get attack from statblock
	attk = mon_get_attacktype(magr, AT_TENT, &attkbuff);
	if(!attk){
		if(youagr){
			attk = &symbiote;
		}
		else return FALSE;
	}
	
	
	//Attack all surrounding foes
	for(j=8;j>=1;j--){
		ax = x(magr)+clockwisex[(i+j)%8];
		ay = y(magr)+clockwisey[(i+j)%8];
		if(youagr && u.ustuck && u.uswallow)
			mdef = u.ustuck;
		else if(!isok(ax, ay))
			continue;
		else if(onscary(ax, ay, magr))
			continue;
		else mdef = m_at(ax, ay);
		
		if(u.ux == ax && u.uy == ay)
			mdef = &youmonst;
		
		if(!mdef)
			continue;
		
		youdef = (mdef == &youmonst);

		if(youagr && (mdef->mpeaceful))
			continue;
		if(youdef && (magr->mpeaceful))
			continue;
		if(youdef && Invulnerable)
			continue;
		if(!youagr && !youdef && ((mdef->mpeaceful == magr->mpeaceful) || (!!mdef->mtame == !!magr->mtame)))
			continue;

		if(youdef && u.uswallow)
			continue;
		if(!youdef && nonthreat(mdef))
			continue;
		if(youagr){
			u.yogAttack = moves;
		}

		if(attk->aatyp != AT_MAGC && attk->aatyp != AT_GAZE){
			if((touch_petrifies(mdef->data)
				|| mdef->mtyp == PM_MEDUSA)
			 && (youagr ? !Stone_resistance : !resists_ston(magr))
			) continue;
			
			if(mdef->mtyp == PM_PALE_NIGHT)
				continue;
		}
		attacked = TRUE;
		xmeleehity(magr, mdef, attk, (struct obj **)0, -1, 0, FALSE);
	}
	return attacked;
}

void
dojellysting(magr)
struct monst *magr;
{
	struct monst *mdef;
	extern const int clockwisex[8];
	extern const int clockwisey[8];
	int i = rnd(8),j;
	int ax, ay;
	struct attack * attk;
	struct attack attkbuff = {0};
	boolean youagr = (magr == &youmonst);
	boolean youdef;
	boolean attacked = FALSE;
	struct permonst *pa;
	struct attack symbiote = { AT_TENT, AD_PAIN, u.jellyfish, 1 };
	int max = u.jellyfish*2;
	int mult = u.jellyfish/3+1;

	pa = youagr ? youracedata : magr->data;

	//Attack all surrounding foes
	for(j=8*mult;j>=1;j--){
		ax = x(magr)+clockwisex[(i+j)%8];
		ay = y(magr)+clockwisey[(i+j)%8];
		if(youagr && u.ustuck && u.uswallow)
			mdef = u.ustuck;
		else if(!isok(ax, ay))
			continue;
		else if(onscary(ax, ay, magr))
			continue;
		else mdef = m_at(ax, ay);
		
		if(u.ux == ax && u.uy == ay)
			mdef = &youmonst;
		
		if(!mdef)
			continue;
		
		if(rn2(2)) //1/2 chance to just skip anyway, stings are too powerful if they attack too frequently.
			continue;

		youdef = (mdef == &youmonst);

		if(youagr && (mdef->mpeaceful))
			continue;
		if(youdef && (magr->mpeaceful))
			continue;
		if(youdef && Invulnerable)
			continue;
		if(!youagr && !youdef && ((mdef->mpeaceful == magr->mpeaceful) || (!!mdef->mtame == !!magr->mtame)))
			continue;

		if(youdef && u.uswallow)
			continue;
		if(!youdef && nonthreat(mdef))
			continue;

		if(symbiote.aatyp != AT_MAGC && symbiote.aatyp != AT_GAZE){
			if((touch_petrifies(mdef->data)
				|| mdef->mtyp == PM_MEDUSA)
			 && (youagr ? !Stone_resistance : !resists_ston(magr))
			) continue;
			
			if(mdef->mtyp == PM_PALE_NIGHT)
				continue;
		}
		
		xmeleehity(magr, mdef, &symbiote, (struct obj **)0, -1, 0, FALSE);
		// Limited stings
		if(--max <= 0)
			return;
	}
	return;
}

STATIC_OVL void
dorotattack(struct monst *magr, struct attack * attk, int max, int mult)
{
	struct monst *mdef;
	extern const int clockwisex[8];
	extern const int clockwisey[8];
	int i = rnd(8),j;
	int ax, ay;
	boolean youagr = (magr == &youmonst);
	boolean youdef;
	boolean attacked = FALSE;
	struct permonst *pa;

	pa = youagr ? youracedata : magr->data;

	//Attack all surrounding foes
	for(j=8*mult;j>=1;j--){
		ax = x(magr)+clockwisex[(i+j)%8];
		ay = y(magr)+clockwisey[(i+j)%8];
		if(youagr && u.ustuck && u.uswallow)
			mdef = u.ustuck;
		else if(!isok(ax, ay))
			continue;
		else if(onscary(ax, ay, magr))
			continue;
		else mdef = m_at(ax, ay);
		
		if(u.ux == ax && u.uy == ay)
			mdef = &youmonst;
		
		if(!mdef)
			continue;
		
		if(rn2(2)) //1/2 chance to just skip anyway, stings are too powerful if they attack too frequently.
			continue;

		youdef = (mdef == &youmonst);

		if(youagr && (mdef->mpeaceful))
			continue;
		if(youdef && (magr->mpeaceful))
			continue;
		if(youdef && Invulnerable)
			continue;
		if(!youagr && !youdef && ((mdef->mpeaceful == magr->mpeaceful) || (!!mdef->mtame == !!magr->mtame)))
			continue;

		if(youdef && u.uswallow)
			continue;
		if(!youdef && nonthreat(mdef))
			continue;

		if(attk->aatyp != AT_MAGC && attk->aatyp != AT_GAZE){
			if((touch_petrifies(mdef->data)
				|| mdef->mtyp == PM_MEDUSA)
			 && (youagr ? !Stone_resistance : !resists_ston(magr))
			) continue;
			
			if(mdef->mtyp == PM_PALE_NIGHT)
				continue;
		}
		
		xmeleehity(magr, mdef, attk, (struct obj **)0, -1, 0, FALSE);
		// Limited attacks
		if(--max <= 0)
			return;
	}
	return;
}

void
dorotbite(struct monst *magr)
{
	struct attack * attk;
	struct attack symbiote = { AT_OBIT, AD_DISE, 1, 3 };
	dorotattack(magr, &symbiote, 1, 1);
}

void
dorotsting(struct monst *magr)
{
	struct attack * attk;
	struct attack symbiote = { AT_STNG, AD_PFBT, 1, 4 };
	dorotattack(magr, &symbiote, 1, 1);
}

void
dokraken_mon(magr)
struct monst *magr;
{
	struct monst *mdef;
	extern const int clockwisex[8];
	extern const int clockwisey[8];
	int i = rnd(8),j;
	int mult = 1;
	int ax, ay;
	struct attack * attk;
	struct attack attkbuff = {0};
	boolean youagr = (magr == &youmonst);
	boolean youdef;
	struct permonst *pa;
	int max = 8;
	
	pa = youagr ? youracedata : magr->data;
	
	// get attack from statblock
	attk = mon_get_attacktype(magr, AT_TENT, &attkbuff);
	if(!attk)
		return;

	//1-8 tentacles attack, up to all of which can be vs. one target
	max = mult = rnd(8);

	//Attack all surrounding foes
	for(j=8*mult;j>=1;j--){
		ax = x(magr)+clockwisex[(i+j)%8];
		ay = y(magr)+clockwisey[(i+j)%8];
		if(youagr && u.ustuck && u.uswallow)
			mdef = u.ustuck;
		else if(!isok(ax, ay))
			continue;
		else if(onscary(ax, ay, magr))
			continue;
		else mdef = m_at(ax, ay);
		
		if(u.ux == ax && u.uy == ay)
			mdef = &youmonst;
		
		if(!mdef)
			continue;
		
		youdef = (mdef == &youmonst);

		if(youagr && (mdef->mpeaceful))
			continue;
		if(youdef && (magr->mpeaceful))
			continue;
		if(youdef && Invulnerable)
			continue;
		if(!youagr && !youdef && ((mdef->mpeaceful == magr->mpeaceful) || (!!mdef->mtame == !!magr->mtame)))
			continue;

		if(youdef && u.uswallow)
			continue;
		if(!youdef && nonthreat(mdef))
			continue;

		if(attk->aatyp != AT_MAGC && attk->aatyp != AT_GAZE){
			if((touch_petrifies(mdef->data)
				|| mdef->mtyp == PM_MEDUSA)
			 && (youagr ? !Stone_resistance : !resists_ston(magr))
			) continue;
			
			if(mdef->mtyp == PM_PALE_NIGHT)
				continue;
		}
		
		xmeleehity(magr, mdef, attk, (struct obj **)0, -1, 0, FALSE);
		// 1-8 tentacles attack
		if(--max <= 0)
			return;
	}
}

void
dotailslap(magr)
struct monst *magr;
{
	struct monst *mdef;
	extern const int clockwisex[8];
	extern const int clockwisey[8];
	int i = rnd(8),j;
	int ax, ay;
	struct attack * attk;
	struct attack attkbuff = {0};
	boolean youagr = (magr == &youmonst);
	boolean youdef;
	struct permonst *pa;
	
	pa = youagr ? youracedata : magr->data;
	
	// get attack from statblock
	attk = mon_get_attacktype(magr, AT_TAIL, &attkbuff);
	if(!attk)
		return;
	
	//Attack one foe
	for(j=8;j>=1;j--){
		ax = x(magr)+clockwisex[(i+j)%8];
		ay = y(magr)+clockwisey[(i+j)%8];
		if(youagr && u.ustuck && u.uswallow)
			mdef = u.ustuck;
		else if(!isok(ax, ay))
			continue;
		else if(onscary(ax, ay, magr))
			continue;
		else mdef = m_at(ax, ay);
		
		if(u.ux == ax && u.uy == ay)
			mdef = &youmonst;
		
		if(!mdef)
			continue;
		
		if(rn2(3)) //2/3rds chance to just skip anyway, tails slaps grow more frequent if multiple targets are nearby.
			continue;
		
		youdef = (mdef == &youmonst);

		if(youagr && (mdef->mpeaceful))
			continue;
		if(youdef && (magr->mpeaceful))
			continue;
		if(youdef && Invulnerable)
			continue;
		if(!youagr && !youdef && ((mdef->mpeaceful == magr->mpeaceful) || (!!mdef->mtame == !!magr->mtame)))
			continue;

		if(youdef && u.uswallow)
			continue;
		if(!youdef && nonthreat(mdef))
			continue;

		if(attk->aatyp != AT_MAGC && attk->aatyp != AT_GAZE){
			if((touch_petrifies(mdef->data)
				|| mdef->mtyp == PM_MEDUSA)
			 && (youagr ? !Stone_resistance : !resists_ston(magr))
			) continue;
			
			if(mdef->mtyp == PM_PALE_NIGHT)
				continue;
		}
		
		xmeleehity(magr, mdef, attk, (struct obj **)0, -1, 0, FALSE);
		return; //Only attack one foe
	}
}

void
dovines(magr)
struct monst *magr;
{
	struct monst *mdef;
	extern const int clockwisex[8];
	extern const int clockwisey[8];
	int i = rnd(8),j;
	int ax, ay;
	struct attack * attk;
	struct attack attkbuff = {0};
	boolean youagr = (magr == &youmonst);
	boolean youdef;
	struct permonst *pa;
	
	pa = youagr ? youracedata : magr->data;
	
	// get attack from statblock
	attk = mon_get_attacktype(magr, AT_VINE, &attkbuff);
	if(!attk)
		return;
	
	//Attack one foe
	for(j=8;j>=1;j--){
		ax = x(magr)+clockwisex[(i+j)%8];
		ay = y(magr)+clockwisey[(i+j)%8];
		if(youagr && u.ustuck && u.uswallow)
			mdef = u.ustuck;
		else if(!isok(ax, ay))
			continue;
		else if(onscary(ax, ay, magr))
			continue;
		else mdef = m_at(ax, ay);
		
		if(u.ux == ax && u.uy == ay)
			mdef = &youmonst;
		
		if(!mdef)
			continue;
		
		if(mlev(magr) < 30 && rn2(31-mlev(magr))) //Vine attacks grow more likely as the moster grows more powerful.
			continue;
		
		youdef = (mdef == &youmonst);

		if(youagr && (mdef->mpeaceful))
			continue;
		if(youdef && (magr->mpeaceful))
			continue;
		if(youdef && Invulnerable)
			continue;
		if(!youagr && !youdef && ((mdef->mpeaceful == magr->mpeaceful) || (!!mdef->mtame == !!magr->mtame)))
			continue;

		if(youdef && u.uswallow)
			continue;
		if(!youdef && nonthreat(mdef))
			continue;

		if(attk->aatyp != AT_MAGC && attk->aatyp != AT_GAZE){
			if((touch_petrifies(mdef->data)
				|| mdef->mtyp == PM_MEDUSA)
			 && (youagr ? !Stone_resistance : !resists_ston(magr))
			) continue;
			
			if(mdef->mtyp == PM_PALE_NIGHT)
				continue;
		}
		
		xmeleehity(magr, mdef, attk, (struct obj **)0, -1, 0, FALSE);
	}
}

void
dostarblades(magr)
struct monst *magr;
{
	struct monst *mdef;
	extern const int clockwisex[8];
	extern const int clockwisey[8];
	int i = rnd(8),j;
	int ax, ay, n;
	struct attack * attk;
	struct attack attkbuff = {0};
	boolean youagr = (magr == &youmonst);
	boolean youdef;
	struct permonst *pa;
	
	pa = youagr ? youracedata : magr->data;
	
	// get attack from statblock
	attk = mon_get_attacktype(magr, AT_ESPR, &attkbuff);
	if(!attk && youagr && uring_art(ART_STAR_EMPEROR_S_RING)){
		attkbuff.aatyp = AT_ESPR;
		attkbuff.adtyp = AD_STAR;
		attkbuff.damn = 4;
		attkbuff.damd = 4;
		attk = &attkbuff;
	}
	if(!attk)
		return;
	// count attacks in statblock (assumes that all of these attacks are equals! If this is not true, this will need to be adjusted)
	n = mon_count_attacktype(magr, AT_ESPR);
	if(youagr && uring_art(ART_STAR_EMPEROR_S_RING)){
		n += u.ulevel/10;
	}
	//Possible there are no attacks due to star ring etc.
	if(!n)
		return;

	for(j=8*n;j>=1;j--){
		ax = x(magr)+clockwisex[(i+j)%8];
		ay = y(magr)+clockwisey[(i+j)%8];
		if(youagr && u.ustuck && u.uswallow)
			mdef = u.ustuck;
		else if(!isok(ax, ay))
			continue;
		else if(onscary(ax, ay, magr))
			continue;
		else mdef = m_at(ax, ay);
		
		if(u.ux == ax && u.uy == ay)
			mdef = &youmonst;
		
		if(!mdef)
			continue;
		
		if(!rn2(3))
			continue;
		
		youdef = (mdef == &youmonst);

		if(youagr && (mdef->mpeaceful))
			continue;
		if(youdef && (magr->mpeaceful))
			continue;
		if(youdef && Invulnerable)
			continue;
		if(!youagr && !youdef && ((mdef->mpeaceful == magr->mpeaceful) || (!!mdef->mtame == !!magr->mtame)))
			continue;

		if(youdef && u.uswallow)
			continue;
		if(!youdef && nonthreat(mdef))
			continue;

		if(mdef->mtyp == PM_PALE_NIGHT)
			continue;

		xmeleehity(magr, mdef, attk, (struct obj **)0, -1, 0, FALSE);
	}
}

void
dostorm(magr)
struct monst *magr;
{
	struct monst *mdef;
	boolean youagr = (magr == &youmonst);
	struct attack attkbuff = {AT_MAGC, AD_CLRC, 0, 6};
	struct permonst *pa;
	int spellnum = 0;
	int range = magr->mtyp == PM_DAO_LAO_GUI_MONK ? 2 : BOLT_LIM;
	boolean frequency_decrease = TRUE;
	
	pa = youagr ? youracedata : magr->data;

	if(pa->mtyp == PM_GHAELE_ELADRIN){
		spellnum = rn2(3) ? LIGHTNING : HAIL_FLURY;
	}
	else if(pa->mtyp == PM_LUMINOUS_CLOUD){
		if(rn2(2) && (mlev(magr) >= 30 || !rn2(31-mlev(magr)))){
			cast_spell(magr, (struct monst *)0, &attkbuff, MASS_CURE_CLOSE, x(magr), y(magr));
			return;
		}
		spellnum = rn2(3) ? LIGHTNING : HAIL_FLURY;
	}
	else if(pa->mtyp == PM_CAILLEA_ELADRIN){
		spellnum = ICE_STORM;
	}
	else if(pa->mtyp == PM_MOONSHADOW){
		spellnum = STARFALL;
	}
	else if(pa->mtyp == PM_PYROCLASTIC_VORTEX){
		spellnum = rn2(3) ? PYRO_STORM : rn2(2) ? MON_POISON_GAS : GOD_RAY;
	}
	else if(pa->mtyp == PM_DAO_LAO_GUI_MONK){
		spellnum = rn2(3) ? RAIN : rn2(3) ? LIGHTNING : HAIL_FLURY;
		frequency_decrease = FALSE;
	}
	else {
		spellnum = ACID_RAIN;
	}

	if(youagr && u.ustuck && u.uswallow){
		cast_spell(magr, u.ustuck, &attkbuff, spellnum, x(u.ustuck), y(u.ustuck));
		return;
	}
	for(mdef = fmon; mdef; mdef = mdef->nmon){
		if(DEADMONSTER(mdef))
			continue;

		if(youagr && (mdef->mpeaceful))
			continue;
		if(!youagr && ((mdef->mpeaceful == magr->mpeaceful) || (!!mdef->mtame == !!magr->mtame)))
			continue;

		if(distmin(x(magr), y(magr), x(mdef), y(mdef)) > range)
			continue;

		if(youagr && !canspotmon(mdef))
			continue;
		if(!youagr && !mon_can_see_mon(magr, mdef))
			continue;

		if(onscary(mdef->mx, mdef->my, magr))
			continue;

		if(!rn2(4) || (frequency_decrease && mlev(magr) < 30 && rn2(31-mlev(magr)))) //Storm attacks grow more likely as the moster grows more powerful.
			continue;

		cast_spell(magr, mdef, &attkbuff, spellnum, x(mdef), y(mdef));
	}

	if(!magr->mpeaceful && mon_can_see_you(magr)
		&& distmin(x(magr), y(magr), u.ux, u.uy) <= range
		&& !onscary(u.ux, u.uy, magr)
		&& !(!rn2(4) || (frequency_decrease && mlev(magr) < 30 && rn2(31-mlev(magr))))
	){
		cast_spell(magr, &youmonst, &attkbuff, spellnum, u.ux, u.uy);
	}

}

void
dochain_lashes(magr)
struct monst *magr;
{
	struct monst *mdef;
	extern const int clockwisex[8];
	extern const int clockwisey[8];
	struct attack attkbuff = {AT_WEAP, AD_ELEC, 3, 6};
	int i = rnd(8),j;
	int ax, ay, n;
	int chain_count = 0;
	struct obj *ofirst;
	struct obj *chain;
	boolean youagr = (magr == &youmonst);
	boolean youdef;
	struct permonst *pa;
	
	pa = youagr ? youracedata : magr->data;
	ofirst = youagr ? invent : magr->minvent;
	
	// Find chains etc:
	for(chain = ofirst; chain; chain = chain->nobj){
		if(chain->otyp == CHAIN)
			chain_count+=4;
		else if(chain->otyp == BALL)
			chain_count++;
	}
	if(!chain_count)
		return;
	chain_count = rnd(chain_count);
	for(chain = ofirst; chain; chain = chain->nobj){
		if(chain->otyp == CHAIN)
			chain_count-=4;
		else if(chain->otyp == BALL)
			chain_count--;
		if(chain_count <= 0)
			break;
	}
	if(!chain)
		return;
	
	for(j=8;j>=1;j--){
		ax = x(magr)+clockwisex[(i+j)%8];
		ay = y(magr)+clockwisey[(i+j)%8];
		if(youagr && u.ustuck && u.uswallow)
			mdef = u.ustuck;
		else if(!isok(ax, ay))
			continue;
		else if(onscary(ax, ay, magr))
			continue;
		else mdef = m_at(ax, ay);
		
		if(u.ux == ax && u.uy == ay)
			mdef = &youmonst;
		
		if(!mdef)
			continue;
		
		if(!rn2(3))
			continue;
		
		youdef = (mdef == &youmonst);

		if(youagr && (mdef->mpeaceful))
			continue;
		if(youdef && (magr->mpeaceful))
			continue;
		if(youdef && Invulnerable)
			continue;
		if(!youagr && !youdef && ((mdef->mpeaceful == magr->mpeaceful) || (!!mdef->mtame == !!magr->mtame)))
			continue;

		if(youdef && u.uswallow)
			continue;
		if(!youdef && nonthreat(mdef))
			continue;

		if(mdef->mtyp == PM_PALE_NIGHT)
			continue;

		xmeleehity(magr, mdef, &attkbuff, &chain, +3, 0, FALSE);
	}
}

void
make_rage_walker_polts(int rage)
{
	struct obj *otmp, *nobj;
	struct monst *polt;
	int ox, oy;
	boolean created = FALSE;
	int otyp = ELVEN_BROADSWORD;
	int otyp_index = 0;
	int elven_weapon_types[] = {ELVEN_BROADSWORD, ELVEN_SHORT_SWORD, ELVEN_SPEAR, ELVEN_SICKLE, ELVEN_SCIMITAR, HIGH_ELVEN_WARSWORD, ELVEN_LANCE, ELVEN_MACE};
	while(rage > 0){
		if(!otyp){
			polt = makemon(&mons[PM_POLTERGEIST], 0, 0, MM_ADJACENTOK|NO_MINVENT);
			otmp = mksobj(ROLL_FROM(elven_weapon_types), NO_MKOBJ_FLAGS);
			set_material_gm(otmp, IRON);
			rage--;
			if(otmp->spe < 3)
				otmp->spe = 3;
			curse(otmp);
			mpickobj(polt, otmp);
			m_dowear(polt, TRUE);
			continue;
		}
		created = FALSE;
		for(ox = 0; ox < COLNO && rage > 0; ox++){
		for(oy = 0; oy < ROWNO && rage > 0; oy++){
			otmp =  level.objects[ox][oy];
			if(!otmp)
				continue;
			for(; otmp; otmp = otmp->nexthere){
				if(otmp->otyp == otyp){
					polt = makemon(&mons[PM_POLTERGEIST], otmp->ox, otmp->oy, MM_ADJACENTOK|NO_MINVENT);
					obj_extract_self(otmp);
					set_material_gm(otmp, IRON);
					rage--;
					if(otmp->spe < 3)
						otmp->spe = 3;
					curse(otmp);
					mpickobj(polt, otmp);
					m_dowear(polt, TRUE);
					created = TRUE;
					break; //Break nexthere loop, continue location loop
				}
			}
		}}
		if(!created){
			otyp_index++;
			if(otyp_index < SIZE(elven_weapon_types)){
				otyp = elven_weapon_types[otyp_index];
			}
			else otyp = 0;
		}
	}
	doredraw(); //Just moved a bunch of items
}

#endif /* OVLB */

/*allmain.c*/
