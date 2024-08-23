#ifndef PRINTER_I_H
#define PRINTER_I_H

#include <protocols/printer_proto.h>

#define PKT_START_L		0x88
#define PKT_START_H		0x33

#define PRINTER_ID		0x81
#define CMD_INIT		0x01
#define CMD_PRINT		0x02
#define CMD_TRANSFER		0x10
#define CMD_DATA		0x04
#define CMD_BREAK		0x08 // ??
#define CMD_STATUS		0x0f

#define STATUS_LOWBATT		(1 << 7)
#define STATUS_ERR		(1 << 6)
#define STATUS_JAM		(1 << 5)
#define STATUS_PKT_ERR		(1 << 4)
#define STATUS_READY		(1 << 3)
#define STATUS_FULL		(1 << 2)
#define STATUS_PRINTING		(1 << 1)
#define STATUS_CKSUM		(1 << 0)

/* emulate printer's internal print receive buffer */
#define TILE_SIZE		16 // 8x8 tile, 2bpp color
#define WIDTH			20 // 20 tiles wide
#define HEIGHT			18 // 18 tiles tall
#define PRINT_RECV_SZ		640 // (TILE_SIZE * WIDTH * 2)
#define PRINT_FULL_SZ		5760 // (PRINT_RECV_SZ * HEIGHT / 2)
#define TRANSFER_RECV_SZ	3584 // (16*16*14) // Image minus frame

//Note that TRANSFER uses a locked size, 16x14 tiles, 16*16*14

//GB seems to use 2 second busy timeout? I think that is a go to busy/printing within 2 seconds?
//20 second print timeout


typedef enum {
	START_L,
	START_H,
	COMMAND,
	COMPRESS,
	LEN_L,
	LEN_H,
	COMMAND_DAT,
	CKSUM_L,
	CKSUM_H,
	ALIVE,
	STATUS,
} printer_packet_state;

typedef enum {
	PROTO_RECV,
	PROTO_SEND,
} printer_proto;

/* Does not need to care about start bytes */
struct packet {
	uint8_t cmd;
	uint8_t compress;
	uint16_t len; // This is stored in the flipper endianness, arrives LSB first from GB, unmodified in code
	uint8_t recv_data[PRINT_RECV_SZ]; // 640 bytes, enough for two lines of tiles
	uint16_t cksum; // This is stored in the flipper endianness, arrives LSB first from GB
	
	/* These are not part of the packet, but used by us */
	uint16_t cksum_calc;
	size_t recv_data_sz;
	uint8_t status;
	bool printed;
};

#define THREAD_FLAGS_EXIT	(1 << 0)
#define THREAD_FLAGS_DATA	(1 << 1)
#define THREAD_FLAGS_PRINT	(1 << 2)
#define THREAD_FLAGS_ALL	(THREAD_FLAGS_EXIT | THREAD_FLAGS_DATA | THREAD_FLAGS_PRINT)

struct printer_proto {
	void *gblink_handle;

	void (*callback)(void *cb_context, void *buf, size_t len, enum cb_reason reason);
	void *cb_context;

	struct packet *packet; //packet data used by send()/receive() for tracking
	uint8_t *data; // Final resting place of image data
	size_t data_sz;

	printer_packet_state state;
	printer_proto proto;

	uint32_t time;

	FuriThread *thread;
};

#endif // PRINTER_I_H
