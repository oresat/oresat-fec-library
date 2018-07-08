// Adds UDP packet: broadcast, no checksum, dest port 0
int addUDP(unsigned int length, FILE *out);

// Add UDP packet headers to a stream of data
int inlvUDP(unsigned int length, FILE *in, FILE *out);

// "Decode" UDP stream. See fec.c
int decUDP(int pnum, unsigned int plen, FILE *in, FILE *out);

// Encode hamming 7,4
int h74(FILE *in, FILE *out);

// decode h 7,4
int d_h74(FILE *in, FILE *out);

// Break up stream into hamming code interleaved packets
int inlvham(unsigned int plen, FILE *in, FILE *out);

// Stratified scrambler
void sstrat(int n, FILE *in, FILE *out);

// power of 2 scrambler
void scram(int n, FILE *in, FILE *out);

// Below are WIP:

/*



// Reed solomon 2,1
int rs2x1(int p,FILE *in, FILE *out);

// Galois field multiplication (no logs)
int mulGF(unsigned char poly1, unsigned char poly2,
	unsigned short power, unsigned short generator);

*/
