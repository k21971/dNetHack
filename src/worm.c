/*	SCCS Id: @(#)worm.c	3.4	1995/01/28	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"

#define newseg()		(struct wseg *) alloc(sizeof(struct wseg))
#define dealloc_seg(wseg)	free((genericptr_t) (wseg))

/* worm segment structure */
struct wseg {
    struct wseg *nseg;
    xchar  wx, wy;	/* the segment's position */
};

STATIC_DCL void FDECL(toss_wsegs, (struct wseg *,BOOLEAN_P));
STATIC_DCL void FDECL(shrink_worm, (int));
STATIC_DCL void FDECL(random_dir, (XCHAR_P,XCHAR_P,xchar *,xchar *));
STATIC_DCL struct wseg *FDECL(create_worm_tail, (int));

/*  Description of long worm implementation.
 *
 *  Each monst struct of the head of a tailed worm has a wormno set to
 *			1 <= wormno < MAX_NUM_WORMS
 *  If wormno == 0 this does not mean that the monster is not a worm,
 *  it just means that the monster does not have a long worm tail.
 *
 *  The actual segments of a worm are not full blown monst structs.
 *  They are small wseg structs, and their position in the levels.monsters[][]
 *  array is held by the monst struct of the head of the worm.  This makes
 *  things like probing and hit point bookkeeping much easier.
 *
 *  The segments of the long worms on a level are kept as an array of
 *  singly threaded linked lists.  The wormno variable is used as an index
 *  for these segment arrays.
 *
 *  wtails:	The first (starting struct) of a linked list.  This points
 *		to the tail (last) segment of the worm.
 *
 *  wheads:	The last (end) of a linked list of segments.  This points to
 *		the segment that is at the same position as the real monster
 *		(the head).  Note that the segment that wheads[wormno] points
 *		to, is not displayed.  It is simply there to keep track of
 *		where the head came from, so that worm movement and display are
 *		simplified later.
 *		Keeping the head segment of the worm at the end of the list
 *		of tail segments is an endless source of confusion, but it is
 *		necessary.
 *		From now on, we will use "start" and "end" to refer to the
 *		linked list and "head" and "tail" to refer to the worm.
 *
 *  One final worm array is:
 *
 *  wgrowtime:	This tells us when to add another segment to the worm.
 *
 *  When a worm is moved, we add a new segment at the head, and delete the
 *  segment at the tail (unless we want it to grow).  This new head segment is
 *  located in the same square as the actual head of the worm.  If we want
 *  to grow the worm, we don't delete the tail segment, and we give the worm
 *  extra hit points, which possibly go into its maximum.
 *
 *  Non-moving worms (worm_nomove) are assumed to be surrounded by their own
 *  tail, and, thus, shrink instead of grow (as their tails keep going while
 *  their heads are stopped short).  In this case, we delete the last tail
 *  segment, and remove hit points from the worm.
 */

struct wseg *wheads[MAX_NUM_WORMS]   = DUMMY, *wtails[MAX_NUM_WORMS] = DUMMY;
long	    wgrowtime[MAX_NUM_WORMS] = DUMMY;

/*
 *  get_wormno()
 *
 *  Find an unused worm tail slot and return the index.  A zero means that
 *  there are no slots available.  This means that the worm head can exist,
 *  it just cannot ever grow a tail.
 *
 *  It, also, means that there is an optimisation to made.  The [0] positions
 *  of the arrays are never used.  Meaning, we really *could* have one more
 *  tailed worm on the level, or use a smaller array (using wormno - 1).
 *
 *  Implementation is left to the interested hacker.
 */
int
get_wormno()
{
    register int new_wormno = 1;

    while (new_wormno < MAX_NUM_WORMS) {
	if (!wheads[new_wormno])
	    return new_wormno; /* found an empty wtails[] slot at new_wormno */
	new_wormno++;
    }

    return(0);	/* level infested with worms */
}

/*
 *  initworm()
 *
 *  Use if (mon->wormno = get_wormno()) before calling this function!
 *
 *  Initialize the worm entry.  This will set up the worm grow time, and
 *  create and initialize the dummy segment for wheads[] and wtails[].
 *
 *  If the worm has no tail (ie get_wormno() fails) then this function need
 *  not be called.
 */
