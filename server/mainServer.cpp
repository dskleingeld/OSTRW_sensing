#include "mainServer.h"

const char* test_page = "<html><body>Test Page.</body></html>";
const char* unknown_page = "<html><body>Unknown Page.</body></html>";
const char* OK = "OK";
const char* greatingpage="<html><body><h1>Welcome, %s!</center></h1></body></html>";


const char *askpage = "<html><body>\
                       What's your name, Sir?<br>\
                       <form action=\"/namepost\" method=\"post\">\
                       <input name=\"tempHumid\" type=\"text\">\
                       <input type=\"submit\" value=\" Send \"></form>\
                       </body></html>";

inline uint32_t unix_timestamp() {
  time_t t = std::time(0);
  uint32_t now = static_cast<uint32_t> (t);
  return now;
}

inline int authorised_connection(struct MHD_Connection* connection){
  bool fail = true;
  char* pass = nullptr;
  char* user = MHD_basic_auth_get_username_password(connection, &pass);
  fail = ( (user == nullptr) ||
         (0 != strcmp(user, config::HTTPSERVER_USER)) ||
         (0 != strcmp(pass, config::HTTPSERVER_PASS)) );
  free(user);
  free(pass);// cant use delete here as MHD uses malloc internally

  return !fail ;
  //return true;
}


int answer_to_connection(void* cls,struct MHD_Connection* connection, const char* url,
                         const char* method, const char* version, const char* upload_data,
                         size_t* upload_data_size, void** con_cls) {
  int ret;
  struct MHD_Response* response;
	struct connection_info_struct* con_info;

  std::ofstream* outfile;
  fromVoidArr(cls, outfile);

  //if start of connection, remember connection and store usefull info about
	//connection if post create post processor.
  if (NULL == *con_cls) {
		con_info = new connection_info_struct;
		con_info->answerstring = nullptr;

		//set up post processor if post request
		if(0 == strcmp (method, MHD_HTTP_METHOD_POST)){
			con_info->outfile_mutex = mutexFromVoidArr(cls);
			con_info->outfile = outfileFromVoidArr(cls);
			con_info->connectiontype = POST;
			con_info->outfile = outfile;
		}
		//als geen post request set connectiontype to GET
		else con_info->connectiontype = GET;

		//in all cases make sure con_cls has a value
		*con_cls = (void*)con_info;
    return MHD_YES;
  }

  //correct password, repond dependig on url
  if (authorised_connection(connection)){

    if (0 == strcmp (method, "GET")){
      //create diffrent pages (responses) to different url's
      if (strcmp(url, "/askpage") == 0) {
        response = MHD_create_response_from_buffer(strlen (askpage),
        (void *) askpage, MHD_RESPMEM_PERSISTENT);

      }
      else if (strcmp(url, "/data.txt") == 0) {
    		std::lock_guard<std::mutex> lock(*mutexFromVoidArr(cls));
   			std::string contents;
    		{
    		std::fstream* outfile = outfileFromVoidArr(cls);
		    contents.resize(outfile->tellp());
		    outfile->seekp(0, std::ios::beg);
				outfile->read(&contents[0], contents.size());
    		}

        response = MHD_create_response_from_buffer(contents.size(),
        (void *) contents.c_str(), MHD_RESPMEM_PERSISTENT);
      	MHD_add_response_header(response, "Content-Type", "application/octet-stream");
      }
      else {
        response = MHD_create_response_from_buffer(strlen (unknown_page),
        (void *) unknown_page, MHD_RESPMEM_PERSISTENT);
      }
    //prepare respons to be send to server
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    }

    else if (0 == strcmp (method, "POST")) {
			struct connection_info_struct* con_info = (connection_info_struct*)*con_cls;

      if (*upload_data_size != 0)	{

        MHD_post_process(con_info->postprocessor, upload_data,
                         *upload_data_size);
        *upload_data_size = 0;
				std::cout<<"done with POST\n";
        return MHD_YES;
      }
			else if (nullptr != con_info->answerstring){
				response = MHD_create_response_from_buffer(strlen (unknown_page),
        (void *) unknown_page, MHD_RESPMEM_PERSISTENT);
				ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
			}
    }

  //incorrect password, present go away page
  }
  else {
    const char* page = "<html><body>Incorrect password.</body></html>";
    response = MHD_create_response_from_buffer(strlen (page), (void*) page,
	       MHD_RESPMEM_PERSISTENT);
    ret = MHD_queue_basic_auth_fail_response(connection, "test", response);
  }

  MHD_destroy_response(response); //free memory of the respons
  return ret;
}

