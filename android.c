#define LOG_TAG "rftgJNI"
#include "Log.h"
#include "Native.h"
#include "rftg.h"

int display_deck, display_discard, display_pool;
int hand_size, player_us;
int action_max, action_min, action_restrict;
int action_cidx, action_oidx;
int action_payment_which, action_payment_mil;
int *orig_log[MAX_PLAYER];
int *orig_history[MAX_PLAYER];
int restart_loop, num_undo;
displayed hand[MAX_DECK];
options opt;
decisions ai_func;
decisions gui_func;
int message_last_y;

static JavaVM *cached_jvm;
static jclass Class_C;
static jmethodID MID_createComboBox;
static jmethodID MID_messageAdd;
static jmethodID MID_restoreStatusArea;
static jmethodID MID_restoreTableArea;
static jmethodID MID_fullImage;
static jmethodID MID_drawCard;
static jmethodID MID_clearDrawCard;

void redraw_status_area(int who) {
}

void redraw_status(void) {
	char buf[1024];
	int i;

	/* Loop over players */
	for (i = 0; i < real_game.num_players; i++) {
		/* Redraw player's status */
		//redraw_status_area(i, player_status[i]);
		redraw_status_area(i);
	}

	/* Create status label */
	sprintf(buf, "Draw: %d  Discard: %d  Pool: %d", display_deck,
		display_discard, display_pool);
	
	/* Set label */
	//gtk_label_set_text(GTK_LABEL(game_status), buf);
	label_set_text(buf);
}

void redraw_phase(void) {
	int i;
	char buf[1024], *name;

	/* Loop over actions */
	for (i = ACT_EXPLORE_5_0; i <= ACT_PRODUCE; i++) {
		/* Skip second explore/consume actions */
		if (i == ACT_EXPLORE_1_1 || i == ACT_CONSUME_X2) continue;

		/* Get phase name */
		switch (i) {
		  case ACT_EXPLORE_5_0: name = "Explore"; break;
		  case ACT_DEVELOP:
		  case ACT_DEVELOP2: name = "Develop"; break;
		  case ACT_SETTLE:
		  case ACT_SETTLE2: name = "Settle"; break;
		  case ACT_CONSUME_TRADE: name = "Consume"; break;
		  case ACT_PRODUCE: name = "Produce"; break;
		  default: name = ""; break;
		}

		/* Check for basic game and advanced actions */
		if (!real_game.advanced &&
		    (i == ACT_DEVELOP2 || i == ACT_SETTLE2)) {
			/* Simply hide label */
			//gtk_widget_hide(phase_labels[i]);

			/* Next label */
			continue;
		}
			
		/* Check for inactive phase */
		else if (!real_game.action_selected[i]) {
			/* Strikeout name */
			sprintf(buf, "<s>%s</s>", name);
		}

		/* Check for current phase */
		else if (real_game.cur_action == i) {
			/* Bold name */
			sprintf(buf,
				"<span foreground=\"blue\" weight=\"bold\">%s</span>",
				name);
		}

		/* Normal phase */
		else {
			/* Normal name */
			sprintf(buf, "%s", name);
		}

		/* Set label text */
		//gtk_label_set_markup(GTK_LABEL(phase_labels[i]), buf);

		/* Show label */
		//gtk_widget_show(phase_labels[i]);
	}
}

void redraw_hand(void) {
	//GtkWidget *box;
	displayed *i_ptr;
	int count = 0, gap = 1, n, num_gap = 0;
	int key_count = 1, key_code;
	int width, height, highlight;
	int card_w, card_h;
	int i, j;

	/* Sort hand */
	qsort(hand, hand_size, sizeof(displayed), cmp_hand);

	/* First destroy all pre-existing card widgets */
	//gtk_container_foreach(GTK_CONTAINER(hand_area), destroy_widget, NULL);
	JNIEnv *env;
	(*cached_jvm)->GetEnv(cached_jvm, (void **) &env, JNI_VERSION_1_6);
	(*env)->CallStaticVoidMethod(env, Class_C, MID_clearDrawCard, NULL);
	
	/* Get number of cards in hand */
	n = hand_size;
	
	/* Loop over cards in hand */
	for (i = 0; i < n; i++) {
		/* Get card pointer */
		i_ptr = &hand[i];
		
		/* Check for "gapped" card */
		if (i_ptr->gapped) {
			/* Count cards marked for gap */
			num_gap++;
		}
	}
	
	/* Check for some but not all cards needing gap */
	if (num_gap > 0 && num_gap < n) {
		/* Add extra space for gap */
		n++;
	} else {
		/* Mark gap as not needed */
		gap = 0;
	}
	
	/* Get hand area width and height */
	//width = hand_area->allocation.width;
	//height = hand_area->allocation.height;
	width = CARD_WIDTH;
	height = CARD_HEIGHT;
	
	/* Get width of individual card */
	card_w = width / 6;
	
	/* Compute height of card */
	card_h = card_w * CARD_HEIGHT / CARD_WIDTH;
	
	/* Compute pixels per card */
	if (n > 0) width = width / n;
	
	/* Maximum width */
	if (width > card_w) width = card_w;
	
	/* Loop over cards */
	for (i = 0; i < hand_size; i++) {
		/* Get card pointer */
		i_ptr = &hand[i];
		
		/* Skip spot before first gap card */
		if (i_ptr->gapped && gap) {
			/* Increase count */
			count++;
			
			/* Gap no longer needed */
			gap = 0;
		}
		
		/* Assume no highlighting */
		highlight = HIGH_NONE;
		
		/* Check for selected */
		if (i_ptr->selected) {
			/* Use given highlight color */
			highlight = i_ptr->highlight;
		} else {
			/* Check for other card selected */
			for (j = 0; j < hand_size; j++) {
				/* Skip current card */
			        if (i == j) continue;
				
			        /* Check for selected */
				if (hand[j].selected) {
					
					/* Use alternate highlight color */
				        highlight = i_ptr->highlight_else;
				}
			}
		}
		
		/* Get event box with image */
		//box = new_image_box(i_ptr->d_ptr, card_w, card_h,
		//      i_ptr->eligible || i_ptr->color, highlight, 0);
		
		/* Place event box */
		//gtk_fixed_put(GTK_FIXED(hand_area), box, count * width,
		//            i_ptr->selected && i_ptr->push ? 0 : height - card_h);
		
		/* Check for eligible card */
		if (i_ptr->eligible) {
			/* Connect "button released" signal */
			//g_signal_connect(G_OBJECT(box), "button-release-event",
			//                G_CALLBACK(card_selected), i_ptr);
			
			/* Check for enough keyboard numbers */
			if (key_count < 10) {
				/* Compute keyboard code */
				if (key_count <= 9) {
					/* Start with '1' */
					//key_code = GDK_1 + key_count - 1;
				} else {
					/* Use '0' for 10 */
					//key_code = GDK_0;
				}
				
				/* Add handler for keypresses */
				//gtk_widget_add_accelerator(box, "key-signal",
				//                   	   window_accel,
				//                       key_code,
				//                       0, 0);
				
				/* Connect key-signal */
				//g_signal_connect(G_OBJECT(box), "key-signal",
				//                 G_CALLBACK(card_keyed),
				//                 i_ptr);
				
				/* Increment count */
				key_count++;
			}
		}
		
		/* Add tooltip if available */
		if (i_ptr->tooltip) {
			/* Add tooltip to widget */
			//gtk_widget_set_tooltip_text(box, i_ptr->tooltip);
			
			/* Free copy of string */
			free(i_ptr->tooltip);
			i_ptr->tooltip = NULL;
		}
		
		/* Show image */
		//gtk_widget_show_all(box);
		//bob
		(*env)->CallStaticVoidMethod(env, Class_C, MID_drawCard, i_ptr->index, count);

		/* Count images shown */
		count++;
	}
}

