#include <stdio.h>
#include <stdlib.h>
#include <time.h>


/* There is a distinct lack of error checking which should probably be fixed. */


/* UDP HEADER ADDER */

/* There are 4 components to a UDP header:
 * - 16 bit source port
 *   - Not really needed in OreSat.

 * - 16 bit destination port: should be set to broadcast (all 1s)
 *   - Note that if a single bit is accidentally set to 0, for example via
 *     a bit error on a noisy signal, then the destination port, instead
 *     of being interpreted as a broadcast packet, will be interpreted as
 *     an incorrect port, and will typically be discarded by the wifi
 *     adapter's firmware.

 * - 16 bit length field
 *   - Note that if a single bit is in error, the packet will be read
 *     incorrectly, and data loss/incorrect decoding is highly likely.
 *   - We will need some way of detecting when this kind of error has
 *     occured; possibly with our own additional field at the beginning
 *     of the data, with very high FEC, which contains the true length.
 *   - Note that actually, the length can be set to 0; this indicates
 *     that the packet is longer than the theoretical maximum.

 * - 16 bit checksum: if unused, should be set to all 0s
 *   - Note that if a single bit is accidentally set to 1, then it will be
 *     incorrectly interpreted as an incorrect checksum and discarded by
 *     the adapter's firmware.

 * THIS MEANS 48 BITS MUST BE PERFECT OR THE PACKET WILL BE LOST *

 * Reed-Solomon encoding is one way to recover from packet loss (see below)

 */

int addUDP(unsigned int length, FILE *out)
{
	// Check to make sure length is valid
	if (length > 65535)
		return -1;
	if (length > 1 && length < 8)
		return -1;
	// Cast length to chars
	unsigned char lengthLo = (unsigned char) length;
	unsigned char lengthHi = (unsigned char) (length >> 8);
	// Source port
	fputc(0xff, out);// value is arbitrary;
	fputc(0xff, out);
	// Destination port (broadcast)
	fputc(0xff, out);
	fputc(0xff, out);
	// Length
	fputc(lengthLo, out); //endian BS caused a bug here
	fputc(lengthHi, out);
	// Checksum (none)
	fputc(0x00, out);
	fputc(0x00, out);

	return 0;
}

// Interleave UDP packets with stream of data
// returns number of packet headers added
int inlvUDP(unsigned int length, FILE *in, FILE *out)
{
	int end = 0;
	int next = 0;
	unsigned char c = 0x00;
	int pcount = 0;

	// Check to make sure length is valid
	if (length > 65535)
		return -1;
	if (length > 1 && length < 8)
		return -1;

	while (!end)
	{
		addUDP(length, out);
		pcount++;
		for (unsigned int i = 0; i < length; i++)
		{
			if (next != EOF)
			{
				next = fgetc(in);
				if (next == EOF)
				{
					c = 0x00;
					end = 1;
				}
				else
				{
					c = (unsigned char) next;;
				}
			}
			fputc(c,out);
		}
	}
	return pcount;
}


// "decode" udp stream

// Function for simulating how a wifi adapter would interpret and drop packts
// For testing purposes, to show packet loss on final data
// Not necessarily following the same logic as an actual adapter
// In particular, if the length is wrong, this function just drops the packet

// pnum = number of packets added; plen = packet length
int decUDP(int pnum, unsigned int plen, FILE *in, FILE *out)
{
	unsigned char c;
	int pcounter = 0;
	int droppack;

	// because testing length by each byte
	unsigned char lengthLo = (unsigned char) plen;
	unsigned char lengthHi = (unsigned char) (plen >> 8);

	while (pcounter < pnum)
	{
		droppack = 0;
		// First, check packet header to see if dropped
		c = fgetc(in); // first is source port
		c = fgetc(in); // (each fgetc returns 8 bits)

		c = fgetc(in); // dest port
		if (c != 0xff)
			droppack = 1;
		c = fgetc(in); // dest port again
		if (c != 0xff)
			droppack = 1;

		c = fgetc(in); // length (tested by each byte)
		if (c != lengthLo)
			droppack = 1;
		c = fgetc(in);
		if (c != lengthHi)
			droppack = 1;

		c = fgetc(in); // checksum
		if (c != 0x00)
			droppack = 1;
		c = fgetc(in); // checksum, part 2
		if (c != 0x00)
			droppack = 1;

		// If so, write all 0s; else, write data
		if (droppack)
		{
			for (unsigned int i = 0; i < plen; i++)
			{
				c = fgetc(in); // still fgetc for count
				fputc(0x00, out);
			}
		}
		else
		{
			for (unsigned int i = 0; i < plen; i++)
			{
				c = fgetc(in);
				fputc(c, out);
			}
		}
		pcounter++;
	}
	return 0;
}



