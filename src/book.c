/*
 * book.c - C source for GNU SHOGI
 *
 * Copyright (c) 1993 Matthias Mutz
 *
 * GNU SHOGI is based on GNU CHESS
 *
 * Copyright (c) 1988,1989,1990 John Stanback Copyright (c) 1992 Free Software
 * Foundation
 *
 * This file is part of GNU SHOGI.
 *
 * GNU Shogi is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 1, or (at your option) any later
 * version.
 *
 * GNU Shogi is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * GNU Shogi; see the file COPYING.  If not, write to the Free Software
 * Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "gnushogi.h"
#include "ataks.h"
#ifdef MSDOS
#include <io.h>
#endif
#if !defined MSDOS && !defined THINK_C
#define O_BINARY 0
#endif
#include <fcntl.h>
#ifdef THINK_C
#include <unix.h>
/* #define BOOKTEST */
#endif



unsigned booksize = BOOKSIZE;
unsigned short bookmaxply = BOOKMAXPLY;
unsigned bookcount = 0;


char *bookfile = NULL;
#ifdef BINBOOK
char *binbookfile = BINBOOK;
#else
char *binbookfile = NULL;
#endif



int GotBook = false;
static char bmvstr[3][7];
long bhashbd, bhashkey;


void
Balgbr (short int f, short int t, short int flag)


     /*
      * Generate move strings in different formats.
      */

{
  int m3p;
  short promoted = false;

  if ( (f & 0x80) != 0)
    {                
      f &= 0x7f;
      promoted = true;
    }

  if ( f > NO_SQUARES ) 
    { short piece;
      piece = f - NO_SQUARES;
      if ( f > (NO_SQUARES+NO_PIECES) )
        piece -= NO_PIECES;
      flag = (dropmask | piece); 
    }
  if ( (t & 0x80) != 0 )
    {
      flag |= promote;
      t &= 0x7f;
    }
  if ( f == t && (f != 0 || t != 0) ) 
    { 
#if !defined BAREBONES
      printz("error in algbr: FROM=TO=%d, flag=0x%4x\n",t,flag);
#endif
      bmvstr[0][0] = bmvstr[1][0] = bmvstr[2][0] = '\0';
    }
  else
  if ( (flag & dropmask) != 0 )
    {
      /* bmvstr[0]: P*3c bmvstr[1]: P'3c */ 
      short piece = flag & pmask;
      bmvstr[0][0] = pxx[piece];
      bmvstr[0][1] = '*';
      bmvstr[0][2] = cxx[column (t)];
      bmvstr[0][3] = rxx[row (t)];
      bmvstr[0][4] = bmvstr[2][0] = '\0';
      strcpy (bmvstr[1], bmvstr[0]);
      bmvstr[1][1] = '\'';
    }
  else
  if (f != 0 || t != 0)
    {
      /* algebraic notation */
      /* bmvstr[0]: 7g7f bmvstr[1]: (+)P7g7f(+) bmvstr[2]: (+)P7f(+) */
      bmvstr[0][0] = cxx[column (f)];
      bmvstr[0][1] = rxx[row (f)];
      bmvstr[0][2] = cxx[column (t)];
      bmvstr[0][3] = rxx[row (t)];
      bmvstr[0][4] = '\0';
      if (promoted)
	{
          bmvstr[1][0] = bmvstr[2][0] = '+';
          bmvstr[1][1] = bmvstr[2][1] = pxx[board[f]];
          strcpy(&bmvstr[1][2],&bmvstr[0][0]);
          strcpy(&bmvstr[2][2],&bmvstr[0][2]);
	}                                  
      else
	{
          bmvstr[1][0] = bmvstr[2][0] = pxx[board[f]];
          strcpy(&bmvstr[1][1],&bmvstr[0][0]);
          strcpy(&bmvstr[2][1],&bmvstr[0][2]);
	}
      if (flag & promote)
        {
  		strcat(bmvstr[0], "+");
  		strcat(bmvstr[1], "+");
  		strcat(bmvstr[2], "+");
        }
    }
  else
    bmvstr[0][0] = bmvstr[1][0] = bmvstr[2][0] = '\0';
}