void redraw_table_area(int who) {
	//GtkWidget *box, *good_box;
	displayed *i_ptr;
	int x = 0, y = 0;
	int col, row;
	int width, height, highlight;
	int card_w, card_h;
	int i, j, n;

	/* First destroy all pre-existing card widgets */
	//gtk_container_foreach(GTK_CONTAINER(area), destroy_widget, NULL);

	/* Number of cards to display */
	n = table_size[who];

	/* Always have room for 12 cards */
	if (n < 12) n = 12;

	/* Get hand area width and height */
	//width = area->allocation.width;
	//height = area->allocation.height;
	width = CARD_WIDTH;
	height = CARD_HEIGHT;

	/* Check for wide area */
	if (width > height) {
		/* Six columns */
		col = 6;
	}

	/* Check for narrow area */
	else if (height > (3 * width) / 2) {
		/* Three columns */
		col = 3;
	} else {
        /* Four columns */
		col = 4;
	}

	/* Compute number of rows needed */
	row = (n + col - 1) / col;

	/* Get width of individual card */
	card_w = width / col;

	/* Compute height of card */
	card_h = card_w * CARD_HEIGHT / CARD_WIDTH;

	/* Height of row */
	height = height / row;

	/* Width is card width */
	width = card_w;

	/* Loop over cards */
	for (i = 0; i < table_size[who]; i++) {
		/* Get displayed card pointer */
		i_ptr = &table[who][i];

		/* Assume no highlighting */
		highlight = HIGH_NONE;

		/* Check for selected */
		if (i_ptr->selected) {
			/* Use given highlight color */
			highlight = i_ptr->highlight;
		} else {
			/* Check for other card selected */
			for (j = 0; j < table_size[who]; j++) {
				/* Skip current card */
				if (i == j) continue;

				/* Check for selected */
				if (table[who][j].selected) {
					/* Use alternate highlight color */
					highlight = i_ptr->highlight_else;
				}
			}
		}

		/* Get event box with image */
		//box = new_image_box(i_ptr->d_ptr, card_w, card_h,
		//		i_ptr->eligible || i_ptr->color,
		//		highlight, 0);

		/* Place event box */
		//gtk_fixed_put(GTK_FIXED(area), box, x * width, y * height);

		/* Show image */
		//gtk_widget_show_all(box);

		/* Add tooltip if available */
		if (i_ptr->tooltip) {
			/* Add tooltip to widget */
			//gtk_widget_set_tooltip_text(box, i_ptr->tooltip);

			/* Free copy of string */
			free(i_ptr->tooltip);
			i_ptr->tooltip = NULL;
		}

		/* Check for good */
		if (i_ptr->covered || (i_ptr->selected && i_ptr->push)) {
			/* Get event box with no image */
			//good_box = new_image_box(i_ptr->d_ptr, 3 * card_w / 4,
			//		3 * card_h / 4,
			//		i_ptr->eligible ||
			//		i_ptr->color, 0, 1);

			/* Place box on card */
			//gtk_fixed_put(GTK_FIXED(area), good_box,
			//	x * width + card_w / 4,
			//	i_ptr->selected && i_ptr->push ?
			//	y * height :
			//	y * height + card_h / 4);

			/* Show image */
			//gtk_widget_show_all(good_box);

			/* Check for eligible card */
			if (i_ptr->eligible) {
				/* Connect "button released" signal */
				//g_signal_connect(G_OBJECT(good_box),
				//	"button-release-event",
				//	G_CALLBACK(card_selected),
				//	i_ptr);
			}
		}

		/* Check for eligible card */
		if (i_ptr->eligible) {
			/* Connect "button released" signal */
			//g_signal_connect(G_OBJECT(box), "button-release-event",
			//G_CALLBACK(card_selected), i_ptr);
		}

		/* Next slot */
		x++;

		/* Check for next row */
		if (x == col) {
			/* Go to next row */
			x = 0; y++;
		}
	}
}


void redraw_table(void) {
	int i;

	/* Loop over players */
	for (i = 0; i < real_game.num_players; i++) {
		/* Redraw player area */
		// redraw_table_area(i, player_area[i]);
		redraw_table_area(i);
	}
}

void redraw_goal(void) {
 	//GtkWidget *image;
	//GdkPixbuf *buf;
	int i;
	//int n;
	int width, height, goal_h, y = 0;

	/* First destroy all pre-existing goal widgets */
	//gtk_container_foreach(GTK_CONTAINER(goal_area), destroy_widget, NULL);

	/* Assume six goals */
	//n = 6;

	/* Get goal area width and height */
	//width = goal_area->allocation.width;
	//height = goal_area->allocation.height;
	width = GOALF_WIDTH;
	height = GOALF_HEIGHT;

	/* Loop over goals */
	for (i = 0; i < MAX_GOAL; i++) {
		/* Skip inactive goals */
		if (!real_game.goal_active[i]) continue;

		/* Check for "first" goal */
		if (i <= GOAL_FIRST_4_MILITARY) {
			/* Compute height of "first" goal */
			goal_h = width * GOALF_HEIGHT / GOALF_WIDTH;
		} else {
			/* Compute height of "most" goal */
			goal_h = width * GOALM_HEIGHT / GOALM_WIDTH;
		}

		/* Create goal image */
		//buf = gdk_pixbuf_scale_simple(goal_cache[i], width, goal_h,
        	//	GDK_INTERP_BILINEAR);

		/* Check for unavailable goal */
		if (!real_game.goal_avail[i]) {
			/* Desaturate */
			//gdk_pixbuf_saturate_and_pixelate(buf, buf, 0, TRUE);
		}

		/* Make image widget */
		//image = gtk_image_new_from_pixbuf(buf);

		/* Destroy local copy of the pixbuf */
		//g_object_unref(G_OBJECT(buf));
		/* Place image */
		//gtk_fixed_put(GTK_FIXED(goal_area), image, 0, y * height / 100);

		/* Add tooltip */
		//gtk_widget_set_tooltip_text(image, goal_tooltip(&real_game, i));

		/* Show image */
		//gtk_widget_show(image);

		/* Adjust distance to next card */
		if (i <= GOAL_FIRST_4_MILITARY) {
			/* Give "first" goals 15% */
			y += 15;
		} else {
			/* Give "most" goals 20% */
			y += 20;
		}
	}
}

