#include <hildon-im-settings-plugin.h>
#include <hildon-im-languages.h>

#include <gconf/gconf-client.h>

#include <libintl.h>

#include "cptextinput.h"

struct vbox_tab
{
  GtkWidget *vbox;
  HildonIMSettingsCategory category;
};

struct plugin_widget
{
  GtkWidget *widget;
  gint weight;
};

static GSList *plugins;
static GSList *vbox_tabs;
static osso_context_t *g_osso;
static HildonIMSettingsPluginManager *settings_manager;

static gchar *selected_primary_language = NULL;
static gchar *selected_primary_language_full = NULL;
static gchar *selected_secondary_language = NULL;
static gchar *selected_secondary_language_full = NULL;

static GtkWidget *secondary_settings_button = NULL;

static HildonIMSettingsCategory applet_plugin_categories[] =
{
  HILDON_IM_SETTINGS_HARDWARE,
  HILDON_IM_SETTINGS_ONSCREEN,
  HILDON_IM_SETTINGS_LANGUAGE_GENERAL,
  HILDON_IM_SETTINGS_LANGUAGE_ADDITIONAL,
  HILDON_IM_SETTINGS_OTHER
};

static char *settings[] =
{
  HILDON_IM_GCONF_PRIMARY_LANGUAGE,
  HILDON_IM_GCONF_SECONDARY_LANGUAGE
};

static int
plugin_widgets_compare(const struct plugin_widget *a,
                       const struct plugin_widget *b)
{
  gint wa = a->weight;
  gint wb = b->weight;

  if (wa >= wb)
    return wa > wb;

  return -1;
}

static void
create_other_settings(GtkWidget *vbox)
{
  GSList *l;
  GSList *plugin_widgets = NULL;
  gint weight;

  for (l = plugins; l; l = l->next)
  {
    if (l->data)
    {
      HildonIMSettingsPluginInfo *info = l->data;
      GtkWidget *widget = hildon_im_settings_plugin_create_widget(
            info->plugin, HILDON_IM_SETTINGS_OTHER, NULL, &weight);

      if (widget)
      {
        struct plugin_widget *pw = g_new0(struct plugin_widget, 1);

        pw->widget = widget;
        pw->weight = weight;
        plugin_widgets = g_slist_prepend(plugin_widgets, pw);
      }
    }
  }

  plugin_widgets = g_slist_sort(plugin_widgets,
                                (GCompareFunc)plugin_widgets_compare);

  for (l = plugin_widgets; l; l = l->next)
  {
    struct plugin_widget *pw = l->data;

    gtk_box_pack_end(GTK_BOX(vbox), pw->widget, FALSE, FALSE, 0);
  }

  g_slist_free_full(plugin_widgets, g_free);
}

static GConfClient *
get_gconf_client()
{
  static GConfClient *gconf = NULL;

  if (!gconf)
    gconf = gconf_client_get_default();

  return gconf;
}

static gchar *
get_language_settings(guint language)
{
  GConfClient *gconf = get_gconf_client();

  if (language <= G_N_ELEMENTS(settings))
    return gconf_client_get_string(gconf, settings[language], NULL);

  return NULL;
}

static void
set_language_settings(guint language, const gchar *val)
{
  GConfClient *gconf = get_gconf_client();

  if (language <= G_N_ELEMENTS(settings))
  {
    if (!val)
      val = "";

    gconf_client_set_string(gconf, settings[language], val, NULL);
  }
}

static gint
compare_languages(const HildonIMLanguage *a, const HildonIMLanguage *b)
{
  return g_utf8_collate(a->description, b->description);
}

static void
picker_value_changed_cb(HildonPickerButton *button, gpointer data)
{
  HildonTouchSelector *selector = hildon_picker_button_get_selector(button);
  GtkTreeModel *model =
      hildon_touch_selector_get_model(HILDON_TOUCH_SELECTOR(selector), 0);
  GtkTreeIter iter;
  gchar *description;
  gchar *language_code;

  hildon_touch_selector_get_selected(selector, 0, &iter);
  gtk_tree_model_get(model, &iter,
                     1, &language_code,
                     0, &description,
                     -1);

  if (GPOINTER_TO_INT(data))
  {
    selected_secondary_language = language_code;
    selected_secondary_language_full = description;

    if (language_code)
    {
      hildon_im_settings_plugin_manager_set_internal_value(
            settings_manager, G_TYPE_STRING,
            HILDON_IM_SETTINGS_SECONDARY_LANGUAGE, language_code);
      gtk_widget_show(secondary_settings_button);
    }
    else
    {
      hildon_im_settings_plugin_manager_unset_internal_value(
            settings_manager, HILDON_IM_SETTINGS_SECONDARY_LANGUAGE);
      gtk_widget_hide(secondary_settings_button);
    }
  }
  else
  {
    selected_primary_language_full = description;
    selected_primary_language = language_code;
    hildon_im_settings_plugin_manager_set_internal_value(
          settings_manager, G_TYPE_STRING, HILDON_IM_SETTINGS_PRIMARY_LANGUAGE,
          language_code);
  }
}

