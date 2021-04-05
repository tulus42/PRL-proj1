#include <fstream>
#include <iterator>
#include <vector>
#include <stdio.h>
#include <mpi.h>
#include <math.h>

#define COMM MPI_COMM_WORLD
#define TAG 0
#define MASTER 0

using namespace std;


bool write_to_queue(vector<int>* left, vector<int>* right, int num, int i, int length_of_cycle, bool left_queue_flag) {
    if (left_queue_flag) {
        left->insert(left->begin(), num);
    }
    else {
        right->insert(right->begin(), num);
    }

    // switch between LEFT/RIGHT queue
    if ((i+1) % length_of_cycle == 0) {
        left_queue_flag = !left_queue_flag;
    }
    return left_queue_flag;
}


int get_max(vector<int>* left, vector<int>* right, int* left_cnt, int* right_cnt) {
    int num;
    bool left_flag;

    if (right->size() == 0 || *right_cnt == 0) {
        left_flag = true;
    }
    else if (left->size() == 0 || *left_cnt == 0) {
        left_flag = false;
    }
    else if ( left->back() > right->back()) {
        left_flag = true;
    }
    else {
        left_flag = false;
    }


    if (left_flag) {
        num = left->back();
        left->pop_back();
        *left_cnt -= 1;
    }
    else {
        num = right->back();
        right->pop_back();
        *right_cnt -= 1;
    }

    return num;
}


void first_proc(vector<int> numbers, int numbers_size) {
    int last_num;

    // Print numbers to STD-out
    for (int i=0; i<numbers_size; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");

    for (int i=0; i<numbers_size; i++) {

        last_num = numbers.back();
        // printf("SENDING %d. num: %d\n", i, last_num);
        MPI_Send(&last_num, 1, MPI_INT, MASTER+1, i, COMM);

        numbers.pop_back();
    }
}


vector<int> other_procs(int rank, int num_size, bool last_proc) {
    bool left_queue_flag = true;
    int rcvd_num;
    int length_of_cycle = (int)pow(2, rank-1);
    int start_i = length_of_cycle + 1;
    int left_cnt = length_of_cycle;
    int right_cnt =  length_of_cycle;
    vector<int> left_queue;
    vector<int> right_queue;
    vector<int> res;

    for (int i=0; i<num_size+start_i; i++) {
       
        // RECIEVE
        if (i < num_size) {
            
            MPI_Recv(&rcvd_num, 1, MPI_INT, rank-1, i, COMM, nullptr);
            
            // if (rank == 3)
            //     printf("RCVD %d\n", rcvd_num);

            left_queue_flag = write_to_queue(&left_queue, &right_queue, rcvd_num, i, length_of_cycle, left_queue_flag);
        }

        // SEND
        if (i >= start_i) {
            // OTHER procs
            if (!last_proc) {
            
                if (left_queue.size() > 0 || right_queue.size() > 0) {
                    int snd_num = get_max(&left_queue, &right_queue, &left_cnt, &right_cnt);

                    MPI_Send(&snd_num, 1, MPI_INT, rank+1, i-start_i, COMM);
                }
            }
        
            // LAST proc
            else {
                int num = get_max(&left_queue, &right_queue, &left_cnt, &right_cnt);
                res.insert(res.begin(), num);
            }

            // used whole subsequence
            if (left_cnt == 0 && right_cnt == 0) {
                left_cnt = length_of_cycle;
                right_cnt = length_of_cycle;
            }
        }
    }

    return res;
}


vector<int> do_sort(int rank, int proc_size, vector<int> numbers, int numbers_size) {

    if (rank == 0) {
        first_proc(numbers, numbers_size);
    }
    else if (rank == proc_size-1) {
        numbers = other_procs(rank, numbers_size, true);
    }
    else {
        other_procs(rank, numbers_size, false);
    }
    
    return numbers;
}


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


int distribute_size(int rank, int size, vector<int> numbers) {
    int num_size;

    if (rank == MASTER) {
        num_size = (int)numbers.size();
        for (int i=1; i < size; i++) {
            MPI_Send(&num_size, 1, MPI_INT, i, TAG, COMM);
        }
        
    }
    else { 
        MPI_Recv(&num_size, 1, MPI_INT, MASTER, TAG, COMM, nullptr);
    }

    return num_size;
}


int main(int argc, char** argv){
    
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

    int numbers_size = distribute_size(rank, size, numbers);
    
    // sorting
    vector<int> result = do_sort(rank, size, numbers, numbers_size);

    // handle 
    if (rank == size-1) {
        for (int i=0; i < numbers_size; i++) {
            printf("%d\n", result[i]);
        }
    }

    MPI_Finalize();

    return 0;
}