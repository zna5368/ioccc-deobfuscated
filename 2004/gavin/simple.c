/*
 * One source file, three programs:
 *
 *   gavin   Run on Linux, kernel_main() is called with argc >= 1 and writes
 *           the bootable kernel image to stdout.  The image is the
 *           setup_image[] data followed by the machine code of the kernel
 *           functions (kernel_main, draw_windows and kernel_syscall) copied
 *           straight out of the running program.
 *
 *   kernel  The boot loader jumps into the copied kernel_main() with a zero
 *           first argument, so it runs the operating system.
 *
 *   sh, vi  Built with -DB=_start '-Dputchar(a)=' so that application() is
 *           the ELF entry point.  The kernel loads it from the tar file system
 *           and calls it with messages.
 *
 * The OS is 32 bit, so every address is held in an int.  Kernel and
 * application code is copied to a different address than it was linked for,
 * so it may only use local variables, constants and fixed addresses - never
 * global variables or string literals.
 */

int kernel_main(int argc, int argv, int envp);

int main(int argc, char **argv, char **envp) {
  return kernel_main(argc, (int)argv, (int)envp);
}

/* every handler, syscall and port stub uses this calling convention */
typedef int (*handler_fn)(int arg, int message, int data);
#define CALL(address, arg, message, data)                                      \
  ((handler_fn)(address))(arg, message, data)

#define BYTE(address) (*(signed char *)(address))
#define WORD(address) (*(int *)(address))

/* 1024x768 screen with one byte per pixel */
#define SCREEN_WIDTH 1024
#define SCREEN_PIXELS 786432
/* screen offset of a pixel, also used to pack a height * 1024 + width size */
#define AT(row, col) ((row) * SCREEN_WIDTH + (col))
#define WIDTH(size) ((size) & 1023)

/* is screen offset p inside the rectangle at origin with the given size? */
#define IN_RECT(p, origin, size)                                               \
  ((p) >= (origin) && (p) < (origin) + (size) &&                               \
   ((p) - (origin) & 1023) < WIDTH(size))

/* colors of the 256 color palette */
#define BLACK 0
#define WHITE 31
#define BEVEL_DARK 20
#define BEVEL_LIGHT 28
#define WINDOW_GRAY 24
#define TITLE_BLUE 176
#define DESKTOP_COLOR 125

/* Fixed addresses inside the kernel image, which is loaded at 0x90000 with
   the setup part at 0x90200. */
#define SETUP_BASE 0x90200
#define SYSCALL_ADDRESS WORD(SETUP_BASE)    /* set to kernel_syscall's address */
#define INB_STUB (SETUP_BASE - 8)          /* int inb(port) */
#define OUTB_STUB (SETUP_BASE + 8)         /* outb(port, value) */
#define HEAP_POINTER WORD(SETUP_BASE - 16) /* next free memory address */
#define TASK_LIST WORD(SETUP_BASE - 12)    /* topmost window's task */
#define TASK_LIST_NODE (SETUP_BASE - 20)   /* fake task whose next is TASK_LIST */
#define RAMDISK_IMAGE WORD(SETUP_BASE + 24) /* tar file system from the loader */
#define FONT_ADDRESS WORD(SETUP_BASE + 36) /* BIOS 8x16 font */
#define KEYMAP (SETUP_BASE + 131)          /* scan code to character table */
#define KERNEL_CODE (SETUP_BASE + 208)     /* where kernel_main is copied to */
#define FRAMEBUFFER WORD(0x11028) /* VBE PhysBasePtr in the mode information */
#define BACK_BUFFER (1 << 24)     /* screen is drawn here, then copied */

#define port_in(port) CALL(INB_STUB, port, 0, 0)
#define port_out(port, value) CALL(OUTB_STUB, port, value, 0)

/* PS/2 keyboard controller */
#define KBC_DATA 0x60
#define KBC_STATUS 0x64
#define KBC_BUSY 3     /* output buffer full or input buffer full */
#define KBC_MOUSE 0x20 /* the waiting byte came from the mouse */
#define KEY_RELEASE 128

/* read and throw away bytes until the controller is idle */
#define DRAIN_KBC()                                                            \
  while ((status = port_in(KBC_STATUS)) & KBC_BUSY)                            \
  data = port_in(KBC_DATA)

#define KBC_SEND(command, data_byte)                                           \
  port_out(KBC_STATUS, command);                                               \
  DRAIN_KBC();                                                                 \
  port_out(KBC_DATA, data_byte);                                               \
  DRAIN_KBC()