static GtkWidget *
create_language_widget(gint language)
{
  gchar *selected_language;
  GSList *widgets;
  GtkWidget *top_plugin_widget = NULL;
  GSList *l;
  const char *msgid;
  GSList *available_languages;
  gchar *active_language;
  HildonTouchSelectorColumn *col;
  GtkWidget *event_box;
  GtkWidget *picker;
  GtkWidget *touch_selector;
  GtkWidget *hbox;
  GtkTreeIter iter;
  gint weight;

  picker = hildon_picker_button_new(HILDON_SIZE_FINGER_HEIGHT,
                                    HILDON_BUTTON_ARRANGEMENT_VERTICAL);
  hildon_button_set_alignment(HILDON_BUTTON(picker), 0.0, 0.5, 1.0, 0.0);

  if (language)
    selected_language = selected_secondary_language;
  else
    selected_language = selected_primary_language;

  hildon_im_settings_plugin_manager_set_internal_value(
        settings_manager, G_TYPE_STRING, "SelectedLanguage", selected_language);

  widgets = 0;

  for (l = plugins; l; l = l->next)
  {
    if (l->data)
    {
      HildonIMSettingsPluginInfo *info = l->data;
      HildonIMSettingsCategory where =
          language ? HILDON_IM_SETTINGS_SECONDARY_LANGUAGE_SETTINGS_WIDGET :
                     HILDON_IM_SETTINGS_PRIMARY_LANGUAGE_SETTINGS_WIDGET;
      GtkWidget *widget = hildon_im_settings_plugin_create_widget(
            info->plugin, where, NULL, &weight);

      if (widget)
      {
        struct plugin_widget *pw = g_new0(struct plugin_widget, 1);

        pw->widget = widget;
        pw->weight = weight;
        widgets = g_slist_prepend(widgets, pw);
      }
    }
  }

  widgets = g_slist_sort(widgets, (GCompareFunc)plugin_widgets_compare);

  for (l = widgets; l; l = l->next)
  {
    struct plugin_widget *pw = l->data;

    top_plugin_widget = GTK_WIDGET(pw->widget);

    if (top_plugin_widget)
      break;
  }

  g_slist_free_full(widgets, g_free);

  hbox = gtk_hbox_new(TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), picker, TRUE, TRUE, 0);

  if (language)
  {
    event_box = gtk_event_box_new();

    if (top_plugin_widget)
    {
      GTK_WIDGET_SET_FLAGS (top_plugin_widget, GTK_NO_SHOW_ALL);
      gtk_container_add(GTK_CONTAINER(event_box), top_plugin_widget);
    }

    gtk_box_pack_start(GTK_BOX(hbox), event_box, TRUE, TRUE, 0);
    secondary_settings_button = top_plugin_widget;
    msgid = "tein_fi_secondary_language";
  }
  else
  {
    if (top_plugin_widget)
      gtk_box_pack_start(GTK_BOX(hbox), top_plugin_widget, TRUE, TRUE, 0);

    msgid = "tein_fi_primary_language";
  }

  hildon_button_set_title(HILDON_BUTTON(picker),
                          dgettext("osso-applet-textinput", msgid));
  touch_selector = hildon_touch_selector_new();
  available_languages = hildon_im_get_available_languages();

  if (available_languages)
  {
    GtkListStore *list_store =
        gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    GtkTreeIter *selected_iter = NULL;

    available_languages = g_slist_sort(available_languages,
                                       (GCompareFunc)compare_languages);
    active_language = get_language_settings(language);

    for (l = available_languages; l; l = l->next)
    {
      HildonIMLanguage *lang = l->data;

      gtk_list_store_append(list_store, &iter);
      gtk_list_store_set(list_store, &iter,
                         0, lang->description,
                         1, lang->language_code,
                         -1);

      if (!selected_iter &&
          !g_ascii_strcasecmp(lang->language_code, active_language))
      {
        selected_iter = gtk_tree_iter_copy(&iter);

        if (language)
          selected_secondary_language = lang->language_code;
        else
          selected_primary_language = lang->language_code;
      }
    }

    hildon_im_free_available_languages(available_languages);

    if (language > 0)
    {
      gtk_list_store_append(list_store, &iter);
      gtk_list_store_set(list_store, &iter,
                         0, dgettext("osso-applet-textinput",
                                     "tein_fi_not_in_use"),
                         1, NULL,
                         -1);

      if (selected_iter)
        gtk_widget_show(secondary_settings_button);
      else
      {
        selected_iter = gtk_tree_iter_copy(&iter);
        gtk_widget_hide(secondary_settings_button);
      }
    }

    if (active_language)
      g_free(active_language);

    col = hildon_touch_selector_append_text_column(
          HILDON_TOUCH_SELECTOR(touch_selector), GTK_TREE_MODEL(list_store), 1);

    g_object_set(G_OBJECT(col), "text-column", 0, NULL);
    g_signal_connect(G_OBJECT(picker), "value-changed",
                     G_CALLBACK(picker_value_changed_cb),
                     GINT_TO_POINTER(language));

    if (selected_iter)
    {
      hildon_touch_selector_select_iter(HILDON_TOUCH_SELECTOR(touch_selector),
                                        0, selected_iter, 1);
      gtk_tree_iter_free(selected_iter);
    }

    hildon_picker_button_set_selector(HILDON_PICKER_BUTTON(picker),
                                      HILDON_TOUCH_SELECTOR(touch_selector));
  }

  return hbox;
}

