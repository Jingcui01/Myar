#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ar.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <utime.h>
#include <fcntl.h>/*open creat*/
#include <dirent.h>
#include <errno.h>
#include <time.h>




char* itoa(int num, int base)
{
	static char buf[32];
	char sel[]="0123456789abcdef";
	int i=30;
	if(num==0)
		buf[i--]='0';
	for(;num && i; --i, num/=base)
		buf[i]=sel[num%base];
	return &buf[i+1];
}

void suffix(char*path, int size)
{
	int i = strlen(path);
	for(;i<size;i++)
		path[i]=' ';
}

void append(char* file, int fd_ar)//append one file to archive
{
	int fd_file=open(file,O_RDWR);//open file
	if (fd_file<0) 
	{
		printf("%s cannot append!\n",file);
		return;
	}
	struct stat buff;
	struct ar_hdr f_head;
	fstat(fd_file, &buff);//get info
	strcpy(f_head.ar_name,file);
	suffix(f_head.ar_name,16);
	strcpy(f_head.ar_date,itoa(buff.st_mtime,10));
	suffix(f_head.ar_date,12);
	strcpy(f_head.ar_uid,itoa(buff.st_uid,10));
	suffix(f_head.ar_uid,6);
	strcpy(f_head.ar_gid,itoa(buff.st_gid,10));
	suffix(f_head.ar_gid,6);
	strcpy(f_head.ar_mode,itoa(buff.st_mode,8));
	suffix(f_head.ar_mode,8);
	strcpy(f_head.ar_size,itoa(buff.st_size,10));
	suffix(f_head.ar_size,10);
	strcpy(f_head.ar_fmag,ARFMAG);
 
	write(fd_ar,f_head.ar_name,16);//write header
	write(fd_ar,f_head.ar_date,12);
	write(fd_ar,f_head.ar_uid,6);
	write(fd_ar,f_head.ar_gid,6);
	write(fd_ar,f_head.ar_mode,8);
	write(fd_ar,f_head.ar_size,10);
	write(fd_ar,f_head.ar_fmag,2);
	
	
	//write data
	//off_t P_O=lseek(fd_ar,0,SEEK_CUR);//record current offset for later usage
	
	char buffer[1024];//write file data
	
	int m;
	while((m=read(fd_file, buffer, 128))>0)//?????could be wrong
		write(fd_ar,buffer,m);
	
	//off_t P_F=lseek(fd_ar,0,SEEK_CUR)-P_O;
	if(buff.st_size%2!=0)//check odd
	{
		//buffer[0]=' ';
		write(fd_ar," ",1);   
	}
	close(fd_file);
}
int main(int argc, char **argv)
{
	if(argc<3)
    {
        printf("input error!\n");
    }

    else
    {		
        char *key = argv[1];//command
        char *afile = argv[2];//archive file
        int fd_ar;
		if(access(afile,F_OK)==0)
		{
			char buff_test[9];
			fd_ar=open(afile, O_RDONLY);
			read(fd_ar,buff_test,8);
			buff_test[8]='\0';
			close(fd_ar);
			if(strcmp(buff_test,"!<arch>\n")!=0) 
			{
				printf("input error!\n");
				return 0;
			}		
		}		
		
        if(key[0]=='q'&& strlen(key)==1 && argc>=4)//there is at least one file to be appended		
        {			
            if(access(afile,F_OK)!=0)//no afile, creat
            {
                fd_ar = creat(afile,0666);
                write(fd_ar,ARMAG,SARMAG);//write magic string
            }
			else//open
            {
                fd_ar=open(afile,O_RDWR|O_APPEND);
            }
			int i=0;
			for(;i<argc-3;i++)//for all files need to append
            {
                char *file=argv[i+3];
				append(file,fd_ar);
            }
			close(fd_ar);
        }
        else if(key[0]=='x' && strlen(key)==1 && argc>=4)//extract
        {
            if(access(afile,F_OK)!=0)
            {
                printf("no archive!");
            }
            else
            {
                fd_ar=open(afile,O_RDONLY);                
                off_t P_INI=lseek(fd_ar,0,SEEK_END);
                off_t P_CUR= lseek(fd_ar,SARMAG,SEEK_SET);              
                int i;
				int flg[argc-3];//file in archive or not,0
				for(i=0;i<argc-3;i++)
				{
					flg[i]=0;
				}
				char buffc[1024];
				char namec[16];
				//char namestr[16];
				char modec[10];
				char sizec[10];						
				int sizef;
                while(1)
                {
					if(P_CUR==P_INI)
					{
						for(i=0;i<argc-3;i++)
						{
							if(flg[i]==1) printf("%s been extracted.\n",argv[i+3]);
							else printf("%s do not exist.\n",argv[i+3]);
						}
						break;
					}
					read(fd_ar,buffc,60);
					strncpy(namec, buffc, 16);
					strncpy(modec, buffc + 40, 8);
					strncpy(sizec, buffc + 48, 10);						
					sizef=atoi(sizec);
					for(i=0;i<argc-3;i++)
					{
						if(strncmp(namec,argv[i+3],strlen(argv[i+3]))==0&&flg[i]==0)//the same file name,never been extracted
						{
							//printf("equal!\n");
							int j,mode=0;
							for (j =3;j<6;j++)
								mode=mode*8+(modec[j]-'0');
							int fd_file = open(argv[i+3], O_WRONLY|O_CREAT|O_TRUNC, mode);
							char buffer_f[sizef];////////////////
							read(fd_ar,buffer_f,sizef);/////////
							write(fd_file,buffer_f,sizef);//////
							close(fd_file);
							flg[i]=1;
							break;
						}
						if(i==argc-4) lseek(fd_ar, sizef,SEEK_CUR);
					}											
					if(sizef%2==1)
						P_CUR=lseek(fd_ar,1,SEEK_CUR); 
                }
               close(fd_ar);
            }
            
        }
        else if(key[0]=='x'&& key[1]=='o' && strlen(key)==2 && argc>=4)//extract and mtime
        {
            if(access(afile,F_OK)!=0)
            {
                printf("no archive!");
            }
            else
            {
                fd_ar=open(afile,O_RDONLY);                
                off_t P_INI=lseek(fd_ar,0,SEEK_END);
                off_t P_CUR= lseek(fd_ar,SARMAG,SEEK_SET);              
                int i;
				int flg[argc-3];//file in archive or not,0
				for(i=0;i<argc-3;i++)
				{
					flg[i]=0;
				}
				char buffc[1024];
				char namec[16];							
				char datec[12];
				char uidc[6];
				char gidc[6];
				char modec[10];
				char sizec[10];					
				int sizef;
                while(1)
                {
					if(P_CUR==P_INI)
					{
						for(i=0;i<argc-3;i++)
						{
							if(flg[i]==1) printf("%s been extracted.\n",argv[i+3]);
							else printf("%s do not exist.\n",argv[i+3]);
						}
						break;
					}
					read(fd_ar,buffc,60);
					strncpy(namec, buffc, 16);
					strncpy(datec, buffc + 16, 12);
					strncpy(uidc, buffc + 28, 6);
					strncpy(gidc, buffc + 34, 6);
					strncpy(modec, buffc + 40, 8);
					strncpy(sizec, buffc + 48, 10);						
					sizef=atoi(sizec);
					for(i=0;i<argc-3;i++)
					{
						if(strncmp(namec,argv[i+3],strlen(argv[i+3]))==0&&flg[i]==0)//the same file name,never been extracted
						{
							//printf("equal!\n");
							int j,mode=0;
							for (j =3;j<6;j++)
								mode=mode*8+(modec[j]-'0');
							int fd_file = open(argv[i+3], O_WRONLY|O_CREAT|O_TRUNC, mode);
							char buffer_f[sizef];////////////////
							read(fd_ar,buffer_f,sizef);/////////
							write(fd_file,buffer_f,sizef);//////
							struct utimbuf buff_t;
							buff_t.actime = buff_t.modtime = strtol(datec,NULL,0);//0x
							utime(namec, &buff_t);
							chmod(namec, strtoul(modec,NULL,8));//octal
							close(fd_file);
							flg[i]=1;
							break;
						}
						if(i==argc-4) lseek(fd_ar, sizef,SEEK_CUR);
					}											
					if(sizef%2==1)
						P_CUR=lseek(fd_ar,1,SEEK_CUR); 
                }
               close(fd_ar);
            }
            
        }

        else if(key[0]=='t'&& strlen(key)==1)
        {
            if(access(afile,F_OK)!=0)
            {
                printf("no archive!");
            }
            else
            {
                fd_ar=open(afile,O_RDONLY);                
                off_t P_INI=lseek(fd_ar,0,SEEK_END);//store the current offset
                off_t P_CUR= lseek(fd_ar,SARMAG,SEEK_SET);                
                if(P_CUR>=P_INI)
                {
                    printf("no file in archive!");
                }
                else
                {
					
					char buffc[1024];
					char namec[16];
					char sizec[10];						
					int sizef;		
					if(argc==3)//print all files 
					{
						while(P_CUR<P_INI)
						{
							read(fd_ar,buffc,60);
							strncpy(namec, buffc, 16);
					
							strncpy(sizec, buffc + 48, 10);						
							sizef=atoi(sizec);
							
							printf("%s\n",namec);//print it out 
							lseek(fd_ar,sizef,SEEK_CUR);
							if (sizef%2==1)
                                lseek(fd_ar,1,SEEK_CUR);
                            P_CUR=lseek(fd_ar,0,SEEK_CUR);                            
                            
						}
					}
					else //print named file
					{
						int i = 0;
						for(;i<argc-3;i++)
						{
							char *file=argv[i+3];
							suffix(file,16);
							P_CUR= lseek(fd_ar,SARMAG,SEEK_SET);
							while(1)
							{								
								read(fd_ar,buffc,60);
								strncpy(namec, buffc, 16);
					
								strncpy(sizec, buffc + 48, 10);						
								sizef=atoi(sizec);
								
								if(strncmp(namec,file,15)==0)// this is the file, print
								{
									printf("%s\n",namec);
									break;
								}
								lseek(fd_ar,sizef,SEEK_CUR);
								if (sizef%2==1)
									lseek(fd_ar,1,SEEK_CUR);
								P_CUR=lseek(fd_ar,0,SEEK_CUR);  
								if(P_CUR>=P_INI)
								{
									printf("no entry %s in archive",namec);
									break;
								}
							}
						}
					}	
                }
               //lseek(fd_ar,P_INI,SEEK_SET); 
               close(fd_ar);
            }
        }
		else if(key[0]=='t'&&key[1]=='v'&&strlen(key)==2)
        {
			if(access(afile,F_OK)!=0)
            {
                printf("no archive!");
            }
            else
            {
                fd_ar=open(afile,O_RDONLY);                
                off_t P_INI=lseek(fd_ar,0,SEEK_END);//store the current offset
                off_t P_CUR= lseek(fd_ar,SARMAG,SEEK_SET);                
                if(P_CUR>=P_INI)
                {
                    printf("no file in archive!");
                }
                else
                {
					char buffc[1024];
					char namec[16];
					char namestr[16];
					char sizec[10];
					char datec[12];
					char uidc[6];
					char gidc[6];
					char modec[10];						
					int  sizef;
					char uper[3];
					char pper[3];	
					char oper[3];	

					if(argc==3)//print all files 
					{
						while(P_CUR<P_INI)
						{
							read(fd_ar,buffc,60);
							strncpy(namec, buffc, 16);
							strncpy(datec, buffc+16,12);
							strncpy(uidc,  buffc+28,6);
							strncpy(gidc,  buffc+34,6);
							strncpy(modec, buffc+40,8);
							strncpy(sizec, buffc+48,10);						
							sizef=atoi(sizec);
							switch(modec[3])//permission
							{
								case '0': stpcpy(uper,"---");  break;
								case '1': stpcpy(uper,"--x");  break;
								case '2': stpcpy(uper,"-w-");  break;
								case '3': stpcpy(uper,"-wx");  break;
								case '4': stpcpy(uper,"r--");  break;
								case '5': stpcpy(uper,"r-x");  break;
								case '6': stpcpy(uper,"rw-");  break;
								case '7': stpcpy(uper,"rwx");  break;
								default: break;
							}
							switch(modec[4])
							{
								case '0': stpcpy(pper,"---");  break;
								case '1': stpcpy(pper,"--x");  break;
								case '2': stpcpy(pper,"-w-");  break;
								case '3': stpcpy(pper,"-wx");  break;
								case '4': stpcpy(pper,"r--");  break;
								case '5': stpcpy(pper,"r-x");  break;
								case '6': stpcpy(pper,"rw-");  break;
								case '7': stpcpy(pper,"rwx");  break;
								default: break;
							}
							switch(modec[5])
							{
								case '0': stpcpy(oper,"---");  break;
								case '1': stpcpy(oper,"--x");  break;
								case '2': stpcpy(oper,"-w-");  break;
								case '3': stpcpy(oper,"-wx");  break;
								case '4': stpcpy(oper,"r--");  break;
								case '5': stpcpy(oper,"r-x");  break;
								case '6': stpcpy(oper,"rw-");  break;
								case '7': stpcpy(oper,"rwx");  break;
								default: break;
							}
							time_t seconds = atoi(datec);
							//char *t_point=ctime(&seconds);							
							
							int t=0;
							for(;t<16;t++)
							{
								
								if(namec[t]!=' ') namestr[t]=namec[t];
								else 
								{
									namestr[t]='\0';
									break;
								}
								
							}							
							printf("%s  %s%s%s  %d/%d   %6d   %s", 
							namestr,uper,pper,oper,atoi(uidc),atoi(gidc),atoi(sizec),ctime(&seconds));//print it out 
							
							lseek(fd_ar,sizef,SEEK_CUR);
							if (sizef%2==1)
                                lseek(fd_ar,1,SEEK_CUR);
                            P_CUR=lseek(fd_ar,0,SEEK_CUR);                            
                            
						}
					}
					else //print named file
					{
						int i = 0;
						for(;i<argc-3;i++)
						{
							char *file=argv[i+3];
							suffix(file,16);
							P_CUR= lseek(fd_ar,SARMAG,SEEK_SET);
							while(1)
							{								
								read(fd_ar,buffc,60);
								strncpy(namec, buffc, 16);
								strncpy(datec, buffc+16,12);
								strncpy(uidc,  buffc+28,6);
								strncpy(gidc,  buffc+34,6);
								strncpy(modec, buffc+40,8);
								strncpy(sizec, buffc+48,10);						
								sizef=atoi(sizec);
								
								if(strncmp(namec,file,15)==0)// this is the file, print
								{
									switch(modec[3])//permission
									{
										case '0': stpcpy(uper,"---");  break;
										case '1': stpcpy(uper,"--x");  break;
										case '2': stpcpy(uper,"-w-");  break;
										case '3': stpcpy(uper,"-wx");  break;
										case '4': stpcpy(uper,"r--");  break;
										case '5': stpcpy(uper,"r-x");  break;
										case '6': stpcpy(uper,"rw-");  break;
										case '7': stpcpy(uper,"rwx");  break;
										default: break;
									}
									switch(modec[4])
									{
										case '0': stpcpy(pper,"---");  break;
										case '1': stpcpy(pper,"--x");  break;
										case '2': stpcpy(pper,"-w-");  break;
										case '3': stpcpy(pper,"-wx");  break;
										case '4': stpcpy(pper,"r--");  break;
										case '5': stpcpy(pper,"r-x");  break;
										case '6': stpcpy(pper,"rw-");  break;
										case '7': stpcpy(pper,"rwx");  break;
										default: break;
									}
									switch(modec[5])
									{
										case '0': stpcpy(oper,"---");  break;
										case '1': stpcpy(oper,"--x");  break;
										case '2': stpcpy(oper,"-w-");  break;
										case '3': stpcpy(oper,"-wx");  break;
										case '4': stpcpy(oper,"r--");  break;
										case '5': stpcpy(oper,"r-x");  break;
										case '6': stpcpy(oper,"rw-");  break;
										case '7': stpcpy(oper,"rwx");  break;
										default: break;
									}
									time_t seconds = atoi(datec);							
							
									int t=0;
									for(;t<16;t++)
									{
										
										if(namec[t]!=' ') namestr[t]=namec[t];
										else 
										{
											namestr[t]='\0';
											break;
										}
										
									}							
									printf("%s  %s%s%s  %d/%d   %6d   %s", 
									namestr,uper,pper,oper,atoi(uidc),atoi(gidc),atoi(sizec),ctime(&seconds));//print it out 
									break;
								}
								lseek(fd_ar,sizef,SEEK_CUR);
								if (sizef%2==1)
									lseek(fd_ar,1,SEEK_CUR);
								P_CUR=lseek(fd_ar,0,SEEK_CUR);  
								if(P_CUR>=P_INI)
								{
									printf("no entry %s in archive",file);/////怎么又是。out啊！！
									break;
								}
							}
						}
					}	
                }
               //lseek(fd_ar,P_INI,SEEK_SET); 
               close(fd_ar);
            }
        }
        else if(key[0]=='d'&& strlen(key)==1)
        {
			
            if(access(afile,F_OK)!=0)
            {
                printf("no archive!");
            }
            
			else if (argc>3)
            {
                fd_ar=open(afile,O_RDONLY);                
                off_t P_INI=lseek(fd_ar,0,SEEK_END);
                off_t P_CUR= lseek(fd_ar,SARMAG,SEEK_SET);              
                int i;
				int flg[argc-3];//file in archive or not,0
				for(i=0;i<argc-3;i++)
				{
					flg[i]=0;
				}
				char buffc[1024];
				char namec[16];				
				char modec[10];
				char sizec[10];						
				int sizef;
				struct stat buff_old;
				fstat(fd_ar,&buff_old);
				unlink(afile);
				int fd_arcpy = creat(afile, buff_old.st_mode);										
				write(fd_arcpy,ARMAG,SARMAG);//write magic string	
                while(1)
                {
					if(P_CUR==P_INI)
					{
						for(i=0;i<argc-3;i++)
						{
							if(flg[i]==1) printf("%s been deleted.\n",argv[i+3]);
							else printf("%s do not exist.\n",argv[i+3]);
						}
						break;
					}
					read(fd_ar,buffc,60);
					strncpy(namec, buffc, 16);
					strncpy(modec, buffc + 40, 8);
					strncpy(sizec, buffc + 48, 10);						
					sizef=atoi(sizec);
					for(i=0;i<argc-3;i++)
					{
						if(strncmp(namec,argv[i+3],strlen(argv[i+3]))==0&&flg[i]==0)//the same file name,never been deleted
						{
							lseek(fd_ar, sizef,SEEK_CUR);
							flg[i]=1;
							break;
						}
						if(i==argc-4) 
						{	
							write(fd_arcpy,buffc,60);//////						
							char buffer_f[sizef];////////////////
							read(fd_ar,buffer_f,sizef);/////////							
							write(fd_arcpy,buffer_f,sizef);//////
							if(sizef%2==1)
								write(fd_arcpy," ",1);  
						}
					}											
					if(sizef%2==1)
						P_CUR=lseek(fd_ar,1,SEEK_CUR); 
                }
               close(fd_ar);
			   close(fd_arcpy);			   
            }            
            
          
        }
       else if(key[0]=='A'&& strlen(key)==1 && argc==3)
        {
            if(access(afile,F_OK)!=0)//no afile, creat
            {
                fd_ar = creat(afile,0666);
                write(fd_ar,ARMAG,SARMAG);//write magin string
            }
            else//open
            {
                fd_ar=open(afile,O_RDWR|O_APPEND);
            }            
            struct stat buff1;
            struct dirent *dir_en;
			dir_en = (struct dirent*)malloc(sizeof(struct dirent));//in case overflow
			DIR *cwd = opendir(".");
            dir_en=readdir(cwd);
            while(dir_en!=NULL)
            {				
                stat(dir_en->d_name,&buff1);
                if(S_ISREG(buff1.st_mode)&&(strcmp(dir_en->d_name,afile)!=0)&&(strcmp(dir_en->d_name,"t.c")!=0))//regular
                {					
                    char *file=dir_en->d_name;
                    append(file,fd_ar);
                }
				if(S_ISLNK(buff1.st_mode))//symbolic
				{
					char buf_tem[1024];
					long slnk=readlink(dir_en->d_name, buf_tem, 1024);//
					write(fd_ar, buf_tem, slnk);
					if (slnk % 2 == 1)
					write(fd_ar," ", 1);
				}
				dir_en=readdir(cwd);
            }
            close(fd_ar);
        }
	   
        else
        {
            printf("input error!\n");
        }
    }
     
    return 0;
}