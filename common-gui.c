#include "rftg.h"

/*
 * Use simple random number generator.
 */
int game_rand(game *g)
{
	/* Call simple random number generator */
	return simple_rand(&g->random_seed);
}

/*
 * Reset a displayed card structure.
 */
void reset_display(displayed *i_ptr)
{
	/* Clear all fields */
	memset(i_ptr, 0, sizeof(displayed));
}

/*
 * Function to compare two cards on the table for sorting.
 */
static int cmp_table(const void *t1, const void *t2)
{
	displayed *i_ptr1 = (displayed *)t1, *i_ptr2 = (displayed *)t2;

	/* Sort by order played */
	return i_ptr1->order - i_ptr2->order;
}

/*
 * Reset our list of cards in hand.
 */
static void reset_hand(game *g, int color)
{
	displayed *i_ptr;
	card *c_ptr;
	int i;

	/* Clear size */
	hand_size = 0;

	/* Loop over cards in deck */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[i];

		/* Skip unowned cards */
		if (c_ptr->owner != player_us) continue;

		/* Skip cards not in hand */
		if (c_ptr->where != WHERE_HAND) continue;

		/* Get next entry in hand list */
		i_ptr = &hand[hand_size++];

		/* Reset structure */
		reset_display(i_ptr);

		/* Add card information */
		i_ptr->index = i;
		i_ptr->d_ptr = c_ptr->d_ptr;

		/* Card is in hand */
		i_ptr->hand = 1;

		/* Set color flag */
		i_ptr->color = color;
	}
}

/*
 * Create a tooltip for a military strength icon.
 */
static char *get_military_tooltip(game *g, int who)
{
	static char msg[1024];
	char text[1024];
	card *c_ptr;
	power *o_ptr;
	int i, j;
	int base, rebel, blue, brown, green, yellow;

	/* Begin with base military strength */
	base = total_military(g, who);

	/* Create text */
	sprintf(msg, "Base strength: %+d", base);

	/* Start other strengths at base */
	rebel = blue = brown = green = yellow = base;

	/* Loop over cards */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[i];

		/* Skip cards not belonging to current player */
		if (c_ptr->owner != who) continue;

		/* Skip inactive cards */
		if (c_ptr->start_where != WHERE_ACTIVE) continue;

		/* Loop over card's powers */
		for (j = 0; j < c_ptr->d_ptr->num_power; j++)
		{
			/* Get power pointer */
			o_ptr = &c_ptr->d_ptr->powers[j];

			/* Skip incorrect phase */
			if (o_ptr->phase != PHASE_SETTLE) continue;

			/* Skip non-military powers */
			if (!(o_ptr->code & P3_EXTRA_MILITARY)) continue;

			/* Check for strength against rebels */
			if (o_ptr->code & P3_AGAINST_REBEL)
				rebel += o_ptr->value;

			/* Check for strength against Novelty worlds */
			if (o_ptr->code & P3_NOVELTY)
				blue += o_ptr->value;

			/* Check for strength against Rare worlds */
			if (o_ptr->code & P3_RARE)
				brown += o_ptr->value;

			/* Check for strength against Gene worlds */
			if (o_ptr->code & P3_GENE)
				green += o_ptr->value;

			/* Check for strength against Alien worlds */
			if (o_ptr->code & P3_ALIEN)
				yellow += o_ptr->value;
		}
	}

	/* Add rebel strength if different than base */
	if (rebel != base)
	{
		/* Create rebel text */
		sprintf(text, "\nAgainst Rebel: %+d", rebel);
		strcat(msg, text);
	}

	/* Add Novelty strength if different than base */
	if (blue != base)
	{
		/* Create text */
		sprintf(text, "\nAgainst Novelty: %+d", blue);
		strcat(msg, text);
	}

	/* Add Rare strength if different than base */
	if (brown != base)
	{
		/* Create text */
		sprintf(text, "\nAgainst Rare: %+d", brown);
		strcat(msg, text);
	}

	/* Add Gene strength if different than base */
	if (green != base)
	{
		/* Create text */
		sprintf(text, "\nAgainst Gene: %+d", green);
		strcat(msg, text);
	}

	/* Add Alien strength if different than base */
	if (yellow != base)
	{
		/* Create text */
		sprintf(text, "\nAgainst Alien: %+d", yellow);
		strcat(msg, text);
	}

	/* Check for active Imperium card */
	if (count_active_flags(g, who, FLAG_IMPERIUM))
	{
		/* Add vulnerability text */
		strcat(msg, "\nIMPERIUM card played");
	}

	/* Check for active Rebel military world */
	if (count_active_flags(g, who, FLAG_MILITARY | FLAG_REBEL))
	{
		/* Add vulnerability text */
		strcat(msg, "\nREBEL Military world played");
	}

	/* Return tooltip text */
	return msg;
}

