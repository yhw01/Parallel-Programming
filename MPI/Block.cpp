#include <bits/stdc++.h>
#include <mpi.h>
#include <sys/time.h>
using namespace std;

// 矩阵规模
const int N = 1024;

void Gauss_plain(float **B)
{
	for (int k = 0; k < N; k++)
	{
		for (int j = k + 1; j < N; j++)
		{
			B[k][j] = B[k][j] / B[k][k];
		}
		B[k][k] = 1.0;
		for (int i = k + 1; i < N; i++)
		{
			for (int j = k + 1; j < N; j++)
			{
				B[i][j] = B[i][j] - B[i][k] * B[k][j];
			}
			B[i][k] = 0;
		}
	}
}

void comp(float **A, float **B)
{
    for (int i = 0; i < N; i++)
    {
        for (int j = i; j < N; j++)
        {
            if (fabs(A[i][j] - B[i][j]) > 1e-4)
            {
                cout << "error > 1e-4:";
                cout << fabs(A[i][j] - B[i][j]) << endl;
                return;
            }
        }
    }
    cout << "correct!" << endl;
}


int main(int argc, char **argv)
{
	// 初始化
	MPI_Init(&argc, &argv);
	// rank 当前进程，size 进程数量
	int rank, size;
	MPI_Status status;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	double start_time = 0;
	double end_time = 0;
	float **A, **B;
	// 0 号节点初始化矩阵
	if (rank == 0)
	{
		cout << "N = " << N << " Number of Process: " << size << endl;
		A = new float *[N];
		for (int i = 0; i < N; i++)
			A[i] = new float[N];
		start_time = MPI_Wtime();
		//初始化矩阵
		for (int i = 0; i < N; i++)
		{
			for (int j = 0; j < N; j++)
			{
				if (i == j)
					A[i][j] = 1;
				else
					A[i][j] = 2;
			}
		}
	}
	else
	{
		A = new float *[1];
		A[0] = new float[1];
	}
	// 分配至各进程的子矩阵大小为n * N
	int n = N / size;
	float **subA = new float *[n];
	for (int i = 0; i < n; i++)
		subA[i] = new float[N];
	// 0 号节点划分
	if (rank == 0)
	{
		for (int i = 0; i < n; i++)
			for (int j = 0; j < N; j++)
				subA[i][j] = A[i][j];
		for (int i = n; i < N; i++)
		{
			//块划分
			MPI_Send(&A[i][0], N, MPI_FLOAT, i / n, i % n, MPI_COMM_WORLD);
		}
		// cout << "subA:" << endl;
		// for (int i = 0; i < n; i++)
		// {
		//     for (int j = 0; j < N; j++)
		//         cout << subA[i][j] << " ";
		//     cout << endl;
		// }
	}
	else
	{
		for (int i = 0; i < n; i++)
			MPI_Recv(&subA[i][0], N, MPI_FLOAT, 0, i, MPI_COMM_WORLD, &status);
	}
	// 初始矩阵通信完毕，开始计算
	// 首行元素
	float *line = new float[N];
	for (int i = 0; i < N; i++)
	{
		if (rank == i / n)
		{
			int row = i % n;
			for (int k = i + 1; k < N; k++)
			{
				subA[row][k] /= subA[row][i];
				line[k] = subA[row][k];
			}
			subA[row][i] = 1;
			line[i] = 1;
			// cout << "line "<< row <<" to be bcasted: " << endl;
			// for (int t = 0; t < N; t++)
			// 	cout << line[t] << " ";
			// cout << endl;
		}
		MPI_Bcast(line, N, MPI_FLOAT, i / n, MPI_COMM_WORLD);
		for (int j = i + 1; j < N; j++)
		{
			if (rank * n + n < j)
				continue;
			// 右下角消去计算
			if (rank * n <= j && rank * n + n > j)
			{
				int row = j % n;
				for (int k = i + 1; k < N; k++)
					subA[row][k] = subA[row][k] - subA[row][i] * line[k];
				subA[row][i] = 0;
			}
		}
	}
	// rank=0 的进程收集计算结果
	if (rank == 0)
	{
		for (int i = 1; i < size; i++)
		{
			for (int j = 0; j < n; j++)
			{
				MPI_Recv(&A[i * n + j][0], N, MPI_FLOAT, i, j, MPI_COMM_WORLD, &status);
			}
		}

		for (int i = 0; i < n; i++)
			for (int j = 0; j < N; j++)
				A[i][j] = subA[i][j];
		end_time = MPI_Wtime();
		cout << "Running Time: " << (end_time - start_time) * 1000 << "ms" << endl;

		// 检验结果
		B = new float *[N];
		for (int i = 0; i < N; i++)
		{
			B[i] = new float[N];
		}
		timeval tv_start, tv_end;
    	gettimeofday(&tv_start, NULL);
		for (int i = 0; i < N; i++)
		{
			for (int j = 0; j < N; j++)
				if (i == j)
					B[i][j] = 1;
				else
					B[i][j] = 2;
		}
		for (int k = 0; k < N; k++)
		{
			for (int j = k + 1; j < N; j++)
			{
				B[k][j] = B[k][j] / B[k][k];
			}
			B[k][k] = 1.0;
			for (int i = k + 1; i < N; i++)
			{
				for (int j = k + 1; j < N; j++)
				{
					B[i][j] = B[i][j] - B[i][k] * B[k][j];
				}
				B[i][k] = 0;
			}
		}
		gettimeofday(&tv_end, NULL);
    	cout << "Plain running time: " << 1000 * (tv_end.tv_sec - tv_start.tv_sec) + (tv_end.tv_usec - tv_start.tv_usec) / 1000 << "ms" << endl;
		comp(A, B);
	}
	else
	{
		for (int i = 0; i < n; i++)
			MPI_Send(&subA[i][0], N, MPI_FLOAT, 0, i, MPI_COMM_WORLD);
	}
	MPI_Finalize();
	return 0;
}