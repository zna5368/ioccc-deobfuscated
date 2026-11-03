/*
 * FIRST: a tiny FORTH-like virtual machine capable of bootstrapping THIRD.
 *
 * Main memory layout (all addresses are indices into memory[]):
 *   memory[0]   dictionary pointer ("here"): next free cell to compile into
 *   memory[1]   return stack pointer
 *   memory[2]   always 0 (the PUSH_LITERAL opcode); compiled numbers are a
 *               reference to address 2 followed by the number itself
 *   memory[3..31] free scratch cells (THIRD uses some as variables)
 *   memory[32..] the dictionary, followed by a 512 cell return stack and
 *               then everything THIRD defines
 *
 * A dictionary entry for a word at address w looks like:
 *   memory[w]     link to the previous word (1 terminates the list)
 *   memory[w + 1] offset of the word's name inside names[]
 *   memory[w + 2] code field: the opcode run when the word is read
 *   memory[w + 3] the word's body
 *
 * Compiled code is a list of addresses; running an address runs the opcode
 * stored there with the following address as its argument.
 */

/* opcodes */
#define OP_PUSH_LITERAL 0 /* internal: push the next cell of code         */
#define OP_COMPILE 1      /* internal: compile a reference to the body    */
#define OP_CALL 2         /* internal: run the body as threaded code      */
#define OP_DEFINE 3       /* :         start a new word                   */
#define OP_IMMEDIATE 4    /* immediate make the new word run when read    */
#define OP_READ 5         /* _read     read a word and compile/run it     */
#define OP_FETCH 6        /* @                                            */
#define OP_STORE 7        /* !                                            */
#define OP_SUBTRACT 8     /* -                                            */
#define OP_MULTIPLY 9     /* *                                            */
#define OP_DIVIDE 10      /* /                                            */
#define OP_LESS_ZERO 11   /* <0                                           */
#define OP_EXIT 12        /* exit      return from the current word       */
#define OP_ECHO 13        /* echo      print a character                  */
#define OP_KEY 14         /* key       read a character                   */
#define OP_PICK 15        /* _pick     copy an item from deep in stack    */

#define TOKEN_MAX 64        /* names[0..63] holds the token just read */
#define RETURN_STACK_SIZE 512
#define LITERAL_ADDRESS 2   /* memory[2] holds OP_PUSH_LITERAL */
#define END_OF_DICTIONARY 1 /* link that ends the dictionary search */

#define here memory[0]
#define return_sp memory[1]

/* compile one cell onto the end of the dictionary */
#define compile(value) memory[here++] = (value)

char names[5000];
int memory[20000] = {32};
int latest = END_OF_DICTIONARY; /* most recently defined word */
int ip;                         /* instruction pointer */
int data_stack[500];
int *sp = data_stack; /* data stack, holds everything below the top */
int name_end = TOKEN_MAX;
int word;
int top; /* cached top of the data stack */

/* read a name and add a new dictionary header with the given code field */
create_word(code) {
  compile(latest);
  latest = here - 1;
  compile(name_end);
  compile(code);
  scanf("%s", names + name_end);
  name_end += strlen(names + name_end) + 1;
}

/* run the opcode stored at address, giving it the following address */
execute(address) {
  int opcode = memory[address++];

  switch (opcode) {
  case OP_READ:
    if (scanf("%s", names) < 1)
      exit(0);
    /* the end marker's name offset is memory[2] == 0, which is the token
       itself, so the search always stops there if the word is unknown */
    for (word = latest; strcmp(names, &names[memory[word + 1]]) != 0;
         word = memory[word])
      ;
    if (word != END_OF_DICTIONARY) {
      execute(word + 2);
    } else {
      compile(LITERAL_ADDRESS);
      compile(atoi(names));
    }
    break;
  case OP_EXIT:
    ip = memory[return_sp--];
    break;
  case OP_PICK:
    top = sp[-top];
    break;
  case OP_COMPILE:
    compile(address);
    break;
  case OP_MULTIPLY:
    top *= *sp--;
    break;
  case OP_STORE:
    memory[top] = *sp--;
    top = *sp--;
    break;
  case OP_PUSH_LITERAL:
    *++sp = top;
    top = memory[ip++];
    break;
  case OP_SUBTRACT:
    top = *sp-- - top;
    break;
  case OP_CALL:
    memory[++return_sp] = ip;
    ip = address;
    break;
  case OP_LESS_ZERO:
    top = top < 0;
    break;
  case OP_IMMEDIATE:
    /* rewind over the code field and change it to OP_CALL */
    here -= 2;
    compile(OP_CALL);
    break;
  case OP_FETCH:
    top = memory[top];
    break;
  case OP_DIVIDE:
    top = *sp-- / top;
    break;
  case OP_DEFINE:
    create_word(OP_COMPILE);
    compile(OP_CALL);
    break;
  case OP_ECHO:
    putchar(top);
    top = *sp--;
    break;
  case OP_KEY:
    *++sp = top;
    top = getchar();
  }
}

main() {
  int read_code;
  int opcode;

  /* the first three names typed in are ":", "immediate" and "_read" */
  create_word(OP_DEFINE);
  create_word(OP_IMMEDIATE);
  create_word(OP_COMPILE);
  read_code = here;
  compile(OP_READ);
  compile(OP_CALL);

  /* the main loop: read a word, then call this loop again */
  ip = here;
  compile(read_code);
  compile(ip - 1);

  /* the remaining primitives, named by the input */
  for (opcode = OP_FETCH; opcode <= OP_PICK; opcode++) {
    create_word(OP_COMPILE);
    compile(opcode);
  }

  /* reserve the return stack after the built in words */
  return_sp = here;
  here += RETURN_STACK_SIZE;

  for (;;)
    execute(memory[ip++]);
}