void
create_main_dialog(GtkWindow *parent, osso_context_t *osso)
{
  int i;
  GtkWidget *dialog;
  GSList *plugin;
  GtkWidget *area;
  GtkWidget *vbox;
  gint weight;

  settings_manager = hildon_im_settings_plugin_manager_new();
  hildon_im_settings_plugin_manager_set_context(settings_manager, osso);

  vbox_tabs = NULL;

  area = hildon_pannable_area_new();
  g_object_set(G_OBJECT(area), "mov-mode", HILDON_MOVEMENT_MODE_VERT, NULL);
  gtk_widget_set_size_request(GTK_WIDGET(area), -1, 360);

  vbox = gtk_vbox_new(FALSE, 0);
  hildon_pannable_area_add_with_viewport(HILDON_PANNABLE_AREA(area), vbox);

  for (i = 0; i < G_N_ELEMENTS(applet_plugin_categories); i++)
  {
    struct vbox_tab *tab = g_new0(struct vbox_tab, 1);

    tab->vbox = gtk_vbox_new(FALSE, 0);
    tab->category = applet_plugin_categories[i];
    vbox_tabs = g_slist_append(vbox_tabs, tab);
    gtk_box_pack_start(GTK_BOX(vbox), tab->vbox, FALSE, FALSE, 0);
  }

  g_osso = osso;

  if (hildon_im_settings_plugin_manager_load_plugins(settings_manager))
  {
    GSList *tab = vbox_tabs;
    GSList *l;

    plugins = hildon_im_settings_plugin_manager_get_plugins(settings_manager);

    for (tab = vbox_tabs; tab; tab = tab->next)
    {
      struct vbox_tab *vtab = tab->data;
      GSList *plugin_widgets = NULL;
      GtkSizeGroup *size_group =
          GTK_SIZE_GROUP(gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL));

      if (vtab->category == HILDON_IM_SETTINGS_LANGUAGE_ADDITIONAL)
      {
        selected_secondary_language = NULL;
        selected_primary_language = NULL;
        gtk_box_pack_start(GTK_BOX(vtab->vbox), create_language_widget(0),
                           FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vtab->vbox), create_language_widget(1),
                           FALSE, FALSE, 0);
      }

      for (plugin = plugins; plugin; plugin = plugin->next)
      {
        if (plugin->data)
        {
          HildonIMSettingsPluginInfo *info = plugin->data;
          GtkWidget *widget = hildon_im_settings_plugin_create_widget(
                info->plugin, vtab->category, size_group, &weight);

          if (widget)
          {
            struct plugin_widget *pw = g_new0(struct plugin_widget, 1);

            pw->widget = widget;
            pw->weight = weight;
            plugin_widgets = g_slist_prepend(plugin_widgets, pw);
          }
        }
      }

      g_object_unref (G_OBJECT (size_group));

      plugin_widgets = g_slist_sort(plugin_widgets,
                                    (GCompareFunc)plugin_widgets_compare);

      for (l = plugin_widgets; l; l = l->next)
      {
        struct plugin_widget *pw = l->data;

        gtk_box_pack_start(GTK_BOX(vtab->vbox), pw->widget, FALSE, FALSE, 0);
      }

      g_slist_free_full(plugin_widgets, g_free);
    }

    create_other_settings(vbox);
  }
  else
    g_warning("Unable to load plugins");


  dialog = gtk_dialog_new_with_buttons(
        dgettext("osso-applet-textinput", "tein_ti_text_input_title"),
        GTK_WINDOW(parent),
        GTK_DIALOG_NO_SEPARATOR|GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_MODAL,
        0);

  gtk_dialog_add_button(GTK_DIALOG(dialog),
                        dgettext("hildon-libs", "wdgt_bd_save"),
                        GTK_RESPONSE_OK);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), area);
  gtk_widget_show_all(dialog);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
  {
    GSList *l;

    gtk_widget_hide(dialog);

    for (l = plugins ; l; l = l->next)
    {
      if (l->data)
      {
        hildon_im_settings_plugin_save_data(
            ((HildonIMSettingsPluginInfo *)l->data)->plugin, 0);
      }
    }

    set_language_settings(0, selected_primary_language);
    set_language_settings(1, selected_secondary_language);
  }

  gtk_widget_destroy(dialog);
  hildon_im_settings_plugin_manager_destroy(settings_manager);
}
