/*	SCCS Id: @(#)mkobj.c	3.4	2002/10/07	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "prop.h"
#include "artifact.h"

STATIC_DCL void FDECL(mkbox_cnts,(struct obj *));
STATIC_DCL void FDECL(obj_timer_checks,(struct obj *, XCHAR_P, XCHAR_P, int));
STATIC_DCL void FDECL(handle_material_specials, (struct obj *, int, int));
STATIC_DCL void FDECL(init_obj_material, (struct obj *));
#ifdef OVL1
#ifdef WIZARD
STATIC_DCL const char *FDECL(where_name, (int));
STATIC_DCL void FDECL(check_contained, (struct obj *,const char *));
STATIC_DCL int FDECL(maid_clean, (struct monst *, struct obj *));
#endif
#endif /* OVL1 */

//extern struct obj *thrownobj;		/* defined in dothrow.c */

/*#define DEBUG_EFFECTS*/	/* show some messages for debugging */

#define quest_equipment(otmp) ((In_quest(&u.uz) && in_mklev							\
						&& !Role_if(PM_CONVICT)										\
						&& !(Role_if(PM_NOBLEMAN) && Race_if(PM_HALF_DRAGON))		\
						) && (														\
						otmp->oclass == WEAPON_CLASS								\
						|| (otmp->oclass == TOOL_CLASS && is_weptool(otmp))			\
						|| (otmp->oclass == TOOL_CLASS && (otmp->otyp == LENSES || otmp->otyp == SUNGLASSES))	\
						|| (otmp->oclass == ARMOR_CLASS && !Is_dragon_scales(otmp))	\
						))

struct icp {
    int  iprob;		/* probability of an item type */
    char iclass;	/* item class */
};

#ifdef OVL1

const struct icp mkobjprobs[] = {
{10, WEAPON_CLASS},
{10, ARMOR_CLASS},
{18, FOOD_CLASS},
{ 2, BELT_CLASS},
{ 8, TOOL_CLASS},
{ 8, GEM_CLASS},
{16, POTION_CLASS},
{16, SCROLL_CLASS},
{ 4, SPBOOK_CLASS},
{ 4, WAND_CLASS},
{ 3, RING_CLASS},
{ 1, AMULET_CLASS}
};

const struct icp boxiprobs[] = {
{18, GEM_CLASS},
{15, FOOD_CLASS},
{18, POTION_CLASS},
{18, SCROLL_CLASS},
{12, SPBOOK_CLASS},
{ 7, COIN_CLASS},
{ 6, WAND_CLASS},
{ 5, RING_CLASS},
{ 1, AMULET_CLASS}
};

#ifdef REINCARNATION
const struct icp rogueprobs[] = {
{12, WEAPON_CLASS},
{12, ARMOR_CLASS},
{22, FOOD_CLASS},
{22, POTION_CLASS},
{22, SCROLL_CLASS},
{ 5, WAND_CLASS},
{ 5, RING_CLASS}
};
#endif

const struct icp hellprobs[] = {
{20, WEAPON_CLASS},
{20, ARMOR_CLASS},
{ 8, FOOD_CLASS},
{ 6, BELT_CLASS},
{12, TOOL_CLASS},
{12, GEM_CLASS},
{ 1, POTION_CLASS},
{ 1, SCROLL_CLASS},
{ 8, WAND_CLASS},
{ 8, RING_CLASS},
{ 4, AMULET_CLASS}
};

/* Object material probabilities.*/
/* for objects which are normally iron or metal */
static const struct icp metal_materials[] = {
	{975, 0 }, /* default to base type, iron or metal */
	{  5, IRON },
	{  5, WOOD },
	{  5, SILVER },
	{  3, COPPER },
	{  3, MITHRIL },
	{  1, GOLD },
	{  1, BONE },
	{  1, GLASS },
	{  1, PLATINUM },
	{  0, 0 }
};

/* for objects which are normally wooden */
static const struct icp wood_materials[] = {
	{980, WOOD },
	{ 10, MINERAL },
	{  5, IRON },
	{  3, BONE },
	{  1, COPPER },
	{  1, SILVER },
	{  0, 0 }
};

/* for objects which are normally cloth */
static const struct icp cloth_materials[] = {
    {800, CLOTH },
    {199, LEATHER },
	{  1, DRAGON_HIDE },
	{  0, 0 }
};

/* for objects which are normally leather */
static const struct icp leather_materials[] = {
    {750, LEATHER },
    {240, CLOTH },
	{ 10, DRAGON_HIDE },
	{  0, 0 }
};

/* for objects of dwarvish make */
static const struct icp dwarvish_materials[] = {
	{800, IRON },
	{100, MITHRIL },
	{ 50, COPPER },
	{ 20, SILVER },
	{ 15, GOLD },
	{ 10, PLATINUM },
	{  5, GEMSTONE },
	{  0, 0 }
};

/* for objects of high-elven make (which is currently specified as elven equipment normally made of mithril) */
static const struct icp high_elven_materials[] = {
    {900, MITHRIL },
    { 60, SILVER },
    { 30, GOLD },
	{ 10, GEMSTONE },
	{  0, 0 }
};
/* for armor-y objects of elven make - no iron!
* Does not cover clothy items; those use the regular cloth probs. */
static const struct icp elven_materials[] = {
	{900, 0 }, /* use base material */
	{ 50, MITHRIL },
	{ 25, SILVER },
	{ 15, COPPER },
	{  5, BONE },
	{  5, GOLD },
	{  0, 0 }
};

/* for eilistran armor */
static const struct icp eli_materials[] = {
	{600, SILVER },
	{300, PLATINUM },
	{ 50, MITHRIL },
	{ 25, GLASS },
	{ 13, GEMSTONE },
	{ 12, MINERAL },
	{  0, 0 }
};

static const struct icp lens_materials[] = {
	{ 1000, GEMSTONE },
	{  0, 0 }
};

/* for weapons of droven make -- armor is all shadowsteel */
static const struct icp droven_materials[] = {
	{900, 0 }, /* use base material */
	{ 45, MITHRIL },
	{ 45, SILVER },
	{ 10, SHADOWSTEEL }, /* shadowsteel was intended to be quite bad for weapons, at least given the techniques the drow know */
	{  0, 0 }
};

/* for objects of orcish make */
static const struct icp orcish_materials[] = {
	{549, 0 }, /* use base material */
	{250, IRON },
	{160, BONE },
	{ 20, LEAD },
	{ 15, GOLD },
	{  5, OBSIDIAN_MT },
	{  1, GREEN_STEEL },
	{  0, 0 }
};

/* Reflectable items - for the shield of reflection; anything that can hold a
 * polish. Amulets also arbitrarily use this list. */
static const struct icp shiny_materials[] = {
	{950, 0 }, /* use base material */
	{ 15, SILVER },
	{ 11, COPPER },
	{  6, GOLD },
	{  6, IRON }, /* stainless steel */
	{  5, GLASS },
	{  3, MITHRIL },
	{  3, METAL }, /* aluminum, or similar */
	{  1, PLATINUM },
	{  0, 0 }
};

/* for bells and other tools, especially instruments, which are normally copper
 * or metal.  Wood and glass in other lists precludes us from using those. */
static const struct icp resonant_materials[] = {
	{700, 0 }, /* use base material */
	{100, COPPER },
	{ 60, SILVER },
	{ 50, IRON },
	{ 50, MITHRIL },
	{ 30, GOLD },
	{ 10, PLATINUM },
	{  0, 0 }
};

/* for horns, currently. */
static const struct icp horn_materials[] = {
	{700, BONE },
	{100, COPPER },
	{ 70, MITHRIL },
	{ 50, WOOD },
	{ 50, SILVER },
	{ 20, GOLD },
	{ 10, DRAGON_HIDE },
	{  0, 0 }
};

/* hacks for specific objects... not great because it's a lot of data, but it's
 * a relatively clean solution */
static const struct icp bow_materials[] = {
    /* assumes all bows will be wood by default, fairly safe assumption */
    {975, WOOD },
    {  7, IRON },
    {  5, MITHRIL },
    {  4, COPPER },
    {  4, BONE },
    {  2, SILVER },
	{  2, METAL },
    {  1, GOLD },
	{  0, 0 }
};

static const struct icp cane_materials[] = {
	{550, 0 }, /* use base material */
	{250, WOOD },
	{ 90, COPPER },
	{ 50, SILVER },
	{ 25, BONE },
	{ 20, MITHRIL },
	{ 10, GOLD },
	{  5, DRAGON_HIDE },
	{  0, 0 }
};

static const struct icp chikage_materials[] = {
	{550, 0 }, /* use base material */
	{250, GOLD },
	{200, GREEN_STEEL },
	{  0, 0 }
};

static const struct icp brick_materials[] = {
	{550, 0 }, /* use base material */
	{150, WOOD },
	{100, IRON },//800
	{ 50, LEAD },
	{ 30, SILVER },
	{ 25, PLATINUM },//905
	{ 25, MITHRIL },//930
	{ 25, WAX },//955
	{ 18, GLASS },//973
	{ 17, GEMSTONE },//990
	{ 10, OBSIDIAN_MT },//1000
	{  0, 0 }
};

static const struct icp special_materials[] = {
	{250, 0 }, /* use base material */
	{250, SILVER },
	{150, GOLD },
	{100, IRON },
	{ 75, MITHRIL },
	{ 75, PLATINUM },
	{ 50, GREEN_STEEL },
	{ 50, DRAGON_HIDE },
	{  0, 0 }
};

static const struct icp fiend_materials[] = {
	{350, 0 }, /* use base material */
	{250, GOLD },
	{150, GREEN_STEEL },
	{ 75, LEAD },
	{ 75, PLATINUM },
	{ 50, MITHRIL },
	{ 50, DRAGON_HIDE },
	{  0, 0 }
};

struct obj *
mkobj_at(let, x, y, mkflags)
char let;
int x, y;
int mkflags;
{
	struct obj *otmp;

	otmp = mkobj(let, mkflags);
	
	place_object(otmp, x, y);
	
	return(otmp);
}

struct obj *
mksobj_at(otyp, x, y, mkflags)
int otyp, x, y;
int mkflags;
{
	struct obj *otmp;

	otmp = mksobj(otyp, mkflags);
	
	place_object(otmp, x, y);
	
	return(otmp);
}

struct obj *
mkobj(oclass, mkflags)
char oclass;
int mkflags;
{
	int tprob, i, prob = rnd(1000);

	if(oclass == RANDOM_CLASS) {
		const struct icp *iprobs =
#ifdef REINCARNATION
				    (Is_rogue_level(&u.uz)) ?
				    (const struct icp *)rogueprobs :
#endif
				    Inhell ? (const struct icp *)hellprobs :
				    (const struct icp *)mkobjprobs;

		for(tprob = rnd(100);
		    (tprob -= iprobs->iprob) > 0;
		    iprobs++);
		oclass = iprobs->iclass;
	}

	i = bases[(int)oclass];
	while((prob -= objects[i].oc_prob) > 0) i++;

	if(objects[i].oc_class != oclass || !OBJ_NAME(objects[i]))
		panic("probtype error, oclass=%d i=%d", (int) oclass, i);

	return(mksobj(i, mkflags));
}

STATIC_OVL void
mkbox_cnts(box)
struct obj *box;
{
	register int n;
	register int min = 0;
	register struct obj *otmp;

	box->cobj = (struct obj *) 0;

	switch (box->otyp) {
	case ICE_BOX:		n = 20; break;
	case CHEST:		n = 5; break;
	case BOX:		n = 3; break;
	case WRITING_DESK:	n = 3; break;
	case MASSIVE_STONE_CRATE: min=1;
	case SACK:
	case OILSKIN_SACK:
				/* initial inventory: sack starts out empty */
				if (moves <= 1 && !in_mklev) { n = 0; break; }
				/*else FALLTHRU*/
	case BAG_OF_HOLDING:	n = 1; break;
	default:		n = 0; break;
	}

	for (n = max_ints(rn2(n+1),min); n > 0; n--) {
	    if (box->otyp == ICE_BOX) {
			if (!(otmp = mksobj(CORPSE, MKOBJ_ARTIF))) continue;
			/* Note: setting age to 0 is correct.  Age has a different
			 * from usual meaning for objects stored in ice boxes. -KAA
			 */
			otmp->age = 0L;
			stop_corpse_timers(otmp);
	    } else if(box->otyp == MASSIVE_STONE_CRATE){
			if (!(otmp = mkobj(FOOD_CLASS, TRUE))) continue;
	    } else if(box->otyp == WRITING_DESK){
			if(rn2(4)){
				if (!(otmp = mkobj(SCROLL_CLASS, TRUE))) continue;
			}
			else if(rn2(4)){
				if (!(otmp = mksobj(SCR_BLANK_PAPER, MKOBJ_NOINIT))) continue;
			}
			else {
				if (!(otmp = mkobj(SPBOOK_CLASS, TRUE))) continue;
			}
	    } else {
		register int tprob;
		const struct icp *iprobs = boxiprobs;

		for (tprob = rnd(100); (tprob -= iprobs->iprob) > 0; iprobs++)
		    ;
		if (!(otmp = mkobj(iprobs->iclass, TRUE))) continue;

		/* handle a couple of special cases */
		if (otmp->oclass == COIN_CLASS) {
		    /* 2.5 x level's usual amount; weight adjusted below */
		    otmp->quan = (long)(rnd(level_difficulty()+2) * rnd(75));
			if(uring_art(ART_RING_OF_THROR))
				otmp->quan = 2*otmp->quan + d(2,2)-3;
			if(otmp->quan < 1)
				otmp->quan = 1;
		    otmp->owt = weight(otmp);
		} else while (otmp->otyp == ROCK) {
		    otmp->otyp = rnd_class(DILITHIUM_CRYSTAL, LOADSTONE);
			set_material_gm(otmp, objects[otmp->otyp].oc_material);
		    if (otmp->quan > 2L) otmp->quan = 1L;
		    otmp->owt = weight(otmp);
		}
		if (box->otyp == BAG_OF_HOLDING) {
		    if (Is_mbag(otmp)) {
			otmp->otyp = SACK;
			otmp->spe = 0;
			otmp->owt = weight(otmp);
		    } else while (otmp->otyp == WAN_CANCELLATION)
			    otmp->otyp = rnd_class(WAN_LIGHT, WAN_LIGHTNING);
		}
	    }
	    (void) add_to_container(box, otmp);
	}
}

int
rndmonnum()	/* select a random, common monster type */
{
	register struct permonst *ptr;
	register int	i;

	/* Plan A: get a level-appropriate common monster */
	ptr = rndmonst(0, 0);
	if (ptr) return(monsndx(ptr));

	/* Plan B: get any common monster */
	do {
	    i = rn1(SPECIAL_PM - LOW_PM, LOW_PM);
	    ptr = &mons[i];
	} while((ptr->geno & G_NOGEN) || (!Inhell && (ptr->geno & G_HELL)));

	return(i);
}

/*
 * Split obj so that it gets size gets reduced by num. The quantity num is
 * put in the object structure delivered by this call.  The returned object
 * has its wornmask cleared and is positioned just following the original
 * in the nobj chain (and nexthere chain when on the floor).
 */
struct obj *
splitobj(obj, num)
struct obj *obj;
long num;
{
	struct obj *otmp;

	if (obj->cobj || num <= 0L || obj->quan <= num)
	    panic("splitobj");	/* can't split containers */
	otmp = newobj(0);
	*otmp = *obj;		/* copies whole structure */
	/* invalidate pointers */
	otmp->light = (struct ls_t *)0;
	otmp->timed = (struct timer *)0;
	otmp->oextra_p = (union oextra *)0;
	otmp->mp = (struct mask_properties *)0;	/* not sure if correct -- these are very unfinished */

	otmp->o_id = flags.ident++;
	if (!otmp->o_id) otmp->o_id = flags.ident++;	/* ident overflowed */
	otmp->lamplit = 0;	/* not lit, yet */
	otmp->owornmask = 0L;	/* new object isn't worn */
	obj->quan -= num;
	obj->owt = weight(obj);
	otmp->quan = num;
	otmp->owt = weight(otmp);	/* -= obj->owt ? */
	obj->nobj = otmp;
	/* Only set nexthere when on the floor, nexthere is also used */
	/* as a back pointer to the container object when contained. */
	if (obj->where == OBJ_FLOOR)
	    obj->nexthere = otmp;

	register int ox_id;
	for (ox_id=0; ox_id<NUM_OX; ox_id++)
		cpy_ox(obj, otmp, ox_id);
		
	if (obj->unpaid) splitbill(obj,otmp);
	if (obj->timed) copy_timers(obj->timed, TIMER_OBJECT, (genericptr_t)otmp);
	if (obj_sheds_light(obj)) obj_split_light_source(obj, otmp);
	return otmp;
}

/*
 * Duplicate obj. The returned object
 * has its wornmask cleared and is positioned just following the original
 * in the nobj chain (and nexthere chain when on the floor).
 */
struct obj *
duplicate_obj(struct obj *obj, boolean same_chain)
{
	struct obj *otmp;

	otmp = newobj(0);
	*otmp = *obj;		/* copies whole structure */
	/* invalidate pointers */
	otmp->light = (struct ls_t *)0;
	otmp->timed = (struct timer *)0;
	otmp->oextra_p = (union oextra *)0;
	otmp->mp = (struct mask_properties *)0;	/* not sure if correct -- these are very unfinished */
	otmp->cobj = (struct obj *)0;

	if (obj->cobj){
		struct obj *cntdup;
		struct obj **curcobj = &(otmp->cobj);
		for(struct obj *cntobj = obj->cobj; cntobj; cntobj = cntobj->nobj){
			cntdup = duplicate_obj(cntobj, TRUE);
			if(cntdup){
				obj_extract_self(cntdup);
				//Note: don't use the normal add to container function, or it will reverse the order of cobj
				cntdup->where = OBJ_CONTAINED;
				cntdup->ocontainer = otmp;
				*curcobj = cntdup;
				curcobj = &(cntdup->nobj);
			}
		}
		
	}
	otmp->o_id = flags.ident++;
	if (!otmp->o_id) otmp->o_id = flags.ident++;	/* ident overflowed */
	otmp->lamplit = 0;	/* not lit, yet */
	otmp->owornmask = 0L;	/* new object isn't worn */
	if(same_chain){
		obj->nobj = otmp;
		/* Only set nexthere when on the floor, nexthere is also used */
		/* as a back pointer to the container object when contained. */
		if (obj->where == OBJ_FLOOR)
			obj->nexthere = otmp;
	}
	else {
		otmp->where = OBJ_FREE;
		otmp->nobj = 0;
		otmp->nexthere = 0;
	}

	register int ox_id;
	for (ox_id=0; ox_id<NUM_OX; ox_id++)
		cpy_ox(obj, otmp, ox_id);
	
	otmp->unpaid = 0;
	if (obj->timed) copy_timers(obj->timed, TIMER_OBJECT, (genericptr_t)otmp);
	if (obj_sheds_light(obj)) obj_split_light_source(obj, otmp);
	return otmp;
}

