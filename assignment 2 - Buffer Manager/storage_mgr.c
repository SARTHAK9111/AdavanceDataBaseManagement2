#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage_mgr.h"
#include "dberror.h"
#include <stdbool.h>
#include <unistd.h>

FILE *file;

bool StorageManagerStatus = false;
bool fileIsOpen = false;
void initStorageManager(void)
{
	if (StorageManagerStatus == false)
	{
		StorageManagerStatus = true;
		printf("\n <----- Starting Storage Manager------->\n");
		printf("\n       <--------BY Group 49-------->      \n");
	}
}

// Creating Page file
RC createPageFile(char *fileName)
{
	
	if (StorageManagerStatus != true)
    {
        return RC_STORAGE_MGR_NOT_INIT;
    }
    RC memorySize = PAGE_SIZE * sizeof(char);
    char *memoryBlock = malloc(memorySize);

    file= fopen(fileName, "w+");

    if (file == 0)
    {
        return RC_FILE_NOT_FOUND;
    }

    fileIsOpen = true;
    memset(memoryBlock, '\0', PAGE_SIZE);
    fwrite(memoryBlock, sizeof(char), PAGE_SIZE, file);
    fclose(file);

    free(memoryBlock);
    return RC_OK;
}
// Opening Page file
RC openPageFile(char *fileName, SM_FileHandle *fHandle)
{

	if (StorageManagerStatus != true)
    {
        return RC_STORAGE_MGR_NOT_INIT;
    }

    if (access(fileName, F_OK) == -1)
    {
        return RC_FILE_NOT_FOUND;
    }
    FILE *fp = fopen(fileName, "r+");

    fseek(fp, 0, SEEK_END); // pointing to end

    int LastByte = ftell(fp);
    int length = LastByte + 1;

    fHandle->totalNumPages = length / PAGE_SIZE;
    
    fHandle->curPagePos = 0;
    fHandle->fileName = fileName;
    fHandle->mgmtInfo = fp;

    fseek(fp, 0, SEEK_SET); // pointing back to start;

    return RC_OK;
	
}

// Closing Page file
RC closePageFile(SM_FileHandle *fHandle)
{

	if (StorageManagerStatus != true)
	{
		return RC_STORAGE_MGR_NOT_INIT;
	}
	file = fHandle->mgmtInfo;

    if (fclose(file) == 0)
    {
        fHandle->mgmtInfo = NULL;
        fileIsOpen = false;
        return RC_OK;
    }
    else
    {
        return RC_FILE_NOT_FOUND;
    }
}

// Destroying Page file
RC destroyPageFile(char *fileName)
{
    // Use the remove function to delete the file
    if (fileIsOpen == true)
    {
        return RC_FILE_NOT_CLOSE;
    }
    int fileflag = remove(fileName);
    if (fileflag != 0)
    {
        // If remove fails, return an error code
        return RC_FILE_NOT_FOUND; // You can define this error code in dberror.h
    }
    // printf("\n in Destroy function \n");
    return RC_OK;
}

// Reading pageNum^th block from the file
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	 if (StorageManagerStatus != true)
        return RC_STORAGE_MGR_NOT_INIT;

    if (fHandle == NULL || fHandle->fileName == NULL)
        return RC_FILE_NOT_FOUND;

    if (pageNum < 0 || pageNum >= fHandle->totalNumPages)
        return RC_READ_NON_EXISTING_PAGE;
  
    file= fHandle->mgmtInfo;
    RC position = pageNum * PAGE_SIZE;
    fseek(file, position, SEEK_SET);
    RC ReadErrorFlag = fread(memPage, sizeof(char), PAGE_SIZE, file);
  
    if (ReadErrorFlag < PAGE_SIZE || ReadErrorFlag > PAGE_SIZE)
    {
       
        return RC_READ_NON_EXISTING_PAGE;
    }
    fHandle->curPagePos = pageNum;
   
    return RC_OK;
}

