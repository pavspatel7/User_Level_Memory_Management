// Project Partners
// Kush Patel (kp1085)
// Pavitra Patel (php51)

// Tested ilab:
// ilab: kill.cs.rutgers.edu
// ilab: cp.cs.rutgers.edu
// ilab: ilab1.cs.rutgers.edu

// Run commands from the benchmark directory:
// Compile command test          : cd .. && make clean && make && cd benchmark && make clean && make && ./test 
// Compile command multi-test    : cd .. && make clean && make && cd benchmark && make clean && make && ./mtest 

#include "my_vm.h"
struct tlb tlb_store;

void initialize_physical_virtual_bitmaps();                              // Method to initialize physical and virtual bitmaps
static void  set_bit_at_index(char *bitmap, int index);                  // Set bit at the given index
static int  get_bit_at_index(char *bitmap, int index);                   // Get bit at the given index
static void  unset_bit_at_index(char *bitmap, int index);                // Reset bit at the given index
int remove_TLB(int st, int en);                                          // Remove from TLB
unsigned long long tot__physical_pages = (MEMSIZE)/(PGSIZE);             // Get the physical pages
unsigned long long tot__virtual_pages = (MAX_MEMSIZE)/(PGSIZE);          // Get the virtual pages


// Variable Initializations
int miss__TLB = 0;                        // TLB miss hit
int look__TLB = 0;                        // TLB look up hit
pthread_mutex_t mutex;                    // P thread
struct page * my_physical__memory;        // Physical Memory
char * my_physical__bit_map;              // Physical bitmap
char * my_virtual__bit_map;               // Virtual bitmap
int initt = 0;                            // Initialzation
int OSet_B = 0;                           // Off set bit
int bit_lvl__two = 0;                     // Outer Page :- Second Level bits 
int bit_lvl__one = 0;                     // Inner Page :- First Level bits
int isdebug = 0;                          // Debug True = 0, False = 1 
pde_t *global_pgdir;                      // Global page directory variable

/*
Function responsible for allocating and setting your physical memory 
*/
void set_physical_mem() {
    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating

    // Step 1)
    // Allocating physical memory using malloc with pages of total physical page 
    my_physical__memory = (struct page *)malloc(sizeof(struct page) * tot__physical_pages);

    // Step 2) 
    // HINT: Also calculate the number of physical and virtual pages and allocate
    // Calculating the Offset bits, outer bits, and Inner bits
    OSet_B = log2(PGSIZE);                             // Get the offset using the log2 of the page size which is 4096
    bit_lvl__two = log2((PGSIZE)/sizeof(pte_t));       // Get Outer Page, Second Level bits
    bit_lvl__one = (32 - OSet_B) - bit_lvl__two;       // Get Inner Page, Inner Level bits using the VPN
    
    // Step 3)
    //virtual and physical bitmaps and initialize them
    initialize_physical_virtual_bitmaps();
}   


void initialize_physical_virtual_bitmaps() {

    // Using Malloc, we are giving an illusion of the virtual memory, 
    // that has a large region of contigency memeory refering to physical memory.
    // Variable Initialization
    int i = 0;
    int j = 0;

    //physical and virtual pages are declared as global variables above
    // Initialize physical bitmap
    my_physical__bit_map = (char *)malloc(tot__physical_pages/8);
    int temp = sizeof(my_physical__bit_map)/sizeof(my_physical__bit_map[0]) ;
    // iterating loop at index
    while (i < temp){ 
        my_physical__bit_map[i] = '\0';
        i++;
    }

    // Initialize Virtual bitmap
    my_virtual__bit_map = (char *)malloc(tot__virtual_pages/8);
    temp = sizeof(my_virtual__bit_map)/sizeof(my_virtual__bit_map[0]) ;
    // iterating loop at index
    while (j <temp) {
        my_virtual__bit_map[j] = '\0';
        j++;
    }

    // reserving index is the end / last in physical memory for the page directory
    int index = (tot__physical_pages) - 1;
    // set the bit for the page directory index to 1
	set_bit_at_index(my_physical__bit_map, index);
	struct page * ptr = &my_physical__memory[tot__physical_pages - 1];
	unsigned long * arr = ptr->array;
    i = 0;
    while (i < (1 << bit_lvl__two)){
        *(arr + i) = -1;
        i++;
    }
}


