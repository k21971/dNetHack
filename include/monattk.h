/*	SCCS Id: @(#)monattk.h	3.4	2002/03/24	*/
/* NetHack may be freely redistributed.  See license for details. */
/* Copyright 1988, M. Stephenson */

#ifndef MONATTK_H
#define MONATTK_H

/*	Add new attack types below - ordering affects experience (exper.c).
 *	Attacks > AT_BUTT are worth extra experience.
 */
#define AT_ANY		(-1)	/* fake attack; dmgtype_fromattack wildcard */
#define AT_NONE		0	/* passive monster (ex. acid blob) */
#define AT_CLAW		1	/* claw (punch, hit, etc.) */
#define AT_BITE		2	/* bite */
#define AT_KICK		3	/* kick */
#define AT_BUTT		4	/* head butt (ex. a unicorn) */
#define AT_TUCH		5	/* touches */
#define AT_STNG		6	/* sting */
#define AT_HUGS		7	/* crushing bearhug */
#define AT_SPIT		10	/* spits substance - ranged */
#define AT_ENGL		11	/* engulf (swallow or by a cloud) */
#define AT_BREA		12	/* breath - ranged */
#define AT_BRSH		13	/* breath - close-ranged splash */
#define AT_EXPL		14	/* explodes - proximity */
#define AT_BOOM		15	/* explodes when killed */
#define AT_GAZE		16	/* gaze - ranged, active, like pyrolisk */
#define AT_TENT		17	/* tentacles */
#define AT_ARRW		18	/* fire silver arrows from internal reservour.  Other ammo created as needed. */
#define AT_WHIP		19	/* Whips you */
#define AT_LRCH		20	/* Reach attack */
#define AT_HODS		21  /* Hod Sephirah's mirror attack */
#define AT_LNCK		22  /* Bite attack with reach */
#define AT_MMGC		23	/* uses magic spell(s), but don't allow player spellcasting (Monster-only MaGiC) */
#define AT_ILUR		24 	/* Two stage swallow attack, currently belongs to Illurien only */
#define AT_HITS		25	/* Automatic hit, no contact */
#define AT_WISP		26	/* Attack with wisps of mist, no contact */
#define AT_TNKR		27	/* Tinker attacks */
#define AT_SRPR		28	/* Phased non-contact attack, "spiritual rapier" */
#define AT_BEAM		29	/* non-contact ranged beam attack */
#define AT_DEVA		30	/* million-arm weapon attack */
#define AT_5SQR		31	/* five square reach touch */
#define AT_5SBT		32	/* five square reach bite */
#define AT_WDGZ		33	/* wide gaze - passive, like medusa */
#define AT_REND		34	/* hits if the previous two attacks hit, otherwise does nothing */
#define AT_VINE		35	/* Lashing vines */
#define AT_BKGT		36	/* Black-goat, resolves as a bite, kick, butt, tuch, or gaze */
#define AT_BKG2		37	/* Black-goat 2, resolves as a butt, tentacle suck, or gaze */
#define AT_XSPR		38	/* Phased non-contact attack, "spiritual rapier" (xweapon offhand version) */
#define AT_MSPR		39	/* Phased non-contact attack, "spiritual rapier" (marilith offhand version) */
#define AT_DSPR		40	/* Phased non-contact attack, "spiritual rapier" (million-arm mainhand version) */
#define AT_ESPR		41	/* Phased non-contact attack, "spiritual rapier" (non-hand floating version) */
#define AT_OBIT		42	/* "Other" bite. Medusa's hair, an ancient naga's canopy, and the ring armor's skirt */
#define AT_WBIT		43	/* Wolf-head bite */
#define AT_TAIL		44	/* Tail-slap */
#define AT_TONG		45	/* Tongue attack */
#define AT_VOMT		46	/* Vomit attack */

#define AT_WEAP		252	/* uses weapon */
#define AT_XWEP		253	/* uses offhanded weapon */
#define AT_MARI		254	/* uses (i-2)th unwielded weapon in inventory */
#define AT_MAGC		255	/* uses magic spell(s) */

#define weapon_aatyp(aatyp)	( \
	(aatyp) == AT_WEAP || \
	(aatyp) == AT_XWEP || \
	(aatyp) == AT_MARI || \
	(aatyp) == AT_DEVA || \
	(aatyp) == AT_HODS \
	)

#define spirit_rapier_at(aatyp) (\
	(aatyp) == AT_SRPR || \
	(aatyp) == AT_XSPR || \
	(aatyp) == AT_MSPR || \
	(aatyp) == AT_DSPR || \
	(aatyp) == AT_ESPR \
	)

#define is_bite_at(aatyp)	(\
		(aatyp) == AT_BITE\
		|| (aatyp) == AT_LNCK\
		|| (aatyp) == AT_5SBT\
		|| (aatyp) == AT_OBIT\
		|| (aatyp) == AT_WBIT\
	)