int iterate_post(void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
												const char *filename, const char *content_type,
												const char *transfer_encoding, const char *data,
												uint64_t off, size_t size) {


	char* answerstring = "nice post!";
  struct connection_info_struct* con_info = (connection_info_struct*)coninfo_cls;

  if (0 == strcmp (key, "tempHumid")) {
    if ((size > 0) && (size <= config::MAXNAMESIZE)) {
			answerstring = new char[config::MAXANSWERSIZE];
			std::cout<<"data: "<<data<<"\n";

			con_info->data = new uint8_t[size];
			memcpy(con_info->data, data, size);
      con_info->answerstring = answerstring;
    }
    else con_info->answerstring = nullptr;
		std::cout<<"done iterating?\n";
		return MHD_NO;//inform postprocessor no further call to this func are needed
  }
  return MHD_YES;
}


void request_completed(void *cls, struct MHD_Connection *connection,
     		        			 void **con_cls, enum MHD_RequestTerminationCode toe) {

  struct connection_info_struct* con_info = (connection_info_struct*)*con_cls;

  if (NULL == con_info) return;
	//do cleanup for post if the con type was a post
  if (con_info->connectiontype == POST) {
		std::lock_guard<std::mutex> lock(outfile_mutex);
		*(con_info->outfile)<<std::to_string(unix_timestamp())<<", "
	                      <<con_info->data<<"\n";


    MHD_destroy_post_processor (con_info->postprocessor);
    delete[] con_info->answerstring;
  }

  delete con_info;
  *con_cls = NULL;
}





//used by load_file to find out the file size
long get_file_size(const char *filename) {
  FILE *fp;

  fp = fopen (filename, "rb");
  if (fp) {
    long size;

    if ((0 != fseek (fp, 0, SEEK_END)) || (-1 == (size = ftell (fp))))
      size = 0;

    fclose(fp);

    return size;
  }
  else
    return 0;
}

//used to load the key files into memory
char* load_file(const char* filename) {
  FILE *fp;
  char* buffer;
  unsigned long size;

  size = get_file_size(filename);
  if (0 == size)
    return nullptr;

  fp = fopen(filename, "rb");
  if (!fp)
    return nullptr;

  buffer = new char[size + 1];
  if (!buffer) {
      fclose (fp);
      return nullptr;
  }
  buffer[size] = '\0';

  if (size != fread(buffer, 1, size, fp)) {
      free(buffer);
      buffer = nullptr;
  }

  fclose(fp);
  return buffer;
}


void toVoidArr(void* arrayOfPointers[1], std::fstream* outfile,
std::mutex* outfile_mutex){

  //convert arguments
  arrayOfPointers[0] = (void*)outfile;
  arrayOfPointers[1] = (void*)outfile_mutex;

  return;
}

inline void fromVoidArr(void* cls, std::fstream*& outfile){
  void** arrayOfPointers;
  void* element1;

  //convert arguments back (hope this optimises well),
  arrayOfPointers = (void**)cls;
  element1 = (void*)*(arrayOfPointers+0);
  outfile = (std::fstream*)element1;
  return;
}

inline std::fstream* outfileFromVoidArr(void* cls){
  void** arrayOfPointers;
  void* element1;

  //convert arguments back (hope this optimises well),
  arrayOfPointers = (void**)cls;
  element1 = (void*)*(arrayOfPointers+0);
  return (std::fstream*)element1;
}

inline std::mutex* mutexFromVoidArr(void* cls){
  void** arrayOfPointers;
  void* element2;

  //convert arguments back (hope this optimises well),
  arrayOfPointers = (void**)cls;
  element2 = (void*)*(arrayOfPointers+1);
  return (std::mutex*)element2;
}
