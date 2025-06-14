/*	SCCS Id: @(#)read.c	3.4	2003/10/22	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#include "artifact.h"

/* KMH -- Copied from pray.c; this really belongs in a header file */
#define DEVOUT 14
#define STRIDENT 4

#define Your_Own_Role(mndx) \
	((mndx) == urole.malenum || \
	 (urole.femalenum != NON_PM && (mndx) == urole.femalenum))
#define Your_Own_Race(mndx) \
	((mndx) == urace.malenum || \
	 (urace.femalenum != NON_PM && (mndx) == urace.femalenum))

#ifdef OVLB

boolean	known;

static NEARDATA const char readable[] =
		   { ALL_CLASSES, SCROLL_CLASS, TILE_CLASS, SPBOOK_CLASS, 0 };
static const char all_count[] = { ALLOW_COUNT, ALL_CLASSES, 0 };
static const int random_cloud_types[] = { AD_FIRE, AD_COLD, AD_ELEC, AD_ACID};

static void FDECL(wand_explode, (struct obj *));
static void NDECL(do_class_genocide);
static void FDECL(stripspe,(struct obj *));
static void FDECL(p_glow1,(struct obj *));
static void FDECL(p_glow2,(struct obj *,const char *));
static void FDECL(randomize,(int *, int));
static void FDECL(forget_single_object, (int));
static void FDECL(maybe_tame, (struct monst *,struct obj *));
static void FDECL(ranged_set_lightsources, (int, int, genericptr_t));
static int FDECL(read_tile, (struct obj *));
static int FDECL(study_word, (struct obj *));
static int NDECL(learn_word);
static void FDECL(learn_spell_aphanactonan, (int));

int
doread()
{
	register struct obj *scroll;
	register boolean confused;
	char class_list[MAXOCLASSES+2];
	
	known = FALSE;
	if(check_capacity((char *)0)) return MOVE_CANCELLED;
	
	Strcpy(class_list, readable);
	if(carrying_applyable_ring())
		add_class(class_list, RING_CLASS);
	if(carrying_applyable_amulet())
		add_class(class_list, AMULET_CLASS);
	if(carrying_readable_tool())
		add_class(class_list, TOOL_CLASS);
	if(carrying_readable_weapon())
		add_class(class_list, WEAPON_CLASS);
	if(carrying_readable_armor())
		add_class(class_list, ARMOR_CLASS);
	
	scroll = getobj(class_list, "read");
	if(!scroll) return MOVE_CANCELLED;
	
	if((scroll->oartifact 
			&& !(scroll->oclass == SCROLL_CLASS)
			&& !(scroll->oclass == SPBOOK_CLASS)
			&& !(scroll->oclass == AMULET_CLASS)
			&& !arti_mandala(scroll)
			&& !scroll->oward
			&& scroll->oartifact != ART_ROD_OF_THE_ELVISH_LORDS
		) || scroll->otyp==LIGHTSABER || scroll->otyp==BEAMSWORD
	){
		if(scroll->oartifact == ART_ROD_OF_SEVEN_PARTS){
			if (Blind) {
				You_cant("see the writing!");
				exercise(A_WIS, FALSE); //The rod is critical of such logical blunders.
				return MOVE_INSTANT;
			}
			else{//Ruat Coelum, Fiat Justitia.  Ecce!  Lex Rex!
				pline(scroll->spe < 7 ? "The Rod is badly scarred, mute testimony to a tumultuous history." :
			"The surface of the Rod, once badly defaced, is now shiny and smooth, its mathematical perfection testament to the overriding power of Law.");
				switch(scroll->spe){
					case(-5):
					case(-4):
					case(-3):
						pline("The damage seems heaviest around the point, but the whole rod is really pretty messed up.");
					break;
					case(-2):
					case(-1):
					case(0):
						pline("It seems that at one point, an inscription spiraled around the Rod, but it can no longer be read.");
					break;
					case(1):
						pline("The damage is lighter further from the point, and looking close you can just make out the word \"Ruat\" near the pommel.");
						u.uconduct.literate++;
					break;
					case(2):
						pline("The damage is lighter further from the point.");
						pline("Looking close, you can just make out the phrase \"Ruat Coelum\" near the pommel.");
						pline("It seems to be part of a longer inscription, but the Rod has been thoroughly defaced.");
						u.uconduct.literate++;
					break;
					case(3):
						pline("The damage is lighter near the pommel, and an inscription spirals around the rod.");
						pline("You can make out the phrase \"Ruat Coelum, Fiat\", but the rest of the inscription is defaced.");
						u.uconduct.literate++;
					break;
					case(4):
						pline("The damage is lighter on the half nearest the pommel.");
						pline("You can make out the phrase \"Ruat Coelum, Fiat Justitia.\"");
						pline("There's more, but the rest of it is illegible.");
						u.uconduct.literate++;
					break;
					case(5):
						pline("The damage is heaviest near the tip.");
						pline("You read part of an inscription: \"Ruat Coelum, Fiat Justitia. Ecce!\"");
						pline("There's more, but the rest of it is illegible.");
						u.uconduct.literate++;
					break;
					case(6):
						pline("The damage is heaviest near the tip.");
						pline("You read most of the inscription: \"Ruat Coelum, Fiat Justitia. Ecce! Lex\"");
						pline("There's more, but the rest of it is scratched out.");
						u.uconduct.literate++;
					break;
					case(7):
						pline("An inscription spirals around the Rod, from pommel to tip:");
						pline("\"Ruat Coelum, Fiat Justitia. Ecce! Lex Rex!\"");
						u.uconduct.literate++;
					break;
				}
				if(artinstance[ART_ROD_OF_SEVEN_PARTS].RoSPflights > 0){
					static const char *numbers[]={
						"no", "a single", "two","three","four","five","six","seven"
					};
					pline("There is a crownlike decoration with %s raised segment%s around the pommel.",
						numbers[artinstance[ART_ROD_OF_SEVEN_PARTS].RoSPflights], artinstance[ART_ROD_OF_SEVEN_PARTS].RoSPflights!=1 ? "s" : "");
				}
				return MOVE_READ;
			}
		} else if(scroll->oartifact == ART_PEN_OF_THE_VOID){
			if (Blind) {
				You_cant("see the blade!");
				return MOVE_INSTANT;
			}
			else{
				int j;
				if(!(quest_status.killed_nemesis && Role_if(PM_EXILE))) pline("It seems this athame once had dual blades, but one blade has been snapped off at the hilt.");
				else pline("The dual-bladed athame vibrates faintly.");
				if(u.spiritTineA) for(j=0;j<32;j++){
					if((u.spiritTineA >> j) == 1){
						if(!quest_status.killed_nemesis) pline("The blade bears the seal of %s.",sealNames[j]);
						else pline("The first blade bears the seal of %s.",sealNames[j]);
						break;
					}
				}
				if(quest_status.killed_nemesis && u.spiritTineB) for(j=0;j<32;j++){
					if((u.spiritTineB >> j) == 1){
						pline("The second blade bears the seal of %s.",sealNames[j]);
						break;
					}
				}
				return MOVE_READ;
			}
		} else if(scroll->oartifact == ART_EXCALIBUR){
			if (Blind) {
				You_cant("see the blade!");
				return MOVE_INSTANT;
			} else if(scroll == uwep) {
				pline("\"Cast me away\"");
			} else {
				pline("\"Take me up\"");
			}
			return MOVE_READ;
		} else if(scroll->oartifact == ART_HOLY_MOONLIGHT_SWORD && scroll->lamplit){
			/* Note: you can see the blade even when blind */
			if(Insight < 2) {
				pline("The glowing cyan blade is decorated with faint curves.");
			}
			else if(Insight < 5) {
				You("faintly see strange arches inside the cyan blade.");
			}
			else if(Insight < 10){
				You("can barely see faint bright stars behind the arches inside the cyan blade.");
			}
			else if(Insight < 20){
				pline("The blade is the deep black of the night sky. You don't know why you ever thought it was cyan.");
			}
			else if(Insight < 40){
				pline("The distant stars wink and dance among the arches within the black night sky.");
			}
			else {
				pline("Tiny spirits of light dance in and out of the blade's sky and the black night of your %s.", (eyecount(youracedata) == 1) ? body_part(EYE) : makeplural(body_part(EYE)));
				maybe_give_thought(GUIDANCE);
			}
			return MOVE_READ;
		} else if(scroll->oartifact == ART_ITLACHIAYAQUE){
			if (Blind) {
				You_cant("see the mirror!");
				return MOVE_INSTANT;
			} else {
				pline("You see the nearby terrain reflected in the smoky depths of the mirror.");
				do_vicinity_map(u.ux,u.uy);
			}
			return MOVE_READ;
		} else if(scroll->oartifact == ART_GLAMDRING){
			if (Blind) {
				You_cant("see the blade!");
				return MOVE_INSTANT;
			} else if(Race_if(PM_ELF)) {
				pline("\"Turgon, king of Gondolin, wields, has, and holds the sword Foe-hammer, Foe of Morgoth's realm, Hammer of the Orcs\".");
			} else {
				pline("\"Turgon aran Gondolin tortha gar a matha i vegil Glamdring gud daedheloth, dam an Glamhoth\".");
			}
			return MOVE_READ;
		} else if(scroll->oartifact == ART_BOW_OF_SKADI){
			if (Blind) {
				You_cant("see the bow!");
				return MOVE_INSTANT;
			} else {
				int i;
				You("read the Galdr of Skadi!");
				for (i = 0; i < MAXSPELL; i++)  {
					if (spellid(i) == SPE_CONE_OF_COLD)  {
						if (spellknow(i) <= 1000) {
							Your("knowledge of Cone of Cold is keener.");
							spl_book[i].sp_know = 20000;
							exercise(A_WIS,TRUE);       /* extra study */
						} else { /* 1000 < spellknow(i) <= MAX_SPELL_STUDY */
							You("know Cone of Cold quite well already.");
						}
						break;
					} else if (spellid(i) == NO_SPELL)  {
						spl_book[i].sp_id = SPE_CONE_OF_COLD;
						spl_book[i].sp_lev = objects[SPE_CONE_OF_COLD].oc_level;
						spl_book[i].sp_know = 20000;
						You("learn to cast Cone of Cold!");
						break;
					}
				}
				if (further_study(SPE_CONE_OF_COLD))
				{
					for (i = 0; i < MAXSPELL; i++)  {
						if (spellid(i) == SPE_BLIZZARD)  {
							if (spellknow(i) <= 1000) {
								Your("knowledge of Blizzard is keener.");
								spl_book[i].sp_know = 20000;
								exercise(A_WIS, TRUE);       /* extra study */
							}
							else { /* 1000 < spellknow(i) <= MAX_SPELL_STUDY */
								You("know Blizzard quite well already.");
							}
							break;
						}
						else if (spellid(i) == NO_SPELL)  {
							spl_book[i].sp_id = SPE_BLIZZARD;
							spl_book[i].sp_lev = objects[SPE_BLIZZARD].oc_level;
							spl_book[i].sp_know = 20000;
							You("learn to cast Frost Storm!");
							break;
						}
					}
				}
				if (i == MAXSPELL) impossible("Too many spells memorized!");
				return MOVE_READ;
			}
		
		} else if(scroll->oartifact == ART_GUNGNIR){
			if (Blind) {
				You_cant("see the spear!");
				return MOVE_INSTANT;
			} else {
				int i;
				You("read the secret runes!");
				for (i = 0; i < MAXSPELL; i++){
					if (spellid(i) == SPE_IDENTIFY)  {
						if (spellknow(i) <= 1000) {
							Your("knowledge of Identify is keener.");
							spl_book[i].sp_know = 20000;
							exercise(A_WIS,TRUE);       /* extra study */
						} else { /* 1000 < spellknow(i) <= MAX_SPELL_STUDY */
							You("know Identify quite well already.");
						}
						break;
					} else if (spellid(i) == NO_SPELL)  {
						spl_book[i].sp_id = SPE_IDENTIFY;
						spl_book[i].sp_lev = objects[SPE_IDENTIFY].oc_level;
						spl_book[i].sp_know = 20000;
						You("learn to cast Identify!");
						break;
					}
				}
				if (i == MAXSPELL) impossible("Too many spells memorized!");
				return MOVE_READ;
			}
		
		} else if(scroll->oartifact == ART_STAFF_OF_NECROMANCY){
			if (Blind) {
				You_cant("see the staff!");
				return MOVE_INSTANT;
			} else {
				int i;
				You("read the dark secrets inscribed on the staff.");
				for (i = 0; i < MAXSPELL; i++)  {
					if (spellid(i) == SPE_DRAIN_LIFE)  {
						if (spellknow(i) <= 1000) {
							Your("knowledge of Drain Life is keener.");
							spl_book[i].sp_know = 20000;
							exercise(A_WIS,TRUE);       /* extra study */
						} else { /* 1000 < spellknow(i) <= MAX_SPELL_STUDY */
							You("know Drain Life quite well already.");
						}
						break;
					} else if (spellid(i) == NO_SPELL)  {
						spl_book[i].sp_id = SPE_DRAIN_LIFE;
						spl_book[i].sp_lev = objects[SPE_DRAIN_LIFE].oc_level;
						spl_book[i].sp_know = 20000;
						You("learn to cast Drain Life!");
						break;
					}
				}
				if (i == MAXSPELL) impossible("Too many spells memorized!");
				return MOVE_READ;
			}
		} else if(scroll->oartifact == ART_STAFF_OF_AESCULAPIUS){
			if (Blind) {
				You_cant("see the staff!");
				return MOVE_INSTANT;
			} else {
				int i;
				You("read the traceries of healing magics inscribed on the staff.");
				for (i = 0; i < MAXSPELL; i++)  {
					if (spellid(i) == SPE_MASS_HEALING)  {
						if (spellknow(i) <= 1000) {
							Your("knowledge of Mass Healing is keener.");
							spl_book[i].sp_know = 20000;
							exercise(A_WIS,TRUE);       /* extra study */
						} else { /* 1000 < spellknow(i) <= MAX_SPELL_STUDY */
							You("know Mass Healing quite well already.");
						}
						break;
					} else if (spellid(i) == NO_SPELL)  {
						spl_book[i].sp_id = SPE_MASS_HEALING;
						spl_book[i].sp_lev = objects[SPE_MASS_HEALING].oc_level;
						spl_book[i].sp_know = 20000;
						You("learn to cast Mass Healing!");
						break;
					}
				}
				if (i == MAXSPELL) impossible("Too many spells memorized!");
				return MOVE_READ;
			}
		} else if(scroll->oartifact == ART_ESSCOOAHLIPBOOURRR){
			if (Blind) {
				You_cant("see the staff!");
				return MOVE_INSTANT;
			} else {
				int i;
				You("read the still-visible traceries of healing magics inscribed on the shackles fusing the sword together.");
				for (i = 0; i < MAXSPELL; i++)  {
					if (spellid(i) == SPE_MASS_HEALING)  {
						if (spellknow(i) <= 1000) {
							Your("knowledge of Mass Healing is keener.");
							spl_book[i].sp_know = 20000;
							exercise(A_WIS,TRUE);       /* extra study */
						} else { /* 1000 < spellknow(i) <= MAX_SPELL_STUDY */
							You("know Mass Healing quite well already.");
						}
						break;
					} else if (spellid(i) == NO_SPELL)  {
						spl_book[i].sp_id = SPE_MASS_HEALING;
						spl_book[i].sp_lev = objects[SPE_MASS_HEALING].oc_level;
						spl_book[i].sp_know = 20000;
						You("learn to cast Mass Healing!");
						break;
					}
				}
				if (i == MAXSPELL) impossible("Too many spells memorized!");
				return MOVE_READ;
			}
		} else if(scroll->otyp == LIGHTSABER){
			if (Blind) {
				You_cant("see it!");
				return MOVE_INSTANT;
			} else {
				pline(lightsaber_hiltText(scroll),xname(scroll));
			}
			return MOVE_READ;
		} else if(scroll->otyp == BEAMSWORD){
			if (Blind) {
				You_cant("see it!");
				return MOVE_INSTANT;
			} else {
				pline(beamsword_hiltText(scroll),xname(scroll));
			}
			return MOVE_READ;
		} else {
			pline(silly_thing_to, "read");
			return MOVE_CANCELLED;
		}
	} else if(scroll->oartifact && scroll->oartifact == ART_ENCYCLOPEDIA_GALACTICA){
      const char *line;
      char buf[BUFSZ];

      line = getrumor(bcsign(scroll), buf, TRUE);
      if (!*line)
        line = "NetHack rumors file closed for renovation.";

      pline("%s:", Tobjnam(scroll, "display"));
      verbalize("%s", line);
      return MOVE_READ;
    } else if(scroll->oartifact && scroll->oartifact == ART_LOG_OF_THE_CURATOR){
      int oindx = 1 + rn2(NUM_OBJECTS - 1);
      if(objects[oindx].oc_name_known){
        makeknown(oindx);
        You("study the pages of %s, you learn to recognize %s!", xname(scroll),
            obj_typename(oindx));
      } else {
        You("study the pages of %s, but you already can recognize that.", xname(scroll));
      }
      return MOVE_READ;
	} else if(scroll->otyp == CANDLE_OF_INVOCATION ){
		if (Blind) {
			You_cant("see the candle!");
			return MOVE_INSTANT;
		} else {
			pline("The candle is carved with many pictographs, including %s.", fetchHaluWard(FIRST_PLANE_SYMBOL+rn2(LAST_PLANE_SYMBOL-FIRST_PLANE_SYMBOL+1)));
		}
		return MOVE_READ;
	} else if(scroll->otyp == MISOTHEISTIC_PYRAMID){
		if (Blind) {
			pline("The density of the pyramid increases towards the tip, painfully so, enabling it to stand tip-downwards.");
			pline("There are engravings on the square 'roof' of the pyramid, but you can't see them.");
			return MOVE_STANDARD;
		} else {
			pline("The density of the pyramid increases towards the tip, painfully so, enabling it to stand tip-downwards.");
			if(Role_if(PM_EXILE) || Role_if(PM_WIZARD) || u.specialSealsKnown&SEAL_NUDZIRATH)
				pline("The square 'roof' of the pyramid is engraved with curses to the bringers of light.");
			else
				pline("There are engravings on the square 'roof' of the pyramid, but you can't read them.");
			pline("A seam runs around the edges of the roof.");
		}
		return MOVE_READ;
	} else if(scroll->otyp == MISOTHEISTIC_FRAGMENT){
		if (Blind) {
			pline("Some mysterious force holds these mirrored fragments together in a rough approximation of a pyramid.");
			You_cant("see the fragments!");
			return MOVE_STANDARD;
		} else {
			pline("Some mysterious force holds these mirrored fragments together in a rough approximation of a pyramid.");
			You("are not reflected in the shards.");
			You("have the unnerving feeling that there is something inside the pyramid, barely hidden from view.");
			/*Note: this is intended to work for any PC, not just Binders */
			if(!(u.specialSealsKnown&SEAL_NUDZIRATH)){
				pline("The shattered fragments form part of a seal.");
				pline("In fact, you realize that all cracked and broken mirrors everywhere together are working towards writing this seal.");
				pline("With that realization comes knowledge of the seal's final form!");
				u.specialSealsKnown |= SEAL_NUDZIRATH;
			}
		}
		return MOVE_READ;
	} else if(scroll->oclass == WEAPON_CLASS && (scroll)->obj_material == WOOD && scroll->oward != 0){
		pline("A %s is carved into the wood.",wardDecode[decode_wardID(scroll->oward)]);
		if(! (u.wardsknown & scroll->oward) ){
			You("have learned a new warding stave!");
			u.wardsknown |= scroll->oward;
		}
		return MOVE_READ;
	}
	else if(scroll->oclass == RING_CLASS && (isEngrRing((scroll)->otyp)||isSignetRing((scroll)->otyp)) && scroll->oward){
		if(!(scroll->ohaluengr)){
			pline("A %s is engraved on the ring.",wardDecode[scroll->oward]);
			if( !(u.wardsknown & get_wardID(scroll->oward)) ){
				You("have learned a new warding sign!");
				u.wardsknown |= get_wardID(scroll->oward);
			}
		}
		else{
			pline("There is %s engraved on the ring.",fetchHaluWard((int)scroll->oward));
		}
		return MOVE_READ;
	}
	else if(scroll->oclass == AMULET_CLASS && scroll->oward){
		if(!(scroll->ohaluengr)){
			pline("A %s is engraved on the amulet.",wardDecode[scroll->oward]);
			if( !(u.wardsknown & get_wardID(scroll->oward)) ){
				You("have learned a new warding sign!");
				u.wardsknown |= get_wardID(scroll->oward);
			}
		}
		else{
			pline("There is %s engraved on the amulet.",fetchHaluWard((int)scroll->oward));
		}
		return MOVE_READ;
	}
	/* outrumor has its own blindness check */
	else if(scroll->otyp == FORTUNE_COOKIE) {
	    if(flags.verbose)
		You("break up the cookie and throw away the pieces.");
		if(scroll->ostolen){
			if(u.sealsActive&SEAL_ANDROMALIUS)
				unbind(SEAL_ANDROMALIUS, TRUE);
			/*stealing is impure*/
			IMPURITY_UP(u.uimp_theft)
		}
	    outrumor(bcsign(scroll), BY_COOKIE);
	    if (!Blind) u.uconduct.literate++;
	    useup(scroll);
	    return MOVE_READ;
#ifdef TOURIST
	} else if(scroll->oclass == ARMOR_CLASS
		&& is_readable_armor(scroll)
	){
		pline("There is %s %s on the %s.",fetchHaluWard((int)scroll->oward), hard_mat(scroll->obj_material) ? "engraved" : "embroidered", distant_name(scroll, xname));
		if(!scroll->ohaluengr){
			if( !(u.wardsknown & get_wardID(scroll->oward)) ){
				You("have learned a new warding sign!");
				u.wardsknown |= get_wardID(scroll->oward);
			}
		}
		return MOVE_READ;
	}else if (scroll->otyp == T_SHIRT) {
	    static const char *shirt_msgs[] = { /* Scott Bigham */
    "I explored the Dungeons of Doom and all I got was this lousy T-shirt!",
    "Is that Mjollnir in your pocket or are you just happy to see me?",
    "It's not the size of your sword, it's how #enhance'd you are with it.",
    "Madame Elvira's House O' Succubi Lifetime Customer",
    "Madame Elvira's House O' Succubi Employee of the Month",
    "Rogues Do It From Behind", /*the original D&D joke now fits*/
    "Ludios Vault Guards Do It In Small, Dark Rooms",
    "Yendor Military Soldiers Do It In Large Groups",
    "Arcadian Ants Do It In Swarms",
    "Autons Do It Under Close Supervision",
    "I survived Yendor Military Boot Camp",
    "Ludios Accounting School Intra-Mural Lacrosse Team",
    "Oracle(TM) Fountains 10th Annual Wet T-Shirt Contest",
    "Hey, black dragon!  Disintegrate THIS!",
    "I'm With Stupid -->",
    "Don't blame me, I voted for Izchak!",
    "Don't Panic",				/* HHGTTG */
    "Furinkan High School Athletic Dept.",	/* Ranma 1/2 */
    "Hel-LOOO, Nurse!",			/* Animaniacs */
    "Stand back, I'm going to try MAGIC!",
	"Magic. It works, witches!",
    "=^.^=",
    "100% goblin hair - do not wash",
    "Aberzombie and Fitch",
    "Aim >>> <<< here",
    "cK -- Cockatrice touches the Kop",
    "Croesus for President 2008 - Campaign Finance Reform Now (for the other guy)",
    "- - - - - - CUT HERE - - - - - -",
    "Don't ask me, I only adventure here",
    "Down With Pants!",
    "d, your dog or a killer?",
    "FREE PUG AND NEWT!",
    "Gehennoms Angels",
    "Glutton For Punishment",
    "Go Team Ant!",
    "Got Newt?",
    "Heading for Godhead",
    "Hello, my darlings!", /* Charlie Drake */
    "Hey! Nymphs! Steal This T-Shirt!",
    "I <3 Dungeon of Doom",
    "I <3 Maud",
    "I am a Valkyrie. If you see me running, try to keep up.",
    "I Am Not a Pack Rat - I Am a Collector",
    "I bounced off a rubber tree",
    "If you can read this, I can hit you with my polearm",
    "I'm Confused!",
    "I met Carl, the swordmaster of Jambalaya island and all I got was this lousy t-shirt",
    "I'm in ur base, killin ur doods",
    "I scored with the princess",
    "I Support Single Succubi",
    "I want to live forever or die in the attempt.",
    "I'll stick it in you!",
    "Kop Killaz",
    "Lichen Park",
    "LOST IN THOUGHT - please send search party",
    "Meat Is Mordor",
    "Minetown Better Business Bureau",
    "Minetown Watch",
    "Ms. Palm's House of Negotiable Affection -- A Very Reputable House Of Disrepute",
    "^^  My eyes are up there!  ^^",
    "Neferet/Pelias '08",
    "Next time you wave at me, use more than one finger, please.",
    "No Outfit Is Complete Without a Little Cat Fur",
    "Objects In This Shirt Are Closer Than They Appear",
    "Protection Racketeer",
    "P Happens",
    "Real men love Crom",
    "Rodney in '08. OR ELSE!",
    "Sokoban Gym -- Get Strong or Die Trying",
    "Somebody stole my Mojo!",
    "The Hellhound Gang",
    "The Werewolves",
    "They Might Be Storm Giants",
    "Up with miniskirts!",
    "Weapons don't kill people, I kill people",
    "Where's the beef?",
    "White Zombie",
    "Worship me",
    "You laugh because I'm different, I laugh because you're about to die",
    "You're killing me!",
    "You should hear what the voices in my head are saying about you.",
    "Anhur State University - Home of the Fighting Fire Ants!",
    "FREE HUGS",
    "Serial Ascender",
    "Plunder Island Brimstone Beach Club",
    "Real Men Are Valkyries",
    "Young Men's Cavedigging Association",
    "YMRCIGBSA - recovering mind, body, and spirit since 1844",
    "YHPA - Young Humanoid's Pagan Association",
    "Occupy Fort Ludios",
    "I couldn't afford this T-shirt so I stole it!",
    "End Mercantile Opacity Discrimination Now: Let Invisible Customers Shop!",
    "Elvira's House O'Succubi, granting the gift of immorality!",
    "Mind Flayers Suck",
    "I'm not wearing any pants",
    "Newt Fodder",
    "My Dog ate Og",
    "End Lich Prejudice Now!",
    "Down With The Living!",
    "Pudding Farmer",
    "Dudley",
    "I pray to Our Lady of Perpetual Mood Swings",
    "Soooo, When's the Wizard Getting Back to You About That Brain?",
    "Vegetarian",
    "I plan to go to Astral",
    "If They Don't Have Fruit Juice in Heaven, I Ain't Going",
    "Living Dead",
	
	/* #nethack suggestions */
    "Lich Park",
    "I'd rather be pudding farming",
	
    "Great Cthulhu for President -- why settle for the lesser evil?",
	    };
	    char buf[BUFSZ];
	    int erosion;
		if(arti_mandala(scroll)){
			if(uarmu && uarmu == scroll && uarm && arm_blocks_upper_body(uarm->otyp)){
				if( !is_opaque(uarm)){
					You("look at your shirt through your glass armor.");
				}
				else{
					if (Blind) You_cant("see that.");
					else You_cant("see that through your %s.", aobjnam(uarm, (char *)0));
					return MOVE_INSTANT;
				}
			}
			if (Blind) You("feel as though you can see the image, despite your blindness.");
			You("feel a sense of peace come over you as you study the mandala.");
			use_unicorn_horn(scroll);
			return MOVE_READ;
		}
		else{
			if (Blind) {
				You_cant("feel any Braille writing.");
				return MOVE_INSTANT;
			}
			if(uarmu && uarmu == scroll && uarm && arm_blocks_upper_body(uarm->otyp)){
				if(!is_opaque(uarm)){
					You("look at your shirt through your glass armor.");
				}
				else{
					You_cant("read that through your %s.", aobjnam(uarm, (char *)0));
					return MOVE_INSTANT;
				}
			}
			u.uconduct.literate++;
			if(flags.verbose)
			pline("It reads:");
			Strcpy(buf, shirt_msgs[scroll->o_id % SIZE(shirt_msgs)]);
			erosion = greatest_erosion(scroll);
			if (erosion)
			wipeout_text(buf,
				(int)(strlen(buf) * erosion / (2*MAX_ERODE)),
				     scroll->o_id ^ (unsigned)u.ubirthday);
			pline("\"%s\"", buf);
			return MOVE_READ;
		}
#endif	/* TOURIST */
	} else if (scroll->oclass == TILE_CLASS){
		return read_tile(scroll);
	} else if (scroll->oclass != SCROLL_CLASS
		&& scroll->oclass != SPBOOK_CLASS
	) {
	    pline(silly_thing_to, "read");
	    return MOVE_CANCELLED;
	} else if ((Babble || Screaming || mad_turn(MAD_TOO_BIG))
		&& (scroll->oclass == SPBOOK_CLASS)
	){
		if(Screaming)
			You_cant("focus on that while you're screaming!");
		else if(Babble)
			You_cant("focus on that while you're babbling incoherently!");
		else if(mad_turn(MAD_TOO_BIG))
			pline("It's too big!");
		else
			impossible("You can't read that book for some reason?");
	    return MOVE_INSTANT;
	} else if ((Babble || Strangled || Drowning || mad_turn(MAD_TOO_BIG))
		&& (scroll->oclass == SCROLL_CLASS || (scroll->oclass == TILE_CLASS && objects[scroll->otyp].oc_magic))
	){
		if(Strangled)
			You_cant("read that aloud, you can't breathe!");
		else if(Drowning)
			You_cant("read that aloud, you're drowning!");
		else if(Babble)
			You_cant("read that aloud, you're babbling incoherently!");
		else if(mad_turn(MAD_TOO_BIG))
			pline("It's too big!");
		else
			impossible("You can't read that aloud for some reason?");
	    return MOVE_INSTANT;
		//Note, you CAN scream one syllable
	} else if (Screaming && (scroll->oclass == SCROLL_CLASS)){
	    You_cant("read that aloud, you're too busy screaming!");
	    return MOVE_INSTANT;
	} else if (Blind) {
	    const char *what = 0;
	    if (scroll->oclass == SPBOOK_CLASS)
			what = "mystic runes";
	    else if (!scroll->dknown)
			what = "formula on the scroll";
	    if (what) {
			if(check_oprop(scroll, OPROP_TACTB)){
				pline("A tactile script supplements the %s.", what);
			}
			else {
				pline("Being blind, you cannot read the %s.", what);
				return MOVE_INSTANT;
			}
	    }
	}

	/* Actions required to win the game aren't counted towards conduct */
	if (scroll->otyp != SPE_BOOK_OF_THE_DEAD &&
	    scroll->otyp != SCR_MAIL &&
		scroll->otyp != SPE_BLANK_PAPER &&
		scroll->otyp != SCR_BLANK_PAPER){
			if(scroll->ostolen){
				if(u.sealsActive&SEAL_ANDROMALIUS)
					unbind(SEAL_ANDROMALIUS, TRUE);
				/*stealing is impure*/
				IMPURITY_UP(u.uimp_theft)
			}
			u.uconduct.literate++;
		}

	confused = (Confusion != 0);
#ifdef MAIL
	if (scroll->otyp == SCR_MAIL) confused = FALSE;
#endif
	if(scroll->oclass == SPBOOK_CLASS) {
	    return(study_book(scroll));
	}
	scroll->in_use = TRUE;	/* scroll, not spellbook, now being read */
	if(scroll->oartifact) {
		if(Blind) {
			pline("Being blind, you cannot see %s.", the(xname(scroll)));
			scroll->in_use = FALSE;
			return MOVE_INSTANT;
		}
		if(scroll->oartifact != ART_PAINTING_FRAGMENT)
			pline("You examine %s.", the(xname(scroll)));
	} else if(scroll->otyp != SCR_BLANK_PAPER) {
	  if(Blind)
	    pline("As you %s the formula on it, the scroll disappears.",
			is_silent(youracedata) ? "cogitate" : "pronounce");
	  else
	    pline("As you read the scroll, it disappears.");
	  if(confused) {
	    if (Hallucination)
		pline("Being so trippy, you screw up...");
	    else{
			pline("Being confused, you mis%s the magic words...",
				is_silent(youracedata) ? "understand" : "pronounce");
			if(u.sealsActive&SEAL_PAIMON) unbind(SEAL_PAIMON,TRUE);
		}
	  }
	}
	if(!seffects(scroll))  {
		if(!objects[scroll->otyp].oc_name_known) {
		    if(known) {
			makeknown(scroll->otyp);
			more_experienced(0,10);
		    } else if(!objects[scroll->otyp].oc_uname)
			docall(scroll);
		}
		if(scroll->otyp != SCR_BLANK_PAPER && !scroll->oartifact)
			useup(scroll);
		else scroll->in_use = FALSE;
	}
	return MOVE_READ;
}

