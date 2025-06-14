/*	SCCS Id: @(#)extern.h	3.4	2003/03/10	*/
/* Copyright (c) Steve Creps, 1988.				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef EXTERN_H
#define EXTERN_H

#define E extern

/* ### alloc.c ### */

#if 0
E long *FDECL(alloc, (unsigned int));
#endif
E char *FDECL(fmt_ptr, (const genericptr,char *));

/* This next pre-processor directive covers almost the entire file,
 * interrupted only occasionally to pick up specific functions as needed. */
#if !defined(MAKEDEFS_C) && !defined(LEV_LEX_C)

/* ### allmain.c ### */

E int FDECL(you_action_cost, (int, boolean));
E int NDECL(san_threshhold);
E int NDECL(random_frequency);
E int NDECL(spawn_random_monster);
E void NDECL(moveloop);
E void NDECL(stop_occupation);
E void NDECL(display_gamewindows);
E void NDECL(newgame);
E void FDECL(welcome, (BOOLEAN_P));
#if defined(RECORD_REALTIME) || defined(REALTIME_ON_BOTL)
E time_t NDECL(get_realtime);
#endif
E void NDECL(dogoat);
E void FDECL(dogoat_mon, (struct monst *));
E void FDECL(dojellysting, (struct monst *));
E void FDECL(dorotbite, (struct monst *));
E void FDECL(dorotsting, (struct monst *));
E boolean FDECL(doyog, (struct monst *));
E void FDECL(dotwin_cast, (struct monst *));
E void FDECL(dochaos_mon, (struct monst *));
E void FDECL(dohost_mon, (struct monst *));
E void FDECL(dosnake, (struct monst *));
E void FDECL(dokraken_mon, (struct monst *));
E void FDECL(dochain_lashes, (struct monst *));
E void FDECL(dotailslap, (struct monst *));
E void FDECL(dovines, (struct monst *));
E void FDECL(dostarblades, (struct monst *));
E void FDECL(dostorm, (struct monst *));
E void FDECL(donachash, (struct monst *));
E void FDECL(incarnator_spawn, (xchar, xchar, boolean));

/* ### apply.c ### */

E int NDECL(doapply);
E int NDECL(dorub);
E int NDECL(doresearch);
E int NDECL(dojump);
E int FDECL(jump, (int));
E int NDECL(number_leashed);
E void FDECL(o_unleash, (struct obj *));
E void FDECL(m_unleash, (struct monst *,BOOLEAN_P));
E void NDECL(unleash_all);
E boolean NDECL(next_to_u);
E struct obj *FDECL(get_mleash, (struct monst *));
E void FDECL(check_leash, (XCHAR_P,XCHAR_P));
E boolean FDECL(um_dist, (XCHAR_P,XCHAR_P,XCHAR_P));
E boolean FDECL(snuff_candle, (struct obj *));
E boolean FDECL(snuff_lit, (struct obj *));
E boolean FDECL(catch_lit, (struct obj *));
E void FDECL(use_unicorn_horn, (struct obj *));
E void FDECL(use_bell, (struct obj **, int));
E boolean FDECL(tinnable, (struct obj *));
E void NDECL(reset_trapset);
E void FDECL(fig_transform, (genericptr_t, long));
E int FDECL(use_whip, (struct obj *));
E int FDECL(use_nunchucks, (struct obj *));
E int FDECL(use_force_sword, (struct obj *));
E int FDECL(use_hunter_axe, (struct obj *));
E int FDECL(use_church_pick, (struct obj *));
E int FDECL(do_bloodletter, (struct obj *));
E int NDECL(parasite_count);
E boolean NDECL(parasite_ok);
E int NDECL(defile_count);
E int NDECL(preservation_count);
E int NDECL(vampire_count);
E int NDECL(rot_count);
E boolean NDECL(defile_ok);
E boolean NDECL(impurity_ok);
E boolean NDECL(research_incomplete);
E int NDECL(reanimation_count);
E boolean NDECL(reanimation_insight_ok);
E boolean NDECL(reanimation_ok);
E void FDECL(pyramid_effects, (struct obj *, int, int));
E boolean FDECL(use_ring_of_wishes, (struct obj *));
E boolean FDECL(use_candle_of_invocation, (struct obj *));
E void FDECL(use_magic_whistle, (struct obj *));
E void FDECL(x_uses_crystal_skull, (struct obj **, struct monst *, coord *));
E int FDECL(unfixable_trouble_count,(BOOLEAN_P));
E int NDECL(dotrephination_menu);
E int NDECL(partial_action);
E int NDECL(check_partial_action);
E int FDECL(check_res_engine, (struct monst *, int));
E void FDECL(add_class, (char *, char));
E boolean FDECL(set_obj_shape, (struct obj *, long));
E void FDECL(salve_effect, (struct obj *));

/* ### artifact.c ### */

E boolean FDECL(CountsAgainstGifts, (int));
E boolean FDECL(offerable_artifact, (struct obj *));
E void NDECL(init_artifacts);
E void NDECL(make_singing_sword_nameable);
E void NDECL(hack_artifacts);
E void FDECL(save_artifacts, (int));
E void FDECL(restore_artifacts, (int));
E int NDECL(n_artifacts);
E struct artifact * NDECL(add_artifact);
E const char *FDECL(artiname, (int));
E int FDECL(arti_value, (struct obj *));
E struct obj *FDECL(mk_randart, (struct obj *));
E struct obj *FDECL(mk_artifact, (struct obj *,ALIGNTYP_P));
E struct obj *FDECL(mk_special, (struct obj *));
E struct obj *FDECL(mk_vault_special, (struct obj *, int));
E struct obj *FDECL(mk_lolth_vault_special, (struct obj *));
E struct obj *FDECL(mk_minor_special, (struct obj *));
E void FDECL(add_oprop, (struct obj *, int));
E void FDECL(remove_oprop, (struct obj *, int));
E void FDECL(add_oprop_list, (unsigned long int *, int));
E boolean FDECL(check_oprop, (struct obj *, int));
E boolean FDECL(oprops_match, (struct obj *, struct obj *));
E void FDECL(copy_oprop_list, (struct obj *, unsigned long int *));
E const char *FDECL(artifact_name, (const char *,short *, int *));
E boolean FDECL(art_already_exists, (int));
E boolean FDECL(art_already_exists_byname, (int,const char *));
E int FDECL(get_already_exists_byname, (int,const char *));
E void FDECL(flag_existance, (int, int));
E void FDECL(artifact_exists, (struct obj *,const char *,BOOLEAN_P));
E struct obj * FDECL(mksartifact, (int));
E void FDECL(get_art_property_list, (int *, int, BOOLEAN_P));
E int NDECL(nartifact_exist);
E boolean FDECL(arti_gen_prop, (struct obj *, unsigned long));
E boolean FDECL(arti_worn_prop, (struct obj *, unsigned long));
E boolean FDECL(arti_carry_prop, (struct obj *, unsigned long));
E boolean FDECL(arti_attack_prop, (struct obj *, unsigned long));
E boolean FDECL(arti_is_prop, (struct obj *, unsigned long));
E boolean FDECL(confers_luck, (struct obj *));
E boolean FDECL(arti_digs, (struct obj *));
E boolean FDECL(arti_poisoned, (struct obj *));
E boolean FDECL(arti_bright, (struct obj *));
E boolean FDECL(arti_shattering, (struct obj *));
E boolean FDECL(arti_disarm, (struct obj *));
E boolean FDECL(arti_steal, (struct obj *));
E boolean FDECL(arti_tentRod, (struct obj *));
E boolean FDECL(arti_threeHead, (struct obj *));
E boolean FDECL(arti_dluck, (struct obj *));
E boolean FDECL(arti_dexpl, (struct obj *));
E boolean FDECL(arti_phasing, (struct obj *));
E boolean FDECL(arti_mandala, (struct obj *));
E boolean FDECL(arti_lighten, (struct obj *, boolean));
E boolean FDECL(arti_chawis, (struct obj *, boolean));
E boolean FDECL(arti_blindres, (struct obj *, boolean));
E boolean FDECL(arti_plussev, (struct obj *));
E boolean FDECL(arti_plusten, (struct obj *));
E boolean FDECL(arti_silvered, (struct obj *));
E boolean FDECL(arti_returning, (struct obj *));
E boolean FDECL(arti_reflects, (struct obj *));
E int FDECL(artifact_weight, (struct obj *));
E boolean FDECL(arti_light, (struct obj *));
E boolean FDECL(restrict_name, (struct obj *,const char *));
E void FDECL(set_artifact_intrinsic, (struct obj *,BOOLEAN_P,long));
E int FDECL(touch_artifact, (struct obj *,struct monst *, int));
E int FDECL(spec_abon, (struct obj *,struct monst *, boolean));
E boolean FDECL(spec_dbon, (struct obj *,struct monst *,int,int*,int*));
E boolean FDECL(oproperty_dbon, (struct obj *, struct monst *, struct monst *, int, int*, int*));
E boolean FDECL(material_dbon, (struct obj *, struct monst *, struct monst *, int, int*, int*, int));
E void FDECL(mercy_blade_conflict, (struct monst *, struct monst *, int, boolean));
E void FDECL(mind_blade_conflict, (struct monst *, struct monst *, int, boolean));
E void FDECL(mindstealer_conflict, (struct monst *, struct monst *));
E int FDECL(spec_applies, (struct obj *,struct monst *, boolean));
E void FDECL(discover_artifact, (int));
E void FDECL(undiscover_artifact, (int));
E boolean FDECL(undiscovered_artifact, (int));
E int FDECL(disp_artifact_discoveries, (winid));
E void FDECL(otyp_hit, (struct monst *,struct monst *,
				struct obj *,int, int *,int*, int, boolean *, boolean, boolean));
E int FDECL(special_weapon_hit, (struct monst *,struct monst *,
				struct obj *,struct obj*,int,int*,int*,int,boolean*,boolean));
E int NDECL(doinvoke);
E int FDECL(doparticularinvoke,(struct obj *));
E void FDECL(arti_speak, (struct obj *));
E boolean FDECL(artifact_light, (struct obj *));
E int FDECL(artifact_wet, (struct obj *, BOOLEAN_P));
E long FDECL(spec_mm, (int));
E long FDECL(spec_mt, (int));
E long FDECL(spec_mf, (int));
E long FDECL(spec_mb, (int));
E long FDECL(spec_mg, (int));
E long FDECL(spec_ma, (int));
E long FDECL(spec_mv, (int));
E long FDECL(spec_s,  (int));
E boolean FDECL(artifact_has_invprop, (struct obj *,UCHAR_P));
E long FDECL(arti_cost, (struct obj *));
E void FDECL(arti_poly_contents, (struct obj *));
E struct obj * FDECL(minor_artifact, (struct obj *, char *));
E void NDECL(dosymbiotic_equip);
E void NDECL(do_passive_attacks);
E void NDECL(living_items);
E int FDECL(oresist_disintegration, (struct obj *));
E int FDECL(wrath_target, (struct obj *, struct monst *));
E int FDECL(goat_weapon_damage_turn, (struct obj *));
E int FDECL(soth_weapon_damage_turn, (struct obj *));
E int FDECL(merc_weapon_damage_slice, (struct obj *, struct monst *, int));
E int FDECL(infinity_s_mirrored_arc_litness, (struct obj *));
E int FDECL(merge_skies, (struct obj **));
E void NDECL(do_your_auras);
E int NDECL(throweffect);

/* ### astar.c ### */
E boolean FDECL(path_exists, (int, int, int, int, long, int));
/* ### attrib.c ### */

E boolean FDECL(adjattrib, (int,int,int));
E void FDECL(change_luck, (SCHAR_P));
E int FDECL(stone_luck, (BOOLEAN_P));
E boolean NDECL(has_luckitem);
E void NDECL(set_moreluck);
E int NDECL(stone_health);
E int NDECL(stone_energy);
E void FDECL(gainstr, (struct obj *,int));
E void FDECL(losestr, (int));
E void NDECL(restore_attrib);
E void FDECL(exercise, (int,BOOLEAN_P));
E void NDECL(exerchk);
E void NDECL(reset_attribute_clock);
E void FDECL(init_attr, (int));
E void NDECL(redist_attr);
E void FDECL(adjabil, (int,int));
E int NDECL(newhp);
E int NDECL(newen);
E int FDECL(maxhp, (int));
E int NDECL(maxen);
E double FDECL(conplus, (int));
E void FDECL(change_usanity, (int, boolean));
E void FDECL(set_silvergrubs, (boolean));
E void FDECL(change_uinsight, (int));
E boolean NDECL(check_insight);
E int FDECL(roll_impurity, (boolean));
E int FDECL(roll_generic_madness, (boolean));
E int FDECL(roll_generic_flat_madness, (boolean));
E int NDECL(count_madnesses);
E int FDECL(roll_madness, (long long int));
E int FDECL(mad_turn, (long long int));
E int FDECL(flat_mad_turn, (long long int));
E int FDECL(mad_monster_turn, (struct monst *, long long int));
E void NDECL(roll_av_frigophobia);
E void NDECL(roll_frigophobia);
E void FDECL(give_madness, (struct monst *));
E void FDECL(you_inflict_madness, (struct monst *));
E void NDECL(calc_total_maxhp);
E void NDECL(calc_total_maxen);
E schar FDECL(acurr, (int, struct monst *));
E schar FDECL(acurr_player, (int));
E schar FDECL(acurrstr, (int));
E void FDECL(adjalign, (int));
E void FDECL(unSetFightingForm, (int));
E void FDECL(setFightingForm, (int));
E boolean FDECL(activeFightingForm, (int));
E boolean FDECL(activeMentalEdge, (int));
E boolean FDECL(selectedFightingForm, (int));
E int FDECL(getFightingFormSkill, (int));
E const char * FDECL(nameOfMentalEdge, (int));
E const char * FDECL(nameOfFightingForm, (int));
E void NDECL(validateLightsaberForm);
E boolean FDECL(blockedFightingForm, (int));
E boolean FDECL(blockedMentalEdge, (int));
E int NDECL(uhp);
E int NDECL(uhpmax);
E void NDECL(check_brainlessness);
E boolean FDECL(change_hod, (int));
E boolean FDECL(change_chokhmah, (int));
E boolean FDECL(change_gevurah, (int));
E boolean FDECL(change_keter, (int));

/* ### ball.c ### */

E void FDECL(ballrelease, (boolean));
E void NDECL(ballfall);
E void NDECL(placebc);
E void NDECL(unplacebc);
E void FDECL(set_bc, (int));
E void FDECL(move_bc, (int,int,XCHAR_P,XCHAR_P,XCHAR_P,XCHAR_P));
E boolean FDECL(drag_ball, (XCHAR_P,XCHAR_P,
		int *,xchar *,xchar *,xchar *,xchar *, boolean *,BOOLEAN_P));
E void FDECL(drop_ball, (XCHAR_P,XCHAR_P));
E void NDECL(drag_down);
E void NDECL(bc_sanity_check);

/* ### bones.c ### */

E boolean NDECL(can_make_bones);
E void FDECL(savebones, (struct obj *));
E int NDECL(getbones);

/* ### botl.c ### */

E int NDECL(force_bot);
E int FDECL(xlev_to_rank, (int));
E int FDECL(title_to_mon, (const char *,int *,int *));
E void NDECL(max_rank_sz);
#ifdef SCORE_ON_BOTL
E long NDECL(botl_score);
#endif
E int FDECL(describe_level, (char *));
E const char *FDECL(code_of, (SHORT_P));
E const char *FDECL(rank_of, (int,SHORT_P,BOOLEAN_P));
E void NDECL(bot);
#ifdef DUMP_LOG
E void FDECL(bot1str, (char *));
E void FDECL(bot2str, (char *, boolean, int, boolean));
E void FDECL(bot3str, (char *, boolean, int));
#endif
E void FDECL(do_statuseffects, (char *, boolean, int, int));
E long FDECL(get_status_duration, (long long));
E long long NDECL(get_status_mask);

/* ### cmd.c ### */


E char NDECL(randomkey);
E int NDECL(doGithForm);
#ifdef USE_TRAMPOLI
E int NDECL(doextcmd);
E int NDECL(domonability);
E int NDECL(dofightingform);
E int NDECL(doprev_message);
E int NDECL(timed_occupation);
E int NDECL(wiz_attributes);
E int NDECL(enter_explore_mode);
# ifdef WIZARD
E int NDECL(wiz_detect);
E int NDECL(wiz_genesis);
E int NDECL(wiz_identify);
E int NDECL(wiz_level_tele);
E int NDECL(wiz_map);
E int NDECL(wiz_where);
E int NDECL(wiz_wish);
# endif /* WIZARD */
#endif /* USE_TRAMPOLI */
E int NDECL(wiz_makemap);
E void NDECL(reset_occupations);
E void FDECL(set_occupation, (int (*)(void),const char *,int));
#ifdef REDO
E char NDECL(pgetchar);
E void FDECL(pushch, (CHAR_P));
E void FDECL(savech, (CHAR_P));
#endif
#ifdef WIZARD
E void NDECL(add_debug_extended_commands);
#endif /* WIZARD */
E void FDECL(rhack, (char *));
E void NDECL(dokeylist);
E char * FDECL(find_command_key, (const char *, char *));
E int NDECL(doextlist);
E int NDECL(extcmd_via_menu);
E int FDECL(xytod, (SCHAR_P,SCHAR_P));
E void FDECL(dtoxy, (coord *,int));
E int FDECL(movecmd, (CHAR_P));
E int FDECL(getdir, (const char *));
E void NDECL(confdir);
E int FDECL(isok, (int,int));
E int FDECL(get_adjacent_loc, (const char *, const char *, XCHAR_P, XCHAR_P, coord *));
E const char *FDECL(click_to_cmd, (int,int,int));
E char NDECL(readchar);
#ifdef WIZARD
E void NDECL(sanity_check);
#endif
E void NDECL(commands_init);
E char* FDECL(stripspace, (char*));
E void FDECL(parsebindings, (char*));
E void FDECL(parseautocomplete, (char*,BOOLEAN_P));
E char FDECL(txt2key, (char*));
E char* FDECL(key2txt, (CHAR_P, char*));
E char* FDECL(str2txt, (char*, char*));
E void FDECL(querytype_add, (xchar, char *));
E void FDECL(querytype_free, (void));
E char FDECL(yn_function, (const char *, const char *, CHAR_P));
E char yesno(const char *, boolean);
E int NDECL(dotravel);

/* ### crown.c ### */

E const char * NDECL(crowning_title);
E int NDECL(gcrownu);

/* ### dbridge.c ### */

E boolean FDECL(is_pool, (int,int, BOOLEAN_P));
E boolean FDECL(is_3dwater, (int,int));
E boolean FDECL(is_lava, (int,int));
E boolean FDECL(is_ice, (int,int));
E int FDECL(is_drawbridge_wall, (int,int));
E boolean FDECL(is_db_wall, (int,int));
E boolean FDECL(find_drawbridge, (int *,int*));
E boolean FDECL(create_drawbridge, (int,int,int,BOOLEAN_P));
E void FDECL(open_drawbridge, (int,int));
E void FDECL(close_drawbridge, (int,int));
E void FDECL(destroy_drawbridge, (int,int));

/* ### decl.c ### */

E void NDECL(decl_init);

/* ### detect.c ### */

E struct obj *FDECL(o_in, (struct obj*,CHAR_P));
E struct obj *FDECL(o_material, (struct obj*,unsigned));
E struct obj *FDECL(o_artifact, (struct obj*));
E int FDECL(gold_detect, (struct obj *));
E int FDECL(food_detect, (struct obj *));
E int FDECL(object_detect, (struct obj *,int));
E int FDECL(artifact_detect, (struct obj *));
E int FDECL(book_detect, (BOOLEAN_P));
E int FDECL(monster_detect, (struct obj *,int));
E int FDECL(pet_detect_and_tame, (struct obj *));
E int FDECL(pet_detect_and_heal, (struct obj *));
E int FDECL(trap_detect, (struct obj *));
E const char *FDECL(level_distance, (d_level *));
E int FDECL(use_crystal_ball, (struct obj *));
E void NDECL(do_mapping);
E void FDECL(do_vicinity_map, (int,int));
E void FDECL(cvt_sdoor_to_door, (struct rm *));
#ifdef USE_TRAMPOLI
E void FDECL(findone, (int,int,genericptr_t));
E void FDECL(openone, (int,int,genericptr_t));
#endif
E int NDECL(findit);
E int NDECL(openit);
E void FDECL(find_trap, (struct trap *));
E int FDECL(dosearch0, (int));
E int NDECL(dosearch);
E void NDECL(sokoban_detect);
E int NDECL(doterrain);

/* ### dig.c ### */

E schar FDECL(fillholetyp, (int, int));
E boolean NDECL(is_digging);
#ifdef USE_TRAMPOLI
E int NDECL(dig);
#endif
E int NDECL(holetime);
E boolean FDECL(dig_check, (struct monst *, BOOLEAN_P, int, int));
E void FDECL(openactualrocktrap, (int,int,struct monst *));
E void FDECL(openactualdoor, (int,int,struct monst *,int));
E void FDECL(digactualhole, (int,int,struct monst *,int,BOOLEAN_P,BOOLEAN_P));
E boolean NDECL(openrocktrap);
E boolean FDECL(opennewdoor, (int,int));
E boolean FDECL(opentrapdoor, (BOOLEAN_P));
E boolean FDECL(dighole, (BOOLEAN_P));
E boolean FDECL(digfarhole, (BOOLEAN_P,int,int,boolean));
E int FDECL(use_pick_axe, (struct obj *));
E int FDECL(use_pick_axe2, (struct obj *));
E boolean FDECL(mdig_tunnel, (struct monst *));
E void FDECL(mworldshape, (struct monst *, int, int));
E void FDECL(watch_dig, (struct monst *,XCHAR_P,XCHAR_P,BOOLEAN_P));
E void FDECL(zap_dig, (int,int,int));
E struct obj *FDECL(bury_an_obj, (struct obj *));
E void FDECL(bury_objs, (int,int));
E void FDECL(unearth_objs, (int,int));
E void FDECL(rot_organic, (genericptr_t, long));
E void FDECL(rot_corpse, (genericptr_t, long));
E void FDECL(dig_up_grave, (int, int));
E void FDECL(hell_vault_items, (int, int, int));
E void FDECL(levi_spawn_items, (int, int, struct monst *));
E void FDECL(break_seal, (int, int));
#if 0
E void FDECL(bury_monst, (struct monst *));
E void NDECL(bury_you);
E void NDECL(unearth_you);
E void NDECL(escape_tomb);
E void FDECL(bury_obj, (struct obj *));
#endif