#ifndef QUIETBOOKGEN
void
bkdisplay (s, cnt, moveno)
     char *s;
     int cnt;
     int moveno;
{
    static short pnt;
    struct leaf *node;
    int r, c, l;

    pnt = TrPnt[2];
    printf ("matches = %d\n", cnt);
    printf ("inout move is :%s: move number %d side %s\n", 
                s, moveno / 2 + 1, (moveno & 1) ? "white" : "black");
#ifndef SEMIQUIETBOOKGEN
    printf ("legal moves are \n");
    while (pnt < TrPnt[3])
      {
	  node = &Tree[pnt++];
 	  if ( is_promoted[board[node->f]] )
	    Balgbr (node->f | 0x80, node->t, (short) node->flags);
	  else
	    Balgbr (node->f, node->t, (short) node->flags);
	  printf ("%s %s %s\n", 
            bmvstr[0], bmvstr[1], bmvstr[2]);
      }
    printf ("\n current board is\n");
    for (r = (NO_ROWS-1); r >= 0; r--)
	{
	  for (c = 0; c <= (NO_COLS-1); c++)
	    { char pc;
	      l = locn (r, c);
	      pc = (is_promoted[board[l]] ? '+' : ' ');
	      if (color[l] == neutral)
		printf (" -");
	      else if (color[l] == black)
		printf ("%c%c", pc, qxx[board[l]]);
	      else
		printf ("%c%c", pc, pxx[board[l]]);
	    }
	  printf ("\n");
	}
    printf ("\n");
    {  
	short color;
	for (color = black; color <= white; color++)
	  { short piece, c;
	    printf((color==black)?"black ":"white ");
            for (piece = pawn; piece <= king; piece++)
	      if (c = Captured[color][piece]) 
		printf("%i%c ",c,pxx[piece]);
            printf("\n");
          };
    }
#endif /* SEMIQUIETBOOKGEN */
}

#endif /* QUIETBOOKGEN */

int
BVerifyMove (char *s, short unsigned int *mv, int moveno)

     /*
      * Compare the string 's' to the list of legal moves available for the
      * opponent. If a match is found, make the move on the board.
      */

{
    static short pnt, tempb, tempc, tempsf, tempst, cnt;
    static struct leaf xnode;
    struct leaf *node;

    *mv = 0;
    cnt = 0;
    MoveList (opponent, 2);
    pnt = TrPnt[2];
    while (pnt < TrPnt[3])
      {
	  node = &Tree[pnt++];
	  if ( is_promoted[board[node->f]] )
	    Balgbr (node->f | 0x80, node->t, (short) node->flags);
	  else
	    Balgbr (node->f, node->t, (short) node->flags);
	  if (strcmp (s, bmvstr[0]) == 0 || strcmp (s, bmvstr[1]) == 0 ||
	      strcmp (s, bmvstr[2]) == 0)
	    {
		cnt++;
		xnode = *node;
	    }
      }
    if (cnt == 1)
      {
	  MakeMove (opponent, &xnode, &tempb, &tempc, &tempsf, &tempst, &INCscore);
	  if (SqAtakd (PieceList[opponent][0], computer))
	    {
		UnmakeMove (opponent, &xnode, &tempb, &tempc, &tempsf, &tempst);
		/* Illegal move in check */
#ifndef QUIETBOOKGEN
		printf (CP[77]);
		printf ("\n");
		bkdisplay (s, cnt, moveno);
#endif
		return (false);
	    }
	  else
	    {
		*mv = (xnode.f << 8) | xnode.t;
		if ( is_promoted[board[xnode.t]] )
		  Balgbr (xnode.f | 0x80, xnode.t, false);
		else
		  Balgbr (xnode.f, xnode.t, false);
		return (true);
	    }
      }
    /* Illegal move */
#ifndef QUIETBOOKGEN
    printf (CP[75], s);
    bkdisplay (s, cnt, moveno);
#endif
    return (false);
}

void
RESET (void)

     /*
      * Reset the board and other variables to start a new game.
      */

{
    short int l;

    flag.illegal = flag.mate = flag.post = flag.quit = flag.reverse = flag.bothsides = flag.onemove = flag.force = false;
    flag.material = flag.coords = flag.hash = flag.easy = flag.beep = flag.rcptr = true;
    flag.stars = flag.shade = flag.back = flag.musttimeout = false;
    flag.gamein = false;
    GenCnt = 0;
    GameCnt = 0;
    CptrFlag[0] = false;
    opponent = black;
    computer = white;
    for (l = 0; l < NO_SQUARES; l++)
      {
	  board[l] = Stboard[l];
	  color[l] = Stcolor[l];
	  Mvboard[l] = 0;
      }
    ClearCaptured ();
    InitializeStats ();
    hashbd = hashkey = 0; 
}

