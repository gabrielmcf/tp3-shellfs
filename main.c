#include <ext2fs/ext2fs.h>
#include <com_err.h>

int main(int argc, char *argv[]) 
{ 
    char *fsname = argv[1];
    errcode_t err;
    ext2_filsys fs;
    add_error_table(&et_ext2_error_table);
    
    err = ext2fs_open(fsname, 0, 0, 0, unix_io_manager, &fs);

    if (err) {
        com_err(argv[0], err, "ao abrir sistema de arquivos %s", fsname);
        exit(1);
    }



    ext2_ino_t dir;

    err = ext2fs_check_directory(fs, EXT2_ROOT_INO);

    if (err) {
        com_err(argv[0], err, "ao abrir sistema de arquivos %s", fsname);
        exit(1);
    }
}