void
initworm(worm, wseg_count)
    struct monst *worm;
    int wseg_count;
{
    register struct wseg *seg, *new_tail = create_worm_tail(wseg_count);
    register int wnum = worm->wormno;

/*  if (!wnum) return;  bullet proofing */

    if (new_tail) {
	wtails[wnum] = new_tail;
	for (seg = new_tail; seg->nseg; seg = seg->nseg);
	wheads[wnum] = seg;
    } else {
	wtails[wnum] = wheads[wnum] = seg = newseg();
	seg->nseg    = (struct wseg *) 0;
	seg->wx      = worm->mx;
	seg->wy      = worm->my;
    }
    wgrowtime[wnum] = 0L;
}


/*
 *  toss_wsegs()
 *
 *  Get rid of all worm segments on and following the given pointer curr.
 *  The display may or may not need to be updated as we free the segments.
 */
STATIC_OVL
void
toss_wsegs(curr, display_update)
    register struct wseg *curr;
    register boolean display_update;
{
    register struct wseg *seg;

    while (curr) {
	seg = curr->nseg;

	/* remove from level.monsters[][] */

	/* need to check curr->wx for genocided while migrating_mon */
	if (curr->wx) {
	    remove_monster(curr->wx, curr->wy);

	    /* update screen before deallocation */
	    if (display_update) newsym(curr->wx,curr->wy);
	}

	/* free memory used by the segment */
	dealloc_seg(curr);
	curr = seg;
    }
}


/*
 *  shrink_worm()
 *
 *  Remove the tail segment of the worm (the starting segment of the list).
 */
STATIC_OVL
void
shrink_worm(wnum)
    int wnum;	/* worm number */
{
    struct wseg *seg;

    if (wtails[wnum] == wheads[wnum]) return;	/* no tail */

    seg = wtails[wnum];
    wtails[wnum] = seg->nseg;
    seg->nseg = (struct wseg *) 0;
    toss_wsegs(seg, TRUE);
}

/*
 *  worm_move()
 *
 *  Check for mon->wormno before calling this function!
 *
 *  Move the worm.  Maybe grow.
 */
void
worm_move(worm)
    struct monst *worm;
{
    register struct wseg *seg, *new_seg;	/* new segment */
    register int	 wnum = worm->wormno;	/* worm number */
	boolean hh = worm->mtyp == PM_HUNTING_HORROR;
	int w_length = count_wsegs(worm);
	


/*  if (!wnum) return;  bullet proofing */

    /*
     *  Figure if worm is shorter than two
     */
    seg = wtails[wnum];
    /*
     *  Place a segment at the old worm head.  The head has already moved.
     */
    seg = wheads[wnum];
    place_worm_seg(worm, seg->wx, seg->wy);
    newsym(seg->wx,seg->wy);		/* display the new segment */

    /*
     *  Create a new dummy segment head and place it at the end of the list.
     */
	
    new_seg       = newseg();
    new_seg->wx   = worm->mx;
    new_seg->wy   = worm->my;
    new_seg->nseg = (struct wseg *) 0;
    seg->nseg     = new_seg;		/* attach it to the end of the list */
    wheads[wnum]  = new_seg;		/* move the end pointer */


    if ((!hh && wgrowtime[wnum] <= moves)
		|| ( hh && w_length < 2)
	) {
		if (!wgrowtime[wnum])
			wgrowtime[wnum] = moves + rnd(5);
		else
			wgrowtime[wnum] += rn1(15, 3);

		if( worm->mtyp == PM_CHORISTER_JELLY){
			wgrowtime[wnum] += rn2(w_length) *  rn2(w_length);
		}

		worm->mhp += 3;
		if (worm->mhp > MHPMAX) worm->mhp = MHPMAX;
		if (worm->mhp > worm->mhpmax) worm->mhpmax = worm->mhp;
    } else
	/* The worm doesn't grow, so the last segment goes away. */
		shrink_worm(wnum);
}

/*
 *  worm_nomove()
 *
 *  Check for mon->wormno before calling this function!
 *
 *  The worm don't move so it should shrink.
 */
void
worm_nomove(worm)
    register struct monst *worm;
{
    shrink_worm((int) worm->wormno);	/* shrink */

    if (worm->mhp > 3)
	worm->mhp -= 3;		/* mhpmax not changed ! */
    else
	worm->mhp = 1;
}

