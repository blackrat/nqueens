/* Timings:
	Recursive:
	25	1071
	24
	23
	22
	21
	20
	19
	18
	17
	16
	15
	14
	13
	12
	11
	10
	9
	8
	7
	6
	5
	4
 */


#ifdef __BORLANDC__
	#include <conio.h>
#else
	#include <pc.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define getch getchar

#define HSIZE (24)
#define VSIZE HSIZE
#define NUMBEROFQUEENS HSIZE

#define POPULATION (2400)
#define MAXSOLNS (92)

#ifdef RANDSEED
	#define SEED time(NULL)
#else
	#define SEED NULL
#endif


/*#define GENETIC*/
#ifdef GENETIC
	#define MUTSON		/* Chance of Son mutating */
/*	#define MUTDAUGHTER /* Chance of Daughter mutating */
/*	#define FITTEST	/* Already found solution has weight of zero */
#else
	#define RECURSIVE
#endif

typedef char *CODE;
typedef int  FITNESS;

typedef struct {
	CODE	code;
	FITNESS fitness;
}GENE;

static char individuals[POPULATION][NUMBEROFQUEENS+1];

static GENE gene[POPULATION];
static char board[HSIZE][VSIZE];
static char cum_board[HSIZE][VSIZE];
static int solutions=0;
static char *boards[MAXSOLNS];
static long gen=0;

int main(void);
int compar(const void *f1, const void *f2);
void init (void);
void breed(void);
void crossover(GENE *father, GENE *mother, GENE *son, GENE *daughter);
void play_gene(GENE *g);
void finished(GENE *g,int soln);
void show_board(int soln);
void show_cum_board(void);
void print_top(void);
int unique_board(GENE *g);
void place_queens(int i);
int get_score(int xpos, int ypos);
void qsort(void *base,int n,unsigned size,int (*compar)(void*,void*));
static qst(char *base,char *max);

time_t start_time;

#define timesecs (60)

main()
{
time_t end_time;

	end_time=time(&start_time)+timesecs;
	init();
#ifdef GENETIC
	while (solutions<MAXSOLNS && end_time>time(NULL))
	{
		gen++;
		breed();
/*		if (gen==15) printf("Solutions in %d secs for pop of %d = %d in %ld generations\n",(int)(end_time-start_time),POPULATION,solutions,gen);
*/
		print_top();
	}
#endif
#ifdef RECURSIVE
	place_queens(0);
#endif
	end_time=time(NULL);
	printf("Solutions in %d secs for pop of %d = %d in %ld generations\n",(int)(end_time-start_time),POPULATION,solutions,gen);
	show_cum_board();
	return 0;
}

void
init()
{
	int i;
	gen=0;
	solutions=0;
	memset(&cum_board,0,HSIZE*VSIZE);
	memset(&board,0,HSIZE*VSIZE);
#ifdef GENETIC
	srand(SEED);
	for (i=0;i<POPULATION;i++)
	{
		int j;
		for (j=0;j<NUMBEROFQUEENS;j++)
		{
			individuals[i][j]=(char)(rand()%(HSIZE));
		}
		gene[i].code=individuals[i];
		play_gene(&gene[i]);
	}
#endif
}

static  int	(*testcmp)(void *,void *);	/* the comparison routine */

void
breed()
{
	int i;
	GENE *father,*mother,*son,*daughter;

	testcmp=compar;
	qsort(gene,POPULATION,sizeof(GENE),testcmp);
	for (i=POPULATION/2;i<POPULATION;i+=2)
	{
		father=&gene[rand()%(POPULATION/2)];
		mother=&gene[rand()%(POPULATION/2)];
		son=&gene[i];
		daughter=&gene[i+1];
		crossover(father,mother,son,daughter);
		play_gene(son);
		play_gene(daughter);
	}
}

int compar(const void *f1, const void *f2)
{
	return (((GENE *)f2)->fitness - ((GENE *)f1)->fitness);
}

void
crossover(GENE *father, GENE *mother, GENE *son, GENE *daughter)
{
	int split=rand()%(NUMBEROFQUEENS);
	memcpy(son->code,father->code,NUMBEROFQUEENS);
	memcpy(daughter->code,mother->code,NUMBEROFQUEENS);
	memcpy(son->code+split,mother->code+split,NUMBEROFQUEENS-split);
	memcpy(daughter->code+split,father->code+split,NUMBEROFQUEENS-split);
	if ((unsigned)rand()<328)
	{
#ifndef MUTSON
		rand();
		rand();
#else
	char *tmp;

		tmp=son->code+(char)(rand()%NUMBEROFQUEENS);
		*tmp=(char)(rand()%HSIZE);
#endif
	}
	if ((unsigned)rand()<328)
	{
#ifndef MUTDAUGHTER
		rand();
		rand();
#else
	char *tmp;

		tmp=daughter->code+(char)(rand()%NUMBEROFQUEENS);
		*tmp=(char)(rand()%HSIZE);
#endif
	}

}

