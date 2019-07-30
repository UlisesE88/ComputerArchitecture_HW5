#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int hit;
unsigned char memory[16777216]; //Need 16777216 because address is 2^N where N = 24

//blocks will have the tag and pointers to excute LRU
typedef struct Block {
    struct Block *next, *prev;
    int tag;
} Block;

typedef struct Set_row {
    int set_num;
    int num_columns;
    int num_of_frames_filled;
    struct Block *head, *tail;
} Set_row;

void store_function(Set_row *setRow, int addr_tag, int address, int access_size, unsigned int value[]);
void load_function(Set_row *setRow, int addr_tag, int address, int access_size, unsigned int load_array[]);
void helper(Block* curr, Set_row *setRow, int addr_tag, int signal);
Set_row* initialize_cache(int associativity, int row_num);
void add_to_setRow(Set_row *setRow, int addr_tag);
void remove_block(Set_row *setRow);
void add_to_memory(int address, int access_size, unsigned int value[]);
void retrieve_from_memory(int address, int access_size, unsigned int value[]);
int log2(int n);

int main(int argc, char* argv[]) {
    int cache_size = atoi(argv[2]) * 1024; //convert KB into Bytes 
    int associativity = atoi(argv[3]);
    int block_size = atoi(argv[4]);

    int address_tag;
    int address_offset;
    int address_index;

    int num_of_frames = cache_size / block_size;
    int num_of_sets = num_of_frames / associativity;

    int offset_bits = log2(block_size);
    int index_bits = log2(num_of_sets);
    int tag_length = 24 - offset_bits - index_bits;

    char function_type[10];

    int address_dec; //address in decimal
    int size_access; 
    unsigned int store_value[8];

    Set_row cache_array[num_of_sets];
    Set_row *set = NULL;

    for (int i = 0; i < num_of_sets; i++) {
        set = initialize_cache(associativity, i);
        cache_array[i] = *set;
    }

    FILE* pFile = fopen(argv[1], "r");

    while (!feof(pFile)) {
        strncpy(function_type, "\0", 1);

        fscanf(pFile, "%s", function_type);
        if (strcmp(function_type, "") == 0)
            break;

        fscanf(pFile, "%x", &address_dec); //24 bit address
        fscanf(pFile, "%i", &size_access);

        address_tag = address_dec >> (offset_bits + index_bits);
        //address_offset = (address_dec << (tag_length + index_bits)) >> (tag_length + index_bits);
        address_index = ((address_dec >> offset_bits) & (1 << index_bits) - 1);

        if (strcmp(function_type, "store") == 0) {
            int i = 0;
            while(size_access > i){
                fscanf(pFile, "%2hhx", &store_value[i]);
                i++;
            }
        
            store_function(&cache_array[address_index], address_tag, address_dec, size_access, store_value);
            printf("%s 0x%x ", function_type, address_dec);
            
            if (hit == 1){
                printf("hit\n");
            } 
            else {
                printf("miss\n");
            }
        }

        if (strcmp(function_type, "load") == 0) {
            unsigned int loaded_value[size_access];
            load_function(&cache_array[address_index], address_tag, address_dec, size_access, loaded_value);
            printf("%s 0x%x ", function_type, address_dec);
            
            if (hit == 1){
                printf("hit ");
            } 
            else {
                printf("miss ");
            }
            
            for (int i = 0; i < size_access; i++)
            {
                printf("%0*x", 2, loaded_value[i]);
            }
            printf("\n");            
        }
    }
    return (EXIT_SUCCESS);
}

void store_function(Set_row *setRow, int addr_tag, int address, int access_size, unsigned int value[]) {
    Block* curr = setRow->head;
    hit = 0;
    if (setRow->num_of_frames_filled != 0){
        while (curr != NULL) {
            if (curr->tag == addr_tag) {
                hit = 1;

                //case where curr is in the front
                if (curr == setRow->head) {
                    int signal = 0;
                    helper(curr, setRow, addr_tag, signal);
                    break;
                }
                //case where curr is in the middle of the row
                if(curr != setRow->head && curr != setRow->tail){
                    int signal = 1;
                    helper(curr, setRow, addr_tag, signal);
                    break;
                }
            }
        curr = curr->prev;
        }
    }
    //store value in memory
    add_to_memory (address, access_size, value) ;                                            
}

