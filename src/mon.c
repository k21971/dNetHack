/*	SCCS Id: @(#)mon.c	3.4	2003/12/04	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* If you're using precompiled headers, you don't want this either */
#ifdef MICROPORT_BUG
#define MKROOM_H
#endif

 //Note: math.h must be included before hack.h bc it contains a yn() macro that is incompatible with the one in hack
#include <math.h>

#include "hack.h"
#include "mfndpos.h"

#include "artifact.h"
#include "xhity.h"
#include <ctype.h>
#include <stdlib.h>

extern int monstr[];

STATIC_DCL boolean FDECL(restrap,(struct monst *));
STATIC_DCL int FDECL(scent_callback,(genericptr_t, int, int));
STATIC_DCL void FDECL(dead_familiar,(long));
STATIC_DCL void FDECL(clothes_bite_mon,(struct monst *));
STATIC_DCL void FDECL(emit_healing, (struct monst *));
int scentgoalx, scentgoaly;

#ifdef OVL2
STATIC_DCL void FDECL(kill_eggs, (struct obj *));
#endif

static const char *nyar_description[] = {
/* 0*/	"a titanic tri-radial monster crowned with a massive crimson tentacle",
/* 1*/	"a huge shadowy winged-thing with a burning three-lobed eye",
/* 2*/	"a bloated, vaguely feminine humanoid",
/* 3*/	"a winged giant with round glassy eyes and a long trunk",
/* 4*/	"a towering column of wailing mouths and reaching tentacles",
/* 5*/	"a colossal sphinx whose face is a starry void",
/* 6*/	"a small humanoid with four arms and who crawls on four tentacles",
/* 7*/	"a large skinless man with a burning third eye in his brow",
/* 8*/	"an enormous mass of translucent lunging tentacles that pull in all directions",
/* 9*/	"a howling storm, full of flashing lightning and the glimpse of claws",
/*10*/	"a towering monolith with four arthropoid legs and a high, tri-lobed eye"
};

static const char *nyar_name[] = {
/*0*/	"The God of the Bloody Tongue",
/*1*/	"The Haunter of the Dark",
/*2*/	"The Bloated Woman",
/*3*/	"Shugoran",
/*4*/	"The Wailing Writher",
/*5*/	"The Faceless God",
/*6*/	"The Small Crawler",
/*7*/	"The Skinless One",
/*8*/	"The Messenger of the Old Ones",
/*9*/	"The Black Wind",
/*10*/	"The Minister of the Monoliths"
};

#ifdef REINCARNATION
#define LEVEL_SPECIFIC_NOCORPSE(mdat) \
	 ((Is_rogue_level(&u.uz) || \
	   (level.flags.graveyard && is_undead(mdat) && rn2(3))) \
	   && mdat->mtyp!=PM_GARO && mdat->mtyp!=PM_GARO_MASTER)
#else
#define LEVEL_SPECIFIC_NOCORPSE(mdat) \
	   (level.flags.graveyard && is_undead(mdat) && rn2(3) \
	   && mdat->mtyp!=PM_GARO && mdat->mtyp!=PM_GARO_MASTER)
#endif


#if 0
/* part of the original warning code which was replaced in 3.3.1 */
#ifdef OVL1
#define warnDelay 10
long lastwarntime;
int lastwarnlev;

const char *warnings[] = {
	"white", "pink", "red", "ruby", "purple", "black"
};

STATIC_DCL void NDECL(warn_effects);
#endif /* OVL1 */
#endif /* 0 */

#ifndef OVLB
STATIC_VAR int cham_to_pm[];
#else
STATIC_DCL struct obj *FDECL(make_corpse,(struct monst *));
STATIC_DCL void FDECL(m_detach, (struct monst *, struct permonst *));
STATIC_DCL void FDECL(lifesaved_monster, (struct monst *));

STATIC_DCL double FDECL(atanGerald, (double x));

STATIC_OVL double 
atanGerald(x)
double x;
{
	double temp1 = x >= 0 ? x : -x;
	double temp2 = temp1 <= 1.0 ? temp1 : (temp1 - 1) / (temp1 + 1);
	double sum = temp2;
	size_t i;
	for (i = 1; i != 6; ++i)
		sum += (i % 2 ? -1 : 1) * pow(temp2, (i << 1) + 1) / ((i << 1) + 1);

	if (temp1 > 1.0) sum += 0.785398;
	return x >= 0 ? sum : -sum;
}

STATIC_OVL void
ara_died(mtmp)
struct monst *mtmp;
{
	if(mtmp->mtame)
		u.goldkamcount_tame++;
	else if(mtmp->mpeaceful)
		level.flags.goldkamcount_peace++;
	else
		level.flags.goldkamcount_hostile++;
}

void
removeMonster(x,y)
int x,y;
{
    if (level.monsters[x][y] &&
	opaque(level.monsters[x][y]->data) &&
	 (!level.monsters[x][y]->minvis || See_invisible(x,y)))
		unblock_point(x,y);
    level.monsters[x][y] = (struct monst *)0;
}

/* convert the monster index of an undead to its living counterpart */
int
undead_to_corpse(mndx)
int mndx;
{
	switch (mndx) {
	// case PM_KOBOLD_ZOMBIE:
	case PM_KOBOLD_MUMMY:	mndx = PM_KOBOLD;  break;
	// case PM_DWARF_ZOMBIE:
	case PM_DWARF_MUMMY:	mndx = PM_DWARF;  break;
	// case PM_GNOME_ZOMBIE:
	case PM_GNOME_MUMMY:	mndx = PM_GNOME;  break;
	// case PM_ORC_ZOMBIE:
	case PM_ORC_MUMMY:	mndx = PM_ORC;  break;
	// case PM_ELF_ZOMBIE:
	case PM_ELF_MUMMY:	mndx = PM_ELF;  break;
	case PM_VAMPIRE:
	case PM_VAMPIRE_LORD:
	case PM_VAMPIRE_LADY:
#if 0	/* DEFERRED */
	case PM_VAMPIRE_MAGE:
#endif
	// case PM_HUMAN_ZOMBIE:
	case PM_ZOMBIE:
	case PM_HUMAN_MUMMY:	mndx = PM_HUMAN;  break;
	case PM_HEDROW_ZOMBIE:	mndx = PM_DROW;  break;
	case PM_DROW_MUMMY:	mndx = PM_DROW_MATRON;  break;
	// case PM_GIANT_ZOMBIE:
	case PM_GIANT_MUMMY:	mndx = PM_GIANT;  break;
	// case PM_ETTIN_ZOMBIE:
	case PM_ETTIN_MUMMY:	mndx = PM_ETTIN;  break;
	case PM_ALABASTER_MUMMY: mndx = PM_ALABASTER_ELF_ELDER;  break;
	default:  break;
	}
	return mndx;
}

/* Convert the monster index of some monsters (such as quest guardians)
 * to their generic species type.
 *
 * Return associated character class monster, rather than species
 * if mode is 1.
 */
int
genus(mndx, mode)
int mndx, mode;
{
	switch (mndx) {
/* Quest guardians */
	case PM_STUDENT:     mndx = mode ? PM_ARCHEOLOGIST  : PM_HUMAN; break;
	case PM_TROOPER:   mndx = mode ? PM_ANACHRONONAUT : PM_HUMAN; break;
	case PM_CHIEFTAIN:   mndx = mode ? PM_BARBARIAN : PM_HUMAN; break;
#ifdef BARD
	case PM_RHYMER:      mndx = mode ? PM_BARD   : PM_HUMAN; break;
#endif
	case PM_NEANDERTHAL: mndx = mode ? PM_CAVEMAN   : PM_HUMAN; break;
	case PM_ATTENDANT:   mndx = mode ? PM_HEALER    : PM_HUMAN; break;
	case PM_PAGE:        mndx = mode ? PM_KNIGHT    : PM_HUMAN; break;
	case PM_ABBOT:       mndx = mode ? PM_MONK      : PM_HUMAN; break;
	case PM_PATIENT:       mndx = mode ? PM_MADMAN      : PM_HUMAN; break;
	case PM_ACOLYTE:     mndx = mode ? PM_PRIEST    : PM_HUMAN; break;
	case PM_HUNTER:      mndx = mode ? PM_RANGER    : PM_HUMAN; break;
	case PM_THUG:        mndx = mode ? PM_ROGUE     : PM_HUMAN; break;
	case PM_ROSHI:       mndx = mode ? PM_SAMURAI   : PM_HUMAN; break;
#ifdef TOURIST
	case PM_GUIDE:       mndx = mode ? PM_TOURIST   : PM_HUMAN; break;
#endif
	case PM_APPRENTICE:  mndx = mode ? PM_WIZARD    : PM_HUMAN; break;
	case PM_WARRIOR:     mndx = mode ? PM_VALKYRIE  : PM_HUMAN; break;
	default:
		if (mndx >= LOW_PM && mndx < NUMMONS) {
			struct permonst *ptr = &mons[mndx];
			if (is_human(ptr))      mndx = PM_HUMAN;
			else if (is_myrkalfr(ptr))   mndx = PM_MYRKALFR;
			else if (is_elf(ptr))   mndx = PM_ELF;
			else if (is_dwarf(ptr)) mndx = PM_DWARF;
			else if (is_gnome(ptr)) mndx = PM_GNOME;
			else if (is_orc(ptr))   mndx = PM_ORC;
		}
		break;
	}
	return mndx;
}

/* convert monster index to chameleon index */
int
pm_to_cham(mndx)
int mndx;
{
	int mcham;

	switch (mndx) {
	case PM_CHAMELEON:	mcham = CHAM_CHAMELEON; break;
	case PM_DOPPELGANGER:	mcham = CHAM_DOPPELGANGER; break;
	case PM_SANDESTIN:	mcham = CHAM_SANDESTIN; break;
	case PM_DREAM_QUASIELEMENTAL:	mcham = CHAM_DREAM; break;
	default: mcham = CHAM_ORDINARY; break;
	}
	return mcham;
}

/* convert chameleon index to monster index */
STATIC_VAR int cham_to_pm[] = {
		NON_PM,		/* placeholder for CHAM_ORDINARY */
		PM_CHAMELEON,
		PM_DOPPELGANGER,
		PM_SANDESTIN,
		PM_DREAM_QUASIELEMENTAL,
};

/* for deciding whether corpse or statue will carry along full monster data */
#define KEEPTRAITS(mon)	((mon)->isshk || (mon)->mtame ||		\
			 ((mon)->data->geno & G_UNIQ) ||		\
			 is_reviver((mon)->data) ||			\
			 ((mon)->mfaction) ||			\
			 (templated(mon)) ||			\
			 (get_mx((mon), MX_ESUM)) ||	\
			 ((mon)->ispolyp) ||			\
			 ((mon)->zombify) ||			\
			 ((mon)->mspores) ||			\
			 ((mon)->brainblooms) ||			\
			 ((mon)->mtyp == PM_UNDEAD_KNIGHT) ||			\
			 ((mon)->mtyp == PM_WARRIOR_OF_SUNLIGHT) ||			\
			 ((mon)->mtyp == PM_UNDEAD_MAIDEN) ||			\
			 ((mon)->mtyp == PM_KNIGHT_OF_THE_PRINCESS_S_GUARD) ||			\
			 ((mon)->mtyp == PM_BLUE_SENTINEL) ||			\
			 ((mon)->mtyp == PM_DARKMOON_KNIGHT) ||			\
			 ((mon)->mtyp == PM_UNDEAD_REBEL) ||			\
			 ((mon)->mtyp == PM_PARDONER) ||			\
			 ((mon)->mtyp == PM_OCCULTIST) ||			\
			 ((mon)->mtyp == PM_LIVING_DOLL) ||			\
			 ((mon)->mtyp == PM_ANDROID) ||			\
			 ((mon)->mtyp == PM_GYNOID) ||			\
			 ((mon)->mtyp == PM_COMMANDER) ||			\
			 ((mon)->mtyp == PM_OPERATOR) ||			\
			 /* normally leader the will be unique, */	\
			 /* but he might have been polymorphed  */	\
			 (mon)->m_id == quest_status.leader_m_id ||	\
			 /* special cancellation handling for these */	\
			 (dmgtype((mon)->data, AD_SEDU) ||		\
			  dmgtype((mon)->data, AD_SSEX)))

			  
const int humanoid_eyes[] = {
	PM_HOBBIT,
	PM_DWARF,
	PM_ORC,
	PM_GNOME,
	PM_APE,
	PM_HUMAN,
	PM_ELF,
	PM_MYRKALFR,
	PM_HALF_DRAGON,
	PM_SHEEP,
	PM_HOUSECAT,
	PM_DOG,
	PM_HORSE
};
/* Creates a monster corpse, a "special" corpse, or nothing if it doesn't
 * leave corpses.  Monsters which leave "special" corpses should have
 * G_NOCORPSE set in order to prevent wishing for one, finding tins of one,
 * etc....
 */
STATIC_OVL struct obj *
make_corpse(mtmp)
register struct monst *mtmp;
{
	register struct permonst *mdat = mtmp->data;
	int num;
	struct obj *obj = (struct obj *)0;
	int x = mtmp->mx, y = mtmp->my;
	int mndx = monsndx(mdat);
	struct obj *otmp;
	struct monst *mon;
	
	if (get_mx(mtmp, MX_ESUM)) {
		return (struct obj *)0;
	}
	
	if(u.specialSealsActive&SEAL_NUDZIRATH && !rn2(4)){
		(void) mksobj_at(MIRROR, x, y, NO_MKOBJ_FLAGS);
	}

	if(has_template(mtmp, CRYSTALFIED)){
		obj = mkcorpstat(STATUE, (struct monst *)0,
			mdat, x, y, FALSE);
		set_material_gm(obj, GLASS);
	} else if(has_template(mtmp, TOMB_HERD)) {
		obj = mkcorpstat(STATUE, (struct monst *)0,
			mdat, x, y, FALSE);
	} else switch(mndx) {
	    case PM_LICH__THE_FIEND_OF_EARTH:
			// if(mvitals[PM_GARLAND].died){
				// otmp = mksobj_at(CRYSTAL_BALL, x, y, MKOBJ_NOINIT);
				// otmp = oname(otmp, artiname(ART_EARTH_CRYSTAL));		
				// curse(otmp);
				// otmp->oerodeproof = TRUE;
			// }
		goto default_1;
		break;
	    case PM_KARY__THE_FIEND_OF_FIRE:
			// if(mvitals[PM_GARLAND].died){
				// otmp = mksobj_at(CRYSTAL_BALL, x, y, MKOBJ_NOINIT);
				// otmp = oname(otmp, artiname(ART_FIRE_CRYSTAL));		
				// curse(otmp);
				// otmp->oerodeproof = TRUE;
			// }
		goto default_1;
		break;
	    case PM_KRAKEN__THE_FIEND_OF_WATER:
			// if(mvitals[PM_GARLAND].died){
				// otmp = mksobj_at(CRYSTAL_BALL, x, y, MKOBJ_NOINIT);
				// otmp = oname(otmp, artiname(ART_WATER_CRYSTAL));		
				// curse(otmp);
				// otmp->oerodeproof = TRUE;
			// }
		goto default_1;
		break;
	    case PM_TIAMAT__THE_FIEND_OF_WIND:
			// if(mvitals[PM_GARLAND].died){
				// otmp = mksobj_at(CRYSTAL_BALL, x, y, MKOBJ_NOINIT);
				// otmp = oname(otmp, artiname(ART_AIR_CRYSTAL));		
				// curse(otmp);
				// otmp->oerodeproof = TRUE;
			// }
		goto default_1;
		break;
	    case PM_CHAOS:
			// otmp = mksobj_at(CRYSTAL_BALL, x, y, MKOBJ_NOINIT);
			// otmp = oname(otmp, artiname(ART_BLACK_CRYSTAL));		
			// curse(otmp);
			// otmp->oerodeproof = TRUE;
		goto default_1;
		break;
	    case PM_GRAY_DRAGON:
	    case PM_SILVER_DRAGON:
	    case PM_SHIMMERING_DRAGON:
	    case PM_RED_DRAGON:
	    case PM_ORANGE_DRAGON:
	    case PM_WHITE_DRAGON:
	    case PM_BLACK_DRAGON:
	    case PM_BLUE_DRAGON:
	    case PM_GREEN_DRAGON:
	    case PM_YELLOW_DRAGON:
	    case PM_DEEP_DRAGON:
		/* Make dragon scales.  This assumes that the order of the */
		/* dragons is the same as the order of the scales.	   */
		if (!rn2(Role_if(PM_CAVEMAN) ? (mtmp->mrevived ? 14 : 2) : (mtmp->mrevived ? 20 : 3))) {
		    num = GRAY_DRAGON_SCALES + monsndx(mdat) - PM_GRAY_DRAGON;
		    obj = mksobj_at(num, x, y, MKOBJ_NOINIT);
		    obj->spe = 0;
		    obj->cursed = obj->blessed = FALSE;
		}
		goto default_1;
	    case PM_CHROMATIC_DRAGON:
		    obj = mksobj_at(BLACK_DRAGON_SCALES, x, y, MKOBJ_NOINIT);
			obj = oname(obj, artiname(ART_CHROMATIC_DRAGON_SCALES));
		goto default_1;
	    case PM_PLATINUM_DRAGON:
		    obj = mksobj_at(SILVER_DRAGON_SCALE_MAIL, x, y, MKOBJ_NOINIT);
			obj = oname(obj, artiname(ART_DRAGON_PLATE));
		goto default_1;
	    case PM_MANTICORE:
		if (mtmp->mrevived ? (Role_if(PM_CAVEMAN) ? rn2(2) : !rn2(6)) : TRUE) {
			obj = mksobj_at(SPIKE, x, y, NO_MKOBJ_FLAGS);
			set_material_gm(obj, BONE);
			obj->blessed = 0;
			obj->cursed = 0;
			obj->quan = d(4,6);
			obj->spe = 0;
			obj->opoisoned = (OPOISON_PARAL);
		}
		goto default_1;
	    case PM_SON_OF_TYPHON:
		if (!rn2(Role_if(PM_CAVEMAN) ? (mtmp->mrevived ? 14 : 2) : (mtmp->mrevived ? 20 : 3))) {
			obj = mksobj_at(LEO_NEMAEUS_HIDE, x, y, MKOBJ_NOINIT);
		    obj->spe = 0;
		    obj->cursed = obj->blessed = FALSE;
		}
		goto default_1;

	    case PM_WATER_ELEMENTAL:
		if (levl[mtmp->mx][mtmp->my].typ == ROOM) {
		    levl[mtmp->mx][mtmp->my].typ = PUDDLE;
		    water_damage(level.objects[mtmp->mx][mtmp->my], FALSE, TRUE, level.flags.lethe, 0);
			newsym(mtmp->mx, mtmp->my);
		}
		goto default_1;

	    case PM_SARA__THE_LAST_ORACLE:
		if (mtmp->mrevived) {
			if (canseemon(mtmp))
			   pline("%s recently returned eyes vanish once more.",
				s_suffix(Monnam(mtmp)));
		} else {
			if (canseemon(mtmp))
			   pline("%s eyes vanish.",
				s_suffix(Monnam(mtmp)));
		}
		goto default_1;
	    case PM_ORACLE:
		if (mtmp->mrevived) {
			if (canseemon(mtmp))
			   pline("%s recently returned eyes vanish once more.",
				s_suffix(Monnam(mtmp)));
		} else {
			if (canseemon(mtmp))
			   pline("%s eyes vanish.",
				s_suffix(Monnam(mtmp)));
		}
		goto default_1;
	    case PM_DIRE_SHEEP:
		if (!mtmp->mrevived && find_pcloth() > 0 && Role_if(PM_CAVEMAN) ? !rn2(3) : !rn2(20)) {
			obj = mksobj_at(find_pcloth(), x, y, MKOBJ_NOINIT);
		    obj->spe = 0;
		    obj->cursed = obj->blessed = FALSE;
			add_oprop(obj, OPROP_WOOL);
		}
		goto default_1;

	    case PM_HARROWER_OF_ZARIEL:
			for(num = rnd(4); num > 0; num--){
				otmp = mksobj_at(SPEAR, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(otmp, SILVER);
				add_oprop(otmp, OPROP_LESSER_HOLYW);
				otmp->spe = 7;
				bless(otmp);
			}
			for(num = rnd(4); num > 0; num--){
				otmp = mksobj_at(SPEAR, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(otmp, IRON);
				add_oprop(otmp, OPROP_LESSER_UNHYW);
				otmp->obj_color = CLR_BLACK;
				otmp->spe = 7;
				curse(otmp);
			}
		break;

	    case PM_WHITE_UNICORN:
	    case PM_GRAY_UNICORN:
	    case PM_BLACK_UNICORN:
		if (mtmp->mrevived && rn2(20)) {
			if (canseemon(mtmp))
			   pline("%s recently regrown horn crumbles to dust.",
				s_suffix(Monnam(mtmp)));
		} else
			(void) mksobj_at(UNICORN_HORN, x, y, NO_MKOBJ_FLAGS);
		goto default_1;
//		case PM_UNICORN_OF_AMBER:{
//				int spe2;
			    /* create special stuff; can't use mongets */
//			    otmp = mksobj(UNICORN_HORN, NO_MKOBJ_FLAGS);
//				otmp = oname(otmp, artiname(ART_AMBER_HORN));		
//			    curse(otmp);
//			    otmp->oerodeproof = TRUE;
//			    spe2 = rn2(4)-3;
//			    otmp->spe = spe2;
//				place_object(otmp, x, y);
//			}
//		goto default_1;
//		case PM_PINK_UNICORN:{
//			int spe2;
		    /* create special stuff; can't use mongets */
//		    otmp = mksobj(UNICORN_HORN, NO_MKOBJ_FLAGS);
//			otmp = oname(otmp, artiname(ART_WHITE_PINK_HORN));
//			
//		    curse(otmp);
//		    otmp->oerodeproof = TRUE;
//		    spe2 = rn2(4)-3;
//		    otmp->spe = spe2;
//			place_object(otmp, x, y);
//		  }
//		goto default_1;
		case PM_NIGHTMARE:
			{
			int spe2;
		    /* create special stuff; can't use mongets */
		    otmp = mksobj(UNICORN_HORN, NO_MKOBJ_FLAGS);
			otmp = oname(otmp, artiname(ART_NIGHTHORN));
		
		    curse(otmp);
		    otmp->oerodeproof = TRUE;
		    spe2 = rn2(4)-3;
		    otmp->spe = spe2;
			place_object(otmp, x, y);
			}
		goto default_1;
		case PM_VECNA:
			{
			if(!rn2(2)){
				pline("All that remains is a hand...");
				otmp = oname(mksobj(SEVERED_HAND, 0),
						artiname(ART_HAND_OF_VECNA));
			} else {
				pline("All that remains is a single eye...");
				otmp = oname(mksobj(EYEBALL, 0),
						artiname(ART_EYE_OF_VECNA));
			}
		    /* create special stuff; can't use mongets */
			
		    curse(otmp);
		    otmp->oerodeproof = TRUE;
			place_object(otmp, x, y);
			}
		goto default_1;
		case PM_SCORPION:
			if (!rn2(Role_if(PM_CAVEMAN) ? 3 : 20) && !(
				(Role_if(PM_RANGER) && In_quest(&u.uz)) ||
				(art_already_exists(ART_SCORPION_CARAPACE)) ||
				(mtmp->mrevived && rn2(20))
				)) {
				otmp = oname(mksobj(SCALE_MAIL, 0), artiname(ART_SCORPION_CARAPACE));
				place_object(otmp, x, y);
			}
		goto default_1;
	    case PM_LONG_WORM:
			(void) mksobj_at(WORM_TOOTH, x, y, NO_MKOBJ_FLAGS);
		goto default_1;
	    case PM_VAMPIRE:
	    case PM_VAMPIRE_LORD:
	    case PM_VAMPIRE_LADY:
		/* include mtmp in the mkcorpstat() call */
		if(mtmp->mpetitioner 
			&& !is_rider(mtmp->data) 
			&& !(uwep && uwep->oartifact == ART_SINGING_SWORD && uwep->osinging == OSING_LIFE && mtmp->mtame)
		) 
			break;
		num = undead_to_corpse(mndx);
		obj = mkcorpstat(CORPSE, mtmp, &mons[num], x, y, TRUE);
		obj->age -= 100;		/* this is an *OLD* corpse */
		break;
	    case PM_KOBOLD_MUMMY:
	    case PM_DWARF_MUMMY:
	    case PM_GNOME_MUMMY:
	    case PM_ORC_MUMMY:
	    case PM_ELF_MUMMY:
	    case PM_HUMAN_MUMMY:
	    case PM_DROW_MUMMY:
	    case PM_HALF_DRAGON_MUMMY:
	    case PM_GIANT_MUMMY:
	    case PM_ETTIN_MUMMY:
	    // case PM_KOBOLD_ZOMBIE:
	    // case PM_DWARF_ZOMBIE:
	    // case PM_GNOME_ZOMBIE:
	    // case PM_ORC_ZOMBIE:
	    // case PM_ELF_ZOMBIE:
	    // case PM_HUMAN_ZOMBIE:
	    case PM_ZOMBIE:
	    case PM_HEDROW_ZOMBIE:
	    // case PM_HALF_DRAGON_ZOMBIE:
	    // case PM_GIANT_ZOMBIE:
	    // case PM_ETTIN_ZOMBIE:
	    case PM_ALABASTER_MUMMY:
		if(is_alabaster_mummy(mtmp->data) && mtmp->mvar_syllable >= SYLLABLE_OF_STRENGTH__AESH && mtmp->mvar_syllable <= SYLLABLE_OF_SPIRIT__VAUL){
			mksobj_at(mtmp->mvar_syllable, x, y, NO_MKOBJ_FLAGS);
			if(mtmp->mvar_syllable == SYLLABLE_OF_SPIRIT__VAUL)
				remove_mintrinsic(mtmp, DISPLACED);
			mtmp->mvar_syllable = 0; //Lose the bonus if resurrected
		}
		if(mtmp->mpetitioner 
			&& !is_rider(mtmp->data) 
			&& !(uwep && uwep->oartifact == ART_SINGING_SWORD && uwep->osinging == OSING_LIFE && mtmp->mtame)
		) //u.uevent.invoked || 
			break;
		num = undead_to_corpse(mndx);
		obj = mkcorpstat(CORPSE, mtmp, &mons[num], x, y, TRUE);
		break;
	    case PM_ALABASTER_CACTOID:
			obj = mkobj_at(TILE_CLASS, x, y, NO_MKOBJ_FLAGS);
			if(obj)
				curse(obj);
		break;
	    case PM_ARSENAL:
			num = d(3,6);
			while(num--){
				obj = mksobj_at(PLATE_MAIL, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, COPPER);
				obj->spe = 3;
			}
			num = d(2,4);
			while(num--)
				obj = mksobj_at(BALL, x, y, NO_MKOBJ_FLAGS);
			rem_mx(mtmp, MX_ENAM);
		    otmp = mksobj(MACE, NO_MKOBJ_FLAGS);
			otmp = oname(otmp, artiname(ART_FIELD_MARSHAL_S_BATON));
		    otmp->oerodeproof = TRUE;
		    otmp->spe = -3;
			place_object(otmp, x, y);
		break;
	    case PM_TINKER_GNOME:
			obj = mksobj_at(CLOCKWORK_COMPONENT, x, y, NO_MKOBJ_FLAGS);
			obj->quan = d(1,4);
			obj->owt = weight(obj);
			if(!mtmp->mrevived && !rn2(Role_if(PM_CAVEMAN) ? 9 : 20)){
				obj = mksobj_at(UPGRADE_KIT, x, y, NO_MKOBJ_FLAGS);
			} else if(!mtmp->mrevived && !rn2(Role_if(PM_CAVEMAN) ? 8 : 19)){
				obj = mksobj_at(TINNING_KIT, x, y, NO_MKOBJ_FLAGS);
			} else if(!mtmp->mrevived && !rn2(Role_if(PM_CAVEMAN) ? 3 : 10)){
				obj = mksobj_at(CAN_OF_GREASE, x, y, NO_MKOBJ_FLAGS);
			}
			rem_mx(mtmp, MX_ENAM);
		goto default_1;
	    case PM_CLOCKWORK_DWARF:
			obj = mksobj_at(CLOCKWORK_COMPONENT, x, y, NO_MKOBJ_FLAGS);
			obj->quan = d(1,4);
			obj->owt = weight(obj);
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_FABERGE_SPHERE:
			obj = mksobj_at(CLOCKWORK_COMPONENT, x, y, NO_MKOBJ_FLAGS);
			obj->quan = d(1,3);
			obj->owt = weight(obj);
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_FIREWORK_CART:
			obj = mksobj_at(CLOCKWORK_COMPONENT, x, y, NO_MKOBJ_FLAGS);
			obj->quan = d(1,4);
			obj->owt = weight(obj);
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_CLOCKWORK_SOLDIER:
			obj = mksobj_at(CLOCKWORK_COMPONENT, x, y, NO_MKOBJ_FLAGS);
			obj->quan = d(1,3);
			obj->owt = weight(obj);
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_GOLDEN_HEART:
			obj = mksobj_at(CLOCKWORK_COMPONENT, x, y, NO_MKOBJ_FLAGS);
			obj->quan = d(1,4);
			obj->owt = weight(obj);
			obj = mksobj_at(SUBETHAIC_COMPONENT, x, y, NO_MKOBJ_FLAGS);
			obj->quan = 1;
			obj->owt = weight(obj);
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_JUGGERNAUT:
			obj = mksobj_at(CLOCKWORK_COMPONENT, x, y, NO_MKOBJ_FLAGS);
			obj->quan = d(3,4);
			obj->owt = weight(obj);
			if(!rn2(Role_if(PM_CAVEMAN) ? 3 : 20)){
				obj = mksobj_at(TINNING_KIT, x, y, NO_MKOBJ_FLAGS);
			} else if(!rn2(Role_if(PM_CAVEMAN) ? 2 : 10)){
				obj = mksobj_at(CAN_OF_GREASE, x, y, NO_MKOBJ_FLAGS);
			}
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_CLOCKWORK_FACTORY:
			obj = mksobj_at(CLOCKWORK_COMPONENT, x, y, NO_MKOBJ_FLAGS);
			obj->quan = d(3,4);
			obj->owt = weight(obj);
			if(!rn2(Role_if(PM_CAVEMAN) ? 9 : 20)){
				obj = mksobj_at(UPGRADE_KIT, x, y, NO_MKOBJ_FLAGS);
			} else if(!rn2(Role_if(PM_CAVEMAN) ? 8 : 19)){
				obj = mksobj_at(TINNING_KIT, x, y, NO_MKOBJ_FLAGS);
			} else if(!rn2(Role_if(PM_CAVEMAN) ? 3 : 10)){
				obj = mksobj_at(CAN_OF_GREASE, x, y, NO_MKOBJ_FLAGS);
			}
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_ID_JUGGERNAUT:
			obj = mksobj_at(CLOCKWORK_COMPONENT, x, y, NO_MKOBJ_FLAGS);
			obj->quan = d(4,4);
			obj->owt = weight(obj);
			obj = mksobj_at(SUBETHAIC_COMPONENT, x, y, NO_MKOBJ_FLAGS);
			obj->quan = d(1,4);
			obj->owt = weight(obj);
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_SCRAP_TITAN:
			obj = mksobj_at(CLOCKWORK_COMPONENT, x, y, NO_MKOBJ_FLAGS);
			obj->quan = d(4,4);
			obj->owt = weight(obj);
			rem_mx(mtmp, MX_ENAM);
			num = d(2,4);
			while (num--){
				obj = mksobj_at(CHAIN, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, IRON);
				obj->oeroded = 3;
				obj = mksobj_at(CHAIN, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, IRON);
				obj->oeroded = 3;
				obj = mksobj_at(BAR, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, IRON);
				obj->oeroded = 3;
				obj = mksobj_at(SCRAP, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, IRON);
				obj->oeroded = 3;
				obj = mksobj_at(SCRAP, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, IRON);
				obj->oeroded = 3;
				obj = mksobj_at(SCRAP, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, IRON);
				obj->oeroded = 3;
			}
		break;
	    case PM_HELLFIRE_COLOSSUS:
			obj = mksobj_at(CLOCKWORK_COMPONENT, x, y, NO_MKOBJ_FLAGS);
			obj->quan = d(4,4);
			obj->owt = weight(obj);
			obj = mksobj_at(HELLFIRE_COMPONENT, x, y, NO_MKOBJ_FLAGS);
			obj->quan = d(4,4);
			obj->owt = weight(obj);
			num = d(2,6);
			while (num--){
				obj = mksobj_at(CHAIN, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, IRON);
				obj = mksobj_at(CHAIN, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, IRON);
				obj = mksobj_at(BAR, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, IRON);
			}
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_APHANACTONAN_AUDIENT:
			obj = mksobj_at(CLOCKWORK_COMPONENT, x, y, NO_MKOBJ_FLAGS);
			obj->quan = d(2,4);
			obj->owt = weight(obj);
			rem_mx(mtmp, MX_ENAM);
			if(!rn2(Role_if(PM_CAVEMAN) ? 3 : 20))
				obj = mksobj_at(UPGRADE_KIT, x, y, NO_MKOBJ_FLAGS);
			obj = mksobj_at(APHANACTONAN_RECORD, x, y, NO_MKOBJ_FLAGS);
		break;
	    case PM_APHANACTONAN_ASSESSOR:
			obj = mksobj_at(CLOCKWORK_COMPONENT, x, y, NO_MKOBJ_FLAGS);
			obj->quan = d(4,8);
			obj->owt = weight(obj);
			rem_mx(mtmp, MX_ENAM);
			obj = mksobj_at(UPGRADE_KIT, x, y, NO_MKOBJ_FLAGS);
			num = d(4,4);
			while (num--){
				obj = mksobj_at(EYEBALL, x, y, MKOBJ_NOINIT);
				obj->corpsenm = PM_BEHOLDER;
			}
			num = d(2,4);
			while(num--){
				obj = mksobj_at(ARCHAIC_PLATE_MAIL, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, COPPER);
				obj->spe = 4;
			}
			obj = mksobj_at(CORPSE, x, y, NO_MKOBJ_FLAGS);
			obj->corpsenm = PM_FLOATING_EYE;
			fix_object(obj);
			obj = mksobj_at(APHANACTONAN_ARCHIVE, x, y, NO_MKOBJ_FLAGS);
		break;
	    case PM_PARASITIZED_DOLL:
			num = d(2,4);
			while (num--){
				obj = mksobj_at(EYEBALL, x, y, MKOBJ_NOINIT);
				obj->corpsenm = PM_PARASITIZED_DOLL;
			}
			
			mon = makemon(&mons[PM_LIVING_DOLL], x, y, MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
			if(mon){
				if(mtmp->m_insight_level)
					mon->m_insight_level = mtmp->m_insight_level;
				obj = mkcorpstat(LIFELESS_DOLL, mon, (struct permonst *)0, x, y, FALSE);
				if(obj)
					obj->ovar1_insightlevel = mon->m_insight_level;
				mongone(mon);
			}
		break;
	    case PM_PARASITIZED_KNIGHT:
			obj = mksobj_at(SUNLIGHT_MAGGOT, x, y, MKOBJ_NOINIT);
			if(mtmp->mpetitioner 
				&& !is_rider(mtmp->data) 
			) //u.uevent.invoked || 
				break;
			if(rn2(4)){
				struct obj *otmp = mksobj_at(CORPSE, x, y, MKOBJ_NOINIT);
				otmp->corpsenm = PM_ZOMBIE;
				fix_object(otmp);
			}
			else {
				mon = makemon(&mons[PM_KNIGHT], x, y, MM_EDOG | MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
				if (mon){
					initedog(mon);
					mon->mtame = 10;
					mon->mpeaceful = 1;
					mon->mconf = 1;
					EDOG(mon)->waspeaceful = TRUE;
					mon->mpeacetime = 0;
					mon->female = mtmp->female;
					mkcorpstat(CORPSE, mon, (struct permonst *)0, x, y, FALSE);
					mongone(mon);
				}
			}
		break;
	    case PM_LIVING_DOLL:
		    obj = mkcorpstat(LIFELESS_DOLL, KEEPTRAITS(mtmp) ? mtmp : 0,
				     mdat, x, y, TRUE);
			if(obj)
				obj->ovar1_insightlevel = mtmp->m_insight_level;
		break;
	    case PM_ANDROID:
		    obj = mkcorpstat(BROKEN_ANDROID, KEEPTRAITS(mtmp) ? mtmp : 0,
				     mdat, x, y, TRUE);
			if(obj)
				obj->ovar1_insightlevel = mtmp->m_insight_level;
		break;
	    case PM_CRUCIFIED_ANDROID:
			obj = mksobj_at(BAR, x, y, MKOBJ_NOINIT);
			set_material_gm(obj, IRON);
			obj->oeroded = 1;
			obj = mksobj_at(BAR, x, y, MKOBJ_NOINIT);
			set_material_gm(obj, IRON);
			obj->oeroded = 1;
			mon = makemon(&mons[PM_ANDROID], x, y, MM_EDOG | MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
			if (mon){
				initedog(mon);
				mon->mtame = 10;
				mon->mpeaceful = 1;
				set_template(mon, M_BLACK_WEB);
				EDOG(mon)->loyal = TRUE;
				EDOG(mon)->waspeaceful = TRUE;
				mon->mpeacetime = 0;
				obj = mkcorpstat(BROKEN_ANDROID, mon, (struct permonst *)0, x, y, FALSE);
				mongone(mon);
			} else {
				obj = mksobj_at(BROKEN_ANDROID, x, y, MKOBJ_NOINIT);
			}
		break;
	    case PM_MUMMIFIED_ANDROID:
			mon = makemon(&mons[PM_ANDROID], x, y, MM_EDOG | MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
			if (mon){
				initedog(mon);
				mon->mtame = 10;
				mon->mpeaceful = 1;
				set_template(mon, ZOMBIFIED);
				EDOG(mon)->loyal = TRUE;
				obj = mkcorpstat(BROKEN_ANDROID, mon, (struct permonst *)0, x, y, FALSE);
				mongone(mon);
			} else {
				obj = mksobj_at(BROKEN_ANDROID, x, y, MKOBJ_NOINIT);
			}
		break;
	    case PM_FLAYED_ANDROID:
			obj = mkcorpstat(BROKEN_ANDROID, mtmp, mdat, x, y, FALSE);
		break;
	    case PM_PARASITIZED_ANDROID:
			mon = makemon(&mons[PM_ANDROID], x, y, MM_EDOG | MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
			if (mon){
				initedog(mon);
				mon->mtame = 10;
				mon->mpeaceful = 1;
				mon->mcrazed = 1;
				EDOG(mon)->loyal = TRUE;
				EDOG(mon)->waspeaceful = TRUE;
				mon->mpeacetime = 0;
				mkcorpstat(BROKEN_ANDROID, mon, (struct permonst *)0, x, y, FALSE);
				mongone(mon);
			} else {
				obj = mksobj_at(BROKEN_ANDROID, x, y, MKOBJ_NOINIT);
			}
			if(mtmp->mpetitioner 
				&& !is_rider(mtmp->data) 
			) //u.uevent.invoked || 
				break;
			obj = mksobj_at(CORPSE, x, y, MKOBJ_NOINIT);
			obj->corpsenm = PM_PARASITIC_MIND_FLAYER;
			fix_object(obj);
		break;
	    case PM_GYNOID:
		    obj = mkcorpstat(BROKEN_GYNOID, KEEPTRAITS(mtmp) ? mtmp : 0,
				     mdat, x, y, TRUE);
			if(obj)
				obj->ovar1_insightlevel = mtmp->m_insight_level;
		break;
	    case PM_CRUCIFIED_GYNOID:
			obj = mksobj_at(BAR, x, y, MKOBJ_NOINIT);
			set_material_gm(obj, IRON);
			obj->oeroded = 1;
			obj = mksobj_at(BAR, x, y, MKOBJ_NOINIT);
			set_material_gm(obj, IRON);
			obj->oeroded = 1;
			mon = makemon(&mons[PM_GYNOID], x, y, MM_EDOG | MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
			if (mon){
				initedog(mon);
				mon->mtame = 10;
				mon->mpeaceful = 1;
				set_template(mon, M_BLACK_WEB);
				EDOG(mon)->loyal = TRUE;
				obj = mkcorpstat(BROKEN_GYNOID, mon, (struct permonst *)0, x, y, FALSE);
				mongone(mon);
			} else {
				obj = mksobj_at(BROKEN_GYNOID, x, y, MKOBJ_NOINIT);
			}
		break;
	    case PM_MUMMIFIED_GYNOID:
			mon = makemon(&mons[PM_GYNOID], x, y, MM_EDOG | MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
			if (mon){
				initedog(mon);
				mon->mtame = 10;
				mon->mpeaceful = 1;
				set_template(mon, ZOMBIFIED);
				EDOG(mon)->loyal = TRUE;
				obj = mkcorpstat(BROKEN_GYNOID, mon, (struct permonst *)0, x, y, FALSE);
				mongone(mon);
			} else {
				obj = mksobj_at(BROKEN_GYNOID, x, y, MKOBJ_NOINIT);
			}
		break;
	    case PM_FLAYED_GYNOID:
			obj = mkcorpstat(BROKEN_GYNOID, mtmp, mdat, x, y, FALSE);
		break;
	    case PM_PARASITIZED_GYNOID:
			mon = makemon(&mons[PM_GYNOID], x, y, MM_EDOG | MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
			if (mon){
				initedog(mon);
				mon->mtame = 10;
				mon->mpeaceful = 1;
				mon->mcrazed = 1;
				EDOG(mon)->loyal = TRUE;
				EDOG(mon)->waspeaceful = TRUE;
				mon->mpeacetime = 0;
				obj = mkcorpstat(BROKEN_GYNOID, mon, (struct permonst *)0, x, y, FALSE);
				mongone(mon);
			} else {
				obj = mksobj_at(BROKEN_GYNOID, x, y, MKOBJ_NOINIT);
			}
			if(mtmp->mpetitioner 
				&& !is_rider(mtmp->data) 
			) //u.uevent.invoked || 
				break;
			obj = mksobj_at(CORPSE, x, y, MKOBJ_NOINIT);
			obj->corpsenm = PM_PARASITIC_MIND_FLAYER;
			fix_object(obj);
		break;
	    case PM_OPERATOR:
		    obj = mkcorpstat(BROKEN_GYNOID, mtmp, mdat, x, y, TRUE);
			if(obj)
				obj->ovar1_insightlevel = mtmp->m_insight_level;
		break;
	    case PM_PARASITIZED_OPERATOR:
			mon = makemon(&mons[PM_OPERATOR], x, y, MM_EDOG | MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
			if (mon){
				initedog(mon);
				mon->mtame = 10;
				mon->mpeaceful = 1;
				mon->mcrazed = 1;
				EDOG(mon)->loyal = TRUE;
				EDOG(mon)->waspeaceful = TRUE;
				mon->mpeacetime = 0;
				obj = mkcorpstat(BROKEN_GYNOID, mon, (struct permonst *)0, x, y, FALSE);
				mongone(mon);
			} else {
				obj = mksobj_at(BROKEN_GYNOID, x, y, MKOBJ_NOINIT);
			}
			if(mtmp->mpetitioner 
				&& !is_rider(mtmp->data) 
			) //u.uevent.invoked || 
				break;
			obj = mksobj_at(CORPSE, x, y, MKOBJ_NOINIT);
			obj->corpsenm = PM_PARASITIC_MIND_FLAYER;
			fix_object(obj);
		break;
	    case PM_COMMANDER:
		    obj = mkcorpstat(BROKEN_GYNOID, mtmp, mdat, x, y, TRUE);
			if(obj)
				obj->ovar1_insightlevel = mtmp->m_insight_level;
		break;
	    case PM_PARASITIZED_COMMANDER:
			obj = mksobj_at(SHACKLES, x, y, MKOBJ_NOINIT);
			set_material_gm(obj, IRON);
			add_oprop(obj, OPROP_ELECW);
			obj->oeroded = 1;
			mon = makemon(&mons[PM_COMMANDER], x, y, MM_EDOG | MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
			if (mon){
				initedog(mon);
				mon->mtame = 10;
				mon->mpeaceful = 1;
				mon->mcrazed = 1;
				mon->mcansee = 0;
				mon->mcanhear = 0;
				EDOG(mon)->loyal = TRUE;
				EDOG(mon)->waspeaceful = TRUE;
				mon->mpeacetime = 0;
				set_template(mon, M_GREAT_WEB);
				obj = mkcorpstat(BROKEN_GYNOID, mon, (struct permonst *)0, x, y, FALSE);
				mongone(mon);
			} else {
				obj = mksobj_at(BROKEN_GYNOID, x, y, MKOBJ_NOINIT);
			}
			if(mtmp->mpetitioner 
				&& !is_rider(mtmp->data) 
			) //u.uevent.invoked || 
				break;
			obj = mksobj_at(CORPSE, x, y, MKOBJ_NOINIT);
			obj->corpsenm = PM_PARASITIC_MIND_FLAYER;
			fix_object(obj);
		break;
	    case PM_BRAINBLOSSOM_PATCH:{
			struct trap *ttmp = t_at(x, y);
			obj = mksobj_at(BRAINROOT, x, y, MKOBJ_NOINIT);
			if(obj && !(ttmp && (ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT || ttmp->ttyp == HOLE))){
				bury_an_obj(obj);
			}
			obj = (struct obj *)0;
			goto default_1;
		}break;
	    case PM_IRON_GOLEM:
			obj = mksobj_at(PLATE_MAIL, x, y, MKOBJ_NOINIT);
			set_material_gm(obj, IRON);
			obj->objsize = MZ_LARGE;
			fix_object(obj);
			num = d(2,6);
			while (num--){
				obj = mksobj_at(CHAIN, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, IRON);
				obj = mksobj_at(KITE_SHIELD, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, IRON);
				obj = mksobj_at(BAR, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, IRON);
			}
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_GREEN_STEEL_GOLEM:
			obj = mksobj_at(PLATE_MAIL, x, y, MKOBJ_NOINIT);
			set_material_gm(obj, GREEN_STEEL);
			obj->objsize = MZ_LARGE;
			fix_object(obj);
			num = d(2,6);
			while (num--){
				obj = mksobj_at(rn2(3) ? CHAIN : SHACKLES, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, GREEN_STEEL);
				obj = mksobj_at(KITE_SHIELD, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, GREEN_STEEL);
				obj = mksobj_at(BAR, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, GREEN_STEEL);
			}
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_CHAIN_GOLEM:
			num = d(5,6);
			while (num--){
				obj = mksobj_at(CHAIN, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, IRON);
			}
			num = d(1,6);
			while (num--){
				obj = mksobj_at(SHACKLES, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, IRON);
			}
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_ARGENTUM_GOLEM:
			num = d(1,3);
			while (num--){
				obj = mksobj_at(SILVER_SLINGSTONE, x, y, NO_MKOBJ_FLAGS);
				set_obj_quan(obj, d(10, 5));
			}
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_GLASS_GOLEM:
			num = d(2,4);   /* very low chance of creating all glass gems */
			while (num--)
				obj = mksobj_at((LAST_GEM + rnd(9)), x, y, NO_MKOBJ_FLAGS);
				rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_CLAY_GOLEM:
			obj = mksobj_at(ROCK, x, y, MKOBJ_NOINIT);
			set_obj_quan(obj, (rn2(20) + 50));
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_STONE_GOLEM:
			obj = mkcorpstat(STATUE, (struct monst *)0,
				mdat, x, y, FALSE);
		break;
	    case PM_JRT_NETJER:
			if(has_template(mtmp, POISON_TEMPLATE)){
				(void) mksobj_at(FANG_OF_APEP, x, y, MKOBJ_NOINIT);
			}
		break;
	    case PM_TERAPHIM_TANNAH:
			obj = mkcorpstat(STATUE, (struct monst *)0,
				mdat, x, y, FALSE);
			if(obj){
				obj->corpsenm = PM_MARILITH;
				fix_object(obj);
			}
		break;
	    case PM_SENTINEL_OF_MITHARDIR:
			obj = mkcorpstat(STATUE, (struct monst *)0,
				mdat, x, y, FALSE);
			if(obj){
				obj->corpsenm = PM_ALABASTER_ELF;
				fix_object(obj);
			}
		break;
	    case PM_WOOD_GOLEM:
			num = d(2,4);
			while(num--) {
				obj = mksobj_at(QUARTERSTAFF, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, WOOD);
			}
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_GROVE_GUARDIAN:
			num = d(3,4);
			while(num--) {
				obj = mksobj_at(QUARTERSTAFF, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, WOOD);
				obj->spe = rnd(3);
			}
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_LIVING_LECTERN:
			num = d(2,3);
			while(num--) {
				obj = mksobj_at(CLUB, x, y, NO_MKOBJ_FLAGS);
				set_material_gm(obj, WOOD);
			}
			obj = mkobj_at(SPBOOK_CLASS, x, y, NO_MKOBJ_FLAGS);
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_LEATHER_GOLEM:
			num = d(2,4);
			while(num--)
				obj = mksobj_at(LEATHER_ARMOR, x, y, NO_MKOBJ_FLAGS);
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_GOLD_GOLEM:
			/* Good luck gives more coins */
			obj = mkgold((long)(200 - rnl(101)), x, y);
			rem_mx(mtmp, MX_ENAM);
			break;
	    case PM_ARA_KAMEREL:
			/* Ara Kamerel are projecting their images into gold golems */
			obj = mkgold((long)(200 - rnl(101)), x, y);
			rem_mx(mtmp, MX_ENAM);
			break;
		case PM_TREASURY_GOLEM:
			num = d(2,4); 
			while (num--)
				obj = mksobj_at((LAST_GEM - rnd(9)), x, y, NO_MKOBJ_FLAGS);
			rem_mx(mtmp, MX_ENAM);
			/* Good luck gives more coins */
			obj = mkgold((long)(400 - rnl(101)), x, y);
			rem_mx(mtmp, MX_ENAM);
		break;
		case PM_PAPER_GOLEM:
			num = rnd(4);
			while (num--)
				obj = mksobj_at(SCR_BLANK_PAPER, x, y, NO_MKOBJ_FLAGS);
			rem_mx(mtmp, MX_ENAM);
		break;
		case PM_ZHI_REN_MONK:
			obj = mksobj_at(rn2(20) ? SCR_BLANK_PAPER : EFFIGY, x, y, NO_MKOBJ_FLAGS);
			if(obj) set_material_gm(obj, PAPER);
			rem_mx(mtmp, MX_ENAM);
		break;
	    case PM_STRAW_GOLEM:
		if(!rn2(10)) mksobj_at(SEDGE_HAT, x, y, MKOBJ_NOINIT);
		obj = mksobj_at(SHEAF_OF_HAY, x, y, MKOBJ_NOINIT);
		obj->quan = (long)(d(2,4));
		obj->owt = weight(obj);
		rem_mx(mtmp, MX_ENAM);
		break;
		case PM_SPELL_GOLEM:{
			int scrnum = 0;
			int scrrng = SCR_STINKING_CLOUD-SCR_ENCHANT_ARMOR;
			num = rnd(8);
			while (num--){
				scrnum = d(1, scrrng)-1;
				obj = mksobj_at(scrnum+SCR_ENCHANT_ARMOR, x, y, NO_MKOBJ_FLAGS);
			}
			rem_mx(mtmp, MX_ENAM);
		}
		break;
	    case PM_GARO:
			if(!rn2(100)){
				obj = mksobj_at(MASK, x, y, NO_MKOBJ_FLAGS);
				obj->corpsenm = PM_GARO;
			}
		goto default_1;
		break;
	    case PM_GARO_MASTER:
			obj = mksobj_at(MASK, x, y, NO_MKOBJ_FLAGS);
			obj->corpsenm = PM_GARO_MASTER;
		goto default_1;
		break;
	    case PM_COILING_BRAWN:{
			obj = mksobj_at(EYEBALL, x, y, MKOBJ_NOINIT);
			obj->corpsenm = mtmp->mvar_lifesigns ? genus(mtmp->mvar_lifesigns, FALSE) : PM_HUMAN;
			obj->quan = 2;
			obj->owt = weight(obj);
		}
		goto default_1;
	    case PM_CHANGED:
			create_gas_cloud(x, y, 4, rnd(3)+1, FALSE);
			obj = mksobj_at(EYEBALL, x, y, MKOBJ_NOINIT);
			obj->corpsenm = humanoid_eyes[rn2(SIZE(humanoid_eyes))];
			obj->quan = 2;
			obj->owt = weight(obj);
		goto default_1;
		break;
	    case PM_WARRIOR_CHANGED:
			create_gas_cloud(x, y, 5, rnd(3)+1, FALSE);
			num = rn1(10,10);
			while (num--){
				obj = mksobj_at(EYEBALL, x, y, MKOBJ_NOINIT);
				obj->corpsenm = humanoid_eyes[rn2(SIZE(humanoid_eyes))];
			}
		goto default_1;
		break;
	    case PM_TWITCHING_FOUR_ARMED_CHANGED:
			create_gas_cloud(x, y, 4, rnd(3)+1, FALSE);
			obj = mksobj_at(EYEBALL, x, y, MKOBJ_NOINIT);
			obj->corpsenm = PM_MYRKALFR;
			obj->quan = 2;
			obj = mksobj_at(EYEBALL, x, y, MKOBJ_NOINIT);
			obj->corpsenm = PM_ELF;
			obj->quan = 2;
			num = rn1(10,10);
			while (num--){
				obj = mksobj_at(EYEBALL, x, y, MKOBJ_NOINIT);
				obj->corpsenm = humanoid_eyes[rn2(SIZE(humanoid_eyes))];
			}
			num = rn1(10,10);
			while (num--){
				obj = mksobj_at(WORM_TOOTH, x, y, MKOBJ_NOINIT);
				add_oprop(obj, OPROP_LESSER_FLAYW);
			}
		goto default_1;
		break;
	    case PM_CLAIRVOYANT_CHANGED:
			create_gas_cloud(x, y, 4, rnd(3)+1, FALSE);
			obj = mksobj_at(EYEBALL, x, y, MKOBJ_NOINIT);
			obj->corpsenm = PM_HUMAN;
			obj->quan = 2;
			obj->owt = weight(obj);
			obj->oartifact = ART_EYE_OF_THE_ORACLE;
			obj = mksobj_at(EYEBALL, x, y, MKOBJ_NOINIT);
			obj->corpsenm = PM_HUMAN;
			obj->quan = 14;
			obj->owt = weight(obj);
			obj = mksobj_at(EYEBALL, x, y, MKOBJ_NOINIT);
			obj->corpsenm = PM_HUMAN;
			obj->quan = 4;
			obj->owt = weight(obj);
		goto default_1;
		break;
	    case PM_EMBRACED_DROWESS:
			if(mtmp->mpetitioner 
				&& !is_rider(mtmp->data) 
				&& !(uwep && uwep->oartifact == ART_SINGING_SWORD && uwep->osinging == OSING_LIFE && mtmp->mtame)
			) //u.uevent.invoked || 
				break;
			mon = makemon(&mons[PM_DROW_CAPTAIN], x, y, MM_EDOG | MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
			if (mon){
				initedog(mon);
				mon->mhpmax = (mon->m_lev * 8) - 4;
				mon->mhp = mon->mhpmax;
				mon->female = TRUE;
				mon->mtame = 10;
				mon->mpeaceful = 1;
				set_template(mon, M_BLACK_WEB);
			}
			obj = mkcorpstat(CORPSE, mon, (struct permonst *)0, x, y, FALSE);
			mongone(mon);
		break;
	    case PM_PARASITIZED_EMBRACED_ALIDER:
			mon = makemon(&mons[PM_ALIDER], x, y, MM_EDOG | MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
			if (mon){
				initedog(mon);
				set_template(mon, M_GREAT_WEB);
				mon->mhpmax = (mon->m_lev * 1);
				mon->mhp = mon->mhpmax;
				mon->female = TRUE;
				mon->mtame = 10;
				mon->mpeaceful = 1;
				mon->mcrazed = 1;
				mon->mcansee = 0;
				mon->mcanhear = 0;
				set_mcan(mon, TRUE);
				mon->mdoubt = 1;
				if(EDOG(mon)){
					EDOG(mon)->loyal = TRUE;
					EDOG(mon)->waspeaceful = TRUE;
					mon->mpeacetime = 0;
				}
				mkcorpstat(CORPSE, mon, (struct permonst *)0, x, y, FALSE);
				mongone(mon);
			}
			obj = mksobj_at(DROVEN_CLOAK, x, y, MKOBJ_NOINIT);
			obj->objsize = MZ_LARGE;
			fix_object(obj);
			obj = 0;
			if(mtmp->mpetitioner 
				&& !is_rider(mtmp->data) 
				&& !(uwep && uwep->oartifact == ART_SINGING_SWORD && uwep->osinging == OSING_LIFE && mtmp->mtame)
			) //u.uevent.invoked || 
				break;
			obj = mksobj_at(CORPSE, x, y, MKOBJ_NOINIT);
			obj->corpsenm = PM_PARASITIC_MASTER_MIND_FLAYER;
			fix_object(obj);
		break;
	    case PM_WARDEN_ARIANNA:
			mon = makemon(&mons[PM_ARIANNA], x, y, MM_EDOG | MM_ADJACENTOK | NO_MINVENT);
			if (mon){
				initedog(mon);
				mon->m_lev = mtmp->m_lev/2;
				mon->mhpmax = (mtmp->mhpmax+1)/2;
				mon->female = TRUE;
				mon->mtame = 10;
				mon->mpeaceful = 1;
				if(EDOG(mon)){
					EDOG(mon)->loyal = TRUE;
					EDOG(mon)->waspeaceful = TRUE;
					mon->mpeacetime = 0;
				}
				mkcorpstat(CORPSE, mon, (struct permonst *)0, x, y, FALSE);
				mongone(mon);
			}
		break;
	    default_1:
	    default:
		if (mvitals[mndx].mvflags & G_NOCORPSE)
		    return (struct obj *)0;
		if(mtmp->mpetitioner 
			&& !is_rider(mtmp->data) 
			&& !(uwep && uwep->oartifact == ART_SINGING_SWORD && uwep->osinging == OSING_LIFE && mtmp->mtame)
		) //u.uevent.invoked || 
			break;
		else if(u.sealsActive&SEAL_BERITH && !(mvitals[mndx].mvflags & G_UNIQ) 
				&& mtmp->m_lev > u.ulevel && !KEEPTRAITS(mtmp)
		){
			obj = mkgold((long)(mtmp->m_lev*25 - rnl(mtmp->m_lev*25/2+1)), x, y);
			rem_mx(mtmp, MX_ENAM);
		} else{	/* preserve the unique traits of some creatures */
//			pline("preserving unique traits");
		    obj = mkcorpstat(CORPSE, KEEPTRAITS(mtmp) ? mtmp : 0,
				     mdat, x, y, TRUE);
		}
		break;
	}
	if(obj && obj->otyp == CORPSE){
		if(is_undead(mtmp->data)){
			obj->age -= 100;		/* this is an *OLD* corpse */
		}
		/* All special cases should precede the G_NOCORPSE check */
		
		/* if polymorph or undead turning has killed this monster,
		   prevent the same attack beam from hitting its corpse */
		if (flags.bypasses) bypass_obj(obj);

		if (M_HAS_NAME(mtmp))
			obj = oname(obj, MNAME(mtmp));

		/* Avoid "It was hidden under a green mold corpse!" 
		 *  during Blind combat. An unseen monster referred to as "it"
		 *  could be killed and leave a corpse.  If a hider then hid
		 *  underneath it, you could be told the corpse type of a
		 *  monster that you never knew was there without this.
		 *  The code in hitmu() substitutes the word "something"
		 *  if the corpses obj->dknown is 0.
		 */
		if (Blind && !sensemon(mtmp)) obj->dknown = 0;

#ifdef INVISIBLE_OBJECTS
		/* Invisible monster ==> invisible corpse */
		obj->oinvis = mtmp->minvis;
#endif
		stackobj(obj);
	}
	newsym(x, y);
	
	return obj;
}

#endif /* OVLB */
#ifdef OVL1

#if 0
/* part of the original warning code which was replaced in 3.3.1 */
STATIC_OVL void
warn_effects()
{
    if (warnlevel == 100) {
	if(!Blind && uwep &&
	    (warnlevel > lastwarnlev || moves > lastwarntime + warnDelay)) {
	    Your("%s %s!", aobjnam(uwep, "glow"),
		hcolor(NH_LIGHT_BLUE));
	    lastwarnlev = warnlevel;
	    lastwarntime = moves;
	}
	warnlevel = 0;
	return;
    }

    if (warnlevel >= SIZE(warnings))
	warnlevel = SIZE(warnings)-1;
    if (!Blind &&
	    (warnlevel > lastwarnlev || moves > lastwarntime + warnDelay)) {
	const char *which, *what, *how;
	long rings = (EWarning & (LEFT_RING|RIGHT_RING));

	if (rings) {
	    what = Hallucination ? "mood ring" : "ring";
	    how = "glows";	/* singular verb */
	    if (rings == LEFT_RING) {
		which = "left ";
	    } else if (rings == RIGHT_RING) {
		which = "right ";
	    } else {		/* both */
		which = "";
		what = (const char *) makeplural(what);
		how = "glow";	/* plural verb */
	    }
	    Your("%s%s %s %s!", which, what, how, hcolor(warnings[warnlevel]));
	} else {
	    if (Hallucination)
		Your("spider-sense is tingling...");
	    else
		You_feel("apprehensive as you sense a %s flash.",
		    warnings[warnlevel]);
	}

	lastwarntime = moves;
	lastwarnlev = warnlevel;
    }
}
#endif /* 0 */

/* check mtmp and water/lava for compatibility, 0 (survived), 1 (died) */
int
minliquid(mtmp)
register struct monst *mtmp;
{
	/* mtmp must be alive */
	if (!mtmp || DEADMONSTER(mtmp))
		return 1;	/* mon is already dead */
	boolean inpool, inlava, infountain, inshallow;

	inpool = is_3dwater(mtmp->mx, mtmp->my) || (is_pool(mtmp->mx, mtmp->my, FALSE) &&
		(!mon_resistance(mtmp, WWALKING) && !mon_resistance(mtmp, FLYING) && !mon_resistance(mtmp, LEVITATION)));
	inlava = is_lava(mtmp->mx,mtmp->my) &&
		(!mon_resistance(mtmp, WWALKING) && !mon_resistance(mtmp, FLYING) && !mon_resistance(mtmp, LEVITATION));
	infountain = IS_FOUNTAIN(levl[mtmp->mx][mtmp->my].typ);
	inshallow = IS_PUDDLE(levl[mtmp->mx][mtmp->my].typ) &&
		(!mon_resistance(mtmp, WWALKING) && !mon_resistance(mtmp, FLYING) && !mon_resistance(mtmp, LEVITATION));

#ifdef STEED
	/* Flying and levitation keeps our steed out of the liquid */
	/* (but not water-walking or swimming) */
	if (mtmp == u.usteed && (Flying || Levitation) && !is_3dwater(mtmp->mx,mtmp->my))
		return (0);
#endif

	/* Frost Treads freeze water and lava */
	struct obj * otmp;
	if ((otmp = which_armor(mtmp, W_ARMF)) &&
		otmp->oartifact == ART_FROST_TREADS &&
		!is_3dwater(mtmp->mx, mtmp->my) && !Is_waterlevel(&u.uz) &&
		(is_pool(mtmp->mx, mtmp->my, TRUE) || is_lava(mtmp->mx, mtmp->my)))
	{
		zap_over_floor(mtmp->mx, mtmp->my, AD_COLD, WAND_CLASS, FALSE, NULL);
		inpool = FALSE;
		inlava = FALSE;
		inshallow = FALSE;
	}

    /* Gremlin multiplying won't go on forever since the hit points
     * keep going down, and when it gets to 1 hit point the clone
     * function will fail.
     */
	if (mtmp->mtyp == PM_GREMLIN && (inpool || infountain || inshallow) && rn2(3)) {
		if (split_mon(mtmp, (struct monst *)0))
			dryup(mtmp->mx, mtmp->my, FALSE);
		if (inpool) water_damage(mtmp->minvent, FALSE, FALSE, level.flags.lethe, mtmp);
		return (0);	/* gremlins in water */
	}
	else if (is_iron(mtmp) && ((inpool && !rn2(5)) || inshallow)) {
		/* rusting requires oxygen and water, so it's faster for shallow water */
		int dam = d(2, 6);
		if (cansee(mtmp->mx, mtmp->my))
			pline("%s rusts.", Monnam(mtmp));
		mtmp->mhp -= dam;
		if (mtmp->mhpmax > dam) mtmp->mhpmax -= dam;
		if (mtmp->mhp < 1) {
			if (canseemon(mtmp)) pline("%s falls to pieces!", Monnam(mtmp));
			mondead(mtmp);
			if (mtmp->mhp < 1) {
				if (mtmp->mtame && !canseemon(mtmp))
					pline("May %s rust in peace.", mon_nam(mtmp));
				return (1);
			}
		}
		if (inshallow) water_damage(which_armor(mtmp, W_ARMF), FALSE, FALSE, level.flags.lethe, mtmp);
		else water_damage(mtmp->minvent, FALSE, FALSE, level.flags.lethe, mtmp);
		return (0);	/* iron/chain golems in water */
	}

    if (inlava) {
		/*
		 * Lava effects much as water effects. Lava likers are able to
		 * protect their stuff. Fire resistant monsters can only protect
		 * themselves  --ALI
		 */
		if (!is_clinger(mtmp->data) && !likes_lava(mtmp->data)) {
			if (!resists_fire(mtmp)) {
			if (cansee(mtmp->mx,mtmp->my))
				pline("%s %s.", Monnam(mtmp),
				  mtmp->mtyp == PM_WATER_ELEMENTAL ?
				  "boils away" : "burns to a crisp");
			mondead(mtmp);
			}
			else {
			if (--mtmp->mhp < 1) {
				if (cansee(mtmp->mx,mtmp->my))
				pline("%s surrenders to the fire.", Monnam(mtmp));
				mondead(mtmp);
			}
			else if (cansee(mtmp->mx,mtmp->my))
				pline("%s burns slightly.", Monnam(mtmp));
			}
			if (mtmp->mhp > 0) {
			(void) fire_damage_chain(mtmp->minvent, FALSE, FALSE,
							mtmp->mx, mtmp->my);
			(void) rloc(mtmp, TRUE);
			return 0;
			}
			return (1);
		}
    } else if (inpool) {
		/* Most monsters drown in pools.  flooreffects() will take care of
		 * water damage to dead monsters' inventory, but survivors need to
		 * be handled here.  Swimmers are able to protect their stuff...
		 */
		if (!is_clinger(mtmp->data)
			&& !mon_resistance(mtmp,SWIMMING) && !amphibious_mon(mtmp)) {
			if (cansee(mtmp->mx,mtmp->my)) {
				if(mtmp->mtyp == PM_ACID_PARAELEMENTAL){
					int tx = mtmp->mx, ty = mtmp->my, dn = mtmp->m_lev;
					pline("%s explodes.", Monnam(mtmp));
					mondead(mtmp);
					explode_pa(tx, ty, AD_EACD, MON_EXPLODE, d(dn, 10), EXPL_NOXIOUS, 1, mtmp->data);
				} else pline("%s drowns.", Monnam(mtmp));
			}
			if (u.ustuck && u.uswallow && u.ustuck == mtmp) {
			/* This can happen after a purple worm plucks you off a
			flying steed while you are over water. */
			pline("%s sinks as water rushes in and flushes you out.",
				Monnam(mtmp));
			}
			if(!DEADMONSTER(mtmp))
				mondead(mtmp);
			if (mtmp->mhp > 0) {
			(void) rloc(mtmp, TRUE);
			water_damage(mtmp->minvent, FALSE, FALSE, level.flags.lethe, mtmp);
			return 0;
			}
			return (1);
		}
    } else {
		/* but eels have a difficult time outside */
		if ((mtmp->data->mflagsm&MM_AQUATIC) && !Is_waterlevel(&u.uz)) {
			/* Puddles can sustain a tiny sea creature, or lessen the burdens of a larger one */
			if (!(inshallow && mtmp->data->msize == MZ_TINY))
			{
				if (mtmp->mhp > 1 && rn2(1 + mtmp->data->msize)) mtmp->mhp--;
				monflee(mtmp, 2, FALSE, FALSE);
			}
		}
    }
    return (0);
}


int
mcalcmove(mon)
struct monst *mon;
{
    int mmove = mon->data->mmove;
	
	if(u.ustuck == mon && mmove < 12 && mon->data->mlet == S_VORTEX){
		mmove *= 2;
	}

	if(isdark(mon->mx, mon->my) && mon->mtyp == PM_GRUE){
		mmove *= 2;
	}
	
	if(is_pool(mon->mx, mon->my, FALSE) && mon->mtyp == PM_DEEP_DWELLER){
		mmove = mmove*12/7;
	}
	
	if(mon->mcan && mon->mtyp == PM_ETHEREAL_FILCHER){
		mmove = mmove/2;
	}
	
	if(mon->mtyp == PM_PYTHON && !no_upos(mon) && 
		dist2(mon->mx, mon->my, mon->mux, mon->muy) <= 8)
		mmove *= 4;
	
	if(uwep && uwep->oartifact == ART_SINGING_SWORD && !mindless_mon(mon) && !is_deaf(mon)){
		if(uwep->osinging == OSING_HASTE && mon->mtame)
			mmove += 2;
		else if(uwep->osinging == OSING_LETHARGY && !mon->mtame)
			mmove -= 2;
	}
	
	if(mon->mtyp == PM_CHOKHMAH_SEPHIRAH)
		mmove += u.chokhmah;
	if(mon->mtyp == PM_BANDERSNATCH && mon->mflee)
		mmove += 12;
	if(mon->mtyp == PM_UVUUDAUM && mon->mpeaceful)
		mmove /= 4;
	if(mon->mtyp == PM_CHAOS && !PURIFIED_FIRE)
		mmove += 3;
	if(mon->mtyp == PM_OONA)//+0 to +7 (+8 would only occur at 0 hp)
		mmove += 8*(mon->mhpmax-mon->mhp)/mon->mhpmax;
    /* Note: MSLOW's `+ 1' prevents slowed speed 1 getting reduced to 0;
     *	     MFAST's `+ 2' prevents hasted speed 1 from becoming a no-op;
     *	     both adjustments have negligible effect on higher speeds.
     */
	//Intrinsic-ish fast speed, stacks with monster extrinsic speed
	//	Probably a bit better than player speed.
	if (mon_resistance(mon, FAST))
		mmove += (mmove+2)/3;
    if (mon->mspeed == MSLOW)
	mmove = (2 * mmove + 1) / 3;
    else if (mon->mspeed == MFAST)
	mmove = (4 * mmove + 2) / 3;

#ifdef STEED
    if (mon == u.usteed) {
	if (u.ugallop && flags.mv) {
	    /* average movement is 1.50 times normal */
	    mmove = ((rn2(2) ? 4 : 5) * mmove) / 3;
	}
    }
#endif
	if(is_alabaster_mummy(mon->data) && mon->mvar_syllable == SYLLABLE_OF_GRACE__UUR)
		mmove += 6;
	
	if(u.sealsActive&SEAL_CHUPOCLOPS && distmin(mon->mx, mon->my, u.ux, u.uy) <= u.ulevel/5+1){
		mmove = max(mmove-(u.ulevel/10+1),1);
	}
	if(In_fog_cloud(mon)) mmove = max(mmove/3, 1);
	mon->mfell = 0;
	return mmove;
}

/* actions that happen once per ``turn'', regardless of each
   individual monster's metabolism; some of these might need to
   be reclassified to occur more in proportion with movement rate */
void
mcalcdistress()
{
    struct monst *mtmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	if (DEADMONSTER(mtmp)) continue;

	/* must check non-moving monsters once/turn in case
	 * they managed to end up in liquid */
	if (mtmp->data->mmove == 0) {
	    if (vision_full_recalc) vision_recalc(0);
	    if (minliquid(mtmp)) continue;
	}

	if((mtmp->mtyp == PM_HEZROU || (mtmp->mrotting && mtmp->m_lev < rnd(100))) && !(mtmp->mtrapped && t_at(mtmp->mx, mtmp->my) && t_at(mtmp->mx, mtmp->my)->ttyp == VIVI_TRAP)){
		create_gas_cloud(mtmp->mx+rn2(3)-1, mtmp->my+rn2(3)-1, rnd(3), rnd(3)+1, FALSE);
	}
	
	if(mtmp->mtyp == PM_FIRST_WRAITHWORM && !mtmp->mcan && distmin(u.ux, u.uy, mtmp->mx, mtmp->my) < 6){
		struct monst *tmpm;
		You("are thrown by a blast of wind!");
		hurtle(rn2(3)-1, rn2(3)-1, rnd(6), FALSE, TRUE);
		for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
			if(tmpm != mtmp && !DEADMONSTER(tmpm) && distmin(tmpm->mx, tmpm->my, mtmp->mx, mtmp->my) < 6){
				mhurtle(tmpm, rn2(3)-1, rn2(3)-1, rnd(6), FALSE);
			}
		}
	}
	
	/* do special once-per-global turn effects */
	if(!noactions(mtmp)){
		do_ancient_breaths(mtmp);
		do_auras(mtmp);
	}
	
	/* regenerate hit points */
	mon_regen(mtmp, FALSE);
	clothes_bite_mon(mtmp);
	if(mtmp->mscorpions){
		phantom_scorpions_sting(mtmp);
	}
	if(mtmp->mcaterpillars){
		rot_caterpillars_bite(mtmp);
	}
	if(mtmp->mvermin){
		int damage = d(10,10);
		damage -= avg_mdr(mtmp);
		damage = reduce_dmg(mtmp, damage, TRUE, FALSE);
		if(damage > 0)
			m_losehp(mtmp, damage, FALSE, "swarming vermin");
	}

	timeout_problems(mtmp);
	
	/* FIXME: mtmp->mlstmv ought to be updated here */
    }
}

void
timeout_problems(mtmp)
struct monst *mtmp;
{
	if(mtmp->mflee && (bold(mtmp->data) || (mtmp->mtame && uring_art(ART_NARYA)))){
		if(mtmp->mfleetim > 4) mtmp->mfleetim /= 4;
		else {
			mtmp->mfleetim = 0;
			mtmp->mflee = 0;
		}
	}
	
	
	/* possibly polymorph shapechangers and lycanthropes */
	if (mtmp->cham && !rn2(6))
	    (void) newcham(mtmp, NON_PM, FALSE, FALSE);
	were_change(mtmp);

	if((quest_status.time_doing_quest >= UH_QUEST_TIME_2 || (quest_status.time_doing_quest >= UH_QUEST_TIME_1 && !Is_qhome(&u.uz)))
		&& In_quest(&u.uz)
		&& Role_if(PM_UNDEAD_HUNTER)
		&& !(mvitals[PM_MOON_S_CHOSEN].died)
		&& (mtmp->mfaction == QUEST_FACTION || mtmp->mfaction == CITY_FACTION || quest_status.leader_m_id == mtmp->m_id)
	){
		if((Is_qlocate(&u.uz) && (quest_status.time_doing_quest >= UH_QUEST_TIME_2 || (!rn2((UH_QUEST_TIME_2-quest_status.time_doing_quest)/200) && couldsee(mtmp->mx, mtmp->my))))
			|| (!Is_qlocate(&u.uz) && (quest_status.time_doing_quest >= UH_QUEST_TIME_4 || (!rn2((UH_QUEST_TIME_4-quest_status.time_doing_quest)/200) && couldsee(mtmp->mx, mtmp->my))))
		){
			set_faction(mtmp, MOON_FACTION);
			if(mtmp->mtyp == PM_VICAR_AMALIA){
				if(quest_status.time_doing_quest >= UH_QUEST_TIME_3){
					(void) newcham(mtmp, PM_VICAR_WOLF, FALSE, TRUE);
					quest_status.got_quest = TRUE;
					quest_status.leader_is_dead = TRUE;
					mtmp->mstrategy &= ~STRAT_WAITMASK;
					quest_status.leader_m_id = 0;
					if(!mtmp->mtame){
						set_faction(mtmp, MOON_FACTION);
						mtmp->mpeaceful = 0;
						set_malign(mtmp);
					}
				}
			}
			else {
				(void) newcham(mtmp, PM_WEREWOLF, FALSE, TRUE);
				if(!mtmp->mtame){
					set_faction(mtmp, MOON_FACTION);
					mtmp->mpeaceful = 0;
					set_malign(mtmp);
				}
			}
		}
	}

	if(mtmp->mtyp == PM_IKSH_NA_DEVA && !mtmp->mcan && mtmp->mhp*2 < mtmp->mhpmax && !rn2(4) && !hates_holy_mon(mtmp))
		emit_healing(mtmp);
		

	if(!mtmp->mcansee && (mtmp->mtyp == PM_SHOGGOTH || mtmp->mtyp == PM_PRIEST_OF_GHAUNADAUR)){
		if(canspotmon(mtmp)) pline("%s forms new eyes!",Monnam(mtmp));
		mtmp->mblinded = 1;
	}
	if(!mtmp->mcansee && (mtmp->mtyp == PM_BLASPHEMOUS_LURKER || mtmp->mtyp == PM_LURKING_ONE)){
		if(canspotmon(mtmp)) pline("%s opens more eyes!",Monnam(mtmp));
		mtmp->mblinded = 1;
	}
	
	//No point in checking it before setting it.
	mtmp->mgoatmarked = FALSE;
	mtmp->mflamemarked = FALSE;
	mtmp->mibitemarked = FALSE;
	mtmp->myoumarked = FALSE;
	mtmp->mironmarked = FALSE;
	
	/* gradually time out temporary problems */
	if (mtmp->mblinded && !--mtmp->mblinded)
	    mtmp->mcansee = 1;
	if (mtmp->mdeafened && !--mtmp->mdeafened)
	    mtmp->mcanhear = 1;
	if (mtmp->mlaughing && !--mtmp->mlaughing)
	    mtmp->mnotlaugh = 1;
	if (mtmp->mfrozen && !--mtmp->mfrozen)
	    mtmp->mcanmove = 1;
	if (mtmp->mfleetim && !--mtmp->mfleetim)
	    mtmp->mflee = 0;
	if (mtmp->mequipping)
	    mtmp->mequipping--;
}


int
movemon()
{
    register struct monst *mtmp, *nmtmp;
    register boolean somebody_can_move = FALSE;
	int maxtheta=0, mintheta=0, indextheta, deltax, deltay, theta, arc;
	boolean thetafirst=TRUE;

#if 0
    /* part of the original warning code which was replaced in 3.3.1 */
    warnlevel = 0;
#endif

    /*
    Some of you may remember the former assertion here that
    because of deaths and other actions, a simple one-pass
    algorithm wasn't possible for movemon.  Deaths are no longer
    removed to the separate list fdmon; they are simply left in
    the chain with hit points <= 0, to be cleaned up at the end
    of the pass.

    The only other actions which cause monsters to be removed from
    the chain are level migrations and losedogs().  I believe losedogs()
    is a cleanup routine not associated with monster movements, and
    monsters can only affect level migrations on themselves, not others
    (hence the fetching of nmon before moving the monster).  Currently,
    monsters can jump into traps, read cursed scrolls of teleportation,
    and drink cursed potions of raise level to change levels.  These are
    all reflexive at this point.  Should one monster be able to level
    teleport another, this scheme would have problems.
	
	Uh oh, weeping 	angels have a levelport attack. Doesn't seem to 
	cause problems, though....
    */
	
	/////////////////////////////////////////////////////
	//Weeping angel handling////////////////////////////
	///////////////////////////////////////////////////
	/*
	 * 1) Angels generated randomly prior to you becoming a demigod are generated as individuals. 
	 *		Angels generated afterwards are generated as large groups.
	 *		Check if any angels on current level were generated before you became a demigod, and create a large group if so.
	 * 2) Angels move faster if there are more of them in your line of sight, and especially if they are widely separated.
	 *		Calculate the minimum vision arc that covers all angels in view
	 * 3) Once the current movement loop begins, scale the angel's movement by the value calculated in 2. Also, scale
	 *		the angel's AC based on the value from 2, slower speed equals higher AC (Quantum Lock).
	 */
	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon){
		//Weeping angel step 1
		if(is_weeping(mtmp->data)){
			if(mtmp->mvar3 && u.uevent.invoked){
				mtmp->mvar3 = 0; //Quantum Lock status will be reset below.
				m_initgrp(mtmp, 0, 0, 10);
			}
		} else if(mtmp->mtyp == PM_RAZORVINE && 
				  mtmp->mhp == mtmp->mhpmax && 
				  !mtmp->mcan && 
				  !((monstermoves + mtmp->m_id) % (30-depth(&u.uz)/2)) && 
				  !rn2(4)
		){
			struct monst *sprout = (struct monst *) 0;
			int newx = (mtmp->mx-1)+rn2(3), newy = (mtmp->my-1)+rn2(3);
			if(isok(newx,newy)){
				sprout = makemon(mtmp->data, newx, newy, MM_NOCOUNTBIRTH|NO_MINVENT);
				if(sprout) sprout->mhp = In_hell(&u.uz) ? sprout->mhp*3/4 : sprout->mhp/2;
			}
		}
	}
	
	//Weeping angel step 2
	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if(is_weeping(mtmp->data) && canseemon(mtmp)){
			deltax = u.ux - mtmp->mx;
			deltay = u.uy - mtmp->my;
			theta = (int)(atanGerald((float)deltay/(float)deltax)*100);
			if(deltax<0&&deltay<0) theta+=314;
			else if(deltax<0) theta+=314;
			else if(deltay<=0) theta+=628;
			theta -= 314;
			if(thetafirst){
				indextheta = theta;
				thetafirst = FALSE;
			}else{
				theta = theta - indextheta;
				if(theta <= -314) theta+=628;
				else if(theta > 314) theta -= 628;
				if(theta<mintheta) mintheta = theta;
				else if(theta>maxtheta) maxtheta = theta;
			}
		}
	}
	arc = maxtheta - mintheta;
	
	if(u.specialSealsActive&SEAL_LIVING_CRYSTAL)
		average_dogs();
	//Current Movement Loop///////////////////////////////////////////////////
    for(mtmp = fmon; mtmp; mtmp = nmtmp) {
	if (flags.run_timers){
		run_timers();
	}
	/* check that mtmp wasn't migrated by previous mtmp's actions */
	if (!(mtmp->mx || mtmp->my)) {
		/* uh oh -- it looks like mtmp is marked as on migrating_mons. Confirm. */
		struct monst * m2;
		for (m2 = migrating_mons; m2 && m2 != mtmp; m2 = m2->nmon);
		if (m2 == mtmp) {
			/* extra check -- is mtmp also on the fmon chain? If it is, we've got serious trouble */
			for (m2 = fmon; m2 && m2 != mtmp; m2 = m2->nmon);
			if (m2 == mtmp)
				panic("monster on both fmon and migrating_mons");
			else {
				/* restart loop at fmon. This will let already-handled very fast
				 * monsters expend movement points to act out of turn, but will not result in anyone gaining
				 * or losing turns, or worse, this loop affecting anyone in the migrating_mons chain */
				mtmp = fmon;
				if (!mtmp)
					break;	/* exit current movement loop, there is no one left at all */
			}
		}
	}
	nmtmp = mtmp->nmon;
	/* Find a monster that we have not treated yet.	 */
	if(DEADMONSTER(mtmp))
	    continue;
	if(mtmp->mlast_movement != monstermoves){
		mtmp->mprev_dir.x = 0;
		mtmp->mprev_dir.y = 0;
	}
	if(mtmp->mlstmv != monstermoves){
		mtmp->mprev_attk.x = 0;
		mtmp->mprev_attk.y = 0;
	}
	if(u.specialSealsActive&SEAL_LIVING_CRYSTAL)
		average_dogs();
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
    if(In_quest(&u.uz) && urole.neminum == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH && levl[mtmp->mx][mtmp->my].typ == AIR
		&& !mon_resistance(mtmp,FLYING)
		&& !mon_resistance(mtmp,LEVITATION)
		&& mtmp != u.ustuck
		&& mtmp != u.usteed
	){
		struct d_level target_level;
		target_level.dnum = u.uz.dnum;
		target_level.dlevel = qlocate_level.dlevel+1;
		mtmp->mhp = 1; //How Lucky! Almost Died!
		if(canseemon(mtmp))
			pline("%s plummets to the rocky cavern floor below!", Monnam(mtmp));
	    migrate_to_level(mtmp, ledger_no(&target_level), MIGR_RANDOM, (coord *)0);
		continue;
	}
	if(TimeStop && !is_uvuudaum(mtmp))
		continue;
	if(mtmp->movement < NORMAL_SPEED)
	    continue;
	
	mtmp->movement -= NORMAL_SPEED;
	if (mtmp->movement >= NORMAL_SPEED)
	    somebody_can_move = TRUE;
	
	if(mtmp->mstdy > 0) mtmp->mstdy -= 1; //monster is moving, reduce studied level
	else if(mtmp->mstdy < 0) mtmp->mstdy += 1; //monster is moving, reduce protection level

	if(mtmp->mopen) mtmp->mopen--;
	//Weeping angel step 3
	if(is_weeping(mtmp->data)){
		mtmp->mvar2 &= 0x1L; //clear higher order bits, first bit is whether it should generate a swarm when you return
		if(canseemon(mtmp)){
			mtmp->mvar1 +=  (long)(NORMAL_SPEED*arc/314);
			if(!arc){
				mtmp->mvar2 |= 0x4L; //Quantum Locked
			}
			else if(arc < 314/2){
				mtmp->mvar2 |= 0x2L; //Partial Quantum Lock
			}
			m_respond(mtmp);
			if(mtmp->mvar1 >= NORMAL_SPEED*2) mtmp->mvar1 -= NORMAL_SPEED*2;
			else continue;
		}
		//else no quant lock
	}
	
	if(Nightmare && mon_can_see_you(mtmp) && !mindless_mon(mtmp) && !mtmp->mpeaceful && !is_render(mtmp->mtyp)){
		you_inflict_madness(mtmp);
	}
	
	if(mtmp->mtyp == PM_METROID_QUEEN){
		if(mtmp->mtame){
			struct monst *baby;
			int tnum = d(1,6);
			int i;
			untame(mtmp, 1);
			//Make sure it won't turn hostile on a timer (possible for Bards at least)
			mtmp->mpeacetime = 0;
			for(i = 0; i < 6; i++){
				baby = makemon(&mons[PM_METROID], mtmp->mx, mtmp->my, MM_ADJACENTOK);
				if(tnum-- > 0) tamedog(baby,(struct obj *) 0);
				else baby->mpeaceful = 1;
			}
		}
		if((levl[mtmp->mx][mtmp->my].typ == ROOM || levl[mtmp->mx][mtmp->my].typ == CORR)
			&& !rn2(100)
		){
			struct obj *egg;
			egg = mksobj(EGG, MKOBJ_NOINIT);
			egg->spe = 0; //not yours
			egg->quan = 1;
			egg->owt = weight(egg);
			egg->corpsenm = egg_type_from_parent(PM_METROID_QUEEN, FALSE);
			attach_egg_hatch_timeout(egg);
			if(canseemon(mtmp)){
				egg->known = egg->dknown = 1;
				pline("%s lays an egg.",Monnam(mtmp));
			}
			if (!flooreffects(egg, mtmp->mx, mtmp->my, "")) {
				place_object(egg, mtmp->mx, mtmp->my);
				stackobj(egg);
				newsym(mtmp->mx, mtmp->my);  /* map the rock */
			}
			stackobj(egg);
		}
	}

	if(mtmp->mtyp == PM_GRAY_DEVOURER && mtmp->mtame && !rn2(13) && base_casting_stat() == A_CHA){
		if(canspotmon(mtmp))
			pline("%s shakes off your control!", Monnam(mtmp));
		untame(mtmp, FALSE);
	}
	
	if(mtmp->mtyp == PM_CROW_WINGED_HALF_DRAGON && mtmp->mtame && !EDOG(mtmp)->loyal && Race_if(PM_HALF_DRAGON) && Role_if(PM_NOBLEMAN) && flags.initgend && u.uevent.qcompleted){
		EDOG(mtmp)->loyal = 1;
		EDOG(mtmp)->waspeaceful = TRUE;
		mtmp->mpeacetime = 0;
	}
	
	if(u.uevent.uaxus_foe && 
		is_auton(mtmp->data) && 
		mtmp->mpeaceful
	){
		if(canseemon(mtmp)) pline("%s gets angry...", Monnam(mtmp));
		untame(mtmp, 0);
	}
	if(mtmp->mtyp == PM_UVUUDAUM){
		if(u.uevent.invoked 
		|| Infuture
		|| mtmp->mhp < mtmp->mhpmax
		|| mtmp->m_lev < 30
		|| mtmp->mspec_used > 0
		){ 
			if(mtmp->mpeaceful){
				pline("%s ceases its meditation...", Amonnam(mtmp));
				mtmp->mpeaceful = 0;
				set_malign(mtmp);
			}
		} else if(!mtmp->mpeaceful && mtmp->mhp == mtmp->mhpmax && mtmp->m_lev >= 30 && mtmp->mspec_used == 0){
			pline("%s resumes its meditation...", Amonnam(mtmp));
			mtmp->mpeaceful = 1;
			set_malign(mtmp);
		}
	}
	if(mtmp->data->msound == MS_GLYPHS
		|| quest_faction(mtmp)
		|| (mtmp->mtyp == PM_LADY_CONSTANCE && Role_if(PM_MADMAN))
	){
		if(!mtmp->mpeaceful && mtmp->mhp >= mtmp->mhpmax && mtmp->m_lev >= mtmp->data->mlevel-1){
			pline("%s calms down...", Amonnam(mtmp));
			mtmp->mpeaceful = 1;
			set_malign(mtmp);
		}
	}
	if(mtmp->mtyp == PM_TWIN_SIBLING && flags.made_twin){
		if(!mtmp->mpeaceful && mtmp->mhp >= mtmp->mhpmax){
			pline("%s calms down...", Amonnam(mtmp));
			mtmp->mpeaceful = 1;
			set_malign(mtmp);
		}
		else if(mtmp->mpeaceful && !mtmp->mtame){
			if(canspotmon(mtmp))
				pline("%s looks friendly...", Amonnam(mtmp));
			mtmp = tamedog_core(mtmp, (struct obj *)0, TRUE);
			if(mtmp && get_mx(mtmp, MX_EDOG)){
				EDOG(mtmp)->loyal = TRUE;
			}
		}
	}
	if(mtmp->mtyp == PM_DREAD_SERAPH && 
		mtmp->mhp == mtmp->mhpmax && 
		!(mtmp->mstrategy&STRAT_WAITMASK) && 
		!(u.uevent.invoked || Infuture)
			
	){
		//go back to sleep
		mtmp->mstrategy |= STRAT_WAITMASK;
	}
	
	if (vision_full_recalc) vision_recalc(0);	/* vision! */

	if (is_hider(mtmp->data)) {
	    /* unwatched mimics and piercers may hide again  [MRS] */
	    if(restrap(mtmp))   continue;
	    if(mtmp->m_ap_type == M_AP_FURNITURE ||
				mtmp->m_ap_type == M_AP_OBJECT)
		    continue;
	    if(mtmp->mundetected){
			if(mtmp->mtyp == PM_INCARNATOR_MAGGOT){
				if(!rn2(6)){
					incarnator_spawn(mtmp->mx, mtmp->my, FALSE);
				}
			}
			continue;
		}
	}

	if (minliquid(mtmp)) continue;

	if(u.specialSealsActive&SEAL_LIVING_CRYSTAL)
		average_dogs();

	/* continue if the monster died fighting */
	if (!mtmp->iswiz && !is_blind(mtmp)) {
	    /* Note:
	     *  Conflict does not take effect in the first round.
	     *  Therefore, A monster when stepping into the area will
	     *  get to swing at you.
	     *
	     *  The call to fightm() must be _last_.  The monster might
	     *  have died if it returns 1.
	     */
	    if (fightm(mtmp)) continue;	/* mon might have died */
	}
	if(dochugw(mtmp))		/* otherwise just move the monster */
	    continue;
    }
#if 0
    /* part of the original warning code which was replaced in 3.3.1 */
    if(warnlevel > 0)
	warn_effects();
#endif

    if (any_light_source())
	vision_full_recalc = 1;	/* in case a mon moved with a light source */
    dmonsfree();	/* remove all dead monsters */

    /* a monster may have levteleported player -dlc */
    if (u.utotype) {
	deferred_goto();
	/* changed levels, so these monsters are dormant */
	somebody_can_move = FALSE;
    }

    return somebody_can_move;
}

#endif /* OVL1 */
#ifdef OVLB

#define mstoning(obj)	(ofood(obj) && \
					(((obj)->corpsenm >= LOW_PM && touch_petrifies(&mons[(obj)->corpsenm])) || \
					(obj)->corpsenm == PM_MEDUSA))

/*
 * Maybe eat a metallic object (not just gold).
 * Return value: 0 => nothing happened, 1 => monster ate something,
 * 2 => monster died (it must have grown into a genocided form, but
 * that can't happen at present because nothing which eats objects
 * has young and old forms).
 */
int
meatmetal(mtmp)
	register struct monst *mtmp;
{
	register struct obj *otmp;
	struct permonst *ptr;
	int poly, grow, heal, mstone;

	/* If a pet, eating is handled separately, in dog.c */
	if (mtmp->mtame) return 0;

	/* Eats topmost metal object if it is there */
	for (otmp = level.objects[mtmp->mx][mtmp->my];
						otmp; otmp = otmp->nexthere) {
	    if (mtmp->mtyp == PM_RUST_MONSTER && !is_rustprone(otmp))
		continue;
	    if (is_metallic(otmp) && !obj_resists(otmp, 0, 100) &&
			touch_artifact(otmp, mtmp, FALSE) && !(otmp->otyp == MAGIC_CHEST && otmp->obolted)
			) {
		if (mtmp->mtyp == PM_RUST_MONSTER && otmp->oerodeproof) {
		    if (canseemon(mtmp) && flags.verbose) {
			pline("%s eats %s!",
				Monnam(mtmp),
				distant_name(otmp,doname));
		    }
		    /* The object's rustproofing is gone now */
		    otmp->oerodeproof = 0;
		    mtmp->mstun = 1;
		    if (canseemon(mtmp) && flags.verbose) {
			pline("%s spits %s out in disgust!",
			      Monnam(mtmp), distant_name(otmp,doname));
		    }
		/* KMH -- Don't eat indigestible/choking objects */
		} else if (otmp->otyp != AMULET_OF_STRANGULATION &&
				otmp->otyp != RIN_SLOW_DIGESTION) {
		    if (cansee(mtmp->mx,mtmp->my) && flags.verbose)
			pline("%s eats %s!", Monnam(mtmp),
				distant_name(otmp,doname));
		    else if (flags.soundok && flags.verbose)
			You_hear("a crunching sound.");
		    mtmp->meating = otmp->owt/2 + 1;
		    /* Heal up to the object's weight in hp */
		    if (mtmp->mhp < mtmp->mhpmax) {
			mtmp->mhp += objects[otmp->otyp].oc_weight;
			if (mtmp->mhp > mtmp->mhpmax) mtmp->mhp = mtmp->mhpmax;
		    }

			struct obj *obj2;
			while((obj2 = otmp->cobj)){
				obj_extract_self(obj2);
				/* Compartmentalize tip() */
				place_object(obj2, mtmp->mx, mtmp->my);
				stackobj(obj2);
			}

		    if(otmp == uball) {
			unpunish();
			delobj(otmp);
		    } else if (otmp == uchain) {
			unpunish();	/* frees uchain */
		    } else {
			poly = polyfodder(otmp) && !resists_poly(mtmp->data);
			grow = mlevelgain(otmp);
			heal = mhealup(otmp);
			mstone = mstoning(otmp);
			delobj(otmp);
			ptr = mtmp->data;
			if (poly) {
			    if (newcham(mtmp, NON_PM,
					FALSE, FALSE))
				ptr = mtmp->data;
			} else if (grow) {
			    ptr = grow_up(mtmp, (struct monst *)0);
			} else if (mstone) {
			    if (poly_when_stoned(ptr)) {
				mon_to_stone(mtmp);
				ptr = mtmp->data;
			    } else if (!resists_ston(mtmp)) {
				if (canseemon(mtmp))
				    pline("%s turns to stone!", Monnam(mtmp));
				monstone(mtmp);
				ptr = (struct permonst *)0;
			    }
			} else if (heal) {
			    mtmp->mhp = mtmp->mhpmax;
			}
			if (!ptr) return 2;		 /* it died */
		    }
		    /* Left behind a pile? */
		    if (rnd(25) < 3)
			(void)mksobj_at(ROCK, mtmp->mx, mtmp->my, NO_MKOBJ_FLAGS);
		    newsym(mtmp->mx, mtmp->my);
		    return 1;
		}
	    }
	}
	return 0;
}

/*
 * Maybe eat a food item.
 * Return value: 0 => nothing happened, 1 => monster ate something,
 * 2 => monster died (from eating poison, petrifying corpses, etc.).
 */
int
meatgluttony(mtmp)
	register struct monst *mtmp;
{
	register struct obj *otmp, *otmp2;
	struct permonst *ptr;
	int poly, grow, heal, count = 0, ecount = 0;
	char buf[BUFSZ];

	/* If a pet, eating is handled separately, in dog.c */
	if (mtmp->mtame)
		return 0;
	else if (inediate(mtmp->data))
		return 0;

	ptr = mtmp->data;
	for (otmp = level.objects[mtmp->mx][mtmp->my]; otmp; otmp = otmp2) {
	    otmp2 = otmp->nexthere;
		if(!is_edible_mon(mtmp, otmp))
			continue;
		if(!mtmp->mgluttony && mtmp->mcannibal && 
			(otmp->otyp != CORPSE || !((mons[otmp->corpsenm].mflagsa&mtmp->data->mflagsa) || otmp->corpsenm == mtmp->mtyp))
		)
			continue;
	    if (!obj_resists(otmp, 0, 100) &&
		    touch_artifact(otmp, mtmp, FALSE)
		){
			if(mtmp->mtame && get_mx(mtmp, MX_EDOG)){
				return dog_eat(mtmp, otmp, mtmp->mx, mtmp->my, FALSE);
			}
			else {
				return monster_eat(mtmp, otmp, mtmp->mx, mtmp->my, FALSE);
			}
		}
	}
	return 0;
}

/* returns 2 if monster dies, otherwise 1 */
int
monster_eat(mtmp, obj, x, y, devour)
register struct monst *mtmp;
register struct obj * obj;
int x, y;
boolean devour;
{
	boolean poly = FALSE, grow = FALSE, heal = FALSE, ston = FALSE,
		tainted = FALSE, acidic = FALSE, freeze = FALSE, burning = FALSE, poison = FALSE;
	boolean vampiric = is_vampire(mtmp->data);
	boolean eatonlyone = (obj->oclass == FOOD_CLASS || obj->oclass == CHAIN_CLASS || obj->oclass == POTION_CLASS);

	boolean can_choke = !Breathless_res(mtmp);

	int nutrit = dog_nutrition(mtmp, obj);
	long rotted = 0;
	int mtyp = NON_PM;
	poly = polyfodder(obj) && !resists_poly(mtmp->data);
	grow = mlevelgain(obj);
	heal = mhealup(obj);
	ston = (obj->otyp == CORPSE || obj->otyp == EGG || obj->otyp == TIN || obj->otyp == POT_BLOOD) && obj->corpsenm >= LOW_PM && touch_petrifies(&mons[obj->corpsenm]) && !Stone_res(mtmp);
	
	if(obj->otyp == CORPSE){
		mtyp = obj->corpsenm;
		if (mtyp != PM_LIZARD && mtyp != PM_SMALL_CAVE_LIZARD && mtyp != PM_CAVE_LIZARD 
			&& mtyp != PM_LARGE_CAVE_LIZARD && mtyp != PM_LICHEN && mtyp != PM_BEHOLDER
		) {
			long age = peek_at_iced_corpse_age(obj);

			rotted = (monstermoves - age)/(10L + rn2(20));
			if (obj->cursed) rotted += 2L;
			else if (obj->blessed) rotted -= 2L;
		}

		if(mtyp != PM_ACID_BLOB && !ston && rotted > 5L)
			tainted = TRUE;
		else if(acidic(&mons[mtyp]) && !Acid_res(mtmp))
			acidic = TRUE;
		if(freezing(&mons[mtyp]) && !Cold_res(mtmp))
			freeze = TRUE;
		if(burning(&mons[mtyp]) && !Fire_res(mtmp))
			burning = TRUE;
		if(poisonous(&mons[mtyp]) && !Poison_res(mtmp))
			poison = TRUE;
	}

	if (devour) {
	    if (mtmp->meating > 1) mtmp->meating /= 2;
	    if (nutrit > 1) nutrit = (nutrit * 3) / 4;
	}
	/* vampires only get 1/5 normal nutrition */
	if (vampiric) {
	    mtmp->meating = (mtmp->meating + 4) / 5;
	    nutrit = (nutrit + 4) / 5;
	}

	if(u.sealsActive&SEAL_MALPHAS && mtmp->mtyp == PM_CROW && obj->otyp == CORPSE){
		more_experienced(ptrexperience(&mons[obj->corpsenm]),0);
		newexplevel();
	}
	mtmp->mconf = 0;

	if (mtmp->mflee && mtmp->mfleetim > 1) mtmp->mfleetim /= 2;

	if (x != mtmp->mx || y != mtmp->my) {	/* moved & ate on same turn */
	    newsym(x, y);
	    newsym(mtmp->mx, mtmp->my);
	}
	if (is_pool(x, y, FALSE) && !Underwater) {
	    /* Don't print obj */
	    /* TODO: Reveal presence of sea monster (especially sharks) */
	} else
	/* hack: observe the action if either new or old location is in view */
	/* However, invisible monsters should still be "it" even though out of
	   sight locations should not. */
	if (cansee(x, y) || cansee(mtmp->mx, mtmp->my))
	    pline("%s %s %s.", mon_visible(mtmp) ? noit_Monnam(mtmp) : "It",
		  obj->oclass == POTION_CLASS ? "drinks" : vampiric ? "drains" : devour ? "devours" : "eats",
		  eatonlyone ? singular(obj, doname) : doname(obj));

	if (mtmp->mtyp == PM_RUST_MONSTER && obj->oerodeproof) {
	    /* The object's rustproofing is gone now */
	    obj->oerodeproof = 0;
	    mtmp->mstun = 1;
	    if (canseemon(mtmp) && flags.verbose) {
		pline("%s spits %s out in disgust!",
		      Monnam(mtmp), distant_name(obj,doname));
	    }
		can_choke = FALSE;
		nutrit = 0;
	} else if (vampiric && !(obj->otyp == POT_BLOOD)) {
		/* Split Object */
		if (obj->quan > 1L) {
		    if(!carried(obj)) {
			(void) splitobj(obj, 1L);
		    } else {
		    	/* Carried */
			obj = splitobj(obj, obj->quan - 1L);
			
			freeinv(obj);
			if (inv_cnt() >= 52 && !merge_choice(invent, obj))
			    dropy(obj);
			else
			    obj = addinv(obj); /* unlikely but a merge is possible */			
		    }
#ifdef DEBUG
		    debugpline("split object,");
#endif
		}
		
		/* Take away blood nutrition */
	    	obj->oeaten = drainlevel(obj);
		obj->odrained = 1;
		can_choke = FALSE;
	} else {
		/*These cases destroy the object, rescue its contents*/
		struct obj *obj2;
		while((obj2 = obj->cobj)){
			obj_extract_self(obj2);
			/* Compartmentalize tip() */
			place_object(obj2, mtmp->mx, mtmp->my);
			stackobj(obj2);
		}

		if (obj == uball) {
			unpunish();
			delobj(obj);
		} else if (obj == uchain)
			unpunish();
		else if (obj->quan > 1L && eatonlyone) {
			obj->quan--;
			obj->owt = weight(obj);
		} else
			delobj(obj);
	}

	if (can_choke && rn2(nutrit) > 5*mtmp->data->cnutrit) /*Magic number: dog_nutrition multiplier for medium-sized monsters*/
	{
	    if (canseemon(mtmp))
	    {
	        pline("%s chokes over %s food!", Monnam(mtmp), mhis(mtmp));
			pline("%s dies!", Monnam(mtmp));
	    }
		mondied(mtmp);
	    if (mtmp->mhp <= 0)
			return 2;
	}

	if (ston) {
		xstoney((struct monst *)0, mtmp);
	    if (mtmp->mhp <= 0)
			return 2;
	}
	if(tainted){
		int dmg = d(3, 12);
		if(!rn2(10))
			dmg += 100;
		if(m_losehp(mtmp, dmg, FALSE, "tainted corpse"))
			return 2;
	}
	if(acidic){
		// if(canspotmon(mtmp))
			// pline()
		if(m_losehp(mtmp, rnd(15), FALSE, "acidic corpse"))
			return 2;
	}
	if(freeze){
		if(m_losehp(mtmp, d(2, 12), FALSE, "frozen corpse"))
			return 2;
	}
	if(burning){
		if(m_losehp(mtmp, rnd(20), FALSE, "burning corpse"))
			return 2;
	}
	if(poison){
		int dmg = d(1, 8);
		if(!rn2(10))
			dmg += 80;
		if(m_losehp(mtmp, dmg, FALSE, "poisonous corpse"))
			return 2;
	}
	if (poly) {
	    (void) newcham(mtmp, NON_PM, FALSE,
			   cansee(mtmp->mx, mtmp->my));
	}
	
	/* limit "instant" growth to prevent potential abuse */
	if (grow && (int) mtmp->m_lev < (int)mtmp->data->mlevel + 15) {
	    if (!grow_up(mtmp, (struct monst *)0)) return 2;
	}
	if (heal) mtmp->mhp = mtmp->mhpmax;
	if(mtyp != NON_PM){
		give_mon_corpse_intrinsic(mtmp, mtyp);
	}
	return 1;
}

int
meatobj(mtmp)		/* for gelatinous cubes */
	register struct monst *mtmp;
{
	register struct obj *otmp, *otmp2;
	struct permonst *ptr;
	int poly, grow, heal, count = 0, ecount = 0;
	char buf[BUFSZ];

	buf[0] = '\0';
	/* If a pet, eating is handled separately, in dog.c */
	if (mtmp->mtame) return 0;

	/* Eats organic objects, including cloth and wood, if there */
	/* Engulfs others, except huge rocks and metal attached to player */
	for (otmp = level.objects[mtmp->mx][mtmp->my]; otmp; otmp = otmp2) {
	    otmp2 = otmp->nexthere;
	    if (is_organic(otmp) && !obj_resists(otmp, 0, 100) &&
		    touch_artifact(otmp, mtmp, FALSE)) {
		if (otmp->otyp == CORPSE && touch_petrifies(&mons[otmp->corpsenm]) &&
			!resists_ston(mtmp))
		    continue;
		if (otmp->otyp == AMULET_OF_STRANGULATION ||
				otmp->otyp == RIN_SLOW_DIGESTION)
		    continue;
		++count;
		if (cansee(mtmp->mx,mtmp->my) && flags.verbose)
		    pline("%s eats %s!", Monnam(mtmp),
			    distant_name(otmp, doname));
		else if (flags.soundok && flags.verbose)
		    You_hear("a slurping sound.");
		/* Heal up to the object's weight in hp */
		if (mtmp->mhp < mtmp->mhpmax) {
		    mtmp->mhp += objects[otmp->otyp].oc_weight;
		    if (mtmp->mhp > mtmp->mhpmax) mtmp->mhp = mtmp->mhpmax;
		}
		if (Has_contents(otmp)) {
		    register struct obj *otmp3;
		    /* contents of eaten containers become engulfed; this
		       is arbitrary, but otherwise g.cubes are too powerful */
		    while ((otmp3 = otmp->cobj) != 0) {
			obj_extract_self(otmp3);
			if (otmp->otyp == ICE_BOX && otmp3->otyp == CORPSE) {
			    otmp3->age = monstermoves - otmp3->age;
			    start_corpse_timeout(otmp3);
			}
			(void) mpickobj(mtmp, otmp3);
		    }
		}
		poly = polyfodder(otmp) && !resists_poly(mtmp->data);
		grow = mlevelgain(otmp);
		heal = mhealup(otmp);
		delobj(otmp);		/* munch */
		ptr = mtmp->data;
		if (poly) {
		    if (newcham(mtmp, NON_PM, FALSE, FALSE))
			ptr = mtmp->data;
		} else if (grow) {
		    ptr = grow_up(mtmp, (struct monst *)0);
		} else if (heal) {
		    mtmp->mhp = mtmp->mhpmax;
		}
		/* it died during growth */
		if(!ptr)
		    return 2;
		/* in case it polymorphed or died */
		if (ptr->mtyp != PM_GELATINOUS_CUBE && ptr->mtyp != PM_ANCIENT_OF_CORRUPTION)
		    return 1;
	    } else if (otmp->oclass != ROCK_CLASS && !(otmp->otyp == MAGIC_CHEST && otmp->obolted) &&
				    otmp != uball && otmp != uchain) {
		++ecount;
		if (ecount == 1) {
			Sprintf(buf, "%s engulfs %s.", Monnam(mtmp),
			    distant_name(otmp,doname));
		} else if (ecount == 2)
			Sprintf(buf, "%s engulfs several objects.", Monnam(mtmp));
		obj_extract_self(otmp);
		(void) mpickobj(mtmp, otmp);	/* slurp */
	    }
	    /* Engulf & devour is instant, so don't set meating */
	    if (mtmp->minvis) newsym(mtmp->mx, mtmp->my);
	}
	if (ecount > 0) {
	    if (cansee(mtmp->mx, mtmp->my) && flags.verbose && buf[0])
		pline("%s", buf);
	    else if (flags.soundok && flags.verbose)
	    	You_hear("%s slurping sound%s.",
			ecount == 1 ? "a" : "several",
			ecount == 1 ? "" : "s");
	}
	return ((count > 0) || (ecount > 0)) ? 1 : 0;
}

void
mvanishobj(mtmp, x, y)		/* for nachash tanninim */
	register struct monst *mtmp;
	int x, y;
{
	register struct obj *otmp, *otmp2;
	struct permonst *ptr;
	int poly, grow, heal, ecount = 0;
	char buf[BUFSZ];

	buf[0] = '\0';

	/* Engulfs everything */
	for (otmp = level.objects[x][y]; otmp; otmp = otmp2) {
	    otmp2 = otmp->nexthere;
		if (!(otmp->otyp == MAGIC_CHEST && otmp->obolted)){
			++ecount;
			if (ecount == 1) {
				Sprintf(buf, "%s into %s shadow.", Tobjnam(otmp,"fall"), s_suffix(mon_nam(mtmp)));
			} else if (ecount == 2){
				Sprintf(buf, "Some objects fall into %s shadow.", s_suffix(mon_nam(mtmp)));
			}
			if(otmp == uball){
				unpunish();
			}
			if(otmp == uchain){
				unpunish();	/* frees uchain */
				continue; /* move to next */
			}
			obj_extract_self(otmp);
			(void) mpickobj(mtmp, otmp);	/* slurp */
	    }
	    newsym(x, y);
	}
	if (ecount > 0) {
	    if (cansee(mtmp->mx, mtmp->my) && flags.verbose && buf[0])
			pline("%s", buf);
	}
}

void
mpickgold(mtmp)
	register struct monst *mtmp;
{
    register struct obj *gold;
    int mat_idx;

	/* prevent dead monsters from picking up anything */
	if (DEADMONSTER(mtmp))
		return;

    if ((gold = g_at(mtmp->mx, mtmp->my)) != 0) {
	mat_idx = gold->obj_material;
#ifndef GOLDOBJ
	mtmp->mgold += gold->quan;
	delobj(gold);
#else
        obj_extract_self(gold);
        add_to_minv(mtmp, gold);
#endif
	if (cansee(mtmp->mx, mtmp->my) ) {
	    if (flags.verbose && !mtmp->isgd)
		pline("%s picks up some %s.", Monnam(mtmp),
			mat_idx == GOLD ? "gold" : "money");
	    newsym(mtmp->mx, mtmp->my);
	}
    }
}
#endif /* OVLB */
#ifdef OVL2

boolean
mpickstuff(mtmp, str)
	register struct monst *mtmp;
	register const char *str;
{
	register struct obj *otmp, *otmp2;

/*	prevent shopkeepers from leaving the door of their shop */
	if(mtmp->isshk && inhishop(mtmp)) return FALSE;

	/* prevent dead monsters from picking up anything */
	if (DEADMONSTER(mtmp))
		return FALSE;

	for(otmp = level.objects[mtmp->mx][mtmp->my]; otmp; otmp = otmp2) {
	    otmp2 = otmp->nexthere;
/*	Nymphs take everything.  Most monsters don't pick up corpses. */
	    if (!str ? searches_for_item(mtmp,otmp) :
		  !!(index(str, otmp->oclass))) {
		if (otmp->otyp == CORPSE && mtmp->data->mlet != S_NYMPH &&
			/* let a handful of corpse types thru to can_carry() */
			!touch_petrifies(&mons[otmp->corpsenm]) &&
			otmp->corpsenm != PM_LIZARD &&
			!acidic(&mons[otmp->corpsenm])) continue;
		if (!touch_artifact(otmp, mtmp, FALSE)) continue;
		if (!can_carry(mtmp,otmp)) continue;
		if (is_pool(mtmp->mx,mtmp->my, FALSE)) continue;
#ifdef INVISIBLE_OBJECTS
		if (otmp->oinvis && !mon_resistance(mtmp,SEE_INVIS)) continue;
#endif
		if (cansee(mtmp->mx,mtmp->my) && flags.verbose)
			pline("%s picks up %s.", Monnam(mtmp),
			      (distu(mtmp->mx, mtmp->my) <= 5) ?
				doname(otmp) : distant_name(otmp, doname));
		obj_extract_self(otmp);
		/* unblock point after extract, before pickup */
		if (is_boulder(otmp))
		    unblock_point(otmp->ox,otmp->oy);	/* vision */
		if(otmp) (void) mpickobj(mtmp, otmp);	/* may merge and free otmp */
		m_dowear(mtmp, FALSE);
		newsym(mtmp->mx, mtmp->my);
		return TRUE;			/* pick only one object */
	    }
	}
	return FALSE;
}

#endif /* OVL2 */
#ifdef OVL0

int
curr_mon_load(mtmp)
register struct monst *mtmp;
{
	register int curload = 0;
	register struct obj *obj;

	for(obj = mtmp->minvent; obj; obj = obj->nobj) {
		if(!is_boulder(obj) || !throws_rocks(mtmp->data))
			curload += obj->owt;
	}

	return curload;
}

int
max_mon_load(mtmp)
register struct monst *mtmp;
{
	long maxload = MAX_CARR_CAP;
	struct permonst *mdat = mtmp->data;
	struct obj *gloves;
	struct obj *cloak;
	struct obj *bodyarmor;
	struct obj *underarmor;	
	struct obj *boots;	
	struct obj *belt;	
	// long carcap = 25L*(acurrstr((int)(mtmp->mstr)) + mtmp->mcon) + 50L;
	long carcap;
	gloves = which_armor(mtmp, W_ARMG);
	cloak = which_armor(mtmp, W_ARMC);
	bodyarmor = which_armor(mtmp, W_ARM);
	underarmor = which_armor(mtmp, W_ARMU);
	boots = which_armor(mtmp, W_ARMF);
	belt = which_armor(mtmp, W_BELT);
	
	if(gloves && (gloves->otyp == GAUNTLETS_OF_POWER || (gloves->otyp == IMPERIAL_ELVEN_GAUNTLETS && check_imp_mod(gloves, IEA_GOPOWER)))){
		carcap = 25L*(25L + 11L) + 50L;
	} else if(strongmonst(mdat)){
		carcap = 25L*(18L + 11L) + 50L;
	} else {
		carcap = 25L*(11L + 11L) + 50L;
	}


	if (!mdat->cwt){
		maxload = (maxload * (long)mdat->msize) / MZ_HUMAN;
		carcap = (carcap * (long)mdat->msize) / MZ_HUMAN;
	} else if (!strongmonst(mdat)
		|| (strongmonst(mdat) && (mdat->cwt > WT_HUMAN))
	){
		maxload = (maxload * (long)mdat->cwt) / WT_HUMAN;
		carcap = (carcap * (long)mdat->cwt) / WT_HUMAN;
	}

	if (!mdat->cwt)
		carcap = (carcap * (long)mdat->msize) / MZ_HUMAN;
	else if (!strongmonst(mdat)
		|| (strongmonst(mdat) && (mdat->cwt > WT_HUMAN)))
		carcap = (carcap * (long)mdat->cwt / WT_HUMAN);
	
	if (carcap > maxload) carcap = maxload;

	if (belt && belt->otyp == BELT_OF_CARRYING){
		if(belt->blessed)
			carcap += carcap/4;
		else if(belt->cursed)
			carcap -= carcap/4;
		else
			carcap += carcap/8;
	}

	static int hboots = 0;
	if (!hboots) hboots = find_hboots();
	if (boots && boots->otyp == hboots) carcap += boots->cursed ? 0 : maxload/10;
	if (boots && check_oprop(boots, OPROP_RBRD) && is_lawful_mon(mtmp)) 
		carcap += boots->cursed ? 0 : max(200, maxload/5);
	

	if(animaloid(mdat) || naoid(mdat)){
		carcap *= 1.5;
	}
	if(centauroid(mdat) || mdat->mtyp == PM_BLESSED){
		carcap *= 1.25;
	}
	if(is_half_dragon(mdat) && mtmp->mvar_hdBreath == AD_FIRE && mdat->mlevel >= 15){
		carcap *= 1.2;
	}
	if(arti_lighten(bodyarmor, FALSE)){
		if(bodyarmor->blessed) carcap *= 1.5;
		else if(!bodyarmor->cursed) carcap *= 1.25;
		else carcap *= .75;
	}
	if(arti_lighten(cloak, FALSE)){
		if(cloak->blessed) carcap *= 1.5;
		else if(!cloak->cursed) carcap *= 1.25;
		else carcap *= .75;
	}
#ifdef TOURIST
	if(arti_lighten(underarmor, FALSE)){
		if(underarmor->blessed) carcap *= 1.5;
		else if(!underarmor->cursed) carcap *= 1.25;
		else carcap *= .75;
	}
#endif	/* TOURIST */

	if (carcap < 1) carcap = 1;

	return (int) carcap;
}

boolean
mon_can_see_mon(looker, lookie)
	struct monst *looker;
	struct monst *lookie;
{
	boolean clearpath;
	boolean hardtosee;
	boolean indark = (dimness(looker->mx, looker->my) > 0);
	
	if(lookie->mtyp == PM_TWIN_SIBLING && !insightful(looker->data) && !is_great_old_one(looker->data))
		return FALSE;
	
	if(looker->mtyp == PM_DREADBLOSSOM_SWARM){
		if(lookie->mtyp == PM_DREADBLOSSOM_SWARM) return FALSE;
		else return mon_can_see_mon(lookie, looker);
	}
	if(looker->mtyp == PM_SWARM_OF_SNAKING_TENTACLES || looker->mtyp == PM_LONG_SINUOUS_TENTACLE){
		struct monst *witw;
		for(witw = fmon; witw; witw = witw->nmon) if(witw->mtyp == PM_WATCHER_IN_THE_WATER) break;
		if(witw){
			looker->mux = witw->mux;
			looker->muy = witw->muy;
			if(mon_can_see_mon(witw, lookie)) return TRUE;
		}
		//may still be able to feel target adjacent
	}
	
	if(looker->mtyp == PM_WIDE_CLUBBED_TENTACLE){
		struct monst *keto;
		for(keto = fmon; keto; keto = keto->nmon) if(keto->mtyp == PM_KETO) break;
		if(keto){
			looker->mux = keto->mux;
			looker->muy = keto->muy;
			if(mon_can_see_mon(keto, lookie)) return TRUE;
		}
		//may still be able to feel target adjacent
	}
	
	if(looker->mtyp == PM_DANCING_BLADE && looker->mvar_suryaID){
		struct monst *surya;
		for(surya = fmon; surya; surya = surya->nmon) if(surya->m_id == looker->mvar_suryaID) break;
		if(surya){
			looker->mux = surya->mux;
			looker->muy = surya->muy;
			if(mon_can_see_mon(surya, lookie)) return TRUE;
		}
		return FALSE;
		//can't feel target adjacent
	}

	/* Monsters that have recently made noise can be targeted by other monsters */
	if(lookie->mnoise && !is_deaf(looker) && distmin(looker->mx,looker->my,lookie->mx,lookie->my) <= BOLT_LIM)
		return TRUE;

	/* 1/8 chance to stumble onto adjacent targets. Ish. */
	if(distmin(looker->mx,looker->my,lookie->mx,lookie->my) <= 1 && !rn2(8))
		return TRUE;
	
	/* Monsters with sensitive ears can find enemies without stealth */
	if(sensitive_ears(looker->data) && looker->mcanhear && !mon_resistance(lookie,STEALTH) && distmin(looker->mx,looker->my,lookie->mx,lookie->my) <= rn2(8))
		return TRUE;
	
	/* R'lyehian psychic sight, see minds, blocked by water */
	if(rlyehiansight(looker->data) && !mindless_mon(lookie)
		&& (!is_pool(looker->mx, looker->my, FALSE) || mon_resistance(looker,FLYING) || mon_resistance(looker,LEVITATION))
		&& (!is_pool(lookie->mx, lookie->my, FALSE) || !is_underswimmer(lookie->data) || mon_resistance(lookie,FLYING) || mon_resistance(lookie,LEVITATION))
	)
		return TRUE;
	
	clearpath = clear_path(looker->mx, looker->my, lookie->mx, lookie->my);
	hardtosee = (!is_tracker(looker->data) && (
		(lookie->minvis && !mon_resistance(looker, SEE_INVIS)) ||
		(lookie->mundetected) ||
		(level.objects[lookie->mx][lookie->my] && level.objects[lookie->mx][lookie->my]->otyp == EXPENSIVE_BED)
		));

	/* sight -- requires looker to not be blind, have clear LoS, and notice an invisible lookie somehow */
	if (!is_blind(looker)
		&& clearpath
		&& (!hardtosee || !rn2(11))) {
		/* where can we see? */
		int nvrange = 0;
		boolean darksight = FALSE;
		/* darksight */
		if (darksight(looker->data) || (catsight(looker->data) && indark)) {
			darksight = TRUE;
		}
		/* normal sight */
		if (normalvision(looker->data) || (catsight(looker->data) && !indark)) {
			nvrange = 1;
		}
		/* low-light vision */
		if (lowlightsight2(looker->data)) {
			nvrange = 2;
		}
		/* elfsight */
		if (lowlightsight3(looker->data)) {
			nvrange = 3;
		}
		/* nv range auto-succeeds within its distance */
		if (nvrange > 0
			&& dist2(looker->mx, looker->my, lookie->mx, lookie->my) <= nvrange * nvrange) {
			return TRUE;
		}
		/* otherwise, check sight vs how lit/dim the square is */
		if ((!darksight
			? (dimness(lookie->mx, lookie->my) < nvrange)
			: (dimness(lookie->mx, lookie->my) > -nvrange)
			)
			||
			extramission(looker->data)
			)
		{
			return TRUE;
		}
	}

	if(echolocation(looker->data) && !is_deaf(looker) && !unsolid(lookie->data)){
		if (clearpath) {
			return TRUE;
		}
	}
	if(infravision(looker->data) && infravisible(lookie->data) && !is_blind(looker)){
		if((clearpath || omnisense(looker->data)) && (!hardtosee || !rn2(11))){
			return TRUE;
		}
	}
	if(bloodsense(looker->data) && has_blood(lookie->data)){
		if((clearpath || omnisense(looker->data)) && (!hardtosee || !rn2(11))){
			return TRUE;
		}
	}
	if(lifesense(looker->data) && !nonliving(lookie->data)){
		if((clearpath || omnisense(looker->data)) && (!hardtosee || !rn2(11))){
			return TRUE;
		}
	}
	if(senseall(looker->data)){
		if((clearpath || omnisense(looker->data)) && (!hardtosee || !rn2(11))){
			return TRUE;
		}
	}
	if(earthsense(looker->data) && !(mon_resistance(lookie,FLYING) || mon_resistance(lookie,LEVITATION) || unsolid(lookie->data))){
		return TRUE;
	}
	if(mon_resistance(looker,TELEPAT) && !mindless_mon(lookie)){
		return TRUE;
	}
	if(goodsmeller(looker->data) && distmin(lookie->mx, lookie->my, looker->mx, looker->my) <= 6){
	/*sanity check: don't bother trying to path to it if it is farther than a path can possibly exist*/
		if(clearpath){
		/*don't running a complicated path function if there is a straight line to target*/
			return TRUE;
		} else {
			if(path_exists(looker->mx, looker->my, lookie->mx, lookie->my, 0, 6)){
				return TRUE;
			}
		}
	}
	return FALSE;
}

boolean
mon_can_see_you(looker)
struct monst *looker;
{
	boolean clearpath;
	boolean hardtosee;
	boolean catsightdark = !(levl[looker->mx][looker->my].lit || (viz_array[looker->my][looker->mx]&TEMP_LIT1 && !(viz_array[looker->my][looker->mx]&TEMP_DRK1)));
	
	if(looker->mtyp == PM_DREADBLOSSOM_SWARM){
		if(youracedata->mtyp == PM_DREADBLOSSOM_SWARM) return FALSE;
		else return canseemon(looker);
	}
	
	if(looker->mtyp == PM_SWARM_OF_SNAKING_TENTACLES || looker->mtyp == PM_LONG_SINUOUS_TENTACLE){
		struct monst *witw;
		for(witw = fmon; witw; witw = witw->nmon) if(witw->mtyp == PM_WATCHER_IN_THE_WATER) break;
		if(witw && mon_can_see_you(witw)) return TRUE;
		//may still be able to feel target adjacent
	}
	
	if(looker->mtyp == PM_WIDE_CLUBBED_TENTACLE){
		struct monst *keto;
		for(keto = fmon; keto; keto = keto->nmon) if(keto->mtyp == PM_KETO) break;
		if(keto && mon_can_see_you(keto)) return TRUE;
		//may still be able to feel target adjacent
	}
	
	if(looker->mtyp == PM_DANCING_BLADE && looker->mvar_suryaID){
		struct monst *surya;
		for(surya = fmon; surya; surya = surya->nmon) if(surya->m_id == looker->mvar_suryaID) break;
		if(surya && mon_can_see_you(surya)) return TRUE;
		return FALSE;
		//can't feel target adjacent
	}
	
	if(Aggravate_monster) return TRUE;
	
	if(Withering_stake
		&& (quest_status.time_doing_quest >= UH_QUEST_TIME_2 || mvitals[PM_MOON_S_CHOSEN].died)
		&& (looker->data->mflagsa&MA_ANIMAL || looker->data->mflagsa&MA_DEMIHUMAN || looker->data->mflagsa&MA_WERE)
	)
		return TRUE;
	
	/* 1/8 chance to stumble onto adjacent targets. Ish. */
	if(distmin(looker->mx,looker->my,u.ux,u.uy) <= 1 && !rn2(8))
		return TRUE;
	
	/* Monsters with sensitive ears can find enemies without stealth */
	if(sensitive_ears(looker->data) && looker->mcanhear && !Stealth && distmin(looker->mx,looker->my,u.ux, u.uy) <= rn2(8))
		return TRUE;

	if(Invis && (artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_VILQUAR) && !resist(looker, '\0', 0, NOTELL)){
		return FALSE;
	}

	/* Note: the player is never mindless */
	/* R'lyehian psyichic sight, see minds, blocked by water */
	if(rlyehiansight(looker->data)
		&& !(is_pool(looker->mx, looker->my, FALSE) || mon_resistance(looker,FLYING) || mon_resistance(looker,LEVITATION))
		&& !u.usubwater
	)
		return TRUE;
	clearpath = couldsee(looker->mx, looker->my);
	hardtosee = (!can_track(looker->data) && (
		(Invis && !mon_resistance(looker, SEE_INVIS)) ||
		(u.uundetected) ||
		(level.objects[u.ux][u.uy] && level.objects[u.ux][u.uy]->otyp == EXPENSIVE_BED)
		));

	/* sight -- requires looker to not be blind, have clear LoS, and notice an invisible lookie somehow */
	if (!is_blind(looker)
		&& clearpath
		&& (!hardtosee || !rn2(11))) {
		/* where can we see? */
		int nvrange = 0;
		boolean darksight = FALSE;
		/* darksight */
		if (darksight(looker->data) || (catsight(looker->data) && catsightdark)) {
			darksight = TRUE;
		}
		/* normal sight */
		if (normalvision(looker->data) || (catsight(looker->data) && !catsightdark)) {
			nvrange = 1;
		}
		/* low-light vision */
		if (lowlightsight2(looker->data)) {
			nvrange = 2;
		}
		/* elfsight */
		if (lowlightsight3(looker->data)) {
			nvrange = 3;
		}
		/* nv range auto-succeeds within its distance */
		if (nvrange > 0
			&& dist2(looker->mx, looker->my, u.ux, u.uy) <= nvrange * nvrange + nvrange
			&& !(Stealth && (Role_if(PM_ROGUE) || (u.sealsActive&SEAL_ANDROMALIUS) || !rn2(8)))
		) {
			return TRUE;
		}
		/* otherwise, check sight vs how lit/dim the square is */
		if ((!darksight
			? (dimness(u.ux, u.uy) < nvrange)
			: (dimness(u.ux, u.uy) > -nvrange)
			)
			||
			extramission(looker->data)
			)
		{
			return TRUE;
		}
	}
	if(echolocation(looker->data) && !is_deaf(looker) && !unsolid(youracedata)){
		if (clearpath) {
			return TRUE;
		}
	}
	if(infravision(looker->data) && infravisible(youracedata) && !is_blind(looker)){
		if((clearpath || omnisense(looker->data)) && (!hardtosee || !rn2(11))){
			return TRUE;
		}
	}
	if(bloodsense(looker->data) && has_blood(youracedata)){
		if((clearpath || omnisense(looker->data)) && (!hardtosee || !rn2(11))){
			return TRUE;
		}
	}
	if(lifesense(looker->data) && !nonliving(youracedata)){
		if((clearpath || omnisense(looker->data)) && (!hardtosee || !rn2(11))){
			return TRUE;
		}
	}
	if(senseall(looker->data)){
		if((clearpath || omnisense(looker->data)) && (!hardtosee || !rn2(11))){
			return TRUE;
		}
	}
	if(earthsense(looker->data) && !(Flying || Levitation|| unsolid(youracedata) || Stealth)){
		return TRUE;
	}
	if (mon_resistance(looker, TELEPAT) && !Tele_blind) {/* player always has a mind */
		return TRUE;
	}

	//As a foulness shall ye know Them.
	if(goodsmeller(looker->data) && u.specialSealsActive&SEAL_YOG_SOTHOTH)
		return TRUE;

	if(goodsmeller(looker->data) && distmin(u.ux, u.uy, looker->mx, looker->my) <= 6){
	/*sanity check: don't bother trying to path to it if it is farther than a path can possibly exist*/
		if(clearpath){
		/*don't running a complicated path function if there is a straight line to you*/
			return TRUE;
		} else {
			if(path_exists(looker->mx, looker->my, u.ux, u.uy, 0, 6)){
				return TRUE;
			}
		}
	}
	return FALSE;
}

STATIC_DCL int
scent_callback(data, x, y)
genericptr_t data;
int x, y;
{
    int is_accessible = ZAP_POS(levl[x][y].typ);
    boolean *shouldsmell = (boolean *)data;
    if (scentgoalx == x && scentgoaly == y) *shouldsmell = TRUE;
	
	if(!(*shouldsmell)) return !is_accessible;
	else return 1; /* Once a path to you is found, quickly end the xpath function */
}

/* for restricting monsters' object-pickup */
boolean
can_carry(mtmp,otmp)
struct monst *mtmp;
struct obj *otmp;
{
	int otyp = otmp->otyp, newload = otmp->owt;
	struct permonst *mdat = mtmp->data;

	if (notake(mdat)) return FALSE;		/* won't carry anything */

	if (otyp == CORPSE && touch_petrifies(&mons[otmp->corpsenm]) &&
		!(mtmp->misc_worn_check & W_ARMG) && !resists_ston(mtmp))
	    return FALSE;
	if (otyp == CORPSE && is_rider(&mons[otmp->corpsenm]))
	    return FALSE;
	if (otyp == MAGIC_CHEST && otmp->obolted)
		return FALSE;
	if (otmp->obj_material == SILVER && hates_silver(mdat) &&
		(otyp != BELL_OF_OPENING || !is_covetous(mdat)))
	    return FALSE;
	if (is_iron_obj(otmp) && hates_iron(mdat))
	    return FALSE;

#ifdef STEED
	/* Steeds don't pick up stuff (to avoid shop abuse) */
	if (mtmp == u.usteed) return (FALSE);
#endif
	if (mtmp->isshk) return(TRUE); /* no limit */
	if ((mtmp->mpeaceful && mtmp->mtyp != PM_MAID && !(Infuture && mtmp->mfaction == QUEST_FACTION)) && !mtmp->mtame) return(FALSE);
	/* otherwise players might find themselves obligated to violate
	 * their alignment if the monster takes something they need
	 * 
	 * Maids, on the other hand, will vaccum up everything on the floor. 
	 * But if the player can't tame or assasinate one midlevel monster they
	 * deserve what's coming ;-)
	 */

	/* special--boulder throwers carry unlimited amounts of boulders */
	if (throws_rocks(mdat) && is_boulder(otmp))
		return(TRUE);

	/* nymphs deal in stolen merchandise, but not boulders or statues */
	if (mdat->mlet == S_NYMPH)
		return (boolean)(otmp->oclass != ROCK_CLASS);
	/* ditto filchers */
	if (mdat->mtyp == PM_ETHEREAL_FILCHER)
		return (boolean)(otmp->oclass != ROCK_CLASS);

	if (curr_mon_load(mtmp) + newload > max_mon_load(mtmp)) return FALSE;

	return(TRUE);
}

boolean
wall_adjacent(x,y)
int x;
int y;
{
	for(int i = x-1; i < x+2; i++){
		for(int j = y-1; j < y+2; j++){
			if(isok(i,j) && (
				IS_ROCK(levl[i][j].typ)
				|| (levl[i][j].typ == IRONBARS)
				|| (levl[i][j].typ == DRAWBRIDGE_UP)
				|| (levl[i][j].typ == DOOR)
			)) return TRUE;
		}
	}
	return FALSE;
}

boolean
space_adjacent(x,y)
int x;
int y;
{
	for(int i = x-1; i < x+2; i++){
		for(int j = y-1; j < y+2; j++){
			if(isok(i,j) && (ZAP_POS(levl[i][j].typ))) 
				return TRUE;
		}
	}
	return FALSE;
}

/* return number of acceptable neighbour positions */
int
mfndpos(mon, poss, info, flag)
	register struct monst *mon;
	coord *poss;	/* coord poss[9] */
	long *info;	/* long info[9] */
	long flag;
{
	struct permonst *mdat = mon->data;
	struct monst *witw = 0;
	register xchar x,y,nx,ny;
	register int cnt = 0;
	register uchar ntyp;
	uchar nowtyp;
	boolean wantpool, puddleispool, wantdry, poolok, cubewaterok, lavaok, nodiag, quantumlock;
	boolean rockok = FALSE, treeok = FALSE, thrudoor;
	int maxx, maxy;
	
	if(mdat->mtyp == PM_SWARM_OF_SNAKING_TENTACLES || mdat->mtyp == PM_LONG_SINUOUS_TENTACLE){
		for(witw = fmon; witw; witw = witw->nmon) if(witw->mtyp == PM_WATCHER_IN_THE_WATER) break;
	}
	
	if(mdat->mtyp == PM_WIDE_CLUBBED_TENTACLE){
		for(witw = fmon; witw; witw = witw->nmon) if(witw->mtyp == PM_KETO) break;
	}

	x = mon->mx;
	y = mon->my;
	nowtyp = levl[x][y].typ;

	nodiag = (mdat->mtyp == PM_GRID_BUG) || (mdat->mtyp == PM_BEBELITH);
	wantpool = !!(mdat->mflagsm&MM_AQUATIC);
	wantdry = !(mdat->mflagsm&MM_AQUATIC);
	puddleispool = (wantpool && mdat->msize == MZ_TINY) || (wantdry && is_iron(mon));

	/* nexttry can reset some of the above booleans, but recalculates the ones below. */
	/* eels prefer the water, but if there is no water nearby, they will crawl over land */
nexttry:	

	cubewaterok = (mon_resistance(mon, SWIMMING) || (amphibious_mon(mon) && !wantdry && !wantpool));
	poolok = mon_resistance(mon, WWALKING) || mon_resistance(mon, FLYING) || is_clinger(mdat)
		|| (mon_resistance(mon, SWIMMING) && !wantpool)
		|| (amphibious_mon(mon) && !wantdry && !wantpool);
	lavaok = mon_resistance(mon,FLYING) || is_clinger(mdat) || likes_lava(mdat)
		|| (mon_resistance(mon, WWALKING) && resists_fire(mon));
	quantumlock = (is_weeping(mdat) && !u.uevent.invoked);
	thrudoor = ((flag & (ALLOW_WALL|BUSTDOOR)) != 0L);
	if (flag & ALLOW_DIG) {
	    struct obj *mw_tmp;

	    /* need to be specific about what can currently be dug */
	    if (!needspick(mdat)) {
		rockok = treeok = TRUE;
	    } else if ((mw_tmp = MON_WEP(mon)) && (mw_tmp->cursed && !is_weldproof_mon(mon)) &&
		       mon->weapon_check == NO_WEAPON_WANTED) {
		rockok = is_pick(mw_tmp);
		treeok = is_axe(mw_tmp);
	    } else {
			rockok = (select_pick(mon) != (struct obj*)0);
			treeok = (select_axe(mon) != (struct obj*)0);
	    }
	    thrudoor |= rockok || treeok;
	}
	if(mon->mconf) {
		flag |= ALLOW_ALL;
		flag &= ~NOTONL;
	}
	if(is_blind(mon))
		flag |= ALLOW_SSM;
	maxx = min(x+1,COLNO-1);
	maxy = min(y+1,ROWNO-1);
	for(nx = max(1,x-1); nx <= maxx; nx++)
	  for(ny = max(0,y-1); ny <= maxy; ny++) {
	    //if(nx == x && ny == y) continue;
	    if(IS_ROCK(ntyp = levl[nx][ny].typ) &&
	       !((flag & ALLOW_WALL) && may_passwall(nx,ny)) &&
	       !((IS_TREE(ntyp) ? treeok : rockok) && may_dig(nx,ny))) continue;
	    /* KMH -- Added iron bars */
	    if (ntyp == IRONBARS && !(flag & ALLOW_BARS)) continue;
	    /* ALI -- Artifact doors (no passage unless open/openable) from Slash'em*/
	    if (IS_DOOR(ntyp))
			if (artifact_door(nx, ny) ?
				(levl[nx][ny].doormask & D_CLOSED && !(flag & OPENDOOR))
				  || levl[nx][ny].doormask & D_LOCKED :
				!amorphous(mdat) &&
			   ((levl[nx][ny].doormask & D_CLOSED && !(flag & OPENDOOR)) ||
			(levl[nx][ny].doormask & D_LOCKED && !(flag & UNLOCKDOOR))) &&
			   !thrudoor) continue;
	    if(nx != x && ny != y && (nodiag || mdat->mtyp == PM_LONG_WORM ||
#ifdef REINCARNATION
	       ((IS_DOOR(nowtyp) &&
		 ((levl[x][y].doormask & ~D_BROKEN) || Is_rogue_level(&u.uz))) ||
		(IS_DOOR(ntyp) &&
		 ((levl[nx][ny].doormask & ~D_BROKEN) || Is_rogue_level(&u.uz))))
#else
	       ((IS_DOOR(nowtyp) && (levl[x][y].doormask & ~D_BROKEN)) ||
		(IS_DOOR(ntyp) && (levl[nx][ny].doormask & ~D_BROKEN)))
#endif
	       ))
			continue;
		if(is_vectored_mtyp(mdat->mtyp)){
			if(x + xdir[(int)mon->mvar_vector] != nx || 
			   y + ydir[(int)mon->mvar_vector] != ny 
			) continue;
		}
		if(mon->mfrigophobia && ntyp == ICE)
			continue;
		if(In_quest(&u.uz) && urole.neminum == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH && ntyp == AIR
			&& !mon_resistance(mon,FLYING)
			&& !mon_resistance(mon,LEVITATION)
		)
			continue;
		if((mdat->mtyp == PM_GRUE) && isdark(mon->mx, mon->my) && !isdark(nx, ny))
				continue;
		if((mdat->mtyp == PM_WATCHER_IN_THE_WATER || mdat->mtyp == PM_KETO || mdat->mtyp == PM_TETTIGON_LEGATUS) && 
			!no_upos(mon) && 
			distmin(nx, ny, mon->mux, mon->muy) <= 3 && 
			dist2(nx, ny, mon->mux, mon->muy) <= dist2(mon->mx, mon->my, mon->mux, mon->muy)) continue;
		if((mdat->mtyp == PM_WATCHER_IN_THE_WATER || mdat->mtyp == PM_TETTIGON_LEGATUS) && 
			onlineu(nx, ny) && (lined_up(mon) || !rn2(4))) continue;
		if(witw && dist2(nx, ny, witw->mx, witw->my) > 32 && 
			dist2(nx, ny, witw->mx, witw->my) >= dist2(mon->mx, mon->my, witw->mx, witw->my)) continue;
		if(mdat->mtyp == PM_HOOLOOVOO
			&& (IS_ROCK(levl[mon->mx][mon->my].typ) && space_adjacent(mon->mx,mon->my))
			&& !(IS_ROCK(levl[nx][ny].typ) && space_adjacent(nx,ny))
		) continue;
		if(mdat->mtyp == PM_PARASITIC_WALL_HUGGER && 
			wall_adjacent(mon->mx, mon->my) &&
			!wall_adjacent(nx, ny)
		) continue;
		if(mdat->mtyp == PM_PARASITIC_WALL_HUGGER && 
			wall_adjacent(mon->mx, mon->my) &&
			!wall_adjacent(nx, ny)
		) continue;
		//Weeping angels should avoid stepping into corredors, where they can be forced into a standoff.
		if(quantumlock && IS_ROOM(levl[mon->mx][mon->my].typ) && !IS_ROOM(ntyp) ) continue;
		
		if(((is_pool(nx, ny, puddleispool) == wantpool) || poolok) &&
			(cubewaterok || !is_3dwater(nx,ny)) && 
			((is_pool(nx, ny, puddleispool) == !wantdry) || poolok) &&
			(lavaok || !is_lava(nx,ny))) {
		int dispx, dispy;
		boolean monseeu = (!is_blind(mon) && (!Invis || mon_resistance(mon,SEE_INVIS)));
		boolean checkobj = OBJ_AT(nx,ny);
		
		/* Displacement also displaces the Elbereth/scare monster,
		 * as long as you are visible.
		 */
		if(Displaced && !no_upos(mon) && monseeu && (mon->mux==nx) && (mon->muy==ny)) {
		    dispx = u.ux;
		    dispy = u.uy;
		} else {
		    dispx = nx;
		    dispy = ny;
		}

		info[cnt] = 0;
		if ((checkobj || Displaced) && onscary(dispx, dispy, mon)) {
		    if(!(flag & ALLOW_SSM)) continue;
		    info[cnt] |= ALLOW_SSM;
		}
		if((nx == u.ux && ny == u.uy) ||
		   (nx == mon->mux && ny == mon->muy && !no_upos(mon))) {
			if (nx == u.ux && ny == u.uy) {
				/* If it's right next to you, it found you,
				 * displaced or no.  We must set mux and muy
				 * right now, so when we return we can tell
				 * that the ALLOW_U means to attack _you_ and
				 * not the image.
				 */
				mon->mux = u.ux;
				mon->muy = u.uy;
			}
			if(!(flag & ALLOW_U)) continue;
			info[cnt] |= ALLOW_U;
		} else {
			if(MON_AT(nx, ny)) {
				struct monst *mtmp2 = m_at(nx, ny);
				if (mtmp2 && !(nx == x && ny == y)) {
					long mmflag = flag | mm_aggression(mon, mtmp2);

					if (!(mmflag & ALLOW_M)) continue;
					info[cnt] |= ALLOW_M;
					if (mtmp2->mtame) {
						if (!(mmflag & ALLOW_TM)) continue;
						info[cnt] |= ALLOW_TM;
					}
				}
			}
			/* Note: ALLOW_SANCT only prevents movement, not */
			/* attack, into a temple. */
			if(level.flags.has_temple &&
			   *in_rooms(nx, ny, TEMPLE) &&
			   !*in_rooms(x, y, TEMPLE) &&
			   in_your_sanctuary((struct monst *)0, nx, ny)) {
				if(!(flag & ALLOW_SANCT)) continue;
				info[cnt] |= ALLOW_SANCT;
			}
		}
		if(checkobj && sobj_at(CLOVE_OF_GARLIC, nx, ny)) {
			if(flag & NOGARLIC) continue;
			info[cnt] |= NOGARLIC;
		}
		if(checkobj && boulder_at(nx, ny)) {
			if(!(flag & ALLOW_ROCK)) continue;
			info[cnt] |= ALLOW_ROCK;
		}
		if (monseeu && onlineu(nx,ny)) {
			if(flag & NOTONL) continue;
			info[cnt] |= NOTONL;
		}
		if (levl[nx][ny].typ == CLOUD && Is_lolth_level(&u.uz) && !(nonliving(mdat) || breathless_mon(mon) || resists_poison(mon))) {
			if(!(flag & ALLOW_TRAPS)) continue;
			info[cnt] |= ALLOW_TRAPS;
		}
		if (nx != x && ny != y && bad_rock(mon, x, ny)
			    && bad_rock(mon, nx, y)
			    && ((bigmonst(mdat) && !amorphous(mdat)) || (curr_mon_load(mon) > 600)))
			continue;
		/* The monster avoids a particular type of trap if it's familiar
		 * with the trap type.  Pets get ALLOW_TRAPS and checking is
		 * done in dogmove.c.  In either case, "harmless" traps are
		 * neither avoided nor marked in info[].
		 */
		{ register struct trap *ttmp = t_at(nx, ny);
		    if(ttmp) {
			if(ttmp->ttyp >= TRAPNUM || ttmp->ttyp == 0)  {
				impossible("A monster looked at a very strange trap of type %d.", ttmp->ttyp);
			    continue;
			}
			struct obj *mwep;
			mwep = MON_WEP(mon);
			if ((ttmp->ttyp != RUST_TRAP
					|| mdat->mtyp == PM_IRON_GOLEM
					|| mdat->mtyp == PM_GREEN_STEEL_GOLEM
					|| mdat->mtyp == PM_CHAIN_GOLEM)
				&& ((ttmp->ttyp != PIT
				    && ttmp->ttyp != SPIKED_PIT
				    && ttmp->ttyp != TRAPDOOR
				    && ttmp->ttyp != HOLE)
				      || (!mon_resistance(mon,FLYING)
				    && !mon_resistance(mon,LEVITATION)
				    && !is_clinger(mdat))
				      || In_sokoban(&u.uz))
				&& (ttmp->ttyp != SLP_GAS_TRAP ||
				    !resists_sleep(mon))
				&& (ttmp->ttyp != BEAR_TRAP ||
				    (mdat->msize > MZ_SMALL &&
				     !amorphous(mdat) && !mon_resistance(mon,FLYING)))
				&& (ttmp->ttyp != FLESH_HOOK ||
				    !amorphous(mdat))
				&& (ttmp->ttyp != FIRE_TRAP ||
				    !resists_fire(mon) || (!no_upos(mon) && distmin(mon->mx, mon->my, mon->mux, mon->muy) > 2)) /*Cuts down on plane of fire message spam*/
				&& (ttmp->ttyp != SQKY_BOARD || !mon_resistance(mon,FLYING))
				&& (ttmp->ttyp != WEB || (!amorphous(mdat) &&
				    !(webmaker(mdat) || (Is_lolth_level(&u.uz) && !mon->mpeaceful)) && !(
						species_tears_webs(mdat) ||
						(mon->wormno && count_wsegs(mon) > 5)
					) && 
					!(mwep && (mwep->oartifact == ART_STING || 
						mwep->oartifact == ART_LIECLEAVER))))
			) {
			    if (!(flag & ALLOW_TRAPS)) {
				if (mon->mtrapseen & (1L << (ttmp->ttyp - 1)) || mon_resistance(mon, SEARCHING))
				    continue;
			    }
			    info[cnt] |= ALLOW_TRAPS;
			}
		    }
		}
		poss[cnt].x = nx;
		poss[cnt].y = ny;
		cnt++;
	    }
	}
	/* possibly try again with relaxed requirements */
	if (!cnt && (wantpool || wantdry)) {
		wantpool = FALSE;
		wantdry = FALSE;
		goto nexttry;
	}
	/* clear the creatures current square from poss and info */
	if (cnt) {
		int i;
		for (i = 0; i < cnt; i++)
		if (poss[i].x == x && poss[i].y == y)
		{
			int j;
			for (j = i; j < cnt - 1; j++) {
				poss[j].x = poss[j + 1].x;
				poss[j].y = poss[j + 1].y;
				info[j] = info[j + 1];
			}
			poss[j].x = 0;
			poss[j].y = 0;
			info[j] = 0L;
			cnt--;
			break;
		}
	}	return(cnt);
}

#endif /* OVL0 */
#ifdef OVL1

/* Monster against monster special attacks; for the specified monster
   combinations, this allows one monster to attack another adjacent one
   in the absence of Conflict. Incorporates changes from grudge mod */
long
mm_aggression(magr, mdef)
struct monst * magr;	/* monster that is currently deciding where to move */
struct monst * mdef;	/* another monster which is next to it */
{
	long res = mm_grudge(magr, mdef, TRUE);

	// must be able to see mdef -- note that this has a 1/8 chance when adjacent even when totally blind!
	if(res && mon_can_see_mon(magr, mdef)){
		return res;
	}
	return 0L;
}

long
mm_grudge(magr, mdef, actual)
struct monst * magr;	/* monster that is currently deciding where to move */
struct monst * mdef;	/* another monster which is next to it */
boolean actual;			/* actual attack or faction check? */
{
	struct permonst *ma, *md;
	ma = magr->data;
	md = mdef->data;

#define mm_undead(mon) ((is_undead(mon->data) || has_template(mon, CORDYCEPS) || has_template(mon, SPORE_ZOMBIE)) && mon->mfaction != HOLYDEAD_FACTION)

	// Pets don't attack:
	if(magr->mtame && (
		/* peacefuls if you told them not to */
		(u.peaceful_pets && mdef->mpeaceful) ||
		/* other pets */
		(mdef->mtame) ||
		/* creatures you are interacting with */
		(mdef->moccupation) ||
		/* peaceful quest guardians/leaders */
		(mdef->mpeaceful && (mdef->m_id == quest_status.leader_m_id || quest_faction(mdef))) ||
		/* peaceful Axus */
		(mdef->mpeaceful && !u.uevent.uaxus_foe && md->mtyp == PM_AXUS)
	)){
		return 0L;
	}
	if(actual
		&& magr->mtame 
		&& get_mx(magr, MX_EDOG)
		&& ((monstermoves - EDOG(magr)->whistletime < 5)
			|| magr->mpassive
			// || magr->mretreat
			)
	){
		return 0L;
	}
	//3/4 chance to avoid drawing attacks even from zombies etc.
	if(actual
		&& mdef->mtame 
		&& get_mx(mdef, MX_EDOG)
		&& ((monstermoves - EDOG(mdef)->whistletime < 5)
			|| mdef->mpassive
			// || mdef->mretreat
			)
		&& rn2(4)
	){
		return 0L;
	}
	// monsters trapped in vivisection traps are excluded
	// shackled monsters aren't a threat
	if(nonthreat(magr) || nonthreat(mdef)){
		return 0L;
	}
	// must be in range to attack mdef
	if (actual && distmin(magr->mx, magr->my, mdef->mx, mdef->my) > BOLT_LIM) {
		return 0L;
	}
	// magr cannot be waiting
	if (magr->mstrategy & STRAT_WAITMASK) {
		return 0L;
	}
	if(magr->mtyp == PM_ELVEN_WRAITH || magr->mtyp == PM_SILVERFIRE_SHADOW_S_WRAITH){
		if(magr->mvar_elfwraith_target == (long) mdef->m_id)
			return ALLOW_M | ALLOW_TM;
		else
			return 0L;
	}
	// Respect wards
	if (actual && onscary(mdef->mx, mdef->my, magr)) {
		return 0L;
	}
	// Don't focus-down steeds
	if (actual && mdef == u.usteed) {
		return 0L;
	}
	// Berserked creatures are effectively always conflicted, and aren't careful about anything unnecessary
	if (magr->mberserk) {
		return ALLOW_M | ALLOW_TM;
	}
	// Some madnesses cause infighting.
	//  These grudges are one-way by design.
	if(mdef->mophidio && triggers_ophidiophobia(magr)){
		return ALLOW_M | ALLOW_TM;
	}
	if(mdef->marachno && (
		triggers_arachnophobia(magr->data)
		|| (gender(magr) == 1 && humanoid_upperbody(magr->data))
	)){
		return ALLOW_M | ALLOW_TM;
	}
	if(mdef->mentomo && triggers_entomophobia(magr->data)){
		return ALLOW_M | ALLOW_TM;
	}
	if(mdef->mthalasso && is_aquatic(magr->data)){
		return ALLOW_M | ALLOW_TM;
	}
	if(mdef->mparanoid && mdef->m_lev < rnd(100)){
		return ALLOW_M | ALLOW_TM;
	}
	if(mdef->mhelmintho && triggers_helminthophobia(magr)){
		return ALLOW_M | ALLOW_TM;
	}
	if(magr->mcannibal && (magr->data->mflagsa&mdef->data->mflagsa)){
		//I.e., attack if the defender and attacker have some MA flags in common.
		return ALLOW_M | ALLOW_TM;
	}
	// careful around mandrakes 
	if (attacktype_fordmg(md, AT_BOOM, AD_MAND) && (
		(!mindless(magr->data) && magr->mhp < 100) ||
		(magr->mtame && maybe_polyd(u.mh, u.uhp) < 100)
	)) {
		return 0L;
	}
	// careful around cockatrices
	if (actual && touch_petrifies(md) && !resists_ston(magr)
		&& !mindless(magr->data) && distmin(magr->mx, magr->my, mdef->mx, mdef->my) < 3 && !MON_WEP(magr)
	) {
		return 0L;
	}
	// quest leaders should generally not be in danger
	if(!Role_if(PM_ANACHRONONAUT) && mdef->m_id == quest_status.leader_m_id){
		return 0L;
	}
	// quest friendlies shouldn't attack your pets
	if((quest_faction(magr) || quest_faction(mdef))
		&& magr->mpeaceful && mdef->mpeaceful
		&& (magr->mtame || mdef->mtame)
	){
		return 0L;
	}
	// Slime remnants attack everything not of the same peacefulness as them
	if(has_template(magr, SLIME_REMNANT) || has_template(mdef, SLIME_REMNANT)){
		if(magr->mpeaceful != mdef->mpeaceful)
			return ALLOW_M|ALLOW_TM;
		else return 0L;
	}
	// Beauteous ones attack everything not of the same peacefulness as them
	if(magr->mtyp == PM_BEAUTEOUS_ONE || mdef->mtyp == PM_BEAUTEOUS_ONE){
		if(magr->mpeaceful != mdef->mpeaceful)
			return ALLOW_M|ALLOW_TM;
		else return 0L;
	}
	// In the anachrononaut quest, all peaceful monsters are at threat from all hostile monsters.
	// The leader IS in serious danger
	// However, the imminent doom causes all peacefuls to forget any grudges against each other
	// (and Ilsensine can make her own forces coordinate, of course)
	if(Infuture){
		if(magr->mfaction != mdef->mfaction && (magr->mfaction == ILSENSINE_FACTION || mdef->mfaction == ILSENSINE_FACTION)) return ALLOW_M|ALLOW_TM;
		else return 0L;
	}
	// While the player is sowing discord (enhanced conflict), almost anything goes 
	if (u.sowdisc && !mdef->mtame && canseemon(magr) && canseemon(mdef))
	    return ALLOW_M|ALLOW_TM;
	// drow don't attack their own
	if (is_drow(ma) && is_drow(md) && (magr->mfaction == mdef->mfaction)) {
		return 0L;
	}
	// Lost (Lich) house drow are friendly to undead
	if (((is_drow(ma) && magr->mfaction == LOST_HOUSE) && mm_undead(mdef)) ||
		((is_drow(md) && mdef->mfaction == LOST_HOUSE) && mm_undead(magr))) {
		return 0L;
	}
	// supposedly purple worms are attracted to shrieking because they
	// like to eat shriekers, so attack the latter when feasible
	if (ma->mtyp == PM_PURPLE_WORM && md->mtyp == PM_SHRIEKER) {
		return ALLOW_M|ALLOW_TM;
	}
	// gray devourers eat psions
	if(ma->mtyp == PM_GRAY_DEVOURER){
		struct attack * aptr;
		aptr = permonst_dmgtype(md, AD_PSON);
		if(aptr || has_mind_blast_mon(mdef))
			return ALLOW_M|ALLOW_TM;
	}
	// ghouls attack gugs (who can retaliate, but won't initiate)
	if (ma->mtyp == PM_GHOUL && md->mtyp == PM_GUG) {
		return ALLOW_M|ALLOW_TM;
	}
#ifdef ATTACK_PETS
    // pets attack hostile monsters
	if (magr->mtame && !mdef->mpeaceful && (!actual || magr->mhp > magr->mhpmax/2 || banish_kill_mon(magr)) && !magr->mflee)
	    return ALLOW_M|ALLOW_TM;
	// and vice versa, with some limitations that will help your pet survive
	if (mdef->mtame && !magr->mpeaceful && (!actual || mdef->mhp > mdef->mhpmax/2 || banish_kill_mon(mdef)) && !mdef->meating && mdef != u.usteed && !mdef->mflee)
	    return ALLOW_M|ALLOW_TM;
#endif /* ATTACK_PETS */

	/* Since the quest guardians are under siege, it makes sense to have 
       them fight hostiles.  (But we don't want the quest leader to be in danger.) */
	if( ( (quest_faction(magr) && magr->mpeaceful && In_quest(&u.uz))
		  || (Role_if(PM_NOBLEMAN) && (ma->mtyp == PM_PEASANT) && magr->mpeaceful && In_quest(&u.uz))
		  || (Role_if(PM_MADMAN) && ma->mtyp == PM_LADY_CONSTANCE)
		)
		// && !(Race_if(PM_DROW) && !(flags.stag || Role_if(PM_NOBLEMAN) || !is_drow(md)))
		&& !(Role_if(PM_EXILE))
	) {
		if(magr->mpeaceful==TRUE && mdef->mpeaceful==FALSE) return ALLOW_M|ALLOW_TM;
		if(magr->mpeaceful==TRUE && mdef->mtame==TRUE) return FALSE;
	}
	/* and vice versa */
	if( ( (quest_faction(mdef) && mdef->mpeaceful && In_quest(&u.uz))
		  || (Role_if(PM_NOBLEMAN) && (md->mtyp == PM_PEASANT) && mdef->mpeaceful && In_quest(&u.uz))
		  || (Role_if(PM_MADMAN) && md->mtyp == PM_LADY_CONSTANCE)
		)
		// && !(Race_if(PM_DROW) && !(flags.stag || Role_if(PM_NOBLEMAN) || !is_drow(ma)))
		&& !(Role_if(PM_EXILE))
	){
		if(mdef->mpeaceful==TRUE && magr->mpeaceful==FALSE && rn2(2)) return ALLOW_M|ALLOW_TM;
		if(mdef->mpeaceful==TRUE && magr->mtame==TRUE) return FALSE;
	}
	if(magr->mfaction == MOON_FACTION && mdef->mfaction == CITY_FACTION){
		if(magr->mpeaceful==FALSE && mdef->mpeaceful==TRUE) return ALLOW_M|ALLOW_TM;
	}
	if(mdef->mfaction == MOON_FACTION && magr->mfaction == CITY_FACTION){
		if(mdef->mpeaceful==TRUE && magr->mpeaceful==FALSE && rn2(2)) return ALLOW_M|ALLOW_TM;
		if(mdef->mpeaceful==TRUE && magr->mtame==TRUE) return FALSE;
	}
	
	/* Various factions don't attack faction-mates */
	if(magr->mfaction == mdef->mfaction && mdef->mfaction == YENDORIAN_FACTION)
		return 0L;
	if(magr->mfaction == mdef->mfaction && mdef->mfaction == NECROMANCY_FACTION)
		return 0L;
	if(magr->mfaction == mdef->mfaction && mdef->mfaction == GOATMOM_FACTION)
		return 0L;
	if(magr->mfaction == mdef->mfaction && mdef->mfaction == QUEST_FACTION)
		return 0L;
	if(magr->mfaction == mdef->mfaction && mdef->mfaction == ILSENSINE_FACTION)
		return 0L;
	if(magr->mfaction == mdef->mfaction && mdef->mfaction == SEROPAENES_FACTION)
		return 0L;
	if(magr->mfaction == mdef->mfaction && mdef->mfaction == YELLOW_FACTION)
		return 0L;
	if(magr->mfaction == mdef->mfaction && mdef->mfaction == YOG_FACTION)
		return 0L;
	if(magr->mfaction == mdef->mfaction && mdef->mfaction == MOON_FACTION)
		return 0L;
	if(magr->mfaction == mdef->mfaction && mdef->mfaction == ROT_FACTION)
		return 0L;
	
	// rot kin attack almost anything
	if(magr->mfaction == ROT_FACTION || mdef->mfaction == ROT_FACTION) {
		return ALLOW_M|ALLOW_TM;
	}
	// dreadblossoms attack almost anything
	if(ma->mtyp == PM_DREADBLOSSOM_SWARM &&
		!(is_fey(md) || is_plant(md))
	) {
		return ALLOW_M|ALLOW_TM;
	}
	// brainblossoms attack almost anything (and vice versa)
	if(ma->mtyp == PM_BRAINBLOSSOM_PATCH &&
		!mindless_mon(mdef) &&
		!(is_fey(md) || is_plant(md))
	) {
		return ALLOW_M|ALLOW_TM;
	}
	if(md->mtyp == PM_BRAINBLOSSOM_PATCH &&
		!mindless_mon(magr) &&
		!(is_fey(ma) || is_plant(ma))
	) {
		return ALLOW_M|ALLOW_TM;
	}
	// grue attacks anything in the dark
	if(ma->mtyp == PM_GRUE &&
		isdark(mdef->mx, mdef->my)
	) {
		return ALLOW_M|ALLOW_TM;
	}
	// Oona attacks chaotics and vice versa (normal pet-vs-monster logic takes precedence)
	if ((ma->mtyp == PM_OONA || md->mtyp == PM_OONA)
		&& sgn(ma->maligntyp) == -1*sgn(md->maligntyp) //"Oona grudges on chaotics, but not on neutrals"
		&& !(magr->mtame || mdef->mtame) //Normal pet-vs-monster logic should take precedence over this
	){
		return ALLOW_M|ALLOW_TM;
	}
	/* elves (and Eladrin) vs. (orcs and undead and wargs) */
	if((is_elf(ma) || is_eladrin(ma) || ma->mtyp == PM_GROVE_GUARDIAN || ma->mtyp == PM_FORD_GUARDIAN || ma->mtyp == PM_FORD_ELEMENTAL)
		&& (is_orc(md) || md->mtyp == PM_WARG || is_ogre(md) || is_undead(mdef->data))
		&& !(is_orc(ma) || is_ogre(ma) || mm_undead(magr) || mdef->mfaction == HOLYDEAD_FACTION)
	)
		return ALLOW_M|ALLOW_TM;
	/* and vice versa */
	if((is_elf(md) || is_eladrin(md) || md->mtyp == PM_GROVE_GUARDIAN || md->mtyp == PM_FORD_GUARDIAN || md->mtyp == PM_FORD_ELEMENTAL) 
		&& (is_orc(ma) || ma->mtyp == PM_WARG || is_ogre(ma) || is_undead(magr->data))
		&& !(is_orc(md) || is_ogre(md) || mm_undead(mdef) || magr->mfaction == HOLYDEAD_FACTION)
	)
		return ALLOW_M|ALLOW_TM;

	/* dwarves vs. orcs */
	if(is_dwarf(ma) && (is_orc(md) || is_ogre(md) || is_troll(md))
					&&!(is_orc(ma) || is_ogre(ma) || is_troll(ma) || mm_undead(magr)))
		return ALLOW_M|ALLOW_TM;
	/* and vice versa */
	if(is_dwarf(md) && (is_orc(ma) || is_ogre(ma) || is_troll(ma))
					&&!(is_orc(md) || is_ogre(md) || is_troll(md) || mm_undead(mdef)))
		return ALLOW_M|ALLOW_TM;

	/* elves vs. drow */
	if(is_elf(ma) && is_drow(md) && mdef->mfaction != EILISTRAEE_SYMBOL && mdef->mfaction != PEN_A_SYMBOL)
		return ALLOW_M|ALLOW_TM;
	if(is_elf(md) && is_drow(ma) && magr->mfaction != EILISTRAEE_SYMBOL && magr->mfaction != PEN_A_SYMBOL)
		return ALLOW_M|ALLOW_TM;
	/* drow healer quest: drow vs. invaders */
	/* Other houses do fight the Y-cult, but that's handled by the drow faction code */
	/* Pen'a's faction is handled by the pet friendly code */
	if(In_quest(&u.uz)
		&& urole.neminum == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH
		&& magr->mfaction != Y_CULT_SYMBOL
		&& mdef->mfaction != Y_CULT_SYMBOL
		&& magr->mfaction != PEN_A_SYMBOL
		&& mdef->mfaction != PEN_A_SYMBOL
		&& (!is_drow(ma) + !is_drow(md)) == 1
	){
		return ALLOW_M|ALLOW_TM;
	}
	
	/* undead vs civs */
	if(!(In_cha(&u.uz)
#ifdef REINCARNATION
		|| Is_rogue_level(&u.uz)
#endif
	)){
		if(!(magr->mpeaceful && mdef->mpeaceful && is_undead(youracedata))){
			if(mm_undead(magr) && 
				(!is_witch_mon(mdef) && mdef->mtyp != PM_WITCH_S_FAMILIAR && !mdef->mpetitioner && !mm_undead(mdef) && !mindless_mon(mdef) && mdef->mfaction != NECROMANCY_FACTION && mdef->mfaction != YELLOW_FACTION)
			){
				return ALLOW_M|ALLOW_TM;
			}
			if((!is_witch_mon(magr) && magr->mtyp != PM_WITCH_S_FAMILIAR && !magr->mpetitioner && !mm_undead(magr) && !mindless_mon(magr) && magr->mfaction != NECROMANCY_FACTION && magr->mfaction != YELLOW_FACTION)
				&& mm_undead(mdef)
			){
				return ALLOW_M|ALLOW_TM;
			}
		}
	}
	
	/* Alabaster elves vs. oozes */
	if((ma->mtyp == PM_ALABASTER_ELF || ma->mtyp == PM_ALABASTER_ELF_ELDER || ma->mtyp == PM_SENTINEL_OF_MITHARDIR) 
		&& (md->mlet == S_PUDDING || md->mlet == S_BLOB || md->mlet == S_UMBER)
				&&	!(mm_undead(magr)))
		return ALLOW_M|ALLOW_TM;
	/* and vice versa */
	if((md->mtyp == PM_ALABASTER_ELF || md->mtyp == PM_ALABASTER_ELF_ELDER || md->mtyp == PM_SENTINEL_OF_MITHARDIR)
		&& (ma->mlet == S_PUDDING || ma->mlet == S_BLOB || ma->mlet == S_UMBER)
				&&	!(mm_undead(mdef)))
		return ALLOW_M|ALLOW_TM;

	/* Androids vs. mind flayers */
	if(free_android(ma) && is_mind_flayer(md))
		return ALLOW_M|ALLOW_TM;
	/* and vice versa */
	if(free_android(md) && is_mind_flayer(ma))
		return ALLOW_M|ALLOW_TM;

	/* drow vs. other drow */
	/* Note that factions may be different than the displayed house name, 
		as faction is set during generation and displayed house name goes by equipment! */
	if( is_drow(ma) && is_drow(md) && 
		magr->mfaction != mdef->mfaction &&
		!allied_faction(magr->mfaction, mdef->mfaction)
	){
		return ALLOW_M|ALLOW_TM;
	}

	/* drow vs. edderkops */
	if(is_drow(ma) && magr->mfaction != XAXOX && magr->mfaction != EDDER_SYMBOL && md->mtyp == PM_EDDERKOP){
		return ALLOW_M|ALLOW_TM;
	}
	/* and vice versa */
	if(is_drow(md) && mdef->mfaction != XAXOX && mdef->mfaction != EDDER_SYMBOL && ma->mtyp == PM_EDDERKOP){
		return ALLOW_M|ALLOW_TM;
	}
	
	/* Nazgul vs. hobbits */
	if(ma->mtyp == PM_NAZGUL && md->mtyp == PM_HOBBIT)
		return ALLOW_M|ALLOW_TM;
	/* and vice versa */
	if(md->mtyp == PM_NAZGUL && ma->mtyp == PM_HOBBIT)
		return ALLOW_M|ALLOW_TM;

	/* gnolls vs. minotaurs */
	if(is_gnoll(ma) && is_minotaur(md))
		return ALLOW_M|ALLOW_TM;
	/* and vice versa */
	if(is_gnoll(md) && is_minotaur(ma))
		return ALLOW_M|ALLOW_TM;

	/* angels vs. demons (excluding Lamashtu) */
	if (normalAngel(magr) && (is_demon(md) || fallenAngel(mdef)))
		return ALLOW_M|ALLOW_TM;
	/* and vice versa */
	if (normalAngel(mdef) && (is_demon(ma) || fallenAngel(magr)))
		return ALLOW_M|ALLOW_TM;

	/* monadics vs. undead */
	if(ma->mtyp == PM_MONADIC_DEVA && (mm_undead(mdef)))
		return ALLOW_M|ALLOW_TM;
	/* and vice versa */
	if(md->mtyp == PM_MONADIC_DEVA && (mm_undead(magr)))
		return ALLOW_M|ALLOW_TM;

	/* woodchucks vs. The Oracle */
	if(ma->mtyp == PM_WOODCHUCK && md->mtyp == PM_ORACLE)
		return ALLOW_M|ALLOW_TM;

	/* ravens like eyes */
	if(ma->mtyp == PM_RAVEN && md->mtyp == PM_FLOATING_EYE)
		return ALLOW_M|ALLOW_TM;

	/* dungeon fern spores hate everything */
	if(is_fern_spore(ma) && !is_vegetation(md))
		return ALLOW_M|ALLOW_TM;
	/* and everything hates them */
	if(is_fern_spore(md) && !is_vegetation(ma))
		return ALLOW_M|ALLOW_TM;
	/* everything attacks razorvine */
	// if(md->mtyp == PM_RAZORVINE && !is_vegetation(ma))
		// return ALLOW_M|ALLOW_TM;

	else if (magr->mtyp == PM_SKELETAL_PIRATE &&
		mdef->mtyp == PM_SOLDIER)
	    return ALLOW_M|ALLOW_TM;
	else if (mdef->mtyp == PM_SKELETAL_PIRATE &&
		magr->mtyp == PM_SOLDIER)
	    return ALLOW_M|ALLOW_TM;
	else if (magr->mtyp == PM_SKELETAL_PIRATE &&
		mdef->mtyp == PM_SERGEANT)
	    return ALLOW_M|ALLOW_TM;
	else if (mdef->mtyp == PM_SKELETAL_PIRATE &&
		magr->mtyp == PM_SERGEANT)
	    return ALLOW_M|ALLOW_TM;

	else if (magr->mtyp == PM_DAMNED_PIRATE &&
		mdef->mtyp == PM_SOLDIER)
	    return ALLOW_M|ALLOW_TM;
	else if (mdef->mtyp == PM_DAMNED_PIRATE &&
		magr->mtyp == PM_SOLDIER)
	    return ALLOW_M|ALLOW_TM;
	else if (magr->mtyp == PM_DAMNED_PIRATE &&
		mdef->mtyp == PM_SERGEANT)
	    return ALLOW_M|ALLOW_TM;
	else if (mdef->mtyp == PM_DAMNED_PIRATE &&
		magr->mtyp == PM_SERGEANT)
	    return ALLOW_M|ALLOW_TM;

	else if (magr->mtyp == PM_SKELETAL_PIRATE &&
		u.ukinghill)
	    return ALLOW_M|ALLOW_TM;
	else if (magr->mtyp == PM_DAMNED_PIRATE &&
		u.ukinghill)
	    return ALLOW_M|ALLOW_TM;
	else if (magr->mtyp == PM_GITHYANKI_PIRATE &&
		u.ukinghill)
	    return ALLOW_M|ALLOW_TM;
	else if (mdef->mtyp != PM_TINKER_GNOME && mdef->mtyp != PM_HOOLOOVOO && 
		is_vectored_mtyp(magr->mtyp))
	    return ALLOW_M|ALLOW_TM;
	/* Various other combinations such as dog vs cat, cat vs rat, and
	   elf vs orc have been suggested.  For the time being we don't
	   support those. */
	return 0L;
}

boolean
monnear(mon, x, y)
register struct monst *mon;
register int x,y;
/* Is the square close enough for the monster to move or attack into? */
{
	register int distance = dist2(mon->mx, mon->my, x, y);
	if (distance==2 && (mon->mtyp==PM_GRID_BUG || mon->mtyp==PM_BEBELITH)) return 0;
	if(mon->mtyp == PM_CLOCKWORK_SOLDIER || mon->mtyp == PM_CLOCKWORK_DWARF || 
	   mon->mtyp == PM_FABERGE_SPHERE
	) if(mon->mx + xdir[(int)mon->mvar_vector] != x || 
		   mon->my + ydir[(int)mon->mvar_vector] != y 
		) return 0;
	if(mon->mtyp == PM_FIREWORK_CART || 
	   mon->mtyp == PM_JUGGERNAUT || 
	   mon->mtyp == PM_ID_JUGGERNAUT
	){
		if(mon->mx + xdir[(int)mon->mvar_vector] == x && 
		   mon->my + ydir[(int)mon->mvar_vector] == y 
		) return ((boolean)(distance < 3));
		else if(rn2(2) && mon->mx + xdir[((int)mon->mvar_vector + 1)%8] == x && 
		   mon->my + ydir[((int)mon->mvar_vector + 1)%8] == y 
		) return (!rn2(2) && (distance < 3));
		else if(mon->mx + xdir[((int)mon->mvar_vector + 7)%8] == x && 
		   mon->my + ydir[((int)mon->mvar_vector + 7)%8] == y 
		) return (!rn2(2) && (distance < 3));
		else return 0;
	}
	return((boolean)(distance < 3));
}

/* really free dead monsters */
void
dmonsfree()
{
    struct monst **mtmp;
    int count = 0;

    for (mtmp = &fmon; *mtmp;) {
		if (DEADMONSTER(*mtmp)) {
			struct monst *freetmp = *mtmp;
			if(!((*mtmp)->deadmonster&DEADMONSTER_DEAD))
				impossible("dmonsfree: monster (%s) marked as purged but not marked dead", m_monnam(*mtmp));
			if(!((*mtmp)->deadmonster&DEADMONSTER_PURGE))
				impossible("dmonsfree: monster (%s) marked dead but not purged", m_monnam(*mtmp));
			*mtmp = (*mtmp)->nmon;
			rem_all_mx(freetmp);
			dealloc_monst(freetmp);
			count++;
		}
		else {
			/* sanity check */
			if ((*mtmp)->mhp < 1) {
				impossible("dmonsfree: monster (%s) has hp <1 (%d) but not marked dead", m_monnam(*mtmp), (*mtmp)->mhp);
				mondead(*mtmp);
			}
			mtmp = &(*mtmp)->nmon;
		}
    }

    if (count != iflags.purge_monsters)
	impossible("dmonsfree: %d removed doesn't match %d pending",
		   count, iflags.purge_monsters);
    iflags.purge_monsters = 0;
}

#endif /* OVL1 */
#ifdef OVLB

/* called when monster is moved to larger structure */
void
replmon(mtmp, mtmp2)
register struct monst *mtmp, *mtmp2;
{
    struct obj *otmp;

    /* transfer the monster's inventory */
    for (otmp = mtmp2->minvent; otmp; otmp = otmp->nobj) {
#ifdef DEBUG
	if (otmp->where != OBJ_MINVENT || otmp->ocarry != mtmp)
	    panic("replmon: minvent inconsistency");
#endif
	otmp->ocarry = mtmp2;
    }
    mtmp->minvent = 0;

	/* transfer the monster's components */
	mov_all_mx(mtmp, mtmp2);

    /* remove the old monster from the map and from `fmon' list */
    relmon(mtmp);

    /* finish adding its replacement */
#ifdef STEED
    if (mtmp == u.usteed) ; else	/* don't place steed onto the map */
#endif
    place_monster(mtmp2, mtmp2->mx, mtmp2->my);
    if (mtmp2->wormno)	    /* update level.monsters[wseg->wx][wseg->wy] */
	place_wsegs(mtmp2); /* locations to mtmp2 not mtmp. */
    if (emits_light_mon(mtmp2)) {
	/* since this is so rare, we don't have any `mon_move_light_source' */
	new_light_source(LS_MONSTER, (genericptr_t)mtmp2, emits_light_mon(mtmp2));
	/* here we rely on the fact that `mtmp' hasn't actually been deleted */
	del_light_source(mtmp->light);
    }
    mtmp2->nmon = fmon;
    fmon = mtmp2;
    if (u.ustuck == mtmp) u.ustuck = mtmp2;
#ifdef STEED
    if (u.usteed == mtmp) u.usteed = mtmp2;
#endif
    if (mtmp2->isshk) replshk(mtmp2);

    /* discard the old monster */
    dealloc_monst(mtmp);
}

/* release mon from display and monster list */
void
relmon(mon)
register struct monst *mon;
{
	register struct monst *mtmp;

	if (fmon == (struct monst *)0)  panic ("relmon: no fmon available.");

	removeMonster(mon->mx, mon->my);

	if(mon == fmon) fmon = fmon->nmon;
	else {
		for(mtmp = fmon; mtmp && mtmp->nmon != mon; mtmp = mtmp->nmon) ;
		if(mtmp)    mtmp->nmon = mon->nmon;
		else	    panic("relmon: mon not in list.");
	}
}

/* remove effects of mtmp from other data structures */
STATIC_OVL void
m_detach(mtmp, mptr)
struct monst *mtmp;
struct permonst *mptr;	/* reflects mtmp->data _prior_ to mtmp's death */
{
	if(mtmp->deadmonster & DEADMONSTER_PURGE) {
		impossible("attempting to detach already-marked deadmonster %s", m_monnam(mtmp));
		return;
	}
	if (mtmp->mleashed) m_unleash(mtmp, FALSE);
	    /* to prevent an infinite relobj-flooreffects-hmon-killed loop */
	mtmp->mtrapped = 0;
	mtmp->mhp = 0; /* simplify some tests: force mhp to 0 */
	relobj(mtmp, 0, FALSE);
	summoner_gone(mtmp, FALSE);
	remove_monster(mtmp->mx, mtmp->my);
	del_light_source(mtmp->light);
	stop_all_timers(mtmp->timed);
	newsym(mtmp->mx,mtmp->my);
	unstuck(mtmp);
	fill_pit(mtmp->mx, mtmp->my);

	if(mtmp->isshk) shkgone(mtmp);
	if(mtmp->wormno) wormgone(mtmp);

	mtmp->deadmonster |= DEADMONSTER_PURGE;
	iflags.purge_monsters++;
}

/* find the worn amulet of life saving which will save a monster */
struct obj *
mlifesaver(mon)
struct monst *mon;
{
	struct obj *otmp;
	if (!nonliving(mon->data)) {
	    otmp = which_armor(mon, W_AMUL);

	    if (otmp && otmp->otyp == AMULET_OF_LIFE_SAVING)
		return otmp;
	}
	for(otmp = mon->minvent; otmp; otmp = otmp->nobj){
	    if (otmp && otmp->owornmask && (check_oprop(otmp, OPROP_LIFE) || check_oprop(otmp, OPROP_SLIF)))
			return otmp;
	}
	return (struct obj *)0;
}

boolean
plague_victim_on_level()
{
	struct monst *mon;
	struct obj *obj;
	for(mon = fmon; mon; mon = mon->nmon)
		if(has_template(mon, PLAGUE_TEMPLATE) && !DEADMONSTER(mon))
			return TRUE;

	int owhere = ((1 << OBJ_FLOOR) |
				(1 << OBJ_INVENT) |
				(1 << OBJ_MINVENT) |
				(1 << OBJ_BURIED) |
				// (1 << OBJ_CONTAINED) |
				(1 << OBJ_INTRAP));

	for (obj = start_all_items(&owhere); obj; obj = next_all_items(&owhere)) {
		if(Is_real_container(obj) && obj->spe == 9){
			end_all_items();
			return TRUE;
		}
	}
	return FALSE;
}

boolean
allied_iaso_on_level(mtmp)
struct monst *mtmp;
{
	struct monst *mon;
	for(mon = fmon; mon; mon = mon->nmon)
		if(mon->mtyp == PM_IASOIAN_ARCHON
		  && !mon->mcan && !mon->mspec_used
		  && !nonthreat(mon)
		  && !mon->mtame == !mtmp->mtame
		  && mon->mpeaceful == mtmp->mpeaceful
		)
			return TRUE;
	return FALSE;
}

struct monst *
random_plague_victim()
{
	struct monst *mon;
	int count = 0;
	for(mon = fmon; mon; mon = mon->nmon)
		if(has_template(mon, PLAGUE_TEMPLATE) && !DEADMONSTER(mon))
			count++;

	if(count){
		count = rn2(count);
		for(mon = fmon; mon; mon = mon->nmon)
			if(has_template(mon, PLAGUE_TEMPLATE) && !DEADMONSTER(mon)){
				if(!count)
					return mon;
				else count--;
			}
	}

	return (struct monst *) 0;
}

struct obj *
random_sacked_victim()
{
	struct obj *obj;
	int count = 0;
	int owhere = ((1 << OBJ_FLOOR) |
				(1 << OBJ_INVENT) |
				(1 << OBJ_MINVENT) |
				(1 << OBJ_BURIED) |
				// (1 << OBJ_CONTAINED) |
				(1 << OBJ_INTRAP));

	for (obj = start_all_items(&owhere); obj; obj = next_all_items(&owhere)) {
		if(Is_real_container(obj) && obj->spe == 9)
			count++;
	}

	if(count){
		count = rn2(count);
		owhere = ((1 << OBJ_FLOOR) |
				(1 << OBJ_INVENT) |
				(1 << OBJ_MINVENT) |
				(1 << OBJ_BURIED) |
				// (1 << OBJ_CONTAINED) |
				(1 << OBJ_INTRAP));

		for (obj = start_all_items(&owhere); obj; obj = next_all_items(&owhere)) {
			if(Is_real_container(obj) && obj->spe == 9){
				if(!count){
					end_all_items();
					return obj;
				}
				else count--;
			}
		}
	}

	return (struct obj *) 0;
}

void
timeout_random_allied_iaso(mtmp)
struct monst *mtmp;
{
	struct monst *mon;
	int count = 0;
	for(mon = fmon; mon; mon = mon->nmon)
		if(mon->mtyp == PM_IASOIAN_ARCHON
		  && !mon->mcan && !mon->mspec_used
		  && !nonthreat(mon)
		  && !mon->mtame == !mtmp->mtame
		  && mon->mpeaceful == mtmp->mpeaceful
		)
			count++;
	if(!count)
		return;

	count = rn2(count);
	for(mon = fmon; mon; mon = mon->nmon)
		if(mon->mtyp == PM_IASOIAN_ARCHON
		  && !mon->mcan && !mon->mspec_used
		  && !nonthreat(mon)
		  && !mon->mtame == !mtmp->mtame
		  && mon->mpeaceful == mtmp->mpeaceful
		){
			if(!count){
				mon->mspec_used = mtmp->mhpmax;
				return;
			}
			else count--;
		}
}

/* maybe kills mtmp, possibly lifesaving it */
STATIC_OVL void
lifesaved_monster(mtmp)
struct monst *mtmp;
{
	struct obj *lifesave = mlifesaver(mtmp);
	boolean messaged = FALSE;
	int lifesavers = 0;
	int i;
#define LSVD_ANA 0x00000001	/* anachrononaut quest */
#define LSVD_IAS 0x00000002	/* Iasoian Archon grants recovery */
#define LSVD_HLO 0x00000004	/* Halo (Blessed) */
#define LSVD_UVU 0x00000008	/* uvuuduam + prayerful thing */
#define LSVD_ASC 0x00000010	/* drained the life from another */
#define LSVD_TRA 0x00000020	/* Transforms */
#define LSVD_OBJ 0x00000040	/* lifesaving items */
#define LSVD_ILU 0x00000080	/* illuminated */
#define LSVD_TWN 0x00000100	/* twin sibling */
#define LSVD_FRC 0x00000200	/* fractured kamerel */
#define LSVD_NBW 0x00000400	/* nitocris's black wraps */
#define LSVD_YEL 0x00000800	/* Cannot die unless on the Astral Plane */
#define LSVD_PLY 0x00001000	/* polypoids */
#define LSVD_NIT 0x00002000	/* Nitocris becoming a ghoul */
#define LSVD_KAM 0x00004000	/* kamerel becoming fractured */
#define LSVD_ALA 0x00008000	/* alabaster decay */
#define LSVD_FLS 0x00010000	/* God of flesh claims body */
#define LSVDLAST LSVD_FLS	/* last lifesaver */

	/* set to kill */
	mtmp->mhp = 0;

	/* get all lifesavers */
	if (Infuture && !(mtmp->mpeaceful) && !rn2(20))
		lifesavers |= LSVD_ANA;
	if(mtmp->mtyp == PM_NITOCRIS
		&& which_armor(mtmp, W_ARMC)
		&& which_armor(mtmp, W_ARMC)->oartifact == ART_SPELL_WARDED_WRAPPINGS_OF_
	)
		lifesavers |= LSVD_NBW;
	if (mtmp->mspec_used == 0 && (is_uvuudaum(mtmp->data) || mtmp->mtyp == PM_PRAYERFUL_THING) && !mtmp->mcan)
		lifesavers |= LSVD_UVU;
	if (lifesave)
		lifesavers |= LSVD_OBJ;
	if ((!rn2(20) && (mtmp->mtyp == PM_ALABASTER_ELF
		|| mtmp->mtyp == PM_ALABASTER_ELF_ELDER
		|| is_alabaster_mummy(mtmp->data)
		)))
		lifesavers |= LSVD_ALA;
	if (mtmp->mtyp == PM_TETTIGON_LEGATUS)
		lifesavers |= LSVD_TRA;
	if (Infuture && mtmp->mpeaceful && !is_myrkalfr(mtmp) && !nonliving(mtmp->data) && !is_android(mtmp->data))
		lifesavers |= LSVD_FLS;
	if (has_template(mtmp, FRACTURED) && !rn2(2) && !mtmp->mcan)
		lifesavers |= LSVD_FRC;
	if (mtmp->ispolyp)
		lifesavers |= LSVD_PLY;
	if (has_template(mtmp, ILLUMINATED) && !mtmp->mcan)
		lifesavers |= LSVD_ILU;
	if (mtmp->zombify && is_kamerel(mtmp->data))
		lifesavers |= LSVD_KAM;
	if (Shattering && rn2(2) && !has_template(mtmp, FRACTURED))
		lifesavers |= LSVD_KAM;
	if (mtmp->mtyp == PM_NITOCRIS)
		lifesavers |= LSVD_NIT;
	if (mtmp->mtyp == PM_BLESSED && !mtmp->mcan && rn2(3))
		lifesavers |= LSVD_HLO;
	if (mtmp->mtyp == PM_CYCLOPS && !mtmp->mcan && mon_has_arti(mtmp, 0) && plague_victim_on_level())
		lifesavers |= LSVD_ASC;
	if (mtmp->mtyp == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH && !mtmp->mcan && (mon_has_arti(mtmp, 0) || !quest_status.touched_artifact) && plague_victim_on_level())
		lifesavers |= LSVD_ASC;
	if (allied_iaso_on_level(mtmp))
		lifesavers |= LSVD_IAS;
	if ((mtmp->mtyp == PM_PUPPET_EMPEROR_XELETH || mtmp->mtyp == PM_PUPPET_EMPRESS_XEDALLI) && !Is_astralevel(&u.uz))
		lifesavers |= LSVD_YEL;
	if (mtmp->mtyp == PM_TWIN_SIBLING)
		lifesavers |= LSVD_TWN;

	/* some lifesavers do NOT work on stone/gold/glass-ing */
	if (stoned || golded || glassed)
		lifesavers &= ~(LSVD_FLS | LSVD_ALA | LSVD_NBW | LSVD_FRC | LSVD_PLY | LSVD_ILU | LSVD_KAM | LSVD_NIT | LSVD_HLO);

	/* some lifesavers should SILENTLY fail to protect from genocide */
	if (mvitals[monsndx(mtmp->data)].mvflags & G_GENOD && !In_quest(&u.uz))
		lifesavers &= ~(LSVD_FRC | LSVD_NBW | LSVD_KAM | LSVD_HLO);

	/* quick check -- if no lifesavers, let's fail immediately */
	if (!lifesavers) {
		return;
	}
	/* perform lifesaving, preferring small-number lifesavers */
	for (i = 1; ((i <= LSVDLAST) && (mtmp->mhp < 1)); i = i<<1) {
		if (!(lifesavers&i)) {
			continue;
		}

		switch (i)
		{
		case LSVD_ANA:
			/* message */
			if (canseemon(mtmp)) {
				messaged = TRUE;
				pline("But wait...");
				if (mon_attacktype(mtmp, AT_EXPL)
					|| mon_attacktype(mtmp, AT_BOOM))
					pline("%s reappears, looking much better!", Monnam(mtmp));
				else
					pline("%s flickers, then reappears looking much better!", Monnam(mtmp));
			}
			break;

		case LSVD_NBW:
			/* message */
			if (canseemon(mtmp)) {
				messaged = TRUE;
				pline("But wait...");
				pline("Something vast and terrible writhes beneath %s wrappings!", hisherits(mtmp));
				pline("It's trying to escape!");
			}
			if(rn2(3)){
				if(which_armor(mtmp, W_ARMC)->oeroded3){
					if (cansee(mtmp->mx, mtmp->my))
						pline("%s wrappings rip to shreds!", s_suffix(Monnam(mtmp)));
					m_useup(mtmp, which_armor(mtmp, W_ARMC));
					continue; //didn't life save after all :(
				} else {
					which_armor(mtmp, W_ARMC)->oeroded3 = 1;
					if (canseemon(mtmp))
						pline("%s wrappings fray, but hold!", s_suffix(Monnam(mtmp)));
				}
			} else {
				if (canseemon(mtmp)){
					pline("%s wrappings hold!", s_suffix(Monnam(mtmp)));
					pline("The writhing subsides!");
				}
			}
			break;

		case LSVD_UVU:
			/* message */
			if (couldsee(mtmp->mx, mtmp->my)) {
				messaged = TRUE;
				pline("But wait...");
				pline("A glowing halo forms over %s!",
					mon_nam(mtmp));
				if (mon_attacktype(mtmp, AT_EXPL)
					|| mon_attacktype(mtmp, AT_BOOM))
					pline("%s reconstitutes!", Monnam(mtmp));
				else
					pline("%s looks much better!", Monnam(mtmp));
			}
			/* restore level, maxhp */
			if (mtmp->m_lev < 38)
				mtmp->m_lev = 38;
			if (mtmp->mhpmax < 38*hd_size(mtmp->data))
				mtmp->mhpmax = 38*hd_size(mtmp->data);
			/* set mspec_used */
			mtmp->mspec_used = mtmp->mhpmax / 5;
			break;
		case LSVD_TRA:{
			struct obj *otmp;
			/* message */
			if (couldsee(mtmp->mx, mtmp->my)) {
				messaged = TRUE;
				pline("But wait...");
				pline("A glowing crack forms around the head!");
			}
			/* restore level, maxhp */
			if (mtmp->m_lev < 16)
				mtmp->m_lev = 16;
			if (mtmp->mhpmax < 16*hd_size(mtmp->data))
				mtmp->mhpmax = 16*hd_size(mtmp->data);
			if(mtmp->mcan)
				set_mcan(mtmp, FALSE);
			otmp = mksobj_at(ENCOUNTER_EXOSKELETON, mtmp->mx, mtmp->my, NO_MKOBJ_FLAGS);
			if(otmp){
				otmp->quan = 1;
				if(stoned)
					set_material(otmp, MINERAL);
				else if(golded)
					set_material(otmp, GOLD);
				else if(glassed)
					set_material(otmp, GLASS);
				fix_object(otmp);
			}
			set_mon_data(mtmp, Insight > 40 ? PM_TRANSCENDENT_TETTIGON : PM_UNMASKED_TETTIGON);
			mtmp->m_insight_level = 5+rn2(6);
			newsym(x(mtmp), y(mtmp));
		}break;
		case LSVD_ASC:{
			struct monst *victim = random_plague_victim();
			struct obj *sacked_victim = 0;
			if(!victim)
				sacked_victim = random_sacked_victim();

			if(!victim && !sacked_victim)
				break; //???

			/* message */
			if (!Blind && (couldsee(mtmp->mx, mtmp->my) || (victim && couldsee(victim->mx, victim->my)))) {
				messaged = TRUE;
				pline("But wait...");
				if(victim)
					pline("A glowing mist rises from %s and flows to %s!",
						mon_nam(victim), mon_nam(mtmp));
				else
					pline("A glowing mist gathers around %s!", mon_nam(mtmp));

				if (mon_attacktype(mtmp, AT_EXPL)
					|| mon_attacktype(mtmp, AT_BOOM))
					pline("%s reconstitutes!", Monnam(mtmp));
				else
					pline("%s looks much better!", Monnam(mtmp));
			}
			if(victim){
				if(canseemon(victim))
					pline("%s dies of %s illness!", Monnam(victim), mhis(victim));
				mondied(victim);
			}
			else if(sacked_victim){
				kill_giants_sack(sacked_victim);
			}
			break;
		}
		case LSVD_IAS:{
			timeout_random_allied_iaso(mtmp);
			/* message */
			if (couldsee(mtmp->mx, mtmp->my)) {
				messaged = TRUE;
				pline("But wait...");
				if (mon_attacktype(mtmp, AT_EXPL)
					|| mon_attacktype(mtmp, AT_BOOM))
					pline("%s reconstitutes!", Monnam(mtmp));
				pline("%s recovers!", Monnam(mtmp));
			}
			break;
		}
		case LSVD_OBJ:
			/* message */
			if (couldsee(mtmp->mx, mtmp->my)) {
				messaged = TRUE;
				pline("But wait...");
				pline("%s %s begins to glow!",
					s_suffix(Monnam(mtmp)),
					lifesave->otyp == AMULET_OF_LIFE_SAVING ? "medallion" : xname(lifesave));

				if(lifesave->otyp == AMULET_OF_LIFE_SAVING && !check_oprop(lifesave, OPROP_LIFE) && !check_oprop(lifesave, OPROP_SLIF))
					makeknown(AMULET_OF_LIFE_SAVING);

				if (mon_attacktype(mtmp, AT_EXPL)
				    || mon_attacktype(mtmp, AT_BOOM))
					pline("%s reconstitutes!", Monnam(mtmp));
				else
					pline("%s looks much better!", Monnam(mtmp));
				if(lifesave->otyp == AMULET_OF_LIFE_SAVING && !check_oprop(lifesave, OPROP_LIFE) && !check_oprop(lifesave, OPROP_SLIF))
					pline_The("medallion crumbles to dust!");
				else 
					pline_The("%s fades.", xname(lifesave));
			}
			if(check_oprop(lifesave, OPROP_SLIF)){
				remove_oprop(lifesave, OPROP_SLIF);
			}
			else if(check_oprop(lifesave, OPROP_LIFE)){
				remove_oprop(lifesave, OPROP_LIFE);
			}
			else {
				/* use up amulet (or other item, I guess) */
				m_useup(mtmp, lifesave);
			}
			break;
		case LSVD_ALA:
			/* message */
			if (canseemon(mtmp)) {
				messaged = TRUE;
				pline("%s putrefies with impossible speed!", Monnam(mtmp));
			}
			/* alabaster-specific effects */
			mtmp->mspec_used = 0;
			if(is_alabaster_mummy(mtmp->data) && mtmp->mvar_syllable >= SYLLABLE_OF_STRENGTH__AESH && mtmp->mvar_syllable <= SYLLABLE_OF_SPIRIT__VAUL){
				mksobj_at(mtmp->mvar_syllable, mtmp->mx, mtmp->my, NO_MKOBJ_FLAGS);
				if(mtmp->mvar_syllable == SYLLABLE_OF_SPIRIT__VAUL)
					mtmp->mintrinsics[(DISPLACED - 1) / 32] &= ~(1 << (DISPLACED - 1) % 32);
				mtmp->mvar_syllable = 0; //Lose the bonus if resurrected
			}
			newcham(mtmp, (rn2(4) ? PM_ACID_BLOB : PM_BLACK_PUDDING), FALSE, FALSE);
			break;
		case LSVD_FLS:{
			/* message */
			int mtyp = mtmp->mtyp;
			switch(quest_status.leader_m_id == mtmp->m_id ? 0 : rnd(8)){
				default: //0 or 5-8
					if(canseemon(mtmp)){
						pline("But wait...");
						pline("%s screams and writhes. You hear %s bones splintering!", Monnam(mtmp), mhis(mtmp));
					}
					else You_hear("screaming.");
					if(quest_status.leader_m_id == mtmp->m_id){
						quest_status.leader_m_id = 0;
						quest_status.leader_is_dead = TRUE;
						verbalize("**ALAAAAAAAAAAAAAAAAA:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa...**");
					}
					set_mon_data(mtmp, PM_COILING_BRAWN);
					possibly_unwield(mtmp, FALSE);	/* might lose use of weapon */
					mon_break_armor(mtmp, FALSE);
				break;
				case 1:{
					struct obj *helm, *robe;
					helm = which_armor(mtmp, W_ARMH);
					robe = which_armor(mtmp, W_ARMC);
					if(helm && !FacelessHelm(helm))
						helm = (struct obj *)0;
					if(robe && !FacelessCloak(robe))
						robe = (struct obj *)0;
					if(canseemon(mtmp)){
						pline("But wait...");
						pline("%s gags and grasps at %s face. Purple tentacles tear out of %s %s!", Monnam(mtmp), mhis(mtmp), mhis(mtmp), robe ? "robe" : helm ? "helmet" : "mouth");
					}
					else You_hear("gagging.");
					if(helm)
						m_useup(mtmp, helm);
					if(robe)
						m_useup(mtmp, robe);
					set_mon_data(mtmp, PM_MIND_FLAYER);
				}
				break;
				case 2:
					if(canseemon(mtmp))
						pline("%s head splits open in a profusion of fungal growths!", s_suffix(Monnam(mtmp)));
					else You_hear("a wet crack.");
					set_mon_data(mtmp, PM_FUNGAL_BRAIN);
					possibly_unwield(mtmp, FALSE);	/* might lose use of weapon */
					mon_break_armor(mtmp, FALSE);
				break;
				case 3:
					if(canseemon(mtmp)){
						if(!which_armor(mtmp, W_ARMH)){
							pline("Or not....");
							pline("The back of %s head falls off, and %s eyes gleam with newfound madness!", s_suffix(mon_nam(mtmp)), mhis(mtmp));
						}
						else pline("Or not...?");
					}
					set_template(mtmp, CRANIUM_RAT);
				break;
				case 4:
					if(canseemon(mtmp)){
						pline("But wait...");
						pline("%s screams and unnaturally contorts. Tentacles worm their way out of %s body!", Monnam(mtmp), mhis(mtmp));
					}
					else You_hear("screaming.");
					set_template(mtmp, PSEUDONATURAL);
				break;
			}
			if(Role_if(PM_ANACHRONONAUT) && In_quest(&u.uz) && Is_qstart(&u.uz) && !(quest_status.leader_is_dead)){
				if(mtyp == PM_TROOPER){
					verbalize("**WARNING: Anomalous vital signs detected in trooper %d**", (int)(mtmp->m_id));
					if(!canspotmon(mtmp)) map_invisible(mtmp->mx, mtmp->my);
				} else if(mtyp == PM_MYRKALFAR_WARRIOR){
					verbalize("**WARNING: Anomalous vital signs detected in warrior %d**", (int)(mtmp->m_id));
					if(!canspotmon(mtmp)) map_invisible(mtmp->mx, mtmp->my);
				} else {
					verbalize("**WARNING: Anomalous vital signs detected in citizen %d**", (int)(mtmp->m_id));
					if(!canspotmon(mtmp)) map_invisible(mtmp->mx, mtmp->my);
				}
			}
			messaged = TRUE;
			mtmp->mvar_lifesigns = mtyp;
			untame(mtmp, FALSE);
			set_malign(mtmp);
			set_faction(mtmp, ILSENSINE_FACTION);
			newsym(mtmp->mx, mtmp->my);
		}	break;
		case LSVD_FRC:
			/* message */
			if (couldsee(mtmp->mx, mtmp->my)) {
				messaged = TRUE;
				pline("But wait...");
				if (canseemon(mtmp))
					pline("%s fractures further%s, but now looks uninjured!", Monnam(mtmp), !is_silent(mtmp->data) ? " with an unearthly scream" : "");
				else
					You_hear("something crack%s!", !is_silent(mtmp->data) ? " with an unearthly scream" : "");
			}
			/* make hostile */
			untame(mtmp, 0);
			/* boost level */
			if(mtmp->m_lev < 45)
				mtmp->m_lev += 4;
			mtmp->mhpmax += d(4, 8);
			break;

		case LSVD_HLO:
			/* message */
			if (couldsee(mtmp->mx, mtmp->my)) {
				messaged = TRUE;
				pline("But wait...");
				pline("A glowing halo forms over %s!",
					mon_nam(mtmp));
			}
			break;
		case LSVD_YEL:
			/* message */
			if (couldsee(mtmp->mx, mtmp->my)) {
				messaged = TRUE;
				pline("But wait...");
				pline("A sickly yellow aura forms around %s!",
					mon_nam(mtmp));
			}
			mtmp->mvar_yellow_lifesaved = TRUE;
			break;
		case LSVD_TWN:
			/* message */
			if (couldsee(mtmp->mx, mtmp->my)) {
				messaged = TRUE;
				pline("But wait...");
				pline("Lightning seems to strike %s!",
					mon_nam(mtmp));
			}
			mtmp->mvar_twin_lifesaved = TRUE;
			break;
		case LSVD_ILU:
			/* normally has only a 1/3 chance of losing its lifesaving ability */
			if (rn2(3) && !(
				(mvitals[monsndx(mtmp->data)].mvflags & G_GENOD && !In_quest(&u.uz))	/* genocide-death always breaks halo */
				))
			{
				/* free lifesaving */;
			}
			/* else this revive would be its last */
			else {
				if (lifesavers & (LSVD_PLY)) {
					/* we would prefer to use some lower forms of lifesaving over losing illuminated */
					continue;
				}
				else {
					/* note that we will strip illuminated status - we will handle other parts later */
					lifesavers &= ~LSVD_ILU;
				}
			}
			/* message */
			if (couldsee(mtmp->mx, mtmp->my)) {
				messaged = TRUE;
				pline("But wait...");
				pline("A glowing halo forms over %s!",
					mon_nam(mtmp));
			}
			/* If marked to do so, remve illuminated status */
			if (!(lifesavers&LSVD_ILU)){
				set_template(mtmp, 0);
				del_light_source(mtmp->light);
				if (emits_light_mon(mtmp))
					new_light_source(LS_MONSTER, (genericptr_t)mtmp, emits_light_mon(mtmp));
			}
			break;
		case LSVD_PLY:
			/* message */
			if (canseemon(mtmp)){
				messaged = TRUE;
				pline("But wait...");
				pline("%s mask breaks!", s_suffix(Monnam(mtmp)));
			}
			/* turn into a polypoid */
			mtmp->ispolyp = 0;
			newcham(mtmp, PM_POLYPOID_BEING, FALSE, FALSE);
			mtmp->m_insight_level = 40;
			break;
		case LSVD_KAM:
			if (couldsee(mtmp->mx, mtmp->my)) {
				messaged = TRUE;
				pline("But wait...");
				if (canseemon(mtmp))
					pline("%s fractures%s, but now looks uninjured!", Monnam(mtmp), !is_silent(mtmp->data) ? " with an unearthly scream" : "");
				else
					You_hear("something crack%s!", !is_silent(mtmp->data) ? " with an unearthly scream" : "");
			}
			set_template(mtmp, FRACTURED);
			/* make hostile */
			untame(mtmp, 0);
			/* boost level */
			mtmp->m_lev += 4;
			mtmp->mhpmax += d(4, 8);
			break;
		case LSVD_NIT:
			if(lifesavers&LSVD_NBW)
				pline("%s is boiling and screaming in agony!", Monnam(mtmp));
			else if(canseemon(mtmp))
				pline("%s screams in agony, bloats, and begins boiling violently!", Monnam(mtmp));
			else
				pline("%s screams in agony and begins boiling violently!", Monnam(mtmp));
			messaged = TRUE;
			if(cansee(mtmp->mx, mtmp->my)){
				int nyar_form = rn2(SIZE(nyar_description));
				pline("The escaping phantasmal mist condenses into %s.", nyar_description[nyar_form]);
				pline("%s tears off the right half of %s face before rising through the ceiling!", nyar_name[nyar_form], s_suffix(Monnam(mtmp)));
				change_usanity(u_sanity_loss_nyar(), TRUE);
				TRANSCENDENCE_IMPURITY_UP(TRUE)
				if(!(uarmc && uarmc->oartifact == ART_SPELL_WARDED_WRAPPINGS_OF_))
					u.umadness |= MAD_THOUSAND_MASKS;
			}
			//Gold turns to lead
			struct obj *nobj;
			for(struct obj *otmp = mtmp->minvent; otmp; otmp = nobj){
				nobj = otmp->nobj;
				if(otmp->obj_material == GOLD && otmp->otyp != GOLD_PIECE){
					set_material_gm(otmp, LEAD);
				}
			}
			set_mon_data(mtmp, PM_GHOUL_QUEEN_NITOCRIS);
			//Surprisingly, this is an effective means of life saving!
			break;
		}
		/* perform common lifesaving effects */
		mtmp->mcanmove = 1;
		mtmp->mfrozen = 0;
		if (get_mx(mtmp, MX_EDOG)) {
			wary_dog(mtmp, FALSE);
		}
		if (mtmp->mhpmax < 10)
			mtmp->mhpmax = 10;
		mtmp->mhp = mtmp->mhpmax;
		/* genocided creatures get re-killed */
		if (mvitals[monsndx(mtmp->data)].mvflags & G_GENOD && !In_quest(&u.uz)) {
			if (messaged) {
				pline("Unfortunately %s is still genocided...",
					mon_nam(mtmp));
			}
			mtmp->mhp = 0;
		}
	}
#undef LSVD_ANA
#undef LSVD_UVU
#undef LSVD_OBJ
#undef LSVD_ALA
#undef LSVD_FRC
#undef LSVD_ILU
#undef LSVD_PLY
#undef LSVD_KAM
#undef LSVD_HLO
#undef LSVDLAST
	return;
}

void
mondead(mtmp)
register struct monst *mtmp;
{
	struct permonst *mptr;
	int tmp;

	/* cease occupation if the monster was associated */
	if(mtmp->moccupation) stop_occupation();
	
	if(mtmp->isgd) {
		/* if we're going to abort the death, it *must* be before
		 * the m_detach or there will be relmon problems later */
		if(!grddead(mtmp)) return;
	}
	if (!DEADMONSTER(mtmp)) {
		lifesaved_monster(mtmp);
		if (mtmp->mhp > 0) return;
	}
	/* we did not lifesave */
	mtmp->deadmonster |= DEADMONSTER_DEAD;
	mtmp->mbdrown = 0;
	//Special messages (Nyarlathotep)
	if(canseemon(mtmp) && (mtmp->mtyp == PM_GOOD_NEIGHBOR || mtmp->mtyp == PM_HMNYW_PHARAOH)){
		int nyar_form = rn2(SIZE(nyar_description));
		pline("%s twists and morphs into %s.", Monnam(mtmp), nyar_description[nyar_form]);
		pline("%s rises through the ceiling!", nyar_name[nyar_form]);
		change_usanity(u_sanity_loss_nyar(), TRUE);
		TRANSCENDENCE_IMPURITY_UP(TRUE)
		if(!(uarmc && uarmc->oartifact == ART_SPELL_WARDED_WRAPPINGS_OF_))
			u.umadness |= MAD_THOUSAND_MASKS;
	}
	

#ifdef STEED
	/* Player is thrown from his steed when it dies */
	if (mtmp == u.usteed)
		dismount_steed(DISMOUNT_GENERIC);
#endif

	mptr = mtmp->data;		/* save this for m_detach() */
	/* restore chameleon, lycanthropes to true form at death */
	if (mtmp->cham)
		set_mon_data(mtmp, cham_to_pm[mtmp->cham]);
	else if (mtmp->mtyp == PM_WEREJACKAL)
		set_mon_data(mtmp, PM_HUMAN_WEREJACKAL);
	else if (mtmp->mtyp == PM_WEREWOLF)
		set_mon_data(mtmp, PM_HUMAN_WEREWOLF);
	else if (mtmp->mtyp == PM_WERERAT)
		set_mon_data(mtmp, PM_HUMAN_WERERAT);
	else if (mtmp->mtyp == PM_ANUBAN_JACKAL)
		set_mon_data(mtmp, PM_ANUBITE);

	if(mtmp->mtyp == PM_WITCH_S_FAMILIAR && mtmp->mvar_witchID){
		dead_familiar(mtmp->mvar_witchID);
	}

	/* if MAXMONNO monsters of a given type have died, and it
	 * can be done, extinguish that monster.
	 *
	 * mvitals[].died does double duty as total number of dead monsters
	 * and as experience factor for the player killing more monsters.
	 * this means that a dragon dying by other means reduces the
	 * experience the player gets for killing a dragon directly; this
	 * is probably not too bad, since the player likely finagled the
	 * first dead dragon via ring of conflict or pets, and extinguishing
	 * based on only player kills probably opens more avenues of abuse
	 * for rings of conflict and such.
	 */
	tmp = monsndx(mtmp->data);
	if (mvitals[tmp].died < 255) mvitals[tmp].died++;
	if(tmp == PM_INDEX_WOLF){
		//The phase of the moon may change to full.
		if(flags.moonphase != FULL_MOON && flags.moonphase != HUNTING_MOON){
			if(ACURR(A_WIS) > 14){
				pline("The moon waxes full.");
				change_uinsight(1); //iff you've already pierced the veil
			}
			flags.moonphase = phase_of_the_moon();
			change_luck(1);
		}
		struct obj * stake = mksartifact(ART_STAKE_OF_WITHERING);
		if(stake){
			place_object(stake, mtmp->mx, mtmp->my);
		}
		mksobj_at(PORTABLE_ELECTRODE, mtmp->mx, mtmp->my, NO_MKOBJ_FLAGS);
	}
	if(tmp == PM_MOON_S_CHOSEN){
		//The moon draws near.
		int luckmod = flags.moonphase != FULL_MOON ? 2 : 1;
		pline("The moon draws near.");
		flags.moonphase = phase_of_the_moon();
		quest_status.moon_close = TRUE;
		change_luck(luckmod);
		u.uevent.qcompleted = TRUE; //Insurance
	}
	
	if (tmp == PM_NAZGUL){
			if(mvitals[tmp].born > 0) mvitals[tmp].born--;
			if(mvitals[tmp].mvflags&G_EXTINCT){
				mvitals[tmp].mvflags &= (~G_EXTINCT);
				reset_rndmonst(tmp);
			}
	}
	if(tmp == PM_ARA_KAMEREL && !has_template(mtmp, FRACTURED)){
		ara_died(mtmp);
	}
	if(tmp == PM_FATHER_DAGON){
		u.uevent.ukilled_dagon = 1;
	}
	if(tmp == PM_MOTHER_HYDRA){
		u.uevent.ukilled_hydra = 1;
	}
	/* if it's a (possibly polymorphed) quest leader, mark him as dead */
	if (mtmp->m_id == quest_status.leader_m_id)
		quest_status.leader_is_dead = TRUE;
#ifdef MAIL
	/* if the mail daemon dies, no more mail delivery.  -3. */
	if (tmp == PM_MAIL_DAEMON) mvitals[tmp].mvflags |= G_GENOD;
#endif

	if (mtmp->data->mlet == S_KETER) {
		/* Dead Keter may come back. */
		switch(rnd(5)) {
		case 1:	     /* returns near the stairs */
			(void) makemon(mtmp->data,xdnstair,ydnstair,NO_MM_FLAGS);
			break;
		case 2:	     /* returns near upstair */
			(void) makemon(mtmp->data,xupstair,yupstair,NO_MM_FLAGS);
			break;
		default:
			break;
		}
	}
	if(mtmp->iswiz) wizdead();
	if(mtmp->data->msound == MS_NEMESIS) nemdead();        
	//Asc items and crucial bookkeeping
	if(Race_if(PM_DROW) && !Role_if(PM_NOBLEMAN) && mtmp->mtyp == urole.neminum && !flags.made_bell){
		(void) mksobj_at(BELL_OF_OPENING, mtmp->mx, mtmp->my, NO_MKOBJ_FLAGS);
		flags.made_bell = TRUE;
	}
	if(mtmp->mtyp == PM_OONA){
		struct obj *obj;
		obj = mksobj_at(SKELETON_KEY, mtmp->mx, mtmp->my, MKOBJ_NOINIT);
		obj = oname(obj, artiname(ART_THIRD_KEY_OF_LAW));
		obj->spe = 0;
		obj->cursed = obj->blessed = FALSE;
	}
	if(mtmp->mtyp == PM_ASPECT_OF_THE_SILENCE){
		int remaining = 0;
		if(!flags.made_first)
			remaining++;
		if(!flags.made_divide)
			remaining++;
		if(!flags.made_life)
			remaining++;
		if(remaining){
			remaining = rnd(remaining);
			if(!flags.made_first && !(--remaining))
				mksobj_at(FIRST_WORD, mtmp->mx, mtmp->my, NO_MKOBJ_FLAGS);
			else if(!flags.made_divide && !(--remaining))
				mksobj_at(DIVIDING_WORD, mtmp->mx, mtmp->my, NO_MKOBJ_FLAGS);
			else if(!flags.made_life && !(--remaining))
				mksobj_at(NURTURING_WORD, mtmp->mx, mtmp->my, NO_MKOBJ_FLAGS);
		}
	}
	if(mtmp->mtyp == PM_GARLAND){
		int x = mtmp->mx, y = mtmp->my;
		struct obj *otmp;
		makemon(&mons[PM_CHAOS], mtmp->mx, mtmp->my, MM_ADJACENTOK);
	}
	if(mtmp->mtyp == PM_ILLURIEN_OF_THE_MYRIAD_GLIMPSES && !(u.uevent.ukilled_illurien)){
		u.uevent.ukilled_illurien = 1;
		u.ill_cnt = rn1(1000, 250);
	}
	if(mtmp->mtyp == PM_ORCUS){
		struct engr *oep = engr_at(mtmp->mx,mtmp->my);
		if(!oep){
			make_engr_at(mtmp->mx, mtmp->my,
			 "", 0L, DUST);
			oep = engr_at(mtmp->mx,mtmp->my);
		}
		oep->ward_id = TENEBROUS;
		oep->halu_ward = 0;
		oep->ward_type = BURN;
		oep->complete_wards = 1;
	}
	if(mtmp->mtyp == PM_CHOKHMAH_SEPHIRAH){
		change_chokhmah(1);
		change_keter(1);
	}
	if(u.ublood_smithing && (mtmp->mtyp == PM_BLASPHEMOUS_LURKER
		|| mtmp->mtyp == PM_BAALPHEGOR
		|| mtmp->mtyp == PM_ASMODEUS
		|| mtmp->mtyp == PM_PALE_NIGHT
		|| mtmp->mtyp == PM_GOOD_NEIGHBOR
		|| mtmp->mtyp == PM_HMNYW_PHARAOH
		|| mtmp->mtyp == PM_MOON_S_CHOSEN
	)){
		struct obj *otmp = mksobj_at(CRYSTAL, mtmp->mx, mtmp->my, MKOBJ_NOINIT);
		if(otmp){
			set_material_gm(otmp, HEMARGYOS);
			otmp->spe = 4;
		}
	}
	//Livelogs
	if (mtmp->data->mlet == S_VAMPIRE && mtmp->data->geno & G_UNIQ && mtmp->mtyp != PM_VLAD_THE_IMPALER)
		/* don't livelog Vlad's wives, too spammy */;
	else if(mtmp->mtyp == PM_CHOKHMAH_SEPHIRAH && mvitals[PM_CHOKHMAH_SEPHIRAH].died == 1)
		livelog_write_string("destroyed a chokhmah sephirah");	/* not unique but worth logging the first */
	else if (mtmp->mtyp == PM_CHAOS && mvitals[PM_CHAOS].died == 1 && Hallucination)
		livelog_write_string("perpetuated an asinine paradigm"); /* YAF-livelog if hallucinating */
	else if(mtmp->data->geno & G_UNIQ && mvitals[mtmp->mtyp].died == 1){
		char buf[BUFSZ];
		buf[0]='\0';
		if(nonliving(mtmp->data)) Sprintf(buf,"destroyed %s",noit_nohalu_mon_nam(mtmp));
		else Sprintf(buf,"killed %s",noit_nohalu_mon_nam(mtmp));
		livelog_write_string(buf);
	}
	//Remove linked hungry dead
	if(mtmp->mtyp == PM_BLOB_OF_PRESERVED_ORGANS){
		struct monst *mon, *mtmp2;
		for (mon = fmon; mon; mon = mtmp2){
			mtmp2 = mon->nmon;
			if(mon->mtyp == PM_HUNGRY_DEAD){
				if(mtmp->mvar_huskID == (long)mon->m_id){
					if(mon->mhp > 0){
						mon->mhp = 0;
						mondied(mon);
					}
					break;
				}
			}
		}
		if(!mon){
			struct monst **mmtmp;
			mmtmp = &migrating_mons;
			for (mon = migrating_mons; mon; mon = mtmp2){
				mtmp2 = mon->nmon;
				if(mon->mtyp == PM_HUNGRY_DEAD){
					if(mtmp->mvar_huskID == (long)mon->m_id){
						*mmtmp = mon->nmon;
						mon_arrive(mon, TRUE);
						if(mon->mhp > 0){
							mon->mhp = 0;
							mondied(mon);
						}
						break;
					}
				}
				mmtmp = &(mon->nmon);
			}
		}
	}
	//Remove linked tentacles
	if(mtmp->mtyp == PM_WATCHER_IN_THE_WATER || mtmp->mtyp == PM_KETO){
		struct monst *mon, *mtmp2;
		for (mon = fmon; mon; mon = mtmp2){
			mtmp2 = mon->nmon;
			if(mon->mtyp == PM_SWARM_OF_SNAKING_TENTACLES || mon->mtyp == PM_LONG_SINUOUS_TENTACLE || mon->mtyp == PM_WIDE_CLUBBED_TENTACLE){
				if (DEADMONSTER(mon)) continue;
				mon->mhp = -10;
				monkilled(mon,"",AD_DRLI);
			}
		}
	}
	
	//Quest flavor
	if(Role_if(PM_ANACHRONONAUT) && (mtmp->mpeaceful || (has_lifesigns(mtmp) && mtmp->mvar_lifesigns)) && In_quest(&u.uz) && Is_qstart(&u.uz) && !(quest_status.leader_is_dead)){
		if(mtmp->mtyp == PM_TROOPER || (has_lifesigns(mtmp) && mtmp->mvar_lifesigns == PM_TROOPER)){
			verbalize("**ALERT: trooper %d vital signs terminated**", (int)(mtmp->m_id));
		} else if(mtmp->mtyp == PM_MYRKALFAR_WARRIOR){
			verbalize("**ALERT: warrior %d vital signs terminated**", (int)(mtmp->m_id));
		} else {
			verbalize("**ALERT: citizen %d vital signs terminated**", (int)(mtmp->m_id));
		}
	}
	if(mtmp->mtame && roll_madness(MAD_TALONS)){
		You("panic after losing a pet!");
		HPanicking += 1+rnd(6);
	}
	if(mtmp->mtyp == PM_ASMODEUS){
		u.umadness |= MAD_OVERLORD;
	}
	if(mtmp->mtyp == PM_BLASPHEMOUS_LURKER){
		u.umadness |= MAD_REACHER;
	}
	if(mtmp->mtyp == PM_PUPPET_EMPEROR_XELETH || mtmp->mtyp == PM_PUPPET_EMPRESS_XEDALLI){
		makemon(&mons[PM_SUZERAIN], 0, 0, MM_ADJACENTOK);
	}
#ifdef RECORD_ACHIEVE
	if(mtmp->mtyp == PM_LUCIFER){
		achieve.killed_lucifer = 1;
	}
	else if(mtmp->mtyp == PM_ASMODEUS){
		achieve.killed_asmodeus = 1;
	}
	else if(mtmp->mtyp == PM_DEMOGORGON){
		achieve.killed_demogorgon = 1;
	}
	else if(mtmp->mtyp == PM_LAMASHTU){
		give_lamashtu_trophy();
	}
	else if(mtmp->mtyp == PM_BAALPHEGOR){
		give_baalphegor_trophy();
	}
	else if (mtmp->mtyp == PM_MEDUSA
	|| mtmp->mtyp == PM_GRUE
	|| mtmp->mtyp == PM_ECHO
	|| mtmp->mtyp == PM_SYNAISTHESIA
	) {
		achieve.killed_challenge = 1;
	}
	if(mtmp->mtyp == PM_ARSENAL
	|| mtmp->mtyp == PM_OONA
	|| mtmp->mtyp == PM_PLATINUM_DRAGON
	)
		give_law_trophy();
	if(mtmp->mtyp == PM_CHAOS
	|| mtmp->mtyp == PM_LICH__THE_FIEND_OF_EARTH
	|| mtmp->mtyp == PM_KARY__THE_FIEND_OF_FIRE
	|| mtmp->mtyp == PM_KRAKEN__THE_FIEND_OF_WATER
	|| mtmp->mtyp == PM_TIAMAT__THE_FIEND_OF_WIND
	){
		if(mvitals[PM_CHAOS].died
		&& mvitals[PM_LICH__THE_FIEND_OF_EARTH].died
		&& mvitals[PM_KARY__THE_FIEND_OF_FIRE].died
		&& mvitals[PM_KRAKEN__THE_FIEND_OF_WATER].died
		&& mvitals[PM_TIAMAT__THE_FIEND_OF_WIND].died
		)
			give_chaos_temple_trophy();
	}
	if(mtmp->mtyp == PM_GOOD_NEIGHBOR
	|| mtmp->mtyp == PM_HMNYW_PHARAOH
	|| mtmp->mtyp == PM_NITOCRIS
	|| mtmp->mtyp == PM_GHOUL_QUEEN_NITOCRIS
	|| mtmp->mtyp == PM_BLASPHEMOUS_LURKER
	|| mtmp->mtyp == PM_BLESSED
	|| mtmp->mtyp == PM_MOUTH_OF_THE_GOAT
	|| mtmp->mtyp == PM_APHANACTONAN_ASSESSOR
	){
		if(mvitals[PM_GOOD_NEIGHBOR].died
		&& mvitals[PM_HMNYW_PHARAOH].died
		&& (mvitals[PM_NITOCRIS].died || mvitals[PM_GHOUL_QUEEN_NITOCRIS].died)
		&& mvitals[PM_BLASPHEMOUS_LURKER].died
		&& mvitals[PM_BLESSED].died
		&& mvitals[PM_MOUTH_OF_THE_GOAT].died
		&& mvitals[PM_APHANACTONAN_ASSESSOR].died
		)
			give_nightmare_hunter_trophy();
	}
#endif
	if(glyph_is_invisible(levl[mtmp->mx][mtmp->my].glyph))
		unmap_object(mtmp->mx, mtmp->my);
	m_detach(mtmp, mptr);
}

STATIC_DCL int
mon_expl_color(mdat, adtyp)
struct permonst *mdat;
int adtyp;
{
	switch(mdat->mtyp){
		case PM_GAS_SPORE:
		case PM_DUNGEON_FERN_SPORE:
		case PM_APHANACTONAN_AUDIENT:
			return EXPL_NOXIOUS;
		case PM_SPHERE_OF_FORCE:
			return EXPL_GRAY;
		case PM_SWAMP_FERN_SPORE:
			return EXPL_MAGICAL;
		case PM_BURNING_FERN_SPORE:
			return EXPL_FIERY;
		case PM_ANCIENT_OF_THE_BURNING_WASTES:
			return EXPL_YELLOW;
		case PM_FABERGE_SPHERE:
			return rn2(7);
	}
	return adtyp_expl_color(adtyp);
}

/* TRUE if corpse might be dropped, magr may die if mon was swallowed */
boolean
corpse_chance(mon, magr, was_swallowed)
struct monst *mon;
struct monst *magr;			/* killer, if swallowed */
boolean was_swallowed;			/* digestion */
{
	struct permonst *mdat = mon->data;
	int i, tmp;
	//Must be done here for reasons that are obscure
	if(Role_if(PM_ANACHRONONAUT) && (mon->mpeaceful || (has_lifesigns(mon) && mon->mvar_lifesigns)) && In_quest(&u.uz) && Is_qstart(&u.uz)){
		if(!cansee(mon->mx,mon->my)) map_invisible(mon->mx, mon->my);
	}
	/* bypass anything about templates, etc. just always make a corpse*/
	if (is_rider(mdat)) return TRUE;

	/* Liches and Vlad and his wives have a fancy death message, and leave no corpse */
	if ((mdat->mlet == S_LICH) ||
		(mdat->mlet == S_VAMPIRE && mdat->geno & G_UNIQ)) {
		if (cansee(mon->mx, mon->my) && !was_swallowed)
			pline("%s body crumbles into dust.", s_suffix(Monnam(mon)));
	    
		if(mdat->mtyp != PM_VECNA) return FALSE; /* exception for Vecna, who leaves his hand or eye*/
	}
	else if(Infuture && is_myrkalfr(mdat) && !get_template(mon))
		return FALSE;
	else if(mon->mibitemarked)
		return FALSE;
	else if(mdat->mtyp == PM_ECLAVDRA)
		return FALSE;
	else if(mdat->mtyp == PM_CHOKHMAH_SEPHIRAH)
		return FALSE;
	
	if(uwep && uwep->oartifact == ART_PEN_OF_THE_VOID && uwep->ovara_seals&SEAL_MALPHAS && rn2(20) <= (mvitals[PM_ACERERAK].died > 0 ? 4 : 1)){
		struct monst *mtmp;
		mtmp = makemon(&mons[PM_CROW], u.ux, u.uy, MM_EDOG|MM_ADJACENTOK);
		initedog(mtmp);
		mtmp->m_lev += uwep->spe;
		mtmp->mhpmax = (mtmp->m_lev * 8) - 4;
		mtmp->mhp =  mtmp->mhpmax;
	}
	
	if(!get_mx(mon, MX_ESUM) && roll_madness(MAD_THOUSAND_MASKS) && !(mon->data->geno&G_UNIQ) && !(uarmc && uarmc->oartifact == ART_SPELL_WARDED_WRAPPINGS_OF_)){
		struct monst *maskmon = makemon(mkclass(S_UMBER, G_NOHELL|G_HELL), mon->mx, mon->my, MM_ADJACENTOK|MM_NOCOUNTBIRTH|MM_ESUM);
		if(maskmon){
			struct obj *mask;
			mask = mksobj(MASK, MKOBJ_NOINIT);
			mask->corpsenm = mon->mtyp;
			doMaskStats(mask);
			if(canseemon(maskmon))
				pline("%s takes off %s %s!", The(mon_nam(maskmon)), mhis(maskmon), xname(mask));
			if(maskmon->mpeaceful){
				maskmon->mpeaceful = 0;
				set_malign(maskmon);
				newsym(maskmon->mx, maskmon->my);
			}
			mark_mon_as_summoned(maskmon, (struct monst *)0, max(Insanity, 10)/5, 0);//2 to 20
			//Don't hand the monster the mask untill AFTER the summoning is all initialized, so that the mask gets left.
			mpickobj(maskmon, mask);
			maskmon->mspec_used = 0;
			if(youmonst.movement >= 24)
				maskmon->movement = 12;
		}
	}
	/* On-death explosions and other effects */
	for(i = 0; i < NATTK; i++) {
		int aatyp = mdat->mattk[i].aatyp;
		int adtyp = mdat->mattk[i].adtyp;
		if(aatyp == AT_NONE &&  adtyp == AD_OONA){
			aatyp = AT_BOOM;
			adtyp = u.oonaenergy;
		}
		if (aatyp == AT_BOOM  &&  adtyp != AD_HLBD && adtyp != AD_POSN) {
			if (mdat->mattk[i].damn)
				tmp = d((int)mdat->mattk[i].damn,
						(int)mdat->mattk[i].damd);
	    	else if(mdat->mattk[i].damd)
	    	    tmp = d((int)mdat->mlevel+1, (int)mdat->mattk[i].damd);
	    	else tmp = 0;
			if (was_swallowed && magr) {
				if (magr == &youmonst) {
					There("is an explosion in your %s!",
						  body_part(STOMACH));
					Sprintf(killer_buf, "%s explosion",
						s_suffix(mdat->mname));
					tmp = reduce_dmg(&youmonst,tmp,TRUE,FALSE);
					losehp(tmp, killer_buf, KILLED_BY_AN);
				} 
				else {
					if (flags.soundok) You_hear("an explosion.");
					magr->mhp -= tmp;
					if (magr->mhp < 1) mondied(magr);
					if (magr->mhp < 1) { /* maybe lifesaved */
						if (canspotmon(magr))
						pline("%s rips open!", Monnam(magr));
					} 
					else if (canseemon(magr)) pline("%s seems to have indigestion.", Monnam(magr));
				}
				return FALSE;
			}
			
	    	Sprintf(killer_buf, "%s explosion", s_suffix(mdat->mname));
	    	killer = killer_buf;
	    	killer_format = KILLED_BY_AN;
			if(adtyp == AD_JAILER){
				explode_pa(mon->mx, mon->my, AD_FIRE, MON_EXPLODE, tmp, EXPL_FIERY, 1, mdat);
				u.uevent.ukilled_apollyon = 1;
			}
			else if(mdat->mtyp == PM_ANCIENT_OF_DEATH){
				if(!(u.sealsActive&SEAL_OSE)) explode_pa(mon->mx, mon->my, adtyp, MON_EXPLODE, tmp, EXPL_DARK, 1, mdat);
			}
			else if(adtyp == AD_GARO){
				if(couldsee(mon->mx, mon->my)){
					pline("\"R-regrettable... Although my rival, you were spectacular.");
					pline("I shall take my bow by opening my heart and revealing my wisdom...");
					outrumor(rn2(2), BY_OTHER); //either true (3/4) or false (1/4), no mechanism specified.
					pline("Belief or disbelief rests with you.");
					pline("To die without leaving a corpse....\"");
					explode_pa(mon->mx, mon->my, AD_PHYS, MON_EXPLODE, tmp, EXPL_MUDDY, 1, mdat);
					pline("\"That is the way of us Garo.\"");
				} else {
					explode_pa(mon->mx, mon->my, AD_PHYS, MON_EXPLODE, tmp, EXPL_MUDDY, 1, mdat);
				}
			}
			else if(adtyp == AD_GARO_MASTER){
				if(couldsee(mon->mx, mon->my)){
					pline("\"To think thou couldst defeat me...");
					pline("Though my rival, thou were't spectacular.");
					pline("I shall take my bow by opening my heart and revealing my wisdom...");
					outgmaster(); //Gives out a major consultation. Does not set the consultation flags.
					pline("Do not forget these words...");
					pline("Die I shall, leaving no corpse.\"");
					explode_pa(mon->mx, mon->my, AD_PHYS, MON_EXPLODE, tmp, EXPL_MUDDY, 1, mdat);
					pline("\"That is the law of us Garo.\"");
				} else {
					explode_pa(mon->mx, mon->my, AD_PHYS, MON_EXPLODE, tmp, EXPL_MUDDY, 1, mdat);
				}
			}
			else if(adtyp == AD_FRWK){
				int x, y, i;
				for(i = rn2(3)+2; i > 0; i--){
					x = mon->mx+rn2(7)-3;
					y = mon->my+rn2(7)-3;
					if(!isok(x,y)){
						x = mon->mx;
						y = mon->my;
					}
					explode(x, y, AD_PHYS, -1, tmp, rn2(7), 1);
				}
				tmp=0;
			}
			else if(adtyp == AD_SPNL){
				struct monst *levi;
				explode_pa(mon->mx, mon->my, AD_COLD, MON_EXPLODE, tmp, EXPL_WET, 1, mdat);
				levi = makemon(&mons[rn2(2) ? PM_LEVISTUS : PM_LEVIATHAN], mon->mx, mon->my, MM_ADJACENTOK);
				if(levi)
					levi_spawn_items(mon->mx, mon->my, levi);
			}
			else if(adtyp == AD_MAND){
				struct monst *mtmp, *mtmp2;
				if(mon->mcan){
					char buf[BUFSZ];
					Sprintf(buf, "%s croaks out a hoarse shriek.", Monnam(mon)); //Monnam and mon_nam share a buffer and can't be used on the same line.
					pline("%s  It seems %s has a sore throat!", buf, mon_nam(mon));
					return FALSE;
				}
				else pline("%s lets out a terrible shriek!", Monnam(mon));
				for (mtmp = fmon; mtmp; mtmp = mtmp2){
					mtmp2 = mtmp->nmon;
					if(mtmp->data->geno & G_GENO && !nonliving(mon->data) && !is_demon(mon->data) && !is_keter(mon->data) && mtmp->mhp <= 100){
						if (DEADMONSTER(mtmp)) continue;
						mtmp->mhp = -10;
						monkilled(mtmp,"",AD_DRLI);
					}
				}
				/* And a finger of death type attack on you */
				if (nonliving(youracedata) || is_demon(youracedata)) {
					You("seem no deader than before.");
				} else if ((Upolyd ? u.mh : u.uhp) <= 100 && !(u.sealsActive&SEAL_OSE)) {
					if (Hallucination) {
					You("have an out of body experience.");
					} else {
					killer_format = KILLED_BY_AN;
					killer = "mandrake's dying shriek";
					if (!u.uconduct.killer){
						//Pcifist PCs aren't combatants so if something kills them up "killed peaceful" type impurities
						IMPURITY_UP(u.uimp_murder)
						IMPURITY_UP(u.uimp_bloodlust)
					}
					done(DIED);
					}
				} else shieldeff(u.ux,u.uy);
			}
			else {
				explode_pa(mon->mx, mon->my, 
						adtyp, 
						MON_EXPLODE, 
						tmp, 
						mon_expl_color(mdat, adtyp), 
						1,
						mdat);
			}
	    	if(mdat->mtyp == PM_GARO_MASTER
				|| mdat->mtyp == PM_GARO
				|| mdat->mtyp == PM_APHANACTONAN_AUDIENT
			) return (TRUE);
			else return (FALSE);
	    } //End AT_BOOM != AD_HLBD && != AD_POSN
		else if(adtyp == AD_HLBD && mdat->mtyp == PM_ASMODEUS){
			int i;
			for(i=0; i<2; i++) makemon(&mons[PM_NESSIAN_PIT_FIEND], mon->mx, mon->my, MM_ADJACENTOK);
			for(i=0; i<7; i++) makemon(&mons[PM_PIT_FIEND], mon->mx, mon->my, MM_ADJACENTOK);
			for(i = 0; i<12; i++) makemon(&mons[PM_BARBED_DEVIL], mon->mx, mon->my, MM_ADJACENTOK);
			for(i = 0; i<20; i++) makemon(&mons[PM_HORNED_DEVIL], mon->mx, mon->my, MM_ADJACENTOK);
			for(i = 0; i<30; i++) makemon(&mons[PM_LEMURE], mon->mx, mon->my, MM_ADJACENTOK);
	    	return (FALSE);
		}
		else if(adtyp == AD_HLBD && mdat->mtyp == PM_VERIER){
			int i;
			for(i=0; i<9; i++) makemon(&mons[PM_PIT_FIEND], mon->mx, mon->my, MM_ADJACENTOK);
			for(i = 0; i<12; i++) makemon(&mons[PM_BARBED_DEVIL], mon->mx, mon->my, MM_ADJACENTOK);
			for(i = 0; i<18; i++) makemon(&mons[PM_HORNED_DEVIL], mon->mx, mon->my, MM_ADJACENTOK);
			for(i = 0; i<30; i++) makemon(&mons[PM_LEMURE], mon->mx, mon->my, MM_ADJACENTOK);
	    	return (FALSE);
		}
		else if(adtyp == AD_OMUD){
			int i;
			for(i=0; i<39; i++) incarnator_spawn(mon->mx, mon->my, TRUE);
		}
  		else if(	( (aatyp == AT_NONE && mdat->mtyp==PM_GREAT_CTHULHU)
					 || aatyp == AT_BOOM) 
				&& adtyp == AD_POSN
		){
	    	Sprintf(killer_buf, "%s explosion", s_suffix(mdat->mname));
	    	killer = killer_buf;
	    	killer_format = KILLED_BY_AN;
			explode_pa(mon->mx, mon->my, AD_PHYS, MON_EXPLODE, d(8,8), EXPL_NOXIOUS, 1, mdat);
			if(mdat->mtyp==PM_GREAT_CTHULHU){
				create_gas_cloud(mon->mx, mon->my, 2, 30, FALSE);
			}
		}
		else if(adtyp == AD_GROW){
			struct monst *mtmp;
			struct monst * axus = (struct monst *)0;
			boolean found;
			int current_ton;
			/* ASSUMES AUTONS ARE IN ORDER FROM MONOTON TO QUINON */
			for (current_ton = mon->mtyp == PM_AXUS ? PM_QUINON : mon->mtyp; current_ton >= PM_MONOTON; current_ton--) {
				found = FALSE; //haven't found this child yet - 1 per level
				/* search for child */
				for (mtmp = fmon; mtmp && (!found || !axus); mtmp = mtmp->nmon) {
					if (DEADMONSTER(mtmp))
						continue;
					if (!axus && !DEADMONSTER(mtmp) && mtmp->mtyp == PM_AXUS)
						axus = mtmp;
					if (!found && current_ton != PM_MONOTON && mtmp->mtyp == current_ton-1) {
						set_mon_data(mtmp, current_ton);
						//Assumes Auton
						mtmp->m_lev += 1;
						mtmp->mhp += 4;
						mtmp->mhpmax += 4;
						newsym(mtmp->mx, mtmp->my);
						found = TRUE;
					}
				}
				/* if Axus is on the level, generate a 'ton beside him */
				/* monoton if we found someone to grow */
				/* the full 'ton if we didn't find one */
				if (axus && (!found || current_ton == PM_MONOTON)) {
					mtmp = makemon(&mons[current_ton], axus->mx, axus->my, MM_ADJACENTOK | MM_ANGRY | NO_MINVENT);
					if (mtmp) mtmp->mclone = 1;
				}
				/* growth is chained -- if we didn't find a child, we don't touch the lower 'tons. */
				/* 	Axus is able to pull from anywhere, so its chain continues. */
				if (!found && mdat->mtyp != PM_AXUS)
					break;
			}
			if(mdat->mtyp == PM_AXUS){
				u.uevent.uaxus_foe = 1;//enemy of the modrons
				for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
						if (is_auton(mtmp->data) && mtmp->mpeaceful && mtmp != mon) {
							if(canseemon(mtmp)) pline("%s gets angry...", Monnam(mtmp));
							untame(mtmp, 0);
						}
				}
			}
		}//end AD_GROW basic
		else if(adtyp == AD_SOUL){
			struct monst *mtmp;
			struct permonst *mdat1;
			int hpgain = 0;
			int lvlgain = 0;
			int lvls = 0;
			if(mdat->mtyp==PM_DEEP_ONE){
				hpgain = 2;
			} else if(mdat->mtyp==PM_DEEPER_ONE){
				hpgain = 4;
			} else if(mdat->mtyp==PM_DEEPEST_ONE || mdat->mtyp==PM_FATHER_DAGON || mdat->mtyp==PM_MOTHER_HYDRA){
				hpgain = 8;
			} else { //arcadian avenger
				lvlgain = 1;
			}
			if(mdat->mtyp==PM_DEEP_ONE || mdat->mtyp==PM_DEEPER_ONE || mdat->mtyp==PM_DEEPEST_ONE
			 || mdat->mtyp==PM_FATHER_DAGON || mdat->mtyp==PM_MOTHER_HYDRA
			){
				for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
					if(DEADMONSTER(mtmp))
						continue;
					mdat1 = mtmp->data;
					if( mdat1->mtyp==PM_DEEP_ONE
						|| mdat1->mtyp==PM_DEEPER_ONE
						|| mdat1->mtyp==PM_DEEPEST_ONE
						|| mdat1->mtyp==PM_FATHER_DAGON
						|| mdat1->mtyp==PM_MOTHER_HYDRA
					){
						if(lvlgain) for(lvls = lvlgain; lvls > 0; lvls--){
							grow_up(mtmp, 0);
							//Grow up may have killed mtmp
							if(DEADMONSTER(mtmp))
								continue;
						}
						if(hpgain){
							if (mtmp->mhpmax < 300){
								mtmp->mhpmax += hpgain-1;
								mtmp->mhp += hpgain-1;
								grow_up(mtmp, mtmp); //gain last HP and grow up if needed
							}
							else {
								mtmp->mhpmax += 1;
								mtmp->mhp += 1;
								grow_up(mtmp, mtmp); //gain last HP and grow up if needed
							}
						}
					}
				}
			} else { //arcadian avenger
				for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
					if(DEADMONSTER(mtmp))
						continue;
					mdat1 = mtmp->data;
					if( mdat1==mdat ){
						if(lvlgain) for(lvls = lvlgain; lvls > 0; lvls--){
							grow_up(mtmp, 0);
							//Grow up may have killed mtmp
							if(DEADMONSTER(mtmp))
								continue;
						}
						if(hpgain){
							mtmp->mhpmax += hpgain-1;
							mtmp->mhp += hpgain-1;
							grow_up(mtmp, mtmp); //gain last HP and grow up if needed
						}
					}
				}
			}
		}//end of AD_SOUL
	} //end of for loop that views attacks.
	/* must duplicate this below check in xkilled() since it results in
	 * creating no objects as well as no corpse
	 */
	if (has_template(mon, SKELIFIED) || has_template(mon, SLIME_REMNANT))
		return FALSE;

	if (has_template(mon, CRYSTALFIED))
		return TRUE;

	if (has_template(mon, TOMB_HERD))
		return TRUE;

	if (has_template(mon, POISON_TEMPLATE))
		return TRUE;

	if (is_golem(mdat)
		   || is_mplayer(mdat)
		   || is_rider(mdat)
		   || mon->m_id == quest_status.leader_m_id
		   || mon->data->msound == MS_NEMESIS
		   || (mdat->geno & G_UNIQ)
		   || is_alabaster_mummy(mon->data)
		   || (uwep && uwep->oartifact == ART_SINGING_SWORD && uwep->osinging == OSING_LIFE && mon->mtame)
		   || mdat->mtyp == PM_APHANACTONAN_AUDIENT
		   || mdat->mtyp == PM_APHANACTONAN_ASSESSOR
		   || mdat->mtyp == PM_HARROWER_OF_ZARIEL
		   || mdat->mtyp == PM_UNDEAD_KNIGHT
		   || mdat->mtyp == PM_WARRIOR_OF_SUNLIGHT
		   || mdat->mtyp == PM_UNDEAD_MAIDEN
		   || mdat->mtyp == PM_KNIGHT_OF_THE_PRINCESS_S_GUARD
		   || mdat->mtyp == PM_BLUE_SENTINEL
		   || mdat->mtyp == PM_DARKMOON_KNIGHT
		   || mdat->mtyp == PM_UNDEAD_REBEL
		   || mdat->mtyp == PM_PARDONER
		   || mdat->mtyp == PM_OCCULTIST
		   || mdat->mtyp == PM_CROW_WINGED_HALF_DRAGON
		   || mdat->mtyp == PM_SEYLL_AUZKOVYN
		   || mdat->mtyp == PM_DARUTH_XAXOX
		   || mdat->mtyp == PM_ORION
		   || mdat->mtyp == PM_VECNA
//		   || mdat->mtyp == PM_UNICORN_OF_AMBER
		   || mdat->mtyp == PM_NIGHTMARE
		   || mdat->mtyp == PM_CHROMATIC_DRAGON
		   || mdat->mtyp == PM_PLATINUM_DRAGON
		   || mdat->mtyp == PM_CHANGED
		   || mdat->mtyp == PM_WARRIOR_CHANGED
		   || mdat->mtyp == PM_TWITCHING_FOUR_ARMED_CHANGED
		   || mdat->mtyp == PM_CLAIRVOYANT_CHANGED
		   || mdat->mtyp == PM_NITOCRIS
		   || mdat->mtyp == PM_GHOUL_QUEEN_NITOCRIS
		   || mdat->mtyp == PM_ANDROID
		   || mdat->mtyp == PM_GYNOID
		   || mdat->mtyp == PM_OPERATOR
		   || mdat->mtyp == PM_COMMANDER
		   || mdat->mtyp == PM_MUMMIFIED_ANDROID
		   || mdat->mtyp == PM_MUMMIFIED_GYNOID
		   || mdat->mtyp == PM_FLAYED_ANDROID
		   || mdat->mtyp == PM_FLAYED_GYNOID
		   || mdat->mtyp == PM_PARASITIZED_EMBRACED_ALIDER
		   || mdat->mtyp == PM_PARASITIZED_ANDROID
		   || mdat->mtyp == PM_PARASITIZED_GYNOID
		   || mdat->mtyp == PM_PARASITIZED_OPERATOR
		   || mdat->mtyp == PM_PARASITIZED_COMMANDER
		   || mdat->mtyp == PM_CRUCIFIED_ANDROID
		   || mdat->mtyp == PM_CRUCIFIED_GYNOID
		   || mdat->mtyp == PM_ALABASTER_CACTOID
//		   || mdat->mtyp == PM_PINK_UNICORN
		   )
		return TRUE;
		
	if (LEVEL_SPECIFIC_NOCORPSE(mdat))
		return FALSE;

	if(bigmonst(mdat) || mdat->mtyp == PM_LIZARD) return TRUE;
	
	tmp = (int)(2 + ((int)(mdat->geno & G_FREQ)<2) + verysmall(mdat));
	return (u.sealsActive&SEAL_EVE) ? (!rn2(tmp)||!rn2(tmp)) : 
		  (uwep && uwep->oartifact == ART_PEN_OF_THE_VOID && uwep->ovara_seals&SEAL_EVE) ?  (!rn2(tmp)||!rn2(2*tmp - 1)) :
		  !rn2(tmp);
}


void
spore_dies(mon)
struct monst *mon;
{
	if (mon->mhp > 0)	/* just in case */
		return;
		
	int ferntype = NON_PM;
	coord mm;
	mm.x = mon->mx; mm.y = mon->my;
	
	/* based on spore type and terrain, select fern to make */
	/* note: generic fern spores, if implemented, would choose a fern based on terrain */
	/* all currently written fern spores have a 2/3 chance of not making a fern even on favoured terrain */
	if (mon->mtyp == PM_DUNGEON_FERN_SPORE) {
	/* dungeon ferns cannot reproduce on ice, lava, or water; swamp is okay */
		if (!is_ice(mm.x, mm.y) && !is_lava(mm.x, mm.y) && !is_pool(mm.x, mm.y, FALSE))
			ferntype = !rn2(3) ? PM_DUNGEON_FERN : NON_PM;
	} else if (mon->mtyp == PM_SWAMP_FERN_SPORE) {
		if (!is_ice(mm.x, mm.y) && !is_lava(mm.x, mm.y))
			ferntype = !rn2(3) ? PM_SWAMP_FERN : NON_PM;
	} else if (mon->mtyp == PM_BURNING_FERN_SPORE) {
		if (!is_ice(mm.x, mm.y) && !is_pool(mm.x, mm.y, TRUE))
			ferntype = !rn2(3) ? PM_BURNING_FERN : NON_PM;
	}
	else {
		impossible("Unhandled spore type in spore_dies, %d", mon->mtyp);
		ferntype = NON_PM;
	}

	/* create a gas cloud */
	create_gas_cloud(mm.x, mm.y, rn1(2,1), rnd(8), FALSE);
	
	/*Note: the original summoner might already be dead by the time the fern grows, so what to do with summoned ferns can be unclear*/
	if (ferntype != NON_PM && !get_mx(mon, MX_ESUM)) 
	{
		struct monst * mtmp;
		/* when creating a new fern, 5/6 chance of creating a fern sprout and 1/6 chance of a fully-grown one */
		if (rn2(6))
			ferntype = big_to_little(ferntype);

		mtmp = makemon(&mons[ferntype], mm.x, mm.y, NO_MM_FLAGS);
	}
}

/* drop (perhaps) a cadaver and remove monster */
void
mondied(mdef)
register struct monst *mdef;
{
	mondead(mdef);
	if (mdef->mhp > 0) return;	/* lifesaved */
	/* we did not lifesave */
	mdef->deadmonster |= DEADMONSTER_DEAD;

	if (corpse_chance(mdef, (struct monst *)0, FALSE) &&
	    (accessible(mdef->mx, mdef->my) || is_pool(mdef->mx, mdef->my, FALSE)))
		(void) make_corpse(mdef);
}

/* monster disappears, not dies */
void
mongone(mdef)
register struct monst *mdef;
{
	mdef->mhp = 0;	/* can skip some inventory bookkeeping */
	mdef->deadmonster |= DEADMONSTER_DEAD;

#ifdef STEED
	/* Player is thrown from his steed when it disappears */
	if (mdef == u.usteed)
		dismount_steed(DISMOUNT_GENERIC);
#endif
	/* cease occupation if the monster was associated */
	if(mdef->moccupation) stop_occupation();
	
	
	/* drop special items like the Amulet so that a dismissed Keter or nurse
	   can't remove them from the game */
	mdrop_special_objs(mdef);
	/* release rest of monster's inventory--it is removed from game */
	discard_minvent(mdef);
#ifndef GOLDOBJ
	mdef->mgold = 0L;
#endif
	m_detach(mdef, mdef->data);
}

/* monster vanishes, not dies, leaving inventory */
void
monvanished(mdef)
register struct monst *mdef;
{
	mdef->mhp = 0;	/* can skip some inventory bookkeeping */
	mdef->deadmonster |= DEADMONSTER_DEAD;

#ifdef STEED
	/* Player is thrown from his steed when it disappears */
	if (mdef == u.usteed)
		dismount_steed(DISMOUNT_GENERIC);
#endif

	/* cease occupation if the monster was associated */
	if(mdef->moccupation) stop_occupation();
	
	/* drop special items like the Amulet so that a dismissed Keter or nurse
	   can't remove them from the game */
	mdrop_special_objs(mdef);
	/* release rest of monster's inventory--it is removed from game */
	// discard_minvent(mdef);
// #ifndef GOLDOBJ
	// mdef->mgold = 0L;
// #endif
	m_detach(mdef, mdef->data);
}

void
monslime(mdef)
struct monst *mdef;
{
	int mhp = mdef->mhp;
	lifesaved_monster(mdef);
	if (mdef->mhp > 0)
		return;
	mdef->mhp = mhp;
	if(mdef->mtyp == PM_ARA_KAMEREL && !has_template(mdef, FRACTURED)){
		ara_died(mdef);
	}
	(void)newcham(mdef, PM_GREEN_SLIME, FALSE, canseemon(mdef));
}

/* drop a statue or rock and remove monster */
void
monstone(mdef)
register struct monst *mdef;
{
	struct obj *otmp, *obj, *oldminvent;
	xchar x = mdef->mx, y = mdef->my;
	boolean wasinside = FALSE;

	/* This happens twice because some paths enter monstone() directly instead of through minstapetrify()*/
	if (poly_when_stoned(mdef->data)) {
		mon_to_stone(mdef);
		return;
	}
	else if(is_delouseable(mdef->data)){
		mdef = delouse(mdef, AD_STON);
		return;
	}
	/* we have to make the statue before calling mondead, to be able to
	 * put inventory in it, and we have to check for lifesaving before
	 * making the statue....
	 */
	lifesaved_monster(mdef);
	if (mdef->mhp > 0) return;
	/* we did not lifesave */
	mdef->deadmonster |= DEADMONSTER_DEAD;

	mdef->mtrapped = 0;	/* (see m_detach) */

	if ((int)mdef->data->msize > MZ_TINY ||
		    !rn2(2 + ((int) (mdef->data->geno & G_FREQ) > 2))) {
		oldminvent = 0;
		/* some objects may end up outside the statue */
		while ((obj = mdef->minvent) != 0) {
		    obj_extract_self(obj);
		    if (obj->owornmask)
			update_mon_intrinsics(mdef, obj, FALSE, TRUE);
		    obj_no_longer_held(obj);
		    if (obj->owornmask & W_WEP){
				setmnotwielded(mdef,obj);
				MON_NOWEP(mdef);
			}
		    if (obj->owornmask & W_SWAPWEP){
				setmnotwielded(mdef,obj);
				MON_NOSWEP(mdef);
			}
		    obj->owornmask = 0L;
		    if (is_boulder(obj) ||
#if 0				/* monsters don't carry statues */
     (obj->otyp == STATUE && mons[obj->corpsenm].msize >= mdef->data->msize) ||
#endif
				obj_resists(obj, 0, 0)) {
				if (flooreffects(obj, x, y, "fall")) continue;
				place_object(obj, x, y);
		    } else {
				if (obj_is_burning(obj)) end_burn(obj, TRUE);
				obj->nobj = oldminvent;
				oldminvent = obj;
		    }
		}
		/* defer statue creation until after inventory removal
		   so that saved monster traits won't retain any stale
		   item-conferred attributes */
		otmp = mkcorpstat(STATUE, KEEPTRAITS(mdef) ? mdef : 0,
				  mdef->data, x, y, FALSE);
		if (M_HAS_NAME(mdef)) otmp = oname(otmp, MNAME(mdef));
		while ((obj = oldminvent) != 0) {
		    oldminvent = obj->nobj;
		    (void) add_to_container(otmp, obj);
		}
#ifndef GOLDOBJ
		if (mdef->mgold) {
			struct obj *au;
			au = mksobj(GOLD_PIECE, MKOBJ_NOINIT);
			au->quan = mdef->mgold;
			au->owt = weight(au);
			(void) add_to_container(otmp, au);
			mdef->mgold = 0;
		}
#endif
		/* Archeologists should not break unique statues */
		if (mdef->data->geno & G_UNIQ)
			otmp->spe = STATUE_HISTORIC;
		if (has_template(mdef, ILLUMINATED))
			otmp->spe |= STATUE_FACELESS;
		otmp->owt = weight(otmp);
	} else
		otmp = mksobj_at(ROCK, x, y, NO_MKOBJ_FLAGS);

	stackobj(otmp);
	/* mondead() already does this, but we must do it before the newsym */
	if(glyph_is_invisible(levl[x][y].glyph))
	    unmap_object(x, y);
	if (cansee(x, y)) newsym(x,y);
	/* We don't currently trap the hero in the statue in this case but we could */
	if (u.uswallow && u.ustuck == mdef) wasinside = TRUE;
	mondead(mdef);
	if (wasinside) {
		if (is_animal(mdef->data))
			You("%s through an opening in the new %s.",
				locomotion(&youmonst, "jump"),
				xname(otmp));
	}
}

/* drop a gold statue or rock and remove monster */
void
mongolded(mdef)
register struct monst *mdef;
{
	struct obj *otmp, *obj, *oldminvent;
	xchar x = mdef->mx, y = mdef->my;
	boolean wasinside = FALSE;

	/* we have to make the statue before calling mondead, to be able to
	 * put inventory in it, and we have to check for lifesaving before
	 * making the statue....
	 */
	lifesaved_monster(mdef);
	if (mdef->mhp > 0) return;
	/* we did not lifesave */
	mdef->deadmonster |= DEADMONSTER_DEAD;

	mdef->mtrapped = 0;	/* (see m_detach) */

	if ((int)mdef->data->msize > MZ_TINY ||
		    !rn2(2 + ((int) (mdef->data->geno & G_FREQ) > 2))) {
		oldminvent = 0;
		/* some objects may end up outside the statue */
		while ((obj = mdef->minvent) != 0) {
		    obj_extract_self(obj);
		    if (obj->owornmask)
			update_mon_intrinsics(mdef, obj, FALSE, TRUE);
		    obj_no_longer_held(obj);
		    if (obj->owornmask & W_WEP){
				setmnotwielded(mdef,obj);
				MON_NOWEP(mdef);
			}
		    if (obj->owornmask & W_SWAPWEP){
				setmnotwielded(mdef,obj);
				MON_NOSWEP(mdef);
			}
		    obj->owornmask = 0L;
		    if (is_boulder(obj) ||
#if 0				/* monsters don't carry statues */
     (obj->otyp == STATUE && mons[obj->corpsenm].msize >= mdef->data->msize) ||
#endif
				obj_resists(obj, 0, 0)) {
			if (flooreffects(obj, x, y, "fall")) continue;
			place_object(obj, x, y);
		    } else {
			if (obj_is_burning(obj)) end_burn(obj, TRUE);
			obj->nobj = oldminvent;
			oldminvent = obj;
		    }
		}
		/* defer statue creation until after inventory removal
		   so that saved monster traits won't retain any stale
		   item-conferred attributes */
		otmp = mkcorpstat(STATUE, KEEPTRAITS(mdef) ? mdef : 0,
				  mdef->data, x, y, FALSE);
		set_material_gm(otmp, GOLD);
		if (M_HAS_NAME(mdef)) otmp = oname(otmp, MNAME(mdef));
		while ((obj = oldminvent) != 0) {
		    oldminvent = obj->nobj;
			set_material(obj, GOLD);
		    (void) add_to_container(otmp, obj);
		}
#ifndef GOLDOBJ
		if (mdef->mgold) {
			struct obj *au;
			au = mksobj(GOLD_PIECE, MKOBJ_NOINIT);
			au->quan = mdef->mgold;
			au->owt = weight(au);
			(void) add_to_container(otmp, au);
			mdef->mgold = 0;
		}
#endif
		/* Archeologists should not break unique statues */
		if (mdef->data->geno & G_UNIQ)
			otmp->spe = STATUE_HISTORIC;
		if (has_template(mdef, ILLUMINATED))
			otmp->spe |= STATUE_FACELESS;
		otmp->owt = weight(otmp);
	} else {
		oldminvent = 0;
		/* some objects may end up outside the statue */
		while ((obj = mdef->minvent) != 0) {
		    obj_extract_self(obj);
		    if (obj->owornmask)
			update_mon_intrinsics(mdef, obj, FALSE, TRUE);
		    obj_no_longer_held(obj);
		    if (obj->owornmask & W_WEP){
				setmnotwielded(mdef,obj);
				MON_NOWEP(mdef);
			}
		    if (obj->owornmask & W_SWAPWEP){
				setmnotwielded(mdef,obj);
				MON_NOSWEP(mdef);
			}
		    obj->owornmask = 0L;
		    if (is_boulder(obj) ||
#if 0				/* monsters don't carry statues */
     (obj->otyp == STATUE && mons[obj->corpsenm].msize >= mdef->data->msize) ||
#endif
				obj_resists(obj, 0, 0)) {
			if (flooreffects(obj, x, y, "fall")) continue;
			place_object(obj, x, y);
		    } else {
			if (obj_is_burning(obj)) end_burn(obj, TRUE);
			obj->nobj = oldminvent;
			oldminvent = obj;
		    }
		}
		while ((obj = oldminvent) != 0) {
		    oldminvent = obj->nobj;
			set_material(obj, GOLD);
			place_object(obj, x, y);
			stackobj(obj);
		}
		otmp = mksobj_at(ROCK, x, y, NO_MKOBJ_FLAGS);
		set_material(otmp, GOLD);
		if (M_HAS_NAME(mdef)) otmp = oname(otmp, MNAME(mdef));
	}
	
	stackobj(otmp);
	/* mondead() already does this, but we must do it before the newsym */
	if(glyph_is_invisible(levl[x][y].glyph))
	    unmap_object(x, y);
	if (cansee(x, y)) newsym(x,y);
	/* We don't currently trap the hero in the statue in this case but we could */
	if (u.uswallow && u.ustuck == mdef) wasinside = TRUE;
	mondead(mdef);
	if (wasinside) {
		if (is_animal(mdef->data))
			You("%s through an opening in the new %s.",
				locomotion(&youmonst, "jump"),
				xname(otmp));
	}
}

/* drop a glass statue or rock and remove monster */
void
monglassed(mdef)
register struct monst *mdef;
{
	struct obj *otmp, *obj, *oldminvent;
	xchar x = mdef->mx, y = mdef->my;
	boolean wasinside = FALSE;

	/* we have to make the statue before calling mondead, to be able to
	 * put inventory in it, and we have to check for lifesaving before
	 * making the statue....
	 */
	lifesaved_monster(mdef);
	if (mdef->mhp > 0) return;
	/* we did not lifesave */
	mdef->deadmonster |= DEADMONSTER_DEAD;

	mdef->mtrapped = 0;	/* (see m_detach) */

	if ((int)mdef->data->msize > MZ_TINY ||
		    !rn2(2 + ((int) (mdef->data->geno & G_FREQ) > 2))) {
		oldminvent = 0;
		/* some objects may end up outside the statue */
		while ((obj = mdef->minvent) != 0) {
		    obj_extract_self(obj);
		    if (obj->owornmask)
			update_mon_intrinsics(mdef, obj, FALSE, TRUE);
		    obj_no_longer_held(obj);
		    if (obj->owornmask & W_WEP){
				setmnotwielded(mdef,obj);
				MON_NOWEP(mdef);
			}
		    if (obj->owornmask & W_SWAPWEP){
				setmnotwielded(mdef,obj);
				MON_NOSWEP(mdef);
			}
		    obj->owornmask = 0L;
		    if (is_boulder(obj) ||
#if 0				/* monsters don't carry statues */
     (obj->otyp == STATUE && mons[obj->corpsenm].msize >= mdef->data->msize) ||
#endif
				obj_resists(obj, 0, 0)) {
			if (flooreffects(obj, x, y, "fall")) continue;
			place_object(obj, x, y);
		    } else {
			if (obj_is_burning(obj)) end_burn(obj, TRUE);
			obj->nobj = oldminvent;
			oldminvent = obj;
		    }
		}
		/* defer statue creation until after inventory removal
		   so that saved monster traits won't retain any stale
		   item-conferred attributes */
		otmp = mkcorpstat(STATUE, KEEPTRAITS(mdef) ? mdef : 0,
				  mdef->data, x, y, FALSE);
		set_material_gm(otmp, GLASS);
		if (M_HAS_NAME(mdef)) otmp = oname(otmp, MNAME(mdef));
		while ((obj = oldminvent) != 0) {
		    oldminvent = obj->nobj;
		    (void) add_to_container(otmp, obj);
		}
#ifndef GOLDOBJ
		if (mdef->mgold) {
			struct obj *au;
			au = mksobj(GOLD_PIECE, MKOBJ_NOINIT);
			au->quan = mdef->mgold;
			au->owt = weight(au);
			(void) add_to_container(otmp, au);
			mdef->mgold = 0;
		}
#endif
		/* Archeologists should not break unique statues */
		if (mdef->data->geno & G_UNIQ)
			otmp->spe = STATUE_HISTORIC;
		if (has_template(mdef, ILLUMINATED))
			otmp->spe |= STATUE_FACELESS;
		otmp->owt = weight(otmp);
	} else {
		oldminvent = 0;
		/* some objects may end up outside the statue */
		while ((obj = mdef->minvent) != 0) {
		    obj_extract_self(obj);
		    if (obj->owornmask)
			update_mon_intrinsics(mdef, obj, FALSE, TRUE);
		    obj_no_longer_held(obj);
		    if (obj->owornmask & W_WEP){
				setmnotwielded(mdef,obj);
				MON_NOWEP(mdef);
			}
		    if (obj->owornmask & W_SWAPWEP){
				setmnotwielded(mdef,obj);
				MON_NOSWEP(mdef);
			}
		    obj->owornmask = 0L;
		    if (is_boulder(obj) ||
#if 0				/* monsters don't carry statues */
     (obj->otyp == STATUE && mons[obj->corpsenm].msize >= mdef->data->msize) ||
#endif
				obj_resists(obj, 0, 0)) {
			if (flooreffects(obj, x, y, "fall")) continue;
			place_object(obj, x, y);
		    } else {
			if (obj_is_burning(obj)) end_burn(obj, TRUE);
			obj->nobj = oldminvent;
			oldminvent = obj;
		    }
		}
		while ((obj = oldminvent) != 0) {
		    oldminvent = obj->nobj;
			place_object(obj, x, y);
			stackobj(obj);
		}
		otmp = mksobj_at(ROCK, x, y, NO_MKOBJ_FLAGS);
		set_material(otmp, GLASS);
		if (M_HAS_NAME(mdef)) otmp = oname(otmp, MNAME(mdef));
	}
	
	stackobj(otmp);
	/* mondead() already does this, but we must do it before the newsym */
	if(glyph_is_invisible(levl[x][y].glyph))
	    unmap_object(x, y);
	if (cansee(x, y)) newsym(x,y);
	/* We don't currently trap the hero in the statue in this case but we could */
	if (u.uswallow && u.ustuck == mdef) wasinside = TRUE;
	mondead(mdef);
	if (wasinside) {
		if (is_animal(mdef->data))
			You("%s through an opening in the new %s.",
				locomotion(&youmonst, "jump"),
				xname(otmp));
	}
}

/* another monster has killed the monster mdef */
void
monkilled(mdef, fltxt, how)
register struct monst *mdef;
const char *fltxt;
int how;
{
	boolean be_sad = FALSE;		/* true if unseen pet is killed */

	if(Infuture && is_myrkalfr(mdef->data) && !get_template(mdef) && canseemon(mdef) && how != AD_DGST){
		pline("Strands of black webbing flow from %s mortal wounds, engulfing %s body and choking off %s screams!", s_suffix(mon_nam(mdef)), mhis(mdef), mhis(mdef));
		pline("Now totally engulfed, %s is yanked upright and vanishes from the world.", mhe(mdef));
	}
	else if ((mdef->wormno ? worm_known(mdef) : cansee(mdef->mx, mdef->my))
		&& fltxt)
	    pline("%s is %s%s%s!", Monnam(mdef),
			(banish_kill_mon(mdef) && !has_template(mdef, SPARK_SKELETON)) ? "banished" : nonliving(mdef->data) ? "destroyed" : "killed",
		    *fltxt ? " by the " : "",
		    fltxt
		 );
	else
	    be_sad = (mdef->mtame != 0);

	/* no corpses if digested or disintegrated */
	if(how == AD_DGST || how == -AD_RBRE || how == AD_DISN)
	    mondead(mdef);
	else if(how == AD_STON)
	    monstone(mdef);
	else
	    mondied(mdef);

	if (be_sad && mdef->mhp <= 0)
	    You("have a sad feeling for a moment, then it passes.");
	if (is_fern_spore(mdef->data)) {
		spore_dies(mdef);
	}
}

void
unstuck(mtmp)
struct monst *mtmp;
{
	if(u.ustuck == mtmp) {
		if(u.uswallow){
			u.ux = mtmp->mx;
			u.uy = mtmp->my;
			u.uswallow = 0;
			u.uswldtim = 0;
			if (Punished){
				placebc();
			}
			vision_full_recalc = 1;
			docrt();
		}
		u.ustuck = 0;
	}
}

/* Monster is damaged by some source */
/*  Returns 1 if it died or 0 otherwise. */
boolean
m_losehp(mon, dam, yours, how)
struct monst *mon;
int dam;
boolean yours;
char *how;
// int adtype; //Note: AD type should be used to signal nocorpse etc, but would not be respected if yours.
{
	boolean killed = FALSE;

	if ((mon->mhp -= dam) <= 0) {
		int xx = mon->mx;
		int yy = mon->my;

		if (yours) xkilled(mon, TRUE);
		else monkilled(mon, how, AD_PHYS);
		if (mon->mhp <= 0) {
			newsym(xx, yy);
			killed = TRUE;
		}
	}
	return killed;
}

void
killed(mtmp)
register struct monst *mtmp;
{
	xkilled(mtmp, 1);
}

struct obj *
mk_death_drop_obj(mtmp)
struct monst *mtmp;
{
	int typ;
	struct obj *otmp;

	otmp = mkobj(RANDOM_CLASS, MKOBJ_ARTIF);
	if(In_quest(&u.uz) && !Role_if(PM_CONVICT)){
		if(otmp->oclass == WEAPON_CLASS || otmp->oclass == ARMOR_CLASS) otmp->objsize = (&mons[urace.malenum])->msize;
		if(otmp->oclass == ARMOR_CLASS){
			set_obj_shape(otmp, mons[urace.malenum].mflagsb);
		}
	}
	/* Don't create large objects from small monsters */
	typ = otmp->otyp;
	if (mtmp->data->msize < MZ_HUMAN && typ != FOOD_RATION
		&& typ != LEASH
		&& typ != FIGURINE
		&& (otmp->owt > max(3, mtmp->data->cwt/10) || objects[typ].oc_size > MZ_MEDIUM)
		&& !is_divider(mtmp->data)
	) {
		//properly uncreate an artifact
		if (otmp->oartifact)
			artifact_exists(otmp, ONAME(otmp), FALSE);
		delobj(otmp);
		otmp = 0;
	}
	return otmp;
}

/* the player has killed the monster mtmp */
void
xkilled(mtmp, dest)
	register struct monst *mtmp;
/*
 * Dest=1, normal; dest=0, don't print message; dest=2, don't drop corpse
 * either; dest=3, message but no corpse
 */
	int	dest;
{
	int tmp, x = mtmp->mx, y = mtmp->my;
	struct permonst *mdat;
	int mndx;
	struct obj *otmp;
	struct trap *t;
	boolean redisp = FALSE, illalarm = FALSE;
	boolean wasinside = u.uswallow && (u.ustuck == mtmp);


	/* KMH, conduct */
	u.uconduct.killer++;
	if(mtmp->mtyp == PM_CROW && u.sealsActive&SEAL_MALPHAS) unbind(SEAL_MALPHAS,TRUE);

	if(uwep && check_oprop(uwep, OPROP_WRTHW)){
		struct obj *otmp = uwep;
		if(wrath_target(otmp, mtmp)){
			if((otmp->wrathdata&0x3L) < 3) otmp->wrathdata++;
		}
		else {
			if(has_template(mtmp, ZOMBIFIED) || has_template(mtmp, YELLOW_DEAD)){
				otmp->wrathdata = PM_GHOUL<<2;
			} else if(has_template(mtmp, SKELIFIED)){
				otmp->wrathdata = PM_SKELETON<<2;
			} else if(has_template(mtmp, VAMPIRIC)){
				otmp->wrathdata = PM_VAMPIRE<<2;
			} else if(has_template(mtmp, PSEUDONATURAL)){
				otmp->wrathdata = PM_PARASITIC_MIND_FLAYER<<2;
			} else {
				otmp->wrathdata = monsndx(mtmp->data)<<2;
			}
		}
	}
	if(uswapwep && u.twoweap && check_oprop(uswapwep, OPROP_WRTHW)){
		struct obj *otmp = uswapwep;
		if(wrath_target(otmp, mtmp)){
			if((otmp->wrathdata&0xFF) < 3) otmp->wrathdata++;
		}
		else {
			if(has_template(mtmp, ZOMBIFIED) || has_template(mtmp, YELLOW_DEAD)){
				otmp->wrathdata = PM_GHOUL<<2;
			} else if(has_template(mtmp, SKELIFIED)){
				otmp->wrathdata = PM_SKELETON<<2;
			} else if(has_template(mtmp, VAMPIRIC)){
				otmp->wrathdata = PM_VAMPIRE<<2;
			} else if(has_template(mtmp, PSEUDONATURAL)){
				otmp->wrathdata = PM_PARASITIC_MIND_FLAYER<<2;
			} else {
				otmp->wrathdata = monsndx(mtmp->data)<<2;
			}
		}
	}
	if(mtmp->mtyp == PM_LUCKSUCKER && mtmp->mvar_lucksucker){
		You_feel("your luck return!");
		change_luck(mtmp->mvar_lucksucker);
	}
	if (dest & 1) {
		static boolean sflm_message = FALSE;
	    const char *verb = (banish_kill_mon(mtmp) && !has_template(mtmp, SPARK_SKELETON)) ? "banish" : (mtmp->mflamemarked && !Infuture) ? "burn" : nonliving(mtmp->data) ? "destroy" : "kill";

	    if (!wasinside && !canspotmon(mtmp))
			You("%s it!", verb);
	    else if(Infuture && is_myrkalfr(mtmp->data) && !get_template(mtmp) && canseemon(mtmp)){
			pline("Strands of black webbing flow from %s mortal wounds, engulfing %s body and choking off %s screams!", s_suffix(mon_nam(mtmp)), mhis(mtmp), mhis(mtmp));
			pline("Now totally engulfed, %s is yanked upright and vanishes from the world.", mhe(mtmp));
		}
	    else if(!banish_kill_mon(mtmp) && mtmp->mibitemarked && !Infuture){
			pline("%s vanishes down into the murky waters.", Monnam(mtmp));
		}
	    else if(!banish_kill_mon(mtmp) && mtmp->mflamemarked && !Infuture && !sflm_message){
			pline("%s burns from within, consumed by silver fire!", Monnam(mtmp));
			sflm_message = TRUE;
		}
	    else {
			You("%s %s!", verb,
				!mtmp->mtame ? mon_nam(mtmp) :
				x_monnam(mtmp,
					 M_HAS_NAME(mtmp) ? ARTICLE_NONE : ARTICLE_THE,
					 "poor",
					 M_HAS_NAME(mtmp) ? SUPPRESS_SADDLE : 0,
					 FALSE));
	    }
		if(!mtmp->mflamemarked)
			sflm_message = FALSE;
	}

	if(Is_illregrd(&u.uz)){
		if(mtmp->mtrapped && t_at(x, y) && t_at(x, y)->ttyp == VIVI_TRAP){
			illalarm = TRUE;
		}
	}

	// You killed a mummy and suffer from its curse.
	if(!mtmp->mcan && attacktype_fordmg(mtmp->data, AT_NONE, AD_MROT)){
		mummy_curses_x(mtmp, &youmonst);
	}
	
	if (mtmp->mtrapped && (t = t_at(x, y)) != 0 &&
		(t->ttyp == PIT || t->ttyp == SPIKED_PIT) &&
		boulder_at(x, y))
	    dest |= 2;     /*
			    * Prevent corpses/treasure being created "on top"
			    * of the boulder that is about to fall in. This is
			    * out of order, but cannot be helped unless this
			    * whole routine is rearranged.
			    */

	/* your pet knows who just killed it...watch out */
	if (get_mx(mtmp, MX_EDOG)) EDOG(mtmp)->killed_by_u = 1;

	/* dispose of monster and make cadaver */
	if(stoned) monstone(mtmp);
	else if(golded) mongolded(mtmp);
	else if(glassed) monglassed(mtmp);
	else mondead(mtmp);

	if (mtmp->mhp > 0) { /* monster lifesaved */
		/* Cannot put the non-visible lifesaving message in
		 * lifesaved_monster() since the message appears only when you
		 * kill it (as opposed to visible lifesaving which always
		 * appears).
		 */
		stoned = FALSE;
		golded = FALSE;
		glassed = FALSE;
		if (!cansee(x,y)) pline("Maybe not...");
		return;
	}
	/* we did not lifesave */
	mtmp->deadmonster |= DEADMONSTER_DEAD;
	mtmp->mbdrown = 0;

	mdat = mtmp->data; /* note: mondead can change mtmp->data */
	mndx = monsndx(mdat);

	if (is_fern_spore(mdat)) {
		spore_dies(mtmp);
	}
	if (stoned) {
		stoned = FALSE;
		goto cleanup;
	}

	if (golded) {
		golded = FALSE;
		goto cleanup;
	}

	if (glassed) {
		glassed = FALSE;
		goto cleanup;
	}

	if(!is_rider(mdat) && ((dest & 2) || LEVEL_SPECIFIC_NOCORPSE(mdat)))
		goto cleanup;

#ifdef MAIL
	if(mdat->mtyp == PM_MAIL_DAEMON) {
		stackobj(mksobj_at(SCR_MAIL, x, y, MKOBJ_NOINIT));
		redisp = TRUE;
	}
#endif
	if((!accessible(x, y) && !is_pool(x, y, FALSE)) ||
	   (x == u.ux && y == u.uy)) {
	    /* might be mimic in wall or corpse in lava or on player's spot */
	    redisp = TRUE;
	    if(wasinside) spoteffects(TRUE);
	} else if(x != u.ux || y != u.uy) {
		/* might be here after swallowed */
		int out_of = 6;
		if(active_glyph(EYE_THOUGHT) && active_glyph(LUMEN))
			out_of = 2;
		else if(active_glyph(EYE_THOUGHT))
			out_of = 3;
		else if(active_glyph(LUMEN))
			out_of = 4;
		if (!rn2(out_of) && !(mvitals[mndx].mvflags & G_NOCORPSE)
					&& mdat->mlet != S_KETER
					&& mdat->mlet != S_PLANT
					&& !(get_mx(mtmp, MX_ESUM))
					&& !(mtmp->mclone)
					/*Not reviving templates*/
					&& !(has_template(mtmp, ZOMBIFIED))
					&& !(has_template(mtmp, VAMPIRIC))
					&& !(has_template(mtmp, TOMB_HERD))
					&& !(has_template(mtmp, YELLOW_TEMPLATE))
					&& !(has_template(mtmp, SPORE_ZOMBIE))
					&& !(has_template(mtmp, CORDYCEPS))
					&& !(is_auton(mtmp->data))
		) {
			/*Death Drop*/
			otmp = mk_death_drop_obj(mtmp);
			if(otmp){
				place_object(otmp, x, y);
				redisp = TRUE;
			}
		}
		/* Whether or not it always makes a corpse is, in theory,
		 * different from whether or not the corpse is "special";
		 * if we want both, we have to specify it explicitly.
		 */
		struct obj *corpse = 0;
		if (corpse_chance(mtmp, (struct monst *)0, FALSE)){
			corpse = make_corpse(mtmp);
		}
		if(mtmp->mironmarked && (
			is_elf(mtmp->data)
			|| is_fey(mtmp->data)
		)){
			u.uz.rage++;
		}
		if(mtmp->mibitemarked){
			mtmp->mflamemarked = FALSE;
			mtmp->mgoatmarked = FALSE;
			artinstance[ART_IBITE_ARM].IbiteFavor += monstr[mtmp->mtyp] + 1;
			if(wizard)
				pline("IbiteCredit = %ld", artinstance[ART_IBITE_ARM].IbiteFavor);
		}
		if(mtmp->mflamemarked){
			if(mtmp->data->geno&G_NOCORPSE)
				flame_consume(mtmp, (struct obj *) 0, mtmp->myoumarked);
			else if(corpse){
				flame_consume((struct monst *) 0, corpse, mtmp->myoumarked);
				corpse = (struct obj *)0; //corpse pointer is now stale
			}
		}
		if(corpse && corpse->otyp == CORPSE && !corpse->oartifact){
			//We are in the "player has killed monster" function, so it's their fault
			if(goat_mouth_at(x,y) && has_object_type(invent, HOLY_SYMBOL_OF_THE_BLACK_MOTHE)){
				goat_eat(corpse, GOAT_EAT_OFFERED); //Goat eat tries *really* hard to destroy whatever you give it.
			} else if(mtmp->mgoatmarked && !Infuture){
				goat_eat(corpse, mtmp->myoumarked ? GOAT_EAT_MARKED : GOAT_EAT_PASSIVE); //Goat eat tries *really* hard to destroy whatever you give it.
			} else goat_seenonce = FALSE;
			corpse = (struct obj *)0; //corpse pointer is now stale
		}
	}
	if(redisp) newsym(x,y);
cleanup:
	/* punish bad behaviour */
	if(murderable_mon(mtmp) && 
	  !(u.sealsActive&SEAL_MALPHAS) && mtmp->malign <= 0 &&
	   (mndx < PM_ARCHEOLOGIST || mndx > PM_WIZARD) &&
	   u.ualign.type != A_CHAOTIC) {
		HTelepat &= ~INTRINSIC;
		change_luck(-2);
		if(mtmp->mtyp == PM_BLASPHEMOUS_LURKER)
			You("...murderer!?");
		else
			You("murderer!");
		IMPURITY_UP(u.uimp_murder)
		if(u.ualign.type == A_LAWFUL){
			change_hod(10);
			u.ualign.sins += 5;
			if(u.usanity > 0){
				change_usanity(-1*rnd(4), FALSE);
			}
		}
		else{
			change_hod(5); 
			u.ualign.sins += 2;
			if(u.usanity > 0){
				change_usanity(-1, FALSE);
			}
		}
		if (Blind && !Blind_telepat)
		    see_monsters(); /* Can't sense monsters any more. */
	}
	if((mtmp->mpeaceful && !rn2(2)) || mtmp->mtame)	change_luck(-1);
	if (is_unicorn(mdat) &&
		!always_hostile(mdat) &&
		sgn(u.ualign.type) == sgn(mdat->maligntyp) && 
		u.ualign.type != A_VOID
	) {
		change_luck(-5);
		change_hod(10);
		You_feel("guilty...");
	}
	/*Killing the specimens in the Dungeon of Ill-Regard angers the autons*/
	if(illalarm){
		u.uevent.uaxus_foe = 1;
		pline("An alarm sounds!");
		aggravate();
	}
	/* give experience points */
	if (mvitals[mndx].killed < 255)
		mvitals[mndx].killed++; /*The kills the player specifically has been responsible for*/
	tmp = experience(mtmp, (int)mvitals[mndx].killed);
	more_experienced(tmp, 0);
	newexplevel();		/* will decide if you go up */
	if(!u.veil && !mvitals[mndx].insightkill && yields_insight(mtmp->data)){
		mvitals[mndx].insightkill = TRUE;
		uchar insight = u_insight_gain(mtmp);
		mvitals[monsndx(mtmp->data)].insight_gained += insight;
		change_uinsight(insight);
		change_usanity(u_sanity_gain(mtmp), FALSE);
	}

	/* adjust alignment points */
	if (mtmp->m_id == quest_status.leader_m_id && !always_hostile(mtmp->data)) {
		if(flags.leader_backstab){ /* They attacked you! */
			adjalign((int)(ALIGNLIM/4));
			// pline("That was %sa bad idea...",
					// u.uevent.qcompleted ? "probably " : "");
		} else {/* REAL BAD! */
			adjalign(-(u.ualign.record+(int)ALIGNLIM/2));
			change_hod(30);
			pline("That was %sa bad idea...",
					u.uevent.qcompleted ? "probably " : "");
		}
	} else if (mdat->msound == MS_NEMESIS){	/* Real good! */
	    adjalign((int)(ALIGNLIM/4));
		change_hod(-10);
	} else if (quest_faction(mtmp) && mdat->mtyp == urole.guardnum && mdat->mtyp != PM_THUG) {	/*nobody cares if you kill thugs*/
	    adjalign(-(int)(ALIGNLIM/8));															/* what's a little murder amongst rogues?*/
		change_hod(10);
	    if (!Hallucination) pline("That was probably a bad idea...");
	    else pline("Whoopsie-daisy!");
	}else if (mtmp->ispriest && !always_hostile(mdat)) {
		adjalign((p_coaligned(mtmp)) ? -2 : 2);
		/* cancel divine protection for killing your priest */
		if (p_coaligned(mtmp)) u.ublessed = 0;
		if (mdat->maligntyp == A_NONE)
			adjalign((int)(ALIGNLIM / 4));		/* BIG bonus */
	} else if (mtmp->mtame && u.ualign.type != A_VOID && !banish_kill_mon(mtmp) && !(EDOG(mtmp) && EDOG(mtmp)->dominated)) {
		adjalign(-15);	/* bad!! */
		/* your god is mighty displeased... */
		if (!Hallucination) You_hear("the rumble of distant thunder...");
		else You_hear("the studio audience applaud!");
	} else if (mtmp->mpeaceful)
		adjalign(-5);
	
	if(((mtmp->mferal || mtmp->mtame)) && u.sealsActive&SEAL_BERITH) unbind(SEAL_BERITH,TRUE);

	if(mtmp->mferal){
		IMPURITY_UP(u.uimp_betrayal)
	}

	/* malign was already adjusted for u.ualign.type and randomization */
	adjalign(mtmp->malign);
}

/* changes the monster into a stone monster of the same type */
/* this should only be called when poly_when_stoned() is true */
void
mon_to_stone(mtmp)
    register struct monst *mtmp;
{
    if(mtmp->data->mlet == S_GOLEM) {
	/* it's a golem, and not a stone golem */
	if(canseemon(mtmp))
	    pline("%s solidifies...", Monnam(mtmp));
	if (newcham(mtmp, PM_STONE_GOLEM, FALSE, FALSE)) {
	    if(canseemon(mtmp))
		pline("Now it's %s.", an(mtmp->data->mname));
	} else {
	    if(canseemon(mtmp))
		pline("... and returns to normal.");
	}
    } else
	impossible("Can't polystone %s!", a_monnam(mtmp));
}

void
mon_to_gold(mtmp)
    register struct monst *mtmp;
{
    if(mtmp->data->mlet == S_GOLEM) {
	/* it's a golem, and not a stone golem */
	if(canseemon(mtmp))
	    pline("%s turns shiny...", Monnam(mtmp));
	if (newcham(mtmp, PM_GOLD_GOLEM, FALSE, FALSE)) {
	    if(canseemon(mtmp))
		pline("Now it's %s.", an(mtmp->data->mname));
	} else {
	    if(canseemon(mtmp))
		pline("... and returns to normal.");
	}
    } else
	impossible("Can't polygold %s!", a_monnam(mtmp));
}

void
mnexto(mtmp)	/* Make monster mtmp next to you (if possible) */
	struct monst *mtmp;
{
	coord mm;

#ifdef STEED
	if (mtmp == u.usteed) {
		/* Keep your steed in sync with you instead */
		mtmp->mx = u.ux;
		mtmp->my = u.uy;
		return;
	}
#endif

	if(!enexto(&mm, u.ux, u.uy, mtmp->data)) return;
	rloc_to(mtmp, mm.x, mm.y);
	return;
}

void
monline(mtmp)	/* Make monster mtmp next to you (if possible) */
	struct monst *mtmp;
{
	coord mm;

#ifdef STEED
	if (mtmp == u.usteed) {
		/* Keep your steed in sync with you instead */
		mtmp->mx = u.ux;
		mtmp->my = u.uy;
		return;
	}
#endif

	if(!eonline(&mm, u.ux, u.uy, mtmp->data)) return;
	rloc_to(mtmp, mm.x, mm.y);
	return;
}

void
mofflin(mtmp)	/* Make monster mtmp near to you (if possible) */
	struct monst *mtmp;
{
	coord mm;

#ifdef STEED
	if (mtmp == u.usteed) {
		/* Keep your steed in sync with you instead */
		mtmp->mx = u.ux;
		mtmp->my = u.uy;
		return;
	}
#endif

	if(!eofflin(&mm, u.ux, u.uy, mtmp->data)) return;
	rloc_to(mtmp, mm.x, mm.y);
	return;
}

void
mofflin_close(mtmp)	/* Make monster mtmp near to you (if possible) */
	struct monst *mtmp;
{
	coord mm;

#ifdef STEED
	if (mtmp == u.usteed) {
		/* Keep your steed in sync with you instead */
		mtmp->mx = u.ux;
		mtmp->my = u.uy;
		return;
	}
#endif

	if(!eofflin_close(&mm, u.ux, u.uy, mtmp->data)) return;
	rloc_to(mtmp, mm.x, mm.y);
	return;
}

/* mnearto()
 * Put monster near (or at) location if possible.
 * Returns:
 *	1 - if a monster was moved from x, y to put mtmp at x, y.
 *	0 - in most cases.
 */
boolean
mnearto(mtmp,x,y,move_other)
register struct monst *mtmp;
xchar x, y;
boolean move_other;	/* make sure mtmp gets to x, y! so move m_at(x, y) */
{
	struct monst *othermon = (struct monst *)0;
	xchar newx, newy;
	coord mm;

	if ((mtmp->mx == x) && (mtmp->my == y)) return(FALSE);

	if (move_other && (othermon = m_at(x, y))) {
		if (othermon->wormno)
			remove_worm(othermon);
		else
			remove_monster(x, y);
	}

	newx = x;
	newy = y;

	if (!goodpos(newx, newy, mtmp, 0)) {
		/* actually we have real problems if enexto ever fails.
		 * migrating_mons that need to be placed will cause
		 * no end of trouble.
		 */
		if (!enexto(&mm, newx, newy, mtmp->data)) return(FALSE);
		newx = mm.x; newy = mm.y;
	}

	rloc_to(mtmp, newx, newy);

	if (move_other && othermon) {
	    othermon->mx = othermon->my = 0;
	    (void) mnearto(othermon, x, y, FALSE);
	    if ((othermon->mx != x) || (othermon->my != y))
		return(TRUE);
	}

	return(FALSE);
}


static const char *poiseff[] = {

	" feel weaker", "r brain is on fire",
	"r judgement is impaired", "r muscles won't obey you",
	" feel very sick", " break out in hives"
};

void
poisontell(typ)

	int	typ;
{
	pline("You%s.", poiseff[typ]);
}

/* poisoned()
 * the player is poisoned by something other than an object's poison coating (which is done via hmon in xhity)
 * 
 */
void
poisoned(string, attrib, pname, fatal, severe)
const char *string;	/* base string of poison message (The killer bee's sting) */
int attrib;			/* attribute to target */
const char *pname;	/* name of poisoner (for killer) */
int fatal;			/* 1 in X chance of significant debilitation */
boolean severe;			/* Powerful poison that partially overcomes poison resistance */
{
	int i, kprefix = KILLED_BY_AN;
	int drain;
	boolean plural = strcmp(string, makesingular(string));
	boolean blast = !strcmp(string, "blast");
	boolean printed = FALSE;

	if(!blast) {
	    /* avoid "The" Orcus's sting was poisoned... */
	    pline("%s%s %s poisoned!", isupper(*string) ? "" : "The ",
			string, plural ? "were" : "was");
	}
	if (Poison_resistance && !severe) {
		if (!blast)
			shieldeff(u.ux, u.uy);
		pline_The("poison doesn't seem to affect you.");
		/* fear of snakes (and so poison) */
		if (roll_madness(MAD_OPHIDIOPHOBIA)){
			You("panic anyway!");
			HPanicking += 1+rnd(3);
		}
	}
	else {
		/* suppress killer prefix if it already has one */
		if ((i = name_to_mon(pname)) >= LOW_PM && mons[i].geno & G_UNIQ) {
			kprefix = KILLED_BY;
			if (!type_is_pname(&mons[i])) pname = the(pname);
		}
		else if (!strncmpi(pname, "the ", 4) ||
			!strncmpi(pname, "an ", 3) ||
			!strncmpi(pname, "a ", 2)) {
			/*[ does this need a plural check too? ]*/
			kprefix = KILLED_BY;
		}
		if(severe && !Poison_resistance)
			fatal = (fatal + 1)/2;
		i = rn2(fatal);
		if (i == 0 && attrib != A_CHA) {
			drain = attrib == A_CON ? -2 : -rn1(3, 3);
			if(Poison_resistance)
				drain = (drain - 1)/2;
			else if(severe)
				drain -= 4;
			if (adjattrib(A_CON, drain, 1)){
				pline_The("poison was quite debilitating...");
				printed = TRUE;
			}
			IMPURITY_UP(u.uimp_poison)
		}
		if (i <= 5) {
			drain = -rn1(3, 3);
			if(Poison_resistance)
				drain = (drain - 1)/2;
			else if(severe)
				drain -= 2;
			/* Check that a stat change was made */
			if (adjattrib(attrib, drain, 1) && !printed)
				pline("You%s!", poiseff[attrib]);
		}

		i = rn1(10, 6);
		if (Poison_resistance) i = (i + 1) / 2;
		i = reduce_dmg(&youmonst,i,FALSE,TRUE);
		losehp(i, pname, kprefix);

		if (u.uhp < 1) {
			killer_format = kprefix;
			killer = pname;
			/* "Poisoned by a poisoned ___" is redundant */
			done(strstri(pname, "poison") ? DIED : POISONING);
		}
		if (roll_madness(MAD_OPHIDIOPHOBIA)){
			You("panic!");
			HPanicking += 1+rnd(6);
		}
		(void)encumber_msg();
	}
	return;
}

/* monster responds to player action; not the same as a passive attack */
/* assumes reason for response has been tested, and response _must_ be made */
void
m_respond(mtmp)
register struct monst *mtmp;
{
	register int i;
    if(mtmp->data->msound == MS_SHRIEK) {
		if(flags.soundok) {
			pline("%s shrieks.", Monnam(mtmp));
			mtmp->mnoise = TRUE;
			stop_occupation();
		}
		if (!rn2(10)) {
			if (mtmp->mtyp == PM_SHRIEKER && !rn2(13))
			(void) makemon(&mons[PM_PURPLE_WORM], 0, 0, NO_MM_FLAGS);
			else
			(void) makemon((struct permonst *)0, 0, 0, NO_MM_FLAGS);

		}
		aggravate();
    } else if(!(mtmp->mspec_used) &&
		(
		mtmp->data->msound == MS_JUBJUB || mtmp->data->msound == MS_DREAD
		|| mtmp->data->msound == MS_SONG || mtmp->data->msound == MS_OONA
		|| mtmp->data->msound == MS_INTONE || mtmp->data->msound == MS_FLOWER
		|| mtmp->data->msound == MS_TRUMPET || mtmp->mtyp == PM_RHYMER
		|| mtmp->data->msound == MS_SECRETS || mtmp->data->msound == MS_HOWL
		|| mtmp->data->msound == MS_SCREAM || mtmp->data->msound == MS_HARROW
		|| mtmp->data->msound == MS_APOC
		)
	) {
		domonnoise(mtmp, FALSE);
    }
	else if(mtmp->data->msound == MS_SHOG){
		domonnoise(mtmp, FALSE);
	}
    if(is_weeping(mtmp->data)) {
		for(i = 0; i < NATTK; i++)
			 if(mtmp->data->mattk[i].aatyp == AT_GAZE) {
			 (void) xgazey(mtmp, &youmonst, &mtmp->data->mattk[i], -1);
			 break;
			 }
    }
}

void
m_widegaze(mtmp)
register struct monst *mtmp;
{
	struct attack gaze_mem;
	struct attack *gaze = mon_get_attacktype(mtmp, AT_WDGZ, &gaze_mem);
	if(gaze) {
		if (!Gaze_immune)
			(void) xgazey(mtmp, &youmonst, gaze, -1);
	}
}

#endif /* OVLB */
#ifdef OVL2

void
setmangry(mtmp)
register struct monst *mtmp;
{
	mtmp->mstrategy &= ~STRAT_WAITMASK;
	if(!mtmp->mpeaceful) return;
	if(mtmp->mtame) return;
	mtmp->mpeaceful = 0;
	newsym(mtmp->mx, mtmp->my);
	if(mtmp->ispriest) {
		if(p_coaligned(mtmp)) adjalign(-5); /* very bad */
		else adjalign(2);
	} else
		adjalign(-1);		/* attacking peaceful monsters is bad */
	
	if(mtmp->mtyp == PM_DANCING_BLADE && mtmp->mvar_suryaID){
		struct monst *surya;
		for(surya = fmon; surya; surya = surya->nmon) if(surya->m_id == mtmp->mvar_suryaID) break;
		if(surya) setmangry(surya);
	}
	if(mtmp->mtyp == PM_SURYA_DEVA){
		struct monst *blade;
		for(blade = fmon; blade; blade = blade->nmon) if(blade->mtyp == PM_DANCING_BLADE && mtmp->m_id == blade->mvar_suryaID) break;
		if(blade) setmangry(blade);
	}
	if (couldsee(mtmp->mx, mtmp->my)) {
		if (humanoid(mtmp->data) || mtmp->isshk || mtmp->isgd)
		    pline("%s gets angry!", Monnam(mtmp));
		else if (flags.verbose && flags.soundok) growl(mtmp);
		if(flags.stag && (mtmp->mtyp == PM_DROW_MATRON_MOTHER || mtmp->mtyp == PM_ECLAVDRA)){
			struct monst *tm;
			for(tm = fmon; tm; tm = tm->nmon){
				if(tm->mfaction != EILISTRAEE_SYMBOL && 
					tm->mfaction != XAXOX && tm->mfaction != EDDER_SYMBOL && 
					is_drow(tm->data) && !tm->mtame 
				){
					tm->housealert = 1;
					tm->mpeaceful = 0;
					tm->mstrategy &= ~STRAT_WAITMASK;
					set_malign(tm);
				}
			}
		}
	}

	/* attacking your own quest leader will anger his or her guardians */
	if (!flags.mon_moving &&	/* should always be the case here */
		mtmp->m_id == quest_status.leader_m_id) {
	    struct monst *mon;
	    struct permonst *q_guardian = &mons[quest_info(MS_GUARDIAN)];
	    int got_mad = 0;

	    /* guardians will sense this attack even if they can't see it */
	    for (mon = fmon; mon; mon = mon->nmon)
		if (!DEADMONSTER(mon) && quest_faction(mon) && mon->mpeaceful) {
		    mon->mpeaceful = 0;
			newsym(mon->mx, mon->my);
		    if (canseemon(mon)) ++got_mad;
		}
	    if (got_mad && !Hallucination)
		pline_The("%s appear%s to be angry too...",
		      got_mad == 1 ? q_guardian->mname :
				    makeplural(q_guardian->mname),
		      got_mad == 1 ? "s" : "");
	}
}

void
wakeup(mtmp, anger)
register struct monst *mtmp;
int anger;
{
	mtmp->msleeping = 0;
	mtmp->meating = 0;	/* assume there's no salvagable food left */
	mtmp->mstrategy &= ~(STRAT_WAITMASK);
	if(anger) setmangry(mtmp);
	if(mtmp->m_ap_type) see_passive_mimic(mtmp);
	else if (flags.forcefight && !flags.mon_moving && mtmp->mundetected) {
	    mtmp->mundetected = 0;
	    newsym(mtmp->mx, mtmp->my);
	}
}

/* Wake up nearby monsters. */
void
wake_nearby()
{
	register struct monst *mtmp;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (!DEADMONSTER(mtmp) && distu(mtmp->mx,mtmp->my) < u.ulevel*20) {
			mtmp->msleeping = 0;
	    }
	}
}

/* Wake up nearby monsters, deafening those with sensitive ears. */
void
wake_nearby_noisy(){
	register struct monst *mtmp;
	wake_nearby();
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (!DEADMONSTER(mtmp) && sensitive_ears(mtmp->data) && !is_deaf(mtmp) &&
				 distu(mtmp->mx,mtmp->my) < (u.ulevel*20)/3){
			mtmp->mstun = 1;
			mtmp->mconf = 1;
			mtmp->mcanhear = 0;
			mtmp->mdeafened = min(125, (u.ulevel*20)/3 - distu(mtmp->mx,mtmp->my));
		}
	}
}

/* Wake up monsters near some particular location. */
void
wake_nearto(x, y, distance)
register int x, y, distance;
{
	if(!isok(x,y)) {
		impossible("bad wake_nearto location (%d %d)", x, y);
		return;
	}
	register struct monst *mtmp;

	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (!DEADMONSTER(mtmp) && mtmp->msleeping && !is_deaf(mtmp) && (distance == 0 ||
				 dist2(mtmp->mx, mtmp->my, x, y) < distance)
		){
			mtmp->msleeping = 0;
			if(no_upos(mtmp)){
				mtmp->mux = x;
				mtmp->muy = y;
			}
		}
	}
}

/* Wake up monsters near some particular location, deafening those with sensitive ears. */
void
wake_nearto_noisy(x, y, distance)
register int x, y, distance;
{
	register struct monst *mtmp;
	wake_nearto(x,y,distance);
	distance /= 3;
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (!DEADMONSTER(mtmp)){
			if(sensitive_ears(mtmp->data) && !is_deaf(mtmp) &&
			 dist2(mtmp->mx, mtmp->my, x, y) < distance
			){
				mtmp->mstun = 1;
				mtmp->mconf = 1;
				mtmp->mcanhear = 0;
				mtmp->mdeafened = min(125, distance - dist2(mtmp->mx, mtmp->my, x, y));
			}
			if(mtmp->mtyp == PM_ECHO){
				struct monst *tmpm;
				int targets = 0, damage = 0;
				for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
					if(distmin(tmpm->mx,tmpm->my,mtmp->mx,mtmp->my) <= 4
						&& tmpm->mpeaceful != mtmp->mpeaceful
						&& tmpm->mtame != mtmp->mtame
						&& !nonliving(tmpm->data)
						&& !resists_drain(tmpm)
						&& !DEADMONSTER(tmpm)
						&& !nonthreat(tmpm)
					) targets++;
				}
				if(dist2(u.ux,u.uy,mtmp->mx,mtmp->my) <= distance
					&& !mtmp->mpeaceful
					&& !mtmp->mtame
					&& !nonliving(youracedata)
				) targets++;
				if(targets){
					targets = rnd(targets);
					for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
						if(dist2(tmpm->mx,tmpm->my,mtmp->mx,mtmp->my) <= distance
							&& tmpm->mpeaceful != mtmp->mpeaceful
							&& tmpm->mtame != mtmp->mtame
							&& !nonliving(tmpm->data)
							&& !resists_drain(tmpm)
							&& !DEADMONSTER(tmpm)
							&& !nonthreat(tmpm)
						) targets--;
						if(!targets) break;
					}
				}
				if(tmpm){
					if(canseemon(tmpm)){
						pline("Powerful reverberations shake %s to %s soul!", mon_nam(tmpm), hisherits(tmpm));
					}
					tmpm->mconf = 1;
					tmpm->mcanhear = 0;
					tmpm->mdeafened = min(125, distance - dist2(tmpm->mx, tmpm->my, x, y));
					damage = tmpm->m_lev/2+1;
					if(damage > 0){
						tmpm->mhp -= 8*damage;
						tmpm->mhpmax -= 8*damage;
						if(tmpm->m_lev < damage) 
							tmpm->m_lev = 0;
						else tmpm->m_lev -= damage;
						if(tmpm->mhp < 1
						|| tmpm->mhpmax < 1
						|| tmpm->m_lev < 0
						){
							grow_up(mtmp,tmpm);
							mondied(tmpm);
							//Grow up may have killed mtmp
							if(DEADMONSTER(mtmp))
								return;
						}
					}
				} else if(targets > 0
					&& dist2(u.ux,u.uy,mtmp->mx,mtmp->my) <= distance
					&& !mtmp->mpeaceful
					&& !mtmp->mtame
					&& !Drain_resistance
					&& !nonliving(youracedata)
				){
					pline("Powerful reverberations shake you to your soul!");
					damage = u.ulevel/2+1;
					while(--damage)
						losexp("soul echoes",FALSE,TRUE,TRUE);
					losexp("soul echoes",TRUE,TRUE,TRUE);
					losehp(8, "soul echoes", KILLED_BY); //might kill you before you hit level 0;
				}
			}
		}
	}
}

/* reveals monster-mimickers as well as passive-mimickers with a surprised message */
/* If the player is suffering from the REAL_DELUSIONS madness, this may instead polymorph mtmp to its pretend form */
void
seemimic_ambush(mtmp)
struct monst *mtmp;
{
	//Note: not affected by time stop
	if(mtmp->m_ap_type == M_AP_MONSTER && NightmareAware_Sanity < rnd(100))
		return; //Failed to notice monster wasn't as it seemed
	unsigned old_app = mtmp->mappearance;
	uchar old_ap_type = mtmp->m_ap_type;
	boolean couldsense = canseemon(mtmp) && !(sensemon(mtmp) && old_ap_type != M_AP_MONSTER);
	
	seemimic(mtmp);

	if (couldsense) {
		const char * app;
		switch (old_ap_type)
		{
		case M_AP_OBJECT:
			if (old_app == STRANGE_OBJECT)
				app = "strange object";
			else
				app = simple_typename(old_app);
			break;
		case M_AP_FURNITURE:
			app = defsyms[old_app].explanation;
			break;
		case M_AP_MONSTER:
			app = mons[old_app].mname;
			break;
		case M_AP_NOTHING:
			app = m_monnam(mtmp);
			break;
		}
		pline("That %s was actually %s!",
			app,
			a_monnam(mtmp)
			);
	}
	/* if suffering from *real* delusions, and monster was mimicing a monster, it may polymorph! */
	if (roll_madness(MAD_REAL_DELUSIONS) && old_ap_type == M_AP_MONSTER) {
		if(!(mtmp->data->geno&(G_UNIQ|G_NOGEN)) // isn't a unique or special monster
			&& !(mons[old_app].geno&(G_UNIQ|G_NOGEN)) // not pretending to be a unique or special monster
			&& !(roll_madness(MAD_REAL_DELUSIONS) && (monstr[mtmp->mtyp] > monstr[old_app])) // if a 2nd roll succeeds, only if becoming something nastier
			){
			if (couldsense)
				pline("...or was it?");
			newcham(mtmp, old_app, FALSE, FALSE);
		}
	}
	return;
}


/* NOTE: we must check for mimicry before calling this routine */
/* reveals monster-mimickers as well as passive-mimickers, with no message */
void
seemimic(mtmp)
register struct monst *mtmp;
{
	unsigned old_app = mtmp->mappearance;
	uchar old_ap_type = mtmp->m_ap_type;

	mtmp->m_ap_type = M_AP_NOTHING;
	mtmp->mappearance = 0;

	/*
	 *  Discovered mimics don't block light.
	 */
	if (((old_ap_type == M_AP_FURNITURE &&
	      (old_app == S_hcdoor || old_app == S_vcdoor)) ||
	     (old_ap_type == M_AP_OBJECT && old_app == BOULDER)) &&
	    !does_block(mtmp->mx, mtmp->my, &levl[mtmp->mx][mtmp->my]))
	    unblock_point(mtmp->mx, mtmp->my);

	newsym(mtmp->mx,mtmp->my);
}

/* seemimic() for furniture and objects only */
void
see_passive_mimic(mtmp)
register struct monst * mtmp;
{
	if (mtmp->m_ap_type == M_AP_FURNITURE || mtmp->m_ap_type == M_AP_OBJECT)
		seemimic(mtmp);	
}

/* force all chameleons to become normal */
void
rescham()
{
	register struct monst *mtmp;
	int mcham;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if (DEADMONSTER(mtmp)) continue;
		mcham = (int) mtmp->cham;
		if (mcham) {
			mtmp->cham = CHAM_ORDINARY;
			(void) newcham(mtmp, cham_to_pm[mcham],
				       FALSE, FALSE);
		}
		if(is_were(mtmp->data) && !healing_were(mtmp->data) && mtmp->data->mlet != S_HUMAN)
			new_were(mtmp);
		if(mtmp->m_ap_type && cansee(mtmp->mx, mtmp->my) && mtmp->m_ap_type != M_AP_MONSTER) {
			seemimic(mtmp);
			/* we pretend that the mimic doesn't */
			/* know that it has been unmasked.   */
			mtmp->msleeping = 1;
		}
	}
}

/* Let the chameleons change again -dgk */
void
restartcham()
{
	register struct monst *mtmp;

	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if (DEADMONSTER(mtmp)) continue;
		mtmp->cham = pm_to_cham(monsndx(mtmp->data));
		if (mtmp->data->mlet == S_MIMIC && mtmp->msleeping &&
				cansee(mtmp->mx, mtmp->my)) {
			set_mimic_sym(mtmp);
			newsym(mtmp->mx,mtmp->my);
		}
	}
}

/* called when restoring a monster from a saved level; protection
   against shape-changing might be different now than it was at the
   time the level was saved. */
void
restore_cham(mon)
struct monst *mon;
{
	int mcham;

	if (Protection_from_shape_changers) {
	    mcham = (int) mon->cham;
	    if (mcham) {
		mon->cham = CHAM_ORDINARY;
		(void) newcham(mon, cham_to_pm[mcham], FALSE, FALSE);
	    } else if (is_were(mon->data) && !healing_were(mon->data) && !is_human(mon->data)) {
		new_were(mon);
	    }
	} else if (mon->cham == CHAM_ORDINARY) {
	    mon->cham = pm_to_cham(monsndx(mon->data));
	}
}

/* unwatched hiders may hide again; if so, a 1 is returned.  */
STATIC_OVL boolean
restrap(mtmp)
register struct monst *mtmp;
{
	if(mtmp->cham || mtmp->mcan || mtmp->m_ap_type ||
	   cansee(mtmp->mx, mtmp->my) || rn2(3) || (mtmp == u.ustuck) ||
	   (sensemon(mtmp) && distu(mtmp->mx, mtmp->my) <= 2))
		return(FALSE);

	if(mtmp->data->mlet == S_MIMIC) {
		set_mimic_sym(mtmp);
		return(TRUE);
	} else
	    if(levl[mtmp->mx][mtmp->my].typ == ROOM)  {
		mtmp->mundetected = 1;
		return(TRUE);
	    }

	return(FALSE);
}

/* is_animal() as a proper function (and of mtyp) for use with rndshape() */
boolean
pick_animal(mtyp)
int mtyp;
{
	return (is_animal(&mons[mtyp]));
}

int
select_newcham_form(mon)
struct monst *mon;
{
	int mndx = NON_PM;
	int attempts = 0;

	do {
		switch (mon->cham) {
			case CHAM_SANDESTIN:
			if (rn2(7)) mndx = pick_nasty();
			break;
			case CHAM_DOPPELGANGER:
				if (!rn2(7)) mndx = pick_nasty();
				else if (rn2(3)){
					do{
						mndx = rn1(PM_WIZARD - PM_ARCHEOLOGIST + 1, PM_ARCHEOLOGIST);
					} while(mndx == PM_ITINERANT_PRIESTESS
						|| mndx == PM_TRANSCENDENT_VALKYRIE
						|| mndx == PM_AWAKENED_VALKYRIE
						|| mndx == PM_WORM_THAT_WALKS
						|| mndx == PM_INCANTIFIER);
				}
			break;
			case CHAM_CHAMELEON:
			if (!rn2(3)) mndx = rndshape(&pick_animal);
			else mndx = rndshape((void*)0);//try to get an in-depth monster of any kind
			break;
			case CHAM_DREAM:
			if(rn2(2)) mndx = rndshape((void*)0);//try to get an in-depth monster of any kind
			else mndx = PM_DREAM_QUASIELEMENTAL;
			break;
			case CHAM_ORDINARY:
			if (attempts == 0) {
				struct obj *m_armr = which_armor(mon, W_ARM);

				if (m_armr && Is_dragon_scales(m_armr))
					mndx = Dragon_scales_to_pm(m_armr) - mons;
				else if (m_armr && Is_dragon_mail(m_armr))
					mndx = Dragon_mail_to_pm(m_armr) - mons;
				else if ((m_armr = which_armor(mon, W_ARMC)) && m_armr->otyp == LEO_NEMAEUS_HIDE)
					mndx = PM_SON_OF_TYPHON;
			}
			break;
		}
	#ifdef WIZARD
		/* For debugging only: allow control of polymorphed monster; not saved */
		if (wizard && iflags.mon_polycontrol && attempts == 0) {
			char pprompt[BUFSZ], buf[BUFSZ];
			int tries = 0;
			do {
				Sprintf(pprompt,
					"Change %s into what kind of monster? [type the name]",
					mon_nam(mon));
				getlin(pprompt,buf);
				mndx = name_to_mon(buf);
				if (mndx < LOW_PM)
					You("cannot polymorph %s into that.", mon_nam(mon));
				else break;
			} while(++tries < 5);
			if (tries==5) pline1(thats_enough_tries);
		}
	#endif /*WIZARD*/

		/* otherwise */
		if (mndx == NON_PM) mndx = rndshape((void*)0);//first try to get an in-depth monster

		/* special case for mtemplates: "deloused" can be lost via polymorph */
		if (mon->mtemplate == DELOUSED && !mtemplate_accepts_mtyp(DELOUSED, mndx))
			mon->mtemplate = 0;
		else if (!mtemplate_accepts_mtyp(mon->mtemplate, mndx))
			mndx = NON_PM;

		/* after 50 attempts, try really hard */
		if (attempts > 50 && mndx == NON_PM)
			mndx = rn1(SPECIAL_PM - LOW_PM, LOW_PM);

	} while(++attempts < 60 && (mndx == NON_PM || !mtemplate_accepts_mtyp(mon->mtemplate, mndx)));

	if (attempts >= 60)	// failed to find an acceptable new form
		mndx = mon->mtyp;

	return mndx;
}

/* make a chameleon look like a new monster; returns 1 if it actually changed */
int
newcham(mtmp, mtyp, polyspot, msg)
struct monst *mtmp;
int mtyp;			/* index of monster data to turn into */
boolean polyspot;	/* change is the result of wand or spell of polymorph */
boolean msg;		/* "The oldmon turns into a newmon!" */
{
	int mhp, hpn, hpd;
	int tryct;
	int oldmtyp = mtmp->mtyp;
	int faceless = 0;
	struct permonst *olddata = mtmp->data;
	struct permonst * mdat = &mons[mtyp];
	char oldname[BUFSZ];
	
	if (msg) {
	    /* like Monnam() but never mention saddle */
	    Strcpy(oldname, x_monnam(mtmp, ARTICLE_THE, (char *)0,
				     SUPPRESS_SADDLE, FALSE));
	    oldname[0] = highc(oldname[0]);
	}

	/* mdat = 0 -> caller wants a random monster shape */
	tryct = 0;
	if (mtyp == NON_PM) {
	    while (++tryct <= 100) {
		mtyp = select_newcham_form(mtmp);
		mdat = &mons[mtyp];
		if ((mvitals[mtyp].mvflags & G_GENOD && !In_quest(&u.uz)) != 0 ||
			is_placeholder(mdat)) continue;
		/* polyok rules out all MG_PNAME and MA_WERE's;
		   select_newcham_form might deliberately pick a player
		   character type, so we can't arbitrarily rule out all
		   human forms any more */
		if (is_mplayer(mdat) || (!is_human(mdat) && polyok(mdat)))
		    break;
	    }
	    if (tryct > 100) return 0;	/* Should never happen */
	} else if (mvitals[mtyp].mvflags & G_GENOD && !In_quest(&u.uz))
	    return(0);	/* passed in mdat is genocided */

	if(is_male(mdat)) {
		if(mtmp->female) mtmp->female = FALSE;
	} else if (is_female(mdat)) {
		if(!mtmp->female) mtmp->female = TRUE;
	} else if (!is_neuter(mdat)) {
		if(!rn2(10)) mtmp->female = !mtmp->female;
	}

	if (In_endgame(&u.uz) && is_mplayer(olddata) && M_HAS_NAME(mtmp)) {
		/* mplayers start out as "Foo the Bar", but some of the
		 * titles are inappropriate when polymorphed, particularly
		 * into the opposite sex.  players don't use ranks when
		 * polymorphed, so dropping the rank for mplayers seems
		 * reasonable.
		 */
		char *p = index(MNAME(mtmp), ' ');
		if (p) {
			char nambuf[BUFSZ];
			Strcpy(nambuf, MNAME(mtmp));
			*p = '\0';
			christen_monst(mtmp, nambuf);
		}
	}

	if (mtyp == oldmtyp) return(0);	/* still the same monster */

	if(mtmp->wormno) {			/* throw tail away */
		wormgone(mtmp);
		place_monster(mtmp, mtmp->mx, mtmp->my);
	}
	
	/* Possibly Unblock  */
	if (!opaque(mdat) && opaque(olddata)) unblock_point(mtmp->mx, mtmp->my);
	/* Possibly Block  */
	if (opaque(mdat) && !opaque(olddata)) block_point(mtmp->mx, mtmp->my);
	
	if(mtmp->cham != CHAM_DREAM){
		hpn = mtmp->mhp;
		hpd = (mtmp->m_lev < 50) ? ((int)mtmp->m_lev)*8 : mdat->mlevel;
		if(!hpd) hpd = 4;

		mtmp->m_lev = adj_lev(mdat);		/* new monster level */

		mhp = (mtmp->m_lev < 50) ? ((int)mtmp->m_lev)*8 : mdat->mlevel;
		if(!mhp) mhp = 4;

		/* new hp: same fraction of max as before */
#ifndef LINT
		mtmp->mhp = (int)(((long)hpn*(long)mhp)/(long)hpd);
#endif
		if(mtmp->mhp < 0) mtmp->mhp = hpn;	/* overflow */
	/* Unlikely but not impossible; a 1HD creature with 1HP that changes into a
	   0HD creature will require this statement */
		if (!mtmp->mhp) mtmp->mhp = 1;

	/* and the same for maximum hit points */
		hpn = mtmp->mhpmax;
#ifndef LINT
		mtmp->mhpmax = (int)(((long)hpn*(long)mhp)/(long)hpd);
#endif
		if(mtmp->mhpmax < 0) mtmp->mhpmax = hpn;	/* overflow */
		if (!mtmp->mhpmax) mtmp->mhpmax = 1;
	} //else just take on new form I think....

	if (is_horror(mdat)) {
		rem_mx(mtmp, MX_EHOR);	// in case it used to be a horror before
		add_mx(mtmp, MX_EHOR);
		if (mtyp == PM_NAMELESS_HORROR) {
			extern char * nameless_horror_name;
			int plslev = rn2(12);
			EHOR(mtmp)->basehorrordata = *mdat;
			nameless_horror_name = EHOR(mtmp)->randname;
			make_horror(&(EHOR(mtmp)->basehorrordata), 37 + plslev, 15 + plslev);
			nameless_horror_name = (char *)0;
			mdat = &(EHOR(mtmp)->basehorrordata);
			EHOR(mtmp)->currhorrordata = *mdat;
		}
		else {
			EHOR(mtmp)->basehorrordata = *mdat;
			EHOR(mtmp)->currhorrordata = *mdat;
		}
	}
	/* take on the new form... */
	set_mon_data(mtmp, mtyp);

	/* monster lightsources */
	del_light_source(mtmp->light);	/* clear old */
	if (emits_light_mon(mtmp)|| olddata->mtyp == PM_MASKED_QUEEN) {
		/* masked queen is effectively illuminated, but not -- poly should add illumination */
		if(olddata->mtyp == PM_MASKED_QUEEN)
			set_template(mtmp, ILLUMINATED);
		/* now add lightsource */
		new_light_source(LS_MONSTER, (genericptr_t)mtmp, emits_light_mon(mtmp));
	}
	if (!mtmp->perminvis || pm_invisible(olddata))
	    mtmp->perminvis = pm_invisible(mdat);
	mtmp->minvis = mtmp->invis_blkd ? 0 : mtmp->perminvis;
	if (!(hides_under(mdat) && OBJ_AT(mtmp->mx, mtmp->my)) &&
			!(is_underswimmer(mdat) && is_pool(mtmp->mx, mtmp->my, FALSE)))
		mtmp->mundetected = 0;
	if (u.ustuck == mtmp) {
		if(u.uswallow) {
			if(!attacktype(mdat,AT_ENGL)) {
				/* Does mdat care? */
				if (!noncorporeal(mdat) && !amorphous(mdat) &&
				    !is_whirly(mdat) &&
				    (mdat->mtyp != PM_YELLOW_LIGHT)) {
					You("break out of %s%s!", mon_nam(mtmp),
					    (is_animal(mdat)?
					    "'s stomach" : ""));
					mtmp->mhp = 1;  /* almost dead */
				}
				expels(mtmp, olddata, FALSE);
			} else {
				/* update swallow glyphs for new monster */
				swallowed(0);
			}
		} else if (!sticks(mtmp) && !sticks(&youmonst))
			unstuck(mtmp);
	}

#ifndef DCC30_BUG
	if ((mtyp == PM_LONG_WORM || mtyp == PM_HUNTING_HORROR || mtyp == PM_CHORISTER_JELLY) && 
		(mtmp->wormno = get_wormno()) != 0) {
#else
	/* DICE 3.0 doesn't like assigning and comparing mtmp->wormno in the
	 * same expression.
	 */
	if ((mtyp == PM_LONG_WORM || mtyp == PM_HUNTING_HORROR || mtyp == PM_CHORISTER_JELLY) &&
		(mtmp->wormno = get_wormno(), mtmp->wormno != 0)) {
#endif
	    /* we can now create worms with tails - 11/91 */
		initworm(mtmp, mtyp == PM_HUNTING_HORROR ? 2 : rn2(5));
	    if (count_wsegs(mtmp))
		place_worm_tail_randomly(mtmp, mtmp->mx, mtmp->my);
	}

	newsym(mtmp->mx,mtmp->my);

	if (msg) {
		char nambuf[BUFSZ];
		boolean hadname = FALSE;
		if(M_HAS_NAME(mtmp)) {
			Strcpy(nambuf, MNAME(mtmp));
	    	rem_mx(mtmp, MX_ENAM);
			hadname = TRUE;
		}
	    pline("%s turns into %s!", oldname,
			mtyp == PM_GREEN_SLIME ? "slime" :
		  x_monnam(mtmp, ARTICLE_A, (char*)0, SUPPRESS_SADDLE, FALSE));

		if (hadname) {
			christen_monst(mtmp, nambuf);
		}
	}

	possibly_unwield(mtmp, polyspot);	/* might lose use of weapon */
	mon_break_armor(mtmp, polyspot);
	if (!(mtmp->misc_worn_check & W_ARMG))
	    mselftouch(mtmp, "No longer petrify-resistant, ",
			!flags.mon_moving);

	/* This ought to re-test can_carry() on each item in the inventory
	 * rather than just checking ex-giants & boulders, but that'd be
	 * pretty expensive to perform.  If implemented, then perhaps
	 * minvent should be sorted in order to drop heaviest items first.
	 */
	/* former giants can't continue carrying boulders */
	if (mtmp->minvent && !throws_rocks(mdat)) {
	    register struct obj *otmp, *otmp2;

	    for (otmp = mtmp->minvent; otmp; otmp = otmp2) {
		otmp2 = otmp->nobj;
		if (is_boulder(otmp)) {
		    /* this keeps otmp from being polymorphed in the
		       same zap that the monster that held it is polymorphed */
		    if (polyspot) bypass_obj(otmp);
		    obj_extract_self(otmp);
		    /* probably ought to give some "drop" message here */
		    if (flooreffects(otmp, mtmp->mx, mtmp->my, "")) continue;
		    place_object(otmp, mtmp->mx, mtmp->my);
		}
	    }
	}

	return(1);
}

/* sometimes an egg will be special */
#define BREEDER_EGG (!rn2(77))

/*
 * Determine if the given monster number can be hatched from an egg.
 * Return the monster number to use as the egg's corpsenm.  Return
 * NON_PM if the given monster can't be hatched.
 */
int
can_be_hatched(mtyp)
int mtyp;
{
    /* ranger quest nemesis has the oviparous bit set, making it
       be possible to wish for eggs of that unique monster; turn
       such into ordinary eggs rather than forbidding them outright */
    if (mtyp == PM_SCORPIUS) mtyp = PM_SCORPION;
	else if(mtyp == PM_ANCIENT_NAGA) mtyp = rn2(PM_GUARDIAN_NAGA_HATCHLING - PM_RED_NAGA_HATCHLING + 1) + PM_RED_NAGA_HATCHLING;
	else if(mtyp == PM_SERPENT_MAN_OF_YOTH) mtyp = rn2(PM_COBRA - PM_GARTER_SNAKE + 1) + PM_GARTER_SNAKE;
	else if(mtyp == PM_SMAUG) mtyp = PM_BABY_RED_DRAGON;
	
    mtyp = little_to_big(mtyp, (boolean)rn2(2));
    /*
     * Queen bees lay killer bee eggs (usually), but killer bees don't
     * grow into queen bees.  Ditto for [winged-]gargoyles.
     */
    if (mtyp == PM_KILLER_BEE || mtyp == PM_GARGOYLE ||
	    (lays_eggs(&mons[mtyp]) && (BREEDER_EGG ||
		(mtyp != PM_QUEEN_BEE && mtyp != PM_WINGED_GARGOYLE))))
	return mtyp;
    return NON_PM;
}

/* type of egg laid by #sit; usually matches parent */
int
egg_type_from_parent(mtyp, force_ordinary)
int mtyp;	/* parent monster; caller must handle lays_eggs() check */
boolean force_ordinary;
{
    if (force_ordinary || !BREEDER_EGG) {
	if (mtyp == PM_QUEEN_BEE) mtyp = PM_KILLER_BEE;
	else if (mtyp == PM_WINGED_GARGOYLE) mtyp = PM_GARGOYLE;
    }
    return mtyp;
}

/* decide whether an egg of the indicated monster type is viable; */
/* also used to determine whether an egg or tin can be created... */
boolean
dead_species(m_idx, egg)
int m_idx;
boolean egg;
{
	/*
	 * For monsters with both baby and adult forms, genociding either
	 * form kills all eggs of that monster.  Monsters with more than
	 * two forms (small->large->giant mimics) are more or less ignored;
	 * fortunately, none of them have eggs.  Species extinction due to
	 * overpopulation does not kill eggs.
	 */
	return (boolean)
		(m_idx >= LOW_PM &&
		 ((mvitals[m_idx].mvflags & G_GENOD && !In_quest(&u.uz)) != 0 ||
		  (egg &&
		   (mvitals[big_to_little(m_idx)].mvflags & G_GENOD && !In_quest(&u.uz)) != 0)));
}

/* kill off any eggs of genocided monsters */
STATIC_OVL void
kill_eggs(obj_list)
struct obj *obj_list;
{
	struct obj *otmp;

	for (otmp = obj_list; otmp; otmp = otmp->nobj)
	    if (otmp->otyp == EGG) {
		if (dead_species(otmp->corpsenm, TRUE)) {
		    /*
		     * It seems we could also just catch this when
		     * it attempted to hatch, so we wouldn't have to
		     * search all of the objlists.. or stop all
		     * hatch timers based on a corpsenm.
		     */
		    kill_egg(otmp);
		}
#if 0	/* not used */
	    } else if (otmp->otyp == TIN) {
		if (dead_species(otmp->corpsenm, FALSE))
		    otmp->corpsenm = NON_PM;	/* empty tin */
	    } else if (otmp->otyp == CORPSE) {
		if (dead_species(otmp->corpsenm, FALSE))
		    ;		/* not yet implemented... */
#endif
	    } else if (Has_contents(otmp)) {
		kill_eggs(otmp->cobj);
	    }
}

/* kill all members of genocided species */
void
kill_genocided_monsters()
{
	struct monst *mtmp, *mtmp2;
	boolean kill_cham[CHAM_MAX_INDX+1];
	int mndx;

	kill_cham[CHAM_ORDINARY] = FALSE;	/* (this is mndx==0) */
	for (mndx = 1; mndx <= CHAM_MAX_INDX; mndx++)
	  kill_cham[mndx] = (mvitals[cham_to_pm[mndx]].mvflags & G_GENOD && !In_quest(&u.uz)) != 0;
	/*
	 * Called during genocide, and again upon level change.  The latter
	 * catches up with any migrating monsters as they finally arrive at
	 * their intended destinations, so possessions get deposited there.
	 *
	 * Chameleon handling:
	 *	1) if chameleons have been genocided, destroy them
	 *	   regardless of current form;
	 *	2) otherwise, force every chameleon which is imitating
	 *	   any genocided species to take on a new form.
	 */
	for (mtmp = fmon; mtmp; mtmp = mtmp2) {
	    mtmp2 = mtmp->nmon;
	    if (DEADMONSTER(mtmp)) continue;
	    mndx = monsndx(mtmp->data);
	    if ((mvitals[mndx].mvflags & G_GENOD && !In_quest(&u.uz)) || kill_cham[mtmp->cham]) {
		if (mtmp->cham && !kill_cham[mtmp->cham])
		    (void) newcham(mtmp, NON_PM, FALSE, FALSE);
		else
		    mondead(mtmp);
	    }
	    if (mtmp->minvent) kill_eggs(mtmp->minvent);
	}

	kill_eggs(invent);
	kill_eggs(fobj);
	kill_eggs(level.buriedobjlist);
}

#endif /* OVL2 */
#ifdef OVLB

void
golemeffects(mon, damtype, dam)
struct monst *mon;
int damtype, dam;
{
    int heal = 0, slow = 0;

	/* intercept player */
	if (mon == &youmonst) {
		ugolemeffects(damtype, dam);
		return;
	}

    if (mon->mtyp == PM_FLESH_GOLEM) {
	if (damtype == AD_ELEC || damtype == AD_EELC) heal = dam / 6;
	else if (damtype == AD_FIRE || damtype == AD_EFIR 
		|| damtype == AD_ECLD || damtype == AD_COLD
	) slow = 1;
    } else if (mon->mtyp == PM_IRON_GOLEM || mon->mtyp == PM_GREEN_STEEL_GOLEM || mon->mtyp == PM_CHAIN_GOLEM || mon->mtyp == PM_ARGENTUM_GOLEM) {
	if (damtype == AD_ELEC || damtype == AD_EELC) slow = 1;
	else if (damtype == AD_FIRE || damtype == AD_EFIR) heal = dam;
    } else {
	return;
    }
    if (slow) {
	if (mon->mspeed != MSLOW)
	    mon_adjust_speed(mon, -1, (struct obj *)0, TRUE);
    }
    if (heal) {
	if (mon->mhp < mon->mhpmax) {
	    mon->mhp += dam;
	    if (mon->mhp > mon->mhpmax) mon->mhp = mon->mhpmax;
	    if (cansee(mon->mx, mon->my))
		pline("%s seems healthier.", Monnam(mon));
	}
    }
}

/* metroid is hit by a death ray and splits off more metroids */
void
bud_metroid(mon)
struct monst * mon;
{
	if (mon->mtyp == PM_METROID){
		if (canseemon(mon))
			pline("The metroid is irradiated with pure energy!  It divides!");
		makemon(&mons[PM_METROID], mon->mx, mon->my, MM_ADJACENTOK);
	}
	else if (mon->mtyp == PM_ALPHA_METROID || mon->mtyp == PM_GAMMA_METROID){
		if (canseemon(mon))
			pline("The metroid is irradiated with pure energy!  It buds off a baby metroid!");
		makemon(&mons[PM_BABY_METROID], mon->mx, mon->my, MM_ADJACENTOK);
	}
	else if (mon->mtyp == PM_ZETA_METROID || mon->mtyp == PM_OMEGA_METROID){
		if (canseemon(mon))
			pline("The metroid is irradiated with pure energy!  It buds off another metroid!");
		makemon(&mons[PM_METROID], mon->mx, mon->my, MM_ADJACENTOK);
	}
	else if (mon->mtyp == PM_METROID_QUEEN){
		if (canseemon(mon))
			pline("The metroid queen is irradiated with pure energy!  She buds off more metroids!");
		makemon(&mons[PM_METROID], mon->mx, mon->my, MM_ADJACENTOK);
		makemon(&mons[PM_METROID], mon->mx, mon->my, MM_ADJACENTOK);
		makemon(&mons[PM_METROID], mon->mx, mon->my, MM_ADJACENTOK);
	}
	return;
}

boolean
angry_guards(silent)
register boolean silent;
{
	register struct monst *mtmp;
	register int ct = 0, nct = 0, sct = 0, slct = 0;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if (DEADMONSTER(mtmp)) continue;
		if((mtmp->mtyp == PM_WATCHMAN ||
			       mtmp->mtyp == PM_WATCH_CAPTAIN)
					&& mtmp->mpeaceful && !mtmp->mtame) {
			ct++;
			if(cansee(mtmp->mx, mtmp->my) && mtmp->mcanmove) {
				if (distu(mtmp->mx, mtmp->my) == 2) nct++;
				else sct++;
			}
			if (mtmp->msleeping || mtmp->mfrozen) {
				slct++;
				mtmp->msleeping = mtmp->mfrozen = 0;
			}
			mtmp->mpeaceful = 0;
		}
	}
	if(ct) {
	    if(!silent) { /* do we want pline msgs? */
		if(slct) pline_The("guard%s wake%s up!",
				 slct > 1 ? "s" : "", slct == 1 ? "s" : "");
		if(nct || sct) {
			if(nct) pline_The("guard%s get%s angry!",
				nct == 1 ? "" : "s", nct == 1 ? "s" : "");
			else if(!Blind)
				You("see %sangry guard%s approaching!",
				  sct == 1 ? "an " : "", sct > 1 ? "s" : "");
		} else if(flags.soundok)
			You_hear("the shrill sound of a guard's whistle.");
	    }
	    return(TRUE);
	}
	return(FALSE);
}

void
pacify_guards()
{
	register struct monst *mtmp;

	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (DEADMONSTER(mtmp)) continue;
	    if (mtmp->mtyp == PM_WATCHMAN ||
		mtmp->mtyp == PM_WATCH_CAPTAIN)
	    mtmp->mpeaceful = 1;
	}
}

void
mimic_hit_msg(mtmp, otyp)
struct monst *mtmp;
short otyp;
{
	short ap = mtmp->mappearance;

	switch(mtmp->m_ap_type) {
	    case M_AP_NOTHING:			
	    case M_AP_FURNITURE:
	    case M_AP_MONSTER:
		break;
	    case M_AP_OBJECT:
		if (otyp == SPE_HEALING || otyp == SPE_EXTRA_HEALING || otyp == SPE_FULL_HEALING) {
		    pline("%s seems a more vivid %s than before.",
				The(simple_typename(ap)),
				c_obj_colors[objects[ap].oc_color]);
		}
		break;
	}
}

int
u_sanity_loss(mtmp)
struct monst *mtmp;
{
	int mndx = monsndx(mtmp->data);
	
	
	if(save_vs_sanloss()){
		if(mndx == PM_GREAT_CTHULHU)
			return -1*rnd(10);
		else return -1*(max_ints(0, (monstr[mndx]-u.ulevel)/4) + rnd(max(1, (monstr[mndx]-u.ulevel)/4)));
	} else {
		if(mndx == PM_GREAT_CTHULHU)
			return -1*rnd(100);
		else return -1*rnd(monstr[mndx]);
	}
}

int
u_sanity_loss_minor(mtmp)
struct monst *mtmp;
{
	int mndx = monsndx(mtmp->data);
	int diesize, sanloss;
	
	if(!monstr[mndx])
		return 0;

	if(monstr[mndx] <= 3){
		diesize = monstr[mndx];
	}
	else if(monstr[mndx] <= 6){
		diesize = 4;
	}
	else if(monstr[mndx] <= 10){
		diesize = 6;
	}
	else if(monstr[mndx] <= 20){
		diesize = 8;
	}
	else if(monstr[mndx] <= 30){
		diesize = 10;
	}
	else {
		diesize = 12;
	}
	
	sanloss = rnd(diesize);
	
	if(save_vs_sanloss()){
		return -1*sanloss/3;
	} else {
		return -1*sanloss;
	}
}

int
u_sanity_loss_nyar()
{
	/* Nitocris's wrappings are specially warded vs. Nyarlathotep, but he can still break them. */
	/* If this happens the shock ensures hefty san loss. */
	if(uarmc && uarmc->oartifact == ART_SPELL_WARDED_WRAPPINGS_OF_){
		if (rn2(3)){
			if(uarmc->oeroded3){
				Your("wrappings rip to shreds!");
				useup(uarmc);
				return -50 - rnd(50);
			} else {
				uarmc->oeroded3 = 1;
				Your("wrappings fray, but hold!");
				return -5 - rnd(5);
			}
		}
		else return 0;
	}

	if(save_vs_sanloss()){
		return -1*rnd(10);
	} else {
		return -1*rnd(100);
	}
}

int
u_sanity_gain(mtmp)
struct monst *mtmp;
{
	int mndx = monsndx(mtmp->data);
	
	
	if((uwep && uwep->oartifact == ART_HOLY_MOONLIGHT_SWORD)
		|| (rnd(30) < ACURR(A_WIS))
	){
		return max(1, monstr[mndx]/10);
	}
	else return 0;
}

int
u_insight_gain(mtmp)
struct monst *mtmp;
{
	int mndx = monsndx(mtmp->data);
	int insight = monstr[mndx]/10;
	
	//One point given for seeing it
	if(insight > 1)
		insight--;
	
	return max(1, insight);
}

int
u_visible_insight(mtmp)
struct monst *mtmp;
{
	int mndx = monsndx(mtmp->data);
	int insight = monstr[mndx]/10;
	
	if(insight > 1)
		return 1;
	return 0;
}

STATIC_OVL void
dead_familiar(id)
long id;
{
	struct monst *mtmp;
	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		if(id == (long)mtmp->m_id){
			if(mtmp->mtyp == PM_COVEN_LEADER){
				pline("%s screams in rage!", Monnam(mtmp));
				mtmp->mspec_used = 4;
				mtmp->encouraged += 8;
			} else {
				pline("%s screams in dismay!", Monnam(mtmp));
				monflee(mtmp, 0, FALSE, TRUE);
				if(mtmp->mtyp == PM_WITCH)
					mtmp->mspec_used = 10;
			}
		}
}

void
repair(mech, targ, verbose)
struct monst *mech, *targ;
boolean verbose;
{
	int * hp, *max;
	if(verbose){
		if(mech == targ)
			pline("%s repairs %sself.", Monnam(mech), himherit(targ));
		else if(targ == &youmonst)
			pline("%s repairs you.", Monnam(mech));
		else
			pline("%s repairs %s.", Monnam(mech), mon_nam(targ));
	}
	if(targ == &youmonst){
		if(Upolyd){
			hp = &u.mh;
			max = &u.mhmax;
		} else {
			hp = &u.uhp;
			max = &u.uhpmax;
		}
	} else {
		hp = &(targ->mhp);
		max = &(targ->mhpmax);
	}
	*hp += mech->m_lev;
	if(*hp > *max)
		*hp = *max;
}

void
nurse_heal(nurse, targ, verbose)
struct monst *nurse, *targ;
boolean verbose;
{
	int *hp, *max;
	int healing = 0;
	if(verbose){
		if(nurse == targ)
			pline("%s heals %sself.", Monnam(nurse), himherit(targ));
		else if(targ == &youmonst)
			pline("%s heals you.", Monnam(nurse));
		else
			pline("%s heals %s.", Monnam(nurse), mon_nam(targ));
	}
	if(targ == &youmonst){
		if(Upolyd){
			hp = &u.mh;
			max = &u.mhmax;
		} else {
			hp = &u.uhp;
			max = &u.uhpmax;
		}
	} else {
		hp = &(targ->mhp);
		max = &(targ->mhpmax);
	}
	healing += d(2+max(0, nurse->m_lev - 9)/3,6); //Note, nurses start at 11th level, healers 10th.
	if(targ == &youmonst){
		healing -= roll_udr(nurse, UPPER_TORSO_DR);
	} else {
		healing -= roll_mdr(targ, nurse, UPPER_TORSO_DR);
	}
	if(healing < 1)
		healing = 1;
	*hp += healing;
	if(*hp > *max)
		*hp = *max;
}

void
itiner_heal(priestess, targ, verbose)
struct monst *priestess, *targ;
boolean verbose;
{
	int *hp, *max;
	int healing = 0;
	if(verbose){
		if(priestess == targ)
			pline("%s heals %sself.", Monnam(priestess), himherit(targ));
		else if(targ == &youmonst)
			pline("%s touches you.", Monnam(priestess));
		else
			pline("%s touches %s.", Monnam(priestess), mon_nam(targ));
	}
	if(targ == &youmonst){
		if(Upolyd){
			hp = &u.mh;
			max = &u.mhmax;
		} else {
			hp = &u.uhp;
			max = &u.uhpmax;
		}
	} else {
		hp = &(targ->mhp);
		max = &(targ->mhpmax);
	}
	healing += d(2+max(0, priestess->m_lev - 9)/3,3); //Note, nurses start at 11th level, healers 10th.
	if(hates_unholy_mon(targ))
		healing *= 2;
	*hp += healing;
	if(*hp > *max)
		*hp = *max;
}

void
insight_vanish(mtmp)
struct monst *mtmp;
{
	if(DEADMONSTER(mtmp))
		return;
	if(mtmp->mtyp == PM_LIVING_DOLL){
		mondied(mtmp);
	} else {
		migrate_to_level(mtmp, ledger_no(&u.uz), MIGR_EXACT_XY, (coord *)0);
	}
}

STATIC_OVL void
mark_item_chain_summoned(struct obj *otmp, struct monst *mon, int duration)
{
	for (; otmp; otmp = otmp->nobj) {
		if(otmp->cobj)
			mark_item_chain_summoned(otmp->cobj, mon, duration);
		if (!get_ox(otmp, OX_ESUM)) {
			/* add component to obj */
			add_ox(otmp, OX_ESUM);
			otmp->oextra_p->esum_p->summoner = mon;
			otmp->oextra_p->esum_p->sm_id = mon->m_id;
			otmp->oextra_p->esum_p->sm_o_id = 0;
			otmp->oextra_p->esum_p->summonstr = 0;
			otmp->oextra_p->esum_p->sticky = 0;
			otmp->oextra_p->esum_p->permanent = (duration == ESUMMON_PERMANENT);
			otmp->oextra_p->esum_p->staleptr = 0;
			/* add timer to obj */
			start_timer(duration, TIMER_OBJECT, DESUMMON_OBJ, (genericptr_t)otmp);
		}
		else {
			/* already marked as summoned -- double-check it's the right mon */
			if (otmp->oextra_p->esum_p->summoner != mon)
				impossible("%s already attached to %s, cannot attach to %s",
					xname(otmp), m_monnam(otmp->oextra_p->esum_p->summoner), m_monnam(mon));
			else {
				/* change duration, if applicable */
				if (duration != ESUMMON_PERMANENT) {
					otmp->oextra_p->esum_p->permanent = 0;
					adjust_timer_duration(get_timer(otmp->timed, DESUMMON_OBJ), duration - ESUMMON_PERMANENT);
				}
			}
		}
	}
}

/* marks `mon` as being summoned by the summoner, which causes it to vanish after duration expires or summoner dies */
/* its inventory at time of marking is set to vanish when `mon` dies */
void
mark_mon_as_summoned(mon, summoner, duration, flags)
struct monst * mon;
struct monst * summoner;
int duration;
int flags;
{
	/* check that mon was set up to be a summon */
	if (!get_mx(mon, MX_ESUM)) {
		impossible("%s not made as summon, fixing", m_monnam(mon));
		add_mx(mon, MX_ESUM);
		start_timer(ESUMMON_PERMANENT, TIMER_MONSTER, DESUMMON_MON, (genericptr_t)mon);
	}
	/* sanity-check */
	if (summoner && DEADMONSTER(summoner)) {
		impossible("summoner (%s) is dead?", m_monnam(summoner));
	}
	/* set data */
	mon->mextra_p->esum_p->summoner = summoner;
	mon->mextra_p->esum_p->sm_id = summoner ? summoner->m_id : 0;
	mon->mextra_p->esum_p->sm_o_id = 0;
	mon->mextra_p->esum_p->summonstr = mon->data->mlevel;
	mon->mextra_p->esum_p->sticky = (!summoner || summoner == &youmonst) && !(flags & ESUMMON_NOFOLLOW);
	mon->mextra_p->esum_p->permanent = (duration == ESUMMON_PERMANENT);
	mon->mextra_p->esum_p->staleptr = 0;
	/* change duration, if applicable */
	if (duration != ESUMMON_PERMANENT) {
		mon->mextra_p->esum_p->permanent = 0;
		adjust_timer_duration(get_timer(mon->timed, DESUMMON_MON), duration - ESUMMON_PERMANENT);
	}
	if (summoner)
		summoner->summonpwr += mon->mextra_p->esum_p->summonstr;
	// mark mon's inventory as summoned, including contained objects
#ifndef GOLDOBJ
	u.spawnedGold -= mon->mgold;
	mon->mgold = 0;
#else
	{
		struct obj *mongold = findgold(mon->minvent);
		if (mongold) {
			u.spawnedGold -= mongold->quan;
			obj_extract_self(mongold);
			obfree(mongold, (struct obj *)0);
		}
	}
#endif
	mark_item_chain_summoned(mon->minvent, mon, duration);
}

struct monst *
mtyp_on_level(mtyp)
int mtyp;
{
	for(struct monst *mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		if(mtmp->mtyp == mtyp)
			return mtmp;
	return (struct monst *) 0;
}

struct monst *
mtyp_on_migrating(mtyp)
int mtyp;
{
	for(struct monst *mtmp = migrating_mons; mtmp; mtmp = mtmp->nmon)
		if(mtmp->mtyp == mtyp)
			return mtmp;
	return (struct monst *) 0;
}

STATIC_OVL boolean
has_blessings(mtmp)
struct monst *mtmp;
{
	struct obj *otmp;

	for(otmp = (mtmp == &youmonst) ? invent : mtmp->minvent; otmp; otmp = otmp->nobj){
		if(otmp->blessed) return TRUE;
	}
	//else no blessed items

	if(mtmp == &youmonst){
		if(u.uluck > -7 || u.ublessed > -7 || u.uhitinc > 0 || u.udaminc > 0 || u.ucarinc > 0)
			return TRUE;
	}
	else {
		if(mtmp->encouraged > -7)
			return TRUE;
	}
	
	return FALSE;
}

STATIC_OVL int
blessings_consume(magr, mdef)
struct monst *magr;
struct monst *mdef;
{
	struct obj *otmp, *objchain;
	int remaining = min(10, (magr->m_lev)/3), damage = min(10, (magr->m_lev)/3), candidates = 0;
	int damaged = 0;
	/* Why do we do all integer probabilities again? >_< */
	const int smoothing_factor = 100000;

	damage = reduce_dmg(mdef,damage,FALSE,TRUE);
	
	objchain = (mdef == &youmonst) ? invent : mdef->minvent;

	for(otmp = objchain; otmp; otmp = otmp->nobj)
		if(otmp->blessed)
			candidates++;
	//Note: May be more desired targets than actual candidates, this is fine.
	
	if(candidates){
		int prob = (remaining*smoothing_factor)/candidates;
		for(otmp = objchain; otmp && remaining; otmp = otmp->nobj){
			// pline("targets: %d candidates: %d prob: %d", remaining, candidates, prob);
			if(otmp->blessed){
				if(rnd(smoothing_factor) <= prob){
					curse(otmp);
					damaged++;
					remaining--;
				}
				candidates--;
				if(!remaining || !candidates)
					return damaged;
				prob = (remaining*smoothing_factor)/candidates;
			}
		}
	}

#define otherBlessing(blessing, i)\
	if(blessing > i){\
		update = max(-remaining, i - blessing);\
		blessing += update;\
		remaining += update;\
		damaged += -update;\
		if(remaining < 1)\
			return damaged;\
	}
	
	if(remaining){
		char update;
		if(mdef == &youmonst){
			// if(u.uluck > -7){
				// update = max(-remaining, -7 - u.uluck);
				// u.uluck += update;
				// remaining += update;
				// damaged += -update;
				// if(remaining < 1)
					// return damaged;
			// }
			otherBlessing(u.uluck, -7)
			otherBlessing(u.ublessed, -7)
			otherBlessing(u.uhitinc, 0)
			otherBlessing(u.udaminc, 0)
			if(u.ucarinc > 0){
				update = max(-remaining*10, 0 - u.ucarinc);\
				u.ucarinc += update;
				remaining += update/10;
				damaged += -update/10;
				if(remaining < 1)
					return damaged;
			}
		}
		else {
			otherBlessing(mdef->encouraged, -7)
		}
	}

	if(damaged){
		if(mdef == &youmonst){
			if(!Blind){
				pline("Tiny %s sparks rise around you.", hcolor(NH_LIGHT_BLUE));
				if(canseemon(magr)){
					pline("The sparks are drawn into %s toothless mouth!", s_suffix(mon_nam(magr)));
				}
			}
		}
		else {
			if(!Blind){
				if(canseemon(mdef)){
					pline("Tiny %s sparks rise around %s.", hcolor(NH_LIGHT_BLUE), mon_nam(mdef));
					if(canseemon(magr)){
						pline("The sparks are drawn into %s toothless mouth!", s_suffix(mon_nam(magr)));
					}
				}
				else if(canseemon(magr)){
					pline("Tiny %s sparks are drawn into %s toothless mouth!", hcolor(NH_LIGHT_BLUE), s_suffix(mon_nam(magr)));
				}
			}
		}
	}

	return damaged;
}

STATIC_OVL boolean
has_organic(mtmp)
struct monst *mtmp;
{
	struct obj *otmp;
	otmp = (mtmp == &youmonst) ? invent : mtmp->minvent;
	
	if(!otmp) return FALSE;
	
	do{
		if(is_organic(otmp)) return TRUE;
	} while((otmp = otmp->nobj));
	
	return FALSE;
}

void
held_item_bites(mdef, obj)
struct monst *mdef;
struct obj *obj;
{
	int nslots = 0;
	long slotvar = 0;
	long sloti = 0;
	int nd;
	int dd;
	int damage;
	boolean vis = mdef != &youmonst && canseemon(mdef);
	if(obj->owornmask){
		/*Count raised bits*/
		if(objects[obj->otyp].oc_class == ARMOR_CLASS)
			slotvar = objects[obj->otyp].oc_dtyp;
		
		if(!slotvar)
			slotvar = default_DR_slot(obj->owornmask);
		
		if(obj->owornmask == W_SWAPWEP && mdef == &youmonst && !u.twoweap)
			slotvar = LOWER_TORSO_DR;

		for(sloti = slotvar; sloti; sloti = sloti >> 1)
			if(sloti&0x1L)
				nslots++;

		/*pick a slot*/
		if(nslots > 1){
			int i = 0;
			nslots = rnd(nslots);
			sloti = slotvar;
			//In practice, this loop should be broken with break;
			while(sloti){
				if(sloti&0x1L)
					nslots--;
				if(!nslots)
					break;
				i++;
				sloti = sloti >> 1;
			}
			slotvar = 0x1L << i;
		}
	}
	
	if(obj->olarva){
		dd = 1 + obj->olarva;
		nd = max(1, (objects[obj->otyp].oc_size + obj->objsize - MZ_MEDIUM));
		
		damage = d(nd,dd);
		if(mdef == &youmonst){
			Your("%s bite%s you!", xname(obj), obj->quan == 1 ? "s":"");
			// pline("damage pre DR: %d, slotvar: %ld, wornmask: %ld", damage, slotvar, obj->owornmask);
			damage -= roll_udr_detail((struct monst *)0, slotvar, (is_dress(obj->otyp) && slotvar != UPPER_TORSO_DR) ? W_ARM : obj->owornmask, ROLL_SLOT);
			// pline("damage post DR: %d", damage);
			if(damage < 1)
				damage = 1;
			damage = reduce_dmg(mdef,damage,TRUE,FALSE);
		
			losehp(damage, "their clothes", KILLED_BY);
		}
		else {
			// pline("damage pre DR: %d", damage);
			damage -= roll_mdr_detail(mdef, (struct monst *)0, slotvar, (is_dress(obj->otyp) && slotvar != UPPER_TORSO_DR) ? W_ARM : obj->owornmask, ROLL_SLOT);
			// pline("damage post DR: %d", damage);
			if(damage < 1)
				damage = 1;

			damage = reduce_dmg(mdef,damage,TRUE,FALSE);

			mdef->mhp -= damage;
			if(mdef->mhp < 1){
				pline("%s is killed by %s %s!", Monnam(mdef), mhis(mdef), simple_typename(obj->otyp));
				mondied(mdef);
			}
			else {
				static long mid = 0, move = 0;
				
				if(mdef->m_id != mid || move != monstermoves){
					pline("%s is bitten by %s clothes!", Monnam(mdef), mhis(mdef));
					mid = mdef->m_id;
					move = monstermoves;
				}
			}
		}
	}
	if((obj->obyak || check_oprop(obj, OPROP_BYAK)) && (mdef == &youmonst || !yellow_monster(mdef))){
		nd = max(1, (objects[obj->otyp].oc_size + obj->objsize - MZ_MEDIUM));
		nd *= check_oprop(obj, OPROP_BYAK) ? 3 : obj->obyak;
		
		damage = d(nd,2) + nd*2;
		if(mdef == &youmonst){
			Your("%s bite%s and sting%s you!", xname(obj), obj->quan == 1 ? "s":"", obj->quan == 1 ? "s":"");
			// pline("damage pre DR: %d, slotvar: %ld, wornmask: %ld", damage, slotvar, obj->owornmask);
			damage -= roll_udr_detail((struct monst *)0, slotvar, (is_dress(obj->otyp) && slotvar != UPPER_TORSO_DR) ? W_ARM : obj->owornmask, ROLL_SLOT);
			// pline("damage post DR: %d", damage);
			if(damage < 1)
				damage = 1;
			damage = reduce_dmg(mdef,damage,TRUE,FALSE);
		
			losehp(damage, "their clothes", KILLED_BY);
		}
		else {
			// pline("damage pre DR: %d", damage);
			damage -= roll_mdr_detail(mdef, (struct monst *)0, slotvar, (is_dress(obj->otyp) && slotvar != UPPER_TORSO_DR) ? W_ARM : obj->owornmask, ROLL_SLOT);
			// pline("damage post DR: %d", damage);
			if(damage < 1)
				damage = 1;

			damage = reduce_dmg(mdef,damage,TRUE,FALSE);

			mdef->mhp -= damage;
			if(mdef->mhp < 1){
				pline("%s is killed by %s %s!", Monnam(mdef), mhis(mdef), simple_typename(obj->otyp));
				mondied(mdef);
			}
			else {
				pline("%s is stung by %s %s!", Monnam(mdef), mhis(mdef), simple_typename(obj->otyp));
			}
		}
	}
}

void
add_byakhee_to_obj(obj)
struct obj *obj;
{
	if(obj->obyak < 3){
		if(!obj->olarva && !obj->obyak) start_timer(4+d(2,4), TIMER_OBJECT, LARVAE_DIE, (genericptr_t)obj);
		obj->obyak++;
	}
}

STATIC_OVL boolean
obj_vitality_damage(obj)
struct obj *obj;
{
	boolean floor = (obj->where == OBJ_FLOOR), youdef;
	struct monst *mon;
	struct monst *mtmp;
	struct obj *obj2;
	int nlarvae;
	int x, y;
	mon = (obj->where == OBJ_MINVENT) ? obj->ocarry : (obj->where == OBJ_INVENT) ? &youmonst : (struct monst *) 0;
	youdef = (obj->where == OBJ_INVENT);

	if(!floor && !mon){
		impossible("unhandled object location in obj_vitality_damage()");
		return FALSE;
	}

	if(obj_resists(obj, 5, 95)) return FALSE;

	if(floor){
		x = obj->ox;
		y = obj->oy;
	} else if(youdef){
		x = u.ux;
		y = u.uy;
	} else {
		x = mon->mx;
		y = mon->my;
	}

	nlarvae = max(1, (objects[obj->otyp].oc_size + obj->objsize - MZ_MEDIUM));
	
	if(objects[obj->otyp].oc_merge){
		if(!Blind){
			if(floor){
				if(cansee(x, y)) pline("One of %s is riddled with strange larvae!", the(xname(obj)));
			}
			else if(obj->owornmask){
				if(youdef){
					pline("One of your %s is riddled with strange larvae!", xname(obj));
				} else {
					if(canseemon(mon)) pline("One of %s %s is riddled with strange larvae!", s_suffix(mon_nam(mon)), xname(obj));
				}
			}
			else if(youdef){
				pline(nlarvae > 1 ? "Strange larvae drop out of your pack!" : "A strange larva drops out of your pack!");
			}
		}
		else {
			if(youdef && obj->owornmask) pline("One of your %s is riddled with squirming things!", xname(obj));
		}
		if(!obj->oartifact){
			if(obj->quan > 1){
				obj->quan--;
				fix_object(obj);
			}
			else {
				obj_extract_and_unequip_self(obj);
				obfree(obj, (struct obj *)0);
			}
		}
	}
	else {
		if(!Blind){
			if(floor && obj->olarva == 3 && !obj->oartifact){
				if(cansee(x, y)) pline("%s disintegrate%s into a mass of strange larvae!", An(xname(obj)), obj->quan == 1 ? "s":"");
			}
			else if(obj->owornmask){
				if(youdef){
					if(obj->olarva == 3 && !obj->oartifact)
						pline("Your %s disintegrate%s into a mass of strange larvae!", xname(obj), obj->quan == 1 ? "s":"");
					else if(obj->olarva > 0)
						pline("The biting mouths in your %s grow more numerous!", xname(obj));
					else
						pline("Your %s %s suddenly full of biting mouths!", xname(obj), obj->quan == 1 ? "is":"are");
				} else {
					if(canseemon(mon) && obj->olarva == 3) pline("%s %s disintegrate%s into a mass of strange larvae!", s_suffix(Monnam(mon)), xname(obj), obj->quan == 1 ? "s":"");
				}
			}
			else if(youdef && obj->olarva == 3 && !obj->oartifact){
				pline(nlarvae > 1 ? "Strange larvae drop out of your pack!" : "A strange larva drops out of your pack!");
			}
		}
		else {
			if(youdef && obj->owornmask){
				if(obj->olarva == 0) pline("Your %s is riddled with biting things!", xname(obj));
				else if(obj->olarva == 3 && !obj->oartifact) pline("Your %s disintegrates into a mass of squirming things!", xname(obj));
			}
		}
		if(obj->olarva < 3){
			if(!obj->olarva && !obj->obyak) start_timer(4+d(2,4), TIMER_OBJECT, LARVAE_DIE, (genericptr_t)obj);
			obj->olarva++;
			return TRUE;
		}
		else if(obj->oartifact)
			return TRUE;
		else {
			obj_extract_and_unequip_self(obj);
			while((obj2 = obj->cobj)){
				obj_extract_self(obj2);
				/* Compartmentalize tip() */
				if(floor){
					place_object(obj2, x, y);
					stackobj(obj2);
				} else if(youdef){
					dropy(obj2);
				} else {
					mdrop_obj(mon, obj2, FALSE);
				}
			}
			obfree(obj, (struct obj *)0);
		}
	}
	//May have returned in the previous block
	for(; nlarvae; nlarvae--){
		mtmp = makemon(&mons[PM_STRANGE_LARVA], x, y, MM_ADJACENTOK|NO_MINVENT|MM_NOCOUNTBIRTH);
		if(mtmp)
			mtmp->mvar_tanninType = PM_ANCIENT_NUPPERIBO;
	}
	return TRUE;
}

STATIC_OVL int
vitality_consume(origin, objchain, owner)
struct monst *origin;
struct obj *objchain;
struct monst *owner;
{
	struct obj *otmp, *nobj;
	int remaining = min(10, (origin->m_lev)/3), damage = min(10, (origin->m_lev)/3), candidates = 0;
	int damaged = 0;
	/* Why do we do all integer probabilities again? >_< */
	const int smoothing_factor = 100000;
	
	if(owner)
		damage = reduce_dmg(owner,damage,FALSE,TRUE);

	/* &0x1 means odd */
	if(remaining > 2)
		remaining = (remaining&0x1) ? (rnd(remaining/2) + rnd(remaining/2+1)) : d(2, remaining/2);
	
	for(otmp = objchain; otmp; otmp = otmp->nobj)
		if(is_organic(otmp))
			candidates++;
	
	if(remaining > candidates)
		remaining = candidates;

	if(owner){
		// Black flecks rise from 
		if(owner == &youmonst){
			if(!Blind){
				pline("Flakes of %s ash rise around you.", hcolor("black"));
				if(canseemon(origin)){
					pline("The ash is drawn into %s apical bloom!", s_suffix(mon_nam(origin)));
				}
			}
		}
		else {
			if(!Blind){
				if(canseemon(owner)){
					pline("Flakes of %s ash rise around %s.", hcolor("black"), mon_nam(owner));
					if(canseemon(origin)){
						pline("The ash is drawn into %s apical bloom!", s_suffix(mon_nam(origin)));
					}
				}
				else if(canseemon(origin)){
					pline("Flakes of %s ash are drawn into %s apical bloom!", hcolor("black"), s_suffix(mon_nam(origin)));
				}
			}
		}
	}

	if(candidates){
		int prob = (remaining*smoothing_factor)/candidates;
		for(otmp = objchain; otmp && remaining; otmp = nobj){
			// pline("targets: %d candidates: %d prob: %d", remaining, candidates, prob);
			nobj = otmp->nobj;
			if(is_organic(otmp)){
				if(rnd(smoothing_factor) <= prob){
					if(obj_vitality_damage(otmp)){
						damaged++;
					}
					remaining--;
				}
				candidates--;
				if(!remaining || !candidates)
					return damaged;
				prob = (remaining*smoothing_factor)/candidates;
			}
		}
	}
	return damaged;
}

STATIC_OVL void
vitality_overload(magr, mdef, damage)
struct monst *magr, *mdef;
int damage;
{
	struct monst *mtmp;
	if(canseemon(magr))
		pline("Waves of light roll from %s petals!", s_suffix(mon_nam(magr)));
	if(mdef == &youmonst){
		if(Blind)
			You_feel("like you're burning from within!");
		else
			Your("%s glows brilliant %s!", body_part(BODY_SKIN), hcolor("white"));
		for(; damage; damage--){
			mtmp = makemon(&mons[PM_BEAUTEOUS_ONE], u.ux, u.uy, MM_ADJACENTOK|NO_MINVENT|MM_NOCOUNTBIRTH);
			if(mtmp){
				mtmp->mpeaceful = magr->mpeaceful;
				set_malign(mtmp);
				mtmp->m_lev = u.ulevel;
				mtmp->mhpmax = max(4, 8*mtmp->m_lev);
				mtmp->mhp = mtmp->mhpmax;
			}
			losexp("overflowing vital force",!damage,TRUE,FALSE);
		}
	}
	else{
		if(canseemon(mdef))
			pline("%s %s glows brilliant %s!", s_suffix(Monnam(mdef)), mbodypart(mdef, BODY_SKIN), hcolor("white"));
		for(; damage && !DEADMONSTER(mdef); damage--){
			mtmp = makemon(&mons[PM_BEAUTEOUS_ONE], mdef->mx, mdef->my, MM_ADJACENTOK|NO_MINVENT|MM_NOCOUNTBIRTH);
			if(mtmp){
				mtmp->mpeaceful = magr->mpeaceful;
				set_malign(mtmp);
				mtmp->m_lev = mdef->m_lev;
				mtmp->mhpmax = max(4, 8*mtmp->m_lev);
				mtmp->mhp = mtmp->mhpmax;
			}
			if(mdef->m_lev){
				mdef->mhp -= mdef->mhpmax/mdef->m_lev;
				mdef->mhpmax -= mdef->mhpmax/mdef->m_lev;
				mdef->m_lev--;
				mdef->mhp = max(1, mdef->mhp);
				mdef->mhpmax = max(1, mdef->mhpmax);
			}
			else {
				mdef->mhp = 0;
				mondied(mdef);
			}
		}
	}
}

STATIC_OVL void
thought_scream_side_effects(origin, primary, damage)
struct monst *origin;
struct monst *primary;
int damage;
{
	struct monst *m2, *nmon;
	for(m2=fmon; m2; m2 = nmon) {
		nmon = m2->nmon;
		if (DEADMONSTER(m2)) continue;
		if (m2->mpeaceful == origin->mpeaceful) continue;
		if (mindless_mon(m2)) continue;
		if (m2 == origin) continue;
		if (m2 == primary) continue;
		if (species_is_telepathic(m2->data) ||
			(is_law_demon(m2->data) && !is_ancient(m2->data) && m2->mtyp != PM_ASMODEUS) ||
			(mon_resistance(m2,TELEPAT) &&
			(rn2(2) || m2->mblinded)) || 
			!rn2(10)
		) {
			m2->mhp -= d(damage,7);
			if (m2->mhp <= 0)
				monkilled(m2, "", AD_DRIN);
			else{
				m2->msleeping = 0;
				monflee(m2, damage*5, FALSE, TRUE);
			}
		}
	}
	if(primary != &youmonst && !origin->mpeaceful){
		if (!Tele_blind && (tp_sensemon(origin) || (Blind_telepat && rn2(2)) || !rn2(10))) {
			int dmg;
			//Note: You "hear" it in your mind, not with your ears, so You_hear() is wrong.
			You("hear a terrible scream!");
			dmg = d(damage,7);
			losehp(dmg, "the scream of an old one", KILLED_BY);
			if(rn2(100) >= NightmareAware_Sanity){
				if(!Panicking)
					You("panic!");
				HPanicking += damage*2;
			}
			nomul(0, NULL); //Interrupt
		}
	}
}

void
do_auras(mon)
struct monst *mon;
{
	if(mon->mtyp == PM_VERMIURGE){
		int distance = 0, damage = 0;
		static const int dice[] = {10,10,9,8,6,2};
		if(!mon->mcan)
			mon->mvar_vermiurge += 100;
		if(mon->mvar_vermiurge > 1000)
			mon->mvar_vermiurge = 1000;
		for(struct monst *tmpm = fmon; tmpm && mon->mvar_vermiurge > 0; tmpm = tmpm->nmon){
			if(!DEADMONSTER(tmpm)
				&& tmpm->mtyp != PM_VERMIURGE
				&& (tmpm->mpeaceful != mon->mpeaceful || mm_aggression(mon, tmpm))
				&& clear_path(tmpm->mx,tmpm->my,mon->mx,mon->my)
				&& !nonthreat(tmpm)
			){
				distance = dist2(tmpm->mx,tmpm->my,mon->mx,mon->my);
				distance = (int)sqrt(distance);
				//0, 1, 2, 3, 4, 5
				//0, 0, 1, 2, 4, 8
				if(distance < 6){
					damage = d(dice[distance],10);
					if(damage > mon->mvar_vermiurge)
						damage = mon->mvar_vermiurge;
					damage -= avg_mdr(tmpm);
					if(damage > 0){
						damage = min(damage, tmpm->mhp);
						mon->mvar_vermiurge -= damage;
						//Reduce after charging vermin
						damage = reduce_dmg(tmpm, damage, TRUE, FALSE);
						if(canspotmon(tmpm)){
							pline("%s is stung%s by swarming vermin!",
								Monnam(tmpm),
								damage >= tmpm->mhp ? " to death" : ""
							);
						}
						tmpm->mhp -= damage;
						if(tmpm->mhp <= 0){
							grow_up(mon,tmpm);
							mondied(tmpm);
						}
					}
					// else continue
				}
				// else continue
			}
			// else continue
		}
		if(!mon->mpeaceful && couldsee(mon->mx,mon->my)){
			distance = dist2(u.ux,u.uy,mon->mx,mon->my);
			distance = (int)sqrt(distance);
			//0, 1, 2, 3, 4, 5
			//0, 0, 1, 2, 4, 8
			if(distance < 6){
				damage = d(dice[distance],10);
				if(damage > mon->mvar_vermiurge)
					damage = mon->mvar_vermiurge;
				damage -= u.udr;
				if(damage > 0){
					damage = min(damage, uhp());
					mon->mvar_vermiurge -= damage;
					//Reduce after charging vermin
					damage = reduce_dmg(&youmonst, damage, TRUE, FALSE);
					You("are stung by swarming vermin!");
					losehp(damage,"swarming vermin",KILLED_BY);
				}
				// else continue
			}
			// else continue
		}
	}
	if(mon->mtyp == PM_ALKILITH){
		struct monst *mtmp;
		int xlocale = mon->mx, ylocale = mon->my;
		for(mtmp = fmon; mtmp; mtmp = mtmp->nmon){
			if(mon->mpeaceful && mtmp->mpeaceful)
				continue;
			if(hates_holy_mon(mtmp) || taxes_sanity(mtmp->data))
				continue;
			if(!mtmp->mconf && dist2(xlocale, ylocale, mtmp->mx, mtmp->my) <= 36){
				if(!mm_resist(mtmp, mtmp, 0, FALSE)){
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
					if(canspotmon(mon))
						You_hear("%s buzzing!", mon_nam(mon));
					else You_hear("a strange buzzing!");
					change_usanity(-1, !save_vs_sanloss()); //Second save to avoid minor madness check
				}
			}
			if(!rn2(66)){
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
	if(mon->mtyp == PM_CHORISTER_JELLY){
		struct monst *mtmp;
		int xlocale = mon->mx, ylocale = mon->my;
		for(mtmp = fmon; mtmp; mtmp = mtmp->nmon){
			if(mon->mhp < mon->mhpmax && mon->mpeaceful == mtmp->mpeaceful && !mm_aggression(mtmp, mon))
				mon->mhp++;
		}
		if(mon->mpeaceful){
			if(dist2(xlocale, ylocale, u.ux, u.uy) <= 64){
				if(Insanity > 10 && !save_vs_sanloss()){
					if(canspotmon(mon))
						You_hear("%s strange chime!", s_suffix(mon_nam(mon)));
					else You_hear("a strange chiming!");
					change_usanity(-1, !save_vs_sanloss()); //Second save to avoid minor madness check
				}
				else {
					static long lastheard = 0L;
					if(lastheard + 10 < moves){
						if(canspotmon(mon))
							You_hear("%s chime.", mon_nam(mon));
						else You_hear("a soft chiming.");
					}
					if(rn2(100) < Insanity){
						change_usanity(rnd(Role_if(PM_MADMAN) ? 6 : Role_if(PM_EXILE) ? 5 : 1), FALSE);
					}
					healup(1, 0, FALSE, FALSE);
					lastheard = moves;
				}
			}
		}
	}
}

void
do_ancient_breaths(mtmp)
struct monst *mtmp;
{
#define common_valid_target_inhale(tmpm) distmin(tmpm->mx,tmpm->my,mtmp->mx,mtmp->my) <= 4\
	&& tmpm->mpeaceful != mtmp->mpeaceful\
	&& tmpm->mtame != mtmp->mtame\
	&& !is_ancient(tmpm)\
	&& !DEADMONSTER(tmpm)\
	&& !(tmpm->mtrapped && t_at(tmpm->mx, tmpm->my) && t_at(tmpm->mx, tmpm->my)->ttyp == VIVI_TRAP)
#define common_valid_target_exhale_nodistance(tmpm)\
	tmpm->mpeaceful != mtmp->mpeaceful\
	&& tmpm->mtame != mtmp->mtame\
	&& !is_ancient(tmpm)\
	&& !DEADMONSTER(tmpm)\
	&& !nonthreat(tmpm)
#define common_valid_target_exhale(tmpm) distmin(tmpm->mx,tmpm->my,mtmp->mx,mtmp->my) <= BOLT_LIM\
	&& common_valid_target_exhale_nodistance(tmpm)

#define common_you_target_inhale() distmin(u.ux,u.uy,mtmp->mx,mtmp->my) <= 4\
	&& !mtmp->mpeaceful\
	&& !mtmp->mtame
#define common_you_target_exhale_nodistance() !mtmp->mtame && !mtmp->mpeaceful
#define common_you_target_exhale(mtmp) common_you_target_exhale_nodistance()\
	&& distmin(u.ux,u.uy,mtmp->mx,mtmp->my) <= BOLT_LIM

#define valid_blessings_target_inhale(tmpm) common_valid_target_inhale(tmpm)\
		&& !is_demon(tmpm->data)\
		&& has_blessings(tmpm)
#define valid_blessings_target_exhale(mtmp, tmpm) common_valid_target_exhale_nodistance(tmpm)\
		&& !(no_innards(tmpm->data) && !has_blood(tmpm->data))\
		&& clear_path(mtmp->mx, mtmp->my, tmpm->mx, tmpm->my)

#define you_blessings_target_inhale() common_you_target_inhale()\
		&& !is_demon(youracedata)\
		&& has_blessings(&youmonst)
#define you_blessings_target_exhale(mtmp) common_you_target_exhale_nodistance()\
		&& !(no_innards(youracedata) && !has_blood(youracedata))\
		&& couldsee(mtmp->mx, mtmp->my)

	// The breath attack can be on cooldown under very rare circumstances (ex: just created from a hell vault)
	if(is_ancient(mtmp) && mtmp->mvar_ancient_breath_cooldown > 0){
		mtmp->mvar_ancient_breath_cooldown--;
		return;
	}

	if(mtmp->mtyp == PM_ANCIENT_OF_BLESSINGS){
		struct monst *tmpm;
		int targets = 0, damage = 0;
		for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
			if(valid_blessings_target_inhale(tmpm)) targets++;
		}
		if(you_blessings_target_inhale()) targets++;

		if(targets){
			targets = rnd(targets);
			for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
				if(valid_blessings_target_inhale(tmpm)) targets--;
				if(!targets) break;
			}
		}
		if(tmpm){
			damage = blessings_consume(mtmp, tmpm);
		}
		else if(you_blessings_target_inhale()){
			damage = blessings_consume(mtmp, &youmonst);
			nomul(0, NULL); //Interrupt
		}
		/** Exhale: Cause misfortune (wounds) in line of sight **/
		if(damage){
			struct attack fakespell = { AT_MAGC, AD_CLRC, 0, 7 };
			if(canseemon(mtmp))
				pline("%s halo flares and casts blades of light in all directions!", s_suffix(Monnam(mtmp)));
			for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
				if(valid_blessings_target_exhale(mtmp, tmpm)){
					cast_spell(mtmp, tmpm, &fakespell, OPEN_WOUNDS, tmpm->mx, tmpm->my);
				}
			}
			if(you_blessings_target_exhale(mtmp)){
				cast_spell(mtmp, &youmonst, &fakespell, OPEN_WOUNDS, u.ux, u.uy);
			}
		}
	}

#define valid_vitality_target_inhale(tmpm) common_valid_target_inhale(tmpm)\
		&& has_organic(tmpm)
#define valid_vitality_target_exhale(tmpm) common_valid_target_exhale(tmpm)\
		&& !nonliving(tmpm->data)

#define you_vitality_target_inhale() common_you_target_inhale()\
		&& has_organic(&youmonst)
#define you_vitality_target_exhale(mtmp) common_you_target_exhale(mtmp)\
		&& !nonliving(youracedata)

	if(mtmp->mtyp == PM_ANCIENT_OF_VITALITY){
		struct monst *tmpm;
		int targets = 0, damage = 0;
		for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
			if(valid_vitality_target_inhale(tmpm)) targets++;
		}
		if(you_vitality_target_inhale()) targets++;

		if(targets){
			targets = rnd(targets);
			for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
				if(valid_vitality_target_inhale(tmpm)) targets--;
				if(!targets) break;
			}
		}
		if(tmpm){
			damage = vitality_consume(mtmp, tmpm->minvent, tmpm);
		}
		else if(you_vitality_target_inhale()){
			damage = vitality_consume(mtmp, invent, &youmonst);
			nomul(0, NULL); //Interrupt
		}
		
		/** Exhale: Overload positive energy **/
		if(damage){
			targets = 0;
			for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
				if(valid_vitality_target_exhale(tmpm)) targets++;
			}
			if(you_vitality_target_exhale(mtmp)) targets++;
			if(targets){
				targets = rnd(targets);
				for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
					if(valid_vitality_target_exhale(tmpm)) targets--;
					if(!targets) break;
				}
			}
			if(tmpm){
				vitality_overload(mtmp, tmpm, damage);
			}
			else if(you_vitality_target_exhale(mtmp)){
				vitality_overload(mtmp, &youmonst, damage);
				nomul(0, NULL); //Interrupt
			}
		}
	}

#define valid_corruption_target_inhale(tmpm) common_valid_target_inhale(tmpm)\
		&& !(resists_fire(tmpm) && resists_sickness(tmpm))\
		&& tmpm->mtyp != PM_GREEN_SLIME\
		&& !has_template(tmpm, SLIME_REMNANT)
#define you_corruption_target_inhale() common_you_target_inhale()\
		&& youracedata->mtyp != PM_GREEN_SLIME\
		&& !(Fire_resistance && Sick_resistance)

	if(mtmp->mtyp == PM_ANCIENT_OF_CORRUPTION){
		struct monst *tmpm;
		int targets = 0, damage = 0;
		for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
			if(valid_corruption_target_inhale(tmpm)) targets++;
		}
		if(you_corruption_target_inhale()) targets++;
		if(targets){
			targets = rnd(targets);
			for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
				if(valid_corruption_target_inhale(tmpm)) targets--;
				if(!targets) break;
			}
		}
		if(tmpm){
			if(canseemon(tmpm) && canseemon(mtmp)){
				pline("Slime bubbles up from under %s %s.", s_suffix(mon_nam(tmpm)), mbodypart(tmpm, BODY_SKIN));
				pline("The bursting bubbles' spray is drawn into the oral groove of %s.", mon_nam(mtmp));
			} else if(canseemon(tmpm)){
				pline("Slime bubbles up from under %s %s.", s_suffix(mon_nam(tmpm)), mbodypart(tmpm, BODY_SKIN));
			} else if(canseemon(mtmp)){
				pline("Shimmering spray is drawn into the oral groove of %s.", mon_nam(mtmp));
			}
			damage = d(min(10, (mtmp->m_lev)/3), 6);
			if(is_wooden(tmpm->data) || (!resists_fire(tmpm) && species_resists_cold(tmpm)))
				damage *= 2;

			if(Half_spel(tmpm)) damage = (damage+1)/2;

			tmpm->mhp -= damage;
			if(!resists_sickness(tmpm)){
				tmpm->mhpmax -= damage;
				tmpm->mhpmax = max(tmpm->mhpmax, tmpm->m_lev);
			}
			if(tmpm->mhp < 1){
				if (canspotmon(tmpm))
					pline("%s liquifies!", Monnam(tmpm));
				set_template(tmpm, SLIME_REMNANT);
				grow_up(mtmp,tmpm);
				tmpm->m_lev = max(1, max(tmpm->m_lev, tmpm->data->mlevel));
				tmpm->mhp = tmpm->mhpmax = tmpm->m_lev*hd_size(tmpm->data);
				if(mtmp->mtame){
					tmpm->mpeaceful = 1;
				} else {
					if(tmpm->mtame && !(EDOG(tmpm)->loyal)){
						untame(tmpm, FALSE);
						tmpm->mpeaceful = 0;
						set_malign(tmpm);
					}
					else if(!tmpm->mtame && tmpm->mpeaceful){
						tmpm->mpeaceful = 0;
						set_malign(tmpm);
					}
				}
				newsym(tmpm->mx, tmpm->my);
				//Grow up may have killed mtmp
				if(DEADMONSTER(mtmp))
					return;
			}
			mtmp->mhp += damage;
			if(mtmp->mhp > mtmp->mhpmax){
				mtmp->mhp = mtmp->mhpmax;
			}
			mtmp->mspec_used = 0;
			set_mcan(mtmp, FALSE);
		} else if(targets > 0 && you_corruption_target_inhale()){
			pline("Slime bubbles up from under your %s.", body_part(BODY_SKIN));
			if(canseemon(mtmp)){
				pline("The bursting bubbles' spray is drawn into the oral groove of %s.", mon_nam(mtmp));
			}
			damage = d(min(10, (mtmp->m_lev)/3), 6);
			
			make_sick(Sick ? Sick / 2L + 1L : (long)rn1(ACURR(A_CON), 20), mtmp->data->mname, TRUE, SICK_NONVOMITABLE);
			
			if(is_wooden(youracedata) || (!Fire_resistance && species_resists_cold(&youmonst)))
				damage *= 2;
			damage = reduce_dmg(&youmonst,damage,FALSE,TRUE);

			int temparise = u.ugrave_arise;
			u.ugrave_arise = PM_ANCIENT_OF_CORRUPTION;
			losehp(damage, "corrupting slime", KILLED_BY);
			u.ugrave_arise = temparise;
			
			nomul(0, NULL); //Interrupt
			mtmp->mhp += damage;
			if(mtmp->mhp > mtmp->mhpmax){
				mtmp->mhp = mtmp->mhpmax;
			}
			mtmp->mspec_used = 0;
			set_mcan(mtmp, FALSE);
			mtmp->mux = u.ux;
			mtmp->muy = u.uy;
		}
		/** Exhale: Slime explosion **/
		if(damage){
			if(canseemon(mtmp))
				pline("%s breathes out a spout of slime!", Monnam(mtmp));
			damage = d(min(10, (mtmp->m_lev)/3), 12);
			explode_yours(mtmp->mx, mtmp->my, AD_SLIM, MON_EXPLODE, damage, EXPL_NOXIOUS, 8, FALSE);
		}
	}

#define valid_wastes_target_inhale(tmpm) common_valid_target_inhale(tmpm)\
		&& (!is_anhydrous(tmpm->data) || has_blood_mon(tmpm))
#define valid_wastes_target_exhale(mtmp, tmpm) common_valid_target_exhale(tmpm)\
		&& (!resists_fire(tmpm) || nonliving(tmpm->data))

#define you_wastes_target_inhale() common_you_target_inhale()\
		&& (!is_anhydrous(youracedata) || has_blood(youracedata))
#define you_wastes_target_exhale(mtmp) common_you_target_exhale(mtmp)\
		&& (!Fire_resistance || nonliving(youracedata))

	if(mtmp->mtyp == PM_ANCIENT_OF_THE_BURNING_WASTES){
		struct monst *tmpm;
		int targets = 0, damage = 0;
		for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
			if(valid_wastes_target_inhale(tmpm)) targets++;
		}

		if(you_wastes_target_inhale()) targets++;

		if(targets){
			targets = rnd(targets);
			for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
				if(valid_wastes_target_inhale(tmpm)) targets--;
				if(!targets) break;
			}
		}
		if(tmpm){
			if(canseemon(tmpm) && canseemon(mtmp)){
				pline("%s rises from %s %s.", has_blood_mon(tmpm) ? "Bloody mist" : "Mist", s_suffix(mon_nam(tmpm)), mbodypart(tmpm, BODY_SKIN));
				pline("The mist is drawn down to the shifting panes of %s.", mon_nam(mtmp));
			} else if(canseemon(tmpm)){
				pline("%s rises from %s %s.", has_blood_mon(tmpm) ? "Bloody mist" : "Mist", s_suffix(mon_nam(tmpm)), mbodypart(tmpm, BODY_SKIN));
			} else if(canseemon(mtmp)){
				pline("%s is drawn down to the shifting panes of %s.", has_blood_mon(tmpm) ? "Bloody mist" : "Mist", mon_nam(mtmp));
			}
			damage = d(min(10, (mtmp->m_lev)/3), 5);
			if(Half_spel(tmpm)) damage = (damage+1)/2;

			tmpm->mhp -= damage;
			if(has_blood_mon(tmpm) && !resists_drain(tmpm)){
				tmpm->mhpmax = max(tmpm->mhpmax-8, 1);
				if(tmpm->m_lev){
					tmpm->m_lev--;
				}
				else {
					tmpm->mhp = 0;
				}
			}
			if(tmpm->mhp < 1){
				tmpm->mhp = 0;
				grow_up(mtmp,tmpm);
				mondied(tmpm);
				//Grow up may have killed mtmp
				if(DEADMONSTER(mtmp))
					return;
			}
			mtmp->mhp += damage;
			if(mtmp->mhp > mtmp->mhpmax){
				mtmp->mhp = mtmp->mhpmax;
			}
			mtmp->mspec_used = 0;
			set_mcan(mtmp, FALSE);
		} else if(targets > 0 && you_wastes_target_inhale()){
			if(!Blind)
				pline("%s rises from your %s.", has_blood(youracedata) ? "Bloody mist" : "Mist", body_part(BODY_SKIN));
			if(canseemon(mtmp)){
				pline("The mist is drawn down to the shifting panes of %s.", mon_nam(mtmp));
			}
			damage = d(min(10, (mtmp->m_lev)/3), 5);
			damage = reduce_dmg(&youmonst,damage,FALSE,TRUE);


			xdamagey(mtmp, &youmonst, (struct attack *)0, damage);
			
			if(has_blood(youracedata) && !Drain_resistance){
				losexp("blood drain",TRUE,TRUE,TRUE);
			}
			nomul(0, NULL); //Interrupt

			mtmp->mhp += damage;
			if(mtmp->mhp > mtmp->mhpmax){
				mtmp->mhp = mtmp->mhpmax;
			}
			mtmp->mspec_used = 0;
			set_mcan(mtmp, FALSE);
			mtmp->mux = u.ux;
			mtmp->muy = u.uy;
		}
		/** Exhale: Heat shimmers **/
		if(damage){
			if(canseemon(mtmp))
				pline("Heat shimmers surround %s!", mon_nam(mtmp));
			else if(distmin(u.ux,u.uy,mtmp->mx,mtmp->my) <= BOLT_LIM){
				if(!Fire_resistance){
					if(Blind) You_feel("oppressively hot!");
					else You_feel("mildly warm.");
				}
				else
					pline("Heat shimmers surround you!");
			}
			for(tmpm = fmon; tmpm; tmpm = tmpm->nmon) if(valid_wastes_target_exhale(mtmp, tmpm)){
				damage = d(5, 5);
				if((!resists_fire(tmpm) && nonliving(tmpm->data))){
					damage += d(5, 5);
				}
				if(Half_spel(tmpm)) damage = (damage+1)/2;

				xdamagey(mtmp, tmpm, (struct attack *)0, damage);
				nomul(0, NULL); //Interrupt
			}
			if(you_wastes_target_exhale(mtmp)){
				damage = d(5, 5);
				if(!Fire_resistance && nonliving(youracedata)){
					damage += d(5, 5);
				}
				damage = reduce_dmg(&youmonst,damage,FALSE,TRUE);

				xdamagey(mtmp, &youmonst, (struct attack *)0, damage);
			}
			if(distmin(u.ux,u.uy,mtmp->mx,mtmp->my) <= BOLT_LIM){
				mofflin_close(mtmp);
			}
		}
	}



#define valid_gray_target_inhale(tmpm) common_valid_target_inhale(tmpm)\
		&& !mindless_mon(tmpm)
#define valid_gray_target_exhale(tmpm) common_valid_target_exhale(tmpm)\
		&& !mindless_mon(tmpm)\
		&& !nonliving(tmpm->data)

/*Note: the player is never mindless*/
#define you_gray_target_inhale() common_you_target_inhale()
#define you_gray_target_exhale(mtmp) common_you_target_exhale(mtmp)\
		&& !nonliving(youracedata)

	if(mtmp->mtyp == PM_ANCIENT_OF_THOUGHT){
		struct monst *tmpm;
		int targets = 0, damage = 0;
		for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
			if(valid_gray_target_inhale(tmpm)) targets++;
		}

		if(you_gray_target_inhale()) targets++;

		if(targets){
			targets = rnd(targets);
			for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
				if(valid_gray_target_inhale(tmpm)) targets--;
				if(!targets) break;
			}
		}
		if(tmpm){
			struct obj *helm;
			helm = which_armor(tmpm, W_ARMH);
			if(helm && helm->otyp == DUNCE_CAP){
				damage = 0;
				if(helm->oartifact);/*Blocks it*/
				else if(helm->spe > -5)
					helm->spe--;
				else
					destroy_marm(tmpm, helm);
			}
			else {
				if(canseemon(tmpm) && canseemon(mtmp)){
					pline("Gray light shines from %s %s.", s_suffix(mon_nam(tmpm)), mbodypart(tmpm, EARS));
					pline("The light is drawn under %s bell.", s_suffix(mon_nam(mtmp)));
				} else if(canseemon(tmpm)){
					pline("Gray light shines from %s %s.", s_suffix(mon_nam(tmpm)), mbodypart(tmpm, EARS));
				} else if(canseemon(mtmp)){
					pline("Gray light is drawn under %s bell.", s_suffix(mon_nam(mtmp)));
				}
				damage = d(1, min(10, (mtmp->m_lev)/3));
				if(Half_spel(tmpm)) damage = (damage+1)/2;

				tmpm->mhp -= d(damage, 10);
				if(tmpm->mhp < 1){
					if (canseemon(tmpm)) {
						pline("%s last thought fades away...",
							s_suffix(Monnam(tmpm)));
					}
					tmpm->mhp = 0;
					grow_up(mtmp,tmpm);
					mondied(tmpm);
					//Grow up may have killed mtmp
					if(DEADMONSTER(mtmp))
						return;
				}
				mtmp->mhp += damage*10;
				if(mtmp->mhp > mtmp->mhpmax){
					mtmp->mhp = mtmp->mhpmax;
				}
				mtmp->mspec_used = 0;
				set_mcan(mtmp, FALSE);
			}
		} else if(targets > 0 && you_gray_target_inhale()){
			if(uarmh && uarmh->otyp == DUNCE_CAP){
				damage = 0;
				if(uarmh->oartifact);/*Blocks it*/
				else if(uarmh->spe > -5)
					uarmh->spe--;
				else
					destroy_arm(uarmh);
			}
			else {
				//Assumes you can't see your own ears
				damage = d(1, min(10, (mtmp->m_lev)/3));
				damage = reduce_dmg(&youmonst,damage,FALSE,TRUE);

				if(!Blind){
					if(Fixed_abil){
						Your("mind goes slightly numb.");
						damage /= 2;
					}
					else Your("mind goes numb.");
				}
				if(canseemon(mtmp)){
					pline("Gray light is drawn under %s bell.", s_suffix(mon_nam(mtmp)));
				}
				
				if(!Fixed_abil){
					(void)adjattrib(A_INT, -damage, FALSE);
					int i = damage;
					while (i--){
						forget(4);	/* lose 4% of memory per point lost*/
						exercise(A_WIS, FALSE);
					}
					check_brainlessness();
				}

				mtmp->mhp += damage*10;
				if(mtmp->mhp > mtmp->mhpmax){
					mtmp->mhp = mtmp->mhpmax;
				}
				if(damage){
					mtmp->mspec_used = 0;
					set_mcan(mtmp, FALSE);
					mtmp->mux = u.ux;
					mtmp->muy = u.uy;
				}
			}
			nomul(0, NULL); //Interrupt
		}
		/** Exhale: Scream into mind **/
		if(damage){
			if(you_gray_target_exhale(mtmp)){
				pline("%s screams into your mind!", Monnam(mtmp));
				if(Hallucination){
					You("have an out of body experience.");
					losehp(15, "a bad trip", KILLED_BY); //you still take damage
				} else if(maybe_polyd((u.mh >= 100), (u.uhp >= 100))){
					Your("%s stops!  When it finally beats again, it is weak and thready.", body_part(HEART));
					losehp(d(damage,15), "the scream of an old one", KILLED_BY); //Same as death's touch attack, sans special effects
				} else {
					killer_format = KILLED_BY;
					killer = "the scream of an old one";
					if (!u.uconduct.killer){
						//Pcifist PCs aren't combatants so if something kills them up "killed peaceful" type impurities
						IMPURITY_UP(u.uimp_murder)
						IMPURITY_UP(u.uimp_bloodlust)
					}
					done(DIED);
				}
				//Roll vs. sanity
				if(rn2(100) >= NightmareAware_Sanity){
					if(!Panicking)
						You("panic!");
					HPanicking = max(damage*5, HPanicking);
				}
				//Handle off-target side effects
				thought_scream_side_effects(mtmp, &youmonst, damage);
				nomul(0, NULL); //Interrupt
			} else {
				for(tmpm = fmon; tmpm; tmpm = tmpm->nmon)
					if(valid_gray_target_exhale(tmpm))
						targets++;

				if(targets){
					targets = rnd(targets);

					for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
						if(valid_gray_target_exhale(tmpm))
							targets--;
						if(!targets) break;
					}
				}

				if(tmpm){
					/* Note: you may be damaged by the scream in side effects if it's not peaceful */
					if(tp_sensemon(mtmp) && !mtmp->mpeaceful){
						pline("%s screams!", Monnam(mtmp));
					}
					if(tmpm->mhp >= 100){
						tmpm->mhp -= d(damage,15);
						if(tmpm->mhp < 1){
							if (canspotmon(tmpm))
								pline("%s %s!", Monnam(tmpm),
								nonliving(tmpm->data)
								? "is destroyed" : "dies");
							tmpm->mhp = 0;
							grow_up(mtmp,tmpm);
							mondied(tmpm);
						}
					} else {
						if (canspotmon(tmpm))
							pline("%s %s!", Monnam(tmpm),
							nonliving(tmpm->data)
							? "is destroyed" : "dies");
						tmpm->mhp = 0;
						grow_up(mtmp,tmpm);
						mondied(tmpm);
					}
					if(!DEADMONSTER(tmpm))
						monflee(tmpm, damage*10, FALSE, TRUE);
					//Handle off-target side effects
					thought_scream_side_effects(mtmp, tmpm, damage);
					//Grow up may have killed mtmp
					if(DEADMONSTER(mtmp))
						return;
				}
				else {
					thought_scream_side_effects(mtmp, 0, damage);
				}
			}
		}
	}

#define valid_ice_target_inhale(tmpm) common_valid_target_inhale(tmpm)\
		&& !resists_fire(tmpm)
#define valid_ice_target_exhale(tmpm) common_valid_target_exhale(tmpm)\
		&& !resists_cold(tmpm)\
		&& m_online(mtmp, tmpm, tmpm->mx, tmpm->my, !(mtmp->mtame && !mtmp->mconf), FALSE)
	if(mtmp->mtyp == PM_ANCIENT_OF_ICE){
		struct monst *tmpm;
		int targets = 0, damage = 0;
		for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
			if(valid_ice_target_inhale(tmpm)) targets++;
		}
		if(distmin(u.ux,u.uy,mtmp->mx,mtmp->my) <= 4
			&& !mtmp->mpeaceful
			&& !mtmp->mtame
			&& !Fire_resistance
		) targets++;
		if(targets){
			targets = rnd(targets);
			for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
				if(valid_ice_target_inhale(tmpm)) targets--;
				if(!targets) break;
			}
		}
		if(tmpm){
			if(canseemon(tmpm) && canseemon(mtmp)){
				pline("Heat shimmer dances in the air above %s.", mon_nam(tmpm));
				pline("%s is covered in frost!", Monnam(tmpm));
				if(resists_cold(tmpm) && has_head_mon(tmpm)) pline("%s looks very surprised!", Monnam(tmpm));
				pline("The shimmers are drawn into the open mouth of %s.", mon_nam(mtmp));
			} else if(canseemon(tmpm)){
				pline("Heat shimmer dances in the air above %s.", mon_nam(tmpm));
				pline("%s is covered in frost!", Monnam(tmpm));
				if(resists_cold(tmpm) && has_head_mon(tmpm)) pline("%s looks very surprised!", Monnam(tmpm));
			} else if(canseemon(mtmp)){
				pline("Heat shimmers are drawn into the open mouth of %s.", mon_nam(mtmp));
			}
			damage = d(min(10, (mtmp->m_lev)/3), 8);
			if(Half_spel(tmpm)) damage = (damage+1)/2;

			tmpm->mhp -= damage;
			if(tmpm->mhp < 1){
				if (canspotmon(tmpm))
					pline("%s %s!", Monnam(tmpm),
					nonliving(tmpm->data)
					? "is destroyed" : "dies");
				tmpm->mhp = 0;
				grow_up(mtmp,tmpm);
				mondied(tmpm);
				//Grow up may have killed mtmp
				if(DEADMONSTER(mtmp))
					return;
			}
			mtmp->mhp += damage;
			if(mtmp->mhp > mtmp->mhpmax){
				mtmp->mhp = mtmp->mhpmax;
			}
			mtmp->mspec_used = 0;
			set_mcan(mtmp, FALSE);
		} else if(targets > 0
			&& distmin(u.ux,u.uy,mtmp->mx,mtmp->my) <= 4
			&& !mtmp->mpeaceful
			&& !mtmp->mtame
			&& !Fire_resistance
		){
			pline("Heat shimmer dances in the air above you.");
			pline("You are covered in frost!");
			if(canseemon(mtmp)){
				pline("The shimmers are drawn into the open mouth of %s.", mon_nam(mtmp));
			}
			damage = d(min(10, (mtmp->m_lev)/3), 8);
			damage = reduce_dmg(&youmonst,damage,FALSE,TRUE);

			losehp(damage, "heat drain", KILLED_BY);
			mtmp->mhp += damage;
			if(mtmp->mhp > mtmp->mhpmax){
				mtmp->mhp = mtmp->mhpmax;
			}
			nomul(0, NULL); //Interrupt
			mtmp->mspec_used = 0;
			set_mcan(mtmp, FALSE);
			mtmp->mux = u.ux;
			mtmp->muy = u.uy;
		}
		/** Exhale: Cold breath **/
		if(damage){
			struct attack mattk;
			mattk.aatyp = AT_BREA;
			mattk.adtyp = AD_COLD;
			mattk.damn = 8;
			mattk.damd = 8;
			
			if(mon_can_see_you(mtmp)){
				mtmp->mux = u.ux;
				mtmp->muy = u.uy;
			}
			
			if (!mtmp->mtame && !mtmp->mpeaceful && m_online(mtmp, &youmonst, mtmp->mux, mtmp->muy, FALSE, FALSE)){
				flags.drgn_brth = 1;
				xbreathey(mtmp, &mattk, mtmp->mux, mtmp->muy);
				flags.drgn_brth = 0;
			} else {
				for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
					if(valid_ice_target_exhale(tmpm)){
						flags.drgn_brth = 1;
						xbreathey(mtmp, &mattk, tmpm->mx, tmpm->my);
						flags.drgn_brth = 0;
						break;
					};
				}
			}
		}
	}
	
#define valid_death_target_inhale(tmpm) common_valid_target_inhale(tmpm)\
		&& !nonliving(tmpm->data)
#define valid_death_target_exhale(tmpm) common_valid_target_exhale(tmpm)\
		&& !(nonliving(tmpm->data) || is_demon(tmpm->data))\
		&& !(ward_at(tmpm->mx,tmpm->my) == CIRCLE_OF_ACHERON)\
		&& !(resists_death(tmpm))

	if(mtmp->mtyp == PM_ANCIENT_OF_DEATH){
		struct monst *tmpm;
		int targets = 0, damage = 0;
		for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
			if(valid_death_target_inhale(tmpm)) targets++;
		}
		if(distmin(u.ux,u.uy,mtmp->mx,mtmp->my) <= 4
			&& !mtmp->mpeaceful
			&& !mtmp->mtame
			&& !nonliving(youracedata)
		) targets++;
		if(targets){
			targets = rnd(targets);
			for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
				if(valid_death_target_inhale(tmpm)) targets--;
				if(!targets) break;
			}
		}
		if(tmpm){
			if(canseemon(tmpm) && canseemon(mtmp)){
				pline("Motes of light dance in the air above %s.", mon_nam(tmpm));
				pline("%s suddenly seems weaker!", Monnam(tmpm));
				if(resists_drain(tmpm) && has_head_mon(tmpm)) pline("%s looks very surprised!", Monnam(tmpm));
				pline("The motes are drawn into the %s of %s.", mtmp->mtyp == PM_BAALPHEGOR ? "open mouth" : "ghostly hood", mon_nam(mtmp));
			} else if(canseemon(tmpm)){
				pline("Motes of light dance in the air above %s.", mon_nam(tmpm));
				pline("%s suddenly seems weaker!", Monnam(tmpm));
				if(resists_drain(tmpm) && has_head_mon(tmpm)) pline("%s looks very surprised!", Monnam(tmpm));
			} else if(canseemon(mtmp)){
				pline("Motes of light are drawn into the %s of %s.", mtmp->mtyp == PM_BAALPHEGOR ? "open mouth" : "ghostly hood", mon_nam(mtmp));
			}
			damage = d(min(10, (mtmp->m_lev)/3), 4);
			if(Half_spel(tmpm)) damage = (damage+1)/2;

			tmpm->mhp -= damage;
			if(tmpm->mhp < 1){
				if (canspotmon(tmpm))
					pline("%s %s!", Monnam(tmpm),
					nonliving(tmpm->data)
					? "is destroyed" : "dies");
				tmpm->mhp = 0;
				grow_up(mtmp,tmpm);
				mondied(tmpm);
				//Grow up may have killed mtmp
				if(DEADMONSTER(mtmp))
					return;
			}
			mtmp->mhp += damage;
			if(mtmp->mhp > mtmp->mhpmax){
				mtmp->mhp = mtmp->mhpmax;
			}
			mtmp->mspec_used = 0;
			set_mcan(mtmp, FALSE);
		} else if(targets > 0
			&& distmin(u.ux,u.uy,mtmp->mx,mtmp->my) <= 4
			&& !mtmp->mpeaceful
			&& !mtmp->mtame
			&& !nonliving(youracedata)
		){
			pline("Motes of light dance in the air above you.");
			pline("You suddenly feel weaker!");
			if(canseemon(mtmp)){
				pline("The motes are drawn into the %s of %s.", mtmp->mtyp == PM_BAALPHEGOR ? "open mouth" : "ghostly hood", mon_nam(mtmp));
			}
			damage = d(min(10, (mtmp->m_lev)/3), 4);
			damage = reduce_dmg(&youmonst,damage,FALSE,TRUE);

			losehp(damage, "life-force theft", KILLED_BY);
			mtmp->mhp += damage;
			if(mtmp->mhp > mtmp->mhpmax){
				mtmp->mhp = mtmp->mhpmax;
			}
			nomul(0, NULL); //Interrupt
			mtmp->mspec_used = 0;
			set_mcan(mtmp, FALSE);
			mtmp->mux = u.ux;
			mtmp->muy = u.uy;
		}
		/** Exhale: Death **/
		if(damage){
			if(!mtmp->mtame && !mtmp->mpeaceful && distmin(u.ux,u.uy,mtmp->mx,mtmp->my) <= BOLT_LIM
				&& !(nonliving(youracedata) || is_demon(youracedata))
				&& !(ward_at(u.ux,u.uy) == CIRCLE_OF_ACHERON)
				&& !(u.sealsActive&SEAL_OSE || resists_death(&youmonst))
			){
				if(canseemon(mtmp)){
					pline("%s breathes out dark vapors.", Monnam(mtmp));
				}
				if(Hallucination){
					You("have an out of body experience.");
					losehp(8, "a bad trip", KILLED_BY); //you still take damage
				} else if (Antimagic){
					shieldeff(u.ux, u.uy);
					Your("%s flutters!", body_part(HEART));
					losehp(d(4,4), "the ancient breath of death", KILLED_BY); //you still take damage
				} else if(maybe_polyd((u.mh >= 100), (u.uhp >= 100))){
					Your("%s stops!  When it finally beats again, it is weak and thready.", body_part(HEART));
					losehp(d(8,8), "the ancient breath of death", KILLED_BY); //Same as death's touch attack, sans special effects
				} else {
					killer_format = KILLED_BY;
					killer = "the ancient breath of death";
					if (!u.uconduct.killer){
						//Pcifist PCs aren't combatants so if something kills them up "killed peaceful" type impurities
						IMPURITY_UP(u.uimp_murder)
						IMPURITY_UP(u.uimp_bloodlust)
					}
					done(DIED);
				}
				nomul(0, NULL); //Interrupt
			} else {
				struct monst *targ = 0;
				for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
					if(valid_death_target_exhale(tmpm)){
						if(!targ || (distmin(tmpm->mx, tmpm->my, mtmp->mx, mtmp->my) < distmin(targ->mx, targ->my, mtmp->mx, mtmp->my))){
							targ = tmpm;
						}
					}
				}
				if(targ){
					if(canseemon(mtmp)){
						pline("%s breathes out dark vapors.", Monnam(mtmp));
					}
					if(resists_magm(targ)){
						targ->mhp -= 8;
						if(targ->mhp < 1){
							if (canspotmon(targ))
								pline("%s %s!", Monnam(targ),
								nonliving(targ->data)
								? "is destroyed" : "dies");
							targ->mhp = 0;
							grow_up(mtmp,targ);
							mondied(targ);
							//Grow up may have killed mtmp
							if(DEADMONSTER(mtmp))
								return;
						}
					} else if(targ->mhp >= 100){
						targ->mhp -= d(8,8);
						if(targ->mhp < 1){
							if (canspotmon(targ))
								pline("%s %s!", Monnam(targ),
								nonliving(targ->data)
								? "is destroyed" : "dies");
							targ->mhp = 0;
							grow_up(mtmp,targ);
							mondied(targ);
							//Grow up may have killed mtmp
							if(DEADMONSTER(mtmp))
								return;
						}
					} else {
						if (canspotmon(targ))
							pline("%s %s!", Monnam(targ),
							nonliving(targ->data)
							? "is destroyed" : "dies");
						targ->mhp = 0;
						grow_up(mtmp,targ);
						mondied(targ);
						//Grow up may have killed mtmp
						if(DEADMONSTER(mtmp))
							return;
					}
				}
			}
		}
	}

	if(mtmp->mtyp == PM_BAALPHEGOR && mtmp->mhp < mtmp->mhpmax/2){
		struct monst *tmpm;
		int targets = 0, damage = 0;
		for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
			if(distmin(tmpm->mx,tmpm->my,mtmp->mx,mtmp->my) <= 4
				&& (tmpm->mpeaceful != mtmp->mpeaceful || mtmp->mhp < mtmp->mhpmax/4)
				&& (tmpm->mtame != mtmp->mtame || mtmp->mhp < mtmp->mhpmax/4)
				&& !has_template(tmpm, CRYSTALFIED)
				&& !is_demon(tmpm->data)
				&& !DEADMONSTER(tmpm)
				&& !nonthreat(tmpm)
			) targets++;
		}
		if(distmin(u.ux,u.uy,mtmp->mx,mtmp->my) <= 4
			&& !mtmp->mpeaceful
			&& !mtmp->mtame
			&& !is_demon(youracedata)
		) targets++;
		if(targets){
			targets = rnd(targets);
			for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
				if(distmin(tmpm->mx,tmpm->my,mtmp->mx,mtmp->my) <= 4
					&& (tmpm->mpeaceful != mtmp->mpeaceful || mtmp->mhp < mtmp->mhpmax/4)
					&& (tmpm->mtame != mtmp->mtame || mtmp->mhp < mtmp->mhpmax/4)
					&& !has_template(tmpm, CRYSTALFIED)
					&& !is_demon(tmpm->data)
					&& !DEADMONSTER(tmpm)
					&& !nonthreat(tmpm)
				) targets--;
				if(!targets) break;
			}
		}
		if(tmpm){
			if(canseemon(tmpm) && canseemon(mtmp)){
				pline("Some unseen virtue is drawn from %s.", mon_nam(tmpm));
//				pline("%s suddenly seems weaker!", Monnam(tmpm));
				pline("The virtue is sucked into the open mouth of %s.", mon_nam(mtmp));
			} else if(canseemon(tmpm)){
				pline("Some unseen virtue is drawn from %s.", mon_nam(tmpm));
//				pline("%s suddenly seems weaker!", Monnam(tmpm));
				if(resists_drain(tmpm) && has_head_mon(tmpm)) pline("%s looks very surprised!", Monnam(tmpm));
			} else if(canseemon(mtmp)){
				pline("Some unseen virtue is sucked into the open mouth of %s.", mon_nam(mtmp));
			}
			damage = d(min(10, (mtmp->m_lev)/3), 8);
			if(mon_resistance(tmpm, FREE_ACTION)) damage /= 2;
			if(resists_cold(tmpm)) damage /= 2;
			if(damage >= tmpm->mhp){
				grow_up(mtmp,tmpm);
				if (canspotmon(tmpm))
					pline("%s turns to glass!", Monnam(tmpm));
				tmpm->mpeaceful = mtmp->mpeaceful;
				if(tmpm->mtame && tmpm->mtame != mtmp->mtame)
					untame(tmpm, mtmp->mpeaceful);
				set_template(tmpm, CRYSTALFIED);
				newsym(tmpm->mx, tmpm->my);
				//Grow up may have killed mtmp
				if(DEADMONSTER(mtmp))
					return;
				mtmp->mhp += tmpm->mhp;
			}
			else{
				tmpm->mhp -= damage;
				mtmp->mhp += damage;
			}

			if(mtmp->mhp > mtmp->mhpmax){
				mtmp->mhp = mtmp->mhpmax;
			}
			mtmp->mspec_used = 0;
			set_mcan(mtmp, FALSE);
		} else if(targets > 0
			&& distmin(u.ux,u.uy,mtmp->mx,mtmp->my) <= 4
			&& !mtmp->mpeaceful
			&& !mtmp->mtame
			&& !is_demon(youracedata)
		){
			pline("Some unseen virtue is drawn from you.");
			u.umadness |= MAD_COLD_NIGHT;
			if(canseemon(mtmp)){
				pline("The virtue is sucked into the open mouth of %s.", mon_nam(mtmp));
			}
			damage = d(min(10, (mtmp->m_lev)/3), 8);
			if(Free_action) damage /= 2;
			if((HCold_resistance && ECold_resistance)
				|| (NCold_resistance)
			) damage /= 2;
			int temparise = u.ugrave_arise;
			mtmp->mhp += min(damage, maybe_polyd(u.mh, u.uhp));
			u.ugrave_arise = PM_BAALPHEGOR;
			mdamageu(mtmp, damage);
			/*If the player surived the attack, restore the value of arise*/
			u.ugrave_arise = temparise;
			
			if(mtmp->mhp > mtmp->mhpmax){
				mtmp->mhp = mtmp->mhpmax;
			}
			nomul(0, NULL); //Interrupt
			mtmp->mspec_used = 0;
			set_mcan(mtmp, FALSE);
			mtmp->mux = u.ux;
			mtmp->muy = u.uy;
		}
		/** Exhale: Static curses **/
		if(damage){
			struct	obj	*otmp;
			for(otmp = invent; otmp; otmp=otmp->nobj)
				if(otmp->oartifact == ART_HELPING_HAND)
					break;
			if(!mtmp->mtame && !mtmp->mpeaceful && distmin(u.ux,u.uy,mtmp->mx,mtmp->my) <= BOLT_LIM
				&& (!Curse_res(&youmonst, FALSE) || !rn2(8))
				&& !(otmp && rn2(20))
				&& !(u.ukinghill && rn2(20))
			){
				if(canseemon(mtmp)){
					pline("%s breathes out static curses.", Monnam(mtmp));
				}
				if(
					(!Free_action || HFast) &&
					(!uarmh || uarmh->cursed) &&
					(!uarmc || uarmc->cursed) &&
					(!uarm || uarm->cursed) &&
					(!uarmu || uarmu->cursed) &&
					(!uarmg || uarmg->cursed) &&
					(!uarmf || uarmf->cursed)
				) {
					if(Free_action){
						HFast = 0L;
						if (!Very_fast)
							You_feel("yourself slowing down%s.",
										Fast ? " a bit" : "");
					} else{
						if (!u.uconduct.killer){
							//Pcifist PCs aren't combatants so if something kills them up "killed peaceful" type impurities
							IMPURITY_UP(u.uimp_murder)
							IMPURITY_UP(u.uimp_bloodlust)
						}
						done(GLASSED);
					}
				} else {
					int nobj = 0, cnt, onum;
					for (otmp = invent; otmp; otmp = otmp->nobj) {
#ifdef GOLDOBJ
						/* gold isn't subject to being cursed or blessed */
						if (otmp->oclass == COIN_CLASS) continue;
#endif
						if(!otmp->cursed) nobj++;
					}
					if (nobj) {
						for (cnt = 8; cnt > 0; cnt--)  {
							onum = rnd(nobj);
							for (otmp = invent; otmp; otmp = otmp->nobj) {
#ifdef GOLDOBJ
								/* gold isn't subject to being cursed or blessed */
								if (otmp->oclass == COIN_CLASS) continue;
#endif
								if(!otmp->cursed) onum--;
								if(!onum) break;
							}
							if (!otmp || otmp->cursed) continue;	/* next target */
							if(otmp->oartifact && arti_gen_prop(otmp, ARTG_MAJOR) &&
							   rn2(10) < 8) {
								pline("%s!", Tobjnam(otmp, "resist"));
								continue;
							}

							if(otmp->blessed)
								unbless(otmp);
							else
								curse(otmp);
							update_inventory();
						}
					}
				}
				u.umadness |= MAD_COLD_NIGHT;
				nomul(0, NULL); //Interrupt
			} else {
				struct monst *targ = 0;
				struct obj *otmp;
				for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
					if(distmin(tmpm->mx,tmpm->my,mtmp->mx,mtmp->my) <= BOLT_LIM
						&& tmpm->mpeaceful != mtmp->mpeaceful
						&& tmpm->mtame != mtmp->mtame
						&& !has_template(tmpm, CRYSTALFIED)
						&& (!Curse_res(tmpm, FALSE) || !rn2(8))
						&& !DEADMONSTER(tmpm)
						&& !nonthreat(tmpm)
					){
						for(otmp = tmpm->minvent; otmp; otmp=otmp->nobj)
							if(otmp->oartifact == ART_TREASURY_OF_PROTEUS)
								break;
						if(otmp && rn2(20))
							continue;
						for(otmp = tmpm->minvent; otmp; otmp=otmp->nobj)
							if(otmp->oartifact == ART_HELPING_HAND)
								break;
						if(otmp && rn2(20))
							continue;
						if(!targ || (distmin(tmpm->mx, tmpm->my, mtmp->mx, mtmp->my) < distmin(targ->mx, targ->my, mtmp->mx, mtmp->my))){
							targ = tmpm;
						}
					}
				}
				if(targ){
					if(canseemon(mtmp)){
						pline("%s breathes out static curses.", Monnam(mtmp));
					}
					if(
						!(mon_resistance(targ, FREE_ACTION)) &&
						!(targ->misc_worn_check & W_ARMH && (otmp = which_armor(targ, W_ARMH)) && !otmp->cursed) &&
						!(targ->misc_worn_check & W_ARMC && (otmp = which_armor(targ, W_ARMC)) && !otmp->cursed) &&
						!(targ->misc_worn_check & W_ARM && (otmp = which_armor(targ, W_ARM)) && !otmp->cursed) &&
						!(targ->misc_worn_check & W_ARMU && (otmp = which_armor(targ, W_ARMU)) && !otmp->cursed) &&
						!(targ->misc_worn_check & W_ARMG && (otmp = which_armor(targ, W_ARMG)) && !otmp->cursed) &&
						!(targ->misc_worn_check & W_ARMF && (otmp = which_armor(targ, W_ARMF)) && !otmp->cursed)
					) {
						minstaglass(targ,FALSE);
					} else {
						int nobj = 0, cnt, onum;
						for (otmp = targ->minvent; otmp; otmp = otmp->nobj) {
#ifdef GOLDOBJ
							/* gold isn't subject to being cursed or blessed */
							if (otmp->oclass == COIN_CLASS) continue;
#endif
							if(!otmp->cursed) nobj++;
						}
						if (nobj) {
							for (cnt = 8; cnt > 0; cnt--)  {
								onum = rnd(nobj);
								for (otmp = targ->minvent; otmp; otmp = otmp->nobj) {
#ifdef GOLDOBJ
									/* gold isn't subject to being cursed or blessed */
									if (otmp->oclass == COIN_CLASS) continue;
#endif
									if(!otmp->cursed) onum--;
									if(!onum) break;
								}
								if (!otmp || otmp->cursed) continue;	/* next target */
								if (otmp->oartifact && arti_gen_prop(otmp, ARTG_MAJOR) &&
								   rn2(10) < 8) {
									continue;
								}

								if(otmp->blessed)
									unbless(otmp);
								else
									curse(otmp);
								// update_inventory();
							}
						}
					}
				}
			}
		}
	}
}

STATIC_OVL void
clothes_bite_mon(mon)
struct monst *mon;
{
	struct obj *otmp;
	for(otmp = mon->minvent; otmp && !DEADMONSTER(mon); otmp = otmp->nobj){
		if((otmp->olarva || otmp->obyak || check_oprop(otmp, OPROP_BYAK)) && otmp->owornmask)
			held_item_bites(mon, otmp);
	}
}

STATIC_OVL void
emit_healing(mon)
struct monst *mon;
{
	int heal_amnt = (*hpmax(mon) - *hp(mon))/4;
	int dmg;

	if(!heal_amnt)
		return;

	if(canseemon(mon))
		pline("%s bleeds holy starlight!", Monnam(mon));

	if(mon == &youmonst)
		healup(heal_amnt, 0, FALSE, FALSE);
	else
		mon->mhp = min(mon->mhpmax, mon->mhp+heal_amnt);

	heal_amnt /= 8;
	
	if(!heal_amnt)
		return;

	struct monst *mtmp;
	for(int i = mon->mx-1; i < mon->mx+2; i++){
		for(int j = mon->my-1; j < mon->my+2; j++){
			if(isok(i,j)){
				mtmp = m_u_at(i,j);
				if(mtmp == mon)
					continue;
				if(mtmp == &youmonst){
					if(hates_holy(youracedata) || hates_silver(youracedata)){
						You("are seared by the %s!", hates_silver(youracedata) ? "silver starlight" : "holy energy");
						dmg = 0;
						if(hates_holy(youracedata))
							dmg += heal_amnt;
						if(hates_silver(youracedata))
							dmg += rnd(20);
						dmg = reduce_dmg(&youmonst,dmg,FALSE,TRUE);
						losehp(dmg, "holy healing light", KILLED_BY);
					}
					else {
						healup(heal_amnt, 0, FALSE, FALSE);
					}
				}
				else if(mtmp && !DEADMONSTER(mtmp)){
					if(hates_holy_mon(mtmp) || hates_silver(mtmp->data)){
						dmg = 0;
						if(hates_holy_mon(mtmp))
							dmg += heal_amnt;
						if(hates_silver(mtmp->data))
							dmg += rnd(20);
						dmg = reduce_dmg(mtmp,dmg,FALSE,TRUE);
						if(canseemon(mtmp) && dmg)
							pline("%s is seared by the %s!", Monnam(mtmp), hates_silver(mtmp->data) ? "silver starlight" : "holy energy");
						mtmp->mhp -= dmg;
						if(mtmp->mhp < 1){
							grow_up(mon,mtmp);
							mondied(mtmp);
						}
					}
					else if(mtmp->mhp < mtmp->mhpmax){
						if(canspotmon(mtmp))
							pline("%s looks better!", Monnam(mtmp));
						mtmp->mhp = min(mtmp->mhpmax, mtmp->mhp+heal_amnt);
					}
				}
			}
		}
	}
}

void
phantom_scorpions_sting(mdef)
struct monst *mdef;
{
	long slotvar = 0;
	long depthvar = 0;
	int damage = 0;
	int dmg;
	int i = 0;
	if(mdef == &youmonst){
		i = ((NightmareAware_Insanity*NightmareAware_Insanity) + 999)/1000;
		if(i < 1)
			i = 1;
	}
	else {
		i = mdef->m_lev/10;
		if(insightful(mdef->data))
			i /= 2;
		if(i < 1)
			i = 1;
	}
	
	for(; i > 0; i--){
		/*pick a slot*/
		slotvar = 0x1<<rn2(5);

		/*pick a depth*/
		depthvar = rn2(3) ? W_ARMC : rn2(2) ? W_ARM : W_SKIN;
		
		dmg = d(1,4); //d(1,2) + d(1,2)
		if(mdef == &youmonst){
			// Your("%s bite%s you!", xname(obj), obj->quan == 1 ? "s":"");
			// pline("damage pre DR: %d, slotvar: %ld, wornmask: %ld", damage, slotvar, obj->owornmask);
			dmg -= roll_udr_detail((struct monst *)0, slotvar, depthvar, ROLL_SLOT);
			// pline("damage post DR: %d", damage);
		}
		else {
			// pline("damage pre DR: %d", damage);
			dmg -= roll_mdr_detail(mdef, (struct monst *)0, slotvar, depthvar, ROLL_SLOT);
			// pline("damage post DR: %d", damage);
		}
		if(dmg < 1)
			dmg = 1;
		damage += dmg;
	}

	damage = reduce_dmg(mdef,damage,TRUE,TRUE);

	if(mdef == &youmonst){
		You("are stung by phantom scorpions!");
		losehp(damage, "a swarm of illusory scorpions", KILLED_BY);
		poisoned("scorpion stings", A_STR, "poisoned illusory scorpion stings", 8, FALSE);
	}
	else {
		if(!Poison_res(mdef)){
			if(!rn2(8))
				damage += 80;
			else damage += rn1(10,6);
		}
		if(m_losehp(mdef, damage, FALSE, "swarm of scorpions")); //died
		else if (canseemon(mdef))
			pline("%s is stung by phantom scorpions.", Monnam(mdef));
	}
}

void
rot_caterpillars_bite(struct monst *mdef)
{
	int damage = 0;
	if(mdef == &youmonst){
		IMPURITY_UP(u.uimp_rot)
		if (!Sick_res(mdef)) {
			if(!Sick) make_sick((long)rn1(ACURR(A_CON), 20), "rotting caterpillars", TRUE, SICK_NONVOMITABLE);
			damage += (*hp(mdef))*3.3/100 + 26;
		}
		else {
			damage += (*hp(mdef))*2/100 + 8;
		}
		You("are bitten by a swarm of parasitic caterpillars!");
		losehp(damage, "a swarm of parasitic caterpillars", KILLED_BY);
		if(has_blood(youracedata)){
			Your("blood is being drained!");
			IMPURITY_UP(u.uimp_blood)
			if(!rn2(3) && !Drain_res(mdef)){
				losexp("life force drain", TRUE, FALSE, FALSE);
			}
		}
	}
	else {
		if (!Sick_res(mdef)) {
			damage += (!rn2(10)) ? 100 : rnd(12);
			damage += (*hp(mdef))*3.3/100 + 26;
		}
		else {
			damage += (*hp(mdef))*2/100 + 8;
		}
		if(has_blood_mon(mdef) && !rn2(3) && !Drain_res(mdef)){
			pline("%s suddenly seems weaker!", Monnam(mdef));
			if(!mdef->m_lev)
				damage += mdef->mhpmax;
			else mdef->m_lev--;
			mdef->mhpmax -= (hd_size(mdef->data)+1)/2;
			mdef->mhpmax = max(mdef->mhpmax, 1);
			mdef->mhp = min(mdef->mhpmax, mdef->mhp);
		}
		if(m_losehp(mdef, damage, FALSE, "swarm of parasitic caterpillars")); //died
		else if (canseemon(mdef))
			pline("%s is bitten by parasitic caterpillars.", Monnam(mdef));
	}
}

void
orc_mud_stabs(struct monst *mdef)
{
	int damage = 0;
	int number = rnd(3);
	damage += d(number, 2);
	if(mdef == &youmonst)
		damage -= roll_udr_detail((struct monst *)0, 0x1<<rn2(5), W_ARMC, ROLL_SLOT);
	else
		damage -= roll_mdr_detail(mdef, (struct monst *)0, 0x1<<rn2(5), W_ARMC, ROLL_SLOT);

	damage = max(damage, 1);

	if (!Acid_res(mdef)) {
		damage += d(number, 2) + d(number, 10);
	}

	if(mdef == &youmonst){
		You("are stabbed by the writhing tarry mud!");
		losehp(damage, "inchoate orcs", KILLED_BY);
	}
	else {
		if(m_losehp(mdef, damage, FALSE, "inchoate orcs")); //died
		else if (canseemon(mdef))
			pline("%s is stabbed by the writhing tarry mud.", Monnam(mdef));
	}
}

#endif /* OVLB */

/*mon.c*/
