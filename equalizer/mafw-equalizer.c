/*
 * This file is a part of MAFW-GST-EQ-RENDERER
 *
 * Copyright (C) Igalia S.L.
 * Author: Juan A. Suarez Romero <jasuarez@igalia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <hildon/hildon.h>

#include <hildon-cp-plugin/hildon-cp-plugin-interface.h>

#include <gtk/gtk.h>

#include <gconf/gconf-client.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "constants.h"

#define NUM_BANDS 10

#define PRESETS_PATH "/home/user/.presets"

#define XML_NODE_BAND        "band"
#define XML_NODE_EQUALIZER   "equalizer"
#define XML_PROP_BAND_NUMBER "num"

static GConfClient *confclient = NULL;

static const gchar *title_band[NUM_BANDS] = { "29 Hz: %.1f dB",
                                              "59 Hz: %.1f dB",
                                              "119 Hz: %.1f dB",
                                              "227 Hz: %.1f dB",
                                              "474 Hz: %.1f dB",
                                              "947 Hz: %.1f dB",
                                              "2 KHz: %.1f dB",
                                              "4 KHz: %.1f dB",
                                              "8 KHz: %.1f dB",
                                              "15 KHz: %.1f dB" };

struct label_band {
        GtkWidget *label;
        gint id;
};

struct dialog_and_sliders {
        GtkWidget *dialog;
        GtkWidget **slider_band;
};


/* Returns currently stored gain in gconf for band */
static gdouble get_band_value(gint band)
{
        gchar *key;
        gdouble value;

        g_return_val_if_fail(band >= 0 && band < NUM_BANDS, 0);

        if (!confclient) {
                confclient = gconf_client_get_default();
        }

        key = g_strdup_printf("%s%d", GCONF_MAFW_GST_EQ_RENDERER "/band", band);
        value =  gconf_client_get_float(confclient, key, NULL);
        g_free(key);

        return CLAMP(value, EQ_GAIN_MIN, EQ_GAIN_MAX);
}


/* Callback invoked when slider #band changes value. Updates gconf value */
static void update_band_cb(GtkRange *range, gpointer band)
{
        gchar *key;
        gdouble value;

        if (!confclient) {
                confclient = gconf_client_get_default();
        }

        key = g_strdup_printf("%s%d", GCONF_MAFW_GST_EQ_RENDERER "/band",
                              GPOINTER_TO_INT(band));
        value = gtk_range_get_value(range);

        gconf_client_set_float(confclient, key, value, NULL);
        g_free(key);
}

/* Callback invoked when slider #band changes value. Updates current gain value
 * shown in the slider */
static void update_label_cb(GtkRange *range, gpointer user_data)
{
        gchar *label_value;
        struct label_band *lband = (struct label_band *) user_data;

        label_value = g_strdup_printf(title_band[lband->id],
                                      gtk_range_get_value(range));
        gtk_label_set_label(GTK_LABEL(lband->label), label_value);
        g_free(label_value);
}

/* Callback invoked when equalizer values change in gconf. Updates sliders
 * accordingly */
static void update_slider_cb(GConfClient *client,
                             guint cnxn_id,
                             GConfEntry *entry,
                             GtkWidget *slider_band[NUM_BANDS])
{
        const gchar *key;
        GConfValue *value;
        gdouble gain;
        gint band_number;

        key = gconf_entry_get_key(entry);
        value = gconf_entry_get_value(entry);

        /* Get band number */
        sscanf (key, GCONF_MAFW_GST_EQ_RENDERER "/band%d", &band_number);
        if (band_number < NUM_BANDS) {
                gain = value?
                        CLAMP(gconf_value_get_float(value), -24.0, +12.0): 0.0;
                gtk_range_set_value(GTK_RANGE(slider_band[band_number]), gain);
        }
}

/* Sort two xmlDoc based on their name */
static gint sort_xmldoc(gconstpointer a, gconstpointer b)
{
        xmlDoc *a_xml = (xmlDoc *) a;
        xmlDoc *b_xml = (xmlDoc *) b;

        return g_utf8_collate(a_xml->name, b_xml->name);
}