/* HAMMING ENCODER AND DECODER */

// Hamming (7,4)

int h74(FILE *in, FILE *out)
{




/*
 * c1-c4 are copies of source data; c5-c7 are parity checks (odd or even).
 * ie, the first bit in c5 will be parity of first bits in c1,c2,c4;
 * the second bit in c6 will check second bits in c1,c3,c4;
 * the fourth bit in c7 will check fourth bits in c2,c3,c4, etc.
 *
 * This hamming code follows a typical (7,4) hamming code logic.
 * However, since operations are done at the byte level (unsigned char),
 * it can be thougt of as 8 unique (7,4) codes calculated in parallel on
 * each individual bit position.
 *
 */

	unsigned char c1,c2,c3,c4,c5,c6,c7;
	int end = 0;

	while (!end) {

		// if end of file, flag it but continue to make codes;
		// else, put full character into cx.

		int c; // Wait to cast to character so we can read EOF correctly
		if (((c = fgetc(in)) == EOF)) {
			c1 = 0;
			end = 1;
		} else {
			c1 = (char) c;
		}
		if (((c = fgetc(in)) == EOF )) {
			c2 = 0;
			end = 1;
		} else {
			c2 = (char) c;
		}
		if (((c = fgetc(in)) == EOF)) {
			c3 = 0;
			end = 1;
		} else {
			c3 = (char) c;
		}
		if (((c = fgetc(in)) == EOF)) {
			c4 = 0;
			end = 1;
		} else {
			c4 = (char) c;
		}

		// parity bits (xor = mod_2)
		c5 = c1 ^ c2 ^ c4;
		c6 = c1 ^ c3 ^ c4;
		c7 = c2 ^ c3 ^ c4;


		fputc(c1,out);
		fputc(c2,out);
		fputc(c3,out);
		fputc(c4,out);
		fputc(c5,out);
		fputc(c6,out);
		fputc(c7,out);
	}
	return 0;
}



// Interleave hamming 7,4 into UDP packets to correct packet loss

// First call h74, then call inlvham
// returns total packets sent
int inlvham(int plen, FILE *in, FILE *out)
{
	int end = 0;
	int next = 0;
	unsigned char c = 0x00;
	int pcount = 0;

	unsigned char packets[7][plen];


	// Check to make sure length is valid
	if (plen > 65535)
		return -1;


	while (!end)
	{
		addUDP(plen, out);
		pcount += 7;

		// Note the order: packet position, then packet
		// because h74() was called first, this is the order implied

		// for each position in packet
		for (int i = 0; i < plen; i++)
		{
			// for each of the 7 packets currently in use
			for (int j = 0; j < 7; j++)
			{
				// make sure not end of file
				if (next != EOF)
				{
					next = fgetc(in);
					if (next == EOF)
					{
						c = 0x00;
						end = 1;
					}
					else
					{
						c = (unsigned char) next;;
					}
				}
				// write to buffer
				packets[j][i] = c;
			}
		}
		// then write them all to stream, in correct order
		for (int i = 0; i < 7; i++)
		{
			for (int j = 0; j < plen; j++)
			{
				fputc(packets[i][j],out);
			}
		}
	}
	return pcount;
}




/* Decoding the 7,4 hamming code.
 *
 * A basic linear search is used to find the syndrome, as
 * the matrices are not large enough to justify advanced techniques.
 */


/* Decoder (syndrome) matrix:
 *
 *            chars, left to right in order of reading
 *            (d1,d2,d3,d4,p1,p2,p3)
 * decoder 0: 1101100
 * decoder 1: 1011010
 * decoder 2: 0111001
 */

