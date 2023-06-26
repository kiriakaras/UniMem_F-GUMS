#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define DEBUG 1

#define N 160 //TC line N width 

#define TAG_N 28
#define RFU1_N 4

#define VALID_N 1
#define RFU2_N 9
#define PAGE_SIZE_N 2
//#define UNIMEM_PAGE_ID_N 36
#define UNIMEM_PAGE_ID_2_N 4
#define UNIMEM_PAGE_ID_1_N 32
#define ADDR_EXT_N 16

#define COORDINATES_N 16
#define CONTEXT_N 16
#define VERSION_N 4
#define OPERATION_N 8
#define UNIMEM_GVA_N 20




uint32_t gen_mask(int width) {
	uint32_t mask;

	for(int i = 0; i < width; i++)
		mask |= (1 << i);

	return mask;
}




//UNCOMMENT THE CORRECT PAGE_SIZE
#define PAGE_SIZE 1 //1GB
//#define PAGE_SIZE 2 //2MB
//#define PAGE_SIZE 4 //4KB



// ------> NOTES <--------
//   Data0 Register[0:31]
//   AxUSER signal

//   Data1 Register[32:63]
//   AxUSER signal

//   Data2 Register[64:95]
//   entry[95:80] --> Data2[95:80] (UNIMEM_PAGE_ID)
//									  (PAGE_SIZE == 4) = Addr[27:12]
//									  (PAGE_SIZE == 2) = {Addr[27:21],9'b0}
//									  (PAGE_SIZE == 1) = 16'b0
//	 entry[79:64] = Addr[63:48] --> Data2[79:64]


//   Data3 Register[96:127]
//   entry[115:96] --> Data3[115:96] (UNIMEM_PAGE_ID)
//									  (PAGE_SIZE == 4) = Addr[47:28]
//									  (PAGE_SIZE == 2) = Addr[47:28]
//									  (PAGE_SIZE == 1) = {Addr[47:30],2'b0}
//	 entry[117:116] --> PAGE_SIZE
//   entry[126:118] --> 0x0
//   entry[127] --> 0x1 (VALID bit == ALWAYS VALID)

//   Data4 Register[128:159] --> 0x0

#define ADDR 0xAAAA012821130000
#define USR  0xDEADBEEFDEADBEEF

//GENERATE DATA REGISTERS FROM ADDR
uint32_t gen_data(uint64_t addr, int regID) {
	uint32_t data_reg;
	uint32_t temp;





	switch(regID) {
		case 0:
				//Data0 = AxUSER[31:0]
				data_reg = addr & gen_mask(32);
				break;
		case 1:
				//Data1 = AxUSER[64:32]
				data_reg = addr >> 32;
				break;
		case 2: 
				data_reg = (addr >> 48); //16LSB
				if (PAGE_SIZE == 1) {
					data_reg |= 0x00000000; //16MSB
				}
				else if (PAGE_SIZE == 2) {
					temp = (addr >> 21) & gen_mask(7);
					data_reg |= (((temp << 9) | 0x000) << 16); //16MSB
				}
				else if (PAGE_SIZE == 4) {
					temp = (addr >> 12) & gen_mask(16);
					data_reg |= (temp << 16); //16MSB
				}
				else {
					//Do nothing
				}
				break;
		case 3:
				if (PAGE_SIZE == 1) {
					temp = (((addr >> 30) & gen_mask(18)) << 2); //20LSB
					data_reg = (0x2 << 20) | temp; 				
				}
				else if (PAGE_SIZE == 2) {
					temp = ((addr >> 30) & gen_mask(20));
					data_reg = (0x1 << 20) | temp;
					
				}
				else if (PAGE_SIZE == 4) {
					temp = ((addr >> 30) & gen_mask(20));
					data_reg = (0x0 << 20) | temp;
				}
				else {
					//Do nothing
				}
				data_reg |= (0x200 << 22); //12MSB
				break;
		case 4:
				data_reg = 0;
				//Mostly RFU or not possible to extracted from address
				break;
		default:
			printf("Warning: Never enter here\n");
	}

	return data_reg;
}


