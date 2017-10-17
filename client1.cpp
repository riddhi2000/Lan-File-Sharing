/*Name:Riddhi Patel 
Roll No:20162082*/

#include<iostream>
#include<string.h>
#include<string>

#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<stdio.h>

#include<sys/types.h>
#include<sys/socket.h>

#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<netdb.h>

#include<vector>
#include<utility>

#define port 9999

using namespace std;

int logfd;


/*****************************************************logfile********************************************************************************/


void logFile(string msg , string ipaddr){

   string s,t;
	// current date/time based on current system
   time_t now = time(0);
	
   //cout << "Number of sec since January 1,1970:" << now << endl;
	
   tm *ltm = localtime(&now);
   
   t=to_string(ltm->tm_mday) ;
	if(t.length() < 2)
		t="0"+t;
   s+=t;
   s+="-";
   
   t=to_string((1 + ltm->tm_mon));
   if(t.length() < 2)
		t='0'+t;
   s+=t;
   s+="-";
   s+=to_string((1900 + ltm->tm_year));
   s+=" ";
   t=to_string(1 + ltm->tm_hour);
   if(t.length() < 2)
		t='0'+t;
   s+=t;
   s+="-";
   t=to_string(1 + ltm->tm_min);
   if(t.length() < 2)
		t='0'+t;
   s+=t;
   s+="-";
   t=to_string(1 + ltm->tm_sec);
   if(t.length() < 2)
		t='0'+t;
   s+=t;
   s+=":";
   s+=msg;
   s+=ipaddr;
   s+="\n";
   write(logfd,s.c_str(),s.length());
}

/********************************************************************uploading***************************************************************/
void uploading(int sfd,string client_ip){
	int n,i,read_fd;
	string path;
	char buffer[256];
	
	logFile("Download request from ",client_ip);			
				
	
	if( (n = read(sfd,buffer,256) ) > 0){
		for(i=0;i<n;i++)
		path+=buffer[i];
		
	}
	
	
	read_fd=open(path.c_str(),O_RDONLY,0777);
	
	if(read_fd == -1){
		cout << " file can not be opened.\n" <<endl;
		shutdown(sfd, SHUT_WR);
		close(read_fd);
		close(sfd);
		return;
	}
	
	else{
		bzero(buffer,0);
		while((n=read(read_fd,&buffer,1)) > 0){
			write(sfd,buffer,1);
		}	
		shutdown(sfd, SHUT_WR);	
		logFile("File sent to ",client_ip);	
		close(read_fd);
		close(sfd);
		return;
	}
}

