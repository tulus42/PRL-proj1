#include <fstream>
#include <iterator>
#include <vector>
#include <stdio.h>
#include <mpi.h>

#define COMM MPI_COMM_WORLD
#define TAG 0
#define MASTER 0

using namespace std;


vector<int> load_input() {
    ifstream input("numbers", ios::binary);

    vector<char> loaded(
         (istreambuf_iterator<char>(input)),
         (istreambuf_iterator<char>()));

    input.close();
    
    vector<int> res;
    for (int i=0; i < loaded.size(); i++) {
        res.push_back(loaded[i]);
    }

    return res;
}


void first_proc(vector<int> numbers) {
    int last_num;
    const int next_rank = 1;

    for (int i = 0; i < numbers.size(); i++)
        printf("%d ", numbers[i]);
    printf("\n");

    for (int i=0; i < numbers.size(); i++) {
        last_num = numbers.back();
        MPI_Send(&last_num, 1, MPI_INT, next_rank, TAG, COMM);
        numbers.pop_back();
    }
}


void other_procs(int rank, int num_size) {
    int rcvd_num;
    bool left_queue_flag = true;
    int next_rank = rank + 1;
    vector<char> left_queue;
    vector<char> right_queue;


    // Receive number
    for (int i=0; i < num_size; i++) {
        MPI_Recv(&rcvd_num, 1, MPI_CHAR, rank-1, TAG, COMM, nullptr);
        printf("proc %d, rcvd: %d\n", rank, rcvd_num);
        // add number into one of queues
        if (left_queue_flag) {
            left_queue.insert(left_queue.begin(), rcvd_num);
        }
        else {
            right_queue.insert(right_queue.begin(), rcvd_num);
        }

        left_queue_flag = !left_queue_flag;
    }

    if (rank == 1) {
        for (int i = 0; i < left_queue.size(); i++)
            printf("%i ", left_queue[i]);
        printf("\n");
        for (int i = 0; i < right_queue.size(); i++)
            printf("%i ", right_queue[i]);
        printf("\n");
    }

    for (int i=0; i < num_size; i++) {
        MPI_Send(&rcvd_num, 1, MPI_INT, next_rank, TAG, COMM);
    }
}

vector<int> last_proc(int rank, int num_size) {
    int rcvd_num;


    for (int i=0; i < num_size; i++) {
        MPI_Recv(&rcvd_num, 1, MPI_CHAR, rank-1, TAG, COMM, nullptr);
    }
}


vector<int> do_sort(int rank, int size, vector<int> numbers) {
    if (rank == 0) {
        printf("master proc\n");
        first_proc(numbers);
    }
    else if (rank == size-1) {
        printf("last proc\n");
        return last_proc(rank, numbers.size());
    }
    else {
        printf("proc %d\n", rank);
        other_procs(rank, numbers.size());
    }

    return numbers;
}


int main(int argc, char** argv){
    // TODO odstranit odriadkovanie
    printf("\n");
    // ----------------------------
    
    MPI_Init(&argc, &argv);
    int rank;
    int size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Load input number into vector
    vector<int> numbers;
    if (rank == 0) {
        numbers = load_input(); 
    }

    // sorting
    vector<int> result = do_sort(rank, size, numbers);

    // handle 
    if (rank == size-1) {
        printf("Vysledok - hura\n");
    }

    MPI_Finalize();

    return 0;
}