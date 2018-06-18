#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>

#include "inc/types.h"
#include "inc/superblock.h"
#include "inc/blockgroup_descriptor.h"
#include "inc/inode.h"
#include "inc/directoryentry.h"

#define BASE_OFFSET 1024  /* location of the super-block in the first group */

#define BLOCK_OFFSET(block) (BASE_OFFSET + (block-1)*block_size)

#define EXT2_FT_ALL 8

#define GROUP_INDEX(inode_num) (inode_num - 1)/superblock->s_inodes_per_group
#define INODE_INDEX(inode_num) (inode_num - 1)%superblock->s_inodes_per_group
#define GET_INODE(inode_num) inodes[GROUP_INDEX(inode_num)][INODE_INDEX(inode_num)]

u_int32_t block_size;
struct os_superblock_t *superblock;
struct os_blockgroup_descriptor_t **group_descr;
struct os_inode_t **inodes;

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

void printInodePerm(int inode_num)
{
	short int mode = GET_INODE(inode_num).i_mode;

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
}

int findInodeByName(int fd, int inode_num, char* filename, int filetype)
{
	char* name;
	int curr_inode_num;
	int curr_inode_type;

	struct os_direntry_t* dirEntry = malloc(sizeof(struct os_direntry_t));
	assert (dirEntry != NULL);
	lseek(fd, (off_t)(GET_INODE(inode_num).i_block[0]*1024), SEEK_SET);
	read(fd, (void *)dirEntry, sizeof(struct os_direntry_t));

	 while (dirEntry->inode) {

		name = (char*)malloc(dirEntry->name_len+1);
		memcpy(name, dirEntry->file_name, dirEntry->name_len);
		name[dirEntry->name_len+1] = '\0';

		curr_inode_num = dirEntry->inode;
		curr_inode_type = dirEntry->file_type;

		if (filetype == curr_inode_type || filetype == EXT2_FT_ALL) {
			if(!strcmp(name, filename)) {
				return(curr_inode_num);
			}
		}

		lseek(fd, (dirEntry->rec_len - sizeof(struct os_direntry_t)), SEEK_CUR);
		assert(read(fd, (void *)dirEntry, sizeof(struct os_direntry_t)) == sizeof(struct os_direntry_t));

	} 

	return(-1);	
}


void ls(int fd, int inode_num)
{
	char* name;
	int curr_inode_num;
	int curr_inode_type;
	struct os_inode_t inode = GET_INODE(inode_num);
	int read_size = 0;

	struct os_direntry_t* dirEntry = malloc(sizeof(struct os_direntry_t));
	assert (dirEntry != NULL);
	lseek(fd, BLOCK_OFFSET(inode.i_block[0]), SEEK_SET);
	read(fd, (void *)dirEntry, sizeof(struct os_direntry_t));

	read_size = dirEntry->rec_len;
	while (dirEntry->inode && read_size <= inode.i_size) {

		name = (char*)malloc(dirEntry->name_len+1);
		memcpy(name, dirEntry->file_name, dirEntry->name_len);
		name[dirEntry->name_len+1] = '\0';

		curr_inode_num = dirEntry->inode;
		curr_inode_type = dirEntry->file_type;

		lseek(fd, (dirEntry->rec_len - sizeof(struct os_direntry_t)), SEEK_CUR);
		assert(read(fd, (void *)dirEntry, sizeof(struct os_direntry_t)) == sizeof(struct os_direntry_t));
		read_size += dirEntry->rec_len;

		if (name[0] == '.') {
			if ( name[1]=='.' || name[1]=='\0')
				continue;
		} else {
			printInodeType(curr_inode_type);
			printInodePerm(curr_inode_num);
			printf("%d\t", curr_inode_num);
			printf("%s\t", name);
			printf("\n");
		}
	} 

	return;
}