static
int
read_tile(scroll)
struct obj *scroll;
{
	int duration;
	long int thought;

	if(!scroll->dknown){
		You("have never seen it!");
		return MOVE_CANCELLED;
	}
	if(objects[scroll->otyp].oc_unique){
		u.uconduct.literate++;
		study_word(scroll);
		return MOVE_READ;
	}
	if(!objects[scroll->otyp].oc_name_known && objects[scroll->otyp].oc_magic){
		You("don't know how to pronounce the glyph!");
		return MOVE_CANCELLED;
	}
	if(scroll->otyp == SYLLABLE_OF_STRENGTH__AESH){
		if(scroll->cursed){
			pline("\"Aesh!\" The shard's glyph resonates and turns black while the shard turns to dust.");
			duration = 40;
			//Note: 4x duration, no permanent bonus
		} else if(scroll->blessed){
			pline("\"Aesh!\" The shard's glyph resonates and glows brightly while the shard turns to dust.");
			duration = 15; //150%
		} else {
			pline("\"Aesh!\" The shard's glyph resonates and begins to glow while the shard turns to dust.");
			duration = 10;
		}
		u.uaesh_duration += duration;
		if(!scroll->cursed) u.uaesh++;
	} else if(scroll->otyp == SYLLABLE_OF_POWER__KRAU){
		if(scroll->cursed){
			pline("\"Krau!\" The shard's glyph resonates and turns black while the shard turns to dust.");
			duration = 40;
			//Note: 4x duration, no permanent bonus
		} else if(scroll->blessed){
			pline("\"Krau!\" The shard's glyph resonates and glows brightly while the shard turns to dust.");
			duration = 15; //150%
		} else {
			pline("\"Krau!\" The shard's glyph resonates and begins to glow while the shard turns to dust.");
			duration = 10;
		}
		u.ukrau_duration += duration;
		if(!scroll->cursed) u.ukrau++;
	} else if(scroll->otyp == SYLLABLE_OF_LIFE__HOON){
		if(scroll->cursed){
			pline("\"Hoon!\" The shard's glyph resonates and turns black while the shard turns to dust.");
			duration = 40;
			//Note: 4x duration, no permanent bonus
		} else if(scroll->blessed){
			pline("\"Hoon!\" The shard's glyph resonates and glows brightly while the shard turns to dust.");
			duration = 15; //150%
		} else {
			pline("\"Hoon!\" The shard's glyph resonates and begins to glow while the shard turns to dust.");
			duration = 10;
		}
		u.uhoon_duration += duration;
		if(!scroll->cursed) u.uhoon++;
	} else if(scroll->otyp == SYLLABLE_OF_GRACE__UUR){
		if(scroll->cursed){
			pline("\"Uur!\" The shard's glyph resonates and turns black while the shard turns to dust.");
			duration = 40;
			//Note: 4x duration, no permanent bonus
		} else if(scroll->blessed){
			pline("\"Uur!\" The shard's glyph resonates and glows brightly while the shard turns to dust.");
			duration = 15; //150%
		} else {
			pline("\"Uur!\" The shard's glyph resonates and begins to glow while the shard turns to dust.");
			duration = 10;
		}
		u.uuur_duration += duration;
		if(!scroll->cursed) u.uuur++;
	} else if(scroll->otyp == SYLLABLE_OF_THOUGHT__NAEN){
		if(scroll->cursed){
			pline("\"Naen!\" The shard's glyph resonates and turns black while the shard turns to dust.");
			duration = 40;
			//Note: 4x duration, no permanent bonus
		} else if(scroll->blessed){
			pline("\"Naen!\" The shard's glyph resonates and glows brightly while the shard turns to dust.");
			duration = 15; //150%
		} else {
			pline("\"Naen!\" The shard's glyph resonates and begins to glow while the shard turns to dust.");
			duration = 10;
		}
		u.unaen_duration += duration;
		if(!scroll->cursed) u.unaen++;
	} else if(scroll->otyp == SYLLABLE_OF_SPIRIT__VAUL){
		if(scroll->cursed){
			pline("\"Vaul!\" The shard's glyph resonates and turns black while the shard turns to dust.");
			duration = 40;
			//Note: 4x duration, no permanent bonus
		} else if(scroll->blessed){
			pline("\"Vaul!\" The shard's glyph resonates and glows brightly while the shard turns to dust.");
			duration = 15; //150%
		} else {
			pline("\"Vaul!\" The shard's glyph resonates and begins to glow while the shard turns to dust.");
			duration = 10;
		}
		u.uvaul_duration += duration;
		if(!scroll->cursed) u.uvaul++;
	} else if(scroll->otyp == APHANACTONAN_RECORD){
		if(Blind){
			You("can't see the disk.");
			return 0;
		}
		else {
			You("study the glyph-inscribed disk.");
			if(u.veil){
				You("can't understand what they say.");
				return 0;
			}
			else {
				int objcount = 2 + rn2(3) + rn2(5);
				You("can't understand what they say...");
				pline("Suddenly, the glyphs glow in rainbow hues and escape from the fracturing disk!");
				pline("Some of the glyphs get trapped in your %s!", (eyecount(youracedata) == 1) ? body_part(EYE) : makeplural(body_part(EYE)));
				know_random_obj(objcount);
				if(!u.udisks || !rn2(u.udisks)){
					change_uinsight(1);
					objcount++;
				}
				u.udisks++;
				more_experienced(d(objcount, 100), 0);
				newexplevel();
			}
		}
	} else if(scroll->otyp == APHANACTONAN_ARCHIVE){
		if(Blind){
			You("can't see the disk.");
			return 0;
		}
		else {
			You("study the glyph-inscribed disk.");
			if(u.veil){
				You("can't understand what it says.");
				return 0;
			}
			else {
				int i;
				int rolls;
				int effectcount;
				int xp = 0;
				boolean seals = FALSE, wards = FALSE, combat = FALSE;
				You("can't understand what it says...");
				pline("Suddenly, the glyphs glow in impossible hues and escape from the fracturing disk!");
				pline("Some of the glyphs get trapped in your %s!", (eyecount(youracedata) == 1) ? body_part(EYE) : makeplural(body_part(EYE)));
				//ID
				effectcount = 4 + rn2(5) + rn2(9);
				xp += d(effectcount,100);
				know_random_obj(effectcount);

				//Insight
				effectcount = 1;
				for(rolls = rnd(8); rolls > 0; rolls--){
					if(!u.udisks || !rn2(u.udisks))
						effectcount++;
					u.udisks++;
				}
				xp += d(effectcount,100);
				change_uinsight(effectcount);

				for(rolls = d(1,4); rolls > 0; rolls--){
					switch(rnd(4)){
						case 1:
							for(i = rnd(4); i > 0; i--){
								xp += d(1,200);
								learn_spell_aphanactonan(rn1(SPE_BLANK_PAPER - SPE_DIG, SPE_DIG));
							}
						break;
						case 2:
							if(!Role_if(PM_EXILE)){
								if(!seals){
									xp += 625;
									You("see circular seals!");
									seals = TRUE;
								}
								for(i = rnd(8); i > 0; i--){
									u.sealsKnown |= 0x1L<<rn2(YMIR-FIRST_SEAL+1);
								}
							}
						break;
						case 3:
							if(!wards){
								You("see warding signs!");
								wards = TRUE;
							}
							for(i = d(2,4); i > 0; i--){
								xp += d(1,50);
								u.wardsknown |= 0x1L<<rnd(NUM_WARDS-1); //Note: Ward_Elbereth is 0x1L, and does nothing.
							}
						break;
						case 4:
							if(!combat){
								You("suddenly know secret combat techniques!");
								combat = TRUE;
							}
							xp += 1000;
							u.uhitinc = min_ints(100, u.uhitinc+d(1,2));
							u.udaminc = min_ints(100, u.udaminc+d(1,2));
							u.uacinc = min_ints(100, u.uacinc+d(1,2));
						break;
					}
				}
				more_experienced(xp, 0);
				newexplevel();

				//Sanity
				change_usanity(-1*d(8,8),TRUE);
			}
		}
	} else if(scroll->otyp >= ANTI_CLOCKWISE_METAMORPHOSIS_G && scroll->otyp <= ORRERY_GLYPH) {
		thought = otyp_to_thought(scroll->otyp);

		/* maybe_give_thought checks requirements, returns FALSE if it didn't work */
		if (!maybe_give_thought(thought))
		{
			pline("Nothing happens.");
			return MOVE_INSTANT;
		}

	} else {
		impossible("bad shard");
		return MOVE_CANCELLED;
	}

