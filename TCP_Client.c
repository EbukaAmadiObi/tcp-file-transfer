/*  EEEN20060 Communication Systems Group 42 Option 2
    TCP_Client program, to communicate with our TCP_Server program.

    This program asks for details of the server, and tries to connect. 
    It will then ask for details for a request to send to the server. 
    This program will allow you to either upload to or download from 
      the server. 
    Once an attempt has been made to communicate, you will be given
      the option to try upload or download again, or exit the
      connection.
    You will have the option to either connect to another server or
      exit the program. 
*/

#include <stdio.h>		 // for input and output functions
#include <string.h>	     // for string handling functions
#include <math.h>        // for mathematical operations

#include <errno.h>       // contains errno = error number
#include <string.h>      // needed for strncat, strcpy

#include <winsock2.h>	 // Windows socket functions and structs
#include <ws2tcpip.h>	 // Windows socket functions and structs

// This header file must be included AFTER the libraries above
#include "CS_TCP.h"		 // TCP functions for this assignment

// Marker used in this example protocol
#define ENDMARK 10       // the newline character

// Size limits
#define MAXREQUEST 100    // maximum size of request, in bytes
#define MAXRESPONSE 100   // size of response array, in bytes

// file handling constants
#define BLOCKSIZE 100    // number of bytes to read at a time
#define MAXFILENAME 100  // maximum length of file name
#define MAXPATHNAME 100  // maximum length of path