int gui_choose_search_type(game *g, int who) {
    //GtkWidget *combo;
    char buf[1024];
    int i;

    /* Activate action button */
    //gtk_widget_set_sensitive(action_button, TRUE);

    /* Reset displayed cards */
    reset_cards(g, true, true);

    /* Redraw everything */
    redraw_everything();

    /* Set prompt */
    //gtk_label_set_text(GTK_LABEL(action_prompt), "Choose Search category");
    label_set_text("Choose Search category");

    /* Create simple combo box */
    //combo = gtk_combo_box_new_text();

    /* Loop over search categories */
    for (i = 0; i < MAX_SEARCH; i++) {
        /* Skip takeover category if disabled */
        if (real_game.takeover_disabled && i == SEARCH_TAKEOVER)
            continue;

            /* Copy search name */
            strcpy(buf, search_name[i]);

            /* Capitalize search name */
            buf[0] = toupper(buf[0]);

            /* Append option to combo box */
            //gtk_combo_box_append_text(GTK_COMBO_BOX(combo), buf);
    }

    /* Set first choice */
    //gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

    /* Add combo box to action box */
    //gtk_box_pack_end(GTK_BOX(action_box), combo, FALSE, TRUE, 0);

    /* Show everything */
    //gtk_widget_show_all(combo);

    /* Process events */
    //gtk_main();

    /* Get selection */
    //i = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
    /* Destroy combo box */
    //gtk_widget_destroy(combo);

    /* Return choice */
    return i;
}

int gui_choose_search_keep(game *g, int who, int arg1, int arg2) {
 	//GtkWidget *combo;
	card *c_ptr;
	displayed *i_ptr;
	char buf[1024];
	int i;

	/* Get card pointer */
	c_ptr = &g->deck[arg1];

	/* Create prompt */
	sprintf(buf, "Choose to keep/discard %s", c_ptr->d_ptr->name);

	/* Set prompt */
	//gtk_label_set_text(GTK_LABEL(action_prompt), buf);

	/* Reset displayed cards */
	reset_cards(g, true, true);

	/* Activate action button */
	//gtk_widget_set_sensitive(action_button, TRUE);

	/* Get next entry in hand list */
	i_ptr = &hand[hand_size++];

	/* Reset structure */
	reset_display(i_ptr);

	/* Add card information */
	i_ptr->index = arg1;
	i_ptr->d_ptr = c_ptr->d_ptr;

	/* Card is in hand */
	i_ptr->hand = 1;

	/* Card should be separated from hand */
	i_ptr->gapped = 1;
	i_ptr->color = 1;
	/* Create simple combo box */
	//combo = gtk_combo_box_new_text();

	/* Append options to combo box */
	//gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
	//                            "Discard (keep searching)");
	//gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Keep card");

	/* Set first choice */
	//gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

	/* Add combo box to action box */
	//gtk_box_pack_end(GTK_BOX(action_box), combo, FALSE, TRUE, 0);

	/* Show everything */
	//gtk_widget_show_all(combo);

	/* Redraw everything */
	redraw_everything();

	/* Process events */
	//gtk_main();

	/* Get selection */
	//i = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));

	/* Destroy combo box */
	//    gtk_widget_destroy(combo);

	/* Return choice */
	return i;
}

int gui_choose_oort_kind(game *g, int who) {
    //GtkWidget *combo;
    char buf[1024];
    int i;

    /* Create prompt */
    sprintf(buf, "Choose Alien Oort Cloud Refinery kind");

    /* Set prompt */
    //gtk_label_set_text(GTK_LABEL(action_prompt), buf);

    /* Reset displayed cards */
    reset_cards(g, true, true);

    /* Activate action button */
    //gtk_widget_set_sensitive(action_button, TRUE);

    /* Create simple combo box */
    //combo = gtk_combo_box_new_text();

    /* Append options to combo box */
    //gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Novelty");
    //gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Rare");
    //gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Genes");
    //gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Alien");

    /* Set first choice */
    //gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

    /* Add combo box to action box */
    //gtk_box_pack_end(GTK_BOX(action_box), combo, FALSE, TRUE, 0);

    /* Show everything */
    //gtk_widget_show_all(combo);

    /* Redraw everything */
    redraw_everything();

    /* Process events */
    //gtk_main();

    /* Get selection */
    //i = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));

    /* Destroy combo box */
    //gtk_widget_destroy(combo);

    /* Return choice */
    return i + 2;
}