void
play_gene(GENE *g)
{
	int i,j,k,score,xpos,ypos;
	time_t curr_time;

	memset(&board,0,HSIZE*VSIZE);
	g->fitness=0;
	for (i=0;i<NUMBEROFQUEENS;i++)
	{
		xpos=i;
		ypos=g->code[i];
		board[xpos][ypos]++;
	}
	for (i=0;i<HSIZE;i++)
	{
		xpos=i;
		ypos=g->code[i];
		score=get_score(xpos,ypos);
		if (score==NUMBEROFQUEENS)
		{
			score*=2;
		}
		score*=VSIZE;
		g->fitness+=score;
	}
	if (g->fitness==NUMBEROFQUEENS*HSIZE*VSIZE*2)
	{
		if (unique_board(g))
		{
			finished(g,1);
			time(&curr_time);
			printf("Solution %d in %d secs and %ld generations\n",solutions,(int)(curr_time-start_time),gen);
			if (solutions<2)
			{
/*				while(!kbhit()){}
				getch();
*/			}
		}
#ifdef FITTEST
		g->fitness=0;
#endif
	}
}

int get_score(int xpos, int ypos)
{
int score,j,k;

	score=NUMBEROFQUEENS;
	for (j=0;j<HSIZE;j++) if (board[j][ypos] && j!=xpos) score--;
	for (j=0;j<VSIZE;j++) if (board[xpos][j] && j!=ypos) score--;
	j=xpos-1;
	k=ypos-1;
	while(j>=0 && k>=0)
	{
		if (board[j][k]) score--;
		j--;
		k--;
	}
	j=xpos+1;
	k=ypos+1;
	while(j<HSIZE && k<VSIZE)
	{
		if (board[j][k]) score--;
		j++;
		k++;
	}
	j=xpos-1;
	k=ypos+1;
	while(j>=0 && k<VSIZE)
	{
		if (board[j][k]) score--;
		j--;
		k++;
	}
	j=xpos+1;
	k=ypos-1;
	while(j<HSIZE && k>=0)
	{
		if (board[j][k]) score--;
		j++;
		k--;
	}
	return score;
}

int unique_board(GENE *g)
{
int i;
	for (i=0;i<solutions;i++)
	{
		if (!memcmp(g->code,boards[i],NUMBEROFQUEENS)) return 0;
	}
	boards[solutions]=(char *)malloc(NUMBEROFQUEENS+1);
	memcpy(boards[solutions++],g->code,NUMBEROFQUEENS+1);
	return 1;
}

void
finished(GENE *g,int soln)
{
	printf("Solutions so far: %d\t",solutions);
	printf("Fitness: %d of %d\n",g->fitness,NUMBEROFQUEENS*HSIZE*VSIZE*2);
	show_board(soln);
}

void show_board(int soln)
{
int i,j;

	for (i=0;i<VSIZE;i++)
	{
		for(j=0;j<HSIZE;j++)
		{
			if (board[i][j])
			{
				if (soln) cum_board[i][j]++;
				printf("Q");
			}
			else printf(".");
		}
		printf("\n");
	}
/*
	while (!kbhit()){}
	getch();
*/
}

void show_cum_board()
{
int i,j;

	for (i=0;i<VSIZE;i++)
	{
		for(j=0;j<HSIZE;j++)
		{
			printf("%d ",cum_board[i][j]);
		}
		printf("\n");
	}
/*
	while (!kbhit()){}
	getch();
*/
}


void
print_top()
{
	int i,j;
	printf("Generation %ld\tTop score: %d\n",gen,gene[0].fitness);
/*
	if (kbhit())
	{
		if ((char)getch()==0x1b) exit(1);
		play_gene(gene);
		finished(gene,0);
	}
*/
}

void
place_queens(int i)
{
	time_t soltime;
	int j;
	for (j=0;j<VSIZE;j++)
	{
		if (get_score(i,j)==NUMBEROFQUEENS)
		{
			board[i][j]++;
			if (i<HSIZE-1) place_queens(i+1);
			else
			{
				time(&soltime);
				solutions++;

				printf("Solution %d\n",solutions);
				show_board(1);
				printf("with %ld backtraces in %d secs\n",gen,(int)(soltime-start_time));
				getchar();

/*				while (!kbhit()){}
				getch();
*/
			}
			gen++;
			printf("Backtrace %ld\n",gen);
			board[i][j]--;
		}
	}
}