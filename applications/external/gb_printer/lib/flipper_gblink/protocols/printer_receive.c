#include <stdint.h>

#include <furi.h>

#include <gblink.h>
#include "printer_i.h"

#define TAG "printer_receive"

#define TIMEOUT_US 1000000

static void printer_reset(struct printer_proto *printer)
{
	printer->state = START_L;
	memset(printer->packet, '\0', sizeof(struct packet));

	printer->data_sz = 0;

	/* Packet timeout start */
	printer->time = DWT->CYCCNT;
}

static void byte_callback(void *context, uint8_t val)
{
	struct printer_proto *printer = context;
	size_t *recv_data_sz = &printer->packet->recv_data_sz;
	const uint32_t time_ticks = furi_hal_cortex_instructions_per_microsecond() * TIMEOUT_US;
	uint8_t data_out = 0x00;

	if ((DWT->CYCCNT - printer->time) > time_ticks)
		printer_reset(printer);

	/* TODO: flash led? */

	switch (printer->state) {
	case START_L:
		if (val == PKT_START_L) {
			printer->state = START_H;
			/* Packet timeout restart */
			printer->time = DWT->CYCCNT;
		}
		break;
	case START_H:
		if (val == PKT_START_H)
			printer->state = COMMAND;
		else
			printer->state = START_L;
		break;
	case COMMAND:
		printer->packet->cmd = val;
		printer->state = COMPRESS;
		printer->packet->cksum_calc += val;

		/* We only do a real reset after the packet is completed, however
		 * we need to clear the status flags at this point.
		 */
		if (val == CMD_INIT)
			printer->packet->status = 0;

		break;
	case COMPRESS:
		printer->packet->cksum_calc += val;
		printer->state = LEN_L;
		if (val) {
			FURI_LOG_E(TAG, "Compression not supported!");
			printer->packet->status |= STATUS_PKT_ERR;
		}
		break;
	case LEN_L:
		printer->packet->cksum_calc += val;
		printer->state = LEN_H;
		printer->packet->len = (val & 0xff);
		break;
	case LEN_H:
		printer->packet->cksum_calc += val;
		printer->packet->len |= ((val & 0xff) << 8);
		/* Override length for a TRANSFER */
		if (printer->packet->cmd == CMD_TRANSFER)
			printer->packet->len = TRANSFER_RECV_SZ;

		if (printer->packet->len) {
			printer->state = COMMAND_DAT;
		} else {
			printer->state = CKSUM_L;
			if (printer->packet->cmd == CMD_DATA)
				printer->packet->status |= STATUS_FULL;
		}
		break;
	case COMMAND_DAT:
		printer->packet->cksum_calc += val;
		printer->packet->recv_data[*recv_data_sz] = val;
		(*recv_data_sz)++;
		if (*recv_data_sz == printer->packet->len) 
			printer->state = CKSUM_L;
		break;
	case CKSUM_L:
		/* TODO: TRANSFER from Photo! does not use checksum for some reason */
		printer->state = CKSUM_H;
		printer->packet->cksum = (val & 0xff);
		break;
	case CKSUM_H:
		printer->state = ALIVE;
		printer->packet->cksum |= ((val & 0xff) << 8);
		if (printer->packet->cksum != printer->packet->cksum_calc)
			printer->packet->status |= STATUS_CKSUM;
		// TRANSFER does not set checksum bytes
		if (printer->packet->cmd == CMD_TRANSFER)
			printer->packet->status &= ~STATUS_CKSUM;
		data_out = PRINTER_ID;
		break;
	case ALIVE:
		printer->state = STATUS;
		data_out = printer->packet->status;
		break;
	case STATUS:
		printer->state = START_L;
		switch (printer->packet->cmd) {
		case CMD_INIT:
			printer_reset(printer);
			break;
		case CMD_DATA:
			/* READY is a measure of any valid data packets received */
			if (!(printer->packet->status & STATUS_CKSUM))
				printer->packet->status |= STATUS_READY;

			/* If there is space in the printer buffer, copy our data to it */
			if (printer->data_sz < PRINT_FULL_SZ) {
				if ((printer->data_sz + printer->packet->len) <= PRINT_FULL_SZ) {
					memcpy((printer->data)+printer->data_sz, printer->packet->recv_data, printer->packet->len);
					printer->data_sz += printer->packet->len;
				} else {
					memcpy((printer->data)+printer->data_sz, printer->packet->recv_data, ((printer->data_sz + printer->packet->len)) - PRINT_FULL_SZ);
					printer->data_sz += (PRINT_FULL_SZ - (printer->data_sz + printer->packet->len));
					furi_assert(printer->data_sz <= PRINT_FULL_SZ);
				}
			}
			furi_thread_flags_set(printer->thread, THREAD_FLAGS_DATA);
			break;
		case CMD_TRANSFER:
			/* XXX: TODO: Check to see if we're still printing when getting
			 * a transfer command. If so, then we have failed to beat the clock.
			 */
		case CMD_PRINT:
			printer->packet->status &= ~STATUS_READY;
			printer->packet->status |= STATUS_PRINTING;
			furi_thread_flags_set(printer->thread, THREAD_FLAGS_PRINT);
			break;
		case CMD_STATUS:
			/* READY cleared on status request */
			printer->packet->status &= ~STATUS_READY;
		}

		printer->packet->recv_data_sz = 0;
		printer->packet->cksum_calc = 0;


		/* XXX: TODO: if the command had something we need to do, do it here. */
		/* done! flush our buffers, deal with any status changes like
		 * not printing -> printing -> not printing, etc.
		 */
		/* Do a callback here?
		 * if so, I guess we should wait for callback completion before accepting more recv_data?
		 * but that means the callback is in an interrupt context, which, is probably okay?
		 */
		/* XXX: TODO: NOTE: FIXME:
		 * all of the notes..
		 * This module needs to maintain the whole buffer, but it can be safely assumed that the buffer
		 * will never exceed 20x18 tiles (no clue how many bytes) as that is the max the printer can
		 * take on in a single print. Printing mulitples needs a print, and then a second print with
		 * no margin. So the margins are important and need to be passed to the final application,
		 * SOMEHOW.
		 *
		 * More imporatntly, is the completed callback NEEDS to have a return value. This allows
		 * the end application to take that whole panel, however its laid out, and do whatever
		 * it wants to do with it. Write it to a file, convert, etc., etc., so that this module
		 * will forever return that it is printing until the callback returns true.
		 *
		 * Once we call the callback and it shows a true, then we can be sure the higher module
		 * is done with the buffer, and we can tell the host that yes, its done, you can continue
		 * if you want.
		 */
		/* XXX: On TRANSFER, there is no checking of status, it is only two packets in total.
		 * I can assume that if we delay a bit in moving the buffer around that should be okay
		 * but we probably don't want to wait too long.
		 * Upon testing, transfer seems to doesn't 
		 */
		break;
	default:
		FURI_LOG_E(TAG, "unknown status!");
		break;
	}

	/* transfer next byte */
	gblink_transfer(printer->gblink_handle, data_out);
}

void printer_receive(void *printer_handle)
{
	struct printer_proto *printer = printer_handle;

	/* Set up defaults the receive path needs */
	gblink_callback_set(printer->gblink_handle, byte_callback, printer);
	gblink_clk_source_set(printer->gblink_handle, GBLINK_CLK_EXT);

	printer->proto = PROTO_RECV;

	printer_reset(printer);

	gblink_start(printer->gblink_handle);
}

void printer_receive_print_complete(void *printer_handle)
{
	struct printer_proto *printer = printer_handle;

	printer->packet->status &= ~STATUS_PRINTING;
}
