/* siicode
 * 
 * 2013-02-27 jeschke@fjes.de
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define BYTES_TO_WORD(x,y)          ((((int)y<<8)&0xff00) | (x&0xff))
#define BYTES_TO_DWORD(a,b,c,d)     ((unsigned int)(d&0xff)<<24)  | \
	                            ((unsigned int)(c&0xff)<<16) | \
				    ((unsigned int)(b&0xff)<<8)  | \
				     (unsigned int)(a&0xff)

enum eSection {
	SII_CAT_NOP
	,SII_PREAMBLE
	,SII_STD_CONFIG
	,SII_CAT_STRINGS = 10
	,SII_CAT_DATATYPES = 20 /* future use */
	,SII_CAT_GENERAL = 30
	,SII_CAT_FMMU = 40
	,SII_CAT_SYNCM = 41
	,SII_CAT_TXPDO = 50
	,SII_CAT_RXPDO = 51
	,SII_CAT_DCLOCK = 60/* future use */
	,SII_END = 0xffff
};

int read_eeprom(FILE *f, unsigned char *buffer, size_t size)
{
	size_t count = 0;
	int input;

	while ((input=getchar()) != EOF) {
		//printf("%.2x ", (unsigned char)(input&0xff));

		buffer[count++] = (unsigned char)(input&0xff);
	}

	return count;
}

void print_preamble(unsigned char *buffer, size_t size)
{
	size_t count = 0;

	printf("Preamble:\n");
	/* pdi control */
	int pdi_ctrl = ((int)buffer[1]<<8) | buffer[0];
	printf("PDI Control: %.4x\n", pdi_ctrl);

	/* pdi config */
	int pdi_conf = ((int)buffer[3]<<8) | buffer[2];
	printf("PDI config: %.4x\n", pdi_conf);

	count = 4;

	/* sync impulse len (multiples of 10ns) */
	int sync_imp = BYTES_TO_WORD(buffer[count++], buffer[count++]);
	printf("Sync Impulse length = %d ns (raw: %.4x)\n", sync_imp*10, sync_imp);

	/* PDI config 2 */
	int pdi_conf2 = BYTES_TO_WORD(buffer[count++], buffer[count++]);
	printf("PDI config 2: %.4x\n", pdi_conf2);

	/* configured station alias */
	printf("Configured station alias: %.4x\n", BYTES_TO_WORD(buffer[count++], buffer[count++]));

	count+=4; /* next 4 bytes are reserved */

	/* checksum FIXME add checksum test */
	printf("DEBUG: buffer[%u] buffer[%u] = %2x %2x\n", (unsigned int)count, (unsigned int)count+1, buffer[count+0], buffer[count+1]);
	/* FIXME wrong bytes are displayed! PANIC */
	int checksum = BYTES_TO_WORD(buffer[count++], buffer[count++]);
	printf("Checksum of preamble: %.4x\n", checksum);
	printf("DEBUG: buffer[%u] buffer[%u] = %2x %2x\n", (unsigned int)count-2, (unsigned int)count-1, buffer[count-2], buffer[count-1]);
}


#define MBOX_EOE    0x0002
#define MBOX_COE    0x0004
#define MBOX_FOE    0x0008
#define MBOX_SOE    0x0010
#define MBOX_VOE    0x0020

