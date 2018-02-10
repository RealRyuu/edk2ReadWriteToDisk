#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/ShellCEntryLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Protocol/BlockIo.h>
#include  <IndustryStandard/Mbr.h>
#include  <stdio.h>
#include  <Library/MemoryAllocationLib.h>
#include  <Library/BaseLib.h>
#include  <stdbool.h>
#include  <string.h>
#include  <stdlib.h>

void printChar(IN char c)
{
  putchar(c);
  fflush(stdout);
}

void printByteHex(IN unsigned char byte)
{
  int tmp = byte;
  if(tmp < 16)
    printChar('0');
  Print(L"%x", tmp);
}

void printBuffer(IN unsigned char* buffer, IN int size)
{
  UINTN EventIndex;
  int k = 0;
  for(int i = 0; i < size; i += 16, k++)
  {
    for(int j = 0; j < 16; j++)
    {
      printByteHex(buffer[i+j]);
      printChar(' ');
    }
    printChar('\n');
    if(k % 4 == 0 && k != 0)
    {
      gST->ConIn->Reset(gST->ConIn, FALSE);
      gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);
    }
  }
}

void printGUID(IN unsigned char* guidPtr)
{
  for(int i = 0; i < 4; i++)
    printByteHex(guidPtr[3 - i]);
  printChar('-');
  for(int i = 0; i < 2; i++)
    printByteHex(guidPtr[5 - i]);
  printChar('-');
  for(int i = 0; i < 2; i++)
    printByteHex(guidPtr[7 - i]);
  printChar('-');
  for(int i = 0; i < 2; i++)
    printByteHex(guidPtr[8 + i]);
  printChar('-');
  for(int i = 0; i < 6; i++)
    printByteHex(guidPtr[10 + i]);
  printChar('\n');
}

char* getGUID(IN unsigned char* guidMixed, OUT unsigned char* guidString)
{
  for(int i = 0; i < 4; i++)
    guidString[i] = guidMixed[3 - i];
  guidString[4] = '-';
  for(int i = 0; i < 2; i++)
    guidString[5 + i] = guidMixed[5 - i];
  guidString[7] = '-';
  for(int i = 0; i < 2; i++)
    guidString[8 + i] = guidMixed[7 - i];
  guidString[10] = '-';
  for(int i = 0; i < 2; i++)
    guidString[11 + i] = guidMixed[8 + i];
  guidString[13] = '-';
  for(int i = 0; i < 6; i++)
    guidString[14 + i] = guidMixed[10 + i];
  return guidString;
}

void printMBRsig(IN unsigned char* sigPtr)
{
  for(int i = 0; i < 4; i++)
  {
    printByteHex(sigPtr[3 - i]);
    if(i < 3)
      printChar('-');
  }
  printChar('\n');
}

int readNumber()
{
  int retVal;

  gST->ConIn->Reset(gST->ConIn, FALSE);
  //fflush(stdin);

  char buff2[256];

  int indx = 0;
  int c;

  //while((c = getchar()) != '\n' && c != EOF );
  c = getchar();
  while(c != '\n' && c != EOF)
  {
    if(c == 'q' || c == 'Q')
      exit(0);
    buff2[indx] = (char)c;
    indx++;
    c = getchar();
  }

  retVal = atoi(buff2);
  if(retVal < 0)
  {
    Print(L"Wrong input, exiting...\n");
    exit(1);
  }

  return atoi(buff2);
}

unsigned long long int getFirstLBA(IN unsigned char* GPTEntry)
{
  unsigned long long int retVal = 0;
  for(int i = 0; i < 8; i++)
  {
    retVal += GPTEntry[32 + i] << i*8;
  }
  return retVal;
}

void modify(EFI_BLOCK_IO_PROTOCOL* BlkIo, unsigned int lba, int Size, char* blockData, VOID* Buffer)
{
  unsigned char newVal;
  unsigned int offset;

  do
    {
      gST->ConOut->ClearScreen(gST->ConOut);
      Print(L"LBA nr %d\n", lba);
      printBuffer(blockData, Size);

      Print(L"Enter offset of value to modify:\n");
      offset = readNumber();
      Print(L"%d\n", offset);
      Print(L"Enter new value:\n");
      newVal = (unsigned char)readNumber();
      Print(L"%d\n", newVal);
      blockData[offset] = newVal;
      Print(L"Changed buffer:\n");
      gST->ConOut->ClearScreen(gST->ConOut);
      //printBuffer(blockData, Size);
      //printChar('\n');
      BlkIo->WriteBlocks(BlkIo, BlkIo->Media->MediaId, lba, Size, Buffer);
    }while(true);
    FreePool(Buffer);
}

