    #include "client_headers.h"
    // #include  "defs.h"


    char ns_ip[4096];
    int ns_port;

    int main(int argc, char *argv[])
    {
        memset(ns_ip,0,sizeof(ns_ip));
        if(argc!=3)
        {
            printf("INVALID INPUT CANNOT CONNECT TO NS\n");
            exit(1);
        }
        int sock = 0;
        ns_port=atoi(argv[2]);
        strcpy(ns_ip,argv[1]);
        printf("%s %d\n",ns_ip,ns_port);
        // printf("Connected to NFS Server\n");
        while(1)
        {
            // printf("NOOOO");
            printf("%s---ENTER THE OPERATION YOU WANT TO DO---%s\n",YELLOW_CODE,RESET_CODE);
            // char c;
            // while((c = getchar()) != '\n' && c != EOF);
            // printf("CLER\n");
            char input[INPUT_SIZE];
            memset(input,'\0',sizeof(input));
            fgets(input,INPUT_SIZE,stdin);
            // scanf("%[^\n]%*c",input);
            input[strlen(input) - 1]='\0';
            char* token=strtok(input," ");
            if(strcmp(token,"exit")==0)
            {
                printf("%s---THANKS FOR CONNECTING---%s\n",PINK_CODE,RESET_CODE);
                break;
            }       
            else if(strcmp(token,READ)==0)
            {
                token=strtok(NULL," ");
                if(token==NULL)
                {
                    printf("%sPATH NOT MENTIONED\n%s",RED_CODE,RESET_CODE);
                    continue;
                }
                char path[INPUT_SIZE];
                memset(path,0,sizeof(path));
                strcpy(path,token);
                token=strtok(NULL," ");
                if(token!=NULL)
                {
                    printf("%sINVALID INPUT\n%s",RED_CODE,RESET_CODE);
                    continue;
                }
                c_read_from_path(path);
            }
            else if(strcmp(token,GET_SIZE_AND_PERMISSIONS)==0)
            {
                token=strtok(NULL," ");
                if(token==NULL)
                {
                    printf("%sPATH NOT MENTIONED\n%s",RED_CODE,RESET_CODE);
                }
                char path[INPUT_SIZE];
                memset(path,0,sizeof(path));
                strcpy(path,token);
                token=strtok(NULL," ");
                if(token!=NULL)
                {
                    printf("%sINVALID INPUT\n%s",RED_CODE,RESET_CODE);
                    continue;
                }
                c_get_size_and_permissions(path);
            }
            else if(strcmp(token,COPY)==0)
            {
                token=strtok(NULL," ");
                if(token==NULL)
                {
                    printf("%sPATH NOT MENTIONED\n%s",RED_CODE,RESET_CODE);
                    continue;
                }
                char source[INPUT_SIZE];
                strcpy(source,token);
                token=strtok(NULL," ");
                if(token==NULL)
                {
                    printf("%sPATH NOT MENTIONED\n%s",RED_CODE,RESET_CODE);
                    continue;
                }
                char dest[INPUT_SIZE];
                strcpy(dest,token);
                token=strtok(NULL," ");
                if(token!=NULL)
                {
                    printf("%sINVALID INPUT\n%s",RED_CODE,RESET_CODE);
                    continue;
                }
                c_copy_at_path(source,dest);
            }
            else if(strcmp(token,STREAM_AUDIO)==0)
            {
                token=strtok(NULL," ");
                if(token==NULL)
                {
                    printf("%sPATH NOT MENTIONED\n%s",RED_CODE,RESET_CODE);
                    continue;
                }
                char source[INPUT_SIZE];
                strcpy(source,token);
                token=strtok(NULL," ");
                if(token!=NULL)
                {
                    printf("%sINVALID INPUT\n%s",RED_CODE,RESET_CODE);
                    continue;
                }
                c_play_audio(source);
            }
            else if(strcmp(token,CREATE)==0)
            {
                token=strtok(NULL," ");
                if(token==NULL)
                {
                    printf("%sPATH NOT MENTIONED\n%s",RED_CODE,RESET_CODE);
                    continue;
                }
                char path[INPUT_SIZE];
                strcpy(path,token);
                token=strtok(NULL," ");
                if(token==NULL)
                {
                    printf("%sNAME NOT MENTIONED\n%s",RED_CODE,RESET_CODE);
                    continue;
                }
                char name[INPUT_SIZE];
                strcpy(name,token);
                token=strtok(NULL," ");
                if(token!=NULL)
                {
                    printf("%sINVALID INPUT\n%s",RED_CODE,RESET_CODE);
                    continue;
                }
                c_create_at_path(path,name);
            }
            else if(strcmp(token,DELETE)==0)
            {
                token=strtok(NULL," ");
                if(token==NULL)
                {
                    printf("%sPATH NOT MENTIONED\n%s",RED_CODE,RESET_CODE);
                    continue;
                }
                char path[INPUT_SIZE];
                strcpy(path,token);
                token=strtok(NULL," ");
                if(token!=NULL)
                {
                    printf("%sINVALID INPUT\n%s",RED_CODE,RESET_CODE);
                    continue;
                }
                c_delete_at_path(path);
            }
            else if(strcmp(token,WRITE)==0)
            {
                token=strtok(NULL," ");
                if(token==NULL)
                {
                    printf("%sPATH NOT MENTIONED\n%s",RED_CODE,RESET_CODE);
                    continue;
                }
                char path[INPUT_SIZE];
                strcpy(path,token);
                token=strtok(NULL," ");
                if(token!=NULL)
                {
                    printf("%sINVALID INPUT\n%s",RED_CODE,RESET_CODE);
                    continue;
                }
                c_write_to_path(path,0);
            }
            else if(strcmp(token,"WRITE--")==0)
            {
                token=strtok(NULL," ");
                if(token==NULL)
                {
                    printf("%sPATH NOT MENTIONED\n%s",RED_CODE,RESET_CODE);
                    continue;
                }
                char path[INPUT_SIZE];
                strcpy(path,token);
                token=strtok(NULL," ");
                if(token!=NULL)
                {
                    printf("%sINVALID INPUT\n%s",RED_CODE,RESET_CODE);
                    continue;
                }
                c_write_to_path(path,1);
            }
            else if(strcmp(token,LIST_PATHS)==0)
            {
                token = strtok(NULL," ");
                if(token!=NULL) 
                {
                    printf("%sINVALID INPUT\n%s",RED_CODE,RESET_CODE);
                    continue;
                }
                c_list_paths();
            }
            else
            {
                printf("%sINVALID OPERATION\n%s",RED_CODE,RESET_CODE);
            }
        }
    return 0;
    }