/* draw an 8x16 character of the BIOS font with the top-left corner at dest */
#define DRAW_GLYPH(dest, ch, color)                                            \
  for (row = 0; row < 16; row++)                                               \
    for (col = 0; col < 8; col++)                                              \
      if (BYTE(FONT_ADDRESS + 16 * (ch) + row) & 128 >> col)                   \
  BYTE((dest) + AT(row, col)) = (color)

/* system calls */
#define SYS_ALLOC 1     /* allocate arg bytes, returns the address */
#define SYS_FILE_SIZE 2 /* size of the tar file whose header is at arg */
#define SYS_EXEC 3      /* run the program named by the command line at arg */
#define SYS_FIND 4      /* returns the tar header of the file named arg */

/* messages sent to applications */
#define MSG_INIT 1
#define MSG_RENDER 2 /* data is the buffer to draw into */
#define MSG_KEY 3    /* data is the character typed */

struct task {
  int handler;  /* address of the application's message handler */
  int name;     /* address of the command line that started it */
  int next;     /* the next window below this one */
  int position; /* screen offset of the window's top-left corner */
  int size;     /* window height * 1024 + width */
  /* the remaining fields are for the application */
  int scroll;   /* first line of text shown */
  int cursor;   /* shell input column */
  int text;     /* 64 column text buffer */
};
#define TASK(address) ((struct task *)(address))
#define TASK_ALLOC_SIZE 164
#define TASK_NAME_OFFSET 64 /* the command line is copied after the task */
#define COMMAND_COPY 99

/* application text layout */
#define TEXT_COLUMNS 64
#define TEXT_LINES 25
#define PROMPT_LINE 24
#define TEXT_AREA AT(27, 5) /* inside the window */

#ifndef K
#define K 0
#endif

/* -DB=_start makes this the entry point of the sh and vi programs */
#define application B

int application(int t, int message, int data) {
  struct task *task = TASK(t);
  int file, length, buffer_size, text, source, pad_until, c;
  int is_shell, pixel, line, column, row, col, i;

  if (message == MSG_INIT) {
    /* vi skips "vi " to find its file name, sh finds nothing */
    file = CALL(SYSCALL_ADDRESS, task->name + 3, SYS_FIND, 0);
    length = CALL(SYSCALL_ADDRESS, file, SYS_FILE_SIZE, 0);
    task->position = task->scroll = 0;
    task->cursor = 1;
    task->size = AT(432, 522);
    buffer_size = length * 8 + (TEXT_LINES + 1) * TEXT_COLUMNS;
    task->text = text = CALL(SYSCALL_ADDRESS, buffer_size, SYS_ALLOC, 0);

    /* lay the file out in 64 column lines, writing zeros as padding */
    pad_until = 0;
    source = 0;
    for (i = 0; i < buffer_size; i++, pad_until++) {
      if (source == length || i < pad_until) {
        pad_until--;
        BYTE(text + i) = 0;
      } else if ((c = BYTE(file + 512 + source++)) < ' ') {
        /* a newline (63) pads to the end of the line, a tab (7) pads to the
           next multiple of 8 columns */
        pad_until |= c * 56 - 497;
        BYTE(text + i) = 0;
      } else {
        BYTE(text + i) = c;
      }
    }
  }

  /* the program name decides the behavior: 's' is odd, 'v' is even */
  is_shell = BYTE(task->name) & 1;
  text = task->text;

  if (message == MSG_RENDER) {
    if (is_shell)
      BYTE(text + PROMPT_LINE * TEXT_COLUMNS) |= '>';

    /* black frame with a white text area inside */
    for (pixel = 0; pixel < SCREEN_PIXELS; pixel++)
      if (IN_RECT(pixel, task->position + AT(26, 4), AT(402, 514)))
        BYTE(data + pixel) =
            IN_RECT(pixel, task->position + TEXT_AREA, AT(400, 512)) ? WHITE
                                                                     : BLACK;

    for (line = 0; line < TEXT_LINES; line++)
      for (column = 0; column < TEXT_COLUMNS; column++)
        DRAW_GLYPH(data + TEXT_AREA + task->position +
                       AT(line * 16, column * 8),
                   BYTE(text + (line + task->scroll) * TEXT_COLUMNS + column),
                   BLACK);
  } else if (message == MSG_KEY) {
    if (is_shell) {
      if (data != '\n') {
        task->cursor++;
        BYTE(text + PROMPT_LINE * TEXT_COLUMNS + task->cursor - 1) = data;
      } else {
        /* scroll everything up a line and run the command just typed */
        for (i = 0; i < TEXT_LINES * TEXT_COLUMNS; i++)
          BYTE(text + i) = BYTE(text + i + TEXT_COLUMNS);
        task->cursor = 1;
        CALL(SYSCALL_ADDRESS, text + (PROMPT_LINE - 1) * TEXT_COLUMNS + 1,
             SYS_EXEC, 0);
      }
    } else {
      /* up arrow and page up are '7' and '8' (-1), down arrow and page down
         are 'q' and 'w' (+1) in the keymap */
      task->scroll += ((data >> 4) ^ (data >> 5)) - 3;
    }
  }
  return 0;
}