int cd(int fd, int inode_num)
{
	char dirname[255];
	int ret;

	scanf("%s", dirname);

	ret = findInodeByName(fd, inode_num, dirname, EXT2_FT_DIR);

	if(ret==-1) {
		printf("O diretório %s não existe\n", dirname);
		return(inode_num);
	} else {
		printf("No diretório %s\n", dirname);
		return(ret);
	}

}

void my_stat(int fd, int inode_num)
{
	char dirname[255];
	char file_type_name[30];
	char* name;
	int ret;
	struct stat true_stat;
	struct os_inode_t *inode = malloc(sizeof(struct os_inode_t));
	struct os_direntry_t* dirEntry = malloc(sizeof(struct os_direntry_t));
	u_int16_t i_mode;
	int filetype;
	time_t a_timestamp;
	time_t m_timestamp;
	time_t c_timestamp;

	scanf("%s", dirname);

	ret = findInodeByName(fd, inode_num, dirname, EXT2_FT_ALL);

	inode = &GET_INODE(ret);
	i_mode = inode->i_mode;
	a_timestamp = (time_t) inode->i_atime;
	m_timestamp = (time_t) inode->i_mtime;
	c_timestamp = (time_t) inode->i_ctime;

	if(S_ISREG(i_mode)){ //Regular file
		strcpy(file_type_name,"arquivo comum");
		filetype = EXT2_FT_REG_FILE;
	} else if(S_ISDIR(i_mode)){ //Directory 	
		strcpy(file_type_name,"diretório");
		filetype = EXT2_FT_DIR;
	} else if(S_ISCHR(i_mode)){ //Character Device  
		strcpy(file_type_name,"dispositivo de caracteres");
		filetype = EXT2_FT_CHRDEV;
	} else if(S_ISBLK(i_mode)){ //Block Device	
		strcpy(file_type_name,"dispositivo de bloco");
		filetype = EXT2_FT_BLKDEV;
	} else if(S_ISFIFO(i_mode)){ //Fifo	
		strcpy(file_type_name,"arquivo de buffer");
		filetype = EXT2_FT_FIFO;
	} else if(S_ISSOCK(i_mode)){ //Socket
		strcpy(file_type_name,"soquete");
		filetype = EXT2_FT_SOCK;
	} else if(S_ISLNK(i_mode)){ //Symbolic Link	
		strcpy(file_type_name,"link simbólico");
		filetype = EXT2_FT_SYMLINK;
	} else {
		strcpy(file_type_name,"desconhecido");
		filetype = EXT2_FT_UNKNOWN;
	}

	if(ret==-1) {
		printf("Arquivo %s não existe!\n", dirname);
		return;
	} else {
		printf("File: \"%s\"\n", dirname);
		printf("Size: %u\tBlocks: %u\tIO Blocks %d\t%s\n", inode->i_size, inode->i_blocks, 1024 << superblock->s_log_block_size, file_type_name);
		printf("Device: -\tInode: %d\tLinks: %hu\n", ret, inode->i_links_count);
		printf("Access: ");
		printInodeType(filetype);
		printInodePerm(inode_num);
		printf("\tUid: %u\t Gid: %u\n", inode->i_uid, inode->i_gid);
		printf("Access: %s", asctime(gmtime(&a_timestamp)));
		printf("Modify: %s", asctime(gmtime(&m_timestamp)));
		printf("Change: %s", asctime(gmtime(&c_timestamp)));
		printf("Birth: -\n");
		return;
	}

}

