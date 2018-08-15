#pragma once
#include <wlr/types/wlr_layer_shell.h>

struct phosh_private {
	struct wl_resource* resource;
	struct wl_global *global;

	struct roots_desktop *desktop;
	struct {
		struct wl_listener layer_shell_new_surface;
		struct wl_listener panel_surface_destroy;
	} listeners;
	struct wlr_layer_surface *panel;
};


struct phosh_private* phosh_create(struct roots_desktop *desktop, struct wl_display *display);
void phosh_destroy(struct phosh_private *shell);
struct phosh_private *phosh_private_from_resource(struct wl_resource *resource);
