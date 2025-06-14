/*	SCCS Id: @(#)artifact.c 3.4	2003/08/11	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include <math.h>
#include "hack.h"
#include "artifact.h"
#include "artilist.h"
#include "xhity.h"
/*
 * Note:  both artilist[] and artinstance[] have a dummy element #0,
 *	  so loops over them should normally start at #1.  The primary
 *	  exception is the save & restore code, which doesn't care about
 *	  the contents, just the total size.
 */

extern boolean notonhead;	/* for long worms */
extern boolean lifehunt_sneak_attacking;
extern struct attack grapple;

//duplicates of other functions, created due to problems with the linker
STATIC_DCL void NDECL(cast_protection);
STATIC_DCL void FDECL(awaken_monsters,(int));

STATIC_DCL void FDECL(do_item_blast, (int));
STATIC_DCL void FDECL(nitocris_sarcophagous, (struct obj *));
STATIC_DCL void FDECL(fulvous_desk, (struct obj *));

int FDECL(donecromenu, (const char *,struct obj *));
int FDECL(dopetmenu, (const char *,struct obj *));
int FDECL(dosongmenu, (const char *,struct obj *));
int FDECL(dolordsmenu, (const char *,struct obj *));
int FDECL(doannulmenu, (const char *,struct obj *));
int FDECL(doselfpoisonmenu, (const char *,struct obj *));
int FDECL(doartificemenu, (const char *,struct obj *));
int FDECL(doprismaticmenu, (const char *,struct obj *));

static NEARDATA schar delay;		/* moves left for this spell */
static NEARDATA struct obj *artiptr;/* last/current artifact being used */
static NEARDATA int necro_effect;	/* necro effect picked */
static NEARDATA int lostname;	/* spirit # picked */

static NEARDATA	int oozes[] = {PM_ACID_BLOB, PM_QUIVERING_BLOB, 
					  PM_GELATINOUS_CUBE, PM_DARKNESS_GIVEN_HUNGER, PM_GRAY_OOZE, 
					  PM_BROWN_PUDDING, PM_BLACK_PUDDING, PM_GREEN_SLIME,
					  PM_AOA, PM_BROWN_MOLD, PM_RED_MOLD};

static NEARDATA	int devils[] = {PM_IMP, PM_LEMURE, 
					  PM_LEGION_DEVIL_GRUNT, PM_LEGION_DEVIL_SOLDIER, PM_LEGION_DEVIL_SERGEANT, 
					  PM_HORNED_DEVIL, PM_BARBED_DEVIL, PM_BONE_DEVIL,
					  PM_ICE_DEVIL, PM_PIT_FIEND, PM_ANCIENT_OF_ICE, PM_ANCIENT_OF_DEATH};

static NEARDATA	int demons[] = {PM_QUASIT, PM_MANES, PM_QUASIT,
					  PM_MANES, PM_QUASIT, PM_MANES, 
					  PM_SUCCUBUS, PM_INCUBUS, PM_VROCK, 
					  PM_HEZROU, PM_NALFESHNEE, PM_MARILITH,
					  PM_BALROG, PM_VROCK, PM_HEZROU};


STATIC_PTR int NDECL(read_necro);
STATIC_PTR int NDECL(read_lost);

STATIC_DCL int FDECL(select_gift_artifact, (aligntyp));
STATIC_DCL int FDECL(select_floor_artifact, (struct obj *));

STATIC_DCL int FDECL(arti_invoke, (struct obj*));
STATIC_DCL boolean FDECL(Mb_hit, (struct monst *magr,struct monst *mdef,
				  struct obj *,int *,int,BOOLEAN_P,char *,char *));
STATIC_DCL boolean FDECL(voidPen_hit, (struct monst *magr,struct monst *mdef,
				  struct obj *,int *,int,BOOLEAN_P,char *));
STATIC_DCL boolean FDECL(narrow_voidPen_hit, (struct monst *mdef, struct obj *));

#ifndef OVLB
STATIC_DCL int spec_dbon_applies;
STATIC_DCL int artidisco[NROFARTIFACTS];
#else	/* OVLB */
/* coordinate effects from spec_dbon() with messages in artifact_hit() */
STATIC_OVL int spec_dbon_applies = 0;

/* flags including which artifacts have already been created */
struct artinstance * artinstance;
/* and a discovery list for them (no dummy first entry here) */
STATIC_OVL int * artidisco;
/* and a counter for how many artifacts we have */
int nrofartifacts;
/* and a pointer to space for randarts' names */
char ** artiextranames;

STATIC_DCL boolean FDECL(attacks, (int,struct obj *));

const int elements[4] = {AD_PHYS, AD_FIRE, AD_COLD, AD_ELEC};
const int explType[4] = {0, EXPL_FIERY, EXPL_FROSTY, EXPL_MAGICAL};

boolean
CountsAgainstGifts(x)
int x;
{
	return (!(artilist[x].gflags & ARTG_NOCNT));
							/*(x != ART_WATER_CRYSTAL && \
								x != ART_FIRE_CRYSTAL && \
								x != ART_EARTH_CRYSTAL && \
								x != ART_AIR_CRYSTAL && \
								x != ART_WEREBUSTER && \
								!(x >= ART_FIRST_KEY_OF_LAW && x <= ART_THIRD_KEY_OF_NEUTRALITY) && \
								x != ART_CARESS && \
								x != ART_ICONOCLAST \
								) //checks to make sure x isn't the index of an artifact that doesn't count against
									//the odds of getting a sac gift.  These are the alignment keys and a few
									//auto-generated artifacts of dubious worth.
									*/
}

/* returns TRUE if otmp may be offered to an artifact-desiring being */
boolean
offerable_artifact(otmp)
struct obj * otmp;
{
	// validate
	if (!otmp || !otmp->oartifact)
		return FALSE;

	int oart = otmp->oartifact;

	// Filter out specific artifacts
	switch (oart) {
	case ART_SILVER_KEY:
	case ART_ANNULUS:
	case ART_PEN_OF_THE_VOID:
		// Outright forbidden
		return FALSE;
	case ART_FLUORITE_OCTAHEDRON:
		// requires the full set of 8
		if (otmp->quan < 8)
			return FALSE;
		break;
	case ART_BLADE_SINGER_S_SABER:
		// cannot be one of the halves
		if (otmp->otyp == RAKUYO_DAGGER || otmp->otyp == RAKUYO_SABER)
			return FALSE;
		break;
		// otherwise continue
	}

	// Must not have the ARTG_NOCNT flag
	if (!CountsAgainstGifts(oart))
		return FALSE;

	// otherwise, is acceptable
	return TRUE;
}

void
make_singing_sword_nameable()
{
	artilist[ART_SINGING_SWORD].gflags |= ARTG_NAME;
}

/* handle some special cases; must be called after u_init() 
	Uh, it isn't, it's called BEFORE u_init. See allmain */
void
hack_artifacts()
{
	struct artifact *art;
	int alignmnt = flags.stag ? u.ualign.type : aligns[flags.initalign].value;
	
	if(Role_if(PM_EXILE)) alignmnt = A_VOID; //hack_artifacts may be called before this is properly set
	if((Race_if(PM_DROW) || Race_if(PM_MYRKALFR)) && !Role_if(PM_EXILE) && !Role_if(PM_CONVICT) && !flags.initgend){
		alignmnt = A_NEUTRAL; /* Males are neutral */
	}

	/* Fix up the alignments of "gift" artifacts */
	for (art = artilist+1; art->otyp; art++)
	    if ((art->role == Role_switch || Pantheon_if(art->role))
	    		&& (art->alignment != A_NONE || Role_if(PM_BARD))
		    	&& !(art->gflags & ARTG_FXALGN))
			art->alignment = alignmnt;


	/* Excalibur can be used by any lawful character, not just knights */
	if (!Role_if(PM_KNIGHT)){
 	    artilist[ART_EXCALIBUR].role = NON_PM;
	    //artilist[ART_CLARENT].role = NON_PM; Clarent is Knight-only
	} else if(Race_if(PM_DWARF) && urole.ldrnum == PM_THORIN_II_OAKENSHIELD){
		artilist[ART_EXCALIBUR].role = NON_PM;
		artilist[ART_RHONGOMYNIAD].role = NON_PM;
		artilist[ART_CLARENT].role = NON_PM;
		artilist[ART_GLAMDRING].gflags &= ~(ARTG_NOGEN);
		artilist[ART_GLAMDRING].role = PM_KNIGHT;
	}
	artilist[ART_MANTLE_OF_HEAVEN].otyp = find_cope();
	artilist[ART_VESTMENT_OF_HELL].otyp = find_opera_cloak();
	artilist[ART_CROWN_OF_THE_SAINT_KING].otyp = find_gcirclet();
	artilist[ART_HELM_OF_THE_DARK_LORD].otyp = find_vhelm();
	artilist[ART_SHARD_FROM_MORGOTH_S_CROWN].otyp = find_good_iring();
	
	artilist[ART_RING_OF_THROR].otyp = find_good_dring();
	artilist[ART_NARYA].otyp = find_good_fring();
	artilist[ART_NENYA].otyp = find_good_wring();
	artilist[ART_VILYA].otyp = find_good_aring();
	
	artilist[ART_LOMYA].otyp = find_signet_ring();

	if(!Role_if(PM_BARD)){
 	    artilist[ART_SINGING_SWORD].role = NON_PM;
	}
	
	if(Role_if(PM_ANACHRONONAUT)){
		artilist[ART_CRESCENT_BLADE].race = NON_PM;
		artilist[ART_CRESCENT_BLADE].alignment = A_NONE;
	}
	
	/* Remove flag from the non-matching first gift */
	if(Pantheon_if(PM_BARBARIAN)){
		if(u.role_variant == TWO_HANDED_SWORD){
			artilist[ART_CLEAVER].role = NON_PM;
			artilist[ART_ATLANTEAN_ROYAL_SWORD].role = PM_BARBARIAN;
		} else if(u.role_variant == BATTLE_AXE){
			artilist[ART_ATLANTEAN_ROYAL_SWORD].role = NON_PM;
			artilist[ART_CLEAVER].role = PM_BARBARIAN;
		} else {
			if(rn2(2)){
				u.role_variant = TWO_HANDED_SWORD;
				artilist[ART_CLEAVER].role = NON_PM;
				artilist[ART_ATLANTEAN_ROYAL_SWORD].role = PM_BARBARIAN;
			} else { //Priest with this pantheon
				u.role_variant = BATTLE_AXE;
				artilist[ART_ATLANTEAN_ROYAL_SWORD].role = NON_PM;
				artilist[ART_CLEAVER].role = PM_BARBARIAN;
			}
		}
	}
	if(Pantheon_if(PM_SAMURAI)){
		if(u.role_variant == NAGINATA){
			artilist[ART_KIKU_ICHIMONJI].role = NON_PM;
			artilist[ART_JINJA_NAGINATA].role = PM_SAMURAI;
		} else if(u.role_variant == KATANA){
			artilist[ART_JINJA_NAGINATA].role = NON_PM;
			artilist[ART_KIKU_ICHIMONJI].role = PM_SAMURAI;
		} else { //Priest with this pantheon
			if(rn2(2)){
				u.role_variant = NAGINATA;
				artilist[ART_KIKU_ICHIMONJI].role = NON_PM;
				artilist[ART_JINJA_NAGINATA].role = PM_SAMURAI;
			} else {
				u.role_variant = KATANA;
				artilist[ART_JINJA_NAGINATA].role = NON_PM;
				artilist[ART_KIKU_ICHIMONJI].role = PM_SAMURAI;
			}
		}
	}
	
	if(Role_if(PM_HEALER) && Race_if(PM_DROW)){
		artilist[ART_ROBE_OF_CLOSED_EYES].gflags |= ARTG_GIFT;
		artilist[ART_ROBE_OF_CLOSED_EYES].gflags &= ~ARTG_NOGEN;
	}
	
	/* fem hlf nob, or fem hlf without a first gift, always get Lifehunt Scythe */
	/* previously overrode all other first gifts, but now that fem hlf nob exists only overrides no first gift */
	boolean first_gift_found = FALSE;
	if(Race_if(PM_HALF_DRAGON) && flags.initgend){
		int i;
		for(i = 0; i < ART_ROD_OF_SEVEN_PARTS; i++){
			if(artilist[i].role == Role_switch){
				if (Role_if(PM_NOBLEMAN)) artilist[i].role = NON_PM;
				else {first_gift_found = TRUE; break; }
			}
		}
		if (!first_gift_found){
			artilist[ART_LIFEHUNT_SCYTHE].role = Role_switch;
			artilist[ART_LIFEHUNT_SCYTHE].alignment = alignmnt;
		}
	}
	
	/* Fix up the crown */
	switch (find_gcirclet())
	{
	case HELMET:
		obj_descr[HELMET].oc_name = "circlet";
		break;
	case HELM_OF_BRILLIANCE:
		obj_descr[HELM_OF_BRILLIANCE].oc_name = "crown of cognizance";
		break;
	case HELM_OF_OPPOSITE_ALIGNMENT:
		obj_descr[HELM_OF_OPPOSITE_ALIGNMENT].oc_name = "tiara of treachery";
		break;
	case HELM_OF_TELEPATHY:
		obj_descr[HELM_OF_TELEPATHY].oc_name = "tiara of telepathy";
		break;
	case HELM_OF_DRAIN_RESISTANCE:
		obj_descr[HELM_OF_DRAIN_RESISTANCE].oc_name = "diadem of drain resistance";
		break;
	}
	
	/* Fix up the quest artifact */
	if(Pantheon_if(PM_NOBLEMAN) || Role_if(PM_NOBLEMAN)){
		artilist[ART_CROWN_OF_THE_SAINT_KING].size = (&mons[urace.malenum])->msize;
		artilist[ART_HELM_OF_THE_DARK_LORD].size = (&mons[urace.malenum])->msize;

		if(Race_if(PM_VAMPIRE)){
			artilist[ART_HELM_OF_THE_DARK_LORD].alignment = alignmnt;
		} else if(Race_if(PM_ELF)){
			artilist[ART_ROD_OF_LORDLY_MIGHT].gflags |= ARTG_NOGEN;
			artilist[ART_ROD_OF_LORDLY_MIGHT].role = NON_PM;
			artilist[ART_ROD_OF_THE_ELVISH_LORDS].gflags &= ~(ARTG_NOGEN);
		} else if(Race_if(PM_DROW)){
			if(flags.initgend){ /* TRUE == female */
				artilist[ART_ROD_OF_LORDLY_MIGHT].gflags |= ARTG_NOGEN;
				artilist[ART_ROD_OF_LORDLY_MIGHT].role = NON_PM;
				artilist[ART_SCEPTRE_OF_LOLTH].gflags &= ~(ARTG_NOGEN);
			} else {
				artilist[ART_ROD_OF_LORDLY_MIGHT].gflags |= ARTG_NOGEN;
				artilist[ART_ROD_OF_LORDLY_MIGHT].role = NON_PM;
				artilist[ART_DEATH_SPEAR_OF_KEPTOLO].gflags &= ~(ARTG_NOGEN);
			}
		} else if(Race_if(PM_DWARF) && urole.ldrnum == PM_DAIN_II_IRONFOOT){
			artilist[ART_ROD_OF_LORDLY_MIGHT].gflags |= ARTG_NOGEN;
			artilist[ART_ROD_OF_LORDLY_MIGHT].role = NON_PM;
			artilist[ART_ARMOR_OF_KHAZAD_DUM].gflags &= ~(ARTG_NOGEN);
		} else if(alignmnt == A_NEUTRAL) {
			artilist[ART_CROWN_OF_THE_SAINT_KING].alignment = alignmnt;
		}
	}
	if (Role_if(PM_MONK)) {
	    artilist[ART_GRANDMASTER_S_ROBE].alignment = alignmnt;

	    artilist[ART_EYE_OF_THE_OVERWORLD].size = (&mons[urace.malenum])->msize;
	}

	if(Role_if(PM_PRIEST)){
		artilist[ART_MITRE_OF_HOLINESS].size = (&mons[urace.malenum])->msize;
	}

	if (urole.questarti) {
	    artilist[urole.questarti].alignment = alignmnt;
	    artilist[urole.questarti].role = Role_switch;
	}
	if(aligns[flags.initalign].value == artilist[ART_CARNWENNAN].alignment){
		artilist[ART_CARNWENNAN].gflags |= (ARTG_NAME); //name only
	}
	if(Role_if(PM_BARD) || Race_if(PM_ELF)){
		make_singing_sword_nameable(); //name only
	}
	/* Callandor only works for natural male players */
	if (flags.initgend) {
		artilist[ART_CALLANDOR].wprops[0] = NO_PROP;
		artilist[ART_CALLANDOR].wprops[1] = NO_PROP;
		artilist[ART_CALLANDOR].wprops[2] = NO_PROP;
	}
	/* Fix up fire brand and frost brand */
	if (u.brand_otyp != STRANGE_OBJECT) {
		artilist[ART_FIRE_BRAND].otyp = u.brand_otyp;
		artilist[ART_FROST_BRAND].otyp = u.brand_otyp;
	}
	/* fix up pen of the void */
	artilist[ART_PEN_OF_THE_VOID].alignment = A_VOID; //something changes this??? Change it back.
	if (Role_if(PM_EXILE))
		artilist[ART_PEN_OF_THE_VOID].gflags &= ~ARTG_NOGEN;
	return;
}

/* declare the global artilist pointer */
struct artifact * artilist;

/* zero out the artifact existence list */
void
init_artifacts()
{
	extern const struct artifact base_artilist[];

	nrofartifacts = NROFARTIFACTS;
	artinstance = malloc(sizeof(struct artinstance) * (1+nrofartifacts+1));
	artidisco = malloc(sizeof(int) * (nrofartifacts));
	artilist = malloc(sizeof(struct artifact) * (1+nrofartifacts+1));
	
	(void) memset((genericptr_t) artinstance, 0, sizeof(struct artinstance) * (1+nrofartifacts+1));
	(void) memset((genericptr_t) artidisco, 0, sizeof(int) * (nrofartifacts));
	memcpy(artilist, base_artilist, sizeof(struct artifact) * (1+nrofartifacts+1));
	hack_artifacts();
}

void
save_artifacts(fd)
int fd;
{
	bwrite(fd, (genericptr_t) &nrofartifacts, sizeof(int));
	bwrite(fd, (genericptr_t) artinstance, sizeof(struct artinstance) * (1+nrofartifacts+1));
	bwrite(fd, (genericptr_t) artidisco, sizeof(int) * (nrofartifacts));
	bwrite(fd, (genericptr_t) artilist, sizeof(struct artifact) * (1+nrofartifacts+1));
	if(nrofartifacts > NROFARTIFACTS) {
		bwrite(fd, (genericptr_t) artiextranames, sizeof(char)*PL_PSIZ*(nrofartifacts - NROFARTIFACTS));
	}
}

void
restore_artifacts(fd)
int fd;
{
	extern const struct artifact base_artilist[];
	mread(fd, (genericptr_t) &nrofartifacts, sizeof(int));

	artinstance = malloc(sizeof(struct artinstance) * (1+nrofartifacts+1));
	artidisco = malloc(sizeof(int) * (nrofartifacts));
	artilist = malloc(sizeof(struct artifact) * (1+nrofartifacts+1));
	mread(fd, (genericptr_t) artinstance, sizeof(struct artinstance) * (1+nrofartifacts+1));
	mread(fd, (genericptr_t) artidisco, sizeof(int) * (nrofartifacts));
	mread(fd, (genericptr_t) artilist, sizeof(struct artifact) * (1+nrofartifacts+1));
	/*Fixup name pointers*/
	for(int i = 0; i <= NROFARTIFACTS; i++){
		artilist[i].name = base_artilist[i].name;
		artilist[i].desc = base_artilist[i].desc;
	}
	if(nrofartifacts > NROFARTIFACTS) {
		artiextranames = malloc(sizeof(char*)*(nrofartifacts - NROFARTIFACTS));
		int i, j;
		for(i=NROFARTIFACTS, j=0; i < nrofartifacts; i++, j++) {
			artiextranames[j] = malloc(sizeof(char)*PL_PSIZ);
			mread(fd, (genericptr_t) artiextranames[j], sizeof(char)*PL_PSIZ);
			artilist[i+1].name = artiextranames[j];
		}
	}
}

int
n_artifacts()
{
	return nrofartifacts;
}

struct artifact *
add_artifact()
{
	int old_narts = nrofartifacts;
	int i;
	nrofartifacts++;

	/* allocate new memory, slightly larger */
	void * tmp_artinstance = malloc(sizeof(struct artinstance) * (1+nrofartifacts+1));	// same indices as artilist
	void * tmp_artidisco = malloc(sizeof(int) * (nrofartifacts));						// only need 1 per artifact
	void * tmp_artilist = malloc(sizeof(struct artifact) * (1+nrofartifacts+1));		// dummy + artifacts + terminator
	void ** tmp_artiextranames = malloc(sizeof(char*)*(nrofartifacts - NROFARTIFACTS));
	
	/* copy old data to new memory */
	memcpy(tmp_artinstance, artinstance, (sizeof(struct artinstance) * (1+old_narts+1)));
	memcpy(tmp_artidisco, artidisco, (sizeof(int) * (old_narts)));
	memcpy(tmp_artilist, artilist, (sizeof(struct artifact) * (old_narts+1)));
	
	/* free old memory */
	free(artinstance);
	free(artidisco);
	free(artilist);

	/* reassign pointers */
	artinstance = tmp_artinstance;
	artidisco = tmp_artidisco;
	artilist = tmp_artilist;
	
	/* repeat, with loop, for artiextranames */
	for (i = 0; i < nrofartifacts - NROFARTIFACTS; i++) {
		tmp_artiextranames[i] = malloc(sizeof(char)*PL_PSIZ);
		if (i == old_narts - NROFARTIFACTS) continue;	// copy&free 1 fewer than created
		Strcpy(tmp_artiextranames[i], artiextranames[i]);
		free(artiextranames[i]);
	}
	if (old_narts != NROFARTIFACTS)
		free(artiextranames);
	artiextranames = (char **)tmp_artiextranames;
	for (i = 0; i < nrofartifacts - NROFARTIFACTS; i++) {
		artilist[i+NROFARTIFACTS+1].name = artiextranames[i];
	}

	/* fill with default values */
	memset((genericptr_t) &(artinstance[nrofartifacts]), 0, sizeof(struct artinstance));
	artidisco[nrofartifacts-1] = 0;
	memcpy((genericptr_t) &(artilist[nrofartifacts]), (genericptr_t)artilist, sizeof(struct artifact));
	artilist[nrofartifacts].name = artiextranames[nrofartifacts-NROFARTIFACTS-1];
	// name will be written front-forwards and we don't have to worry about old data
	/* re-zero terminator */
	memset((genericptr_t) &(artilist[nrofartifacts+1]), 0, sizeof(struct artifact));

	return &(artilist[nrofartifacts]);
}

const char *
artiname(artinum)
int artinum;
{
	if (artinum <= 0 || artinum > nrofartifacts) return("");
	return(artilist[artinum].name);
}

int
arti_value(otmp)
struct obj * otmp;
{
	struct artifact * arti = get_artifact(otmp);
	int artival;
	int baseartival;

	if (!arti)
		return 0;

	/* start with the artifact's base value */
	baseartival = artival = arti->giftval;

	if (artival == NO_TIER)
		return 0;

	/* monks can get a bonus for some offensive armors */
	if (otmp->oclass == ARMOR_CLASS
		&& Role_if(PM_MONK)) {
		switch (otmp->oartifact)
		{
		case ART_GODHANDS:
		case ART_PREMIUM_HEART:
		case ART_GRAPPLER_S_GRASP:
		case ART_GRANDMASTER_S_ROBE:
			artival += artival / 2;
			break;
		case ART_HAMMERFEET:
			artival++;
			break;
		}
	}
	/* everyone gets a bonus for a weapon they get Expert with */
	if (weapon_type(otmp) != P_BARE_HANDED_COMBAT
		&& P_MAX_SKILL(weapon_type(otmp)) >= P_EXPERT)
	{
		artival++;
	}
	/* as well as if they're Skilled or better and the weapon is good enough for endgame */
	if (weapon_type(otmp) != P_BARE_HANDED_COMBAT
		&& P_MAX_SKILL(weapon_type(otmp)) >= P_SKILLED
		&& baseartival >= TIER_B)
	{	
		artival++;
	}
	/* and -1 if they're only Basic */
		if (weapon_type(otmp) != P_BARE_HANDED_COMBAT
		&& P_MAX_SKILL(weapon_type(otmp)) <= P_BASIC)
	{	
		artival--;
	}
	return artival;
}

/* WORK IN PROGRESS */
/* creates a randart out of otmp */
struct obj *
mk_randart(otmp)
struct obj *otmp;	/* existing object; NOT ignored even if alignment specified */
{
	if (otmp->oclass != WEAPON_CLASS &&
		otmp->oclass != ARMOR_CLASS &&
		!is_weptool(otmp)
	){
		impossible("cannot make randart out of %s", xname(otmp));
		return otmp;
	}
	struct artifact * a = add_artifact();
	int w = 0;

	/* maximum artifact name length is PL_PSIZ */
	/* randart generation will fail if the name isn't unique */
	int jr = n_artifacts() - NROFARTIFACTS;
	Sprintf((char *)a->name, "Randart McRandFace the %d%s", jr, jr==1?"st":jr==2?"nd":jr==3?"rd":"th");
	a->alignment = rn2(3) ? A_NONE : !rn2(3) ? A_LAWFUL : !rn2(2) ? A_NEUTRAL : A_CHAOTIC;
	a->otyp = otmp->otyp;

	if (otmp->oclass == WEAPON_CLASS || is_weptool(otmp)) {
		/* offensive stuff */
		do { a->adtyp = rn2(9); }while(a->adtyp == AD_DISN);
		a->accuracy = max(1, rn2(5)*5);
		a->damage = rn2(3)*10;

		if (objects[otmp->otyp].oc_merge)
			a->damage /= 2;
		else {
			if (a->adtyp > AD_MAGM && rn2(3))
				a->wprops[w++] = a->adtyp-1;	//assumes adtyp-1 == x_res, which is true from AD_FIRE to AD_ACID
			if (!rn2(30) && w<8)
				a->wprops[w++] = FAST;
			if (!rn2(60) && w<8)
				a->wprops[w++] = FREE_ACTION;
		}

		if (!rn2(20))
			a->aflags |= ARTA_DEXPL;
		else if (!rn2(20))
			a->aflags |= ARTA_DLUCK;

		if (!rn2(20) && a->alignment != A_LAWFUL)
			a->aflags |= ARTA_POIS;
		if (!rn2(40) && a->alignment == A_LAWFUL)
			a->aflags |= ARTA_BRIGHT|ARTA_BLIND;
		if (!rn2(60) && a->alignment != A_LAWFUL)
			a->aflags |= ARTA_DRAIN;

		if (!rn2(60))
			a->aflags |= ARTA_EXPLFIRE;
		if (!rn2(60))
			a->aflags |= ARTA_EXPLCOLD;
		if (!rn2(60))
			a->aflags |= ARTA_EXPLELEC;
		
		if (!rn2(90) && !(objects[otmp->otyp].oc_merge))
			a->iflags |= ARTI_BLOODTHRST;
		if (!rn2(90))
			a->aflags |= ARTA_VORPAL;
	}
	else if ((is_gloves(otmp) && rn2(4)) ||	// 3/4 gloves
			(is_boots(otmp) && !rn2(3))) {	// 1/3 boots
		/* mix of offense (for unarmed) and defense */
		a->adtyp = AD_PHYS;
		a->accuracy = max(1, rn2(5)*3);
		a->damage = rn2(3)*6;
		/* some resistances, and maybe re-elementing the damage */
		for (; w < rnd(3); w++) {
			a->wprops[w++] = rnd(18);	/* FIRE_RES to TELEPORT_CONTROL */
			if (a->wprops[w-1] == FIRE_RES || a->wprops[w-1] == COLD_RES ||
				a->wprops[w-1] == ACID_RES || a->wprops[w-1] == SHOCK_RES) {
				a->adtyp = a->wprops[w-1] + 1; //assumes x_res+1 == adtyp, which is true from AD_FIRE to AD_ACID
				if (a->damage) a->damage += 4;
			}
		}
		if (rn2(10))
			a->iflags |= ARTI_PLUSSEV;
	}
	else if (otmp->oclass == ARMOR_CLASS) {
		/* some resistances */
		for (; w < (objects[otmp->otyp].oc_magic ? rnd(3) : rnd(4)+1); w++) {
			a->wprops[w++] = rnd(18);	/* FIRE_RES to TELEPORT_CONTROL */
		}
		if (!rn2(10) && w<8)
			a->wprops[w++] = TELEPAT;
		if (!rn2(10) && w<8)
			a->wprops[w++] = FAST;
		if (!rn2(20) && w<8)
			a->wprops[w++] = HALLUC_RES;
		if (!rn2(20) && w<8)
			a->wprops[w++] = FREE_ACTION;
		if (!rn2(60) && w<8)
			a->wprops[w++] = ENERGY_REGENERATION;
		if (!rn2(60) && w<8)
			a->wprops[w++] = HALF_SPDAM;
		if (!rn2(60) && w<8)
			a->wprops[w++] = HALF_PHDAM;

		if (rn2(20))
			a->iflags |= ARTI_PLUSSEV;

		if (!rn2(30))
			a->iflags |= ARTI_LUCK;
	}

	if (!rn2(3)) {
		rand_interesting_obj_material(otmp);
		a->material = otmp->obj_material;
	}

	otmp = oname(otmp, a->name);

	if (objects[otmp->otyp].oc_merge) {
		set_obj_quan(otmp, is_ammo(otmp) ? 20 : 10);
	}

	return otmp;
}

/*
   Make an artifact.  If a specific alignment is specified, then an object of
   the appropriate alignment is created from scratch, or 0 is returned if
   none is available.  (If at least one aligned artifact has already been
   given, then unaligned ones also become eligible for this.)
   If no alignment is given, then 'otmp' is converted
   into an artifact of matching type, or returned as-is if that's not possible.
   For the 2nd case, caller should use ``obj = mk_artifact(obj, A_NONE);''
   for the 1st, ``obj = mk_artifact((struct obj *)0, some_alignment);''.
 */
struct obj *
mk_artifact(otmp, alignment)
struct obj *otmp;	/* existing object; ignored if alignment specified */
aligntyp alignment;	/* target alignment, or A_NONE */
{
	int arti;
	boolean by_align = (alignment != (aligntyp)A_NONE);
	
	/* get an artifact */
	if (by_align)
		arti = select_gift_artifact(alignment);
	else
		arti = select_floor_artifact(otmp);

	if (arti) {
		const struct artifact * a = &artilist[arti];
	    /* make an appropriate object if necessary */
		if (by_align) {
			int otyp = a->otyp;

			if ((arti == ART_FIRE_BRAND || arti == ART_FROST_BRAND) && u.brand_otyp == STRANGE_OBJECT) {
				if (Role_if(PM_MONK))
					otyp = GAUNTLETS;
				else
					otyp =	!rn2(3) ? LONG_SWORD :
							!rn2(7) ? SABER :
							!rn2(6) ? SCIMITAR :
							!rn2(5) ? GAUNTLETS :
							!rn2(4) ? BROADSWORD :
							!rn2(3) ? AXE :
							!rn2(2) ? KHOPESH :
							!rn2(4) ? WAKIZASHI :
							!rn2(2) ? SHORT_SWORD :
									  ATHAME;
			}
			otmp = mksobj(otyp, NO_MKOBJ_FLAGS);
		}
		/* christen the artifact, which also sets its artifactness */
	    otmp = oname(otmp, a->name);
		/* WHY */
		if (arti == ART_HELM_OF_THE_ARCANE_ARCHER && by_align){
			unrestrict_weapon_skill(P_ATTACK_SPELL);
        }
	} else {
	    /* nothing appropriate could be found; return the original object */
	    if (by_align) otmp = 0;	/* (there was no original object) */
	}
	return otmp;
}

/* select an artifact to gift */
int
select_gift_artifact(alignment)
aligntyp alignment;
{
	const struct artifact * a;	/* artifact pointer, being looped */
	int m;						/* artifact index, being looped */
	int n = 0;					/* number of acceptable artifacts, reset every attempt at selecting */
	int attempts = 0;			/* attempts made creating a list of artifacts */
	int condition;
	int eligible[nrofartifacts];

	/* Pirates quite purposefully can only get the Marauder's Map */
	if (Role_if(PM_PIRATE))
		return (artinstance[ART_MARAUDER_S_MAP].exists ? 0 : ART_MARAUDER_S_MAP);

	for (attempts = 1; (!n || (!rn2(n - 1) && u.ugifts)); attempts++) {
		n = 0;
		for (m = 1, a = artilist + 1; a->otyp; a++, m++)
		{
			condition = 1;

#define skip_if(x) if((attempts < ++condition) && (x)) continue

			/* cannot already exist */
			if (artinstance[m].exists)
				continue;
			/* cannot be nogen */
			if (a->gflags & ARTG_NOGEN)
				continue;

			/* conditions that can be relaxed, in order of least-to-most important */

			/* try to get a role-specific first (second, third, etc.) gift -- overrides alignment, artg_gift considerations */
			skip_if(!(Role_if(a->role) || Pantheon_if(a->role)));
			
			if (attempts > 1) {
				/* try to get an aligned first gift */
				skip_if(!u.ugifts && a->alignment != alignment);
				/* go for preferred artifacts for your first gift (if you didn't already have one specified) */
				skip_if(!u.ugifts && !(a->gflags & ARTG_GIFT));
			}

			/* avoid weapons for Monks */
			skip_if(Role_if(PM_MONK) && !is_monk_safe_artifact(m) && !u.uconduct.weaphit);
			skip_if(Role_if(PM_MONK) && !is_monk_safe_artifact(m) && rn2(20));	/* we relax this requirement before removing it */

			/* avoid artifacts of materials that hate the player's natural form */
			skip_if(!(Role_if(a->role) || Pantheon_if(a->role)) && (
				(hates_iron((&mons[urace.malenum]))
				&& (
					(a->material == IRON || (a->material == MT_DEFAULT && objects[a->otyp].oc_material == IRON))
					|| (a->material == GREEN_STEEL || (a->material == MT_DEFAULT && objects[a->otyp].oc_material == GREEN_STEEL))
				))
				||
				(hates_silver((&mons[urace.malenum]))
				&& (a->material == SILVER || (a->material == MT_DEFAULT && objects[a->otyp].oc_material == SILVER)))
				));

			/* skip lightsources for Drow */
			skip_if(Race_if(PM_DROW) &&
				(  (a->iflags & ARTI_PERMALIGHT)
				|| (a->iflags & ARTI_LIGHT)
				|| (m == ART_HOLY_MOONLIGHT_SWORD)
				));

			/* avoid boots for chiropterans */
			skip_if(Race_if(PM_CHIROPTERAN) && objects[a->otyp].oc_class == ARMOR_CLASS && objects[a->otyp].oc_armcat == ARM_BOOTS);

			/* skip nameable artifacts */
			skip_if((a->gflags & ARTG_NAME));

			/* skip Callandor for non-males */
			skip_if(m == ART_CALLANDOR && flags.initgend);

			/* skip artifacts that outright hate the player */
			skip_if((a->aflags & ARTA_HATES) && (urace.selfmask & a->mflagsa));

			/* skip cross-aligned artifacts */
			skip_if(a->alignment != A_NONE && a->alignment != alignment);

			/* if we made it through that gauntlet, we're good */
			eligible[n++] = m;
		}
	}
#undef skip_if
	/* return index of an artifact */
	if (n > 0)
		return eligible[rn2(n)];
	return 0;
}
/* try to select an artifact to convert otmp into */
int
select_floor_artifact(otmp)
struct obj * otmp;
{
	const struct artifact * a;	/* artifact pointer, being looped */
	int m;						/* artifact index, being looped */
	int n = 0;					/* number of acceptable artifacts, reset every attempt at selecting */
	int eligible[nrofartifacts];

	for (m = 1, a = artilist + 1; a->otyp; a++, m++)
	{
		/* cannot already exist */
		if (artinstance[m].exists)
			continue;
		/* cannot be nogen, with exceptions */
		if (a->gflags & ARTG_NOGEN && !(otmp->otyp == SPE_SECRETS))
			continue;
		/* must match otyp (or be acceptable) */
		if (!artitypematch(a, otmp))
			continue;
		/* Fire Brand and Frost Brand can generate out of MANY otypes, so decrease their odds of being chosen at random */
		/* if one's been generated, the other HAS to be the same otyp, so no penalty is needed */
		if ((m == ART_FIRE_BRAND || m == ART_FROST_BRAND)
			&& u.brand_otyp == STRANGE_OBJECT
			&& (
				rn2(6)
				/* small base weapons less frequent */
				|| !rn2(objects[otmp->otyp].oc_size + 1)
				/* elven/droven base weapons much less frequent */
				|| ((is_elven_weapon(otmp) || is_droven_weapon(otmp)) && rn2(4))
				)
			)
			continue;
		/* Since Pirates can only be gifted the Marauder's Map, don't let it generate on the floor and leave 
		 * them sacrificing eternally for a gift that will never come */
		if (m == ART_MARAUDER_S_MAP && Role_if(PM_PIRATE))
			continue;

		/* if we made it through that gauntlet, we're good */
		eligible[n++] = m;
	}
	/* return index of an artifact */
	if (n > 0)
		return eligible[rn2(n)];
	return 0;
}

void
add_oprop(obj, oprop)
struct obj *obj;
int oprop;
{
	if(!oprop)
		return;
	
	if(oprop >= MAX_OPROP || oprop < 0)
		impossible("Attempting to give %s oprop number %d?", doname(obj), oprop);
	
	switch(obj->where){
		case OBJ_INVENT:{
			long mask = obj->owornmask;
			setnotworn(obj);
			obj->oproperties[(oprop-1)/32] |= (0x1L << ((oprop-1)%32));
			setworn(obj, mask);
		}break;
		case OBJ_MINVENT:
			update_mon_intrinsics(obj->ocarry, obj, FALSE, TRUE); /* remove all intrinsics for now */
			obj->oproperties[(oprop-1)/32] |= (0x1L << ((oprop-1)%32));
			update_mon_intrinsics(obj->ocarry, obj, TRUE, TRUE); /* re-set remaining intrinsics */
		break;
		default:
			obj->oproperties[(oprop-1)/32] |= (0x1L << ((oprop-1)%32));
		break;
	}
}

void
remove_oprop(obj, oprop)
struct obj *obj;
int oprop;
{
	if(!oprop)
		return;

	if(oprop >= MAX_OPROP || oprop < 0)
		impossible("Attempting to remove oprop number %d from %s?", oprop, doname(obj));

	switch(obj->where){
		case OBJ_INVENT:{
			long mask = obj->owornmask;
			setnotworn(obj);
			obj->oproperties[(oprop-1)/32] &= ~(0x1L << ((oprop-1)%32));
			setworn(obj, mask);
		}break;
		case OBJ_MINVENT:
			update_mon_intrinsics(obj->ocarry, obj, FALSE, TRUE); /* remove all intrinsics for now */
			obj->oproperties[(oprop-1)/32] &= ~(0x1L << ((oprop-1)%32));
			update_mon_intrinsics(obj->ocarry, obj, TRUE, TRUE); /* re-set remaining intrinsics */
		break;
		default:
			obj->oproperties[(oprop-1)/32] &= ~(0x1L << ((oprop-1)%32));
		break;
	}
}

void
add_oprop_list(oprop_list, oprop)
unsigned long int *oprop_list;
int oprop;
{
	if(!oprop)
		return;
	
	if(oprop >= MAX_OPROP || oprop < 0)
		impossible("Attempting to add oprop number %d to a wish list?", oprop);
	
	oprop_list[(oprop-1)/32] |= (0x1L << ((oprop-1)%32));
}

boolean
check_oprop(obj, oprop)
struct obj *obj;
int oprop;
{
	if(!obj) //Just in case something checks a monk's bare fist for object properties or some such.
		return FALSE;
	
	if(!oprop){
		//Check if there are ANY oprops on this item at all
		int i;
		for(i=0;i < OPROP_LISTSIZE; i++){
			if(obj->oproperties[i])
				return FALSE; //Found (at least one) oprop, return FALSE
		}
		return TRUE; //Found no oprops, return TRUE
	}
	
	if(oprop >= MAX_OPROP || oprop < 0){
		impossible("Attempting to check oprop number %d on %s?", oprop, doname(obj));
		return FALSE;
	}

	//May have changed mats/be ammo. Silver fire only works on some materials.
	if((oprop == OPROP_SFLMW || oprop == OPROP_MORTW || oprop == OPROP_TDTHW || oprop == OPROP_SFUWW
	 || oprop == OPROP_CGLZ || oprop == OPROP_RWTH || oprop == OPROP_RBRD || oprop == OPROP_SLIF
	)
		&& !sflm_active(obj)
	)
		return FALSE;

	return !!(obj->oproperties[(oprop-1)/32] & (0x1L << ((oprop-1)%32)));
}

boolean
oprops_match(obj1, obj2)
struct obj *obj1;
struct obj *obj2;
{
	//Check if there are any mismatching subsets of oprops
	int i;
	for(i=0;i < OPROP_LISTSIZE; i++){
		if(obj1->oproperties[i] != obj2->oproperties[i])
			return FALSE; //Found (at least one) oprop that's on one but not on the other.
	}
	return TRUE; //Found no mismatches, return TRUE
}

void
copy_oprop_list(obj, oprop_list)
struct obj *obj;
unsigned long int *oprop_list;
{
	int i;
	for(i=0;i < OPROP_LISTSIZE; i++){
		obj->oproperties[i] |= oprop_list[i];
	}
}

/*
 * Make a special-but-non-artifact weapon based on otmp
 */

#define ADD_WEAPON_ARMOR_OPROP(otmp, oproptoken) \
	add_oprop(otmp, OPROP_##oproptoken);\
	if(accepts_weapon_oprops(otmp) || otmp->oclass == RING_CLASS){\
		if(rn2(3))\
			add_oprop(otmp, OPROP_LESSER_##oproptoken##W);\
		else\
			add_oprop(otmp, OPROP_##oproptoken##W);\
	}

#define ADD_WEAPON_ARMOR_OPROPS(otmp, oproptoken1, oproptoken2) \
	add_oprop(otmp, OPROP_##oproptoken1);\
	add_oprop(otmp, OPROP_##oproptoken2);\
	if(accepts_weapon_oprops(otmp) || otmp->oclass == RING_CLASS){\
		if(rn2(3)){\
			add_oprop(otmp, OPROP_LESSER_##oproptoken1##W);\
			add_oprop(otmp, OPROP_LESSER_##oproptoken2##W);\
		}\
		else{\
			add_oprop(otmp, OPROP_##oproptoken1##W);\
			add_oprop(otmp, OPROP_##oproptoken2##W);\
		}\
	}

#define ADD_WEAK_OR_STRONG_OPROP(otmp, oproptoken) \
	if(rn2(4))\
		add_oprop(otmp, OPROP_##oproptoken##W);\
	else\
		add_oprop(otmp, OPROP_LESSER_##oproptoken##W);

#define ADD_WEAK_OR_STRONG_OPROPS(otmp, oproptoken1, oproptoken2) \
	if(otmp->oclass != RING_CLASS && rn2(4)){\
		add_oprop(otmp, OPROP_##oproptoken1##W);\
		add_oprop(otmp, OPROP_##oproptoken2##W);\
	}\
	else{\
		add_oprop(otmp, OPROP_LESSER_##oproptoken1##W);\
		add_oprop(otmp, OPROP_LESSER_##oproptoken2##W);\
	}

struct obj *
mk_lolth_vault_special(otmp)
struct obj *otmp;	/* existing object */
{
	/* materials */
	if(rn2(3)){
		if(otmp->oclass == WEAPON_CLASS || is_weptool(otmp) 
			|| (is_hard(otmp) && otmp->oclass == ARMOR_CLASS && otmp->otyp != ORIHALCYON_GAUNTLETS)
		){
			void (*func)(struct obj *, int) = is_ammo(otmp) ? &set_material : &set_material_gm;
			if(objects[otmp->otyp].oc_material == GLASS)
				(*func)(otmp, rn2(2) ? GLASS : rn2(3) ? OBSIDIAN_MT : GEMSTONE);
			else
				(*func)(otmp, !rn2(7) ? SHADOWSTEEL 
									: !rn2(6) ? SILVER 
									: !rn2(5) ? MITHRIL 
									: !rn2(4) ? GLASS
									: !rn2(3) ? OBSIDIAN_MT
									: !rn2(2) ? GEMSTONE
									: MINERAL
									);
		}
	}
	/* armor props */
	if(otmp->oclass == ARMOR_CLASS || (rn2(3) && otmp->oclass == RING_CLASS)){
		if(!rn2(3)){
			ADD_WEAPON_ARMOR_OPROPS(otmp, UNHY, HOLY);
		}
		if(rn2(3)) switch(rn2(5)){
			case 0:
				ADD_WEAPON_ARMOR_OPROP(otmp, ACID);
			break;
			case 1:
				ADD_WEAPON_ARMOR_OPROP(otmp, FIRE);
			break;
			case 2:
				ADD_WEAPON_ARMOR_OPROP(otmp, ELEC);
			break;
			case 3:
				ADD_WEAPON_ARMOR_OPROP(otmp, COLD);
			break;
			case 4:
				add_oprop(otmp, OPROP_BCRS);
			break;
		}
		else if(rn2(3)){
			ADD_WEAPON_ARMOR_OPROP(otmp, ANAR);
		}
		if(!rn2(8)){
			add_oprop(otmp, OPROP_HEAL);
		}
	}
	/* weapon props */
	else if(otmp->oclass == WEAPON_CLASS || otmp->oclass == RING_CLASS){
		if(rn2(2)){
			ADD_WEAK_OR_STRONG_OPROPS(otmp, UNHY, HOLY);
		}
		if(rn2(3)) switch(rn2(5)){
			case 0:
				add_oprop(otmp, OPROP_WRTHW);
			break;
			case 1:
				ADD_WEAK_OR_STRONG_OPROP(otmp, FIRE);
			break;
			case 2:
				ADD_WEAK_OR_STRONG_OPROP(otmp, ELEC);
			break;
			case 3:
				ADD_WEAK_OR_STRONG_OPROP(otmp, COLD);
			break;
			case 4:
				ADD_WEAK_OR_STRONG_OPROP(otmp, ACID);
			break;
		}
		else if(rn2(3)){
			switch(rn2(3)){
				case 0:
					ADD_WEAK_OR_STRONG_OPROP(otmp, ANAR);
				break;
				case 1:
					ADD_WEAK_OR_STRONG_OPROP(otmp, CONC);
				break;
				case 2:
					ADD_WEAK_OR_STRONG_OPROP(otmp, AXIO);
				break;
			}
		}
		if(otmp->oclass != RING_CLASS){
			if(!rn2(20)){
				add_oprop(otmp, OPROP_VORPW);
			}
			if(!rn2(20)){
				add_oprop(otmp, OPROP_LIVEW);
			}
			else if(!rn2(20)){
				add_oprop(otmp, OPROP_RETRW);
			}
			if(!rn2(20)){
				add_oprop(otmp, OPROP_ASECW);
			}
			else if(!rn2(20)){
				add_oprop(otmp, OPROP_PSECW);
			}
		}
	}
	/* ring props (stack with weapon) */
	else if(otmp->oclass == RING_CLASS){
		if(!rn2(3)){
			ADD_WEAPON_ARMOR_OPROPS(otmp, UNHY, HOLY);
		}
		if(rn2(3)) switch(rn2(4)){
			case 0:
				ADD_WEAPON_ARMOR_OPROP(otmp, ACID);
			break;
			case 1:
				ADD_WEAPON_ARMOR_OPROP(otmp, FIRE);
			break;
			case 2:
				ADD_WEAPON_ARMOR_OPROP(otmp, ELEC);
			break;
			case 3:
				ADD_WEAPON_ARMOR_OPROP(otmp, COLD);
			break;
		}
		else if(rn2(3)){
			ADD_WEAPON_ARMOR_OPROP(otmp, ANAR);
		}
		if(!rn2(8)){
			add_oprop(otmp, OPROP_HEAL);
		}
	}
	return otmp;
}

STATIC_OVL struct obj *
mk_jrt_special(otmp)
struct obj *otmp;	/* existing object */
{
	//Can make metal studed leather
	bless(otmp);
	/* materials */
	if((otmp->oclass == WEAPON_CLASS || is_weptool(otmp)) && !is_ammo(otmp) && !rn2(3)){
		set_material_gm(otmp, MERCURIAL);
	}
	else if(rn2(3)){
		if(otmp->oclass == WEAPON_CLASS || is_weptool(otmp)
			|| (is_hard(otmp) && otmp->oclass == ARMOR_CLASS && otmp->otyp != ORIHALCYON_GAUNTLETS)
		){
			//Ammo can be stuff like silver arrows, so it should use sim-facing material setting
			if(is_ammo(otmp))
				set_material(otmp, rn2(4) ? GOLD : SILVER);
			else
				set_material_gm(otmp, rn2(4) ? GOLD : SILVER);
		}
		else if(otmp->oclass == ARMOR_CLASS){
			set_material_gm(otmp, DRAGON_HIDE);
		}
	}
	/* armor props */
	if(otmp->oclass == ARMOR_CLASS){
		if(rn2(2)){
			ADD_WEAPON_ARMOR_OPROP(otmp, HOLY);
		}
		if(rn2(3)){
			ADD_WEAPON_ARMOR_OPROP(otmp, ANAR);
		}
		if(rn2(3)) switch(rnd(is_hard(otmp) ? 5 : 4)){
			case 1:
				ADD_WEAPON_ARMOR_OPROP(otmp, FIRE);
			break;
			case 2:
				ADD_WEAPON_ARMOR_OPROP(otmp, ELEC);
			break;
			case 3:
				ADD_WEAPON_ARMOR_OPROP(otmp, COLD);
			break;
			case 4:
				add_oprop(otmp, OPROP_BCRS);
			break;
			//Anything above here should be ok for soft items
			case 5:
				add_oprop(otmp, OPROP_REFL);
				set_material_gm(otmp, rn2(4) ? GOLD : SILVER);
			break;
		}
		if(!rn2(7)){
			add_oprop(otmp, OPROP_LIFE);
		}
		if(!rn2(7)){
			add_oprop(otmp, OPROP_HEAL);
		}
		if(is_gloves(otmp)){
			if(!rn2(7)){
				add_oprop(otmp, OPROP_BLADED);
			}
		}
	}
	/* weapon props */
	else if(otmp->oclass == WEAPON_CLASS){
		if(rn2(2)){
			ADD_WEAK_OR_STRONG_OPROP(otmp, HOLY);
		}
		if(rn2(3)){
			ADD_WEAK_OR_STRONG_OPROP(otmp, ANAR);
		}
		if(rn2(3)) switch(rn2(6)){
			case 0:
				add_oprop(otmp, OPROP_WRTHW);
			break;
			case 1:
				ADD_WEAK_OR_STRONG_OPROP(otmp, FIRE);
			break;
			case 2:
				ADD_WEAK_OR_STRONG_OPROP(otmp, ELEC);
			break;
			case 3:
				ADD_WEAK_OR_STRONG_OPROP(otmp, COLD);
			break;
			case 4:
				add_oprop(otmp, OPROP_VORPW);
			break;
			case 5:
				ADD_WEAK_OR_STRONG_OPROP(otmp, ACID);
			break;
		}
		/* Living weapons */
		if(!rn2(20)){
			add_oprop(otmp, OPROP_LIVEW);
		}
		if(!rn2(20)){
			add_oprop(otmp, OPROP_ASECW);
		}
	}
	/* ring props (stack with weapon) */
	else if(otmp->oclass == RING_CLASS){
		if(rn2(2)){
			ADD_WEAPON_ARMOR_OPROP(otmp, HOLY);
		}
		if(rn2(3)){
			ADD_WEAPON_ARMOR_OPROP(otmp, ANAR);
		}
		if(rn2(3)) switch(rnd(3)){
			case 1:
				ADD_WEAPON_ARMOR_OPROP(otmp, FIRE);
			break;
			case 2:
				ADD_WEAPON_ARMOR_OPROP(otmp, ELEC);
			break;
			case 3:
				ADD_WEAPON_ARMOR_OPROP(otmp, COLD);
			break;
		}
		if(!rn2(7)){
			add_oprop(otmp, OPROP_LIFE);
		}
		if(!rn2(7)){
			add_oprop(otmp, OPROP_HEAL);
		}
	}
	return otmp;
}

STATIC_OVL struct obj *
mk_holy_special(otmp)
struct obj *otmp;	/* existing object */
{
	bless(otmp);
	/* materials */
	if(rn2(3)){
		if(otmp->oclass == WEAPON_CLASS || is_weptool(otmp) 
			|| (is_hard(otmp) && otmp->oclass == ARMOR_CLASS && otmp->otyp != ORIHALCYON_GAUNTLETS)
		){
			//Ammo can be stuff like silver arrows, so it should use sim-facing material setting
			if(is_ammo(otmp))
				set_material(otmp, rn2(2) ? GOLD : SILVER);
			else
				set_material_gm(otmp, rn2(2) ? GOLD : SILVER);
		}
		else if(otmp->oclass == ARMOR_CLASS){
			add_oprop(otmp, OPROP_WOOL);
			otmp->obj_color = CLR_WHITE;
		}
	}
	/* armor props */
	if(otmp->oclass == ARMOR_CLASS){
		if(rn2(2)){
			ADD_WEAPON_ARMOR_OPROP(otmp, HOLY);
		}
		if(rn2(3)) switch(rn2(is_hard(otmp) ? 6 : 5)){
			case 0:
				add_oprop(otmp, OPROP_WOOL);
				if(!is_hard(otmp))
					otmp->obj_color = CLR_WHITE;
			break;
			case 1:
				ADD_WEAPON_ARMOR_OPROP(otmp, FIRE);
			break;
			case 2:
				ADD_WEAPON_ARMOR_OPROP(otmp, ELEC);
			break;
			case 3:
				ADD_WEAPON_ARMOR_OPROP(otmp, COLD);
			break;
			case 4:
				add_oprop(otmp, OPROP_BCRS);
			break;
			//Anything above here should be ok for soft items
			case 5:
				add_oprop(otmp, OPROP_REFL);
				set_material_gm(otmp, rn2(2) ? GOLD : SILVER);
			break;
		}
		else if(rn2(3)){
			switch(rn2(3)){
				case 0:
					ADD_WEAPON_ARMOR_OPROP(otmp, ANAR);
				break;
				case 1:
					ADD_WEAPON_ARMOR_OPROP(otmp, CONC);
				break;
				case 2:
					ADD_WEAPON_ARMOR_OPROP(otmp, AXIO);
				break;
			}
		}
		if(!rn2(7)){
			add_oprop(otmp, OPROP_LIFE);
		}
		if(!rn2(7)){
			add_oprop(otmp, OPROP_HEAL);
		}
		if(is_gloves(otmp)){
			if(!rn2(7)){
				add_oprop(otmp, OPROP_BLADED);
			}
		}
	}
	/* weapon props */
	else if(otmp->oclass == WEAPON_CLASS){
		if(rn2(2)){
			ADD_WEAK_OR_STRONG_OPROP(otmp, HOLY);
		}
		if(rn2(3)) switch(rn2(5)){
			case 0:
				add_oprop(otmp, OPROP_WRTHW);
			break;
			case 1:
				ADD_WEAK_OR_STRONG_OPROP(otmp, FIRE);
			break;
			case 2:
				ADD_WEAK_OR_STRONG_OPROP(otmp, ELEC);
			break;
			case 3:
				ADD_WEAK_OR_STRONG_OPROP(otmp, COLD);
			break;
			case 4:
				add_oprop(otmp, OPROP_VORPW);
			break;
		}
		else if(rn2(3)){
			switch(rn2(3)){
				case 0:
					ADD_WEAK_OR_STRONG_OPROP(otmp, ANAR);
				break;
				case 1:
					ADD_WEAK_OR_STRONG_OPROP(otmp, CONC);
				break;
				case 2:
					ADD_WEAK_OR_STRONG_OPROP(otmp, AXIO);
				break;
			}
		}
	}
	/* ring props (stack with weapon) */
	else if(otmp->oclass == RING_CLASS){
		if(rn2(2)){
			ADD_WEAPON_ARMOR_OPROP(otmp, HOLY);
		}
		if(rn2(3)) switch(rnd(3)){
			case 1:
				ADD_WEAPON_ARMOR_OPROP(otmp, FIRE);
			break;
			case 2:
				ADD_WEAPON_ARMOR_OPROP(otmp, ELEC);
			break;
			case 3:
				ADD_WEAPON_ARMOR_OPROP(otmp, COLD);
			break;
		}
		else if(rn2(3)){
			switch(rn2(3)){
				case 0:
					ADD_WEAPON_ARMOR_OPROP(otmp, ANAR);
				break;
				case 1:
					ADD_WEAPON_ARMOR_OPROP(otmp, CONC);
				break;
				case 2:
					ADD_WEAPON_ARMOR_OPROP(otmp, AXIO);
				break;
			}
		}
		if(!rn2(7)){
			add_oprop(otmp, OPROP_LIFE);
		}
		if(!rn2(7)){
			add_oprop(otmp, OPROP_HEAL);
		}
	}
	return otmp;
}

STATIC_OVL struct obj *
mk_devil_special(otmp)
struct obj *otmp;	/* existing object */
{
	curse(otmp);
	/* materials */
	if(rn2(4)){
		if(otmp->oclass == WEAPON_CLASS || is_weptool(otmp) 
			|| (is_hard(otmp) && otmp->oclass == ARMOR_CLASS && otmp->otyp != ORIHALCYON_GAUNTLETS)
		){
			//Ammo can be stuff like silver arrows, so it should use sim-facing material setting
			if(is_ammo(otmp))
				set_material(otmp, rn2(2) ? GOLD : GREEN_STEEL);
			else
				set_material_gm(otmp, rn2(2) ? GOLD : GREEN_STEEL);
		}
	}
	/* armor props */
	if(otmp->oclass == ARMOR_CLASS){
		if(rn2(4)) switch(rn2(6)){
			case 0:
				ADD_WEAPON_ARMOR_OPROP(otmp, ELEC);
			break;
			case 1:
				ADD_WEAPON_ARMOR_OPROP(otmp, FIRE);
			break;
			case 2:
				ADD_WEAPON_ARMOR_OPROP(otmp, COLD);
			break;
			case 3:
				ADD_WEAPON_ARMOR_OPROP(otmp, MAGC);
			break;
			case 4:
				ADD_WEAPON_ARMOR_OPROP(otmp, UNHY);
			break;
			case 5:
				ADD_WEAPON_ARMOR_OPROP(otmp, AXIO);
			break;
		}
		if(is_gloves(otmp) || is_boots(otmp)){
			if(!rn2(4)){
				add_oprop(otmp, rn2(2) ? OPROP_BLADED : OPROP_SPIKED);
			}
		}
	}
	/* weapon props */
	else if(otmp->oclass == WEAPON_CLASS){
		if(rn2(4)) switch(rn2(8)){
			case 0:
				ADD_WEAK_OR_STRONG_OPROP(otmp, ELEC);
			break;
			case 1:
				ADD_WEAK_OR_STRONG_OPROP(otmp, FIRE);
			break;
			case 2:
				ADD_WEAK_OR_STRONG_OPROP(otmp, COLD);
			break;
			case 3:
				ADD_WEAK_OR_STRONG_OPROP(otmp, MAGC);
			break;
			case 4:
				ADD_WEAK_OR_STRONG_OPROP(otmp, UNHY);
			break;
			case 5:
				ADD_WEAK_OR_STRONG_OPROP(otmp, FLAY);
			break;
			case 6:
				add_oprop(otmp, OPROP_DRANW);
			break;
			case 7:
				ADD_WEAK_OR_STRONG_OPROP(otmp, AXIO);
			break;
		}
	}
	/* ring props (stack with weapon) */
	else if(otmp->oclass == RING_CLASS){
		if(rn2(4)) switch(rn2(6)){
			case 0:
				ADD_WEAPON_ARMOR_OPROP(otmp, ELEC);
			break;
			case 1:
				ADD_WEAPON_ARMOR_OPROP(otmp, FIRE);
			break;
			case 2:
				ADD_WEAPON_ARMOR_OPROP(otmp, COLD);
			break;
			case 3:
				ADD_WEAPON_ARMOR_OPROP(otmp, MAGC);
			break;
			case 4:
				ADD_WEAPON_ARMOR_OPROP(otmp, UNHY);
			break;
			case 5:
				ADD_WEAPON_ARMOR_OPROP(otmp, AXIO);
			break;
		}
	}
	return otmp;
}

STATIC_OVL struct obj *
mk_ancient_special(otmp)
struct obj *otmp;	/* existing object */
{
	otmp = mk_devil_special(otmp);
	/* armor props */
	/* weapon props */
	if(otmp->oclass == WEAPON_CLASS){
		/* Living weapons */
		if(!rn2(20)){
			add_oprop(otmp, OPROP_LIVEW);
		}
		else if(!rn2(20)){
			add_oprop(otmp, OPROP_RETRW);
		}
	}
	return otmp;
}

STATIC_OVL struct obj *
mk_demon_special(otmp)
struct obj *otmp;	/* existing object */
{
	curse(otmp);
	/* materials */
	if(rn2(4)){
		if(otmp->oclass == WEAPON_CLASS || is_weptool(otmp) 
			|| (is_hard(otmp) && otmp->oclass == ARMOR_CLASS && otmp->otyp != ORIHALCYON_GAUNTLETS)
		){
			//Ammo can be stuff like silver arrows, so it should use sim-facing material setting
			if(is_ammo(otmp))
				set_material(otmp, rn2(2) ? GOLD : IRON);
			else
				set_material_gm(otmp, rn2(2) ? GOLD : IRON);
		}
	}
	/* armor props */
	if(otmp->oclass == ARMOR_CLASS){
		if(rn2(4)) switch(rn2(6)){
			case 0:
				ADD_WEAPON_ARMOR_OPROP(otmp, ACID);
			break;
			case 1:
				ADD_WEAPON_ARMOR_OPROP(otmp, FIRE);
			break;
			case 2:
				ADD_WEAPON_ARMOR_OPROP(otmp, COLD);
			break;
			case 3:
				ADD_WEAPON_ARMOR_OPROP(otmp, MAGC);
			break;
			case 4:
				ADD_WEAPON_ARMOR_OPROP(otmp, UNHY);
			break;
			case 5:
				ADD_WEAPON_ARMOR_OPROP(otmp, ANAR);
			break;
		}
		if(is_gloves(otmp) || is_boots(otmp)){
			if(!rn2(2)){
				add_oprop(otmp, rn2(4) ? OPROP_SPIKED : OPROP_BLADED);
			}
		}
	}
	/* weapon props */
	else if(otmp->oclass == WEAPON_CLASS){
		if(rn2(4)) switch(rn2(8)){
			case 0:
				ADD_WEAK_OR_STRONG_OPROP(otmp, ACID);
			break;
			case 1:
				ADD_WEAK_OR_STRONG_OPROP(otmp, FIRE);
			break;
			case 2:
				ADD_WEAK_OR_STRONG_OPROP(otmp, COLD);
			break;
			case 3:
				ADD_WEAK_OR_STRONG_OPROP(otmp, MAGC);
			break;
			case 4:
				ADD_WEAK_OR_STRONG_OPROP(otmp, UNHY);
			break;
			case 5:
				ADD_WEAK_OR_STRONG_OPROP(otmp, FLAY);
			break;
			case 6:
				add_oprop(otmp, OPROP_DRANW);
			break;
			case 7:
				ADD_WEAK_OR_STRONG_OPROP(otmp, ANAR);
			break;
		}
		if(!rn2(10)){
			add_oprop(otmp, rn2(4) ? OPROP_SPIKED : OPROP_BLADED);
		}
	}
	/* ring props (stack with weapon) */
	else if(otmp->oclass == RING_CLASS){
		if(rn2(4)) switch(rn2(6)){
			case 0:
				ADD_WEAPON_ARMOR_OPROP(otmp, ACID);
			break;
			case 1:
				ADD_WEAPON_ARMOR_OPROP(otmp, FIRE);
			break;
			case 2:
				ADD_WEAPON_ARMOR_OPROP(otmp, COLD);
			break;
			case 3:
				ADD_WEAPON_ARMOR_OPROP(otmp, MAGC);
			break;
			case 4:
				ADD_WEAPON_ARMOR_OPROP(otmp, UNHY);
			break;
			case 5:
				ADD_WEAPON_ARMOR_OPROP(otmp, ANAR);
			break;
		}
	}
	return otmp;
}

STATIC_OVL struct obj *
mk_tannin_special(otmp)
struct obj *otmp;	/* existing object */
{
	otmp = mk_demon_special(otmp);
	/* armor props */
	if(otmp->oclass == ARMOR_CLASS){
		if(!rn2(20)){
			add_oprop(otmp, OPROP_GRES);
		}
	}
	/* weapon props */
	else if(otmp->oclass == WEAPON_CLASS){
		/* Living weapons */
		if(!rn2(20)){
			add_oprop(otmp, OPROP_LIVEW);
		}
		else if(!rn2(20)){
			add_oprop(otmp, OPROP_RETRW);
		}
		if(!rn2(20)){
			add_oprop(otmp, OPROP_ASECW);
		}
		else if(!rn2(20)){
			add_oprop(otmp, OPROP_PSECW);
		}
	}
	return otmp;
}

struct obj *
mk_vault_special(otmp, vn)
struct obj *otmp;	/* existing object */
int vn;
{
	int type = -1;
#define VL_TANNIN	0
#define VL_ANCIENT	1
#define VL_ANGEL	2
#define VL_DEVIL	3
#define VL_DEMON	4
#define VL_JRT		5
	if(vn == VN_JRT){
		type = VL_JRT;
	}
	else if(vn < LIMIT_VN_RANGE_1_TANNINIM){
		type = VL_TANNIN;
	}
	else if(vn < LIMIT_VN_RANGE_2_ANCIENT){
		type = VL_ANCIENT;
	}
	else if(vn < LIMIT_VN_RANGE_3_ANGEL){
		type = VL_ANGEL;
	}
	else if(vn < LIMIT_VN_RANGE_4_DEVIL){
		type = VL_DEVIL;
	}
	else if(vn < LIMIT_VN_RANGE_5_DEMON){
		type = VL_DEMON;
	}
	switch(type){
		case VL_JRT:
			otmp = mk_jrt_special(otmp);
		break;
		case VL_ANGEL:
			otmp = mk_holy_special(otmp);
		break;
		case VL_DEVIL:
			otmp = mk_devil_special(otmp);
		break;
		case VL_ANCIENT:
			otmp = mk_ancient_special(otmp);
		break;
		case VL_DEMON:
			otmp = mk_demon_special(otmp);
		break;
		case VL_TANNIN:
			otmp = mk_tannin_special(otmp);
		break;
		default:
			impossible("Unhandled vault number %d for making loot special.", vn);
		break;
	}
	return otmp;
}

struct obj *
mk_special(otmp)
struct obj *otmp;	/* existing object */
{
	int prop;
	
	if(otmp->oclass == WEAPON_CLASS || is_weptool(otmp)){
		prop = rnd(15);
		switch(prop)
		{
		case 1:
			add_oprop(otmp, OPROP_FIREW);
		break;
		case 2:
			add_oprop(otmp, OPROP_COLDW);
		break;
		case 3:
			add_oprop(otmp, OPROP_ELECW);
		break;
		case 4:
			add_oprop(otmp, OPROP_ACIDW);
		break;
		case 5:
			add_oprop(otmp, OPROP_MAGCW);
		break;
		case 6:
			add_oprop(otmp, OPROP_ANARW);
		break;
		case 7:
			add_oprop(otmp, OPROP_CONCW);
		break;
		case 8:
			add_oprop(otmp, OPROP_AXIOW);
		break;
		case 9:
			add_oprop(otmp, OPROP_HOLYW);
		break;
		case 10:
			add_oprop(otmp, OPROP_UNHYW);
		break;
		case 11:
			add_oprop(otmp, OPROP_WATRW);
		break;
		case 12:
			add_oprop(otmp, OPROP_DEEPW);
		break;
		case 13:
			add_oprop(otmp, OPROP_PSIOW);
		break;
		case 14:
			add_oprop(otmp, OPROP_WRTHW);
		break;
		case 15:
			add_oprop(otmp, OPROP_DRANW);
		break;
		}
		if(!rn2(20)){
			add_oprop(otmp, OPROP_LIVEW);
		}
	}
	else if(otmp->oclass == ARMOR_CLASS || otmp->oclass == RING_CLASS){
		prop = rnd(10);
		switch(prop)
		{
		case 1:
			ADD_WEAPON_ARMOR_OPROP(otmp, FIRE);
		break;
		case 2:
			ADD_WEAPON_ARMOR_OPROP(otmp, COLD);
		break;
		case 3:
			ADD_WEAPON_ARMOR_OPROP(otmp, ELEC);
		break;
		case 4:
			ADD_WEAPON_ARMOR_OPROP(otmp, ACID);
		break;
		case 5:
			ADD_WEAPON_ARMOR_OPROP(otmp, MAGC);
		break;
		case 6:
			ADD_WEAPON_ARMOR_OPROP(otmp, ANAR);
		break;
		case 7:
			ADD_WEAPON_ARMOR_OPROP(otmp, CONC);
		break;
		case 8:
			ADD_WEAPON_ARMOR_OPROP(otmp, AXIO);
		break;
		case 9:
			ADD_WEAPON_ARMOR_OPROP(otmp, HOLY);
		break;
		case 10:
			ADD_WEAPON_ARMOR_OPROP(otmp, UNHY);
		break;
		}
	}
	/* ring props (stack with weapon) */
	else if(otmp->oclass == RING_CLASS){
		prop = rnd(13);
		switch(prop)
		{
		case 1:
			ADD_WEAPON_ARMOR_OPROP(otmp, FIRE);
		break;
		case 2:
			ADD_WEAPON_ARMOR_OPROP(otmp, COLD);
		break;
		case 3:
			ADD_WEAPON_ARMOR_OPROP(otmp, ELEC);
		break;
		case 4:
			ADD_WEAPON_ARMOR_OPROP(otmp, ACID);
		break;
		case 5:
			ADD_WEAPON_ARMOR_OPROP(otmp, MAGC);
		break;
		case 6:
			ADD_WEAPON_ARMOR_OPROP(otmp, ANAR);
		break;
		case 7:
			ADD_WEAPON_ARMOR_OPROP(otmp, CONC);
		break;
		case 8:
			ADD_WEAPON_ARMOR_OPROP(otmp, AXIO);
		break;
		case 9:
			ADD_WEAPON_ARMOR_OPROP(otmp, HOLY);
		break;
		case 10:
			ADD_WEAPON_ARMOR_OPROP(otmp, UNHY);
		break;
		case 11:
			ADD_WEAK_OR_STRONG_OPROP(otmp, WATR);
		break;
		case 12:
			ADD_WEAK_OR_STRONG_OPROP(otmp, PSIO);
		break;
		case 13:
			add_oprop(otmp, OPROP_DRANW);
		break;
		}
	}
	
	return otmp;
}

/*
 * Make a low-damage special-but-non-artifact weapon based on otmp
 */

struct obj *
mk_minor_special(otmp)
struct obj *otmp;	/* existing object */
{
	int prop = rnd(12);
	
	if(otmp->oclass == WEAPON_CLASS || is_weptool(otmp) || otmp->oclass == RING_CLASS){
		switch(prop)
		{
		case 1:
			add_oprop(otmp, OPROP_LESSER_FIREW);
		break;
		case 2:
			add_oprop(otmp, OPROP_LESSER_COLDW);
		break;
		case 3:
			add_oprop(otmp, OPROP_LESSER_ELECW);
		break;
		case 4:
			add_oprop(otmp, OPROP_LESSER_ACIDW);
		break;
		case 5:
			add_oprop(otmp, OPROP_LESSER_MAGCW);
		break;
		case 6:
			add_oprop(otmp, OPROP_LESSER_ANARW);
		break;
		case 7:
			add_oprop(otmp, OPROP_LESSER_CONCW);
		break;
		case 8:
			add_oprop(otmp, OPROP_LESSER_AXIOW);
		break;
		case 9:
			add_oprop(otmp, OPROP_LESSER_HOLYW);
		break;
		case 10:
			add_oprop(otmp, OPROP_LESSER_UNHYW);
		break;
		case 11:
			add_oprop(otmp, OPROP_LESSER_WATRW);
		break;
		case 12:
			add_oprop(otmp, OPROP_LESSER_PSIOW);
		break;
		}
	}
	
	return otmp;
}
/*
 * Returns the full name (with articles and correct capitalization) of an
 * artifact named "name" if one exists, or NULL, it not.
 * The given name must be rather close to the real name for it to match.
 * The object type of the artifact is returned in otyp if the return value
 * is non-NULL.
 */
const char*
artifact_name(name, otyp, artinum)
const char *name;
short *otyp;
int *artinum;
{
    register const struct artifact *a;
    register const char *aname;

    if(!strncmpi(name, "the ", 4)) name += 4;

    for (a = artilist+1; a->otyp; a++) {
		aname = a->name;
		if(!strncmpi(aname, "the ", 4)) aname += 4;
		if(!strcmpi(name, aname)) {
			if (a == &artilist[ART_FIRE_BRAND] || a == &artilist[ART_FROST_BRAND])
				*otyp = (u.brand_otyp != STRANGE_OBJECT ? u.brand_otyp : a->otyp);
			else
				*otyp = a->otyp;
			if(artinum)
				*artinum = a - artilist;
			return a->name;
		}
    }
	aname = "Fluorite Octet";
	if(!strcmpi(name, aname)) {
		*otyp = artilist[ART_FLUORITE_OCTAHEDRON].otyp;
		if(artinum) *artinum = ART_FLUORITE_OCTAHEDRON;
		return artilist[ART_FLUORITE_OCTAHEDRON].name;
	}
	aname = "Fluorite Octahedra";
	if(!strcmpi(name, aname)) {
		*otyp = artilist[ART_FLUORITE_OCTAHEDRON].otyp;
		if(artinum) *artinum = ART_FLUORITE_OCTAHEDRON;
		return artilist[ART_FLUORITE_OCTAHEDRON].name;
	}
	aname = "Lancet of Longinus";
	if(!strcmpi(name, aname)) {
		if (Role_if(PM_TOURIST)) *otyp = LIGHTSABER;
		else *otyp = SCALPEL;
		if(artinum) *artinum = ART_LANCE_OF_LONGINUS;
		return artilist[ART_LANCE_OF_LONGINUS].name;
	}
    return (char *)0;
}

boolean
art_already_exists(artinum)
int artinum;
{
	if(artinum < 1 || artinum > nrofartifacts) {
		impossible("bad artifact number %d", artinum);
		return 0;
	}
	return artinstance[artinum].exists;
}

boolean
art_already_exists_byname(otyp, artiname)
int otyp;
const char * artiname;
{
	int i;
	if (otyp && *artiname){
	    for (i = 1; artilist[i].otyp; i++){
			if (artinstance[i].exists && !strcmp(artilist[i].name, artiname))
				return TRUE;
			if ((int) artilist[i].otyp == otyp && !strcmp(artilist[i].name, artiname))
				return artinstance[i].exists;
		}
	}
	return FALSE;
}

int
get_already_exists_byname(otyp, artiname)
int otyp;
const char * artiname;
{
	int i;
	if (otyp && *artiname){
	    for (i = 1; artilist[i].otyp; i++){
			if (artinstance[i].exists && !strcmp(artilist[i].name, artiname))
				return i;
		}
	}
	return 0;
}

void
flag_existance(m, mod)
int m;
int mod;
{
	artinstance[m].exists = mod;
}

void
artifact_exists(otmp, name, mod)
struct obj *otmp;
const char *name;
boolean mod;
{
	const struct artifact *a;

	if (otmp && *name)
	    for (a = artilist+1; a->otyp; a++)
		if ((a->otyp == otmp->otyp || ((is_malleable_artifact(a) || a == &artilist[ART_LANCE_OF_LONGINUS]) && artitypematch(a, otmp))) && !strcmp(a->name, name)) {
		    int m = a - artilist;
		    otmp->oartifact = (mod ? m : 0);
		    otmp->age = 0;
		    if(otmp->otyp == RIN_INCREASE_DAMAGE) otmp->spe = 0;
			/* for "summoned" temporary artifacts, artinstance things are skipped, such as declaring the artifact extant */
			if (!get_ox(otmp, OX_ESUM)) {
				flag_existance(m, mod);
				artinstance[m].spawn_dnum = u.uz.dnum;
				artinstance[m].spawn_dlevel = u.uz.dlevel;
				// artinstance[m].exists = mod;
				if(otmp->oartifact == ART_ROD_OF_SEVEN_PARTS){
					artinstance[ART_ROD_OF_SEVEN_PARTS].RoSPkills = 7;//number of hits untill you gain a +
					artinstance[ART_ROD_OF_SEVEN_PARTS].RoSPflights = 0;//number of flights remaining
				}
				if(otmp->oartifact == ART_TENSA_ZANGETSU){
					artinstance[ART_TENSA_ZANGETSU].ZangetsuSafe = u.ulevel;//turns for which you can use Zangetsu safely
				}
				if(otmp->oartifact == ART_SODE_NO_SHIRAYUKI){
					artinstance[ART_SODE_NO_SHIRAYUKI].SnSd1 = 0;//turn on which you can reuse the first dance
					artinstance[ART_SODE_NO_SHIRAYUKI].SnSd2 = 0;//turn on which you can reuse the second dance
					artinstance[ART_SODE_NO_SHIRAYUKI].SnSd3 = 0;//turn on which you can reuse the third dance
					artinstance[ART_SODE_NO_SHIRAYUKI].SnSd3duration = 0;//turn until which the weapon does full damage
				}
				if(otmp->oartifact == ART_SCORPION_CARAPACE){
					artinstance[ART_SCORPION_CARAPACE].CarapaceLevel = 10;//Starts off at "10th level" Max upgrade points is therefor 20, and it takes a while to earn the first
				}
				if(otmp->oartifact == ART_SILVER_SKY){
					artinstance[ART_SILVER_SKY].GithStyle = 0;
					artinstance[ART_SILVER_SKY].GithStylesSeen = 0;
				}
				if(otmp->oartifact == ART_SKY_REFLECTED){
					artinstance[ART_SKY_REFLECTED].ZerthUpgrades = 0;
					artinstance[ART_SKY_REFLECTED].ZerthOtyp = otmp->otyp;
				}
				if(otmp->oartifact == ART_AMALGAMATED_SKIES){
					artinstance[ART_AMALGAMATED_SKIES].TwinSkiesEtraits = objects[TWO_HANDED_SWORD].expert_traits;
				}
			}
			if(otmp->oartifact && (get_artifact(otmp)->inv_prop == NECRONOMICON || get_artifact(otmp)->inv_prop == SPIRITNAMES)){
				otmp->ovar1_necronomicon = 0;//used to track special powers, via flags
				otmp->spestudied = 0;//use to track number of powers discovered
			} if(otmp->oartifact && get_artifact(otmp)->inv_prop == INFINITESPELLS){
				otmp->ovar1_infinitespells = rn2(SPE_BLANK_PAPER - SPE_DIG) + SPE_DIG;
			}
			if( arti_light(otmp) ){
				begin_burn(otmp);
			}
		    break;
		}
	return;
}

struct obj *
mksartifact(art_id)
int art_id;
{
	struct obj *otmp = mksobj(artilist[art_id].otyp, MKOBJ_NOINIT);
	otmp = oname(otmp, artiname(art_id));
	return otmp;
}

/*
 * Fills an int array of all the properties (that are in prop.h) an artifact has (either while equiped or while carried)
 * If while_carried == TRUE, do not list properties that require the artifact to be wielded / worn
 */
void
get_art_property_list(property_list, oartifact, while_carried)
int * property_list;
int oartifact;
boolean while_carried;
{
	/* quick safety check */
	if (oartifact < 1 || oartifact > nrofartifacts)
	{
		property_list[0] = 0;
		return;
	}

	int cur_prop = NO_PROP;
	int i = 0;
	int j = 0;
	register const struct artifact *oart = &artilist[oartifact];

	for (i = 0; i < MAXARTPROP; i++)
	{
		cur_prop = (while_carried ? oart->cprops[i] : oart->wprops[i]);
		if (cur_prop != NO_PROP)
		{
			property_list[j] = cur_prop;
			j++;
		}
	}
	// add a terminator to the array
	property_list[j] = 0;
	return;
}

int
nartifact_exist()
{
    int a = 0;
    int i;
	for(i = 1; artilist[i].otyp; i++)
		if(artinstance[i].exists && (CountsAgainstGifts(i) /*|| a < 1*/)) a++;

    return max(a-u.regifted, 0);
}
#endif /* OVLB */
#ifdef OVL0

boolean
arti_gen_prop(otmp, flag)
struct obj *otmp;
unsigned long flag;
{
	const struct artifact *arti = get_artifact(otmp);
	return((boolean)(arti && (arti->gflags & flag)));
}

boolean
arti_worn_prop(otmp, flag)
struct obj *otmp;
unsigned long flag;
{
	const struct artifact *arti = get_artifact(otmp);
	return((boolean)(arti && (arti->wflags & flag)));
}

boolean
arti_carry_prop(otmp, flag)
struct obj *otmp;
unsigned long flag;
{
	const struct artifact *arti = get_artifact(otmp);
	return((boolean)(arti && (arti->cflags & flag)));
}

boolean
arti_attack_prop(otmp, flag)
struct obj *otmp;
unsigned long flag;
{
	const struct artifact *arti = get_artifact(otmp);
	return((boolean)(arti && (arti->aflags & flag)));
}

boolean
arti_is_prop(otmp, flag)
struct obj *otmp;
unsigned long flag;
{
	const struct artifact *arti = get_artifact(otmp);
	return((boolean)(arti && (arti->iflags & flag)));
}

/* used so that callers don't need to known about SPFX_ codes */
boolean
confers_luck(obj)
struct obj *obj;
{
    /* might as well check for this too */
    if (obj && obj->otyp == LUCKSTONE) return TRUE;

    return (obj && obj->oartifact && arti_is_prop(obj, ARTI_LUCK));
}

/* used so that callers don't need to known about SPFX_ codes */
boolean
arti_digs(obj)
struct obj *obj;
{
    /* might as well check for this too */
    if (obj && (obj->oclass == WEAPON_CLASS || obj->oclass == TOOL_CLASS) && (objects[obj->otyp].oc_skill == P_PICK_AXE)) return TRUE;

    return (obj && obj->oartifact && arti_is_prop(obj, ARTI_DIG));
}


/* used so that callers don't need to known about SPFX_ codes */
boolean
arti_poisoned(obj)
struct obj *obj;
{
    return (obj && obj->oartifact && ((arti_attack_prop(obj, ARTA_POIS)) || (obj->oartifact == ART_PEN_OF_THE_VOID && obj->ovara_seals&SEAL_YMIR)));
}

/* used so that callers don't need to known about SPFX_ codes */
boolean
arti_silvered(obj)
struct obj *obj;
{
    return (obj && obj->oartifact && (arti_attack_prop(obj, ARTA_SILVER) || 
									  (obj->oartifact == ART_PEN_OF_THE_VOID &&
									   obj->ovara_seals & SEAL_EDEN) ||
									  ((obj->oartifact == ART_HOLY_MOONLIGHT_SWORD) && !obj->lamplit))
			);
}

/* used so that callers don't need to known about SPFX_ codes */
boolean
arti_returning(obj)
struct obj *obj;
{
    return (obj && obj->oartifact && (arti_attack_prop(obj, ARTA_RETURNING)));
}

/* used so that callers don't need to known about SPFX_ codes */
boolean
arti_bright(obj)
struct obj *obj;
{
	if(obj && obj->oartifact == ART_INFINITY_S_MIRRORED_ARC){
		int str = infinity_s_mirrored_arc_litness(obj);
		if (str >= 2)
			return TRUE;
		else if (str >= 1)
			return !rn2(10);
	}
    return (obj && obj->oartifact && (arti_attack_prop(obj, ARTA_BRIGHT) || 
									  (obj->oartifact == ART_PEN_OF_THE_VOID &&
									   obj->ovara_seals & SEAL_JACK)));
}

boolean
arti_shattering(obj)
struct obj *obj;
{
    return (obj && (
		(obj->oartifact && arti_attack_prop(obj, ARTA_SHATTER)) ||
		(is_lightsaber(obj) && litsaber(obj))
	));
}

boolean
arti_disarm(obj)
struct obj *obj;
{
    return (obj && obj->oartifact && arti_attack_prop(obj, ARTA_DISARM));
}

boolean
arti_steal(obj)
struct obj *obj;
{
    return (obj && obj->oartifact && arti_attack_prop(obj, ARTA_STEAL));
}

boolean
arti_tentRod(obj)
struct obj *obj;
{
    return (obj && obj->oartifact && arti_attack_prop(obj, ARTA_TENTROD));
}

boolean
arti_webweaver(obj)
struct obj *obj;
{
    return (obj && obj->oartifact == ART_WEBWEAVER_S_CROOK);
}

boolean
arti_threeHead(obj)
struct obj *obj;
{
    return (obj && obj->oartifact && arti_attack_prop(obj, ARTA_THREEHEAD));
}

boolean
arti_dluck(obj)
struct obj *obj;
{
    return (obj && obj->oartifact && arti_attack_prop(obj, ARTA_DLUCK));
}
boolean
arti_dexpl(obj)
struct obj *obj;
{
    return (obj && obj->oartifact && arti_attack_prop(obj, ARTA_DEXPL));
}

boolean
arti_phasing(obj)
struct obj *obj;
{
    return (obj && (
		(obj->oartifact && arti_attack_prop(obj, ARTA_PHASING)) ||
		(check_oprop(obj, OPROP_ELFLW) && Insight >= 22) ||
		(check_oprop(obj, OPROP_PHSEW)) ||
		(check_oprop(obj, OPROP_RLYHW) && Insight >= 40) ||
		((obj->oartifact == ART_HOLY_MOONLIGHT_SWORD) && obj->lamplit)
	));
}

boolean
arti_mandala(obj)
struct obj *obj;
{
    return (obj && obj->oartifact && arti_is_prop(obj, ARTI_MANDALA));
}

boolean
arti_lighten(obj, while_carried)
struct obj *obj;
boolean while_carried;
{
	return (obj && obj->oartifact &&
		(arti_carry_prop(obj, ARTP_LIGHTEN)
		|| (!while_carried && arti_worn_prop(obj, ARTP_LIGHTEN))));
}

boolean
arti_chawis(obj, while_carried)
struct obj * obj;
boolean while_carried;
{
	return (obj && obj->oartifact &&
		(arti_carry_prop(obj, ARTP_WCATRIB)
		|| (!while_carried && arti_worn_prop(obj, ARTP_WCATRIB))));
}

boolean
arti_forcesight(obj, while_carried)
struct obj * obj;
boolean while_carried;
{
	return (obj && obj->oartifact &&
		(arti_carry_prop(obj, ARTP_FORCESIGHT)
		|| (!while_carried && arti_worn_prop(obj, ARTP_FORCESIGHT))));
}

boolean
arti_plussev(obj)
struct obj *obj;
{
    return (obj && obj->oartifact && arti_is_prop(obj, ARTI_PLUSSEV));
}

boolean
arti_plusten(obj)
struct obj *obj;
{
    return (obj && obj->oartifact && arti_is_prop(obj, ARTI_PLUSTEN));
}

/* used to check if a monster is getting reflection from this object */
/* ASSUMES OBJ IS BEING WORN */
boolean
arti_reflects(obj)
struct obj *obj;
{
	return item_has_property(obj, REFLECTING);
}

#endif /* OVL0 */
#ifdef OVLB

int
artifact_weight(obj)
struct obj *obj;
{
	if(!obj->oartifact)
		return -1;	// error
	int baseweight = objects[obj->otyp].oc_weight;
	int artiweight = get_artifact(obj)->weight;
	int wt = -1;

	if (artiweight == WT_DEFAULT) {
		wt = baseweight;
	}
	else if (artiweight == WT_SPECIAL) {
		/* it needs special handling here */
		switch (obj->oartifact) {
		case ART_ROD_OF_LORDLY_MIGHT:
			wt = objects[MACE].oc_weight;
			break;
		case ART_ANNULUS:
			wt = objects[BELL_OF_OPENING].oc_weight;
			break;
		case ART_SCEPTRE_OF_LOLTH:
			wt = 3*objects[MACE].oc_weight;
			break;
		case ART_ROD_OF_THE_ELVISH_LORDS:
			wt = objects[ELVEN_MACE].oc_weight;
			break;
		case ART_VAMPIRE_KILLER:
			wt = 2*objects[BULLWHIP].oc_weight;
			break;
		case ART_GOLDEN_SWORD_OF_Y_HA_TALLA:
			wt = 2*objects[SCIMITAR].oc_weight;
			break;
		case ART_GREEN_DRAGON_CRESCENT_BLAD:
			wt = 2*objects[NAGINATA].oc_weight;
			break;
		case ART_AEGIS:
			wt = objects[CLOAK].oc_weight;
			break;
		case ART_HERMES_S_SANDALS:
			wt = objects[FLYING_BOOTS].oc_weight;
			break;
		case ART_LYRE_OF_ORPHEUS:
			wt = objects[MAGIC_HARP].oc_weight;
			break;
		case ART_DRAGON_PLATE:
			wt = objects[SILVER_DRAGON_SCALE_MAIL].oc_weight*1.5;
			break;
		case ART_HOLY_MOONLIGHT_SWORD:
			wt = objects[LONG_SWORD].oc_weight*1.5;
			break;
		default:
			impossible("unhandled special artifact weight for %d", obj->oartifact);
			wt = baseweight;
			break;
		}
	}
	else {
		/* explicitly set in artilist.h */
		wt = artiweight;
	}

	if((obj->oartifact == ART_AMALGAMATED_SKIES || obj->oartifact == ART_SKY_REFLECTED) && artinstance[ART_SKY_REFLECTED].ZerthMaterials&ZMAT_MITHRIL){
		wt = (wt+1)/2;//Note: Iron to mithril, not merc to mithril
	}

	return wt;
}

boolean
restrict_name(otmp, name)  /* returns 1 if name is restricted for otmp->otyp */
register struct obj *otmp;
register const char *name;
{
	register const struct artifact *a;
	register const char *aname;

	if (!*name) return FALSE;
	if (!strncmpi(name, "the ", 4)) name += 4;

		/* Since almost every artifact is SPFX_RESTR, it doesn't cost
		   us much to do the string comparison before the spfx check.
		   Bug fix:  don't name multiple elven daggers "Sting".
		 */
	for (a = artilist+1; a->otyp; a++) {
	    /* if (a->otyp != otmp->otyp) continue; */ //don't consider type anymore -CM
		if(a == &artilist[ART_SKY_REFLECTED] && otmp->obj_material != MERCURIAL)
			continue;
	    aname = a->name;
	    if (!strncmpi(aname, "the ", 4)) aname += 4;
	    if (!strcmp(aname, name))
			return ((boolean)( !is_nameable_artifact(a) ||
				otmp->quan > 1L));
	}

	return FALSE;
}

STATIC_OVL boolean
attacks(adtype, otmp)
register int adtype;
register struct obj *otmp;
{
	register const struct artifact *weap;

	if ((weap = get_artifact(otmp)) != 0)
		return((boolean)(weap->adtyp == adtype));
	return FALSE;
}

/*
 * a potential artifact has just been worn/wielded/picked-up or
 * unworn/unwielded/dropped.  Pickup/drop only set/reset the W_ART mask.
 */
void
set_artifact_intrinsic(otmp,on,wp_mask)
register struct obj *otmp;
boolean on;
long wp_mask;
{
	long *mask = 0;
	register const struct artifact *oart = get_artifact(otmp);
	int oartifact = otmp->oartifact;
	uchar dtyp;
	long spfx, spfx2, spfx3, wpfx;
	long exist_warntypem = 0, exist_warntypet = 0, exist_warntypeb = 0, exist_warntypeg = 0, exist_warntypea = 0, exist_warntypev = 0;
	long long exist_montype = 0;
	boolean exist_nonspecwarn;
	int i, j;
	int this_art_property_list[LAST_PROP];
	int tmp_property_list[LAST_PROP];

	if (!oart) return;
	
	/* get the property list (either for the slot or for slotless) */
	get_art_property_list(tmp_property_list, oartifact, wp_mask == W_ART);
	/* copy it to this local array */
	for (i = 0; tmp_property_list[i]; i++)
		this_art_property_list[i] = tmp_property_list[i];	// set indices
	this_art_property_list[i] = 0;							// add null terminator

	/* loop through all properties the artifact gives */
	for (i = 0; this_art_property_list[i]; i++)
	{
		/* select property field from uprops */
		mask = &(u.uprops[this_art_property_list[i]].extrinsic);

		/* if we are removing the artifact and we are dealing with W_ART,
		   we need to check if the property is being provided from another source in the same slot
		   if so, we need to leave the mask alone */
		if (mask && !on && (wp_mask == W_ART || this_art_property_list[i] == WARN_OF_MON))
		{
			boolean got_prop = FALSE;
			register struct obj* obj;
			for (obj = invent; (obj && !got_prop); obj = obj->nobj)
			if (obj != otmp && obj->oartifact) {
				/* write over tmp_property_list with the carried artifact -- this is why we needed a copy earlier */
				get_art_property_list(tmp_property_list, obj->oartifact, TRUE);

				/* specific-monster warning needs to be specially handled */
				if (this_art_property_list[i] == WARN_OF_MON)
				{
					/* check that the artifact is indeed granting WARN_OF_MON -- some artifacts are hateful without granting warning! */
					for (j = 0; tmp_property_list[j]; j++) {
						if(tmp_property_list[j] == WARN_OF_MON)
							got_prop = TRUE;
					}
					if (!got_prop && (obj->owornmask & *mask)) {
						get_art_property_list(tmp_property_list, obj->oartifact, FALSE);
						for (j = 0; tmp_property_list[j]; j++) {
							if(tmp_property_list[j] == WARN_OF_MON)
								got_prop = TRUE;
						}
					}

					if (got_prop) {
						/* add to the types of things we are warned about, to handle later */
						exist_warntypem |= spec_mm(obj->oartifact);
						exist_warntypet |= spec_mt(obj->oartifact);
						exist_warntypet |= spec_mf(obj->oartifact);
						exist_warntypeb |= spec_mb(obj->oartifact);
						exist_warntypeg |= spec_mg(obj->oartifact);
						exist_warntypea |= spec_ma(obj->oartifact);
						exist_warntypev |= spec_mv(obj->oartifact);
						exist_montype |= (long long int)((long long int)1 << (int)(spec_s(obj->oartifact)));
						got_prop = FALSE;	/* continue checking all items */ 
					}

				}
				/* everything else is just a standard property */
				else
				{
					for (j = 0; tmp_property_list[j]; j++)
					{
						if (tmp_property_list[j] == this_art_property_list[i])
							got_prop = TRUE;
					}
				}
			}
			if (got_prop)
				continue;	// do not modify mask for this property
		}

		/* time to modify the mask */
		switch (this_art_property_list[i])
		{
		/* specific warning */
		case WARN_OF_MON:
			/* most specific flags */
			/*  flag                 if on  {add to mask     ; add to warning type                   } else {remove from warning type unless another art fills in       ; remove from mask           } */
			if (spec_mm(oartifact)) {if(on) {*mask |= wp_mask; flags.warntypem |= spec_mm(oartifact);} else {flags.warntypem &= ~(spec_mm(oartifact)&(~exist_warntypem)); *mask &= (~wp_mask)|W_ART;}}
			if (spec_mt(oartifact)) {if(on) {*mask |= wp_mask; flags.warntypet |= spec_mt(oartifact);} else {flags.warntypet &= ~(spec_mt(oartifact)&(~exist_warntypet)); *mask &= (~wp_mask)|W_ART;}}
			if (spec_mf(oartifact)) {if(on) {*mask |= wp_mask; flags.warntypet |= spec_mf(oartifact);} else {flags.warntypet &= ~(spec_mf(oartifact)&(~exist_warntypet)); *mask &= (~wp_mask)|W_ART;}}
			if (spec_mb(oartifact)) {if(on) {*mask |= wp_mask; flags.warntypeb |= spec_mb(oartifact);} else {flags.warntypeb &= ~(spec_mb(oartifact)&(~exist_warntypeb)); *mask &= (~wp_mask)|W_ART;}}
			if (spec_mg(oartifact)) {if(on) {*mask |= wp_mask; flags.warntypeg |= spec_mg(oartifact);} else {flags.warntypeg &= ~(spec_mg(oartifact)&(~exist_warntypeg)); *mask &= (~wp_mask)|W_ART;}}
			if (spec_ma(oartifact)) {if(on) {*mask |= wp_mask; flags.warntypea |= spec_ma(oartifact);} else {flags.warntypea &= ~(spec_ma(oartifact)&(~exist_warntypea)); *mask &= (~wp_mask)|W_ART;}}
			if (spec_mv(oartifact)) {if(on) {*mask |= wp_mask; flags.warntypev |= spec_mv(oartifact);} else {flags.warntypev &= ~(spec_mv(oartifact)&(~exist_warntypev)); *mask &= (~wp_mask)|W_ART;}}
			/* monster symbol */
			if (spec_s(oartifact)) {
				if (on) {
					*mask |= wp_mask;
					flags.montype |= (long long int)((long long int)1 << (int)(spec_s(oartifact))); //spec_s(oartifact);
				}
				else {
					*mask &= (~wp_mask)|W_ART;
					flags.montype &= ~((long long int)((long long int)1 << (int)(spec_s(oartifact)))&(~exist_montype));
				}
			}
			/* update vision */
			see_monsters();	// it should be fine to run a vision update even if there wasn't a change

			/* if there are no specific warnings remaining, toggle off the extrinsic entirely */
			if (!(flags.warntypem || flags.warntypet || flags.warntypeb || flags.warntypeg || flags.warntypea || flags.warntypev || flags.montype))
				*mask = 0L;
			break;
		/* hallucination */
		case HALLUC_RES:
			/* make_hallucinated must (re)set the mask itself to get
			* the display right */
			/* restoring needed because this is the only artifact intrinsic
			* that can print a message--need to guard against being printed
			* when restoring a game
			*/
			(void)make_hallucinated((long)!on, restoring ? FALSE : TRUE, wp_mask);
			break;
		/* needs full vision update*/
		case XRAY_VISION:
			if (otmp->oartifact == ART_AXE_OF_THE_DWARVISH_LORDS && !Race_if(PM_DWARF))
				break;	/* do not modify xray vision */

			if (on) *mask |= wp_mask;
			else *mask &= ~wp_mask;

			vision_full_recalc = 1;
			break;
		/* needs monster vision update */
		case WARNING:
		case TELEPAT:
			if (on) *mask |= wp_mask;
			else *mask &= ~wp_mask;
			see_monsters();
			break;
		/* everything else (that are in uprops) */
		default:
			if (on) *mask |= wp_mask;
			else *mask &= ~wp_mask;
			break;
		}//end switch
	}//end for

	/* things that aren't properties at all */

	/* assumes there can only be one source of forcesight at once! */
	if (arti_forcesight(otmp, wp_mask == W_ART))
		forcesight = on;

	if(wp_mask == W_ART && !on && oart->inv_prop) {
	    /* might have to turn off invoked power too */
	    if (oart->inv_prop <= LAST_PROP &&
		(u.uprops[oart->inv_prop].extrinsic & W_ARTI))
		(void) arti_invoke(otmp);
	}
}

/*
 * creature (usually player) tries to touch (pick up or wield) an artifact obj.
 * Returns 0 if the object refuses to be touched.
 * This routine does not change any object chains.
 * Ignores such things as gauntlets, assuming the artifact is not
 * fooled by such trappings.
 */
int
touch_artifact(obj, mon, hypothetical)
    struct obj *obj;
    struct monst *mon;
	int hypothetical;
{
    register const struct artifact *oart = get_artifact(obj);
    boolean badclass=0, badalign=0, self_willed=0, yours, forceEvade = FALSE;

    if(!oart) return 1;

    yours = (mon == &youmonst);
    /* all quest artifacts are self-willed; it this ever changes, `badclass'
       will have to be extended to explicitly include quest artifacts */
    self_willed = ((oart->gflags & ARTG_MAJOR) != 0);
    if (yours) {
#ifdef RECORD_ACHIEVE
		if(!hypothetical){
			if(obj->oartifact == ART_ROD_OF_SEVEN_PARTS)
				give_law_trophy();
			if(obj->oartifact == ART_SILVER_KEY || obj->oartifact == ART_HAND_MIRROR_OF_CTHYLLA)
				give_neutral_trophy();
			if(obj->oartifact == ART_SHARD_FROM_MORGOTH_S_CROWN)
				give_mordor_trophy();
		}
#endif
		if(Role_if(PM_EXILE) && !hypothetical){
			if(obj->oartifact == ART_ROD_OF_SEVEN_PARTS && !(u.specialSealsKnown&SEAL_MISKA)){
				pline("There is a seal on the tip of the Rod! You can't see it, you know it's there, just the same.");
				pline("You learn a new seal!");
				u.specialSealsKnown |= SEAL_MISKA;
			}
			else if(obj->oartifact == ART_BLACK_CRYSTAL && !(u.specialSealsKnown&SEAL_COSMOS)){
				pline("There is a seal in the heart of the crystal, shining bright through the darkness.");
				pline("You learn a new seal!");
				u.specialSealsKnown |= SEAL_COSMOS;
			}
			else if(obj->oartifact == ART_SHARD_FROM_MORGOTH_S_CROWN && !(u.specialSealsKnown&SEAL_TWO_TREES)){
				pline("There is a seal in the space enclosed by the ring, shining in the darkness.");
				pline("You learn a new seal!");
				u.specialSealsKnown |= SEAL_TWO_TREES;
			}
			else if(obj->oartifact == ART_HAND_MIRROR_OF_CTHYLLA && !(u.specialSealsKnown&SEAL_NUDZIRATH)){
				pline("The cracks on the mirror's surface form part of a seal.");
				pline("In fact, you realize that all cracked and broken mirrors everywhere together are working towards writing this seal.");
				pline("With that realization comes knowledge of the seal's final form!");
				u.specialSealsKnown |= SEAL_NUDZIRATH;
			}
			else if(obj->oartifact == ART_STAFF_OF_TWELVE_MIRRORS && !(u.specialSealsKnown&SEAL_NUDZIRATH)){
				pline("The cracks on the mirrors' surfaces form part of a seal.");
				pline("In fact, you realize that all cracked and broken mirrors everywhere together are working towards writing this seal.");
				pline("With that realization comes knowledge of the seal's final form!");
				u.specialSealsKnown |= SEAL_NUDZIRATH;
			}
			else if(obj->oartifact == ART_INFINITY_S_MIRRORED_ARC && !(u.specialSealsKnown&SEAL_NUDZIRATH)){
				pline("The cracks on the thin mirrored arcs form part of a seal.");
				pline("In fact, you realize that all cracked and broken mirrors everywhere together are working towards writing this seal.");
				pline("With that realization comes knowledge of the seal's final form!");
				u.specialSealsKnown |= SEAL_NUDZIRATH;
			}
			else if(obj->oartifact == ART_SANSARA_MIRROR && !(u.specialSealsKnown&SEAL_NUDZIRATH)){
				pline("The cracks on the gold-mirrored blade form part of a seal.");
				pline("In fact, you realize that all cracked and broken mirrors everywhere together are working towards writing this seal.");
				pline("With that realization comes knowledge of the seal's final form!");
				u.specialSealsKnown |= SEAL_NUDZIRATH;
			}
			else if(obj->oartifact == ART_MIRRORED_MASK && obj->corpsenm == NON_PM && !(u.specialSealsKnown&SEAL_NUDZIRATH)){
				pline("The cracks on the otherwise-blank silver face form part of a seal.");
				pline("In fact, you realize that all cracked and broken mirrors everywhere together are working towards writing this seal.");
				pline("With that realization comes knowledge of the seal's final form!");
				u.specialSealsKnown |= SEAL_NUDZIRATH;
			}
		}
		if(oart->otyp == UNICORN_HORN){
			badclass = TRUE; //always get blasted by unicorn horns.  
							//They've been removed from the unicorn, after all -D_E
			badalign = !(oart->gflags & ARTG_NAME) && oart->alignment != A_NONE &&
			   (oart->alignment == -1*u.ualign.type); //Unicorn horns blast OPOSITE alignment alignment -D_E
														//Neutral horns blast NEUTRAL.
		} else if(obj->oartifact == ART_CLOAK_OF_THE_CONSORT){
			if(flags.female) badalign = badclass = TRUE;
			else if(!Race_if(PM_DROW)){
				badclass = TRUE;
				badalign = FALSE;
			}
		} else if(obj->oartifact == ART_FINGERPRINT_SHIELD && u.yog_sothoth_atten){
			badclass = FALSE;
			badalign = FALSE;
		} else {
			if(!Role_if(PM_BARD)) badclass = self_willed && 
			   (((oart->role != NON_PM && !Role_if(oart->role)) &&
				 (oart->role != NON_PM && !Pantheon_if(oart->role))) ||
			    (oart->race != NON_PM && !Race_if(oart->race)));
			badalign = !(oart->gflags & ARTG_NAME) && oart->alignment != A_NONE &&
			   ((oart->alignment != u.ualign.type && obj->otyp != HELM_OF_OPPOSITE_ALIGNMENT) || u.ualign.record < 0);
			if(badalign && !badclass && self_willed && oart->role == NON_PM 
				&& oart->race == NON_PM
			) badclass = TRUE;
			
			if(oart == &artilist[ART_KUSANAGI_NO_TSURUGI] && !(u.ulevel >= 22 || u.uhave.amulet)){
				badclass = TRUE;
				badalign = TRUE;
			}
		}
	} else if (!is_covetous(mon->data) && !is_mplayer(mon->data)) {
		if(oart->otyp != UNICORN_HORN){
			badclass = self_willed &&
				   ((oart->role != NON_PM && mon->mtyp != oart->role && 
				   !(oart == &artilist[ART_EXCALIBUR] || oart == &artilist[ART_CLARENT]))
				   ||
				   (oart->race != NON_PM && ((mons[oart->race].mflagsa & mon->data->mflagsa) == 0)));
			if(get_mx(mon, MX_EPRI)){
				badalign = !(oart->gflags & ARTG_NAME) && oart->alignment != A_NONE &&
				   (oart->alignment != sgn(EPRI(mon)->shralign));
			} else if(get_mx(mon, MX_EMIN)){
				badalign = !(oart->gflags & ARTG_NAME) && oart->alignment != A_NONE &&
				   (oart->alignment != sgn(EMIN(mon)->min_align));
			} else {
				badalign = !(oart->gflags & ARTG_NAME) && oart->alignment != A_NONE &&
				   (oart->alignment != sgn(mon->data->maligntyp));
			}
		}
		else{/* Unicorn horns */
			badclass = TRUE;
			if(get_mx(mon, MX_EPRI)) {
				badalign = oart->alignment != A_NONE && (oart->alignment != sgn(EPRI(mon)->shralign));
			} else if(get_mx(mon, MX_EMIN)){
				badalign = oart->alignment != A_NONE && (oart->alignment != sgn(EMIN(mon)->min_align));
			} else {
				badalign = oart->alignment != A_NONE && (oart->alignment != sgn(mon->data->maligntyp));
			}
		}
    } else {    /* an M3_WANTSxxx monster or a fake player */
		/* special monsters trying to take the Amulet, invocation tools or
		   quest item can touch anything except for `spec_applies' artifacts */
		if(mon->mtyp == PM_WARDEN_ARIANNA && obj->oartifact == ART_IRON_SPOON_OF_LIBERATION)
			badclass = badalign = forceEvade = TRUE;
		else badclass = badalign = FALSE;
    }
    /* weapons which attack specific categories of monsters are
       bad for them even if their alignments happen to match */
    if (!badalign && (arti_attack_prop(obj, ARTA_HATES) != 0) && obj->oartifact != ART_LIFEHUNT_SCYTHE) {
		/* spec_applies for hateful artifacts should always return FALSE if mon isn't hated. */
		/* a hateful artifact should never apply to non-hated foes */
		badalign = spec_applies(obj, mon, TRUE);
    }
	if(badalign && yours && Role_if(PM_EXILE)){
		if(u.specialSealsActive&SEAL_ALIGNMENT_THING){
			badalign = FALSE;
			// badclass = FALSE;
		}
		else if(oart->alignment == A_LAWFUL && (u.specialSealsActive&SEAL_COSMOS || u.specialSealsActive&SEAL_LIVING_CRYSTAL || u.specialSealsActive&SEAL_TWO_TREES)){
			badalign = FALSE;
			// badclass = FALSE;
		}
		else if(oart->alignment == A_CHAOTIC && u.specialSealsActive&SEAL_MISKA){
			badalign = FALSE;
			// badclass = FALSE;
		}
		else if(oart->alignment == A_NEUTRAL && u.specialSealsActive&SEAL_NUDZIRATH){
			badalign = FALSE;
			// badclass = FALSE;
		}
	}
	if(mon->mtyp == PM_DAEMON) badclass = badalign = FALSE;
	
	if(obj->oartifact >= ART_BLACK_CRYSTAL && obj->oartifact <= ART_AIR_CRYSTAL){
		if(obj->oartifact == ART_BLACK_CRYSTAL){
			if(yours) badclass = badalign = forceEvade = !(PURIFIED_CHAOS);
			else badclass = badalign = forceEvade = TRUE;
		} else if(obj->oartifact == ART_EARTH_CRYSTAL){
			if(yours) badclass = badalign = forceEvade = !PURIFIED_EARTH;
			else badclass = badalign = forceEvade = !!(PURIFIED_EARTH);
		} else if(obj->oartifact == ART_FIRE_CRYSTAL){
			if(yours) badclass = badalign = forceEvade = !PURIFIED_FIRE;
			else badclass = badalign = forceEvade = !!(PURIFIED_FIRE);
		} else if(obj->oartifact == ART_WATER_CRYSTAL){
			if(yours) badclass = badalign = forceEvade = !PURIFIED_WATER;
			else badclass = badalign = forceEvade = !!(PURIFIED_WATER);
		} else if(obj->oartifact == ART_AIR_CRYSTAL){
			if(yours) badclass = badalign = forceEvade = !PURIFIED_WIND;
			else badclass = badalign = forceEvade = !!(PURIFIED_WIND);
		}
	}
	
	if(obj->oartifact == ART_KUSANAGI_NO_TSURUGI && badalign){
		if (yours && !hypothetical)
			pline("You have betrayed what you stood for, and are no longer worthy of even bearing the sword.");
		forceEvade = TRUE;
	}
	
	if(obj->oartifact == ART_DIRGE && (badalign || badclass)){
		forceEvade = TRUE;
	}
	
	if (((badclass || badalign) && self_willed) ||
       (badalign && (!yours || !rn2(4)))
	   )  {
		int dmg;
		char buf[BUFSZ];
	
		if (!yours) return 0;
		if(!hypothetical){
			You("are blasted by %s power!", s_suffix(the(xname(obj))));
			// pline("class: %d align: %d", badclass, badalign);
			dmg = d((Antimagic ? 2 : 4), (self_willed ? 10 : 4));
			Sprintf(buf, "touching %s", oart->name);
			losehp(dmg, buf, KILLED_BY);
			exercise(A_WIS, FALSE);
		}
    }

    /* can pick it up unless you're totally non-synch'd with the artifact */
    if ((badclass && badalign && self_willed) || forceEvade) {
		if (yours && !hypothetical) pline("%s your grasp!", Tobjnam(obj, "evade"));
		return 0;
    } //Clarent sticks itself into it's surroundings if dropped
//	if (oart == &artilist[ART_CLARENT]  && (!yours || ) )
#ifdef CONVICT
    /* This is a kludge, but I'm not sure where else to put it */
    if (oart == &artilist[ART_IRON_BALL_OF_LEVITATION] && !hypothetical) {
		if (Role_if(PM_CONVICT) && (!obj->oerodeproof)) {
			obj->oerodeproof = TRUE;
		}

		if (Punished && (obj != uball)) {
			unpunish(); /* Remove a mundane heavy iron ball */
		}
    }
#endif /* CONVICT */

    return 1;
}

#endif /* OVLB */
#ifdef OVL1

/* decide whether an artifact's special attacks apply against mdef */
int
spec_applies(otmp, mdef, narrow_only)
struct obj * otmp;
struct monst *mdef;
boolean narrow_only;
{
	register const struct artifact *weap = get_artifact(otmp);
	struct permonst *ptr;
	boolean youdef;

	if (on_level(&spire_level,&u.uz))
		return FALSE;

	/* requires some kind of offense in the artilist block */
	if (!weap || !(weap->adtyp || weap->accuracy || weap->damage))
		return FALSE;

	/* special cases */
	if (weap->name == artilist[ART_PEN_OF_THE_VOID].name) {
		if (narrow_only)
			return narrow_voidPen_hit(mdef, otmp);
		else
			return (mvitals[PM_ACERERAK].died > 0);
	}
	
	
	/* artifacts that deal physical bonus damage and aren't limited to a specific group of foes are always applicable */
	if (weap->adtyp == AD_PHYS && !(weap->aflags&ARTA_HATES))
		return (!narrow_only);

	if(!mdef)
		return FALSE; //Invoked with a null monster while calculating hypothetical data (I think)

	youdef = (mdef == &youmonst);
	if(youdef) ptr = youracedata;
	else ptr = mdef->data;
	
	/* artifacts that ONLY apply to an unusual group */
	if(weap->name == artilist[ART_LIFEHUNT_SCYTHE].name)
		return (!is_unalive(ptr));
	if(weap->name == artilist[ART_HOLY_MOONLIGHT_SWORD].name)
		return !(Magic_res(mdef));
	if(weap->name == artilist[ART_PROFANED_GREATSCYTHE].name)
		return (!is_unalive(ptr));
	/* artifacts that ALSO apply to an unusual group */
	if(weap->name == artilist[ART_GIANTSLAYER].name && bigmonst(ptr))
		return TRUE;
	if(weap->name == artilist[ART_STING].name && webmaker(ptr))
		return TRUE;
	
	/* elements can be resisted */
	if (weap->adtyp != AD_PHYS && weap->adtyp != AD_STDY) {
		switch(weap->adtyp) 
		{
		case AD_FIRE:
			if (Fire_res(mdef))
				return FALSE;
		break;
		case AD_COLD:
			if (Cold_res(mdef))
				return FALSE;
		break;
		case AD_ELEC:
			if (Shock_res(mdef))
				return FALSE;
		break;
		case AD_ACID:
			if (Acid_res(mdef))
				return FALSE;
		break;
		case AD_MAGM:
			if (Magic_res(mdef))
				return FALSE;
			/* monsters can save via MR against magic damage? */
			if (mdef != &youmonst && rn2(100) < ptr->mr)
				return FALSE;
		break;
		case AD_DRST:
			if (Poison_res(mdef))
				return FALSE;
		break;
		case AD_DRLI:
			if (Drain_res(mdef))
				return FALSE;
		break;
		case AD_STON:
			/* nothing does this */
			if (Stone_res(mdef))
				return FALSE;
		break;
		case AD_DARK:
			if (Dark_res(mdef))
				return FALSE;
		break;
		case AD_BLUD:
			if (!has_blood_mon(mdef))
				return FALSE;
		break;
		case AD_SLEE:
			if (Sleep_res(mdef))
				return FALSE;
		break;
		case AD_STAR:
			if (!hates_silver(ptr))
				return FALSE;
		break;
		case AD_HLUH:
			if (!hates_holy_mon(mdef) && !hates_unholy_mon(mdef))
				return FALSE;
		break;
		case AD_HOLY:
			if (hates_unholy_mon(mdef))
				return FALSE;
		break;
		default:
			impossible("Weird weapon special attack: (%d).", weap->adtyp);
		}
		/* if it bypassed resistances, it still needs to pass ARTA_HATES (if applicable) */
	}
	
	/* if the artifact is hateful, it only applies its damage to the specific monsters it hates */
	/* this block always returns if entered */
	if (weap->aflags & ARTA_HATES) {
		/* letter */
		if (weap->mtype != 0L && (weap->mtype == (unsigned long)ptr->mlet)) {
			return TRUE;
		}
		/* movement */
		if (weap->mflagsm != 0L && ((ptr->mflagsm & weap->mflagsm) != 0L)) {
			return TRUE;
		}
		/* thinking */
		if (weap->mflagst != 0L && ((ptr->mflagst & weap->mflagst) != 0L)) {
			return TRUE;
		}
		/* fighting */
		if (weap->mflagsf != 0L && ((ptr->mflagsf & weap->mflagsf) != 0L)) {
			return TRUE;
		}
		/* body */
		if (weap->mflagsb != 0L && ((ptr->mflagsb & weap->mflagsb) != 0L)) {
			return TRUE;
		}
		/* game mechanics */
		if (weap->mflagsg != 0L && ((ptr->mflagsg & weap->mflagsg) != 0L)) {
			return TRUE;
			if (youdef && Role_if(PM_NOBLEMAN) && ((weap->mflagsg & (MG_PRINCE | MG_LORD)) != 0))
				return TRUE;
		}
		/* vision */
		if (weap->mflagsv != 0L && ((ptr->mflagsv & weap->mflagsv) != 0L)) {
			return TRUE;
		}
		/* race */
		if (weap->mflagsa != 0L){
			if ((weap->name == artilist[ART_SCOURGE_OF_LOLTH].name) && is_drow(ptr))
				; // skip; the scourge of lolth hates Elves but not Drow.
			else if ((weap->mflagsa & MA_UNDEAD) && is_undead(ptr))
				return TRUE;
			else if (
				(ptr->mflagsa & weap->mflagsa) ||
				(youdef &&
					((!Upolyd && (urace.selfmask & weap->mflagsa)) ||
					((weap->mflagsa & MA_WERE) && u.ulycn >= LOW_PM))
					)
				)
				return TRUE;
		}
		/* alignment */
		if (weap->aflags & ARTA_CROSSA && (youdef ? (u.ualign.type != weap->alignment) :
			(ptr->maligntyp == A_NONE ||
			sgn(ptr->maligntyp) != weap->alignment))
			) {
			return TRUE;
		}

		/* otherwise, no the artifact does not apply! */
		return FALSE;
	}

	return TRUE; //Made it through all checks above, so return true now
}

/* return the MM flags of monster that an artifact's special attacks apply against */
long
spec_mm(oartifact)
int oartifact;
{
	register const struct artifact *artifact = &artilist[oartifact];
	if (artifact && artifact->mflagsm)
		return artifact->mflagsm;
	return 0L;
}

/* return the MT flags of monster that an artifact's special attacks apply against */
long
spec_mt(oartifact)
int oartifact;
{
	register const struct artifact *artifact = &artilist[oartifact];
	if (artifact && artifact->mflagst)
		return artifact->mflagst;
	return 0L;
}

/* return the MF flags of monster that an artifact's special attacks apply against */
long
spec_mf(oartifact)
int oartifact;
{
	register const struct artifact *artifact = &artilist[oartifact];
	if (artifact && artifact->mflagsf)
		return artifact->mflagsf;
	return 0L;
}

/* return the MB flags of monster that an artifact's special attacks apply against */
long
spec_mb(oartifact)
int oartifact;
{
	register const struct artifact *artifact = &artilist[oartifact];
	if (artifact && artifact->mflagsb)
		return artifact->mflagsb;
	return 0L;
}

/* return the MG flags of monster that an artifact's special attacks apply against */
long
spec_mg(oartifact)
int oartifact;
{
	register const struct artifact *artifact = &artilist[oartifact];
	if (artifact && artifact->mflagsg)
		return artifact->mflagsg;
	return 0L;
}

/* return the MA flags of monster that an artifact's special attacks apply against */
long
spec_ma(oartifact)
int oartifact;
{
	register const struct artifact *artifact = &artilist[oartifact];
	if (artifact && artifact->mflagsa)
		return artifact->mflagsa;
	return 0L;
}

/* return the MV flags of monster that an artifact's special attacks apply against */
long
spec_mv(oartifact)
int oartifact;
{
	register const struct artifact *artifact = &artilist[oartifact];
	if (artifact && artifact->mflagsv)
		return artifact->mflagsv;
	return 0L;
}

/* return the S number of monster that an artifact's special attacks apply against */
long
spec_s(oartifact)
int oartifact;
{
	register const struct artifact *artifact = &artilist[oartifact];
	if (artifact && artifact->mtype)
		return artifact->mtype;
	return 0L;
}

/* special attack bonus */
int
spec_abon(otmp, mon, youagr)
struct obj *otmp;
struct monst *mon;
boolean youagr;
{
	const struct artifact *weap = get_artifact(otmp);
	int bonus = 0;
	/* no need for an extra check for `NO_ATTK' because this will
	   always return 0 for any artifact which has that attribute */
	if(otmp->oartifact == ART_BLACK_ARROW){
		return 1000;
	}
	if(otmp->oartifact == ART_GUNGNIR){
		bonus = 50;
	}
	if(youagr && Role_if(PM_BARD)) //legend lore
		bonus += 5;

	if(weap && (weap->inv_prop == GITH_ART || weap->inv_prop == ZERTH_ART || weap->inv_prop == AMALGUM_ART) && (artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_STEEL))
		bonus += otmp->spe+1;
	
	if(youagr && Role_if(PM_PRIEST)) return bonus + weap->accuracy; //priests always get the maximum to-hit bonus.
	
	if (weap && weap->accuracy && spec_applies(otmp, mon, FALSE))
	    return bonus + rnd((int)weap->accuracy);
	
	return bonus;
}

int
get_premium_heart_multiplier()
{
	int multiplier = 1;
	if (!Upolyd && u.uhp<u.uhpmax / 4) multiplier++;
	if (!Upolyd && u.uhp<u.uhpmax / 16) multiplier++;
	if (Insanity > 50 && !ClearThoughts) multiplier++;
	if (Insanity > 75 && !ClearThoughts) multiplier++;
	if (NightmareAware_Insanity >= 10 && ClearThoughts && Nightmare) multiplier++;
	if (Blind) multiplier++;
	if (Stunned) multiplier++;
	if (Confusion) multiplier++;
	if (Doubt) multiplier++;
	if (Punished) multiplier++;
	if (Sick) multiplier++;
	if (Stoned) multiplier++;
	if (Golded) multiplier++;
	if (Salted) multiplier++;
	if (Strangled) multiplier++;
	if (Vomiting) multiplier++;
	if (Slimed) multiplier++;
	if (FrozenAir) multiplier++;
	if (BloodDrown) multiplier++;
	if (Hallucination) multiplier++;
	if (Fumbling) multiplier++;
	if (Wounded_legs) multiplier++;
	if (Panicking) multiplier++;
	if (StumbleBlind) multiplier++;
	if (StaggerShock) multiplier++;
	if (Babble) multiplier++;
	if (Screaming) multiplier++;
	if (FaintingFits) multiplier++;

	return multiplier;
}

/* special damage bonus */
/* returns FALSE if no bonus damage was applicable */
boolean
spec_dbon(otmp, mon, basedmg, plusdmgptr, truedmgptr)
struct obj *otmp;
struct monst *mon;
int basedmg;
int * plusdmgptr;
int * truedmgptr;
{
	const struct artifact *weap = get_artifact(otmp);
	int damd = (int)weap->damage;
	boolean goodpointers = (plusdmgptr && truedmgptr);
	
	/* check that we were given an artifact */
	if (!weap)
		return FALSE;
	
	/* This is a super awkward property to check for :( */
	if(weap->inv_prop == RINGED_SPEAR){
		if (!Fire_res(mon) && artinstance[otmp->oartifact].RRSember >= moves)
			*truedmgptr += basedmg;
		if (!Magic_res(mon) && artinstance[otmp->oartifact].RRSlunar >= moves)
			*truedmgptr += basedmg;
	}
	
	/* check that the artifact is offensive */
	if (!(weap->adtyp || weap->damage || weap->accuracy))
		return FALSE;

	/* The Annulus is a 2x damage artifact if it isn't a lightsaber */
	if(otmp->oartifact == ART_ANNULUS && !is_lightsaber(otmp))
		damd = 0;
	
	/* The black arrow deals 4x damage + 108, and overkills Smaug */
	if (otmp->oartifact == ART_BLACK_ARROW) {
		if (goodpointers) {
			*plusdmgptr += basedmg * 3 + 108;
			if (mon->mtyp == PM_SMAUG)
				*truedmgptr += mon->mhpmax;
		}
		/* return immediately */
		return TRUE;
	}
	
	spec_dbon_applies = spec_applies(otmp, mon, FALSE)
			&& !(
			/* additional conditions required to deal bonus damage */
			(weap->inv_prop == ICE_SHIKAI && artinstance[otmp->oartifact].SnSd3duration < monstermoves) /* only while invoked */
			||	(otmp->oartifact == ART_HOLY_MOONLIGHT_SWORD && !otmp->lamplit)	/* only while lit */
			);
	/* determine if we will apply bonus damage */
	if ((spec_dbon_applies && !(
			(weap->adtyp == AD_SLEE) ||	(weap->adtyp == AD_STDY)	/* doesn't do damage */
		)) || (
		/* alternative conditions for dealing bonus damage (ie spec_dbon_applies == FALSE) */
		(otmp->oartifact == ART_FIRE_BRAND) ||
		(otmp->oartifact == ART_FROST_BRAND)
		)
	){
		int multiplier = 1;
		int dmgtomulti = basedmg;

		/* premium heart has special cases to get a huge damage multiplier -- player only */
		if (otmp==uarmg && otmp->oartifact == ART_PREMIUM_HEART)
			multiplier = get_premium_heart_multiplier();
		
		if(otmp->oartifact == ART_BLOODLETTER){
		 	int *hp = (Upolyd) ? (&u.mh) : (&u.uhp);
		 	int *hpmax = (Upolyd) ? (&u.mhmax) : (&u.uhpmax);

			if (*hp < (*hpmax / 4))
				multiplier = 3;
			else if (*hp < (*hpmax / 2))
				multiplier = 2;
		}
		/* some artifacts are 3x damage, or add 2dX damage */
		if (double_bonus_damage_artifact(otmp->oartifact)
			|| (otmp->oartifact == ART_FROST_BRAND && species_resists_fire(mon) && spec_dbon_applies)
			|| (otmp->oartifact == ART_FIRE_BRAND && species_resists_cold(mon) && spec_dbon_applies)
			|| (attacks(AD_HOLY, otmp) && hates_holy_mon(mon) && spec_dbon_applies)
		)
			multiplier *= 2;
		if(otmp->oartifact == ART_SILVER_STARLIGHT)
			multiplier *= 4;
		/* lightsabers add 3dX damage (but do not multiply multiplicative bonus damage) */
		if (damd && (is_lightsaber(otmp) && litsaber(otmp)))
			multiplier *= 3;
		/* The profaned flame should do half damage to fire resistant things */
		if(otmp->oartifact == ART_PROFANED_GREATSCYTHE && Fire_res(mon)){
			damd = (damd + 1) / 2;
			dmgtomulti = (dmgtomulti + 1) / 2;
		}
		/* some artifacts made it into here despite their spec_dbon not applying -- they have reduced bonus damage */
		if (!spec_dbon_applies) {
			damd = (damd + 1) / 2;
			dmgtomulti = (dmgtomulti + 1) / 2;
		}

		/* add the damage to the appropriate pointer, if allowable */
		if (goodpointers) {
			if (spec_applies(otmp, mon, TRUE))
				*truedmgptr += (damd ? d(multiplier, damd) : max(dmgtomulti, 1)*multiplier);
			else
				*plusdmgptr += (damd ? d(multiplier, damd) : max(dmgtomulti, 1)*multiplier);

			//Also do a fireball or a hail effect
			if(otmp->oartifact == ART_FIRE_BRAND && spec_dbon_applies){
				*truedmgptr += d(6*multiplier, 6);
			}
			else if(otmp->oartifact == ART_FROST_BRAND){
				if(spec_dbon_applies)
					*truedmgptr += d(3*multiplier, 8);
				*plusdmgptr += d(3*multiplier, 8);
			}
		}
		return TRUE;
	}
	return FALSE;
}

/* add identified artifact to discoveries list */
void
discover_artifact(m)
int m;
{
    int i;

    /* look for this artifact in the discoveries list;
       if we hit an empty slot then it's not present, so add it */
    for (i = 0; i < nrofartifacts; i++)
	if (artidisco[i] == 0 || artidisco[i] == m) {
	    artidisco[i] = m;
	    return;
	}
    /* there is one slot per artifact, so we should never reach the
       end without either finding the artifact or an empty slot... */
    impossible("couldn't discover artifact (%d)", (int)m);
}
/* remove identifed artifact from discoveries list */
void
undiscover_artifact(m)
int m;
{
	int i;
	boolean found = FALSE;
	/* look for this artifact in the discoveries list;
       if we hit an empty slot then it's not present, so add it */
	for (i = 0; i < nrofartifacts - 1 && artidisco[i]; i++) {
		if (artidisco[i] == m) {
			found = TRUE;
		}
		if (found)
			artidisco[i] = artidisco[i + 1];
	}
	artidisco[i] = 0;
}

/* used to decide whether an artifact has been fully identified */
boolean
undiscovered_artifact(m)
int m;
{
    int i;

    /* look for this artifact in the discoveries list;
       if we hit an empty slot then it's undiscovered */
    for (i = 0; i < nrofartifacts; i++)
	if (artidisco[i] == m)
	    return FALSE;
	else if (artidisco[i] == 0)
	    break;
    return TRUE;
}

/* display a list of discovered artifacts; return their count */
int
disp_artifact_discoveries(tmpwin)
winid tmpwin;		/* supplied by dodiscover() */
{
    int i, m, otyp;
    char buf[BUFSZ];

    for (i = 0; i < nrofartifacts; i++) {
	if (artidisco[i] == 0) break;	/* empty slot implies end of list */
	if (i == 0) putstr(tmpwin, iflags.menu_headings, "Artifacts");
	m = artidisco[i];
	// pline("%d: %s",m,artilist[m].name);
	otyp = artilist[m].otyp;
	Sprintf(buf, "  %s [%s %s]", artiname(m),
		align_str(artilist[m].alignment), simple_typename(otyp));
	putstr(tmpwin, 0, buf);
    }
    return i;
}

#endif /* OVL1 */

#ifdef OVLB

boolean
near_yourteam(mon)
struct monst *mon;
{
	struct monst *mnear;
	for(int x = mon->mx-1; x < mon->mx+2; x++){
		for(int y = mon->my-1; y < mon->my+2; y++){
			if(!isok(x,y))
				continue;
			mnear = m_u_at(x, y);
			if(!mnear)
				continue;
			if(mnear == &youmonst)
				return TRUE;
			if(mnear->mtame)
				return TRUE;
		}
	}
	return FALSE;
}

	/*
	 * Magicbane's intrinsic magic is incompatible with normal
	 * enchantment magic.  Thus, its effects have a negative
	 * dependence on spe.  Against low mr victims, it typically
	 * does "double athame" damage, 2d4.  Occasionally, it will
	 * cast unbalancing magic which effectively averages out to
	 * 4d4 damage (3d4 against high mr victims), for spe = 0.
	 *
	 * Prior to 3.4.1, the cancel (aka purge) effect always
	 * included the scare effect too; now it's one or the other.
	 * Likewise, the stun effect won't be combined with either
	 * of those two; it will be chosen separately or possibly
	 * used as a fallback when scare or cancel fails.
	 *
	 * [Historical note: a change to artifact_hit() for 3.4.0
	 * unintentionally made all of Magicbane's special effects
	 * be blocked if the defender successfully saved against a
	 * stun attack.  As of 3.4.1, those effects can occur but
	 * will be slightly less likely than they were in 3.3.x.]
	 */
#define MB_MAX_DIEROLL		8	/* rolls above this aren't magical */
static const char * const mb_verb[2][4] = {
	{ "probe", "stun", "scare", "cancel" },
	{ "prod", "amaze", "tickle", "purge" },
};
#define MB_INDEX_PROBE		0
#define MB_INDEX_STUN		1
#define MB_INDEX_SCARE		2
#define MB_INDEX_CANCEL		3

/* called when someone is being hit by the pen of the void */
STATIC_OVL boolean
voidPen_hit(magr, mdef, pen, dmgptr, dieroll, vis, hittee)
struct monst *magr, *mdef;	/* attacker and defender */
struct obj *pen;			/* Pen of the Void */
int *dmgptr;			/* extra damage target will suffer */
int dieroll;			/* d20 that has already scored a hit */
boolean vis;			/* whether the action can be seen */
char *hittee;			/* target's name: "you" or mon_nam(mdef) */
{
    char buf[BUFSZ];
	boolean youdefend = mdef == &youmonst;
	boolean youattack = magr == &youmonst;
	boolean and = FALSE;
	int dnum = (Role_if(PM_EXILE) && quest_status.killed_nemesis) ? 4 : 1; /* Die number is doubled after the quest */
	int berithdamage = 0;
	
	buf[0] = '\0';
		
	if(u.voidChime){
		pline("The ringing blade hits %s.", hittee);
		vis = FALSE;
	}
	
	if (u.specialSealsActive&SEAL_COSMOS || u.specialSealsActive&SEAL_LIVING_CRYSTAL || u.specialSealsActive&SEAL_TWO_TREES){
		*dmgptr += (mdef->data->maligntyp == A_CHAOTIC)? d(2*dnum,4) : ((mdef->data->maligntyp == A_NEUTRAL) ? d(dnum, 4) : 0);
	} else if (u.specialSealsActive&SEAL_MISKA){
		*dmgptr += (mdef->data->maligntyp == A_LAWFUL)? d(2*dnum,4) : ((mdef->data->maligntyp == A_NEUTRAL) ? d(dnum, 4) : 0);
	} else if (u.specialSealsActive&SEAL_NUDZIRATH){
		*dmgptr += (mdef->data->maligntyp != A_NEUTRAL)? d(dnum,6) : 0;
	} else if (u.specialSealsActive&SEAL_ALIGNMENT_THING){
		if(rn2(3)) dmgptr += d(rnd(2)*dnum,4);
	} else if (u.specialSealsActive&SEAL_UNKNOWN_GOD){
		*dmgptr -= pen->spe;
	}
	
	if (pen->ovara_seals&SEAL_AHAZU && dieroll < 5){
	    *dmgptr += d(dnum,4);
		if(vis) {
			pline("The blade's shadow catches on %s.", hittee);
		}
		mdef->movement -= 3;
	}
	if (pen->ovara_seals&SEAL_AMON) {
	    if (vis){
			Sprintf(buf, "fiery"); // profane
			and = TRUE;
		}
		if (!UseInvFire_res(mdef)) {
			if (!rn2(4)) (void) destroy_item(mdef, POTION_CLASS, AD_FIRE);
			if (!rn2(4)) (void) destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
			if (!rn2(7)) (void) destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
		}
	    if (youdefend) burn_away_slime();
	    if (youdefend && FrozenAir) melt_frozen_air();
	    // if(youdef ? (hates_unholy(youracedata)) : (hates_unholy_mon(mdef))){
		if(youdefend ? !Fire_resistance : !resists_fire(mdef)){
			*dmgptr += d(dnum,4);
		}
	} // triggers narrow_voidPen_hit - fire res
	if (pen->ovara_seals&SEAL_ASTAROTH) {
	    if (vis){ 
			and ? Strcat(buf, " and crackling") : Sprintf(buf, "crackling");
			and = TRUE;
		}
		if (!UseInvShock_res(mdef)) {
			if (!rn2(5)) (void) destroy_item(mdef, RING_CLASS, AD_ELEC);
			if (!rn2(5)) (void) destroy_item(mdef, WAND_CLASS, AD_ELEC);
		}
		if(youdefend ? !Shock_resistance : !resists_elec(mdef)){
			if(shock_vulnerable_species(mdef))
				dnum *= 2;
			*dmgptr += d(dnum,4);
		}
	} // nvPh - shock res
	if (pen->ovara_seals&SEAL_BALAM) {
	    if (vis){ 
			and ? Strcat(buf, " yet freezing") : Sprintf(buf, "freezing");
			and = TRUE;
		}
		if (!UseInvCold_res(mdef)) {
	    	if (!rn2(4)) (void) destroy_item(mdef, POTION_CLASS, AD_COLD);
		}
		if(youdefend ? !Cold_resistance : !resists_cold(mdef)){
			*dmgptr += d(dnum,4);
		}
	} // nvPh - cold res
	if (pen->ovara_seals&SEAL_BERITH){
		berithdamage = d(dnum,4);
#ifndef GOLDOBJ
		if (!youattack || u.ugold >= berithdamage)
#else
		if (!youattack || money_cnt(invent) >= berithdamage)
#endif
		{
			if(youattack)
#ifndef GOLDOBJ
				u.ugold -= berithdamage;
#else
				money2none(berithdamage);
#endif
			*dmgptr += berithdamage;
		} else {
			berithdamage = 0;
		}
		
		if(vis && berithdamage > 0){
			if(pen->ovara_seals&SEAL_AMON) and ? Strcat(buf, " and blood-crusted") : Sprintf(buf, "blood-crusted");
			else and ? Strcat(buf, " and blood-soaked") : Sprintf(buf, "blood-soaked");
			and = TRUE;
		}
	}
	if (pen->ovara_seals&SEAL_BUER){
		if(youattack) healup(d(dnum,4), 0, FALSE, FALSE);
		else magr->mhp = min(magr->mhp + d(dnum,4),magr->mhpmax);
	}
	if (pen->ovara_seals&SEAL_CHUPOCLOPS && dieroll < 3){
		struct trap *ttmp2 = maketrap(mdef->mx, mdef->my, WEB);
	    *dmgptr += d(dnum,4);
		if (ttmp2){
			if(youdefend){
				pline_The("webbing sticks to you. You're caught!");
				dotrap(ttmp2, NOWEBMSG);
#ifdef STEED
				if (u.usteed && u.utrap) {
				/* you, not steed, are trapped */
				dismount_steed(DISMOUNT_FELL);
				}
#endif
			}
			else mintrap(mdef);
		}
	}
	if (pen->ovara_seals&SEAL_DANTALION){
		if(youdefend || !mindless_mon(mdef))
			*dmgptr += d(dnum,4);
		if(dieroll < 3){
			if(youdefend) aggravate();
			else probe_monster(mdef);
		}
	} // nvPh - !mindless
	if (pen->ovara_seals&SEAL_ECHIDNA) {
	    if (vis){ 
			and ? Strcat(buf, " and sizzling") : Sprintf(buf, "sizzling");
			and = TRUE;
		}
	    if (!rn2(2)) (void) destroy_item(mdef, POTION_CLASS, AD_FIRE);
		if(youdefend ? !Acid_resistance : !resists_acid(mdef)){
			*dmgptr += d(dnum,4);
		}
	} // nvPh - acid res
	if (pen->ovara_seals&SEAL_ENKI) {
	    if (vis){ 
			if(pen->ovara_seals&SEAL_AMON) and ? Strcat(buf, " and steaming") : Sprintf(buf, "steaming");
			else and ? Strcat(buf, " and dripping") : Sprintf(buf, "dripping");
			and = TRUE;
		}
		water_damage(youdefend ? invent : mdef->minvent, FALSE, FALSE, FALSE, mdef);
		if (youdefend){
			if (!Waterproof) {
				if (uarmc && uarmc->greased){
					if (!rn2(uarmc->blessed ? 4 : 2)){
						uarmc->greased = 0;
						pline("The layer of grease on your %s dissolves.", xname(uarmc));
					}
				} else {
					int mult = (flaming(youracedata) || is_iron(youracedata)) ? 2 : 1;
					
					*dmgptr += d(dnum,4)*mult;
					if(youracedata->mtyp == PM_GREMLIN && rn2(3))
						(void)split_mon(&youmonst, (struct monst *)0);
				}
			}
		} else { //Monster
			struct obj *cloak = which_armor(mdef, W_ARMC);
			if (!mon_resistance(mdef, WATERPROOF)) {
				if (cloak && cloak->greased){
					if (!rn2(cloak->blessed ? 4 : 2)){
						cloak->greased = 0;
						if(canseemon(mdef)) pline("The layer of grease on %s's %s dissolves.", mon_nam(mdef), xname(cloak));
					}
				} else {
					int mult = (flaming(mdef->data) || is_iron(mdef->data))  ? 2 : 1;
				
					*dmgptr += d(dnum,4)*mult;
					if(mdef->mtyp == PM_GREMLIN && rn2(3)){
						(void)split_mon(mdef, (struct monst *)0);
					}
				}
			}
		}
	} // nvPh - water res
	if (pen->ovara_seals&SEAL_FAFNIR){
		if (vis){
			and ? Strcat(buf, " and ruinous") : Sprintf(buf, "ruinous");
			and = TRUE;
		}
		if(youdefend ? is_golem(youracedata) : is_golem(mdef->data)){
			*dmgptr += d(2*dnum,4);
		} else if(youdefend ? nonliving(youracedata) : nonliving(mdef->data)){
			*dmgptr += d(dnum,4);
		}
	} // nvPh - golem/nonliving
	if (pen->ovara_seals&SEAL_HUGINN_MUNINN){
		if(youdefend){
			if(!Blind){
				make_blinded(Blinded+1L,FALSE);
				*dmgptr += d(dnum,4);
			}
		}
		else if(!youdefend){
			if(mdef->mcansee && haseyes(mdef->data)){
				*dmgptr += d(dnum,4);
				mdef->mcansee = 0;
				mdef->mblinded = 1;
			}
		}
	} // nvPh - blind
	if (pen->ovara_seals&SEAL_IRIS) {
	    if (vis){ 
			if(pen->ovara_seals&SEAL_ENKI) and ? Strcat(buf, " yet thirsty") : Sprintf(buf, "thirsty");
			else and ? Strcat(buf, " and thirsty") : Sprintf(buf, "thirsty");
			and = TRUE;
		}
		if(youdefend ? !(nonliving(youracedata) || is_anhydrous(youracedata)) : !(nonliving(mdef->data) || is_anhydrous(mdef->data))){
			*dmgptr += d(dnum,4);
		}
	} // nvPh - hydrous
	if (pen->ovara_seals&SEAL_MOTHER && dieroll <= dnum){
		if(youdefend) nomul(5,"held by the Pen of the Void");
	    else if(mdef->mcanmove || mdef->mfrozen){
			mdef->mcanmove = 0;
			mdef->mfrozen = max(mdef->mfrozen, 5);
		}
	}
	if (pen->ovara_seals&SEAL_NABERIUS){
		if(youdefend && (Upolyd ? u.mh < .25*u.mhmax : u.uhp < .25*u.uhpmax)) *dmgptr += d(dnum,4);
		else if(!youdefend){
			if(mdef->mflee) *dmgptr += d(dnum,4);
			if(mdef->mpeaceful) *dmgptr += d(dnum,4);
		}
	} // nvPh - peaceful/fleeing
	if (pen->ovara_seals&SEAL_ORTHOS && dieroll < 3){
	    *dmgptr += d(2*dnum,8);
		if(youdefend){
			You("are addled by the gusting winds!");
			make_stunned((HStun + 3), FALSE);
		} else {
			if(vis) pline("%s is thrown backwards by the gusting winds!",Monnam(mdef));
			if(mdef->data->msize >= MZ_HUGE) mhurtle(mdef, u.dx, u.dy, 1, TRUE);
			else mhurtle(mdef, u.dx, u.dy, 10, FALSE);
			if(DEADMONSTER(mdef) || MIGRATINGMONSTER(mdef)) return vis;//Monster was killed as part of movement OR fell to new level and we should stop.
		}
		and = TRUE;
	}
	if (pen->ovara_seals&SEAL_OSE){
		if(youdefend && !Tele_blind && (Blind_telepat || !rn2(5))) *dmgptr += d(dnum,15);
		else if(!youdefend && !mindless_mon(mdef) && (mon_resistance(mdef,TELEPAT) || !rn2(5))) *dmgptr += d(dnum,15);
	} // nvPh - telepathy
	if (pen->ovara_seals&SEAL_OTIAX){
		char bufO[BUFSZ];
		bufO[0] = '\0';
	    *dmgptr += d(1,dnum);
		if(youattack){
			if(dieroll == 1){
				struct obj *otmp2;
				long unwornmask = 0L;

				/* Don't steal worn items, and downweight wielded items */
				if((otmp2 = mdef->minvent) != 0) {
					while(otmp2 && 
						  otmp2->owornmask&W_ARMOR && 
						  !( (otmp2->owornmask&W_WEP) && !rn2(10))
					) otmp2 = otmp2->nobj;
				}
				/* Still has handling for worn items, incase that changes */
				if(otmp2 != 0){
					/* take the object away from the monster */
					if(otmp2->quan > 1L){
						otmp2 = splitobj(otmp2, 1L);
						obj_extract_self(otmp2); //wornmask is cleared by splitobj
					} else {
						obj_extract_self(otmp2);
						if ((unwornmask = otmp2->owornmask) != 0L) {
							mdef->misc_worn_check &= ~unwornmask;
							if (otmp2->owornmask & W_WEP) {
								setmnotwielded(mdef,otmp2);
								MON_NOWEP(mdef);
							}
							if (otmp2->owornmask & W_SWAPWEP) {
								setmnotwielded(mdef,otmp2);
								MON_NOSWEP(mdef);
							}
							otmp2->owornmask = 0L;
							update_mon_intrinsics(mdef, otmp2, FALSE, FALSE);
						}
					}
					Your("blade's mist tendril frees %s.",doname(otmp2));
					mdrop_obj(mdef,otmp2,FALSE);
					/* more take-away handling, after theft message */
					if (unwornmask & W_WEP) {		/* stole wielded weapon */
						possibly_unwield(mdef, FALSE);
					} else if (unwornmask & W_ARMG) {	/* stole worn gloves */
						mselftouch(mdef, (const char *)0, TRUE);
						if (mdef->mhp <= 0)	/* it's now a statue */
							return 1; /* monster is dead */
					}
				}
			}
		} else if(youdefend){
			if(!(u.sealsActive&SEAL_ANDROMALIUS) && !check_mutation(TENDRIL_HAIR)){
				bufO[0] = '\0';
				switch(steal(magr, bufO, FALSE, FALSE)){
				  case -1:
					return vis;
				  case 0:
				  break;
				  default:
					pline("%s steals %s.", Monnam(magr), bufO);
				  break;
				}
			}
		} else {
			struct obj *otmp;
			for (otmp = mdef->minvent; otmp; otmp = otmp->nobj) if (!magr->mtame || !otmp->cursed) break;

			if (otmp) {
				char onambuf[BUFSZ], mdefnambuf[BUFSZ];

				/* make a special x_monnam() call that never omits
				   the saddle, and save it for later messages */
				Strcpy(mdefnambuf, x_monnam(mdef, ARTICLE_THE, (char *)0, 0, FALSE));

	#ifdef STEED
				if (u.usteed == mdef &&
						otmp == which_armor(mdef, W_SADDLE))
					/* "You can no longer ride <steed>." */
					dismount_steed(DISMOUNT_POLY);
	#endif
				obj_extract_self(otmp);
				if (otmp->owornmask) {
					mdef->misc_worn_check &= ~otmp->owornmask;
					if (otmp->owornmask & W_WEP)
						setmnotwielded(mdef,otmp);
					otmp->owornmask = 0L;
					update_mon_intrinsics(mdef, otmp, FALSE, FALSE);
				}
				/* add_to_minv() might free otmp [if it merges] */
				if (vis)
					Strcpy(onambuf, doname(otmp));
				(void) add_to_minv(magr, otmp);
				if (vis) {
					Strcpy(bufO, Monnam(magr));
					pline("%s steals %s from %s!", bufO,
						onambuf, mdefnambuf);
				}
				possibly_unwield(mdef, FALSE);
				mdef->mstrategy &= ~STRAT_WAITFORU;
				mselftouch(mdef, (const char *)0, FALSE);
				
				if (mdef->mhp <= 0) return vis;
			}
		}
	}
	if (pen->ovara_seals&SEAL_PAIMON){
		if(youattack) u.uen = min(u.uen+dnum,u.uenmax);
		else magr->mspec_used = max(magr->mspec_used - dnum,0);
	} // nvPh - !cancelled
	if (pen->ovara_seals&SEAL_SHIRO){
		struct obj *otmp;
		otmp = mksobj((mvitals[PM_ACERERAK].died > 0) ? BOULDER : ROCK, MKOBJ_NOINIT);
		projectile(magr, otmp, (void *)0, HMON_PROJECTILE|HMON_FIRED, mdef->mx, mdef->my, 0, 0, 0, 0, TRUE, FALSE, FALSE);
		if(mdef->mhp <= 0) return vis;//Monster was killed by throw and we should stop.
	} // nvPh potential - invisible?
	if (pen->ovara_seals&SEAL_SIMURGH){
		if(youdefend && !Blind){
			You("are dazzled by prismatic feathers!");
			make_stunned((HStun + 5), FALSE);
			if(mvitals[PM_ACERERAK].died > 0) *dmgptr += d(2,4);
		}
		else if(mdef->mcansee && haseyes(mdef->data)){
			pline("%s is dazzled by prismatic feathers!", Monnam(mdef));
			mdef->mstun = 1;
			mdef->mconf = 1;
			if(mvitals[PM_ACERERAK].died > 0) *dmgptr += d(2,4);
		}
		and = TRUE;
	} // nvPh - blind
	if (pen->ovara_seals&SEAL_TENEBROUS && dieroll <= dnum){
		if(youdefend && !Drain_resistance){
			if (Blind)
				You_feel("an unholy blade drain your life!");
			else pline_The("unholy blade drains your life!");
			losexp("life drainage",TRUE,FALSE,FALSE);
		}
		else if(!youdefend && !resists_drli(mdef)){
			if (vis) {
				pline_The("unholy blade draws the life from %s!",
				      mon_nam(mdef));
			}
			if (mdef->m_lev <= 0) {
				mdef->m_lev = 0;
				if(youattack) killed(mdef);
				else monkilled(mdef, (const char *)0, AD_DRLI);
				
				if(mdef->mhp <= 0) return vis; //otherwise lifesaved
			} else {
			    int drain = rnd(hd_size(mdef->data));
			    *dmgptr += drain;
			    mdef->mhpmax -= drain;
			    mdef->m_lev--;
			    drain /= 2;
			}
		}
		and = TRUE;
	} // nvPh - drain res
	
	if (buf[0] != '\0'){
		pline("The %s blade hits %s.", buf, hittee);
		return (and || vis);
	}
	
	return FALSE;
}

/* called when someone is being hit by the pen of the void */
STATIC_OVL boolean
narrow_voidPen_hit(mdef, pen)
struct monst *mdef;	/* defender */
struct obj *pen;	/* Pen of the Void */
{
	boolean youdefend = mdef == &youmonst;
	
	/*
	if (u.specialSealsActive&SEAL_COSMOS || u.specialSealsActive&SEAL_LIVING_CRYSTAL || u.specialSealsActive&SEAL_TWO_TREES) {
		if(!youdefend && (mdef->data->maligntyp == A_CHAOTIC)) return TRUE;
	}
	else if (u.specialSealsActive&SEAL_MISKA) {
		if(!youdefend && (mdef->data->maligntyp == A_LAWFUL)) return TRUE;
	}
	else if (u.specialSealsActive&SEAL_NUDZIRATH) {
		if(!youdefend && (mdef->data->maligntyp != A_NEUTRAL) && !rn2(2)) return TRUE;
	}
	else if (u.specialSealsActive&SEAL_ALIGNMENT_THING) {
		if(!youdefend && !rn2(3)) return TRUE;
	}
	*/
	// I went ahead and implemented these, not sure if they're balanced so commented out - riker	
	
	if (pen->ovara_seals&SEAL_AMON) {
		if(youdefend ? !Fire_resistance : !resists_fire(mdef)){
			return TRUE;
		}
	}
	if (pen->ovara_seals&SEAL_ASTAROTH) {
		if(youdefend ? !Shock_resistance : !resists_elec(mdef)){
			return TRUE;
		}
	}
	if (pen->ovara_seals&SEAL_BALAM) {
		if(youdefend ? !Cold_resistance : !resists_cold(mdef)){
			return TRUE;
		}
	}
	if (pen->ovara_seals&SEAL_DANTALION){
	    if(youdefend || !mindless_mon(mdef))
			return TRUE;
	}
	if (pen->ovara_seals&SEAL_ENKI) {
		if(youdefend){
			if(!Waterproof && !(uarmc && uarmc->greased))
				return TRUE;
		} else{ //Monster
			struct obj *cloak = which_armor(mdef, W_ARMC);
			if(!mon_resistance(mdef, WATERPROOF) && !(cloak && cloak->greased)){
				return TRUE;
			}
		}
	}
	if (pen->ovara_seals&SEAL_ECHIDNA) {
		if(youdefend ? !Acid_resistance : !resists_acid(mdef)){
			return TRUE;
		}
	}
	if (pen->ovara_seals&SEAL_FAFNIR){
		if(youdefend ? is_golem(youracedata) : is_golem(mdef->data)){
			return TRUE;
		} else if(youdefend ? nonliving(youracedata) : nonliving(mdef->data)){
			return TRUE;
		}
	}
	if (pen->ovara_seals&SEAL_HUGINN_MUNINN){
		if(youdefend && !Blind){
			return TRUE;
		}
		else if(mdef->mcansee && haseyes(mdef->data)){
			return TRUE;
		}
	}
	if (pen->ovara_seals&SEAL_IRIS) {
		if(youdefend ? !(nonliving(youracedata) || is_anhydrous(youracedata)) : !(nonliving(mdef->data) || is_anhydrous(mdef->data))){
			return TRUE;
		}
	}
	if (pen->ovara_seals&SEAL_NABERIUS){
		if(youdefend && (Upolyd ? u.mh < .25*u.mhmax : u.uhp < .25*u.uhpmax)){
			return TRUE;
		}
		else if(mdef->mflee || mdef->mpeaceful){
			return TRUE;
		}
	}
	if (pen->ovara_seals&SEAL_OSE){
		if(youdefend && !Tele_blind && Blind_telepat){
			return TRUE;
		}
		else if(!mindless_mon(mdef) && mon_resistance(mdef,TELEPAT)){
			return TRUE;
		}
	}
	if (pen->ovara_seals&SEAL_PAIMON){
		if(youdefend && (u.uen < u.uenmax/4)){
			return TRUE;
		}
		else if(mdef->mcan){
			return TRUE;
		}
	}
	if (pen->ovara_seals&SEAL_SIMURGH){
		if(youdefend && !Blind){
			return TRUE;
		}
		else if(mdef->mcansee && haseyes(mdef->data)){
			return TRUE;
		}
	}
	if (pen->ovara_seals&SEAL_TENEBROUS){
		if(youdefend && !Drain_resistance){
			return TRUE;
		}
		else if(!youdefend && !resists_drli(mdef)){
			return TRUE;
		}
	}

	return FALSE;
}

STATIC_OVL boolean
Mb_hit(magr, mdef, mb, dmgptr, dieroll, vis, hittee, type)
struct monst *magr, *mdef;	/* attacker and defender */
struct obj *mb;			/* Magicbane */
int *dmgptr;			/* extra damage target will suffer */
int dieroll;			/* d20 that has already scored a hit */
boolean vis;			/* whether the action can be seen */
char *hittee;			/* target's name: "you" or mon_nam(mdef) */
char *type;			/* blade, staff, etc */
{
    struct permonst *old_uasmon;
    const char *verb;
    boolean youattack = (magr == &youmonst),
	    youdefend = (mdef == &youmonst),
	    resisted = FALSE, do_stun, do_confuse, result;
    int attack_indx, scare_dieroll = MB_MAX_DIEROLL / 2,
		dsize=4, dnum=1;
	
    result = FALSE;		/* no message given yet */
	
    /* the most severe effects are less likely at higher enchantment */
    if (mb->spe >= 3)
	scare_dieroll /= (1 << (mb->spe / 3));
    /* if target successfully resisted the artifact damage bonus,
       INCREASE overall likelihood of the assorted special effects 
	   this is Magic BANE, after all, it is stronger the less 
	   magical it is */
    if (mb->oartifact == ART_MAGICBANE && !spec_dbon_applies) dieroll -= 2;

    /* might stun even when attempting a more severe effect, but
       in that case it will only happen if the other effect fails;
       extra damage will apply regardless; 3.4.1: sometimes might
       just probe even when it hasn't been enchanted */
    do_stun = (max(mb->spe,0) < rn2(spec_dbon_applies ? 11 : 7));

	if(mb->oartifact == ART_MAGICBANE){
		if (!spec_dbon_applies){
			//Equal to a fireball, makes +0 MB good vs magic resistant creatures (avg damage is almost equal for +0,+1,+2), +7 MB least bad vs magic sensitives
			dnum = 6;
			dsize = 6;
		}
	} else if(mb->oartifact == ART_MIRROR_BRAND){
		//average longsword damage, mirror brand is best vs magic sensitives, has a flatter distribution vs magic resistants +2 is better than +7, but by a very small margin
		dnum = 2;
		dsize = 10;
	} else if(mb->oartifact == ART_STAFF_OF_WILD_MAGIC){
		//The wild magic staff, on the other hand, does nothing if the target resists its effects, and is not degraded by enchantment;
		//Because of the low die size, its straight weapon damage is never more than so-so, though.
		if (!spec_dbon_applies) return FALSE;
		dnum = 1;
		dsize = 6;
		scare_dieroll = MB_MAX_DIEROLL / 2;
		dieroll += 2;
		do_stun = rn2(2);
	} else if(mb->oartifact == ART_FORCE_PIKE_OF_THE_RED_GUAR){
		//Strongest vs magic sensitives and +7, also good vs magic resistants at +2
		dnum = 3;
		dsize = 10;
	} else if(mb->oartifact == ART_FRIEDE_S_SCYTHE){
		if(youdefend){
			if(!Cold_resistance){
				if(species_resists_fire(&youmonst)){
					*dmgptr += 1.5 * (*dmgptr);
				} else {
					*dmgptr += *dmgptr;
				}
				if(u.ustdy >= 10){
					u_slow_down();
				}
				if(u.ustdy >= 20){
					*dmgptr += d(12,6);
					u.ustdy -= 20;
				}
			}
		} else {
			if(!resists_cold(mdef)){
				if(species_resists_fire(mdef)){
					*dmgptr += 1.5 * (*dmgptr);
				} else {
					*dmgptr += *dmgptr;
				}
				if(mdef->mstdy >= 10){
					if(mdef->mspeed != MSLOW)
						pline("%s slows down!", Monnam(mdef));
					mdef->mspeed = MSLOW;
					mdef->permspeed = MSLOW;
				}
				if(mdef->mstdy >= 20){
					*dmgptr += d(12,6);
					mdef->mstdy -= 20;
				}
			}
		}
		return 0;
	}
	
	
    /* the special effects also boost physical damage; increments are
       generally cumulative, but since the stun effect is based on a
       different criterium its damage might not be included; the base
       damage is either 1d4 (athame) or 2d4 (athame+spec_dbon) depending
       on target's resistance check against AD_STUN (handled by caller)
       [note that a successful save against AD_STUN doesn't actually
       prevent the target from ending up stunned] */
    attack_indx = MB_INDEX_PROBE;
    *dmgptr += d(dnum,dsize);			/* (2..3)d4 */
    if (do_stun) {
	attack_indx = MB_INDEX_STUN;
	*dmgptr += d(dnum,dsize);		/* (3..4)d4 */
    }
    if (dieroll <= scare_dieroll) {
	attack_indx = MB_INDEX_SCARE;
	*dmgptr += d(dnum,dsize);		/* (3..5)d4 */
    }
    if (dieroll <= (scare_dieroll / 2)) {
	attack_indx = MB_INDEX_CANCEL;
	*dmgptr += d(dnum,dsize);		/* (4..6)d4 */
    }

    /* give the hit message prior to inflicting the effects */
    verb = mb_verb[!!Hallucination][attack_indx];
    if (youattack || youdefend || vis) {
	result = TRUE;
	pline_The("magic-absorbing %s %s %s!",
		  type, vtense((const char *)0, verb), hittee);
	/* assume probing has some sort of noticeable feedback
	   even if it is being done by one monster to another */
	if (attack_indx == MB_INDEX_PROBE && !canspotmon(mdef))
	    map_invisible(mdef->mx, mdef->my);
    }

    /* now perform special effects */
    switch (attack_indx) {
    case MB_INDEX_CANCEL:
	old_uasmon = youmonst.data;
	/* No mdef->mcan check: even a cancelled monster can be polymorphed
	 * into a golem, and the "cancel" effect acts as if some magical
	 * energy remains in spellcasting defenders to be absorbed later.
	 */
	if (!cancel_monst(mdef, mb, youattack, FALSE, FALSE,0)) {
	    resisted = TRUE;
	} else {
	    do_stun = FALSE;
	    if (youdefend) {
		if (youmonst.data != old_uasmon)
		    *dmgptr = 0;    /* rehumanized, so no more damage */
		if (u.uenmax > 0) {
		    You("lose magical energy!");
		    u.uenbonus--;
			calc_total_maxen();
		    if (u.uen > 0) u.uen--;
		    flags.botl = 1;
		}
	    } else {
		if (mdef->mtyp == PM_CLAY_GOLEM || mdef->mtyp == PM_SPELL_GOLEM)
		    mdef->mhp = 1;	/* cancelled clay golems will die */
		if (youattack && (attacktype(mdef->data, AT_MAGC) || attacktype(mdef->data, AT_MMGC))) {
		    You("absorb magical energy!");
		    u.uenbonus++;
		    u.uen++;
			calc_total_maxen();
		    flags.botl = 1;
		}
	    }
	}
	break;

    case MB_INDEX_SCARE:
	if (youdefend) {
	    if (Antimagic) {
		resisted = TRUE;
	    } else {
		nomul(-3, "being scared stiff");
		nomovemsg = "";
		if (magr && magr == u.ustuck && sticks(&youmonst)) {
		    u.ustuck = (struct monst *)0;
		    You("release %s!", mon_nam(magr));
		}
	    }
	} else {
	    if (rn2(2) && resist(mdef, WEAPON_CLASS, 0, NOTELL))
		resisted = TRUE;
	    else
		monflee(mdef, 3, FALSE, (mdef->mhp > *dmgptr));
	}
	if (!resisted) do_stun = FALSE;
	break;

    case MB_INDEX_STUN:
	do_stun = TRUE;		/* (this is redundant...) */
	break;

    case MB_INDEX_PROBE:
	if (youattack && (mb->spe == 0 || !rn2(3 * abs(mb->spe)))) {
	    pline_The("%s is insightful.", verb);
	    /* pre-damage status */
	    probe_monster(mdef);
	}
	break;
    }
    /* stun if that was selected and a worse effect didn't occur */
    if (do_stun) {
	if (youdefend)
	    make_stunned((HStun + 3), FALSE);
	else
	    mdef->mstun = 1;
	/* avoid extra stun message below if we used mb_verb["stun"] above */
	if (attack_indx == MB_INDEX_STUN) do_stun = FALSE;
    }
    /* lastly, all this magic can be confusing... */
    do_confuse = !rn2(12);
    if (do_confuse) {
	if (youdefend)
	    make_confused(HConfusion + 4, FALSE);
	else
	    mdef->mconf = 1;
    }

    if (youattack || youdefend || vis) {
	(void) upstart(hittee);	/* capitalize */
	if (resisted) {
	    pline("%s %s!", hittee, vtense(hittee, "resist"));
	    shieldeff(youdefend ? u.ux : mdef->mx,
		      youdefend ? u.uy : mdef->my);
	}
	if ((do_stun || do_confuse) && flags.verbose) {
	    char buf[BUFSZ];

	    buf[0] = '\0';
	    if (do_stun) Strcat(buf, "stunned");
	    if (do_stun && do_confuse) Strcat(buf, " and ");
	    if (do_confuse) Strcat(buf, "confused");
	    pline("%s %s %s%c", hittee, vtense(hittee, "are"),
		  buf, (do_stun && do_confuse) ? '!' : '.');
	}
    }

    return result;
}


/* special damage bonus */
/* returns FALSE if no bonus damage was applicable */
/* Just do bonus damage, don't make any modifications to the defender */
boolean
oproperty_dbon(otmp, magr, mdef, basedmg, plusdmgptr, truedmgptr)
struct obj * otmp;
struct monst * magr;
struct monst * mdef;
int basedmg;
int * plusdmgptr;
int * truedmgptr;
{
	boolean youdef = (mdef == &youmonst);
	boolean youagr = (magr == &youmonst);
	struct permonst * pd = (youdef ? youracedata : mdef->data);
	int original_plusdmgptr = *plusdmgptr;
	int original_truedmgptr = *truedmgptr;
	const struct artifact *oart = get_artifact(otmp);
	
	if(!Fire_res(mdef)){
		if(check_oprop(otmp, OPROP_FIREW))
			*truedmgptr += basedmg;
		if(check_oprop(otmp, OPROP_OONA_FIREW))
			*truedmgptr += d(1, 8);
		if(check_oprop(otmp, OPROP_LESSER_FIREW))
			*truedmgptr += d(2, 6);
	}
	if(!Cold_res(mdef)){
		if(check_oprop(otmp, OPROP_COLDW))
			*truedmgptr += basedmg;
		if(check_oprop(otmp, OPROP_OONA_COLDW))
			*truedmgptr += d(1, 8);
		if(check_oprop(otmp, OPROP_LESSER_COLDW))
			*truedmgptr += d(2, 6);
	}
	if(check_oprop(otmp, OPROP_WATRW) || check_oprop(otmp, OPROP_LESSER_WATRW)){
		struct obj *cloak = which_armor(mdef, W_ARMC);
		struct obj *armor = which_armor(mdef, W_ARM);
		struct obj *shield = which_armor(mdef, W_ARMS);

		if (youdef && uarmc && uarmc->greased) {
			if (!rn2(uarmc->blessed ? 4 : 2)){
				uarmc->greased = 0;
				pline("The layer of grease on your %s dissolves.", xname(uarmc));
			}
		} else if (!youdef && cloak && cloak->greased){
			if (!rn2(cloak->blessed ? 4 : 2)){
				cloak->greased = 0;
				if(canseemon(mdef)) pline("The layer of grease on %s's %s dissolves.", mon_nam(mdef), xname(cloak));
			}
		} else if (!(youdef && Waterproof) && !(!youdef && mon_resistance(mdef, WATERPROOF))){
			int mult = (flaming(pd) || is_iron(pd)) ? 2 : 1;

			if(check_oprop(otmp, OPROP_WATRW))
				*truedmgptr += basedmg*mult;
			if(check_oprop(otmp, OPROP_LESSER_WATRW))
				*truedmgptr += d(2, 6)*mult;
		}
	}

	if (!Shock_res(mdef)){
		int mult = 1;
		if(shock_vulnerable_species(mdef))
			mult *= 2;
		if(check_oprop(otmp, OPROP_ELECW))
			*truedmgptr += mult*basedmg;
		if(check_oprop(otmp, OPROP_OONA_ELECW))
			*truedmgptr += d(mult*1, 8);
		if(check_oprop(otmp, OPROP_LESSER_ELECW))
			*truedmgptr += d(mult*2, 6);
	}
	
	if (!Acid_res(mdef)){
		if(check_oprop(otmp, OPROP_ACIDW))
			*truedmgptr += basedmg;
		if(check_oprop(otmp, OPROP_LESSER_ACIDW))
			*truedmgptr += d(2, 6);
	}
	if (!Magic_res(mdef)){
		if(check_oprop(otmp, OPROP_MAGCW))
			*truedmgptr += basedmg;
		if(check_oprop(otmp, OPROP_LESSER_MAGCW))
			*truedmgptr += d(3, 4);
		
		if(check_oprop(otmp, OPROP_OCLTW)){
			*truedmgptr += d(1, 4);
			if(otmp->spe > 0)
				*truedmgptr += d(1, otmp->spe);
		}
	}
	if(check_oprop(otmp, OPROP_ELFLW)){
		int level = Insight >= 33 ? 2 : Insight >= 11 ? 1 : 0;
		int bonus = 0;
		if(youagr){
			if(u.ualign.record < -3){
				bonus += d(level ? 2 : 1, 8);
				if(level > 1)
					bonus += otmp->spe;
				if(youdef ? (hates_unholy(youracedata)) : (hates_unholy_mon(mdef))){
					bonus *= 2;
				}
			}
			else if(u.ualign.record > 3){
				bonus += d(level ? 2 : 1, 8);
				if(level > 1)
					bonus += otmp->spe;
				if(youdef ? (hates_holy(youracedata)) : (hates_holy_mon(mdef))){
					bonus *= 2;
				}
			}
			else {
				bonus += d(level ? 2 : 1, 4);
				if(youdef ? (hates_unblessed(youracedata)) : (hates_unblessed_mon(mdef))){
					bonus *= 4;
					if(level > 1)
						bonus += 2*otmp->spe;
				}
			}
		}
		else if(magr){
			if(hates_holy_mon(magr) || is_unholy_mon(magr)){
				bonus += d(level ? 2 : 1, 8);
				if(level > 1)
					bonus += otmp->spe;
				if(youdef ? (hates_unholy(youracedata)) : (hates_unholy_mon(mdef))){
					bonus *= 2;
				}
			}
			else if(hates_unholy_mon(magr) || is_holy_mon(magr)){
				bonus += d(level ? 2 : 1, 8);
				if(level > 1)
					bonus += otmp->spe;
				if(youdef ? (hates_holy(youracedata)) : (hates_holy_mon(mdef))){
					bonus *= 2;
				}
			}
			else {
				bonus += d(level ? 2 : 1, 4);
				if(youdef ? (hates_unblessed(youracedata)) : (hates_unblessed_mon(mdef))){
					bonus *= 4;
					if(level > 1)
						bonus += 2*otmp->spe;
				}
			}
		}
		else {
			bonus += d(level ? 2 : 1, 4);
			if(youdef ? (hates_unblessed(youracedata)) : (hates_unblessed_mon(mdef))){
				bonus *= 4;
				if(level > 1)
					bonus += 2*otmp->spe;
			}
		}
		*truedmgptr += bonus;
	}
	if(is_undead(pd) && check_oprop(otmp, OPROP_TDTHW) && !(youagr && FLAME_BAD)){
		*truedmgptr += basedmg + d(2,7);
	}
	if(check_oprop(otmp, OPROP_GOATW) && !(youagr && GOAT_BAD)){
		switch(goat_weapon_damage_turn(otmp)){
			case AD_EACD:
				if (!Acid_res(mdef)){
					*truedmgptr += basedmg;
				}
			break;
			case AD_DRST:
				*plusdmgptr += d(4,4);
				if (!Poison_res(mdef)){
					*truedmgptr += basedmg;
				}
			break;
			case AD_STDY:
				if (youdef){
					u.ustdy += basedmg;
				} else {
					mdef->mstdy += basedmg;
				}
			break;
			case AD_FIRE:
				if (!Fire_res(mdef)){
					*truedmgptr += d(3,10);
				}
			break;
			case AD_COLD:
				if (!Cold_res(mdef)){
					*truedmgptr += d(3,8);
				}
			break;
			case AD_ELEC:
				if (!Shock_res(mdef)){
					int mult = 1;
					if(shock_vulnerable_species(mdef))
						mult *= 2;
					*truedmgptr += d(mult*3,8);
				}
			break;
			case AD_ACID:
				if (!Acid_res(mdef)){
					*truedmgptr += d(4,4);
				}
			break;
		}
	}
	if(check_oprop(otmp, OPROP_SOTHW) && !(youagr && YOG_BAD)){
		switch(soth_weapon_damage_turn(otmp)){
			// case AD_STTP:
			// break;
			// case AD_VAMP:
			// break;
			case AD_FIRE:
				*truedmgptr += rnd(6);
				if(!Fire_res(mdef)){
					*truedmgptr += rnd(6);
				}
			break;
			// case AD_POLY:
				// //None
			// break;
			case AD_DESC:
				if (nonliving(pd) || is_anhydrous(pd)){
					//no effect
					break;
				}
				if (is_watery(pd)){
					*truedmgptr += basedmg;
					heal(magr, min(*hp(mdef), basedmg));
					if(youagr && !youdef)
						yog_credit(max(mdef->m_lev, mdef->data->cnutrit/50), FALSE);
				}
				else {
					*truedmgptr += basedmg/2;
					heal(magr, min(*hp(mdef), basedmg/2));
					if(youagr && !youdef)
						yog_credit(mdef->data->cnutrit/500, FALSE);
				}
			break;
			case AD_DRST:
				if(!Poison_res(mdef) && (youdef ? !Breathless : !breathless_mon(mdef))){
					*truedmgptr += d(1,6);
				}
			break;
			case AD_MAGM:
				if(!Magic_res(mdef)){
					*truedmgptr += max_ints(d(4,4) + otmp->spe, 1);
				}
			break;
			// case AD_MADF:
			// break;
		}
	}
	//Note: silver flame oprops stay active. I forsee no balance problems from this at all.
	
	//Psionic does slightly buffed damage, but triggers less frequently
	// Buffed vs. telepathic beings
	if(youdef && !Tele_blind && (Blind_telepat || !rn2(5))){
		if (check_oprop(otmp, OPROP_PSIOW))
			*truedmgptr += basedmg + otmp->spe;
		if (check_oprop(otmp, OPROP_LESSER_PSIOW))
			*truedmgptr += d(2, 12);
	}
	else if(!youdef && !mindless_mon(mdef) && (mon_resistance(mdef,TELEPAT) || !rn2(5))){
		if (check_oprop(otmp, OPROP_PSIOW))
			*truedmgptr += basedmg + otmp->spe;
		if (check_oprop(otmp, OPROP_LESSER_PSIOW))
			*truedmgptr += d(2, 12);
	}
	if(check_oprop(otmp, OPROP_RLYHW)){
		int bonus = 0;
		if(magr && (magr->mtyp == PM_LADY_CONSTANCE || is_mind_flayer(magr->data) || (youagr && u.sealsActive&SEAL_OSE))){
			if(mlev(magr)/10 > 1)
				bonus = d(mlev(magr)/10, 15);
			else
				bonus = rnd(15);
			bonus += otmp->spe;
		}
		if(Insight >= 36){//Works on all monsters, even mindless ones
			*truedmgptr += basedmg + otmp->spe + bonus;
		}
		else if(youdef && !Tele_blind && (Blind_telepat || Insight >= 6 || !rn2(4))){
			*truedmgptr += basedmg + otmp->spe + bonus;
		}
		else if(!youdef && !mindless_mon(mdef) && (mon_resistance(mdef,TELEPAT) || Insight >= 6 || !rn2(4))){
			*truedmgptr += basedmg + otmp->spe + bonus;
		}
	}
	if(check_oprop(otmp, OPROP_GSSDW)){
		int power = youagr ? min(Insight, u.usanity) : magr ? magr->m_lev : 0;
		//"Crit" chance
		if(power > 0){
			int multiplier = power >= 50 ? 3 : power >= 25 ? 2 : 1; 
			int chance = power >= 50 ? 4 : power >= 25 ? 3 : 2;
			if(youagr && u.usanity > 80 && artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_FOCUS)
				chance += (u.usanity-81)/5;//0, 1, 2, or 3 starting at 81, 86, 91, 96
			if(rn2(20) < chance){
				*truedmgptr += multiplier*basedmg;
				if(otmp->oartifact){
					const struct artifact *weap = get_artifact(otmp);
					if(youagr && (weap->inv_prop == GITH_ART || weap->inv_prop == AMALGUM_ART) && activeMentalEdge(GSTYLE_RESONANT)){
						for(struct monst *tmon = fmon; tmon; tmon = tmon->nmon){
							if(DEADMONSTER(tmon))
								continue;
							if(tmon->mtame){
								tmon->movement += 12;
								tmon->encouraged = max_ints(u.usanity < 50 ? 0 : u.usanity < 75 ? 2 : u.usanity < 90 ? 5 : 8, tmon->encouraged);
							}
							else if(near_yourteam(tmon)){
								if(!resist(tmon, '\0', 0, NOTELL))
									tmon->movement -= 12;
								tmon->encouraged = min_ints(u.usanity < 50 ? 0 : u.usanity < 75 ? -2 : u.usanity < 90 ? -5 : -8, tmon->encouraged);
							}
						}
					}
				}
			}
		}
		//Bonus psychic damage (More reliable than regular psychic damage)
		if(youdef || !mindless_mon(mdef)){
			if (power >= 30)
				*truedmgptr += d(2, 12);
			else if (power >= 20)
				*truedmgptr += d(2, 10);
			else if (power >= 10)
				*truedmgptr += d(2, 8);
			else
				*truedmgptr += d(2, 6);
		}
	}
	if(check_oprop(otmp, OPROP_DEEPW)){
		if(otmp->spe < 8){
		if(youdef && !Tele_blind && (Blind_telepat || !rn2(5)))
			*truedmgptr += d(1, 15 - (otmp->spe) * 2);
		else if(!youdef && !mindless_mon(mdef) && (mon_resistance(mdef,TELEPAT) || !rn2(5)))
			*truedmgptr += d(1, 15 - (otmp->spe) * 2);
		}
	}
	if(mdef && check_oprop(otmp, OPROP_WRTHW) && (otmp->wrathdata >> 2) >= 0 && (otmp->wrathdata >> 2) < NUMMONS){
		int bmod = 0;
		if(wrath_target(otmp, mdef)){
			bmod = (otmp->wrathdata&0x3L)+1;
			*plusdmgptr += bmod*basedmg / 4;
		}
		if(magr){
			bmod = min_ints(4, (*hpmax(magr)-*hp(magr)+mlev(magr))*4/(*hpmax(magr)));
			*plusdmgptr += bmod*basedmg / 4;
		}
	}
	if(mercy_blade_prop(otmp)){
		if(!u.veil && !Magic_res(mdef)){
			int mod = min(Insight, 50);
			if(youagr && Insight > 25)
				mod += min((Insight-25)/2, ACURR(A_CHA));
			*truedmgptr += basedmg*mod/50;
		}
	}
	if(youdef ? (u.ualign.type != A_CHAOTIC) : (sgn(mdef->data->maligntyp) >= 0)){
		if(check_oprop(otmp, OPROP_ANARW))
			*truedmgptr += basedmg;
		if(check_oprop(otmp, OPROP_LESSER_ANARW))
			*truedmgptr += d(2, 6);
	}
	if(youdef ? (u.ualign.type != A_NEUTRAL) : (sgn(mdef->data->maligntyp) != 0)){
		if(check_oprop(otmp, OPROP_CONCW))
			*truedmgptr += basedmg;
		if(check_oprop(otmp, OPROP_LESSER_CONCW))
			*truedmgptr += d(2, 6);
	}
	if(youdef ? (u.ualign.type != A_LAWFUL) : (sgn(mdef->data->maligntyp) <= 0)){
		if(check_oprop(otmp, OPROP_AXIOW))
			*truedmgptr += basedmg;
		if(check_oprop(otmp, OPROP_OONA_FIREW) || check_oprop(otmp, OPROP_OONA_COLDW) || check_oprop(otmp, OPROP_OONA_ELECW))
			*truedmgptr += d(1, 8);
		if(check_oprop(otmp, OPROP_LESSER_AXIOW))
			*truedmgptr += d(2, 6);
	}
	if((youdef ? (hates_holy(youracedata)) : (hates_holy_mon(mdef))) && otmp->blessed){
		if(check_oprop(otmp, OPROP_HOLYW))
			*truedmgptr += basedmg;
		if(check_oprop(otmp, OPROP_LESSER_HOLYW))
			*truedmgptr += d(2, 6);
	}
	if((youdef ? (hates_unholy(youracedata)) : (hates_unholy_mon(mdef))) && otmp->cursed){
		if(check_oprop(otmp, OPROP_UNHYW))
			*truedmgptr += basedmg;
		if(check_oprop(otmp, OPROP_LESSER_UNHYW))
			*truedmgptr += d(2, 6);
	}
	if((youdef ? (hates_unblessed(youracedata)) : (hates_unblessed_mon(mdef))) && !otmp->cursed && !otmp->blessed){
		if(check_oprop(otmp, OPROP_CONCW))
			*truedmgptr += basedmg;
		if(check_oprop(otmp, OPROP_LESSER_CONCW))
			*truedmgptr += d(2, 6);
	}
	if(check_oprop(otmp, OPROP_SFLMW) && !(youagr && FLAME_BAD) && sflm_target(mdef)){
		*truedmgptr += d(2,7);
	}
	if(check_oprop(otmp, OPROP_ANTAW)){
		int mult = 1;
		if(shock_vulnerable_species(mdef))
			mult *= 2;
		if(!Shock_res(mdef) && magr)
			*truedmgptr += mult*atr_dbon(otmp, magr, A_INT);

		if(check_reanimation(ANTENNA_ERRANT)){
			if(youdef && !Tele_blind && (Blind_telepat || !rn2(5))){
				*truedmgptr += mult*(rnd(15) + otmp->spe);
			}
			else if(!youdef && !mindless_mon(mdef) && (mon_resistance(mdef,TELEPAT) || !rn2(5))){
				*truedmgptr += mult*(rnd(15) + otmp->spe);
			}
		}

		if(check_reanimation(ANTENNA_BOLT)){
			int modifier = (youdef ? (Blind_telepat && !Tele_blind) : mon_resistance(mdef, TELEPAT)) ? 2 : 1;
			(*truedmgptr) += mult*modifier*(rnd(10) + otmp->spe);
		}
	}
	return ((*truedmgptr != original_truedmgptr) || (*plusdmgptr != original_plusdmgptr));
}

/* special damage bonus */
/* returns FALSE if no bonus damage was applicable */
/* Just do bonus damage, don't make any modifications to the defender */
boolean
material_dbon(otmp, magr, mdef, basedmg, plusdmgptr, truedmgptr, dieroll)
struct obj * otmp;
struct monst * magr;
struct monst * mdef;
int basedmg;
int * plusdmgptr;
int * truedmgptr;
int dieroll;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct permonst * pd = (youdef ? youracedata : mdef->data);
	int original_plusdmgptr = *plusdmgptr;
	int original_truedmgptr = *truedmgptr;
	
	if(otmp->obj_material == MERCURIAL && magr && mlev(magr) > 20 && (
		(youagr && Insight > 20 && YOU_MERC_SPECIAL)
		|| (!youagr && insightful(magr->data) && is_chaotic_mon(magr))
	)){
		if(is_streaming_merc(otmp)){
			int abil[] = {A_INT, A_WIS, A_CHA};
			int i = 0;
			boolean notResisted = TRUE;
			int dsize;
			for(i = 0; i < SIZE(abil); i++){
				switch(hash((unsigned long) (nonce + otmp->o_id + youmonst.movement + i))%4){
					case 0:
						if(Fire_res(mdef))
							notResisted = FALSE;
					break;
					case 1:
						if(Cold_res(mdef))
							notResisted = FALSE;
					break;
					case 2:
						if(Shock_res(mdef))
							notResisted = FALSE;
					break;
					case 3:
						if(Acid_res(mdef))
							notResisted = FALSE;
					break;
				}
				if(notResisted){
					if(youagr){
						if(ACURR(abil[i]) >= 15){
							if(ACURR(abil[i]) >= 25)
								dsize = (Insight + u.usanity)/10;
							else if(ACURR(abil[i]) >= 18)
								dsize = (Insight + u.usanity)/20;
							else /*>= 15*/
								dsize = (Insight + u.usanity)/40;
							
							*truedmgptr += rnd(dsize);
						}
					}
					else {
						*truedmgptr += d(1, mlev(magr)/5);
					}
				}
			}
		}
		else if(is_chained_merc(otmp)){
			if(youdef){
				if(u.uen > 0){
					u.uen -= youagr ? Insight/10 : mlev(magr)/10;
					flags.botl = 1;
				}
				if((dieroll == 1 && (youagr ? Insight : mlev(magr)) >= 40) || u.uen < 0){
					if(u.uen > 0){
						u.uen = max(u.uen-400, 0);
						flags.botl = 1;
					}
				}
			}
			else {
				mdef->mspec_used += rnd(youagr ? Insight/10 : mlev(magr)/10);
				if(dieroll == 1 && (youagr ? Insight : mlev(magr)) >= 40)
					set_mcan(mdef, TRUE);
			}
			int wt = youagr ? inv_weight() : curr_mon_load(magr);
			if(wt >= 160){
				wt /= 160;
				if(wt > 80)
					wt = 80;
				*plusdmgptr += rnd(wt);
			}
		}
		else if(is_kinstealing_merc(otmp)){
			//
		}
	}

	return ((*truedmgptr != original_truedmgptr) || (*plusdmgptr != original_plusdmgptr));
}

void
weapon_conflict(struct monst *mdef, struct monst *magr, int spe, boolean lethal, boolean mercy_blade)
{
	int x, y, cx, cy, count = 0;
	struct monst *target;
	struct monst *targets[8];
	extern const int clockwisex[8];
	extern const int clockwisey[8];
	boolean youdef = (mdef == &youmonst);
	boolean youagr = (magr == &youmonst);
	boolean youtar;
	static boolean in_conflict = FALSE;
	
	if(in_conflict)
		return;

	in_conflict = TRUE;
	
	x = x(mdef);
	y = y(mdef);
	for(int i = 0; i < 8; i++){
		cx = x + clockwisex[i];
		cy = y + clockwisey[i];
		if(!isok(cx, cy))
			continue;
		target = m_u_at(cx, cy);
		if(!target)
			continue;

		if(target == mdef || target == magr)
			continue;

		youtar = (target == &youmonst);

		if(youtar){
			if(youagr)
				continue;
			else if(magr->mpeaceful)
				continue;
		}

		if(DEADMONSTER(target))
			continue;


		if(	(youagr && target->mpeaceful)
			|| (youdef && !target->mpeaceful)
			|| (youtar && magr->mpeaceful)
			|| (!youagr && !youdef && !youtar && (magr->mpeaceful == target->mpeaceful))
		)
			continue;

		if(!youtar && nonthreat(target))
			continue;

		targets[count++] = target;
	}
	if(count){
		target = targets[rn2(count)];
		if(youdef){
			int temp_encouraged = u.uencouraged;
			if(lethal)
				pline("The blade lodges in your %s!", body_part(mercy_blade ? SPINE : BRAIN));
			u.uencouraged = (youagr ? (Insight + ACURR(A_CHA))/5 : magr->m_lev/5) + spe;
			flags.forcefight = TRUE;
			xattacky(mdef, target, x(target), y(target));
			flags.forcefight = FALSE;
			u.uencouraged = temp_encouraged;
		}
		else {
			int temp_encouraged = mdef->encouraged;
			boolean friendly_fire;
			long result;
			mdef->encouraged = (youagr ? (Insight + ACURR(A_CHA))/5 : magr->m_lev/5) + spe;
			if(lethal)
				pline("The blade lodges in %s %s!", s_suffix(mon_nam(mdef)), mbodypart(mdef, mercy_blade ? SPINE : BRAIN));
			friendly_fire = !mm_grudge(mdef, target, FALSE);
			result = xattacky(mdef, target, x(target), y(target));
			if(friendly_fire && (result&(MM_DEF_DIED|MM_DEF_LSVD)) && !taxes_sanity(mdef->data) && !mindless_mon(mdef) && !resist(mdef, POTION_CLASS, 0, NOTELL)){
				if (canseemon(mdef) && !lethal)
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
			mdef->encouraged = temp_encouraged;
		}
	}

	in_conflict = FALSE;
}

void
mercy_blade_conflict(struct monst *mdef, struct monst *magr, int spe, boolean lethal)
{
	weapon_conflict(mdef, magr, spe, lethal, TRUE);
}

void
mind_blade_conflict(struct monst *mdef, struct monst *magr, int spe, boolean lethal)
{
	weapon_conflict(mdef, magr, spe, lethal, FALSE);
}

void
mindstealer_conflict(mdef, magr)
struct monst *mdef;
struct monst *magr;
{
	int x, y, cx, cy, count = 0;
	struct monst *target;
	struct monst *targets[8];
	extern const int clockwisex[8];
	extern const int clockwisey[8];
	boolean youdef = (mdef == &youmonst);
	boolean youagr = (magr == &youmonst);
	boolean youtar;
	static boolean in_conflict = FALSE;
	
	if(in_conflict)
		return;

	in_conflict = TRUE;
	
	x = x(mdef);
	y = y(mdef);
	for(int i = 0; i < 8; i++){
		cx = x + clockwisex[i];
		cy = y + clockwisey[i];
		if(!isok(cx, cy))
			continue;
		target = m_u_at(cx, cy);
		if(!target)
			continue;

		if(target == mdef || target == magr)
			continue;

		youtar = (target == &youmonst);

		if(youtar){
			if(youagr)
				continue;
			else if(magr->mpeaceful)
				continue;
		}

		if(DEADMONSTER(target))
			continue;


		if(	(youagr && target->mpeaceful)
			|| (youdef && !target->mpeaceful)
			|| (youtar && magr->mpeaceful)
			|| (!youagr && !youdef && !youtar && (magr->mpeaceful == target->mpeaceful))
		)
			continue;

		if(!youtar && nonthreat(target))
			continue;

		targets[count++] = target;
	}
	if(count){
		target = targets[rn2(count)];
		if(youdef){
			int temp_encouraged = u.uencouraged;
			u.uencouraged = (youagr ? (Insight + ACURR(A_CHA))/5 : magr->m_lev/5);
			flags.forcefight = TRUE;
			xattacky(mdef, target, x(target), y(target));
			flags.forcefight = FALSE;
			u.uencouraged = temp_encouraged;
		}
		else {
			int temp_encouraged = mdef->encouraged;
			mdef->encouraged = (youagr ? (Insight + ACURR(A_CHA))/5 : magr->m_lev/5);
			xattacky(mdef, target, x(target), y(target));
			mdef->encouraged = temp_encouraged;
		}
	}
	in_conflict = FALSE;
}

/*  */
void
otyp_hit(magr, mdef, otmp, basedmg, plusdmgptr, truedmgptr, dieroll, hittxt, printmessages, direct_weapon)
struct monst *magr, *mdef;
struct obj *otmp;
int basedmg;
int * plusdmgptr;
int * truedmgptr;
int dieroll; /* needed for Magicbane and vorpal blades */
boolean * hittxt;
boolean printmessages;
boolean direct_weapon;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct permonst * pd = (youdef ? youracedata : mdef->data);
	
	if (otmp->otyp == TORCH && otmp->lamplit) {
		if (!Fire_res(mdef)) {
			if (species_resists_cold(mdef))
				(*truedmgptr) += 3 * (rnd(10) + otmp->spe) / 2;
			else
				(*truedmgptr) += rnd(10) + otmp->spe;
		}
		if (!UseInvFire_res(mdef)){
			if (!rn2(3)) destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
			if (!rn2(3)) destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
			if (!rn2(3)) destroy_item(mdef, POTION_CLASS, AD_FIRE);
		}
	    if (youdef) burn_away_slime();
	    if (youdef && FrozenAir) melt_frozen_air();
	}
	if (otmp->otyp == MAGIC_TORCH && otmp->lamplit){
		if (!Fire_res(mdef)) {
			if (species_resists_cold(mdef))
				(*truedmgptr) += 3 * (rnd(8) + 2*otmp->spe) / 2;
			else
				(*truedmgptr) += rnd(8) + 2*otmp->spe;
		}
	}
	if (otmp->otyp == SHADOWLANDER_S_TORCH && otmp->lamplit){
		if (!Cold_res(mdef)) {
			if (species_resists_fire(mdef))
				(*truedmgptr) += 3 * (rnd(10) + otmp->spe) / 2;
			else
				(*truedmgptr) += rnd(10) + otmp->spe;
		}
		if (!UseInvCold_res(mdef)){
			if (!rn2(3)) destroy_item(mdef, POTION_CLASS, AD_COLD);
		}
	}
	if (otmp->otyp == SUNROD && otmp->lamplit) {
		int num = 1, den = 1;
		if(!Shock_res(mdef) && !Acid_res(mdef)){
			num *= 3;
			den *= 2;
		}
		if(shock_vulnerable_species(mdef)){
			num *= 2;
		}
		if(!Shock_res(mdef) || !Acid_res(mdef)){
			(*truedmgptr) += num * (rnd(10) + otmp->spe) / den;
		}
		if (!UseInvShock_res(mdef)){
			if (!rn2(3)) destroy_item(mdef, WAND_CLASS, AD_ELEC);
			if (!rn2(3)) destroy_item(mdef, RING_CLASS, AD_ELEC);
		}
		if (!UseInvAcid_res(mdef)){
			if (rn2(3)) destroy_item(mdef, POTION_CLASS, AD_FIRE);
		}
		if (!resists_blnd(mdef)) {
			if (youdef) {
				if(printmessages){
					You("are blinded by the sun-bright rod!");
					*hittxt = TRUE;
				}
				make_blinded((long)d(1, 50), FALSE);
				if (!Blind) Your1(vision_clears);
			}
			else if (!(youagr && u.uswallow && mdef == u.ustuck)) {
				register unsigned rnd_tmp = rnd(50);
				mdef->mcansee = 0;
				if ((mdef->mblinded + rnd_tmp) > 127)
					mdef->mblinded = 127;
				else mdef->mblinded += rnd_tmp;
			}
		}
	}
	if (otmp->otyp == TOOTH && Insight >= 20 && otmp->o_e_trait&ETRAIT_FOCUS_FIRE && CHECK_ETRAIT(otmp, magr, ETRAIT_FOCUS_FIRE)){
		if(otmp->ovar1_tooth_type == MAGMA_TOOTH){
			if (!Fire_res(mdef)) {
				if (species_resists_cold(mdef))
					(*truedmgptr) += 3 * (d(5,10) + otmp->spe) / 2;
				else
					(*truedmgptr) += d(5,10) + otmp->spe;
			}
			if (!UseInvFire_res(mdef)){
				if (rn2(3)) destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
				if (rn2(3)) destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
				if (rn2(3)) destroy_item(mdef, POTION_CLASS, AD_FIRE);
			}
		}
		else if(otmp->ovar1_tooth_type == SERPENT_TOOTH){
			if (!Poison_res(mdef)) {
				(*truedmgptr) += d(1,8) + otmp->spe;
			}
			if (!Acid_res(mdef)) {
				(*truedmgptr) += d(1,8) + otmp->spe;
			}
			if (!UseInvAcid_res(mdef)){
				if (rn2(3)) destroy_item(mdef, POTION_CLASS, AD_FIRE);
			}
		}
		else if(otmp->ovar1_tooth_type == VOID_TOOTH){
			if (!Cold_res(mdef)) {
				(*truedmgptr) += d(3,3) + otmp->spe;
			}
			if (!UseInvCold_res(mdef)){
				if (rn2(3)) destroy_item(mdef, POTION_CLASS, AD_COLD);
			}
		}
	}
	
	if (otmp->otyp == TONITRUS && otmp->lamplit){
		int modifier = (youdef ? (Blind_telepat && !Tele_blind) : mon_resistance(mdef, TELEPAT)) ? 2 : 1;
		if(shock_vulnerable_species(mdef))
			modifier += 1;
		if (!Shock_res(mdef)) {
			if(magr)
				(*truedmgptr) += modifier*(rnd(10) + otmp->spe) + atr_dbon(otmp, magr, A_INT);
			else
				(*truedmgptr) += modifier*(rnd(10) + otmp->spe);
		}
		if (!UseInvShock_res(mdef)){
			if (!rn2(3)) destroy_item(mdef, WAND_CLASS, AD_ELEC);
			if (!rn2(3)) destroy_item(mdef, RING_CLASS, AD_ELEC);
		}
	}

	if (otmp->otyp == STAKE && is_vampire(mdef->data) && !naoid(mdef->data)) {
		*plusdmgptr += d(1, 6);
	}

	if (otmp->otyp == KAMEREL_VAJRA && litsaber(otmp)) {
		if (!Shock_res(mdef)) {
			int num = 1;
			if(shock_vulnerable_species(mdef)){
				num *= 2;
			}
			if (otmp->where == OBJ_MINVENT && otmp->ocarry->mtyp == PM_ARA_KAMEREL)
				*truedmgptr += num*d(6, 6);
			else
				*truedmgptr += num*d(2, 6);
		}
		if (!UseInvShock_res(mdef)) {
			if (!rn2(3)) destroy_item(mdef, WAND_CLASS, AD_ELEC);
			if (!rn2(3)) destroy_item(mdef, RING_CLASS, AD_ELEC);
		}
		if (!resists_blnd(mdef)) {
			if (youdef) {
				if(printmessages){
					You("are blinded by the flashing blade!");
					*hittxt = TRUE;
				}
				make_blinded((long)d(1, 50), FALSE);
				if (!Blind) Your1(vision_clears);
			}
			else if (!(youagr && u.uswallow && mdef == u.ustuck)) {
				register unsigned rnd_tmp = rnd(50);
				mdef->mcansee = 0;
				if ((mdef->mblinded + rnd_tmp) > 127)
					mdef->mblinded = 127;
				else mdef->mblinded += rnd_tmp;
			}
		}
	}

	if(force_weapon(otmp) && otmp->ovar1_charges && !on_level(&spire_level,&u.uz)){
		int bonus = 0;
		//Non-elemental energy attack: 1 die plus the remaining half die
		bonus += rnd((mdef && bigmonst(pd)) ? 
						objects[otmp->otyp].oc_wldam.oc_damd : 
						objects[otmp->otyp].oc_wsdam.oc_damd);
		bonus += (mdef && bigmonst(pd)) ? 
					(objects[otmp->otyp].oc_wldam.oc_damd+1)/2 : 
					(objects[otmp->otyp].oc_wsdam.oc_damd+1)/2;
		if(otmp->otyp == FORCE_WHIP){
			bonus += (mdef && bigmonst(pd)) ? 
							rnd(4)+2 : 
							2;
		}
		if(otmp->otyp == DOUBLE_FORCE_BLADE &&
			((otmp == uwep && !u.twoweap) || (mcarried(otmp) && otmp->owornmask&W_WEP))
		){
			bonus *= 2;
		}
		*truedmgptr += bonus;
	}
	
	if(otmp->otyp == DISKOS && Insight >= 15 && !on_level(&spire_level,&u.uz)){
		int bonus = 0;
		//Holy/Unholy energy attack
		if(Insight >= 50){
			bonus += d(3, (mdef && bigmonst(pd)) ? 
							objects[otmp->otyp].oc_wldam.oc_damd : 
							objects[otmp->otyp].oc_wsdam.oc_damd);
			bonus += (mdef && bigmonst(pd)) ? 
						(objects[otmp->otyp].oc_wldam.oc_damd) : 
						(objects[otmp->otyp].oc_wsdam.oc_damd);
		} else if(Insight >= 45){
			bonus += d(3, (mdef && bigmonst(pd)) ? 
							objects[otmp->otyp].oc_wldam.oc_damd : 
							objects[otmp->otyp].oc_wsdam.oc_damd);
		} else if(Insight >= 20){
			bonus += d(2, (mdef && bigmonst(pd)) ? 
							objects[otmp->otyp].oc_wldam.oc_damd : 
							objects[otmp->otyp].oc_wsdam.oc_damd);
		} else { //>= 15
			bonus += d(1, (mdef && bigmonst(pd)) ? 
							objects[otmp->otyp].oc_wldam.oc_damd : 
							objects[otmp->otyp].oc_wsdam.oc_damd);
		}
		if(mdef){
			if(youagr){
				int bonus_die = (mdef && bigmonst(pd)) ? 
							objects[otmp->otyp].oc_wldam.oc_damd : 
							objects[otmp->otyp].oc_wsdam.oc_damd;
				if(u.ualign.record < -3 && Insanity > 50)
					bonus += bonus_die*(50-u.usanity)/50;
				else if(u.ualign.record > 3 && u.usanity > 90)
					bonus += bonus_die*(10-Insanity)/10;

				if(u.ualign.record < -3 && hates_unholy_mon(mdef))
					bonus *= 2;
				else if(u.ualign.record > 3 && hates_holy_mon(mdef))
					bonus *= 2;
			}
			else if(magr){
				if((magr->mtyp == PM_UVUUDAUM || hates_holy_mon(magr)) && hates_unholy_mon(mdef))
					bonus *= 2;
				else if((magr->mtyp == PM_UVUUDAUM || hates_unholy_mon(magr)) && hates_holy_mon(mdef))
					bonus *= 2;
			}
		}
		bonus = reduce_dmg(mdef,bonus,FALSE,TRUE);
		*truedmgptr += bonus;
	}
	
	if(pure_weapon(otmp) && otmp->spe >= 6 && !on_level(&spire_level,&u.uz)){
		if(youagr){
			if(Upolyd && u.mh == u.mhmax)
				*plusdmgptr += basedmg*.2;
			else if(!Upolyd && u.uhp == u.uhpmax)
				*plusdmgptr += basedmg*.2;
		} else {
			if(magr && magr->mhp == magr->mhpmax)
				*plusdmgptr += basedmg*.2;
		}
	}
	
	if(dark_weapon(otmp) && otmp->spe >= 6 && !on_level(&spire_level,&u.uz)){
		if(youagr){
			if(Upolyd && u.mh <= u.mhmax*.3)
				*plusdmgptr += basedmg*.2;
			else if(!Upolyd && u.uhp <= u.uhpmax*.3)
				*plusdmgptr += basedmg*.2;
		} else {
			if(magr && magr->mhp <= magr->mhpmax*.3)
				*plusdmgptr += basedmg*.2;
		}
	}
	
	if(otmp->otyp == CROW_QUILL){
		if(youdef){
			u.ustdy += 4;
		} else if(mdef){
			mdef->mstdy += 4;
		}
	}
	if(otmp->otyp == BESTIAL_CLAW && !on_level(&spire_level,&u.uz)){
		int insight_mod = 0;
		int studystack = 0;
		if(youagr){
			if(active_glyph(BEASTS_EMBRACE))
				insight_mod = 30*pow(.97,Insight);
		}
		else if(magr){
			if(magr->mcrazed)
				insight_mod = 30.0*pow(.97,(yields_insight(magr->data) ? u_insight_gain(magr) : 0) + magr->m_insight_level);
		}
		if(youdef){
			studystack = min((basedmg+9)/10, insight_mod - u.ustdy);
			if(studystack > 0) u.ustdy += studystack;
		} else if(mdef){
			studystack = min((basedmg+9)/10, insight_mod - mdef->mstdy);
			if(studystack > 0) mdef->mstdy += studystack;
		}
		if(youagr){
			studystack = min((basedmg+19)/20, insight_mod - u.ustdy);
			if(studystack > 0) u.ustdy += studystack;
		} else if(magr){
			studystack = min((basedmg+19)/20, insight_mod - magr->mstdy);
			if(studystack > 0) magr->mstdy += studystack;
		}
	}
	//Called once per blade striking
	if(otmp->otyp == SET_OF_CROW_TALONS){
		if(youdef){
			u.ustdy += 3;
		} else if(mdef){
			mdef->mstdy += 3;
		}
	}
	if(otmp->otyp == MOON_AXE && otmp->ovar1_moonPhase == HUNTING_MOON && Insight >= 5){
		if(youdef){
			u.ustdy += rnd(Insight/5);
		} else if(mdef){
			mdef->mstdy += rnd(Insight/5);
		}
	}
	//Banishes summoned monsters
	if(otmp->otyp == CHURCH_HAMMER && !youdef && mdef && get_mx(mdef, MX_ESUM)){
		*truedmgptr += 10*Insight;
	}

	//Flogging raises sanity (note: this is "backwards" on purpose)
	if((otmp->otyp == CANE && !youdef && mdef && is_serration_vulnerable(mdef) && Insight >= rnd(100))
	|| (otmp->otyp == WHIP_SAW && !youdef && mdef && hates_holy_mon(mdef) && Insight >= rnd(100))
	|| (otmp->otyp == CHURCH_SHORTSWORD && (resist_pierce(mdef->data) && !resist_slash(mdef->data)) && Insight >= rnd(100))
	){
		int reglevel = san_threshhold() + otmp->spe;
		if(u.usanity < reglevel){
			if(rn2(10) < basedmg)
				change_usanity(1, FALSE);
		}
		else if(u.usanity < reglevel*2){
			if(rn2(32) < basedmg)
				change_usanity(1, FALSE);
		}
		else if(u.usanity < reglevel*3){
			if(rn2(100) < basedmg)
				change_usanity(1, FALSE);
		}
	}
	if(otmp->otyp == ISAMUSEI && Insight >= 10 && !on_level(&spire_level,&u.uz) && mdef){
		if(youdef || !resist(mdef, WEAPON_CLASS, 0, TRUE)){
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
			if(Half_phys(mdef))
				factor *= 2;
			*hp(mdef) -= *hp(mdef)/factor;
			// pline("off: %d", *hp(mdef)/factor);
		}
	}

	if(otmp->otyp == PINCER_STAFF && Insight >= 10 && !on_level(&spire_level,&u.uz) && mdef){
		if(otmp->ovar1_pincerTarget == mdef->m_id){
			*plusdmgptr += basedmg;
			if (Insight >= 20 && otmp->oartifact && otmp->oartifact == ART_FALLINGSTAR_MANDIBLES && !Magic_res(mdef)){
				*truedmgptr += reduce_dmg(mdef, d(1, 12), FALSE, TRUE);
			}
		}
		else otmp->ovar1_pincerTarget = mdef->m_id;
		if(Insight >= 50){
			struct obj *armor = some_armor(mdef);
			if(!armor){
				*plusdmgptr += basedmg;
			}
			else if(!obj_resists(armor, 20, 80)){
				obj_extract_and_unequip_self(armor);
				if(youdef){
					if(printmessages){
						pline("The grasping staff tears off your %s", simple_typename(armor->otyp));
						*hittxt = TRUE;
					}
					dropy(armor);
				}
				else {
					if(printmessages && canspotmon(mdef)){
						pline("The grasping staff tears off %s %s", s_suffix(mon_nam(mdef)), simple_typename(armor->otyp));
						*hittxt = TRUE;
					}
					mdrop_obj(mdef,armor,FALSE);
				}
			}
		}
	}

	if(otmp->otyp == SHANTA_PATA && mdef){
		if(otmp->ovar1_last_blooded > moves - 10){
			if(youagr)
				*plusdmgptr += u.uimpurity/2;
			if(!u.veil && !Magic_res(mdef)){
				int mod = min(Insight, 100);
				*truedmgptr += basedmg*mod/100;
			}
			if(has_blood_mon(mdef) && direct_weapon){
				otmp->ovar1_last_blooded += 10;
			}
		}
		else {
			if(has_blood_mon(mdef) && direct_weapon){
				otmp->ovar1_last_blooded = moves;
			}
		}
	}
	else if(otmp->otyp == TWINGUN_SHANTA && mdef){
		if(otmp->ovar1_last_blooded > moves - 10){
			if(youagr)
				*plusdmgptr += u.uimpurity/4;
			if(!u.veil && !Magic_res(mdef)){
				int mod = min(Insight/2, 100);
				*truedmgptr += basedmg*mod/100;
			}
			if(has_blood_mon(mdef) && direct_weapon){
				otmp->ovar1_last_blooded += 10;
			}
		}
		else {
			if(has_blood_mon(mdef) && direct_weapon){
				otmp->ovar1_last_blooded = moves;
			}
		}
	}

	if(otmp->otyp == DEVIL_FIST && !on_level(&spire_level,&u.uz) && otmp->cobj && mdef){

		switch(otmp->cobj->otyp){
			case WAGE_OF_WRATH:
			case WAGE_OF_ENVY:
				if(!Fire_res(mdef)){
					*truedmgptr += d(2, 9);
				}
			break;
			case WAGE_OF_GREED:
			case WAGE_OF_GLUTTONY:
				if(!Acid_res(mdef)){
					*truedmgptr += d(2, 9);
				}
			break;
			case WAGE_OF_SLOTH:
				if(!Cold_res(mdef)){
					*truedmgptr += d(2, 9);
				}
			break;
			case WAGE_OF_PRIDE:
				if(!Shock_res(mdef)){
					if(shock_vulnerable_species(mdef)){
						*truedmgptr += d(4, 9);
					}
					else
						*truedmgptr += d(2, 9);
				}
			break;
		}
	}

	if(otmp->otyp == DEMON_CLAW && !on_level(&spire_level,&u.uz) && otmp->cobj){

		switch(otmp->cobj->otyp){
			case WAGE_OF_ENVY:
				if(mdef){
					int n = 0;
					for(struct obj *inv = youdef ? invent : mdef->minvent; inv && n < mlev(magr); inv = inv->nobj)
						n++;
					n = (n+2)/3;
					if(n){
						*plusdmgptr += d(n,6);
						if(hates_unholy_mon(mdef))
							*truedmgptr += d(n,6);
					}
				}
			break;
			case WAGE_OF_GREED:
				//Item theft?
				//Money held by attacker
				if(magr){
					int gold;
					int n = 0;
					if(youagr){
#ifndef GOLDOBJ
						gold = u.ugold;
#else
						gold = money_cnt(invent);
#endif
					}
					else {
#ifndef GOLDOBJ
						gold = magr->mgold;
#else
						gold = money_cnt(magr->minvent);
#endif
					}
					gold /= 616;
					if(gold)
						n++;
					while((gold = gold/6))
						n++;
					if(n){
						*plusdmgptr += d(n,6);
						if(hates_unholy_mon(mdef))
							*truedmgptr += d(n,6);
					}
				}
			break;
			case WAGE_OF_GLUTTONY:{
				//Starve target?
				//Nutrition of attacker?
				int n = 0;
				if(youagr){
					if(YouHunger > get_satiationlimit()){
						n = 4;
						morehungry(20);
					}
					else if(YouHunger > 150*get_uhungersizemod()){
						n = 2;
						morehungry(10);
					}
					else if(YouHunger > 50*get_uhungersizemod()){
						n = 1;
						morehungry(5);
					}
				}
				else {
					n = 2; //The PC is more detailed
				}
				if(n){
					*plusdmgptr += d(n,6);
					if(n/2 && hates_unholy_mon(mdef))
						*truedmgptr += d(n/2,6);
				}
			}
			break;
			case WAGE_OF_SLOTH:
				if(mdef){
					if(!youdef && !resist(mdef, WEAPON_CLASS, 0, NOTELL)){
						mdef->mspeed = MSLOW;
						mdef->permspeed = MSLOW;
					}
					mdef->movement -= 2; //1/6th of a standard turn
				}
			break;
			case WAGE_OF_PRIDE:
				//Debuf target
				if(magr && mdef && mlev(magr) > 0){
					int level = min(mlev(magr), 36);
					if (youdef){
						u.ustdy += d((level+2)/3, 6);
						u.uencouraged = min(u.uencouraged, max(u.uencouraged - level/6, -6));
					} else {
						mdef->mstdy += d((level+2)/3, 6);
						mdef->encouraged = min(u.uencouraged, max(mdef->encouraged - level/6, -6));
					}
				}
			break;
		}
	}
}

/* returns MM_style hitdata now, and is used for both artifacts and weapon properties */
int
special_weapon_hit(magr, mdef, otmp, msgr, basedmg, plusdmgptr, truedmgptr, dieroll, messaged, printmessages)
struct monst * magr;
struct monst * mdef;
struct obj * otmp;
struct obj * msgr;		/* object to describe as doing the hitting (even though otmp is causing the special effects) */
int basedmg;
int * plusdmgptr;
int * truedmgptr;
int dieroll; /* needed for Magicbane and vorpal blades */
boolean * messaged;
boolean printmessages; /* print generic elemental damage messages */
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct permonst * pd = (youdef ? youracedata : mdef->data);
	boolean vis = getvis(magr, mdef, 0, 0);

	int oartifact = otmp->oartifact;
	
	int goatweaponturn = 0;
	int sothweaponturn = 0;
	int mercweaponslice[3] = {0};

	char hittee[BUFSZ];
	static const char you[] = "you";
	Strcpy(hittee, youdef ? you : mon_nam(mdef));

	const char *wepdesc;

	const struct artifact *arti_struct = get_artifact(otmp);
	
	if(on_level(&spire_level,&u.uz))
		return MM_HIT;

	/* set msgr to otmp if we weren't given one */
	/* for artifacts, we must assume they are used properly -- msgr will not override artifact-specific hitmsgs */
	if (!msgr)
		msgr = otmp;
	
	if(check_oprop(otmp,OPROP_GOATW) && !(youagr && GOAT_BAD))
		goatweaponturn = goat_weapon_damage_turn(otmp);
	if(check_oprop(otmp,OPROP_SOTHW) && !(youagr && YOG_BAD))
		sothweaponturn = soth_weapon_damage_turn(otmp);
	if(otmp->obj_material == MERCURIAL){
		mercweaponslice[0] = merc_weapon_damage_slice(otmp, magr, A_INT);
		mercweaponslice[1] = merc_weapon_damage_slice(otmp, magr, A_WIS);
		mercweaponslice[2] = merc_weapon_damage_slice(otmp, magr, A_CHA);
	}
	
#define currdmg (basedmg + *plusdmgptr + *truedmgptr)

	/* apply spec_dbon to appropriate damage pointers */
	if (oartifact)
		spec_dbon(otmp, mdef, basedmg, plusdmgptr, truedmgptr);
	if (!check_oprop(otmp, OPROP_NONE))
		oproperty_dbon(otmp, magr, mdef, basedmg, plusdmgptr, truedmgptr);
	if (otmp->obj_material == MERCURIAL)
		material_dbon(otmp, magr, mdef, basedmg, plusdmgptr, truedmgptr, dieroll);
	
	/* this didn't trigger spec_dbon_applies, but still needs to happen later */
	if (dieroll <= 2 && youagr && otmp->oclass == SPBOOK_CLASS && (u.sealsActive&SEAL_PAIMON)
		&& !Drain_res(mdef) && !otmp->oartifact && otmp->otyp != SPE_BLANK_PAPER && otmp->otyp != SPE_SECRETS)
		spec_dbon_applies = TRUE;

	/* EXTERNAL damage sources -- explosions and the like, primarily */
	/* knockback effect */
	if (((arti_attack_prop(otmp, ARTA_KNOCKBACK) && !rn2(4))
		|| arti_attack_prop(otmp, ARTA_KNOCKBACKX)
		|| (otmp->otyp == DEMON_CLAW && !on_level(&spire_level,&u.uz) && otmp->cobj && otmp->cobj->otyp == WAGE_OF_LUST)
		|| (otmp->otyp == IMPERIAL_ELVEN_BOOTS && check_imp_mod(otmp, IEA_KICKING) && check_imp_mod(otmp, IEA_JUMPING))
	) && !(
		/* exclusions below */
		(oartifact == ART_TOBIUME) /* Tobiume only does the knockback if mdef is nearly dead */
		))
	{
		/* determine if we do the full hurtle, or just stun */
		int hurtledistance;
		
		if (!magr)
			hurtledistance = 0;	/* we can't tell which way to throw mdef! */
		else if (mdef->data->msize >= MZ_HUGE)
			hurtledistance = 0;
		else
			hurtledistance = min(BOLT_LIM, 3*(20-dieroll)*basedmg/(*hp(mdef)));

		/* message */
		if (vis) {
			pline("%s %s %s by %s blow!",
				(youdef ? "You" : Monnam(mdef)),
				(youdef ? "are" : "is"),
				(hurtledistance > 1 ? "thrown" : "stunned"),
				(youagr ? "your" : (magr && (vis&VIS_MAGR)) ? s_suffix(mon_nam(magr)) : magr ? "a" : "the")
				);
			*messaged = TRUE;
		}
		/* do we stun, or do the full hurtle? Based on damage. */
		if (hurtledistance > 1) {
			/* hurtle */
			int dx = x(mdef) - x(magr);
			int dy = y(mdef) - y(magr);
			if (youdef) {
				hurtle(dx, dy, hurtledistance, FALSE, TRUE);
			}
			else {
				mhurtle(mdef, dx, dy, hurtledistance, FALSE);
				if (DEADMONSTER(mdef))
					return MM_DEF_DIED;
				if(MIGRATINGMONSTER(mdef))
					return MM_AGR_STOP;
			}
		}
		else {
			/* just stun with a small movement slow */
			if (youdef) {
				make_stunned((HStun + 3), FALSE);
				youmonst.movement -= 2;
			}
			else {
				mdef->mstun = 1;
				mdef->movement -= 2;
			}
		}
		if (DEADMONSTER(mdef))
			return MM_DEF_DIED;
		if (migrating_mons == mdef)
			return MM_AGR_STOP; //Monster was killed as part of movement OR fell to new level and we should stop.
	}
	/* fire explosions */
	if (((arti_attack_prop(otmp, ARTA_EXPLFIRE) && !rn2(4)) || arti_attack_prop(otmp, ARTA_EXPLFIREX)) && !(
		/* exclusion */
		(oartifact == ART_TOBIUME)
		))
	{
		explode(x(mdef), y(mdef),
			AD_FIRE, 0,
			d(6, 6),
			EXPL_FIERY, 1);
	}
	/* cold explosions */
	if ((arti_attack_prop(otmp, ARTA_EXPLCOLD) && !rn2(4)) || arti_attack_prop(otmp, ARTA_EXPLCOLDX))
	{
		explode(x(mdef), y(mdef),
			AD_COLD, 0,
			d(6, 6),
			EXPL_FROSTY, 1);
	}
	/* elec explosions AND bolts */
	if ((arti_attack_prop(otmp, ARTA_EXPLELEC) && !rn2(4)) || arti_attack_prop(otmp, ARTA_EXPLELECX))
	{
		int deltax = 0;
		int deltay = 0;
		if (magr && mdef) {
			deltax = x(mdef) - x(magr);
			deltay = y(mdef) - y(magr);
		}
		/* create a bolt on striking an adjacent target */
		if ((deltax || deltay) &&
			(abs(deltax) <= 1) &&
			(abs(deltay) <= 1)
			){
			zap(magr, x(magr), y(magr), sgn(deltax), sgn(deltay), rn1(7, 7), basiczap(0, AD_ELEC, ZAP_WAND, 6));
		}
		/* otherwise explosion */
		else {
			explode(x(mdef), y(mdef),
				AD_ELEC, 0,
				d(6, 6),
				EXPL_MAGICAL, 1);
		}
	}
	/* shockwave knocks back and stuns */
	if (arti_attack_prop(otmp, ARTA_SONICX)){
		extern const int clockwisex[8];
		extern const int clockwisey[8];
		struct monst *tmpm;
		int sannum = youagr ? u.usanity : 100;
		int sonicboom = basedmg*sannum/100, i, offset, opposite, tx, ty;
		int peacefulness = youagr ? TRUE : magr->mpeaceful;
		offset = rn2(8);
		mdef->msleeping = 0;
		mdef->mcanhear = 0;
		mdef->mdeafened = max(sannum/5, mdef->mdeafened);
		mdef->mstun = 1;
		mdef->mconf = 1;
		You_hear("a deafening clap of thunder!");
		wake_nearto_noisy(x(mdef), y(mdef), sonicboom*sonicboom);
		for(i = 0; i<4; i++){
			for(opposite = 0; opposite<8; opposite+=4){
				tx = x(mdef) + clockwisex[(i+opposite+offset)%8];
				ty = y(mdef) + clockwisey[(i+opposite+offset)%8];
				if(!isok(tx,ty))
					continue;
				tmpm = m_at(tx,ty);
				if(tmpm){
					if(tmpm == magr)
						continue;
					mhurtle(tmpm, clockwisex[(i+opposite+offset)%8], clockwisey[(i+opposite+offset)%8], rnd(sannum/20), FALSE);
					if(DEADMONSTER(tmpm) || MIGRATINGMONSTER(tmpm))
						continue; /* Fell down a pit or died while hurtling*/
					if(peacefulness != tmpm->mpeaceful){
						tmpm->mhp -= sonicboom;
						if (tmpm->mhp <= 0){
							if(youagr)
								killed(tmpm);
							else
								monkilled(tmpm, "", AD_PHYS);
						}
						else {
							tmpm->msleeping = 0;
							tmpm->mcanhear = 0;
							tmpm->mdeafened = max(sannum/5, tmpm->mdeafened);
							tmpm->mstun = 1;
							tmpm->mconf = 1;
						}
					}
				}
			}
		}
		if(!youagr && !youdef && distmin(u.ux, u.uy, x(mdef), y(mdef)) == 1){
			You("are blown backwards by a blast of thunder!");
			hurtle(u.ux-x(mdef), u.uy-y(mdef), rnd(sannum/20), FALSE, FALSE);
			if(!peacefulness){
				losehp(sonicboom, "thunderblast", KILLED_BY_AN);
				make_stunned(HStun + sonicboom, TRUE);
			}
		}
	}
	/* Genocide */
	if (oartifact == ART_GENOCIDE) {
		struct monst *tmpm, *nmon;
		int genoburn;
		struct permonst * pd2;

		/* affect all creatures on the level, starting with player */
		for (tmpm = &youmonst; tmpm; tmpm = nmon) {
			if (tmpm == &youmonst) {
				pd2 = youracedata;
				nmon = fmon;
			}
			else {
				pd2 = tmpm->data;
				nmon = tmpm->nmon;
			}
			/* skip already-dead creatures */
			if (*hp(tmpm) < 1)
				continue;
			/* don't affect fire-resistant creatures */
			if (Fire_res(tmpm))
				continue;
			/* needs some kind of racial similarity between mdef and tmpm */
			if ((pd->mflagsa & pd2->mflagsa) == 0 && !(pd == pd2))
				continue;

			/* made it past the checks, deal damage and maybe even explode */
			genoburn = d(6, 6);

			if (*hp(tmpm) < currdmg + genoburn + 6){
				*hp(tmpm) = 1;
				explode(x(tmpm), y(tmpm),
					AD_FIRE, 0,
					d(6, 6),
					EXPL_FIERY, 1);
			}
			else
				*truedmgptr += genoburn;
		}
	}
	
	/* Shadowlock has a specific interaction with Nudzirath */
	if(youagr && oartifact == ART_SHADOWLOCK && u.specialSealsActive&SEAL_NUDZIRATH && !rn2(4)){
		int dsize = spiritDsize();
		explode(x(magr), y(magr), AD_PHYS, WEAPON_CLASS, d(5, dsize), EXPL_DARK, 1); //Obsidian Glass
		explode(x(mdef), y(mdef), AD_PHYS, WEAPON_CLASS, d(5, dsize), EXPL_DARK, 1); //Obsidian Glass
	}

	/* The Grappler's Grasp has a chance to begin grapples.  */
	if (oartifact == ART_GRAPPLER_S_GRASP || (otmp->otyp == IMPERIAL_ELVEN_GAUNTLETS && check_imp_mod(otmp, IEA_STRANGLE))) {
		/* check if we can begin a grapple -- Damage is done by adding an AT_HUGS to your attack chain, NOT here. */
		if ((youagr || youdef) && !u.ustuck && !sticks(mdef) && !(youagr && u.uavoid_grabattk))
		{
			int newres = xmeleehurty(magr, mdef, &grapple, &grapple, &otmp, (youagr || youdef), 0, dieroll, -1, FALSE);

			/* quick return in case we grabbed a cockatrice or something */
			if (newres & (MM_AGR_DIED | MM_AGR_STOP))
				return newres;
		}
	}

	/* The defender was killed, by the player, by the artifact's special external damage sources */
	/* return early; we don't want additional messages and we don't care about any additional damage either */
	if(youagr && *hp(mdef) <= 0)
		return MM_DEF_DIED; //killed it.

	/* Additional damage modifiers */

	/* limited moon's damage multiplier is reduced based on your pw */
	if (oartifact == ART_LIMITED_MOON && youagr) {
		*plusdmgptr = (*plusdmgptr) * u.uen / max(u.uenmax,1);
		*truedmgptr = (*truedmgptr) * u.uen / max(u.uenmax,1);
		/* costs some pw */
		if (u.uen >= (u.uenmax*3/10))
			losepw(3);
	}
	/* sickle of thunderblasts damage multiplier is reduced based on your sanity */
	else if (oartifact == ART_SICKLE_OF_THUNDERBLASTS && youagr) {
		*plusdmgptr = (*plusdmgptr) * u.usanity / 100;
		*truedmgptr = (*truedmgptr) * u.usanity / 100;
	}

	/* iconoclast deals 9 bonus damage to angels */
	if (oartifact == ART_ICONOCLAST && is_angel(pd))
		*truedmgptr += 9;

	/* the staff of twelve mirrors is noisy */
	if (oartifact == ART_STAFF_OF_TWELVE_MIRRORS) {
		wake_nearto_noisy(x(mdef), y(mdef), (basedmg)*(basedmg)* 2);
	}

	/* yourshka's spear drains pw */
	if (oartifact == ART_YORSHKA_S_SPEAR){
		if (youdef) losepw(14);
		else mdef->mspec_used = max(mdef->mspec_used + 1, 1);
	}

	/* dirge drains pw */
	if (oartifact == ART_DIRGE){
		if (youdef){
			if(hates_unholy(youracedata) && !hates_holy(youracedata)){
				if(dieroll == 1)
					losepw(666);
				else
					losepw(66);
			}
		}
		else if (hates_unholy_mon(mdef) && !hates_holy_mon(mdef)){
			if(dieroll == 1)
				set_mcan(mdef, TRUE);
			mdef->mspec_used += 6;
		}
		if(youagr){
			*plusdmgptr += u.uimpurity/2;
			*truedmgptr += u.uimp_murder/2;
			if(check_mutation(SHUB_CLAWS) && !resist_slash(pd))
				*plusdmgptr += basedmg;
		}
	}

	/* Liecleaver does NOT double the damage of fired bolts */
	if (oartifact == ART_LIECLEAVER && (otmp != msgr)) {
		/* reduce bonus damage by 1x basedmg to negate previous doubling */
		*plusdmgptr -= basedmg;
	}

	if (oartifact == ART_AVENGER ){
		/* Avenger cancels victims */
		if(dieroll <= 2){
			if (cancel_monst(mdef, otmp, youagr, FALSE, FALSE, 0)){
				if (youagr){
					u.uen += 10;
				}
				else {
					set_mcan(magr, FALSE);
					magr->mspec_used = 0;
				}
			}
		}
		if(youdef ? (u.ualign.type != A_LAWFUL) : (sgn(mdef->data->maligntyp) <= 0)){
			*truedmgptr += d(2, 7);
		}
	}
	//Magic-disrupting focus gems in lightsabers
	// Antimagic rifts can cancel monsters with AD_SPEL magic attacks (or PCs who cast with Int)
	// Catapsi vorticies can cancel monsters with AD_PSON magic attacks (or PCs who cast with Cha)
	if(dieroll <= 2 && is_lightsaber(otmp) && litsaber(otmp) && otmp->cobj){
		if(otmp->cobj->otyp == CATAPSI_VORTEX){
			if(youdef){
				if(base_casting_stat() == A_CHA){
					cancel_monst(mdef, otmp->cobj, youagr, FALSE, FALSE, 0);
				}
			}
			else {
				struct attack * aptr;
				aptr = permonst_dmgtype(mdef->data, AD_PSON);
				if(aptr || has_mind_blast_mon(mdef)){
					cancel_monst(mdef, otmp->cobj, youagr, FALSE, FALSE, 0);
				}
			}
		}
		else if(otmp->cobj->otyp == ANTIMAGIC_RIFT){
			if(youdef){
				if(base_casting_stat() == A_INT){
					cancel_monst(mdef, otmp->cobj, youagr, FALSE, FALSE, 0);
				}
			}
			else {
				struct attack * aptr;
				aptr = permonst_dmgtype(mdef->data, AD_SPEL);
				if(aptr){
					cancel_monst(mdef, otmp->cobj, youagr, FALSE, FALSE, 0);
				}
			}
		}
	}

	/* singing sword -- youagr only */
	if (youagr && oartifact == ART_SINGING_SWORD){
		if (otmp->osinging == OSING_RALLY)
			use_magic_whistle(otmp);
		else if (otmp->osinging == OSING_QUAKE && !rn2(10))
			do_earthquake(mdef->mx, mdef->my, 7, max(abs(otmp->spe), 1), otmp->cursed, (struct monst *)0);
		else if (otmp->osinging == OSING_OPEN && !unsolid(pd) && !skeleton_innards(pd) && !rn2(20)){
			pline("The Singing Sword cuts %s wide open!", mon_nam(mdef));
			if (undiffed_innards(pd)){
				pline("Some of %s insides leak out.", hisherits(mdef));
				*truedmgptr += basedmg + 1;
			}
			else if (removed_innards(pd)) {
				pline("%s organs have already been removed!", HisHerIts(mdef));
				shieldeff(mdef->mx, mdef->my);
			}
			else if (no_innards(pd)) {
				pline("The inside of %s is much like the outside.", mon_nam(mdef));
				shieldeff(mdef->mx, mdef->my);
			}
			else {
				pline("%s vital organs fall out!", HisHerIts(mdef));
				mdef->mhp = 1;
				*truedmgptr += 10;
			}
			findit();//detect secret doors in range
		}
		else if (!mindless_mon(mdef) && !is_deaf(mdef)){
			if (otmp->osinging == OSING_CONFUSE)
				mdef->mconf = 1;
			else if (otmp->osinging == OSING_CANCEL)
				mdef->mspec_used = max(0, mdef->mspec_used + otmp->spe + 1);
			else if (otmp->osinging == OSING_INSANE){
				mdef->mconf = 1;
				mdef->mstun = 1;
				mdef->mberserk = 1;
			}
			else if (otmp->osinging == OSING_FEAR) {
				monflee(mdef, basedmg, TRUE, FALSE);
			}

			//May be interesting here?
			else if (otmp->osinging == OSING_FIRE && !resists_fire(mdef))
				*truedmgptr += basedmg + 1;
			else if (otmp->osinging == OSING_FROST && !resists_cold(mdef))
				*truedmgptr += basedmg + 1;
			else if (otmp->osinging == OSING_ELECT && !resists_elec(mdef))
				*truedmgptr += basedmg + 1;
			else if (otmp->osinging == OSING_DEATH && !rn2(4)){
				if (!resists_death(mdef)){
					if (!(nonliving(mdef->data) || is_demon(pd) || is_angel(pd))){
						pline("%s withers under the touch of %s.", The(Monnam(mdef)), The(xname(otmp)));
						mdef->mhp = 1;
						*truedmgptr += 10;
					}
					else {
						pline("%s seems no deader than before.", The(Monnam(mdef)));
					}
				}
				else {
					pline("%s resists.", the(Monnam(mdef)));
				}
			}
		}
	}
	/* the four basic attacks: fire, cold, shock, and missiles */
	switch (oartifact)
	{
	case ART_LIMB_OF_THE_BLACK_TREE:	wepdesc = "tree-branch";					break;
	case ART_PROFANED_GREATSCYTHE:		wepdesc = "greatscythe";					break;
	case ART_LASH_OF_THE_COLD_WASTE:	wepdesc = "whip";							break;
	/* shock damage seems to like having many different descriptions */
	case ART_CARESS:					wepdesc = "lashing whip";					break;
	case ART_ARYFAERN_KERYM:			wepdesc = "crackling sword-shaped void";	break;
	case ART_RAMIEL:					wepdesc = "thundering polearm";				break;
	case ART_MJOLLNIR:					wepdesc = "massive hammer";					break;
	case ART_IBITE_ARM:
		//Torch effects when the moon is gibbous
		if(otmp->otyp == CLUB)
			wepdesc = "flabby arm";
		else if(phase_of_the_moon() == 3 || phase_of_the_moon() == 5)
			wepdesc = "flame-wielding hand";
		else
			wepdesc = "webbed hand";
	break;

	default:
		/* try to be as vague as possible */
		if (is_axe(msgr))
			wepdesc = "axe";
		else if (msgr->otyp == SET_OF_CROW_TALONS)
			wepdesc = "blades";
		else if (is_blade(msgr))
			wepdesc = "blade";
		else if (msgr->otyp == UNICORN_HORN)
			wepdesc = "horn";
		else if (msgr->otyp == QUARTERSTAFF)
			wepdesc = "staff";
		else if (is_grenade(msgr))
			wepdesc = "grenade";
		else
			wepdesc = OBJ_DESCR(objects[msgr->otyp]) ? OBJ_DESCR(objects[msgr->otyp]) : OBJ_NAME(objects[msgr->otyp]);
		break;
	}
	
	if(check_oprop(otmp, OPROP_SFLMW) && !(youagr && FLAME_BAD) && !youdef && sflm_target(mdef)){
		mdef->mflamemarked = TRUE;
		if(youagr)
			mdef->myoumarked = TRUE;
	}
	else if(goatweaponturn && !youdef && !is_rider(mdef->data)){
		mdef->mgoatmarked = TRUE;
		if(youagr)
			mdef->myoumarked = TRUE;
	}
	if (mercweaponslice[0] || mercweaponslice[1] || mercweaponslice[2]){
		int fire_count = 0;
		int cold_count = 0;
		int shock_count = 0;
		int acid_count = 0;
		for(int i = 0; i < 3; i++){
			switch(mercweaponslice[i]){
				case AD_FIRE:
					fire_count++;
				break;
				case AD_COLD:
					cold_count++;
				break;
				case AD_ELEC:
					shock_count++;
				break;
				case AD_ACID:
					acid_count++;
				break;
			}
		}
		pline_The("%s %s %s %s%c",
			(youagr && Insight > 20 && YOU_MERC_SPECIAL) ? "many-colored" : "paper-thin",
			wepdesc,
			vtense(wepdesc, "hit"),
			hittee, !spec_dbon_applies ? '.' : '!');
		*messaged = TRUE;
		if(fire_count){
			if(!UseInvFire_res(mdef)){
				if (!rn2(6-fire_count)) destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
				if (!rn2(6-fire_count)) destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
				if (!rn2(6-fire_count)) destroy_item(mdef, POTION_CLASS, AD_FIRE);
			}
		}
		if(cold_count){
			if (!UseInvCold_res(mdef)){
				if (!rn2(5-cold_count)) destroy_item(mdef, POTION_CLASS, AD_COLD);
			}
		}
		if(shock_count){
			if (!UseInvShock_res(mdef)){
				if (!rn2(5-shock_count)) destroy_item(mdef, WAND_CLASS, AD_ELEC);
				if (!rn2(5-shock_count)) destroy_item(mdef, RING_CLASS, AD_ELEC);
			}
		}
		if(acid_count){
			if(!UseInvAcid_res(mdef)){
				if (rn2(5-acid_count)) destroy_item(mdef, POTION_CLASS, AD_FIRE);
			}
		}
	}
	else if(is_chained_merc(otmp)){
		pline_The("%s %s %s %s%c",
			(youagr && Insight > 20 && YOU_MERC_SPECIAL) ? "dense" : "shimmering",
			wepdesc,
			vtense(wepdesc, "hit"),
			hittee, !spec_dbon_applies ? '.' : '!');
		*messaged = TRUE;
	}
	else if(is_kinstealing_merc(otmp)){
		pline_The("%s %s %s %s%c",
			(youagr && Insight > 20 && YOU_MERC_SPECIAL) ? "grasping" : "jagged",
			wepdesc,
			vtense(wepdesc, "hit"),
			hittee, !spec_dbon_applies ? '.' : '!');
		*messaged = TRUE;
	}
	if (attacks(AD_FIRE, otmp)
		|| check_oprop(otmp,OPROP_FIREW)
		|| check_oprop(otmp,OPROP_OONA_FIREW)
		|| check_oprop(otmp,OPROP_LESSER_FIREW)
		|| goatweaponturn == AD_FIRE
		|| sothweaponturn == AD_FIRE
	){
		if (attacks(AD_FIRE, otmp) && (vis&VIS_MAGR) && printmessages) {	/* only artifacts message */
			pline_The("fiery %s %s %s%c",
				wepdesc,
				vtense(wepdesc,
					!spec_dbon_applies ? "hit" :
					is_watery(pd) ? "partly vaporize" :
					"burn"),
				hittee, !spec_dbon_applies ? '.' : '!');
			*messaged = TRUE;
		}
		if(oartifact != ART_PROFANED_GREATSCYTHE && !UseInvFire_res(mdef)){
			if (!rn2(4)) (void) destroy_item(mdef, POTION_CLASS, AD_FIRE);
			if (!rn2(4)) (void) destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
			if (!rn2(7)) (void) destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
		}
	    if (youdef) burn_away_slime();
	    if (youdef && FrozenAir) melt_frozen_air();
	}
	if(oartifact == ART_ARYVELAHR_KERYM &&
		(is_orc(pd) || is_demon(pd) || is_drow(pd) || is_undead(pd))
	){
		/*Note: magic blue flames, damage through fire res but still check invent fire res to see if inventory should burn*/
		if(printmessages){
			pline_The("blue-flamed %s %s %s!",
				wepdesc,
				vtense(wepdesc,
					is_watery(pd) ? "partly vaporize" :
					"burn"),
				hittee);
			*messaged = TRUE;
		}
		*truedmgptr += d(2, 10);
		if(!UseInvFire_res(mdef)){
			if (!rn2(4)) (void) destroy_item(mdef, POTION_CLASS, AD_FIRE);
			if (!rn2(4)) (void) destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
			if (!rn2(7)) (void) destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
		}
	    if (youdef) burn_away_slime();
	    if (youdef && FrozenAir) melt_frozen_air();
	}
	if(check_oprop(otmp, OPROP_ELFLW)){
		static boolean suddenly = TRUE;
		if(Insight >= 56 && 
			(yellow_monster(mdef) || mdef->mfaction == YELLOW_FACTION)
		){
			/*Note: magic green flames, damage through fire res but still check invent fire res to see if inventory should burn*/
			if(printmessages){
				pline_The("%schartreuse-flamed %s %s %s!",
					suddenly ? "suddenly-" : "",
					wepdesc,
					vtense(wepdesc,
						is_watery(pd) ? "partly vaporize" :
						"burn"),
					hittee);
				*messaged = TRUE;
				suddenly = FALSE;
			}
			*truedmgptr += d(2, 8) + otmp->spe;
			if(!UseInvFire_res(mdef)){
				if (!rn2(4)) (void) destroy_item(mdef, POTION_CLASS, AD_FIRE);
				if (!rn2(4)) (void) destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
				if (!rn2(7)) (void) destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
			}
			if (youdef) burn_away_slime();
			if (youdef && FrozenAir) melt_frozen_air();
		}
		else suddenly = TRUE;
	}
	if ((attacks(AD_COLD, otmp)  && !(
			/* exceptions */
			(oartifact && arti_struct->inv_prop == ICE_SHIKAI && artinstance[otmp->oartifact].SnSd3duration < monstermoves)
		)) 
		|| check_oprop(otmp,OPROP_COLDW)
		|| check_oprop(otmp,OPROP_OONA_COLDW) 
		|| check_oprop(otmp,OPROP_LESSER_COLDW) 
		|| goatweaponturn == AD_COLD
	){
		if (attacks(AD_COLD, otmp) && (vis&VIS_MAGR) && printmessages) {
			pline_The("ice-cold %s %s %s%c",
				wepdesc,
				vtense(wepdesc,
					!spec_dbon_applies ? "hit" :
					"freeze"),
				hittee, !spec_dbon_applies ? '.' : '!');
			*messaged = TRUE;
		}
		if (!UseInvCold_res(mdef)) {
	    	if (!rn2(4)) (void) destroy_item(mdef, POTION_CLASS, AD_COLD);
		}
		if(otmp->oartifact == ART_FROST_BRAND && spec_dbon_applies)
			mdef->movement = max(youdef ? 6 : -12, mdef->movement-2);
	}
	if (oartifact == ART_SHADOWLOCK) {
		if (!Cold_res(mdef)) {
			if (vis&VIS_MAGR && printmessages) {
				pline_The("ice-cold blade-shadow freezes %s!", hittee);
				*messaged = TRUE;
			}
			*truedmgptr += d(2, 6) + otmp->spe;
			if (!UseInvCold_res(mdef)) {
				if (!rn2(4)) (void) destroy_item(mdef, POTION_CLASS, AD_COLD);
			}
		}
	}
	if (attacks(AD_ELEC, otmp)
		|| check_oprop(otmp,OPROP_ELECW)
		|| check_oprop(otmp,OPROP_OONA_ELECW)
		|| check_oprop(otmp,OPROP_LESSER_ELECW)
		|| goatweaponturn == AD_ELEC
	){
		if (attacks(AD_ELEC, otmp) && (vis&VIS_MAGR) && printmessages) {
			pline_The("%s %s %s%c",
				wepdesc,
				vtense(wepdesc,
					!spec_dbon_applies ? "hit" :
					"shock"),
				hittee, !spec_dbon_applies ? '.' : '!');
			*messaged = TRUE;
		}
		if (!UseInvShock_res(mdef)) {
			if (!rn2(5)) (void) destroy_item(mdef, RING_CLASS, AD_ELEC);
			if (!rn2(5)) (void) destroy_item(mdef, WAND_CLASS, AD_ELEC);
		}
	}
	if (attacks(AD_ACID, otmp)
		|| check_oprop(otmp,OPROP_ACIDW)
		|| check_oprop(otmp,OPROP_LESSER_ACIDW)
		|| goatweaponturn == AD_EACD
		|| goatweaponturn == AD_ACID
	){
		if (attacks(AD_ACID, otmp) && (vis&VIS_MAGR) && printmessages) {
			pline_The("foul %s %s %s%c",
				wepdesc,
				vtense(wepdesc,
					!spec_dbon_applies ? "hit" :
					"burn"),
				hittee, !spec_dbon_applies ? '.' : '!');
			*messaged = TRUE;
		}
		if (!UseInvAcid_res(mdef)) {
	    	if (!rn2(2)) (void) destroy_item(mdef, POTION_CLASS, AD_FIRE);
//	    	if (!rn2(4)) (void) destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
//	    	if (!rn2(7)) (void) destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
		}
	}
	if (attacks(AD_SLEE, otmp)){
		if (attacks(AD_SLEE, otmp) && (vis&VIS_MAGR) && printmessages) {
			pline_The("soporific %s %s %s%c",
				wepdesc,
				vtense(wepdesc, "hit"),
				hittee, !spec_dbon_applies ? '.' : '!');
			*messaged = TRUE;
		}
	}
	if (attacks(AD_STAR, otmp)){
		if (attacks(AD_STAR, otmp) && (vis&VIS_MAGR) && printmessages) {
			pline_The("starlit %s %s %s%c",
				wepdesc,
				vtense(wepdesc,
					!spec_dbon_applies ? "hit" :
					"sear"),
				hittee, !spec_dbon_applies ? '.' : '!');
			*messaged = TRUE;
		}
	}
	if (attacks(AD_HLUH, otmp)){
		if (attacks(AD_HLUH, otmp) && (vis&VIS_MAGR) && printmessages) {
			pline_The("corrupted %s %s %s%c",
				wepdesc,
				vtense(wepdesc,
					!spec_dbon_applies ? "hit" :
					"sear"),
				hittee, !spec_dbon_applies ? '.' : '!');
			*messaged = TRUE;
		}
	}
	if (attacks(AD_HOLY, otmp)){
		if (attacks(AD_HOLY, otmp) && (vis&VIS_MAGR) && printmessages) {
			pline_The("holy %s %s %s%c",
				wepdesc,
				vtense(wepdesc,
					!spec_dbon_applies ? "hit" :
					"sear"),
				hittee, !spec_dbon_applies ? '.' : '!');
			*messaged = TRUE;
		}
	}
	if (attacks(AD_BLUD, otmp)){
		if (vis&VIS_MAGR && printmessages) {
			pline_The("bloodstained %s %s %s%c",
				wepdesc,
				vtense(wepdesc, "hit"),
				hittee, !spec_dbon_applies ? '.' : '!');
			*messaged = TRUE;
		}
		if (spec_dbon_applies && !(otmp->oartifact == ART_BLOODLETTER && artinstance[otmp->oartifact].BLactive < monstermoves)){
			*truedmgptr += mlev(mdef);
			if(youagr)
				*plusdmgptr += u.uimpurity/2;
			if (otmp->oartifact == ART_BLOODLETTER)
				artinstance[otmp->oartifact].BLactive += max(0, mlev(mdef)/10 + rn2(5));
		}
	}
	if (arti_attack_prop(otmp, ARTA_MAGIC) && dieroll <= MB_MAX_DIEROLL) {
		int dmg = basedmg;
	    /* Magicbane's special attacks (possibly modifies hittee[]) */
		*messaged = Mb_hit(magr, mdef, otmp, &dmg, dieroll, vis, hittee,
			oartifact == ART_STAFF_OF_WILD_MAGIC ? "staff" : "blade");
		dmg -= min(dmg, basedmg);	/* take back the basedmg that was passed to Mb_hit() */
		/* assumes all bonus damage from Mb_hit should ignore resistances */
		*truedmgptr += dmg;
	}
	if(oartifact == ART_PEN_OF_THE_VOID){
		int dmg = basedmg;
		*messaged = voidPen_hit(magr, mdef, otmp, &dmg, dieroll, vis, hittee);
		dmg -= min(dmg, basedmg);	/* take back the basedmg that was passed to voidPen_hit() */
		/* assumes all bonus damage from voidPen_hit should ignore resistances */
		*truedmgptr += dmg;
	}
//	pline("D20 role was %d", dieroll);
//	pline("%d", otmp->oeaten);
	if (oartifact == ART_ROD_OF_SEVEN_PARTS ) {
		if(--artinstance[ART_ROD_OF_SEVEN_PARTS].RoSPkills < 1){
			if(youagr){
				if(otmp->spe < 7 && u.ulevel/3 > otmp->spe){
					otmp->spe++;
					pline("Your weapon has become more perfect!");
					artinstance[ART_ROD_OF_SEVEN_PARTS].RoSPkills = 7;
				} else artinstance[ART_ROD_OF_SEVEN_PARTS].RoSPkills=1;
			} else {
				if(otmp->spe < 7 && (magr->m_lev)/3 > otmp->spe){
					otmp->spe++;
					artinstance[ART_ROD_OF_SEVEN_PARTS].RoSPkills = 7;
				} else artinstance[ART_ROD_OF_SEVEN_PARTS].RoSPkills=1;
			}
		}
		/* bonus 7+enchant damage to chaotics */
		if(sgn(pd->maligntyp) < 0){
			*truedmgptr += 7 + otmp->spe;
		}
	}
		//sunlight code adapted from Sporkhack
	if ((pd->mtyp == PM_GREMLIN || pd->mtyp == PM_HUNTING_HORROR)
		&& (arti_bright(otmp) || (check_oprop(otmp, OPROP_ELFLW) && Insight >= 33 && u.ualign.record > 3))
	){
		wepdesc = oartifact ? artilist[oartifact].name : simple_typename(otmp->otyp);
		/* Sunlight kills gremlins */
		if (vis&VIS_MAGR && vis&VIS_MDEF) {
			pline("%s sunlight sears %s!", s_suffix(wepdesc), hittee);
			otmp->dknown = TRUE;
			*messaged = TRUE;
		}
		if (!youdef)
			return xdamagey(magr, mdef, (struct attack *)0, *hp(mdef));
		else
		{
			losehp(*hp(&youmonst) + 1, "bright light", KILLED_BY);
			return MM_DEF_LSVD;
		}
	}
	if ((pd->mlet == S_TROLL)
		&& (arti_bright(otmp) || (check_oprop(otmp, OPROP_ELFLW) && Insight >= 33 && u.ualign.record > 3))
	){
		wepdesc = oartifact ? artilist[oartifact].name : simple_typename(otmp->otyp);
		/* Sunlight turns trolls to stone (Middle-earth) */
		if (pd->mlet == S_TROLL &&
			!munstone(mdef, TRUE)) {
			if (vis&VIS_MAGR && vis&VIS_MDEF){
				pline("%s sunlight strikes %s!", s_suffix(wepdesc), hittee);
				if (Stone_res(mdef)) pline("But %s %s even slow down!", hittee, youdef ? "don't" : "doesn't");
				otmp->dknown = TRUE;
				*messaged = TRUE;
			}
			if (youdef)
				instapetrify(wepdesc);
			else
				minstapetrify(mdef, youagr);
			if (!Stone_res(mdef)) {
				if (*hp(mdef) > 0)
					return MM_DEF_LSVD;
				else
					return MM_DEF_DIED;
			}
		}
	}
	if (youagr && is_covetous(pd) && oartifact == ART_TROLLSBANE)
	{
		if (Hallucination) You("flame the nasty troll!");  //trollsbane hits monsters that pop in to ruin your day.
		*truedmgptr += d(2, 20) + 2 * otmp->spe; //boosts power better than demonbane hitting silver hating.
	}
	
	if (youagr && is_alien(pd) && oartifact == ART_FALLINGSTAR_MANDIBLES)
	{
		*truedmgptr += basedmg;
	}
	
	
	if (arti_attack_prop(otmp, ARTA_BLIND) && !resists_blnd(mdef) && !rn2(3)) {
		long rnd_tmp;
		wepdesc = "brilliant light";
		if ((vis&VIS_MAGR && vis&VIS_MDEF) && mdef->mcansee)
			pline_The("%s blinds %s!", wepdesc, hittee);
		rnd_tmp = rn2(5) + otmp->spe;
		if (rnd_tmp<2) rnd_tmp = 2;
		if (youdef) {
			make_blinded(Blinded + rnd_tmp, FALSE);
			if (!Blind) Your1(vision_clears);
		}
		else {
			if ((rnd_tmp += mdef->mblinded) > 127) rnd_tmp = 127;
			mdef->mblinded = rnd_tmp;
			mdef->mcansee = 0;
			mdef->mstrategy &= ~STRAT_WAITFORU;
		}
	}
	if(oartifact == ART_MASAMUNE){
		int experts = -2;
		if(youagr){
			for(int skl = 1; skl < P_NUM_SKILLS; skl++){
				if(P_SKILL(skl) >= P_EXPERT)
					experts += 2;
			}
		}
		else if(m_martial_skill(magr->data) == P_EXPERT && !magr->mformication && !magr->mscorpions && !magr->mcaterpillars){
			experts = 10;
		}
		/* Masamune rewards skill */
		if(experts > 0)
			*plusdmgptr += experts;
		/* Masamune can crit (this purposefully bypasses dr etc.) */
		if(dieroll <= 4)
			*truedmgptr += basedmg + rnd(20);
	}
	/* The anti-elemental sword */
	if (oartifact == ART_KUSANAGI_NO_TSURUGI){
		/* special effects vs whirly */
		if (is_whirly(pd)){
			wepdesc = "The winds";
			if (youagr && u.uswallow && mdef == u.ustuck) {
				pline("%s blow %s open!  It dissipates in the breeze.", wepdesc, mon_nam(mdef));
				*messaged = TRUE;
				return xdamagey(magr, mdef, (struct attack *)0, *hp(mdef));
			}
			else if (!youdef) {
				pline("%s blow %s apart!", wepdesc, mon_nam(mdef));
				otmp->dknown = TRUE;
				*messaged = TRUE;
				return xdamagey(magr, mdef, (struct attack *)0, *hp(mdef));
			}
			else {
				losehp(*hp(&youmonst) + 1, "gusting winds", KILLED_BY);
				return MM_DEF_LSVD;
			}
		}
		/* special effects vs flaming */
		else if (flaming(pd)) {
			struct monst *mtmp = 0;
			int result = 0;
			pline("The winds fan the flames into a roaring inferno!");

			if (!youdef) {
				result = xdamagey(magr, mdef, (struct attack *)0, *hp(mdef));
			}
			else {
				losehp(*hp(&youmonst) + 1, "roaring winds", KILLED_BY);
				result = MM_DEF_LSVD;
			}	

			if (!(result&MM_DEF_LSVD)){
				otmp->dknown = TRUE;
				if (youagr || magr->mtame){
					/* create a tame fire vortex */
					mtmp = makemon(&mons[PM_FIRE_VORTEX], x(magr), y(magr), MM_EDOG | MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
					if (mtmp){
						initedog(mtmp);
						mtmp->m_lev = u.ulevel / 2 + 1;
						mtmp->mhpmax = (mtmp->m_lev * hd_size(mtmp->data)) - hd_size(mtmp->data)/2;
						mtmp->mhp = mtmp->mhpmax;
						mtmp->mtame = 10;
						mtmp->mpeaceful = 1;
					}
				}
				else {
					/* create a fire vortex */
					mtmp = makemon(&mons[PM_FIRE_VORTEX], x(magr), y(magr), MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
					if (mtmp){
						mtmp->mpeaceful = magr->mpeaceful;
					}
				}
			}
			*messaged = TRUE;
			return result;
		}
		/* special effects vs earth elementals */
		else if (pd->mtyp == PM_EARTH_ELEMENTAL){
			struct monst *mtmp = 0;
			int result = 0;
			pline("The winds blast the stone and sweep the fragments into a whirling dust storm!");

			if (!youdef) {
				result = xdamagey(magr, mdef, (struct attack *)0, *hp(mdef));
			}
			else {
				losehp(*hp(&youmonst) + 1, "blasting winds", KILLED_BY);
				result = MM_DEF_LSVD;
			}

			if (!(result&MM_DEF_LSVD)) {
				otmp->dknown = TRUE;
				if (youagr || magr->mtame){
					/* create a tame dust vortex */
					mtmp = makemon(&mons[PM_DUST_VORTEX], x(magr), y(magr), MM_EDOG | MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
					if (mtmp){
						initedog(mtmp);
						mtmp->m_lev = u.ulevel / 2 + 1;
						mtmp->mhpmax = (mtmp->m_lev * hd_size(mtmp->data)) - hd_size(mtmp->data)/2;
						mtmp->mhp = mtmp->mhpmax;
						mtmp->mtame = 10;
						mtmp->mpeaceful = 1;
					}
				}
				else {
					/* create a dust vortex */
					mtmp = makemon(&mons[PM_DUST_VORTEX], x(magr), y(magr), MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
					if (mtmp){
						mtmp->mpeaceful = magr->mpeaceful;
					}
				}
			}
			*messaged = TRUE;
			return result;
		}
		/* special effects vs water elementals */
		else if (pd->mtyp == PM_WATER_ELEMENTAL) {
			struct monst *mtmp = 0;
			int result = 0;
			pline("The winds whip the waters into a rolling fog!");

			if (!youdef) {
				result = xdamagey(magr, mdef, (struct attack *)0, *hp(mdef));
			}
			else {
				losehp(*hp(&youmonst) + 1, "whipping winds", KILLED_BY);
				result = MM_DEF_LSVD;
			}

			if (!(result&MM_DEF_LSVD)) {
				otmp->dknown = TRUE;
				if (youagr || magr->mtame){
					/* create a tame fog cloud */
					mtmp = makemon(&mons[PM_FOG_CLOUD], x(magr), y(magr), MM_EDOG | MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
					if (mtmp){
						initedog(mtmp);
						mtmp->m_lev = u.ulevel / 2 + 1;
						mtmp->mhpmax = (mtmp->m_lev * hd_size(mtmp->data)) - hd_size(mtmp->data)/2;
						mtmp->mhp = mtmp->mhpmax;
						mtmp->mtame = 10;
						mtmp->mpeaceful = 1;
					}
				}
				else {
					/* create a fog cloud */
					mtmp = makemon(&mons[PM_FOG_CLOUD], x(magr), y(magr), MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
					if (mtmp){
						mtmp->mpeaceful = magr->mpeaceful;
					}
				}
			}
			*messaged = TRUE;
			return result;
		}
	}
	if (arti_steal(otmp) && magr != mdef && magr && mdef){
		if (youagr){
			if (mdef->minvent && (Role_if(PM_PIRATE) || !rn2(10))){
				struct obj *otmp2;
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
					/*stealing is impure*/
					IMPURITY_UP(u.uimp_theft)
					/* take the object away from the monster */
					if (otmp2->quan > 1L){
						otmp2 = splitobj(otmp2, 1L);
						obj_extract_self(otmp2); //wornmask is cleared by splitobj
					}
					else {
						obj_extract_self(otmp2);
						if ((unwornmask = otmp2->owornmask) != 0L) {
							mdef->misc_worn_check &= ~unwornmask;
							if (otmp2->owornmask & W_WEP) {
								setmnotwielded(mdef, otmp2);
								MON_NOWEP(mdef);
							}
							if (otmp2->owornmask & W_SWAPWEP) {
								setmnotwielded(mdef,otmp2);
								MON_NOSWEP(mdef);
							}
							otmp2->owornmask = 0L;
							update_mon_intrinsics(mdef, otmp2, FALSE, FALSE);
						}
					}
					/* Ask the player if they want to keep the object */
					if(otmp->oartifact == ART_REAVER)
						pline("Reaver sweeps %s away from %s.", doname(otmp2), mon_nam(mdef));
					else
						pline("Your blade sweeps %s away from %s.", doname(otmp2), mon_nam(mdef));
					if (otmp->ovara_artiTheftType == 0 && yn("Do you try to grab it for yourself?") == 'y'){
						/* give the object to the character */
						otmp2 = Role_if(PM_PIRATE) ?
							hold_another_object(otmp2, "Ye snatched but dropped %s.",
							doname(otmp2), "Ye steal: ") :
							hold_another_object(otmp2, "You snatched but dropped %s.",
							doname(otmp2), "You steal: ");
						if (otmp2->otyp == CORPSE &&
							touch_petrifies(&mons[otmp2->corpsenm]) && !uarmg) {
							char kbuf[BUFSZ];

							Sprintf(kbuf, "stolen %s corpse", mons[otmp2->corpsenm].mname);
							instapetrify(kbuf);
						}
					}
					else{
						if (otmp->ovara_artiTheftType == 0){
							getdir((char *)0);
							if (u.dx || u.dy){
								You("toss it away.");
								projectile(&youmonst, otmp2, (void *)0, HMON_PROJECTILE, u.ux, u.uy, u.dx, u.dy, u.dz, (int)((ACURRSTR) / 2 - otmp2->owt / 40), FALSE, TRUE, FALSE);
							}
							else{
								You("drop it at your feet.");
								(void)dropy(otmp2);
							}
							if (mdef->mhp <= 0) /* flung weapon killed monster */
								return MM_DEF_DIED;
						}
						else {
							(void)dropy(otmp2);
							(void)pickup(2);
						}
					}
					/* more take-away handling, after theft message */
					if (unwornmask & W_WEP) {		/* stole wielded weapon */
						possibly_unwield(mdef, FALSE);
					}
					else if (unwornmask & W_ARMG) {	/* stole worn gloves */
						mselftouch(mdef, (const char *)0, TRUE);
						if (mdef->mhp <= 0)	/* it's now a statue */
							return MM_DEF_DIED; /* monster is dead */
					}
				}
			}
		}
		else if (youdef && !(u.sealsActive&SEAL_ANDROMALIUS) && !check_mutation(TENDRIL_HAIR)){
			char buf[BUFSZ];
			buf[0] = '\0';
			steal(magr, buf, TRUE, FALSE);
		}
		else{
			struct obj *obj;
			/* find an object to steal, non-cursed if magr is tame */
			for (obj = mdef->minvent; obj; obj = obj->nobj)
			if (!magr->mtame || !obj->cursed)
				break;

			if (obj) {
				char buf[BUFSZ], onambuf[BUFSZ], mdefnambuf[BUFSZ];

				/* make a special x_monnam() call that never omits
				   the saddle, and save it for later messages */
				Strcpy(mdefnambuf, x_monnam(mdef, ARTICLE_THE, (char *)0, 0, FALSE));
#ifdef STEED
				if (u.usteed == mdef &&
					obj == which_armor(mdef, W_SADDLE))
					/* "You can no longer ride <steed>." */
					dismount_steed(DISMOUNT_POLY);
#endif
				obj_extract_self(obj);
				if (obj->owornmask) {
					mdef->misc_worn_check &= ~obj->owornmask;
					if (obj->owornmask & W_WEP)
						setmnotwielded(mdef, obj);
					obj->owornmask = 0L;
					update_mon_intrinsics(mdef, obj, FALSE, FALSE);
				}
				/* add_to_minv() might free obj [if it merges] */
				if (vis)
					Strcpy(onambuf, doname(obj));
				(void)add_to_minv(magr, obj);
				if (vis) {
					Strcpy(buf, Monnam(magr));
					pline("%s steals %s from %s!", buf,
						onambuf, mdefnambuf);
				}
				possibly_unwield(mdef, FALSE);
				mdef->mstrategy &= ~STRAT_WAITFORU;
				mselftouch(mdef, (const char *)0, FALSE);
				if (mdef->mhp <= 0)
					return MM_DEF_DIED;
			}
		}
	}
	/* giantslayer slows giants */
	if (oartifact == ART_GIANTSLAYER && (throws_rocks(pd) || is_giant(pd)))
	{
		mdef->movement -= NORMAL_SPEED / 2;
		if (vis&VIS_MDEF) {
			if (!youdef) pline("The axe hamstrings %s!", mon_nam(mdef));
			else pline("The hateful axe hamstrings you!");
			*messaged = TRUE;
		}
	}

	/* the Tecpatl of Huhetotl can sacrifice low-enough-health monsters (not you, though!) */
	if (oartifact == ART_TECPATL_OF_HUHETOTL){
		if (!youdef) {
			if (has_blood_mon(mdef) && !noncorporeal(pd)) {
				if (vis & VIS_MDEF) {
					*messaged = TRUE;
					pline("The sacrificial blade drinks the blood of %s!", mon_nam(mdef));
				}
#define MAXVALUE 24
				/* if this bonus damage would be the edge when killing, a sacrifice to the gods! */
				int sacrifice_dmg = d(2, 4);
				if ((mdef->mhp > currdmg)
					&& (*truedmgptr += sacrifice_dmg)
					&& (mdef->mhp <= currdmg)
					)
				{
					killed(mdef);
					if (DEADMONSTER(mdef)) {	//else lifesaved
						/* only the player gets benefits from sacrificing */
						if (youagr)
						{
							extern const int monstr[];
							int value = min(monstr[monsndx(pd)] + 1, MAXVALUE);
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
						}
						return MM_DEF_DIED;
					}
					else
						return MM_DEF_LSVD;
				}

			}
		}
	} 
	/* while Plague is invoked, lethal-filth arrows cause victims to virulently explode.
	 * Not you, though. You just die. It's simpler that way. */
	if (oartifact == ART_PLAGUE && artinstance[ART_PLAGUE].PlagueDoOnHit && (*hp(mdef) <= currdmg + 100) && !youdef) {
		artinstance[ART_PLAGUE].PlagueDoOnHit = FALSE;
		int mx = x(mdef), my = y(mdef);
		if (vis&VIS_MAGR && vis&VIS_MDEF) {
			pline_The("tainted %s strikes true!", xname(msgr));
		}
		if (vis&VIS_MDEF) {
			pline("%s %s bubbles, and %s explodes!",
				s_suffix(Monnam(mdef)),
				mbodypart(mdef, BODY_SKIN),
				mon_nam(mdef)
				);
			*messaged = TRUE;
		}
		/* we want to avoid catching mdef in this explosion -- kludge time */
		/* note: long worms still get caught in the explosion, because of course they do, so don't kludge at all to be on the safe side */
		if (!is_longworm(mdef->data)) {
			level.monsters[mx][my] = (struct monst *)0;
		}

		killer = "virulent explosion";
		explode(mx, my, AD_DISE, MON_EXPLODE, d(6, 6), EXPL_NOXIOUS,
			((mdef->data->msize + 3) / 4));	/* tiny -> R0; gigantic -> R2; others -> R1 */

		if (!is_longworm(mdef->data)) {
			level.monsters[mx][my] = mdef;
		}

		return xdamagey(magr, mdef, (struct attack *)0, *hp(mdef)); /* instakill */
	}

	/* vorpal weapons */
	if (arti_attack_prop(otmp, ARTA_VORPAL) || check_oprop(otmp,OPROP_VORPW)
		|| (otmp->otyp == STAKE && is_vampire(mdef->data) && !naoid(mdef->data))
	) {
		char buf[BUFSZ];
		int vorpaldamage = (basedmg * 20) + d(8, 20);
		int method = 0;
#define VORPAL_BEHEAD	1
#define VORPAL_BISECT	2
#define VORPAL_PIERCE	3
#define VORPAL_SMASH	4
#define VORPAL_IGNITE	5
#define VORPAL_SHEAR	6

		switch (oartifact) {
		case ART_TSURUGI_OF_MURAMASA:
			wepdesc = "razor-sharp blade";
			if (dieroll == 1)
				method = VORPAL_BISECT;
			break;
		case ART_KUSANAGI_NO_TSURUGI:
			wepdesc = "razor-sharp blade";
			vorpaldamage *= 2;	/* very very lethal */
			if (dieroll <= 2) {
				if (bigmonst(pd) && has_head_mon(mdef) && !(youagr && u.uswallow)) {
					if (noncorporeal(pd) || amorphous(pd)) {
						method = VORPAL_SHEAR;
						wepdesc = "shearing wind";
					}
					else
						method = VORPAL_BEHEAD;
				}
				else
					method = VORPAL_BISECT;
			}
			break;
		case ART_LIFEHUNT_SCYTHE:
			wepdesc = "neck-seeking scythe";
			if (dieroll == 1) {
				if (has_head_mon(mdef) && lifehunt_sneak_attacking && !(youagr && u.uswallow)
					&& magr && mdef && (distmin(x(magr), y(magr), x(mdef), y(mdef)) <= 1))
				{
					method = VORPAL_BEHEAD;
					lifehunt_sneak_attacking = FALSE;	/* max once per attack action */
				}
			}
			break;
		case ART_RELEASE_FROM_CARE:
			wepdesc = "heedless scythe";
			if (dieroll <= 2) {
				if ((has_head_mon(mdef) || !bigmonst(pd)) && !(youagr && u.uswallow)
					&& !(youagr && u.uswallow && mdef == u.ustuck))
				{
					method = VORPAL_BEHEAD;
				}
				else
					method = VORPAL_BISECT;
			}
			break;
		case ART_ARROW_OF_SLAYING:
			wepdesc = "heart-seeking arrow";
			if (dieroll == 1) {
				method = VORPAL_PIERCE;
			}
			break;
		case ART_VORPAL_BLADE:
			wepdesc = "vorpal blade";
			if (dieroll == 1 || pd->mtyp == PM_JABBERWOCK) {
				method = VORPAL_BEHEAD;
				if (pd->mtyp == PM_JABBERWOCK)
					vorpaldamage *= 2;
			}
			break;
		case ART_CRESCENT_BLADE:
			wepdesc = "burning blade";
			if (dieroll == 1) {
				method = VORPAL_BEHEAD;
			}
			break;
		case ART_OGRESMASHER:
			wepdesc = "vengeful hammer";
			if (pd->mlet == S_OGRE) {
				if (youagr){
					exercise(A_STR, TRUE);
					exercise(A_STR, TRUE);
					exercise(A_WIS, TRUE);
				}
				method = VORPAL_SMASH;
			}
			break;
		case ART_TORCH_OF_ORIGINS:
			wepdesc = "ancient inferno";
			vorpaldamage = 3000;
			if (dieroll <= 2) {
				if (!Fire_res(mdef))
					method = VORPAL_IGNITE;
			}
			break;
		default:
			/* hopefully it's a vorpal-property weapon at this point */
			if (check_oprop(otmp, OPROP_VORPW))
				Sprintf(buf, "vorpal %s", simple_typename(msgr->otyp));
			else
				Strcpy(buf, simple_typename(msgr->otyp));
			wepdesc = buf;
			if (dieroll == 1)
			{
				if(check_oprop(otmp,OPROP_GSSDW) && has_head_mon(mdef))
					method = VORPAL_BEHEAD;
				else if (otmp->otyp == STAKE && is_vampire(mdef->data) && !naoid(mdef->data))
					method = VORPAL_PIERCE;
				else if (is_slashing(msgr) && is_stabbing(msgr))
					method = VORPAL_BEHEAD;
				else if (is_slashing(msgr))
					method = VORPAL_BISECT;
				else if (is_stabbing(msgr))
					method = VORPAL_PIERCE;
				else if (is_bludgeon(msgr))
					method = VORPAL_SMASH;
			}
			break;
		}

		/* check misses */
		if ((method == VORPAL_BEHEAD) && (!has_head_mon(mdef) || notonhead || (youagr && u.uswallow && mdef == u.ustuck)))
		{
			if (youagr)
				pline("Somehow, you miss %s wildly.", mon_nam(mdef));
			else if (vis)
				pline("Somehow, %s misses %swildly.", mon_nam(magr), (youdef ? "you " : ""));
			*messaged = ((boolean)(youagr || vis));
			return MM_MISS;	/* miss */
		}
		/* check engulfer instakills */
		else if ((youagr && u.uswallow && mdef == u.ustuck) &&
			((method == VORPAL_BISECT) || (method == VORPAL_SMASH)))
		{
			if(oartifact == ART_RELEASE_FROM_CARE && !youdef)
				set_mcan(mdef, TRUE);
			You("%s %s wide open!", 
				((method == VORPAL_BISECT) ? "slice" : "smash"),
				mon_nam(mdef));
			*messaged = TRUE;
			return xdamagey(magr, mdef, (struct attack *)0, *hp(mdef));	/* instakill */
		}
		/* normal vorpal */
		else if (method != 0 && !check_res_engine(mdef, AD_VORP)) {
			/* find defender's applicable armor */
			struct obj * armor = (struct obj *)0;
			switch (method) {
			case VORPAL_BEHEAD:
				armor = youdef ? uarmh : which_armor(mdef, W_ARMH);
				break;
			case VORPAL_BISECT:
			case VORPAL_PIERCE:
				armor = youdef ? uarm  : which_armor(mdef, W_ARM);
				if(armor && !arm_blocks_upper_body(armor->otyp))
					armor = 0;
				break;
			case VORPAL_SMASH:
				armor = youdef ? uarms : which_armor(mdef, W_ARMS);
				break;
			}
			if (arti_phasing(otmp))
				armor = (struct obj *)0;

			/* damage, destroy armor */
			while (armor && vorpaldamage > basedmg * 4) {
				if (armor->spe > -1 * objects[(armor)->otyp].a_ac){
					damage_item(armor);
					/* artifacts are too much more resistant to being damaged than standard items (90% vs 10%) */
					/* so they need adjusting here to only be somewhat better */
					vorpaldamage -= (armor->spe + objects[(armor)->otyp].a_ac)
						* ((armor->oartifact) ? 3 : 18) / 2;
				}
				else {
					claws_destroy_marm(mdef, armor);
					armor = (struct obj *)0;
					break;	/* exit armor-killing loop */
				}
			}
			/* apply other damage modifiers */
			if (method == VORPAL_BEHEAD && (noncorporeal(pd) || amorphous(pd)))
				vorpaldamage = 0;
			if ((method == VORPAL_BISECT) && (bigmonst(pd) || notonhead))
				vorpaldamage = basedmg;
			if ((method == VORPAL_PIERCE) && (!has_blood_mon(mdef) || !(pd->mflagsb&MB_BODYTYPEMASK) || noncorporeal(pd) || amorphous(pd)))
				vorpaldamage = basedmg;

			/* Are we sufficiently lethal for a vorpal kill? */
			if ((vorpaldamage + basedmg > *hp(mdef)) && vorpaldamage >= basedmg*4) {
				if (vis)
				{
					if(oartifact == ART_RELEASE_FROM_CARE && !youdef)
						set_mcan(mdef, TRUE);
					/* print message */
					switch (method) {
					case VORPAL_BEHEAD:
						pline("%s %s %s!",
							The(wepdesc),
							(!rn2(2) ? "beheads" : "decapitates"),
							hittee);
						break;
					case VORPAL_BISECT:
						pline("%s cuts %s in half!",
							The(wepdesc),
							hittee);
						break;
					case VORPAL_PIERCE:
						pline("%s %s %s %s!",
							The(wepdesc),
							(!rn2(2) ? "pierces" : "punctures"),
							(youdef ? "your" : s_suffix(hittee)),
							mbodypart(mdef, HEART)
							);
						break;
					case VORPAL_SMASH:
						pline("%s smashes %s flat!",
							The(wepdesc),
							hittee
							);
						break;
					case VORPAL_IGNITE:
						pline("An ancient inferno flows from %s.", xname(otmp));
						break;
					case VORPAL_SHEAR:
						pline("%s slices through %s %s.",
							The(wepdesc),
							(youdef ? "your" : s_suffix(mon_nam(mdef))),
							mbodypart(mdef, NECK));
						pline("%s blow%s apart in the wind.",
							(youdef ? "You" : "It"),
							(youdef ? "" : "s"));
						break;
					}
					otmp->dknown = TRUE;
					*messaged = TRUE;
				}
				/* deal the lethal damage directly */
				if (!youdef) {
					return xdamagey(magr, mdef, (struct attack *)0, *hp(mdef));
				}
				else {
					losehp(*hp(&youmonst) + 1, an(wepdesc), KILLED_BY);
					return MM_DEF_LSVD;
				}
			}
			else {
				/* non-lethal messages */
				if (vis) {
					switch (method) {
					case VORPAL_BEHEAD:
						if (armor)
							pline("%s killing blow was blocked by %s helmet!",
								The(s_suffix(wepdesc)),
								(youdef ? "your" : s_suffix(mon_nam(mdef)))
								);
						else
							pline("%s slices %s %s %s%s",
								The(wepdesc),
								((noncorporeal(pd) || amorphous(pd)) ? "through" : "into"),
								(youdef ? "your" : s_suffix(mon_nam(mdef))),
								mbodypart(mdef, NECK),
								((vorpaldamage > 0) ? "!" : ".")
								);
						break;
					case VORPAL_BISECT:
						if (armor)
							pline("%s killing blow was blocked by %s armor!",
							The(s_suffix(wepdesc)),
							(youdef ? "your" : s_suffix(mon_nam(mdef)))
							);
						else
							pline("%s slices into %s!",
								The(wepdesc),
								hittee
								);
						break;
					case VORPAL_PIERCE:
						if (armor)
							pline("%s killing blow was blocked by %s armor!",
							The(s_suffix(wepdesc)),
							(youdef ? "your" : s_suffix(mon_nam(mdef)))
							);
						else
							pline("%s pierces into %s!",
								The(wepdesc),
								hittee
								);
						break;
					case VORPAL_SMASH:
						if (armor)
							pline("%s killing blow was blocked by %s shield!",
							The(s_suffix(wepdesc)),
							(youdef ? "your" : s_suffix(mon_nam(mdef)))
							);
						else
							pline("%s hits %s!",
								The(wepdesc),
								hittee
								);
						break;
					case VORPAL_IGNITE:
						/* we can't be here if mdef was fire-resistant */
						pline("%s burns %s terribly!",
							The(wepdesc),
							hittee
							);
						break;
					case VORPAL_SHEAR:
						pline("%s slices through %s %s.",
							The(wepdesc),
							(youdef ? "your" : s_suffix(mon_nam(mdef))),
							mbodypart(mdef, NECK));
						pline("%s ravages %s!",
							An(wepdesc),
							hittee
							);
						break;
					}
					otmp->dknown = TRUE;
					*messaged = TRUE;
				}
				/* don't directly deal the damage; let hmon() apply it */
				/* this damage may now be reduced by 1/2phys or DR or MG_RSLASH etc */
				*plusdmgptr += vorpaldamage;
			}
		}
	}

	if(arti_webweaver(otmp)){
		if(!youdef){
			struct trap *ttmp2 = maketrap(mdef->mx, mdef->my, WEB);
			if (ttmp2) mintrap(mdef);
		} else {
			struct trap *ttmp2 = maketrap(u.ux, u.uy, WEB);
			if (ttmp2) {
				pline_The("webbing sticks to you. You're caught!");
				dotrap(ttmp2, NOWEBMSG);
#ifdef STEED
				if (u.usteed && u.utrap) {
				/* you, not steed, are trapped */
				dismount_steed(DISMOUNT_FELL);
				}
#endif
			}
		}
	}

	if (oartifact == ART_HOLY_MOONLIGHT_SWORD) {
		if (youagr && mdef->mattackedu) {
			int life = otmp->lamplit ? ((basedmg)*.3 + 1) : ((basedmg)*.1 + 1);
			healup(life, 0, FALSE, FALSE);
		} else { /* m vs m or m vs u*/
			if (magr && magr->mhp < magr->mhpmax) {
				int life = otmp->lamplit ? ((basedmg)*.3 + 1) : ((basedmg)*.1 + 1);
				magr->mhp += life;
				if (magr->mhp > magr->mhpmax) magr->mhp = magr->mhpmax;
			}
		}
	}

	/* water damage to inventory */
	if (check_oprop(otmp, OPROP_WATRW)) {
		water_damage(youdef ? invent : mdef->minvent, FALSE, FALSE, FALSE, mdef);

		struct obj *cloak;
		if (youdef)
			cloak = uarmc;
		else
			cloak = which_armor(mdef, W_ARMC);
		
		if (!(youdef && Waterproof) && !(!youdef && mon_resistance(mdef, WATERPROOF)) && !(cloak && cloak->greased)) {
			if (pd->mtyp == PM_GREMLIN && rn2(3)){
				(void)split_mon(mdef, (struct monst *)0);
			}
		}
	}

	/*level drain*/
	if ((check_oprop(otmp, OPROP_DRANW) 
		|| (otmp->otyp == TOOTH && otmp->ovar1_tooth_type == VOID_TOOTH && Insight >= 20 && otmp->o_e_trait&ETRAIT_FOCUS_FIRE && CHECK_ETRAIT(otmp, magr, ETRAIT_FOCUS_FIRE))
		) && !Drain_res(mdef)
	){
		int dlife;
		int n = 0;
		if(check_oprop(otmp, OPROP_DRANW))
			n++;
		if(otmp->otyp == TOOTH && otmp->ovar1_tooth_type == VOID_TOOTH && Insight >= 20 && otmp->o_e_trait&ETRAIT_FOCUS_FIRE && CHECK_ETRAIT(otmp, magr, ETRAIT_FOCUS_FIRE))
			n += 3;
		/* message */
		if (youdef) {
			if (Blind)
				You_feel("an object drain your life!");
			else
				pline("%s drains your life!", The(wepdesc));
			*messaged = TRUE;
		}
		else if (vis) {
				pline("%s draws the life from %s!",
				The(wepdesc),
				mon_nam(mdef));
			*messaged = TRUE;
		}

		if (youdef) {
			dlife = *hpmax(mdef);
			for(int i = 0; i < n; i++)
				losexp("life drainage", FALSE, FALSE, FALSE);
			dlife -= *hpmax(mdef);
		}
		else {
			dlife = *hpmax(mdef);
			for(int i = 0; i < n; i++){
				*hpmax(mdef) -= min_ints(rnd(hd_size(mdef->data)), *hpmax(mdef));
				if (*hpmax(mdef) == 0 || mlev(mdef) == 0) {
					*hp(mdef) = 1;
					*hpmax(mdef) = 1;
				}
				if (mdef->m_lev > 0)
					mdef->m_lev--;
				else
					*truedmgptr += *hpmax(mdef);
			}
			dlife -= *hpmax(mdef);
		}

		/* gain from drain */
		if (magr) {
			*hp(magr) += dlife / 2;
			if (*hp(magr) > *hpmax(magr))
				*hp(magr) = *hpmax(magr);
		}
		*truedmgptr += dlife;
		if (youagr || youdef)
			flags.botl = 1;
	}

	/*reveal mortality*/
	if (check_oprop(otmp, OPROP_MORTW) && !(youagr && FLAME_BAD) && !Drain_res(mdef) && (youdef ? Mortal_race : mortal_race(mdef))){
		int dlife;
		int i = rnd(2);
		/* message */
		if (youdef) {
			if (Blind)
				You_feel("your frailty revealed!");
			else
				pline("%s reveals your frailty!", The(wepdesc));
			*messaged = TRUE;
		}
		else if (vis) {
				pline("%s reveals %s frailty!",
				The(wepdesc),
				s_suffix(mon_nam(mdef)));
			*messaged = TRUE;
		}

		if (youdef) {
			dlife = *hpmax(mdef);
			while(i--)
				losexp("life drainage", FALSE, FALSE, FALSE);
			dlife -= *hpmax(mdef);
		}
		else {
			dlife = *hpmax(mdef);
			*hpmax(mdef) -= min_ints(d(i, hd_size(mdef->data)), *hpmax(mdef));
			if (*hpmax(mdef) == 0 || mlev(mdef) == 0) {
				*hp(mdef) = 1;
				*hpmax(mdef) = 1;
			}
			if(mdef->m_lev == 0)
				*truedmgptr += *hpmax(mdef);
			else if (mdef->m_lev >= i)
				mdef->m_lev -= i;
			else mdef->m_lev = 0;
			dlife -= *hpmax(mdef);
		}

		*truedmgptr += dlife;
		if (youdef)
			flags.botl = 1;
	}

	/* morgul weapons */
	if (otmp->cursed && (check_oprop(otmp, OPROP_MORGW) || check_oprop(otmp, OPROP_LESSER_MORGW))){
		int bonus = 0;
		if (youdef) {
			if (check_oprop(otmp, OPROP_MORGW))
				u.umorgul++;
			if (check_oprop(otmp, OPROP_LESSER_MORGW))
				u.umorgul = max(1, u.umorgul);
			bonus = u.umorgul;
		}
		else {
			if (check_oprop(otmp, OPROP_MORGW))
				bonus += d(1, 4);
			if (check_oprop(otmp, OPROP_LESSER_MORGW))
				bonus += 1;
			mdef->mhpmax -= bonus;
			if (mdef->mhpmax < 1)
				mdef->mhpmax = 1;
			mdef->mhp = min(mdef->mhp, mdef->mhpmax);
		}
		*truedmgptr += bonus;
	}

	/* flaying weapons */
	if ((check_oprop(otmp, OPROP_FLAYW)
		|| check_oprop(otmp, OPROP_LESSER_FLAYW)
		|| otmp->oartifact == ART_THORNS //Note: Thorns's damage is not reduced to 0 like flaying weapons.
		) && !check_res_engine(mdef, AD_SHRD)
	) {
		struct obj *obj = some_armor(mdef);
		int i;
		if (obj){
			if(!((youdef && Preservation)
				|| (!youdef && mon_resistance(mdef, PRESERVATION))
			)){
				if (vis) {
					pline("%s slices %s armor!",
						The(xname(msgr)),
						(youdef ? "your" : s_suffix(mon_nam(mdef)))
						);
					*messaged = TRUE;
				}
				if (check_oprop(otmp, OPROP_FLAYW)) i = rnd(4);
				else if (otmp->oartifact == ART_THORNS) i = rnd(3);
				else i = 1;
				for (; i>0; i--){
					if ((obj->oclass == ARMOR_CLASS || obj->oclass == BELT_CLASS) && obj->spe > -1 * a_acdr(objects[(obj)->otyp])){
						damage_item(obj);
						if (!i && vis) {
							pline("%s %s less effective.",
								(youdef ? "Your" : s_suffix(Monnam(mdef))),
								aobjnam(obj, "seem")
								);
						}
					}
					else if (!obj->oartifact){
						claws_destroy_marm(mdef, obj);
						i = 0;
					}
				}
			}
		}
		else if (!(thick_skinned(pd) || (youdef && u.sealsActive&SEAL_ECHIDNA) || nonliving(mdef->data)))
		{
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
				if (check_oprop(otmp, OPROP_FLAYW))
					HScreaming += 2;
				else
					HScreaming += 1;
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
				if (check_oprop(otmp, OPROP_FLAYW))
					mdef->movement = max(mdef->movement - 6, -12);
				else
					mdef->movement = max(mdef->movement - 2, -12);
			}
		}
	}

	/* Reveal unworthy */
	if (check_oprop(otmp, OPROP_SFUWW) && !(youagr && FLAME_BAD) && (is_minion(pd) || is_demon(pd) || (Drain_res(mdef) && (youdef ? Mortal_race : mortal_race(mdef))))){
		struct obj *obj;
		int i = (basedmg+1)/2;
		boolean printed = FALSE;
		while((obj = some_armor(mdef)) && i > 0){
			i-=1;
			if(oresist_disintegration(obj))
				continue;//May possibly sellect a different item next time.

			if(!((youdef && Preservation)
				|| (!youdef && mon_resistance(mdef, PRESERVATION))
			)){
				if (vis && !printed) {
					pline("%s disintegrates %s armor!",
						The(xname(msgr)),
						(youdef ? "your" : s_suffix(mon_nam(mdef)))
						);
					*messaged = TRUE;
					printed = TRUE;
				}
				if (obj->spe > -1 * objects[(obj)->otyp].a_ac){
					damage_item(obj);
				}
				else if (!obj->oartifact){
					destroy_marm(mdef, obj);
				}
			}
		}
		//Note: i may be as low as -2.
		if(i > 0){
			if(2*i >= basedmg)
				*truedmgptr += 2*basedmg;
			else
				*truedmgptr += basedmg + 2*i;
		}
		else
			*plusdmgptr += basedmg/2;
	}
	if(otmp->oartifact == ART_FINGERPRINT_SHIELD){
		// Blood
		if(youdef){
			u.umadness |= MAD_FRENZY;
			u.ustdy = min(u.ustdy+basedmg, basedmg);
		}
		else if(youagr){
			if(has_blood_mon(mdef) && roll_generic_madness(FALSE) && d(2,u.ulevel) >= mdef->m_lev){
				pline("%s %s leaps through %s %s!", s_suffix(Monnam(mdef)), mbodypart(mdef, BLOOD), mhis(mdef), mbodypart(mdef, BODY_SKIN));
				mdef->mhp = (3*mdef->mhp + 9)/10;
			}
			mdef->mstdy = min(mdef->mstdy+basedmg, basedmg);
		}
		else {
			if(has_blood_mon(mdef) && d(2,mlev(magr)) >= mdef->m_lev){
				pline("%s %s leaps through %s %s!", s_suffix(Monnam(mdef)), mbodypart(mdef, BLOOD), mhis(mdef), mbodypart(mdef, BODY_SKIN));
				mdef->mhp = (3*mdef->mhp + 9)/10;
			}
			mdef->mstdy = min(mdef->mstdy+basedmg, basedmg);
		}
		// Madness
		boolean affected = FALSE;
		if(youdef){
			if(!save_vs_sanloss()){
				change_usanity(-1*basedmg, TRUE);
				affected = TRUE;
			}
		}
		else if(youagr){
			if(!mindless_mon(mdef) && (mon_resistance(mdef,TELEPAT) || tp_sensemon(mdef) || !rn2(5)) && roll_generic_madness(FALSE)){
				//reset seen madnesses
				mdef->seenmadnesses = 0L;
				you_inflict_madness(mdef);
				affected = TRUE;
			}
		}
		else {
			if(!mindless_mon(mdef) && (mon_resistance(mdef,TELEPAT) || !rn2(5))){
				if(!resist(mdef, WEAPON_CLASS, 0, FALSE))
					mdef->mcrazed = TRUE;
				affected = TRUE;
			}
		}
		if(affected){
			if(!Fire_res(mdef)){
				*truedmgptr += d(2,6);
				*truedmgptr += (basedmg)/2;
			}
			if (!UseInvFire_res(mdef)) {
				if (!rn2(4)) (void) destroy_item(mdef, POTION_CLASS, AD_FIRE);
				if (!rn2(4)) (void) destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
				if (!rn2(7)) (void) destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
			}
			if(!Magic_res(mdef)){
				*truedmgptr += max_ints(d(4,4) + otmp->spe, 1);
				*truedmgptr += (basedmg+1)/2;
			}
		}
	}
	if(otmp->oartifact == ART_IBITE_ARM){
		struct obj *cloak = which_armor(mdef, W_ARMC);

		if (youdef && uarmc && uarmc->greased) {
			if (!rn2(uarmc->blessed ? 4 : 2)){
				uarmc->greased = 0;
				pline("The layer of grease on your %s dissolves.", xname(uarmc));
			}
		} else if (!youdef && cloak && cloak->greased){
			if (!rn2(cloak->blessed ? 4 : 2)){
				cloak->greased = 0;
				if(canseemon(mdef)) pline("The layer of grease on %s's %s dissolves.", mon_nam(mdef), xname(cloak));
			}
		} else if (!(youdef && Waterproof) && !(!youdef && mon_resistance(mdef, WATERPROOF))){
			int mult = (flaming(pd) || is_iron(pd)) ? 2 : 1;

			if(otmp->otyp == CLAWED_HAND && artinstance[ART_IBITE_ARM].IbiteUpgrades&IPROP_WAVE)
				*truedmgptr += d(6, 6)*mult;
			else
				*truedmgptr += d(2, 4)*mult;
		}
		
		if(is_human(pd)){
			*truedmgptr += rnd(10);
			if(is_mercenary(pd) || is_lord(pd) || is_prince(pd) || attacktype_fordmg(pd, AT_MAGC, AD_CLRC) || attacktype_fordmg(pd, AT_MMGC, AD_CLRC)){
				mdef->mibitemarked = TRUE;
				if(youagr)
					mdef->myoumarked = TRUE;
			}
		}
		if(artinstance[ART_IBITE_ARM].IbiteUpgrades&IPROP_DESTROY){
			if(is_unalive(pd)){
				*truedmgptr += basedmg;
			}
		}
		if(otmp->otyp == CLAWED_HAND && artinstance[ART_IBITE_ARM].IbiteUpgrades&IPROP_REVOKE){
			struct obj *stolen;
			if(!youdef){
				if(mdef->minvent){
					stolen = mdef->minvent;
					do {
						obj_extract_and_unequip_self(stolen);
						dropy(stolen);
					} while((stolen = mdef->minvent) && !resist(mdef, WEAPON_CLASS, 0, NOTELL));
					
				}
			}
		}
		
		//Torch effects when the moon is gibbous
		if((phase_of_the_moon() == 3 || phase_of_the_moon() == 5) && otmp->otyp == CLAWED_HAND){
			if(!Fire_res(mdef)){
				if (species_resists_cold(mdef))
					(*truedmgptr) += 3 * (rnd(16) + otmp->spe) / 2;
				else
					(*truedmgptr) += rnd(16) + otmp->spe;
			}
			if (!UseInvFire_res(mdef)){
				if (rn2(3)) destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
				if (rn2(3)) destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
				if (rn2(3)) destroy_item(mdef, POTION_CLASS, AD_FIRE);
			}
		}
	}

	if(youagr && otmp->oartifact){
		const struct artifact *weap = get_artifact(otmp);
		if((weap->inv_prop == GITH_ART || weap->inv_prop == AMALGUM_ART)){
			if(activeMentalEdge(GSTYLE_COLD)){
				if(!Cold_res(mdef))
					(*truedmgptr) += u.usanity > 50 ? 0 : u.usanity > 25 ? d(2,6) : u.usanity > 10 ? d(4,6) : d(6,6);
				if(hates_unholy_mon(mdef))
					(*truedmgptr) += u.usanity > 50 ? 0 : u.usanity > 25 ? d(1,9) : u.usanity > 10 ? d(2,9) : d(3,9);
			}
			if(activeMentalEdge(GSTYLE_ANTIMAGIC)){
				int major_chance = u.usanity < 50 ? 0 : u.usanity < 75 ? 1 : u.usanity < 90 ? 2 : 5;
				if(youdef){
					if(u.uen > 0){
						u.uen -= u.usanity/10;
						flags.botl = 1;
					}
					if(rn2(20) < major_chance){
						if(u.uen > 0){
							u.uen = max(u.uen-400, 0);
							flags.botl = 1;
						}
					}
				}
				else {
					mdef->mspec_used += rnd(u.usanity/10);
					if(rn2(20) < major_chance)
						set_mcan(mdef, TRUE);
				}
			}
		}
	}
	
	if(otmp->oartifact == ART_ESSCOOAHLIPBOOURRR){
		if(artinstance[otmp->oartifact].Esscoo_mid == mdef->m_id)
			*plusdmgptr += basedmg;
		else artinstance[otmp->oartifact].Esscoo_mid = mdef->m_id;
	}
	
	if(otmp->oartifact == ART_RED_CORDS_OF_ILMATER && otmp->owornmask&W_ARMG && magr){
		int wounds = *hpmax(magr) - *hp(magr);
		wounds = min(wounds, *hp(mdef)/2);
		
		if(youagr)
			healup(wounds, 0, FALSE, FALSE);
		else
			*hp(magr) += wounds;
		*hp(mdef) -= wounds;
		if(youagr || youdef)
			flags.botl = 1;
	}
	
	if(arti_struct && arti_struct->inv_prop == FALLING_STARS){
		if(!Fire_res(mdef)){
			if(Insight >= 36){
				*truedmgptr += d(6,6);
			}
			else if(Insight >= 6){
				*truedmgptr += d(Insight/6,6);
			}
		}
		if (!UseInvFire_res(mdef) && Insight >= 6){
			if (rn2(3)) destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
			if (rn2(3) && Insight >= 12) destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
			if (rn2(3) && Insight >= 18) destroy_item(mdef, POTION_CLASS, AD_FIRE);
		}
		if(NightmareAware_Insanity >= 4){
			int n = ClearThoughts ? 1 : 2;
			if(!Acid_res(mdef)){
				if(NightmareAware_Insanity >= 48)
					*truedmgptr += d(n,12);
				else
					*truedmgptr += d(n,NightmareAware_Insanity/4);
			}
			if(!UseInvAcid_res(mdef)){
				if (rn2(3)) destroy_item(mdef, POTION_CLASS, AD_FIRE);
			}
		}
		if(Insight >= 42 && Insight > rn2(73)){
			int x, y, n, tries = 0;
			coord cc;
			do{
				x = rn2(COLNO-2)+1;
				y = rn2(ROWNO-2)+1;
				cc.x=x;cc.y=y;
			} while (!(isok(x,y) && ACCESSIBLE(levl[x][y].typ) && (!magr || (distmin(x(magr), y(magr), x, y) > 2 &&
					clear_path(x(magr), y(magr), x, y)))) && tries++ < 1000);

			if(Insight >= 72)
				n = 6;
			else
				n = (Insight - 36)/6;
			n=rnd(n)+1;
			explode_yours(x, y, AD_PHYS, 0, d(6,6), EXPL_MUDDY, 1, FALSE);
			if (cansee(x, y)){
				if (Hallucination || Insight > 96) pline("Another Star Falls.");
				else pline("A star falls from the %s!", (In_outdoors(&u.uz) ? "sky" : ceiling(x, y)));
			} else {
				You_hear("a thunderous crash!");
			}

			while(n--) {
				explode_full(x, y, AD_FIRE, 0, d(6,6), EXPL_FIERY, 1, 4, FALSE, (struct permonst *) 0, 0L);
				
				x = cc.x+rnd(3)-1; y = cc.y+rnd(3)-1;
				if (!isok(x,y)) {
					/* Spell is reflected back to center */
					x = cc.x;
					y = cc.y;
				}
			}
		}
	}
	
	if(otmp->oartifact == ART_GOLDEN_SWORD_OF_Y_HA_TALLA){
		if(Insight > 8){
			extern const int clockwisex[8];
			extern const int clockwisey[8];
			int nx, ny;
			if(otmp->otyp != BULLWHIP || !rn2(4)){
				if(youdef && !Tele_blind && (Blind_telepat || !rn2(5))){
					*truedmgptr += d(2,2)+d(1,4);
				}
				else if(!youdef && !mindless_mon(mdef) && (mon_resistance(mdef,TELEPAT) || !rn2(5))){
					*truedmgptr += d(2,2)+d(1,4);
				}
			}
			for(int i = 0, j = rn2(8); i<8; i++,j++){
				nx = x(mdef) + clockwisex[j%8];
				ny = y(mdef) + clockwisey[j%8];
				if(!isok(nx,ny))
					continue;
				if(m_u_at(nx,ny) != 0)
					continue;
				//The world around the target warps into giant stinging scorpion tails
				if(Insight > 64 || Insight > rnd(64)){
					*plusdmgptr += d(1,8);
					if(!Poison_res(mdef)){
						if(!rn2(10))
							*truedmgptr += 80;
						else *truedmgptr += rnd(6);
						
					}
				}
			}
		}
		if(Insight > 64 && (otmp->otyp == BULLWHIP || !rn2(4))){
			if(youdef){
				u.umadness |= MAD_SCORPIONS;
			}
			else {
				mdef->mscorpions = TRUE;
			}
		}
	}
	
	if(check_oprop(otmp, OPROP_ELFLW) && Insight >= 33 && u.ualign.record > 3 && Insight > rn2(333)){
		/* cancel_monst handles resistance */
		cancel_monst(mdef, otmp, youagr, FALSE, FALSE, FALSE);
	}

	//Also does the bolt (when it hits as a launcher)
	if(otmp->otyp == CARCOSAN_STING){
		if(Insight >= 25){
			struct obj *arm = some_armor(mdef);
			if(arm){
				add_byakhee_to_obj(arm);
			}
		}
		if(youdef)
			make_stunned(HStun + d(1,3), FALSE);
		else
			mdef->mconf = TRUE;
	}
	if(sothweaponturn) switch(sothweaponturn){
		case AD_STTP:
			if(youdef ? !Teleport_control : !mon_resistance(mdef, TELEPORT_CONTROL)){
				if(!Teleport_control){
					struct obj *armor = some_armor(mdef);
					if (armor)
						teleport_arm(armor, mdef);
					else
						*truedmgptr += basedmg;
				}
			}
		break;
		case AD_VAMP:
			if(has_blood_mon(mdef)){
				if (youdef) {
					Your("blood is being drained!");
					IMPURITY_UP(u.uimp_blood)
				}
				/* aggressor lifesteals by dmg dealt */
				heal(magr, min(basedmg, *hp(mdef)));
				if(youagr && !youdef){
					yog_credit(mdef->data->cnutrit/500, FALSE);
					IMPURITY_UP(u.uimp_blood)
				}
				if(!Drain_res(mdef) && !rn2(3)){
					if(youagr && !youdef){
						yog_credit(max(mdef->m_lev, mdef->data->cnutrit/50), FALSE);
					}
					if(youdef){
						losexp("blood drain", TRUE, FALSE, FALSE);
					}
					else {
						if(vis&VIS_MDEF)
							pline("%s suddenly seems weaker!", Monnam(mdef));
						if(mlev(mdef) == 0)
							*truedmgptr += basedmg + *hp(mdef);
						else {
							int drain = rnd(hd_size(mdef->data));
							/* drain stats */
							mdef->m_lev--;
							mdef->mhpmax = max(mdef->mhpmax - drain, 1);
							mdef->mhp = min(mdef->mhpmax, mdef->mhp);
							*truedmgptr += drain;
						}
					}
				}
			}
		break;
		case AD_FIRE:
			if (youdef) {
				if (!Antimagic) {
					HPanicking += 3;
					if (magr && magr == u.ustuck && sticks(&youmonst)) {
						u.ustuck = (struct monst *)0;
						You("release %s!", mon_nam(magr));
					}
					*truedmgptr += d(2,6);
				}
			} else if(!mindless_mon(mdef)){
				if (!resist(mdef, WEAPON_CLASS, 0, NOTELL)){
					*truedmgptr += d(2,6);
					monflee(mdef, 3, FALSE, (mdef->mhp > *truedmgptr));
				}
			}
		break;
		case AD_POLY:{
			int weaktyp = big_to_little(mdef->mtyp);
			if(weaktyp != mdef->mtyp && !resist(mdef, WEAPON_CLASS, 0, FALSE)){
				newcham(mdef, weaktyp, FALSE, TRUE);
			}
			else {
				int hpdrain = d(2,6);
				*hp(mdef) = max_ints(1, *hp(mdef) - hpdrain);
				if(!resist(mdef, WEAPON_CLASS, 0, FALSE)){
					if(youdef){
						u.uhpbonus -= hpdrain;
						calc_total_maxhp();
					}
					else {
						*hpmax(mdef) = max_ints(1, *hpmax(mdef) - hpdrain);
						if(mdef->m_lev > 1 && mdef->mhpmax < mdef->m_lev*hd_size(mdef->data))
							mdef->m_lev--;
					}
				}
			}
		}break;
		// case AD_DESC:
		// break;
		case AD_DRST:
			if(!Poison_res(mdef)){
				if(haseyes(pd)){
					if(youdef){
						int numeyes = eyecount(youracedata);
						Your("%s sting%s!",
							 (numeyes == 1) ? body_part(EYE) : makeplural(body_part(EYE)),
							 (numeyes == 1) ? "s" : "");
						make_blinded(basedmg, FALSE);
					}
					else {
						mdef->mcansee = 0;
						mdef->mblinded = max(mdef->mblinded, 3);
					}
				}

				if(youdef && !Breathless && !inediate(pd)){
					make_vomiting(Vomiting+15+d(5,4), TRUE);
				}
				else if(!youdef && !breathless_mon(mdef) && (!inediate(pd) || is_android(mdef->data) || mdef->mtyp == PM_INCANTIFIER) && !resist(mdef, WEAPON_CLASS, 0, FALSE)){
					if(canseemon(mdef)){
						if(is_android(mdef->data))
							pline("%s retches!", Monnam(mdef));
						else if(mdef->mtyp == PM_INCANTIFIER)
							pline("%s pukes up a rainbow!", Monnam(mdef));
						else pline("%s vomits!", Monnam(mdef));
					}
					mdef->mcanmove = 0;
					if ((mdef->mfrozen + 3) > 127)
						mdef->mfrozen = 127;
					else mdef->mfrozen += 3;
				}
			}
		break;
		// case AD_MAGM:
		// break;
		case AD_MADF:{
			boolean affected = FALSE;
			if(youdef){
				if(!save_vs_sanloss()){
					change_usanity(-1*basedmg, TRUE);
					affected = TRUE;
				}
			}
			else if(youagr){
				if(!mindless_mon(mdef) && (mon_resistance(mdef,TELEPAT) || tp_sensemon(mdef) || !rn2(5)) && roll_generic_madness(FALSE)){
					//reset seen madnesses
					mdef->seenmadnesses = 0L;
					you_inflict_madness(mdef);
					affected = TRUE;
				}
			}
			else {
				if(!mindless_mon(mdef) && (mon_resistance(mdef,TELEPAT) || !rn2(5))){
					if(!resist(mdef, WEAPON_CLASS, 0, FALSE))
						mdef->mcrazed = TRUE;
					affected = TRUE;
				}
			}
			if(!Fire_res(mdef)){
				*truedmgptr += d(2,6);
				if(affected) *truedmgptr += (basedmg)/2;
			}
			if (!UseInvFire_res(mdef)) {
				if (!rn2(4)) (void) destroy_item(mdef, POTION_CLASS, AD_FIRE);
				if (!rn2(4)) (void) destroy_item(mdef, SCROLL_CLASS, AD_FIRE);
				if (!rn2(7)) (void) destroy_item(mdef, SPBOOK_CLASS, AD_FIRE);
			}
			if(!Magic_res(mdef)){
				*truedmgptr += max_ints(d(4,4) + otmp->spe, 1);
				if(affected) *truedmgptr += (basedmg+1)/2;
			}
		}break;
	}

	if(is_kinstealing_merc(otmp) && (
		(youagr && Insight > 20 && YOU_MERC_SPECIAL)
		|| (!youagr && insightful(magr->data) && is_chaotic_mon(magr))
	)){
		int target;
		// Level drain
		target = youagr ? (NightmareAware_Insanity + ACURR(A_CON)) : mlev(magr);
		target -= mlev(mdef);
		if(!Drain_res(mdef) && rnd(100) < target){
			if(youdef)
				losexp("life drainage", FALSE, FALSE, FALSE);
			else {
				int drain = rnd(hd_size(mdef->data));
				*hpmax(mdef) = max(1, *hpmax(mdef) - drain);
				*hp(mdef) = max(1, *hp(mdef) - drain);
				if (mdef->m_lev > 0)
					mdef->m_lev--;
			}
		}
		target = youagr ? (Insanity + ACURR(A_STR)) : mlev(magr);
		target -= mlev(mdef);
		if(rnd(100) < target && !check_res_engine(mdef, AD_SHRD)){
			struct obj *obj = some_armor(mdef);
			int i;
			if (obj){
				if(!((youdef && Preservation)
					|| (!youdef && mon_resistance(mdef, PRESERVATION))
				)){
					// if (vis) {
						// pline("%s slices %s armor!",
							// The(xname(msgr)),
							// (youdef ? "your" : s_suffix(mon_nam(mdef)))
							// );
						// *messaged = TRUE;
					// }
					if(youagr){
						i = rnd(2*count_madnesses()+1);
					}
					else i = rnd(3);

					for (; i>0; i--){
						if (obj->spe > -1 * objects[(obj)->otyp].a_ac){
							damage_item(obj);
							// if (!i && vis) {
								// pline("%s %s less effective.",
									// (youdef ? "Your" : s_suffix(Monnam(mdef))),
									// aobjnam(obj, "seem")
									// );
							// }
						}
						else if (!obj->oartifact){
							claws_destroy_marm(mdef, obj);
							obj = some_armor(mdef);
							if(!obj)
								i = 0;
						}
					}
				}
			}
		}
		// Theft
		target = youagr ? (Insight + ACURR(A_DEX)) : mlev(magr);
		target -= mlev(mdef);
		if(rnd(100) < target){
			if (youagr){
				if (mdef->minvent){
					struct obj *otmp2;
					long unwornmask = 0L;

					/* Don't steal worn items, and downweight wielded items */
					if ((otmp2 = mdef->minvent) != 0) {
						while (otmp2 &&
							otmp2->owornmask&W_ARMOR &&
							!((otmp2->owornmask&W_WEP) && !rn2(10))
							) otmp2 = otmp2->nobj;
					}

					/* Still has handling for worn items, in case that changes */
					if (otmp2 != 0){
						/* take the object away from the monster */
						if (otmp2->quan > 1L){
							otmp2 = splitobj(otmp2, 1L);
							obj_extract_self(otmp2);
						}
						else {
							obj_extract_and_unequip_self(otmp2);
							if ((unwornmask = otmp2->owornmask) != 0L) {
								mdef->misc_worn_check &= ~unwornmask;
								if (otmp2->owornmask & W_WEP) {
									setmnotwielded(mdef, otmp2);
									MON_NOWEP(mdef);
								}
								if (otmp2->owornmask & W_SWAPWEP) {
									setmnotwielded(mdef,otmp2);
									MON_NOSWEP(mdef);
								}
								otmp2->owornmask = 0L;
								update_mon_intrinsics(mdef, otmp2, FALSE, FALSE);
							}
						}
						(void)dropy(otmp2);
						(void)pickup(2);
						/* more take-away handling, after theft message */
						if (unwornmask & W_WEP) {		/* stole wielded weapon */
							possibly_unwield(mdef, FALSE);
						}
						else if (unwornmask & W_ARMG) {	/* stole worn gloves */
							mselftouch(mdef, (const char *)0, TRUE);
							if (mdef->mhp <= 0)	/* it's now a statue */
								return MM_DEF_DIED; /* monster is dead */
						}
					}
				}
			}
			else if (youdef){
				if(!(u.sealsActive&SEAL_ANDROMALIUS) && !check_mutation(TENDRIL_HAIR)){
					char buf[BUFSZ];
					buf[0] = '\0';
					steal(magr, buf, TRUE, FALSE);
				}
			}
			else {
				struct obj *obj;
				/* find an object to steal, non-cursed if magr is tame */
				for (obj = mdef->minvent; obj; obj = obj->nobj)
					if (!magr->mtame || !obj->cursed)
						break;

				if (obj) {
					char buf[BUFSZ], onambuf[BUFSZ], mdefnambuf[BUFSZ];

					/* make a special x_monnam() call that never omits
					   the saddle, and save it for later messages */
					Strcpy(mdefnambuf, x_monnam(mdef, ARTICLE_THE, (char *)0, 0, FALSE));
#ifdef STEED
					if (u.usteed == mdef &&
						obj == which_armor(mdef, W_SADDLE))
						/* "You can no longer ride <steed>." */
						dismount_steed(DISMOUNT_POLY);
#endif
					obj_extract_self(obj);
					if (obj->owornmask) {
						mdef->misc_worn_check &= ~obj->owornmask;
						if (obj->owornmask & W_WEP)
							setmnotwielded(mdef, obj);
						obj->owornmask = 0L;
						update_mon_intrinsics(mdef, obj, FALSE, FALSE);
					}
					/* add_to_minv() might free obj [if it merges] */
					// if (vis)
						// Strcpy(onambuf, doname(obj));
					// (void)add_to_minv(magr, obj);
					// if (vis) {
						// Strcpy(buf, Monnam(magr));
						// pline("%s steals %s from %s!", buf,
							// onambuf, mdefnambuf);
					// }
					possibly_unwield(mdef, FALSE);
					mdef->mstrategy &= ~STRAT_WAITFORU;
					mselftouch(mdef, (const char *)0, FALSE);
					if (mdef->mhp <= 0)
						return MM_DEF_DIED;
				}
			}
		}
	}
	if(mercy_blade_prop(otmp)){
		if(Insight >= 25 && !resist(mdef, youagr ? SPBOOK_CLASS : WEAPON_CLASS, 0, NOTELL)){
			if(youdef){
				if(u.uencouraged >= 0 && ACURR_MON(A_CHA, magr)/5 > 0)
					You("feel a rush of irrational mercy!");
				u.uencouraged = max(-1*(otmp->spe + ACURR_MON(A_CHA, magr)), u.uencouraged - ACURR_MON(A_CHA, magr)/5);
			}
			else if(youagr){
				mdef->encouraged = max(-1*(otmp->spe + ACURR(A_CHA)), mdef->encouraged - ACURR(A_CHA)/5);
			}
			else {
				mdef->encouraged = max(-1*(otmp->spe + ACURR_MON(A_CHA, mdef)), mdef->encouraged - ACURR_MON(A_CHA, magr)/5);
			}
		}
	}
	/* ********************************************
	KLUDGE ALERT AND WARNING: FROM THIS POINT ON, NON-ARTIFACTS OR ARTIFACTS THAT DID NOT TRIGGER SPEC_DBON_APPLIES WILL NOT OCCUR
	********************************************************
	*/

	if (!spec_dbon_applies) {
	    /* since damage bonus didn't apply, nothing more to do;  
	       no further attacks have side-effects on inventory */
	    return MM_HIT;
	}

	/* some artifacts will attempt to cancel (10% chance) the creatures they hit */
	if (arti_attack_prop(otmp, ARTA_CANCEL) && dieroll <= 2 && (youdef || !mdef->mcan)) {
		if (!Blind && (youagr || canseemon(magr))) {
			pline("%s flashes %s as it hits %s!",
				The(xname(otmp)),
				hcolor(NH_WHITE),
				mon_nam(mdef)
				);
			*messaged = TRUE;
		}
		/* cancel_monst handles resistance */
		cancel_monst(mdef, otmp, youagr, FALSE, FALSE, FALSE);
	}

	if (oartifact == ART_LIFEHUNT_SCYTHE) {
		if (youagr) {
			int life = (basedmg)/2+1;
			if(mdef->mstdy > 0) life += (mdef->mstdy)/2+1;
			
			mdef->mstdy += (basedmg)/2+1;
			u.ustdy += (basedmg) / 4 + 1;
			
			healup(life, 0, FALSE, FALSE);
		} else if (youdef) {
			int life = (basedmg) / 2 + 1;
			if(u.ustdy > 0) life += (u.ustdy)/2+1;
			
			u.ustdy += (basedmg) / 2 + 1;
			magr->mstdy += (basedmg) / 4 + 1;
			
			if (magr && magr->mhp < magr->mhpmax) {
				magr->mhp += life;
				if (magr->mhp > magr->mhpmax) magr->mhp = magr->mhpmax;
			}
		} else { /* m vs m */
			int life = (basedmg) / 2 + 1;
			if(mdef->mstdy > 0) life += (mdef->mstdy)/2+1;
			
			mdef->mstdy += (basedmg) / 2 + 1;
			magr->mstdy += (basedmg) / 4 + 1;
			
			if (magr && magr->mhp < magr->mhpmax) {
				magr->mhp += life+1;
				if (magr->mhp > magr->mhpmax) magr->mhp = magr->mhpmax;
			}
		}
	}
	if (oartifact == ART_PROFANED_GREATSCYTHE || oartifact == ART_FRIEDE_S_SCYTHE) {
		if (youagr) {
			mdef->mstdy += (basedmg) / 4 + 1;
		} else if (youdef) {
			u.ustdy += (basedmg) / 4 + 1;
		} else { /* m vs m */
			mdef->mstdy += (basedmg) / 4 + 1;
		}
	}
	if (oartifact == ART_SHADOWLOCK) {
		if (youagr) {
			losehp(d(4,6) + otmp->spe + (Cold_resistance ? 0 : (d(2,6) + otmp->spe)), "a cold black blade", KILLED_BY);
			if(!Cold_resistance && !rn2(4)) (void) destroy_item(magr, POTION_CLASS, AD_COLD);
		} else if(magr->mtyp != PM_LEVISTUS){
			magr->mhp -= d(4,6) + otmp->spe + (resists_cold(magr) ? 0 : (d(2,6) + otmp->spe));
			if(!resists_cold(magr) && !rn2(4)) (void) destroy_item(magr, POTION_CLASS, AD_COLD);
			// if(magr->mhp < 0){
				// mondied(magr);
			// }
		}
	}
	if (arti_attack_prop(otmp, ARTA_DRAIN)
		|| (otmp->oartifact == ART_ESSCOOAHLIPBOOURRR && !Drain_res(mdef))
		|| (dieroll <= 2 && youagr && otmp->oclass == SPBOOK_CLASS && (u.sealsActive&SEAL_PAIMON))
	){
		int dlife;
		int leveldrain = 1;
		/* message */
		if (youdef) {
			if (Blind)
				You_feel("an %s drain your life!",
				oartifact == ART_STORMBRINGER ?
				"unholy blade" : "object");
			else if (oartifact == ART_STORMBRINGER)
				pline_The("%s blade drains your life!",
				hcolor(NH_BLACK));
			else
				pline("%s drains your life!",
				The(distant_name(msgr, xname)));
			*messaged = TRUE;
		}
		else if (vis) {
			if (oartifact == ART_STORMBRINGER)
				pline_The("%s blade draws the life from %s!",
				hcolor(NH_BLACK),
				mon_nam(mdef));
			else
				pline("%s draws the life from %s!",
				The(distant_name(msgr, xname)),
				mon_nam(mdef));
			*messaged = TRUE;
		}

		/* Stormbringer special -- convert base damage into lifedrain */
		if (oartifact == ART_STORMBRINGER && dieroll <= 2) {
			leveldrain += basedmg / hd_size(mdef->data)/2;
			*plusdmgptr -= basedmg;
		}

		for (; leveldrain > 0; leveldrain--) {
			if (youdef) {
				dlife = *hpmax(mdef);
				losexp("life drainage", FALSE, FALSE, FALSE);
				dlife -= *hpmax(mdef);
			}
			else {
				dlife = *hpmax(mdef);
				*hpmax(mdef) -= min_ints(rnd(hd_size(mdef->data)), *hpmax(mdef));
				if (*hpmax(mdef) == 0 || mlev(mdef) == 0) {
					if (youagr && oartifact == ART_STORMBRINGER && dieroll <= 2 && !is_silent_mon(mdef))
						pline("%s cries out in pain and despair and terror.", Monnam(mdef));
					*hp(mdef) = 1;
					*hpmax(mdef) = 1;
					leveldrain = 0;	/* end leveldrain loop */
				}
				if (mdef->m_lev > 0)
					mdef->m_lev--;
				dlife -= *hpmax(mdef);
			}

			/* gain from drain */

			/* paimon drains into the book, not the attacker */
			if (dieroll <= 2 && youagr && otmp->oclass == SPBOOK_CLASS && (u.sealsActive&SEAL_PAIMON)) {
				if (otmp->spestudied>0)
					otmp->spestudied--;
			}
			/* everything else turns the drain into life */
			else {
				if (magr) {
					*hp(magr) += dlife / 2;
					if (*hp(magr) > *hpmax(magr))
						*hp(magr) = *hpmax(magr);
				}
			}
			*truedmgptr += dlife;
		}
		if (youagr || youdef)
			flags.botl = 1;
	}
	//Sleeping weapons put targets to sleep
	if(attacks(AD_SLEE, otmp) && !rn2(2)){
		if (youdef) {
			if (Blind) You("are put to sleep!");
			else You("are put to sleep by %s!", mon_nam(magr));
			fall_asleep(-rnd(arti_struct->damage ? arti_struct->damage : basedmg ? basedmg : 1), TRUE);
		}
		else {
			if (sleep_monst(mdef, rnd(arti_struct->damage ? arti_struct->damage : basedmg ? basedmg : 1), youagr ? 0 : WEAPON_CLASS)) {
				pline("%s falls asleep!",
					Monnam(mdef));
				mdef->mstrategy &= ~STRAT_WAITFORU;
				slept_monst(mdef);
			}
		}
	}
	//Studying weapons study target
	if(attacks(AD_STDY, otmp)){
		int * stdy = (youdef ? &(u.ustdy) : &(mdef->mstdy));
		int dmg = arti_struct->damage ? rnd(arti_struct->damage) : basedmg ? basedmg : 1;
		/* only print message or increase study if it is an increase */
		if(dmg > *stdy)
			(*stdy) = dmg;
	}
	return MM_HIT;
#undef currdmg
}

static NEARDATA const char recharge_type[] = { ALLOW_COUNT, ALL_CLASSES, 0 };
static NEARDATA const char invoke_types[] = { ALL_CLASSES, 0 };
		/* #invoke: an "ugly check" filters out most objects */


void
zerth_mantras()
{
	pline("There are mantras wound around the grip.");
	//Reign of Anger (MM)
	if(ACURR(A_WIS) < 8 && ACURR(A_WIS)+ACURR(A_INT) > 19 && Insanity > 75 && !(artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_WRATH)){
		// Modifies MM spell, +1 damage per die (implemented as +2 faces)
		You("decipher a new mantra!");
		pline("\"Greed and hates, pains and joys, jealousies and doubts. All of these fed on each other and the minds of the People were divided. In their division, the People were punished.\"");
		artinstance[ART_SKY_REFLECTED].ZerthUpgrades |= ZPROP_WRATH;
		more_experienced(300, 0);
		newexplevel();
	}
	//Scripture of Steel (+1 to-hit and +1 to pet saves)
	else if(ACURR(A_WIS)+ACURR(A_INT) > 23 && !(artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_STEEL)){
		You("decipher a new mantra!");
		pline("\"*Know* that flesh cannot mark steel. *Know* that steel may mark flesh.\"");
		artinstance[ART_SKY_REFLECTED].ZerthUpgrades |= ZPROP_STEEL;
		more_experienced(600, 0);
		newexplevel();
	}
	//Submerge the Will (Protection + Saves)
	else if(ACURR(A_WIS)+ACURR(A_INT) > 27 && !(artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_WILL)){
		// Modifies protection spell, grants half magic damage while active, pets get +10 mr
		You("decipher a new mantra!");
		pline("\"Lashed upon the Pillars, Zerthimon moved his mind to a place where pain could not reach, leaving his body behind.\"");
		artinstance[ART_SKY_REFLECTED].ZerthUpgrades |= ZPROP_WILL;
		more_experienced(900, 0);
		newexplevel();
	}
	//Vilquar's Eye (Blindness)
	else if(ACURR(A_WIS)+ACURR(A_INT) > 29 && !(artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_VILQUAR)){
		// Modifies invis effects, monsters must roll vs. resist to see you
		You("decipher a new mantra!");
		pline("\"Vilquar's eye was filled only with the reward he had been promised. He would see what he wished to see.\"");
		artinstance[ART_SKY_REFLECTED].ZerthUpgrades |= ZPROP_VILQUAR;
		more_experienced(1500, 0);
		newexplevel();
	}
	//Power of One (Strength)
	else if(ACURR(A_WIS)+ACURR(A_INT) > 31 && !(artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_POWER)){
		// Grants 25 str while wielded, bonus damage for pets
		You("decipher a new mantra!");
		pline("\"The strength of her *knowing* was so great, that all those that walked her path came to *know* themselves.\"");
		artinstance[ART_SKY_REFLECTED].ZerthUpgrades |= ZPROP_POWER;
		more_experienced(3000, 0);
		newexplevel();
	}
	//Balance in All Things
	else if(ACURR(A_WIS)+ACURR(A_INT) > 34 && !(artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_BALANCE)){
		// Modifies protection spell, monsters take damage when attacking
		You("decipher a new mantra!");
		pline("\"From the Separation of the People, came the *knowing* of Two Skies. From the *knowing* of Two Skies came the realization that hurting others, hurts oneself.\"");
		artinstance[ART_SKY_REFLECTED].ZerthUpgrades |= ZPROP_BALANCE;
		more_experienced(5000, 0);
		newexplevel();
	}
	//Missile of Patience
	else if(ACURR(A_WIS)+ACURR(A_INT) > 37 && !(artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_PATIENCE)){
		// Modifies Force Bolt
		You("decipher a new mantra!");
		pline("\"*Know* that the Rising of the People against the *illithid* was a thing built upon many turnings.\"");
		artinstance[ART_SKY_REFLECTED].ZerthUpgrades |= ZPROP_PATIENCE;
		more_experienced(8000, 0);
		newexplevel();
	}
	//Zerthimon's Focus
	else if(ACURR(A_WIS)+ACURR(A_INT) > 40 && !(artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_FOCUS)){
		// Modifies "crit" chance, proportional to sanity
		You("decipher a new mantra!");
		pline("\"A divided mind is one that does not *know* itself. When it is divided, it cleaves the body in two. When one has a single purpose, the body is strengthened. In *knowing* the self, grow strong.\"");
		artinstance[ART_SKY_REFLECTED].ZerthUpgrades |= ZPROP_FOCUS;
		more_experienced(16000, 0);
		newexplevel();
	}
}

int
ibite_upgrade_menu(obj)
struct obj *obj;
{

	winid tmpwin;
	int n, how;
	char buf[BUFSZ];
	menu_item *selected;
	char inclet = 'a';
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */

	*buf = '\0';
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
	
	if(!(artinstance[ART_IBITE_ARM].IbiteUpgrades&IPROP_WAVE)){
		Sprintf(buf, "Improved water attack");
		any.a_int = IPROP_WAVE;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(artinstance[ART_IBITE_ARM].IbiteUpgrades&IPROP_REVOKE)){
		Sprintf(buf, "Revoke target equipment");
		any.a_int = IPROP_REVOKE;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(artinstance[ART_IBITE_ARM].IbiteUpgrades&IPROP_DESTROY)){
		Sprintf(buf, "Destroy the nonliving");
		any.a_int = IPROP_DESTROY;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(artinstance[ART_IBITE_ARM].IbiteUpgrades&IPROP_TELEPORT)){
		Sprintf(buf, "Enable teleportation");
		any.a_int = IPROP_TELEPORT;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(artinstance[ART_IBITE_ARM].IbiteUpgrades&IPROP_LEVELPORT)){
		Sprintf(buf, "Enable level teleportation");
		any.a_int = IPROP_LEVELPORT;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(artinstance[ART_IBITE_ARM].IbiteUpgrades&IPROP_BRANCHPORT)){
		Sprintf(buf, "Enable branch teleportation");
		any.a_int = IPROP_BRANCHPORT;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(artinstance[ART_IBITE_ARM].IbiteUpgrades&IPROP_REFLECT)){
		Sprintf(buf, "Make reflective");
		any.a_int = IPROP_REFLECT;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	end_menu(tmpwin, "What upgrade?");

	how = PICK_ONE;
	n = select_menu(tmpwin, how, &selected);
	destroy_nhwindow(tmpwin);
	int picked;
	if(n > 0){
		picked = selected[0].item.a_int;
		free(selected);
	}
	else return MOVE_CANCELLED;

	artinstance[ART_IBITE_ARM].IbiteUpgrades |= picked;
	if(picked == IPROP_REFLECT)
		EReflecting |= W_WEP;
	artinstance[ART_IBITE_ARM].IbiteFavor -= 25*(10 + (artinstance[ART_IBITE_ARM].IbiteBoons * artinstance[ART_IBITE_ARM].IbiteBoons * 2 / 25));
	artinstance[ART_IBITE_ARM].IbiteBoons += TIER_C;
	return MOVE_STANDARD;
}

int
ibite_arm_menu(obj)
struct obj *obj;
{
	winid tmpwin;
	int n, how;
	char buf[BUFSZ];
	menu_item *selected;
	char inclet = 'a';
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */

	Sprintf(buf, "Do what?");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
	*buf = '\0';
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
	
	if(wizard)
		pline("%ld needed, %ld available", 25*(10 + (artinstance[ART_IBITE_ARM].IbiteBoons * artinstance[ART_IBITE_ARM].IbiteBoons * 2 / 25)),
	artinstance[ART_IBITE_ARM].IbiteFavor);
	
	if(artinstance[ART_IBITE_ARM].IbiteFavor > 25*(10 + (artinstance[ART_IBITE_ARM].IbiteBoons * artinstance[ART_IBITE_ARM].IbiteBoons * 2 / 25))){
		Sprintf(buf, "Strengthen the arm");
		any.a_int = -1;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(artinstance[ART_IBITE_ARM].IbiteUpgrades&IPROP_TELEPORT && !notel_level()){
		Sprintf(buf, "Teleport");
		any.a_int = IPROP_TELEPORT;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;
	if(artinstance[ART_IBITE_ARM].IbiteUpgrades&IPROP_LEVELPORT){
		Sprintf(buf, "Level-teleport");
		any.a_int = IPROP_LEVELPORT;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;
	if(artinstance[ART_IBITE_ARM].IbiteUpgrades&IPROP_BRANCHPORT){
		Sprintf(buf, "Branch-teleport");
		any.a_int = IPROP_BRANCHPORT;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;
	if(Insight >= 60){
		Sprintf(buf, "Raise ibite mob");
		any.a_int = -2;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	end_menu(tmpwin, "The Arm looks somehow attentive...");

	how = PICK_ONE;
	n = select_menu(tmpwin, how, &selected);
	destroy_nhwindow(tmpwin);
	int picked;
	if(n > 0){
		picked = selected[0].item.a_int;
		free(selected);
	}
	else return MOVE_CANCELLED;

	switch (picked) {
		case -1:
			return ibite_upgrade_menu(obj);

		case IPROP_TELEPORT:
			if(tele())
				return MOVE_STANDARD;
			else return MOVE_CANCELLED;

		case IPROP_LEVELPORT:
			if(level_tele())
				return MOVE_STANDARD;
			else return MOVE_CANCELLED;

		case IPROP_BRANCHPORT:
			if(branch_tele())
				return MOVE_STANDARD;
			else return MOVE_CANCELLED;

		case -2:
			doibite_ghosts(&youmonst, obj);
			return MOVE_STANDARD;
	}
	impossible("bad menu option in ibite_arm_menu");
	return MOVE_CANCELLED;
}

int
scorpion_upgrade_menu(obj)
struct obj *obj;
{

	winid tmpwin;
	int n, how;
	char buf[BUFSZ];
	menu_item *selected;
	char inclet = 'a';
	const char *msg = 0;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */

	*buf = '\0';
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
	
	if(!(obj->ovara_carapace&CPROP_HARD_SCALE)){
		Sprintf(buf, "Harden exoskeletal scales (1 point)");
		any.a_int = CPROP_HARD_SCALE;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	else if(!(obj->ovara_carapace&CPROP_PLATES)){
		Sprintf(buf, "Grow exoskeletal Plates (2 points)");
		any.a_int = CPROP_PLATES;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(obj->ovara_carapace&CPROP_FLEXIBLE)){
		Sprintf(buf, "Flexible chitin links (1 point)");
		any.a_int = CPROP_FLEXIBLE;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(obj->ovara_carapace&CPROP_WHIPPING)){
		Sprintf(buf, "Whipping tail (2 points)");
		any.a_int = CPROP_WHIPPING;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(obj->ovara_carapace&CPROP_ILL_STING)){
		Sprintf(buf, "Illness-inducing stinger (3 points)");
		any.a_int = CPROP_ILL_STING;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(obj->ovara_carapace&CPROP_SWIMMING)){
		Sprintf(buf, "Swimming (2 points)");
		any.a_int = CPROP_SWIMMING;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(obj->ovara_carapace&CPROP_ACID_RES) && mvitals[PM_SOLDIER_ANT].seen){
		Sprintf(buf, "Acid resistance (1 point)");
		any.a_int = CPROP_ACID_RES;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(obj->ovara_carapace&CPROP_FIRE_RES) && mvitals[PM_FIRE_ANT].seen){
		Sprintf(buf, "Fire resistance (1 point)");
		any.a_int = CPROP_FIRE_RES;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(obj->ovara_carapace&CPROP_SHIELDS) && mvitals[PM_GIANT_BEETLE].seen){
		Sprintf(buf, "Arm shields (2 points)");
		any.a_int = CPROP_SHIELDS;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(obj->ovara_carapace&CPROP_TELEPORT) && mvitals[PM_PHASE_SPIDER].seen){
		Sprintf(buf, "Teleportation (1 point)");
		any.a_int = CPROP_TELEPORT;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(obj->ovara_carapace&CPROP_TCONTROL) && mvitals[PM_PHASE_SPIDER].seen){
		Sprintf(buf, "Teleport control (1 point)");
		any.a_int = CPROP_TCONTROL;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(obj->ovara_carapace&CPROP_DRAINRES) && mvitals[PM_DEMONIC_BLACK_WIDOW].seen){
		Sprintf(buf, "Drain resistance (1 point)");
		any.a_int = CPROP_DRAINRES;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(obj->ovara_carapace&CPROP_IMPURITY) && Insight >= 5){
		Sprintf(buf, "Armor of impurity (2 points)");
		any.a_int = CPROP_IMPURITY;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(obj->ovara_carapace&CPROP_WINGS) && mvitals[PM_VERMIURGE].seen){
		Sprintf(buf, "Wings (2 points)");
		any.a_int = CPROP_WINGS;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(obj->ovara_carapace&CPROP_CLAWS) && mvitals[PM_VERMIURGE].seen){
		Sprintf(buf, "Claws (2 points)");
		any.a_int = CPROP_CLAWS;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	if(!(obj->ovara_carapace&CPROP_CROWN) && mvitals[PM_VERMIURGE].seen && mvitals[PM_VERMIURGE].died > 0){
		Sprintf(buf, "Crown (3 points)");
		any.a_int = CPROP_CROWN;
		add_menu(tmpwin, NO_GLYPH, &any,
			inclet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	inclet++;

	Sprintf(buf, "What upgrade?  (%ld upgrade point%s available and %ld unearned)",
		artinstance[ART_SCORPION_CARAPACE].CarapacePoints, plur(artinstance[ART_SCORPION_CARAPACE].CarapacePoints), 30 - artinstance[ART_SCORPION_CARAPACE].CarapaceLevel);
	end_menu(tmpwin, buf);

	how = PICK_ONE;
	n = select_menu(tmpwin, how, &selected);
	destroy_nhwindow(tmpwin);
	int picked;
	if(n > 0){
		picked = selected[0].item.a_int;
		free(selected);
	}
	else return MOVE_CANCELLED;

	int price = 1;
	if(picked == CPROP_ILL_STING
	 || picked == CPROP_CROWN
	){
		price = 3;
	}
	else if(picked == CPROP_PLATES
	 || picked == CPROP_WHIPPING
	 || picked == CPROP_SHIELDS
	 || picked == CPROP_IMPURITY
	 || picked == CPROP_SWIMMING
	 || picked == CPROP_CLAWS
	){
		price = 2;
	}

	if(price > artinstance[ART_SCORPION_CARAPACE].CarapacePoints){
		pline("Not enough upgrade points!");
		return MOVE_CANCELLED;
	}

	if(picked == CPROP_HARD_SCALE){
		msg = "The little scorpion carapaces clink like steel.";
	}
	else if(picked == CPROP_PLATES){
		msg = "The little scorpion carapaces fuse and grow.";
	}
	else if(picked == CPROP_FLEXIBLE){
		msg = "The chitin links between carapaces grow more flexible.";
	}
	else if(picked == CPROP_WHIPPING){
		msg = "The scorpion tail takes on a life of its own.";
	}
	else if(picked == CPROP_ILL_STING){
		msg = "The scorpion tail's venom crawls with a life of its own!";
	}
	else if(picked == CPROP_SWIMMING){
		msg = "...Scorpions can swim.";
	}
	else if(picked == CPROP_ACID_RES){
		msg = "The carapaces look tough.";
	}
	else if(picked == CPROP_FIRE_RES){
		msg = "The carapaces look fireproof.";
	}
	else if(picked == CPROP_SHIELDS){
		msg = "Elytra grow over the vambrace carapaces.";
	}
	else if(picked == CPROP_TELEPORT){
		msg = "The scorpion carapaces look jumpy.";
	}
	else if(picked == CPROP_TCONTROL){
		msg = "The scorpion carapaces are in control.";
	}
	else if(picked == CPROP_DRAINRES){
		msg = "The scorpion carapaces look full of energy.";
	}
	else if(picked == CPROP_IMPURITY){
		msg = "Red millipedes crawl over the filth coating the carapaces.";
	}
	else if(picked == CPROP_WINGS){
		msg = "Vermiurge wings unfold from your back!";
	}
	else if(picked == CPROP_CLAWS){
		msg = "The carapaces grow an extra pair of claws!";
	}
	else if(picked == CPROP_CROWN){
		msg = "A crown of vermin gathers!";
	}

	if(msg)
		pline("%s", msg);
	obj->ovara_carapace |= picked;
	if(obj == uarm){
		if(picked == CPROP_FIRE_RES)
			EFire_resistance |= W_ARM;
		else if(picked == CPROP_ACID_RES)
			EAcid_resistance |= W_ARM;
		else if(picked == CPROP_DRAINRES)
			EDrain_resistance |= W_ARM;
		else if(picked == CPROP_SWIMMING){
			ESwimming |= W_ARM;
			EWaterproof |= W_ARM;
		}
		else if(picked == CPROP_TELEPORT)
			ETeleportation |= W_ARM;
		else if(picked == CPROP_TCONTROL)
			ETeleport_control |= W_ARM;
		else if(picked == CPROP_WINGS)
			EFlying |= W_ARM;
	}
	
	artinstance[ART_SCORPION_CARAPACE].CarapacePoints -= price;

	return MOVE_STANDARD;
}

int
doinvoke()
{
    register struct obj *obj;

    obj = getobj(invoke_types, "invoke");
    if (!obj) return 0;
    if (obj->oartifact && !touch_artifact(obj, &youmonst, FALSE)) return 1;
	if(is_lightsaber(obj) && obj->cobj && obj->oartifact == obj->cobj->oartifact)
		obj = obj->cobj;
    return arti_invoke(obj);
}

int
doparticularinvoke(obj)
    register struct obj *obj;
{
    if (!obj) return 0;
    if (obj->oartifact && !touch_artifact(obj, &youmonst, FALSE)) return 1;
	if(is_lightsaber(obj) && obj->cobj && obj->oartifact == obj->cobj->oartifact)
		obj = obj->cobj;
    return arti_invoke(obj);
}

STATIC_OVL int
fingerprint_shield(obj)
struct obj *obj;
{
	if (nohands(youracedata)) {	/* should also check for no ears and/or deaf */
		You("can't investigate the shield with no hands!");	/* not `body_part(HAND)' */
		return MOVE_CANCELLED;
	} else if (!freehand()) {
		You("can't investigate the shield without free %s.", body_part(HAND));
		return MOVE_CANCELLED;
	}
	You("run your %s over the sharp fingerprints.", makeplural(body_part(FINGER)));
	if(!has_blood(youracedata) || mlev(&youmonst) == 1){
		pline("Nothing happens.");
		return MOVE_CANCELLED;
	}
	You("fear you may cut yourself.");
	if(yn("Continue?") != 'y')
		return MOVE_CANCELLED;
	Your("blood flows down the ridges and pools in the valleys.");
	losexp("blood loss",TRUE,FALSE,TRUE);
	if(u.veil){
		You("feel reality threatening to slip away!");
		if (yn("Are you sure you want to studying the patterns?") == 'y'){
			pline("So be it.");
			u.veil = FALSE;
			change_uinsight(1);
		}
		return MOVE_STANDARD;
	}

	int difficulty = artinstance[ART_FINGERPRINT_SHIELD].FingerprintProgress + 1;
	difficulty *= 12;
	difficulty -= Insight;
	difficulty -= ACURR(A_INT);
	difficulty -= 14 - ACURR(A_WIS); //High wis may apply a penalty
	difficulty -= 10 - ACURR(A_CHA); //High cha may apply a penalty
	if(rnd(20) < difficulty){
		You("study the patterns, but no revelation comes.");
		return MOVE_STANDARD;
	}

	if(artinstance[ART_FINGERPRINT_SHIELD].FingerprintProgress == 0){
		pline("As you study the patterns, you begin to sense some elusive deeper meaning.");
		if(u.usanity > 90){
			if(Insight > 40)
				You("feel an itch behind your eyes.");
			change_usanity(-1*d(2,6), FALSE);
		}
		else {
			if(Insight > 60)
				You("scratch at your eyes.");
			else 
				You("are blinded by a flash of magenta light.");
			make_blinded(3L, FALSE);
			
			if(Blind)
				pline("Trembling hands rest upon you, but for a moment.");
			else
				You("catch a brief glimpse of emaciated figures all around you; their trembling hands rest on your body.");
			artinstance[ART_FINGERPRINT_SHIELD].FingerprintProgress++;
		}
	}
	else if(artinstance[ART_FINGERPRINT_SHIELD].FingerprintProgress == 1){
		pline("You study the patterns, chasing the elusive deeper meaning.");
		if(u.usanity > 50){
			if(Insight > 45)
				You("feel a scratching behind your eyes.");
			change_usanity(-1*d(3,12), TRUE);
		}
		else {
			if(Insight > 60)
				You("claw at your eyes.");
			else 
				You("are blinded by a flare of magenta flame.");
			make_blinded(33L, FALSE);

			if(Blind){
				You_hear("howling voices all around.");
			}
			else {
				You("are surrounded by a crowd of howling emaciated figures!");
			}
			verbalize("All fractures, all that is broken.");
			verbalize("All torment and despair.");
			verbalize("All that distinguishes and divides.");
			verbalize("Melt it all away, until all is One again.");
			artinstance[ART_FINGERPRINT_SHIELD].FingerprintProgress++;
		}
	}
	else if(artinstance[ART_FINGERPRINT_SHIELD].FingerprintProgress == 2){
		pline("You study the patterns. You are closing on the Truth at last.");
		if(u.usanity > 10){
			if(Insight > 50)
				You("feel a clawing behind your eyes.");
			u.umadness |= MAD_FRENZY;
			change_usanity(-1*d(4,20), TRUE);
		}
		else {
			if(Insight > 60)
				You("tear out your eyes!");
			else 
				Your("eyes are blasted away by magenta fire.");
			Blinded |= TIMEOUT_INF; //Note: Easily reversable by the time you're likely to have this item.

			godvoice(GOD_YOG_SOTHOTH, "No more division. No more birth. All shall be one again.");
			if(!(u.specialSealsKnown & SEAL_YOG_SOTHOTH)){
				pline("You chant in tongues as a seal is engraved into your mind!");
				u.specialSealsKnown |= SEAL_YOG_SOTHOTH;
				u.yog_sothoth_atten = TRUE;
			}
			u.yog_sothoth_mutagen += 5;
			//Free mutation.
			if(!check_mutation(YOG_GAZE_1)){
				add_mutation(YOG_GAZE_1);
			}
			else if(!check_mutation(YOG_GAZE_2)){
				add_mutation(YOG_GAZE_2);
			}
			else {
				u.yog_sothoth_mutations--;
			}
			artinstance[ART_FINGERPRINT_SHIELD].FingerprintProgress++;
		}
	}
	return MOVE_STANDARD;
}

STATIC_OVL int
arti_invoke(obj)
    struct obj *obj;
{
    const struct artifact *oart = get_artifact(obj);
    char buf[BUFSZ];
 	struct obj *pseudo, *otmp, *pseudo2, *pseudo3;
	int summons[10] = {0, PM_FOG_CLOUD, PM_DUST_VORTEX, PM_STALKER, 
					  PM_ICE_VORTEX, PM_ENERGY_VORTEX, PM_STEAM_VORTEX, 
					  PM_FIRE_VORTEX, PM_AIR_ELEMENTAL, PM_LIGHTNING_PARAELEMENTAL};
	int windpets[9] = {0, PM_FOG_CLOUD, PM_DUST_VORTEX, PM_ICE_PARAELEMENTAL, PM_ICE_VORTEX,
					   PM_ENERGY_VORTEX, PM_AIR_ELEMENTAL, PM_LIGHTNING_PARAELEMENTAL, PM_COUATL};
	coord cc;
	int n, damage, time = MOVE_STANDARD;
	struct permonst *pm;
	struct monst *mtmp = 0;

	if(obj->oartifact == ART_FINGERPRINT_SHIELD && artinstance[ART_FINGERPRINT_SHIELD].FingerprintProgress < 3){
		return fingerprint_shield(obj);
	}

	if (!oart || !oart->inv_prop) {
		switch (obj->otyp)
		{
		case CRYSTAL_BALL:
			use_crystal_ball(obj);
			break;
		case RIN_WISHES:
			(void) use_ring_of_wishes(obj);
			break;
		case CANDLE_OF_INVOCATION:
			(void) use_candle_of_invocation(obj);
			break;
		default:
			pline1(nothing_happens);
			break;
		}
		return MOVE_STANDARD;
	}

    if(oart->inv_prop > LAST_PROP) {
	/* It's a special power, not "just" a property */
	if(obj->age > monstermoves && !(
		oart->inv_prop == FIRE_SHIKAI ||
		oart->inv_prop == SEVENFOLD ||
		oart->inv_prop == ANNUL ||
		oart->inv_prop == ALTMODE || 
		oart->inv_prop == LORDLY ||
		oart->inv_prop == DETESTATION ||
		oart->inv_prop == THEFT_TYPE ||
		oart->inv_prop == GITH_ART
		|| oart->inv_prop == ZERTH_ART
		|| oart->inv_prop == AMALGUM_ART
		|| oart->inv_prop == SCORPION_UPGRADES
	)) {
	    /* the artifact is tired :-) */
		if(obj->oartifact == ART_FIELD_MARSHAL_S_BATON){
			You_hear("the sounds of hurried preparation.");
			return MOVE_PARTIAL;
		}
		if(oart->inv_prop == VOID_CHIME) {
	    pline("%s makes no sound.", The(xname(obj)));
			return MOVE_PARTIAL;
		}
	    You_feel("that %s %s ignoring you.",
		     the(xname(obj)), otense(obj, "are"));
	    /* and just got more so; patience is essential... */
		obj->age += Role_if(PM_PRIEST) ? (long) d(1,20) : (long) d(3,10);
	    return MOVE_PARTIAL;
	}
	if(!( /* some properties can be used as often as desired, or track cooldowns in a different way */
		oart->inv_prop == FIRE_SHIKAI ||
		oart->inv_prop == ICE_SHIKAI ||
		oart->inv_prop == NECRONOMICON ||
		oart->inv_prop == SPIRITNAMES ||
		oart->inv_prop == ALTMODE ||
		oart->inv_prop == LORDLY ||
		oart->inv_prop == ANNUL ||
		oart->inv_prop == VOID_CHIME ||
		oart->inv_prop == CHANGE_SIZE ||
		oart->inv_prop == IMPERIAL_RING ||
		oart->inv_prop == THEFT_TYPE ||
		oart->inv_prop == GITH_ART ||
		oart->inv_prop == ZERTH_ART ||
		oart->inv_prop == AMALGUM_ART ||
		oart->inv_prop == SCORPION_UPGRADES ||
		oart->inv_prop == SEVENFOLD
	))
		obj->age = monstermoves + (long)(rnz(100)*(Role_if(PM_PRIEST) ? .8 : 1));

	if(oart->inv_prop == VOID_CHIME) obj->age = monstermoves + 125L;

	switch(oart->inv_prop) {
	case TAMING: {
	    struct obj pseudo;

	    pseudo = zeroobj;	/* neither cursed nor blessed */
	    pseudo.otyp = SCR_TAMING;
	    (void) seffects(&pseudo);
	    break;
	  }
	case LEADERSHIP: {
	    (void) pet_detect_and_tame(obj);
	    break;
	  }
	case HEALING: {
		int healamt = (maybe_polyd(u.mhmax - u.mh, u.uhpmax - u.uhp) + 1) / 2;
	    long creamed = (long)u.ucreamed;

	    if (healamt || Sick || Slimed || Blinded > creamed)
		You_feel("better.");
	    else
		goto nothing_special;
	    if (healamt > 0) {
		if (Upolyd) u.mh += healamt;
		else u.uhp += healamt;
	    }
	    if(Sick) make_sick(0L,(char *)0,FALSE,SICK_ALL);
	    if(Slimed) Slimed = 0L;
	    if (Blinded > creamed) make_blinded(creamed, FALSE);
	    flags.botl = 1;
	    break;
	  }
	case ENERGY_BOOST: {
	    int epboost = (u.uenmax + 1 - u.uen) / 2;
	    if(epboost < u.uenmax*.1) epboost = u.uenmax - u.uen;
		if(epboost > 400) epboost = 400;
	    if(epboost) {
			You_feel("re-energized.");
			u.uen += epboost;
			flags.botl = 1;
	    } else goto nothing_special;
	    break;
	  }
	case UNTRAP: {
	    if(!untrap(obj)) {
		obj->age = 0; /* don't charge for changing their mind */
		return MOVE_CANCELLED;
	    }
	    break;
	  }
	case CHARGE_OBJ: {
	    struct obj *otmp = getobj(recharge_type, "charge");
	    boolean b_effect;

	    if (!otmp) {
		obj->age = 0;
		return MOVE_CANCELLED;
	    }
	    b_effect = obj->blessed &&
		((Role_switch == oart->role || oart->role == NON_PM) && (Race_switch == oart->race || oart->race == NON_PM));
	    recharge(otmp, b_effect ? 1 : obj->cursed ? -1 : 0);
	    update_inventory();
	    break;
	  }
	case LEV_TELE:
	    if(!level_tele())
			goto nothing_special;
	    break;
	case CREATE_PORTAL:
		if(obj->oartifact == ART_SILVER_KEY && IS_ALTAR(levl[u.ux][u.uy].typ) && god_at_altar(u.ux, u.uy) == GOD_YOG_SOTHOTH){
			if(yn("Travel to the Gate of the Silver Key?") == 'y'){
				/*With appologies to "Through the Gates of the Silver Key" by H. P. Lovecraft. */
				pline("You have come to a place and a time where place and time have no meaning.");
				pline("There is no distinction between your past and present selves, just as there is no distinction between your future and present. There is only the immutable %s.", plname);
				pline("Light filters down from a sky of no color in baffling, contradictory directions, and plays almost sentiently over a line of gigantic hexagonal pedestals.");
				pline("Surmounting these pedestals are cloaked, ill-defined Shapes, and another Shape, too, which occupies no pedestal.");
				pline("It speaks to you without sound or language, and you find yourself back where you started!");
				pline("A seal is engraved into your mind!");
				u.specialSealsKnown |= SEAL_YOG_SOTHOTH;
			}
		}
		else if(!branch_tele())
			goto nothing_special;
	    break;
	case LOOT_SELF:
		obj->age = 0; //Age will be reset in loot container for Esscoo only
		if(use_container(obj, TRUE) == MOVE_CANCELLED){
			return MOVE_CANCELLED;
		}
	break;
	case QUEST_PORTAL: {
	    int i;
	    d_level newlev;
	    extern int n_dgns; /* from dungeon.c */
		
		if(In_quest(&u.uz)){
			pline("Nothing happens!");
			break;
		}
	    if(u.uhave.amulet || In_endgame(&u.uz)){
			You_feel("very disoriented for a moment.");
			break;
		}
		
		//Clear any old return info
	    for (i = 0; i < n_dgns; i++)
			dungeons[i].dunlev_ureturn = 0;
		
		//Set return info
		dungeons[u.uz.dnum].dunlev_ureturn = u.uz.dlevel;
		
	    newlev.dnum = quest_dnum;
		newlev.dlevel = qstart_level.dlevel;
		
		if(u.usteed && mon_has_amulet(u.usteed)){
			dismount_steed(DISMOUNT_VANISHED);
		}
		if(!Blind) You("are sucked into the scroll!");
		else You_feel("weightless for a moment.");
		
		obj_extract_self(obj);
		dropy(obj);
		
		schedule_goto(&newlev, FALSE, FALSE, FALSE,
				  "You feel dizzy for a moment, but the sensation passes.",
				  (char *)0, 0, 0);
	}break;
	case ENLIGHTENING:
	    (void)doenlightenment();
	break;
	case CREATE_AMMO: {
	    struct obj *otmp;
		if(obj->oartifact == ART_LONGBOW_OF_DIANA) otmp = mksobj(ARROW, NO_MKOBJ_FLAGS);
		else if(obj->oartifact == ART_FUMA_ITTO_NO_KEN) otmp = mksobj(SHURIKEN, NO_MKOBJ_FLAGS);
		else if(obj->oartifact == ART_SILVER_STARLIGHT){
			if(cansee(u.ux, u.uy)) pline("Silver starlight shines upon your blade!");
			obj->cursed = 0;
			obj->blessed = 1;
			obj->oeroded = 0;
			obj->oeroded2= 0;
			obj->oerodeproof = 1;
			if(obj->spe < 3) obj->spe = 3;
			otmp = mksobj(SHURIKEN, NO_MKOBJ_FLAGS);
		} else if(obj->oartifact == ART_YOICHI_NO_YUMI) otmp = mksobj(YA, NO_MKOBJ_FLAGS);
		else if(obj->oartifact == ART_WRATHFUL_SPIDER) otmp = mksobj(DROVEN_BOLT, NO_MKOBJ_FLAGS);
		else if(obj->oartifact == ART_LIECLEAVER) otmp = mksobj(DROVEN_BOLT, NO_MKOBJ_FLAGS);
		else if(obj->oartifact == ART_BELTHRONDING) otmp = mksobj(ELVEN_ARROW, NO_MKOBJ_FLAGS);
		else otmp = mksobj(ROCK, NO_MKOBJ_FLAGS);

	    if (!otmp) goto nothing_special;
	    otmp->blessed = obj->blessed;
	    otmp->cursed = obj->cursed;

	    otmp->bknown |= obj->bknown;
		otmp->known |= obj->known;
		otmp->rknown |= obj->rknown;
		otmp->dknown |= obj->dknown;
		otmp->sknown |= obj->sknown;

	    if (obj->blessed) {
			if (otmp->spe < 0) otmp->spe = 0;
			otmp->quan += rnd(10);
	    } else if (obj->cursed) {
			if (otmp->spe > 0) otmp->spe = 0;
	    } else otmp->quan += rnd(5);
	    otmp->owt = weight(otmp);
	    otmp = hold_another_object(otmp, "Suddenly %s out.",
				       aobjnam(otmp, "fall"), (const char *)0);
	}
	break;
	case OBJECT_DET:
		object_detect(obj, 0);
		artifact_detect(obj);
	break;
	case TELEPORT_SHOES:
		if(obj->owornmask){
			if(notel_level() && obj->oartifact == ART_CLOAK_OF_THE_UNHELD_ONE){
				if(!u.uhave.amulet){
					if(Can_rise_up(u.ux, u.uy, &u.uz)){
						int newlev = depth(&u.uz)-1;
						d_level newlevel;
						get_level(&newlevel, newlev);
						if(on_level(&newlevel, &u.uz)) {
	break;
						}
						goto_level(&newlevel, FALSE, FALSE, FALSE);
					} else if(Can_fall_thru(&u.uz)){
						int newlev = depth(&u.uz)+1;
						d_level newlevel;
						get_level(&newlevel, newlev);
						if(on_level(&newlevel, &u.uz)) {
	break;
						}
						goto_level(&newlevel, FALSE, FALSE, FALSE);
					} else {
						(void) safe_teleds(FALSE);
					}
				} else {
					if(Can_fall_thru(&u.uz)){
						int newlev = depth(&u.uz)+1;
						d_level newlevel;
						get_level(&newlevel, newlev);
						if(on_level(&newlevel, &u.uz)) {
	break;
						}
						goto_level(&newlevel, FALSE, FALSE, FALSE);
					} else if(Can_rise_up(u.ux, u.uy, &u.uz)){
						int newlev = depth(&u.uz)-1;
						d_level newlevel;
						get_level(&newlevel, newlev);
						if(on_level(&newlevel, &u.uz)) {
	break;
						}
						goto_level(&newlevel, FALSE, FALSE, FALSE);
					} else {
						(void) safe_teleds(FALSE);
					}
				}
			}
			else tele();//hungerless teleport
		} else {
			obj_extract_and_unequip_self(obj);
			randomly_place_obj(obj);
			pline("%s teleports without you.",xname(obj));
		}
	break;
#ifdef CONVICT
	case PHASING:   /* Walk through walls and stone like a xorn */
        if (Passes_walls) goto nothing_special;
        if (!Hallucination) {    
            Your("body begins to feel less solid.");
        } else {
            You_feel("one with the spirit world.");
        }
		if(obj->oartifact == ART_CLOAK_OF_THE_UNHELD_ONE)
			u.spiritPColdowns[PWR_PHASE_STEP] = moves + 25;
        incr_itimeout(&Phasing, (50 + rnd(100)));
        obj->age += Phasing; /* Time begins after phasing ends */
    break;
#endif /* CONVICT */

	case SATURN:
		{
			if (!(uwep && uwep == obj)) {
				You_feel("that you should be wielding %s.", the(xname(obj)));
				obj->age = 0;
				break;
			}
			if (!getdir((char *)0)) { //Oh, getdir must set the .d_ variables below.
			    /* getdir cancelled, just do the nondirectional scroll */
				obj->age = 0;
				break;
			}
			else if(u.dx || u.dy) {
				pline("Death Reborn Revolution!");
				pseudo = mksobj(SPE_MAGIC_MISSILE, MKOBJ_NOINIT);
				pseudo->blessed = pseudo->cursed = 0;
				pseudo->quan = 20L;			/* do not let useup get it */
				weffects(pseudo);
				obfree(pseudo, (struct obj *)0);	/* now, get rid of it */
			}
			else if(u.dz < 0){
				pline("Silence Wall!");
				cast_protection();
			}
			else if(u.dz > 0 ){
				if (yesno("Are you sure you want to bring down the Silence Glaive?", iflags.paranoid_quit) == 'y') {
					struct monst *mtmp, *mtmp2;
					int gonecnt = 0;
					You("touch the tip of the Silence Glaive to the ground.");
					// pline("The walls of the dungeon quake!");
					do_earthquake(u.ux, u.uy, 24, 3, FALSE, (struct monst *) 0);
					do_earthquake(u.ux, u.uy, 12, 3, FALSE, (struct monst *) 0);
					do_earthquake(u.ux, u.uy,  6, 3, FALSE, (struct monst *) 0);
					You("call out to the souls and spirits inhabiting this land.");
					for (mtmp = fmon; mtmp; mtmp = mtmp2) {
						mtmp2 = mtmp->nmon;
						if(mtmp->data->geno & G_GENO){
							if (DEADMONSTER(mtmp)) continue;
							mongone(mtmp);
							gonecnt++;
						}
					}
					pline("Having gathered %d followers, you set out for a distant place, your charges in tow.",
						gonecnt);
					killer_format = KILLED_BY;
					killer = "leading departed souls to another world";
					done(DIED);
					You("return, having delivered your followers to their final destination.");
				}
			}
			else{
				obj->age = 0;
			}
		}
	break;
 	case PLUTO:
		{
			if (!(uwep && uwep == obj)) {
				You_feel("that you should be wielding %s.", the(xname(obj)));
				obj->age = 0;
				break;
			}
			if (!getdir((char *)0)) { //Oh, getdir must set the .d_ variables below.
			    /* getdir cancelled, just do the nondirectional scroll */
				obj->age = 0;
				break;
			}
			else if(u.dx || u.dy) {
				pline("Dead Scream.");
				pseudo = mksobj(SPE_MAGIC_MISSILE, MKOBJ_NOINIT);
				pseudo->blessed = pseudo->cursed = 0;
				pseudo->quan = 20L;			/* do not let useup get it */
				weffects(pseudo);
				obfree(pseudo, (struct obj *)0);	/* now, get rid of it */
			}
			else if(u.dz < 0 && (yn("This is a forbidden technique.  Do you wish to use it anyway?") == 'y')){
				struct monst *mtmp;
				pline("Time Stop!");
				for(mtmp = fmon; mtmp; mtmp = mtmp->nmon){
					if(is_uvuudaum(mtmp->data) && !DEADMONSTER(mtmp)){
						mtmp->movement += NORMAL_SPEED*10;
					}
				}
				HTimeStop += 10;
				Stoned = 9;
				delayed_killer = "termination of personal timeline.";
				killer_format = KILLED_BY;
			}
			else{
				obj->age = 0;
			}
		}
	break;
	case AEGIS:
		{
			if (mvitals[PM_MEDUSA].died < 1
			&& mvitals[PM_GRUE].died < 1
			&& mvitals[PM_ECHO].died < 1
			&& mvitals[PM_SYNAISTHESIA].died < 1
			) {
				pline("It would seem that the Aegis is incomplete.");
				obj->age = 0;
				break;
			}
			if (!getdir((char *)0)) { //Oh, getdir must set the .d_ variables below.
			    /* getdir canceled */
				obj->age = 0;
				break;
			} else if(u.dx || u.dy) {
				if (isok(u.ux+u.dx, u.uy+u.dy) && (mtmp = m_at(u.ux+u.dx, u.uy+u.dy)) != 0 && mtmp->mcansee && mon_can_see_you(mtmp)) {
					boolean youattack = mtmp == &youmonst;
					if(obj->otyp == ROUNDSHIELD) You("display the shield's device to %s.", mon_nam(mtmp));
					else if(obj->otyp == CLOAK) You("display the cloak's clasp to %s.", mon_nam(mtmp));
					else You("display it to %s.", mon_nam(mtmp)); //Shouldn't be used
					
					if (!resists_ston(mtmp) && (mtmp->mcan || rn2(100)>(mtmp->data->mr/2))){
						minstapetrify(mtmp, youattack);
					}
					else {
						monflee(mtmp, rnd(100), TRUE, TRUE);
						if (!obj->cursed) mtmp->mcrazed = 1;
					}
				}
			} else if(u.dz > 0){
				struct engr *engrHere = engr_at(u.ux,u.uy);
				if(!engrHere){
					make_engr_at(u.ux, u.uy,	"", (moves - multi), DUST); 
					engrHere = engr_at(u.ux,u.uy); 
				}
				engrHere->ward_type = BURN;
				engrHere->ward_id = GORGONEION;
				engrHere->complete_wards = obj->blessed ? 3 : (obj->cursed ? 1 : 2);
			} else{
				obj->age = 0;
			}
		}
	break;
	case CREATE_POOL:
		{
			if (!(uwep && uwep == obj)) {
				You_feel("that you should be wielding %s.", the(xname(obj)));
				obj->age = 0;
				break;
			}
			if (!getdir((char *)0)) { //Oh, getdir must set the .d_ variables below.
			    /* getdir canceled */
				obj->age = 0;
				break;
			} else if(u.dx || u.dy) {
				int x = u.ux+u.dx, y = u.uy+u.dy;
				if (isok(x, y) && ACCESSIBLE(levl[x][y].typ)) {
					You("strike the %s with %s, and water springs forth!", surface(u.ux, u.uy), xname(obj));
					levl[x][y].typ = POOL;
					newsym(x, y);
					if(m_at(x,y))
						minliquid(m_at(x,y));
				} else {
					pline("Not enough room to strike the ground!");
					obj->age = 0;
				}
			} else if(u.dz > 0){
				int x = u.ux+u.dx, y = u.uy+u.dy;
				if (isok(x, y) && ACCESSIBLE(levl[x][y].typ)) {
					You("strike the %s with %s, and water springs forth!", surface(u.ux, u.uy), xname(obj));
					levl[x][y].typ = POOL;
					newsym(x, y);
					spoteffects(FALSE);
				} else {
					pline("Not enough room to strike the ground!");
					obj->age = 0;
				}
			} else {
				obj->age = 0;
			}
		}
	break;
 	case SPEED_BANKAI:
		{
			if ( !(uwep && uwep == obj)) {
				You_feel("that you should be wielding %s.", the(xname(obj)));
				obj->age = 0;
				break;
			}
			if (!getdir((char *)0)) { //Oh, getdir must set the .d_ variables below.
				obj->age = 0;
				break;
			}
			else if(u.dx || u.dy) {
				int range = u.ulevel+obj->spe;
			    int x,y;
				uchar typ;
				boolean shopdoor = FALSE;
				bhitpos.x = u.ux;
				bhitpos.y = u.uy;
				pline("Getsuga Tensho!");
				pseudo = mksobj(SPE_FORCE_BOLT, MKOBJ_NOINIT);
				pseudo->blessed = pseudo->cursed = 0;
				pseudo->quan = 20L;
				while(range-- > 0) {
					
				    bhitpos.x += u.dx;
				    bhitpos.y += u.dy;
				    x = bhitpos.x; y = bhitpos.y;
					
				    if(!isok(x, y)) {
						bhitpos.x -= u.dx;
						bhitpos.y -= u.dy;
						break;
				    }
					zap_dig(x,y,1);
				    typ = levl[bhitpos.x][bhitpos.y].typ;
					
				    if (typ == IRONBARS){
						break_iron_bars(bhitpos.x, bhitpos.y, TRUE);
				    }

				    if (find_drawbridge(&x,&y))
						    destroy_drawbridge(x,y);

				    if ((mtmp = m_at(bhitpos.x, bhitpos.y)) != 0) {
						int dmg;
						dmg = d(u.ulevel+obj->spe,12);
						if(u.ukrau_duration) dmg *= 1.5;
						dmg += spell_damage_bonus()*3;
//						pline("mon found at %d, %d", bhitpos.x, bhitpos.y);
					    if (cansee(bhitpos.x,bhitpos.y) && !canspotmon(mtmp)) {
						    map_invisible(bhitpos.x, bhitpos.y);
						}
					    resist(mtmp, WEAPON_CLASS, dmg, FALSE);
						setmangry(mtmp);
					}
					bhitpile(pseudo,bhito,bhitpos.x,bhitpos.y);
				    if(IS_DOOR(typ) || typ == SDOOR) {
					    if (doorlock(pseudo, bhitpos.x, bhitpos.y)) {
							if (levl[bhitpos.x][bhitpos.y].doormask == D_BROKEN
							    && *in_rooms(bhitpos.x, bhitpos.y, SHOPBASE)) {
								    shopdoor = TRUE;
								    add_damage(bhitpos.x, bhitpos.y, 400L);
							}
					    }
				    }
				    typ = levl[bhitpos.x][bhitpos.y].typ;//must check type again, incase the preceding code changed it.
				    if(!ZAP_POS(typ) || closed_door(bhitpos.x, bhitpos.y)) {
//						pline("breaking, due to %d!",typ);
						bhitpos.x -= u.dx;
						bhitpos.y -= u.dy;
						break;
					}
				}
				if(shopdoor) pay_for_damage("destroy", FALSE);
				obfree(pseudo, (struct obj *)0);	/* now, get rid of it */
			}
			else{
				obj->age = 0;
			}
		}
	break;
 	case ICE_SHIKAI:
		{
			char dancenumber = 0;
			if ( !(uwep && uwep == obj) ) {
				You_feel("that you should be wielding %s.", the(xname(obj)));
				break;
			}
			pline("What dance will you use (1/2/3)?");
			dancenumber =  readchar();
			if (dancenumber == '1') {
				if(artinstance[obj->oartifact].SnSd1 > monstermoves){
				    You_feel("that %s %s ignoring you.",
					     the(xname(obj)), otense(obj, "are"));
					/* and just got more so; patience is essential... */
					artinstance[obj->oartifact].SnSd1 += Role_if(PM_PRIEST)||Role_if(PM_SAMURAI) ? (long) d(1,20) : (long) d(3,10);
				    return MOVE_PARTIAL;
				}
				else if(throweffect()){
					int dmg;
					dmg = u.ulevel + obj->spe;
					if(u.ukrau_duration) dmg *= 1.5;
					dmg += spell_damage_bonus();
					dmg *= 3;
					pline("Some no mai, Tsukishiro!");
					artinstance[obj->oartifact].SnSd1 = monstermoves + (long)(rnz(100)*(Role_if(PM_PRIEST)||Role_if(PM_SAMURAI) ? .9 : 1));
						explode(u.dx, u.dy,
							AD_COLD, 0,
							dmg,
							EXPL_FROSTY, 1);
					nomul(-1, "performing a sword dance");//both the first and the second dance leave you momentarily exposed.
				}
			}
			else if (dancenumber == '2') {
				if(artinstance[obj->oartifact].SnSd2 > monstermoves){
				    You_feel("that %s %s ignoring you.",
					     the(xname(obj)), otense(obj, "are"));
					/* and just got more so; patience is essential... */
					artinstance[obj->oartifact].SnSd2 += Role_if(PM_PRIEST)||Role_if(PM_SAMURAI) ? (long) d(1,20) : (long) d(3,10);
				    return MOVE_PARTIAL;
				}
				else if(getdir((char *)0) && (u.dx || u.dy)) {
					struct zapdata zapdata;
					basiczap(&zapdata, AD_COLD, ZAP_SPELL, 0);
					zapdata.flat = u.ulevel + obj->spe;

					pline("Tsugi no mai, Hakuren!");
					artinstance[obj->oartifact].SnSd2 = monstermoves + (long)(rnz(100)*(Role_if(PM_PRIEST)||Role_if(PM_SAMURAI) ? .9 : 1));
					
					zap(&youmonst, u.ux, u.uy, u.dx, u.dy, 7 + obj->spe, &zapdata);
				}
					nomul(-1, "performing a sword dance");//both the first and the second dance leave you momentarily exposed.
			}
			else if (dancenumber == '3') {
				if(artinstance[obj->oartifact].SnSd3 > monstermoves){
				    You_feel("that %s %s ignoring you.",
					     the(xname(obj)), otense(obj, "are"));
					/* and just got more so; patience is essential... */
					artinstance[obj->oartifact].SnSd3 += Role_if(PM_PRIEST)||Role_if(PM_SAMURAI) ? (long) d(1,20) : (long) d(3,10);
				    return MOVE_PARTIAL;
				}
				else{
					pline("San no mai, Shirafune!");
					pline("Ice crackles around your weapon!");
					artinstance[obj->oartifact].SnSd3 = monstermoves + (long)(rnz(100)*(Role_if(PM_PRIEST)||Role_if(PM_SAMURAI) ? .9 : 1));
					obj->cursed = 0;
					obj->blessed = 1;
					obj->oeroded = 0;
					obj->oeroded2= 0;
					if(obj->spe < 3) obj->spe = 3;
					if(obj->spe < u.ulevel/4) obj->spe = u.ulevel/4; //0 to 7
					artinstance[obj->oartifact].SnSd3duration = monstermoves + (long) u.ulevel + obj->spe;
				}
			}
			else{
				You("don't know that dance....");
			}
		}
	break;
 	case FIRE_SHIKAI:
		{
			boolean toosoon = monstermoves < obj->age;
			if ( !(uwep && uwep == obj) ) {
				You_feel("that you should be wielding %s.", the(xname(obj)));
	break;
			}
			else {
				int energy, damage, n;
				int role_skill;
				struct obj *pseudo;
				coord cc;

				energy = toosoon ? 25 : 15;
				pseudo = mksobj(SPE_FIREBALL, MKOBJ_NOINIT);
				pseudo->blessed = pseudo->cursed = 0;
				pseudo->quan = 20L;			/* do not let useup get it */
				role_skill = max(P_SKILL(uwep_skill_type()), P_SKILL(spell_skilltype(pseudo->otyp)) );
				if (role_skill >= P_SKILLED && yn("Use advanced technique?") == 'y'){
					if(energy > u.uen) {
						You("don't have enough energy to use this technique");
						if(!toosoon) obj->age = 0;
	break;
					}
			        if (throweffect()) {
						pline("Snap, %s!", ONAME(obj));
					    cc.x=u.dx;cc.y=u.dy;
					    n=rnd(role_skill*2)+role_skill*2;
					    while(n--) {
							if(!u.dx && !u.dy && !u.dz) {
							    if ((damage = zapyourself(pseudo, TRUE)) != 0) {
									char buf[BUFSZ];
									Sprintf(buf, "blasted %sself with a fireball", uhim());
									losehp(damage + obj->spe, buf, NO_KILLER_PREFIX);
							    }
							} else {
							    explode(u.dx, u.dy,
								    spell_adtype(pseudo->otyp), 0,
								    u.ulevel/2 + 10 + obj->spe,
									EXPL_FIERY, 1);
							}
							u.dx = cc.x+rnd(5)-3; u.dy = cc.y+rnd(5)-3;
							if (!isok(u.dx,u.dy) || !cansee(u.dx,u.dy) ||
							    IS_STWALL(levl[u.dx][u.dy].typ) || u.uswallow) {
							    /* Spell is reflected back to center */
								    u.dx = cc.x;
								    u.dy = cc.y;
					        }
					    }
					}
				}
				else{
					if (role_skill >= P_SKILLED) energy = toosoon ? 15 : 5;
					if(energy > u.uen) {
						You("don't have enough energy to use this technique");
						if(!toosoon) obj->age = 0;
	break;
					}
					if (!getdir((char *)0)) { //Oh, getdir must set the .d_ variables below.
					    /* getdir cancelled, re-use previous direction */
//					    pline_The("spiritual energy is released!");
	break;
					}
					pline("Snap, %s!", ONAME(obj));
					if(!u.dx && !u.dy && !u.dz) {
					    if ((damage = zapyourself(pseudo, TRUE)) != 0) {
							char buf[BUFSZ];
							Sprintf(buf, "zapped %sself with a spiritual technique", uhim());
							losehp(damage, buf, NO_KILLER_PREFIX);
					    }
					} 
					else weffects(pseudo);
				}
				update_inventory();	/* spell may modify inventory */
				losepw(energy);
				obfree(pseudo, (struct obj *)0);	/* now, get rid of it */
			}
		}
		obj->age = monstermoves + d(1,100);
	break;
	case SMOKE_CLOUD: {
           coord cc;
           cc.x = u.ux;
           cc.y = u.uy;
           /* Cause trouble if cursed or player is wrong role */
           if (obj->cursed || ((Role_switch == oart->role || oart->role == NON_PM) && (Race_switch == oart->race || oart->race == NON_PM))) {
              You("may summon a stinking cloud.");
               pline("Where do you want to center the cloud?");
               if (getpos(&cc, TRUE, "the desired position") < 0) {
                   pline1(Never_mind);
                   obj->age = 0;
                   return MOVE_CANCELLED;
               }
               if (!cansee(cc.x, cc.y) || distu(cc.x, cc.y) >= 32) {
                   You("smell rotten eggs.");
                   return MOVE_INSTANT;
               }
           }
           pline("A cloud of toxic smoke pours out!");
           (void) create_gas_cloud(cc.x, cc.y, 3+bcsign(obj), 8+4*bcsign(obj), TRUE);
		   }
    break;
	case BLESS:
		if(obj->owornmask&(~(W_ART|W_ARTI))){
			You("can't bless your artifact while it is worn, wielded, or readied.");
			obj->age = 0;
		} else {
			You("bless your artifact.");
			if(cansee(u.ux, u.uy)) pline("Holy light shines upon it!");
			obj->cursed = 0;
			obj->blessed = 1;
			obj->oeroded = 0;
			obj->oeroded2= 0;
			obj->oerodeproof = 1;
			if(obj->spe < 3){
				obj->spe = 3;
			}
		}
    break;
	case ARTI_REMOVE_CURSE:
	    {	register struct obj *otmp;
		if(Confusion != 0)
		    if (Hallucination)
			You_feel("the power of the Force against you!");
		    else
			You_feel("like you need some help.");
		else
		    if (Hallucination)
			You_feel("in touch with the Universal Oneness.");
		    else
			You_feel("like someone is helping you.");
		if (obj->cursed) {
			pline("The curse is lifted.");
			obj->cursed = FALSE;
		} else {
			if((Confusion == 0) && u.sealsActive&SEAL_MARIONETTE){
				unbind(SEAL_MARIONETTE,TRUE);
			} 
			if(Confusion == 0) u.wimage = 0;
			if(youmonst.mbleed)
				Your("accursed wound closes up.");
			youmonst.mbleed = 0;
		    for (otmp = invent; otmp; otmp = otmp->nobj) {
			long wornmask;
#ifdef GOLDotmp
			/* gold isn't subject to cursing and blessing */
			if (otmp->oclass == COIN_CLASS) continue;
#endif
			wornmask = (otmp->owornmask & ~(W_BALL|W_ART|W_ARTI));
			if (wornmask && !obj->blessed) {
			    /* handle a couple of special cases; we don't
			       allow auxiliary weapon slots to be used to
			       artificially increase number of worn items */
			    if (otmp == uswapwep) {
				if (!u.twoweap) wornmask = 0L;
			    } else if (otmp == uquiver) {
				if (otmp->oclass == WEAPON_CLASS) {
				    /* mergeable weapon test covers ammo,
				       missiles, spears, daggers & knives */
				    if (!objects[otmp->otyp].oc_merge) 
					wornmask = 0L;
				} else if (otmp->oclass == GEM_CLASS) {
				    /* possibly ought to check whether
				       alternate weapon is a sling... */
				    if (!uslinging()) wornmask = 0L;
				} else {
				    /* weptools don't merge and aren't
				       reasonable quivered weapons */
				    wornmask = 0L;
				}
			    }
			}
			if (obj->blessed || wornmask ||
			     otmp->otyp == LOADSTONE ||
			     (otmp->otyp == LEASH && otmp->leashmon)) {
			    if(Confusion != 0) blessorcurse(otmp, 2);
			    else uncurse(otmp);
			}
		    }
		}
		if(Punished && Confusion == 0) unpunish();
		update_inventory();
		break;
	    }
    break;
	case CANNONADE:
		You_hear("a voice shouting\"By your order, Sah!\"");
		if (getdir((char *)0) && (u.dx || u.dy)){
		    struct obj *otmp;
			int i;
			for(i = 12; i > 0; i--){
				int xadj=0;
				int yadj=0;
				otmp = mksobj(BALL, NO_MKOBJ_FLAGS);
			    otmp->blessed = 0;
			    otmp->cursed = 0;
				if(u.dy == 0) yadj = d(1,3)-2;
				else if(u.dx == 0) xadj = d(1,3)-2;
				else if(u.dx == u.dy){
					int dd = d(1,3)-2;
					xadj = dd;
					yadj = -1 * dd;
				}
				else{
					int dd = d(1,3)-2;
					xadj = yadj = dd;
				}
				projectile(&youmonst, otmp, (void *)0, HMON_PROJECTILE|HMON_FIRED, u.ux + xadj, u.uy + yadj, u.dx, u.dy, 0, 2*BOLT_LIM, TRUE, FALSE, FALSE);
				nomul(0, NULL);
			}
		}
    break;
	case FIRAGA:
		if(throweffect()){
			int dmg;
			dmg = u.ulevel + 10;
			if(u.ukrau_duration) dmg *= 1.5;
			dmg += spell_damage_bonus();
					exercise(A_WIS, TRUE);
					cc.x=u.dx;cc.y=u.dy;
					n=3;
					while(n--) {
						explode(u.dx, u.dy,
							AD_FIRE, 0,
							dmg,
							EXPL_FIERY, 1);
						u.dx = cc.x+rnd(3)-2; u.dy = cc.y+rnd(3)-2;
						if (!isok(u.dx,u.dy) || !cansee(u.dx,u.dy) ||
							IS_STWALL(levl[u.dx][u.dy].typ) || u.uswallow) {
							/* Spell is reflected back to center */
								u.dx = cc.x;
								u.dy = cc.y;
						}
					}
		} else obj->age = 0;
	break;
	case BLIZAGA:
		if(throweffect()){
			int dmg;
			dmg = u.ulevel + 10;
			if(u.ukrau_duration) dmg *= 1.5;
			dmg += spell_damage_bonus();
					exercise(A_WIS, TRUE);
					cc.x=u.dx;cc.y=u.dy;
					n=3;
					while(n--) {
						explode(u.dx, u.dy,
							AD_COLD, 0,
					dmg,
							EXPL_FROSTY, 1);
						u.dx = cc.x+rnd(3)-2; u.dy = cc.y+rnd(3)-2;
						if (!isok(u.dx,u.dy) || !cansee(u.dx,u.dy) ||
							IS_STWALL(levl[u.dx][u.dy].typ) || u.uswallow) {
							/* Spell is reflected back to center */
								u.dx = cc.x;
								u.dy = cc.y;
						}
					}
		} else obj->age = 0;
	break;
	case THUNDAGA:
		if(throweffect()){
			int dmg;
			dmg = u.ulevel + 10;
			if(u.ukrau_duration) dmg *= 1.5;
			dmg += spell_damage_bonus();
					exercise(A_WIS, TRUE);
					cc.x=u.dx;cc.y=u.dy;
					n=3;
					while(n--) {
						explode(u.dx, u.dy,
							AD_ELEC, 0,
					dmg,
							EXPL_MAGICAL, 1);
						u.dx = cc.x+rnd(3)-2; u.dy = cc.y+rnd(3)-2;
						if (!isok(u.dx,u.dy) || !cansee(u.dx,u.dy) ||
							IS_STWALL(levl[u.dx][u.dy].typ) || u.uswallow) {
							/* Spell is reflected back to center */
								u.dx = cc.x;
								u.dy = cc.y;
						}
					}
		} else obj->age = 0;
	break;
	case QUAKE:{
				register struct monst *mtmp, *mtmp2;
				pline("For a moment, you feel a crushing weight settle over you.");
				for (mtmp = fmon; mtmp; mtmp = mtmp2) {
					mtmp2 = mtmp->nmon;
					if(mtmp->data->geno & G_GENO){
						if (DEADMONSTER(mtmp)) continue;
						mtmp->mhp = mtmp->mhp/4 + 1;
						if(mon_can_see_you(mtmp) && !mtmp->mtame) //pets are willing to take the colateral damage
							setmangry(mtmp);
					}
				}
				pline_The("entire dungeon is quaking around you!");
				do_earthquake(u.ux, u.uy, u.ulevel / 2 + 1, 3, obj->cursed, (struct monst *)0);
				do_earthquake(u.ux, u.uy, u.ulevel*2 / 3 + 1, 2, obj->cursed, (struct monst *)0);
				awaken_monsters(ROWNO * COLNO);
			   }
	break;
	case FALLING_STARS:{
		int starfall = rnd(u.ulevel/10+1), x, y, n;
		int tries = 0;
		boolean needLoc = FALSE;
		coord cc;
		cc.x = u.ux;
		cc.y = u.uy;
		pline("Where do you want to target?");
		if (getpos(&cc, TRUE, "the desired position") < 0) {
			needLoc = TRUE;
		}
		x = cc.x;
		y = cc.y;

		verbalize("Even Stars Fall.");
		for (; starfall > 0; starfall--){
			if(needLoc){
				x = rn2(COLNO-2)+1;
				y = rn2(ROWNO-2)+1;
				if(!isok(x,y) || !ACCESSIBLE(levl[x][y].typ) || distmin(u.ux, u.uy, x, y) < 3){
					if(tries++ < 1000){
						continue;
					}
				}
				needLoc = FALSE;
			}
			cc.x=x;cc.y=y; // spell drifts slightly between iterations
			n=rnd(8)+1;
			explode(x, y, AD_PHYS, 0, d(6,6), EXPL_MUDDY, 1);
			while(n--) {
				if (rn2(2)) explode_sound(x, y, AD_FIRE, 0, d(6,6), EXPL_FIERY, 1, 4);
				else 		explode_sound(x, y, AD_PHYS, 0, d(6,6), EXPL_MUDDY, 1, 4);
				
				x = cc.x+rnd(3)-1; y = cc.y+rnd(3)-1;
				if (!isok(x,y)) {
					/* Spell is reflected back to center */
					x = cc.x;
					y = cc.y;
				}
			}
			do_earthquake(cc.x, cc.y, 6, 3, FALSE, &youmonst);
			do_earthquake(cc.x, cc.y, 3, 5, FALSE, &youmonst);
		}
		awaken_monsters(ROWNO * COLNO);
	} break;
	case THEFT_TYPE:
		if(obj->ovara_artiTheftType == 0){
			if(yn("Switch to autopickup mode")) obj->ovara_artiTheftType = 1;
		} else {
			if(yn("Switch to interactive mode")) obj->ovara_artiTheftType = 0;
		}
	break;
	case SHADOW_FLARE:
		if(throweffect()){
			int dmg;
			dmg = u.ulevel + 10;
			if(u.ukrau_duration) dmg *= 1.5;
			dmg += spell_damage_bonus();
			exercise(A_WIS, TRUE);
			cc.x=u.dx;cc.y=u.dy;
			n=3;
			while(n--) {
				explode(u.dx, u.dy,
					AD_DISN, 0,
					dmg,
					EXPL_DARK, 1);
				u.dx = cc.x+rnd(3)-2; u.dy = cc.y+rnd(3)-2;
				if (!isok(u.dx,u.dy) || !cansee(u.dx,u.dy) ||
					IS_STWALL(levl[u.dx][u.dy].typ) || u.uswallow) {
					/* Spell is reflected back to center */
						u.dx = cc.x;
						u.dy = cc.y;
				}
			}
		}
	break;
	case BLADESONG:
	{
		// check whether or not you're wielding the mate, used for speed/protection
		// intentional that the dagger is always weaker than the spear
		int dancer = 0;	
		if (obj->oartifact == ART_BLADE_DANCER_S_DAGGER) dancer = 1; 
		else if (uswapwep && uswapwep->oartifact == ART_BLADE_DANCER_S_DAGGER) dancer = 2;
		else dancer = 1;
		
		pline("You enter a %strance, giving you an edge in battle.", (dancer==2) ? "deep ":"");
		// heal you up to half of your lost hp, modified by enchantment
		int healamt = (maybe_polyd(u.mhmax - u.mh, u.uhpmax - u.uhp) + 1) / ((dancer == 2) ? 2 : 4);
		healamt = max(0, healamt * (obj->spe/10));
		healup(healamt, 0, FALSE, FALSE);

		// make you very fast for a limited time
		incr_itimeout(&HFast, rn1(u.ulevel, 50*dancer));
		
		// give you some protection
		int l = u.ulevel;
		int loglev = 0;
		int gain;

		while (l) {
			loglev++;
			l /= 2;
		}
		gain = dancer*loglev - u.uspellprot; //refill spellprot, don't stack it.
		if (gain > 0) {
			if (!Blind) {
			const char *hgolden = hcolor(NH_GOLDEN);

			if (u.uspellprot)
				pline_The("%s haze around you becomes more dense.",
					  hgolden);
			else
				pline_The("%s around you begins to shimmer with %s haze.",
					  (Underwater || Is_waterlevel(&u.uz)) ? "water" :
					   u.uswallow ? mbodypart(u.ustuck, STOMACH) :
					  IS_STWALL(levl[u.ux][u.uy].typ) ? "stone" : "air",
					  an(hgolden));
			}
			u.uspellprot += gain;
			if (u.uspellprot > 2*loglev)
				u.uspellprot = 2*loglev;
			
			u.usptime = (u.ulevel * dancer * ((obj->blessed) ? 30 : ((obj->cursed) ? 5 : 15)))/30;
			u.uspmtime = (obj->blessed) ? 10 : ((obj->cursed) ? 1 : 5);
			
			find_ac();

		} else {
			Your("skin feels warm for a moment.");
		}
	}
	break;
	case SEVENFOLD:
//Ruat:  Fall.  Makes a pit.  One charge.
//Coelum: Heaven.  Casts cure.  One charge.
//Fiat:  Let it be.  Creates food.  One charge.
//Justitia:  Justice.  Creates light.  One charge.
//Ecce:  See.  Casts detect monsters or detect unseen.  Two charges.
//Lex:  Law.  Grants you enlightenment.  Two charges.
//Rex:  King.  Grants Levitation.  Three charges.
//Ruat Coelum:  Heaven Falls.  Attack spell.  One charge.
//Fiat Justitia:  Let justice be done.  Slows, fears and damages target.  Two charges.
//Lex Rex:  Law is King.  Heals aflictions, removes curses.  Three charges.
//Ecce!  Lex Rex:  See!  Law is King.  Charms monsters.  Five charges.
//Ruat Coelum, Fiat Justitia:  Though heaven falls, let justice be done.  Calls pets.  Seven charges.
//Ruat Coelum, Fiat Justitia.  Ecce!  Lex Rex!:  Though heaven falls, let justice be done.  See!  Law is King!  Heals you fully, calls pets, casts charm monster.  Fourteen charges.
		getlin("Speak your incantation:", buf);
			if(!*buf || *buf == '\033'){
				if( (d(1,20)-10) > 0 ){
					exercise(A_WIS, FALSE);
					obj->spe--;//lose charge for false invoke
					pline("Your weapon has become more flawed.");
				}
				else pline("Your weapon rattles warningly.");
				break;
			}
			if ( !(uwep && uwep == obj) ) {
				You_feel("that you should be wielding %s.", the(xname(obj)));
				if( (d(1,20)-10) > 0 ){
					obj->spe--;//lose charge for false invoke
					exercise(A_WIS, FALSE);
					pline("Your weapon has become more flawed.");
				}
				else pline("Your weapon rattles warningly.");
		break;
			}
			if(strcmp(buf, "Ruat") == 0 ||
						strcmp(buf, "Ruat.") == 0 ||
						strcmp(buf, "Ruat!") == 0 ){ //Ruat:  Fall.  Makes a pit.  One charge.
				if( (obj->spe > -7) ){
					exercise(A_WIS, TRUE);
					if (!getdir((char *)0)) {
					 if( (d(1,20)-10) > 0 ){
						obj->spe--;//lose charge for false invoke
						exercise(A_WIS, FALSE);
						pline("Your weapon has become more flawed.");
					 }
					}
					else{
						int x, y;
						x = u.ux + u.dx;
						y = u.uy + u.dy;
						if(isok(x, y) && digfarhole(TRUE, x, y, TRUE)){
							obj->spe--; // lose charge
							pline("Your weapon has become more flawed.");
						} else if( (d(1,20)-10) > 0 ){
							obj->spe--;//lose charge for false invoke
							exercise(A_WIS, FALSE);
							pline("Your weapon has become more flawed.");
						}
					}
				} else pline("Your weapon rattles faintly.");
			}
			else if(strcmp(buf, "Coelum") == 0 ||
						strcmp(buf, "Coelum.") == 0 ||
						strcmp(buf, "Coelum!") == 0 ){//Coelum: Heaven.  Casts cure.  One charge.
				if( (obj->spe > -7) ){
					exercise(A_WIS, TRUE);
					You_feel("better.");
					healup(d(1 + u.ulevel/3, 4)+ u.ulevel, 0, FALSE, FALSE);
					exercise(A_CON, TRUE);
					obj->spe--; // lose charge
					pline("Your weapon has become more flawed.");
				} else pline("Your weapon rattles faintly.");
			}
			else if(strcmp(buf, "Fiat") == 0 ||
						strcmp(buf, "Fiat.") == 0 ||
						strcmp(buf, "Fiat!") == 0 ){//Fiat:  Let it be.  Creates food.  One charge.
				if( (obj->spe > -7)){
					exercise(A_WIS, TRUE);
					otmp = mksobj(FOOD_RATION, NO_MKOBJ_FLAGS);
					hold_another_object(otmp, "Suddenly %s out.",
				       an(aobjnam(otmp, "fall")), (const char *)0);
					obj->spe--; // lose charge
					pline("Your weapon has become more flawed.");
				} else pline("Your weapon rattles faintly.");
			}
			else if(strcmp(buf, "Justitia") == 0 ||
						strcmp(buf, "Justitia.") == 0 ||
						strcmp(buf, "Justitia!") == 0 ){//Justitia:  Justice.  Creates light.  One charge.
				if( (obj->spe > -7) ){
					exercise(A_WIS, TRUE);
					exercise(A_WIS, TRUE);
					pseudo = mksobj(SPE_LIGHT, MKOBJ_NOINIT);
					pseudo->blessed = pseudo->cursed = 0;
					pseudo->quan = 20L;			/* do not let useup get it */
					litroom(TRUE,pseudo);
					obfree(pseudo, (struct obj *)0);	/* now, get rid of it */
					obj->spe--; // lose charge
					pline("Your weapon has become more flawed.");
				} else pline("Your weapon rattles faintly.");
			}
			else if(strcmp(buf, "Ecce") == 0 ||
						strcmp(buf, "Ecce.") == 0 ||
						strcmp(buf, "Ecce!") == 0 ){//Ecce:  See.  Casts detect monsters and detect unseen.  Two charges.
				if( (obj->spe > -6) ){
					exercise(A_WIS, TRUE);
					pseudo = mksobj(SPE_DETECT_MONSTERS, MKOBJ_NOINIT);
					pseudo->blessed = pseudo->cursed = 0;
					pseudo->blessed = TRUE;
					pseudo->quan = 20L;			/* do not let useup get it */
					(void) peffects(pseudo, TRUE);
					obfree(pseudo, (struct obj *)0);	/* now, get rid of it */
					pseudo = mksobj(SPE_DETECT_UNSEEN, MKOBJ_NOINIT);
					pseudo->blessed = pseudo->cursed = 0;
					pseudo->blessed = TRUE;
					pseudo->quan = 20L;			/* do not let useup get it */
					weffects(pseudo);
					obfree(pseudo, (struct obj *)0);	/* now, get rid of it */

					obj->spe--; obj->spe--; // lose two charge
					pline("Your weapon has become more flawed!");
				} else pline("Your weapon rattles faintly.");
			}
			else if(strcmp(buf, "Lex") == 0 ||
						strcmp(buf, "Lex.") == 0 ||
						strcmp(buf, "Lex!") == 0 ){//Lex:  Law.  Grants you enlightenment.  Two charges.
				if((obj->spe > -6)){
					exercise(A_WIS, TRUE);
					exercise(A_WIS, TRUE);
					doenlightenment(); //not dead yet!
					unrestrict_weapon_skill(P_SPEAR);
					if(!obj->known){
						discover_artifact(ART_ROD_OF_SEVEN_PARTS);
						identify(obj);
						update_inventory();
					}
					obj->spe--; obj->spe--; // lose two charge
					pline("Your weapon has become more flawed!");
				} else pline("Your weapon rattles faintly.");
			}
			else if(strcmp(buf, "Rex") == 0){//Rex:  King.  Grants Levitation.  Three charges.
				if( (artinstance[ART_ROD_OF_SEVEN_PARTS].RoSPflights > 0) ){
					pseudo = mksobj(SPE_LEVITATION, MKOBJ_NOINIT);
					pseudo->blessed = pseudo->cursed = 0;
					pseudo->blessed = TRUE;
					pseudo->quan = 23L;			/* do not let useup get it */
					(void) peffects(pseudo, TRUE);
					(void) peffects(pseudo, TRUE);
					(void) peffects(pseudo, TRUE);
					obfree(pseudo, (struct obj *)0);	/* now, get rid of it */
					artinstance[ART_ROD_OF_SEVEN_PARTS].RoSPflights--;
				}
				else if( (obj->spe > -5) ){
					exercise(A_WIS, TRUE);
					exercise(A_DEX, TRUE);
					pseudo = mksobj(SPE_LEVITATION, MKOBJ_NOINIT);
					pseudo->blessed = pseudo->cursed = 0;
					pseudo->blessed = TRUE;
					pseudo->quan = 23L;			/* do not let useup get it */
					(void) peffects(pseudo, TRUE);
					(void) peffects(pseudo, TRUE);
					(void) peffects(pseudo, TRUE);
					obfree(pseudo, (struct obj *)0);	/* now, get rid of it */
					obj->spe--; obj->spe--; obj->spe--; // lose three charge
					pline("Your weapon has become much more flawed!");
					artinstance[ART_ROD_OF_SEVEN_PARTS].RoSPflights = 7;
				} else pline("Your weapon rattles faintly.");
			}
			else if(strcmp(buf, "Ruat Coelum") == 0 ||
						strcmp(buf, "Ruat Coelum.") == 0 ||
						strcmp(buf, "Ruat Coelum!") == 0 ){//Ruat Coelum:  Heaven Falls.  Attack spell.  One charge.
				if( (obj->spe > 0) && throweffect()){
					int dmg;
					dmg = u.ulevel/2 + 1;
					if(u.ukrau_duration) dmg *= 1.5;
					dmg += spell_damage_bonus();
					exercise(A_WIS, TRUE);
					cc.x=u.dx;cc.y=u.dy;
					n=u.ulevel/5 + 1;
					while(n--) {
						int type = d(1,3);
						explode(u.dx, u.dy,
							elements[type], 0,
							 dmg,
							explType[type], 1);
						u.dx = cc.x+rnd(3)-2; u.dy = cc.y+rnd(3)-2;
						if (!isok(u.dx,u.dy) || !cansee(u.dx,u.dy) ||
							IS_STWALL(levl[u.dx][u.dy].typ) || u.uswallow) {
							/* Spell is reflected back to center */
								u.dx = cc.x;
								u.dy = cc.y;
						}
					}
					obj->spe--; // lose one charge
					pline("Your weapon has become more flawed.");
				} else pline("Your weapon rattles faintly.");
			}
			else if(strcmp(buf, "Fiat Justitia") == 0 ||
						strcmp(buf, "Fiat Justitia.") == 0 ||
						strcmp(buf, "Fiat Justitia!") == 0 ){//Fiat Justitia:  Let justice be done.  Slows, fears and damages target.  Two charges.
				if( (obj->spe > 1) ){
					exercise(A_WIS, TRUE);
					pseudo = mksobj(SPE_SLOW_MONSTER, MKOBJ_NOINIT);
					pseudo->blessed = pseudo->cursed = 0;
					pseudo->quan = 20L;			/* do not let useup get it */
					pseudo2 = mksobj(SPE_FORCE_BOLT, MKOBJ_NOINIT);
					pseudo2->blessed = pseudo2->cursed = 0;
					pseudo2->quan = 20L;			/* do not let useup get it */
					pseudo3 = mksobj(SPE_CAUSE_FEAR, MKOBJ_NOINIT);
					pseudo3->blessed = pseudo3->cursed = 0;
					pseudo3->quan = 20L;			/* do not let useup get it */
					if(u.ulevel > 13) pseudo3->blessed = TRUE;
					if (!getdir((char *)0)) { //Oh, getdir must set the .d_ variables below.
					    /* getdir cancelled, just do the nondirectional scroll */
						(void) seffects(pseudo3);
					}
					else if(!u.dx && !u.dy && !u.dz) {
					    if ((damage = zapyourself(pseudo2, TRUE)) != 0) {
							char buf[BUFSZ];
							Sprintf(buf, "zapped %sself with a spell", uhim());
							losehp(damage, buf, NO_KILLER_PREFIX);
							weffects(pseudo);
							(void) seffects(pseudo3);
						}
					}
					else {
						weffects(pseudo);
						weffects(pseudo2);
						(void) seffects(pseudo3);
					}
					obfree(pseudo, (struct obj *)0);	/* now, get rid of it */
					obfree(pseudo2, (struct obj *)0);	/* now, get rid of it */
					obfree(pseudo3, (struct obj *)0);	/* now, get rid of it */
					obj->spe--; obj->spe--; // lose two charge
					pline("Your weapon has become more flawed!");
				} else pline("Your weapon rattles faintly.");
			}
			else if(strcmp(buf, "Lex Rex") == 0 ||
						strcmp(buf, "Lex Rex.") == 0 ||
						strcmp(buf, "Lex Rex!") == 0 ){//Lex Rex:  Law is King.  Heals aflictions, removes curses.  Three charges.
				if( (obj->spe > 2) ){
					exercise(A_WIS, TRUE);
					pseudo = mksobj(SPE_REMOVE_CURSE, MKOBJ_NOINIT);
					pseudo->blessed = pseudo->cursed = 0;
					pseudo->quan = 20L;			/* do not let useup get it */
					if(u.ulevel > 13) pseudo->blessed = TRUE;
					(void) seffects(pseudo);
					if (Sick) You("are no longer ill.");
					if (Slimed) {
						pline_The("slime disappears!");
						Slimed = 0;
					 /* flags.botl = 1; -- healup() handles this */
					}
					healup(u.ulevel, 0, TRUE, TRUE);
					obfree(pseudo, (struct obj *)0);	/* now, get rid of it */
					obj->spe--; obj->spe--; obj->spe--; // lose three charge
					pline("Your weapon has become much more flawed!");
				} else pline("Your weapon rattles faintly.");
			}
			else if(strcmp(buf, "Ecce! Lex Rex") == 0 ||
						strcmp(buf, "Ecce! Lex Rex.") == 0 ||
						strcmp(buf, "Ecce! Lex Rex!") == 0 ||
						strcmp(buf, "Ecce!  Lex Rex") == 0 ||
						strcmp(buf, "Ecce!  Lex Rex.") == 0 ||
						strcmp(buf, "Ecce!  Lex Rex!") == 0){//Ecce!  Lex Rex:  See!  Law is King.  Charms monsters.  Five charges.
				if( (obj->spe > 4) ){
					pseudo = mksobj(SPE_CHARM_MONSTER, MKOBJ_NOINIT);
					pseudo->blessed = pseudo->cursed = 0;
					pseudo->quan = 20L;			/* do not let useup get it */
					if(u.ulevel > 13) pseudo->blessed = TRUE;
					(void) seffects(pseudo);
					obfree(pseudo, (struct obj *)0);	/* now, get rid of it */
					obj->spe = obj->spe - 5;// lose five charge
					pline("Your weapon rattles alarmingly.  It has become much more flawed!");
					wake_nearto_noisy(u.ux, u.uy, 7);
				} else pline("Your weapon rattles faintly.");
			}
			else if(strcmp(buf, "Ruat Coelum, Fiat Justitia") == 0 ||
						strcmp(buf, "Ruat Coelum, Fiat Justitia.") == 0 ||
						strcmp(buf, "Ruat Coelum, Fiat Justitia!") == 0){//Ruat Coelum, Fiat Justitia:  Though heaven falls, let justice be done.  Calls pets.  Seven charges.
				if( (obj->spe > 6) ){
					exercise(A_WIS, TRUE);
					n=u.ulevel/5 + 1;
					cast_protection();
					if(!DimensionalLock) while(n--) {
						pm = &mons[summons[d(1,7)]];
						mtmp = makemon(pm, u.ux, u.uy, MM_ESUM|MM_EDOG|MM_ADJACENTOK|NO_MINVENT|MM_NOCOUNTBIRTH);
						if(mtmp){
							initedog(mtmp);
							mark_mon_as_summoned(mtmp, &youmonst, ESUMMON_PERMANENT, 0);
							if(u.ulevel > 12) mtmp->m_lev += u.ulevel / 3;
							mtmp->mhpmax = (mtmp->m_lev * hd_size(mtmp->data)) - hd_size(mtmp->data)/2;
							mtmp->mhp =  mtmp->mhpmax;
							mtmp->mtame = 10;
							mtmp->mpeaceful = 1;
						}
					}
					obj->spe = obj->spe - 7;// lose seven charge
					pline("Your weapon rattles alarmingly!  It has become exceedingly flawed!");
					wake_nearto_noisy(u.ux, u.uy, 21);
				} else pline("Your weapon rattles faintly.");
			}
			else if(strcmp(buf, "Ruat Coelum, Fiat Justitia.  Ecce!  Lex Rex!") == 0 ||
						strcmp(buf, "Ruat Coelum, Fiat Justitia. Ecce! Lex Rex!") == 0){//Ruat Coelum, Fiat Justitia.  Ecce!  Lex Rex!:  Though heaven falls, let justice be done.  See!  Law is King!  Heals you fully, calls pets, casts charm monster.
				if( (obj->spe > 6) ){
					exercise(A_WIS, TRUE);
					n=u.ulevel/5 + 1;
					if (Sick) You("are no longer ill.");
					if (Slimed) {
						pline_The("slime disappears!");
						Slimed = 0;
					 /* flags.botl = 1; -- healup() handles this */
					}
					healup(maybe_polyd(u.mhmax - u.mh, u.uhpmax - u.uhp), 0, TRUE, TRUE); //heal spell
					if(!DimensionalLock) while(n--) {
						pm = &mons[summons[d(1,6)+3]];
						mtmp = makemon(pm, u.ux, u.uy, MM_ESUM|MM_EDOG|MM_ADJACENTOK);
						if(mtmp){
							initedog(mtmp);
							mark_mon_as_summoned(mtmp, &youmonst, ESUMMON_PERMANENT, 0);
							mtmp->m_lev += 7;
							if(u.ulevel > 12) mtmp->m_lev += u.ulevel / 3;
							mtmp->mhpmax = (mtmp->m_lev * hd_size(mtmp->data)) - hd_size(mtmp->data)/2;
							mtmp->mhp =  mtmp->mhpmax;
						}
					}
					pseudo = mksobj(SCR_TAMING, MKOBJ_NOINIT);
					pseudo->blessed = pseudo->cursed = 0;
					pseudo->forceconf = TRUE;
					pseudo->quan = 20L;			/* do not let useup get it */
					if(u.ulevel > 13) pseudo->blessed = TRUE;
					(void) seffects(pseudo);
					obfree(pseudo, (struct obj *)0);	/* now, get rid of it */
					artinstance[ART_ROD_OF_SEVEN_PARTS].RoSPflights = 7; /* Max out your flight gauge */
					obj->spe = obj->spe - 14;// lose fourteen charge
					pline("Your weapon rattles alarmingly!!  It has become much more flawed!");
					wake_nearto_noisy(u.ux, u.uy, 77);
				} else pline("Your weapon rattles faintly.");
			}
			else {
				You_feel("foolish.");
				exercise(A_WIS, FALSE);
				if( (d(1,20)-10) > 0 ){
					obj->spe--;//lose charge for false invoke
					exercise(A_WIS, FALSE);
					pline("Your weapon has become more flawed.");
				}
				else pline("Your weapon rattles warningly.");
			}
		break;
		case WIND_PETS:
			pline("You call upon the minions of Quetzalcoatl!");
			int n = u.ulevel/5 + 1;
			if(dog_limit() < n) n = dog_limit();

			if(DimensionalLock){
				pline("%s", nothing_happens);
				obj->age = 0;
				return MOVE_CANCELLED;
			}

			pline("Where do you want to summon minions?");
			cc.x = u.ux;
			cc.y = u.uy;
			if (getpos(&cc, TRUE, "the desired position") < 0) {
				pline1(Never_mind);
				obj->age = 0;
				return MOVE_CANCELLED;
			}
			if (!cansee(cc.x, cc.y) || distu(cc.x, cc.y) >= 32) {
				You("feel a light breeze.");
				obj->age = 0;
				return MOVE_INSTANT;
			}
			else while(n--) {
				pm = &mons[windpets[d(1,8)]];
				mtmp = makemon(pm, cc.x, cc.y, MM_EDOG|MM_ESUM|MM_ADJACENTOK);
				if(mtmp){
					initedog(mtmp);
					mark_mon_as_summoned(mtmp, &youmonst, ESUMMON_PERMANENT, 0);
					mtmp->m_lev += 7;
					if(u.ulevel > 12) mtmp->m_lev += u.ulevel / 3;
					mtmp->mhpmax = (mtmp->m_lev * hd_size(mtmp->data)) - hd_size(mtmp->data)/2;
					mtmp->mhp = mtmp->mhpmax;
				}
			}
		break;
		case DEATH_TCH:
			if (!(uwep && uwep == obj)){
				You_feel("that you should be wielding %s.", the(xname(obj)));;
				obj->age = monstermoves;
				return MOVE_CANCELLED;
			}
			getdir((char *)0);
			
			if ((mtmp = m_at(u.ux + u.dx, u.uy + u.dy))) {
				/* message */
				pline("You reach out and stab at %s very soul.", s_suffix(mon_nam(mtmp)));
				/* nonliving, demons, angels are immune */
				if (nonliving(mtmp->data) || is_demon(mtmp->data) || is_angel(mtmp->data))
					pline("... but %s seems to lack one!", mon_nam(mtmp));
				/* circle of acheron provides protection */
				else if (ward_at(mtmp->mx, mtmp->my) == CIRCLE_OF_ACHERON)
					pline("But %s is already beyond Acheron.", mon_nam(mtmp));
				else
					xkilled(mtmp, 1);
				setmangry(mtmp);
			}
			else {
				/* reset timeout; we didn't actually use the invoke */
				obj->age = monstermoves;
			}
		break;
		case SKELETAL_MINION:
		{
			if ((!uwep && uwep == obj)){
				You_feel("that you should be wielding %s.", the(xname(obj)));;
				obj->age = monstermoves;
				return MOVE_CANCELLED;
			}
			char food_types[] = { FOOD_CLASS, 0 };
			struct obj *otmp = (struct obj *) 0;
			struct obj *corpse = (struct obj *) 0;
			boolean onfloor = FALSE;
			char qbuf[QBUFSZ];
			char c;
			int lvl;
	
			for (otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere) {
				if(otmp->otyp==CORPSE) {
		
					Sprintf(qbuf, "There %s %s here; create a skeletal minion of %s?",
						otense(otmp, "are"),
						doname(otmp),
						(otmp->quan == 1L) ? "it" : "one");
					if((c = yn_function(qbuf,ynqchars,'n')) == 'y'){
						corpse = otmp;
						onfloor = TRUE;
						break;
					}
					else if(c == 'q')
						break;
				}
			}
			if (!corpse) corpse = getobj(food_types, "make a skeletal minion of");
			if (!corpse) {
				obj->age = monstermoves;
				return MOVE_CANCELLED;
			}

			struct permonst *pm = &mons[corpse->corpsenm];

			if (!(humanoid(pm) || is_insectoid(pm) || is_arachnid(pm) || is_bird(pm) || 
				(((pm)->mflagsa & MA_ANIMAL) != 0L) || (((pm)->mflagsa & MA_REPTILIAN) != 0L)
			) || (is_clockwork(pm) || is_unalive(pm))){
				pline("That creature has no bones!");
				obj->age = monstermoves;
				return MOVE_CANCELLED;
			} else if (undiffed_innards(pm) || no_innards(pm)){
				pline("That creature has no bones!");
				obj->age = monstermoves;
				return MOVE_CANCELLED;
			}
			if (is_mind_flayer(pm)){
				//Ceremorphosis works on a wide variety of hostes, however, it is typically only done to human-sized creatures.
				int prob = rnd(100);
				if(prob > 50){
					pm = &mons[PM_HUMAN];
				}
				else if(prob > 25)
					pm = &mons[PM_DROW];
				else if(prob > 15)
					pm = &mons[PM_GITHYANKI_PIRATE];
				else if(prob > 5)
					pm = &mons[PM_ELF];
				else if(prob > 2)
					pm = &mons[PM_MORDOR_ORC];
				else
					pm = &mons[PM_URUK_HAI];
			}
			
			struct monst *mtmp = makemon(pm, u.ux, u.uy, MM_EDOG|MM_ADJACENTOK);
			initedog(mtmp);
			if (!skeleton_innards(mtmp->data))
				set_template(mtmp, SKELIFIED);

			if (onfloor) useupf(corpse, 1);
			else useup(corpse);

		break;
		}
		case PETMASTER:{
			int pet_effect = 0;
			if(uarm && uarm == obj && yn("Take something out of your pockets?") == 'y'){
				pet_effect = dopetmenu("Take what out of your pockets?", obj);
				switch(pet_effect){
					case 0:
					break;
					case SELECT_WHISTLE:
					otmp = mksobj(MAGIC_WHISTLE, NO_MKOBJ_FLAGS);
					otmp->blessed = obj->blessed;
					otmp->cursed = obj->cursed;
					otmp->bknown = obj->bknown;
					hold_another_object(otmp, "You fumble and %s.",
				       aobjnam(otmp, "fall"), (const char *)0);
					break;
					case SELECT_LEASH:
					otmp = mksobj(LEASH, NO_MKOBJ_FLAGS);
					otmp->blessed = obj->blessed;
					otmp->cursed = obj->cursed;
					otmp->bknown = obj->bknown;
					hold_another_object(otmp, "You fumble and %s.",
				       aobjnam(otmp, "fall"), (const char *)0);
					break;
					case SELECT_SADDLE:
					otmp = mksobj(SADDLE, NO_MKOBJ_FLAGS);
					otmp->blessed = obj->blessed;
					otmp->cursed = obj->cursed;
					otmp->bknown = obj->bknown;
					hold_another_object(otmp, "You fumble and %s.",
				       aobjnam(otmp, "fall"), (const char *)0);
					break;
					case SELECT_TRIPE:
					otmp = mksobj(TRIPE_RATION, NO_MKOBJ_FLAGS);
					otmp->blessed = obj->blessed;
					otmp->cursed = obj->cursed;
					otmp->bknown = obj->bknown;
					hold_another_object(otmp, "You fumble and %s.",
				       aobjnam(otmp, "fall"), (const char *)0);
					break;
					case SELECT_APPLE:
					otmp = mksobj(APPLE, NO_MKOBJ_FLAGS);
					otmp->blessed = obj->blessed;
					otmp->cursed = obj->cursed;
					otmp->bknown = obj->bknown;
					hold_another_object(otmp, "You fumble and %s.",
				       aobjnam(otmp, "fall"), (const char *)0);
					break;
					case SELECT_BANANA:
					otmp = mksobj(BANANA, NO_MKOBJ_FLAGS);
					otmp->blessed = obj->blessed;
					otmp->cursed = obj->cursed;
					otmp->bknown = obj->bknown;
					hold_another_object(otmp, "You fumble and %s.",
				       aobjnam(otmp, "fall"), (const char *)0);
					break;
				}
			}else if(uarm && uarm == obj && yn("Restore discipline?") == 'y'){
				(void) pet_detect_and_tame(obj);
			}else{
				if(!(uarm && uarm == obj)) You_feel("that you should be wearing the %s", xname(obj));
				obj->age = 0; //didn't invoke
			}
		} break;
		case NECRONOMICON:
		   if(yn("Open the Necronomicon?")=='y'){
			if(Blind){
				You_cant("feel any Braille writing.");
		break;
			}
			if(!u.uevent.uread_necronomicon){
				if(obj->ovar1_necronomicon) You("find notes scribbled the margins!  These will come in handy!");
				u.uevent.uread_necronomicon = 1;
				livelog_write_string("read the necronomicon for the first time");
			}
			if(!obj->known){
				discover_artifact(ART_NECRONOMICON);
				identify(obj);
				update_inventory();
			}
			if(obj->ovar1_necronomicon && yn("Read a known incantation?") == 'y'){
				int booktype = 0;
				u.uconduct.literate++;
				necro_effect = donecromenu("Choose which incantation to read", obj);
				//first switch sets the delay if a new effect, or finds the spellbook if a spell effect
				switch(necro_effect){
					case 0:
						delay = 0;
					break;
					case SELECT_BYAKHEE:
						delay = -2;
					break;
					case SELECT_NIGHTGAUNT:
						delay = -3;
					break;
					case SELECT_SHOGGOTH:
						delay = -2;
					break;
					case SELECT_OOZE:
						delay = -1;
					break;
					case SELECT_DEMON:
						delay = -6;
					break;
					case SELECT_DEVIL:
						delay = -9;
					break;
					case SELECT_PROTECTION:
						booktype = SPE_PROTECTION;
					break;
					case SELECT_TURN_UNDEAD:
						booktype = SPE_TURN_UNDEAD;
					break;
					case SELECT_FORCE_BOLT:
						booktype = SPE_FORCE_BOLT;
					break;
					case SELECT_DRAIN_LIFE:
						booktype = SPE_DRAIN_LIFE;
					break;
					case SELECT_DEATH:
						booktype = SPE_FINGER_OF_DEATH;
					break;
					case SELECT_DETECT_MNSTR:
						booktype = SPE_DETECT_MONSTERS;
					break;
					case SELECT_CLAIRVOYANCE:
						booktype = SPE_CLAIRVOYANCE;
					break;
					case SELECT_DETECT_UNSN:
						booktype = SPE_DETECT_UNSEEN;
					break;
					case SELECT_IDENTIFY:
						booktype = SPE_IDENTIFY;
					break;
					case SELECT_CONFUSE:
						booktype = SPE_CONFUSE_MONSTER;
					break;
					case SELECT_CAUSE_FEAR:
						booktype = SPE_CAUSE_FEAR;
					break;
					case SELECT_LEVITATION:
						booktype = SPE_LEVITATION;
					break;
					case SELECT_STONE_FLESH:
						booktype = SPE_STONE_TO_FLESH;
					break;
					case SELECT_CANCELLATION:
						booktype = SPE_CANCELLATION;
					break;
					case SELECT_COMBAT:
						delay = -100;
					break;
					case SELECT_HEALTH:
						delay = 0;
					break;
					case SELECT_SIGN:
						delay = -100;
					break;
					case SELECT_WARDS:
						delay = -60;
					break;
					case SELECT_ELEMENTS:
						delay = -100;
					break;
					case SELECT_SPIRITS1:
						delay = -125;
					break;
					case SELECT_SPIRITS2:
						delay = -125;
					break;
				}
				//if it WAS a spellbook effect, set the delay here
				if(booktype) switch (objects[booktype].oc_level) {
				 case 1:
				 case 2:
					delay = -objects[booktype].oc_delay;
					break;
				 case 3:
				 case 4:
					delay = -(objects[booktype].oc_level - 1) *
						objects[booktype].oc_delay;
					break;
				 case 5:
				 case 6:
					delay = -objects[booktype].oc_level *
						objects[booktype].oc_delay;
					break;
				 case 7:
					delay = -8 * objects[booktype].oc_delay;
					break;
				 default:
					impossible("Unknown spellbook level %d, book %d;",
						objects[booktype].oc_level, booktype);
					return MOVE_CANCELLED;
				}
				artiptr = obj;
				set_occupation(read_necro, "studying", 0);
			}
			else if(yn(obj->ovar1_necronomicon ? "Study the parts you don't yet understand?" : "Study the text?") == 'y'){
				u.uconduct.literate++;
				You("struggle to understand the mad scrawlings of Abdul Alhazred and the horrid, rushed annotations of those who came after him.");
				necro_effect = SELECT_STUDY;
				delay = -100;
				artiptr = obj;
				set_occupation(read_necro, "studying", 0);
				exercise(A_WIS, FALSE);
			}
			else{
				if(Hallucination) pline("The strange symbols stare at you reproachfully.");
				You("close the Necronomicon.");
			}
		   }
		   else{
				Hallucination ? 
					pline("Cool it, Link.  It's just a book.") : 
					You("hold the Necronomicon awkwardly, then put it away.");
		   }
		break;
		case SPIRITNAMES:{
		   if(yn("Open the Book of Lost Names?")=='y'){
			if(Blind){
				You_cant("feel any Braille writing.");
		break;
			}
			if(!obj->known){
				discover_artifact(ART_BOOK_OF_LOST_NAMES);
				identify(obj);
				update_inventory();
			}
			if(obj->ovar1_lostNames && yn("Contact a known spirit?") == 'y'){
				long yourseals = u.sealsKnown;
				long yourspecial = u.specialSealsKnown;
				u.sealsKnown = obj->ovar1_lostNames;
				u.specialSealsKnown = 0;
				u.uconduct.literate++;
				lostname = pick_seal("Bind spirit:");
				u.sealsKnown = yourseals;
				u.specialSealsKnown = yourspecial;
				if(!lostname) break;
				delay = -25;
				artiptr = obj;
				set_occupation(read_lost, "studying", 0);
			}
			else if(yn("Risk your name amongst the Lost?") == 'y'){
				u.uconduct.literate++;
				You("open your mind to the cold winds of the Void.");
				lostname = QUEST_SPIRITS;
				delay = -125;
				artiptr = obj;
				set_occupation(read_lost, "studying", 0);
				exercise(A_WIS, FALSE);
			}
			else{
				if(Hallucination) pline("The whisperers berate you ceaselessly.");
				You("close the Book of Lost Names.");
			}
		   }
		   else{
				Hallucination ? 
					pline("Cool it, Link.  It's just a book.") : 
					You("hold the Book of Lost Names awkwardly, then put it away.");
		   }
		}break;
		case INFINITESPELLS:{
			boolean turnpage = yn("Turn to a new page?") == 'y';
			if(!turnpage){
				pline("The endless pages of the book cover the material of a spellbook of %s in exhaustive detail.",OBJ_NAME(objects[obj->ovar1_infinitespells]));
				pline("Following the instructions on the pages, you cast the spell!");
				spelleffects(0,FALSE,obj->ovar1_infinitespells);
			}
			if(turnpage || !rn2(20)){
				obj->ovar1_infinitespells = rn2(SPE_BLANK_PAPER - SPE_DIG) + SPE_DIG;
				pline("The endless pages of the book turn themselves. They settle on a section describing %s.",OBJ_NAME(objects[obj->ovar1_infinitespells]));
			}
		}break;
		case ALTMODE:
			if(obj->oartifact == ART_INFINITY_S_MIRRORED_ARC){
				if(obj->altmode)
					You("slide the tangled mirrored arcs around (and through?) each-other, closing off the second beam-path.");
				else
					You("slide the tangled mirrored arcs around (and through?) each-other, opening the second beam-path.");
			}
			obj->altmode = !obj->altmode;
			obj->age = monstermoves;
		break;
		case LORDLY:
			if(uwep && uwep == obj){
				
				int lordlydictum;
				
				if (obj->oartifact == ART_ROD_OF_LORDLY_MIGHT ||
						obj->oartifact == ART_ROD_OF_THE_ELVISH_LORDS ||
						obj->oartifact == ART_SCEPTRE_OF_LOLTH){
					
					if (flags.female)
						lordlydictum = dolordsmenu("What is your command, my Lady?", obj);
					else
						lordlydictum = dolordsmenu("What is your command, my Lord?", obj);
				
				} else {
					lordlydictum = dolordsmenu("What is your command?", obj);
				}
				
				
				switch(lordlydictum){
					case 0:
					break;
					/*These effects can be used at any time*/
					case COMMAND_RAPIER:
						uwep->otyp = RAPIER;
					break;
					case COMMAND_AXE:
						uwep->otyp = AXE;
					break;
					case COMMAND_MACE:
						uwep->otyp = MACE;
					break;
					case COMMAND_SPEAR:
						uwep->otyp = SPEAR;
					break;
					case COMMAND_LANCE:
						uwep->otyp = LANCE;
					break;
					/* Sceptre of Lolth*/
					case COMMAND_D_GREAT:
						uwep->otyp = DROVEN_GREATSWORD;
					break;
					case COMMAND_MOON_AXE:
						uwep->otyp = MOON_AXE;
						uwep->ovar1_moonPhase = ECLIPSE_MOON;
					break;
					case COMMAND_KHAKKHARA:
						uwep->otyp = KHAKKHARA;
					break;
					case COMMAND_DROVEN_SPEAR:
						uwep->otyp = DROVEN_SPEAR;
					break;
					case COMMAND_D_LANCE:
						uwep->otyp = DROVEN_LANCE;
					break;
					/* Rod of the Elvish Lords*/
					case COMMAND_E_SWORD:
						uwep->otyp = ELVEN_BROADSWORD;
					break;
					case COMMAND_E_SICKLE:
						uwep->otyp = ELVEN_SICKLE;
					break;
					case COMMAND_E_MACE:
						uwep->otyp = ELVEN_MACE;
					break;
					case COMMAND_E_SPEAR:
						uwep->otyp = ELVEN_SPEAR;
					break;
					case COMMAND_E_LANCE:
						uwep->otyp = ELVEN_LANCE;
					break;
					case COMMAND_SCIMITAR:
						uwep->otyp = SCIMITAR;
					break;
					case COMMAND_WHIP:
						uwep->otyp = BULLWHIP;
						if(uwep->oartifact == ART_XIUHCOATL && uwep->obj_material == WOOD)
							set_material_gm(uwep, artilist[uwep->oartifact].material);
					break;
					case COMMAND_ATLATL:
						uwep->otyp = ATLATL;
						if(uwep->oartifact == ART_XIUHCOATL && uwep->obj_material == artilist[uwep->oartifact].material)
							set_material_gm(uwep, WOOD);
					break;
					/*These effects are limited by timeout*/
					case COMMAND_LADDER:
						if(u.uswallow){
							mtmp = u.ustuck;
							if (!is_whirly(mtmp->data)) {
								if (is_animal(mtmp->data))
									pline("The %s quickly lengthens and pierces %s %s wall!",
									obj->oartifact == ART_SCEPTRE_OF_LOLTH ? "Sceptre" : "Rod", 
									s_suffix(mon_nam(mtmp)), mbodypart(mtmp, STOMACH));
								if(mtmp->mtyp == PM_JUIBLEX 
									|| mtmp->mtyp == PM_LEVIATHAN
									|| mtmp->mtyp == PM_METROID_QUEEN
								) mtmp->mhp = (int)(.75*mtmp->mhp + 1);
								else mtmp->mhp = 1;		/* almost dead */
								expels(mtmp, mtmp->data, !is_animal(mtmp->data));
								setmangry(mtmp);
							} else{
								pline("The %s quickly lengthens and pierces %s %s",
									obj->oartifact == ART_SCEPTRE_OF_LOLTH ? "Sceptre" : "Rod", 
									s_suffix(mon_nam(mtmp)), mbodypart(mtmp, STOMACH));
								pline("However, %s is unfazed", mon_nam(mtmp));
							}
					break;
						} else if(!u.uhave.amulet){
							if(Can_rise_up(u.ux, u.uy, &u.uz)) {
								int newlev = depth(&u.uz)-1;
								d_level newlevel;
								
								pline("The %s extends quickly, reaching for the %s",
									obj->oartifact == ART_SCEPTRE_OF_LOLTH ? "Sceptre" : "Rod", 
									ceiling(u.ux,u.uy));
								
								get_level(&newlevel, newlev);
								if(on_level(&newlevel, &u.uz)) {
									pline("However, it is unable to pierce the %s.",ceiling(u.ux,u.uy));
					break;
								} else pline("The %s obediently yields before the %s, and you climb to the level above.",
											ceiling(u.ux,u.uy), obj->oartifact == ART_SCEPTRE_OF_LOLTH ? "Sceptre" : "Rod");
								goto_level(&newlevel, FALSE, FALSE, FALSE);
							}
						} else{
							if (!Weightless && !Is_waterlevel(&u.uz) && !Underwater) {
								pline("The %s begins to extend quickly upwards.", obj->oartifact == ART_SCEPTRE_OF_LOLTH ? "Sceptre" : "Rod");
								pline("However, a mysterious force slams it back into the %s below.", surface(u.ux, u.uy));
								watch_dig((struct monst *)0, u.ux, u.uy, TRUE);
								(void) dighole(FALSE);
							}
						}
					break;
					case COMMAND_CLAIRVOYANCE:
						do_vicinity_map(u.ux,u.uy); /*Note that this is not blocked by pointy hats*/
					break;
					case COMMAND_FEAR:
						You("thrust the %s into the %s, that all may know of your %s.", 
							obj->oartifact == ART_SCEPTRE_OF_LOLTH ? "Sceptre" : "Rod", 
							Underwater ? "water" : "air",
							obj->oartifact == ART_ROD_OF_LORDLY_MIGHT ? "Might" : "majesty");
						for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
							if (DEADMONSTER(mtmp)) continue;
							if(!is_blind(mtmp) && couldsee(mtmp->mx,mtmp->my)) {
//								if (! resist(mtmp, sobj->oclass, 0, NOTELL))
								monflee(mtmp, 0, FALSE, FALSE);
							}
						}
					break;
					case COMMAND_LIFE:
						if(!getdir((char *)0) ||
							(!u.dx && !u.dy) ||
							((mtmp = m_at(u.ux+u.dx,u.uy+u.dy)) == 0)
						){
							pline("The %s glows and then fades.", obj->oartifact == ART_SCEPTRE_OF_LOLTH ? "Sceptre" : "Rod");
						} else {
							int dmg = d(2,hd_size(mtmp->data));
							if (resists_drli(mtmp)){
								shieldeff(mtmp->mx, mtmp->my);
					break;
							} else {
								mtmp->mhp -= 2*dmg;
								mtmp->mhpmax -= dmg;
								if(mtmp->m_lev < 2)
									mtmp->m_lev = 0;
								else mtmp->m_lev -= 2;
								if (mtmp->mhp <= 0 || mtmp->mhpmax <= 0 || mtmp->m_lev < 1)
									xkilled(mtmp, 1);
								else {
								if (canseemon(mtmp))
									pline("%s suddenly seems weaker!", Monnam(mtmp));
								}
								setmangry(mtmp);
								healup(2*dmg, 0, FALSE, TRUE);
								You_feel("better.");
							}
						}
					break;
					case COMMAND_STRIKE:
						if(!getdir((char *)0) ||
							(!u.dx && !u.dy) ||
							((mtmp = m_at(u.ux+u.dx,u.uy+u.dy)) == 0)
						){
							You("swing wildly.");
						} else {
							int dmg = d(6,6);
							if (resists_poison(mtmp)){
								shieldeff(mtmp->mx, mtmp->my);
								setmangry(mtmp);
								break;
							} else {
								if(!rn2(10)) dmg += mtmp->mhp;
								mtmp->mhp -= dmg;
								mtmp->mhpmax -= dmg;
								if(mtmp->m_lev < (int)(dmg/4.5))
									mtmp->m_lev = 0;
								else mtmp->m_lev -= (int)(dmg/4.5);
								if (mtmp->mhp <= 0 || mtmp->mhpmax <= 0 || mtmp->m_lev < 1)
									xkilled(mtmp, 1);
								else {
									if (canseemon(mtmp))
										pline("%s suddenly seems weaker!", Monnam(mtmp));
								}
								setmangry(mtmp);
							}
						}
					break;
					case COMMAND_KNEEL:
						if(!getdir((char *)0) ||
							(!u.dx && !u.dy) ||
							((mtmp = m_at(u.ux+u.dx,u.uy+u.dy)) == 0)
						){
							pline("The %s glows and then fades.", obj->oartifact == ART_SCEPTRE_OF_LOLTH ? "Sceptre" : "Rod");
						} else{
							mtmp->mcanmove = 0;
							mtmp->mfrozen = max(1, u.ulevel - ((int)(mtmp->m_lev)));
							pline("%s kneels before you.",Monnam(mtmp));
						}
					break;
					case COMMAND_AMMO:
						otmp = mksobj(JAVELIN, NO_MKOBJ_FLAGS);
						if (!otmp) break;
						otmp->blessed = obj->blessed;
						otmp->cursed = obj->cursed;

						otmp->bknown |= obj->bknown;
						otmp->known |= obj->known;
						otmp->rknown |= obj->rknown;
						otmp->dknown |= obj->dknown;
						otmp->sknown |= obj->sknown;
						if (obj->blessed) {
							if (otmp->spe < 0) otmp->spe = 0;
							otmp->quan += rnd(10);
						} else if (obj->cursed) {
							if (otmp->spe > 0) otmp->spe = 0;
						} else otmp->quan += rnd(5);
						otmp->owt = weight(otmp);
						otmp = hold_another_object(otmp, "Suddenly %s out.", 
							aobjnam(otmp, "fall"), (const char *)0);
					break;
					default:
						pline("What is this strange command!?");
					break;
				}
				if(lordlydictum >= COMMAND_LADDER) obj->age = monstermoves + (long)(rnz(100)*(Role_if(PM_PRIEST) ? .8 : 1)); 
			} else You_feel("that you should be wielding %s.", the(xname(obj)));;
		break;
		case ANNUL:
			if(uwep && uwep == obj){
				//struct obj *otmp;
				int annulusFunc = doannulmenu("Select function.", obj);
				switch(annulusFunc){
					case 0:
					break;
					/*These effects can be used at any time*/
					case COMMAND_SABER:
						uwep->oclass = TOOL_CLASS;
						uwep->altmode = FALSE;
						uwep->otyp = LIGHTSABER;
					break;
					// case COMMAND_ARM:
						// if(uwep->lamplit) lightsaber_deactivate(uwep,TRUE);
						// uwep->otyp = ARM_BLASTER;
					// break;
					// case COMMAND_RAY:
						// if(uwep->lamplit) lightsaber_deactivate(uwep,TRUE);
						// uwep->otyp = RAYGUN;
					// break;
					case COMMAND_RING:
						if(uwep->lamplit) lightsaber_deactivate(uwep,TRUE);
						uwep->oclass = RING_CLASS;
						uwep->altmode = FALSE;
						uwep->otyp = find_sring();
					break;
					case COMMAND_KHAKKHARA:
						if(uwep->lamplit) lightsaber_deactivate(uwep,TRUE);
						uwep->oclass = WEAPON_CLASS;
						uwep->altmode = FALSE;
						uwep->otyp = KHAKKHARA;
					break;
					case COMMAND_BFG:
						if(uwep->lamplit) lightsaber_deactivate(uwep,TRUE);
						uwep->oclass = WEAPON_CLASS;
						uwep->altmode = WP_MODE_BURST;
						uwep->otyp = BFG;
					break;
					case COMMAND_ANNULUS:
						if(uwep->lamplit) lightsaber_deactivate(uwep,TRUE);
						uwep->oclass = WEAPON_CLASS;
						uwep->altmode = FALSE;
						uwep->otyp = CHAKRAM;
					break;
					/*These effects are limited by timeout*/
					case COMMAND_BELL:{
						boolean invoking = invocation_pos(u.ux, u.uy) && !On_stairs(u.ux, u.uy);
						You("ring %s.", the(xname(obj)));
						
						if (Underwater) {
#ifdef	AMIGA
							amii_speaker( obj, "AhDhGqEqDhEhAqDqFhGw", AMII_MUFFLED_VOLUME );
#endif
							pline("But the sound is muffled.");
						}
						/* charged Bell of Opening */
						if (u.uswallow) {
							if (!obj->cursed)
								(void) openit();
							else
								pline("%s", nothing_happens);
						} else if (obj->cursed) {
							coord mm;

							mm.x = u.ux;
							mm.y = u.uy;
							pline("Graves open around you...");
							mkundead(&mm, FALSE, NO_MINVENT, 0);
							wake_nearby_noisy();

						} else  if (invoking) {
							pline("%s an unsettling shrill sound...",
								  Tobjnam(obj, "issue"));
#ifdef	AMIGA
							amii_speaker( obj, "aefeaefeaefeaefeaefe", AMII_LOUDER_VOLUME );
#endif
							u.rangBell = moves;
							wake_nearby_noisy();
						} else if (obj->blessed) {
							int res = 0;

#ifdef	AMIGA
							amii_speaker( obj, "ahahahDhEhCw", AMII_SOFT_VOLUME );
#endif
							if (uchain) {
								unpunish();
								res = 1;
							}
							res += openit();
							switch (res) {
							  case 0:  pline("%s", nothing_happens); break;
							  case 1:  pline("%s opens...", Something); break;
							  default: pline("Things open around you..."); break;
							}

						} else {  /* uncursed */
#ifdef	AMIGA
							amii_speaker( obj, "AeFeaeFeAefegw", AMII_OKAY_VOLUME );
#endif
							if (findit() == 0) pline("%s", nothing_happens);
						}
						/* charged BofO */
					}break;
					case COMMAND_BULLETS:{
						otmp = mksobj(SILVER_BULLET, NO_MKOBJ_FLAGS);
						otmp->blessed = obj->blessed;
						otmp->cursed = obj->cursed;
						otmp->bknown |= obj->bknown;
						otmp->known |= obj->known;
						otmp->rknown |= obj->rknown;
						otmp->dknown |= obj->dknown;
						otmp->sknown |= obj->sknown;
						if (obj->blessed) {
							if (otmp->spe < 0) otmp->spe = 0;
							otmp->quan += 10+rnd(10);
						} else if (obj->cursed) {
							if (otmp->spe > 0) otmp->spe = 0;
						} else
							otmp->quan += rnd(5);
						otmp->quan += 20+rnd(20);
						otmp->owt = weight(otmp);
						otmp = hold_another_object(otmp, "Suddenly %s out.",
							aobjnam(otmp, "fall"), (const char *)0);
					}break;
					case COMMAND_ROCKETS:{
						otmp = mksobj(ROCKET, NO_MKOBJ_FLAGS);
						otmp->blessed = obj->blessed;
						otmp->cursed = obj->cursed;
						otmp->bknown |= obj->bknown;
						otmp->known |= obj->known;
						otmp->rknown |= obj->rknown;
						otmp->dknown |= obj->dknown;
						otmp->sknown |= obj->sknown;
						if (obj->blessed) {
							if (otmp->spe < 0) otmp->spe = 0;
						} else if (obj->cursed) {
							if (otmp->spe > 0) otmp->spe = 0;
						}
						otmp->quan = 3+rnd(5);
						otmp->owt = weight(otmp);
						otmp = hold_another_object(otmp, "Suddenly %s out.",
							aobjnam(otmp, "fall"), (const char *)0);
					}break;
					case COMMAND_BEAM:{
						if (!getdir((char *)0)) {
							annulusFunc = 0;
							break;
						}
						otmp = mksobj(RAYGUN, NO_MKOBJ_FLAGS);
						otmp->blessed = obj->blessed;
						otmp->cursed = obj->cursed;
						otmp->bknown = obj->bknown;
						otmp->altmode = AD_DISN;
						otmp->ovar1_charges = 100;
						zap_raygun(otmp,1,0);
						obfree(otmp,(struct obj *)0);
					}break;
					case COMMAND_ANNUL:{
						if(Is_astralevel(&u.uz) && IS_ALTAR(levl[u.ux][u.uy].typ) && a_align(u.ux, u.uy) == A_LAWFUL){
							struct engr *engrHere = engr_at(u.ux,u.uy);
							pline("A column of cerulean light blasts through the center of the Annulus, striking the High Altar with intolerable force.");
							pline("The whole plane shakes, and the Altar and Annulus both explode into a rapidly-fading ring of cerulean light.");
							flags.questprogress = 2;
							levl[u.ux][u.uy].typ = CORR;
							newsym(u.ux, u.uy);
							useupall(obj);
							if(!engrHere){
								make_engr_at(u.ux, u.uy,	"", (moves - multi), DUST); /* absense of text =  dust */
								engrHere = engr_at(u.ux,u.uy); /*note: make_engr_at does not return the engraving it made, it returns void instead*/
							}
							engrHere->halu_ward = TRUE;
							engrHere->ward_id = CERULEAN_SIGN;
							engrHere->ward_type = BURN;
							engrHere->complete_wards = 1;
							return MOVE_STANDARD;
						} else {
							struct monst *mtmp = fmon, *ntmp;
							boolean maanze = FALSE;
							
							You("raise the Annulus into the %s, and it releases a rapidly-expanding ring of cerulean energy.", Underwater ? "water" : "air");
							for(; mtmp; mtmp = ntmp){
								ntmp = mtmp->nmon;
								if(mon_resistance(mtmp,TELEPAT) && couldsee(mtmp->mx,mtmp->my)){
									if(mtmp->mtyp == PM_LUGRIBOSSK) maanze = TRUE;
									killed(mtmp);
								} else if(is_magical(mtmp->data) && couldsee(mtmp->mx,mtmp->my) 
									&& !resist(mtmp, WEAPON_CLASS, 0, NOTELL)
								){
									killed(mtmp);
								} else if(!rn2(5)){
									mtmp->mhp -= d(5,15);
									if (mtmp->mhp <= 0) xkilled(mtmp, 1);
									else {
										mtmp->mstun = 1;
										mtmp->mconf = 1;
									}
									setmangry(mtmp);
								}
							}
							if(HTelepat & INTRINSIC){
								Your("inner eye is blinded by the flash!");
								HTelepat &= ~INTRINSIC;
								if (Blind && !Blind_telepat)
									see_monsters(); /* Can't sense monsters any more. */
								make_stunned(HStun + d(5,15), FALSE);
								make_confused(HConfusion + d(5,15), FALSE);
							}
							if(maanze && mvitals[PM_MAANZECORIAN].born == 0 && (mtmp = makemon(&mons[PM_MAANZECORIAN],u.ux,u.uy,MM_ADJACENTOK))){
								if (!Blind || sensemon(mtmp))
									pline("An enormous ghostly mind flayer appears next to you!");
								else You("sense a presence close by!");
								mtmp->mpeaceful = 0;
								set_malign(mtmp);
								//Assumes Maanzecorian is 27 (archon)
								mtmp->m_lev = 27;
								mtmp->mhpmax = hd_size(mtmp->data)*26 + rn2(hd_size(mtmp->data));
								mtmp->mhp = mtmp->mhpmax;
								do_clear_area(mtmp->mx,mtmp->my, 4, set_lit, (genericptr_t)0);
								do_clear_area(u.ux,u.uy, 4, set_lit, (genericptr_t)0);
								doredraw();
								if(flags.verbose) You("are frightened to death, and unable to move.");
								nomul(-4, "being frightened to death");
								nomovemsg = "You regain your composure.";
							}
						}
					}break;
					case COMMAND_CHARGE: {
						boolean b_effect;
						
						b_effect = obj->blessed &&
							((Role_switch == oart->role || oart->role == NON_PM) && (Race_switch == oart->race || oart->race == NON_PM));
						recharge(uwep, b_effect ? 1 : obj->cursed ? -1 : 0);
						update_inventory();
						break;
					  }
					break;
					default:
						pline("What is this strange function.");
					break;
				}
				if(annulusFunc >= COMMAND_ANNUL) obj->ovara_timeout = monstermoves + (long)(rnz(1000)*(Role_if(PM_PRIEST) ? .8 : 1)); 
				else if(annulusFunc >= COMMAND_BELL) obj->ovara_timeout = monstermoves + (long)(rnz(100)*(Role_if(PM_PRIEST) ? .8 : 1)); 
			} else You_feel("that you should be wielding %s.", the(xname(obj)));
		break;
		case VOID_CHIME:
			if(quest_status.killed_nemesis){
				int i;
				int * spiritprops;
				u.voidChime = 5;
				pline("You strike the twin-bladed athame like a tuning fork. The beautiful chime is like nothing you have ever heard.");
				obj->ovara_seals |= u.sealsActive;
				u.sealsActive |= obj->ovara_seals;
				set_spirit_powers(u.spiritTineA);
					if(u.spiritTineA&wis_spirits) u.wisSpirits++;
					if(u.spiritTineA&int_spirits) u.intSpirits++;
					for (i = 0, spiritprops = spirit_props(decode_sealID(u.spiritTineA)); spiritprops[i] != NO_PROP; i++)
						u.uprops[spiritprops[i]].extrinsic |= W_SPIRIT;
				set_spirit_powers(u.spiritTineB);
					if(u.spiritTineB&wis_spirits) u.wisSpirits++;
					if(u.spiritTineB&int_spirits) u.intSpirits++;
					for (i = 0, spiritprops = spirit_props(decode_sealID(u.spiritTineB)); spiritprops[i] != NO_PROP; i++)
						u.uprops[spiritprops[i]].extrinsic |= W_SPIRIT;
				for(i=0;i < NUMBER_POWERS;i++){
					u.spiritPColdowns[i] = 0;
				}
				if(obj == uwep && uwep->lamplit && artifact_light(obj)) begin_burn(uwep);
			}
			else pline("You strike the single-bladed athame, but nothing happens.");
		break;
		case DEATH_GAZE:{
			struct monst *mtmp2;
			if (u.uluck < -9) { /* uh oh... */
				pline("The Eye turns on you!");
				u.uhp = 0;
				killer_format = KILLED_BY;
				killer = "the Eye of Vecna";
				done(DIED);
			}
			pline("The Eye looks around with its icy gaze!");
			for (mtmp = fmon; mtmp; mtmp = mtmp2) {
				mtmp2 = mtmp->nmon;
				/* The eye is never blind ... */
				if (couldsee(mtmp->mx, mtmp->my) && !is_undead(mtmp->data)) {
					pline("%s screams in agony!",Monnam(mtmp));
					mtmp->mhp /= 4;
					if (mtmp->mhp < 1) mtmp->mhp = 1;
					if(!mtmp->mtame) //pets are willing to take the colateral damage
						setmangry(mtmp);
				}
			}
			/* Tsk,tsk.. */
			adjalign(-3);
			u.uluck -= 3;
	    }break;
        case CHANGE_SIZE: {
			winid tmpwin;
			anything any;
			menu_item *selected;
			int n;

			any.a_void = 0;         /* zero out all bits */
			tmpwin = create_nhwindow(NHW_MENU);
			start_menu(tmpwin);
			
			any.a_int = 1;
			add_menu(tmpwin, NO_GLYPH, &any, 't', 0, ATR_NONE, "Tiny.", MENU_UNSELECTED);
			any.a_int++;
			add_menu(tmpwin, NO_GLYPH, &any, 's', 0, ATR_NONE, "Small.", MENU_UNSELECTED);
			any.a_int++;
			add_menu(tmpwin, NO_GLYPH, &any, 'm', 0, ATR_NONE, "Medium.", MENU_UNSELECTED);
			any.a_int++;
			add_menu(tmpwin, NO_GLYPH, &any, 'l', 0, ATR_NONE, "Large.", MENU_UNSELECTED);
			any.a_int++;
			add_menu(tmpwin, NO_GLYPH, &any, 'h', 0, ATR_NONE, "Huge.", MENU_UNSELECTED);
			any.a_int++;
			add_menu(tmpwin, NO_GLYPH, &any, 'g', 0, ATR_NONE, "Gigantic.", MENU_UNSELECTED);

			end_menu(tmpwin, "Become what size?");
			n = select_menu(tmpwin, PICK_ONE, &selected);

			if(n > 0){
				n = selected[0].item.a_int;
				free(selected);
			}
			destroy_nhwindow(tmpwin);
			if(!n){
				return MOVE_CANCELLED;
			}
			
			switch(n){
				case 1:
					set_obj_size(obj, MZ_TINY);
				break;
				case 2:
					set_obj_size(obj, MZ_SMALL);
				break;
				case 3:
					set_obj_size(obj, MZ_MEDIUM);
				break;
				case 4:
					set_obj_size(obj, MZ_LARGE);
				break;
				case 5:
					set_obj_size(obj, MZ_HUGE);
				break;
				case 6:
					set_obj_size(obj, MZ_GIGANTIC);
				break;
			}
			pline("%s complies.", The(xname(obj)));
		}break;
        case SMITE: {
          if(uwep && uwep == obj){
            /*if(ugod_is_angry()){
            } else
            */
            if(!getdir((char *)0) ||
                (!u.dx && !u.dy) ||
                Misotheism ||
                ((mtmp = m_at(u.ux+u.dx,u.uy+u.dy)) == 0)){
              pline("The %s glows and then fades.", the(xname(obj)));
            } else {
              pline("Suddenly a bolt of divine energy comes down at the %s from the heavens!", mon_nam(mtmp));
              pline("It strikes the %s!", mon_nam(mtmp));

              int dmg;

              if(u.ublesscnt){
                dmg = d(2, 10);
                u.ublesscnt += rnz(350);
              } else {
                dmg = d(4,20);
                u.ublesscnt = rnz(350);
              }

              u.lastprayed = moves;
              u.uconduct.gnostic++;
              if(u.sealsActive&SEAL_AMON) unbind(SEAL_AMON,TRUE);

              mtmp->mhp -= dmg;

              if(mtmp->mhp <= 0)
                xkilled(mtmp, 1);
			  setmangry(mtmp);
            }
          } else You_feel("that you should be wielding %s.", the(xname(obj)));
        } break;
        case PROTECT: {
          if(uarmg && uarmg->oartifact == ART_GAUNTLETS_OF_THE_DIVINE_DI){
            cast_protection();
            if(!u.ublesscnt) cast_protection();
          } else You_feel("that you should be wearing %s", the(xname(obj)));
        } break;
        case TRAP_DET: {
          trap_detect(obj);
        } break;
        case UNBIND_SEALS: {
            boolean released = FALSE;
            long seal;
            for(seal = SEAL_AHAZU; seal < SEAL_SPECIAL; seal *= 2){
              if(u.sealsActive & seal){
                unbind(seal, FALSE);
                released = TRUE;
              }
            }
            if(released) You_feel("cleansed.");
        } break;
        case HEAL_PETS: {
          (void) pet_detect_and_heal(obj);
        } break;
        case FREE_SPELL: {
          if(uarmc && uarmc == obj){
            if(!docast()){
              pline("Your %s glows then fades.", The(xname(obj)));
            }
          } else You_feel("that you should be wearing %s.", The(xname(obj)));
        } break;
        case BURN_WARD: {
          if(uarms && uarms == obj){
            struct obj *scroll;
            scroll = mksobj(SCR_WARDING, NO_MKOBJ_FLAGS);
            scroll->blessed = obj->blessed;
            scroll->cursed = obj->cursed;
            seffects(scroll);
            obfree(scroll,(struct obj *)0);
          } else You_feel("that you should be wearing %s.", The(xname(obj)));
        } break;
        case FAST_TURNING: {
          if(uarmg && uarmg == obj){
            if(!obj->cursed){
              if(!getdir((char *)0))
                break;
              struct obj *wand;
              wand = mksobj(WAN_UNDEAD_TURNING, NO_MKOBJ_FLAGS);
              wand->blessed = obj->blessed;
              wand->cursed = obj->cursed;
              wand->ovar1 = 1;
              weffects(wand);
              obfree(wand,(struct obj *)0);
            } else {
              uncurse(obj);
              if(!Blind)
                pline("Your %s %s.", aobjnam(obj, "softly glow"), hcolor(NH_AMBER));
            }
          } else You_feel("that you should be wearing %s.", The(xname(obj)));
        } break;
        case FIRE_BLAST: {
          if(!getdir((char *)0))
            break;
          switch(obj->oclass){
            case POTION_CLASS:
              You("take a sip from %s, then belch out a blast of fire.", The(xname(obj)));
              break;
            case RING_CLASS:
              /* TODO players who don't have fingers but wear rings */
              You_feel("%s grow hot on your finger.", The(xname(obj)));
              break;
          }
          struct obj *wand;
          wand = mksobj(WAN_FIRE, NO_MKOBJ_FLAGS);
          wand->blessed = obj->blessed;
          wand->cursed= obj->cursed;
          wand->ovar1 = 1;
          if(wand->cursed) uncurse(wand);
          weffects(wand);
          obfree(wand,(struct obj *)0);
        } break;
        case SELF_POISON:{
          int selfpoisonFunc = doselfpoisonmenu("Pick your poison.", obj);
          switch(selfpoisonFunc){
              case 0:
                break;
              case COMMAND_POISON:
                obj->opoisoned = OPOISON_BASIC;
                pline("A poisoned coating forms on %s.", The(xname(obj)));
                break;
              case COMMAND_DRUG:
                obj->opoisoned = OPOISON_SLEEP;
                pline("A drug coating forms on %s.", The(xname(obj)));
                break;
              case COMMAND_STAIN:
                obj->opoisoned = OPOISON_BLIND;
                pline("A stained coating forms on %s.", The(xname(obj)));
                break;
              case COMMAND_ENVENOM:
                obj->opoisoned = OPOISON_PARAL;
                pline("A venomous coating forms on %s.", The(xname(obj)));
                break;
              case COMMAND_FILTH:
                obj->opoisoned = OPOISON_FILTH;
                pline("A filthy coating forms on %s.", The(xname(obj)));
				IMPURITY_UP(u.uimp_dirtiness)
                break;
          }
        } break;
        case ADD_POISON:{
          if(!(uarmh && uarmh == obj)){
            if(uwep){
              if(is_poisonable(uwep)){
                uwep->opoisoned = OPOISON_BASIC;
                pline("A poisoned coating forms your %s.", xname(uwep));
              } else pline("Interesting...");
            } else You_feel("like you should be wielding something.");
          } else You_feel("like you should be wearing %s.", The(xname(obj)));
        } break;
        case TOWEL_ITEMS:{
          if(uwep && uwep == obj){
            struct obj *otmp;
            switch(rn2(5)){
              case 0:
                otmp = mksobj(TIN, NO_MKOBJ_FLAGS);
                otmp->corpsenm = PM_LICHEN;
                break;
              case 1:
                otmp = mksobj(POT_BOOZE, NO_MKOBJ_FLAGS);
                break;
              case 2:
                otmp = mksobj(SCR_MAGIC_MAPPING, NO_MKOBJ_FLAGS);
                break;
              case 3:
                otmp = mksobj(OILSKIN_CLOAK, NO_MKOBJ_FLAGS);
                break;
              case 4:
                otmp = mksobj(CRYSTAL_HELM, NO_MKOBJ_FLAGS);
                break;
            }
            otmp->blessed = obj->blessed;
            otmp->cursed = obj->cursed;
            otmp->bknown = obj->bknown;
            otmp->owt = weight(otmp);
            otmp = hold_another_object(otmp, "Suddenly %s out.",
                           aobjnam(otmp, "fall"), (const char *)0);
          } else You_feel("like you should be wielding %s.", The(xname(obj)));
        } break;
        case MAJ_RUMOR:{
          outgmaster();
        } break;
        case ARTIFICE:{
          int artificeFunc = doartificemenu("Improve weapon or armor:", obj);
          struct obj *scroll = (struct obj *)0;
          switch(artificeFunc){
              case 0:
                break;
              case COMMAND_IMPROVE_WEP:
                scroll = mksobj(SCR_ENCHANT_WEAPON, NO_MKOBJ_FLAGS);
                break;
              case COMMAND_IMPROVE_ARM:
                scroll = mksobj(SCR_ENCHANT_ARMOR, NO_MKOBJ_FLAGS);
                break;
          }
		  if (scroll) {
			scroll->blessed = obj->blessed;
			scroll->cursed = obj->cursed;
			scroll->quan = 20;				/* do not let useup get it */
			seffects(scroll);
			obfree(scroll,(struct obj *)0);	/* now, get rid of it */
		  }
        } break;
        case SUMMON_PET:{
          /* TODO */
        } break;
        case STEAL:{
          /* TODO */
        } break;
        case COLLECT_TAX:{
          /* TODO */
        } break;
        case LIFE_DEATH:{
          if(uwep && uwep == obj){
            if(!getdir((char *)0))
              break;
            switch(obj->ovara_lifeDeath){
              case COMMAND_DEATH: {
                struct monst *mtmp;
                if((u.dx || u.dy) && (mtmp = m_at(u.ux+u.dx,u.uy+u.dy))){
                  if(!resists_death(mtmp)){
                    if(!(nonliving(mtmp->data) || is_demon(mtmp->data) || is_angel(mtmp->data))){
                      pline("%s withers under the touch of %s.", The(Monnam(mtmp)), The(xname(obj)));
                      xkilled(mtmp, 1);
                      obj->ovara_lifeDeath = COMMAND_LIFE;
                    } else {
                      pline("%s seems no deader than before.", The(Monnam(mtmp)));
                    }
					setmangry(mtmp);
                  } else {
                    pline("%s resists.", the(Monnam(mtmp)));
                  }
                } else {
                  pline("The %s glows and then fades.", the(xname(obj)));
                  break;
                }
              } break;
              case COMMAND_LIFE:{
                struct obj *ocur,*onxt;
                struct monst *mtmp;
                for(ocur = level.objects[u.ux+u.dx][u.uy+u.dy]; ocur; ocur = onxt) {
                    onxt = ocur->nexthere;
                    if (ocur->otyp != EGG){
                      revive(ocur, FALSE);
                      if((mtmp = m_at(u.ux+u.dx,u.uy+u.dy))){
                        pline("%s resurrects!", Monnam(mtmp));
                        obj->ovara_lifeDeath = COMMAND_DEATH;
                        break;
                      } else {
                        pline("You fail to resurrect %s.", the(xname(ocur)));
                      }
                    }
                }
              } break;
            }
          } else You_feel("like you should be wielding %s.", The(xname(obj)));
        } break;
        case PRISMATIC:{
          if(uarm && uarm == obj){
            int prismaticFunc  = doprismaticmenu("Choose a new color for your armor:", obj);
            setnotworn(obj);
            switch(prismaticFunc){
                case COMMAND_GRAY:
                  obj->otyp = GRAY_DRAGON_SCALE_MAIL;
                  break;
                case COMMAND_SILVER:
                  obj->otyp = SILVER_DRAGON_SCALE_MAIL;
                  break;
                case COMMAND_SHIMMERING:
                  obj->otyp = SHIMMERING_DRAGON_SCALE_MAIL;
                  break;
                case COMMAND_RED:
                  obj->otyp = RED_DRAGON_SCALE_MAIL;
                  break;
                case COMMAND_WHITE:
                  obj->otyp = WHITE_DRAGON_SCALE_MAIL;
                  break;
                case COMMAND_ORANGE:
                  obj->otyp = ORANGE_DRAGON_SCALE_MAIL;
                  break;
                case COMMAND_BLACK:
                  obj->otyp = BLACK_DRAGON_SCALE_MAIL;
                  break;
                case COMMAND_BLUE:
                  obj->otyp = BLUE_DRAGON_SCALE_MAIL;
                  break;
                case COMMAND_GREEN:
                  obj->otyp = GREEN_DRAGON_SCALE_MAIL;
                  break;
                case COMMAND_YELLOW:
                  obj->otyp = YELLOW_DRAGON_SCALE_MAIL;
                  break;
                case 0:
                  break;
            }
            setworn(obj, W_ARM);
            obj->owt = weight(obj);
          } else You_feel("like you should be wearing %s.", The(xname(obj)));
        } break;
        case SUMMON_VAMP:{
          if(uamul && uamul == obj){
            /* TODO */
          } else You_feel("like you should be wearing %s.", The(xname(obj)));
        } break;
		case SUMMON_UNDEAD:{
			int summon_loop;
			struct monst *mtmp2;
			if (u.uluck < -9) { /* uh oh... */
			u.uhp -= (rn2(20)+5);
			pline("The Hand claws you with its icy nails!");
			if (u.uhp <= 0) {
			  killer_format = KILLED_BY;
			  killer="the Hand of Vecna";
			  done(DIED);
			}
			}
			summon_loop = rn2(4) + 4;
			pline("Creatures from the grave surround you!");
			do {
			  switch (rn2(6)+1) {
			case 1: mtmp = makemon(mkclass(S_VAMPIRE,0), u.ux, u.uy, MM_ESUM);
			   break;
			case 2:
			case 3: mtmp = makemon(mkclass(S_ZOMBIE,0), u.ux, u.uy, MM_ESUM);
			   break;
			case 4: mtmp = makemon(mkclass(S_MUMMY,0), u.ux, u.uy, MM_ESUM);
			   break;
			case 5: mtmp = makemon(mkclass(S_GHOST,0), u.ux, u.uy, MM_ESUM);
			   break;
				   default: mtmp = makemon(mkclass(S_WRAITH,0), u.ux, u.uy, MM_ESUM);
			   break;
			  }
			  if ((mtmp2 = tamedog(mtmp, (struct obj *)0)) != 0){
					mtmp = mtmp2;
					mtmp->mtame = 30;
					summon_loop--;
					mark_mon_as_summoned(mtmp, &youmonst, 100, 0);
				} else mongone(mtmp);
			} while (summon_loop);
			/* Tsk,tsk.. */
			adjalign(-3);
			u.uluck -= 3;
	    }break;
		case RAISE_UNDEAD:
			if(getdir((char *)0)){
				struct obj *ocur,*onxt;
				struct monst *mtmp;
				int tx = u.ux + u.dx;
				int ty = u.uy + u.dy;
				if (isok(tx, ty)) {
					for(ocur = level.objects[tx][ty]; ocur; ocur = onxt) {
						onxt = ocur->nexthere;
						if (ocur->otyp == EGG) revive_egg(ocur);
						else revive(ocur, FALSE);
					}
				}
				if(!(u.dx || u.dy))
					unturn_dead(&youmonst);
				else if((mtmp = m_at(tx, ty)) != 0)
					unturn_dead(mtmp);
			}
		break;
		case SINGING:{
			int song = dosongmenu("Select song?", obj);
			if(song == 0);//canceled
			else obj->osinging = song;
			obj->age = 0;
		}break;
		case ORACLE:{
			char ch;
			int oops;

			if (Blind) {
				pline("Too bad you can't see %s.", the(xname(obj)));
				break;
			}
			oops = (rnd(obj->blessed ? 16 : 20) > ACURR(A_INT) || obj->cursed);
			if (oops) {
				switch (rnd(4)) {
					case 1 : pline("%s too much to comprehend!", Tobjnam(obj, "are"));
					break;
					case 2 : pline("%s you!", Tobjnam(obj, "confuse"));
						make_confused(HConfusion + rnd(100),FALSE);
						break;
					case 3 : if (!resists_blnd(&youmonst)) {
						pline("%s your vision!", Tobjnam(obj, "damage"));
						make_blinded(Blinded + rnd(100),FALSE);
						if (!Blind) Your1(vision_clears);
						} else {
						pline("%s your vision.", Tobjnam(obj, "assault"));
						You("are unaffected!");
						}
						break;
					case 4 : pline("%s your mind!", Tobjnam(obj, "zap"));
						(void) make_hallucinated(HHallucination + rnd(100),FALSE,0L);
						break;
				}
				break;
			}

			if (Hallucination) {
				switch(rnd(6)) {
				case 1 : You("grok some groovy globs of incandescent lava.");
				break;
				case 2 : pline("Whoa!  Psychedelic colors, %s!",
					   poly_gender() == 1 ? "babe" : "dude");
				break;
				case 3 : pline_The("crystal pulses with sinister %s light!",
						hcolor((char *)0));
				break;
				case 4 : You("see goldfish swimming above fluorescent rocks.");
				break;
				case 5 : You("see tiny snowflakes spinning around a miniature farmhouse.");
				break;
				default: pline("Hey!  A girl with kaleidoscope eyes!");
				break;
				}
			break;
			}

			/* read a single character */
			if (flags.verbose) You("may look for an object or monster symbol.");
			ch = yn_function("What do you look for?", (char *)0, '\0');
			/* Don't filter out ' ' here; it has a use */
			if ((ch != def_monsyms[S_GHOST]) && (ch != def_monsyms[S_SHADE]) && index(quitchars,ch)) { 
			if (flags.verbose) pline1(Never_mind);
			return MOVE_STANDARD;
			}
			You("peer into %s...", the(xname(obj)));
			nomul(-rnd(obj->blessed ? 6 : 10), "peering through the Oracle's eye");
			nomovemsg = "";
			{
				int class;
				int ret = 0;

				/* special case: accept ']' as synonym for mimic
				 * we have to do this before the def_char_to_objclass check
				 */
				if (ch == DEF_MIMIC_DEF) ch = DEF_MIMIC;

				if ((class = def_char_to_objclass(ch)) != MAXOCLASSES)
					ret = object_detect((struct obj *)0, class);
				else if ((class = def_char_to_monclass(ch)) != MAXMCLASSES)
					ret = monster_detect((struct obj *)0, class);
				else if (iflags.bouldersym && (ch == iflags.bouldersym))
					ret = object_detect((struct obj *)0, ROCK_CLASS);
				else switch(ch) {
					case '^':
						ret = trap_detect((struct obj *)0);
						break;
					default:
						{
						static const struct {
							const char *what;
							d_level *where;
						} level_detects[] = {
						  { "Delphi", &oracle_level },
						  { "a worthy opponent", &challenge_level },
						  { "a castle", &stronghold_level },
						  { "the Wizard of Yendor's tower", &wiz1_level },
						};
						int i = rn2(SIZE(level_detects));
						You("see %s, %s.",
						level_detects[i].what,
						level_distance(level_detects[i].where));
						}
						ret = 0;
						break;
				}

				if (ret) {
					if (!rn2(100))  /* make them nervous */
					You("see the Wizard of Yendor gazing back at you.");
					else pline_The("vision is unclear.");
				}
			}
			return MOVE_STANDARD;
		}break;
		case ALLSIGHT:
			You("see the world in utter clarity.");
			/* Clear blindness and hallucination, and provide temporary immunity.
			* However, any new applications can still result in blindness/hallu the moment the protection wears off */
			n = (Race_if(PM_ORC) && !Upolyd) ? 40 : 30;
			Blinded = 0;
			HBlind_res += n;
			HHalluc_resistance += n;
			(void)make_hallucinated(FALSE, FALSE, W_ART);	/* silent */
			/* also, grant Xray vision and protection from shape changers */
			HProtection_from_shape_changers += n;
			HXray_vision += n;
			/* we'll need a full recalc */
			vision_full_recalc = 1;

			/* todo: temporarily set insight and bring insight creatures into view, mwahaha */
		break;
		case FILTH_ARROWS:
			if ((!uwep && uwep == obj)){
				You_feel("that you should be wielding %s.", the(xname(obj)));;
				obj->age = monstermoves;
				return MOVE_CANCELLED;
			}
			/* message */
			if (flags.soundok) {
				pline("%s keens quietly.", The(xname(obj)));
			}
			else {
				pline("%s vibrates softly.", The(xname(obj)));
			}
			/* while monstermoves < duration, arrows will be filthed (done in xhity.c) */
			IMPURITY_UP(u.uimp_dirtiness)
			artinstance[ART_PLAGUE].PlagueDuration = monstermoves + 13;
		break;
		case INVOKE_DARK:{
          struct obj *wand;
		  wand = mksobj(WAN_DARKNESS, MKOBJ_NOINIT);
          wand->spe = 1;
          wand->blessed = 1;
          wand->ovar1 = 1;
          weffects(wand);
          obfree(wand,(struct obj *)0);
		}break;
        case STONE_DRAGON:
			You("wake the stone dragonhead.");
			doliving_dragonhead(&youmonst, obj, TRUE);
			time = MOVE_PARTIAL;
		break;
        case MAD_KING:
			You("wake the mad king.");
			doliving_mad_king(&youmonst, obj, TRUE);
			time = MOVE_PARTIAL;
		break;
        case RINGED_SPEAR:
			You("wake the ringed spear.");
			doliving_ringed_spear(&youmonst, obj, TRUE);
			time = MOVE_PARTIAL;
		break;
        case RINGED_ARMOR:
			You("wake the ringed armor.");
			doliving_ringed_armor(&youmonst, obj, TRUE);
			time = MOVE_PARTIAL;
		break;
		case BLOODLETTER:
			if (!uwep || uwep != obj){
				You_feel("that you should be wielding %s.", the(xname(obj)));
				obj->age = monstermoves;
				return MOVE_CANCELLED;
			}
			if (artinstance[obj->oartifact].BLactive < monstermoves){
				if (has_blood_mon(&youmonst)){
					You("plunge Bloodletter into your chest, making an offering of your tainted blood.");
					losehp(Upolyd ? u.mhmax * 0.2 : u.uhpmax * 0.2, "purging tainted blood", KILLED_BY);
					artinstance[obj->oartifact].BLactive = monstermoves + 20 + rn2(10) + rn2(10);
				} else {
					You("are free of tainted blood, and have none to offer.");
				}
			} else {
				do_bloodletter(obj);
			}

		break;
		case WAVES_DARKNESS:
			if (!uwep || uwep != obj){
				You_feel("that you should be wielding %s.", the(xname(obj)));
				obj->age = monstermoves;
				return MOVE_CANCELLED;
			}
#define astel_blast(x, y, size) if (isok(x, y)) explode(x, y, AD_MAGM, WAND_CLASS, (d(1, 12) + uwep->spe) * ((Double_spell_size) ? 3 : 2) / 2, EXPL_CYAN, size + !!Double_spell_size)
			if (!getdir((char *)0)) { //Oh, getdir must set the .d_ variables below.
			    /* getdir cancelled, just do the nondirectional scroll */
				obj->age = 0;
				break;
			}
			else if(u.dx && !u.dy) {
				astel_blast(u.ux+u.dx, u.uy-1, 1);
				astel_blast(u.ux+u.dx, u.uy, 1);
				astel_blast(u.ux+u.dx, u.uy+1, 1);
			}
			else if(!u.dx && u.dy) {
				astel_blast(u.ux-1, u.uy+u.dy, 1);
				astel_blast(u.ux, u.uy+u.dy, 1);
				astel_blast(u.ux+1, u.uy+u.dy, 1);
			}
			else if(u.dx && u.dy) {
				astel_blast(u.ux, u.uy+u.dy, 1);
				astel_blast(u.ux+u.dx, u.uy+u.dy, 1);
				astel_blast(u.ux+u.dx, u.uy, 1);
			}
			else if(u.dz > 0){
				astel_blast(u.ux, u.uy, 0);
				astel_blast(u.ux, u.uy, 1);
				astel_blast(u.ux, u.uy, 2);
			}
			else if(u.dz < 0){
				doliving_fallingstar(&youmonst, obj, TRUE);
			} else {
				pline("%s", nothing_happens);
				obj->age = 0;
			}
#undef astel_blast
		break;
		case SEVEN_LEAGUE_STEP:
			if (obj != uarmf) {
				You_feel("that you should be wearing %s.", the(xname(obj)));
				obj->age = monstermoves;
				return MOVE_CANCELLED;
			}
			You("click your heels together and take a step... ");
			jump(15);
			// Set seven league boots to full speed as well
			if(obj->oartifact == ART_SEVEN_LEAGUE_BOOTS)
				artinstance[ART_SEVEN_LEAGUE_BOOTS].LeagueMod = 10;
			break;
        case DETESTATION:
			obj->age = 0L;
			if(!Pantheon_if(PM_MADMAN) || u.uevent.uhand_of_elbereth){
				pline("It's just a piece of blank paper!");
				obj->oartifact = 0;
				/* remove old name */
				rem_ox(obj, OX_ENAM);
			}
			else if(!u.uevent.qcompleted && Role_if(PM_MADMAN)){
				pline("A throbbing yellow haze obscures your vision!");
				You_cant("use this right now.");
			}
			else if(!IS_ALTAR(levl[u.ux][u.uy].typ)){
				pline("This records the very secret and ancient rite once performed by the high-priests of Sarnath in detestation of Bokrug, the water-lizard, whose followers their ancestors had cruelly slain.");
				if(!u.detestation_ritual){
					pline("The rite is penned in your hand.");
					pline("What were you going to do with it?");
				}
				else {
					if((u.detestation_ritual&RITUAL_DONE) != RITUAL_DONE)
						You("need to find another altar.");
					else if(!(u.detestation_ritual&HI_RITUAL_DONE))
						You("need to find some more powerful altars.");
					else if((u.detestation_ritual&HI_RITUAL_DONE) != HI_RITUAL_DONE)
						You("need to find another high altar.");
					else You("have finished your work at long-last.");
				}
			}
			else if(!Is_astralevel(&u.uz) && (u.detestation_ritual&RITUAL_DONE) == RITUAL_DONE){
				pline("The ritual has run its course here on the material plane. You need to find some more powerful altars!");
			}
			else if(Is_astralevel(&u.uz)){
				//Make sure madman astral always has these three if the player is aligned to Bokrug
				int dreamgods[] = {GOD_ZO_KALAR, GOD_LOBON, GOD_TAMASH};
				int high_ritual[] = {RITUAL_HI_LAW, RITUAL_HI_NEUTRAL, RITUAL_HI_CHAOS};
				int godnum = altars[levl[u.ux][u.uy].altar_num].god;
				int altaralign = a_align(u.ux,u.uy);
				You("perform a rite in detestation of %s!", godname(godnum));
				change_luck(-7);
				for(int i = 0; i < 3; i++){
					if(u.detestation_ritual&high_ritual[i])
						continue;
					godlist[dreamgods[i]].anger += 3;
					summon_god_minion(dreamgods[i], FALSE);
					summon_god_minion(dreamgods[i], FALSE);
					summon_god_minion(dreamgods[i], FALSE);
				}
				gods_upset(godnum);
				pline("The high altar sinks into swampy water!");
				levl[u.ux][u.uy].typ = PUDDLE;
				levl[u.ux][u.uy].flags = 0;
				newsym(u.ux, u.uy);
				u.detestation_ritual |= RITUAL_STARTED;
				u.detestation_ritual |= Align2hiritual(altaralign);
				if((u.detestation_ritual&HI_RITUAL_DONE) == HI_RITUAL_DONE){
					struct obj *statue = mksartifact(ART_IDOL_OF_BOKRUG__THE_WATER_);
					statue->oerodeproof = TRUE;
					statue->spe = 1;
					give_bokrug_trophy();
					place_object(statue, 37+rn2(7), 18+rn2(2));
					You_hear("water bubbling.");
					int i, j;
					for(i = 0; i < COLNO; i++){
						for(j = 0; j < ROWNO; j++){
							if(isok(i,j) && levl[i][j].typ == AIR)
								levl[i][j].typ = PUDDLE;
						}
					}
					doredraw();
				}
			}
			else {
				int altaralign = a_align(u.ux,u.uy);
				boolean used_align = u.detestation_ritual&Align2ritual(altaralign);
				int godnum = altars[levl[u.ux][u.uy].altar_num].god;
				if(!godnum)
					godnum = align_to_god(altaralign);
				if(used_align || !(godnum == GOD_ZO_KALAR || godnum == GOD_LOBON || godnum == GOD_TAMASH)){
					int destAlign, destGod;
					if(!(u.detestation_ritual&RITUAL_LAW)){
						destAlign = A_LAWFUL;
						destGod = GOD_ZO_KALAR;
					}
					else if(!(u.detestation_ritual&RITUAL_NEUTRAL)){
						destAlign = A_NEUTRAL;
						destGod = GOD_LOBON;
					}
					else {
						destAlign = A_CHAOTIC;
						destGod = GOD_TAMASH;
					}

					You("modify the rite to attune the altar to %s!", godname(destGod));
					change_luck(-3);
					altars[levl[u.ux][u.uy].altar_num].align = destAlign;
					altars[levl[u.ux][u.uy].altar_num].god = destGod;
					
					if(u.ulevel > 20) summon_god_minion(godnum, FALSE);
					if(u.ulevel >= 14) summon_god_minion(godnum, FALSE);
					(void) summon_god_minion(godnum, TRUE);

					if(u.ulevel > 20) summon_god_minion(destGod, FALSE);
					if(u.ulevel >= 14) summon_god_minion(destGod, FALSE);
					(void) summon_god_minion(destGod, TRUE);
					angry_priest();
					if(in_town(u.ux, u.uy))
						(void) angry_guards(FALSE);
					u.detestation_ritual |= RITUAL_STARTED;
				}
				else {
					You("perform a rite in detestation of %s!", godname(godnum));
					change_luck(-3);
					godlist[GOD_ZO_KALAR].anger++;
					godlist[GOD_LOBON].anger++;
					godlist[GOD_TAMASH].anger++;
					if(u.ulevel > 20) summon_god_minion(godnum, FALSE);
					if(u.ulevel >= 14) summon_god_minion(godnum, FALSE);
					(void) summon_god_minion(godnum, FALSE);
					gods_upset(godnum);
					angry_priest();
					if(in_town(u.ux, u.uy))
						(void) angry_guards(FALSE);
					pline("The altar sinks into swampy water!");
					levl[u.ux][u.uy].typ = PUDDLE;
					levl[u.ux][u.uy].flags = 0;
					newsym(u.ux, u.uy);
					u.detestation_ritual |= RITUAL_STARTED;
					u.detestation_ritual |= Align2ritual(altaralign);
					if(u.ugodbase[UGOD_CURRENT] != GOD_BOKRUG__THE_WATER_LIZARD && (u.ugodbase[UGOD_CURRENT] == godnum || (u.detestation_ritual&RITUAL_DONE) == RITUAL_DONE)){
						You("have a sudden sense of a new direction.");
						/* The player wears a helm of opposite alignment? */
						if (uarmh && uarmh->otyp == HELM_OF_OPPOSITE_ALIGNMENT)
							u.ugodbase[UGOD_CURRENT] = GOD_BOKRUG__THE_WATER_LIZARD;
						else {
							u.ualign.god = u.ugodbase[UGOD_CURRENT] = GOD_BOKRUG__THE_WATER_LIZARD;
							u.ualign.type = A_NONE;
						}
						u.ublessed = 0;
						flags.botl = 1;

						u.lastprayed = moves;
						u.reconciled = REC_NONE;
						u.lastprayresult = PRAY_CONV;
						adjalign((int)(galign(u.ugodbase[UGOD_ORIGINAL]) * (ALIGNLIM / 2)));
					}
					if((u.detestation_ritual&RITUAL_DONE) == RITUAL_DONE){
						struct obj *arm = mksartifact(ART_IBITE_ARM);
						arm->oerodeproof = TRUE;
						arm->spe = 1;
						place_object(arm, u.ux, u.uy);
						if(Blind)
							You_hear("water bubbling.");
						else
							pline("%s rises from the swamp!", An(xname(arm)));
					}
				}
			}
			break;
        case INVULNERABILITY:
			make_invulnerable(HSanctuary + 3, TRUE);
			break;
        case IBITE_ARM:
			time = ibite_arm_menu(obj);
		break;
        case IMPERIAL_RING:{
			if(obj->spe <= 0 && artinstance[obj->oartifact].uconstel_pets >= 2){
				pline("Nothing happens!");
				break;
			}
			winid tmpwin;
			anything any;
			menu_item *selected;
			int n;

			any.a_void = 0;         /* zero out all bits */
			tmpwin = create_nhwindow(NHW_MENU);
			start_menu(tmpwin);
			
			any.a_int = 1;
			if(obj->spe > 0)
				add_menu(tmpwin, NO_GLYPH, &any, 'a', 0, ATR_NONE, "Make Wish", MENU_UNSELECTED);
			any.a_int++; //Advance to next option anyway
			if(artinstance[obj->oartifact].uconstel_pets < 2)
				add_menu(tmpwin, NO_GLYPH, &any, 'b', 0, ATR_NONE, "Summon Constellation", MENU_UNSELECTED);

			end_menu(tmpwin, "Call upon the stars?");
			n = select_menu(tmpwin, PICK_ONE, &selected);

			if(n > 0){
				n = selected[0].item.a_int;
				free(selected);
			}
			destroy_nhwindow(tmpwin);
			if(!n){
				return MOVE_CANCELLED;
			}

			if(n == 1){
				use_ring_of_wishes(obj);
			}
			else if(n == 2){
				if(artinstance[obj->oartifact].uconstel_pets < 2){
					struct monst *mtmp;
					long futurewishflag = Role_if(PM_TOURIST) ? 0 : MG_FUTURE_WISH;
					mtmp = create_particular(u.ux, u.uy, MT_DOMESTIC, 0, FALSE, 0, MG_NOWISH|MG_NOTAME|futurewishflag, G_UNIQ, (char *)0);
					if (!mtmp) {
						pline("Perhaps try summoning something else?");
					}
					else {
						set_template(mtmp, CONSTELLATION);
						EDOG(mtmp)->loyal = 1;
						if(!Blind)
							pline("Motes of light condense into %s.", mon_nam(mtmp));
						artinstance[obj->oartifact].uconstel_pets++;
					}
				}
				else pline("Nothing happens!");
			}
			else {
				pline1(Never_mind);
			}
		}break;
        case SNARE_WEAPONS:{
			struct monst *mtmp;
			struct obj *otmp, *nobj;
			long unwornmask;
			You("throw the snare into the air!");
			for(mtmp = fmon; mtmp; mtmp = mtmp->nmon){
				if(couldsee(mtmp->mx, mtmp->my) && !mtmp->mpeaceful && !resist(mtmp, WEAPON_CLASS, 0, NOTELL)){
					for (otmp = mtmp->minvent; otmp; otmp = nobj) {
						nobj = otmp->nobj;
						/* Not clothing */
						if(otmp->owornmask && !(otmp->owornmask&(W_WEP|W_SWAPWEP)))
							continue;
						if(mtmp->entangled_oid == otmp->o_id)
							continue;
						/* take the object away from the monster */
						obj_extract_self(otmp);
						if ((unwornmask = otmp->owornmask) != 0L) {
							mtmp->misc_worn_check &= ~unwornmask;
							if (otmp->owornmask & W_WEP) {
								setmnotwielded(mtmp,otmp);
								MON_NOWEP(mtmp);
							}
							if (otmp->owornmask & W_SWAPWEP){
								setmnotwielded(mtmp,otmp);
								MON_NOSWEP(mtmp);
							}
							otmp->owornmask = 0L;
						}
						update_mon_intrinsics(mtmp, otmp, FALSE, FALSE);
						if(!Blind) pline("The snare sucks up %s %s and drops it to the %s.",
							  s_suffix(mon_nam(mtmp)), xname(otmp), surface(u.ux, u.uy));
						dropy(otmp);
						/* more take-away handling, after theft message */
						if (unwornmask & W_WEP || unwornmask & W_SWAPWEP) {		/* stole wielded weapon */
							possibly_unwield(mtmp, FALSE);
						}
					}
					//Has already failed a resist check
					set_mcan(mtmp, TRUE);
					mtmp->mspec_used = 8;
				}
			}
		}break;
		case GITH_ART:{
			doGithForm();
		}break;
		case AMALGUM_ART:
		case ZERTH_ART:{
			zerth_mantras();
		}break;
		case MORGOTH:{
			int base_otyp = find_good_iring();
			int new_otyp = obj->otyp == base_otyp ? RIN_NOTHING : base_otyp;
			boolean worn_left = obj == uleft;
			boolean worn_right = obj == uright;
			pline("%s suddenly seems much more %s.", artilist[obj->oartifact].name,
			      new_otyp == RIN_NOTHING ? "boring" : "interesting");
			if (worn_left || worn_right) Ring_off(obj);
			obj->otyp = new_otyp;
			if (worn_left || worn_right) {
				setworn(obj, worn_left ? LEFT_RING : RIGHT_RING);
				Ring_on(obj);
			}
			if (obj->otyp == RIN_NOTHING) makeknown(RIN_NOTHING);
			obj->age = 0;
		}break;
		case SCORPION_UPGRADES:
			scorpion_upgrade_menu(obj);
		break;
		default: pline("Program in disorder.  Artifact invoke property not recognized");
		break;
	} //end of first case:  Artifact Specials!!!!

    } else {
	long eprop = (u.uprops[oart->inv_prop].extrinsic ^= W_ARTI),
	     iprop = u.uprops[oart->inv_prop].intrinsic;
	boolean on = (eprop & W_ARTI) != 0; /* true if invoked prop just set */

	if(on && obj->age > monstermoves) {
	    /* the artifact is tired :-) */
	    u.uprops[oart->inv_prop].extrinsic ^= W_ARTI;
	    You_feel("that %s %s ignoring you.",
		     the(xname(obj)), otense(obj, "are"));
	    /* can't just keep repeatedly trying */
	    obj->age += (long) d(3,10);
	    return MOVE_PARTIAL;
	} else if(!on) {
	    /* when turning off property, determine downtime */
	    /* arbitrary for now until we can tune this -dlc */
//	    obj->age = monstermoves + rnz(100);
		obj->age = monstermoves + (long)(rnz(100)*(Role_if(PM_PRIEST) ? .75 : 1));
	}

	if ((eprop & ~W_ARTI) ||
		(iprop && !(
		oart->inv_prop == FAST	/* Fast has a noticable difference between intrinsic and extrinsic effects */
		)
		)){	
nothing_special:
	    /* you had the property from some other source too */
	    if (carried(obj))
		You_feel("a surge of power, but nothing seems to happen.");
	    return MOVE_STANDARD;
	}
	switch(oart->inv_prop) {
	case FAST:
	    if(on) You_feel("yourself speed up%s.", (iprop ? " a bit more" : ""));
		else You_feel("yourself slow down%s.", (iprop ? " a bit" : ""));
	    break;
	case CONFLICT:
	    if(on) You_feel("like a rabble-rouser.");
	    else You_feel("the tension decrease around you.");
	    break;
	case LEVITATION:
	    if(on) {
		float_up();
		spoteffects(FALSE);
	    } else (void) float_down(I_SPECIAL|TIMEOUT, W_ARTI);
	    break;
	case INVIS:
	    if (BInvis || Blind) goto nothing_special;
	    newsym(u.ux, u.uy);
	    if (on)
		Your("body takes on a %s transparency...",
		     Hallucination ? "normal" : "strange");
	    else
		Your("body seems to unfade...");
	    break;
	case WWALKING:
		if (on) You_feel("more buoyant!");
		else You_feel("heavier!");
		break;
#ifdef BARD
	    /*
	case HARMONIZE:
	    {
		struct monst *mtmp;
		struct obj *otmp;
		int i,j;

		for(i = -u.ulevel; i <= u.ulevel; i++)
		    for(j = -u.ulevel; j <= u.ulevel; j++)
			if (isok(u.ux+i,u.uy+j) 
			    && i*i + j*j < u.ulevel * u.ulevel) {
			    mtmp = m_at(u.ux+i,u.uy+j);
			    if (mtmp->mtame && distu(mtmp->mx, mtmp->my) < u.ulevel) {
				mtmp->mhp = mtmp->mhpmax;
				mtmp->msleeping = mtmp->mflee = mtmp->mblinded = mtmp->mcanmove = mtmp->mfrozen = mtmp->mstun = mtmp->mconf = 0;
				mtmp->mcansee = 1;
				mtmp->mtame++;
			    }
			}
	    }
	    break;
	    */
#endif
	}
    }
	if (oart == &artilist[ART_IRON_BALL_OF_LEVITATION]) {
		if (Punished && (obj != uball)) {
			unpunish(); /* Remove a mundane heavy iron ball */
		}
		
		if (!Punished) {
			setworn(mkobj(CHAIN_CLASS, TRUE), W_CHAIN);
			setworn(obj, W_BALL);
			uball->spe = 1;
			if (!u.uswallow) {
			placebc();
			if (Blind) set_bc(1);	/* set up ball and chain variables */
			newsym(u.ux,u.uy);		/* see ball&chain if can't see self */
			}
			Your("%s chains itself to you!", xname(obj));
		}
	}

    return time;
}

int
donecromenu(prompt, obj)
const char *prompt;
struct obj *obj;
{
	winid tmpwin;
	int n, how;
	char buf[BUFSZ];
	char incntlet = 'a';
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */

	Sprintf(buf, "Known Passages");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
	if(obj->ovar1_necronomicon & S_BYAKHEE){
		Sprintf(buf, "Summon byakhee");
		any.a_int = SELECT_BYAKHEE;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & S_NIGHTGAUNT){
		Sprintf(buf, "Summon Night-gaunt");
		any.a_int = SELECT_NIGHTGAUNT;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & S_SHOGGOTH){
		Sprintf(buf, "Summon Shoggoth");
		any.a_int = SELECT_SHOGGOTH;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & S_OOZE){
		Sprintf(buf, "Summon Ooze");
		any.a_int = SELECT_OOZE;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & SUM_DEMON){
		Sprintf(buf, "Summon Demon");
		any.a_int = SELECT_DEMON;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & S_DEVIL){
		Sprintf(buf, "Summon Devil");
		any.a_int = SELECT_DEVIL;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & SP_PROTECTION){
		Sprintf(buf, "Study secrets of defense");
		any.a_int = SELECT_PROTECTION;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & SP_TURN_UNDEAD){
		Sprintf(buf, "Study secrets of life and death");
		any.a_int = SELECT_TURN_UNDEAD;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & SP_FORCE_BOLT){
		Sprintf(buf, "Study secrets of the intangible forces");
		any.a_int = SELECT_FORCE_BOLT;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & SP_DRAIN_LIFE){
		Sprintf(buf, "Study secrets of time and decay");
		any.a_int = SELECT_DRAIN_LIFE;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & SP_DEATH){
		Sprintf(buf, "Study secrets of the Reaper");
		any.a_int = SELECT_DEATH;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & SP_DETECT_MNSTR){
		Sprintf(buf, "Study secrets of monster detection");
		any.a_int = SELECT_DETECT_MNSTR;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & SP_CLAIRVOYANCE){
		Sprintf(buf, "Study secrets of clairvoyance");
		any.a_int = SELECT_CLAIRVOYANCE;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & SP_DETECT_UNSN){
		Sprintf(buf, "Study secrets of unseen things");
		any.a_int = SELECT_DETECT_UNSN;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & SP_IDENTIFY){
		Sprintf(buf, "Study secrets of knowledge without learning");
		any.a_int = SELECT_IDENTIFY;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & SP_CONFUSE){
		Sprintf(buf, "Study secrets of confusion");
		any.a_int = SELECT_CONFUSE;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & SP_CAUSE_FEAR){
		Sprintf(buf, "Study secrets of fear");
		any.a_int = SELECT_CAUSE_FEAR;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & SP_LEVITATION){
		Sprintf(buf, "Study secrets of gravity");
		any.a_int = SELECT_LEVITATION;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & SP_STONE_FLESH){
		Sprintf(buf, "Study secrets of the earth mother");
		any.a_int = SELECT_STONE_FLESH;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & SP_CANCELLATION){
		Sprintf(buf, "Study secrets of fraying the weave");
		any.a_int = SELECT_CANCELLATION;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & SP_COMBAT){
		Sprintf(buf, "Study secrets of war and killing");
		any.a_int = SELECT_COMBAT;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & SP_HEALTH){
		Sprintf(buf, "Study secrets of health and recovery");
		any.a_int = SELECT_HEALTH;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & R_Y_SIGN){
		Sprintf(buf, "Study the legends of lost Carcosa");
		any.a_int = SELECT_SIGN;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & R_WARDS){
		Sprintf(buf, "Study up on ancient warding signs");
		any.a_int = SELECT_WARDS;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & R_ELEMENTS){
		Sprintf(buf, "Study the Lords of the Elements");
		any.a_int = SELECT_ELEMENTS;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & R_NAMES_1){
		Sprintf(buf, "Study the first half of the testament of whispers");
		any.a_int = SELECT_SPIRITS1;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & R_NAMES_2){
		Sprintf(buf, "Study the second half of the testament of whispers");
		any.a_int = SELECT_SPIRITS2;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	if(obj->ovar1_necronomicon & R_YOG_SOTH){
		Sprintf(buf, "Read the long chant of opening the gates");
		any.a_int = SELECT_YOG_SOTHOTH;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			incntlet, 0, ATR_NONE, buf,
			MENU_UNSELECTED);
		incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	}
	end_menu(tmpwin, prompt);

	how = PICK_ONE;
	n = select_menu(tmpwin, how, &selected);
	destroy_nhwindow(tmpwin);
	if(n > 0){
		int picked = selected[0].item.a_int;
		free(selected);
		return (n < SELECT_STUDY) ? picked : 0;
	}
	return 0;
}

int
dopetmenu(prompt, obj)
const char *prompt;
struct obj *obj;
{
	winid tmpwin;
	int n, how;
	char buf[BUFSZ];
	char incntlet = 'a';
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */

	Sprintf(buf, "What will you take out?");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
	
	Sprintf(buf, "Magic whistle");
	any.a_int = SELECT_WHISTLE;	/* must be non-zero */
	add_menu(tmpwin, NO_GLYPH, &any,
		incntlet, 0, ATR_NONE, buf,
		MENU_UNSELECTED);
	incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	
	Sprintf(buf, "Leash");
	any.a_int = SELECT_LEASH;	/* must be non-zero */
	add_menu(tmpwin, NO_GLYPH, &any,
		incntlet, 0, ATR_NONE, buf,
		MENU_UNSELECTED);
	incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	
	
	Sprintf(buf, "Saddle");
	any.a_int = SELECT_SADDLE;	/* must be non-zero */
	add_menu(tmpwin, NO_GLYPH, &any,
		incntlet, 0, ATR_NONE, buf,
		MENU_UNSELECTED);
	incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	
	
	Sprintf(buf, "Tripe");
	any.a_int = SELECT_TRIPE;	/* must be non-zero */
	add_menu(tmpwin, NO_GLYPH, &any,
		incntlet, 0, ATR_NONE, buf,
		MENU_UNSELECTED);
	incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	
	
	Sprintf(buf, "Apple");
	any.a_int = SELECT_APPLE;	/* must be non-zero */
	add_menu(tmpwin, NO_GLYPH, &any,
		incntlet, 0, ATR_NONE, buf,
		MENU_UNSELECTED);
	incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	
	
	Sprintf(buf, "Banana");
	any.a_int = SELECT_BANANA;	/* must be non-zero */
	add_menu(tmpwin, NO_GLYPH, &any,
		incntlet, 0, ATR_NONE, buf,
		MENU_UNSELECTED);
	incntlet = (incntlet != 'z') ? (incntlet+1) : 'A';
	
	end_menu(tmpwin, prompt);

	how = PICK_ONE;
	n = select_menu(tmpwin, how, &selected);
	destroy_nhwindow(tmpwin);
	if(n > 0){
		int picked = selected[0].item.a_int;
		free(selected);
		return picked;
	}
	return 0;
}

int
dolordsmenu(prompt, obj)
const char *prompt;
struct obj *obj;
{
	winid tmpwin;
	int n, how;
	char buf[BUFSZ];
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */

	Sprintf(buf, "What do you command?");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
	if(obj->oartifact == ART_SCEPTRE_OF_LOLTH){
		if(obj->otyp != DROVEN_GREATSWORD){
			Sprintf(buf, "Become a greatsword");
			any.a_int = COMMAND_D_GREAT;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'g', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		
		if(obj->otyp != MOON_AXE){
			Sprintf(buf, "Become a moon axe");
			any.a_int = COMMAND_MOON_AXE;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'a', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		
		if(obj->otyp != KHAKKHARA){
			Sprintf(buf, "Become a khakkhara");
			any.a_int = COMMAND_KHAKKHARA;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'k', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		
		if(obj->otyp != DROVEN_SPEAR){
			Sprintf(buf, "Become a spear");
			any.a_int = COMMAND_DROVEN_SPEAR;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				's', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		
		if(obj->otyp != DROVEN_LANCE){
			Sprintf(buf, "Become a lance");
			any.a_int = COMMAND_D_LANCE;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'l', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
	} else if(obj->oartifact == ART_ROD_OF_THE_ELVISH_LORDS){
		if(obj->otyp != ELVEN_BROADSWORD){
			Sprintf(buf, "Become a sword");
			any.a_int = COMMAND_E_SWORD;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'w', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		
		if(obj->otyp != ELVEN_SICKLE){
			Sprintf(buf, "Become a sickle");
			any.a_int = COMMAND_E_SICKLE;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'c', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		
		if(obj->otyp != ELVEN_MACE){
			Sprintf(buf, "Become a mace");
			any.a_int = COMMAND_E_MACE;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'm', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		
		if(obj->otyp != ELVEN_SPEAR){
			Sprintf(buf, "Become a spear");
			any.a_int = COMMAND_E_SPEAR;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				's', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		
		if(obj->otyp != ELVEN_LANCE){
			Sprintf(buf, "Become a lance");
			any.a_int = COMMAND_E_LANCE;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'l', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
	} else if(obj->oartifact == ART_GOLDEN_SWORD_OF_Y_HA_TALLA){
		if(obj->otyp != SCIMITAR){
			Sprintf(buf, "Become a sword");
			any.a_int = COMMAND_SCIMITAR;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				's', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		
		if(obj->otyp != BULLWHIP){
			Sprintf(buf, "Become a scorpion whip");
			any.a_int = COMMAND_WHIP;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'w', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
	} else if(obj->oartifact == ART_XIUHCOATL){
		if(obj->otyp != ATLATL){
			Sprintf(buf, "Become an atlatl");
			any.a_int = COMMAND_ATLATL;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'a', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		
		if(obj->otyp != BULLWHIP){
			Sprintf(buf, "Become a flaming whip");
			any.a_int = COMMAND_WHIP;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'w', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
	} else {
		if(obj->otyp != RAPIER){
			Sprintf(buf, "Become a rapier");
			any.a_int = COMMAND_RAPIER;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'r', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		
		if(obj->otyp != AXE){
			Sprintf(buf, "Become an axe");
			any.a_int = COMMAND_AXE;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'a', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		
		if(obj->otyp != MACE){
			Sprintf(buf, "Become a mace");
			any.a_int = COMMAND_MACE;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'm', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		
		if(obj->otyp != SPEAR){
			Sprintf(buf, "Become a spear");
			any.a_int = COMMAND_SPEAR;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				's', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		
		if(obj->otyp != LANCE){
			Sprintf(buf, "Become a lance");
			any.a_int = COMMAND_LANCE;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'l', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
	}
	
	if(obj->age < monstermoves){
		if(obj->oartifact == ART_GOLDEN_SWORD_OF_Y_HA_TALLA){
			Sprintf(buf, "Strike");
			any.a_int = COMMAND_STRIKE;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'S', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		} else if(obj->oartifact == ART_XIUHCOATL){
			Sprintf(buf, "Conjure ammunition");
			any.a_int = COMMAND_AMMO;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'c', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		} else {
			if((obj->otyp == MACE || obj->otyp == ELVEN_MACE || obj->otyp == KHAKKHARA) && (
			   u.uswallow || 
			   (!u.uhave.amulet && Can_rise_up(u.ux, u.uy, &u.uz)) || 
			   (u.uhave.amulet && !Weightless && !Is_waterlevel(&u.uz) && !Underwater) 
			  )){
				Sprintf(buf, "Become a ladder");
				any.a_int = COMMAND_LADDER;	/* must be non-zero */
				add_menu(tmpwin, NO_GLYPH, &any,
					'L', 0, ATR_NONE, buf,
					MENU_UNSELECTED);
			}
			
			Sprintf(buf, "Show me my surroundings");
			any.a_int = COMMAND_CLAIRVOYANCE;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'S', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
			
			Sprintf(buf, "Inspire fear");
			any.a_int = COMMAND_FEAR;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'F', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
			
			if((obj->otyp == SPEAR || obj->otyp == DROVEN_SPEAR || obj->otyp == KHAKKHARA || obj->otyp == ELVEN_SPEAR)){
				Sprintf(buf, "Give me your life");
				any.a_int = COMMAND_LIFE;	/* must be non-zero */
				add_menu(tmpwin, NO_GLYPH, &any,
					'G', 0, ATR_NONE, buf,
					MENU_UNSELECTED);
			}
			
			Sprintf(buf, "Kneel");
			any.a_int = COMMAND_KNEEL;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'K', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
	}
	end_menu(tmpwin, prompt);

	how = PICK_ONE;
	n = select_menu(tmpwin, how, &selected);
	destroy_nhwindow(tmpwin);
	if(n > 0){
		int picked = selected[0].item.a_int;
		free(selected);
		return picked;
	}
	return 0;
}

int
doannulmenu(prompt, obj)
const char *prompt;
struct obj *obj;
{
	winid tmpwin;
	int n, how;
	char buf[BUFSZ];
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */

	Sprintf(buf, "Function list:");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
	if(obj->otyp != KHAKKHARA && !uarms && !u.twoweap){
		Sprintf(buf, "Become a khakkhara");
		any.a_int = COMMAND_KHAKKHARA;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'k', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->otyp != LIGHTSABER){
		Sprintf(buf, "Become a lightsaber");
		any.a_int = COMMAND_SABER;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			's', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->oclass != RING_CLASS){
		Sprintf(buf, "Become a ring");
		any.a_int = COMMAND_RING;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'r', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->otyp != BFG && !uarms && !u.twoweap){
		Sprintf(buf, "Become a gun");
		any.a_int = COMMAND_BFG;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'g', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->otyp != CHAKRAM){
		Sprintf(buf, "Become a chakram");
		any.a_int = COMMAND_ANNULUS;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'c', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}

	if(obj->ovara_timeout < monstermoves){
		if(obj->otyp == KHAKKHARA){
			Sprintf(buf, "Ring Out");
			any.a_int = COMMAND_BELL;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'R', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		
		if(obj->otyp == BFG){
			Sprintf(buf, "Create Bullets");
			any.a_int = COMMAND_BULLETS;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'B', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
			Sprintf(buf, "Create Rockets");
			any.a_int = COMMAND_ROCKETS;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'R', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
			Sprintf(buf, "Fire Beam");
			any.a_int = COMMAND_BEAM;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'F', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		if(obj->otyp == CHAKRAM){
			Sprintf(buf, "Annul");
			any.a_int = COMMAND_ANNUL;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'A', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
		
		if(obj->otyp == LIGHTSABER){
			Sprintf(buf, "Recharge");
			any.a_int = COMMAND_CHARGE;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'R', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
	} else if(Is_astralevel(&u.uz) && IS_ALTAR(levl[u.ux][u.uy].typ) && a_align(u.ux, u.uy) == A_LAWFUL){
		if(obj->otyp == CHAKRAM){
			Sprintf(buf, "Annul");
			any.a_int = COMMAND_ANNUL;	/* must be non-zero */
			add_menu(tmpwin, NO_GLYPH, &any,
				'A', 0, ATR_NONE, buf,
				MENU_UNSELECTED);
		}
	}
	end_menu(tmpwin, prompt);

	how = PICK_ONE;
	n = select_menu(tmpwin, how, &selected);
	destroy_nhwindow(tmpwin);
	if(n > 0){
		int picked = selected[0].item.a_int;
		free(selected);
		return picked;
	}
	return 0;
}

int
doselfpoisonmenu(prompt, obj)
const char *prompt;
struct obj *obj;
{
	winid tmpwin;
	int n, how;
	char buf[BUFSZ];
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */

	Sprintf(buf, "Pick your poison:");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
    Sprintf(buf, "Poison");
    any.a_int = COMMAND_POISON;	/* must be non-zero */
    add_menu(tmpwin, NO_GLYPH, &any,
        'p', 0, ATR_NONE, buf,
        MENU_UNSELECTED);
    Sprintf(buf, "Drug");
    any.a_int = COMMAND_DRUG;	/* must be non-zero */
    add_menu(tmpwin, NO_GLYPH, &any,
        'd', 0, ATR_NONE, buf,
        MENU_UNSELECTED);
    Sprintf(buf, "Stain");
    any.a_int = COMMAND_STAIN;	/* must be non-zero */
    add_menu(tmpwin, NO_GLYPH, &any,
        'f', 0, ATR_NONE, buf,
        MENU_UNSELECTED);
    Sprintf(buf, "Envenom");
    any.a_int = COMMAND_ENVENOM;	/* must be non-zero */
    add_menu(tmpwin, NO_GLYPH, &any,
        'e', 0, ATR_NONE, buf,
        MENU_UNSELECTED);
    Sprintf(buf, "Diseased Filth");
    any.a_int = COMMAND_FILTH;	/* must be non-zero */
    add_menu(tmpwin, NO_GLYPH, &any,
        'f', 0, ATR_NONE, buf,
        MENU_UNSELECTED);
	end_menu(tmpwin, prompt);

	how = PICK_ONE;
	n = select_menu(tmpwin, how, &selected);
	destroy_nhwindow(tmpwin);
	if(n > 0){
		int picked = selected[0].item.a_int;
		free(selected);
		return picked;
	}
	return 0;
}

int
doartificemenu(prompt, obj)
const char *prompt;
struct obj *obj;
{
	winid tmpwin;
	int n, how;
	char buf[BUFSZ];
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */

	Sprintf(buf, "Improve weapons or armors:");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
    Sprintf(buf, "Weapons");
    any.a_int = COMMAND_IMPROVE_WEP;	/* must be non-zero */
    add_menu(tmpwin, NO_GLYPH, &any,
        'w', 0, ATR_NONE, buf,
        MENU_UNSELECTED);
    Sprintf(buf, "Armors");
    any.a_int = COMMAND_IMPROVE_ARM;	/* must be non-zero */
    add_menu(tmpwin, NO_GLYPH, &any,
        'a', 0, ATR_NONE, buf,
        MENU_UNSELECTED);
	end_menu(tmpwin, prompt);

	how = PICK_ONE;
	n = select_menu(tmpwin, how, &selected);
	destroy_nhwindow(tmpwin);
	if(n > 0){
		int picked = selected[0].item.a_int;
		free(selected);
		return picked;
	}
	return 0;
}

int
doprismaticmenu(prompt, obj)
const char *prompt;
struct obj *obj;
{
	winid tmpwin;
	int n, how;
	char buf[BUFSZ];
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */

	Sprintf(buf, "Choose a new color for your armor:");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
    if(mvitals[PM_GRAY_DRAGON].seen || mvitals[PM_BABY_GRAY_DRAGON].seen){
      Sprintf(buf, "Gray");
      any.a_int = COMMAND_GRAY;	/* must be non-zero */
      add_menu(tmpwin, NO_GLYPH, &any,
          'g', 0, ATR_NONE, buf,
          MENU_UNSELECTED);
    }
    if(mvitals[PM_SILVER_DRAGON].seen || mvitals[PM_BABY_SILVER_DRAGON].seen){
      Sprintf(buf, "Silver");
      any.a_int = COMMAND_SILVER;	/* must be non-zero */
      add_menu(tmpwin, NO_GLYPH, &any,
          's', 0, ATR_NONE, buf,
          MENU_UNSELECTED);
    }
    if(mvitals[PM_SHIMMERING_DRAGON].seen || mvitals[PM_BABY_SHIMMERING_DRAGON].seen){
      Sprintf(buf, "Shimmering");
      any.a_int = COMMAND_SHIMMERING;	/* must be non-zero */
      add_menu(tmpwin, NO_GLYPH, &any,
          'S', 0, ATR_NONE, buf,
          MENU_UNSELECTED);
    }
    if(mvitals[PM_RED_DRAGON].seen || mvitals[PM_BABY_RED_DRAGON].seen){
      Sprintf(buf, "Red");
      any.a_int = COMMAND_RED;	/* must be non-zero */
      add_menu(tmpwin, NO_GLYPH, &any,
          'r', 0, ATR_NONE, buf,
          MENU_UNSELECTED);
    }
    if(mvitals[PM_WHITE_DRAGON].seen || mvitals[PM_BABY_WHITE_DRAGON].seen){
      Sprintf(buf, "White");
      any.a_int = COMMAND_WHITE;	/* must be non-zero */
      add_menu(tmpwin, NO_GLYPH, &any,
          'w', 0, ATR_NONE, buf,
          MENU_UNSELECTED);
    }
    if(mvitals[PM_ORANGE_DRAGON].seen || mvitals[PM_BABY_ORANGE_DRAGON].seen){
      Sprintf(buf, "Orange");
      any.a_int = COMMAND_ORANGE;	/* must be non-zero */
      add_menu(tmpwin, NO_GLYPH, &any,
          'o', 0, ATR_NONE, buf,
          MENU_UNSELECTED);
    }
    if(mvitals[PM_BLACK_DRAGON].seen || mvitals[PM_BABY_BLACK_DRAGON].seen){
      Sprintf(buf, "Black");
      any.a_int = COMMAND_BLACK;	/* must be non-zero */
      add_menu(tmpwin, NO_GLYPH, &any,
          'b', 0, ATR_NONE, buf,
          MENU_UNSELECTED);
    }
    if(mvitals[PM_BLUE_DRAGON].seen || mvitals[PM_BABY_BLUE_DRAGON].seen){
      Sprintf(buf, "Blue");
      any.a_int = COMMAND_BLUE;	/* must be non-zero */
      add_menu(tmpwin, NO_GLYPH, &any,
          'B', 0, ATR_NONE, buf,
          MENU_UNSELECTED);
    }
    if(mvitals[PM_GREEN_DRAGON].seen || mvitals[PM_BABY_GREEN_DRAGON].seen){
      Sprintf(buf, "Green");
      any.a_int = COMMAND_GREEN;	/* must be non-zero */
      add_menu(tmpwin, NO_GLYPH, &any,
          'G', 0, ATR_NONE, buf,
          MENU_UNSELECTED);
    }
    if(mvitals[PM_YELLOW_DRAGON].seen || mvitals[PM_BABY_YELLOW_DRAGON].seen){
      Sprintf(buf, "Yellow");
      any.a_int = COMMAND_YELLOW;	/* must be non-zero */
      add_menu(tmpwin, NO_GLYPH, &any,
          'Y', 0, ATR_NONE, buf,
          MENU_UNSELECTED);
    }
	end_menu(tmpwin, prompt);

	how = PICK_ONE;
	n = select_menu(tmpwin, how, &selected);
	destroy_nhwindow(tmpwin);
	if(n > 0){
		int picked = selected[0].item.a_int;
		free(selected);
		return picked;
	}
	return 0;
}

int
dosongmenu(prompt, obj)
const char *prompt;
struct obj *obj;
{
	winid tmpwin;
	int n, how;
	char buf[BUFSZ];
	menu_item *selected;
	anything any;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_void = 0;		/* zero out all bits */

	Sprintf(buf, "What will have me sing?");
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_BOLD, buf, MENU_UNSELECTED);
	
	Sprintf(buf, "Nothing in particular");
	any.a_int = SELECT_NOTHING;	/* must be non-zero */
	add_menu(tmpwin, NO_GLYPH, &any,
		'n', 0, ATR_NONE, buf,
		MENU_UNSELECTED);
	if(obj->ovara_heard&OHEARD_FEAR){
		Sprintf(buf, "Chant ominously.");
		any.a_int = OSING_FEAR;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'o', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->ovara_heard&OHEARD_HEALING){
		Sprintf(buf, "Hum soothingly.");
		any.a_int = OSING_HEALING;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			's', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->ovara_heard&OHEARD_RALLY){
		Sprintf(buf, "Rally my forces.");
		any.a_int = OSING_RALLY;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'r', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->ovara_heard&OHEARD_CONFUSE){
		Sprintf(buf, "Confuse my foes.");
		any.a_int = OSING_CONFUSE;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'c', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->ovara_heard&OHEARD_CANCEL){
		Sprintf(buf, "Disrupt spellcasting.");
		any.a_int = OSING_CANCEL;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'd', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->ovara_heard&OHEARD_HASTE){
		Sprintf(buf, "A marching song.");
		any.a_int = OSING_HASTE;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'm', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->ovara_heard&OHEARD_LETHARGY){
		Sprintf(buf, "A lethargic song.");
		any.a_int = OSING_LETHARGY;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'l', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->ovara_heard&OHEARD_COURAGE){
		Sprintf(buf, "An inspiring song.");
		any.a_int = OSING_COURAGE;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'i', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->ovara_heard&OHEARD_DIRGE){
		Sprintf(buf, "A dismal dirge.");
		any.a_int = OSING_DIRGE;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'D', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->ovara_heard&OHEARD_FIRE){
		Sprintf(buf, "A fiery song.");
		any.a_int = OSING_FIRE;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'F', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->ovara_heard&OHEARD_FROST){
		Sprintf(buf, "A chilling song.");
		any.a_int = OSING_FROST;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'C', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->ovara_heard&OHEARD_ELECT){
		Sprintf(buf, "An electrifying song.");
		any.a_int = OSING_ELECT;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'E', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->ovara_heard&OHEARD_QUAKE){
		Sprintf(buf, "An earthshaking chant.");
		any.a_int = OSING_QUAKE;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'Q', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->ovara_heard&OHEARD_OPEN){
		Sprintf(buf, "A tune of opening.");
		any.a_int = OSING_OPEN;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'O', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->ovara_heard&OHEARD_DEATH){
		Sprintf(buf, "Of the passing of life.");
		any.a_int = OSING_DEATH;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'P', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->ovara_heard&OHEARD_LIFE){
		Sprintf(buf, "Beat to the Cadence.");
		any.a_int = OSING_LIFE;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'L', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	if(obj->ovara_heard&OHEARD_INSANE){
		Sprintf(buf, "A hymn to the creator.");
		any.a_int = OSING_INSANE;	/* must be non-zero */
		add_menu(tmpwin, NO_GLYPH, &any,
			'I', 0, ATR_NONE, buf,
			MENU_UNSELECTED);
	}
	end_menu(tmpwin, prompt);

	how = PICK_ONE;
	n = select_menu(tmpwin, how, &selected);
	destroy_nhwindow(tmpwin);
	if(n > 0){
		int picked = selected[0].item.a_int;
		free(selected);
		return picked;
	}
	return 0;
}

STATIC_PTR int
read_necro(VOID_ARGS)
{
	struct permonst *pm;
	struct monst *mtmp = 0;
	int i;
	short booktype;
	char splname[BUFSZ];

	if (Confusion) {		/* became confused while learning */
//	    (void) confused_book(book);
	    artiptr = 0;		/* no longer studying */
	    nomul(delay, "struggling with the Necronomicon");		/* remaining delay is uninterrupted */
	    delay = 0;
	    return MOVE_FINISHED_OCCUPATION;
	}
	if(u.veil && delay >= -50){
		You("feel reality threatening to slip away!");
		if (yn("Are you sure you want to keep reading?") != 'y'){
			delay = 0;
			return(0);
		}
		else pline("So be it.");
		u.veil = FALSE;
		change_uinsight(1);
	}
	if (delay) {	/* not if (delay++), so at end delay == 0 */
	/* lenses give 50% faster reading */
//	    nomul( (ublindf && ublindf->otyp == LENSES) ? 
//			(-26+ACURR(A_INT))/2 :
//			(-26+ACURR(A_INT))); /* Necronomicon is difficult to put down */
//	    delay -= -26+ACURR(A_INT); /* - a negative number is + */
		delay++;
		if(ublindf && ublindf->otyp == LENSES) delay++;
		if(delay < 0){
			return MOVE_READ; /* still busy */
		}
		delay = 0;
	}
	if(necro_effect < SELECTED_SPELL){/* summoning spell*/
		boolean summon_failed = TRUE;
		switch(necro_effect){
			case 0:
				Hallucination ? 
					pline("The strange symbols stare at you reproachfully.") :
					You("close the Necronomicon.");
			break;
			case SELECT_BYAKHEE:
				pm = &mons[PM_BYAKHEE];
				if(u.uen >= 20){
					losepw(20);
					summon_failed = FALSE;
					if(DimensionalLock){
						pline("%s", nothing_happens);
						break;
					}
					for(i=max(1, d(1,20) - 16); i > 0; i--){
						mtmp = makemon(pm, u.ux, u.uy, MM_EDOG|MM_ADJACENTOK|MM_NOCOUNTBIRTH|MM_ESUM);
						if(mtmp){
							initedog(mtmp);
							mtmp->m_lev += d(1,15) - 5;
							if (rn2((youmonst.summonpwr + mtmp->m_lev) / (u.ulevel + 10) + 1)) {
								untame(mtmp, 0);
								mtmp->mtraitor = 1;
								mtmp->encouraged = 5;
							}
							mtmp->mhpmax = mtmp->mhp = mtmp->m_lev * hd_size(mtmp->data) - hd_size(mtmp->data)/2;
							mark_mon_as_summoned(mtmp, mtmp->mtame ? &youmonst : (struct monst *)0, mtmp->mtame ? ESUMMON_PERMANENT : 100, 0);
						}
					}
				}
			break;
			case SELECT_NIGHTGAUNT:
				pm = &mons[PM_NIGHTGAUNT];
				for(i=d(1,4); i > 0; i--){
					if(u.uen >= 10){
						losepw(10);
						summon_failed = FALSE;
						if(DimensionalLock){
							pline("%s", nothing_happens);
							break;
						}
						mtmp = makemon(pm, u.ux, u.uy, MM_EDOG|MM_ADJACENTOK|MM_NOCOUNTBIRTH|MM_ESUM);
						if(mtmp){
							initedog(mtmp);
							EDOG(mtmp)->loyal = 1;
							mark_mon_as_summoned(mtmp, &youmonst, ESUMMON_PERMANENT, 0);
						}
					}
				}
			break;
			case SELECT_SHOGGOTH:
				if(u.uen > 30){
					losepw(30);
					summon_failed = FALSE;
					if(DimensionalLock){
						pline("%s", nothing_happens);
						break;
					}
					pm = &mons[PM_SHOGGOTH];
					mtmp = makemon(pm, u.ux+d(1,5)-3, u.uy+d(1,5)-3, MM_ADJACENTOK|MM_NOCOUNTBIRTH|MM_ESUM);
					if(mtmp){
						mtmp->mcrazed = 1;
						mtmp->msleeping = 1;
						mark_mon_as_summoned(mtmp, (struct monst *)0, ESUMMON_PERMANENT, 0);
					}
				}
			break;
			case SELECT_OOZE:
				if(u.uen >= 20){
					losepw(20);
					summon_failed = FALSE;
					if(DimensionalLock){
						pline("%s", nothing_happens);
						break;
					}
					for(i=max(1, d(1,10) - 2); i > 0; i--){
						mtmp = makemon(&mons[ROLL_FROM(oozes)], u.ux+d(1,5)-3, u.uy+d(1,5)-3, MM_EDOG|MM_ADJACENTOK|MM_NOCOUNTBIRTH|MM_ESUM);
						if(mtmp){
							initedog(mtmp);
							mtmp->m_lev += d(1,(3 * mtmp->m_lev)/2);
							mtmp->mhpmax = mtmp->mhp = mtmp->m_lev*hd_size(mtmp->data) - rnd(hd_size(mtmp->data)-1);
							if (rn2((youmonst.summonpwr + mtmp->m_lev) / (u.ulevel + 10) + 1)) {
								untame(mtmp, 0);
								mtmp->mtraitor = 1;
							}
							mtmp->mhpmax = mtmp->mhp = mtmp->m_lev * hd_size(mtmp->data) - hd_size(mtmp->data)/2;
							mark_mon_as_summoned(mtmp, mtmp->mtame ? &youmonst : (struct monst *)0, ESUMMON_PERMANENT, 0);
						}
					}
				}
			break;
			case SELECT_DEVIL:
				if(u.uen >= 60){
					losepw(60);
					summon_failed = FALSE;
					if(DimensionalLock){
						pline("%s", nothing_happens);
						break;
					}
					mtmp = makemon(&mons[ROLL_FROM(devils)], u.ux, u.uy, MM_EDOG|MM_ADJACENTOK|MM_NOCOUNTBIRTH|MM_ESUM);
					if(mtmp){
						initedog(mtmp);
						mtmp->m_lev += d(1,(3 * mtmp->m_lev)/2);
						if(!rn2(9)) mtmp->m_lev += d(1,(3 * mtmp->m_lev)/2);
						mtmp->mhpmax = mtmp->mhp = mtmp->m_lev*hd_size(mtmp->data) - rnd(hd_size(mtmp->data)-1);
						if (rn2((youmonst.summonpwr + mtmp->m_lev) / (u.ulevel + 10) + 1)) {
							untame(mtmp, 0);
							mtmp->mtraitor = 1;
						}
						mark_mon_as_summoned(mtmp, mtmp->mtame ? &youmonst : (struct monst *)0, ESUMMON_PERMANENT, 0);
					}
				}
			break;
			case SELECT_DEMON:
				if(u.uen >= 45){
					losepw(45);
					summon_failed = FALSE;
					if(DimensionalLock){
						pline("%s", nothing_happens);
						break;
					}
					mtmp = makemon(&mons[ROLL_FROM(demons)], u.ux, u.uy, MM_EDOG|MM_ADJACENTOK|MM_NOCOUNTBIRTH|MM_ESUM);
					if(mtmp){
						initedog(mtmp);
						if(!rn2(6)) mtmp->m_lev += d(1,(3 * mtmp->m_lev)/2);
						mtmp->mhpmax = mtmp->mhp = mtmp->m_lev*hd_size(mtmp->data) - rnd(hd_size(mtmp->data)-1);
						if (rn2((youmonst.summonpwr + mtmp->m_lev) / (u.ulevel + 10) + 1) || !rn2(10)) {
							untame(mtmp, 0);
							mtmp->mtraitor = 1;
						}
						mark_mon_as_summoned(mtmp, mtmp->mtame ? &youmonst : (struct monst *)0, ESUMMON_PERMANENT, 0);
					}
				}
			break;
		}
		if (summon_failed && necro_effect) {
			pline("You lack the necessary power for the summoning.");
		}
	}
	else if(necro_effect < SELECTED_SPECIAL){ /* spellbook-like */
		switch(necro_effect){
			case SELECT_PROTECTION:
				booktype = SPE_PROTECTION;
			break;
			case SELECT_TURN_UNDEAD:
				booktype = SPE_TURN_UNDEAD;
			break;
			case SELECT_FORCE_BOLT:
				booktype = SPE_FORCE_BOLT;
			break;
			case SELECT_DRAIN_LIFE:
				booktype = SPE_DRAIN_LIFE;
			break;
			case SELECT_DEATH:
				booktype = SPE_FINGER_OF_DEATH;
			break;
			case SELECT_DETECT_MNSTR:
				booktype = SPE_DETECT_MONSTERS;
			break;
			case SELECT_CLAIRVOYANCE:
				booktype = SPE_CLAIRVOYANCE;
			break;
			case SELECT_DETECT_UNSN:
				booktype = SPE_DETECT_UNSEEN;
			break;
			case SELECT_IDENTIFY:
				booktype = SPE_IDENTIFY;
			break;
			case SELECT_CONFUSE:
				booktype = SPE_CONFUSE_MONSTER;
			break;
			case SELECT_CAUSE_FEAR:
				booktype = SPE_CAUSE_FEAR;
			break;
			case SELECT_LEVITATION:
				booktype = SPE_LEVITATION;
			break;
			case SELECT_STONE_FLESH:
				booktype = SPE_STONE_TO_FLESH;
			break;
			case SELECT_CANCELLATION:
				booktype = SPE_CANCELLATION;
			break;
			default:
				impossible("bad necro_effect for necronomicon %d", necro_effect);
				return MOVE_FINISHED_OCCUPATION;
			break;
		}
		Sprintf(splname, objects[booktype].oc_name_known ?
				"\"%s\"" : "the \"%s\" spell",
			OBJ_NAME(objects[booktype]));
		for (i = 0; i < MAXSPELL; i++)  {
			if (spellid(i) == booktype)  {
				if (spellknow(i) <= 1000) {
					Your("knowledge of %s is keener.", splname);
					spl_book[i].sp_know += 20000;
					exercise(A_WIS,TRUE);
				} else {
					You("know %s quite well already.", splname);
				}
				/* make book become known even when spell is already
				   known, in case amnesia made you forget the book */
				makeknown((int)booktype);
				break;
			} else if (spellid(i) == NO_SPELL)  {
				spl_book[i].sp_id = booktype;
				spl_book[i].sp_lev = objects[booktype].oc_level;
				spl_book[i].sp_know = 20000;
				You(i > 0 ? "add %s to your repertoire." : "learn %s.",
					splname);
				makeknown((int)booktype);
				break;
			}
		}
		if (i == MAXSPELL) impossible("Too many spells memorized!");
	}
	else if(necro_effect < SELECT_STUDY){/* special effect */
		switch(necro_effect){
			case SELECT_COMBAT:{
				int skill = 0;
				int newmax; 
				
				if(!uwep){
					skill = P_MARTIAL_ARTS;
					newmax = P_GRAND_MASTER;
				} else if(is_weptool(uwep) || uwep->oclass == WEAPON_CLASS){
					skill = weapon_type(uwep);
					newmax = P_EXPERT;
				}
				else {
					You("aren't holding a weapon.");
					break;
				}
				
				if((skill <= P_NONE || skill > P_LAST_WEAPON) && skill != P_MUSICALIZE && skill != P_MARTIAL_ARTS){
					You("aren't holding a weapon.");
					break;
				}
				
				if(!(P_MAX_SKILL(skill) < newmax)) You("already know how to use your %s.", skill == P_MARTIAL_ARTS ? "fists" : "weapon");
				else if(u.uen >= 100){
					if(skill == P_MARTIAL_ARTS){
						You("grasp the principles of your body's use.");
						u.umartial = TRUE;
					}
					else
						You("grasp the principles of your weapon's use.");
						
					losepw(100);
					u.uenbonus -= 20;
					calc_total_maxen();
					expert_weapon_skill(skill);
					u.weapon_skills[skill].max_skill = newmax;
				} else You("lack the magical energy to cast this incantation.");
			}break;
			case SELECT_HEALTH:
				use_unicorn_horn(artiptr);
			break;
			case SELECT_SIGN:
				if(u.wardsknown & WARD_YELLOW) You("skim the legends of Lost Carcosa");
				else You("read the legends of Lost Carcosa.");
				u.wardsknown |= WARD_YELLOW;
			break;
			case SELECT_WARDS:
				You("review the section on ancient warding signs.");
				u.wardsknown |= WARD_ACHERON|WARD_PENTAGRAM|WARD_HEXAGRAM|WARD_ELDER_SIGN|WARD_HAMSA|WARD_EYE|WARD_QUEEN|WARD_CAT_LORD|WARD_GARUDA;
			break;
			case SELECT_ELEMENTS:
				if( (u.wardsknown & WARD_CTHUGHA) && (u.wardsknown & WARD_ITHAQUA) && (u.wardsknown & WARD_KARAKAL)){
					You("page through the section on the Lords of the Elements");
				}else if((u.wardsknown & WARD_CTHUGHA) || (u.wardsknown & WARD_ITHAQUA) || (u.wardsknown & WARD_KARAKAL)){
					You("refresh your memory about the Lords of the Elements");
				}else{
					You("read about the Lords of the Elements");
				}
				u.wardsknown |= WARD_CTHUGHA|WARD_ITHAQUA|WARD_KARAKAL;
			break;
			case SELECT_SPIRITS1:{
				int i;
				You("read the first half of the testament of whispers.");
				for(i=0; i<16; i++) u.sealsKnown |= sealKey[u.sealorder[i]];
			}break;
			case SELECT_SPIRITS2:{
				int i;
				You("read the second half of the testament of whispers.");
				for(i=15; i<31; i++) u.sealsKnown |= sealKey[u.sealorder[i]];
			}break;
			case SELECT_YOG_SOTHOTH:{
				int i;
				You("read aloud the long chant of opening the gates.");
				if(!(u.specialSealsKnown & SEAL_YOG_SOTHOTH)){
					pline("A seal is engraved into your mind!");
					u.specialSealsKnown |= SEAL_YOG_SOTHOTH;
					u.yog_sothoth_atten = TRUE;
				}
				else if(!(u.specialSealsActive&SEAL_YOG_SOTHOTH)){
					You("percieve a great BEING beyond the gate, and it addresses you with waves of thunderous and burning power.");
					You("are smote and changed by the unendurable violence of its voice!");
					exercise(A_CON, FALSE);
					bindspirit(YOG_SOTHOTH);
					u.sealTimeout[YOG_SOTHOTH-FIRST_SEAL] = moves + 5000;
				}
				else {
					You("are again smote by the unendurable violence the VOICE!");
					if(rnd(5000) > u.spiritT[ALIGN_SPIRIT])
						exercise(A_CON, FALSE);
					u.spiritT[ALIGN_SPIRIT] = moves + 5000;
					u.sealTimeout[YOG_SOTHOTH-FIRST_SEAL] = moves + 5000;
				}
			}break;
		}
	}
	else if(necro_effect == SELECT_STUDY){
		int chance = 0;
//		pline("learned %d abilities",artiptr->spestudied);
		if(!(artiptr->ovar1_necronomicon)) artiptr->spestudied = 0; /* Sanity enforcement. Something wierd is going on with the artifact creation code.*/
//		pline("learned %d abilities",artiptr->spestudied);
		if(d(1,10) - artiptr->spestudied - 15 + ACURR(A_INT) > 0){
			if(!(artiptr->ovar1_necronomicon & LAST_PAGE)){
				chance = d(1,28);
			}
			else if(!(artiptr->ovar1_necronomicon & (S_OOZE|S_SHOGGOTH|S_NIGHTGAUNT|S_BYAKHEE|SUM_DEMON|S_DEVIL) ) && rn2(3)){
				chance = d(1,6);
			}
			else if(!(artiptr->ovar1_necronomicon & (R_YOG_SOTH) ) && !rn2(5)){
				chance = 28;
			}
		}
		if(chance > 0){
			lift_veil();
			if(u.usanity < 100 && rnd(30) < ACURR(A_WIS))
				change_usanity(1, FALSE);
		}
#define STUDY_NECRONOMICON(flag, msg, refndmsg) \
	if(!(artiptr->ovar1_necronomicon & (flag))){\
			You(msg);\
			artiptr->ovar1_necronomicon |= (flag);\
			artiptr->spestudied++;\
			change_uinsight(1);\
			if(artiptr->spestudied > rn1(4,7)){\
				artiptr->ovar1_necronomicon|= LAST_PAGE;\
			}\
		}else{\
			You(refndmsg);\
	}
		switch(chance){
			case 0:
				You("find only %s.",
					!rn2(2) ? "ravings" :
					!rn2(5) ? "deranged ravings" :
					!rn2(4) ? "bloodstains" :
					!rn2(3) ? "writings in Deep Mandaic" :
					!rn2(2) ? "fever-dreams" :
							  "names of nameless horrors");
				exercise(A_WIS, FALSE);
				exercise(A_WIS, FALSE);
				exercise(A_INT, FALSE);
				if(rn2(100) < u.usanity)
					change_usanity(-1, FALSE);
			break;
			case 1:
			STUDY_NECRONOMICON(S_OOZE,
				"learn how to bend the myriad amoeboid monsters of earth to your will.",
				"find another incantation for controling amoeboid monsters.");
			break;
			case 2:
			STUDY_NECRONOMICON(S_SHOGGOTH,
				"find a spell for summoning shoggoths.",
				"find another way to summon shoggoths.");
			break;
			case 3:
			STUDY_NECRONOMICON(S_NIGHTGAUNT,
				"learn to invoke the servetors of Nodens.",
				"find another invocation to Nodens.");
			break;
			case 4:
			STUDY_NECRONOMICON(S_BYAKHEE,
				"learn to call down byakhees.",
				"find another way to call down byakhees.");
			break;
			case 5:
			STUDY_NECRONOMICON(SUM_DEMON,
				"learn to call the children of the abyss.",
				"find another way to summon demons.");
			break;
			case 6:
			STUDY_NECRONOMICON(S_DEVIL,
				"learn to free the prisoners of hell.",
				"find another way to bind devils.");
			break;
			case 7:
			STUDY_NECRONOMICON(SP_PROTECTION,
				"find a spell of protection.",
				"find another spell of protection.");
			break;
			case 8:
			STUDY_NECRONOMICON(SP_TURN_UNDEAD,
				"find a passage concerning the secrets of life and death.",
				"find another passage concerning life and death.");
			break;
			case 9:
			STUDY_NECRONOMICON(SP_FORCE_BOLT,
				"find a passage detailing the invisible forces.",
				"find another passage describing the invisible forces.");
			break;
			case 10:
			STUDY_NECRONOMICON(SP_DRAIN_LIFE,
				"find a passage concerning the secrets of time and decay.",
				"find another passage concerning time and decay.");
			break;
			case 11:
			STUDY_NECRONOMICON(SP_DEATH,
				"find a passage detailing the arts of the Reaper.",
				"find another passage concerning the Reaper's arts.");
			break;
			case 12:
			STUDY_NECRONOMICON(SP_DETECT_MNSTR,
				"find a passage teaching monster detection.",
				"find an alternative school of monster detection.");
			break;
			case 13:
			STUDY_NECRONOMICON(SP_CLAIRVOYANCE,
				"learn the secret of clairvoyance.",
				"find another reference to the secret of clairvoyance.");
			break;
			case 14:
			STUDY_NECRONOMICON(SP_DETECT_UNSN,
				"learn the ways of unseen things.",
				"find another reference to the ways of unseen things.");
			break;
			case 15:
			STUDY_NECRONOMICON(SP_IDENTIFY,
				"discover the secret of knowledge without learning.",
				"already know the secret of knowledge without learning.");
			break;
			case 16:
			STUDY_NECRONOMICON(SP_CONFUSE,
				"learn the ways of confusion.",
				"learn more about the ways of confusion.");
			break;
			case 17:
			STUDY_NECRONOMICON(SP_CAUSE_FEAR,
				"learn the ways of fear.",
				"learn more about the ways of fear.");
			break;
			case 18:
			STUDY_NECRONOMICON(SP_LEVITATION,
				"learn the true nature of gravity.",
				"already know the true nature of gravity.");
			break;
			case 19:
			STUDY_NECRONOMICON(SP_STONE_FLESH,
				"learn the true name of the earth mother.",
				"already know the true name of the earth mother.");
			break;
			case 20:
			STUDY_NECRONOMICON(SP_CANCELLATION,
				"learn the weaknesses of the cosmic weave.",
				"learn a few more weaknesses of the cosmic weave.");
			break;
			case 21:
			STUDY_NECRONOMICON(SP_COMBAT,
				"learn the secrets of war and killing.",
				"already know the secrets of war and killing.");
			break;
			case 22:
			STUDY_NECRONOMICON(SP_HEALTH,
				"learn the secret to health and recovery.",
				"find another secret to health and recovery.");
			break;
			case 23:
			STUDY_NECRONOMICON(R_Y_SIGN,
				"discover the legends of Lost Carcosa.",
				"find another section on Lost Carcosa.");
			break;
			case 24:
			STUDY_NECRONOMICON(R_WARDS,
				"discover a treatise on ancient warding signs.",
				"discover another treatise on ancient warding signs.");
			break;
			case 25:
			STUDY_NECRONOMICON(R_ELEMENTS,
				"discover a section detailing the Lords of the Elements.",
				"discover another section on the Lords of the Elements.");
			break;
			case 26:
			STUDY_NECRONOMICON(R_NAMES_1,
				(artiptr->ovar1_necronomicon & R_NAMES_2) ? "find the first half of the testament of whispers."
					: "come across the first half of something called 'the testament of whispers.'",
					"discover another transcription of the first half of the testament of whispers.");
			break;
			case 27:
			STUDY_NECRONOMICON(R_NAMES_2,
				(artiptr->ovar1_necronomicon & R_NAMES_1) ? "find the second half of the testament of whispers."
					: "come across the latter half of something called 'the testament of whispers.'",
				"discover another transcription of the latter half of the testament of whispers.");
			break;
			case 28:
			STUDY_NECRONOMICON(R_YOG_SOTH,
				"discover the long chant of opening the gates.",
				"discover another copy of the long chant.");
			break;
		}
#undef STUDY_NECRONOMICON
//		artiptr->ovar1_necronomicon = ~0; //debug
	}
	else{
		pline("Unrecognized Necronomicon effect.");
	}
	artiptr = 0;
	return MOVE_FINISHED_OCCUPATION;
}

STATIC_PTR int
read_lost(VOID_ARGS)
{
	int i, numSlots;

	if (Confusion) {		/* became confused while learning */
//	    (void) confused_book(book);
	    artiptr = 0;		/* no longer studying */
	    nomul(delay, "struggling with the Book of Lost Names");		/* remaining delay is uninterrupted */
		losexp("getting lost in a book",TRUE,TRUE,TRUE);
	    delay = 0;
	    return MOVE_FINISHED_OCCUPATION;
	}
	if(u.veil && delay >= -55){
		You("feel reality threatening to slip away!");
		if (yn("Are you sure you want to keep reading?") != 'y'){
			delay = 0;
			return(0);
		}
		else pline("So be it.");
		u.veil = FALSE;
		change_uinsight(1);
	}
	if (delay) {	/* not if (delay++), so at end delay == 0 */
	/* lenses give 50% faster reading */
//	    nomul( (ublindf && ublindf->otyp == LENSES) ? 
//			(-26+ACURR(A_INT))/2 :
//			(-26+ACURR(A_INT))); /* Necronomicon is difficult to put down */
//	    delay -= -26+ACURR(A_INT); /* - a negative number is + */
		delay++;
		if(ublindf && ublindf->otyp == LENSES) delay++;
		if(delay < 0){
			return MOVE_READ; /* still busy */
		}
		delay = 0;
	}
	if(lostname < QUEST_SPIRITS){/* Bind spirit */
		if(Role_if(PM_EXILE)){
			if(u.ulevel <= 2) numSlots=1;
			else if(u.ulevel <= 9) numSlots=2;
			else if(u.ulevel <= 18) numSlots=3;
			else if(u.ulevel <= 25) numSlots=4;
			else numSlots=5;
		} else {
			numSlots=1;
		}
		// for(i=0;i<QUEST_SPIRITS;i++){
			// pline("#%d:%s",i,sealNames[i]);
		// }
		pline("Using the rituals in the book, you attempt to form a bond with %s.",sealNames[lostname-FIRST_SEAL]);
		if(u.sealCounts < numSlots){
			bindspirit(lostname);
			u.sealsKnown |= (1L << lostname);
			You("feel %s within your soul.",sealNames[lostname-FIRST_SEAL]);
		} else You("can't feel the spirit.");
	}
	else if(lostname == QUEST_SPIRITS){
		long putativeSeal;
		if(!(artiptr->ovar1_lostNames)) artiptr->spestudied = 0; /* Sanity enforcement. Something wierd is going on with the artifact creation code.*/

		i = rn2(31);
		putativeSeal = 1L << i;
		if(artiptr->ovar1_lostNames & putativeSeal){
			losexp("getting lost in a book",TRUE,TRUE,TRUE);
			if(rn2(100) < u.usanity)
				change_usanity(-1, FALSE);
		} else {
			u.sealsKnown |= putativeSeal;
			artiptr->ovar1_lostNames |= putativeSeal;
			You("learn the name \"%s\" while studying the book.",sealNames[i]);
			artiptr->spestudied++;
			u.veil = FALSE;
			if(!u.uboln || !rn2(u.uboln)){
				change_uinsight(1);
			}
			u.uboln++;
			if(u.usanity < 100 && rnd(30) < ACURR(A_WIS))
				change_usanity(1, FALSE);
		}
	}
	else{
		pline("Unrecognized Lost Names effect.");
	}
	artiptr = 0;
	return MOVE_FINISHED_OCCUPATION;
}

/*
 * Artifact is dipped into water
 * -1 not handled here (not used up here)
 *  0 no effect but used up
 *  else return
 *  AD_FIRE, etc.
 *  Note caller should handle what happens to the medium in these cases.
 *      This only prints messages about the actual artifact.
 */

int
artifact_wet(obj, silent)
struct obj *obj;
boolean silent;
{
	int adtyp = 0;

	if (is_lightsaber(obj) && litsaber(obj) && obj->otyp != ROD_OF_FORCE)
		adtyp = (obj->otyp == KAMEREL_VAJRA ? AD_ELEC : AD_FIRE);
	else if (obj->oartifact)
		adtyp = artilist[(int)(obj)->oartifact].adtyp;

	switch (adtyp) 
	{
	case AD_FIRE:
		if (!silent) {
			pline("A cloud of steam rises.");
			pline("%s is untouched.", The(xname(obj)));
		}
		return (AD_FIRE);
	case AD_COLD:
		if (!silent) {
			pline("Icicles form and fall from the freezing %s.",
				xname(obj));
		}
		return (AD_COLD);
	case AD_ELEC:
		if (!silent) {
			pline_The("humid air crackles with electricity from %s.",
				the(xname(obj)));
		}
		return (AD_ELEC);
	case AD_DRLI:
		if (!silent) {
			pline("%s absorbs the water!", The(xname(obj)));
		}
		return (AD_DRLI);
	default:
		break;
	}
	return (-1);
}

/* WAC return TRUE if artifact is always lit */
boolean
artifact_light(obj)
struct obj *obj;
{

	if (obj && obj->oartifact == ART_PEN_OF_THE_VOID && obj->ovara_seals&SEAL_JACK) return TRUE;

	return (obj && obj->oartifact && arti_is_prop(obj, ARTI_LIGHT));
}

/* return TRUE if artifact is permanently lit */
boolean
arti_light(obj)
    struct obj *obj;
{
	const struct artifact *arti = get_artifact(obj);
    return	(arti && !is_lightsaber(obj) &&
			arti_is_prop(obj, ARTI_PERMALIGHT)
			);
}

/* KMH -- Talking artifacts are finally implemented */
void
arti_speak(obj)
    struct obj *obj;
{
	register const struct artifact *oart = get_artifact(obj);
	const char *line;
	char buf[BUFSZ];


	/* Is this a speaking artifact? */
	if (!oart || !arti_is_prop(obj, ARTI_SPEAK))
		return;

	line = getrumor(bcsign(obj), buf, TRUE);
	if (!*line)
		line = "NetHack rumors file closed for renovation.";
	pline("%s:", Tobjnam(obj, "whisper"));
	verbalize("%s", line);
	return;
}

boolean
artifact_has_invprop(otmp, inv_prop)
struct obj *otmp;
uchar inv_prop;
{
	const struct artifact *arti = get_artifact(otmp);

	return((boolean)(arti && (arti->inv_prop == inv_prop)));
}

/* Return the price sold to the hero of a given artifact or unique item */
long
arti_cost(otmp)
struct obj *otmp;
{
	if (!otmp->oartifact)
	    return ((long)objects[otmp->otyp].oc_cost);
	else if (artilist[(int) otmp->oartifact].cost)
	    return (artilist[(int) otmp->oartifact].cost);
	else
	    return (100L * (long)objects[otmp->otyp].oc_cost);
}

static const char *random_seasound[] = {
	"distant waves",
	"distant surf",
	"the distant sea",
	"the call of the ocean",
	"waves against the shore",
	"flowing water",
	"the sighing of waves",
	"quarrelling gulls",
	"the song of the deep",
	"rumbling in the deeps",
	"the singing of Eidothea",
	"the laughter of the protean nymphs",
	"rushing tides",
	"the elusive sea change",
	"the silence of the sea",
	"the passage of the albatross",
	"dancing raindrops",
	"coins rolling on the seabed",
	"treasure galleons crumbling in the depths",
	"waves lapping against a hull"
};

/* Polymorph obj contents */
void
arti_poly_contents(obj)
    struct obj *obj;
{
    struct obj *dobj = 0;  /*object to be deleted*/
    struct obj *otmp;
	You_hear("%s.",random_seasound[rn2(SIZE(random_seasound))]);
	for (otmp = obj->cobj; otmp; otmp = otmp->nobj){
		if (dobj) {
			delobj(dobj);
			dobj = 0;
		}
		if(!obj_resists(otmp, 0, 100)){
			/* KMH, conduct */
			u.uconduct.polypiles++;
			/* any saved lock context will be dangerously obsolete */
			if (Is_box(otmp)) (void) boxlock(otmp, obj);

			if (obj_shudders(otmp)) {
				dobj = otmp;
			}
			else otmp = randpoly_obj(otmp);
		}
	}
	if (dobj) {
		delobj(dobj);
		dobj = 0;
	}
}
#endif /* OVLB */

int
throweffect()
{
	coord cc;

	if (u.uinwater) {
	    pline("You're joking! In this weather?"); return 0;
	} else if (Is_waterlevel(&u.uz)) {
	    You("had better wait for the sun to come out."); return 0;
	}

	pline("Where do you want to cast the spell?");
	cc.x = u.ux;
	cc.y = u.uy;
	if (getpos(&cc, TRUE, "the desired position") < 0)
	    return 0;	/* user pressed ESC */
	/* The number of moves from hero to where the spell drops.*/
	if (distmin(u.ux, u.uy, cc.x, cc.y) > 10) {
	    pline_The("spell dissipates over the distance!");
	    return 0;
	} else if (u.uswallow) {
	    pline_The("spell is cut short!");
	    exercise(A_WIS, FALSE); /* What were you THINKING! */
	    u.dx = 0;
	    u.dy = 0;
	    return 1;
	} else if (!cansee(cc.x, cc.y) || IS_STWALL(levl[cc.x][cc.y].typ)) {
	    Your("mind fails to lock onto that location!");
	    return 0;
	} else {
	    u.dx=cc.x;
	    u.dy=cc.y;
	    return 1;
	}
}

STATIC_OVL void
cast_protection()
{
	int loglev = 0;
	int l = u.ulevel;
	int natac = u.uac - u.uspellprot;
	int gain;

	/* loglev=log2(u.ulevel)+1 (1..5) */
	while (l) {
	    loglev++;
	    l /= 2;
	}

	/* The more u.uspellprot you already have, the less you get,
	 * and the better your natural ac, the less you get.
	 */
	gain = loglev - (int)u.uspellprot / (4 - min(3,(10 - natac)/10));

	if (gain > 0) {
	    if (!Blind) {
		const char *hgolden = hcolor(NH_GOLDEN);

		if (u.uspellprot)
		    pline_The("%s haze around you becomes more dense.",
			      hgolden);
		else
		    pline_The("%s around you begins to shimmer with %s haze.",
			/*[ what about being inside solid rock while polyd? ]*/
			(Underwater || Is_waterlevel(&u.uz)) ? "water" : "air",
			      an(hgolden));
	    }
	    u.uspellprot += gain;
	    u.uspmtime =
			P_SKILL(spell_skilltype(SPE_PROTECTION)) == P_EXPERT ? 50 : 20;
	    if (!u.usptime)
			u.usptime = 100;
	    find_ac();
	} else {
	    Your("skin feels warm for a moment.");
	}
}
STATIC_OVL void
awaken_monsters(distance)
int distance;
{
	register struct monst *mtmp = fmon;
	register int distm;

	while(mtmp) {
	    if (!DEADMONSTER(mtmp)) {
		distm = distu(mtmp->mx, mtmp->my);
		if (distm < distance) {
		    mtmp->msleeping = 0;
		    mtmp->mcanmove = 1;
		    mtmp->mfrozen = 0;
		}
	    }
	    mtmp = mtmp->nmon;
	}
}

//For use with the level editor and elsewhere
struct obj *
minor_artifact(otmp, name)
struct obj *otmp;	/* existing object; used if not name */
char *name;	/* target name or ""*/
{
	if(!strcmp(name,  "Mistlight")){
		if(!rn2(4)) otmp->otyp = LONG_SWORD;
		
		if(rn2(2)) set_material_gm(otmp, SILVER);
		else set_material_gm(otmp, GOLD);
		
		switch(rnd(4)){
			case 1:
				add_oprop(otmp, OPROP_HOLYW);
			break;
			case 2:
				add_oprop(otmp, OPROP_AXIOW);
			break;
			case 3:
				add_oprop(otmp, OPROP_MAGCW);
			break;
			case 4:
				add_oprop(otmp, OPROP_WATRW);
			break;
		}
	} else if (!strcmp(name,  "Inherited")){
		rem_ox(otmp, OX_ENAM);
		if (flags.descendant){
			struct obj *art;
			art = mksartifact(u.inherited);
			if(art){
				discover_artifact(u.inherited);
				fully_identify_obj(art);
				if(Is_real_container(otmp)){
					add_to_container(otmp, art);
				}
				else {
					place_object(art, otmp->ox, otmp->oy);
				}
			}
			else impossible("Delayed inheritance failed in minor_artifact.");
		}
	}
	return otmp;
}

struct blast_element {
	int spe;
	struct blast_element *nblast;
};

void
dosymbiotic_equip()
{
	struct monst *mtmp;
	struct obj *obj;
	for(obj = invent; obj; obj = obj->nobj){
		if(obj->owornmask && check_oprop(obj, OPROP_HEAL))
			doliving_healing_armor(&youmonst, obj, FALSE);
		if(obj->otyp == ARMOR_SALVE)
			doliving_armor_salve(&youmonst, obj);
	}
	if(uarm && (uarm->otyp == LIVING_ARMOR || uarm->otyp == BARNACLE_ARMOR))
		dosymbiotic(&youmonst, uarm);
	if(uarm && uarm->oartifact == ART_SCORPION_CARAPACE && check_carapace_mod(uarm, CPROP_WHIPPING))
		doscorpion(&youmonst, uarm);
	if(uwep && ((check_oprop(uwep, OPROP_LIVEW) && Insight >= 40)
				|| is_living_artifact(uwep)
				|| is_bloodthirsty_artifact(uwep) 
				|| (uwep->oartifact == ART_DIRGE && check_mutation(SHUB_TENTACLES) && roll_generic_flat_madness(FALSE))
	))
		doliving(&youmonst, uwep);
	if(uswapwep && ((check_oprop(uswapwep, OPROP_LIVEW) && u.twoweap && Insight >= 40) || is_living_artifact(uswapwep) || (is_bloodthirsty_artifact(uswapwep) && u.twoweap) ))
		doliving(&youmonst, uswapwep);
	if(uarms && ((check_oprop(uarms, OPROP_LIVEW) && Insight >= 40) || is_living_artifact(uarms) ))
		doliving(&youmonst, uarms);
	if(uarm && ((check_oprop(uarm, OPROP_LIVEW) && Insight >= 40) || is_living_artifact(uarm) ))
		doliving(&youmonst, uarm);
	if(uarmh && ((check_oprop(uarmh, OPROP_LIVEW) && Insight >= 40) || is_living_artifact(uarmh) ))
		doliving(&youmonst, uarmh);
	if(check_mutation(ABHORRENT_SPORE)){
		if(uwep && uwep->oartifact == ART_DIRGE && roll_generic_flat_madness(FALSE))
			dogoat();
		if(uswapwep && uswapwep->oartifact == ART_DIRGE && roll_generic_flat_madness(FALSE))
			dogoat();
	}
	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon){
		if(DEADMONSTER(mtmp))
			continue;
		for(obj = mtmp->minvent; obj; obj = obj->nobj){
			if(obj->owornmask && check_oprop(obj, OPROP_HEAL))
				doliving_healing_armor(mtmp, obj, FALSE);
		}
		obj = which_armor(mtmp, W_ARM);
		if(obj && (obj->otyp == LIVING_ARMOR || obj->otyp == BARNACLE_ARMOR))
			dosymbiotic(mtmp, obj);
		if(obj && obj->oartifact == ART_SCORPION_CARAPACE && check_carapace_mod(obj, CPROP_WHIPPING))
			doscorpion(mtmp, obj);
		
		obj = MON_WEP(mtmp);
		if(obj && ((check_oprop(obj, OPROP_LIVEW) && Insight >= 40) || is_living_artifact(obj) ))
			doliving(mtmp, obj);
		obj = MON_SWEP(mtmp);
		if(obj && ((check_oprop(obj, OPROP_LIVEW) && Insight >= 40) || is_living_artifact(obj) ))
			doliving(mtmp, obj);
		obj = which_armor(mtmp, W_ARMS);
		if(obj && ((check_oprop(obj, OPROP_LIVEW) && Insight >= 40) || is_living_artifact(obj) ))
			doliving(mtmp, obj);
		obj = which_armor(mtmp, W_ARM);
		if(obj && ((check_oprop(obj, OPROP_LIVEW) && Insight >= 40) || is_living_artifact(obj) ))
			doliving(mtmp, obj);
		obj = which_armor(mtmp, W_ARMH);
		if(obj && ((check_oprop(obj, OPROP_LIVEW) && Insight >= 40) || is_living_artifact(obj) ))
			doliving(mtmp, obj);
	}
}

void
do_passive_attacks()
{
	struct monst *mtmp;
	struct obj *armor;
	if(Invulnerable)
		return;

	if(!u.uno_auto_attacks){
		if(is_goat_tentacle_mtyp(youracedata))
			dogoat();
		if(u.specialSealsActive&SEAL_YOG_SOTHOTH){
			if(!doyog(&youmonst)){
				if(check_mutation(TWIN_MIND) && roll_generic_madness(FALSE))
					dotwin_cast(&youmonst);
			}
		}
		if(is_snake_bite_mtyp(youracedata))
			dosnake(&youmonst);
		if(u.jellyfish && active_glyph(LUMEN))
			dojellysting(&youmonst);
		if(is_tailslap_mtyp(youracedata))
			dotailslap(&youmonst);
		if(uring_art(ART_STAR_EMPEROR_S_RING))
			dostarblades(&youmonst);
		if(check_rot(ROT_CENT))
			dorotbite(&youmonst);
		if(check_rot(ROT_STING))
			dorotsting(&youmonst);
	}
	if(check_mutation(SHUB_TENTACLES)){
		//Note: If we have the mutation we know that we have the madness, and if we somehow don't, well...
		//Also, only able to control the attacks if you have the mutation
		if(!u.uno_auto_attacks && roll_generic_madness(FALSE) && adjacent_mon()){
			if(!ClearThoughts && youracedata->mtyp != PM_DARK_YOUNG)
				morehungry(d(4,4)*get_uhungersizemod());
			dogoat();
		}
	}
	else if(roll_madness(MAD_GOAT_RIDDEN) && adjacent_mon()){
		if(!ClearThoughts && youracedata->mtyp != PM_DARK_YOUNG){
			pline("Lashing tentacles erupt from your brain!");
			losehp(max(1,(Upolyd ? ((d(4,4)*u.mh)/u.mhmax) : ((d(4,4)*u.uhp)/u.uhpmax))), "the black mother's touch", KILLED_BY);
			morehungry(d(4,4)*get_uhungersizemod());
		}
		dogoat();
	}
	
	//Note: The player never gets Eladrin vines, starblades, or storms
	flags.mon_moving = TRUE;
	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon){
		if(DEADMONSTER(mtmp))
			continue;
		if(!mtmp->mappearance && !nonthreat(mtmp) && !mtmp->msleeping && mtmp->mcanmove && !(mtmp->mstrategy & STRAT_WAITMASK)){
			if(is_goat_tentacle_mon(mtmp))
				dogoat_mon(mtmp);
			if(is_snake_bite_mon(mtmp))
				dosnake(mtmp);
			if(mtmp->mtyp == PM_TWIN_SIBLING)
				doyog(mtmp);
			if(is_tailslap_mon(mtmp))
				dotailslap(mtmp);
			if(is_vines_mon(mtmp))
				dovines(mtmp);
			if(is_star_blades_mon(mtmp))
				dostarblades(mtmp);
			if(mtmp->mtyp == PM_SHALOSH_TANNAH)
				dohost_mon(mtmp);
			if(is_storm_mon(mtmp))
				dostorm(mtmp);
			if(is_chain_lash_mon(mtmp))
				dochain_lashes(mtmp);
			if(mtmp->mtyp == PM_KRAKEN__THE_FIEND_OF_WATER)
				dokraken_mon(mtmp);
			if(mtmp->mtyp == PM_CHAOS && !PURIFIED_WATER)
				dochaos_mon(mtmp);
		}
		if(mtmp->mtyp == PM_NACHASH_TANNIN){
			donachash(mtmp);
		}
	}
	flags.mon_moving = FALSE;
}

void
living_items()
{
	struct monst *mtmp, *nmon = (struct monst *)0;
	struct obj *obj, *nobj;
	struct blast_element *blast_list = 0, *nblast = 0;
	int whisper = 0;
	if(Invulnerable)
		return;
	
	//collect all the blasting items into a list so that they can blast away without worrying about changing the state of the dungeon.

	/* does not search migrating monsters' inventories nor migrating objects nor magic chests */
	int owhere = ((1 << OBJ_FLOOR) |
				(1 << OBJ_INVENT) |
				(1 << OBJ_MINVENT) |
				(1 << OBJ_BURIED) |
				(1 << OBJ_CONTAINED) |
				(1 << OBJ_INTRAP));

	for (obj = start_all_items(&owhere); obj; obj = next_all_items(&owhere)) {
		/* collect blasting items into a list so they can happen AFTER collecting them all,
		 * and so not changing the state of the dungeon (which would cause items to be missed) */
		if (check_oprop(obj, OPROP_DEEPW) && obj->spe < 8){
			nblast = (struct blast_element *)malloc(sizeof(struct blast_element));
			nblast->nblast = blast_list;
			nblast->spe = obj->spe;
			blast_list = nblast;
		}
		/* find number of faceless statues */
		if (obj->otyp == STATUE && (obj->spe&STATUE_FACELESS)){
			whisper++;
		}
		/* acidify self-acidifying objects */ 
		if (check_oprop(obj, OPROP_ASECW)){
			if ((!(obj->opoisoned&OPOISON_ACID) ||
				((obj->otyp == VIPERWHIP || obj->otyp == find_signet_ring()) && obj->opoisonchrgs < 3)
				) && !rn2(40)
				){
				obj->opoisoned |= OPOISON_ACID;
				if ((obj->otyp == VIPERWHIP || obj->otyp == find_signet_ring()) && obj->opoisonchrgs < 3){
					obj->opoisonchrgs++;
				}
			}
		}
		/* poison self-poisoning objects */
		if (check_oprop(obj, OPROP_PSECW)){
			if ((!(obj->opoisoned&OPOISON_BASIC) ||
				((obj->otyp == VIPERWHIP || obj->otyp == find_signet_ring()) && obj->opoisonchrgs < 3)
				) && !rn2(40)
				){
				obj->opoisoned |= OPOISON_BASIC;
				if ((obj->otyp == VIPERWHIP || obj->otyp == find_signet_ring()) && obj->opoisonchrgs < 3){
					obj->opoisonchrgs++;
				}
			}
		}
		/* grease self-greasing objects */
		if (check_oprop(obj, OPROP_GRES) && !obj->greased && !rn2(40)){
			obj->greased = TRUE;
		}
		/* Isamusei may change collor */
		if (obj->otyp == ISAMUSEI){
			int oldColor = obj->obj_color;
			set_isamusei_color(obj);
			if(oldColor != obj->obj_color && obj->where == OBJ_FLOOR){
				if(isok(obj->ox, obj->oy)){
					newsym(obj->ox, obj->oy);
				}
			}
		}
	}

	/* now do the item blasts -- this had to be delayed -- what if a blast killed a monster,
	 * and its items fell onto the floor after we had already checked the floor's items? */
	for(nblast = blast_list; nblast; nblast = nblast->nblast){
		do_item_blast(nblast->spe);
	}
	/* free the item-blast list */
	while (blast_list){
		nblast = blast_list;
		blast_list = blast_list->nblast;
		free(nblast);
	}

	const char * whispers[4][4] =
	{
		/* whisper >= 10 */
		{ "murmuring babble surrounding you", "whispering babble all around you", "faint singing", "soft sobbing all around you" },
		/* whisper >= 5 */
		{ "faint murmuring", "whispering babble", "distant music", "soft sobbing" },
		/* >= 2 */
		{ "isolated murmurs", "scattered whispers", "faint snatches of songs", "soft sobbing" },
		/* == 1 */
		{ "someone murmuring", "someone whispering", "someone humming", "someone crying" }
	};

	/* print whisper message from the faceless statues */
	if(whisper && !rn2(20)){
		int i = (whisper >= 10) + (whisper >= 5) + (whisper >= 2);
		You_hear("%s.", whispers[i][rn2(4)]);
	}

	/* Animate objects in the dungeon -- this only happens to items in one chain (floor) and it changes the state of the dungeon,
	 * so it's convenient not to handle this in the all_items() loop */
	extern const int monstr[];
	if(!Inhell && (level_difficulty()+u.ulevel)/2 > monstr[PM_STONE_GOLEM] && check_insight()){
		for (obj = fobj; obj; obj = nobj) {
			nobj = obj->nobj;
			if(obj->otyp == STATUE && !get_ox(obj, OX_EMON) && !(obj->spe)){
				mtmp = animate_statue(obj, obj->ox, obj->oy, ANIMATE_NORMAL, (int *) 0);
				if(mtmp){
					set_template(mtmp, TOMB_HERD);
					mtmp->m_lev += 4;
					mtmp->mhpmax += d(4, 8);
					mtmp->mhp = mtmp->mhpmax;
					mtmp->mpeaceful = 0;
					untame(mtmp, 0); //Note: should never happen, since we check for the absense of emon
					set_malign(mtmp);
					// mtmp->m_ap_type = M_AP_OBJECT;
					// mtmp->mappearance = STATUE;
					// mtmp->m_ap_type = M_AP_MONSTER;
					// mtmp->mappearance = PM_STONE_GOLEM;
					newsym(mtmp->mx, mtmp->my);
				}
			}
		}
	}
	for (obj = fobj; obj; obj = nobj) {
		nobj = obj->nobj;
		if((obj->otyp == BROKEN_ANDROID || obj->otyp == BROKEN_GYNOID || obj->otyp == LIFELESS_DOLL) && obj->ovar1_insightlevel){
			xchar ox, oy;
			get_obj_location(obj, &ox, &oy, 0);
			if(obj->ovar1_insightlevel <= Insight && !rn2(20)){
				struct monst *mtmp;
				mtmp = revive(obj, TRUE);
				if (mtmp && cansee(ox, oy)) pline("%s wakes up.", Monnam(mtmp));
			} else if(cansee(ox, oy) && !rn2(20))
				pline("%s its finger.", Tobjnam(obj, "tap"));
		}
		else if(obj->otyp == CORPSE && obj->corpsenm == PM_CROW_WINGED_HALF_DRAGON){
			xchar ox, oy;
			get_obj_location(obj, &ox, &oy, 0);
			if(cansee(ox, oy) && !rn2(20))
				pline("%s.", Tobjnam(obj, "blink"));
		}
		//Nitocris's coffin causes Egyptian spawns
		else if(Is_real_container(obj) && obj->spe == 5){
			nitocris_sarcophagous(obj);
		}
		//Fulvous desk spawns phantoms
		else if(Is_real_container(obj) && obj->spe == 8){
			fulvous_desk(obj);
		}
	}
}


static int nitocrisspawns[] = {PM_PIT_VIPER, PM_PIT_VIPER, PM_COBRA, PM_COBRA, PM_GHOUL, PM_GHOUL, PM_CROCODILE, PM_CROCODILE, PM_SERPENT_NECKED_LIONESS, PM_AMMIT};

STATIC_OVL void
nitocris_sarcophagous(obj)
struct obj *obj;
{
	struct permonst *pm;
	struct monst *mtmp;
	xchar xlocale, ylocale;
	if(get_obj_location(obj, &xlocale, &ylocale, 0)){
		if(!rn2(70)){
			struct permonst *pm;
			pm = &mons[nitocrisspawns[rn2(SIZE(nitocrisspawns))]];
			mtmp = makemon(pm, xlocale, ylocale, MM_ADJACENTOK|MM_NOCOUNTBIRTH);
			if(mtmp){
				mtmp->mpeaceful = 0;
				set_malign(mtmp);
			}
		}
	}
}

static int fulvousspawns[] = {PM_GHOST, PM_GHOST, PM_GHOST, PM_GHOST, 
							  PM_WRAITH, PM_WRAITH, PM_WRAITH, 
							  PM_SHADE};
STATIC_OVL void
fulvous_desk(obj)
struct obj *obj;
{
	struct permonst *pm;
	struct monst *mtmp;
	xchar xlocale, ylocale;
	if(get_obj_location(obj, &xlocale, &ylocale, 0)){
		if(!rn2(210)){
			struct permonst *pm;
			pm = &mons[fulvousspawns[rn2(SIZE(fulvousspawns))]];
			mtmp = makemon(pm, xlocale, ylocale, MM_ADJACENTOK|MM_NOCOUNTBIRTH|MM_NONAME);
			if(!obj->cobj){
				struct obj *sobj;
				sobj = mksobj(SCR_BLANK_PAPER, NO_MKOBJ_FLAGS);
				if(sobj){
					sobj->obj_color = CLR_YELLOW;
					add_to_container(obj, sobj);
				}
			}
			if(mtmp){
				mtmp->mpeaceful = 0;
				set_faction(mtmp, YELLOW_FACTION);
				set_malign(mtmp);
				switch(rn2(4)){
					case 0:
					case 1:
						set_template(mtmp, YELLOW_TEMPLATE);
					break;
					case 2:
						set_template(mtmp, DREAM_LEECH);
					break;
					case 3:
						set_template(mtmp, VAMPIRIC);
					break;
				}
			}
		}
	}
}

STATIC_OVL void
do_item_blast(spe)
int spe;
{
	struct monst *m2, *nmon2 = (struct monst *)0;
	int dsize = 15 - spe*2;
	int dnum = 1;
	int dmg;
		
	if(dsize >= 15)
		dnum = 5;
	else if(dsize > 10)
		dnum = 3;
	else if(dsize > 5)
		dnum = 2;
	
	if(dnum >= 5){
		if(rn2(20))
			return;
	} else {
		if(rn2(40))
			return;
	}
	
	switch(rn2(10)){
		case 0:
		pline("You hear grumbling in the deeps.");
		break;
		case 1:
		pline("You hear muttering in the dark.");
		break;
		case 2:
		pline("You hear singing in the depths.");
		break;
		case 3:
		pline("You hear chattering among the stars.");
		break;
		case 4:
		pline("You hear ringing in your %s.", body_part(EARS));
		break;
		case 5:
		pline("You hear silence in heaven.");
		break;
		case 6:
		pline("You hear sighing in the wind.");
		break;
		case 7:
		pline("You hear teardrops on the sea.");
		break;
		case 8:
		pline("You hear the wind in the abyss.");
		break;
		case 9:
		pline("You hear a creaking in the sky.");
		break;
	}
	if (!Tele_blind && (Unblind_telepat || (Blind_telepat && Blind) || (Blind_telepat && rn2(2)) || !rn2(10))) {
		pline("It locks on to your %s!",
			(Unblind_telepat || (Blind_telepat && Blind)) ? "telepathy" :
			Blind_telepat ? "latent telepathy" : "mind");
		dmg = d(dnum, dsize);
		dmg = reduce_dmg(&youmonst,dmg,FALSE,TRUE);
		losehp(dmg, "psychic blast", KILLED_BY_AN);
		if(dnum >= 3){
			for (m2 = fmon; m2; m2 = nmon2) {
				nmon2 = m2->nmon;
				if (!DEADMONSTER(m2) && !m2->mpeaceful && (mon_resistance(m2,TELEPAT) || rn2(2)))
				{
					m2->msleeping = 0;
					if (!m2->mcanmove && !rn2(5)) {
						m2->mfrozen = 0;
						if (m2->mtyp != PM_GIANT_TURTLE || !(m2->mflee))
							m2->mcanmove = 1;
					}
					m2->mux = u.ux;
					m2->muy = u.uy;
					m2->encouraged = max(m2->encouraged, dmg / dnum);
				}
			}
		}
		if(dnum >= 5) make_stunned(HStun + dmg*10, TRUE);
	}
	for(m2=fmon; m2; m2 = nmon2) {
		nmon2 = m2->nmon;
		if (DEADMONSTER(m2)) continue;
		if (mindless_mon(m2)) continue;
		if (nonthreat(m2)) continue;
		if ((mon_resistance(m2,TELEPAT) && (rn2(2) || m2->mblinded))
			|| !rn2(10)
		){
			if (cansee(m2->mx, m2->my))
				pline("It locks on to %s.", mon_nam(m2));
			dmg = d(dnum, dsize);
			m2->mhp -= dmg;
			if(dnum >= 3) m2->mstdy = max(m2->mstdy, dmg);
			if(dnum >= 5) m2->mconf=TRUE;
			if (m2->mhp <= 0)
				monkilled(m2, "", AD_DRIN);
			else
				m2->msleeping = 0;
		}
	}
}

int
oresist_disintegration(obj)
struct obj *obj;
{
	return item_has_property(obj, DISINT_RES)
		|| is_quest_artifact(obj);
}

int
wrath_target(otmp, mon)
struct obj *otmp;
struct monst *mon;
{
	boolean youdefend = mon == &youmonst;
	if(youdefend){
		if(&mons[(otmp->wrathdata >> 2)] == youracedata)
			return 1;
		
		if(mons[(otmp->wrathdata >> 2)].mflagsa && 
		 ((mons[(otmp->wrathdata >> 2)].mflagsa&(youracedata->mflagsa)) != 0)
		)
			return 1;
	} else {
		if((otmp->wrathdata >> 2) == mon->mtyp)
			return 1;
		
		if(mons[(otmp->wrathdata >> 2)].mflagsa && 
		 ((mons[(otmp->wrathdata >> 2)].mflagsa&(mon->data->mflagsa)) != 0)
		){
			return 1;
		}
	}
	return 0;
}

int
goat_weapon_damage_turn(obj)
struct obj *obj;
{
	unsigned long int hashed = hash((unsigned long) (nonce + obj->o_id + hash(OPROP_GOATW)));
	switch(hashed%4){
		case 0: return AD_EACD;
		case 1: return AD_DRST;
		case 2: return AD_STDY;
		case 3:
			hashed = hash((unsigned long) (moves + hashed));
			switch(hashed%4){
				case 0: return AD_FIRE;
				case 1: return AD_COLD;
				case 2: return AD_ELEC;
				case 3: return AD_ACID;
			}
	}
	return AD_EACD;
}


int
soth_weapon_damage_turn(obj)
struct obj *obj;
{
	unsigned long int hashed = hash((unsigned long) (monstermoves/100 + obj->o_id + hash(OPROP_SOTHW)));
	switch(hashed%8){
		case 0: return AD_STTP;
		case 1: return AD_VAMP;
		case 2: return AD_FIRE;
		case 3: return AD_POLY;
		case 4: return AD_DESC;
		case 5: return AD_DRST;
		case 6: return AD_MAGM;
		case 7: return AD_MADF;
	}
	return AD_MAGM;
}

int
merc_weapon_damage_slice(otmp, magr, stat)
struct obj *otmp;
struct monst *magr;
int stat;
{
	boolean youagr = (magr == &youmonst);
	if(mlev(magr) <= 20)
		return 0;
	if(is_streaming_merc(otmp)){
		if(youagr){
			if(!YOU_MERC_SPECIAL)
				return 0;
			if(Insight <= 20 || ACURR(stat) < 15)
				return 0;
		}
		else {
			if(!is_chaotic_mon(magr))
				return 0;
			if(!insightful(magr->data))
				return 0;
		}
		unsigned long int hashed = hash((unsigned long) (nonce + otmp->o_id + youmonst.movement + stat));
		switch(hashed%4){
			case 0: return AD_FIRE;
			case 1: return AD_COLD;
			case 2: return AD_ELEC;
			case 3: return AD_ACID;
		}
	}
	return 0;
}
/* returns the current litness of IMA, recalculating it if possible
 * Returns a value from 0 to 3; 0 being unlit and 3 being most-lit.
 */
int
infinity_s_mirrored_arc_litness(obj)
struct obj * obj;
{
	xchar x, y;

	if (obj->where == OBJ_CONTAINED || obj->where == OBJ_BURIED || obj->where == OBJ_MAGIC_CHEST) {
		artinstance[ART_INFINITY_S_MIRRORED_ARC].IMAlitness = 0;
	}
	else if (get_obj_location(obj, &x, &y, 0)) {
		int litness = 0;
		/* uses its own rules, not dimness(x,y) */
		if (levl[x][y].lit &&
			!(viz_array[y][x] & TEMP_DRK3 &&
			!(viz_array[y][x] & TEMP_LIT1)))
		{
			litness += 2;
		}
		if (viz_array[y][x] & TEMP_LIT1 &&
			!(viz_array[y][x] & TEMP_DRK3))
		{
			litness += 1;
		}
		artinstance[ART_INFINITY_S_MIRRORED_ARC].IMAlitness = litness;
	}

	return artinstance[ART_INFINITY_S_MIRRORED_ARC].IMAlitness;
}

int merge_skies(opptr)
struct obj **opptr;
{
	struct obj *sky1 = *opptr;
	int needed;
	if(sky1->oartifact == ART_SILVER_SKY)
		needed = ART_SKY_REFLECTED;
	else
		needed = ART_SILVER_SKY;
	if(yn("Merge the two skies into one?") == 'y'){
		struct obj *sky2;
		struct obj *amalgam;
		for(sky2 = invent; sky2; sky2 = sky2->nobj)
			if(sky2->oartifact == needed)
				break;
		if(!sky2){
			impossible("Missing second sky in merge_skies.");
			return MOVE_CANCELLED;
		}
		if((sky1->oartifact == ART_SKY_REFLECTED && sky1->obj_material != MERCURIAL) ||
		   (sky2->oartifact == ART_SKY_REFLECTED && sky2->obj_material != MERCURIAL)
		){
			pline("The two weapons clink together awkwardly.");
			return MOVE_CANCELLED;
		}
		if((u.usanity < 50 || u.ulevel < 22 || Insight < 36) //Lower threshold than that for using the Resonant Edge of Fellowship
			|| !(artinstance[ART_SKY_REFLECTED].ZerthUpgrades&ZPROP_BALANCE) //From the *knowing* of Two Skies came the realization that hurting others, hurts oneself. 
		){
			pline("The two weapons ripple for a moment, then push each-other away!");
			pline("It seems you are not powerful enough to merge them together.");
			return MOVE_CANCELLED;
		}
		amalgam = mksartifact(ART_AMALGAMATED_SKIES);
		if(!amalgam || !amalgam->oartifact){
			impossible("Make Amalgamated Skies failed in merge_skies.");
			if(amalgam)
				obfree(amalgam, (struct obj *)0);	/* get rid of the useless non-artifact */
			return MOVE_CANCELLED;
		}
		pline("%s and %s melt and dissolve into each-other!", The(xname(sky1)), the(xname(sky2)));
		//merge stats
		amalgam->spe = max(sky1->spe, sky2->spe);
		for(int prop = 1; prop < MAX_OPROP; prop++){
			if(check_oprop(sky1, prop) || check_oprop(sky2, prop))
				add_oprop(amalgam, prop);
		}
		artinstance[ART_AMALGAMATED_SKIES].TwinSkiesEtraits |= sky1->expert_traits;
		artinstance[ART_AMALGAMATED_SKIES].TwinSkiesEtraits |= sky2->expert_traits;
		switch(sky1->obj_material){
			case IRON:
				artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_IRON;
			break;
			case GREEN_STEEL:
				artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_GREEN;
			break;
			case SILVER:
				artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_SILVER;
			break;
			case GOLD:
				artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_GOLD;
			break;
			case PLATINUM:
				artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_PLATINUM;
			break;
			case MITHRIL:
				artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_MITHRIL;
			break;
		}
		switch(sky2->obj_material){
			case IRON:
				artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_IRON;
			break;
			case GREEN_STEEL:
				artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_GREEN;
			break;
			case SILVER:
				artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_SILVER;
			break;
			case GOLD:
				artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_GOLD;
			break;
			case PLATINUM:
				artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_PLATINUM;
			break;
			case MITHRIL:
				artinstance[ART_SKY_REFLECTED].ZerthMaterials |= ZMAT_MITHRIL;
			break;
		}
		if(sky1->blessed || sky2->blessed)
			bless(amalgam);
		else if(sky1->cursed || sky2->cursed)
			curse(amalgam);
		else{
			amalgam->cursed = amalgam->blessed = FALSE;
		}
		if(sky1->ostolen || sky2->ostolen)
			amalgam->ostolen = TRUE;
		useupall(sky1);
		useupall(sky2);
		hold_another_object(amalgam, "You fumble and %s.",
		   aobjnam(amalgam, "fall"), (const char *)0); //Should've just freed up two iniventory slots to add one new item, but....
		*opptr = (struct obj *) 0;
		return MOVE_STANDARD;
	}
	else return MOVE_CANCELLED;
}

void
do_your_auras()
{
	if(uarm && uarm->oartifact == ART_SCORPION_CARAPACE && check_carapace_mod(uarm, CPROP_CROWN)){
		int distance = 0, damage = 0;
		static const int dice[] = {6,6,5,4,2,1};
		artinstance[ART_SCORPION_CARAPACE].CarapaceAura += C_CROWN_AURA_ADD;
		if(artinstance[ART_SCORPION_CARAPACE].CarapaceAura > C_CROWN_AURA_MAX)
			artinstance[ART_SCORPION_CARAPACE].CarapaceAura = C_CROWN_AURA_MAX;
		for(struct monst *tmpm = fmon; tmpm && artinstance[ART_SCORPION_CARAPACE].CarapaceAura > 0; tmpm = tmpm->nmon){
			if(!DEADMONSTER(tmpm)
				&& tmpm->mtyp != PM_VERMIURGE
				&& !tmpm->mpeaceful
				&& couldsee(tmpm->mx,tmpm->my)
				&& !nonthreat(tmpm)
			){
				distance = dist2(tmpm->mx,tmpm->my,u.ux,u.uy);
				distance = (int)sqrt(distance);
				//0, 1, 2, 3, 4, 5
				//0, 0, 1, 2, 4, 5
				if(distance < 6){
					damage = d(dice[distance],10);
					if(damage > artinstance[ART_SCORPION_CARAPACE].CarapaceAura)
						damage = artinstance[ART_SCORPION_CARAPACE].CarapaceAura;
					damage -= avg_mdr(tmpm);
					if(damage > 0){
						damage = min(damage, tmpm->mhp);
						artinstance[ART_SCORPION_CARAPACE].CarapaceAura -= damage;
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
							xkilled(tmpm, 0);
						}
					}
					// else continue
				}
				// else continue
			}
			// else continue
		}
		// if(!mon->mpeaceful && couldsee(mon->mx,mon->my)){
			// distance = dist2(u.ux,u.uy,mon->mx,mon->my);
			// distance = (int)sqrt(distance);
			// //0, 1, 2, 3, 4, 5
			// //0, 0, 1, 2, 4, 8
			// if(distance < 6){
				// damage = d(dice[distance],10);
				// if(damage > mon->mvar_vermiurge)
					// damage = mon->mvar_vermiurge;
				// damage -= u.udr;
				// if(damage > 0){
					// damage = min(damage, uhp());
					// You("are stung by swarming vermin!");
					// mon->mvar_vermiurge -= damage;
					// //Reduce after charging vermin
					// damage = reduce_dmg(&youmonst, damage, TRUE, FALSE);
					// losehp(damage,"swarming vermin",KILLED_BY);
				// }
				// // else continue
			// }
			// // else continue
		// }
	}
}
/*artifact.c*/
