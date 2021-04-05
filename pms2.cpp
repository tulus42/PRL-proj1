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


void print_queues(vector<int> left_queue, vector<int> right_queue, int rank) {
    printf("# RANK: %d #########\n", rank);
    printf("# LEFT: \n# ");
    for (long i=0; i < left_queue.size(); i++)
        printf("%d ", left_queue[i]);
    printf("\n");

    printf("# RIGHT: \n# ");
    for (long i=0; i < right_queue.size(); i++)
        printf("%d ", right_queue[i]);
    printf("\n\n");
}


int calculate_wait(int rank) {
    
    int wait = 0;
    for (int i=0; i<rank; i++) {
        wait += (int)pow(2, i) +1;
    }

    return wait;
}


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


int get_max(vector<int>* left, vector<int>* right) {
    int num;
    if (right->size() == 0) {
        num = left->back();
        left->pop_back();
    }
    else if (left->size() == 0) {
        num = right->back();
        right->pop_back();
    }
    else if ( left->back() > right->back()) {
        num = left->back();
        left->pop_back();
    }
    else {
        num = right->back();
        right->pop_back();
    }

    return num;
}


void first_proc(int len_of_sort, vector<int> numbers, int numbers_size) {
    int last_num;

    for (int i=0; i<numbers_size; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");

    for (int i=0; i<len_of_sort; i++) {
        MPI_Barrier(COMM);

        if (i < numbers_size) {
            last_num = numbers.back();
            // printf("SENDING %d. num: %d\n", i, last_num);
            MPI_Send(&last_num, 1, MPI_INT, MASTER+1, TAG, COMM);

            numbers.pop_back();
        }
    }
}


void other_procs(int len_of_sort, int rank, int num_size, bool last_proc) {
    bool left_queue_flag = true;
    int rcvd_num;
    int length_of_cycle = (int)pow(2, rank-1);
    int start_i = calculate_wait(rank);
    int send_i = start_i + length_of_cycle + 1;
    vector<int> left_queue;
    vector<int> right_queue;
    vector<int> res;

    for (int i=0; i<len_of_sort; i++) {
        MPI_Barrier(COMM);

        if (start_i <= i && i < start_i+num_size) {
            
            MPI_Recv(&rcvd_num, 1, MPI_INT, rank-1, TAG, COMM, nullptr);
            
            if (rank == 3)
                printf("RCVD %d\n", rcvd_num);

            left_queue_flag = write_to_queue(&left_queue, &right_queue, rcvd_num, i-start_i, length_of_cycle, left_queue_flag);
        }

        if (send_i <= i  && i < send_i+num_size ) {
            if (!last_proc) {
            
                if (left_queue.size() > 0 || right_queue.size() > 0) {
                    int snd_num = get_max(&left_queue, &right_queue);

                    MPI_Send(&snd_num, 1, MPI_INT, rank+1, TAG, COMM);
                    if (rank == 3)
                        printf("SEND %d\n", snd_num);
                }
            }
        
            else {
                int num = get_max(&left_queue, &right_queue);
                res.insert(res.begin(), num);
                printf("%d ", num);
            }
        }
    }

}




vector<int> do_sort(int rank, int proc_size, vector<int> numbers, int numbers_size) {

    int len_of_sort = (int)pow(2, proc_size) + proc_size + (numbers_size - 1);

    if (rank == 0) {
        first_proc(len_of_sort, numbers, numbers_size);
    }
    else if (rank == proc_size-1) {
        other_procs(len_of_sort, rank, numbers_size, true);
    }
    else {
        other_procs(len_of_sort, rank, numbers_size, false);
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

    int numbers_size = distribute_size(rank, size, numbers);
    
    // sorting
    vector<int> result = do_sort(rank, size, numbers, numbers_size);

    // handle 
    if (rank == size-1) {
        printf("Vysledok - hura\n");
    }

    MPI_Finalize();

    return 0;
}