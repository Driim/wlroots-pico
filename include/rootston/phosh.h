#ifndef _ROOTSTON_PHOSH_H
#define _ROOTSTON_PHOSH_H

#include <wlr/types/wlr_layer_shell.h>

struct phosh {
	struct wl_resource* resource;
	struct wl_global *global;
	struct roots_desktop *desktop;
	struct {
		struct wl_listener layer_shell_new_surface;
		struct wl_listener panel_surface_destroy;
	} listeners;
	struct wlr_layer_surface *panel;
};

struct phosh* phosh_create(struct roots_desktop *desktop, struct wl_display *display);
void phosh_destroy(struct phosh *shell);
#endif