/*
 * Insert otmp right after obj in whatever chain(s) it is on.  Then extract
 * obj from the chain(s).  This function does a literal swap.  It is up to
 * the caller to provide a valid context for the swap.  When done, obj will
 * still exist, but not on any chain.
 *
 * Note:  Don't use use obj_extract_self() -- we are doing an in-place swap,
 * not actually moving something.
 */
void
replace_object(obj, otmp)
struct obj *obj;
struct obj *otmp;
{
    otmp->where = obj->where;
    switch (obj->where) {
    case OBJ_FREE:
	/* do nothing */
	break;
    case OBJ_INVENT:
	otmp->nobj = obj->nobj;
	obj->nobj = otmp;
	extract_nobj(obj, &invent);
	break;
    case OBJ_CONTAINED:
	otmp->nobj = obj->nobj;
	otmp->ocontainer = obj->ocontainer;
	obj->nobj = otmp;
	extract_nobj(obj, &obj->ocontainer->cobj);
	break;
	case OBJ_MAGIC_CHEST:
	otmp->nobj = obj->nobj;
	obj->nobj = otmp;
	extract_magic_chest_nobj(obj);
	break;
    case OBJ_MINVENT:
	otmp->nobj = obj->nobj;
	otmp->ocarry =  obj->ocarry;
	obj->nobj = otmp;
	extract_nobj(obj, &obj->ocarry->minvent);
	break;
    case OBJ_FLOOR:
	otmp->nobj = obj->nobj;
	otmp->nexthere = obj->nexthere;
	otmp->ox = obj->ox;
	otmp->oy = obj->oy;
	obj->nobj = otmp;
	obj->nexthere = otmp;
	extract_nobj(obj, &fobj);
	extract_nexthere(obj, &level.objects[obj->ox][obj->oy]);
	break;
    default:
	panic("replace_object: obj position");
	break;
    }
}

/*
 * Create a dummy duplicate to put on shop bill.  The duplicate exists
 * only in the billobjs chain.  This function is used when a shop object
 * is being altered, and a copy of the original is needed for billing
 * purposes.  For example, when eating, where an interruption will yield
 * an object which is different from what it started out as; the "I x"
 * command needs to display the original object.
 *
 * The caller is responsible for checking otmp->unpaid and
 * costly_spot(u.ux, u.uy).  This function will make otmp no charge.
 *
 * Note that check_unpaid_usage() should be used instead for partial
 * usage of an object.
 */
void
bill_dummy_object(otmp)
register struct obj *otmp;
{
	register struct obj *dummy;

	if (otmp->unpaid)
	    subfrombill(otmp, shop_keeper(*u.ushops));
	dummy = newobj(0);
	*dummy = *otmp;
	dummy->oextra_p = NULL;
	dummy->light = NULL;
	dummy->timed = NULL;
	dummy->mp = NULL;
	dummy->where = OBJ_FREE;
	dummy->o_id = flags.ident++;
	if (!dummy->o_id) dummy->o_id = flags.ident++;	/* ident overflowed */
	register int ox_id;
	for (ox_id=0; ox_id<NUM_OX; ox_id++)
		cpy_ox(otmp, dummy, ox_id);
	if (Is_candle(dummy)) dummy->lamplit = 0;
	addtobill(dummy, FALSE, TRUE, TRUE);
	otmp->no_charge = 1;
	otmp->unpaid = 0;
	return;
}

#endif /* OVL1 */
#ifdef OVLB