#define is_insight_tentacle_at(aatyp)	(\
		(aatyp) == AT_TENT\
		|| (aatyp) == AT_WISP\
	)
/*	Add new damage types below.
 *
 *	Note that 1-10 correspond to the types of attack used in buzz().
 *	Please don't disturb the order unless you rewrite the buzz() code.
 */
#define AD_ANY		(-1)	/* fake damage; attacktype_fordmg wildcard */
#define AD_PHYS		0	/* ordinary physical */
#define AD_MAGM		1	/* magic missiles */
#define AD_FIRE		2	/* fire damage */
#define AD_COLD		3	/* frost damage */
#define AD_SLEE		4	/* sleep ray */
#define AD_DISN		5	/* disintegration (death ray) */
#define AD_ELEC		6	/* shock damage */
#define AD_DRST		7	/* drains str (poison) */
#define AD_ACID		8	/* acid damage */
#define AD_SPC1		9	/* for extension of buzz() */
#define AD_SPC2		10	/* for extension of buzz() */
#define AD_BLND		11	/* blinds (yellow light) */
#define AD_STUN		12	/* stuns */
#define AD_SLOW		13	/* slows */
#define AD_PLYS		14	/* paralyses */
#define AD_DRLI		15	/* drains life levels */
#define AD_DREN		16	/* drains magic energy */
#define AD_LEGS		17	/* damages legs (xan) */
#define AD_STON		18	/* petrifies (Medusa, cockatrice) */
#define AD_STCK		19	/* sticks to you (mimic) */
#define AD_SGLD		20	/* steals gold (leppie) */
#define AD_SITM		21	/* steals item (nymphs) */
#define AD_SEDU		22	/* seduces & steals multiple items */
#define AD_TLPT		23	/* teleports you (Quantum Mech.) */
#define AD_RUST		24	/* rusts armour (Rust Monster)*/
#define AD_CONF		25	/* confuses (Umber Hulk) */
#define AD_DGST		26	/* digests opponent (trapper, etc.) */
#define AD_HEAL		27	/* DEPRICATED heals opponent's wounds (nurse) */
#define AD_WRAP		28	/* special "stick" for eels */
#define AD_WERE		29	/* confers lycanthropy */
#define AD_DRDX		30	/* drains dexterity (quasit) */
#define AD_DRCO		31	/* drains constitution */
#define AD_DRIN		32	/* drains intelligence (mind flayer) */
#define AD_DISE		33	/* confers diseases */
#define AD_DCAY		34	/* decays organics (brown Pudding) */
#define AD_SSEX		35	/* Succubus seduction (extended) */
				/* If no SEDUCE then same as AD_SEDU */
