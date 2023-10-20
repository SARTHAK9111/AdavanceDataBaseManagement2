#include "buffer_mgr.h"
#include "dberror.h"
#include "storage_mgr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RC_QUEUE_IS_EMPTY 5;
#define RC_NO_FREE_BUFFER_ERROR 6;


typedef struct pageInfo{
	char *bufferData;
	RC isDirty;
	RC fixCount;
	RC pageNum;
	RC frameNum;
	struct pageInfo *nextPageInfo;
	struct pageInfo *prevPageInfo;
} pageInfo;

typedef struct Queue{
	pageInfo *head_ptr;
	pageInfo *tail_ptr;
	RC filledframes;
	RC totalNumOfFrames;
	pageInfo *clockHand ;
} Queue;

pageInfo *createFrame(pageInfo *);
pageInfo *createBufferOfSize(RC, pageInfo *);

SM_FileHandle *fh;
Queue *q;
RC readIndex;
RC writeIndex;






// Custom Function for maintaining the Queue
void createQueue(BM_BufferPool *const bm)
{	
	// Creating Array of numPagesSize
	pageInfo *newPage[bm->numPages];
	
	RC lastPage = (bm->numPages) - 1;
	
	for ( int k = 0; k <= lastPage; k++)
	{
		newPage[k] = (pageInfo *)malloc(sizeof(pageInfo));	
		newPage[k]->frameNum = k;
		newPage[k]->isDirty = 0;
		newPage[k]->fixCount = 0;
		newPage[k]->pageNum = -1;
		newPage[k]->bufferData = (char *)calloc(PAGE_SIZE, sizeof(char));
	}
	
	for (int k = 0; k <= lastPage; k++)
	{
		
		if (k == 0)
		{
			newPage[k]->prevPageInfo = NULL;
			newPage[k]->nextPageInfo = newPage[k + 1];
		}

		else if (k == lastPage)
		{
			newPage[k]->nextPageInfo = NULL;
			newPage[k]->prevPageInfo = newPage[k - 1];
		}
		else
		{

			newPage[k]->nextPageInfo = newPage[k + 1];
			newPage[k]->prevPageInfo = newPage[k - 1];
		}
	}
	q->head_ptr = newPage[0];
	q->tail_ptr = newPage[lastPage];
	q->filledframes = 0;
	q->totalNumOfFrames = bm->numPages;
	q->clockHand = NULL;
}

RC isEmpty(){
	return (q->filledframes == 0);
}

pageInfo *createNode(const PageNumber pageNum){
	pageInfo *newpinfo = (pageInfo *)malloc(sizeof(pageInfo));
	char *c = (char *)calloc(PAGE_SIZE, sizeof(char));

	newpinfo->pageNum = pageNum;
	newpinfo->isDirty = 0;
	newpinfo->frameNum = 0;
	newpinfo->fixCount = 1;
	newpinfo->bufferData = c;
	newpinfo->prevPageInfo = NULL;
	newpinfo->nextPageInfo = NULL;

	return newpinfo;
}

RC deQueue(){
	if (isEmpty())
	{
		return RC_QUEUE_IS_EMPTY;
	}

	pageInfo *p = q->head_ptr;
	for (RC i = 0; i < q->filledframes; i++)
	{
		if (i == (q->filledframes - 1))
		{
			q->tail_ptr = p;
		}
		else
			p = p->nextPageInfo;
	}

	RC tail_pagenum;
	RC pageDelete = 0;
	pageInfo *pinfo = q->tail_ptr;
	pageInfo* current_next =pinfo->nextPageInfo;
	pageInfo* current_prev =pinfo->prevPageInfo;
	for (RC i = 0; i < q->totalNumOfFrames; i++)
	{
		
		if ((pinfo->fixCount) == 0)
		{
			
			if (pinfo->pageNum == q->tail_ptr->pageNum)
			{
				pageDelete = pinfo->pageNum;
				q->tail_ptr = (q->tail_ptr->prevPageInfo);
				q->tail_ptr->nextPageInfo = NULL;
			}
			else
			{
				pageDelete = pinfo->pageNum;
				
				pinfo->prevPageInfo->nextPageInfo = current_next;
				pinfo->nextPageInfo->prevPageInfo = current_prev;
			}
		}
		else
		{
			tail_pagenum = pinfo->pageNum;
			// printf("\n next node%d", pinfo->pageNum);
			pinfo = current_prev;
		}
	}

	if (tail_pagenum == q->tail_ptr->pageNum)
	{
		// m printf("\nBuffer not free");
		return 0; // Add error
	}

	if (pinfo->isDirty == 1)
	{
		writeBlock(pinfo->pageNum, fh, pinfo->bufferData);
		// printf("write block");
		writeIndex++;
	}

	q->filledframes--;

	return pageDelete;
}