void gui_choose_produce(game *g, int who, int cidx[], int oidx[], int num) {
	//GtkWidget *combo;
	card *c_ptr = NULL;
	power *o_ptr, bonus;
	char buf[1024];
	int i;

	/* Activate action button */
	//gtk_widget_set_sensitive(action_button, TRUE);

	/* Reset displayed cards */
	reset_cards(g, true, true);

	/* Redraw everything */
	redraw_everything();

	/* Set prompt */
	//gtk_label_set_text(GTK_LABEL(action_prompt), "Choose Produce power");

	/* Create simple combo box */
	//combo = gtk_combo_box_new_text();

	/* Loop over powers */
	for (i = 0; i < num; i++) {
		/* Check for produce or prestige bonus */
		if (cidx[i] < 0) {
			/* Create fake produce power */
			bonus.code = P5_WINDFALL_ANY;
			o_ptr = &bonus;
		} else {
			/* Get card pointer */
			c_ptr = &g->deck[cidx[i]];

			/* Get power pointer */
			o_ptr = &c_ptr->d_ptr->powers[oidx[i]];
		}

		/* Clear string describing power */
		strcpy(buf, "");

		/* Check for simple powers */
		if (o_ptr->code & P5_DRAW_EACH_NOVELTY) {
			/* Make string */
			sprintf(buf, "Draw per Novelty produced");
		} else if (o_ptr->code & P5_DRAW_EACH_RARE) {
			/* Make string */
			sprintf(buf, "Draw per Rare produced");
		} else if (o_ptr->code & P5_DRAW_EACH_GENE) {
			/* Make string */
			sprintf(buf, "Draw per Gene produced");
		} else if (o_ptr->code & P5_DRAW_EACH_ALIEN) {
			/* Make string */
			sprintf(buf, "Draw per Alien produced");
		} else if (o_ptr->code & P5_DRAW_DIFFERENT) {
			/* Make string */
			sprintf(buf, "Draw per type produced");
		}

		/* Check for discard required */
		if (o_ptr->code & P5_DISCARD) {
			/* Start string */
			sprintf(buf, "Discard to ");
		}

		/* Regular production powers */
		if (o_ptr->code & P5_PRODUCE) {
			/* Add to string */
			strcat(buf, "produce on ");
			strcat(buf, c_ptr->d_ptr->name);
		} else if (o_ptr->code & P5_WINDFALL_ANY) {
			/* Add to string */
			strcat(buf, "produce on any windfall");
		} else if (o_ptr->code & P5_WINDFALL_NOVELTY) {
			/* Add to string */
			strcat(buf, "produce on Novelty windfall");
		} else if (o_ptr->code & P5_WINDFALL_RARE) {
			/* Add to string */
			strcat(buf, "produce on Rare windfall");
		} else if (o_ptr->code & P5_WINDFALL_GENE) {
			/* Add to string */
			strcat(buf, "produce on Genes windfall");
		} else if (o_ptr->code & P5_WINDFALL_ALIEN) {
			/* Add to string */
			strcat(buf, "produce on Alien windfall");
		}
		/* Capitalize string if needed */
		buf[0] = toupper(buf[0]);

		/* Append option to combo box */
		//gtk_combo_box_append_text(GTK_COMBO_BOX(combo), buf);
	}

	/* Set first choice */
	//gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

	/* Add combo box to action box */
	//gtk_box_pack_end(GTK_BOX(action_box), combo, FALSE, TRUE, 0);

	/* Show everything */
	//gtk_widget_show_all(combo);

	/* Process events */
	//gtk_main();

	/* Get selection */
	//i = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));

	/* Destroy combo box */
	//gtk_widget_destroy(combo);

	/* Select chosen power */
	cidx[0] = cidx[i];
	oidx[0] = oidx[i];
}

void gui_choose_action(game *g, int who, int action[2], int one) {

	//GtkWidget *button[MAX_ACTION], *image, *label, *group;
	//GtkWidget *prestige = NULL;
	//int i, h, key = GDK_1;
	int i, h;

	/* Check for advanced game */
	if (real_game.advanced) {
		/* Call advanced function instead */
		return; //gui_choose_action_advanced(g, who, action, one);
	}

	/* Activate action button */
	//gtk_widget_set_sensitive(action_button, TRUE);

	/* Reset displayed cards */
	reset_cards(g, true, true);

	/* Redraw everything */
	redraw_everything();

	/* Set prompt */
	//gtk_label_set_text(GTK_LABEL(action_prompt), "Choose Action");

	/* Clear grouping of buttons */
	//group = NULL;

	/* Get height of action box */
	//h = action_box->allocation.height - 10;

	/* Loop over actions */
	for (i = 0; i < MAX_ACTION; i++) {
	    /* Clear button pointer */
		//button[i] = NULL;

		/* Check for unusable search action */
		if (i == ACT_SEARCH && (real_game.expanded < 3 ||
				real_game.p[who].prestige_action_used)) {
	        /* Skip search action */
			continue;
		}

		/* Skip second develop/settle */
		if (i == ACT_DEVELOP2 || i == ACT_SETTLE2) continue;

		/* Create radio button */
		//button[i] = gtk_radio_button_new_from_widget(
			//	GTK_RADIO_BUTTON(group));

		/* Remember grouping */
		//group = button[i];

		/* Get icon for action */
		//image = action_icon(i, h);

		/* Do not request height */
		//gtk_widget_set_size_request(image, -1, 0);

		/* Draw button without separate indicator */
		//gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(button[i]), FALSE);

		/* Pack image into button */
		//gtk_container_add(GTK_CONTAINER(button[i]), image);

		/* Pack button into action box */
		//gtk_box_pack_start(GTK_BOX(action_box), button[i], FALSE,
			//	FALSE, 0);

		/* Create tooltip for button */
		//gtk_widget_set_tooltip_text(button[i], action_name(i));

		/* Add handler for keypresses */
		//gtk_widget_add_accelerator(button[i], "key-signal",
			//	window_accel, key++, 0, 0);

		/* Connect "pointer enter" signal */
		//g_signal_connect(G_OBJECT(button[i]), "enter-notify-event",
			//	G_CALLBACK(redraw_action), GINT_TO_POINTER(i));

		/* Connect key-signal */
		//g_signal_connect(G_OBJECT(button[i]), "key-signal",
			//	G_CALLBACK(action_keyed), NULL);

		/* Show everything */
		//gtk_widget_show_all(button[i]);
	}

	/* Check for usable prestige action */
	if (real_game.expanded >= 3 && !real_game.p[who].prestige_action_used &&
			real_game.p[who].prestige > 0) {
		/* Create toggle button for prestige */
		//prestige = gtk_toggle_button_new();

		/* Get icon for action */
		//image = action_icon(ICON_PRESTIGE, h);

		/* Do not request height */
		//gtk_widget_set_size_request(image, -1, 0);

		/* Pack image into button */
		//gtk_container_add(GTK_CONTAINER(prestige), image);

		/* Pack button into action box */
		//gtk_box_pack_start(GTK_BOX(action_box), prestige, FALSE,
			//	FALSE, h);

		/* Connect "pointer enter" signal */
		//g_signal_connect(G_OBJECT(prestige), "enter-notify-event",
			//	G_CALLBACK(redraw_action),
				//GINT_TO_POINTER(10));

		/* Show everything */
		//gtk_widget_show_all(prestige);
	}

	/* Create filler label */
	//label = gtk_label_new("");

	/* Add label after action buttons */
	//gtk_box_pack_start(GTK_BOX(action_box), label, TRUE, TRUE, 0);

	/* Show label */
	//gtk_widget_show(label);

	/* Process events */
	//gtk_main();

	/* Loop over choices */
	for (i = 0; i < MAX_ACTION; i++) {
		/* Skip uncreated buttons */
		//if (button[i] == NULL) continue;

		/* Check for active */
		//if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button[i]))) {
			/* Set choice */
			//action[0] = i;
			//action[1] = -1;
		//}
	}

	/* Check for prestige button available */
	//if (prestige) {
		/* Check for pressed */
		//if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prestige)) &&
			//	action[0] != ACT_SEARCH) {
			/* Add prestige flag to action */
			//action[0] |= ACT_PRESTIGE;
		//}

		/* Destroy prestige button */
		//gtk_widget_destroy(prestige);
	//}

	/* Destroy buttons */
	for (i = 0; i < MAX_ACTION; i++) {
		/* Skip uncreated buttons */
		//if (button[i] == NULL) continue;

		/* Destroy button */
		//gtk_widget_destroy(button[i]);
	}

	/* Destroy filler label */
	//gtk_widget_destroy(label);
}