/* ### display.c ### */

#ifdef INVISIBLE_OBJECTS
E struct obj * FDECL(vobj_at, (XCHAR_P,XCHAR_P));
#endif /* INVISIBLE_OBJECTS */
E void FDECL(magic_map_background, (XCHAR_P,XCHAR_P,int));
E void FDECL(map_background, (XCHAR_P,XCHAR_P,int));
E void FDECL(map_trap, (struct trap *,int));
E int FDECL(obj_to_glyph, (struct obj *));
E void FDECL(map_object, (struct obj *,int));
E void FDECL(map_invisible, (XCHAR_P,XCHAR_P));
E void FDECL(unmap_object, (int,int));
E void FDECL(map_location, (int,int,int));
E void FDECL(feel_location, (XCHAR_P,XCHAR_P));
E void FDECL(echo_location, (XCHAR_P,XCHAR_P));
E void FDECL(newsym, (int,int));
E boolean FDECL(sense_by_scent, (struct monst *));
E void FDECL(shieldeff, (XCHAR_P,XCHAR_P));
E void FDECL(tmp_at, (int,int));
E void FDECL(swallowed, (int));
E void FDECL(under_ground, (int));
E void FDECL(under_water, (int));
E void NDECL(see_monsters);
E void NDECL(set_mimic_blocking);
E void NDECL(see_objects);
E void NDECL(see_traps);
E void NDECL(see_altars);
E void NDECL(curs_on_u);
E int NDECL(doredraw);
E void NDECL(docrt);
E void FDECL(show_glyph, (int,int,int));
E void NDECL(clear_glyph_buffer);
E void FDECL(row_refresh, (int,int,int));
E void NDECL(cls);
E void FDECL(flush_screen, (int));
#ifdef DUMP_LOG
E void FDECL(dump_screen, (int));
#endif
E int FDECL(back_to_glyph, (XCHAR_P,XCHAR_P));
E int FDECL(zapdir_to_glyph, (int,int,int));
E int FDECL(glyph_at, (XCHAR_P,XCHAR_P));
E void NDECL(set_wall_state);

/* ### do.c ### */

#ifdef USE_TRAMPOLI
E int FDECL(drop, (struct obj *));
E int NDECL(wipeoff);
#endif
E int NDECL(dodrop);
E boolean FDECL(boulder_hits_pool, (struct obj *,int,int,BOOLEAN_P));
E boolean FDECL(flooreffects, (struct obj *,int,int,const char *));
E void FDECL(doaltarobj, (struct obj *, int));
E boolean FDECL(canletgo, (struct obj *,const char *));
E void FDECL(dropx, (struct obj *));
E void FDECL(dropy, (struct obj *));
E void FDECL(obj_no_longer_held, (struct obj *));
E boolean FDECL(obj_summon_out, (struct obj *));
E int NDECL(doddrop);
E int NDECL(dodown);
E int NDECL(doup);
#ifdef INSURANCE
E void NDECL(save_currentstate);
#endif
E void FDECL(goto_level, (d_level *,BOOLEAN_P,BOOLEAN_P,int));
E void FDECL(schedule_goto, (d_level *,BOOLEAN_P,BOOLEAN_P,int,
			     const char *,const char *,int,int));
E void NDECL(deferred_goto);
E boolean FDECL(revive_corpse, (struct obj *, int));
E void FDECL(revive_mon, (genericptr_t, long));
E void FDECL(revive_mon_pickup, (genericptr_t, long));
E void FDECL(moldy_corpse, (genericptr_t, long));
E void FDECL(slimy_corpse, (genericptr_t, long));
E void FDECL(shady_corpse, (genericptr_t, long));
E void FDECL(zombie_corpse, (genericptr_t, long));
E void FDECL(yellow_corpse, (genericptr_t, long));
E int NDECL(donull);
E int NDECL(dowipe);
E void FDECL(set_wounded_legs, (long,int));
E void NDECL(heal_legs);
E int NDECL(dowait);
E int NDECL(docome);
E int NDECL(doattack);
E int NDECL(dopassive);
E int NDECL(dodownboy);
E int NDECL(dosickem);
E int NDECL(dodropall);
E int FDECL(drop, (struct obj *));

/* ### do_name.c ### */

E void FDECL(do_oname, (struct obj *));
E void NDECL(do_floorname);
E int FDECL(getpos, (coord *,BOOLEAN_P,const char *));
E struct monst *FDECL(christen_monst, (struct monst *,const char *));
E int NDECL(do_mname);
E struct obj *FDECL(oname, (struct obj *,const char *));
E int NDECL(ddocall);
E void FDECL(docall, (struct obj *));
E const char *NDECL(rndghostname);
E void FDECL(append_template_desc, (struct monst *, char *, boolean, boolean));
E void FDECL(adjust_permonst_template_desc, (struct permonst *, char *, int));
E const char *FDECL(injury_desc_word, (struct monst *));
E char *FDECL(x_monnam, (struct monst *,int,const char *,int,BOOLEAN_P));
E char *FDECL(l_monnam, (struct monst *));
E char *FDECL(mon_nam, (struct monst *));
E const char *FDECL(sheheit, (struct monst *));
E const char *FDECL(SheHeIt, (struct monst *));
E const char *FDECL(hisherits, (struct monst *));
E const char *FDECL(HisHerIts, (struct monst *));
E const char *FDECL(himherit, (struct monst *));
E char *FDECL(noit_mon_nam, (struct monst *));
E char *FDECL(noit_nohalu_mon_nam, (struct monst *));
E char *FDECL(Monnam, (struct monst *));
E char *FDECL(noit_Monnam, (struct monst *));
E char *FDECL(m_monnam, (struct monst *));
E char *FDECL(y_monnam, (struct monst *));
E char *FDECL(Adjmonnam, (struct monst *,const char *));
E char *FDECL(Amonnam, (struct monst *));
E char *FDECL(Aptrnam, (struct permonst *));
E char *FDECL(a_monnam, (struct monst *));
E char *FDECL(a_ptrnam, (struct permonst *));
E char *FDECL(mon_nam_too, (struct monst *, struct monst *));
E char *FDECL(getDrowHouse, (long));
E char *FDECL(distant_monnam, (struct monst *,int,char *));
E const char *NDECL(rndmonnam);
E const char *FDECL(hcolor, (const char *));
E const char *NDECL(rndcolor);
#ifdef REINCARNATION
E const char *NDECL(roguename);
#endif
E const char *FDECL(hliquid, (const char *));
E char *FDECL(coyotename, (struct monst *,char *));

/* ### do_wear.c ### */

#ifdef USE_TRAMPOLI
E int NDECL(Boots_on);
E int NDECL(Gloves_on);
E int NDECL(Helmet_on);
E int FDECL(select_off, (struct obj *));
E int NDECL(take_off);
#endif
E void FDECL(off_msg, (struct obj *));
E void NDECL(set_wear);
E boolean FDECL(donning, (struct obj *));
E void NDECL(cancel_don);
E int NDECL(Armor_on);
E int NDECL(Armor_off);
E int NDECL(Armor_gone);
E int NDECL(Helmet_on);
E int NDECL(Helmet_off);
E int NDECL(Gloves_off);
E int NDECL(Boots_on);
E int NDECL(Boots_off);
E int NDECL(Cloak_off);
E int NDECL(Shield_off);
E int NDECL(Shirt_on);
E int NDECL(Shirt_off);
E void NDECL(Amulet_off);
E void NDECL(Amulet_on);
E void NDECL(Belt_off);
E void NDECL(Belt_on);
E void FDECL(Ring_on, (struct obj *));
E void FDECL(Ring_off, (struct obj *));
E void FDECL(Ring_gone, (struct obj *));
E void FDECL(Blindf_on, (struct obj *));
E void FDECL(Blindf_off, (struct obj *));
E int NDECL(dotakeoff);
E int NDECL(doremring);
E int FDECL(cursed, (struct obj *));
E int FDECL(armoroff, (struct obj *));
E int FDECL(canwearobj, (struct obj *, long *, BOOLEAN_P));
E int NDECL(dowear);
E int NDECL(doputon);
E int FDECL(arm_total_bonus, (struct obj *));
E int FDECL(greatest_erosion, (struct obj *));
E int FDECL(arm_ac_bonus, (struct obj *));
E int FDECL(arm_dr_bonus, (struct obj *));
E int NDECL(base_uac);
E void NDECL(find_ac);
E int NDECL(base_udr);
E int NDECL(base_nat_udr);
E void NDECL(find_dr);
E int FDECL(slot_udr, (int, struct monst *, int, uchar));
E int FDECL(roll_udr, (struct monst *, int));
E int FDECL(roll_udr_detail, (struct monst *, int, int, uchar));
E void NDECL(glibr);
E void NDECL(bumbler);
E struct obj *FDECL(some_armor,(struct monst *));
E void FDECL(erode_armor, (struct monst *,BOOLEAN_P));
E struct obj *FDECL(stuck_ring, (struct obj *,int));
E struct obj *NDECL(cursed_unchanger);
E void NDECL(reset_remarm);
E int NDECL(doddoremarm);
E int FDECL(destroy_arm, (struct obj *));
E int FDECL(destroy_marm, (struct monst *, struct obj *));
E int FDECL(claws_destroy_arm, (struct obj *));
E int FDECL(claws_destroy_marm, (struct monst *, struct obj *));
E int FDECL(saber_destroys_arm, (struct obj *));
E int FDECL(saber_destroys_marm, (struct monst *, struct obj *));
E int FDECL(teleport_arm, (struct obj *, struct monst *));
E int FDECL(teleport_steal_arm, (struct monst *, struct obj *));
E int FDECL(tent_destroy_arm, (struct obj *));
E void FDECL(adj_abon, (struct obj *,SCHAR_P));
E int FDECL(properties_dr, (struct obj *,int,int));
E void FDECL(dosymbiotic, (struct monst *, struct obj *));
E void FDECL(doscorpion, (struct monst *, struct obj *));
E void FDECL(doliving, (struct monst *, struct obj *));
E void FDECL(doliving_dragonhead, (struct monst *, struct obj *, boolean));
E void FDECL(doliving_mad_king, (struct monst *, struct obj *, boolean));
E void FDECL(doliving_ringed_spear, (struct monst *, struct obj *, boolean));
E void FDECL(doliving_ringed_armor, (struct monst *, struct obj *, boolean));
E void FDECL(doliving_ibite_arm, (struct monst *, struct obj *, boolean));
E void FDECL(doibite_ghosts, (struct monst *, struct obj *));
E void FDECL(doliving_fallingstar, (struct monst *, struct obj *, boolean));
E void FDECL(doliving_healing_armor, (struct monst *, struct obj *, boolean));
E void FDECL(doliving_armor_salve, (struct monst *, struct obj *));
E void FDECL(dotsmi_theft, (struct monst *, struct monst *, struct obj *, struct obj *));

/* ### dog.c ### */

E void FDECL(initedog, (struct monst *));
E struct monst *FDECL(make_familiar, (struct obj *,XCHAR_P,XCHAR_P,BOOLEAN_P));
E struct monst *NDECL(makedog);
E void NDECL(update_mlstmv);
E void NDECL(losedogs);
E void FDECL(mon_extract_from_list, (struct monst *, struct monst **));
E void FDECL(mon_arrive_on_level, (struct monst *));
E void FDECL(mon_arrive, (struct monst *,BOOLEAN_P));
E void FDECL(mon_catchup_elapsed_time, (struct monst *,long));
E void FDECL(keepdogs, (BOOLEAN_P, d_level *, int));
E void FDECL(migrate_to_level, (struct monst *,int,XCHAR_P,coord *));
E int FDECL(dogfood, (struct monst *,struct obj *));
E boolean FDECL(is_edible_mon, (struct monst *,struct obj *));
E struct monst *FDECL(tamedog, (struct monst *,struct obj *));
E struct monst *FDECL(tamedog_core, (struct monst *,struct obj *, int));
E void FDECL(untame, (struct monst *, boolean));
E struct monst *FDECL(make_pet_minion, (int,int, boolean));
E void FDECL(abuse_dog, (struct monst *));
E void FDECL(wary_dog, (struct monst *, BOOLEAN_P));
E void FDECL(enough_dogs, (int));
E void NDECL(vanish_dogs);
E int NDECL(dog_limit);
E void NDECL(average_dogs);

/* ### dogmove.c ### */

E boolean FDECL(could_use_item, (struct monst *, struct obj *,BOOLEAN_P));
E boolean FDECL(acceptable_pet_target, (struct monst *,struct monst *,BOOLEAN_P));
E boolean FDECL(betrayed, (struct monst *));
E int FDECL(dog_nutrition, (struct monst *,struct obj *));
E int FDECL(dog_eat, (struct monst *,struct obj *,int,int,BOOLEAN_P));
E void FDECL(give_mon_corpse_intrinsic, (struct monst *,int));
E int FDECL(monster_eat, (struct monst *,struct obj *,int,int,BOOLEAN_P));
E int FDECL(dog_move, (struct monst *,int));
#ifdef USE_TRAMPOLI
E void FDECL(wantdoor, (int,int,genericptr_t));
#endif
E struct obj *FDECL(DROPPABLES, (struct monst *));
E struct obj *FDECL(drop_envy, (struct monst *));

/* ### dokick.c ### */

E void FDECL(kickdmg, (struct monst *, BOOLEAN_P));
E void FDECL(dive_kick_monster, (struct monst *));
E void NDECL(bird_kick_monsters);
E void NDECL(wing_storm_monsters);
E boolean FDECL(ghitm, (struct monst *,struct obj *));
E void FDECL(container_impact_dmg, (struct obj *));
E int NDECL(dokick);
E int FDECL(dokick_core, (int, int));
E boolean FDECL(ship_object, (struct obj *,XCHAR_P,XCHAR_P,BOOLEAN_P));
E void NDECL(obj_delivery);
E schar FDECL(down_gate, (XCHAR_P,XCHAR_P));
E void FDECL(impact_drop, (struct obj *,XCHAR_P,XCHAR_P,XCHAR_P,boolean));

/* ### dothrow.c ### */

E int FDECL(zap_raygun, (struct obj *,int, int));
E void FDECL(hurtle, (int,int,int,BOOLEAN_P,BOOLEAN_P));
E void FDECL(mhurtle, (struct monst *,int,int,int,boolean));
E int FDECL(hero_breaks, (struct obj *,XCHAR_P,XCHAR_P,BOOLEAN_P));
E int FDECL(breaks, (struct obj *,XCHAR_P,XCHAR_P));
E boolean FDECL(breaktest, (struct obj *));
E boolean FDECL(walk_path, (coord *, coord *, boolean (*)(genericptr_t,int,int), genericptr_t));
E boolean FDECL(hurtle_step, (genericptr_t, int, int));

/* ### drawing.c ### */
#endif /* !MAKEDEFS_C && !LEV_LEX_C */
E int FDECL(def_char_to_objclass, (CHAR_P));
E int FDECL(def_char_to_monclass, (CHAR_P));
#if !defined(MAKEDEFS_C) && !defined(LEV_LEX_C)
E void FDECL(assign_graphics, (glyph_t *,int,int,int));
E void FDECL(switch_graphics, (int));
#ifdef REINCARNATION
E void FDECL(assign_rogue_graphics, (BOOLEAN_P));
#endif
E void FDECL(assign_utf8graphics_symbol, (int, glyph_t));
#ifdef USER_DUNGEONCOLOR
E void FDECL(assign_colors, (uchar *,int,int,int));
#endif

/* ### dungeon.c ### */

E void FDECL(save_dungeon, (int,BOOLEAN_P,BOOLEAN_P));
E void FDECL(restore_dungeon, (int));
E void FDECL(insert_branch, (branch *,BOOLEAN_P));
E void NDECL(init_dungeons);
E s_level *FDECL(find_level, (const char *));
E s_level *FDECL(Is_special, (d_level *));
E branch *FDECL(Is_branchlev, (d_level *));
E d_level *FDECL(branchlev_other_end, (branch *, d_level *));
E int FDECL(ledger_no, (d_level *));
E int NDECL(maxledgerno);
E schar FDECL(depth, (d_level *));
E int FDECL(dunlev, (d_level *));
E int FDECL(dunlevs_in_dungeon, (d_level *));
E int FDECL(ledger_to_dnum, (int));
E int FDECL(ledger_to_dlev, (int));
E int FDECL(deepest_lev_reached, (BOOLEAN_P));
E boolean FDECL(on_level, (d_level *,d_level *));
E void FDECL(next_level, (BOOLEAN_P));
E void FDECL(prev_level, (BOOLEAN_P));
E void FDECL(u_on_newpos, (int,int));
E void NDECL(u_on_sstairs);
E void NDECL(u_on_upstairs);
E void NDECL(u_on_dnstairs);
E boolean FDECL(On_stairs, (XCHAR_P,XCHAR_P));
E void FDECL(get_level, (d_level *,int));
E boolean FDECL(Is_botlevel, (d_level *));
E boolean FDECL(Can_fall_thru, (d_level *));
E boolean FDECL(Can_dig_down, (d_level *));
E boolean FDECL(Can_rise_up, (int,int,d_level *));
E boolean FDECL(In_quest, (d_level *));
E boolean FDECL(In_outdoors, (d_level *));
E boolean FDECL(In_cave, (d_level *));
E boolean FDECL(In_mines, (d_level *));
E boolean FDECL(In_mines_quest, (d_level *));
E boolean FDECL(In_neu, (d_level *));
E boolean FDECL(In_outlands, (d_level *));
E boolean FDECL(In_lost_cities, (d_level *));
E boolean FDECL(In_cha, (d_level *));
E boolean FDECL(In_mithardir_desert, (d_level *));
E boolean FDECL(In_mithardir_catacombs, (d_level *));
E boolean FDECL(In_mithardir_terminus, (d_level *));
E boolean FDECL(In_mithardir_quest, (d_level *));
E boolean FDECL(In_FF_quest, (d_level *));
E boolean FDECL(In_mordor_quest, (d_level *));
E boolean FDECL(In_mordor_forest, (d_level *));
E boolean FDECL(In_mordor_fields, (d_level *));
E boolean FDECL(In_mordor_buildings, (d_level *));
E boolean FDECL(In_mordor_depths, (d_level *));
E boolean FDECL(In_mordor_borehole, (d_level *));
E boolean FDECL(In_law, (d_level *));
E branch *FDECL(dungeon_branch, (const char *));
E boolean FDECL(at_dgn_entrance, (const char *));
E boolean FDECL(In_hell, (d_level *));
E boolean FDECL(In_V_tower, (d_level *));
E boolean FDECL(On_W_tower_level, (d_level *));
E boolean FDECL(In_W_tower, (int,int,d_level *));
E void FDECL(find_hell, (d_level *));
E void FDECL(goto_hell, (BOOLEAN_P,BOOLEAN_P));
E void FDECL(assign_level, (d_level *,d_level *));
E void FDECL(assign_rnd_level, (d_level *,d_level *,int));
E int FDECL(induced_align, (int));
E boolean FDECL(Invocation_lev, (d_level *));
E int NDECL(level_difficulty);
E void FDECL(name_by_lev, (char *, d_level *));
E schar FDECL(lev_by_name, (const char *));
#ifdef WIZARD
E boolean FDECL(print_dungeon, (BOOLEAN_P,BOOLEAN_P,schar *,int *));
#endif
E int NDECL(donamelevel);
E int NDECL(dooverview);
E void FDECL(forget_mapseen, (int));
E void FDECL(rm_mapseen, (int));
E void FDECL(init_mapseen, (d_level *));
E void NDECL(recalc_mapseen);
E void FDECL(recbranch_mapseen, (d_level *, d_level *));
E void FDECL(remdun_mapseen, (int));
E void NDECL(dust_storm);

/* ### eat.c ### */

#ifdef USE_TRAMPOLI
E int NDECL(eatmdone);
E int NDECL(eatfood);
E int NDECL(opentin);
#endif
E int NDECL(unfaint);
E void FDECL(cprefx, (int, BOOLEAN_P, BOOLEAN_P));
E void FDECL(cpostfx, (int, BOOLEAN_P, BOOLEAN_P, BOOLEAN_P));
E boolean FDECL(incantifier_edible, (struct obj *));
E boolean FDECL(is_edible, (struct obj *));
E void NDECL(init_uhunger);
E void NDECL(reset_uhunger);
E boolean NDECL(satiate_uhunger);
E double NDECL(get_uhungersizemod);
E int NDECL(get_uhungermax);
E int NDECL(get_satiationlimit);
E int NDECL(Hear_again);
E void NDECL(reset_eat);
E int NDECL(doeat);
E void NDECL(gethungry);
E void FDECL(morehungry, (int));
E void FDECL(lesshungry, (int));
E boolean NDECL(is_fainted);
E void NDECL(reset_faint);
E void NDECL(violated_vegetarian);
#if 0
E void NDECL(sync_hunger);
#endif
E void FDECL(newuhs, (BOOLEAN_P));
E struct obj *FDECL(floorfood, (const char *,int));
E void NDECL(vomit);
E int FDECL(eaten_stat, (int,struct obj *));
E void FDECL(food_disappears, (struct obj *));
E void FDECL(food_substitution, (struct obj *,struct obj *));
E boolean FDECL(bite_monster, (struct monst *));
E void FDECL(give_intrinsic, (int, long));
E void NDECL(fix_petrification);
E int FDECL(intrinsic_possible, (int,struct permonst *));
E void FDECL(consume_oeaten, (struct obj *,int));
E int FDECL(obj_nutrition, (struct obj *));
E boolean FDECL(maybe_finished_meal, (BOOLEAN_P));
E int FDECL(ask_turns, (struct monst *, int, int));
E int FDECL(ask_cp, (struct monst *, int));
E int FDECL(start_clockwinding, (struct obj *,struct monst *, int));