/*
 * Create a tooltip for a displayed card.
 */
static char *display_card_tooltip(game *g, int who, int which)
{
	char text[1024];
	card *c_ptr, *b_ptr;
	int i, count = 0;

	/* Get card pointer */
	c_ptr = &g->deck[which];

	/* Check for nothing to display */
	if (!(c_ptr->d_ptr->flags & FLAG_START_SAVE)) return NULL;

	/* Loop over cards in deck */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		b_ptr = &g->deck[i];

		/* Count cards saved */
		if (b_ptr->where == WHERE_SAVED) count++;
	}

	/* Start tooltip text */
	sprintf(text, "%d card%s saved", count, (count == 1) ? "" : "s");

	/* Check for card not owned by player showing */
	if (count == 0 || c_ptr->owner != who) return strdup(text);

	/* Add list of cards saved */
	strcat(text, ":\n");

	/* Loop over cards in deck */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		b_ptr = &g->deck[i];

		/* Skip cards that aren't saved */
		if (b_ptr->where != WHERE_SAVED) continue;

		/* Add card name to tooltip */
		strcat(text, "\n\t");
		strcat(text, b_ptr->d_ptr->name);
	}

	/* Return tooltip */
	return strdup(text);
}

/*
 * Create a tooltip for a goal image.
 */
char *goal_tooltip(game *g, int goal)
{
	static char msg[1024];
	player *p_ptr;
	int i;
	char text[1024];

	/* Create tooltip text */
	sprintf(msg, "%s", goal_name[goal]);

	/* Check for first goal */
	if (goal <= GOAL_FIRST_4_MILITARY)
	{
		/* Check for claimed goal */
		if (!g->goal_avail[goal])
		{
			/* Add text to tooltip */
			strcat(msg, "\nClaimed by:");

			/* Loop over players */
			for (i = 0; i < g->num_players; i++)
			{
				/* Get player pointer */
				p_ptr = &g->p[i];

				/* Check for claim */
				if (p_ptr->goal_claimed[goal])
				{
					/* Add name to tooltip */
					strcat(msg, "\n  ");
					strcat(msg, p_ptr->name);
				}
			}
		}
	}

	/* Check for most goal */
	if (goal >= GOAL_MOST_MILITARY)
	{
		/* Add text to tooltip */
		strcat(msg, "\nProgress:");

		/* Loop over players */
		for (i = 0; i < g->num_players; i++)
		{
			/* Get player pointer */
			p_ptr = &g->p[i];

			/* Create progress string */
			sprintf(text, "\n%c %s: %d",
				p_ptr->goal_claimed[goal] ? '*' : ' ',
				p_ptr->name, p_ptr->goal_progress[goal]);

			/* Add progress string to tooltip */
			strcat(msg, text);
		}
	}

	/* Return tooltip text */
	return msg;
}

/*
 * Reset status information for a player.
 */
