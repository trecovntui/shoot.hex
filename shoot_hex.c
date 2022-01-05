#include <stdio.h>
#include <error.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <termcap.h>
#include <pthread.h>
#include <stdatomic.h>

static char termbuf[2048];
static struct termios oldt, newt;

int size;
int diff;
int *p1__n;
int *p2__n;
int *field;
atomic_int next_one;
int travel_time_increment;
unsigned long long int score;

void handle_ctrl_c(int dummy) {
    printf("\nGot Ctrl+C. Exiting ..\n");
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    free(p1__n);
    free(p2__n);
    exit(0);
}

void clear_screen() {
  printf("\e[1;1H\e[2J");
}

void print_line(int *line) {
  printf("   ");
  for(int i = 0; i < (size - 1); i++) {
    printf("%d ", line[i]);
  }
  printf("%d\n", line[size - 1]);
}

void print_line_field(int *line) {
  printf("   ");
  for(int i = 0; i < (size - 1); i++) {
    if(line[i] == 1) {
      printf("| ");
    }
    else {
      printf("  ");
    }
  }
  if(line[size - 1] == 1) {
    printf("|\n");
  }
  else {
    printf(" \n");
  }
}

void print_blank_line() {
  printf("   ");
  for(int i = 0; i < (size - 1); i++) {
    printf("  ");
  }
  printf(" \n");
}

void update_field(int *line, int row) {
  for(int i = 0; i < size; i++) {
    if(i != (size - row - 1)) {
      print_blank_line();
    }
    else {
      print_line_field(line);
    }
  }
}

void set_line_random(int *line, int size_type) {
  for(int i = 0; i < (4 * size_type); i++) {
    line[i] = (rand() % 2);
  }
}

int set_line_from_chars(int *line, char *types, int size_type) {
  int valid = 1;
  int j = size - 1;
  for(int i = (size_type - 1); i >= 0; i--) {
    if((types[i] >= '0') && (types[i] <= '9')) {
      types[i] -= '0';
    }
    else if((types[i] >= 'a') && (types[i] <= 'f')) {
      types[i] = (types[i] - 'a') + 10;
    }
    else {
      valid = -1;
      break;
    }

    for(int k = 0; k < 4; k++) {
      line[j] = (int) (types[i] & 1);
      types[i] = types[i] >> 1;
      j--;
    }
  }

  if(valid < 0) {
    printf("\rInvalid hex number: ");
    for(int i = 0; i < size_type; i++) {
      printf("%c", types[i]);
    }
    printf("\n");
  }

  return valid;
}

void *game_thread(void *arg) {
  p1__n = (int *) calloc(size, sizeof(int));
  p2__n = (int *) calloc(size, sizeof(int));
  field = (int *) calloc(size * size, sizeof(int));

  int type_size = size / 4;

  clear_screen();
  printf("\n"); // top padding

  print_line(p2__n);
  update_field(p1__n, size + 1);
  print_line(p1__n);
  print_blank_line(); 

  while(1) {
    set_line_random(p2__n, type_size);
    for(int i = 0; i <= size; i++) {
      clear_screen();
      printf("score: %llu\n", score);
      printf("\n"); // top padding

      print_line(p2__n);
      update_field(p2__n, size - i - 1);
      print_line(p1__n);
      print_blank_line(); 
      //usleep(TRAVEL_TIME_INCREMENT);
      usleep(travel_time_increment);
      if(next_one) {
        next_one = 0;
        break;
      }
    }
    for(int i = 0; i < size; i++) {
      if(p1__n[i] != p2__n[i]) {
        printf("FAIL !!\n");
        exit(0);
      }
    }
    score++;
  }
}

void *player_input_thread(void *arg) {
  int type_size = size / 4;
  char c[type_size];

  while(1) {
    do {
      for(int i = 0; i < type_size; i++) {
        do {
          c[i] = getchar();
          if(c[i] == 'n') {
            next_one = 1;
            printf("moving on\n");
          }
        } while(c[i] == 'n');
      }
    } while(set_line_from_chars(p1__n, c, type_size) < 0);
  }

}

int main(int argc, char *argv[]) {
  srand((unsigned int)time(NULL));

  score = 0;
  next_one = 0;

  if(argc < 3) {
    printf("Error. Provide all inputs.\n");
    printf("Field size, allowed: 4, 8, 12, 16\n");
    printf("Difficulty, allowed: 1 - easy, 2 - not so easy, 3 - bot!\n");
    exit(0);
  }

  size = atoi(argv[1]);
  if(!((size == 4) || (size == 8) || (size == 12) || (size == 16))) {
    printf("Invalid field size.\nAllowed: 4, 8, 12, 16\n");
    exit(0);
  }

  char *termtype = getenv("TERM");
  if (tgetent(termbuf, termtype) < 0) {
    error(EXIT_FAILURE, 0, "Could not access the termcap data base.\n");
  }
  int rows = tgetnum("li");
  int columns = tgetnum("co");
  if(!((columns >= (3 + (2 * size))) && (rows >= (6 + size)))) {
    printf("Terminal not big enough for chosen size. cols: %d, rows: %d\n", columns, rows);
    printf("Required: columns >= %d, rows >= %d\n", 3 + (2 * size), 6 + size);
    exit(0);
  }

  diff = atoi(argv[2]);
  if(!((diff == 1) || (diff == 2) || (diff == 3))) {
    printf("Invalid difficulty.\nAllowed: 1 - easy, 2 - not so easy, 3 - bot!\n");
    exit(0);
  }
  int difficulties[3] = {4000, 1500, 700};
  travel_time_increment = (difficulties[diff - 1] * 1000) / 4;

  signal(SIGINT, handle_ctrl_c);

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  pthread_t game_thread_id, player_thread_id;
  pthread_create(&game_thread_id, NULL, game_thread, NULL);
  pthread_create(&player_thread_id, NULL, player_input_thread, NULL);
  pthread_join(game_thread_id, NULL);
  pthread_join(player_thread_id, NULL);

  return 0;
}