/* Returns a list with all available presets. Each preset is returned as a
 * xmlDoc */
static GList *presets_preload_all(void)
{
        GError *error = NULL;
        GDir *preset_path;
        const gchar *preset_name;
        gchar *preset_fullname;
        GList *preset_list = NULL;
        xmlDoc *preset_xml;

        preset_path = g_dir_open(PRESETS_PATH, 0, &error);
        if (error) {
                return NULL;
        }

        /* Populate the list with valid presets */
        while ((preset_name = g_dir_read_name(preset_path)) != NULL) {
                preset_fullname = g_strconcat(PRESETS_PATH, "/", preset_name,
                                              NULL);
                preset_xml = xmlReadFile(preset_fullname, NULL, 0);
                if (preset_xml) {
                        preset_xml->name = g_strdup(preset_name);
                        preset_list = g_list_prepend (preset_list, preset_xml);
                }
                g_free(preset_fullname);
        }
        g_dir_close(preset_path);

        /* Sort list alphabetically */
        if (preset_list) {
                preset_list = g_list_sort(preset_list, sort_xmldoc);
        }

        return preset_list;
}

/* Load preset values into gconf */
static void preset_load(xmlDoc *preset)
{
        xmlNode *root_node;
        xmlNode *current_node;
        xmlChar *band_number;
        xmlChar *value_text;
        gdouble value;
        gchar *key;

        g_return_if_fail(preset != NULL);

        if (!confclient) {
                confclient = gconf_client_get_default();
        }

        root_node = xmlDocGetRootElement(preset);
        current_node = root_node->children;
        while (current_node) {
                if (xmlStrcmp(current_node->name,
                              BAD_CAST XML_NODE_BAND) == 0) {
                        band_number = xmlGetProp(current_node,
                                                 BAD_CAST XML_PROP_BAND_NUMBER);
                        if (band_number) {
                                key = g_strconcat(GCONF_MAFW_GST_EQ_RENDERER,
                                                  "/band",
                                                  band_number,
                                                  NULL);
                                value_text = xmlNodeGetContent(current_node);
                                value = atof((char *) value_text);
                                gconf_client_set_float(confclient, key,
                                                       value, NULL);
                                g_free(key);
                                g_free(band_number);
                                xmlFree(value_text);
                        }
                }
                current_node = current_node->next;
        }
}

/* Populate the preset with the current values in sliders, and save it to
 * disk */
static void preset_save(xmlDoc *preset, GtkWidget *slider_band[NUM_BANDS])
{
        gint i;
        gchar *gain;
        gchar *band_number;
        gchar *preset_fullname;
        xmlNode *child_node;
        xmlNode *root_node;

        /* Create nodes with band values */
        root_node = xmlNewDocNode(preset, NULL, BAD_CAST XML_NODE_EQUALIZER,
                                  NULL);
        xmlDocSetRootElement(preset, root_node);
        for (i = 0; i < NUM_BANDS; i++) {
                gain = g_strdup_printf(
                        "%.1f",
                        gtk_range_get_value(GTK_RANGE(slider_band[i])));
                band_number = g_strdup_printf("%d", i);
                child_node = xmlNewChild(root_node, NULL,
                                         BAD_CAST XML_NODE_BAND,
                                         BAD_CAST gain);
                xmlSetProp(child_node, BAD_CAST XML_PROP_BAND_NUMBER,
                           BAD_CAST band_number);
                g_free(gain);
                g_free(band_number);
        }

        preset_fullname = g_strdup_printf ("%s/%s", PRESETS_PATH, preset->name);
        xmlSaveFile(preset_fullname, preset);
        g_free(preset_fullname);
}

/* Shos a dialog asking for user confirmation. Returns GTK_RESPONSE_YES or
 * GTK_RESPONSE_NO */