	u.uconduct.literate++;

	//Speak one word of power per move free.
	if(objects[scroll->otyp].oc_magic){
		useup(scroll);
		check_illumian_trophy();
		return MOVE_READ|MOVE_PARTIAL;
	}
	else {
		useup(scroll);
		return MOVE_READ;
	}
}

static int delay = 0;
static struct obj *curslab = 0;

STATIC_PTR int
study_word(slab)
struct obj *slab;
{
	if (delay && !Confusion && slab == curslab) {
		You("continue your efforts to memorize the %s.", OBJ_DESCR(objects[slab->otyp]));
	} else {
		delay = -99;
		You("begin to study the %s.", OBJ_DESCR(objects[slab->otyp]));
	}
	curslab = slab;
	set_occupation(learn_word, "studying", 0);
	return MOVE_READ;
}

STATIC_PTR int
learn_word()
{
	if (Confusion) {		/* became confused while learning */
	    curslab = 0;			/* no longer studying */
	    nomul(delay, "studying a word");		/* remaining delay is uninterrupted */
	    delay = 0;
	    return MOVE_CANCELLED;
	}
	if (delay) {	/* not if (delay++), so at end delay == 0 */
	    delay++;
	    return MOVE_READ; /* still busy */
	}
	exercise(A_WIS, TRUE);		/* you're studying. */
	switch(curslab->otyp){
		default:
			impossible("bad slab.");
			return MOVE_FINISHED_OCCUPATION;
		case FIRST_WORD:
			You("learn the First Word of Creation!");
			u.ufirst_light = TRUE;
			u.ufirst_light_timeout = 0;
			HFlying |= FROMOUTSIDE;
			float_up();
			spoteffects(FALSE);
		break;
		case DIVIDING_WORD:
			You("learn the Dividing Word of Creation!");
			u.ufirst_sky = TRUE;
			u.ufirst_sky_timeout = 0;
		break;
		case NURTURING_WORD:
			You("learn the Nurturing Word of Creation!");
			u.ufirst_life = TRUE;
			u.ufirst_life_timeout = 0;
		break;
		case WORD_OF_KNOWLEDGE:
			You("learn the Red Word of Knowledge!");
			u.ufirst_know = TRUE;
			u.ufirst_know_timeout = 0;
		break;
	}
	if (flags.verbose) {
		char buf[BUFSZ];
		char buf2[BUFSZ];
		char * ptr;
		if ((ptr = find_command_key("ability", buf)))
			Sprintf(buf2, "or %s ", ptr);
		else
			Strcpy(buf2, "");
		pline("Use the command #ability %sto speak it.", buf2);
	}
	//Note: the word of knowledge can't actually give the mithardir trophy, but it's harmless to check.
	check_mithardir_trophy();
	check_illumian_trophy();
	if(Role_if(PM_EXILE) && u.ufirst_life && u.ufirst_sky && u.ufirst_light && (u.sealsUsed&SEAL_TENEBROUS) && !(u.specialSealsKnown&SEAL_LIVING_CRYSTAL)){
		pline("As you learn the Word, you also realize how the Words can be used in the drawing of a seal!");
		u.specialSealsKnown |= SEAL_LIVING_CRYSTAL;
	}
	useupall(curslab);
	return MOVE_FINISHED_OCCUPATION;
}

static void
stripspe(obj)
register struct obj *obj;
{
	if (obj->blessed) pline1(nothing_happens);
	else {
		if (obj->spe > 0) {
		    obj->spe = 0;
		    if (obj->otyp == OIL_LAMP || obj->otyp == LANTERN)
			obj->age = 0;
		    Your("%s %s briefly.",xname(obj), otense(obj, "vibrate"));
		} else pline1(nothing_happens);
	}
}

static void
p_glow1(otmp)
register struct obj	*otmp;
{
	Your("%s %s briefly.", xname(otmp),
	     otense(otmp, Blind ? "vibrate" : "glow"));
}

static void
p_glow2(otmp,color)
register struct obj	*otmp;
register const char *color;
{
	Your("%s %s%s%s for a moment.",
		xname(otmp),
		otense(otmp, Blind ? "vibrate" : "glow"),
		Blind ? "" : " ",
		Blind ? nul : hcolor(color));
}

/* Is the object chargeable?  For purposes of inventory display; it is */
/* possible to be able to charge things for which this returns FALSE. */
boolean
is_chargeable(obj)
struct obj *obj;
{
	if (obj->oclass == WAND_CLASS) return TRUE;
	/* known && !uname is possible after amnesia/mind flayer */
	if (obj->oclass == RING_CLASS)
	    return (boolean)(objects[obj->otyp].oc_charged &&
			(obj->known || objects[obj->otyp].oc_uname) && obj->otyp != RIN_WISHES);
	if (is_lightsaber(obj) && obj->oartifact != ART_INFINITY_S_MIRRORED_ARC && obj->otyp != KAMEREL_VAJRA)
	    return TRUE;
//#ifdef FIREARMS
	if (is_blaster(obj) && (obj->recharged < 4 || (obj->otyp != HAND_BLASTER && obj->otyp != ARM_BLASTER)))
	    return TRUE;
	if (is_vibroweapon(obj) || obj->otyp == SEISMIC_HAMMER)
	    return TRUE;
//#endif
	if (is_weptool(obj))	/* specific check before general tools */
	    return FALSE;
	if (obj->oclass == TOOL_CLASS)
	    return (boolean)(objects[obj->otyp].oc_charged);
	return FALSE; /* why are weapons/armor considered charged anyway? */
}

/*
 * recharge an object; curse_bless is -1 if the recharging implement
 * was cursed, +1 if blessed, 0 otherwise.
 *
 * Returns 1 if the item is destroyed
 */
int
recharge(obj, curse_bless)
struct obj *obj;
int curse_bless;
{
	register int n;
	boolean is_cursed, is_blessed;

	is_cursed = curse_bless < 0;
	is_blessed = curse_bless > 0;

	if (obj->oclass == WAND_CLASS) {
	    /* undo any prior cancellation, even when is_cursed */
	    if (obj->spe == -1) obj->spe = 0;

	    /*
	     * Recharging might cause wands to explode.
	     *	v = number of previous recharges
	     *	      v = percentage chance to explode on this attempt
	     *		      v = cumulative odds for exploding
	     *	0 :   0       0
	     *	1 :   0.29    0.29
	     *	2 :   2.33    2.62
	     *	3 :   7.87   10.28
	     *	4 :  18.66   27.02
	     *	5 :  36.44   53.62
	     *	6 :  62.97   82.83
	     *	7 : 100     100
	     */
	    n = (int)obj->recharged;
	    if (!(obj->oartifact) && n > 0 && (obj->otyp == WAN_WISHING ||
		    (n * n * n > rn2(7*7*7)))) {	/* recharge_limit */
		wand_explode(obj);
		return 1;
	    }
	    /* didn't explode, so increment the recharge count */
	    if(n < 7) obj->recharged = (unsigned)(n + 1);

	    /* now handle the actual recharging */
	    if (is_cursed) {
		stripspe(obj);
	    } else {
		int lim = (obj->otyp == WAN_WISHING) ? 3 :
			(objects[obj->otyp].oc_dir != NODIR) ? 8 : 15;

		n = (lim == 3) ? 3 : rn1(5, lim + 1 - 5);
		if (!is_blessed) n = rnd(n);

		if (obj->spe < n) obj->spe = n;
		else obj->spe++;
		if (obj->otyp == WAN_WISHING && obj->spe > 3) {
		    wand_explode(obj);
		    return 1;
		}
		if (obj->spe >= lim) p_glow2(obj, NH_BLUE);
		else p_glow1(obj);
	    }

	} else if (obj->oclass == RING_CLASS &&
					objects[obj->otyp].oc_charged && obj->otyp != RIN_WISHES) {
	    /* charging does not affect ring's curse/bless status */
	    int s = is_blessed ? rnd(3) : is_cursed ? -rnd(2) : 1;
	    boolean is_on = (obj == uleft || obj == uright);

	    /* destruction depends on current state, not adjustment */
	    if (obj->spe > (6-rnl(7)) || obj->spe <= -5) {
			if(!obj->oartifact){
				Your("%s %s momentarily, then %s!",
					 xname(obj), otense(obj,"pulsate"), otense(obj,"explode"));
				if (is_on) Ring_gone(obj);
				s = rnd(3 * abs(obj->spe));	/* amount of damage */
				useup(obj);
				losehp(s, "exploding ring", KILLED_BY_AN);
				return 1;
			} else {
				long mask = is_on ? (obj == uleft ? LEFT_RING :
							 RIGHT_RING) : 0L;
				Your("%s %s momentarily!", xname(obj), otense(obj,"pulsate"));
				/* cause attributes and/or properties to be updated */
				if (is_on) Ring_off(obj);
				obj->spe = 0;	/* update the ring while it's off */
				if (is_on) setworn(obj, mask), Ring_on(obj);
				/* oartifact: if a touch-sensitive artifact ring is
				   ever created the above will need to be revised  */
			}
		} else {
			long mask = is_on ? (obj == uleft ? LEFT_RING :
						 RIGHT_RING) : 0L;
			Your("%s spins %sclockwise for a moment.",
				 xname(obj), s < 0 ? "counter" : "");
			/* cause attributes and/or properties to be updated */
			if (is_on) Ring_off(obj);
			obj->spe += s;	/* update the ring while it's off */
			if (is_on) setworn(obj, mask), Ring_on(obj);
			/* oartifact: if a touch-sensitive artifact ring is
			   ever created the above will need to be revised  */
	    }

	} else if (obj->oclass == TOOL_CLASS || is_blaster(obj)
		   || obj->otyp == DWARVISH_HELM || obj->otyp == LANTERN_PLATE_MAIL
		   || is_vibroweapon(obj)
	   ) {
	    int rechrg = (int)obj->recharged;

	    if (objects[obj->otyp].oc_charged) {
		/* tools don't have a limit, but the counter used does */
		if (rechrg < 7)	/* recharge_limit */
		    obj->recharged++;
	    }
	    switch(obj->otyp) {
	    case BELL_OF_OPENING:
		if (is_cursed) stripspe(obj);
		else if (is_blessed) obj->spe += rnd(3);
		else obj->spe += 1;
		if (obj->spe > 5) obj->spe = 5;
		break;
	    case MAGIC_MARKER:
	    case TINNING_KIT:
	    case DISSECTION_KIT:
#ifdef TOURIST
	    case EXPENSIVE_CAMERA:
#endif
		if (is_cursed) stripspe(obj);
		else if (rechrg && obj->otyp == MAGIC_MARKER) {	/* previously recharged */
		    obj->recharged = 1;	/* override increment done above */
		    if (obj->spe < 3)
			Your("marker seems permanently dried out.");
		    else
			pline1(nothing_happens);
		} else if (is_blessed) {
		    n = rn1(16,15);		/* 15..30 */
		    if (obj->spe + n <= 50)
			obj->spe = 50;
		    else if (obj->spe + n <= 75)
			obj->spe = 75;
		    else {
			int chrg = (int)obj->spe;
			if ((chrg + n) > 127)
				obj->spe = 127;
			else
				obj->spe += n;
		    }
		    p_glow2(obj, NH_BLUE);
		} else {
		    n = rn1(11,10);		/* 10..20 */
		    if (obj->spe + n <= 50)
			obj->spe = 50;
		    else {
			int chrg = (int)obj->spe;
			if ((chrg + n) > 127)
				obj->spe = 127;
			else
				obj->spe += n;
		    }
		    p_glow2(obj, NH_WHITE);
		}
		break;
	    case DWARVISH_HELM:
	    case OIL_LAMP:
	    case LANTERN:
	    case LANTERN_PLATE_MAIL:
		if (is_cursed) {
		    if (obj->otyp == DWARVISH_HELM && obj->otyp != LANTERN_PLATE_MAIL) {
			/* Don't affect the +/- of the helm */
			obj->age = 0;
		    }
		    else 
				stripspe(obj);
		    if (obj->lamplit) {
			if (!Blind)
			    pline("%s out!", Tobjnam(obj, "go"));
			end_burn(obj, TRUE);
		    }
		} else if (is_blessed) {
		    if (obj->otyp != DWARVISH_HELM && obj->otyp != LANTERN_PLATE_MAIL) {
				obj->spe = 1;
		    }
		    obj->age = 1500;
		    p_glow2(obj, NH_BLUE);
		} else {
		    if (obj->otyp != DWARVISH_HELM) {
				obj->spe = 1;
		    }
		    obj->age += 750;
		    if (obj->age > 1500) obj->age = 1500;
		    p_glow1(obj);
		}
		break;
//#ifdef FIREARMS
	    case HAND_BLASTER:
	    case ARM_BLASTER:
			if(obj->recharged >= 4){
				obj->recharged = 4;
			} else {
				if(is_blessed) obj->ovar1_charges = 100L;
				else if(is_cursed) obj->ovar1_charges = 10L;
				else obj->ovar1_charges = 80L + rn2(20);
			}
		break;
	    case CARCOSAN_STING:
			if(is_blessed) obj->ovar1_charges = 50L;
			else if(is_cursed) obj->ovar1_charges = 5L;
			else obj->ovar1_charges = 20L + d(5,5);
		break;
	    case MASS_SHADOW_PISTOL:
			if(is_blessed) obj->ovar1_charges = 1000L;
			else if(is_cursed) obj->ovar1_charges = 100L;
			else obj->ovar1_charges = 800L + rn2(200);
		break;
	    case CUTTING_LASER:
	    case VIBROBLADE:
	    case WHITE_VIBROSWORD:
	    case GOLD_BLADED_VIBROSWORD:
	    case WHITE_VIBROZANBATO:
	    case GOLD_BLADED_VIBROZANBATO:
	    case RED_EYED_VIBROSWORD:
	    case WHITE_VIBROSPEAR:
	    case GOLD_BLADED_VIBROSPEAR:
	    case FORCE_PIKE:
	    case FORCE_BLADE:
	    case DOUBLE_FORCE_BLADE:
	    case FORCE_SWORD:
	    case FORCE_WHIP:
	    case SEISMIC_HAMMER:
			if(is_blessed) obj->ovar1_charges = 100L;
			else if(is_cursed) obj->ovar1_charges = 10L;
			else obj->ovar1_charges = 80L + rn2(20);
		break;
	    case RAYGUN:
			if(Role_if(PM_ANACHRONONAUT) || Role_if(PM_TOURIST)){
				if(is_blessed) obj->ovar1_charges = 160L;
				else if(is_cursed) obj->ovar1_charges = 10L;
				else obj->ovar1_charges = (8 + rn2(8))*10L;
			} else {
				//The Raygun's power cell is damaged:
				if(is_blessed) obj->ovar1_charges = 15L;
				else if(is_cursed) obj->ovar1_charges = 2L;
				else obj->ovar1_charges = 2+rnd(5)*2;
			}
		break;
//#endif
	    case LIGHTSABER:
	    case BEAMSWORD:
	    case DOUBLE_LIGHTSABER:
	    case ROD_OF_FORCE:
		if (is_cursed) {
		    if (obj->lamplit) {
			end_burn(obj, TRUE);
			obj->age = 0;
			if (!Blind)
			    pline("%s deactivates!", The(xname(obj)));
		    } else
			obj->age = 0;
		} else if (is_blessed) {
		    obj->age = LIGHTSABER_MAX_CHARGE;
		    p_glow2(obj, NH_BLUE);
		} else {
		    obj->age += LIGHTSABER_MAX_CHARGE/2;
		    if (obj->age > LIGHTSABER_MAX_CHARGE) obj->age = LIGHTSABER_MAX_CHARGE;
		    p_glow1(obj);
		}
		break;
	    case SENSOR_PACK:
			if(is_blessed) obj->spe = 25;
			else if(is_cursed) obj->spe = 0;
			else obj->spe = rn1(12,12);
		break;
	    case CRYSTAL_BALL:
		if (is_cursed) stripspe(obj);
		else if (is_blessed) {
		    obj->spe = 6;
		    p_glow2(obj, NH_BLUE);
		} else {
		    if (obj->spe < 5) {
			obj->spe++;
			p_glow1(obj);
		    } else pline1(nothing_happens);
		}
		break;
	    case PRESERVATIVE_ENGINE:
		if (is_cursed){
		    obj->spe = 8;
		    p_glow2(obj, NH_RED);
		}
		else if (is_blessed) {
			stripspe(obj);
		} else {
		    if (obj->spe < 8) {
				obj->spe++;
				p_glow1(obj);
		    } else pline1(nothing_happens);
		}
		break;
	    case ARMOR_SALVE:
		if (is_cursed){
		    obj->spe = 6;
		    p_glow2(obj, NH_RED);
		}
		else if (is_blessed) {
			stripspe(obj);
		} else {
		    if (obj->spe < 6) {
				obj->spe++;
				p_glow1(obj);
		    } else pline1(nothing_happens);
		}
		break;
	    case HORN_OF_PLENTY:
	    case BAG_OF_TRICKS:
	    case CAN_OF_GREASE:
	    case TREPHINATION_KIT:
	    case MIST_PROJECTOR:
		if (is_cursed) stripspe(obj);
		else if (is_blessed) {
		    if (obj->spe <= 10)
			obj->spe += rn1(10, 6);
		    else obj->spe += rn1(5, 6);
		    if (obj->spe > 50) obj->spe = 50;
		    p_glow2(obj, NH_BLUE);
		} else {
		    obj->spe += rnd(5);
		    if (obj->spe > 50) obj->spe = 50;
		    p_glow1(obj);
		}
		break;
		case PHLEBOTOMY_KIT:
		if (is_cursed) stripspe(obj);
		else if (is_blessed) {
		    if (obj->spe <= 0)
				obj->spe += rn1(3, 3);
		    else obj->spe += 1;
		    if (obj->spe > 20) obj->spe = 20;
		    p_glow2(obj, NH_BLUE);
		} else {
		    obj->spe += 1;
		    if (obj->spe > 20) obj->spe = 20;
		    p_glow1(obj);
		}
		break;
	    case MAGIC_FLUTE:
	    case MAGIC_HARP:
	    case FROST_HORN:
	    case FIRE_HORN:
	    case DRUM_OF_EARTHQUAKE:
		if (is_cursed) {
		    stripspe(obj);
		} else if (is_blessed) {
		    obj->spe += d(2,4);
		    if (obj->spe > 20) obj->spe = 20;
		    p_glow2(obj, NH_BLUE);
		} else {
		    obj->spe += rnd(4);
		    if (obj->spe > 20) obj->spe = 20;
		    p_glow1(obj);
		}
		break;
	    default:
		goto not_chargable;
		/*NOTREACHED*/
		break;
	    } /* switch */

	} else {
 not_chargable:
	    You("have a feeling of loss.");
	}
	return 0;
}