struct obj *
mksobj(otyp, mkflags)
int otyp;
int mkflags;
{
	int mndx, tryct;
	struct obj *otmp;
	char let = objects[otyp].oc_class;

	boolean init =   ((mkflags & MKOBJ_NOINIT) == 0);
	boolean artif =  ((mkflags & MKOBJ_ARTIF) != 0);
	boolean summon = ((mkflags & MKOBJ_SUMMON) != 0) && otyp != GOLD_PIECE;

	otmp = newobj(0);
	*otmp = zeroobj;
	otmp->age = monstermoves;
	otmp->o_id = flags.ident++;
	if (!otmp->o_id) otmp->o_id = flags.ident++;	/* ident overflowed */
	otmp->quan = 1L;
	otmp->oclass = let;
	otmp->otyp = otyp;
	otmp->where = OBJ_FREE;
	otmp->dknown = 0;
	otmp->corpsenm = 0; /* BUGFIX: Where does this get set? shouldn't it be given a default during initialization? */
	otmp->objsize = MZ_MEDIUM;
	otmp->bodytypeflag = 0;
	otmp->ovar1 = 0;
	otmp->oward = 0;
	for(int i = 0; i < OPROP_LISTSIZE; i++)
		otmp->oproperties[i] = 0;
	otmp->gifted = GOD_NONE;
	otmp->lifted = 0;
	otmp->shopOwned = 0;
	otmp->sknown = 0;
	otmp->ostolen = 0;
	otmp->lightened = 0;
	otmp->obroken = 0; /* BUGFIX: shouldn't this be set to 0 initially? */
	otmp->opoisoned = 0;
	otmp->fromsink = 0;
	otmp->mp = (struct mask_properties *) 0;
	
	init_obj_material(otmp);
	if(otmp->otyp == CANE){
		int mat_primary = otmp->obj_material;
		otmp->otyp = WHIP_SAW;
		init_obj_material(otmp);

		otmp->otyp = CANE;
		otmp->ovar1_alt_mat = otmp->obj_material;
		set_material_gm(otmp, mat_primary);
	}
	else if(otmp->otyp == WHIP_SAW){
		int mat_primary = otmp->obj_material;
		otmp->otyp = CANE;
		init_obj_material(otmp);

		otmp->otyp = WHIP_SAW;
		otmp->ovar1_alt_mat = otmp->obj_material;
		set_material_gm(otmp, mat_primary);
	}
	else if(otmp->otyp == TOOTH){
		otmp->ovar1_tooth_type = rnd(MAX_TOOTH);
	}
	
	set_object_color(otmp);
	
	set_obj_shape(otmp, MB_HUMANOID);
	
	if(otyp == VIPERWHIP) otmp->ovar1_heads = rn2(2) ? 1 : rn2(5) ? rnd(2) : rnd(5);
	
	if (summon) {
		/* set up otmp as summoned indefinitely
		   caller is responsible for 1. giving to a monster and 2. setting duration and removing permanence if desired */
		add_ox(otmp, OX_ESUM);
		otmp->oextra_p->esum_p->summoner = (struct monst *)0;
		otmp->oextra_p->esum_p->sm_id = 0;
		otmp->oextra_p->esum_p->sm_o_id = 0;
		otmp->oextra_p->esum_p->summonstr = 0;
		otmp->oextra_p->esum_p->staleptr = 0;
		otmp->oextra_p->esum_p->permanent = 1;
		otmp->oextra_p->esum_p->sticky = 1; /* mark as unfinished -- add_to_minv will detect this and attach it automatically */
		start_timer(ESUMMON_PERMANENT, TIMER_OBJECT, DESUMMON_OBJ, (genericptr_t)otmp);
	}

	fix_object(otmp);
	
	if (!objects[otyp].oc_uses_known)
		otmp->known = 1;

#ifdef INVISIBLE_OBJECTS
	otmp->oinvis = !rn2(1250);
#endif
	otmp->quan = is_multigen(otmp) ? ((long) rn1(4,4) + d(2,level_difficulty()+2)) : 1L;
	if (is_rakuyo(otmp))
		add_oprop(otmp, OPROP_RAKUW);
	if (is_mercy_blade(otmp))
		add_oprop(otmp, OPROP_MRCYW);
	//Always-init otyps
	if (otmp->otyp == WHITE_VIBROSWORD
		|| otmp->otyp == WHITE_VIBROSPEAR
		|| otmp->otyp == WHITE_VIBROZANBATO
		)
		add_oprop(otmp, OPROP_HOLYW);
	else if (otmp->otyp == GOLD_BLADED_VIBROSWORD
		|| otmp->otyp == GOLD_BLADED_VIBROSPEAR
		|| otmp->otyp == GOLD_BLADED_VIBROZANBATO
		)
		add_oprop(otmp, OPROP_UNHYW);
	else if (otmp->otyp == MOON_AXE){
		switch (phase_of_the_moon()){
		case 0:
			otmp->ovar1_moonPhase = ECLIPSE_MOON;
			break;
		case 1:
		case 7:
			otmp->ovar1_moonPhase = CRESCENT_MOON;
			break;
		case 2:
		case 6:
			otmp->ovar1_moonPhase = HALF_MOON;
			break;
		case 3:
		case 5:
			otmp->ovar1_moonPhase = GIBBOUS_MOON;
			break;
		case 4:
			otmp->ovar1_moonPhase = FULL_MOON;
			break;
		case 8:
			otmp->ovar1_moonPhase = HUNTING_MOON;
			break;
		}
	}
	else if(otmp->otyp == CHURCH_HAMMER){
		struct obj *sword = mksobj(HUNTER_S_SHORTSWORD, mkflags);
		add_to_container(otmp, sword);
	}
	else if(otmp->otyp == CHURCH_BLADE){
		struct obj *sword = mksobj(HUNTER_S_LONGSWORD, mkflags);
		add_to_container(otmp, sword);
	}
	else if (otmp->otyp == MASS_SHADOW_PISTOL){
		struct obj *stone = mksobj(ROCK, NO_MKOBJ_FLAGS);
		stone->quan = 1;
		stone->owt = weight(stone);
		add_to_container(otmp, stone);
		container_weight(otmp);
	}

	if (init) {
		switch (let) {
		case WEAPON_CLASS:
			if (!rn2(11)) {
				otmp->spe = rne(3);
				otmp->blessed = rn2(2);
			}
			else if (!rn2(10)) {
				curse(otmp);
				otmp->spe = -rne(3);
			}
			else	blessorcurse(otmp, 10);

			if (is_vibroweapon(otmp)){
				otmp->ovar1_charges = 80L + rnd(20);
			}
			else if (otmp->otyp == RAYGUN){
				otmp->ovar1_charges = (8 + rnd(8)) * 10L;
				otmp->altmode = AD_SLEE;
			}
			else if (otmp->otyp == MASS_SHADOW_PISTOL){
				otmp->ovar1_charges = 800L + rnd(200);
			}
			else if (otmp->otyp == DEVIL_FIST || otmp->otyp == DEMON_CLAW){
				struct obj *coin = mksobj(WAGE_OF_SLOTH + rn2(WAGE_OF_PRIDE - (WAGE_OF_SLOTH-1)), NO_MKOBJ_FLAGS);
				add_to_container(otmp, coin);
			}
			else if (is_blaster(otmp)){ //Rayguns and mass-shadow pistols are also blasters, so this has to go under that case
				otmp->ovar1_charges = 80L + rnd(20);
				if (otmp->otyp == ARM_BLASTER) otmp->altmode = WP_MODE_SINGLE;
				if (otmp->otyp == RAYGUN) otmp->altmode = AD_FIRE;	// I think this is never reached?
			}
			//#ifdef FIREARMS
			if (otmp->otyp == STICK_OF_DYNAMITE) {
				otmp->age = (otmp->cursed ? rn2(15) + 2 :
					(otmp->blessed ? 15 : rn2(10) + 10));
			}
			//#endif
			if (is_poisonable(otmp) && ((is_ammo(otmp) && !rn2(100)) || !rn2(1000))){
				if (!rn2(100)) otmp->opoisoned = OPOISON_FILTH; /* Once a game or once every few games */
				else otmp->opoisoned = OPOISON_BASIC;
			}
			else if ((otmp)->obj_material == WOOD
				&& otmp->oartifact != ART_BOW_OF_SKADI
				&& !rn2(100)
				){
				switch (d(1, 4)){
				case 1: otmp->oward = WARD_TOUSTEFNA; break;
				case 2: otmp->oward = WARD_DREPRUN; break;
				case 3: otmp->oward = WARD_VEIOISTAFUR; break;
				case 4: otmp->oward = WARD_THJOFASTAFUR; break;
				}
			}
			break;
		case FOOD_CLASS:
			otmp->odrained = 0;
			otmp->oeaten = 0;
			switch (otmp->otyp) {
			case CORPSE:
				/* possibly overridden by mkcorpstat() */
				tryct = 50;
				do otmp->corpsenm = undead_to_corpse(rndmonnum());
				while ((mvitals[otmp->corpsenm].mvflags & G_NON_GEN_CORPSE) && (--tryct > 0));
				if (tryct == 0) {
					/* perhaps rndmonnum() only wants to make G_NOCORPSE monsters on
					   this level; let's create an adventurer's corpse instead, then */
					otmp->corpsenm = PM_HUMAN;
				}
				/* timer set below */
				break;
			case EGG:
				otmp->corpsenm = NON_PM;	/* generic egg */

				if (In_sokoban(&u.uz)
					|| Is_gatetown(&u.uz)
					|| Infuture
				) break; /*Some levels shouldn't have mosnters spawning from eggs*/

				if (!rn2(3)) for (tryct = 200; tryct > 0; --tryct) {
					mndx = can_be_hatched(rndmonnum());
					if (mndx != NON_PM && !dead_species(mndx, TRUE)) {
						otmp->corpsenm = mndx;		/* typed egg */
						attach_egg_hatch_timeout(otmp);
						break;
					}
				}
				break;
			case EYEBALL:
				otmp->corpsenm = PM_HUMAN;
				break;
			case TIN:
				otmp->corpsenm = NON_PM;	/* empty (so far) */
				if (!rn2(6))
					otmp->spe = 1;		/* spinach */
				else for (tryct = 200; tryct > 0; --tryct) {
					mndx = undead_to_corpse(rndmonnum());
					if (mons[mndx].cnutrit &&
						!(mvitals[mndx].mvflags & G_NON_GEN_CORPSE)) {
						otmp->corpsenm = mndx;
						break;
					}
				}
				blessorcurse(otmp, 10);
				break;
			case SLIME_MOLD:
				otmp->spe = current_fruit;
				break;
			case KELP_FROND:
				otmp->quan = (long)rnd(2);
				break;
			case PROTEIN_PILL:
				otmp->quan = rnd(5) + 5;
				otmp->owt = weight(otmp);
				break;
			}
			if (otmp->otyp == CORPSE || otmp->otyp == MEAT_RING ||
				otmp->otyp == KELP_FROND) break;
			/* fall into next case */

		case GEM_CLASS:
			if (otmp->otyp == LOADSTONE) curse(otmp);
			else if (otmp->otyp == ROCK) otmp->quan = (long)rn1(6, 6);
			else if (otmp->otyp != LUCKSTONE && !rn2(6)) otmp->quan = 2L;
			else otmp->quan = 1L;
			break;
		case TOOL_CLASS:
			switch (otmp->otyp) {
			case TALLOW_CANDLE:
			case WAX_CANDLE:	otmp->spe = 1;
				otmp->age = 20L * /* 400 or 200 */
					(long)objects[otmp->otyp].oc_cost;
				otmp->lamplit = 0;
				otmp->quan = 1L + ((long)(rn2(2) && !Is_grue_level(&u.uz)) ? rn2(7) : 0);
				blessorcurse(otmp, 5);
				break;
			case LANTERN:
			case OIL_LAMP:		otmp->spe = 1;
				otmp->age = (long)rn1(500, 1000);
				otmp->lamplit = 0;
				blessorcurse(otmp, 5);
				break;
			case TORCH:
				otmp->age = (long)rn1(500, 1000);
				otmp->lamplit = 0;
				blessorcurse(otmp, 5);
				break;
			case SUNROD:
				otmp->age = (long)rn1(500, 1000);
				otmp->lamplit = 0;
				blessorcurse(otmp, 5);
				break;
			case SHADOWLANDER_S_TORCH:
				otmp->age = (long)rn1(500, 1000);
				otmp->lamplit = 0;
				blessorcurse(otmp, 5);
				break;
			case MAGIC_TORCH:
				otmp->spe = 0;
				otmp->lamplit = 0;
				break;
			case CANDLE_OF_INVOCATION:
			case MAGIC_LAMP:	otmp->spe = 1;
				otmp->lamplit = 0;
				blessorcurse(otmp, 2);
				break;
			case SEISMIC_HAMMER:
				otmp->ovar1_charges = 80L + rnd(20);
				break;
			case DOUBLE_LIGHTSABER:
			case LIGHTSABER:
			case BEAMSWORD:
			case ROD_OF_FORCE:
				otmp->altmode = FALSE;
				otmp->lamplit = 0;
				otmp->age = (long)rn1(50000, 100000);
				blessorcurse(otmp, 2);
				if(is_gemable_lightsaber(otmp)){
					struct obj *gem = mksobj(rn2(6) ? BLUE_FLUORITE : GREEN_FLUORITE, NO_MKOBJ_FLAGS);
					gem->quan = 1;
					gem->owt = weight(gem);
					add_to_container(otmp, gem);
					container_weight(otmp);
				}
				if(otmp->otyp == LIGHTSABER)
					otmp->ovar1_lightsaberHandle = random_saber_hilt();
				else if(otmp->otyp == BEAMSWORD)
					otmp->ovar1_lightsaberHandle = random_beam_hilt();
				break;
			case CHEST:
			case BOX:
			case SARCOPHAGUS:
				if (Is_stronghold(&u.uz) && in_mklev){
					otmp->olocked = 1;
					otmp->otrapped = 0;
				}
				else {
					otmp->olocked = !!(rn2(5));
					otmp->otrapped = !(rn2(10));
				}
			case ICE_BOX:
			case SACK:
			case OILSKIN_SACK:
			case MASSIVE_STONE_CRATE:
			case WRITING_DESK:
			case BAG_OF_HOLDING:	mkbox_cnts(otmp);
				break;
			case MAGIC_CHEST:
				otmp->olocked = 1;
				otmp->obolted = 0;
				break;
#ifdef TOURIST
			case EXPENSIVE_CAMERA:
#endif
			case DISSECTION_KIT:
			case TINNING_KIT:
			case MAGIC_MARKER:	otmp->spe = rn1(70, 30);
				break;
			case MIST_PROJECTOR:
			case TREPHINATION_KIT:
			case CAN_OF_GREASE:	otmp->spe = rnd(25);
				blessorcurse(otmp, 10);
				break;
			case PHLEBOTOMY_KIT:
				otmp->spe = rnd(5);
				break;
			case CRYSTAL_BALL:	otmp->spe = rnd(5);
				blessorcurse(otmp, 2);
				break;
			case PRESERVATIVE_ENGINE:
				otmp->spe = rn1(4,4);
				otmp->altmode = ENG_MODE_PYS;
				break;
			case ARMOR_SALVE:
				otmp->spe = rn1(3,3);
				break;
			case POWER_PACK:
				otmp->quan = rnd(5) + 5;
				otmp->owt = weight(otmp);
				break;
			case SENSOR_PACK:
				otmp->spe = rnd(5) + 20;
				break;
			case HYPOSPRAY_AMPULE:{
									  int pick;
									  switch (rn2(14)){
									  case 0:
										  pick = POT_GAIN_ABILITY;
										  break;
									  case 1:
										  pick = POT_RESTORE_ABILITY;
										  break;
									  case 2:
										  pick = POT_BLINDNESS;
										  break;
									  case 3:
										  pick = POT_CONFUSION;
										  break;
									  case 4:
										  pick = POT_PARALYSIS;
										  break;
									  case 5:
										  pick = POT_SPEED;
										  break;
									  case 6:
										  pick = POT_HALLUCINATION;
										  break;
									  case 7:
										  pick = POT_HEALING;
										  break;
									  case 8:
										  pick = POT_EXTRA_HEALING;
										  break;
									  case 9:
										  pick = POT_GAIN_ENERGY;
										  break;
									  case 10:
										  pick = POT_SLEEPING;
										  break;
									  case 11:
										  pick = POT_FULL_HEALING;
										  break;
									  case 12:
										  pick = POT_POLYMORPH;
										  break;
									  case 13:
										  pick = POT_AMNESIA;
										  break;
									  }
									  otmp->ovar1_ampule = (long)(pick);
									  otmp->spe = rn1(6, 6);
			}break;
			case HORN_OF_PLENTY:
			case BAG_OF_TRICKS:	otmp->spe = rnd(20);
				break;
			case FIGURINE:	{
				if (Is_paradise(&u.uz)){
					switch (rn2(3)){
					case 0:
						otmp->corpsenm = PM_SHADE;
						break;
					case 1:
						otmp->corpsenm = PM_DARKNESS_GIVEN_HUNGER;
						break;
					case 2:
						otmp->corpsenm = PM_NIGHTGAUNT;
						break;
					}
				}
				else if (Is_sunkcity(&u.uz)){
					switch (rn2(3)){
					case 0:
						otmp->corpsenm = PM_DEEPEST_ONE;
						break;
					case 1:
						otmp->corpsenm = PM_MASTER_MIND_FLAYER;
						break;
					case 2:
						otmp->corpsenm = PM_SHOGGOTH;
						break;
					}
				}
				else {
					int tryct2 = 0;
					do
					otmp->corpsenm = rndmonnum();
					while (is_human(&mons[otmp->corpsenm])
						&& tryct2++ < 30);
				}
				blessorcurse(otmp, 4);
				break;
			}
			case CRYSTAL_SKULL:	{
				struct monst * mon;
				struct obj * otmp2;
				struct obj * oinv;
				int skull;
				int template = 0;
				int extra_flags = 0;
				if(Infuture){
					if(Race_if(PM_ANDROID)){
						int skulls[] = {PM_DWARF_KING, PM_DWARF_QUEEN, PM_GITHYANKI_PIRATE, PM_DEMINYMPH, PM_MORDOR_MARSHAL, PM_MOUNTAIN_CENTAUR, PM_DRIDER, 
							PM_DROW_CAPTAIN, PM_HEDROW_WIZARD, PM_DROW_MATRON, PM_HEDROW_BLADEMASTER,
							PM_ARCHEOLOGIST, PM_KNIGHT, PM_MADMAN, PM_MADWOMAN,
							PM_EMBRACED_DROWESS, PM_EMBRACED_DROWESS, PM_NURSE, 
							
							PM_GYNOID, PM_GYNOID, PM_GYNOID, PM_OPERATOR, PM_GYNOID, PM_GYNOID, PM_GYNOID, PM_OPERATOR, PM_ANDROID,
							PM_MYRKALFAR_WARRIOR, PM_MYRKALFAR_WARRIOR, PM_DWARF_WARRIOR, PM_DWARF_WARRIOR, PM_HUMAN, PM_HUMAN, 
							PM_INCANTIFIER, PM_INCANTIFIER 
						};
						skull = ROLL_FROM(skulls);
					}
					else {
						int skulls[] = {PM_DWARF_KING, PM_DWARF_QUEEN, PM_GITHYANKI_PIRATE, PM_DEMINYMPH, PM_MORDOR_MARSHAL, PM_MOUNTAIN_CENTAUR, PM_DRIDER, 
							PM_DROW_CAPTAIN, PM_HEDROW_WIZARD, PM_DROW_MATRON, PM_HEDROW_BLADEMASTER,
							PM_ARCHEOLOGIST, PM_KNIGHT, PM_MADMAN, PM_MADWOMAN,
							PM_EMBRACED_DROWESS, PM_EMBRACED_DROWESS, 
							
							PM_MYRKALFR, PM_MYRKALFR, PM_ELF, PM_ELF, 
							PM_MYRKALFAR_WARRIOR, PM_MYRKALFAR_WARRIOR, PM_DWARF_WARRIOR, PM_DWARF_WARRIOR, PM_HUMAN, PM_HUMAN, 
							PM_INCANTIFIER, PM_INCANTIFIER 
						};
						skull = ROLL_FROM(skulls);
					}
				}
				// else if(In_quest(&u.uz) && Role_if(PM_HEALER) && urole.neminum == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH && mvitals[PM_BLIBDOOLPOOLP_S_MINDGRAVEN_CHAMPION].born == 0){
				else if(In_quest(&u.uz) && Is_nemesis(&u.uz) && Role_if(PM_HEALER) && urole.neminum == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH && mvitals[PM_BLIBDOOLPOOLP_S_MINDGRAVEN_CHAMPION].born == 0){
					skull = PM_BLIBDOOLPOOLP_S_MINDGRAVEN_CHAMPION;
					mvitals[PM_BLIBDOOLPOOLP_S_MINDGRAVEN_CHAMPION].born = 1;
				}
				else if(Role_if(PM_MADMAN) && Race_if(PM_GNOME) && on_level(&u.uz, &nemesis_level) && in_mklev){
					skull = PM_COURE_ELADRIN;
					template = PSEUDONATURAL;
					extra_flags = NO_MINVENT;
				}
				else {
					int skulls[] = {PM_DWARF_KING, PM_DWARF_QUEEN, PM_MAID, 
						PM_DUERGAR_DEEPKING,
						PM_GITHYANKI_PIRATE, PM_GITHYANKI_PIRATE,
						PM_DEMINYMPH, 
						PM_MORDOR_ORC_ELITE, PM_MORDOR_MARSHAL, PM_ANGBAND_ORC, PM_ORC_OF_THE_AGES_OF_STARS,
						PM_MOUNTAIN_CENTAUR, PM_DRIDER, 
						PM_DROW_CAPTAIN, PM_HEDROW_WIZARD, PM_DROW_MATRON, PM_HEDROW_BLADEMASTER, 
						PM_DROW_CAPTAIN, PM_HEDROW_WARRIOR, PM_DROW_MATRON, PM_DROW_ALIENIST, 
						PM_ANULO, PM_ANULO_DANCER,
						PM_ELF_LORD, PM_ELF_LADY, PM_ELVENKING, PM_ELVENQUEEN, 
						PM_STAR_ELF, PM_STAR_ELF, PM_STAR_EMPEROR, PM_STAR_EMPRESS,
						PM_ARCHEOLOGIST, PM_BARBARIAN, PM_HALF_DRAGON, PM_CAVEMAN, PM_CAVEWOMAN, 
						PM_KNIGHT, PM_KNIGHT, PM_MADMAN, PM_MADWOMAN, PM_PRIEST, PM_PRIESTESS,
						PM_RANGER, PM_ROGUE, PM_ROGUE, PM_SAMURAI, PM_UNDEAD_HUNTER, PM_VALKYRIE,
						PM_WIZARD 
					};
					skull = ROLL_FROM(skulls);
				}
				mon = makemon(&mons[skull], 0, 0, MM_ADJACENTOK|MM_NOCOUNTBIRTH|extra_flags);
				if(mon){
					if(template)
						set_template(mon, template);
					if(mon->m_lev < 10){
						mon->m_lev = 10;
						mon->mhp = mon->mhpmax = d(10, 8);
					}
					if(quest_faction(mon))
						set_faction(mon, 0);
					for(oinv = mon->minvent; oinv; oinv = mon->minvent){
						mon->misc_worn_check &= ~oinv->owornmask;
						update_mon_intrinsics(mon, oinv, FALSE, FALSE);
						if (oinv->owornmask & W_WEP){
							setmnotwielded(mon,oinv);
							MON_NOWEP(mon);
						}
						if (oinv->owornmask & W_SWAPWEP){
							setmnotwielded(mon,oinv);
							MON_NOSWEP(mon);
						}
						oinv->owornmask = 0L;
						if(obj_is_burning(oinv))
							end_burn(oinv, TRUE);
						obj_extract_self(oinv);
						add_to_container(otmp, oinv);
						container_weight(otmp);
					}
					otmp2 = save_mtraits(otmp, mon);
					if(otmp2)
						otmp = otmp2;
					mongone(mon);
				}
			}break;
			case BELL_OF_OPENING:   otmp->spe = 3;
				break;
			case MAGIC_FLUTE:
			case MAGIC_HARP:
			case FROST_HORN:
			case FIRE_HORN:
			case DRUM_OF_EARTHQUAKE:
				otmp->spe = rn1(5, 4);
				break;
			case MASK:
				if (rn2(4)){
					int tryct2 = 0;
					do otmp->corpsenm = rndmonnum();
					while (is_human(&mons[otmp->corpsenm])
						&& tryct2++ < 30);
				}
				else if (rn2(10)){
					do otmp->corpsenm = rn2(PM_LONG_WORM_TAIL);
					while (mons[otmp->corpsenm].geno & G_UNIQ);
				}
				else {
					do otmp->corpsenm = PM_ARCHEOLOGIST + rn2(PM_WIZARD - PM_ARCHEOLOGIST);
					while (otmp->corpsenm == PM_WORM_THAT_WALKS);
				}
				doMaskStats(otmp);
				break;
			case DOLL_S_TEAR:
				otmp->ovar1_dollTypes = init_doll_sales();
				otmp->spe = rnd(20);
				break;
			}
			break;
		case AMULET_CLASS:
			if (otmp->otyp == AMULET_OF_YENDOR) flags.made_amulet = TRUE;
			if (rn2(10) && (otmp->otyp == AMULET_OF_STRANGULATION ||
				otmp->otyp == AMULET_OF_CHANGE ||
				otmp->otyp == AMULET_OF_RESTFUL_SLEEP)) {
				curse(otmp);
			}
			else	blessorcurse(otmp, 10);
		case VENOM_CLASS:
		case CHAIN_CLASS:
			switch(otmp->otyp){
				case BROKEN_ANDROID:
					otmp->corpsenm = PM_ANDROID;
				break;
				case BROKEN_GYNOID:
					otmp->corpsenm = rn2(20) ? PM_GYNOID : PM_OPERATOR;
				break;
				case LIFELESS_DOLL:
					otmp->corpsenm = PM_LIVING_DOLL;
				break;
			}
			if(otmp->otyp == BROKEN_ANDROID || otmp->otyp == BROKEN_GYNOID || otmp->otyp == LIFELESS_DOLL){
				struct monst * mon;
				struct obj * otmp2;
				int tame = 0;
				if(Role_if(PM_MADMAN) && on_level(&u.uz, &nemesis_level) && in_mklev){
					tame = MM_EDOG;
				}
				mon = makemon(&mons[otmp->corpsenm], 0, 0, MM_ADJACENTOK|NO_MINVENT|MM_NOCOUNTBIRTH|tame);
				if(mon){
					if(tame){
						initedog(mon);
						EDOG(mon)->loyal = TRUE;
					}
					otmp2 = save_mtraits(otmp, mon);
					if(otmp2)
						otmp = otmp2;
					if(mon->m_insight_level){
						otmp->ovar1_insightlevel = mon->m_insight_level;
					}
					mongone(mon);
				}
			}
			break;
		case BALL_CLASS:
			break;
		case POTION_CLASS:
			if (otmp->otyp == POT_BLOOD){
				otmp->corpsenm = PM_HUMAN;	/* default value */
				for (tryct = 200; tryct > 0; --tryct) {
					mndx = undead_to_corpse(rndmonnum());
					if (mons[mndx].cnutrit &&
						!(mvitals[mndx].mvflags & G_NON_GEN_CORPSE)
						&& has_blood(&mons[mndx])) {
						otmp->corpsenm = mndx;
						break;
					}
				}
				blessorcurse(otmp, 10);
			}
			if (otmp->otyp == POT_OIL)
				otmp->age = MAX_OIL_IN_FLASK;	/* amount of oil */
			/* fall through */
		case SCROLL_CLASS:
#ifdef MAIL
			if (otmp->otyp != SCR_MAIL)
#endif
				blessorcurse(otmp, 4);
			if (otmp->otyp == SCR_WARD){
				int prob = rn2(73);
				/*The circle of acheron is so common and so easy to draw that noone makes ward scrolls of it*/
				if (prob < 10) otmp->oward = WINGS_OF_GARUDA;
				else if (prob < 20) otmp->oward = CARTOUCHE_OF_THE_CAT_LORD;
				else if (prob < 30) otmp->oward = SIGN_OF_THE_SCION_QUEEN;
				else if (prob < 40) otmp->oward = ELDER_ELEMENTAL_EYE;
				else if (prob < 50) otmp->oward = ELDER_SIGN;
				else if (prob < 60) otmp->oward = HAMSA;
				else if (prob < 70) otmp->oward = PENTAGRAM;
				else otmp->oward = HEXAGRAM;
			}
			break;
		case SPBOOK_CLASS:
			blessorcurse(otmp, 17);
			// WARD_ACHERON			0x0000008L
			// WARD_QUEEN			0x0000200L
			// WARD_GARUDA			0x0000800L

			// WARD_ELDER_SIGN		0x0000080L
			// WARD_EYE				0x0000100L
			// WARD_CAT_LORD		0x0000400L

			// WARD_HEXAGRAM		0x0000020L
			// WARD_PENTAGRAM		0x0000010L
			// WARD_HAMSA			0x0000040L

			// WARD_HEPTAGRAM		0x0000002L

			/*Spellbooks are warded to help contain the magic.
			  Some of the wards contain usable symbols*/
			switch (objects[otmp->otyp].oc_level) {
			case 0:
				break;
			case 1:
				if ((rn2(3))) otmp->oward = WARD_ACHERON;
				else if (!(rn2(3))){
					if (rn2(2)) otmp->oward = WARD_QUEEN;
					else otmp->oward = WARD_GARUDA;
				}
				break;
			case 2:
				if (rn2(2)){
					if (!(rn2(8))) otmp->oward = WARD_EYE;
					else if (rn2(2)) otmp->oward = WARD_QUEEN;
					else otmp->oward = WARD_GARUDA;
				}
				else if (rn2(3)) otmp->oward = WARD_ACHERON;
				break;
			case 3:
				if (!(rn2(3))){
					if (!(rn2(5))) otmp->oward = WARD_EYE;
					else if (!(rn2(4))) otmp->oward = WARD_QUEEN;
					else if (!(rn2(3)))otmp->oward = WARD_GARUDA;
					else if (rn2(2))otmp->oward = WARD_ELDER_SIGN;
					else otmp->oward = WARD_CAT_LORD;
				}
				else if (rn2(2)){
					if (!(rn2(4))) otmp->oward = WARD_TOUSTEFNA;
					else if (!(rn2(3)))otmp->oward = WARD_DREPRUN;
					else if ((rn2(2)))otmp->oward = WARD_VEIOISTAFUR;
					else otmp->oward = WARD_THJOFASTAFUR;
				}
				else if (rn2(3)) otmp->oward = WARD_ACHERON;
				break;
			case 4:
				if (rn2(4)){
					if (!(rn2(9))) otmp->oward = WARD_EYE;
					else if (!rn2(8)) otmp->oward = WARD_QUEEN;
					else if (!rn2(7))otmp->oward = WARD_GARUDA;
					else if (!rn2(6))otmp->oward = WARD_ELDER_SIGN;
					else if (!rn2(5))otmp->oward = WARD_CAT_LORD;
					else if (!rn2(4)) otmp->oward = WARD_TOUSTEFNA;
					else if (!rn2(3))otmp->oward = WARD_DREPRUN;
					else if (rn2(2))otmp->oward = WARD_VEIOISTAFUR;
					else otmp->oward = WARD_THJOFASTAFUR;
				}
				else otmp->oward = WARD_ACHERON;
				break;
			case 5:
				if (!(rn2(4))){
					if (!(rn2(2))) otmp->oward = WARD_PENTAGRAM;
					else otmp->oward = WARD_HAMSA;
				}
				else if ((rn2(3))){
					if (!(rn2(8))) otmp->oward = WARD_QUEEN;
					else if (!rn2(7))otmp->oward = WARD_GARUDA;
					else if (!rn2(6))otmp->oward = WARD_ELDER_SIGN;
					else if (!rn2(5))otmp->oward = WARD_CAT_LORD;
					else if (!rn2(4)) otmp->oward = WARD_TOUSTEFNA;
					else if (!rn2(3))otmp->oward = WARD_DREPRUN;
					else if (rn2(2))otmp->oward = WARD_VEIOISTAFUR;
					else otmp->oward = WARD_THJOFASTAFUR;
				}
				else otmp->oward = WARD_EYE;
				break;
			case 6:
				if (!(rn2(3))){
					if (!(rn2(3))) otmp->oward = WARD_PENTAGRAM;
					else if (!rn2(2))otmp->oward = WARD_HEXAGRAM;
					else otmp->oward = WARD_HAMSA;
				}
				else if ((rn2(6))){
					if (!(rn2(8))) otmp->oward = WARD_QUEEN;
					else if (!rn2(7))otmp->oward = WARD_GARUDA;
					else if (!rn2(6))otmp->oward = WARD_ELDER_SIGN;
					else if (!rn2(5))otmp->oward = WARD_CAT_LORD;
					else if (!rn2(4)) otmp->oward = WARD_TOUSTEFNA;
					else if (!rn2(3))otmp->oward = WARD_DREPRUN;
					else if (rn2(2))otmp->oward = WARD_VEIOISTAFUR;
					else otmp->oward = WARD_THJOFASTAFUR;
				}
				else otmp->oward = WARD_EYE;
				break;
			case 7:
				if (!(rn2(4))){
					if (!(rn2(3))) otmp->oward = WARD_EYE;
					else if (!rn2(2))otmp->oward = WARD_ELDER_SIGN;
					else otmp->oward = WARD_CAT_LORD;
				}
				else if (!(rn2(3))){
					if (!(rn2(3))) otmp->oward = WARD_ACHERON;
					else if (!rn2(2))otmp->oward = WARD_QUEEN;
					else otmp->oward = WARD_GARUDA;
				}
				else if (!(rn2(2))){
					if (!(rn2(3))) otmp->oward = WARD_HEXAGRAM;
					else if (!rn2(2))otmp->oward = WARD_PENTAGRAM;
					else otmp->oward = WARD_HAMSA;
				}
				else{
					otmp->oward = WARD_HEPTAGRAM;
				}
				break;
			default:
				impossible("Unknown spellbook level %d, book %d;",
					objects[otmp->otyp].oc_level, otmp->otyp);
				return 0;
			}
			break;
		case ARMOR_CLASS:
			if (rn2(10) && (otmp->otyp == FUMBLE_BOOTS ||
				otmp->otyp == HELM_OF_OPPOSITE_ALIGNMENT ||
				otmp->otyp == GAUNTLETS_OF_FUMBLING ||
				!rn2(11))) {
				curse(otmp);
				otmp->spe = -rne(3);
			}
			else if (!rn2(10)) {
				otmp->blessed = rn2(2);
				otmp->spe = rne(3);
			}
			else	blessorcurse(otmp, 10);
			/* simulate lacquered armor for samurai */
			if (Role_if(PM_SAMURAI) && otmp->otyp == SPLINT_MAIL &&
				(moves <= 1 || In_quest(&u.uz))) {
#ifdef UNIXPC
				/* optimizer bitfield bug */
				otmp->oerodeproof = 1;
				otmp->rknown = 1;
#else
				otmp->oerodeproof = otmp->rknown = 1;
#endif
			}
			/* MRKR: Mining helmets have lamps */
			if (otmp->otyp == DWARVISH_HELM) {
				otmp->age = (long)rn1(300, 300);//Many fewer turns than brass lanterns, as there are so many.
				otmp->lamplit = 0;
			}
			/* CM: gnomish hats have candles */
			if (otmp->otyp == GNOMISH_POINTY_HAT) {
				otmp->age = (long)rn1(900, 900);//Last longer than dwarvish helms, since the radius is smaller
				otmp->lamplit = 0;
			}
			if (otmp->otyp == LANTERN_PLATE_MAIL) {
				otmp->age = (long)rn1(500, 1000);
				otmp->lamplit = 0;
			}
			if (otmp->otyp == EILISTRAN_ARMOR) {
				otmp->altmode = EIL_MODE_ON;
				otmp->ovar1_eilistran_charges = 600;
			}
			if (is_readable_armor_otyp(otmp->otyp)){
				otmp->ohaluengr = TRUE;
				if (Race_if(PM_DROW) && Is_qstart(&u.uz)) otmp->oward = u.start_house;
				else if (!(rn2(10))) otmp->oward = rn2(EDDER_SYMBOL + 1 - LOLTH_SYMBOL) + LOLTH_SYMBOL;
				else if (!(rn2(4))) otmp->oward = rn2(LAST_HOUSE + 1 - FIRST_HOUSE) + FIRST_HOUSE;
				else otmp->oward = rn2(LAST_FALLEN_HOUSE + 1 - FIRST_FALLEN_HOUSE) + FIRST_FALLEN_HOUSE;
			}

			break;
		case WAND_CLASS:
			if (otmp->otyp == WAN_WISHING)
				otmp->spe = rnd(3);
			else
				otmp->spe = rn1(5, (objects[otmp->otyp].oc_dir == NODIR) ? 11 : 4);
			blessorcurse(otmp, 17);
			if (otmp->otyp == WAN_WISHING)
				otmp->recharged = 1;
			else
				otmp->recharged = 0; /* used to control recharging */
			break;
		case RING_CLASS:
			if (isEngrRing(otmp->otyp) && !rn2(3)){
				if (rn2(4)){
					otmp->ohaluengr = TRUE;
					otmp->oward = (long)random_haluIndex();
				}
				else{
					otmp->ohaluengr = FALSE;
					otmp->oward = rn2(4) ? CIRCLE_OF_ACHERON :
						!rn2(6) ? HAMSA :
						!rn2(5) ? ELDER_SIGN :
						!rn2(4) ? WINGS_OF_GARUDA :
						!rn2(3) ? ELDER_ELEMENTAL_EYE :
						!rn2(2) ? SIGN_OF_THE_SCION_QUEEN :
						CARTOUCHE_OF_THE_CAT_LORD;
				}
			}
			if (isSignetRing(otmp->otyp)){
				otmp->ohaluengr = TRUE;
				if (!(rn2(100))) otmp->oward = rn2(EDDER_SYMBOL + 1 - LOLTH_SYMBOL) + LOLTH_SYMBOL;
				else if (!(rn2(4))) otmp->oward = rn2(LAST_HOUSE + 1 - FIRST_HOUSE) + FIRST_HOUSE;
				else otmp->oward = rn2(LAST_FALLEN_HOUSE + 1 - FIRST_FALLEN_HOUSE) + FIRST_FALLEN_HOUSE;
			}
			if (otmp->otyp == RIN_WISHES){
				if(Is_stronghold(&u.uz)){
					if(u.ring_wishes == -1){
						otmp->spe = rnd(3);
						u.ring_wishes = otmp->spe;
					} else {
						otmp->spe = u.ring_wishes;
					}
				} else {
					otmp->spe = rnd(3);
				}
			}
			if (objects[otmp->otyp].oc_charged && otmp->otyp != RIN_WISHES) {
				blessorcurse(otmp, 3);
				if (rn2(10)) {
					if (rn2(10) && bcsign(otmp))
						otmp->spe = bcsign(otmp) * rne(3);
					else otmp->spe = rn2(2) ? rne(3) : -rne(3);
				}
				/* make useless +0 rings much less common */
				if (otmp->spe == 0) otmp->spe = rn2(4) - rn2(3);
				/* negative rings are usually cursed */
				if (otmp->spe < 0 && rn2(5)) curse(otmp);
			}
			else if (rn2(10) && (otmp->otyp == RIN_TELEPORTATION ||
				otmp->otyp == RIN_POLYMORPH ||
				otmp->otyp == RIN_AGGRAVATE_MONSTER ||
				otmp->otyp == RIN_HUNGER || !rn2(9))) {
				curse(otmp);
			}
			break;
		case ROCK_CLASS:
			switch (otmp->otyp) {
			case STATUE:
				/* possibly overridden by mkcorpstat() */
				otmp->corpsenm = rndmonnum();
				if (!verysmall(&mons[otmp->corpsenm]) &&
					rn2(level_difficulty() / 2 + 10) > 10)
					(void)add_to_container(otmp,
					mkobj(SPBOOK_CLASS, FALSE));
				break;
			case FOSSIL:
				switch (rnd(12)){
				case 1:
					otmp->corpsenm = PM_SABER_TOOTHED_CAT;
					break;
				case 2:
					otmp->corpsenm = PM_TYRANNOSAURUS;
					break;
				case 3:
					otmp->corpsenm = PM_TRICERATOPS;
					break;
				case 4:
					otmp->corpsenm = PM_DIPLODOCUS;
					break;
				case 5:
				case 6:
				case 7:
				case 8:
					otmp->corpsenm = PM_TRILOBITE;
					break;
				case 9:
					otmp->corpsenm = PM_TITANOTHERE;
					break;
				case 10:
					otmp->corpsenm = PM_BALUCHITHERIUM;
					break;
				case 11:
					otmp->corpsenm = PM_MASTODON;
					break;
				case 12:
					otmp->corpsenm = PM_CENTIPEDE;
					break;
				}
				otmp->owt = weight(otmp);
			}
			break;
		case BED_CLASS:
			if(otmp->otyp == BERGONIC_CHAIR){
				otmp->spe = d(3,3);
			}
			break;
		case BELT_CLASS:
			if (rn2(10) && (otmp->otyp == BELT_OF_WEAKNESS
				|| otmp->otyp == BELT_OF_CARRYING 
				|| otmp->otyp == BELT_OF_WEIGHT 
				|| !rn2(11))
			) {
				curse(otmp);
				if(otmp->otyp == KIDNEY_BELT)
					otmp->spe = -rne(3);
			}
			else if (!rn2(10)) {
				otmp->blessed = rn2(2);
				if(otmp->otyp == KIDNEY_BELT)
					otmp->spe = rne(3);
			}
			else	blessorcurse(otmp, 10);
			break;
		case COIN_CLASS:
		case TILE_CLASS:
		case SCOIN_CLASS:
			break;	/* do nothing */
		default:
			impossible("impossible mkobj %d, sym '%c'.", otmp->otyp,
				objects[otmp->otyp].oc_class);
			return (struct obj *)0;
		}
		/* possibly make into an artifact */
		if (artif && !rn2(Role_if(PM_PIRATE) ? 5 : 20) &&
			(!(let == ARMOR_CLASS || let == TOOL_CLASS) || !rn2(2))	/* armor and tool artifacts are rarer (1/2 the usual rate) */
			)
		{
			otmp = mk_artifact(otmp, (aligntyp)A_NONE);
		}

		/* your quest tends to be stocked with things that fit you */
		if (quest_equipment(otmp) && !otmp->oartifact) {
			otmp->objsize = (&mons[urace.malenum])->msize;
			if (otmp->oclass == ARMOR_CLASS){
				set_obj_shape(otmp, mons[urace.malenum].mflagsb);
			}
		}
	}

	/* Some things must get done (timers) even if init = 0 */
	if (obj_eternal_light(otmp))
		begin_burn(otmp);
	switch (otmp->otyp) {
	    case CORPSE:
		start_corpse_timeout(otmp);
		break;
	}

	if(check_oprop(otmp, OPROP_WRTHW))
		otmp->wrathdata = PM_ORC<<2;//wrathful + 1/4 vs orcs

	/* spellbooks of secrets should become a random artifact spellbook, if it wasn't randomly made one already  */
	if (otyp == SPE_SECRETS && init && !otmp->oartifact)
	    otmp = mk_artifact(otmp, (aligntyp)A_NONE);
	
	/* track words */
	if (otyp == FIRST_WORD)
	    flags.made_first = TRUE;
	if (otyp == DIVIDING_WORD)
	    flags.made_divide = TRUE;
	if (otyp == NURTURING_WORD)
	    flags.made_life = TRUE;
	if (otyp == WORD_OF_KNOWLEDGE)
	    flags.made_know = TRUE;
	
	fix_object(otmp);
	return(otmp);
}

