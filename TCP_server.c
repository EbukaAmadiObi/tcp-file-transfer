/*  EEEN20060 Communication Systems Group 42 Option 2 
    TCP_Server program, to communicate with our TCP_Client program.

    This server listens on a specified port until a client connects.
    It'll send a welcome message then wait for a request from the client.
    Then the server sends a response to the client, based on the request.
    If there's an upload request, it'll respond with either yes or no, 
      with some information if it says no.
    If there's a download request, it'll respond with a yes or no, 
      and some information.
    Once a positive response is sent, the server either accepts a
      stream of data (upload) or sends back a stream of data (download).
    There are other request scenarios aswell, but upload and download are
      the main important ones.
    Once an upload or download request has been processed then responded
      to, the server will wait for the client to state if the connection
      should be kept open or not to fulfill another request.
    This connection will remain open until the client asks to close the
      connection. After this, the server will loop back to listen for
      another connection request. 
*/

#include <stdio.h>          // input and output functions
#include <string.h>         // string handling functions

#include <winsock2.h>	 // Windows socket functions and structs
#include <ws2tcpip.h>	 // Windows socket functions and structs

// This header file must be included AFTER the libraries above
#include "CS_TCP.h"		 // TCP functions for this assignment

// Port number - this port has been opened in the firewall on the lab PCs
#define SERVER_PORT 32980  // port to be used by the server

// Markers used for this example protocol
#define ENDMARK 10    // the newline character  is the end marker
#define CR 13     // the CR character is ignored when counting end markers
#define DOWNLOAD 68     // the download marker in the request string
#define UPLOAD 85       // the upload marker in the request string
#define PROBLEM 69      // the error marker, defined based on "ERROR" but renamed to "PROBLEM" due to ERROR already being defined
#define EXIT 65     // the exit request marker

// Size limits - these should all be much bigger for a real application
#define MAXRECEIVE  100    // maximum number of bytes to receive at a time
#define MAXREQUEST  100    // size of request array, in bytes
#define MAXRESPONSE 200    // size of response array (at least 46 bytes more)
#define MAXFILENAME 250    //size of max file name

// FIle handling definiitons
#define BLOCKSIZE 100    // number of bytes to read at a time
#define MAXPATHNAME 100  // maximum length of path
#define BYTESPERLINE 10 // number of bytes to print on each line
#define MINTEXT 10      // limits for text characters
#define MAXTEXT 127

