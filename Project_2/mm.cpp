#include <mpi.h>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

#define DOWN 0
#define RIGHT 1
#define RESULT 2

using namespace std;

// load matrix from file filename and return number at beggining of file
int load_matrix(char const *filename, vector<vector<int>> *mtx)
{
    int num, ret = 0;
    string line;
    ifstream input;
    input.open(filename);

    if (input.is_open()) {
        // load first number on first line in file
        getline(input, line);
        ret = stoi(line);
        // read numbers in rest of file by lines
        for (int i = 0; getline(input, line); i++) {
            (*mtx).resize(i+1);
            istringstream iss(line);
            while (iss >> num) {
                (*mtx)[i].push_back(num);
            }
        }
        input.close();
    }

    return ret;
}


int main(int argc, char *argv[])
{
    int numprocs, myid;
    int rows, cols, shared;
    int num1, num2, res = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Status stat;

    // first process loads and send numbers
    if (myid == 0) {
        vector<vector<int>> mat1, mat2;
        rows = load_matrix("mat1", &mat1);
        cols = load_matrix("mat2", &mat2);
        shared = mat2.size();
        
        // send numbers to processes at top and left side
        for (int i = 0; i < shared; i++) {
            for (int j = 0; j < rows; j++) {
                num1 = mat1[j][i];
                MPI_Send(&num1, 1, MPI_INT, j * cols, RIGHT, MPI_COMM_WORLD);
            }
            for (int j = 0; j < cols; j++) {
                num2 = mat2[i][j];
                MPI_Send(&num2, 1, MPI_INT, j, DOWN, MPI_COMM_WORLD);
            }
        }
    }
  
    // synchronization of dimensions of matrixes
    MPI_Bcast(&rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&cols, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&shared, 1, MPI_INT, 0, MPI_COMM_WORLD);


    // all processes receive pairs of values to compute result and send numbers to next processes
    for (int i = 0; i < shared; i++) {
        if (myid % cols == 0)
            MPI_Recv(&num1, 1, MPI_INT, 0, RIGHT, MPI_COMM_WORLD, &stat);
        else
            MPI_Recv(&num1, 1, MPI_INT, myid - 1, RIGHT, MPI_COMM_WORLD, &stat);
        
        if (myid % cols < cols - 1)
            MPI_Send(&num1, 1, MPI_INT, myid + 1, RIGHT, MPI_COMM_WORLD);

        if (myid < cols)
            MPI_Recv(&num2, 1, MPI_INT, 0, DOWN, MPI_COMM_WORLD, &stat);
        else
            MPI_Recv(&num2, 1, MPI_INT, myid - cols, DOWN, MPI_COMM_WORLD, &stat);

        if (myid < numprocs - cols)
            MPI_Send(&num2, 1, MPI_INT, myid + cols, DOWN, MPI_COMM_WORLD);
        
        res += num1 * num2;
    }

    // processes send their results to the first process
    MPI_Send(&res, 1, MPI_INT, 0, RESULT, MPI_COMM_WORLD);
    
    // first process prints results
    if (myid == 0) {
        cout << rows << ':' << cols << endl;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                MPI_Recv(&res, 1, MPI_INT, i * cols + j, RESULT, MPI_COMM_WORLD, &stat);
                cout << res << (j == cols - 1 ? '\n' : ' ');
            }
        }
    }

    MPI_Finalize();
    return 0;
}
