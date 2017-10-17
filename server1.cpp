/*Name:Riddhi Patel 
Roll No:20162082*/


#include<iostream>
#include<string>
#include<ctime>

#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/param.h>
#include<sys/types.h>

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<unistd.h>
#include<netdb.h>

#include<map>
#include<vector>
#include<utility>

using namespace std;

int repofd,logfd;
map <string , vector < pair < string , string > > > repomap;

/****open and load repo file****/

void openandloadrepo(){
	
	ssize_t ret;	
	char buffer[1];
	string filename,path,ipaddr,temp;
	int flag=0;
	
	
	logfd=open("repo.log",O_RDWR | O_APPEND | O_CREAT ,0777);
	
	if(logfd == -1){
		cout<<"repo.log file can not be opened.\n";
		exit(1);
	}
	
	repofd=open("repo.txt",O_RDWR | O_CREAT ,0777);
	
	if(repofd == -1){
		cout<<"repo.txt file can not be opened.\n";
		exit(1);
	}
		
	temp="";
	while(ret=read(repofd,buffer,1) > 0){
	
		if(buffer[0] == '#' || buffer[0] == '\n'){
		
				if(flag==0){
					filename=temp;
					flag=(flag+1)%3;
					temp="";	
				}
				else if(flag==1){
					path=temp;
					flag=(flag+1)%3;
					temp="";
				}
				else if(flag==2){
					ipaddr=temp;
					repomap[filename].push_back(make_pair(path,ipaddr));
					flag=(flag+1)%3;
					temp="";
				}
			}
			else{
				temp+=buffer;
			}
		
	}
	
}



/****print repomap****/

void printrepomap(){
	//cout<<"size of map in printrepomap  :"<<repomap.size()<<"\n\n";

	for(map <string , vector < pair < string , string > > > :: iterator i=begin(repomap);i!=end(repomap);++i){
		string const& file=i->first;
		vector< pair < string , string > > &mirror=i->second;
		
		for(int i=0;i<mirror.size();i++){	
			cout<<file<<"\t"<<mirror[i].first<<"\t"<<mirror[i].second<<"\n";
		}
	}
	cout<<endl<<endl;
}

/*******************************log file************************************/


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
/**********************search function*****************************/

void filesearch(string buffer,int n,int forkfd){
	string buffer1 = "";
	string filename=buffer.substr(1,n-1);
	int i;

	vector < pair < string , string > > v =  repomap[filename];
	
	if(v.size() == 0){
		buffer1+="@File not Found...............";
	}
	else{
	
		for(i=0;i<v.size();i++){
			string p=v[i].first;
			string ip=v[i].second;
		
			buffer1+=p;
			buffer1+="\t";
			buffer1+=ip;
			buffer1+="\n";
		
		}

		cout << buffer1 << endl;
		
	
	}

		if((n = write(forkfd,buffer1.c_str(),buffer1.length())) > 0){
			//cout<<"mirror sent"<<endl;
		}
		else{
			cout<<"error in sending mirror"<<endl;
		}
}


/*********************share function*************************/

void sharefile(string buffer,string client_ip,int n){
	
	/*****find filename path and ipaddress of client and add it to repomap*****/
	
	string filename,path,ipaddr;
	string temp;
	int flag=0,i=0,j=0;

	string temp3=buffer.substr(1,n-1);
	temp3+=client_ip;
	temp3=temp3+="\n";

	/*********write it into file********/
	const char *s=temp3.c_str();
	write(repofd,s,temp3.size());
	
	for(i=0;i<temp3.size();i++){
		if(temp3[i] == '#' || temp3[i] == '\n'){
		
				if(flag==0){
					
					filename=temp;
					flag=(flag+1)%3;
					temp='\0';	
				}
				else if(flag==1){
					
					path=temp;
					flag=(flag+1)%3;
					
					temp='\0';
				}
				else if(flag==2){
					temp[j]='\0';
					ipaddr=temp;

					repomap[filename].push_back(make_pair(path,ipaddr));
					printrepomap();

					flag=(flag+1)%3;
					temp='\0';
					break;
				}
			}
			else{
				temp+=temp3[i];
			}
		
		}
				
}	
	


/***serve the client***/

void doService(int forkfd,string client_ip){
	int logfd,i=0;
	int n;
	char buffer[256];
	string logbuffer;
	
	
	while(1){
	
		if((n=read(forkfd,&buffer,256)) > 0){
	
		switch(buffer[0]){
	
		case '1':{	
				repomap.erase(repomap.begin(),repomap.end());
				openandloadrepo();
				logFile("Search request from ",client_ip);			
				filesearch(buffer,n,forkfd);
				logFile("Search response sent to ",client_ip);				
				break;
			}
		case '2':{
				logFile("Share request from ",client_ip);	
				sharefile(buffer,client_ip,n);
				logFile("Share ack sent to ",client_ip);
				repomap.erase(repomap.begin(),repomap.end());
				openandloadrepo();
				break;
			}
		default:         break;
		}
	
		}
	}

}


int main(int argc,char **argv){
	
	int clifd,forkfd;
	int pid;
	socklen_t client_length;
	struct sockaddr_in server_addr,client_addr;
	
	
	/**call to socket function***/
	clifd=socket(AF_INET,SOCK_STREAM,0);
	
	if(clifd < 0){
		cout<<"Error in opening socket..."<<clifd<<endl;
		exit(1);
	}


	/***open repo.txt and load it into map***/
		openandloadrepo();
		//printrepomap();
	
	/***initialize socket structure***/
	
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=INADDR_ANY;
	server_addr.sin_port=htons(atoi(argv[1]));
	
	
	/***bind host address***/
	
	if(bind(clifd,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0){
		perror("Error in binding......");
	}
	

	
	
	if(repofd == -1){
		cout<<"repo.txt can not be opened...."<<endl;
		exit(1);
	}
	
	
	/****listening for clients****/
	
	if(!(listen(clifd,100))){

		write(1,"Waiting For Client...\n",22);
	
		client_length=sizeof(client_addr);
		while(1){
			client_length=sizeof(client_addr);
			
			if((forkfd=accept(clifd,(struct sockaddr*)&client_addr,&client_length)) >= 0)
			{
				
				
				string client_ip=inet_ntoa(client_addr.sin_addr);
				
				pid=fork();
	
				if(pid < 0){
					cout<<"Error infork...\n";
					exit(1);
				}
				if(pid == 0){
			
					doService(forkfd,client_ip);
					exit(0);
				}else{
					close(forkfd);
				}

				
			}
			else{
			perror("while accepting.....");
			exit(1);
			}
		
		}
	}
	else{
		perror("error in listening.....");
	}
	
	return 0;
}