static gint dialog_confirm(GtkWidget *parent, const gchar *message)
{
        GtkWidget *dialog;
        GtkWidget *label;
        GtkWidget *content;
        gint response;

        dialog = gtk_dialog_new_with_buttons("Confirmation",
                                             GTK_WINDOW(parent),
                                             GTK_DIALOG_MODAL,
                                             GTK_STOCK_YES,
                                             GTK_RESPONSE_YES,
                                             GTK_STOCK_NO,
                                             GTK_RESPONSE_NO,
                                             NULL);

        content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        label = gtk_label_new(message);
        gtk_container_add(GTK_CONTAINER(content), label);
        gtk_widget_show_all(dialog);

        response = gtk_dialog_run(GTK_DIALOG(dialog));

        gtk_widget_destroy(dialog);

        return response;
}

/* Shows a dialog asking user to select a preset. If allow_new is TRUE, then
 * user can specify a new preset. In this case, allow_new will become TRUE if
 * user selects a new preset, or FALSE if she is selecting an already
 * existent */
static xmlDoc *dialog_choose_preset(gpointer parent, gboolean *allow_new)
{
        GList *presets;
        GList *node;
        GtkWidget *selector;
        GtkWidget *dialog;
        xmlDoc *preset;
        xmlDoc *preset_selected = NULL;
        gchar *preset_name_selected;
        gint response;

        presets = presets_preload_all();
        if (!presets && !allow_new) {
                hildon_banner_show_information(GTK_WIDGET(parent),
                                               NULL,
                                               "No available presets");
                return NULL;
        }

        /* Check if user can specify a new preset */
        if (allow_new && *allow_new) {
                selector = hildon_touch_selector_entry_new_text();
        } else {
                selector = hildon_touch_selector_new_text();
        }

        node = presets;
        while (node) {
                preset = (xmlDoc *) node->data;
                hildon_touch_selector_append_text(
                        HILDON_TOUCH_SELECTOR(selector),
                        preset->name);
                node = g_list_next(node);
        }

        dialog = hildon_picker_dialog_new(parent);
        hildon_picker_dialog_set_selector(HILDON_PICKER_DIALOG(dialog),
                                          HILDON_TOUCH_SELECTOR(selector));
        response = gtk_dialog_run(GTK_DIALOG(dialog));

        if (response != GTK_RESPONSE_DELETE_EVENT) {
                preset_name_selected =
                        hildon_touch_selector_get_current_text(
                                HILDON_TOUCH_SELECTOR(selector));
        } else {
                preset_name_selected = NULL;
        }

        /* NOTE: There is a bug in hildon that prevent using gtk_widget_destroy
           when a hildon_touch_selector_entry is usedcd, as it segs fault.
           The bad part is that using hide_all makes a leak. */
        if (allow_new && *allow_new) {
                gtk_widget_hide_all(dialog);
        } else {
                gtk_widget_destroy(dialog);
        }

        /* Check the selected xmlDoc, freeing remaining */
        node = presets;
        while (node) {
                preset = (xmlDoc *) node->data;
                if (preset_name_selected &&
                    g_utf8_collate(preset->name, preset_name_selected) == 0) {
                        preset_selected = preset;
                } else {
                        xmlFreeDoc(preset);
                }
                node = g_list_next(node);
        }
        g_list_free(presets);

        if (preset_name_selected) {
                /* User asks for new presets */
                if (allow_new && *allow_new) {
                        if (preset_selected) {
                                *allow_new = FALSE;
                                g_free(preset_name_selected);
                        } else {
                                preset_selected = xmlNewDoc(BAD_CAST "1.0");
                                preset_selected->name = preset_name_selected;
                                *allow_new = TRUE;
                        }
                } else {
                        g_free(preset_name_selected);
                }
        }

        return preset_selected;
}

/* Callback invoked when user cliks on "Open" button */
static void open_button_cb(GtkToolButton *toolbutton, gpointer user_data)
{
        xmlDoc *preset;

        preset = dialog_choose_preset(user_data, NULL);

        if (preset) {
		preset_load(preset);
                xmlFreeDoc(preset);
        }
}