// Get current block position
RC getBlockPos(SM_FileHandle *fHandle)
{
	if (fileIsOpen == true)
	{
		RC block_pos = fHandle->curPagePos;
		return block_pos;
	}
}
// Reading first block from the file
extern RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{

	if (StorageManagerStatus != true)
	{
		return RC_STORAGE_MGR_NOT_INIT;
	}
	// Check if the file handle is initialized
	if (fHandle == NULL || fHandle->mgmtInfo == NULL)
	{
		return RC_FILE_HANDLE_NOT_INIT;
	}

	// Set the file position to the beginning (first page)
	FILE *fp = fHandle->mgmtInfo;
	RC position = 0;
	fseek(fp, position, SEEK_SET);

	// Read the first block (page) into memPage
	RC ReadErrorFlag = fread(memPage, sizeof(char), PAGE_SIZE, fp);

	if (ReadErrorFlag != PAGE_SIZE)
	{
		return RC_FILE_READ_ERROR;
	}

	// Update the current page position
	fHandle->curPagePos = 0;

	return RC_OK;
}

// Reading previous block in file
extern RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	if (StorageManagerStatus != true)
	{
		return RC_STORAGE_MGR_NOT_INIT;
	}
	// Check if the file handle is initialized

	FILE *fp = fHandle->mgmtInfo;
	// Calculate the position of the previous block
	RC position = fHandle->curPagePos - 1;

	// Ensure the previous block is within bounds
	if (position < 0)
	{
		return RC_READ_NON_EXISTING_PAGE;
	}

	// Set the file position to the beginning of the previous block
	fseek(fp, position * PAGE_SIZE, SEEK_SET);

	// Read the previous block (page) into memPage
	RC ReadErrorFlag = fread(memPage, sizeof(char), PAGE_SIZE, fp);

	if (ReadErrorFlag != PAGE_SIZE)
	{
		return RC_FILE_READ_ERROR;
	}

	// Update the current page position
	fHandle->curPagePos = position;

	return RC_OK;
}

// Reads the current block which the file pointer points to
RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{

	if (StorageManagerStatus != true)
	{
		return RC_STORAGE_MGR_NOT_INIT;
	}

	// Check if the file handle is initialized
	if (fHandle == NULL || fHandle->mgmtInfo == NULL)
	{
		return RC_FILE_HANDLE_NOT_INIT;
	}
	FILE *fp = fHandle->mgmtInfo;
	// Calculate the offset to the current block
	RC position = fHandle->curPagePos * PAGE_SIZE;
	fseek(fp, position, SEEK_SET);
	RC ReadErrorFlag = fread(memPage, sizeof(char), PAGE_SIZE, fp);

	if (ReadErrorFlag != PAGE_SIZE)
	{

		return RC_FILE_READ_ERROR;
	}

	// Update the current page position
	fHandle->curPagePos = position + 1;

	return RC_OK;
}

// Reading last block of the file
RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	printf("in readNextBlock ");
	if (StorageManagerStatus != true)
		return RC_STORAGE_MGR_NOT_INIT;

	if (fHandle == NULL || fHandle->fileName == NULL)
		return RC_FILE_NOT_FOUND;

	RC pageNum = fHandle->curPagePos + 1;

	if (pageNum < 0 || pageNum >= fHandle->totalNumPages)
		return RC_READ_NON_EXISTING_PAGE;

	FILE *fp = fHandle->mgmtInfo;
	fseek(fp, pageNum * PAGE_SIZE, SEEK_SET);
	RC read_size = fread(memPage, sizeof(char), PAGE_SIZE, fp);
	if (read_size < PAGE_SIZE || read_size > PAGE_SIZE)
	{

		return RC_READ_NON_EXISTING_PAGE;
	}
	fHandle->curPagePos = pageNum;
	return RC_OK;
}