/*
 *  wormgone()
 *
 *  Check for mon->wormno before calling this function!
 *
 *  Kill a worm tail.
 */
void
wormgone(worm)
    register struct monst *worm;
{
    register int wnum = worm->wormno;

/*  if (!wnum) return;  bullet proofing */

    worm->wormno = 0;

    /*  This will also remove the real monster (ie 'w') from the its
     *  position in level.monsters[][].
     */
    toss_wsegs(wtails[wnum], TRUE);

    wheads[wnum] = wtails[wnum] = (struct wseg *) 0;
}

/*
 *  wormhitu()
 *
 *  Check for mon->wormno before calling this function!
 *
 *  If the hero is near any part of the worm, the worm will try to attack.
 */
void
wormhitu(worm)
    register struct monst *worm;
{
    register int wnum = worm->wormno;
    register struct wseg *seg;
	struct attack mattk = {AT_CLAW, AD_PHYS, 2, 4};
	boolean dangerous_tail = (worm->mtyp == PM_CHORISTER_JELLY || worm->mtyp == PM_HUNTING_HORROR);

    if (!wnum) return; /* bullet proofing */
	
	if(worm->mtyp == PM_HUNTING_HORROR){
		mattk.damn = 5;
		mattk.damd = 6;
	}
	else if(worm->mtyp == PM_CHORISTER_JELLY){
		mattk.aatyp = AT_STNG;
		mattk.adtyp = AD_PAIN;
		mattk.damn = 3;
		mattk.damd = 6;
	}

/*  This does not work right now because mattacku() thinks that the head is
 *  out of range of the player.  We might try to kludge, and bring the head
 *  within range for a tiny moment, but this needs a bit more looking at
 *  before we decide to do this.
 */
	seg = wtails[wnum];
    for (seg = seg->nseg; seg && (dangerous_tail || seg->nseg); seg = seg->nseg)
		if (distu(seg->wx, seg->wy) < 3 && magr_can_attack_mdef(worm, &youmonst, u.ux, u.uy, FALSE)){
			if (xmeleehity(worm, &youmonst, &mattk, (struct obj **)0, -1, 0, FALSE) > 1)	// some result other than hit or miss
				break;
		}
}

/*  wormline()
 *
 *  Second-to-last segment of the worm is lined up with the given coordinates (lunging momentum should boost damage)
 */
boolean
wormline(worm, x, y)
    struct monst *worm;
	int x, y;
{
		int dtax = x - worm->mx, dtay = y - worm->my;
		int wnum = worm->wormno;
		struct wseg *seg, *nseg;

		for (seg = wtails[wnum], nseg = seg->nseg; nseg && nseg->nseg; seg = nseg, nseg = nseg->nseg)
			;
		if(seg == wtails[wnum])
			return FALSE;
		if (dtax == (worm->mx - seg->wx) && dtay == (worm->my - seg->wy))
			return TRUE;
		return FALSE;
}

/*  cutoff()
 *
 *  Remove the tail of a worm and adjust the hp of the worm.
 */
void
cutoff(worm, tail)
    struct monst *worm;
    struct wseg *tail;
{
  if (flags.mon_moving)
    pline("Part of the tail of %s is cut off.", mon_nam(worm));
  else
    You("cut part of the tail off of %s.", mon_nam(worm));
  toss_wsegs(tail, TRUE);
  if (worm->mhp > 1) worm->mhp /= 2;
}


/*  cutworm()
 *
 *  Check for mon->wormno before calling this function!
 *
 *  When hitting a worm (worm) at position x, y, with a weapon (weap),
 *  there is a chance that the worm will be cut in half, and a chance
 *  that both halves will survive.
 */
void
cutworm(worm, x, y, weap)
    struct monst *worm;
    xchar x,y;
    struct obj *weap;
{
    register struct wseg  *curr, *new_tail;
    register struct monst *new_worm;
    int wnum = worm->wormno;
    int cut_chance, new_wnum;

    if (!wnum) return; /* bullet proofing */

    if (x == worm->mx && y == worm->my) return;		/* hit on head */

    /* cutting goes best with a bladed weapon */
    cut_chance = rnd(20);	/* Normally  1-16 does not cut */
				/* Normally 17-20 does */

