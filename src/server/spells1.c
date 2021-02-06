/* File: spells1.c */

/* Purpose: Spell code (part 1) */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "mangband.h"




/*
 * Helper function -- return a "nearby" race for polymorphing
 *
 * Note that this function is one of the more "dangerous" ones...
 */
s16b poly_r_idx(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	int i, r, lev1, lev2;

	/* Hack -- Uniques never polymorph */
	if (r_ptr->flags1 & RF1_UNIQUE) return (r_idx);

	/* Allowable range of "levels" for resulting monster */
	lev1 = r_ptr->level - ((randint1(20)/randint1(9))+1);
	lev2 = r_ptr->level + ((randint1(20)/randint1(9))+1);

	/* Pick a (possibly new) non-unique race */
	for (i = 0; i < 1000; i++)
	{
		/* Pick a new race, using a level calculation */
		/* Don't base this on "dun_level" */
		/*r = get_mon_num((dun_level + r_ptr->level) / 2 + 5);*/
		r = get_mon_num(r_ptr->level + 5);

		/* Handle failure */
		if (!r) break;

		/* Obtain race */
		r_ptr = &r_info[r];

		/* Ignore unique monsters */
		if (r_ptr->flags1 & RF1_UNIQUE) continue;

		/* Ignore monsters with incompatible levels */
		if ((r_ptr->level < lev1) || (r_ptr->level > lev2)) continue;

		/* Use that index */
		r_idx = r;

		/* Done */
		break;
	}

	/* Result */
	return (r_idx);
}


/*
 * Teleport a monster, normally up to "dis" grids away.
 *
 * Attempt to move the monster at least "dis/2" grids away.
 *
 * But allow variation to prevent infinite loops.
 */
void teleport_away(int m_idx, int dis)
{
	int			ny, nx, oy, ox, Depth, d, i, min;

	bool		look = TRUE;

	monster_type	*m_ptr = &m_list[m_idx];


	/* Paranoia */
	if (!m_ptr->r_idx) return;

	/* Save the old location */
	oy = m_ptr->fy;
	ox = m_ptr->fx;

	Depth = m_ptr->dun_depth;

	/* Minimum distance */
	min = dis / 2;

	/* Look until done */
	while (look)
	{
		/* Verify max distance */
		if (dis > 200) dis = 200;

		/* Try several locations */
		for (i = 0; i < 500; i++)
		{
			/* Pick a (possibly illegal) location */
			while (1)
			{
				ny = rand_spread(oy, dis);
				nx = rand_spread(ox, dis);
				d = distance(oy, ox, ny, nx);
				if ((d >= min) && (d <= dis)) break;
			}

			/* Ignore illegal locations */
			if (!in_bounds(Depth, ny, nx)) continue;

			/* Require "empty" floor space */
			if (!cave_empty_bold(Depth, ny, nx)) continue;

			/* Hack -- no teleport onto glyph of warding */
			if (cave[Depth][ny][nx].feat == FEAT_GLYPH) continue;

			/* No teleporting into vaults and such */
			if (cave[Depth][ny][nx].info & CAVE_ICKY) continue;

			/* This grid looks good */
			look = FALSE;

			/* Stop looking */
			break;
		}

		/* Increase the maximum distance */
		dis = dis * 2;

		/* Decrease the minimum distance */
		min = min / 2;
	}

	/* Update the new location */
	cave[Depth][ny][nx].m_idx = m_idx;

	/* Update the old location */
	cave[Depth][oy][ox].m_idx = 0;

	/* Move the monster */
	m_ptr->fy = ny;
	m_ptr->fx = nx;

	/* Update the monster (new location) */
	update_mon(m_idx, TRUE);

	/* Redraw the old grid */
	everyone_lite_spot(Depth, oy, ox);

	/* Redraw the new grid */
	everyone_lite_spot(Depth, ny, nx);
}


/*
 * Teleport the player to a location up to "dis" grids away.
 *
 * If no such spaces are readily available, the distance may increase.
 * Try very hard to move the player at least a quarter that distance.
 */
void teleport_player(player_type *p_ptr, int dis)
{
	int d, i, min, ox, oy, x = p_ptr->py, y = p_ptr->px;
	int Depth = p_ptr->dun_depth;

	bool look = TRUE;

	/* XXX XXX Hack -- hijack teleport in Arena */
	if (p_ptr->arena_num != -1)
	{
		i = p_ptr->arena_num;
		teleport_player_to(p_ptr, arenas[i].y_1+1+randint1(arenas[i].y_2-arenas[i].y_1-2) , arenas[i].x_1+1+randint1(arenas[i].x_2-arenas[i].x_1-2) );
		return;
	}

	/* Minimum distance */
	min = dis / 2;

	/* Look until done */
	while (look)
	{
		/* Verify max distance */
		if (dis > 200) dis = 200;

		/* Try several locations */
		for (i = 0; i < 500; i++)
		{
			/* Pick a (possibly illegal) location */
			while (1)
			{
				y = rand_spread(p_ptr->py, dis);
				x = rand_spread(p_ptr->px, dis);
				d = distance(p_ptr->py, p_ptr->px, y, x);
				if ((d >= min) && (d <= dis)) break;
			}

			/* Ignore illegal locations */
			if (!in_bounds(Depth, y, x)) continue;

			/* Require floor space if not ghost */
			if (!p_ptr->ghost && !cave_naked_bold(Depth, y, x)) continue;

			/* Require empty space if a ghost */
			if (p_ptr->ghost && cave[Depth][y][x].m_idx) continue;

			/* No teleporting into vaults and such */
			if (cave[Depth][y][x].info & CAVE_ICKY) continue;

			/* This grid looks good */
			look = FALSE;

			/* Stop looking */
			break;
		}

		/* Increase the maximum distance */
		dis = dis * 2;

		/* Decrease the minimum distance */
		min = min / 2;
	}

	/* Sound */
	sound(p_ptr, MSG_TELEPORT);

	/* Save the old location */
	oy = p_ptr->py;
	ox = p_ptr->px;

	/* Hack -- fix store */
	if (p_ptr->store_num != -1)
		send_store_leave(p_ptr);

	/* Move the player */
	p_ptr->py = y;
	p_ptr->px = x;

	/* The player isn't on his old spot anymore */
	cave[Depth][oy][ox].m_idx = 0;

	/* The player is on his new spot */
	cave[Depth][y][x].m_idx = 0 - p_ptr->Ind;

	/* Redraw the old spot */
	everyone_lite_spot(Depth, oy, ox);

	/* Redraw the new spot */
	everyone_lite_spot(Depth, p_ptr->py, p_ptr->px);

	/* Tell everyone to re-calculate visiblity for this player */
	update_player(p_ptr);

	/* Check for new panel (redraw map) */
	verify_panel(p_ptr);

	/* Update stuff */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);
	
	/* Redraw floor */
	p_ptr->redraw |= (PR_FLOOR);

	/* Update the monsters */
	p_ptr->update |= (PU_DISTANCE);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);

	/* Handle stuff XXX XXX XXX */
	handle_stuff(p_ptr);
}



/*
 * Teleport player to a grid near the given location
 *
 * This function is slightly obsessive about correctness.
 * This function allows teleporting into vaults (!)
 */
void teleport_player_to(player_type *p_ptr, int ny, int nx)
{
	int y, x, oy, ox, dis = 0, ctr = 0;
	int Depth = p_ptr->dun_depth;


	/* Find a usable location */
	while (1)
	{
		/* Pick a nearby legal location */
		while (1)
		{
			y = rand_spread(ny, dis);
			x = rand_spread(nx, dis);
			if (in_bounds(Depth, y, x)) break;
		}

		/* Accept "naked" floor grids */
		if (cave_naked_bold(Depth, y, x)) break;

		/* Occasionally advance the distance */
		if (++ctr > (4 * dis * dis + 4 * dis + 1))
		{
			ctr = 0;
			dis++;
		}
	}

	/* Save the old location */
	oy = p_ptr->py;
	ox = p_ptr->px;

	/* Move the player */
	p_ptr->py = y;
	p_ptr->px = x;

	/* The player isn't here anymore */
	cave[Depth][oy][ox].m_idx = 0;

	/* The player is now here */
	cave[Depth][y][x].m_idx = 0 - p_ptr->Ind;

	/* Redraw the old spot */
	everyone_lite_spot(Depth, oy, ox);

	/* Redraw the new spot */
	everyone_lite_spot(Depth, p_ptr->py, p_ptr->px);

	/* Tell everyone to re-calculate visiblity for this player */
	update_player(p_ptr);

	/* Check for new panel (redraw map) */
	verify_panel(p_ptr);

	/* Update stuff */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

	/* Update the monsters */
	p_ptr->update |= (PU_DISTANCE);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);

	/* Handle stuff XXX XXX XXX */
	handle_stuff(p_ptr);
}



/*
 * Teleport the player one level up or down (random when legal)
 *
 * Note that keeping the "players_on_depth" array correct is VERY important,
 * otherwise levels with players still on them might be destroyed, or empty
 * levels could be kept around, wasting memory.
 */
 
 /* in the wilderness, teleport to a neighboring wilderness level. */
 
void teleport_player_level(player_type *p_ptr)
{
	wilderness_type *w_ptr;
	int Depth = p_ptr->dun_depth, new_depth, new_world_x = 0, new_world_y = 0;
	char *msg;

	/* Ironmen OR Arena fighters don't teleport level */
	if (cfg_ironman || p_ptr->arena_num != -1)
	{
		msg_print(p_ptr, "Nothing happens.");
		return;
	}
	
	/* sometimes go down */
	if ((!Depth) || ((randint0(100) < 50) && (Depth < MAX_DEPTH-1) && !is_quest_level(p_ptr, Depth)))
	{
		new_depth = Depth+1;
		msg = "You sink through the floor.";
		p_ptr->new_level_method = LEVEL_RAND;
	}	
	/* else go up */
	else 
	{
		new_depth = Depth-1;
		msg = "You rise up through the ceiling.";
		p_ptr->new_level_method = LEVEL_RAND;
	}
	
	/* If in the wilderness, teleport to a random neighboring level */
	if (Depth < 0)
	{
		w_ptr = &wild_info[Depth];
		/* get a valid neighbor */
		do
		{	
			switch (randint0(4))
			{
				case DIR_NORTH:
					new_depth = world_index(w_ptr->world_x,w_ptr->world_y+1);
					msg = "A gust of wind blows you north.";
					new_world_x = w_ptr->world_x;
					new_world_y = w_ptr->world_y+1;
					break;
				case DIR_EAST:
					new_depth = world_index(w_ptr->world_x+1,w_ptr->world_y);
					msg = "A gust of wind blows you east.";
					new_world_x = w_ptr->world_x+1;
					new_world_y = w_ptr->world_y;
					break;
				case DIR_SOUTH:
					new_depth = world_index(w_ptr->world_x,w_ptr->world_y-1);
					msg = "A gust of wind blows you south.";
					new_world_x = w_ptr->world_x;
					new_world_y = w_ptr->world_y-1;
					break;
				case DIR_WEST:
					new_depth = world_index(w_ptr->world_x-1,w_ptr->world_y);
					msg = "A gust of wind blows you west.";
					new_world_x = w_ptr->world_x-1;
					new_world_y = w_ptr->world_y;
					break;		
			}
		}
		while ((new_depth <= -MAX_WILD) || (new_depth > 0));	
		p_ptr->new_level_method = LEVEL_OUTSIDE_RAND;
		/* update the players new wilderness location */
		p_ptr->world_x = new_world_x;
		p_ptr->world_y = new_world_y;

		/* update the players wilderness map */
		p_ptr->wild_map[(new_depth)/8] |= (1<<((new_depth)%8));
	}	
	
	/* Tell the player */
	msg_print(p_ptr, msg);
	sound(p_ptr, MSG_TPLEVEL);

	/* One less player here */
	players_on_depth[Depth]--;
	
	/* Paranoia, required for merging old saves with new wilderness */
	if (players_on_depth[Depth] < 0) players_on_depth[Depth] = 0;

	/* Remove the player */
	cave[Depth][p_ptr->py][p_ptr->px].m_idx = 0;

	/* Show that he's left */
	everyone_lite_spot(Depth, p_ptr->py, p_ptr->px);

	/* Tell everyone to re-calculate visiblity for this player */
	update_player(p_ptr);

	/* Forget his lite and viewing area */
	forget_lite(p_ptr);
	forget_view(p_ptr);

	Depth = p_ptr->dun_depth = new_depth;

	/* One more player here */
	players_on_depth[Depth]++;

	p_ptr->new_level_flag = TRUE;
}






/*
 * Get a legal "multi-hued" color for drawing "spells"
 */
static byte mh_attr(void)
{
	switch (randint1(9))
	{
		case 1: return (TERM_RED);
		case 2: return (TERM_GREEN);
		case 3: return (TERM_BLUE);
		case 4: return (TERM_YELLOW);
		case 5: return (TERM_ORANGE);
		case 6: return (TERM_VIOLET);
		case 7: return (TERM_L_RED);
		case 8: return (TERM_L_GREEN);
		case 9: return (TERM_L_BLUE);
	}

	return (TERM_WHITE);
}


/*
 * Return a color to use for the bolt/ball spells
 */
static byte spell_color(int type, bool resolve_mh)
{
	/* Analyze */
	switch (type)
	{
		case GF_MISSILE:	return (resolve_mh ? mh_attr() : 0x7F);
		case GF_ACID:		return (TERM_SLATE);
		case GF_ELEC:		return (TERM_BLUE);
		case GF_FIRE:		return (TERM_RED);
		case GF_COLD:		return (TERM_WHITE);
		case GF_POIS:		return (TERM_GREEN);
		case GF_HOLY_ORB:	return (TERM_L_DARK);
		case GF_MANA:		return (TERM_L_DARK);
		case GF_BOULDER:	return (TERM_WHITE);
		case GF_BOLT:		return (TERM_L_WHITE);
		case GF_ARROW:		return (TERM_WHITE);
		case GF_WATER:		return (TERM_SLATE);
		case GF_NETHER:		return (TERM_L_GREEN);
		case GF_CHAOS:		return (resolve_mh ? mh_attr() : 0x7F);
		case GF_DISENCHANT:	return (TERM_VIOLET);
		case GF_NEXUS:		return (TERM_L_RED);
		case GF_CONFUSION:	return (TERM_L_UMBER);
		case GF_SOUND:		return (TERM_YELLOW);
		case GF_SHARDS:		return (TERM_UMBER);
		case GF_FORCE:		return (TERM_UMBER);
		case GF_INERT:		return (TERM_L_WHITE);
		case GF_GRAVITY:	return (TERM_L_WHITE);
		case GF_TIME:		return (TERM_L_BLUE);
		case GF_LITE_WEAK:	return (TERM_ORANGE);
		case GF_LITE:		return (TERM_ORANGE);
		case GF_DARK_WEAK:	return (TERM_L_DARK);
		case GF_DARK:		return (TERM_L_DARK);
		case GF_PLASMA:		return (TERM_RED);
		case GF_METEOR:		return (TERM_RED);
		case GF_ICE:		return (TERM_WHITE);
	}

	/* Standard "color" */
	return (TERM_WHITE);
}






/*
 * Decreases players hit points and sets death flag if necessary
 *
 * XXX XXX XXX Invulnerability needs to be changed into a "shield"
 *
 */