int d_h74(FILE *in, FILE *out) {

// Create matrix
unsigned char decoder[3][7];
	decoder[0][0] = 0xff;
	decoder[0][1] = 0xff;
	decoder[0][2] = 0x00;
	decoder[0][3] = 0xff;
	decoder[0][4] = 0xff;
	decoder[0][5] = 0x00;
	decoder[0][6] = 0x00;

	decoder[1][0] = 0xff;
	decoder[1][1] = 0x00;
	decoder[1][2] = 0xff;
	decoder[1][3] = 0xff;
	decoder[1][4] = 0x00;
	decoder[1][5] = 0xff;
	decoder[1][6] = 0x00;

	decoder[2][0] = 0x00;
	decoder[2][1] = 0xff;
	decoder[2][2] = 0xff;
	decoder[2][3] = 0xff;
	decoder[2][4] = 0x00;
	decoder[2][5] = 0x00;
	decoder[2][6] = 0xff;

int c;
unsigned char received[7];
int end = 0;

while (!end)
{
	unsigned char syndrome[3] = {0,0,0};
	for (int i = 0; i < 7; i++)
	{
		c = fgetc(in);
		if (c == EOF)
		{
			end = 1;
			received[i] = 0;
		}
		else
		{
			received[i] = (unsigned char) c;
			syndrome[0] ^= (received[i] & decoder[0][i]);
			syndrome[1] ^= (received[i] & decoder[1][i]);
			syndrome[2] ^= (received[i] & decoder[2][i]);
		}
	}
	// If syndrome is all 0s, go ahead and output message chars
	unsigned char synpos = syndrome[0] | syndrome[1] | syndrome[2]; //syndrome position
	if (synpos == 0)
	{
		fputc(received[0],out);
		fputc(received[1],out);
		fputc(received[2],out);
		fputc(received[3],out);
	}
	else // Otherwise, determine the syndrome and bit position
	{
		for (int i = 0; i < 8; i++) // Looping through each bit position
		{
			unsigned char mask = 0x01 << i; // Test each bit position
			if (mask & synpos)
				// If any bits are wrong at that position,
				// go through decoder array and match
				// syndrome with bit to flip
			{
				for (int j = 0; j < 7; j++)
				{
					// If the decoder array matches the syndrome
					// at the specified bit position.
					// Technically this is unnecessary for the
					// parity bits, so the loop could be changed
					// to j < 4 if desired.
					if ( ~(decoder[0][j] ^ syndrome[0]) & mask &&
					     ~(decoder[1][j] ^ syndrome[1]) & mask &&
					     ~(decoder[2][j] ^ syndrome[2]) & mask )
					{	// switch that bit
						received[j] ^= mask;
						break;
					}
				}
			}
		}
	fputc(received[0],out);
	fputc(received[1],out);
	fputc(received[2],out);
	fputc(received[3],out);
	}
}
return 0;
}


// de-interleave 7,4 hamming

int d_inlvham(int plen, FILE *in, FILE *out)
{
	int next = 0;
	int end = 0;
	unsigned char c = 0x00;
	unsigned char buff[7][plen];

	while (!end)
	{
		for (int i = 0; i < 7; i++)
		{
			for (int j = 0; j < plen; j++)
			{
				next = fgetc(in);
				if (next == EOF)
				{
					end = 1;
					c = 0x00;
				}
				else
				{
					c = (unsigned char) next;
				}
				buff[i][j] = c;
			}
		}
		for (int i = 0; i < plen; i++)
		{
			for (int j = 0; j < 7; j++)
			{
				fputc(buff[j][i],out);
			}
		}
	}
	printf("finished deinterleave\n");
	return 0;
}




/*---*/




/* REED SOLOMON FUNCTIONS */

// Helper function: Galois Field multiplication

// Naturally, I made this without realizing log multiplicaiton is more efficient.
// If encoding resources becomes a problem, definitely use log multiplication instead.