/* ### end.c ### */

E void FDECL(mk_mapdump, (char *));
E void NDECL(mk_dgl_extrainfo);
E char *FDECL(dump_format_str, (char *));
E void FDECL(done1, (int));
E int NDECL(done2);
#ifdef USE_TRAMPOLI
E void FDECL(done_intr, (int));
#endif
E void FDECL(done_in_by, (struct monst *));
#endif /* !MAKEDEFS_C && !LEV_LEX_C */
E void VDECL(panic, (const char *,...)) PRINTF_F(1,2);
#if !defined(MAKEDEFS_C) && !defined(LEV_LEX_C)
E const char* NDECL(get_alignment_code);
E const char* NDECL(get_alignment_adj);
E boolean NDECL(Check_crystal_lifesaving);
E boolean NDECL(Check_iaso_lifesaving);
E boolean NDECL(Check_twin_lifesaving);
E void FDECL(done, (int));
E void FDECL(container_contents, (struct obj *,BOOLEAN_P,BOOLEAN_P));
#ifdef DUMP_LOG
E void FDECL(dump, (char *, char *));
E void FDECL(do_containerconts, (struct obj *,BOOLEAN_P,BOOLEAN_P,BOOLEAN_P,BOOLEAN_P));
#endif
E void FDECL(terminate, (int));
E int NDECL(num_genocides);
E int NDECL(num_extinct);
E void FDECL(list_genocided, (int,BOOLEAN_P,BOOLEAN_P,BOOLEAN_P));

/* ### engrave.c ### */

E void FDECL(bindspirit, (int));
E void NDECL(blank_all_wards);
E void NDECL(randomize_all_wards);
E int FDECL(pick_seal, (const char *));
E int FDECL(pick_ward, (int));
E void FDECL(describe_ward, (int));
E int NDECL(randHaluWard);
E int NDECL(random_unknown_ward);
E int FDECL(get_num_wards_added, (int, int));
E char *FDECL(random_engraving, (char *));
E int NDECL(random_haluIndex);
E const char *FDECL(fetchHaluWard, (int));
E void FDECL(wipeout_text, (char *,int,unsigned));
E boolean NDECL(can_reach_floor);
E struct engr *NDECL(get_head_engr);
E const char *FDECL(surface, (int,int));
E const char *FDECL(ceiling, (int,int));
E struct engr *FDECL(engr_at, (XCHAR_P,XCHAR_P));
E void NDECL(move_gliders);
E int FDECL(sengr_at, (const char *,XCHAR_P,XCHAR_P));
E int FDECL(ward_at, (XCHAR_P,XCHAR_P));
E int FDECL(num_wards_at, (XCHAR_P,XCHAR_P));
E void FDECL(u_wipe_engr, (int));
E int FDECL(decode_wardID, (int));
E int FDECL(get_wardID, (int));
E int FDECL(decode_sealID, (long));
E long FDECL(get_sealID, (int));
E void FDECL(wipe_engr_at, (XCHAR_P,XCHAR_P,XCHAR_P));
E void FDECL(read_engr_at, (int,int));
E void FDECL(make_engr_at, (int,int,const char *,long,XCHAR_P));
E void FDECL(del_engr_at, (int,int));
E void FDECL(del_ward_at, (int,int));
E void FDECL(del_engr_ward_at, (int,int));
E int NDECL(freehand);
E int NDECL(doengrave);
E int NDECL(doward);
E int NDECL(doseal);
E int NDECL(doengward);
E void FDECL(save_engravings, (int,int));
E void FDECL(rest_engravings, (int));
E void FDECL(del_engr, (struct engr *));
E void FDECL(del_ward, (struct engr *));
E void FDECL(del_engr_ward, (struct engr *));
E void FDECL(rloc_engr, (struct engr *));
E void FDECL(make_grave, (int,int,const char *));
E boolean FDECL(allied_faction, (int,int));

/* ### enlighten.c ### */

E int NDECL(doattributes);
E int NDECL(doconduct);
E int NDECL(doenlightenment);
E void NDECL(udr_enlightenment);
E void NDECL(signs_mirror);
E void FDECL(show_enlightenment, (int, boolean));
E void FDECL(show_conduct, (int, boolean));

/* ### exper.c ### */

E long FDECL(newuexp, (int));
E int FDECL(experience, (struct monst *,int));
E int FDECL(ptrexperience, (struct permonst *));
E void FDECL(more_experienced, (int,int));
E void FDECL(lose_experience, (int));
E void NDECL(nightmare_mold_lose_experience);
E void FDECL(losexp, (const char *,BOOLEAN_P,BOOLEAN_P,BOOLEAN_P));
E void NDECL(newexplevel);
E void NDECL(binderup);
E void FDECL(pluslvl, (BOOLEAN_P));
E long FDECL(rndexp, (BOOLEAN_P));

/* ### explode.c ### */

E void FDECL(explode, (int,int,int,int,int,int,int));
E void FDECL(explode_spell, (int, int, int, int, int, int, int, long));
E void FDECL(explode_sound, (int,int,int,int,int,int,int,int));
E void FDECL(explode_pa, (int,int,int,int,int,int,int,struct permonst *));
E void FDECL(explode_yours, (int,int,int,int,int,int,int,boolean));
E void FDECL(explode_full, (int,int,int,int,int,int,int,int, boolean, struct permonst *, long));
E void FDECL(splash, (int,int,int,int,int,int,int,int, long));
E long FDECL(scatter, (int, int, int, unsigned int, struct obj *, long *, struct monst *));
E void FDECL(splatter_burning_oil, (int, int));
E int FDECL(adtyp_expl_color, (int));
//#ifdef FIREARMS
E void FDECL(grenade_explode, (struct obj *, int, int, BOOLEAN_P, int));
E void FDECL(arm_bomb, (struct obj *, BOOLEAN_P));
//#endif

/* ### extralev.c ### */

#ifdef REINCARNATION
E void NDECL(makeroguerooms);
E void FDECL(corr, (int,int));
E void NDECL(makerogueghost);
#endif

/* ### files.c ### */
E int FDECL(chmod, (const char *path, mode_t mode));
E char *FDECL(fname_encode, (const char *, CHAR_P, char *, char *, int));
E char *FDECL(fname_decode, (CHAR_P, char *, char *, int));
E const char *FDECL(fqname, (const char *, int, int));
E FILE *FDECL(fopen_datafile, (const char *,const char *,int));
E boolean FDECL(uptodate, (int,const char *));
E void FDECL(store_version, (int));
#ifdef MFLOPPY
E void NDECL(set_lock_and_bones);
#endif
E void FDECL(set_levelfile_name, (char *,int));
E int FDECL(create_levelfile, (int,char *));
E int FDECL(open_levelfile, (int,char *));
E void FDECL(delete_levelfile, (int));
E void NDECL(clearlocks);
E int FDECL(create_bonesfile, (d_level*,char **, char *));
#ifdef MFLOPPY
E void NDECL(cancel_bonesfile);
#endif
E void FDECL(commit_bonesfile, (d_level *));
E int FDECL(open_bonesfile, (d_level*,char **));
E int FDECL(delete_bonesfile, (d_level*));
E void NDECL(compress_bonesfile);
E void NDECL(set_savefile_name);
#ifdef INSURANCE
E void FDECL(save_savefile_name, (int));
#endif
#if defined(WIZARD) && !defined(MICRO)
E void NDECL(set_error_savefile);
#endif
E int NDECL(create_savefile);
E int NDECL(open_savefile);
E int NDECL(delete_savefile);
E int NDECL(restore_saved_game);
E void FDECL(compress, (const char *));
E void FDECL(uncompress, (const char *));
E boolean FDECL(lock_file, (const char *,int,int));
E void FDECL(unlock_file, (const char *));
#ifdef USER_SOUNDS
E boolean FDECL(can_read_file, (const char *));
#endif
E void FDECL(read_config_file, (const char *));
E void FDECL(check_recordfile, (const char *));
#if defined(WIZARD)
E void NDECL(read_wizkit);
#endif
E void FDECL(paniclog, (const char *, const char *));
E int FDECL(validate_prefix_locations, (char *));
E char** NDECL(get_saved_games);
E void FDECL(free_saved_games, (char**));
#ifdef SELF_RECOVER
E boolean NDECL(recover_savefile);
#endif
#ifdef HOLD_LOCKFILE_OPEN
E void NDECL(really_close);
#endif
#ifdef WHEREIS_FILE
E void NDECL(touch_whereis);
E void NDECL(delete_whereis);
#endif
E void FDECL(livelog_write_string, (char *));

/* ### fountain.c ### */

E void FDECL(floating_above, (const char *));
E void FDECL(dogushforth, (int));
# ifdef USE_TRAMPOLI
E void FDECL(gush, (int,int,genericptr_t));
# endif
E void FDECL(dryup, (XCHAR_P,XCHAR_P, BOOLEAN_P));
E int NDECL(smithing_bonus);
E boolean FDECL(have_blood_smithing_crystal, (struct obj *));
E struct obj * FDECL(find_blood_smithing_crystal, (struct obj *));
E boolean FDECL(have_blood_smithing_fire, (struct obj *));
E boolean FDECL(have_blood_smithing_cold, (struct obj *));
E boolean FDECL(have_blood_smithing_lightning, (struct obj *));
E boolean FDECL(have_blood_smithing_acid, (struct obj *));
E boolean FDECL(have_blood_smithing_buc, (struct obj *));
E boolean FDECL(have_blood_smithing_x, (struct obj *, int));
E boolean FDECL(get_blood_smithing_x, (int, struct obj **, int *));
E boolean NDECL(check_undead_hunter_weapon_skills);
E void NDECL(expert_undead_hunter_skill);
E void NDECL(drinkfountain);
E void FDECL(dipfountain, (struct obj *));
E void FDECL(breakforge, (int, int));
E void NDECL(drinkforge);
E void FDECL(dipforge, (struct obj *));
#ifdef SINKS
E void FDECL(breaksink, (int,int));
E void NDECL(drinksink);
#endif

/* ### thoughtglyph.c ### */

E int NDECL(count_glyphs);
E long int FDECL(otyp_to_thought, (int));
E long int FDECL(mtyp_to_thought, (int));
E boolean FDECL(maybe_give_thought, (long int));
E void FDECL(give_thought, (long int));
E void FDECL(remove_thought, (long int));
E int FDECL(glyph_sanity, (long int));
E int FDECL(glyph_insight, (long int));
E boolean FDECL(active_glyph, (long int));
E boolean FDECL(known_glyph, (long int));
E boolean FDECL(was_active_glyph, (long int, int, int));
E void FDECL(change_glyph_active, (long int, boolean));
E boolean NDECL(dofreethought);

E void FDECL(doguidance, (struct monst *, int));

/* ### hack.c ### */

E int FDECL(max_ints, (int,int));
E int FDECL(min_ints, (int,int));
E boolean FDECL(revive_nasty, (int,int,const char*));
E void FDECL(movobj, (struct obj *,XCHAR_P,XCHAR_P));
E boolean FDECL(may_dig, (XCHAR_P,XCHAR_P));
E boolean FDECL(may_passwall, (XCHAR_P,XCHAR_P));
E boolean FDECL(bad_rock, (struct monst *,XCHAR_P,XCHAR_P));
E boolean FDECL(invocation_pos, (XCHAR_P,XCHAR_P));
E boolean FDECL(test_move, (int, int, int, int, int));
E boolean NDECL(adjacent_mon);
E void NDECL(domove);
E void NDECL(invocation_message);
E void FDECL(spoteffects, (BOOLEAN_P));
E char *FDECL(in_rooms, (XCHAR_P,XCHAR_P,int));
E boolean FDECL(in_town, (int,int));
E void FDECL(check_special_room, (BOOLEAN_P));
E int NDECL(dopickup);
E void NDECL(lookaround);
E int NDECL(monster_nearby);
E void FDECL(nomul, (int, const char *));
E void NDECL(maybe_wail);
E void FDECL(unmul, (const char *));
E void FDECL(showdmg, (int, BOOLEAN_P));
E void FDECL(losehp, (int,const char *,BOOLEAN_P));
E void FDECL(losepw, (int));
E void FDECL(mdamageu, (struct monst *, int));
E int NDECL(weight_cap);
E int NDECL(inv_weight);
E int NDECL(near_capacity);
E int FDECL(calc_capacity, (int));
E int NDECL(max_capacity);
E boolean FDECL(check_capacity, (const char *));
E int NDECL(inv_cnt);
#ifdef GOLDOBJ
E long FDECL(money_cnt, (struct obj *));
#endif
E struct obj * FDECL(start_all_items, (int * ));
E struct obj * FDECL(next_all_items, (int *));
E void NDECL(end_all_items);

/* ### hacklib.c ### */

E boolean FDECL(digit, (CHAR_P));
E boolean FDECL(letter, (CHAR_P));
E char FDECL(highc, (CHAR_P));
E char FDECL(lowc, (CHAR_P));
E char *FDECL(lcase, (char *));
E char *FDECL(upstart, (char *));
E char *FDECL(mungspaces, (char *));
E char *FDECL(eos, (char *));
E void FDECL(sanitizestr, (char *));
E char *FDECL(strkitten, (char *,CHAR_P));
E char *FDECL(s_suffix, (const char *));
E char *FDECL(ing_suffix, (const char *));
E char *FDECL(xcrypt, (const char *,char *));
E boolean FDECL(onlyspace, (const char *));
E char *FDECL(tabexpand, (char *));
E char *FDECL(visctrl, (CHAR_P));
E const char *FDECL(ordin, (int));
E char *FDECL(sitoa, (int));
E int FDECL(sgn, (int));
E int FDECL(rounddiv, (long,int));
E int FDECL(dist2, (int,int,int,int));
E int FDECL(distmin, (int,int,int,int));
E boolean FDECL(online2, (int,int,int,int));
E boolean FDECL(pmatch, (const char *,const char *));
#ifndef STRNCMPI
E int FDECL(strncmpi, (const char *,const char *,int));
#endif
#ifndef STRSTRI
E char *FDECL(strstri, (const char *,const char *));
#endif
E char *FDECL(strsubst, (char *bp, const char *orig, const char *replacement));
E boolean FDECL(fuzzymatch, (const char *,const char *,const char *,BOOLEAN_P));
E void NDECL(setrandom);
E struct tm * NDECL(getlt);
E int NDECL(getyear);
#if 0
E char *FDECL(yymmdd, (time_t));
#endif
E long FDECL(yyyymmdd, (time_t));
E int NDECL(phase_of_the_moon);
E boolean NDECL(friday_13th);
E boolean NDECL(is_june);
E int NDECL(night);
E int NDECL(midnight);
E unsigned long int FDECL(hash, (unsigned long int));

/* ### invent.c ### */

E boolean FDECL(mergable_traits,(struct obj *,struct obj *));
E void FDECL(assigninvlet, (struct obj *));
E struct obj *FDECL(merge_choice, (struct obj *,struct obj *));
E int FDECL(merged, (struct obj **,struct obj **));
#ifdef USE_TRAMPOLI
E int FDECL(ckunpaid, (struct obj *));
#endif
E void FDECL(addinv_core1, (struct obj *));
E void FDECL(addinv_core2, (struct obj *));
E struct obj *FDECL(addinv, (struct obj *));
E struct obj *FDECL(hold_another_object,
			(struct obj *,const char *,const char *,const char *));
E void FDECL(useupall, (struct obj *));
E void FDECL(useup, (struct obj *));
E void FDECL(consume_obj_charge, (struct obj *,BOOLEAN_P));
E void FDECL(freeinv_core, (struct obj *));
E void FDECL(freeinv, (struct obj *));
E void FDECL(m_freeinv, (struct obj *));
E void FDECL(delallobj, (int,int));
E void FDECL(delobj, (struct obj *));
E struct obj *FDECL(sobj_at, (int,int,int));
E struct obj *FDECL(boulder_at, (int,int));
E struct obj *FDECL(toustefna_at, (int,int));
E struct obj *FDECL(dreprun_at, (int,int));
E struct obj *FDECL(veioistafur_at, (int,int));
E struct obj *FDECL(thjofastafur_at, (int,int));
E struct obj *FDECL(fear_arti_at, (int, int));
E struct obj *FDECL(carrying, (int));
E char NDECL(carrying_applyable_ring);
E char NDECL(carrying_applyable_amulet);
E char NDECL(carrying_applyable_gem);
E char NDECL(carrying_readable_weapon);
E char NDECL(carrying_readable_tool);
E char NDECL(carrying_readable_armor);
E struct obj *FDECL(carrying_art, (int));
E boolean NDECL(have_lizard);
E struct obj *FDECL(o_on, (unsigned int,struct obj *));
E boolean FDECL(obj_here, (struct obj *,int,int));
E boolean NDECL(wearing_armor);
E boolean FDECL(is_worn_no_flags, (struct obj *));
E boolean FDECL(is_worn, (struct obj *, int));
E struct obj *FDECL(g_at, (int,int));
E struct obj *FDECL(mkgoldobj, (long));
E struct obj *FDECL(getobj, (const char *,const char *));
E int FDECL(ggetobj, (const char *,int (*)(OBJ_P),int,BOOLEAN_P,unsigned *));
E void FDECL(fully_identify_obj, (struct obj *));
E int FDECL(identify, (struct obj *));
E void FDECL(identify_pack, (int));
E int FDECL(askchain, (struct obj **,const char *,int,int (*)(OBJ_P),
			int (*)(OBJ_P),int,const char *));
E void FDECL(prinv, (const char *,struct obj *,long));
E char *FDECL(xprname, (struct obj *,const char *,CHAR_P,BOOLEAN_P,long,long));
E int NDECL(ddoinv);
E void FDECL(describe_item, (struct obj *, int, int, winid *));
E char FDECL(display_inventory, (const char *,BOOLEAN_P));
#ifdef DUMP_LOG
E char FDECL(dump_inventory, (const char *,BOOLEAN_P,BOOLEAN_P));
#endif
E int FDECL(display_binventory, (int,int,BOOLEAN_P));
E struct obj *FDECL(display_cinventory,(struct obj *));
E struct obj *FDECL(display_minventory,(struct monst *,int,char *));
E int NDECL(dotypeinv);
E const char *FDECL(dfeature_at, (int,int,char *));
E int FDECL(look_here, (int,BOOLEAN_P));
E int NDECL(dolook);
E boolean FDECL(will_feel_cockatrice, (struct obj *,BOOLEAN_P));
E void FDECL(feel_cockatrice, (struct obj *,BOOLEAN_P));
E void FDECL(stackobj, (struct obj *));
E int NDECL(doprgold);
E int NDECL(doprwep);
E int NDECL(doprarm);
E int NDECL(doprring);
E int NDECL(dopramulet);
E int NDECL(doprtool);
E int NDECL(doprinuse);
E void FDECL(useupf, (struct obj *,long));
E char *FDECL(let_to_name, (CHAR_P,BOOLEAN_P,BOOLEAN_P));
E char *NDECL(rand_class_name);
E void NDECL(free_invbuf);
E void NDECL(reassign);
E int NDECL(doorganize);
E int FDECL(count_unpaid, (struct obj *));
E boolean FDECL(has_object_type, (struct obj *, int));
E struct obj *FDECL(find_object_type, (struct obj *, int));
E struct obj *FDECL(find_charged_object_type, (struct obj *, int));
E int FDECL(count_buc, (struct obj *,int));
E void FDECL(carry_obj_effects, (struct obj *));
E const char *FDECL(currency, (long));
E void FDECL(silly_thing, (const char *,struct obj *));
E struct obj *NDECL(getnextgetobj);
#ifdef SORTLOOT
E int FDECL(sortloot_cmp, (struct obj *, struct obj *));
#endif
E int FDECL(mon_healing_penalty, (struct monst *));
E int NDECL(u_healing_penalty);
E int NDECL(u_clothing_discomfort);
E struct obj * FDECL(outermost_armor, (struct monst *));
E struct obj * NDECL(get_most_complete_puzzle);

/* ### ioctl.c ### */

#if defined(UNIX) || defined(__BEOS__)
E void NDECL(getwindowsz);
E void NDECL(getioctls);
E void NDECL(setioctls);
# ifdef SUSPEND
E int NDECL(dosuspend);
# endif /* SUSPEND */
#endif /* UNIX || __BEOS__ */

/* ### light.c ### */

E void FDECL(new_light_source, (int, genericptr_t, int));
E void FDECL(del_light_source, (struct ls_t *));
E void FDECL(do_light_sources, (char **));
E boolean NDECL(uswallow_indark);
E struct monst *FDECL(find_mid, (unsigned, unsigned));
E void FDECL(save_lightsource, (struct ls_t *, int, int));
E void FDECL(rest_lightsource, (int, genericptr_t, struct ls_t *, int, boolean));
E boolean NDECL(any_light_source);
E void FDECL(snuff_light_source, (int, int));
E boolean FDECL(obj_sheds_light, (struct obj *));
E boolean FDECL(obj_eternal_light, (struct obj *));
E boolean FDECL(obj_is_burning, (struct obj *));
E boolean FDECL(litsaber, (struct obj *));
E void FDECL(obj_split_light_source, (struct obj *, struct obj *));
E void FDECL(obj_merge_light_sources, (struct obj *,struct obj *));
E int FDECL(candle_light_range, (struct obj *));
#ifdef WIZARD
E int NDECL(wiz_light_sources);
#endif

/* ### lock.c ### */

#ifdef USE_TRAMPOLI
E int NDECL(forcelock);
E int NDECL(picklock);
#endif
E boolean NDECL(is_box_picking_context);
E boolean FDECL(picking_lock, (int *,int *));
E boolean FDECL(picking_at, (int,int));
E boolean FDECL(forcing_door, (int *, int*));
E void NDECL(reset_pick);
E struct obj *FDECL(autokey, (BOOLEAN_P));
E int FDECL(pick_lock, (struct obj *, XCHAR_P, XCHAR_P, struct obj *));
E int NDECL(doforce);
E boolean FDECL(boxlock, (struct obj *,struct obj *));
E boolean FDECL(doorlock, (struct obj *,int,int));
E int NDECL(doopen);
E int FDECL(doopen_indir, (int,int));
E int NDECL(doclose);
E int FDECL(artifact_door, (int,int));