void take_hit(player_type *p_ptr, int damage, cptr hit_from)
{
	int old_chp = p_ptr->chp;
	int warning = (p_ptr->mhp * p_ptr->hitpoint_warn / 10);

	// The "number" that the character is displayed as before the hit
	int old_num = (p_ptr->chp * 95) / (p_ptr->mhp*10); 
	int new_num; 

	if (old_num >= 7) old_num = 10;

	/* Paranoia */
	if (p_ptr->death) return;


	/* Disturb */
	disturb(p_ptr, 1, 0);

	/* Mega-Hack -- Apply "invulnerability" */
	if (p_ptr->invuln && (damage < 9000)) return;

	/* Hurt the player */
	p_ptr->chp -= damage;

	/* Update health bars */
	update_health(0 - p_ptr->Ind);

	/* Display the hitpoints */
	p_ptr->redraw |= (PR_HP);

	/* Figure out of if the player's "number" has changed */
	new_num = (p_ptr->chp * 95) / (p_ptr->mhp*10); 
	if (new_num >= 7) new_num = 10;

	/* If so then refresh everyone's view of this player */
	if (new_num != old_num)
		everyone_lite_spot(p_ptr->dun_depth, p_ptr->py, p_ptr->px);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	/* Dead player */
	if (p_ptr->chp < 0)
	{
		/* Note cause of death */
		/* To preserve the players original (pre-ghost) cause
		   of death, use died_from_list.  To preserve the original
		   depth, use died_from_depth. */
		
		my_strcpy(p_ptr->died_from, hit_from, sizeof(p_ptr->died_from));
		if (!p_ptr->ghost) 
		{
			my_strcpy(p_ptr->died_from_list, hit_from, sizeof(p_ptr->died_from_list));
			p_ptr->died_from_depth = p_ptr->dun_depth;
		}

		/* No longer a winner */
		p_ptr->total_winner = FALSE;

		/* Note death */
		p_ptr->death = TRUE;

		/* Dead */
		return;
	}

	/* Hack -- hitpoint warning */
	if (warning && (p_ptr->chp <= warning))
	{
		/* Hack -- bell on first notice */
		/*if (alert_hitpoint && (old_chp > warning)) bell();*/

		/* HACK -- Message on first notice*/
		if (option_p(p_ptr,ALERT_HITPOINT) && (old_chp > warning)) {
			msg_print(p_ptr, "*** LOW HITPOINT WARNING! ***");
			sound(p_ptr, MSG_HITPOINT_WARN);
			msg_print(p_ptr, NULL);
		}
	}
}





/*
 * Note that amulets, rods, and high-level spell books are immune
 * to "inventory damage" of any kind.  Also sling ammo and shovels.
 */


/*
 * Does a given class of objects (usually) hate acid?
 * Note that acid can either melt or corrode something.
 */
