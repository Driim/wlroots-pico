#define _POSIX_C_SOURCE 200112L
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <wayland-server.h>
#include <wlr/config.h>
#include <wlr/types/wlr_surface.h>
#include <wlr/util/log.h>
#include <phosh-mobile-shell-protocol.h>
#include "rootston/config.h"
#include "rootston/server.h"
#include "rootston/desktop.h"
#include "rootston/desktop-shell.h"

#if 0
static void
recalc_output_box (struct mobile_shell *shell)
{
  struct wlr_surface_state *state = shell->panel.wlr_surface->current;
  const struct wlr_box *output_box =
    wlr_output_layout_get_box(shell->desktop->layout, shell->panel.output);

  memcpy(&shell->panel.output_box, output_box, sizeof(struct wlr_box));
  wlr_trace ("x: %d, y: %d, width: %d, height: %d",
	     output_box->x,
	     output_box->y,
	     output_box->width,
	     output_box->height);

  switch (shell->panel.position) {
  case PHOSH_MOBILE_SHELL_PANEL_POSITION_TOP:
    shell->panel.output_box.y += state->height;
    shell->panel.output_box.height -= state->height;
    break;
  case PHOSH_MOBILE_SHELL_PANEL_POSITION_BOTTOM:
    shell->panel.y = output_box->height - state->height;
    shell->panel.output_box.height = output_box->height - state->height;
    break;
  case PHOSH_MOBILE_SHELL_PANEL_POSITION_LEFT:
    shell->panel.output_box.x += state->width;
    shell->panel.output_box.width -= state->width;
    break;
  case PHOSH_MOBILE_SHELL_PANEL_POSITION_RIGHT:
    shell->panel.x = output_box->width - state->width;
    shell->panel.output_box.width = output_box->width - state->width;
    break;
  }

  wlr_trace ("x: %d, y: %d, width: %d, height: %d",
	     shell->panel.output_box.x,
	     shell->panel.output_box.y,
	     shell->panel.output_box.width,
	     shell->panel.output_box.height);
}
#endif


static void
mobile_shell_rotate_display(struct wl_client *client,
			     struct wl_resource *resource,
			     struct wl_resource *surface_resource,
			     uint32_t degrees)
{
  struct mobile_shell *shell = wl_resource_get_user_data(resource);
  enum wl_output_transform transform = WL_OUTPUT_TRANSFORM_NORMAL;

  wlr_trace("rotation: %d", degrees);
  if (degrees % 90 != 0)
    wl_resource_post_error(resource,
			   PHOSH_MOBILE_SHELL_ERROR_INVALID_ARGUMENT,
			   "Can only rotate in 90 degree steps");
  degrees %= 360;

  switch (degrees) {
  case 0:
    transform =  WL_OUTPUT_TRANSFORM_NORMAL;
    break;
  case 90:
    transform = WL_OUTPUT_TRANSFORM_90;
    break;
  case 180:
    transform = WL_OUTPUT_TRANSFORM_180;
    break;
  case 270:
    transform = WL_OUTPUT_TRANSFORM_270;
    break;
  }
  wlr_output_set_transform(shell->panel.output, transform);

  const struct wlr_box *output_box =
    wlr_output_layout_get_box(shell->desktop->layout,
			      shell->panel.output);
  /* Tell the shell about the new size due to rotation */
  phosh_mobile_shell_send_configure(resource, 0,
				    surface_resource,
				    output_box->width,
				    output_box->height);

  recalc_output_box (shell);
  /* Tell notify surfaces about their new size */
  struct roots_view *view;
  wl_list_for_each(view, &shell->desktop->views, link) {
    if (view->maximized == true) {
      view_move_resize(view,
		       shell->panel.output_box.x,
		       shell->panel.output_box.y,
		       shell->panel.output_box.width,
		       shell->panel.output_box.height);
    }
  }
}


static void
unbind_mobile_shell(struct wl_resource *resource)
{
  struct mobile_shell *shell = wl_resource_get_user_data(resource);
  mobile_shell_destroy (shell);
}


static const struct phosh_mobile_shell_interface mobile_shell_impl = {
  mobile_shell_rotate_display,
};


static void
bind_mobile_shell(struct wl_client *client,
		   void *data, uint32_t version, uint32_t id)
{
  struct mobile_shell *shell = data;
  struct wl_resource *resource;

  resource = wl_resource_create(client, &phosh_mobile_shell_interface,
				1, id);

  /* FIXME: unsafe needs client == shell->child.client */
  if (true) {
    wlr_log(L_ERROR, "FIXME: allowing every client to bind as desktop-shell");
    wl_resource_set_implementation(resource,
				   &mobile_shell_impl,
				   shell, unbind_mobile_shell);
    shell->resource = resource;
    lockscreen_create(shell->desktop, shell->display);
    return;
  }

  wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
			 "permission to bind mobile_shell denied");
}


struct mobile_shell*
mobile_shell_create(struct roots_desktop *desktop, struct wl_display *display)
{
  struct mobile_shell *shell;

  shell = calloc(1, sizeof (struct mobile_shell));

  shell->desktop = desktop;
  shell->display = display;
  shell->ready = false;
  shell->panel.position = PHOSH_MOBILE_SHELL_PANEL_POSITION_TOP;
  shell->lock.locked = false;
  wl_list_init(&shell->menus);

  desktop->mobile_shell = shell;

  wlr_log(L_INFO, "Initializing desktop shell");
  if (wl_global_create(display,
		       &phosh_mobile_shell_interface, 1,
		       shell , bind_mobile_shell) == NULL)
    return NULL;
  return shell;
}