#ifdef MAC
/* These declarations are here because the main code calls them. */

/* ### macfile.c ### */

E int FDECL(maccreat, (const char *,long));
E int FDECL(macopen, (const char *,int,long));
E int FDECL(macclose, (int));
E int FDECL(macread, (int,void *,unsigned));
E int FDECL(macwrite, (int,void *,unsigned));
E long FDECL(macseek, (int,long,short));
E int FDECL(macunlink, (const char *));

/* ### macsnd.c ### */

E void FDECL(mac_speaker, (struct obj *,char *));

/* ### macunix.c ### */

E void FDECL(regularize, (char *));
E void NDECL(getlock);

/* ### macwin.c ### */

E void FDECL(lock_mouse_cursor, (Boolean));
E int NDECL(SanePositions);

/* ### mttymain.c ### */

E void FDECL(getreturn, (char *));
E void VDECL(msmsg, (const char *,...));
E void NDECL(gettty);
E void NDECL(setftty);
E void FDECL(settty, (const char *));
E int NDECL(tgetch);
E void FDECL(cmov, (int x, int y));
E void FDECL(nocmov, (int x, int y));

#endif /* MAC */

/* ### mail.c ### */

E void NDECL(server_admin_msg);
#ifdef MAIL
# ifdef UNIX
E void NDECL(getmailstatus);
# endif
E void NDECL(ckmailstatus);
E void FDECL(readmail, (struct obj *));
#endif /* MAIL */
E void NDECL(ck_server_admin_msg);

/* ### makemon.c ### */

E void FDECL(m_initgrp,(struct monst *,int,int,int));
E boolean FDECL(is_home_elemental, (struct permonst *));
E struct monst *FDECL(clone_mon, (struct monst *,XCHAR_P,XCHAR_P));
E struct monst *NDECL(cloneu);
E struct monst *FDECL(makemon, (struct permonst *,int,int,int));
E struct monst *FDECL(makemon_full, (struct permonst *,int,int,int,int,int));
E struct monst *FDECL(makemon_core, (struct permonst *,int,int,int,int,int));
E boolean FDECL(create_critters, (int,struct permonst *));
E struct permonst *FDECL(rndmonst, (int, int));
E int FDECL(rndshape, (boolean(*)(int)));
E void FDECL(reset_rndmonst, (int));
E struct permonst *FDECL(mkclass, (CHAR_P,int));
E struct permonst *NDECL(mkzombie);
E int FDECL(adj_lev, (struct permonst *));
E void FDECL(m_level_up_intrinsic, (struct monst *));
E struct permonst *FDECL(grow_up, (struct monst *,struct monst *));
E struct obj * FDECL(mongets, (struct monst *,int,int));
E void FDECL(m_initthrow,(struct monst *, int, int, int));
E int FDECL(golemhp, (int));
E boolean FDECL(peace_minded, (struct monst *));
E void FDECL(set_malign, (struct monst *));
E void FDECL(set_mimic_sym, (struct monst *));
E void FDECL(set_curhouse, (int));
E int FDECL(mbirth_limit, (int));
E void FDECL(mimic_hit_msg, (struct monst *, SHORT_P));
#ifdef GOLDOBJ
E void FDECL(mkmonmoney, (struct monst *, long));
#endif
E int FDECL(bagotricks, (struct obj *, boolean, int *));
E long NDECL(init_doll_sales);
E boolean FDECL(propagate, (int, BOOLEAN_P,BOOLEAN_P));
E boolean FDECL(mon_can_see_you, (struct monst *));
E boolean FDECL(mon_can_see_mon, (struct monst *, struct monst *));
E int FDECL(permonst_max_lev,(struct permonst *));
E int FDECL(mon_max_lev,(struct monst *));

/* ### mapglyph.c ### */

E void FDECL(mapglyph, (int, glyph_t *, int *, unsigned *, int, int));

/* ### mcastu.c ### */

E void FDECL(cursetxt, (struct monst *, struct monst *, boolean));
E int FDECL(xcasty, (struct monst *, struct monst *, struct attack *, int, int, int));
E int FDECL(cast_spell, (struct monst *, struct monst *, struct attack *, int, int, int));
E int FDECL(needs_familiar, (struct monst *));
E int FDECL(pick_tannin, (struct monst *));
E void FDECL(dream_wolves, (int, int));

/* ### mextra.c ### */
E void FDECL(add_mx, (struct monst *, int));
E void FDECL(add_mx_l, (struct monst *, int, long));
E void FDECL(rem_mx, (struct monst *, int));
E void FDECL(rem_all_mx, (struct monst *));
E void FDECL(cpy_mx, (struct monst *, struct monst *, int));
E void FDECL(mov_mx, (struct monst *, struct monst *, int));
E void FDECL(mov_all_mx, (struct monst *, struct monst *));
E void * FDECL(get_mx, (struct monst *, int));
E long FDECL(siz_mx, (struct monst *, int));
E void * FDECL(bundle_mextra, (struct monst *, long *));
E void FDECL(unbundle_mextra, (struct monst *, void *));
E void FDECL(save_mextra, (struct monst *, int, int));
E void FDECL(rest_mextra, (struct monst *, int, boolean));
E void FDECL(relink_mx, (struct monst *));

/* out of order, but it makes sense to directly follow mextra */
/* ### oextra.c ### */
E void FDECL(add_ox, (struct obj *, int));
E void FDECL(add_ox_l, (struct obj *, int, long));
E void FDECL(rem_ox, (struct obj *, int));
E void FDECL(rem_all_ox, (struct obj *));
E void FDECL(cpy_ox, (struct obj *, struct obj *, int));
E void FDECL(mov_ox, (struct obj *, struct obj *, int));
E void FDECL(mov_all_ox, (struct obj *, struct obj *));
E void * FDECL(get_ox, (struct obj *, int));
E long FDECL(siz_ox, (struct obj *, int));
E void * FDECL(bundle_oextra, (struct obj *, long *));
E void FDECL(unbundle_oextra, (struct obj *, void *));
E void FDECL(save_oextra, (struct obj *, int, int));
E void FDECL(rest_oextra, (struct obj *, int, boolean));
E void FDECL(relink_ox, (struct obj *));

/* ### minion.c ### */

E void FDECL(msummon, (struct monst *, struct permonst *));
E struct monst * FDECL(summon_god_minion, (int,BOOLEAN_P));
E struct monst * FDECL(summon_minion, (ALIGNTYP_P,BOOLEAN_P,BOOLEAN_P,BOOLEAN_P));
E int FDECL(demon_talk, (struct monst *));
E long FDECL(bribe, (struct monst *));
E int FDECL(dprince, (struct permonst *, ALIGNTYP_P));
E int FDECL(dlord, (struct permonst *, ALIGNTYP_P));
E int NDECL(llord);
E int NDECL(nlord);
E int NDECL(clord);
E int FDECL(ndemon, (ALIGNTYP_P));
E int NDECL(lminion);
E int NDECL(nminion);
E int NDECL(cminion);

/* ### mklev.c ### */

#ifdef USE_TRAMPOLI
E int FDECL(do_comp, (genericptr_t,genericptr_t));
#endif
E void NDECL(sort_rooms);
E void FDECL(add_room, (int,int,int,int,BOOLEAN_P,SCHAR_P,BOOLEAN_P));
E void FDECL(add_subroom, (struct mkroom *,int,int,int,int,
			   BOOLEAN_P,SCHAR_P,BOOLEAN_P));
E void NDECL(makecorridors);
E int FDECL(add_door, (int,int,struct mkroom *));
E void FDECL(add_altar, (int, int, aligntyp, boolean, int));
E int NDECL(random_special_room);
E void NDECL(mkpoolroom);
E void NDECL(mkslabroom);
E void NDECL(mkhellvaultroom);
E void NDECL(mklev);
#ifdef SPECIALIZATION
E void FDECL(topologize, (struct mkroom *,BOOLEAN_P));
#else
E void FDECL(topologize, (struct mkroom *));
#endif
E void FDECL(place_branch, (branch *,XCHAR_P,XCHAR_P));
E boolean FDECL(occupied, (XCHAR_P,XCHAR_P));
E int FDECL(okdoor, (XCHAR_P,XCHAR_P));
E void FDECL(dodoor, (int,int,struct mkroom *));
E void FDECL(mktrap, (int,int,struct mkroom *,coord*));
E struct mkroom *FDECL(room_at, (XCHAR_P, XCHAR_P));
E void FDECL(mkstairs, (XCHAR_P,XCHAR_P,CHAR_P,struct mkroom *));
E boolean FDECL(mkfeature, (int, int, struct mkroom *));
E void NDECL(mkinvokearea);
E void FDECL(mkHVstatue, (int, int, int));

/* ### mkmap.c ### */

void FDECL(flood_fill_rm, (int,int,int,BOOLEAN_P,BOOLEAN_P));
void FDECL(remove_rooms, (int,int,int,int,SCHAR_P));
void FDECL(remove_room, (unsigned));

/* ### mkmaze.c ### */

E void FDECL(wallification, (int,int,int,int));
E void FDECL(walkfrom, (int,int,int));
E void FDECL(makemaz, (const char *));
E void FDECL(mazexy, (coord *));
E void NDECL(bound_digging);
E void FDECL(mkportal, (XCHAR_P,XCHAR_P,int,int));
E boolean FDECL(bad_location, (XCHAR_P,XCHAR_P,XCHAR_P,XCHAR_P,XCHAR_P,XCHAR_P));
E void FDECL(place_lregion, (XCHAR_P,XCHAR_P,XCHAR_P,XCHAR_P,
			     XCHAR_P,XCHAR_P,XCHAR_P,XCHAR_P,
			     XCHAR_P,d_level *));
E void FDECL(maze_add_rooms, (int, int));
E void FDECL(maze_add_openings, (int));
E void FDECL(maze_remove_deadends, (int, int));
E void FDECL(maze_damage_rooms, (int));
E void FDECL(maze_touchup_rooms, (int));
E void FDECL(maze_remove_room, (int));
E void NDECL(movebubbles);
E void NDECL(water_friction);
E void FDECL(save_waterlevel, (int,int));
E void FDECL(restore_waterlevel, (int));
E const char *FDECL(waterbody_name, (XCHAR_P,XCHAR_P));

/* ### mkobj.c ### */

E struct obj *FDECL(mkobj_at, (CHAR_P,int,int,int));
E struct obj *FDECL(mksobj_at, (int,int,int,int));
E struct obj *FDECL(mkobj, (CHAR_P,int));
E int NDECL(rndmonnum);
E struct obj *FDECL(splitobj, (struct obj *,long));
E struct obj *FDECL(duplicate_obj, (struct obj *, boolean));
E void FDECL(replace_object, (struct obj *,struct obj *));
E void FDECL(bill_dummy_object, (struct obj *));
E struct obj *FDECL(mksobj, (int,int));
E int FDECL(bcsign, (struct obj *));
E void FDECL(set_obj_size, (struct obj *, int));
E void FDECL(set_obj_quan, (struct obj *, int));
E void FDECL(maybe_set_material, (struct obj *, int));
E void FDECL(rand_interesting_obj_material, (struct obj *));
E void FDECL(set_material, (struct obj *, int));
E void FDECL(set_material_gm, (struct obj *, int));
E int FDECL(weight, (struct obj *));
E struct obj *FDECL(mkgold, (long,int,int));
E struct obj *FDECL(mkgold_core, (long,int,int,boolean));
E struct obj *FDECL(mkcorpstat,
		(int,struct monst *,struct permonst *,int,int,BOOLEAN_P));
E struct obj *FDECL(obj_attach_mid, (struct obj *, unsigned));
E struct obj *FDECL(save_mtraits, (struct obj *, struct monst *));
E struct monst *FDECL(get_mtraits, (struct obj *, BOOLEAN_P));
E struct obj *FDECL(mk_tt_object, (int,int,int));
E struct obj *FDECL(mk_named_object,
			(int,struct permonst *,int,int,const char *));
E struct obj *FDECL(rnd_treefruit_at, (int, int));
E void FDECL(start_corpse_timeout, (struct obj *));
E void FDECL(bless, (struct obj *));
E void FDECL(unbless, (struct obj *));
E void FDECL(curse, (struct obj *));
E void FDECL(uncurse, (struct obj *));
E void FDECL(blessorcurse, (struct obj *,int));
E boolean FDECL(is_flammable, (struct obj *));
E boolean FDECL(is_rottable, (struct obj *));
E void FDECL(place_object, (struct obj *,int,int));
E void FDECL(remove_object, (struct obj *));
E void FDECL(uncontain, (struct obj *));
E void FDECL(discard_minvent, (struct monst *));
E void FDECL(obj_extract_self, (struct obj *));
E void FDECL(obj_extract_and_unequip_self, (struct obj *));
E void FDECL(extract_magic_chest_nobj, (struct obj *));
E void FDECL(extract_nobj, (struct obj *, struct obj **));
E void FDECL(extract_nexthere, (struct obj *, struct obj **));
E int FDECL(add_to_minv, (struct monst *, struct obj *));
E struct obj *FDECL(add_to_magic_chest, (struct obj *, int));
E struct obj *FDECL(add_to_container, (struct obj *, struct obj *));
E void FDECL(add_to_migration, (struct obj *));
E void FDECL(add_to_buried, (struct obj *));
E void FDECL(container_weight, (struct obj *));
E void FDECL(dealloc_obj, (struct obj *));
E int FDECL(hornoplenty, (struct obj *, boolean));
E void FDECL(obj_ice_effects, (int, int, BOOLEAN_P));
E long FDECL(peek_at_iced_corpse_age, (struct obj *));
E void FDECL(size_and_shape_to_fit, (struct obj *, struct monst *));
E void FDECL(doMaskStats, (struct obj *));
#ifdef WIZARD
E void NDECL(obj_sanity_check);
#endif

/* ### mkroom.c ### */

E boolean FDECL(isspacious, (struct mkroom *));
E void NDECL(mksepulcher);
E void NDECL(mkmivault);
E void NDECL(mkfingervault);
E void FDECL(mkmivaultitem,(struct obj *));
E struct obj * NDECL(mkjewel);
E struct obj * NDECL(mkyourblood);
E struct obj * FDECL(mkcolumnarcrystal,(int));
E struct obj * NDECL(mklolthvaultitem);
E struct obj * FDECL(mkhellvaultitem,(int));
E void FDECL(mkhellvaultitem_cnt,(struct obj *, int, boolean));
E void NDECL(place_lolth_vaults);
E void NDECL(place_chaos_forest_features);
E void NDECL(place_elfquest_forest_features);
E void NDECL(place_haunted_forest_features);
E void NDECL(place_drow_healer_features);
E void NDECL(place_law_features);
E void NDECL(place_neutral_features);
E void NDECL(place_sum_all_features);
E struct mkroom * FDECL(pick_room,(BOOLEAN_P, BOOLEAN_P));
E boolean FDECL(special_room_requires_full_walls, (int));
E void FDECL(mkroom, (int));
E void FDECL(fill_zoo, (struct mkroom *));
E boolean FDECL(nexttodoor, (int,int));
E boolean FDECL(has_dnstairs, (struct mkroom *));
E boolean FDECL(has_upstairs, (struct mkroom *));
E int FDECL(somex, (struct mkroom *));
E int FDECL(somey, (struct mkroom *));
E boolean FDECL(inside_room, (struct mkroom *,XCHAR_P,XCHAR_P));
E boolean FDECL(somexy, (struct mkroom *,coord *));
E void FDECL(mkundead, (coord *,BOOLEAN_P,int,long));
E void FDECL(mk_yellow_undead, (coord *,BOOLEAN_P,int,long));
E struct permonst *FDECL(courtmon,(int));
E struct permonst *NDECL(mivaultmon);
E void FDECL(save_rooms, (int));
E void FDECL(rest_rooms, (int));
E struct mkroom *FDECL(search_special, (SCHAR_P));
E struct monst *FDECL(prisoner,(int, int, int));

/* ### mon.c ### */

E int FDECL(undead_to_corpse, (int));
E int FDECL(genus, (int,int));
E int FDECL(pm_to_cham, (int));
E int FDECL(minliquid, (struct monst *));
E int NDECL(movemon);
E int FDECL(meatmetal, (struct monst *));
E int FDECL(meatgluttony, (struct monst *));
E int FDECL(meatobj, (struct monst *));
E void FDECL(mvanishobj, (struct monst *, int, int));
E void FDECL(mpickgold, (struct monst *));
E boolean FDECL(mpickstuff, (struct monst *,const char *));
E int FDECL(curr_mon_load, (struct monst *));
E int FDECL(max_mon_load, (struct monst *));
E boolean FDECL(can_carry, (struct monst *,struct obj *));
E int FDECL(mfndpos, (struct monst *,coord *,long *,long));
E long FDECL(mm_aggression, (struct monst *,struct monst *));
E long FDECL(mm_grudge, (struct monst *,struct monst *, boolean));
E boolean FDECL(monnear, (struct monst *,int,int));
E void NDECL(dmonsfree);
E int FDECL(mcalcmove, (struct monst*));
E void NDECL(mcalcdistress);
E void FDECL(replmon, (struct monst *,struct monst *));
E void FDECL(relmon, (struct monst *));
E struct obj *FDECL(mlifesaver, (struct monst *));
E boolean FDECL(corpse_chance,(struct monst *,struct monst *,BOOLEAN_P));
E void FDECL(mondead, (struct monst *));
E void FDECL(spore_dies, (struct monst *));
E void FDECL(mondied, (struct monst *));
E void FDECL(monvanished, (struct monst *));
E void FDECL(mongone, (struct monst *));
E void FDECL(monslime, (struct monst *));
E void FDECL(monstone, (struct monst *));
E void FDECL(mongolded, (struct monst *));
E void FDECL(monglassed, (struct monst *));
E void FDECL(monkilled, (struct monst *,const char *,int));
E void FDECL(unstuck, (struct monst *));
E boolean FDECL(m_losehp, (struct monst *, int, boolean, char*));
E void FDECL(killed, (struct monst *));
E void FDECL(xkilled, (struct monst *,int));
E void FDECL(mon_to_stone, (struct monst*));
E void FDECL(mon_to_gold, (struct monst*));
E void FDECL(mnexto, (struct monst *));
E void FDECL(monline, (struct monst *));
E void FDECL(mofflin, (struct monst *));
E void FDECL(mofflin_close, (struct monst *));
E boolean FDECL(mnearto, (struct monst *,XCHAR_P,XCHAR_P,BOOLEAN_P));
E void FDECL(poisontell, (int));
E void FDECL(poisoned, (const char *,int,const char *,int, boolean));
E void FDECL(m_respond, (struct monst *));
E void FDECL(m_widegaze, (struct monst *));
E void FDECL(setmangry, (struct monst *));
E void FDECL(wakeup, (struct monst *, int));
E void NDECL(wake_nearby);
E void NDECL(wake_nearby_noisy);
E void FDECL(wake_nearto, (int,int,int));
E void FDECL(wake_nearto_noisy, (int,int,int));
E void FDECL(seemimic_ambush, (struct monst *));
E void FDECL(seemimic, (struct monst *));
E void FDECL(see_passive_mimic, (struct monst *));
E void NDECL(rescham);
E void NDECL(restartcham);
E void FDECL(restore_cham, (struct monst *));
E void FDECL(mon_animal_list, (BOOLEAN_P));
E int FDECL(newcham, (struct monst *,int,BOOLEAN_P,BOOLEAN_P));
E boolean FDECL(pick_animal, (int));
E int FDECL(select_newcham_form, (struct monst *));
E int FDECL(can_be_hatched, (int));
E int FDECL(egg_type_from_parent, (int,BOOLEAN_P));
E boolean FDECL(dead_species, (int,BOOLEAN_P));
E void NDECL(kill_genocided_monsters);
E void FDECL(golemeffects, (struct monst *,int,int));
E void FDECL(bud_metroid, (struct monst *));
E boolean FDECL(angry_guards, (BOOLEAN_P));
E void NDECL(pacify_guards);
E void FDECL(removeMonster,(int,int));
E int FDECL(u_sanity_loss,(struct monst *));
E int FDECL(u_sanity_loss_minor,(struct monst *));
E int NDECL(u_sanity_loss_nyar);
E int FDECL(u_sanity_gain,(struct monst *));
E int FDECL(u_insight_gain,(struct monst *));
E int FDECL(u_visible_insight,(struct monst *));
E void FDECL(repair,(struct monst *, struct monst *, int));
E void FDECL(nurse_heal,(struct monst *, struct monst *, int));
E void FDECL(itiner_heal,(struct monst *, struct monst *, int));
E void FDECL(insight_vanish,(struct monst *));
E long FDECL(timer_duration_remaining,(timer_element *));
E void FDECL(adjust_timer_duration, (timer_element *, long));
E timer_element * FDECL(get_timer,(timer_element *, short));
E void FDECL(abjure_summon, (struct monst *, int));
E void FDECL(mark_mon_as_summoned,(struct monst *, struct monst *, int, int));
E struct monst * FDECL(mtyp_on_level,(int));
E struct monst * FDECL(mtyp_on_migrating,(int));

/* ### mondata.c ### */

