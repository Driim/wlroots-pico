#define _POSIX_C_SOURCE 200112L
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <wayland-server.h>
#include <wlr/config.h>
#include <wlr/types/wlr_surface.h>
#include <wlr/util/log.h>
#include <phosh-private-protocol.h>
#include "rootston/config.h"
#include "rootston/server.h"
#include "rootston/desktop.h"
#include "rootston/phosh.h"

static void phosh_rotate_display(struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *surface_resource,
	uint32_t degrees)
{
  struct phosh *shell = wl_resource_get_user_data(resource);
  enum wl_output_transform transform = WL_OUTPUT_TRANSFORM_NORMAL;

  wlr_log(WLR_DEBUG, "rotation: %d", degrees);
  if (degrees % 90 != 0) {
	  wl_resource_post_error(resource,
		  PHOSH_PRIVATE_ERROR_INVALID_ARGUMENT,
		  "Can only rotate in 90 degree steps");
  }
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

  wlr_output_set_transform(shell->panel->output, transform);
}


static void handle_phosh_layer_shell_surface(struct wl_listener *listener, void *data) {
	struct wlr_layer_surface *surface = data;
	struct phosh *phosh =
		wl_container_of(listener, phosh, layer_shell_surface);

	if (!strcmp(surface->namespace, "phosh")) {
		phosh->panel = surface;
	}
}

void phosh_destroy(struct phosh *shell) {
	shell->resource = NULL;
	wl_list_remove(&shell->layer_shell_surface.link);
}


static void unbind_phosh(struct wl_resource *resource) {
	struct phosh *shell = wl_resource_get_user_data(resource);
	phosh_destroy (shell);
}


static const struct phosh_private_interface phosh_impl = {
	phosh_rotate_display,
};


static void
bind_phosh(struct wl_client *client, void *data, uint32_t version, uint32_t id) {
	struct phosh *shell = data;
	struct wl_resource *resource  = wl_resource_create(client, &phosh_private_interface,
		1, id);

	/* FIXME: unsafe needs client == shell->child.client */
	if (true) {
		wlr_log(WLR_ERROR, "FIXME: allowing every client to bind as phosh");
		wl_resource_set_implementation(resource,
			&phosh_impl,
			shell, unbind_phosh);
		shell->resource = resource;
		return;
	}

	wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
		"permission to bind phosh denied");
}


struct phosh*
phosh_create(struct roots_desktop *desktop, struct wl_display *display) {
  struct phosh *shell;

  shell = calloc(1, sizeof (struct phosh));

  shell->desktop = desktop;
  shell->display = display;

  wl_signal_add(&desktop->layer_shell->events.new_surface,
	  &shell->layer_shell_surface);
  shell->layer_shell_surface.notify = handle_phosh_layer_shell_surface;

  desktop->phosh = shell;

  wlr_log(WLR_INFO, "Initializing mobile shell");
  if (wl_global_create(display, &phosh_private_interface, 1, shell , bind_phosh) == NULL) {
	  return NULL;
  }
  return shell;
}
