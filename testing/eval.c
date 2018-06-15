#include "fec.c"

int main(int argc, char *argv[])
{
	FILE *in1 = fopen(argv[1],"rb");
        FILE *in2 = fopen(argv[2],"rb");
	FILE *out = fopen(argv[3],"wb");

	for (int i = 0; i < 32000; i++)
	{
		fputc( (fgetc(in1) ^ fgetc(in2)), out);
	}

	fclose(in1);
	fclose(in2);
	fclose(out);

	int acc = 0;

	out = fopen(argv[3],"rb");

	for (int i = 0; i < 32000; i++)
	{
		if (fgetc(out) != 0)
			acc++;
	}

	printf("Number of byte differences: %d\n", acc);

	return 0;
}