E void NDECL(id_permonst);
E void FDECL(set_mcan, (struct monst *,boolean));
E void FDECL(set_mon_data, (struct monst *,int));
E void FDECL(set_mon_data_core, (struct monst *, struct permonst *));
E void FDECL(make_horror, (struct permonst *, int, int));
E void FDECL(give_mintrinsic, (struct monst *, long));
E void FDECL(remove_mintrinsic, (struct monst *, long));
E void FDECL(set_faction, (struct monst *, int));
E void FDECL(set_template, (struct monst *, int));
E boolean FDECL(mtemplate_accepts_mtyp, (int, int));
E struct permonst * FDECL(permonst_of, (int, int));
E struct attack *FDECL(permonst_dmgtype, (struct permonst *,int));
E struct attack *FDECL(attacktype_fordmg, (struct permonst *,int,int));
E boolean FDECL(at_least_one_attack, (struct monst *));
E boolean FDECL(attacktype, (struct permonst *,int));
E boolean FDECL(mon_attacktype, (struct monst *,int));
E boolean FDECL(mon_offhand_attack, (struct monst *));
E boolean FDECL(cantwield, (struct monst *));
E boolean FDECL(you_cantwield, (struct permonst *));
E int FDECL(mon_count_attacktype, (struct monst *,int));
E struct attack *FDECL(mon_get_attacktype, (struct monst *,int, struct attack *));
E boolean FDECL(noattacks, (struct permonst *));
E int FDECL(attackindex, (struct permonst *,int,int));
E boolean FDECL(poly_when_stoned, (struct permonst *));
E boolean FDECL(poly_when_golded, (struct permonst *));
E boolean FDECL(resists_drli, (struct monst *));
E boolean FDECL(resists_oona, (struct monst *));
E boolean FDECL(resists_fire, (struct monst *));
E boolean FDECL(resists_cold, (struct monst *));
E boolean FDECL(resists_sleep, (struct monst *));
E boolean FDECL(resists_disint, (struct monst *));
E boolean FDECL(resists_elec, (struct monst *));
E boolean FDECL(resists_poison, (struct monst *));
E boolean FDECL(resists_acid, (struct monst *));
E boolean FDECL(Slime_res, (struct monst *));
E boolean FDECL(resists_ston, (struct monst *));
E boolean FDECL(resists_drain, (struct monst *));
E boolean FDECL(resists_sickness, (struct monst *));
E boolean FDECL(resists_magm, (struct monst *));
E boolean FDECL(resists_death, (struct monst *));
E boolean FDECL(resists_blnd, (struct monst *));
E boolean FDECL(can_blnd, (struct monst *,struct monst *,UCHAR_P,struct obj *));
E int FDECL(m_martial_skill, (struct permonst *));
E boolean FDECL(ranged_attk, (struct permonst *));
E boolean FDECL(passes_bars, (struct monst *));
E boolean FDECL(can_track, (struct permonst *));
E boolean FDECL(sticks, (struct monst *));
E int FDECL(num_horns, (struct permonst *));
/* E boolean FDECL(canseemon, (struct monst *)); */
E struct attack *FDECL(dmgtype_fromattack, (struct permonst *,int,int));
E boolean FDECL(dmgtype, (struct permonst *,int));
E int FDECL(max_passive_dmg, (struct monst *,struct monst *));
E int FDECL(monsndx, (struct permonst *));
E int FDECL(name_to_mon, (const char *));
E int FDECL(gender, (struct monst *));
E int FDECL(pronoun_gender, (struct monst *));
E boolean FDECL(levl_follower, (struct monst *));
E int FDECL(little_to_big, (int,int));
E int FDECL(big_to_little, (int));
E const char *FDECL(locomotion, (struct monst *,const char *));
E const char *FDECL(stagger, (struct monst *,const char *));
E const char *FDECL(on_fire, (struct permonst *,struct attack *));
E const struct permonst *FDECL(raceptr, (struct monst *));
E int FDECL(mstrength, (struct permonst *));
E long FDECL(mon_str, (struct monst *));
E long FDECL(mon_con, (struct monst *));
E boolean FDECL(hiddenwidegaze, (struct monst *));
E int FDECL(hd_size, (struct permonst *));
E void FDECL(do_ancient_breaths, (struct monst *));
E void FDECL(do_auras, (struct monst *));

/* ### monmove.c ### */

E boolean FDECL(scaryElb, (struct monst *));
E boolean FDECL(scaryLol, (struct monst *));
E boolean FDECL(scaryItem, (struct monst *));
E boolean FDECL(scaryYellow, (int, struct monst *));
E boolean FDECL(scaryHept, (int, struct monst *));
E boolean FDECL(scaryGorg, (int, struct monst *));
E boolean FDECL(scaryCircle, (int, struct monst *));
E boolean FDECL(scaryPent, (int, struct monst *));
E boolean FDECL(scaryHex, (int, struct monst *));
E boolean FDECL(scaryHam, (int, struct monst *));
E boolean FDECL(scarySign, (int, struct monst *));
E boolean FDECL(scaryEye, (int, struct monst *));
E boolean FDECL(scaryQueen, (int, struct monst *));
E boolean FDECL(scaryCat, (int, struct monst *));
E boolean FDECL(scaryWings, (int, struct monst *));
E boolean FDECL(scaryTou, (struct monst *));
E boolean FDECL(scaryDre, (struct monst *));
E boolean FDECL(scaryVei, (struct monst *));
E boolean FDECL(scaryThj, (struct monst *));
E boolean FDECL(itsstuck, (struct monst *));
E boolean FDECL(mb_trapped, (struct monst *));
E void FDECL(mon_regen, (struct monst *,BOOLEAN_P));
E void FDECL(timeout_problems, (struct monst *));
E int FDECL(dochugw, (struct monst *));
E boolean FDECL(onscary, (int,int,struct monst *));
E void FDECL(monflee, (struct monst *, int, BOOLEAN_P, BOOLEAN_P));
E void FDECL(dracae_eladrin_spawn_equip, (struct monst *, int));
E int FDECL(dochug, (struct monst *));
E int FDECL(m_move, (struct monst *,int));
E void FDECL(break_iron_bars, (int, int, boolean));
E void FDECL(dissolve_bars, (int,int));
E boolean FDECL(closed_door, (int,int));
E boolean FDECL(accessible, (int,int));
E void FDECL(set_apparxy, (struct monst *));
E boolean FDECL(can_ooze, (struct monst *));
E void FDECL(remove_monster, (int, int));
E void FDECL(held_item_bites, (struct monst *, struct obj *));
E void FDECL(add_byakhee_to_obj, (struct obj *));
E boolean FDECL(likes_obj, (struct monst *, struct obj *));
E boolean FDECL(can_equip, (struct monst *, struct obj *));
E void FDECL(phantom_scorpions_sting, (struct monst *));
E void FDECL(rot_caterpillars_bite, (struct monst *));
E void FDECL(orc_mud_stabs, (struct monst *));

/* ### monst.c ### */

E void NDECL(monst_init);

/* ### monstr.c ### */

E void NDECL(monstr_init);

/* ### mplayer.c ### */

E struct monst *FDECL(mk_mplayer, (struct permonst *,XCHAR_P, XCHAR_P,long));
E void FDECL(create_mplayers, (int,BOOLEAN_P));
E void FDECL(mplayer_talk, (struct monst *));

#if defined(MICRO) || defined(WIN32)

/* ### msdos.c,os2.c,tos.c,winnt.c ### */

#  ifndef WIN32
E int NDECL(tgetch);
#  endif
#  ifndef TOS
E char NDECL(switchar);
#  endif
# ifndef __GO32__
E long FDECL(freediskspace, (char *));
#  ifdef MSDOS
E int FDECL(findfirst_file, (char *));
E int NDECL(findnext_file);
E long FDECL(filesize_nh, (char *));
#  else
E int FDECL(findfirst, (char *));
E int NDECL(findnext);
E long FDECL(filesize, (char *));
#  endif /* MSDOS */
E char *NDECL(foundfile_buffer);
# endif /* __GO32__ */
E void FDECL(chdrive, (char *));
# ifndef TOS
E void NDECL(disable_ctrlP);
E void NDECL(enable_ctrlP);
# endif
# if defined(MICRO) && !defined(WINNT)
E void NDECL(get_scr_size);
#  ifndef TOS
E void FDECL(gotoxy, (int,int));
#  endif
# endif
# ifdef TOS
E int FDECL(_copyfile, (char *,char *));
E int NDECL(kbhit);
E void NDECL(set_colors);
E void NDECL(restore_colors);
#  ifdef SUSPEND
E int NDECL(dosuspend);
#  endif
# endif /* TOS */
# ifdef WIN32
E char *FDECL(get_username, (int *));
E void FDECL(nt_regularize, (char *));
E int NDECL((*nt_kbhit));
E void FDECL(Delay, (int));
# endif /* WIN32 */
#endif /* MICRO || WIN32 */

/* ### mthrowu.c ### */

E int FDECL(set_bypassDR, (int));
E int FDECL(set_destroy_thrown, (int));
//E int FDECL(thitu, (int,int,struct obj *,const char *,BOOLEAN_P));
//E int FDECL(ohitmon, (struct monst *,struct monst *,struct obj *,int,BOOLEAN_P));
//E void FDECL(thrwmu, (struct monst *));
//E int FDECL(spitmu, (struct monst *,struct attack *));
//E int FDECL(firemu, (struct monst *,struct attack *));
//E int FDECL(firemm, (struct monst *,struct monst *,struct attack *));
/*E int FDECL(firemm, (struct monst *,struct attack *));*/
//E int FDECL(spitmm, (struct monst *,struct monst *,struct attack *));
//E int FDECL(breamu, (struct monst *,struct attack *));
//E int FDECL(breamm, (struct monst *,struct monst *,struct attack *));
E boolean FDECL(mtarget_adjacent, (struct monst *));
E struct monst *FDECL(mfind_target, (struct monst *, boolean, boolean));
E boolean FDECL(linedup, (XCHAR_P,XCHAR_P,XCHAR_P,XCHAR_P));
E boolean FDECL(lined_up, (struct monst *));
E int FDECL(m_pole_range, (struct monst *));
E struct obj *FDECL(m_carrying, (struct monst *,int));
E struct obj *FDECL(m_carrying_charged, (struct monst *,int));
E void FDECL(m_useup, (struct monst *,struct obj *));
E void FDECL(m_throw, (struct monst *,int,int,int,int,int,struct obj *,BOOLEAN_P));
E boolean FDECL(hits_bars, (struct obj **,int,int,int,int));

/* ### muse.c ### */

E boolean FDECL(find_defensive, (struct monst *));
E int FDECL(use_defensive, (struct monst *));
E int FDECL(rnd_defensive_item, (struct monst *));
E boolean FDECL(find_offensive, (struct monst *));
E struct permonst * FDECL(find_mask, (struct monst *));
#ifdef USE_TRAMPOLI
E int FDECL(mbhitm, (struct monst *,struct obj *));
#endif
E int FDECL(use_offensive, (struct monst *));
E int FDECL(rnd_offensive_item, (struct monst *));
E boolean FDECL(is_attack_wand, (int));
E int FDECL(rnd_attack_wand, (struct monst *));
E int FDECL(rnd_attack_potion, (struct monst *));
E int FDECL(rnd_utility_wand, (struct monst *));
E int FDECL(rnd_utility_potion, (struct monst *));
E int NDECL(rnd_good_amulet);
E boolean FDECL(find_misc, (struct monst *));
E int FDECL(use_misc, (struct monst *));
E int FDECL(rnd_misc_item, (struct monst *));
E boolean FDECL(searches_for_item, (struct monst *,struct obj *));
E boolean FDECL(mon_reflects, (struct monst *,const char *));
E boolean FDECL(ureflects, (const char *,const char *));
E boolean FDECL(munstone, (struct monst *,BOOLEAN_P));
E int FDECL(mbreak_entanglement, (struct monst *));
E int FDECL(mescape_entanglement, (struct monst *));
E int FDECL(entangle_material, (struct monst *, int));
E int FDECL(entangle_beatitude, (struct monst *, int));

/* ### music.c ### */

E void FDECL(song_noise,(int));
E void NDECL(awaken_soldiers);
E void FDECL(do_earthquake, (int, int, int, int,BOOLEAN_P,struct monst *));
E int FDECL(do_play_instrument, (struct obj *));
#ifdef BARD
E int FDECL(pet_can_sing, (struct monst *,BOOLEAN_P));
E int FDECL(mon_can_counter_sing, (struct monst *,BOOLEAN_P));
E int FDECL(resist_song, (struct monst *, int, struct obj *));
#endif

/* ### mutation.c ### */
E void FDECL(confer_mutation, (int));
E boolean NDECL(any_mutation);

/* ### nhlan.c ### */
#ifdef LAN_FEATURES
E void NDECL(init_lan_features);
E char *NDECL(lan_username);
# ifdef LAN_MAIL
E boolean NDECL(lan_mail_check);
E void FDECL(lan_mail_read, (struct obj *));
E void NDECL(lan_mail_init);
E void NDECL(lan_mail_finish);
E void NDECL(lan_mail_terminate);
# endif
#endif

/* ### nttty.c ### */

#ifdef WIN32CON
E void NDECL(get_scr_size);
E int NDECL(nttty_kbhit);
E void NDECL(nttty_open);
E void NDECL(nttty_rubout);
E int NDECL(tgetch);
E int FDECL(ntposkey,(int *, int *, int *));
E void FDECL(set_output_mode, (int));
E void NDECL(synch_cursor);
#endif

/* ### o_init.c ### */

E void NDECL(init_objects);
E void NDECL(hack_objects);
E int NDECL(find_sawant);
E int NDECL(find_gawant);
E int NDECL(find_pawant);
E int NDECL(find_riwant);
E int NDECL(find_gcirclet);
E int NDECL(find_sring);
E int NDECL(find_iring);
E int NDECL(find_good_iring);
E int NDECL(find_good_dring);
E int NDECL(find_good_fring);
E int NDECL(find_good_wring);
E int NDECL(find_good_aring);
E int NDECL(find_vhelm);
E int NDECL(find_skates);
E int NDECL(find_cboots);
E int NDECL(find_mboots);
E int NDECL(find_hboots);
E int NDECL(find_bboots);
E int NDECL(find_jboots);
E int NDECL(find_rboots);
E int NDECL(find_cope);
E int NDECL(find_opera_cloak);
E int NDECL(find_pcloth);
E int NDECL(find_signet_ring);
E int NDECL(find_engagement_ring);
E int NDECL(find_gold_ring);
E int NDECL(find_silver_ring);
E int NDECL(find_opal_ring);
E int NDECL(find_clay_ring);
E int NDECL(find_coral_ring);
E int NDECL(find_onyx_ring);
E int NDECL(find_moonstone_ring);
E int NDECL(find_jade_ring);
E int NDECL(find_agate_ring);
E int NDECL(find_topaz_ring);
E int NDECL(find_sapphire_ring);
E int NDECL(find_ruby_ring);
E int NDECL(find_pearl_ring);
E int NDECL(find_ivory_ring);
E int NDECL(find_emerald_ring);
E int NDECL(find_droven_ring);
E boolean FDECL(isEngrRing, (int));
E boolean FDECL(isSignetRing, (int));
E int NDECL(find_golden_potion);
E int NDECL(find_cloth_book);
E int NDECL(find_leather_book);
E int NDECL(find_bronze_book);
E int NDECL(find_silver_book);
E int NDECL(find_gold_book);
E int FDECL(matWand, (int, int));
E int NDECL(find_ogloves);
E int NDECL(find_tgloves);
E int NDECL(find_pgloves);
E int NDECL(find_fgloves);
E int NDECL(find_rgloves);
E void NDECL(oinit);
E void FDECL(savenames, (int,int));
E void FDECL(restnames, (int));
E void FDECL(discover_object, (int,BOOLEAN_P,BOOLEAN_P));
E void FDECL(undiscover_object, (int));
E int NDECL(dodiscovered);
E void FDECL(set_isamusei_color, (struct obj *));
E void FDECL(set_object_color, (struct obj *));
E int FDECL(object_color, (struct obj *));
E void FDECL(set_submat, (struct obj *, int));
E void FDECL(fix_object, (struct obj *));

/* ### objects.c ### */

E void NDECL(objects_init);

/* ### objnam.c ### */

E const char *FDECL(lightsaber_colorText, (struct obj *));
E int FDECL(lightsaber_colorCLR, (struct obj *));
E char *FDECL(lightsaber_hiltText, (struct obj *));
E char *FDECL(beamsword_hiltText, (struct obj *));
E int NDECL(random_saber_hilt);
E int NDECL(random_beam_hilt);
E char *FDECL(obj_typename, (int));
E char *FDECL(obj_descname, (struct obj *));
E char *FDECL(simple_typename, (int));
E char *FDECL(artiadjusted_objnam, (char *, int));
E boolean FDECL(obj_is_pname, (struct obj *));
E char *FDECL(distant_name, (struct obj *,char *(*)(OBJ_P)));
E char *FDECL(fruitname, (BOOLEAN_P));
E char *FDECL(xname, (struct obj *));
E const char *FDECL(material_name, (struct obj *, boolean));
E char *FDECL(mshot_xname, (struct obj *));
E boolean FDECL(the_unique_obj, (struct obj *obj));
E char *FDECL(doname, (struct obj *));
E char *FDECL(doname_with_price, (struct obj *));
E boolean FDECL(not_fully_identified_dummy_flags, (struct obj *, int));
E boolean FDECL(not_fully_identified, (struct obj *));
E char *FDECL(corpse_xname, (struct obj *,BOOLEAN_P));
E char *FDECL(cxname, (struct obj *));
E char *FDECL(xname_bland, (struct obj *));
#ifdef SORTLOOT
E char *FDECL(cxname2, (struct obj *));
#endif
E char *FDECL(killer_xname, (struct obj *));
E const char *FDECL(singular, (struct obj *,char *(*)(OBJ_P)));
E char *FDECL(an, (const char *));
E char *FDECL(An, (const char *));
E char *FDECL(The, (const char *));
E char *FDECL(the, (const char *));
E char *FDECL(aobjnam, (struct obj *,const char *));
E char *FDECL(Tobjnam, (struct obj *,const char *));
E char *FDECL(otense, (struct obj *,const char *));
E char *FDECL(vtense, (const char *,const char *));
E char *FDECL(Doname2, (struct obj *));
E char *FDECL(yname, (struct obj *));
E char *FDECL(Yname2, (struct obj *));
E char *FDECL(ysimple_name, (struct obj *));
E char *FDECL(Ysimple_name2, (struct obj *));
E char *FDECL(makeplural, (const char *));
E char *FDECL(makesingular, (const char *));
E struct obj *FDECL(readobjnam, (char *,int *,int));
E int FDECL(rnd_class, (int,int));
E const char *FDECL(cloak_simple_name, (struct obj *));
E const char *FDECL(mimic_obj_name, (struct monst *));

/* ### options.c ### */

E boolean FDECL(match_optname, (const char *,const char *,int,BOOLEAN_P));
E void NDECL(initoptions);
E void FDECL(parseoptions, (char *,BOOLEAN_P,BOOLEAN_P));
E int NDECL(doset);
E int NDECL(dotogglepickup);
E void NDECL(option_help);
E void FDECL(next_opt, (winid,const char *));
E int FDECL(fruitadd, (char *));
E int FDECL(choose_classes_menu, (const char *,int,BOOLEAN_P,char *,char *));
E void FDECL(add_menu_cmd_alias, (CHAR_P, CHAR_P));
E char FDECL(map_menu_cmd, (CHAR_P));
E void FDECL(assign_warnings, (uchar *));
E char *FDECL(nh_getenv, (const char *));
E void FDECL(set_duplicate_opt_detection, (int));
E void FDECL(set_wc_option_mod_status, (unsigned long, int));
E void FDECL(set_wc2_option_mod_status, (unsigned long, int));
E void FDECL(set_option_mod_status, (const char *,int));
#ifdef AUTOPICKUP_EXCEPTIONS
E int FDECL(add_autopickup_exception, (const char *));
E void NDECL(free_autopickup_exceptions);
#endif /* AUTOPICKUP_EXCEPTIONS */
#ifdef MENU_COLOR
E boolean FDECL(add_menu_coloring, (char *));
#endif /* MENU_COLOR */
E boolean FDECL(parse_monster_color, (char *));
E boolean FDECL(parse_monster_template, (char *));
E int FDECL(parse_codepoint, (char *));
E boolean FDECL(parse_monster_symbol, (const char *));
E boolean FDECL(parse_object_symbol, (const char *));
E boolean FDECL(parse_symbol, (const char *));

/* ### pager.c ### */
E char *FDECL(flag_to_word, (unsigned long, int));
E int NDECL(dowhatis);
E int NDECL(doquickwhatis);
E int NDECL(doidtrap);
E int NDECL(dowhatdoes);
E boolean FDECL(object_from_map, (int,int,int,struct obj **));
E const char *NDECL(rndobjnam);
E char *FDECL(dowhatdoes_core,(CHAR_P, char *));
E int NDECL(dohelp);
E int NDECL(dohistory);
E boolean FDECL(checkfile, (char *,struct permonst *,BOOLEAN_P,BOOLEAN_P, winid*));
E char *FDECL(do_look_letter, (glyph_t, BOOLEAN_P, BOOLEAN_P, BOOLEAN_P, coord, char *, const char *));

/* ### pcmain.c ### */

#if defined(MICRO) || defined(WIN32)
# ifdef CHDIR
E void FDECL(chdirx, (char *,BOOLEAN_P));
# endif /* CHDIR */
#endif /* MICRO || WIN32 */

/* ### pcsys.c ### */

#if defined(MICRO) || defined(WIN32)
E void NDECL(flushout);
E int NDECL(dosh);
# ifdef MFLOPPY
E void FDECL(eraseall, (const char *,const char *));
E void FDECL(copybones, (int));
E void NDECL(playwoRAMdisk);
E int FDECL(saveDiskPrompt, (int));
E void NDECL(gameDiskPrompt);
# endif
E void FDECL(append_slash, (char *));
E void FDECL(getreturn, (const char *));
# ifndef AMIGA
E void VDECL(msmsg, (const char *,...));
# endif
E FILE *FDECL(fopenp, (const char *,const char *));
#endif /* MICRO || WIN32 */

/* ### pctty.c ### */

#if defined(MICRO) || defined(WIN32)
E void NDECL(gettty);
E void FDECL(settty, (const char *));
E void NDECL(setftty);
E void VDECL(error, (const char *,...));
#if defined(TIMED_DELAY) && defined(_MSC_VER)
E void FDECL(msleep, (unsigned));
#endif
#endif /* MICRO || WIN32 */

/* ### pcunix.c ### */

#if defined(MICRO)
E void FDECL(regularize, (char *));
#endif /* MICRO */
#if defined(PC_LOCKING)
E void NDECL(getlock);
#endif