/* Forget known information about this object class. */
static void
forget_single_object(obj_id)
	int obj_id;
{
	objects[obj_id].oc_name_known = 0;
	objects[obj_id].oc_pre_discovered = 0;	/* a discovery when relearned */
	if (objects[obj_id].oc_uname) {
	    free((genericptr_t)objects[obj_id].oc_uname);
	    objects[obj_id].oc_uname = 0;
	}
	undiscover_object(obj_id);	/* after clearing oc_name_known */

	/* clear & free object names from matching inventory items too? */
}


#if 0	/* here if anyone wants it.... */
/* Forget everything known about a particular object class. */
static void
forget_objclass(oclass)
	int oclass;
{
	int i;

	for (i=bases[oclass];
		i < NUM_OBJECTS && objects[i].oc_class==oclass; i++)
	    forget_single_object(i);
}
#endif


/* randomize the given list of numbers  0 <= i < count */
static void
randomize(indices, count)
	int *indices;
	int count;
{
	int i, iswap, temp;

	for (i = count - 1; i > 0; i--) {
	    if ((iswap = rn2(i + 1)) == i) continue;
	    temp = indices[i];
	    indices[i] = indices[iswap];
	    indices[iswap] = temp;
	}
}


/* Forget % of known objects. */
void
forget_objects(percent)
	int percent;
{
	int i, count;
	int indices[NUM_OBJECTS];
	
	return; //Disable object forgetting
	
	if (percent == 0) return;
	if (percent <= 0 || percent > 100) {
	    impossible("forget_objects: bad percent %d", percent);
	    return;
	}

	for (count = 0, i = 1; i < NUM_OBJECTS; i++)
	    if (OBJ_DESCR(objects[i]) &&
		    (objects[i].oc_name_known || objects[i].oc_uname))
		indices[count++] = i;

	randomize(indices, count);
	
	/* forget first % of randomized indices */
	count = ((count * percent) + 50) / 100;
	for (i = 0; i < count; i++)
	    forget_single_object(indices[i]);
	
	if(rn2(100) < percent) u.uevent.uread_necronomicon = 0; /* potentially forget having read necronomicon */
}


/* Forget some or all of map (depends on parameters). */
void
forget_map(howmuch)
	int howmuch;
{
	register int zx, zy;
	
	if(iflags.no_forget_map)
		return;
	
	if (In_sokoban(&u.uz))
	    return;

	known = TRUE;
	for(zx = 0; zx < COLNO; zx++) for(zy = 0; zy < ROWNO; zy++)
		if (howmuch >= 100 || rn2(100) < howmuch) {
			/* Zonk all memory of this location. */
			levl[zx][zy].seenv = 0;
			levl[zx][zy].waslit = 0;
			levl[zx][zy].glyph = cmap_to_glyph(S_stone);
			levl[zx][zy].styp = STONE;
		}
}

/* Forget all traps on the level. */
void
forget_traps()
{
	register struct trap *trap;

	/* forget all traps (except the one the hero is in :-) */
	for (trap = ftrap; trap; trap = trap->ntrap)
	    if ((trap->tx != u.ux || trap->ty != u.uy)
			&& (trap->ttyp != HOLE)
			&& !(trap->ttyp == MAGIC_PORTAL && visible_portals(&u.uz))
		)
		trap->tseen = 0;
}

/*
 * Forget given % of all levels that the hero has visited and not forgotten,
 * except this one.
 */
void
forget_levels(percent)
	int percent;
{
	int i, count;
	xchar  maxl, this_lev;
	int indices[MAXLINFO];

	if (percent == 0) return;

	if(iflags.no_forget_map)
		return;

	if (percent <= 0 || percent > 100) {
	    impossible("forget_levels: bad percent %d", percent);
	    return;
	}

	this_lev = ledger_no(&u.uz);
	maxl = maxledgerno();

	/* count & save indices of non-forgotten visited levels */
	/* Sokoban levels are pre-mapped for the player, and should stay
	 * so, or they become nearly impossible to solve.  But try to
	 * shift the forgetting elsewhere by fiddling with percent
	 * instead of forgetting fewer levels.
	 */
	for (count = 0, i = 0; i <= maxl; i++)
	    if ((level_info[i].flags & VISITED) &&
			!(level_info[i].flags & FORGOTTEN) && i != this_lev) {
		if (ledger_to_dnum(i) == sokoban_dnum)
		    percent += 2;
		else
		    indices[count++] = i;
	    }
	
	if (percent > 100) percent = 100;

	randomize(indices, count);

	/* forget first % of randomized indices */
	count = ((count * percent) + 50) / 100;
	for (i = 0; i < count; i++) {
	    level_info[indices[i]].flags |= FORGOTTEN;
//		forget_mapseen(indices[i]);
	}
}

/* Forget % of insight gained and sanity lost from monsters. */
void
losesaninsight(percent)
	int percent;
{
	int i, count = 0;
	int indices[NUMMONS] = {0};
	
	for(i = 0; i < NUMMONS; i++){
		if(mvitals[i].san_lost != 0 || mvitals[i].insight_gained != 0)
			indices[count++] = i;
	}
	randomize(indices, count);

	/* forget first % of randomized indices */
	count = ((count * percent) + 50) / 100;
	for (i = 0; i < count; i++) {
		if(discover || wizard)
			pline("Forgeting %s", mons[indices[i]].mname);
		change_usanity(-1*mvitals[indices[i]].san_lost, FALSE);
	    mvitals[indices[i]].san_lost = 0;
		change_uinsight(-1*mvitals[indices[i]].insight_gained);
	    mvitals[indices[i]].insight_gained = 0;
	    mvitals[indices[i]].vis_insight = FALSE;
	    mvitals[indices[i]].insightkill = FALSE;
	}
}

/*
 * Forget some things (e.g. after reading a scroll of amnesia).  When called,
 * the following are always forgotten:
 *
 *	- felt ball & chain
 *	- traps
 *	- part (6 out of 7) of the map
 *
 * Other things are subject to flags:
 *
 *	howmuch & ALL_MAP	= forget whole map
 *	howmuch & ALL_SPELLS	= forget all spells
 */
void
forget(howmuch)
int howmuch;
{
	u.wimage = 0; //clear wa image from your mind
	
	if(u.sealsActive&SEAL_HUGINN_MUNINN){
		unbind(SEAL_HUGINN_MUNINN,TRUE);
		return; //expel and end
	}
	if (Punished) u.bc_felt = 0;	/* forget felt ball&chain */

	forget_map(howmuch);
	// forget_traps();
	
	//Silently reduce the doubt timer (itimeout_incr handles negative timeouts)
	if(HDoubt){
		make_doubtful(itimeout_incr(HDoubt, -1*howmuch), FALSE);
	}
	
	/* 1 in 3 chance of forgetting some levels */
	if (howmuch && !rn2(3)) forget_levels(howmuch);

	/* 1 in 3 chance of forgeting some objects */
	if (howmuch && !rn2(3)) forget_objects(howmuch);

	if (howmuch) losespells(howmuch);
	
	if(howmuch) losesaninsight(howmuch);
	/*
	 * Make sure that what was seen is restored correctly.  To do this,
	 * we need to go blind for an instant --- turn off the display,
	 * then restart it.  All this work is needed to correctly handle
	 * walls which are stone on one side and wall on the other.  Turning
	 * off the seen bits above will make the wall revert to stone,  but
	 * there are cases where we don't want this to happen.  The easiest
	 * thing to do is to run it through the vision system again, which
	 * is always correct.
	 */
	docrt();		/* this correctly will reset vision */
}

/* monster is hit by scroll of taming's effect */
static void
maybe_tame(mtmp, sobj)
struct monst *mtmp;
struct obj *sobj;
{
	if (sobj->cursed) {
	    setmangry(mtmp);
	} else {
	    if (mtmp->isshk)
			make_happy_shk(mtmp, FALSE);
	    else if (sobj->otyp == SPE_CHARM_MONSTER){
			int skill = spell_skilltype(sobj->otyp);
			int role_skill = P_SKILL(skill)-1; //P_basic would be 2
			if(Spellboost) role_skill++;
			if(u.cuckoo && active_glyph(LUMEN))
				role_skill += u.cuckoo/3;
			if(role_skill < 1)
				role_skill = 1;
			if(sobj->blessed)
				role_skill++;
			if(u.ukrau_duration)
				role_skill += (role_skill+1)/2;
			
			for(; role_skill; role_skill--)
				if(!resist(mtmp, sobj->oclass, 0, NOTELL)){
					(void) tamedog(mtmp, sobj);
					return;
				}
	    } else if (!resist(mtmp, sobj->oclass, 0, NOTELL))
			(void) tamedog(mtmp, sobj);
	}
}

int
seffects(sobj)
struct obj	*sobj;
{
	int cval;
	boolean confused = (Confusion != 0) || sobj->forceconf;
	struct obj *otmp;
	boolean extra = FALSE;
	