    if (weap && is_blade(weap))	/* With a blade 1- 6 does not cut */
	cut_chance += 10;	/*		7-20 does */

    if (cut_chance < 17) return;	/* not good enough */

    /* Find the segment that was attacked. */
    curr = wtails[wnum];

    while ( (curr->wx != x) || (curr->wy != y) ) {
	curr = curr->nseg;
	if (!curr) {
	    impossible("cutworm: no segment at (%d,%d)", (int) x, (int) y);
	    return;
	}
    }

    /* If this is the tail segment, then the worm just loses it. */
    if (curr == wtails[wnum]) {
	shrink_worm(wnum);
	return;
    }

    /*
     *  Split the worm.  The tail for the new worm is the old worm's tail.
     *  The tail for the old worm is the segment that follows "curr",
     *  and "curr" becomes the dummy segment under the new head.
     */
    new_tail = wtails[wnum];
    wtails[wnum] = curr->nseg;
    curr->nseg = (struct wseg *) 0;	/* split the worm */

    /*
     *  At this point, the old worm is correct.  Any new worm will have
     *  it's head at "curr" and its tail at "new_tail".
     */

    /* Sometimes the tail end dies. */
    if (worm->mtyp != PM_LONG_WORM ||
		rn2(3) || !(new_wnum = get_wormno())
	) {
		cutoff(worm, new_tail);
		return;
    }

    remove_monster(x, y);		/* clone_mon puts new head here */
    if (!(new_worm = clone_mon(worm, x, y))) {
      cutoff(worm, new_tail);
      return;
    }
    new_worm->wormno = new_wnum;	/* affix new worm number */

    /* Devalue the monster level of both halves of the worm. */
    worm->m_lev = max((unsigned)worm->m_lev - 2, 3);
    new_worm->m_lev = worm->m_lev;

    /* Calculate the mhp on the new_worm for the (lower) monster level. */
    new_worm->mhpmax = new_worm->mhp = d((int)new_worm->m_lev, 8);

    /* Calculate the mhp on the old worm for the (lower) monster level. */
    if (worm->m_lev > 3) {
	worm->mhpmax = d((int)worm->m_lev, 8);
	if (worm->mhpmax < worm->mhp) worm->mhp = worm->mhpmax;
    }

    wtails[new_wnum] = new_tail;	/* We've got all the info right now */
    wheads[new_wnum] = curr;		/* so we can do this faster than    */
    wgrowtime[new_wnum] = 0L;		/* trying to call initworm().       */

    /* Place the new monster at all the segment locations. */
    place_wsegs(new_worm);

    if (flags.mon_moving)
	pline("%s is cut in half.", Monnam(worm));
    else
	You("cut %s in half.", mon_nam(worm));
}


/*
 *  see_wsegs()
 *
 *  Refresh all of the segments of the given worm.  This is only called
 *  from see_monster() in display.c or when a monster goes minvis.  It
 *  is located here for modularity.
 */
void
see_wsegs(worm)
    struct monst *worm;
{
    struct wseg *curr = wtails[worm->wormno];

/*  if (!mtmp->wormno) return;  bullet proofing */

    while (curr != wheads[worm->wormno]) {
	newsym(curr->wx,curr->wy);
	curr = curr->nseg;
    }
}

/*
 *  detect_wsegs()
 *
 *  Display all of the segments of the given worm for detection.
 */
void
detect_wsegs(worm, use_detection_glyph)
    struct monst *worm;
    boolean use_detection_glyph;
{
    int num;
    struct wseg *curr = wtails[worm->wormno];

/*  if (!mtmp->wormno) return;  bullet proofing */

    while (curr != wheads[worm->wormno]) {
	if(worm->mtyp == PM_HUNTING_HORROR) num = use_detection_glyph ?
		detected_monnum_to_glyph(what_mon(PM_HUNTING_HORROR_TAIL, worm)) :
		monnum_to_glyph(what_mon(PM_HUNTING_HORROR_TAIL, worm));
	else if(worm->mtyp == PM_CHORISTER_JELLY) num = use_detection_glyph ?
		detected_monnum_to_glyph(what_mon(PM_CHORISTER_TRAIN, worm)) :
		monnum_to_glyph(what_mon(PM_CHORISTER_TRAIN, worm));
	else num = use_detection_glyph ?
		detected_monnum_to_glyph(what_mon(PM_LONG_WORM_TAIL, worm)) :
		monnum_to_glyph(what_mon(PM_LONG_WORM_TAIL, worm));
	show_glyph(curr->wx,curr->wy,num);
	curr = curr->nseg;
    }
}


