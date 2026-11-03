#include <ctype.h>
#include <stdio.h>

#define ROOMS 30
#define TUNNELS 3      /* every room has exactly three passageways */
#define FAR (ROOMS * 2) /* "infinite" distance from the Wumpus */
#define NO_ROOM -1

int tunnel[ROOMS][TUNNELS]; /* rooms each passageway leads to */
int wumpus_distance[ROOMS]; /* steps to the Wumpus; 0 is the Wumpus' room */
int bats[ROOMS];            /* room holds bats */
int pit[ROOMS];             /* room holds a bottomless pit */
int free_tunnels[ROOMS];    /* passageways not yet dug while building */
int arrow_here[ROOMS];      /* an arrow lies in the room */
int visited[ROOMS];         /* the player has already been here */

int arrows = 0;         /* arrows the player carries */
int score = 0;
int arrows_shot = 0;    /* arrows fired since the last robber capture */
int arrows_found = 0;   /* arrows picked up since the last robber capture */
int robber_room;        /* NO_ROOM once the robber is caught */
int arrows_stolen = 0;  /* arrows the robber has taken */

void game_over(void)
{
  printf("Your score is %d\n", score);
  exit(0);
}

int random_room(void) { return random() % ROOMS; }

int is_adjacent(int room, int other)
{
  return tunnel[room][0] == other || tunnel[room][1] == other ||
         tunnel[room][2] == other;
}

int pit_nearby(int room)
{
  return pit[tunnel[room][0]] || pit[tunnel[room][1]] || pit[tunnel[room][2]];
}

int bats_nearby(int room)
{
  return bats[tunnel[room][0]] || bats[tunnel[room][1]] ||
         bats[tunnel[room][2]];
}

/*
 * Dig the remaining passageways out of `room`, recursively continuing from
 * every room it connects to.  Returns 0 if it got stuck so the caller can undo
 * the last passageway and try a different one.
 */
int dig_tunnels(int room)
{
  int other, tries = 0, min_free;

  while (free_tunnels[room]) {
    min_free = 3;
    free_tunnels[room]--;
    /* pick a different room that still has a free slot and is not already
       connected; prefer rooms with many free slots, relax every 100 tries */
    do {
      other = random_room();
      if (tries++ > 1000)
        return 0;
      if (tries % 100 == 0)
        min_free--;
    } while (other == room || free_tunnels[other] <= 0 ||
             other == tunnel[room][2] || other == tunnel[room][1] ||
             free_tunnels[other] < min_free);
    free_tunnels[other]--;
    tunnel[room][free_tunnels[room]] = other;
    tunnel[other][free_tunnels[other]] = room;
    if (!dig_tunnels(other)) {
      tunnel[room][free_tunnels[room]] = tunnel[other][free_tunnels[other]] =
          NO_ROOM;
      free_tunnels[other]++;
      free_tunnels[room]++;
    } else
      tries = 0;
  }
  return 1;
}

#define SORT2(x, y)                                                            \
  if (x < y) {                                                                 \
    tmp = x;                                                                   \
    x = y;                                                                     \
    y = tmp;                                                                   \
  }

/*
 * Sort each room's passageways in ascending order.  Returns 1 if a room is
 * missing a passageway (the cave must be rebuilt).
 */
int sort_tunnels_find_missing(void)
{
  int room, tmp;

  for (room = 0; room < ROOMS; room++) {
    SORT2(tunnel[room][2], tunnel[room][1])
    SORT2(tunnel[room][1], tunnel[room][0])
    SORT2(tunnel[room][2], tunnel[room][1])
    if (tunnel[room][0] == NO_ROOM)
      return 1;
  }
  return 0;
}

/* Record how many steps every room is from the Wumpus */
void spread_wumpus_scent(int room, int distance)
{
  int t;

  wumpus_distance[room] = distance;
  for (t = 0; t < TUNNELS; t++)
    if (wumpus_distance[tunnel[room][t]] > distance)
      spread_wumpus_scent(tunnel[room][t], distance + 1);
}

int empty_room(void)
{
  int room;

  do {
    room = random_room();
  } while (bats[room] || pit[room]);
  return room;
}

void place_hazards(void)
{
  int n, room;

  for (n = 0; n < ROOMS; n += 8) { /* four bats and four pits */
    bats[empty_room()] = 1;
    pit[empty_room()] = 1;
  }
  spread_wumpus_scent(random_room(), 0);
  for (n = 0; n < 3; n++) {
    do {
      room = random_room();
    } while (bats[room] || pit[room] || arrow_here[room] ||
             !wumpus_distance[room]);
    arrow_here[room] = 1;
  }
  do {
    robber_room = random_room();
  } while (bats[robber_room] || pit[robber_room] ||
           wumpus_distance[robber_room] == 0);
}