unsigned char mulGF(unsigned char poly1, unsigned char poly2, unsigned short power, unsigned short generator)
{
	// result will be temporarily larger than a char
	unsigned short p1 = (unsigned short) poly1;
	unsigned short p2 = (unsigned short) poly2;
	unsigned short result = 0;
	unsigned short mask;
	unsigned short gen = generator;

	/* Expanded multiplication with mod_2 addition */
	for (int i = 0; i < power; i++)
	{
		if (p2 & 0x01)
		{
			result ^= p1;
		}
		p2 >>= 1;
		p1 <<= 1;
		if (p2==0) break;
	}

	/* Reduction over generator */
	if (gen < result)
	{ // If result already less than gen, no need
		return result;
	}

	mask = 0x8000; // one bit set
	// 16 bits in unsigned short minus 9 bits for gen
	while (!(mask&result))
	{
		mask >>= 1;
		printf("1 result %x mask %x\n",result,mask);
		if (mask == 0) break;
	}

	// Because bit-shifting right is compiler-dependent,
	// shift is used to re-shift from the right instead.

	// ... these are all unsigned so most likely the compiler will be fine.

	// Repeated subtraction (xor) in long division:
//	gen <<= shift; // gen should already be 9 bits
	gen *= mask;
	while (2^power >= result)
	{
		if (mask & result)
		{
			result ^= gen;
		}
		mask >>= 1;
		gen >>= 1;
		printf("2 gen %x mask %x result %x\n",gen,mask,result);
		if (mask == 0) break;
	}
	return result;
}





// Reed-Solomon Encoder
// BCH perspective

//Eventually this will become a generic function for RS

// n,k: 2,1
// packet size: p
// code word size: 1 byte (GF(2^8))
int rs2x1(int p, FILE *in, FILE *out)
{

// matrix:
// [n+k] x [n]

/*
 * 1 0
 * 0 1
 * 1 1
 */



// There will be 2 message packets, and one parity packet.
// This is very easy to do without matrices; however,
// this will still be done in the matrix-encoding style
// for consistency with other n,k values. // no it won't i lied.

// Currently, some values are hard-coded, while others are generic.

// NOTE: this function currently does NOT add in information about
// packet placement; for testing, we can assume that packets follow
// a specific order, and we can imply their position and contents
// based on their position in the stream, but that might not be
// true later.



/* unused currently
unsigned char vand[3][2]

vand[0][0] = 0x01
vand[0][1] = 0x00

vand[1][0] = 0x00
vand[1][1] = 0x01

vand[2][0] = 0x01
vand[2][1] = 0x01
*/



	int n = 2;
	int k = 1;

	int counter = 0;

	unsigned char packet[n+k][p];
	int end = 0;

	while (!end)
	{	// For every group of bytes about to be a original message packet
		for (int i = 0; i < n; i++)
		{
			// For every byte in the group
			for (int packpos = 0; packpos < p; packpos++)
			{	// get the byte
				int next = fgetc(in);
				if (next == EOF)
				{	//if end of file, continue to write 0s in code
					packet[i][packpos] = 0x00;
					end = 1;
				}
				else
				{	//store the byte
					packet[i][packpos] = (unsigned char) next;
				}
			}
		}
		// for each parity packet (only 1, pointless loop for rs2,1)
		for (int i = n; i < n+k; i++)
		{
			// for each position in the parity packet
			for (int packpos = 0; packpos < p; packpos++)
			// galois field multiply the packets together
			{
				packet[i][packpos] = mulGF(packet[0][packpos],packet[1][packpos],8,285);
							//generator: binary 100011101 decimal 285
			}

		}
		// for each packet
		for (int i = 0; i < n+k; i++)
		{
			// for each byte in the packet
			for (int packpos = 0; packpos < p; packpos++)
			{
				// write to stream
				fputc(packet[i][packpos],out);
			}
		}
		counter++;
	}
	return counter;
}


// Reed-Solomon decoder

// Very simplified approach for 2,1
// simply looks for the first two packets
// if parity packet is necessary, divides it by received

// Again, for testing, it assumes an order to packets received,
// and "not received" packets are actually all 0s. (see udp decoder)