int gui_choose_lucky(game *g, int who) {
}

void gui_choose_takeover_prevent(game *g, int who, int list[], int *num,
                                        int special[], int *num_special) {
 	//GtkWidget *combo;
	card *c_ptr, *b_ptr;
	char buf[1024];
	int i;

	/* Activate action button */
	//gtk_widget_set_sensitive(action_button, TRUE);

	/* Reset displayed cards */
	reset_cards(g, true, true);

	/* Redraw everything */
	redraw_everything();

	/* Create prompt */
	sprintf(buf, "Choose takeover to prevent");

	/* Set prompt */
	//gtk_label_set_text(GTK_LABEL(action_prompt), buf);
	label_set_text(buf);

	/* Create simple combo box */
	//combo = gtk_combo_box_new_text();

	/* Loop over powers */
	for (i = 0; i < *num; i++) {
		/* Get target world */
		c_ptr = &g->deck[list[i]];

		/* Get card holding takeover power being used */
		b_ptr = &g->deck[special[i]];

		/* Format choice */
		sprintf(buf, "%s using %s", c_ptr->d_ptr->name,
			b_ptr->d_ptr->name);

		/* Append option to combo box */
		//gtk_combo_box_append_text(GTK_COMBO_BOX(combo), buf);
	}

	/* Add choice for no prevention */
	sprintf(buf, "None (allow all takeovers)");

	/* Append option to combo box */
	//gtk_combo_box_append_text(GTK_COMBO_BOX(combo), buf);

	/* Set last choice */
	//gtk_combo_box_set_active(GTK_COMBO_BOX(combo), *num);

	/* Add combo box to action box */
	//gtk_box_pack_end(GTK_BOX(action_box), combo, FALSE, TRUE, 0);

	/* Show everything */
	//gtk_widget_show_all(combo);

	/* Process events */
	//gtk_main();

	/* Get selection */
	//i = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));

	/* Destroy combo box */
	//gtk_widget_destroy(combo);

	/* Check for last choice (no prevention) */
	if (i == *num) {
		/* Set no choice */
		*num = *num_special = 0;
		return;
	}

	/* Select takeover to prevent */
	list[0] = list[i];
	special[0] = special[i];
	*num = *num_special = 1;
}

void gui_choose_consume(game *g, int who, int cidx[], int oidx[], int *num,
                               int *num_special, int optional) {
	//GtkWidget *combo;
	card *c_ptr;
	power *o_ptr, prestige_bonus;
	pow_loc l_list[MAX_DECK];
	char buf[1024], *name, buf2[1024];
	int i;

	/* Activate action button */
	//gtk_widget_set_sensitive(action_button, TRUE);

	/* Reset displayed cards */
	reset_cards(g, true, true);

	/* Redraw everything */
	redraw_everything();

	/* Set prompt */
	//gtk_label_set_text(GTK_LABEL(action_prompt), "Choose Consume power");
	label_set_text("Choose Consume power");

	/* Create simple combo box */
	//combo = gtk_combo_box_new_text();

	/* Loop over powers */
	for (i = 0; i < *num; i++) {
		/* Create power location */
		l_list[i].c_idx = cidx[i];
		l_list[i].o_idx = oidx[i];
	}

	/* Sort consume powers */
	qsort(l_list, *num, sizeof(pow_loc), cmp_consume);

	/* Loop over powers */
	for (i = 0; i < *num; i++) {
		/* Check for prestige trade bonus power */
		if (l_list[i].c_idx < 0) {
			/* Make fake power */
			prestige_bonus.phase = PHASE_CONSUME;
			prestige_bonus.code = P4_DISCARD_HAND | P4_GET_VP;
			prestige_bonus.value = 1;
			prestige_bonus.times = 2;

			/* Use fake power */
			o_ptr = &prestige_bonus;
		}
		else {
			/* Get card pointer */
			c_ptr = &g->deck[l_list[i].c_idx];

			/* Get power pointer */
			o_ptr = &c_ptr->d_ptr->powers[l_list[i].o_idx];
		}

		/* Check for simple powers */
		if (o_ptr->code == P4_DRAW) {
			/* Make string */
			sprintf(buf, "Draw %d", o_ptr->value);
		}
		else if (o_ptr->code == P4_VP) {
			/* Make string */
			sprintf(buf, "Take VP");
		}
		else if (o_ptr->code == P4_DRAW_LUCKY) {
			/* Make string */
			sprintf(buf, "Draw if lucky");
		}
		else if (o_ptr->code == P4_ANTE_CARD) {
			/* Make string */
			sprintf(buf, "Ante card");
		}
		else if (o_ptr->code & P4_CONSUME_3_DIFF) {
			/* Make string */
			sprintf(buf, "Consume 3 kinds");
		}
		else if (o_ptr->code & P4_CONSUME_N_DIFF) {
			/* Make string */
			sprintf(buf, "Consume different kinds");
		}
		else if (o_ptr->code & P4_CONSUME_ALL) {
			/* Make string */
			sprintf(buf, "Consume all goods");
		}
		else if (o_ptr->code & P4_TRADE_ACTION) {
			/* Make string */
			sprintf(buf, "Trade good");

			/* Check for no bonuses */
			if (o_ptr->code & P4_TRADE_NO_BONUS) {
				/* Append qualifier */
				strcat(buf, " (no bonus)");
			}
		}
		else {
			/* Get type of good to consume */
			if (o_ptr->code & P4_CONSUME_NOVELTY) {
				/* Novelty good */
				name = "Novelty ";
			}
			else if (o_ptr->code & P4_CONSUME_RARE) {
				/* Rare good */
				name = "Rare ";
			}
			else if (o_ptr->code & P4_CONSUME_GENE) {
				/* Gene good */
				name = "Gene ";
			}
			else if (o_ptr->code & P4_CONSUME_ALIEN) {
				/* Alien good */
				name = "Alien ";
			}
			else {
				/* Any good */
				name = "";
			}

			/* Start consume string */
			if (o_ptr->code & P4_DISCARD_HAND) {
				/* Make string */
				sprintf(buf, "Consume from hand for ");
			}
			else if (o_ptr->code & P4_CONSUME_TWO) {
				/* Start string */
				sprintf(buf, "Consume two %sgoods for ", name);
			}
			else if (o_ptr->code & P4_CONSUME_PRESTIGE) {
				/* Make string */
				sprintf(buf, "Consume prestige for ");
			}
			else {
				/* Start string */
				sprintf(buf, "Consume %sgood for ", name);
			}

			/* Check for cards */
			if (o_ptr->code & P4_GET_CARD) {
				/* Create card reward string */
				sprintf(buf2, "%d card%s", o_ptr->value,
					(o_ptr->value != 1) ? "s" : "");

				/* Add to string */
				strcat(buf, buf2);

				/* Check for other reward as well */
				if (o_ptr->code & (P4_GET_VP | P4_GET_PRESTIGE)) {
					/* Add "and" */
					strcat(buf, " and ");
				}
			}

			/* Check for extra cards */
			if (o_ptr->code & P4_GET_2_CARD) {
				/* Create card reward string */
				strcat(buf, "2 cards");

				/* Check for other reward as well */
				if (o_ptr->code & (P4_GET_VP | P4_GET_PRESTIGE)) {
					/* Add "and" */
					strcat(buf, " and ");
				}
			}

			/* Check for points */
			if (o_ptr->code & P4_GET_VP) {
				/* Create VP reward string */
				sprintf(buf2, "%d VP", o_ptr->value);

				/* Add to string */
				strcat(buf, buf2);

				/* Check for other reward as well */
				if (o_ptr->code & P4_GET_PRESTIGE) {
					/* Add "and" */
					strcat(buf, " and ");
				}
			}

			/* Check for prestige */
			if (o_ptr->code & P4_GET_PRESTIGE) {
				/* Create prestige reward string */
				sprintf(buf2, "%d prestige", o_ptr->value);
				/* Add to string */
				strcat(buf, buf2);
			}

			/* Check for multiple times */
			if (o_ptr->times > 1) {
				/* Create times string */
				sprintf(buf2, " (x%d)", o_ptr->times);

				/* Add to string */
				strcat(buf, buf2);
			}
		}

		/* Append option to combo box */
		//gtk_combo_box_append_text(GTK_COMBO_BOX(combo), buf);
		// call into java with combo text (buf)
		JNIEnv *env = NULL;
		// check for error on env?
		(*cached_jvm)->GetEnv(cached_jvm, (void **) &env, JNI_VERSION_1_6);
		(*env)->CallStaticVoidMethod(env, Class_C, MID_createComboBox, buf);
	}

	/* Check for all optional powers */
	if (optional) {
		/* Append no choice option */
		sprintf(buf, "None (done with Consume)");

		/* Append option to combo box */
		//gtk_combo_box_append_text(GTK_COMBO_BOX(combo), buf);
	}
	/* Set first choice */
	//gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

	/* Add combo box to action box */
	//gtk_box_pack_end(GTK_BOX(action_box), combo, FALSE, TRUE, 0);

	/* Show everything */
	//gtk_widget_show_all(combo);

	/* Process events */
	//gtk_main();

	/* Get selection */
	//i = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));

	/* Destroy combo box */
	//gtk_widget_destroy(combo);

	/* Check for done */
	if (i == *num) {
		/* Set no choice */
		*num = *num_special = 0;
		return;
	}

	/* Select chosen power */
	cidx[0] = l_list[i].c_idx;
	oidx[0] = l_list[i].o_idx;
	*num = *num_special = 1;
}