unsigned int getLEInt32(IN unsigned char* dataPtr, IN int offset)
{
  unsigned int retVal = 0;
  for(int i = 0; i < 4; i++)
  {
    unsigned int tmp = dataPtr[offset + i];
    retVal += tmp << 8*i;
  }
  return retVal;
}

unsigned int getLEInt16(IN unsigned char* dataPtr, IN int offset)
{
  unsigned int retVal = 0;
  for(int i = 0; i < 2; i++)
  {
    unsigned int tmp = dataPtr[offset + i];
    retVal += tmp << 8*i;
  }
  return retVal;
}

void printSuperBlock(unsigned char* blockData){
  unsigned int s_inodes_count = getLEInt32(blockData, 0x0);
  Print(L"Total inode count: %d\n", s_inodes_count);

  unsigned int s_free_inodes_count = getLEInt32(blockData, 0x10);
  Print(L"Free inode count: %d\n", s_free_inodes_count);

  unsigned int s_first_data_block = getLEInt32(blockData, 0x14);
  Print(L"First data block: %d\n", s_first_data_block);

  unsigned int s_log_block_size = getLEInt32(blockData, 0x18);
  Print(L"Block size is: 2 ^ (10 + %d)\n", s_log_block_size);

  unsigned int s_log_clusetr_size = getLEInt32(blockData, 0x1C);
  Print(L"Cluster size is: 2 ^ (10 + %d)\n", s_log_clusetr_size);

  unsigned int s_blocks_per_group = getLEInt32(blockData, 0x20);
  Print(L"Blocks per group: %d\n", s_blocks_per_group);

  unsigned int s_clusters_per_group = getLEInt32(blockData, 0x24);
  Print(L"Clusters per group: %d\n", s_clusters_per_group);

  unsigned int s_inodes_per_group = getLEInt32(blockData, 0x28);
  Print(L"Inodes per group: %d\n", s_inodes_per_group);

  unsigned int s_mtime = getLEInt32(blockData, 0x2C);
  Print(L"Mount time, in seconds since the epoch: %d\n", s_mtime);

  unsigned int s_wtime = getLEInt32(blockData, 0x30);
  Print(L"Write time, in seconds since the epoch: %d\n", s_wtime);

  unsigned int s_mnt_count = getLEInt16(blockData, 0x34);
  Print(L"Number of mounts since the last fsck: %d\n", s_mnt_count);

  unsigned int s_max_mnt_count = getLEInt16(blockData, 0x36);
  Print(L"Number of mounts beyond which a fsck is needed: %d\n", s_max_mnt_count);

  unsigned int s_magic = getLEInt16(blockData, 0x38);
  Print(L"Magic signature, 0xEF53: %d - 0x%xh\n", s_magic, s_magic);
}