static void print_stdconfig(unsigned char *buffer, size_t size)
{
	size_t count =0;
	unsigned char *b = buffer;
	printf("General Information:\n");
	printf("Vendor ID: ....... 0x%08x\n", BYTES_TO_DWORD(*(b+0), *(b+1), *(b+2), *(b+3)));
	b+=4;
	printf("Product ID: ...... 0x%08x\n", BYTES_TO_DWORD(*(b+0), *(b+1), *(b+2), *(b+3)));
	b+=4;
	printf("Revision ID: ..... 0x%08x\n", BYTES_TO_DWORD(*(b+0), *(b+1), *(b+2), *(b+3)));
	b+=4;
	printf("Serial Number: ... 0x%08x\n", BYTES_TO_DWORD(*(b+0), *(b+1), *(b+2), *(b+3)));
	b+=4;

	b+=8; /* another reserved 8 bytes */

	/* mailbox settings */
	printf("\nDefault mailbox settings:\n");
	printf("Bootstrap received mailbox offset: 0x%04x\n", BYTES_TO_WORD(*(b+0), *(b+1)));
	b+=2;
	printf("Bootstrap received mailbox size:   %d\n", BYTES_TO_WORD(*(b+0), *(b+1)));
	b+=2;
	printf("Bootstrap send mailbox offset:     0x%04x\n", BYTES_TO_WORD(*(b+0), *(b+1)));
	b+=2;
	printf("Bootstrap send mailbox size:       %d\n", BYTES_TO_WORD(*(b+0), *(b+1)));
	b+=2;

	printf("Standard received mailbox offset:  0x%04x\n", BYTES_TO_WORD(*(b+0), *(b+1)));
	b+=2;
	printf("Standard received mailbox size:    %d\n", BYTES_TO_WORD(*(b+0), *(b+1)));
	b+=2;
	printf("Standard send mailbox offset:      0x%04x\n", BYTES_TO_WORD(*(b+0), *(b+1)));
	b+=2;
	printf("Standard send mailbox size:        %d\n", BYTES_TO_WORD(*(b+0), *(b+1)));
	b+=2;

	uint16_t recmbox = BYTES_TO_WORD(*(b+0), *(b+1));
	b+=2;
	printf("\nSupported Mailboxes: ");
	if (recmbox&MBOX_EOE)
		printf("EoE, ");
	if (recmbox&MBOX_COE)
		printf("CoE, ");
	if (recmbox&MBOX_FOE)
		printf("FoE, ");
	if (recmbox&MBOX_SOE)
		printf("SoE, ");
	if (recmbox&MBOX_VOE)
		printf("VoE, ");
	printf("\n");

	b+=66; /* more reserved bytes */

	printf("EEPROM size: %d kbit\n", BYTES_TO_WORD(*(b+0), *(b+1)));
	b+=2;
	printf("Version: %d\n",  BYTES_TO_WORD(*(b+0), *(b+1)));
	printf("\n");
}

static void print_stringsection(const unsigned char *buffer, size_t secsize)
{
	printf("+++ Ignoring String section\n");
}

static void print_datatype_section(const unsigned char *buffer, size_t secsize)
{
	printf("+++ datatypes section not yet implemented\n");
}

static void print_general_section(const unsigned char *buffer, size_t secsize)
{
	printf("+++ general section not yet implemented\n");
}

static void print_fmmu_section(const unsigned char *buffer, size_t secsize)
{
	int fmmunbr = 0;
	size_t count=0;
	const unsigned char *b = buffer;

	printf("\nFMMU Settings:\n");
	while ((b-buffer)<secsize) {
		printf("FMMU%d: ", fmmunbr++);
		switch (*b) {
		case 0x00:
		case 0xff:
			printf("not used\n");
			break;
		case 0x01:
			printf("used for Outputs\n");
			break;
		case 0x02:
			printf("used for Inputs\n");
			break;
		case 0x03:
			printf("used for SyncM status\n");
			break;
		default:
			printf("WARNING: undefined behavior\n");
			break;
		}
		b++;
	}
}

static void print_syncm_section(const unsigned char *buffer, size_t secsize)
{
	size_t count=0;
	int smnbr = 0;
	const unsigned char *b = buffer;

	while (count<secsize) {
		printf("\nSyncManager SM%d\n", smnbr);
		printf("  Physical Startaddress: 0x%04x\n", BYTES_TO_WORD(*b, *(b+1)));
		b+=2;
		printf("  Length: %d\n", BYTES_TO_WORD(*b, *(b+1)));
		b+=2;
		printf("  Control Register: 0x%02x\n", *b);
		b++;
		printf("  Status Register: 0x%02x\n", *b);
		b++;
		printf("  Enable byte: 0x%02x\n", *b);
		b++;
		printf("  SM Type: ");
		switch (*b) {
		case 0x00:
			printf("not used or unknown\n");
			break;
		case 0x01:
			printf("Mailbox Out\n");
			break;
		case 0x02:
			printf("Mailbox In\n");
			break;
		case 0x03:
			printf("Process Data Out\n");
			break;
		case 0x04:
			printf("Process Data In\n");
			break;
		default:
			printf("undefined\n");
			break;
		}
		b++;
		count=(size_t)(b-buffer);
		smnbr++;
	}
}

enum ePdoType {
	RxPDO,
	TxPDO
};