void gui_choose_action_advanced(game *g, int who, int action[2], int one) {
	//GtkWidget *prestige = NULL, *image, *label;
	//int i, a, h, n = 0, key = GDK_1;
	int i, a, h, n = 0;

	/* Deactivate action button */
	//gtk_widget_set_sensitive(action_button, FALSE);

	/* Clear count of buttons chosen */
	//actions_chosen = 0;

	/* Check for needing only one action */
	//if (one == 1) actions_chosen = 1;

	/* Reset displayed cards */
	reset_cards(g, true, true);

	/* Redraw everything */
	redraw_everything();

	/* Set prompt */
	//gtk_label_set_text(GTK_LABEL(action_prompt), "Choose Actions");

	/* Check for needing only first/second action */
	if (one == 1)
	{
		/* Set prompt */
		//gtk_label_set_text(GTK_LABEL(action_prompt),
		//		"Choose first Action");
	}
	else if (one == 2)
	{
		/* Set prompt */
		//gtk_label_set_text(GTK_LABEL(action_prompt),
		//		"Choose second Action");
	}

	/* Get height of action box */
	//h = action_box->allocation.height - 10;

	/* Loop over actions */
	for (i = 0; i < MAX_ACTION; i++)
	{
		/* Check for unusable search action */
		if (i == ACT_SEARCH && (g->expanded < 3 ||
				g->p[who].prestige_action_used ||
				(one == 2 &&
				 g->p[who].action[0] & ACT_PRESTIGE)))
                	{
				/* Clear toggle button */
				//action_toggle[i] = NULL;
				/* Skip search action */
				continue;
			}

		/* Create toggle button */
		//action_toggle[i] = gtk_toggle_button_new();

		/* Get action index */
		a = i;

		/* Check for second Develop or Settle */
		if (a == ACT_DEVELOP2) a = ACT_DEVELOP;
		if (a == ACT_SETTLE2) a = ACT_SETTLE;

		/* Get icon for action */
		//image = action_icon(a, h);

		/* Do not request height */
		//gtk_widget_set_size_request(image, -1, 0);

		/* Pack image into button */
		//gtk_container_add(GTK_CONTAINER(action_toggle[i]), image);

		/* Pack button into action box */
		//gtk_box_pack_start(GTK_BOX(action_box), action_toggle[i], FALSE,
		//		FALSE, 0);

		/* Create tooltip for button */
		//gtk_widget_set_tooltip_text(action_toggle[i], action_name(i));
		/* Add handler for keypresses */
		//gtk_widget_add_accelerator(action_toggle[i], "key-signal",
		//		window_accel, key++, 0, 0);

		/* Connect "toggled" signal */
		//g_signal_connect(G_OBJECT(action_toggle[i]), "toggled",
		//		G_CALLBACK(action_choice_changed),
		//		GINT_TO_POINTER(i));

		/* Connect "pointer enter" signal */
		//g_signal_connect(G_OBJECT(action_toggle[i]),
		//		"enter-notify-event",
		//		G_CALLBACK(redraw_action), GINT_TO_POINTER(a));

		/* Connect key-signal */
		//g_signal_connect(G_OBJECT(action_toggle[i]), "key-signal",
		//		G_CALLBACK(action_keyed), NULL);

		/* Show everything */
		//gtk_widget_show_all(action_toggle[i]);

		/* Check for choosing second action and this was first */
		if (one == 2 && (g->p[player_us].action[0] & ACT_MASK) == i)
		{
			/* Press button */
			//gtk_toggle_button_set_active(
			//		GTK_TOGGLE_BUTTON(action_toggle[i]), TRUE);

			/* Do not allow user to press button */
			//gtk_widget_set_sensitive(action_toggle[i], FALSE);
			/* Check for prestige action */
			if (g->p[player_us].action[0] & ACT_PRESTIGE)
			{
				/* Reset icon */
				//reset_action_icon(action_toggle[i],
				//		i | ACT_PRESTIGE);
			}
		}
	}

	/* Do not boost any action yet */
	//prestige_action = -1;

	/* Check for forced first action */
	if (one == 2)
	{
		/* Check for first action as prestige */
		if (g->p[player_us].action[0] & ACT_PRESTIGE)
		{
			/* Mark prestige action */
			//prestige_action = g->p[player_us].action[0] & ACT_MASK;
		}
	}

	/* Check for usable prestige action */
	//if (real_game.expanded >= 3 && !real_game.p[who].prestige_action_used &&
	//		real_game.p[who].prestige > 0 && prestige_action == -1)
	//{
	/* Create button to toggle prestige */
	//prestige = gtk_button_new();

	/* Get icon for action */
	//image = action_icon(ICON_PRESTIGE, h);

	/* Do not request height */
	//gtk_widget_set_size_request(image, -1, 0);

	/* Pack image into button */
	//gtk_container_add(GTK_CONTAINER(prestige), image);

	/* Pack button into action box */
	//gtk_box_pack_start(GTK_BOX(action_box), prestige, FALSE,
	//		FALSE, h);

	/* Connect "pointer enter" signal */
	//g_signal_connect(G_OBJECT(prestige), "enter-notify-event",
	//		G_CALLBACK(redraw_action),
	//		GINT_TO_POINTER(10));

	/* Connect "pressed" signal */
	//g_signal_connect(G_OBJECT(prestige), "pressed",
	//		G_CALLBACK(prestige_pressed), NULL);

	/* Show everything */
	//gtk_widget_show_all(prestige);
	//}

	/* Create filler label */
	//label = gtk_label_new("");
	/* Add label after action buttons */
	//gtk_box_pack_start(GTK_BOX(action_box), label, TRUE, TRUE, 0);

	/* Show label */
	//gtk_widget_show(label);

	/* Process events */
	//gtk_main();

	/* Loop over choices */
	for (i = 0; i < MAX_ACTION; i++)
	{
		/* Skip unavailable actions */
		//if (!action_toggle[i]) continue;

		/* Check for active */
		//if (gtk_toggle_button_get_active(
			//	GTK_TOGGLE_BUTTON(action_toggle[i])))
		//{
			/* Check for prestige action */
			//if (i == prestige_action)
			//{
				/* Mark prestige as chosen */
			//	action[n++] = i | ACT_PRESTIGE;
			//}
			//else
			//{
				/* Set choice */
			//	action[n++] = i;
			//}
		//}
	}

	/* Destroy buttons */
	//for (i = 0; i < MAX_ACTION; i++)
	//{
		/* Skip unavailable actions */
		//if (!action_toggle[i]) continue;

		/* Destroy button */
		//gtk_widget_destroy(action_toggle[i]);
	//}

	/* Destroy filler label */
	//gtk_widget_destroy(label);

	/* Destroy prestige button if created */
	//if (prestige) gtk_widget_destroy(prestige);

	/* Check for second Develop chosen without first */
	if ((action[0] & ACT_MASK) == ACT_DEVELOP2)
		action[0] = ACT_DEVELOP | (action[0] & ACT_PRESTIGE);
	if ((action[1] & ACT_MASK) == ACT_DEVELOP2 &&
			(action[0] & ACT_MASK) != ACT_DEVELOP)
		action[1] = ACT_DEVELOP | (action[1] & ACT_PRESTIGE);

	/* Check for second Settle chosen without first */
	if ((action[0] & ACT_MASK) == ACT_SETTLE2)
		action[0] = ACT_SETTLE | (action[0] & ACT_PRESTIGE);
	if ((action[1] & ACT_MASK) == ACT_SETTLE2 &&
			(action[0] & ACT_MASK) != ACT_SETTLE)
		action[1] = ACT_SETTLE | (action[1] & ACT_PRESTIGE);
}