// Reading the last block of the file
RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	printf("in readLastBlock ");
	if (StorageManagerStatus != true)
		return RC_STORAGE_MGR_NOT_INIT;

	if (fHandle == NULL || fHandle->fileName == NULL)
		return RC_FILE_NOT_FOUND;

	RC pageNum = fHandle->totalNumPages - 1;

	if (pageNum < 0 || pageNum >= fHandle->totalNumPages)
		return RC_READ_NON_EXISTING_PAGE;

	FILE *fp;
	fseek(fp, pageNum * PAGE_SIZE, SEEK_SET);
	RC read_size = fread(memPage, sizeof(char), PAGE_SIZE, fp);
	if (read_size < PAGE_SIZE || read_size > PAGE_SIZE)
	{

		return RC_READ_NON_EXISTING_PAGE;
	}
	fHandle->curPagePos = pageNum;
	return RC_OK;
}

// Writing block at page pageNum
RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	RC returncode;

	if (pageNum < 0 || pageNum > fHandle->totalNumPages) // If the pageNum is greater than totalNumber of pages, or less than zero, throw an error
		returncode = RC_WRITE_FAILED;
	if (file == NULL)
	{
		return RC_FILE_NOT_FOUND;
	}

	if (fseek(file, (PAGE_SIZE * pageNum), SEEK_SET) == 0)
	{
		fwrite(memPage, sizeof(char), PAGE_SIZE, file);

		//(*fHandle).curPagePos = pageNum; //Update current page position to the pageNum i.e. index of the block written
		fHandle->curPagePos = pageNum;
		fseek(file, 0, SEEK_END);
		(*fHandle).totalNumPages = ftell(file) / PAGE_SIZE; // update the value of totalNumPages which increases by 1

		return RC_OK;
	}
	else
	{
		return RC_WRITE_FAILED;
	}
}

// Writing current block in the file
RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	if (fHandle == NULL || fHandle->mgmtInfo == NULL)
		return RC_FILE_HANDLE_NOT_INIT;

	FILE *file = fopen(fHandle->fileName, "rb+");

	if (file == NULL)
		return RC_FILE_NOT_FOUND;

	if (fseek(file, ((long)fHandle->curPagePos * PAGE_SIZE), SEEK_SET) != 0)
	{
		fclose(file);
		return RC_FILE_SEEK_ERROR;
	}

	RC bytesWrittenCount = fwrite(memPage, sizeof(char), PAGE_SIZE, file);

	if (bytesWrittenCount != PAGE_SIZE)
	{
		fclose(file);
		return RC_WRITE_FAILED;
	}

	fclose(file);

	return RC_OK;
}

// Append empty block to the file
RC appendEmptyBlock(SM_FileHandle *fHandle)
{
	if (fHandle == NULL || fHandle->mgmtInfo == NULL)
		return RC_FILE_HANDLE_NOT_INIT;

	FILE *file = fopen(fHandle->fileName, "ab+");

	if (file == NULL)
		return RC_FILE_NOT_FOUND;

	if (fseek(file, 0, SEEK_END) != 0)
	{
		fclose(file);
		return RC_FILE_SEEK_ERROR;
	}

	char emptyBlock[PAGE_SIZE] = {0};

	RC bytesWrittenCount = fwrite(emptyBlock, sizeof(char), PAGE_SIZE, file);

	if (bytesWrittenCount != PAGE_SIZE)
	{
		fclose(file);
		return RC_WRITE_FAILED;
	}

	fHandle->totalNumPages++;
	fHandle->curPagePos = fHandle->totalNumPages - 1;

	fclose(file);
	return RC_OK;
}

// Ensure capacity of the file is numberOfPages
RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle)
{
	printf("ensureCapacity");
	if (fHandle->totalNumPages < numberOfPages)
	{

		for (int i = 0; i < numberOfPages - fHandle->totalNumPages; i++)
		{
			appendEmptyBlock(fHandle);
		}
		return RC_OK;
	}

	else
		return RC_WRITE_FAILED;
}