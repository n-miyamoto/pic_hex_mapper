#include <stdio.h>
#include <string.h>

#ifdef unix
#include<stdlib.h>
#endif

#define FILENAME "test.hex"
#define BUF 		(10000)
#define DATA_BUF 	(0xFF)
#define MEMORY_SIZE (0xAC00)

#define EMURATE_MEMORY
#ifdef EMURATE_MEMORY
unsigned char program_memory[MEMORY_SIZE];
#endif



typedef struct format{
	unsigned char status_code;
	unsigned int data_length;
	unsigned long address_offset;
    unsigned int record_type;
	unsigned char data[DATA_BUF];
 	unsigned char check_sum;	
} FORMAT;


int char2hex(char chr){
	int ret;
	ret = chr-0x30;
	if(ret<0){  
		return -1;
	}

	//0~9
	if(ret<10){ 
		return ret;
	}

	//A~F
	ret-=0x11;
	if(ret<0){
		return -1;
	}
	if(ret<6){
		return ret+10;
	}

	//a~f
	ret-=0x20;
	if(ret<0){
		return -1;
	}
	if(ret<6){
		return ret+10;
	}
}


int parse_hex_format(const char *str, FORMAT *format ){
	char hex_data[DATA_BUF+4];
			
	//NULL check	
	if(format == NULL ){
		return -1;
	}
	if(str==NULL){
		return -1;
	}

	//Read status code.
	if(str[0]!=':'){	
		return -1;
	}else{
		format->status_code = ':';
	}

	//Read data length
	format->data_length = 0x10*char2hex(str[1])+char2hex(str[2]);

	//Read address format
	format->address_offset= 0x1000*char2hex(str[3])+
				0x0100*char2hex(str[4])+
				0x0010*char2hex(str[5])+
				0x0001*char2hex(str[6]);

	//Read record type
	format->record_type= 0x10*char2hex(str[7])+char2hex(str[8]);

	//Read data
	int i;
	for(i=0;i<format->data_length;i++){
		format->data[i]=0x10*char2hex(str[ 9+2*i])+
						0x01*char2hex(str[ 10+2*i]);
	}
	//Read check sum
	format->check_sum = (unsigned char)0x10*char2hex(str[9+2*format->data_length])+char2hex(str[10+2*format->data_length]);
	
	return 0;
}



void erase_memory(void){
#ifdef EMURATE_MEMORY
	memset(program_memory,0xFF,sizeof(program_memory));
#endif 
}	

void write_memory(long address, unsigned char data){
#ifdef EMURATE_MEMORY
	program_memory[address]=data;
#endif
}

unsigned char read_memory(long address){
#ifdef EMURATE_MEMORY
	return program_memory[address];
#endif 
	return 0;
}

void show_memory(void){
#ifdef  EMURATE_MEMORY
	int i=0;
	for(i=0;i<MEMORY_SIZE;i++){
		if(i%0x20==0) printf("\r\n%04x :",i/0x20*0x20);
		printf("%02x ",program_memory[i]);
	}
#endif 
}

void map_hex_format(const FORMAT *format){
	int i=0;
	long offset = format->address_offset;
	for(i=0;i<format->data_length;i++){
		write_memory(offset+i,format->data[i]);
	}
}

#ifndef TEST

int main(void){
	FILE *fp;
	char str[BUF];

	fp=fopen(FILENAME, "r");
	if(fp==NULL){
		printf("failed to open file.");
		return 1;	
	}


	FORMAT fmt;
	erase_memory();
	while(fgets(str,256,fp)!=NULL){
		//printf("%s",str);		
		if(-1==parse_hex_format(str,&fmt)){
			printf("ERROR!!!!\r\n");
			return 0;
		}
		//printf("datalength:%d\r\n",fmt.data_length);
		map_hex_format(&fmt);
	}

	show_memory();
	return 0;
}

#else 

int check_char2hex(char chr, int answer){
	if(answer!=char2hex(chr))return -1;
	else return 0;
}

