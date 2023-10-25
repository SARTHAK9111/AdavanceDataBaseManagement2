--------* CS525 - Advanced Database Organization *--------
-----------* Assignment - 2 - Buffer Manager *-----------

---------------* Implemented By Group - 49 *--------------
- Sarthak Raghuvanshi (A20543280)
- Amarthya Parvatha Reddy (A20550000)
- Neel Patel (A20524638)

==> Steps to run the buffer Manager:

> All the test cases are implemented in "test_assign2_1.c"  and "test_assign2_2.c" file.

> Following command will compile all the c files into it's consecutive  0 (object) files and consequently makes a single executable file "test_assign2.exe" :-
   
  $ make all : for all the test cases.
  $ make : for "test_assign2_1.c" 

> Following command will run the executable file "test_assign2.exe" :

  $ ./test_assign2 : for runing the file "test_assign2_1.c".
  $ ./test_assign2_2 : for running the file "test_assign2_2.c".

----------------------------------------------------------------------------------------------------------------------------------------

==> Page Replacement Strategies implemented :

a) pinPageLRU():

- Checks if the desired page is in the buffer pool and initializes pageFound.
- If the page is not found, it enqueues the page into buffer pool and if the page is found, it updates the fixCount of the page and the BM_PageHandle with data and page number.
- The function guarantees that the accessed page is relocated to the beginning of the LRU queue by making adjustments to the linked list of pageInfo objects.


b) pinpageLRUK():

-Checks if the desired page is already in the buffer pool. If not found, the function enqueues the page.
-If the page is found, it checks the fixCount and considers the most two recent references to decide which page to replace. (The page with a lower fixed count will be selected for replacement).
-The function then updates the 'fixcount' of the accessed page and rearranges the linked list of pageInfo objects, so that the accessed page is positioned at the front.

c) pinPageFIFO():

- Checks if the desried page is already in the buffer. If not found, it searches for an available slot in the buffer to load the new page. 
- If there are empty slots, it selects one, sets the necessary information, and loads the page into it.
- If the slots are all occupied, it identifies the oldest page based on the order they were loaded, and that page is selected for replacement. The selected page's data is wrtiten to the disk if it's dirty, and the new page is loaded into its place.
- If the page is not found in the buffer, it increments the fixCount for that page and updates the BM_PageHandle with the page's data and number.

----------------------------------------------------------------------------------------------------------------------------------------

==> Data Structures used:

- Data Structures defined : pageInfo and Queue, which represent information about individual pages and a queue of pages, respectively.
- Queue contains a linked list of pageInfo objects and keeps track of the filled frames, total frames, and a clock hand (which is used in some page replacement strategies).

----------------------------------------------------------------------------------------------------------------------------------------

==> Functions Implementated :


a) createQueue(BM_BufferPool *const bm) :
   - Initializes a queue of empty pages for the buffer pool.
   - Creates and configures `pageInfo` structures for the frames in the pool.
   
b) EmptyQueue():
   - Checks if the queue is empty.
   - Returns an indicator to inform whether the queue is empty.

c) createNewList(const PageNumber pageNum):
   - Creates a new `pageInfo` structure for a given page number.
   - Initializes page-related data such as page number, fix count, and buffer data.

d) deQueue():
   - Removes a page from the queue based on the page replacement strategy.
   - Decreases the filled frames count and updates the queue accordingly.
   - Checks if the queue is empty and returns an error if it is.

e) Enqueue(BM_PageHandle *const page, const PageNumber pageNum, BM_BufferPool *const bm):
   - Adds a new page to the queue, possibly removing a page if the pool is full.
   - Loads page data into memory.
   - Updates the queue with the new page.

f) updateBM_BufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy):
   - Updates the properties of the buffer pool.
   - Sets attributes like the page file name, number of pages, and replacement strategy.

g) initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData):
   - Initializes the buffer pool based on the provided parameters.
   - Opens the specified page file for use.

h) shutdownBufferPool(BM_BufferPool *const bm):
   - Cleans up the buffer pool and writes modified pages back to disk if necessary.
   - Frees allocated resources and closes the page file.

i) forceFlushPool(BM_BufferPool *const bm):
   - Writes all dirty pages to disk.
   - Ensures that any changes made in memory are saved to the page file.

j) unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page):
    - Decrements the fix count for a page.
    - Checks if the page exists in the buffer pool.

k) forcePage(BM_BufferPool *const bm, BM_PageHandle *const page):
    - Writes a specific page to disk if it's dirty.
    - Returns an error code if the write operation fails.

l) markDirty(BM_BufferPool *const bm, BM_PageHandle *const page):
    - Marks a page as dirty, indicating that changes have been made.
    - Checks if the page exists in the buffer pool.

m) getFrameContents(BM_BufferPool *const bm):
    - Retrieves an array of page numbers contained in the frames.
    - Returns an array indicating the page numbers currently in memory.

n) getDirtyFlags(BM_BufferPool *const bm):
    - Retrieves an array of flags indicating whether pages are dirty.
    - Returns an array indicating the dirty status of pages in memory.

o) getFixCounts(BM_BufferPool *const bm):
    - Retrieves an array of fix counts for pages.
    - Returns an array indicating the number of times each page is pinned.

p) getNumReadIO(BM_BufferPool *const bm):
    - Returns the number of read I/O operations that have occurred.

q) getNumWriteIO(BM_BufferPool *const bm):
    - Returns the number of write I/O operations that have occurred.

r) pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum):
    - Orchestrates page pinning based on the selected page replacement strategy.
    - Calls the appropriate strategy function (e.g., LRU, FIFO, LRU_K) to manage the pinning process.