static void reset_status(game *g, int who)
{
	int i;

	/* Copy player's name */
	strcpy(status_player[who].name, g->p[who].name);

	/* Check for actions known */
	if (g->cur_action >= ACT_SEARCH ||
	    count_active_flags(g, player_us, FLAG_SELECT_LAST))
	{
		/* Copy actions */
		status_player[who].action[0] = g->p[who].action[0];
		status_player[who].action[1] = g->p[who].action[1];
	}
	else
	{
		/* Actions aren't known */
		status_player[who].action[0] = ICON_NO_ACT;
		status_player[who].action[1] = ICON_NO_ACT;
	}

	/* Copy VP chips */
	status_player[who].vp = g->p[who].vp;
	status_player[who].end_vp = g->p[who].end_vp;

	/* Count cards in hand */
	status_player[who].cards_hand = count_player_area(g, who, WHERE_HAND);

	/* Copy prestige */
	status_player[who].prestige = g->p[who].prestige;

	/* Count military strength */
	status_player[who].military = total_military(g, who);

	/* Get text of military tooltip */
	strcpy(status_player[who].military_tip, get_military_tooltip(g, who));

	/* Loop over goals */
	for (i = 0; i < MAX_GOAL; i++)
	{
		/* Assume goal is not displayed */
		status_player[who].goal_display[i] = 0;

		/* Assume goal is not grayed */
		status_player[who].goal_gray[i] = 0;

		/* Skip inactive goals */
		if (!g->goal_active[i]) continue;

		/* Check for "first" goal */
		if (i <= GOAL_FIRST_4_MILITARY)
		{
			/* Check for unclaimed */
			if (!g->p[who].goal_claimed[i]) continue;
		}
		else
		{
			/* Check for insufficient progress */
			if (g->p[who].goal_progress[i] < goal_minimum(i))
				continue;

			/* Check for less progress than other players */
			if (g->p[who].goal_progress[i] < g->goal_most[i])
				continue;

			/* Unclaimed goals should be gray */
			if (!g->p[who].goal_claimed[i])
				status_player[who].goal_gray[i] = 1;
		}

		/* Goal should be displayed */
		status_player[who].goal_display[i] = 1;

		/* Get text of goal tooltip */
		strcpy(status_player[who].goal_tip[i], goal_tooltip(g, i));
	}
}

/*
 * Reset list of displayed cards on the table for the given player.
 */
static void reset_table(game *g, int who, int color)
{
	displayed *i_ptr;
	card *c_ptr;
	int i;

	/* Clear size */
	table_size[who] = 0;

	/* Loop over cards in deck */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[i];

		/* Skip unowned cards */
		if (c_ptr->owner != who) continue;

		/* Skip cards not on table */
		if (c_ptr->where != WHERE_ACTIVE) continue;

		/* Get next entry in table list */
		i_ptr = &table[who][table_size[who]++];

		/* Reset structure */
		reset_display(i_ptr);

		/* Add card information */
		i_ptr->index = i;
		i_ptr->d_ptr = c_ptr->d_ptr;

		/* Set color flag */
		i_ptr->color = color;

		/* Check for good */
		i_ptr->covered = (c_ptr->covered != -1);

		/* Copy order played */
		i_ptr->order = c_ptr->order;

		/* Get tooltip */
		i_ptr->tooltip = display_card_tooltip(g, player_us, i);
	}

	/* Sort list */
	qsort(table[who], table_size[who], sizeof(displayed), cmp_table);
}

/*
 * Reset our hand list, and all players' table lists.
 */
void reset_cards(game *g, int color_hand, int color_table)
{
	card *c_ptr;
	int i;

	/* Score game */
	score_game(g);

	/* Reset hand */
	reset_hand(g, color_hand);

	/* Loop over players */
	for (i = 0; i < real_game.num_players; i++)
	{
		/* Reset table of player */
		reset_table(g, i, color_table);

		/* Reset status information for player */
		reset_status(g, i);
	}

	/* Clear displayed status info */
	display_deck = 0;
	display_discard = 0;

	/* Loop over cards */
	for (i = 0; i < g->deck_size; i++)
	{
		/* Get card pointer */
		c_ptr = &g->deck[i];

		/* Check for card in draw pile */
		if (c_ptr->where == WHERE_DECK) display_deck++;

		/* Check for card in discard pile */
		if (c_ptr->where == WHERE_DISCARD) display_discard++;
	}

	/* Get chips in VP pool */
	display_pool = g->vp_pool;

	/* Do not display negative numbers */
	if (display_pool < 0) display_pool = 0;
}