/* ### pickup.c ### */

E void FDECL(observe_quantum_cat, (struct obj *, BOOLEAN_P));
E void FDECL(open_coffin, (struct obj *, BOOLEAN_P));
E void FDECL(open_sarcophagus, (struct obj *, BOOLEAN_P));
E void FDECL(open_crazy_box, (struct obj *, BOOLEAN_P));
E boolean FDECL(open_madstuff_box, (struct obj *, BOOLEAN_P));
E void FDECL(open_giants_sack, (struct obj *, BOOLEAN_P));
E void FDECL(kill_giants_sack, (struct obj *));
#ifdef GOLDOBJ
E int FDECL(collect_obj_classes,
	(char *,struct obj *,BOOLEAN_P,boolean FDECL((*),(OBJ_P)), int *));
#else
E int FDECL(collect_obj_classes,
	(char *,struct obj *,BOOLEAN_P,BOOLEAN_P,boolean FDECL((*),(OBJ_P)), int *));
#endif
E void FDECL(add_valid_menu_class, (int));
E boolean FDECL(allow_all_nomods, (struct obj *));
E boolean FDECL(allow_all, (struct obj *, int));
E boolean FDECL(allow_category, (struct obj *, int));
E boolean FDECL(is_worn_by_type, (struct obj *, int));
#ifdef USE_TRAMPOLI
E int FDECL(ck_bag, (struct obj *));
E int FDECL(in_container, (struct obj *));
E int FDECL(out_container, (struct obj *));
#endif
E int FDECL(pickup, (int));
E int FDECL(pickup_object, (struct obj *, long, BOOLEAN_P));
E int FDECL(query_category, (const char *, struct obj *, int,
				menu_item **, int));
E int FDECL(query_objlist, (const char *, struct obj *, int,
				menu_item **, int, boolean (*)(OBJ_P, int)));
E struct obj *FDECL(pick_obj, (struct obj *));
E int NDECL(encumber_msg);
E int NDECL(doloot);
E int NDECL(dotip);
E int NDECL(dopetequip);
E int FDECL(use_container, (struct obj *,int));
E int FDECL(use_massblaster, (struct obj *));
E int FDECL(use_demon_claw, (struct obj *));
E int FDECL(loot_mon, (struct monst *,int *,boolean *));
E const char *FDECL(safe_qbuf, (const char *,unsigned,
				const char *,const char *,const char *));
E boolean FDECL(is_autopickup_exception, (struct obj *, BOOLEAN_P));

/* ### pline.c ### */

E void FDECL(msgpline_add, (int, char *));
E void NDECL(msgpline_free);
E void VDECL(pline, (const char *,...)) PRINTF_F(1,2);
E void VDECL(Norep, (const char *,...)) PRINTF_F(1,2);
E void NDECL(free_youbuf);
E void VDECL(You, (const char *,...)) PRINTF_F(1,2);
E void VDECL(Your, (const char *,...)) PRINTF_F(1,2);
E void VDECL(You_feel, (const char *,...)) PRINTF_F(1,2);
E void VDECL(You_cant, (const char *,...)) PRINTF_F(1,2);
E void VDECL(You_hear, (const char *,...)) PRINTF_F(1,2);
E void VDECL(pline_The, (const char *,...)) PRINTF_F(1,2);
E void VDECL(There, (const char *,...)) PRINTF_F(1,2);
E void VDECL(verbalize, (const char *,...)) PRINTF_F(1,2);
E void VDECL(raw_printf, (const char *,...)) PRINTF_F(1,2);
E void VDECL(impossible, (const char *,...)) PRINTF_F(1,2);
E const char *FDECL(align_str, (ALIGNTYP_P));
E const char *FDECL(align_str_proper, (ALIGNTYP_P));
E void FDECL(mdrslotline, (struct monst *));
E void FDECL(mstatusline, (struct monst *));
E void NDECL(ustatusline);
E void NDECL(self_invis_message);
E const char *FDECL(piratesay, (const char *));

/* ### polyself.c ### */

E void NDECL(init_uasmon);
E void NDECL(set_uasmon);
E void NDECL(change_sex);
E void FDECL(polyself, (BOOLEAN_P));
E int FDECL(polymon, (int));
E void NDECL(rehumanize);
E int FDECL(dobreathe, (struct permonst *));
E int NDECL(domakewhisperer);
E int NDECL(dokiai);
E int NDECL(doelementalbreath);
E int NDECL(dospit);
E int NDECL(doremove);
E int NDECL(dospinweb);
E int NDECL(dosummon);
E int NDECL(dodemonpet);
E int NDECL(dovampminion);
E int NDECL(dotinker);
E int NDECL(dogaze);
E int NDECL(dohide);
E int NDECL(domindblast);
E int NDECL(dodarken);
E int NDECL(doandroid);
E int NDECL(doclockspeed);
E int FDECL(dowords, (int));
E void FDECL(skinback, (BOOLEAN_P));
E const char *FDECL(mbodypart, (struct monst *,int));
E const char *FDECL(ptrbodypart, (struct permonst *,int, struct monst *));
E const char *FDECL(body_part, (int));
E int NDECL(poly_gender);
E void FDECL(ugolemeffects, (int,int));

/* ### potion.c ### */

E void FDECL(set_itimeout, (long *,long));
E void FDECL(incr_itimeout, (long *,long));
E long FDECL(itimeout_incr, (long,long));
E void FDECL(make_confused, (long,BOOLEAN_P));
E void FDECL(make_stunned, (long,BOOLEAN_P));
E void FDECL(make_doubtful, (long,BOOLEAN_P));
E void FDECL(make_invulnerable, (long,BOOLEAN_P));
E void FDECL(make_blinded, (long,BOOLEAN_P));
E void FDECL(make_sick, (long, const char *, BOOLEAN_P,int));
E void FDECL(make_vomiting, (long,BOOLEAN_P));
E boolean FDECL(make_hallucinated, (long,BOOLEAN_P,long));
E int NDECL(dodrink);
E int FDECL(dopotion, (struct obj *, boolean));
E int FDECL(peffects, (struct obj *, boolean));
E void FDECL(healup, (int,int,BOOLEAN_P,BOOLEAN_P));
E void FDECL(strange_feeling, (struct obj *,const char *));
E void FDECL(potionhit, (struct monst *,struct obj *,BOOLEAN_P));
E void FDECL(potionbreathe, (struct obj *));
E boolean FDECL(get_wet, (struct obj *, BOOLEAN_P));
E int NDECL(dodip);
E void FDECL(djinni_from_bottle, (struct obj *));
E struct monst *FDECL(split_mon, (struct monst *,struct monst *));
E const char *NDECL(bottlename);

/* ### pray.c ### */
#ifdef USE_TRAMPOLI
E int NDECL(prayer_done);
#endif
E void FDECL(godvoice,(int,const char*));
E void FDECL(gods_angry,(int));
E void FDECL(gods_upset,(int));
E void FDECL(angrygods,(int));
E boolean NDECL(maybe_god_gives_gift);
E void NDECL(pacify_goat_faction);
E int NDECL(dosacrifice);
E void FDECL(at_your_feet, (const char *));
E boolean FDECL(can_pray, (BOOLEAN_P));
E int NDECL(dopray);
E int FDECL(turn_level, (struct monst *));
E const char *NDECL(u_gname);
E int NDECL(doturn);
E int FDECL(mon_doturn, (struct monst *));
E const char *NDECL(a_gname);
E const char *FDECL(a_gname_at, (XCHAR_P x,XCHAR_P y));
E const char *FDECL(align_gname, (ALIGNTYP_P));
E const char *FDECL(gtitle, (int));
E void FDECL(altar_wrath, (int,int));
E int FDECL(candle_on_altar, (struct obj *));
E void FDECL(goat_eat, (struct obj *, int));
E void FDECL(yog_sothoth_drink, (struct obj *));
E void FDECL(flame_consume, (struct monst *, struct obj *, boolean));
E void FDECL(yog_credit, (int, boolean));
E void FDECL(bokrug_offer, (struct obj *));
E boolean FDECL(goat_mouth_at, (int, int));
E boolean FDECL(bokrug_idol_at, (int, int));
E boolean FDECL(yog_altar_at, (int, int));
E int NDECL(commune_with_goat);
E int NDECL(commune_with_silver_flame);
E int NDECL(commune_with_yog);
E void NDECL(init_gods);
E void FDECL(save_gods, (int));
E void FDECL(restore_gods, (int));
E aligntyp FDECL(galign, (int));
E int FDECL(gholiness, (int));
E int FDECL(align_to_god, (aligntyp));
E const char * FDECL(godname_full, (int));
E const char * FDECL(godname, (int));
E int FDECL(god_faction, (int));
E int FDECL(altaralign_to_godnum, (int));
E const int * FDECL(god_minions, (int));
E struct monst * FDECL(god_priest, (int, int, int, int));
E int FDECL(god_at_altar, (int, int));
E boolean FDECL(gods_are_friendly, (int, int));
E boolean FDECL(god_accepts_you, (int));

/* ### priest.c ### */

E coord * FDECL(shrine_pos, (int));
E coord * FDECL(find_shrine_altar, (int));
E aligntyp FDECL(temple_alignment, (int));
E int FDECL(temple_god, (int));
E int FDECL(move_special, (struct monst *,BOOLEAN_P,SCHAR_P,BOOLEAN_P,BOOLEAN_P,
			   XCHAR_P,XCHAR_P,XCHAR_P,XCHAR_P));
E char FDECL(temple_occupied, (char *));
E int FDECL(pri_move, (struct monst *));
E void FDECL(priestini, (d_level *,struct mkroom *,int,int,int));
E char *FDECL(priestname, (struct monst *,char *));
E boolean FDECL(p_coaligned, (struct monst *));
E struct monst *FDECL(findpriest, (CHAR_P));
E void FDECL(intemple, (int));
E void FDECL(priest_talk, (struct monst *));
E struct monst *FDECL(mk_roamer, (struct permonst *,ALIGNTYP_P,
				  XCHAR_P,XCHAR_P,BOOLEAN_P));
E void FDECL(reset_hostility, (struct monst *));
E boolean FDECL(in_your_sanctuary, (struct monst *,XCHAR_P,XCHAR_P));
E void FDECL(ghod_hitsu, (struct monst *));
E void NDECL(angry_priest);
E void NDECL(clearpriests);

/* ### projectile.c ### */

E void FDECL(interrupt_multishot, (struct obj *, struct obj *));
E int FDECL(projectile, (struct monst *, struct obj *, void *, int, int, int, int, int, int, int, boolean, boolean, boolean));
E void FDECL(do_digging_impact, (struct monst *, struct obj *, int, int));
E void FDECL(hitfloor2, (struct monst *, struct obj **, struct obj *, boolean, boolean));
E boolean FDECL(xbreathey, (struct monst *, struct attack *, int, int));
E boolean FDECL(xspity, (struct monst *, struct attack *, int, int));
E boolean FDECL(xfirey, (struct monst *, struct attack *, int, int, int));
E boolean FDECL(m_online, (struct monst *, struct monst *, int, int, boolean, boolean));
E boolean FDECL(m_insplash, (struct monst *, struct monst *, int, int, boolean));
E boolean FDECL(mdofire, (struct monst *, struct monst *, int, int));
E boolean FDECL(umetgaze, (struct monst *));
E int NDECL(dothrow);
E int FDECL(ufire_blaster, (struct obj *, int));
E int NDECL(dofire);
E int FDECL(uthrow, (struct obj *, struct obj *, int, boolean));
E int FDECL(mthrow, (struct monst *, struct obj *, struct obj *, int, int, boolean));
E void FDECL(return_ammo, (genericptr_t, long));

/* ### quest.c ### */

E void NDECL(onquest);
E void NDECL(nemdead);
E void NDECL(artitouch);
E boolean NDECL(ok_to_quest);
E void FDECL(leader_speaks, (struct monst *));
E void NDECL(nemesis_speaks);
E void FDECL(quest_chat, (struct monst *));
E void FDECL(quest_talk, (struct monst *));
E void FDECL(quest_stat_check, (struct monst *));
E void FDECL(finish_quest, (struct obj *));
E void NDECL(turn_stag);

/* ### questpgr.c ### */

E void NDECL(load_qtlist);
E void NDECL(unload_qtlist);
E short FDECL(quest_info, (int));
E const char *NDECL(ldrname);
E boolean FDECL(is_quest_artifact, (struct obj*));
E boolean FDECL(is_primary_quest_artifact, (struct obj*));
E void FDECL(com_pager, (int));
E void FDECL(qt_pager, (int));
E void FDECL(string_pager, (char *));
E struct permonst *FDECL(qt_montype, (int, int));
E struct permonst *NDECL(chaos_montype);
E struct permonst *NDECL(chaos2_montype);
E struct permonst *NDECL(chaos3_montype);
E struct permonst *FDECL(ford_montype, (int));
E struct permonst *NDECL(neutral_montype);
E struct permonst *NDECL(law_montype);
E struct permonst *NDECL(moon_montype);

/* ### random.c ### */

#if defined(RANDOM) && !defined(__GO32__) /* djgpp has its own random */
E void FDECL(srandom, (unsigned));
E char *FDECL(initstate, (unsigned,char *,int));
E char *FDECL(setstate, (char *));
E long NDECL(random);
#endif /* RANDOM */

/* ### read.c ### */

E int NDECL(doread);
E boolean FDECL(is_chargeable, (struct obj *));
E int FDECL(recharge, (struct obj *,int));
E void FDECL(forget, (int));
E void FDECL(forget_objects, (int));
E void FDECL(forget_levels, (int));
E void NDECL(forget_traps);
E void FDECL(forget_map, (int));
E int FDECL(seffects, (struct obj *));
E void FDECL(litroom, (BOOLEAN_P,struct obj *));
E void FDECL(set_lit, (int,int,genericptr_t));
E void FDECL(do_genocide, (int));
E void FDECL(punish, (struct obj *));
E void NDECL(unpunish);
E boolean FDECL(cant_create, (int *, BOOLEAN_P));
#ifdef WIZARD
E int NDECL(wiz_kill_all);
E struct monst * FDECL(create_particular, (int, int, unsigned long, int, int, unsigned long, unsigned long, int, char *));
#endif

/* ### rect.c ### */

E void NDECL(init_rect);
E NhRect *FDECL(get_rect, (NhRect *));
E NhRect *NDECL(rnd_rect);
E void FDECL(remove_rect, (NhRect *));
E void FDECL(add_rect, (NhRect *));
E void FDECL(split_rects, (NhRect *,NhRect *));

/* ## region.c ### */
E void NDECL(clear_regions);
E void NDECL(run_regions);
E boolean FDECL(In_fog_cloud, (struct monst *));
E boolean FDECL(check_solid_fog_region, (XCHAR_P,XCHAR_P));
E boolean FDECL(check_stinking_cloud_region, (XCHAR_P,XCHAR_P));
E boolean FDECL(check_dust_cloud_region, (XCHAR_P,XCHAR_P));
E boolean FDECL(in_out_region, (XCHAR_P,XCHAR_P));
E boolean FDECL(m_in_out_region, (struct monst *,XCHAR_P,XCHAR_P));
E void NDECL(update_player_regions);
E void FDECL(update_monster_region, (struct monst *));
E NhRegion *FDECL(visible_region_at, (XCHAR_P,XCHAR_P));
E void FDECL(show_region, (NhRegion*, XCHAR_P, XCHAR_P));
E void FDECL(save_regions, (int,int));
E void FDECL(rest_regions, (int,BOOLEAN_P));
E NhRegion* FDECL(create_generic_cloud, (XCHAR_P, XCHAR_P, int, struct region_arg *, boolean));
E NhRegion* FDECL(create_gas_cloud, (XCHAR_P, XCHAR_P, int, int, boolean));
E NhRegion* FDECL(create_fog_cloud, (XCHAR_P, XCHAR_P, int, int, boolean));
E NhRegion* FDECL(create_dust_cloud, (XCHAR_P, XCHAR_P, int, int));

/* ### research.c ### */
E int NDECL(doupgradeability);

/* ### restore.c ### */

E void FDECL(inven_inuse, (BOOLEAN_P));
E int FDECL(dorecover, (int));
E void FDECL(trickery, (char *));
E void FDECL(getlev, (int,int,int,BOOLEAN_P));
E void NDECL(minit);
E boolean FDECL(lookup_id_mapping, (unsigned, unsigned *));
#ifdef ZEROCOMP
E int FDECL(mread, (int,genericptr_t,unsigned int));
#else
E void FDECL(mread, (int,genericptr_t,unsigned int));
#endif

/* ### rip.c ### */

E void FDECL(genl_outrip, (winid,int));

/* ### rnd.c ### */

E void NDECL(check_reseed);
E int FDECL(rn2, (int));
E int FDECL(rnl, (int));
E int FDECL(rnd, (int));
E int FDECL(d, (int,int));
E int FDECL(exploding_d, (int,int,int));
E int FDECL(lucky_exploding_d, (int,int,int));
E int FDECL(unlucky_exploding_d, (int,int,int));
E int FDECL(rne, (int));
E int FDECL(rnz, (int));

/* ### role.c ### */

E boolean FDECL(validrole, (int));
E boolean FDECL(validrace, (int, int));
E boolean FDECL(validdescendant, (int));
E boolean FDECL(validgend, (int, int, int));
E boolean FDECL(validalign, (int, int, int));
E int FDECL(randrole, (int));
E int FDECL(randrace, (int));
E int FDECL(randgend, (int, int));
E int FDECL(randalign, (int, int));
E int FDECL(str2role, (char *));
E struct Role *FDECL(pm2role, (int));
E int FDECL(str2race, (char *));
E int FDECL(str2gend, (char *));
E int FDECL(str2align, (char *));
E boolean FDECL(ok_role, (int, int, int, int));
E int FDECL(pick_role, (int, int, int, int));
E boolean FDECL(ok_race, (int, int, int, int));
E int FDECL(pick_race, (int, int, int, int));
E boolean FDECL(ok_gend, (int, int, int, int));
E int FDECL(pick_gend, (int, int, int, int));
E boolean FDECL(ok_align, (int, int, int, int));
E int FDECL(pick_align, (int, int, int, int));
E void FDECL(role_init, (int));
E int FDECL(get_vgod, (int));
E void NDECL(rigid_role_checks);
E void NDECL(plnamesuffix);
E const char *FDECL(Hello, (struct monst *));
E const char *NDECL(Goodbye);
E char *FDECL(build_plselection_prompt, (char *, int, int, int, int, int, int));
E char *FDECL(root_plselection_prompt, (char *, int, int, int, int, int, int));
#ifdef RECORD_ACHIEVE
E void FDECL(add_imp_record, (long));
E void NDECL(give_quest_trophy);
E void NDECL(give_ascension_trophy);
E void NDECL(give_lamashtu_trophy);
E void NDECL(give_baalphegor_trophy);
E void NDECL(give_castle_trophy);
E void NDECL(give_ugwish_trophy);
E void NDECL(give_law_trophy);
E void NDECL(give_angel_vault_trophy);
E void NDECL(give_ancient_vault_trophy);
E void NDECL(give_tannin_vault_trophy);
E void NDECL(give_devil_vault_trophy);
E void NDECL(give_demon_vault_trophy);
E void FDECL(give_hell_vault_trophy, (int));
E void NDECL(give_neutral_trophy);
E void NDECL(give_chaos_temple_trophy);
E void NDECL(check_mithardir_trophy);
E void NDECL(give_mordor_trophy);
E void NDECL(check_illumian_trophy);
E void NDECL(give_rescue_trophy);
E void NDECL(check_loadout_trophy);
E void NDECL(give_nightmare_hunter_trophy);
E void NDECL(check_madman_trophy);
E void NDECL(check_drunkard_trophy);
E void NDECL(give_bokrug_trophy);
#endif

/* ### rumors.c ### */

E char *FDECL(getrumor, (int,char *, BOOLEAN_P));
E void FDECL(outrumor, (int,int));
E void NDECL(outgmaster);
E void FDECL(outoracle, (BOOLEAN_P, BOOLEAN_P));
E void FDECL(save_oracles, (int,int));
E void FDECL(restore_oracles, (int));
E int FDECL(doconsult, (struct monst *));

/* ### save.c ### */

E int NDECL(dosave);
#if defined(UNIX) || defined(VMS) || defined(__EMX__) || defined(WIN32)
E void FDECL(hangup, (int));
#endif
E int NDECL(dosave0);
#ifdef INSURANCE
E void NDECL(savestateinlock);
#endif
#ifdef MFLOPPY
E boolean FDECL(savelev, (int,int,int));
E boolean FDECL(swapin_file, (int));
E void NDECL(co_false);
#else
E void FDECL(savelev, (int,int,int));
#endif
E void FDECL(bufon, (int));
E void FDECL(bufoff, (int));
E void FDECL(bflush, (int));
E void FDECL(bwrite, (int,genericptr_t,unsigned int));
E void FDECL(bclose, (int));
E void FDECL(savefruitchn, (int,int));
E void NDECL(free_dungeons);
E void NDECL(freedynamicdata);
E boolean NDECL(dahlverNarVis);

/* ### seduce.c ### */

E int FDECL(could_seduce, (struct monst *, struct monst *, struct attack *));
#ifdef SEDUCE
E int FDECL(doseduce, (struct monst *));
E int FDECL(dosflseduce, (struct monst *));
E int FDECL(dopaleseduce, (struct monst *));
E int FDECL(dotent, (struct monst *, int));
E boolean FDECL(msteal_m, (struct monst *, struct monst *, struct attack *, int *));
#endif

/* ### shk.c ### */