	if (objects[sobj->otyp].oc_magic)
		exercise(A_WIS, TRUE);		/* just for trying */
	switch(sobj->otyp) {
#ifdef MAIL
	case SCR_MAIL:
		known = TRUE;
		if (sobj->spe)
		    pline("This seems to be junk mail addressed to the finder of the Eye of Larn.");
		/* note to the puzzled: the game Larn actually sends you junk
		 * mail if you win!
		 */
		else readmail(sobj);
		break;
#endif
	case SCR_ENCHANT_ARMOR:
	    {
		register schar s;
		boolean special_armor, plusten_armor;
		boolean same_color;

		otmp = some_armor(&youmonst);
		if(!otmp) {
			exercise(A_CON, !sobj->cursed);
			exercise(A_STR, !sobj->cursed);
			/* strange feeling may free sobj */
			strange_feeling(sobj,
					!Blind ? "Your skin glows then fades." :
					"Your skin feels warm for a moment.");
			return(1);
		}
		if(confused) {
			otmp->oerodeproof = !(sobj->cursed);
			if (otmp->otyp >= GRAY_DRAGON_SCALES &&
				otmp->otyp <= YELLOW_DRAGON_SCALES) {
				/* dragon scales get turned into dragon scale shield */
				if(Blind) {
					Your("%s %s harder!", xname(otmp), otense(otmp, "feel"));
				} else {
					Your("%s merges and hardens!", xname(otmp));
				}
				boolean is_worn = !!(otmp->owornmask & W_ARM);
				if (is_worn) {
					setworn((struct obj *)0, W_ARM);
				}
				/* assumes same order */
				otmp->otyp = GRAY_DRAGON_SCALE_SHIELD +
					otmp->otyp - GRAY_DRAGON_SCALES;
				otmp->objsize = youracedata->msize;
				otmp->cursed = 0;
				if (sobj->blessed) {
					otmp->spe++;
					otmp->blessed = 1;
				}
				otmp->known = 1;
				fix_object(otmp);

				long wornmask = 0L;
				if (is_worn && canwearobj(otmp, &wornmask, FALSE)) {
					setworn(otmp, wornmask);
				}
				break;
			} else {
				if(Blind) {
					otmp->rknown = FALSE;
					Your("%s %s warm for a moment.",
					xname(otmp), otense(otmp, "feel"));
				} else {
					otmp->rknown = TRUE;
					Your("%s %s covered by a %s %s %s!",
					xname(otmp), otense(otmp, "are"),
					sobj->cursed ? "mottled" : "shimmering",
					 hcolor(sobj->cursed ? NH_BLACK : NH_GOLDEN),
					sobj->cursed ? "glow" :
					  (is_shield(otmp) ? "layer" : "shield"));
				}
				if (otmp->oerodeproof &&
					(otmp->oeroded || otmp->oeroded2)) {
					otmp->oeroded = otmp->oeroded2 = 0;
					Your("%s %s as good as new!",
					 xname(otmp),
					 otense(otmp, Blind ? "feel" : "look"));
				}
			}
			break;
		}
		if(is_belt(otmp) && otmp->otyp != KIDNEY_BELT){
			Your("%s %s.",
			 xname(otmp),
			 otense(otmp, Blind ? "feels warm for a moment" : "glows and then fades"));
			break;
		}
		/* elven armor vibrates warningly when enchanted beyond a limit */
		special_armor = is_plussev_armor(otmp);
		plusten_armor = is_plusten(otmp);
		if (sobj->cursed)
		    same_color =
			(otmp->otyp == BLACK_DRAGON_SCALE_MAIL ||
			 otmp->otyp == BLACK_DRAGON_SCALE_SHIELD ||
			 otmp->otyp == BLACK_DRAGON_SCALES);
		else
		    same_color =
			(otmp->otyp == SILVER_DRAGON_SCALE_MAIL ||
			 otmp->otyp == SILVER_DRAGON_SCALE_SHIELD ||
			 otmp->otyp == SILVER_DRAGON_SCALES ||
			 otmp->otyp == SHIELD_OF_REFLECTION);
		if (Blind) same_color = FALSE;

		/* KMH -- catch underflow */
		s = sobj->cursed ? -otmp->spe : otmp->spe;
		if (s > (plusten_armor ? 9 : special_armor ? 5 : 3) && rn2(s)) {
			if(otmp->oartifact){
				int delta = 0 - otmp->spe;
				Your("%s violently %s%s%s for a while, then %s.",
					 xname(otmp),
					 otense(otmp, Blind ? "vibrate" : "glow"),
					 (!Blind && !same_color) ? " " : nul,
					 (Blind || same_color) ? nul :
					hcolor(sobj->cursed ? NH_BLACK : NH_SILVER),
					 otense(otmp, "fade"));
					otmp->spe = 0;
					adj_abon(otmp, delta);
			} else {
				Your("%s violently %s%s%s for a while, then %s.",
					 xname(otmp),
					 otense(otmp, Blind ? "vibrate" : "glow"),
					 (!Blind && !same_color) ? " " : nul,
					 (Blind || same_color) ? nul :
					hcolor(sobj->cursed ? NH_BLACK : NH_SILVER),
					 otense(otmp, "evaporate"));
				if(is_cloak(otmp)) (void) Cloak_off();
				if(is_boots(otmp)) (void) Boots_off();
				if(is_helmet(otmp)) (void) Helmet_off();
				if(is_gloves(otmp)) (void) Gloves_off();
				if(is_shield(otmp)) (void) Shield_off();
				if(otmp == uarm) (void) Armor_gone();
				useup(otmp);
			}
			break;
		}
		s = sobj->cursed ? -1 :
		    otmp->spe >= 9 ? (rn2(otmp->spe) == 0) :
		    sobj->blessed ? rnd(3-otmp->spe/3) : 1;
		if (s >= 0 && otmp->otyp >= GRAY_DRAGON_SCALES &&
					otmp->otyp <= YELLOW_DRAGON_SCALES) {
			
			//Don't end up wearing invalid dragon scale mail!
			if(!(youracedata->mflagsb&MB_BODYTYPEMASK)){
				Your("%s begins to merge and harden, but your oddly-shaped body interferes.", xname(otmp));
				pline("The magic fizzles!");
				break;
			}
			/* dragon scales get turned into dragon scale mail */
			boolean is_worn = !!(otmp->owornmask & W_ARM);

			Your("%s merges and hardens!", xname(otmp));
			if (is_worn) {
				setworn((struct obj *)0, W_ARM);
			}
			/* assumes same order */
			otmp->otyp = GRAY_DRAGON_SCALE_MAIL +
						otmp->otyp - GRAY_DRAGON_SCALES;
			otmp->objsize = youracedata->msize;
			
			set_obj_shape(otmp, youracedata->mflagsb);
			
			otmp->cursed = 0;
			if (sobj->blessed) {
				otmp->spe++;
				otmp->blessed = 1;
			}
			otmp->known = 1;
			fix_object(otmp);
			long wornmask = 0L;
			if (is_worn && canwearobj(otmp, &wornmask, FALSE)) {
				setworn(otmp, W_ARM);
			}
			break;
		}
		Your("%s %s%s%s%s for a %s.",
			xname(otmp),
		        s == 0 ? "violently " : nul,
			otense(otmp, Blind ? "vibrate" : "glow"),
			(!Blind && !same_color) ? " " : nul,
			(Blind || same_color) ? nul : hcolor(sobj->cursed ? NH_BLACK : NH_SILVER),
			  (s*s>1) ? "while" : "moment");
		otmp->cursed = sobj->cursed;
		if (!otmp->blessed || sobj->cursed)
			otmp->blessed = sobj->blessed;
		if (s) {
			otmp->spe += s;
			adj_abon(otmp, s);
			known = otmp->known;
		}

		if ((otmp->spe > (plusten_armor ? 9 : special_armor ? 5 : 3)) &&
		    (plusten_armor || special_armor || !rn2(7)))
			Your("%s suddenly %s %s.",
				xname(otmp), otense(otmp, "vibrate"),
				Blind ? "again" : "unexpectedly");
		break;
	    }
	case SCR_DESTROY_ARMOR:
	    {
		otmp = some_armor(&youmonst);
		if(confused) {
			if(!otmp) {
				char tempBuff[BUFSZ];
				Sprintf(tempBuff, "Your %s itch.", body_part(BONES));
				exercise(A_STR, FALSE);
				exercise(A_CON, FALSE);
				/* Strange feeling may free the object it's given */
				strange_feeling(sobj,tempBuff);
//				strange_feeling(sobj,"Your bones itch.");
				return(1);
			}
			otmp->oerodeproof = sobj->cursed;
			p_glow2(otmp, NH_PURPLE);
			break;
		}
		if(!sobj->cursed || !otmp || !otmp->cursed) {
		    if(!destroy_arm(otmp)) {
				char tempBuff[BUFSZ];
				Sprintf(tempBuff, "Your %s itches.", body_part(BODY_SKIN));
				exercise(A_STR, FALSE);
				exercise(A_CON, FALSE);
				/* Strange feeling may free the object it's given */
				strange_feeling(sobj,tempBuff);
			return(1);
		    } else
			known = TRUE;
		} else {	/* armor and scroll both cursed */
		    Your("%s %s.", xname(otmp), otense(otmp, "vibrate"));
		    if (otmp->spe >= -6) otmp->spe--;
			if (otmp->spe >= -6) {
				otmp->spe += -1;
				adj_abon(otmp, -1);
			}
		    make_stunned(HStun + rn1(10, 10), TRUE);
		}
	    }
	    break;
	case SCR_CONFUSE_MONSTER:
	case SPE_CONFUSE_MONSTER:
		// if(youracedata->mlet != S_HUMAN || sobj->cursed) {
			// if(!HConfusion) You_feel("confused.");
			// make_confused(HConfusion + rnd(100),FALSE);
		// } else  if(confused) {
		if(confused) {
		    if(!sobj->blessed) {
			Your("%s begin to %s%s.",
			    makeplural(body_part(HAND)),
			    Blind ? "tingle" : "glow ",
			    Blind ? nul : hcolor(NH_PURPLE));
			make_confused(HConfusion + rnd(100),FALSE);
		    } else {
			pline("A %s%s surrounds your %s.",
			    Blind ? nul : hcolor(NH_RED),
			    Blind ? "faint buzz" : " glow",
			    body_part(HEAD));
			make_confused(0L,TRUE);
		    }
		} else {
		    if (!sobj->blessed) {
				Your("%s%s %s%s.",
					makeplural(body_part(HAND)), Blind ? "" : " begin to glow",
					Blind ? (const char *)"tingle" : hcolor(NH_RED), u.umconf ? " even more" : "");
				u.umconf++;
		    } else {
			if (Blind)
			    Your("%s tingle %s sharply.",
				makeplural(body_part(HAND)),
				u.umconf ? "even more" : "very");
			else
			    Your("%s glow%s %s.",
				makeplural(body_part(HAND)),
				u.umconf ? " even more" : "",
				hcolor(NH_RED));
			/* after a while, repeated uses become less effective */
			if (u.umconf >= 40)
			    u.umconf++;
			else
			    u.umconf += rn1(8, 2);
		    }
		}
		break;
	case SCR_SCARE_MONSTER:
	case SPE_CAUSE_FEAR:
	    {	register int ct = 0;
		register struct monst *mtmp;

		for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		    if (DEADMONSTER(mtmp)) continue;
		    if (mindless_mon(mtmp)) continue;
		    if(cansee(mtmp->mx,mtmp->my)) {
			if(confused || sobj->cursed) {
			    mtmp->mflee = mtmp->mfrozen = mtmp->msleeping = 0;
			    mtmp->mcanmove = 1;
			} else
			    if (! resist(mtmp, sobj->oclass, 0, NOTELL))
				monflee(mtmp, 0, FALSE, FALSE);
			if(!mtmp->mtame) ct++;	/* pets don't laugh at you */
		    }
		}
		if(!ct)
		      You_hear("%s in the distance.",
			       (confused || sobj->cursed) ? "sad wailing" :
							"maniacal laughter");
		else if(sobj->otyp == SCR_SCARE_MONSTER)
			You_hear("%s close by.",
				  (confused || sobj->cursed) ? "sad wailing" :
						 "maniacal laughter");
		break;
	    }
	case SCR_BLANK_PAPER:
		if(sobj->oartifact == ART_PAINTING_FRAGMENT){
			You("can't make out any detail. There seems to have been a lot of white paint involved, though.");
		} else if(sobj->oartifact == ART_RITE_OF_DETESTATION){
			doparticularinvoke(sobj);
		} else {
			if (Blind)
			You("don't remember there being any magic words on this scroll.");
			else
			pline("This scroll seems to be blank.");
			known = TRUE;
		}
	    break;
	case SCR_REMOVE_CURSE:
	case SPE_REMOVE_CURSE:
	    {	register struct obj *obj;
		if(confused)
		    if (Hallucination)
			You_feel("the power of the Force against you!");
		    else
			You_feel("like you need some help.");
		else
		    if (Hallucination)
			You_feel("in touch with the Universal Oneness.");
		    else
			You_feel("like someone is helping you.");

		if (sobj->cursed) {
		    pline_The("scroll disintegrates.");
		} else {
			if(!confused && u.sealsActive&SEAL_MARIONETTE){
				unbind(SEAL_MARIONETTE,TRUE);
			}
			if(!confused){
				u.wimage = 0;
				if(youmonst.mbleed)
					Your("accursed wound closes up.");
				youmonst.mbleed = 0;
			}
		    for (obj = invent; obj; obj = obj->nobj) {
			long wornmask;
#ifdef GOLDOBJ
			/* gold isn't subject to cursing and blessing */
			if (obj->oclass == COIN_CLASS) continue;
#endif
			wornmask = (obj->owornmask & ~(W_BALL|W_ART|W_ARTI));
			if (wornmask && !sobj->blessed) {
			    /* handle a couple of special cases; we don't
			       allow auxiliary weapon slots to be used to
			       artificially increase number of worn items */
			    if (obj == uswapwep) {
				if (!u.twoweap) wornmask = 0L;
			    } else if (obj == uquiver) {
				if (obj->oclass == WEAPON_CLASS) {
				    /* mergeable weapon test covers ammo,
				       missiles, spears, daggers & knives */
				    if (!objects[obj->otyp].oc_merge) 
					wornmask = 0L;
				} else if (obj->oclass == GEM_CLASS) {
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
			if (sobj->blessed || wornmask ||
			     obj->otyp == LOADSTONE ||
			     (obj->otyp == LEASH && obj->leashmon)) {
			    if(confused) blessorcurse(obj, 2);
			    else uncurse(obj);
			}
		    }
#ifdef STEED
			/* also affects saddles */
			if (u.usteed) {	
				obj = which_armor(u.usteed, W_SADDLE);
				if (obj) {
					if (confused) blessorcurse(obj, 2);
					else uncurse(obj);
				}
			}
#endif
		}
		if(Punished && !confused) unpunish();
		if(u.umummyrot && !confused){
			u.umummyrot = 0;
			You("stop shedding dust.");
		}
		update_inventory();
		break;
	    }
	case SCR_CREATE_MONSTER:
	case SPE_CREATE_MONSTER:
		if(!DimensionalLock){
			if (create_critters(1 + ((confused || sobj->cursed) ? 12 : 0) +
					((sobj->blessed || rn2(73)) ? 0 : rnd(4)),
				confused ? &mons[PM_ACID_BLOB] : (struct permonst *)0)
			)
				known = TRUE;
			/* no need to flush monsters; we ask for identification only if the
			 * monsters are not visible
			 */
		}
		else {
			pline1(nothing_happens);
		}
	    break;
	case SCR_ENCHANT_WEAPON:
		if(uwep && (uwep->oclass == WEAPON_CLASS || is_weptool(uwep) || uwep->otyp == STILETTOS || uwep->otyp == WIND_AND_FIRE_WHEELS)
			&& confused && uwep->oartifact != ART_ROD_OF_SEVEN_PARTS) {
		/* read text for the rod of seven parts may lead players to think they need to errode-proof it.
			Since this is a logical deduction, it is not penalized. CM */
		/* oclass check added 10/25/86 GAN */
			uwep->oerodeproof = !(sobj->cursed);
			if (Blind) {
			    uwep->rknown = FALSE;
			    Your("weapon feels warm for a moment.");
			} else {
			    uwep->rknown = TRUE;
			    Your("%s covered by a %s %s %s!",
				aobjnam(uwep, "are"),
				sobj->cursed ? "mottled" : "shimmering",
				hcolor(sobj->cursed ? NH_PURPLE : NH_GOLDEN),
				sobj->cursed ? "glow" : "shield");
			}
			if (uwep->oerodeproof && (uwep->oeroded || uwep->oeroded2)) {
			    uwep->oeroded = uwep->oeroded2 = 0;
			    Your("%s as good as new!",
				 aobjnam(uwep, Blind ? "feel" : "look"));
			}
		} else return !chwepon(sobj,
				       sobj->cursed ? -1 :
				       !uwep ? 1 :
				       uwep->spe >= 10 ? (rn2(uwep->spe) == 0) :
				       sobj->blessed ? rnd(3-uwep->spe/3) : 1);
		break;
	case SCR_TAMING:
	case SPE_PACIFY_MONSTER:
	case SPE_CHARM_MONSTER:
		if (u.uswallow) {
		    maybe_tame(u.ustuck, sobj);
		} else {
		    struct monst *mtmp;
			if(confused && sobj->otyp == SCR_TAMING){
				int i, j, bd = 5;
				for (i = -bd; i <= bd; i++) for(j = -bd; j <= bd; j++) {
					if (!isok(u.ux + i, u.uy + j)) continue;
					mtmp = m_at(u.ux + i, u.uy + j);
					if(mtmp){
						if(!mtmp->mtame){
							boolean waspeaceful = mtmp->mpeaceful;
							maybe_tame(mtmp, sobj);
							mtmp = m_at(u.ux + i, u.uy + j); /* Old monster was freed if it was tamed */
							if(mtmp && mtmp->mtame){
								EDOG(mtmp)->friend = TRUE;
								mtmp->mtame += ACURR(A_CHA)*10;
								EDOG(mtmp)->waspeaceful = waspeaceful;
								if(!waspeaceful) mtmp->mpeacetime += ACURR(A_CHA);
							}
						} else if(EDOG(mtmp)->friend){
							mtmp->mtame += ACURR(A_CHA)*10;
							if(mtmp->mpeacetime) mtmp->mpeacetime += ACURR(A_CHA);
						} else if(mtmp->mpeacetime) mtmp->mpeacetime += ACURR(A_CHA);
					}
				}
			} else {
				getdir((char *)0);
				while(!(u.dx || u.dy)){
					u.dx = rn2(3)-1;
					u.dy = rn2(3)-1;
				}
				if (!isok(u.ux + u.dx, u.uy + u.dy)) break;
				mtmp = m_at(u.ux + u.dx, u.uy + u.dy);
				if(mtmp){
					if(!mtmp->mtame){
						maybe_tame(mtmp, sobj);
					} else if(EDOG(mtmp)->friend){
						mtmp->mtame += ACURR(A_CHA)*10;
						if(mtmp->mpeacetime) mtmp->mpeacetime += ACURR(A_CHA);
					} else if(mtmp->mpeacetime) mtmp->mpeacetime += ACURR(A_CHA);
					if(mtmp->mtame && mtmp->mtame < ACURR(A_CHA))
						mtmp->mtame = ACURR(A_CHA);
				}
			}
		}
		break;
	case SCR_GENOCIDE:
		You("have found a scroll of genocide!");
		known = TRUE;
		if (sobj->blessed) do_class_genocide();
		else do_genocide((!sobj->cursed) | (2 * !!Confusion));
		break;
	case SCR_LIGHT:
		if(!Blind) known = TRUE;
		litroom(!confused && !sobj->cursed, sobj);
		if(!confused && !sobj->cursed && u.sealsActive&SEAL_TENEBROUS) unbind(SEAL_TENEBROUS,TRUE);
		break;
	case SCR_TELEPORTATION:
		if(confused || sobj->cursed) level_tele();
		else {
			if (sobj->blessed && !Teleport_control) {
				known = TRUE;
				if (yn("Do you wish to teleport?")=='n')
					break;
			}
			tele();
			if(Teleport_control || !couldsee(u.ux0, u.uy0) ||
			   (distu(u.ux0, u.uy0) >= 16))
				known = TRUE;
		}
		break;
	case SCR_GOLD_DETECTION:
		if (confused || sobj->cursed) return(trap_detect(sobj));
		else return(gold_detect(sobj));
	case SCR_FOOD_DETECTION:
	case SPE_DETECT_FOOD:
		if (food_detect(sobj))
			return(1);	/* nothing detected */
		break;
	case SPE_IDENTIFY:
		cval = rn2(5);
		goto id;
	case SCR_IDENTIFY:
		/* known = TRUE; */
		if(confused)
			You("identify this as an identify scroll.");
		else
			pline("This is an identify scroll.");
		if (sobj->blessed || (!sobj->cursed && !rn2(5))) {
			cval = rn2(5);
			/* Note: if rn2(5)==0, identify all items */
			if (cval == 1 && sobj->blessed && Luck > 0) ++cval;
		} else	cval = 1;
		if(!objects[sobj->otyp].oc_name_known) more_experienced(0,10);
		useup(sobj);
		makeknown(SCR_IDENTIFY);
	id:
		if(invent && !confused) {
		    identify_pack(cval);
		}
		return(1);
	case SCR_CHARGING:
		if (confused) {
		    You_feel("charged up!");
		    if (u.uen < u.uenmax){
				u.uen = min(u.uen+400, u.uenmax);
			} else {
				u.uenbonus += d(5,4);
				calc_total_maxen();
				u.uen = u.uenmax;
			}
		    flags.botl = 1;
		    break;
		}
		known = TRUE;
		pline("This is a charging scroll.");
		otmp = getobj(all_count, "charge");
		if (!otmp) break;
		recharge(otmp, sobj->cursed ? -1 : (sobj->blessed ? 1 : 0));
		break;
	case SCR_MAGIC_MAPPING:
		if (level.flags.nommap) {
		    Your("mind is filled with crazy lines!");
		    if (Hallucination)
			pline("Wow!  Modern art.");
		    else
			Your("%s spins in bewilderment.", body_part(HEAD));
		    make_confused(HConfusion + rnd(30), FALSE);
		    break;
		}
		if (sobj->blessed && !(sobj->oartifact)) {
		    register int x, y;

		    for (x = 1; x < COLNO; x++)
		    	for (y = 0; y < ROWNO; y++)
		    	    if (levl[x][y].typ == SDOOR)
		    	    	cvt_sdoor_to_door(&levl[x][y]);
		    /* do_mapping() already reveals secret passages */
		}
		if((sobj->blessed || sobj->oartifact) && Is_paradise(&u.uz)){
			struct obj* obj;
			for (obj = level.buriedobjlist; obj; obj = obj->nobj) {
				if(obj->otyp == CHEST || obj->otyp == MAGIC_LAMP){
					int x = obj->ox, y = obj->oy;
					extra = TRUE;
					make_engr_at(x,y,"X",moves,BURN);
					if(isok(x+1,y+1)) make_engr_at(x+1,y+1,"\\",moves,BURN);
					if(isok(x+1,y-1)) make_engr_at(x+1,y-1,"/",moves,BURN);
					if(isok(x-1,y+1)) make_engr_at(x-1,y+1,"/",moves,BURN);
					if(isok(x-1,y-1)) make_engr_at(x-1,y-1,"\\",moves,BURN);
				} else if(obj->otyp == FIGURINE){
					int x = obj->ox, y = obj->oy;
					make_engr_at(x,y,"x",moves,BURN);
				}
			}
		}
		known = TRUE;
	case SPE_MAGIC_MAPPING:
		if (level.flags.nommap) {
		    Your("%s spins as %s blocks the spell!", body_part(HEAD), something);
		    make_confused(HConfusion + rnd(30), FALSE);
		    break;
		}
		if(!(sobj->oartifact)){
		pline("A map coalesces in your mind!");
		if(extra) pline("X marks the spot!");
		cval = (sobj->cursed && !confused);
		if(cval) HConfusion = 1;	/* to screw up map */
		do_mapping();
		if(cval) {
		    HConfusion = 0;		/* restore */
		    pline("Unfortunately, you can't grasp the details.");
		}
		}
		else{
			if(sobj->age > monstermoves){
				pline("The map %s hard to see.", vtense((char *)0,"are"));
				nomul(-1*rnd(3), "studying a magic map");
				sobj->age += (long) d(3,10);
			} else sobj->age = monstermoves + (long) d(3,10);
			do_vicinity_map(u.ux,u.uy);
		}
		break;
	case SCR_AMNESIA:
		known = TRUE;
		forget(sobj->cursed ? 25 : sobj->blessed ? 0 : 10);
		if (Hallucination) /* Ommmmmm! */
			Your("mind releases itself from mundane concerns.");
		else if (!strncmpi(plname, "Maud", 4))
			pline("As your mind turns inward on itself, you forget everything else.");
		else if (rn2(2))
			pline("Who was that Maud person anyway?");
		else
			pline("Thinking of Maud you forget everything else.");
		/* Blessed amnesia makes you forget lycanthropy, sickness */
		if (sobj->blessed) {
			if (u.ulycn >= LOW_PM && !Race_if(PM_HUMAN_WEREWOLF)) {
				You("forget your affinity to %s!",
						makeplural(mons[u.ulycn].mname));
				if (youmonst.data->mtyp == u.ulycn)
					you_unwere(FALSE);
				u.ulycn = NON_PM;	/* cure lycanthropy */
			}
			make_sick(0L, (char *) 0, TRUE, SICK_ALL);

			/* You feel refreshed */
			if(Race_if(PM_INCANTIFIER)) u.uen += 50 + rnd(50);
			else u.uhunger += 50 + rnd(50);
			
			newuhs(FALSE);
		} else {
			if(Role_if(PM_MADMAN)){
				You_feel("ashamed of wiping your own memory.");
				change_hod(sobj->cursed ? 5 : 2);
			}
			exercise(A_WIS, FALSE);
		}
		break;
	case SCR_FIRE:{
		/*
		 * Note: Modifications have been made as of 3.0 to allow for
		 * some damage under all potential cases.
		 */
		int damlevel = max(3, u.ulevel);
		cval = bcsign(sobj);
		if(Fire_resistance) cval *= -1;//if you resist fire, blessed does more damage than cursed
		if(!objects[sobj->otyp].oc_name_known) more_experienced(0,10);
		useup(sobj);
		makeknown(SCR_FIRE);
		if(confused) {
		    if(Fire_resistance) {
			shieldeff(u.ux, u.uy);
			if(!Blind)
			    pline("Oh, look, what a pretty fire in your %s.",
				makeplural(body_part(HAND)));
			else You_feel("a pleasant warmth in your %s.",
				makeplural(body_part(HAND)));
		    } else {
			pline_The("scroll catches fire and you burn your %s.",
				makeplural(body_part(HAND)));
			losehp(1, "scroll of fire", KILLED_BY_AN);
		    }
		    return(1);
		}
		if (Underwater)
			pline_The("water around you vaporizes violently!");
		else {
		    pline_The("scroll erupts in a tower of flame!");
		    burn_away_slime();
		    melt_frozen_air();
		}
		explode(u.ux, u.uy, AD_FIRE, SCROLL_CLASS, (2*(rn1(damlevel, damlevel) - (damlevel-1) * cval) + 1)/3,
							EXPL_FIERY, 1);
		return(1);
	}
	case SCR_EARTH:
	    /* TODO: handle steeds */
	    if (
#ifdef REINCARNATION
		!Is_rogue_level(&u.uz) && 
#endif
	    	 (!In_endgame(&u.uz) || Is_earthlevel(&u.uz))) {
	    	register int x, y;

	    	/* Identify the scroll */
	    	pline_The("%s rumbles %s you!", ceiling(u.ux,u.uy),
	    			sobj->blessed ? "around" : "above");
	    	known = 1;
	    	if (In_sokoban(&u.uz))
	    	    change_luck(-1);	/* Sokoban guilt */

	    	/* Loop through the surrounding squares */
	    	if (!sobj->cursed) for (x = u.ux-1; x <= u.ux+1; x++) {
	    	    for (y = u.uy-1; y <= u.uy+1; y++) {

	    	    	/* Is this a suitable spot? */
	    	    	if (isok(x, y) && !closed_door(x, y) &&
	    	    			!IS_ROCK(levl[x][y].typ) &&
	    	    			!IS_AIR(levl[x][y].typ) &&
					(x != u.ux || y != u.uy)) {
			    register struct obj *otmp2;
			    register struct monst *mtmp;

	    	    	    /* Make the object(s) */
	    	    	    otmp2 = mksobj(confused ? ROCK : BOULDER, MKOBJ_NOINIT);
	    	    	    if (!otmp2) continue;  /* Shouldn't happen */
	    	    	    otmp2->quan = confused ? rn1(5,2) : 1;
	    	    	    otmp2->owt = weight(otmp2);

	    	    	    /* Find the monster here (won't be player) */
	    	    	    mtmp = m_at(x, y);
	    	    	    if (mtmp && !amorphous(mtmp->data) &&
	    	    	    		!mon_resistance(mtmp,PASSES_WALLS) &&
	    	    	    		!noncorporeal(mtmp->data) &&
	    	    	    		!unsolid(mtmp->data)) {
				struct obj *helmet = which_armor(mtmp, W_ARMH);
				int mdmg;

				if (cansee(mtmp->mx, mtmp->my)) {
				    pline("%s is hit by %s!", Monnam(mtmp),
	    	    	    			doname(otmp2));
				    if (mtmp->minvis && !canspotmon(mtmp))
					map_invisible(mtmp->mx, mtmp->my);
				}
	    	    	    	mdmg = dmgval(otmp2, mtmp, 0, &youmonst) * otmp2->quan;
				if (helmet) {
				    if(is_hard(helmet)) {
					if (canspotmon(mtmp))
					    pline("Fortunately, %s is wearing a hard helmet.", mon_nam(mtmp));
					else if (flags.soundok)
					    You_hear("a clanging sound.");
					if (mdmg > 2) mdmg = 2;
				    } else {
					if (canspotmon(mtmp))
					    pline("%s's %s does not protect %s.",
						Monnam(mtmp), xname(helmet),
						mhim(mtmp));
				    }
				}
	    	    	    	mtmp->mhp -= mdmg;
	    	    	    	if (mtmp->mhp <= 0)
	    	    	    	    xkilled(mtmp, 1);
	    	    	    }
	    	    	    /* Drop the rock/boulder to the floor */
	    	    	    if (!flooreffects(otmp2, x, y, "fall")) {
	    	    	    	place_object(otmp2, x, y);
	    	    	    	stackobj(otmp2);
	    	    	    	newsym(x, y);  /* map the rock */
	    	    	    }
	    	    	}
		    }
		}
		/* Attack the player */
		if (!sobj->blessed) {
		    int dmg;
		    struct obj *otmp2;

		    /* Okay, _you_ write this without repeating the code */
		    otmp2 = mksobj(confused ? ROCK : BOULDER, MKOBJ_NOINIT);
		    if (!otmp2) break;
		    otmp2->quan = confused ? rn1(5,2) : 1;
		    otmp2->owt = weight(otmp2);
		    if (!amorphous(youracedata) &&
				!Passes_walls &&
				!noncorporeal(youracedata) &&
				!unsolid(youracedata)) {
			You("are hit by %s!", doname(otmp2));
			dmg = dmgval(otmp2, &youmonst, 0, &youmonst) * otmp2->quan;
			if (uarmh && !sobj->cursed) {
			    if(is_hard(uarmh)) {
				pline("Fortunately, you are wearing a hard helmet.");
				if (dmg > 2) dmg = 2;
			    } else if (flags.verbose) {
				Your("%s does not protect you.",
						xname(uarmh));
			    }
			}
		    } else
			dmg = 0;
		    /* Must be before the losehp(), for bones files */
		    if (!flooreffects(otmp2, u.ux, u.uy, "fall")) {
			place_object(otmp2, u.ux, u.uy);
			stackobj(otmp2);
			newsym(u.ux, u.uy);
		    }
		    if (dmg) losehp(dmg, "scroll of earth", KILLED_BY_AN);
		}
	    }
	    break;
	case SCR_PUNISHMENT:
		known = TRUE;
		if(confused || sobj->blessed) {
			You_feel("guilty.");
			break;
		}
		punish(sobj);
		break;
	case SCR_WARD:{
		if(confused && !Hallucination){
		register struct monst *mtmp;
			//Scare nearby monsters.
			for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
				if (DEADMONSTER(mtmp)) continue;
				if(distu(mtmp->mx,mtmp->my) <= 2 ) {
					if(sobj->cursed) {
						mtmp->mflee = mtmp->mfrozen = mtmp->msleeping = 0;
						mtmp->mcanmove = 1;
					} else if (! resist(mtmp, sobj->oclass, 0, NOTELL)){
						if (rn2(7))
							monflee(mtmp, rnd(10), TRUE, TRUE);
						else
							monflee(mtmp, rnd(100), TRUE, TRUE);
					}
				}
			}
		}
		struct engr *engrHere = engr_at(u.ux,u.uy);
		if(is_lava(u.ux, u.uy)){
			pline("The lava shifts and flows beneath you.");
	break;
		}
		if (IS_ALTAR(levl[u.ux][u.uy].typ)) {
			pline("The ground around the altar shifts briefly."); 
			altar_wrath(u.ux, u.uy);
	break;
		}
		pline("The %s shifts beneath you,%sengraving %s.", 
			surface(u.ux,u.uy),
			engrHere ? " wiping away the existing engraving and " : " ",
			an(wardDecode[sobj->oward])
		);
		known = TRUE;
		del_engr_ward_at(u.ux,u.uy);
		make_engr_at(u.ux, u.uy,	"", (moves - multi), DUST); /* absense of text =  dust */
			
		engrHere = engr_at(u.ux,u.uy); /*note: make_engr_at does not return the engraving it made, it returns void instead*/
		if(Hallucination){
			//Scribe hallucinatory ward
			engrHere->halu_ward = TRUE;
			engrHere->ward_type = ENGRAVE;
			engrHere->ward_id = randHaluWard();
	break;
		}
		else{
			engrHere->ward_id = sobj->oward;
			if(sobj->cursed){
				if(is_pool(u.ux, u.uy, TRUE)){
					pline("The lines of blood quickly disperse into the water.");
	break;
				}
				engrHere->ward_type = ENGR_BLOOD;
			} else engrHere->ward_type = ENGRAVE;
			engrHere->complete_wards = sobj->blessed ? 
										wardMax[sobj->oward] : 
										get_num_wards_added(engrHere->ward_id,0);
			if( !(u.wardsknown & get_wardID(sobj->oward)) ){
				You("have learned a new warding sign!");
				u.wardsknown |= get_wardID(sobj->oward);
			}
		}
	break;}
	case SCR_WARDING:{
		//Confused: Scribe random ward that you DON'T know.
		//Hallucination: A ripple of %color energy.... Changes all wards to hallucinatory random wards.
		if(sobj->cursed){
			//Cursed: Destroy all wards on the level
			pline("A ripple of black fire spreads out over the %s.", surface(u.ux,u.uy));
			blank_all_wards();
	break;
		}
		else if(Hallucination){
			//Convert all wards to random wards
			pline("A ripple of %s fire spreads out over the %s.", hcolor(0), surface(u.ux,u.uy));
			randomize_all_wards();
	break;
		}
		struct engr *engrHere = engr_at(u.ux,u.uy);
		int wardNum;
		if(!confused) wardNum = pick_ward(FALSE);
		else wardNum = random_unknown_ward();
//		pline("%d",wardNum);
		known = TRUE;
		if(!wardNum) /*Player cancelled prompt*/
	break;
		if(is_lava(u.ux, u.uy)){
			pline("Brilliant fire dances over the lava for a moment.");
	break;
		}
		if (IS_ALTAR(levl[u.ux][u.uy].typ)) {
			pline("Brilliant fire dances over the altar briefly."); 
			altar_wrath(u.ux, u.uy);
	break;
		}
		pline("Brilliant fire plays over the %s, burning a %s ward into it.", 
			surface(u.ux,u.uy),
			wardNum != -1 ? wardDecode[wardNum] : "Cerulean Sign"
		);
		known = TRUE;
		if(!engrHere){
			make_engr_at(u.ux, u.uy,	"", (moves - multi), DUST); /* absense of text =  dust */
			engrHere = engr_at(u.ux,u.uy); /*note: make_engr_at does not return the engraving it made, it returns void instead*/
		}
		
		if(wardNum == -1){ /*Confused reading of scroll with no unknown wards remaining. Engrave Cerulean Sign, which can't be learned*/
			engrHere->halu_ward = TRUE;
			engrHere->ward_id = CERULEAN_SIGN;
			engrHere->ward_type = BURN;
			engrHere->complete_wards = 1;
		}
		else if(engrHere->ward_id != wardNum 
				|| engrHere->ward_type != BURN){
			engrHere->ward_id = wardNum;
			engrHere->ward_type = BURN;
			engrHere->complete_wards = sobj->blessed ? 
										wardMax[wardNum] : 
										get_num_wards_added(engrHere->ward_id,0);
		}
		else{
			engrHere->complete_wards = sobj->blessed ? 
										wardMax[wardNum] : 
										engrHere->complete_wards + get_num_wards_added(engrHere->ward_id, engrHere->complete_wards);
		}
	break;}
	case SCR_STINKING_CLOUD: {
	        coord cc;

		You("have found a scroll of stinking cloud!");
		known = TRUE;
		pline("Where do you want to center the cloud?");
		cc.x = u.ux;
		cc.y = u.uy;
		if (getpos(&cc, TRUE, "the desired position") < 0) {
		    pline1(Never_mind);
		    return 0;
		}
		if (!cansee(cc.x, cc.y) || distu(cc.x, cc.y) >= 32) {
		    You("smell rotten eggs.");
		    return 0;
		}
		if(confused){
			struct region_arg cloud_data;
			cloud_data.damage = 8+4*bcsign(sobj);
			cloud_data.adtyp = random_cloud_types[rn2(SIZE(random_cloud_types))];
			(void) create_generic_cloud(cc.x, cc.y, 3+bcsign(sobj), &cloud_data, TRUE);
		} else {
			(void) create_gas_cloud(cc.x, cc.y, 3+bcsign(sobj), 8+4*bcsign(sobj), TRUE);
		}
		break;
	}
	case SCR_ANTIMAGIC:{
		int amt = 400;
		if(confused && sobj->cursed){
			//Confused
			pline("Shimmering sparks shoot into your body!");
			if(u.uen + 400 > u.uenmax){
				u.uenbonus += 4;
				calc_total_maxen();
				u.uen = u.uenmax;
			} else u.uen += 400;
	break;
		}
		if(sobj->cursed){
			//Cursed: Attacks YOUR magic
			pline("Shimmering sparks shoot from your body!");
			losepw(400);
	break;
		}
		if(confused){
			//Confused: Remove magic shield
			if(HNullmagic && (HNullmagic & ~TIMEOUT) == 0L)
				pline("The shimmering film around you pops!");
			else pline("Nothing happens.");
			give_intrinsic(NULLMAGIC, -2);
	break;
		}
		if(!Nullmagic) pline("A shimmering film surrounds you!");
		else pline("The shimmering film grows brighter!");
		give_intrinsic(NULLMAGIC, amt);
	}
	break;
	case SCR_RESISTANCE:{
		if(confused){
			int damlevel = max(3, u.ulevel), sx = u.ux, sy = u.uy;
			cval = bcsign(sobj);
			sx = u.ux+rnd(3)-2; 
			sy = u.uy+rnd(3)-2;
			if (!isok(sx,sy) ||
				IS_STWALL(levl[sx][sy].typ) || u.uswallow) {
				/* Spell is reflected back to center */
				sx = u.ux; 
				sy = u.uy;
			}
			explode(sx, sy, AD_FIRE, SCROLL_CLASS, (2 * (rn1(damlevel, damlevel) - (damlevel - 1) * cval) + 1) / 3,
							EXPL_FIERY, 1);
			sx = u.ux+rnd(3)-2; 
			sy = u.uy+rnd(3)-2;
			if (!isok(sx,sy) ||
				IS_STWALL(levl[sx][sy].typ) || u.uswallow) {
				/* Spell is reflected back to center */
				sx = u.ux; 
				sy = u.uy;
			}
			explode(sx, sy, AD_COLD, SCROLL_CLASS, (2 * (rn1(damlevel, damlevel) - (damlevel - 1) * cval) + 1) / 3,
							EXPL_FROSTY, 1);
			sx = u.ux+rnd(3)-2; 
			sy = u.uy+rnd(3)-2;
			if (!isok(sx,sy) ||
				IS_STWALL(levl[sx][sy].typ) || u.uswallow) {
				/* Spell is reflected back to center */
				sx = u.ux; 
				sy = u.uy;
			}
			explode(sx, sy, AD_ELEC, SCROLL_CLASS, (2 * (rn1(damlevel, damlevel) - (damlevel - 1) * cval) + 1) / 3,
							EXPL_MAGICAL, 1);
			sx = u.ux+rnd(3)-2; 
			sy = u.uy+rnd(3)-2;
			if (!isok(sx,sy) ||
				IS_STWALL(levl[sx][sy].typ) || u.uswallow) {
				/* Spell is reflected back to center */
				sx = u.ux; 
				sy = u.uy;
			}
			explode(sx, sy, AD_ACID, SCROLL_CLASS, (2 * (rn1(damlevel, damlevel) - (damlevel - 1) * cval) + 1) / 3,
							EXPL_NOXIOUS, 1);
	break;
		}
		long rturns = sobj->blessed ? 5000L : sobj->cursed ? 5L : 250L;
		
		if (!(HFire_resistance))
			You(Hallucination ? "be chillin'." : "feel a momentary chill.");
		give_intrinsic(FIRE_RES, rturns);
		
		if (!(HSleep_resistance))
			You_feel("wide awake.");
		give_intrinsic(SLEEP_RES, rturns);
		
		if (!(HCold_resistance))
			You_feel("full of hot air.");
		give_intrinsic(COLD_RES, rturns);
		
		
		if (!(HShock_resistance)) {
			if (Hallucination)
				rn2(2) ? You_feel("grounded in reality.") : Your("health currently feels amplified!");
			else
				You_feel("well grounded.");
		}
		give_intrinsic(SHOCK_RES, rturns);
		
		if (!(HAcid_resistance)) {
			if (Hallucination)
				rn2(2) ? You_feel("like you've really gotten back to basics!") : You_feel("insoluble.");
			else
				Your("skin feels leathery.");
		}
		give_intrinsic(ACID_RES, rturns);
		
	}
	break;
	case SCR_CONSECRATION:
	if (confused) {
		/* consecrates your weapon */
		/* NOT valid_weapon(), which also allows non-enchantable things that are effective to hit with */
		if (uwep && (uwep->oclass == WEAPON_CLASS || is_weptool(uwep) || is_gloves(uwep))) {
			if (sobj->cursed){
				curse(uwep);
				if (!check_oprop(uwep, OPROP_UNHYW))
					add_oprop(uwep, OPROP_LESSER_UNHYW);
				if(uwep->spe < 3)
					uwep->spe = 3;
			} else { // blessed/uncursed
				bless(uwep);
				if (!check_oprop(uwep, OPROP_HOLYW))
					add_oprop(uwep, OPROP_LESSER_HOLYW);
				if(uwep->spe < 3)
					uwep->spe = 3;
			} 
		}
		else {
			goto returnscroll;
		}
	}
	else if(In_endgame(&u.uz)){
		if(Is_astralevel(&u.uz)) pline("This place is already pretty consecrated.");
		else pline("It would seem base matter alone cannot be consecrated.");
		goto returnscroll;
	} else if(Is_sanctum(&u.uz)){
		pline("This place is much too unholy for the scroll to work.");
		goto returnscroll;
	} else {
		aligntyp whichgod;
		if(sobj->cursed || In_hell(&u.uz)){
			whichgod = A_NONE;
		} else whichgod = u.ualign.type;
		
		if (levl[u.ux][u.uy].typ == CORR ||
			levl[u.ux][u.uy].typ == ROOM ||
			levl[u.ux][u.uy].typ == GRASS ||
			levl[u.ux][u.uy].typ == SOIL ||
			levl[u.ux][u.uy].typ == SAND)
		{
			int godnum = GOD_NONE;
			if(philosophy_index(align_to_god(whichgod))){
				//Undead slayer used a second Egyptian pantheon for some reason.
				// Just go ahead and use the Egyptian one as a reference.
				if(whichgod == A_LAWFUL)
					godnum = GOD_PTAH;
				else if(whichgod == A_NEUTRAL)
					godnum = GOD_THOTH;
				else if(whichgod == A_CHAOTIC)
					godnum = GOD_ANHUR;
			}
			add_altar(u.ux, u.uy, whichgod, FALSE, godnum);
			pline("%s altar appears in front of you!", An(align_str(whichgod)));
			newsym(u.ux, u.uy);
		}
		else {
			pline1(nothing_happens);
			/* this is a fairly unique scroll; don't waste it */
		returnscroll:
			pline("The scroll reappears in your pack!");
			sobj->quan++;	/* don't let useup get it */
		}
	}break;
	case SCR_GOLD_SCROLL_OF_LAW: {
		register struct monst *mtmp;
		aligntyp mal, ual = u.ualign.type;
	    int i, j, bd = confused ? 7 : 1;
		You("read from the golden scroll.");
		if(u.ualign.type == A_LAWFUL && u.ualign.record > 20){
		 pline("It is a log-book from the Eternal Library.");
		 pline("It's meaning is clear in your mind, and the pronunciation obvious.");
		 known = TRUE; //id the scroll
		 more_experienced(777,0);//the knowledge from the scroll transfers to you.
		 newexplevel();
		 if (Upolyd) {// the lawful energies rebuild your body
			u.mh += u.ualign.record;
			if(u.mhmax < u.mh)
				u.mh = u.mhmax;
		 }
		 else{
			u.uhp += u.ualign.record;
			if(u.uhp > u.uhpmax)
				u.uhp = u.uhpmax;
		 }
		 if (u.uswallow) {
		    if(u.ustuck->data->maligntyp > A_NEUTRAL) maybe_tame(u.ustuck, sobj);
			else if(u.ustuck->data->maligntyp == A_NEUTRAL) monflee(u.ustuck, 7, FALSE, FALSE);
			else monflee(u.ustuck, 77, FALSE, FALSE);
	break;
		 }//else:
	     for (i = -bd; i <= bd; i++) for(j = -bd; j <= bd; j++) {
			if (!isok(u.ux + i, u.uy + j)) continue;
			if ((mtmp = m_at(u.ux + i, u.uy + j)) == 0) continue;
			if (DEADMONSTER(mtmp)) continue;
			if(confused || sobj->cursed) {
			    mtmp->mflee = mtmp->mfrozen = mtmp->msleeping = 0;
			    mtmp->mcanmove = 1;
			} else{
				mal = mtmp->data->maligntyp;
				if(mal < A_NEUTRAL && !resist(mtmp, sobj->oclass, 0, NOTELL)){
					monflee(mtmp, 77, FALSE, FALSE);
				}
				else if(mal == A_NEUTRAL && !resist(mtmp, sobj->oclass, 0, NOTELL)){
					monflee(mtmp, 7, FALSE, FALSE);
				}
				else if(mal > A_NEUTRAL){//maybe_tame includes a resistence check.
				    maybe_tame(mtmp, sobj);
				}
			}
	     }
		 You_feel("guilty for destroying such a valuable text.");
		 u.ualign.record /= 2;
		 u.ualign.sins += 4;
		}
		else if(u.ualign.type == A_NEUTRAL || (u.ualign.type == A_LAWFUL && u.ualign.record > 0)){
		 pline("It's meaning is unclear, and you think you didn't pronounce it quite right.");
		 if (u.uswallow) {
			if(is_chaotic_mon(u.ustuck)) monflee(u.ustuck, 7, FALSE, FALSE);
	break;
		 }//else:
	     for (i = -bd; i <= bd; i++) for(j = -bd; j <= bd; j++) {
			if (!isok(u.ux + i, u.uy + j)) continue;
			if ((mtmp = m_at(u.ux + i, u.uy + j)) == 0) continue;
			if (DEADMONSTER(mtmp)) continue;
			if(confused || sobj->cursed) {
			    mtmp->mflee = mtmp->mfrozen = mtmp->msleeping = 0;
			    mtmp->mcanmove = 1;
			} else{
				mal = mtmp->data->maligntyp;
				if(mal < A_NEUTRAL && !resist(mtmp, sobj->oclass, 0, NOTELL)){
					monflee(mtmp, 7, FALSE, FALSE);
				}
			}
	     }
		 if(u.ualign.type == A_NEUTRAL) {
			You_feel("less neutral.");
			u.ualign.record -= 8;
			u.ualign.sins += 8;
		 }
		 else{
			You_feel("guilty for destroying such a valuable text.");
			u.ualign.record -= 20;
			u.ualign.sins += 10;
		 }
		}
		else{//chaotic or poorly aligned lawful
		 You("find it quite confusing.");
		 incr_itimeout(&HConfusion, max(u.ualign.record, -1*u.ualign.record));
		 if(u.ualign.type == A_LAWFUL){
			You_feel("unworthy of such a lawful text.");
			u.ualign.record -= 8;
			u.ualign.sins += 8;
		 }
		}
	}
	break;
	default:
		impossible("What weird effect is this? (%u)", sobj->otyp);
	}
	return(0);
}

static void
wand_explode(obj)
register struct obj *obj;
{
    obj->in_use = TRUE;	/* in case losehp() is fatal */
    Your("%s vibrates violently, and explodes!",xname(obj));
    nhbell();
    losehp(rnd(2*((Upolyd ? u.mhmax : u.uhpmax)+1)/3), "exploding wand", KILLED_BY_AN);
    useup(obj);
    exercise(A_STR, FALSE);
}

/*
 * Low-level lit-field update routine.
 */
void
set_lit(x,y,val)
int x, y;
genericptr_t val;
{
	if (val)
	    levl[x][y].lit = 1;
	else {
	    levl[x][y].lit = 0;
	    snuff_light_source(x, y);
	}
}

// lights/snuffs simple lightsources lying at the location, lighting assumes this is triggered by the player for shk purposes
// this could also be added to set_lit, I suppose
void
ranged_set_lightsources(x,y,val)
int x, y;
genericptr_t val;
{
	if (val) {
		struct obj *ispe = mksobj(SPE_LIGHT, NO_MKOBJ_FLAGS);
		bhitpile(ispe, bhito, x, y);
	}
	else {
		snuff_light_source(x, y);
	}
}

void
litroom(on,obj)
register boolean on;
struct obj *obj;
{
	char is_lit;	/* value is irrelevant; we use its address
			   as a `not null' flag for set_lit() */
	boolean permanent_darkness = Is_grue_level(&u.uz);
	/* first produce the text (provided you're not blind) */
		register struct obj *otmp;
	if(!on) {
		if (!Blind) {
		    if(u.uswallow) {
			pline("It seems even darker in here than before.");
			return;
		    }
		    if (uwep && artifact_light(uwep) && uwep->lamplit)
			pline("Suddenly, the only light left comes from %s!",
				the(xname(uwep)));
		    else
			You("are surrounded by darkness!");
		}

		/* the magic douses lamps, et al, too */
		for(otmp = invent; otmp; otmp = otmp->nobj)
			if (otmp->lamplit && otmp->otyp != SUNROD && !Darkness_cant_snuff(otmp))
				(void) snuff_lit(otmp);
		if (Blind) goto do_it;
	} else {
		if (Blind) goto do_it;
		if(u.uswallow){
			if (is_animal(u.ustuck->data))
				pline("%s %s is lit.",
				        s_suffix(Monnam(u.ustuck)),
					mbodypart(u.ustuck, STOMACH));
			else
				if (is_whirly(u.ustuck->data))
					pline("%s shines briefly.",
					      Monnam(u.ustuck));
				else
					pline("%s glistens.", Monnam(u.ustuck));
			return;
		}
		for(otmp = invent; otmp; otmp = otmp->nobj)
		    if (otmp->otyp == SHADOWLANDER_S_TORCH && otmp->lamplit)
				end_burn(otmp, TRUE);
		
		if (!permanent_darkness)
			pline("A lit field surrounds you!");
		else
			pline("A ripple of energy travels through the darkness.");
	}

do_it:
	/* No-op in water - can only see the adjacent squares and that's it! */
	if (Underwater || Is_waterlevel(&u.uz)) return;
	/*
	 *  If we are darkening the room and the hero is punished but not
	 *  blind, then we have to pick up and replace the ball and chain so
	 *  that we don't remember them if they are out of sight.
	 */
	if (Punished && (Darksight ? on : !on) && !Blind)
	    move_bc(1, 0, uball->ox, uball->oy, uchain->ox, uchain->oy);

#ifdef REINCARNATION
	if (Is_rogue_level(&u.uz)) {
	    /* Can't use do_clear_area because MAX_RADIUS is too small */
	    /* rogue lighting must light the entire room */
	    int rnum = levl[u.ux][u.uy].roomno - ROOMOFFSET;
	    int rx, ry;
	    if(rnum >= 0) {
		for(rx = rooms[rnum].lx-1; rx <= rooms[rnum].hx+1; rx++)
		    for(ry = rooms[rnum].ly-1; ry <= rooms[rnum].hy+1; ry++)
			set_lit(rx, ry,
				(genericptr_t)(on ? &is_lit : (char *)0));
		rooms[rnum].rlit = on;
	    }
	    /* hallways remain dark on the rogue level */
	} else
#endif
	    do_clear_area(u.ux,u.uy,
		((obj && obj->oclass==SCROLL_CLASS && obj->blessed) ? 9 : 5) * (permanent_darkness ? 3 : 2) / 2, permanent_darkness ? ranged_set_lightsources :
		set_lit, (genericptr_t)(on ? &is_lit : (char *)0));