int main(IN int Argc, IN char **Argv){
  UINTN                       EventIndex;
  EFI_STATUS                  Status;
  EFI_HANDLE                  *BlkIoHandle;
  UINTN                       NoBlkIoHandles;
  UINTN                       i;
  EFI_BLOCK_IO_PROTOCOL       *BlkIo;
  EFI_BLOCK_IO_PROTOCOL*      Disks[10];
  bool                        isDiskGPT[10];
  VOID*                       Buffer;
  CHAR8*                       GPTTableBuffer;
  int                         Size;
  UINTN                       Offset = 1;
  bool                        lba_mode = false;
  unsigned int                nDisk = 0;
  //VOID*                       GPT;

  if(Argc > 0)
    if(AsciiStrCmp(Argv[1], "lba") == 0)
    {
      Print(L"Lba mode on!\n");
      lba_mode = true;
    }

  Print(L"\n Press any Key to continue : \n");
  gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);

  Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &NoBlkIoHandles, &BlkIoHandle);

  unsigned char* blockData;
  unsigned char* blockData2;

  char gpt_guid[9];
  gpt_guid[8] = '\0';
  char mbr_signature[5];
  mbr_signature[4] = '\0';

  char* EFI_PARTITION_GUID = "EFI PART";

  for(i = 0; i < NoBlkIoHandles; i++)
  {
    Status = gBS->HandleProtocol(BlkIoHandle[i], &gEfiBlockIoProtocolGuid, (VOID**)&BlkIo);

    //if partition skip
    if(BlkIo->Media->LogicalPartition)
      continue;
    else
    {
      Disks[nDisk] = BlkIo;
      nDisk++;
    }
  }

  Print(L"Found %d disks!\n\n", nDisk);

  for(unsigned int i = 0; i < nDisk; i++)
  {
    Size = Disks[i]->Media->BlockSize;

    Buffer = AllocateZeroPool(Size);

    // read gpt
    Disks[i]->ReadBlocks(Disks[i], Disks[i]->Media->MediaId, Offset, Size, Buffer);

    blockData = Buffer;

    for(int j = 0; j < 8; j++)
      gpt_guid[j] = blockData[j];

    Print(L"Disk nr %d:\n", i);

    if(AsciiStrCmp(gpt_guid, EFI_PARTITION_GUID) == 0)
    {
      Print(L"this is a GPT DISK!\n");
      Print(L"Disk GUID: ");
      printGUID(blockData + 56);
      isDiskGPT[i] = true;
    }
    else
    {
      // try to read mbr
      Disks[i]->ReadBlocks(Disks[i], Disks[i]->Media->MediaId, 0, Size, Buffer);

      if(blockData[510] == 0x55 && blockData[511] == 0xAA)
        Print(L"this is a MBR DISK!\n");
      else
        Print(L"something is not right!\nboot signature not found!\n");

      Print(L"Disk signature: ");
      printMBRsig(blockData + 440);
      isDiskGPT[i] = false;
    }

    printChar('\n');

    FreePool(Buffer);
  }

  Print(L"Chose disk by entering it's id:\n");
  int val = readNumber();

  if(val > (int)nDisk)
  {
    Print(L"Wrong input, exiting...\n");
    exit(1);
  }

  const int choosenDisk = val;
  Print(L"Disk nr %d chosen\n", val);
  BlkIo = Disks[val];

  unsigned int lba;

  // if raw lba read/write mode
  if(lba_mode)
  {
    Print(L"Enter LBA to work on:\n");
    lba = readNumber();
    Size = BlkIo->Media->BlockSize;
    Buffer = AllocateZeroPool(Size);
    BlkIo->ReadBlocks(BlkIo, BlkIo->Media->MediaId, lba, Size, Buffer);
    blockData = Buffer;
    Print(L"Will print LBA %d, press any Key to continue : \n", lba);

    gST->ConIn->Reset(gST->ConIn, FALSE);
    gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);
    
    modify(BlkIo, lba, Size, blockData, Buffer);
    exit(0);
  }
  // if not we seek exts
  else
  {
    gST->ConOut->ClearScreen(gST->ConOut);

    // if we have gpt
    if(isDiskGPT[choosenDisk])
    {
      Print(L"This disk is GPT!\n");

      Size = BlkIo->Media->BlockSize;

      Buffer = AllocateZeroPool(Size);

      BlkIo->ReadBlocks(BlkIo, BlkIo->Media->MediaId, 1, Size, Buffer);

      blockData = Buffer;

      int nLBAEntries = blockData[80] + (blockData[81] << 8) + (blockData[82] << 16) + (blockData[83] << 24);

      int nLBAforEntries = (nLBAEntries*128)/Size;

      GPTTableBuffer = AllocateZeroPool(nLBAEntries*128);

      unsigned int nEntry = 0;

      // zeros for empty entry comparsion
      char emptyEntry[128] = {0};
      bool endOfTable = false;
      
      // for each LBA of GPT
      for(int i = 2 ; i < nLBAforEntries + 2; i++)
      {
        BlkIo->ReadBlocks(BlkIo, BlkIo->Media->MediaId, i, Size, (GPTTableBuffer + Size*(i - 2)));

        blockData = GPTTableBuffer + Size*(i - 2);

        // for each entry in current LBA
        for(int j = 0; j < Size/128; j++)
        {
          // if next entry is empty
          if(memcmp(emptyEntry, blockData + (j*128), sizeof(emptyEntry)) == 0)
          {
            //Print(L"empty entry!\n");
            endOfTable = true;
            break;
          }
          else
          {
            nEntry++;
          }
        }
        if(endOfTable)
          break;
      }

      long long int* partitions;
      partitions = malloc(nEntry*sizeof(long long int));

      for(unsigned int i = 0; i < nEntry; i++)
      {
        char linuxGUID1[] = {0xEB,0xD0,0xA0,0xA2,'-',0xB9,0xE5,'-',0x44,0x33,'-',0x87,0xC0,'-',0x68,0xB6,0xB7,0x26,0x99,0xC7};
        char linuxGUID2[] = {0x0F,0xC6,0x3D,0xAF,'-',0x84,0x83,'-',0x47,0x72,'-',0x8E,0x79,'-',0x3D,0x69,0xD8,0x47,0x7D,0xE4};
        unsigned char* GPTEntryPtr = GPTTableBuffer + i*128;
        char tempoGUID[32] = {0};
        getGUID(GPTEntryPtr, tempoGUID);

        if(memcmp(linuxGUID1, tempoGUID, sizeof(linuxGUID1)) == 0 || memcmp(linuxGUID2, tempoGUID, sizeof(linuxGUID2)) == 0)
        {
          //Print(L"Possible linux partition detected!\n");
          unsigned long long int firstLba = getFirstLBA(GPTEntryPtr);

          Size = BlkIo->Media->BlockSize;

          Buffer = malloc(Size);

          BlkIo->ReadBlocks(BlkIo, BlkIo->Media->MediaId, firstLba + 2, Size, Buffer);

          blockData = Buffer;

          //printBuffer(blockData, Size);

          if(blockData[0x39] == 0xEF && blockData[0x38] == 0x53)
          {
            unsigned long long int firstLba = getFirstLBA(GPTEntryPtr);
            partitions[i] = firstLba;
            Print(L"Partition nr %d: ", i);
            for(int i = 0 ; i < 20; i++)
              if(tempoGUID[i] == '-')
                printChar('-');
              else
                printByteHex(tempoGUID[i]);
            Print(L"  First LBA: %d", firstLba);
            printChar('\n');
          }
          else
          {
            //Print(L"we're not interested in filthy microsoft\n");
          }
        }

        // unsigned long long int firstLba = getFirstLBA(GPTEntryPtr);
        // partitions[i] = firstLba;
        // Print(L"Partition nr %d: ", i);
        // for(int i = 0 ; i < 20; i++)
        //   if(tempoGUID[i] == '-')
        //     printChar('-');
        //   else
        //     printByteHex(tempoGUID[i]);
        // Print(L"  First LBA: %d", firstLba);
        // printChar('\n');
      }

      Print(L"Enter partition nr to work on:\n");
      val = readNumber();
      gST->ConOut->ClearScreen(gST->ConOut);
      if(val > (int)nEntry)
        return(1);

      if(partitions[val] == 0)
        return(1);

      Size = BlkIo->Media->BlockSize;

      Buffer = malloc(Size*2);

      BlkIo->ReadBlocks(BlkIo, BlkIo->Media->MediaId, partitions[val] + 2, Size, Buffer);

      blockData = Buffer;

      printBuffer(blockData, Size);

      BlkIo->ReadBlocks(BlkIo, BlkIo->Media->MediaId, partitions[val] + 3, Size, blockData + Size);

      blockData2 = blockData + Size;

      printBuffer(blockData2, Size);

      Print(L"\nBasic ext partition info:\n");

      Print(L"SuperBlock LBA %d\n", partitions[val] + 2);

      printSuperBlock(blockData);

      free(partitions);

      free(GPTTableBuffer);

      free(Buffer);
    }
    // if we have mbr disk
    else if(!(isDiskGPT[choosenDisk]))
    {
      Print(L"This disk is MBR!\n");
      Print(L"MBR partition search not implented yet!\n");
    }
  }

  FreePool(Buffer);

  return(0);
}