int kernel_syscall(int arg, int number, int unused);
int draw_windows(int t, int unused, int buffer);

int kernel_main(int argc, int argv, int envp) {
  int button_down = 0, drag_target = 0, mouse_position = AT(384, 512);
  int shell_name = 's' | 'h' << 8; /* "sh" on the stack */
  int mouse_byte = K, status, data, top, prev, redraw, pixel, row, col;
  int offset;

  if (!argc) {
    /* running as the OS: publish the system call entry for applications */
    SYSCALL_ADDRESS =
        KERNEL_CODE + ((int)kernel_syscall - (int)kernel_main);

    KBC_SEND(0xd1, 0xdf); /* enable the A20 address line */
    KBC_SEND(0xa8, 0);    /* enable the mouse port */
    KBC_SEND(0xd4, 0xf4); /* tell the mouse to start sending data */

    kernel_syscall((int)&shell_name, SYS_EXEC, 0);

    for (;;)
      while ((status = port_in(KBC_STATUS)) & KBC_BUSY) {
        data = port_in(KBC_DATA);
        top = TASK_LIST;
        redraw = 0;

        if (status & KBC_MOUSE) {
          /* mouse packets are buttons, x movement, y movement; the first
             byte received is the acknowledge of the 0xf4 command */
          mouse_byte = (mouse_byte + 1) % 3;
          if (mouse_byte < 2) {
            /* the dragged window moves with the mouse; when nothing is
               dragged drag_target is 0 and memory address 0 is changed */
            WORD(drag_target) -= mouse_position;
            mouse_position +=
                (signed char)data * (mouse_byte ? -SCREEN_WIDTH : 1);
            WORD(drag_target) += mouse_position;
            redraw = mouse_byte;
          } else {
            if (!button_down && data & 1) {
              /* left click: bring the window under the mouse to the top
                 and start dragging it */
              for (prev = TASK_LIST_NODE; top;
                   prev = top, top = TASK(top)->next)
                if (IN_RECT(mouse_position, TASK(top)->position,
                            TASK(top)->size)) {
                  drag_target = (int)&TASK(top)->position;
                  TASK(prev)->next = TASK(top)->next;
                  TASK(top)->next = TASK_LIST;
                  TASK_LIST = top;
                  break;
                }
            }
            button_down = data & 1;
            if (!button_down)
              drag_target = 0;
          }
        } else if (!(data & KEY_RELEASE)) {
          /* key press goes to the top window; the keymap is stored with
             each character XOR 32 */
          CALL(TASK(top)->handler, top, MSG_KEY,
               BYTE(KEYMAP + data % 64) ^ 32);
          top = TASK_LIST;
          redraw = 1;
        }

        if (redraw) {
          for (pixel = 0; pixel < SCREEN_PIXELS; pixel++)
            BYTE(BACK_BUFFER + pixel) = DESKTOP_COLOR;
          draw_windows(top, 0, BACK_BUFFER);
          /* the mouse pointer is a black X centered on the position */
          DRAW_GLYPH(BACK_BUFFER + mouse_position - AT(8, 4), 'X', BLACK);
          for (pixel = 0; pixel < SCREEN_PIXELS; pixel++)
            BYTE(FRAMEBUFFER + pixel) = BYTE(BACK_BUFFER + pixel);
        }
      }
  }

  /* running on Linux: write the kernel image */
  {
#define SETUP_IMAGE_SIZE 720
#define KERNEL_CODE_SIZE 3888
#define KEY(c) ((c) ^ 32)
    /* Loaded at 0x90000.  Only the end of the boot sector (from 0x1f0)
       is used, the original filled the rest with whatever memory followed
       its string. */
    static const unsigned char setup_image[SETUP_IMAGE_SIZE] = {
        /* 0x1f0: initial heap pointer 0x01100800, whose second byte is
           also the setup_sects = 8 field of the Linux boot header */
        [0x1f0] = 0x00, 0x08, 0x10, 0x01,
        /* 0x1f4: the task list, initially empty */
        0x00, 0x00, 0x00, 0x00,
        /* 0x1f8: inb: pop ecx; pop edx; push edx; push ecx; in al,dx; ret */
        0x59, 0x5a, 0x52, 0x51, 0xec, 0xc3,
        /* 0x1fe: boot sector signature */
        0x55, 0xaa,
        /* 0x200: jmp to the setup code at 0x230 */
        0xeb, 0x2e,
        /* 0x202: Linux boot header magic and version */
        'H', 'd', 'r', 'S', 0x00, 0x20,
        /* 0x208: outb: pop ecx; pop edx; pop eax; push eax; push edx;
           push ecx; out dx,al; ret */
        0x59, 0x5a, 0x58, 0x50, 0x52, 0x51, 0xee, 0xc3,
        /* 0x210: type_of_loader, loadflags, setup_move_size 0x8000 which
           is also the GDT limit */
        0x00, 0x00, 0x00, 0x80,
        /* 0x214: code32_start 0x9026b which is also the GDT address */
        0x6b, 0x02, 0x09, 0x00,
        /* 0x218: ramdisk address and size, filled in by the boot loader */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* 0x220: bootsect_kludge */
        0x00, 0x00, 0x00, 0x00,
        /* 0x224: font address, the setup code stores the offset and the
           upper half makes it segment 0xc000 */
        0x00, 0x00, 0x0c, 0x00,
        /* 0x228: cmd_line_ptr */
        0x00, 0x00, 0x00, 0x00,
        /* 0x22c: initrd_addr_max, keep the ramdisk below 16MB */
        0xff, 0xff, 0xff, 0x00,
        /* 0x230: real mode setup code */
        0xb8, 0x01, 0x4f,       /* mov ax,0x4f01   VBE mode information */
        0xb9, 0x05, 0x41,       /* mov cx,0x4105   1024x768x256, linear */
        0x50,                   /* push ax */
        0x51,                   /* push cx */
        0xbf, 0x00, 0x10,       /* mov di,0x1000   into 0x1000:0x1000 */
        0x8e, 0xc7,             /* mov es,di */
        0xcd, 0x10,             /* int 0x10 */
        0x5b,                   /* pop bx */
        0x58,                   /* pop ax */
        0x40,                   /* inc ax          VBE set mode */
        0xcd, 0x10,             /* int 0x10 */
        0xb8, 0x30, 0x11,       /* mov ax,0x1130   get font address */
        0xb7, 0x06,             /* mov bh,6        8x16 font */
        0xcd, 0x10,             /* int 0x10 */
        0x2e, 0x89, 0x2e, 0x24, 0x00, /* mov cs:[0x24],bp */
        0xfa,                   /* cli */
        0x2e, 0x0f, 0x01, 0x1e, 0x6b, 0x00, /* lidt cs:[0x6b] */
        0x2e, 0x0f, 0x01, 0x16, 0x12, 0x00, /* lgdt cs:[0x12] */
        0xb8, 0x01, 0x00,       /* mov ax,1 */
        0x0f, 0x01, 0xf0,       /* lmsw ax         protected mode */
        0x66, 0xea, 0xc3, 0x02, 0x09, 0x00, 0x08, 0x00, /* jmp 8:0x902c3 */
        /* 0x26b: GDT null entry, also the empty IDT */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* 0x273: GDT flat 4GB code segment */
        0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0xcf, 0x00,
        /* 0x27b: GDT flat 4GB data segment */
        0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xcf, 0x00,
        /* 0x283: keymap for scan codes 0 to 63 */
        KEY(' '), KEY(' '), KEY('1'), KEY('2'), KEY('3'), KEY('4'),
        KEY('5'), KEY('6'), KEY('7'), KEY('8'), KEY('9'), KEY('0'),
        KEY('-'), KEY('='), KEY(' '), KEY(' '), KEY('q'), KEY('w'),
        KEY('e'), KEY('r'), KEY('t'), KEY('y'), KEY('u'), KEY('i'),
        KEY('o'), KEY('p'), KEY('['), KEY(']'), KEY('\n'), KEY(' '),
        KEY('a'), KEY('s'), KEY('d'), KEY('f'), KEY('g'), KEY('h'),
        KEY('j'), KEY('k'), KEY('l'), KEY(';'), KEY(' '), KEY(' '),
        KEY(' '), KEY(' '), KEY('z'), KEY('x'), KEY('c'), KEY('v'),
        KEY('b'), KEY('n'), KEY('m'), KEY(','), KEY('.'), KEY(' '),
        KEY(' '), KEY(' '), KEY(' '), KEY(' '), KEY(' '), KEY(' '),
        KEY(' '), KEY(' '), KEY(' '), KEY(' '),
        /* 0x2c3: 32 bit code, falls into kernel_main at 0x2d0 */
        0x66, 0xb8, 0x10, 0x00,       /* mov ax,0x10 */
        0x8e, 0xd8,                   /* mov ds,ax */
        0x8e, 0xd0,                   /* mov ss,ax */
        0xbc, 0xf0, 0x01, 0x09, 0x00, /* mov esp,0x901f0 */
    };

    for (offset = 0; offset < SETUP_IMAGE_SIZE; offset++)
      putchar(setup_image[offset]);
    /* followed by the compiled kernel functions */
    for (offset = 0; offset < KERNEL_CODE_SIZE; offset++)
      putchar(((unsigned char *)kernel_main)[offset]);
  }
  return 0;
}