void label_set_text(char *buf) {
	//gtk_label_set_text(GTK_LABEL(action_prompt), buf);
}

void widget_set_sensitive(bool activate) {
	// gtk_widget_set_sensitive(action_button, activate);
}


//static bool redraw_full(GtkWidget *widget, GdkEventCrossing *event,
//			    gpointer data)
bool redraw_full(int index)
{
        //design *d_ptr = (design *)data;

        /* Check for no design */
        //if (!d_ptr)
	//{
		/* Set image to card back */
		//buf = card_back;
	//}
	//  else
	//{
		/* Set image to card face */
		//buf = image_cache[d_ptr->index];
	//}

        /* Check for halfsize image */
	//  if (opt.full_reduced == 1)
	//{
		/* Scale image */
		//buf = gdk_pixbuf_scale_simple(buf, CARD_WIDTH / 2,
						      //CARD_HEIGHT / 2, GDK_INTERP_BILINEAR);
	//}

	/* Set image */
        //gtk_image_set_from_pixbuf(GTK_IMAGE(full_image), buf);
	// call into Java
	JNIEnv *env;
	(*cached_jvm)->GetEnv(cached_jvm, (void **) &env, JNI_VERSION_1_6);
	(*env)->CallStaticVoidMethod(env, Class_C, MID_fullImage, index);

        /* Check for halfsize image */
	//  if (opt.full_reduced == 1)
	//{
		/* Remove our scaled buffer */
		//g_object_unref(G_OBJECT(buf));
	//}

        /* Event handled */
	return true;
}


void modify_gui(void) {
	int i;

	/* Check for basic game */
	if (!real_game.expanded || real_game.goal_disabled)
	{
		/* Hide goal area */
		//	gtk_widget_hide(goal_area);
	}
	else
	{
		/* Show goal area */
		//	gtk_widget_show(goal_area);
	}

	/* Loop over existing players */
	for (i = 0; i < real_game.num_players; i++)
        {
		/* Show status */
		//gtk_widget_show_all(player_box[i]);
	}

	/* Loop over non-existant players */
	for ( ; i < MAX_PLAYER; i++)
        {
		/* Hide status */
		//gtk_widget_hide_all(player_box[i]);
	}

	/* Show/hide separators */
	for (i = 1; i < MAX_PLAYER; i++)
        {
		/* Do not show first or last separators */
		if (i < 2 || i >= real_game.num_players)
		{
			/* Hide separator */
			//gtk_widget_hide(player_sep[i]);
		}
		else
		{
			/* Show separator */
			//gtk_widget_show(player_sep[i]);
		}
	}

	/* Check for no full-size image */
	if (opt.full_reduced == 2)
	{
		/* Hide image */
		//gtk_widget_hide(full_image);
	}
	else
	{
		/* Show image */
		//gtk_widget_show(full_image);
	}

	/* Redraw full-size image */
	//redraw_full(NULL, NULL, NULL);
	redraw_full(-1);

	/* Resize status areas */
	//status_resize();

	/* Handle pending events */
	//while (gtk_events_pending()) gtk_main_iteration();
}