void
size_and_shape_to_fit(obj, mon)
struct obj *obj;
struct monst *mon;
{
	struct permonst *ptr = mon->data;
	if (Is_dragon_scales(obj)){
		//Fits everything
		return;
	}
	// change shape
	if (is_shirt(obj) || obj->otyp == ELVEN_TOGA){
		//Check that the monster can actually have armor that fits it.
		if(!(ptr->mflagsb&MB_BODYTYPEMASK)){
			return;
		}
		set_obj_shape(obj, ptr->mflagsb);
	}
	else if (is_suit(obj)){
		//Check that the monster can actually have armor that fits it.
		if(!(ptr->mflagsb&MB_BODYTYPEMASK)){
			return;
		}
		set_obj_shape(obj, ptr->mflagsb);
	}
	else if (is_helmet(obj) && !is_hat(obj)){
		//Check that the monster can actually have armor that fits it.
		if(!has_head(ptr) || nohat(ptr)){
			return;
		}
		set_obj_shape(obj, ptr->mflagsb);
	}
	
	// change size (AFTER shape, because this may be aborted during that step.
	obj->objsize = ptr->msize;
	if(ptr->mtyp == PM_BLIBDOOLPOOLP_S_MINDGRAVEN_CHAMPION && is_boots(obj))
		obj->objsize++;
	
	fix_object(obj);
}

void
doMaskStats(mask)
	struct obj *mask;
{
	mask->mp = malloc(sizeof(struct mask_properties));
	if(is_mplayer(&mons[mask->corpsenm])){
		mask->mp->msklevel = rnd(10);
//		mask->mp->mskmonnum = 
		mask->mp->mskrolenum = mask->corpsenm;
//		mask->mp->mskfemale = 
////		mask->mp->mskacurr = malloc(sizeof(struct attribs));
////		mask->mp->mskaexe = malloc(sizeof(struct attribs));
////		mask->mp->mskamask = malloc(sizeof(struct attribs));
//		mask->mp->mskalign = 
		mask->mp->mskluck = rn1(9,-4);
//		mask->mp->mskhp = 
//		mask->mp->mskhpmax = mask->mp->mskhp;
//		mask->mp->msken = 
//		mask->mp->mskenmax = mask->mp->msken;
//		mask->mp->mskexp = 
		mask->mp->mskrexp = mask->mp->msklevel-1;
		mask->mp->mskweapon_slots = 0;
		mask->mp->mskskills_advanced = 0;
		// mask->mp->mskskill_record;
		// mask->mp->mskweapon_skills;
//	} else {
	}
}

/*
 * Start a corpse decay or revive timer.
 * This takes the age of the corpse into consideration as of 3.4.0.
 */
void
start_corpse_timeout(body)
	struct obj *body;
{
	long when; 		/* rot away when this old */
	long corpse_age;	/* age of corpse          */
	int rot_adjust;
	short action;
	long age;
	int chance;
	struct monst *attchmon = 0;

#define TAINT_AGE (50L)		/* age when corpses go bad */
#define TROLL_REVIVE_CHANCE 37	/* 1/37 chance for 50 turns ~ 75% chance */
#define MOLD_REVIVE_CHANCE 23	/*  1/23 chance for 50 turns ~ 90% chance */
#define BASE_MOLDY_CHANCE 19900	/*  1/19900 chance for 200 turns ~ 1% chance */
#define HALF_MOLDY_CHANCE 290	/*  1/290 chance for 200 turns ~ 50% chance */
#define FULL_MOLDY_CHANCE 87	/*  1/87 chance for 200 turns ~ 90% chance */
#define ROT_AGE (250L)		/* age when corpses rot away */

	/* lizards, beholders, and lichen don't rot or revive */
	if (body->corpsenm == PM_LIZARD
		|| body->corpsenm == PM_LICHEN
		|| body->corpsenm == PM_CROW_WINGED_HALF_DRAGON
		|| body->corpsenm == PM_BEHOLDER
		|| body->spe
	) return;
	
	if(get_ox(body, OX_EMON)) attchmon = EMON(body);

	action = ROT_CORPSE;		/* default action: rot away */
	rot_adjust = in_mklev ? 25 : 10;	/* give some variation */
	corpse_age = monstermoves - body->age;
	if (corpse_age > ROT_AGE)
		when = rot_adjust;
	else
		when = ROT_AGE - corpse_age;
	when += (long)(rnz(rot_adjust) - rot_adjust);
	
//	pline("corpse type: %d, %c",mons[body->corpsenm].mlet,def_monsyms[mons[body->corpsenm].mlet]);
	if(is_migo(&mons[body->corpsenm]) || body->corpsenm == PM_DEEP_DWELLER){
		when = when/10 + 1;
	}

	if (is_rider(&mons[body->corpsenm])
		|| (uwep && uwep->oartifact == ART_SINGING_SWORD && uwep->osinging == OSING_LIFE && attchmon && attchmon->mtame)
		) {
		/*
		 * Riders always revive.  They have a 1/3 chance per turn
		 * of reviving after 12 turns.  Always revive by 500.
		 */
//		pline("setting up rider revival for %s", xname(body));
		action = REVIVE_MON;
		for (when = 12L; when < 500L; when++)
		    if (!rn2(3)) break;

	} else if (attchmon && 
		(body->corpsenm == PM_UNDEAD_KNIGHT
		|| body->corpsenm == PM_WARRIOR_OF_SUNLIGHT
		|| body->corpsenm == PM_UNDEAD_MAIDEN
		|| body->corpsenm == PM_KNIGHT_OF_THE_PRINCESS_S_GUARD
		|| body->corpsenm == PM_BLUE_SENTINEL
		|| body->corpsenm == PM_DARKMOON_KNIGHT
		|| body->corpsenm == PM_UNDEAD_REBEL
		|| body->corpsenm == PM_PARDONER
		|| body->corpsenm == PM_OCCULTIST
		)
		&& !body->norevive
	) {
//		pline("setting up undead revival for %s", xname(body));
		attchmon->mclone = 1;
		action = REVIVE_MON;
		when = rn2(TAINT_AGE)+2;
	} else if (attchmon
	 && (has_template(attchmon, ZOMBIFIED))
	 && !body->norevive
	) {
		// if(wizard)
			// pline("checking for zombie revival for %s", xname(body));
		attchmon->mclone = 1;
		for (age = 2; age <= ROT_AGE; age++)
		    if (!rn2(HALF_MOLDY_CHANCE)) {	/* zombie revives */
			action = REVIVE_MON;
			when = age;
			break;
		    }
	} else if (mons[body->corpsenm].mlet == S_TROLL && !body->norevive) {
//		pline("setting up troll revival for %s", xname(body));
		for (age = 2; age <= TAINT_AGE; age++)
		    if (!rn2(TROLL_REVIVE_CHANCE)) {	/* troll revives */
			action = REVIVE_MON;
			when = age;
			break;
		    }
	} else if (is_fungus(&mons[body->corpsenm]) && 
			  !is_migo(&mons[body->corpsenm])) {
		/* Fungi come back with a vengeance - if you don't eat it or
		 * destroy it,  any live cells will quickly use the dead ones
		 * as food and come back.
		 */
//		pline("setting up fungus revival for %s", xname(body));
		for (age = 2; age <= TAINT_AGE; age++)
		    if (!rn2(MOLD_REVIVE_CHANCE)) {    /* mold revives */
			action = REVIVE_MON;
			when = age;
			break;
		    }
	}
	
