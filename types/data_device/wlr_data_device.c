#define _XOPEN_SOURCE 700
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <wayland-server.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/util/log.h>
#include "types/wlr_data_device.h"
#include "util/signal.h"

static const struct wl_data_device_interface data_device_impl;

static struct wlr_seat_client *seat_client_from_data_device_resource(
		struct wl_resource *resource) {
	assert(wl_resource_instance_of(resource, &wl_data_device_interface,
		&data_device_impl));
	return wl_resource_get_user_data(resource);
}

static void data_device_set_selection(struct wl_client *client,
		struct wl_resource *device_resource,
		struct wl_resource *source_resource, uint32_t serial) {
	struct wlr_client_data_source *source = NULL;
	if (source_resource != NULL) {
		source = client_data_source_from_resource(source_resource);
	}

	struct wlr_seat_client *seat_client =
		seat_client_from_data_device_resource(device_resource);

	struct wlr_data_source *wlr_source = (struct wlr_data_source *)source;
	wlr_seat_set_selection(seat_client->seat, wlr_source, serial);
}

static void data_device_start_drag(struct wl_client *client,
		struct wl_resource *device_resource,
		struct wl_resource *source_resource,
		struct wl_resource *origin_resource, struct wl_resource *icon_resource,
		uint32_t serial) {
	struct wlr_seat_client *seat_client =
		seat_client_from_data_device_resource(device_resource);
	struct wlr_surface *origin = wlr_surface_from_resource(origin_resource);
	struct wlr_data_source *source = NULL;
	struct wlr_surface *icon = NULL;

	if (source_resource) {
		struct wlr_client_data_source *client_source =
			client_data_source_from_resource(source_resource);
		source = (struct wlr_data_source *)client_source;
	}

	if (icon_resource) {
		icon = wlr_surface_from_resource(icon_resource);
	}
	if (icon) {
		if (wlr_surface_set_role(icon, "wl_data_device-icon",
					icon_resource, WL_DATA_DEVICE_ERROR_ROLE) < 0) {
			return;
		}
	}

	if (!seat_client_start_drag(seat_client, source, icon, origin, serial)) {
		wl_resource_post_no_memory(device_resource);
		return;
	}

	if (source) {
		source->seat_client = seat_client;
	}
}

static void data_device_release(struct wl_client *client,
		struct wl_resource *resource) {
	wl_resource_destroy(resource);
}

static const struct wl_data_device_interface data_device_impl = {
	.start_drag = data_device_start_drag,
	.set_selection = data_device_set_selection,
	.release = data_device_release,
};

static void data_device_destroy(struct wl_resource *resource) {
	wl_list_remove(wl_resource_get_link(resource));
}


void wlr_seat_client_send_selection(struct wlr_seat_client *seat_client) {
	if (wl_list_empty(&seat_client->data_devices)) {
		return;
	}

	if (seat_client->seat->selection_source) {
		struct wlr_data_offer *offer = data_source_send_offer(
			seat_client->seat->selection_source, seat_client);
		if (offer == NULL) {
			return;
		}

		struct wl_resource *resource;
		wl_resource_for_each(resource, &seat_client->data_devices) {
			wl_data_device_send_selection(resource, offer->resource);
		}
	} else {
		struct wl_resource *resource;
		wl_resource_for_each(resource, &seat_client->data_devices) {
			wl_data_device_send_selection(resource, NULL);
		}
	}
}

static void seat_client_selection_source_destroy(
		struct wl_listener *listener, void *data) {
	struct wlr_seat *seat =
		wl_container_of(listener, seat, selection_source_destroy);
	struct wlr_seat_client *seat_client = seat->keyboard_state.focused_client;

	if (seat_client && seat->keyboard_state.focused_surface) {
		struct wl_resource *resource;
		wl_resource_for_each(resource, &seat_client->data_devices) {
			wl_data_device_send_selection(resource, NULL);
		}
	}

	seat->selection_source = NULL;

	wlr_signal_emit_safe(&seat->events.selection, seat);
}

void wlr_seat_set_selection(struct wlr_seat *seat,
		struct wlr_data_source *source, uint32_t serial) {
	if (seat->selection_source &&
			seat->selection_serial - serial < UINT32_MAX / 2) {
		return;
	}

	if (seat->selection_source) {
		wl_list_remove(&seat->selection_source_destroy.link);
		wlr_data_source_cancel(seat->selection_source);
		seat->selection_source = NULL;
	}

	seat->selection_source = source;
	seat->selection_serial = serial;

	struct wlr_seat_client *focused_client =
		seat->keyboard_state.focused_client;

	if (focused_client) {
		wlr_seat_client_send_selection(focused_client);
	}

	wlr_signal_emit_safe(&seat->events.selection, seat);

	if (source) {
		seat->selection_source_destroy.notify =
			seat_client_selection_source_destroy;
		wl_signal_add(&source->events.destroy,
			&seat->selection_source_destroy);
	}
}


static void data_device_manager_get_data_device(struct wl_client *client,
		struct wl_resource *manager_resource, uint32_t id,
		struct wl_resource *seat_resource) {
	struct wlr_seat_client *seat_client =
		wlr_seat_client_from_resource(seat_resource);

	struct wl_resource *resource = wl_resource_create(client,
		&wl_data_device_interface, wl_resource_get_version(manager_resource),
		id);
	if (resource == NULL) {
		wl_resource_post_no_memory(manager_resource);
		return;
	}
	wl_resource_set_implementation(resource, &data_device_impl, seat_client,
		&data_device_destroy);
	wl_list_insert(&seat_client->data_devices, wl_resource_get_link(resource));
}

static void data_device_manager_create_data_source(struct wl_client *client,
		struct wl_resource *resource, uint32_t id) {
	client_data_source_create(client, wl_resource_get_version(resource), id);
}

static const struct wl_data_device_manager_interface
		data_device_manager_impl = {
	.create_data_source = data_device_manager_create_data_source,
	.get_data_device = data_device_manager_get_data_device,
};

static void data_device_manager_bind(struct wl_client *client,
		void *data, uint32_t version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
		&wl_data_device_manager_interface,
		version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &data_device_manager_impl,
		NULL, NULL);
}

void wlr_data_device_manager_destroy(struct wlr_data_device_manager *manager) {
	if (!manager) {
		return;
	}
	wl_list_remove(&manager->display_destroy.link);
	// TODO: free wl_resources
	wl_global_destroy(manager->global);
	free(manager);
}

static void handle_display_destroy(struct wl_listener *listener, void *data) {
	struct wlr_data_device_manager *manager =
		wl_container_of(listener, manager, display_destroy);
	wlr_data_device_manager_destroy(manager);
}

struct wlr_data_device_manager *wlr_data_device_manager_create(
		struct wl_display *display) {
	struct wlr_data_device_manager *manager =
		calloc(1, sizeof(struct wlr_data_device_manager));
	if (manager == NULL) {
		wlr_log(L_ERROR, "could not create data device manager");
		return NULL;
	}

	manager->global =
		wl_global_create(display, &wl_data_device_manager_interface,
			3, NULL, data_device_manager_bind);
	if (!manager->global) {
		wlr_log(L_ERROR, "could not create data device manager wl global");
		free(manager);
		return NULL;
	}

	manager->display_destroy.notify = handle_display_destroy;
	wl_display_add_destroy_listener(display, &manager->display_destroy);

	return manager;
}
