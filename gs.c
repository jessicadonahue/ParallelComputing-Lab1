#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/***** Globals ******/
float **a; /* The coefficients */
float *x;  /* The unknowns */
float *b;  /* The constants */
float err; /* The absolute relative error */
int num = 0;  /* number of unknowns */
int done = 0;

/****** Function declarations */
void check_matrix(); /* Check whether the matrix will converge */
void get_input();  /* Read input from file */
/********************************/



/* Function definitions: functions are ordered alphabetically ****/
/*****************************************************************/

int check_error (float *newX) {
	
	if (done == 1) return 0;
	int check = 0;

	int i = 0;
	float difference;
	float absError;

	for (i = 0; i < num; i++) {
		difference = ( newX[i] - x[i]) /newX[i] ;

		absError = fabs(difference) *100;

		/*
		printf("new: %f\n", newX[i]);
		printf("x: %f\n", x[i]);

	

                printf("err: %f\n", err);

		printf("error: %f\n", absError);

		*/

		if (absError > err) {
			check = 1;
			return check;
		}

	}
	//this means we are returning false --> ends the loop
	return check;


}

float newValues(int index) {
    
    int i;
    float sum = 0;

   
    for (i = 0; i < num; i++) {
        
        if (i != index) {
            sum+= a[index][i] * x[i];
            
        }
    }
   /* printf("sum: %f\n", sum); */
    float new = (b[index] - sum) / a[index][index];
 

    return new;
}


/* 
   Conditions for convergence (diagonal dominance):
   1. diagonal element >= sum of all other elements of the row
   2. At least one diagonal element > sum of all other elements of the row
 */
void check_matrix()
{
  int bigger = 0; /* Set to 1 if at least one diag element > sum  */
  int i, j;
  float sum = 0;
  float aii = 0;
  
  for(i = 0; i < num; i++)
  {
    sum = 0;
    aii = fabs(a[i][i]);
    
    for(j = 0; j < num; j++)
       if( j != i)
   sum += fabs(a[i][j]);
       
    if( aii < sum)
    {
      printf("The matrix will not converge\n");
      exit(1);
    }
    
    if(aii > sum)
      bigger++;
    
  }
  
  if( !bigger )
  {
     printf("The matrix will not converge\n");
     exit(1);
  }
}


/******************************************************/
/* Read input from file */
void get_input(char filename[])
{
  FILE * fp;
  int i,j;  
 
  fp = fopen(filename, "r");
  if(!fp)
  {
    printf("Cannot open file %s\n", filename);
    exit(1);
  }

 fscanf(fp,"%d ",&num);
 fscanf(fp,"%f ",&err);

 /* Now, time to allocate the matrices and vectors */
 a = (float**)malloc(num * sizeof(float*));
 if( !a)
  {
  printf("Cannot allocate a!\n");
  exit(1);
  }

 for(i = 0; i < num; i++) 
  {
    a[i] = (float *)malloc(num * sizeof(float)); 
    if( !a[i])
    {
    printf("Cannot allocate a[%d]!\n",i);
    exit(1);
    }
  }
 
 x = (float *) malloc(num * sizeof(float));
 if( !x)
  {
  printf("Cannot allocate x!\n");
  exit(1);
  }



 b = (float *) malloc(num * sizeof(float));
 if( !b)
  {
  printf("Cannot allocate b!\n");
  exit(1);
  }

 /* Now .. Filling the blanks */ 

 /* The initial values of Xs */
 for(i = 0; i < num; i++)
  fscanf(fp,"%f ", &x[i]);
 
 for(i = 0; i < num; i++)
 {
   for(j = 0; j < num; j++)
     fscanf(fp,"%f ",&a[i][j]);
   
   /* reading the b element */
   fscanf(fp,"%f ",&b[i]);
 }
 
 fclose(fp); 

}


/************************************************************/


int main(int argc, char *argv[])
{

 int i;
 int nit = 0; /* number of iterations */
 int n;
 float error = 1000;

float * newX = (float *) malloc(num * sizeof(float));

/*initialize new communicators*/
MPI_Comm USE;
MPI_Comm DONT;
  
 if( argc != 2)
 {
   printf("Usage: gsref filename\n");
   exit(1);
 }
  
 /* Read the input file and fill the global data structure above */ 
 get_input(argv[1]);
 
 /* Check for convergence condition */
 check_matrix();

 /* mpi stuff */

 
  int source;
 int        my_rank;               /* My process rank        */
int comm_sz;

double MPI_Wtime(void);
double startTime, finishTime;

startTime = MPI_Wtime();

 /* Let the system do what it needs to start up MPI */
   MPI_Init(&argc, &argv);

   /* Get my process rank */
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
 MPI_Comm_size (MPI_COMM_WORLD, &comm_sz);

int count[comm_sz];
int recieve[comm_sz];
int displacement[comm_sz];

int amount = num / comm_sz;
int extra = num % comm_sz;
int displace = 0;

	if (my_rank <  num) {
		
		MPI_Comm_split(MPI_COMM_WORLD, 0, 0, &USE);

		
	}

	else {
		MPI_Comm_split(MPI_COMM_WORLD, 1, 0, &DONT);

	}

/*Now we are going to fill the count array with the num of unknowns */
int c;
for (c = 0; c < comm_sz; c++) {
	if (c < extra) {
		count[c] = amount + 1;
	}

	else {
		count[c] = amount;
	}

	recieve[c] = count[c];

	displacement[c] = displace;
	displace+= count[c];
}

	

/*  printf ("Hello from process %i of %i\n", my_rank, comm_sz);
  */



float * gatherArray = malloc(num* sizeof(float));

int o = 0;
//lets put all old x values in old 
for (o = 0; o < num; o++) {
	newX[o] = x[o];
}

do {

	nit++;
	int j;
	for (j = 0; j < num; j++) {
		x[j] = newX[j];

	}



/* we need the number of unknowns for each process */
int localNum = (int)ceil( (double) num/comm_sz  );

float *localArray = (float *) malloc(localNum * sizeof(float));
int k, m;
for (k = 0; k < count[my_rank]; k++) {
	int newIndex = k;
	for (m = 0; m < my_rank; m++) {
	
		newIndex = newIndex + count[m];
	}

	localArray[k] = newValues(newIndex);
	
	/*printf("local %f\n", localArray[k]);
	*/
}




	MPI_Allgatherv(localArray, count[my_rank], MPI_FLOAT, newX, recieve, displacement, MPI_FLOAT, USE); 


if (done == 1) {
break;
}
   

}

while ( check_error(newX) );

   MPI_Barrier(USE);



 if (my_rank == 0) {
 /*  to the stdout */
 /* Keep that same format */
 	for( i = 0; i < num; i++)
   	printf("%f\n",x[i]);
 
 	printf("total number of iterations: %d\n", nit);
	
	done = 1;

finishTime = MPI_Wtime();

   printf("Elapsed time = %e seconds\n", finishTime - startTime);


	//free(x);
	//free(a);
	//free(b);	
}


/* Shut down MPI */

   MPI_Finalize();

   
   return 0;
 
 exit(0);

}