int draw_windows(int t, int unused, int buffer) {
  struct task *task = TASK(t);
  int position, size, name, pixel, color, i, row, col;

  if (t) {
    position = task->position;
    size = task->size;
    /* windows below are drawn first */
    draw_windows(task->next, 0, buffer);

    /* bevelled border, title bar and gray body */
    for (pixel = 0; pixel < SCREEN_PIXELS; pixel++)
      if (IN_RECT(pixel, position, size)) {
        if (!IN_RECT(pixel, position + AT(1, 1), size - AT(2, 2)))
          color = BLACK;
        else if (!IN_RECT(pixel, position + AT(2, 2), size - AT(3, 3)))
          color = BEVEL_LIGHT;
        else if (!IN_RECT(pixel, position + AT(2, 2), size - AT(4, 4)))
          color = BEVEL_DARK;
        else if (IN_RECT(pixel, position + AT(4, 4),
                         AT(18, WIDTH(size) - 8)))
          color = TITLE_BLUE;
        else
          color = WINDOW_GRAY;
        BYTE(buffer + pixel) = color;
      }

    /* command line as the title */
    for (name = task->name, i = 0; BYTE(name + i); i++)
      DRAW_GLYPH(buffer + task->position + AT(5, 6) + i * 8, BYTE(name + i),
                 WHITE);

    CALL(task->handler, t, MSG_RENDER, buffer);
  }
  return 0;
}

