#include <iostream>
#include <thread> //FIXME needed?
#include <microhttpd.h>
#include <stdlib.h> // atoi
#include <fstream> //writing to files etc

#include "config.h"
#include "mainServer.h"

struct MHD_Daemon* server;
char* key_pem;
char* cert_pem;
void* arrayOfPointers[4];

int main(int argc, char* argv[])
{
  int port = config::HTTPSERVER_PORT;
  const char* keyPath = "privkey1.pem";
  const char* certPath = "fullchain1.pem";

  //parse arguments
  if(argc > 1){
    port = atoi(argv[0]);
    if(argc > 2){
      keyPath = argv[1];
      if(argc > 3){
        certPath = argv[2];
      }
    }
  }

  //load the ssl key and certificate (needed for https)                    
  key_pem = load_file(keyPath);
  cert_pem = load_file(certPath);

  key_pem = load_file("privkey1.pem");
  cert_pem = load_file("fullchain1.pem");

  //check if key and cert were read okay
  if ((key_pem == NULL) || (cert_pem == NULL)) {
    std::cout<<"The key/certificate files could not be read.\n"
    <<"please put \""<<keyPath<<"\" and \""<<keyPath<<"\" in the same dir as this "
    <<"program.\n";
    return 1;
  }

  //open file for writing
  std::ofstream outfile("data.txt", std::ofstream::out | std::ofstream::app);
  if(!outfile.is_open()){std::cerr<<"Couldn't open 'output.txt'\n"; return 1;}
  
  //start the server
  server = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY | MHD_USE_SSL,
                             port, NULL, NULL,
                             &answer_to_connection, toVoidArr(&outfile),
														 MHD_OPTION_NOTIFY_COMPLETED, &request_completed, NULL,
                             MHD_OPTION_HTTPS_MEM_KEY, key_pem,
                             MHD_OPTION_HTTPS_MEM_CERT, cert_pem,
                             MHD_OPTION_END);
  
  //check if the server started alright                           
  if(NULL == server) {
      std::cout<<"server could not start\n";
      
      //free memory if the server crashed
      delete key_pem;
      delete cert_pem;

      return 1;
  } else {
    std::cout<<"started server with paramaters:\n  port="
    <<port<<", ssl key:"<<keyPath<<", ssl cert:"<<certPath<<"\n"
    <<"\nPress [Enter] to close the server";
  
  } 

  getchar();  //wait for enter to shut down
  std::cout<<"shutting down server\n";
  
  //free memory if the server stops
  MHD_stop_daemon(server);
  delete key_pem;
  delete cert_pem;
  
  std::cout<<"done\n";
  return 0;      
}

