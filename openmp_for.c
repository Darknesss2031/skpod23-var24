/*********************************************************************
      You can adjust the size of this benchmark code to fit your target
      computer.  In that case, please chose following sets of 
      (mimax,mjmax,mkmax):
       small : 129,65,65
       midium: 257,129,129
       large : 513,257,257
       ext.large: 1025,513,513
      This program uses a kernel which appears in a linear solver of pressure 
      Poisson included in an incompressible Navier-Stokes solver. 
      A point-Jacobi method is employed in this solver.
      A,B,C:coefficient matrix, wrk1: source term of Poisson equation
      wrk2 : working area, OMEGA : relaxation parameter
      BND:control variable for boundaries and objects ( = 0 or 1)
      P: pressure 
*********************************************************************/

#include <stdio.h>
#include <omp.h>
#include <stdlib.h>

#define SMALL

#ifdef SMALL
#define MIMAX            129
#define MJMAX            65
#define MKMAX            65
#endif

#ifdef MIDDLE
#define MIMAX            257
#define MJMAX            129
#define MKMAX            129
#endif

#ifdef LARGE
#define MIMAX            513
#define MJMAX            257
#define MKMAX            257
#endif

#ifdef EXTLARGE
#define MIMAX            1025
#define MJMAX            513
#define MKMAX            513
#endif

#define NN               200

double second();
double jacobi(int);
void initmt();

static double  p[MIMAX][MJMAX][MKMAX];
static double  a[MIMAX][MJMAX][MKMAX][4],
              b[MIMAX][MJMAX][MKMAX][3],
              c[MIMAX][MJMAX][MKMAX][3];
static double  bnd[MIMAX][MJMAX][MKMAX];
static double  wrk1[MIMAX][MJMAX][MKMAX],
              wrk2[MIMAX][MJMAX][MKMAX];

static int imax, jmax, kmax;
static double omega;

int
main(int argc, char **argv)
{
  int i, j, k;
  double gosa;
  double cpu0, cpu1, nflop, xmflops2, score;
  omp_set_num_threads(strtol(argv[1], NULL, 10));
  omega = 0.8;
  imax = MIMAX-1;
  jmax = MJMAX-1;
  kmax = MKMAX-1;

  /*
   *    Initializing matrixes
   */
  initmt();

  printf("mimax = %d mjmax = %d mkmax = %d\n",MIMAX, MJMAX, MKMAX);
  printf("imax = %d jmax = %d kmax =%d\n",imax,jmax,kmax);

  /*
   *    Start measuring
   */
  cpu0 = second();

  /*
   *    Jacobi iteration
   */

  gosa = jacobi(NN);
  
  cpu1 = second();

  nflop = (kmax-2)*(jmax-2)*(imax-2)*34;

  if(cpu1 != 0.0)
    xmflops2 = nflop/cpu1*1.0e-6*(double)NN;

  score = xmflops2/32.27;
  
  printf("cpu : %f sec.\n", cpu1);
  printf("Loop executed for %d times\n",NN);
  printf("Gosa : %e \n",gosa);
  printf("MFLOPS measured : %f\n",xmflops2);
  printf("Score based on MMX Pentium 200MHz : %f\n",score);
  
  return (0);
}

void
initmt()
{
	int i,j,k;
  #pragma omp parallel for private(i,j,k)
  for(i=0 ; i<imax ; ++i)
    for(j=0 ; j<jmax ; ++j)
      for(k=0 ; k<kmax ; ++k){
        a[i][j][k][0]=0.0;
        a[i][j][k][1]=0.0;
        a[i][j][k][2]=0.0;
        a[i][j][k][3]=0.0;
        b[i][j][k][0]=0.0;
        b[i][j][k][1]=0.0;
        b[i][j][k][2]=0.0;
        c[i][j][k][0]=0.0;
        c[i][j][k][1]=0.0;
        c[i][j][k][2]=0.0;
        p[i][j][k]=0.0;
        wrk1[i][j][k]=0.0;
        bnd[i][j][k]=0.0;
      }
  #pragma omp parallel for private(i,j,k)
  for(i=0 ; i<imax ; ++i)
    for(j=0 ; j<jmax ; ++j)
      for(k=0 ; k<kmax ; ++k){
        a[i][j][k][0]=1.0;
        a[i][j][k][1]=1.0;
        a[i][j][k][2]=1.0;
        a[i][j][k][3]=1.0/6.0;
        b[i][j][k][0]=0.0;
        b[i][j][k][1]=0.0;
        b[i][j][k][2]=0.0;
        c[i][j][k][0]=1.0;
        c[i][j][k][1]=1.0;
        c[i][j][k][2]=1.0;
        p[i][j][k]=(double)(k*k)/(double)((kmax-1)*(kmax-1));
        wrk1[i][j][k]=0.0;
        bnd[i][j][k]=1.0;
      }
}

double
jacobi(int nn)
{
  int i,j,k,n;
  double gosa, s0, ss;
  #pragma omp parallel for private(n,i,j,k)
  for(n=0;n<nn;++n){
    gosa = 0.0;

    for(i=1 ; i<imax-1 ; ++i)
      for(j=1 ; j<jmax-1 ; ++j)
        for(k=1 ; k<kmax-1 ; ++k){
          s0 = a[i][j][k][0] * p[i+1][j  ][k  ]
             + a[i][j][k][1] * p[i  ][j+1][k  ]
             + a[i][j][k][2] * p[i  ][j  ][k+1]
             + b[i][j][k][0] * ( p[i+1][j+1][k  ] - p[i+1][j-1][k  ]
                               - p[i-1][j+1][k  ] + p[i-1][j-1][k  ] )
             + b[i][j][k][1] * ( p[i  ][j+1][k+1] - p[i  ][j-1][k+1]
                               - p[i  ][j+1][k-1] + p[i  ][j-1][k-1] )
             + b[i][j][k][2] * ( p[i+1][j  ][k+1] - p[i-1][j  ][k+1]
                               - p[i+1][j  ][k-1] + p[i-1][j  ][k-1] )
             + c[i][j][k][0] * p[i-1][j  ][k  ]
             + c[i][j][k][1] * p[i  ][j-1][k  ]
             + c[i][j][k][2] * p[i  ][j  ][k-1]
             + wrk1[i][j][k];

          ss = ( s0 * a[i][j][k][3] - p[i][j][k] ) * bnd[i][j][k];

          gosa = gosa + ss*ss;

          wrk2[i][j][k] = p[i][j][k] + omega * ss;
        }
    
    for(i=1 ; i<imax-1 ; ++i)
      for(j=1 ; j<jmax-1 ; ++j)
        for(k=1 ; k<kmax-1 ; ++k)
          p[i][j][k] = wrk2[i][j][k];
  } /* end n loop */

  return(gosa);
}

double
second()
{
#include <sys/time.h>

  struct timeval tm;
  double t ;

  static int base_sec = 0,base_usec = 0;

  gettimeofday(&tm, NULL);
  
  if(base_sec == 0 && base_usec == 0)
    {
      base_sec = tm.tv_sec;
      base_usec = tm.tv_usec;
      t = 0.0;
  } else {
    t = (double) (tm.tv_sec-base_sec) + 
      ((double) (tm.tv_usec-base_usec))/1.0e6 ;
  }

  return t ;
}
