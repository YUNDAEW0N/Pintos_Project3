/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "kernel/hash.h"
#include "threads/mmu.h"

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

// int get_offset(void *kva);
// void set_offset(void *va, int offset);

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;
	type = (enum vm_type)VM_TYPE(type);

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		struct page *page = (struct page *)calloc(1,sizeof(struct page));
		if (!page)
			return false;

		switch (VM_TYPE(type)){

			case VM_ANON:
				uninit_new(page,upage,init,type,aux,anon_initializer);
				break;
			case VM_FILE:
				uninit_new(page,upage,init,type,aux,file_backed_initializer);
				break;
		}

		/* TODO: Insert the page into the spt. */
		page->writable = writable;
		return spt_insert_page(spt,page);

	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt, void *va) {
	struct page *page = NULL;
	/* TODO: Fill this function. */
	struct hash_elem *e;
	page = malloc(sizeof(struct page));
	page->va = va;
	e = hash_find (&spt,&page->spt_elem); 

	return e != NULL ? hash_entry(e, struct page, spt_elem) : NULL;
}


/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt,
		struct page *page) {
	/* TODO: Fill this function. */
	
	return hash_insert(&spt, &page->spt_elem) == NULL ? true : false;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	hash_delete(&spt,&page->spt_elem);

	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim  = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return victim;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	

	frame = (struct frame *)calloc(1,sizeof(struct frame));
	frame->kva = palloc_get_page(PAL_USER);
	
	//swap out 구현 해야 함
	if (!frame->kva){
		PANIC("to do");
	}

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	
		
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
/* spt_find_page를 통해 SPT를 참조하여 faulted 주소에 해당하는 페이지 구조체를
 * 해결하도록 해야함. 인자로 f, fault_addr, user, write, not_present 를 받음.
 * f = intr_frame / fault_addr = fault 주소 / write = write 여부 / not_present = 물리프레임이 있는지? 
 *  not_present = true 이면 : 할당된 물리 프레임이 존재하지 않아서 발생한 예외
 *  not_present = false 이면 : 물리 프레임이 할당 되었지만 page_fault가 일어난 것이므로
 *  read_only page 에 write 를 한 경우가 되니 예외 처리로 return false 하면 됨
 *  또 not_present가 true 임에도 불구하고 read_only_pate에 write 요청 할수 있으므로 그것도
 *  예외 처리*/
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct thread *curr = thread_current();
	struct supplemental_page_table *spt = &curr->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	// 예외 처리 1

	if (addr == NULL)
		return false;

	if (is_kernel_vaddr(addr))
		return false;


	if(not_present)
	{
		page = spt_find_page(spt,addr);
		if(page == NULL)
			return false;
		if(write == 1 && page->writable == 0) // read_only page 에 write 요청 한 경우
			return false;
		
		return vm_do_claim_page(page);	
	}

	return false;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va) {
	struct page *page = NULL;
	struct thread *curr = thread_current();
	/* TODO: Fill this function */
	// page = (struct page *)calloc(1,sizeof(struct page));
	// page->va = va;
	page = spt_find_page(&curr->spt,va);
	if(page == NULL)
		return false;

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	struct thread *curr = thread_current();
	pml4_set_page(curr->pml4,page->va,frame->kva,page->writable);

	return swap_in (page, frame->kva);
}


void
supplemental_page_table_init (struct supplemental_page_table *spt) {
	
	hash_init(&spt->spt_hash, page_hash, page_less, NULL);		
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst,
		struct supplemental_page_table *src) {

}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}