	if (action == ROT_CORPSE && !acidic(&mons[body->corpsenm])){
		/* Corpses get moldy
		 */
		chance = ((Is_zuggtmoy_level(&u.uz) && flags.spore_level) || (attchmon && attchmon->brainblooms)) ? FULL_MOLDY_CHANCE : 
				 (Is_zuggtmoy_level(&u.uz) || flags.spore_level) ? HALF_MOLDY_CHANCE : 
				 BASE_MOLDY_CHANCE;
		for (age = TAINT_AGE + 1; age <= ROT_AGE; age++)
			if (!rn2(chance)) {    /* "revives" as a random s_fungus */
				action = MOLDY_CORPSE;
				when = age;
				break;
			}
	}
	chance = (Is_juiblex_level(&u.uz) && flags.slime_level) ? FULL_MOLDY_CHANCE : 
			 (Is_juiblex_level(&u.uz) || flags.slime_level) ? HALF_MOLDY_CHANCE : 
			 0;
	if(action == ROT_CORPSE && chance){
		for (age = TAINT_AGE + 1; age <= ROT_AGE; age++)
			if (!rn2(chance)) {    /* "revives" as a random s_fungus */
				action = SLIMY_CORPSE;
				when = age;
				break;
			}
	}
	chance = (flags.walky_level) ? TROLL_REVIVE_CHANCE : 
			 (attchmon && (attchmon->zombify || attchmon->mspores)) ? FULL_MOLDY_CHANCE : 
			 (Is_night_level(&u.uz)) ? HALF_MOLDY_CHANCE : 
			 0;
	if(action == ROT_CORPSE && chance){
		for (age = TAINT_AGE + 1; age <= ROT_AGE; age++)
			if (!rn2(chance)) {    /* "revives" as a zombie */
				action = ZOMBIE_CORPSE;
				when = age;
				break;
			}
	}
	chance = (flags.yello_level || (Role_if(PM_MADMAN) && In_quest(&u.uz) && !mvitals[PM_STRANGER].died))
			 ? TROLL_REVIVE_CHANCE : 
			 0;
	if(action == ROT_CORPSE && chance){
		for (age = TAINT_AGE + 1; age <= ROT_AGE; age++)
			if (!rn2(chance)) {
				action = YELLOW_CORPSE;
				when = age;
				break;
			}
	}
	chance = (Is_orcus_level(&u.uz) && flags.shade_level) ? FULL_MOLDY_CHANCE : 
			 (Is_orcus_level(&u.uz) || flags.shade_level) ? HALF_MOLDY_CHANCE : 
			 0;
	if(action == ROT_CORPSE && chance) {
		chance = FULL_MOLDY_CHANCE;
		for (age = TAINT_AGE + 1; age <= ROT_AGE; age++)
			if (!rn2(chance)) {    /* "revives" as a random s_fungus */
				action = SHADY_CORPSE;
				when = age;
				break;
			}
	}
	
	if (body->norevive) body->norevive = 0;
//	pline("Starting timer %d on %s", action, xname(body));
	(void) start_timer(when, TIMER_OBJECT, action, (genericptr_t)body);
}

void
bless(otmp)
register struct obj *otmp;
{
#ifdef GOLDOBJ
	if (otmp->oclass == COIN_CLASS) return;
#endif
	otmp->cursed = 0;
	otmp->blessed = 1;
	if (carried(otmp) && confers_luck(otmp))
	    set_moreluck();
	else if (otmp->otyp == BAG_OF_HOLDING)
	    otmp->owt = weight(otmp);
	else if ((artifact_light(otmp)||arti_light(otmp)) && otmp->lamplit)
		begin_burn(otmp);
	else if (otmp->otyp == FIGURINE && otmp->timed)
		(void) stop_timer(FIG_TRANSFORM, otmp->timed);
	return;
}

void
unbless(otmp)
register struct obj *otmp;
{
	otmp->blessed = 0;
	if (carried(otmp) && confers_luck(otmp))
	    set_moreluck();
	else if (otmp->otyp == BAG_OF_HOLDING)
	    otmp->owt = weight(otmp);
	else if ((artifact_light(otmp)||arti_light(otmp)) && otmp->lamplit)
		begin_burn(otmp);

}

void
curse(otmp)
register struct obj *otmp;
{
#ifdef GOLDOBJ
	if (otmp->oclass == COIN_CLASS) return;
#endif
	otmp->blessed = 0;
	otmp->cursed = 1;
	/* welded two-handed weapon interferes with some armor removal */
	if (otmp == uwep && bimanual(uwep,youracedata)) reset_remarm();
	/* rules at top of wield.c state that twoweapon cannot be done
	   with cursed alternate weapon */
	if (otmp == uswapwep && u.twoweap && !Weldproof)
	    drop_uswapwep();
	if(otmp->otyp == STRAITJACKET){
		if (otmp == uarm){
			struct obj *o;
			reset_remarm();
			if(u.twoweap && uswapwep) drop_uswapwep();
			if(uwep){
				o = uwep;
				setuwep((struct obj *)0);
				dropx(o);
			}
		}
		else if(otmp->where == OBJ_MINVENT && otmp->owornmask&W_ARM){
			struct monst *owner = otmp->ocarry;
			struct obj *dropit;
			if((dropit = MON_WEP(owner))){
				obj_extract_and_unequip_self(dropit);
				place_object(dropit, owner->mx, owner->my);
			}
			if((dropit = MON_SWEP(owner))){
				obj_extract_and_unequip_self(dropit);
				place_object(dropit, owner->mx, owner->my);
			}
		}
	}
	/* some cursed items need immediate updating */
	if (carried(otmp) && confers_luck(otmp))
	    set_moreluck();
	else if (otmp->otyp == BAG_OF_HOLDING)
	    otmp->owt = weight(otmp);
	else if ((artifact_light(otmp)||arti_light(otmp)) && otmp->lamplit) 
		begin_burn(otmp);
	else if (otmp->otyp == FIGURINE) {
		if (otmp->corpsenm != NON_PM
		    && !dead_species(otmp->corpsenm,TRUE)
		    && (carried(otmp) || mcarried(otmp)))
			attach_fig_transform_timeout(otmp);
	}
	return;
}

void
uncurse(otmp)
register struct obj *otmp;
{
	otmp->cursed = 0;
	if (carried(otmp) && confers_luck(otmp))
	    set_moreluck();
	else if (otmp->otyp == BAG_OF_HOLDING)
	    otmp->owt = weight(otmp);
	else if (otmp->otyp == FIGURINE && otmp->timed)
	    (void) stop_timer(FIG_TRANSFORM, otmp->timed);
	else if ((artifact_light(otmp)||arti_light(otmp)) && otmp->lamplit)
		begin_burn(otmp);

	return;
}

#endif /* OVLB */
#ifdef OVL1

void
blessorcurse(otmp, chance)
register struct obj *otmp;
register int chance;
{
	if(otmp->blessed || otmp->cursed) return;

	if(!rn2(chance)) {
	    if(!rn2(2)) {
		curse(otmp);
	    } else {
		bless(otmp);
	    }
	}
	return;
}

#endif /* OVL1 */
#ifdef OVLB

int
bcsign(otmp)
register struct obj *otmp;
{
	return(!!otmp->blessed - !!otmp->cursed);
}

#endif /* OVLB */
#ifdef OVL0
/* set the size of an object, making sure the object is proper */
void
set_obj_size(obj, size)
struct obj* obj;
int size;
{
	/* check bad cases */
	if (!obj) {
		impossible("set_obj_size called without object");
		return;
	}
	/* set quantity */
	obj->objsize = size;
	/* update weight */
	fix_object(obj);
	return;
}

/* set the quantity of an object, making sure to update its weight */
void
set_obj_quan(obj, quan)
struct obj* obj;
int quan;
{
	/* check bad cases */
	if (!obj) {
		impossible("set_obj_quan called without object");
		return;
	}
	if (!objects[obj->otyp].oc_merge && quan != 1) {
		impossible("set_obj_quan called for singular quantity object with quan = %d", quan);
		return;
	}
	/* set quantity */
	obj->quan = quan;
	/* update weight */
	fix_object(obj);
	return;
}

/* Return the appropriate random material list for a given object,
 * or NULL if there isn't an appropriate list. */
const struct icp*
material_list(obj)
struct obj* obj;
{
    unsigned short otyp = obj->otyp;
    int default_material = objects[otyp].oc_material;

	if(otyp == find_pcloth())
		return NULL;
    /* Cases for specific object types. */
	switch (otyp) {
	/* Special exceptions to the whole randomized materials system - where
	 * we ALWAYS want an object to use its base material regardless of
	 * other cases in this function - go here.
	 * Return NULL so that init_obj_material and valid_obj_material both
	 * work properly. */
	case BULLWHIP:
	case TORCH:
	case MAGIC_TORCH:
	case SHADOWLANDER_S_TORCH:
	case SUNROD:
	case WORM_TOOTH:
	case MUMMY_WRAPPING:
	case PRAYER_WARDED_WRAPPING:
	case ELVEN_CLOAK:
	case DROVEN_CLOAK:
	case OILSKIN_CLOAK:
	case ROBE:
	case WHITE_FACELESS_ROBE:
	case BLACK_FACELESS_ROBE:
	case SMOKY_VIOLET_FACELESS_ROBE:
	case CRYSKNIFE:
	case LEATHER_HELM:
	case LEATHER_ARMOR:
	case SHEMAGH:
	case STUDDED_LEATHER_ARMOR:
	case STUDDED_LEATHER_CLOAK:
	case BARNACLE_ARMOR:
	case HEALER_UNIFORM:
	case NOBLE_S_DRESS:
	case CONSORT_S_SUIT:
	case GENTLEMAN_S_SUIT:
	case PLAIN_DRESS:
	case T_SHIRT:
	case HAWAIIAN_SHIRT:
	case HAWAIIAN_SHORTS:
	case STRIPED_SHIRT:
	case ICHCAHUIPILLI:
	case RUFFLED_SHIRT:
	case VICTORIAN_UNDERWEAR:
	case YA:
	case ORIHALCYON_GAUNTLETS:
	case HARMONIUM_HELM:
	case HARMONIUM_PLATE:
	case HARMONIUM_SCALE_MAIL:
	case HARMONIUM_GAUNTLETS:
	case HARMONIUM_BOOTS:
	case ELVEN_MITHRIL_COAT:
	case DWARVISH_MITHRIL_COAT:
	case BROKEN_ANDROID:
	case BROKEN_GYNOID:
	case WRITING_DESK:
	case BLASTER_BOLT:
	case HEAVY_BLASTER_BOLT:
	case LASER_BEAM:
	case CARCOSAN_BOLT:
		return NULL;
		/* Any other cases for specific object types go here. */
	case SARCOPHAGUS:
	case SHIELD_OF_REFLECTION:
		return shiny_materials;
	case BOW:
	case ELVEN_BOW:
	case ORCISH_BOW:
	case YUMI:
	case BOOMERANG: /* wooden base, similar shape */
		return bow_materials;
	case CHEST:
	case BOX:
		return wood_materials;
	case SKELETON_KEY:
	case LOCK_PICK:
	case TIN_OPENER:
		return metal_materials;
	case BELL:
	case BUGLE:
	case LANTERN:
	case OIL_LAMP:
	case MAGIC_LAMP:
	case MAGIC_WHISTLE:
	case FLUTE:
	case MAGIC_FLUTE:
	case HARP:
	case MAGIC_HARP:
	case KHAKKHARA:
		return resonant_materials;
	case TOOLED_HORN:
	case FIRE_HORN:
	case FROST_HORN:
	case HORN_OF_PLENTY:
		return horn_materials;
	case EILISTRAN_ARMOR:
		return eli_materials;
	case SOUL_LENS:
		return lens_materials;
	case CANE:
	case CHURCH_BLADE:
	case CHURCH_SHEATH:
		return cane_materials;
	case CHIKAGE:
		return chikage_materials;
	case CHURCH_HAMMER:
	case CHURCH_BRICK:
		return brick_materials;
	case WHIP_SAW:
	case HUNTER_S_SHORTSWORD:
	case HUNTER_S_LONGSWORD:
	case SHANTA_PATA:
	case TWINGUN_SHANTA:
		return special_materials;
	case DEVIL_FIST:
	case DEMON_CLAW:
		return fiend_materials;
	default:
		break;
	}

    /* Otherwise, select an appropriate list, or return NULL if no appropriate
     * list exists. */
	if (otyp == find_gcirclet()) {
		return shiny_materials;
	}
	else if (is_firearm(obj) && (default_material == IRON || default_material == METAL)) {
		return resonant_materials;	// guns and bells are surprisingly similar (see: Wheel of Time)
	}
	else if (obj->oclass == WEAPON_CLASS && objects[otyp].oc_skill == -P_FIREARM) {
		return NULL;	// do not attempt to randomize the material of bullets/shells/grenades
	}
    else if (is_elven_obj(obj) && default_material == MITHRIL) {
        return high_elven_materials;
    }
	else if (is_elven_obj(obj) && default_material != CLOTH) {
		return elven_materials;
	}
    else if (is_dwarvish_obj(obj) && default_material != CLOTH) {
        return dwarvish_materials;
    }
	else if (is_droven_weapon(obj)) {
		return droven_materials;
	}
	else if (is_orcish_obj(obj) && default_material != CLOTH) {
		return orcish_materials;
	}
    else if (obj->oclass == AMULET_CLASS && otyp != AMULET_OF_YENDOR
             && otyp != FAKE_AMULET_OF_YENDOR) {
        /* could use metal_materials too */
        return shiny_materials;
    }
    else if (obj->oclass == WEAPON_CLASS || is_weptool(obj)
             || obj->oclass == ARMOR_CLASS) {
        if (default_material == IRON || default_material == METAL) {
            return metal_materials;
        }
        else if (default_material == WOOD) {
            return wood_materials;
        }
        else if (default_material == CLOTH) {
            return cloth_materials;
        }
		else if (default_material == LEATHER) {
            return leather_materials;
        }
    }
    return NULL;
}

/* Tries to set obj's material to mat
 * Checks material list for obj.
 * Used by wishing code
 */
void
maybe_set_material(obj, mat)
struct obj *obj;
int mat;
{
	const struct icp* random_mat_list;

	/* check that the item isn't currently of mat */
	if (obj->obj_material == mat)
		return;	// already done

	/* randomized materials */
	random_mat_list = material_list(obj);
	/* check that the object can have a random material */
	if (!random_mat_list)
		return;

	int i = 1000;
	while (i > 0 && random_mat_list->iclass != mat) {
		if(random_mat_list->iprob == 0){
			impossible("maybe_set_material random_mat_list out-of-range.");
			break;
		}
		i -= random_mat_list->iprob;
		random_mat_list++;
	}
	if (random_mat_list->iclass == mat)
		set_material_gm(obj, random_mat_list->iclass);
}


/* Some variables of objects are special because of its material
 * Called in init_obj_material to initialize objects, and in
 * set_material to handle changes to material
 * 
 */
void
handle_material_specials(obj, oldmat, newmat)
struct obj* obj;
int oldmat, newmat;
{
	/* start the timer for shadowsteel objects */
	/* is_evaporable() is no good because we also need to ask using the previous material state */
	if (newmat == SHADOWSTEEL && oldmat != SHADOWSTEEL){
		start_timer(1, TIMER_OBJECT,
			LIGHT_DAMAGE, (genericptr_t)obj);
	}
	else if (oldmat == SHADOWSTEEL) {/* Or turn it off, if the object used to be made of shadowsteel.*/
		stop_timer(LIGHT_DAMAGE, obj->timed);
	}
	/* set random gemstone type for valid gemstone objects */
	if (newmat == GEMSTONE && obj->oclass != GEM_CLASS && (obj->sub_material < MAGICITE_CRYSTAL || obj->sub_material > LAST_GEM || oldmat != GEMSTONE)) {
		if(obj->oartifact == ART_JIN_GANG_ZUO){
			set_submat(obj, DIAMOND);
		}
		else do{
			set_submat(obj, MAGICITE_CRYSTAL + rn2(LAST_GEM - MAGICITE_CRYSTAL + 1));
		} while (obj->sub_material == OBSIDIAN);
	}
	else if (oldmat == GEMSTONE && newmat != GEMSTONE && obj->oclass != GEM_CLASS) {
		set_submat(obj, 0);	/* and reset if changing away from gemstone*/
	}
}

/* Initialize the material field of an object
 * Called on object creation, and can be called during
 *   wishing to re-randomize the material of an object
 */
void
init_obj_material(obj)
struct obj* obj;
{
	int otyp = obj->otyp;
	const struct icp* random_mat_list;

	/* start by setting the material to its default */
	obj->obj_material = objects[obj->otyp].oc_material;

	/* initialize special properties of materials like shadowsteel timer and gemstone type */
	handle_material_specials(obj, 0, obj->obj_material);

	/* randomized materials */
	random_mat_list = material_list(obj);
	if (random_mat_list) {
		int i = rnd(1000);
#ifdef REINCARNATION
		if(Is_rogue_level(&u.uz))
			i = 1;//No fancy materials
#endif
		while (i > 0) {
			if(random_mat_list->iprob == 0){
				impossible("init_obj_material random_mat_list out-of-range.");
				break;
			}
			if (i <= random_mat_list->iprob)
				break;
			i -= random_mat_list->iprob;
			random_mat_list++;
		}
		if (random_mat_list->iclass) /* a 0 indicates to use default material */
			set_material_gm(obj, random_mat_list->iclass);
	}
	return;
}

/* attempts to randomly give obj a rare material */
void
rand_interesting_obj_material(obj)
struct obj * obj;
{
	const struct icp* random_mat_list;
	int tries = 0;
	int original_mat;

	init_obj_material(obj);
	original_mat = obj->obj_material;

	/* randomized materials */
	do {
		random_mat_list = material_list(obj);
		if (random_mat_list) {
			int i = rnd(1000);
			while (i > 0) {
				if(random_mat_list->iprob == 0){
					impossible("rand_interesting_obj_material random_mat_list out-of-range.");
					break;
				}
				if (i <= random_mat_list->iprob)
					break;
				i -= random_mat_list->iprob;
				random_mat_list++;
			}
			if (random_mat_list->iclass) /* a 0 indicates to use default material */
				set_material_gm(obj, random_mat_list->iclass);
		}
	} while (obj->obj_material == original_mat && ++tries<40);
}

/* "Game Master"-facing set material function.
 *  Should do the absolute minimum to make sure that obj is not in an inconsistent state after mat is set.
 *  In particular, it should never change the base object type.
 */

void
set_material_gm(obj, mat)
struct obj *obj;
int mat;
{
	int oldmat = obj->obj_material;

	if(mat == obj->obj_material) return; //Already done!
	
	obj->obj_material = mat; //Set material
	
	if(obj->oeroded && !is_rustprone(obj) && !is_flammable(obj) && !is_evaporable(obj))
		obj->oeroded = 0;
	if(obj->oeroded2 && !is_rottable(obj) && !is_corrodeable(obj))
		obj->oeroded2 = 0;
	
	/* cover special properties of materials like shadowsteel timer and gemstone type */
	handle_material_specials(obj, oldmat, obj->obj_material);

	/* reset the color if needed */
	set_object_color(obj);
	
	fix_object(obj);
}

/* Player-facing (or simulation-facing) set material function.
 *  Intended to be called when objects change material during the course of play.
 *  May make whatever changes to the base item type it sees fit.
 *  May even refuse to actually set the material, currently in the case of spellbooks.
 */