int
Vparse (FILE * fd, unsigned short *mv, short int side, char *opening, int moveno)
{
    register int c, i;
    char s[128];
    char *p;

    while (true)
      {
          
	  while ((c = getc (fd)) == ' ' || c == '!' || c == '/' || c == '\n');

	  if ( c == '(' ) 
	    { 	/* amount of time spent for the last move */
		while ((c = getc(fd)) != ')' && c != EOF);
		if ( c == ')' ) 
		  while ((c = getc (fd)) == ' ' || c == '\n');
	    }

	  if (c == '[')
	    { 	/* comment for the actual game */
		while ( (c = getc(fd)) != ']' && c != EOF );
		if ( c == ']' ) 
		  while ((c = getc (fd)) == ' ' || c == '\n');
	    }
          
	  if (c == '\r')
	      continue;

	  if (c == '#')
	    {	/* comment */
		p = opening;
		do
		  {
		      *p++ = c;
		      c = getc (fd);
		      if (c == '\r')
			  continue;
		      /* goes to end of line */
		      if (c == '\n')
			{
			    *p = '\0';
			    return 0;
		        } 
		      if (c == EOF)
			return -1;
		  }
		while (true);
	    }

	  s[i = 0] = (char) c;

          while ( c >= '0' && c <= '9' )
	    { 
	      c = getc(fd);
	      s[++i] = (char) c;
	    }

	  if ( c == '.' )
	    { 
	      while ((c = getc (fd)) == ' ' || c == '.' || c == '\n');
	      s[i = 0] = (char) c;
	    }     

	  while ((c = getc (fd)) != '?' && c != '!' && c != ' ' && c != '(' && c != '\n' && c != '\t' && c != EOF)
	    {
		if (c == '\r')
		    continue;
		if (c != 'x' && c != '-' && c != ',' && c != ';' && c != '=')
		    s[++i] = (char) c;
	    }
	  s[++i] = '\0';

	  if ( c == '(' ) 
	    {
	      while ((c = getc(fd)) != ')' && c != EOF);
	      if ( c == ')' ) 
		c = getc(fd);
	    }

	  if (c == EOF)
	      return (-1);

	  if (s[0] == '#')
	    {
		while (c != '\n' && c != EOF)
		    c = getc (fd);
		if (c == EOF)
		    return -1;
		else
		    return (0);
	    }

	  if (strcmp (s, "draw") == 0)
	      continue;
	  else if (strcmp (s, "1-0") == 0)
	      continue;
	  else if (strcmp (s, "0-1") == 0)
	      continue;
	  else if (strcmp (s, "Resigns") == 0)
	      continue;
	  else if (strcmp (s, "Resigns.") == 0)
	      continue;
	  else if (strcmp (s, "Sennichite") == 0)
	      continue;
	  else if (strcmp (s, "Sennichite.") == 0)
	      continue;
	  else if (strcmp (s, "Jishogi") == 0)
	      continue;
	  else if (strcmp (s, "Jishogi.") == 0)
	      continue;

	  bhashkey = hashkey;
	  bhashbd = hashbd;

	  i = BVerifyMove (s, mv, moveno);

	  if (c == '?')
	    {			/* Bad move, not for the program to play */
		*mv |= BADMOVE;	/* Flag it ! */
		while ((c = getc (fd)) == '?' || c == '!' || c == '/');
	    }
	  else if (c == '!')
	    {			/* Good move */
		while ((c = getc (fd)) == '?' || c == '!' || c == '/');
	    }
	  else if (c == '\r')
	      c = getc (fd);

	  if ( c == '(' ) 
	    while ((c = getc(fd)) != ')' && c != EOF);

	  if (!i)
	    {
		printf ("%s \n", opening);
		/* flush to start of next */
		while ((c = getc (fd)) != '#' && c != EOF);
		if (c == EOF)
		    return -1;
		else
		  {
		      ungetc (c, fd);
		      return i;
		  }
	    }

	  return (i);
      }
}


#ifdef GDX       


struct gdxadmin
{
    unsigned int bookcount;
    unsigned int booksize;
    unsigned long maxoffset;
} ADMIN, B;

struct gdxdata
{
    unsigned long hashbd;
    unsigned short hashkey;
    unsigned short bmove;
    unsigned short flags; /* flag LASTMOVE */
    unsigned short hint;
    unsigned short count;
} DATA;

#ifdef LONG64
#define lts(x) (((x>>48)&0xfffe)|side)
#else
#if defined THINK_C || defined USE_LTSIMP

static unsigned short ltsimp (long x)
{ unsigned short n;
  n = (((x>>16)&0xfffe));
#if 0
  printf("x=0x%lx lts(x)=0x%x\n",x,n);
#endif
  return(n);
}
#define lts(x) (ltsimp(x) | side)
#else
#define lts(x) (((x>>16)&0xfffe)|side)
#endif
#endif
unsigned long currentoffset;
int gfd;

