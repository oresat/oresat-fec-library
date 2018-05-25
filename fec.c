#include <stdio.h>
#include <stdlib.h>

#define DEBUG 0

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
 * it can be thougt of as 8 unique (7,4) codes calculated in parallel.
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