	/*
	 *  If we are not blind, then force a redraw on all positions in sight
	 *  by temporarily blinding the hero.  The vision recalculation will
	 *  correctly update all previously seen positions *and* correctly
	 *  set the waslit bit [could be messed up from above].
	 */
	if (!Blind) {
	    vision_recalc(2);

	    /* replace ball&chain */
	    if (Punished && (Darksight ? on : !on))
			move_bc(0, 0, uball->ox, uball->oy, uchain->ox, uchain->oy);
	}

	vision_full_recalc = 1;	/* delayed vision recalculation */
}

int
wiz_kill_all()
{
	register struct monst *mtmp, *mtmp2;

	register int gonecnt = 0;
	for (mtmp = fmon; mtmp; mtmp = mtmp2) {
		mtmp2 = mtmp->nmon;
		if (DEADMONSTER(mtmp)) continue;
		mongone(mtmp);
		gonecnt++;
	}
	pline("Eliminated %d monster%s.", gonecnt, plur(gonecnt));
	return 0;	/* takes no time, unless caller takes time */
}

static void
do_class_genocide()
{
	int i, j, immunecnt, gonecnt, goodcnt, class, feel_dead = 0;
	char buf[BUFSZ];
	boolean gameover = FALSE;	/* true iff killed self */

	for(j=0; ; j++) {
		if (j >= 5) {
			pline1(thats_enough_tries);
			return;
		}
		do {
		    getlin("What class of monsters do you wish to genocide?",
			buf);
		    (void)mungspaces(buf);
		} while (buf[0]=='\033' || !buf[0]);
		/* choosing "none" preserves genocideless conduct */
		if (!strcmpi(buf, "none") ||
		    !strcmpi(buf, "nothing")) return;

		if (strlen(buf) == 1) {
		    if (buf[0] == ILLOBJ_SYM)
			buf[0] = def_monsyms[S_MIMIC];
		    class = def_char_to_monclass(buf[0]);
		} else {
		    char buf2[BUFSZ];

		    class = 0;
		    Strcpy(buf2, makesingular(buf));
		    Strcpy(buf, buf2);
		}
		immunecnt = gonecnt = goodcnt = 0;
		for (i = LOW_PM; i < NUMMONS; i++) {
		    if (class == 0 &&
			    strstri(monexplain[(int)mons[i].mlet], buf) != 0)
			class = mons[i].mlet;
		    if (mons[i].mlet == class) {
			if (!(mons[i].geno & G_GENO)) immunecnt++;
			else if(mvitals[i].mvflags & G_GENOD) gonecnt++;
			else goodcnt++;
		    }
		}
		/*
		 * TODO[?]: If user's input doesn't match any class
		 *	    description, check individual species names.
		 */
		if (!goodcnt && class != mons[urole.malenum].mlet &&
				class != mons[urace.malenum].mlet) {
			if (gonecnt)
	pline("All such monsters are already nonexistent.");
			else if (immunecnt ||
				(buf[0] == DEF_INVISIBLE && buf[1] == '\0'))
	You("aren't permitted to genocide such monsters.");
			else
#ifdef WIZARD	/* to aid in topology testing; remove pesky monsters */
			if (wizard && buf[0] == '*') {
				(void)wiz_kill_all();
			return;
			} else
#endif
	pline("That symbol does not represent any monster.");
			continue;
		}

		for (i = LOW_PM; i < NUMMONS; i++) {
		    if(mons[i].mlet == class) {
			char nam[BUFSZ];

			Strcpy(nam, makeplural(mons[i].mname));
			/* Although "genus" is Latin for race, the hero benefits
			 * from both race and role; thus genocide affects either.
			 */
			if (Your_Own_Role(i) || Your_Own_Race(i) ||
				((mons[i].geno & G_GENO)
				&& !(mvitals[i].mvflags & G_GENOD))) {
			/* This check must be first since player monsters might
			 * have G_GENOD or !G_GENO.
			 */
			    mvitals[i].mvflags |= (G_GENOD|G_NOCORPSE);
			    reset_rndmonst(i);
			    kill_genocided_monsters();
			    update_inventory();		/* eggs & tins */
			    pline("Wiped out all %s.", nam);
				if(u.sealsActive&SEAL_MOTHER) unbind(SEAL_MOTHER,TRUE);
			    if (Upolyd && i == u.umonnum) {
				u.mh = -1;
				if (Unchanging) {
				    if (!feel_dead++) You("die.");
				    /* finish genociding this class of
				       monsters before ultimately dying */
				    gameover = TRUE;
				} else {
				    rehumanize();
					change_gevurah(1); //cheated death.
				}
			    }
			    /* Self-genocide if it matches either your race
			       or role.  Assumption:  male and female forms
			       share same monster class. */
			    if (i == urole.malenum || i == urace.malenum) {
				u.uhp = -1;
				if (Upolyd) {
				    if (!feel_dead++){
						You_feel("dead inside.");
						change_gevurah(20); //cheated death.
					}
				} else {
				    if (!feel_dead++) You("die.");
				    gameover = TRUE;
				}
			    }
			} else if (mvitals[i].mvflags & G_GENOD) {
			    if (!gameover)
				pline("All %s are already nonexistent.", nam);
			} else if (!gameover) {
			  /* suppress feedback about quest beings except
			     for those applicable to our own role */
			  if ((mons[i].msound != MS_LEADER ||
			       quest_info(MS_LEADER) == i)
			   && (mons[i].msound != MS_NEMESIS ||
			       quest_info(MS_NEMESIS) == i)
			   && (i < PM_STUDENT ||
			       quest_info(MS_GUARDIAN) == i)
			/* non-leader/nemesis/guardian role-specific monster */
			   && (i != PM_NINJA ||		/* nuisance */
			       Role_if(PM_SAMURAI))) {
				boolean named, uniq;

				named = type_is_pname(&mons[i]) ? TRUE : FALSE;
				uniq = (mons[i].geno & G_UNIQ) ? TRUE : FALSE;
				/* one special case */
				if (i == PM_HIGH_PRIEST) uniq = FALSE;

				You("aren't permitted to genocide %s%s.",
				    (uniq && !named) ? "the " : "",
				    (uniq || named) ? mons[i].mname : nam);
			    }
			}
		    }
		}
		if (gameover || u.uhp == -1) {
		    killer_format = KILLED_BY_AN;
		    killer = "scroll of genocide";
		    if (gameover) done(GENOCIDED);
		}
		return;
	}
}

#define REALLY 1
#define PLAYER 2
#define ONTHRONE 4
void
do_genocide(how)
int how;
/* 0 = no genocide; create monsters (cursed scroll) */
/* 1 = normal genocide */
/* 3 = forced genocide of player */
/* 5 (4 | 1) = normal genocide from throne */
{
	char buf[BUFSZ];
	register int	i, killplayer = 0;
	register int mndx;
	register struct permonst *ptr;
	const char *which;

	if (how & PLAYER) {
		mndx = u.umonster;	/* non-polymorphed mon num */
		ptr = &mons[mndx];
		Strcpy(buf, ptr->mname);
		killplayer++;
	} else {
	    for(i = 0; ; i++) {
		if(i >= 5) {
		    pline1(thats_enough_tries);
		    return;
		}
		getlin("What monster do you want to genocide? [type the name]",
			buf);
		(void)mungspaces(buf);
		/* choosing "none" preserves genocideless conduct */
		if (!strcmpi(buf, "none") || !strcmpi(buf, "nothing")) {
		    /* ... but no free pass if cursed */
		    if (!(how & REALLY)) {
			ptr = rndmonst(0, 0);
			if (!ptr) return; /* no message, like normal case */
			mndx = monsndx(ptr);
			break;		/* remaining checks don't apply */
		    } else return;
		}

		mndx = name_to_mon(buf);
		if (mndx == NON_PM || (mvitals[mndx].mvflags & G_GENOD)) {
			pline("Such creatures %s exist in this world.",
			      (mndx == NON_PM) ? "do not" : "no longer");
			continue;
		}
		ptr = &mons[mndx];
		/* Although "genus" is Latin for race, the hero benefits
		 * from both race and role; thus genocide affects either.
		 */
		if (Your_Own_Role(mndx) || Your_Own_Race(mndx)) {
			killplayer++;
			break;
		}
		if (is_human(ptr)) adjalign(-sgn(u.ualign.type));
		if (is_demon(ptr)) adjalign(sgn(u.ualign.type));

		if(!(ptr->geno & G_GENO)) {
			if(flags.soundok) {
	/* fixme: unconditional "caverns" will be silly in some circumstances */
			    if(flags.verbose)
			pline("A thunderous voice booms through the caverns:");
			    verbalize("No, mortal!  That will not be done.");
			}
			continue;
		}
		/* KMH -- Unchanging prevents rehumanization */
		if (Unchanging && ptr == youmonst.data)
		    killplayer++;
		break;
	    }
	}

	which = "all ";
	if (Hallucination) {
	    if (Upolyd)
		Strcpy(buf,youmonst.data->mname);
	    else {
		Strcpy(buf, (flags.female && urole.name.f) ?
				urole.name.f : urole.name.m);
		buf[0] = lowc(buf[0]);
	    }
	} else {
	    Strcpy(buf, ptr->mname); /* make sure we have standard singular */
	    if ((ptr->geno & G_UNIQ) && ptr->mtyp != PM_HIGH_PRIEST)
		which = !type_is_pname(ptr) ? "the " : "";
	}
	if (how & REALLY) {
	    /* setting no-corpse affects wishing and random tin generation */
	    mvitals[mndx].mvflags |= (G_GENOD | G_NOCORPSE);
	    pline("Wiped out %s%s.", which,
		  (*which != 'a') ? buf : makeplural(buf));
		if(u.sealsActive&SEAL_MOTHER) unbind(SEAL_MOTHER,TRUE);

	    if (killplayer) {
		/* might need to wipe out dual role */
		if (urole.femalenum != NON_PM && mndx == urole.malenum)
		    mvitals[urole.femalenum].mvflags |= (G_GENOD | G_NOCORPSE);
		if (urole.femalenum != NON_PM && mndx == urole.femalenum)
		    mvitals[urole.malenum].mvflags |= (G_GENOD | G_NOCORPSE);
		if (urace.femalenum != NON_PM && mndx == urace.malenum)
		    mvitals[urace.femalenum].mvflags |= (G_GENOD | G_NOCORPSE);
		if (urace.femalenum != NON_PM && mndx == urace.femalenum)
		    mvitals[urace.malenum].mvflags |= (G_GENOD | G_NOCORPSE);

		u.uhp = -1;
		if (how & PLAYER) {
		    killer_format = KILLED_BY;
		    killer = "genocidal confusion";
		} else if (how & ONTHRONE) {
		    /* player selected while on a throne */
		    killer_format = KILLED_BY_AN;
		    killer = "imperious order";
		} else { /* selected player deliberately, not confused */
		    killer_format = KILLED_BY_AN;
		    killer = "scroll of genocide";
		}

	/* Polymorphed characters will die as soon as they're rehumanized. */
	/* KMH -- Unchanging prevents rehumanization */
		if (Upolyd && ptr != youmonst.data) {
			delayed_killer = killer;
			killer = 0;
			You_feel("dead inside.");
			change_gevurah(20); //cheated death.
		} else
			done(GENOCIDED);
	    } else if (ptr == youmonst.data) {
			rehumanize();
			change_gevurah(1); //cheated death.
	    }
	    reset_rndmonst(mndx);
	    kill_genocided_monsters();
	    update_inventory();	/* in case identified eggs were affected */
	} else {
	    int cnt = 0;

	    if (!(mons[mndx].geno & G_UNIQ) &&
		    !(mvitals[mndx].mvflags & G_GONE && !In_quest(&u.uz)))
		for (i = rn1(3, 4); i > 0; i--) {
		    if (!makemon(ptr, u.ux, u.uy, NO_MINVENT))
			break;	/* couldn't make one */
		    ++cnt;
		    if (mvitals[mndx].mvflags & G_EXTINCT && !In_quest(&u.uz))
			break;	/* just made last one */
		}
	    if (cnt)
		pline("Sent in some %s.", makeplural(buf));
	    else
		pline1(nothing_happens);
	}
}

void
punish(sobj)
register struct obj	*sobj;
{
#ifdef CONVICT
    struct obj *otmp;
#endif /* CONVICT */
	boolean cursed = (sobj && sobj->cursed);
	/* KMH -- Punishment is still okay when you are riding */
	if(u.sealsActive&SEAL_MALPHAS){
		You("avoid punishment for your misbehavior!");
		return;
	}
	You("are being punished for your misbehavior!");
	if(Punished){
		if(uball->owt < 1600){
			Your("iron ball gets heavier.");
			uball->owt += 160 * (1 + cursed);
		} else {
			Your("iron ball can grow no heavier.");
		}
		return;
	}
	if (amorphous(youracedata) || is_whirly(youracedata) || unsolid(youracedata)) {
		pline("A ball and chain appears, then falls away.");
		dropy(mkobj(BALL_CLASS, TRUE));
		return;
	}
	setworn(mkobj(CHAIN_CLASS, TRUE), W_CHAIN);
#ifdef CONVICT
    if (((otmp = carrying(BALL)) != 0) &&(otmp->oartifact ==
     ART_IRON_BALL_OF_LEVITATION)) {
        setworn(otmp, W_BALL);
        Your("%s chains itself to you!", xname(otmp));
    } else {
		setworn(mkobj(BALL_CLASS, TRUE), W_BALL);
    }
#else
	setworn(mkobj(BALL_CLASS, TRUE), W_BALL);
#endif /* CONVICT */
	uball->spe = 1;		/* special ball (see save) */

	/*
	 *  Place ball & chain if not swallowed.  If swallowed, the ball &
	 *  chain variables will be set at the next call to placebc().
	 */
	if (!u.uswallow) {
	    placebc();
	    if (Blind) set_bc(1);	/* set up ball and chain variables */
	    newsym(u.ux,u.uy);		/* see ball&chain if can't see self */
	}
}

void
unpunish()
{	    /* remove the ball and chain */
	struct obj *savechain = uchain;

	obj_extract_self(uchain);
	newsym(uchain->ox,uchain->oy);
	setworn((struct obj *)0, W_CHAIN);
	dealloc_obj(savechain);
	uball->spe = 0;
	setworn((struct obj *)0, W_BALL);
}

/* some creatures have special data structures that only make sense in their
 * normal locations -- if the player tries to create one elsewhere, or to revive
 * one, the disoriented creature becomes a zombie
 */
boolean
cant_create(mtype, revival)
int *mtype;
boolean revival;
{

	/* SHOPKEEPERS can be revived now */
	if (*mtype==PM_GUARD || (*mtype==PM_SHOPKEEPER && !revival)
	     || *mtype==PM_ALIGNED_PRIEST || *mtype==PM_ANGEL) {
		*mtype = PM_ZOMBIE;
		return TRUE;
	} else if (*mtype==PM_LONG_WORM_TAIL) {	/* for create_particular() */
		*mtype = PM_LONG_WORM;
		return TRUE;
	} else if (*mtype==PM_HUNTING_HORROR_TAIL) {	/* for create_particular() */
		*mtype = PM_HUNTING_HORROR;
		return TRUE;
	} else if (*mtype==PM_CHORISTER_TRAIN) {	/* for create_particular() */
		*mtype = PM_CHORISTER_JELLY;
		return TRUE;
	}
	return FALSE;
}

#ifdef WIZARD
/*
 * Make a new monster with the type controlled by the user.
 *
 * Note:  when creating a monster by class letter, specifying the
 * "strange object" (']') symbol produces a random monster rather
 * than a mimic; this behavior quirk is useful so don't "fix" it...
 */
struct monst *
create_particular(x, y, specify_attitude, specify_derivation, allow_multi, ma_require, mg_restrict, gen_restrict, in_buff)\
int x,y;
unsigned long specify_attitude;		// -1 -> true; 0 -> false; >0 -> as given
int specify_derivation;				// -1 -> true; 0 -> false; >0 -> as given
int allow_multi;
unsigned long ma_require;
unsigned long mg_restrict;
int gen_restrict;
char *in_buff;
{
	char buf[BUFSZ], *bufp, *p, *q, monclass = MAXMCLASSES;
	int which, tries, i;
	int undeadtype = 0;
	boolean mad_suicidal = FALSE;
	boolean noequip = FALSE;
	struct permonst *whichpm;
	struct monst *mtmp = (struct monst *)0;
	boolean madeany = FALSE;
	boolean maketame, makeloyal, makepeaceful, makehostile, makesummoned, makemale, makefemale;
	int l = 0;

	tries = 0;
	do {
	    which = urole.malenum;	/* an arbitrary index into mons[] */
	    maketame = makeloyal = makepeaceful = makehostile = makesummoned = makemale = makefemale = FALSE;
		if(in_buff){
			Sprintf(buf, "%s", in_buff);
		}
		else {
			getlin("Create what kind of monster? [type the name or symbol]",
			   buf);
		}
	    bufp = mungspaces(buf);
	    if (*bufp == '\033' || !strncmpi(bufp, "nothing", l = 7) || !strncmpi(bufp, "nil", l = 3)) return (struct monst *)0;

		/* grab all prefixes -- this *requires* that NO monsters have a name overlapping with these prefixes! */
		while (TRUE)
		{
			if (!strncmpi(bufp, "tame ", l = 5)){
				maketame = TRUE && (specify_attitude == -1);
			}
			else if (!strncmpi(bufp, "loyal ", l = 6)){
				maketame = TRUE && (specify_attitude == -1);
				makeloyal = TRUE && (specify_attitude == -1);
			}
			else if (!strncmpi(bufp, "peaceful ", l = 9)){
				makepeaceful = TRUE && (specify_attitude == -1);
			}
			else if (!strncmpi(bufp, "hostile ", l = 8)) {
				makehostile = TRUE && (specify_attitude == -1);
			}
			else if (!strncmpi(bufp, "summoned ", l = 9)) {
				makesummoned = TRUE && (specify_attitude == -1);
			}
			else if (!strncmpi(bufp, "zombified ", l = 10)) {
				undeadtype = ZOMBIFIED;
			}
			else if (!strncmpi(bufp, "skelified ", l = 10) ||
					 !strncmpi(bufp, "skeletal ", l = 9)) {
				undeadtype = SKELIFIED;
			}
			else if (!strncmpi(bufp, "crystalfied ", l = 12)) {
				undeadtype = CRYSTALFIED;
			}
			else if (!strncmpi(bufp, "slimy ", l = 6)) {
				undeadtype = SLIME_REMNANT;
			}
			else if (!strncmpi(bufp, "fulvous ", l = 8)) {
				undeadtype = YELLOW_TEMPLATE;
			}
			else if (!strncmpi(bufp, "swollen ", l = 8)) {
				undeadtype = SWOLLEN_TEMPLATE;
			}
			else if (!strncmpi(bufp, "mad_angel ", l = 10)) {
				undeadtype = MAD_TEMPLATE;
			}
			else if (!strncmpi(bufp, "fractured ", l = 10)) {
				undeadtype = FRACTURED;
			}
			else if (!strncmpi(bufp, "illuminated ", l = 12)) {
				undeadtype = ILLUMINATED;
			}
			else if (!strncmpi(bufp, "vampiric ", l = 9)) {
				undeadtype = VAMPIRIC;
			}
			else if (!strncmpi(bufp, "pseudonatural ", l = 14)) {
				undeadtype = PSEUDONATURAL;
			}
			else if (!strncmpi(bufp, "tomb herd ", l = 10)) {
				undeadtype = TOMB_HERD;
			}
			else if (!strncmpi(bufp, "yith ", l = 5)) {
				undeadtype = YITH;
			}
			else if (!strncmpi(bufp, "cranium ", l = 8)) {
				undeadtype = CRANIUM_RAT;
			}
			else if (!strncmpi(bufp, "psurlon ", l = 8)) {
				undeadtype = PSURLON;
			}
			else if (!strncmpi(bufp, "constellation ", l = 14)) {
				undeadtype = CONSTELLATION;
			}
			else if (!strncmpi(bufp, "mistweaver ", l = 11)) {
				undeadtype = MISTWEAVER;
			}
			else if (!strncmpi(bufp, "mindless ", l = 9)) {
				undeadtype = MINDLESS;
			}
			else if (!strncmpi(bufp, "poison_template ", l = 16)) {
				undeadtype = POISON_TEMPLATE;
			}
			else if (!strncmpi(bufp, "moly-", l = 5)) {
				undeadtype = MOLY_TEMPLATE;
			}
			else if (!strncmpi(bufp, "suicidal ", l = 9)) {
				mad_suicidal = TRUE;
			}
			else if (!strncmpi(bufp, "noequip ", l = 8)) {
				noequip = TRUE;
			}
			else if (!strncmpi(bufp, "male ", l = 5)) {
				makemale = TRUE;
			}
			else if (!strncmpi(bufp, "female ", l = 7)) {
				makefemale = TRUE;
			}
			else
				break;

			bufp += l;
		}
		/* grab a single suffix */
		if ((p = rindex(bufp, ' ')) != 0){	/* finds the rightmost space */
			*p++ = 0;
			if		(!strncmpi(p, "zombie",		6))
				undeadtype = ZOMBIFIED;
			else if (!strncmpi(p, "skeleton",	8))
				undeadtype = SKELIFIED;
			else if (!strncmpi(p, "vitrean",	7))
				undeadtype = CRYSTALFIED;
			else if (!strncmpi(p, "remnant",	7))
				undeadtype = SLIME_REMNANT;
			else if (!strncmpi(p, "fulvous",	7))
				undeadtype = YELLOW_TEMPLATE;
			else if (!strncmpi(p, "mad_angel",	9))
				undeadtype = MAD_TEMPLATE;
			else if (!strncmpi(p, "witness",	7))
				undeadtype = FRACTURED;
			else if (!strncmpi(p, "swollen",	7))
				undeadtype = SWOLLEN_TEMPLATE;
			else if (!strncmpi(p, "one", 3) && ((q = rindex(bufp, ' ')) != 0))
			{
				*q++ = 0;
				if(!strncmpi(q, "shining", 7))
					undeadtype = ILLUMINATED;
				else
					q[-1] = ' ';
			}
			else if (!strncmpi(p, "vampire_template",	16))
				undeadtype = VAMPIRIC;
			else if (!strncmpi(p, "pseudonatural",	13))
				undeadtype = PSEUDONATURAL;
			else if (!strncmpi(p, "tomb", 4) && ((q = rindex(bufp, ' ')) != 0))
			{
				*q++ = 0;
				if (!strncmpi(q, "herd", 4))
					undeadtype = TOMB_HERD;
				else
					q[-1] = ' ';
			}
			else if (!strncmpi(p, "yith",		4))
				undeadtype = YITH;
			else if (!strncmpi(p, "cranium",	7))
				undeadtype = CRANIUM_RAT;
			else if (!strncmpi(p, "psurlon",	7))
				undeadtype = PSURLON;
			else if (!strncmpi(p, "constellation",	13))
				undeadtype = CONSTELLATION;
			else if (!strncmpi(p, "mistweaver", 10))
				undeadtype = MISTWEAVER;
			else if (!strncmpi(p, "worldshaper", 11))
				undeadtype = WORLD_SHAPER;
			else if (!strncmpi(p, "husk",	4))
				undeadtype = MINDLESS;
			else if (!strncmpi(p, "infectee",	8))
				undeadtype = SPORE_ZOMBIE;
			else if (!strncmpi(p, "cordyceps",	9))
				undeadtype = CORDYCEPS;
			else if (!strncmpi(p, "finger",	6))
				undeadtype = PSURLON;
			else if (!strncmpi(p, "plague-victim",	13))
				undeadtype = PLAGUE_TEMPLATE;
			else
			{
				/* no suffix was used, undo the split made to search for suffixes */
				p[-1] = ' ';
			}
		}

	    /* possibly allow the initial disposition to be specified */
		switch(specify_attitude)
		{
		case MT_DOMESTIC:
			maketame = TRUE;
			break;
		case MT_PEACEFUL:
			makepeaceful = TRUE;
			break;
		case MT_HOSTILE:
			makehostile = TRUE;
			break;
		default:
			/* either keep what was set above, or nothing */
			break;
		}
		/* possibly allow the derivation of monster to be specified */
		if (specify_derivation != -1)
		{
			/* possibly override prefixes/suffixes from above */
			undeadtype = specify_derivation;
		}

		/* decide whether a valid monster was chosen */
		if (strlen(bufp) == 1) {
			monclass = MAXMCLASSES;
			monclass = def_char_to_monclass(*bufp);
			if (monclass == MAXMCLASSES)
			{
				pline("I've never heard of such monsters.");
				if(in_buff){
					impossible("Bad parsed monster name in sp_lev");
					tries = 5;
					break;
				}
				continue;	//try again
			}
			if (monclass == S_MIMIC_DEF && !(ma_require || mg_restrict || gen_restrict))	// bugfeature made feature
			{
				whichpm = mkclass(monclass, Inhell ? G_HELL : G_NOHELL);
				goto createmon;	// skip past the section which needs whichpm to exist
			}
			if(!((whichpm = mkclass(monclass, G_NOHELL | G_HELL | G_PLANES | G_DEPTHS))))
			{
				pline("You ask too generally for creatures so uncommon.");
				continue;
			}
		}
		else {
			which = name_to_mon(bufp);
			if (which < LOW_PM)
			{
				pline("I've never heard of such monsters.");
				continue;	//try again
			}
			(void)cant_create(&which, FALSE);
			whichpm = &mons[which];
		}
		// validate that the creature falls within the restrictions placed on it
		if (ma_require || mg_restrict || gen_restrict){
			i = 0;
			if (monclass != MAXMCLASSES)
				while ((!(!ma_require || (whichpm->mflagsa & ma_require)) ||
						(whichpm->mflagsg & mg_restrict) ||
						(whichpm->geno & gen_restrict)) && i < 100)
					{
					if ((whichpm = mkclass(monclass, G_NOHELL | G_HELL | G_PLANES | G_DEPTHS)))
						i++;
					else
						i = 100;
					}
			else
			{
				if (!(!ma_require || (whichpm->mflagsa & ma_require)) ||
					(whichpm->mflagsg & mg_restrict) ||
					(whichpm->geno & gen_restrict))
				{
					i = 100;
				}
			}
			if (i == 100){
				pline("%s %s%s cannot be summoned.",
					(monclass == MAXMCLASSES) ? "That" : "Those",
					(is_angel(whichpm)) ? "being" : "monster",
					(monclass == MAXMCLASSES) ? "" : "s");
				continue;	// try again
			}
		}
		/* if it didn't hit a continue at this point, we're good */
		break;
	} while (++tries < 5);

	if (tries == 5) {
	    pline1(thats_enough_tries);
	} else {
createmon:
		for (i = 0; i <= (allow_multi ? multi : 0); i++) {
			if (monclass != MAXMCLASSES && !(ma_require || mg_restrict || gen_restrict))
				whichpm = mkclass(monclass, G_NOHELL | G_HELL | G_PLANES | G_DEPTHS);

			int mm_flags = NO_MM_FLAGS;
			if (maketame)
				mm_flags |= MM_EDOG;
			if (makesummoned)
				mm_flags |= MM_ESUM;
			if (noequip)
				mm_flags |= NO_MINVENT;
			if (makemale)
				mm_flags |= MM_MALE;
			if (makefemale)
				mm_flags |= MM_FEMALE;

			mtmp = makemon_full(whichpm, x, y, mm_flags, undeadtype ? undeadtype : -1, -1);

			if (mtmp) {
				if (maketame){
					initedog(mtmp);
					if(makeloyal)
						EDOG(mtmp)->loyal = 1;
				}
				else if (makepeaceful)
					mtmp->mpeaceful = 1;
				else if (makehostile){
					mtmp->mpeaceful = 0;
					if(Infuture)
						set_faction(mtmp, ILSENSINE_FACTION);
				}
				set_malign(mtmp);

				if (makesummoned)
					mark_mon_as_summoned(mtmp, (struct monst *)0, ESUMMON_PERMANENT, 0);

				if (mad_suicidal)
					mtmp->msuicide = TRUE;

				madeany = TRUE;
				newsym(mtmp->mx, mtmp->my);
			}
		}
	}
	return mtmp;
}

static void
learn_spell_aphanactonan(spellnum)
int spellnum;
{
	int i;
	char splname[BUFSZ];
	Sprintf(splname, objects[spellnum].oc_name_known ? "\"%s\"" : "the \"%s\" spell", OBJ_NAME(objects[spellnum]));
	for (i = 0; i < MAXSPELL; i++)  {
		if (spellid(i) == spellnum)  {
			if (spellknow(i) < KEEN) {
				Your("knowledge of %s is keener.", splname);
				incrnknow(i);
				exercise(A_WIS,TRUE);       /* extra study */
			// } else { /* 1000 < spellknow(i) <= KEEN */
				// You("know %s quite well already.", splname);
			}
			break;
		} else if (spellid(i) == NO_SPELL)  {
			spl_book[i].sp_id = spellnum;
			spl_book[i].sp_lev = objects[spellnum].oc_level;
			incrnknow(i);
			You("suddenly know how to cast %s!",OBJ_NAME(objects[spellnum]));
			break;
		}
	}
	int booktype;
	if ((booktype = further_study(spellnum))){
		You("can even see a way to cast another spell.");
		Sprintf(splname, objects[booktype].oc_name_known ? "\"%s\"" : "the \"%s\" spell", OBJ_NAME(objects[booktype]));
		for (i = 0; i < MAXSPELL; i++)  {
			if (spellid(i) == booktype)  {
				if (spellknow(i) <= KEEN) {
					Your("knowledge of %s is keener.", splname);
					incrnknow(i);
					exercise(A_WIS, TRUE);       /* extra study */
				}
				else { /* 1000 < spellknow(i) <= KEEN */
					You("know %s quite well already.", splname);
				}
				break;
			}
			else if (spellid(i) == NO_SPELL)  {
				spl_book[i].sp_id = booktype;
				spl_book[i].sp_lev = objects[booktype].oc_level;
				incrnknow(i);
				You("add %s to your repertoire.", splname);
				break;
			}
		}
	}
}


#endif /* WIZARD */

#endif /* OVLB */

/*read.c*/