void
GetOpenings (void)

     /*
      * Read in the Opening Book file and parse the algebraic notation for a move
      * into an unsigned integer format indicating the from and to square. Create
      * a linked list of opening lines of play, with entry->next pointing to the
      * next line and entry->move pointing to a chunk of memory containing the
      * moves. More Opening lines of up to 100 half moves may be added to
      * gnuchess.book. But now its a hashed table by position which yields a move
      * or moves for each position. It no longer knows about openings per say only
      * positions and recommended moves in those positions.
      */
{
    register short int i;
    char opening[80];
    char msg[80];
    int mustwrite = false;
    unsigned short xside, doit, side;
    short int c;
    unsigned short mv;
    unsigned short ix;
    unsigned int x;
    unsigned int games = 0;

    FILE *fd;

    if ((fd = fopen (bookfile, "r")) == NULL)
	fd = fopen ("gnushogi.book", "r");
    if (fd != NULL)
      {
	  /* yes add to book */
	  /* open book as writer */
	  gfd = open (binbookfile, O_RDONLY | O_BINARY);
	  if (gfd >= 0)
	    {
		if (sizeof(struct gdxadmin) == read (gfd, (char *)&ADMIN, sizeof (struct gdxadmin)))
		  {
		      B.bookcount = ADMIN.bookcount;
		      B.booksize = ADMIN.booksize;
		      B.maxoffset = ADMIN.maxoffset;
		      if (B.booksize && !(B.maxoffset == ((unsigned long)(B.booksize - 1) * sizeof (struct gdxdata) + sizeof (struct gdxadmin))))
			{
			    printf ("bad format %s\n", binbookfile);
			    exit (1);
			}
		  }
		else
		  {
		      printf ("bad format %s\n", binbookfile);
		      exit (1);
		  }
		close (gfd);
                gfd = open (binbookfile, O_RDWR | O_BINARY);

	    }
	  else
	    {
#ifdef THINK_C
                gfd = open (binbookfile, O_RDWR | O_CREAT | O_BINARY);
#else
                gfd = open (binbookfile, O_RDWR | O_CREAT | O_BINARY, 0644);
#endif
		ADMIN.bookcount = B.bookcount = 0;
		ADMIN.booksize = B.booksize = booksize;
                B.maxoffset = ADMIN.maxoffset = (unsigned long) (booksize - 1) * sizeof (struct gdxdata) + sizeof (struct gdxadmin);
		DATA.hashbd = 0;
		DATA.hashkey = 0;
		DATA.bmove = 0;
		DATA.flags = 0;
		DATA.hint = 0;
		DATA.count = 0;
		write (gfd, (char *)&ADMIN, sizeof (struct gdxadmin));
		printf ("creating bookfile %s  %ld %d\n", binbookfile, B.maxoffset, B.booksize);
		for (x = 0; x < B.booksize; x++)
		  {
		      write (gfd, (char *)&DATA, sizeof (struct gdxdata));
		  }


	    }
	  if (gfd >= 0)
	    {


		/* setvbuf(fd,buffr,_IOFBF,2048); */
		side = black;
		xside = white;
		hashbd = hashkey = 0;
		i = 0;

		while ((c = Vparse (fd, &mv, side, opening, i)) >= 0)
		  {
		      if (c == 1)
                        {

			    /*
                             * if not first move of an opening and first
                             * time we have seen it save next move as
                             * hint
                             */
			    i++;
			    if (i < bookmaxply + 2)
			      {
				  if (i > 1)
				    {
					DATA.hint = mv & 0x7fff;
				    }
				  if (i < bookmaxply + 1)
				    {
					doit = true;

					/*
		                         * see if this position and
		                         * move already exist from
		                         * some other opening
		                         */

					/*
		                         * is this ethical, to offer
		                         * the bad move as a
		                         * hint?????
		                         */
					ix = 0;
					if (mustwrite)
					  {
					      lseek (gfd, currentoffset, SEEK_SET);
					      write (gfd, (char *)&DATA, sizeof (struct gdxdata));
					      mustwrite = false;
					  }
					doit = true;
                                        currentoffset = (unsigned long) ((unsigned long)bhashkey % B.booksize) * sizeof (struct gdxdata) + sizeof (struct gdxadmin);
					while (true)
					  {

					      lseek (gfd, currentoffset, SEEK_SET);
					      if ((read (gfd, (char *)&DATA, sizeof (struct gdxdata)) == 0))
						    break;

					      if (DATA.bmove == 0) break;
					      if (DATA.hashkey == (unsigned short)(lts(bhashkey)) && 
					          DATA.hashbd == (unsigned long)bhashbd)
						{                                 
						    if (DATA.bmove == mv)
						      {
							  DATA.count++;
                                                          /*
				                           * yes so just bump count - count is
				                           * used to choose opening move in
				                           * proportion to its presence in the book
				                           */
							  doit = false;
							  mustwrite = true;
							  break;   
						      } else if(DATA.flags & LASTMOVE){
							DATA.flags &= (~LASTMOVE);	
					      		lseek (gfd, currentoffset, SEEK_SET);
					      		write (gfd, (char *)&DATA, sizeof (struct gdxdata));
							}
						}
					      currentoffset += (unsigned long)sizeof (struct gdxdata);
					      if (currentoffset > B.maxoffset)
						  currentoffset = (unsigned long)sizeof (struct gdxadmin);
					  }

					/*
		                         * doesn`t exist so add it to
		                         * the book
		                         */
					if (!mustwrite)
					  {
					      B.bookcount++;
#if !defined BAREBONES
#ifdef THINK_C
					      if (B.bookcount % 100 == 0)
#else
					      if (B.bookcount % 1000 == 0)
#endif
						  printf ("%d rec %d openings processed\n", B.bookcount,games);
#endif
					      /* initialize a record */
					      DATA.hashbd = (unsigned long)bhashbd;
					      DATA.hashkey = (unsigned short)(lts(bhashkey));
					      DATA.bmove = mv;
					      DATA.flags = LASTMOVE;
					      DATA.count = 1;
					      DATA.hint = 0;
					      mustwrite = true;
					  }
				    }
			      }
			    computer = opponent;
			    opponent = computer ^ 1;

			    xside = side;
			    side = side ^ 1;
			}
		      else if (i > 0)
			{
			    /* reset for next opening */
			    games++;
			    if (mustwrite)
			      {
				  lseek (gfd, currentoffset, SEEK_SET);
				  write (gfd, (char *)&DATA, sizeof (struct gdxdata));
				  mustwrite = false;
			      }
			    RESET ();
			    i = 0;
			    side = black;
			    xside = white;
			    hashbd = hashkey = 0;

			}
		  }
		if (mustwrite)
		  {
		      lseek (gfd, currentoffset, SEEK_SET);
		      write (gfd, (char *)&DATA, sizeof (struct gdxdata));
		      mustwrite = false;
		  }
		fclose (fd);
		/* write admin rec with counts */
		ADMIN.bookcount = B.bookcount;
		currentoffset = 0;
		lseek (gfd, currentoffset, 0);
		write (gfd, (char *)&ADMIN, sizeof (struct gdxadmin));

		close (gfd);
	    }
      }
    if (binbookfile != NULL)
      {
	  /* open book as writer */
	  gfd = open (binbookfile, O_RDONLY | O_BINARY);
	  if (gfd >= 0)
	    {
		read (gfd, (char *)&ADMIN, sizeof (struct gdxadmin));
		B.bookcount = ADMIN.bookcount;
		B.booksize = ADMIN.booksize;
		B.maxoffset = ADMIN.maxoffset;
                if (B.booksize && !(B.maxoffset == ((unsigned long) (B.booksize - 1) * sizeof (struct gdxdata) + sizeof (struct gdxadmin))))
		  {
		      printf ("bad format %s\n", binbookfile);
		      exit (1);
		  }

	    }
	  else
	    {
		B.bookcount = 0;
		B.booksize = booksize;

	    }

#if !defined BAREBONES 
	  sprintf (msg, CP[213], B.bookcount, B.booksize);
	  ShowMessage (msg);
#endif
      }
    /* set every thing back to start game */
    Book = BOOKFAIL;
    RESET ();
    /* now get ready to play */
    if (!B.bookcount)
      {
#if !defined BAREBONES
	  ShowMessage (CP[212]);
#endif
	  Book = 0;
      }
}


