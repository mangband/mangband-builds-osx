/* File: randart.c */

/* 
 * Purpose: Random artifacts 
 * Adapted from Greg Wooledge's artifact randomiser.
 */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "mangband.h"

/* How much power is assigned to randarts */
#define RANDART_QUALITY	40

/* How many attempts to add abilities */
#define MAX_TRIES 200


#define sign(x)	((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))

/*
 * We will prototype the artifact in this:
 */

artifact_type	randart;

artifact_type	*a_ptr;
object_kind 	*k_ptr;


/* 
 * Power assigned to artifact being made.
 */
s16b	power;


/* 
 * Calculate the multiplier we'll get with a given bow type.
 * This is done differently in 2.8.2 than it was in 2.8.1.
 */
static int bow_multiplier (int sval)
{
	switch (sval)
	{
		case SV_SLING: case SV_SHORT_BOW: return 2;

		case SV_LONG_BOW: case SV_LIGHT_XBOW: return 3;

		case SV_HEAVY_XBOW: return 4;
      	}

	return 0;
}


/* 
 * We've just added an ability which uses the pval bonus.
 * Make sure it's not zero.  If it's currently negative, leave
 * it negative (heh heh).
 */
static void do_pval (artifact_type *a_ptr)
{
	/* Add some pval */
	if (a_ptr->pval == 0)
	{
		a_ptr->pval = 1 + rand_int (3);
	}

	/* Cursed -- make it worse! */
	else if (a_ptr->pval < 0)
	{
		if (rand_int (2) == 0) a_ptr->pval--;
	}

	/* Bump up an existing pval */
	else if (rand_int (3) > 0)
	{
		a_ptr->pval++;
	}

	/* Done */
	return;
}


/* 
 * Make it bad, or if it's already bad, make it worse!
 */
static void do_curse (artifact_type *a_ptr)
{
	/* Some chance of picking up these flags */

	/* Some chance or reversing good bonuses */
	if ((a_ptr->pval > 0) && (rand_int (2) == 0)) a_ptr->pval = -a_ptr->pval;
	if ((a_ptr->to_a > 0) && (rand_int (2) == 0)) a_ptr->to_a = -a_ptr->to_a;
	if ((a_ptr->to_h > 0) && (rand_int (2) == 0)) a_ptr->to_h = -a_ptr->to_h;
	if ((a_ptr->to_d > 0) && (rand_int (4) == 0)) a_ptr->to_d = -a_ptr->to_d;

	/* Some chance of making bad bonuses worse */
	if ((a_ptr->pval < 0) && (rand_int (2) == 0)) a_ptr->pval -= randint0(2);
	if ((a_ptr->to_a < 0) && (rand_int (2) == 0)) a_ptr->to_a -= 3 + randint0(10);
	if ((a_ptr->to_h < 0) && (rand_int (2) == 0)) a_ptr->to_h -= 3 + randint0(6);
	if ((a_ptr->to_d < 0) && (rand_int (4) == 0)) a_ptr->to_d -= 3 + randint0(6);

	/* If it is cursed, we can heavily curse it */
	if (a_ptr->flags3 & TR3_LIGHT_CURSE)
	{
		if (rand_int (2) == 0) a_ptr->flags3 |= TR3_HEAVY_CURSE;
		return;
	}

	/* Always light curse the item */
	a_ptr->flags3 |= TR3_LIGHT_CURSE;

	/* Chance of heavy curse */
	if (rand_int (4) == 0) a_ptr->flags3 |= TR3_HEAVY_CURSE;
}


/* 
 * Evaluate the artifact's overall power level.
 */