RC Enqueue(BM_PageHandle *const page, const PageNumber pageNum, BM_BufferPool *const bm){

	RC pageDelete = -1;
	if (q->filledframes == q->totalNumOfFrames)
	{ // If frames are full remove a page
		pageDelete = deQueue();
	}

	pageInfo *pinfo = createNode(pageNum);
	// pinfo->bufferData=bm->mgmtData;

	if (isEmpty())
	{

		readBlock(pinfo->pageNum, fh, pinfo->bufferData);
		page->data = pinfo->bufferData;
		readIndex++;

		pinfo->frameNum = q->head_ptr->frameNum;
		pinfo->nextPageInfo = q->head_ptr;
		q->head_ptr->prevPageInfo = pinfo;
		pinfo->pageNum = pageNum;
		page->pageNum = pageNum;
		q->head_ptr = pinfo;
		// q->tail_ptr=pinfo;
		/*pageInfo *p = q->head_ptr;
		for (RC i = 0; i <= q->filledframes; i++) {
			if (i == (q->filledframes)) {
				q->tail_ptr = p;
			} else
				p = p->nextPageInfo;

		} */
	}
	else
	{
		readBlock(pageNum, fh, pinfo->bufferData);
		if (pageDelete == -1)
			pinfo->frameNum = q->head_ptr->frameNum + 1;
		else
			pinfo->frameNum = pageDelete;
		page->data = pinfo->bufferData;
		readIndex++;
		pinfo->nextPageInfo = q->head_ptr;
		q->head_ptr->prevPageInfo = pinfo;
		q->head_ptr = pinfo;
		page->pageNum = pageNum;
	}
	q->filledframes++;

	return RC_OK;
}



void updateBM_BufferPool(BM_BufferPool *const bm, const char *const pageFileName, const RC numPages, ReplacementStrategy strategy){

	char *buffersize = (char *)calloc(numPages, sizeof(char) * PAGE_SIZE);

	bm->pageFile = (char *)pageFileName;
	bm->numPages = numPages;
	bm->strategy = strategy;
	bm->mgmtData = buffersize;
	// free(buffersize);
}