int kernel_syscall(int arg, int number, int unused) {
  int result, header, c, i, j, t, command, elf, section;
  struct task *task;

  if (number == SYS_ALLOC) {
    HEAP_POINTER += arg;
    return HEAP_POINTER - arg;
  }

  if (number == SYS_FILE_SIZE) {
    /* 11 octal digits at offset 124 of the tar header */
    result = 0;
    for (i = 124; i < 135; i++)
      result = result << 3 | BYTE(arg + i) & 7;
    return result;
  }

  /* find the first file whose name starts the command line arg; the
     empty header at the end of the archive matches anything */
  header = RAMDISK_IMAGE;
  for (i = 0; (c = BYTE(header + i)); i++)
    if (c != BYTE(arg + i)) {
      header += kernel_syscall(header, SYS_FILE_SIZE, 0) + 1023 & ~511;
      i = -1;
    }

  if (number == SYS_EXEC && WORD(header)) {
    /* new task goes on top of the window list */
    t = kernel_syscall(TASK_ALLOC_SIZE, SYS_ALLOC, 0);
    task = TASK(t);
    task->next = TASK_LIST;
    TASK_LIST = t;
    task->name = command = t + TASK_NAME_OFFSET;
    for (j = 0; j < COMMAND_COPY; j++)
      BYTE(command + j) = BYTE(arg + j);

    /* the handler is the start of the ELF section that holds the entry
       point (e_shoff at 32, e_entry at 24, sh_addr at 12, sh_offset at 16) */
    elf = header + 512;
    section = elf + WORD(elf + 32);
    while (WORD(section + 12) != WORD(elf + 24))
      section += 40;
    task->handler = WORD(section + 16) + elf;
    CALL(task->handler, t, MSG_INIT, 0);
  }
  return header;
}