#ifdef GOLDOBJ
E long FDECL(money2mon, (struct monst *, long));
E long FDECL(money2none, (long));
E void FDECL(money2u, (struct monst *, long));
#endif
E char *FDECL(shkname, (struct monst *));
E void FDECL(shkgone, (struct monst *));
E void FDECL(set_residency, (struct monst *,BOOLEAN_P));
E void FDECL(replshk, (struct monst *));
E void FDECL(restshk, (struct monst *,BOOLEAN_P));
E char FDECL(inside_shop, (XCHAR_P,XCHAR_P));
E void FDECL(u_left_shop, (char *,BOOLEAN_P));
E void FDECL(remote_burglary, (struct obj *));
E void FDECL(u_entered_shop, (char *));
E boolean FDECL(same_price, (struct obj *,struct obj *));
E void NDECL(shopper_financial_report);
E int FDECL(inhishop, (struct monst *));
E struct monst *FDECL(shop_keeper, (CHAR_P));
E boolean FDECL(tended_shop, (struct mkroom *));
E void FDECL(delete_contents, (struct obj *));
E void FDECL(obfree, (struct obj *,struct obj *));
E void FDECL(home_shk, (struct monst *,BOOLEAN_P));
E void FDECL(pacify_shk, (struct monst *));
E void FDECL(make_happy_shk, (struct monst *,BOOLEAN_P));
E void FDECL(hot_pursuit, (struct monst *));
E void FDECL(make_angry_shk, (struct monst *,XCHAR_P,XCHAR_P));
E long FDECL(get_cost_of_shop_item, (struct obj *));
E int NDECL(dopay);
E boolean FDECL(paybill, (int));
E void NDECL(finish_paybill);
E struct obj *FDECL(find_oid, (unsigned));
E long FDECL(contained_cost, (struct obj *,struct monst *,long,BOOLEAN_P, BOOLEAN_P));
E long FDECL(contained_gold, (struct obj *));
E void FDECL(picked_container, (struct obj *));
E long FDECL(unpaid_cost, (struct obj *));
E void FDECL(addtobill, (struct obj *,BOOLEAN_P,BOOLEAN_P,BOOLEAN_P));
E void FDECL(splitbill, (struct obj *,struct obj *));
E void FDECL(subfrombill, (struct obj *,struct monst *));
E long FDECL(stolen_value, (struct obj *,XCHAR_P,XCHAR_P,BOOLEAN_P,BOOLEAN_P));
E void FDECL(sellobj_state, (int));
E void FDECL(sellobj, (struct obj *,XCHAR_P,XCHAR_P));
E void FDECL(setpaid, (struct monst *));
E int FDECL(doinvbill, (int));
E long FDECL(getprice, (struct obj *, BOOLEAN_P, BOOLEAN_P));
E struct monst *FDECL(shkcatch, (struct obj *,XCHAR_P,XCHAR_P));
E void FDECL(add_damage, (XCHAR_P,XCHAR_P,long));
E int FDECL(repair_damage, (struct monst *,struct damage *,BOOLEAN_P));
E int FDECL(shk_move, (struct monst *));
E void FDECL(after_shk_move, (struct monst *));
E boolean FDECL(is_fshk, (struct monst *));
E void FDECL(shopdig, (int));
E void FDECL(pay_for_damage, (const char *,BOOLEAN_P));
E boolean FDECL(costly_spot, (XCHAR_P,XCHAR_P));
E struct obj *FDECL(shop_object, (XCHAR_P,XCHAR_P));
E void FDECL(price_quote, (struct obj *));
E void FDECL(shk_chat, (struct monst *));
E void FDECL(check_unpaid_usage, (struct obj *,BOOLEAN_P));
E void FDECL(check_unpaid, (struct obj *));
E void FDECL(costly_gold, (XCHAR_P,XCHAR_P,long));
E boolean FDECL(block_door, (XCHAR_P,XCHAR_P));
E boolean FDECL(block_entry, (XCHAR_P,XCHAR_P));
E char *FDECL(shk_mons, (char *,struct obj *, struct monst *));
E char *FDECL(Shk_Mons, (char *,struct obj *, struct monst *));
E char *FDECL(shk_your, (char *,struct obj *));
E char *FDECL(Shk_Your, (char *,struct obj *));
E void FDECL(makeketer, (coord *));
E int FDECL(countFarSigns, (struct monst *));
E int FDECL(countCloseSigns, (struct monst *));
E void FDECL(smith_resizeArmor, (struct monst *, struct obj *));
E void FDECL(smithy_services, (struct monst *));
E void FDECL(initialize_smith_stocks, (struct monst *));

/* ### shknam.c ### */

E void FDECL(stock_room, (int,struct mkroom *));
E boolean FDECL(saleable, (struct monst *,struct obj *));
E int FDECL(get_special_shop_item, (int));
E int FDECL(get_shop_item, (int));

/* ### sit.c ### */

E void NDECL(take_gold);
E int NDECL(dosit);
E int FDECL(sit_bergonic, (struct obj *));
E boolean NDECL(rndcurse);
E boolean FDECL(mrndcurse, (struct monst *));
E void NDECL(attrcurse);

/* ### sounds.c ### */

E void NDECL(dosounds);
E const char *FDECL(growl_sound, (struct monst *));
E void FDECL(growl, (struct monst *));
E void FDECL(yelp, (struct monst *));
E void FDECL(whimper, (struct monst *));
E void FDECL(beg, (struct monst *));
E void FDECL(m_command, (struct monst *));
E int NDECL(dotalk);
E int FDECL(dochat, (BOOLEAN_P, int, int, int));
E int NDECL(binder_nearvoid_slots);
E int * FDECL(spirit_props, (int));
E void FDECL(councilspirit, (int));
E void FDECL(gnosisspirit, (int));
E void NDECL(scatter_seals);
E int FDECL(P_MAX_SKILL, (int));
E int FDECL(P_MAX_SKILL_CORE, (int, boolean));
E int FDECL(P_SKILL, (int));
E int FDECL(P_SKILL_CORE, (int, boolean));
E int FDECL(P_RESTRICTED, (int));
E boolean FDECL(spiritSkill, (int));
E boolean FDECL(roleSkill, (int));
E int FDECL(domonnoise,(struct monst *, BOOLEAN_P));
#ifdef USER_SOUNDS
E int FDECL(add_sound_mapping, (const char *));
E void FDECL(play_sound_for_message, (const char *));
#endif
E boolean NDECL(doreinforce_binding);

/* ### sys/msdos/sound.c ### */

#ifdef MSDOS
E int FDECL(assign_soundcard, (char *));
#endif

/* ### sp_lev.c ### */

E boolean FDECL(check_room, (xchar *,xchar *,xchar *,xchar *,BOOLEAN_P));
E boolean FDECL(create_room, (XCHAR_P,XCHAR_P,XCHAR_P,XCHAR_P,
			      XCHAR_P,XCHAR_P,XCHAR_P,XCHAR_P));
E void FDECL(create_secret_door, (struct mkroom *,XCHAR_P));
E boolean FDECL(dig_corridor, (coord *,coord *,BOOLEAN_P,SCHAR_P,SCHAR_P));
E void FDECL(fill_room, (struct mkroom *,BOOLEAN_P));
E boolean FDECL(load_special, (const char *));

/* ### spell.c ### */

#ifdef USE_TRAMPOLI
E int NDECL(learn);
#endif
E int FDECL(study_book, (struct obj *));
E int FDECL(further_study, (int));
E int FDECL(spellid_to_spellno, (int));
E boolean FDECL(spell_maintained, (int));
E void FDECL(spell_maintain, (int));
E void FDECL(spell_unmaintain, (int));
E void NDECL(run_maintained_spells);
E void FDECL(book_disappears, (struct obj *));
E void FDECL(book_substitution, (struct obj *,struct obj *));
E void NDECL(age_spells);
E void FDECL(damage_spells, (int));
E int NDECL(docast);
E boolean FDECL(tt_findadjacent, (coord *, struct monst *));
E int NDECL(spiritDsize);
E int NDECL(dospirit);
E int FDECL(spell_skill_from_adtype, (int));
E int FDECL(spell_adtype, (int));
E const char *FDECL(spelltypemnemonic, (int));
E int FDECL(spell_skilltype, (int));
E int FDECL(spiriteffects, (int,BOOLEAN_P));
E int FDECL(nudzirath_hit_pile, (struct obj *, struct obj *));
E int FDECL(nudzirath_hit_mon, (struct monst *, struct obj *));
E void FDECL(nudzirath_shatter, (struct obj *, int, int));
E int FDECL(spellenergy, (int));
E int FDECL(spelleffects, (int,boolean,int));
E int FDECL(wordeffects, (int));
E void FDECL(losespells, (int));
E int NDECL(throwgaze);
E int NDECL(dovspell);
E int NDECL(base_casting_stat);
E int FDECL(percent_success, (int));
E void FDECL(initialspell, (struct obj *));
E void FDECL(initialforgotwizardspells, (int));
E void FDECL(initialforgotpriestspells, (int));
E void FDECL(initialforgotspell, (int));
E void FDECL(initialward, (struct obj *));
E void FDECL(set_spirit_powers, (long));
E int NDECL(reorder_spirit_powers);
E void NDECL(dopseudonatural);
E void NDECL(dodestruction);
E void NDECL(domindblast_strong);
E boolean NDECL(doreinforce_spell);
E void NDECL(update_externally_granted_spells);
E void NDECL(expel_summons);

/* ### steal.c ### */

#ifdef USE_TRAMPOLI
E int NDECL(stealarm);
#endif
#ifdef GOLDOBJ
E long FDECL(somegold, (long));
#else
E long NDECL(somegold);
#endif
E void FDECL(stealgold, (struct monst *));
E void FDECL(remove_worn_item, (struct obj *,BOOLEAN_P));
E int FDECL(steal, (struct monst *, char *,int,int));
E int FDECL(mpickobj, (struct monst *,struct obj *));
E void FDECL(stealamulet, (struct monst *));
E void FDECL(stealquestart, (struct monst *));
E void FDECL(mdrop_special_objs, (struct monst *));
E void FDECL(relobj, (struct monst *,int,BOOLEAN_P));
E void FDECL(relobj_envy, (struct monst *,int));
E void FDECL(mdrop_obj, (struct monst *,struct obj *,BOOLEAN_P));
#ifdef GOLDOBJ
E struct obj *FDECL(findgold, (struct obj *));
#endif

/* ### steed.c ### */

#ifdef STEED
E void NDECL(rider_cant_reach);
E boolean FDECL(can_saddle, (struct monst *, struct obj *));
E int FDECL(use_saddle, (struct obj *));
E boolean FDECL(can_ride, (struct monst *));
E int NDECL(doride);
E boolean FDECL(mount_steed, (struct monst *, BOOLEAN_P));
E void NDECL(exercise_steed);
E void NDECL(kick_steed);
E void FDECL(dismount_steed, (int));
E void FDECL(place_monster, (struct monst *,int,int));
#endif

/* ### teleport.c ### */

E boolean FDECL(teleok, (int,int,BOOLEAN_P));
E boolean FDECL(goodpos, (int,int,struct monst *,unsigned));
E boolean FDECL(eonline, (coord *,XCHAR_P,XCHAR_P,struct permonst *));
E boolean FDECL(eofflin, (coord *,XCHAR_P,XCHAR_P,struct permonst *));
E boolean FDECL(eofflin_close, (coord *,XCHAR_P,XCHAR_P,struct permonst *));
E boolean FDECL(enexto, (coord *,XCHAR_P,XCHAR_P,struct permonst *));
E boolean FDECL(enexto_core, (coord *,XCHAR_P,XCHAR_P,struct permonst *,unsigned));
E void FDECL(xpathto, (int,XCHAR_P,XCHAR_P,int (*)(genericptr_t,int,int),void *));
E void FDECL(teleds, (int,int,BOOLEAN_P));
E boolean FDECL(safe_teleds, (BOOLEAN_P));
E boolean FDECL(teleport_pet, (struct monst *,BOOLEAN_P));
E boolean NDECL(tele);
E int NDECL(dotele);
E boolean NDECL(level_tele);
E boolean NDECL(branch_tele);
E void FDECL(domagicportal, (struct trap *));
E void FDECL(tele_trap, (struct trap *));
E void FDECL(level_tele_trap, (struct trap *, boolean));
E void FDECL(rloc_to, (struct monst *,int,int));
E boolean FDECL(rloc, (struct monst *, BOOLEAN_P));
E boolean FDECL(tele_restrict, (struct monst *));
E void FDECL(mtele_trap, (struct monst *, struct trap *,int));
E int FDECL(mlevel_tele_trap, (struct monst *, struct trap *,BOOLEAN_P,int));
E void FDECL(rlocos_at, (int, int));
E void FDECL(rloco, (struct obj *));
E void FDECL(randomly_place_obj, (struct obj *));
E int NDECL(random_teleport_level);
E boolean FDECL(u_teleport_mon, (struct monst *,BOOLEAN_P));

/* ### testing.c ### */
E int NDECL(wiz_testmenu);

/* ### tile.c ### */
#ifdef USE_TILES
E void FDECL(substitute_tiles, (d_level *));
#endif

/* ### timeout.c ### */

E void NDECL(burn_away_slime);
E void NDECL(melt_frozen_air);
E void FDECL(unbind_lifesaving, (long));
E void FDECL(unbind, (long, BOOLEAN_P));
E void FDECL(unbind_core, (long, BOOLEAN_P, BOOLEAN_P));
E void NDECL(nh_timeout);
E void FDECL(fall_asleep, (int, BOOLEAN_P));
E void FDECL(attach_bomb_blow_timeout, (struct obj *,int,BOOLEAN_P));
E void FDECL(attach_egg_hatch_timeout, (struct obj *));
E void FDECL(attach_fig_transform_timeout, (struct obj *));
E void FDECL(kill_egg, (struct obj *));
E void FDECL(hatch_egg, (genericptr_t, long));
E void FDECL(learn_egg_type, (int));
E void FDECL(burn_object, (genericptr_t, long));
E int FDECL(lightsource_radius, (struct obj *));
E long FDECL(lightsource_turns, (struct obj *));
E boolean FDECL(lightsource_timed, (struct obj *));
E void FDECL(begin_burn, (struct obj *));
E void FDECL(end_burn, (struct obj *, BOOLEAN_P));
E void FDECL(lightsaber_deactivate, (struct obj *, BOOLEAN_P));
E void NDECL(do_storms);
E struct obj * FDECL(update_skull_mon, (struct monst *, struct obj *));
E void FDECL(desummon_mon, (genericptr_t, long));
E void FDECL(cleanup_msummon, (genericptr_t, long));
E void FDECL(desummon_obj, (genericptr_t, long));
E void FDECL(summoner_gone, (struct monst *, boolean));
E void FDECL(stop_corpse_timers, (struct obj *));
E timer_element * FDECL(start_timer, (long, SHORT_P, SHORT_P, genericptr_t));
E void FDECL(pause_timer, (timer_element *));
E void FDECL(resume_timer, (timer_element *));
E void FDECL(pause_timers, (timer_element *));
E void FDECL(resume_timers, (timer_element *));
E void FDECL(migrate_timers, (timer_element *));
E void FDECL(receive_timers, (timer_element *));
E long FDECL(stop_timer, (SHORT_P, timer_element *));
E void FDECL(stop_all_timers, (timer_element *));
E void NDECL(run_timers);
E void FDECL(save_timers, (timer_element *,int,int));
E void FDECL(rest_timers, (int,genericptr_t,timer_element *,int,BOOLEAN_P,long));
E void FDECL(copy_timer, (struct timer *, int, genericptr_t));
E void FDECL(copy_timers, (struct timer *, int, genericptr_t));
#ifdef WIZARD
E int NDECL(wiz_timeout_queue);
E void NDECL(timer_sanity_check);
#endif

/* ### topten.c ### */

#ifdef RECORD_CONDUCT
E long FDECL(encodeconduct, (void));
#endif
E void FDECL(topten, (int));
E void FDECL(prscore, (int,char **));
E struct obj *FDECL(tt_oname, (struct obj *));
E void NDECL(write_HUP_file);

/* ### track.c ### */

E void NDECL(initrack);
E void NDECL(settrack);
E coord *FDECL(gettrack, (int,int,int));

/* ### trap.c ### */

E boolean FDECL(burnarmor,(struct monst *, BOOLEAN_P));
E boolean FDECL(rust_dmg, (struct obj *,const char *,int,BOOLEAN_P,struct monst *,BOOLEAN_P));
E void FDECL(grease_protect, (struct obj *,const char *,struct monst *));
E struct trap *FDECL(maketrap, (int,int,int));
E void FDECL(set_trap_ammo, (struct trap *, struct obj *));
E void FDECL(fall_through, (BOOLEAN_P));
E struct monst *FDECL(animate_statue, (struct obj *,XCHAR_P,XCHAR_P,int,int *));
E struct monst *FDECL(activate_statue_trap,
			(struct trap *,XCHAR_P,XCHAR_P,BOOLEAN_P));
E void FDECL(dotrap, (struct trap *, unsigned));
E void FDECL(seetrap, (struct trap *));
E int FDECL(mintrap, (struct monst *));
E void FDECL(reward_untrap, (struct trap *, struct monst *));
E void FDECL(rloc_trap, (struct trap *));
E void FDECL(instapetrify, (const char *));
E void FDECL(minstapetrify, (struct monst *,BOOLEAN_P));
E void FDECL(minstaglass, (struct monst *,BOOLEAN_P));
E void FDECL(minstagoldify, (struct monst *,int));
E void FDECL(selftouch, (const char *));
E void FDECL(mselftouch, (struct monst *,const char *,BOOLEAN_P));
E void NDECL(float_up);
E void FDECL(m_float_up, (struct monst *, boolean));
E void FDECL(fill_pit, (int,int));
E int FDECL(float_down, (long, long));
E void FDECL(m_float_down, (struct monst *, boolean));
E int FDECL(fire_damage_chain, (struct obj *,BOOLEAN_P,BOOLEAN_P,XCHAR_P,XCHAR_P));
E int FDECL(fire_damage, (struct obj *,BOOLEAN_P,XCHAR_P,XCHAR_P));
E boolean FDECL(lava_damage, (struct obj *,XCHAR_P,XCHAR_P));
E boolean FDECL(water_damage, (struct obj *, BOOLEAN_P, BOOLEAN_P, uchar, struct monst *));
E boolean NDECL(drown);
E int NDECL(dodeepswim);
E void FDECL(drain_en, (int));
E int NDECL(dountrap);
E void FDECL(remove_trap_ammo, (struct trap *));
E void FDECL(obj_extract_self_from_trap, (struct obj *));
E int FDECL(you_remove_jrt_fang, (struct monst *, struct obj *));
E int FDECL(untrap, (struct obj *));
E boolean FDECL(chest_trap, (struct obj *,int,BOOLEAN_P));
E void FDECL(deltrap, (struct trap *));
E boolean FDECL(delfloortrap, (struct trap *));
E struct trap *FDECL(t_at, (int,int));
E void FDECL(b_trapped, (const char *,int));
E boolean NDECL(unconscious);
E boolean FDECL(lava_effects, (boolean));
E void FDECL(blow_up_landmine, (struct trap *));
E int FDECL(launch_obj, (SHORT_P, struct trap *, int));
E void FDECL(unshackle_mon, (struct monst *));
E void FDECL(dowebgush, (int,int,int));
E void FDECL(webgush, (int,int,genericptr_t));
E int NDECL(ubreak_entanglement);
E void FDECL(entangle_effects, (struct monst *));
E int NDECL(uescape_entanglement);

/* ### u_init.c ### */

E void NDECL(u_init);
E void FDECL(knows_object,(int) );
E void FDECL(know_random_obj,(int));
E void NDECL(scatter_weapons);

/* ### unixmain.c ### */

#ifdef UNIX
# ifdef PORT_HELP
E void NDECL(port_help);
# endif
#endif /* UNIX */


/* ### unixtty.c ### */

#if defined(UNIX) || defined(__BEOS__)
E void NDECL(gettty);
E void FDECL(settty, (const char *));
E void NDECL(setftty);
E void NDECL(intron);
E void NDECL(introff);
E void VDECL(error, (const char *,...)) PRINTF_F(1,2);
#endif /* UNIX || __BEOS__ */

/* ### unixunix.c ### */

#ifdef UNIX
E void NDECL(getlock);
E void FDECL(regularize, (char *));
# if defined(TIMED_DELAY) && !defined(msleep) && defined(SYSV)
E void FDECL(msleep, (unsigned));
# endif
# ifdef SHELL
E int NDECL(dosh);
# endif /* SHELL */
# if defined(SHELL) || defined(DEF_PAGER) || defined(DEF_MAILREADER)
E int FDECL(child, (int));
# endif
#endif /* UNIX */

/* ### unixres.c ### */

#ifdef UNIX
# ifdef GNOME_GRAPHICS
E int FDECL(hide_privileges, (BOOLEAN_P));
# endif
#endif /* UNIX */

/* ### vault.c ### */

E boolean FDECL(grddead, (struct monst *));
E char FDECL(vault_occupied, (char *));
E void NDECL(invault);
E int FDECL(gd_move, (struct monst *));
E void NDECL(paygd);
E long NDECL(hidden_gold);
E boolean NDECL(gd_sound);

/* ### version.c ### */

E char *FDECL(version_string, (char *));
E char *FDECL(getversionstring, (char *));
E int NDECL(doversion);
E int NDECL(doextversion);
#ifdef MICRO
E boolean FDECL(comp_times, (long));
#endif
E boolean FDECL(check_version, (struct version_info *,
				const char *,BOOLEAN_P));
E unsigned long FDECL(get_feature_notice_ver, (char *));
E unsigned long NDECL(get_current_feature_ver);
#ifdef RUNTIME_PORT_ID
E void FDECL(append_port_id, (char *));
#endif

/* ### video.c ### */

#ifdef MSDOS
E int FDECL(assign_video, (char *));
# ifdef NO_TERMS
E void NDECL(gr_init);
E void NDECL(gr_finish);
# endif
E void FDECL(tileview,(BOOLEAN_P));
#endif
#ifdef VIDEOSHADES
E int FDECL(assign_videoshades, (char *));
E int FDECL(assign_videocolors, (char *));
#endif

/* ### vis_tab.c ### */

#ifdef VISION_TABLES
E void NDECL(vis_tab_init);
#endif

/* ### vision.c ### */