RC LRU(BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum){

	RC pageFound = 0;
	pageInfo *pinfo = q->head_ptr;
	RC i;
	RC oldestCount=0;
	for (i = 0; i < bm->numPages; i++)
	{
		if (pageFound == 0)
		{
			if (pinfo->pageNum == pageNum)
			{
				pageFound = 1;
				break;
			}
			else
				pinfo = pinfo->nextPageInfo;
		}
	}

	if (pageFound == 0)
		Enqueue(page, pageNum, bm);

	if (pageFound == 1)
	{
		pageInfo *oldest = q->head_ptr;
        pageInfo *temp = q->head_ptr;

		while (temp != NULL)
        {
				
                if (temp->fixCount == 0 && temp->pageNum != -1)
                {
                    if (oldestCount == 0)
                    {
                        oldest = temp;
                        break;
                    }
                    oldest = temp;
                    oldestCount--;
                }
                temp = temp->nextPageInfo;
		}
		pinfo->fixCount++;
		page->data = pinfo->bufferData;
		page->pageNum = pageNum;
		if (pinfo == q->head_ptr)
		{
			pinfo->nextPageInfo = q->head_ptr;
			q->head_ptr->prevPageInfo = pinfo;
			q->head_ptr = pinfo;
		}

		if (pinfo != q->head_ptr)
		{
			pinfo->prevPageInfo->nextPageInfo = pinfo->nextPageInfo;
			if (pinfo->nextPageInfo)
			{
				pinfo->nextPageInfo->prevPageInfo = pinfo->prevPageInfo;

				if (pinfo == q->tail_ptr)
				{
					q->tail_ptr = pinfo->prevPageInfo;
					q->tail_ptr->nextPageInfo = NULL;
				}
				pinfo->nextPageInfo = q->head_ptr;
				pinfo->prevPageInfo = NULL;
				pinfo->nextPageInfo->prevPageInfo = pinfo;
				q->head_ptr = pinfo;
			}
		}
	}
	/*pageInfo *p = q->head_ptr;
	for (RC i = 0; i < q->totalNumOfFrames; i++) {
		p->frameNum = i;
		p = p->nextPageInfo;
	}*/
	return RC_OK;
}

RC LRU_K(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum){
	RC pageFound = 0;
	pageInfo *pinfo = q->head_ptr;
	RC i;
	RC oldestCount=2;
	for (i = 0; i < bm->numPages; i++)
	{
		if (pageFound == 0)
		{
			if (pinfo->pageNum == pageNum)
			{
				pageFound = 1;
				break;
			}
			else
				pinfo = pinfo->nextPageInfo;
		}
	}

	if (pageFound == 0)
		Enqueue(page, pageNum, bm);

	if (pageFound == 1)
	{
		pageInfo *oldest = q->head_ptr;
        pageInfo *temp = q->head_ptr;

		while (temp != NULL)
        {
				
                if (temp->fixCount == 0 && temp->pageNum != -1)
                {
                    if (oldestCount == 0)
                    {
                        oldest = temp;
                        break;
                    }
                    oldest = temp;
                    oldestCount--;
                }
                temp = temp->nextPageInfo;
		}
		pinfo->fixCount++;
		page->data = pinfo->bufferData;
		page->pageNum = pageNum;
		if (pinfo == q->head_ptr)
		{
			pinfo->nextPageInfo = q->head_ptr;
			q->head_ptr->prevPageInfo = pinfo;
			q->head_ptr = pinfo;
		}

		if (pinfo != q->head_ptr)
		{
			pinfo->prevPageInfo->nextPageInfo = pinfo->nextPageInfo;
			if (pinfo->nextPageInfo)
			{
				pinfo->nextPageInfo->prevPageInfo = pinfo->prevPageInfo;

				if (pinfo == q->tail_ptr)
				{
					q->tail_ptr = pinfo->prevPageInfo;
					q->tail_ptr->nextPageInfo = NULL;
				}
				pinfo->nextPageInfo = q->head_ptr;
				pinfo->prevPageInfo = NULL;
				pinfo->nextPageInfo->prevPageInfo = pinfo;
				q->head_ptr = pinfo;
			}
		}
	}
	/*pageInfo *p = q->head_ptr;
	for (RC i = 0; i < q->totalNumOfFrames; i++) {
		p->frameNum = i;
		p = p->nextPageInfo;
	}*/
	return RC_OK;
}

