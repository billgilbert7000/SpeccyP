//#include "ff.h"//#include "VFS.h"

#include <fcntl.h>
#include "zx_emu/Z80.h"
#include "g_config.h"
#include "util_sd.h"
#include "config.h"  

 //FileSystem
 FATFS FileSystem;

int last_error=0;
//bool init_fs;


//работа с каталогами и файлами

int init_filesystem(void){
    DIR d;
    int fd =-1;

    fd = f_mount(&FileSystem,"0", 1);
    if (fd!=FR_OK) return fd; 

    files[0][LENF1]=1;
 // read_select_dir(1);
    strcpy(conf.activefilename,"");
  fd=f_opendir(&d,dirs[0]);
   // fd=f_opendir(&d,"");
    return fd;
    if (fd==FR_OK) return fd;   
  //  printf("Open SD-OK\n");
    return fd;
}

/* const char* get_file_extension(const char *filename){
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
} */

const char* get_file_extension(const char *filename){
   const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return (filename+(strlen (filename)-3));
 
  //  printf(dot);
  //  printf(filename);
  //  return dot ;
}


void sortfiles(char* nf_buf, int N_FILES){
    int inx=0;
    if (N_FILES==0) return;
    uint8_t tmp_buf[LENF];
    while(inx!=(N_FILES-1)){
        if (nf_buf[LENF*inx+LENF1]>nf_buf[LENF*(inx+1)+LENF1]){inx++; continue;}

        if ((nf_buf[LENF*inx+LENF1]<nf_buf[LENF*(inx+1)+LENF1])||(strcmp(nf_buf+LENF*inx,nf_buf+LENF*(inx+1))>0)){
            memcpy(tmp_buf,nf_buf+LENF*inx,LENF);
            memcpy(nf_buf+LENF*inx,nf_buf+LENF*(inx+1),LENF);
            memcpy(nf_buf+LENF*(inx+1),tmp_buf,LENF);
            if (inx) inx--;
            continue;
        }
        inx++;
    }
}

int get_files_from_dir(char *dir_name,char* nf_buf, int MAX_N_FILES){
    DIR d;
    int fd =-1;
    FILINFO dir_file_info;
    //"0:/z80"
    fd=f_opendir(&d,dir_name); 
    if (fd!=FR_OK){init_fs=fd;return 0;}// ошибка

    //printf("\nFile FD: %d\n", fd);
    int inx=0;
    while (1){   
        
        fd=f_readdir(&d,&dir_file_info);
        if (fd!=FR_OK){init_fs=fd;return 0;}// ошибка
        if (strlen(dir_file_info.fname)==0) break;
        if(strlen(dir_file_info.fname)>LENF1){
         strncpy(nf_buf+LENF*inx,dir_file_info.altname,LENF1) ;    
   //         strncpy(nf_buf+LENF*inx,dir_file_info.fname,LENF1) ;
        } else {
           strncpy(nf_buf+LENF*inx,dir_file_info.fname,LENF1) ;
        }


        
      //  if (dir_file_info.fattrib == AM_DIR) nf_buf[LENF*inx+LENF1]=1; else nf_buf[LENF*inx+LENF1]=0;
if ((dir_file_info.fattrib&AM_DIR) == AM_DIR) nf_buf[LENF*inx+LENF1]=1; else nf_buf[LENF*inx+LENF1]=0;

        inx++; //???????????????????
        if (inx>=MAX_N_FILES) break;
        //if (dir_file_info.fname) break;
        //file_list[i] = dir_file_info.fname;
        //printf("[%s] %d\n",dir_file_info.fname,strlen(dir_file_info.fname));
    }
    f_closedir(&d);
    sortfiles(nf_buf,inx);
    return inx;
}
// вроде как не нужно???
/* 
char* get_file_from_dir(char *dir_name,int inx_file)
{

    DIR d;
    int fd =-1;
    FILINFO dir_file_info;
    //static char filename[20];
    static char filename[256];
    //filename[0]=0;
    //filename[LENF]=0;    
    memset(filename, 0, 256);

    fd = f_opendir(&d,dir_name);// открытие директории
    if (fd!=FR_OK) return filename;// если ошибка
    //printf("\nFile FD: %d\n", fd);
    int inx=0;
    while (1){   
        fd = f_readdir(&d,&dir_file_info);
        if (fd!=FR_OK) return filename;
        if (strlen(dir_file_info.fname)==0) break;
        if (inx++==inx_file){
            strncpy(filename,dir_file_info.fname,LENF) ;
       //     strncpy(filename,dir_file_info.fname,sizeof(dir_file_info.fname)) ;

        //strncpy(long_name,dir_file_info.fname,sizeof(dir_file_info.fname)) ;

          //  if (dir_file_info.fattrib == AM_DIR) strcat(filename,"/");
       //    if ((dir_file_info.fattrib&AM_DIR) == AM_DIR) strcat(filename,"/");//..????
            break;
        }
        //if (dir_file_info.fname) break;
        //file_list[i] = dir_file_info.fname;
    }
    f_closedir(&d);
    //printf("[%s] %d\n",filename,strlen(filename));
    return filename;

} */
//=======================================================
char* get_current_altname(char *dir_name,char *part_name)
{

    DIR d;
    int fd =-1;
    FILINFO dir_file_info;
     static char filename[22];//error
   
    memset(filename, 0, 22);//20

    fd = f_opendir(&d,dir_name);
    if (fd!=FR_OK) return filename;
    //printf("\nFile FD: %d\n", fd);
    int inx=0;
    while (1){   
        fd = f_readdir(&d,&dir_file_info);
        if (fd!=FR_OK) return filename;
        if (strlen(dir_file_info.fname)==0) break;


 file_attr[0]= dir_file_info.fattrib; // Read Only


        if (strncmp(dir_file_info.altname,part_name,LENF1)==0)
        {
            strncpy(filename,dir_file_info.altname,LENF) ;//LENF+  /// вывод длинного имени
       //    if (dir_file_info.fattrib == AM_DIR) strcat(filename,"/..");
            break;
        }
        if (strncmp(dir_file_info.fname,part_name,LENF1)==0)
        {
            strncpy(filename,dir_file_info.altname,LENF) ;
          //  if (dir_file_info.fattrib == AM_DIR) strcat(filename,"/..");
          break;
        }

    }
    f_closedir(&d);
    //printf(">[%s] %d\n",filename,strlen(filename));
    return filename;

}





