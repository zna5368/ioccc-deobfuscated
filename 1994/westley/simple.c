/*
 * Compiling with -DO turns this file into the empty (but legal) program
 * below. Without it, the preprocessor plays the game.
 */
#if O
main() {}
#else

/*
 * Player words.
 *
 * Every word starts with a "D" so that it can be typed as a compiler
 * -D option: "-Drop" defines the macro "rop" as 1, "-Dwarf" defines "warf"
 * as 1, and so on. A word that was not typed is an unknown name, and the
 * preprocessor treats unknown names as 0 inside an #if.
 */

/* verbs */
#define DESCRIBE escribe /* -Describe */
#define DIAL ial         /* -Dial     */
#define DRINK rink       /* -Drink    */
#define DROP rop         /* -Drop     */
#define DEPRESS epress   /* -Depress  */
#define DEPOSIT eposit   /* -Deposit  */

/* nouns */
#define DOOR oor              /* -Door       */
#define DRAIN rain            /* -Drain      */
#define DWARF warf            /* -Dwarf      */
#define DOWN own              /* -Down       */
#define DUNGEON ungeon        /* -Dungeon    */
#define DEBRIS ebris          /* -Debris     */
#define DESK esk              /* -Desk       */
#define DIME ime              /* -Dime       */
#define DIMES imes            /* -Dimes      */
#define DIRECTORY irectory    /* -Directory  */
#define DOCUMENT ocument      /* -Document   */
#define DAIQUIRI aiquiri      /* -Daiquiri   */
#define DALMATIAN almatian    /* -Dalmatian  */
#define DALMATION almation    /* -Dalmation (misspelling also accepted) */
#define DONUT onut            /* -Donut      */
#define PHONE_NUMBER UNgeon0614 /* -DUNgeon0614 */

/*
 * Game text.
 *
 * Each message is the name of an include file that does not exist, so the
 * compiler prints the message inside its "No such file or directory"
 * error. A file name cannot contain a newline, so every line of text is
 * padded with tabs and spaces until it fills a whole 80 column terminal
 * line (with 8 column tabs) and wraps onto the next line by itself.
 * The comment above each message shows how it looks on the screen.
 */

/*
 *  The orc scrambles to the drain,
 *  trying to get the donut; you
 *  run out the door and escape!
 *
 *         The End
 */
#define MSG_ESCAPE "						  										  The orc scrambles to the drain,					          trying to get the donut; you						          run out the door and escape!							  										  	       The End								  										  "

/*
 *  It reads:
 *      Depress Dotted Dog
 */
#define MSG_DOCUMENT "						  										  It reads:								              Depress Dotted Dog							  										  "

/*
 *  The door is decorated with relief
 *  figures of various dog breeds
 */
#define MSG_DOOR "						  										  The door is decorated with relief						  figures of various dog breeds						            										  "

/*
 *  Through the drain you see a dwarf
 *  in another cell
 */
#define MSG_DRAIN "						  										  Through the drain you see a dwarf						  in another cell							            										  "

/*
 *  The gnarled dwarf looks thirsty
 */
#define MSG_DWARF "						  										  The gnarled dwarf looks thirsty					            										  "

/*
 *  You notice a desk, a phone, and
 *  a phone directory among the clutter
 */
#define MSG_CLUTTER "						  										  You notice a desk, a phone, and					          a phone directory among the clutter						  										  "

/*
 *  There are some dimes on the desk
 */
#define MSG_DESK "						  										  There are some dimes on the desk						  										  "

/*
 *  It reads:
 *      Pixie's Pub -
 *      instant service -
 *      Dial DUNgeon0614
 */
#define MSG_DIRECTORY "						  										  It reads:								              Pixie's Pub -								      instant service -								      Dial DUNgeon0614								  										  "

/*
 *  You notice nothing unusual
 */
#define MSG_NOTHING "						  										  You notice nothing unusual							  										  "

/*
 *  Pixie takes your order, and
 *  magically, the room is filled
 *  with banana daiquiris
 */
#define MSG_DIAL "						  										  Pixie takes your order, and							  magically, the room is filled						          with banana daiquiris								  										  "