RC FIFO(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum){
	RC pageFound = 0, numPages;
	numPages = bm->numPages;
	pageInfo *list = NULL, *temp = NULL, *temp1 = NULL;
	list = q->head_ptr;
	for (RC i = 0; i < numPages; i++)
	{
		if (pageFound == 0)
		{
			if (list->pageNum == pageNum)
			{
				pageFound = 1;
				break;
			}
			else
				list = list->nextPageInfo;
		}
	}

	if (pageFound == 1)
	{
		list->fixCount++;
		page->data = list->bufferData;
		page->pageNum = pageNum;

		// free(list);
		return RC_OK;
	}
	// free(list);
	temp = q->head_ptr;

	// printf("\n\n q->filledframes %d\n",q->filledframes);
	// printf("\n\n q->totalNumOfFrames %d\n",q->totalNumOfFrames);

	while (q->filledframes < q->totalNumOfFrames)
	{
		if (temp->pageNum == -1)
		{
			temp->fixCount = 1;
			temp->isDirty = 0;
			temp->pageNum = pageNum;
			page->pageNum = pageNum;
			// temp->bufferData = bm->mgmtData;
			q->filledframes = q->filledframes + 1;

			readBlock(temp->pageNum, fh, temp->bufferData);
			// printf("\n\n temp->bufferData=%s hello \n",temp->bufferData);
			page->data = temp->bufferData;
			readIndex++;
			return RC_OK;
		}
		else
			temp = temp->nextPageInfo;
	}

		
	pageInfo *addnode = (pageInfo *)malloc(sizeof(pageInfo));
	addnode->fixCount = 1;
	addnode->isDirty = 0;
	addnode->pageNum = pageNum;
	addnode->bufferData = NULL;
	addnode->nextPageInfo = NULL;
	page->pageNum = pageNum;
	addnode->prevPageInfo = q->tail_ptr;
	temp = q->head_ptr;
	RC i;
	for (i = 0; i < numPages; i++)
	{
		if ((temp->fixCount) == 0)
			break;
		else
			temp = temp->nextPageInfo;
	}

	if (i == numPages)
	{
		return RC_NO_FREE_BUFFER_ERROR;
	}

	temp1 = temp;
	if (temp == q->head_ptr)
	{

		q->head_ptr = q->head_ptr->nextPageInfo;
		q->head_ptr->prevPageInfo = NULL;
	}
	else if (temp == q->tail_ptr)
	{
		q->tail_ptr = temp->prevPageInfo;
		addnode->prevPageInfo = q->tail_ptr;
	}
	else
	{
		pageInfo * current_next= temp->nextPageInfo;
		pageInfo * current_prev= temp->prevPageInfo;
		temp->prevPageInfo->nextPageInfo = current_next;
		temp->nextPageInfo->prevPageInfo = current_prev;
	}

	if (temp1->isDirty == 1)
	{
		writeBlock(temp1->pageNum, fh, temp1->bufferData);
		writeIndex++;
	}
	addnode->bufferData = temp1->bufferData;
	addnode->frameNum = temp1->frameNum;

	readBlock(pageNum, fh, addnode->bufferData);
	page->data = addnode->bufferData;
	readIndex++;

	q->tail_ptr->nextPageInfo = addnode;
	q->tail_ptr = addnode;
	return RC_OK;
}
 
//Given Function to be implemented 
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const RC numPages, ReplacementStrategy strategy, void *stratData){
	readIndex = 0;
	writeIndex = 0;
	fh = (SM_FileHandle *)malloc(sizeof(SM_FileHandle));
	q = (Queue *)malloc(sizeof(Queue));

	updateBM_BufferPool(bm, pageFileName, numPages, strategy);

	openPageFile(bm->pageFile, fh);

	createQueue(bm);

	// free(q);
	return RC_OK;
}

RC shutdownBufferPool(BM_BufferPool *const bm){
	RC i=0;
	pageInfo *pinfo = NULL, *temp = NULL;

	pinfo = q->head_ptr;

	while (i < q->filledframes)
	{
		if (pinfo->fixCount == 0 && pinfo->isDirty == 1)
		{
			writeBlock(pinfo->pageNum, fh, pinfo->bufferData);
			writeIndex++;
			pinfo->isDirty = 0;
		}
		pinfo = pinfo->nextPageInfo;
		i++;
	}

	
	closePageFile(fh);

	return RC_OK;
}