int main () {
	
	uint32_t tag;
	uint32_t rfu1 = 0;
	uint32_t valid;
	uint32_t rfu2 = 0;
	uint32_t page_size;
	uint32_t unimem_page_id_2; //MSB[35:32]
	uint32_t unimem_page_id_1; //LSB[31:0]
	uint32_t addr_ext;
	uint32_t coordinates;
	uint32_t context;
	uint32_t version;
	uint32_t operation;
	uint32_t unimem_gva;

	int x = (0x103A << 10) + 8;		
		printf("%x\n",x);

	//ONLY TO DEBUG CODE
    if (DEBUG) {
		tag = 0x8000001; //1000 0000 0000 0000 0000 0000 0001
		valid = 0x1; //0001
		page_size = 0x3; //0011
		unimem_page_id_2 = 0x9; //1001
		unimem_page_id_1 = 0x80000001; //1000 0000 0000 0000 0000 0000 0000 0001
		addr_ext = 0x8001; //1000 0000 0000 0001
		coordinates = 0x8009; //1000 0000 0000 1001
		context = 0x8011; //1000 0000 0001 0001
		version = 0x9; //1001
		operation = 0x81; //1000 0001
		unimem_gva = 0x80009; //1000 0000 0000 0000 1001
	}


	tag &= gen_mask(TAG_N);
	rfu1 &= gen_mask(RFU1_N);
	valid &= gen_mask(VALID_N);
	rfu2 &= gen_mask(RFU2_N);
	page_size &= gen_mask(PAGE_SIZE_N);
	unimem_page_id_1 &= gen_mask(UNIMEM_PAGE_ID_1_N);
	unimem_page_id_2 &= gen_mask(UNIMEM_PAGE_ID_2_N);
	page_size &= gen_mask(PAGE_SIZE_N);
	addr_ext &= gen_mask(ADDR_EXT_N);
	coordinates &= gen_mask(COORDINATES_N);
	context &= gen_mask(CONTEXT_N);
	version &= gen_mask(VERSION_N);
	operation &= gen_mask(OPERATION_N);
	unimem_gva &= gen_mask(UNIMEM_GVA_N);


	uint64_t bit_63_0 = ((long)coordinates << (CONTEXT_N + VERSION_N + OPERATION_N + UNIMEM_GVA_N)) |
						((long)context << (VERSION_N + OPERATION_N + UNIMEM_GVA_N)) |
						((long)version << (OPERATION_N + UNIMEM_GVA_N)) |
						((long)operation << UNIMEM_GVA_N) |
						(long)unimem_gva;

	uint64_t bit_127_64 = ((long)valid << (RFU2_N + PAGE_SIZE_N + UNIMEM_PAGE_ID_1_N + UNIMEM_PAGE_ID_2_N + ADDR_EXT_N)) |
						  ((long)rfu2 << (PAGE_SIZE_N + UNIMEM_PAGE_ID_1_N + UNIMEM_PAGE_ID_2_N + ADDR_EXT_N)) |
						  ((long)page_size << (UNIMEM_PAGE_ID_1_N + UNIMEM_PAGE_ID_2_N + ADDR_EXT_N)) |
						  ((long)unimem_page_id_2 << (UNIMEM_PAGE_ID_1_N + ADDR_EXT_N)) |
						  ((long)unimem_page_id_1 << ADDR_EXT_N) |
						  (long)addr_ext;

	uint64_t bit_159_128 = ((long)tag << (TAG_N + RFU1_N)) |
						   (long)rfu1;


	//ONLY TO DEBUG CODE	
    if(DEBUG) {
    	printf("bit_63_0:%llu\n",bit_63_0);
		printf("bit_127_64:%llu\n",bit_127_64);
		printf("bit_160_128:%llu\n",bit_159_128);

    	char tc_line[N];
    	char BIT_63_0[64];
    	char BIT_127_64[64];
    	char BIT_159_128[32];

		sprintf(BIT_63_0, "%llu", bit_63_0);
		sprintf(BIT_127_64, "%llu", bit_127_64);
		sprintf(BIT_159_128, "%llu", bit_159_128);

		//Concatenate string values
		strcat(tc_line, BIT_159_128);
		strcat(tc_line, BIT_127_64);
		strcat(tc_line, BIT_63_0);

		printf ("TC LINE:%s\n", tc_line);
	}

	if (DEBUG) {
		printf("REG. DATA_0:%x\n", gen_data(USR,0));
		printf("REG. DATA_1:%x\n", gen_data(USR,1));
		printf("REG. DATA_2:%x\n", gen_data(ADDR,2));
		printf("REG. DATA_3:%x\n", gen_data(ADDR,3));
		printf("REG. DATA_4:%x\n", gen_data(ADDR,4));
	}

	return 0;
}

//the bits we need to set:
	/*	tag =		
		valid = 1
		page_size = 1 //for 1GB addresses		
		unimem_page_id_2 =	
		unimem_page_id_1 = 		
		addr_ext =		
		coordinates = 0x1039 when accessing 22->57
		context = 
		version = that should be the encoding versioning. So 0 for a 48-bit GVA and location bits without Address Extension

		operation = 0 (should be 0 for normal read/writes)
		unimem_gva =
	*/
