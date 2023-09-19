#include <mpi.h>
#include <fstream>
#include <queue>

using namespace std;

#define TAG 0
#define TO_RCV 16


// First process load numbers from file and send them to a second process
void start_process()
{
    int num;
    char input[] = "numbers";                   //jmeno souboru 
    fstream fin;                                //cteni ze souboru
    fin.open(input, ios::in);                   

    // first print without space
    num = fin.get();
    if (fin.good()) {
        MPI_Send(&num, 1, MPI_INT, 1, TAG, MPI_COMM_WORLD);
        cout << num;
    }
    // print next numbers
    num = fin.get();
    while (fin.good()) {
        MPI_Send(&num, 1, MPI_INT, 1, TAG, MPI_COMM_WORLD);
        cout << " " << num;
        num = fin.get();
    }

    cout << endl;
    fin.close();                                
}

// Middle processes receives numbers from previous process and send them more ordered to next process
void middle_process(int myid)
{
    MPI_Status stat;
    queue<int> nums_top;
    queue<int> nums_bot;
    int mynumber;
    int min_front = 1 << (myid - 1);
    int received = 0, top = 0, bot = 0;
    bool topB = false; // top vs bot queue

    // Load first queue
    while (received < min_front) {
        MPI_Recv(&mynumber, 1, MPI_INT, myid - 1, TAG, MPI_COMM_WORLD, &stat);
        nums_top.push(mynumber);
        received++;
    }

    // Load rest of numbers
    while (received < TO_RCV) {
        for (int i = 0; i < min_front; i++, received++) {
            MPI_Recv(&mynumber, 1, MPI_INT, myid - 1, TAG, MPI_COMM_WORLD, &stat);
            if (topB)
                nums_top.push(mynumber);
            else
                nums_bot.push(mynumber);
            
            if (top == min_front && bot == min_front) {
                top = 0;
                bot = 0;
            }

            if (top == min_front || (bot != min_front && nums_top.front() > nums_bot.front())) {
                bot++;
                mynumber = nums_bot.front();
                nums_bot.pop();
            } else {
                top++;
                mynumber = nums_top.front();
                nums_top.pop();
            }

            MPI_Send(&mynumber, 1, MPI_INT, myid + 1, TAG, MPI_COMM_WORLD);
        }
        topB = !topB;
    }

    // Send rest of numbers
    while (!nums_top.empty()) {
        if (!nums_bot.empty() && nums_bot.front() < nums_top.front()) {
            mynumber = nums_bot.front();
            nums_bot.pop();
        } else {
            mynumber = nums_top.front();
            nums_top.pop();
        }
        MPI_Send(&mynumber, 1, MPI_INT, myid + 1, TAG, MPI_COMM_WORLD);
    }

    while (!nums_bot.empty()) {
        mynumber = nums_bot.front();
        nums_bot.pop();
        MPI_Send(&mynumber, 1, MPI_INT, myid + 1, TAG, MPI_COMM_WORLD);
    }
}

// End process (last process) receives data from previous process and write them ordered
void end_process(int myid)
{
    MPI_Status stat;
    queue<int> nums_top;
    queue<int> nums_bot;
    int min_front = 1 << (myid - 1);
    int mynumber;
    
    //Receive first half of numbers
    for (int i = 0; i < min_front; i++) {
        MPI_Recv(&mynumber, 1, MPI_INT, myid - 1, TAG, MPI_COMM_WORLD, &stat);
        nums_top.push(mynumber);
    }

    // Receive rest of numbers, write first half
    for (int i = min_front; i < TO_RCV; i++) {
        MPI_Recv(&mynumber, 1, MPI_INT, myid - 1, TAG, MPI_COMM_WORLD, &stat);
        nums_bot.push(mynumber);
        if (nums_top.front() > nums_bot.front()) {
            cout << nums_bot.front() << endl;
            nums_bot.pop();
        } else {
            cout << nums_top.front() << endl;
            nums_top.pop();
        }
    }
    // Write rest of numbers   
    while (!nums_top.empty()) {
        if (!nums_bot.empty() && nums_bot.front() < nums_top.front()) {
            cout << nums_bot.front() << endl;
            nums_bot.pop();
        } else {
            cout << nums_top.front() << endl;
            nums_top.pop();
        }
    }

    while (!nums_bot.empty()) {
        cout << nums_bot.front() << endl;
        nums_bot.pop();
    }
}

int main(int argc, char *argv[])
{
    int numprocs;               //pocet procesoru
    int myid;                   //muj rank

    //MPI INIT
    MPI_Init(&argc, &argv);                         // inicializace MPI 
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);       // zjistime, kolik procesu bezi 
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);           // zjistime id sveho procesu 
 
    if (myid == 0)
        start_process();
    else if (myid != numprocs - 1)
        middle_process(myid);
    else
        end_process(myid);
 
    MPI_Finalize();
    return 0;
}