/*
 *  As you drink, a voice from
 *  below says 'Could you drop
 *  one down here?'
 */
#define MSG_DRINK "						  										  As you drink, a voice from						          below says 'Could you drop						          one down here?'								  										  "

/*
 *  The dwarf eagerly accepts
 *  the drink.  'Thanks!  Here,
 *  I found this, but I can't
 *  understand it.'  He hands
 *  you a document
 */
#define MSG_GIVE_DRINK "						  										  The dwarf eagerly accepts						          the drink.  'Thanks!  Here,						          I found this, but I can't							  understand it.'  He hands							  you a document								  										  "

/*
 *  You drop the drink
 *  on the floor
 */
#define MSG_DROP_DRINK "						  										  You drop the drink							          on the floor									  										  "

/*
 *  As you press the dalmatian
 *  figure, the door opens - only
 *  to reveal a menacing orc by
 *  a vending machine (with a
 *  sign reading 'DEPOSIT MONEY')
 */
#define MSG_DALMATIAN "						  										  As you press the dalmatian						          figure, the door opens - only						          to reveal a menacing orc by							  a vending machine (with a							  sign reading 'DEPOSIT MONEY')							  										  "

/*
 *  Donuts spill out; you get
 *  some, the orc hungrily eats
 *  the rest
 */
#define MSG_DEPOSIT "						  										  Donuts spill out; you get						          some, the orc hungrily eats						          the rest									  										  "

/*
 *  The orc quickly eats the
 *  donut; he still bars the
 *  way
 */
#define MSG_DROP_DONUT "						  										  The orc quickly eats the						          donut; he still bars the						          way										  										  "

/*
 *  Be more specific
 */
#define MSG_BE_SPECIFIC "						  										  Be more specific								  										  "

/*
 *  You are in a debris-filled
 *  dungeon; a door bars the
 *  way, and sludge trickles into
 *  a floor drain
 */
#define MSG_OPENING "						  										  You are in a debris-filled						          dungeon; a door bars the						          way, and sludge trickles into						          a floor drain									  										  "

/*
 * Game logic.
 *
 * The game has no saved state: the words on the compiler command line
 * alone decide which message is "included". Word order does not matter.
 */

#if DROP && DONUT && ((DOWN && DRAIN) || DWARF)

/* Drop a donut down the drain (or to the dwarf): the orc goes after it */
#include MSG_ESCAPE

#elif DESCRIBE

/* Look at something: only the first matching object is described */
#if DOCUMENT
#include MSG_DOCUMENT
#elif DOOR
#include MSG_DOOR
#elif DRAIN
#include MSG_DRAIN
#elif DWARF
#include MSG_DWARF
#elif DUNGEON || DEBRIS
#include MSG_CLUTTER
#elif DESK
#include MSG_DESK
#elif DIRECTORY
#include MSG_DIRECTORY
#else
#include MSG_NOTHING
#endif

#else

/*
 * Do something: every action that matches gets a message. Compilers that
 * stop at the first missing include file show only the first one; those
 * that keep going show them all, followed by the opening description.
 */
#if DIAL && PHONE_NUMBER
#include MSG_DIAL
#endif

#if DRINK && DAIQUIRI
#include MSG_DRINK
#endif

#if DROP && DAIQUIRI
#if DWARF || DOWN
#include MSG_GIVE_DRINK
#else
#include MSG_DROP_DRINK
#endif
#endif

#if DEPRESS && (DALMATIAN || DALMATION)
#include MSG_DALMATIAN
#endif

#if DEPOSIT && (DIME || DIMES)
#include MSG_DEPOSIT
#endif

#if DROP && DONUT
#include MSG_DROP_DONUT
#endif

/* exactly one of these words on its own is not enough to do anything */
#if DROP + DEPRESS + DIME + DEPOSIT + DRINK + DAIQUIRI + DONUT == 1
#include MSG_BE_SPECIFIC
#endif

/* no command (or nothing happened): describe the dungeon */
#include MSG_OPENING

#endif

#endif