static s32b artifact_power (artifact_type *a_ptr)
{
	s32b p = 0;
	int immunities = 0;

	/* Start with a "power" rating derived from the base item's level. */
	p = (k_ptr->level + 7) / 8;

	/* Evaluate certain abilities based on type of object. */
	switch (a_ptr->tval)
	{
		case TV_BOW:
		{
			int mult;

			p += (a_ptr->to_d + sign (a_ptr->to_d)) / 2;
			mult = bow_multiplier (a_ptr->sval);
			if (a_ptr->flags1 & TR1_MIGHT)
			{
				if (a_ptr->pval > 3)
				{
					p += 20000;	/* inhibit */
					mult = 1;	/* don't overflow */
				}
				else
					mult += a_ptr->pval;
			}
			p *= mult;
			if (a_ptr->flags1 & TR1_SHOTS)
			{
				if (a_ptr->pval > 3)
					p += 20000;	/* inhibit */
				else if (a_ptr->pval > 0)
					p *= (2 * a_ptr->pval);
			}
			p += (a_ptr->to_h + 3 * sign (a_ptr->to_h)) / 4;
			if (a_ptr->weight < k_ptr->weight) p++;
			break;
		}
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		{
			p += (a_ptr->dd * a_ptr->ds + 1) / 2;
			if (a_ptr->flags1 & TR1_KILL_DRAGON) p = (p * 3) / 2;
			if (a_ptr->flags1 & TR1_KILL_DEMON) p = (p * 3) / 2;
			if (a_ptr->flags1 & TR1_KILL_UNDEAD) p = (p * 3) / 2;

			if (a_ptr->flags1 & TR1_SLAY_EVIL) p = (p * 3) / 2;
			if (a_ptr->flags1 & TR1_SLAY_ANIMAL) p = (p * 4) / 3;
			if (a_ptr->flags1 & TR1_SLAY_UNDEAD) p = (p * 4) / 3;
			if (a_ptr->flags1 & TR1_SLAY_DRAGON) p = (p * 4) / 3;
			if (a_ptr->flags1 & TR1_SLAY_DEMON) p = (p * 4) / 3;
			if (a_ptr->flags1 & TR1_SLAY_TROLL) p = (p * 5) / 4;
			if (a_ptr->flags1 & TR1_SLAY_ORC) p = (p * 5) / 4;
			if (a_ptr->flags1 & TR1_SLAY_GIANT) p = (p * 6) / 5;

			if (a_ptr->flags1 & TR1_BRAND_ACID) p = (p * 3) / 2;
			if (a_ptr->flags1 & TR1_BRAND_ELEC) p = (p * 3) / 2;
			if (a_ptr->flags1 & TR1_BRAND_FIRE) p = (p * 4) / 3;
			if (a_ptr->flags1 & TR1_BRAND_COLD) p = (p * 4) / 3;
			if (a_ptr->flags1 & TR1_BRAND_POIS) p = (p * 3) / 2;

			p += (a_ptr->to_d + 2 * sign (a_ptr->to_d)) / 3;
			if (a_ptr->to_d > 15) p += (a_ptr->to_d - 14) / 2;

			if ((a_ptr->flags1 & TR1_TUNNEL) &&
			    (a_ptr->tval != TV_DIGGING))
				p += a_ptr->pval * 3;

			p += (a_ptr->to_h + 3 * sign (a_ptr->to_h)) / 4;

			/* Remember, weight is in 0.1 lb. units. */
			if (a_ptr->weight != k_ptr->weight)
				p += (k_ptr->weight - a_ptr->weight) / 20;

			break;
		}
		case TV_BOOTS:
		case TV_GLOVES:
		case TV_HELM:
		case TV_CROWN:
		case TV_SHIELD:
		case TV_CLOAK:
		case TV_SOFT_ARMOR:
		case TV_HARD_ARMOR:
		{
			p += (a_ptr->ac + 4 * sign (a_ptr->ac)) / 5;
			p += (a_ptr->to_h + sign (a_ptr->to_h)) / 2;
			p += (a_ptr->to_d + sign (a_ptr->to_d)) / 2;
			if (a_ptr->weight != k_ptr->weight)
				p += (k_ptr->weight - a_ptr->weight) / 30;
			break;
		}
		case TV_LITE:
		{
			p += 35;
			break;
		}
		case TV_RING:
		case TV_AMULET:
		{
			p += 20;
			break;
		}
	}

	/* Other abilities are evaluated independent of the object type. */
	p += (a_ptr->to_a + 3 * sign (a_ptr->to_a)) / 4;
	if (a_ptr->to_a > 20) p += (a_ptr->to_a - 19) / 2;
	if (a_ptr->to_a > 30) p += (a_ptr->to_a - 29) / 2;
	if (a_ptr->to_a > 40) p += 20000;	/* inhibit */

	if (a_ptr->pval > 0)
	{
		if (a_ptr->flags1 & TR1_STR) p += a_ptr->pval * a_ptr->pval;
		if (a_ptr->flags1 & TR1_INT) p += a_ptr->pval * a_ptr->pval;
		if (a_ptr->flags1 & TR1_WIS) p += a_ptr->pval * a_ptr->pval;
		if (a_ptr->flags1 & TR1_DEX) p += a_ptr->pval * a_ptr->pval;
		if (a_ptr->flags1 & TR1_CON) p += a_ptr->pval * a_ptr->pval;
		if (a_ptr->flags1 & TR1_STEALTH) p += a_ptr->pval * a_ptr->pval;
		if (a_ptr->flags1 & TR1_SEARCH) p += a_ptr->pval * a_ptr->pval;
	}
	else if (a_ptr->pval < 0)	/* hack: don't give large negatives */
	{
		if (a_ptr->flags1 & TR1_STR) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_INT) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_WIS) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_DEX) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_CON) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_STEALTH) p += a_ptr->pval;
		if (a_ptr->flags1 & TR1_SEARCH) p += a_ptr->pval;
	}
	if (a_ptr->flags1 & TR1_CHR) p += a_ptr->pval;
	if (a_ptr->flags1 & TR1_INFRA) p += (a_ptr->pval + sign (a_ptr->pval)) / 2;
        if (a_ptr->flags1 & TR1_SPEED) p += a_ptr->pval * 2;
	/*if (a_ptr->flags1 & TR1_MANA) p += a_ptr->pval * 2;*/

	if (a_ptr->flags2 & TR2_SUST_STR) p += 5;
	if (a_ptr->flags2 & TR2_SUST_INT) p += 4;
	if (a_ptr->flags2 & TR2_SUST_WIS) p += 4;
	if (a_ptr->flags2 & TR2_SUST_DEX) p += 4;
	if (a_ptr->flags2 & TR2_SUST_CON) p += 5;
	if (a_ptr->flags2 & TR2_SUST_CHR) p += 1;
	if (a_ptr->flags2 & TR2_IM_ACID)
	{
		p += 24;
		immunities++;
	}
	if (a_ptr->flags2 & TR2_IM_ELEC)
	{
		p += 20;
		immunities++;
	}
	if (a_ptr->flags2 & TR2_IM_FIRE)
	{
		p += 32;
		immunities++;
	}
	if (a_ptr->flags2 & TR2_IM_COLD)
	{
		p += 24;
		immunities++;
	}
	if (immunities > 1) p += 10;
	if (immunities > 2) p += 10;
	if (immunities > 3) p += 20000;		/* inhibit */
	if (a_ptr->flags2 & TR2_RES_FEAR) p += 4;
	if (a_ptr->flags3 & TR3_FREE_ACT) p += 8;
	if (a_ptr->flags3 & TR3_HOLD_LIFE) p += 10;
	if (a_ptr->flags2 & TR2_RES_ACID) p += 6;
	if (a_ptr->flags2 & TR2_RES_ELEC) p += 6;
	if (a_ptr->flags2 & TR2_RES_FIRE) p += 6;
	if (a_ptr->flags2 & TR2_RES_COLD) p += 6;
	if (a_ptr->flags2 & TR2_RES_POIS) p += 12;
	if (a_ptr->flags2 & TR2_RES_LITE) p += 8;
	if (a_ptr->flags2 & TR2_RES_DARK) p += 10;
	if (a_ptr->flags2 & TR2_RES_BLIND) p += 10;
	if (a_ptr->flags2 & TR2_RES_CONFU) p += 8;
	if (a_ptr->flags2 & TR2_RES_SOUND) p += 10;
	if (a_ptr->flags2 & TR2_RES_SHARD) p += 8;
	if (a_ptr->flags2 & TR2_RES_NETHR) p += 12;
	if (a_ptr->flags2 & TR2_RES_NEXUS) p += 10;
	if (a_ptr->flags2 & TR2_RES_CHAOS) p += 12;
	if (a_ptr->flags2 & TR2_RES_DISEN) p += 13;

	if (a_ptr->flags3 & TR3_FEATHER) p += 2;
	if (a_ptr->flags3 & TR3_LITE) p += 2;
	if (a_ptr->flags3 & TR3_SEE_INVIS) p += 8;