/*
 * Return whether the selected world and hand is a valid start.
 */
int action_check_start(void)
{
	game sim;
	displayed *i_ptr;
	int i, n = 0, ns = 0;
	int list[MAX_DECK], special[MAX_DECK];

	/* Loop over cards in hand */
	for (i = 0; i < hand_size; i++)
	{
		/* Get hand pointer */
		i_ptr = &hand[i];

		/* Skip unselected */
		if (!i_ptr->selected) continue;

		/* Add to regular list */
		list[n++] = i_ptr->index;
	}

	/* Loop over cards on table */
	for (i = 0; i < table_size[player_us]; i++)
	{
		/* Get table card pointer */
		i_ptr = &table[player_us][i];

		/* Skip unselected */
		if (!i_ptr->selected) continue;

		/* Add to special list */
		special[ns++] = i_ptr->index;
	}

	/* Check for exactly 1 world selected */
	if (ns != 1) return 0;

	/* Copy game */
	sim = real_game;

	/* Set simulation flag */
	sim.simulation = 1;

	/* Try to start */
	return start_callback(&sim, player_us, list, n, special, ns);
}

/*
 * Function to compare two cards in the hand for sorting.
 */
int cmp_hand(const void *h1, const void *h2)
{
	displayed *i_ptr1 = (displayed *)h1, *i_ptr2 = (displayed *)h2;
	card *c_ptr1, *c_ptr2;

	/* Get card pointers */
	c_ptr1 = &real_game.deck[i_ptr1->index];
	c_ptr2 = &real_game.deck[i_ptr2->index];

	/* Gapped cards always go after non-gapped */
	if (i_ptr1->gapped && !i_ptr2->gapped) return 1;
	if (!i_ptr1->gapped && i_ptr2->gapped) return -1;

	/* Worlds come before developments */
	if (c_ptr1->d_ptr->type != c_ptr2->d_ptr->type)
	{
		/* Check for development */
		if (c_ptr1->d_ptr->type == TYPE_DEVELOPMENT) return 1;
		if (c_ptr2->d_ptr->type == TYPE_DEVELOPMENT) return -1;
	}

	/* Sort by cost */
	if (c_ptr1->d_ptr->cost != c_ptr2->d_ptr->cost)
	{
		/* Return cost difference */
		return c_ptr1->d_ptr->cost - c_ptr2->d_ptr->cost;
	}

	/* Otherwise sort by index */
	return i_ptr1->index - i_ptr2->index;
}

/*
 * Redraw everything.
 */
void redraw_everything(void)
{
	/* Redraw hand, table, and status area */
	redraw_status();
	redraw_hand();
	redraw_table();
	redraw_goal();
	redraw_phase();
}

/*
 * Return a "score" for sorting consume powers.
 */
