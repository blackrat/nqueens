#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define HSIZE 8
#define VSIZE 8
#define NUMBEROFQUEENS 8

#define POPULATION 3000

typedef char *CODE;
typedef int  FITNESS;

typedef struct {
	CODE	code;
	FITNESS fitness;
}GENE;

static char individuals[POPULATION][NUMBEROFQUEENS+1];

static GENE gene[POPULATION];
static char board[HSIZE][VSIZE];
static int solutions=0;
static char *boards[512];
static long gen=0;

int main(void);
int comp(const void *f1, const void *f2);
void init (void);
void breed(void);
void crossover(GENE *father, GENE *mother, GENE *son, GENE *daughter);
void play_gene(GENE *g);
void finished(GENE *g);
void print_top(void);
int unique_board(GENE *g);
time_t start_time;

#define timesecs (60*60)

main()
{
time_t end_time;

	while(1)
	{
		init();
		end_time=time(&start_time)+timesecs;
		while (end_time>time(NULL) && solutions<60)
		{
			gen++;
			breed();
			if (gen==15) printf("Solutions in %d secs for pop of %d = %d in %ld generations\n",(int)(end_time-start_time),POPULATION,solutions,gen);
//			print_top();
		}
		end_time=time(NULL);
		printf("Solutions in %d secs for pop of %d = %d in %ld generations\n",(int)(end_time-start_time),POPULATION,solutions,gen);
	}
	return 0;
}

void
init()
{
	int i;
	gen=0;
	solutions=0;
	srand(time(NULL));
	for (i=0;i<POPULATION;i++)
	{
		int j;
		for (j=0;j<NUMBEROFQUEENS;j++)
		{
			individuals[i][j]=(char)((j*HSIZE)+rand()%(VSIZE));
		}
		gene[i].code=individuals[i];
		play_gene(&gene[i]);
	}
}

void
breed()
{
	int i;
	GENE *father,*mother,*son,*daughter;

	qsort(gene,POPULATION,sizeof(GENE),comp);
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

int comp(const void *f1, const void *f2)
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
/*	if ((unsigned)rand()<328)
	{
	char *tmp;

		tmp=son->code+rand()%NUMBEROFQUEENS;
		*tmp=rand()%HSIZE*VSIZE;
	}
*/
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
		xpos=g->code[i]%HSIZE;
		ypos=g->code[i]/HSIZE;
		board[xpos][ypos]++;
	}
	for (i=0;i<NUMBEROFQUEENS;i++)
	{
		score=12;
		xpos=g->code[i]%HSIZE;
		ypos=g->code[i]/HSIZE;
		for (j=0;j<HSIZE;j++)
		{
			if (board[j][ypos])
			{
				score-=(int)board[j][ypos];
			}
		}
		for (j=0;j<VSIZE;j++)
		{
			if (board[xpos][j])
			{
				score-=(int)board[xpos][j];
			}
		}
		j=xpos;
		k=ypos;
		while(j && k)
		{
			if (board[j][k])
			{
				score-=board[j][k];
			}
			j--;
			k--;
		}
		j=xpos;
		k=ypos;
		while(j<HSIZE && k<VSIZE)
		{
			if (board[j][k])
			{
				score-=board[j][k];
			}
			j++;
			k++;
		}
		j=xpos;
		k=ypos;
		while(j && k<VSIZE)
		{
			if (board[j][k])
			{
				score-=board[j][k];
			}
			j--;
			k++;
		}
		j=xpos;
		k=ypos;
		while(j<HSIZE && k)
		{
			if (board[j][k])
			{
				score-=board[j][k];
			}
			j++;
			k--;
		}
		if (score==8)
		{
			score*=2;
		}

		score*=8;
		g->fitness+=score;
//		if (score==4) g->fitness++;
/*	if (score<4)
	{
		break;
	}
*/	}
//	g->fitness=score;
	if (g->fitness==544)
	{
		if (unique_board(g))
		{
		//finished(g);
		time(&curr_time);
		printf("Solution %d in %d secs and %ld generations\n",solutions,(int)(curr_time-start_time),gen);
		}
//		else g->fitness=0;
	}
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
finished(GENE *g)
{
int i,j;

	printf("Solutions so far: %d\t",solutions);
	printf("Fitness: %d\n",g->fitness);
	for (i=0;i<VSIZE;i++)
	{
		for(j=0;j<HSIZE;j++)
		{
			if (board[i][j]) printf("Q");
			else printf(".");
		}
		printf("\n");
	}
}

void
print_top()
{
	int i,j;
	printf("Generation %ld\tTop score: %d\n",gen++,gene[0].fitness);
	if (kbhit())
	{
		if (getch()==0x1b) exit(1);
		play_gene(gene);
		finished(gene);
	}
}