/*
        if (a_ptr->flags4 & TR4_ESP_ORC) p += 1;
	if (a_ptr->flags4 & TR4_ESP_TROLL) p += 1;
	if (a_ptr->flags4 & TR4_ESP_DRAGON) p += 4;
	if (a_ptr->flags4 & TR4_ESP_GIANT) p += 2;
	if (a_ptr->flags4 & TR4_ESP_DEMON) p += 3;
	if (a_ptr->flags4 & TR4_ESP_UNDEAD) p += 3;
        if (a_ptr->flags4 & TR4_ESP_EVIL) p += 14;
	if (a_ptr->flags4 & TR4_ESP_ANIMAL) p += 3;
	if (a_ptr->flags4 & TR4_ESP_ALL) p += 22;
*/
        if (a_ptr->flags3 & TR3_SLOW_DIGEST) p += 4;
	if (a_ptr->flags3 & TR3_REGEN) p += 8;
	if (a_ptr->flags3 & TR3_TELEPORT) p -= 20;
	if (a_ptr->flags3 & TR3_DRAIN_EXP) p -= 16;
	if (a_ptr->flags3 & TR3_AGGRAVATE) p -= 8;
	if (a_ptr->flags3 & TR3_BLESSED) p += 4;
	if (a_ptr->flags3 & TR3_LIGHT_CURSE) p -= 2;
	if (a_ptr->flags3 & TR3_HEAVY_CURSE) p -= 4;
	if (a_ptr->flags3 & TR3_PERMA_CURSE) p -= 8;
	if (a_ptr->flags3 & TR3_HEAVY_CURSE) p -= 20;

	return p;
}


static void remove_contradictory (artifact_type *a_ptr)
{
	if (a_ptr->flags3 & TR3_AGGRAVATE) a_ptr->flags1 &= ~(TR1_STEALTH);
	if (a_ptr->flags2 & TR2_IM_ACID) a_ptr->flags2 &= ~(TR2_RES_ACID);
	if (a_ptr->flags2 & TR2_IM_ELEC) a_ptr->flags2 &= ~(TR2_RES_ELEC);
	if (a_ptr->flags2 & TR2_IM_FIRE) a_ptr->flags2 &= ~(TR2_RES_FIRE);
	if (a_ptr->flags2 & TR2_IM_COLD) a_ptr->flags2 &= ~(TR2_RES_COLD);
	if (a_ptr->pval < 0)
	{
		if (a_ptr->flags1 & TR1_STR) a_ptr->flags2 &= ~(TR2_SUST_STR);
		if (a_ptr->flags1 & TR1_INT) a_ptr->flags2 &= ~(TR2_SUST_INT);
		if (a_ptr->flags1 & TR1_WIS) a_ptr->flags2 &= ~(TR2_SUST_WIS);
		if (a_ptr->flags1 & TR1_DEX) a_ptr->flags2 &= ~(TR2_SUST_DEX);
		if (a_ptr->flags1 & TR1_CON) a_ptr->flags2 &= ~(TR2_SUST_CON);
		if (a_ptr->flags1 & TR1_CHR) a_ptr->flags2 &= ~(TR2_SUST_CHR);
	}
	if (a_ptr->flags3 & TR3_LIGHT_CURSE) a_ptr->flags3 &= ~(TR3_BLESSED);
	if (a_ptr->flags3 & TR3_HEAVY_CURSE) a_ptr->flags3 &= ~(TR3_BLESSED);
	if (a_ptr->flags3 & TR3_PERMA_CURSE) a_ptr->flags3 &= ~(TR3_BLESSED);
	if (a_ptr->flags3 & TR3_DRAIN_EXP) a_ptr->flags3 &= ~(TR3_HOLD_LIFE);

	/* Remove redundant slay mods */
        if (a_ptr->flags1 & TR1_KILL_DRAGON) a_ptr->flags1 &= ~(TR1_SLAY_DRAGON);
	if (a_ptr->flags1 & TR1_KILL_UNDEAD) a_ptr->flags1 &= ~(TR1_SLAY_UNDEAD);
	if (a_ptr->flags1 & TR1_KILL_DEMON) a_ptr->flags1 &= ~(TR1_SLAY_DEMON);

	/* Remove redundant resistances */
	if (a_ptr->flags2 & TR2_RES_CHAOS) a_ptr->flags2 &= ~(TR2_RES_CONFU);
}


static void remove_redundant_esp(artifact_type *a_ptr)
{
/*
	if (a_ptr->flags4 & TR4_ESP_EVIL)
	{
		a_ptr->flags4 &= ~(TR4_ESP_UNDEAD);
		a_ptr->flags4 &= ~(TR4_ESP_DEMON);
		a_ptr->flags4 &= ~(TR4_ESP_ORC);
		a_ptr->flags4 &= ~(TR4_ESP_TROLL);
		a_ptr->flags4 &= ~(TR4_ESP_GIANT);
	}
	if (a_ptr->flags4 & TR4_ESP_ALL)
	{
		a_ptr->flags4 &= ~(TR4_ESP_ANIMAL);
		a_ptr->flags4 &= ~(TR4_ESP_EVIL);
		a_ptr->flags4 &= ~(TR4_ESP_UNDEAD);
		a_ptr->flags4 &= ~(TR4_ESP_DEMON);
		a_ptr->flags4 &= ~(TR4_ESP_ORC);
		a_ptr->flags4 &= ~(TR4_ESP_TROLL);
		a_ptr->flags4 &= ~(TR4_ESP_GIANT);
		a_ptr->flags4 &= ~(TR4_ESP_DRAGON);
	}
*/
}


static void add_esp(artifact_type *a_ptr)
{
/*
        int rr = rand_int (12);
	if (rr < 1) a_ptr->flags4 |= TR4_ESP_ORC;
	else if (rr < 2) a_ptr->flags4 |= TR4_ESP_TROLL;
	else if (rr < 3) a_ptr->flags4 |= TR4_ESP_GIANT;
	else if (rr < 4) a_ptr->flags4 |= TR4_ESP_ANIMAL;
	else if (rr < 5) a_ptr->flags4 |= TR4_ESP_DRAGON;
	else if (rr < 6) a_ptr->flags4 |= TR4_ESP_DEMON;
	else if (rr < 7) a_ptr->flags4 |= TR4_ESP_UNDEAD;
	else if (rr < 10) a_ptr->flags4 |= TR4_ESP_EVIL;
	else a_ptr->flags4 |= TR4_ESP_ALL;
*/
}


/* 
 * Randomly select an extra ability to be added to the artifact in question.
 * This function is way too large.
 */
