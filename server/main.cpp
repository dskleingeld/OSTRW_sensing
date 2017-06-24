#include <iostream>
#include <thread>

#include "config.h"
#include "mainServer.h"

struct MHD_Daemon* daemon;
char* key_pem;
char* cert_pem;
void* arrayOfPointers[4];

int main(int argc, char* argv[])
{
	//load the ssl key and certificate (needed for https)										
  key_pem = load_file("server.key");
  cert_pem = load_file("server.pem");

  //check if key and cert where read okay
  if ((key_pem == NULL) || (cert_pem == NULL))
  {
    std::cout<<"The key/certificate files could not be read.\n";
    return 1;
  }

	//start the server
  daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY | MHD_USE_SSL,
														 config::HTTPSERVER_PORT, NULL, NULL,
                             &answer_to_connection, (void*)arrayOfPointers,
                             MHD_OPTION_HTTPS_MEM_KEY, key_pem,
                             MHD_OPTION_HTTPS_MEM_CERT, cert_pem,
                             MHD_OPTION_END);
  
  //check if the server started alright                           
  if(NULL == daemon)
    {
  		std::cout<<"server could not start\n";
      
      //free memory if the server crashed
      delete key_pem;
      delete cert_pem;

      return 1;
    }
    

	getchar();	//wait for enter to shut down
	std::cout<<"shutting https server down gracefully\n";
	
  //free memory if the server stops
  MHD_stop_daemon(daemon);
  delete key_pem;
  delete cert_pem;
  
  return 0;      
}