void build_cave(void)
{
  int room;

  srandom(getpid());
  do {
    for (room = 0; room < ROOMS; free_tunnels[room++] = TUNNELS) {
      tunnel[room][0] = tunnel[room][1] = tunnel[room][2] = NO_ROOM;
      bats[room] = pit[room] = visited[room] = 0;
      wumpus_distance[room] = FAR;
    }
    dig_tunnels(0);
  } while (sort_tunnels_find_missing());
  place_hazards();
}

void shoot(int room)
{
  int hop, target;
  char ch;

  if (arrows == 0) {
    printf("You have no arrow\n");
    return;
  }
  arrows--;
  arrows_shot++;
  printf("Shoot into which rooms?  ");
  for (hop = 0; hop < 4; hop++) {
    score -= 20 * hop;
    do {
      ch = getchar();
      if (ch == '\n' && arrows > 0) {
        score -= 50;
        printf("You have %d arrow%c\n", arrows, arrows - 1 ? 's' : ' ');
        return;
      } else if (arrows == 0)
        return;
    } while (!isdigit(ch));
    target = ch - '0';
    ch = getchar();
    if (isdigit(ch))
      target = 10 * target + ch - '0';

    if (is_adjacent(room, target)) {
      room = target;
      if (wumpus_distance[room] == 0) {
        printf("You have killed the Wumpus!\n");
        score += 500 + arrows * 100;
        game_over();
      }
    } else if (random() % 3 == 1) {
      printf("The arrow bounces back and kills you\n");
      score -= 100;
      game_over();
    } else {
      /* the missed arrow wakes the Wumpus, who moves to a random room */
      for (target = 0; target < ROOMS; target++)
        wumpus_distance[target] = 2 * ROOMS;
      spread_wumpus_scent(random_room(), 0);
      score -= 100;
    }
    if (ch == '\n') {
      score -= 50;
      printf("You have %d arrow%s\n", arrows, arrows != 1 ? "s" : "");
      return;
    }
  }
}

void play(void)
{
  int room, target;
  char ch;

  /* start in a safe room with no hazards nearby */
  do {
    room = random_room();
  } while (bats[room] || pit[room] || wumpus_distance[room] < 3 ||
           pit_nearby(room) || bats_nearby(room));

  while (1) {
    printf("You are in room %d\n", room);
    if (arrow_here[room]) {
      arrows++;
      printf("You found an arrow\n");
      arrows_found++;
      score += 20;
      arrow_here[room] = 0;
    }
    if (wumpus_distance[room] == 0) {
      printf("Wumpus meat\n");
      score -= 100;
      game_over();
    }
    if (pit[room]) {
      printf("You fell in a pit\n");
      score -= 50;
      game_over();
    }
    if (room == robber_room) {
      if (arrows) {
        arrows--;
        arrows_stolen++;
        score -= 20;
        printf("You are missing an arrow!\n");
      }
      if (arrows_stolen && arrows_stolen == arrows_found - arrows_shot) {
        printf("You catch the robber and recover all %d arrow%s!\n",
               arrows_found - arrows_shot,
               arrows_found - arrows_shot != 1 ? "s" : "");
        arrows = arrows_stolen;
        arrows_stolen = arrows_found = arrows_shot = 0;
        score += 200;
        robber_room = NO_ROOM;
      } else {
        printf("The robber scurries away\n");
        do {
          robber_room = random_room();
        } while (bats[robber_room] || pit[robber_room] ||
                 wumpus_distance[robber_room] == 0 || robber_room == room);
      }
    }

    if (bats[room]) {
      bats[room] = 0;
      bats[random_room()] = 1;
      room = random_room();
      printf("The bats pick you up and drop you in room %d\n", room);
      score -= 10;
      continue;
    }

    if (wumpus_distance[room] < 3)
      printf("You smell a wumpus\n");
    if (pit_nearby(room))
      printf("You feel a breeze\n");
    if (bats_nearby(room))
      printf("You hear a rustling sound\n");
    printf("Passageways to room %d, %d, and %d\n", tunnel[room][0],
           tunnel[room][1], tunnel[room][2]);
    if (!visited[room]) {
      score += 10;
      visited[room] = 1;
    }

    do {
      ch = getchar();
      if (ch == 's')
        shoot(room);
      if (ch == 'l')
        break;
      if (ch == 'i')
        printf("You have %d arrow%c\n", arrows, arrows - 1 ? 's' : ' ');
      if (ch == 'q')
        game_over();
    } while (!isdigit(ch));

    if (isdigit(ch)) {
      target = ch - '0';
      ch = getchar();
      if (isdigit(ch))
        target = target * 10 + ch - '0';
      if (is_adjacent(room, target))
        room = target;
    }
  }
}

int main()
{
  build_cave();
  play();
  game_over();
}
