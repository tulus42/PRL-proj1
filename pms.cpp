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

bool send_num(vector<int> left_queue, vector<int> right_queue, int next_rank, int tag) {
    // SEND to next processor
    int num_L;
    int num_R;

    if (left_queue.size() == 0) {
        num_R = right_queue.back();
        MPI_Send(&num_R, 1, MPI_INT, next_rank, tag, COMM);
        if (next_rank-1 == 2)
        // printf("proc %d, sent: %d, tag: %d\n", next_rank-1, num_R, tag);
        return false;
    }
    else if (right_queue.size() == 0) {
        num_L = left_queue.back();
        MPI_Send(&num_L, 1, MPI_INT, next_rank, tag, COMM);
        if (next_rank-1 == 2)
        // printf("proc %d, sent: %d, tag: %d\n", next_rank-1, num_L, tag);
        return true;
    }

    num_L = left_queue.back();
    num_R = right_queue.back();

    if (num_L > num_R) {
        MPI_Send(&num_L, 1, MPI_INT, next_rank, tag, COMM);
        if (next_rank-1 == 2)
        // printf("proc %d, sent: %d, tag: %d\n", next_rank-1, num_L, tag);
        return true;
    }
    else {
        MPI_Send(&num_R, 1, MPI_INT, next_rank, tag, COMM);
        if (next_rank-1 == 2)
        // printf("proc %d, sent: %d, tag: %d\n", next_rank-1, num_R, tag);
        return false;
    }
}

bool save_num(vector<int> left_queue, vector<int> right_queue) {
    int num_L;
    int num_R;

    if (left_queue.size() == 0) {
        return false;
    }
    else if (right_queue.size() == 0) {
        return true;
    }
    
    num_L = left_queue.back();
    num_R = right_queue.back();

    if (num_L > num_R) {
        return true;
    }
    else {
        return false;
    }
}


void first_proc(vector<int> numbers, int numbers_size) {
    int last_num;
    int next_rank = 1;

    for (int i=0; i < numbers_size; i++)
        printf("%d ", numbers[i]);
    printf("\n");

    // printf("SIZE: %d\n", numbers_size);
    for (int i=0; i < numbers_size; i++) {
        last_num = numbers.back();
        // printf("SENDING %d. num: %d\n", i, last_num);
        MPI_Send(&last_num, 1, MPI_INT, next_rank, i, COMM);

        numbers.pop_back();

    }
    printf("MASTER END\n");
}


void other_procs(int rank, int num_size) {
    bool left_queue_flag = true;
    int rcvd_num;
    int next_rank = rank + 1;
    int length_of_cycle = (int)pow(2, rank-1);
    int start_i = length_of_cycle + 1;
    vector<int> left_queue;
    vector<int> right_queue;



    for (int i=0; i < num_size+start_i; i++) {

        // print_queues(left_queue, right_queue, rank);

        // RECEIVE number
        if (i < num_size) {

            MPI_Recv(&rcvd_num, 1, MPI_INT, rank-1, i, COMM, nullptr);

            if (rank == 2)
                print_queues(left_queue, right_queue, rank);
        

            // add number into one of queues
            if (left_queue_flag) {
                // printf("Proc %d, i:%d, L\n", rank, i);
                left_queue.insert(left_queue.begin(), rcvd_num);
            }
            else {
                // printf("Proc %d, i:%d, R\n", rank, i);
                right_queue.insert(right_queue.begin(), rcvd_num);
            }
        
            // switch between LEFT/RIGHT queue
            if ((i+1) % length_of_cycle == 0) {
                left_queue_flag = !left_queue_flag;
            }
        }
    

        // SEND number
        if (i >= start_i && (left_queue.size() > 0 || right_queue.size() > 0)) {
            bool left_bigger = send_num(left_queue, right_queue, next_rank, i-start_i);

            if (left_bigger) {
                left_queue.pop_back();
            }
            else {
                right_queue.pop_back();
            }
        }

        // printf("End of %d iteration\n", i);
    }



    printf("PROC %d END\n", rank);
}

void last_proc(int rank, long num_size) {
    bool left_queue_flag = true;
    int rcvd_num;
    int next_rank = rank + 1;
    int length_of_cycle = (int)pow(2, rank-1);
    int start_i = length_of_cycle + 1;
    vector<int> left_queue;
    vector<int> right_queue;
    vector<int> res;
    
    for (int i=0; i < num_size+start_i; i++) {

        // print_queues(left_queue, right_queue, rank);

        // RECEIVE number
        if (i < num_size) {

            MPI_Recv(&rcvd_num, 1, MPI_INT, rank-1, i, COMM, nullptr);        

            // add number into one of queues
            if (left_queue_flag) {
                // printf("Proc %d, i:%d, L\n", rank, i);
                left_queue.insert(left_queue.begin(), rcvd_num);
            }
            else {
                // printf("Proc %d, i:%d, R\n", rank, i);
                right_queue.insert(right_queue.begin(), rcvd_num);
            }
        
            // switch between LEFT/RIGHT queue
            if ((i+1) % length_of_cycle == 0) {
                left_queue_flag = !left_queue_flag;
            }
        }

        // SAVE number
        if (i >= start_i && (left_queue.size() > 0 || right_queue.size() > 0)) {
            bool left_bigger = save_num(left_queue, right_queue);

            if (left_bigger) {
                res.insert(res.begin(), left_queue.back());
                left_queue.pop_back();
            }
            else {
                res.insert(res.begin(), right_queue.back());
                right_queue.pop_back();
            }
        }
    }


    for (int i=0; i < num_size; i++) {
        printf("%d ", res[i]);
    }
    printf("\n");
    printf("LAST PROC END\n");

}


vector<int> do_sort(int rank, int size, vector<int> numbers, int numbers_size) {

    if (rank == 0) {
        first_proc(numbers, numbers_size);
    }
    else if (rank == size-1) {
        last_proc(rank, numbers_size);
    }
    else {
        other_procs(rank, numbers_size);
    }

    return numbers;
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