void
set_material(obj, mat)
struct obj *obj;
int mat;
{
	struct monst *owner = 0;
	long mask = 0;
	int oldmat = obj->obj_material;

	if(mat == obj->obj_material) return; //Already done!
	if(oldmat == MERCURIAL){
		start_timer(d(8,8), TIMER_OBJECT,
					REVERT_MERC, (genericptr_t)obj);
	}


	if (obj->where == OBJ_INVENT) {
		owner = &youmonst;
		if ((mask = obj->owornmask))
			setnotworn(obj);
	}
	if (obj->where == OBJ_MINVENT) {
		owner = obj->ocarry;
		if ((mask = obj->owornmask))
			update_mon_intrinsics(owner, obj, FALSE, TRUE);
	}
	/* change otyp to be appropriate
	 * does not set material
	 */
	switch(obj->otyp){
		/* arrows */
		case ARROW:
		case GOLDEN_ARROW:
		case SILVER_ARROW:
			if(mat == GOLD) obj->otyp = GOLDEN_ARROW;
			else if (mat == SILVER)		obj->otyp = SILVER_ARROW;
			else obj->otyp = ARROW;
		break;
		/* bullets */
		case BULLET:
		case SILVER_BULLET:
		case BLOOD_BULLET:
		case BLOOD_SPEAR:
			if (mat == SILVER)			obj->otyp = SILVER_BULLET;
			else if (mat == HEMARGYOS)	obj->otyp = BLOOD_BULLET;
			else						obj->otyp = BULLET;
		break;
		case CRYSKNIFE:
			if(mat > CHITIN && mat != MINERAL)
				obj->otyp = KNIFE;
		break;
		/* elven helmets */
		case ELVEN_HELM:
		case HIGH_ELVEN_HELM:
			if		(mat >= IRON)		obj->otyp = HIGH_ELVEN_HELM;
			else						obj->otyp = ELVEN_HELM;
		break;
		/* helmets */
		case HELMET:
		case LEATHER_HELM:
		case ARCHAIC_HELM:
		case DROVEN_HELM:
//		case PLASTEEL_HELM:				/* has a unique function of shape -- needs a generic version? */
//		case CRYSTAL_HELM:				/* has a unique function of shape -- needs a generic version? */
			if (mat == LEATHER)			obj->otyp = LEATHER_HELM;
			else if (mat == SHADOWSTEEL)obj->otyp = DROVEN_HELM;
			else if (obj->otyp != HELMET
				&& obj->otyp != ARCHAIC_HELM
			)
										obj->otyp = HELMET;
			break;
		/* gauntlets + gloves */
		case CRYSTAL_GAUNTLETS:			/* irreversible, glass */
			if		(mat == GLASS		
				||   mat == OBSIDIAN_MT	
				||   mat == GEMSTONE)	break;	// remain as crystal gauntlets
			else;// fall through to general gauntlets
		case GLOVES:
		case GAUNTLETS:
		case ARCHAIC_GAUNTLETS:
		case PLASTEEL_GAUNTLETS:		/* irreversible, plastic */
		case ORIHALCYON_GAUNTLETS:		/* irreversible, metal */
			if		(mat == DRAGON_HIDE)obj->otyp = (is_hard(obj) ? GAUNTLETS : GLOVES);
			else if	(mat == LEATHER
				||   mat == CLOTH)		obj->otyp = GLOVES;
			else if (obj->otyp != GAUNTLETS
				&& obj->otyp != ARCHAIC_GAUNTLETS
			)
										obj->otyp = GAUNTLETS;
			break;
		/* boots */
		case CRYSTAL_BOOTS:				/* irreversible, glass */
			if		(mat == GLASS		
				||   mat == OBSIDIAN_MT	
				||   mat == GEMSTONE)	break;	// remain as crystal boots
			else;
				// fall through to general boots
		case ARMORED_BOOTS:
		case ARCHAIC_BOOTS:
		case HIGH_BOOTS:
		case PLASTEEL_BOOTS:			/* irreversible, plastic */
			if(mat == DRAGON_HIDE)
				obj->otyp = (is_hard(obj) ? ARMORED_BOOTS : HIGH_BOOTS);
			else if(hard_mat(mat) != is_hard(obj)){
				if(hard_mat(mat)){
					if(obj->otyp != ARMORED_BOOTS && 
						obj->otyp != ARCHAIC_BOOTS
					) obj->otyp = ARMORED_BOOTS;
				} else
					obj->otyp = HIGH_BOOTS;
			}
		break;
		/* shoes */
		case SHOES:
		case LOW_BOOTS:
			if		(mat == DRAGON_HIDE)obj->otyp = (is_hard(obj) ? SHOES : LOW_BOOTS);
			else if (mat >= WOOD)		obj->otyp = SHOES;
			else						obj->otyp = LOW_BOOTS;
			break;
		/* chain mail */
		case CHAIN_MAIL:
		case DROVEN_CHAIN_MAIL:			/* irreversible, shadowsteel */
		case DWARVISH_MITHRIL_COAT:		/* irreversible, mithril */
		case ELVEN_MITHRIL_COAT:		/* irreversible, mithril */
			obj->otyp = CHAIN_MAIL;
		break;
		/* scale mail */
//		dragon scale mail commented out for now -- metal dragons when?
//		case GRAY_DRAGON_SCALE_MAIL:
//		case SILVER_DRAGON_SCALE_MAIL:
//		case SHIMMERING_DRAGON_SCALE_MAIL:
//		case RED_DRAGON_SCALE_MAIL:
//		case WHITE_DRAGON_SCALE_MAIL:
//		case ORANGE_DRAGON_SCALE_MAIL:
//		case BLACK_DRAGON_SCALE_MAIL:
//		case BLUE_DRAGON_SCALE_MAIL:
//		case GREEN_DRAGON_SCALE_MAIL:
//		case YELLOW_DRAGON_SCALE_MAIL:
//			if		(mat == DRAGON_HIDE)break;	// remain as dragon scale mail
//			else;
//				// fall through
		case SCALE_MAIL:
		case STUDDED_LEATHER_ARMOR:		/* irreversible, leather */
		case LEATHER_ARMOR:				/* irreversible, leather */
			obj->otyp = SCALE_MAIL;
		break;
		/* plate mail */
		case CRYSTAL_PLATE_MAIL:		/* irreversible, glass */
			if		(mat == GLASS		
				||   mat == OBSIDIAN_MT	
				||   mat == GEMSTONE)	break;	// remain as crystal plate
			else;// fall through to general plate mail
		case PLATE_MAIL:
		case PLASTEEL_ARMOR:			/* irreversible, plastic */
		case DROVEN_PLATE_MAIL:			/* irreversible, shadowsteel */
			obj->otyp = PLATE_MAIL;
		break;
		/* long swords */
		case CRYSTAL_SWORD:				/* irreversible, glass */
			if		(mat == GLASS		
				||   mat == OBSIDIAN_MT	
				||   mat == GEMSTONE)	break;	// remain as crystal sword
			else;// fall through to general long sword
		case LONG_SWORD:
										obj->otyp = LONG_SWORD;
		break;
		// case CRYSTAL_BALL:
			// obj->otyp = ;
		// break;
		case SILVER_SLINGSTONE:
		case ROCK:
			if(mat == SILVER) obj->otyp = SILVER_SLINGSTONE;
			else if (mat == GLASS)		obj->otyp = LAST_GEM + rnd(9);
			else if (mat == GEMSTONE)	obj->otyp = MAGICITE_CRYSTAL + rn2(LAST_GEM - MAGICITE_CRYSTAL + 1);
			else						obj->otyp = ROCK;
		break;
		case WHITE_VIBROSPEAR:
			if(mat == GOLD) obj->otyp = GOLD_BLADED_VIBROSPEAR;
		break;
		case WHITE_VIBROSWORD:
			if(mat == GOLD) obj->otyp = GOLD_BLADED_VIBROSWORD;
		break;
		case WHITE_VIBROZANBATO:
			if(mat == GOLD) obj->otyp = GOLD_BLADED_VIBROZANBATO;
		break;
		case GOLD_BLADED_VIBROSPEAR:
			if(mat != GOLD) obj->otyp = WHITE_VIBROSPEAR;
		break;
		case GOLD_BLADED_VIBROSWORD:
			if(mat != GOLD) obj->otyp = WHITE_VIBROSWORD;
		break;
		case GOLD_BLADED_VIBROZANBATO:
			if(mat != GOLD) obj->otyp = WHITE_VIBROZANBATO;
		break;
		// case BALL:
			// obj->otyp = ;
		// break;
		// case CHAIN:
			// obj->otyp = ;
		// break;
		// case BANDS:
			// obj->otyp = ;
		// break;
	}
	/* switch based on class
	 * this will actually set the material
	 *   some exceptions exist where material purposefully not changed
	 */
	switch(objects[obj->otyp].oc_class){
		case POTION_CLASS:
			switch(mat){
				case GOLD:
					if(find_golden_potion()) {
						obj->otyp = find_golden_potion();
					}
					break;
			}
		break;
		case SPBOOK_CLASS:
			switch(mat){
				case CLOTH:
					if (find_cloth_book()) {
						obj->otyp = find_cloth_book();
					}
					break;
				case LEATHER:
					if (find_leather_book()) {
						obj->otyp = find_leather_book();
					}
					break;
				case COPPER:
					if (find_bronze_book()) {
						obj->otyp = find_bronze_book();
					}
					break;
				case SILVER:
					if (find_silver_book()) {
						obj->otyp = find_silver_book();
					}
					break;
				case GOLD:
					if (find_gold_book()) {
						obj->otyp = find_gold_book();
					}
					break;
			}
		break;
		case WAND_CLASS:
			obj->otyp = matWand(obj->otyp, mat);
			if(obj->otyp == WAN_WISHING)
				obj->spe /= 5;
			if(!(obj->recharged)) obj->recharged = 1;
			set_material_gm(obj, mat); //Set the material using the minimal set material function
		break;
		case GEM_CLASS:
			if(mat != GLASS && mat != GEMSTONE){
				if(mat == SILVER) obj->otyp = SILVER_SLINGSTONE;
				else obj->otyp = ROCK;
			}
			else if (mat == GEMSTONE && oldmat == GLASS)
				obj->otyp = MAGICITE_CRYSTAL + rn2(LAST_GEM - MAGICITE_CRYSTAL + 1);
			else if (mat == GLASS && oldmat == GEMSTONE)
				obj->otyp = LAST_GEM + rnd(9);
			set_material_gm(obj, mat); //Set the material using the minimal set material function
		break;
		default:
			set_material_gm(obj, mat); //Set the material using the minimal set material function
		break;
	}
	//Silver bell should resist
	
	/* re-equip */
	if (mask && owner) {
		if(owner == &youmonst){
			setworn(obj, mask);
		} else {
			owner->misc_worn_check |= mask;
			obj->owornmask |= mask;
			update_mon_intrinsics(owner, obj, TRUE, TRUE);
		}
	}
}


/*
 *  Calculate the weight of the given object.  This will recursively follow
 *  and calculate the weight of any containers.
 *
 *  Note:  It is possible to end up with an incorrect weight if some part
 *	   of the code messes with a contained object and doesn't update the
 *	   container's weight.
 */
int
weight(obj)
register struct obj *obj;
{
	int wt = objects[obj->otyp].oc_weight;
	int base_mat = (obj->oartifact && artilist[obj->oartifact].material != MT_DEFAULT && artilist[obj->oartifact].weight != WT_DEFAULT) ? artilist[obj->oartifact].material : objects[obj->otyp].oc_material;

	if (obj->otyp == MAGIC_CHEST && obj->obolted) return 99999;	/* impossibly heavy */

	if (obj->oartifact)
		wt = artifact_weight(obj);
	if(obj->otyp == INGOT){
		return obj->quan * wt;
	}
	else if(obj->otyp == CANE || obj->otyp == WHIP_SAW){
		int otyp_alt = obj->otyp == CANE ? WHIP_SAW : CANE;
		int base_mat_alt = (obj->oartifact && artilist[obj->oartifact].material != MT_DEFAULT && artilist[obj->oartifact].weight != WT_DEFAULT) ? artilist[obj->oartifact].material : objects[otyp_alt].oc_material;

		wt = ((wt * materials[obj->obj_material].density / materials[base_mat].density) + (wt * materials[obj->ovar1_alt_mat].density / materials[base_mat_alt].density))/2;
	}
	else if(obj->otyp == CHIKAGE && obj->obj_material == HEMARGYOS){
		if(obj->ovar1_alt_mat != base_mat)
			wt = wt * materials[obj->ovar1_alt_mat].density / materials[base_mat].density;
	}
	else if(obj->obj_material != base_mat) {
		/* do not apply this to artifacts; those are handled in artifact_weight() */
		wt = wt * materials[obj->obj_material].density / materials[base_mat].density;
	}
	
	if(obj->otyp == MOON_AXE && obj->oartifact != ART_SCEPTRE_OF_LOLTH){
		if(obj->ovar1_moonPhase == HUNTING_MOON) wt =  wt/3;
		else if(obj->ovar1_moonPhase) wt =  wt/4*obj->ovar1_moonPhase;
		else wt = wt/4;
	}

	if(obj->objsize != MZ_MEDIUM){
		int difsize = obj->objsize - MZ_MEDIUM;
		if(difsize > 0){
			difsize++;
			if(obj->oclass == ARMOR_CLASS || obj->oclass == WEAPON_CLASS) wt = wt*difsize;
			else wt = wt*difsize*difsize;
		} else {
			difsize = abs(difsize)+1;
			if(obj->oclass == ARMOR_CLASS || obj->oclass == WEAPON_CLASS) wt = wt/(difsize);
			else wt = wt/(difsize*difsize);
			if (wt < 1 && wt != artifact_weight(obj)) wt = 1;
		}
	}
	
	if ((Is_real_container(obj) && obj->otyp != MAGIC_CHEST) && obj->spe){ /* Schroedinger's Cat */
		if(obj->spe == 1){
			wt += mons[PM_HOUSECAT].cwt;
		}else if(obj->spe == 4){
			wt += mons[PM_VAMPIRE].cwt;
		}else if(obj->spe == 5){
			wt += mons[PM_NITOCRIS].cwt;
		}else if(obj->spe == 9){
			if(urole.neminum == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH)
				wt += mons[PM_PRIESTESS_OF_GHAUNADAUR].cwt;
			else
				wt += mons[PM_VAMPIRE_LADY].cwt;
		}
	}
	if ((Is_container(obj) && obj->otyp != MAGIC_CHEST && obj->oartifact != ART_TREASURY_OF_PROTEUS)
		|| obj->otyp == STATUE
		|| obj->otyp == CHURCH_BLADE
		|| obj->otyp == CHURCH_HAMMER
	) {
		struct obj *contents;
		register int cwt = 0;

		if (obj->otyp == STATUE && obj->corpsenm >= LOW_PM)
		    wt = (int)obj->quan *
			 ((int)mons[obj->corpsenm].cwt * 3 / 2);

		if (obj->otyp == FOSSIL && obj->corpsenm >= LOW_PM)
		    wt = (int)obj->quan *
			 ((int)mons[obj->corpsenm].cwt * 1 / 2);

		for(contents=obj->cobj; contents; contents=contents->nobj)
			cwt += weight(contents);
		/*
		 *  The weight of bags of holding is calculated as the weight
		 *  of the bag plus the weight of the bag's contents modified
		 *  as follows:
		 *
		 *	Bag status	Weight of contents
		 *	----------	------------------
		 *	cursed			2x
		 *	blessed			x/4 + 1
		 *	otherwise		x/2 + 1
		 *
		 *  The macro DELTA_CWT in pickup.c also implements these
		 *  weight equations.
		 *
		 *  Note:  The above checks are performed in the given order.
		 *	   this means that if an object is both blessed and
		 *	   cursed (not supposed to happen), it will be treated
		 *	   as cursed.
		 */
		if (obj->oartifact == ART_WALLET_OF_PERSEUS)
			cwt = obj->cursed ? (cwt * 4) : (1 + (cwt / (obj->blessed ? 6 : 3)));
		else if (obj->otyp == BAG_OF_HOLDING)
		    cwt = obj->cursed ? (cwt * 2) : (1 + (cwt / (obj->blessed ? 4 : 2)));

		return wt + cwt;
	}
	if (obj->otyp == CORPSE && obj->corpsenm >= LOW_PM) {
		long long_wt = obj->quan * (long) mons[obj->corpsenm].cwt;

		wt = (long_wt > LARGEST_INT) ? LARGEST_INT : (int)long_wt;
		if (obj->oeaten) wt = eaten_stat(wt, obj);
		return wt;
	} else if (obj->oclass == FOOD_CLASS && obj->oeaten) {
		return eaten_stat((int)obj->quan * wt, obj);
	} else if (obj->oclass == COIN_CLASS)
		return (int)((obj->quan + 50L) / 100L);
	else if (obj->otyp == BALL && obj->owt != 0)
		return((int)(obj->owt));	/* kludge for "very" heavy iron ball */
	return((wt || obj->oartifact) ? wt*(int)obj->quan : ((int)obj->quan + 1)>>1);
}

static int treefruits[] = {APPLE,ORANGE,PEAR,BANANA,EUCALYPTUS_LEAF};

struct obj *
rnd_treefruit_at(x,y)
int x, y;
{
	struct obj *otmp;
	if(Is_zuggtmoy_level(&u.uz)){
		otmp = mksobj_at(SLIME_MOLD, x, y,NO_MKOBJ_FLAGS);
		otmp->spe = fruitadd("slime mold");
		return otmp;
	} else if(In_quest(&u.uz) && (Race_if(PM_DROW) || (Race_if(PM_DWARF) && Role_if(PM_NOBLEMAN)))){
		int chance = rn2(50);
		if(chance < 20){
			otmp = mksobj_at(EGG, x, y,NO_MKOBJ_FLAGS);
			otmp->corpsenm = PM_CAVE_SPIDER;
			return otmp;
		} else if(chance < 45){
			return mksobj_at(EUCALYPTUS_LEAF, x, y, NO_MKOBJ_FLAGS);
		} else {
			otmp = mksobj_at(SLIME_MOLD, x, y,NO_MKOBJ_FLAGS);
			otmp->spe = fruitadd("moldy remains");
			return otmp;
		}
	} else if((In_quest(&u.uz) && Role_if(PM_PIRATE)) || Is_paradise(&u.uz)){
		int chance = rn2(100);
		if(chance < 10){
			otmp = mksobj_at(SLIME_MOLD, x, y,NO_MKOBJ_FLAGS);
			otmp->spe = fruitadd("coconut");
			return otmp;
		} else if(chance < 25){
			return mksobj_at(BANANA, x, y, NO_MKOBJ_FLAGS);
		} else if(chance < 50){
			otmp = mksobj_at(SLIME_MOLD, x, y,NO_MKOBJ_FLAGS);
			otmp->spe = fruitadd("mango");
			return otmp;
		} else if(chance < 75){
			return mksobj_at(ORANGE, x, y, NO_MKOBJ_FLAGS);
		} else if(chance < 99){
			otmp = mksobj_at(EGG, x, y,NO_MKOBJ_FLAGS);
			otmp->corpsenm = PM_PARROT;
			return otmp;
		} else{
			otmp = mksobj_at(SLIME_MOLD, x, y,NO_MKOBJ_FLAGS);
			otmp->spe = fruitadd("tree squid");
			return otmp;
		}
	} else if(In_quest(&u.uz) && Role_if(PM_KNIGHT)){
		return mksobj_at(APPLE, x, y, NO_MKOBJ_FLAGS);
	}
	return mksobj_at(treefruits[rn2(SIZE(treefruits))], x, y, NO_MKOBJ_FLAGS);
}
#endif /* OVL0 */
#ifdef OVLB