int main()
{
  // Create variables needed by this function
  SOCKET clientSocket = INVALID_SOCKET;  // identifier for the client socket
  int retVal = 0;                       // return codes from functions
  int numRx;                  // number of bytes received this time
  int reqLen;                 // request string length
  int stop = 0;               // flag to control the loop
  char serverIPstr[20];       // IP address of server as a string
  int serverPort;             // server port number
  char request[MAXREQUEST+1];   // array to hold request from user
  char frametx[2*MAXREQUEST+3];   // array to hold the frame to send
  char response[MAXRESPONSE+1]; // array to hold response from server
  char up_down;               // string used for choosing upload or dowload
  int number;                 // number variable to find size of numbers using NUMDIGITS
  int repeat = 1;             // flag to allow client to have multiple interactions with server
  char answer;                // variable to hold a response from the user for server interaction

  // File handling variables

  /* string with path to files, with enough space to add the file name.
     Note double backslash in a string represents one backslash.
     The backslash is used to separate items in a path in Windows. */
  char pathWrite[MAXFILENAME+MAXPATHNAME] = "fileStore\\";  // Windows
  char pathRead[MAXFILENAME+MAXPATHNAME]; // string for read path
  FILE *fp;          // file handle for file
  size_t len;        // length of a string
  char data[BLOCKSIZE+2];      // array to hold data to send
  int filesize = 0; //variable to hold size of file
  int i;  //for use in for loop
  int totalBytes;


  // Print starting message
  printf("\nCommunication Systems client program\n\n");

  while (repeat == 1)
  {

    // reset variables if set in a previous communication attempt
    stop = 0;
    serverPort = 0;
    memset(serverIPstr, 0, 20);


  // ============== CONNECT TO SERVER =============================================

  clientSocket = TCPcreateSocket();  // initialise and create a socket
  if (clientSocket == FAILURE)  // check for failure
    stop = 1;       // return to start and try again
  else
  {
    // Get the details of the server from the user
    printf("\nEnter the IP address of the server (e.g. 137.43.168.123): ");
    scanf("%20s", serverIPstr);  // get IP address as a string

    printf("\nEnter the port number on which the server is listening, or enter E to exit: ");
    scanf("%d", &serverPort);     // get port number as an integer
    fgets(request, MAXREQUEST, stdin);  // clear the input buffer
  }

  if(serverPort == 0) // If user entered "E" or anything which isn't a number, then close the socket and return to the beginning
  {
    repeat = 0;
    stop = 1;
    TCPcloseSocket(clientSocket);
    printf("\nClient is closing the connection...\n");
    printf("\nExiting client programme\n");
  }
  else
  {
    // Now connect to the server
    retVal = TCPclientConnect(clientSocket, serverIPstr, serverPort);
    if (retVal < 0)  // failed to connect
    {
      TCPcloseSocket(clientSocket);
      stop = 1;    // set the flag to skip directly to tidy-up section
    }

      //receive the welcome message sent from the server after connection is successfully established then print it, if the stop flag hasn't been triggered
        if(stop == 0)   
        {
        numRx = recv(clientSocket, response, MAXRESPONSE, 0);
        printf("\n\n%s\n", response);
        }


      // start of loop, exiting loop means closing the connection to the server
      while(stop == 0)
      {
        // initialise the totalBytes counter for later loops
        totalBytes = 0;

    // ============== SEND REQUEST ======================================

        if (stop == 0)      // if we are connected
        {
            
          /* Ask user if they wish to upload or download, if they give a response which isn't U or D,
             then request again. If they enter E, then exit the programme and skip to the end*/
          while(stop == 0)
          {
              printf("\nChoose Upload or Download [U/D]: ");
              scanf("%c", &up_down);
              fgets(request, MAXREQUEST, stdin);  // clear the input buffer

              if(up_down == 'E'|| up_down =='e') stop = 1;   //if user wishes to exit, break the loop
              else if(up_down != 'U' && up_down != 'D') printf("\nInvalid response [Enter E if you wish to Exit]");
              else break;
          }
          // if the user wants to exit, then send a message to the server to close down the connection
          if(stop == 1)
          {
            response[0] = 'A';
            response[1] = '{';
            response[2] = 0;
            retVal = send(clientSocket, response, MAXRESPONSE, 0);
            printf("Exiting client\n");
          }

          // Get user request and send it to the server
          if(stop == 0)
          {
             printf("\nEnter request (maximum %d characters): ", MAXREQUEST-2);
             fgets(request, MAXREQUEST, stdin);
             // read in request string

             len = strcspn(request, "\n"); // find actual length of filename
             request[len] = 0;     // mark this as end of string
          }

          frametx[0] = up_down; // set the first array space as the type of request
          
/////// =========================== DOWNLOAD REQUEST PREPARATION =========================== ///////
          if(up_down == 'D')
          {
            //================ Build the file path ===========================
               strncat(pathWrite, "Z ", 1);  // add one character to end of path
               strncat(pathWrite, request, MAXFILENAME);  // add fName to this

            //================ Open the file for writing, as bytes ===========
              printf("\nOpening %s for binary write\n", pathWrite);
              fp = fopen(pathWrite, "wb");  // open for binary write
              // this method will over-write any existing file of that name

              if (fp == NULL) // check for problem
              {
                perror("Problem opening file");
                printf("errno = %d\n", errno);
                stop = 1;
              }
              else
              {
                reqLen = strlen(request);  // find the length of the request

                for(int i = 0; i < reqLen; i++) // assign request to frame to send
                {
                    frametx[1+i] = request[i];
                }
                /* Put two end markers at the end of the frame to be sent.  The array
                was made large enough to allow an extra character to be added.  */
                frametx[reqLen + 1] = ENDMARK;  // end marker in last position
                reqLen++; // the string is now one byte longer
                frametx[reqLen + 1] = 0;   // add a new end of string marker
                printf("request2: |%s|\n", frametx);
              }
          }
/////// =========================== UPLOAD REQUEST PREPARATION ===========================///////
          else if(up_down == 'U')
          {
            //================ Build the file path ===========================
              strncpy(pathRead, pathWrite, MAXPATHNAME);  // copy the path string
            // this limits number of characters copied to MAXPATHNAME

              strncat(pathRead, request, MAXFILENAME);  // add file's name from request to end of path
            // for safety, strncat() limits the number of characters added

            //================ Open the file for reading, as bytes ===========

            fp = fopen(pathRead, "rb");  // open for binary read
            if (fp == NULL)     // check for problem
            {
              perror("Problem opening file");  // perror() gives details
              printf("errno = %d\n", errno); // errno allows program to react
              stop = 1;
            }
            else
            {
            // ================ Find the size of the open file ================

              // First move to the end of the file
              retVal = fseek(fp, 0, SEEK_END);  // set position to end of file
              if (retVal != 0)  // if there was a problem
              {
                perror("Problem in fseek");
                printf("errno = %d\n", errno);
                fclose (fp);
                stop = 1;
              }
              else
              {
                // Then find the position, measured in bytes from start of file
                filesize = ftell(fp);         // find current position

                len = strcspn(request, "\n"); // find actual length of filename
                request[len] = 0;     // mark this as end of string
                number = floor(log10(filesize)) + 1;    //find the number of digits in the filesize number
                itoa(filesize, frametx, 10);   //Enter file size information into array as individual digits

                for(i = number ; i >= 0; i --) //Since U was overwritten through itoa, move the numerical information up 1 array space
                {
                    frametx[i+1] = frametx[i];
                }
                frametx[0] = up_down;      //Insert U back into the first array spot
                frametx[1 + number] = '#';     //Enter end of size information marker
                strcat(frametx, request);   //Add filename to frame
                strcat(frametx, "{");   //Add end of initial request marker
              }
            }
          }

        // If an error was detected, then build the frame to signify that an error occurred
          if(stop == 1)
          {
            frametx[0] = 'E';
            frametx[1] = '{';
            frametx[2] = 0;
          }
          printf("\nSending Request: %s\n", frametx);
          
        // Now send the request to the server over the TCP connection. 
          
          retVal = send(clientSocket, frametx, strlen(frametx), 0);  // send bytes
          // retVal will be the number of bytes sent, or a problem indicator

          if(retVal == SOCKET_ERROR) // check for problem
          {
            printf("*** Problem sending\n");
            stop = 1;       // set the flag to skip to tidy-up section
          }
          else if(retVal <= 0)
          {
            printf("*** Problem sending\n");
            stop = 1;       // set the flag to skip to tidy-up section
          }
          else printf("Sent request with %d bytes, waiting for reply...\n", retVal);


      //wait for a response to the intial Upload/Download request if no error has been detected
          if (stop == 0) // check if any errors occurred before hand
          {
            numRx = recv(clientSocket, response, MAXRESPONSE, 0); // receive response
            printf("\nresponse received: %s\n", response);
          }
//////====================================== UPLOADING FILES ===============================================================================//////
          if(up_down == 'U')
          {
            if(response[0] == 'Y')  // if the server allows the upload
            {
              retVal = fseek(fp, 0, SEEK_SET);   // set position to start of file
                if (retVal != 0)  // if there was a problem
                {
                  perror("Problem in fseek");
                  printf("errno = %d\n", errno);
                  fclose (fp);
                  stop = 1;
                }
                else
                {
                  while (!feof(fp))   // continue until end of file has been reached
                  {
                    retVal = (int) fread(data, 1, BLOCKSIZE, fp);  // read bytes
                    if (ferror(fp))  // check for problem
                    {
                      perror("Problem reading input file");
                      printf("errno = %d\n", errno);
                      fclose(fp);
                      stop = 1;
                      break;
                    }
                    else
                    {
                      totalBytes += retVal;   // add to byte counter

                    //Send the block of data that was read
                      retVal = send(clientSocket, data, retVal, 0);
                      
                      if (retVal <= 0)  // if there was a problem in sending the file data
                      {
                        perror("Problem in sending file data");
                        printf("errno = %d\n", errno);
                        fclose (fp);
                        stop = 1;
                        break;
                      }
                      printf("\nbytes sent: %d\ntotal bytes: %d\n", retVal, totalBytes);
                    }
                  }
                }

              // wait for a response to the data upload assuming the flag hasn't been triggered
              if(stop == 0) // check to see if the stop flag was triggered
              {
                numRx = recv(clientSocket, response, MAXRESPONSE, 0);   // receive upload response
                response[strlen(response)] = 0; //set new end of string marker at the end of the response received
                
                retVal = send(clientSocket, "Thanks!", 8, 0);   // send response to mark the end of the interaction
                stop = 1; // set flag since data has been transferred
              }
            }
            else if(response[0] == 'N') // if the server rejects the upload:
            {
              printf("Server did not allow Upload\n"); // print message
              stop = 1;
            }
            else
            {
              if(stop == 0) // if nothing is wrong
              {
                printf("Incorrect Server upload response, received |%s|\n", response);   // print message and response received for debugging
                stop = 1;
              }
            }
          } // end of upload response


//////====================================== RECEIVE DOWNLOAD RESPONSE ===============================================================================//////
          if (up_down == 'D')
          {
            if(response[0] == 'N') // if the server rejects the request, print the reason why, then skip to repeating the request
            {
              printf("Failed to download from Server\n");
              for(i = 1; i < strlen(response) - 1; i++) printf("%c", response[i]);
              stop = 1;
            }
            else if(response[0] == 'Y')  // if the server accepts the request, extract the file size information
            {
              filesize = atoi(strtok(response,"Y"));
              printf("filesize received: %d", filesize);
            }
            else    
            {
              printf("Invalid download response received: |%s|\n", response");
              stop = 1;
            }

            // ================ Write bytes to the open file ================
            while(stop == 0 && totalBytes < filesize)   // while no errors occur and the entire file hasn't been received
            {
              //receive block of data
              numRx = recv(clientSocket, data, MAXRESPONSE, 0);
              
              if(numRX < 0) // if an error occurs, stop the loop and close the file
              {
                printf("Error receiving bytes");
                fclose(fp);
                stop = 1;
                break;
              }

              // Write block of data
              retVal = (int) fwrite(data, 1, numRx, fp);  // write bytes
              
              if(retVal < 0) // if an error occurs, stop the loop and close the file
              {
                printf("Error writing bytes to file");
                fclose(fp);
                stop = 1;
                break;
              }

              if (ferror(fp))  // check for problem
              {
                perror("Problem writing output file"); // if an error occurs, stop the loop and close the file
                printf("errno = %d\n", errno);
                fclose(fp);
                stop = 1;
                break;
              }
              printf("\n%d bytes written to file\n", retVal);

              totalBytes += numRx;  // update totalBytes, for the while loop
              printf("Total bytes received: %d\n", totalBytes);
              
              if (totalBytes >= filesize)   //check if the number of bytes has been fully received
              {
                strcpy(response, "Thanks!");    // set response to have a response message
                retVal = send(clientSocket, response, strlen(response), 0); // send the message to say the file has been received
                printf("\nSuccessfully downloaded the file\n"); // print a message for the user saying the file was fully and successfully downloaded
                fclose(fp); // close the file with the file pointer
                stop = 1;
                break;
              }

            } // end of while loop receiving and writing download file

          }  // end of download response

          //reset the arrays which were initialised at the start
          memset(pathRead, 0, sizeof(pathRead));
          memset(pathWrite, 0, sizeof(pathWrite));
          memset(frametx, 0, sizeof(frametx));

          strcpy(pathWrite, "fileStore\\");
        }

//////====================================== GET REPEAT REQUEST RESPONSE ===============================================================================//////
        // reset 'answer' to allow entry into next while loop
        answer = 'P';

        // ask user if they wish to do more with the server, don't exit while loop until a valid answer has been given
        printf("Would you like to do more with this server? [Y/N]: ");
        while(answer != 'Y' && answer != 'N')
        {
          scanf("%c", &answer);
          fgets(request, MAXREQUEST, stdin);  // clear the input buffer
          if(answer != 'Y' && answer != 'N')
          {
            printf("Please enter either [Y/N]: ");
          }

          if(answer == 'Y') stop = 0;   // if user wishes to continue, then set the flag to let the outer while loop repeat again
        }

      // build and send a response to the server for further action to be taken
      response[0] = 'R';    // response type, 'R' for repeat, used for server to ensure it doesn't try to reconnect to the client it's already connected to
      response[1] = answer; // answer to either client continuing or stopping
      response[2] = 0;       // end of string marker
      retVal = send(clientSocket, response, strlen(response), 0);    // send the repeat request response

    } // end of outer while (stop == 0) loop
    
//////====================================== TIDY UP AND END ===============================================================================//////

      // receive the goodbye message from the server
      numRx = recv(clientSocket, response, MAXRESPONSE, 0);
      printf("\n%s", response);

    // ================ Close the open file ================
    fclose (fp);        // close whatever file has been opened
    printf("\nOutput file closed\n");

    } //end of "else" when checking server port number

  } //end of while (repeat == 1) loop, if repeat == 0, then the client will close


  printf("\n\nPress Enter to end");
  getchar();
  return 0;
}