int
OpeningBook (unsigned short *hint, short int side)

     /*
      * Go thru each of the opening lines of play and check for a match with the
      * current game listing. If a match occurs, generate a random number. If this
      * number is the largest generated so far then the next move in this line
      * becomes the current "candidate". After all lines are checked, the
      * candidate move is put at the top of the Tree[] array and will be played by
      * the program. Note that the program does not handle book transpositions.
      */

{
    unsigned short r, m;
    int possibles = TrPnt[2] - TrPnt[1];

    gsrand ((unsigned int) time ((long *) 0));
    m = 0;

    /*
     * find all the moves for this position  - count them and get their
     * total count
     */
    {
	register unsigned short i, x;
	register unsigned short rec = 0;
	register unsigned short summ = 0;
	register unsigned short h = 0, b = 0;
	struct gdxdata OBB[128];
#ifdef BOOKTEST
        printf("looking for book move, hashbd = 0x%lx lts(hashkey) = 0x%x\n",
                  (unsigned long)hashbd,(unsigned short)lts(hashkey));
#endif
	if (B.bookcount == 0)
	  {
	      Book--;
	      return false;
	  }
        currentoffset = (unsigned long) ((unsigned long)hashkey % B.booksize) * sizeof (struct gdxdata) + sizeof (struct gdxadmin);
	x = 0;
	lseek (gfd, currentoffset, 0);
	while (true)
	  {
	      if (read (gfd, (char *)&OBB[x], sizeof (struct gdxdata)) == 0)
		    break;
	      if (OBB[x].bmove == 0) break;
	      if (OBB[x].hashkey == (unsigned short)(lts(hashkey)) && 
	          OBB[x].hashbd == (unsigned long)hashbd)
		{
		    x++;if(OBB[x-1].flags & LASTMOVE) break;
		}
		currentoffset += sizeof (struct gdxdata); 
	      if (currentoffset > B.maxoffset){
		  lseek (gfd, sizeof (struct gdxadmin), 0);
		  currentoffset += sizeof (struct gdxadmin); 
		}

	  }
#ifdef BOOKTEST
        printf("%d book move(s) found.\n",x);
#endif
	if (x == 0)
	  {
	      Book--;
	      return false;
	  }
	for (i = 0; i < x; i++)
	  {
	      if ((m = OBB[i].bmove) & BADMOVE)
		{
		    m ^= BADMOVE;
		    /* is the move is in the MoveList */
		    for (b = TrPnt[1]; b < (unsigned) TrPnt[2]; b++)
		      {
			  if (((Tree[b].f << 8) | Tree[b].t) == m)
			    {

				if (--possibles)
				    Tree[b].score = DONTUSE;
				break;
			    }
		      }
		}
	      else 
		{
#if defined DEBUG || defined BOOKTEST
 		  char s[20];
		  movealgbr(m = OBB[i].bmove,s); 
		  printf("finding book move: %s\n",s);
#endif
		  summ += OBB[i].count;
		}
	  }
	if (summ == 0)
          {
              Book--;
              return false;
          }

	r = (urand () % summ);
	for (i = 0; i < x; i++)
	    if (!(OBB[i].bmove & BADMOVE) ){
	        if( r < OBB[i].count)
	            {
		        rec = i;
		        break;
	            }
	          else
		      r -= OBB[i].count;
	    } 

	h = ((OBB[rec].hint) & 0x7fff);
	m = ((OBB[rec].bmove) & 0x7fff);
	/* make sure the move is in the MoveList */
	for (b = TrPnt[1]; b < (unsigned) TrPnt[2]; b++)
	  {
	      if (((Tree[b].f << 8) | Tree[b].t) == m)
		{
		    Tree[b].flags |= book;
		    Tree[b].score = 0;
		    break;
		}
	  }
	/* Make sure its the best */

	pick (TrPnt[1], TrPnt[2] - 1);
	if (Tree[TrPnt[1]].score)
	  {
	      /* no! */
	      Book--;
	      return false;
	  }
	/* ok pick up the hint and go */
	*hint = h;
	return true;
    }
    Book--;
    return false;
}