RC forceFlushPool(BM_BufferPool *const bm){
	RC i;
	pageInfo *temp1;
	temp1 = q->head_ptr;
	for (i = 0; i < q->totalNumOfFrames; i++)
	{
		if ((temp1->isDirty == 1) && (temp1->fixCount == 0))
		{
			writeBlock(temp1->pageNum, fh, temp1->bufferData);
			writeIndex++;
			temp1->isDirty = 0;
			// printf("\n----------Inside forceflush--------------");
		}

		temp1 = temp1->nextPageInfo;
	}
	return RC_OK;
}

RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page){
	pageInfo *current;
	RC i;
	current = q->head_ptr;

	for (i = 0; i < bm->numPages; i++)
	{ 
		if (current->pageNum == page->pageNum)
			break;
		current = current->nextPageInfo;
	}

	if (i == bm->numPages)
		return RC_READ_NON_EXISTING_PAGE;
	else
		current->fixCount = current->fixCount - 1;
	return RC_OK;
}

RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page){
	pageInfo *temp;
	RC i;
	temp = q->head_ptr;

	for (i = 0; i < bm->numPages; i++)
	{
		if (temp->pageNum == page->pageNum)
			break;
		temp = temp->nextPageInfo;
	}

	RC flag;

	if (i == bm->numPages)
		return 1; // give error code
	if ((flag = writeBlock(temp->pageNum, fh, temp->bufferData)) == 0)
		writeIndex++;
	else
		return RC_WRITE_FAILED;

	return RC_OK;
}

RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page){
	pageInfo *temp;
	RC i;
	temp = q->head_ptr;

	for (i = 0; i < bm->numPages; i++)
	{
		if (temp->pageNum == page->pageNum)
			break;
		if (temp->nextPageInfo != NULL)
			temp = temp->nextPageInfo;
	}

	if (i == bm->numPages)
		return RC_READ_NON_EXISTING_PAGE;
	temp->isDirty = 1;
	return RC_OK;
}

RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum){
	RC res;
	if (bm->strategy == RS_FIFO)
		res = FIFO(bm, page, pageNum);
	else if (bm->strategy == RS_LRU)
		res = LRU(bm, page, pageNum);
	else
		res = LRU_K(bm, page, pageNum);
	return res;
}
//------------Statistics Functions-----------------
PageNumber *getFrameContents(BM_BufferPool *const bm){
	PageNumber(*pages)[bm->numPages];
	RC i;
	pages = calloc(bm->numPages, sizeof(PageNumber));
	pageInfo *temp;
	for (i = 0; i < bm->numPages; i++)
	{
		for (temp = q->head_ptr; temp != NULL; temp = temp->nextPageInfo)
		{
			if (temp->frameNum == i)
			{
				(*pages)[i] = temp->pageNum;
				break;
			}
		}
	}
	return *pages;
}

bool *getDirtyFlags(BM_BufferPool *const bm){
	bool(*isDirty)[bm->numPages];
	RC i;
	isDirty = calloc(bm->numPages, sizeof(PageNumber));
	pageInfo *temp;

	for (i = 0; i < bm->numPages; i++)
	{
		for (temp = q->head_ptr; temp != NULL; temp = temp->nextPageInfo)
		{
			if (temp->frameNum == i)
			{
				if (temp->isDirty == 1)
					(*isDirty)[i] = TRUE;
				else
					(*isDirty)[i] = FALSE;
				break;
			}
		}
	}
	return *isDirty;
}

RC *getFixCounts(BM_BufferPool *const bm){
	RC(*fixCounts)[bm->numPages];
	RC i;
	fixCounts = calloc(bm->numPages, sizeof(PageNumber));
	pageInfo *temp;

	for (i = 0; i < bm->numPages; i++)
	{
		for (temp = q->head_ptr; temp != NULL; temp = temp->nextPageInfo)
		{
			if (temp->frameNum == i)
			{
				(*fixCounts)[i] = temp->fixCount;
				break;
			}
		}
	}
	return *fixCounts;
}

RC getNumReadIO(BM_BufferPool *const bm){
	return readIndex;
}

RC getNumWriteIO(BM_BufferPool *const bm){
	return writeIndex;
}