E void NDECL(vision_init);
E int FDECL(does_block, (int,int,struct rm*));
E void NDECL(vision_reset);
E void FDECL(vision_recalc, (int));
E void FDECL(block_point, (int,int));
E void FDECL(unblock_point, (int,int));
E boolean FDECL(clear_path, (int,int,int,int));
E void FDECL(do_clear_area, (int,int,int,
			     void (*)(int,int,genericptr_t),genericptr_t));

#ifdef VMS

/* ### vmsfiles.c ### */

E int FDECL(vms_link, (const char *,const char *));
E int FDECL(vms_unlink, (const char *));
E int FDECL(vms_creat, (const char *,unsigned int));
E int FDECL(vms_open, (const char *,int,unsigned int));
E boolean FDECL(same_dir, (const char *,const char *));
E int FDECL(c__translate, (int));
E char *FDECL(vms_basename, (const char *));

/* ### vmsmail.c ### */

E unsigned long NDECL(init_broadcast_trapping);
E unsigned long NDECL(enable_broadcast_trapping);
E unsigned long NDECL(disable_broadcast_trapping);
# if 0
E struct mail_info *NDECL(parse_next_broadcast);
# endif /*0*/

/* ### vmsmain.c ### */

E int FDECL(main, (int, char **));
# ifdef CHDIR
E void FDECL(chdirx, (const char *,BOOLEAN_P));
# endif /* CHDIR */

/* ### vmsmisc.c ### */

E void NDECL(vms_abort);
E void FDECL(vms_exit, (int));

/* ### vmstty.c ### */

E int NDECL(vms_getchar);
E void NDECL(gettty);
E void FDECL(settty, (const char *));
E void FDECL(shuttty, (const char *));
E void NDECL(setftty);
E void NDECL(intron);
E void NDECL(introff);
E void VDECL(error, (const char *,...)) PRINTF_F(1,2);
#ifdef TIMED_DELAY
E void FDECL(msleep, (unsigned));
#endif

/* ### vmsunix.c ### */

E void NDECL(getlock);
E void FDECL(regularize, (char *));
E int NDECL(vms_getuid);
E boolean FDECL(file_is_stmlf, (int));
E int FDECL(vms_define, (const char *,const char *,int));
E int FDECL(vms_putenv, (const char *));
E char *NDECL(verify_termcap);
# if defined(CHDIR) || defined(SHELL) || defined(SECURE)
E void NDECL(privoff);
E void NDECL(privon);
# endif
# ifdef SHELL
E int NDECL(dosh);
# endif
# if defined(SHELL) || defined(MAIL)
E int FDECL(vms_doshell, (const char *,BOOLEAN_P));
# endif
# ifdef SUSPEND
E int NDECL(dosuspend);
# endif

#endif /* VMS */

/* ### weapon.c ### */

E int FDECL(hitval, (struct obj *,struct monst *,struct monst *));
E int FDECL(attack_mask, (struct obj *, int, int, struct monst *));
E int FDECL(dmgval_core, (struct weapon_dice *, boolean, struct obj *, int, struct monst *));
E int FDECL(weapon_dmg_roll, (struct weapon_dice *, boolean));
E int FDECL(weapon_die_roll, (int, int, struct weapon_dice *, boolean));
E int FDECL(dmgval, (struct obj *,struct monst *, int, struct monst *));
E struct obj *FDECL(select_rwep, (struct monst *));
E struct obj *FDECL(select_hwep, (struct monst *));
E struct obj *FDECL(select_pick, (struct monst *));
E struct obj *FDECL(select_axe, (struct monst *));
E void FDECL(possibly_unwield, (struct monst *,BOOLEAN_P));
E int FDECL(mon_wield_item, (struct monst *));
E void FDECL(setmwielded, (struct monst *, struct obj *, long));
E void FDECL(setmnotwielded, (struct monst *, struct obj *));
E void FDECL(init_mon_wield_item, (struct monst *));
E int NDECL(str_abon);
E int NDECL(abon);
E float FDECL(atr_dbon, (struct obj *, struct monst *, int));
E int FDECL(str_dbon, (struct monst *));
E int FDECL(dbon, (struct obj *, struct monst *));
E void NDECL(reset_skills);
E boolean FDECL(fake_skill, (int));
E int NDECL(enhance_weapon_skill);
E int FDECL(skill_dam_bonus, (int));
E void FDECL(gm_weapon_skill, (int));
E void FDECL(free_skill_up, (int));
#ifdef DUMP_LOG
E void NDECL(dump_weapon_skill);
#endif
E void FDECL(unrestrict_weapon_skill, (int));
E void FDECL(restrict_weapon_skill, (int));
E void FDECL(expert_weapon_skill, (int));
E void FDECL(skilled_weapon_skill, (int));
E void FDECL(use_skill, (int,int));
E void FDECL(lose_skill, (int,int));
E void FDECL(add_weapon_skill, (int));
E void FDECL(lose_weapon_skill, (int));
E int FDECL(weapon_type, (struct obj *));
E int NDECL(uwep_skill_type);
E int NDECL(get_your_size);
E int NDECL(get_your_shield_size);
E int FDECL(weapon_hit_bonus, (struct obj *, int));
E int FDECL(mon_weapon_dam_bonus, (struct permonst *, struct obj *, int));
E int FDECL(weapon_dam_bonus, (struct obj *, int));
E int FDECL(shield_skill, (struct obj *));
E void FDECL(skill_init, (const struct def_skill *));
E void FDECL(skill_add, (const struct def_skill *));
E void FDECL(skill_up, (const struct def_skill *));
E const char * FDECL(P_NAME, (int));
E int NDECL(aeshbon);
E boolean FDECL(ok_mariwep, (struct obj *, struct monst *, struct permonst *, boolean));
E struct obj * FDECL(get_mariwep, (struct monst *, struct permonst *, int));

/* ### were.c ### */

E void FDECL(were_change, (struct monst *));
E int FDECL(counter_were,(int));
E int FDECL(transmitted_were, (int));
E void FDECL(were_transform, (struct monst *, int));
E void FDECL(new_were, (struct monst *));
E int FDECL(were_summon, (struct monst *,int *,char *));
E void NDECL(you_were);
E void FDECL(you_unwere, (BOOLEAN_P));

/* ### wield.c ### */

E void FDECL(setuwep, (struct obj *));
E void FDECL(setuqwep, (struct obj *));
E void FDECL(setuswapwep, (struct obj *));
E int NDECL(dowield);
E int NDECL(doswapweapon);
E int NDECL(dowieldquiver);
E void FDECL(rewield, (struct obj *, long));
E boolean FDECL(wield_tool, (struct obj *,const char *));
E int NDECL(test_twoweapon);
E int NDECL(starting_twoweapon);
E void NDECL(drop_uswapwep);
E int NDECL(dotwoweapon);
E void NDECL(uwepgone);
E void NDECL(uswapwepgone);
E void NDECL(uqwepgone);
E void NDECL(untwoweapon);
E void FDECL(erode_obj, (struct obj *,BOOLEAN_P,BOOLEAN_P));
E int FDECL(chwepon, (struct obj *,int));
E int FDECL(welded, (struct obj *));
E void FDECL(weldmsg, (struct obj *));
E double FDECL(bimanual_mod, (struct obj *, struct monst *));
E boolean FDECL(bimanual, (struct obj *, struct permonst *));

/* ### windows.c ### */

E void FDECL(choose_windows, (const char *));
E char FDECL(genl_message_menu, (CHAR_P,int,const char *));
E void FDECL(genl_preference_update, (const char *));

/* ### wizard.c ### */

E void NDECL(amulet);
E int FDECL(mon_has_amulet, (struct monst *));
E int FDECL(mon_has_special, (struct monst *));
E boolean FDECL(mon_has_arti, (struct monst *,SHORT_P));
E int FDECL(tactics, (struct monst *));
E void NDECL(aggravate);
E void NDECL(clonewiz);
E int NDECL(pick_nasty);
E int FDECL(nasty, (struct monst*));
E void NDECL(yellow_nasty);
E void NDECL(yellow_dead);
E boolean NDECL(yellow_smite);
E void NDECL(resurrect);
E void NDECL(illur_resurrect);
E void NDECL(coa_arrive);
E void NDECL(intervene);
E void NDECL(illur_intervene);
E boolean NDECL(yello_intervene);
E void NDECL(wizdead);
E void FDECL(cuss, (struct monst *));

/* ### worm.c ### */

E int NDECL(get_wormno);
E void FDECL(initworm, (struct monst *,int));
E void FDECL(worm_move, (struct monst *));
E void FDECL(worm_nomove, (struct monst *));
E void FDECL(wormgone, (struct monst *));
E void FDECL(wormhitu, (struct monst *));
E boolean FDECL(wormline, (struct monst *, int, int));
E void FDECL(cutworm, (struct monst *,XCHAR_P,XCHAR_P,struct obj *));
E void FDECL(see_wsegs, (struct monst *));
E void FDECL(detect_wsegs, (struct monst *,BOOLEAN_P));
E void FDECL(save_worm, (int,int));
E void FDECL(rest_worm, (int));
E void FDECL(place_wsegs, (struct monst *));
E void FDECL(remove_worm, (struct monst *));
E void FDECL(place_worm_tail_randomly, (struct monst *,XCHAR_P,XCHAR_P));
E int FDECL(count_wsegs, (struct monst *));
E boolean FDECL(worm_known, (struct monst *));

/* ### worn.c ### */
E boolean FDECL(item_has_property, (struct obj *, int));
E void FDECL(get_item_property_list, (int *, struct obj*, int));
E void FDECL(setworn, (struct obj *,long));
E void FDECL(m_lose_armor, (struct monst *,struct obj *));
E void FDECL(setnotworn, (struct obj *));
E void FDECL(mon_set_minvis, (struct monst *));
E void FDECL(mon_adjust_speed, (struct monst *,int,struct obj *, boolean));
E void FDECL(update_mon_intrinsics,
		(struct monst *,struct obj *,BOOLEAN_P,BOOLEAN_P));
E int FDECL(base_mac, (struct monst *));
E int FDECL(find_mac, (struct monst *));
E int FDECL(full_mac, (struct monst *));
E int FDECL(full_marmorac, (struct monst *));
E int FDECL(base_nat_mdr, (struct monst *));
E int FDECL(base_mdr, (struct monst *));
E int FDECL(avg_spell_mdr, (struct monst *));
E int FDECL(roll_mdr, (struct monst *, struct monst *, int));
E int FDECL(roll_mdr_detail, (struct monst *, struct monst *, int, int, uchar));
E void FDECL(mon_slot_dr, (struct monst *, struct monst *, int, int *, int *, int *, int));
E int FDECL(avg_mdr, (struct monst *)); 
E int FDECL(mdat_avg_mdr, (struct monst *));
E void FDECL(m_dowear, (struct monst *,BOOLEAN_P));
E boolean FDECL(mon_throw_armor, (struct monst *));
E boolean FDECL(mon_strip_armor, (struct monst *));
E struct obj *FDECL(mon_remove_armor, (struct monst *,long));
E struct obj *FDECL(which_armor, (struct monst *,long));
E void FDECL(mon_break_armor, (struct monst *,BOOLEAN_P));
E int FDECL(extra_pref, (struct monst *, struct obj *));
E void FDECL(bypass_obj, (struct obj *));
E void NDECL(clear_bypasses);
E int FDECL(magic_negation, (struct monst *));
E void FDECL(light_damage, (genericptr_t, long));
E int NDECL(heal_mlevel_bonus);
E int NDECL(heal_vilya);
E int NDECL(lev_lomya);
E int NDECL(en_nenya);
E void FDECL(saber_damage_slot, (struct monst *, struct obj *, int, boolean, boolean, boolean *));

/* ### write.c ### */

E int FDECL(dowrite, (struct obj *));

/* ### xhity.c ### */

E int FDECL(check_subout, (int *, int));
E void FDECL(add_subout, (int *, int));
E void FDECL(zero_subout, (int *));
E int FDECL(getvis, (struct monst *, struct monst *, int, int));
E boolean FDECL(attack2, (struct monst *));
E int FDECL(xattacky, (struct monst *, struct monst *, int, int));
E int FDECL(xmeleehity, (struct monst *, struct monst *, struct attack *, struct obj **, int, int, boolean));
E struct attack * FDECL(getattk, (struct monst *, struct monst *, int *, int *, struct attack *, boolean, int *, int *));
E boolean FDECL(slips_free, (struct monst *, struct monst *, struct attack *, int));
E void FDECL(heal, (struct monst *, int));
E int FDECL(xdamagey, (struct monst *, struct monst *, struct attack *, int));
E int FDECL(xstoney, (struct monst *, struct monst *));
E int FDECL(xmeleehurty, (struct monst *, struct monst *, struct attack *, struct attack *, struct obj **, boolean, int, int, int, boolean));
E int FDECL(xmeleehurty_core, (struct monst *, struct monst *, struct attack *, struct attack *, struct obj **, boolean, int, int, int, boolean, unsigned long));
E void FDECL(getgazeinfo, (int, int, struct permonst *, struct monst *, struct monst *, boolean *, boolean *, boolean *, boolean *));
E int FDECL(xgazey, (struct monst *, struct monst *, struct attack *, int));
E int FDECL(xengulfhity, (struct monst *, struct monst *, struct attack *, int));
E int FDECL(xengulfhurty, (struct monst *, struct monst *, struct attack *, int));
E void FDECL(passive_obj2, (struct monst *, struct monst *, struct obj *, struct attack *, struct attack *));
E void FDECL(breakobj, (struct obj *, XCHAR_P, XCHAR_P, BOOLEAN_P, BOOLEAN_P));
E int FDECL(hmon_with_unowned_obj, (struct monst *, struct obj **, int));
E int FDECL(hmon_with_trap, (struct monst *, struct obj **, struct trap *, int, int));
E int FDECL(hmon_general, (struct monst *, struct monst *, struct attack *, struct attack *, struct obj **, void *, int, int, int, boolean, int, boolean, int));
E int FDECL(hmon_general_modifiers, (struct monst *, struct monst *, struct attack *, struct attack *, struct obj **, void *, int, int, int, boolean, int, boolean, int, unsigned long));
E void FDECL(wakeup2, (struct monst *, boolean));
E int FDECL(xpassivey, (struct monst *, struct monst *, struct attack *, struct obj *, int, int, struct permonst *, boolean));
E int tohitval(struct monst *, struct monst *, struct attack *, struct obj *, void *, int, int, int *);
E void FDECL(weave_black_web, (struct monst *));
E int NDECL(android_combo);
E void NDECL(movement_combos);
E boolean FDECL(perform_monk_move, (int));
E boolean NDECL(perform_expert_move);
E int NDECL(check_monk_move);
E int FDECL(u_pole_pound, (struct monst *));
E boolean FDECL(Curse_res, (struct monst *, boolean));
E int FDECL(mummy_curses_x, (struct monst *, struct monst *));
E int FDECL(reduce_dmg, (struct monst *, int, boolean, boolean));

/* ### xhityhelpers.c ### */

E boolean FDECL(magr_can_attack_mdef, (struct monst *, struct monst *, int, int, boolean));
E int FDECL(attack_checks, (struct monst *, struct obj *));
E boolean FDECL(madness_cant_attack, (struct monst *));
E void FDECL(stumble_onto_mimic, (struct monst *));
E void FDECL(check_caitiff, (struct monst *));
E void FDECL(steal_it, (struct monst *, struct attack *));
E int FDECL(mattacku, (struct monst *));
E int FDECL(mattackm, (struct monst *, struct monst *));
E int FDECL(fightm, (struct monst *));
E void FDECL(expels, (struct monst *, struct permonst *, BOOLEAN_P));
E boolean FDECL(diseasemu, (struct permonst *));
E void NDECL(u_slow_down);
E const char *FDECL(mpoisons_subj, (struct monst *, struct attack *));
E int FDECL(sleep_monst, (struct monst *, int, int));
E void FDECL(slept_monst, (struct monst *));
E void FDECL(hurtarmor, (int, boolean));
E void FDECL(hurtmarmor, (struct monst *, int, boolean));
E long FDECL(attk_protection, (int));
E long FDECL(attk_equip_slot, (struct monst *, int));
E boolean FDECL(badtouch, (struct monst *, struct monst *, struct attack *, struct obj *));
E boolean FDECL(safe_attack, (struct monst *, struct monst *, struct attack *, struct obj *, struct permonst *, struct permonst *));
E int NDECL(beastmastery);
E int NDECL(narya);
E int NDECL(mountedCombat);
E boolean FDECL(obj_silver_searing, (struct obj *));
E boolean FDECL(obj_jade_searing, (struct obj *));
E int FDECL(hatesobjdmg, (struct monst *, struct obj *, struct monst *));
E int FDECL(hits_insubstantial, (struct monst *, struct monst *, struct attack *, struct obj *));
E boolean FDECL(miss_via_insubstantial, (struct monst *, struct monst *, struct attack *, struct obj *, int));
E int FDECL(destroy_item, (struct monst *, int, int));
E boolean FDECL(wearing_dragon_armor, (struct monst *, int));
E int FDECL(hit_with_cclaw, (struct monst *,struct obj *, int, int, int, struct attack *));
E int FDECL(hit_with_iwarp, (struct monst *,struct obj *, int, int, int, struct attack *));
E int FDECL(hit_with_rblood, (struct monst *,struct obj *, int, int, int, struct attack *));
E int FDECL(shoot_with_gun_katar, (struct monst *,struct obj *, int, int, int, struct attack *));
E int FDECL(hit_with_cblood, (struct monst *,struct obj *, int, int, int, struct attack *));
E int FDECL(hit_with_rreject, (struct monst *,struct obj *, int, int, int, struct attack *));
E int FDECL(hit_with_dance, (struct monst *,struct obj *, int, int, int, struct attack *));
E int FDECL(hit_with_streaming, (struct monst *,struct obj *, int, int, int, struct attack *));
E boolean FDECL(is_serration_vulnerable, (struct monst *));
E boolean FDECL(obj_is_material, (struct obj *, int));
E int FDECL(weapon_skill_type, (struct obj *, struct obj *, boolean));

/* ### zap.c ### */

E char * FDECL(flash_type, (int,int));
E int FDECL(zap_glyph_color, (int));
E int FDECL(wand_adtype, (int));
E int FDECL(zap_hit, (struct monst *,int, BOOLEAN_P));
E int FDECL(bhitm, (struct monst *,struct obj *));
E void FDECL(probe_monster, (struct monst *));
E boolean FDECL(get_obj_location, (struct obj *,xchar *,xchar *,int));
E boolean FDECL(get_mon_location, (struct monst *,xchar *,xchar *,int));
E struct monst *FDECL(get_container_location, (struct obj *obj, int *, int *));
E struct monst *FDECL(montraits, (struct obj *,coord *));
E struct monst *FDECL(revive, (struct obj *, boolean));
E int FDECL(unturn_dead, (struct monst *));
E void FDECL(cancel_item, (struct obj *));
E boolean FDECL(drain_item, (struct obj *));
E boolean FDECL(damage_item, (struct obj *));
E struct obj *FDECL(randpoly_obj, (struct obj *));
E struct obj *FDECL(poly_obj, (struct obj *, int));
E boolean FDECL(obj_resists, (struct obj *,int,int));
E boolean FDECL(obj_shudders, (struct obj *));
E void FDECL(do_osshock, (struct obj *));
E int FDECL(bhito, (struct obj *,struct obj *));
E int FDECL(bhitpile, (struct obj *,int (*)(OBJ_P,OBJ_P),int,int));
E int FDECL(zapcostchance, (struct obj *, struct monst *));
E int FDECL(zappable, (struct obj *));
E void FDECL(zapnodir, (struct obj *));
E int NDECL(dozap);
E struct zapdata * FDECL(basiczap, (struct zapdata *, int, int, int));
E void FDECL(zap, (struct monst *, int, int, int, int, int, struct zapdata *));
E int FDECL(zapyourself, (struct obj *,BOOLEAN_P));
E boolean FDECL(cancel_monst, (struct monst *,struct obj *,
			       BOOLEAN_P,BOOLEAN_P,BOOLEAN_P,int));
E void FDECL(weffects, (struct obj *));
E int NDECL(spell_damage_bonus);
E int NDECL(kraubon);
E const char *FDECL(exclam, (int force));
E void FDECL(hit, (const char *,struct monst *,const char *));
E void FDECL(miss, (const char *,struct monst *));
E struct monst *FDECL(bhit, (int,int,int,int,int (*)(MONST_P,OBJ_P),
			     int (*)(OBJ_P,OBJ_P),struct obj *, boolean *));
E int FDECL(flash_hits_mon, (struct monst *, struct obj *));
E int FDECL(burn_floor_paper, (int,int,BOOLEAN_P,BOOLEAN_P));
E void FDECL(buzz, (int,int,int,int,XCHAR_P,XCHAR_P,int,int,int,int));
E void FDECL(lightning_blind, (struct monst *, int));
E struct monst * FDECL(delouse, (struct monst *,int));
E struct monst * FDECL(delouse_tame, (struct monst *));
E void FDECL(melt_ice, (XCHAR_P,XCHAR_P));
E int FDECL(zap_over_floor, (XCHAR_P,XCHAR_P,int,int,int,boolean *));
E void FDECL(break_boulder, (struct obj *));
E void FDECL(fracture_rock, (struct obj *));
E boolean FDECL(break_statue, (struct obj *));
E void FDECL(break_crate, (struct obj *));
E void FDECL(separate_mass_of_stuff, (struct obj *, boolean));
E int FDECL(resist, (struct monst *,CHAR_P,int,int));
E int FDECL(mm_resist, (struct monst *,struct monst *,int,int));
E boolean FDECL(makewish, (int));
E void FDECL(costly_cancel, (struct obj *));
E void FDECL(revive_egg, (struct obj *));
E int NDECL(allow_artwish);
E boolean NDECL(dowand_refresh);
E int FDECL(android_braindamage, (int, struct monst *, struct monst *, boolean));
E boolean FDECL(nearby_targets, (struct monst *));

#endif /* !MAKEDEFS_C && !LEV_LEX_C */

/* ### unicode.c ### */
E glyph_t FDECL(get_unicode_codepoint, (int));
E int FDECL(pututf8char, (glyph_t));

#undef E

#endif /* EXTERN_H */