/* SET BIT AT GIVEN INDEX IN THE BITMAP  */
void set_bit_at_index(char *bitmap, int index) {
    int byte_index = index / 8;
    int bit_offset = index % 8;
    bitmap[byte_index] |= (1 << bit_offset);
}
/* GET BIT AT GIVEN INDEX IN THE BITMAP  */
int get_bit_at_index(char *bitmap, int index) {
    int byte_index = index / 8;
    int bit_offset = index % 8;
    return (bitmap[byte_index] >> bit_offset) & 0x1;
}
/* UNSET BIT AT GIVEN INDEX IN THE BITMAP  */
void unset_bit_at_index(char *bitmap, int index) {
    int byte_index = index / 8;
    int bit_offset = index % 8;
    bitmap[byte_index] &= ~(1 << bit_offset);
}


/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
// int add_TLB(void *va, void *pa) {
int add_TLB(int va, int pa) {
    /*Part 2 HINT: Add a virtual to physical page translation to the TLB */

    // Make Sure whether the va or pa are not null
    if ((void *)va == NULL || (void *) pa == NULL) {
        return -1;
    }

    // Used the size_t type for TLB index calculations based on the virtual index
    size_t checker_t = (size_t)va % TLB_ENTRIES;
	// Calculate the TLB index based on the virtual address
	tlb_store.array[checker_t][0] = va;
    tlb_store.array[checker_t][1] = pa;
    return -1;
}

/* REMOVE TLB at the given start and end*/
int remove_TLB(int st, int en) {
    for (unsigned long i = st; i < st + en; i++) {
        int temp = i % TLB_ENTRIES;
        if (tlb_store.array[temp][0] == i) {
            tlb_store.array[temp][0] = -1;
            tlb_store.array[temp][1] = -1;
        }
    }
}


/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
unsigned long check_TLB(void *va) {
    /* Part 2: TLB lookup code here */
    unsigned long _pgn_v_ = (unsigned int)va >> OSet_B;
	int checker_t = _pgn_v_ % TLB_ENTRIES;
	if (tlb_store.array[checker_t][0] != _pgn_v_) {
		return -1;
	} else {
		return tlb_store.array[checker_t][1];
	}
}


/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void print_TLB_missrate() {
    double miss_rate = 0;	
    /*Part 2 Code here to calculate and print the TLB miss rate*/
    miss_rate = (double) miss__TLB / (double) look__TLB;
    fprintf(stderr, "TLB miss rate %lf \n", miss_rate);
}



/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t *translate(pde_t *pgdir, void *va) {
    /* 
    * Part 1 HINT: Get the Page directory index (1st level) Then get the
    * 2nd-level-page table index using the virtual address.  Using the page
    * directory index and page table index get the physical address.
    *
    * Part 2 HINT: Check the TLB before performing the translation. If
    * translation exists, then you can return the physical address from the TLB.
    */
    unsigned long OSet__BM = (1 << OSet_B) - 1;
    look__TLB++;

    // Part 2: Check the TLB before performing the translation
    int checking_TLB = check_TLB(va);
    if (checking_TLB != -1) {
        // Translation exists in TLB, return the physical address
        return (pte_t *)((char *)&my_physical__memory[checking_TLB]) + ((unsigned long)((unsigned int)va) & OSet__BM);
    }

    miss__TLB++;

    unsigned long index_of_outisde = ((unsigned int)va) >> (32 - bit_lvl__two);
    int get_vpn = ((unsigned int)va) >> OSet_B;
    unsigned long index_of_inside = get_vpn & ((1 << bit_lvl__one) - 1);
    int taking_index = (index_of_outisde * (1 << bit_lvl__one)) + index_of_inside;

    if (get_bit_at_index(my_virtual__bit_map, taking_index) != 1) {
        return NULL;
    }

    // Get the page directory entry for the outer index
    pde_t *get_pde;

    if (pgdir == NULL) {
        // Use the global page directory if pgdir is NULL
        get_pde = global_pgdir + index_of_outisde;
    } else {
        // Use the provided page directory
        get_pde = pgdir + index_of_outisde;
    }

    pte_t *_Ipt_addr_ = (pte_t *)&my_physical__memory[ ((unsigned long)(*get_pde)) ];
    pte_t * get_PTE = _Ipt_addr_ + index_of_inside;
    unsigned long pg_number = *get_PTE;
    pte_t * pgAddr = (pte_t *)&my_physical__memory[pg_number];
    unsigned long physicalAddress = (unsigned long)((char *)pgAddr + ((unsigned long)((unsigned int)va) & OSet__BM));

    // Update TLB
    add_TLB(get_vpn, pg_number);

    return (pte_t *)physicalAddress;
}