// plen = packet length, bytes. pnum = number of packets including parity
int d_rs2x1(unsigned int plen, int pnum, FILE *in, FILE *out)
{

	// Once again, some parts are hardcoded, others depend on n,k being 2,1
	int n = 2;
	int k = 1;
	int next;
	unsigned char packet[pnum][plen]; // saves all packets at once (make this better?)
	int superzip[pnum]; // tracks if the packet is all zeroes

	// for each packet
	for (int i = 0; i < pnum; i++)
	{
		// for each position in each packet
		for (unsigned int j1 = 0; j1 < plen; j1++)
		{
			// get next char and store
			next = fgetc(in);
			packet[i][j1] = (unsigned char) next;
		}
	}


	// for each packet
	for (int i = 0; i < pnum; i++)
	{
		superzip[i] = 0x00;
		// for each position
		for (unsigned int j3 = 0; j3 < plen; j3++)
		{	// superzip being 0 => all bytes are 0
			superzip[i] |= packet[i][j3];
		}
	}

	// for each packet group
	for (int i = 0; i < pnum; i+= n+k)
	{
		printf("%d %d %d\n",(unsigned short) superzip[0+i],(unsigned short) superzip[1+i],(unsigned short) superzip[2+i]);
		// if first packet is 0s, decode with 2nd and 3rd, if correct
		// decoder matrix for 2,1: [(1,0),(1,1)]
		if (superzip[0+i] == 0x00 && superzip[2+i] != 0x00)
		{	// for each packet position
			for (unsigned int j2 = 0; j2 < plen; j2++)
			{	// write first packet
//				unsigned char adjusted = mulGF(packet[1+i][j2],packet[2+i][j2],8,285);
				unsigned char adjusted = packet[1+i][j2] ^ packet[2+i][j2];
				fputc(adjusted,out);
			}
			for (unsigned int j2 = 0; j2 < plen; j2++)
			{	// then write second packet
				fputc(packet[1+i][j2],out);
			}
		printf("packet 1 all 0s, packet 3 not\n");
		}
		// if first isn't all 0s, see if 2nd is, and change if 3rd is correct
		// decoder matrix: same for 2,1
		else if (superzip[1+i] == 0x00 && superzip[2+i] != 0x00)
		{
			for (unsigned int j2 = 0; j2 < plen; j2++)
			{
				fputc(packet[0+i][j2],out);
			}
			for (unsigned int j2 = 0; j2 < plen; j2++)
			{
				unsigned char adjusted = packet[0+i][j2] ^ packet[2+i][j2];
				fputc(adjusted,out);
			}
		printf("packet 2 all 0s, packet 3 not\n");
		}

		// if neither are, or if 3rd is incorrect, use first two packets (no berlkamf 4u)
		else
		{
			for (unsigned int j2 = 0; j2 < plen; j2++)
			{
				fputc(packet[0+i][j2],out);
			}
			for (unsigned int j2 = 0; j2 < plen; j2++)
			{
				fputc(packet[1+i][j2],out);
			}
		printf("else\n");

		}
	}

	return 0;
}


/* DATA SCRAMBLING FUNCTIONS
 * to aid in testing
 */



// Stratified scramble
// Every n bytes, the read char will be XORed with 0xff.

void sstrat(int n, FILE *in, FILE *out)
{
        int end = 0;
        while (!end)
        {
                int c, alt;
                for (int i = 0; i < n-1 && !end; i++)
                {
                        c = fgetc(in);
                        if (c==EOF)
                                end = 1;
                        else
                                fputc((unsigned char) c,out);
                }
                alt = fgetc(in);
                if (alt==EOF)
                        break;
                else
                {
                        unsigned char temp = ((unsigned char) alt) ^ 0xff;
                        fputc(temp,out);
                }
        }
        return;
}

// scrambles by &&ing with a random number a power of 2 times
// 1 in 2^n chance of any bit being flipped
void scram(int n, FILE *in, FILE *out)
{
  int end = 0;
  int next = 0;
  srand(time(0));

  while ((next=fgetc(in)) != EOF)
  {
    unsigned char c = (unsigned char) next;
    unsigned char flip = 0xff;
    for (int i = 0; i < n; i++)
    {
      flip &= (unsigned char) rand();
    }
    c ^= flip;
    fputc(c,out);
  }
}