/*
 *  save_worm()
 *
 *  Save the worm information for later use.  The count is the number
 *  of segments, including the dummy.  Called from save.c.
 */
void
save_worm(fd, mode)
    int fd, mode;
{
    int i;
    int count;
    struct wseg *curr, *temp;

    if (perform_bwrite(mode)) {
	for (i = 1; i < MAX_NUM_WORMS; i++) {
	    for (count = 0, curr = wtails[i]; curr; curr = curr->nseg) count++;
	    /* Save number of segments */
	    bwrite(fd, (genericptr_t) &count, sizeof(int));
	    /* Save segment locations of the monster. */
	    if (count) {
		for (curr = wtails[i]; curr; curr = curr->nseg) {
		    bwrite(fd, (genericptr_t) &(curr->wx), sizeof(xchar));
		    bwrite(fd, (genericptr_t) &(curr->wy), sizeof(xchar));
		}
	    }
	}
	bwrite(fd, (genericptr_t) wgrowtime, sizeof(wgrowtime));
    }

    if (release_data(mode)) {
	/* Free the segments only.  savemonchn() will take care of the
	 * monsters. */
	for (i = 1; i < MAX_NUM_WORMS; i++) {
	    if (!(curr = wtails[i])) continue;

	    while (curr) {
		temp = curr->nseg;
		dealloc_seg(curr);		/* free the segment */
		curr = temp;
	    }
	    wheads[i] = wtails[i] = (struct wseg *) 0;
	}
    }

}

/*
 *  rest_worm()
 *
 *  Restore the worm information from the save file.  Called from restore.c
 */
void
rest_worm(fd)
    int fd;
{
    int i, j, count;
    struct wseg *curr, *temp;

    for (i = 1; i < MAX_NUM_WORMS; i++) {
	mread(fd, (genericptr_t) &count, sizeof(int));
	if (!count) continue;	/* none */

	/* Get the segments. */
	for (curr = (struct wseg *) 0, j = 0; j < count; j++) {
	    temp = newseg();
	    temp->nseg = (struct wseg *) 0;
	    mread(fd, (genericptr_t) &(temp->wx), sizeof(xchar));
	    mread(fd, (genericptr_t) &(temp->wy), sizeof(xchar));
	    if (curr)
		curr->nseg = temp;
	    else
		wtails[i] = temp;
	    curr = temp;
	}
	wheads[i] = curr;
    }
    mread(fd, (genericptr_t) wgrowtime, sizeof(wgrowtime));
}

/*
 *  place_wsegs()
 *
 *  Place the segments of the given worm.  Called from restore.c
 */
void
place_wsegs(worm)
    struct monst *worm;
{
    struct wseg *curr = wtails[worm->wormno];

/*  if (!mtmp->wormno) return;  bullet proofing */

    while (curr != wheads[worm->wormno]) {
	place_worm_seg(worm,curr->wx,curr->wy);
	curr = curr->nseg;
    }
}

/*
 *  remove_worm()
 *
 *  This function is equivalent to the remove_monster #define in
 *  rm.h, only it will take the worm *and* tail out of the levels array.
 *  It does not get rid of (dealloc) the worm tail structures, and it does
 *  not remove the mon from the fmon chain.
 */
void
remove_worm(worm)
    register struct monst *worm;
{
    register struct wseg *curr = wtails[worm->wormno];

/*  if (!mtmp->wormno) return;  bullet proofing */

    while (curr) {
	remove_monster(curr->wx, curr->wy);
	newsym(curr->wx, curr->wy);
	curr = curr->nseg;
    }
}

/*
 *  place_worm_tail_randomly()
 *
 *  Place a worm tail somewhere on a level behind the head.
 *  This routine essentially reverses the order of the wsegs from head
 *  to tail while placing them.
 *  x, and y are most likely the worm->mx, and worm->my, but don't *need* to
 *  be, if somehow the head is disjoint from the tail.
 */
