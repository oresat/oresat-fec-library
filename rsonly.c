#include "fec.c"

int main(int argc, char *argv[])
{
	FILE *in = fopen(argv[1],"rb");
        FILE *out = fopen(argv[2],"wb");

	printf("%d\n",rs2x1(1000,in,out));

	fclose(in);
	fclose(out);

	in = fopen(argv[2],"rb");
        out = fopen(argv[3],"wb");

	int pnum = inlvUDP(1000,in,out);

	fclose(in);
	fclose(out);

	in = fopen(argv[3],"rb");
	out = fopen(argv[4],"wb");

	scram(20,in,out);

	fclose(in);
	fclose(out);

	in = fopen(argv[3],"rb");
	out = fopen(argv[6],"wb");

	decUDP(pnum,1000,in,out);

	fclose(in);
	fclose(out);

	in = fopen(argv[2],"rb");
	out = fopen(argv[6],"wb");

	d_rs2x1(1000,pnum,in,out);

	fclose(in);
	fclose(out);

	return 0;
}