#else /* memory based */


unsigned int BKTBLSIZE;
unsigned long BOOKMASK;
unsigned bookpocket = BOOKPOCKET;

static struct bookentry
{
    unsigned long bookkey;
    unsigned long bookbd;
    unsigned short bmove;
    unsigned short hint;
    unsigned short count;
    unsigned short flags;
} *OBEND;

struct bookentry *OpenBook = NULL;
static struct bookentry **BookTable;


static void
bkalloc (unsigned bksize)
{
    register int i, f;
    /* allocate space for the book */
    f = (bksize + bookpocket - 1) / bookpocket;
    bksize = booksize = f * bookpocket;
#ifdef MSDOS
    OpenBook = (struct bookentry *) _halloc (bksize * sizeof (struct bookentry));
#else
    OpenBook = (struct bookentry *) malloc (bksize * sizeof (struct bookentry));
#endif
    if (OpenBook == NULL)
      {
	  perror ("memory alloc");
	  exit (1);
      }
    for (BKTBLSIZE = 1, BOOKMASK = 1; BKTBLSIZE < f; BKTBLSIZE *= 2, BOOKMASK = (BOOKMASK << 1));
    BOOKMASK -= 1;
    BookTable = (struct bookentry **) malloc (BKTBLSIZE * sizeof (struct bookentry *));
    if (BookTable == NULL)
      {
	  perror ("memory alloc");
	  exit (1);
      }
    for (i = 0; i < BKTBLSIZE; i++)
      {
	  BookTable[i] = &OpenBook[bksize / BKTBLSIZE * i];
      }
    OBEND = (OpenBook + ((bksize) * sizeof (struct bookentry)));
}