static int score_consume(power *o_ptr)
{
	int vp = 0, card = 0, prestige = 0, goods = 1;
	int score;

	/* Always discard from hand last */
	if (o_ptr->code & P4_DISCARD_HAND) return 0;

	/* Check for free card draw */
	if (o_ptr->code & P4_DRAW) return o_ptr->value * 500;

	/* Check for free VP */
	if (o_ptr->code & P4_VP) return o_ptr->value * 1000;

	/* Check for VP awarded */
	if (o_ptr->code & P4_GET_VP) vp += o_ptr->value;

	/* Check for card awarded */
	if (o_ptr->code & P4_GET_CARD) card += o_ptr->value;

	/* Check for cards awarded */
	if (o_ptr->code & P4_GET_2_CARD) card += o_ptr->value * 2;

	/* Check for prestige awarded */
	if (o_ptr->code & P4_GET_PRESTIGE) prestige += o_ptr->value;

	/* Assume trade will earn 4 cards */
	if (o_ptr->code & P4_TRADE_ACTION) card += 4;

	/* Assume trade without bonus will earn fewer cards */
	if (o_ptr->code & P4_TRADE_NO_BONUS) card--;

	/* Check for consuming two goods */
	if (o_ptr->code & P4_CONSUME_TWO) goods = 2;

	/* Check for consuming three goods */
	if (o_ptr->code & P4_CONSUME_3_DIFF) goods = 3;

	/* Check for consuming all goods */
	if (o_ptr->code & P4_CONSUME_ALL) goods = 4;

	/* Check for "Consume x2" chosen */
	if (player_chose(&real_game, player_us, ACT_CONSUME_X2)) vp *= 2;

	/* Compute score */
	score = (prestige * 150 + vp * 100 + card * 50) / goods;

	/* Use specific consume powers first */
	if (!(o_ptr->code & P4_CONSUME_ANY)) score++;

	/* Use multi-use powers later */
	if (o_ptr->times > 1) score -= 5 * o_ptr->times;

	/* Return score */
	return score;
}

/*
 * Compare two consume powers for sorting.
 */
int cmp_consume(const void *l1, const void *l2)
{
	pow_loc *l_ptr1 = (pow_loc *)l1;
	pow_loc *l_ptr2 = (pow_loc *)l2;
	power *o_ptr1;
	power *o_ptr2;

	/* Check for prestige trade bonus */
	if (l_ptr1->c_idx < 0) return 1;
	if (l_ptr2->c_idx < 0) return -1;

	/* Get first power */
	o_ptr1 = &real_game.deck[l_ptr1->c_idx].d_ptr->powers[l_ptr1->o_idx];
	o_ptr2 = &real_game.deck[l_ptr2->c_idx].d_ptr->powers[l_ptr2->o_idx];

	/* Compare consume powers */
	return score_consume(o_ptr2) - score_consume(o_ptr1);
}

/*
 * Make a choice of the given type.
 */