//=======================================================
// вывод длинного имени для информации
char* get_lfn_from_dir(char *dir_name,char *part_name)
{

    DIR d;
    int fd =-1;
    FILINFO dir_file_info;
    //static char filename[20];
    static char filename[256];
    //filename[0]=0;
    //filename[LENF]=0;    
    memset(filename, 0, 256);

    fd = f_opendir(&d,dir_name);
    if (fd!=FR_OK) return filename;
    //printf("\nFile FD: %d\n", fd);
    int inx=0;
    while (1){   
        fd = f_readdir(&d,&dir_file_info);
        if (fd!=FR_OK) return filename;
        if (strlen(dir_file_info.fname)==0) break;
        //printf("[%s] %d> %d\n",dir_file_info.fname,strlen(dir_file_info.fname),strncmp(dir_file_info.fname,part_name,LENF));
        //printf("[%s] %s\n",dir_file_info.fname,dir_file_info.altname);
/*         if (strncmp(dir_file_info.altname,part_name,LENF1)==0){
            strncpy(filename,dir_file_info.fname,LENF) ;
            if (dir_file_info.fattrib == AM_DIR) strcat(filename,"@@@@@@@@");
            break; 
        }*/
        if (strncmp(dir_file_info.altname,part_name,LENF1)==0){
            strncpy(filename,dir_file_info.fname,48) ;//LENF+  /// вывод длинного имени

         //  if (dir_file_info.fattrib == AM_DIR) strcat(filename,"/..");
         if ((dir_file_info.fattrib&AM_DIR) == AM_DIR) strcat(filename,"/....");//.. папка длинное имя

            break;
        }
        if (strncmp(dir_file_info.fname,part_name,LENF1)==0){
            strncpy(filename,dir_file_info.fname,48) ;
         //   if (dir_file_info.fattrib == AM_DIR) strcat(filename,"/..");
          if ((dir_file_info.fattrib&AM_DIR) == AM_DIR) strcat(filename,"/..");//.. папка короткое имя
          break;
        }

 


        //if (dir_file_info.fname) break;
        //file_list[i] = dir_file_info.fname;
        inx++;
    }
    f_closedir(&d);
    //printf(">[%s] %d\n",filename,strlen(filename));
    return filename;

}


int read_select_dir(int dir_index){
    dir_patch[0]=0;
    for(int i=0;i<=dir_index;i++){
      if (dir_index>=DIRS_DEPTH) break; //???????? ERROR DIRS_DEPTH
      
      strcat(dir_patch,dirs[i]);
      if (i!=dir_index) strcat(dir_patch,"/");
    }
    return get_files_from_dir(dir_patch,files[1],MAX_FILES);
}
//----------------------------------------------------------------------
/* int sd_mkdir(const TCHAR* path){

    return f_mkdir(path);
}
 */
/* int sd_open_file(FIL *file,char *file_name,BYTE mode){
    return f_open(file,file_name,mode);
    // Returns a negative error code on failure.
    //int lfs_file_open(lfs_t *lfs, lfs_file_t *file,const char *path, int flags);
   // lfs_file_open(lfs_t *lfs, lfs_file_t *file,const char *path, int flags);
} */
//
/* int sd_read_file(FIL *file,void* buffer, UINT bytestoread, UINT* bytesreaded){
    return f_read(file,buffer,bytestoread,bytesreaded);
} */
//
/* int sd_write_file(FIL *file,void* buffer, UINT bytestowrite, UINT* byteswrited){
    return f_write(file,buffer,bytestowrite,byteswrited);
} */
//
/* int sd_seek_file(FIL *file,FSIZE_t offset){
    return f_lseek(file,offset);
} */

/* int sd_close_file(FIL *file){
    return f_close(file);
}
 */
DWORD sd_file_pos(FIL *file){
    return file->fptr;
}

DWORD sd_file_size(FIL *file){
    return file->obj.objsize;
}

