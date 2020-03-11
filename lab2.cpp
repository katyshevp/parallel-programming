#include "mpi.h"

#include <iostream>
#include <stdlib.h>

using namespace std;

int* serialSortBubble;
int* parallelSortBubble;
double serialStart;
double serialTotalTime;
double parallelStart;
double parallelTotalTime;

void generateData(int*& myArray, int arraySize, int seed)
{
	if (myArray != 0)
	{
		delete[] myArray;
	}
	myArray = new int[arraySize];

	srand(seed);
	for (int i = 0; i < arraySize; i++)
	{
		myArray[i] = rand() % 100;
	}
}

void bubbleSort(int* a, int size)
{
	for (int i = 0; i < size; i++)
	{
		for (int j = size - 1; j > i; j--)
		{
			if (a[j - 1] > a[j])
			{
				int x;
				x = a[j - 1];
				a[j - 1] = a[j];
				a[j] = x;
			}
		}
	}
}
void printArray(int* a, int size)
{
	cout << "\nArray\n";
	for (int i = 0; i < size; i++)
	{
		cout << a[i];
	}
}
void merge(int*& mergeArray, int* a1, int size1, int* a2, int size2)
{
	if (mergeArray != 0)
		delete[] mergeArray;
	mergeArray = new int[size1 + size2];

	int i, j, k;
	i = j = k = 0;
	while (i < size1 && j < size2)
	{
		if (a1[i] < a2[j])
		{
			mergeArray[k] = a1[i];
			i++;
		}
		else
		{
			mergeArray[k] = a2[j];
			j++;
		}
		k++;
	}
	if (i == size1)
	{
		while (j < size2)
		{
			mergeArray[k] = a2[j];
			j++;
			k++;
		}
	}
	if (j == size2)
	{
		while (i < size1)
		{
			mergeArray[k] = a1[i];
			i++;
			k++;
		}
	}
}
void exchangeArray(int ProcRank, int anotherProcRank, int& blockSize, int*& array)
{
	MPI_Status Status;
	MPI_Send(&blockSize, 1, MPI_INT, anotherProcRank, 0, MPI_COMM_WORLD);
	MPI_Send(array, blockSize, MPI_INT, anotherProcRank, 0, MPI_COMM_WORLD);
	int anotherProcBlockSize;

	MPI_Recv(&anotherProcBlockSize, 1, MPI_INT, anotherProcRank, 0, MPI_COMM_WORLD, &Status);
	int* anotherProcArray = new int[anotherProcBlockSize];
	MPI_Recv(anotherProcArray, anotherProcBlockSize, MPI_INT, anotherProcRank, 0, MPI_COMM_WORLD, &Status);

	int* mergeArray = 0;

	merge(mergeArray, array, blockSize, anotherProcArray, anotherProcBlockSize);
	int newBlockSize = (blockSize + anotherProcBlockSize) / 2;
	if (array != 0)
		delete[] array;
	if (ProcRank < anotherProcRank)
	{
		array = new int[newBlockSize];
		for (int i = 0; i < newBlockSize; i++)
		{
			array[i] = mergeArray[i];
		}
		blockSize = newBlockSize;
	}
	else
	{
		array = new int[blockSize + anotherProcBlockSize - newBlockSize];
		for (int i = 0; i < blockSize + anotherProcBlockSize - newBlockSize; i++)
		{
			array[i] = mergeArray[i + newBlockSize];
		}
		blockSize = blockSize + anotherProcBlockSize - newBlockSize;
	}

	delete[] anotherProcArray;
	delete[] mergeArray;
}
int main(int argc, char* argv[])
{
	int arraySize = atoi(argv[1]);
	int seed = atoi(argv[2]);

	int procBlockSize = 0;
	int* procArray = 0;

	int ProcRank, ProcNum;
	MPI_Status Status;
	
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
	MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);
	if (ProcRank == 0)
	{
		cout << "\n\n**************************************************************\n";
		generateData(serialSortBubble, arraySize, seed);

		serialStart = MPI_Wtime();
		bubbleSort(serialSortBubble, arraySize);
		serialTotalTime = MPI_Wtime() - serialStart;
		cout << "Time(Serial realization):" << serialTotalTime << '\n';
	}

	if (ProcRank == 0)
	{
		int* myArray = 0;

		generateData(myArray, arraySize, seed);

		parallelStart = MPI_Wtime();
		int blockSize = arraySize / ProcNum;
		for (int i = 1; i < ProcNum - 1; i++)
		{
			int start = blockSize * i;
			MPI_Send(&blockSize, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			MPI_Send(myArray + start, blockSize, MPI_INT, i, 0, MPI_COMM_WORLD);
		}

		if (ProcNum > 1)
		{
			int start = blockSize * (ProcNum - 1);
			int end = arraySize;
			int lastBlockSize = end - start;
			MPI_Send(&lastBlockSize, 1, MPI_INT, ProcNum - 1, 0, MPI_COMM_WORLD);
			MPI_Send(myArray + start, lastBlockSize, MPI_INT, ProcNum - 1, 0, MPI_COMM_WORLD);
		}
		procBlockSize = blockSize;
		procArray = new int[procBlockSize];
		for (int i = 0; i < procBlockSize; i++)
		{
			procArray[i] = myArray[i];
		}

		delete[] myArray;
	}

	if (ProcRank != 0)
	{
		MPI_Recv(&procBlockSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &Status);
		procArray = new int[procBlockSize];
		MPI_Recv(procArray, procBlockSize, MPI_INT, 0, 0, MPI_COMM_WORLD, &Status);
	}

	bubbleSort(procArray, procBlockSize);

	for (int i = 0; i < ProcNum; i++)
	{
		if (i % 2 == 1)
		{
			if (ProcRank % 2 == 1)
			{
				if (ProcRank < ProcNum - 1)
				{
					exchangeArray(ProcRank, ProcRank + 1, procBlockSize, procArray);

				}
			}
			else
			{
				if (ProcRank > 0)
				{
					exchangeArray(ProcRank, ProcRank - 1, procBlockSize, procArray);
				}
			}
		}
		else
		{
			if (ProcRank % 2 == 0)
			{
				if (ProcRank < ProcNum - 1)
				{
					exchangeArray(ProcRank, ProcRank + 1, procBlockSize, procArray);
				}
			}
			else
			{
				exchangeArray(ProcRank, ProcRank - 1, procBlockSize, procArray);
			}
		}
	}

	if (ProcRank != 0)
	{
		MPI_Send(&procBlockSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
		MPI_Send(procArray, procBlockSize, MPI_INT, 0, 0, MPI_COMM_WORLD);
	}
	if (ProcRank == 0)
	{
		parallelSortBubble = new int[arraySize];
		int start = procBlockSize;
		for (int i = 0; i < procBlockSize; i++)
		{
			parallelSortBubble[i] = procArray[i];
		}
		for (int i = 1; i < ProcNum; i++)
		{
			int anotherProcBlockSize;
			MPI_Recv(&anotherProcBlockSize, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &Status);
			int* anotherProcArray = new int[anotherProcBlockSize];
			MPI_Recv(anotherProcArray, anotherProcBlockSize, MPI_INT, i, 0, MPI_COMM_WORLD, &Status);

			for (int j = 0; j < anotherProcBlockSize; j++)
			{
				parallelSortBubble[start + j] = anotherProcArray[j];
			}
			start += anotherProcBlockSize;
			delete[] anotherProcArray;
		}
		parallelTotalTime = MPI_Wtime() - parallelStart;
		cout << "\nTime(Parallel realization):" << parallelTotalTime << '\n';
	}

	if (ProcRank == 0)
	{
		bool equal = true;
		for (int i = 0; i < arraySize; i++)
		{
			if (serialSortBubble[i] != parallelSortBubble[i])
			{
				equal = false;
				break;
			}
		}
		if (equal)
		{
			cout << "\nsorted arrays are equal!!!\n";
		}
		else
		{
			cout << "\nsorted arrays are not equal!!!\n";
			printArray(serialSortBubble, arraySize);
			printf("\n");
			printArray(parallelSortBubble, arraySize);
		}
		if (parallelTotalTime == 0 || serialTotalTime == 0)
		{
			cout << "\nNot enough information to calculate the acceleration!\n";
		}
		else {
			 if (serialTotalTime > parallelTotalTime)
			 {
				cout << "\nParallel version faster on " << serialTotalTime / parallelTotalTime << "%" << '\n';;
			 }
			 else {
				 cout << "\nSequence version faster on " << parallelTotalTime / serialTotalTime << "%" << '\n';
			 }
		}
		cout << "\n**************************************************************\n";
		delete[] serialSortBubble;
		delete[] parallelSortBubble;
	}

	MPI_Finalize();
	return 0;
}