void find(int fd,int inode_num){
	struct os_inode_t inode = GET_INODE(inode_num);
	int read_size = 0;

	struct os_direntry_t* dirEntry = malloc(sizeof(struct os_direntry_t));
	lseek(fd, BLOCK_OFFSET(inode.i_block[0]), SEEK_SET);
	read(fd, (void *)dirEntry, sizeof(struct os_direntry_t));
	unsigned int pular = BLOCK_OFFSET(inode.i_block[0]) + sizeof(struct os_direntry_t);
	
	read_size = dirEntry->rec_len;
	while (dirEntry->inode && read_size <= inode.i_size) {
		//Se não for diretório "." nem diretório ".." , imprima o nome
		if(strcmp(dirEntry->file_name,".") && strcmp(dirEntry->file_name,"..")){
			printf("./%s\n",dirEntry->file_name);

			//Se for um diretório, chame recursivamente para imprimir o nome dos filhos dele.
			if(dirEntry->file_type == EXT2_FT_ALL){
				find(fd,dirEntry->inode);
			}
		}
		//Passa para o próximo diretório.
		pular += (dirEntry->rec_len - sizeof(struct os_direntry_t));
		lseek(fd, pular, SEEK_SET);
		assert(read(fd, (void *)dirEntry, sizeof(struct os_direntry_t)) == sizeof(struct os_direntry_t));
		read_size += dirEntry->rec_len;
		
	}
	return;
}

void sb(int fd){
	int i;

	//Lê o superbloco
	struct os_superblock_t *sb = malloc(sizeof(struct os_superblock_t));
	assert(superblock != NULL);
	assert(lseek(fd, (off_t)1024, SEEK_SET) == (off_t)1024);
	assert(read(fd, (void *)sb, sizeof(struct os_superblock_t)) == sizeof(struct os_superblock_t));

	printf("\nSuperblock informations\n\n");
	printf("Inodes count: %u\nBlocks count: %u\nSuperuser blocks: %u\n",sb->s_inodes_count, sb->s_blocks_count, sb->s_r_blocks_count);
	printf("Free blocks: %u\nFree inodes: %u\nFirst data block: %u\n",sb->s_free_blocks_count, sb->s_free_inodes_count,sb->s_first_data_block);
	printf("Log block size: %u\nShift log frag size: %u\nBlocks per group: %u\n",sb->s_log_block_size,sb->s_log_frag_size,sb->s_blocks_per_group);
	printf("Frags per group: %u\nInodes per group: %u\nLast mounted time: %u\n",sb->s_frags_per_group,sb->s_inodes_per_group,sb->s_mtime);
	printf("Last written time: %u\nMounted count since last check: %hu\n",sb->s_wtime,sb->s_mnt_count);
	printf("Max mount before check: %hu\nMagic number: %x\n",sb->s_max_mnt_count,sb->s_magic);
	printf("System state: %hu\nError politic: %hu\nVersion info: %u\n",sb->s_state,sb->s_errors,sb->s_minor_rev_level);
	printf("Last check: %u\nCheck interval: %u\nCreator id: %u\n",sb->s_lastcheck,sb->s_checkinterval,sb->s_creator_os);
	printf("Revision: %u\nDefaut uId: %u\nDefault gId: %u\n",sb->s_rev_level,sb->s_def_resuid,sb->s_def_resgid);
	printf("First inode for files: %u\nInode size: %u\nThis block group: %hu\n",sb->s_first_ino,sb->s_inode_size,sb->s_block_group_nr);
	printf("Compat. features: %u\nIncompat. features: %u\n",sb->s_feature_compat,sb->s_feature_incompat);
	printf("Read only features: %u\n",sb->s_feature_ro_compat);
	printf("UUID: ");
	for(i = 0; i < 16; i++){
		printf("%hhu",sb->s_uuid[i]);
	}
	printf("\n");
	printf("Volume name: ");
	for(i = 0; i < 16; i++){
		printf("%hhu",sb->s_volume_name[i]);
	}
	printf("\n");
	printf("Last mount point: %s\n", sb->s_last_mounted);
	printf("Compressions: %u\n", sb->s_algo_bitmap);
	printf("Pre-allocated blocks for file: %hhu\n",sb->s_prealloc_blocks);
	printf("Pre-allocated blocks for dir: %hhu\n",sb->s_prealloc_dir_blocks);
	printf("Journal UUID (if has EXT3 journal): %hhu\n",sb->s_journal_uuid);
	printf("Inode number of journal file: %u\n",sb->s_journal_inum);
	printf("Device number of journal file: %u\n",sb->s_journal_dev);
	printf("First inode on list to delete: %u\n",sb->s_last_orphan);
	printf("Hash seeds: %u %u %u %u\n", sb->s_hash_seed[0], sb->s_hash_seed[1], sb->s_hash_seed[2], sb->s_hash_seed[3] );
	printf("Hash version: %hhu\n", sb->s_def_hash_version);
	printf("Default mount options: %u\n",sb->s_default_mount_options);
	printf("Block ID of first meta-block group: %u\n\n",sb->s_first_meta_bg);
}

