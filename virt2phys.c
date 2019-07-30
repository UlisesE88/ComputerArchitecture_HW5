#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int log2(int N);
int ones(int N);

int main(int argc, char* argv[]) {
	int address_bit;
	int page_size;

	FILE* pFile = fopen(argv[1], "r");
	char* virtual_address_str = argv[2];
	int virtual_address = (int)strtol(virtual_address_str, NULL, 16);
	
	fscanf(pFile, "%i", &address_bit);
	fscanf(pFile, "%i", &page_size);

	int page_size_2 = log2(page_size);
 	int power = address_bit - page_size_2;
 	int num_of_ppn = pow(2, power);

	int i = 0;
	int ppn[num_of_ppn];
	while(!feof(pFile)) {
		fscanf(pFile, "%i", &ppn[i]);
		i++;
	}
	fclose(pFile);

	int offset_bits = log2(page_size);
	int list_of_ones = ones(offset_bits);
	
	int offset = virtual_address & list_of_ones;
	int virtual_address_number = virtual_address >> offset_bits;

	
	int value = ppn[virtual_address_number];
	if(value == -1) printf("PAGEFAULT\n");
	else printf("%x\n", ((value << offset_bits) | offset));
			
	return (EXIT_SUCCESS);
}

int log2(int N) {
	int r = 0;
	while (N>>=1) r++;
	return r;
}

int ones(int N) {
 	return ((1 << N) - 1);
 }