void gui_make_choice(game *g, int who, int type, int list[], int *nl,
                           int special[], int *ns, int arg1, int arg2, int arg3)
{
	player *p_ptr;
	int i, rv;
	int *l_ptr;

	/* Determine type of choice */
	switch (type)
	{
		/* Action(s) to play */
		case CHOICE_ACTION:

			/* Choose actions */
			gui_choose_action(g, who, list, arg1);
			rv = 0;
			break;

		/* Start world */
		case CHOICE_START:

			/* Choose start world */
			gui_choose_start(g, who, list, nl, special, ns);
			rv = 0;
			break;

		/* Discard */
		case CHOICE_DISCARD:

			/* Choose discards */
			gui_choose_discard(g, who, list, nl, arg1);
			rv = 0;
			break;

		/* Save a card under a world for later */
		case CHOICE_SAVE:

			/* Choose card to save */
			gui_choose_save(g, who, list, nl);
			rv = 0;
			break;

		/* Choose to discard to gain prestige */
		case CHOICE_DISCARD_PRESTIGE:

			/* Choose card (if any) to discard */
			gui_choose_discard_prestige(g, who, list, nl);
			rv = 0;
			break;

		/* Place a development/world */
		case CHOICE_PLACE:

			/* Choose card to place */
			rv = gui_choose_place(g, who, list, *nl, arg1, arg2);
			break;

		/* Pay for a development/world */
		case CHOICE_PAYMENT:

			/* Choose payment */
			gui_choose_pay(g, who, arg1, list, nl, special, ns,
			               arg2);
			rv = 0;
			break;

		/* Choose a world to takeover */
		case CHOICE_TAKEOVER:

			/* Choose takeover target/power */
			rv = gui_choose_takeover(g, who, list, nl, special, ns);
			break;

		/* Choose a method of defense against a takeover */
		case CHOICE_DEFEND:

			/* Choose defense method */
			gui_choose_defend(g, who, arg1, arg2, arg3, list, nl,
			                  special, ns);
			rv = 0;
			break;

		/* Choose whether to prevent a takeover */
		case CHOICE_TAKEOVER_PREVENT:

			/* Choose takeover to prevent */
			gui_choose_takeover_prevent(g, who, list, nl,
			                            special, ns);
			rv = 0;
			break;

		/* Choose world to upgrade with one from hand */
		case CHOICE_UPGRADE:

			/* Choose upgrade */
			gui_choose_upgrade(g, who, list, nl, special, ns);
			rv = 0;
			break;

		/* Choose a good to trade */
		case CHOICE_TRADE:

			/* Choose good */
			gui_choose_trade(g, who, list, nl, arg1);
			rv = 0;
			break;

		/* Choose a consume power to use */
		case CHOICE_CONSUME:

			/* Choose power */
			gui_choose_consume(g, who, list, special, nl, ns, arg1);
			rv = 0;
			break;

		/* Choose discards from hand for VP */
		case CHOICE_CONSUME_HAND:

			/* Choose cards */
			gui_choose_consume_hand(g, who, arg1, arg2, list, nl);
			rv = 0;
			break;

		/* Choose good(s) to consume */
		case CHOICE_GOOD:

			/* Choose good(s) */
			gui_choose_good(g, who, special[0], special[1],
			                list, nl, arg1, arg2);
			rv = 0;
			break;

		/* Choose lucky number */
		case CHOICE_LUCKY:

			/* Choose number */
			rv = gui_choose_lucky(g, who);
			break;

		/* Choose card to ante */
		case CHOICE_ANTE:

			/* Choose card */
			rv = gui_choose_ante(g, who, list, *nl);
			break;

		/* Choose card to keep in successful gamble */
		case CHOICE_KEEP:

			/* Choose card */
			rv = gui_choose_keep(g, who, list, *nl);
			break;

		/* Choose windfall world to produce on */
		case CHOICE_WINDFALL:

			/* Choose world */
			gui_choose_windfall(g, who, list, nl);
			rv = 0;
			break;

		/* Choose produce power to use */
		case CHOICE_PRODUCE:

			/* Choose power */
			gui_choose_produce(g, who, list, special, *nl);
			rv = 0;
			break;

		/* Choose card to discard in order to produce */
		case CHOICE_DISCARD_PRODUCE:

			/* Choose card */
			gui_choose_discard_produce(g, who, list, nl,
			                           special, ns);
			rv = 0;
			break;

		/* Choose search category */
		case CHOICE_SEARCH_TYPE:

			/* Choose search type */
			rv = gui_choose_search_type(g, who);
			break;

		/* Choose whether to keep searched card */
		case CHOICE_SEARCH_KEEP:

			/* Choose to keep */
			rv = gui_choose_search_keep(g, who, arg1, arg2);
			break;

		/* Choose color of Alien Oort Cloud Refinery */
		case CHOICE_OORT_KIND:

			/* Choose type */
			rv = gui_choose_oort_kind(g, who);
			break;

		/* Error */
		default:
			printf("Unknown choice type!\n");
			exit(1);
	}

	/* Check for aborted game */
	if (g->game_over) return;

	/* Save undo positions */
	for (i = 0; i < g->num_players; i++)
	{
		/* Save log size history */
		g->p[i].choice_history[num_undo] = g->p[i].choice_size;
	}

	/* Add one undo position to list */
	num_undo++;

	/* Get player pointer */
	p_ptr = &g->p[who];

	/* Get pointer to end of choice log */
	l_ptr = &p_ptr->choice_log[p_ptr->choice_size];

	/* Add choice type to log */
	*l_ptr++ = type;

	/* Add return value to log */
	*l_ptr++ = rv;

	/* Check for number of list items available */
	if (nl)
	{
		/* Add number of returned list items */
		*l_ptr++ = *nl;

		/* Copy list items */
		for (i = 0; i < *nl; i++)
		{
			/* Copy list item */
			*l_ptr++ = list[i];
		}
	}
	else
	{
		/* Add no list items */
		*l_ptr++ = 0;
	}

	/* Check for number of special items available */
	if (ns)
	{
		/* Add number of returned special items */
		*l_ptr++ = *ns;

		/* Copy special items */
		for (i = 0; i < *ns; i++)
		{
			/* Copy special item */
			*l_ptr++ = special[i];
		}
	}
	else
	{
		/* Add no special items */
		*l_ptr++ = 0;
	}

	/* Mark new size of choice log */
	p_ptr->choice_size = l_ptr - p_ptr->choice_log;
}