static bool hates_acid(object_type *o_ptr)
{
	/* Analyze the type */
	switch (o_ptr->tval)
	{
		/* Wearable items */
		case TV_ARROW:
		case TV_BOLT:
		case TV_BOW:
		case TV_SWORD:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		case TV_DRAG_ARMOR:
		{
			return (TRUE);
		}

		/* Staffs/Scrolls are wood/paper */
		case TV_STAFF:
		case TV_SCROLL:
		{
			return (TRUE);
		}

		/* Ouch */
		case TV_CHEST:
		{
			return (TRUE);
		}

		/* Junk is useless */
		case TV_SKELETON:
		case TV_BOTTLE:
		case TV_JUNK:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Does a given object (usually) hate electricity?
 */
static bool hates_elec(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_RING:
		case TV_WAND:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Does a given object (usually) hate fire?
 * Hafted/Polearm weapons have wooden shafts.
 * Arrows/Bows are mostly wooden.
 */
static bool hates_fire(object_type *o_ptr)
{
	/* Analyze the type */
	switch (o_ptr->tval)
	{
		/* Wearable */
		case TV_LITE:
		case TV_ARROW:
		case TV_BOW:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		{
			return (TRUE);
		}

		/* Books */
		case TV_MAGIC_BOOK:
		case TV_PRAYER_BOOK:
		{
			return (TRUE);
		}

		/* Chests */
		case TV_CHEST:
		{
			return (TRUE);
		}

		/* Staffs/Scrolls burn */
		case TV_STAFF:
		case TV_SCROLL:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}


/*
 * Does a given object (usually) hate cold?
 */
static bool hates_cold(object_type *o_ptr)
{
	switch (o_ptr->tval)
	{
		case TV_POTION:
		case TV_FLASK:
		case TV_BOTTLE:
		{
			return (TRUE);
		}
	}

	return (FALSE);
}









/*
 * Melt something
 */
static int set_acid_destroy(object_type *o_ptr)
{
        u32b f1, f2, f3;
	if (!hates_acid(o_ptr)) return (FALSE);
	object_flags(o_ptr, &f1, &f2, &f3);
	if (f3 & TR3_IGNORE_ACID) return (FALSE);
	return (TRUE);
}


/*
 * Electrical damage
 */
static int set_elec_destroy(object_type *o_ptr)
{
        u32b f1, f2, f3;
	if (!hates_elec(o_ptr)) return (FALSE);
        object_flags(o_ptr, &f1, &f2, &f3);
	if (f3 & TR3_IGNORE_ELEC) return (FALSE);
	return (TRUE);
}


/*
 * Burn something
 */
static int set_fire_destroy(object_type *o_ptr)
{
        u32b f1, f2, f3;
	if (!hates_fire(o_ptr)) return (FALSE);
        object_flags(o_ptr, &f1, &f2, &f3);
	if (f3 & TR3_IGNORE_FIRE) return (FALSE);
	return (TRUE);
}


/*
 * Freeze things
 */
static int set_cold_destroy(object_type *o_ptr)
{
        u32b f1, f2, f3;
	if (!hates_cold(o_ptr)) return (FALSE);
	object_flags(o_ptr, &f1, &f2, &f3);
	if (f3 & TR3_IGNORE_COLD) return (FALSE);
	return (TRUE);
}




/*
 * This seems like a pretty standard "typedef"
 */
typedef int (*inven_func)(object_type *);

/*
 * Destroys a type of item on a given percent chance
 * Note that missiles are no longer necessarily all destroyed
 * Destruction taken from "melee.c" code for "stealing".
 * Returns number of items destroyed.
 */
static int inven_damage(player_type *p_ptr, inven_func typ, int perc)
{
	int		i, j, k, amt;

	object_type	*o_ptr;

	char	o_name[80];


	/* Count the casualties */
	k = 0;

	/* Scan through the slots backwards */
	for (i = 0; i < INVEN_PACK; i++)
	{
		/* Get the item in that slot */
		o_ptr = &p_ptr->inventory[i];

		/* Hack -- for now, skip artifacts */
		if (artifact_p(o_ptr)) continue;

		/* Give this item slot a shot at death */
		if ((*typ)(o_ptr))
		{
			/* Count the casualties */
			for (amt = j = 0; j < o_ptr->number; ++j)
			{
				if (randint0(100) < perc) amt++;
			}

			/* Some casualities */
			if (amt)
			{
				/* Get a description */
				object_desc(p_ptr, o_name, sizeof(o_name), o_ptr, FALSE, 3);

				/* Message */
				msg_format(p_ptr, "%sour %s (%c) %s destroyed!",
				           ((o_ptr->number > 1) ?
				            ((amt == o_ptr->number) ? "All of y" :
				             (amt > 1 ? "Some of y" : "One of y")) : "Y"),
				           o_name, index_to_label(i),
				           ((amt > 1) ? "were" : "was"));
				sound(p_ptr, MSG_DESTROY);

				/* Destroy "amt" items */
				inven_item_increase(p_ptr, i, -amt);
				inven_item_optimize(p_ptr, i);

				/* Count the casualties */
				k += amt;
			}
		}
	}

	/* Return the casualty count */
	return (k);
}




/*
 * Acid has hit the player, attempt to affect some armor.
 *
 * Note that the "base armor" of an object never changes.
 *
 * If any armor is damaged (or resists), the player takes less damage.
 */
static int minus_ac(player_type *p_ptr)
{
	object_type		*o_ptr = NULL;

        u32b		f1, f2, f3;

	char		o_name[80];

	int item;

	/* Pick a (possibly empty) inventory slot */
	item = randint1(6);
	switch (item)
	{
		case 1: o_ptr = &p_ptr->inventory[INVEN_BODY]; break;
		case 2: o_ptr = &p_ptr->inventory[INVEN_ARM]; break;
		case 3: o_ptr = &p_ptr->inventory[INVEN_OUTER]; break;
		case 4: o_ptr = &p_ptr->inventory[INVEN_HANDS]; break;
		case 5: o_ptr = &p_ptr->inventory[INVEN_HEAD]; break;
		case 6: o_ptr = &p_ptr->inventory[INVEN_FEET]; break;
	}

	/* Nothing to damage */
	if (!o_ptr->k_idx) return (FALSE);

	/* No damage left to be done */
	if (o_ptr->ac + o_ptr->to_a <= 0) return (FALSE);


	/* Describe */
	object_desc(p_ptr, o_name, sizeof(o_name), o_ptr, FALSE, 0);

	/* Extract the flags */
    object_flags(o_ptr, &f1, &f2, &f3);

	/* Object resists */
	if (f3 & TR3_IGNORE_ACID)
	{
		msg_format(p_ptr, "Your %s is unaffected!", o_name);

		return (TRUE);
	}

	/* Message */
	msg_format(p_ptr, "Your %s is damaged!", o_name);

	/* Damage the item */
	o_ptr->to_a--;

	/* Calculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Redraw inventory slot */
	p_ptr->redraw_inven |= (1LL << item);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	/* Item was damaged */
	return (TRUE);
}


/*
 * Hurt the player with Acid
 */
void acid_dam(player_type *p_ptr, int dam, cptr kb_str)
{
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* Total Immunity */
	if (p_ptr->immune_acid || (dam <= 0)) return;

	/* Resist the damage */
	if (p_ptr->resist_acid) dam = (dam + 2) / 3;
	if (p_ptr->oppose_acid) dam = (dam + 2) / 3;

	/* If any armor gets hit, defend the player */
	if (minus_ac(p_ptr)) dam = (dam + 1) / 2;

	/* Take damage */
	take_hit(p_ptr, dam, kb_str);

	/* Inventory damage */
	inven_damage(p_ptr, set_acid_destroy, inv);
}


/*
 * Hurt the player with electricity
 */
void elec_dam(player_type *p_ptr, int dam, cptr kb_str)
{
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* Total immunity */
	if (p_ptr->immune_elec || (dam <= 0)) return;

	/* Resist the damage */
	if (p_ptr->oppose_elec) dam = (dam + 2) / 3;
	if (p_ptr->resist_elec) dam = (dam + 2) / 3;

	/* Take damage */
	take_hit(p_ptr, dam, kb_str);

	/* Inventory damage */
	inven_damage(p_ptr, set_elec_destroy, inv);
}




/*
 * Hurt the player with Fire
 */
void fire_dam(player_type *p_ptr, int dam, cptr kb_str)
{
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* Totally immune */
	if (p_ptr->immune_fire || (dam <= 0)) return;

	/* Resist the damage */
	if (p_ptr->resist_fire) dam = (dam + 2) / 3;
	if (p_ptr->oppose_fire) dam = (dam + 2) / 3;

	/* Take damage */
	take_hit(p_ptr, dam, kb_str);

	/* Inventory damage */
	inven_damage(p_ptr, set_fire_destroy, inv);
}


/*
 * Hurt the player with Cold
 */
void cold_dam(player_type *p_ptr, int dam, cptr kb_str)
{
	int inv = (dam < 30) ? 1 : (dam < 60) ? 2 : 3;

	/* Total immunity */
	if (p_ptr->immune_cold || (dam <= 0)) return;

	/* Resist the damage */
	if (p_ptr->resist_cold) dam = (dam + 2) / 3;
	if (p_ptr->oppose_cold) dam = (dam + 2) / 3;

	/* Take damage */
	take_hit(p_ptr, dam, kb_str);

	/* Inventory damage */
	inven_damage(p_ptr, set_cold_destroy, inv);
}





/*
 * Increases a stat by one randomized level		-RAK-
 *
 * Note that this function (used by stat potions) now restores
 * the stat BEFORE increasing it.
 */
bool inc_stat(player_type *p_ptr, int stat)
{
	int value, gain;

	/* Then augment the current/max stat */
	value = p_ptr->stat_cur[stat];

	/* Cannot go above 18/100 */
	if (value < 18+100)
	{
		/* Gain one (sometimes two) points */
		if (value < 18)
		{
			gain = ((randint0(100) < 75) ? 1 : 2);
			value += gain;
		}

		/* Gain 1/6 to 1/3 of distance to 18/100 */
		else if (value < 18+98)
		{
			/* Approximate gain value */
			gain = (((18+100) - value) / 2 + 3) / 2;

			/* Paranoia */
			if (gain < 1) gain = 1;

			/* Apply the bonus */
			value += randint1(gain) + gain / 2;

			/* Maximal value */
			if (value > 18+99) value = 18 + 99;
		}

		/* Gain one point at a time */
		else
		{
			value++;
		}

		/* Save the new value */
		p_ptr->stat_cur[stat] = value;

		/* Bring up the maximum too */
		if (value > p_ptr->stat_max[stat])
		{
			p_ptr->stat_max[stat] = value;
		}

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Success */
		return (TRUE);
	}

	/* Nothing to gain */
	return (FALSE);
}



/*
 * Decreases a stat by an amount indended to vary from 0 to 100 percent.
 *
 * Amount could be a little higher in extreme cases to mangle very high
 * stats from massive assaults.  -CWS
 *
 * Note that "permanent" means that the *given* amount is permanent,
 * not that the new value becomes permanent.  This may not work exactly
 * as expected, due to "weirdness" in the algorithm, but in general,
 * if your stat is already drained, the "max" value will not drop all
 * the way down to the "cur" value.
 */
bool dec_stat(player_type *p_ptr, int stat, int amount, int permanent)
{
	int cur, max, loss, same, res = FALSE;


	/* Acquire current value */
	cur = p_ptr->stat_cur[stat];
	max = p_ptr->stat_max[stat];

	/* Note when the values are identical */
	same = (cur == max);

	/* Damage "current" value */
	if (cur > 3)
	{
		/* Handle "low" values */
		if (cur <= 18)
		{
			if (amount > 90) cur--;
			if (amount > 50) cur--;
			if (amount > 20) cur--;
			cur--;
		}

		/* Handle "high" values */
		else
		{
			/* Hack -- Decrement by a random amount between one-quarter */
			/* and one-half of the stat bonus times the percentage, with a */
			/* minimum damage of half the percentage. -CWS */
			loss = (((cur-18) / 2 + 1) / 2 + 1);

			/* Paranoia */
			if (loss < 1) loss = 1;

			/* Randomize the loss */
			loss = ((randint1(loss) + loss) * amount) / 100;

			/* Maximal loss */
			if (loss < amount/2) loss = amount/2;

			/* Lose some points */
			cur = cur - loss;

			/* Hack -- Only reduce stat to 17 sometimes */
			if (cur < 18) cur = (amount <= 20) ? 18 : 17;
		}

		/* Prevent illegal values */
		if (cur < 3) cur = 3;

		/* Something happened */
		if (cur != p_ptr->stat_cur[stat]) res = TRUE;
	}

	/* Damage "max" value */
	if (permanent && (max > 3))
	{
		/* Handle "low" values */
		if (max <= 18)
		{
			if (amount > 90) max--;
			if (amount > 50) max--;
			if (amount > 20) max--;
			max--;
		}

		/* Handle "high" values */
		else
		{
			/* Hack -- Decrement by a random amount between one-quarter */
			/* and one-half of the stat bonus times the percentage, with a */
			/* minimum damage of half the percentage. -CWS */
			loss = (((max-18) / 2 + 1) / 2 + 1);
			loss = ((randint1(loss) + loss) * amount) / 100;
			if (loss < amount/2) loss = amount/2;

			/* Lose some points */
			max = max - loss;

			/* Hack -- Only reduce stat to 17 sometimes */
			if (max < 18) max = (amount <= 20) ? 18 : 17;
		}

		/* Hack -- keep it clean */
		if (same || (max < cur)) max = cur;

		/* Something happened */
		if (max != p_ptr->stat_max[stat]) res = TRUE;
	}

	/* Apply changes */
	if (res)
	{
		/* Actually set the stat to its new value. */
		p_ptr->stat_cur[stat] = cur;
		p_ptr->stat_max[stat] = max;

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);
	}

	/* Done */
	return (res);
}


/*
 * Restore a stat.  Return TRUE only if this actually makes a difference.
 */
bool res_stat(player_type *p_ptr, int stat)
{
	/* Restore if needed */
	if (p_ptr->stat_cur[stat] != p_ptr->stat_max[stat])
	{
		/* Restore */
		p_ptr->stat_cur[stat] = p_ptr->stat_max[stat];

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Success */
		return (TRUE);
	}

	/* Nothing to restore */
	return (FALSE);
}




/*
 * Apply disenchantment to the player's stuff
 *
 * XXX XXX XXX This function is also called from the "melee" code
 *
 * The "mode" is currently unused.
 *
 * Return "TRUE" if the player notices anything
 */
bool apply_disenchant(player_type *p_ptr, int mode)
{
	int			t = 0;

	object_type		*o_ptr;

	char		o_name[80];


	/* Unused */
	mode = mode;


	/* Pick a random slot */
	switch (randint1(8))
	{
		case 1: t = INVEN_WIELD; break;
		case 2: t = INVEN_BOW; break;
		case 3: t = INVEN_BODY; break;
		case 4: t = INVEN_OUTER; break;
		case 5: t = INVEN_ARM; break;
		case 6: t = INVEN_HEAD; break;
		case 7: t = INVEN_HANDS; break;
		case 8: t = INVEN_FEET; break;
	}

	/* Get the item */
	o_ptr = &p_ptr->inventory[t];

	/* No item, nothing happens */
	if (!o_ptr->k_idx) return (FALSE);


	/* Nothing to disenchant */
	if ((o_ptr->to_h <= 0) && (o_ptr->to_d <= 0) && (o_ptr->to_a <= 0))
	{
		/* Nothing to notice */
		return (FALSE);
	}


	/* Describe the object */
	object_desc(p_ptr, o_name, sizeof(o_name), o_ptr, FALSE, 0);


	/* Artifacts have 60% chance to resist */
	if (artifact_p(o_ptr) && (randint0(100) < 60))
	{
		/* Message */
		msg_format(p_ptr, "Your %s (%c) resist%s disenchantment!",
		           o_name, index_to_label(t),
		           ((o_ptr->number != 1) ? "" : "s"));

		/* Notice */
		return (TRUE);
	}

	/* Apply disenchantment, depending on which kind of equipment */
	if (t == INVEN_WIELD || t == INVEN_BOW)
	{
		/* Disenchant to-hit */
		if (o_ptr->to_h > 0) o_ptr->to_h--;
		if ((o_ptr->to_h > 5) && (randint0(100) < 20)) o_ptr->to_h--;

		/* Disenchant to-dam */
		if (o_ptr->to_d > 0) o_ptr->to_d--;
		if ((o_ptr->to_d > 5) && (randint0(100) < 20)) o_ptr->to_d--;
	}
	else
	{
		/* Disenchant to-ac */
		if (o_ptr->to_a > 0) o_ptr->to_a--;
		if ((o_ptr->to_a > 5) && (randint0(100) < 20)) o_ptr->to_a--;
	}

	/* Message */
	msg_format(p_ptr, "Your %s (%c) %s disenchanted!",
	           o_name, index_to_label(t),
	           ((o_ptr->number != 1) ? "were" : "was"));

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Redraw inventory slot */
	p_ptr->redraw_inven |= (1LL << t);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	/* Notice */
	return (TRUE);
}


/*
 * Apply Nexus
 */
static void apply_nexus(player_type *p_ptr, monster_type *m_ptr)
{
	int max1, cur1, max2, cur2, ii, jj;

	switch (randint1(7))
	{
		case 1: case 2: case 3:
		{
			teleport_player(p_ptr, 200);
			break;
		}

		case 4: case 5:
		{
			teleport_player_to(p_ptr, m_ptr->fy, m_ptr->fx);
			break;
		}

		case 6:
		{
			if (randint0(100) < p_ptr->skill_sav)
			{
				msg_print(p_ptr, "You resist the effects!");
				break;
			}

			/* Teleport Level */
			teleport_player_level(p_ptr);
			break;
		}

		case 7:
		{
			if (randint0(100) < p_ptr->skill_sav)
			{
				msg_print(p_ptr, "You resist the effects!");
				break;
			}

			msg_print(p_ptr, "Your body starts to scramble...");

			/* Pick a pair of stats */
			ii = randint0(6);
			for (jj = ii; jj == ii; jj = randint0(6)) /* loop */;

			max1 = p_ptr->stat_max[ii];
			cur1 = p_ptr->stat_cur[ii];
			max2 = p_ptr->stat_max[jj];
			cur2 = p_ptr->stat_cur[jj];

			p_ptr->stat_max[ii] = max2;
			p_ptr->stat_cur[ii] = cur2;
			p_ptr->stat_max[jj] = max1;
			p_ptr->stat_cur[jj] = cur1;

			p_ptr->update |= (PU_BONUS);

			break;
		}
	}
}


/*
 * Apply Polymorph
 */
 
/* Ho Ho Ho... I really want this to have a chance of turning people into
   a fruit bat,  but something tells me I should put that off until the next version...
   
   I CANT WAIT. DOING FRUIT BAT RIGHT NOW!!! 
   -APD-
*/   
static void apply_morph(player_type *p_ptr, int power, char * killer)
{
	int max1, cur1, max2, cur2, ii, jj;

	switch (randint1(2))
	{
		/*case 1: msg_print(p_ptr, "You resist the effects!"); break;*/
		case 1:
		{
			if (randint0(40 + power*2) < p_ptr->skill_sav)
			{
				msg_print(p_ptr, "You resist the effects!");
				break;
			}

			msg_print(p_ptr, "Your body starts to scramble...");

			/* Pick a pair of stats */
			ii = randint0(A_MAX);
			for (jj = ii; jj == ii; jj = randint0(A_MAX)) /* loop */;

			max1 = p_ptr->stat_max[ii];
			cur1 = p_ptr->stat_cur[ii];
			max2 = p_ptr->stat_max[jj];
			cur2 = p_ptr->stat_cur[jj];

			p_ptr->stat_max[ii] = max2;
			p_ptr->stat_cur[ii] = cur2;
			p_ptr->stat_max[jj] = max1;
			p_ptr->stat_cur[jj] = cur1;

			p_ptr->update |= (PU_BONUS);

			break;
		}
		case 2:
		{
			if (!p_ptr->fruit_bat)
			{
		
				if (randint0(10 + power * 4) < p_ptr->skill_sav)
				{
					msg_print(p_ptr, "You resist the effects!");
				}
				else
				{
					/* FRUIT BAT!!!!!! */
				
					msg_print(p_ptr, "You have been turned into a fruit bat!");
					my_strcpy(p_ptr->died_from, killer, sizeof(p_ptr->died_from));
					p_ptr->fruit_bat = -1;
					player_death(p_ptr);
				}	
			}
			else				
			{	/* no saving throw for being restored..... */
				msg_print(p_ptr, "You have been restored!");
				p_ptr->fruit_bat = 0;
				p_ptr->update |= (PU_BONUS | PU_HP);
			}
								
		}
	}
}





/*
 * Hack -- track "affected" monsters
 */
static int project_m_n;
static int project_m_x;
static int project_m_y;



/*
 * We are called from "project()" to "damage" terrain features
 *
 * We are called both for "beam" effects and "ball" effects.
 *
 * The "r" parameter is the "distance from ground zero".
 *
 * Note that we determine if the player can "see" anything that happens
 * by taking into account: blindness, line-of-sight, and illumination.
 *
 * We return "TRUE" if the effect of the projection is "obvious".
 *
 * XXX XXX XXX We also "see" grids which are "memorized", probably a hack
 *
 * XXX XXX XXX Perhaps we should affect doors?
 */
static bool project_f(int who, int r, int Depth, int y, int x, int dam, int typ)
{
	cave_type	*c_ptr = &cave[Depth][y][x];

	bool	obvious = FALSE;

	player_type *p_ptr = (who < 0) ? Players[0 - who] : NULL;
	bool quiet = ((p_ptr == NULL) ? TRUE : FALSE);

	int		div;

	byte *w_ptr = (quiet ? NULL : &p_ptr->cave_flag[y][x]);

	/* Extract radius */
	div = r + 1;

	/* Decrease damage */
	dam = dam / div;

	/* Analyze the type */
	switch (typ)
	{
		/* Ignore most effects */
		case GF_ACID:
		case GF_ELEC:
		case GF_COLD:
		case GF_PLASMA:
		case GF_METEOR:
		case GF_ICE:
		case GF_SHARDS:
		case GF_FORCE:
		case GF_SOUND:
		case GF_MANA:
		case GF_HOLY_ORB:
		{
			break;
		}

		/* Burn trees and grass */
		case GF_FIRE:
		{
			/* Destroy trees */
			if (c_ptr->feat == FEAT_TREE)
			{
				/* Hack -- special message */
				if (!quiet && player_can_see_bold(p_ptr, y, x))
				{
					msg_print(p_ptr, "The tree burns to the ground!");
					obvious = TRUE;
				}

				/* Destroy the tree */
				c_ptr->feat = FEAT_DIRT;
				if (Depth == 0) trees_in_town--;
			}

			/* Burn grass */
			if (c_ptr->feat == FEAT_GRASS)
			{
				/* Destroy the grass */
				cave_set_feat(Depth, y, x, FEAT_DIRT);
			}

			/* "Reset" fields */
			if ((c_ptr->feat >= FEAT_CROP_HEAD) &&
			    (c_ptr->feat <= FEAT_CROP_TAIL))
			{
				/* Reset to generic field */
				cave_set_feat(Depth, y, x, FEAT_CROP);
			}

			break;
		}

		/* Destroy Traps (and Locks) */
		case GF_KILL_TRAP:
		{
			/* Reveal secret doors */
			if (c_ptr->feat == FEAT_SECRET)
			{
				place_closed_door(Depth, y, x);

				/* Check line of sight */
				if (player_has_los_bold(p_ptr, y, x))
				{
					obvious = TRUE;
				}
			}

			/* Destroy traps */
			if ((c_ptr->feat == FEAT_INVIS) ||
			    ((c_ptr->feat >= FEAT_TRAP_HEAD) &&
			     (c_ptr->feat <= FEAT_TRAP_TAIL)))
			{
				/* Hack -- special message */
				if (!quiet && player_can_see_bold(p_ptr, y, x))
				{
					msg_print(p_ptr, "There is a bright flash of light!");
					obvious = TRUE;
				}
				/* Inform other nearby players */
				msg_print_near(p_ptr, "There is a bright flash of light!");

				/* Forget the trap */
				everyone_forget_spot(Depth, y, x);

				/* Destroy the trap */
				cave_set_feat(Depth, y, x, FEAT_FLOOR);
			}

			/* Locked doors are unlocked */
			else if ((c_ptr->feat >= FEAT_DOOR_HEAD + 0x01) &&
			         (c_ptr->feat <= FEAT_DOOR_HEAD + 0x07))
			{
				/* Unlock the door */
				cave_set_feat(Depth, y, x, FEAT_DOOR_HEAD + 0x00);

				/* Notice */
				if (!quiet && player_has_los_bold(p_ptr, y, x))
				{
					msg_print(p_ptr, "Click!");
					obvious = TRUE;
				}
			}

			break;
		}

		/* Destroy Doors (and traps) */
		case GF_KILL_DOOR:
		{
			/* Destroy all doors and traps */
			if ((c_ptr->feat == FEAT_INVIS) ||
			    (c_ptr->feat == FEAT_OPEN) ||
			    (c_ptr->feat == FEAT_BROKEN) ||
			    ((c_ptr->feat >= FEAT_TRAP_HEAD) &&
			     (c_ptr->feat <= FEAT_TRAP_TAIL)) ||
			    ((c_ptr->feat >= FEAT_DOOR_HEAD) &&
			     (c_ptr->feat <= FEAT_DOOR_TAIL)))
			{
				/* Hack -- special message */
				if (!quiet && player_can_see_bold(p_ptr, y, x))
				{
					msg_print(p_ptr, "There is a bright flash of light!");
					obvious = TRUE;
				}

				/* Forget the wall */
				everyone_forget_spot(Depth, y, x);

				/* Destroy the feature */
				cave_set_feat(Depth, y, x, FEAT_FLOOR);
			}

			break;
		}

		/* Destroy walls (and doors) */
		case GF_KILL_WALL:
		{
			/* Non-walls (etc) */
			if (cave_floor_bold(Depth, y, x)) break;

			/* Permanent walls */
			if ( (c_ptr->feat >= FEAT_PERM_EXTRA) || (c_ptr->feat == FEAT_PERM_CLEAR) ) break;

			/* Granite */
			if (c_ptr->feat >= FEAT_WALL_EXTRA)
			{
				/* Message */
				if (!quiet && (*w_ptr & CAVE_MARK))
				{
					msg_print(p_ptr, "The wall turns into mud!");
					obvious = TRUE;
				}

				/* Forget the wall */
				everyone_forget_spot(Depth, y, x);

				/* Destroy the wall */
				if (Depth > 0)
					cave_set_feat(Depth, y, x, FEAT_FLOOR);
				else
					cave_set_feat(Depth, y, x, FEAT_MUD);
			}

			/* Quartz / Magma with treasure */
			else if (c_ptr->feat >= FEAT_MAGMA_H)
			{
				/* Message */
				if (!quiet && (*w_ptr & CAVE_MARK))
				{
					msg_print(p_ptr, "The vein turns into mud!");
					msg_print(p_ptr, "You have found something!");
					obvious = TRUE;
				}

				/* Forget the wall */
				everyone_forget_spot(Depth, y, x);

				/* Destroy the wall */
				if (Depth > 0)
					cave_set_feat(Depth, y, x, FEAT_FLOOR);
				else
					cave_set_feat(Depth, y, x, FEAT_MUD);

				/* Place some gold */
				place_gold(Depth, y, x);
			}

			/* Quartz / Magma */
			else if (c_ptr->feat >= FEAT_MAGMA)
			{
				/* Message */
				if (!quiet && (*w_ptr & CAVE_MARK))
				{
					msg_print(p_ptr, "The vein turns into mud!");
					obvious = TRUE;
				}

				/* Forget the wall */
				everyone_forget_spot(Depth, y, x);

				/* Destroy the wall */
				if (Depth > 0)
					cave_set_feat(Depth, y, x, FEAT_FLOOR);
				else
					cave_set_feat(Depth, y, x, FEAT_MUD);
			}

			/* Rubble */
			else if (c_ptr->feat == FEAT_RUBBLE)
			{
				/* Message */
				if (!quiet && (*w_ptr & CAVE_MARK))
				{
					msg_print(p_ptr, "The rubble turns into mud!");
					obvious = TRUE;
				}

				/* Forget the wall */
				everyone_forget_spot(Depth, y, x);

				/* Destroy the rubble */
				if (Depth > 0)
					cave_set_feat(Depth, y, x, FEAT_FLOOR);
				else
					cave_set_feat(Depth, y, x, FEAT_MUD);

				/* Hack -- place an object */
				if (randint0(100) < 10)
				{
					/* Found something */
					if (!quiet && player_can_see_bold(p_ptr, y, x))
					{
						msg_print(p_ptr, "There was something buried in the rubble!");
						obvious = TRUE;
					}

					/* Place gold */
					place_object(Depth, y, x, FALSE, FALSE, ORIGIN_RUBBLE);
				}
			}

			/* House doors are immune */
			else if (c_ptr->feat >= FEAT_HOME_HEAD && c_ptr->feat <= FEAT_HOME_TAIL)
			{
				/* Message */
				if (!quiet && (*w_ptr & CAVE_MARK))
				{
					msg_print(p_ptr, "The door resists.");
					obvious = TRUE;
				}
			}

			/* Destroy doors (and secret doors) */
			else if (c_ptr->feat >= FEAT_DOOR_HEAD)
			{
				/* Hack -- special message */
				if (!quiet && (*w_ptr & CAVE_MARK))
				{
					msg_print(p_ptr, "The door turns into mud!");
					obvious = TRUE;
				}

				/* Destroy the feature */
				if (Depth > 0)
					cave_set_feat(Depth, y, x, FEAT_FLOOR);
				else
					cave_set_feat(Depth, y, x, FEAT_DIRT);
			}

#if 0 /* Moved elsewhere, see above */
			/* Forget the wall */
			everyone_forget_spot(Depth, y, x);
	
			/* Notice */
			note_spot_depth(Depth, y, x);

			/* Redraw */
			everyone_lite_spot(Depth, y, x);

			/* Update some things */
			spot_updates(Depth, y, x, (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS));
#endif
			break;
		}

		/* Make doors */
		case GF_MAKE_DOOR:
		{
			/* Require a "naked" floor grid */
			if (!cave_naked_bold(Depth, y, x)) break;

			/* Create a closed door */
			cave_set_feat(Depth, y, x, FEAT_DOOR_HEAD + 0x00);

			if (!quiet)
			{
				/* Observe */
				if (*w_ptr & CAVE_MARK) obvious = TRUE;
			}

			break;
		}

		/* Make traps */
		case GF_MAKE_TRAP:
		{
			/* Require a "naked" floor grid */
			if (!cave_naked_bold(Depth, y, x)) break;

			/* Place a trap */
			place_trap(Depth, y, x);

			/* Hack -- only the current player gets to know */
			if (!quiet)
			{
				/* Notice */
				note_spot(p_ptr, y, x);

				/* Redraw */
				lite_spot(p_ptr, y, x);
			}

			break;
		}

		/* Lite up the grid */
		case GF_LITE_WEAK:
		case GF_LITE:
		{
			/* Turn on the light */
			c_ptr->info |= CAVE_GLOW;

			/* Notice */
			note_spot_depth(Depth, y, x);

			/* Redraw */
			everyone_lite_spot(Depth, y, x);

			/* Trigger updates */
			spot_updates(Depth, y, x, (PU_VIEW | PU_LITE | PU_UN_VIEW | PU_UN_LITE | PU_MONSTERS));

			if (!quiet && player_can_see_bold(p_ptr, y, x))
			{
				/* Observe */
				if (!p_ptr->blind) obvious = TRUE;
			}

			/* Mega-Hack -- Update the monster in the affected grid */
			/* This allows "spear of light" (etc) to work "correctly" */
			if (c_ptr->m_idx > 0) update_mon(c_ptr->m_idx, FALSE);

			break;
		}

		/* Darken the grid */
		case GF_DARK_WEAK:
		case GF_DARK:
		{
			/* Notice */
			if (!quiet && player_can_see_bold(p_ptr, y, x)) obvious = TRUE;

			/* Turn off the light. */
			c_ptr->info &= ~CAVE_GLOW;

			/* Hack -- Forget "boring" grids */
			if (is_boring(c_ptr->feat))
			{
				/* Forget */
				everyone_forget_spot(Depth, y, x);

				/* Notice */
				note_spot_depth(Depth, y, x);
			}

			/* Redraw */
			everyone_lite_spot(Depth, y, x);

			/* Trigger updates */
			spot_updates(Depth, y, x, (PU_VIEW | PU_LITE | PU_UN_VIEW | PU_UN_LITE | PU_MONSTERS));

			/* Mega-Hack -- Update the monster in the affected grid */
			/* This allows "spear of light" (etc) to work "correctly" */
			if (c_ptr->m_idx > 0) update_mon(c_ptr->m_idx, FALSE);

			/* All done */
			break;
		}
	}

	/* Return "Anything seen?" */
	return (obvious);
}



/*
 * We are called from "project()" to "damage" objects
 *
 * We are called both for "beam" effects and "ball" effects.
 *
 * Perhaps we should only SOMETIMES damage things on the ground.
 *
 * The "r" parameter is the "distance from ground zero".
 *
 * Note that we determine if the player can "see" anything that happens
 * by taking into account: blindness, line-of-sight, and illumination.
 *
 * XXX XXX XXX We also "see" grids which are "memorized", probably a hack
 *
 * We return "TRUE" if the effect of the projection is "obvious".
 */
static bool project_i(int who, int r, int Depth, int y, int x, int dam, int typ)
{
	player_type	*p_ptr = (who < 0) ? Players[0 - who] : NULL;

	cave_type	*c_ptr = &cave[Depth][y][x];

	object_type	*o_ptr = &o_list[c_ptr->o_idx];

	bool	obvious = FALSE;

	bool	is_art = FALSE;
	bool	ignore = FALSE;
	bool	plural = FALSE;
	bool	do_kill = FALSE;

	bool quiet = (p_ptr == NULL) ? TRUE : FALSE;

	cptr	note_kill = NULL;

        u32b	f1, f2, f3;

	char	o_name[80];

	int		div;


	/* Nothing here */
	if (!c_ptr->o_idx) return (FALSE);

	/* Extract radius */
	div = r + 1;

	/* Adjust damage */
	dam = dam / div;


	/* XXX XXX */
	who = who ? who : 0;


	/* Extract the flags */
        object_flags(o_ptr, &f1, &f2, &f3);

	/* Get the "plural"-ness */
	if (o_ptr->number > 1) plural = TRUE;

	/* Check for artifact */
	if (artifact_p(o_ptr)) is_art = TRUE;

	/* Analyze the type */
	switch (typ)
	{
		/* Acid -- Lots of things */
		case GF_ACID:
		{
			if (hates_acid(o_ptr))
			{
				do_kill = TRUE;
				note_kill = (plural ? " melt!" : " melts!");
				if (f3 & TR3_IGNORE_ACID) ignore = TRUE;
			}
			break;
		}

		/* Elec -- Rings and Wands */
		case GF_ELEC:
		{
			if (hates_elec(o_ptr))
			{
				do_kill = TRUE;
				note_kill = (plural ? " are destroyed!" : " is destroyed!");
				if (f3 & TR3_IGNORE_ELEC) ignore = TRUE;
			}
			break;
		}

		/* Fire -- Flammable objects */
		case GF_FIRE:
		{
			if (hates_fire(o_ptr))
			{
				do_kill = TRUE;
				note_kill = (plural ? " burn up!" : " burns up!");
				if (f3 & TR3_IGNORE_FIRE) ignore = TRUE;
			}
			break;
		}

		/* Cold -- potions and flasks */
		case GF_COLD:
		{
			if (hates_cold(o_ptr))
			{
				note_kill = (plural ? " shatter!" : " shatters!");
				do_kill = TRUE;
				if (f3 & TR3_IGNORE_COLD) ignore = TRUE;
			}
			break;
		}

		/* Fire + Elec */
		case GF_PLASMA:
		{
			if (hates_fire(o_ptr))
			{
				do_kill = TRUE;
				note_kill = (plural ? " burn up!" : " burns up!");
				if (f3 & TR3_IGNORE_FIRE) ignore = TRUE;
			}
			if (hates_elec(o_ptr))
			{
				ignore = FALSE;
				do_kill = TRUE;
				note_kill = (plural ? " are destroyed!" : " is destroyed!");
				if (f3 & TR3_IGNORE_ELEC) ignore = TRUE;
			}
			break;
		}

		/* Fire + Cold */
		case GF_METEOR:
		{
			if (hates_fire(o_ptr))
			{
				do_kill = TRUE;
				note_kill = (plural ? " burn up!" : " burns up!");
				if (f3 & TR3_IGNORE_FIRE) ignore = TRUE;
			}
			if (hates_cold(o_ptr))
			{
				ignore = FALSE;
				do_kill = TRUE;
				note_kill = (plural ? " shatter!" : " shatters!");
				if (f3 & TR3_IGNORE_COLD) ignore = TRUE;
			}
			break;
		}

		/* Hack -- break potions and such */
		case GF_ICE:
		case GF_SHARDS:
		case GF_FORCE:
		case GF_SOUND:
		{
			if (hates_cold(o_ptr))
			{
				note_kill = (plural ? " shatter!" : " shatters!");
				do_kill = TRUE;
			}
			break;
		}

		/* Mana -- destroys everything */
		case GF_MANA:
		{
			do_kill = TRUE;
			note_kill = (plural ? " are destroyed!" : " is destroyed!");
		}

		/* Holy Orb -- destroys cursed non-artifacts */
		case GF_HOLY_ORB:
		{
			if (cursed_p(o_ptr))
			{
				do_kill = TRUE;
				note_kill = (plural ? " are destroyed!" : " is destroyed!");
			}
			break;
		}

		/* Unlock chests */
		case GF_KILL_TRAP:
		case GF_KILL_DOOR:
		{
			/* Chests are noticed only if trapped or locked */
			if (o_ptr->tval == TV_CHEST)
			{
				/* Disarm/Unlock traps */
				if (o_ptr->pval > 0)
				{
					/* Disarm or Unlock */
					o_ptr->pval = (0 - o_ptr->pval);

					/* Identify */
					object_known(o_ptr);

					/* Notice */
					if (!quiet && p_ptr->obj_vis[c_ptr->o_idx])
					{
						msg_print(p_ptr, "Click!");
						obvious = TRUE;
					}
				}
			}

			break;
		}
	}


	/* Attempt to destroy the object */
	if (do_kill)
	{
		/* Effect "observed" */
		if (!quiet && p_ptr->obj_vis[c_ptr->o_idx])
		{
			obvious = TRUE;
			object_desc(p_ptr, o_name, sizeof(o_name), o_ptr, FALSE, 0);
		}

		/* Artifacts, and other objects, get to resist */
		if (is_art || ignore)
		{
			/* Observe the resist */
			if (!quiet && p_ptr->obj_vis[c_ptr->o_idx])
			{
				msg_format(p_ptr, "The %s %s unaffected!",
				           o_name, (plural ? "are" : "is"));
			}
		}

		/* Kill it */
		else
		{
			/* Describe if needed */
			if (!quiet && p_ptr->obj_vis[c_ptr->o_idx] && note_kill)
			{
				msg_format(p_ptr, "The %s%s", o_name, note_kill);
				sound(p_ptr, MSG_DESTROY);
			}

			/* Delete the object */
			delete_object(Depth, y, x);

			if (!quiet)
			{
				/* Redraw */
				everyone_lite_spot(Depth, y, x);
			}
		}
	}

	/* Return "Anything seen?" */
	return (obvious);
}



/*
 * Helper function for "project()" below.
 *
 * Handle a beam/bolt/ball causing damage to a monster.
 *
 * This routine takes a "source monster" (by index) which is mostly used to
 * determine if the player is causing the damage, and a "radius" (see below),
 * which is used to decrease the power of explosions with distance, and a
 * location, via integers which are modified by certain types of attacks
 * (polymorph and teleport being the obvious ones), a default damage, which
 * is modified as needed based on various properties, and finally a "damage
 * type" (see below).
 *
 * Note that this routine can handle "no damage" attacks (like teleport) by
 * taking a "zero" damage, and can even take "parameters" to attacks (like
 * confuse) by accepting a "damage", using it to calculate the effect, and
 * then setting the damage to zero.  Note that the "damage" parameter is
 * divided by the radius, so monsters not at the "epicenter" will not take
 * as much damage (or whatever)...
 *
 * Note that "polymorph" is dangerous, since a failure in "place_monster()"'
 * may result in a dereference of an invalid pointer.  XXX XXX XXX
 *
 * Various messages are produced, and damage is applied.
 *
 * Just "casting" a substance (i.e. plasma) does not make you immune, you must
 * actually be "made" of that substance, or "breathe" big balls of it.
 *
 * We assume that "Plasma" monsters, and "Plasma" breathers, are immune
 * to plasma.
 *
 * We assume "Nether" is an evil, necromantic force, so it doesn't hurt undead,
 * and hurts evil less.  If can breath nether, then it resists it as well.
 *
 * Damage reductions use the following formulas:
 *   Note that "dam = dam * 6 / (randint1(6) + 6);"
 *     gives avg damage of .655, ranging from .858 to .500
 *   Note that "dam = dam * 5 / (randint1(6) + 6);"
 *     gives avg damage of .544, ranging from .714 to .417
 *   Note that "dam = dam * 4 / (randint1(6) + 6);"
 *     gives avg damage of .444, ranging from .556 to .333
 *   Note that "dam = dam * 3 / (randint1(6) + 6);"
 *     gives avg damage of .327, ranging from .427 to .250
 *   Note that "dam = dam * 2 / (randint1(6) + 6);"
 *     gives something simple.
 *
 * In this function, "result" messages are postponed until the end, where
 * the "note" string is appended to the monster name, if not NULL.  So,
 * to make a spell have "no effect" just set "note" to NULL.  You should
 * also set "notice" to FALSE, or the player will learn what the spell does.
 *
 * We attempt to return "TRUE" if the player saw anything "useful" happen.
 */
static bool project_m(int who, int r, int Depth, int y, int x, int dam, int typ)
{
	player_type *p_ptr = who < 0 ? Players[0 - who] : NULL;

	int i, div;

	cave_type *c_ptr = &cave[Depth][y][x];

	monster_type *m_ptr;

	monster_race *r_ptr;
	
	monster_lore *l_ptr;

	cptr name;

	/* Is the monster "seen"? */
	bool seen;

	/* Were the "effects" obvious (if seen)? */
	bool obvious = FALSE;

	bool quiet = ((p_ptr == NULL) ? TRUE : FALSE);

	/* Polymorph setting (true or false) */
	int do_poly = 0;

	/* Teleport setting (max distance) */
	int do_dist = 0;

	/* Confusion setting (amount to confuse) */
	int do_conf = 0;

	/* Stunning setting (amount to stun) */
	int do_stun = 0;

	/* Sleep amount (amount to sleep) */
	int do_sleep = 0;

	/* Fear amount (amount to fear) */
	int do_fear = 0;

	/* Hold the monster name */
	char m_name[80];

	/* Assume no note */
	cptr note = NULL;

	/* Assume a default death */
	cptr note_dies = " dies.";


	/* Nobody here */
	if (c_ptr->m_idx <= 0) return (FALSE);

	/* Acquire monster pointer */
 	m_ptr = &m_list[c_ptr->m_idx];

	/* Acquire race pointer */
	r_ptr = &r_info[m_ptr->r_idx];
	
	/* Acquire lore pointer */
	l_ptr = (quiet ? NULL : p_ptr->l_list + m_ptr->r_idx); 

	/* Acquire name */
	name = (r_name + r_ptr->name);

	/* Never affect projector */
	if ((who > 0) && (c_ptr->m_idx == who)) return (FALSE);


	/* Set the "seen" flag */
	if (!quiet)
		seen = p_ptr->mon_vis[c_ptr->m_idx];
	else seen = FALSE;

	/* Extract radius */
	div = r + 1;

	/* Decrease damage */
	dam = dam / div;


	/* Mega-Hack */
	project_m_n++;
	project_m_x = x;
	project_m_y = y;


	/* Get the monster name (BEFORE polymorphing) */
	monster_desc(p_ptr, m_name, c_ptr->m_idx, 0);



	/* Some monsters get "destroyed" */
	if ((r_ptr->flags3 & RF3_DEMON) ||
	    (r_ptr->flags3 & RF3_UNDEAD) ||
	    (r_ptr->flags2 & RF2_STUPID) ||
	    (strchr("Evg", r_ptr->d_char)))
	{
		/* Special note at death */
		note_dies = " is destroyed.";
	}


	/* Analyze the damage type */
	switch (typ)
	{
			/* Magic Missile -- pure damage */
		case GF_MISSILE:
		{
			if (seen) obvious = TRUE;
			break;
		}

			/* Acid */
		case GF_ACID:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_IM_ACID)
			{
				note = " resists a lot.";
				dam /= 9;
				if (seen && !quiet) l_ptr->flags3 |= RF3_IM_ACID;
			}
			break;
		}

			/* Electricity */
		case GF_ELEC:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_IM_ELEC)
			{
				note = " resists a lot.";
				dam /= 9;
				if (seen && !quiet) l_ptr->flags3 |= RF3_IM_ELEC;
			}
			break;
		}

			/* Fire damage */
		case GF_FIRE:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_IM_FIRE)
			{
				note = " resists a lot.";
				dam /= 9;
				if (seen && !quiet) l_ptr->flags3 |= RF3_IM_FIRE;
			}
			break;
		}

			/* Cold */
		case GF_COLD:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_IM_COLD)
			{
				note = " resists a lot.";
				dam /= 9;
				if (seen && !quiet) l_ptr->flags3 |= RF3_IM_COLD;
			}
			break;
		}

			/* Poison */
		case GF_POIS:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_IM_POIS)
			{
				note = " resists a lot.";
				dam /= 9;
				if (seen && !quiet) l_ptr->flags3 |= RF3_IM_POIS;
			}
			break;
		}

			/* Holy Orb -- hurts Evil */
		case GF_HOLY_ORB:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_EVIL)
			{
				dam *= 2;
				if (seen && !quiet) l_ptr->flags3 |= RF3_EVIL;
			}
			break;
		}

			/* Arrow -- XXX no defense */
		case GF_BOULDER:
		case GF_BOLT:
		case GF_ARROW:
		{
			if (seen) obvious = TRUE;
			break;
		}

			/* Plasma -- XXX perhaps check ELEC or FIRE */
		case GF_PLASMA:
		{
			if (seen) obvious = TRUE;
			if (prefix(name, "Plasma") ||
			    (r_ptr->flags4 & RF4_BR_PLAS))
			{
				note = " resists.";
				dam *= 3; dam /= (randint1(6)+6);
			}
			break;
		}

			/* Nether -- see above */
		case GF_NETHER:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_UNDEAD)
			{
				note = " is immune.";
				dam = 0;
				if (seen && !quiet) l_ptr->flags3 |= RF3_UNDEAD;
			}
			else if (r_ptr->flags4 & RF4_BR_NETH)
			{
				note = " resists.";
				dam *= 3; dam /= (randint1(6)+6);
			}
			else if (r_ptr->flags3 & RF3_EVIL)
			{
				dam /= 2;
				note = " resists somewhat.";
				if (seen && !quiet) l_ptr->flags3 |= RF3_EVIL;
			}
			break;
		}

			/* Water (acid) damage -- Water spirits/elementals are immune */
		case GF_WATER:
		{
			if (seen) obvious = TRUE;
			if ((r_ptr->d_char == 'E') && prefix(name, "W"))
			{
				note = " is immune.";
				dam = 0;
			}
			break;
		}

			/* Chaos -- Chaos breathers resist */
		case GF_CHAOS:
		{
			if (seen) obvious = TRUE;
			do_poly = TRUE;
			do_conf = (5 + randint1(11)) / div;
			if (r_ptr->flags4 & RF4_BR_CHAO)
			{
				note = " resists.";
				dam *= 3; dam /= (randint1(6)+6);
				do_poly = FALSE;
			}
			break;
		}

			/* Shards -- Shard breathers resist */
		case GF_SHARDS:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags4 & RF4_BR_SHAR)
			{
				note = " resists.";
				dam *= 3; dam /= (randint1(6)+6);
			}
			break;
		}

			/* Sound -- Sound breathers resist */
		case GF_SOUND:
		{
			if (seen) obvious = TRUE;
			do_stun = (10 + randint1(15)) / div;
			if (r_ptr->flags4 & RF4_BR_SOUN)
			{
				note = " resists.";
				dam *= 2; dam /= (randint1(6)+6);
			}
			break;
		}

			/* Confusion */
		case GF_CONFUSION:
		{
			if (seen) obvious = TRUE;
			do_conf = (10 + randint1(15)) / div;
			if (r_ptr->flags4 & RF4_BR_CONF)
			{
				note = " resists.";
				dam *= 2; dam /= (randint1(6)+6);
			}
			else if (r_ptr->flags3 & RF3_NO_CONF)
			{
				note = " resists somewhat.";
				dam /= 2;
			}
			break;
		}

			/* Disenchantment -- Breathers and Disenchanters resist */
		case GF_DISENCHANT:
		{
			if (seen) obvious = TRUE;
			if ((r_ptr->flags4 & RF4_BR_DISE) ||
			    prefix(name, "Disen"))
			{
				note = " resists.";
				dam *= 3; dam /= (randint1(6)+6);
			}
			break;
		}

			/* Nexus -- Breathers and Existers resist */
		case GF_NEXUS:
		{
			if (seen) obvious = TRUE;
			if ((r_ptr->flags3 & RF3_RES_NEXU) ||
				(r_ptr->flags4 & RF4_BR_NEXU))
			{
				note = " resists.";
				dam *= 3; dam /= (randint1(6)+6);
			}
			break;
		}

			/* Force */
		case GF_FORCE:
		{
			if (seen) obvious = TRUE;
			do_stun = randint1(15) / div;
			if (r_ptr->flags4 & RF4_BR_WALL)
			{
				note = " resists.";
				dam *= 3; dam /= (randint1(6)+6);
			}
			break;
		}

			/* Inertia -- breathers resist */
		case GF_INERT:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags4 & RF4_BR_INER)
			{
				note = " resists.";
				dam *= 3; dam /= (randint1(6)+6);
			}
			break;
		}

			/* Time -- breathers resist */
		case GF_TIME:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags4 & RF4_BR_TIME)
			{
				note = " resists.";
				dam *= 3; dam /= (randint1(6)+6);
			}
			break;
		}

			/* Gravity -- breathers resist */
		case GF_GRAVITY:
		{
			if (seen) obvious = TRUE;

			/* Higher level monsters can resist the teleportation better */
			if (randint1(RIFT_RESIST_TELEPORT) > r_ptr->level)
				do_dist = 10;

			if (r_ptr->flags4 & RF4_BR_GRAV)
			{
				note = " resists.";
				dam *= 3; dam /= (randint1(6)+6);
				do_dist = 0;
			}
			break;
		}

			/* Pure damage */
		case GF_MANA:
		{
			if (seen) obvious = TRUE;
			break;
		}

			/* Meteor -- powerful magic missile */
		case GF_METEOR:
		{
			if (seen) obvious = TRUE;
			break;
		}

			/* Ice -- Cold + Cuts + Stun */
		case GF_ICE:
		{
			if (seen) obvious = TRUE;
			do_stun = randint1(15) / div;
			if (r_ptr->flags3 & RF3_IM_COLD)
			{
				note = " resists a lot.";
				dam /= 9;
				if (seen && !quiet) l_ptr->flags3 |= RF3_IM_COLD;
			}
			break;
		}


			/* Drain Life */
		case GF_OLD_DRAIN:
		{
			if (seen) obvious = TRUE;
			if ((r_ptr->flags3 & RF3_UNDEAD) ||
			    (r_ptr->flags3 & RF3_DEMON) ||
			    (strchr("Egv", r_ptr->d_char)))
			{
				if (r_ptr->flags3 & RF3_UNDEAD)
				{
					if (seen && !quiet) l_ptr->flags3 |= RF3_UNDEAD;
				}
				if (r_ptr->flags3 & RF3_DEMON)
				{
					if (seen && !quiet) l_ptr->flags3 |= RF3_DEMON;
				}

				note = " is unaffected!";
				obvious = FALSE;
				dam = 0;
			}

			break;
		}

			/* Polymorph monster (Use "dam" as "power") */
		case GF_OLD_POLY:
		{
			if (seen) obvious = TRUE;

			/* Attempt to polymorph (see below) */
			do_poly = TRUE;

			/* Powerful monsters can resist */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				note = " is unaffected!";
				do_poly = FALSE;
				obvious = FALSE;
			}

			/* No "real" damage */
			dam = 0;

			break;
		}


			/* Clone monsters (Ignore "dam") */
		case GF_OLD_CLONE:
		{
			if (seen) obvious = TRUE;

			/* Heal fully */
			m_ptr->hp = m_ptr->maxhp;

			/* Speed up */
			if (m_ptr->mspeed < 150) m_ptr->mspeed += 10;

			/* Never in the town */
			//TODO: extend to special levels and safezones!
			if(!Depth) break;

			//TODO: tweak to use safezones!
			if(!p_ptr || p_ptr->lev < 10)
			{
				/* Attempt to clone. */
				if (multiply_monster(c_ptr->m_idx))
				{
					note = " spawns!";
				}
			}

			/* No "real" damage */
			dam = 0;

			break;
		}


			/* Heal Monster (use "dam" as amount of healing) */
		case GF_OLD_HEAL:
		{
			if (seen) obvious = TRUE;

			/* Wake up */
			m_ptr->csleep = 0;

			/* Heal */
			m_ptr->hp += dam;

			/* No overflow */
			if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

			/* Redraw (later) if needed */
			update_health(c_ptr->m_idx);

			/* Message */
			note = " looks healthier.";

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Speed Monster (Ignore "dam") */
		case GF_OLD_SPEED:
		{
			if (seen) obvious = TRUE;

			/* Speed up */
			if (m_ptr->mspeed < 150) m_ptr->mspeed += 10;
			note = " starts moving faster.";

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Slow Monster (Use "dam" as "power") */
		case GF_OLD_SLOW:
		{
			if (seen) obvious = TRUE;

			/* Powerful monsters can resist */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				note = " is unaffected!";
				obvious = FALSE;
			}

			/* Normal monsters slow down */
			else
			{
				if (m_ptr->mspeed > 60) m_ptr->mspeed -= 10;
				note = " starts moving slower.";
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Sleep (Use "dam" as "power") */
		case GF_OLD_SLEEP:
		{
			if (seen) obvious = TRUE;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags3 & RF3_NO_SLEEP) ||
			    (r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				/* Memorize a flag */
				if (r_ptr->flags3 & RF3_NO_SLEEP)
				{
					if (seen && !quiet) l_ptr->flags3 |= RF3_NO_SLEEP;
				}

				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}
			else
			{
				/* Go to sleep (much) later */
				note = " falls asleep!";
				do_sleep = 500;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Confusion (Use "dam" as "power") */
		case GF_OLD_CONF:
		{
			if (seen) obvious = TRUE;

			/* Get confused later */
			do_conf = damroll(3, (dam / 2)) + 1;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags3 & RF3_NO_CONF) ||
			    (r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				/* Memorize a flag */
				if (r_ptr->flags3 & RF3_NO_CONF)
				{
					if (seen && !quiet) l_ptr->flags3 |= RF3_NO_CONF;
				}

				/* Resist */
				do_conf = 0;

				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}



			/* Lite, but only hurts susceptible creatures */
		case GF_LITE_WEAK:
		{
			/* Hurt by light */
			if (r_ptr->flags3 & RF3_HURT_LITE)
			{
				/* Obvious effect */
				if (seen) obvious = TRUE;

				/* Memorize the effects */
				if (seen && !quiet) l_ptr->flags3 |= RF3_HURT_LITE;

				/* Special effect */
				note = " cringes from the light!";
				note_dies = " shrivels away in the light!";
			}

			/* Normally no damage */
			else
			{
				/* No damage */
				dam = 0;
			}

			break;
		}



			/* Lite -- opposite of Dark */
		case GF_LITE:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags4 & RF4_BR_LITE)
			{
				note = " resists.";
				dam *= 2; dam /= (randint1(6)+6);
			}
			else if (r_ptr->flags3 & RF3_HURT_LITE)
			{
				if (seen && !quiet) l_ptr->flags3 |= RF3_HURT_LITE;
				note = " cringes from the light!";
				note_dies = " shrivels away in the light!";
				dam *= 2;
			}
			break;
		}


			/* Dark -- opposite of Lite */
		case GF_DARK:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags4 & RF4_BR_DARK)
			{
				note = " resists.";
				dam *= 2; dam /= (randint1(6)+6);
			}
			break;
		}


			/* Stone to Mud */
		case GF_KILL_WALL:
		{
			/* Hurt by rock remover */
			if (r_ptr->flags3 & RF3_HURT_ROCK)
			{
				/* Notice effect */
				if (seen) obvious = TRUE;

				/* Memorize the effects */
				if (seen && !quiet) l_ptr->flags3 |= RF3_HURT_ROCK;

				/* Cute little message */
				note = " loses some skin!";
				note_dies = " dissolves!";
			}

			/* Usually, ignore the effects */
			else
			{
				/* No damage */
				dam = 0;
			}

			break;
		}


			/* Teleport undead (Use "dam" as "power") */
		case GF_AWAY_UNDEAD:
		{
			/* Only affect undead */
			if (r_ptr->flags3 & RF3_UNDEAD)
			{
				if (seen) obvious = TRUE;
				if (seen && !quiet) l_ptr->flags3 |= RF3_UNDEAD;
				do_dist = dam;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Teleport evil (Use "dam" as "power") */
		case GF_AWAY_EVIL:
		{
			/* Only affect evil */
			if (r_ptr->flags3 & RF3_EVIL)
			{
				if (seen) obvious = TRUE;
				if (seen && !quiet) l_ptr->flags3 |= RF3_EVIL;
				do_dist = dam;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Teleport monster (Use "dam" as "power") */
		case GF_AWAY_ALL:
		{
			/* Obvious */
			if (seen) obvious = TRUE;

			/* Prepare to teleport */
			do_dist = dam;

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Turn undead (Use "dam" as "power") */
		case GF_TURN_UNDEAD:
		{
			/* Only affect undead */
			if (r_ptr->flags3 & RF3_UNDEAD)
			{
				/* Learn about type */
				if (seen && !quiet) l_ptr->flags3 |= RF3_UNDEAD;

				/* Obvious */
				if (seen) obvious = TRUE;

				/* Apply some fear */
				do_fear = damroll(3, (dam / 2)) + 1;

				/* Attempt a saving throw */
				if (r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10)
				{
					/* No obvious effect */
					note = " is unaffected!";
					obvious = FALSE;
					do_fear = 0;
				}
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Turn evil (Use "dam" as "power") */
		case GF_TURN_EVIL:
		{
			/* Only affect evil */
			if (r_ptr->flags3 & RF3_EVIL)
			{
				/* Learn about type */
				if (seen && !quiet) l_ptr->flags3 |= RF3_EVIL;

				/* Obvious */
				if (seen) obvious = TRUE;

				/* Apply some fear */
				do_fear = damroll(3, (dam / 2)) + 1;

				/* Attempt a saving throw */
				if (r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10)
				{
					/* No obvious effect */
					note = " is unaffected!";
					obvious = FALSE;
					do_fear = 0;
				}
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Turn monster (Use "dam" as "power") */
		case GF_TURN_ALL:
		{
			/* Obvious */
			if (seen) obvious = TRUE;

			/* Apply some fear */
			do_fear = damroll(3, (dam / 2)) + 1;

			/* Attempt a saving throw */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
			    (r_ptr->flags3 & RF3_NO_FEAR) ||
			    (r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				/* No obvious effect */
				note = " is unaffected!";
				obvious = FALSE;
				do_fear = 0;
			}

			/* No "real" damage */
			dam = 0;
			break;
		}


			/* Dispel undead */
		case GF_DISP_UNDEAD:
		{
			/* Only affect undead */

			if (r_ptr->flags3 & RF3_UNDEAD)
			{
				/* Learn about type */
				if (seen && !quiet) l_ptr->flags3 |= RF3_UNDEAD;

				/* Obvious */
				if (seen) obvious = TRUE;

				/* Message */
				note = " shudders."; 
				note_dies = " dissolves!";
			}

			/* Ignore other monsters */
			else
			{
				/* No damage */
				dam = 0;
			}

			break;
		}


			/* Dispel evil */
		case GF_DISP_EVIL:
		{
			/* Only affect evil */
			if (r_ptr->flags3 & RF3_EVIL)
			{
				/* Learn about type */
				if (seen && !quiet) l_ptr->flags3 |= RF3_EVIL;

				/* Obvious */
				if (seen) obvious = TRUE;

				/* Message */
				note = " shudders.";
				note_dies = " dissolves!";
			}

			/* Ignore other monsters */
			else
			{
				/* No damage */
				dam = 0;
			}

			break;
		}


			/* Dispel monster */
		case GF_DISP_ALL:
		{
			/* Obvious */
			if (seen) obvious = TRUE;

			/* Message */
			note = " shudders.";
			note_dies = " dissolves!";

			break;
		}

			/* Default */
		default:
		{
			/* No damage */
			dam = 0;

			break;
		}
	}



	/* "Unique" monsters cannot be polymorphed */
	if (r_ptr->flags1 & RF1_UNIQUE) do_poly = FALSE;


	/* "Unique" monsters can only be "killed" by the player */
	if (r_ptr->flags1 & RF1_UNIQUE)
	{
		/* Uniques may only be killed by the player */
		if ((who > 0) && (dam > m_ptr->hp)) dam = m_ptr->hp;
	}


	/* Check for death */
	if (dam > m_ptr->hp)
	{
		/* Extract method of death */
		note = note_dies;
	}

	/* Mega-Hack -- Handle "polymorph" -- monsters get a saving throw */
	else if (do_poly && (randint1(90) > r_ptr->level))
	{
		/* Default -- assume no polymorph */
		note = " is unaffected!";

		/* Pick a "new" monster race */
		i = poly_r_idx(m_ptr->r_idx);

		/* Handle polymorh */
		if (i != m_ptr->r_idx)
		{
			/* Obvious */
			if (seen) obvious = TRUE;

			/* Monster polymorphs */
			note = " changes!";

			/* Turn off the damage */
			dam = 0;

			/* "Kill" the "old" monster */
			delete_monster_idx(c_ptr->m_idx);

			/* Create a new monster (no groups) */
			(void)place_monster_aux(Depth, y, x, i, FALSE, FALSE);

			/* XXX XXX XXX Hack -- Assume success */

			/* Hack -- Get new monster */
			m_ptr = &m_list[c_ptr->m_idx];

			/* Hack -- Get new race */
			r_ptr = &r_info[m_ptr->r_idx];
		}
	}

	/* Handle "teleport" */
	else if (do_dist)
	{
		/* Obvious */
		if (seen) obvious = TRUE;

		/* Message */
		note = " disappears!";

		/* Teleport */
		teleport_away(c_ptr->m_idx, do_dist);

		/* Hack -- get new location */
		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Hack -- get new grid */
		c_ptr = &cave[Depth][y][x];
	}

	/* Sound and Impact breathers never stun */
	else if (do_stun &&
	         !(r_ptr->flags4 & RF4_BR_SOUN) &&
	         !(r_ptr->flags4 & RF4_BR_WALL))
	{
		/* Obvious */
		if (seen) obvious = TRUE;

		/* Get confused */
		if (m_ptr->stunned)
		{
			note = " is more dazed.";
			i = m_ptr->stunned + (do_stun / 2);
		}
		else
		{
			note = " is dazed.";
			i = do_stun;
		}

		/* Apply stun */
		m_ptr->stunned = (i < 200) ? i : 200;
	}

	/* Confusion and Chaos breathers (and sleepers) never confuse */
	else if (do_conf &&
	         !(r_ptr->flags3 & RF3_NO_CONF) &&
	         !(r_ptr->flags4 & RF4_BR_CONF) &&
	         !(r_ptr->flags4 & RF4_BR_CHAO))
	{
		/* Obvious */
		if (seen) obvious = TRUE;

		/* Already partially confused */
		if (m_ptr->confused)
		{
			note = " looks more confused.";
			i = m_ptr->confused + (do_conf / 2);
		}

		/* Was not confused */
		else
		{
			note = " looks confused.";
			i = do_conf;
		}

		/* Apply confusion */
		m_ptr->confused = (i < 200) ? i : 200;
	}


	/* Fear */
	if (do_fear)
	{
		/* Increase fear */
		i = m_ptr->monfear + do_fear;

		/* Set fear */
		m_ptr->monfear = (i < 200) ? i : 200;
	}


	/* If another monster did the damage, hurt the monster by hand */
	if (who > 0)
	{
		/* Redraw (later) if needed */
		update_health(c_ptr->m_idx);

		/* Wake the monster up */
		m_ptr->csleep = 0;

		/* Hurt the monster */
		m_ptr->hp -= dam;

		/* Dead monster */
		if (m_ptr->hp < 0)
		{
			/* Generate treasure, etc */
			if (!quiet) monster_death(p_ptr, c_ptr->m_idx);

			/* Delete the monster */
			delete_monster_idx(c_ptr->m_idx);

			/* Give detailed messages if destroyed */
			if (!quiet && note) msg_format(p_ptr, "%^s%s", m_name, note);
		}

		/* Damaged monster */
		else
		{
			/* Give detailed messages if visible or destroyed */
			if (!quiet && note && seen) msg_format(p_ptr, "%^s%s", m_name, note);

			/* Hack -- Pain message */
			else if (!quiet && dam > 0) message_pain(p_ptr, c_ptr->m_idx, dam);

			/* Hack -- handle sleep */
			if (do_sleep) m_ptr->csleep = do_sleep;
		}
	}

	/* If the player did it, give him experience, check fear */
	else
	{
		bool fear = FALSE;

		/* Hurt the monster, check for fear and death */
		if (!quiet && mon_take_hit(p_ptr, c_ptr->m_idx, dam, &fear, note_dies))
		{
			/* Dead monster */
		}

		/* Damaged monster */
		else
		{
			/* Give detailed messages if visible or destroyed */
			if (!quiet && note && seen) msg_format(p_ptr, "%^s%s", m_name, note);

			/* Hack -- Pain message */
			else if (!quiet && dam > 0) message_pain(p_ptr, c_ptr->m_idx, dam);

			/* Take note */
			if (!quiet && (fear || do_fear) && (p_ptr->mon_vis[c_ptr->m_idx]) && !(r_ptr->flags2 & RF2_WANDERER))
			{
				/* Sound */
				sound(p_ptr, MSG_FLEE);

				/* Message */
				msg_format(p_ptr, "%^s flees in terror!", m_name);
			}

			/* Hack -- handle sleep */
			if (do_sleep) m_ptr->csleep = do_sleep;
		}
	}


	/* Update the monster XXX XXX XXX */
	update_mon(c_ptr->m_idx, FALSE);

	/* Update monster recall window */	
	if (!quiet && p_ptr->monster_race_idx == m_ptr->r_idx)
	{
		p_ptr->window |= (PW_MONSTER);
	}

	if (!quiet)
	{
		/* Hack -- Redraw the monster grid anyway */
		everyone_lite_spot(Depth, y, x);
	}


	/* Return "Anything seen?" */
	return (obvious);
}






/*
 * Helper function for "project()" below.
 *
 * Handle a beam/bolt/ball causing damage to the player "p_ptr".
 *
 * This routine takes a "source monster" (by index), a "distance", a default
 * "damage", and a "damage type".  See "project_m()" above.
 *
 * If "rad" is non-zero, then the blast was centered elsewhere, and the damage
 * is reduced (see "project_m()" above).  This can happen if a monster breathes
 * at the player and hits a wall instead.
 *
 * We return "TRUE" if any "obvious" effects were observed.  XXX XXX Actually,
 * we just assume that the effects were obvious, for historical reasons.
 */
static bool project_p(player_type *p_ptr, int who, int r, int Depth, int y, int x, int dam, int typ)
{
	int k = 0;

	int div;

	/* Hack -- adjust damage */
	bool adjust = TRUE;

	/* Hack -- assume obvious */
	bool obvious = TRUE;

	/* Player blind-ness */
	bool blind;

	/* Player needs a "description" (he is blind) */
	bool fuzzy = FALSE;

	/* The effects were positive, do not disturb */
	bool positive_effect = FALSE;

	/* Source monster */
	monster_type *m_ptr;

	/* Monster name (for attacks) */
	char m_name[80];

	/* Monster name (for damage) */
	char killer[80];

	/* Hack -- messages */
	cptr act = NULL;

	/* Bad player */
	if (p_ptr == NULL) return (FALSE);

	blind = (p_ptr->blind ? TRUE : FALSE);

	/* Player is not here */
	if ((x != p_ptr->px) || (y != p_ptr->py) || (Depth != p_ptr->dun_depth)) return (FALSE);

	/* Player cannot hurt himself */
	if (who < 0 && same_player(Players[0 - who], p_ptr)) return (FALSE);

	/* The damage has already been applied! */
	if (p_ptr->project_hurt) return FALSE;

	/* Extract radius */
	div = r + 1;

	/* If the player is blind, be more descriptive */
	if (blind) fuzzy = TRUE;

	/* Mega-Hack */
	project_m_n++;
	project_m_x = x;
	project_m_y = y;

	if (who > 0)
	{
		/* Get the source monster */
		m_ptr = &m_list[who];

		/* Get the monster name */
		monster_desc(p_ptr, m_name, who, 0);

		/* Get the monster's real name */
		monster_desc(p_ptr, killer, who, 0x88);
	}
	else if (who < 0)
	{
		/* Get the source player */
		player_type *q_ptr = Players[0 - who];

		/* Get the source player name */
		my_strcpy(killer, q_ptr->name, 80);

		/* Do not become hostile if it was a healing or teleport spell */
		if ((typ < GF_HEAL_PLAYER) && (typ != GF_AWAY_ALL))
		{
			if (!pvp_okay(q_ptr, p_ptr, (p_ptr->target_who == who ? 2 : 3)))
			{
				return(FALSE);
			}
			
			/* XXX Reduce damage by 1/3 */
			dam = (dam + 2) / 3;			
		}
		/* Hack -- share monster hurt during healing */
		else if (typ >= GF_HEAL_PLAYER)
		{
			/* Copy hurt flags from healed player "p_ptr" to healer "q_ptr" */
			party_share_hurt(q_ptr, p_ptr);
		}
		/* Hack -- do not modify "dam" variable if it contains spell index */
		if (typ >= GF_PROJECT_SPELL)
		{
			adjust = FALSE;
		}
	}

	/* Adjust damage */
	if (adjust)
	{
		/* Hack -- Never do excessive damage */
		if (dam > 1600) dam = 1600;

		/* Decrease damage */
		dam = (dam + r) / div;

		/* Hack -- always do at least one point of damage */
		if (dam <= 0) dam = 1;
	}

	/* Analyze the damage */
	switch (typ)
	{
		/* Standard damage -- hurts inventory too */
		case GF_ACID:
		if (fuzzy) msg_print(p_ptr, "You are hit by acid!");
		acid_dam(p_ptr, dam, killer);
		break;

		/* Standard damage -- hurts inventory too */
		case GF_FIRE:
		if (fuzzy) msg_print(p_ptr, "You are hit by fire!");
		fire_dam(p_ptr, dam, killer);
		break;

		/* Standard damage -- hurts inventory too */
		case GF_COLD:
		if (fuzzy) msg_print(p_ptr, "You are hit by cold!");
		cold_dam(p_ptr, dam, killer);
		break;

		/* Standard damage -- hurts inventory too */
		case GF_ELEC:
		if (fuzzy) msg_print(p_ptr, "You are hit by lightning!");
		elec_dam(p_ptr, dam, killer);
		break;

		/* Standard damage -- also poisons player */
		case GF_POIS:
		if (fuzzy) msg_print(p_ptr, "You are hit by poison!");
		if (p_ptr->resist_pois) dam = (dam + 2) / 3;
		if (p_ptr->oppose_pois) dam = (dam + 2) / 3;
		take_hit(p_ptr, dam, killer);
		if (!(p_ptr->resist_pois || p_ptr->oppose_pois))
		{
			(void)set_poisoned(p_ptr, p_ptr->poisoned + randint0(dam) + 10);
		}
		break;

		/* Standard damage */
		case GF_MISSILE:
		if (fuzzy) msg_print(p_ptr, "You are hit by something!");
		take_hit(p_ptr, dam, killer);
		break;

		/* Holy Orb -- Player only takes partial damage */
		case GF_HOLY_ORB:
		if (fuzzy) msg_print(p_ptr, "You are hit by something!");
		dam /= 2;
		take_hit(p_ptr, dam, killer);
		break;

		/* Arrow -- XXX no dodging */
		case GF_ARROW:
		if (fuzzy) msg_print(p_ptr, "You are hit by something sharp!");
		take_hit(p_ptr, dam, killer);
		break;

		/* Plasma -- XXX No resist */
		case GF_PLASMA:
		if (fuzzy) msg_print(p_ptr, "You are hit by something!");
		take_hit(p_ptr, dam, killer);
		if (!p_ptr->resist_sound)
		{
			int k = (randint1((dam > 40) ? 35 : (dam * 3 / 4 + 5)));
			(void)set_stun(p_ptr, p_ptr->stun + k);
		}
		break;

		/* Nether -- drain experience */
		case GF_NETHER:
		if (fuzzy) msg_print(p_ptr, "You are hit by something strange!");
		if (p_ptr->resist_neth)
		{
			dam *= 6; dam /= (randint1(6) + 6);
		}
		else
		{
			if (p_ptr->hold_life && (randint0(100) < 75))
			{
				msg_print(p_ptr, "You keep hold of your life force!");
			}
			else if (p_ptr->hold_life)
			{
				msg_print(p_ptr, "You feel your life slipping away!");
				lose_exp(p_ptr, 200 + (p_ptr->exp/1000) * MON_DRAIN_LIFE);
			}
			else
			{
				msg_print(p_ptr, "You feel your life draining away!");
				lose_exp(p_ptr, 200 + (p_ptr->exp/100) * MON_DRAIN_LIFE);
			}
		}
		take_hit(p_ptr, dam, killer);
		break;

		/* Water -- stun/confuse */
		case GF_WATER:
		if (fuzzy) msg_print(p_ptr, "You are hit by something!");
		if (!p_ptr->resist_sound)
		{
			(void)set_stun(p_ptr, p_ptr->stun + randint1(40));
		}
		if (!p_ptr->resist_conf)
		{
			(void)set_confused(p_ptr, p_ptr->confused + randint1(5) + 5);
		}
		take_hit(p_ptr, dam, killer);
		break;

		/* Chaos -- many effects */
		case GF_CHAOS:
		if (fuzzy) msg_print(p_ptr, "You are hit by something strange!");
		if (p_ptr->resist_chaos)
		{
			dam *= 6; dam /= (randint1(6) + 6);
		}
		if (!p_ptr->resist_conf && !p_ptr->resist_chaos)
		{
			(void)set_confused(p_ptr, p_ptr->confused + randint0(20) + 10);
		}
		if (!p_ptr->resist_chaos)
		{
			(void)set_image(p_ptr, p_ptr->image + randint1(10));
		}
		if (!p_ptr->resist_neth && !p_ptr->resist_chaos)
		{
			if (p_ptr->hold_life && (randint0(100) < 75))
			{
				msg_print(p_ptr, "You keep hold of your life force!");
			}
			else if (p_ptr->hold_life)
			{
				msg_print(p_ptr, "You feel your life slipping away!");
				lose_exp(p_ptr, 500 + (p_ptr->exp/1000) * MON_DRAIN_LIFE);
			}
			else
			{
				msg_print(p_ptr, "You feel your life draining away!");
				lose_exp(p_ptr, 5000 + (p_ptr->exp/100) * MON_DRAIN_LIFE);
			}
		}
		take_hit(p_ptr, dam, killer);
		break;

		/* Shards -- mostly cutting */
		case GF_SHARDS:
		if (fuzzy) msg_print(p_ptr, "You are hit by something sharp!");
		if (p_ptr->resist_shard)
		{
			dam *= 6; dam /= (randint1(6) + 6);
		}
		else
		{
			(void)set_cut(p_ptr, p_ptr->cut + dam);
		}
		take_hit(p_ptr, dam, killer);
		break;

		/* Sound -- mostly stunning */
		case GF_SOUND:
		if (fuzzy) msg_print(p_ptr, "You are hit by something!");
		if (p_ptr->resist_sound)
		{
			dam *= 5; dam /= (randint1(6) + 6);
		}
		else
		{
			int k = (randint1((dam > 90) ? 35 : (dam / 3 + 5)));
			(void)set_stun(p_ptr, p_ptr->stun + k);
		}
		take_hit(p_ptr, dam, killer);
		break;

		/* Pure confusion */
		case GF_CONFUSION:
		if (fuzzy) msg_print(p_ptr, "You are hit by something!");
		if (p_ptr->resist_conf)
		{
			dam *= 5; dam /= (randint1(6) + 6);
		}
		if (!p_ptr->resist_conf)
		{
			(void)set_confused(p_ptr, p_ptr->confused + randint1(20) + 10);
		}
		take_hit(p_ptr, dam, killer);
		break;

		/* Disenchantment -- see above */
		case GF_DISENCHANT:
		if (fuzzy) msg_print(p_ptr, "You are hit by something strange!");
		if (p_ptr->resist_disen)
		{
			dam *= 6; dam /= (randint1(6) + 6);
		}
		else
		{
			(void)apply_disenchant(p_ptr, 0);
		}
		take_hit(p_ptr, dam, killer);
		break;

		/* Nexus -- see above */
		case GF_NEXUS:
		if (fuzzy) msg_print(p_ptr, "You are hit by something strange!");
		if (p_ptr->resist_nexus)
		{
			dam *= 6; dam /= (randint1(6) + 6);
		}
		else
		{
			apply_nexus(p_ptr, m_ptr);
		}
		take_hit(p_ptr, dam, killer);
		break;

		/* Force -- mostly stun */
		case GF_FORCE:
		if (fuzzy) msg_print(p_ptr, "You are hit by something!");
		if (!p_ptr->resist_sound)
		{
			(void)set_stun(p_ptr, p_ptr->stun + randint1(20));
		}
		take_hit(p_ptr, dam, killer);
		break;

		/* Inertia -- slowness */
		case GF_INERT:
		if (fuzzy) msg_print(p_ptr, "You are hit by something strange!");
		(void)set_slow(p_ptr, p_ptr->slow + randint0(4) + 4);
		take_hit(p_ptr, dam, killer);
		break;

		/* Lite -- blinding */
		case GF_LITE:
		if (fuzzy) msg_print(p_ptr, "You are hit by something!");
		if (p_ptr->resist_lite)
		{
			dam *= 4; dam /= (randint1(6) + 6);
		}
		else if (!blind && !p_ptr->resist_blind)
		{
			(void)set_blind(p_ptr, p_ptr->blind + randint1(5) + 2);
		}
		take_hit(p_ptr, dam, killer);
		break;

		/* Dark -- blinding */
		case GF_DARK:
		if (fuzzy) msg_print(p_ptr, "You are hit by something!");
		if (p_ptr->resist_dark)
		{
			dam *= 4; dam /= (randint1(6) + 6);
		}
		else if (!blind && !p_ptr->resist_blind)
		{
			(void)set_blind(p_ptr, p_ptr->blind + randint1(5) + 2);
		}
		take_hit(p_ptr, dam, killer);
		break;

		/* Time -- bolt fewer effects XXX */
		case GF_TIME:
		if (fuzzy) msg_print(p_ptr, "You are hit by something strange!");
		switch (randint1(10))
		{
			case 1: case 2: case 3: case 4: case 5:
			msg_print(p_ptr, "You feel life has clocked back.");
			lose_exp(p_ptr, 100 + (p_ptr->exp / 100) * MON_DRAIN_LIFE);
			break;

			case 6: case 7: case 8: case 9:

			switch (randint1(6))
			{
				case 1: k = A_STR; act = "strong"; break;
				case 2: k = A_INT; act = "bright"; break;
				case 3: k = A_WIS; act = "wise"; break;
				case 4: k = A_DEX; act = "agile"; break;
				case 5: k = A_CON; act = "hale"; break;
				case 6: k = A_CHR; act = "beautiful"; break;
			}

			msg_format(p_ptr, "You're not as %s as you used to be...", act);

			p_ptr->stat_cur[k] = (p_ptr->stat_cur[k] * 3) / 4;
			if (p_ptr->stat_cur[k] < 3) p_ptr->stat_cur[k] = 3;
			p_ptr->update |= (PU_BONUS);
			break;

			case 10:

			msg_print(p_ptr, "You're not as powerful as you used to be...");

			for (k = 0; k < A_MAX; k++)
			{
				p_ptr->stat_cur[k] = (p_ptr->stat_cur[k] * 3) / 4;
				if (p_ptr->stat_cur[k] < 3) p_ptr->stat_cur[k] = 3;
			}
			p_ptr->update |= (PU_BONUS);
			break;
		}
		take_hit(p_ptr, dam, killer);
		break;

		/* Gravity -- stun plus slowness plus teleport */
		case GF_GRAVITY:
		if (fuzzy) msg_print(p_ptr, "You are hit by something strange!");
		msg_print(p_ptr, "Gravity warps around you.");
		/* Higher level players can resist the teleportation better */
		if (randint1(127) > p_ptr->lev)
			teleport_player(p_ptr, 5);		

		(void)set_slow(p_ptr, p_ptr->slow + randint0(4) + 4);
		if (!p_ptr->resist_sound)
		{
			int k = (randint1((dam > 90) ? 35 : (dam / 3 + 5)));
			(void)set_stun(p_ptr, p_ptr->stun + k);
		}
		take_hit(p_ptr, dam, killer);
		break;

		/* Pure damage */
		case GF_MANA:
		if (fuzzy) msg_print(p_ptr, "You are hit by something!");
		take_hit(p_ptr, dam, killer);
		break;

		/* Pure damage */
		case GF_METEOR:
		if (fuzzy) msg_print(p_ptr, "You are hit by something!");
		take_hit(p_ptr, dam, killer);
		break;

		/* Ice -- cold plus stun plus cuts */
		case GF_ICE:
		if (fuzzy) msg_print(p_ptr, "You are hit by something sharp!");
		cold_dam(p_ptr, dam, killer);
		if (!p_ptr->resist_shard)
		{
			(void)set_cut(p_ptr, p_ptr->cut + damroll(5, 8));
		}
		if (!p_ptr->resist_sound)
		{
			(void)set_stun(p_ptr, p_ptr->stun + randint1(15));
		}
		break;

		/* Teleport other -- Teleports */
		case GF_AWAY_ALL:
		if (fuzzy) msg_print(p_ptr, "You feel you are somewhere else.");
		else msg_format(p_ptr, "%^s teleports you away.", killer);
		teleport_player(p_ptr, 200);
		break;

		/* MAngband-specific: projected heal */
		case GF_HEAL_PLAYER:
		
		if (fuzzy) msg_print(p_ptr, "You are hit by something good!");		
		else msg_format(p_ptr, "%^s heals you!", killer);
		
		hp_player(p_ptr, dam);
		(void)set_cut(p_ptr, p_ptr->cut - 10);
		
		positive_effect = TRUE;
		break;

		/* MAngband-specific: projected spell */
		case GF_PROJECT_SPELL:
 
		if (fuzzy) msg_print(p_ptr, "You are hit by something good!");		
		else msg_format(p_ptr, "%^s does something to you!", killer);

		cast_spell_hack(p_ptr, TV_MAGIC_BOOK, dam);

		positive_effect = TRUE;
		break;

		/* MAngband-specific: projected prayer */
		case GF_PROJECT_PRAYER:

		if (fuzzy) msg_print(p_ptr, "You are hit by something good!");		
		else msg_format(p_ptr, "%^s prays on your behalf!", killer);

		cast_spell_hack(p_ptr, TV_PRAYER_BOOK, dam);

		positive_effect = TRUE;
		break;

		case GF_OLD_CONF:
		
		if (fuzzy) msg_print(p_ptr, "You hear puzzling noises!");
		else msg_format(p_ptr, "%^s creates a mesmerising illusion!", killer);
		
		if (p_ptr->resist_conf)
		{
			msg_print(p_ptr, "You disbelieve the feeble spell.");
		}
		else if (randint0(100 + dam*6) < p_ptr->skill_sav)
		{
			msg_print(p_ptr, "You disbelieve the feeble spell.");
		}
		else set_confused(p_ptr, p_ptr->confused + dam);
		
		break;
		
		case GF_OLD_SLOW:
		
		if (fuzzy) msg_print(p_ptr, "Something drains power from your muscles!");
		else msg_format(p_ptr, "%^s drains power from your muscles!", killer);
		
		if (p_ptr->free_act)
		{
			msg_print(p_ptr, "You are unaffected!");
		}
		else if (randint0(100 + dam*6) < p_ptr->skill_sav)
		{
			msg_print(p_ptr, "You resist the effects!");
		}
		else set_slow(p_ptr, p_ptr->slow + dam);
		
		break;
		
		case GF_OLD_SLEEP:

		if (fuzzy) msg_print(p_ptr, "Something makes you very sleepy!");
		else msg_format(p_ptr, "%^s makes you very sleepy!", killer);

		if (p_ptr->free_act)
		{
			msg_print(p_ptr, "You are unaffected!");
		}
		else if (randint0(100) < p_ptr->skill_sav)
		{
			msg_print(p_ptr, "You resist the effects!");
		}
		else set_paralyzed(p_ptr, p_ptr->paralyzed + 1 + randint1(6));

		break;

		case GF_TURN_ALL:
		
		if (fuzzy) msg_print(p_ptr, "Something mumbles, and you hear scary noises!");
		else msg_format(p_ptr, "%^s casts a fearful illusion!", killer);
		
		if (p_ptr->resist_fear)
		{
			msg_print(p_ptr, "You refuse to be frightened.");
		}
		else if (randint0(100 + dam*6) < p_ptr->skill_sav)
		{
			msg_print(p_ptr, "You refuse to be frightened.");
		}
		else
		{
			(void)set_afraid(p_ptr, p_ptr->afraid + dam);
		}
		
		break;
		
		case GF_OLD_POLY:
		
		if (fuzzy) msg_print(p_ptr, "You feel bizzare!");
		else msg_format(p_ptr, "%^s polymorphs you!", killer);
		if (p_ptr->resist_nexus)
		{
			msg_print(p_ptr, "You resist the effects!");
		}
		else
		{
			apply_morph(p_ptr, dam, killer);
		}
		break;
		
		/* Default */
		default:

		/* No damage */
		dam = 0;

		break;
	}

	/* The damage has been applied */
	p_ptr->project_hurt = TRUE;

	/* Disturb */
	if (!positive_effect) disturb(p_ptr, 1, 0);


	/* Return "Anything seen?" */
	return (obvious);
}








/*
 * Find the attr/char pair to use for a spell effect
 *
 * It is moving (or has moved) from (x,y) to (nx,ny).
 *
 * If the distance is not "one", we (may) return "*".
 */
static u16b bolt_index(int y, int x, int ny, int nx, int typ)
{
	int dir;

	/* No motion (*) */
	if ((ny == y) && (nx == x)) dir = 5;
	/* Vertical (|) (down->up) */
	else if (nx == x && (ny-y == -1)) dir = 8;
	/* Vertical (|) (up->down) */
	else if (nx == x && (ny-y == 1)) dir = 2;
	/* Horizontal (-) (right->left) */
	else if (ny == y && (nx-x == -1)) dir = 6;
	/* Horizontal (-) (left->right) */
	else if (ny == y && (nx-x == 1)) dir = 4;
	/* Diagonal (/) (down->up) */
	else if ((ny-y) == (x-nx) && (ny-y == -1)) dir = 9;
	/* Diagonal (/) (up->down) */
	else if ((ny-y) == (x-nx) && (ny-y == 1)) dir = 1;
	/* Diagonal (\) (down->up) */
	else if ((ny-y) == (nx-x) && (ny-y == -1)) dir = 3;
	/* Diagonal (\) (up->down) */
	else if ((ny-y) == (nx-x) && (ny-y == 1)) dir = 7;
	/* Weird (*) */
	else dir = 5;

	/* spell_type * 9 + offset - 1 */
	return typ * 9 + dir - 1;
}

static u16b bolt_pict(player_type *p_ptr, int y, int x, int ny, int nx, int typ)
{
	byte a;
	char c;
	int index = bolt_index(y, x, ny, nx, typ);

	/* Obtain attr/char */
	a = p_ptr->misc_attr[index];
	c = p_ptr->misc_char[index];

	/* Resolve multi-hued */
	if (a == 0x7F) a = mh_attr();

	/* Create pict */
	return (PICT(a,c));
}

/* Get "default" bolt attr/char. Only used on init. Replaced by pref files later on. */
u16b default_bolt_pict(int typ, int dir, int *index)
{
	static char dirs[9][2] = {
		{ -1, -1 }, { 0, -1 }, { 1, -1 },
		{ -1,  0 }, { 0,  0 }, { 1,  0 },
		{ -1,  1 }, { 0,  1 }, { 1,  1 },
	};
	static char chars[9] = {
		'\\', '|', '/',
		'-',  '*', '-',
		'/',  '|', '\\',
	};
	byte a;
	char c;

	*index = bolt_index(0, 0, dirs[8 - dir][1], dirs[8 - dir][0], typ);
	a = spell_color(typ, FALSE);
	c = chars[8 - dir];

	/* Create pict */
	return (PICT(a,c));
}

#if 0
/*	XXX -- Supressed by bolt_pict
 * Find the char to use to draw a moving bolt
 * It is moving (or has moved) from (x,y) to (nx,ny).
 * If the distance is not "one", we (may) return "*".
 */
static char bolt_char(int y, int x, int ny, int nx)
{
	if ((ny == y) && (nx == x)) return '*';
	if (ny == y) return '-';
	if (nx == x) return '|';
	if ((ny-y) == (x-nx)) return '/';
	if ((ny-y) == (nx-x)) return '\\';
	return '*';
}
#endif


/*
 * Generic "beam"/"bolt"/"ball" projection routine.  -BEN-
 *
 * Input:
 *   who: Index of "source" monster (or "zero" if "player")
 *   rad: Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 *   y,x: Target location (or location to travel "towards")
 *   dam: Base damage roll to apply to affected monsters (or player)
 *   typ: Type of damage to apply to monsters (and objects)
 *   flg: Extra bit flags (see PROJECT_xxxx in "defines.h")
  *
 * Return:
 *   TRUE if any "effects" of the projection were observed, else FALSE
 *
 * Allows a monster (or player) to project a beam/bolt/ball of a given kind
 * towards a given location (optionally passing over the heads of interposing
 * monsters), and have it do a given amount of damage to the monsters (and
 * optionally objects) within the given radius of the final location.
 *
 * A "bolt" travels from source to target and affects only the target grid.
 * A "beam" travels from source to target, affecting all grids passed through.
 * A "ball" travels from source to the target, exploding at the target, and
 *   affecting everything within the given radius of the target location.
 *
 * Traditionally, a "bolt" does not affect anything on the ground, and does
 * not pass over the heads of interposing monsters, much like a traditional
 * missile, and will "stop" abruptly at the "target" even if no monster is
 * positioned there, while a "ball", on the other hand, passes over the heads
 * of monsters between the source and target, and affects everything except
 * the source monster which lies within the final radius, while a "beam"
 * affects every monster between the source and target, except for the casting
 * monster (or player), and rarely affects things on the ground.
 *
 * Two special flags allow us to use this function in special ways, the
 * "PROJECT_HIDE" flag allows us to perform "invisible" projections, while
 * the "PROJECT_JUMP" flag allows us to affect a specific grid, without
 * actually projecting from the source monster (or player).
 *
 * The player will only get "experience" for monsters killed by himself
 * Unique monsters can only be destroyed by attacks from the player
 *
 * Only 256 grids can be affected per projection, limiting the effective
 * "radius" of standard ball attacks to nine units (diameter nineteen).
 *
 * One can project in a given "direction" by combining PROJECT_THRU with small
 * offsets to the initial location (see "line_spell()"), or by calculating
 * "virtual targets" far away from the player.
 *
 * One can also use PROJECT_THRU to send a beam/bolt along an angled path,
 * continuing until it actually hits somethings (useful for "stone to mud").
 *
 * Bolts and Beams explode INSIDE walls, so that they can destroy doors.
 *
 * Balls must explode BEFORE hitting walls, or they would affect monsters
 * on both sides of a wall.  Some bug reports indicate that this is still
 * happening in 2.7.8 for Windows, though it appears to be impossible.
 *
 * We "pre-calculate" the blast area only in part for efficiency.
 * More importantly, this lets us do "explosions" from the "inside" out.
 * This results in a more logical distribution of "blast" treasure.
 * It also produces a better (in my opinion) animation of the explosion.
 * It could be (but is not) used to have the treasure dropped by monsters
 * in the middle of the explosion fall "outwards", and then be damaged by
 * the blast as it spreads outwards towards the treasure drop location.
 *
 * Walls and doors are included in the blast area, so that they can be
 * "burned" or "melted" in later versions.
 *
 * This algorithm is intended to maximize simplicity, not necessarily
 * efficiency, since this function is not a bottleneck in the code.
 *
 * We apply the blast effect from ground zero outwards, in several passes,
 * first affecting features, then objects, then monsters, then the player.
 * This allows walls to be removed before checking the object or monster
 * in the wall, and protects objects which are dropped by monsters killed
 * in the blast, and allows the player to see all affects before he is
 * killed or teleported away.  The semantics of this method are open to
 * various interpretations, but they seem to work well in practice.
 *
 * We process the blast area from ground-zero outwards to allow for better
 * distribution of treasure dropped by monsters, and because it provides a
 * pleasing visual effect at low cost.
 *
 * Note that the damage done by "ball" explosions decreases with distance.
 * This decrease is rapid, grids at radius "dist" take "1/dist" damage.
 *
 * Notice the "napalm" effect of "beam" weapons.  First they "project" to
 * the target, and then the damage "flows" along this beam of destruction.
 * The damage at every grid is the same as at the "center" of a "ball"
 * explosion, since the "beam" grids are treated as if they ARE at the
 * center of a "ball" explosion.
 *
 * Currently, specifying "beam" plus "ball" means that locations which are
 * covered by the initial "beam", and also covered by the final "ball", except
 * for the final grid (the epicenter of the ball), will be "hit twice", once
 * by the initial beam, and once by the exploding ball.  For the grid right
 * next to the epicenter, this results in 150% damage being done.  The center
 * does not have this problem, for the same reason the final grid in a "beam"
 * plus "bolt" does not -- it is explicitly removed.  Simply removing "beam"
 * grids which are covered by the "ball" will NOT work, as then they will
 * receive LESS damage than they should.  Do not combine "beam" with "ball".
 *
 * The array "gy[],gx[]" with current size "grids" is used to hold the
 * collected locations of all grids in the "blast area" plus "beam path".
 *
 * Note the rather complex usage of the "gm[]" array.  First, gm[0] is always
 * zero.  Second, for N>1, gm[N] is always the index (in gy[],gx[]) of the
 * first blast grid (see above) with radius "N" from the blast center.  Note
 * that only the first gm[1] grids in the blast area thus take full damage.
 * Also, note that gm[rad+1] is always equal to "grids", which is the total
 * number of blast grids.
 *
 * Note that once the projection is complete, (y2,x2) holds the final location
 * of bolts/beams, and the "epicenter" of balls.
 *
 * Note also that "rad" specifies the "inclusive" radius of projection blast,
 * so that a "rad" of "one" actually covers 5 or 9 grids, depending on the
 * implementation of the "distance" function.  Also, a bolt can be properly
 * viewed as a "ball" with a "rad" of "zero".
 *
 * Note that if no "target" is reached before the beam/bolt/ball travels the
 * maximum distance allowed (MAX_RANGE), no "blast" will be induced.  This
 * may be relevant even for bolts, since they have a "1x1" mini-blast.
 *
 * Some people have requested an "auto-explode ball attacks at max range"
 * option, which should probably be handled by this function.  XXX XXX XXX
 *
 * Note that for consistency, we "pretend" that the bolt actually takes "time"
 * to move from point A to point B, even if the player cannot see part of the
 * projection path.  Note that in general, the player will *always* see part
 * of the path, since it either starts at the player or ends on the player.
 *
 * Hack -- we assume that every "projection" is "self-illuminating".
 */
bool project(int who, int rad, int Depth, int y, int x, int dam, int typ, int flg)
{
	int			i, j, t;
	int                 y1, x1, y2, x2;
	int			/*y0, x0,*/ y9, x9;
	int			dist;
	int			who_can_see[26], num_can_see = 0;

	/*int			msec = delay_factor * delay_factor * delay_factor;*/

	/* Affected location(s) */
	cave_type *c_ptr;

	/* Effect density, 1 in density frames are actually drawn */
	int density = 5;	
	
	/* Assume the player sees nothing */
	bool notice = FALSE;

	/* Assume the player has seen nothing */
	/*bool visual = FALSE;*/

	/* Assume the player has seen no blast grids */
	bool drawn = FALSE;

	/* Is the player blind? */
	/* Blindness is currently ignored for this function */
	/*bool blind;*/

	/* Number of grids in the "blast area" (including the "beam" path) */
	int grids = 0;
	
	/* Coordinates of the affected grids */
	byte gx[256], gy[256];

	/* Encoded "radius" info (see above) */
	byte gm[16];

	/* Pointer to source player (or NULL if it was a monster) */
	player_type *q_ptr = (who < 0) ? Players[0 - who] : NULL;

	/* Location of player */
	/*y0 = py;
	x0 = px;*/


	/* Hack -- Jump to target */
	if (flg & PROJECT_JUMP)
	{
		x1 = x;
		y1 = y;
	}

	/* Hack -- Start at a player */
	else if (who < 0)
	{
		x1 = Players[0 - who]->px;
		y1 = Players[0 - who]->py;
	}

	/* Start at a monster */
	else
	{
		x1 = m_list[who].fx;
		y1 = m_list[who].fy;
	}


	/* Default "destination" */
	y2 = y; x2 = x;


	/* Hack -- verify stuff */
	if (flg & PROJECT_THRU)
	{
		if ((x1 == x2) && (y1 == y2))
		{
			flg &= ~PROJECT_THRU;
		}
	}


	/* Hack -- Assume there will be no blast (max radius 16) */
	for (dist = 0; dist < 16; dist++) gm[dist] = 0;


	/* Hack -- Handle stuff */
	/*handle_stuff();*/


	/* Start at the source */
	x = x9 = x1;
	y = y9 = y1;
	dist = 0;

	/* Project until done */
	while (1)
	{
		/* Gather beam grids */
		if (flg & PROJECT_BEAM)
		{
			gy[grids] = y;
			gx[grids] = x;
			grids++;
		}

		/* Check the grid */
		c_ptr = &cave[Depth][y][x];

		/* XXX XXX Hack -- Display "beam" grids */
		if (!(flg & PROJECT_HIDE) &&
		    dist && (flg & PROJECT_BEAM))
		{
			/* Hack -- Visual effect -- "explode" the grids */
			for (j = 1; j <= NumPlayers; j++)
			{
				player_type *p_ptr = Players[j];
				int dispx, dispy, p;
				byte attr;
				char ch;

				if (p_ptr->dun_depth != Depth)
					continue;

				if (p_ptr->blind)
					continue;

				if (!panel_contains(p_ptr, y, x))
					continue;

				if (!player_has_los_bold(p_ptr, y, x))
					continue;

				/* Obtain the bolt pict */
				p = bolt_pict(p_ptr, y, x, y, x, typ);

				/* Extract attr/char */
				attr = PICT_A(p);
				ch = PICT_C(p);

				dispx = x - p_ptr->panel_col_prt;
				dispy = y - p_ptr->panel_row_prt;

				//attr = spell_color(typ);

				/* Beams can have a slightly higher density */
				if (randint1(density>>1) == 1)
				{
					//p_ptr->scr_info[dispy][dispx].c = ch;
					//p_ptr->scr_info[dispy][dispx].a = attr;
					//Stream_tile(j, p_ptr, dispy, dispx);
					/* Tell the client */
					(void)send_air_char(p_ptr, dispy, dispx, attr, ch, 1, density);
				}
			}
		}

		/* Never pass through walls */
		if (dist && !cave_floor_bold(Depth, y, x)) break;

		/* Check for arrival at "final target" (if desired) */
		if (!(flg & PROJECT_THRU) && (x == x2) && (y == y2)) break;

		/* If allowed, and we have moved at all, stop when we hit anybody */		
		/* -AD- Only stop if it isn't a party member */		   
				
		if ((c_ptr->m_idx != 0) && dist && (flg & PROJECT_STOP))
		{
			/* always hit monsters */
			if (c_ptr->m_idx > 0) break;
			
			/* healing spells hit everybody */			
			if (typ >= GF_HEAL_PLAYER) break;
			
			/* neutral people hit each other */			
			if (who < 0 && !q_ptr->party) break;
			
			/* people not in the same party hit each other */			
			if (who < 0 && !player_in_party(q_ptr->party, Players[0 - c_ptr->m_idx])) break;
		}


		/* Calculate the new location */
		y9 = y;
		x9 = x;
		mmove2(&y9, &x9, y1, x1, y2, x2);

		/* Hack -- Balls explode BEFORE reaching walls or doors */
		if (!cave_floor_bold(Depth, y9, x9) && (rad > 0)) break;

		/* Keep track of the distance traveled */
		dist++;

		/* Nothing can travel furthur than the maximal distance */
		if (dist > MAX_RANGE) break;

		/* Only do visual effects (and delay) if requested */
		if (!(flg & PROJECT_HIDE))
		{
			for (j = 1; j <= NumPlayers; j++)
			{
				player_type *p_ptr = Players[j];
				u16b p;
				int dispy, dispx;
				char ch;
				byte attr;

				if (p_ptr->dun_depth != Depth)
					continue;

				if (p_ptr->blind)
					continue;

				if (!panel_contains(p_ptr, y9, x9))
					continue;

				if (!player_has_los_bold(p_ptr, y9, x9))
					continue;

				dispx = x9 - p_ptr->panel_col_prt;
				dispy = y9 - p_ptr->panel_row_prt;

				/* Obtain the bolt pict */
				p = bolt_pict(p_ptr, y, x, y9, x9, typ);

				/* Extract attr/char */
				attr = PICT_A(p);
				ch = PICT_C(p);

				//ch = bolt_char(y, x, y9, x9);
				//attr = spell_color(typ);

				if (randint1(density) == 1) 
				{
					//p_ptr->scr_info[dispy][dispx].c = ch;
					//p_ptr->scr_info[dispy][dispx].a = attr;
					//Stream_tile(j, p_ptr, dispy, dispx);
					//Send_flush(j);
					/* Tell the client */
					(void)send_air_char(p_ptr, dispy, dispx, attr, ch, 1, density);
				}
			}
		}
		/* Clean up */
		everyone_lite_spot(Depth, y9, x9);

		/* Save the new location */
		y = y9;
		x = x9;
	
	}


	/* Save the "blast epicenter" */
	y2 = y;
	x2 = x;

	/* Start the "explosion" */
	gm[0] = 0;

	/* Hack -- make sure beams get to "explode" */
	gm[1] = grids;

	/* If we found a "target", explode there */
	if (dist <= MAX_RANGE)
	{
		/* Mega-Hack -- remove the final "beam" grid */
		/* if ((flg & PROJECT_BEAM) && (grids > 0)) grids--; */

		/* Determine the blast area, work from the inside out */
		for (dist = 0; dist <= rad; dist++)
		{
			/* Scan the maximal blast area of radius "dist" */
			for (y = y2 - dist; y <= y2 + dist; y++)
			{
				for (x = x2 - dist; x <= x2 + dist; x++)
				{
					/* Ignore "illegal" locations */
					if (!in_bounds2(Depth, y, x)) continue;

					/* Enforce a "circular" explosion */
					if (distance(y2, x2, y, x) != dist) continue;

					/* Ball explosions are stopped by walls */
					if (!los(Depth, y2, x2, y, x)) continue;

					/* Save this grid */
					gy[grids] = y;
					gx[grids] = x;
					grids++;
				}
			}

			/* Encode some more "radius" info */
			gm[dist+1] = grids;
		}
	}

	/* Speed -- ignore "non-explosions" */
	if (!grids) return (FALSE);

	/* Display the "blast area" */
	if (!(flg & PROJECT_HIDE))
	{
		/* Then do the "blast", from inside out */
		for (t = 0; t <= rad; t++)
		{
			/* Reset who can see */
			num_can_see = 0;

			/* Dump everything with this radius */
			for (i = gm[t]; i < gm[t+1]; i++)
			{
			    if(randint1(density) != 1) continue;
				/* Extract the location */
				y = gy[i];
				x = gx[i];

				for (j = 1; j <= NumPlayers; j++)
				{
					player_type *p_ptr = Players[j];
					int dispy, dispx;
					u16b p;
					byte attr;
					char ch;
					int k;
					bool flag = TRUE;

					if (p_ptr->dun_depth != Depth)
						continue;

					if (p_ptr->blind)
						continue;

					if (!panel_contains(p_ptr, y, x))
						continue;

					if (!player_has_los_bold(p_ptr, y, x))
						continue;

					/* Obtain the bolt pict */
					p = bolt_pict(p_ptr, y, x, y, x, typ);

					/* Extract attr/char */
					attr = PICT_A(p);
					ch = PICT_C(p);

					//attr = spell_color(typ);

					dispx = x - p_ptr->panel_col_prt;
					dispy = y - p_ptr->panel_row_prt;

					//p_ptr->scr_info[dispy][dispx].c = ch;
					//p_ptr->scr_info[dispy][dispx].a = attr;

					//Stream_tile(j, p_ptr, dispy, dispx);

					/* Tell the client */
					(void)send_air_char(p_ptr, dispy, dispx, attr, ch, 1, density);

					drawn = TRUE;

					for (k = 0; k < num_can_see; k++)
					{
						if (who_can_see[k] == j)
							flag = FALSE;
					}
							
					if (flag)
						who_can_see[num_can_see++] = j;
				}
			}

			/* Flush each "radius" seperately */
			//for (j = 0; j < num_can_see; j++)
			//{
				/* Show this radius and delay */
				//Send_flush(who_can_see[j]);
			//}
		}

		/* Flush the erasing */
		if (FALSE)
		{
			/* Erase the explosion drawn above */
			for (i = 0; i < grids; i++)
			{
				/* Extract the location */
				y = gy[i];
				x = gx[i];

				/* Erase if needed */
				everyone_lite_spot(Depth, y, x);
			}

			/* Flush the explosion */
			for (j = 0; j < num_can_see; j++)
			{
				/* Show this radius and delay */
				Send_flush(who_can_see[j]);
			}
		}
	}

	/* Check features */
	if (flg & PROJECT_GRID)
	{
		/* Start with "dist" of zero */
		dist = 0;

		/* Now hurt the cave grids (and objects) from the inside out */
		for (i = 0; i < grids; i++)
		{
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			/* Affect the feature */
			if (project_f(who, dist, Depth, y, x, dam, typ)) notice = TRUE;
		}
	}


	/* Check objects */
	if (flg & PROJECT_ITEM)
	{
		/* Start with "dist" of zero */
		dist = 0;

		/* Now hurt the cave grids (and objects) from the inside out */
		for (i = 0; i < grids; i++)
		{
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			/* Affect the object */
			if (project_i(who, dist, Depth, y, x, dam, typ)) notice = TRUE;
		}
	}


	/* Check monsters */
	if (flg & PROJECT_KILL)
	{
		/* Start with "dist" of zero */
		dist = 0;

		/* Mega-Hack */
		project_m_n = 0;
		project_m_x = 0;
		project_m_y = 0;

		/* Now hurt the monsters, from inside out */
		for (i = 0; i < grids; i++)
		{
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			/* Walls protect monsters */
			if (!cave_floor_bold(Depth, y, x)) continue;

			/* Affect the monster */
			if (project_m(who, dist, Depth, y, x, dam, typ)) notice = TRUE;
		}

		/* Mega-Hack */
		if ((who < 0) && (project_m_n == 1) && !(flg & PROJECT_JUMP))
		{
			/* Location */
			x = project_m_x;
			y = project_m_y;

			/* Still here */
			if (who < 0)
			{
				player_type *p_ptr = Players[0 - who];
				int m_idx = cave[Depth][y][x].m_idx;

				/* Hack - auto-track monster */
				if (m_idx > 0)
				{
					int r_idx = m_list[m_idx].r_idx;
					if (p_ptr->mon_vis[m_idx]) monster_race_track(p_ptr, r_idx);
					if (p_ptr->mon_vis[m_idx]) health_track(p_ptr, m_idx);
				}
			}
		}
	}


	/* Check player */
	if (flg & PROJECT_KILL)
	{
		/* Start with "dist" of zero */
		dist = 0;

		/* Players didn't get hurt yet */
		for (j = 1; j <= NumPlayers; j++)
		{
			player_type *p_ptr = Players[j];
			if (p_ptr->dun_depth != Depth) continue;
			p_ptr->project_hurt = FALSE;
		}

		/* Mega-Hack */
		project_m_n = 0;
		project_m_x = 0;
		project_m_y = 0;

		/* Now see if the player gets hurt */
		for (i = 0; i < grids; i++)
		{
			/* Who is at the location */
			player_type *p_ptr;

			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			/* Get the player (or NULL) */
			p_ptr = player_on_cave(Depth, y, x);

			/* Affect the player */
			if (project_p(p_ptr, who, dist, Depth, y, x, dam, typ)) notice = TRUE;
		}

		/* Mega-Hack */
		if ((who < 0) && (project_m_n == 1) && !(flg & PROJECT_JUMP))
		{
			/* Location */
			x = project_m_x;
			y = project_m_y;

			/* Still here */
			if (who < 0)
			{
				player_type *p_ptr = Players[0 - who];
				int m_idx = cave[Depth][y][x].m_idx;

				/* Hack - auto-track player */
				if (m_idx < 0)
				{
					if (p_ptr->play_vis[0 - m_idx]) health_track(p_ptr, m_idx);
				}
			}
		}		
	}


	/* Return "something was noticed" */
	return (notice);
}