static void add_ability (artifact_type *a_ptr)
{
	int r;
	
	r = rand_int (100);

	if (a_ptr->tval == TV_BOW) r -= 2;
	if (a_ptr->tval == TV_DRAG_ARMOR) r -= 17;
	if ((a_ptr->tval == TV_SHOT) || (a_ptr->tval == TV_ARROW) || (a_ptr->tval == TV_BOLT)) r = 0;

	if (r < 50)		/* Pick something dependent on item type. */
	{
		r = rand_int (100);
		switch (a_ptr->tval)
		{
			case TV_BOW:
			{
				if (r < 25)
				{
					a_ptr->flags1 |= TR1_SHOTS;
				}
				else if (r < 50)
				{
					a_ptr->flags1 |= TR1_MIGHT;
				}
				else if (r < 70) a_ptr->to_h += 4 + randint0(4);
				else if (r < 90) a_ptr->to_d += 4 + randint0(4);
				else
                                {
					a_ptr->to_h += 4 + randint0(4);
					a_ptr->to_d += 4 + randint0(4);
				}

				break;
			}
			case TV_DIGGING:
			case TV_HAFTED:
			case TV_POLEARM:
			case TV_SWORD:
			{
				if (r < 4)
				{
					a_ptr->flags1 |= TR1_WIS;
					do_pval (a_ptr);
					if (rand_int (2) == 0) a_ptr->flags2 |= TR2_SUST_WIS;
					if (a_ptr->tval == TV_SWORD || a_ptr->tval == TV_POLEARM)
						a_ptr->flags3 |= TR3_BLESSED;
				}
				else if (r < 7)
				{
					a_ptr->flags1 |= TR1_BRAND_ACID;
					if (rand_int (4) > 0) a_ptr->flags2 |= TR2_RES_ACID;
				}
				else if (r < 10)
				{
					a_ptr->flags1 |= TR1_BRAND_ELEC;
					if (rand_int (4) > 0) a_ptr->flags2 |= TR2_RES_ELEC;
				}
				else if (r < 14)
				{
					a_ptr->flags1 |= TR1_BRAND_FIRE;
					if (rand_int (4) > 0) a_ptr->flags2 |= TR2_RES_FIRE;
				}
				else if (r < 18)
				{
					a_ptr->flags1 |= TR1_BRAND_COLD;
					if (rand_int (4) > 0) a_ptr->flags2 |= TR2_RES_COLD;
				}
				else if (r < 21)
				{
					a_ptr->flags1 |= TR1_BRAND_POIS;
				}
				else if (r < 27)
				{
					a_ptr->dd += 1 + rand_int (2) + rand_int (2);
				}
				else if (r < 29)
				{
					a_ptr->flags1 |= TR1_KILL_DRAGON;
					/*if (rand_int (2) == 0) a_ptr->flags4 |= TR4_ESP_DRAGON;*/
				}
				else if (r < 31)
				{
					a_ptr->flags1 |= TR1_KILL_DEMON;
					/*if (rand_int (2) == 0) a_ptr->flags4 |= TR4_ESP_DEMON;*/
				}
				else if (r < 33)
				{
					a_ptr->flags1 |= TR1_KILL_UNDEAD;
					/*if (rand_int (2) == 0) a_ptr->flags4 |= TR4_ESP_UNDEAD;*/
				}
				else if (r < 38)
				{
					a_ptr->flags1 |= TR1_SLAY_DRAGON;
					/*if (rand_int (2) == 0) a_ptr->flags4 |= TR4_ESP_DRAGON;*/
				}
				else if (r < 43)
				{
					a_ptr->flags1 |= TR1_SLAY_EVIL;
					/*if (rand_int (3) == 0) a_ptr->flags4 |= TR4_ESP_EVIL;*/
				}

				else if (r < 48)
				{
					a_ptr->flags1 |= TR1_SLAY_ANIMAL;
					/*if (rand_int (2) == 0) a_ptr->flags4 |= TR4_ESP_ANIMAL;*/
				}
				else if (r < 52)
				{
					a_ptr->flags1 |= TR1_SLAY_UNDEAD;
					/*if (rand_int (2) == 0) a_ptr->flags4 |= TR4_ESP_UNDEAD;*/
					if (rand_int (2) == 0)
					{
						a_ptr->flags1 |= TR1_SLAY_DEMON;
						/*if (rand_int (2) == 0) a_ptr->flags4 |= TR4_ESP_DEMON;*/
					}
				}
				else if (r < 56)
				{
					a_ptr->flags1 |= TR1_SLAY_DEMON;
					/*if (rand_int (2) == 0) a_ptr->flags4 |= TR4_ESP_DEMON;*/
					if (rand_int (2) == 0)
					{
						a_ptr->flags1 |= TR1_SLAY_UNDEAD;
						/*if (rand_int (2) == 0) a_ptr->flags4 |= TR4_ESP_UNDEAD;*/
					}
				}
				else if (r < 60)
				{
					a_ptr->flags1 |= TR1_SLAY_ORC;
					/* if (rand_int (2) == 0) a_ptr->flags4 |= TR4_ESP_ORC; */
					if (rand_int (2) == 0)
					{
						a_ptr->flags1 |= TR1_SLAY_TROLL;
						/*if (rand_int (2) == 0) a_ptr->flags4 |= TR4_ESP_TROLL;*/
					}
					if (rand_int (2) == 0)
					{
						a_ptr->flags1 |= TR1_SLAY_GIANT;
						/* if (rand_int (2) == 0) a_ptr->flags4 |= TR4_ESP_GIANT; */
					}
				}
				else if (r < 64)
				{
					a_ptr->flags1 |= TR1_SLAY_TROLL;
					/*if (rand_int (2) == 0) a_ptr->flags4 |= TR4_ESP_TROLL;*/
					if (rand_int (2) == 0)
					{
						a_ptr->flags1 |= TR1_SLAY_ORC;
						/*if (rand_int (2) == 0) a_ptr->flags4 |= TR4_ESP_ORC;*/
					}
					if (rand_int (2) == 0)
					{
						a_ptr->flags1 |= TR1_SLAY_GIANT;
						/*if (rand_int (2) == 0) a_ptr->flags4 |= TR4_ESP_GIANT;*/
					}
				}
				else if (r < 68)
				{
					a_ptr->flags1 |= TR1_SLAY_GIANT;
					/*if (rand_int (2) == 0) a_ptr->flags4 |= TR4_ESP_GIANT;*/
					if (rand_int (2) == 0)
					{
						a_ptr->flags1 |= TR1_SLAY_ORC;
						/*if (rand_int (2) == 0) a_ptr->flags4 |= TR4_ESP_ORC;*/
					}
					if (rand_int (2) == 0)
					{
						a_ptr->flags1 |= TR1_SLAY_TROLL;
						/*if (rand_int (2) == 0) a_ptr->flags4 |= TR4_ESP_TROLL;*/
					}
				}
				else if (r < 73) a_ptr->flags3 |= TR3_SEE_INVIS;
				else if (r < 77)
				{
					a_ptr->flags1 |= TR1_BLOWS;
					do_pval (a_ptr);
					if (a_ptr->pval > 3) a_ptr->pval = 3;
				}
				else if (r < 91)
				{
					a_ptr->to_d += 2 + rand_int (10);
					a_ptr->to_h += 2 + rand_int (10);
				}
				else if (r < 94) a_ptr->to_a += 3 + rand_int (3);
				else if (r < 98)
					a_ptr->weight = (a_ptr->weight * 9) / 10;
				else
					if (a_ptr->tval != TV_DIGGING)
					{
						a_ptr->flags1 |= TR1_TUNNEL;
					do_pval (a_ptr);
					}

				break;
			}
			case TV_SHOT:
			case TV_ARROW:
			case TV_BOLT:
			{
				if (r < 4)
				{
					a_ptr->flags1 |= TR1_BRAND_ACID;
				}
				else if (r < 8)
				{
					a_ptr->flags1 |= TR1_BRAND_ELEC;
				}
				else if (r < 12)
				{
					a_ptr->flags1 |= TR1_BRAND_FIRE;
				}
				else if (r < 16)
				{
					a_ptr->flags1 |= TR1_BRAND_COLD;
				}
				else if (r < 20)
				{
					a_ptr->flags1 |= TR1_BRAND_POIS;
				}
				else if (r < 24)
				{ 
					a_ptr->flags1 |= TR1_KILL_DRAGON;
				}
				else if (r < 28)
				{ 
					a_ptr->flags1 |= TR1_KILL_DEMON;
				}
				else if (r < 32)
				{ 
					a_ptr->flags1 |= TR1_KILL_UNDEAD;
				}
				else if (r < 40)
				{
					a_ptr->flags1 |= TR1_SLAY_DRAGON;
				}
				else if (r < 44)
				{
					a_ptr->flags1 |= TR1_SLAY_EVIL;
				}
				else if (r < 52)
				{
					a_ptr->flags1 |= TR1_SLAY_ANIMAL;
				}
				else if (r < 60)
				{
					a_ptr->flags1 |= TR1_SLAY_UNDEAD;
				}
				else if (r < 68)
				{
					a_ptr->flags1 |= TR1_SLAY_DEMON;
				}
				else if (r < 76)
				{
					a_ptr->flags1 |= TR1_SLAY_ORC;
				}
				else if (r < 84)
				{
					a_ptr->flags1 |= TR1_SLAY_TROLL;
				}
				else if (r < 92)
				{
					a_ptr->flags1 |= TR1_SLAY_GIANT;
				}
				else
				{
				    /* bad luck */
				}
				break;
			}
			case TV_BOOTS:
			{
				if (r < 10) a_ptr->flags3 |= TR3_FEATHER;
				else if (r < 50) a_ptr->to_a += 2 + rand_int (4);
				else if (r < 80)
				{
					a_ptr->flags1 |= TR1_STEALTH;
					do_pval (a_ptr);
				}
				else if (r < 90)
				{
					a_ptr->flags1 |= TR1_SPEED;
					if (a_ptr->pval < 0) break;
					if (a_ptr->pval == 0) a_ptr->pval = 3 + rand_int (8);
					else if (rand_int (2) == 0) a_ptr->pval++;
				}
				else a_ptr->weight = (a_ptr->weight * 9) / 10;
				break;
			}
			case TV_GLOVES: 
			{
				if (r < 20) a_ptr->flags3 |= TR3_FREE_ACT;
				else if (r < 30)
				{
					/*
					a_ptr->flags1 |= TR1_MANA;
					if (a_ptr->pval == 0) a_ptr->pval = 5 + rand_int (6);
					else do_pval (a_ptr);
					if (a_ptr->pval < 0) a_ptr->pval = 2;
					*/
					a_ptr->flags2 |= TR2_SUST_INT;
				}
				else if (r < 55)
				{
					a_ptr->flags1 |= TR1_DEX;
					do_pval (a_ptr);
				}
				else if (r < 70)
				{
					a_ptr->flags1 |= TR1_STR;
					do_pval (a_ptr);
				}
				else if (r < 85) a_ptr->to_a += 3 + rand_int (3);
				else
				{
					a_ptr->to_h += 2 + rand_int (3);
					a_ptr->to_d += 2 + rand_int (3);
					a_ptr->flags3 |= TR3_SHOW_MODS;
				}
				break;
			}
			case TV_HELM:
			case TV_CROWN:
			{
				if (r < 15) a_ptr->flags2 |= TR2_RES_BLIND;
				else if (r < 18)
                                {
					a_ptr->flags1 |= TR1_INFRA;
					if (a_ptr->pval == 0) a_ptr->pval = randint1(2);
				}
				else if (r < 42) add_esp(a_ptr);
				else if (r < 57) a_ptr->flags3 |= TR3_SEE_INVIS;
				else if (r < 67)
				{
					a_ptr->flags1 |= TR1_WIS;
					do_pval (a_ptr);
				}
				else if (r < 77)
				{
					a_ptr->flags1 |= TR1_INT;
					do_pval (a_ptr);
				}
				else if (r < 81) a_ptr->flags2 |= TR2_RES_CONFU;
				else if (r < 85) a_ptr->flags2 |= TR2_RES_FEAR;
				else a_ptr->to_a += 3 + rand_int (3);
				break;
			}
			case TV_SHIELD:
			{
				if (r < 20) a_ptr->flags2 |= TR2_RES_ACID;
				else if (r < 40) a_ptr->flags2 |= TR2_RES_ELEC;
				else if (r < 60) a_ptr->flags2 |= TR2_RES_FIRE;
				else if (r < 80) a_ptr->flags2 |= TR2_RES_COLD;
				else a_ptr->to_a += 3 + rand_int (3);
				break;
			}
			case TV_CLOAK:
			{
				if (r < 15)
				{
					a_ptr->flags3 |= TR3_FEATHER;
				}
				else if (r < 50)
				{
					a_ptr->flags1 |= TR1_STEALTH;
					do_pval (a_ptr);
				}
				else if (r < 60)
				{
					a_ptr->flags2 |= TR2_RES_SHARD;
				}
				else a_ptr->to_a += 3 + rand_int (3);
				break;
			}
			case TV_DRAG_ARMOR:
			{
				if (r < 15) a_ptr->flags3 |= TR3_HOLD_LIFE;
				else if (r < 30)
				{
					a_ptr->flags1 |= TR1_CON;
					do_pval (a_ptr);
					if (rand_int (2) == 0)
						a_ptr->flags2 |= TR2_SUST_CON;
				}
				else if (r < 45)
				{
					a_ptr->flags1 |= TR1_STR;
					do_pval (a_ptr);
					if (rand_int (2) == 0)
						a_ptr->flags2 |= TR2_SUST_STR;
				}
				else a_ptr->to_a += 1 + rand_int (4);
				break;
			}
			case TV_SOFT_ARMOR:
			case TV_HARD_ARMOR:
			{
				if (r < 8)
				{
					a_ptr->flags1 |= TR1_STEALTH;
					do_pval (a_ptr);
				}
				else if (r < 16) a_ptr->flags3 |= TR3_HOLD_LIFE;
				else if (r < 22)
				{
					a_ptr->flags1 |= TR1_CON;
					do_pval (a_ptr);
					if (rand_int (2) == 0)
						a_ptr->flags2 |= TR2_SUST_CON;
				}
				else if (r < 34) a_ptr->flags2 |= TR2_RES_ACID;
				else if (r < 46) a_ptr->flags2 |= TR2_RES_ELEC;
				else if (r < 58) a_ptr->flags2 |= TR2_RES_FIRE;
				else if (r < 70) a_ptr->flags2 |= TR2_RES_COLD;
				else if (r < 80)
					a_ptr->weight = (a_ptr->weight * 9) / 10;
				else a_ptr->to_a += 3 + rand_int (8);
				break;
			}
		}
	}
	else			/* Pick something universally useful. */
	{
		r = rand_int (45);
		switch (r)
		{
			case 0:
			{
				a_ptr->flags1 |= TR1_STR;
				do_pval (a_ptr);
				if (rand_int (2) == 0) a_ptr->flags2 |= TR2_SUST_STR;
				break;
			}
			case 1:
			{
				a_ptr->flags1 |= TR1_INT;
				do_pval (a_ptr);
				if (rand_int (2) == 0) a_ptr->flags2 |= TR2_SUST_INT;
				break;
			}
			case 2:
			{
				a_ptr->flags1 |= TR1_WIS;
				do_pval (a_ptr);
				if (rand_int (2) == 0) a_ptr->flags2 |= TR2_SUST_WIS;
				if (a_ptr->tval == TV_SWORD || a_ptr->tval == TV_POLEARM)
					a_ptr->flags3 |= TR3_BLESSED;
				break;
			}
			case 3:
			{
				a_ptr->flags1 |= TR1_DEX;
				do_pval (a_ptr);
				if (rand_int (2) == 0) a_ptr->flags2 |= TR2_SUST_DEX;
				break;
			}
			case 4:
			{
				a_ptr->flags1 |= TR1_CON;
				do_pval (a_ptr);
				if (rand_int (2) == 0) a_ptr->flags2 |= TR2_SUST_CON;
				break;
			}
			case 5:
			{
				a_ptr->flags1 |= TR1_CHR;
				do_pval (a_ptr);
				if (rand_int (2) == 0) a_ptr->flags2 |= TR2_SUST_CHR;
				break;
			}
			case 6:
			{
				a_ptr->flags1 |= TR1_STEALTH;
				do_pval (a_ptr);
				break;
			}
			case 7:
			{
				a_ptr->flags1 |= TR1_SEARCH;
				do_pval (a_ptr);
				break;
			}
			case 8:
			{
				a_ptr->flags1 |= TR1_INFRA;
				do_pval (a_ptr);
				break;
			}
			case 9:
			{
				a_ptr->flags1 |= TR1_SPEED;
				if (a_ptr->pval == 0) a_ptr->pval = 3 + rand_int (3);
				else do_pval (a_ptr);
				break;
			}
			case 10:
			{
				a_ptr->flags2 |= TR2_SUST_STR;
				if (rand_int (2) == 0)
				{
					a_ptr->flags1 |= TR1_STR;
					do_pval (a_ptr);
				}
				break;
			}
			case 11:
			{
				a_ptr->flags2 |= TR2_SUST_INT;
				if (rand_int (2) == 0)
				{
					a_ptr->flags1 |= TR1_INT;
					do_pval (a_ptr);
				}
				break;
			}
			case 12:
			{
				a_ptr->flags2 |= TR2_SUST_WIS;
				if (rand_int (2) == 0)
				{
					a_ptr->flags1 |= TR1_WIS;
					do_pval (a_ptr);
					if (a_ptr->tval == TV_SWORD || a_ptr->tval == TV_POLEARM)
						a_ptr->flags3 |= TR3_BLESSED;
				}
				break;
			}
			case 13:
			{
				a_ptr->flags2 |= TR2_SUST_DEX;
				if (rand_int (2) == 0)
				{
					a_ptr->flags1 |= TR1_DEX;
					do_pval (a_ptr);
				}
				break;
			}
			case 14:
			{
				a_ptr->flags2 |= TR2_SUST_CON;
				if (rand_int (2) == 0)
				{
					a_ptr->flags1 |= TR1_CON;
					do_pval (a_ptr);
				}
				break;
			}
			case 15:
			{
				a_ptr->flags2 |= TR2_SUST_CHR;
				if (rand_int (2) == 0)
				{
					a_ptr->flags1 |= TR1_CHR;
					do_pval (a_ptr);
				}
				break;
			}
			case 16:
			{
				if (rand_int (3) == 0) a_ptr->flags2 |= TR2_IM_ACID;
				break;
			}
			case 17:
			{
				if (rand_int (3) == 0) a_ptr->flags2 |= TR2_IM_ELEC;
				break;
			}
			case 18:
			{
				if (rand_int (4) == 0) a_ptr->flags2 |= TR2_IM_FIRE;
				break;
			}
			case 19:
			{
				if (rand_int (3) == 0) a_ptr->flags2 |= TR2_IM_COLD;
				break;
			}
			case 20: a_ptr->flags3 |= TR3_FREE_ACT; break;
			case 21: a_ptr->flags3 |= TR3_HOLD_LIFE; break;
			case 22: a_ptr->flags2 |= TR2_RES_ACID; break;
			case 23: a_ptr->flags2 |= TR2_RES_ELEC; break;
			case 24: a_ptr->flags2 |= TR2_RES_FIRE; break;
			case 25: a_ptr->flags2 |= TR2_RES_COLD; break;
			case 26: a_ptr->flags2 |= TR2_RES_POIS; break;
			case 27: a_ptr->flags2 |= TR2_RES_LITE; break;
			case 28: a_ptr->flags2 |= TR2_RES_DARK; break;
			case 29: a_ptr->flags2 |= TR2_RES_BLIND; break;
			case 30: a_ptr->flags2 |= TR2_RES_CONFU; break;
			case 31: a_ptr->flags2 |= TR2_RES_SOUND; break;
			case 32: a_ptr->flags2 |= TR2_RES_SHARD; break;
			case 33:
			{
				if (rand_int (2) == 0)
					a_ptr->flags2 |= TR2_RES_NETHR;
				break;
			}
			case 34: a_ptr->flags2 |= TR2_RES_NEXUS; break;
			case 35: a_ptr->flags2 |= TR2_RES_CHAOS; break;
			case 36:
			{
				if (rand_int (2) == 0)
					a_ptr->flags2 |= TR2_RES_DISEN;
				break;
			}
			case 37: a_ptr->flags3 |= TR3_FEATHER; break;
			case 38: a_ptr->flags3 |= TR3_LITE; break;
			case 39: a_ptr->flags3 |= TR3_SEE_INVIS; break;
		        case 40: add_esp(a_ptr); break;
			case 41: a_ptr->flags3 |= TR3_SLOW_DIGEST; break;
			case 42:
			case 43: a_ptr->flags3 |= TR3_REGEN; break;
			case 44: a_ptr->flags2 |= TR2_RES_FEAR; break;
		}
	}

	/* Now remove contradictory or redundant powers. */
	remove_contradictory (a_ptr);
	remove_redundant_esp (a_ptr);
}