/*
 * Apply options to game structure.
 */
void apply_options(void)
{
	/* Sanity check number of players in base game */
	if (opt.expanded < 1 && opt.num_players > 4)
	{
		/* Reset to four players */
		opt.num_players = 4;
	}

	/* Sanity check number of players in first expansion */
	if (opt.expanded < 2 && opt.num_players > 5)
	{
		/* Reset to five players */
		opt.num_players = 5;
	}

	/* Set number of players */
	real_game.num_players = opt.num_players;

	/* Set expansion level */
	real_game.expanded = opt.expanded;

	/* Set advanced flag */
	real_game.advanced = opt.advanced;

	/* Set goals disabled */
	real_game.goal_disabled = opt.disable_goal;

	/* Set takeover disabled */
	real_game.takeover_disabled = opt.disable_takeover;

	/* Sanity check advanced mode */
	if (real_game.num_players > 2)
	{
		/* Clear advanced mode */
		real_game.advanced = 0;
	}
}

/*
 * Attempt to place a moved card into the proper display table.
 */
void debug_card_moved(int c, int old_owner, int old_where)
{
	card *c_ptr;
	displayed *i_ptr;
	int i;

	/* Get card pointer */
	c_ptr = &real_game.deck[c];

	/* Check for moving from our hand */
	if (old_owner == player_us && old_where == WHERE_HAND)
	{
		/* Loop over our hand */
		for (i = 0; i < hand_size; i++)
		{
			/* Get displayed card pointer */
			i_ptr = &hand[i];

			/* Check for match */
			if (i_ptr->index == c)
			{
				/* Remove from hand */
				hand[i] = hand[--hand_size];

				/* Done */
				break;
			}
		}
	}

	/* Check for moving from active area */
	if (old_where == WHERE_ACTIVE)
	{
		/* Loop over table area */
		for (i = 0; i < table_size[old_owner]; i++)
		{
			/* Get displayed card pointer */
			i_ptr = &table[old_owner][i];

			/* Check for match */
			if (i_ptr->index == c)
			{
				/* Remove from table */
				table[old_owner][i] =
				      table[old_owner][--table_size[old_owner]];

				/* Done */
				break;
			}
		}
	}

	/* Check for adding card to our hand */
	if (c_ptr->owner == player_us && c_ptr->where == WHERE_HAND)
	{
		/* Add card to hand */
		i_ptr = &hand[hand_size++];

		/* Set design and index */
		i_ptr->d_ptr = c_ptr->d_ptr;
		i_ptr->index = c;

		/* Clear all other fields */
		i_ptr->eligible = i_ptr->gapped = 0;
		i_ptr->selected = i_ptr->color = 0;
		i_ptr->covered = 0;
	}

	/* Check for adding card to active area */
	if (c_ptr->owner != -1 && c_ptr->where == WHERE_ACTIVE)
	{
		/* Add card to hand */
		i_ptr = &table[c_ptr->owner][table_size[c_ptr->owner]++];

		/* Set design and index */
		i_ptr->d_ptr = c_ptr->d_ptr;
		i_ptr->index = c;

		/* Clear all other fields */
		i_ptr->eligible = i_ptr->gapped = 0;
		i_ptr->selected = i_ptr->color = 0;
		i_ptr->covered = 0;
	}

	/* Redraw */
	redraw_everything();
}