static void print_pdo_section(const unsigned char *buffer, size_t secsize, enum ePdoType t)
{
	const unsigned char *b = buffer;
	char *pdo;
	int pdonbr=0;
	int entries = 0;
	int entry = 0;

	switch (t) {
	case RxPDO:
		pdo = "RxPDO";
		break;
	case TxPDO:
		pdo = "TxPDO";
		break;
	default:
		pdo = "undefined";
		break;
	}

	printf("\n%s%d:\n", pdo, pdonbr);
	printf("  PDO Index: 0x%04x\n", BYTES_TO_WORD(*b, *(b+1)));
	b+=2;
	entries = *b;
	printf("  Entries: %d\n", entries);
	b++;
	printf("  SyncM: %d\n", *b);
	b++;
	printf("  Synchronization: 0x%02x\n", *b);
	b++;
	printf("  Name Index: %d\n", *b);
	b++;
	printf("  Flags for future use: 0x%04x\n", BYTES_TO_WORD(*b, *(b+1)));
	b+=2;

	while ((b-buffer)<secsize) {
		printf("\n    Entry %d:\n", entry);
		printf("    Entry Index: 0x%04x\n", BYTES_TO_WORD(*b, *(b+1)));
		b+=2;
		printf("    Subindex: 0x%02x\n", *b);
		b++;
		printf("    String Index: %d\n", *b);
		b++;
		printf("    Data Type: 0x%02x (Index in CoE Object Dictionary)\n", *b);
		b++;
		printf("    Bitlength: %d\n", *b);
		b++;
		printf("     Flags (for future use): 0x%04x\n", BYTES_TO_WORD(*b, *(b+1)));
		b+=2;

		entry++;
	}
}

static enum eSection get_next_section(const unsigned char *b, size_t len, size_t *secsize)
{
	enum eSection next = SII_CAT_NOP;

	next = BYTES_TO_WORD(*b, *(b+1)) & 0xffff; /* attention, on some section bit 7 is vendor specific indicator */
	unsigned wordsize = BYTES_TO_WORD(*(b+2), *(b+3));
	*secsize = (wordsize<<1); /* bytesize = 2*wordsize */

	return next;
}


static void print_dclock_section(const unsigned char *buffer, size_t secsize)
{
	const unsigned char *b = buffer+1; /* first byte is reserved */

	printf("\nDC Sync Parameter\n");

	printf("  Cyclic Operation Enable: %s\n", (*b & 0x01) == 0 ? "no" : "yes");
	printf("  SYNC0 activate: %s\n", (*b & 0x02) == 0 ? "no" : "yes");
	printf("  SYNC1 activate: %s\n", (*b & 0x04) == 0 ? "no" : "yes");
	b++; /* next 5 bit reserved */

	printf("  SYNC Pulse: %d (ns?)\n", BYTES_TO_WORD(*b, *(b+1)));
	b+=2;

	b+=10; /* skipped reserved */

	printf("  Interrupt 0 Status: %s\n",  (*b & 0x01) == 0 ? "not active" : "active");
	b++;
	printf("  Interrupt 1 Status: %s\n",  (*b & 0x01) == 0 ? "not active" : "active");
	b++;

	b+=12; /* skipped reserved */

	printf("  Cyclic Operation Startime: %d ns\n", BYTES_TO_DWORD(*b, *(b+1), *(b+2), *(b+3)));
	b+=4;
	printf("  SYNC0 Cycle Time: %d (ns?)\n", BYTES_TO_DWORD(*b, *(b+1), *(b+2), *(b+3)));
	b+=4;
	printf("  SYNC0 Cycle Time: %d (ns?)\n", BYTES_TO_DWORD(*b, *(b+1), *(b+2), *(b+3)));
	b+=4;

	printf("\nLatch Description\n");
	printf("  Latch 0 PosEdge: %s\n", (*b & 0x01) == 0 ? "continous" : "single");
	printf("  Latch 0 NegEdge: %s\n", (*b & 0x02) == 0 ? "continous" : "single");
	b+=2; /* the follwing 14 bits are reserved */

	printf("  Latch 1 PosEdge: %s\n", (*b & 0x01) == 0 ? "continous" : "single");
	printf("  Latch 1 NegEdge: %s\n", (*b & 0x02) == 0 ? "continous" : "single");
	b+=2; /* the follwing 14 bits are reserved */

	b+=4; /* another reserved block */

	printf("  Latch 0 PosEvnt: %s\n", (*b & 0x01) == 0 ? "no Event" : "Event stored");
	printf("  Latch 0 NegEvnt: %s\n", (*b & 0x02) == 0 ? "no Event" : "Event stored");
	b+=1; /* the follwing 14 bits are reserved */

	printf("  Latch 1 PosEvnt: %s\n", (*b & 0x01) == 0 ? "no Event" : "Event stored");
	printf("  Latch 1 NegEvnt: %s\n", (*b & 0x02) == 0 ? "no Event" : "Event stored");
	b+=1; /* the follwing 14 bits are reserved */

	printf("  Latch0PosEdgeValue: 0x%08x\n", BYTES_TO_DWORD(*b, *(b+1), *(b+2), *(b+3)));
	b+=4;
	b+=4;
	printf("  Latch0NegEdgeValue: 0x%08x\n", BYTES_TO_DWORD(*b, *(b+1), *(b+2), *(b+3)));
	b+=4;
	b+=4;
	printf("  Latch0PosEdgeValue: 0x%08x\n", BYTES_TO_DWORD(*b, *(b+1), *(b+2), *(b+3)));
	b+=4;
	b+=4;
	printf("  Latch0NegEdgeValue: 0x%08x\n", BYTES_TO_DWORD(*b, *(b+1), *(b+2), *(b+3)));
	b+=4;
	b+=4;
}