/*
 * Returns pointer to randart artifact_type structure.
 *
 * o_ptr should contain the seed (in name3) plus a tval
 * and sval. It returns NULL on illegal sval and tvals.
 */
artifact_type *randart_make(const object_type *o_ptr)
{
	s32b power = 0;
	int tries;
	s32b ap;
	bool aggravate_me = FALSE;
	
	/* Get pointer to our artifact_type object */
	a_ptr = &randart;

	/* Get pointer to object kind */
	k_ptr = &k_info[o_ptr->k_idx];

	/* Screen for disallowed TVALS */
	if ((k_ptr->tval!=TV_BOW) &&
	    (k_ptr->tval!=TV_DIGGING) &&
	    (k_ptr->tval!=TV_SHOT) &&
		(k_ptr->tval!=TV_ARROW) &&
		(k_ptr->tval!=TV_BOLT) &&
	    (k_ptr->tval!=TV_HAFTED) &&
	    (k_ptr->tval!=TV_POLEARM) &&
	    (k_ptr->tval!=TV_SWORD) &&
	    (k_ptr->tval!=TV_BOOTS) &&
	    (k_ptr->tval!=TV_GLOVES) &&
	    (k_ptr->tval!=TV_HELM) &&
	    (k_ptr->tval!=TV_CROWN) &&
	    (k_ptr->tval!=TV_SHIELD) &&
	    (k_ptr->tval!=TV_CLOAK) &&
	    (k_ptr->tval!=TV_SOFT_ARMOR) &&
	    (k_ptr->tval!=TV_DRAG_ARMOR) &&
	    (k_ptr->tval!=TV_HARD_ARMOR) &&
	    (k_ptr->tval!=TV_RING) &&
	    (k_ptr->tval!=TV_LITE) &&
	    (k_ptr->tval!=TV_AMULET))
	{
		/* Not an allowed type */
		return(NULL);
	}

	/* Magic ammo are always +0 +0 */
    	if (((k_ptr->tval == TV_SHOT) || (k_ptr->tval == TV_ARROW) ||
		(k_ptr->tval == TV_BOLT)) && (k_ptr->sval == SV_AMMO_MAGIC))
		return(NULL);

	/* Set the RNG seed. */
	Rand_value = o_ptr->name3;
	Rand_quick = TRUE;
	
	/* Wipe the artifact_type structure */
	WIPE(&randart, artifact_type);

	/* 
	 * First get basic artifact quality
	 * 90% of normal items are good
	 * cursed items are always bad
	 */
	if (randint0(10) && !cursed_p(o_ptr))
	{
		power = randint0(80) + RANDART_QUALITY;
	
		/* Really powerful items should aggravate. */
		/* Chance = 60% for max power 120 */
		if (power > 100)
		{
			if (rand_int (100) < (power - 100) * 3)
			{
				aggravate_me = TRUE;
			}
		}
	}

	/* Default values */
	a_ptr->cur_num = 0;
	a_ptr->max_num = 1;
	a_ptr->tval = k_ptr->tval;
	a_ptr->sval = k_ptr->sval;
	a_ptr->pval = k_ptr->pval;

	/* Resetting bpval to avoid items like Kolla +2 +6 */
	if (o_ptr->bpval) a_ptr->pval = (o_ptr->pval < o_ptr->bpval)? o_ptr->bpval: o_ptr->pval;
	/*o_ptr->bpval = 0;*//* Moved elsewhere! */

	if (k_ptr->tval == TV_LITE) a_ptr->pval = 0;

	/* Amulets and Rings keep their (+hit, +dam) [+AC] */
	if ((a_ptr->tval == TV_AMULET) || (a_ptr->tval == TV_RING))
	{
	    a_ptr->to_h = o_ptr->to_h;
	    a_ptr->to_d = o_ptr->to_d;
	    a_ptr->to_a = o_ptr->to_a;
	}
        else
        {
	    a_ptr->to_h = k_ptr->to_h;
	    a_ptr->to_d = k_ptr->to_d;
	    a_ptr->to_a = k_ptr->to_a;
	}
	a_ptr->ac = k_ptr->ac;
	a_ptr->dd = k_ptr->dd;
	a_ptr->ds = k_ptr->ds;
	a_ptr->weight = k_ptr->weight;
	a_ptr->flags1 = k_ptr->flags1;
	a_ptr->flags2 = k_ptr->flags2;
	a_ptr->flags3 = k_ptr->flags3;
	a_ptr->flags3 |= (TR3_IGNORE_ACID | TR3_IGNORE_ELEC |
			   TR3_IGNORE_FIRE | TR3_IGNORE_COLD);
	
	/* Ensure weapons have some bonus to hit & dam */
	if ((a_ptr->tval==TV_DIGGING) ||
	    (a_ptr->tval==TV_HAFTED) ||
	    (a_ptr->tval==TV_POLEARM) ||
	    (a_ptr->tval==TV_SWORD) ||
	    (a_ptr->tval==TV_BOW))
	{
		a_ptr->to_d += 1 + randint0(20);
		a_ptr->to_h += 1 + randint0(20);
	}
	
	/* Ensure armour has some bonus to ac */
	if ((k_ptr->tval==TV_BOOTS) ||
	    (k_ptr->tval==TV_GLOVES) ||
	    (k_ptr->tval==TV_HELM) ||
	    (k_ptr->tval==TV_CROWN) ||
	    (k_ptr->tval==TV_SHIELD) ||
	    (k_ptr->tval==TV_CLOAK) ||
	    (k_ptr->tval==TV_SOFT_ARMOR) ||
	    (k_ptr->tval==TV_DRAG_ARMOR) ||
	    (k_ptr->tval==TV_HARD_ARMOR))
	{
		a_ptr->to_a += 1 + randint0(17);
	}
	
	/* Process ammo */
	if ((a_ptr->tval==TV_SHOT) ||
	    (a_ptr->tval==TV_ARROW) ||
	    (a_ptr->tval==TV_BOLT))
	{
		if (power > 0)
		{
			/* Ammo doesn't get large bonus */
			a_ptr->to_d = 0;
			a_ptr->to_h = 0;
			if (rand_int (2) == 0)
                	{
				a_ptr->to_d += randint1(3);
				if (rand_int (2) == 0) a_ptr->to_d += randint1(3);
			}
			if (rand_int (2) == 0)
                	{
				a_ptr->to_h += randint1(6);
				if (rand_int (2) == 0) a_ptr->to_h += randint1(6);
			}
			if (rand_int (5) == 0) a_ptr->ds += 1;
			else if (rand_int (5) == 0) a_ptr->dd += 1;

			/* Add 1-4 abilities */
			add_ability (a_ptr);
			if (rand_int (2) == 0) add_ability (a_ptr);
			if (rand_int (4) == 0) add_ability (a_ptr);
			if (rand_int (10) == 0) add_ability (a_ptr);
			ap = artifact_power (a_ptr) + RANDART_QUALITY + 15;
		}
		else
		{
			/* Make it really bad */
			a_ptr->to_d = -20 - randint1(30);
			a_ptr->to_h = -20 - randint1(30);
			a_ptr->flags3 |= TR3_LIGHT_CURSE;
			ap = artifact_power(a_ptr);
		}
	}
	
	/* Process cursed items */	
	else if (power == 0)
	{
		/* Add two abilities, then curse it three times. */	
		add_ability (a_ptr);
		add_ability (a_ptr);
		do_curse (a_ptr);
		do_curse (a_ptr);
		do_curse (a_ptr);
		remove_contradictory (a_ptr);
		ap = artifact_power(a_ptr);
	}
	else
	{
		/* Select a random set of abilities which roughly matches the
		   original's in terms of overall power/usefulness. */
		for (tries = 0; tries < MAX_TRIES; tries++)
		{
			artifact_type a_old;

			/* Copy artifact info temporarily. */
			a_old = *a_ptr;
			add_ability (a_ptr);
			ap = artifact_power (a_ptr);
			
			if (ap > (power * 11) / 10 + 1)
			{	/* too powerful -- put it back */
				*a_ptr = a_old;
				continue;
			}

			else if (ap >= (power * 9) / 10)	/* just right */
			{
				break;
			}

			/* Stop if we're going negative, so we don't overload
			   the artifact with great powers to compensate. */
			else if ((ap < 0) && (ap < (-(power * 1)) / 10))
			{
				break;
			}
		} /* end of power selection */

		if (aggravate_me)
		{
			a_ptr->flags3 |= TR3_AGGRAVATE;
			remove_contradictory (a_ptr);
			ap = artifact_power (a_ptr);
		}
	}

	a_ptr->cost = (ap + 10 - RANDART_QUALITY) * (s32b)1500;
	if (a_ptr->cost < 0) a_ptr->cost = 0;

	/* Add TR3_HIDE_TYPE to all artifacts with nonzero pval because we're
	   too lazy to find out which ones need it and which ones don't. */
	if (a_ptr->pval) a_ptr->flags3 |= TR3_HIDE_TYPE;

	/* Ensure a bonus for certain items whose base type always has a pval */
	if ((k_ptr->flags1 & TR1_PVAL_MASK) && !a_ptr->pval)
        {
		a_ptr->pval = randint1(3);
		if (cursed_p(o_ptr)) a_ptr->pval = -a_ptr->pval;
	}

	/* Never more than +11 bonus */
	if (a_ptr->pval > 11) a_ptr->pval = 11;

	/* Weapon dice limited to 2*avg dam = 45 (5d8 or 9d4) */
	while ((a_ptr->dd * (1+a_ptr->ds) > 45) && (a_ptr->dd > k_ptr->dd)) a_ptr->dd -= 1;

	/* Extra weapon dice limited to +3 */
	if (a_ptr->dd > k_ptr->dd + 3) a_ptr->dd = k_ptr->dd + 3;

	/* No more than +4 infra on helms and crowns */
	if ((a_ptr->tval == TV_HELM || a_ptr->tval == TV_CROWN) &&
	    (a_ptr->flags1 & TR1_INFRA) && (a_ptr->pval > 4)) a_ptr->pval = 4;

	/* No more than +6 stealth */
	if ((a_ptr->flags1 & TR1_STEALTH) && (a_ptr->pval > 6)) a_ptr->pval = 6;
	
	/* Never increase stats too greatly */
	if (a_ptr->flags1 & (TR1_STR | TR1_INT | TR1_WIS | TR1_DEX | TR1_CON | TR1_CHR))
	{
		int c = 0; 
		if (a_ptr->flags1 & TR1_STR) c++;
		if (a_ptr->flags1 & TR1_INT) c++;
		if (a_ptr->flags1 & TR1_WIS) c++;
		if (a_ptr->flags1 & TR1_DEX) c++;
		if (a_ptr->flags1 & TR1_CON) c++;
		if (a_ptr->flags1 & TR1_CHR) c++;

		/* no more than +5 on stats */
		if (a_ptr->pval > 5) a_ptr->pval = 5;

		/* if more than 3 stats, limit to +3 */
		if ((c > 3) && (a_ptr->pval > 3)) a_ptr->pval = 3;

		/* if more than 1 stat on an amulet, limit to +3 */
		if ((a_ptr->tval == TV_AMULET) && (c > 1) && (a_ptr->pval > 3)) a_ptr->pval = 3;
	}

	/* Never more than +3 EA */
        if ((a_ptr->flags1 & TR1_BLOWS) && (a_ptr->pval > 3)) a_ptr->pval = 3;

	/* Never more than +6 tohit/todam on gloves, +30 in general */
	if (a_ptr->tval == TV_GLOVES)
	{
		if (a_ptr->to_h > 6) a_ptr->to_h = 6;
		if (a_ptr->to_d > 6) a_ptr->to_d = 6;
	}
	else
	{
		if (a_ptr->to_h > 30) a_ptr->to_h = 30;
		if (a_ptr->to_d > 30) a_ptr->to_d = 30;
	}

	/* Restore RNG */
	Rand_quick = FALSE;

	/* Return a pointer to the artifact_type */	
	return (a_ptr);
}


/*
 * Make random artifact name.
 */
void randart_name(const object_type *o_ptr, char *buffer)
{
	char tmp[80];
	
	/* Set the RNG seed. It this correct. Should it be restored??? XXX */
	Rand_value = o_ptr->name3;
	Rand_quick = TRUE;

	/* Take a random name */
	get_rnd_line("randarts.txt", 0, tmp);

	/* Capitalise first character */
	tmp[0] = toupper(tmp[0]);
	
	/* Either "sword of something" or
	 * "sword 'something'" form */
	if (randint0(2))
	{
		sprintf(buffer, "of %s", tmp);
	}
	else
	{
		sprintf(buffer, "'%s'", tmp);
	}
	
	/* Restore RNG */
	Rand_quick = FALSE;

	return;
}
	
