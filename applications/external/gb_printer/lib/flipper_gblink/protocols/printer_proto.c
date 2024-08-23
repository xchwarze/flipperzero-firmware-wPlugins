#include <furi.h>

#include <gblink.h>
#include <protocols/printer_proto.h>
#include "printer_i.h"

static int32_t printer_callback_thread(void *context)
{
	struct printer_proto *printer = context;
	uint32_t flags;

	while (1) {
		/* XXX: TODO: align flags and enum cb_reason to share them */
		flags = furi_thread_flags_wait(THREAD_FLAGS_ALL, FuriFlagWaitAny, FuriWaitForever);
		furi_check(!(flags & FuriFlagError));
		if (flags & THREAD_FLAGS_EXIT)
			break;
		if (flags & THREAD_FLAGS_DATA)
			printer->callback(printer->cb_context, printer->data, printer->data_sz, reason_data);
		if (flags & THREAD_FLAGS_PRINT)
			printer->callback(printer->cb_context, printer->data, printer->data_sz, reason_print);
	}

	return 0;
}

void *printer_alloc(void *gblink_handle, void **buf)
{
	furi_assert(buf);
	struct printer_proto *printer = NULL;

	printer = malloc(sizeof(struct printer_proto));

	/* Allocate and start callback handling thread */
	printer->thread = furi_thread_alloc_ex("GBLinkPrinterProtoCB",
						1024,
						printer_callback_thread,
						printer);
	/* Highest priority to ensure it runs ASAP */
	furi_thread_set_priority(printer->thread, FuriThreadPriorityHighest);
	furi_thread_start(printer->thread);

	printer->packet = malloc(sizeof(struct packet));
	printer->data = malloc(PRINT_FULL_SZ);
	*buf = malloc(PRINT_FULL_SZ);

	/* Create a new handle.
	 * We don't want higher layers clobbering the settings we need to use,
	 * but we do need the pin setting from it.
	 */
	printer->gblink_handle = gblink_alloc();
	gblink_pin_set(printer->gblink_handle, PIN_SERIN,
			gblink_pin_get(gblink_handle, PIN_SERIN));
	gblink_pin_set(printer->gblink_handle, PIN_SEROUT,
			gblink_pin_get(gblink_handle, PIN_SEROUT));
	gblink_pin_set(printer->gblink_handle, PIN_CLK,
			gblink_pin_get(gblink_handle, PIN_CLK));
	gblink_pin_set(printer->gblink_handle, PIN_SD,
			gblink_pin_get(gblink_handle, PIN_SD));

	/* Set up some settings for the print protocol. The final send/receive() calls
	 * may clobber some of these, but that is intentional and they don't need to
	 * care about some of the other details that are specified here.
	 */
	/* Reported 1.49 ms timeout between bytes, need confirmation */
	gblink_timeout_set(printer->gblink_handle, 1490);
	gblink_nobyte_set(printer->gblink_handle, 0x00);

	return printer;
}

void printer_free(void *printer_handle)
{
	struct printer_proto *printer = printer_handle;

	furi_thread_flags_set(printer->thread, THREAD_FLAGS_EXIT);
	furi_thread_join(printer->thread);
	furi_thread_free(printer->thread);
	gblink_free(printer->gblink_handle);
	free(printer->data);
	free(printer->packet);
	free(printer);
}

void printer_callback_context_set(void *printer_handle, void *context)
{
	struct printer_proto *printer = printer_handle;

	printer->cb_context = context;
}

void printer_callback_set(void *printer_handle, void (*callback)(void *context, void *buf, size_t len, enum cb_reason reason))
{
	struct printer_proto *printer = printer_handle;

	printer->callback = callback;
}

void printer_stop(void *printer_handle)
{
	struct printer_proto *printer = printer_handle;

	gblink_stop(printer->gblink_handle);
	/* TODO: Call the callback one last time with a flag to indicate that the transfer has completely
	 * ended.
	 * Receive/send should also have a separate timeout, doesn't need to call stop, but, will
	 * also retrigger the complete callback. This allows for both the actual process to signal
	 * there was a gap (I think the gameboy print normally has a "I'm done" marker as well),
	 * and then the actual application that started the send/receive, can catch a back or other
	 * nav event, call stop itself, which will then call the callback again with a "we're done here"
	 * message as well.
	 */
	 
	/* TODO: Figure out what mode we're in, and run stop. Though, it might
	 * not be necessary to actually to know the mode. We should be able to
	 * just stop?
	 */
}