int shellFs(int fd )
{
	char cmd[4];

	static int pwd_inode = 2;

	printf("dcc-shell-fs$ ");
	scanf("%s", cmd);

	if(!strcmp(cmd, "q") || !strcmp(cmd, "exit")) {
		return(-1);

	} else if(!strcmp(cmd, "ls")) {
		ls(fd, pwd_inode);

	} 
	else if(!strcmp(cmd, "cd")) {
		pwd_inode = cd(fd, pwd_inode);

	} 
	else if(!strcmp(cmd, "stat")) {
		my_stat(fd, pwd_inode);
	}
	else if(!strcmp(cmd, "find")) {
		find(fd, pwd_inode);
	} 
	else if(!strcmp(cmd,"sb")){
		sb(fd);
	} else {
		printf("Comando desconhecido: %s\n", cmd);
		return(-EINVAL);
	}

	return(0);
}

int main(int argc, char **argv)
{
	int i;
	unsigned int inodes_per_block;
	unsigned int itable_blocks;
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

	//Lê o superbloco
	superblock = malloc(sizeof(struct os_superblock_t));
	assert(superblock != NULL);
	assert(lseek(fd, (off_t)1024, SEEK_SET) == (off_t)1024);
	assert(read(fd, (void *)superblock, sizeof(struct os_superblock_t)) == sizeof(struct os_superblock_t));

	//Calcula o número de grupos no sistema de arquivos
	unsigned int group_count;
	group_count = 1 + (superblock->s_blocks_count - 1) / superblock->s_blocks_per_group;

	//Calcula o tamanho da lista de descritores de arquivo
	unsigned int descr_list_size;
	descr_list_size = group_count * sizeof(struct os_blockgroup_descriptor_t);

	//Calcula o tamanho do bloco
	block_size = 1024 << superblock->s_log_block_size;

	//Aloca e o espaço para os descris lê
	group_descr = malloc(group_count*sizeof(struct  os_blockgroup_descriptor_t*));
	for(i = 0; i < group_count; i++){
		group_descr[i] = malloc(sizeof(struct os_blockgroup_descriptor_t));
		lseek(fd, (off_t)(BLOCK_OFFSET(2)+i*sizeof(struct os_blockgroup_descriptor_t)), SEEK_SET);
		read(fd, (void *)group_descr[i], sizeof(struct os_blockgroup_descriptor_t));
	}

	//Calcula a quantidade de inodes por bloco
	inodes_per_block = block_size / sizeof(struct os_inode_t);

	/* size in blocks of the inode table */
	itable_blocks = superblock->s_inodes_per_group / inodes_per_block;

	// inodes = (struct os_inode_t*)malloc(superblock->s_inodes_count*superblock->s_inode_size);
	// preparing to cache inode table in inodes
	inodes = malloc(group_count*sizeof(struct os_inode_t*));
	assert(inodes != NULL);

	for(i = 0; i < group_count; i++){
		inodes[i] = malloc(superblock->s_inodes_per_group*superblock->s_inode_size);
		lseek(fd, (off_t)BLOCK_OFFSET(group_descr[i]->bg_inode_table), SEEK_SET);
		read(fd, (void *)inodes[i], itable_blocks*(unsigned int)block_size);
	}

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