#define AD_HALU		36	/* causes hallucination */
#define AD_DETH		37	/* for Death only */
#define AD_PEST		38	/* for Pestilence only */
#define AD_FAMN		39	/* for Famine only */
#define AD_SLIM		40	/* turns you into green slime */
#define AD_ENCH		41	/* remove enchantment (disenchanter) */
#define AD_CORR		42	/* corrode armor (black pudding) */
#define AD_POSN		43	/* poison damage */
#define AD_WISD		44	/* wisdom damage */
#define AD_VORP		45	/* vorpal strike */
#define AD_SHRD		46	/* shreads low enchantment armor, disenchants high ench armor */
#define AD_SLVR		47	/* arrows should be silver */
#define AD_BALL		48	/* arrows should be iron balls */
#define AD_BLDR		49	/* arrows should be boulders */
#define AD_VBLD		50	/* arrows should be boulders and fired in a random spread */
#define AD_TCKL		51	/* Tickle (Nightgaunts) */
#define AD_WET		52	/* Splash with water */
#define AD_LETHE	53	/* Splash with lethe water */
#define AD_BIST		54	/* yora hawk bisecting beak attack (not implemented) */
#define AD_CNCL		60	/* same effect as a wand of cancelation */
#define AD_DEAD		61	/* deadly gaze (gaze ONLY, please!) */
#define AD_SUCK		62	/* tries to suck you appart */
#define AD_MALK		63	/* tries immobalizes you and destroys wands */
#define AD_UVUU		64	/* Uvuudaum head spike attack */
#define AD_ABDC		65	/* Abduction attack, teleports you regardless of monster cancellation */
#define AD_KAOS		66  /* Spawn Chaos */
#define AD_LSEX		67	/* DEPRICATED? Lilith's seduction (extended) */
#define AD_HLBD		68  /* Asmodeus's blood */
#define AD_SPNL		69	/* Spawns Leviathan when used with AT_BOOM*/
#define AD_MIST		70 /* Mi-go mist projector.  Works with AT_GAZE */
#define AD_TELE		71 /* Monster teleports away.  Use for hit and run attacks */
#define AD_POLY		72	/* Monster alters your DNA (was for the now-defunct genetic enginier Q) */
#define AD_PSON		73 /* "Psionic" magic attacks.  uses some from cleric and wiz */
#define AD_GROW		74 /* Monster causes a child to grow up upon death.  'ton special */
#define AD_SOUL		75 /* Monster causes like monsters to power up upon death.  Deep One special */
#define AD_TENT		76	/* extended tentacle invasion (elder priest) */
#define AD_JAILER	77	/* Sets Lucifer to apear and drops third key of law when killed */
#define AD_AXUS		78 /* Multi-element counterattack, angers 'tons */
#define AD_UNKNWN	79	/* Priest of an unknown God */
#define AD_SOLR		80	/* Light Archon's silver arrow attack */
#define AD_CHKH		81 /* Chokhmah Sephirah's escalating damage attack */
#define AD_HODS		82 /* Hod Sephirah's mirror attack */
#define AD_CHRN		83 /* Nightmare's horn attack */
#define AD_LOAD		84 /* throws loadstones */
#define AD_GARO		85 /* blows up after dispensing rumor */
#define AD_GARO_MASTER	86 /* blows up after dispensing oracle */
#define AD_LVLT		87	/* level teleport (was weeping angel) */
#define AD_BLNK		88	/* mental invasion (weeping angel) */
#define AD_WEEP		89	/* Level teleport and drain (weeping angel) */
#define AD_SPOR		90	/* generate spore */
#define AD_FNEX		91	/* FerN spore EXplosion */
#define AD_SSUN		92	/* Slaver Sunflower gaze */
#define AD_MAND		93	/* Mandrake's dying shriek (kills all on level, use w/ AT_BOOM) */
#define AD_BARB		94	/* Physical damage retalitory attack */
#define AD_LUCK		95	/* Luck-draining gaze (UnNetHack) */
#define AD_VAMP		96	/* Vampire's blood drain attack */
#define AD_WEBS		97	/* Spreads webbing on a hit */
#define AD_ILUR		98 /* memory draining engulf attack belonging to Illurien */
#define AD_TNKR		99 /* Tinker attacks */
#define AD_FRWK		100 /* "Firework" explosions */
#define AD_STDY		101 /* Monster studies you, other monsters do more damage. */
#define AD_OONA		102 /* Oona's variable energy type and v and e spawning */
#define AD_NTZC		103 /* netzach sephiroth's anti-equipment attack */
#define AD_WTCH		104 /* The Watcher in the water's tentacle-spawning gaze */
#define AD_SHDW		105 /* Black Web shadow weapons */
#define AD_STTP		106 /* Steal by Teleportation: Teleports your gear away */
#define AD_HDRG		107 /* Half-dragon breath weapon */
#define AD_STAR		108 /* Silver starlight rapier */
#define AD_EELC		109	/* Elemental electric */
#define AD_EFIR		110	/* Elemental fire */
#define AD_EDRC		111	/* Elemental poison (con) */
#define AD_ECLD		112	/* Elemental cold */
#define AD_EACD		113	/* Elemental acid */
#define AD_CNFT		114	/* War's conflict-inducing touch */
#define AD_BLUD		115	/* Sword of Blood inflicts heavy damage on creatures with blood */
#define AD_SURY		116	/* Surya Deva's arrow of slaying */
#define AD_NPDC		117	/* drains constitution (not poison) */
#define AD_GLSS		118	/* silver mirror shards */
#define AD_MERC		119	/* mercury blade */
#define AD_GOLD     120 /* goldify (only implemented for mammon's breath attack!) */
#define AD_ACFR     121 /* Archon fire (1x Phys, +1x Fire, +1x Holy) */
#define AD_DESC     122 /* Suck water out of the target */
#define AD_BLAS     123 /* Blaspheme on the target's behaf */
#define AD_SESN     124 /* 4 seasons attack */
#define AD_POLN     125 /* Cover in pollen */
#define AD_BDFN     126 /* Blood Frenzy */
#define AD_SPHR     127 /* Creates spheres */
#define AD_DARK     128 /* Does extra damage to mortal races */

#define AD_LRVA     129 /* Implants eggs (tannin special attack) */
#define AD_NPDS     133 /* Non-poison-based drain strength */
#define AD_NPDD     134 /* Non-poison-based drain dexterity */
#define AD_NPDR     135 /* Non-poison-based drain charisma */
#define AD_NPDA     136 /* Non-poison-based drain all */

