#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "inc/types.h"
#include "inc/superblock.h"
#include "inc/blockgroup_descriptor.h"
#include "inc/inode.h"
#include "inc/directoryentry.h"

struct os_superblock_t *superblock;
struct os_blockgroup_descriptor_t *group_descr;
struct os_inode_t *inodes;

void read_superblock(int fd)
{
	superblock = malloc(sizeof(struct os_superblock_t));
	assert(superblock != NULL);
       
	assert(lseek(fd, (off_t)1024, SEEK_SET) == (off_t)1024);
	assert(read(fd, (void *)superblock, sizeof(struct os_superblock_t)) == sizeof(struct os_superblock_t));
}

// void read_blockgroup(int fd)
// {
// 	blockgroup = malloc(sizeof(struct os_blockgroup_descriptor_t));
// 	assert(blockgroup != NULL);
       
// 	assert(lseek(fd, (off_t)2048, SEEK_SET) == (off_t)2048);
// 	assert(read(fd, (void *)blockgroup, sizeof(struct os_blockgroup_descriptor_t)) == sizeof(struct os_blockgroup_descriptor_t));
// }


// void read_inodeTable(int fd)
// {

// 	// preparing to cache inode table in inodes
// 	inodes = (struct os_inode_t*)malloc(superblock->s_inodes_count*superblock->s_inode_size);
// 	assert(inodes != NULL);

// 	// seek to start of inode_table
// 	assert(lseek(fd, (off_t)(blockgroup->bg_inode_table*1024), SEEK_SET) == (off_t)(blockgroup->bg_inode_table*1024));

// 	assert(read(fd, (void *)inodes, 0x40000) == 0x40000);

// }

void printInodeType(int inode_type)
{
	switch(inode_type)
	{
	case 1:
		printf("-");
		break;
	case 2:
		printf("d");
		break;
	case 3:
		printf("c");
		break;
	case 4:
		printf("b");
		break;
	case 5:
		printf("B");
		break;
	case 6:
		printf("S");
		break;
	case 7:
		printf("l");
		break;
	default:
		printf("X");
		break;
	}
}

void printInodePerm(int fd, int inode_num)
{
	//int curr_pos = lseek(fd, 0, SEEK_CUR);
	short int mode = inodes[inode_num-1].i_mode;

	mode & EXT2_S_IRUSR ? printf("r") : printf("-");
	mode & EXT2_S_IWUSR ? printf("w") : printf("-");
	mode & EXT2_S_IXUSR ? printf("x") : printf("-");
	mode & EXT2_S_IRGRP ? printf("r") : printf("-");
	mode & EXT2_S_IWGRP ? printf("w") : printf("-");
	mode & EXT2_S_IXGRP ? printf("x") : printf("-");
	mode & EXT2_S_IROTH ? printf("r") : printf("-");
	mode & EXT2_S_IWOTH ? printf("w") : printf("-");
	mode & EXT2_S_IXOTH ? printf("x") : printf("-");

	printf("\t");

	//lseek(fd, curr_pos, SEEK_SET);
}

int findInodeByName(int fd, int base_inode_num, char* filename, int filetype)
{
	char* name;
	int curr_inode_num;
	int curr_inode_type;

	// debug("data block addr\t= 0x%x\n", inodes[base_inode_num-1].i_block[0]);

	struct os_direntry_t* dirEntry = malloc(sizeof(struct os_direntry_t));
	assert (dirEntry != NULL);
	assert(lseek(fd, (off_t)(inodes[base_inode_num-1].i_block[0]*1024), SEEK_SET) == (off_t)(inodes[base_inode_num-1].i_block[0]*1024));
	assert(read(fd, (void *)dirEntry, sizeof(struct os_direntry_t)) == sizeof(struct os_direntry_t));

	 while (dirEntry->inode) {

		name = (char*)malloc(dirEntry->name_len+1);
		memcpy(name, dirEntry->file_name, dirEntry->name_len);
		name[dirEntry->name_len+1] = '\0';

		curr_inode_num = dirEntry->inode;
		curr_inode_type = dirEntry->file_type;

		if (filetype == curr_inode_type) {
			if(!strcmp(name, filename)) {
				return(curr_inode_num);
			}
		}

		lseek(fd, (dirEntry->rec_len - sizeof(struct os_direntry_t)), SEEK_CUR);
		assert(read(fd, (void *)dirEntry, sizeof(struct os_direntry_t)) == sizeof(struct os_direntry_t));

	} 

	return(-1);	
}


void ls(int fd, int base_inode_num)
{
	char* name;
	int curr_inode_num;
	int curr_inode_type;

	// debug("data block addr\t= 0x%x\n", inodes[base_inode_num-1].i_block[0]);

	struct os_direntry_t* dirEntry = malloc(sizeof(struct os_direntry_t));
	assert (dirEntry != NULL);
	assert(lseek(fd, (off_t)(inodes[base_inode_num-1].i_block[0]*1024), SEEK_SET) == (off_t)(inodes[base_inode_num-1].i_block[0]*1024));
	assert(read(fd, (void *)dirEntry, sizeof(struct os_direntry_t)) == sizeof(struct os_direntry_t));

	 while (dirEntry->inode) {

		name = (char*)malloc(dirEntry->name_len+1);
		memcpy(name, dirEntry->file_name, dirEntry->name_len);
		name[dirEntry->name_len+1] = '\0';

		curr_inode_num = dirEntry->inode;
		curr_inode_type = dirEntry->file_type;

		lseek(fd, (dirEntry->rec_len - sizeof(struct os_direntry_t)), SEEK_CUR);
		assert(read(fd, (void *)dirEntry, sizeof(struct os_direntry_t)) == sizeof(struct os_direntry_t));

		if (name[0] == '.') {
			if ( name[1]=='.' || name[1]=='\0')
				continue;
		} else {
			// debug("rec_len\t\t= %d\n", dirEntry->rec_len);
			// debug("dirEntry->inode\t= %d\n",dirEntry->inode);
			printInodeType(curr_inode_type);
			printInodePerm(fd, curr_inode_num);
			printf("%d\t", curr_inode_num);
			printf("%s\t", name);
			printf("\n");
		}
	} 

	return;
}