void load_function(Set_row *setRow, int addr_tag, int address, int access_size, unsigned int load_array[]) {
    Block *curr = setRow->head;
    hit = 0;
    if (setRow->num_of_frames_filled != 0){
        while (curr != NULL) {
            if (curr->tag == addr_tag) {
                hit = 1;
                retrieve_from_memory(address, access_size,load_array);

                if (curr == setRow->head) {
                    int signal = 0;
                    helper(curr, setRow, addr_tag, signal);
                    return;
                }

                if (curr != setRow->head && curr != setRow->tail){
                    int signal = 1;
                    helper(curr, setRow, addr_tag, signal);
                    return;
                }
                return;
            }
            curr = curr->prev;
        }
    }
    //add block in case frames are empty and retrieve value from memory
    add_to_setRow(setRow, addr_tag);
    retrieve_from_memory(address, access_size,load_array);
}

void helper(Block* curr, Set_row *setRow, int addr_tag, int signal){
    if (signal == 0){
        remove_block(setRow);
        add_to_setRow(setRow, addr_tag);
    }

    if (signal == 1){
        curr->prev->next = curr->next;
        curr->next->prev = curr->prev;
        setRow->num_of_frames_filled = setRow->num_of_frames_filled - 1;
        add_to_setRow(setRow, addr_tag);
    }
}

Set_row* initialize_cache(int associativity, int row_num){
    Set_row *set;
    set = (Set_row*)malloc(sizeof(Set_row));

    set->num_of_frames_filled = 0;
    set->num_columns = associativity;
    set->set_num = row_num;   
    
    set->head = NULL;
    set->tail = set->head;

    return set;
}
//removes front block since it's least used
void add_to_setRow(Set_row *setRow, int addr_tag) {
    Block* temp = (Block*)malloc(sizeof(Block));
    temp->tag = addr_tag;

    if (setRow->num_of_frames_filled == 0 && setRow->num_columns != setRow->num_of_frames_filled) { //This is when all the frames are empty in the set
         setRow->head = setRow->tail = temp;
         setRow->head->prev = setRow->head->next = NULL;
     }

    else if (setRow->num_of_frames_filled != 0 && setRow->num_columns != setRow->num_of_frames_filled) { //This is the case where there are blocks in the set but there are still empty blocks left
         temp->next = setRow->tail;
         setRow->tail->prev = temp;
         setRow->tail = temp;
     }

    else if (setRow->num_columns == setRow->num_of_frames_filled) { //This is the case where all blocks are filled, need to make space for new one       
        if (setRow->num_of_frames_filled == 1){
            remove_block(setRow);
            setRow->head = setRow->tail = temp;
            setRow->tail->prev = setRow->tail->next = NULL;    
        }

        if (setRow->num_of_frames_filled > 1){
            remove_block(setRow);
            temp->next = setRow->tail;
            setRow->tail->prev = temp;
            setRow->tail = temp;
        }
    }
    setRow->num_of_frames_filled++;
}

void remove_block(Set_row *setRow) {
    if (setRow->num_of_frames_filled == 1) {
        setRow->head = setRow->tail = NULL;
    }

    if (setRow->num_of_frames_filled > 1) {
        setRow->head = setRow->head->prev;
        setRow->head->next = NULL;
    }
    setRow->num_of_frames_filled--;
}

void add_to_memory(int address, int access_size, unsigned int value[]){
    for (int i = 0; i < access_size; i++) {
        memory[address + i] = value[i];
    }
}

void retrieve_from_memory(int address, int access_size, unsigned int value[]) {
    for (int i = 0; i < access_size; i++) {
        value[i] = memory[address + i];
    }
}

int log2(int n) {
    int r = 0;
    while (n >>= 1) r++;
    return r;
}