/*Author: Hunter Esler
 * Course: CSCI 4330
 * Lab number: Lab 1
 * Purpose: This lab will use mpich to calculage integral of sqrt(4-x^2) from 0 to 2 using trapezoidal rule
 * Due date: 3/29/2019
 * */

#include <stdio.h>
#include <mpi.h>
#include <math.h>

//returns f(x) at position x
double func(double x) {
	return (sqrt(4-x*x));
}

int main(int argc, char * argv[]) {
	int nproc, myrank;
	int source, dest;
	double step, left, right;
	double sum, area;
	MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nproc);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

	//source and dest for send/recv for ring communication
	source = (myrank+nproc-1)%nproc;
	dest = (myrank+1)%nproc;
	
	//step, and left/right side of trapezoid
	step = 2.0 / nproc;
	left = step * myrank;
	right = step * (myrank + 1);
	
	//area of a trapezoid
	area = ((func(left)+func(right))*step)/2.0;

	if (myrank == 0) {
		MPI_Send(&area, 1, MPI_DOUBLE, dest, 1, MPI_COMM_WORLD);
		MPI_Recv(&sum, 1, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, &status);
		printf("sum is %f\n", sum);
	} else {

		MPI_Recv(&sum, 1, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, &status);
		sum = sum + area;
		MPI_Send(&sum, 1, MPI_DOUBLE, dest, 1, MPI_COMM_WORLD);

	}

	MPI_Finalize();

	return 0;
}
