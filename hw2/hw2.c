/* File: hw2.c
 * Author: William Gao
 * Email: wgao1@umbc.edu
 * Last Edited: 09-16-2017
 * Description: 
 */

/*
 * Question 1: It is necessary to kill and reap child processes to avoid creating orphans. 
 * Describing the consequences of having orphans on your system. 

 * Question 2: Suppose the user creates two children processes, 
 * by selecting the first menu option twice. The first child process will have three 
 * opened file descriptors. Why does the second child have four descriptors instead of three? 
 */

int main(int argc, char *argv[]){
  //if there is only 1 command line argument
  if(argc == 1){ //run in parent mode
    //initialize global counter to zero

    //create an array of 100 structures
    
  }else{ //otherwise run in child mode

  }
  
  return 0;
}

/* this functin displays the following menu and returns the user's choice
 * repeated until valid user input
 * Show user input "(User enters #.)" in italics
 * Main Menu:
 * 0. Spawn a child
 * 1. Kill a child
 * 2. Kill a random child
 * 3. End program
 */
int showMenu(){

}

/* first menu option
 * create unnamed pipe with pipe()
 * call fork() to create another child process
 *
 * first child's ID will be zero
 * current value of the global counter is the new child's unique ID
 * update global counter by one
 */
void spawnChild(){

}

/* second option
 * list all child processes still alive
 * read from user which child ID to kill
 * send SIGUSR1 to that child process
 * when a child receives a SIGUSR1, terminate itself
 *
 * back in parent, after sending SIGUSR1, reap the child process
 * update the child's entry in the array of structures as terminated
 */
void killChild(){
}

/* third option
 * send SIGUSR2 to all child processes alive
 * once a child receives a SIGUSR2, open /dev/urandom
 *
 * read first 4 bytes into an unsigned int
 * then write unsigned int (raw bytes) to 
 * stdout file descriptor using write()
 * DON'T use printf()
 * 
 * back in parent, read from pipe for each child
 * for the child that wrote the largest unsigned random num,
 * kill that child (send SIGUSR1, then reap)
 * if more than one child had the largest num, arbitrarily
 * pick one to kill
 */
void killRandomChild(){
}

/* fourth option
 * Send SIGUSR1 to all alive childern
 * reap those children
 * quite the program
 */
void endProgram(){

}
