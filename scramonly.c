#include "fec.c"

int main(int argc, char *argv[])
{
	FILE *in = fopen(argv[1],"rb");
        FILE *out = fopen(argv[2],"wb");
	scram(15,in,out);
	fclose(in);
	fclose(out);
	return 0;
}