int cd(int fd, int base_inode_num)
{
	char dirname[255];
	int ret;

	//printf("Enter directory name:");
	scanf("%s", dirname);

	ret = findInodeByName(fd, base_inode_num, dirname, EXT2_FT_DIR);
	// debug("findInodeByName=%d\n", ret);

	if(ret==-1) {
		printf("Directory %s does not exist\n", dirname);
		return(base_inode_num);
	} else {
		printf("Now in directory %s\n", dirname);
		return(ret);
	}

}

void my_stat(int fd, int base_inode_num)
{
	char dirname[255];
	char* name;
	int ret;
	struct os_inode_t inode;

	scanf("%s", dirname);
	// printf("li: %s\n",dirname);

	ret = findInodeByName(fd, base_inode_num, dirname, EXT2_FT_DIR);

	

	if(ret==-1) {
		printf("Arquivo %s não existe!\n", dirname);
		return;
	} else {
		inode = inodes[ret-1];
		printf("File: \"%s\"\n", dirname);
		printf("Size: %d\t Blocks: %d\t", inode.i_size, inode.i_blocks);
		printf("block size \t\t= %d bytes\n", 1<<(10 + superblock->s_log_block_size));
		printf("inode count \t\t= %d\n", superblock->s_inodes_count);
		printf("inode size \t\t= %d\n", superblock->s_inode_size);
		printf("inode table address \t= %d\n", group_descr->bg_inode_table);
		printf("inode table size \t= %dKB\n", (superblock->s_inodes_count*superblock->s_inode_size)>>10);
		return;
	}

}

int shellFs(int fd )
{
	char cmd[4];
	static int pwd_inode = 2;

	printf("dcc-shell-fs$ ");
	scanf("%s", cmd);

	// debug("cmd=%s\n", cmd);

	if(!strcmp(cmd, "q")) {
		return(-1);

	} else if(!strcmp(cmd, "ls")) {
		ls(fd, pwd_inode);

	} else if(!strcmp(cmd, "cd")) {
		pwd_inode = cd(fd, pwd_inode);

	} 
	else if(!strcmp(cmd, "stat")) {
		my_stat(fd, pwd_inode);
	} 
	else {
		printf("Unknown command: %s\n", cmd);
		return(-EINVAL);
	}

	return(0);
}

int main(int argc, char **argv)
{
	// open up the disk file
	if (argc !=2) {
	printf("usage:  ext-shell <file.img>\n");
		return -1; 
	}

	int fd = open(argv[1], O_RDONLY|O_SYNC);
	if (fd == -1) {
		printf("Could NOT open file \"%s\"\n", argv[1]);
		return -1; 
	}

	// reading superblock
	// read_superblock(fd);

	superblock = malloc(sizeof(struct os_superblock_t));
	assert(superblock != NULL);
       
	assert(lseek(fd, (off_t)1024, SEEK_SET) == (off_t)1024);
	assert(read(fd, (void *)superblock, sizeof(struct os_superblock_t)) == sizeof(struct os_superblock_t));

	/* calculate number of block groups on the disk */
	unsigned int group_count;
	group_count = 1 + (superblock->s_blocks_count - 1) / superblock->s_blocks_per_group;

	/* calculate size of the group descriptor list in bytes */
	unsigned int descr_list_size;
	descr_list_size = group_count * sizeof(struct os_blockgroup_descriptor_t);


	


	// struct os_blockgroup_descriptor_t group_descr;
	/* position head above the group descriptor block */
	/* sd --> storage device, no nosso caso um arquivo */
	group_descr = malloc(sizeof(struct os_blockgroup_descriptor_t));
	lseek(fd, (off_t)2048, SEEK_SET);
	read(fd, (void *)group_descr, sizeof(struct os_blockgroup_descriptor_t));


	// assert(blockgroup != NULL);
	
	// assert(lseek(fd, (off_t)2048, SEEK_SET) == (off_t)2048);
	// assert(read(fd, (void *)blockgroup, sizeof(struct os_blockgroup_descriptor_t)) == sizeof(struct os_blockgroup_descriptor_t));

	// // reading blockgroup
	// read_blockgroup(fd);
	

	// preparing to cache inode table in inodes
	inodes = (struct os_inode_t*)malloc(superblock->s_inodes_count*superblock->s_inode_size);
	assert(inodes != NULL);

	// seek to start of inode_table
	assert(lseek(fd, (off_t)(group_descr->bg_inode_table*1024), SEEK_SET) == (off_t)(group_descr->bg_inode_table*1024));

	assert(read(fd, (void *)inodes, 0x40000) == 0x40000);

	// reading inode table
	// read_inodeTable(fd);

	while(1) {
		// extShell waits for one cmd and executes it.
		// returns 0 on successfull execution of command,
		// returns -EINVAL on unknown command
		// returns -1 if cmd="q" i.e. quit.
		if ( shellFs(fd)==-1 )
			break;
	}

    // ls(fd, 2);

	printf("\n\nQuitting ext-shell.\n\n");
	return(0);
}