void restore_status_area(int i) {
	JNIEnv *env = NULL;
	(*cached_jvm)->GetEnv(cached_jvm, (void **) &env, JNI_VERSION_1_6);
	(*env)->CallStaticVoidMethod(env, Class_C, MID_restoreStatusArea, i);
	//player_status[i] = orig_status[i];
}

void restore_table_area(int i) {
	JNIEnv *env = NULL;
	(*cached_jvm)->GetEnv(cached_jvm, (void **) &env, JNI_VERSION_1_6);
	(*env)->CallStaticVoidMethod(env, Class_C, MID_restoreTableArea, i);
	//player_area[i] = orig_area[i];
}

void process_events() {
	// gtk_main();
}

void clear_log() {
 	//GtkTextBuffer *message_buffer;

 	/* Get message buffer */
 	//message_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(message_view));

 	/* Clear text buffer */
 	//gtk_text_buffer_set_text(message_buffer, "", 0);

 	/* Reset last seen line */
 	//message_last_y = 0;
	JNIEnv *env;
	// check for error on env?
	//(*cached_jvm)->GetEnv(cached_jvm, (void **) &env, JNI_VERSION_1_6);
	int getEnvStat = (*cached_jvm)->GetEnv(cached_jvm, (void **) &env, JNI_VERSION_1_6);
	jstring js = (*env)->NewStringUTF(env, "");
	(*env)->CallStaticVoidMethod(env, Class_C, MID_messageAdd, js);

}

void message_add(game *g, char *msg) {
	JNIEnv *env;
	// check for error on env?
	(*cached_jvm)->GetEnv(cached_jvm, (void **) &env, JNI_VERSION_1_6);
	jstring js = (*env)->NewStringUTF(env, msg);
	(*env)->CallStaticVoidMethod(env, Class_C, MID_messageAdd, js);
	//GtkTextIter end_iter;
	//GtkTextBuffer *message_buffer;

	/* Get message buffer */
	//message_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(message_view));

	/* Get end mark */
	//gtk_text_buffer_get_iter_at_mark(message_buffer, &end_iter,
	//                               message_end);

	/* Add message */
	//gtk_text_buffer_insert(message_buffer, &end_iter, msg, -1);

	/* Scroll to end mark */
	//gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(message_view),
	//                                 message_end);

}

static void read_prefs() {
	/* Check range of values */
	if (opt.num_players < 2) opt.num_players = 2;
	if (opt.num_players > MAX_PLAYER) opt.num_players = MAX_PLAYER;
	if (opt.expanded < 0) opt.expanded = 0;
	if (opt.expanded > MAX_EXPANSION - 1) opt.expanded = MAX_EXPANSION - 1;
}

void gui_notify_rotation(game *g, int who) {
}

/*
 * Interface to GUI decision functions.
 */
decisions gui_func =
{
	NULL,
	gui_notify_rotation,
	NULL,
	gui_make_choice,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};


static void initGame(JNIEnv *env, jclass classz) {
	real_game.random_seed = time(NULL);

	/* Load card designs */
	read_cards();

	/* Load card images */
	// done in Java onCreate()
	//load_images();

	/* Read preference file */
	read_prefs();

	apply_options();

	/* Create choice logs for each player */
	int i;
	for (i = 0; i < MAX_PLAYER; i++) {
		/* Create log */
		real_game.p[i].choice_log = (int *)malloc(sizeof(int) * 4096);

		/* Save original log */
		orig_log[i] = real_game.p[i].choice_log;

		/* Create history of log sizes */
		real_game.p[i].choice_history = (int*)malloc(sizeof(int) * 512);

		/* Save original history */
		orig_history[i] = real_game.p[i].choice_history;

		/* Clear choice log size and position */
		real_game.p[i].choice_size = 0;
		real_game.p[i].choice_pos = 0;
	}

 	reset_gui();
	modify_gui(); // adjust for the number of players

	/* Start new game */
	restart_loop = RESTART_NEW;

	run_game();
}

static void readCards(JNIEnv *env, jclass clazz) {
}

/*
 * JNI registration.
 */
static JNINativeMethod methods[] = {
	/* name,            signature,       funcPtr */
	{ "readCards"       , "()V",         (void*)readCards },
	{ "initGame"        , "()V",         (void*)initGame  },
};

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *jvm, void *reserved)
{
	JNIEnv *env;
	jclass cls;
	cached_jvm = jvm;  /* cache the JavaVM pointer */

	if (((*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_6)) != JNI_OK) {
		return JNI_ERR; /* JNI version not supported */
	}
	cls = (*env)->FindClass(env, "com/example/rftg/MainActivity");
	if (cls == NULL) {
		return JNI_ERR;
	}

	/* Use weak global ref to allow C class to be unloaded */
	Class_C = (*env)->NewWeakGlobalRef(env, cls);
	if (Class_C == NULL) {
		return JNI_ERR;
	}

	(*env)->RegisterNatives(env, cls, methods, sizeof(methods)/sizeof(methods[0]));

	MID_createComboBox = (*env)->GetStaticMethodID(env, cls, "createComboBox", "(Ljava/lang/String;)V");
	if (MID_createComboBox == NULL) {
		return JNI_ERR;
	}

	MID_messageAdd = (*env)->GetStaticMethodID(env, cls, "messageAdd", "(Ljava/lang/String;)V");
	if (MID_messageAdd == NULL) {
		return JNI_ERR;
	}

	MID_restoreStatusArea = (*env)->GetStaticMethodID(env, cls, "restoreStatusArea", "(I)V");
	if (MID_restoreStatusArea == NULL) {
		return JNI_ERR;
	}

	MID_restoreTableArea = (*env)->GetStaticMethodID(env, cls, "restoreTableArea", "(I)V");
	if (MID_restoreTableArea == NULL) {
		return JNI_ERR;
	}

	MID_fullImage = (*env)->GetStaticMethodID(env, cls, "fullImage", "(I)V");
	if (MID_fullImage == NULL) {
		return JNI_ERR;
	}

	MID_drawCard = (*env)->GetStaticMethodID(env, cls, "drawCard", "(II)V");
	if (MID_drawCard == NULL) {
		return JNI_ERR;
	}

	MID_clearDrawCard = (*env)->GetStaticMethodID(env, cls, "clearDrawCard", "()V");
	if (MID_clearDrawCard == NULL) {
		return JNI_ERR;
	}

	return JNI_VERSION_1_6;
}

jint JNI_UnLoad(JavaVM* vm, void* reserved) {
	return 1;
}