int main(int argc, char *argv[])
{
	/* FIXME if '-f' command line argument is available no read from stdin */

	enum eSection section = SII_PREAMBLE;
	size_t count = 0;
	unsigned char eeprom[1024];
	unsigned char *buffer;
	size_t secsize = 0;

	int input = 0;

	read_eeprom(stdin, eeprom, 1024);
	buffer = eeprom;

	while (1) {
		switch (section) {
		case SII_CAT_NOP:
			break;

		case SII_PREAMBLE:
			print_preamble(buffer, 16);
			buffer = eeprom+16;
			section = SII_STD_CONFIG;
			break;

		case SII_STD_CONFIG:
			printf("Print std config:\n");
			print_stdconfig(buffer, 46+66);
			buffer = buffer+46+66;
			section = get_next_section(buffer, 4, &secsize);
			buffer += 4;
			printf("+++ next section: %x\n", section);
			break;

		case SII_CAT_STRINGS:
			print_stringsection(buffer, secsize);
			buffer+=secsize;
			section = get_next_section(buffer, 4, &secsize);
			buffer+=4;
			printf("+++ next section: %x\n", section);
			break;

		case SII_CAT_DATATYPES:
			print_datatype_section(buffer, secsize);
			buffer+=secsize;
			section = get_next_section(buffer, 4, &secsize);
			buffer+=4;
			printf("+++ next section: %x\n", section);
			break;

		case SII_CAT_GENERAL:
			print_general_section(buffer, secsize);
			buffer+=secsize;
			section = get_next_section(buffer, 4, &secsize);
			buffer+=4;
			printf("+++ next section: %x\n", section);
			break;

		case SII_CAT_FMMU:
			print_fmmu_section(buffer, secsize);
			buffer+=secsize;
			section = get_next_section(buffer, 4, &secsize);
			buffer+=4;
			printf("+++ next section: %x\n", section);
			break;

		case SII_CAT_SYNCM:
			print_syncm_section(buffer, secsize);
			buffer+=secsize;
			section = get_next_section(buffer, 4, &secsize);
			buffer+=4;
			printf("+++ next section: %x\n", section);
			break;

		case SII_CAT_TXPDO:
			print_pdo_section(buffer, secsize, TxPDO);
			buffer+=secsize;
			section = get_next_section(buffer, 4, &secsize);
			buffer+=4;
			printf("+++ next section: %x\n", section);
			break;

		case SII_CAT_RXPDO:
			print_pdo_section(buffer, secsize, RxPDO);
			buffer+=secsize;
			section = get_next_section(buffer, 4, &secsize);
			buffer+=4;
			printf("+++ next section: %x\n", section);
			break;

		case SII_CAT_DCLOCK:
			print_dclock_section(buffer, secsize);
			buffer+=secsize;
			section = get_next_section(buffer, 4, &secsize);
			buffer+=4;
			printf("+++ next section: %x\n", section);
			break;

		case SII_END:
			printf("finished\n");
			goto finish;
			break;
		}
	}

finish:
	return 0;
}