struct obj *
mkgold_core(amount, x, y, new)
long amount;
int x, y;
boolean new;
{
    register struct obj *gold = g_at(x,y);

    if (amount <= 0L)
		amount = (long)(1 + rnd(level_difficulty()+2) * rnd(30));
    if (gold) {
	gold->quan += amount;
    } else {
	gold = mksobj_at(GOLD_PIECE, x, y, NO_MKOBJ_FLAGS);
	gold->quan = amount;
    }
    gold->owt = weight(gold);
	if(new)
		u.spawnedGold += amount;
    return (gold);
}

struct obj *
mkgold(amount, x, y)
long amount;
int x, y;
{
	struct obj *gold;
	int quan = 0;

	gold = g_at(x,y);
	if(gold)
		quan = gold->quan;

	gold = mkgold_core(amount, x, y, TRUE);
	if(gold && uring_art(ART_RING_OF_THROR)){
		quan = gold->quan - quan; //Final minus initial
		gold->quan += 2*quan + d(2,2) - 3;
		if(gold->quan < 1)
			gold->quan = 1;
		gold->owt = weight(gold);
	}
	return gold;
}

#endif /* OVLB */
#ifdef OVL1

/* return TRUE if the corpse has special timing */
#define special_corpse(num)  (((num) == PM_LIZARD)		\
				|| ((num) == PM_LICHEN)		\
				|| ((num) == PM_CROW_WINGED_HALF_DRAGON)		\
				|| ((num) == PM_BEHOLDER)		\
				|| (is_rider(&mons[num]))	\
				|| (mons[num].mlet == S_TROLL))

/*
 * OEXTRA note: Passing mtmp causes mtraits to be saved
 * even if ptr passed as well, but ptr is always used for
 * the corpse type (corpsenm). That allows the corpse type
 * to be different from the original monster,
 *	i.e.  vampire -> human corpse
 * yet still allow restoration of the original monster upon
 * resurrection.
 */
struct obj *
mkcorpstat(objtype, mtmp, ptr, x, y, init)
int objtype;	/* CORPSE or STATUE */
struct monst *mtmp;
struct permonst *ptr;
int x, y;
boolean init;
{
	register struct obj *otmp;
	int mkobjflags = (init ? NO_MKOBJ_FLAGS : MKOBJ_NOINIT);

	if (objtype != CORPSE && objtype != STATUE && objtype != BROKEN_ANDROID && objtype != BROKEN_GYNOID && objtype != LIFELESS_DOLL)
	    impossible("making corpstat type %d", objtype);
	if (x == 0 && y == 0) {		/* special case - random placement */
		otmp = mksobj(objtype, mkobjflags);
		if (otmp) rloco(otmp);
	} else
		otmp = mksobj_at(objtype, x, y, mkobjflags);
	if (otmp) {
	    if (mtmp) {
			if (!ptr) ptr = mtmp->data;
			/* override any OX_EMON made by default mkobj with mtmp */
			rem_ox(otmp, OX_EMON);
			/* save_mtraits reallocates the data pointed to by otmp */
			otmp = save_mtraits(otmp, mtmp);
	    }
	    /* use the corpse or statue produced by mksobj() as-is
	       unless `ptr' is non-null */
	    if (ptr) {
			otmp->corpsenm = monsndx(ptr);
			otmp->owt = weight(otmp);
			// if (otmp->otyp == CORPSE &&
				// (special_corpse(old_corpsenm) ||
					// special_corpse(otmp->corpsenm))) {
			//Between molding and all the special effects, would be best to just reset timers for everything.
			if (otmp->otyp == CORPSE) {
				stop_all_timers(otmp->timed);
				/* if the monster was cancelled, don't self-revive */
				if (mtmp && mtmp->mcan && !is_rider(ptr))
					otmp->norevive = 1;
				start_corpse_timeout(otmp);
			}
	    }
	}
	return(otmp);
}

/*
 * Attach a monster id to an object, to provide
 * a lasting association between the two.
 */
struct obj *
obj_attach_mid(obj, mid)
struct obj *obj;
unsigned mid;
{
    if (!mid || !obj) return (struct obj *)0;

	add_ox(obj, OX_EMID);
	obj->oextra_p->emid_p[0] = mid;

    return obj;
}

struct obj *
save_mtraits(obj, mtmp)
struct obj *obj;
struct monst *mtmp;
{
	long lth, namelth;

	void * mextra_bundle;
	/* if mtmp has an mextra, bundle it */
	if (mtmp->mextra_p) {
		void * tmp, * tmp2;
		mextra_bundle = bundle_mextra(mtmp, &lth);
		/* save the whole monster followed by its mextrabundle */
		tmp2 = tmp = malloc(lth + sizeof(struct monst));
		memcpy(tmp2, (genericptr_t) mtmp, sizeof(struct monst));
		tmp2 = tmp2 + sizeof(struct monst);
		memcpy(tmp2, mextra_bundle, lth);
		lth += sizeof(struct monst);
		free(mextra_bundle);	/* free the memory allocated to make the original bundle */
		mextra_bundle = tmp;	/* mextra_bundle now points to a continuous mtmp+mextra */
	}
	else {
		mextra_bundle = malloc(sizeof(struct monst));
		memcpy(mextra_bundle, (genericptr_t) mtmp, sizeof(struct monst));
		lth = sizeof(struct monst);
	}
	/* attach it to obj */
	add_ox_l(obj, OX_EMON, lth);
	memcpy((genericptr_t)EMON(obj), mextra_bundle, lth);
	free(mextra_bundle);
	/* invalidate pointers */
	/* m_id is needed to know if this is a revived quest leader */
	/* but m_id must be cleared when loading bones */
	EMON(obj)->nmon     = (struct monst *)0;
	EMON(obj)->data     = (struct permonst *)0;
	EMON(obj)->minvent  = (struct obj *)0;
	EMON(obj)->timed    = (struct timer *)0;
	EMON(obj)->light    = (struct ls_t *)0;
	EMON(obj)->mgold    = 0;
	return obj;
}

/* returns a pointer to a new monst structure based on
 * the one contained within the obj.
 * If obj did not have an oattached monst, returns a
 * temporary monst pointer which cannot be inserted
 * into the monst chains.
 */
struct monst *
get_mtraits(obj, copyof)
struct obj *obj;
boolean copyof;
{
	struct monst *mtmp = (struct monst *)0;
	struct monst *mnew = (struct monst *)0;

	if (get_ox(obj, OX_EMON)) {
		mtmp = EMON(obj);
	}
	if (mtmp) {
		if (copyof) {
			mnew = malloc(sizeof(struct monst));	/* allocate memory for the monster */
			memcpy((genericptr_t)mnew, (genericptr_t)mtmp, sizeof(struct monst));

			/* we may also need to copy mextra */
			if (mnew->mextra_p) {
				void * mextra_bundle = mtmp+1;
				unbundle_mextra(mnew, mextra_bundle);
				/* relink any stale pointers */
				relink_mx(mnew);
			}
		}
		else {
			/* return a read-only mtmp */
			mnew = mtmp;
		}
	}
	return mnew;
}

#endif /* OVL1 */
#ifdef OVLB

/* make an object named after someone listed in the scoreboard file */
struct obj *
mk_tt_object(objtype, x, y)
int objtype; /* CORPSE or STATUE */
register int x, y;
{
	register struct obj *otmp, *otmp2;
	int mkobjflags = NO_MKOBJ_FLAGS;

	/* player statues never contain books */
	if (objtype == STATUE)
		mkobjflags |= MKOBJ_NOINIT;
	if ((otmp = mksobj_at(objtype, x, y, mkobjflags)) != 0) {
	    /* tt_oname will return null if the scoreboard is empty */
	    if ((otmp2 = tt_oname(otmp)) != 0) otmp = otmp2;
	}
	return(otmp);
}

/* make a new corpse or statue, uninitialized if a statue (i.e. no books) */
struct obj *
mk_named_object(objtype, ptr, x, y, nm)
int objtype;	/* CORPSE or STATUE */
struct permonst *ptr;
int x, y;
const char *nm;
{
	struct obj *otmp;

	otmp = mkcorpstat(objtype, (struct monst *)0, ptr,
				x, y, (boolean)(objtype != STATUE));
	if (nm)
		otmp = oname(otmp, nm);
	return(otmp);
}

boolean
is_flammable(otmp)
register struct obj *otmp;
{
	int otyp = otmp->otyp;
	int omat = otmp->obj_material;

	/* Candles can be burned, but they're not flammable in the sense that
	 * they can't get fire damage and it makes no sense for them to be
	 * fireproofed.
	 */
	if (Is_candle(otmp))
		return FALSE;

	if (item_has_property(otmp, FIRE_RES) || otyp == WAN_FIRE
	 || otyp == SCR_FIRE || otyp == SCR_RESISTANCE || otyp == SPE_FIREBALL || otyp == FIRE_HORN
	)
		return FALSE;

	if (otyp == SPE_BOOK_OF_THE_DEAD)
		return FALSE;

	return((boolean)((omat <= CHITIN && omat != LIQUID) || omat == PLASTIC));
}

boolean
is_rottable(otmp)
register struct obj *otmp;
{
	int otyp = otmp->otyp;

	return((boolean)(otmp->obj_material <= CHITIN &&
			otmp->obj_material != LIQUID));
}

#endif /* OVLB */
#ifdef OVL1

/*
 * These routines maintain the single-linked lists headed in level.objects[][]
 * and threaded through the nexthere fields in the object-instance structure.
 */

/* put the object at the given location */
void
place_object(otmp, x, y)
register struct obj *otmp;
int x, y;
{
    register struct obj *otmp2;

    if (otmp->where != OBJ_FREE)
	panic("place_object: obj not free");

    obj_no_longer_held(otmp);
    if (is_boulder(otmp)) block_point(x,y);	/* vision */

	otmp2 = level.objects[x][y];//Special effects of obj_no_longer_held may change level.objects[x][y]
    /* obj goes under boulders */
    if (otmp2 && is_boulder(otmp2)) {
		otmp->nexthere = otmp2->nexthere;
		otmp2->nexthere = otmp;
    } else {
		otmp->nexthere = otmp2;
		level.objects[x][y] = otmp;
    }

    /* set the new object's location */
    otmp->ox = x;
    otmp->oy = y;

    otmp->where = OBJ_FLOOR;

    /* add to floor chain */
    otmp->nobj = fobj;
    fobj = otmp;
    if (otmp->timed) obj_timer_checks(otmp, x, y, 0);
}

#define ON_ICE(a) ((a)->recharged)
#define ROT_ICE_ADJUSTMENT 2	/* rotting on ice takes 2 times as long */

/* If ice was affecting any objects correct that now
 * Also used for starting ice effects too. [zap.c]
 */
void
obj_ice_effects(x, y, do_buried)
int x, y;
boolean do_buried;
{
	struct obj *otmp;

	for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere) {
		if (otmp->timed) obj_timer_checks(otmp, x, y, 0);
	}
	if (do_buried) {
	    for (otmp = level.buriedobjlist; otmp; otmp = otmp->nobj) {
 		if (otmp->ox == x && otmp->oy == y) {
			if (otmp->timed) obj_timer_checks(otmp, x, y, 0);
		}
	    }
	}
}

/*
 * Returns an obj->age for a corpse object on ice, that would be the
 * actual obj->age if the corpse had just been lifted from the ice.
 * This is useful when just using obj->age in a check or calculation because
 * rot timers pertaining to the object don't have to be stopped and
 * restarted etc.
 */
long
peek_at_iced_corpse_age(otmp)
struct obj *otmp;
{
    long age, retval = otmp->age;
    
    if (otmp->otyp == CORPSE && ON_ICE(otmp)) {
	/* Adjust the age; must be same as obj_timer_checks() for off ice*/
	age = monstermoves - otmp->age;
	retval = otmp->age + (age / ROT_ICE_ADJUSTMENT);
#ifdef DEBUG_EFFECTS
	pline_The("%s age has ice modifications:otmp->age = %ld, returning %ld.",
		s_suffix(doname(otmp)),otmp->age, retval);
	pline("Effective age of corpse: %ld.",
		monstermoves - retval);
#endif
    }
    return retval;
}

STATIC_OVL void
obj_timer_checks(otmp, x, y, force)
struct obj *otmp;
xchar x, y;
int force;	/* 0 = no force so do checks, <0 = force off, >0 force on */
{
    long tleft = 0L;
    short action = ROT_CORPSE;
    boolean restart_timer = FALSE;
    boolean on_floor = (otmp->where == OBJ_FLOOR);
    boolean buried = (otmp->where == OBJ_BURIED);

    /* Check for corpses just placed on or in ice */
    if (otmp->otyp == CORPSE && (on_floor || buried) && is_ice(x,y)) {
		tleft = stop_timer(action, otmp->timed);
		if (tleft == 0L) {
			action = REVIVE_MON;
			tleft = stop_timer(action, otmp->timed);
		} 
		if (tleft != 0L) {
			long age;
			
			tleft = tleft - monstermoves;
			/* mark the corpse as being on ice */
			ON_ICE(otmp) = 1;
#ifdef DEBUG_EFFECTS
			pline("%s is now on ice at %d,%d.", The(xname(otmp)),x,y);
#endif
			/* Adjust the time remaining */
			tleft *= ROT_ICE_ADJUSTMENT;
			restart_timer = TRUE;
			/* Adjust the age; must be same as in obj_ice_age() */
			age = monstermoves - otmp->age;
			otmp->age = monstermoves - (age * ROT_ICE_ADJUSTMENT);
		}
    }
    /* Check for corpses coming off ice */
    else if ((force < 0) ||
	     (otmp->otyp == CORPSE && ON_ICE(otmp) &&
	     ((on_floor && !is_ice(x,y)) || !on_floor))) {
		tleft = stop_timer(action, otmp->timed);
		if (tleft == 0L) {
			action = REVIVE_MON;
			tleft = stop_timer(action, otmp->timed);
		}
		if (tleft != 0L) {
			long age;

			tleft = tleft - monstermoves;
			ON_ICE(otmp) = 0;
#ifdef DEBUG_EFFECTS
				pline("%s is no longer on ice at %d,%d.", The(xname(otmp)),x,y);
#endif
			/* Adjust the remaining time */
			tleft /= ROT_ICE_ADJUSTMENT;
			restart_timer = TRUE;
			/* Adjust the age */
			age = monstermoves - otmp->age;
			otmp->age = otmp->age + (age / ROT_ICE_ADJUSTMENT);
		}
    }
    /* now re-start the timer with the appropriate modifications */ 
    if (restart_timer)
	(void) start_timer(tleft, TIMER_OBJECT, action, (genericptr_t)otmp);
}

#undef ON_ICE
#undef ROT_ICE_ADJUSTMENT

void
remove_object(otmp)
register struct obj *otmp;
{
    xchar x = otmp->ox;
    xchar y = otmp->oy;

    if (otmp->where != OBJ_FLOOR)
	panic("remove_object: obj not on floor");
    if (is_boulder(otmp)) unblock_point(x,y); /* vision */
    extract_nexthere(otmp, &level.objects[x][y]);
    extract_nobj(otmp, &fobj);
    if (otmp->timed) obj_timer_checks(otmp,x,y,0);

	/* if this removed the last object there... */
	if (!level.objects[x][y]) {
		/* hiding creatures might be revealed */
		struct monst * mtmp = m_at(x, y);
		if (mtmp && hides_under(mtmp->data) && mtmp->mundetected)
			mtmp->mundetected = FALSE;
		/* hiding players might be revealed */
		if ((x == u.ux && y == u.uy) && hides_under(youracedata) && u.uundetected)
			u.uundetected = FALSE;
	}
}

/* extra handling on extracting an object from a (non magic chest) container */
void
uncontain(obj)
struct obj * obj;
{
	if (is_gemable_lightsaber(obj->ocontainer) && obj->ocontainer->lamplit)
		end_burn(obj->ocontainer, TRUE);
}

/* throw away all of a monster's inventory */
void
discard_minvent(mtmp)
struct monst *mtmp;
{
    struct obj *otmp;

    while ((otmp = mtmp->minvent) != 0) {
	obj_extract_self(otmp);
	obfree(otmp, (struct obj *)0);	/* dealloc_obj() isn't sufficient */
    }
}

/* Extract self, including from monster or player worn equipment */

void
obj_extract_and_unequip_self(obj)
struct obj *obj;
{
	struct monst *mon;
	long unwornmask;
	if (obj->where == OBJ_MINVENT) {
		mon = obj->ocarry;
		obj_extract_self(obj);
		if ((unwornmask = obj->owornmask) != 0L) {
			mon->misc_worn_check &= ~unwornmask;
			if (obj->owornmask & W_WEP) {
				setmnotwielded(mon,obj);
				MON_NOWEP(mon);
			}
			if (obj->owornmask & W_SWAPWEP){
				setmnotwielded(mon,obj);
				MON_NOSWEP(mon);
			}
			obj->owornmask = 0L;
			update_mon_intrinsics(mon, obj, FALSE, FALSE);
		}
	}
	else if(obj->where == OBJ_INVENT){
		if(obj->owornmask) remove_worn_item(obj, TRUE);
		obj_extract_self(obj);
	}
	else obj_extract_self(obj);
	if (obj->otyp == LEASH && obj->leashmon) o_unleash(obj);
	if (obj->oclass == FOOD_CLASS) food_disappears(obj);
	if (obj->oclass == SPBOOK_CLASS) book_disappears(obj);
}

/*
 * Free obj from whatever list it is on in preperation of deleting it or
 * moving it elsewhere.  This will perform all high-level consequences
 * involved with removing the item.  E.g. if the object is in the hero's
 * inventory and confers heat resistance, the hero will lose it.
 *
 * Object positions:
 *	OBJ_FREE	not on any list
 *	OBJ_FLOOR	fobj, level.locations[][] chains (use remove_object)
 *	OBJ_CONTAINED	cobj chain of container object
 *	OBJ_INVENT	hero's invent chain (use freeinv)
 *	OBJ_MINVENT	monster's invent chain
 *	OBJ_MIGRATING	migrating chain
 *	OBJ_BURIED	level.buriedobjs chain
 *	OBJ_ONBILL	on billobjs chain
 *	OBJ_MAGIC_CHEST	magic chest chains
 */