/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/
int page_map(pde_t *pgdir, void *va, void *pa) {
    unsigned long get_ppn = (unsigned int)pa >> OSet_B;
    unsigned long get_vpn = ((unsigned int)va) >> OSet_B;

    int found_pg = check_TLB(va);
    look__TLB++;

    if (found_pg == get_ppn) {
        return 0;
    }

    // Use the global page directory if pgdir is NULL
    pde_t *get_pde = (pgdir != NULL) ? pgdir : (pde_t *)(my_physical__memory + tot__physical_pages - 1);

    unsigned long index_of_outside = ((unsigned int)va) >> (32 - bit_lvl__two);
    unsigned long firstLevelBitsMask = (1 << bit_lvl__one);
    unsigned long index_of_inside = get_vpn & (firstLevelBitsMask - 1);

    if (*get_pde == -1) {
        // Map the page table entry to a free page
        int endP = tot__physical_pages - 1;
        while (endP >= 0 && get_bit_at_index(my_physical__bit_map, endP) != 0) {
            endP--;
        }
        if (endP >= 0) {
            set_bit_at_index(my_physical__bit_map, endP);
            *get_pde = endP;
        }
    }

    // Map the page in the page table
    pte_t *get_PTE = (pte_t *)&my_physical__memory[*get_pde] + index_of_inside;
    *get_PTE = get_ppn;

    // Update TLB
    miss__TLB++;
    add_TLB(get_vpn, get_ppn);

    return -1;
}




/*Function that gets the next available page
*/
unsigned long get_next_avail(int num_pages) {
    int start_p = 0;
    for (int checker_i = 0; checker_i < tot__virtual_pages; ) {
        if (get_bit_at_index(my_virtual__bit_map, checker_i) == 0) {
            int checker_t = 1;
            int checker_n = checker_i + 1;
            for (; checker_n < tot__virtual_pages && checker_t < num_pages &&
                   get_bit_at_index(my_virtual__bit_map, checker_n) == 0; checker_t++, checker_n++) {
            }
            if (checker_t == num_pages) {
                start_p = checker_i;
                return start_p;
            }
            checker_i = checker_n;  // Move to the next index after the found range
        } else {
            checker_i++;
        }
    }
    return start_p;  
}


