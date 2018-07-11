#include "cptextinput.h"

osso_return_t
execute(osso_context_t *osso, gpointer data, gboolean user_activated)
{
  create_main_dialog(data, osso);
  return 0;
}