void
place_worm_tail_randomly(worm, x, y)
    struct monst *worm;
    xchar x, y;
{
    int wnum = worm->wormno;
    struct wseg *curr = wtails[wnum];
    struct wseg *new_tail;
    register xchar ox = x, oy = y;

/*  if (!wnum) return;  bullet proofing */

    if (wnum && (!wtails[wnum] || !wheads[wnum]) ) {
	impossible("place_worm_tail_randomly: wormno is set without a tail!");
	return;
    }

    wheads[wnum] = new_tail = curr;
    curr = curr->nseg;
    new_tail->nseg = (struct wseg *) 0;
    new_tail->wx = x;
    new_tail->wy = y;

    while(curr)  {
	xchar nx, ny;
	char tryct = 0;

	/* pick a random direction from x, y and search for goodpos() */

	do {
	    random_dir(ox, oy, &nx, &ny);
	} while (!goodpos(nx, ny, worm, 0) && (tryct++ < 50));

	if (tryct < 50)  {
	    place_worm_seg(worm, nx, ny);
	    curr->wx = ox = nx;
	    curr->wy = oy = ny;
	    wtails[wnum] = curr;
	    curr = curr->nseg;
	    wtails[wnum]->nseg = new_tail;
	    new_tail = wtails[wnum];
	    newsym(nx, ny);
	} else {			/* Oops.  Truncate because there was */
	    toss_wsegs(curr, FALSE);    /* no place for the rest of it */
	    curr = (struct wseg *) 0;
	}
    }
}

/*
 * Given a coordinate x, y.
 * return in *nx, *ny, the coordinates of one of the <= 8 squares adjoining.
 *
 * This function, and the loop it serves, could be eliminated by coding
 * enexto() with a search radius.
 */
STATIC_OVL
void
random_dir(x, y, nx, ny)
    register xchar   x,   y;
    register xchar *nx, *ny;
{
    *nx = x;
    *ny = y;

    *nx += (x > 1 ?			/* extreme left ? */
		(x < COLNO ?		 /* extreme right ? */
			(rn2(3) - 1)	  /* neither so +1, 0, or -1 */
		:	-rn2(2))	 /* 0, or -1 */
	   :	rn2(2));		/* 0, or 1 */

    *ny += (*nx == x ?			/* same kind of thing with y */
		(y > 1 ?
		    (y < ROWNO ?
			(rn2(2) ?
			    1
			:   -1)
		    :	-1)
		:   1)
	    :	(y > 1 ?
		    (y < ROWNO ?
			(rn2(3) - 1)
		    :	-rn2(2))
		:   rn2(2)));
}

/*  count_wsegs()
 *
 *  returns
 *  the number of visible segments that a worm has.
 */

int
count_wsegs(mtmp)
    struct monst *mtmp;
{
    register int i=0;
    register struct wseg *curr = (wtails[mtmp->wormno])->nseg;

/*  if (!mtmp->wormno) return 0;  bullet proofing */

    while (curr) {
	i++;
	curr = curr->nseg;
    }

    return i;
}

/*  create_worm_tail()
 *
 *  will create a worm tail chain of (num_segs + 1) and return a pointer to it.
 */
STATIC_OVL
struct wseg *
create_worm_tail(num_segs)
    int num_segs;
{
    register int i=0;
    register struct wseg *new_tail, *curr;

    if (!num_segs) return (struct wseg *)0;

    new_tail = curr = newseg();
    curr->nseg = (struct wseg *)0;
    curr->wx = 0;
    curr->wy = 0;

    while (i < num_segs) {
	curr->nseg = newseg();
	curr = curr->nseg;
	curr->nseg = (struct wseg *)0;
	curr->wx = 0;
	curr->wy = 0;
	i++;
    }

    return (new_tail);
}

/*  worm_known()
 *
 *  Is any segment of this worm in viewing range?  Note: caller must check
 *  invisibility and telepathy (which should only show the head anyway).
 *  Mostly used in the canseemon() macro.
 */
boolean
worm_known(worm)
struct monst *worm;
{
    struct wseg *curr = wtails[worm->wormno];

    while (curr) {
	if(cansee(curr->wx,curr->wy)) return TRUE;
	curr = curr->nseg;
    }
    return FALSE;
}

/*worm.c*/