int main()
{
  // Create variables needed by this function
  /* The server uses 2 sockets - one to listen for connection requests,
     the other to make a connection with the client. */
  SOCKET listenSocket = INVALID_SOCKET;  // identifier for listening socket
  SOCKET connectSocket = INVALID_SOCKET; // identifier for connection socket
  //int retVal;         // return value from various functions
  int numRx;          // number of bytes received this time
  int totalReq;       // number of bytes received in this request

  int numRecv;        // number of times recv() has got data on this connection
  int totalResp;      // total number of bytes sent as response
  enum stopValues {CONTIN, CLOSE, FINAL}; // values for stop variable
  int stop = CONTIN;       // variable to control the loops
  int i;              // used in for loop
  char download = 'Y';       // set to Y if can fulfil download request or N if can't
  char request[MAXREQUEST+1];   // array to hold request from client
  char response[MAXRESPONSE+1]; // array to hold our response
  char welcome[] = "\nWelcome to the Communication Systems server. \r\n";
  char goodbye[] = "\nGoodbye and thank you for using the server.    \n";
  char success[] = "\nSuccessfully uploaded the file                 \n";
  char fail[] =    "\nSadly failed the file upload                   \n";

  //Ebuka's Variables
  int ReqType;        // type of request received (Download or Upload?)
  long filesize;       // size of file to be exchanged
  char filename[MAXFILENAME+1];     //array to hold filename
  char * char_ptr;   //pointer to point to various sections in request

  //File handling variables
  char data[BLOCKSIZE+2];     // array to hold file data
  /* string with path to files, with enough space to add the file name.
     Note double backslash in a string represents one backslash.
     The backslash is used to separate items in a path in Windows.
     For Linux, use a forward slash. */
  char pathWrite[MAXFILENAME+MAXPATHNAME] = "fileStore\\";  // Windows
  char pathRead[MAXFILENAME+MAXPATHNAME]; // string for read path
  FILE *fp;          // file handle for file
  int retVal, rv2;   // return codes from functions
  long totalBytes = 0;    // total bytes read from file
  size_t len;        // length of a string

  // Print starting message
  printf("\nCommunication Systems server program\n\n");


  // ============== SERVER SETUP ===========================================

  listenSocket = TCPcreateSocket();  // initialise and create a socket
  if (listenSocket == FAILURE)  // check for problem
    return 1;       // no point in continuing

  // Set up the server to use this socket and the specified port
  retVal = TCPserverSetup(listenSocket, SERVER_PORT);
  if (retVal == FAILURE) // check for problem
    stop = FINAL;   // set the flag to prevent other things happening


  // ============== LOOP TO HANDLE CONNECTIONS =============================

  while (stop != FINAL)
  {
    stop = CONTIN;  // initialise the stop signal for this connection
    numRecv = 0;    // initialise the receive counter
    totalReq = 0;   // initialise number of bytes in the request
    totalResp = 0;  // initialise the response byte counter
    totalBytes = 0; // initialise the totalBytes counter


    // ============== WAIT FOR CONNECTION ====================================

    // if this is a repeat connection attempt with the client, then just skip this part
    if(response[0] != 'R' || response[1] != 'Y')connectSocket = TCPserverConnect(listenSocket); // Listen for a client to try to connect, and accept the connection
    if (connectSocket == FAILURE)  // check for problem
      stop = CLOSE;   // prevent other things happening
    else
    {
      if(response[0] != 'R' || response[1] != 'Y')
      {
        // First send the welcome message
        retVal = send(connectSocket, welcome, strlen(welcome), 0);
        // retVal will be the number of bytes sent, or a problem indicator

        if( retVal == SOCKET_ERROR)  // check for problem
        {
          printf("\n*** Problem sending welcome message\n");
          printProblem();
          stop = FINAL;   // set the flag to end the outer loop
          continue;       // skip the rest of the response
        }
        else
        {
          printf("\nSent welcome message of %d bytes\n", retVal);
          totalResp += retVal;
        }
      }
    }


    // ============== RECEIVE REQUEST ==================================

    // Loop to receive data from the client, until the request is complete
    while (stop == CONTIN)   // loop is controlled by the stop variable
    {
      printf("Waiting for data from the client...\n");

      /* Wait to receive bytes from the client, using the recv() function
         recv() arguments: socket identifier, pointer to array for received bytes,
         maximum number of bytes to receive, last argument 0.  Normally,
         this function will not return until it receives at least one byte. */

      //  First, receive the request from the client
      numRx = recv(connectSocket, request, MAXRECEIVE, 0);
      // numRx will be number of bytes received, or a problem indicator

      if( numRx < 0)  // check for problem
      {
        printf("\nProblem receiving\n");
        printProblem();   // give details of the problem
        stop = CLOSE;     // this will end the inner loop
      }
      else if (numRx == 0)  // indicates connection closing (normal close)
      {
        printf("\nConnection closed by client\n");
        stop = CLOSE;    // this will end the inner loop
      }
      else    // numRx > 0 => we got some data from the client
      {
        numRecv++;   // increment the receive counter
        printf("\nReceived %d bytes from the client", numRx);

        /* Check if the client has completed the request.
           The end marker is an opening curly bracket.*/
           
        for (i=totalReq; i<totalReq+numRx; i++) // for each new received byte
        {
          if (request[i] == '{') // if end marker found
          {
            stop = CLOSE;   // end of request found, can stop receiving
            printf("\nFound the end marker after %d bytes\n", i+1);
          }
        } // end of for loop

        totalReq += numRx;  // update the number of bytes in the request

        /* If the end marker has not been found yet, check if there is
           enough room in the request array to try to receive more data.
           This will often stop before the array is full. */
        if (stop == CONTIN && totalReq+MAXRECEIVE > MAXREQUEST)
        {
          stop = CLOSE;   // stop receiving, array is close to full
          printf("No end marker, approaching size limit, %d bytes received\n", totalReq);
        }
      }  // end of else - finished processing the bytes received this time
      
    }  // end of inner while loop, receiving request
    request[totalReq] = 0;  // add 0 byte to make request into a string


    printf("\nReceived total of %d bytes: %s\n", totalReq, request);

    // ============== PARSE REQUEST ==================================

    // Define type of request received
    switch (request[0]){
        case UPLOAD:
            printf("\n****************************\nReceived upload request!\n****************************\n");
            ReqType = UPLOAD;
            break;

        case DOWNLOAD:
            printf("\n****************************\nReceived download request!\n****************************\n");
            ReqType = DOWNLOAD;
            break;

        case PROBLEM:
            printf("\n****************************\nReceived error\n****************************\n");
            ReqType = PROBLEM;
            break;
            
        case EXIT:
            printf("\n****************************\nReceived exit request\n****************************\n");
            ReqType = EXIT;
            break;

        default:
            printf("\n****************************\nERROR: Received neither download nor upload request!\n****************************\n");

    }

    //printf("DEBUG REQUEST INT:%d", ReqType);

    switch (ReqType){
        case(UPLOAD):
            filesize = atoi(strtok(request,"U#{"));

            char_ptr = strtok(NULL,"U#{");
            strcpy(filename, char_ptr);

            printf("\nDEBUG FILENAME - %s, FILESIZE - %ld", filename, filesize);
            break;

        case(DOWNLOAD):
            char_ptr = strtok(request,"D\\0");
            strcpy(filename, char_ptr);

            printf("\nDEBUG FILENAME - %s", filename);
            break;

    }

    // ============== EXECUTE REQUEST ==================================

    switch (ReqType){
//////======================================UPLOAD_HANDELING======================================================================================//////
        case(UPLOAD):
            printf("\nReceived upload request");

            //================ Get name of file ==================
            len = strcspn(filename, "\n"); // find actual length of filename
            filename[len] = 0;     // mark this as end of string

          //================ Build the file path ===========================
             strncat(pathWrite, "Z ", 1);  // add one character to end of path
             strncat(pathWrite, filename, MAXFILENAME);  // add filename to this

          //================ Open the file for writing, as bytes ===========
            printf("\nOpening %s\n", pathWrite);
            fp = fopen(pathWrite, "wb");  // open for binary write
            // this method will over-write any existing file of that name

            if (fp == NULL) // check for problem
            {
              perror("Problem opening file");
              printf("errno = %d\n", errno);
            // if an error has occurred, then build the negative response frame
              response[0] = 'N';
              strcat(response, "Problem opening file             "); // add negative response reason to array
              stop = CLOSE; // don't allow for any receiving
            }
            else // if there are no issues, send a positive response
            {
                response[0] = 'Y';
                response[1] = 0;
                stop = CONTIN;
            }
            retVal = send(connectSocket, response, strlen(response), 0); // send response built based on situation
            printf("\nSent response: %s\n", response);

            if (stop == CLOSE) break;   // if stop flag was triggered, then exit the case

        printf("\nNumber of bytes to receive: %ld", filesize);

        while(totalBytes < filesize)  // don't exit the loop until we received the number of bytes the file should contain
        {
          // ================ Write bytes to the open file ================

          //receive block of data
          numRx = recv(connectSocket, data, MAXRECEIVE, 0);

          // Write block of data
          rv2 = (int) fwrite(data, 1, numRx, fp);  // write bytes

          if (ferror(fp))  // check for problem
          {
            perror("Problem writing output file");
            printf("errno = %d\n", errno);
            stop = CLOSE; // if a problem occurs, set stop to CLOSE
          }
          printf("\n%d bytes written to file\n", rv2);

          totalBytes += numRx;  // update filesize condition, totalBytes, for the while loop
          printf("\nTotal bytes received: %ld", totalBytes);
          if (totalBytes >= filesize)
          {
            if(ferror(fp)) // if a problem occurred during transmission
            {
              retVal = send(connectSocket, fail, strlen(fail), 0); //send a message saying the upload failed
              fclose(fp); // close the file with the file pointer
              break;
            }
            retVal = send(connectSocket, success, strlen(success), 0); // send a message saying the file was fully and successfully received to the client
            fclose(fp); // close the file with the file pointer
            break;
          }
        }
        break;

//////======================================DOWNLOAD_HANDELING===============================================================================//////
        case(DOWNLOAD):
          download = 'Y';
    // First test to see if the file is available for download, if it isn't available, then build the response to be negative with a reason why//

          len = strcspn(filename, "\n"); // find actual length of filename
          filename[len] = 0;     // mark this as end of string

          //================ Build the file path ===========================
            strncpy(pathRead, pathWrite, MAXPATHNAME);  // copy the path string
          // this limits number of characters copied to MAXPATHNAME

            strncat(pathRead, filename, MAXFILENAME);  // add file's name from request to end of path
          // for safety, strncat() limits the number of characters added

            //printf("File directory to open |%s|", pathRead);

          //================ Open the file for reading, as bytes ===========

          fp = fopen(pathRead, "rb");  // open for binary read
          if (fp == NULL)     // check for problem
          {
            perror("Problem opening file");  // perror() gives details
            printf("errno = %d\n", errno); // errno allows program to react
            strcpy(request, "Problem Opening File\n                ");
            download = 'N';
          }

    // if every other check has been passed, allowing for the download to occur, find and set the file size for the response//
          if(download == 'Y')
          {
          // ================ Find the size of the open file ================

          // First move to the end of the file
            retVal = fseek(fp, 0, SEEK_END);  // set position to end of file
              if (retVal != 0 && fp != NULL)  // if there was a problem in fseek
              {
                perror("Problem in fseek");
                printf("errno = %d\n", errno);
                fclose (fp);
                strcpy(request, "Problem in finding filesize\n");
                download = 'N';
              }
            // Then find the position, measured in bytes from start of file and set as file size
            filesize = ftell(fp);
            itoa(filesize, request, 10);  // assign file size information as a string to send in request
          }

          // build the response to the download request
          response[0] = download; // Set the first byte as the download response
          response[1] = 0;        // Set the next byte as the end of the string to allow the addition of the reponse info
          strcat(response, request);  // Set the remainder of the response as additional information based on the response
          response[strlen(response)] = 0;   // Set the end of string marker to ensure the sent information is just the response
          printf("\nResponse to send: %s\n", response);

          retVal = send(connectSocket, response, strlen(response), 0); // send the built response to the download request


          retVal = fseek(fp, 0, SEEK_SET);   // set position to start of file to read through the bytes
          if (retVal != 0)  // if there was a problem
          {
            perror("Problem in fseek");
            printf("errno = %d\n", errno);
            fclose (fp);
            stop = CLOSE;
            break;
          }

          while(!feof(fp)) // loop to send file data until end of file reached
          {
            if (download == 'N' || totalBytes == filesize) break;

            retVal = (int) fread(data, 1, BLOCKSIZE, fp);  // read bytes
            if (ferror(fp))  // check for problem
            {
              perror("Problem reading input file");
              printf("errno = %d\n", errno);
              fclose(fp);
              stop = CLOSE;
              break;
            }
            else
            {
              totalBytes += retVal;   // add to byte counter
              retVal = send(connectSocket, data, retVal, 0);
              printf("\nbytes sent: %d\ntotal bytes: %ld\n", retVal, totalBytes);
            }
          }
          break;

    }

// if there has been a successful attempt at either an upload or download, where data bytes have been sent, then do what's below
      if(ReqType != PROBLEM && (response[0] == 'Y' || download == 'Y')) 
      {
          
          //receive the message which states the file has been transferred
          numRx = recv(connectSocket, request, 8, 0);
          printf("\nMessage from client: %s", request);

          if (request[0] == 'T' && request[6] == '!') // if received "Thanks!" message from server, then reset some information
          {
            //reset the arrays which were initialised at the start
            memset(response, 0, sizeof(response));
            memset(filename, 0, sizeof(filename));
            memset(pathRead, 0, sizeof(pathRead));
            memset(pathWrite, 0, sizeof(pathWrite));

            strcpy(pathWrite, "fileStore\\");

            // Print some information at the server
            printf("\nFinished processing request from client\n");
            printf("\nSent a total of %d bytes to the client\n", totalResp);
          }
          else  // if the "Thanks!" message isn't received, then just shut down the connection.
          {
            stop = CLOSE;
            printf("\nThanks not received, closing connection ");
            TCPcloseSocket(connectSocket);
          }
        }

    if(ReqType == EXIT) // if the client chose to exit earlier on
    {
      printf("\nReceived EXIT request\nServer closing connection...\n");
      stop = CLOSE; // set the loop flag to go back to the start
      TCPcloseSocket(connectSocket);    // close the connection
    }
    else
    {
      // wait to see if client wishes to continue communicating
      printf("\nWaiting for continue/terminate choice\n");
      numRx = recv(connectSocket, response, MAXRESPONSE, 0);
      printf("\ncontinue/terminate response: |%c%c|\n", response[0], response[1]);
    
     // ============== CLOSE THE CONNECTION ======================================
     
      if(response[1] == 'N')    // if client doesn't want to continue communicating with the server:
      {
        stop = CLOSE;   // set the flag to loop back to the start
        retVal = send(connectSocket, goodbye, strlen(goodbye), 0);  // send the goodbye message
        if( retVal == SOCKET_ERROR)  // check for problem
        {
          printf("\n*** Problem sending closing message\n");
          stop = FINAL;   // set the flag to end the loop
        }
        else
        {
          printf("\nSent closing message of %d bytes\n", retVal);
          totalResp += retVal;
        }
    
        printf("\nServer is closing the connection...\n");
        // Close the connection socket
        TCPcloseSocket(connectSocket);
        stop = CLOSE;
      }
    }

  } // end of outer while(stop != FINAL) loop - go back to wait for another connection


  // ================== TIDY UP AND SHUT DOWN THE SERVER ==========================

  // If we get to this point, something has gone wrong

  //Close the listening socket
  printf("\nPress Enter to continue");
  getchar();
  TCPcloseSocket(listenSocket);


  return SUCCESS;
}
