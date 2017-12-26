#include <stdio.h>

int main(int argc, char *argv[]){
  const char* str;
  str = "00121231";
  
  if(argc > 1){
    goto two_words;
  }else{
    goto one_word;
  }
  
 two_words:
  printf("Hello this is line two %ld\n", sizeof(str));
 one_word:
  printf("Hello this line one\n");
}