/*********************************************downloadserver code************************************************************************/
void downloadserver(){
	
	int clifd,forkfd;
	int pid;
	int client_length;
	struct sockaddr_in server_addr,client_addr;
	
	
	/**call to socket function***/
	clifd=socket(AF_INET,SOCK_STREAM,0);
	
	if(clifd < 0){
		cout<<"Error in opening socket..."<<clifd<<endl;
		exit(1);
	}
	
	/***initialize socket structure***/
	
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=INADDR_ANY;
	server_addr.sin_port=htons(port);
	
	
	/***bind host address***/
	
	if(bind(clifd,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0){
		perror("Error in binding...");
	}
	

	logfd=open("client.log",O_RDWR | O_APPEND | O_CREAT ,0777);
	
	if(logfd == -1){
		cout<<"repo.txt file can not be opened.\n";
		exit(1);
	}
	
	/****listening for clients****/
	
	listen(clifd,100);

	while(1){
		client_length=sizeof(client_addr);
		forkfd=accept(clifd,(struct sockaddr*)&client_addr,(socklen_t*)&client_length);
		string client_ip=inet_ntoa(client_addr.sin_addr);
		if(forkfd < 0){
			cout<<"Error in accepting connection from downloadserver server...\n";
			exit(1);
		}
	
		pid=fork();
	
		if(pid < 0){
			cout<<"Error infork...\n";
			exit(1);
		}
		if(pid == 0){
			uploading(forkfd,client_ip);
			close(forkfd);
			exit(0);
		}
		
	}
}


/******************************************download connection for recipent client*******************************************************/

void downloadfromclient(string path ,string file ,string ipaddr){
	path+="/";
	path+=file;
		

	int servicefd,n,flag;
	struct hostent *server;
	struct sockaddr_in server_addr;
	char buffer[256];
	
	/**creating socket***/
	
	servicefd=socket(AF_INET,SOCK_STREAM,0);

	
	if(servicefd < 0){
		cout<<"\n Error in creating socket...\n";
		exit(1);
	}

	server=gethostbyname(ipaddr.c_str());
	
	if(server == NULL){
		cout<<"No such host....\n";
		exit(0);
	}
	

	bzero((char *) &server_addr, sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length); 
	server_addr.sin_port=htons(port);	
	
	/***connect to server***/
	
	if(connect(servicefd,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0){
		cout<<"Error in connecting downloadserver client...\n";
		exit(1);
	}	

	if( (n=write(servicefd,path.c_str(),path.length())) > 0 ){
	}


	/******create file at client side and write in it**********/
	
	int write_fd=open(file.c_str(),O_RDWR | O_CREAT ,0777);
	
	if(write_fd == -1){
		cout<< " file can not be opened.\n"<<endl;
		exit(1);
	}
	
	bzero(buffer,0);
	flag=0;

	while((n=read(servicefd,&buffer,1)) > 0){
		write(write_fd,buffer,1);
		flag=1;	
	}
	
	if(flag == 1){
	cout<<"Download Complete!..........\n"<<endl;
	}else
	{
	write(1,"File can not be downloaded..\n",30);
	}

	close(servicefd);
	close(write_fd);	
	return;
}

/*******************************************************search module*******************************************************************/
void search(int servfd,char **argv){

	string file,temp,path,addr,file1;
	vector < pair < string ,string > > v;
	int mirror,n,i,j;
	char buffer[256];

	cout<<"Type string to search....";
	cin>>file;
	file1=file;
	
	file="1"+file;
	const char *s=file.c_str();

	if( (n=write(servfd,s,file.length())) > 0 ){
	}

	
	
	
	if((n = read(servfd,buffer,256)) > 0){

		if(buffer[0] == '@'){
			cout<<"File does not exist!........\n"<<endl;
			return;
		}
		else{
			cout<<"Select a mirror:...."<<endl;

			for(i=0;i<n;i++){

				if(buffer[i] == '\n'|| buffer[i] == '\t'){			
			
					if(buffer[i] == '\t'){
						path=temp;
						temp="";
					}
					else if(buffer[i] == '\n'){
						addr=temp;
						v.push_back(make_pair(path,addr));
						temp="";
					}
				}
				else{
					temp+=buffer[i];
				}
			}
		
		
			for(i=0;i<v.size();i++){
				cout << i+1 << ". " << v[i].first <<"\t"<< v[i].second << endl;
			}
		
	

			cin>>mirror;

			if(mirror > v.size() || mirror <= 0){
				write(1,"Wrong option selected......\n",28);
			}
			else{
			downloadfromclient(v[mirror-1].first ,file1 ,v[mirror-1].second);
			}
			return;
		}

	}
}

/******************************share module****************************************/
void share(int servfd,char **argv){
	string path,ipaddr,mpath;
	int index=0;
	
	cout<<"Path :";
	cin>>path;
	
	for(int i=path.size();i>=0;i--){
		if(path[i] == '/'){
			index=i;
			break;
		}
	}
	
	string file=path.substr(index+1,path.size());
	file+="#";
	string dir=path.substr(0,index);
	file+=dir;
	file="2"+file+"#";

	cout<<file;
	
	const char *s=file.c_str();
	cout<<file.length();
	write(servfd,s,file.length());
	write(1,"File Shared Successfully on repo!...\n",40);
}

/********************************************exit file descriptor****************************************************************/
void exitfd(int servfd){
	close(servfd);
	exit(0);
}

/****************************************client server*******************************************************************************/
void clientserver(int argc,char **argv){

	int servfd,n,z;
	struct hostent *server;
	struct sockaddr_in server_addr;
	socklen_t size;
	char buffer[256];

	if(argc <3){
		perror("Error in no of argument....\n");
		exit(0);
	}
	
	/**creating socket***/
	
	servfd=socket(AF_INET,SOCK_STREAM,0);

	
	if(servfd < 0){
		perror("\n Error in creating socket...\n");
		exit(1);
	}

	server=gethostbyname(argv[1]);
	
	if(server == NULL){
		perror("No such host....\n");
		exit(0);
	}
	
	bzero((char *) &server_addr, sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);   
	server_addr.sin_port=htons(atoi(argv[2]));	
	
	/***connect to server***/
	size=sizeof(server_addr);
	if((z=(connect(servfd,(struct sockaddr*)&server_addr,size))) < 0){
		cout<<"the value of z....:"<<z<<endl;
		perror("Error in connecting in client server .....\n");
		exit(1);
	}	
	else{

	
		while(1){
		
			int option;
		
			cout<<"Select:\n";
			cout<<"1. Search:\n";
			cout<<"2. Share:\n";
			cout<<"3. Exit:\n";
			cout<<">>";
			cin>>option;
			
				if(option == 1 || option == 2 || option == 3)
				{
					switch(option){
		
						case 1: {
							search(servfd,argv);
							break;
							}
				
						case 2:	{
							share(servfd,argv);
							break;
							}		
						case 3:{	
							exitfd(servfd);
							//exit(0);
							break;
							}
		
					}
				}else{
					
					write(1,"Please Enter correct option...\n",30);
					
				}

			}
		exit(0);
			
	}		


}

/************************************main program*******************************************************************/
int main(int argc,char ** argv){
	
	//clientserver(argc,argv);
	int pid=fork();
	
	if(pid < 0){
		cout<<"Error infork...\n";
		exit(1);
	}
	if(pid > 0)
	{	
		clientserver(argc,argv);
	}
	
	if(pid == 0 ){
		for(int i=0;i<10000;i++);
		downloadserver();
	}
		
	
	return 0;
}