#define AD_HOOK     130 /* Flesh hooks (imobalize target) */
#define AD_MDWP     131 /* Mindwipe (amnesia-based leveldrain) */
#define AD_SSTN     132 /* Slow stoning */
#define AD_DOBT     137 /* Inflict doubt */
#define AD_APCS     138 /* Revelatory whispers */
#define AD_PULL     139 /* Pull target closer */
#define AD_PAIN     140 /* Crippling pain */
#define AD_MROT     141 /* Mummy rot/other curses */
#define AD_LAVA     142 /* Lava effects while stuck */
#define AD_PYCL     143 /* Fire, poison, phys, or blindness effects */
#define AD_MOON     144 /* Silver moonlight rapier, depends on phase of moon */
#define AD_HOLY     145 /* Holy energy (MM-stype damage not resisted by MR and doubled against holy-haters)  */
#define AD_UNHY     146 /* Unholy energy (MM-stype damage not resisted by MR and doubled against unholy-haters) */
#define AD_PERH     147 /* "Per-Hit-Die damage (x damage per HD of the defender, be careful with damage die size!) */
#define AD_SVPN     148 /* Severe poison that partially pierces poison resistance */
#define AD_HLUH     149 /* Holy/Unholy energy (MM-stype damage not resisted by MR and doubled against both holy- and unholy-haters) */
#define AD_TSMI     150 /* Tentacles Steal Magic Items */
#define AD_BYAK     151 /* Byakhee eggs */
#define AD_BSTR		152 /* Black star rapier */
#define AD_UNRV		153 /* Unnerving gaze */
#define AD_MADF		154 /* Madness fire */
#define AD_FATK		155 /* Force target to attack */
#define AD_DRHP		156 /* Drains bonus HP */
#define AD_PUSH		157 /* Push target away */
#define AD_LICK		158 /* Pull target, immobalize target, cold touch, acid touch */
#define AD_PFBT		159 /* rot and poison damage */
#define AD_OMUD		160 /* Acid and stab with bone daggers */

//#define AD_AHAZU	116 /*  */
//Amon is a headbutt (AT_BUTT/AD_PHYS)
//Chupoclops is a poisonous bite (AT_BITE/AD_DRST)
#define AD_DUNSTAN	161
#define AD_IRIS		AD_DUNSTAN+1
#define AD_NABERIUS	AD_DUNSTAN+2
#define AD_OTIAX	AD_DUNSTAN+3
#define AD_SIMURGH	AD_DUNSTAN+4


//#define AD_VMSL		124 //Vorlon missile: elect, disintegration, blast.  Triple damage.
#define AD_CMSL		AD_SIMURGH+1 //Cold missile
#define AD_FMSL		AD_SIMURGH+2 //Fire missile
#define AD_EMSL		AD_SIMURGH+3 //Electric missile
#define AD_SMSL		AD_SIMURGH+4 //Shrapnel missile: physical

//#define AD_VTGT		129 //Vorlon targeting GAZE
#define AD_WMTG		AD_SIMURGH+5 //War machine targeting GAZE

#define AD_CLRC		AD_WMTG+1	/* random clerical spell */
#define AD_SPEL		AD_CLRC+1	/* random magic spell */
#define AD_RBRE		AD_CLRC+2	/* random breath weapon */
#define AD_RGAZ		AD_CLRC+3	/* random gaze attack */
#define AD_RETR		AD_CLRC+4 /* elemental gaze attack */

#define AD_SAMU		AD_CLRC+5	/* hits, may steal Amulet (Wizard) */
#define AD_CURS		AD_CLRC+6	/* random curse (ex. gremlin) */
#define AD_SQUE		AD_CLRC+7	/* hits, may steal Quest Art or Amulet (Nemeses) */

#define NUM_AD_TYPES AD_SQUE+1

#define real_spell_adtyp(adtyp) \
	((adtyp) == AD_SPEL || (adtyp) == AD_CLRC || (adtyp) == AD_PSON)

#define no_contact_attk(attk) (\
	(spirit_rapier_at(attk->aatyp) && attk->adtyp != AD_MERC) || \
	attk->aatyp == AT_WISP || attk->aatyp == AT_HITS || attk->aatyp == AT_VOMT)
/*
 *  Monster to monster attacks.  When a monster attacks another (mattackm),
 *  any or all of the following can be returned.  See mattackm() for more
 *  details.
 */
#define MM_MISS		0x00	/* aggressor missed */
#define MM_HIT		0x01	/* aggressor hit defender */
#define MM_DEF_DIED	0x02	/* defender died */
#define MM_AGR_DIED	0x04	/* aggressor died */
#define MM_AGR_STOP 0x08	/* aggressor stopped attacking for some reason other than being fully dead */
#define MM_DEF_LSVD 0x10	/* defender died and was lifesaved */
#define MM_REFLECT	0x20	/* for projectiles: projectile was reflected. */

#endif /* MONATTK_H */