void
GetOpenings (void)

/*
 * Read in the Opening Book file and parse the algebraic notation for a move
 * into an unsigned integer format indicating the from and to square. Create
 * a linked list of opening lines of play, with entry->next pointing to the
 * next line and entry->move pointing to a chunk of memory containing the
 * moves. More Opening lines of up to 100 half moves may be added to
 * gnuchess.book. But now its a hashed table by position which yields a move
 * or moves for each position. It no longer knows about openings per say only
 * positions and recommended moves in those positions.
 */
{
    FILE *fd;
    register struct bookentry *OB = NULL;
    register struct bookentry *OC = NULL;
    register short int i;
    char opening[80];
    char msg[80];
    unsigned short xside, doit, side;
    short int c;
    unsigned short mv;

    if (binbookfile != NULL)
      {
#ifdef THINK_C
	  fd = fopen (binbookfile, RWA_ACC);
#else
	  fd = fopen (binbookfile, "r");
#endif
	  if (fd != NULL)
	    {
		fscanf (fd, "%d\n", &booksize);
		fscanf (fd, "%d\n", &bookcount);
		fscanf (fd, "%d\n", &bookpocket);
		bkalloc (booksize);
#if !defined BAREBONES
		sprintf (msg, CP[213], bookcount, booksize);
		ShowMessage (msg);
#endif
		if (0 > fread (OpenBook, sizeof (struct bookentry), booksize, fd))
		  {
		      perror ("fread");
		      exit (1);
		  }
		/* set every thing back to start game */
		Book = BOOKFAIL;
		for (i = 0; i < NO_SQUARES; i++)
		  {
		      board[i] = Stboard[i];
		      color[i] = Stcolor[i];
		  }
       	  	ClearCaptured ();
		/* InitializeStats (); Mutz, 2.2.93 */
		fclose (fd);

	    }
      }
    if ((fd = fopen (bookfile, "r")) == NULL)
	fd = fopen ("gnushogi.book", "r");
    if (fd != NULL)
      {

	  if (OpenBook == NULL)
	    {
		bkalloc (booksize);
		for (OB = OpenBook; OB < &OpenBook[booksize]; OB++)
		    OB->count = 0;
	    }
	  OC = NULL;
	  /* setvbuf(fd,buffr,_IOFBF,2048); */
	  side = black;
	  xside = white;
	  hashbd = hashkey = 0;
	  i = 0;

	  while ((c = Vparse (fd, &mv, side, opening, i)) >= 0)
	    {
		if (c == 1)
		  {

		      /*
		       * if not first move of an opening and first
		       * time we have seen it save next move as
		       * hint
		       */
		      i++;
		      if (i < bookmaxply + 2)
			{
			    if (i > 1 && OB->count == 1)
				OB->hint = mv & 0x7fff;
			    OC = OB;	/* save for end marking */
			    if (i < bookmaxply + 1)
			      {
				  doit = true;

				  /*
			           * see if this position and
			           * move already exist from
			           * some other opening
			           */

				  /*
			           * is this ethical, to offer
			           * the bad move as a
			           * hint?????
			           */
				  OB = BookTable[bhashkey & BOOKMASK];
				  while (OB->count)
				    {
					if (OB->bookkey == (unsigned long)bhashkey
					    && OB->bookbd == (unsigned long)bhashbd
					    && (OB->flags & SIDEMASK) == side
					    && OB->bmove == mv)
					  {

					      /*
					       * yes so * just bump * count - * count is * used to
					       * choose * opening * move in * proportion * to its
					       * presence * in the * book
					       */
					      doit = false;
					      OB->count++;
					      break;
					  }

					/*
				         * Book is hashed
				         * into BKTBLSIZE
				         * chunks based on
				         * hashkey
				         */
					if (++OB == OBEND)
					    OB = OpenBook;
				    }

				  /*
			           * doesn`t exist so add it to
			           * the book
			           */
				  if (doit)
				    {
					bookcount++;
					if (bookcount > (booksize - 2 * BKTBLSIZE))
					  {
					      printf ("booksize exceeded\n");
					      exit (1);
					  }
#if !defined BAREBONES
					if (bookcount % 1000 == 0)
					    printf ("%d processed\n", bookcount);
#endif
					OB->bookkey = (unsigned long)bhashkey;
					OB->bookbd = (unsigned long)bhashbd;
					OB->bmove = mv;
					OB->hint = 0;
					OB->count = 1;
					OB->flags = side;
				    }
			      }
			}
		      computer = opponent;
		      opponent = computer ^ 1;

		      xside = side;
		      side = side ^ 1;
		  }
		else if (i > 0)
		  {
		      /* reset for next opening */
		      RESET ();
		      i = 0;
		      side = black;
		      xside = white;
		      hashbd = hashkey = 0;

		  }
	    }
	  fclose (fd);
	  if (binbookfile != NULL)
	    {
#ifdef THINK_C
		fd = fopen (binbookfile, WA_ACC);
#else
		fd = fopen (binbookfile, "w");
#endif
		if (fd != NULL)
		  {
		      fprintf (fd, "%d\n%d\n%d\n", booksize, bookcount, bookpocket);
		      if (0 > fwrite (OpenBook, sizeof (struct bookentry), booksize, fd))
			    perror ("fwrite");
		      fclose (fd);
		      binbookfile = NULL;
		  }
	    }
#if !defined BAREBONES
	  sprintf (msg, CP[213], bookcount, booksize);
	  ShowMessage (msg);
#endif
	  /* set every thing back to start game */
	  Book = BOOKFAIL;
	  RESET ();
      }
    else if (OpenBook == NULL)
      {
#if !defined BAREBONES
	  if (!bookcount)
	      ShowMessage (CP[212]);
#endif
	  Book = 0;
      }
}