int test_char2hex(void){
	if(check_char2hex('0',0x0)<0)return -1;
	if(check_char2hex('1',0x1)<0)return -1;
	if(check_char2hex('9',0x9)<0)return -1;
	if(check_char2hex('A',0xA)<0)return -1;
	if(check_char2hex('C',0xC)<0)return -1;
	if(check_char2hex('F',0xF)<0)return -1;
	if(check_char2hex('a',0xA)<0)return -1;
	if(check_char2hex('c',0xC)<0)return -1;
	if(check_char2hex('f',0xF)<0)return -1;
	if(check_char2hex(':',-1) <0)return -1;
	if(check_char2hex(' ',-1)<0)return -1;
	return 0;	
}

int check_parse_hex_format(const char *input , FORMAT *answer ){
	FORMAT temp;
	FORMAT *res;
	res=&temp;
	if(-1==parse_hex_format(input,res)){
		return -1;
	}	
	if(answer->status_code		!=res->status_code	) 		return -1;
	if(answer->data_length		!=res->data_length 	) 		return -1;
	if(answer->address_offset	!=res->address_offset)		return -1;
	if(answer->record_type		!=res->record_type	) 		return -1;
	if(answer->check_sum		!=res->check_sum		) 	return -1;
	
	int i=0;
	for(i=0;i<res->data_length;i++){
		if(answer->data[i]!=res->data[i]) return -1;
	}	
	return 0;
}

int test_parse_hex_format(void){
	char test_str_1[BUF] = ":020000040000FA";
	FORMAT ans_1 = {':',0x02,0x0000,0x04,{0x00,0x00},0xFA};
	if(-1==check_parse_hex_format(test_str_1,&ans_1)) return -1;

	char test_str_2[BUF] = ":100000008316850186010130860083120030850049";
	FORMAT ans_2 = {':',0x10,0x0000,0x00,{0x83,0x16,0x85,0x01,0x86,0x01,0x01,0x30,0x86,0x00,0x83,0x12,0x00,0x30,0x85,0x00},0x49};
	if(-1==check_parse_hex_format(test_str_2,&ans_2)) return -1;

	char test_str_3[BUF] = ":00000001FF";
	FORMAT ans_3 = {':',0x00,0x0000,0x01,{""},0xFF};
	if(-1==check_parse_hex_format(test_str_3,&ans_3)) return -1;

	char test_str_4[BUF] = ":10000000abcd8501860101308600cdef0030850049";
	FORMAT ans_4 = {':',0x10,0x0000,0x00,{0xab,0xcd,0x85,0x01,0x86,0x01,0x01,0x30,0x86,0x00,0xcd,0xef,0x00,0x30,0x85,0x00},0x49};
	if(-1==check_parse_hex_format(test_str_4,&ans_4)) return -1;
	return 0;
}



int test_memory_rw(void){
	erase_memory();
	int i=0;
	for(i=0;i<MEMORY_SIZE;i++){
		if(0xFF!=read_memory(i)) return -1;
	}
	write_memory(0xAB,0xCD);
	if(0xCD!=read_memory(0xAB)) return -1;
	//show_memory();
	return 0;
}

int test_map_hex_format(void){
	erase_memory();
	FORMAT fmt = {	':',	//status code
					0x10,	//data length
					0x00F,	//address offset
					0x00,	//type
					{0x83,0x16,0x85,0x01,0x86,0x01,0x01,0x30,0x86,0x00,0x83,0x12,0x00,0x30,0x85,0x00},//data
					0x49//check sum
					};
	map_hex_format(&fmt);
	show_memory();
	return 0;
}

//TEST CODE
int main(void){
	//Test char2hex function.
	if(test_char2hex()<0)printf("test char2hex failed.\r\n");
	else printf("Test chat2hex successed.\r\n");

	//Test parse_hex_format function.
	if(test_parse_hex_format()<0) printf("Test parse_hex_format failed.\r\n");
	else printf("Test parse_hex_format succeess.\r\n");

	//Test read & write memory function.
	if(test_memory_rw()<0) printf("Test memory r/w failed.\r\n");
	else printf("Test memory r/w succes. \r\n");

	//Test map_hex_format function.
	test_map_hex_format();

	return 0;
}

#endif 