/* Callback invoked when user clicks in "Save as" button */
static void save_as_button_cb(GtkToolButton *toolbutton, gpointer user_data)
{
        xmlDoc *preset;
        gchar *preset_name;
        gchar *message;
        gboolean allow_new = TRUE;
        struct dialog_and_sliders *dialog_slid =
                (struct dialog_and_sliders *) user_data;

        preset = dialog_choose_preset(dialog_slid->dialog, &allow_new);

        if (preset) {
                /* Preset already exists */
                if (!allow_new) {
                        message =
                                g_strdup_printf("Do you want to overwrite %s "
                                                "preset?", preset->name);
                        if (dialog_confirm(dialog_slid->dialog, message) ==
                            GTK_RESPONSE_YES) {
                                preset_name = g_strdup(preset->name);
                                xmlFreeDoc(preset);
                                preset = xmlNewDoc(BAD_CAST "1.0");
                                preset->name = preset_name;
                                preset_save(preset, dialog_slid->slider_band);
                                xmlFreeDoc(preset);
                        } else {
                                xmlFreeDoc(preset);
                        }
                        g_free(message);
                } else {
                        /* Preset is new */
                        preset_save(preset, dialog_slid->slider_band);
                        xmlFreeDoc(preset);
                }
        }
}

/* Callback invoked when user clicks in "Delete" button */
static void delete_button_cb(GtkToolButton *toolbutton, gpointer user_data)
{
        xmlDoc *preset;
        gchar *preset_name;
        gchar *preset_fullname;
        gchar *message;

        preset = dialog_choose_preset(user_data, NULL);

        if (preset) {
                preset_name = g_strdup(preset->name);
                xmlFreeDoc(preset);

                /* Ask for confirmation */
                message = g_strdup_printf("Do you want to remove %s preset?",
                                          preset_name);
                if (dialog_confirm(user_data, message) == GTK_RESPONSE_YES) {
                        preset_fullname = g_strdup_printf("%s/%s", PRESETS_PATH,
                                                          preset_name);
                        g_unlink(preset_fullname);
                        g_free(preset_fullname);
                }
                g_free(message);
                g_free(preset_name);
        }
}

/*********** PUBLIC ***********/