int
OpeningBook (unsigned short *	hint, short int side)

/*
 * Go thru each of the opening lines of play and check for a match with the
 * current game listing. If a match occurs, generate a random number. If this
 * number is the largest generated so far then the next move in this line
 * becomes the current "candidate". After all lines are checked, the
 * candidate move is put at the top of the Tree[] array and will be played by
 * the program. Note that the program does not handle book transpositions.
 */

{
    short pnt;
    unsigned short m;
    unsigned r, cnt, tcnt, ccnt;
    register struct bookentry *OB, *OC;
    int possibles = TrPnt[2] - TrPnt[1];

    gsrand ((unsigned int) time ((long *) 0));
    m = 0;
    cnt = 0;
    tcnt = 0;
    ccnt = 0;
    OC = NULL;


    /*
     * find all the moves for this position  - count them and get their
     * total count
     */
    OB = BookTable[hashkey & BOOKMASK];
    while (OB->count)
      {
	  if (OB->bookkey == (unsigned long)hashkey
	      && OB->bookbd == (unsigned long)hashbd
	      && ((OB->flags) & SIDEMASK) == side)
	    {
		if (OB->bmove & BADMOVE)
		  {
		      m = OB->bmove ^ BADMOVE;
		      /* is the move is in the MoveList */
		      for (pnt = TrPnt[1]; pnt < TrPnt[2]; pnt++)
			{
			    if (((Tree[pnt].f << 8) | Tree[pnt].t) == m)
			      {
				  if (--possibles)
				    {
					Tree[pnt].score = DONTUSE;
					break;
				    }
			      }
			}

		  }
		else
		  {
		      OC = OB;
		      cnt++;
		      tcnt += OB->count;
		  }
	    }
	  if (++OB == OBEND)
	      OB = OpenBook;
      }
    /* if only one just do it */
    if (cnt == 1)
      {
	  m = OC->bmove;
      }
    else
	/* more than one must choose one at random */
    if (cnt > 1)
      {
	  /* pick a number */
	  r = urand () % 1000;

	  OC = BookTable[hashkey & BOOKMASK];
	  while (OC->count)
	    {
		if (OC == OBEND)
		    OC = OpenBook;
		if (OC->bookkey == (unsigned long)hashkey
		    && OC->bookbd == (unsigned long)hashbd
		    && ((OC->flags) & SIDEMASK) == side
		    && !(OC->bmove & BADMOVE))
		  {
		      ccnt += OC->count;
		      if (((unsigned long) (ccnt * BOOKRAND) / tcnt) >= r)
			{
			    m = OC->bmove;
			    break;
			}
		  }
		if (++OC == OBEND)
		    OC = OpenBook;
	    }
      }
    else
      {
	  /* none decrement count of no finds */
	  Book--;
	  return false;
      }
    /* make sure the move is in the MoveList */
    for (pnt = TrPnt[1]; pnt < TrPnt[2]; pnt++)
      {
	  if (((Tree[pnt].f << 8) | Tree[pnt].t) == m)
	    {
		Tree[pnt].flags |= book;
		Tree[pnt].score = 0;
		break;
	    }
      }
    /* Make sure its the best */

    pick (TrPnt[1], TrPnt[2] - 1);
    if (Tree[TrPnt[1]].score)
      {
	  /* no! */
	  Book--;
	  return false;
      }
    /* ok pick up the hint and go */
    *hint = OC->hint;
    return true;
}



#endif
