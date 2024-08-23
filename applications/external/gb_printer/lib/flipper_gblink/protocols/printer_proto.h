#ifndef PRINTER_PROTO_H
#define PRINTER_PROTO_H

#pragma once

enum cb_reason {
	reason_data,
	reason_print,
	reason_complete,
};

void *printer_alloc(void *gblink_handle, void **buf);

void printer_free(void *printer_handle);

void printer_callback_context_set(void *printer_handle, void *context);

void printer_callback_set(void *printer_handle, void (*callback)(void *context, void *buf, size_t len, enum cb_reason reason));

void printer_stop(void *printer_handle);

#endif // PRINTER_PROTO_H
