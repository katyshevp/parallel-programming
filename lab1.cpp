#include "mpi.h"
#include <iostream>
#include <ctime>
#include <fstream>

using namespace std;

int  curr_rank_proc;
int  num_of_procs;
double  time_seq_work_alg = 0;
double  time_pp_work_alg = 0;

double* Create_and_Init_Vector(int size) {
	double* vec = new double[size];
	srand(time(NULL));
	for (int i = 0; i < size; i++) {
		vec[i] = rand() % 1000 + 0.5 * i;
	}
	return vec;
}

void Show_Vec(double* vec, int size) {
	if (size < 100) {
		cout << "-----------------------------------" << endl;
		cout << "Vector:" << endl;
		cout << "-----------------------------------" << endl;
		for (int i = 0; i < size; i++)
			cout << vec[i] << endl;
	}
	else
	{
		ofstream file("vec.txt");
		for (int i = 0; i < size; i++) {
			file << vec[i] << endl;
		}
		file.close();
	}
}

int main(int argc, char* argv[]) {
	int size_vec = 1000;
	double* vec = NULL;
	double average_seq = 0;
	double average_par = 0;

	double end_time_of_seq_alg = 0;
	double start_time_of_seq_alg = 0;
	double end_time_of_pp_alg = 0;
	double start_time_of_pp_alg = 0;

	double part_sum = 0, temp_sum = 0;

	int size_work_of_proc = 0;

	MPI_Status stat;

	MPI_Init(&argc, &argv);


	MPI_Comm_size(MPI_COMM_WORLD, &num_of_procs);

	MPI_Comm_rank(MPI_COMM_WORLD, &curr_rank_proc);

	if (num_of_procs < 1)
	{
		cout << "Incorrect number of processes (at least 1 must be)" << endl;
		return 0;
	}

	if (curr_rank_proc == 0) {
		cout << "Input size of vector:";
		cin >> size_vec;
		vec = Create_and_Init_Vector(size_vec);

		if (vec == NULL)
		{
			cout << "Incorrect input data, try again" << endl;
			return 0;
		}

		Show_Vec(vec, size_vec);

		start_time_of_seq_alg = MPI_Wtime();

		for (int i = 0; i < size_vec; i++) {
			average_seq += vec[i];
		}
		average_seq /= size_vec;
		end_time_of_seq_alg = MPI_Wtime();
		time_seq_work_alg = end_time_of_seq_alg - start_time_of_seq_alg;

		cout << "Average in sequence algorithm:" << average_seq << endl;
		cout << "Spent time on the implementation of this algorithm (Sequence version)" << time_seq_work_alg << " ms " << endl;
		cout << endl;
		cout << "Num of procs: " << num_of_procs << endl;



		start_time_of_pp_alg = MPI_Wtime();

		size_work_of_proc = size_vec / num_of_procs;

		MPI_Bcast(&size_vec, 1, MPI_INT, 0, MPI_COMM_WORLD);
		for (int i = 1; i < num_of_procs; i++)
			MPI_Send(vec + size_work_of_proc * (i - 1), size_work_of_proc, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);

		//cout << "Process with rang " << curr_rank_proc << " start own job" << endl;

		for (int i = size_work_of_proc * (num_of_procs - 1); i < size_vec; i++)
			temp_sum += vec[i];

		average_par = temp_sum;

		MPI_Reduce(&part_sum, &temp_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

		average_par += temp_sum;
		average_par /= size_vec;

		end_time_of_pp_alg = MPI_Wtime();

		time_pp_work_alg = end_time_of_pp_alg - start_time_of_pp_alg;

		cout << "Average in parallel algorithm:" << average_par << endl;
		cout << "Spent time on the implementation of this algorithm (Parallel version)" << time_pp_work_alg << " ms" << endl;

		if (average_par == average_seq)
			cout << "Results of parallel and sequence versions are identical! " << endl;
		else
			cout << "Results of parallel and sequence versions are not identical! " << endl;

		if (time_pp_work_alg <= time_seq_work_alg)
			cout << "Parallel version faster, then sequence" << endl;
		else
			cout << "Sequence version faster, then parallel" << endl;

		delete[] vec;
	}
	else
	{
		double* recv_vector;
		MPI_Bcast(&size_vec, 1, MPI_INT, 0, MPI_COMM_WORLD);
		size_work_of_proc = size_vec / num_of_procs;
		recv_vector = new double[size_work_of_proc];
		MPI_Recv(recv_vector, size_work_of_proc, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, &stat);

		//cout << "Process with rang " << curr_rank_proc << " start own job" << endl;

		for (int i = 0; i < size_work_of_proc; i++) {
			part_sum += recv_vector[i];
		}

		MPI_Reduce(&part_sum, &temp_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
		delete[] recv_vector;
	}
	MPI_Finalize();
	return 0;
}