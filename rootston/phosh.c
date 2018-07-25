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

static void phosh_rotate_output(struct wl_client *client,
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

  if (!shell->panel) {
	  wlr_log(WLR_ERROR, "Tried to rotate inexistent panel");
	  return;
  }

  wlr_output_set_transform(shell->panel->output, transform);
}


static void handle_phosh_panel_surface_destroy (struct wl_listener *listener, void *data) {
	struct phosh *phosh =
		wl_container_of(listener, phosh, listeners.panel_surface_destroy);

	if (phosh->panel) {
		phosh->panel = NULL;
		wl_list_remove(&phosh->listeners.panel_surface_destroy.link);
	}
}


static void handle_phosh_layer_shell_new_surface(struct wl_listener *listener, void *data) {
	struct wlr_layer_surface *surface = data;
	struct phosh *phosh =
		wl_container_of(listener, phosh, listeners.layer_shell_new_surface);

	/* We're only interested in the panel */
	if (strcmp(surface->namespace, "phosh"))
		return;

	phosh->panel = surface;
	wl_signal_add(&surface->events.destroy,
		&phosh->listeners.panel_surface_destroy);
	phosh->listeners.panel_surface_destroy.notify = handle_phosh_panel_surface_destroy;
}


static void phosh_resource_destroy(struct wl_resource *resource) {
	struct phosh *phosh = wl_resource_get_user_data(resource);

	phosh->resource = NULL;
	phosh->panel = NULL;
}


static const struct phosh_private_interface phosh_impl = {
	phosh_rotate_output,
};


static void
bind_phosh(struct wl_client *client, void *data, uint32_t version, uint32_t id) {
	struct phosh *phosh = data;
	struct wl_resource *resource  = wl_resource_create(client, &phosh_private_interface,
		1, id);

	if (phosh->resource) {
		wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
			"Only a single client can bind to phosh's private protocol");
	}

	/* FIXME: unsafe needs client == shell->child.client */
	if (true) {
		wlr_log(WLR_ERROR, "FIXME: allowing every client to bind as phosh");
		wl_resource_set_implementation(resource,
			&phosh_impl,
			phosh, phosh_resource_destroy);
		phosh->resource = resource;
		return;
	}

	wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
		"permission to bind phosh denied");
}


struct phosh*
phosh_create(struct roots_desktop *desktop, struct wl_display *display) {
  struct phosh *shell = calloc(1, sizeof (struct phosh));
  if (!shell)
	  return NULL;

  shell->desktop = desktop;

  wl_signal_add(&desktop->layer_shell->events.new_surface,
	  &shell->listeners.layer_shell_new_surface);
  shell->listeners.layer_shell_new_surface.notify = handle_phosh_layer_shell_new_surface;

  wlr_log(WLR_INFO, "Initializing phosh private inrerface");
  shell->global = wl_global_create(display, &phosh_private_interface, 1, shell, bind_phosh);
  if (!shell->global) {
	  return NULL;
  }

  return shell;
}


void phosh_destroy(struct phosh *shell) {
	wl_list_remove(&shell->listeners.layer_shell_new_surface.link);
	wl_global_destroy(shell->global);
}