osso_return_t
execute(osso_context_t *osso, gpointer data, gboolean user_activated)
{
        /* Create needed variables */
        GtkWidget *dialog;
        GtkObject *adj[NUM_BANDS];
        struct label_band *lband[NUM_BANDS];
        struct dialog_and_sliders *dialog_slid;
        GtkWidget *slider_band[NUM_BANDS];
        GtkWidget *single_slider_container[NUM_BANDS];
        gulong update_label_signal[NUM_BANDS];
        gulong update_band_signal[NUM_BANDS];
        GtkWidget *sliders_container;
        gint i;
        GtkWidget *toolbar;
        GtkToolItem *toolitem_open;
        GtkToolItem *toolitem_save_as;
        GtkToolItem *toolitem_delete;
        GtkWidget *content_area;
        guint update_slider_signal;


        dialog = gtk_dialog_new();
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_transient_for(GTK_WINDOW(dialog),
                                     GTK_WINDOW(data));
        gtk_window_set_title(GTK_WINDOW(dialog), "MAFW Equalizer");
        gtk_window_set_default_size(GTK_WINDOW(dialog), -1, 400);


        sliders_container = gtk_hbox_new(TRUE, 10);
        toolbar = gtk_toolbar_new();

        /* Create the bands */
        for (i = 0; i < NUM_BANDS; i++) {
                slider_band[i] = hildon_gtk_vscale_new();
                adj[i] = gtk_adjustment_new(EQ_GAIN_MIN, EQ_GAIN_MIN,
                                            EQ_GAIN_MAX, 1, 10, 0);
                gtk_range_set_adjustment(GTK_RANGE(slider_band[i]),
                                         GTK_ADJUSTMENT(adj[i]));

                gtk_range_set_inverted(GTK_RANGE(slider_band[i]), TRUE);
                gtk_range_set_update_policy(GTK_RANGE(slider_band[i]),
                                            GTK_UPDATE_DELAYED);
                gtk_range_set_show_fill_level(GTK_RANGE(slider_band[i]), FALSE);

                single_slider_container[i] = gtk_hbox_new(TRUE, 0);

                lband[i] = g_new0(struct label_band, 1);
                lband[i]->label = gtk_label_new(NULL);
                lband[i]->id = i;
                gtk_label_set_angle(GTK_LABEL(lband[i]->label), 90);
                gtk_misc_set_alignment(GTK_MISC(lband[i]->label), 0, 0.9);

                gtk_box_pack_start(GTK_BOX(single_slider_container[i]),
                                   lband[i]->label,
                                   FALSE, FALSE, 0);
                gtk_box_pack_start(GTK_BOX(single_slider_container[i]),
                                   slider_band[i],
                                   TRUE, TRUE, 0);

                gtk_box_pack_start(GTK_BOX(sliders_container),
                                   single_slider_container[i],
                                   TRUE,
                                   TRUE,
                                   10);

                update_label_signal[i] =
                        g_signal_connect(slider_band[i],
                                         "value-changed",
                                         G_CALLBACK(update_label_cb),
                                         lband[i]);

                gtk_range_set_value(GTK_RANGE(slider_band[i]),
                                    get_band_value(i));

                update_band_signal[i] =
                        g_signal_connect(slider_band[i],
                                         "value-changed",
                                         G_CALLBACK(update_band_cb),
                                         GINT_TO_POINTER(i));
        }

        /* Listen for changes in gconf */
        if (!confclient) {
                confclient = gconf_client_get_default();
        }

        gconf_client_add_dir(confclient, GCONF_MAFW_GST_EQ_RENDERER,
                             GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
        update_slider_signal =
                gconf_client_notify_add(
                        confclient,
                        GCONF_MAFW_GST_EQ_RENDERER,
                        (GConfClientNotifyFunc) update_slider_cb,
                        slider_band,
                        NULL, NULL);

        /* Create the toolbuttons */
        toolitem_open = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
        toolitem_save_as = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE_AS);
        toolitem_delete = gtk_tool_button_new_from_stock(GTK_STOCK_DELETE);
        gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem_open, -1);
        gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem_save_as, -1);
        gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem_delete, -1);

        g_signal_connect(toolitem_open, "clicked",
                         G_CALLBACK(open_button_cb),
                         dialog);

        g_signal_connect(toolitem_delete, "clicked",
                         G_CALLBACK(delete_button_cb),
                         dialog);

        dialog_slid = g_new0(struct dialog_and_sliders, 1);
        dialog_slid->dialog = dialog;
        dialog_slid->slider_band = slider_band;
        g_signal_connect(toolitem_save_as, "clicked",
                         G_CALLBACK(save_as_button_cb),
                         dialog_slid);

        content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_box_pack_start(GTK_BOX(content_area), sliders_container,
                           TRUE, TRUE, 1);
        gtk_box_pack_start(GTK_BOX(content_area), toolbar,
                           FALSE, FALSE, 1);

        /* Run the dialog */
        gtk_widget_show_all(GTK_WIDGET(dialog));
        gtk_dialog_run(GTK_DIALOG(dialog));

        /* Free everything */
        gconf_client_notify_remove(confclient, update_slider_signal);
        for (i = 0; i < NUM_BANDS; i++) {
                g_signal_handler_disconnect(slider_band[i],
                                            update_label_signal[i]);
                g_signal_handler_disconnect(slider_band[i],
                                            update_band_signal[i]);

                g_free(lband[i]);
        }
        g_free(dialog_slid);

        gtk_widget_destroy(GTK_WIDGET(dialog));

        return OSSO_OK;
}

osso_return_t
save_state(osso_context_t *osso, gpointer data)
{
        return OSSO_OK;
}