void
obj_extract_self(obj)
    struct obj *obj;
{
    switch (obj->where) {
	case OBJ_FREE:
	    break;
	case OBJ_FLOOR:
	    remove_object(obj);
	    break;
	case OBJ_CONTAINED:
		uncontain(obj);
	    extract_nobj(obj, &obj->ocontainer->cobj);
	    container_weight(obj->ocontainer);
	    break;
	case OBJ_INVENT:
	    freeinv(obj);
	    break;
	case OBJ_MINVENT:
		m_freeinv(obj);
	    break;
	case OBJ_MIGRATING:
	    extract_nobj(obj, &migrating_objs);
	    break;
	case OBJ_BURIED:
	    extract_nobj(obj, &level.buriedobjlist);
	    break;
	case OBJ_ONBILL:
	    extract_nobj(obj, &billobjs);
	    break;
	case OBJ_MAGIC_CHEST:
	    extract_magic_chest_nobj(obj);
	    break;
	case OBJ_INTRAP:
		obj_extract_self_from_trap(obj);
		/* The only place that we should be trying to extract an object inside a
		* trap is from within the trap code, where we have a pointer to the
		* trap that contains the object. We should never be trying to extract
		* an object inside a trap without that context. */
		// panic("trying to extract object from trap with no trap info");
		break;
	default:
	    panic("obj_extract_self");
	    break;
    }
}


/* Extract the given object from the chain, following nobj chain. */
void
extract_nobj(obj, head_ptr)
    struct obj *obj, **head_ptr;
{
    struct obj *curr, *prev;

    curr = *head_ptr;
    for (prev = (struct obj *) 0; curr; prev = curr, curr = curr->nobj) {
	if (curr == obj) {
	    if (prev)
		prev->nobj = curr->nobj;
	    else
		*head_ptr = curr->nobj;
	    break;
	}
    }
    if (!curr){
		panic("extract_nobj: object lost");
	}
    obj->where = OBJ_FREE;
	obj->nobj = (struct obj *)0;
}


/* Extract the given object from the magic_chest. */
void extract_magic_chest_nobj(struct obj *obj)
{
	int i;
    struct obj *curr, *prev, **head_ptr;

	for(i=0;i<10;i++){
		curr = magic_chest_objs[i];
		head_ptr = &(magic_chest_objs[i]);
		for (prev = NULL; curr; prev = curr, curr = curr->nobj) {
			if (curr == obj) {
				if (prev)
				prev->nobj = curr->nobj;
				else
				*head_ptr = curr->nobj;
				i=11;//break outer loop too
				break;
			}
		}
	}
    if (!curr) panic("extract_nobj: object lost");
    obj->where = OBJ_FREE;
	obj->nobj = (struct obj *)0;
}


/*
 * Extract the given object from the chain, following nexthere chain.
 *
 * This does not set obj->where, this function is expected to be called
 * in tandem with extract_nobj, which does set it.
 */
void
extract_nexthere(obj, head_ptr)
    struct obj *obj, **head_ptr;
{
    struct obj *curr, *prev;

    curr = *head_ptr;
    for (prev = (struct obj *) 0; curr; prev = curr, curr = curr->nexthere) {
	if (curr == obj) {
	    if (prev)
		prev->nexthere = curr->nexthere;
	    else
		*head_ptr = curr->nexthere;
	    break;
	}
    }
    if (!curr) panic("extract_nexthere: object lost");
}


/*
 * Add obj to mon's inventory.  If obj is able to merge with something already
 * in the inventory, then the passed obj is deleted and 1 is returned.
 * Otherwise 0 is returned.
 */
int
add_to_minv(mon, obj)
    struct monst *mon;
    struct obj *obj;
{
    struct obj *otmp;

    if (obj->where != OBJ_FREE)
	panic("add_to_minv: obj not free");

	if(mon->mtyp == PM_MAID && maid_clean(mon, obj) ) return 1; /*destroyed by maid*/

    /* merge if possible */
    for (otmp = mon->minvent; otmp; otmp = otmp->nobj)
	if (merged(&otmp, &obj))
	    return 1;	/* obj merged and then free'd */
    /* else insert; don't bother forcing it to end of chain */
    obj->where = OBJ_MINVENT;
    obj->ocarry = mon;
    obj->nobj = mon->minvent;
    mon->minvent = obj;
	/* if it's a summoned obj and "sticky", attach obj to mon */
	if (get_ox(obj, OX_ESUM) && obj->oextra_p->esum_p->sticky) {
		obj->oextra_p->esum_p->summoner = mon;
		obj->oextra_p->esum_p->sm_id = mon->m_id;
		obj->oextra_p->esum_p->sm_o_id = 0;
		obj->oextra_p->esum_p->sticky = 0;
	}
	/* apply artifact on-carry properties */
	update_mon_intrinsics(mon, obj, TRUE, FALSE);
    return 0;	/* obj on mon's inventory chain */
}

/*
 * A maid is evaluating and repairing the object.
 * If she decides to throw it out, useup is called and 1 is returned.
 */
int
maid_clean(mon, obj)
    struct monst *mon;
    struct obj *obj;
{
	if(objects[obj->otyp].oc_unique || obj->oartifact)
		return 0;
	if(obj->oeroded){
		if( d(1,20) < (int)is_rustprone(obj) ? (int)obj->oeroded : ((int)obj->oeroded) * 4){
			if(canseemon(mon)) pline("The maid breaks the %s trash down for parts.", is_rustprone(obj) ? "rusted-out" : "burned-out");
			obj->quan = 0;
			obfree(obj, (struct obj *)0);
			return 1;
		}
		obj->oeroded = 0;
		if(canseemon(mon)) pline("The maid %s", is_rustprone(obj) ? "scourers off the rust." : "patches the burned areas.");
	}
	if(obj->oeroded2){
		if( d(1,12) < (int)is_corrodeable(obj) ? (int)obj->oeroded2 : ((int)obj->oeroded2) * 4){
			if(canseemon(mon)) pline("The maid breaks the %s trash down for parts.", is_corrodeable(obj) ? "corroded" : "rotten");
			obj->quan = 0;
			obfree(obj, (struct obj *)0);
			return 1;
		}
		obj->oeroded2 = 0;
		if(canseemon(mon)) pline("The maid %s", is_corrodeable(obj) ? "scourers away the corrosion." : "excises and patches the rotted areas.");
	}
	if(obj->obroken && Is_real_container(obj)){
		if(canseemon(mon)) pline("The maid mends the broken lock.");
		obj->obroken = 0;
	}
	if(obj->cursed){
		if( d(1,20) < 5){
			struct engr *oep = engr_at(mon->mx,mon->my);
			if(canseemon(mon)) pline("The maid banishes the unholy object to hell.");
			add_to_migration(obj);
			obj->ox = sanctum_level.dnum;
			obj->oy = sanctum_level.dlevel - 1; /* vs level, bottom of the accesible part of hell */
			obj->owornmask = (long)MIGR_RANDOM;
			if(!oep){
				make_engr_at(mon->mx, mon->my,
			     "U G I EI A", 0L, MARK);
				oep = engr_at(mon->mx,mon->my);
			}
			oep->ward_id = PENTAGRAM;
			oep->halu_ward = 0;
			oep->ward_type = MARK;
			oep->complete_wards = 1;
			return 1;
		}
		if(canseemon(mon)) pline("The maid sticks an ofuda to the offending object.");
		obj->cursed = 0;
	}
	if(obj->otyp == DWARVISH_HELM || obj->otyp == OIL_LAMP || obj->otyp == LANTERN || obj->otyp == LANTERN_PLATE_MAIL){
		if(obj->age < 750){
			obj->age += 750;
			if(canseemon(mon)) {
				if(obj->otyp == OIL_LAMP)
					pline("The maid adds some oil.");
				else if(obj->otyp == LANTERN_PLATE_MAIL)
					pline("The maid changes the batteries.");
			}
		}
	}
#ifdef TOURIST
	if(obj->otyp == EXPENSIVE_CAMERA){
		if (obj->spe <= 50){
			obj->spe = 50+rn1(16,10);		/* 10..25 */
			if(canseemon(mon)) pline("The maid replaces the film.");
		}
	}
#endif
	return 0;
}

/*
 * Place obj in a magic chest, make sure "obj" is free.
 * Returns (merged) object.
 * The input obj may be deleted in the process.
 * Based on the implementation of add_to_container.
 */
struct obj *
add_to_magic_chest(struct obj *obj,int key)
{
    struct obj *otmp;

    if (obj->where != OBJ_FREE) {
	panic("add_to_magic_chest: obj not free (%d,%d,%d)",
	      obj->where, obj->otyp, obj->invlet);
    }

    /* merge if possible */
    for (otmp = magic_chest_objs[key]; otmp; otmp = otmp->nobj) if (merged(&otmp, &obj)) return otmp;

    obj->where = OBJ_MAGIC_CHEST;
    obj->nobj = magic_chest_objs[key];
    magic_chest_objs[key] = obj;
    return obj;
}

/*
 * Add obj to container, make sure obj is "free".  Returns (merged) obj.
 * The input obj may be deleted in the process.
 */
struct obj *
add_to_container(container, obj)
    struct obj *container, *obj;
{
    struct obj *otmp;

    if (obj->where != OBJ_FREE)
	panic("add_to_container: obj not free");
    if (container->where != OBJ_INVENT && container->where != OBJ_MINVENT)
	obj_no_longer_held(obj);

    /* merge if possible */
    for (otmp = container->cobj; otmp; otmp = otmp->nobj)
	if (merged(&otmp, &obj)) return (otmp);

    obj->where = OBJ_CONTAINED;
    obj->ocontainer = container;
    obj->nobj = container->cobj;
    container->cobj = obj;
	container->owt = weight(container);
    return (obj);
}

void
add_to_migration(obj)
    struct obj *obj;
{
    if (obj->where != OBJ_FREE)
	panic("add_to_migration: obj not free");
	
	migrate_timers(obj->timed);
    obj->where = OBJ_MIGRATING;
    obj->nobj = migrating_objs;
    migrating_objs = obj;
}

void
add_to_buried(obj)
    struct obj *obj;
{
    if (obj->where != OBJ_FREE)
	panic("add_to_buried: obj not free");

    obj->where = OBJ_BURIED;
    obj->nobj = level.buriedobjlist;
    level.buriedobjlist = obj;
}

/* Recalculate the weight of this container and all of _its_ containers. */
void
container_weight(container)
    struct obj *container;
{
    container->owt = weight(container);
    if (container->where == OBJ_CONTAINED)
	container_weight(container->ocontainer);
/*
    else if (container->where == OBJ_INVENT)
	recalculate load delay here ???
*/
}

/*
 * Deallocate the object.  _All_ objects should be run through here for
 * them to be deallocated.
 */
void
dealloc_obj(obj)
    struct obj *obj;
{
    if (obj->where != OBJ_FREE)
		panic("dealloc_obj: obj not free");

    /* free up any timers attached to the object */
    if (obj->timed)
		stop_all_timers(obj->timed);

    /*
     * Free up any light sources attached to the object.
     */
	if (obj->light)
		del_light_source(obj->light);

	/* Free any oextra attached to the object */
	if (obj->oextra_p)
		rem_all_ox(obj);

	if (obj->mp)
		free((genericptr_t) obj->mp);

    free((genericptr_t) obj);
}

int
hornoplenty(horn, tipping)
struct obj *horn;
boolean tipping; /* caller emptying entire contents; affects shop handling */
{
    int objcount = 0;

    if (!horn || horn->otyp != HORN_OF_PLENTY) {
        impossible("bad horn o' plenty");
    } else if (horn->spe < 1) {
        pline1(nothing_happens);
    } else {
        struct obj *obj;
        const char *what;

        consume_obj_charge(horn, !tipping);
        if (!rn2(13)) {
            obj = mkobj(POTION_CLASS, FALSE);
            if (objects[obj->otyp].oc_magic)
                do {
                    obj->otyp = rnd_class(POT_BOOZE, POT_WATER);
                } while (obj->otyp == POT_SICKNESS);
            what = (obj->quan > 1L) ? "Some potions" : "A potion";
        } else {
            obj = mkobj(FOOD_CLASS, FALSE);
            if (obj->otyp == FOOD_RATION && !rn2(7))
                obj->otyp = LUMP_OF_ROYAL_JELLY;
            what = "Some food";
        }
        ++objcount;
        pline("%s %s out.", what, vtense(what, "spill"));
        obj->blessed = horn->blessed;
        obj->cursed = horn->cursed;
        obj->owt = weight(obj);
        /* using a shop's horn of plenty entails a usage fee and also
           confers ownership of the created item to the shopkeeper */
        if (horn->unpaid)
            addtobill(obj, FALSE, FALSE, tipping);
        /* if it ended up on bill, we don't want "(unpaid, N zorkids)"
           being included in its formatted name during next message */
        if (!tipping) {
            obj = hold_another_object(obj,
                                      u.uswallow
                                        ? "Oops!  %s out of your reach!"
                                        : (Is_airlevel(&u.uz)
                                           || Is_waterlevel(&u.uz)
                                           || levl[u.ux][u.uy].typ < IRONBARS
                                           || levl[u.ux][u.uy].typ >= ICE)
                                          ? "Oops!  %s away from you!"
                                          : "Oops!  %s to the floor!",
                                      The(aobjnam(obj, "slip")), (char *) 0);
        } else {
            /* assumes this is taking place at hero's location */
            if (!can_reach_floor()) {
				bhitpos.x = u.ux; bhitpos.y = u.uy;
				obj->ox = u.ux; obj->oy = u.uy;
				hitfloor2(&youmonst, &obj, (struct obj *)0, FALSE, FALSE);
                //hitfloor(obj, TRUE); /* does altar check, message, drop */
            } else {
                if (IS_ALTAR(levl[u.ux][u.uy].typ))
                    doaltarobj(obj, god_at_altar(u.ux,u.uy)); /* does its own drop message */
                else
                    pline("%s %s to the %s.", Doname2(obj),
                          otense(obj, "drop"), surface(u.ux, u.uy));
                dropy(obj);
            }
        }
        if (horn->dknown)
            makeknown(HORN_OF_PLENTY);
    }
    return objcount;
}

#ifdef WIZARD
/* Check all object lists for consistency. */
void
obj_sanity_check()
{
    int x, y;
    struct obj *obj;
    struct monst *mon;
    const char *mesg;
    char obj_address[20], mon_address[20];  /* room for formatted pointers */

    mesg = "fobj sanity";
    for (obj = fobj; obj; obj = obj->nobj) {
	if (obj->where != OBJ_FLOOR) {
	    pline("%s obj %s %s@(%d,%d): %s\n", mesg,
		fmt_ptr((genericptr_t)obj, obj_address),
		where_name(obj->where),
		obj->ox, obj->oy, doname(obj));
	}
	check_contained(obj, mesg);
    }

    mesg = "location sanity";
    for (x = 0; x < COLNO; x++)
	for (y = 0; y < ROWNO; y++)
	    for (obj = level.objects[x][y]; obj; obj = obj->nexthere)
		if (obj->where != OBJ_FLOOR) {
		    pline("%s obj %s %s@(%d,%d): %s\n", mesg,
			fmt_ptr((genericptr_t)obj, obj_address),
			where_name(obj->where),
			obj->ox, obj->oy, doname(obj));
		}

    mesg = "invent sanity";
    for (obj = invent; obj; obj = obj->nobj) {
	if (obj->where != OBJ_INVENT) {
	    pline("%s obj %s %s: %s\n", mesg,
		fmt_ptr((genericptr_t)obj, obj_address),
		where_name(obj->where), doname(obj));
	}
	check_contained(obj, mesg);
    }

    mesg = "migrating sanity";
    for (obj = migrating_objs; obj; obj = obj->nobj) {
	if (obj->where != OBJ_MIGRATING) {
	    pline("%s obj %s %s: %s\n", mesg,
		fmt_ptr((genericptr_t)obj, obj_address),
		where_name(obj->where), doname(obj));
	}
	check_contained(obj, mesg);
    }

    mesg = "buried sanity";
    for (obj = level.buriedobjlist; obj; obj = obj->nobj) {
	if (obj->where != OBJ_BURIED) {
	    pline("%s obj %s %s: %s\n", mesg,
		fmt_ptr((genericptr_t)obj, obj_address),
		where_name(obj->where), doname(obj));
	}
	check_contained(obj, mesg);
    }

    mesg = "bill sanity";
    for (obj = billobjs; obj; obj = obj->nobj) {
	if (obj->where != OBJ_ONBILL) {
	    pline("%s obj %s %s: %s\n", mesg,
		fmt_ptr((genericptr_t)obj, obj_address),
		where_name(obj->where), doname(obj));
	}
	/* shouldn't be a full container on the bill */
	if (obj->cobj) {
	    pline("%s obj %s contains %s! %s\n", mesg,
		fmt_ptr((genericptr_t)obj, obj_address),
		something, doname(obj));
	}
    }

    mesg = "minvent sanity";
    for (mon = fmon; mon; mon = mon->nmon)
	for (obj = mon->minvent; obj; obj = obj->nobj) {
	    if (obj->where != OBJ_MINVENT) {
		pline("%s obj %s %s: %s\n", mesg,
			fmt_ptr((genericptr_t)obj, obj_address),
			where_name(obj->where), doname(obj));
	    }
	    if (obj->ocarry != mon) {
		pline("%s obj %s (%s) not held by mon %s (%s)\n", mesg,
			fmt_ptr((genericptr_t)obj, obj_address),
			doname(obj),
			fmt_ptr((genericptr_t)mon, mon_address),
			mon_nam(mon));
	    }
	    check_contained(obj, mesg);
	}
}

/* This must stay consistent with the defines in obj.h. */
static const char *obj_state_names[NOBJ_STATES] = {
	"free",		"floor",	"contained",	"invent",
	"minvent",	"migrating",	"buried",	"onbill"
};

STATIC_OVL const char *
where_name(where)
    int where;
{
    return (where<0 || where>=NOBJ_STATES) ? "unknown" : obj_state_names[where];
}

/* obj sanity check: check objs contained by container */
STATIC_OVL void
check_contained(container, mesg)
    struct obj *container;
    const char *mesg;
{
    struct obj *obj;
    char obj1_address[20], obj2_address[20];

    for (obj = container->cobj; obj; obj = obj->nobj) {
	if (obj->where != OBJ_CONTAINED)
	    pline("contained %s obj %s: %s\n", mesg,
		fmt_ptr((genericptr_t)obj, obj1_address),
		where_name(obj->where));
	else if (obj->ocontainer != container)
	    pline("%s obj %s not in container %s\n", mesg,
		fmt_ptr((genericptr_t)obj, obj1_address),
		fmt_ptr((genericptr_t)container, obj2_address));
    }
}
#endif /* WIZARD */

#endif /* OVL1 */

/*mkobj.c*/