/* Function responsible for allocating pages and used by the benchmark */
void *t_malloc(unsigned int num_bytes) {

    /* 
    * HINT: If the physical memory is not yet initialized, then allocate and initialize.
    */

    /* 
    * HINT: If the page directory is not initialized, then initialize the
    * page directory. Next, using get_next_avail(), check if there are free pages. If
    * free pages are available, set the bitmaps and map a new page. Note, you will 
    * have to mark which physical pages are used. 
    */

    pthread_mutex_lock(&mutex);
    if (my_physical__memory == NULL) {
		set_physical_mem();
	}

    // Getting the tot number of pages needed
    int npages = num_bytes / PGSIZE + ((num_bytes % PGSIZE) ? 1 : 0);
    // creating temp array of size total number of pages
    int temp_pp_arr[npages];
    int checker_c = 0;
    int checker_i = 0;
    // Conditioning that to get the avaible pages for the physical  
    while(checker_c < npages && checker_i < tot__physical_pages) {
        if (get_bit_at_index(my_physical__bit_map, checker_i) == 0){
            temp_pp_arr[checker_c] = checker_i;
            checker_c++;
        }
        checker_i++;
    }
    // Take care of the number pages found are not less then the pages
    if(checker_c < npages){
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    int start_p = get_next_avail(npages);
    // If the get next avaible position is successful then the go aheah, otherwise return -1
    if (start_p == -1) {
        pthread_mutex_unlock(&mutex);
        return NULL;
    }


    int end_p = start_p + npages-1;
    checker_c = 0;
    int i = start_p;

    while (i <= end_p) {
        unsigned long checker_tVrtlAddr = i << OSet_B;
        unsigned long checker_tPhyAddr = temp_pp_arr[checker_c] << OSet_B;
        set_bit_at_index(my_virtual__bit_map, i);
        set_bit_at_index(my_physical__bit_map, temp_pp_arr[checker_c]);

        // page_map((pde_t *)(my_physical__memory + tot__physical_pages - 1), (void *)checker_tVrtlAddr, (void *)checker_tPhyAddr);
        page_map(NULL, (void *)checker_tVrtlAddr, (void *)checker_tPhyAddr);

        checker_c++;
        i++;
    }

    unsigned long va = start_p << OSet_B;
    pthread_mutex_unlock(&mutex);
    return (void *)va;
}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void t_free(void *va, int size) {

    pthread_mutex_lock(&mutex);
    // Part 1: Free the page table entries starting from this virtual address (va).
    unsigned long start_p = (unsigned long)va >> OSet_B;
    int npages = size / PGSIZE + ((size % PGSIZE) ? 1 : 0);
    for (unsigned long i = start_p; i < start_p + npages; i++) {
        if (!get_bit_at_index(my_virtual__bit_map, i)) {
            pthread_mutex_unlock(&mutex);
            return;
        }
    }
    for (unsigned long i = start_p; i < start_p + npages; i++) {
        unset_bit_at_index(my_virtual__bit_map, i);
        pte_t *pa_ptr = translate(NULL, va);
        if (pa_ptr != NULL) {
            pte_t pa = *pa_ptr;
            unsigned long phy_p = pa >> OSet_B;
            unset_bit_at_index(my_physical__bit_map, phy_p);
            char *my_physical__memory = (char *)pa_ptr;
            memset(my_physical__memory, 0, PGSIZE);
        }
    }
    remove_TLB(start_p, npages);
    pthread_mutex_unlock(&mutex);
}





/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
 * The function returns 0 if the put is successfull and -1 otherwise.
*/
// Changed
void put_value(void *va, void *val, int size) {
    pthread_mutex_lock(&mutex);

    char *source = (char *)val;
    char *destination = (char *)va;
    
    while (size > 0) {
        pte_t *page_table_entry = (translate(NULL, destination));
        char *page_address = (char *)page_table_entry;
        unsigned int virtual_page = (unsigned int)destination >> OSet_B;
        if (get_bit_at_index(my_virtual__bit_map, virtual_page) == 0) {
            pthread_mutex_unlock(&mutex);
            return;
        }
        size_t bytes_to_copy = PGSIZE - ((unsigned int)destination & (PGSIZE - 1));
        bytes_to_copy = (size < bytes_to_copy) ? size : bytes_to_copy;
        memcpy((void *)page_table_entry, source, bytes_to_copy);
        source += bytes_to_copy;
        destination += bytes_to_copy;
        size -= bytes_to_copy;
    }

    pthread_mutex_unlock(&mutex);
}



/*Given a virtual address, this function copies the contents of the page to val*/
void get_value(void *va, void *val, int size) {
    pthread_mutex_lock(&mutex);
    char *source = (char *)va;
    char *destination = (char *)val;
    unsigned int virtual_page_start = (unsigned int)source >> OSet_B;
    unsigned int virtual_page_end = (unsigned int)(source + size - 1) >> OSet_B;
    for (unsigned int j = virtual_page_start; j <= virtual_page_end; j++) {
        int is_mapped = get_bit_at_index(my_virtual__bit_map, j);
        if (is_mapped == 0) {
            pthread_mutex_unlock(&mutex);
            return;
        }
    }
    while (size > 0) {
        pte_t *page_table_entry = (translate(NULL, source));
        char *page_address = (char *)page_table_entry;
        size_t bytes_to_copy = PGSIZE - ((unsigned int)source & (PGSIZE - 1));
        bytes_to_copy = (size < bytes_to_copy) ? size : bytes_to_copy;
        memcpy((void *)destination, (void *)page_address, bytes_to_copy);
        destination += bytes_to_copy;
        source += bytes_to_copy;
        size -= bytes_to_copy;
    }
    pthread_mutex_unlock(&mutex);
}





/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void mat_mult(void *mat1, void *mat2, int size, void *answer) {

    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
     * matrix accessed. Similar to the code in test.c, you will use get_value() to
     * load each element and perform multiplication. Take a look at test.c! In addition to 
     * getting the values from two matrices, you will perform multiplication and 
     * store the result to the "answer array"
     */
    int x, y, val_size = sizeof(int);
    int i, j, k;
    for (i = 0; i < size; i++) {
        for(j = 0; j < size; j++) {
            unsigned int a, b, c = 0;
            for (k = 0; k < size; k++) {
                int address_a = (unsigned int)mat1 + ((i * size * sizeof(int))) + (k * sizeof(int));
                int address_b = (unsigned int)mat2 + ((k * size * sizeof(int))) + (j * sizeof(int));
                get_value( (void *)address_a, &a, sizeof(int));
                get_value( (void *)address_b, &b, sizeof(int));
                // printf("Values at the index: %d, %d, %d, %d, %d\n", 
                //     a, b, size, (i * size + k), (k * size + j));
                c += (a * b);
            }
            int address_c = (unsigned int)answer + ((i * size * sizeof(int))) + (j * sizeof(int));
            // printf("This is the c: %d, address: %x!\n", c, address_c);
            put_value((void *)address_c, (void *)&c, sizeof(int));
        }
    }
}
